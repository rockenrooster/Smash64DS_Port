#!/usr/bin/env python3
"""Lane 1: render the markdown tables quoted in lane1-stage-ground-files.md from ../lane1_data.json."""
import json
from pathlib import Path

D = json.load(open(Path(__file__).resolve().parents[1] / "lane1_data.json"))
S = D["stages"]
WALL = 158928


def n(x):
    return f"{x:,}"


def table1():
    out = ["| stage (gkind) | map | resident O2R files: id name payload B | tree alloc B | extra preload | blob body | .gxp body |",
           "|---|---|---|---:|---|---:|---:|"]
    for s in S:
        fl = "; ".join(f"{f['file_id']} {f['name']} {n(f['payload'])}" for f in s["files"])
        ex = ", ".join(f"asset {a}: {n(27792)}" for a in s["extra_preload_assets"]) or "-"
        blob = n(s["blob"]["body"]) if s["blob"] else "linked (0)"
        out.append(f"| {s['label']} ({s['gkind']}) | {s['map_id']} | {fl} | {n(s['tree_alloc'])} | {ex} | {blob} | {n(s['gxp']['body'])} |")
    return "\n".join(out)


def table2():
    out = ["| stage | wallpaper pixels | wallpaper Sprite+Bitmap | Gfx | Vtx | texels | TLUT | DObjDesc+DLLink | anim scripts/tables | MObjSub+ptr lists | collision | header+attr | pad+u16 other | total |",
           "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for s in S:
        c = s["class_total"]
        row = [c.get("pixels", 0), c.get("sprite", 0), c.get("gfx", 0), c.get("vtx", 0), c.get("tex", 0), c.get("pal", 0),
               c.get("dobjdesc", 0) + c.get("dltab", 0), c.get("anim", 0), c.get("mobj", 0), c.get("coll", 0),
               c.get("hdr", 0) + c.get("attr", 0), c.get("pad", 0) + c.get("other16", 0)]
        assert sum(row) == s["payload_total"], (s["label"], sum(row), s["payload_total"])
        out.append(f"| {s['label']} | " + " | ".join(n(x) for x in row) + f" | {n(sum(row))} |")
    return "\n".join(out)


def total_reclaim(s):
    return s["T1_net_variant_map"] + s["T2_net"] + s["T3_net"] + s["T4"]


def table3():
    out = ["| stage | A: never read after load | L: read at load / first use only | F: read during frames | T1 wallpaper | T2 packet-only Gfx+Vtx | T3 static-corpus textures | T4 phantom bank | T1..T4 | tree alloc after T1 |",
           "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for s in S:
        assert s["A_never_read"] + s["L_load_or_first_use"] + s["F_read_in_frames"] == s["payload_total"]
        out.append(f"| {s['label']} | {n(s['A_never_read'])} | {n(s['L_load_or_first_use'])} | {n(s['F_read_in_frames'])} | "
                   f"{n(s['T1_net_variant_map'])} | {n(s['T2_net'])} | {n(s['T3_net'])} | {n(s['T4'])} | {n(total_reclaim(s))} | {n(s['alloc_after_T1'])} |")
    return "\n".join(out)


def table4():
    out = ["| stage | tree alloc | wallpaper container share of tree | after T1 | packet-referenced texels+TLUT (T5 candidate, not claimed) | packet-only Gfx+Vtx gross | packet DL roots |",
           "|---|---:|---:|---:|---:|---:|---:|"]
    for s in S:
        t5 = sum(r["tex_pkt"] + r["pal_pkt"] for r in s["per_file"])
        out.append(f"| {s['label']} | {n(s['tree_alloc'])} | {100.0 * WALL / s['tree_alloc']:.1f}% | {n(s['alloc_after_T1'])} | {n(t5)} | {n(s['T2_gross'])} | {s['pkt_roots']} |")
    return "\n".join(out)


def summary():
    out = ["| stage | tree alloc | T1 (high) | T1+T2 (T2 medium) | T1..T4 (all tiers) | tree after T1 |",
           "|---|---:|---:|---:|---:|---:|"]
    for s in S:
        t12 = s["T1_net_variant_map"] + s["T2_net"]
        out.append(f"| {s['label']} | {n(s['tree_alloc'])} | {n(s['T1_net_variant_map'])} | {n(t12)} | {n(total_reclaim(s))} | {n(s['alloc_after_T1'])} |")
    return "\n".join(out)


CONF = {
    "Peach's Castle": "T1 high; T2 medium",
    "Sector Z": "T1 high; T2 medium (FoxSpecial3 12,160 B kept: no packet owner)",
    "Kongo Jungle": "T1 high; T2 medium; T4 medium (heap vs overlay UNPROVEN)",
    "Planet Zebes": "T1 high; T2 medium",
    "Hyrule Castle": "T1 high; T2 medium (100% of its Gfx/Vtx is packet-only)",
    "Yoshi's Island": "T1 high; T2 medium",
    "Dream Land": "T1 high (frozen P1 pins); T2 medium; T3 low-medium (corpus engagement UNPROVEN)",
    "Saffron City": "T1 high; T2 medium",
    "Mushroom Kingdom": "T1 high; T2 medium",
}


def table5():
    out = ["| stage | resident files (ids) | resident B | A never read | L load/first use | F read in frames | T1 | T2 | T3 | T4 | confidence |",
           "|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---|"]
    for s in S:
        ids = ", ".join(str(f["file_id"]) for f in s["files"]) + (f" (+{', '.join(map(str, s['extra_preload_assets']))} preload)" if s["extra_preload_assets"] else "")
        out.append(f"| {s['label']} | {ids} | {n(s['payload_total'] + s['extra_preload'])} | {n(s['A_never_read'])} | {n(s['L_load_or_first_use'])} | {n(s['F_read_in_frames'])} | "
                   f"{n(s['T1_net_variant_map'])} | {n(s['T2_net'])} | {n(s['T3_net'])} | {n(s['T4'])} | {CONF[s['label']]} |")
    return "\n".join(out)


def appendix():
    out = ["| stage | file | role | payload | Gfx (pkt-only) | Vtx (pkt-only) | texels | TLUT | DObjDesc+DLLink | anim | MObjSub+ptr | coll | hdr+attr | pad+o16 | wallpaper pix / sprite |",
           "|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for s in S:
        for f in s["per_file"]:
            c = f["classes"]
            out.append(f"| {s['label']} | {f['file_id']} {f['name']} | {f['role']} | {n(f['payload'])} | {n(c.get('gfx', 0))} ({n(f['gfx_pkt_only'])}) | "
                       f"{n(c.get('vtx', 0))} ({n(f['vtx_pkt_only'])}) | {n(c.get('tex', 0))} | {n(c.get('pal', 0))} | "
                       f"{n(c.get('dobjdesc', 0) + c.get('dltab', 0))} | {n(c.get('anim', 0))} | {n(c.get('mobj', 0))} | {n(c.get('coll', 0))} | "
                       f"{n(c.get('hdr', 0) + c.get('attr', 0))} | {n(c.get('pad', 0) + c.get('other16', 0))} | {n(c.get('pixels', 0))} / {n(c.get('sprite', 0))} |")
    return "\n".join(out)


def xcheck():
    x = D["walker_crosscheck"]
    return x, "; ".join(f"{k} {n(v)} B" for k, v in sorted(x["disagreements"].items(), key=lambda kv: -kv[1]))


if __name__ == "__main__":
    print("### Summary\n\n" + summary())
    print("\n### Table 1\n\n" + table1())
    print("\n### Table 2\n\n" + table2())
    print("\n### Table 3\n\n" + table3())
    print("\n### Table 4\n\n" + table4())
    print("\n### Table 5\n\n" + table5())
    print("\n### Appendix\n\n" + appendix())
    x, d = xcheck()
    print(f"\nWalker vs tiling: {n(x['agree'])} of {n(x['reached'])} ({100.0 * x['agree'] / x['reached']:.2f}%); {d}")
