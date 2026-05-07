#!/usr/bin/env python3
"""Disassemble an Alpha SRM decompressed ROM image at firmware PC addresses."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


AXPBOX_HEADER_SIZE = 16
AXPBOX_MEMORY_SIZE = 0x200000


def parse_u64(text: str) -> int:
    try:
        return int(text, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"invalid integer: {text}") from exc


def find_objdump(path: Path | None) -> Path | None:
    if path is not None:
        return path
    env_path = os.environ.get("ALPHA_OBJDUMP")
    if env_path:
        return Path(env_path)
    found = shutil.which("alpha-linux-gnu-objdump")
    if found:
        return Path(found)
    fallback = Path("/tmp/binutils-alpha/usr/bin/alpha-linux-gnu-objdump")
    if fallback.exists():
        return fallback
    return None


def objdump_env(objdump: Path) -> dict[str, str]:
    env = os.environ.copy()
    if str(objdump).startswith("/tmp/binutils-alpha/usr/bin/"):
        lib = "/tmp/binutils-alpha/usr/lib/x86_64-linux-gnu"
        env["LD_LIBRARY_PATH"] = (
            f"{lib}:{env['LD_LIBRARY_PATH']}"
            if env.get("LD_LIBRARY_PATH")
            else lib
        )
    return env


def axpbox_header_size(path: Path) -> int:
    size = path.stat().st_size
    if size == AXPBOX_HEADER_SIZE + AXPBOX_MEMORY_SIZE:
        return AXPBOX_HEADER_SIZE
    return 0


def copy_memory_image(path: Path, header_size: int, tmpdir: Path) -> Path:
    if header_size == 0:
        return path
    out = tmpdir / "srm-memory.bin"
    with path.open("rb") as src, out.open("wb") as dst:
        src.seek(header_size)
        shutil.copyfileobj(src, dst)
    return out


def disassemble(
    objdump: Path,
    image: Path,
    start: int,
    stop: int,
) -> int:
    cmd = [
        str(objdump),
        "-D",
        "-b",
        "binary",
        "-m",
        "alpha",
        f"--start-address=0x{start:x}",
        f"--stop-address=0x{stop:x}",
        str(image),
    ]
    proc = subprocess.run(cmd, env=objdump_env(objdump))
    return proc.returncode


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Disassemble an Alpha SRM memory image using the same PC addresses "
            "SIMH prints. AXPbox decompressed.rom files are detected and their "
            "16-byte PC/PAL_BASE header is stripped before running objdump."
        )
    )
    parser.add_argument("rom", type=Path)
    parser.add_argument("--objdump", type=Path, help="alpha-linux-gnu-objdump path")
    parser.add_argument("--pc", type=parse_u64, help="center disassembly here")
    parser.add_argument("--start", type=parse_u64, help="first firmware PC")
    parser.add_argument("--stop", type=parse_u64, help="first PC after the range")
    parser.add_argument(
        "--bytes",
        type=parse_u64,
        default=0x100,
        help="range size when --pc is used, default 0x100",
    )
    parser.add_argument(
        "--raw",
        action="store_true",
        help="do not strip an AXPbox decompressed.rom header",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    rom = args.rom.resolve()
    if not rom.is_file():
        print(f"ROM image not found: {rom}", file=sys.stderr)
        return 1

    objdump = find_objdump(args.objdump)
    if objdump is None or not os.access(objdump, os.X_OK):
        print(
            "alpha-linux-gnu-objdump was not found; set ALPHA_OBJDUMP or use --objdump",
            file=sys.stderr,
        )
        return 1

    if args.pc is not None:
        start = args.pc - min(args.pc, args.bytes // 2)
        stop = args.pc + (args.bytes // 2)
    else:
        start = args.start if args.start is not None else 0
        stop = args.stop if args.stop is not None else start + args.bytes
    if args.start is not None:
        start = args.start
    if args.stop is not None:
        stop = args.stop
    start &= ~3
    stop = (stop + 3) & ~3
    if stop <= start:
        print("--stop must be greater than --start", file=sys.stderr)
        return 2

    header_size = 0 if args.raw else axpbox_header_size(rom)
    with tempfile.TemporaryDirectory(prefix="srm-disasm.") as tmp_text:
        image = copy_memory_image(rom, header_size, Path(tmp_text))
        if header_size:
            print(f"{rom}: stripped 0x{header_size:x}-byte AXPbox header", flush=True)
        print(f"firmware PC range: 0x{start:x}..0x{stop:x}", flush=True)
        return disassemble(objdump.resolve(), image, start, stop)


if __name__ == "__main__":
    raise SystemExit(main())
