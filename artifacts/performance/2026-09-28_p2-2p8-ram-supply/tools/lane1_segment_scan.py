#!/usr/bin/env python3
"""Lane 1: unrelocated pointer words in stage display lists (segment addresses).

Pointer-bearing F3DEX2 commands (G_VTX, G_SETTIMG, G_DL, G_MOVEMEM, G_MTX) normally carry an O2R relocation
slot in w1.  A w1 with no slot is a raw (segment) address.  This scans every Gfx declaration of the 21 typed
stage files and reports what the unrelocated ones are.
"""
import collections, json, struct
from pathlib import Path
import lane1_o2r as o, lane1_tiling as tl

idx = o.index()
ids = [103,104,105,106,107,108,109,110,111,112,113,152,153,154,155,156,157,158,159,160,161]
by_op = collections.Counter(); by_seg = collections.Counter(); per_file = {}
for fid in ids:
    f = idx[fid]; p = f.payload
    slots = set(f.internal) | set(f.external)
    blocks, _ = tl.tile_typed(fid)
    n = 0
    for b in blocks:
        if b.cls != 'gfx':
            continue
        for off in range(b.off, b.end, 8):
            w0, w1 = struct.unpack_from('>II', p, off)
            op = w0 >> 24
            if op in (0x01, 0xFD, 0xDE, 0xDC, 0xDA) and (off + 4) not in slots:
                by_op[hex(op)] += 1; by_seg[hex(w1 >> 24)] += 1; n += 1
    if n: per_file[fid] = n
res = dict(total=sum(by_op.values()), by_opcode=dict(by_op), by_w1_high_byte=dict(by_seg), per_file=per_file)
Path(__file__).resolve().parents[1].joinpath('lane1_segment_scan.json').write_text(json.dumps(res, indent=1))
print(res)
