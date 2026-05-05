#!/usr/bin/env python3
"""Extract the payload from a DEC LFU APU firmware container."""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path


HEADER_SIZE = 0x240
MAGIC_OFF = 4
MAGIC = b"LFU APU\0"
TRAILER_OFF = 0x23C
TRAILER = b"\x44\x33\x22\x11"


def c_string(data: bytes) -> str:
    return data.split(b"\0", 1)[0].decode("ascii", "replace").strip()


def read_header(path: Path) -> tuple[bytes, dict[str, object]]:
    with path.open("rb") as src:
        header = src.read(HEADER_SIZE)
    if len(header) != HEADER_SIZE:
        raise ValueError("file is shorter than the LFU ROM header")
    if header[MAGIC_OFF : MAGIC_OFF + len(MAGIC)] != MAGIC:
        raise ValueError("file does not look like a DEC LFU APU ROM image")
    if header[TRAILER_OFF : TRAILER_OFF + len(TRAILER)] != TRAILER:
        raise ValueError("LFU ROM header trailer is not 44 33 22 11")
    return header, {
        "version": c_string(header[0x208:0x214]),
        "vendor": c_string(header[0x214:0x218]),
        "platform": c_string(header[0x218:0x220]),
        "arch": c_string(header[0x220:0x228]),
        "header_image_size": struct.unpack_from("<I", header, 0x22C)[0],
        "kind": c_string(header[0x230:0x238]),
    }


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as src:
        for chunk in iter(lambda: src.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def print_info(
    path: Path,
    output: Path | None,
    info: dict[str, object],
    payload_size: int,
    trailing_size: int,
) -> None:
    print(f"input:             {path}")
    if output is not None:
        print(f"output:            {output}")
    print(f"version:           {info['version']}")
    print(f"vendor:            {info['vendor']}")
    print(f"platform:          {info['platform']}")
    print(f"arch:              {info['arch']}")
    print(f"kind:              {info['kind']}")
    print(f"header image size: 0x{info['header_image_size']:x}")
    print(f"payload size:      0x{payload_size:x}")
    if trailing_size:
        print(f"trailing bytes:    0x{trailing_size:x}")


def extract_payload(input_path: Path, output_path: Path, payload_size: int) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with input_path.open("rb") as src, output_path.open("wb") as dst:
        src.seek(HEADER_SIZE)
        remaining = payload_size
        while remaining:
            chunk = src.read(min(1024 * 1024, remaining))
            if not chunk:
                raise OSError("input ended while reading payload")
            dst.write(chunk)
            remaining -= len(chunk)


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Validate a DEC LFU APU firmware image and write the embedded "
            "payload bytes after the 0x240-byte LFU wrapper."
        )
    )
    parser.add_argument("input_rom", type=Path)
    parser.add_argument("output_payload", type=Path, nargs="?")
    parser.add_argument("--info-only", action="store_true")
    args = parser.parse_args()

    input_path = args.input_rom.resolve()
    if not input_path.is_file():
        print(f"input ROM not found: {input_path}", file=sys.stderr)
        return 1

    try:
        _, info = read_header(input_path)
    except ValueError as exc:
        print(f"{input_path}: {exc}", file=sys.stderr)
        return 1

    available_size = input_path.stat().st_size - HEADER_SIZE
    image_size = int(info["header_image_size"])
    output_path = args.output_payload.resolve() if args.output_payload else None
    if image_size <= 0:
        print("payload is empty", file=sys.stderr)
        return 1
    if image_size > available_size:
        print("header image size is larger than the file payload", file=sys.stderr)
        return 1
    trailing_size = available_size - image_size
    print_info(input_path, output_path, info, image_size, trailing_size)

    if args.info_only:
        return 0
    if output_path is None:
        print("output payload path is required unless --info-only is used", file=sys.stderr)
        return 2

    extract_payload(input_path, output_path, image_size)
    print(f"sha256:            {sha256_file(output_path)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
