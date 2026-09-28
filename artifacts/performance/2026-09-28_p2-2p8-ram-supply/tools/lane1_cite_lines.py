#!/usr/bin/env python3
"""Lane 1: resolve every file:line the report cites against the CURRENT working tree.

The tree was being edited concurrently (src/port/reloc_backend_assets.c gained an elastic motion cache
after ~line 13,388 while this lane ran), so citations are re-derived here from patterns rather than typed
by hand.  Output: name -> file:line list, written to ../lane1_citations.json and printed.
"""
import json
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
R = "src/port/reloc_backend_assets.c"
CHECKS = {
    "nativeEntryOwnsDependency": (R, r"^static s32 ndsRelocNativeEntryOwnsDependency\("),
    "resolveNativeEntryExternalFixup": (R, r"^static s32 ndsRelocResolveNativeEntryExternalFixup\("),
    "applyExternalPointerFixups": (R, r"^static s32 ndsRelocApplyExternalPointerFixups\("),
    "shieldPoseResolve": (R, r"native_dep_result = ndsShieldPoseResolveExternalFixup\("),
    "externTreeAllocSize_def": (R, r"^static size_t ndsRelocExternTreeAllocSize\(u32 asset_id, u32 \*seen,\s*$"),
    "ownsDep_in_treesize": (R, r"if \(ndsRelocNativeEntryOwnsDependency\(asset_id, dep_asset_id\) != FALSE\)"),
    "loadExternTreeAsset_def": (R, r"^static NDSRelocLoadedFile \*ndsRelocLoadExternTreeAsset\(u32 asset_id,\s*$"),
    "ensureLoadedAsset": (R, r"^static NDSRelocLoadedFile \*ndsRelocEnsureLoadedAsset\("),
    "staticBufferForAsset": (R, r"^static void \*ndsRelocStaticBufferForAsset\("),
    "normalizeGroundMapAsset": (R, r"^static void ndsRelocNormalizeGroundMapAsset\("),
    "normalizeStageDreamLandSprite": (R, r"^static void ndsRelocNormalizeStageDreamLandSprite\("),
    "isStageWallpaperAsset": (R, r"^static u32 ndsRelocIsStageWallpaperAsset\("),
    "loadedPointerProvenance": (R, r"^s32 ndsRelocGetLoadedPointerProvenance\("),
    "findLoadedFileContaining": (R, r"^static NDSRelocLoadedFile \*ndsRelocFindLoadedFileContaining\("),
    "lbRelocGetFileSize": (R, r"^size_t lbRelocGetFileSize\("),
    "lbRelocGetExternHeapFile": (R, r"^void \*lbRelocGetExternHeapFile\("),
    "ifCompactLoad": (R, r"^static void \*ndsRelocLoadIfGameStatusCompact\("),
    "ifCompactMap": (R, r"^static s32 ndsRelocIfCompactMap\("),
    "ifBuildSpans": (R, r"^static u32 ndsRelocIfBuildSpans\("),
    "groundMapAssetsTable": (R, r"^static const struct \{\s*$"),
    "relocAlign": (R, r"^#define NDS_RELOC_ALIGN\(value\)"),
    "mpCollisionInitGroundData_port": ("src/port/reloc_backend_compat_shims.c", r"^void mpCollisionInitGroundData\("),
    "ground_syTaskmanMalloc": ("src/port/reloc_backend_compat_shims.c", r"syTaskmanMalloc\(bytes, 0x10\)"),
    "ground_blobLoad": ("src/port/reloc_backend_compat_shims.c", r"ndsNativeStageBlobLoad\(i\)"),
    "ground_preload": ("src/port/reloc_backend_compat_shims.c", r"^\s+ndsRendererAdapterNativeStagePreloadAssets\(\);"),
    "previewFileOffset": ("src/port/reloc_preview_pack.c", r"^static s32 ndsPreviewFileOffset\("),
    "nativeRootAddress": ("src/port/reloc_preview_pack.c", r"^const void \*ndsRelocNativeRootAddress\("),
    "nativeAssetAddress": ("src/port/reloc_preview_pack.c", r"^const void \*ndsRelocNativeAssetAddress\("),
    "nativeForeignImageAddress": ("src/port/reloc_preview_pack.c", r"^const void \*ndsRelocNativeForeignImageAddress\("),
    "previewResident": ("src/port/reloc_preview_pack.c", r"^typedef struct NDSPreviewResident"),
    "admission_data_size": ("src/port/renderer_adapter_stage.c", r"\(loaded\[i\]->data_size !="),
    "direct_wallpaper_bind": ("src/port/renderer_adapter_stage.c", r"\(void\)ndsNativeBattleWallpaperDraw\("),
    "collect_dl_link_read": ("src/port/renderer_adapter_stage.c", r"workspace->binding_display_lists\[binding\] = dl_link->dl"),
    "stamp_live_list": ("src/port/renderer_adapter_stage.c", r"if \(live_list != workspace->binding_display_lists\[i\]\)"),
    "commitNativeStageDisplay": ("src/port/renderer_adapter_stage.c", r"^ndsRendererAdapterCommitNativeStageDisplay\("),
    "prepareNativeStageOwnerBody": ("src/port/renderer_adapter_stage.c", r"^static s32 ndsRendererAdapterPrepareNativeStageOwnerBody\(\s*$"),
    "preloadAssets_def": ("src/port/renderer_adapter_matrix.c", r"^void ndsRendererAdapterNativeStagePreloadAssets\("),
    "validate_display_list_compare": ("src/nds/nds_renderer_native_owners.c", r"\(frame->binding_display_lists\[i\] !="),
    "state_delta_image_base": ("src/nds/nds_renderer_native_owners.c", r"asset_base = frame->asset_bases\[delta->asset_index\]"),
    "task26_image_macro": ("src/nds/nds_renderer_native_owners.c", r"#define NDS_TASK26_IMAGE"),
    "warm_resolve_call": ("src/nds/nds_renderer_native_owners.c", r"ndsRendererHardwareResolveStageSourceFrameTexture\("),
    "warm_flag_clear": ("src/nds/nds_renderer_native_owners.c", r"gNdsNativeStageWarmUploads = 0u"),
    "stage_warm_pin": ("src/nds/nds_renderer_native_owners.c", r"resolved\.entry->stage_warm = TRUE"),
    "texel_read": ("src/nds/nds_renderer_textures_effects.c", r"texels_src = ndsRendererResolveTextureDataPointer\("),
    "tlut_read": ("src/nds/nds_renderer_textures_effects.c", r"tlut_src = ndsRendererResolveTextureDataPointer\("),
    "staticKeyBuild": ("src/nds/nds_renderer_textures_effects.c", r"^static s32 ndsRendererHardwareBuildBattleStaticTextureKey\("),
    "resolveTextureDataPointer": ("src/nds/nds_renderer_textures_effects.c", r"^static const void \*ndsRendererResolveTextureDataPointer\("),
    "warmEvictSkip": ("src/nds/nds_renderer_textures_effects.c", r"\(entry->pinned == 0u\) && \(entry->stage_warm == 0u\)"),
    "blob_warm_set": ("src/nds/nds_native_stage_blob.c", r"gNdsNativeStageWarmUploads = 1u"),
    "blob_alloc": ("src/nds/nds_native_stage_blob.c", r"slab = syTaskmanMalloc\("),
    "gx_alloc": ("src/nds/nds_stage_gx.exec.inc", r"body = syTaskmanMalloc\(h\.body_bytes"),
    "gx_keepfree": ("src/nds/nds_stage_gx.exec.inc", r"#define NDS_STAGE_GX_HEAP_KEEP_FREE"),
    "scanDObjs": ("src/port/reloc_backend_movement.c", r"^static void ndsStageGCDrawAllLoopScanDObjs\(GObj \*gobj, u32 owner_mask,"),
    "recordDObjDraw": ("src/port/reloc_backend_movement.c", r"^void ndsStageGCDrawAllLoopRecordDObjDraw\("),
    "recordCapturedDisplay": ("src/port/reloc_backend_movement.c", r"^ndsStageGCDrawAllLoopRecordCapturedDisplay\("),
    "classifyGObj": ("src/port/reloc_backend_movement.c", r"^static sb32 ndsStageGCDrawAllLoopClassifyGObj\("),
    "gcDrawAll_dispatch": ("src/port/opening_movie_backend.c", r"ndsStageGCDrawAllLoopRecordCapturedDisplay\("),
    "gcDrawAll_proc_display": ("src/port/opening_movie_backend.c", r"^\s+current_gobj->proc_display\(current_gobj\);"),
    "makeSObj": ("src/port/sprite_preview_backend.c", r"^SObj \*lbCommonMakeSObjForGObj\("),
    "decode4c": ("src/port/sprite_preview_backend.c", r"u8 \*bitmap_start = \(u8 \*\)bitmap\[n - 1\]\.buf"),
    "drawCachedWallpaperFinal": ("src/port/sprite_preview_backend.c", r"^static s32 ndsSObjDrawCachedWallpaperFinal\("),
    "sobj_provenance": ("src/port/sprite_preview_backend.c", r"\(ndsRelocGetLoadedPointerProvenance\(sprite->bitmap"),
    "drawLayeredSObjFrame": ("src/port/sprite_preview_backend.c", r"^static void ndsDrawLayeredSObjFrame\("),
    "drawSObjPreview": ("src/port/sprite_preview_backend.c", r"^static s32 ndsDrawSObjPreview\("),
    "lbCommonDrawSObjAttr": ("src/port/sprite_preview_backend.c", r"^void lbCommonDrawSObjAttr\("),
    "battleWallpaperBindings": ("src/nds/nds_native_wallpaper.c", r"sBattleWallpaperBindings\[9\]"),
    "wallpaperUpload": ("src/nds/nds_native_wallpaper.c", r"^static s32 ndsNativeWallpaperUpload\("),
    "wallpaperDraw": ("src/nds/nds_native_wallpaper.c", r"^s32 ndsNativeWallpaperDraw\("),
    "battleWallpaperDraw": ("src/nds/nds_native_wallpaper.c", r"^s32 ndsNativeBattleWallpaperDraw\("),
    "battleWallpaperPreload": ("src/nds/nds_native_wallpaper.c", r"^s32 ndsNativeBattleWallpaperPreload\("),
    "preloadCaller": ("src/port/taskman_seam_battle_host.c", r"ndsNativeBattleWallpaperPreload\("),
    "sssStartScene": ("src/nds/nds_menu_shell_router.c", r"^void mnMapsStartScene\("),
    "mnmapsGuard": ("src/import/battleship_mnmaps.c", r"^#if !NDS_P2_MENU_SHELL"),
    "overlayTryAlloc": ("src/nds/nds_frontend_overlay.c", r"^void \*ndsFrontendOverlayTryAlloc\("),
    "sceneAssetAlloc": ("src/nds/nds_frontend_overlay.c", r"^void \*ndsSceneAssetAlloc\("),
    "grwallpaper_uses": ("decomp/BattleShip-main/decomp/src/gr/grwallpaper.c", r"gMPCollisionGroundData->wallpaper"),
    "grwallpaper_decide": ("decomp/BattleShip-main/decomp/src/gr/grwallpaper.c", r"^void grWallpaperMakeDecideKind"),
    "decomp_mpInitGround": ("decomp/BattleShip-main/decomp/src/mp/mpcollision.c", r"^void mpCollisionInitGroundData"),
    "gcSetupCustomDObjs": ("decomp/BattleShip-main/decomp/src/sys/objanim.c", r"^void gcSetupCustomDObjs\("),
    "gcSetupCommonDObjs": ("decomp/BattleShip-main/decomp/src/sys/objanim.c", r"^void gcSetupCommonDObjs\("),
    "scvs_initGround": ("decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c", r"^\smpCollisionInitGroundData\(\);"),
    "efground_filehead": ("decomp/BattleShip-main/decomp/src/ef/efground.c", r"sEFGroundActor\.file_head ="),
    "fpc_build_pack": ("scripts/fighters/generate_preview_core_packs.py", r"^def build_pack\("),
    "fpc_decode_pack": ("scripts/fighters/generate_preview_core_packs.py", r"^def decode_pack\("),
    "fpc_section_struct": ("include/nds/nds_preview_pack.h", r"^typedef struct NDSPreviewPackSection"),
    "gen_load_o2r": ("scripts/stages/generate_nds_native_stage.py", r"^def load_o2r\("),
    "gen_compile_state_delta": ("scripts/stages/generate_nds_native_stage.py", r"^def compile_state_delta\("),
    "gen_wallpaper_sources": ("scripts/stages/generate_native_wallpapers.py", r"^SOURCES: tuple"),
    "makefile_if_compact": ("Makefile", r"^NDS_IF_GAMESTATUS_COMPACT \?="),
    "makefile_gx_stages": ("Makefile", r"^NDS_STAGE_GX_STAGES :="),
    "makefile_reloc_rule": ("Makefile", r"^\$\(NITROFS_DIR\)/reloc/%: \$\(BATTLESHIP_O2R\)/%"),
    "nds_reloc_assets_stage_rows": ("src/nds/nds_reloc_assets.c", r'"nitro:/reloc/reloc_stages/GRPupupuMap"'),
    "descriptor_jungle_107": ("scripts/stages/native_stage_descriptors/jungle.py", r'"stage_images": \{'),
    "descriptor_jungle_asset_order": ("scripts/stages/native_stage_descriptors/jungle.py", r"asset_order="),
    "groundActorSubmit": ("src/port/reloc_backend_movement.c", r"^static void ndsStageGCDrawAllLoopSubmitGroundActorDObj\(GObj"),
    "mpVertexF32Get": ("src/port/reloc_backend_mp_collision.c", r"^static inline void ndsMPVertexF32Get\("),
    "mpSetGeometry": ("src/port/reloc_backend_mp_collision.c", r"^static void ndsMPCollisionSetGeometry\("),
    "gmcamera_bounds": ("decomp/BattleShip-main/decomp/src/gm/gmcamera.c", r"^void gmCameraSetBoundsPosition\("),
    "gcPlayAnimAll": ("decomp/BattleShip-main/decomp/src/sys/objanim.c", r"^void gcPlayAnimAll\("),
    "gcParseDObjAnimJoint": ("decomp/BattleShip-main/decomp/src/sys/objanim.c", r"^void gcParseDObjAnimJoint\("),
    "gcAddMObjAll": ("decomp/BattleShip-main/decomp/src/sys/objanim.c", r"^void gcAddMObjAll\("),
    "itemWeights": ("decomp/BattleShip-main/decomp/src/it/itmanager.c", r"gMPCollisionGroundData->item_weights != NULL"),
    "overlayLoanSym": ("src/nds/nds_frontend_overlay.c", r"^void \*ndsFrontendOverlayTryAlloc\("),
    "mvopeningroom_wallpaper": ("decomp/BattleShip-main/decomp/src/mv/mvopening/mvopeningroom.c", r"llMVOpeningRoomWallpaperSprite\)"),
    "mvopeningyoster_wallpaper": ("decomp/BattleShip-main/decomp/src/mv/mvopening/mvopeningyoster.c", r"&llStageYoshiSprite"),
    "gr_jungle_forcefile": ("decomp/BattleShip-main/decomp/src/gr/grcommon/grsector.c", r"lbRelocGetForceStatusBufferFile\(\(intptr_t\)&llFoxSpecial3FileID\)"),
    "static_texture_check": ("scripts/stages/check_nds_native_stage.py", r"_adapter_asset_sizes\(desc\) == DREAMLAND_ADAPTER_ASSET_SIZES"),
    "emitRows": ("scripts/stages/emit_native_stage_runtime_rows.py", r"sizes = list\(desc\.adapter_asset_sizes\)"),
    "ndsRelocNativeEntryHookComment": ("src/port/reloc_backend_assets.c", r"The native Arwing packet already contains its converted pixels"),
    "pupupuMaskPass": ("src/port/reloc_backend_assets.c", r"if \(\(gNdsStagePupupuRelocAssetMask & 0x1fu\) == 0x1fu\)"),
    "pupupuDiagDoc": ("docs/DIAGNOSTIC_REFERENCE.md", r"gNdsStagePupupuRelocAssetMask`: expected"),
    "assetIsStage": ("src/port/reloc_backend_assets.c", r"^static s32 ndsRelocAssetIsStage\("),
    "preload_comment_jungle": ("src/port/renderer_adapter_matrix.c", r"Congo Jungle's descriptor names bank 107"),
}


def find(path, pat):
    out = []
    p = REPO / path
    rx = re.compile(pat)
    with open(p, encoding="utf-8", errors="replace") as f:
        for i, line in enumerate(f, 1):
            if rx.search(line):
                out.append(i)
    return out


res = {}
for name, (path, pat) in CHECKS.items():
    res[name] = {"file": path, "lines": find(path, pat)}
    if not res[name]["lines"]:
        print("MISSING", name, path, pat)
out = Path(__file__).resolve().parents[1] / "lane1_citations.json"
out.write_text(json.dumps(res, indent=1))
for name, r in res.items():
    print(f"{name:34s} {r['file']}:{r['lines'][:4]}")
