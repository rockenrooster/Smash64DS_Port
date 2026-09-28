#!/usr/bin/env python3
"""Lane 1: validate the typed-source tiling against the O2R payloads (prints the numbers the report cites)."""
import bisect, collections, json
from pathlib import Path
import lane1_o2r as o, lane1_typed as ty, lane1_tiling as tl

PTR_OK = {'Gfx','DObjDesc','MObjSub','DObjDLLink','MPGeometryData','MPGroundData','Sprite','Bitmap','u32','GRSectorDesc','SYInterpDesc','ALIGN','PAD'}
idx = o.index()
ids = [103,104,105,106,107,108,109,110,111,112,113,152,153,154,155,156,157,158,159,160,161]
rows = []
tot_slots = tot_bad = tot_decl = 0
for fid in ids:
    decls, probs = ty.parse_file(fid)
    f = idx[fid]
    end = decls[-1].offset + decls[-1].size
    aligned_end = (end + 15) & ~15
    slots = list(f.internal) + list(f.external)
    starts = [d.offset for d in decls]
    bad = 0
    for s in slots:
        i = bisect.bisect_right(starts, s) - 1
        d = decls[i]
        if not (d.offset <= s < d.offset + d.size) or not (d.ctype in PTR_OK or d.ctype.endswith('*')):
            bad += 1
    rows.append(dict(fid=fid, payload=f.data_size, typed_end=end, exact=(aligned_end == f.data_size),
                     decls=len(decls), slots=len(slots), bad_slots=bad, problems=len(probs)))
    tot_slots += len(slots); tot_bad += bad; tot_decl += len(decls)
ok = sum(1 for r in rows if r['exact'])
print(f"typed files: {len(rows)}; tile to payload size (16-byte tail alignment): {ok}/{len(rows)}; declarations: {tot_decl}; "
      f"relocation slots checked: {tot_slots}; slots outside pointer-bearing fields: {tot_bad}")
for r in rows:
    print(r)
Path(__file__).resolve().parents[1].joinpath("lane1_tiling_validation.json").write_text(json.dumps(rows, indent=1))
