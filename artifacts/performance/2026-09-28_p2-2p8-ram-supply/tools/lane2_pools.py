#!/usr/bin/env python3
"""Lane 2: pool capacities (configured) vs measured high-water in existing
artifacts.  Read-only over artifacts/performance/2026-09-*.

Configured capacities come from the shipping-like build config and sources:
  builds/build-fp-argmax/nds_build_config.h   NDS_R2_EFFECT_POOL
  src/import/battleship_lbparticle.c:266-273  NDS_R2_PARTICLE_POOL_*
  include/it/item.h:439                       ITEM_ALLOC_MAX
  struct sizes read from the DWARF of the saved shipping-like ELF (see
  lane2_struct_sizes.txt, produced by lane2_struct_sizes.ps1).
Measured counters come from the "extras" array of every *-tickhud.json (the
tick-HUD sampler reads the same globals a probe would).
"""
from __future__ import annotations

import glob
import json
import os
import sys
from collections import defaultdict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..", ".."))

WANT = [
    "gNdsEffectPoolDepth", "gNdsEffectPoolFreeMin",
    "gNdsParticleStructsMax", "gNdsParticleGeneratorsMax",
    "gNdsParticleTransformsMax", "gNdsParticleRejectCount",
    "gNdsWeaponPoolEntries", "gNdsWeaponPoolLiveHighWater",
    "gNdsWeaponPoolRefusalCount", "gNdsGCDrawsActiveMax",
    "gNdsVisualEffectMaxActiveCount", "gNdsVisualEffectCreateCount",
    "gNdsVisualEffectDropCount", "gNdsTask39FxHitSparkSpawnCount",
    "gNdsTask39FxHitSparkDropCount", "gNdsTask39FxHitSparkDrawCount",
    "gNdsItemSpawnLawSpawnCount", "gNdsITCommonDataBytes",
    "gNdsTaskmanGeneralHeapFreeMin", "gNdsTaskmanArenaChosenSize",
]


def main():
    pattern = os.path.join(ROOT, "artifacts", "performance", "2026-09-*", "**", "*tickhud*.json")
    files = sorted(glob.glob(pattern, recursive=True))
    stat = defaultdict(list)
    per_file = {}
    for p in files:
        try:
            d = json.load(open(p, encoding="utf-8"))
        except Exception:
            continue
        ex = d.get("extras") if isinstance(d, dict) else None
        if not isinstance(ex, list):
            continue
        row = {}
        for e in ex:
            if isinstance(e, dict) and e.get("name") in WANT:
                row[e["name"]] = e.get("value")
        if row:
            per_file[os.path.relpath(p, ROOT)] = {"target": d.get("target"), **row}
            for k, v in row.items():
                if isinstance(v, (int, float)):
                    stat[k].append((v, os.path.relpath(p, ROOT)))
    print(f"files with counters: {len(per_file)} of {len(files)} tickhud json")
    summary = {}
    for k in WANT:
        vals = stat.get(k)
        if not vals:
            continue
        lo = min(vals, key=lambda t: t[0])
        hi = max(vals, key=lambda t: t[0])
        srt = sorted(v[0] for v in vals)
        med = srt[len(srt) // 2]
        summary[k] = {"n": len(vals), "min": lo[0], "min_file": lo[1],
                      "max": hi[0], "max_file": hi[1], "median": med}
        print(f"{k:34s} n={len(vals):3d} min={lo[0]:>10} median={med:>10} max={hi[0]:>10}")
    # targets seen
    targets = defaultdict(int)
    for v in per_file.values():
        targets[v.get("target")] += 1
    print("targets:", dict(targets))
    out = {"summary": summary, "per_file": per_file}
    outp = os.path.join(os.path.dirname(__file__), "lane2_pools_measured.json")
    json.dump(out, open(outp, "w", encoding="utf-8"), indent=1)
    print("wrote", outp)


if __name__ == "__main__":
    main()
