#!/usr/bin/env python3
"""Lane 2: assemble lane2-common-item-effect-files.md from the template and the
JSON/markdown the other lane2_* scripts wrote.  No number in the report is typed
by hand where a script produces it:

  {{v:key}}        a value from V (comma-formatted)      {{raw:key}}  unformatted
  {{TABLE:name}}   a table from lane2_tables.md (T1..T8) or lane2_perfile.md
  [[ID]]           a citation from lane2_cites.json -> `path:lines`
  {{CITES}}        the whole citation ledger

Usage: python lane2_build_report.py   (after lane2_run_all.sh)
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
OUT = HERE.parent / "lane2-common-item-effect-files.md"


def J(n):
    return json.loads((HERE / n).read_text())


def fmt(n):
    return f"{int(n):,}"


def main():
    summ = J("lane2_summary.json")
    rows = summ["rows"]
    tot = summ["total"]
    ifc = J("lane2_ifcommon.json")
    eff = J("lane2_effects.json")
    itm = J("lane2_items_model.json")
    inv = J("lane2_inventory.json")
    roots = J("lane2_roots.json")
    prefix = J("lane2_prefix.json")
    sites = J("lane2_sites.json")
    spr = J("lane2_sprites_counts.json")
    sizes = J("lane2_struct_sizes.json")
    pools = J("lane2_pools_measured.json")["summary"]
    cites = {c["id"]: c for c in J("lane2_cites.json")}

    def R(fid, prefix_):
        for r in rows:
            if r["fid"] == fid and r["name"].startswith(prefix_):
                return r
        raise KeyError((fid, prefix_))

    V = {}
    V["resident"] = tot["resident_now"]
    V["keep"] = tot["keep"]
    V["A"] = tot["A"]
    V["B"] = tot["B"]
    V["Bx"] = tot["B"] - tot["A"]
    V["payload_all"] = tot["payload"]
    lo, hi = 570000, 700000
    V["A_pct_hi"] = f"{100.0 * tot['A'] / lo:.1f}"
    V["A_pct_lo"] = f"{100.0 * tot['A'] / hi:.1f}"
    V["B_pct_hi"] = f"{100.0 * tot['B'] / lo:.1f}"
    V["B_pct_lo"] = f"{100.0 * tot['B'] / hi:.1f}"
    V["bank_lo"], V["bank_hi"] = lo, hi
    lev = {l["name"]: l for l in tot["levers"]}
    V["lever_pool"] = [l for l in tot["levers"] if l["name"].startswith("particle")][0]["bytes"]
    V["lever_items"] = [l for l in tot["levers"] if l["name"].startswith("skip item")][0]["bytes"]
    V["lever_pool_pct_hi"] = f"{100.0 * V['lever_pool'] / lo:.1f}"
    V["lever_pool_pct_lo"] = f"{100.0 * V['lever_pool'] / hi:.1f}"

    if_five = [R(164, "IFCommonPlayerDamage"), R(165, "IFCommonTimer"), R(36, "IFCommonDigits"),
               R(197, "IFCommonBattlePause"), R(37, "IFCommonAnnounceCommon")]
    V["if_A"] = sum(r["A"] for r in if_five)
    V["if_A_H"] = sum(r["A"] for r in if_five if r["confA"] == "H")
    V["if_A_M"] = sum(r["A"] for r in if_five if r["confA"] == "M")
    V["announce_A"] = R(37, "IFCommonAnnounceCommon")["A"]
    V["pause_A"] = R(197, "IFCommonBattlePause")["A"]
    V["tags_B"] = R(38, "IFCommonPlayerTags")["B"]
    V["gs_A"] = R(82, "IFCommonGameStatus (compact")["A"]
    V["gs_B"] = R(82, "IFCommonGameStatus (compact")["B"]
    V["streams"] = R(82, "IFCommonGameStatus baked")["B"]
    V["gs_px"] = V["gs_B"] - V["gs_A"]
    ef = [R(83, "EFCommonEffects1"), R(84, "EFCommonEffects2"), R(85, "EFCommonEffects3")]
    V["ef1_A"], V["ef2_A"], V["ef3_A"] = (r["A"] for r in ef)
    V["ef1_B"] = ef[0]["B"]
    V["ef_A"] = sum(r["A"] for r in ef)
    V["ef_Bx"] = sum(r["B"] - r["A"] for r in ef)
    V["ef23_A"] = V["ef2_A"] + V["ef3_A"]
    it = R(86, "MiscData086")
    V["it_A"] = it["A"]
    V["it_B"] = it["B"]
    V["it_Bx"] = it["B"] - it["A"]
    V["it_keep"] = it["keep"]
    V["A_H"] = V["if_A_H"] + V["ef_A"] + V["gs_A"] + V["it_A"]
    V["A_M"] = V["if_A_M"]
    assert V["A_H"] + V["A_M"] == V["A"], (V["A_H"], V["A_M"], V["A"])
    assert V["if_A"] + V["ef_A"] + V["gs_A"] + V["it_A"] == V["A"]
    assert V["Bx"] == V["tags_B"] + V["gs_px"] + V["streams"] + V["ef_Bx"] + V["it_Bx"], "B increments do not add up"
    V["wave1"] = V["ef23_A"] + V["if_A"]
    V["wave1_pct_hi"] = f"{100.0 * V['wave1'] / lo:.1f}"
    V["wave1_pct_lo"] = f"{100.0 * V['wave1'] / hi:.1f}"
    V["wave2"] = V["wave1"] + V["ef1_A"]
    V["wave2_share"] = f"{100.0 * V['wave2'] / V['A']:.0f}"
    # overhead accounting (must equal gross dead - A per family)
    span, cell, ident = 12, 8, 28
    if_sprites = sum(ifc[n]["sprites"] for n in ("IFCommonPlayerDamage", "IFCommonTimer", "IFCommonDigits",
                                                  "IFCommonBattlePause", "IFCommonAnnounceCommon"))
    ef_runs = sum(eff[n]["kept_runs_A"] for n in eff if n.startswith("EF"))
    ef_cells = sum(roots[n]["dl_roots_reachable"] for n in roots)
    ef_ident = sum(eff[n]["identity_cells"] for n in eff if n.startswith("EF"))
    it_runs, it_cells = itm["kept_runs_A"], itm["decode_roots"]
    V["oh_if"] = span * if_sprites
    V["oh_gs"] = span * 24
    V["oh_ef_spans"] = span * ef_runs
    V["oh_ef_cells"] = cell * ef_cells
    V["oh_ef_ident"] = ident * ef_ident
    V["oh_it_spans"] = span * it_runs
    V["oh_it_cells"] = cell * it_cells
    V["oh_total"] = (V["oh_if"] + V["oh_gs"] + V["oh_ef_spans"] + V["oh_ef_cells"] + V["oh_ef_ident"]
                     + V["oh_it_spans"] + V["oh_it_cells"])
    V["gross_dead"] = V["A"] + V["oh_total"]
    V["n_if_sprites"] = if_sprites
    V["n_ef_runs"], V["n_ef_cells"], V["n_ef_ident"] = ef_runs, ef_cells, ef_ident
    V["n_it_runs"], V["n_it_cells"] = it_runs, it_cells
    # item numbers
    t = itm["tags"]
    V["it_dead_dl"], V["it_dead_vtx"], V["it_pad"] = t["dead_dl"], t["dead_vertices"], t["pad_other"]
    V["it_keep_rt"], V["it_keep_fp"] = t["keep_runtime"], t["keep_fingerprint"]
    V["it_bind"], V["it_noowner"], V["it_unreached"] = t["bind_read_owned_kinds"], t["no_owner_kinds_texture"], t["unreached_keep_unproven"]
    V["it_prefix_extra"] = prefix["contiguous_prefix_bytes_union"] - prefix["compared_words_bytes_union"]
    V["it_A_prefix"] = V["it_A"] - V["it_prefix_extra"]
    et = {n: eff[n]["tags"] for n in eff if n.startswith("EF")}
    V["ef_unref"] = sum(x.get("dead_unreferenced", 0) for x in et.values())
    V["ef1_unref"] = et["EFCommonEffects1"]["dead_unreferenced"]
    V["ef1_prepare"] = et["EFCommonEffects1"]["prepare_only"]
    V["ef_dead_dl"] = sum(x.get("dead_dl", 0) for x in et.values())
    V["ef_dead_vtx"] = sum(x.get("dead_vertices", 0) for x in et.values())
    V["ef_dead_tex"] = sum(x.get("dead_texture", 0) for x in et.values())
    V["ef_ident_bytes"] = sum(x.get("identity_only", 0) for x in et.values())
    V["ef_pad"] = sum(x.get("pad_other", 0) for x in et.values())
    V["ef_keep"] = sum(x.get("keep_runtime", 0) + x.get("keep_fingerprint", 0) for x in et.values())
    V["gs_dropped"] = ifc["IFCommonGameStatus"]["compact"]["dropped_letter_pixel_bytes"]
    V["gs_image"] = ifc["IFCommonGameStatus"]["compact"]["resident_image_bytes"]
    V["gs_net"] = V["gs_dropped"] - V["streams"]
    # owner totals
    V["ownerA_flag0"] = inv["if_block_flag0"]
    V["ownerA_flag1_block"] = inv["if_block_flag1"]
    V["ownerA_flag1"] = V["ownerA_flag1_block"] + V["gs_image"] + V["streams"]
    V["ownerB"] = inv["item_tree"]
    V["ownerC"] = inv["ef_files"]
    V["owners_flag0"] = inv["owner_totals_flag0"]
    for k, v in sites.items():
        V["site_" + k] = v
    for k in ("gNdsParticleStructsMax", "gNdsParticleGeneratorsMax", "gNdsParticleTransformsMax"):
        V["max_" + k] = pools[k]["max"]
        V["med_" + k] = pools[k]["median"]
    V["n_cites"] = len(cites)
    rep_txt = (HERE / "item-closure-run" / "report.txt").read_text(encoding="utf-8")
    m = re.search(r"MEASURED \(251_ITCommonData\.c row census\):\s*\n\s*(.+?)\n\s*(file is [^\n]+)", rep_txt)
    V["itcd_census"] = (m.group(1).rstrip(";") + "; " + m.group(2).rstrip(".")) if m else "(census line not found)"
    for k, v in sizes.items():
        V["sz_" + k] = v
    c4 = J("lane2_4c_check.json")
    V["n_sprites_checked"] = sum(x["sprites"] for x in c4.values())
    V["n_4c"] = sum(x["sprites_4c"] for x in c4.values())
    reach = J("lane2_reach.json")
    own = {"EFCommonEffects1": ("CommonSpark", "FlyOrbs"),
           "EFCommonEffects2": ("FireSpark", "NessPKFlash", "ShockSmall")}
    V["ownerless_tex"] = sum(reach[f]["per_effect"][e].get("texture", 0) + reach[f]["per_effect"][e].get("palette", 0)
                             for f, es in own.items() for e in es)
    V["ef_free_min0_runs"] = 3
    V["heap_free_min_lo"] = pools["gNdsTaskmanGeneralHeapFreeMin"]["min"]
    V["heap_free_min_med"] = pools["gNdsTaskmanGeneralHeapFreeMin"]["median"]
    V["heap_free_min_hi"] = pools["gNdsTaskmanGeneralHeapFreeMin"]["max"]
    V["pool_runs"] = pools["gNdsEffectPoolFreeMin"]["n"]

    # ---- tables ----------------------------------------------------------
    md = (HERE / "lane2_tables.md").read_text(encoding="utf-8")
    tables = {}
    for m in re.finditer(r"^### (T\w+)([^\n]*)\n\n(.*?)(?=\n\n### |\n\nA = |\Z)", md, re.S | re.M):
        tables[m.group(1)] = f"**{m.group(1)}{m.group(2)}**\n\n{m.group(3).rstrip()}"
    tables["PERFILE"] = (HERE / "lane2_perfile.md").read_text(encoding="utf-8").rstrip()
    # overhead table
    oh = [
        "| family | what is charged | count | bytes |",
        "|---|---|---:|---:|",
        f"| IF sprite files (5) | span row per kept Sprite header run, 12 B | {V['n_if_sprites']} | {fmt(V['oh_if'])} |",
        f"| IFCommonGameStatus | span row per kept sprite, 12 B | 24 | {fmt(V['oh_gs'])} |",
        f"| EF1-3 | span row per kept run (upper bound), 12 B | {V['n_ef_runs']} | {fmt(V['oh_ef_spans'])} |",
        f"| EF1-3 | 8 B root cell per reachable DObj display-list root | {V['n_ef_cells']} | {fmt(V['oh_ef_cells'])} |",
        f"| EF2 | 28 B stub (12 B span + 16 B bytes) per static-corpus identity key | {V['n_ef_ident']} | {fmt(V['oh_ef_ident'])} |",
        f"| MiscData086 | span row per kept run (upper bound), 12 B | {V['n_it_runs']} | {fmt(V['oh_it_spans'])} |",
        f"| MiscData086 | 8 B root cell per decoded display-list root | {V['n_it_cells']} | {fmt(V['oh_it_cells'])} |",
        f"| **total overhead** | | | **{fmt(V['oh_total'])}** |",
        f"| gross dead bytes before overhead (A + overhead) | | | {fmt(V['gross_dead'])} |",
    ]
    tables["OVERHEAD"] = "\n".join(oh)
    # citation ledger
    led = ["| id | location | why it is cited |", "|---|---|---|"]
    for cid in sorted(cites):
        c = cites[cid]
        led.append(f"| {cid} | `{c['path']}:{c['lines']}` | {c['note']} |")
    tables["CITES"] = "\n".join(led)

    # ---- reclaim by class (gross, before overhead) ---------------------------
    five = ("IFCommonPlayerDamage", "IFCommonTimer", "IFCommonDigits", "IFCommonBattlePause", "IFCommonAnnounceCommon")
    if_px = sum(ifc[n]["tags"].get("dead_pixels", 0) for n in five)
    if_bmp = sum(ifc[n]["tags"].get("dead_bitmap_array", 0) for n in five)
    if_pad = sum(ifc[n]["tags"].get("pad_other", 0) for n in five)
    gs_pad = ifc["IFCommonGameStatus"]["tags_resident_image"]["pad_other"]
    cls_rows = [
        ("pixels/texels with no reader after load", if_px, V["ef_dead_tex"] + V["ef_ident_bytes"], 0),
        ("islands no source root reaches (EF only)", 0, V["ef_unref"], 0),
        ("display lists (no interpreter linked)", 0, V["ef_dead_dl"], V["it_dead_dl"]),
        ("vertices (geometry is ROM tables)", 0, V["ef_dead_vtx"], V["it_dead_vtx"]),
        ("Bitmap[] arrays", if_bmp, 0, 0),
        ("pad / gaps", if_pad + gs_pad, V["ef_pad"], V["it_pad"]),
    ]
    ctab = ["| class of dead byte | interface files | effect files | item file | total |", "|---|---:|---:|---:|---:|"]
    gross = 0
    for name, a, b, c in cls_rows:
        ctab.append(f"| {name} | {fmt(a)} | {fmt(b)} | {fmt(c)} | {fmt(a + b + c)} |")
        gross += a + b + c
    ctab.append(f"| **gross dead** | | | | **{fmt(gross)}** |")
    ctab.append(f"| less span rows, root cells, identity stubs | | | | -{fmt(V['oh_total'])} |")
    ctab.append(f"| **A (reclaimable, no new reader)** | | | | **{fmt(gross - V['oh_total'])}** |")
    assert gross - V["oh_total"] == V["A"], (gross, V["oh_total"], V["A"])
    tables["CLASS"] = "\n".join(ctab)
    V["gross"] = gross
    V["if_px"], V["if_bmp"], V["if_pad"] = if_px, if_bmp, if_pad

    # ---- pin + working-tree check -----------------------------------------------
    import subprocess
    repo = HERE.parents[3]
    pin = (HERE / "lane2_cites_pin.txt").read_text().strip()
    V["pin"] = pin
    head = subprocess.run(["git", "-C", str(repo), "rev-parse", "--short=11", "HEAD"], capture_output=True, text=True).stdout.strip()
    V["head_now"] = head
    st = subprocess.run(["git", "-C", str(repo), "status", "--porcelain"], capture_output=True, text=True).stdout.splitlines()
    dirty = sorted({ln[3:].strip().replace(chr(92), "/") for ln in st if not ln.startswith("??")})
    cited = {c["path"] for c in cites.values()}
    tables["DIRTY"] = ", ".join(f"`{d}`" for d in dirty) if dirty else "(none)"
    tables["DIRTY_CITED"] = ", ".join(f"`{d}`" for d in dirty if d in cited) or "(none)"
    # input files the scripts read from the working tree: must equal the pinned commit
    inputs = ["src/import/battleship_efmanager_symbols.h", "src/nds/nds_ifcommon_oam.c",
              "src/port/renderer_adapter_stage.c", "src/nds/nds_renderer_native_common.c",
              "include/nds/generated", "src/nds/generated", "src/nds/nds_native_item_bat.exec.inc",
              "src/nds/nds_native_damage_slash.exec.inc", "src/nds/nds_native_damage_fly_mdust.exec.inc",
              "src/nds/nds_native_castle_bumper.exec.inc", "decomp/BattleShip-main/decomp/src/relocData",
              "scripts/items/item_memory_closure.py"]
    diff = subprocess.run(["git", "-C", str(repo), "diff", "--name-only", pin, "--"] + inputs,
                          capture_output=True, text=True).stdout.split()
    V["input_diffs"] = len(diff)
    tables["INPUT_DIFFS"] = ", ".join(f"`{d}`" for d in diff) or "none"
    if diff:
        print("WARNING: input files differ from the pinned commit:", diff)

    tpl = (HERE / "lane2_report_template.md").read_text(encoding="utf-8")

    def rv(m):
        key = m.group(2)
        if key not in V:
            raise KeyError(f"unknown value {{{{{m.group(1)}:{key}}}}}")
        v = V[key]
        if m.group(1) == "raw" or isinstance(v, str):
            return str(v)
        return fmt(v)

    tpl = re.sub(r"\{\{(v|raw):([A-Za-z0-9_]+)\}\}", rv, tpl)

    def rt(m):
        name = m.group(1)
        if name not in tables:
            raise KeyError(f"unknown table {name}")
        return tables[name]

    tpl = re.sub(r"\{\{TABLE:([A-Za-z0-9_]+)\}\}", rt, tpl)
    tpl = tpl.replace("{{CITES}}", tables["CITES"])

    def rc(m):
        cid = m.group(1)
        if cid not in cites:
            raise KeyError(f"unknown citation {cid}")
        c = cites[cid]
        return f"`{c['path']}:{c['lines']}`"

    tpl = re.sub(r"\[\[([A-Z]\d\d)\]\]", rc, tpl)
    left = re.findall(r"\{\{[^}]*\}\}", tpl)
    if left:
        print("UNRESOLVED:", left[:10])
        return 1
    OUT.write_text(tpl, encoding="utf-8")
    print(f"wrote {OUT} ({len(tpl.splitlines())} lines, {len(tpl)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
