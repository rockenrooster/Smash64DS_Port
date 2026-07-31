/* Compile the original BattleShip VS Battle translation unit. Live-input
 * battle_playable uses the original scene tail through a DS arena adapter;
 * bounded harness builds retain their short taskman path below. */
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <mn/menu.h>
#include <nds/nds_audio_assets.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/nds_startup.h>
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

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);
extern s32 gcGetGObjsActiveNum(void);
extern u32 sGCCamerasActiveNum;
extern u32 sGCSpritesActiveNum;
extern s32 sSYTaskmanStatus;
extern sb32 (*dLBCommonFuncMatrixList[])(void);

void scVSBattleSetupFiles(void);
void scVSBattleFuncUpdate(void);
void scVSBattleFuncLights(Gfx **dls);
void scVSBattleStartBattle(void);
void gmCameraSetViewportDimensions(s32 ulx, s32 uly, s32 lrx, s32 lry);
GObj *gmCameraMakeWallpaperCamera(void);
#if NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE
void gmCameraMakeBattleCamera(void);
void gmCameraMakePlayerArrowsCamera(void);
void gmCameraMakePlayerMagnifyCamera(void);
void gmCameraScreenFlashMakeCamera(void);
#else
GObj *gmCameraMakeBattleCamera(void);
GObj *gmCameraMakePlayerArrowsCamera(void);
GObj *gmCameraMakePlayerMagnifyCamera(void);
GObj *gmCameraScreenFlashMakeCamera(void);
#endif
GObj *gmCameraMakeInterfaceCamera(void);
GObj *gmCameraMakeEffectCamera(void);
void itManagerInitItems(void);
void wpManagerAllocWeapons(void);
void efManagerInitEffects(void);
void gmRumbleMakeActor(void);
void gmRumbleInitPlayers(void);
void ndsSCVSBattleManagerFuncUpdate(SYTaskmanSetup *setup);

#define scVSBattleStartScene ndsBaseSCVSBattleStartScene
#define scVSBattleStartBattle ndsBaseSCVSBattleStartBattle
#define scVSBattleStartSuddenDeath ndsBaseSCVSBattleStartSuddenDeath
#define scVSBattleFuncUpdate ndsBaseSCVSBattleFuncUpdate
#define scManagerFuncUpdate ndsSCVSBattleManagerFuncUpdate
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
#define ftManagerMakeFighter ndsSCVSBattleFTManagerMakeFighter
#endif

void ndsBaseSCVSBattleStartScene(void);
void ndsBaseSCVSBattleStartBattle(void);
void ndsBaseSCVSBattleStartSuddenDeath(void);
void ndsBaseSCVSBattleFuncUpdate(void);
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
GObj *ndsSCVSBattleFTManagerMakeFighter(FTDesc *desc);
#endif

#include "../../decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattlefiles.c"

#undef scVSBattleStartScene
#undef scVSBattleStartBattle
#undef scVSBattleStartSuddenDeath
#undef scVSBattleFuncUpdate
#undef scManagerFuncUpdate
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
#undef ftManagerMakeFighter
#endif

/* Defined below, next to `scVSBattleStartBattle`. Declared here because the
 * adapter remaps `func_start` onto it and runs first in this file. */
void scVSBattleStartSuddenDeath(void);

void ndsSCVSBattleManagerFuncUpdate(SYTaskmanSetup *setup)
{
    SYTaskmanSetup ds_setup = *setup;

    ds_setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    ds_setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    if (ds_setup.scene_setup.func_update == ndsBaseSCVSBattleFuncUpdate)
    {
        ds_setup.scene_setup.func_update = scVSBattleFuncUpdate;
    }
    if (ds_setup.func_start == ndsBaseSCVSBattleStartBattle)
    {
        ds_setup.func_start = scVSBattleStartBattle;
    }
    /* Same remap for the other entry into this scene. Without it Sudden Death
     * kept the decomp start even once a wrapper existed, so the setup table and
     * this adapter have to agree. */
    if (ds_setup.func_start == ndsBaseSCVSBattleStartSuddenDeath)
    {
        ds_setup.func_start = scVSBattleStartSuddenDeath;
    }
    gNdsSCVSBattleLifecycleArenaAdapterCount++;
    scManagerFuncUpdate(&ds_setup);
}

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
GObj *ndsSCVSBattleFTManagerMakeFighter(FTDesc *desc)
{
    if (desc != NULL)
    {
        /* BattleShip scVSBattleFuncStart sets this before ftManagerMakeFighter. */
        desc->is_skip_entry = TRUE;
    }
    return ftManagerMakeFighter(desc);
}
#endif

#if !NDS_DEV_LIVE_INPUT_PREVIEW
static SYTaskmanSetup ndsSCVSBattleMakeTaskmanSetup(void)
{
    SYTaskmanSetup setup = dSCVSBattleTaskmanSetup;

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = scVSBattleStartBattle;
    return setup;
}
#endif

/* The DS-side texture preparation every entry into `nSCKindVSBattle` performs,
 * in one place so that entry one, a START rematch, and Sudden Death cannot drift
 * apart -- which is exactly how the second-entry corruption happened: the calls
 * matched and their effects did not.
 *
 * Order is load-bearing. The atlas release has to precede the allocator reset
 * (`glResetTextures` invalidates every texture name, so a software owner still
 * holding one must let go first), and the reset has to precede both prepares so
 * they upload into an empty pool. Cost is one atlas rebuild per entry at
 * scene-entry time, which is the cheap side of the load/gameplay trade this
 * project always takes. */
static void ndsSCVSBattleBeginSceneTextures(void)
{
    ndsIFCommonNativeOamReleaseCloudTextures();
    ndsRendererHardwareResetSceneTextureVram();
    (void)ndsRendererHardwarePrepareBattleStaticTextures();
    (void)ndsIFCommonNativeOamPrepareClouds();
}

void scVSBattleStartBattle(void)
{
    gNdsSCVSBattleOriginalFuncStartResult =
        NDS_SCVSBATTLE_ORIGINAL_FUNC_START_PASS;

    ndsBaseSCVSBattleStartBattle();
    ndsSCVSBattleBeginSceneTextures();
#if NDS_R2_ANIM_CACHE
    /* R2-04 E4/E5. Same prepare-at-load seam as the two above, but armed here
     * and stepped from scVSBattleFuncUpdate: the match's animation streams
     * become resident during the countdown so no gameplay frame pays a NitroFS
     * walk and a cartridge read for a move. Doing all 41 here missed a BGM
     * buffer seam and killed the music. */
    ndsR2AnimCachePreloadMatch();
#endif

    gNdsSCVSBattleOriginalGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsSCVSBattleOriginalCameraCount = sGCCamerasActiveNum;
    gNdsSCVSBattleOriginalMainGObjID = 0xffffffffu;
    gNdsSCVSBattleOriginalPlayerCount = gSCManagerBattleState->pl_count;
    gNdsSCVSBattleOriginalCpuCount = gSCManagerBattleState->cp_count;
    gNdsSCVSBattleOriginalGameRule = gSCManagerBattleState->game_rules;
    gNdsSCVSBattleOriginalTime = gSCManagerBattleState->time_limit;
    gNdsSCVSBattleOriginalStock = gSCManagerBattleState->stocks;
    gNdsSCVSBattleOriginalIsTeam = gSCManagerBattleState->is_team_battle;
    gNdsSCVSBattleOriginalGKind = gSCManagerBattleState->gkind;
    gNdsSCVSBattleOriginalScenePrev = gSCManagerSceneData.scene_prev;
    gNdsSCVSBattleOriginalSceneCurr = gSCManagerSceneData.scene_curr;

    if (gNdsSCVSBattleOriginalLoadedFileCount == 8u)
    {
        gNdsSCVSBattleOriginalSetupMask |=
            NDS_SCVSBATTLE_SETUP_FILES_READY;
    }
    if (sGCCamerasActiveNum >= 1u)
    {
        gNdsSCVSBattleOriginalSetupMask |=
            NDS_SCVSBATTLE_SETUP_DEFAULT_CAMERA_READY;
    }
    if (gNdsSCVSBattleCompatManagerMask != 0u)
    {
        gNdsSCVSBattleOriginalSetupMask |=
            NDS_SCVSBATTLE_SETUP_MANAGER_STUBS_READY;
    }
    if (gNdsSCVSBattleOriginalFighterGObjCount != 0u)
    {
        gNdsSCVSBattleOriginalSetupMask |=
            NDS_SCVSBATTLE_SETUP_FIGHTER_DESCS_READY;
    }
    if (gNdsSCVSBattleCompatInterfaceMask != 0u)
    {
        gNdsSCVSBattleOriginalSetupMask |=
            NDS_SCVSBATTLE_SETUP_INTERFACE_STUBS_READY;
    }
    if (gNdsSCVSBattleCompatAudioMask != 0u)
    {
        gNdsSCVSBattleOriginalSetupMask |=
            NDS_SCVSBATTLE_SETUP_AUDIO_STUBS_READY;
    }

    gNdsSCVSBattleOriginalSetupResult =
        NDS_SCVSBATTLE_ORIGINAL_SETUP_PASS;
}

/* Sudden Death is a SECOND entry into the battle scene, and until now it was the
 * only one with no port wrapper at all: the `scVSBattleStartSuddenDeath`
 * define/undef pair existed above, but nothing replaced the symbol, so the
 * decomp's own start ran bare. Every piece of DS-side battle preparation that
 * `scVSBattleStartBattle` performs was therefore skipped -- the hardware static
 * textures, the native OAM cloud textures, and the animation-cache warm cursor.
 * A Sudden Death drawn against textures the scene load already tore down is the
 * corruption the owner reported, and `ndsSCVSBattleManagerFuncUpdate` below
 * compounded it by remapping `func_start` for the battle case only.
 *
 * The three prepares are safe to repeat: the texture prepare early-returns while
 * `sNdsRendererBattleStaticTexturePrepared` holds (and
 * `ndsRendererHardwareDiscardTextureCache` clears it when the cache is actually
 * dropped, counting a violation if it was still armed), the cloud prepare
 * early-returns once its texture names are non-zero, and the anim-cache preload
 * only rewinds a cursor. So this is the same call sequence, not a second one.
 *
 * R2-07 E2 CORRECTION: "the same call sequence" was true of the calls and false
 * of their effect, and that gap is the second-entry corruption. The cloud
 * prepare's early return is exactly the problem -- those three atlas textures
 * are the only ones that survive `ndsRendererHardwareDiscardTextureCache`, so
 * the static prepare below frees and re-uploads its 24 pinned textures AROUND
 * atlas blocks that entry one placed after them. libnds allocates texture VRAM
 * inside glTexImage2D, so the same bytes in a different order leave the dynamic
 * stage textures unable to fit: entry one's texture-reject mask is 0, entry
 * two's is 0x1000 (TEXIMAGE), which fails PrepareRun for run 42 and rejects the
 * entire native stage owner. The stage then draws from whatever the fallback
 * has, which is what looked like corrupt geometry.
 *
 * R2-07 E3/E4 CLOSED it one level down: releasing the atlases only reproduces
 * entry one's allocation ORDER, and reproducing a layout is not owning one. The
 * scene now resets the texture allocator itself
 * (`ndsSCVSBattleBeginSceneTextures` above), so this entry and every other start
 * from an empty pool. */
void scVSBattleStartSuddenDeath(void)
{
    ndsBaseSCVSBattleStartSuddenDeath();
    ndsSCVSBattleBeginSceneTextures();
#if NDS_R2_ANIM_CACHE
    ndsR2AnimCachePreloadMatch();
#endif
    gNdsSCVSBattleSuddenDeathPrepareCount++;
}

void scVSBattleFuncUpdate(void)
{
    ndsBaseSCVSBattleFuncUpdate();

#if NDS_R2_ANIM_CACHE
    ndsR2AnimCachePreloadStep();
#endif

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    if (ndsFighterMarioFoxNaturalMotionUpdateEnabled() != FALSE)
    {
        gNdsFighterNaturalMotionBaseVSBattleUpdateCount++;
        ndsFighterMarioFoxNaturalMotionRunVSBattleUpdate();
    }
    else
#endif
    if (ndsFighterMarioFoxLivePreviewUpdateEnabled() != FALSE)
    {
        gNdsFighterLivePreviewBaseVSBattleUpdateCount++;
        ndsFighterMarioFoxLivePreviewRunVSBattleUpdate();
    }
    else if (ndsFighterMarioFoxGCDrawAllLoopUpdateEnabled() != FALSE)
    {
        gNdsFighterGCDrawAllLoopBaseVSBattleUpdateCount++;
        ndsFighterMarioFoxGCDrawAllLoopRunVSBattleUpdate();
    }
    else if (ndsFighterMarioFoxGCRunAllLoopUpdateEnabled() != FALSE)
    {
        gNdsFighterGCRunAllLoopBaseVSBattleUpdateCount++;
        ndsFighterMarioFoxGCRunAllLoopRunVSBattleUpdate();
    }
    else if (ndsFighterMarioFoxPreviewLoopUpdateEnabled() != FALSE)
    {
        gNdsFighterPreviewLoopBaseVSBattleUpdateCount++;
        ndsFighterMarioFoxPreviewLoopRunVSBattleUpdate();
    }
    else if (ndsFighterMarioFoxControllerLoopUpdateEnabled() != FALSE)
    {
        gNdsFighterControllerLoopBaseVSBattleUpdateCount++;
        ndsFighterMarioFoxControllerLoopRunVSBattleUpdate();
    }
    else if (ndsFighterMarioFoxSchedulerLoopUpdateEnabled() != FALSE)
    {
        gNdsFighterSchedulerLoopBaseVSBattleUpdateCount++;
        ndsFighterMarioFoxSchedulerLoopRunVSBattleUpdate();
    }
}

void scVSBattleStartScene(void)
{
#if NDS_DEV_LIVE_INPUT_PREVIEW
    gNdsSCVSBattleOriginalStartResult =
        NDS_SCVSBATTLE_ORIGINAL_START_PASS;

    /* Stage the bounded DS-native audio bank at the scene boundary.  The
     * original func_start immediately plays PublicExcited, before the DS
     * task loop begins, so loading from syTaskmanRunTask is too late. */
#if NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS
    ndsAudioAssetLoadFenced();
#endif

    /* The N64 validation overlay is not staged on DS. */
    gSCManagerBackupData.boot = 0;
    ndsBaseSCVSBattleStartScene();
    ndsRendererHardwareDiscardBattleStaticTextures();

    gNdsSCVSBattleLifecycleScenePrev = gSCManagerSceneData.scene_prev;
    gNdsSCVSBattleLifecycleSceneCurr = gSCManagerSceneData.scene_curr;
    gNdsSCVSBattleLifecycleIsSuddenDeath =
        (u32)gSCManagerSceneData.is_suddendeath;
    if ((gNdsSCVSBattleLifecycleTaskmanExitCount != 0u) &&
        (gNdsSCVSBattleLifecycleTaskmanStatus ==
            nSYTaskmanStatusLoadScene) &&
        (gNdsSCVSBattleLifecycleTimeRemain == 0u) &&
        (gSCManagerSceneData.scene_prev == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSResults))
    {
        gNdsSCVSBattleLifecycleResult =
            NDS_SCVSBATTLE_LIFECYCLE_PASS;
    }
#else
    SYTaskmanSetup setup;

    gNdsSCVSBattleOriginalStartResult =
        NDS_SCVSBATTLE_ORIGINAL_START_PASS;

    gSCManagerBattleState = &gSCManagerTransferBattleState;
    gSCManagerBattleState->game_type = nSCBattleGameTypeRoyal;
    gSCManagerBattleState->gkind = gSCManagerSceneData.gkind;

    if (gSCManagerBackupData.error_flags & LBBACKUP_ERROR_VSBATTLECASTLE)
    {
        gSCManagerBattleState->gkind = nGRKindCastle;
    }

    /* Keep the bounded setup proof out of the original anti-tamper tail. */
    gSCManagerBackupData.boot = 0;

    syVideoInit(&dSCVSBattleVideoSetup);

    setup = ndsSCVSBattleMakeTaskmanSetup();
    scManagerFuncUpdate(&setup);
#endif
}
