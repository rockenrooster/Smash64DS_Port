#!/usr/bin/env python3
"""Minimal R4300 disassembler for porting Remix routines (P4 S5).

Reads code from the staged Remix build (ssb64asm.z64 through remix_rom's RAM
map) and prints one instruction per line with symbol names for jump
targets: Remix's from the bass log, the original game's from the decomp's
symbols_us.txt. Covers the integer, branch, load/store and COP1 forms the
game's code uses; anything else prints as `.word`.

    mipsdis.py --staging <remix staging> <symbol or 0xADDR> [--count N]

With a symbol and no --count, it stops at the next symbol.
"""
from __future__ import annotations

import argparse
import bisect
import struct
from pathlib import Path

import remix_rom as R

REG = ["zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
       "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"]
DECOMP_SYMBOLS = Path(__file__).resolve().parents[2] / "decomp" / "BattleShip-main" / "decomp" / "symbols" / "symbols_us.txt"

I_OPS = {0x08: "addi", 0x09: "addiu", 0x0A: "slti", 0x0B: "sltiu", 0x0C: "andi", 0x0D: "ori", 0x0E: "xori"}
MEM_OPS = {0x20: "lb", 0x21: "lh", 0x23: "lw", 0x24: "lbu", 0x25: "lhu", 0x28: "sb", 0x29: "sh", 0x2B: "sw",
           0x31: "lwc1", 0x35: "ldc1", 0x39: "swc1", 0x3D: "sdc1", 0x22: "lwl", 0x26: "lwr", 0x2A: "swl", 0x2E: "swr"}
BR_OPS = {0x04: "beq", 0x05: "bne", 0x06: "blez", 0x07: "bgtz", 0x14: "beql", 0x15: "bnel", 0x16: "blezl", 0x17: "bgtzl"}
FUNCT = {0x00: "sll", 0x02: "srl", 0x03: "sra", 0x04: "sllv", 0x06: "srlv", 0x07: "srav", 0x08: "jr", 0x09: "jalr",
         0x10: "mfhi", 0x11: "mthi", 0x12: "mflo", 0x13: "mtlo", 0x18: "mult", 0x19: "multu", 0x1A: "div",
         0x1B: "divu", 0x20: "add", 0x21: "addu", 0x22: "sub", 0x23: "subu", 0x24: "and", 0x25: "or",
         0x26: "xor", 0x27: "nor", 0x2A: "slt", 0x2B: "sltu", 0x0D: "break"}
FPU = {0x00: "add", 0x01: "sub", 0x02: "mul", 0x03: "div", 0x04: "sqrt", 0x05: "abs", 0x06: "mov", 0x07: "neg",
       0x0C: "round.w", 0x0D: "trunc.w", 0x0E: "ceil.w", 0x0F: "floor.w", 0x20: "cvt.s", 0x21: "cvt.d",
       0x24: "cvt.w", 0x30: "c.f", 0x32: "c.eq", 0x34: "c.olt", 0x36: "c.ole", 0x3C: "c.lt", 0x3E: "c.le",
       0x38: "c.sf", 0x3A: "c.seq", 0x31: "c.un", 0x33: "c.ueq", 0x35: "c.ult", 0x37: "c.ule", 0x39: "c.ngle",
       0x3B: "c.ngl", 0x3D: "c.nge", 0x3F: "c.ngt"}
FMT = {16: "s", 17: "d", 20: "w"}


def decode(word: int, pc: int, name) -> str:
    op = word >> 26
    rs, rt, rd = (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31
    sa, funct = (word >> 6) & 31, word & 63
    imm = word & 0xFFFF
    simm = imm - 0x10000 if imm & 0x8000 else imm
    if word == 0:
        return "nop"
    if op == 0:
        f = FUNCT.get(funct)
        if f in ("sll", "srl", "sra"):
            return f"{f} {REG[rd]}, {REG[rt]}, {sa}"
        if f in ("sllv", "srlv", "srav"):
            return f"{f} {REG[rd]}, {REG[rt]}, {REG[rs]}"
        if f == "jr":
            return f"jr {REG[rs]}"
        if f == "jalr":
            return f"jalr {REG[rd]}, {REG[rs]}"
        if f in ("mfhi", "mflo"):
            return f"{f} {REG[rd]}"
        if f in ("mult", "multu", "div", "divu"):
            return f"{f} {REG[rs]}, {REG[rt]}"
        if f:
            return f"{f} {REG[rd]}, {REG[rs]}, {REG[rt]}"
    if op == 1:
        kind = {0: "bltz", 1: "bgez", 2: "bltzl", 3: "bgezl", 16: "bltzal", 17: "bgezal"}.get(rt)
        if kind:
            return f"{kind} {REG[rs]}, {pc + 4 + simm * 4:#x}"
    if op in (2, 3):
        target = ((pc + 4) & 0xF0000000) | ((word & 0x3FFFFFF) << 2)
        return f"{'j' if op == 2 else 'jal'} {target:#x} <{name(target)}>"
    if op in BR_OPS:
        if op in (6, 7, 0x16, 0x17):
            return f"{BR_OPS[op]} {REG[rs]}, {pc + 4 + simm * 4:#x}"
        return f"{BR_OPS[op]} {REG[rs]}, {REG[rt]}, {pc + 4 + simm * 4:#x}"
    if op in I_OPS:
        value = imm if op in (0x0C, 0x0D, 0x0E) else simm
        return f"{I_OPS[op]} {REG[rt]}, {REG[rs]}, {value:#x}" if op in (0x0C, 0x0D, 0x0E) else \
            f"{I_OPS[op]} {REG[rt]}, {REG[rs]}, {value}"
    if op == 0x0F:
        return f"lui {REG[rt]}, {imm:#06x}"
    if op in MEM_OPS:
        reg = f"f{rt}" if op in (0x31, 0x35, 0x39, 0x3D) else REG[rt]
        return f"{MEM_OPS[op]} {reg}, {simm:#x}({REG[rs]})"
    if op == 0x11:
        fmt = rs
        if fmt == 0:
            return f"mfc1 {REG[rt]}, f{rd}"
        if fmt == 4:
            return f"mtc1 {REG[rt]}, f{rd}"
        if fmt == 2:
            return f"cfc1 {REG[rt]}, fcr{rd}"
        if fmt == 6:
            return f"ctc1 {REG[rt]}, fcr{rd}"
        if fmt == 8:
            kind = {0: "bc1f", 1: "bc1t", 2: "bc1fl", 3: "bc1tl"}.get(rt & 3)
            return f"{kind} {pc + 4 + simm * 4:#x}"
        if fmt in FMT and funct in FPU:
            fs, fd, ft = rd, sa, rt
            f = FPU[funct]
            if f.startswith("c."):
                return f"{f}.{FMT[fmt]} f{fs}, f{ft}"
            if f in ("sqrt", "abs", "mov", "neg", "round.w", "trunc.w", "ceil.w", "floor.w", "cvt.s", "cvt.d", "cvt.w"):
                return f"{f}.{FMT[fmt]} f{fd}, f{fs}"
            return f"{f}.{FMT[fmt]} f{fd}, f{fs}, f{ft}"
    return f".word {word:#010x}"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("target")
    ap.add_argument("--count", type=int, default=0)
    ap.add_argument("--to-return", action="store_true",
                    help="stop after the first `jr ra` and its delay slot, across inner labels")
    args = ap.parse_args()
    rom = R.Rom(args.staging / "ssb64asm.z64")
    sym, by_addr = R.load_symbols(args.staging / "logfile.log")
    decomp = R.load_decomp_symbols(DECOMP_SYMBOLS)
    names = {a: n for a, n in decomp.items()}
    for a, ns in by_addr.items():
        names.setdefault(a, ns[0])
    starts = sorted(names)

    def name(addr: int) -> str:
        i = bisect.bisect_right(starts, addr) - 1
        if i < 0:
            return "?"
        base = starts[i]
        return names[base] if base == addr else f"{names[base]}+{addr - base:#x}"

    addr = int(args.target, 16) if args.target.startswith("0x") else sym[args.target]
    count = args.count
    if args.to_return:
        count = count or 1024
    elif count == 0:
        later = [a for a in sorted(by_addr) if a > addr]
        count = ((later[0] - addr) // 4) if later else 64
    stop_after = None
    for i in range(count):
        pc = addr + 4 * i
        word = struct.unpack(">I", rom.read_ram(pc, 4))[0]
        label = by_addr.get(pc)
        if label and i:
            print(f"{label[0]}:")
        print(f"  {pc:#010x}: {word:08x}  {decode(word, pc, name)}")
        if args.to_return and word == 0x03E00008:
            stop_after = i + 1
        if stop_after is not None and i >= stop_after:
            break
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
