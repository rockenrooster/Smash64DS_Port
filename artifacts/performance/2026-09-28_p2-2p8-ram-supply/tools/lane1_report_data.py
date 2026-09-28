#!/usr/bin/env python3
"""Lane 1: build the numbers the report quotes (every figure comes from here).

Writes ../lane1_data.json and prints the tables in markdown.

Definitions (see the report for the reader evidence behind each):
  wallpaper pixels          never read after load (T1)
  packet-only Gfx / Vtx     Gfx/Vtx decls that (a) the native stage packet's DL roots reach and
                            (b) no relocation slot outside packet-owned structures points into (T2)
  static-corpus textures    Dream Land only: textures/palettes the P1 static texture corpus covers (T3)
  phantom bank              Jungle: ExternDataBank107 listed in the packet asset table, referenced by
                            no binding / epoch / material / state delta (T4)
"""
from __future__ import annotations

import json
import struct
import sys
from collections import defaultdict
from pathlib import Path

import lane1_o2r as o
import lane1_tiling as tl
import lane1_readers as rd
import lane1_run_all as ra

OUT = Path(__file__).resolve().parents[1]
BUILD_STAGES = o.REPO / "builds" / "build-p2p8-s1" / "nitrofs" / "stages"


def align16(n: int) -> int:
    return (n + 15) & ~15


def alloc_of(aligned_sizes):
    total = 0
    for i, a in enumerate(aligned_sizes):
        total = a if i == 0 else align16(total) + a
    return total


def spans_needed(blocks, removed_keys):
    """Number of maximal runs of kept bytes when `removed_keys` (block offsets) are dropped."""
    runs = 0
    in_run = False
    for b in blocks:
        if b.off in removed_keys:
            in_run = False
        else:
            if not in_run:
                runs += 1
                in_run = True
    return runs


def build():
    out = []
    for label, key, gname, gkind in rd.STAGES:
        st = rd.Stage(label, key, gname, gkind)
        a = st.analyze()
        tree = st.tree
        files = []
        for f in tree:
            files.append({"file_id": f.file_id, "name": f.rel.split("/")[-1], "role": st.roles[f.file_id],
                          "payload": f.data_size, "aligned": f.aligned})
        tree_alloc = o.tree_alloc(tree)
        # ------------------------------------------------------------ T1 (variant map + stub sprite)
        wall = [f for f in tree if st.roles[f.file_id] == "wallpaper"][0]
        pix = 0x269C8                          # pixel strips [0, 0x269C8)
        stub = wall.data_size - pix            # Bitmap[44] + Sprite = 776
        sizes_new = []
        for f in tree:
            if f is wall:
                continue
            if st.roles[f.file_id] == "map":
                sizes_new.append(align16(f.data_size + stub))
            else:
                sizes_new.append(f.aligned)
        alloc_t1 = alloc_of(sizes_new)
        t1_net = tree_alloc - alloc_t1
        t1_gross = pix                          # pixel bytes that no longer exist
        # ------------------------------------------------------------ T2
        gv = defaultdict(int)
        removed_by_file = defaultdict(set)
        cells = 0
        for row in a["files"]:
            gv["gfx"] += row["gfx_pkt_only"]
            gv["vtx"] += row["vtx_pkt_only"]
        # exact removed decl sets for span/cell accounting
        pkt_only_keys = defaultdict(set)
        # recompute per decl (cheap): a decl is packet-only if analyze() counted it; mirror by re-running
        # the same rule through Stage internals is avoided: use per-file totals only and bound cells/spans
        # by the number of packet roots and packet-only decls reported by analyze().
        roots = a["pkt_roots"]
        cells = 8 * roots
        pkt_decls = a["pkt_only_decl_count"]
        spans = pkt_decls + len(tree)          # upper bound: one run break per removed decl
        t2_gross = gv["gfx"] + gv["vtx"]
        t2_net = t2_gross - cells - 12 * spans - 32 * len(tree)
        # ------------------------------------------------------------ T3 (Dream Land only)
        t3_gross = sum(r["tex_static"] + r["pal_static"] for r in a["files"])
        t3_cells = 8 * (0 if gname != "dreamland" else 40)
        t3_net = max(0, t3_gross - t3_cells)
        # ------------------------------------------------------------ T4 (Jungle phantom packet asset)
        t4 = 0
        phantom = None
        aid = a["packet_assets"]
        if gname == "jungle":
            pk = st.pk
            used = set()
            for b in pk.bindings:
                used.add(b.asset_index)
            for e in pk.epochs:
                used.add(e.asset_index)
            for m in pk.materials:
                used.add(m.asset_index)
            for d in pk.state_deltas:
                if d.asset_index != 255:
                    used.add(d.asset_index)
            unused = [aid[i] for i in range(len(aid)) if i not in used and aid[i] != st.mid]
            phantom = unused
            for u in unused:
                if u not in st.files:                       # not in the map tree: an extra preload
                    t4 += o.index()[u].aligned
        # ------------------------------------------------------------ class totals
        cls_total = defaultdict(int)
        for row in a["files"]:
            for k, v in row["classes"].items():
                cls_total[k] += v
        extra = 0
        for u in a["extra_preload_assets"]:
            extra += o.index()[u].aligned
        # ------------------------------------------------------------ reader buckets A / L / F
        A = L = F = 0
        for row in a["files"]:
            c = row["classes"]
            if row["role"] == "wallpaper":
                A += c.get("pixels", 0)
                L += c.get("sprite", 0)
                continue
            A += c.get("pad", 0)
            A += row["gfx_pkt_only"] + row["vtx_pkt_only"]
            A += row["tex_static"] + row["pal_static"]
            L += row["tex_pkt"] + row["pal_pkt"]
            L += row["dobjdesc_pkt_owned"]
            F += c.get("dobjdesc", 0) - row["dobjdesc_pkt_owned"]
            F += (c.get("gfx", 0) - row["gfx_pkt_only"]) + (c.get("vtx", 0) - row["vtx_pkt_only"])
            F += row["tex_mat"] + row["tex_other"] + row["pal_mat"] + row["pal_other"]
            for k in ("dltab", "mobj", "anim", "coll", "hdr", "attr", "other16"):
                F += c.get(k, 0)
        payload_total = sum(f["payload"] for f in files)
        # ------------------------------------------------------------ non-tree resident stage data (info)
        blob = gxp = None
        nm = {"castle": "castle", "sector": "sector", "jungle": "jungle", "zebes": "zebes", "hyrule": "hyrule",
              "yoster": "yoster", "dreamland": None, "yamabuki": "yamabuki", "inishie": "inishie"}[gname]
        if nm:
            b = (BUILD_STAGES / f"native_stage_{nm}.bin").read_bytes()
            blob = {"file": len(b), "body": struct.unpack_from("<I", b, 12)[0]}
        g = (BUILD_STAGES / f"{gname}.gxp").read_bytes()
        gxp = {"file": len(g), "body": struct.unpack_from("<I", g, 32)[0]}
        out.append({
            "label": label, "gkind": gkind, "map_id": st.mid, "files": files,
            "payload_total": payload_total, "tree_alloc": tree_alloc, "extra_preload": extra,
            "extra_preload_assets": a["extra_preload_assets"], "class_total": dict(cls_total),
            "per_file": a["files"], "pkt_roots": roots, "pkt_only_decls": pkt_decls,
            "T1_gross_pixels": t1_gross, "T1_net_variant_map": t1_net, "T1_net_skip_dep": wall.aligned,
            "alloc_after_T1": alloc_t1,
            "T2_gross": t2_gross, "T2_gfx": gv["gfx"], "T2_vtx": gv["vtx"], "T2_cells": cells,
            "T2_spans_bound": spans, "T2_net": t2_net,
            "T3_gross": t3_gross, "T3_net": t3_net,
            "T4_phantom_assets": phantom, "T4": t4,
            "A_never_read": A, "L_load_or_first_use": L, "F_read_in_frames": F,
            "blob": blob, "gxp": gxp,
        })
    return out


def walker_crosscheck():
    rows = []
    tot_reached = tot_agree = 0
    dis = defaultdict(int)
    for label, key, gname, gkind in rd.STAGES:
        mid, tree, w = ra.walk(key)
        cmp = ra.compare(tree, w, mid)
        reached = agree = 0
        for fid, (ag, d) in cmp.items():
            reached += ag + sum(d.values())
            agree += ag
            for k, v in d.items():
                dis[k] += v
        rows.append({"label": label, "walker_reached": reached, "agree": agree})
        tot_reached += reached
        tot_agree += agree
    return {"rows": rows, "reached": tot_reached, "agree": tot_agree,
            "disagreements": {f"{a}->{b}": v for (a, b), v in dis.items()}}


if __name__ == "__main__":
    data = build()
    xc = walker_crosscheck()
    dump = {"stages": data, "walker_crosscheck": xc}
    (OUT / "lane1_data.json").write_text(json.dumps(dump, indent=1))
    print("wrote", OUT / "lane1_data.json")
    hdr = "| stage | payload | tree alloc | T1 net | T2 net | T3 net | T4 | A never | L load/1st | F frames |"
    print(hdr)
    for s in data:
        print(f"| {s['label']} | {s['payload_total']:,} | {s['tree_alloc']:,} | {s['T1_net_variant_map']:,} | "
              f"{s['T2_net']:,} | {s['T3_net']:,} | {s['T4']:,} | {s['A_never_read']:,} | "
              f"{s['L_load_or_first_use']:,} | {s['F_read_in_frames']:,} |")
    print("walker/tiling agreement:", xc["agree"], "of", xc["reached"], xc["disagreements"])
