/*
 * Bounded BattleShip ftcommonguard1.c / ftcommonguard2.c import for the NDS
 * Mario/Fox dash-run guard interrupt proof. Public symbols are remapped so the
 * port backend can guard the original path and keep full shield runtime
 * deferred.
 */
#include <PR/ultratypes.h>
#include <math.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <macros.h>
#include <sys/audio.h>
#include <sys/obj.h>

#ifndef AOBJ_ANIM_NULL
#define AOBJ_ANIM_NULL 0.0F
#endif

typedef struct EFStruct {
    struct {
        struct {
            sb32 is_damage_shield;
        } shield;
    } effect_vars;
} EFStruct;

void ftParamSetHitStatusPartID(GObj *fighter_gobj, s32 joint_id,
                               s32 hitstatus);
void ftParamResetModelPartAll(GObj *fighter_gobj);
void ftParamProcStopEffect(GObj *fighter_gobj);
void ftParamHideModelPartAll(GObj *fighter_gobj);
void ftParamsUpdateFighterPartsTransform(DObj *joint);
void ftParamsUpdateFighterPartsTransformAll(DObj *joint);
void ftCommonShieldBreakFlyCommonSetStatus(GObj *fighter_gobj);
void gmCollisionGetFighterPartsWorldPosition(DObj *main_dobj, Vec3f *vec);
void efManagerEggBreakMakeEffect(Vec3f *pos);
GObj *efManagerShieldMakeEffect(GObj *fighter_gobj);
GObj *efManagerYoshiShieldMakeEffect(GObj *fighter_gobj);
EFStruct *efGetStruct(GObj *effect_gobj);
f32 syUtilsArcTan2(f32 y, f32 x);
void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                        f32 anim_frame);
void gcParseDObjAnimJoint(DObj *dobj);
void gcPlayDObjAnimJoint(DObj *dobj);
void lbCommonAddDObjAnimJointAll(DObj *dobj, AObjEvent32 **anim_joint,
                                 f32 anim_frame);
void lbCommonPlayTranslateScaledDObjAnim(DObj *dobj, Vec3f *scale);
sb32 ftCommonLightThrowCheckInterruptGuardOn(GObj *fighter_gobj);
sb32 ftCommonEscapeCheckInterruptGuard(GObj *fighter_gobj);
sb32 ftCommonCatchCheckInterruptGuard(GObj *fighter_gobj);
sb32 ftCommonDokanStartCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonGuardPassCheckInterruptGuard(GObj *fighter_gobj);

#define ftCommonGuardCheckScheduleRelease \
    ndsBaseFTCommonGuardCheckScheduleRelease
#define ftCommonGuardOnSetHitStatusYoshi \
    ndsBaseFTCommonGuardOnSetHitStatusYoshi
#define ftCommonGuardSetHitStatusYoshi \
    ndsBaseFTCommonGuardSetHitStatusYoshi
#define ftCommonGuardOffSetHitStatusYoshi \
    ndsBaseFTCommonGuardOffSetHitStatusYoshi
#define ftCommonGuardUpdateShieldVars ndsBaseFTCommonGuardUpdateShieldVars
#define ftCommonGuardUpdateShieldCollision \
    ndsBaseFTCommonGuardUpdateShieldCollision
#define ftCommonGuardUpdateShieldAngle ndsBaseFTCommonGuardUpdateShieldAngle
#define ftCommonGuardGetJointTransform ndsBaseFTCommonGuardGetJointTransform
#define ftCommonGuardGetJointTransformScale \
    ndsBaseFTCommonGuardGetJointTransformScale
#define ftCommonGuardUpdateJoints ndsBaseFTCommonGuardUpdateJoints
#define ftCommonGuardInitJoints ndsBaseFTCommonGuardInitJoints
#define ftCommonGuardOnProcUpdate ndsBaseFTCommonGuardOnProcUpdate
#define ftCommonGuardCommonProcInterrupt \
    ndsBaseFTCommonGuardCommonProcInterrupt
#define ftCommonGuardOnSetStatus ndsBaseFTCommonGuardOnSetStatus
#define ftCommonGuardOnCheckInterruptSuccess \
    ndsBaseFTCommonGuardOnCheckInterruptSuccess
#define ftCommonGuardOnCheckInterruptCommon \
    ndsBaseFTCommonGuardOnCheckInterruptCommon
#define ftCommonGuardOnCheckInterruptDashRun \
    ndsBaseFTCommonGuardOnCheckInterruptDashRun
#define ftCommonGuardProcUpdate ndsBaseFTCommonGuardProcUpdate
#define ftCommonGuardSetStatus ndsBaseFTCommonGuardSetStatus
#define ftCommonGuardSetStatusFromEscape \
    ndsBaseFTCommonGuardSetStatusFromEscape
#define ftCommonGuardCheckInterruptEscape \
    ndsBaseFTCommonGuardCheckInterruptEscape
#define ftCommonGuardOffProcUpdate ndsBaseFTCommonGuardOffProcUpdate
#define ftCommonGuardOffSetStatus ndsBaseFTCommonGuardOffSetStatus
#define ftCommonGuardSetOffProcUpdate ndsBaseFTCommonGuardSetOffProcUpdate
#define ftCommonGuardSetOffSetStatus ndsBaseFTCommonGuardSetOffSetStatus

void ndsBaseFTCommonGuardOnProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonGuardCommonProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonGuardOnSetStatus(GObj *fighter_gobj, s32 slide_tics);
sb32 ndsBaseFTCommonGuardOnCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonGuardOnCheckInterruptDashRun(GObj *fighter_gobj,
                                                 s32 slide_tics);
void ndsBaseFTCommonGuardProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonGuardSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonGuardOffProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonGuardOffSetStatus(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonguard1.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonguard2.c"

#undef ftCommonGuardCheckScheduleRelease
#undef ftCommonGuardOnSetHitStatusYoshi
#undef ftCommonGuardSetHitStatusYoshi
#undef ftCommonGuardOffSetHitStatusYoshi
#undef ftCommonGuardUpdateShieldVars
#undef ftCommonGuardUpdateShieldCollision
#undef ftCommonGuardUpdateShieldAngle
#undef ftCommonGuardGetJointTransform
#undef ftCommonGuardGetJointTransformScale
#undef ftCommonGuardUpdateJoints
#undef ftCommonGuardInitJoints
#undef ftCommonGuardOnProcUpdate
#undef ftCommonGuardCommonProcInterrupt
#undef ftCommonGuardOnSetStatus
#undef ftCommonGuardOnCheckInterruptSuccess
#undef ftCommonGuardOnCheckInterruptCommon
#undef ftCommonGuardOnCheckInterruptDashRun
#undef ftCommonGuardProcUpdate
#undef ftCommonGuardSetStatus
#undef ftCommonGuardSetStatusFromEscape
#undef ftCommonGuardCheckInterruptEscape
#undef ftCommonGuardOffProcUpdate
#undef ftCommonGuardOffSetStatus
#undef ftCommonGuardSetOffProcUpdate
#undef ftCommonGuardSetOffSetStatus

EFStruct *efGetStruct(GObj *effect_gobj)
{
    static EFStruct s_effect;

    (void)effect_gobj;
    return &s_effect;
}
