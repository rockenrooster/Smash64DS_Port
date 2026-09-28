#!/usr/bin/env python3
"""Lane 2 inventory: every file / allocation behind scVSBattleSetupFiles,
itManagerInitItems and ndsBaseEFManagerInitEffects in a VS battle of the
shipping configuration (builds/build-fp-argmax config; HEAD default
NDS_IF_GAMESTATUS_COMPACT ?= 1).

All sizes are computed, none typed in:
  * payload sizes from the O2R containers (lane2_o2r.load);
  * allocation sizes with the loader's own rule: NDS_RELOC_ALIGN(x) = 16-byte
    round-up (src/port/reloc_backend_assets.c:802-804), the IFCommon block via
    lbRelocGetAllocSize (:15943-15975) and the extern tree via
    ndsRelocExternTreeAllocSize (:11579-11615);
  * struct sizes from lane2_struct_sizes.json (DWARF of the shipping ELF);
  * NitroFS byte identity (builds/build-fp-argmax/nitrofs/reloc) so the O2R
    source analysis provably describes what the DS loads.
"""
from __future__ import annotations

import filecmp
import json
from pathlib import Path

import lane2_o2r as o2r

HERE = Path(__file__).resolve().parent
SIZES = json.loads((HERE / "lane2_struct_sizes.json").read_text())
ALIGN = 16


def al(n: int) -> int:
    return (n + ALIGN - 1) // ALIGN * ALIGN


IF_FILES = [  # dGMCommonFileIDs order (decomp gm/gmcommon.c:11-21)
    ("IFCommonPlayer", "gGMCommonFiles[0]"),
    ("IFCommonGameStatus", "gGMCommonFiles[1]"),
    ("IFCommonPlayerDamage", "gGMCommonFiles[2]"),
    ("IFCommonTimer", "gGMCommonFiles[3]"),
    ("IFCommonDigits", "gGMCommonFiles[4]"),
    ("IFCommonBattlePause", "gGMCommonFiles[5]"),
    ("IFCommonPlayerTags", "gGMCommonFiles[6]"),
    ("IFCommonAnnounceCommon", "gGMCommonFiles[7]"),
]


def nitro_identical(f: o2r.O2R) -> bool:
    rel = f.path.relative_to(o2r.O2R_ROOT)
    n = o2r.NITRO_ROOT / rel
    return n.is_file() and filecmp.cmp(f.path, n, shallow=False)


def if_block(compact: bool):
    total = 0
    rows = []
    for name, slot in IF_FILES:
        f = o2r.by_name(name)
        size = al(f.data_size)
        if compact and name == "IFCommonGameStatus":
            size = 0  # lbRelocGetAllocSize: placeholder sizeof(uintptr_t)
        total = al(total)
        add = size if size else 4
        rows.append((name, slot, f.file_id, f.path.name, f.container_size,
                     f.data_size, size, total, nitro_identical(f)))
        total += add
    return rows, total


def main():
    res = {}
    print("== Owner A: scVSBattleSetupFiles -> lbRelocLoadFilesListed(dGMCommonFileIDs)")
    rows0, total0 = if_block(False)
    rows1, total1 = if_block(True)
    print(f"{'file':24s} {'slot':18s} fid  container payload aligned  nitro==o2r")
    for r in rows0:
        print(f"{r[0]:24s} {r[1]:18s} {r[2]:4d} {r[4]:9d} {r[5]:7d} {r[6]:7d}  {r[8]}")
    print(f"block, compact flag OFF (census 2026-09-23): {total0}")
    print(f"block, compact flag ON  (HEAD default)     : {total1}  "
          f"(GameStatus placeholder + separately allocated image/streams)")
    res["if_block_flag0"] = total0
    res["if_block_flag1"] = total1
    res["if_files"] = [{"name": r[0], "slot": r[1], "fid": r[2], "container": r[4],
                        "payload": r[5], "aligned": r[6], "nitro_identical": r[8]}
                       for r in rows0]

    print("\n== Owner B: itManagerInitItems")
    f251 = o2r.by_name("ITCommonData")
    f86 = o2r.by_name("MiscData086")
    f87 = o2r.by_name("IFCommonItem")
    tree = al(f251.data_size) + al(f86.data_size)
    print(f"ITCommonData  fid {f251.file_id} payload {f251.data_size} externs {len(f251.external)} "
          f"(all -> fid {sorted(set(f251.extern_ids))})")
    print(f"MiscData086   fid {f86.file_id} payload {f86.data_size}")
    print(f"extern tree (ndsRelocExternTreeAllocSize): {tree}")
    print(f"IFCommonItem  fid {f87.file_id} payload {f87.data_size} (ifCommonItemArrowSetAttr)")
    pool_it = SIZES["ITStruct"] * 16
    print(f"ITStruct pool: {SIZES['ITStruct']} B x ITEM_ALLOC_MAX 16 = {pool_it}")
    res.update(item_tree=tree, item_pool=pool_it, ifcommonitem=al(f87.data_size),
               nitro_items=[nitro_identical(f251), nitro_identical(f86), nitro_identical(f87)])

    print("\n== Owner C: ndsBaseEFManagerInitEffects (decomp efManagerInitEffects + port wrapper)")
    ef = [o2r.by_name(n) for n in ("EFCommonEffects1", "EFCommonEffects2", "EFCommonEffects3")]
    eftot = 0
    for f in ef:
        eftot += al(f.data_size)
        print(f"{f.path.name:18s} fid {f.file_id} payload {f.data_size} externs {len(f.external)} "
              f"nitro==o2r {nitro_identical(f)}")
    print(f"three files: {eftot}   (census: ndsBaseEFManagerInitEffects 94,704)")
    pool_ef = SIZES["EFStruct"] * 38
    vt = SIZES["NDSVisualTemplate"] * 7
    print(f"EFStruct pool: {SIZES['EFStruct']} x NDS_R2_EFFECT_POOL 38 = {pool_ef}")
    print(f"visual templates: {SIZES['NDSVisualTemplate']} x 7 = {vt}  (battleship_efmanager.c:593-620)")
    res.update(ef_files=eftot, ef_pool=pool_ef, ef_templates=vt,
               nitro_ef=[nitro_identical(f) for f in ef])

    print("\n== Particle pools (efParticleInitAll, battleship_lbparticle.c:266-273 + 402-430)")
    ps, pg, pt = 112, 24, 80
    ppool = ps * SIZES["LBParticle"] + pg * SIZES["LBGenerator"] + pt * SIZES["LBTransform"]
    print(f"structs {ps}x{SIZES['LBParticle']}={ps*SIZES['LBParticle']}  "
          f"generators {pg}x{SIZES['LBGenerator']}={pg*SIZES['LBGenerator']}  "
          f"transforms {pt}x{SIZES['LBTransform']}={pt*SIZES['LBTransform']}  total {ppool}")
    res["particle_pools"] = {"structs": ps, "generators": pg, "transforms": pt, "bytes": ppool}

    res["owner_totals_flag0"] = total0 + tree + al(f87.data_size) + pool_it + eftot + pool_ef + vt
    print("\nSUMMARY (all three owners, compact flag OFF, incl. pools and templates):",
          res["owner_totals_flag0"])
    (HERE / "lane2_inventory.json").write_text(json.dumps(res, indent=1))


if __name__ == "__main__":
    main()
