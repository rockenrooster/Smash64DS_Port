#!/usr/bin/env python3
"""Rank memory-access stalls, per instruction, off an existing profile.

Cycle 108 established that the ROM is memory-bound: non-idle **CPI 2.85**, so
**65% of all non-idle cycles are stall rather than issue**, and the soft-float
helpers -- the lane the campaign was about to convert -- are the *most efficient*
code in the build at CPI ~1.15. Instruction count is not the lever; the working
set is. This finds where the waiting actually happens.

The mechanism is the profile's own `average_cycles` column. On ARM9 a cached
`ldr` retires in ~1-3 cycles, so a load whose measured average is 10, 20 or 40
cycles is a cache miss being paid on most executions. Joining that to the
disassembly gives the *instruction*, which gives the base register and offset --
i.e. which field of which structure is missing. That is one step past "this
function is slow" and is what a layout change needs.

Costs no build and no emulator run: it reads a profile that already exists, the
same way `task37_census.py --split-by-symbol` and
`analyze-leaf-helper-attribution.py` do.

Two things it deliberately does NOT do:

  - It does not treat every expensive load as a D-cache miss. A PC-relative
    literal-pool load is an I-side access, and `pop`/`push` touch the stack,
    which is nearly always warm; both are reported in their own rows so they
    cannot be silently counted as data-structure misses. The one that matters
    for layout is a base-register load with an offset.
  - It does not guess a hit latency. `--baseline` is explicit (default 3, the
    conservative end for ARM9 `ldr`), and excess is reported as
    `cycles - executions * baseline` so the ranking is insensitive to the exact
    value while the absolute number stays honest about its assumption.

Usage:
  python scripts/analyze-dcache-stalls.py \
      artifacts/performance/<run>/arm9-profile.csv \
      --dis <objdump -d of the matching ELF> \
      --census artifacts/performance/<run>/census.json \
      --top-sites 25 --top-functions 20
"""

from __future__ import annotations

import argparse
import collections
import csv
import json
import re
import sys

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2,8} )+\s*(\S+)\s*(.*)$")

LOAD_OPS = ("ldr", "ldm", "pop", "ldrd")
STORE_OPS = ("str", "stm", "push", "strd")


def classify(mnemonic: str, operands: str):
    """-> ('load'|'store'|'other', is_literal_pool, touches_stack)"""
    m = mnemonic.lower()
    literal = "[pc" in operands or "@ (" in operands
    stack = m.startswith(("pop", "push")) or "[sp" in operands
    base = m.split(".")[0]
    if base.startswith(LOAD_OPS):
        return "load", literal, stack
    if base.startswith(STORE_OPS):
        return "store", literal, stack
    return "other", literal, stack


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--census", default="")
    ap.add_argument("--baseline", type=float, default=3.0,
                    help="assumed cycles for a CACHED access (default 3)")
    ap.add_argument("--min-execs", type=int, default=2000,
                    help="ignore sites executed fewer times than this")
    ap.add_argument("--top-sites", type=int, default=25)
    ap.add_argument("--top-functions", type=int, default=20)
    args = ap.parse_args(argv)

    site = {}
    cur = None
    with open(args.dis, errors="ignore") as handle:
        for line in handle:
            m = FUNC.match(line.rstrip("\n"))
            if m:
                cur = m.group(2)
                continue
            if cur is None:
                continue
            mi = INSN.match(line)
            if not mi:
                continue
            kind, literal, stack = classify(mi.group(2), mi.group(3))
            site[int(mi.group(1), 16)] = (
                cur, kind, literal, stack,
                (mi.group(2) + " " + mi.group(3)).strip())

    execs = collections.Counter()
    cycles = collections.Counter()
    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        next(rows)
        for row in rows:
            pc = int(row[1], 16)
            if pc in site:
                execs[pc] += int(row[4])
                cycles[pc] += int(row[5])

    totals = collections.Counter()
    for pc, n in execs.items():
        _, kind, literal, stack = site[pc][:4]
        bucket = kind
        if kind == "load" and literal:
            bucket = "load (literal pool)"
        elif kind == "load" and stack:
            bucket = "load (stack)"
        elif kind == "load":
            bucket = "load (data)"
        totals[bucket] += cycles[pc]
        totals[bucket + " #"] += n

    grand = sum(v for k, v in totals.items() if not k.endswith("#"))
    print("accesses attributed: {:,} cycles across {:,} sites\n".format(
        grand, len(execs)))
    print("{:24s} {:>14s} {:>13s} {:>8s}".format(
        "class", "cycles", "executions", "cyc/ex"))
    for key in ("load (data)", "load (literal pool)", "load (stack)",
                "store", "other"):
        c, n = totals.get(key, 0), totals.get(key + " #", 0)
        if not n:
            continue
        print("{:24s} {:>14,} {:>13,} {:>8.2f}".format(key, c, n, c / n))

    # Excess over a cached access, data loads only -- the layout-relevant set.
    by_func = collections.Counter()
    sites = []
    for pc, n in execs.items():
        name, kind, literal, stack, text = site[pc]
        if kind != "load" or literal or stack:
            continue
        excess = cycles[pc] - n * args.baseline
        if excess <= 0:
            continue
        by_func[name] += excess
        if n >= args.min_execs:
            sites.append((excess, cycles[pc] / n, n, name, text, pc))

    print("\nEXCESS over a {:.0f}-cycle cached access, DATA loads only".format(
        args.baseline))
    print("total excess: {:,} cycles\n".format(int(sum(by_func.values()))))

    print("{:46s} {:>14s}".format("function", "excess cycles"))
    for name, exc in by_func.most_common(args.top_functions):
        print("{:46s} {:>14,}".format(name[:46], int(exc)))

    sites.sort(reverse=True)
    print("\n{:>8s} {:>10s} {:>12s}  {:34s}  {}".format(
        "cyc/ex", "execs", "excess", "function", "instruction"))
    for excess, per, n, name, text, pc in sites[:args.top_sites]:
        print("{:>8.1f} {:>10,} {:>12,}  {:34s}  {}".format(
            per, n, int(excess), name[:34], text[:44]))

    if args.census:
        cen = json.load(open(args.census))
        cyc = {s["name"]: s["cycles"] for s in cen["symbols"]}
        work = cen["total_cycles"] - cyc.get("armWaitForIrq", 0)
        print("\ndata-load excess is {:.2f}% of non-idle work ({:,})".format(
            100.0 * sum(by_func.values()) / work, work))
    return 0


if __name__ == "__main__":
    sys.exit(main())
