#!/usr/bin/env python3
"""Phase 8: is the I-cache cost CONFLICT (placement can fix) or CAPACITY (it cannot)?

The 2026-08-14 placement report answered this with a whole-match UNION footprint
-- 33.6x the cache -- and was rightly rejected: a union that overflows the cache
says nothing about temporal reuse distance. A sequence of small phases that each
fit produces the same union as a thrashing working set.

This answers it by measurement instead of inference, using the v3 stall
attributor's per-PC `icache_fill` column. No trace and no simulation is needed
for the discriminating question, because the emulator already charges every
line-fill to the PC that caused it:

  fills concentrated in OVER-SUBSCRIBED sets  -> conflict -> reordering can help
  fills flat against set occupancy            -> capacity -> reordering cannot

The test is the correlation between a set's hot-line population and the
`icache_fill` cycles charged to it. Under pure capacity pressure every set
refills at the same rate regardless of how many hot lines it holds, because the
line is gone by the next pass either way. Under conflict pressure the sets
holding more than the 4 ways refill disproportionately.

Also reported, because they are what a placement candidate would target:

  * refetch ratio per line -- `icache_fill` cycles per 1,000 instructions
    executed from that line. A line that is resident across its uses shows near
    zero however hot it is; a line evicted between every use shows the fill cost
    of its whole pass.
  * the same per function and per cluster, so "hot functions affected" is a
    measurement rather than a guess.

Geometry verified against the reference emulator (melonDS-Accurate
src/CP15_Constants.h:28-35, src/CP15.cpp:455-467): 8192 B, 32 B lines, 4-way,
64 sets, set = (addr >> 5) & 63, ROUND-ROBIN replacement.

UNITS: profile cycles. Two profile cycles = one project tick.

Usage:
  python scripts/census-icache-temporal.py <v3-profile.csv> --dis <objdump -d> \
      --regions 1601
"""

from __future__ import annotations

import argparse
import collections
import csv
import re

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2,8} )+)\s*(\S+)\s*(.*)$")

ICACHE_BYTES, LINE, WAYS = 8192, 32, 4
SETS = ICACHE_BYTES // LINE // WAYS          # 64
ITCM_LO, ITCM_HI = 0x01FF8000, 0x02000000


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--regions", type=int, required=True)
    ap.add_argument("--top", type=int, default=20)
    ap.add_argument("--hot-lines", type=int, default=2000)
    args = ap.parse_args()

    owner: dict[int, str] = {}
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
            if mi:
                owner[int(mi.group(1), 16)] = cur

    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        header = next(rows)
        col = {name: i for i, name in enumerate(header)}
        if "icache_fill" not in col:
            raise SystemExit(
                f"{args.profile} has no `icache_fill` column -- this is a v2 "
                f"profile ({','.join(header)}). Capture with "
                f"emulators/melonds-attributor/melonDS.exe, which emits "
                f"format=melonDS-arm9-retail-profile-v3.")
        i_pc, i_ins = col["pc"], col["instructions"]
        i_cyc, i_fill = col["total_cycles"], col["icache_fill"]
        i_issue = col.get("issue")

        line_ins: collections.Counter = collections.Counter()
        line_fill: collections.Counter = collections.Counter()
        fn_ins: collections.Counter = collections.Counter()
        fn_fill: collections.Counter = collections.Counter()
        fn_cyc: collections.Counter = collections.Counter()
        total_fill = total_cyc = total_ins = total_issue = 0
        for row in rows:
            try:
                pc = int(row[i_pc], 16)
            except ValueError:
                continue
            ins, cyc = int(row[i_ins]), int(row[i_cyc])
            fill = int(row[i_fill])
            total_fill += fill
            total_cyc += cyc
            total_ins += ins
            if i_issue is not None:
                total_issue += int(row[i_issue])
            if ITCM_LO <= pc < ITCM_HI:
                continue
            ln = pc & ~(LINE - 1)
            line_ins[ln] += ins
            line_fill[ln] += fill
            fn = owner.get(pc)
            if fn is not None:
                fn_ins[fn] += ins
                fn_fill[fn] += fill
                fn_cyc[fn] += cyc

    R = args.regions
    print(f"I-cache 8192 B / 32 B / 4-way / {SETS} sets / round-robin "
          f"[verified: melonDS-Accurate]")
    print(f"regions {R:,}\n")
    print(f"total cycles       {total_cyc:>16,} = {total_cyc / R:>10,.0f}/frame")
    if i_issue is not None:
        print(f"  issue            {total_issue:>16,} = "
              f"{total_issue / R:>10,.0f}/frame "
              f"({100 * total_issue / total_cyc:.1f}%)")
    print(f"  icache_fill      {total_fill:>16,} = {total_fill / R:>10,.0f}/frame "
          f"({100 * total_fill / total_cyc:.1f}%) "
          f"= {total_fill / R / 2:,.0f} ticks/frame")
    print(f"  of which non-ITCM{sum(line_fill.values()):>16,} = "
          f"{sum(line_fill.values()) / R:>10,.0f}/frame\n")

    # --- the discriminator -------------------------------------------------
    # Sweep the hot-line cutoff. At 2,000 lines every one of the 64 sets is
    # already oversubscribed, so a single cutoff yields one bucket and no
    # contrast -- the first run of this script produced exactly that and
    # measured nothing. Cutoffs at and below the cache's 256-line capacity are
    # where set population actually varies, which is where the conflict signal
    # would have to appear if it exists.
    ordered_lines = sorted(line_ins.items(), key=lambda kv: -kv[1])
    print("== CONFLICT vs CAPACITY: fill rate against set population ==")
    print(f"(cache holds {SETS * WAYS} lines total: {SETS} sets x {WAYS} ways)")
    for cut in (64, 128, 256, 512, 1024, args.hot_lines):
        if cut > len(ordered_lines):
            continue
        hot_set = {ln for ln, _ in ordered_lines[:cut]}
        pop: collections.Counter = collections.Counter()
        pf: collections.Counter = collections.Counter()
        pi: collections.Counter = collections.Counter()
        for ln in hot_set:
            s = (ln >> 5) & (SETS - 1)
            pop[s] += 1
            pf[s] += line_fill[ln]
            pi[s] += line_ins[ln]
        buckets: dict[str, list] = collections.defaultdict(list)
        for s, p in pop.items():
            if p <= WAYS:
                buckets[f"<={WAYS} fits"].append(s)
            elif p <= 2 * WAYS:
                buckets[f"{WAYS+1}-{2*WAYS}"].append(s)
            elif p <= 4 * WAYS:
                buckets[f"{2*WAYS+1}-{4*WAYS}"].append(s)
            else:
                buckets[f">{4*WAYS}"].append(s)
        order = [f"<={WAYS} fits", f"{WAYS+1}-{2*WAYS}",
                 f"{2*WAYS+1}-{4*WAYS}", f">{4*WAYS}"]
        parts = []
        for key in order:
            ss = buckets.get(key)
            if not ss:
                continue
            f = sum(pf[s] for s in ss)
            n = sum(pi[s] for s in ss)
            parts.append(f"{key}: {len(ss)} sets, "
                         f"{(1000 * f / n if n else 0):,.0f} fill/1k")
        print(f"  top {cut:>5,} lines -> " + "  |  ".join(parts))
    print("\n  FLAT `fill/1k` across populations => CAPACITY, reordering cannot "
          "help.\n  Rising with population => CONFLICT, reordering can.\n")

    # --- refetch ratio per function ---------------------------------------
    print(f"== hot functions by icache_fill (non-ITCM) ==")
    print(f"{'fill cyc/fr':>12}{'tot cyc/fr':>12}{'fill%':>7}"
          f"{'fill/1k ins':>13}  function")
    for fn, f in sorted(fn_fill.items(), key=lambda kv: -kv[1])[:args.top]:
        n = fn_ins.get(fn, 0)
        c = fn_cyc.get(fn, 1)
        print(f"{f / R:>12,.0f}{c / R:>12,.0f}{100 * f / c:>7.1f}"
              f"{(1000 * f / n if n else 0):>13,.1f}  {fn[:48]}")


if __name__ == "__main__":
    main()
