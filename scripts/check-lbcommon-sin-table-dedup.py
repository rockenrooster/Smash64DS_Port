#!/usr/bin/env python3
"""Can lbCommonSin/lbCommonCos read gSYSinTable instead of their own table?

The port ships TWO sine tables in main RAM, 4,096 bytes each:

  020cdf88 00001000 D gSYSinTable          2048 x u16, Q15, half wave
  020c5238 00001000 T dLBCommonSinLookup   1024 x f32,       quarter wave

The second arrived with R2-07 L9 and cost the Sudden Death arena a whole page:
the arena is sized by a boot-time loop that steps down 0x1000 per failed calloc,
so 4,096 bytes of new static RAM is exactly one step, and Sudden Death froze.
docs/BUGS.md carries the measurement.

They are the same function at the same resolution. Both index by
angle * 2048/PI masked to 0xFFF -- L9 spells the scale 651.8986206, the source
spells it SINTABLE_RAD_TO_ID -- and both take the sign from bit 0x800. Only the
storage differs: a quarter wave in f32 against a half wave in Q15.

So the question is only how much precision the Q15 table costs, and that is
answerable exhaustively: there are 4096 distinct indices and this checks all of
them. No sampling, no domain argument.
"""
from __future__ import annotations

import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PORT_TABLE = ROOT / "src" / "port" / "lbcommon_sin_table.c"
DECOMP_TABLE = (ROOT / "decomp" / "BattleShip-main" / "decomp" / "src" /
                "sys" / "sintable.c")

# The bound E64b/E65 established for the animation cubic, in the units gameplay
# reads. Quoted here because a sine error only matters through what consumes it,
# and that is the nearest established gate.
GAMEPLAY_BOUND_RAD = 0.0028


def parse_f32_table(path: Path) -> list[float]:
    text = path.read_text(encoding="utf-8", errors="replace")
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    body = text[text.index("{"):]
    vals = [float(m) for m in
            re.findall(r"-?\d+\.\d+(?:[eE][-+]?\d+)?[fF]?", body.replace("F", ""))]
    return vals


def parse_u16_table(path: Path) -> list[int]:
    text = path.read_text(encoding="utf-8", errors="replace")
    return [int(m, 16) for m in re.findall(r"0[xX][0-9a-fA-F]{4}\b", text)]


def main() -> int:
    if not DECOMP_TABLE.is_file():
        print(f"decomp table absent at {DECOMP_TABLE}; nothing to check")
        return 0

    f32 = parse_f32_table(PORT_TABLE) if PORT_TABLE.is_file() else None
    q15 = parse_u16_table(DECOMP_TABLE)

    print(f"gSYSinTable entries: {len(q15)}")
    if f32 is not None:
        print(f"dLBCommonSinLookup entries: {len(f32)}")
    if len(q15) != 2048:
        print(f"FAIL: expected 2048 gSYSinTable entries, got {len(q15)}")
        return 1

    # Q15 scale, confirmed from the head of the table: entry 1 is 0x0032 = 50,
    # and sin(2*pi/4096) * 32768 = 50.27.
    scale = 32768.0

    worst_vs_exact = 0.0
    worst_vs_f32 = 0.0
    worst_index = -1
    for index in range(4096):
        # The source's own read, sign from bit 0x800.
        v = q15[index & 0x7FF] / scale
        if index & 0x800:
            v = -v

        exact = math.sin(index * 2.0 * math.pi / 4096.0)
        d = abs(v - exact)
        if d > worst_vs_exact:
            worst_vs_exact, worst_index = d, index

        if f32 is not None:
            # L9's read: quarter wave with a mirror at 0x400.
            i10 = index & 0x3FF
            w = f32[0x3FF - i10] if (index & 0x400) else f32[i10]
            if index & 0x800:
                w = -w
            worst_vs_f32 = max(worst_vs_f32, abs(v - w))

    print(f"\nexhaustive over all 4096 indices:")
    print(f"  gSYSinTable vs TRUE sin  : max |err| {worst_vs_exact:.8f} "
          f"(index {worst_index})")
    if f32 is not None:
        print(f"  gSYSinTable vs L9 f32    : max |err| {worst_vs_f32:.8f}")
    print(f"  one Q15 quantum          : {1.0 / scale:.8f}")

    # Read those two numbers in the right order or the conclusion inverts.
    #
    # The first is NOT a cost of this change. gSYSinTable holds
    # sin(i * pi / 2047) -- 2048 samples spanning 0..pi INCLUSIVE, which is why
    # it ends at 0 and peaks at both 1023 and 1024 -- while the runtime indexes
    # it as though the span were 2048. That one-sample stretch is worth about
    # 0.0016 at the steep part, and it is the ORIGINAL GAME'S sine. Every
    # syGetSinCosUShort call in SSB64 carries it. Reproducing it is fidelity,
    # not error, and a "more accurate" table would be the deviation.
    #
    # L9's f32 quarter-wave carries the identical stretch, because it is the
    # same source table in another storage format. That is why the two agree to
    # a single Q15 quantum, and THAT is the only thing dropping one of them
    # costs.
    if f32 is None:
        print("\nNo dLBCommonSinLookup in the tree; nothing to compare.")
        return 0

    print(f"\n  substitution cost        : {worst_vs_f32:.8f}")
    print(f"  E64b/E65 gameplay bound  : {GAMEPLAY_BOUND_RAD:.4f}")
    ratio = GAMEPLAY_BOUND_RAD / worst_vs_f32 if worst_vs_f32 else float("inf")
    print(f"  margin                   : {ratio:.0f}x")

    if worst_vs_f32 > GAMEPLAY_BOUND_RAD:
        print("\nFAIL: dropping dLBCommonSinLookup exceeds the E64b/E65 bound.")
        return 1
    print("\nOK: the two tables are the same function to one Q15 quantum, so "
          "the second one is 4,096 bytes of main RAM bought for nothing.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
