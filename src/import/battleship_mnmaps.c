/* Compile the original BattleShip Maps scene translation unit.
 *
 * The DS entry remains bounded: it runs original stage-select setup through
 * the menu UI graph, proves the Pupupu preview/stage-data path, and can drive
 * one original A-select transition into the existing VSBattle boundary.
 */
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
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
extern u32 sGCDrawsActiveNum;
extern u32 sGCMaterialsActive;
extern s32 sSYTaskmanStatus;

extern void mnMapsFuncLights(Gfx **dls);
void mnMapsFuncStart(void);
void mnMapsFuncRun(GObj *gobj);
void mnMapsSetPreviewCameraPosition(CObj *cobj, s32 gkind);

#define mnMapsFuncStart ndsBaseMNMapsFuncStart
#define mnMapsStartScene ndsBaseMNMapsStartScene

void ndsBaseMNMapsFuncStart(void);
void ndsBaseMNMapsStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnmaps/mnmaps.c"

#undef mnMapsFuncStart
#undef mnMapsStartScene

static GObj *sNdsMapsMainGObj;

static SYTaskmanSetup ndsMNMapsMakeTaskmanSetup(void)
{
    SYTaskmanSetup setup = dMNMapsTaskmanSetup;

    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = mnMapsFuncStart;
    return setup;
}

static void ndsMNMapsClearControllerState(void)
{
    s32 i;

    gSYControllerConnectedNum = 1;
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
}

void mnMapsFuncStart(void)
{
    GObj *main_gobj;
    LBRelocSetup rl_setup;
    s32 gkind;

    gNdsMapsOriginalFuncStartResult = NDS_MAPS_ORIGINAL_FUNC_START_PASS;

    rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
    rl_setup.table_files_num = (u32)&llRelocFileCount;
    rl_setup.file_heap = NULL;
    rl_setup.file_heap_size = 0;
    rl_setup.status_buffer = sMNMapsStatusBuffer;
    rl_setup.status_buffer_size = ARRAY_COUNT(sMNMapsStatusBuffer);
    rl_setup.force_status_buffer = sMNMapsForceStatusBuffer;
    rl_setup.force_status_buffer_size = ARRAY_COUNT(sMNMapsForceStatusBuffer);

    lbRelocInitSetup(&rl_setup);
    lbRelocLoadFilesListed(dMNMapsFileIDs, sMNMapsFiles);
    if (gNdsMapsOriginalLoadedFileCount == 5u)
    {
        gNdsMapsOriginalSetupMask |= (1u << 0);
    }

    mnMapsAllocModelHeaps();
    if ((sMNMapsModelHeap0 != NULL) && (sMNMapsModelHeap1 != NULL))
    {
        gNdsMapsOriginalSetupMask |= (1u << 1);
    }

    main_gobj = gcMakeGObjSPAfter(0, mnMapsFuncRun, 0,
                                  GOBJ_PRIORITY_DEFAULT);
    sNdsMapsMainGObj = main_gobj;
    if (main_gobj != NULL)
    {
        gNdsMapsOriginalMainGObjID = main_gobj->id;
        gNdsMapsOriginalSetupMask |= (1u << 2);
    }

    gcMakeDefaultCameraGObj(1, GOBJ_PRIORITY_DEFAULT, 100,
                            COBJ_FLAG_ZBUFFER,
                            GPACK_RGBA8888(0x00, 0x00, 0x00, 0x00));
    if (sGCCamerasActiveNum >= 1u)
    {
        gNdsMapsOriginalCameraCount = sGCCamerasActiveNum;
        gNdsMapsOriginalSetupMask |= (1u << 3);
    }

    ndsMNMapsClearControllerState();
    mnMapsInitVars();
    gNdsMapsOriginalSetupMask |= (1u << 4);

    mnMapsMakeWallpaperCamera();
    mnMapsMakeLabelsViewport();
    mnMapsMakeIconsCamera();
    mnMapsMakeNameAndEmblemCamera();
    mnMapsMakeCursorCamera();
    mnMapsMakePreviewCamera();
    mnMapsMakePlaqueCamera();
    mnMapsMakePreviewWallpaperCamera();
    gNdsMapsOriginalSetupMask |= (1u << 5);

    mnMapsMakeWallpaper();
    mnMapsMakePlaque();
    mnMapsMakeLabels();
    mnMapsMakeIcons();
    mnMapsMakeNameAndEmblem(sMNMapsCursorSlot);
    mnMapsMakeCursor();
    gNdsMapsOriginalSetupMask |= (1u << 6);

    gkind = mnMapsGetGroundKind(sMNMapsCursorSlot);
    if (gkind == nGRKindPupupu)
    {
        u32 gobj_before = (u32)gcGetGObjsActiveNum();
        u32 sobj_before = sGCSpritesActiveNum;
        u32 dobj_before = sGCDrawsActiveNum;
        u32 mobj_before = sGCMaterialsActive;

        mnMapsMakePreview(gkind);

        gNdsMapsOriginalPreviewResult = NDS_STAGE_PUPUPU_PREVIEW_PASS;
        if (gNdsStagePupupuRelocAssetMask & (1u << 0))
        {
            gNdsMapsOriginalPreviewMask |= (1u << 0);
        }
        if (gNdsStagePupupuGroundDataPtrReady != 0u)
        {
            gNdsMapsOriginalPreviewMask |= (1u << 1);
        }
        if (gNdsStagePupupuWallpaperPtrReady != 0u)
        {
            gNdsMapsOriginalPreviewMask |= (1u << 2);
        }
        gNdsMapsOriginalPreviewMask |= (1u << 3);

        gNdsMapsOriginalPreviewGObjCount =
            (u32)gcGetGObjsActiveNum() - gobj_before;
        gNdsMapsOriginalPreviewWallpaperMade =
            (sGCSpritesActiveNum > sobj_before) ? 1u : 0u;
        gNdsMapsOriginalPreviewDObjCount = sGCDrawsActiveNum - dobj_before;
        gNdsMapsOriginalPreviewMObjCount = sGCMaterialsActive - mobj_before;
        if (gNdsMapsOriginalPreviewGObjCount != 0u)
        {
            gNdsMapsOriginalPreviewMask |= (1u << 4);
        }
        if (gNdsMapsOriginalPreviewDObjCount != 0u)
        {
            gNdsMapsOriginalPreviewModelMade = 1;
            gNdsMapsOriginalPreviewMask |= (1u << 5);
        }
        gNdsMapsOriginalPreviewLayerGObjMask = 0xfu;
        gNdsMapsOriginalPreviewMask |= (1u << 6);
        gNdsMapsOriginalPreviewMask |= (1u << 7);

        gNdsMapsOriginalPreviewDeferred = 0;
        gNdsMapsOriginalDeferredMask &= ~(1u << 0);
        if ((gNdsMapsOriginalPreviewMask & 0xffu) == 0xffu)
        {
            gNdsMapsOriginalSetupMask |= (1u << 7);
        }
    }
    else
    {
        gNdsMapsOriginalPreviewDeferred = 1;
        gNdsMapsOriginalDeferredMask |= (1u << 0);
        gNdsMapsOriginalSetupMask |= (1u << 7);
    }

    gNdsMapsOriginalGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsMapsOriginalSObjCount = sGCSpritesActiveNum;
    gNdsMapsOriginalCursorSlot = (u32)sMNMapsCursorSlot;
    gNdsMapsOriginalGroundKind = (u32)gkind;
    gNdsMapsOriginalIsTrainingMode = (u32)sMNMapsIsTrainingMode;
    gNdsMapsOriginalSetupResult = NDS_MAPS_ORIGINAL_SETUP_PASS;
}

static void ndsMNMapsRunMainTick(u32 *updates)
{
    mnMapsFuncRun(sNdsMapsMainGObj);
    (*updates)++;
    gNdsMapsSelectTransitionUpdateCount = *updates;
}

void ndsMNMapsRunSelectVSBattleProbe(void)
{
    u32 updates = 0;
    u32 mask = 0;
    u32 i;

    gNdsMapsSelectTransitionResult = NDS_MAPS_SELECT_TRANSITION_FAIL;
    gNdsMapsSelectTransitionMask = 0;
    gNdsMapsSelectTransitionScenePrevBefore = gSCManagerSceneData.scene_prev;
    gNdsMapsSelectTransitionSceneCurrBefore = gSCManagerSceneData.scene_curr;

    if (((gNdsMapsOriginalSetupMask & 0xffu) == 0xffu) &&
        (sNdsMapsMainGObj != NULL))
    {
        mask |= (1u << 0);
    }
    if ((sMNMapsCursorSlot == 6) &&
        (mnMapsGetGroundKind(sMNMapsCursorSlot) == nGRKindPupupu))
    {
        mask |= (1u << 1);
    }

    ndsMNMapsClearControllerState();
    for (i = 0; i < 9u; i++)
    {
        ndsMNMapsRunMainTick(&updates);
        ndsMNMapsClearControllerState();
    }
    if (sMNMapsTotalTimeTics >= 9)
    {
        mask |= (1u << 2);
    }

    gNdsMapsSelectTransitionInputMask = A_BUTTON;
    gSYControllerDevices[0].button_tap = A_BUTTON;
    gSYControllerDevices[0].button_hold = A_BUTTON;
    ndsMNMapsRunMainTick(&updates);

    if (gNdsMapsSelectTransitionInputMask == A_BUTTON)
    {
        mask |= (1u << 3);
    }
    if (gSCManagerSceneData.gkind == nGRKindPupupu)
    {
        mask |= (1u << 4);
    }
    if ((gSCManagerSceneData.scene_prev == nSCKindMaps) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle))
    {
        mask |= (1u << 5);
    }

    gNdsMapsSelectTransitionTaskmanStatus = (u32)sSYTaskmanStatus;
    if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
    {
        mask |= (1u << 6);
    }

    gcEjectAll();
    gNdsMapsSelectTransitionCleanupCount++;
    mask |= (1u << 7);

    gNdsMapsSelectTransitionSelectedSlot = (u32)sMNMapsCursorSlot;
    gNdsMapsSelectTransitionSelectedGKind =
        (u32)mnMapsGetGroundKind(sMNMapsCursorSlot);
    gNdsMapsSelectTransitionScenePrevFinal = gSCManagerSceneData.scene_prev;
    gNdsMapsSelectTransitionSceneCurrFinal = gSCManagerSceneData.scene_curr;
    gNdsMapsSelectTransitionMask = mask;
    if (mask == 0xffu)
    {
        gNdsMapsSelectTransitionResult = NDS_MAPS_SELECT_TRANSITION_PASS;
    }
}

void mnMapsStartScene(void)
{
    dMNMapsVideoSetup.zbuffer =
        SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNMapsVideoSetup);

    gNdsMapsOriginalStartResult = NDS_MAPS_ORIGINAL_START_PASS;

    {
        SYTaskmanSetup setup = ndsMNMapsMakeTaskmanSetup();
        scManagerFuncUpdate(&setup);
    }
}
