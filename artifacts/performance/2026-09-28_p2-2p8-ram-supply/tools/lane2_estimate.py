#!/usr/bin/env python3
"""Lane 2: master table and reclaim estimates.  Reads the JSON written by the
other lane2_* scripts (run them first: lane2_run_all.sh does) and applies ONE
explicit rule set:

  resident_now  bytes the file occupies on the general heap in the shipping
                config (IFCommonGameStatus: the compact image, HEAD default)
  keep          bytes some post-load reader still needs (per the read model)
  A             reclaimable with NO new reader: dead bytes only, net of the
                span table (12 B per kept run, NDSRelocIfSpan-sized), one 28 B
                identity stub per static-corpus key, and one 8 B root cell per
                reachable effect display-list root
  B             A plus bytes read only while the interface/effect is being
                built (setup/prepare/atlas reads), if that reader is retargeted
                to read the same range from the NitroFS copy (byte-identical
                to the O2R: lane2_inventory) or bakes before the drop
  confidence    per column.  H: holds for the linked code as read (structure
                and no-reader facts, no unproven premise); M: holds unless a
                listed UNPROVEN item is false, or needs a mechanism not yet
                built/measured; L: weak
"""
from __future__ import annotations

import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
SPAN = 12   # bytes per kept run (NDSRelocIfSpan / NDSPreviewPackSpan are both 12 B)
# One identity-only static texture key (EF2's three shadow images) must still
# resolve through ndsRelocNativeAssetAddress -> ndsPreviewFileOffset, which maps
# source offsets through SPANS only (src/port/reloc_preview_pack.c:110-133; the
# 8 B ENDDL cells are for display-list roots only).  Price it as a retained
# 16 B stub plus its 12 B span row.
CELL = SPAN + 16
ROOT = 8    # dense identity cell {G_ENDDL, source root offset} (NDSPreviewPackSection roots table)


def J(name):
    return json.loads((HERE / name).read_text())


def main():
    inv = J("lane2_inventory.json")
    ifc = J("lane2_ifcommon.json")
    eff = J("lane2_effects.json")
    itm = J("lane2_items_model.json")
    spr = J("lane2_sprites_counts.json")
    rows = []

    def add(owner, fid, name, payload, resident, keep, A, B, conf, note, tags=None):
        ca, cb = (conf.split("/") + [conf])[:2] if "/" in conf else (conf, conf)
        rows.append({"owner": owner, "fid": fid, "name": name, "payload": payload,
                     "resident_now": resident, "keep": keep, "A": max(A, 0), "B": max(B, 0),
                     "conf": conf, "confA": ca, "confB": cb, "note": note, "tags": tags or {}})

    # --- owner A: scVSBattleSetupFiles -------------------------------------
    add("A", 166, "IFCommonPlayer", 976, 976, 976, 0, 0, "H", "DObjDesc/DL/Vtx/anim/IA8; tiny, kept")
    g = ifc["IFCommonGameStatus"]
    gt = g["tags_resident_image"]
    gspans = spr["IFCommonGameStatus"]
    keep_g = gt["keep_headers"] + gt["keep_bitmap_arrays"]
    A_g = gt["pad_other"] - SPAN * gspans
    add("A", 82, "IFCommonGameStatus (compact image)", 152288, 21056, keep_g,
        A_g, A_g + gt["setup_read_pixels"], "H/M",
        "letters (131,232 B) already dropped by NDS_IF_GAMESTATUS_COMPACT; lamp/rod/frame pixels read only by the scene-entry atlas prepare",
        {"keep_headers": gt["keep_headers"], "keep_bitmap_arrays": gt["keep_bitmap_arrays"],
         "setup_read_pixels": gt["setup_read_pixels"], "pad_other": gt["pad_other"]})
    add("A", 82, "IFCommonGameStatus baked end streams", 0, g["baked_end_streams_measured"],
        0, 0, g["baked_end_streams_measured"], "-/M",
        "RLE TIME UP / GAME SET banks (measured 22,104 B); decoded into OBJ VRAM at game end; could be a NitroFS pack read at the announcement")
    for n, fid in (("IFCommonPlayerDamage", 164), ("IFCommonTimer", 165), ("IFCommonDigits", 36),
                   ("IFCommonBattlePause", 197), ("IFCommonPlayerTags", 38), ("IFCommonAnnounceCommon", 37)):
        v = ifc[n]
        t = v["tags"]
        sprites = v["sprites"]
        ov = SPAN * sprites
        dead = t.get("dead_pixels", 0) + t.get("dead_bitmap_array", 0) + t.get("pad_other", 0)
        setup = t.get("setup_read_pixels", 0)
        conf = "M/M" if n in ("IFCommonAnnounceCommon", "IFCommonBattlePause") else "H/H"
        add("A", fid, n, v["payload"], v["payload"], t["keep_headers"] + t.get("keep_bitmap_arrays", 0),
            dead - ov, dead - ov + setup, conf, "", t)
    v = ifc["IFCommonItem"]
    add("B", 87, "IFCommonItem", v["payload"], v["payload"], v["payload"], 0, 0, "H",
        "160 B; arrow sprite baked at itManagerInitItems; not worth compacting")

    # --- owner B: itManagerInitItems ---------------------------------------
    add("B", 251, "ITCommonData", 3392, 3392, 3392, 0, 0, "H",
        "68 externs + attribute rows decoded once per kind by ndsItDecodeAttributes; kept whole")
    it = itm["tags"]
    dead_it = it.get("dead_dl", 0) + it.get("dead_vertices", 0) + it.get("pad_other", 0)
    ov_it = SPAN * itm["kept_runs_A"] + ROOT * itm["decode_roots"]   # span rows + one 8 B root cell per decoded DL root
    keep_it = (it.get("keep_runtime", 0) + it.get("keep_fingerprint", 0)
               + it.get("bind_read_owned_kinds", 0) + it.get("no_owner_kinds_texture", 0)
               + it.get("unreached_texture_palette", 0) + it.get("unreached_keep_unproven", 0))
    add("B", 86, "MiscData086 (ITCommonObject)", 79584, 79584, keep_it + ov_it if False else keep_it,
        dead_it - ov_it, dead_it - ov_it + it.get("bind_read_owned_kinds", 0), "H/M",
        "B = owned-kind textures fetched from a NitroFS pack at first bind (UNPROVEN cost); "
        "12 Pokemon + weapons have no native owner (their DL/Vtx/tex are unread today)", it)
    add("B", None, "ITStruct pool (16 x 924)", inv["item_pool"], inv["item_pool"], inv["item_pool"], 0, 0, "H",
        "ITEM_ALLOC_MAX is the source's; no high-water counter exists for concurrent items")

    # --- owner C: ndsBaseEFManagerInitEffects ------------------------------
    for n, fid in (("EFCommonEffects1", 83), ("EFCommonEffects2", 84), ("EFCommonEffects3", 85)):
        e = eff[n]
        t = e["tags"]
        dead = (t.get("dead_unreferenced", 0) + t.get("dead_dl", 0) + t.get("dead_vertices", 0)
                + t.get("dead_texture", 0) + t.get("pad_other", 0) + t.get("identity_only", 0))
        # span rows (upper bound: one per kept run) + identity stubs for static-corpus
        # keys + one 8 B ENDDL/source-offset cell per reachable DObj display-list
        # root (lane2_roots.py) so every dobj->dl stays a valid, recognisable address
        roots = J("lane2_roots.json")[n]["dl_roots_reachable"]
        ov = SPAN * e["kept_runs_A"] + CELL * e["identity_cells"] + ROOT * roots
        keep = t.get("keep_runtime", 0) + t.get("keep_fingerprint", 0)
        add("C", fid, n, e["payload"], e["payload"], keep + t.get("prepare_only", 0),
            dead - ov, dead - ov + t.get("prepare_only", 0), {"EFCommonEffects1": "H/M", "EFCommonEffects2": "H/H", "EFCommonEffects3": "H/H"}[n],
            "unreferenced island / DL / Vtx / textures with no DS reader", t)
    add("C", None, "EFStruct pool (38 x 60)", inv["ef_pool"], inv["ef_pool"], inv["ef_pool"], 0, 0, "H",
        "measured FreeMin 0 of 38 in three four-kind runs (pool saturates): cannot shrink")
    add("C", None, "visual templates (7 x 352)", inv["ef_templates"], inv["ef_templates"], inv["ef_templates"],
        0, 0, "L/L", "procedural Task-39 stand-ins; reader coverage not audited")
    pp = inv["particle_pools"]
    add("C", None, "particle pools (efParticleInitAll)", pp["bytes"], pp["bytes"], pp["bytes"], 0, 0, "H",
        "measured max 53/15/36 of 112/24/80 over 30 runs; see pools table")
    total = {}
    for k in ("payload", "resident_now", "keep", "A", "B"):
        total[k] = sum(r[k] for r in rows)
    # levers outside A/B: they change a constant or add a gate rather than a reader
    import json as _j
    sizes = _j.loads((HERE / "lane2_struct_sizes.json").read_text())
    pool_now = {"structs": 112, "generators": 24, "transforms": 80}
    pool_new = {"structs": 64, "generators": 20, "transforms": 48}   # measured max 53/15/36 + ~20-33 % margin
    right = (pool_now["structs"] - pool_new["structs"]) * sizes["LBParticle"]         + (pool_now["generators"] - pool_new["generators"]) * sizes["LBGenerator"]         + (pool_now["transforms"] - pool_new["transforms"]) * sizes["LBTransform"]
    item_gate = inv["item_tree"] + inv["ifcommonitem"] + inv["item_pool"]
    levers = [
        {"name": "particle pools right-sized 112/24/80 -> 64/20/48", "bytes": right, "conf": "M",
         "note": "measured max 53/15/36 in 30 runs; KO burst needs a transform (silent missing effect if short)"},
        {"name": "skip item files + pool when no item can spawn", "bytes": item_gate, "conf": "L",
         "note": "MiscData086 is also an extern of Yoshi's main file (slot 0x40 -> 0x5458, lane2_xrefs.py); ITCommonData is read by the thrown Master Ball effect (Pikachu/Jigglypuff), stage bumpers and the Poke Ball; UNPROVEN consumer census"},
    ]
    need = (570000, 700000)
    total["A_pct_of_need"] = [round(100.0 * total["A"] / n_, 1) for n_ in need]
    total["B_pct_of_need"] = [round(100.0 * total["B"] / n_, 1) for n_ in need]
    total["levers"] = levers
    print(f"{'owner':5s} {'fid':>4s} {'name':38s} {'payload':>8s} {'resident':>8s} {'keep':>8s} {'A':>8s} {'B':>8s} conf")
    for r in rows:
        print(f"{r['owner']:5s} {str(r['fid'] or ''):>4s} {r['name']:38s} {r['payload']:8d} {r['resident_now']:8d} "
              f"{r['keep']:8d} {r['A']:8d} {r['B']:8d} {r['conf']}")
    print("TOTAL", {k: v for k, v in total.items() if k != "levers"})
    for l in levers:
        print("LEVER", l["name"], l["bytes"], l["conf"])
    (HERE / "lane2_summary.json").write_text(json.dumps({"rows": rows, "total": total}, indent=1))


if __name__ == "__main__":
    main()
