#!/usr/bin/env python3
"""Lane 1: classify every byte of each resident stage file (reachability walk).

Method (see the report): start from the roots the source code uses, follow
pointers through the O2R relocation tables (internal + external fixups), decode
F3DEX2 display lists to find Vtx / texture / palette / light / matrix spans, and
parse the typed structures whose layout the decomp defines (MPGroundData,
DObjDesc, MPGeometryData, MObjSub, Sprite/Bitmap).  Bytes not reached are
"unreached" (zero-filled vs non-zero), never "free".

Classes
  hdr      MPGroundData header + item weights (map file)
  dobjdesc DObjDesc arrays (44-byte entries, sentinel id==18 included)
  dltab    DObjDLLink tables (8-byte entries)
  gfx      F3DEX2 display-list commands
  vtx      Vtx arrays (G_VTX targets)
  tex      texel images (G_SETTIMG + LOADBLOCK/LOADTILE, MObjSub sprite arrays)
  pal      TLUT palettes (G_LOADTLUT, MObjSub palette arrays)
  lmv      Light / Mtx / Vp data referenced from G_MOVEMEM / G_MTX
  coll     map collision geometry (MPGeometryData + its arrays)
  mobj     MObjSub structs and their pointer lists / sprite pointer arrays
  anim     AObj event32 scripts, AnimJoint lists, anim pointer tables
  sprite   Sprite record + Bitmap table (wallpaper container)
  pixels   Bitmap pixel strips (wallpaper container)
  ptrtab   other pointer tables reached as roots
  unk0     unreached, all-zero bytes (probable padding)
  unk      unreached, non-zero bytes (unknown)
"""
from __future__ import annotations

import re
import struct
import sys
from collections import defaultdict
from dataclasses import dataclass, field

import lane1_o2r as o

CLASSES = ["hdr", "dobjdesc", "dltab", "gfx", "vtx", "tex", "pal", "lmv", "coll",
           "mobj", "anim", "sprite", "pixels", "ptrtab", "unk0", "unk"]
CI = {c: i + 1 for i, c in enumerate(CLASSES)}   # 0 = unassigned
CN = {v: k for k, v in CI.items()}

OPS_VALID = set(range(0x00, 0x09)) | {0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
                                       0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB,
                                       0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
                                       0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF}


def be32(b: bytes, off: int) -> int:
    return struct.unpack_from(">I", b, off)[0]


def bits_of(siz: int) -> int:
    return 4 << siz  # 0:4b 1:8b 2:16b 3:32b


class FileLayout:
    def __init__(self, f: o.OFile):
        self.f = f
        n = len(f.payload)
        self.n = n
        self.cls = bytearray(n)
        self.label = {}          # start offset -> label (first writer)
        self.conflicts = 0       # bytes claimed by a second, different class
        self.conflict_detail = defaultdict(int)
        self.starts = set()      # object starts (pointer targets) for gap bounding
        self.objs = []           # (start, end, cls, label)

    def mark(self, start: int, end: int, cls: str, label: str = "") -> None:
        start = max(0, start)
        end = min(self.n, end)
        if end <= start:
            return
        c = CI[cls]
        self.objs.append((start, end, cls, label))
        seg = self.cls[start:end]
        for i in range(start, end):
            cur = self.cls[i]
            if cur == 0:
                self.cls[i] = c
            elif cur != c:
                self.conflicts += 1
                self.conflict_detail[(CN[cur], cls)] += 1
        if label and start not in self.label:
            self.label[start] = label

    def covered(self, off: int) -> bool:
        return self.cls[off] != 0

    def finish(self) -> None:
        # unreached: zero vs non-zero
        p = self.f.payload
        for i in range(self.n):
            if self.cls[i] == 0:
                self.cls[i] = CI["unk0"] if p[i] == 0 else CI["unk"]

    def counts(self) -> dict:
        d = defaultdict(int)
        for b in self.cls:
            d[CN[b]] += 1
        return dict(d)

    def unreached_ranges(self, min_len=1):
        out = []
        i = 0
        n = self.n
        while i < n:
            if self.cls[i] in (CI["unk0"], CI["unk"]):
                j = i
                nz = 0
                while j < n and self.cls[j] in (CI["unk0"], CI["unk"]):
                    if self.cls[j] == CI["unk"]:
                        nz += 1
                    j += 1
                if j - i >= min_len:
                    out.append((i, j, nz))
                i = j
            else:
                i += 1
        return out


class Walker:
    def __init__(self, tree_files):
        self.files = {f.file_id: f for f in tree_files}
        self.lay = {fid: FileLayout(f) for fid, f in self.files.items()}
        # slot -> (target_file, target_off) per file
        self.slots = {}
        for fid, f in self.files.items():
            m = {}
            for s, r in f.internal.items():
                m[s] = (r.asset_id, r.offset)
            for s, r in f.external.items():
                m[s] = (r.asset_id, r.offset)
            self.slots[fid] = m
            # every pointer target is an object start in its own file
        for fid, m in self.slots.items():
            for s, (tf, to) in m.items():
                if tf in self.lay:
                    self.lay[tf].starts.add(to)
        self.seen_dl = set()      # (file, off, timg) already decoded
        self.seen_root = set()
        self.notes = []
        self.dl_roots = defaultdict(set)     # file -> set(root offsets) reached as DL roots
        self.dl_tag = {}                     # (file, off) -> tag string of first root that reached it
        self.tex_spans = []                  # (file, start, end, kind)
        self.reach_by_tag = defaultdict(lambda: defaultdict(list))  # tag -> class -> [(file,start,end)]

    # ------------------------------------------------------------------ helpers
    def ptr(self, fid: int, slot: int):
        return self.slots[fid].get(slot)

    def next_start(self, fid: int, off: int) -> int:
        lay = self.lay[fid]
        nxt = lay.n
        for s in lay.starts:
            if off < s < nxt:
                nxt = s
        return nxt

    def mark(self, fid, start, end, cls, label="", tag=None):
        self.lay[fid].mark(start, end, cls, label)
        if tag is not None:
            self.reach_by_tag[tag][cls].append((fid, start, min(end, self.lay[fid].n)))

    # ------------------------------------------------------------------ display lists
    def decode_dl(self, fid: int, off: int, timg=None, tag=None, depth=0, label="dl"):
        """Stateful F3DEX2 decode. timg = current SETTIMG (file, off, fmt, siz, width)
        carried into sub-lists exactly like RDP state.  Returns final timg."""
        f = self.files[fid]
        p = f.payload
        lay = self.lay[fid]
        key = (fid, off, timg)
        if key in self.seen_dl or depth > 64:
            return timg
        self.seen_dl.add(key)
        self.dl_roots[fid].add(off)
        if (fid, off) not in self.dl_tag:
            self.dl_tag[(fid, off)] = tag
        cur = off
        start = off
        while cur + 8 <= lay.n:
            w0 = be32(p, cur)
            w1 = be32(p, cur + 4)
            op = w0 >> 24
            if op not in OPS_VALID:
                self.notes.append(f"file {fid}: DL@0x{off:x} stops at 0x{cur:x} op=0x{op:02x}")
                break
            # a pointer-target boundary in the middle of a fall-through fragment (no ENDDL) ends us
            if cur != off and cur in lay.starts and lay.cls[cur] == CI["gfx"] and op != 0xDF:
                pass
            slot = self.ptr(fid, cur + 4)
            if op == 0x01 and slot is not None:      # G_VTX
                n = (w0 >> 12) & 0xFF
                tf, to = slot
                if tf in self.lay:
                    self.mark(tf, to, to + 16 * n, "vtx", f"vtx@{fid}:0x{cur:x}", tag)
            elif op == 0xDE:                          # G_DL / branch
                if slot is not None:
                    tf, to = slot
                    if tf in self.lay:
                        timg = self.decode_dl(tf, to, timg, tag, depth + 1, "dl")
                if (w0 >> 16) & 0xFF:                 # branch: never returns
                    cur += 8
                    self.mark(fid, start, cur, "gfx", f"{label}@0x{start:x}", tag)
                    return timg
            elif op == 0xFD:                          # G_SETTIMG
                if slot is not None:
                    fmt = (w0 >> 21) & 7
                    siz = (w0 >> 19) & 3
                    wid = (w0 & 0xFFF) + 1
                    timg = (slot[0], slot[1], fmt, siz, wid)
                else:
                    timg = None
            elif op == 0xF3 and timg is not None:     # G_LOADBLOCK
                texels = ((w1 >> 12) & 0xFFF) + 1
                bytes_ = (texels * bits_of(timg[3]) + 7) // 8
                if timg[0] in self.lay:
                    self.mark(timg[0], timg[1], timg[1] + bytes_, "tex", f"tex@{timg[0]}:0x{timg[1]:x}", tag)
            elif op == 0xF4 and timg is not None:     # G_LOADTILE
                uls, ult = (w0 >> 12) & 0xFFF, w0 & 0xFFF
                lrs, lrt = (w1 >> 12) & 0xFFF, w1 & 0xFFF
                cols = ((lrs - uls) >> 2) + 1
                rows = ((lrt - ult) >> 2) + 1
                rowbytes = (timg[4] * bits_of(timg[3]) + 7) // 8
                bytes_ = (rows - 1) * rowbytes + (cols * bits_of(timg[3]) + 7) // 8
                if timg[0] in self.lay:
                    self.mark(timg[0], timg[1], timg[1] + bytes_, "tex", f"tex@{timg[0]}:0x{timg[1]:x}", tag)
            elif op == 0xF0 and timg is not None:     # G_LOADTLUT
                count = ((w1 >> 14) & 0x3FF) + 1
                if timg[0] in self.lay:
                    self.mark(timg[0], timg[1], timg[1] + 2 * count, "pal", f"pal@{timg[0]}:0x{timg[1]:x}", tag)
            elif op == 0xDC and slot is not None:     # G_MOVEMEM
                nb = (((w0 >> 19) & 0x1F) + 1) * 8
                tf, to = slot
                if tf in self.lay:
                    self.mark(tf, to, to + nb, "lmv", f"movemem@{tf}:0x{to:x}", tag)
            elif op == 0xDA and slot is not None:     # G_MTX
                tf, to = slot
                if tf in self.lay:
                    self.mark(tf, to, to + 64, "lmv", f"mtx@{tf}:0x{to:x}", tag)
            cur += 8
            if op == 0xDF:                            # G_ENDDL
                break
        self.mark(fid, start, cur, "gfx", f"{label}@0x{start:x}", tag)
        return timg

    def dl_or_link(self, fid: int, off: int, tag=None):
        """A DObjDesc.dl target: DObjDLLink table or a plain DL."""
        f = self.files[fid]
        p = f.payload
        lay = self.lay[fid]
        if off + 8 > lay.n:
            return
        w0 = be32(p, off)
        is_link = (w0 <= 4) and (self.ptr(fid, off + 4) is not None or w0 == 4)
        if is_link and w0 != 0xFFFFFFFF:
            cur = off
            while cur + 8 <= lay.n:
                lid = be32(p, cur)
                sl = self.ptr(fid, cur + 4)
                if lid == 4:
                    cur += 8
                    break
                if lid > 4 or sl is None:
                    break
                tf, to = sl
                if tf in self.lay:
                    self.decode_dl(tf, to, None, tag, 0, "dl")
                cur += 8
            self.mark(fid, off, cur, "dltab", f"dllinks@0x{off:x}", tag)
        else:
            self.decode_dl(fid, off, None, tag, 0, "dl")

    # ------------------------------------------------------------------ typed parsers
    def dobjdesc_array(self, fid: int, off: int, tag=None, follow_dl=True):
        f = self.files[fid]
        p = f.payload
        lay = self.lay[fid]
        if (fid, off, "dobjdesc") in self.seen_root:
            return 0
        self.seen_root.add((fid, off, "dobjdesc"))
        cur = off
        live = 0
        while cur + 44 <= lay.n:
            did = struct.unpack_from(">i", p, cur)[0]
            if did == 18:
                cur += 44
                break
            live += 1
            sl = self.ptr(fid, cur + 4)
            if follow_dl and sl is not None and sl[0] in self.lay:
                self.dl_or_link(sl[0], sl[1], tag)
            cur += 44
        self.mark(fid, off, cur, "dobjdesc", f"dobjdesc@0x{off:x}", tag)
        return live

    def ptr_array(self, fid: int, off: int, count: int, child, cls="ptrtab", label="ptrs", tag=None, null_term=False):
        """Array of pointer slots.  child(target_file, target_off) called per non-null slot."""
        f = self.files[fid]
        lay = self.lay[fid]
        cur = off
        i = 0
        while cur + 4 <= lay.n and (null_term or i < count):
            sl = self.ptr(fid, cur)
            if sl is None:
                if null_term:
                    cur += 4
                    break
                cur += 4
                i += 1
                continue
            if sl[0] in self.lay:
                child(sl[0], sl[1])
            cur += 4
            i += 1
        self.mark(fid, off, cur, cls, f"{label}@0x{off:x}", tag)

    def anim_script(self, fid: int, off: int, tag=None):
        # Extent unknown without full event32 semantics: bound by next known start.
        lay = self.lay[fid]
        if (fid, off, "anim") in self.seen_root:
            return
        self.seen_root.add((fid, off, "anim"))
        end = self.next_start(fid, off)
        # trim trailing zero padding (<16 bytes) after the last non-zero word
        p = self.files[fid].payload
        e = end
        while e - 4 > off and p[e - 4:e] == b"\0\0\0\0" and (end - e) < 16:
            e -= 4
        self.mark(fid, off, e, "anim", f"anim@0x{off:x}", tag)
        # scripts may Jump to other scripts / contain pointer words: follow relocated slots
        cur = off
        while cur < e:
            sl = self.ptr(fid, cur)
            if sl is not None and sl[0] in self.lay and not (sl[0] == fid and off <= sl[1] < e):
                self.anim_script(sl[0], sl[1], tag)
            cur += 4

    def mobjsub(self, fid: int, off: int, tag=None):
        f = self.files[fid]
        p = f.payload
        lay = self.lay[fid]
        if (fid, off, "mobjsub") in self.seen_root:
            return
        self.seen_root.add((fid, off, "mobjsub"))
        SZ = 0x78
        if off + SZ > lay.n:
            return
        self.mark(fid, off, off + SZ, "mobj", f"MObjSub@0x{off:x}", tag)
        width = struct.unpack_from(">H", p, off + 0x0C)[0]
        height = struct.unpack_from(">H", p, off + 0x0E)[0]
        bfmt = p[off + 0x32]
        bsiz = p[off + 0x33]
        spr = self.ptr(fid, off + 4)
        if spr is not None and spr[0] in self.lay:
            tf, to = spr

            def tex(tf2, to2, _w=width, _h=height, _s=bsiz):
                nbytes = (_w * _h * bits_of(_s) + 7) // 8
                nbytes = min(nbytes, self.next_start(tf2, to2) - to2)
                self.mark(tf2, to2, to2 + nbytes, "tex", f"mobjtex@{tf2}:0x{to2:x}", tag)

            self.ptr_array(tf, to, 0, tex, "mobj", "sprites", tag, null_term=True)
        pal = self.ptr(fid, off + 0x2C)
        if pal is not None and pal[0] in self.lay:
            tf, to = pal
            # palette array of pointers; palette size from bsiz (CI4: 16 entries, CI8: 256)
            ent = 16 if bsiz == 0 else 256

            def palchild(tf2, to2, _e=ent):
                nb = min(2 * _e, self.next_start(tf2, to2) - to2)
                self.mark(tf2, to2, to2 + nb, "pal", f"mobjpal@{tf2}:0x{to2:x}", tag)

            self.ptr_array(tf, to, 0, palchild, "mobj", "palettes", tag, null_term=True)

    def mobjsub_list(self, fid: int, off: int, tag=None):
        """MObjSub** : NULL-terminated array of MObjSub*."""
        self.ptr_array(fid, off, 0, lambda tf, to: self.mobjsub(tf, to, tag), "mobj", "mobjsub-list", tag, null_term=True)

    def collision(self, fid: int, off: int, tag=None):
        f = self.files[fid]
        p = f.payload
        lay = self.lay[fid]
        if off + 28 > lay.n:
            return
        yak = struct.unpack_from(">H", p, off)[0]
        mobj_n = struct.unpack_from(">H", p, off + 0x14)[0]
        self.mark(fid, off, off + 28, "coll", f"MPGeometryData@0x{off:x}", tag)
        names = ["vertex_data", "vertex_id", "vertex_links", "line_info"]
        starts = []
        for i, nm in enumerate(names):
            sl = self.ptr(fid, off + 4 + 4 * i)
            if sl is not None and sl[0] == fid:
                starts.append((sl[1], nm))
        mo = self.ptr(fid, off + 0x18)
        if mo is not None and mo[0] == fid:
            starts.append((mo[1], "mapobjs"))
        starts.sort()
        for idx, (s, nm) in enumerate(starts):
            if nm == "line_info":
                ln = yak * 18
            elif nm == "mapobjs":
                ln = mobj_n * 6
            else:
                ln = None
            if ln is None:
                # bounded by the next collision array start or the geometry descriptor
                nxt = starts[idx + 1][0] if idx + 1 < len(starts) else off
                if nxt <= s:
                    nxt = self.next_start(fid, s)
                ln = nxt - s
            self.mark(fid, s, s + ln, "coll", f"{nm}@0x{s:x}", tag)

    def sprite_wallpaper(self, fid: int, off: int, tag="wallpaper"):
        """Sprite record + Bitmap table + pixel strips."""
        f = self.files[fid]
        p = f.payload
        lay = self.lay[fid]
        SPR = 0x48
        self.mark(fid, off, min(lay.n, off + SPR), "sprite", "Sprite", tag)
        bm = self.ptr(fid, off + 0x34)         # Sprite.bitmap
        if bm is None:
            return
        tf, to = bm
        # 44 bitmaps x 16 B (nbitmaps is at Sprite+0x24 as BE s16 pair: read via fixup count instead)
        # count = number of relocated buf slots in this run
        cnt = 0
        cur = to
        while cur + 16 <= lay.n:
            sl = self.ptr(fid, cur + 8)
            if sl is None:
                break
            cnt += 1
            cur += 16
        self.mark(tf, to, to + 16 * cnt, "sprite", f"Bitmap[{cnt}]", tag)
        for i in range(cnt):
            b = to + 16 * i
            sl = self.ptr(fid, b + 8)
            width = struct.unpack_from(">H", p, b)[0]
            act_h = struct.unpack_from(">H", p, b + 12)[0]
            nbytes = width * act_h * 2         # RGBA16
            self.mark(sl[0], sl[1], sl[1] + nbytes, "pixels", f"strip{i}", tag)

    def ground_header(self, fid: int, off: int, tag="map"):
        """MPGroundData at off (map file)."""
        f = self.files[fid]
        p = f.payload
        lay = self.lay[fid]
        # sizeof(MPGroundData) = 0xA8: alt_warning@0x88 + 32 bytes (asserted in
        # reloc_backend_assets.c ndsRelocNormalizeGroundDataBounds)
        self.mark(fid, off, off + 0xA8, "hdr", "MPGroundData", tag)
        # gr_desc[4] : dobjdesc, anim_joints, p_mobjsubs, p_matanim_joints
        rows = []
        for i in range(4):
            base = off + i * 16
            row = [self.ptr(fid, base + 4 * k) for k in range(4)]
            rows.append(row)
        live_counts = []
        for i, row in enumerate(rows):
            dd = row[0]
            live = 0
            if dd is not None and dd[0] in self.lay:
                live = self.dobjdesc_array(dd[0], dd[1], f"layer{i}")
            live_counts.append(live)
        for i, row in enumerate(rows):
            live = live_counts[i]
            aj, pm, pa = row[1], row[2], row[3]
            if aj is not None and aj[0] in self.lay:
                self.ptr_array(aj[0], aj[1], live, lambda tf, to: self.anim_script(tf, to, f"layer{i}"), "anim", "anim_joints", f"layer{i}")
            if pm is not None and pm[0] in self.lay:
                self.ptr_array(pm[0], pm[1], live, lambda tf, to: self.mobjsub_list(tf, to, f"layer{i}"), "mobj", "p_mobjsubs", f"layer{i}")
            if pa is not None and pa[0] in self.lay:
                self.ptr_array(pa[0], pa[1], live, lambda tf, to: self.ptr_array(tf, to, 0, lambda t2, o2: self.anim_script(t2, o2, f"layer{i}"), "anim", "matanim", f"layer{i}", null_term=True), "anim", "p_matanim", f"layer{i}")
        geom = self.ptr(fid, off + 0x40)
        if geom is not None and geom[0] in self.lay:
            self.collision(geom[0], geom[1], "collision")
        wall = self.ptr(fid, off + 0x48)
        if wall is not None and wall[0] in self.lay:
            self.sprite_wallpaper(wall[0], wall[1])
        iw = self.ptr(fid, off + 0x84)
        if iw is not None and iw[0] in self.lay:
            self.mark(iw[0], iw[1], iw[1] + 20, "hdr", "item_weights", tag)
        return self.ptr(fid, off + 0x80)     # map_nodes

    def finish(self):
        for lay in self.lay.values():
            lay.finish()


if __name__ == "__main__":
    idx = o.index()
    print(len(idx))
