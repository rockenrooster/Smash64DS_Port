#!/usr/bin/env python3
"""Phase 5/7: score candidate link orders against an I-cache conflict model.

The question this exists to answer is the one a whole-match footprint argument
cannot: for the SAME code and the SAME execution sequence, would different
addresses reduce cache conflict?

Model, stated so it can be argued with:

  * A CLUSTER (from `census-call-transitions.py`) is the temporal-locality unit.
    Its members call each other tens to hundreds of times a frame, so their
    lines are live together. This is a proxy for a reuse-distance measurement
    and is labelled as one -- it is the weakest link in the model.
  * A cluster's SELF-CONFLICT is what a link order actually changes. Laid out
    contiguously, a cluster of L lines occupies ceil(L/64) lines per set, evenly.
    Scattered, its lines land at effectively random set indices, and the
    occupancy of the worst sets follows the balls-in-bins distribution.
  * COST = sum over sets of max(0, occupancy - WAYS), weighted by the execution
    count of the lines in that set. Lines beyond the 4 ways must refill on every
    pass; lines within them need not. Round-robin replacement (verified in
    src/CP15.cpp) means the 5th line in a set evicts in fill order, so the excess
    is charged in full rather than discounted for recency.

What the model deliberately does NOT claim: it does not simulate the ARM946E-S,
does not model cross-cluster interference, and does not know the interleaving of
phases within a frame. It is deterministic and explainable, which is what Phase 5
asked for, and its predictions are meant to be checked against the v3
`icache_fill` measurement rather than believed.

Layouts scored:
  current            addresses as linked today
  cluster            each cluster gathered contiguously, clusters by weight
  phase              clusters gathered AND spaced so adjacent clusters start on
                     complementary set offsets
  conflict-min       greedy: place each function at the start offset minimising
                     added weighted set pressure
  falsifier          deterministic shuffle -- must score WORSE, or the model is
                     measuring nothing

UNITS: profile cycles. Two profile cycles = one project tick.
"""

from __future__ import annotations

import argparse
import collections
import csv
import json
import re

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2,8} )+)\s*(\S+)\s*(.*)$")
CALL = re.compile(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2,8} )+\s*bl(?:x)?(?:\.\w)?\s+"
                  r"[0-9a-f]+\s+<([^>+]+)(?:\+[^>]*)?>")

ICACHE_BYTES, LINE, WAYS = 8192, 32, 4
SETS = ICACHE_BYTES // LINE // WAYS
ITCM_LO, ITCM_HI = 0x01FF8000, 0x02000000


def cluster_cost(members, base_of, size_of, weight_of):
    """Weighted set pressure for one cluster at a given placement."""
    per_set: collections.Counter = collections.Counter()
    per_set_w: collections.Counter = collections.Counter()
    for fn in members:
        base, size = base_of[fn], size_of[fn]
        w = weight_of.get(fn, 0) / max(1, (size + LINE - 1) // LINE)
        for off in range(0, size, LINE):
            s = ((base + off) >> 5) & (SETS - 1)
            per_set[s] += 1
            per_set_w[s] += w
    cost = 0.0
    for s, occ in per_set.items():
        if occ > WAYS:
            # the share of this set's weight that cannot stay resident
            cost += per_set_w[s] * (occ - WAYS) / occ
    return cost, (max(per_set.values()) if per_set else 0)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--regions", type=int, required=True)
    ap.add_argument("--min-edge", type=float, default=5.0)
    ap.add_argument("--json", default="")
    args = ap.parse_args()

    owner: dict[int, str] = {}
    start: dict[str, int] = {}
    end: dict[str, int] = {}
    sites: dict[int, tuple[str, str]] = {}
    cur = None
    with open(args.dis, errors="ignore") as handle:
        for raw in handle:
            m = FUNC.match(raw.rstrip("\n"))
            if m:
                cur = m.group(2)
                start.setdefault(cur, int(m.group(1), 16))
                continue
            if cur is None:
                continue
            mi = INSN.match(raw)
            if not mi:
                continue
            pc = int(mi.group(1), 16)
            owner[pc] = cur
            end[cur] = max(end.get(cur, 0),
                           pc + len(mi.group(2).replace(" ", "")) // 2)
            mc = CALL.match(raw)
            if mc:
                sites[pc] = (cur, mc.group(2))

    edge: collections.Counter = collections.Counter()
    weight: collections.Counter = collections.Counter()
    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        next(rows)
        for row in rows:
            try:
                pc = int(row[1], 16)
            except ValueError:
                continue
            n = int(row[4])
            s = sites.get(pc)
            if s is not None:
                edge[s] += n
            fn = owner.get(pc)
            if fn is not None and not (ITCM_LO <= pc < ITCM_HI):
                weight[fn] += n

    parent: dict[str, str] = {}

    def find(x):
        parent.setdefault(x, x)
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for (a, b), n in edge.items():
        if n / args.regions < args.min_edge:
            continue
        if a in weight and b in weight and a in start and b in start:
            ra, rb = find(a), find(b)
            if ra != rb:
                parent[rb] = ra

    groups: dict[str, list] = collections.defaultdict(list)
    for fn in list(parent):
        if fn in start and fn in end:
            groups[find(fn)].append(fn)
    clusters = sorted(groups.values(),
                      key=lambda g: -sum(weight.get(f, 0) for f in g))

    size_of = {f: end[f] - start[f] for g in clusters for f in g}

    def layout_current():
        return {f: start[f] for g in clusters for f in g}

    def layout_gathered(spacing=0):
        base = {}
        cursor = start[clusters[0][0]] & ~(LINE - 1)
        for gi, g in enumerate(clusters):
            ordered = sorted(g, key=lambda f: -weight.get(f, 0))
            if spacing:
                # start each cluster at a complementary set offset
                cursor = (cursor + spacing * gi) & ~3
            for f in ordered:
                base[f] = cursor
                cursor += (size_of[f] + 3) & ~3
        return base

    def layout_conflict_min():
        base = {}
        cursor = start[clusters[0][0]] & ~(LINE - 1)
        for g in clusters:
            ordered = sorted(g, key=lambda f: -weight.get(f, 0))
            placed: list[str] = []
            for f in ordered:
                best, best_cost = cursor, None
                for pad in range(0, 8 * LINE, LINE):
                    cand = cursor + pad
                    base[f] = cand
                    c, _ = cluster_cost(placed + [f], base, size_of, weight)
                    if best_cost is None or c < best_cost:
                        best, best_cost = cand, c
                base[f] = best
                placed.append(f)
                cursor = best + ((size_of[f] + 3) & ~3)
        return base

    def layout_falsifier():
        """Deliberately adversarial: force every hot function to START at the
        same cache set.

        The first falsifier packed functions contiguously and padded by one
        line, which SPREADS set indices -- it was a mildly good layout wearing a
        bad name, and it scored better than the principled candidates. That is
        the failure mode decision-gate item 5 exists to catch, and it caught it.

        This one aligns every function base to a multiple of the 2048-byte set
        period, so each function's first line lands in set 0, its second in set
        1, and so on. Every cluster's hot heads then pile into the low sets. If
        the model cannot tell this apart from a gathered layout, the model is
        measuring nothing and no candidate derived from it may be trusted."""
        base = {}
        period = SETS * LINE                      # 2048
        cursor = start[clusters[0][0]] & ~(period - 1)
        for g in clusters:
            for f in sorted(g, key=lambda x: -weight.get(x, 0)):
                base[f] = cursor
                # advance a whole number of set periods -> same start set again
                span = ((size_of[f] + period - 1) // period) * period
                cursor += span
        return base

    layouts = {
        "current": layout_current(),
        "cluster": layout_gathered(),
        "phase": layout_gathered(spacing=LINE * 7),
        "conflict-min": layout_conflict_min(),
        "falsifier": layout_falsifier(),
    }

    print(f"I-cache {ICACHE_BYTES} B / {LINE} B / {WAYS}-way / {SETS} sets / "
          f"round-robin  [verified: melonDS-Accurate]")
    print(f"clusters {len(clusters)} (edges >= {args.min_edge}/frame), "
          f"functions {len(size_of)}\n")

    base_scores = {}
    for name, base in layouts.items():
        total = 0.0
        rows_out = []
        for g in clusters:
            c, mx = cluster_cost(g, base, size_of, weight)
            total += c
            rows_out.append((sum(weight.get(f, 0) for f in g), c, mx, g))
        base_scores[name] = total
        rel = ""
        if name != "current" and base_scores.get("current"):
            d = total - base_scores["current"]
            rel = f"   delta {d:+,.0f} ({100 * d / base_scores['current']:+.1f}%)"
        print(f"{name:<14} conflict score {total:>14,.0f}{rel}")

    print("\n== per-cluster, current vs best gathered ==")
    print(f"{'weight/fr':>11}{'own B':>8}{'x8K':>6}{'cur cost':>12}"
          f"{'clust cost':>12}{'maxset cur':>11}{'->new':>7}  head")
    for g in clusters[:10]:
        w = sum(weight.get(f, 0) for f in g) / args.regions
        own = sum(size_of[f] for f in g)
        c0, m0 = cluster_cost(g, layouts["current"], size_of, weight)
        c1, m1 = cluster_cost(g, layouts["cluster"], size_of, weight)
        head = max(g, key=lambda f: weight.get(f, 0))
        print(f"{w:>11,.0f}{own:>8,}{own / ICACHE_BYTES:>6.1f}{c0:>12,.0f}"
              f"{c1:>12,.0f}{m0:>11}{m1:>7}  {head[:32]}")

    if args.json:
        with open(args.json, "w") as fh:
            json.dump({"scores": base_scores,
                       "clusters": [[f for f in g] for g in clusters[:20]]},
                      fh, indent=1)
        print(f"\nwrote {args.json}")


if __name__ == "__main__":
    main()
