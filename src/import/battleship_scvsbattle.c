/* Compile the original BattleShip VS Battle setup translation unit.
 *
 * The DS entry is deliberately bounded: it runs original common battle file
 * loading and setup through fighter-descriptor creation, then parks before
 * continuous gameplay/update/draw, sudden death, results, or audio backend
 * behavior.
 */
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <mn/menu.h>
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
GObj *gmCameraMakeBattleCamera(void);
GObj *gmCameraMakePlayerArrowsCamera(void);
GObj *gmCameraMakePlayerMagnifyCamera(void);
GObj *gmCameraScreenFlashMakeCamera(void);
GObj *gmCameraMakeInterfaceCamera(void);
GObj *gmCameraMakeEffectCamera(void);
void itManagerInitItems(void);
void wpManagerAllocWeapons(void);
void efManagerInitEffects(void);
void gmRumbleMakeActor(void);
void gmRumbleInitPlayers(void);

#define scVSBattleStartScene ndsBaseSCVSBattleStartScene
#define scVSBattleStartBattle ndsBaseSCVSBattleStartBattle
#define scVSBattleStartSuddenDeath ndsBaseSCVSBattleStartSuddenDeath

void ndsBaseSCVSBattleStartScene(void);
void ndsBaseSCVSBattleStartBattle(void);
void ndsBaseSCVSBattleStartSuddenDeath(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c"
#include "../../decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattlefiles.c"

#undef scVSBattleStartScene
#undef scVSBattleStartBattle
#undef scVSBattleStartSuddenDeath

static SYTaskmanSetup ndsSCVSBattleMakeTaskmanSetup(void)
{
    SYTaskmanSetup setup = dSCVSBattleTaskmanSetup;

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = scVSBattleStartBattle;
    return setup;
}

void scVSBattleStartBattle(void)
{
    gNdsSCVSBattleOriginalFuncStartResult =
        NDS_SCVSBATTLE_ORIGINAL_FUNC_START_PASS;

    ndsBaseSCVSBattleStartBattle();

    gNdsSCVSBattleOriginalGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsSCVSBattleOriginalCameraCount = sGCCamerasActiveNum;
    gNdsSCVSBattleOriginalMainGObjID = 0xffffffffu;
    gNdsSCVSBattleOriginalPlayerCount = gSCManagerBattleState->pl_count;
    gNdsSCVSBattleOriginalCpuCount = gSCManagerBattleState->cp_count;
    gNdsSCVSBattleOriginalGameRule = gSCManagerBattleState->game_rules;
    gNdsSCVSBattleOriginalStock = gSCManagerBattleState->stocks;
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

void scVSBattleStartScene(void)
{
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
}
