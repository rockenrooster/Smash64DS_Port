#!/usr/bin/env python3
"""The ROM's NitroFS directories numbered as a sorted listing numbers them,
whatever the file system listed (owner 2026-10-29: one tree, one ROM).

ndstool numbers NitroFS directories in the order readdir() lists them: each
directory takes the next id just before ndstool descends into it (pre-order),
siblings in listing order. Its file ids, the FAT and the order of each
directory's name-table entries do not follow the listing, so a listing that
is not sorted changes the directory ids alone: each directory's main-table
slot, the parent field of its children's entries and the id its parent's
subtable gives it. NTFS lists sorted. This PC's D: is a StableBit DrivePool
pool, which lists each underlying disk's part of a folder in turn, and
FAT32/exFAT list in creation order, so the same tree built there gave a
different ROM (docs/RELEASE/RELEASE_PLAN.md R5).

This renumbers the directories as an NTFS listing would (siblings compared
upper-cased, NTFS's collation for these ASCII names), moves each main-table
entry to its new slot and rewrites the ids in place. The name table keeps
its size and its subtables their places; the file ids, the FAT and every
other byte stay. The ROM is rewritten in place only when an id moves, and
every path is checked to name the same file id afterwards. Runs in the ROM
recipe right after ndstool (mk/rules_arm7.mk).

    nds_nitrofs_order.py ROM            renumber in place
    nds_nitrofs_order.py ROM --check    exit 1 when the ROM is not in order
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

ROOT_ID = 0xF000


class OrderError(SystemExit):
    pass


def parse(rom: bytes) -> tuple[int, list[dict]]:
    """The name table's offset and its directories: each one's main-table
    fields and its subtable's entries (name, child index or None for a file,
    and where a child directory's id lies)."""
    fnt, size = struct.unpack_from("<II", rom, 0x40)
    count = struct.unpack_from("<H", rom, fnt + 6)[0]
    if not 0 < count <= 0x1000 or 8 * count > size:
        raise OrderError(f"name table at {fnt:#x}: {count} directories in {size} bytes")
    dirs = []
    for index in range(count):
        sub, first, parent = struct.unpack_from("<IHH", rom, fnt + 8 * index)
        entries, at = [], fnt + sub
        while True:
            kind = rom[at]
            at += 1
            if kind == 0:
                break
            name = bytes(rom[at:at + (kind & 0x7F)])
            at += kind & 0x7F
            if kind & 0x80:
                child = struct.unpack_from("<H", rom, at)[0] - ROOT_ID
                if not 0 < child < count:
                    raise OrderError(f"directory {name!r}: id {child + ROOT_ID:#x} outside the table")
                entries.append((name, child, at))
                at += 2
            else:
                entries.append((name, None, None))
            if at > fnt + size:
                raise OrderError("a subtable runs past the name table")
        dirs.append({"first": first, "parent": parent, "entries": entries})
    return fnt, dirs


def paths(dirs: list[dict]) -> dict[bytes, int]:
    """Every file's path and file id (the name table read as the runtime
    reads it)."""
    out = {}

    def walk(index: int, prefix: bytes) -> None:
        file_id = dirs[index]["first"]
        for name, child, _ in dirs[index]["entries"]:
            if child is None:
                out[prefix + name] = file_id
                file_id += 1
            else:
                walk(child, prefix + name + b"/")

    walk(0, b"")
    return out


def sorted_ids(dirs: list[dict]) -> list[int]:
    """Each directory's index under a sorted listing: pre-order, a
    directory numbered just before its own subdirectories."""
    new = [0] * len(dirs)
    seen = {0}
    counter = 1

    def walk(index: int) -> None:
        nonlocal counter
        children = [(name, child) for name, child, _ in dirs[index]["entries"] if child is not None]
        for name, child in sorted(children, key=lambda c: c[0].upper()):
            if child in seen:
                raise OrderError(f"directory {name!r} is listed twice")
            seen.add(child)
            new[child] = counter
            counter += 1
            walk(child)

    walk(0)
    if counter != len(dirs):
        raise OrderError(f"{len(dirs) - counter} directories hang from no parent")
    return new


def renumber(rom: bytearray) -> int:
    """Renumber in place; the count of directories whose id moved."""
    fnt, dirs = parse(rom)
    new = sorted_ids(dirs)
    moved = sum(1 for i, n in enumerate(new) if i != n)
    if not moved:
        return 0
    before = paths(dirs)
    main = bytearray(8 * len(dirs))
    for index, d in enumerate(dirs):
        sub = struct.unpack_from("<I", rom, fnt + 8 * index)[0]
        parent = d["parent"] if index == 0 else ROOT_ID + new[d["parent"] - ROOT_ID]
        struct.pack_into("<IHH", main, 8 * new[index], sub, d["first"], parent)
        for _, child, at in d["entries"]:
            if child is not None:
                struct.pack_into("<H", rom, at, ROOT_ID + new[child])
    rom[fnt:fnt + len(main)] = main
    if paths(parse(rom)[1]) != before:
        raise OrderError("renumbering changed a path's file id")
    return moved


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("rom", type=Path)
    ap.add_argument("--check", action="store_true", help="exit 1 when the directories are not in order")
    args = ap.parse_args()
    rom = bytearray(args.rom.read_bytes())
    count = len(parse(rom)[1])
    moved = renumber(rom)
    if args.check:
        print(f"NITROFS_ORDER: {count} directories, {moved} out of sorted order")
        return 1 if moved else 0
    if moved:
        args.rom.write_bytes(rom)
    print(f"NITROFS_ORDER: {count} directories, {moved} renumbered")
    return 0


if __name__ == "__main__":
    sys.exit(main())
