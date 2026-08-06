"""Disassemble a region of an extracted DS binary to ARM assembly.

Quick scratch disassembler for eyeballing functions before we hand them to a
real analysis pass (Ghidra). Operates on the local extracted/ binaries.

Usage:
    # Disassemble 0x200 bytes of arm9.bin starting at file offset 0x1000
    python tools/disasm.py extracted/arm9.bin --offset 0x1000 --length 0x200

The DS ARM9 runs in ARM (32-bit) mode for most engine code; pass --thumb for
Thumb regions. Load address defaults to the ARM9 RAM base 0x02000000 so the
printed addresses line up with what a debugger shows.

Requires: pip install capstone
"""
import sys
import argparse
import pathlib

try:
    from capstone import Cs, CS_ARCH_ARM, CS_MODE_ARM, CS_MODE_THUMB
except ImportError:
    sys.exit("capstone not installed. Run: pip install capstone")


def auto_int(x: str) -> int:
    return int(x, 0)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("binary")
    ap.add_argument("--offset", type=auto_int, default=0, help="file offset to start")
    ap.add_argument("--length", type=auto_int, default=0x100, help="bytes to disassemble")
    ap.add_argument("--base", type=auto_int, default=0x02000000, help="RAM load address")
    ap.add_argument("--thumb", action="store_true", help="decode as Thumb instead of ARM")
    args = ap.parse_args()

    data = pathlib.Path(args.binary).read_bytes()
    chunk = data[args.offset:args.offset + args.length]

    md = Cs(CS_ARCH_ARM, CS_MODE_THUMB if args.thumb else CS_MODE_ARM)
    md.detail = False

    addr = args.base + args.offset
    for insn in md.disasm(chunk, addr):
        raw = insn.bytes.hex()
        print(f"{insn.address:08X}  {raw:<8}  {insn.mnemonic:<7} {insn.op_str}")


if __name__ == "__main__":
    main()
