#!/usr/bin/env python3
"""Rank functions by what they spend ENTERING and LEAVING, not by what they do.

The 2026-08-14 D-cache census closed the layout lane and left one direction
open: change how much is VISITED, not how it is arranged. The largest single
answer turned out not to belong to any subsystem. It is the call frame --
129,727 cyc/frame across 1,169 functions, 5.9% of the attributed frame, spent
saving and restoring registers.

The board had already found this for one function (`ndsR2AnimValueQ`, "6,683
cyc/frame, 16% of the evaluator, to save and restore registers") and treated it
as a property of that function. It is a property of the program, and this is the
instrument that says so.

Method: a function's prologue and epilogue are individually addressable
instructions, and the profile carries a cycle count per address. Summing the
`push`/`pop`/`stmdb`/`ldmia` forms that carry a register list, plus the `sp`
adjustments, gives each function's frame cost exactly -- no sampling, no
estimate, no build.

Read the `% of fn` column, not the absolute one. A function whose frame is a
third of its cost has a hot early-out guarding a cold body, and the fix is to
move the body into a `noinline` function of its own so the early-out stops
paying for registers only the body needs. GCC's `-fshrink-wrap` would normally
do this; it cannot on this target, because ARMv5TE Thumb-1 has no conditional
execution and the early exit cannot be predicated.

Usage:
  python scripts/census-call-frames.py <profile.csv> --dis <objdump -d> \
      --regions 1601
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
    ap.add_argument("--top", type=int, default=30)
    args = ap.parse_args()

    owner: dict[int, str] = {}
    entry: dict[int, str] = {}
    is_frame: set[int] = set()
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
            op, arg = mi.group(3), mi.group(4)
            # A register list is what separates a frame push from a stack store.
            if op.startswith(("push", "pop", "stmdb", "ldmia")) and "{" in arg:
                is_frame.add(pc)
            # `sub sp, #N` / `add sp, #N` -- the locals half of the same frame.
            elif op in ("sub", "add") and arg.startswith("sp,"):
                is_frame.add(pc)

    total: collections.Counter = collections.Counter()
    frame: collections.Counter = collections.Counter()
    calls: collections.Counter = collections.Counter()
    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        next(rows)
        for row in rows:
            try:
                pc = int(row[1], 16)
            except ValueError:
                continue
            fn = owner.get(pc)
            if fn is None:
                continue
            total[fn] += int(row[5])
            if pc in is_frame:
                frame[fn] += int(row[5])
            if pc in entry:
                calls[fn] += int(row[4])

    regions = args.regions
    grand = sum(frame.values())
    attributed = sum(total.values())
    print(f"TOTAL prologue/epilogue = {grand / regions:,.0f} cyc/frame "
          f"({grand / regions / 2:,.0f} ticks/frame) across {len(frame)} "
          f"functions")
    print(f"  = {100 * grand / attributed:.2f}% of {attributed / regions:,.0f} "
          f"attributed cyc/frame\n")
    print(f"{'frame/fr':>10}{'fn cyc/fr':>11}{'%fn':>7}{'calls/fr':>10}"
          f"{'cyc/call':>10}  function")
    print("-" * 100)
    for fn, cyc in frame.most_common(args.top):
        n = calls.get(fn, 0)
        print(f"{cyc / regions:>10,.0f}{total[fn] / regions:>11,.0f}"
              f"{100 * cyc / total[fn]:>7.1f}{n / regions:>10,.1f}"
              f"{(total[fn] / n if n else float('nan')):>10,.0f}  {fn[:50]}")


if __name__ == "__main__":
    main()
