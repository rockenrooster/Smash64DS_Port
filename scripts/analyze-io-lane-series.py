#!/usr/bin/env python3
"""Price a SYMBOL FAMILY at P95 from its own per-frame series -- no mask needed.

Why this exists, next to `analyze-profile-region-split.py`
----------------------------------------------------------
That tool carries a per-frame MASK in (a gate lane column) and asks what the
profile ran on those frames. It is the right tool when the question starts from
a lane. It is the wrong one when the question starts from a *mechanism* --
"what does the file-I/O family cost, and what would deleting it pay" -- because
the mask then has to stand in for the mechanism and every premium it reports is
diluted by however badly the mask fits (2026-08-13: r = 0.24, premiums a factor
2.03 low, and `BAND_OWNER.md` s5 named the wrong owner off one).

The profiler emits one row per (region, pc), so the region axis IS a per-frame
series for every symbol. Sum a family over it and you have the mechanism's own
series -- exact, undiluted, and directly subtractable from a gate run's
`WORK-H` column. That is the whole tool.

Three things it prints, and each is load-bearing:

* **presence and runs.** A family on 167 frames in 156 runs is 156 separate
  bursts; one on 1,600 frames in 1 run is a per-frame tax. The two are priced
  completely differently at a percentile.
* **`P95 if removed`** -- subtract the family's ticks from the aligned gate
  frame and re-take the 80th largest. This is the answer, and it needs the
  alignment to be right.
* **`P95 worst-case pairing`** -- sort the family's per-frame ticks descending,
  subtract them from the gate's frames sorted descending, re-take rank 80. This
  needs NO alignment at all: it assumes the bursts landed on the costliest
  frames they possibly could have. **If the worst-case bound is under your bar,
  the lever is dead no matter how the alignment turned out.** Always read it
  before believing the aligned number.

`--cooccur A=sym,... B=sym,...` answers "does every frame that carries A also
carry B", which is what NAMES a trigger. A ratio does not: two mechanisms driven
by the same event both spike together. Zero exceptions in one direction is the
finding (2026-08-13: 91 of 91 file-I/O frames carried an FGM play; only 25.7% of
them carried the status change the previous cycle had blamed).

Alignment
---------
`ticks/frame = cycles / (2 x regions)` -- the ARM9 runs at 2x the tick timer
(`artifacts/performance/2026-08-13_c-residue/RESIDUE.md` section 0).
Profile region `i+1` <-> gate row `i` (`--gate-offset 1`, the default and the
measured value for every 1,600-sample run in this campaign). **Sort
`arm9-profile.regions.csv` by `region` before correlating anything against it**
-- the file is not written in region order, and reading it as written makes
every offset look equally wrong.

Usage
-----
    python scripts/analyze-io-lane-series.py \
        --cache <npz written by analyze-profile-region-split.py --cache> \
        --gate-csv artifacts/performance/<gate>/gate-rows.csv \
        --group "FAT-IO=get_fat.isra.0,f_lseek,f_read,..." \
        --cooccur "io=get_fat.isra.0,f_read" "play=ndsAudioFgmPlayAtPan"

The cache comes from one pass of `analyze-profile-region-split.py --cache`; this
tool never touches the 2.6 GB csv, so iterating on families is instant.
"""
from __future__ import annotations

import argparse
import csv

import numpy as np


def rank80(v: np.ndarray) -> float:
    return float(np.sort(v)[::-1][79])


def runs_of(mask: np.ndarray) -> int:
    m = mask.astype(np.int8)
    return int((np.diff(np.concatenate(([0], m, [0]))) == 1).sum())


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--cache", required=True,
                    help="npz from analyze-profile-region-split.py --cache")
    ap.add_argument("--gate-csv", required=True)
    ap.add_argument("--gate-lane", default="WORK-H")
    ap.add_argument("--gate-offset", type=int, default=1,
                    help="profile region i+offset <-> gate row i (measured 1)")
    ap.add_argument("--group", action="append", default=[],
                    metavar="NAME=SYM[,SYM...]")
    ap.add_argument("--cooccur", nargs="+", default=[], metavar="NAME=SYM[,SYM...]",
                    help="two or more families; prints the presence overlap "
                         "in both directions")
    ap.add_argument("--top", type=int, default=12)
    args = ap.parse_args()

    blob = np.load(args.cache, allow_pickle=True)
    names = list(blob["names"])
    cyc, calls = blob["cyc"], blob["calls"]
    idx = {n: i for i, n in enumerate(names)}
    live = np.arange(1, cyc.shape[0] - 1)   # region 0 and the trailing partial
    R = len(live)
    per_cyc, per_calls = cyc[live], calls[live]
    print(f"regions      {R}")
    print(f"basis        ticks = cycles / (2 x {R}) -- ARM9 is 2x the tick timer")

    rows = list(csv.DictReader(open(args.gate_csv, newline="", encoding="utf-8")))
    gate = np.array([int(r[args.gate_lane]) for r in rows], dtype=np.float64)
    G = len(gate)
    base = rank80(gate)
    print(f"gate         {args.gate_csv} lane {args.gate_lane}, {G} rows, "
          f"rank-80 {base:,.0f}\n")

    def resolve(spec: str):
        label, syms = spec.split("=", 1)
        want = [s for s in syms.split(",") if s]
        missing = [s for s in want if s not in idx]
        cols = [idx[s] for s in want if s in idx]
        return label, cols, missing

    for spec in args.group:
        label, cols, missing = resolve(spec)
        tot = per_cyc[:, cols].sum(axis=1)
        cal = per_calls[:, cols].sum(axis=1)
        tk = tot / 2.0                        # ticks on that frame
        present = tot > 0
        print(f"=== {label} ===  ({len(cols)} symbols"
              + (f", MISSING {missing}" if missing else "") + ")")
        print(f"  whole-match      {tot.sum():>15,} cyc = "
              f"{tot.sum() / (2 * R):>9,.0f} tk/frame mean")
        print(f"  calls            {cal.sum():>15,} total = "
              f"{cal.sum() / R:>9,.1f} /frame")
        print(f"  present on       {int(present.sum()):>5,} of {R} regions "
              f"in {runs_of(present)} runs")
        if present.any():
            p = tk[present]
            print(f"  when present     mean {p.mean():>9,.0f} tk  "
                  f"median {np.median(p):>9,.0f}  max {p.max():>9,.0f} tk")
            order = np.argsort(tot)[::-1][:args.top]
            print("  top regions (region:ticks(calls)):")
            print("   " + "  ".join(
                f"{int(live[i])}:{int(tk[i]):,}({int(cal[i])})" for i in order))
        cand = gate.copy()
        off = args.gate_offset
        for i in range(R):
            gi = live[i] - off
            if 0 <= gi < G:
                cand[gi] -= tk[i]
        print(f"  P95 if removed             {rank80(cand):>12,.0f}   "
              f"delta {rank80(cand) - base:>+9,.0f}   (aligned, offset {off:+d})")
        # Alignment-free upper bound: pair the largest bursts with the largest
        # frames. If THIS is under the bar the lever is dead however the two
        # runs line up.
        v = np.sort(tk[present])[::-1]
        c = np.sort(gate)[::-1].copy()
        n = min(len(v), len(c))
        c[:n] -= v[:n]
        print(f"  P95 worst-case pairing     {rank80(c):>12,.0f}   "
              f"delta {rank80(c) - base:>+9,.0f}   (needs no alignment)\n")

    if len(args.cooccur) >= 2:
        fams = [resolve(s) for s in args.cooccur]
        masks = {lbl: per_cyc[:, cols].sum(axis=1) > 0 for lbl, cols, _ in fams}
        print("=== co-occurrence (presence, not cost) ===")
        for a in masks:
            for bl in masks:
                if a == bl:
                    continue
                A, B = masks[a], masks[bl]
                print(f"  {a:>18s} on {int(A.sum()):>5,} frames; "
                      f"{int((A & B).sum()):>5,} also carry {bl} "
                      f"({100 * (A & B).sum() / max(A.sum(), 1):5.1f}%); "
                      f"{a} frames WITHOUT {bl}: {int((A & ~B).sum()):>4,}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
