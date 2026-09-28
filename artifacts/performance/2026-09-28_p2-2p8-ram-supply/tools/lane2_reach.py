#!/usr/bin/env python3
"""Lane 2: reachability of EFCommonEffects1/2/3 bytes from the source roots.

Roots are exactly the `llEFCommonEffects<N>*` symbols the decomp source uses
(63 names; 59 have offsets in src/import/battleship_efmanager_symbols.h, the
FileIDs are not offsets, and llEFCommonEffects2ShadowTextureImage is the label
dEFCommonEffects2_Shadow_TextureImage).  The pointer graph is the O2R internal
fixup table (ground truth), projected onto the interval partition produced by
lane2_partition.py.  Reachability is interval-level, therefore an UPPER bound
on live bytes (an interval reached by one pointer counts whole).

Outputs per file: live vs unreferenced bytes by class, and per-effect closure
bytes by class.  Dead here means "no source root leads to it": not read by
any decomp path.  It says nothing yet about DS reads of the live part.
"""
from __future__ import annotations

import json
import re
import sys
from collections import defaultdict

import lane2_o2r as o2r
import lane2_partition as part

EF = [
    ("83_EFCommonEffects1", "EFCommonEffects1", "dEFCommonEffects1", "EFCommonEffects1", 1),
    ("84_EFCommonEffects2", "EFCommonEffects2", "dEFCommonEffects2", "EFCommonEffects2", 2),
    ("85_EFCommonEffects3", "EFCommonEffects3", "dEFCommonEffects3", "EFCommonEffects3", 3),
]

STRUCT_KINDS = ("DObjDesc", "MObjSub", "AnimJoint", "MatAnimJoint")


def effect_roots(n: int):
    """{effect_name: {kind: offset}} for EFCommonEffects<n>."""
    roots = part.ll_roots(f"EFCommonEffects{n}")
    out = defaultdict(dict)
    for name, off in roots.items():
        m = re.match(rf"EFCommonEffects{n}(.+?)(DObjDesc|MObjSub|MatAnimJoint|AnimJoint)$", name)
        if m:
            out[m.group(1)][m.group(2)] = off
    return out


def analyse(stem, name, prefix, ll, n, verbose=True):
    f, totals, rows, offsets, unplaced = part.run(stem, name, prefix, ll, verbose=False)
    # interval index over rows (each row = interval start a, extent from next start)
    starts = [r[0] for r in rows]
    ends = starts[1:] + [f.data_size]
    import bisect

    def iv(off):
        i = bisect.bisect_right(starts, off) - 1
        return max(i, 0)

    # edges: interval -> intervals
    adj = defaultdict(set)
    for slot, tgt in f.internal.items():
        adj[iv(slot)].add(iv(tgt))
    eff = effect_roots(n)
    all_roots = {}
    for e, kinds in eff.items():
        for k, off in kinds.items():
            all_roots[(e, k)] = off
    extra = {}
    if n == 2:
        for lab, off in offsets.items():
            if lab.endswith("Shadow_TextureImage"):
                extra[("ShadowTexture", "Image")] = off

    def closure(root_offs):
        seen = set()
        stack = [iv(o) for o in root_offs]
        while stack:
            i = stack.pop()
            if i in seen:
                continue
            seen.add(i)
            stack.extend(adj.get(i, ()))
        return seen

    def bytes_by_class(ivset):
        t = defaultdict(int)
        for i in ivset:
            a, used, cls, lab, pad = rows[i]
            t[cls] += used
            if pad:
                t["pad"] += pad
        return dict(t)

    live = closure(list(all_roots.values()) + list(extra.values()))
    dead = set(range(len(rows))) - live
    per_effect = {}
    for e, kinds in eff.items():
        per_effect[e] = bytes_by_class(closure(list(kinds.values())))
    if extra:
        per_effect["ShadowTexture"] = bytes_by_class(closure(list(extra.values())))
    res = {"file": name, "fid": f.file_id, "payload": f.data_size,
           "all_classes": totals,
           "live": bytes_by_class(live), "unreferenced": bytes_by_class(dead),
           "per_effect": per_effect,
           "dead_intervals": [(hex(rows[i][0]), rows[i][1] + rows[i][4], rows[i][2], rows[i][3])
                              for i in sorted(dead) if rows[i][1] + rows[i][4] >= 64]}
    if verbose:
        print(f"== {name} payload={f.data_size}")
        print("  all       ", dict(sorted(totals.items())))
        print("  live      ", dict(sorted(res['live'].items())), "sum", sum(res['live'].values()))
        print("  unreferenced", dict(sorted(res['unreferenced'].items())), "sum", sum(res['unreferenced'].values()))
        for e, t in sorted(per_effect.items()):
            print(f"   {e:18s} {sum(t.values()):6d} {dict(sorted(t.items()))}")
        print("  large dead intervals:")
        for d in res["dead_intervals"][:40]:
            print("    ", d)
    return res


if __name__ == "__main__":
    out = {}
    for row in EF:
        out[row[1]] = analyse(*row)
    outp = "lane2_reach.json"
    json.dump(out, open(outp, "w"), indent=1)
    print("wrote", outp)
