#!/usr/bin/env python3
"""Diff two or more `sample-tick-hud-buckets.ps1` JSON arms, bucket by bucket.

Reading three 22-row tables by eye is how a control that moved gets missed. This
prints the arms side by side with deltas against the first, and flags whether the
arms share a `romSha256` -- because a same-binary pair (a `.data` route) has no
placement term and a separately-linked pair has one worth about 14,080 ticks.

Usage:
  python scripts/compare-tick-hud-arms.py <baseline.json> <arm.json> [more.json...]
      [--stat mean|p50|p95] [--buckets FTR,STG,SRC,WORK-H]
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("arms", nargs="+", type=Path)
    parser.add_argument("--stat", default="mean",
                        choices=("mean", "p50", "p95"))
    parser.add_argument("--buckets", default="")
    args = parser.parse_args()

    arms = []
    for path in args.arms:
        with path.open(encoding="utf-8") as handle:
            data = json.load(handle)
        arms.append((path.stem, data,
                     {row["bucket"]: row for row in data["buckets"]}))

    shas = {arm[1].get("romSha256") for arm in arms}
    provenance = ("SAME BINARY (no placement term)" if len(shas) == 1
                  else "SEPARATELY LINKED (placement term ~14,080)")
    print(f"stat={args.stat}  arms={len(arms)}  {provenance}")
    for name, data, _ in arms:
        print(f"  {name:<24} rom {str(data.get('romSha256'))[:16]}  "
              f"samples {data.get('samples')}")
    print()

    wanted = [b.strip() for b in args.buckets.split(",") if b.strip()]
    names = wanted or arms[0][1]["bucketNames"]
    head = f"{'bucket':<8}" + "".join(f"{n[:14]:>16}" for n, _, _ in arms)
    print(head)
    print("-" * len(head))
    for bucket in names:
        base = arms[0][2].get(bucket)
        if base is None:
            continue
        line = f"{bucket:<8}{base[args.stat]:>16,}"
        for _, _, rows in arms[1:]:
            row = rows.get(bucket)
            if row is None:
                line += f"{'-':>16}"
                continue
            delta = row[args.stat] - base[args.stat]
            line += f"{row[args.stat]:>10,}{delta:>+7,}"
        print(line)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
