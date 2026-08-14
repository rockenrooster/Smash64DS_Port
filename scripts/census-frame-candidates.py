#!/usr/bin/env python3
"""What a call-frame transformation would ACTUALLY recover, per function.

`census-call-frames.py` reports the ceiling: how much a function spends in
prologue/epilogue. That number is not what a cold-tail split recovers, and the
first call-frame slice was sized against it and overstated `ftGetStruct` by 2x.

Fitting every frame instruction in the build with >=2,000 executions against its
register count gives the real cost:

    push  of N registers   ~ 1.6 + 1.2*N cycles
    pop+pc of N registers  ~ 5.0 + 1.6*N cycles

So a function that still returns through `pop {..., pc}` keeps ~9.3 cycles a call
however few registers it saves -- the pc load's pipeline flush is the floor. A
split recovers only (N_old - N_new) * ~2.8 cycles a call. The whole frame is only
recoverable if the function becomes genuinely frameless, or if the call goes away
entirely (inlined or deleted), and a deleted call takes its body with it.

This ranks candidates under all three transformations so the difference is
visible before any code is written:

    SPLIT   cold tail moved out; frame shrinks to the hot route's own need
    LEAF    function becomes frameless (no push, `bx lr`)
    DELETE  the call disappears; frame AND body recovered

`hot%` is the share of the function's entry executions that reach its cheapest
return -- a high number means the body really is cold and a split is available.
It is measured from the profile's execution count at each `pop`, not guessed.

UNITS: profile cycles. Two profile cycles = one project tick.

Usage:
  python scripts/census-frame-candidates.py <profile.csv> --dis <objdump -d> \
      --regions 1601 --top 50
"""

from __future__ import annotations

import argparse
import collections
import csv
import re

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2,8} )+)\s*(\S+)\s*(.*)$")

# Fitted from every frame instruction in the build with >=2,000 executions.
PUSH_BASE, PUSH_PER = 1.6, 1.2
POPPC_BASE, POPPC_PER = 5.0, 1.6


def push_cost(n: int) -> float:
    return PUSH_BASE + PUSH_PER * n


def poppc_cost(n: int) -> float:
    return POPPC_BASE + POPPC_PER * n


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--regions", type=int, required=True)
    ap.add_argument("--top", type=int, default=50)
    args = ap.parse_args()

    owner: dict[int, str] = {}
    entry: dict[int, str] = {}
    # pc -> ('push'|'pop', nregs, returns_via_pc)
    frame_insn: dict[int, tuple[str, int, bool]] = {}
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
            if op.startswith(("push", "pop", "stmdb", "ldmia")) and "{" in arg:
                regs = [r.strip() for r
                        in arg[arg.index("{") + 1:arg.rindex("}")].split(",")
                        if r.strip()]
                kind = "push" if op.startswith(("push", "stmdb")) else "pop"
                frame_insn[pc] = (kind, len(regs), "pc" in regs)
            elif op in ("sub", "add") and arg.startswith("sp,"):
                frame_insn[pc] = ("sp", 0, False)

    total: collections.Counter = collections.Counter()
    frame: collections.Counter = collections.Counter()
    calls: collections.Counter = collections.Counter()
    # function -> {(kind, nregs, via_pc): executions}
    shape: dict[str, collections.Counter] = collections.defaultdict(
        collections.Counter)
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
            n, c = int(row[4]), int(row[5])
            total[fn] += c
            if pc in entry:
                calls[fn] += n
            f = frame_insn.get(pc)
            if f is not None:
                frame[fn] += c
                shape[fn][f] += n

    regions = args.regions
    print("UNITS: profile cycles. 2 profile cycles = 1 project tick.")
    print(f"cost model: push(N) = {PUSH_BASE}+{PUSH_PER}N   "
          f"pop+pc(N) = {POPPC_BASE}+{POPPC_PER}N\n")
    header = (f"{'fn':<44}{'frame/fr':>9}{'tot/fr':>9}{'calls/fr':>9}"
              f"{'push':>6}{'pop':>5}{'hot%':>6}{'SPLIT':>8}{'LEAF':>8}"
              f"{'DELETE':>9}")
    print(header)
    print("-" * len(header))

    rows_out = []
    for fn, fcyc in frame.most_common(args.top):
        n = calls.get(fn, 0)
        if not n:
            continue
        sh = shape[fn]
        pushes = {k[1]: v for k, v in sh.items() if k[0] == "push"}
        pops = {k[1]: v for k, v in sh.items() if k[0] == "pop" and k[2]}
        max_push = max(pushes) if pushes else 0
        # The cheapest executed return is the hot route's; its share of entries
        # is how often the body is skipped.
        min_pop = min(pops) if pops else 0
        hot_exec = pops.get(min_pop, 0) if pops else 0
        hot_pct = 100.0 * hot_exec / n if n else 0.0

        per_call_now = push_cost(max_push) + poppc_cost(max_push)
        # SPLIT: the hot route keeps a minimal 1-register frame.
        split_save = max(0.0, (per_call_now - (push_cost(1) + poppc_cost(1)))
                         ) * n / regions
        leaf_save = per_call_now * n / regions
        delete_save = total[fn] / regions
        rows_out.append((fn, fcyc / regions, total[fn] / regions, n / regions,
                         max_push, min_pop, hot_pct, split_save, leaf_save,
                         delete_save))
        print(f"{fn[:43]:<44}{fcyc / regions:>9,.0f}{total[fn] / regions:>9,.0f}"
              f"{n / regions:>9,.1f}{max_push:>6}{min_pop:>5}{hot_pct:>6.0f}"
              f"{split_save:>8,.0f}{leaf_save:>8,.0f}{delete_save:>9,.0f}")

    print(f"\nTOP {len(rows_out)} totals, profile cycles/frame:")
    for label, idx in (("SPLIT (cold tail out, 1-reg hot frame)", 7),
                       ("LEAF  (frameless)", 8),
                       ("DELETE (call and body gone)", 9)):
        s = sum(r[idx] for r in rows_out)
        print(f"  {label:<42}{s:>10,.0f} cyc/frame = {s / 2:>9,.0f} ticks/frame")


if __name__ == "__main__":
    main()
