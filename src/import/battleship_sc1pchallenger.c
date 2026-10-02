/* P2-6 step 8. Challenger-approaching screen (the unlocked fighter rotates in).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pchallenger.c whole
 * (381 lines: dSC1PChallengerFileIDs :15, lights, decals, rotating-fighter
 * maker + camera makers, FuncRun/FuncStart, sc1PChallengerStartScene :374),
 * following battleship_sc1pintro.c (scene TU with its taskman/video setup
 * and start/update/draw functions), NOT a data-only transcription.
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c, followed
 * here): the include OWNS every symbol it defines under its source name --
 * no renamed private copies, no ndsExcluded* duplicates. The only rename is
 * the scene entry, imported as ndsBase* and re-exported under its source
 * name (the battleship_sc1pbonusstage.c / battleship_scvsbattle.c seam),
 * so a later measured DS arena rebudget has a seam and the diff stays
 * reviewable. The adapter is a verbatim pass-through; no behaviour invented
 * here. Gated on NDS_P2_1P_GAME like the ladder tables and the runtime.
 *
 * Reloc files (dSC1PChallengerFileIDs :15):
 * - SC1PChallenger 0xa: STAGED (llSC1PChallengerFileID in
 *   src/port/diagnostics_mp_taskman_state.c:479, manifest rows in
 *   include/reloc_data.h:1395-1401, asset in src/port/reloc_backend_assets.c,
 *   NitroFS in NDS_1P_RELOC_FILES). All 4 sprites the TU draws
 *   (DecalExclaim, WarningText, ChallengerText, ApproachingText) are staged.
 *   No file the scene loads is unstaged.
 *
 * Fighter-side symbols the challenger resolves (all port-provided, no action):
 * ftManagerMakeFighter (battleship_ftmanager.c:94),
 * ftManagerSetupFilesAllKind (battleship_ftmanager.c:80),
 * ftManagerAllocFighter + gFTManagerFigatreeHeapSize
 * (reloc_backend_compat_shims.c:1639), dFTManagerDefaultFighterDesc
 * (include/ft/fighter.h:4158), ftParamGetCostumeCommonID
 * (reloc_backend_compat_shims.c:16271), ftParamCheckSetFighterColAnimID
 * (reloc_backend_compat_shims.c:1983), dSCSubsysFighterScales
 * (reloc_backend_fighter_display_seam.c:93). The challenger shows one fighter
 * (sSC1PChallengerFighterKind from gSCManagerSceneData.challenger_fkind); its
 * production data is the same manifest that backs the intro (ftchar_data_slots.c
 * slots + reloc_backend_ftdata_symbols.c), so nothing new is staged here. No
 * camera AnimJoint is consumed: both cameras are fixed (fighter cam :232-267,
 * decals cam :270-292), unlike the intro's AObj camera path.
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - No struct shim: the TU needs no FTDemoDesc and no bonus/tally types.
 * - BGM enumerator the TU names but the port header lacks (separate task widens
 *   it -- listed, NOT defined here): nSYAudioBGM1PChallenger (:369). Its numeric
 *   value can only come from the gm/gmsound.h promotion. nSYAudioFGMDeadUpStar
 *   (:370) is already in port include/gm/gmsound.h:60.
 * - Everything else the TU calls is port-provided, no action:
 *   lbRelocGetFileData / lbRelocInitSetup / lbRelocLoadFilesListed,
 *   gcMakeGObjSPAfter / gcAddGObjDisplay / gcAddGObjProcess /
 *   gcMakeCameraGObj / gcMakeDefaultCameraGObj / gcRunAll / gcDrawAll,
 *   lbCommonMakeSObjForGObj / lbCommonDrawSObjAttr / lbCommonDrawSprite /
 *   lbCommonClearExternSpriteParams, scSubsysControllerGetPlayerTapButtons /
 *   GetPlayerStickInRangeLR/UD (include/sc/scene.h), syTaskmanMalloc /
 *   syTaskmanSetLoadScene / syTaskmanStartTask, syVideoInit, syRdpSetViewport,
 *   syControllerFuncRead, syAudioPlayBGM (include/sys/audio.h),
 *   func_800269C0_275C0 (include/sys/audio.h:92), func_80017EC0
 *   (opening_movie_backend.c:4479), efParticleInitAll / efManagerInitEffects,
 *   ftGetStruct-adjacent DObj access via sys/obj.h, gSCManagerSceneData /
 *   gSYTaskmanDLHeads, lLBRelocTableAddr / llRelocFileCount,
 *   dLBCommonFuncMatrixList, ovl23_BSS_END + ovl1_VRAM (DECLARE_OVL in
 *   include/sc/scene.h covers both).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   sc1PChallengerStartScene (adapter below) vs
 *   src/port/title_backend.c:470 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <it/item.h>
#include <mn/menu.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/controller.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>
#include <nds/arm9/video.h>
#include <nds/nds_platform.h>
#include <nds/nds_renderer.h>

#define sc1PChallengerStartScene ndsBaseSC1PChallengerStartScene
void ndsBaseSC1PChallengerStartScene(void);

extern sb32 (*dLBCommonFuncMatrixList[])(void);
void sc1PChallengerFuncLights(Gfx **dls);
void sc1PChallengerFuncStart(void);
void efManagerInitEffects(void);

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
/* Transient native seam (src/port/renderer_adapter_fighter.c): the
 * challenger is one Demo actor that nothing registers, drawn through the
 * scratch binding the 1P intro and the ending use. Cleared at entry so the
 * last visit's arena pointers can never match this one's fighter. */
extern void ndsFighterIntroTransientReset(void);
#endif

/* THE SILHOUETTE'S BOX (sc1PChallengerDecalsProcDisplay, :125-138): a navy
 * PRIM rectangle, (207,92)-(287,216), that the fighter turns in front of --
 * the fighter camera (priority 40) draws after the decals camera (70). The
 * fighter's colanim (nGMColAnimFighterChallenger, gmcolscripts.c:1021-1027)
 * blends it fully to black, so without the box the silhouette is black on
 * the scene's black fill. The native renderer never reads an RDP fill: the
 * rectangle is taken here, where the source writes it, and painted into the
 * BG2 overlay, which sits behind BG0's 3D (nds_platform.c:645-647). Its DL
 * word becomes a no-op packet, which the sprite presenter's state drain
 * accepts, instead of a FILLRECT it reports as unpresented every frame. */
static u32 sNdsSC1PChallengerPrimRGBA;
static s16 sNdsSC1PChallengerBox[4];
static u32 sNdsSC1PChallengerBoxRGBA;
static sb32 sNdsSC1PChallengerBoxPainted;
static sb32 sNdsSC1PChallengerBackdropSet;

static void ndsSC1PChallengerWritePrimColor(Gfx *pkt, u32 m, u32 l, u32 r,
                                            u32 g, u32 b, u32 a)
{
    gDPSetPrimColor(pkt, m, l, r, g, b, a);
}

static void ndsSC1PChallengerPrimColor(Gfx *pkt, u32 m, u32 l, u32 r, u32 g,
                                       u32 b, u32 a)
{
    ndsSC1PChallengerWritePrimColor(pkt, m, l, r, g, b, a);
    sNdsSC1PChallengerPrimRGBA = ((r & 0xffu) << 24) | ((g & 0xffu) << 16) |
                                 ((b & 0xffu) << 8) | (a & 0xffu);
}

/* The source frame (10,10)-(310,230) onto the 256x192 screen: the window the
 * scene's sprites (nds_source2d.c) and its 3D viewport map with. */
static s32 ndsSC1PChallengerMap(s32 v, s32 size, s32 span)
{
    s32 mapped = (((v - 10) * size) + (span / 2)) / span;

    return (mapped < 0) ? 0 : ((mapped > size) ? size : mapped);
}

static void ndsSC1PChallengerPaintBox(const s16 box[4], u16 color)
{
    u32 pitch = 0u;
    u16 *layer = ndsPlatformGetOriginalSpriteOverlayLayer(FALSE, &pitch, NULL,
                                                          NULL, NULL);
    s32 x;
    s32 y;

    if ((layer == NULL) || (pitch == 0u))
    {
        ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_SPRITE,
            (u32)gSCManagerSceneData.scene_curr, 0xf6u, 0u, 0u, 0u,
            NDS_NATIVE_FAILURE_NO_PROGRAM);
        return;
    }
    ndsPlatformHideOriginalSpriteOverlayUntilCommit(FALSE);
    for (y = box[1]; y < box[3]; y++)
    {
        for (x = box[0]; x < box[2]; x++)
        {
            layer[((u32)y * pitch) + (u32)x] = color;
        }
    }
    (void)ndsPlatformCommitOriginalSpriteFinalLayer(FALSE,
        (u32)((box[2] - box[0]) * (box[3] - box[1])));
}

static void ndsSC1PChallengerFillRect(Gfx *pkt, s32 ulx, s32 uly, s32 lrx,
                                      s32 lry)
{
    u32 rgba = sNdsSC1PChallengerPrimRGBA;
    s16 box[4];

    NDS_GBI_ZERO_PACKET(pkt);
    box[0] = (s16)ndsSC1PChallengerMap(ulx, 256, 300);
    box[1] = (s16)ndsSC1PChallengerMap(uly, 192, 220);
    box[2] = (s16)ndsSC1PChallengerMap(lrx, 256, 300);
    box[3] = (s16)ndsSC1PChallengerMap(lry, 192, 220);
    if ((sNdsSC1PChallengerBoxPainted != FALSE) &&
        (box[0] == sNdsSC1PChallengerBox[0]) &&
        (box[1] == sNdsSC1PChallengerBox[1]) &&
        (box[2] == sNdsSC1PChallengerBox[2]) &&
        (box[3] == sNdsSC1PChallengerBox[3]) &&
        (rgba == sNdsSC1PChallengerBoxRGBA))
    {
        return;
    }
    if (sNdsSC1PChallengerBoxPainted != FALSE)
    {
        ndsSC1PChallengerPaintBox(sNdsSC1PChallengerBox, 0u);
    }
    ndsSC1PChallengerPaintBox(box, (u16)(0x8000u |
        RGB15((rgba >> 27) & 0x1fu, (rgba >> 19) & 0x1fu,
              (rgba >> 11) & 0x1fu)));
    sNdsSC1PChallengerBox[0] = box[0];
    sNdsSC1PChallengerBox[1] = box[1];
    sNdsSC1PChallengerBox[2] = box[2];
    sNdsSC1PChallengerBox[3] = box[3];
    sNdsSC1PChallengerBoxRGBA = rgba;
    sNdsSC1PChallengerBoxPainted = TRUE;
}

static void ndsSC1PChallengerDraw(void);

#undef gDPSetPrimColor
#define gDPSetPrimColor(pkt, m, l, r, g, b, a) \
    ndsSC1PChallengerPrimColor((pkt), (m), (l), (r), (g), (b), (a))
#undef gDPFillRectangle
#define gDPFillRectangle(pkt, ulx, uly, lrx, lry) \
    ndsSC1PChallengerFillRect((pkt), (ulx), (uly), (lrx), (lry))
#define gcDrawAll ndsSC1PChallengerDraw
#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pchallenger.c"
#undef gcDrawAll
#undef gDPFillRectangle
#undef gDPSetPrimColor

#undef sc1PChallengerStartScene

/* The scene's frame draw (dSC1PChallengerTaskmanSetup's gcDrawAll). Its
 * fighter is 3D, which BG0 shows only when a scene asks (every transition
 * hides it, nds_platform.c:1622), through the cameras' (10,10)-(310,230)
 * window, as the 1P intro and the staff roll present theirs. The backdrop
 * takes the default camera's black fill (:355) at the first draw, so a held
 * transition frame stays until this scene draws. */
static void ndsSC1PChallengerDraw(void)
{
    if (sNdsSC1PChallengerBackdropSet == FALSE)
    {
        ndsPlatformSetBackdropColor(RGB15(0, 0, 0));
        sNdsSC1PChallengerBackdropSet = TRUE;
    }
    ndsPlatformSet3DLayerEnabled(TRUE);
    ndsPlatformSet3DViewportSource(10, 10, 310, 230);
    gcDrawAll();
    ndsPlatformReset3DViewport();
}

void sc1PChallengerStartScene(void)
{
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    ndsFighterIntroTransientReset();
#endif
    sNdsSC1PChallengerPrimRGBA = 0u;
    sNdsSC1PChallengerBoxPainted = FALSE;
    sNdsSC1PChallengerBackdropSet = FALSE;
    ndsBaseSC1PChallengerStartScene();
}

#endif /* NDS_P2_1P_GAME */
