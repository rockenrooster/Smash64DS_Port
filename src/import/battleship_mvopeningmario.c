/* Compile the original BattleShip Mario opening scene translation unit.
 *
 * This wrapper keeps the upstream decomp checkout read-only while letting the
 * DS port bound the first fighter/ground branch. The original func_start still
 * creates the name/camera objects and loads the original O2R files.
 */
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <mn/menu.h>
#include <nds/nds_startup.h>
#include <sc/scene.h>
#include <sys/controller.h>
#include <sys/objdef.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>

extern void efParticleInitAll(void);
extern void mpCollisionInitGroundData(void);
extern void gmCameraSetViewportDimensions(s32 ulx, s32 uly, s32 lrx, s32 lry);
extern GObj *gmCameraMakeWallpaperCamera(void);
extern GObj *gmCameraMakeMovieCamera(void (*func_camera)(GObj *));
extern void grWallpaperMakeDecideKind(void);
extern void grCommonSetupInitAll(void);
extern s32 mpCollisionGetMapObjCountKind(s32 kind);
extern void mpCollisionGetMapObjIDsKind(s32 kind, s32 *ids);
extern void mpCollisionGetMapObjPositionID(s32 id, Vec3f *pos);
extern void gmRumbleMakeActor(void);
extern void wpManagerAllocWeapons(void);
extern void itManagerInitItems(void);
extern void efManagerInitEffects(void);
extern void ftDisplayLightsDrawReflect(Gfx **display_list,
                                       f32 light_angle_x,
                                       f32 light_angle_y);
extern void sySchedulerSetTicCount(u32 tics);
extern u32 sySchedulerGetTicCount(void);
extern void syTaskmanSetLoadScene(void);
extern sb32 (*dLBCommonFuncMatrixList[])(void);

extern f32 gMPCollisionLightAngleX;
extern f32 gMPCollisionLightAngleY;

#define mvOpeningMarioFuncRun ndsBaseMVOpeningMarioFuncRun
#define mvOpeningMarioFuncStart ndsBaseMVOpeningMarioFuncStart
#define mvOpeningMarioStartScene ndsBaseMVOpeningMarioStartScene

void ndsOpeningMarioRecordStart(void);
void ndsOpeningMarioRecordFuncStart(void);
void ndsOpeningMarioRecordRunTick(void);
void ndsOpeningMarioRecordFighterDeferred(void);

#include "../../decomp/BattleShip-main/decomp/src/mv/mvopening/mvopeningmario.c"

#undef mvOpeningMarioFuncRun
#undef mvOpeningMarioFuncStart
#undef mvOpeningMarioStartScene

void mvOpeningMarioFuncRun(GObj *gobj);

static void ndsOpeningMarioPatchMovieActorRunFunc(void)
{
    GObj *gobj = gGCCommonLinks[nGCCommonLinkIDMovie];

    while (gobj != NULL)
    {
        if ((gobj->id == nGCCommonKindMovie) &&
            (gobj->func_run == ndsBaseMVOpeningMarioFuncRun))
        {
            gobj->func_run = mvOpeningMarioFuncRun;
            return;
        }
        gobj = gobj->link_next;
    }
}

void mvOpeningMarioFuncRun(GObj *gobj)
{
    if (sMVOpeningMarioTotalTimeTics < 14)
    {
        ndsBaseMVOpeningMarioFuncRun(gobj);
    }
    else
    {
        sMVOpeningMarioTotalTimeTics++;

        if (scSubsysControllerGetPlayerTapButtons(
                A_BUTTON | B_BUTTON | START_BUTTON) != FALSE)
        {
            gSCManagerSceneData.scene_prev = gSCManagerSceneData.scene_curr;
            gSCManagerSceneData.scene_curr = nSCKindTitle;
            syTaskmanSetLoadScene();
        }
        if (sMVOpeningMarioTotalTimeTics == 15)
        {
            ndsOpeningMarioRecordFighterDeferred();
        }
        if (sMVOpeningMarioTotalTimeTics == 60)
        {
            gSCManagerSceneData.scene_prev = gSCManagerSceneData.scene_curr;
            gSCManagerSceneData.scene_curr = nSCKindOpeningDonkey;
            syTaskmanSetLoadScene();
        }
    }
    ndsOpeningMarioRecordRunTick();
}

void mvOpeningMarioFuncStart(void)
{
    sySchedulerSetTicCount(1515);
    ndsBaseMVOpeningMarioFuncStart();
    ndsOpeningMarioPatchMovieActorRunFunc();
    ndsOpeningMarioRecordFuncStart();
}

void mvOpeningMarioStartScene(void)
{
    ndsOpeningMarioRecordStart();
    dMVOpeningMarioTaskmanSetup.func_start = mvOpeningMarioFuncStart;
    dMVOpeningMarioVideoSetup.zbuffer =
        SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMVOpeningMarioVideoSetup);
    dMVOpeningMarioTaskmanSetup.scene_setup.arena_start =
        ndsTaskmanArenaStart();
    dMVOpeningMarioTaskmanSetup.scene_setup.arena_size =
        ndsTaskmanArenaSize();
    syTaskmanStartTask(&dMVOpeningMarioTaskmanSetup);
}
