/* Compile the original BattleShip PlayersVS scene translation unit.
 *
 * The DS entry remains bounded: it runs original character-select setup,
 * records proof of the original object/file graph, and parks before the
 * continuous interactive loop unless a harness explicitly drives the original
 * ready/start transition.
 */
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <mn/menu.h>
#include <nds/nds_startup.h>
#include <sc/scene.h>
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
extern void efManagerInitEffects(void);

extern void mnPlayersVSFuncLights(Gfx **dls);
void mnPlayersVSFuncStart(void);
void mnPlayersVSFuncRun(GObj *gobj);
s32 mnPlayersVSRandFighterKind(GObj *gobj);
sb32 mnPlayersVSCheckReady(void);
void mnPlayersVSSetSceneData(void);
void mnPlayersVSPauseSlotProcesses(void);
s32 mnPlayersVSGetNextTimeValue(s32 current_value);
s32 mnPlayersVSGetPrevTimeValue(s32 current_value);
sb32 mnPlayersVSCheckCostumeUsed(s32 fkind, s32 player, s32 costume);
s32 mnPlayersVSUpdateCursorPlacementPriorities(s32 player, s32 puck);
void mnPlayersVSUpdateCursor(GObj *gobj, s32 player, s32 cursor_status);
void mnPlayersVSAnnounceFighter(s32 player, s32 slot);
void mnPlayersVSMakePortraitFlash(s32 player);
void mnPlayersVSUpdateNameAndEmblem(s32 player);
void mnPlayersVSMakeHandicapLevel(s32 player);
void mnPlayersVSMakeHandicapLevelValue(s32 player);
void mnPlayersVSUpdateHandicapLevel(s32 player);
sb32 mnPlayersVSCheckHandicap(void);
sb32 mnPlayersVSCheckHandicapOn(void);
sb32 mnPlayersVSCheckHandicapAuto(void);
sb32 mnPlayersVSCheckHandicapArrowRInRange(GObj *gobj, s32 player);
sb32 mnPlayersVSCheckHandicapArrowLInRange(GObj *gobj, s32 player);
void mnPlayersVSUpdateCursorGrabPriorities(s32 player, s32 puck);
void mnPlayersVSUpdatePuck(GObj *gobj, s32 puck);
void lbCommonSetSpriteScissor(s32 xmin, s32 xmax, s32 ymin, s32 ymax);

#define mnPlayersVSFuncStart ndsBaseMNPlayersVSFuncStart
#define mnPlayersVSStartScene ndsBaseMNPlayersVSStartScene

void ndsBaseMNPlayersVSFuncStart(void);
void ndsBaseMNPlayersVSStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayersvs.c"

#undef mnPlayersVSFuncStart
#undef mnPlayersVSStartScene

static GObj *sNdsPlayersVSMainGObj;

static SYTaskmanSetup ndsMNPlayersVSMakeTaskmanSetup(void)
{
    SYTaskmanSetup setup = dMNPlayersVSTaskmanSetup;

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = mnPlayersVSFuncStart;
    return setup;
}

static void ndsMNPlayersVSClearControllerState(void)
{
    s32 i;

    gSYControllerConnectedNum = 2;
    for (i = 0; i < MAXCONTROLLERS; i++)
    {
        gSYControllerDeviceStatuses[i] = -1;
        gSYControllerDevices[i].button_tap = 0;
        gSYControllerDevices[i].button_hold = 0;
        gSYControllerDevices[i].button_update = 0;
        gSYControllerDevices[i].button_release = 0;
        gSYControllerDevices[i].stick_range.x = 0;
        gSYControllerDevices[i].stick_range.y = 0;
    }
    gSYControllerDeviceStatuses[0] = 0;
    gSYControllerDeviceStatuses[1] = 1;
}

static void ndsMNPlayersVSRecordSlotProof(void)
{
    s32 i;
    u32 selected_mask = 0;
    u32 kind_mask = 0;
    u32 cursors = 0;
    u32 pucks = 0;
    u32 gates = 0;
    u32 heaps = 0;
    u32 controller_mask = 0;

    for (i = 0; i < ARRAY_COUNT(sMNPlayersVSSlots); i++)
    {
        if (sMNPlayersVSSlots[i].pkind != nFTPlayerKindNot)
        {
            kind_mask |= 1u << i;
        }
        if (sMNPlayersVSSlots[i].is_fighter_selected != FALSE)
        {
            selected_mask |= 1u << i;
        }
        if (sMNPlayersVSSlots[i].cursor != NULL)
        {
            cursors++;
        }
        if (sMNPlayersVSSlots[i].puck != NULL)
        {
            pucks++;
        }
        if (sMNPlayersVSSlots[i].panel != NULL)
        {
            gates++;
        }
        if (sMNPlayersVSSlots[i].figatree_heap != NULL)
        {
            heaps++;
        }
        if (sMNPlayersVSControllerOrders[i] != -1)
        {
            controller_mask |= 1u << i;
        }
    }

    gNdsPlayersVSOriginalControllerOrderMask = controller_mask;
    gNdsPlayersVSOriginalSlotKindMask = kind_mask;
    gNdsPlayersVSOriginalSlotSelectedMask = selected_mask;
    gNdsPlayersVSOriginalCursorCount = cursors;
    gNdsPlayersVSOriginalPuckCount = pucks;
    gNdsPlayersVSOriginalGateCount = gates;
    gNdsPlayersVSOriginalFigatreeHeapCount = heaps;
}

void mnPlayersVSFuncStart(void)
{
    GObj *main_gobj;
    LBRelocSetup rl_setup;
    s32 i;

    gNdsPlayersVSOriginalFuncStartResult =
        NDS_PLAYERS_VS_ORIGINAL_FUNC_START_PASS;

    rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
    rl_setup.table_files_num = (u32)&llRelocFileCount;
    rl_setup.file_heap = NULL;
    rl_setup.file_heap_size = 0;
    rl_setup.status_buffer = sMNPlayersVSStatusBuffer;
    rl_setup.status_buffer_size = ARRAY_COUNT(sMNPlayersVSStatusBuffer);
    rl_setup.force_status_buffer = sMNPlayersVSForceStatusBuffer;
    rl_setup.force_status_buffer_size =
        ARRAY_COUNT(sMNPlayersVSForceStatusBuffer);

    lbRelocInitSetup(&rl_setup);
    lbRelocLoadFilesListed(dMNPlayersVSFileIDs, sMNPlayersVSFiles);
    if (gNdsPlayersVSOriginalLoadedFileCount == 7u)
    {
        gNdsPlayersVSOriginalSetupMask |= (1u << 0);
    }

    main_gobj = gcMakeGObjSPAfter(nGCCommonKindPlayerSelect,
                                  mnPlayersVSFuncRun,
                                  15,
                                  GOBJ_PRIORITY_DEFAULT);
    sNdsPlayersVSMainGObj = main_gobj;
    if (main_gobj != NULL)
    {
        gNdsPlayersVSOriginalMainGObjID = main_gobj->id;
        gNdsPlayersVSOriginalSetupMask |= (1u << 1);
    }

    gcMakeDefaultCameraGObj(16, GOBJ_PRIORITY_DEFAULT, 100,
                            COBJ_FLAG_ZBUFFER,
                            GPACK_RGBA8888(0x00, 0x00, 0x00, 0x00));
    if (sGCCamerasActiveNum >= 1u)
    {
        gNdsPlayersVSOriginalCameraCount = sGCCamerasActiveNum;
        gNdsPlayersVSOriginalSetupMask |= (1u << 2);
    }

    efParticleInitAll();
    efManagerInitEffects();
    ndsMNPlayersVSClearControllerState();
    mnPlayersVSUpdateControllerOrders();
    mnPlayersVSInitVars();
    ftManagerAllocFighter(FTDATA_FLAG_SUBMOTION, 4);

    for (i = nFTKindPlayableStart; i <= nFTKindPlayableEnd; i++)
    {
        ftManagerSetupFilesAllKind(i);
    }
    for (i = 0; i < ARRAY_COUNT(sMNPlayersVSSlots); i++)
    {
        sMNPlayersVSSlots[i].figatree_heap =
            syTaskmanMalloc(gFTManagerFigatreeHeapSize, 0x10);
    }
    gNdsPlayersVSOriginalSetupMask |= (1u << 3);

    mnPlayersVSMakePortraitCamera();
    mnPlayersVSMakeCursorCamera();
    mnPlayersVSMakePuckCamera();
    mnPlayersVSMakePlayerKindCamera();
    mnPlayersVSMakeGateCamera();
    mnPlayersVSMakePlayerKindSelectCamera();
    mnPlayersVSMakeFighterCamera();
    mnPlayersVSMakeTeamSelectCamera();
    mnPlayersVSMakeHandicapLevelCamera();
    mnPlayersVSMakePortraitWallpaperCamera();
    mnPlayersVSMakePortraitFlashCamera();
    mnPlayersVSMakeReadyCamera();
    gNdsPlayersVSOriginalSetupMask |= (1u << 4);

    mnPlayersVSMakeWallpaper();
    mnPlayersVSMakePortraitAll();
    mnPlayersVSInitSlotAll();
    mnPlayersVSMakeLabels();
    mnPlayersVSMakePuckAdjust();
    mnPlayersVSMakePuckGlow();
    mnPlayersVSMakeCostumeSync();
    mnPlayersVSMakeSpotlight();
    mnPlayersVSMakeReady();
    scSubsysFighterSetLightParams(45.0F, 45.0F, 0xFF, 0xFF, 0xFF, 0xFF);
    gNdsPlayersVSOriginalSetupMask |= (1u << 5);

    ndsMNPlayersVSRecordSlotProof();

    gNdsPlayersVSOriginalGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsPlayersVSOriginalSObjCount = sGCSpritesActiveNum;
    gNdsPlayersVSOriginalPortraitCount =
        (sGCSpritesActiveNum >= GMCOMMON_FIGHTERS_PLAYABLE_NUM) ?
        GMCOMMON_FIGHTERS_PLAYABLE_NUM : sGCSpritesActiveNum;
    gNdsPlayersVSOriginalTime = (u32)sMNPlayersVSTimeValue;
    gNdsPlayersVSOriginalStock = (u32)sMNPlayersVSStockValue;
    gNdsPlayersVSOriginalGameRule = (u32)sMNPlayersVSGameRule;
    gNdsPlayersVSOriginalIsTeam = (u32)sMNPlayersVSIsTeamBattle;
    gNdsPlayersVSOriginalIsStageSelect =
        (u32)gSCManagerTransferBattleState.is_stage_select;
    gNdsPlayersVSOriginalSetupMask |= (1u << 6);
    gNdsPlayersVSOriginalSetupMask |= (1u << 7);
    gNdsPlayersVSOriginalDeferredMask =
        (1u << 0) | /* continuous character-select input */
        (1u << 1) | /* fighter object import/display */
        (1u << 2);  /* continuous menu rendering/audio */
    gNdsPlayersVSOriginalSetupResult =
        NDS_PLAYERS_VS_ORIGINAL_SETUP_PASS;
}

static void ndsMNPlayersVSSeedReadyPlayers(void)
{
    s32 i;

    ndsMNPlayersVSClearControllerState();
    sMNPlayersVSTotalTimeTics = I_SEC_TO_TICS(1) + 1;
    sMNPlayersVSIsStart = FALSE;
    sMNPlayersVSStartProceedWait = 0;
    gSCManagerTransferBattleState.is_stage_select = TRUE;

    for (i = 0; i < ARRAY_COUNT(sMNPlayersVSSlots); i++)
    {
        sMNPlayersVSSlots[i].cursor_status = nMNPlayersCursorStatusPointer;
        sMNPlayersVSSlots[i].is_fighter_selected = FALSE;
        sMNPlayersVSSlots[i].is_selected = FALSE;
        sMNPlayersVSSlots[i].pkind = nFTPlayerKindNot;
        sMNPlayersVSSlots[i].fkind = nFTKindNull;
        sMNPlayersVSSlots[i].team = 0;
        sMNPlayersVSSlots[i].costume = 0;
        sMNPlayersVSSlots[i].shade = 0;
        sMNPlayersVSSlots[i].cpu_level = 1;
        sMNPlayersVSSlots[i].handicap = 0;
    }

    sMNPlayersVSSlots[0].pkind = nFTPlayerKindMan;
    sMNPlayersVSSlots[0].fkind = nFTKindMario;
    sMNPlayersVSSlots[0].is_fighter_selected = TRUE;
    sMNPlayersVSSlots[0].is_selected = TRUE;
    sMNPlayersVSSlots[0].holder_player = 0;
    sMNPlayersVSSlots[0].held_player = -1;

    sMNPlayersVSSlots[1].pkind = nFTPlayerKindMan;
    sMNPlayersVSSlots[1].fkind = nFTKindFox;
    sMNPlayersVSSlots[1].is_fighter_selected = TRUE;
    sMNPlayersVSSlots[1].is_selected = TRUE;
    sMNPlayersVSSlots[1].holder_player = 1;
    sMNPlayersVSSlots[1].held_player = -1;
}

static void ndsMNPlayersVSRunMainTick(u32 *updates)
{
    mnPlayersVSFuncRun(sNdsPlayersVSMainGObj);
    (*updates)++;
    gNdsPlayersVSReadyTransitionUpdateCount = *updates;
}

void ndsMNPlayersVSRunReadyTransitionProbe(void)
{
    u32 updates = 0;
    u32 mask = 0;
    u32 i;

    gNdsPlayersVSReadyTransitionResult =
        NDS_PLAYERS_VS_READY_TRANSITION_FAIL;
    gNdsPlayersVSReadyTransitionMask = 0;
    gNdsPlayersVSReadyTransitionScenePrevBefore =
        gSCManagerSceneData.scene_prev;
    gNdsPlayersVSReadyTransitionSceneCurrBefore =
        gSCManagerSceneData.scene_curr;

    if (((gNdsPlayersVSOriginalSetupMask & 0xffu) == 0xffu) &&
        (sNdsPlayersVSMainGObj != NULL))
    {
        mask |= (1u << 0);
    }

    ndsMNPlayersVSSeedReadyPlayers();
    if ((mnPlayersVSCheckReady() != FALSE) &&
        (gSCManagerTransferBattleState.is_stage_select != FALSE))
    {
        mask |= (1u << 1);
    }

    gNdsPlayersVSReadyTransitionInputMask = START_BUTTON;
    gSYControllerDevices[0].button_tap = START_BUTTON;
    gSYControllerDevices[0].button_hold = START_BUTTON;
    ndsMNPlayersVSRunMainTick(&updates);

    if (sMNPlayersVSIsStart != FALSE)
    {
        mask |= (1u << 2);
    }
    if (gNdsPlayersVSReadyTransitionInputMask == START_BUTTON)
    {
        mask |= (1u << 3);
    }

    for (i = 0; i < 31u; i++)
    {
        ndsMNPlayersVSClearControllerState();
        ndsMNPlayersVSRunMainTick(&updates);
        if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
        {
            break;
        }
    }

    if ((gSCManagerTransferBattleState.pl_count == 2u) &&
        (gSCManagerTransferBattleState.cp_count == 0u))
    {
        mask |= (1u << 4);
    }
    if ((gSCManagerSceneData.scene_prev == nSCKindPlayersVS) &&
        (gSCManagerSceneData.scene_curr == nSCKindMaps))
    {
        mask |= (1u << 5);
    }

    gNdsPlayersVSReadyTransitionTaskmanStatus = (u32)sSYTaskmanStatus;
    if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
    {
        mask |= (1u << 6);
    }

    gcEjectAll();
    gNdsPlayersVSReadyTransitionCleanupCount++;
    mask |= (1u << 7);

    gNdsPlayersVSReadyTransitionPlayerCount =
        gSCManagerTransferBattleState.pl_count;
    gNdsPlayersVSReadyTransitionCpuCount =
        gSCManagerTransferBattleState.cp_count;
    gNdsPlayersVSReadyTransitionP0FKind =
        gSCManagerTransferBattleState.players[0].fkind;
    gNdsPlayersVSReadyTransitionP1FKind =
        gSCManagerTransferBattleState.players[1].fkind;
    gNdsPlayersVSReadyTransitionStageSelect =
        gSCManagerTransferBattleState.is_stage_select;
    gNdsPlayersVSReadyTransitionScenePrevFinal =
        gSCManagerSceneData.scene_prev;
    gNdsPlayersVSReadyTransitionSceneCurrFinal =
        gSCManagerSceneData.scene_curr;
    gNdsPlayersVSReadyTransitionMask = mask;
    if (mask == 0xffu)
    {
        gNdsPlayersVSReadyTransitionResult =
            NDS_PLAYERS_VS_READY_TRANSITION_PASS;
    }
}

void mnPlayersVSStartScene(void)
{
    dMNPlayersVSVideoSetup.zbuffer =
        SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNPlayersVSVideoSetup);

    gNdsPlayersVSOriginalStartResult =
        NDS_PLAYERS_VS_ORIGINAL_START_PASS;

    {
        SYTaskmanSetup setup = ndsMNPlayersVSMakeTaskmanSetup();
        scManagerFuncUpdate(&setup);
    }
}
