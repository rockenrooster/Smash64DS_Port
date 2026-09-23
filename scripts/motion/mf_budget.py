#!/usr/bin/env python3
"""MF experiment: resident-bank budget over every four-kind roster.

For each roster of four distinct shipping kinds (C(12,4) = 495), the resident
motion set is the union over fighters of `mf_corpus.match_set` with the
worst-case switches: pipes ON (Mushroom Kingdom), items ON, taunt ON, the
main-table-unreferenced clip kept, victim-only clips and Kirby copy clips only
for opponents present. Clip ids shared between kinds (Luigi <- Mario clips,
Purin <- Kirby clips) are resident once.

Costs per roster:
  raw   sum of BPS1 clip bytes (today's format)
  B     candidate B "fast": sum of per-clip bitstreams (4 B aligned)
        + 8 B directory row per resident clip + tables (global scope:
        once; per-kind scope: once per bank touched)
  A     candidate A (LZ4-class, 32 KB per-bank dictionary) from --a-clips
        (per-clip sizes written by mf_candidate_a.py --clips-json) + one
        dictionary per bank touched + 8 B per clip

Usage:
  python scripts/motion/mf_budget.py [--a-clips A.json] [--json OUT]
"""

from __future__ import annotations

import argparse
import itertools
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import mf_codec_b as mb  # noqa: E402
import mf_corpus as mc  # noqa: E402

SHIPPING = ("mario", "fox", "donkey", "samus", "luigi", "link", "yoshi",
            "captain", "kirby", "pikachu", "purin", "ness")
DIR_ROW = 8
TARGET = 710000   # the A2 budget, "~710 KB" = 0.45 x ~1.58 MB (docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md:252-257)
STRESS = ("donkey", "samus", "link", "kirby")


def roster_set(c, roster, pipes=True):
    ids = set()
    for k in roster:
        opp = [o for o in roster if o != k]
        ids |= set(mc.match_set(c, k, opp, stage_pipes=pipes))
    return ids


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--a-clips", type=pathlib.Path)
    ap.add_argument("--json", type=pathlib.Path)
    a = ap.parse_args()
    c = mc.load_corpus()
    clips = c["clips"]
    log = lambda *x: None  # noqa: E731

    per_g, per_k = {}, {}
    rg = mb.run(list(mc.KINDS), "global", mb.Config("fast", "slope"), log=log,
                clips_out=per_g)
    rk = mb.run(list(mc.KINDS), "kind", mb.Config("fast", "slope"), log=log,
                clips_out=per_k)
    g_tables = rg["global_tables_bytes"]
    k_tables = {k: v["tables_bytes"] for k, v in rk["kinds"].items()}
    a_clips = a_dict = None
    if a.a_clips and a.a_clips.exists():
        blob = json.loads(a.a_clips.read_text())
        key = "u1/d32"
        a_clips = {int(h, 16): v["comp"] for h, v in blob[key].items()}
        a_dict = 32768

    def cost(ids):
        raw = sum(len(clips[i]["bytes"]) for i in ids)
        banks = {clips[i]["bank"] for i in ids}
        bg = sum(per_g[i]["comp"] for i in ids) + DIR_ROW * len(ids) + g_tables
        bk = sum(per_k[i]["comp"] for i in ids) + DIR_ROW * len(ids) + \
            sum(k_tables[b] for b in banks)
        out = {"clips": len(ids), "raw": raw, "B_global": bg, "B_kind": bk,
               "banks": sorted(banks)}
        if a_clips:
            out["A32"] = sum(a_clips[i] for i in ids) + DIR_ROW * len(ids) + \
                a_dict * len(banks)
        return out

    rows = []
    for roster in itertools.combinations(SHIPPING, 4):
        ids = roster_set(c, roster)
        r = cost(ids)
        r["roster"] = roster
        rows.append(r)
    by_raw = sorted(rows, key=lambda r: -r["raw"])
    by_bg = sorted(rows, key=lambda r: -r["B_global"])
    print("rosters evaluated: %d" % len(rows))
    hdr = "%-34s %5s %9s %9s %6s %9s %6s" % ("roster", "clips", "raw", "B_global",
                                               "xraw", "B_kind", "xraw")
    if a_clips:
        hdr += " %9s %6s" % ("A32", "xraw")
    print("\nworst five by raw BPS1 bytes:")
    print(hdr)

    def show(r):
        s = "%-34s %5d %9d %9d %6.3f %9d %6.3f" % (
            "+".join(r["roster"]), r["clips"], r["raw"], r["B_global"],
            r["B_global"] / r["raw"], r["B_kind"], r["B_kind"] / r["raw"])
        if a_clips:
            s += " %9d %6.3f" % (r["A32"], r["A32"] / r["raw"])
        print(s)
    for r in by_raw[:5]:
        show(r)
    print("\nworst five by candidate-B (global tables) resident bytes:")
    print(hdr)
    for r in by_bg[:5]:
        show(r)
    print("\nstress roster (DK/Samus/Link/Kirby):")
    stress_ws = cost(roster_set(c, STRESS, pipes=True))
    stress_ws["roster"] = STRESS
    show(stress_ws)
    stress_dl = cost(roster_set(c, STRESS, pipes=False))
    stress_dl["roster"] = ("dreamland",) + STRESS
    show(stress_dl)
    over = [r for r in rows if r["B_global"] > TARGET]
    print("\nrosters whose B_global exceeds %d B: %d" % (TARGET, len(over)))
    # per-kind worst-case resident sets (for the per-fighter view)
    print("\nper-kind worst-case set (opponents = the three kinds that maximise it):")
    per_kind = {}
    for k in SHIPPING:
        best = None
        for opp in itertools.combinations([o for o in SHIPPING if o != k], 3):
            ids = set(mc.match_set(c, k, opp, stage_pipes=True))
            raw = sum(len(clips[i]["bytes"]) for i in ids)
            if best is None or raw > best[0]:
                best = (raw, ids, opp)
        raw, ids, opp = best
        bg = sum(per_g[i]["comp"] for i in ids) + DIR_ROW * len(ids)
        mx = max(len(clips[i]["bytes"]) for i in ids)
        per_kind[k] = {"raw": raw, "clips": len(ids), "B_global_ex_tables": bg,
                       "largest_clip": mx, "opponents": opp}
        print("  %-8s %4d clips raw %7d  B %7d (%.3f)  largest clip %5d  vs %s" % (
            k, len(ids), raw, bg, bg / raw, mx, "+".join(opp)))
    if a.json:
        a.json.write_text(json.dumps({
            "target_bytes": TARGET, "global_tables": g_tables,
            "kind_tables": k_tables,
            "worst_by_raw": by_raw[:10], "worst_by_B_global": by_bg[:10],
            "stress_worst_stage": stress_ws, "stress_dreamland": stress_dl,
            "per_kind_worst": per_kind,
            "rosters_over_target_B_global": len(over)}, indent=1, default=list))
    return 0


if __name__ == "__main__":
    sys.exit(main())
