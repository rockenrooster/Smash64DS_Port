/* Source-function shims: the DS bodies of the BattleShip functions the
 * imported sources call -- runtime gates, P4 colour animations, the
 * game-manager seams, deferred BGM and the fighters' parameter setup
 * (colour animations, joints, effects, parts, subtree invalidation). A slice
 * of reloc_backend.c; its topic slices follow it there: fighter_status,
 * fighter_effects, fighter_parts, map_collision. */
/* NDS_NATURAL_COMBAT_ROUTED_EXTERNS */
#include "nds_scene_harness_config.h"
#include <nds/nds_freeze_diagnostics.h>
#include <nds/nds_effects.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_task39_effect_census.h>
#include <nds/nds_task37_itcm.h>
#include <nds/nds_ftanim_track.h>
#include <nds/nds_ft_pose.h>
#include <nds/nds_kirby_hat_residency.h>
#include <nds/nds_native_stage_blob.h>
#include <nds/nds_scene_manager.h>
#include <nds/nds_p4.h>
#include <nds/nds_p5.h>
#include <sys/vector.h>
#if NDS_P5
#include "nds_p5_native_stages.generated.h"
#endif

void ndsFighterRendererInvalidateMaterialCachesForSlot(u32 slot);
void ndsFighterRendererInvalidateDObjStateCaches(GObj *fighter_gobj);

/* Shield anim-joint install engagement + the lab dispatch audit; legends in
 * include/nds/nds_startup.h beside the declarations. */
extern volatile u32 gNdsShieldAnimJointInstallCalls;
extern volatile u32 gNdsShieldAnimJointAttachCount;
extern volatile u32 gNdsShieldAnimJointNullCount;

static sb32 ndsMPReadMapObj(s32 index, u16 *kind, s16 *x, s16 *y);

static sb32 ndsBattlePlayableRuntimeEnabled(void)
{
#if NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE
    return TRUE;
#else
    return FALSE;
#endif
}

/* Imported BattleShip originals exported by src/import wrappers. */
sb32 ndsBaseFTCommonCatchCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack1CheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonAttack11SetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack11ProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack11ProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonAttack12SetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack12ProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack12ProcInterrupt(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack11CheckGoto(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack100StartCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100StartSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100StartProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100EndSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCatchCheckInterruptAttack11(GObj *fighter_gobj);
sb32 ndsBaseFTCommonGuardOnCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonEscapeCheckInterruptGuard(GObj *fighter_gobj);
sb32 ndsBaseFTCommonGuardCheckInterruptEscape(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCatchCheckInterruptGuard(GObj *fighter_gobj);
sb32 ndsBaseFTCommonGuardPassCheckInterruptGuard(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAppealCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonKneeBendCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDashCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonEscapeCheckInterruptDash(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCatchCheckInterruptDashRun(GObj *fighter_gobj);
void ndsBaseFTCommonAttackDashSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackDashCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonGuardOnCheckInterruptDashRun(GObj *fighter_gobj, s32 slide_tics);
sb32 ndsBaseFTCommonKneeBendCheckInterruptRun(GObj *fighter_gobj);
sb32 ndsBaseFTCommonTurnRunCheckInterruptRun(GObj *fighter_gobj);
void ndsBaseFTCommonTurnProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonTurnProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonTurnSetStatus(GObj *fighter_gobj, s32 lr_dash);
void ndsBaseFTCommonTurnSetStatusCenter(GObj *fighter_gobj);
void ndsBaseFTCommonTurnSetStatusInvertLR(GObj *fighter_gobj);
sb32 ndsBaseFTCommonTurnCheckInputSuccess(GObj *fighter_gobj);
sb32 ndsBaseFTCommonSquatCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonPassCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonPassCheckInterruptSquat(GObj *fighter_gobj);
sb32 ndsBaseFTCommonSquatWaitCheckInterruptLanding(GObj *fighter_gobj);
sb32 ndsBaseFTCommonTurnCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonDashProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDashProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDashProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonDashProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonDashSetStatus(GObj *fighter_gobj, u32 flag);
sb32 ndsBaseFTCommonDashCheckTurn(GObj *fighter_gobj);
void ndsBaseFTCommonRunProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonRunSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonRunCheckInterruptDash(GObj *fighter_gobj);
void ndsBaseFTCommonRunBrakeProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonRunBrakeProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonRunBrakeSetStatus(GObj *fighter_gobj, u32 flag);
sb32 ndsBaseFTCommonRunBrakeCheckInterruptRun(GObj *fighter_gobj);
sb32 ndsBaseFTCommonRunBrakeCheckInterruptTurnRun(GObj *fighter_gobj);
void ndsBaseFTCommonKneeBendProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonKneeBendProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonKneeBendSetStatusParam(GObj *fighter_gobj, s32 status_id, s32 input_source);
void ndsBaseFTCommonKneeBendSetStatus(GObj *fighter_gobj, s32 input_source);
void ndsBaseFTCommonGuardKneeBendSetStatus(GObj *fighter_gobj, s32 input_source);
sb32 ndsBaseFTCommonKneeBendCheckButtonTap(FTStruct *fp);
s32 ndsBaseFTCommonKneeBendGetInputTypeCommon(FTStruct *fp);
s32 ndsBaseFTCommonKneeBendGetInputTypeRun(FTStruct *fp);
sb32 ndsBaseFTCommonGuardKneeBendCheckInterruptGuard(GObj *fighter_gobj);
void ndsBaseFTCommonJumpProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonJumpGetJumpForceButton(s32 stick_range_x, s32 *jump_vel_x, s32 *jump_vel_y, sb32 is_shorthop);
void ndsBaseFTCommonJumpSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonFallProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonFallSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonLandingProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonLandingSetStatusParam(GObj *fighter_gobj, s32 status_id, sb32 is_allow_interrupt, f32 anim_speed);
void ndsBaseFTCommonLandingSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonLandingAirNullSetStatus(GObj *fighter_gobj, f32 anim_speed);
void ndsBaseFTCommonLandingAirSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonLandingFallSpecialSetStatus(GObj *fighter_gobj, sb32 is_allow_interrupt, f32 anim_speed);
sb32 ndsBaseFTCommonAttackAirCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonAttackAirProcMap(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS4CheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS4CheckInterruptTurn(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackHi4CheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackLw4CheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackLw4CheckInterruptSquat(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS3CheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackHi3CheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackLw3CheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackS4CheckInterruptDash(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackHi4CheckInterruptKneeBend(GObj *fighter_gobj);
sb32 ndsBaseFTCommonJumpAerialCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonCatchPullProcCatch(GObj *fighter_gobj);
void ndsBaseFTCommonCapturePulledProcCapture(GObj *fighter_gobj, GObj *capture_gobj);
void ndsBaseFTCommonThrownSetStatusDamageRelease(GObj *fighter_gobj);
void ndsBaseFTCommonThrownUpdateDamageStats(FTStruct *this_fp);
void ndsBaseFTCommonThrownSetStatusNoDamageRelease(GObj *fighter_gobj);
#if NDS_P2_DONKEY
void ndsBaseFTCommonCaptureShoulderedProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonCaptureShoulderedSetStatus(GObj *fighter_gobj);
#endif
void ndsBaseFTCommonThrownDecideFighterLoseGrip(GObj *fighter_gobj, GObj *interact_gobj);
void ndsBaseFTCommonThrownDecideDeadResult(GObj *fighter_gobj);
sb32 ndsBaseFTCommonThrowCheckInterruptCatchWait(GObj *fighter_gobj);
void ndsBaseFTCommonThrownReleaseThrownUpdateStats(GObj *fighter_gobj, s32 lr, s32 script_id, sb32 is_proc_status);
void ndsBaseFTCommonThrownReleaseFighterLoseGrip(GObj *fighter_gobj);
void ndsBaseFTCommonDamageSetPublic(FTStruct *fp, f32 knockback, f32 angle);
void ndsBaseFTCommonDamageSetDustEffectInterval(FTStruct *fp);
s32 ndsBaseFTCommonDamageGetDamageLevel(f32 hitstun);
sb32 ndsBaseFTCommonDamageCheckCatchResist(FTStruct *fp);
sb32 ndsBaseFTCommonDamageCheckCaptureKeepHold(FTStruct *fp);
sb32 ndsBaseFTCommonDamageCheckElementSetColAnim(GObj *fighter_gobj, s32 element, s32 damage_level);
void ndsBaseFTCommonDamageCheckMakeScreenFlash(f32 knockback, s32 element);
void ndsBaseFTCommonDamageInitDamageVars(GObj *fighter_gobj, s32 status_id_replace, s32 damage, f32 knockback, s32 angle_start, s32 damage_lr, s32 damage_index, s32 element, s32 damage_player_num, s32 arg9, sb32 unk_bool, sb32 is_public);
void ndsBattleShipIFScreenFlashSetColAnimID(s32 colanim_id, s32 colanim_duration);
void ndsBaseFTCommonDamageGotoDamageStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDamageUpdateDamageColAnim(GObj *fighter_gobj, f32 knockback, s32 element);
void ndsBaseFTCommonDamageSetDamageColAnim(GObj *fighter_gobj);
void ndsBaseFTCommonDamageUpdateMain(GObj *fighter_gobj);
void ndsBaseFTCommonFuraSleepSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonTwisterSetStatus(GObj *fighter_gobj, GObj *tornado_gobj);
void ndsBaseFTCommonReboundProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonReboundSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonReboundWaitProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonReboundWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonGuardSetOffProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonGuardSetOffSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCliffAttackCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCliffEscapeCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCliffClimbOrFallCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonDamageUpdateDustEffect(GObj *fighter_gobj);
void ndsBaseFTCommonDamageDecHitStunSetPublic(GObj *fighter_gobj);
void ndsBaseFTCommonDamageUpdateCatchResist(GObj *fighter_gobj);
void ndsBaseFTCommonDamageCommonProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDamageAirCommonProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDamageCheckSetInvincible(GObj *fighter_gobj);
void ndsBaseFTCommonDamageSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDamageCommonProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDamageAirCommonProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFlyRollUpdateModelPitch(GObj *fighter_gobj);
void ndsBaseFTCommonDamageCommonProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonDamageCommonProcLagUpdate(GObj *fighter_gobj);
sb32 ndsBaseFTCommonWallDamageCheckGoto(GObj *fighter_gobj);
void ndsBaseFTCommonDamageAirCommonProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallClampRumble(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallSetStatusFromDamage(GObj *fighter_gobj);
sb32 ndsBaseFTCommonPassiveStandCheckInterruptDamage(GObj *fighter_gobj);
sb32 ndsBaseFTCommonPassiveCheckInterruptDamage(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonCliffCatchSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDownWaitProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDownWaitProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDownWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceProcUpdate(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDownBounceCheckUpOrDown(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceUpdateEffects(GObj *fighter_gobj);
void ndsBaseFTCommonDownStandProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDownStandSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonShieldBreakFlyCommonSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonShieldBreakFlyReflectorSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDownAttackSetStatus(GObj *fighter_gobj, s32 status_id);
sb32 ndsBaseFTCommonDownAttackCheckInterruptDownWait(GObj *fighter_gobj);
void ndsBaseFTCommonDownForwardOrBackSetStatus(GObj *fighter_gobj, s32 status_id);
sb32 ndsBaseFTCommonDownForwardOrBackCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDownStandCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDownAttackCheckInterruptDownBounce(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallSetStatusFromCliffWait(GObj *fighter_gobj);
void ndsBaseFTCommonCliffCommon2UpdateCollData(GObj *fighter_gobj);
void ndsBaseFTCommonCliffCommon2InitStatusVars(GObj *fighter_gobj);

void ndsBaseFTCommonDashProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDashProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDashProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonDashProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonDashSetStatus(GObj *fighter_gobj, u32 flag);
sb32 ndsBaseFTCommonDashCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDashCheckTurn(GObj *fighter_gobj);
void ndsBaseFTCommonTurnProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonTurnProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonTurnSetStatus(GObj *fighter_gobj, s32 lr_dash);
void ndsBaseFTCommonTurnSetStatusCenter(GObj *fighter_gobj);
void ndsBaseFTCommonTurnSetStatusInvertLR(GObj *fighter_gobj);
sb32 ndsBaseFTCommonTurnCheckInputSuccess(GObj *fighter_gobj);
sb32 ndsBaseFTCommonTurnCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonRunProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonRunSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonRunCheckInterruptDash(GObj *fighter_gobj);
void ndsBaseFTCommonTurnRunProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonTurnRunProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonTurnRunSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonTurnRunCheckInterruptRun(GObj *fighter_gobj);
void ndsBaseFTCommonRunBrakeProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonRunBrakeProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonRunBrakeSetStatus(GObj *fighter_gobj, u32 flag);
sb32 ndsBaseFTCommonRunBrakeCheckInterruptRun(GObj *fighter_gobj);
sb32 ndsBaseFTCommonRunBrakeCheckInterruptTurnRun(GObj *fighter_gobj);
void ndsBaseFTCommonKneeBendProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonKneeBendProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonKneeBendSetStatusParam(GObj *fighter_gobj,
                                           s32 status_id,
                                           s32 input_source);
void ndsBaseFTCommonKneeBendSetStatus(GObj *fighter_gobj,
                                      s32 input_source);
void ndsBaseFTCommonGuardKneeBendSetStatus(GObj *fighter_gobj,
                                           s32 input_source);
sb32 ndsBaseFTCommonKneeBendCheckButtonTap(FTStruct *fp);
s32 ndsBaseFTCommonKneeBendGetInputTypeCommon(FTStruct *fp);
sb32 ndsBaseFTCommonKneeBendCheckInterruptCommon(GObj *fighter_gobj);
s32 ndsBaseFTCommonKneeBendGetInputTypeRun(FTStruct *fp);
sb32 ndsBaseFTCommonKneeBendCheckInterruptRun(GObj *fighter_gobj);
sb32 ndsBaseFTCommonGuardKneeBendCheckInterruptGuard(GObj *fighter_gobj);
void ndsBaseFTCommonSquatProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonSquatWaitProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonSquatWaitProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonSquatWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonSquatRvProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonJumpProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonJumpGetJumpForceButton(s32 stick_range_x,
                                           s32 *jump_vel_x,
                                           s32 *jump_vel_y,
                                           sb32 is_shorthop);
void ndsBaseFTCommonJumpSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonFallProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonFallSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonOttottoSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCliffAttackCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCliffEscapeCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCliffClimbOrFallCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonCliffCommon2UpdateCollData(GObj *fighter_gobj);
void ndsBaseFTCommonCliffCommon2InitStatusVars(GObj *fighter_gobj);
void ndsBaseFTCommonCliffCatchSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallClampRumble(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallSetStatusFromDamage(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFallSetStatusFromCliffWait(GObj *fighter_gobj);
f32 ndsBaseFTCommonDamageGetKnockbackAngle(s32 angle_i, sb32 ga,
                                           f32 knockback);
s32 ndsBaseFTCommonDamageGetDamageLevel(f32 hitstun);
void ndsBaseFTCommonDamageDecHitStunSetPublic(GObj *fighter_gobj);
void ndsBaseFTCommonDamageCheckSetInvincible(GObj *fighter_gobj);
void ndsBaseFTCommonDamageSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDamageCommonProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDamageAirCommonProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDamageCommonProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDamageAirCommonProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFlyRollUpdateModelPitch(GObj *fighter_gobj);
void ndsBaseFTCommonDamageAirCommonProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonDamageCommonProcLagUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDamageCommonProcPhysics(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDamageCheckElementSetColAnim(GObj *fighter_gobj,
                                                 s32 element,
                                                 s32 damage_level);
void ndsBaseFTCommonDamageCheckMakeScreenFlash(f32 knockback, s32 element);
void ndsBaseFTCommonDamageSetDustEffectInterval(FTStruct *fp);
void ndsBaseFTCommonDamageUpdateDustEffect(GObj *fighter_gobj);
void ndsBaseFTCommonDamageSetPublic(FTStruct *this_fp, f32 knockback,
                                    f32 angle);
sb32 ndsBaseFTCommonDamageCheckCatchResist(FTStruct *fp);
sb32 ndsBaseFTCommonDamageCheckCaptureKeepHold(FTStruct *fp);
void ndsBaseFTCommonDamageInitDamageVars(GObj *fighter_gobj,
                                         s32 status_id_replace, s32 damage,
                                         f32 knockback, s32 angle_start,
                                         s32 damage_lr, s32 damage_index,
                                         s32 element, s32 damage_player_num,
                                         s32 arg9, sb32 unk_bool,
                                         sb32 is_public);
void ndsBaseFTCommonDamageGotoDamageStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDamageUpdateCatchResist(GObj *fighter_gobj);
void ndsBaseFTCommonDamageUpdateDamageColAnim(GObj *fighter_gobj,
                                              f32 knockback, s32 element);
void ndsBaseFTCommonDamageSetDamageColAnim(GObj *fighter_gobj);
void ndsBaseFTCommonDamageUpdateMain(GObj *fighter_gobj);
void ftCommonDamageUpdateDamageColAnim(GObj *fighter_gobj, f32 knockback,
                                       s32 element);
void ftCommonDamageSetDamageColAnim(GObj *fighter_gobj);
void ftCommonDamageCommonProcUpdate(GObj *fighter_gobj);
void ftCommonDamageAirCommonProcUpdate(GObj *fighter_gobj);
void ftCommonDamageCommonProcInterrupt(GObj *fighter_gobj);
void ftCommonDamageAirCommonProcInterrupt(GObj *fighter_gobj);
void ftCommonDamageCheckSetInvincible(GObj *fighter_gobj);
void ftCommonDamageSetStatus(GObj *fighter_gobj);
void ftCommonDamageCommonProcPhysics(GObj *fighter_gobj);
void ftCommonDamageCommonProcLagUpdate(GObj *fighter_gobj);
void ftCommonDamageAirCommonProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonPassiveSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonPassiveCheckInterruptDamage(GObj *fighter_gobj);
void ndsBaseFTCommonPassiveStandSetStatus(GObj *fighter_gobj,
                                          s32 status_id);
sb32 ndsBaseFTCommonPassiveStandCheckInterruptDamage(GObj *fighter_gobj);
void ndsBaseFTCommonReboundProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonReboundSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonReboundWaitProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonReboundWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonGuardSetOffProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonGuardSetOffSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDownWaitProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDownWaitProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDownWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceProcUpdate(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDownBounceCheckUpOrDown(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceUpdateEffects(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDownStandProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDownStandSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDownStandCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonDownAttackSetStatus(GObj *fighter_gobj, s32 status_id);
sb32 ndsBaseFTCommonDownAttackCheckInterruptDownBounce(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDownAttackCheckInterruptDownWait(GObj *fighter_gobj);
void ndsBaseFTCommonDownForwardOrBackSetStatus(GObj *fighter_gobj,
                                               s32 status_id);
sb32 ndsBaseFTCommonDownForwardOrBackCheckInterruptCommon(
    GObj *fighter_gobj);
void ndsBaseFTCommonLandingProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonLandingSetStatusParam(GObj *fighter_gobj,
                                          s32 status_id,
                                          sb32 is_allow_interrupt,
                                          f32 anim_speed);
void ndsBaseFTCommonLandingSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonLandingAirNullSetStatus(GObj *fighter_gobj,
                                            f32 anim_speed);
void ndsBaseFTCommonLandingFallSpecialSetStatus(GObj *fighter_gobj,
                                                sb32 is_allow_interrupt,
                                                f32 anim_speed);

f32 dMPCollisionMaterialFrictions[16] = {
    4.0F, 3.0F, 3.0F, 1.0F,
    2.0F, 2.0F, 4.0F, 4.0F,
    4.0F, 4.0F, 4.0F, 4.0F,
    4.0F, 4.0F, 4.0F, 4.0F
};

/* decomp sc/sc1pmode/sc1pgame.c:716,723; with NDS_P2_1P_GAME the included
 * sc1pgame.c (battleship_sc1pgame_runtime.c) owns them. */
#if !NDS_P2_1P_GAME
u8 gSC1PGameBonusStarCount;
u8 gSC1PGameBonusGiantImpact;
#endif


#define NDS_GM_COL_FIELD(value, start, len) \
    ((((u32)(value)) & ((1u << (len)) - 1u)) << (start))
/* ARM allocates these imported u32 bitfields least-significant field first. */
#define NDS_GM_COL_COMMAND_END() NDS_GM_COL_FIELD(nGMColEventEnd, 0, 6)
#define NDS_GM_COL_COMMAND_WAIT(frames) \
    (NDS_GM_COL_FIELD(nGMColEventWait, 0, 6) | \
     NDS_GM_COL_FIELD(frames, 6, 26))
#define NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL() \
    NDS_GM_COL_FIELD(nGMColEventClearColorAll, 0, 6)
#define NDS_GM_COL_COMMAND_SET_COLOR1_S1() \
    NDS_GM_COL_FIELD(nGMColEventSetColor1, 0, 6)
#define NDS_GM_COL_COMMAND_SET_COLOR1_S2(r, g, b, a) \
    (NDS_GM_COL_FIELD(r, 0, 8) | NDS_GM_COL_FIELD(g, 8, 8) | \
     NDS_GM_COL_FIELD(b, 16, 8) | NDS_GM_COL_FIELD(a, 24, 8))
#define NDS_GM_COL_COMMAND_SET_COLOR1(r, g, b, a) \
    NDS_GM_COL_COMMAND_SET_COLOR1_S1(), \
        NDS_GM_COL_COMMAND_SET_COLOR1_S2(r, g, b, a)
#define NDS_GM_COL_COMMAND_BLEND_COLOR1_S1(frames) \
    (NDS_GM_COL_FIELD(nGMColEventBlendColor1, 0, 6) | \
     NDS_GM_COL_FIELD(frames, 6, 26))
#define NDS_GM_COL_COMMAND_BLEND_COLOR1_S2(r, g, b, a) \
    (NDS_GM_COL_FIELD(r, 0, 8) | NDS_GM_COL_FIELD(g, 8, 8) | \
     NDS_GM_COL_FIELD(b, 16, 8) | NDS_GM_COL_FIELD(a, 24, 8))
#define NDS_GM_COL_COMMAND_BLEND_COLOR1(frames, r, g, b, a) \
    NDS_GM_COL_COMMAND_BLEND_COLOR1_S1(frames), \
        NDS_GM_COL_COMMAND_BLEND_COLOR1_S2(r, g, b, a)
#define NDS_GM_COL_COMMAND_SET_LIGHT(angle1, angle2) \
    (NDS_GM_COL_FIELD(nGMColEventSetLight, 0, 6) | \
     NDS_GM_COL_FIELD(angle1, 6, 13) | \
     NDS_GM_COL_FIELD(angle2, 19, 13))
#define NDS_GM_COL_COMMAND_CLEAR_LIGHT() \
    NDS_GM_COL_FIELD(nGMColEventClearLight, 0, 6)
#define NDS_GM_COL_COMMAND_SET_SKELETON_ID(skeleton_id) \
    (NDS_GM_COL_FIELD(nGMColEventSetSkeletonID, 0, 6) | \
     NDS_GM_COL_FIELD(skeleton_id, 6, 26))
/* gmdef.h:130 gmColCommandPlayFGM, in the port's LSB-first GMColEventDefault
 * layout (opcode:6, value:26). ftmain.c's colanim step plays `value`. */
#define NDS_GM_COL_COMMAND_PLAY_FGM(sfx_id) \
    (NDS_GM_COL_FIELD(nGMColEventPlayFGM, 0, 6) | \
     NDS_GM_COL_FIELD(sfx_id, 6, 26))
/* Loop/subroutine/effect, needed by the fire-damage scripts below. Layouts are
 * the port's own GMColEvent structs (fighter.h:583+), i.e. LSB-first as the
 * comment above records -- NOT the source's MSB-first GC_FIELDSET shifts. */
#define NDS_GM_COL_COMMAND_LOOP_BEGIN(count) \
    (NDS_GM_COL_FIELD(nGMColEventLoopBegin, 0, 6) | \
     NDS_GM_COL_FIELD(count, 6, 26))
#define NDS_GM_COL_COMMAND_LOOP_END() \
    NDS_GM_COL_FIELD(nGMColEventLoopEnd, 0, 6)
#define NDS_GM_COL_COMMAND_SUBROUTINE(addr) \
    NDS_GM_COL_FIELD(nGMColEventSubroutine, 0, 6), ((u32)(uintptr_t)(addr))
#define NDS_GM_COL_COMMAND_RETURN() \
    NDS_GM_COL_FIELD(nGMColEventReturn, 0, 6)
#define NDS_GM_COL_COMMAND_GOTO(addr) \
    NDS_GM_COL_FIELD(nGMColEventGoto, 0, 6), ((u32)(uintptr_t)(addr))
/* GMColEventMakeEffect1 is opcode:6, joint_id:7 (SIGNED), effect_id:9, flag:10;
 * 2/3/4 are two s16 halves each, low half first. */
#define NDS_GM_COL_COMMAND_EFFECT_S1(joint, effect_id, flag) \
    (NDS_GM_COL_FIELD(nGMColEventEffect, 0, 6) | \
     NDS_GM_COL_FIELD(joint, 6, 7) | \
     NDS_GM_COL_FIELD(effect_id, 13, 9) | \
     NDS_GM_COL_FIELD(flag, 22, 10))
#define NDS_GM_COL_COMMAND_EFFECT_S2(off_x, off_y) \
    (NDS_GM_COL_FIELD(off_x, 0, 16) | NDS_GM_COL_FIELD(off_y, 16, 16))
#define NDS_GM_COL_COMMAND_EFFECT_S3(off_z, rng_x) \
    (NDS_GM_COL_FIELD(off_z, 0, 16) | NDS_GM_COL_FIELD(rng_x, 16, 16))
#define NDS_GM_COL_COMMAND_EFFECT_S4(rng_y, rng_z) \
    (NDS_GM_COL_FIELD(rng_y, 0, 16) | NDS_GM_COL_FIELD(rng_z, 16, 16))
#define NDS_GM_COL_COMMAND_EFFECT(joint, effect_id, flag, off_x, off_y, off_z, \
                                  rng_x, rng_y, rng_z) \
    NDS_GM_COL_COMMAND_EFFECT_S1(joint, effect_id, flag), \
        NDS_GM_COL_COMMAND_EFFECT_S2(off_x, off_y), \
        NDS_GM_COL_COMMAND_EFFECT_S3(off_z, rng_x), \
        NDS_GM_COL_COMMAND_EFFECT_S4(rng_y, rng_z)

/* P2-2 source-parity set: these are the common fighter colour-animation
 * scripts exercised by normal Mario/Fox VS play, CSS CPU previews, respawn,
 * fast-fall, healing/invincibility and the Hammer status family. They are a
 * direct command-for-command transcription of gmcolscripts.c; keeping them in
 * the DS-native event layout avoids reintroducing source relocation work while
 * preserving BattleShip's priorities, looping and effect requests. */
/* P4: Remix's extended colour animations (ids 0x56 on) the enabled contents
 * reach (scripts/p4/p4_colanims.py), appended to the table below. */
#include "nds_p4_colanims.generated.inc"
#define NDS_GM_COL_DESC_COUNT (NDS_P4_COLANIM_LAST + 1)
/* An authored content's tinted copies of its base's colour animations (Ike's
 * blue Eruption from Roy's ROY_CHARGE): the content's SetColAnim of the
 * source id, or that of a Kirby holding its power, plays the copy. */
typedef struct NDSP4ColAnimRemap
{
    u16 content;
    u16 from;
    u16 to;
} NDSP4ColAnimRemap;
static const NDSP4ColAnimRemap sNdsP4ColAnimRemap[] = NDS_P4_COLANIM_REMAP;

static s32 ndsP4ColAnimRemap(const FTStruct *fp, s32 colanim_id)
{
    u32 content = (fp->nds_p4_content != 0u) ? fp->nds_p4_content : fp->nds_p4_kirby_copy;
    u32 i;

    if (content == 0u)
    {
        return colanim_id;
    }
    for (i = 0u; i < NDS_P4_COLANIM_REMAP_COUNT; i++)
    {
        if ((sNdsP4ColAnimRemap[i].content == content) &&
            ((s32)sNdsP4ColAnimRemap[i].from == colanim_id))
        {
            return (s32)sNdsP4ColAnimRemap[i].to;
        }
    }
    return colanim_id;
}

static u32 sNdsGMColScriptsFighterComPlayer[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x30),
    NDS_GM_COL_COMMAND_WAIT(65535),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterComPlayer)
};

static u32 sNdsGMColScriptsFighterHitStatusNormal[] = {
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_END()
};

static u32 sNdsGMColScriptsFighterHitStatusIntangible[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x82),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(3, 0xFF, 0xFF, 0xFF, 0x32),
    NDS_GM_COL_COMMAND_WAIT(3),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterHitStatusIntangible)
};

static u32 sNdsGMColScriptsFighterHitStatusInvincible[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0x80, 0xFF, 0x80, 0x50),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(3, 0x80, 0xFF, 0x80, 0x14),
    NDS_GM_COL_COMMAND_WAIT(3),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterHitStatusInvincible)
};

static u32 sNdsGMColScriptsFighterFallSpecial[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0x00, 0x00, 0x5A),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterFallSpecial)
};

static u32 sNdsGMColScriptsFighterFastFall[] = {
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindSparkleWhiteScale, 0, 0, 0, 0, 0, 0, 0),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0x00, 0x00, 0x80),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_END()
};

static u32 sNdsGMColScriptsFighterHeal[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0x40, 0x96),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindHealSparkles, 0, 0, 0, 0, 300, 300, 300),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0x40, 0x14),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindHealSparkles, 0, 0, 0, 0, 300, 300, 300),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterHeal)
};

static u32 sNdsGMColScriptsFighterNoDamage[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x80),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(6, 0xFF, 0xC8, 0xA0, 0x1E),
    NDS_GM_COL_COMMAND_WAIT(6),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(3, 0xFF, 0xFF, 0xFF, 0xA0),
    NDS_GM_COL_COMMAND_WAIT(3),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterNoDamage)
};

static u32 sNdsGMColScriptsFighterRebirth[] = {
    NDS_GM_COL_COMMAND_SET_LIGHT(0, -70),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0xFF),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(36, 0xFF, 0xFF, 0xFF, 0x0A),
    NDS_GM_COL_COMMAND_WAIT(36),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(18, 0xFF, 0xFF, 0xFF, 0xB4),
    NDS_GM_COL_COMMAND_WAIT(18),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterRebirth)
};

/* BattleShip gmcolscripts.c Pikachu set, transcribed into the DS encoding.
 * SpecialHiStart is a single PlayFGM with no End in the source; like Fox's
 * SpecialHiStart it relies on falling into the next array (SpecialHi's spark
 * loop), and ftpikachuspecialhi.c only ever requests the Start id, so the DS
 * script makes that control transfer explicit. */
static u32 sNdsGMColScriptsFighterPikachuAttackS4[] = {
    NDS_GM_COL_COMMAND_LOOP_BEGIN(4),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x64),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(6, 0x00, 0x00, 0xFF, 0x00),
    NDS_GM_COL_COMMAND_WAIT(6),
    NDS_GM_COL_COMMAND_EFFECT(-1, nEFKindShockSmall, 0, 0, 0, 0, 0, 0, 0),
    NDS_GM_COL_COMMAND_LOOP_END(),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterPikachuAttackS4)
};
static u32 sNdsGMColScriptsFighterPikachuSpecialHi[] = {
    NDS_GM_COL_COMMAND_EFFECT(-1, nEFKindShockSmall, 0, 0, 0, 0, 150, 150, 150),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterPikachuSpecialHi)
};
static u32 sNdsGMColScriptsFighterPikachuSpecialHiStart[] = {
    NDS_GM_COL_COMMAND_PLAY_FGM(nSYAudioFGMPikachuSpecialHiStart),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterPikachuSpecialHi)
};
static u32 sNdsGMColScriptsFighterPikachuSpecialN[] = {
    NDS_GM_COL_COMMAND_LOOP_BEGIN(4),
    NDS_GM_COL_COMMAND_LOOP_BEGIN(3),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0x00, 0xFF, 0x32),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x5A),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_LOOP_END(),
    NDS_GM_COL_COMMAND_EFFECT(-1, nEFKindShockSmall, 0, 0, 0, 0, 0, 0, 0),
    NDS_GM_COL_COMMAND_LOOP_END(),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterPikachuSpecialN)
};
static u32 sNdsGMColScriptsFighterPikachuSpecialLwHit[] = {
    NDS_GM_COL_COMMAND_LOOP_BEGIN(4),
    NDS_GM_COL_COMMAND_LOOP_BEGIN(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0x00, 0xFF, 0x5A),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x64),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_LOOP_END(),
    NDS_GM_COL_COMMAND_EFFECT(-1, nEFKindShockSmall, 0, 0, 0, 0, 100, 100, 100),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindSparkleWhiteScale, 0, 0, 150, 0,
                              400, 400, 400),
    NDS_GM_COL_COMMAND_LOOP_END(),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterPikachuSpecialLwHit)
};
static u32 sNdsGMColScriptsFighterPikachuSpecialLwEnd[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x50),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0x00, 0x00, 0x50),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterPikachuSpecialLwEnd)
};

static u32 sNdsGMColScriptsFighterHammer[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0x00, 0x00, 0x80),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x80, 0xFF, 0x00, 0x64),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterHammer)
};

static u32 sNdsGMColScriptsFighterStar[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x64),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindPsionic, 0, 0, 100, 0, 180, 180, 180),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindHealSparkles, 0, 0, 0, 0, 300, 300, 300),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(3, 0xFF, 0xC8, 0xA0, 0x8C),
    NDS_GM_COL_COMMAND_WAIT(3),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindHealSparkles, 0, 0, 0, 0, 300, 300, 300),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(3, 0x64, 0xC8, 0x64, 0x6E),
    NDS_GM_COL_COMMAND_WAIT(3),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindHealSparkles, 0, 0, 0, 0, 300, 300, 300),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(3, 0x00, 0x00, 0x28, 0x8C),
    NDS_GM_COL_COMMAND_WAIT(3),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindHealSparkles, 0, 0, 0, 0, 300, 300, 300),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(3),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterStar)
};

/* BattleShip gmcolscripts.c:213-343. Mario and Fox both use the common
 * skeleton electric family (dFTParamSkeletonColAnimIDs[] = 0x14 for both).
 * This is gameplay presentation, not a diagnostic-only effect: the source
 * electric damage path asks ftParamCheckSetSkeletonColAnimID for one of these
 * four scripts and each script also owns the repeated ShockSmall effects. */
static u32 sNdsGMColScriptsFighterDamageElectricCommonSub[] = {
    NDS_GM_COL_COMMAND_LOOP_BEGIN(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0x00, 0x94, 0x5A),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x96),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_LOOP_END(),
    NDS_GM_COL_COMMAND_RETURN()
};

static u32 sNdsGMColScriptsFighterDamageElectricSkeletonSub[] = {
    NDS_GM_COL_COMMAND_LOOP_BEGIN(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x14, 0x14, 0x14, 0xFF),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_SET_SKELETON_ID(1),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_LOOP_END(),
    NDS_GM_COL_COMMAND_RETURN()
};

#define NDS_GM_COL_ELECTRIC_SKELETON_BODY(skeleton_count, common_count) \
    NDS_GM_COL_COMMAND_LOOP_BEGIN(skeleton_count), \
    NDS_GM_COL_COMMAND_EFFECT(-1, nEFKindShockSmall, 0, 0, 0, 0, 0, 0, 0), \
    NDS_GM_COL_COMMAND_SUBROUTINE( \
        sNdsGMColScriptsFighterDamageElectricSkeletonSub), \
    NDS_GM_COL_COMMAND_LOOP_END(), \
    NDS_GM_COL_COMMAND_LOOP_BEGIN(common_count), \
    NDS_GM_COL_COMMAND_EFFECT(-1, nEFKindShockSmall, 0, 0, 0, 0, 0, 0, 0), \
    NDS_GM_COL_COMMAND_SUBROUTINE( \
        sNdsGMColScriptsFighterDamageElectricCommonSub), \
    NDS_GM_COL_COMMAND_LOOP_END(), \
    NDS_GM_COL_COMMAND_END()

static u32 sNdsGMColScriptsFighterDamageElectricSkeletonWeak[] = {
    NDS_GM_COL_ELECTRIC_SKELETON_BODY(2, 2)
};
static u32 sNdsGMColScriptsFighterDamageElectricSkeletonMid[] = {
    NDS_GM_COL_ELECTRIC_SKELETON_BODY(3, 5)
};
static u32 sNdsGMColScriptsFighterDamageElectricSkeletonStrong[] = {
    NDS_GM_COL_ELECTRIC_SKELETON_BODY(4, 8)
};
static u32 sNdsGMColScriptsFighterDamageElectricSkeletonFly[] = {
    NDS_GM_COL_ELECTRIC_SKELETON_BODY(5, 11)
};

/* gmcolscripts.c:225-263, the source's own family for bodies without an
 * electric skeleton (dFTParamSkeletonColAnimIDs 0x10). */
#define NDS_GM_COL_ELECTRIC_COMMON_BODY(common_count) \
    NDS_GM_COL_COMMAND_LOOP_BEGIN(common_count), \
    NDS_GM_COL_COMMAND_EFFECT(-1, nEFKindShockSmall, 0, 0, 0, 0, 0, 0, 0), \
    NDS_GM_COL_COMMAND_SUBROUTINE( \
        sNdsGMColScriptsFighterDamageElectricCommonSub), \
    NDS_GM_COL_COMMAND_LOOP_END(), \
    NDS_GM_COL_COMMAND_END()

static u32 sNdsGMColScriptsFighterDamageElectricCommonWeak[] = {
    NDS_GM_COL_ELECTRIC_COMMON_BODY(4)
};
static u32 sNdsGMColScriptsFighterDamageElectricCommonMid[] = {
    NDS_GM_COL_ELECTRIC_COMMON_BODY(8)
};
static u32 sNdsGMColScriptsFighterDamageElectricCommonStrong[] = {
    NDS_GM_COL_ELECTRIC_COMMON_BODY(12)
};
static u32 sNdsGMColScriptsFighterDamageElectricCommonFly[] = {
    NDS_GM_COL_ELECTRIC_COMMON_BODY(16)
};

/* Current-roster damage/status scripts from gmcolscripts.c:500-684.  These
 * were still absent after the generic hit-status restore, so the imported
 * source motion/status code requested the right IDs and the DS descriptor
 * table rejected them for having no script. */
static u32 sNdsGMColScriptsFighterDamageIceWeak[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0x00, 0xFF, 0x80),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x80),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_END()
};
static u32 sNdsGMColScriptsFighterDamageIceMid[] = {
    NDS_GM_COL_COMMAND_END()
};
static u32 sNdsGMColScriptsFighterDamageIceStrong[] = {
    NDS_GM_COL_COMMAND_END()
};
static u32 sNdsGMColScriptsFighterDamageIceFly[] = {
    NDS_GM_COL_COMMAND_END()
};

static u32 sNdsGMColScriptsFighterShieldBreakFly[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0x80, 0x00, 0x00, 0x80),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(10, 0x80, 0x00, 0x00, 0x14),
    NDS_GM_COL_COMMAND_WAIT(10),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterShieldBreakFly)
};

static u32 sNdsGMColScriptsFighterFuraFura[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0x80, 0x00, 0x00, 0x80),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(12, 0x80, 0x00, 0x00, 0x14),
    NDS_GM_COL_COMMAND_WAIT(12),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(6, 0x80, 0x00, 0x00, 0x80),
    NDS_GM_COL_COMMAND_WAIT(6),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterFuraFura)
};

static u32 sNdsGMColScriptsFighterFuraSleep[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0x80, 0x50, 0x64, 0x46),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(12, 0xF0, 0x3C, 0x8C, 0x82),
    NDS_GM_COL_COMMAND_WAIT(12),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(12, 0xF0, 0x3C, 0x8C, 0x3C),
    NDS_GM_COL_COMMAND_WAIT(12),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(6),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterFuraSleep)
};

static u32 sNdsGMColScriptsFighterMarioSpecialN[] = {
    NDS_GM_COL_COMMAND_SET_LIGHT(75, -10),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0x00, 0x00, 0xFF),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0x00, 0x00, 0x80),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(20),
    NDS_GM_COL_COMMAND_CLEAR_LIGHT(),
    NDS_GM_COL_COMMAND_END()
};

static u32 sNdsGMColScriptsFighterMarioAppeal[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0x80, 0xAA),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(3, 0x50, 0x50, 0x50, 0x5A),
    NDS_GM_COL_COMMAND_WAIT(3),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(3, 0x50, 0x50, 0x50, 0x00),
    NDS_GM_COL_COMMAND_WAIT(3),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(4, 0xFF, 0xFF, 0x80, 0xAA),
    NDS_GM_COL_COMMAND_WAIT(3),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterMarioAppeal),
    NDS_GM_COL_COMMAND_END()
};

/* Fox's motion data reuses these two Donkey color-animation IDs. Preserve the
 * data contract instead of renaming/remapping it in the motion interpreter. */
static u32 sNdsGMColScriptsFighterDonkeySpecialNLoop[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0x80, 0x46),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0x80, 0x28),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x3C, 0x3C, 0x00, 0x64),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_END()
};

static u32 sNdsGMColScriptsFighterDonkeySpecialNEnd[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0x80, 0xB4),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(5, 0xFF, 0xFF, 0x80, 0x00),
    NDS_GM_COL_COMMAND_WAIT(5),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_END()
};

static u32 sNdsGMColScriptsFighterUnknown1[] = {
    NDS_GM_COL_COMMAND_END()
};

static u32 sNdsGMColScriptsFighterFoxSpecialLw[] = {
    NDS_GM_COL_COMMAND_SET_LIGHT(0, -80),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0xFF, 0xFF, 0x50),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterFoxSpecialLw)
};

static u32 sNdsGMColScriptsFighterFoxSpecialHiHold[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0xB4),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0x00, 0xB4),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterFoxSpecialHiHold)
};

static u32 sNdsGMColScriptsFighterFoxSpecialHiStart[] = {
    NDS_GM_COL_COMMAND_SET_LIGHT(0, -80),
    NDS_GM_COL_COMMAND_LOOP_BEGIN(4),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x1E),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0x00, 0x1E),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_LOOP_END(),
    NDS_GM_COL_COMMAND_LOOP_BEGIN(3),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x64),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0x00, 0x64),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_LOOP_END(),
    /* Source relies on the adjacent HiHold array after the no-End Start
     * script. C does not promise separate static arrays are contiguous, so the
     * DS-native encoding makes the same control transfer explicit. */
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterFoxSpecialHiHold)
};

static u32 sNdsGMColScriptsFighterFoxSpecialHi[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0x00, 0xDC),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(30, 0xA0, 0x3C, 0x3C, 0x8C),
    NDS_GM_COL_COMMAND_WAIT(30),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(20, 0x80, 0x00, 0x00, 0x00),
    NDS_GM_COL_COMMAND_WAIT(20),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_END()
};

/* Exact decomp gmcolscripts.c dGMColScriptsFighterDamageCommon commands. */
static u32 sNdsGMColScriptsFighterDamageCommon[] = {
    NDS_GM_COL_COMMAND_SET_LIGHT(90, 0),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0xE6),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(6, 0xFF, 0xFF, 0xFF, 0x1E),
    NDS_GM_COL_COMMAND_WAIT(6),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_END()
};

/* BattleShip gmcolscripts.c:62-74. Donkey/Samus/Kirby install this when their
 * stored neutral-special charge reaches max. The old compatibility table had
 * no descriptor for id 6, so ftParamCheckSetColAnimID rejected the source
 * request before either the sparkle or the persistent full-charge pulse could
 * execute. Keep the source's infinite self-goto and 10/FALSE priority exactly. */
static u32 sNdsGMColScriptsFighterCommonSpecialNCharge[] = {
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindChargeSparkle, 0, 0, 0, 0, 0, 0, 0),
    NDS_GM_COL_COMMAND_LOOP_BEGIN(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x30),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(4, 0xFF, 0xFF, 0xFF, 0x00),
    NDS_GM_COL_COMMAND_WAIT(4),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_LOOP_END(),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterCommonSpecialNCharge)
};

static u32 sNdsGMColScriptsScreenFlashDeadExplode[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x00),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(6, 0xFF, 0xFF, 0xFF, 0x6E),
    NDS_GM_COL_COMMAND_WAIT(6),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(30, 0xFF, 0xFF, 0xFF, 0x00),
    NDS_GM_COL_COMMAND_WAIT(30),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_END()
};

static u32 sNdsGMColScriptsScreenFlashDamageNormal[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x46),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(6, 0xFF, 0xFF, 0xFF, 0x00),
    NDS_GM_COL_COMMAND_WAIT(6),
    NDS_GM_COL_COMMAND_END()
};

static u32 sNdsGMColScriptsScreenFlashDamageFire[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0x8C, 0x78, 0x50),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(8, 0xFF, 0x8C, 0x78, 0x00),
    NDS_GM_COL_COMMAND_WAIT(8),
    NDS_GM_COL_COMMAND_END()
};

static u32 sNdsGMColScriptsScreenFlashDamageElectric[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0x8C, 0x8C, 0xFF, 0x50),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(8, 0x8C, 0x8C, 0xFF, 0x00),
    NDS_GM_COL_COMMAND_WAIT(8),
    NDS_GM_COL_COMMAND_END()
};

static u32 sNdsGMColScriptsScreenFlashDamageIce[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0x80, 0xFF, 0x8C),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(6, 0x00, 0x80, 0xFF, 0x00),
    NDS_GM_COL_COMMAND_WAIT(6),
    NDS_GM_COL_COMMAND_END()
};

/* BUGS.md "Missing fire burn effects" -- the root cause, and the first
 * divergence for that row.
 *
 * BattleShip does not draw the fighter fire burn from an effect maker. It draws
 * it from these COLOUR-ANIMATION scripts, which flash the fighter orange/red
 * AND issue nEFKindFlameLR / nEFKindFlameRandom themselves (gmcolscripts.c:134-
 * 210). ftCommonDamageCheckElementSetColAnim already asks for the right ids --
 * nGMColAnimFighterDamageFireStart = 12 through 15 -- but this table had no
 * entries for them, so their p_script was NULL and ftParamCheckSetColAnimID
 * rejected the request below. The flame commands therefore never executed,
 * which is why every Flame counter measured exactly zero: the request was being
 * killed one level UPSTREAM of the effect system, not falling through it.
 *
 * Transcribed exactly, including the asymmetric Fly loop counts (24 then 20).
 * Priorities are the source's: all four are 100/FALSE, the same priority as
 * FighterDamageCommon (gmcolscripts.c:1179-1182). */
static u32 sNdsGMColScriptsFighterDamageFireSub1[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xF0, 0x78, 0xAA),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xDC, 0x6E, 0x1E, 0x96),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_RETURN()
};

static u32 sNdsGMColScriptsFighterDamageFireSub2[] = {
    NDS_GM_COL_COMMAND_LOOP_BEGIN(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xB4, 0x64, 0x00, 0x64),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x3C, 0x00, 0x00, 0xD2),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_LOOP_END(),
    NDS_GM_COL_COMMAND_RETURN()
};

#define NDS_GM_COL_FIRE_BODY(lr_count, random_count) \
    NDS_GM_COL_COMMAND_LOOP_BEGIN(lr_count), \
    NDS_GM_COL_COMMAND_EFFECT(-1, nEFKindFlameLR, 0, 0, 0, 0, 0, 0, 0), \
    NDS_GM_COL_COMMAND_SUBROUTINE(sNdsGMColScriptsFighterDamageFireSub1), \
    NDS_GM_COL_COMMAND_LOOP_END(), \
    NDS_GM_COL_COMMAND_LOOP_BEGIN(random_count), \
    NDS_GM_COL_COMMAND_EFFECT(-1, nEFKindFlameRandom, 0, 0, 0, 0, 0, 0, 0), \
    NDS_GM_COL_COMMAND_SUBROUTINE(sNdsGMColScriptsFighterDamageFireSub2), \
    NDS_GM_COL_COMMAND_LOOP_END(), \
    NDS_GM_COL_COMMAND_END()

static u32 sNdsGMColScriptsFighterDamageFireWeak[] = {
    NDS_GM_COL_FIRE_BODY(4, 4)
};
static u32 sNdsGMColScriptsFighterDamageFireMid[] = {
    NDS_GM_COL_FIRE_BODY(8, 8)
};
static u32 sNdsGMColScriptsFighterDamageFireStrong[] = {
    NDS_GM_COL_FIRE_BODY(16, 16)
};
static u32 sNdsGMColScriptsFighterDamageFireFly[] = {
    NDS_GM_COL_FIRE_BODY(24, 20)
};

/* BattleShip gmcolscripts.c:1107-1115. LinkBomb's final 96-tic fuse uses this
 * exact looping yellow/red Color1 script. The shared DS table is intentionally
 * sparse, so Link is the first item client that requires this source row. */
static u32 sNdsGMColScriptsItemLinkBombCritical[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0x00, 0x8C),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(4, 0x80, 0x00, 0x00, 0x8C),
    NDS_GM_COL_COMMAND_WAIT(4),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x80, 0x00, 0x00, 0x8C),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(12, 0xFF, 0xFF, 0x00, 0x8C),
    NDS_GM_COL_COMMAND_WAIT(12),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsItemLinkBombCritical)
};

/* gmcolscripts.c:1021-1027: the Challenger Approaching silhouette, solid
 * black for as long as the scene runs (sc1PChallengerMakeFighter :228). It
 * had no row, so the setter refused id 80 and the challenger drew in its
 * own colours (owner r67: "missing the applicable silhouette"). */
static u32 sNdsGMColScriptsFighterChallenger[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0x00, 0x00, 0xFF),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterChallenger),
    NDS_GM_COL_COMMAND_END()
};

/* BattleShip gmcolscripts.c:720-779: Kirby's Stone. High/Mid/Low are the
 * three looping white pulses the stone's damage tiers select; Start/End are
 * the brown fade in and out around the transform. */
static u32 sNdsGMColScriptsFighterKirbySpecialLwHigh[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x28),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(8, 0xFF, 0xFF, 0xFF, 0x00),
    NDS_GM_COL_COMMAND_WAIT(8),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(8, 0xFF, 0xFF, 0xFF, 0x28),
    NDS_GM_COL_COMMAND_WAIT(8),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterKirbySpecialLwHigh)
};
static u32 sNdsGMColScriptsFighterKirbySpecialLwMid[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x3C),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(6, 0xFF, 0xFF, 0xFF, 0x0A),
    NDS_GM_COL_COMMAND_WAIT(6),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(6, 0xFF, 0xFF, 0xFF, 0x3C),
    NDS_GM_COL_COMMAND_WAIT(6),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterKirbySpecialLwMid)
};
static u32 sNdsGMColScriptsFighterKirbySpecialLwLow[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x5A),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(4, 0xFF, 0xFF, 0xFF, 0x0A),
    NDS_GM_COL_COMMAND_WAIT(4),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(4, 0xFF, 0xFF, 0xFF, 0x5A),
    NDS_GM_COL_COMMAND_WAIT(4),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterKirbySpecialLwLow)
};
static u32 sNdsGMColScriptsFighterKirbySpecialLwStart[] = {
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xB4, 0x80, 0x64, 0x0A),
    NDS_GM_COL_COMMAND_WAIT(5),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x50, 0x3C, 0x28, 0x14),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xB4, 0x80, 0x64, 0x28),
    NDS_GM_COL_COMMAND_WAIT(5),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x50, 0x3C, 0x28, 0x3C),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xB4, 0x80, 0x64, 0x50),
    NDS_GM_COL_COMMAND_WAIT(5),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x50, 0x3C, 0x28, 0x64),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xB4, 0x80, 0x64, 0x78),
    NDS_GM_COL_COMMAND_WAIT(5),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x50, 0x3C, 0x28, 0x8C),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xB4, 0x80, 0x64, 0xA0),
    NDS_GM_COL_COMMAND_WAIT(5),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x50, 0x3C, 0x28, 0xB4),
    NDS_GM_COL_COMMAND_END()
};
static u32 sNdsGMColScriptsFighterKirbySpecialLwEnd[] = {
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xB4, 0x80, 0x64, 0xB4),
    NDS_GM_COL_COMMAND_WAIT(5),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x50, 0x3C, 0x28, 0xA0),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xB4, 0x80, 0x64, 0x8C),
    NDS_GM_COL_COMMAND_WAIT(5),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x50, 0x3C, 0x28, 0x78),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xB4, 0x80, 0x64, 0x64),
    NDS_GM_COL_COMMAND_WAIT(5),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x50, 0x3C, 0x28, 0x50),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xB4, 0x80, 0x64, 0x3C),
    NDS_GM_COL_COMMAND_WAIT(5),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x50, 0x3C, 0x28, 0x28),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xB4, 0x80, 0x64, 0x14),
    NDS_GM_COL_COMMAND_WAIT(5),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x50, 0x3C, 0x28, 0x0A),
    NDS_GM_COL_COMMAND_END()
};

/* BattleShip gmcolscripts.c:905-962: Ness. PSI Magnet's cyan hold flicker
 * and absorb hit, PK Thunder's hold pulse with its small-shock sparks, the
 * PKT2 self-launch strobe, and the entry fade from black. */
static u32 sNdsGMColScriptsFighterNessSpecialLwHold[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x8C),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0xFF, 0xFF, 0x8C),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterNessSpecialLwHold)
};
static u32 sNdsGMColScriptsFighterNessSpecialLwHit[] = {
    NDS_GM_COL_COMMAND_SET_LIGHT(90, 0),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0xFF),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0xFF, 0xFF, 0xFF),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0xFF, 0xFF, 0x8C),
    NDS_GM_COL_COMMAND_WAIT(1),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_WAIT(4),
    NDS_GM_COL_COMMAND_END()
};
static u32 sNdsGMColScriptsFighterNessSpecialHiHold[] = {
    NDS_GM_COL_COMMAND_LOOP_BEGIN(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0xFF, 0xFF, 0xFF, 0x30),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0xFF, 0xFF, 0x30),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_LOOP_END(),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindShockSmall, 0, 0, 90, 0, 0, 0, 0),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterNessSpecialHiHold)
};
static u32 sNdsGMColScriptsFighterNessSpecialHiJibaku[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0x7F, 0x7F, 0x7F, 0x50),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindSparkleWhiteScale, 0, 0, 90, 0, 180, 180, 180),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindShockSmall, 0, 0, 90, 0, 0, 0, 0),
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0xFF, 0x00, 0x50),
    NDS_GM_COL_COMMAND_WAIT(2),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindSparkleWhiteScale, 0, 0, 90, 0, 180, 180, 180),
    NDS_GM_COL_COMMAND_EFFECT(0, nEFKindShockSmall, 0, 0, 90, 0, 0, 0, 0),
    NDS_GM_COL_COMMAND_GOTO(sNdsGMColScriptsFighterNessSpecialHiJibaku)
};
static u32 sNdsGMColScriptsFighterNessAppear[] = {
    NDS_GM_COL_COMMAND_SET_COLOR1(0x00, 0x00, 0x00, 0xFF),
    NDS_GM_COL_COMMAND_WAIT(30),
    NDS_GM_COL_COMMAND_BLEND_COLOR1(40, 0x00, 0x00, 0x00, 0x00),
    NDS_GM_COL_COMMAND_WAIT(40),
    NDS_GM_COL_COMMAND_CLEAR_COLOR_ALL(),
    NDS_GM_COL_COMMAND_END()
};

GMColDesc dGMColScriptsDescs[NDS_GM_COL_DESC_COUNT] = {
    [nGMColAnimFighterComPlayer] =
        { sNdsGMColScriptsFighterComPlayer, 1, FALSE },
    [nGMColAnimFighterHitStatusNormal] =
        { sNdsGMColScriptsFighterHitStatusNormal, 30, FALSE },
    [nGMColAnimFighterHitStatusIntangible] =
        { sNdsGMColScriptsFighterHitStatusIntangible, 30, FALSE },
    [nGMColAnimFighterHitStatusInvincible] =
        { sNdsGMColScriptsFighterHitStatusInvincible, 30, FALSE },
    [nGMColAnimFighterFallSpecial] =
        { sNdsGMColScriptsFighterFallSpecial, 60, TRUE },
    [nGMColAnimFighterFastFall] =
        { sNdsGMColScriptsFighterFastFall, 60, TRUE },
    [nGMColAnimFighterHeal] =
        { sNdsGMColScriptsFighterHeal, 15, FALSE },
    [nGMColAnimFighterNoDamage] =
        { sNdsGMColScriptsFighterNoDamage, 11, FALSE },
    [nGMColAnimFighterRebirth] =
        { sNdsGMColScriptsFighterRebirth, 10, TRUE },
    [nGMColAnimFighterDamageFireStart + 0] =
        { sNdsGMColScriptsFighterDamageFireWeak, 100, FALSE },
    [nGMColAnimFighterDamageFireStart + 1] =
        { sNdsGMColScriptsFighterDamageFireMid, 100, FALSE },
    [nGMColAnimFighterDamageFireStart + 2] =
        { sNdsGMColScriptsFighterDamageFireStrong, 100, FALSE },
    [nGMColAnimFighterDamageFireStart + 3] =
        { sNdsGMColScriptsFighterDamageFireFly, 100, FALSE },
    [nGMColAnimFighterDamageElectricCommonStart + 0] =
        { sNdsGMColScriptsFighterDamageElectricCommonWeak, 100, FALSE },
    [nGMColAnimFighterDamageElectricCommonStart + 1] =
        { sNdsGMColScriptsFighterDamageElectricCommonMid, 100, FALSE },
    [nGMColAnimFighterDamageElectricCommonStart + 2] =
        { sNdsGMColScriptsFighterDamageElectricCommonStrong, 100, FALSE },
    [nGMColAnimFighterDamageElectricCommonStart + 3] =
        { sNdsGMColScriptsFighterDamageElectricCommonFly, 100, FALSE },
    [nGMColAnimFighterDamageElectricSkeletonStart + 0] =
        { sNdsGMColScriptsFighterDamageElectricSkeletonWeak, 100, FALSE },
    [nGMColAnimFighterDamageElectricSkeletonStart + 1] =
        { sNdsGMColScriptsFighterDamageElectricSkeletonMid, 100, FALSE },
    [nGMColAnimFighterDamageElectricSkeletonStart + 2] =
        { sNdsGMColScriptsFighterDamageElectricSkeletonStrong, 100, FALSE },
    [nGMColAnimFighterDamageElectricSkeletonStart + 3] =
        { sNdsGMColScriptsFighterDamageElectricSkeletonFly, 100, FALSE },
    [nGMColAnimFighterDamageIceStart + 0] =
        { sNdsGMColScriptsFighterDamageIceWeak, 100, FALSE },
    [nGMColAnimFighterDamageIceStart + 1] =
        { sNdsGMColScriptsFighterDamageIceMid, 100, FALSE },
    [nGMColAnimFighterDamageIceStart + 2] =
        { sNdsGMColScriptsFighterDamageIceStrong, 100, FALSE },
    [nGMColAnimFighterDamageIceStart + 3] =
        { sNdsGMColScriptsFighterDamageIceFly, 100, FALSE },
    [nGMColAnimFighterCommonSpecialNCharge] =
        { sNdsGMColScriptsFighterCommonSpecialNCharge, 10, FALSE },
    [nGMColAnimFighterDamageCommon] =
        { sNdsGMColScriptsFighterDamageCommon, 100, FALSE },
    [nGMColAnimFighterShieldBreakFly] =
        { sNdsGMColScriptsFighterShieldBreakFly, 60, TRUE },
    [nGMColAnimFighterFuraFura] =
        { sNdsGMColScriptsFighterFuraFura, 60, TRUE },
    [nGMColAnimFighterFuraSleep] =
        { sNdsGMColScriptsFighterFuraSleep, 60, TRUE },
    [nGMColAnimFighterMarioSpecialN] =
        { sNdsGMColScriptsFighterMarioSpecialN, 60, TRUE },
    [nGMColAnimFighterMarioAppeal] =
        { sNdsGMColScriptsFighterMarioAppeal, 60, TRUE },
    [nGMColAnimFighterDonkeySpecialNLoop] =
        { sNdsGMColScriptsFighterDonkeySpecialNLoop, 60, TRUE },
    [nGMColAnimFighterDonkeySpecialNEnd] =
        { sNdsGMColScriptsFighterDonkeySpecialNEnd, 60, TRUE },
    [nGMColAnimFighterUnknown1] =
        { sNdsGMColScriptsFighterUnknown1, 60, TRUE },
    [nGMColAnimFighterFoxSpecialLw] =
        { sNdsGMColScriptsFighterFoxSpecialLw, 60, TRUE },
    [nGMColAnimFighterFoxSpecialHiStart] =
        { sNdsGMColScriptsFighterFoxSpecialHiStart, 60, TRUE },
    [nGMColAnimFighterFoxSpecialHi] =
        { sNdsGMColScriptsFighterFoxSpecialHi, 60, TRUE },
    [nGMColAnimFighterPikachuAttackS4] =
        { sNdsGMColScriptsFighterPikachuAttackS4, 60, TRUE },
    [nGMColAnimFighterPikachuSpecialHiStart] =
        { sNdsGMColScriptsFighterPikachuSpecialHiStart, 60, TRUE },
    [nGMColAnimFighterPikachuSpecialHi] =
        { sNdsGMColScriptsFighterPikachuSpecialHi, 60, TRUE },
    [nGMColAnimFighterPikachuSpecialN] =
        { sNdsGMColScriptsFighterPikachuSpecialN, 60, TRUE },
    [nGMColAnimFighterPikachuSpecialLwHit] =
        { sNdsGMColScriptsFighterPikachuSpecialLwHit, 60, TRUE },
    [nGMColAnimFighterPikachuSpecialLwEnd] =
        { sNdsGMColScriptsFighterPikachuSpecialLwEnd, 60, TRUE },
    [nGMColAnimFighterKirbySpeciaLwHigh] =
        { sNdsGMColScriptsFighterKirbySpecialLwHigh, 60, TRUE },
    [nGMColAnimFighterKirbySpeciaLwMid] =
        { sNdsGMColScriptsFighterKirbySpecialLwMid, 60, TRUE },
    [nGMColAnimFighterKirbySpeciaLwLow] =
        { sNdsGMColScriptsFighterKirbySpecialLwLow, 60, TRUE },
    [nGMColAnimFighterKirbySpecialLwStart] =
        { sNdsGMColScriptsFighterKirbySpecialLwStart, 60, TRUE },
    [nGMColAnimFighterKirbySpecialLwEnd] =
        { sNdsGMColScriptsFighterKirbySpecialLwEnd, 60, TRUE },
    [nGMColAnimFighterNessSpecialLwHold] =
        { sNdsGMColScriptsFighterNessSpecialLwHold, 60, TRUE },
    [nGMColAnimFighterNessSpecialLwHit] =
        { sNdsGMColScriptsFighterNessSpecialLwHit, 60, TRUE },
    [nGMColAnimFighterNessSpecialHiHold] =
        { sNdsGMColScriptsFighterNessSpecialHiHold, 60, TRUE },
    [nGMColAnimFighterNessSpecialHiJibaku] =
        { sNdsGMColScriptsFighterNessSpecialHiJibaku, 60, TRUE },
    [nGMColAnimFighterNessAppear] =
        { sNdsGMColScriptsFighterNessAppear, 60, TRUE },
    [nGMColAnimFighterHammer] =
        { sNdsGMColScriptsFighterHammer, 12, FALSE },
    [nGMColAnimFighterStar] =
        { sNdsGMColScriptsFighterStar, 100, FALSE },
    [nGMColAnimItemLinkBombCritical] =
        { sNdsGMColScriptsItemLinkBombCritical, 60, TRUE },
    [nGMColAnimFighterChallenger] =
        { sNdsGMColScriptsFighterChallenger, 60, TRUE },
    [nGMColAnimScreenFlashDeadExplode] =
        { sNdsGMColScriptsScreenFlashDeadExplode, 60, TRUE },
    [nGMColAnimScreenFlashDamageNormal] =
        { sNdsGMColScriptsScreenFlashDamageNormal, 60, TRUE },
    [nGMColAnimScreenFlashDamageFire] =
        { sNdsGMColScriptsScreenFlashDamageFire, 60, TRUE },
    [nGMColAnimScreenFlashDamageElectric] =
        { sNdsGMColScriptsScreenFlashDamageElectric, 60, TRUE },
    [nGMColAnimScreenFlashDamageIce] =
        { sNdsGMColScriptsScreenFlashDamageIce, 60, TRUE },
    NDS_P4_COLANIM_ROWS
};

_Static_assert(nGMColAnimScreenFlashDamageIce + 1 == nGMColAnimEnumCount,
               "BattleShip color-animation table count changed");
_Static_assert(ARRAY_COUNT(dGMColScriptsDescs) >= nGMColAnimEnumCount,
               "BattleShip color-animation table count changed");



static SYAudioCSPlayerCompat sNdsAudioCSPlayerCompat;
SYAudioCSPlayerCompat *gSYAudioCSPlayers[1] = {
    &sNdsAudioCSPlayerCompat
};

/* SCENE-START MUSIC WAITS FOR THE SCENE'S FIRST FRAME (owner 2026-10-02: from
 * the 1P stage clear to the next intro, "it starts playing sfx before the
 * rendered still intro is visible"). A scene's FuncStart starts its BGM
 * (sc1pintro.c:1981-1983, the battles, the stage clear); on the N64 its first
 * frame follows within a tic or two, but here the load and the first draw
 * can run 1-3 s under the held frame or the loading cover -- the intro's
 * first draw alone is ~80 VBlanks -- so the music played over black. A BGM
 * requested while the scene has not presented a complete frame
 * (ndsPlatformSceneLoading) is remembered and started by that frame
 * (ndsAudioReleaseDeferredBGM, at every cover release). Until then the
 * source's queries see it as playing, so a scene that waits on its music
 * does not run ahead; a volume set for it is applied when it starts; a stop
 * cancels it. Whatever was already playing continues meanwhile. */
extern u32 ndsPlatformSceneLoading(void);
static s32 sNdsDeferredBgmPlayer = -1;
static s32 sNdsDeferredBgmId;
static u32 sNdsDeferredBgmVolume;
static u32 sNdsDeferredBgmVolumeSet;
volatile u32 gNdsDeferredBgmCount;

static void ndsSyAudioPlayBGMNow(s32 player, s32 bgm_id)
{
    ndsAudioBgmPlay(player, bgm_id);
    sNdsAudioCSPlayerCompat.state =
        (ndsAudioBgmIsPlaying() != FALSE) ? AL_PLAYING : AL_STOPPED;
}

void ndsAudioReleaseDeferredBGM(void)
{
    const s32 player = sNdsDeferredBgmPlayer;

    if (player < 0)
    {
        return;
    }
    sNdsDeferredBgmPlayer = -1;
    ndsSyAudioPlayBGMNow(player, sNdsDeferredBgmId);
    if (sNdsDeferredBgmVolumeSet != 0u)
    {
        ndsAudioBgmSetVolume(player, sNdsDeferredBgmVolume);
    }
}

NDS_COLD void syAudioStopBGMAll(void)
{
    sNdsDeferredBgmPlayer = -1;
    ndsAudioBgmStopAll();
    sNdsAudioCSPlayerCompat.state = AL_STOPPED;
}

void syAudioPlayBGM(s32 player, s32 bgm_id)
{
    if ((player >= 0) && (ndsPlatformSceneLoading() != FALSE))
    {
        sNdsDeferredBgmPlayer = player;
        sNdsDeferredBgmId = bgm_id;
        sNdsDeferredBgmVolumeSet = 0u;
        NDS_DIAG(gNdsDeferredBgmCount++);
        sNdsAudioCSPlayerCompat.state = AL_PLAYING;
    }
    else
    {
        if (player == sNdsDeferredBgmPlayer)
        {
            sNdsDeferredBgmPlayer = -1;
        }
        ndsSyAudioPlayBGMNow(player, bgm_id);
    }
    gNdsSCVSBattleStageBGM = (u32)bgm_id;
    gNdsSCVSBattleCompatAudioMask |= 1u << 0;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_AUDIO;
}

void syAudioUpdateBGMState(void)
{
    sNdsAudioCSPlayerCompat.state =
        ((sNdsDeferredBgmPlayer >= 0) || (ndsAudioBgmIsPlaying() != FALSE)) ?
            AL_PLAYING : AL_STOPPED;
}

NDS_COLD void func_800266A0_272A0(void)
{
    ndsAudioFgmStopAll();
}


void func_80026738_27338(alSoundEffect *sfx)
{
    ndsAudioFgmStop(sfx);
}

static void *ndsPlayFGMAtPan(u16 fgm_id, u8 pan)
{
    alSoundEffect *sound_effect;

    sound_effect = ndsAudioFgmPlayAtPan(fgm_id, pan);
    gNdsSCVSBattleLastFGM = fgm_id;
    if (fgm_id == nSYAudioFGMPupupuWhispyWind)
    {
        NDS_DIAG(gNdsPupupuUpdateWindFGMCount++);
    }
    gNdsSCVSBattleCompatAudioMask |= 1u << 1;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_AUDIO;
    return sound_effect;
}

void *func_800269C0_275C0(u16 fgm_id)
{
    return ndsPlayFGMAtPan(fgm_id, 64u);
}

NDS_COLD s32 syAudioCheckBGMPlaying(s32 sngplayer)
{
    s32 is_playing = ((sNdsDeferredBgmPlayer >= 0) &&
                      (sngplayer == sNdsDeferredBgmPlayer)) ? TRUE :
        ndsAudioBgmCheckPlaying(sngplayer);
    gNdsSCVSBattleCompatAudioMask |= 1u << 2;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_AUDIO;
    return is_playing;
}

NDS_COLD void syAudioSetBGMVolume(s32 sngplayer, u32 vol)
{
    if ((sNdsDeferredBgmPlayer >= 0) && (sngplayer == sNdsDeferredBgmPlayer))
    {
        sNdsDeferredBgmVolume = vol;
        sNdsDeferredBgmVolumeSet = 1u;
    }
    else
    {
        ndsAudioBgmSetVolume(sngplayer, vol);
    }
    gNdsSCVSBattleLastAudioVolume = vol;
    gNdsSCVSBattleCompatAudioMask |= 1u << 3;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_AUDIO;
}

/* scmanager.c excludes these renderer/task wrappers for the NDS target. Keep
 * their original behavior here so imported scenes retain the same entry ABI. */
void scManagerFuncUpdate(SYTaskmanSetup *setup)
{
    syTaskmanStartTask(setup);
}

void scManagerFuncDraw(void)
{
    gcDrawAll();
}

void syAudioSetSettingsUpdated(void)
{
}

sb32 syAudioGetSettingsUpdated(void)
{
    return FALSE;
}

NDS_COLD void syAudioSetFXType(u8 type)
{
    (void)type;
}

NDS_COLD sb32 syAudioGetRestarting(void)
{
    return FALSE;
}


/* FTTEXTUREPART IS THREE BYTES, AND THE FILE HOLDING IT IS WORD-SWAPPED.
 *
 * A fighter's main file reaches the DS with every 32-bit word byte-reversed:
 * logical byte i of the loaded file sits at physical i ^ 3. The FTAttributes
 * fixups in reloc_backend_assets.c undo that for the fields they know; nothing
 * undid it for the texture-part container, a pair of { u8 joint_id;
 * u8 detail[2]; } (fttypes.h:165-175) that this file read as a C struct.
 * Measured in RAM at the CSS: Yoshi's container is 07 00 00 07 00 00 01 01,
 * so the struct read gave part 1 detail {0, 0} where the source has {1, 1},
 * and every part-1 write landed on part 0's MObj. Yoshi and Jigglypuff keep one
 * eye in each part on the same head joint, so one eye took every expression and
 * the other stayed on texture 0 -- the owner's "sometimes one eye is closed".
 * Link's container has the same shape; Mario's one-part container read its
 * joint id (0x0C) from a padding byte, so his face never changed at all.
 *
 * Read the three fields through the lane rather than rewriting the shared file:
 * the loaded file base is word aligned, so a byte's physical address is its
 * logical address XOR 3. A one-part container's part 1 still reads whatever
 * follows it, exactly as the source's does; the source never selects it. */
_Static_assert(sizeof(FTTexturePart) == 3u,
               "FTTexturePart must stay three packed bytes (fttypes.h:165)");

static s32 ndsFTTexturePartByte(const FTTexturePartContainer *container,
                                s32 part, u32 field)
{
    uintptr_t logical = (uintptr_t)container +
                        ((u32)part * (u32)sizeof(FTTexturePart)) + field;

    return (s32)*(const u8 *)(logical ^ 3u);
}

static s32 ndsFTTexturePartJointID(const FTTexturePartContainer *container,
                                   s32 part)
{
    return ndsFTTexturePartByte(container, part,
                                (u32)offsetof(FTTexturePart, joint_id));
}

static s32 ndsFTTexturePartDetail(const FTTexturePartContainer *container,
                                  s32 part, s32 detail_index)
{
    return ndsFTTexturePartByte(container, part,
                                (u32)offsetof(FTTexturePart, detail) +
                                    (u32)detail_index);
}

NDS_COLD void ftParamInitTexturePartAll(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTTexturePartStatus *texturepart_status;
    const FTTexturePartContainer *container;
    DObj *joint;
    MObj *mobj;
    s32 joint_id;
    s32 detail;
    s32 i;
    s32 j;

    if ((fp == NULL) || (fp->attr == NULL) ||
        (fp->attr->textureparts_container == NULL))
    {
        return;
    }
    container = fp->attr->textureparts_container;

    /* BattleShip ftparam.c:1070-1110. Re-applying a costume replaces each
     * joint's MObj chain, so any live texture-part override must be copied back
     * to the newly-created material at this detail level. */
    for (i = 0, texturepart_status = &fp->texturepart_status[0];
         i < (s32)ARRAY_COUNT(fp->texturepart_status);
         i++, texturepart_status++)
    {
        if (texturepart_status->texture_id_curr !=
            texturepart_status->texture_id_base)
        {
            joint_id = ndsFTTexturePartJointID(container, i);
            if (joint_id >= (s32)ARRAY_COUNT(fp->joints))
            {
                continue;
            }
            joint = fp->joints[joint_id];
            detail = ndsFTTexturePartDetail(
                container, i, fp->detail_curr - nFTPartsDetailStart);
            if (joint != NULL)
            {
                mobj = joint->mobj;
                for (j = 0; (mobj != NULL) && (j != detail); j++)
                {
                    mobj = mobj->next;
                }
                if (mobj != NULL)
                {
                    mobj->texture_id_curr = texturepart_status->texture_id_curr;
                }
            }
        }
    }
    fp->is_texturepart_modify = TRUE;
}

NDS_COLD void ftParamInitAllParts(GObj *fighter_gobj, s32 costume, s32 shade)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr;
    DObj *joint;
    GObj *parts_gobj;
    FTParts *parts;
    FTCommonPartContainer *commonparts_container;
    FTAccessPart *accesspart;
    FTModelPartStatus *modelpart_status;
    s32 detail_id;
    FTModelPart *modelpart;
    MObjSub **mobjsubs;
    AObjEvent32 **costume_matanim_joints;
    s32 i;

    if ((fp == NULL) || (fp->attr == NULL))
    {
        return;
    }
    attr = fp->attr;
    commonparts_container = attr->commonparts_container;
    accesspart = attr->accesspart;

    /* BattleShip ftparam.c:976-1065. Costume changes are model/material work,
     * not metadata: rebuild every current model part's MObj chain, maintain the
     * costume accessory, then refresh shade and texture-part overrides. The
     * former no-op made live Team Battle costume changes diverge immediately
     * from the source even when the selected costume number itself was right. */
    for (i = 0;
         i < ((s32)ARRAY_COUNT(fp->joints) - nFTPartsJointCommonStart);
         i++)
    {
        joint = fp->joints[i + nFTPartsJointCommonStart];
        if (joint == NULL)
        {
            continue;
        }

        gcRemoveMObjAll(joint);
        modelpart_status = &fp->modelpart_status[i];
        if (modelpart_status->modelpart_id_curr != -1)
        {
            if ((attr->modelparts_container != NULL) &&
                (attr->modelparts_container->modelparts_desc[i] != NULL))
            {
                modelpart = &attr->modelparts_container->modelparts_desc[i]
                                 ->modelparts[modelpart_status->modelpart_id_curr]
                                             [fp->detail_curr -
                                              nFTPartsDetailStart];
                lbCommonAddMObjForFighterPartsDObj(
                    joint, modelpart->mobjsubs,
                    modelpart->costume_matanim_joints,
                    modelpart->main_matanim_joints, costume);
            }
            else if (commonparts_container != NULL)
            {
                if ((fp->detail_curr == nFTPartsDetailHigh) ||
                    (commonparts_container->commonparts[1].dobjdesc[i].dl ==
                     NULL))
                {
                    detail_id = 0;
                }
                else
                {
                    detail_id = 1;
                }
                mobjsubs =
                    (commonparts_container->commonparts[detail_id].p_mobjsubs !=
                     NULL)
                        ? commonparts_container->commonparts[detail_id]
                              .p_mobjsubs[i]
                        : NULL;
                costume_matanim_joints =
                    (commonparts_container->commonparts[detail_id]
                         .p_costume_matanim_joints != NULL)
                        ? commonparts_container->commonparts[detail_id]
                              .p_costume_matanim_joints[i]
                        : NULL;
                lbCommonAddMObjForFighterPartsDObj(
                    joint, mobjsubs, costume_matanim_joints, NULL, costume);
            }
        }

        if ((accesspart != NULL) &&
            ((i + nFTPartsJointCommonStart) == accesspart->joint_id))
        {
            parts = ftGetParts(joint);
            if ((parts != NULL) && (parts->gobj != NULL))
            {
                gcEjectGObj(parts->gobj);
                parts->gobj = NULL;
            }
            if ((parts != NULL) && (costume != 0))
            {
                parts_gobj = gcMakeGObjSPAfter(
                    nGCCommonKindFighterParts, NULL,
                    nGCCommonLinkIDFighterParts, GOBJ_PRIORITY_DEFAULT);
                parts->gobj = parts_gobj;
                if (parts_gobj != NULL)
                {
                    DObj *parts_dobj =
                        gcAddDObjForGObj(parts_gobj, accesspart->dl);
                    lbCommonAddMObjForFighterPartsDObj(
                        parts_dobj, accesspart->mobjsubs,
                        accesspart->costume_matanim_joints, NULL, costume);
                }
            }
        }
    }

    fp->costume = costume;
    fp->shade = shade;
    fp->shade_color.r =
        (attr->shade_color[fp->shade - 1].r *
         attr->shade_color[fp->shade - 1].a) /
        0xFF;
    fp->shade_color.g =
        (attr->shade_color[fp->shade - 1].g *
         attr->shade_color[fp->shade - 1].a) /
        0xFF;
    fp->shade_color.b =
        (attr->shade_color[fp->shade - 1].b *
         attr->shade_color[fp->shade - 1].a) /
        0xFF;
    ftParamInitTexturePartAll(fighter_gobj);

    /* The source operation above replaces MObjs in place. The DS native
     * renderer caches immutable material conversion by MObj identity, and the
     * source allocator may immediately reuse the just-freed MObj address. Tell
     * that cache about the source lifetime boundary so a costume can never
     * inherit the previous costume's converted texture/material block. */
    ndsFighterRendererInvalidateMaterialCachesForSlot((u32)fp->player);
}

sb32 ftParamCheckSetColAnimID(GMColAnim *colanim, s32 colanim_id, s32 length)
{
    s32 i;

    if ((colanim == NULL) || (colanim_id < 0) ||
        (colanim_id >= NDS_GM_COL_DESC_COUNT) ||
        (dGMColScriptsDescs[colanim_id].p_script == NULL))
    {
        return FALSE;
    }
    if ((colanim->colanim_id >= 0) &&
        (colanim->colanim_id < NDS_GM_COL_DESC_COUNT) &&
        (dGMColScriptsDescs[colanim_id].priority <
         dGMColScriptsDescs[colanim->colanim_id].priority))
    {
        return FALSE;
    }
    colanim->colanim_id = colanim_id;
    colanim->length = length;
    colanim->cs[0].p_script = dGMColScriptsDescs[colanim_id].p_script;
    colanim->cs[0].color_event_timer = 0;
    colanim->cs[0].script_id = 0;
    for (i = 1; i < ARRAY_COUNT(colanim->cs); i++)
    {
        colanim->cs[i].p_script = NULL;
    }
    colanim->is_use_color1 = 0;
    colanim->is_use_light = 0;
    colanim->is_use_color2 = 0;
    colanim->skeleton_id = 0;
    return TRUE;
}

#if NDS_P2_DONKEY
volatile u32 gNdsDonkeyChargePresentationOverrideCount;
volatile s32 gNdsDonkeyChargePresentationRequestedID = -1;
volatile s32 gNdsDonkeyChargePresentationAppliedID = -1;
#endif

sb32 ftParamCheckSetFighterColAnimID(GObj *fighter_gobj, s32 colanim_id,
                                     s32 unused)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    /* Owner-approved presentation delta (BUGS.md, 2026-08-29): use the
     * source's own completed-neutral-special presentation while Donkey is
     * charging, instead of the source-authored yellow charging flash.
     *
     * BattleShip 212_DonkeyMainMotion.c does NOT use the Donkey-named ColAnim
     * here: both GiantPunch{Ground,Air}Loop scripts issue
     * nGMColAnimFighterFoxSpecialHi (48). The source asset contains exactly two
     * uses of ColAnim 48 for Donkey, both of them these Giant Punch loop
     * scripts; restricting by Donkey kind is therefore exact while Fox's real
     * SpecialHi remains source-identical. Only the presentation ID changes;
     * charge timing, levels, cancel/release behavior, hit data and motion
     * events stay BattleShip-authored. */
    if ((fp != NULL) &&
        ((fp->fkind == nFTKindDonkey) || (fp->fkind == nFTKindNDonkey) ||
         (fp->fkind == nFTKindGDonkey)) &&
        (colanim_id == nGMColAnimFighterFoxSpecialHi))
    {
#if NDS_P2_DONKEY
        NDS_DIAG(gNdsDonkeyChargePresentationOverrideCount++);
        gNdsDonkeyChargePresentationRequestedID = colanim_id;
#endif
        colanim_id = nGMColAnimFighterCommonSpecialNCharge;
#if NDS_P2_DONKEY
        gNdsDonkeyChargePresentationAppliedID = colanim_id;
#endif
    }
    if (fp != NULL)
    {
        colanim_id = ndsP4ColAnimRemap(fp, colanim_id);
    }

    ndsTask39EffectCensusRecord(
        NDS_TASK39_EFFECT_FT_PARAM_CHECK_SET_FIGHTER_COL_ANIM_I_D,
        NDS_TASK39_EFFECT_ORIGINAL);
    if ((fighter_gobj != NULL) && (ftGetStruct(fighter_gobj) != NULL))
    {
        return ftParamCheckSetColAnimID(&ftGetStruct(fighter_gobj)->colanim,
                                       colanim_id, unused);
    }
    return FALSE;
}

sb32 ftParamCheckSetSkeletonColAnimID(GObj *fighter_gobj, s32 damage_level)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    sb32 result = FALSE;

    /* BattleShip ftparam.c:1326-1330 maps each fighter kind to its electric
     * family. dFTParamSkeletonColAnimIDs[] is 0x14 for both Mario and Fox,
     * i.e. nGMColAnimFighterDamageElectricSkeletonStart, and those two own a
     * native electric body. Every other fighter runs the source's own
     * no-skeleton family (0x10, the Common scripts) so the flash and the
     * ShockSmall sparks are never dropped. OWED: native electric bodies for
     * the rest of the roster (0x14/0x18/0x1C); each costs ~7 KB of arena. */
    if ((fp != NULL) && (damage_level >= 0) && (damage_level < 4))
    {
        extern sb32 ndsFTManagerSkeletonReady(s32 fkind);

        /* The body image is resident only when the match has an electric
         * attacker; an unexpected source still gets the flash, never a
         * rejected fighter draw. A P4 content has its parent's kind (Wolf,
         * Peach and Sonic are Fox, Crash and Lanky Mario) but no electric
         * body of its own: it takes the common family, as the rest of the
         * cast does. */
        result = ftParamCheckSetFighterColAnimID(
            fighter_gobj,
            ((fp->nds_p4_content == 0u) &&
             (ndsFTManagerSkeletonReady(fp->fkind) != FALSE) ?
                 nGMColAnimFighterDamageElectricSkeletonStart :
                 nGMColAnimFighterDamageElectricCommonStart) + damage_level,
            0);
    }
    if (result != FALSE)
    {
        return TRUE;
    }
    return FALSE;
}

static void ndsFTParamSetHitStatusColAnim(GObj *fighter_gobj, s32 hitstatus)
{
    /* BattleShip ftparam.c:569-585. Hit-status setters are not state-only: the
     * source immediately selects the corresponding collision colour animation.
     * Keep this as one shared helper so the part-all and whole-fighter paths
     * cannot drift apart again. */
    switch (hitstatus)
    {
    case nGMHitStatusNormal:
        ftParamCheckSetFighterColAnimID(
            fighter_gobj, nGMColAnimFighterHitStatusNormal, 0);
        break;
    case nGMHitStatusInvincible:
        ftParamCheckSetFighterColAnimID(
            fighter_gobj, nGMColAnimFighterHitStatusInvincible, 0);
        break;
    case nGMHitStatusIntangible:
        ftParamCheckSetFighterColAnimID(
            fighter_gobj, nGMColAnimFighterHitStatusIntangible, 0);
        break;
    }
}

void ftPhysicsStopVelAll(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp != NULL)
    {
        fp->vel_air.x = 0.0F;
        fp->vel_air.y = 0.0F;
        fp->vel_air.z = 0.0F;
        fp->vel_ground.x = 0.0F;
        fp->vel_ground.y = 0.0F;
        fp->vel_ground.z = 0.0F;
        fp->vel_push.x = 0.0F;
        fp->vel_push.y = 0.0F;
        fp->vel_push.z = 0.0F;
        fp->physics.vel_air = fp->vel_air;
        fp->physics.vel_ground = fp->vel_ground;
        /* The KNOCKBACK velocity, and it is the whole of BUGS row 8. The source
         * zeroes it here (ftphysics.c:453-455) and this did not, so "stop all
         * velocity" stopped everything except the one that was actually moving
         * a KO'd fighter. ftCommonDeadUpStarSetStatus calls this and then drives
         * the fighter to camera_bound_top * 0.6 through physics.vel_air, so with
         * vel_damage_air still live the fighter kept its launch speed and the
         * correction was simply outvoted: measured, the star sparkle spawned at
         * y 79,222 against a target of 2,400, with physics.vel_air.y holding the
         * correct -33.36 the whole way up. The effect was never broken; it was
         * drawn nine stage-heights above the screen.
         *
         * vel_damage_air is summed with vel_air to make the effective velocity
         * (reloc_backend_cliff_ledge.c:7073), so leaving it out of a "stop all"
         * is silent everywhere else too -- every other caller happens to be a
         * state the knockback had already decayed in. */
        fp->physics.vel_damage_air.x = 0.0F;
        fp->physics.vel_damage_air.y = 0.0F;
        fp->physics.vel_damage_air.z = 0.0F;
        fp->physics.vel_damage_ground = 0.0F;
        fp->physics.vel_jostle_x = 0.0F;
        fp->physics.vel_jostle_z = 0.0F;
    }
}

void ftParamClearAttackCollAll(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 i;

    if (fp != NULL)
    {
        for (i = 0; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            fp->attack_colls[i].attack_state = nGMAttackStateOff;
        }
        fp->is_attack_active = FALSE;
    }
}

typedef enum NDSGMHitType
{
    nNDSGMHitTypeDamage = 0,
    nNDSGMHitTypeShield = 1,
    nNDSGMHitTypeAttack = 3
} NDSGMHitType;

extern void battleship_ftMainSetHitInteractStats(FTStruct *fp,
                                                 u32 attack_group_id,
                                                 GObj *victim_gobj,
                                                 s32 attack_type,
                                                 u32 victim_group_id,
                                                 sb32 ignore_damage_or_hit);

void ftMainSetHitInteractStats(FTStruct *fp, u32 attack_group_id,
                               GObj *victim_gobj, s32 attack_type,
                               u32 victim_group_id,
                               sb32 ignore_damage_or_hit)
{
    /* The source's own calls take the hook in battleship_ftmain.c; this
     * wrapper serves the other units (an item's hurtbox, itprocess.c). */
    ndsP4OnHitInteract(fp, attack_type);
    battleship_ftMainSetHitInteractStats(fp, attack_group_id, victim_gobj,
                                         attack_type, victim_group_id,
                                         ignore_damage_or_hit);
}

void ftParamClearAttackRecordID(FTStruct *fp, s32 attack_id)
{
    s32 i;

    if ((fp == NULL) || (attack_id < 0) ||
        ((u32)attack_id >= FTATTACKCOLL_NUM_MAX))
    {
        return;
    }
    for (i = 0; i < GMATTACKREC_NUM_MAX; i++)
    {
        GMAttackRecord *record =
            &fp->attack_colls[attack_id].attack_records[i];

        record->victim_gobj = NULL;
        record->victim_flags.is_interact_hurt = FALSE;
        record->victim_flags.is_interact_shield = FALSE;
        record->victim_flags.timer_rehit = 0;
        record->victim_flags.group_id = 7;
    }
}

void ftParamRefreshAttackCollID(GObj *fighter_gobj, s32 attack_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == NULL) || (attack_id < 0) ||
        ((u32)attack_id >= FTATTACKCOLL_NUM_MAX))
    {
        return;
    }
    fp->attack_colls[attack_id].attack_state = nGMAttackStateNew;
    fp->is_attack_active = TRUE;
    ftParamClearAttackRecordID(fp, attack_id);
    if (((gNdsFighterNaturalMovesetPhase == 9u) ||
         (gNdsFighterNaturalMovesetPhase == 10u)) &&
        (fp->status_id >= nFTCommonStatusAttackAirStart) &&
        (fp->status_id <= nFTCommonStatusAttackAirEnd))
    {
        NDS_DIAG(gNdsFighterNaturalMovesetAerialHitboxFrames++);
    }
}

s32 ftParamGetJointID(FTStruct *fp, s32 joint_id)
{
    if ((joint_id == -2) && (fp != NULL) && (fp->attr != NULL))
    {
        return fp->attr->joint_itemlight_id;
    }
    return joint_id;
}

void ftParamSetHitStatusPartAll(GObj *fighter_gobj, s32 hitstatus)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 i;

    if (fp != NULL)
    {
        for (i = 0; i < FTDAMAGECOLL_NUM_MAX; i++)
        {
            if (fp->damage_colls[i].hitstatus != nGMHitStatusNone)
            {
                fp->damage_colls[i].hitstatus = hitstatus;
            }
        }
        fp->is_hitstatus_nodamage =
            (hitstatus == nGMHitStatusNormal) ? FALSE : TRUE;
        ndsFTParamSetHitStatusColAnim(fighter_gobj, hitstatus);
    }
}

void ftParamResetFighterDamageCollsAll(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    FTDamageColl *damage_coll;
    FTDamageCollDesc *damage_coll_desc;
    s32 i;

    if ((fp == NULL) || (fp->attr == NULL))
    {
        return;
    }
    damage_coll = &fp->damage_colls[0];
    damage_coll_desc = &fp->attr->damage_coll_descs[0];
    for (i = 0;
         i < ((ARRAY_COUNT(fp->damage_colls) +
               ARRAY_COUNT(fp->attr->damage_coll_descs)) / 2);
         i++, damage_coll++, damage_coll_desc++)
    {
        if (damage_coll_desc->joint_id != -1)
        {
            damage_coll->joint_id = damage_coll_desc->joint_id;
            damage_coll->joint = fp->joints[damage_coll->joint_id];
            damage_coll->placement = damage_coll_desc->placement;
            damage_coll->is_grabbable = damage_coll_desc->is_grabbable;
            damage_coll->offset = damage_coll_desc->offset;
            damage_coll->size = damage_coll_desc->size;
            damage_coll->size.x *= 0.5F;
            damage_coll->size.y *= 0.5F;
            damage_coll->size.z *= 0.5F;
        }
    }
    fp->is_damage_coll_modify = FALSE;
}

void ftParamModifyDamageCollID(GObj *fighter_gobj, s32 joint_id,
                               Vec3f *offset, Vec3f *size)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    s32 i;

    if ((fp == NULL) || (offset == NULL) || (size == NULL))
    {
        return;
    }
    for (i = 0; i < ARRAY_COUNT(fp->damage_colls); i++)
    {
        FTDamageColl *damage_coll = &fp->damage_colls[i];

        if (joint_id == damage_coll->joint_id)
        {
            damage_coll->offset = *offset;
            damage_coll->size = *size;
            damage_coll->size.x *= 0.5F;
            damage_coll->size.y *= 0.5F;
            damage_coll->size.z *= 0.5F;
            fp->is_damage_coll_modify = TRUE;
            return;
        }
    }
}

void ftParamSetHitStatusPartID(GObj *fighter_gobj, s32 joint_id,
                               s32 hitstatus)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 i;

    if (fp != NULL)
    {
        for (i = 0; i < FTDAMAGECOLL_NUM_MAX; i++)
        {
            if ((fp->damage_colls[i].hitstatus != nGMHitStatusNone) &&
                (fp->damage_colls[i].joint_id == joint_id))
            {
                fp->damage_colls[i].hitstatus = hitstatus;

                if (hitstatus != nGMHitStatusNormal)
                {
                    fp->is_hitstatus_nodamage = TRUE;
                }
                break;
            }
        }
    }
}

/* BattleShip ftparam.c:748-818 / 831-910 resolve a model-part id into the live
 * DObj, not merely into FTStruct bookkeeping. Keep that source operation in one
 * helper so Set/Reset/Detail all rebuild the same DL, MObj chain and FTParts
 * flags. The DS renderer observes the resulting source DObj state. */
static sb32 ndsFTParamApplyModelPartCurrent(GObj *fighter_gobj, FTStruct *fp,
                                            s32 slot)
{
    FTAttributes *attr;
    FTCommonPartContainer *commonparts_container;
    FTModelPartStatus *status;
    FTModelPart *modelpart;
    FTParts *parts;
    DObj *joint;
    MObjSub **mobjsubs;
    AObjEvent32 **costume_matanim_joints;
    s32 detail_index;
    s32 detail_id;
    s32 joint_id;

    if ((fighter_gobj == NULL) || (fp == NULL) || (fp->attr == NULL) ||
        (slot < 0) || ((u32)slot >= ARRAY_COUNT(fp->modelpart_status)))
    {
        return FALSE;
    }
    joint_id = slot + nFTPartsJointCommonStart;
    if ((u32)joint_id >= ARRAY_COUNT(fp->joints))
    {
        return FALSE;
    }
    joint = fp->joints[joint_id];
    if (joint == NULL)
    {
        return FALSE;
    }

    attr = fp->attr;
    status = &fp->modelpart_status[slot];
    commonparts_container = attr->commonparts_container;
    parts = ftGetParts(joint);
    gcRemoveMObjAll(joint);

    if (status->modelpart_id_curr == -1)
    {
        joint->dl = NULL;
        return TRUE;
    }

    detail_index = (s32)fp->detail_curr - nFTPartsDetailStart;
    if ((detail_index < 0) || (detail_index > 1))
    {
        return FALSE;
    }
    if ((attr->modelparts_container != NULL) &&
        (attr->modelparts_container->modelparts_desc[slot] != NULL))
    {
        modelpart = &attr->modelparts_container->modelparts_desc[slot]
                         ->modelparts[status->modelpart_id_curr][detail_index];
        joint->dl = modelpart->dl;
        lbCommonAddMObjForFighterPartsDObj(
            joint, modelpart->mobjsubs, modelpart->costume_matanim_joints,
            modelpart->main_matanim_joints, fp->costume);
        if (parts != NULL)
        {
            parts->flags = modelpart->flags;
        }
        return TRUE;
    }

    if (commonparts_container == NULL)
    {
        joint->dl = NULL;
        return TRUE;
    }
    if ((fp->detail_curr == nFTPartsDetailHigh) ||
        (commonparts_container->commonparts[1].dobjdesc[slot].dl == NULL))
    {
        detail_id = 0;
    }
    else
    {
        detail_id = 1;
    }
    joint->dl = commonparts_container->commonparts[detail_id].dobjdesc[slot].dl;
    mobjsubs = (commonparts_container->commonparts[detail_id].p_mobjsubs != NULL)
                   ? commonparts_container->commonparts[detail_id].p_mobjsubs[slot]
                   : NULL;
    costume_matanim_joints =
        (commonparts_container->commonparts[detail_id].p_costume_matanim_joints !=
         NULL)
            ? commonparts_container->commonparts[detail_id]
                  .p_costume_matanim_joints[slot]
            : NULL;
    lbCommonAddMObjForFighterPartsDObj(joint, mobjsubs, costume_matanim_joints,
                                        NULL, fp->costume);
    if (parts != NULL)
    {
        parts->flags = commonparts_container->commonparts[detail_id].flags;
    }
    return TRUE;
}

static void ndsFTParamInvalidateModelPartRenderer(GObj *fighter_gobj,
                                                   FTStruct *fp)
{
    ndsFighterRendererInvalidateDObjStateCaches(fighter_gobj);
    if (fp != NULL)
    {
        ndsFighterRendererInvalidateMaterialCachesForSlot((u32)fp->player);
    }
}

void ftParamResetModelPartAll(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    u32 i;
    sb32 changed = FALSE;

    if (fp == NULL)
    {
        return;
    }
    for (i = 0u; i < ARRAY_COUNT(fp->modelpart_status); i++)
    {
        FTModelPartStatus *status = &fp->modelpart_status[i];

        if (fp->joints[i + nFTPartsJointCommonStart] == NULL)
        {
            continue;
        }
        if (status->modelpart_id_curr != status->modelpart_id_base)
        {
            status->modelpart_id_curr = status->modelpart_id_base;
            (void)ndsFTParamApplyModelPartCurrent(fighter_gobj, fp, (s32)i);
            NDS_DIAG(gNdsFighterModelPartResetCount++);
            changed = TRUE;
        }
    }
    /* Source clears this after all requested parts have been restored. */
    fp->is_modelpart_modify = FALSE;
    if (fp->nds_p4_kirby_copy != 0u)
    {
        ndsP4KirbyRefreshHat(fp);
    }
    if (changed != FALSE)
    {
        ndsFTParamInvalidateModelPartRenderer(fighter_gobj, fp);
    }
}

/* P2-2p8 Phase 1 slice 3 (renderer_fighter_lean.c): the lean fighter draw
 * proves its material identity through its writers, and these two shims are
 * the in-status texture-id writer. Counted only when the id actually moves. */
extern void ndsFtrLeanNoteTexturePart(u32 slot);

void ftParamSetTexturePartID(GObj *fighter_gobj, s32 texturepart_id,
                             s32 texture_id)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    FTTexturePartContainer *container;
    DObj *joint;
    MObj *mobj;
    s32 joint_id;
    s32 detail;
    s32 i;

    if ((fp == NULL) || (fp->attr == NULL) ||
        (texturepart_id < 0) ||
        ((u32)texturepart_id >= ARRAY_COUNT(fp->texturepart_status)))
    {
        return;
    }
    container = fp->attr->textureparts_container;
    if (container == NULL)
    {
        /* Same phantom record as below, from the other direction: with no
         * container the source reaches no MObj and writes nothing, so neither
         * does this. The guard itself stays -- the source would fault here. */
        return;
    }

    /* Through the byte lane: see ndsFTTexturePartByte. */
    joint_id = ndsFTTexturePartJointID(container, texturepart_id);
    if ((joint_id >= (s32)ARRAY_COUNT(fp->joints)) ||
        (fp->detail_curr < nFTPartsDetailStart))
    {
        return;
    }

    detail = ndsFTTexturePartDetail(container, texturepart_id,
                                    fp->detail_curr - nFTPartsDetailStart);
    joint = fp->joints[joint_id];
    mobj = (joint != NULL) ? joint->mobj : NULL;
    for (i = 0; (mobj != NULL) && (i < detail); i++)
    {
        mobj = mobj->next;
    }
    if (mobj != NULL)
    {
        /* THE MIRROR BELONGS INSIDE THIS ARM.
         *
         * BattleShip ftparam.c:1127-1141 writes the status mirror and the
         * modify flag only where it found an MObj: a chain shorter than
         * `detail` records NOTHING, so neither does this (r41). r41 blamed
         * the owner's one closed eye on the mirror; the trace that finally
         * watched both eye MObjs showed the mirror right and the part-1 MObj
         * never written, because `detail` itself was read off the wrong byte
         * lane -- ndsFTTexturePartByte. */
        if (mobj->texture_id_curr != (u16)texture_id)
        {
            ndsFtrLeanNoteTexturePart((u32)fp->nds_slot);
        }
        mobj->texture_id_curr = texture_id;
        fp->texturepart_status[texturepart_id].texture_id_curr = texture_id;
        fp->is_texturepart_modify = TRUE;
    }
}

/* BUGS.md #7: this reset only rewound the FTStruct mirror and left the MObj
 * showing whatever the last ftParamSetTexturePartID wrote, so the first damage
 * face a fighter took became permanent -- ftMainSetStatus calls this on every
 * status change without FTSTATUS_PRESERVE_TEXTUREPART, and the model never
 * heard about it. The source writes the base id back through the joint's MObj
 * chain (ftparam.c:1147-1189); do the same, with the container/detail guards
 * ftParamSetTexturePartID already uses. */
void ftParamResetTexturePartAll(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    FTTexturePartContainer *container;
    s32 i;

    if (fp == NULL)
    {
        return;
    }
    container = (fp->attr != NULL) ? fp->attr->textureparts_container : NULL;
    for (i = 0; i < ARRAY_COUNT(fp->texturepart_status); i++)
    {
        FTTexturePartStatus *status = &fp->texturepart_status[i];
        DObj *joint;
        MObj *mobj;
        s32 joint_id;
        s32 detail;
        s32 j;

        if (status->texture_id_curr == status->texture_id_base)
        {
            continue;
        }
        status->texture_id_curr = status->texture_id_base;
        if ((container == NULL) || (fp->detail_curr < nFTPartsDetailStart))
        {
            continue;
        }
        /* Through the byte lane: see ndsFTTexturePartByte. */
        joint_id = ndsFTTexturePartJointID(container, i);
        if (joint_id >= (s32)ARRAY_COUNT(fp->joints))
        {
            continue;
        }
        detail = ndsFTTexturePartDetail(container, i,
                                        fp->detail_curr - nFTPartsDetailStart);
        joint = fp->joints[joint_id];
        mobj = (joint != NULL) ? joint->mobj : NULL;
        for (j = 0; (mobj != NULL) && (j < detail); j++)
        {
            mobj = mobj->next;
        }
        if (mobj != NULL)
        {
            if (mobj->texture_id_curr != (u16)status->texture_id_curr)
            {
                ndsFtrLeanNoteTexturePart((u32)fp->nds_slot);
            }
            mobj->texture_id_curr = status->texture_id_curr;
        }
    }
    fp->is_texturepart_modify = FALSE;
}

void ftParamHideModelPartAll(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    u32 i;
    sb32 changed = FALSE;

    if (fp == NULL)
    {
        return;
    }
    /* BattleShip ftparam.c:913-939: hide means exactly -1 + no MObjs + no DL. */
    for (i = 0u; i < ARRAY_COUNT(fp->modelpart_status); i++)
    {
        DObj *joint = fp->joints[i + nFTPartsJointCommonStart];

        if ((joint == NULL) || (fp->modelpart_status[i].modelpart_id_curr == -1))
        {
            continue;
        }
        fp->modelpart_status[i].modelpart_id_curr = -1;
        gcRemoveMObjAll(joint);
        joint->dl = NULL;
        changed = TRUE;
    }
    fp->is_modelpart_modify = TRUE;
    if (changed != FALSE)
    {
        ndsFTParamInvalidateModelPartRenderer(fighter_gobj, fp);
    }
}

void ftParamProcStopEffect(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    ndsEFManagerStopAttachedEffects(fighter_gobj);
    if (fp != NULL)
    {
        fp->is_effect_attach = FALSE;
    }
}

/* Task 37: 58 bytes, 24,980 calls per census window, and a measured 7.12
 * cycles per instruction -- a recursive walk down the joint tree whose every
 * hot PC is a pointer-chasing load. The loads keep their cost wherever this
 * lives; what ITCM buys is the fetch of the loop around them. */
/* 2026-09-27: out of ITCM (the flat walk serves every call in the
 * four-CPU census; this recursive fallback executed zero times). */
NDS_COLD static void ndsFTParamsInvalidateFighterParts(
    DObj *joint, sb32 reset_mode)
{
    DObj *child;
    FTParts *parts;

    if (joint == NULL)
    {
        return;
    }
    parts = joint->user_data.p;
    if (parts != NULL)
    {
        if ((reset_mode != FALSE) && (parts->transform_update_mode == 1))
        {
            parts->transform_update_mode = 0;
        }
        parts->unk_dobjtrans_word = 0;
    }
    for (child = joint->child; child != NULL; child = child->sib_next)
    {
        ndsFTParamsInvalidateFighterParts(child, reset_mode);
    }
}

/* ...and c106 measured what ITCM could not fix. 15,815 ticks/frame over
 * 159,748 joint visits is 86 cycles a joint for two word writes, because every
 * step is its own cache line: joint->user_data.p, transform_update_mode,
 * unk_dobjtrans_word, joint->child, child->sib_next. The walk is the cost, not
 * the writes -- and the topology it walks is stable during ordinary animation.
 * A fighter's common DObj tree is built when the fighter is built, but
 * ftMainSetStatus is the important exception: enabled-joint animation flags can
 * add/eject/re-parent hidden DObjs for actions such as Catch/Throw. The writer
 * seam explicitly invalidates this flattened walk below when that happens.
 *
 * So flatten it: preorder the subtree once into an array of FTParts pointers
 * and replay that array. Same joints, same order, three of the five loads gone
 * and the recursion with them.
 *
 * Steady-state validity is (root, gNdsTaskmanHeapGeneration). The generation is bumped at
 * the two primitives that rewind the taskman heap cursor
 * (battleship_sys_malloc.c), which is the only way a live fighter tree can be
 * freed and rebuilt, so a stale list cannot survive a scene rewind or a
 * START-rematch; ftMainSetStatus separately invalidates same-generation topology
 * edits. Anything the table cannot hold -- a hash collision, or a subtree
 * deeper than the array -- falls back to the recursive walk above. Fail-closed:
 * never a partial invalidate.
 *
 * P2-2: this is a per-FIGHTER-root steady-state cache, so its capacity follows
 * BattleShip's player bound. Keeping the P1-era two entries is behavior-safe
 * but makes P3/P4 continuously evict a live root and re-walk its entire DObj
 * tree on transform invalidation, an artificial four-player cost the source
 * does not impose. GMCOMMON_PLAYERS_MAX is four and power-of-two, preserving the
 * cheap pointer hash below while allowing one resident entry per live player. */
/* P2-2p8: the paragraph above is wrong about what keys this cache, and the
 * per-PC profile is how that surfaced. `ndsFTParamsInvalidateSubtree` is called
 * with a JOINT, not with a fighter root -- `ftParamsUpdateFighterPartsTransform`
 * passes the DObj whose transform changed -- so the number of distinct keys per
 * frame is the number of invalidated joints, not the number of players. A
 * four-entry direct-mapped table therefore cannot hold a steady state, and the
 * profile shows it does not: the three hottest instructions in the function are
 * the flatten walk's pointer chase (`ldr r3,[r6,#16]` at CPI 42.0,
 * `ldr r6,[r6,#8]` at 30.7, `ldr r0,[r3,#0]` at 48.6, 184.4 executions a frame
 * between them), which only run on a MISS, while the clear's store is 4.4.
 * The walk is 59% of the bucket and the clear is 14%.
 *
 * NDS_FTPARTS_FLAT_SLOTS is a build knob so the miss rate and the fix can be
 * measured on the same instrument. The default preserves the shipped four. */
_Static_assert((NDS_FTPARTS_FLAT_SLOTS & (NDS_FTPARTS_FLAT_SLOTS - 1u)) == 0u,
               "flat fighter-parts cache hash requires a power-of-two slot count");

/* The hit/miss/conflict census that measured this table is REMOVED, not
 * disabled: five volatile counters plus their compares overflowed `.itcm` by 40
 * bytes at the shipped configuration, and the census had already answered its
 * question. Findings in
 * artifacts/performance/2026-09-16_p2-2p8-n0503-flat-cache/. */
/* Capacity per slot, and the dominant term in this table's size: each entry is
 * one pointer, so a slot costs 4*MAX + 12 bytes. 96 was chosen with four slots,
 * where the whole table was 1,584 bytes and the bound did not matter. It does
 * now. A subtree larger than MAX is NOT a correctness problem -- the lookup
 * returns NULL and the caller falls back to the unflattened child loop -- but
 * it silently reinstates the slow path, so gNdsFtPartsFlatOverflows counts it
 * and gNdsFtPartsFlatCountMax reports the real high-water subtree size. */
/* THE CAPACITY BOUND IS NOT A LEVER, AND THIS LANE IS NOW THREE-FOR-THREE
 * NEGATIVE. Note the effective knob is the Makefile: it emits
 * NDS_FTPARTS_FLAT_MAX into the force-included nds_build_config.h, so the
 * #ifndef default below never applies to a ROM. Editing only the default
 * changes nothing, and a measurement taken that way reads identical to its
 * control -- which is exactly what happened before the knob was found.
 *
 *   widen   16 slots   SRC -15,040 but STG +51,520, WORK-H +33,984 -- the
 *                      table crosses 80% of the 4 KB dcache (N05.03)
 *   relocate DTCM      WORK-H -10,176, mechanism confirmed, but under the
 *                      14,080 floor and it costs 1,584 of 1,992 usable DTCM
 *                      bytes (2026-09-16_p2-2p8-dtcm-falsifier)
 *   shrink   MAX 48    WORK-H P50 +1,216 / P95 -6,016, and arena size, alloc
 *                      failures and heap low-water all byte-identical to
 *                      control (2026-09-17)
 *
 * The shrink says something the other two do not: removing 768 bytes of dcache
 * footprint changed nothing, while removing the whole table to DTCM moved
 * -10,176. What matters here is whether the table is in the dcache at all, not
 * how big it is -- so there is no partial credit to collect on size. */

typedef struct NDSFtPartsFlatWalk
{
    const DObj *root;
    u32 heap_generation;
    u32 count;
    /* gNdsFtPartsLatchWrites when this subtree's latch words, and its update
     * modes, were last cleared (valid bits below). */
    u32 clean_words_at;
    u32 clean_modes_at;
    u32 clean_valid;
    FTParts *parts[NDS_FTPARTS_FLAT_MAX];
} NDSFtPartsFlatWalk;

/* P2-2p8 (2026-10-05). Two latch clears a fighter a tick (after its
 * animation and after its physics move it) walked every FTParts of the
 * subtree, a line fill each -- and 97% of them found nothing set: on the gate,
 * 4,177 latched parts in 535,802 visited, 20,799 of 21,437 clears with none
 * (`gNdsFtPartsLatchWrites` census, build-gate-dirty). Every writer of a
 * latch -- the gmcollision.c entry points that reach the float latch walk
 * (src/import/battleship_gmcollision.c), the fixed ring, the stub joint --
 * bumps this counter first, so a subtree cleared since its last value holds
 * no latch and the clear is skipped: the same state, without the walk. Only
 * where every such writer is wrapped (NDS_P2_JOINT_RESIDENT with
 * NDS_P2_HURTBOX_REJECT). Same-ROM A/B word gNdsFtPartsCleanSkip. */
volatile u32 gNdsFtPartsLatchWrites;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
volatile u32 gNdsLabFtPartsCleanSkipViolations __attribute__((used));
volatile u32 gNdsLabFtPartsCleanVerify __attribute__((used, section(".data"))) = 0u;
#endif
#if NDS_P2_JOINT_RESIDENT
volatile u32 gNdsFtPartsCleanSkip __attribute__((used, section(".data"))) = 1u;
#else
volatile u32 gNdsFtPartsCleanSkip __attribute__((used, section(".data"))) = 0u;
#endif

/* P2-2p8 stall class: this table was MEASURED in DTCM and is deliberately not
 * left there. Moving it read WORK-H -10,176, STG -7,936 and SRC -4,992 with
 * every divergence witness identical, so the direction is right -- DTCM is
 * addressable zero-wait memory, so unlike the 16-slot main-RAM table that lost
 * N05.03 it can neither evict the stage nor be evicted. But -10,176 is below
 * the 14,080-tick cross-build significance floor, and the placement costs 1,584
 * of the 1,992 usable DTCM bytes this build actually has.
 *
 * 1,992, not the 5,704 that `dtcm LENGTH = 0x3e80` implies: linker/nds_hot_text.ld
 * asserts __dtcm_bss_end <= 0x02ff3000 against the boot stack's measured
 * low-water mark, so DTCM data has a 12,288-byte ceiling. Widening to 16 slots
 * needs 1,680 bytes more than remain, so the lane cannot be finished here.
 * Spending 80% of the remaining budget on an unproven 2% would block every
 * future DTCM candidate. Evidence:
 * artifacts/performance/2026-09-16_p2-2p8-dtcm-falsifier/. */
static NDSFtPartsFlatWalk sNdsFtPartsFlat[NDS_FTPARTS_FLAT_SLOTS];

/* ftMainSetStatus can materialize/eject/re-parent hidden fighter DObjs without
 * changing either the fighter root pointer or the taskman heap generation. The
 * flattened transform-invalidation walk is keyed on exactly those two values,
 * so a list built before Catch/Throw would otherwise remain "valid" while
 * omitting the newly enabled item-heavy/capture joint (Mario 28 / Fox 30).
 *
 * Invalidate only cached roots owned by this fighter. Status changes are rare
 * compared with the per-frame replay this protects, and the next transform
 * invalidation rebuilds the list from the authoritative live topology. */
void ndsFTParamsInvalidateFlatWalkCacheForFighter(GObj *fighter_gobj)
{
    u32 i;

    if (fighter_gobj == NULL)
    {
        return;
    }
    for (i = 0u; i < NDS_FTPARTS_FLAT_SLOTS; i++)
    {
        NDSFtPartsFlatWalk *flat = &sNdsFtPartsFlat[i];

        if (flat->root == NULL)
        {
            continue;
        }
        if (flat->heap_generation != gNdsTaskmanHeapGeneration)
        {
            flat->root = NULL;
            continue;
        }
        if (flat->root->parent_gobj == fighter_gobj)
        {
            flat->root = NULL;
        }
    }
}

/* Preorder every joint BELOW `root`. The root is excluded because the two
 * callers treat it differently -- ...TransformAll resets the root's update mode
 * and only clears the word on the descendants. Returns capacity + 1 on
 * overflow so the caller falls back rather than invalidating part of a tree.
 *
 * The walk carries its own continuation stack and reads only `child` and
 * `sib_next`, which is exactly the field set the recursion above uses. An
 * earlier draft climbed `joint->parent` to find the next sibling and was wrong
 * for this tree: fighter_model.c sets the top joint's parent to
 * DOBJ_PARENT_NULL, a non-NULL sentinel, so a climb that ran past `root` would
 * dereference it. Depth is bounded for the same reason `count` is -- overflow
 * falls back to the recursion rather than truncating. */
#define NDS_FTPARTS_FLAT_DEPTH 24u

static u32 ndsFTParamsFlattenDescendants(
    DObj *root, FTParts **out, u32 capacity)
{
    DObj *pending[NDS_FTPARTS_FLAT_DEPTH];
    u32 depth = 0u;
    DObj *joint;
    u32 count = 0u;

    if ((root == NULL) || (root->child == NULL))
    {
        return 0u;
    }
    joint = root->child;
    for (;;)
    {
        FTParts *parts = joint->user_data.p;

        if (parts != NULL)
        {
            if (count >= capacity)
            {
                return capacity + 1u;
            }
            out[count] = parts;
            count++;
        }
        if (joint->child != NULL)
        {
            if (joint->sib_next != NULL)
            {
                if (depth >= NDS_FTPARTS_FLAT_DEPTH)
                {
                    return capacity + 1u;
                }
                pending[depth] = joint->sib_next;
                depth++;
            }
            joint = joint->child;
            continue;
        }
        if (joint->sib_next != NULL)
        {
            joint = joint->sib_next;
            continue;
        }
        if (depth == 0u)
        {
            return count;
        }
        depth--;
        joint = pending[depth];
    }
}

/* Searched, not direct-mapped (2026-09-27). The steady keys are the four
 * fighters' TopN joints (ftmain.c:942, once per fighter per tick), and a
 * `(ptr >> 4) & 3` slot let equally-strided DObjs share one slot: the flatten
 * walk re-ran on most calls (~12K ticks a frame of the profile). Four compares
 * find any resident key; a miss takes a stale slot, else the next in turn. */
static u32 sNdsFtPartsFlatNext;

static const NDSFtPartsFlatWalk *ndsFTParamsFlatWalkFor(DObj *root)
{
    NDSFtPartsFlatWalk *flat = NULL;
    u32 count;
    u32 i;

    for (i = 0u; i < NDS_FTPARTS_FLAT_SLOTS; i++)
    {
        NDSFtPartsFlatWalk *slot = &sNdsFtPartsFlat[i];

        if (slot->heap_generation != gNdsTaskmanHeapGeneration)
        {
            if (flat == NULL)
            {
                flat = slot;
            }
            continue;
        }
        if (slot->root == root)
        {
            return slot;
        }
        if ((slot->root == NULL) && (flat == NULL))
        {
            flat = slot;
        }
    }
    if (flat == NULL)
    {
        flat = &sNdsFtPartsFlat[sNdsFtPartsFlatNext++ &
                                (NDS_FTPARTS_FLAT_SLOTS - 1u)];
    }
    count = ndsFTParamsFlattenDescendants(
        root, flat->parts, NDS_FTPARTS_FLAT_MAX);
    if (count > NDS_FTPARTS_FLAT_MAX)
    {
        flat->root = NULL;
        return NULL;
    }
    flat->root = root;
    flat->heap_generation = gNdsTaskmanHeapGeneration;
    flat->count = count;
    flat->clean_valid = 0u;
    return flat;
}

static void NDS_TASK37_ITCM_CODE ndsFTParamsInvalidateRootParts(
    DObj *root, sb32 reset_mode)
{
    FTParts *parts = (root != NULL) ? root->user_data.p : NULL;

    if (parts != NULL)
    {
        if ((reset_mode != FALSE) && (parts->transform_update_mode == 1))
        {
            parts->transform_update_mode = 0;
        }
        parts->unk_dobjtrans_word = 0;
    }
}

static void NDS_TASK37_ITCM_CODE ndsFTParamsInvalidateFlatParts(
    const NDSFtPartsFlatWalk *flat, sb32 reset_mode)
{
    u32 i;

    if (reset_mode != FALSE)
    {
        for (i = 0u; i < flat->count; i++)
        {
            FTParts *parts = flat->parts[i];

            if (parts->transform_update_mode == 1)
            {
                parts->transform_update_mode = 0;
            }
            parts->unk_dobjtrans_word = 0;
        }
        return;
    }
    for (i = 0u; i < flat->count; i++)
    {
        flat->parts[i]->unk_dobjtrans_word = 0;
    }
}

static void NDS_R2_ITCM_PACK2_CODE
ndsFTParamsInvalidateSubtree(DObj *root, sb32 reset_mode)
{
    const NDSFtPartsFlatWalk *flat;
    DObj *child;

    flat = ndsFTParamsFlatWalkFor(root);
    if (flat != NULL)
    {
        NDSFtPartsFlatWalk *mut = (NDSFtPartsFlatWalk *)(uintptr_t)flat;
        const u32 writes = gNdsFtPartsLatchWrites;

        if ((gNdsFtPartsCleanSkip != 0u) && ((flat->clean_valid & 1u) != 0u) &&
            (flat->clean_words_at == writes) &&
            ((reset_mode == FALSE) ||
             (((flat->clean_valid & 2u) != 0u) &&
              (flat->clean_modes_at == writes))))
        {
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
            /* LAB (poke gNdsLabFtPartsCleanVerify=1): a skipped subtree must
             * hold no latch. Off by default: the check reads every part the
             * skip exists not to read. */
            if (gNdsLabFtPartsCleanVerify != 0u)
            {
                u32 i;

                for (i = 0u; i < flat->count; i++)
                {
                    if ((flat->parts[i]->unk_dobjtrans_word != 0) ||
                        ((reset_mode != FALSE) &&
                         (flat->parts[i]->transform_update_mode == 1)))
                    {
                        gNdsLabFtPartsCleanSkipViolations++;
                    }
                }
            }
#endif
            return;
        }
        ndsFTParamsInvalidateFlatParts(flat, reset_mode);
        mut->clean_words_at = writes;
        mut->clean_valid |= 1u;
        if (reset_mode != FALSE)
        {
            mut->clean_modes_at = writes;
            mut->clean_valid |= 2u;
        }
        return;
    }
    for (child = root->child; child != NULL; child = child->sib_next)
    {
        ndsFTParamsInvalidateFighterParts(child, reset_mode);
    }
}

/* src/port/nds_p2_hurtbox_reject.c: its cached fixed worlds live exactly as
 * long as the FTParts latches these functions clear. */
extern volatile u32 gNdsP2HurtboxLatchEpoch;
#define NDS_P2_HURTBOX_LATCH_EPOCH_BUMP() (gNdsP2HurtboxLatchEpoch++)

/* ITCM (2026-09-27 census: ~0.6K ticks/frame non-memory stall at 88 B). */
void __attribute__((noinline, optimize("Os"))) __attribute__((section(".itcm")))
ftParamsUpdateFighterPartsTransform(DObj *joint)
{
    GObj *fighter_gobj;
    FTStruct *fp;
    u32 joint_id;

    NDS_P2_HURTBOX_LATCH_EPOCH_BUMP();
    if (joint == NULL)
    {
        return;
    }
    fighter_gobj = joint->parent_gobj;
    if ((fighter_gobj != NULL) &&
        (ndsFtPoseBodyChangedThisTick(fighter_gobj) == FALSE))
    {
        /* The compact pose hold leaves body LOCAL transforms unchanged, but
         * TopN/TransN/XRotN/YRotN still evaluate and therefore invalidate every
         * descendant WORLD transform. Keep the body-local matrices the source
         * collision path already materialised; reset the four changed locals
         * directly, then run the existing flat world-cache clear without its
         * per-body mode load/branch/write.
         *
         * Only while the pose engine plays the motion. An event32
         * (is_anim_joint) motion -- every Appear entry, Master Hand's whole
         * intro, the shield pose -- is played by the generic player on every
         * tick and never runs the engine, so the engine's body_evaluated is
         * whatever its last figatree tick left. When that was a held tick, the
         * body's transform_update_mode 1 latches (ftparam.c:2300 clears them
         * every tick) survived the whole motion: the renderer and the
         * collision path drew and tested the motion's first frame until it
         * ended -- Master Hand's fingers never moved before GO and his hand
         * never turned side-on (owner playtest, 2026-10-07). */
        fp = ftGetStruct(fighter_gobj);
        if ((fp != NULL) && (!fp->anim_desc.flags.is_anim_joint))
        {
            for (joint_id = 0u;
                 joint_id < (u32)nFTPartsJointCommonStart;
                 joint_id++)
            {
                ndsFTParamsInvalidateRootParts(fp->joints[joint_id], TRUE);
            }
            ndsFTParamsInvalidateSubtree(joint, FALSE);
            return;
        }
    }
    ndsFTParamsInvalidateRootParts(joint, TRUE);
    ndsFTParamsInvalidateSubtree(joint, TRUE);
}

void ftParamsUpdateFighterPartsTransformAll(DObj *joint)
{
    NDS_P2_HURTBOX_LATCH_EPOCH_BUMP();
    if (joint != NULL)
    {
        ndsFTParamsInvalidateRootParts(joint, TRUE);
        ndsFTParamsInvalidateSubtree(joint, FALSE);
    }
}

extern void gcParseDObjAnimJoint(DObj *dobj);
extern void gcPlayDObjAnimJoint(DObj *dobj);
extern void gcParseMObjMatAnimJoint(MObj *mobj);
extern void gcPlayMObjMatAnim(MObj *mobj);
extern void ndsGcParseMObjMatAnimJointNow(MObj *mobj);
extern void ndsGcPlayMObjMatAnimNow(MObj *mobj);
extern volatile u32 gNdsMObjTickMul;
extern void gcAddMObjMatAnimJoint(MObj *mobj, AObjEvent32 *matanim_joint,
                                  f32 anim_frame);
void lbCommonPlayTranslateScaledDObjAnim(DObj *dobj, Vec3f *scale);
void battleship_ftAnimParseDObjFigatree(DObj *root_dobj);
void battleship_ftAnimEndSetWait(GObj *fighter_gobj);
void battleship_ftAnimEndSetFall(GObj *fighter_gobj);
sb32 battleship_ftAnimEndCheckSetStatus(GObj *fighter_gobj,
                                        void (*proc_status)(GObj*));

/* R2-07 cycle 109. The port parser replaces the decomp one on the fighter path.
 *
 * This shim is the interposition point, and it is the ONLY one available: the
 * `#define` in `src/import/battleship_ftanim.c` renames the decomp parser's
 * definition and its internal call sites together, so no macro can redirect
 * just the calls. `ftParamUpdateAnimKeys` (below in this file) calls the
 * unrenamed name, which is this function, so selecting here reaches every
 * fighter joint without touching `decomp/`.
 *
 * Bit-exact except where the body says otherwise: the s16 conversion is proven
 * over all 65,536 x 8 inputs by `scripts/check_ftanim_target_exact.py`, the
 * reciprocal table is compile-time-folded and therefore bit-identical to the
 * runtime divide, and the one divide that would have changed rounding was left
 * a divide on purpose. */
void ndsR2FtAnimParseDObjFigatree(DObj *root_dobj);

void ftAnimParseDObjFigatree(DObj *root_dobj)
{
    ndsR2FtAnimParseDObjFigatree(root_dobj);
}

static s32 ndsFTStructJointLoopLimit(const FTStruct *fp)
{
    u32 limit;

    if ((fp == NULL) || (fp->nds_magic != NDS_FTSTRUCT_MAGIC) ||
        (fp->nds_common_joint_count == 0u))
    {
        return nFTPartsJointNumMax;
    }

    limit = nFTPartsJointCommonStart + fp->nds_common_joint_count;
    if (limit > nFTPartsJointNumMax)
    {
        limit = nFTPartsJointNumMax;
    }
    return (s32)limit;
}

/* Slice 33. Route bit 32 selects the caller-side idle-joint skip below. The
 * route word is the same one the parser and player arms use; this TU only reads
 * it, and only once per joint on the arm being measured. With the route
 * compiled out (every published ROM) `NDS_R2_ANIM_CUT_ON` folds to 1 and the
 * pre-cut arm dead-codes away -- the skip is not optional, it is the shipped
 * path, because it is provably equivalent rather than an approximation. */
#define NDS_R2_ANIM_CUT_ON(bit) (1)

/* Joints skipped whole. The parser's own `gNdsR2FtAnimParseCalls` stops seeing
 * these, so calls + skips is the figure that stays comparable across the cut --
 * without this the parser would appear to lose a third of its callers. */
volatile u32 gNdsR2FtAnimNullSkips;

void NDS_R2_ITCM_PACK2_CODE ftParamUpdateAnimKeys(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    DObj **p_joint;
    DObj *joint;
    FTParts *parts;
    f32 anim_wait_bak;
    s32 joint_limit;
    s32 i;
    u32 mobj_mul;

    if (fp == NULL)
    {
        return;
    }
    p_joint = &fp->joints[nFTPartsJointTopN];
    joint_limit = ndsFTStructJointLoopLimit(fp);
    mobj_mul = gNdsMObjTickMul;

    if (fp->motion_id != -2)
    {
        Vec3f *translate_scales =
            ((fp->is_have_translate_scale != FALSE) && (fp->attr != NULL))
                ? fp->attr->translate_scales
                : NULL;
        u32 pose_mask_lo = 0u;
        u32 pose_mask_hi = 0u;
        /* P2-2p6: a figatree-bound hierarchy animates through the pose engine
         * (parse + play over compact tracks, body joints at 30 Hz under
         * NDS_FT_POSE_HOLD). Ownership is PER JOINT, not per fighter:
         * lbCommonAddFighterPartsFigatree attaches by the TopN hierarchy walk,
         * while BattleShip ftParamUpdateAnimKeys (ftparam.c:380/406) plays the
         * complete indexed fp->joints[] table. Samus makes that distinction
         * gameplay-visible: Catch's grapple joint 36 is outside the 23-DObj
         * attach hierarchy but remains a live indexed joint whose existing
         * animation must be played by the generic source path. The MObj
         * material animations below remain source-owned for every joint. The
         * event32 (`is_anim_joint`) statuses keep the generic path wholesale. */
        const sb32 pose_owned =
            ((NDS_FT_POSE != 0) && (!fp->anim_desc.flags.is_anim_joint)) ?
                ndsFtPoseUpdate(fighter_gobj, fp, translate_scales,
                                &pose_mask_lo, &pose_mask_hi) : FALSE;

        for (i = 0; i < joint_limit; i++, p_joint++)
        {
            MObj *mobj;

            joint = *p_joint;
            if (joint == NULL)
            {
                if (translate_scales != NULL)
                {
                    translate_scales++;
                }
                continue;
            }
            if ((pose_owned != FALSE) &&
                (((i < 32) && ((pose_mask_lo & (1u << (u32)i)) != 0u)) ||
                 ((i >= 32) && (i < 64) &&
                  ((pose_mask_hi & (1u << ((u32)i - 32u))) != 0u))))
            {
                if (translate_scales != NULL)
                {
                    translate_scales++;
                }
                /* A batch's earlier tick plays no material (gNdsMObjTickMul
                 * 0): the joint's DObj is not even read. */
                for (mobj = (mobj_mul != 0u) ? joint->mobj : NULL;
                     mobj != NULL; mobj = mobj->next)
                {
                    gcParseMObjMatAnimJoint(mobj);
                    gcPlayMObjMatAnim(mobj);
                }
                continue;
            }

            /* Slice 33 -- ask once instead of calling twice to find out.
             *
             * Every function reachable from the two calls below wraps its
             * ENTIRE body in `anim_wait != AOBJ_ANIM_NULL`: both parse arms,
             * both play arms, five bodies across the port and the reference.
             * That is asserted by `scripts/check_anim_null_guard.py`, not read
             * off the page -- a counter or a cache poke added above one of
             * those guards later would make this skip drop real work, and the
             * symptom (one joint's bookkeeping stops on the frames it is idle)
             * is not something a screenshot would show.
             *
             * 31.5% of joints are idle on any given frame and each was paying
             * two full calls to discover it: 45 Thumb instructions and 40 stack
             * word accesses, measured off the shipped disassembly, against
             * three instructions to ask here.
             *
             * Two things deliberately stay outside the skip. The MObj loop
             * below runs regardless -- an MObj carries its own `anim_wait` and
             * animates while its joint is idle. And `translate_scales` advances
             * on both paths: it indexes the joint array, so a pointer that
             * stopped advancing on idle joints would mis-scale every later
             * joint in the fighter. */
            if (!NDS_R2_ANIM_CUT_ON(32u) ||
                NDS_FCMP_NE_C(joint->anim_wait, AOBJ_ANIM_NULL))
            {
                if (fp->anim_desc.flags.is_anim_joint)
                {
                    gcParseDObjAnimJoint(joint);
                }
                else
                {
                    ftAnimParseDObjFigatree(joint);
                }

                if (translate_scales != NULL)
                {
                    lbCommonPlayTranslateScaledDObjAnim(joint,
                                                        translate_scales);
                }
                else
                {
                    gcPlayDObjAnimJoint(joint);
                }
            }
            else
            {
                NDS_DIAG(gNdsR2FtAnimNullSkips++);
            }
            if (translate_scales != NULL)
            {
                translate_scales++;
            }

            for (mobj = (mobj_mul != 0u) ? joint->mobj : NULL; mobj != NULL;
                 mobj = mobj->next)
            {
                gcParseMObjMatAnimJoint(mobj);
                gcPlayMObjMatAnim(mobj);
            }
        }
    }
    else
    {
        Vec3f *translate_scales =
            ((fp->is_have_translate_scale != FALSE) && (fp->attr != NULL))
                ? fp->attr->translate_scales
                : NULL;

        /* P2-2p6: re-apply compact tracks for the hierarchy, then preserve
         * BattleShip's indexed-joint loop for anything outside that bind. */
        const sb32 pose_reapplied =
            ((NDS_FT_POSE != 0) && (!fp->anim_desc.flags.is_anim_joint)) ?
                ndsFtPoseReapply(fighter_gobj, fp, translate_scales) : FALSE;
        for (i = 0; i < joint_limit; i++, p_joint++)
        {
            joint = *p_joint;
            if (joint == NULL)
            {
                if (translate_scales != NULL)
                {
                    translate_scales++;
                }
                continue;
            }
            if ((pose_reapplied != FALSE) &&
                (ndsFtPoseOwnsJoint(fighter_gobj, (u32)i) != FALSE))
            {
                if (translate_scales != NULL)
                {
                    translate_scales++;
                }
                continue;
            }
            parts = ftGetParts(joint);
            if ((parts == NULL) || (parts->is_have_anim == FALSE))
            {
                if (translate_scales != NULL)
                {
                    translate_scales++;
                }
                continue;
            }

            anim_wait_bak = joint->anim_wait;
            joint->anim_wait = AOBJ_ANIM_END;
            if (translate_scales != NULL)
            {
                lbCommonPlayTranslateScaledDObjAnim(joint, translate_scales);
                translate_scales++;
            }
            else
            {
                gcPlayDObjAnimJoint(joint);
            }
            joint->anim_wait = anim_wait_bak;
        }
    }
}

NDS_COLD void ftCommonShieldBreakFlyCommonSetStatus(GObj *fighter_gobj)
{
    /* BattleShip ftcommonshieldbreakfly.c owns this state transition.  The
     * previous DS approximation nulled ProcUpdate, substituted the generic
     * damage-fall map callback, used fast-fall physics, and zeroed the launch
     * velocity; all four change the shield-break state machine.  Keep only the
     * port's defensive NULL guard and delegate the behavior to the source. */
    if (ftGetStruct(fighter_gobj) == NULL)
    {
        return;
    }

    ndsBaseFTCommonShieldBreakFlyCommonSetStatus(fighter_gobj);
}

NDS_COLD void ftCommonShieldBreakFlyReflectorSetStatus(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj) == NULL)
    {
        return;
    }

    ndsBaseFTCommonShieldBreakFlyReflectorSetStatus(fighter_gobj);
}


__attribute__((weak)) GObj *efManagerYoshiShieldMakeEffect(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return NULL;
}
