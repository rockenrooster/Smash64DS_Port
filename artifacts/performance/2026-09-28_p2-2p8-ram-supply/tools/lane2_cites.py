#!/usr/bin/env python3
"""Lane 2: citation ledger.  Every file:line the report cites is produced here by
regex against the file as committed at PIN (`git show PIN:path`, read-only), not
against the working tree: other work edits src/port/reloc_backend_assets.c,
src/port/sprite_preview_backend.c and the Makefile while this lane runs, and a
working-tree line number would rot.  A moved line fails loudly.  Prints
`ID  path:start[-end]  note` and exits non-zero if any pattern is missing; the
result is saved to lane2_cites.json.

Usage: python lane2_cites.py
"""
from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
PIN = "039192a0ab6"      # HEAD when this lane was written; contents of every cited file equal e830e06b180's
D = "decomp/BattleShip-main/decomp/src"

# (id, path, start_regex, end_regex or None, nth, note)
CITES = [
    # ---- owner A: interface files
    ("A01", f"{D}/sc/sccommon/scvsbattlefiles.c", r"^void scVSBattleSetupFiles", r"lbRelocLoadFilesListed", 1, "owner A: one lbRelocLoadFilesListed over dGMCommonFileIDs"),
    ("A02", f"{D}/gm/gmcommon.c", r"^u32 dGMCommonFileIDs", r"&llIFCommonAnnounceCommonFileID", 1, "the eight IF file ids and slot order"),
    ("A03", f"{D}/sc/sccommon/scvsbattle.c", r"^\tscVSBattleSetupFiles\(\);", None, 1, "VS setup order: IF files first"),
    ("A04", f"{D}/sc/sccommon/scvsbattle.c", r"^\tefParticleInitAll\(\);", None, 1, "then particle pools"),
    ("A05", f"{D}/sc/sccommon/scvsbattle.c", r"^\titManagerInitItems\(\);", None, 1, "then item tree + pool"),
    ("A06", f"{D}/sc/sccommon/scvsbattle.c", r"^\tefManagerInitEffects\(\);", None, 1, "then effect files"),
    ("A07", f"{D}/sc/sccommon/scvsbattle.c", r"^\t\tftManagerSetupFilesAllKind", None, 1, "then per-player fighter files (after all three owners)"),
    ("A08", "src/port/reloc_backend_assets.c", r"^static void \*ndsRelocLoadIfGameStatusCompact", None, 1, "IFCommonGameStatus compact loader (top-of-heap window, copy kept runs, re-seat)"),
    ("A09", "src/port/reloc_backend_assets.c", r"image = ndsRelocLoadIfGameStatusCompact\(", None, 1, "loader call site; ndsTaskmanElasticReleaseAll() hands the window back first"),
    ("A10", "src/nds/nds_battle_hud.c", r"^static u32 ndsBattleHudPrepare", r"gNdsBattleHudPrepareCount\+\+", 1, "lower HUD prepare"),
    ("A11", "src/nds/nds_battle_hud.c", r"nitro:/menus/battle_hud\.bin", None, 1, "lower HUD glyphs come from a NitroFS blob, not the IF files"),
    ("A12", "src/nds/nds_platform.c", r"gNdsIFCommonHUDLowerTextMode = 1u;", None, 1, "shipping config routes timer/stock/damage to the lower HUD"),
    ("A13", "src/import/battleship_ifcommon.c", r"^u32 ndsIFCommonRouteGObjToLowerTextHUD", r"^}", 1, "timer/stock/damage GObjs are redirected; state only, no pixels"),
    ("A14", "src/import/battleship_ifcommon.c", r"^void ndsIFCommonRecordHUDState", r"^}", 1, "HUD state comes from game state, not sprite data"),
    ("A15", "src/port/sprite_preview_backend.c", r"ndsIFCommonNativeOamDrawGObj\(gobj\) != FALSE", None, 1, "foreground SObjs: native OAM first"),
    ("A16", "src/port/sprite_preview_backend.c", r"ndsSObjRecordSpriteFailure\(gobj, sobj,", None, 1, "anything unrecognised records a native failure, is not drawn"),
    ("A17", "src/port/sprite_preview_backend.c", r"ndsIFCommonRouteGObjToLowerTextHUD\(gobj\) != FALSE", None, 1, "battle: lower-HUD route before the layered path"),
    ("A18", "src/port/sprite_preview_backend.c", r"if \(sprite->bmsiz == G_IM_SIZ_4c\)", None, 1, "SObj creation expands 4c bitmaps in place (no IF sprite is 4c: lane2_4c_check.py)"),
    ("A19", "src/nds/nds_ifcommon_oam.c", r"^static s32 ndsIFCommonAssetForSObj", r"^}", 1, "native OAM recognises SObjs by Bitmap pointer + size (GameStatus assets only)"),
    ("A20", "src/nds/nds_ifcommon_oam.c", r"^s32 ndsIFCommonNativeOamDrawGObj", None, 1, "native OAM draw: tag / arrow / recognised GameStatus assets"),
    ("A21", "src/nds/nds_ifcommon_oam.c", r"^s32 ndsIFCommonNativeOamBakePlayerTag", r"^}", 1, "tag bake reads the sprite's I8 pixels (setup)"),
    ("A22", "src/nds/nds_ifcommon_oam.c", r"^s32 ndsIFCommonNativeOamBakeItemArrow", r"^}", 1, "arrow bake reads the sprite pixels (setup)"),
    ("A23", "src/import/battleship_ifcommon.c", r"ndsIFCommonNativeOamBakePlayerTag\(&sobj->sprite\)", None, 1, "tag bake call at interface creation"),
    ("A24", "src/import/battleship_ifcommon.c", r"ndsIFCommonNativeOamBakeItemArrow\(sIFCommonItemArrowSprite\)", None, 1, "arrow bake call at itManagerInitItems' ifCommonItemArrowSetAttr"),
    ("A25", "src/nds/nds_ifcommon_oam.c", r"^s32 ndsIFCommonNativeOamPrepareGameStatus", None, 1, "GameStatus prepare (bakes GO bank, binds metadata)"),
    ("A26", "src/nds/nds_ifcommon_oam.c", r"^s32 ndsIFCommonNativeOamPrepareClouds", None, 1, "cloud/traffic atlas prepare: reads lamp/rod/frame pixels once"),
    ("A27", "src/nds/nds_ifcommon_oam.c", r"^u32 ndsIFCommonNativeOamLetterPayloads", None, 1, "the twelve letter payloads dropped by the compact load"),
    ("A28", "src/nds/nds_ifcommon_oam.c", r"^s32 ndsIFCommonNativeOamBakeEndVariants", None, 1, "TIME UP / GAME SET run-length streams (22,104 B resident)"),
    ("A29", "src/nds/nds_ifcommon_oam.c", r"^s32 ndsIFCommonNativeOamRebaseGameStatus", None, 1, "retained Sprite/Bitmap pointers moved into the image"),
    ("A30", "src/port/battle_playable_compat_stubs.c", r"weak stub, so ifCommonBattlePauseEjectGObjs", None, 1, "pause decals were once drawn (top-screen layered path, 2026-09-06); now only native owners draw"),
    ("A31", "src/port/sprite_preview_backend.c", r"^SObj \*lbCommonMakeSObjForGObj", r"^}", 1, "SObj creation copies the Sprite header (why headers stay); expands 4c only"),
    ("A32", f"{D}/sys/objman.c", r"new_sobj->sprite = \*sprite;", None, 1, "gcAddSObjForGObj copies the 68 B Sprite header into the SObj (headers are read whenever an interface SObj is made or re-pointed)"),
    # ---- owner B: items
    ("B01", "src/import/battleship_item_link_core.c", r"^void itManagerInitItems", r"ifCommonItemArrowSetAttr\(\);", 1, "owner B: pool + extern tree load + arrow"),
    ("B02", "src/import/battleship_item_link_core.c", r"gITManagerCommonData = lbRelocGetExternHeapFile", None, 1, "82,976 B tree (ITCommonData + MiscData086) in one allocation"),
    ("B03", "src/import/battleship_item_link_core.c", r"^static sb32 ndsItDecodeAttributes", r"^}", 1, "ITAttributes decode: pointers to DObjDesc/MObjSub/anim rows (kept)"),
    ("B04", "src/import/battleship_item_link_core.c", r"^static sb32 itManagerSetupItemDObjs", None, 1, "DObj trees built from DObjDesc arrays"),
    ("B05", "include/it/item.h", r"#define ITEM_ALLOC_MAX 16", None, 1, "ITStruct pool capacity"),
    ("B06", "src/port/renderer_adapter_stage.c", r"ndsRelocNativeRootOffset\(loaded, dl\) == NDS_NATIVE_ITEM_GLUCKY_ROOT", None, 1, "first item owner admission (root identity + DL word compares follow)"),
    ("B07", "src/port/renderer_adapter_stage.c", r"NDS_NATIVE_ITEM_HAMMER_TLUT_OFFSET\)\) &&", None, 1, "example raw base+OFFSET pointer-word compare (dl[11].w1) that a re-seat would break"),
    ("B08", "src/port/renderer_adapter_stage.c", r"NDS_NATIVE_ITEM_HAMMER_FILE_END", None, 1, "example raw data_size >= source FILE_END compare"),
    ("B09", "src/import/battleship_item_link_core.c", r"a Yoshi build's closure then finds it already resident", None, 1, "MiscData086 is shared with Yoshi's fighter closure"),
    ("B10", "src/import/battleship_efmanager.c", r"^GObj \*ndsEFManagerMBallThrownMakeEffectChecked", None, 1, "thrown Master Ball effect reads ITCommonData through gITManagerCommonData (file 86 base recovered by subtraction)"),
    ("B11", "src/port/renderer_adapter_stage.c", r"SUBTRACTS 0x9430", None, 1, "decomp efManagerMBallThrownMakeEffect subtracts a source offset from a fixed-up pointer"),
    ("B12", "src/nds/nds_native_item_hammer.exec.inc", r"tlut = base \+ NDS_NATIVE_ITEM_HAMMER_TLUT_OFFSET;", None, 1, "item owner reads texel/palette bytes at raw base + offset at first draw (bind)"),
    ("B13", "src/nds/nds_native_item_hammer.exec.inc", r"file_bytes < NDS_NATIVE_ITEM_HAMMER_FILE_END", None, 1, "raw source-size compare against the loaded size"),
    # ---- owner C: effects
    ("C01", "src/import/battleship_efmanager.c", r"#define efManagerInitEffects ndsBaseEFManagerInitEffects", None, 1, "owner C: decomp efManagerInitEffects renamed; loads the three files and the EFStruct pool"),
    ("C02", "src/import/battleship_efmanager.c", r"#define EFFECT_ALLOC_NUM NDS_R2_EFFECT_POOL", None, 1, "EFStruct pool = NDS_R2_EFFECT_POOL (38) x 60 B"),
    ("C03", "src/import/battleship_efmanager.c", r"^void efManagerInitEffects", r"ndsEFManagerInitVisualTemplates\(\);", 1, "init sequence incl. desc resolve and visual templates"),
    ("C24", "src/import/battleship_efmanager.c", r"^static void ndsEFManagerResolveAllDescOffsets", None, 1, "resolves every EFDesc against the loaded EF/IT/FT files at init"),
    ("C04", "src/import/battleship_efmanager.c", r"^static size_t ndsEFManagerFileSpan", None, 1, "EFDesc span check uses the LOADED file size"),
    ("C05", "src/import/battleship_efmanager.c", r"^static sb32 ndsEFManagerMapFileOffset", r"^}", 1, "EFDesc offsets already resolve through ndsRelocNativeAssetAddress / ndsRelocNativeRootAddress"),
    ("C06", "src/import/battleship_efmanager.c", r"^static void ndsEFManagerInitVisualTemplates", r"^}", 1, "7 x 352 B procedural templates (heap)"),
    ("C07", "src/port/renderer_adapter_stage.c", r"address == effect_base \+ 0x2500u", r"address == effect_base \+ 0x32e0u", 1, "EF2 entry-effect roots: RAW address equality (must become cell-aware)"),
    ("C08", "src/port/renderer_adapter_stage.c", r"root_offset == 0x0440u", None, 1, "EF3 MBallRays/ItemGetSwirl roots: raw dl - base compare"),
    ("C09", "src/port/renderer_adapter_stage.c", r"rebirth_offset == 0x2378u", None, 1, "EF3 RebirthHalo roots: raw dl - base compare"),
    ("C10", "src/port/renderer_adapter_stage.c", r"slash_root == NDS_NATIVE_DAMAGE_SLASH_ROOT0", None, 1, "EF1 DamageSlash: cell-aware root, then raw fingerprint compares"),
    ("C11", "src/port/renderer_adapter_stage.c", r"\(dl\[18\]\.words\.w1 ==", None, 1, "DamageSlash raw pointer-word fingerprint"),
    ("C12", "src/nds/nds_native_damage_slash.exec.inc", r"^s32 ndsRendererHardwarePrepareDamageSlashTextures", r"^}", 1, "13 CI4 frames read at scene prepare, raw asset_base + offset"),
    ("C13", "src/nds/nds_native_damage_fly_mdust.exec.inc", r"^s32 ndsRendererHardwarePrepareDamageFlyMDustTextures", r"^}", 1, "7 frames read at scene prepare, raw offsets"),
    ("C14", "src/nds/nds_native_damage_fly_mdust.exec.inc", r"^sb32 ndsRendererPreflightNativeDamageFlyMDust", None, 1, "preflight compares source offsets against the loaded size"),
    ("C15", "src/nds/nds_renderer_textures_effects.c", r"^s32 ndsRendererHardwarePrepareImpactWaveTextures", None, 1, "ImpactWave texels are ROM tables"),
    ("C16", "src/nds/nds_renderer_textures_effects.c", r"^s32 ndsRendererHardwarePrepareRebirthHaloTextures", None, 1, "RebirthHalo texels are ROM tables"),
    ("C17", "src/nds/nds_renderer_native_common.c", r"^s32 ndsRendererHardwarePrepareEntryEffectTextures", None, 1, "entry-effect texels are ROM tables (sNdsEntryEffectTextures)"),
    ("C18", "src/port/reloc_backend_compat_shims.c", r"^GObj \*ftShadowMakeShadow", r"^}", 1, "shadow maker returns NULL: decomp ftshadow.c never runs"),
    ("C19", "src/nds/nds_renderer_textures_effects.c", r"^static s32 ndsRendererHardwareBuildBattleStaticTextureKey", r"^}", 1, "static-corpus key build: identity only, fails if the offset does not map"),
    ("C20", "src/nds/nds_renderer_textures_effects.c", r"^s32 ndsRendererHardwareRefreshBattleStaticTexturePointers", None, 1, "every applicable record must build a key or stage admission declines"),
    ("C21", "src/import/battleship_lbparticle.c", r"#define NDS_R2_PARTICLE_POOL_STRUCTS 112", None, 1, "particle pool capacities 112/24/80 (menu shell / four-CPU)"),
    ("C22", "src/import/battleship_lbparticle.c", r"^void efParticleInitAll", None, 1, "pool allocation"),
    ("C23", "src/import/battleship_lbparticle.c", r"KOBurstAttempt  6, Complete 4, DropMask 2", None, 1, "history: a saturated transform pool silently drops KO bursts"),
    ("C25", "src/port/reloc_backend_compat_shims.c", r"^static sb32 ndsFTParamMakeSourceEffect", None, 1, "source (file/particle) effect makers reached for a subset of effect kinds"),
    ("C26", "src/port/reloc_backend_compat_shims.c", r"case nEFKindDamageFlyOrbs:", None, 1, "hit sparks / fly orbs / star-rod spark go to procedural visual templates, not EF file lists"),
    # ---- shared mechanism (precedent)
    ("C28", "src/port/renderer_adapter_stage.c", r"NDS_NATIVE_FAILURE_NO_PROGRAM, render_stats\);", None, 1, "fall-through of the stage/effect/item submit: no owner admitted the list -> NO_PROGRAM record, nothing executed"),
    ("B14", "src/nds/nds_native_item_hammer.exec.inc", r"has no 0xDE opcode; its TLUT, image, state, and both", None, 1, "item owners bake geometry/state into ROM; only TLUT/image bytes are read from the file"),
    ("C29", "src/port/renderer_adapter_stage.c", r"^static void ndsRendererAdapterScanDisplayProcOtherMode", r"^}", 1, "DL-heap scanner: walks the frame's emitted list (gSYTaskmanDLHeads), not a file list"),
    ("C30", "src/port/renderer_adapter_stage.c", r"^void ndsRendererAdapterCaptureItemDisplayProcState", r"^}", 1, "DL-heap scanner for item procs: prim/env colour and other-mode words of the frame's list"),
    ("C31", "src/port/renderer_adapter_stage.c", r"^void ndsRendererAdapterCaptureDisplayProcColors", r"^}", 1, "DL-heap scanner for effect procs"),
    ("S01", "src/import/battleship_sc1pgame_runtime.c", r"llStagePupupuFile2FileID inside gGMCommonFiles\[4\]", None, 1, "1P also reads an IFCommonDigits Sprite header (not audited here: scope is the VS battle)"),
    ("C27", f"{D}/ef/efmanager.c", r"gEFManagerFiles\[0\] = lbRelocGetExternHeapFile", r"gEFManagerFiles\[2\] = lbRelocGetExternHeapFile", 1, "decomp: the three EF files, three separate 16 B-aligned allocations sized by lbRelocGetFileSize"),
    ("R09", "src/port/reloc_backend_assets.c", r"^size_t lbRelocGetFileSize\(const void \*file_id\)", None, 1, "the size hook the EF/IT allocations are made from (return the packed size here)"),
    ("P01", "scripts/fighters/generate_battle_core_packs.py", r"^\"\"\"Generate source-exact LOW-detail", r"^\"\"\"$", 1, "battle-time pack precedent: root identity cells + extern patch manifest (BEX1) for cross-file slots"),
    ("P02", "src/port/reloc_preview_pack.c", r"^s32 ndsRelocPatchCompactBattleMainExterns", None, 1, "runtime patch of a packed file's cross-file externs once the other files are resident"),
    ("P03", "src/port/reloc_preview_pack.c", r"^static s32 ndsRelocLoadPreviewFighterUnlocked", None, 1, "pack load + publish: the loader a common-file pack path would mirror"),
    ("P04", "src/port/reloc_preview_pack.c", r"^static u32 ndsRelocNativeSourceSize", r"^}", 1, "source extent of a packed file (what raw size compares must use)"),
    ("R01", "src/port/reloc_backend_assets.c", r"^void \*ndsRelocGetFileData\(void \*file, const void \*symbol\)", None, 1, "ll* offset-token resolver: IF compact map or FPC span map, then size check (already handles both)"),
    ("R02", "src/port/reloc_backend_assets.c", r"^static s32 ndsRelocIfCompactMap", None, 1, "IF compact source->image offset map (static table, 65 rows max)"),
    ("R03", "src/port/reloc_backend_assets.c", r"^typedef struct NDSRelocIfSpan", r"NDSRelocIfSpan;", 1, "IF span row (12 B)"),
    ("R04", "src/port/reloc_backend_assets.c", r"^size_t lbRelocGetAllocSize", None, 1, "sizes the IFCommon block that lbRelocLoadFilesListed allocates (compact GameStatus counted as a placeholder); EF and item allocations are sized by lbRelocGetFileSize instead"),
    ("R05", "include/nds/nds_reloc_assets.h", r"^s32 ndsRelocAssetReadRawRange", None, 1, "NitroFS range reader for retargeted (scenario B) readers"),
    ("R06", "src/port/reloc_backend_assets.c", r"^s32 ndsRelocGetLoadedAssetView", None, 1, "what owners call to get (base, size): size is the LOADED (compact) size"),
    ("R07", "src/port/reloc_preview_pack.c", r"^static const NDSPreviewPackSection \*ndsPreviewSection\(", None, 1, "pack section lookup: keyed to the fighter resident table today"),
    ("R08", "src/port/reloc_backend_assets.c", r"ndsTaskmanElasticReleaseAll\(\);$", None, 1, "elastic motion cache hands the heap top back before a compaction window is used"),
    ("M01", "src/port/reloc_preview_pack.c", r"^static u32 ndsRelocNativeRootOffset", r"^}", 1, "root identity from an ENDDL cell (pack) or dl - data (plain)"),
    ("M02", "src/port/reloc_preview_pack.c", r"^const void \*ndsRelocNativeRootAddress", r"^}", 1, "source root offset -> cell address"),
    ("M03", "src/port/reloc_preview_pack.c", r"^const void \*ndsRelocNativeAssetAddress", r"^}", 1, "source offset -> packed address through spans"),
    ("M04", "include/nds/nds_preview_pack.h", r"^typedef struct NDSPreviewPackSpan", r"NDSPreviewPackSpan;", 1, "12 B span row"),
    ("M05", "include/nds/nds_preview_pack.h", r"^typedef struct NDSPreviewPackFixup", r"NDSPreviewPackFixup;", 1, "8 B fixup row (file only, not resident)"),
    ("M06", "scripts/fighters/generate_preview_core_packs.py", r"^def build_pack", None, 1, "fighter pack generator (spans, fixups, root cells)"),
    ("M07", "scripts/fighters/generate_preview_core_packs.py", r"roots_body = b\"\"\.join", None, 1, "8 B root cells {ENDDL, source root offset}"),
    ("M08", "scripts/fighters/generate_preview_core_packs.py", r"^def decode_pack", None, 1, "independent decoder used as the pack's verifier"),
    ("M09", "scripts/check_native_only_rom.py", r"^FORBIDDEN", None, 1, "display-list interpreter symbols the ROM must not link"),
    ("M10", "Makefile", r"check_native_only_rom\.py\" --elf \"\$\(OUTPUT\)\.elf\"", None, 1, "the guard runs at ROM packaging (ARM9 image)"),
    ("M11", "src/port/reloc_backend_assets.c", r"^static size_t ndsRelocAssetAllocSize\(u32 asset_id\)$", r"^}", 1, "allocation size = 16 B-aligned payload size"),
    ("M12", "src/port/reloc_backend_assets.c", r"define NDS_RELOC_ALIGN_BYTES 0x10u", None, 1, "16 B allocation alignment"),
]


def find(lines, pat, nth=1, start=0):
    rx = re.compile(pat)
    hits = 0
    for i in range(start, len(lines)):
        if rx.search(lines[i]):
            hits += 1
            if hits == nth:
                return i + 1
    return None


def main():
    ok = True
    out = []
    cache = {}
    for cid, path, a, b, nth, note in CITES:
        if path not in cache:
            r = subprocess.run(["git", "-C", str(REPO), "show", f"{PIN}:{path}"], capture_output=True)
            cache[path] = r.stdout.decode("utf-8", errors="replace").splitlines() if r.returncode == 0 else None
        lines = cache[path]
        if lines is None:
            print(f"{cid} MISSING FILE {path}")
            ok = False
            continue
        s = find(lines, a, nth)
        if s is None:
            print(f"{cid} NOT FOUND {path} /{a}/")
            ok = False
            continue
        e = None
        if b:
            e = find(lines, b, 1, s - 1)
            if e is None:
                print(f"{cid} END NOT FOUND {path} /{b}/")
                ok = False
                continue
        span = f"{s}-{e}" if (e and e != s) else f"{s}"
        out.append({"id": cid, "path": path, "lines": span, "note": note})
        print(f"{cid}  {path}:{span}  {note}")
    Path(__file__).with_name("lane2_cites.json").write_text(json.dumps(out, indent=1))
    Path(__file__).with_name("lane2_cites_pin.txt").write_text(PIN + "\n")
    print(f"{len(out)} citations resolved" + ("" if ok else "; SOME FAILED"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
