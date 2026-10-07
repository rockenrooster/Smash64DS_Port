#!/usr/bin/env python3
"""P3 build identity (docs/P3_Multiplayer/Smash64DS_Multiplayer_Plan.md, 7.1).

Wireless play requires every console to run the same build: the same commit,
the same supported N64 ROM revision and the same toolchain. This hashes what
decides the simulation -- the loadable bytes of the ARM9 and ARM7 programs and
every NitroFS file -- and writes the first eight bytes of the SHA-256 to
net/build.id. The lobby carries that id in its room beacon and join request,
and a console refuses a room whose id differs from its own.

Only loadable segment bytes enter the hash, so debug sections and file
timestamps do not. An absolute path compiled into the program would, which is
one reason reproducible builds stay a release requirement.

Runs in the ROM recipe, after every NitroFS file is in place and before
ndstool packs them.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import struct
import sys

PT_LOAD = 1


def elf_load_segments(path: str) -> list[tuple[int, int, bytes]]:
    data = open(path, "rb").read()
    if data[:4] != b"\x7fELF" or data[4] != 1:
        raise SystemExit(f"{path}: not an ELF32 file")
    endian = "<" if data[5] == 1 else ">"
    (phoff,) = struct.unpack_from(endian + "I", data, 28)
    phentsize, phnum = struct.unpack_from(endian + "HH", data, 42)
    segments = []
    for i in range(phnum):
        (p_type, p_offset, _vaddr, p_paddr, p_filesz, p_memsz, _flags,
         _align) = struct.unpack_from(endian + "8I", data, phoff + i * phentsize)
        if p_type == PT_LOAD and p_filesz != 0:
            segments.append((p_paddr, p_memsz,
                             data[p_offset:p_offset + p_filesz]))
    return sorted(segments)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--arm9", required=True)
    parser.add_argument("--arm7", required=True)
    parser.add_argument("--nitrofs", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    digest = hashlib.sha256(b"smash64ds-build-id-1\0")
    for tag, path in ((b"arm9", args.arm9), (b"arm7", args.arm7)):
        for paddr, memsz, blob in elf_load_segments(path):
            digest.update(tag + struct.pack("<III", paddr, memsz, len(blob)))
            digest.update(blob)

    out_abs = os.path.normcase(os.path.abspath(args.out))
    files = []
    for root, _dirs, names in os.walk(args.nitrofs):
        for name in names:
            full = os.path.join(root, name)
            if os.path.normcase(os.path.abspath(full)) == out_abs:
                continue
            rel = os.path.relpath(full, args.nitrofs).replace(os.sep, "/")
            files.append((rel, full))
    files.sort()
    for rel, full in files:
        size = os.path.getsize(full)
        digest.update(rel.encode("utf-8") + b"\0" + struct.pack("<Q", size))
        with open(full, "rb") as f:
            while True:
                chunk = f.read(1 << 20)
                if not chunk:
                    break
                digest.update(chunk)

    ident = digest.digest()[:8]
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    if not (os.path.exists(args.out) and open(args.out, "rb").read() == ident):
        with open(args.out, "wb") as f:
            f.write(ident)
    print(f"build id {ident.hex()} ({len(files)} NitroFS files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
