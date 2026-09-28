#!/usr/bin/env python3
"""Lane 2: post-load read model of EFCommonEffects1/2/3 (files 83/84/85).

Combines
  * the typed interval partition (lane2_partition),
  * reachability from the decomp-used ll roots (lane2_reach; the decomp-only
    ShadowTextureImage root is NOT a DS root: ftShadowMakeShadow is a NULL shim,
    src/port/reloc_backend_compat_shims.c:12811-12815),
  * the DS reader facts below, each verified in source (file:line in report).

Read tags per interval (mutually exclusive, byte totals sum to the payload):
  dead_unreferenced  no source root reaches it (decomp never dereferences it)
  dead_dl            reachable Gfx bytes: no display-list interpreter is linked
                     (scripts/check_native_only_rom.py FORBIDDEN list, nm of the
                     shipping ELF) and the admission tests compare addresses only
  dead_vertices      Vtx: same reasoning; geometry is generated ROM data
  dead_texture       texture/palette bytes with no DS reader (ROM-baked or no owner)
  identity_only      static-corpus textures: address identity only, bytes unread
  prepare_only       texture bytes read ONLY at scene prepare
                     (DamageSlash 13 frames, DamageFlyMDust 7 frames)
  keep_fingerprint   DL bytes whose words the owner candidate check compares
  keep_runtime       DObjDesc / MObjSub / animation scripts / DObjDLLink lists
                     (source effect construction and per-frame animation)
  pad_other          PAD()/gap filler and non-source data
"""
from __future__ import annotations

import json
import re
import sys
from collections import defaultdict
from pathlib import Path

import lane2_o2r as o2r
import lane2_partition as part
import lane2_reach as reach

HERE = Path(__file__).resolve().parent
REPO = o2r.REPO

# DamageSlash: sNdsDamageSlashTextureOffsets (src/nds/generated/nds_native_damage_slash.generated.inc)
SLASH_TEX = [0x7260, 0x70d8, 0x6f50, 0x6dc8, 0x6c40, 0x6ab8, 0x6930, 0x67a8,
             0x65a0, 0x6398, 0x6190, 0x5f88, 0x5d80]
# DamageFlyMDust: sNdsDamageFlyMDustFrameOffsets (.../nds_native_damage_fly_mdust.generated.inc)
FLY_TEX = [0xc178, 0xb970, 0xb168, 0xa960, 0xa158, 0x9950, 0x9148]
# static-corpus identity keys into file 84 (lane2_static_keys.py): address identity only
IDENTITY_KEYS = {2: [0x3af8, 0x3f00, 0x4708]}
# DamageSlash owner candidate reads dl[7].w1, dl[13].w0/w1, dl[18].w1 on each root
SLASH_ROOTS = [0x75a0, 0x7668]
FP_INDEX = [7, 13, 18]

# DS handler per effect (evidence in the report; key = ll effect name)
HANDLERS = {
    1: {
        "DamageSlash": ("native owner + prepare reads 13 CI4 frames from the file",
                        "src/nds/nds_native_damage_slash.exec.inc:124-190; renderer_adapter_stage.c:8430-8470"),
        "DamageFlyMDust": ("native owner + prepare reads 7 frames from the file",
                           "src/nds/nds_native_damage_fly_mdust.exec.inc:44-100"),
        "ImpactWave": ("NDL/native: geometry+palette+texel from ROM data, GObj identity",
                       "renderer_adapter_stage.c:2963-3012; nds_renderer_textures_effects.c:3754-3781"),
        "FlyOrbs": ("no DS owner found for root 0x7E80 (no reference in src/)", "UNPROVEN"),
        "CommonSpark": ("no DS owner found for root 0x8FA0 (no reference in src/)", "UNPROVEN"),
        "QuakeMag0": ("camera quake: AnimJoint only, no DL", ""),
        "QuakeMag1": ("camera quake: AnimJoint only, no DL", ""),
        "QuakeMag2": ("camera quake: AnimJoint only, no DL", ""),
        "QuakeMag3": ("camera quake: AnimJoint only, no DL", ""),
    },
    2: {
        "CatchSwirl": ("entry-effect generated pack, address-equality admission (roots 0x2500/0x2588/0x2610/0x2698)",
                       "renderer_adapter_stage.c:6211-6231; nds_entry_effects.generated.inc"),
        "DeadExplodeDefault": ("entry-effect generated pack (KO roots 0x5218/0x52b0/0x5310)",
                               "renderer_adapter_stage.c:6217-6222"),
        "DeadExplode1": ("MatAnimJoint only", ""), "DeadExplode2": ("MatAnimJoint only", ""),
        "DeadExplode3": ("MatAnimJoint only", ""), "DeadExplode4": ("MatAnimJoint only", ""),
        "ReflectBreak": ("entry-effect generated pack (roots 0x31d0/0x3258/0x32e0); alt DLs 0x38d8.. unreachable",
                         "renderer_adapter_stage.c:6222-6226"),
        "FireSpark": ("no DS owner found for root 0x1f78 (no reference in src/)", "UNPROVEN"),
        "NessPKFlash": ("no DS owner found for root 0x6c28 (no reference in src/)", "UNPROVEN"),
        "ShockSmall": ("no DS owner found for root 0x1500 (only an unrelated generated-table hit)", "UNPROVEN"),
        "ShadowTexture": ("decomp ftshadow.c only; port ftShadowMakeShadow returns NULL",
                          "src/port/reloc_backend_compat_shims.c:12811-12815"),
    },
    3: {
        "ItemGetSwirl": ("entry-effect generated pack (roots 0x2ef0/0x2f80/0x3010/0x30a0)",
                         "renderer_adapter_stage.c:6626-6640"),
        "MBallRays": ("entry-effect generated pack (roots 0x0440/0x0518)",
                      "renderer_adapter_stage.c:6626-6650"),
        "RebirthHalo": ("native RebirthHalo generated data (roots 0x2378/0x27e8/0x2a88), ROM textures",
                        "renderer_adapter_stage.c:7432-7440; nds_renderer_textures_effects.c:3784-3830"),
    },
}


def model(stem, name, prefix, ll, n):
    import bisect
    import lane2_dl as dl
    f, totals, rows, offsets, unplaced = part.run(stem, name, prefix, ll, verbose=False)
    types = dict(part.LAST_TYPES)
    starts = [r[0] for r in rows]

    def iv(off):
        return max(bisect.bisect_right(starts, off) - 1, 0)

    adj = defaultdict(set)
    for slot, tgt in f.internal.items():
        adj[iv(slot)].add(iv(tgt))
    eff = reach.effect_roots(n)
    roots = [off for kinds in eff.values() for off in kinds.values()]
    seen, stack = set(), [iv(o) for o in roots]
    while stack:
        i = stack.pop()
        if i in seen:
            continue
        seen.add(i)
        stack.extend(adj.get(i, ()))
    # base byte classes from the (declared-size trimmed) label rows
    size = f.data_size
    cls = [None] * size
    is_link = [False] * size
    for i, (a, used, c, lab, pad) in enumerate(rows):
        link = ("DLLink" in lab) or (types.get(lab) == "DObjDLLink")
        for b in range(a, min(a + used, size)):
            cls[b] = "dllink" if link else c
        for b in range(a + used, min(a + used + pad, size)):
            cls[b] = "pad"
    # structural overlay
    d = dl.Decode(f)
    for r_ in sorted(reach_dl_roots(f, rows, types)):
        d.walk(r_)
    rg = d.ranges()
    over = {}
    for c in ("display_list", "vertices"):
        cnt = 0
        for a, b in rg[c]:
            for k in range(a, min(b, size)):
                if cls[k] != "dllink":
                    if cls[k] != c:
                        cnt += 1
                    cls[k] = c
        over[c] = cnt
    prepare = set()
    if n == 1:
        prepare = set(SLASH_TEX) | set(FLY_TEX)
    idkeys = IDENTITY_KEYS.get(n, [])
    fp_starts = set(SLASH_ROOTS) if n == 1 else set()
    fp_words = {r_ + 8 * ix for r_ in fp_starts for ix in FP_INDEX}
    tags = defaultdict(int)
    classes = defaultdict(int)
    bytag = [None] * size
    for b in range(size):
        c = cls[b] or "unclassified"
        classes[c] += 1
        k = iv(b)
        a = starts[k]
        if k not in seen and c not in ("pad",):
            t = "dead_unreferenced"
        elif c in ("animation", "material", "dobjdesc", "dllink", "other", "other_data", "unclassified"):
            t = "keep_runtime"
        elif c == "display_list":
            t = "keep_fingerprint" if (b & ~7) in fp_words else "dead_dl"
        elif c == "vertices":
            t = "dead_vertices"
        elif c in ("texture", "palette"):
            if a in prepare:
                t = "prepare_only"
            elif any(a <= kk < a + rows[k][1] for kk in idkeys):
                t = "identity_only"
            else:
                t = "dead_texture"
        else:
            t = "pad_other"
        bytag[b] = t
        tags[t] += 1
    total = sum(tags.values())
    assert total == f.data_size, (name, total, f.data_size)

    def runs(keep):
        n_runs, prev = 0, False
        for t in bytag:
            cur = t in keep
            if cur and not prev:
                n_runs += 1
            prev = cur
        return n_runs
    keep_a = {"keep_runtime", "keep_fingerprint"}
    # scenario A keeps the prepare-time textures until the reader is retargeted,
    # and one 8-byte identity cell per static-corpus key (3 in file 84)
    runs_a = runs(keep_a | {"prepare_only"})
    runs_b = runs(keep_a)
    return {"file": name, "fid": f.file_id, "payload": f.data_size,
            "tags": dict(tags), "classes": dict(classes),
            "structural_override_bytes": over,
            "kept_runs_A": runs_a, "kept_runs_B": runs_b,
            "identity_cells": len(idkeys),
            "label_classes": totals}, {}


def reach_dl_roots(f, rows, types):
    roots = set()
    for a, used, c, lab, pad in rows:
        if c == "display_list":
            if ("DLLink" in lab) or (types.get(lab) == "DObjDLLink"):
                for off in range(a + 4, a + max(used, 8), 8):
                    t = f.internal.get(off)
                    if t is not None:
                        roots.add(t)
            else:
                roots.add(a)
    return roots


def main():
    out = {}
    print("read model of the effect files (bytes)")
    for row in reach.EF:
        res, detail = model(*row)
        out[res["file"]] = res
        t = res["tags"]
        print(f"== {res['file']} fid={res['fid']} payload={res['payload']}")
        for k in ("keep_runtime", "keep_fingerprint", "prepare_only", "identity_only",
                  "dead_unreferenced", "dead_dl", "dead_vertices", "dead_texture", "pad_other"):
            print(f"   {k:18s} {t.get(k, 0):7d}")
    tot = defaultdict(int)
    for r in out.values():
        for k, v in r["tags"].items():
            tot[k] += v
    print("== all three")
    for k in sorted(tot):
        print(f"   {k:18s} {tot[k]:7d}")
    print("   SUM", sum(tot.values()))
    out["_total"] = dict(tot)
    out["_handlers"] = {str(n): {k: list(v) for k, v in h.items()} for n, h in HANDLERS.items()}
    (HERE / "lane2_effects.json").write_text(json.dumps(out, indent=1))


if __name__ == "__main__":
    main()
