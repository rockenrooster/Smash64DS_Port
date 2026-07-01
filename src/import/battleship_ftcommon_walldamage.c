/*
 * Bounded BattleShip ftcommonwalldamage.c import for NDS wall-damage proofs.
 * Public symbols are remapped so the port backend can gate the original path.
 */
#include <ef/effect.h>
#include <ft/fighter.h>
#include <sys/obj.h>
#include <sys/objman.h>

#ifndef SYVECTOR_AXIS_Z
#define SYVECTOR_AXIS_Z 4
#endif

#ifndef FTCOMMON_WALLDAMAGE_INTANGIBLE_TIMER
#define FTCOMMON_WALLDAMAGE_INTANGIBLE_TIMER 15
#endif

extern f32 syUtilsArcTan2(f32 y, f32 x);
extern f32 lbCommonMag2D(Vec3f *vec);
extern Vec3f *lbCommonAdd2D(Vec3f *a, Vec3f *b);
extern Vec3f *lbCommonScale2D(Vec3f *vec, f32 factor);
extern Vec3f *lbCommonReflect2D(Vec3f *a, Vec3f *b);
extern GObj *efManagerImpactWaveMakeEffect(Vec3f *pos, s32 index,
                                           f32 rotate);
extern void ftCommonDamageUpdateDustEffect(GObj *fighter_gobj);
extern void ftCommonDamageDecHitStunSetPublic(GObj *fighter_gobj);
extern f32 ftParamGetHitStun(f32 knockback);
extern void ftParamSetTimedHitStatusIntangible(FTStruct *fp,
                                               s32 intangible_tics);

#define ftCommonWallDamageProcUpdate ndsBaseFTCommonWallDamageProcUpdate
#define ftCommonWallDamageSetStatus ndsBaseFTCommonWallDamageSetStatus
#define ftCommonWallDamageCheckGoto ndsBaseFTCommonWallDamageCheckGoto

void ndsBaseFTCommonWallDamageProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonWallDamageSetStatus(GObj *fighter_gobj, Vec3f *angle,
                                        Vec3f *pos);
sb32 ndsBaseFTCommonWallDamageCheckGoto(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonwalldamage.c"

#undef ftCommonWallDamageProcUpdate
#undef ftCommonWallDamageSetStatus
#undef ftCommonWallDamageCheckGoto
