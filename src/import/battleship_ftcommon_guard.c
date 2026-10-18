/*
 * Bounded BattleShip ftcommonguard1.c / ftcommonguard2.c import for the NDS
 * Mario/Fox dash-run guard interrupt proof. Public symbols are remapped so the
 * port backend can guard the original path and keep full shield runtime
 * deferred.
 */
#include <PR/ultratypes.h>
#include <ef/effect.h>
#include <math.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <macros.h>
#include <nds/nds_shield_pose.h>
#include <nds/nds_p4_contents.h>
#include <sys/audio.h>
#include <sys/obj.h>
#if NDS_P4
#include <nds/nds_p4.h>
#endif

#ifndef AOBJ_ANIM_NULL
#define AOBJ_ANIM_NULL 0.0F
#endif

void ftParamSetHitStatusPartID(GObj *fighter_gobj, s32 joint_id,
                               s32 hitstatus);
void ftParamResetModelPartAll(GObj *fighter_gobj);
void ftParamProcStopEffect(GObj *fighter_gobj);
void ftParamHideModelPartAll(GObj *fighter_gobj);
void ftParamsUpdateFighterPartsTransform(DObj *joint);
void ftParamsUpdateFighterPartsTransformAll(DObj *joint);
void ftCommonShieldBreakFlyCommonSetStatus(GObj *fighter_gobj);
void gmCollisionGetFighterPartsWorldPosition(DObj *main_dobj, Vec3f *vec);
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

/* Native ShieldPose interposition is intentionally local to guard1.c. These
 * source fallbacks are defined before the macros below, so a diagnostic table,
 * variant fighter or disabled native package still calls the normal public
 * Event32 path with its O2R normalization intact. */
static void ndsGuardSourceAddDObjAnimJoint(DObj *dobj,
                                           AObjEvent32 *anim_joint,
                                           f32 anim_frame)
{
    gcAddDObjAnimJoint(dobj, anim_joint, anim_frame);
}

static void ndsGuardSourceAddDObjAnimJointAll(DObj *dobj,
                                              AObjEvent32 **anim_joint,
                                              f32 anim_frame)
{
    lbCommonAddDObjAnimJointAll(dobj, anim_joint, anim_frame);
}

static void ndsGuardSourcePlayAnimEventsAll(GObj *fighter_gobj)
{
    ftMainPlayAnimEventsAll(fighter_gobj);
}

#define gcAddDObjAnimJoint(dobj, anim_joint, anim_frame)                 \
    do                                                                   \
    {                                                                    \
        s32 nds_guard_pose_result =                                      \
            ndsShieldPoseTryApplySingle((dobj), (anim_frame));           \
        if (nds_guard_pose_result == 0)                                  \
        {                                                                \
            ndsGuardSourceAddDObjAnimJoint((dobj), (anim_joint),         \
                                            (anim_frame));               \
        }                                                                \
    } while (0)

#define lbCommonAddDObjAnimJointAll(dobj, anim_joint, anim_frame)        \
    do                                                                   \
    {                                                                    \
        s32 nds_guard_pose_result =                                      \
            ndsShieldPoseTryApplyAll((dobj), (anim_frame));              \
        if (nds_guard_pose_result == 0)                                  \
        {                                                                \
            ndsGuardSourceAddDObjAnimJointAll((dobj), (anim_joint),      \
                                               (anim_frame));            \
        }                                                                \
    } while (0)

#define ftMainPlayAnimEventsAll(fighter_gobj)                            \
    do                                                                   \
    {                                                                    \
        s32 nds_guard_pose_result =                                      \
            ndsShieldPoseTryPlayBatch((fighter_gobj));                   \
        if (nds_guard_pose_result == 0)                                  \
        {                                                                \
            ndsGuardSourcePlayAnimEventsAll((fighter_gobj));             \
        }                                                                \
    } while (0)

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
#if NDS_P4
/* EXTRA's static parts (nds_p4.h static_parts): the guard's joint count and
 * its DObjDesc rows leave them out (StaticPart.asm
 * fix_ftCommonGuardUpdateJoints_, fix_ftCommonGuardInitJoints_). The source
 * bodies keep their names (first token `GObj`); every call (`fighter_gobj`)
 * runs them through ndsP4GuardRun, which hides the parts from fp->joints. */
#define ftCommonGuardUpdateJoints(first) NDS_P4_GUARD_UPDATE_##first)
#define NDS_P4_GUARD_UPDATE_GObj ndsBaseFTCommonGuardUpdateJoints(GObj
#define NDS_P4_GUARD_UPDATE_fighter_gobj ndsP4GuardUpdateJoints(fighter_gobj
#define ftCommonGuardInitJoints(first) NDS_P4_GUARD_INIT_##first)
#define NDS_P4_GUARD_INIT_GObj ndsBaseFTCommonGuardInitJoints(GObj
#define NDS_P4_GUARD_INIT_fighter_gobj ndsP4GuardInitJoints(fighter_gobj
static void ndsP4GuardUpdateJoints(GObj *fighter_gobj);
static void ndsP4GuardInitJoints(GObj *fighter_gobj);
#else
#define ftCommonGuardUpdateJoints ndsBaseFTCommonGuardUpdateJoints
#define ftCommonGuardInitJoints ndsBaseFTCommonGuardInitJoints
#endif
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

/* P4: Yoshi's egg shield. Remix extends these compares to J Yoshi only
 * (yoshishared.asm yoshi_shield_fix_1-8), so a content aliasing Yoshi
 * (Bowser) guards with the common shield (nds_p4.h NDS_P4_PARENT_KIND). */
#if NDS_P4
#define nFTKindYoshi NDS_P4_PARENT_KIND(fp, nFTKindYoshi)
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonguard1.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonguard2.c"
#if NDS_P4
#undef nFTKindYoshi
#endif

#undef gcAddDObjAnimJoint
#undef lbCommonAddDObjAnimJointAll
#undef ftMainPlayAnimEventsAll

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
#if NDS_P4
#undef NDS_P4_GUARD_UPDATE_GObj
#undef NDS_P4_GUARD_UPDATE_fighter_gobj
#undef NDS_P4_GUARD_INIT_GObj
#undef NDS_P4_GUARD_INIT_fighter_gobj

void ndsBaseFTCommonGuardUpdateJoints(GObj *fighter_gobj);
void ndsBaseFTCommonGuardInitJoints(GObj *fighter_gobj);

static void ndsP4GuardRun(GObj *fighter_gobj, void (*body)(GObj *))
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *saved[4];
    s32 ids[4];
    s32 count = 0;
    s32 i;

    if (fp->nds_p4_content != 0u)
    {
        for (i = nFTPartsJointCommonStart;
             (i < (s32)ARRAY_COUNT(fp->joints)) && (count < 4); i++)
        {
            if ((fp->joints[i] != NULL) && (ndsP4GuardSkips(fp, i) != FALSE))
            {
                ids[count] = i;
                saved[count++] = fp->joints[i];
                fp->joints[i] = NULL;
            }
        }
    }
    body(fighter_gobj);

    for (i = 0; i < count; i++)
    {
        fp->joints[ids[i]] = saved[i];
    }
}

static void ndsP4GuardUpdateJoints(GObj *fighter_gobj)
{
    ndsP4GuardRun(fighter_gobj, ndsBaseFTCommonGuardUpdateJoints);
}

static void ndsP4GuardInitJoints(GObj *fighter_gobj)
{
    ndsP4GuardRun(fighter_gobj, ndsBaseFTCommonGuardInitJoints);
}
#endif
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
