#!/usr/bin/env python3
"""Extract an AXPbox decompressed SRM ROM image from a DEC LFU ROM file."""

from __future__ import annotations

import argparse
import os
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path


HEADER_SIZE = 0x240
AXPBOX_ROM_SIZE = 16 + 0x200000


def c_string(data: bytes) -> str:
    return data.split(b"\0", 1)[0].decode("ascii", "replace").strip()


def read_header(path: Path) -> dict[str, object]:
    data = path.read_bytes()[:HEADER_SIZE]
    if len(data) < HEADER_SIZE:
        raise ValueError("file is shorter than the LFU ROM header")
    if data[4:12] != b"LFU APU\0":
        raise ValueError("file does not look like a DEC LFU APU ROM image")
    if data[0x23C:0x240] != b"\x44\x33\x22\x11":
        raise ValueError("LFU ROM header magic is not 0x11223344")

    return {
        "version": c_string(data[0x208:0x214]),
        "vendor": c_string(data[0x214:0x218]),
        "platform": c_string(data[0x218:0x220]),
        "arch": c_string(data[0x220:0x228]),
        "image_size": struct.unpack_from("<I", data, 0x22C)[0],
        "kind": c_string(data[0x230:0x238]),
    }


def print_header(path: Path, header: dict[str, object]) -> None:
    print(f"input:      {path}")
    print(f"version:    {header['version']}")
    print(f"vendor:     {header['vendor']}")
    print(f"platform:   {header['platform']}")
    print(f"arch:       {header['arch']}")
    print(f"kind:       {header['kind']}")
    print(f"image_size: 0x{header['image_size']:x}")


def connect_serial(host: str, port: int, stop: threading.Event) -> None:
    deadline = time.monotonic() + 20
    while time.monotonic() < deadline and not stop.is_set():
        try:
            with socket.create_connection((host, port), timeout=1):
                stop.wait(1)
                return
        except OSError:
            time.sleep(0.1)


def write_config(
    cfg: Path,
    in_rom: Path,
    out_rom: Path,
    flash: Path,
    dpr: Path,
    port: int,
) -> None:
    cfg.write_text(
        f"""sys0 = tsunami
{{
  memory.bits = 30;

  rom.srm = "{in_rom}";
  rom.decompressed = "{out_rom}";
  rom.flash = "{flash}";
  rom.dpr = "{dpr}";

  cpu0 = ev68cb
  {{
    speed = 800M;
    icache = false;
  }}

  serial0 = serial
  {{
    address = "127.0.0.1";
    port = {port};
  }}

  pci0.7 = ali
  {{
    vga_console = false;
  }}

  pci0.19 = ali_usb
  {{
  }}
}}
""",
        encoding="utf-8",
    )


def default_axpbox() -> Path:
    script = Path(__file__).resolve()
    workspace = script.parents[3]
    return workspace / "axpbox" / "build" / "axpbox"


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Run AXPbox's SRM self-decompressor path and save the "
            "AXPbox decompressed.rom format: PC, PAL_BASE, then 2 MB memory."
        )
    )
    parser.add_argument("input_rom", type=Path)
    parser.add_argument("output_rom", type=Path, nargs="?")
    parser.add_argument("--axpbox", type=Path, default=default_axpbox())
    parser.add_argument("--port", type=int, default=21364)
    parser.add_argument("--timeout", type=float, default=900)
    parser.add_argument("--max-chunks", type=int)
    parser.add_argument("--trace", action="store_true")
    parser.add_argument("--info-only", action="store_true")
    args = parser.parse_args()

    in_rom = args.input_rom.resolve()
    if not in_rom.is_file():
        print(f"input ROM not found: {in_rom}", file=sys.stderr)
        return 1

    try:
        header = read_header(in_rom)
    except ValueError as exc:
        print(f"{in_rom}: {exc}", file=sys.stderr)
        return 1
    print_header(in_rom, header)
    if args.info_only:
        return 0

    if args.output_rom is None:
        print("output ROM path is required unless --info-only is used", file=sys.stderr)
        return 2

    axpbox = args.axpbox.resolve()
    if not os.access(axpbox, os.X_OK):
        print(f"AXPbox executable not found or not executable: {axpbox}", file=sys.stderr)
        return 1

    out_rom = args.output_rom.resolve()
    out_rom.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="axpbox-rom.") as work_text:
        work = Path(work_text)
        tmp_out = work / "decompressed.rom"
        cfg = work / "rom-extract.cfg"
        log_path = work / "axpbox.log"
        write_config(cfg, in_rom, tmp_out, work / "flash.rom", work / "dpr.rom", args.port)

        env = os.environ.copy()
        env["AXPBOX_ROM_EXTRACT_ONLY"] = "1"
        if args.trace:
            env["AXPBOX_ROM_TRACE_DECOMP"] = "1"
        if args.max_chunks is not None:
            env["AXPBOX_ROM_MAX_CHUNKS"] = str(args.max_chunks)

        stop_serial = threading.Event()
        connector = threading.Thread(
            target=connect_serial, args=("127.0.0.1", args.port, stop_serial)
        )
        connector.daemon = True
        connector.start()

        with log_path.open("wb") as log:
            proc = subprocess.Popen(
                [str(axpbox), "run", str(cfg)],
                stdout=log,
                stderr=subprocess.STDOUT,
                env=env,
            )
            deadline = time.monotonic() + args.timeout
            while proc.poll() is None and time.monotonic() < deadline:
                if tmp_out.exists() and tmp_out.stat().st_size == AXPBOX_ROM_SIZE:
                    proc.terminate()
                    try:
                        proc.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        proc.wait()
                    break
                time.sleep(0.2)
            else:
                if proc.poll() is None:
                    proc.kill()
                    proc.wait()

        stop_serial.set()
        connector.join(timeout=1)

        log_text = log_path.read_text(errors="replace")
        print(log_text[log_text.find("%SYS-I-READROM") :].rstrip())

        if "%SYS-W-DECOMPLIM" in log_text:
            print("AXPbox stopped at the debug chunk limit; output is partial.", file=sys.stderr)
            return 1
        if "%SYS-I-ROMWRT" not in log_text:
            print("AXPbox did not report a completed ROM write.", file=sys.stderr)
            return 1
        if not tmp_out.exists() or tmp_out.stat().st_size != AXPBOX_ROM_SIZE:
            print("AXPbox output has the wrong size.", file=sys.stderr)
            return 1

        shutil.move(str(tmp_out), out_rom)
        print(f"wrote:      {out_rom}")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
