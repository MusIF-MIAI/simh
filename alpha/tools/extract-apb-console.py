#!/usr/bin/env python3
"""Extract direct-APB console text from a SIMH Alpha instruction history."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


DEFAULT_PC = 0x20049E5C


def parse_int(text: str) -> int:
    text = text.strip()
    if text.lower().startswith("0x"):
        return int(text, 16)
    return int(text, 16)


def extract_chars(path: Path, pc: int, dedupe: bool) -> str:
    chars: list[str] = []
    last: str | None = None

    with path.open("r", encoding="utf-8", errors="replace") as src:
        for line in src:
            fields = line.split()
            if len(fields) < 2:
                continue
            try:
                hist_pc = parse_int(fields[0])
                value = parse_int(fields[1])
            except ValueError:
                continue
            if hist_pc != pc:
                continue
            char = chr(value & 0xFF)
            if dedupe and last == char:
                continue
            chars.append(char)
            last = char

    return "".join(chars)


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Extract characters printed by the OpenVMS Alpha APB diagnostic "
            "routine from `show cpu history` output."
        )
    )
    parser.add_argument("history", type=Path)
    parser.add_argument(
        "--pc",
        default=f"{DEFAULT_PC:x}",
        help=f"history PC that emits the character value, default {DEFAULT_PC:08X}",
    )
    parser.add_argument(
        "--no-dedupe",
        action="store_true",
        help="do not collapse repeated identical characters",
    )
    args = parser.parse_args()

    if not args.history.is_file():
        print(f"history file not found: {args.history}", file=sys.stderr)
        return 1

    try:
        pc = parse_int(args.pc)
    except ValueError:
        print(f"invalid PC: {args.pc}", file=sys.stderr)
        return 1

    output = extract_chars(args.history, pc, not args.no_dedupe)
    sys.stdout.write(output)
    if not output.endswith("\n"):
        sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
