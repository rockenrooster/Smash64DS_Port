/*
 * Bounded BattleShip ftcommontwister.c import for ground-obstacle hazard
 * dispatch. Public names stay project-owned until the continuous Twister
 * runtime is widened.
 */
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <macros.h>
#include <sys/obj.h>

#ifndef FTCOMMON_TORNADO_RELEASE_WAIT
#define FTCOMMON_TORNADO_RELEASE_WAIT 60.0F
#define FTCOMMON_TORNADO_PICKUP_WAIT 60
#endif

f32 lbCommonCos(f32 angle);
f32 lbCommonSin(f32 angle);
f32 syVectorMag3D(Vec3f *vec);
Vec3f *syVectorDiff3D(Vec3f *dst, Vec3f *src, Vec3f *sub);
Vec3f *syVectorScale3D(Vec3f *vec, f32 scale);
void *func_800269C0_275C0(u16 fgm_id);

#define ftCommonTwisterProcUpdate ndsBaseFTCommonTwisterProcUpdate
#define ftCommonTwisterProcPhysics ndsBaseFTCommonTwisterProcPhysics
#define ftCommonTwisterSetStatus ndsBaseFTCommonTwisterSetStatus
#define ftCommonTwisterShootFighter ndsBaseFTCommonTwisterShootFighter

void ndsBaseFTCommonTwisterProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonTwisterProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonTwisterSetStatus(GObj *fighter_gobj, GObj *tornado_gobj);
void ndsBaseFTCommonTwisterShootFighter(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommontwister.c"

#undef ftCommonTwisterProcUpdate
#undef ftCommonTwisterProcPhysics
#undef ftCommonTwisterSetStatus
#undef ftCommonTwisterShootFighter
