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
#if NDS_R2_PARTICLE_DRAW
    /* Before the reset, for the reason above: glResetTextures invalidates
     * every name, so anything holding one has to let go first. */
    ndsRendererHardwareDiscardParticleAtlas();
#endif
    ndsRendererHardwareResetSceneTextureVram();
    (void)ndsRendererHardwarePrepareBattleStaticTextures();
    (void)ndsIFCommonNativeOamPrepareClouds();
#if NDS_R2_PARTICLE_DRAW
    /* LAST, and the order is the fix rather than a preference.
     *
     * Measured 2026-08-01 on three tick-HUD ROMs that differ only in these
     * flags. Control and NDS_R2_PARTICLE_RUNTIME=1 both run a clean match with
     * ViolationCount 0 and StagePrepareBuildCount 2. Adding
     * NDS_R2_PARTICLE_DRAW=1 -- with the atlas prepared HERE, between the
     * static set and the interface -- aborts at the GO countdown
     * (ifCommonTrafficMakeSObj, MALLOCOVF=0, so not the heap) and, on the runs
     * that got past it, reported ViolationCount 1 with stage rebuilds 197.
     *
     * Raw capacity was never short: 262,144 texture VRAM, 136,192 static,
     * 57,344 for the interface's three A3I5 atlases (256x128 + 128x128 +
     * 128x64), 32,768 for this one, ~30 KB spare. What is short is a
     * CONTIGUOUS 32,768 run -- libnds splits blocks per bank, and this
     * allocator has already refused a 4,096-byte upload with 268,800 free
     * (PORTING.md, the second-entry corruption). Taking the largest free run
     * before the interface asks for one is what breaks it.
     *
     * So the interface gets first refusal and the cosmetic atlas fails closed:
     * AtlasFailCount rises, ndsRendererHardwareParticleAtlasName returns 0, and
     * the quad emit draws nothing. A silent loss of effects beats an abort at
     * the countdown. Still after the static set, so it stays above that span. */
    (void)ndsRendererHardwarePrepareParticleAtlas();
#endif
#if NDS_R2_IMPACT_WAVE_NATIVE && NDS_RENDERER_HW_TRIANGLES
    /* Tiny and last on purpose: five 16x32 PAL16 names cannot fragment the
     * allocator ahead of the large interface/particle allocations above. The
     * gameplay draw then performs a name bind only -- no N64 texel/TLUT decode,
     * conversion, cache lookup or texture allocation on an impact frame. */
    (void)ndsRendererHardwarePrepareImpactWaveTextures();
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE && NDS_RENDERER_HW_TRIANGLES
    /* Five tiny AOT-native names (four PAL16, one A5I3) after every large
     * scene allocation. Rebirth frames therefore bind only resident DS data. */
    (void)ndsRendererHardwarePrepareRebirthHaloTextures();
#endif
}

/* GAME SET never appeared -- owner, 2026-07-31: "No 'Game set' after winning
 * sudden death" -- and the cause is that this port never called the source's own
 * placement initializer.
 *
 * `ifCommonBattleInitPlacement` (decomp ifcommon.c:2558) counts the live teams
 * and sets `sIFCommonBattlePlace = teams - 1`. The announcement is driven by that
 * counter: when a team loses its last stock, `ifcommon.c:2735-2740` does
 * `sIFCommonBattlePlace--` and calls `ifCommonAnnounceEndMessage()` -- which is
 * the only VS path to `ifCommonAnnounceGameSetMakeInterface` -- **only if the
 * result is exactly 0**. Nothing in this tree called the initializer (the
 * original's caller is in one of the unmatched interface routines), so the
 * counter sat at its `.bss` zero, the first elimination took it to -1, and the
 * `== 0` test could never be true. Measured before changing anything: a Sudden
 * Death run read `SD-ANNOUNCE=place=0` at frame 40, before any death.
 *
 * That makes this a NORMAL-match defect too, not a Sudden-Death one: no VS match
 * of any length has ever announced GAME SET. It is called from both battle
 * entries for that reason, after `ndsBase...Start*` has populated
 * `gSCManagerBattleState->players[]`, which is what the initializer reads.
 *
 * Re-initialising per entry is also required rather than incidental: the counter
 * is a scene-lifetime static that the taskman arena rewind does not touch, so a
 * Sudden Death or rematch entry would otherwise inherit the previous match's
 * decremented value -- the same law as SwitchPlan 3.12. */
static void ndsSCVSBattleBeginScenePlacement(void)
{
    ifCommonBattleInitPlacement();
    gNdsSCVSBattlePlacementInitCount++;
    /* And the other half of making GAME SET survivable: the update proc that
     * announcement installs dereferences the two particle GObjs unconditionally,
     * and they are NULL while the particle runtime is off. See
     * battle_playable_compat_stubs.c for the measurement. */
    ndsEFParticleEnsureGObjPlaceholders();
}

void scVSBattleStartBattle(void)
{
    gNdsSCVSBattleOriginalFuncStartResult =
        NDS_SCVSBATTLE_ORIGINAL_FUNC_START_PASS;

    ndsBaseSCVSBattleStartBattle();
    ndsSCVSBattleBeginSceneTextures();
    ndsSCVSBattleBeginScenePlacement();
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
    ndsSCVSBattleBeginScenePlacement();
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

/* 81,904 RESERVED BYTES THIS PORT WILL NEVER WRITE.
 *
 * This is NOT the shield-freeze fix -- that is the missing
 * syTaskmanResetGraphicsHeap in taskman_seam.c, which has the derivation. This
 * is the waste the same investigation walked into on the way there, and it is
 * worth taking on its own.
 *
 * Measured at the freeze, twice:
 *
 *   DLBUF0   len=61,440  used=16
 *   DLBUF1   len=20,480  used=0
 *   DLBUF2/3 len=0
 *
 * dSCVSBattleTaskmanSetup is the N64's budget, and those two buffers are sized
 * for the RSP/RDP display-list pipeline that the DS hardware renderer replaced.
 * Sixteen bytes of 81,920 are ever written -- the gSPEndDisplayList the reset
 * path puts in each. DL buffer 0 keeps 16,384 (a thousand times its observed
 * use), DL buffer 1 keeps 4,096, the graphics heap stays at the source's own
 * 0xD000, and the remaining 61,440 bytes go back to gSYTaskmanGeneralHeap --
 * which lifts the free-space low-water (24,404 on 2026-08-02) clear of the
 * 25,600 threshold ifCommonSetMaxNumGObj latches the GObj cap at, and buys back
 * the arena margin that has been the stated blocker on the crowd actor and the
 * shield rim.
 *
 * Do NOT reduce a DL buffer to zero. syTaskmanCheckBufferLengths tests
 * `start + length < head` for all four and the reset writes into each, so a
 * zero-length buffer hangs at the OTHER `while (TRUE);` on decomp taskman.c:338
 * instead -- the same freeze wearing a different backtrace. */
#define NDS_R2_VSBATTLE_DL_BUFFER0_BYTES (sizeof(Gfx) * 2048u)
#define NDS_R2_VSBATTLE_DL_BUFFER1_BYTES (sizeof(Gfx) * 512u)
#define NDS_R2_VSBATTLE_GRAPHICS_ARENA_BYTES 0xD000u

/* Engagement proof: the re-budget must be visible without a debugger, because
 * "the setup struct says X" and "the scene was built with X" have already
 * differed once in this campaign. */
volatile u32 gNdsSCVSBattleRebudgetCount;
volatile u32 gNdsSCVSBattleRebudgetGraphicsBytes;

static void ndsSCVSBattleRebudgetSceneArena(void)
{
    dSCVSBattleTaskmanSetup.scene_setup.dl_buffer0_size =
        NDS_R2_VSBATTLE_DL_BUFFER0_BYTES;
    dSCVSBattleTaskmanSetup.scene_setup.dl_buffer1_size =
        NDS_R2_VSBATTLE_DL_BUFFER1_BYTES;
    dSCVSBattleTaskmanSetup.scene_setup.graphics_arena_size =
        NDS_R2_VSBATTLE_GRAPHICS_ARENA_BYTES;
    gNdsSCVSBattleRebudgetCount++;
    gNdsSCVSBattleRebudgetGraphicsBytes =
        (u32)dSCVSBattleTaskmanSetup.scene_setup.graphics_arena_size;
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
    ndsSCVSBattleRebudgetSceneArena();
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

    ndsSCVSBattleRebudgetSceneArena();
    setup = ndsSCVSBattleMakeTaskmanSetup();
    scManagerFuncUpdate(&setup);
#endif
}
