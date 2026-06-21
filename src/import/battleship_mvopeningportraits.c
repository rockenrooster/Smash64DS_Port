/* Compile the original BattleShip Opening Portraits scene translation unit.
 *
 * Portraits is the first original scene after Opening Room. Keep it imported
 * through this project-owned wrapper so the upstream checkout remains read-only.
 */
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
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

extern void ftDisplayLightsDrawReflect(Gfx **display_list, f32 light_angle_x, f32 light_angle_y);
extern f32 scSubsysFighterGetLightAngleX(void);
extern f32 scSubsysFighterGetLightAngleY(void);
extern void lbCommonClearExternSpriteParams(void);
extern sb32 (*dLBCommonFuncMatrixList[])(void);
extern void sySchedulerSetTicCount(u32 tics);

#define mvOpeningPortraitsFuncRun ndsBaseMVOpeningPortraitsFuncRun
#define mvOpeningPortraitsFuncStart ndsBaseMVOpeningPortraitsFuncStart
#define mvOpeningPortraitsStartScene ndsBaseMVOpeningPortraitsStartScene

void ndsOpeningPortraitsRecordRunTick(void);
void ndsOpeningPortraitsRecordFuncStart(void);
void ndsOpeningPortraitsRecordStart(void);

#include "../../decomp/BattleShip-main/decomp/src/mv/mvopening/mvopeningportraits.c"

#undef mvOpeningPortraitsFuncRun
#undef mvOpeningPortraitsFuncStart
#undef mvOpeningPortraitsStartScene

void mvOpeningPortraitsFuncRun(GObj *gobj)
{
    ndsBaseMVOpeningPortraitsFuncRun(gobj);
    ndsOpeningPortraitsRecordRunTick();
}

void mvOpeningPortraitsFuncStart(void)
{
    sySchedulerSetTicCount(1335);
    ndsBaseMVOpeningPortraitsFuncStart();
    ndsOpeningPortraitsRecordFuncStart();
}

void mvOpeningPortraitsStartScene(void)
{
    ndsOpeningPortraitsRecordStart();
    dMVOpeningPortraitsTaskmanSetup.func_start = mvOpeningPortraitsFuncStart;
    dMVOpeningPortraitsVideoSetup.zbuffer =
        SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMVOpeningPortraitsVideoSetup);
    dMVOpeningPortraitsTaskmanSetup.scene_setup.arena_start =
        ndsTaskmanArenaStart();
    dMVOpeningPortraitsTaskmanSetup.scene_setup.arena_size =
        ndsTaskmanArenaSize();
    syTaskmanStartTask(&dMVOpeningPortraitsTaskmanSetup);
}
