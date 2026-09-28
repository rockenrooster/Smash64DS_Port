#!/usr/bin/env python3
"""Lane 2: what an UNMODIFIED item-owner admission needs kept.

The item native owners (src/port/renderer_adapter_stage.c, ~8790-10700) test
`dl[i].words.w0/w1` at fixed word indices against generated constants, so the
display list must stay contiguous from its root up to the highest compared
index.  lane2_estimate.py charges only the compared words (8 B per (root,
index)), which is what a converted, root-identity admission would still keep at
most.  This script prices the other variant: keep the whole prefix
[root, root + 8 * (max_index + 1)) clipped to the list's own length.

Reads lane2_items_model.json (owners: roots + compared dl indices) and decodes
the display-list lengths from MiscData086 with lane2_dl.
"""
from __future__ import annotations

import json
from pathlib import Path

import lane2_dl as dl
import lane2_o2r as o2r

HERE = Path(__file__).resolve().parent


def union_len(ranges):
    tot, cur = 0, None
    for a, b in sorted(ranges):
        if cur is None or a > cur[1]:
            if cur is not None:
                tot += cur[1] - cur[0]
            cur = [a, b]
        else:
            cur[1] = max(cur[1], b)
    if cur is not None:
        tot += cur[1] - cur[0]
    return tot


def main():
    m = json.loads((HERE / "lane2_items_model.json").read_text())
    f = o2r.by_name("MiscData086")
    fp_words, prefix_ranges, per_owner = [], [], {}
    for owner, row in sorted(m["owners"].items()):
        roots = [int(r, 16) for r in row["roots"]]
        idx = [int(i, 16) for i in row["dl_indices"]]
        if not idx:
            per_owner[owner] = {"roots": len(roots), "max_index": None, "prefix_bytes": 0}
            continue
        d = dl.Decode(f)
        for r in roots:
            d.walk(r)
        mx = max(idx)
        tot = []
        for r in roots:
            length = d.dl.get(r, 8 * (mx + 1))
            end = r + min(8 * (mx + 1), length)
            tot.append((r, end))
            prefix_ranges.append((r, end))
            for i in idx:
                if 8 * i < length:
                    fp_words.append((r + 8 * i, r + 8 * i + 8))
        per_owner[owner] = {"roots": len(roots), "max_index": mx,
                            "prefix_bytes": union_len(tot)}
    out = {"compared_words_bytes_union": union_len(fp_words),
           "contiguous_prefix_bytes_union": union_len(prefix_ranges),
           "per_owner": per_owner}
    (HERE / "lane2_prefix.json").write_text(json.dumps(out, indent=1))
    print("compared DL words (8 B each, union):", out["compared_words_bytes_union"])
    print("contiguous prefix to max compared index (union):", out["contiguous_prefix_bytes_union"])
    for k, v in per_owner.items():
        print(f"  {k:16s} roots={v['roots']} max_index={v['max_index']} prefix={v['prefix_bytes']}")


if __name__ == "__main__":
    main()
