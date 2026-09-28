#!/usr/bin/env python3
"""Lane 2: F3DEX2 display-list decode over O2R payloads (structural typing).

Walks Gfx from a set of root offsets and records, as byte ranges of the
payload: the commands themselves (display_list), G_VTX target arrays
(vertices), G_SETTIMG targets sized by the following LOADBLOCK / LOADTILE /
LOADTLUT (texture / palette).  Pointer words are recovered from the O2R
internal-fixup table (slot -> target offset); segment-relative words (0x0E......,
material hooks) are recorded but not followed.

Opcodes (F3DEX2, include/nds/nds_gbi_decode.h and the port's own constants):
  0x01 G_VTX  (count = (w0 >> 12) & 0xFF)     0xDE G_DL (bit 16 of w0 = branch)
  0xDF G_ENDDL   0xFD G_SETTIMG   0xF3 G_LOADBLOCK   0xF4 G_LOADTILE
  0xF0 G_LOADTLUT   0xF2 G_SETTILESIZE   0xF5 G_SETTILE
"""
from __future__ import annotations

import struct
from collections import defaultdict

import lane2_o2r as o2r

SIZ_BITS = {0: 4, 1: 8, 2: 16, 3: 32}


class Decode:
    def __init__(self, f: o2r.O2R):
        self.f = f
        self.dl = {}       # offset -> bytes of commands (start -> end)
        self.vtx = {}      # start -> length
        self.tex = {}      # start -> (length, fmt, siz)
        self.pal = {}      # start -> length
        self.segrefs = 0
        self.errors = []
        self.visited = set()

    def ptr(self, slot):
        return self.f.internal.get(slot)

    def walk(self, root, depth=0):
        if root in self.visited or root is None or root < 0 or root + 8 > self.f.data_size:
            return
        self.visited.add(root)
        f = self.f
        pos = root
        timg = None  # (ptr, fmt, siz)
        while pos + 8 <= f.data_size:
            w0 = struct.unpack_from(">I", f.payload, pos)[0]
            w1 = struct.unpack_from(">I", f.payload, pos + 4)[0]
            op = w0 >> 24
            if op == 0xDF:
                self.dl[root] = pos + 8 - root
                return
            if op == 0x01:
                n = (w0 >> 12) & 0xFF
                t = self.ptr(pos + 4)
                if t is not None and 0 < n <= 64:
                    self.vtx[t] = max(self.vtx.get(t, 0), n * 16)
                elif t is None:
                    self.segrefs += 1
            elif op == 0xDE:
                t = self.ptr(pos + 4)
                if t is not None:
                    self.walk(t, depth + 1)
                else:
                    self.segrefs += 1
                if (w0 >> 16) & 0xFF:  # G_DL branch flag (no push): list ends here
                    self.dl[root] = pos + 8 - root
                    return
            elif op == 0xFD:
                fmt, siz = (w0 >> 21) & 7, (w0 >> 19) & 3
                t = self.ptr(pos + 4)
                timg = (t, fmt, siz) if t is not None else None
            elif op == 0xF3 and timg is not None:  # LOADBLOCK
                lrs = (w1 >> 12) & 0xFFF
                nbytes = ((lrs + 1) * SIZ_BITS[timg[2]] + 7) // 8
                t, fmt, siz = timg
                self.tex[t] = (max(self.tex.get(t, (0,))[0], nbytes), fmt, siz)
            elif op == 0xF4 and timg is not None:  # LOADTILE
                lrs = (w1 >> 12) & 0xFFF
                lrt = w1 & 0xFFF
                width = (lrs >> 2) + 1
                height = (lrt >> 2) + 1
                nbytes = (width * height * SIZ_BITS[timg[2]] + 7) // 8
                t, fmt, siz = timg
                self.tex[t] = (max(self.tex.get(t, (0,))[0], nbytes), fmt, siz)
            elif op == 0xF0 and timg is not None:  # LOADTLUT
                count = ((w1 >> 14) & 0x3FF) + 1
                t = timg[0]
                self.pal[t] = max(self.pal.get(t, 0), count * 2)
            pos += 8
        self.errors.append(f"unterminated DL at {root:#x}")

    def ranges(self):
        r = {"display_list": [], "vertices": [], "texture": [], "palette": []}
        for a, n in self.dl.items():
            r["display_list"].append((a, a + n))
        for a, n in self.vtx.items():
            r["vertices"].append((a, a + n))
        for a, (n, fmt, siz) in self.tex.items():
            r["texture"].append((a, a + n))
        for a, n in self.pal.items():
            r["palette"].append((a, a + n))
        return r


def union_len(ranges):
    tot = 0
    cur_a = cur_b = None
    for a, b in sorted(ranges):
        if cur_b is None or a > cur_b:
            if cur_b is not None:
                tot += cur_b - cur_a
            cur_a, cur_b = a, b
        else:
            cur_b = max(cur_b, b)
    if cur_b is not None:
        tot += cur_b - cur_a
    return tot
