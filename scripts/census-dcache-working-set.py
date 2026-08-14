#!/usr/bin/env python3
"""Rank ARM9 data loads by stall, with the address space CLASSIFIED.

`analyze-dcache-stalls.py` ranks data loads by excess cycles over a cached
access and already excludes literal-pool and stack loads. That is not enough to
drive a layout change, and the 2026-08-14 census is why: its number-one site by
a factor of two is

    ndsRendererTask36ReplayRun   ldr r3, [r1, #184]   507.2 cyc/ex

which is not a cache miss at all. `r1` is `0x04000000` and `+184` is `DMA0CNT`;
the three instructions around it are a DMA source/dest/control program followed
by `cmp r3,#0 / blt .-8`, i.e. a busy-wait for a synchronous DMA to the GX FIFO.
507 cycles is the geometry engine draining, and no struct repack touches it.

So this pass walks BACKWARD from each hot load to find where its base register
was last set, and classifies the site:

  mmio        base built from 0x04000000 -- I/O register, a hardware wait
  timer       an mmio site inside the profiler's own clock helpers
  cacheable   an ordinary main-RAM/ROM access, the only class a layout change
              can help
  unknown     base not resolvable in a short backward window -- a load that may
              be either, and is counted as NEITHER

Only `cacheable` rows are ranked for layout work. The others are reported
separately so they cannot be silently counted, and so the DMA wait is visible as
the scheduling question it actually is.

`unknown` fails CLOSED. Until 2026-08-14 an unresolvable base defaulted to
`cacheable`, which is fail-open in the one direction that matters: the size of
the cacheable class is the number that decides whether the layout lane is worth
opening, so every site the backward walk could not reach was inflating the case
for opening it. A base built from a literal pool or across a call boundary is as
likely to be an I/O register as main RAM.

UNITS: every figure this script prints is a PROFILE CYCLE. Two profile cycles are
one project tick -- divide by two before comparing anything here against a gate,
a floor, or a ticks/frame budget.
"""

from __future__ import annotations

import argparse
import collections
import csv
import re

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2,8} )+)\s*(\S+)\s*(.*)$")
# `ldr r3, [r1, #184]` / `ldrb r5, [r4, #5]` / `ldr r3, [r3, r0]` / `ldmia r1!`
BASE = re.compile(r"\[(\w+)")
IMM_MOV = re.compile(r"^\s*(?:mov|movw)\s+(\w+),\s*#(\d+)")
ORR_IMM = re.compile(r"^\s*orr\s+(\w+),\s*(\w+),\s*#(\d+)")

LOAD_OPS = ("ldr", "ldm", "ldrd")
# The profiler's own clock reads. They are MMIO by nature and they exist only in
# the instrumented build, so they must never enter a layout ranking.
TIMER_FUNCS = {"cpuGetTiming", "tickGetCount", "ndsTickHudSample"}

IO_LO, IO_HI = 0x04000000, 0x04FFFFFF


def resolve_base(lines, index, base_reg, window=24):
    """Walk backward for the last write to base_reg. -> int address or None."""
    for i in range(index - 1, max(index - window, -1), -1):
        text = lines[i]
        m = IMM_MOV.match(text)
        if m and m.group(1) == base_reg:
            return int(m.group(2))
        m = ORR_IMM.match(text)
        if m and m.group(1) == base_reg:
            inner = resolve_base(lines, i, m.group(2), window // 2)
            return None if inner is None else inner | int(m.group(3))
        # Any other definition of the register ends the search.
        if re.match(r"^\s*\w+\s+" + re.escape(base_reg) + r"\s*,", text):
            if text.strip().startswith(("cmp", "tst", "teq", "cmn")):
                continue
            return None
    return None


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--regions", type=int, required=True,
                    help="frames in the profile, for per-frame numbers")
    ap.add_argument("--baseline", type=float, default=3.0)
    ap.add_argument("--min-execs", type=int, default=2000)
    ap.add_argument("--top", type=int, default=60)
    args = ap.parse_args()

    site: dict[int, tuple] = {}
    func_lines: dict[str, list[str]] = {}
    func_index: dict[int, tuple[str, int]] = {}
    cur, cur_lines = None, []
    with open(args.dis, errors="ignore") as handle:
        for raw in handle:
            m = FUNC.match(raw.rstrip("\n"))
            if m:
                if cur is not None:
                    func_lines[cur] = cur_lines
                cur, cur_lines = m.group(2), []
                continue
            if cur is None:
                continue
            mi = INSN.match(raw)
            if not mi:
                continue
            pc = int(mi.group(1), 16)
            text = (mi.group(3) + " " + mi.group(4)).strip()
            func_index[pc] = (cur, len(cur_lines))
            cur_lines.append(text)
            mnem = mi.group(3).lower()
            ops = mi.group(4)
            if not mnem.split(".")[0].startswith(LOAD_OPS):
                continue
            if "[pc" in ops or "@ (" in ops or "[sp" in ops:
                continue
            site[pc] = (cur, text)
        if cur is not None:
            func_lines[cur] = cur_lines

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

    ranked = []
    for pc, n in execs.items():
        if n < args.min_execs:
            continue
        func, text = site[pc]
        excess = cycles[pc] - n * args.baseline
        if excess <= 0:
            continue
        base_m = BASE.search(text)
        base_reg = base_m.group(1) if base_m else None
        kind = "unknown"
        addr = None
        if func in TIMER_FUNCS:
            kind = "timer"
        elif base_reg and func in func_lines:
            fn, idx = func_index[pc]
            addr = resolve_base(func_lines[fn], idx, base_reg)
            if addr is None:
                # An unresolved base is UNKNOWN, not cacheable. Calling it
                # cacheable was a fail-open default that silently enrolled every
                # site whose base the backward window could not reach into the
                # one class a layout change is allowed to act on -- the class
                # whose size decides whether the lane is worth opening. A base
                # this walk cannot resolve may equally be an MMIO register
                # loaded from a literal pool or built across a call boundary.
                kind = "unknown"
            elif IO_LO <= addr <= IO_HI:
                kind = "mmio"
            else:
                kind = "cacheable"
        ranked.append((excess, cycles[pc], n, pc, func, text, kind, addr))
    ranked.sort(reverse=True)

    by_kind = collections.Counter()
    for excess, _c, _n, _pc, _f, _t, kind, _a in ranked:
        by_kind[kind] += excess
    total = sum(by_kind.values())
    print(f"regions (frames): {args.regions}")
    print(f"data-load excess over a {args.baseline:g}-cycle cached access: "
          f"{total:,} cycles = {total / args.regions:,.0f}/frame\n")
    print(f"{'class':<12}{'excess cycles':>16}{'/frame':>12}{'share':>8}")
    for kind in ("cacheable", "mmio", "timer", "unknown"):
        v = by_kind.get(kind, 0)
        if not v:
            continue
        print(f"{kind:<12}{v:>16,}{v / args.regions:>12,.0f}{100 * v / total:>7.1f}%")
    print("\nONLY `cacheable` can be helped by a data-layout change.\n")

    for kind in ("cacheable", "mmio"):
        rows = [r for r in ranked if r[6] == kind][:args.top]
        print(f"== {kind.upper()} sites, ranked by excess ==")
        print(f"{'cyc/ex':>8}{'execs':>10}{'excess':>13}{'/frame':>9}  "
              f"{'function':<44}instruction")
        for excess, c, n, pc, func, text, _k, addr in rows:
            note = f"  [base {addr:#x}]" if addr is not None and kind == "mmio" else ""
            print(f"{c / n:>8.1f}{n:>10,}{excess:>13,.0f}"
                  f"{excess / args.regions:>9,.0f}  {func[:43]:<44}{text}{note}")
        print()


if __name__ == "__main__":
    main()
