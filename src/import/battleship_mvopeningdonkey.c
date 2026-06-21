#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <mn/menu.h>
#include <mv/movie.h>
#include <nds/nds_opening_name_scene.h>
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

#define mvOpeningDonkeyFuncRun ndsBaseMVOpeningDonkeyFuncRun
#define mvOpeningDonkeyFuncStart ndsBaseMVOpeningDonkeyFuncStart
#define mvOpeningDonkeyStartScene ndsBaseMVOpeningDonkeyStartScene

#include "../../decomp/BattleShip-main/decomp/src/mv/mvopening/mvopeningdonkey.c"

#undef mvOpeningDonkeyFuncRun
#undef mvOpeningDonkeyFuncStart
#undef mvOpeningDonkeyStartScene

NDS_OPENING_NAME_SCENE_IMPL(Donkey, nSCKindOpeningDonkey, 1605u,
                            nSCKindOpeningLink)
