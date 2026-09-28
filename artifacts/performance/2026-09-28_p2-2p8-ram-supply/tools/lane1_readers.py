#!/usr/bin/env python3
"""Lane 1: per-stage resident set, class tiling and reader attribution.

Inputs
  * O2R files (lane1_o2r)                       -- sizes, fixups, extern trees
  * typed-source tiling (lane1_tiling)          -- exact class of every byte
  * native stage packet (generate_nds_native_stage.generate) -- the DL roots, image
    references and owners the DS native stage path actually consumes
  * static texture corpus (lane1_static_corpus) -- textures the P1 pin set covers

Reader model (derived from the port source, cited in the report):
  wallpaper pixels        never read after load (native BG2 image streams from NitroFS)
  packet-only Gfx / Vtx   contents never read (state deltas + dense vertices are baked in the packet;
                          the DL is compared by ADDRESS only)
  packet image refs       read once by the warm upload (blob stages) / not read when the static corpus hits
  everything else         kept
"""
from __future__ import annotations

import bisect
import json
import re
import sys
from collections import defaultdict

import lane1_o2r as o
import lane1_classify as c
import lane1_tiling as tl
import lane1_static_corpus as sc
import generate_nds_native_stage as g
from native_stage_descriptors import get_descriptor

STAGES = [
    ("Peach's Castle", "Castle", "castle", 0),
    ("Sector Z", "Sector", "sector", 1),
    ("Kongo Jungle", "Jungle", "jungle", 2),
    ("Planet Zebes", "Zebes", "zebes", 3),
    ("Hyrule Castle", "Hyrule", "hyrule", 4),
    ("Yoshi's Island", "Yoster", "yoster", 5),
    ("Dream Land", "Pupupu", "dreamland", 6),
    ("Saffron City", "Yamabuki", "yamabuki", 7),
    ("Mushroom Kingdom", "Inishie", "inishie", 8),
]
_TXT = (o.REPO / "decomp/BattleShip-main/include/reloc_data.us.h").read_text()


def map_id(name: str) -> int:
    m = re.search(r"#define llGR%sMapFileID \(\(intptr_t\)(0x[0-9a-f]+)\)" % name, _TXT)
    return int(m.group(1), 16)


def role_of(f: o.OFile, mapid: int) -> str:
    if f.file_id == mapid:
        return "map"
    if f.rel.startswith("reloc_stages/Stage") or "Wallpaper" in f.rel:
        return "wallpaper"
    return "typed"


class Stage:
    def __init__(self, label, key, gname, gkind):
        self.label, self.key, self.gname, self.gkind = label, key, gname, gkind
        self.mid = map_id(key)
        self.tree = o.tree(self.mid)
        self.files = {f.file_id: f for f in self.tree}
        self.roles = {f.file_id: role_of(f, self.mid) for f in self.tree}
        self.tiles = {}
        for f in self.tree:
            blocks, _ = tl.tile(f.file_id, self.roles[f.file_id])
            self.tiles[f.file_id] = blocks
        self.starts = {fid: [b.off for b in bl] for fid, bl in self.tiles.items()}
        # packet
        self.pk = g.generate(o.REPO, gname)
        self.aid = [a.asset_id for a in self.pk.assets]
        self.desc = get_descriptor(gname)

    # ---------------------------------------------------------------- decl lookup
    def decl_at(self, fid: int, off: int):
        bl = self.tiles.get(fid)
        if bl is None:
            return None
        i = bisect.bisect_right(self.starts[fid], off) - 1
        if i < 0:
            return None
        b = bl[i]
        return (fid, b) if b.off <= off < b.end else None

    def key(self, fid, b):
        return (fid, b.off)

    # ---------------------------------------------------------------- packet walk
    def packet_reach(self):
        """Walk the packet's DL roots with the stateful decoder; returns spans by class."""
        w = c.Walker(self.tree)
        roots = sorted(set((self.aid[b.asset_index], b.root_offset) for b in self.pk.bindings))
        for fid, off in roots:
            w.decode_dl(fid, off, None, "pkt", 0, "dl")
        spans = defaultdict(list)     # class -> [(fid, start, end)]
        for tag, cl in w.reach_by_tag.items():
            for cls, sp in cl.items():
                spans[cls].extend(sp)
        # gfx spans are marked on the layout objs
        for fid, lay in w.lay.items():
            for s, e, cls, lb in lay.objs:
                if cls == "gfx":
                    spans["gfx"].append((fid, s, e))
        self.roots = roots
        self.pkt_walker = w
        return spans

    def image_refs(self):
        refs = set()
        for d in self.pk.state_deltas:
            if d.effect == g.STATE_EFFECT_IMAGE:
                refs.add((self.aid[d.asset_index], d.w1))
        return refs

    # ---------------------------------------------------------------- analysis
    def analyze(self):
        out = {"label": self.label, "gkind": self.gkind, "map_id": self.mid}
        # files
        out["files"] = []
        total_alloc = o.tree_alloc(self.tree)
        out["tree_alloc"] = total_alloc
        spans = self.packet_reach()
        img_refs = self.image_refs()

        # incoming-slot graph
        inc = defaultdict(set)
        for f in self.tree:
            for slot, r in list(f.internal.items()) + list(f.external.items()):
                src = self.decl_at(f.file_id, slot)
                dst = self.decl_at(r.asset_id, r.offset)
                if src is None or dst is None:
                    continue
                inc[(dst[0], dst[1].off)].add((src[0], src[1].off))

        def decls(fid):
            return self.tiles[fid]

        # packet-reached gfx/vtx decls
        reached = set()
        for cls in ("gfx", "vtx"):
            for fid, s, e in spans.get(cls, []):
                bl = self.tiles.get(fid)
                if not bl:
                    continue
                i = max(0, bisect.bisect_right(self.starts[fid], s) - 1)
                while i < len(bl) and bl[i].off < e:
                    b = bl[i]
                    if b.cls == cls and b.end > s:
                        reached.add((fid, b.off))
                    i += 1
        # packet-owned structs: DObjDesc arrays named by owner_specs + gr_desc rows, and DLLink tables they reach
        pkt_struct = set()
        cls_of = {}
        for fid, bl in self.tiles.items():
            for b in bl:
                cls_of[(fid, b.off)] = b

        rname = {"stage_geometry": None, "stage_images": None, "stage_actors": None, "stage_map": None}
        res_to_fid = {k: v["file_id"] for k, v in self.desc.o2r_inputs.items()}
        for row in self.desc.owner_specs:
            owner, name, resource, dobj_off = row[0], row[1], row[2], row[3]
            fid = res_to_fid[resource]
            d = self.decl_at(fid, dobj_off)
            if d is not None:
                pkt_struct.add((d[0], d[1].off))
        # DLLink tables / DObjDesc arrays reached from the header gr_desc rows
        mapf = self.files[self.mid]
        hdr = 0x14
        for i in range(4):
            r = mapf.internal.get(hdr + 16 * i) or mapf.external.get(hdr + 16 * i)
            if r is not None:
                d = self.decl_at(r.asset_id, r.offset)
                if d is not None:
                    pkt_struct.add((d[0], d[1].off))
        # dltab decls pointed to by packet-owned dobjdesc decls
        changed = True
        while changed:
            changed = False
            for key in list(inc.keys()):
                b = cls_of.get(key)
                if b is not None and b.cls == "dltab" and key not in pkt_struct:
                    if inc[key] and all(s in pkt_struct for s in inc[key]):
                        pkt_struct.add(key)
                        changed = True
        S = set(reached)
        changed = True
        while changed:
            changed = False
            for key in list(S):
                for srck in inc.get(key, ()):
                    if srck in pkt_struct or srck in S:
                        continue
                    S.discard(key)
                    changed = True
                    break
        pkt_only = S

        # textures / palettes
        pkt_img_decls = set()
        for (afid, aoff) in img_refs:
            d = self.decl_at(afid, aoff)
            if d is not None and d[1].cls in ("tex", "pal"):
                pkt_img_decls.add((d[0], d[1].off))
        static_decls = set()
        if self.gname == "dreamland":
            for r in sc.records():
                for fid_, off_ in ((r["image_asset"], r["image_off"]), (r["tlut_asset"], r["tlut_off"])):
                    if fid_ in self.files:
                        d = self.decl_at(fid_, off_)
                        if d is not None and d[1].cls in ("tex", "pal"):
                            static_decls.add((d[0], d[1].off))
        # material (MObjSub sprite/palette array) referenced textures
        mat_decls = set()
        for key, srcs in inc.items():
            b = cls_of.get(key)
            if b is None or b.cls not in ("tex", "pal"):
                continue
            for s in srcs:
                sb = cls_of.get(s)
                if sb is not None and sb.cls == "mobj":
                    mat_decls.add(key)

        per_file = []
        tot = defaultdict(int)
        for f in self.tree:
            fid = f.file_id
            cb = tl.class_bytes(self.tiles[fid])
            row = {"file_id": fid, "name": f.rel.split("/")[-1], "role": self.roles[fid],
                   "payload": f.data_size, "aligned": f.aligned, "classes": cb}
            gp = vp = 0
            for b in self.tiles[fid]:
                key = (fid, b.off)
                if b.cls == "gfx" and key in pkt_only:
                    gp += b.size
                if b.cls == "vtx" and key in pkt_only:
                    vp += b.size
            row["gfx_pkt_only"] = gp
            row["vtx_pkt_only"] = vp
            dp = 0
            for b in self.tiles[fid]:
                if b.cls == "dobjdesc" and (fid, b.off) in pkt_struct:
                    dp += b.size
            row["dobjdesc_pkt_owned"] = dp
            tex_pkt = tex_static = tex_mat = tex_other = 0
            pal_pkt = pal_static = pal_mat = pal_other = 0
            for b in self.tiles[fid]:
                if b.cls not in ("tex", "pal"):
                    continue
                key = (fid, b.off)
                if key in static_decls:
                    bucket = "static"
                elif key in pkt_img_decls:
                    bucket = "pkt"
                elif key in mat_decls:
                    bucket = "mat"
                else:
                    bucket = "other"
                if b.cls == "tex":
                    if bucket == "static": tex_static += b.size
                    elif bucket == "pkt": tex_pkt += b.size
                    elif bucket == "mat": tex_mat += b.size
                    else: tex_other += b.size
                else:
                    if bucket == "static": pal_static += b.size
                    elif bucket == "pkt": pal_pkt += b.size
                    elif bucket == "mat": pal_mat += b.size
                    else: pal_other += b.size
            row.update(tex_static=tex_static, tex_pkt=tex_pkt, tex_mat=tex_mat, tex_other=tex_other,
                       pal_static=pal_static, pal_pkt=pal_pkt, pal_mat=pal_mat, pal_other=pal_other)
            per_file.append(row)
            for k, v in cb.items():
                tot[k] += v
        out["files"] = per_file
        out["class_total"] = dict(tot)
        out["pkt_roots"] = len(self.roots)
        out["packet_assets"] = self.aid
        out["extra_preload_assets"] = [a for a in self.aid if a not in self.files]
        out["pkt_only_decl_count"] = len(pkt_only)
        return out


def run_all():
    res = []
    for label, key, gname, gkind in STAGES:
        st = Stage(label, key, gname, gkind)
        res.append(st.analyze())
    return res


if __name__ == "__main__":
    res = run_all()
    json.dump(res, sys.stdout if len(sys.argv) < 2 else open(sys.argv[1], "w"), indent=1)
