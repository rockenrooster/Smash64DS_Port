#!/usr/bin/env python3
"""Lane 2: how many display-list ROOTS each effect file has that a DObj can point
at, and how many of them a native owner admits.

Why it matters for the estimate.  When display-list bytes are cut, every DObj
whose `dl` names one must still see a non-NULL, recognisable address: the draw
pass calls the native owners only for a DObj with a non-NULL `dl`
(src/port/renderer_adapter_stage.c:6154-6231), and a NULL `dl` would also hide
the NO_PROGRAM native-failure record that owner-less effects produce today.  The
fighter packs solve this with one 8-byte identity cell {G_ENDDL, source root
offset} per root, stored in a dense table (include/nds/nds_preview_pack.h:49-59,
src/port/reloc_preview_pack.c:150-163).  So the estimate charges 8 B per
REACHABLE root (roots reachable from the 59 decomp-used ll roots, by pointer
closure over the O2R internal fixups) in addition to the span rows.

A root here = a distinct intra-file pointer target that lands inside a Gfx
interval (label-start or interior: a label can lump several back-to-back lists)
and whose pointer slot is NOT inside a display list's own body (i.e. it lives in
a DObjDesc / DObjDLLink / other data structure).
"""
from __future__ import annotations

import bisect
import json
from collections import defaultdict
from pathlib import Path

import lane2_o2r as o2r
import lane2_partition as part
import lane2_reach as reach

HERE = Path(__file__).resolve().parent

# roots named by native-owner / entry-effect admission tests (report section 3)
ADMITTED = {
    1: {"DamageSlash": [0x75A0, 0x7668]},
    2: {"CatchSwirl": [0x2500, 0x2588, 0x2610, 0x2698],
        "DeadExplodeDefault": [0x5218, 0x52B0, 0x5310],
        "ReflectBreak": [0x31D0, 0x3258, 0x32E0]},
    3: {"MBallRays": [0x0440, 0x0518],
        "ItemGetSwirl": [0x2EF0, 0x2F80, 0x3010, 0x30A0],
        "RebirthHalo": [0x2378, 0x27E8, 0x2A88]},
}


def main():
    out = {}
    for stem, name, prefix, ll, n in reach.EF:
        f, totals, rows, offsets, unplaced = part.run(stem, name, prefix, ll, verbose=False)
        types = dict(part.LAST_TYPES)
        starts = [r[0] for r in rows]

        def iv(off):
            return max(bisect.bisect_right(starts, off) - 1, 0)

        def is_link(r):
            return ("DLLink" in r[3]) or (types.get(r[3]) == "DObjDLLink")

        adj = defaultdict(set)
        for slot, tgt in f.internal.items():
            adj[iv(slot)].add(iv(tgt))
        eff = reach.effect_roots(n)
        seen, stack = set(), [iv(o) for kinds in eff.values() for o in kinds.values()]
        while stack:
            i = stack.pop()
            if i in seen:
                continue
            seen.add(i)
            stack.extend(adj.get(i, ()))
        roots_all, roots_reach = set(), set()
        for slot, tgt in f.internal.items():
            r = rows[iv(slot)]
            if r[2] == "display_list" and not is_link(r):
                continue                      # pointer inside a DL body (G_DL / G_VTX / SETTIMG)
            tr = rows[iv(tgt)]
            if tr[2] == "display_list" and not is_link(tr):
                roots_all.add(tgt)
                if iv(slot) in seen:
                    roots_reach.add(tgt)
        adm = sorted({o for v in ADMITTED.get(n, {}).values() for o in v})
        out[name] = {"dl_roots_all": len(roots_all), "dl_roots_reachable": len(roots_reach),
                     "admitted_roots": len(adm),
                     "admitted_not_in_reachable_roots": [hex(o) for o in adm if o not in roots_reach]}
        print(f"{name:18s} roots(all)={len(roots_all):3d} reachable={len(roots_reach):3d} "
              f"admitted={len(adm):2d} admitted-not-found={out[name]['admitted_not_in_reachable_roots']}")
    (HERE / "lane2_roots.json").write_text(json.dumps(out, indent=1))


if __name__ == "__main__":
    main()
