#!/usr/bin/env python3
"""How much of every fetched I-cache line is actually executed?

Placement closed on measurement: `icache_fill` is 339,275 ticks/frame and it is
CAPACITY, so the lever is fewer bytes of hot code rather than better-arranged
bytes. This is the instrument for that lever.

The unit of fetch is a 32-byte line -- SIXTEEN Thumb instructions. The ARM9 pays
the whole line whether one instruction in it runs or all sixteen. So the question
that sizes footprint work is not "how big is this function" but:

    of the instructions in each line we PAY to fetch, how many ever run?

A line at 16/16 density is honest cost. A line at 2/16 is 87.5% of a fill spent
carrying instructions the match never executes -- and those bytes are evicting
lines that do run. Reported per function as:

  fetched lines      lines containing >=1 executed instruction (what we pay for)
  live insn          distinct instruction slots in those lines that executed
  density            live insn / (fetched lines * insns-per-line)
  dead-in-line B     bytes inside fetched lines that never executed
  fill cyc/frame     what the v3 attributor charged this function

`dead-in-line B` is the recoverable quantity: it is paid for on every fill and
buys nothing. Splitting cold code out of a hot function converts it to lines that
are simply never fetched.

Also reports GCC CLONE families -- `.constprop`, `.isra`, `.part`, `.cold` --
because a clone is a second copy of a body competing for the same cache, and
clone bloat is footprint that no source change asked for.

Requires a v3 profile for the `icache_fill` column; falls back to reporting
density alone if given v2.

UNITS: profile cycles. Two profile cycles = one project tick.
"""

from __future__ import annotations

import argparse
import collections
import csv
import re

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2,8} )+)\s*(\S+)\s*(.*)$")
CLONE = re.compile(r"\.(constprop|isra|part|cold)\.\d+")

LINE = 32
ITCM_LO, ITCM_HI = 0x01FF8000, 0x02000000


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--regions", type=int, required=True)
    ap.add_argument("--top", type=int, default=24)
    args = ap.parse_args()

    owner: dict[int, str] = {}
    width: dict[int, int] = {}
    fn_bytes: collections.Counter = collections.Counter()
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
            w = len(mi.group(2).replace(" ", "")) // 2
            owner[pc] = cur
            width[pc] = w
            fn_bytes[cur] += w

    executed: set[int] = set()
    fn_fill: collections.Counter = collections.Counter()
    fn_cyc: collections.Counter = collections.Counter()
    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        header = next(rows)
        col = {n: i for i, n in enumerate(header)}
        i_pc, i_cyc = col["pc"], col["total_cycles"]
        i_fill = col.get("icache_fill")
        for row in rows:
            try:
                pc = int(row[i_pc], 16)
            except ValueError:
                continue
            if ITCM_LO <= pc < ITCM_HI or pc not in owner:
                continue
            executed.add(pc)
            fn_cyc[owner[pc]] += int(row[i_cyc])
            if i_fill is not None:
                fn_fill[owner[pc]] += int(row[i_fill])

    # Which lines do we pay to fetch, and what lives in them?
    fetched_lines: dict[str, set] = collections.defaultdict(set)
    for pc in executed:
        fetched_lines[owner[pc]].add(pc & ~(LINE - 1))

    live_bytes: collections.Counter = collections.Counter()
    for pc in executed:
        live_bytes[owner[pc]] += width[pc]

    # bytes sitting inside a fetched line that never executed
    line_owner: dict[int, str] = {}
    for pc, fn in owner.items():
        line_owner.setdefault(pc & ~(LINE - 1), fn)
    dead_in_line: collections.Counter = collections.Counter()
    for fn, lines in fetched_lines.items():
        paid = len(lines) * LINE
        dead_in_line[fn] = paid - live_bytes[fn]

    R = args.regions
    total_paid = sum(len(v) for v in fetched_lines.values()) * LINE
    total_live = sum(live_bytes.values())
    total_dead = total_paid - total_live
    print(f"non-ITCM lines fetched at least once: "
          f"{sum(len(v) for v in fetched_lines.values()):,} "
          f"= {total_paid:,} B paid")
    print(f"  executed instruction bytes inside them: {total_live:,} "
          f"({100 * total_live / total_paid:.1f}%)")
    print(f"  DEAD-IN-LINE bytes, fetched but never run: {total_dead:,} "
          f"({100 * total_dead / total_paid:.1f}%)\n")

    key = fn_fill if fn_fill else fn_cyc
    label = "fill cyc/fr" if fn_fill else "cyc/fr"
    print(f"{'lines':>7}{'paid B':>9}{'live B':>8}{'dens%':>7}{'dead B':>8}"
          f"{label:>13}  function")
    print("-" * 104)
    for fn, _ in key.most_common(args.top):
        lines = len(fetched_lines.get(fn, ()))
        if not lines:
            continue
        paid = lines * LINE
        live = live_bytes[fn]
        print(f"{lines:>7}{paid:>9,}{live:>8,}"
              f"{100 * live / paid:>7.1f}{paid - live:>8,}"
              f"{key[fn] / R:>13,.0f}  {fn[:44]}")

    # --- clone families ----------------------------------------------------
    fams: dict[str, list] = collections.defaultdict(list)
    for fn in fn_bytes:
        base = CLONE.sub("", fn)
        if base != fn:
            fams[base].append(fn)
    if fams:
        rows_out = []
        for base, clones in fams.items():
            group = list(clones)
            if base in fn_bytes:
                group.append(base)
            if len(group) < 2:
                continue
            tot = sum(fn_bytes[f] for f in group)
            biggest = max(fn_bytes[f] for f in group)
            rows_out.append((tot - biggest, tot, len(group), base, group))
        rows_out.sort(reverse=True)
        dup = sum(r[0] for r in rows_out)
        print(f"\n== GCC clone families ({len(rows_out)} with >1 copy) ==")
        print(f"duplicate bytes beyond one copy each: {dup:,} B")
        print(f"{'dup B':>8}{'total B':>9}{'copies':>7}  family")
        for extra, tot, n, base, group in rows_out[:12]:
            live_any = any(f in fetched_lines for f in group)
            mark = " *executed" if live_any else ""
            print(f"{extra:>8,}{tot:>9,}{n:>7}  {base[:52]}{mark}")


if __name__ == "__main__":
    main()
