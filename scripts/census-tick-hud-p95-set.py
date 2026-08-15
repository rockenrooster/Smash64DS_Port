#!/usr/bin/env python3
"""Decompose a tick-HUD rows CSV into the two frame sets the gate is written on.

`sample-tick-hud-buckets.ps1` prints per-bucket P50/P95 over the whole run. The
gate is not a whole-run statistic: **P95 of 1,600 samples IS the 80th-largest
value**, so the only frames that can move it are the 80 most expensive ones.
`artifacts/performance/2026-08-14_runtime2-p95-closure/MARGINAL_OWNERS.md` §2
computed that decomposition by hand; this script is that computation, so the
next re-bank costs nothing and cannot drift.

TWO SETS, AND THEY ARE NOT THE SAME SET.

  P95 set      the N most expensive frames by WORK-H (N = ceil(0.05 * samples))
               -> governs `P95 <= 1,120,380`
  cadence set  the M cheapest DROPPED frames (VBlank interval >= 3)
               -> governs the >=95% two-VBlank target

`plan.md` §0 conflated them once and quoted the worst cadence frame's
requirement as if it were the set's.

WHY THE BASELINE IS THE TWO-VBLANK MEAN. "Excess" here is cost above a frame
that actually presented on time, which is the thing a package has to convert.
Ranking on the set's own mean instead would rank the tail against itself.

WHY `ALL` IS ONLY USED FOR CADENCE. `ALL` is the VBlank-quantized present
interval, so sorting on it sorts rounding noise (memory:
`a-threshold-on-the-quantum-sorts-noise`). Every cost figure below is WORK-H
and its child brackets; `ALL` is used only to derive the VBlank interval.

NESTING. `MARGINAL_OWNERS.md` §5: `SRC` contains `GCRA`, which contains
`{SINT (which contains SCPU), SHDT, SPHD/SPHC, SCAT, SPRM}`. Take a parent OR
its children, never both. The derived rows `SITR = SINT - SCPU`,
`SRC-GCRA` and the `GCRA remainder` are printed so the reading is explicit.

Usage:
  python scripts/census-tick-hud-p95-set.py --rows <rows.csv> [--label ARM]
      [--apparatus 24947] [--json out.json]
"""

from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path

# One VBlank of ARM9 ticks, the divisor the campaign uses to turn the
# VBlank-quantized `ALL` bucket into a present interval.
VBLANK_TICKS = 560190
GATE = 1120380

# The completeness identity the tick HUD guarantees (taskman_seam.c:5201-5206).
NAMED = ("FTR", "STG", "BG", "AUD", "SRC", "MISC")


def load_rows(path: Path) -> list[dict]:
    with path.open(newline="") as handle:
        rows = [
            {k: int(v) for k, v in row.items()}
            for row in csv.DictReader(handle)
        ]
    if not rows:
        raise SystemExit(f"{path} has no rows")
    return rows


def percentile_rank(sorted_desc: list[int], fraction: float) -> tuple[int, int]:
    """Return (rank, value) for the campaign's percentile convention.

    P95 of n samples is the ceil(0.05 * n)-th largest, i.e. 80th of 1,600.
    """
    rank = max(1, math.ceil(fraction * len(sorted_desc)))
    return rank, sorted_desc[rank - 1]


def mean(values) -> float:
    values = list(values)
    return sum(values) / len(values) if values else 0.0


def bucket_excess(hot: list[dict], base: list[dict], key: str) -> float:
    return mean(r[key] for r in hot) - mean(r[key] for r in base)


def derived(rows: list[dict], key: str) -> list[int]:
    if key == "SITR":
        return [r["SINT"] - r["SCPU"] for r in rows]
    if key == "SRC-GCRA":
        return [r["SRC"] - r["GCRA"] for r in rows]
    if key == "GCRA-REM":
        return [
            r["GCRA"] - r["SINT"] - r["SHDT"] - r["SPHD"] - r["SPHC"]
            - r["SCAT"] - r["SPRM"]
            for r in rows
        ]
    if key == "OTHR-WAIT":
        return [r["OTHR"] - r["WAIT"] for r in rows]
    return [r[key] for r in rows]


def derived_excess(hot: list[dict], base: list[dict], key: str) -> float:
    return mean(derived(hot, key)) - mean(derived(base, key))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--rows", type=Path, required=True)
    ap.add_argument("--label", default="")
    ap.add_argument("--cadence", type=int, default=160,
                    help="size of the cadence set (cheapest dropped frames)")
    ap.add_argument("--apparatus", type=int, default=24947,
                    help="approved instrument apparatus, RESIDUE.md section 5")
    ap.add_argument("--json", type=Path, default=None)
    args = ap.parse_args()

    rows = load_rows(args.rows)
    n = len(rows)
    for r in rows:
        r["VBI"] = int(round(r["ALL"] / VBLANK_TICKS))

    # ---- completeness, frame by frame -----------------------------------
    worst = 0
    for r in rows:
        lhs = r["WORK-H"]
        rhs = sum(r[k] for k in NAMED) + r["OTHR"] - r["WAIT"]
        worst = max(worst, abs(lhs - rhs))
    hud_identity = max(abs(r["WORK"] - r["HUD"] - r["WORK-H"]) for r in rows)

    work_desc = sorted((r["WORK-H"] for r in rows), reverse=True)
    p95_rank, p95 = percentile_rank(work_desc, 0.05)
    p90_rank, p90 = percentile_rank(work_desc, 0.10)
    p99_rank, p99 = percentile_rank(work_desc, 0.01)
    p50 = sorted(work_desc)[(n - 1) // 2] if n % 2 else \
        (sorted(work_desc)[n // 2 - 1] + sorted(work_desc)[n // 2]) // 2

    hist = {}
    for r in rows:
        hist[r["VBI"]] = hist.get(r["VBI"], 0) + 1
    two = [r for r in rows if r["VBI"] == 2]
    dropped = sorted((r for r in rows if r["VBI"] >= 3),
                     key=lambda r: r["WORK-H"])
    boundary = max((r["WORK-H"] for r in two), default=0)

    # ---- the two sets ---------------------------------------------------
    p95_set = sorted(rows, key=lambda r: r["WORK-H"], reverse=True)[:p95_rank]
    cadence_set = dropped[:args.cadence]

    out = {
        "rows": str(args.rows),
        "label": args.label,
        "samples": n,
        "identity_max_abs_error": worst,
        "work_minus_hud_identity_max_abs_error": hud_identity,
        "work_h": {
            "p50": p50, "p90": p90, "p95": p95, "top1pct": p99,
            "p95_rank": p95_rank, "p90_rank": p90_rank, "p99_rank": p99_rank,
            "max": work_desc[0], "min": work_desc[-1],
            "p95_net_of_apparatus": p95 - args.apparatus,
            "apparatus": args.apparatus,
            "gate": GATE,
            "gap_raw": p95 - GATE,
            "gap_net": p95 - args.apparatus - GATE,
        },
        "cadence": {
            "histogram": {str(k): hist[k] for k in sorted(hist)},
            "max_interval": max(hist),
            "two_vblank_share": len(two) / n,
            "boundary_work_h": boundary,
            "dropped": len(dropped),
        },
    }

    print(f"rows           {args.rows}")
    if args.label:
        print(f"arm            {args.label}")
    print(f"samples        {n}")
    print(f"identity       max |WORK-H - (named + OTHR - WAIT)| = {worst}  "
          f"max |WORK - HUD - WORK-H| = {hud_identity}")
    print()
    print("WORK-H         P50 {:,}  P90 {:,} (rank {})  P95 {:,} (rank {})  "
          "top-1% {:,} (rank {})  max {:,}".format(
              p50, p90, p90_rank, p95, p95_rank, p99, p99_rank, work_desc[0]))
    print("               P95 neighbourhood: " + "  ".join(
        "r{}={:,}".format(i + 1, work_desc[i])
        for i in range(max(0, p95_rank - 4), min(len(work_desc), p95_rank + 3))))
    print("               net of apparatus {:,}   raw gap {:+,}   "
          "net gap {:+,}".format(
              p95 - args.apparatus, p95 - GATE, p95 - args.apparatus - GATE))
    print()
    print("cadence (ALL / {:,})  ".format(VBLANK_TICKS) + "  ".join(
        f"{k}:{hist[k]}" for k in sorted(hist)) +
        f"   max {max(hist)}   two-VBlank {len(two) / n:.1%}"
        f"   boundary WORK-H {boundary:,}")
    print()

    keys = ["WORK-H", "SRC", "GCRA", "SRC-GCRA", "SINT", "SCPU", "SITR",
            "SHDT", "SPHD", "SPHC", "SPRM", "SCAT", "GCRA-REM",
            "MISC", "AUD", "FTR", "STG", "BG", "OTHR-WAIT", "SWRM"]

    for name, hot in (("P95 SET (top {} by WORK-H)".format(p95_rank), p95_set),
                      ("CADENCE SET ({} cheapest dropped)".format(
                          len(cadence_set)), cadence_set)):
        base_mean_work = mean(r["WORK-H"] for r in two)
        total = derived_excess(hot, two, "WORK-H")
        print(f"== {name} vs the {len(two)} two-VBlank frames "
              f"(WORK-H mean {base_mean_work:,.0f}) ==")
        print("  {:<10} {:>13} {:>13} {:>13} {:>7} {:>8}".format(
            "bracket", "set mean", "2-VBI mean", "excess", "ratio", "share"))
        table = {}
        for k in keys:
            hm = mean(derived(hot, k))
            bm = mean(derived(two, k))
            ex = hm - bm
            ratio = (hm / bm) if bm > 0 else float("nan")
            share = ex / total if total else 0.0
            table[k] = {"set_mean": hm, "base_mean": bm, "excess": ex,
                        "ratio": ratio, "share": share}
            print("  {:<10} {:>13,.0f} {:>13,.0f} {:>+13,.0f} {:>7} {:>7.1%}"
                  .format(k, hm, bm, ex,
                          "-" if bm <= 0 else "{:.2f}x".format(ratio), share))
        # children of GCRA must reconstruct GCRA
        child = sum(table[k]["excess"] for k in
                    ("SINT", "SHDT", "SPHD", "SPHC", "SCAT", "SPRM",
                     "GCRA-REM"))
        print("  check: GCRA excess {:+,.0f} vs sum of its children {:+,.0f} "
              "(delta {:+,.0f})".format(
                  table["GCRA"]["excess"], child,
                  table["GCRA"]["excess"] - child))
        named_sum = sum(table[k]["excess"] for k in NAMED) + \
            table["OTHR-WAIT"]["excess"]
        print("  check: WORK-H excess {:+,.0f} vs named+OTHR-WAIT {:+,.0f} "
              "(delta {:+,.0f})".format(
                  total, named_sum, total - named_sum))
        # instrument share of the set
        burst = [r for r in hot if r["HUD"] > 100000]
        below = [r for r in hot if r["WORK-H"] <= boundary]
        print("  instrument: {} of {} carry a HUD draw burst (HUD>100,000); "
              "{} sit at or below the cadence boundary".format(
                  len(burst), len(hot), len(below)))
        print()
        out[name.split(" ")[0].lower() + "_set"] = {
            "n": len(hot), "table": table,
            "burst_frames": len(burst), "below_boundary": len(below),
        }

    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(out, indent=2))
        print(f"wrote {args.json}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
