#!/usr/bin/env python3
"""Split "dead-in-line" bytes into LITERAL POOLS and genuinely cold code.

`census-fetch-density.py` reports that 26.1% of every fetched I-cache line is
bytes that never executed. Read naively that is 75,312 bytes of removable
footprint and, at 339,275 ticks/frame of icache_fill, roughly 88,000 ticks of
headroom. That reading is wrong, and this exists to say by how much.

Thumb-1 has no way to materialise an arbitrary 32-bit constant in an
instruction, so GCC emits `ldr rX, [pc, #N]` against a LITERAL POOL placed
inside .text -- usually just past the function's last instruction. Those pool
words are data. objdump disassembles them as instructions, they never appear in
a PC profile because they never execute, and `census-fetch-density.py` therefore
counts every one of them as dead. They are not removable: the code needs the
constants, and deleting the pool deletes the function.

This resolves every `[pc, #N]` load target in the disassembly and reclassifies
the bytes it lands on. What remains after that is cold CODE inside hot
functions -- the part a split could actually move out of the fetched lines.

Reports, per function and in total:

  pool B    bytes proven to be PC-relative load targets
  cold B    dead bytes that are not pool targets -- the real, removable share

UNITS: bytes.
"""

from __future__ import annotations

import argparse
import collections
import csv
import re

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2,8} )+)\s*(\S+)\s*(.*)$")
# objdump annotates the resolved target:  ldr r3, [pc, #204] @ (20542a0 <fn+0xd8>)
PCREL = re.compile(r"\[pc,\s*#-?\d+\].*?@\s*\(?([0-9a-f]+)")

LINE = 32
ITCM_LO, ITCM_HI = 0x01FF8000, 0x02000000


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--top", type=int, default=20)
    args = ap.parse_args()

    owner: dict[int, str] = {}
    width: dict[int, int] = {}
    pool_targets: set[int] = set()
    cur = None
    with open(args.dis, errors="ignore") as handle:
        for raw in handle:
            m = FUNC.match(raw.rstrip("\n"))
            if m:
                cur = m.group(2)
                continue
            if cur is None:
                continue
            mi = INSN.match(raw)
            if not mi:
                continue
            pc = int(mi.group(1), 16)
            owner[pc] = cur
            width[pc] = len(mi.group(2).replace(" ", "")) // 2
            mp = PCREL.search(raw)
            if mp:
                # a pool entry is one aligned word
                t = int(mp.group(1), 16) & ~3
                pool_targets.add(t)
                pool_targets.add(t + 2)   # cover half-word disassembly splits

    executed: set[int] = set()
    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        header = next(rows)
        i_pc = {n: i for i, n in enumerate(header)}["pc"]
        for row in rows:
            try:
                pc = int(row[i_pc], 16)
            except ValueError:
                continue
            if not (ITCM_LO <= pc < ITCM_HI):
                executed.add(pc)

    fetched: dict[str, set] = collections.defaultdict(set)
    for pc in executed:
        fn = owner.get(pc)
        if fn is not None:
            fetched[fn].add(pc & ~(LINE - 1))

    pool_b: collections.Counter = collections.Counter()
    cold_b: collections.Counter = collections.Counter()
    live_b: collections.Counter = collections.Counter()
    for pc, fn in owner.items():
        if fn not in fetched:
            continue
        if (pc & ~(LINE - 1)) not in fetched[fn]:
            continue                    # line never fetched: costs nothing
        w = width[pc]
        if pc in executed:
            live_b[fn] += w
        elif pc in pool_targets:
            pool_b[fn] += w
        else:
            cold_b[fn] += w

    tot_live = sum(live_b.values())
    tot_pool = sum(pool_b.values())
    tot_cold = sum(cold_b.values())
    paid = tot_live + tot_pool + tot_cold
    print(f"bytes inside fetched non-ITCM lines: {paid:,}")
    print(f"  live (executed)       {tot_live:>9,}  "
          f"{100 * tot_live / paid:>5.1f}%")
    print(f"  LITERAL POOL          {tot_pool:>9,}  "
          f"{100 * tot_pool / paid:>5.1f}%   not removable -- the code needs "
          f"the constants")
    print(f"  cold code             {tot_cold:>9,}  "
          f"{100 * tot_cold / paid:>5.1f}%   the only removable share\n")

    print(f"{'live B':>8}{'pool B':>8}{'cold B':>8}{'cold%':>7}  function")
    print("-" * 84)
    for fn, c in cold_b.most_common(args.top):
        tot = live_b[fn] + pool_b[fn] + c
        print(f"{live_b[fn]:>8,}{pool_b[fn]:>8,}{c:>8,}"
              f"{100 * c / tot:>7.1f}  {fn[:48]}")


if __name__ == "__main__":
    main()
