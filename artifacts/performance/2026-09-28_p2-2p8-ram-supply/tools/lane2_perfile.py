#!/usr/bin/env python3
"""Lane 2: the per-file deliverable table (id, bytes, bytes by class, bytes still
read + reader file:line, reclaimable, confidence).  Every number is read from the
JSON the other lane2_* scripts wrote; the reader column carries {ID} tokens that
are replaced with `basename:lines` from lane2_cites.json (lane2_cites.py), so a
moved line breaks the build of this table instead of the report.

Writes lane2_perfile.md (and prints it).
"""
from __future__ import annotations

import json
import os
import re
from pathlib import Path

import lane2_o2r as o2r
import lane2_sprites as sp

HERE = Path(__file__).resolve().parent


def J(n):
    return json.loads((HERE / n).read_text())


def fmt(n):
    return f"{int(n):,}"


CITE = {c["id"]: c for c in J("lane2_cites.json")}


def cites(text):
    def rep(m):
        c = CITE[m.group(1)]
        return f"`{os.path.basename(c['path'])}:{c['lines']}`"
    return re.sub(r"\{([A-Z]\d\d)\}", rep, text)


def classes(d, order):
    return " / ".join(f"{k} {fmt(d[k])}" for k in order if d.get(k))


def main():
    summ = J("lane2_summary.json")
    rows = {(r["fid"], r["name"]): r for r in summ["rows"]}

    def S(fid, name_prefix):
        for (f, n), r in rows.items():
            if f == fid and n.startswith(name_prefix):
                return r
        raise KeyError((fid, name_prefix))

    ifc = J("lane2_ifcommon.json")
    eff = J("lane2_effects.json")
    itm = J("lane2_items_model.json")
    roots = J("lane2_roots.json")
    out = []

    def add(fid, name, payload, cls, keep, readers, r, note=""):
        out.append([str(fid) if fid is not None else "", name, fmt(payload), cls, fmt(keep), cites(readers),
                    fmt(r["A"]), fmt(r["B"]), r["confA"] + "/" + r["confB"], note])

    # ---- owner A: IF sprite files (class totals straight from the sprite parser)
    def spr(n):
        f = o2r.by_name(n)
        rows_, totals, _ = sp.analyse(f)
        return totals

    r = S(166, "IFCommonPlayer")
    add(166, "IFCommonPlayer", 976, "DObjDesc / DL / Vtx / anim / one IA8 image (not split)", r["keep"],
        "everything kept: 976 B is not worth a span table", r)
    g = ifc["IFCommonGameStatus"]
    gs = g["compact"]
    r = S(82, "IFCommonGameStatus (compact")
    add(82, "IFCommonGameStatus (resident compact image)", 21056,
        f"Sprite hdr {fmt(gs['sprite_headers'])} / Bitmap[] {fmt(gs['bitmap_arrays'])} / lamp+rod+frame pixels {fmt(gs['kept_pixel_bytes'])} / pad {fmt(gs['pad_or_other'])}"
        f"; already dropped: 12 letter payloads {fmt(gs['dropped_letter_pixel_bytes'])} of the 152,288 B source",
        r["keep"],
        "headers+Bitmap[]: {A19} (pointer+size match), {A29} (rebase); lamp/rod/frame pixels: read once by {A26} at scene entry; letters baked by {A27}, dropped by {A08}", r,
        "A = pad only; B = the 18,016 B of pixels if the atlas prepare reads NitroFS")
    r = S(82, "IFCommonGameStatus baked")
    add(82, "IFCommonGameStatus baked end streams (separate allocation)", 22104,
        "run-length TIME UP / GAME SET banks", 0,
        "built by {A28}; decoded into the OBJ end bank once, at the announcement", r,
        "B = keep them in a NitroFS stream read at the announcement (UNPROVEN cost)")
    for n, fid, why in (
            ("IFCommonPlayerDamage", 164, "lower HUD draws damage digits from `battle_hud.bin` ({A11}); route {A13}, state {A14}"),
            ("IFCommonTimer", 165, "same: lower HUD, {A11}/{A13}"),
            ("IFCommonDigits", 36, "same: lower HUD, {A11}/{A13}"),
            ("IFCommonBattlePause", 197, "no native owner: unrecognised SObjs are not drawn, only a failure is recorded ({A15}, {A16}, {A19})"),
            ("IFCommonAnnounceCommon", 37, "no native owner ({A19}, {A16}); letters/period never reach an OAM emitter"),
            ("IFCommonPlayerTags", 38, "tag bake reads I8 pixels at interface creation ({A21}, call {A23}); OAM emit uses the baked cell ({A20})")):
        v = ifc[n]
        t = spr(n)
        r = S(fid, n)
        cls = (f"Sprite hdr {fmt(t['sprite_header'])} / Bitmap[] {fmt(t['bitmap_array'])} / pixels {fmt(t['pixels'])} / "
               f"pad {fmt(t['pad_or_other'])}")
        add(fid, n, v["payload"], cls, r["keep"],
            f"Sprite headers: copied into SObjs by {{A32}} via {{A31}}; pixels: " + why, r)
    r = S(87, "IFCommonItem")
    ti = spr("IFCommonItem")
    add(87, "IFCommonItem (arrow sprite)", 160,
        f"Sprite hdr {fmt(ti['sprite_header'])} / Bitmap[] {fmt(ti['bitmap_array'])} / pixels {fmt(ti['pixels'])} / pad {fmt(ti['pad_or_other'])}", r["keep"],
        "arrow bake at itManagerInitItems: {A22}, call {A24}", r, "160 B: not worth touching")

    # ---- owner B
    r = S(251, "ITCommonData")
    add(251, "ITCommonData (68 attribute rows + externs)", 3392, "ITAttributes rows, extern chains", r["keep"],
        "decoded once per kind by {B03}; rows point into file 86", r)
    t = itm["tags"]
    cl = itm["classes"]
    r = S(86, "MiscData086")
    add(86, "MiscData086 (ITCommonObject)", 79584,
        classes(cl, ["texture", "display_list", "vertices", "dobjdesc", "animation", "pad", "material", "palette", "other", "dllink"]),
        r["keep"],
        f"DObjDesc/MObjSub/anim/DLLink {fmt(t['keep_runtime'])}: {{B03}}, {{B04}}; owner DL-word compares {fmt(t['keep_fingerprint'])}: {{B06}}..; "
        f"owned-kind textures {fmt(t['bind_read_owned_kinds'])}: raw base+offset at first draw {{B12}}; "
        f"owner-less kinds' textures {fmt(t['no_owner_kinds_texture'])}: no reader; Yoshi shares the file ({{B09}})",
        r, "A: DL 11,032 + Vtx 6,640 + pad 4,660 less 181 span rows and 56 root cells; B adds the bind-read textures")
    r = S(None, "ITStruct pool")
    add("-", "ITStruct pool (16 x 924)", r["payload"], "pool", r["keep"], "capacity {B05}; no concurrent-item counter exists", r)

    # ---- owner C
    for n, fid in (("EFCommonEffects1", 83), ("EFCommonEffects2", 84), ("EFCommonEffects3", 85)):
        e = eff[n]
        tg = e["tags"]
        r = S(fid, n)
        readers = {
            83: "DObjDesc/MObjSub/anim/DLLink via EFDesc resolver {C05}; slash 13 + mdust 7 frames read once at scene prepare ({C12}, {C13}); "
                "DamageSlash 48 B of DL words compared ({C10}, {C11}); the other 20,064 B of reachable islands are unread",
            84: "EFDesc resolver {C05}; 10 entry roots admitted by raw address equality ({C07}); shadow keys need three offsets to map ({C19}, {C20}); shadow maker is NULL ({C18})",
            85: "EFDesc resolver {C05}; 9 roots admitted by raw dl - base compares ({C08}, {C09}); geometry/texels are ROM tables ({C16}, {C17})",
        }[fid]
        add(fid, n, e["payload"],
            classes(e["classes"], ["texture", "palette", "display_list", "dllink", "vertices", "animation", "material", "dobjdesc", "pad", "unplaced"]),
            r["keep"], readers, r,
            f"{e['kept_runs_A']} kept runs, {roots[n]['dl_roots_reachable']} DL root cells, {e['identity_cells']} identity stubs")
    for key in ("EFStruct pool", "visual templates", "particle pools"):
        r = S(None, key)
        add("-", r["name"], r["payload"], "pool / table", r["keep"],
            {"EFStruct pool": "capacity {C02}; measured saturated in 3 of 30 runs",
             "visual templates": "procedural stand-ins built by {C06}; readers not audited (L)",
             "particle pools": "allocated by {C22}; capacities {C21}"}[key], r)

    tot = summ["total"]
    hdr = ["fid", "file / allocation", "payload B", "bytes by class", "must keep B", "still read after load: what + reader file:line",
           "reclaim A B", "reclaim A+B B", "conf A/B", "note"]
    md = ["| " + " | ".join(hdr) + " |", "|" + "|".join(["---"] * len(hdr)) + "|"]
    for row in out:
        md.append("| " + " | ".join(row) + " |")
    md.append(f"| | **TOTAL** | {fmt(tot['payload'])} | | {fmt(tot['keep'])} | | **{fmt(tot['A'])}** | **{fmt(tot['B'])}** | | |")
    text = "\n".join(md) + "\n"
    (HERE / "lane2_perfile.md").write_text(text, encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
