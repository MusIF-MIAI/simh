#!/usr/bin/env python3
"""Keep a SIMH remote/console telnet session alive and expose it as a raw socket."""

from __future__ import annotations

import argparse
import asyncio
import collections
import contextlib
import signal
from typing import Deque, Optional, Set


IAC = 255
DONT = 254
DO = 253
WONT = 252
WILL = 251
SB = 250
SE = 240


class TelnetFilter:
    """Strip telnet negotiation and answer with conservative refusals."""

    def __init__(self, writer: asyncio.StreamWriter) -> None:
        self.writer = writer
        self.state = "data"

    def feed(self, data: bytes) -> bytes:
        out = bytearray()
        for byte in data:
            if self.state == "data":
                if byte == IAC:
                    self.state = "iac"
                else:
                    out.append(byte)
            elif self.state == "iac":
                if byte == IAC:
                    out.append(IAC)
                    self.state = "data"
                elif byte in (DO, DONT, WILL, WONT):
                    self.state = f"cmd:{byte}"
                elif byte == SB:
                    self.state = "sub"
                else:
                    self.state = "data"
            elif self.state.startswith("cmd:"):
                cmd = int(self.state.split(":", 1)[1])
                if cmd == DO:
                    self.writer.write(bytes([IAC, WONT, byte]))
                elif cmd == WILL:
                    self.writer.write(bytes([IAC, DONT, byte]))
                self.state = "data"
            elif self.state == "sub":
                if byte == IAC:
                    self.state = "sub-iac"
            elif self.state == "sub-iac":
                self.state = "data" if byte == SE else "sub"
        return bytes(out)


class ConsoleProxy:
    def __init__(self, args: argparse.Namespace) -> None:
        self.args = args
        self.history: Deque[int] = collections.deque(maxlen=args.history_bytes)
        self.clients: Set[asyncio.StreamWriter] = set()
        self.simh_writer: Optional[asyncio.StreamWriter] = None
        self.stop = asyncio.Event()

    async def run(self) -> None:
        server = await asyncio.start_server(
            self.handle_client, self.args.listen_host, self.args.listen_port
        )
        addrs = ", ".join(str(sock.getsockname()) for sock in server.sockets or [])
        print(f"proxy listening on {addrs}")
        async with server:
            connector = asyncio.create_task(self.connect_loop())
            await self.stop.wait()
            connector.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await connector

    async def connect_loop(self) -> None:
        while not self.stop.is_set():
            try:
                reader, writer = await asyncio.open_connection(
                    self.args.simh_host, self.args.simh_port
                )
            except OSError as exc:
                print(f"waiting for SIMH console: {exc}")
                await asyncio.sleep(self.args.retry_delay)
                continue

            self.simh_writer = writer
            print(f"connected to SIMH telnet at {self.args.simh_host}:{self.args.simh_port}")
            await self.broadcast(b"\r\n[proxy: connected to SIMH]\r\n")
            telnet = TelnetFilter(writer)
            try:
                while not self.stop.is_set():
                    data = await reader.read(4096)
                    if not data:
                        break
                    cooked = telnet.feed(data)
                    if cooked:
                        self.history.extend(cooked)
                        await self.broadcast(cooked)
                    await writer.drain()
            except (ConnectionError, OSError) as exc:
                print(f"SIMH telnet connection lost: {exc}")
            finally:
                self.simh_writer = None
                writer.close()
                with contextlib.suppress(Exception):
                    await writer.wait_closed()
                await self.broadcast(b"\r\n[proxy: SIMH disconnected]\r\n")

            await asyncio.sleep(self.args.retry_delay)

    async def handle_client(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ) -> None:
        peer = writer.get_extra_info("peername")
        print(f"client connected: {peer}")
        self.clients.add(writer)
        try:
            writer.write(b"[proxy: SIMH console proxy]\r\n")
            if self.history:
                writer.write(b"[proxy: replay]\r\n")
                writer.write(bytes(self.history))
                writer.write(b"\r\n[proxy: live]\r\n")
            await writer.drain()
            while not reader.at_eof():
                data = await reader.read(4096)
                if not data:
                    break
                if self.simh_writer is None:
                    writer.write(b"\r\n[proxy: SIMH is not connected]\r\n")
                    await writer.drain()
                    continue
                self.simh_writer.write(data)
                await self.simh_writer.drain()
        except (ConnectionError, OSError):
            pass
        finally:
            self.clients.discard(writer)
            writer.close()
            with contextlib.suppress(Exception):
                await writer.wait_closed()
            print(f"client disconnected: {peer}")

    async def broadcast(self, data: bytes) -> None:
        dead = []
        for writer in self.clients:
            try:
                writer.write(data)
                await writer.drain()
            except (ConnectionError, OSError):
                dead.append(writer)
        for writer in dead:
            self.clients.discard(writer)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Proxy a SIMH remote/console telnet socket through a persistent raw socket."
    )
    parser.add_argument("--simh-host", default="127.0.0.1")
    parser.add_argument("--simh-port", type=int, default=23230)
    parser.add_argument("--listen-host", default="127.0.0.1")
    parser.add_argument("--listen-port", type=int, default=23231)
    parser.add_argument("--history-bytes", type=int, default=256 * 1024)
    parser.add_argument("--retry-delay", type=float, default=0.5)
    return parser.parse_args()


async def main() -> None:
    args = parse_args()
    proxy = ConsoleProxy(args)
    loop = asyncio.get_running_loop()
    for sig in (signal.SIGINT, signal.SIGTERM):
        loop.add_signal_handler(sig, proxy.stop.set)
    await proxy.run()


if __name__ == "__main__":
    asyncio.run(main())
