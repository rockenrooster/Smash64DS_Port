/* Compile the original BattleShip Opening Room scene translation unit.
 *
 * The upstream checkout under decomp/ is read-only for this port. When the DS
 * slice needs a narrow hook, rename the imported symbols here and expose a
 * project-owned wrapper with the original ABI. */
#include <PR/ultratypes.h>

#define mvOpeningRoomFuncRun ndsBaseMVOpeningRoomFuncRun
#define mvOpeningRoomFuncStart ndsBaseMVOpeningRoomFuncStart
#define mvOpeningRoomStartScene ndsBaseMVOpeningRoomStartScene

void ndsOpeningMovieRecordRoomHandoff(u32 tick, u32 next_scene);

#include "../../decomp/BattleShip-main/decomp/src/mv/mvopening/mvopeningroom.c"

#undef mvOpeningRoomFuncRun
#undef mvOpeningRoomFuncStart
#undef mvOpeningRoomStartScene

GObj *sMVOpeningRoomOutsideGObj;
GObj *sMVOpeningRoomOutsideHazeGObj;
GObj *sMVOpeningRoomSunlightGObj;
GObj *sMVOpeningRoomDeskGObj;

static void ndsOpeningRoomMakeOutside(void)
{
    GObj *gobj;
    void *display_list;

    gNdsOpeningRoomOutsideAssetMask =
        NDS_OPENING_ROOM_OUTSIDE_ASSET_READY_MASK;
    display_list =
        lbRelocGetFileData(void*, sMVOpeningRoomFiles[0], &llMVCommonRoomOutsideDisplayList);
    gNdsOpeningRoomOutsideDisplayListOffset =
        (u32)((uintptr_t)display_list - (uintptr_t)sMVOpeningRoomFiles[0]);

    ndsOpeningRoomCaptureOutsideCountsBefore();
    sMVOpeningRoomOutsideGObj = gobj =
        gcMakeGObjSPAfter(0, NULL, 17, GOBJ_PRIORITY_DEFAULT);
    gcAddXObjForDObjFixed(
        gcAddDObjForGObj(gobj, display_list),
        nGCMatrixKindTraRotRpyRSca,
        0);
    gcAddGObjDisplay(
        gobj,
        gcDrawDObjDLLinksForGObj,
        6,
        GOBJ_PRIORITY_DEFAULT,
        ~0);
    ndsOpeningRoomCaptureOutsideCreation(gobj);
}

static void ndsOpeningRoomMakeHaze(void)
{
    GObj *gobj;
    void *display_list;

    gNdsOpeningRoomHazeAssetMask =
        NDS_OPENING_ROOM_HAZE_ASSET_READY_MASK;
    display_list =
        lbRelocGetFileData(void*, sMVOpeningRoomFiles[0], &llMVCommonRoomHazeDisplayList);
    gNdsOpeningRoomHazeDisplayListOffset =
        (u32)((uintptr_t)display_list - (uintptr_t)sMVOpeningRoomFiles[0]);

    ndsOpeningRoomCaptureHazeCountsBefore();
    sMVOpeningRoomOutsideHazeGObj = gobj =
        gcMakeGObjSPAfter(0, NULL, 17, GOBJ_PRIORITY_DEFAULT);
    gcAddXObjForDObjFixed(
        gcAddDObjForGObj(gobj, display_list),
        nGCMatrixKindTraRotRpyRSca,
        0);
    gcAddGObjDisplay(
        gobj,
        gcDrawDObjDLLinksForGObj,
        6,
        GOBJ_PRIORITY_DEFAULT,
        ~0);
    ndsOpeningRoomCaptureHazeCreation(gobj);
}

static void ndsOpeningRoomMakeSunlight(void)
{
    GObj *gobj;
    void *display_list;

    gNdsOpeningRoomSunlightAssetMask =
        NDS_OPENING_ROOM_SUNLIGHT_ASSET_READY_MASK;
    display_list =
        lbRelocGetFileData(void*, sMVOpeningRoomFiles[0], &llMVCommonRoomSunlightDisplayList);
    gNdsOpeningRoomSunlightDisplayListOffset =
        (u32)((uintptr_t)display_list - (uintptr_t)sMVOpeningRoomFiles[0]);

    ndsOpeningRoomCaptureSunlightCountsBefore();
    sMVOpeningRoomSunlightGObj = gobj =
        gcMakeGObjSPAfter(0, NULL, 17, GOBJ_PRIORITY_DEFAULT);
    gcAddXObjForDObjFixed(
        gcAddDObjForGObj(gobj, display_list),
        nGCMatrixKindTraRotRpyRSca,
        0);
    gcAddGObjDisplay(
        gobj,
        gcDrawDObjDLLinksForGObj,
        6,
        GOBJ_PRIORITY_DEFAULT,
        ~0);
    ndsOpeningRoomCaptureSunlightCreation(gobj);
}

static void ndsOpeningRoomMakeDesk(void)
{
    GObj *gobj;
    DObjDesc *dobjdesc;

    gNdsOpeningRoomDeskAssetMask =
        NDS_OPENING_ROOM_DESK_ASSET_READY_MASK;
    dobjdesc =
        lbRelocGetFileData(DObjDesc*, sMVOpeningRoomFiles[0], &llMVCommonRoomDeskDObjDesc);
    gNdsOpeningRoomDeskDObjOffset =
        (u32)((uintptr_t)dobjdesc - (uintptr_t)sMVOpeningRoomFiles[0]);

    ndsOpeningRoomCaptureDeskCountsBefore();
    sMVOpeningRoomDeskGObj = gobj =
        gcMakeGObjSPAfter(0, NULL, 17, GOBJ_PRIORITY_DEFAULT);
    gcSetupCommonDObjs(gobj, dobjdesc, NULL);
    gcAddGObjDisplay(
        gobj,
        gcDrawDObjTreeForGObj,
        6,
        GOBJ_PRIORITY_DEFAULT,
        ~0);
    ndsOpeningRoomCaptureDeskCreation(gobj);
}

void mvOpeningRoomFuncRun(GObj *gobj)
{
    ndsBaseMVOpeningRoomFuncRun(gobj);

    if ((sMVOpeningRoomTotalTimeTics == 450) &&
        (sMVOpeningRoomSunlightGObj != NULL) &&
        (gNdsOpeningRoomSunlightEjectResult !=
         NDS_OPENING_ROOM_SUNLIGHT_EJECT_PASS))
    {
        gNdsOpeningRoomSunlightEjectBeforeGObjCount =
            (u32)gcGetGObjsActiveNum();
        gcEjectGObj(sMVOpeningRoomSunlightGObj);
        gNdsOpeningRoomSunlightEjectAfterGObjCount =
            (u32)gcGetGObjsActiveNum();
        ndsOpeningRoomRecordSunlightEject(sMVOpeningRoomSunlightGObj);
        gNdsOpeningRoomTick450DeferredMask =
            NDS_OPENING_ROOM_TICK450_DEFER_MASK;
    }
    if (sMVOpeningRoomTotalTimeTics == (22 * 60))
    {
        gSCManagerSceneData.scene_prev = gSCManagerSceneData.scene_curr;
        gSCManagerSceneData.scene_curr = nSCKindOpeningPortraits;
        ndsOpeningMovieRecordRoomHandoff(
            (u32)sMVOpeningRoomTotalTimeTics,
            (u32)gSCManagerSceneData.scene_curr);
        syTaskmanSetLoadScene();
    }
}

void mvOpeningRoomFuncStart(void)
{
    GObj *actor_gobj;

    ndsBaseMVOpeningRoomFuncStart();

    actor_gobj = gcFindGObjByID(0);
    if ((actor_gobj != NULL) &&
        (actor_gobj->func_run == ndsBaseMVOpeningRoomFuncRun))
    {
        actor_gobj->func_run = mvOpeningRoomFuncRun;
    }
    ndsOpeningRoomMakeOutside();
    ndsOpeningRoomMakeHaze();
    ndsOpeningRoomMakeSunlight();
    ndsOpeningRoomMakeDesk();
}

void mvOpeningRoomStartScene(void)
{
    dMVOpeningRoomTaskmanSetup.func_start = mvOpeningRoomFuncStart;
    ndsBaseMVOpeningRoomStartScene();
}
