#!/usr/bin/env python3
"""Does the hot instruction working set fit in the ARM946E-S I-cache?

Placement optimization only pays if the hot code CONFLICTS. That is one
measurement, and it gates the whole lane: if the executed footprint fits inside
the cache with room to spare, reordering cannot help because nothing is evicting
anything. If it overflows by a wide margin, capacity misses dominate and
reordering redistributes conflict rather than removing it. Placement pays in the
band between -- where the set-conflict pattern, not the total size, is what
evicts.

ARM946E-S I-cache, as configured on the Nintendo DS ARM9:

    size          8192 bytes
    line          32 bytes           (libnds `cache.h` documents the 32-byte
                                      line: DC_InvalidateRange warns base and
                                      size must be "cache line size (32-byte)
                                      aligned")
    associativity 4-way
    sets          8192 / 32 / 4 = 64
    index         set = (addr >> 5) & 63
    set period    64 * 32 = 2048 bytes

So two addresses collide in the cache when they are congruent modulo 2048, and
any five hot lines sharing a set evict one another regardless of how much total
cache is free. That modulus is what a link-order change actually moves.

Reports the executed footprint, its cache multiple, and the per-set pressure
histogram weighted by execution count -- the distribution that says whether a
reordering has anything to fix.

UNITS: profile cycles. Two profile cycles = one project tick.

Usage:
  python scripts/census-icache-placement.py <profile.csv> --dis <objdump -d> \
      --elf <linked.elf> --regions 1601
"""

from __future__ import annotations

import argparse
import collections
import csv
import re
import subprocess

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2,8} )+)\s*(\S+)\s*(.*)$")

ICACHE_BYTES = 8192
LINE_BYTES = 32
WAYS = 4
SETS = ICACHE_BYTES // LINE_BYTES // WAYS          # 64
SET_PERIOD = SETS * LINE_BYTES                     # 2048

# ITCM is zero-wait and never occupies an I-cache line; excluding it is the
# difference between measuring the cache and measuring the binary.
ITCM_LO, ITCM_HI = 0x01FF8000, 0x02000000


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--elf")
    ap.add_argument("--regions", type=int, required=True)
    ap.add_argument("--top", type=int, default=20)
    args = ap.parse_args()

    owner: dict[int, str] = {}
    fn_start: dict[str, int] = {}
    fn_end: dict[str, int] = {}
    cur = None
    with open(args.dis, errors="ignore") as handle:
        for raw in handle:
            m = FUNC.match(raw.rstrip("\n"))
            if m:
                cur = m.group(2)
                fn_start.setdefault(cur, int(m.group(1), 16))
                continue
            if cur is None:
                continue
            mi = INSN.match(raw)
            if not mi:
                continue
            pc = int(mi.group(1), 16)
            owner[pc] = cur
            width = len(mi.group(2).replace(" ", "")) // 2
            fn_end[cur] = max(fn_end.get(cur, 0), pc + width)

    cycles: collections.Counter = collections.Counter()
    execs: collections.Counter = collections.Counter()
    line_exec: collections.Counter = collections.Counter()
    line_cyc: collections.Counter = collections.Counter()
    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        next(rows)
        for row in rows:
            try:
                pc = int(row[1], 16)
            except ValueError:
                continue
            fn = owner.get(pc)
            if fn is None or ITCM_LO <= pc < ITCM_HI:
                continue
            n, c = int(row[4]), int(row[5])
            cycles[fn] += c
            execs[fn] += n
            line_exec[pc & ~(LINE_BYTES - 1)] += n
            line_cyc[pc & ~(LINE_BYTES - 1)] += c

    regions = args.regions
    executed = [f for f in cycles if fn_start.get(f) is not None]
    footprint = sum(fn_end[f] - fn_start[f] for f in executed if f in fn_end)
    touched_lines = len(line_exec)
    touched_bytes = touched_lines * LINE_BYTES

    print(f"ARM946E-S I-cache: {ICACHE_BYTES} B, {LINE_BYTES} B lines, "
          f"{WAYS}-way, {SETS} sets, set period {SET_PERIOD} B")
    print(f"  set index = (addr >> 5) & {SETS - 1};  two addresses collide when "
          f"congruent mod {SET_PERIOD}\n")
    print(f"executed non-ITCM functions        {len(executed):>12,}")
    print(f"  their total text footprint       {footprint:>12,} B "
          f"= {footprint / ICACHE_BYTES:,.1f}x the I-cache")
    print(f"distinct 32B lines actually fetched{touched_lines:>12,} "
          f"= {touched_bytes:,} B = {touched_bytes / ICACHE_BYTES:,.1f}x cache")

    # Weighted set pressure: how many DISTINCT hot lines land in each set.
    hot_cut = sorted(line_exec.values(), reverse=True)
    top_n = min(len(hot_cut), 2000)
    threshold = hot_cut[top_n - 1] if hot_cut else 0
    hot_lines = [a for a, n in line_exec.items() if n >= threshold]
    per_set: collections.Counter = collections.Counter()
    for addr in hot_lines:
        per_set[(addr >> 5) & (SETS - 1)] += 1
    if per_set:
        occ = sorted(per_set.values(), reverse=True)
        over = sum(1 for v in per_set.values() if v > WAYS)
        print(f"\nhottest {len(hot_lines):,} lines spread over "
              f"{len(per_set)}/{SETS} sets")
        print(f"  distinct hot lines per set: max {occ[0]}, median "
              f"{occ[len(occ) // 2]}, min {occ[-1]}")
        print(f"  sets holding more than {WAYS} hot lines (i.e. guaranteed "
              f"conflict): {over}/{SETS}")
        print(f"  perfectly even spread would be "
              f"{len(hot_lines) / SETS:,.1f} lines/set")

    print(f"\ntop functions by cycles (non-ITCM), "
          f"span and sets occupied:")
    print(f"{'cyc/fr':>9}{'bytes':>8}{'lines':>7}{'sets':>6}  function")
    for fn, c in sorted(cycles.items(), key=lambda kv: -kv[1])[:args.top]:
        if fn not in fn_end:
            continue
        size = fn_end[fn] - fn_start[fn]
        lines = (size + LINE_BYTES - 1) // LINE_BYTES
        sets = min(lines, SETS)
        print(f"{c / regions:>9,.0f}{size:>8,}{lines:>7}{sets:>6}  {fn[:52]}")


if __name__ == "__main__":
    main()
