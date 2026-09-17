#!/usr/bin/env python3
"""Count four-fighter roster/stage configurations; no ROM or source-catalogue proof.

Python 3.10+, standard library only. This planning helper accepts counts, NOT
source-derived fighter/stage manifests. N00.06 must implement the real catalogue
adapter and exact-case enumeration. These totals are not runtime test results.
"""
from __future__ import annotations
import argparse
import json
import math


def count_cases(fighters: int, stages: int) -> dict:
    """Return independent multiplicity and closed-form base/ordered counts."""
    for name, value in (("fighters", fighters), ("stages", stages)):
        if type(value) is not int or value < 1:
            raise ValueError(f"{name} must be a positive integer")
    def choose(n: int, k: int) -> int:
        return math.comb(n, k) if n >= k >= 0 else 0
    rosters = {
        "AAAA": (fighters, 1),
        "AAAB": (fighters * (fighters - 1), 4),
        "AABB": (choose(fighters, 2), 6),
        "AABC": (fighters * choose(fighters - 1, 2), 12),
        "ABCD": (choose(fighters, 4), 24),
    }
    patterns = {
        name: {"base_rosters": n, "orders_per_roster": orders,
               "base_cases": n * stages,
               "ordered_cases": n * orders * stages}
        for name, (n, orders) in rosters.items()
    }
    base_cases = choose(fighters + 3, 4) * stages
    ordered_cases = fighters ** 4 * stages
    if sum(p["base_cases"] for p in patterns.values()) != base_cases:
        raise AssertionError("base multiplicity count disagrees with formula")
    if sum(p["ordered_cases"] for p in patterns.values()) != ordered_cases:
        raise AssertionError("ordered multiplicity count disagrees with formula")
    return {
        "nature": "ENUMERATION_ARITHMETIC_ONLY_NOT_RUNTIME_QUALIFICATION",
        "fighters": fighters, "stages": stages, "slots": 4,
        "base_cases": base_cases, "ordered_cases": ordered_cases,
        "multiplicity_classes": patterns,
        "excludes": ["costume/team/control/CPU/rule/detail variants",
                     "seeds and input histories"],
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--fighters", type=int, required=True)
    ap.add_argument("--stages", type=int, required=True)
    args = ap.parse_args()
    try:
        result = count_cases(args.fighters, args.stages)
    except ValueError as exc:
        ap.error(str(exc))
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
