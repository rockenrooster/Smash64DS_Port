#!/usr/bin/env python3
"""Materialize BattleShip relocData from Splat's pinned relocData.bin.

The upstream decomp's ``make extract`` target rejects native Windows and its
VPK0 helper is published only for Linux and macOS.  This host-only adapter
reproduces the target's relocData.py ``extractAll`` output without editing the
read-only decomp source.

The VPK0 decoder is adapted from vpk0 0.8.2 (https://github.com/tehzz/vpk0):

MIT License

Copyright (c) 2021 tehzz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

from __future__ import annotations

import argparse
import re
import shutil
import struct
import sys
from dataclasses import dataclass
from pathlib import Path


class ExtractError(RuntimeError):
    """A deterministic extraction falsifier."""


class BitReader:
    def __init__(self, data: bytes) -> None:
        self.data = data
        self.bit_offset = 0

    def read(self, count: int) -> int:
        if count < 0 or self.bit_offset + count > len(self.data) * 8:
            raise ExtractError("truncated VPK0 bitstream")
        value = 0
        for _ in range(count):
            byte = self.data[self.bit_offset >> 3]
            bit = (byte >> (7 - (self.bit_offset & 7))) & 1
            self.bit_offset += 1
            value = (value << 1) | bit
        return value


@dataclass(frozen=True)
class TreeEntry:
    left: int = -1
    right: int = -1
    bits: int = -1

    @property
    def is_leaf(self) -> bool:
        return self.bits >= 0


class VpkTree:
    def __init__(self, entries: list[TreeEntry]) -> None:
        self.entries = entries

    @classmethod
    def read_from(cls, bits: BitReader) -> "VpkTree":
        entries: list[TreeEntry] = []
        pending: list[int] = []
        while True:
            if bits.read(1):
                if len(pending) < 2:
                    break
                right = pending.pop()
                left = pending.pop()
                entries.append(TreeEntry(left=left, right=right))
            else:
                entries.append(TreeEntry(bits=bits.read(8)))
            pending.append(len(entries) - 1)
        return cls(entries)

    def read_value(self, bits: BitReader) -> int:
        if not self.entries:
            return 0
        index = len(self.entries) - 1
        while not self.entries[index].is_leaf:
            entry = self.entries[index]
            index = entry.right if bits.read(1) else entry.left
        return bits.read(self.entries[index].bits)

    def format(self) -> str:
        if not self.entries:
            return "()"

        def visit(index: int) -> str:
            entry = self.entries[index]
            if entry.is_leaf:
                return str(entry.bits)
            return f"({visit(entry.left)}, {visit(entry.right)})"

        return visit(len(self.entries) - 1)


def decode_vpk0(data: bytes) -> tuple[bytes, str]:
    if len(data) < 9 or data[:4] != b"vpk0":
        raise ExtractError("invalid VPK0 header")
    output_size = int.from_bytes(data[4:8], "big")
    method = data[8]
    if method not in (0, 1):
        raise ExtractError(f"unsupported VPK0 method {method}")

    bits = BitReader(data)
    bits.read(9 * 8)
    offsets = VpkTree.read_from(bits)
    lengths = VpkTree.read_from(bits)
    output = bytearray()
    while len(output) < output_size:
        if bits.read(1):
            initial_move = offsets.read_value(bits)
            if method == 1:
                if initial_move < 3:
                    move_back = initial_move + 1 + (offsets.read_value(bits) << 2) - 8
                else:
                    move_back = (initial_move << 2) - 8
            else:
                move_back = initial_move
            if move_back <= 0 or move_back > len(output):
                raise ExtractError(
                    f"invalid VPK0 lookback {move_back} at output byte {len(output)}"
                )
            start = len(output) - move_back
            length = lengths.read_value(bits)
            for index in range(start, start + length):
                output.append(output[index])
        else:
            output.append(bits.read(8))
    if len(output) != output_size:
        raise ExtractError(
            f"VPK0 output overrun: expected {output_size}, got {len(output)}"
        )
    config = f"{method}\n{offsets.format()}\n{lengths.format()}"
    return bytes(output), config


def file_count(descriptions: Path) -> int:
    match = re.search(
        r"^# FILE_COUNT:\s*(\d+)\s*$",
        descriptions.read_text(encoding="utf-8"),
        flags=re.MULTILINE,
    )
    if match is None:
        raise ExtractError(f"FILE_COUNT is absent from {descriptions}")
    return int(match.group(1))


def extract(decomp_root: Path, version: str, output_dir: Path | None) -> int:
    decomp_root = decomp_root.resolve()
    assets_dir = decomp_root / "assets" / version
    source = assets_dir / "relocData.bin"
    descriptions = decomp_root / "tools" / f"relocFileDescriptions.{version}.txt"
    if not source.is_file() or not descriptions.is_file():
        raise ExtractError(
            "Splat inputs are absent; run the pinned split.py before relocData extraction"
        )

    count = file_count(descriptions)
    data = source.read_bytes()
    table_size = (count + 1) * 12
    if len(data) < table_size:
        raise ExtractError("relocData.bin is shorter than its relocation table")
    table = []
    for offset in range(0, table_size, 12):
        first, internal, compressed, external, decompressed = struct.unpack_from(
            ">IHHHH", data, offset
        )
        table.append(
            (bool(first & 0x80000000), first & 0x7FFFFFFF,
             internal, compressed, external, decompressed)
        )

    target = output_dir.resolve() if output_dir else assets_dir / "relocData"
    expected_parent = assets_dir.resolve()
    if output_dir is None and target.parent != expected_parent:
        raise ExtractError(f"refusing unsafe relocData target {target}")
    if target.exists():
        if not target.is_dir():
            raise ExtractError(f"relocData target is not a directory: {target}")
        if output_dir is not None:
            raise ExtractError(f"explicit output directory already exists: {target}")
        shutil.rmtree(target)
    target.mkdir(parents=True)

    csv_path = assets_dir / "relocData.csv"
    with csv_path.open("w", encoding="utf-8", newline="\n") as output:
        output.write(
            "isVpk0, dataOffset, relocInternOffset, compressedSize, "
            "relocExternOffset, decompressedSize\n"
        )
        for is_vpk0, data_offset, internal, compressed, external, decompressed in table:
            output.write(
                f"{1 if is_vpk0 else 0}, {data_offset:#08x}, {internal:#06x}, "
                f"{compressed:#06x}, {external:#06x}, {decompressed:#06x}\n"
            )

    payload_base = table_size
    compressed_count = 0
    for file_id, entry in enumerate(table[:-1]):
        is_vpk0, data_offset, *_ = entry
        next_offset = table[file_id + 1][1]
        if next_offset < data_offset:
            raise ExtractError(f"relocData offset order fails at file {file_id}")
        payload = data[payload_base + data_offset:payload_base + next_offset]
        if is_vpk0:
            compressed_count += 1
            encoded_path = target / f"{file_id}.vpk0"
            decoded_path = target / f"{file_id}.vpk0.bin"
            decoded, config = decode_vpk0(payload)
            encoded_path.write_bytes(payload)
            decoded_path.write_bytes(decoded)
            decoded_path.with_suffix(".vpk0_config").write_text(
                config, encoding="utf-8", newline=""
            )
        else:
            (target / f"{file_id}.bin").write_bytes(payload)

    actual_files = sum(1 for path in target.rglob("*") if path.is_file())
    expected_files = count + compressed_count * 2
    if actual_files != expected_files:
        raise ExtractError(
            f"relocData file count mismatch: expected {expected_files}, got {actual_files}"
        )
    print(
        "BATTLESHIP_RELOCDATA_OK "
        f"version={version} entries={count} compressed={compressed_count} "
        f"files={actual_files} output={target}"
    )
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--decomp-root", type=Path, required=True)
    parser.add_argument("--version", choices=("us",), default="us")
    parser.add_argument("--output-dir", type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        return extract(args.decomp_root, args.version, args.output_dir)
    except (ExtractError, OSError, struct.error) as exc:
        print(f"BATTLESHIP_RELOCDATA_FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
