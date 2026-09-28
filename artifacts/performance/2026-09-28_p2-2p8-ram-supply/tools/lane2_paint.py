#!/usr/bin/env python3
"""Lane 2: byte-level class map = structural decode first, labels second.

Every payload byte gets exactly one class.  Priority (first paint wins):
  1 dllink        DObjDLLink arrays (typed labels)
  2 display_list  decoded Gfx command bytes (lane2_dl)
  3 vertices      G_VTX-referenced arrays
  4 texture/palette  LOAD-sized G_SETTIMG targets
  5 label classes (declared-size trimmed): texture palette vertices display_list
                  material animation dobjdesc other pad
  6 unclassified  bytes in no decoded range and no typed label
Structural evidence overrides label names (labels lump trailing bytes into the
last interval of a block).  Unclassified bytes are treated as KEEP downstream
(conservative).
"""
from __future__ import annotations

import struct
from collections import defaultdict

import lane2_o2r as o2r
import lane2_dl as dl

PRIORITY = ["dllink", "display_list", "vertices", "texture", "palette"]


def paint(f, label_rows, decode: dl.Decode, dllink_ranges=()):
    n = f.data_size
    cls = [None] * n

    def fill(a, b, c, over=False):
        for i in range(max(a, 0), min(b, n)):
            if cls[i] is None or over:
                cls[i] = c

    for a, b in dllink_ranges:
        fill(a, b, "dllink")
    rg = decode.ranges()
    for c in ("display_list", "vertices", "texture", "palette"):
        for a, b in rg[c]:
            fill(a, b, c)
    for a, used, c, lab in label_rows:
        fill(a, a + used, c)
    tot = defaultdict(int)
    for c in cls:
        tot[c or "unclassified"] += 1
    return cls, dict(tot)
