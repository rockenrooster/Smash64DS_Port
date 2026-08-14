#!/usr/bin/env python3
"""Exact per-CALL-SITE counts for a named callee, off an existing profile.

`census-visit-counts.py` says a helper runs N times a frame. It does not say
WHO runs it, and for a leaf like `memset` or `__aeabi_fadd` that is the only
question worth asking -- see the campaign's standing note that self time is not
a subsystem budget.

This resolves it exactly rather than by estimate: every `bl <callee>` is an
instruction with its own address, and the profile carries an execution count per
address. Summing those counts per call site gives the exact number of calls each
site made, and grouping by enclosing function gives the caller breakdown. No
sampling, no call-graph guesswork, no build.

Usage:
  python scripts/census-call-sites.py <profile.csv> --dis <objdump -d> \
      --regions 1601 --callee memset --callee memcpy
"""

from __future__ import annotations

import argparse
import collections
import csv
import re

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
# `  1ff90b0:  ebfffffe   bl  201a2c4 <memset>`
CALL = re.compile(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2,8} )+\s*bl(?:x)?(?:\.\w)?\s+"
                  r"[0-9a-f]+\s+<([^>+]+)(?:\+[^>]*)?>")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--regions", type=int, required=True)
    ap.add_argument("--callee", action="append", required=True)
    ap.add_argument("--top", type=int, default=18)
    args = ap.parse_args()

    wanted = set(args.callee)
    # pc -> (caller, callee)
    sites: dict[int, tuple[str, str]] = {}
    cur = None
    with open(args.dis, errors="ignore") as handle:
        for raw in handle:
            m = FUNC.match(raw.rstrip("\n"))
            if m:
                cur = m.group(2)
                continue
            if cur is None:
                continue
            mc = CALL.match(raw)
            if mc and mc.group(2) in wanted:
                sites[int(mc.group(1), 16)] = (cur, mc.group(2))

    counts: collections.Counter = collections.Counter()
    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        next(rows)
        for row in rows:
            pc = int(row[1], 16)
            if pc in sites:
                counts[pc] += int(row[4])

    regions = args.regions
    for callee in args.callee:
        by_caller: collections.Counter = collections.Counter()
        n_sites: collections.Counter = collections.Counter()
        for pc, n in counts.items():
            caller, cal = sites[pc]
            if cal != callee:
                continue
            by_caller[caller] += n
            n_sites[caller] += 1
        total = sum(by_caller.values())
        if not total:
            print(f"== {callee}: no direct `bl` sites executed "
                  f"(tail-called, inlined, or reached via a veneer) ==\n")
            continue
        print(f"== {callee}: {total:,} direct calls = "
              f"{total / regions:,.1f}/frame, over {len(by_caller)} callers ==")
        print(f"{'calls/fr':>10}{'share':>8}{'sites':>7}  caller")
        for caller, n in by_caller.most_common(args.top):
            print(f"{n / regions:>10,.1f}{100 * n / total:>7.1f}%"
                  f"{n_sites[caller]:>7}  {caller[:66]}")
        print()


if __name__ == "__main__":
    main()
