#!/usr/bin/env python3
"""Per-region execution counts for named functions, out of a v3 profile capture.

WHY THIS EXISTS. `census-marginal-frame-owners.py --reduce` collapses the v3
`arm9-profile.csv` to two columns per PC (whole match, and the marginal mask),
which answers "who costs the most" but destroys the per-frame axis. Several
questions need that axis back:

  * how many times per presented frame does mechanism X run?
  * does it run on the frames that SET P95, or on ordinary frames?
  * what is its exact call count?  (memory: `entry-PC-gives-exact-call-counts` --
    the profiler is instruction-accurate, so the instruction count at a
    function's ENTRY PC is exactly the number of calls to it.)

A per-frame guest counter answers the same questions and costs a build. This
costs one scan of a file that is already on disk.

WHAT IT CANNOT SEE, stated so nobody over-reads the output:

  * inlined callees have no entry PC of their own -- check `nm` first, and if
    the symbol is absent say so rather than reporting 0 calls;
  * a tail-called or `b`-entered function still counts, but a function entered
    only by falling through from its neighbour does not;
  * a region is a profiler region, not a tick-HUD frame -- the two instruments
    run on different binaries. Region indices are reported as regions.

Usage:
  python scripts/census-profile-pc-per-region.py \
      --profile artifacts/performance/<date>/<capture-dir> \
      --elf builds/<build>/<target>.elf \
      --symbols memcpy,f_read,get_fat.isra.0 \
      --marginal 80 --out <out.csv> [--series-out <series.csv>]
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import subprocess
import sys
from pathlib import Path

# Same resolution as census-marginal-frame-owners.py:59 -- devkitARM is not on
# PATH in every shell this repo is driven from.
NM = os.environ.get("NM", r"C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe")


def symbol_table(elf: Path) -> dict[str, tuple[int, int]]:
    out = subprocess.run(
        [NM, "-S", "--defined-only", str(elf)],
        check=True, capture_output=True, text=True).stdout
    table: dict[str, tuple[int, int]] = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 4:
            addr, size, _kind, name = parts
            table[name] = (int(addr, 16), int(size, 16))
        elif len(parts) == 3:
            addr, _kind, name = parts
            table.setdefault(name, (int(addr, 16), 0))
    return table


def load_regions(path: Path) -> dict[int, dict[str, int]]:
    regions: dict[int, dict[str, int]] = {}
    with path.open(newline="") as handle:
        for row in csv.DictReader(handle):
            regions[int(row["region"])] = {
                "total_cycles": int(row["total_cycles"]),
                "halt_wait": int(row["halt_wait"]),
                "instructions": int(row["instructions"]),
            }
    return regions


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--profile", required=True, type=Path,
                    help="capture directory holding arm9-profile.csv")
    ap.add_argument("--elf", required=True, type=Path)
    ap.add_argument("--symbols", required=True,
                    help="comma-separated symbol names")
    ap.add_argument("--marginal", type=int, default=80,
                    help="size of the P95 mask (regions), ranked on "
                         "total_cycles-halt_wait")
    ap.add_argument("--cycles-per-tick", type=int, default=2)
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--series-out", type=Path,
                    help="optional per-region series CSV (region x symbol)")
    ap.add_argument("--json", type=Path)
    args = ap.parse_args()

    csv_path = args.profile / "arm9-profile.csv"
    regions = load_regions(args.profile / "arm9-profile.regions.csv")

    # Region 0 is the pre-window accumulator (GATE_ARM_OWNERS.md 4.4) and is
    # never part of a frame mask.
    ranked = sorted(
        (r for r in regions if r != 0),
        key=lambda r: regions[r]["total_cycles"] - regions[r]["halt_wait"],
        reverse=True)
    marginal = set(ranked[:args.marginal])
    body = [r for r in ranked[args.marginal:]]

    table = symbol_table(args.elf)
    names = [s for s in args.symbols.split(",") if s]
    missing = [n for n in names if n not in table]
    pc_to_name: dict[bytes, str] = {}
    for name in names:
        if name in table:
            pc_to_name[b"0x%08x" % table[name][0]] = name

    # region -> name -> executions
    series: dict[str, dict[int, int]] = {n: {} for n in pc_to_name.values()}

    with csv_path.open("rb") as handle:
        handle.readline()  # header
        for line in handle:
            first = line.find(b",")
            if first < 0:
                continue
            second = line.find(b",", first + 1)
            pc = line[first + 1:second]
            name = pc_to_name.get(pc)
            if name is None:
                continue
            fields = line.split(b",")
            region = int(fields[0])
            series[name][region] = series[name].get(region, 0) + int(fields[4])

    per_tick = args.cycles_per_tick
    rows = []
    for name in names:
        if name not in series:
            rows.append({"symbol": name, "entry_pc": "ABSENT (inlined or "
                                                     "not linked)"})
            continue
        s = series[name]
        total = sum(s.values())
        marg = sum(v for r, v in s.items() if r in marginal)
        body_total = total - marg
        marg_regions = sum(1 for r in marginal if s.get(r, 0) > 0)
        body_regions = sum(1 for r in body if s.get(r, 0) > 0)
        rows.append({
            "symbol": name,
            "entry_pc": "0x%08x" % table[name][0],
            "size": table[name][1],
            "calls_whole_match": total,
            "calls_per_region": round(total / max(1, len(regions) - 1), 4),
            "calls_on_p95_set": marg,
            "calls_per_p95_region": round(marg / max(1, len(marginal)), 4),
            "calls_off_p95_set": body_total,
            "calls_per_body_region": round(body_total / max(1, len(body)), 4),
            "p95_regions_with_a_call": marg_regions,
            "p95_regions_total": len(marginal),
            "body_regions_with_a_call": body_regions,
            "body_regions_total": len(body),
            "presence_ratio": (
                round((marg / max(1, len(marginal))) /
                      max(1e-9, body_total / max(1, len(body))), 3)
                if body_total else "inf"),
        })

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()) if rows
                                else ["symbol"])
        writer.writeheader()
        for row in rows:
            writer.writerow(row)

    if args.series_out:
        cols = [n for n in names if n in series]
        with args.series_out.open("w", newline="") as handle:
            handle.write("region,ticks,in_p95_set," + ",".join(cols) + "\n")
            for r in sorted(regions):
                if r == 0:
                    continue
                ticks = ((regions[r]["total_cycles"] - regions[r]["halt_wait"])
                         // per_tick)
                handle.write("%d,%d,%d," % (r, ticks, 1 if r in marginal else 0)
                             + ",".join(str(series[c].get(r, 0))
                                        for c in cols) + "\n")

    summary = {
        "profile": str(csv_path),
        "elf": str(args.elf),
        "regions": len(regions) - 1,
        "marginal_regions": len(marginal),
        "marginal_threshold_ticks": (
            (regions[ranked[args.marginal - 1]]["total_cycles"]
             - regions[ranked[args.marginal - 1]]["halt_wait"]) // per_tick),
        "absent_symbols": missing,
        "rows": rows,
    }
    if args.json:
        args.json.write_text(json.dumps(summary, indent=2))
    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
