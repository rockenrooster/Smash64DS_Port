#!/usr/bin/env python3
"""Rank functions by CALL COUNT and cost-per-call, not by self time.

The 2026-08-14 D-cache census closed the layout lane: the ARM9 is memory-bound,
but the cost is spread uniformly over BattleShip's pointer-linked data model and
no single structure repack reaches the bar. What is left is changing how much
data gets VISITED -- node counts, call counts, visit rates.

This is the instrument for that. A function's ENTRY PC is executed exactly once
per call, so the profile's execution count at that address is an exact call
count -- no sampling, no estimate. Joining it to the function's total cycles
gives cycles/call, and dividing by regions gives calls/frame.

The question this is built to answer is not "what is slow" but "what runs 300
times a frame, and does it have to?"

Reports, per function: calls/frame, cycles/frame, cycles/call, and the share of
the frame. Sorted by cycles/frame, because a cheap function called constantly
and an expensive one called rarely are different problems and only the product
says which you have.
"""

from __future__ import annotations

import argparse
import collections
import csv
import re

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2,8} )+)\s*(\S+)\s*(.*)$")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--regions", type=int, required=True)
    ap.add_argument("--top", type=int, default=45)
    ap.add_argument("--min-calls-per-frame", type=float, default=0.0)
    args = ap.parse_args()

    entry: dict[int, str] = {}
    owner: dict[int, str] = {}
    cur = None
    first = True
    with open(args.dis, errors="ignore") as handle:
        for raw in handle:
            m = FUNC.match(raw.rstrip("\n"))
            if m:
                cur = m.group(2)
                first = True
                continue
            if cur is None:
                continue
            mi = INSN.match(raw)
            if not mi:
                continue
            pc = int(mi.group(1), 16)
            owner[pc] = cur
            if first:
                entry[pc] = cur
                first = False

    calls: collections.Counter = collections.Counter()
    cycles: collections.Counter = collections.Counter()
    insns: collections.Counter = collections.Counter()
    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        next(rows)
        for row in rows:
            pc = int(row[1], 16)
            fn = owner.get(pc)
            if fn is None:
                continue
            n = int(row[4])
            cycles[fn] += int(row[5])
            insns[fn] += n
            if pc in entry:
                calls[fn] += n

    regions = args.regions
    total = sum(cycles.values())
    ranked = sorted(cycles.items(), key=lambda kv: -kv[1])

    print(f"regions (frames): {regions:,}   attributed cycles: {total:,} "
          f"= {total / regions:,.0f}/frame\n")
    print(f"{'cyc/frame':>10}{'calls/fr':>10}{'cyc/call':>10}{'%tot':>7}"
          f"{'CPI':>6}  function")
    print("-" * 104)
    shown = 0
    for fn, cyc in ranked:
        c = calls.get(fn, 0)
        cpf = c / regions
        if cpf < args.min_calls_per_frame:
            continue
        cpc = (cyc / c) if c else float("nan")
        cpi = cyc / insns[fn] if insns[fn] else 0.0
        print(f"{cyc / regions:>10,.0f}{cpf:>10,.1f}"
              f"{cpc:>10,.0f}{100 * cyc / total:>7.2f}{cpi:>6.2f}  {fn[:60]}")
        shown += 1
        if shown >= args.top:
            break

    print("\n== ranked by CALLS/FRAME (what runs most often) ==")
    print(f"{'calls/fr':>10}{'cyc/frame':>11}{'cyc/call':>10}  function")
    print("-" * 104)
    by_calls = sorted(calls.items(), key=lambda kv: -kv[1])
    for fn, c in by_calls[:args.top]:
        print(f"{c / regions:>10,.1f}{cycles[fn] / regions:>11,.0f}"
              f"{cycles[fn] / c:>10,.0f}  {fn[:60]}")


if __name__ == "__main__":
    main()
