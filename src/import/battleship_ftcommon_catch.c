/*
 * Bounded BattleShip catch imports for the NDS Mario/Fox catch proof.
 * Throw release/damage runtime is still bounded by project-owned seams.
 */
#include <ft/fighter.h>
#include <gr/ground.h>
#include <it/item.h>
#include <sys/obj.h>

GObj *efManagerCatchSwirlMakeEffect(Vec3f *pos);
GObj *efManagerSamusGrappleBeamGlowMakeEffect(GObj *fighter_gobj);
sb32 mpCommonCheckFighterOnEdge(GObj *fighter_gobj);
sb32 mpCommonCheckFighterOnFloor(GObj *fighter_gobj);
sb32 ftCommonThrowCheckInterruptCatchWait(GObj *fighter_gobj);
void ftCommonCaptureShoulderedSetStatus(GObj *fighter_gobj);
void ftDonkeyThrowFWaitSetStatus(GObj *fighter_gobj);
void ftCommonThrownReleaseThrownUpdateStats(GObj *fighter_gobj, s32 lr,
                                            s32 script_id,
                                            sb32 is_proc_status);
void ftCommonThrownReleaseFighterLoseGrip(GObj *fighter_gobj);
void ftParamSetModelPartID(GObj *fighter_gobj, s32 joint_id,
                           s32 modelpart_id);
void func_ovl0_800C9A38(Mtx44f mtx, DObj *dobj);
void func_ovl2_800EDA0C(Mtx44f mtx, Vec3f *rotate);
void gmCollisionGetWorldPosition(Mtx44f mtx, Vec3f *vec);

#define ftCommonCatchProcUpdate ndsBaseFTCommonCatchProcUpdate
#define ftCommonCatchCaptureSetStatusRelease \
    ndsBaseFTCommonCatchCaptureSetStatusRelease
#define func_ovl3_80149B48 ndsBaseFTCommonCatchUnusedMapCheck
#define ftCommonCatchProcMap ndsBaseFTCommonCatchProcMap
#define ftCommonCatchSetStatus ndsBaseFTCommonCatchSetStatus
#define ftCommonCatchCheckInterruptGuard \
    ndsBaseFTCommonCatchCheckInterruptGuard
#define ftCommonCatchCheckInterruptCommon \
    ndsBaseFTCommonCatchCheckInterruptCommon
#define ftCommonCatchCheckInterruptDashRun \
    ndsBaseFTCommonCatchCheckInterruptDashRun
#define ftCommonCatchCheckInterruptAttack11 \
    ndsBaseFTCommonCatchCheckInterruptAttack11

void ndsBaseFTCommonCatchProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonCatchCaptureSetStatusRelease(GObj *fighter_gobj);
void ndsBaseFTCommonCatchUnusedMapCheck(GObj *fighter_gobj);
void ndsBaseFTCommonCatchProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonCatchSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCatchCheckInterruptGuard(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCatchCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCatchCheckInterruptDashRun(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCatchCheckInterruptAttack11(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncatch1.c"

#undef ftCommonCatchProcUpdate
#undef ftCommonCatchCaptureSetStatusRelease
#undef func_ovl3_80149B48
#undef ftCommonCatchProcMap
#undef ftCommonCatchSetStatus
#undef ftCommonCatchCheckInterruptGuard
#undef ftCommonCatchCheckInterruptCommon
#undef ftCommonCatchCheckInterruptDashRun
#undef ftCommonCatchCheckInterruptAttack11

#define ftCommonCatchPullProcUpdate ndsBaseFTCommonCatchPullProcUpdate
#define ftCommonCatchPullProcCatch ndsBaseFTCommonCatchPullProcCatch
#define ftCommonCatchWaitProcInterrupt ndsBaseFTCommonCatchWaitProcInterrupt
#define ftCommonCatchWaitSetStatus ndsBaseFTCommonCatchWaitSetStatus

void ndsBaseFTCommonCatchPullProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonCatchPullProcCatch(GObj *fighter_gobj);
void ndsBaseFTCommonCatchWaitProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonCatchWaitSetStatus(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncatch2.c"

#undef ftCommonCatchPullProcUpdate
#undef ftCommonCatchPullProcCatch
#undef ftCommonCatchWaitProcInterrupt
#undef ftCommonCatchWaitSetStatus

#define ftCommonCaptureWaitProcMap ndsBaseFTCommonCaptureWaitProcMap
#define ftCommonCaptureWaitSetStatus ndsBaseFTCommonCaptureWaitSetStatus
#define ftCommonCapturePulledRotateScale \
    ndsBaseFTCommonCapturePulledRotateScale
#define ftCommonCapturePulledProcPhysics \
    ndsBaseFTCommonCapturePulledProcPhysics
#define ftCommonCapturePulledProcMap ndsBaseFTCommonCapturePulledProcMap
#define ftCommonCapturePulledProcCapture \
    ndsBaseFTCommonCapturePulledProcCapture

void ndsBaseFTCommonCaptureWaitProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonCaptureWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonCapturePulledRotateScale(GObj *fighter_gobj,
                                             Vec3f *this_pos,
                                             Vec3f *rotate);
void ndsBaseFTCommonCapturePulledProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonCapturePulledProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonCapturePulledProcCapture(GObj *fighter_gobj,
                                             GObj *capture_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncapturewait.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncapturepulled.c"

#undef ftCommonCaptureWaitProcMap
#undef ftCommonCaptureWaitSetStatus
#undef ftCommonCapturePulledRotateScale
#undef ftCommonCapturePulledProcPhysics
#undef ftCommonCapturePulledProcMap
#undef ftCommonCapturePulledProcCapture

#define ftCommonThrownProcUpdate ndsBaseFTCommonThrownProcUpdate
#define ftCommonThrownProcPhysics ndsBaseFTCommonThrownProcPhysics
#define ftCommonThrownProcMap ndsBaseFTCommonThrownProcMap
#define ftCommonThrownSetStatusQueue ndsBaseFTCommonThrownSetStatusQueue
#define ftCommonThrownSetStatusImmediate \
    ndsBaseFTCommonThrownSetStatusImmediate
#define ftCommonCapturePulledRotateScale \
    ndsBaseFTCommonCapturePulledRotateScale
#define ftCommonThrowProcUpdate ndsBaseFTCommonThrowProcUpdate
#define ftCommonThrowSetStatus ndsBaseFTCommonThrowSetStatus
#define ftCommonThrowCheckInterruptCatchWait \
    ndsBaseFTCommonThrowCheckInterruptCatchWait
#define nFTKirbyStatusThrowF nFTCommonStatusThrowF

void ndsBaseFTCommonThrownProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonThrownProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonThrownProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonThrownSetStatusQueue(GObj *fighter_gobj,
                                         s32 status_id_new,
                                         s32 status_id_queue);
void ndsBaseFTCommonThrownSetStatusImmediate(GObj *fighter_gobj,
                                             s32 status_id);
void ndsBaseFTCommonThrowProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonThrowSetStatus(GObj *fighter_gobj, sb32 is_throwf);
sb32 ndsBaseFTCommonThrowCheckInterruptCatchWait(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonthrown1.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonthrow.c"

#undef ftCommonThrownProcUpdate
#undef ftCommonThrownProcPhysics
#undef ftCommonThrownProcMap
#undef ftCommonThrownSetStatusQueue
#undef ftCommonThrownSetStatusImmediate
#undef ftCommonCapturePulledRotateScale
#undef ftCommonThrowProcUpdate
#undef ftCommonThrowSetStatus
#undef ftCommonThrowCheckInterruptCatchWait
#undef nFTKirbyStatusThrowF

#define ftCommonThrownReleaseFighterLoseGrip \
    ndsBaseFTCommonThrownReleaseFighterLoseGrip
#define ftCommonThrownDecideFighterLoseGrip \
    ndsBaseFTCommonThrownDecideFighterLoseGrip
#define ftCommonThrownDecideDeadResult ndsBaseFTCommonThrownDecideDeadResult
#define ftCommonThrownProcStatus ndsBaseFTCommonThrownProcStatus
#define ftCommonThrownReleaseThrownUpdateStats \
    ndsBaseFTCommonThrownReleaseThrownUpdateStats
#define ftCommonThrownUpdateDamageStats ndsBaseFTCommonThrownUpdateDamageStats
#define ftCommonThrownSetStatusDamageRelease \
    ndsBaseFTCommonThrownSetStatusDamageRelease
#define ftCommonThrownSetStatusNoDamageRelease \
    ndsBaseFTCommonThrownSetStatusNoDamageRelease

void ndsBaseFTCommonThrownReleaseFighterLoseGrip(GObj *fighter_gobj);
void ndsBaseFTCommonThrownDecideFighterLoseGrip(GObj *fighter_gobj,
                                                GObj *interact_gobj);
void ndsBaseFTCommonThrownDecideDeadResult(GObj *fighter_gobj);
void ndsBaseFTCommonThrownProcStatus(GObj *fighter_gobj);
void ndsBaseFTCommonThrownReleaseThrownUpdateStats(GObj *fighter_gobj,
                                                   s32 lr,
                                                   s32 script_id,
                                                   sb32 is_proc_status);
void ndsBaseFTCommonThrownUpdateDamageStats(FTStruct *this_fp);
void ndsBaseFTCommonThrownSetStatusDamageRelease(GObj *fighter_gobj);
void ndsBaseFTCommonThrownSetStatusNoDamageRelease(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonthrown2.c"

#undef ftCommonThrownReleaseFighterLoseGrip
#undef ftCommonThrownDecideFighterLoseGrip
#undef ftCommonThrownDecideDeadResult
#undef ftCommonThrownProcStatus
#undef ftCommonThrownReleaseThrownUpdateStats
#undef ftCommonThrownUpdateDamageStats
#undef ftCommonThrownSetStatusDamageRelease
#undef ftCommonThrownSetStatusNoDamageRelease
