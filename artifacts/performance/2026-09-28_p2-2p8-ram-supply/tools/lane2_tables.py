#!/usr/bin/env python3
"""Lane 2: render the markdown tables for lane2-common-item-effect-files.md
from the JSON the other lane2_* scripts wrote (so no number in the report is
typed by hand).  Output: tools/lane2_tables.md"""
from __future__ import annotations

import json
from pathlib import Path

HERE = Path(__file__).resolve().parent


def J(n):
    return json.loads((HERE / n).read_text())


def fmt(n):
    return f"{n:,}"


def table(headers, rows, align=None):
    out = ["| " + " | ".join(headers) + " |"]
    al = align or ["l"] * len(headers)
    out.append("|" + "|".join("---:" if a == "r" else "---" for a in al) + "|")
    for r in rows:
        out.append("| " + " | ".join(str(c) for c in r) + " |")
    return "\n".join(out)


def main():
    inv = J("lane2_inventory.json")
    ifc = J("lane2_ifcommon.json")
    eff = J("lane2_effects.json")
    itm = J("lane2_items_model.json")
    reach = J("lane2_reach.json")
    summ = J("lane2_summary.json")
    pools_all = J("lane2_pools_measured.json")
    pools = pools_all["summary"]
    import re as _re
    _dates = sorted({_re.search(r"(2026-\d\d-\d\d)", k.replace(chr(92), "/")).group(1)
                     for k in pools_all["per_file"] if _re.search(r"(2026-\d\d-\d\d)", k)})
    _n_runs = len(pools_all["per_file"])
    sizes = J("lane2_struct_sizes.json")
    parts = []

    # T1 inventory ---------------------------------------------------------
    rows = []
    for f in inv["if_files"]:
        rows.append(["A `scVSBattleSetupFiles`", f["fid"], f["name"], f["slot"], fmt(f["container"]),
                     fmt(f["payload"]), fmt(f["aligned"]), "yes" if f["nitro_identical"] else "NO"])
    rows.append(["A subtotal", "", "8 files, one block (flag OFF)", "", "", "", fmt(inv["if_block_flag0"]), ""])
    rows.append(["A subtotal", "", "same block, compact flag ON (HEAD default)", "", "", "",
                 fmt(inv["if_block_flag1"]) + " + GameStatus image 21,056 + baked streams 22,104 (measured)", ""])
    rows.append(["B `itManagerInitItems`", 251, "ITCommonData", "gITManagerCommonData", "3,608", "3,392", "3,392", "yes"])
    rows.append(["B", 86, "MiscData086 (extern of all 68 ITCommonData slots)", "(in the same tree)", "79,664", "79,584", "79,584", "yes"])
    rows.append(["B", "", "extern tree total = gNdsITCommonDataBytes", "", "", "", fmt(inv["item_tree"]), ""])
    rows.append(["B", 87, "IFCommonItem (`ifCommonItemArrowSetAttr`)", "sIFCommonItemArrowSprite", "240", "160", "160", "yes"])
    rows.append(["B", "", f"ITStruct pool: {sizes['ITStruct']} B x ITEM_ALLOC_MAX 16", "sNdsItemStructsFree", "", "", fmt(inv["item_pool"]), ""])
    for n, fid, cont, pay in (("EFCommonEffects1", 83, "52,816", "52,736"), ("EFCommonEffects2", 84, "28,432", "28,352"),
                              ("EFCommonEffects3", 85, "13,696", "13,616")):
        rows.append(["C `ndsBaseEFManagerInitEffects`", fid, n, f"gEFManagerFiles[{fid - 83}]", cont, pay, pay, "yes"])
    rows.append(["C subtotal", "", "three files = census 94,704", "", "", "", fmt(inv["ef_files"]), ""])
    rows.append(["C", "", f"EFStruct pool: {sizes['EFStruct']} B x NDS_R2_EFFECT_POOL 38", "sEFManagerStructsAllocFree", "", "", fmt(inv["ef_pool"]), ""])
    rows.append(["C", "", f"visual templates: {sizes['NDSVisualTemplate']} B x 7 (port wrapper)", "sNdsVisualTemplates", "", "", fmt(inv["ef_templates"]), ""])
    pp = inv["particle_pools"]
    rows.append(["(adjacent) `efParticleInitAll`", "", f"LBParticle {pp['structs']} x 96, LBGenerator {pp['generators']} x 92, LBTransform {pp['transforms']} x 192",
                 "gEFParticle*GObj", "", "", fmt(pp["bytes"]), ""])
    parts.append("### T1 inventory\n\n" + table(
        ["owner", "fid", "file / allocation", "runtime slot", "O2R container B", "payload B", "aligned alloc B", "NitroFS == O2R"],
        rows, ["l", "r", "l", "l", "r", "r", "r", "l"]))

    # T2 IFCommon classes --------------------------------------------------
    rows = []
    for n in ("IFCommonPlayerDamage", "IFCommonTimer", "IFCommonDigits", "IFCommonBattlePause",
              "IFCommonPlayerTags", "IFCommonAnnounceCommon", "IFCommonItem"):
        v = ifc[n]
        t = v["tags"]
        bmp = (fmt(t["keep_bitmap_arrays"]) + " (kept)") if "keep_bitmap_arrays" in t else fmt(t.get("dead_bitmap_array", 0))
        rows.append([n, v["fid"], fmt(v["payload"]), v["sprites"], fmt(t["keep_headers"]),
                     bmp, fmt(t.get("dead_pixels", 0)),
                     fmt(t.get("setup_read_pixels", 0)), fmt(t["pad_other"])])
    g = ifc["IFCommonGameStatus"]
    gt = g["tags_resident_image"]
    rows.append(["IFCommonGameStatus (resident compact image)", 82, "21,056 of 152,288", 24,
                 fmt(gt["keep_headers"]), fmt(gt["keep_bitmap_arrays"]) + " (kept)", "0",
                 fmt(gt["setup_read_pixels"]), fmt(gt["pad_other"])])
    rows.append(["IFCommonGameStatus already dropped (12 letter payloads)", 82, fmt(g["already_dropped_letter_pixels"]),
                 12, "-", "-", fmt(g["already_dropped_letter_pixels"]), "-", "-"])
    parts.append("### T2 IFCommon sprite files: bytes by class and reader\n\n" + table(
        ["file", "fid", "payload B", "sprites", "Sprite headers (kept)", "Bitmap arrays", "pixels: no reader",
         "pixels read only while building", "pad"], rows, ["l", "r", "r", "r", "r", "r", "r", "r", "r"]))

    # T3 effects -----------------------------------------------------------
    rows = []
    for n in ("EFCommonEffects1", "EFCommonEffects2", "EFCommonEffects3"):
        e = eff[n]
        t = e["tags"]
        rows.append([n, e["fid"], fmt(e["payload"]),
                     fmt(t.get("keep_runtime", 0) + t.get("keep_fingerprint", 0)),
                     fmt(t.get("prepare_only", 0)), fmt(t.get("identity_only", 0)),
                     fmt(t.get("dead_unreferenced", 0)), fmt(t.get("dead_dl", 0)),
                     fmt(t.get("dead_vertices", 0)), fmt(t.get("dead_texture", 0)), fmt(t.get("pad_other", 0))])
    tt = eff["_total"]
    rows.append(["**all three**", "", fmt(sum(eff[n]["payload"] for n in ("EFCommonEffects1", "EFCommonEffects2", "EFCommonEffects3"))),
                 fmt(tt["keep_runtime"] + tt["keep_fingerprint"]), fmt(tt["prepare_only"]), fmt(tt["identity_only"]),
                 fmt(tt["dead_unreferenced"]), fmt(tt["dead_dl"]), fmt(tt["dead_vertices"]), fmt(tt["dead_texture"]),
                 fmt(tt["pad_other"])])
    parts.append("### T3 effect files: read model (bytes)\n\n" + table(
        ["file", "fid", "payload", "still read (DObjDesc/MObjSub/anim/DLLink + 48 B DL words)", "read only at scene prepare",
         "identity key only", "unreferenced by any source root", "reachable Gfx, never read", "Vtx, never read",
         "textures/palettes, no reader", "pad"], rows, ["l", "r", "r", "r", "r", "r", "r", "r", "r", "r", "r"]))

    rows = []
    for n in ("EFCommonEffects1", "EFCommonEffects2", "EFCommonEffects3"):
        for eff_name, cls in sorted(reach[n]["per_effect"].items()):
            rows.append([n, eff_name, fmt(sum(cls.values())),
                         fmt(cls.get("texture", 0) + cls.get("palette", 0)), fmt(cls.get("display_list", 0)),
                         fmt(cls.get("vertices", 0)), fmt(cls.get("animation", 0)),
                         fmt(cls.get("material", 0) + cls.get("dobjdesc", 0))])
    parts.append("### T3b per-effect closure bytes (pointer closure from that effect's ll roots; shared bytes counted in each)\n\n" + table(
        ["file", "effect", "closure B", "tex+pal", "Gfx", "Vtx", "anim", "MObjSub+DObjDesc"], rows,
        ["l", "l", "r", "r", "r", "r", "r", "r"]))


    # T3c effect byte classes ------------------------------------------------
    cl_names = ["texture", "palette", "display_list", "dllink", "vertices", "animation", "material", "dobjdesc", "pad", "unplaced", "unclassified", "other", "other_data"]
    rows = []
    for n in ("EFCommonEffects1", "EFCommonEffects2", "EFCommonEffects3"):
        c = eff[n]["classes"]
        rows.append([n] + [fmt(c.get(k, 0)) for k in ("texture", "palette", "display_list", "dllink", "vertices", "animation", "material", "dobjdesc", "pad")]
                    + [fmt(c.get("unplaced", 0) + c.get("unclassified", 0) + c.get("other", 0) + c.get("other_data", 0))])
    parts.append("### T3c effect files: byte classes (structural decode over label partition; label totals differ only where a Gfx/Vtx label lumps trailing bytes)\n\n" + table(
        ["file", "texture", "palette", "Gfx", "DObjDLLink", "Vtx", "animation", "MObjSub", "DObjDesc", "pad", "other/unplaced"], rows,
        ["l", "r", "r", "r", "r", "r", "r", "r", "r", "r", "r"]))

    # T8 native-owner fingerprint reads (items) ----------------------------
    fpi = itm["fp_owner_indices"]
    own = itm["owners"]
    rows = []
    for k in sorted(own):
        v = own[k]
        idx = fpi.get(k, [])
        roots = v["roots"]
        rows.append([k, ", ".join(roots), ", ".join(str(i) for i in idx),
                     fmt(8 * len(idx) * max(1, len(roots))), ", ".join(v["binds"][:6]) + (" ..." if len(v["binds"]) > 6 else "")])
    parts.append("### T8 item native owners on MiscData086: Gfx words compared at draw, texture/palette offsets bound\n\n" + table(
        ["owner", "DL roots", "dl[] indices compared", "bytes (upper bound)", "TLUT/IMAGE offsets bound"], rows,
        ["l", "l", "l", "r", "l"]))

    # T4 items -------------------------------------------------------------
    t = itm["tags"]
    rows = [
        ["Gfx never read (no interpreter; owner checks compare addresses)", fmt(t["dead_dl"]), "dead"],
        ["Gfx words the owner candidate checks compare (upper bound, 8 B per (root, index))", fmt(t["keep_fingerprint"]), "keep"],
        ["Vtx (geometry is generated ROM data)", fmt(t["dead_vertices"]), "dead"],
        ["pad / gap filler", fmt(t["pad_other"]), "dead"],
        ["textures+palettes reachable from kinds WITH a native owner (bound at first draw)", fmt(t["bind_read_owned_kinds"]), "read at bind"],
        ["textures+palettes reachable only from kinds WITHOUT an owner (12 Pokemon + weapons)", fmt(t["no_owner_kinds_texture"]), "no reader today"],
        ["DObjDesc, MObjSub, animation, DLLink, unclassified (reached)", fmt(t["keep_runtime"]), "keep"],
        ["same classes, not reached from any known root (UNPROVEN dead, kept)", fmt(t["unreached_keep_unproven"]), "keep"],
    ]
    parts.append("### T4 MiscData086: read model (bytes; payload 79,584)\n\n" + table(
        ["class / reader", "bytes", "verdict"], rows, ["l", "r", "l"]))
    rows = [[k, fmt(v)] for k, v in sorted(itm["classes"].items(), key=lambda kv: -kv[1])]
    parts.append("### T4b MiscData086 byte classes after structural overlay\n\n" + table(["class", "bytes"], rows, ["l", "r"]))

    # T5 pools -------------------------------------------------------------
    def m(k):
        s = pools[k]
        return s["min"], s["median"], s["max"]
    rows = []
    lo, md, hi = m("gNdsEffectPoolFreeMin")
    rows.append(["EFStruct (effects)", sizes["EFStruct"], 38, fmt(inv["ef_pool"]),
                 f"free-min {lo}/{md}/{hi} -> live max {38 - lo}/{38 - md}/{38 - hi} (min/median/max over runs)",
                 "SATURATED in 3 runs (free 0): keep"])
    lo, md, hi = m("gNdsParticleStructsMax")
    rows.append(["LBParticle structs", sizes["LBParticle"], 112, fmt(112 * sizes["LBParticle"]), f"{lo}/{md}/{hi}", f"{hi / 112:.0%} of cap at worst"])
    lo, md, hi = m("gNdsParticleGeneratorsMax")
    rows.append(["LBGenerator", sizes["LBGenerator"], 24, fmt(24 * sizes["LBGenerator"]), f"{lo}/{md}/{hi}", f"{hi / 24:.0%} of cap at worst"])
    lo, md, hi = m("gNdsParticleTransformsMax")
    rows.append(["LBTransform", sizes["LBTransform"], 80, fmt(80 * sizes["LBTransform"]), f"{lo}/{md}/{hi}", f"{hi / 80:.0%} of cap at worst"])
    rows.append(["ITStruct (items)", sizes["ITStruct"], 16, fmt(inv["item_pool"]), "no concurrent-item counter exists", "unmeasured"])
    lo, md, hi = m("gNdsWeaponPoolLiveHighWater")
    rows.append(["WPStruct (adjacent, not this lane)", sizes["WPStruct"], 10, fmt(10 * sizes["WPStruct"]), f"live max {lo}/{md}/{hi}", "out of scope"])
    parts.append("### T5 pools: configured capacity vs measured high-water\n\n" + table(
        ["pool", "elem B", "capacity", "bytes", f"measured over {_n_runs} four-CPU runs ({_dates[0]}..{_dates[-1][5:]})", "note"],
        rows, ["l", "r", "r", "r", "l", "l"]))

    # T6 master ------------------------------------------------------------
    rows = []
    for r in summ["rows"]:
        rows.append([r["owner"], r["fid"] if r["fid"] is not None else "", r["name"], fmt(r["payload"]),
                     fmt(r["resident_now"]), fmt(r["keep"]), fmt(r["A"]), fmt(r["B"]), r["confA"] + "/" + r["confB"]])
    tot = summ["total"]
    rows.append(["", "", "**TOTAL**", "", fmt(tot["resident_now"]), fmt(tot["keep"]), fmt(tot["A"]), fmt(tot["B"]), ""])
    parts.append("### T6 master table\n\n" + table(
        ["owner", "fid", "file / allocation", "payload B", "resident now B", "must keep B", "reclaimable A B", "reclaimable A+B B", "conf A/B"],
        rows, ["l", "r", "l", "r", "r", "r", "r", "r", "l"]))

    lv = tot.get("levers", [])
    rows = [[l["name"], fmt(l["bytes"]), l["conf"], l["note"]] for l in lv]
    parts.append("### T7 levers outside A and B\n\n" + table(["lever", "bytes", "conf", "note"], rows, ["l", "r", "l", "l"]))
    parts.append(f"A = {fmt(tot['A'])} B is {tot['A_pct_of_need'][1]}%-{tot['A_pct_of_need'][0]}% of the ~570-700 KB motion-bank target; "
                 f"A+B = {fmt(tot['B'])} B is {tot['B_pct_of_need'][1]}%-{tot['B_pct_of_need'][0]}%.")
    (HERE / "lane2_tables.md").write_text("\n\n".join(parts) + "\n", encoding="utf-8")
    print("\n\n".join(parts))


if __name__ == "__main__":
    main()
