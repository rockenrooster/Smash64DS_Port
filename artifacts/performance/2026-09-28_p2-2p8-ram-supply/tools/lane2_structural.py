#!/usr/bin/env python3
"""Lane 2: structural (decode-based) byte classes vs label-based classes.

For each DObj-style file the display lists are decoded from every Gfx root
(Gfx-typed labels, DObjDLLink targets, native-owner ROOT constants) with
lane2_dl.Decode; the union of decoded command bytes, G_VTX arrays and
LOAD-sized textures / palettes are the STRUCTURAL class totals.  They are
lower bounds (a range no DL loads is not counted) and independent of label
names, so they validate or contradict the label partition.
"""
import json
import re
import struct
from collections import defaultdict
from pathlib import Path

import lane2_o2r as o2r
import lane2_dl as dl
import lane2_partition as part
import lane2_items as items
import lane2_items_offsets as lio

HERE = Path(__file__).resolve().parent


def roots_from_partition(f, rows):
    roots = set()
    for a, used, cls, lab, pad in rows:
        if cls == "display_list":
            t = part.LAST_TYPES.get(lab)
            if t == "DObjDLLink" or "DLLink" in lab:
                # entries {list_id, Gfx*}: targets via internal slots at +4
                for off in range(a + 4, a + max(used, 8), 8):
                    tgt = f.internal.get(off)
                    if tgt is not None:
                        roots.add(tgt)
            else:
                roots.add(a)
    return roots


def run_ef():
    out = {}
    for stem, name, pre, ll in (("83_EFCommonEffects1", "EFCommonEffects1", "dEFCommonEffects1", "EFCommonEffects1"),
                                ("84_EFCommonEffects2", "EFCommonEffects2", "dEFCommonEffects2", "EFCommonEffects2"),
                                ("85_EFCommonEffects3", "EFCommonEffects3", "dEFCommonEffects3", "EFCommonEffects3")):
        f, totals, rows, offsets, unpl = part.run(stem, name, pre, ll, verbose=False)
        d = dl.Decode(f)
        for r in sorted(roots_from_partition(f, rows)):
            d.walk(r)
        rg = d.ranges()
        s = {k: dl.union_len(v) for k, v in rg.items()}
        out[name] = {"structural": s, "label": {k: totals.get(k, 0) for k in
                     ("display_list", "vertices", "texture", "palette")},
                     "roots": len(roots_from_partition(f, rows)), "errors": d.errors[:5],
                     "segrefs": d.segrefs}
        print(name, "structural", s, "label", out[name]["label"], "errors", len(d.errors))
    return out


def run_86():
    r = lio.placement()
    f = o2r.by_name("MiscData086")
    offsets, decls = r["offsets"], r["decls"]
    roots = {off for lab, off in offsets.items() if decls.get(lab) == "Gfx"}
    for lab, off in offsets.items():
        if "dllink" in lab.lower() or decls.get(lab) == "DObjDLLink":
            for o in range(off + 4, off + 32, 8):
                t = f.internal.get(o)
                if t is not None:
                    roots.add(t)
    ow = items.owners()
    for k, v in ow.items():
        if v["asset"] == 86:
            for n, x in v["consts"].items():
                if "ROOT" in n:
                    roots.add(x)
    # DObjDesc.dl targets for every kind
    for kind, fields in items.parse_251_externs().items():
        if "data" in fields:
            for (_, ts) in items.dobjdesc_roots(f, fields["data"]):
                roots.update(ts)
    d = dl.Decode(f)
    for x in sorted(roots):
        d.walk(x)
    rg = d.ranges()
    s = {k: dl.union_len(v) for k, v in rg.items()}
    print("MiscData086 structural", s, "roots", len(roots), "errors", len(d.errors), d.errors[:3])
    return {"structural": s, "roots": len(roots), "errors": d.errors[:5], "segrefs": d.segrefs}


def main():
    res = {"ef": run_ef(), "m86": run_86()}
    (HERE / "lane2_structural.json").write_text(json.dumps(res, indent=1))


if __name__ == "__main__":
    main()
