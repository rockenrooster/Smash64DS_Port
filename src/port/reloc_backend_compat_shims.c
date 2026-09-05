/* NDS_NATURAL_COMBAT_ROUTED_EXTERNS */
#include "nds_scene_harness_config.h"
#include <nds/nds_freeze_diagnostics.h>
#include <nds/nds_effects.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_task39_effect_census.h>
#include <nds/nds_task37_itcm.h>
#include <nds/nds_ftanim_track.h>
#include <nds/nds_ft_pose.h>
#include <nds/nds_native_stage_blob.h>
#include <sys/vector.h>

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
void ndsFighterRendererInvalidateMaterialCaches(void);
void ndsFighterRendererInvalidateMaterialCachesForSlot(u32 slot);
void ndsFighterRendererInvalidateDObjStateCaches(GObj *fighter_gobj);
#endif

/* Shield anim-joint install engagement + the lab dispatch audit; legends in
 * include/nds/nds_startup.h beside the declarations. */
extern volatile u32 gNdsShieldAnimJointInstallCalls;
extern volatile u32 gNdsShieldAnimJointAttachCount;
extern volatile u32 gNdsShieldAnimJointNullCount;
#if NDS_ANIM_JOINT_AUDIT
extern volatile u32 gNdsAnimJointFlagFrames;
extern volatile u32 gNdsAnimJointIdleSkip32Count;
extern volatile u32 gNdsAnimJointDispatch32Count;
extern volatile u32 gNdsAnimJointDispatchFigatreeCount;
extern volatile u32 gNdsAnimJointDispatchMisalignCount;
extern volatile u32 gNdsAnimJointDispatchFigatreeScript;
extern s32 ndsRelocPointerIsFighterAObj16(const void *ptr);
#endif

static sb32 ndsMPReadMapObj(s32 index, u16 *kind, s16 *x, s16 *y);
#if NDS_P2_SAMUS_TUMBLE_TOUR
static void ndsSamusTumbleTourPrepareDamageFallMap(FTStruct *fp);
#endif

static sb32 ndsBattlePlayableRuntimeEnabled(void)
{
#if NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE && \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE)
    return TRUE;
#else
    return FALSE;
#endif
}

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
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
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
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
#endif
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
#endif

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

#if !NDS_IMPORT_BATTLESHIP_VS_RESULTS
static FTOpeningDesc sNdsDefaultOpeningDesc = { 0xFFFFFFFF, NULL };

FTOpeningDesc D_ovl1_80390BE8[] = { { 0x00010000, NULL } };

FTOpeningDesc *D_ovl1_80390D20[] = {
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc,
    &sNdsDefaultOpeningDesc
};
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

#if NDS_TASK39_FX_FLASH
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
#endif

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

GMColDesc dGMColScriptsDescs[nGMColAnimEnumCount] = {
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
#if NDS_TASK39_FX_FLASH
    [nGMColAnimFighterDamageCommon] =
        { sNdsGMColScriptsFighterDamageCommon, 100, FALSE },
#endif
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
    [nGMColAnimScreenFlashDeadExplode] =
        { sNdsGMColScriptsScreenFlashDeadExplode, 60, TRUE },
    [nGMColAnimScreenFlashDamageNormal] =
        { sNdsGMColScriptsScreenFlashDamageNormal, 60, TRUE },
    [nGMColAnimScreenFlashDamageFire] =
        { sNdsGMColScriptsScreenFlashDamageFire, 60, TRUE },
    [nGMColAnimScreenFlashDamageElectric] =
        { sNdsGMColScriptsScreenFlashDamageElectric, 60, TRUE },
    [nGMColAnimScreenFlashDamageIce] =
        { sNdsGMColScriptsScreenFlashDamageIce, 60, TRUE }
};

_Static_assert(ARRAY_COUNT(dGMColScriptsDescs) == nGMColAnimEnumCount,
               "BattleShip color-animation table count changed");

static FTParts sNdsFTManagerPartsAllocPool[64];
static FTParts *sNdsFTManagerPartsAllocFree;
static sb32 sNdsFTManagerPartsAllocInit;

#if !NDS_IMPORT_BATTLESHIP_FTMANAGER
static void ndsFTManagerInitPartsAllocPool(void)
{
    s32 i;

    if (sNdsFTManagerPartsAllocInit != FALSE)
    {
        return;
    }
    for (i = 0; i < (ARRAY_COUNT(sNdsFTManagerPartsAllocPool) - 1); i++)
    {
        sNdsFTManagerPartsAllocPool[i].next =
            &sNdsFTManagerPartsAllocPool[i + 1];
    }
    sNdsFTManagerPartsAllocPool[ARRAY_COUNT(sNdsFTManagerPartsAllocPool) - 1]
        .next = NULL;
    sNdsFTManagerPartsAllocFree = &sNdsFTManagerPartsAllocPool[0];
    sNdsFTManagerPartsAllocInit = TRUE;
}

FTParts *ftManagerGetNextPartsAlloc(void)
{
    FTParts *parts;

    ndsFTManagerInitPartsAllocPool();
    parts = sNdsFTManagerPartsAllocFree;
    if (parts == NULL)
    {
        return NULL;
    }
    sNdsFTManagerPartsAllocFree = parts->next;
    bzero(parts, sizeof(*parts));
    return parts;
}

void ftManagerSetPrevPartsAlloc(FTParts *parts)
{
    if (parts == NULL)
    {
        return;
    }
    ndsFTManagerInitPartsAllocPool();
    parts->next = sNdsFTManagerPartsAllocFree;
    sNdsFTManagerPartsAllocFree = parts;
}
#endif

static SYAudioCSPlayerCompat sNdsAudioCSPlayerCompat;
SYAudioCSPlayerCompat *gSYAudioCSPlayers[1] = {
    &sNdsAudioCSPlayerCompat
};

void syAudioStopBGMAll(void)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
    ndsAudioBgmStopAll();
#endif
    sNdsAudioCSPlayerCompat.state = AL_STOPPED;
}

void syAudioPlayBGM(s32 player, s32 bgm_id)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
    ndsAudioBgmPlay(player, bgm_id);
    sNdsAudioCSPlayerCompat.state =
        (ndsAudioBgmIsPlaying() != FALSE) ? AL_PLAYING : AL_STOPPED;
#else
    (void)player;
    sNdsAudioCSPlayerCompat.state = AL_STOPPED;
#endif
    gNdsSCVSBattleStageBGM = (u32)bgm_id;
    gNdsSCVSBattleCompatAudioMask |= 1u << 0;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_AUDIO;
}

void syAudioUpdateBGMState(void)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
    sNdsAudioCSPlayerCompat.state =
        (ndsAudioBgmIsPlaying() != FALSE) ? AL_PLAYING : AL_STOPPED;
#else
    sNdsAudioCSPlayerCompat.state = AL_STOPPED;
#endif
}

void func_800266A0_272A0(void)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_FGM
    ndsAudioFgmStopAll();
#endif
}

#if !NDS_IMPORT_BATTLESHIP_AUDIO_FGM
static alSoundEffect sNdsStubSoundEffect;
#endif

void func_80026738_27338(alSoundEffect *sfx)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_FGM
    ndsAudioFgmStop(sfx);
#else
    if (sfx != NULL)
    {
        sfx->sfx_id = 0;
    }
#endif
}

static void *ndsPlayFGMAtPan(u16 fgm_id, u8 pan)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_FGM
    alSoundEffect *sound_effect;

    sound_effect = ndsAudioFgmPlayAtPan(fgm_id, pan);
#else
    sNdsStubSoundEffect.sfx_id = fgm_id;
    sNdsStubSoundEffect.balance = pan;
#endif
    gNdsSCVSBattleLastFGM = fgm_id;
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDownBounceSFXCount++;
        gNdsStageMPCliffWaitDamageLoopDownBounceFGM = fgm_id;
    }
    if (fgm_id == nSYAudioFGMPupupuWhispyWind)
    {
        gNdsPupupuUpdateWindFGMCount++;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunGuardOnActive != FALSE) &&
        (fgm_id == nSYAudioFGMGuardOn))
    {
        gNdsFighterDashRunGuardFGMCount++;
        gNdsFighterDashRunGuardLastFGM = fgm_id;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageVoiceActive != FALSE))
    {
        sNdsFighterDashRunDamageVoiceCount++;
        sNdsFighterDashRunDamageVoiceLastFGM = fgm_id;
    }
    gNdsSCVSBattleCompatAudioMask |= 1u << 1;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_AUDIO;
#if NDS_IMPORT_BATTLESHIP_AUDIO_FGM
    return sound_effect;
#else
    return &sNdsStubSoundEffect;
#endif
}

void *func_800269C0_275C0(u16 fgm_id)
{
    return ndsPlayFGMAtPan(fgm_id, 64u);
}

s32 syAudioCheckBGMPlaying(s32 sngplayer)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
    s32 is_playing = ndsAudioBgmCheckPlaying(sngplayer);
#else
    (void)sngplayer;
#endif
    gNdsSCVSBattleCompatAudioMask |= 1u << 2;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_AUDIO;
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
    return is_playing;
#else
    return FALSE;
#endif
}

void syAudioSetBGMVolume(s32 sngplayer, u32 vol)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
    ndsAudioBgmSetVolume(sngplayer, vol);
#else
    (void)sngplayer;
#endif
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

void syAudioSetFXType(u8 type)
{
    (void)type;
}

sb32 syAudioGetRestarting(void)
{
    return FALSE;
}

#if !NDS_IMPORT_BATTLESHIP_FTMANAGER
void ftManagerSetupFileSize(void)
{
}

/* THE FIGATREE HEAP IS SIZED FROM THE ASSETS, NOT FROM A CONSTANT.
 *
 * Owner, 2026-08-23 and again 2026-08-24: "the Mario intro green tube ... only
 * the rim renders" and then "the intro animation for mario is broken".
 *
 * One fighter animation is loaded at a time, into a single heap whose size the
 * source computes in `ftManagerAllocFighter` (ft/ftmanager.c:79-206) as the
 * LARGEST animation file across every fighter it allocates -- it walks each
 * kind's motion descriptors and takes the max of `lbRelocGetFileSize`. This
 * port's shim used a flat `0x1000`, so every figatree over 4 KB failed to load:
 * the pointer resolve in `lbCommonAddFighterPartsFigatree` then handed
 * `ndsFtPoseBindEntry` a NULL script, the joint bound with
 * `anim_wait = AOBJ_ANIM_NULL`, and nothing ever published the GObj clock.
 *
 * The symptom was character-specific in a way that hid the cause: Wait and the
 * other short motions fit 4 KB and animate, so fighters looked fine, while
 * Mario's entry (motion 197, `nFTMarioMotionAppearR`) did not fit and left
 * `fighter_gobj->anim_frame` at its bind value. `ftCommonAppearProcUpdate` ends
 * Appear on `anim_frame <= 0.0F`, so both fighters left their entry on the tick
 * they began it -- measured, Mario `status_id=222 motion_id=197
 * anim_frame=0x80000000` and Fox `status_id=223 motion_id=198` identically --
 * and stood in Wait while their own entry effect played on beside them. The
 * pipe was never the defect; its animation is byte-faithful to the asset.
 *
 * This is the source's own computation, restricted to the kinds this build
 * actually carries (`dFTManagerDataFiles` entries are NULL for the rest) and
 * floored at the previous constant so a build whose tables are not yet
 * populated behaves exactly as it did before rather than allocating nothing. */
#define NDS_FTMANAGER_FIGATREE_HEAP_FLOOR 0x1000u

static size_t ndsFTManagerLargestFigatree(const FTMotionDescArray *array,
                                          s32 count)
{
    size_t largest = 0u;
    s32 i;

    if ((array == NULL) || (count <= 0))
    {
        return 0u;
    }
    for (i = 0; i < count; i++)
    {
        const FTMotionDesc *motion_desc = &array->motion_desc[i];
        size_t size;

        if (motion_desc->anim_file_id == 0u)
        {
            continue;
        }
        /* Shield poses live inside the fighter's own shieldpose file and are
         * addressed by offset, never loaded into this heap (ftmain.c:4617). */
        if (motion_desc->anim_desc.flags.is_use_shieldpose != FALSE)
        {
            continue;
        }
        size = lbRelocGetFileSize((const void *)motion_desc->anim_file_id);
        if (size > largest)
        {
            largest = size;
        }
    }
    return largest;
}

/* One fighter's worst animation, which is all a slot holding that fighter can
 * ever need. */
static size_t ndsFTManagerFigatreeHeapSizeKind(u32 data_flags, s32 kind)
{
    size_t largest = NDS_FTMANAGER_FIGATREE_HEAP_FLOOR;
    const FTData *data;
    size_t size;

    if ((kind < 0) || (kind >= nFTKindEnumCount))
    {
        return 0;
    }
    data = dFTManagerDataFiles[kind];
    if (data == NULL)
    {
        return 0;
    }
    if ((data_flags & FTDATA_FLAG_MAINMOTION) != 0u)
    {
        size = ndsFTManagerLargestFigatree(data->mainmotion,
                                           data->mainmotion_array_count);
        if (size > largest)
        {
            largest = size;
        }
    }
    if (((data_flags & FTDATA_FLAG_SUBMOTION) != 0u) &&
        (data->submotion_array_count != NULL))
    {
        size = ndsFTManagerLargestFigatree(data->submotion,
                                           *data->submotion_array_count);
        if (size > largest)
        {
            largest = size;
        }
    }
    return largest;
}

static size_t ndsFTManagerFigatreeHeapSize(u32 data_flags)
{
    size_t largest = NDS_FTMANAGER_FIGATREE_HEAP_FLOOR;
    s32 kind;

    for (kind = 0; kind < nFTKindEnumCount; kind++)
    {
        size_t size = ndsFTManagerFigatreeHeapSizeKind(data_flags, kind);

        if (size > largest)
        {
            largest = size;
        }
    }
    return largest;
}

__attribute__((used)) volatile u32 gNdsFTManagerFigatreeHeapMeasured;
/* Per-slot figatree sizing, P2-3f49. The roster maximum below is what the
 * source computes and what every allocation used, which is correct on a
 * console whose whole roster is twelve fighters and whose binary is small.
 * Here the same rule made admitting a fighter raise the cost of EVERY match:
 * a slot's heap holds one animation of the fighter that occupies it, so
 * sizing it by the worst animation in the roster charges four slots for a
 * fighter who is not in the match. The flags the roster pass ran with are
 * kept so a per-kind size can be computed later, at the point the kind is
 * finally known. */
static u32 sNdsFTManagerFigatreeDataFlags;
__attribute__((used)) volatile u32 gNdsFTManagerFigatreeHeapKindCount;
__attribute__((used)) volatile u32 gNdsFTManagerFigatreeHeapKindBytes;
__attribute__((used)) volatile u32 gNdsFTManagerFigatreeHeapSavedBytes;

void ftManagerAllocFighter(u32 data_flags, s32 allocs_num)
{
    (void)allocs_num;
    gNdsSCVSBattleCompatManagerMask |= 1u << 0;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
    sNdsFTManagerFigatreeDataFlags = data_flags;
    if (gFTManagerFigatreeHeapSize == 0)
    {
        gFTManagerFigatreeHeapSize = ndsFTManagerFigatreeHeapSize(data_flags);
        gNdsFTManagerFigatreeHeapMeasured =
            (u32)gFTManagerFigatreeHeapSize;
    }
    if (ndsFighterMarioFoxModelProofEnabled() != FALSE)
    {
        ndsFighterMarioFoxSetupManagerFiles();
    }
}

void ftManagerSetupFilesAllKind(s32 fkind)
{
    gNdsSCVSBattleCompatManagerMask |= 1u << 1;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
    if (ndsFighterMarioFoxModelProofEnabled() != FALSE)
    {
        ndsFighterMarioFoxSetupFilesKind(fkind);
    }
}

void ftManagerSetupFilesPlayablesAll(void)
{
    gNdsSCVSBattleCompatManagerMask |= 1u << 2;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
    if (ndsFighterMarioFoxModelProofEnabled() != FALSE)
    {
        ndsFighterMarioFoxSetupFilesKind(nFTKindMario);
        ndsFighterMarioFoxSetupFilesKind(nFTKindFox);
    }
}

void *ftManagerAllocFigatreeHeapKind(s32 fkind)
{
    /* The kind was already a parameter and was already being discarded. Use
     * it: this heap parses one animation at a time for the fighter in this
     * slot, so the roster maximum is only ever an upper bound, never a
     * requirement. Falls back to that maximum when the kind has no data --
     * the caller must still get a heap the source's parse can use. */
    size_t kind_size =
        ndsFTManagerFigatreeHeapSizeKind(sNdsFTManagerFigatreeDataFlags, fkind);

    if ((kind_size == 0u) || (kind_size > gFTManagerFigatreeHeapSize))
    {
        kind_size = gFTManagerFigatreeHeapSize;
    }
    gNdsFTManagerFigatreeHeapKindCount++;
    gNdsFTManagerFigatreeHeapKindBytes = (u32)kind_size;
    gNdsFTManagerFigatreeHeapSavedBytes +=
        (u32)(gFTManagerFigatreeHeapSize - kind_size);
    return syTaskmanMalloc(kind_size, 0x10);
}

GObj *ftManagerMakeFighter(FTDesc *desc)
{
    if (ndsFighterMarioFoxModelProofEnabled() != FALSE)
    {
        GObj *model_gobj = ndsFighterMarioFoxMakeFighter(desc);

        if (model_gobj != NULL)
        {
            return model_gobj;
        }
    }

    GObj *fighter_gobj = gcMakeGObjSPAfter(nGCCommonKindFighter,
                                           NULL,
                                           nGCCommonLinkIDFighter,
                                           GOBJ_PRIORITY_DEFAULT);

    if (fighter_gobj != NULL)
    {
        fighter_gobj->user_data.s = desc->player;
        gcAddDObjForGObj(fighter_gobj, NULL);
        gNdsSCVSBattleOriginalFighterGObjCount++;
        gNdsSCVSBattleOriginalFighterCreateCount++;
        gNdsSCVSBattleOriginalActivePlayerCount++;
        gNdsSCVSBattleOriginalActivePlayerMask |= 1u << (desc->player & 3);
        /* P2-3r15: one byte per slot, `fkind + 1`, so a four-name roster is
         * readable at all -- the P0/P1 kind pair below stops at two. The
         * recorder that actually runs in a VSBattle is
         * ndsFighterManagerRecordCreatedFighter; this path is the compat/proof
         * twin and writes the same field the same way. */
        gNdsSCVSBattleOriginalFighterKinds =
            (gNdsSCVSBattleOriginalFighterKinds &
             ~(0xffu << ((desc->player & 3u) * 8u))) |
            ((((u32)desc->fkind + 1u) & 0xffu) << ((desc->player & 3u) * 8u));
        if (desc->player == 0)
        {
            gNdsSCVSBattleOriginalP0FKind = (u32)desc->fkind;
            gNdsSCVSBattleOriginalP0LR = (u32)desc->lr;
        }
        else if (desc->player == 1)
        {
            gNdsSCVSBattleOriginalP1FKind = (u32)desc->fkind;
            gNdsSCVSBattleOriginalP1LR = (u32)desc->lr;
        }
        gNdsSCVSBattleCompatManagerMask |= 1u << 3;
        gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
        ndsFighterMarioFoxRecordStubFighter(desc, fighter_gobj);
    }
    return fighter_gobj;
}

void ftManagerDestroyFighter(GObj *fighter_gobj)
{
    if (fighter_gobj != NULL)
    {
        gcEjectGObj(fighter_gobj);
    }
}
#endif

void ftParamInitTexturePartAll(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTTexturePartStatus *texturepart_status;
    FTTexturePart *texturepart;
    DObj *joint;
    MObj *mobj;
    s32 detail;
    s32 i;
    s32 j;

    if ((fp == NULL) || (fp->attr == NULL) ||
        (fp->attr->textureparts_container == NULL))
    {
        return;
    }

    /* BattleShip ftparam.c:1070-1110. Re-applying a costume replaces each
     * joint's MObj chain, so any live texture-part override must be copied back
     * to the newly-created material at this detail level. */
    for (i = 0,
         texturepart_status = &fp->texturepart_status[0],
         texturepart = &fp->attr->textureparts_container->textureparts[0];
         i < (s32)ARRAY_COUNT(fp->texturepart_status);
         i++, texturepart_status++, texturepart++)
    {
        if (texturepart_status->texture_id_curr !=
            texturepart_status->texture_id_base)
        {
            joint = fp->joints[texturepart->joint_id];
            detail = texturepart->detail[fp->detail_curr - nFTPartsDetailStart];
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

void ftParamInitAllParts(GObj *fighter_gobj, s32 costume, s32 shade)
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

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    /* The source operation above replaces MObjs in place. The DS native
     * renderer caches immutable material conversion by MObj identity, and the
     * source allocator may immediately reuse the just-freed MObj address. Tell
     * that cache about the source lifetime boundary so a costume can never
     * inherit the previous costume's converted texture/material block. */
    ndsFighterRendererInvalidateMaterialCachesForSlot((u32)fp->player);
#endif
}

sb32 ftParamCheckSetColAnimID(GMColAnim *colanim, s32 colanim_id, s32 length)
{
    s32 i;

    if ((colanim == NULL) || (colanim_id < 0) ||
        (colanim_id >= nGMColAnimEnumCount) ||
        (dGMColScriptsDescs[colanim_id].p_script == NULL))
    {
        return FALSE;
    }
    if ((colanim->colanim_id >= 0) &&
        (colanim->colanim_id < nGMColAnimEnumCount) &&
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
        gNdsDonkeyChargePresentationOverrideCount++;
        gNdsDonkeyChargePresentationRequestedID = colanim_id;
#endif
        colanim_id = nGMColAnimFighterCommonSpecialNCharge;
#if NDS_P2_DONKEY
        gNdsDonkeyChargePresentationAppliedID = colanim_id;
#endif
    }

    ndsTask39EffectCensusRecord(
        NDS_TASK39_EFFECT_FT_PARAM_CHECK_SET_FIGHTER_COL_ANIM_I_D,
        NDS_TASK39_EFFECT_ORIGINAL);
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        sNdsFighterDashRunDamageColAnimLastID = colanim_id;
        sNdsFighterDashRunDamageColAnimLastDuration = unused;
        if (colanim_id == nGMColAnimFighterFuraSleep)
        {
            sNdsFighterDashRunDamageSetupColAnimCount++;
        }
    }
#if NDS_TASK39_FX_FLASH
    if ((fighter_gobj != NULL) && (ftGetStruct(fighter_gobj) != NULL))
    {
        return ftParamCheckSetColAnimID(&ftGetStruct(fighter_gobj)->colanim,
                                       colanim_id, unused);
    }
    return FALSE;
#else
    (void)fighter_gobj;
    return ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
            (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
               ? TRUE
               : FALSE;
#endif
}

sb32 ftParamCheckSetSkeletonColAnimID(GObj *fighter_gobj, s32 damage_level)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    sb32 result = FALSE;

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        sNdsFighterDashRunDamageSkeletonColAnimLastLevel = damage_level;
    }
    /* BattleShip ftparam.c:1326-1330 maps each fighter kind to its electric
     * skeleton family. dFTParamSkeletonColAnimIDs[] is 0x14 for both Mario and
     * Fox, i.e. nGMColAnimFighterDamageElectricSkeletonStart. P2-2 currently
     * owns only those two fighter kinds, so keep the mapping bounded to the
     * supported roster instead of inventing tables for unloaded characters. */
    if ((fp != NULL) && (damage_level >= 0) && (damage_level < 4) &&
        ((fp->fkind == nFTKindMario) || (fp->fkind == nFTKindFox)))
    {
        result = ftParamCheckSetFighterColAnimID(
            fighter_gobj,
            nGMColAnimFighterDamageElectricSkeletonStart + damage_level, 0);
    }
    if (result != FALSE)
    {
        return TRUE;
    }
    return ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
            (sNdsFighterDashRunDamageStatusSetupActive != FALSE)) ? TRUE : FALSE;
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

    if ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffCatchFloorLoopStopVelCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopCliffCatchStopVelCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCaptureActive != FALSE))
    {
        gNdsStageMPPassiveLoopCaptureStopVelCount++;
    }
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
    if (ndsFighterMarioFoxInitProofEnabled() != FALSE)
    {
        gNdsFighterInitPhysicsStopCount++;
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
    if (ndsFighterMarioFoxInitProofEnabled() != FALSE)
    {
        gNdsFighterInitAttackClearCount++;
    }
}

typedef enum NDSGMHitType
{
    nNDSGMHitTypeDamage = 0,
    nNDSGMHitTypeShield = 1,
    nNDSGMHitTypeAttack = 3
} NDSGMHitType;

#if NDS_IMPORT_BATTLESHIP_FTMAIN
extern void battleship_ftMainSetHitInteractStats(FTStruct *fp,
                                                 u32 attack_group_id,
                                                 GObj *victim_gobj,
                                                 s32 attack_type,
                                                 u32 victim_group_id,
                                                 sb32 ignore_damage_or_hit);

static void ndsFTMainSetHitInteractStatsCompat(FTStruct *fp,
                                               u32 attack_group_id,
                                               GObj *victim_gobj,
                                               s32 attack_type,
                                               u32 victim_group_id,
                                               sb32 ignore_damage_or_hit)
{
    u32 i;
    u32 j;

    if ((fp == NULL) || (victim_gobj == NULL))
    {
        return;
    }
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        FTAttackColl *attack_coll = &fp->attack_colls[i];

        if ((attack_coll->attack_state == nGMAttackStateOff) ||
            (attack_coll->group_id != attack_group_id))
        {
            continue;
        }
        for (j = 0u; j < GMATTACKREC_NUM_MAX; j++)
        {
            if (victim_gobj == attack_coll->attack_records[j].victim_gobj)
            {
                break;
            }
        }
        if (j == GMATTACKREC_NUM_MAX)
        {
            for (j = 0u; j < GMATTACKREC_NUM_MAX; j++)
            {
                if (attack_coll->attack_records[j].victim_gobj == NULL)
                {
                    break;
                }
            }
            if (j == GMATTACKREC_NUM_MAX)
            {
                j = 0u;
            }
            attack_coll->attack_records[j].victim_gobj = victim_gobj;
        }
        switch (attack_type)
        {
        case nNDSGMHitTypeDamage:
            attack_coll->attack_records[j].victim_flags.is_interact_hurt =
                TRUE;
            break;
        case nNDSGMHitTypeShield:
            attack_coll->attack_records[j].victim_flags.is_interact_shield =
                TRUE;
            break;
        case nNDSGMHitTypeAttack:
            attack_coll->attack_records[j].victim_flags.group_id =
                victim_group_id;
            break;
        default:
            break;
        }
        if (ignore_damage_or_hit == FALSE)
        {
            gFTMainIsDamageDetect[i] = FALSE;
        }
        else
        {
            gFTMainIsAttackDetect[i] = FALSE;
        }
    }
}

void ftMainSetHitInteractStats(FTStruct *fp, u32 attack_group_id,
                               GObj *victim_gobj, s32 attack_type,
                               u32 victim_group_id,
                               sb32 ignore_damage_or_hit)
{
    battleship_ftMainSetHitInteractStats(fp, attack_group_id, victim_gobj,
                                         attack_type, victim_group_id,
                                         ignore_damage_or_hit);
    if (ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE)
    {
        ndsFTMainSetHitInteractStatsCompat(fp, attack_group_id, victim_gobj,
                                           attack_type, victim_group_id,
                                           ignore_damage_or_hit);
    }
}
#else
sb32 gFTMainIsDamageDetect[FTATTACKCOLL_NUM_MAX];
sb32 gFTMainIsAttackDetect[FTATTACKCOLL_NUM_MAX];

void ftMainSetHitInteractStats(FTStruct *fp, u32 attack_group_id,
                               GObj *victim_gobj, s32 attack_type,
                               u32 victim_group_id,
                               sb32 ignore_damage_or_hit)
{
    u32 i;
    u32 j;

    if ((fp == NULL) || (victim_gobj == NULL))
    {
        return;
    }

    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        FTAttackColl *attack_coll = &fp->attack_colls[i];

        if ((attack_coll->attack_state == nGMAttackStateOff) ||
            (attack_coll->group_id != attack_group_id))
        {
            continue;
        }

        for (j = 0u; j < GMATTACKREC_NUM_MAX; j++)
        {
            if (victim_gobj == attack_coll->attack_records[j].victim_gobj)
            {
                break;
            }
        }
        if (j == GMATTACKREC_NUM_MAX)
        {
            for (j = 0u; j < GMATTACKREC_NUM_MAX; j++)
            {
                if (attack_coll->attack_records[j].victim_gobj == NULL)
                {
                    break;
                }
            }
            if (j == GMATTACKREC_NUM_MAX)
            {
                j = 0u;
            }
            attack_coll->attack_records[j].victim_gobj = victim_gobj;
        }

        switch (attack_type)
        {
        case nNDSGMHitTypeDamage:
            attack_coll->attack_records[j].victim_flags.is_interact_hurt =
                TRUE;
            break;

        case nNDSGMHitTypeShield:
            attack_coll->attack_records[j].victim_flags.is_interact_shield =
                TRUE;
            break;

        case nNDSGMHitTypeAttack:
            attack_coll->attack_records[j].victim_flags.group_id =
                victim_group_id;
            break;

        default:
            break;
        }

        if (ignore_damage_or_hit == 0)
        {
            gFTMainIsDamageDetect[i] = FALSE;
        }
        else
        {
            gFTMainIsAttackDetect[i] = FALSE;
        }
    }
}
#endif

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
    if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
        (sNdsFighterJumpAttackAirRefreshActive != FALSE))
    {
        gNdsFighterJumpAttackAirRefreshCount++;
        gNdsFighterJumpAttackAirRefreshMask |= 1u << (u32)attack_id;
    }
    if ((ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE) &&
        (sNdsStageMPLiveHitOriginalRehitRefreshActive != FALSE))
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshIDMask |=
            1u << (u32)attack_id;
    }
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    if (((gNdsFighterNaturalMovesetPhase == 9u) ||
         (gNdsFighterNaturalMovesetPhase == 10u)) &&
        (fp->status_id >= nFTCommonStatusAttackAirStart) &&
        (fp->status_id <= nFTCommonStatusAttackAirEnd))
    {
        gNdsFighterNaturalMovesetAerialHitboxFrames++;
    }
#endif
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
    if (ndsFighterMarioFoxInitProofEnabled() != FALSE)
    {
        gNdsFighterInitHitStatusPartCount++;
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
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunGuardOnActive != FALSE))
    {
        gNdsFighterDashRunGuardStateMask |= 1u << 8u;
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
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    ndsFighterRendererInvalidateDObjStateCaches(fighter_gobj);
    if (fp != NULL)
    {
        ndsFighterRendererInvalidateMaterialCachesForSlot((u32)fp->player);
    }
#else
    (void)fighter_gobj;
    (void)fp;
#endif
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
            gNdsFighterModelPartResetCount++;
            changed = TRUE;
        }
    }
    /* Source clears this after all requested parts have been restored. */
    fp->is_modelpart_modify = FALSE;
    if (changed != FALSE)
    {
        ndsFTParamInvalidateModelPartRenderer(fighter_gobj, fp);
    }
}

void ftParamSetTexturePartID(GObj *fighter_gobj, s32 texturepart_id,
                             s32 texture_id)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    FTTexturePartContainer *container;
    FTTexturePart *texturepart;
    DObj *joint;
    MObj *mobj;
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
        fp->texturepart_status[texturepart_id].texture_id_curr = texture_id;
        fp->is_texturepart_modify = TRUE;
        return;
    }

    texturepart = &container->textureparts[texturepart_id];
    if ((texturepart->joint_id >= ARRAY_COUNT(fp->joints)) ||
        (fp->detail_curr < nFTPartsDetailStart))
    {
        return;
    }

    detail = texturepart->detail[fp->detail_curr - nFTPartsDetailStart];
    joint = fp->joints[texturepart->joint_id];
    mobj = (joint != NULL) ? joint->mobj : NULL;
    for (i = 0; (mobj != NULL) && (i < detail); i++)
    {
        mobj = mobj->next;
    }
    if (mobj != NULL)
    {
        mobj->texture_id_curr = texture_id;
    }
    fp->texturepart_status[texturepart_id].texture_id_curr = texture_id;
    fp->is_texturepart_modify = TRUE;
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
        FTTexturePart *texturepart;
        DObj *joint;
        MObj *mobj;
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
        texturepart = &container->textureparts[i];
        if (texturepart->joint_id >= ARRAY_COUNT(fp->joints))
        {
            continue;
        }
        detail = texturepart->detail[fp->detail_curr - nFTPartsDetailStart];
        joint = fp->joints[texturepart->joint_id];
        mobj = (joint != NULL) ? joint->mobj : NULL;
        for (j = 0; (mobj != NULL) && (j < detail); j++)
        {
            mobj = mobj->next;
        }
        if (mobj != NULL)
        {
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
static void NDS_TASK37_ITCM_CODE ndsFTParamsInvalidateFighterParts(
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
#define NDS_FTPARTS_FLAT_SLOTS GMCOMMON_PLAYERS_MAX
_Static_assert((NDS_FTPARTS_FLAT_SLOTS & (NDS_FTPARTS_FLAT_SLOTS - 1u)) == 0u,
               "flat fighter-parts cache hash requires a power-of-two player bound");
#define NDS_FTPARTS_FLAT_MAX 96u

typedef struct NDSFtPartsFlatWalk
{
    const DObj *root;
    u32 heap_generation;
    u32 count;
    FTParts *parts[NDS_FTPARTS_FLAT_MAX];
} NDSFtPartsFlatWalk;

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

static const NDSFtPartsFlatWalk *ndsFTParamsFlatWalkFor(DObj *root)
{
    NDSFtPartsFlatWalk *flat =
        &sNdsFtPartsFlat[((u32)(uintptr_t)root >> 4) &
                         (NDS_FTPARTS_FLAT_SLOTS - 1u)];
    u32 count;

    if ((flat->root == root) &&
        (flat->heap_generation == gNdsTaskmanHeapGeneration))
    {
        return flat;
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
        ndsFTParamsInvalidateFlatParts(flat, reset_mode);
        return;
    }
    for (child = root->child; child != NULL; child = child->sib_next)
    {
        ndsFTParamsInvalidateFighterParts(child, reset_mode);
    }
}

void __attribute__((noinline, optimize("Os")))
ftParamsUpdateFighterPartsTransform(DObj *joint)
{
    GObj *fighter_gobj;
    FTStruct *fp;
    u32 joint_id;

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
         * per-body mode load/branch/write. */
        fp = ftGetStruct(fighter_gobj);
        if (fp != NULL)
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
    if (joint != NULL)
    {
        ndsFTParamsInvalidateRootParts(joint, TRUE);
        ndsFTParamsInvalidateSubtree(joint, FALSE);
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunGuardOnActive != FALSE))
    {
        gNdsFighterDashRunGuardStateMask |= 1u << 9u;
    }
}

extern void gcParseDObjAnimJoint(DObj *dobj);
extern void gcPlayDObjAnimJoint(DObj *dobj);
extern void gcParseMObjMatAnimJoint(MObj *mobj);
extern void gcPlayMObjMatAnim(MObj *mobj);
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
 * a divide on purpose.
 *
 * NDS_R2_ANIM_PARSER defaults to 1 because the replacement is strictly cheaper
 * at equal output. The route arm exists only under NDS_R2_ANIM_CUT_ROUTE, which
 * a published build never sets. */
#ifndef NDS_R2_ANIM_PARSER
#define NDS_R2_ANIM_PARSER 1
#endif
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
void ndsR2FtAnimParseDObjFigatree(DObj *root_dobj);
#if NDS_R2_ANIM_CUT_ROUTE
extern volatile u32 gNdsR2AnimCutRoute;
#endif
#endif

void ftAnimParseDObjFigatree(DObj *root_dobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
#if NDS_R2_FTANIM_TRACK
    /* Stage 3. One range compare against a static array decides it: a joint
     * bound to precompiled rows carries its own cursor block in
     * `anim_joint.event16`, and nothing else in the tree reads that field for a
     * figatree joint (`gcParseDObjAnimJoint` owns the AObj32 path and is
     * selected by `is_anim_joint` at the caller). At dispatch 0 no block is
     * ever installed, so this is FALSE for every joint on the same binary. */
    if (ndsFtAnimTrackIsDense(root_dobj->anim_joint.event16) != FALSE)
    {
        ndsFtAnimTrackStep(root_dobj);
        return;
    }
#endif
#if NDS_R2_ANIM_CUT_ROUTE
    if ((gNdsR2AnimCutRoute & 4u) != 0u)
    {
        ndsR2FtAnimParseDObjFigatree(root_dobj);
    }
    else
    {
        battleship_ftAnimParseDObjFigatree(root_dobj);
    }
#elif NDS_R2_ANIM_PARSER
    ndsR2FtAnimParseDObjFigatree(root_dobj);
#else
    battleship_ftAnimParseDObjFigatree(root_dobj);
#endif
#else
    (void)root_dobj;
#endif
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
#if NDS_R2_ANIM_CUT_ROUTE
extern volatile u32 gNdsR2AnimCutRoute;
#define NDS_R2_ANIM_CUT_ON(bit) ((gNdsR2AnimCutRoute & (bit)) != 0u)
#else
#define NDS_R2_ANIM_CUT_ON(bit) (1)
#endif

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

    if (fp == NULL)
    {
        return;
    }
    p_joint = &fp->joints[nFTPartsJointTopN];
    joint_limit = ndsFTStructJointLoopLimit(fp);
#if NDS_ANIM_JOINT_AUDIT
    /* Once per call, OUTSIDE the idle-joint skip, so a Dispatch32 of zero can be
     * read: FlagFrames 0 means the flag was never set here, FlagFrames high with
     * Dispatch32 0 means every joint was already stopped. */
    if (fp->anim_desc.flags.is_anim_joint)
    {
        gNdsAnimJointFlagFrames++;
    }
#endif

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
                mobj = joint->mobj;
                while (mobj != NULL)
                {
                    gcParseMObjMatAnimJoint(mobj);
                    gcPlayMObjMatAnim(mobj);
                    mobj = mobj->next;
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
#if NDS_ANIM_JOINT_AUDIT
                    /* The invariant fttypes.h:59 states, checked where it is
                     * consumed. See nds_startup.h for the legend. */
                    gNdsAnimJointDispatch32Count++;
                    if (ndsRelocPointerIsFighterAObj16(
                            joint->anim_joint.event32) != FALSE)
                    {
                        gNdsAnimJointDispatchFigatreeCount++;
                        gNdsAnimJointDispatchFigatreeScript =
                            (u32)(uintptr_t)joint->anim_joint.event32;
                        if ((((uintptr_t)joint->anim_joint.event32) & 3u) != 0u)
                        {
                            gNdsAnimJointDispatchMisalignCount++;
                        }
                    }
#endif
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
                gNdsR2FtAnimNullSkips++;
#if NDS_ANIM_JOINT_AUDIT
                if (fp->anim_desc.flags.is_anim_joint)
                {
                    gNdsAnimJointIdleSkip32Count++;
                }
#endif
            }
            if (translate_scales != NULL)
            {
                translate_scales++;
            }

            mobj = joint->mobj;
            while (mobj != NULL)
            {
                gcParseMObjMatAnimJoint(mobj);
                gcPlayMObjMatAnim(mobj);
                mobj = mobj->next;
            }
        }
#if NDS_FT_POSE_ORACLE
        /* Lab: the engine ran on its shadows before the generic path ran on
         * the live joints; compare them now, bit for bit. */
        if (!fp->anim_desc.flags.is_anim_joint)
        {
            ndsFtPoseOracleAfter(fighter_gobj);
        }
#endif
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

void ftCommonShieldBreakFlyCommonSetStatus(GObj *fighter_gobj)
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

void ftCommonShieldBreakFlyReflectorSetStatus(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj) == NULL)
    {
        return;
    }

    ndsBaseFTCommonShieldBreakFlyReflectorSetStatus(fighter_gobj);
}

#if !NDS_P2_NESS
/* ftnessspeciallw.c:42 owns this once Ness is built (P2-3f47). */
void ftNessSpecialLwProcAbsorb(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp != NULL)
    {
        fp->lr = fp->absorb_lr;
        fp->is_absorb = TRUE;
    }
}
#endif

__attribute__((weak)) GObj *efManagerYoshiShieldMakeEffect(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return NULL;
}

__attribute__((weak)) LBParticle *efManagerEggBreakMakeEffect(Vec3f *pos)
{
    (void)pos;
    return NULL;
}

/* Defined beside lbCommonAddFighterPartsFigatree, which owns the shared tree
 * walk and the same pointer resolve. */
void lbCommonAddDObjAnimJointAll(DObj *dobj, AObjEvent32 **anim_joint,
                                 f32 anim_frame);
extern void gcAddDObj3TransformsKind(DObj *dobj, u8 tk1, u8 tk2, u8 tk3);
extern void gcDecideDObj3TransformsKind(DObj *dobj, u8 tk1, u8 tk2,
                                         u8 tk3, s32 flags);
extern DObj *lbCommonGetTreeDObjNextFromRoot(DObj *dobj, DObj *root_dobj);

void lbCommonSetupTreeDObjs(DObj *root_dobj, DObjDesc *dobjdesc,
                            DObj **dobjs, u8 tk1, u8 tk2, u8 tk3)
{
    s32 i;
    DObj *current_dobj;
    s32 id;
    DObj *array_dobjs[DOBJ_ARRAY_MAX];

    /* BattleShip lbcommon.c:914-952, transcribed rather than approximated.
     * This used to be an empty compatibility symbol because no shipped DS
     * effect depended on its tree. P2-2 restores Fox's source Arwing entry,
     * whose maker immediately walks the hierarchy this helper constructs; a
     * no-op therefore left the source's first child NULL and faulted at the
     * first source dereference. The O2R FoxSpecial3 descriptor was audited
     * independently: 12 nodes followed by the source DOBJ_ARRAY_MAX sentinel,
     * and its child/sibling chain matches efManagerFoxEntryArwingMakeEffect.
     * Keep the source topology, transform-kind selection and output ordering
     * exactly; only the reloc loader's already-native pointers are DS-specific. */
    for (i = 0; i < DOBJ_ARRAY_MAX; i++)
    {
        array_dobjs[i] = NULL;
    }
    while (dobjdesc->id != DOBJ_ARRAY_MAX)
    {
        id = dobjdesc->id & 0xFFF;

        if (id != 0)
        {
            current_dobj = array_dobjs[id] =
                gcAddChildForDObj(array_dobjs[id - 1], dobjdesc->dl);
        }
        else
        {
            current_dobj = array_dobjs[0] =
                gcAddChildForDObj(root_dobj, dobjdesc->dl);
        }
        if (dobjdesc->id & 0xF000)
        {
            gcDecideDObj3TransformsKind(current_dobj, tk1, tk2, tk3,
                                         dobjdesc->id & 0xF000);
        }
        else
        {
            gcAddDObj3TransformsKind(current_dobj, tk1, tk2, tk3);
        }
        current_dobj->translate.vec.f = dobjdesc->translate;
        current_dobj->rotate.vec.f = dobjdesc->rotate;
        current_dobj->scale.vec.f = dobjdesc->scale;

        if (dobjs != NULL)
        {
            *dobjs++ = current_dobj;
        }
        dobjdesc++;
    }
}

void lbCommonAddMObjForTreeDObjs(DObj *root_dobj, MObjSub ***p_mobjsubs)
{
    DObj *current_dobj = root_dobj;

    /* BattleShip lbcommon.c:1171-1196. Source model effects share this helper;
     * leaving it as a no-op would build the right hierarchy but silently omit
     * every source material attached through an EFDesc. */
    while (current_dobj != NULL)
    {
        if (p_mobjsubs != NULL)
        {
            if (*p_mobjsubs != NULL)
            {
                MObjSub **mobjsubs = *p_mobjsubs;
                MObjSub *mobjsub = *mobjsubs;

                while (mobjsub != NULL)
                {
                    gcAddMObjForDObj(current_dobj, mobjsub);
                    mobjsubs++;
                    mobjsub = *mobjsubs;
                }
            }
            p_mobjsubs++;
        }
        current_dobj =
            lbCommonGetTreeDObjNextFromRoot(current_dobj, root_dobj);
    }
}

void lbCommonAddTreeDObjsAnimAll(DObj *root_dobj,
                                 AObjEvent32 **anim_joints,
                                 AObjEvent32 ***p_matanim_joints,
                                 f32 anim_frame)
{
    DObj *current_dobj = root_dobj;

    /* BattleShip lbcommon.c:838-881. Animation and material-animation streams
     * remain source-owned; the DS adaptation is only that the reloc layer has
     * already converted their pointers before this source-shaped walk. */
    root_dobj->parent_gobj->anim_frame = anim_frame;

    while (current_dobj != NULL)
    {
        if (anim_joints != NULL)
        {
            AObjEvent32 *anim_joint = *anim_joints;

            if (anim_joint != NULL)
            {
                gcAddDObjAnimJoint(current_dobj, anim_joint, anim_frame);
            }
            else
            {
                current_dobj->anim_wait = AOBJ_ANIM_NULL;
            }
            anim_joints++;
        }
        if (p_matanim_joints != NULL)
        {
            if (*p_matanim_joints != NULL)
            {
                MObj *mobj = current_dobj->mobj;
                AObjEvent32 **matanim_joints = *p_matanim_joints;

                while (mobj != NULL)
                {
                    AObjEvent32 *matanim_joint = *matanim_joints;

                    if (matanim_joint != NULL)
                    {
                        gcAddMObjMatAnimJoint(mobj, matanim_joint, anim_frame);
                    }
                    mobj = mobj->next;
                    matanim_joints++;
                }
            }
            p_matanim_joints++;
        }
        current_dobj =
            lbCommonGetTreeDObjNextFromRoot(current_dobj, root_dobj);
    }
}

void lbCommonSetDObjTransformsForTreeDObjs(DObj *root_dobj,
                                           DObjDesc *dobjdesc)
{
    DObj *current_dobj = root_dobj;

    /* BattleShip lbcommon.c:1211-1225. */
    while ((current_dobj != NULL) && (dobjdesc->id != DOBJ_ARRAY_MAX))
    {
        current_dobj->translate.vec.f = dobjdesc->translate;
        current_dobj->rotate.vec.f = dobjdesc->rotate;
        current_dobj->scale.vec.f = dobjdesc->scale;
        dobjdesc++;
        current_dobj =
            lbCommonGetTreeDObjNextFromRoot(current_dobj, root_dobj);
    }
}

void ftParamResetFighterColAnim(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp != NULL)
    {
        ftParamResetColAnim(&fp->colanim);
    }
    if (ndsFighterMarioFoxInitProofEnabled() != FALSE)
    {
        gNdsFighterInitColAnimResetCount++;
    }
}

void ftParamResetColAnim(GMColAnim *colanim)
{
    s32 i;

    if (colanim == NULL)
    {
        return;
    }
    for (i = 0; i < ARRAY_COUNT(colanim->cs); i++)
    {
        colanim->cs[i].p_script = NULL;
        colanim->cs[i].color_event_timer = 0;
        colanim->cs[i].script_id = 0;
    }
    colanim->length = 0;
    colanim->colanim_id = 0;
    colanim->is_use_color1 = 0;
    colanim->is_use_color2 = 0;
    colanim->is_use_light = 0;
    colanim->skeleton_id = 0;
}

void ftParamResetStatUpdateColAnim(GObj *fighter_gobj)
{
    enum
    {
        /* Source-private constants: ftdonkey.h / ftsamus.h. Keep them local
         * instead of importing character-private headers into this compat TU. */
        NDS_COMPAT_DONKEY_CHARGE_MAX = 10,
        NDS_COMPAT_SAMUS_CHARGE_MAX = 7
    };
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return;
    }

    /* BattleShip ftparam.c:1247-1323. This was once intentionally trimmed to
     * Mario/Fox when those were the only live fighters, but P2 now admits
     * Donkey and Samus. Their full-charge indicator is restored *here* after a
     * higher-priority unlocked ColAnim ends; the direct charge-max request can
     * legitimately lose arbitration while the 60-priority charging flash is
     * still active. Keep the source reset/reapply contract as the roster grows. */
    ftParamResetFighterColAnim(fighter_gobj);

    switch (ftParamGetBestHitStatusPart(fighter_gobj))
    {
    case nGMHitStatusInvincible:
        ftParamCheckSetFighterColAnimID(
            fighter_gobj, nGMColAnimFighterHitStatusInvincible, 0);
        break;
    case nGMHitStatusIntangible:
        ftParamCheckSetFighterColAnimID(
            fighter_gobj, nGMColAnimFighterHitStatusIntangible, 0);
        break;
    }
    switch (fp->fkind)
    {
    case nFTKindDonkey:
    case nFTKindNDonkey:
    case nFTKindGDonkey:
        if (fp->passive_vars.donkey.charge_level ==
            NDS_COMPAT_DONKEY_CHARGE_MAX)
        {
            ftParamCheckSetFighterColAnimID(
                fighter_gobj, nGMColAnimFighterCommonSpecialNCharge, 0);
        }
        break;
    case nFTKindSamus:
    case nFTKindNSamus:
        if (fp->passive_vars.samus.charge_level ==
            NDS_COMPAT_SAMUS_CHARGE_MAX)
        {
            ftParamCheckSetFighterColAnimID(
                fighter_gobj, nGMColAnimFighterCommonSpecialNCharge, 0);
        }
        break;
    }
    if (fp->damage_heal != 0)
    {
        ftParamCheckSetFighterColAnimID(fighter_gobj,
                                        nGMColAnimFighterHeal, 0);
    }
    if (fp->star_invincible_tics != 0)
    {
        ftParamCheckSetFighterColAnimID(fighter_gobj,
                                        nGMColAnimFighterStar, 0);
    }
    if ((fp->invincible_tics != 0) || (fp->intangible_tics != 0))
    {
        ftParamCheckSetFighterColAnimID(fighter_gobj,
                                        nGMColAnimFighterNoDamage, 0);
    }
    if ((fp->status_id >= nFTCommonStatusHammerStart) &&
        (fp->status_id <= nFTCommonStatusHammerEnd))
    {
        ftParamCheckSetFighterColAnimID(fighter_gobj,
                                        nGMColAnimFighterHammer, 0);
    }
}

/* The real ones come with battleship_ftcommon_hammer.c, which imports
 * fthammer.c whole. These no-op/counter shims are why picking the Hammer up
 * started its timer and music but the fighter never entered any Hammer
 * state. */
#if !NDS_P2_ITEM_CORE
sb32 ftHammerCheckHoldHammer(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if (sNdsFighterDashRunDamageHammerCheckActive != FALSE)
    {
        sNdsFighterDashRunDamageHammerCheckCount++;
        return sNdsFighterDashRunDamageHammerHold;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopActive != FALSE))
    {
        return FALSE;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingJumpAnimEndActive != FALSE))
    {
        return FALSE;
    }
    if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
    {
        gNdsFighterWaitHammerCheckCount++;
    }
    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        gNdsFighterJumpHammerHoldCheckCount++;
    }
    return FALSE;
}

void ftHammerProcInterrupt(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if (sNdsFighterDashRunDamageHammerCheckActive != FALSE)
    {
        sNdsFighterDashRunDamageHammerGroundCount++;
    }
}

void ftHammerSetStatusHammerWait(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
    {
        gNdsFighterWaitHammerDeniedCount++;
    }
}
#endif

sb32 ftCommonGroundCheckInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    /* BattleShip fighter.h defines this exact ordered chain as the common
     * ground interrupt. The imported manager runs it for every live fighter;
     * it must not depend on the old scripted natural-motion proof being active. */
    return ((ftCommonSpecialNCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonSpecialHiCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonSpecialLwCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonCatchCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackS4CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackHi4CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackLw4CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackS3CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackHi3CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackLw3CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttack1CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonGuardOnCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAppealCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonKneeBendCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonDashCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonPassCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonDokanStartCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonSquatCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonTurnCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonWalkCheckInterruptCommon(fighter_gobj) != FALSE)) ? TRUE :
                                                                         FALSE;
#endif
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if (fp == NULL)
        {
            return FALSE;
        }
        if ((fp->input.pl.button_tap & U_CBUTTONS) != 0u)
        {
            return ftCommonKneeBendCheckInterruptCommon(fighter_gobj);
        }
        if ((ABS(fp->input.pl.stick_range.x) >= FTCOMMON_DASH_STICK_RANGE_MIN) &&
            (fp->tap_stick_x == 0u))
        {
            return ftCommonDashCheckInterruptCommon(fighter_gobj);
        }
        if (ABS(fp->input.pl.stick_range.x) >= 8)
        {
            return ftCommonWalkCheckInterruptCommon(fighter_gobj);
        }
        return ftCommonWaitCheckInterruptCommon(fighter_gobj);
    }
    if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
    {
        gNdsFighterWaitProcInterruptCallCount++;
    }
    if (ndsFighterMarioFoxWaitTickProofEnabled() != FALSE)
    {
        gNdsFighterWaitTickGroundInterruptCheckCount++;
    }
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpWaitProbeActive != FALSE))
    {
        sb32 result;

        gNdsFighterJumpWaitInterruptCallCount++;
        gNdsFighterJumpGroundCheckCallCount++;
        result = ftCommonKneeBendCheckInterruptCommon(fighter_gobj);
        return result;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunWaitInterruptActive != FALSE))
    {
        sb32 result;

        gNdsFighterDashRunWaitInterruptCallCount++;
        gNdsFighterDashRunGroundCheckCallCount++;
        if (sNdsFighterDashRunAttack1Active != FALSE)
        {
            result = ftCommonAttack1CheckInterruptCommon(fighter_gobj);
        }
        else
        {
            result = ftCommonDashCheckInterruptCommon(fighter_gobj);
        }
        return result;
    }
    if ((ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE) &&
        (sNdsFighterWalkLoopWaitReturnActive != FALSE))
    {
        sb32 result;

        gNdsFighterWalkLoopWaitReturnCheckCount++;
        result = ftCommonWaitCheckInterruptCommon(fighter_gobj);
        if (result != FALSE)
        {
            gNdsFighterWalkLoopWaitReturnSuccessCount++;
        }
        return result;
    }
    if ((ndsFighterMarioFoxWalkInputProofEnabled() != FALSE) &&
        (sNdsFighterWalkInputProbeActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        s32 stick_x = 0;
        s32 lr = 0;
        sb32 result;

        if (fp != NULL)
        {
            stick_x = fp->input.pl.stick_range.x;
            lr = fp->lr;
        }
        gNdsFighterWalkWaitInterruptCallCount++;
        gNdsFighterWalkGroundCheckCallCount++;
        gNdsFighterWalkOriginalCheckCallCount++;
        result = ftCommonWalkCheckInterruptCommon(fighter_gobj);
        if ((result == FALSE) && (fp != NULL) &&
            ((stick_x * lr) >= 8))
        {
            s32 status_id = ftCommonWalkGetWalkStatus((s8)stick_x);

            fp->input.pl.stick_range.x = (s8)stick_x;
            ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F,
                            FTSTATUS_PRESERVE_NONE);
            ftMainPlayAnimEventsAll(fighter_gobj);
            if (status_id != nFTCommonStatusWalkFast)
            {
                fp->is_special_interrupt = TRUE;
            }
            result = TRUE;
        }
        if (result != FALSE)
        {
            gNdsFighterWalkOriginalCheckSuccessCount++;
        }
        return result;
    }
    return FALSE;
}

static sb32 ndsFighterWalkDeferredInterrupt(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        ((sNdsFighterProcessLoopInterruptActive != FALSE) ||
         (sNdsFighterProcessLoopMapActive != FALSE)))
    {
        gNdsFighterProcessLoopDeferredInterruptCheckCount++;
        gNdsFighterMarioFoxProcessLoopDeferredMask |= 0xffu;
        return FALSE;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        ((sNdsFighterLandingFallInterruptActive != FALSE) ||
         (sNdsFighterLandingProcInterruptActive != FALSE)))
    {
        gNdsFighterLandingDeferredInterruptCheckCount++;
        gNdsFighterMarioFoxLandingLoopDeferredMask = 0xffu;
        return FALSE;
    }
    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        gNdsFighterJumpDeferredInterruptCheckCount++;
        gNdsFighterMarioFoxJumpLoopDeferredMask |= 1u;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        ((sNdsFighterDashRunDashInterruptActive != FALSE) ||
         (sNdsFighterDashRunRunInterruptActive != FALSE) ||
         (sNdsFighterDashRunRunBrakeInterruptActive != FALSE)))
    {
        gNdsFighterDashRunDeferredInterruptCount++;
        gNdsFighterMarioFoxDashRunDeferredMask = 0xffu;
        return FALSE;
    }
    if ((ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE) &&
        (sNdsFighterWalkLoopFrameActive != FALSE))
    {
        gNdsFighterMarioFoxWalkLoopDeferredMask = 0xffu;
        return FALSE;
    }
    if ((ndsFighterMarioFoxWalkInputProofEnabled() != FALSE) &&
        (sNdsFighterWalkLoopProbeActive != FALSE))
    {
        gNdsFighterWalkDeferredInterruptCheckCount++;
    }
    return FALSE;
}

sb32 ftCommonSpecialHiCheckInterruptCommon(GObj *fighter_gobj)
{
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpKneeBendInterruptActive != FALSE))
    {
        gNdsFighterJumpSpecialHiCheckCount++;
    }
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
    {
        extern sb32 ndsBaseFTCommonSpecialHiCheckInterruptCommon(
            GObj *fighter_gobj);

        return ndsBaseFTCommonSpecialHiCheckInterruptCommon(fighter_gobj);
    }
#else
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
#endif
}

sb32 ftCommonCatchCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonCatchCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopAppealGuardActive != FALSE))
    {
        gNdsStageMPPassiveLoopAppealGuardCatchCheckCount++;
        return ndsBaseFTCommonCatchCheckInterruptCommon(fighter_gobj);
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCatchActive != FALSE))
    {
        sb32 result;

        gNdsStageMPPassiveLoopCatchCheckCount++;
        result = ndsBaseFTCommonCatchCheckInterruptCommon(fighter_gobj);
        if (result != FALSE)
        {
            gNdsStageMPPassiveLoopCatchSuccessCount++;
        }
        return result;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonAttackS4CheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    return ndsBaseFTCommonAttackS4CheckInterruptCommon(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonAttackS4CheckInterruptTurn(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    return ndsBaseFTCommonAttackS4CheckInterruptTurn(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonAttackHi4CheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    return ndsBaseFTCommonAttackHi4CheckInterruptCommon(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonAttackLw4CheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    return ndsBaseFTCommonAttackLw4CheckInterruptCommon(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonAttackLw4CheckInterruptSquat(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    return ndsBaseFTCommonAttackLw4CheckInterruptSquat(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonAttackS3CheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    return ndsBaseFTCommonAttackS3CheckInterruptCommon(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonAttackHi3CheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    return ndsBaseFTCommonAttackHi3CheckInterruptCommon(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonAttackLw3CheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    return ndsBaseFTCommonAttackLw3CheckInterruptCommon(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonAttack1CheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonAttack1CheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        sb32 result;

        gNdsFighterDashRunAttack1CheckCallCount++;
        result = ndsBaseFTCommonAttack1CheckInterruptCommon(fighter_gobj);
        if (result != FALSE)
        {
            gNdsFighterDashRunAttack1CheckSuccessCount++;
        }
        return result;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

void ftCommonAttack11SetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack11SetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        ndsBaseFTCommonAttack11SetStatus(fighter_gobj);
    }
}

void ftCommonAttack11ProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack11ProcUpdate(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack11UpdateActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) && (fp->player < 2))
        {
            gNdsFighterDashRunAttack11TickMask |=
                1u << ((fp->player * 4u) + 0u);
        }
        ndsBaseFTCommonAttack11ProcUpdate(fighter_gobj);
    }
}

void ftCommonAttack11ProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack11ProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack11InterruptActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) && (fp->player < 2))
        {
            gNdsFighterDashRunAttack11TickMask |=
                1u << ((fp->player * 4u) + 1u);
        }
        ndsBaseFTCommonAttack11ProcInterrupt(fighter_gobj);
    }
}

void ftCommonAttack12SetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack12SetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        ndsBaseFTCommonAttack12SetStatus(fighter_gobj);
    }
}

void ftCommonAttack12ProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack12ProcUpdate(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        ndsBaseFTCommonAttack12ProcUpdate(fighter_gobj);
    }
}

void ftCommonAttack12ProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack12ProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        ndsBaseFTCommonAttack12ProcInterrupt(fighter_gobj);
    }
}

sb32 ftCommonAttack11CheckGoto(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonAttack11CheckGoto(fighter_gobj);
#endif

    (void)fighter_gobj;
    return FALSE;
}

/* The real ones come with battleship_ftcommon_itemuse.c, which imports
 * ftcommonitemshoot.c whole. These no-ops are why a fighter holding a Ray Gun
 * or a Fire Flower pressed A and nothing came out. */
#if !NDS_P2_ITEM_CORE
void ftCommonItemShootSetStatus(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

void ftCommonItemShootAirSetStatus(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}
#endif

/* The real one lives in battleship_ftcommon_get.c wherever the item core is on.
 * This shim answered "no item here" unconditionally, which was correct while
 * nothing could be picked up and is a gameplay hole now that something can. */
#if !NDS_P2_ITEM_CORE
sb32 ftCommonGetCheckInterruptCommon(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return FALSE;
}
#endif

sb32 ftCommonAttack100StartCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonAttack100StartCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        gNdsFighterDashRunAttack100StartCheckCallCount++;
        return ndsBaseFTCommonAttack100StartCheckInterruptCommon(fighter_gobj);
    }
    return FALSE;
}

void ftCommonAttack100StartSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack100StartSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        gNdsFighterDashRunAttack100StartSetStatusCount++;
        ndsBaseFTCommonAttack100StartSetStatus(fighter_gobj);
    }
}

void ftCommonAttack100StartProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack100StartProcUpdate(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        ndsBaseFTCommonAttack100StartProcUpdate(fighter_gobj);
    }
}

void ftCommonAttack100LoopSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack100LoopSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        gNdsFighterDashRunAttack100LoopSetStatusCount++;
        ndsBaseFTCommonAttack100LoopSetStatus(fighter_gobj);
    }
}

void ftCommonAttack100LoopProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack100LoopProcUpdate(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        ndsBaseFTCommonAttack100LoopProcUpdate(fighter_gobj);
    }
}

void ftCommonAttack100LoopProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack100LoopProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        ndsBaseFTCommonAttack100LoopProcInterrupt(fighter_gobj);
    }
}

void ftCommonAttack100EndSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttack100EndSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        ndsBaseFTCommonAttack100EndSetStatus(fighter_gobj);
    }
}

sb32 ftCommonCatchCheckInterruptAttack11(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonCatchCheckInterruptAttack11(fighter_gobj);
#endif

    (void)fighter_gobj;
    return FALSE;
}

u16 ftParamGetMotionCount(void);
u16 ftParamGetStatUpdateCount(void);

void ftParamSetMotionID(FTStruct *fp, s32 motion_attack_id)
{
    if (fp != NULL)
    {
        /* BattleShip ftparam.c:1632-1636. `motion_count` distinguishes two
         * separate uses of the same attack for stale-move accounting; leaving
         * it unchanged makes the four-player stale queue collapse distinct
         * attacks into one old motion. */
        fp->motion_attack_id = motion_attack_id;
        fp->motion_count = ftParamGetMotionCount();
    }
}

void ftParamSetStatUpdate(FTStruct *fp, u16 flags)
{
    if (fp != NULL)
    {
        fp->stat_flags.halfword = flags;
        /* Source ftparam.c:1680-1684 assigns a fresh stat generation here.
         * Keep the DS shadow fields below as mirrors for legacy probes/custom
         * status helpers, but do not substitute them for the source counter. */
        fp->stat_count = ftParamGetStatUpdateCount();
        fp->stat_attack_id = fp->stat_flags.attack_id;
        fp->status_attack_id = fp->stat_flags.attack_id;
        fp->status_is_smash = fp->stat_flags.is_smash_attack;
        fp->status_is_projectile = fp->stat_flags.is_projectile;
    }
}

void ftParamUpdate1PGameAttackStats(FTStruct *fp, u16 flags)
{
    /* BattleShip ftparam.c:1687-1704 is statistics-only and, critically for
     * P2-2, is a complete no-op outside 1P Game.  The old shim rewrote
     * stat_flags to `flags` (normally zero) and incremented stat_count in VS,
     * changing attack identity immediately after ftParamSetStatUpdate had set
     * it.  P2-6 owns the missing 1P bonus counters; do not mutate gameplay
     * state here in VS as a substitute for those counters. */
    (void)fp;
    (void)flags;
}

__attribute__((weak)) GObj *
efManagerKirbyVulcanJabMakeEffect(Vec3f *pos, s32 lr, f32 rotate, f32 vel,
                                  f32 add)
{
    (void)pos;
    (void)lr;
    (void)rotate;
    (void)vel;
    (void)add;
    return NULL;
}

__attribute__((weak)) GObj *
efManagerSamusGrappleBeamGlowMakeEffect(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return NULL;
}

sb32 ftCommonGuardOnCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonGuardOnCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopAppealGuardActive != FALSE))
    {
        sb32 result;

        gNdsStageMPPassiveLoopAppealGuardCheckCount++;
        result = ndsBaseFTCommonGuardOnCheckInterruptCommon(fighter_gobj);
        return result;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunGuardOnActive != FALSE))
    {
        sb32 result;

        gNdsFighterDashRunGuardCheckCallCount++;
        result = ndsBaseFTCommonGuardOnCheckInterruptCommon(fighter_gobj);
        if (result != FALSE)
        {
            gNdsFighterDashRunGuardCheckSuccessCount++;
        }
        return result;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

#if !(NDS_P2_LINK || NDS_P2_ITEM_CORE)
sb32 ftCommonLightThrowCheckInterruptGuardOn(GObj *fighter_gobj)
{
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}
#endif

sb32 ftCommonEscapeCheckInterruptGuard(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonEscapeCheckInterruptGuard(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunEscapeActive != FALSE))
    {
        sb32 result;

        gNdsFighterDashRunEscapeCheckCallCount++;
        result = ndsBaseFTCommonEscapeCheckInterruptGuard(fighter_gobj);
        if (result != FALSE)
        {
            gNdsFighterDashRunEscapeCheckSuccessCount++;
        }
        return result;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

#if !(NDS_P2_LINK || NDS_P2_ITEM_CORE)
sb32 ftCommonLightThrowCheckInterruptEscape(GObj *fighter_gobj)
{
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunEscapeInterruptActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsFighterDashRunEscapeInterruptCount++;
        return FALSE;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}
#endif

sb32 ftCommonGuardCheckInterruptEscape(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonGuardCheckInterruptEscape(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonCatchCheckInterruptGuard(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonCatchCheckInterruptGuard(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonGuardPassCheckInterruptGuard(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonGuardPassCheckInterruptGuard(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonAppealCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonAppealCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopAppealActive != FALSE))
    {
        return ndsBaseFTCommonAppealCheckInterruptCommon(fighter_gobj);
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

void ftKirbySpecialNLoseCopy(GObj *fighter_gobj);
void ftKirbySpecialNDamageCheckLoseCopy(GObj *fighter_gobj);
#if !NDS_P2_KIRBY
/* ftkirbyspecialn.c:945/960 own these once Kirby is built (P2-3f47); the
 * diagnostic globals they publish stay defined in diagnostics_state.c. */
#ifndef FTKIRBY_COPYDAMAGE_LOSECOPY_RANDOM
#define FTKIRBY_COPYDAMAGE_LOSECOPY_RANDOM (1.0F / 12.0F)
#endif

void ftKirbySpecialNLoseCopy(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    gNdsFighterDashRunDamageKirbyCopyBefore =
        (u32)fp->passive_vars.kirby.copy_id;
    (void)func_800269C0_275C0(nSYAudioFGMKirbySpecialNLoseCopy);
    fp->passive_vars.kirby.copy_id = nFTKindKirby;
    fp->passive_vars.kirby.is_ignore_losecopy = FALSE;
    gNdsFighterDashRunDamageKirbyCopyAfter =
        (u32)fp->passive_vars.kirby.copy_id;
    gNdsFighterDashRunDamageKirbyCopyFGM =
        nSYAudioFGMKirbySpecialNLoseCopy;
}

void ftKirbySpecialNDamageCheckLoseCopy(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 mask = 0u;

    if (fp == NULL)
    {
        return;
    }
    if ((fp->fkind == nFTKindKirby) || (fp->fkind == nFTKindNKirby))
    {
        mask |= 0x1u;
    }
    if (fp->passive_vars.kirby.copy_id != nFTKindKirby)
    {
        mask |= 0x2u;
    }
    if (fp->passive_vars.kirby.is_ignore_losecopy == FALSE)
    {
        mask |= 0x4u;
    }
    if ((mask & 0x7u) == 0x7u)
    {
        if (syUtilsRandFloat() < FTKIRBY_COPYDAMAGE_LOSECOPY_RANDOM)
        {
            mask |= 0x8u;
            ftKirbySpecialNLoseCopy(fighter_gobj);
            if (fp->passive_vars.kirby.copy_id == nFTKindKirby)
            {
                mask |= 0x10u;
            }
        }
        mask |= 0x20u;
    }
    gNdsFighterDashRunDamageKirbyCopyMask = mask;
}
#endif /* !NDS_P2_KIRBY */

sb32 ftCommonKneeBendCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonKneeBendCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandInterruptActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPDownWaitLoopDownStandKneeBendCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        return ndsBaseFTCommonKneeBendCheckInterruptCommon(fighter_gobj);
    }
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpWaitProbeActive != FALSE))
    {
        sb32 result;

        gNdsFighterJumpOriginalKneeBendCheckCallCount++;
        result = ndsBaseFTCommonKneeBendCheckInterruptCommon(fighter_gobj);
        if (result != FALSE)
        {
            gNdsFighterJumpOriginalKneeBendCheckSuccessCount++;
            gNdsFighterJumpKneeBendSetStatusCallCount++;
        }
        return result;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonDashCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonDashCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        return ndsBaseFTCommonDashCheckInterruptCommon(fighter_gobj);
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunWaitInterruptActive != FALSE))
    {
        sb32 result;

        gNdsFighterDashRunOriginalDashCheckCallCount++;
        result = ndsBaseFTCommonDashCheckInterruptCommon(fighter_gobj);
        if (result != FALSE)
        {
            gNdsFighterDashRunOriginalDashCheckSuccessCount++;
        }
        return result;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonAttackS4CheckInterruptDash(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    return ndsBaseFTCommonAttackS4CheckInterruptDash(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonEscapeCheckInterruptDash(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonEscapeCheckInterruptDash(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonCatchCheckInterruptDashRun(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonCatchCheckInterruptDashRun(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

void ftCommonAttackDashSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttackDashSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttackDashActive != FALSE))
    {
        ndsBaseFTCommonAttackDashSetStatus(fighter_gobj);
    }
}

sb32 ftCommonAttackDashCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonAttackDashCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttackDashActive != FALSE))
    {
        sb32 result;

        gNdsFighterDashRunAttackDashCheckCallCount++;
        result = ndsBaseFTCommonAttackDashCheckInterruptCommon(fighter_gobj);
        if (result != FALSE)
        {
            gNdsFighterDashRunAttackDashCheckSuccessCount++;
        }
        return result;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

/* The real one comes with battleship_ftcommon_itemthrow.c, which is now gated
 * on the item core as well as Link. */
#if !(NDS_P2_LINK || NDS_P2_ITEM_CORE)
void ftCommonItemThrowSetStatus(GObj *fighter_gobj, s32 status_id)
{
    (void)fighter_gobj;
    (void)status_id;
}
#endif

/* Same, for the Star Rod and Harisen swing (ftcommonitemswing.c:114). */
#if !NDS_P2_ITEM_CORE
void ftCommonItemSwingSetStatus(GObj *fighter_gobj, s32 swing_type)
{
    (void)fighter_gobj;
    (void)swing_type;
}
#endif

sb32 ftCommonGuardOnCheckInterruptDashRun(GObj *fighter_gobj, f32 frame)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonGuardOnCheckInterruptDashRun(fighter_gobj,
                                                       (s32)frame);
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunGuardOnActive != FALSE))
    {
        sb32 result;

        gNdsFighterDashRunGuardCheckCallCount++;
        result = ndsBaseFTCommonGuardOnCheckInterruptDashRun(
            fighter_gobj, (s32)frame);
        if (result != FALSE)
        {
            gNdsFighterDashRunGuardCheckSuccessCount++;
        }
        return result;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonKneeBendCheckInterruptRun(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonKneeBendCheckInterruptRun(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        return ndsBaseFTCommonKneeBendCheckInterruptRun(fighter_gobj);
    }
    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        gNdsFighterJumpDeferredInterruptCheckCount++;
        gNdsFighterMarioFoxJumpLoopDeferredMask |= 1u << 1;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonTurnRunCheckInterruptRun(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonTurnRunCheckInterruptRun(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunTurnRunActive != FALSE))
    {
        sb32 result;

        gNdsFighterDashRunTurnRunCheckCallCount++;
        result = ndsBaseFTCommonTurnRunCheckInterruptRun(fighter_gobj);
        if (result != FALSE)
        {
            gNdsFighterDashRunTurnRunCheckSuccessCount++;
        }
        return result;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

void ftCommonTurnProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonTurnProcUpdate(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        ((sNdsStageTurnLoopUpdateActive != FALSE) ||
         (sNdsStageTurnLoopFinalUpdateActive != FALSE)))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL))
        {
            gNdsStageTurnLoopUnsafeCount++;
            return;
        }
        if (sNdsStageTurnLoopFinalUpdateActive != FALSE)
        {
            gNdsStageTurnLoopFinalUpdateTickCount++;
        }
        else
        {
            gNdsStageTurnLoopUpdateTickCount++;
        }
        if (fp->motion_vars.flags.flag1 != 0)
        {
            fp->motion_vars.flags.flag1 = 0;
            fp->status_vars.common.turn.is_allow_turn_direction = TRUE;
            fp->status_vars.common.turn.is_disable_sa_interrupts = TRUE;
            fp->lr = -fp->lr;
            fp->physics.vel_ground.x = -fp->physics.vel_ground.x;
        }
        if (fighter_gobj->anim_frame <= 0.0F)
        {
            ftCommonWaitSetStatus(fighter_gobj);
        }
        return;
    }
}

void ftCommonTurnProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonTurnProcInterrupt(fighter_gobj);
    return;
#endif

    if (ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE)
    {
        ndsBaseFTCommonTurnProcInterrupt(fighter_gobj);
    }
}

void ftCommonTurnSetStatus(GObj *fighter_gobj, s32 lr_dash)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonTurnSetStatus(fighter_gobj, lr_dash);
    return;
#endif

    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopSetStatusActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageTurnLoopUnsafeCount++;
            return;
        }
        ndsBaseFTCommonTurnSetStatus(fighter_gobj, lr_dash);
        fp->motion_vars.flags.flag1 = 0;
        fp->status_vars.common.turn.is_allow_turn_direction = FALSE;
        fp->status_vars.common.turn.is_disable_sa_interrupts = FALSE;
        fp->status_vars.common.turn.button_mask = 0u;
        fp->status_vars.common.turn.lr_dash = lr_dash;
        fp->status_vars.common.turn.attacks4_buffer =
            (lr_dash != 0) ? 0 : 256;
        fp->status_vars.common.turn.lr_turn = -fp->lr;
        return;
    }
    (void)fighter_gobj;
    (void)lr_dash;
}

void ftCommonTurnSetStatusCenter(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonTurnSetStatusCenter(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopSetStatusActive != FALSE))
    {
        ndsBaseFTCommonTurnSetStatusCenter(fighter_gobj);
    }
}

void ftCommonTurnSetStatusInvertLR(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonTurnSetStatusInvertLR(fighter_gobj);
    return;
#endif

    FTStruct *fp;

    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopSetStatusActive != FALSE))
    {
        ndsBaseFTCommonTurnSetStatusInvertLR(fighter_gobj);
        return;
    }

    fp = ftGetStruct(fighter_gobj);

    if (fp != NULL)
    {
        fp->lr = -fp->lr;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        gNdsFighterDashRunUnexpectedStatusCount++;
    }
}

sb32 ftCommonTurnCheckInputSuccess(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonTurnCheckInputSuccess(fighter_gobj);
#endif

    if (ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE)
    {
        return ndsBaseFTCommonTurnCheckInputSuccess(fighter_gobj);
    }
    return FALSE;
}

void ftParamSetStickLR(FTStruct *fp)
{
    if (fp != NULL)
    {
        /* BattleShip ftparam.c:251-254 deliberately treats centered X as
         * facing right. Several common attack/special entry paths call this
         * helper directly, so suppressing the zero case was a DS-only state
         * rule that could preserve an old left-facing direction. */
        fp->lr = (fp->input.pl.stick_range.x >= 0) ? 1 : -1;
    }
}

sb32 ftCommonSquatCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonSquatCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPPassInputLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassInputLoopInputActive != FALSE))
    {
        return ndsBaseFTCommonSquatCheckInterruptCommon(fighter_gobj);
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonPassCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonPassCheckInterruptCommon(fighter_gobj);
#endif

    sb32 result;

    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandInterruptActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPDownWaitLoopDownStandPassCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPPassInputLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassInputLoopInputActive != FALSE))
    {
        gNdsStageMPPassInputLoopCheckCallCount++;
        result = ndsBaseFTCommonPassCheckInterruptCommon(fighter_gobj);
        if (result != FALSE)
        {
            gNdsStageMPPassInputLoopCheckSuccessCount++;
        }
        return result;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonPassCheckInterruptSquat(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonPassCheckInterruptSquat(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPPassInputLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassInputLoopInputActive != FALSE))
    {
        return ndsBaseFTCommonPassCheckInterruptSquat(fighter_gobj);
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonDokanStartCheckInterruptCommon(GObj *fighter_gobj)
{
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandInterruptActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPDownWaitLoopDownStandDokanCheckCount++;
        return FALSE;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonSquatWaitCheckInterruptLanding(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonSquatWaitCheckInterruptLanding(fighter_gobj);
#endif

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

/* Owned by battleship_ftcommon_hammer.c wherever the item core is on -- that
 * TU imports the whole of ftcommonhammerfall.c, which is where this check
 * lives. */
#if !NDS_P2_ITEM_CORE
sb32 ftCommonHammerFallCheckInterruptCommon(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return FALSE;
}
#endif

sb32 ftCommonTurnCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonTurnCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopSetStatusActive != FALSE))
    {
        gNdsStageTurnLoopCheckCallCount++;
        if (ftCommonTurnCheckInputSuccess(fighter_gobj) != FALSE)
        {
            ftCommonTurnSetStatus(fighter_gobj, 0);
            gNdsStageTurnLoopCheckSuccessCount++;
            gNdsStageTurnLoopSetStatusCount++;
            return TRUE;
        }
        return FALSE;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

void ftAnimEndSetWait(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    battleship_ftAnimEndSetWait(fighter_gobj);
    return;
#endif
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttackDashUpdateActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        u32 slot = ((fp != NULL) && (fp->player < 2)) ? fp->player : 2u;

        if (slot < 2u)
        {
            gNdsFighterDashRunAttackDashTickMask |= 1u << (slot * 3u);
        }
        (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopAttackUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopAttackUpdateTickCount++;
        (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollForwardUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollForwardUpdateTickCount++;
        (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollBackUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollBackUpdateTickCount++;
        (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopDownStandUpdateTickCount++;
        (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandBActive != FALSE))
    {
        (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveStandUpdateTickCount++;
        (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveUpdateTickCount++;
        (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCatchUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopCatchUpdateTickCount++;
        (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassiveStandUpdateTickCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassiveUpdateTickCount++;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopRunBrakeEndActive != FALSE))
    {
        gNdsFighterProcessLoopRunBrakeEndCount++;
        (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopLandingEndActive != FALSE))
    {
        gNdsFighterProcessLoopLandingEndCount++;
        (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingEndActive != FALSE))
    {
        gNdsFighterLandingEndCallCount++;
    }
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpRunBrakeEndActive != FALSE))
    {
        gNdsFighterJumpRunBrakeEndCallCount++;
    }
    (void)ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
}

void ftAnimEndSetFall(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    battleship_ftAnimEndSetFall(fighter_gobj);
    return;
#endif
    if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
        (sNdsFighterJumpAttackAirRefreshActive != FALSE))
    {
        (void)fighter_gobj;
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopJumpAnimEndActive != FALSE))
    {
        gNdsFighterProcessLoopJumpAnimEndCount++;
        ftCommonFallSetStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingJumpAnimEndActive != FALSE))
    {
        gNdsFighterLandingJumpAnimEndCallCount++;
        ftCommonFallSetStatus(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        gNdsFighterJumpFallDeferredCount++;
        gNdsFighterMarioFoxJumpLoopDeferredMask |= 1u << 7;
    }
}

static void ndsFTCommonCliffClimbQuick2SetStatusBounded(GObj *fighter_gobj)
{
    ftCommonCliffCommon2UpdateCollData(fighter_gobj);
    ftMainSetStatus(fighter_gobj, nFTCommonStatusCliffClimbQuick2, 0.0F, 1.0F,
        FTSTATUS_PRESERVE_NONE);
    ftCommonCliffCommon2InitStatusVars(fighter_gobj);
}

static void ndsFTCommonCliffWaitApplyOriginalPostStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    fp->status_vars.common.cliffwait.is_allow_interrupt = FALSE;
    fp->status_vars.common.cliffwait.fall_wait =
        (fp->percent_damage < FTCOMMON_CLIFF_DAMAGE_HIGH) ?
            FTCOMMON_CLIFF_FALL_WAIT_DAMAGE_LOW :
            FTCOMMON_CLIFF_FALL_WAIT_DAMAGE_HIGH;
    fp->is_cliff_hold = TRUE;
}

sb32 ftAnimEndCheckSetStatus(GObj *fighter_gobj, void (*proc_status)(GObj*))
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return battleship_ftAnimEndCheckSetStatus(fighter_gobj, proc_status);
#endif
    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffTickFloorLoopStatusActive != FALSE))
    {
        gNdsStageMPCliffTickFloorLoopOttottoAnimEndCheckCount++;
        if ((proc_status == ftCommonOttottoWaitSetStatus) ||
            (proc_status == ndsBaseFTCommonOttottoWaitSetStatus))
        {
            return FALSE;
        }
    }
    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffLiveLoopWaitUpdateActive != FALSE) &&
        (proc_status == ftCommonCliffWaitSetStatus))
    {
        gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 3;
        if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F))
        {
            sNdsStageMPCliffLiveLoopSetStatusActive = TRUE;
            proc_status(fighter_gobj);
            ndsFTCommonCliffWaitApplyOriginalPostStatus(fighter_gobj);
            sNdsStageMPCliffLiveLoopSetStatusActive = FALSE;
            return TRUE;
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffLiveLoopQuick1UpdateActive != FALSE) &&
        (proc_status == ftCommonCliffClimbQuick2SetStatus))
    {
        gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 4;
        if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F))
        {
            sNdsStageMPCliffLiveLoopSetStatusActive = TRUE;
            ndsFTCommonCliffClimbQuick2SetStatusBounded(fighter_gobj);
            sNdsStageMPCliffLiveLoopSetStatusActive = FALSE;
            return TRUE;
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffLiveLoopCommon2UpdateActive != FALSE) &&
        (proc_status == mpCommonSetFighterWaitOrFall))
    {
        gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 5;
        if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F))
        {
            sNdsStageMPCliffLiveLoopSetStatusActive = TRUE;
            proc_status(fighter_gobj);
            sNdsStageMPCliffLiveLoopSetStatusActive = FALSE;
            return TRUE;
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitFloorLoopUpdateActive != FALSE) &&
        (proc_status == ftCommonCliffWaitSetStatus))
    {
        gNdsStageMPCliffWaitFloorLoopAnimEndCheckCount++;
        if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F))
        {
            gNdsStageMPCliffWaitFloorLoopAnimEndSetStatusCount++;
            gNdsStageMPCliffWaitFloorLoopCliffWaitSetStatusCount++;
            sNdsStageMPCliffWaitFloorLoopSetStatusActive = TRUE;
            proc_status(fighter_gobj);
            ndsFTCommonCliffWaitApplyOriginalPostStatus(fighter_gobj);
            sNdsStageMPCliffWaitFloorLoopSetStatusActive = FALSE;
            return TRUE;
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffAttackActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackActionLoopAnimEndActive != FALSE) &&
        (proc_status == ftCommonCliffAttackQuick2SetStatus))
    {
        gNdsStageMPCliffAttackActionLoopAnimEndCheckCount++;
        if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F))
        {
            sNdsStageMPCliffAttackActionLoopSetStatusActive = TRUE;
            proc_status(fighter_gobj);
            sNdsStageMPCliffAttackActionLoopSetStatusActive = FALSE;
            return TRUE;
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbActionLoopAnimEndActive != FALSE) &&
        (proc_status == ftCommonCliffClimbQuick2SetStatus))
    {
        gNdsStageMPCliffClimbActionLoopAnimEndCheckCount++;
        if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F))
        {
            sNdsStageMPCliffClimbActionLoopSetStatusActive = TRUE;
            ndsFTCommonCliffClimbQuick2SetStatusBounded(fighter_gobj);
            sNdsStageMPCliffClimbActionLoopSetStatusActive = FALSE;
            return TRUE;
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopAnimEndActive != FALSE) &&
        (proc_status == ftCommonCliffEscapeQuick2SetStatus))
    {
        gNdsStageMPCliffEscapeActionLoopAnimEndCheckCount++;
        if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F))
        {
            sNdsStageMPCliffEscapeActionLoopSetStatusActive = TRUE;
            proc_status(fighter_gobj);
            sNdsStageMPCliffEscapeActionLoopSetStatusActive = FALSE;
            return TRUE;
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffCommon2LoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffCommon2LoopUpdateActive != FALSE) &&
        (proc_status == mpCommonSetFighterWaitOrFall))
    {
        gNdsStageMPCliffCommon2LoopAnimEndCheckCount++;
        if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F))
        {
            gNdsStageMPCliffCommon2LoopWaitOrFallCallCount++;
            proc_status(fighter_gobj);
            return TRUE;
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbCommon2LoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbCommon2LoopUpdateActive != FALSE) &&
        (proc_status == mpCommonSetFighterWaitOrFall))
    {
        gNdsStageMPCliffClimbCommon2LoopAnimEndCheckCount++;
        if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F))
        {
            gNdsStageMPCliffClimbCommon2LoopWaitOrFallCallCount++;
            proc_status(fighter_gobj);
            return TRUE;
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFinishLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFinishLoopUpdateActive != FALSE) &&
        (proc_status == mpCommonSetFighterWaitOrFall))
    {
        gNdsStageMPCliffClimbFinishLoopAnimEndCheckCount++;
        if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F))
        {
            gNdsStageMPCliffClimbFinishLoopWaitOrFallCallCount++;
            proc_status(fighter_gobj);
            return TRUE;
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeCommon2LoopUpdateActive != FALSE) &&
        (proc_status == mpCommonSetFighterWaitOrFall))
    {
        gNdsStageMPCliffEscapeCommon2LoopAnimEndCheckCount++;
        if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F))
        {
            gNdsStageMPCliffEscapeCommon2LoopWaitOrFallCallCount++;
            proc_status(fighter_gobj);
            return TRUE;
        }
        return FALSE;
    }
    if ((fighter_gobj != NULL) && (fighter_gobj->anim_frame <= 0.0F) &&
        (proc_status != NULL))
    {
        proc_status(fighter_gobj);
        return TRUE;
    }
    return FALSE;
}

void ftCommonDashProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDashProcUpdate(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopUpdateActive != FALSE))
    {
        ndsBaseFTCommonDashProcUpdate(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        ndsBaseFTCommonDashProcUpdate(fighter_gobj);
    }
}

void ftCommonDashProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDashProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        ndsBaseFTCommonDashProcInterrupt(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        ndsBaseFTCommonDashProcInterrupt(fighter_gobj);
    }
}

void ftCommonDashProcPhysics(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDashProcPhysics(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopPhysicsActive != FALSE))
    {
        ndsBaseFTCommonDashProcPhysics(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        ndsBaseFTCommonDashProcPhysics(fighter_gobj);
    }
}

void ftCommonDashProcMap(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDashProcMap(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopMapActive != FALSE))
    {
        ndsBaseFTCommonDashProcMap(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        ndsBaseFTCommonDashProcMap(fighter_gobj);
    }
}

void ftCommonDashSetStatus(GObj *fighter_gobj, u32 flag)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDashSetStatus(fighter_gobj, flag);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        ndsBaseFTCommonDashSetStatus(fighter_gobj, flag);
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        gNdsFighterDashRunDashSetStatusCount++;
        ndsBaseFTCommonDashSetStatus(fighter_gobj, flag);
    }
}

sb32 ftCommonDashCheckTurn(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonDashCheckTurn(fighter_gobj);
#endif

    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        return ndsBaseFTCommonDashCheckTurn(fighter_gobj);
    }
    return FALSE;
}

void ftCommonRunProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonRunProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        ndsBaseFTCommonRunProcInterrupt(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        ndsBaseFTCommonRunProcInterrupt(fighter_gobj);
    }
}

void ftCommonRunSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonRunSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        ndsBaseFTCommonRunSetStatus(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        gNdsFighterDashRunRunSetStatusCount++;
        ndsBaseFTCommonRunSetStatus(fighter_gobj);
    }
}

sb32 ftCommonRunCheckInterruptDash(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonRunCheckInterruptDash(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        return ndsBaseFTCommonRunCheckInterruptDash(fighter_gobj);
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        return ndsBaseFTCommonRunCheckInterruptDash(fighter_gobj);
    }
    return FALSE;
}

void ftCommonRunBrakeProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonRunBrakeProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        ndsBaseFTCommonRunBrakeProcInterrupt(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        ndsBaseFTCommonRunBrakeProcInterrupt(fighter_gobj);
    }
}

void ftCommonRunBrakeProcPhysics(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonRunBrakeProcPhysics(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopPhysicsActive != FALSE))
    {
        ndsBaseFTCommonRunBrakeProcPhysics(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        ndsBaseFTCommonRunBrakeProcPhysics(fighter_gobj);
    }
}

void ftCommonRunBrakeSetStatus(GObj *fighter_gobj, u32 flag)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonRunBrakeSetStatus(fighter_gobj, flag);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        ndsBaseFTCommonRunBrakeSetStatus(fighter_gobj, flag);
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        gNdsFighterDashRunRunBrakeSetStatusCount++;
        ndsBaseFTCommonRunBrakeSetStatus(fighter_gobj, flag);
    }
}

sb32 ftCommonRunBrakeCheckInterruptRun(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonRunBrakeCheckInterruptRun(fighter_gobj);
#endif

    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        return ndsBaseFTCommonRunBrakeCheckInterruptRun(fighter_gobj);
    }
    return FALSE;
}

sb32 ftCommonRunBrakeCheckInterruptTurnRun(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonRunBrakeCheckInterruptTurnRun(fighter_gobj);
#endif

    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        return ndsBaseFTCommonRunBrakeCheckInterruptTurnRun(fighter_gobj);
    }
    return FALSE;
}

void ftCommonKneeBendProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonKneeBendProcUpdate(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopUpdateActive != FALSE))
    {
        ndsBaseFTCommonKneeBendProcUpdate(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpKneeBendUpdateActive != FALSE))
    {
        gNdsFighterJumpKneeBendUpdateCallCount++;
        ndsBaseFTCommonKneeBendProcUpdate(fighter_gobj);
    }
}

void ftCommonKneeBendProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonKneeBendProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        ndsBaseFTCommonKneeBendProcInterrupt(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpKneeBendInterruptActive != FALSE))
    {
        gNdsFighterJumpKneeBendInterruptCallCount++;
        ndsBaseFTCommonKneeBendProcInterrupt(fighter_gobj);
    }
}

void ftCommonKneeBendSetStatusParam(GObj *fighter_gobj, s32 status_id,
                                    s32 input_source)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonKneeBendSetStatusParam(fighter_gobj, status_id, input_source);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE) &&
        (status_id == nFTCommonStatusKneeBend))
    {
        ndsBaseFTCommonKneeBendSetStatusParam(fighter_gobj, status_id,
                                              input_source);
        return;
    }
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (status_id == nFTCommonStatusKneeBend))
    {
        gNdsFighterJumpKneeBendSetStatusCallCount++;
        ndsBaseFTCommonKneeBendSetStatusParam(fighter_gobj, status_id,
                                              input_source);
    }
}

void ftCommonKneeBendSetStatus(GObj *fighter_gobj, s32 input_source)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonKneeBendSetStatus(fighter_gobj, input_source);
    return;
#endif

    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        ndsBaseFTCommonKneeBendSetStatus(fighter_gobj, input_source);
    }
}

void ftCommonGuardKneeBendSetStatus(GObj *fighter_gobj, s32 input_source)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonGuardKneeBendSetStatus(fighter_gobj, input_source);
    return;
#endif

    (void)fighter_gobj;
    (void)input_source;
    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        gNdsFighterJumpDeniedStatusCount++;
    }
}

sb32 ftCommonKneeBendCheckButtonTap(FTStruct *fp)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonKneeBendCheckButtonTap(fp);
#endif

    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        return ndsBaseFTCommonKneeBendCheckButtonTap(fp);
    }
    return FALSE;
}

s32 ftCommonKneeBendGetInputTypeCommon(FTStruct *fp)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonKneeBendGetInputTypeCommon(fp);
#endif

    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        return ndsBaseFTCommonKneeBendGetInputTypeCommon(fp);
    }
    return FTCOMMON_KNEEBEND_INPUT_TYPE_NONE;
}

s32 ftCommonKneeBendGetInputTypeRun(FTStruct *fp)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonKneeBendGetInputTypeRun(fp);
#endif

    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        return ndsBaseFTCommonKneeBendGetInputTypeRun(fp);
    }
    return FTCOMMON_KNEEBEND_INPUT_TYPE_NONE;
}

sb32 ftCommonGuardKneeBendCheckInterruptGuard(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonGuardKneeBendCheckInterruptGuard(fighter_gobj);
#endif

    (void)fighter_gobj;
    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        gNdsFighterJumpDeferredInterruptCheckCount++;
    }
    return FALSE;
}

sb32 ftCommonAttackHi4CheckInterruptKneeBend(GObj *fighter_gobj)
{
    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        gNdsFighterJumpAttackHi4KneeBendCheckCount++;
        gNdsFighterJumpDeferredInterruptCheckCount++;
        gNdsFighterMarioFoxJumpLoopDeferredMask |= 1u << 2;
        return FALSE;
    }
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    return ndsBaseFTCommonAttackHi4CheckInterruptKneeBend(fighter_gobj);
#else
    (void)fighter_gobj;
    return FALSE;
#endif
}

/* Owned by battleship_ftcommon_hammer.c wherever the item core is on -- that
 * TU imports the whole of ftcommonhammerkneebend.c. */
#if !NDS_P2_ITEM_CORE
sb32 ftCommonHammerKneeBendCheckInterruptCommon(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        gNdsFighterJumpHammerKneeBendCheckCount++;
        gNdsFighterJumpDeferredInterruptCheckCount++;
    }
    return FALSE;
}
#endif

void ftCommonJumpProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonJumpProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        ndsBaseFTCommonJumpProcInterrupt(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpAirInterruptActive != FALSE))
    {
        gNdsFighterJumpAirInterruptCallCount++;
        ndsBaseFTCommonJumpProcInterrupt(fighter_gobj);
    }
}

void ftCommonJumpGetJumpForceButton(s32 stick_range_x, s32 *jump_vel_x,
                                    s32 *jump_vel_y, sb32 is_shorthop)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonJumpGetJumpForceButton(stick_range_x, jump_vel_x, jump_vel_y, is_shorthop);
    return;
#endif

    ndsBaseFTCommonJumpGetJumpForceButton(stick_range_x, jump_vel_x,
                                          jump_vel_y, is_shorthop);
}

static void ndsFTCommonJumpSyncVelocityAfterStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr;
    s32 vel_x;
    s32 vel_y;

    if (fp == NULL)
    {
        return;
    }
    attr = fp->attr;
    if (attr == NULL)
    {
        return;
    }
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (fp->physics.vel_air.y <= 0.0F))
    {
        switch (fp->status_vars.common.kneebend.input_source)
        {
        case FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON:
            ndsBaseFTCommonJumpGetJumpForceButton(
                fp->input.pl.stick_range.x, &vel_x, &vel_y,
                fp->status_vars.common.kneebend.is_shorthop);
            break;
        case FTCOMMON_KNEEBEND_INPUT_TYPE_STICK:
        default:
            vel_x = fp->input.pl.stick_range.x;
            vel_y = fp->status_vars.common.kneebend.jump_force;
            if (vel_y < FTCOMMON_KNEEBEND_STICK_RANGE_MIN)
            {
                vel_y = FTCOMMON_KNEEBEND_STICK_RANGE_MIN;
            }
            break;
        }
        fp->physics.vel_air.y =
            (vel_y * attr->jump_height_mul) + attr->jump_height_base;
        fp->physics.vel_air.x = vel_x * attr->jump_vel_x;
    }
    ndsFighterSyncPhysicsToLegacyVel(fp);
}

void ftCommonJumpSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonJumpSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopUpdateActive != FALSE))
    {
        ndsBaseFTCommonJumpSetStatus(fighter_gobj);
        ndsFTCommonJumpSyncVelocityAfterStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpSetStatusActive != FALSE))
    {
        gNdsFighterJumpSetStatusCallCount++;
        ndsBaseFTCommonJumpSetStatus(fighter_gobj);
        ndsFTCommonJumpSyncVelocityAfterStatus(fighter_gobj);
    }
}

void ftCommonFallProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonFallProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageInterruptActive != FALSE))
    {
        sNdsFighterDashRunDamageCommonFallInterruptCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffTickFloorLoopStatusActive != FALSE))
    {
        gNdsStageMPCliffTickFloorLoopFallInterruptCallCount++;
        ndsBaseFTCommonFallProcInterrupt(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        ndsBaseFTCommonFallProcInterrupt(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingFallInterruptActive != FALSE))
    {
        ndsBaseFTCommonFallProcInterrupt(fighter_gobj);
    }
}

void ftCommonFallSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonFallSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowDeadResultActive != FALSE))
    {
        ndsBaseFTCommonFallSetStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffLiveLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 9;
        ndsBaseFTCommonFallSetStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffStatusFloorLoopStatusActive != FALSE))
    {
        gNdsStageMPCliffStatusFloorLoopFallSetStatusCallCount++;
        ndsBaseFTCommonFallSetStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopInterruptActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        gNdsStageMPCliffClimbFloorLoopFallStatusSetCount++;
        if ((fp == NULL) || (fp->attr == NULL) ||
            (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
            return;
        }
        if (fp->attr->jumps_max <= fp->jumps_used)
        {
            fp->attr->jumps_max = fp->jumps_used + 1;
        }
        if (fp->ga == nMPKineticsGround)
        {
            mpCommonSetFighterAir(fp);
        }
        ftMainSetStatus(fighter_gobj, nFTCommonStatusFall, 0.0F, 1.0F,
                        FTSTATUS_PRESERVE_FASTFALL);
        ftPhysicsClampAirVelXMax(fp);
        fp->is_special_interrupt = TRUE;
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopJumpAnimEndActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) && (fp->attr != NULL) &&
            (fp->attr->jumps_max <= fp->jumps_used))
        {
            fp->attr->jumps_max = fp->jumps_used + 1;
        }
        ndsBaseFTCommonFallSetStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingJumpAnimEndActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) && (fp->attr != NULL) &&
            (fp->attr->jumps_max <= fp->jumps_used))
        {
            fp->attr->jumps_max = fp->jumps_used + 1;
        }
        gNdsFighterLandingFallSetStatusCallCount++;
        ndsBaseFTCommonFallSetStatus(fighter_gobj);
    }
}

void ftCommonOttottoProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonOttottoProcUpdate(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffTickFloorLoopStatusActive != FALSE))
    {
        gNdsStageMPCliffTickFloorLoopOttottoUpdateCallCount++;
        ndsBaseFTCommonOttottoProcUpdate(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffStatusFloorLoopStatusActive != FALSE))
    {
        ndsBaseFTCommonOttottoProcUpdate(fighter_gobj);
    }
}

void ftCommonOttottoProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonOttottoProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffTickFloorLoopStatusActive != FALSE))
    {
        gNdsStageMPCliffTickFloorLoopOttottoInterruptCallCount++;
        ndsBaseFTCommonOttottoProcInterrupt(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffStatusFloorLoopStatusActive != FALSE))
    {
        ndsBaseFTCommonOttottoProcInterrupt(fighter_gobj);
    }
}

void ftCommonOttottoProcMap(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonOttottoProcMap(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffTickFloorLoopStatusActive != FALSE))
    {
        gNdsStageMPCliffTickFloorLoopOttottoMapCallCount++;
        ndsBaseFTCommonOttottoProcMap(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffStatusFloorLoopStatusActive != FALSE))
    {
        ndsBaseFTCommonOttottoProcMap(fighter_gobj);
    }
}

void ftCommonOttottoWaitSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonOttottoWaitSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffTickFloorLoopStatusActive != FALSE))
    {
        (void)fighter_gobj;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffStatusFloorLoopStatusActive != FALSE))
    {
        ndsBaseFTCommonOttottoWaitSetStatus(fighter_gobj);
    }
}

void ftCommonOttottoSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonOttottoSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffStatusFloorLoopStatusActive != FALSE))
    {
        gNdsStageMPCliffStatusFloorLoopOttottoSetStatusCallCount++;
        ndsBaseFTCommonOttottoSetStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE) &&
        (gNdsStageMPUpdateFloorLoopPrepared != 0u))
    {
        gNdsStageMPUpdateFloorLoopOttottoDeniedCount++;
    }
}

/* Owned by battleship_ftcommon_hammer.c wherever the item core is on -- that
 * TU imports the whole of ftcommonhammerfall.c, which is where both of these
 * live. */
#if !NDS_P2_ITEM_CORE
void ftCommonHammerFallSetStatus(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if (ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE)
    {
        gNdsFighterLandingDeniedStatusCount++;
    }
}

void ftCommonHammerFallProcInterrupt(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if (sNdsFighterDashRunDamageHammerCheckActive != FALSE)
    {
        sNdsFighterDashRunDamageHammerAirCount++;
    }
}
#endif

void ftCommonLandingProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonLandingProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopInterruptActive != FALSE))
    {
        ndsBaseFTCommonLandingProcInterrupt(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingProcInterruptActive != FALSE))
    {
        ndsBaseFTCommonLandingProcInterrupt(fighter_gobj);
    }
}

void ftCommonLandingSetStatusParam(GObj *fighter_gobj, s32 status_id,
                                   sb32 is_allow_interrupt,
                                   f32 anim_speed)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonLandingSetStatusParam(fighter_gobj, status_id, is_allow_interrupt, anim_speed);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallLandFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPFallLandFloorLoopLandingParamCallCount++;
        ndsBaseFTCommonLandingSetStatusParam(fighter_gobj, status_id,
                                             is_allow_interrupt, anim_speed);
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopMapActive != FALSE))
    {
        ndsBaseFTCommonLandingSetStatusParam(fighter_gobj, status_id,
                                             is_allow_interrupt, anim_speed);
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingSetStatusActive != FALSE))
    {
        gNdsFighterLandingSetStatusCallCount++;
        ndsBaseFTCommonLandingSetStatusParam(fighter_gobj, status_id,
                                             is_allow_interrupt, anim_speed);
        return;
    }
    if (ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE)
    {
        gNdsFighterLandingDeniedStatusCount++;
    }
}

void ftCommonLandingSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonLandingSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
        (sNdsFighterJumpAttackAirMapLandingActive != FALSE))
    {
        gNdsFighterJumpAttackAirMapLandingMask |= 1u << 8u;
        ndsBaseFTCommonLandingSetStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallLandFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPFallLandFloorLoopLandingSetStatusCallCount++;
        ndsBaseFTCommonLandingSetStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopMapActive != FALSE))
    {
        ndsBaseFTCommonLandingSetStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingSetStatusActive != FALSE))
    {
        gNdsFighterLandingSetStatusCallCount++;
        ndsBaseFTCommonLandingSetStatus(fighter_gobj);
    }
}

void ftCommonLandingAirNullSetStatus(GObj *fighter_gobj, f32 anim_speed)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonLandingAirNullSetStatus(fighter_gobj, anim_speed);
    return;
#endif

    if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
        (sNdsFighterJumpAttackAirMapLandingActive != FALSE))
    {
        gNdsFighterJumpAttackAirMapLandingMask |= (1u << 1u) | (1u << 5u);
        ndsBaseFTCommonLandingAirNullSetStatus(fighter_gobj, anim_speed);
        return;
    }
    if (ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE)
    {
        gNdsFighterLandingDeniedStatusCount++;
    }
}

void ftCommonLandingAirSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonLandingAirSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
        (sNdsFighterJumpAttackAirMapLandingActive != FALSE))
    {
        gNdsFighterJumpAttackAirMapLandingMask |= 1u << 1u;
    }
    ndsBaseFTCommonLandingAirSetStatus(fighter_gobj);
}

void ftCommonLandingFallSpecialSetStatus(GObj *fighter_gobj,
                                         sb32 is_allow_interrupt,
                                         f32 anim_speed)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonLandingFallSpecialSetStatus(fighter_gobj, is_allow_interrupt, anim_speed);
    return;
#endif

    (void)fighter_gobj;
    (void)is_allow_interrupt;
    (void)anim_speed;
    if (ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE)
    {
        gNdsFighterLandingDeniedStatusCount++;
    }
}

sb32 ftCommonAttackAirCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    sb32 result = ndsBaseFTCommonAttackAirCheckInterruptCommon(fighter_gobj);
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    if ((gNdsFighterNaturalMovesetPhase == 9u) ||
        (gNdsFighterNaturalMovesetPhase == 10u))
    {
        if (result != FALSE)
        {
            gNdsFighterNaturalMovesetAerialFrames++;
        }
    }
#endif
    return result;
#else
    sb32 result;

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageFallSourceInterruptActive != FALSE))
    {
        sNdsFighterDashRunDamageFallAttackAirCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDamageFallAttackAirCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffTickFloorLoopStatusActive != FALSE))
    {
        gNdsStageMPCliffTickFloorLoopFallAttackAirCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingFallInterruptActive != FALSE))
    {
        gNdsFighterLandingDeferredInterruptCheckCount++;
        gNdsFighterMarioFoxLandingLoopDeferredMask |= 1u << 1;
        return FALSE;
    }
    if (ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE)
    {
        gNdsFighterJumpAttackAirCheckCount++;
        if (sNdsFighterJumpAttackAirActive != FALSE)
        {
            result = ndsBaseFTCommonAttackAirCheckInterruptCommon(
                fighter_gobj);
            if (result != FALSE)
            {
                gNdsFighterJumpAttackAirCheckSuccessCount++;
            }
            return result;
        }
        gNdsFighterJumpDeferredInterruptCheckCount++;
        gNdsFighterMarioFoxJumpLoopDeferredMask |= 1u << 4;
    }
    return FALSE;
#endif
}

void ftCommonAttackAirProcMap(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonAttackAirProcMap(fighter_gobj);
    return;
#endif

    ndsBaseFTCommonAttackAirProcMap(fighter_gobj);
}

sb32 ftCommonJumpAerialCheckInterruptCommon(GObj *fighter_gobj)
{
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageFallSourceInterruptActive != FALSE))
    {
        sNdsFighterDashRunDamageFallJumpAerialCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDamageFallJumpAerialCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffTickFloorLoopStatusActive != FALSE))
    {
        gNdsStageMPCliffTickFloorLoopFallJumpAerialCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingFallInterruptActive != FALSE))
    {
        gNdsFighterLandingDeferredInterruptCheckCount++;
        gNdsFighterMarioFoxLandingLoopDeferredMask |= 1u << 2;
        return FALSE;
    }
    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        gNdsFighterJumpAerialCheckCount++;
        gNdsFighterJumpDeferredInterruptCheckCount++;
        gNdsFighterMarioFoxJumpLoopDeferredMask |= 1u << 5;
        return FALSE;
    }
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    return ndsBaseFTCommonJumpAerialCheckInterruptCommon(fighter_gobj);
#else
    (void)fighter_gobj;
    return FALSE;
#endif
}

#define NDS_DAMAGE_LOSEGRIP_SELECT 0x1u
#define NDS_DAMAGE_LOSEGRIP_LINKS 0x2u
#define NDS_DAMAGE_LOSEGRIP_POSITION 0x4u
#define NDS_DAMAGE_LOSEGRIP_COLLISION 0x8u
#define NDS_DAMAGE_LOSEGRIP_SETAIR 0x10u
#define NDS_DAMAGE_LOSEGRIP_LINK_CLEAR 0x20u
#define NDS_DAMAGE_LOSEGRIP_ORIGINAL 0x40u

void mpCommonSetFighterAir(FTStruct *fp)
{
    if (fp != NULL)
    {
        fp->ga = nMPKineticsAir;
        fp->physics.vel_air.z = 0.0F;
        if (fp->joints[nFTPartsJointTopN] != NULL)
        {
            fp->joints[nFTPartsJointTopN]->translate.vec.f.z = 0.0F;
        }
        fp->jumps_used = 1;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        gNdsFighterDashRunDamageLoseGripSetAirCount++;
        gNdsFighterDashRunDamageLoseGripMask |= NDS_DAMAGE_LOSEGRIP_SETAIR;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowDeadResultActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowDeadResultSetAirCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopCliffCatchAirSetCount++;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopUpdateActive != FALSE))
    {
        gNdsFighterProcessLoopSetAirCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffStatusFloorLoopStatusActive != FALSE))
    {
        gNdsStageMPCliffStatusFloorLoopAirSetCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassInputLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassInputLoopStatusActive != FALSE))
    {
        gNdsStageMPPassInputLoopSetAirCount++;
    }
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpSetStatusActive != FALSE))
    {
        gNdsFighterJumpSetAirCallCount++;
    }
}

void mpCommonSetFighterProjectFloor(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *root = DObjGetStruct(fighter_gobj);

    if ((fp == NULL) || (root == NULL))
    {
        return;
    }
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    if (fp->coll_data.p_map_coll == NULL)
    {
        fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    }
    mpProcessSetCollProjectFloorID(&fp->coll_data);
}

void ftPhysicsApplyGravityClampTVel(FTStruct *fp, f32 gravity, f32 tvel)
{
    if (fp == NULL)
    {
        return;
    }
    fp->physics.vel_air.y -= gravity;
    if (fp->physics.vel_air.y < -tvel)
    {
        fp->physics.vel_air.y = -tvel;
    }
    if ((ndsFighterMarioFoxStageMPFallMapFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallMapFloorLoopPhysicsActive != FALSE))
    {
        gNdsStageMPFallMapFloorLoopGravityCallCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallLandFloorLoopPhysicsActive != FALSE))
    {
        gNdsStageMPFallLandFloorLoopGravityCallCount++;
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopPhysicsActive != FALSE))
    {
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingFallPhysicsActive != FALSE))
    {
        gNdsFighterLandingGravityCallCount++;
    }
    else if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        gNdsFighterJumpGravityCallCount++;
    }
}

void ftPhysicsApplyGravityDefault(FTStruct *fp, FTAttributes *attr)
{
    if ((fp == NULL) || (attr == NULL))
    {
        return;
    }
    ftPhysicsApplyGravityClampTVel(fp, attr->gravity, attr->tvel_base);
}

void ftPhysicsApplyFastFall(FTStruct *fp, FTAttributes *attr)
{
    if ((fp == NULL) || (attr == NULL))
    {
        return;
    }
    fp->physics.vel_air.y = -attr->tvel_fast;
}

void ftPhysicsClampGroundVel(FTStruct *fp, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->physics.vel_ground.x < -clamp)
    {
        fp->physics.vel_ground.x = -clamp;
    }
    else if (fp->physics.vel_ground.x > clamp)
    {
        fp->physics.vel_ground.x = clamp;
    }
}

void ftPhysicsApplyClampGroundVelStickRange(FTStruct *fp, s32 stick_x_min,
                                            f32 vel, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    if (ABS(fp->input.pl.stick_range.x) >= stick_x_min)
    {
        fp->physics.vel_ground.x +=
            (fp->input.pl.stick_range.x * vel * fp->lr);
        ftPhysicsClampGroundVel(fp, clamp);
    }
}

void ftPhysicsClampAirVelX(FTStruct *fp, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->physics.vel_air.x < -clamp)
    {
        fp->physics.vel_air.x = -clamp;
    }
    else if (fp->physics.vel_air.x > clamp)
    {
        fp->physics.vel_air.x = clamp;
    }
}

void ftPhysicsClampAirVelY(FTStruct *fp, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->physics.vel_air.y > clamp)
    {
        fp->physics.vel_air.y = clamp;
    }
}

void ftPhysicsAddClampAirVelY(FTStruct *fp, f32 vel, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    fp->physics.vel_air.y += vel;
    ftPhysicsClampAirVelY(fp, clamp);
}

void ftPhysicsClampAirVelXMax(FTStruct *fp)
{
    if ((fp != NULL) && (fp->attr != NULL))
    {
        ftPhysicsClampAirVelX(fp, fp->attr->air_speed_max_x);
    }
    if ((ndsFighterMarioFoxStageMPPassInputLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassInputLoopStatusActive != FALSE))
    {
        gNdsStageMPPassInputLoopClampCount++;
    }
}

void ftParamMakeRumble(FTStruct *fp, s32 rumble_id, s32 length)
{
    (void)fp;
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunProcParamsRumbleActive != FALSE))
    {
        gNdsFighterDashRunProcParamsRumbleCount++;
        gNdsFighterDashRunProcParamsRumbleLastID = (u32)rumble_id;
        gNdsFighterDashRunProcParamsRumbleLastLength = length;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        sNdsFighterDashRunDamageSetupRumbleCount++;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageExpiryActive != FALSE) &&
        (rumble_id == 3) && (length == 0))
    {
        sNdsFighterDashRunDamageFallClampRumbleCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopWallDamageActive != FALSE) &&
        (rumble_id == 2) && (length == 0))
    {
        gNdsStageMPPassiveLoopWallDamageRumbleCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopWallDamageActive != FALSE) &&
        (rumble_id == 3) && (length == 0))
    {
        sNdsStageMPPassiveLoopWallDamageFallClampRumbleCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCatchPullActive != FALSE))
    {
        gNdsStageMPPassiveLoopCatchPullRumbleCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowReleaseRumbleCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopClampRumbleCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDownBounceRumbleCount++;
        gNdsStageMPCliffWaitDamageLoopDownBounceRumbleID = (u32)rumble_id;
    }
}

sb32 ftPhysicsCheckClampAirVelXDec(FTStruct *fp, f32 clamp)
{
    if (fp == NULL)
    {
        return FALSE;
    }
    if (ABSF(fp->physics.vel_air.x) > clamp)
    {
        fp->physics.vel_air.x +=
            (fp->physics.vel_air.x >= 0.0F) ? -1.0F : 1.0F;
        if (ABSF(fp->physics.vel_air.x) < clamp)
        {
            fp->physics.vel_air.x =
                (fp->physics.vel_air.x >= 0.0F) ? clamp : -clamp;
        }
        return TRUE;
    }
    return FALSE;
}

sb32 ftPhysicsCheckClampAirVelXDecMax(FTStruct *fp, FTAttributes *attr)
{
    if ((fp == NULL) || (attr == NULL))
    {
        return FALSE;
    }
    return ftPhysicsCheckClampAirVelXDec(fp, attr->air_speed_max_x);
}

void ftPhysicsClampAirVelXStickRange(FTStruct *fp, s32 stick_range_min,
                                     f32 vel, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    if (ABS(fp->input.pl.stick_range.x) >= stick_range_min)
    {
        fp->physics.vel_air.x += fp->input.pl.stick_range.x * vel;
        ftPhysicsClampAirVelX(fp, clamp);
        if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
            (sNdsFighterProcessLoopPhysicsActive != FALSE))
        {
            return;
        }
        if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
        {
            gNdsFighterJumpAirDriftCallCount++;
        }
    }
}

void ftPhysicsClampAirVelXStickDefault(FTStruct *fp, FTAttributes *attr)
{
    if (attr != NULL)
    {
        ftPhysicsClampAirVelXStickRange(fp,
                                        FTPHYSICS_AIRDRIFT_CLAMP_RANGE_MIN,
                                        attr->air_accel,
                                        attr->air_speed_max_x);
    }
}

void ftPhysicsApplyAirVelXFriction(FTStruct *fp, FTAttributes *attr)
{
    if ((fp == NULL) || (attr == NULL))
    {
        return;
    }
    if (fp->physics.vel_air.x < 0.0F)
    {
        fp->physics.vel_air.x += attr->air_friction;
        if (fp->physics.vel_air.x > 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
    else if (fp->physics.vel_air.x > 0.0F)
    {
        fp->physics.vel_air.x -= attr->air_friction;
        if (fp->physics.vel_air.x < 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopPhysicsActive != FALSE))
    {
        return;
    }
    if ((ndsFighterMarioFoxStageMPFallMapFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallMapFloorLoopPhysicsActive != FALSE))
    {
        gNdsStageMPFallMapFloorLoopAirFrictionCallCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallLandFloorLoopPhysicsActive != FALSE))
    {
        gNdsStageMPFallLandFloorLoopAirFrictionCallCount++;
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingFallPhysicsActive != FALSE))
    {
        gNdsFighterLandingAirFrictionCallCount++;
    }
    else if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        gNdsFighterJumpAirFrictionCallCount++;
    }
}

void ftPhysicsCheckSetFastFall(FTStruct *fp)
{
    if ((fp != NULL) &&
        (ndsFighterMarioFoxStageMPFallMapFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallMapFloorLoopPhysicsActive != FALSE))
    {
        gNdsStageMPFallMapFloorLoopFastFallCheckCount++;
        return;
    }
    if ((fp != NULL) &&
        (ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallLandFloorLoopPhysicsActive != FALSE))
    {
        gNdsStageMPFallLandFloorLoopFastFallCheckCount++;
        return;
    }
    if ((fp != NULL) && (ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopPhysicsActive != FALSE))
    {
        return;
    }
    if ((fp != NULL) && (ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingFallPhysicsActive != FALSE))
    {
        gNdsFighterLandingFastFallCheckCount++;
        return;
    }
    if ((fp != NULL) && (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE))
    {
        gNdsFighterMarioFoxJumpLoopDeferredMask |= 1u << 6;
    }
    if ((fp != NULL) &&
        (fp->is_fastfall == FALSE) &&
        (fp->physics.vel_air.y < 0.0F) &&
        (fp->input.pl.stick_range.y <= FTCOMMON_FASTFALL_STICK_RANGE_MIN) &&
        (fp->tap_stick_y < FTCOMMON_FASTFALL_BUFFER_TICS_MAX))
    {
        fp->is_fastfall = TRUE;
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
        /* ponytail: colanim runtime is still deferred; this keeps the hook. */
        (void)ftParamCheckSetFighterColAnimID(fp->fighter_gobj,
                                              nGMColAnimFighterFastFall, 0);
    }
}

void ftPhysicsApplyAirVelDriftFastFall(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = (fp != NULL) ? fp->attr : NULL;

    if ((fp == NULL) || (attr == NULL))
    {
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageFallPhysicsActive != FALSE))
    {
        sNdsFighterDashRunDamageFallPhysicsCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallPhysicsActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDamageFallPhysicsTickCount++;
    }
    if ((ndsFighterMarioFoxStageMPFallMapFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallMapFloorLoopPhysicsActive != FALSE))
    {
        gNdsStageMPFallMapFloorLoopPhysicsCallbackCount++;
        gNdsStageMPFallMapFloorLoopAirDriftCallCount++;
    }
    if ((ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallLandFloorLoopPhysicsActive != FALSE))
    {
        gNdsStageMPFallLandFloorLoopPhysicsCallbackCount++;
        gNdsStageMPFallLandFloorLoopAirDriftCallCount++;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopPhysicsActive != FALSE))
    {
        ftPhysicsCheckSetFastFall(fp);
        if (fp->is_fastfall != FALSE)
        {
            ftPhysicsApplyFastFall(fp, attr);
        }
        else
        {
            ftPhysicsApplyGravityDefault(fp, attr);
        }
        if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
        {
            ftPhysicsClampAirVelXStickDefault(fp, attr);
            ftPhysicsApplyAirVelXFriction(fp, attr);
        }
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingFallPhysicsActive != FALSE))
    {
        gNdsFighterLandingAirDriftCallCount++;
    }
    else if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpAirPhysicsActive != FALSE))
    {
        gNdsFighterJumpAirPhysicsCallCount++;
    }
    ftPhysicsCheckSetFastFall(fp);
    if (fp->is_fastfall != FALSE)
    {
        ftPhysicsApplyFastFall(fp, attr);
    }
    else
    {
        ftPhysicsApplyGravityDefault(fp, attr);
    }
    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsClampAirVelXStickDefault(fp, attr);
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

void ftPhysicsApplyAirVelFriction(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = (fp != NULL) ? fp->attr : NULL;

    if ((fp == NULL) || (attr == NULL))
    {
        return;
    }
    ftPhysicsApplyGravityDefault(fp, attr);
    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

void ftPhysicsApplyAirVelDrift(GObj *fighter_gobj)
{
    ftPhysicsApplyAirVelDriftFastFall(fighter_gobj);
}

void mpCommonSetFighterGround(FTStruct *fp)
{
    if (fp != NULL)
    {
        fp->physics.vel_ground.x =
            fp->physics.vel_air.x * (f32)fp->lr;
        fp->ga = nMPKineticsGround;
        fp->jumps_used = 0;
        fp->stat_flags.ga = nMPKineticsGround;
        fp->vel_ground = fp->physics.vel_ground;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopMapActive != FALSE))
    {
        gNdsFighterProcessLoopSetGroundCount++;
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingSetStatusActive != FALSE))
    {
        gNdsFighterLandingSetGroundCallCount++;
    }
    if ((ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallLandFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPFallLandFloorLoopSetGroundCallCount++;
    }
    if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
    {
        gNdsFighterWaitGroundSetCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDownBounceGroundSetCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive != FALSE) &&
        (fp != NULL) &&
        (fp->tics_since_last_z < FTCOMMON_PASSIVE_BUFFER_TICS_MAX))
    {
        if (ABS(fp->input.pl.stick_range.x) >=
            FTCOMMON_PASSIVE_F_OR_B_RANGE)
        {
            gNdsStageMPCliffWaitDamageLoopPassiveStandGroundSetCount++;
        }
        else
        {
            gNdsStageMPCliffWaitDamageLoopPassiveGroundSetCount++;
        }
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopCliffCatchGroundSetCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandSetStatusActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveStandGroundSetCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveSetStatusActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveGroundSetCount++;
    }
}

/* Declared here rather than through <ft/ftpublic.h>: the port mirrors many decomp
 * headers under include/ but has no ftpublic.h of its own, and this is the only
 * caller. The definition is already compiled -- src/import/battleship_ftpublic.c
 * includes decomp's ftpublic.c -- it was simply --gc-sections'd out of the ROM
 * for want of any caller at all, which is why the cue never fired. */
extern void ftPublicPlayCliffReact(GObj *fighter_gobj, f32 knockback);

/* Engagement proof for the crowd's near-KO gasp. Zero after a match in which
 * someone was launched toward a blast zone and survived means the branch below
 * never ran; the cue being silent while this counts is a different fault. */
volatile u32 gNdsFtPublicCliffReactCount;

void mpCommonSetFighterLandingParams(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    /* THE CROWD'S NEAR-KO GASP. This function used to clear public_knockback and
     * nothing else, and the note here called the missing branch "audio debt" --
     * so FGM 615/616/617 (GaspL/M/S) were packed, allowlisted, and unreachable.
     *
     * Source, mpcommon.c:426-440: a fighter carrying >= 100 knockback who LANDS
     * within 450 units of a blast bound gets a crowd reaction scaled by that
     * knockback, and only then is the flag cleared. Both landing seams already
     * called this function -- :9161 for a floor and :11603 for a cliff catch --
     * so only the branch itself was absent.
     *
     * ftmain.c:1821 is the other half and is already live: it disarms
     * public_knockback every frame the fighter is back between the bounds, so
     * the flag survives only while they are still out by the edge. That is what
     * makes this the "sent flying and saved it" gasp rather than a landing sound.
     *
     * The joint NULL check is a port addition; source dereferences it unguarded.
     * The switch on fp->fkind below is still deliberately not ported: do not
     * clear another fighter's passive union through Mario's member while
     * landing. */
    if (fp->public_knockback != 0.0F)
    {
        if ((fp->public_knockback >= 100.0F) &&
            (fp->joints[nFTPartsJointTopN] != NULL))
        {
            f32 topn_x = fp->joints[nFTPartsJointTopN]->translate.vec.f.x;

            if ((topn_x < (gMPCollisionBounds.current.left + 450.0F)) ||
                (topn_x > (gMPCollisionBounds.current.right - 450.0F)))
            {
                ftPublicPlayCliffReact(fighter_gobj, fp->public_knockback);
                gNdsFtPublicCliffReactCount++;
            }
        }
    }
    fp->public_knockback = 0.0F;
    switch (fp->fkind)
    {
    case nFTKindMario:
    case nFTKindMMario:
    case nFTKindNMario:
        fp->passive_vars.mario.is_expend_tornado = FALSE;
        break;

    default:
        break;
    }
    if ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopMapActive != FALSE))
    {
        gNdsStageMPCliffCatchFloorLoopLandingParamCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopRecatchMapActive != FALSE))
    {
        gNdsStageMPCliffClimbFloorLoopRecatchLandingParamCount++;
    }
}

void mpCommonRunDefaultCollision(MPCollData *coll_data, GObj *gobj,
                                 u32 flags)
{
    (void)gobj;
    (void)flags;
    if (coll_data == NULL)
    {
        return;
    }
    gNdsCollisionRuntimeDiagnostics.default_run_calls++;
    /* Floor acquisition/resolution is graduated first.  The O2R wall runner
     * does not yet populate source line/angle state, so entering those source
     * branches here would be less faithful than leaving them deferred. */
    if (mpProcessRunFloorCollisionAdjNewNULL(coll_data) != FALSE)
    {
        mpProcessSetCollideFloor(coll_data);
        if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
        {
            mpProcessRunFloorEdgeAdjust(coll_data);
        }
    }
    else
    {
        mpProcessSetCollProjectFloorID(coll_data);
    }
}

void mpCommonCopyCollDataStats(MPCollData *this_coll_data, Vec3f *pos,
                               MPCollData *other_coll_data)
{
    if ((this_coll_data == NULL) || (pos == NULL) ||
        (other_coll_data == NULL))
    {
        return;
    }
    gNdsCollisionRuntimeDiagnostics.default_copy_calls++;
    this_coll_data->pos_prev = *pos;
    this_coll_data->p_map_coll = &other_coll_data->map_coll;
    this_coll_data->mask_curr = 0u;
    this_coll_data->mask_unk = 0u;
    this_coll_data->mask_stat = 0u;
    this_coll_data->is_coll_end = FALSE;
    this_coll_data->update_tic = other_coll_data->update_tic;
}

void mpCommonResetCollDataStats(MPCollData *coll_data)
{
    if (coll_data == NULL)
    {
        return;
    }
    gNdsCollisionRuntimeDiagnostics.default_reset_calls++;
    coll_data->p_map_coll = &coll_data->map_coll;
    coll_data->update_tic = gMPCollisionUpdateTic;
    coll_data->mask_curr = 0u;
}

void mpCommonRunFighterCollisionDefault(GObj *fighter_gobj, Vec3f *pos,
                                        FTCollisionData *coll_data)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        gNdsFighterDashRunDamageLoseGripCollisionCount++;
        gNdsFighterDashRunDamageLoseGripMask |= NDS_DAMAGE_LOSEGRIP_COLLISION;
        if (pos != NULL)
        {
            gNdsFighterDashRunDamageLoseGripTargetX =
                ndsFloatToMilliSigned(pos->x);
            gNdsFighterDashRunDamageLoseGripTargetY =
                ndsFloatToMilliSigned(pos->y);
        }
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopCollisionDefaultCount++;
        if ((fp != NULL) && (fp->coll_data.p_translate != NULL))
        {
            gNdsStageMPCliffWaitDamageLoopRootXBeforeMilli =
                ndsFloatToMilliSigned(fp->coll_data.p_translate->x);
            gNdsStageMPCliffWaitDamageLoopRootYBeforeMilli =
                ndsFloatToMilliSigned(fp->coll_data.p_translate->y);
        }
        if (pos != NULL)
        {
            gNdsStageMPCliffWaitDamageLoopTargetXMilli =
                ndsFloatToMilliSigned(pos->x);
            gNdsStageMPCliffWaitDamageLoopTargetYMilli =
                ndsFloatToMilliSigned(pos->y);
        }
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowDeadResultActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowDeadResultCollisionCount++;
    }
    if ((fp != NULL) && (pos != NULL) && (coll_data != NULL) &&
        (fp->coll_data.p_translate != NULL))
    {
        gNdsCollisionRuntimeDiagnostics.default_fighter_calls++;
        mpCommonCopyCollDataStats(&fp->coll_data, pos, coll_data);
        mpCommonRunDefaultCollision(&fp->coll_data, fighter_gobj,
                                    MAP_PROC_TYPE_DEFAULT);
        mpCommonResetCollDataStats(&fp->coll_data);
        if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
                FALSE) &&
            (sNdsStageMPCliffWaitDamageLoopInterruptActive != FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopRootXAfterMilli =
                ndsFloatToMilliSigned(fp->coll_data.p_translate->x);
            gNdsStageMPCliffWaitDamageLoopRootYAfterMilli =
                ndsFloatToMilliSigned(fp->coll_data.p_translate->y);
        }
    }
}

void ftParamSetPlayerTagWait(GObj *fighter_gobj, s32 wait)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp != NULL)
    {
        fp->playertag_wait = wait;
    }
    if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
    {
        gNdsFighterWaitPlayerTagSetCount++;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        sNdsFighterDashRunDamageSetupPlayerTagCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitFloorLoopPlayerTagWaitCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFinishLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFinishLoopUpdateActive != FALSE))
    {
        gNdsStageMPCliffClimbFinishLoopPlayerTagWaitCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassiveStandPlayerTagWaitCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassivePlayerTagWaitCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveStandPlayerTagWaitCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassivePlayerTagWaitCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopAttackUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopAttackPlayerTagWaitCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollForwardUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollForwardPlayerTagWaitCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollBackUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollBackPlayerTagWaitCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopDownStandPlayerTagWaitCount++;
    }
    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopFinalUpdateActive != FALSE))
    {
        gNdsStageTurnLoopPlayerTagWaitCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPDownRecoverLoopDownStandUpdateActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopAttackUpdateActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopRollForwardUpdateActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopRollBackUpdateActive != FALSE)))
    {
        gNdsStageMPDownRecoverLoopPlayerTagWaitCount++;
    }
}

void ftParamSetCaptureImmuneMask(FTStruct *fp, u8 capture_immune_mask)
{
    if (fp != NULL)
    {
        fp->capture_immune_mask = capture_immune_mask;
    }
    if (sNdsFTMainSetStatusCompatReplayActive != FALSE)
    {
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffCatchFloorLoopCaptureImmuneCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitFloorLoopCaptureImmuneCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownWaitSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDownWaitCaptureImmuneCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownWaitSetStatusActive != FALSE))
    {
        gNdsStageMPDownWaitLoopDownWaitCaptureImmuneCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownWaitSetStatusActive != FALSE))
    {
        gNdsStageMPDownRecoverLoopDownWaitCaptureImmuneCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopCliffCatchCaptureImmuneCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPPassiveLoopCatchPullActive != FALSE) ||
         (sNdsStageMPPassiveLoopCatchPullUpdateActive != FALSE)))
    {
        gNdsStageMPPassiveLoopCatchPullCaptureImmuneCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPPassiveLoopCaptureActive != FALSE) ||
         (sNdsStageMPPassiveLoopCapturePhysicsActive != FALSE)))
    {
        gNdsStageMPPassiveLoopCaptureCaptureImmuneCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowCaptureImmuneCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowCallbackImmediateActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowCallbackImmediateCaptureImmuneCount++;
    }
}

void ftParamSetCatchParams(FTStruct *fp, u8 catch_mask,
                           void (*proc_catch)(GObj *),
                           void (*proc_capture)(GObj *, GObj *))
{
    if (fp != NULL)
    {
        fp->is_catchstatus = TRUE;
        fp->catch_mask = catch_mask;
        fp->proc_catch = proc_catch;
        fp->proc_capture = proc_capture;
    }
}

void ftParamSetThrowParams(FTStruct *fp, GObj *throw_gobj)
{
    if (fp != NULL)
    {
        FTStruct *throw_fp =
            (throw_gobj != NULL) ? ftGetStruct(throw_gobj) : NULL;

        fp->throw_gobj = throw_gobj;
        if (throw_fp != NULL)
        {
            fp->throw_fkind = throw_fp->fkind;
            fp->throw_team = throw_fp->team;
            fp->throw_player = throw_fp->player;
            fp->throw_player_num = throw_fp->player_num;
        }
        else
        {
            fp->throw_fkind = nFTKindNull;
            fp->throw_team = 0;
            fp->throw_player = 0;
            fp->throw_player_num = 0;
        }
        if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
            (sNdsStageMPPassiveLoopThrowProcStatusActive != FALSE))
        {
            gNdsStageMPPassiveLoopThrowProcStatusParamSetCount++;
            gNdsStageMPPassiveLoopThrowProcStatusThrowGObjReady =
                (throw_gobj != NULL) ? 1u : 0u;
        }
    }
}

#if !(NDS_P2_LINK || NDS_P2_ITEM_CORE)
sb32 ftCommonLightThrowCheckItemTypeThrow(FTStruct *fp)
{
    (void)fp;
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCatchActive != FALSE))
    {
        gNdsStageMPPassiveLoopCatchItemThrowCheckCount++;
    }
    return FALSE;
}

void ftCommonLightThrowDecideSetStatus(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCatchActive != FALSE))
    {
        gNdsStageMPPassiveLoopCatchItemThrowSetStatusCount++;
    }
}
#endif

void ftCommonCatchPullProcCatch(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonCatchPullProcCatch(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCatchPullActive != FALSE))
    {
        gNdsStageMPPassiveLoopCatchPullProcCatchCount++;
        ndsBaseFTCommonCatchPullProcCatch(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCatchActive != FALSE))
    {
        gNdsStageMPPassiveLoopCatchPullDeferredCount++;
    }
    (void)fighter_gobj;
}

void ftCommonCapturePulledProcCapture(GObj *fighter_gobj,
                                      GObj *capture_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonCapturePulledProcCapture(fighter_gobj, capture_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCaptureActive != FALSE))
    {
        gNdsStageMPPassiveLoopCaptureProcCaptureCount++;
        ndsBaseFTCommonCapturePulledProcCapture(fighter_gobj, capture_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCatchActive != FALSE))
    {
        gNdsStageMPPassiveLoopCapturePulledDeferredCount++;
    }
    (void)fighter_gobj;
    (void)capture_gobj;
}

static void ndsStageMPPassiveLoopCaptureProcDamage(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCaptureActive != FALSE))
    {
        gNdsStageMPPassiveLoopCaptureProcDamageCount++;
    }
}

static void ndsFtParamStopVoice(FTStruct *fp)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->p_voice != NULL)
    {
        if ((fp->p_voice->sfx_id != 0) && (fp->p_voice->sfx_id == fp->voice_id))
        {
            func_80026738_27338(fp->p_voice);
        }
    }
    fp->p_voice = NULL;
    fp->voice_id = 0;
}

void ftParamPlayVoice(FTStruct *fp, u16 voice_id)
{
    if (fp == NULL)
    {
        return;
    }
    fp->p_voice = func_800269C0_275C0(voice_id);
    fp->voice_id = (fp->p_voice != NULL) ? fp->p_voice->sfx_id : 0;
}

void ftParamPlayLoopSFX(FTStruct *fp, u16 sfx_id)
{
    if ((fp != NULL) && (fp->p_loop_sfx == NULL))
    {
        fp->p_loop_sfx = func_800269C0_275C0(sfx_id);
        fp->loop_sfx_id =
            (fp->p_loop_sfx != NULL) ? fp->p_loop_sfx->sfx_id : 0;
    }
}

void ftParamStopLoopSFX(FTStruct *fp)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->p_loop_sfx != NULL)
    {
        if ((fp->p_loop_sfx->sfx_id != 0) &&
            (fp->p_loop_sfx->sfx_id == fp->loop_sfx_id))
        {
            func_80026738_27338(fp->p_loop_sfx);
        }
    }
    fp->p_loop_sfx = NULL;
    fp->loop_sfx_id = 0;
}

void gmRumbleStopRumbleID(s32 player, s32 rumble_id)
{
    (void)player;
    (void)rumble_id;
}

void ftBossCommonUpdateDamageStats(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

__attribute__((weak)) s32 itMainGetDamageOutput(ITStruct *ip)
{
    if (ip == NULL)
    {
        return 0;
    }
    return (s32)((ip->attack_coll.damage * ip->attack_coll.stale) + 0.999F);
}

static void ndsCompatSetHitInteractStats(GMAttackRecord *records,
                                         GObj *victim_gobj, s32 attack_type,
                                         u32 group_id, u32 rehit_time)
{
    s32 i;

    if (records == NULL)
    {
        return;
    }
    for (i = 0; i < GMATTACKREC_NUM_MAX; i++)
    {
        if (records[i].victim_gobj == victim_gobj)
        {
            break;
        }
    }
    if (i == GMATTACKREC_NUM_MAX)
    {
        for (i = 0; i < GMATTACKREC_NUM_MAX; i++)
        {
            if (records[i].victim_gobj == NULL)
            {
                break;
            }
        }
        if (i == GMATTACKREC_NUM_MAX)
        {
            i = 0;
        }
        records[i].victim_gobj = victim_gobj;
    }

    switch (attack_type)
    {
    case nGMHitTypeDamage:
        records[i].victim_flags.is_interact_hurt = TRUE;
        break;

    case nGMHitTypeShield:
        records[i].victim_flags.is_interact_shield = TRUE;
        break;

    case nGMHitTypeShieldRehit:
        records[i].victim_flags.is_interact_shield = TRUE;
        records[i].victim_flags.timer_rehit = rehit_time;
        break;

    case nGMHitTypeReflect:
        records[i].victim_flags.is_interact_reflect = TRUE;
        records[i].victim_flags.timer_rehit = rehit_time;
        break;

    case nGMHitTypeAbsorb:
        records[i].victim_flags.is_interact_absorb = TRUE;
        break;

    case nGMHitTypeAttack:
        records[i].victim_flags.group_id = group_id;
        break;

    case nGMHitTypeDamageRehit:
        records[i].victim_flags.is_interact_hurt = TRUE;
        records[i].victim_flags.timer_rehit = rehit_time;
        break;

    default:
        break;
    }
}

__attribute__((weak)) void itProcessSetHitInteractStats(
    ITAttackColl *attack_coll, GObj *victim_gobj, s32 attack_type,
    u32 victim_group_id)
{
    if (attack_coll == NULL)
    {
        return;
    }
    ndsCompatSetHitInteractStats(attack_coll->attack_records, victim_gobj,
                                 attack_type, victim_group_id,
                                 ITEM_REHIT_TIME_DEFAULT);
}

void ftParamTryPlayItemMusic(s32 bgm_id)
{
    (void)bgm_id;
}

void ftParamSetStarHitStatusInvincible(FTStruct *fp, s32 invincible_tics)
{
    if (fp == NULL)
    {
        return;
    }
    fp->star_hitstatus = nGMHitStatusInvincible;
    fp->star_invincible_tics = invincible_tics;
    ftParamCheckSetFighterColAnimID(fp->fighter_gobj,
                                    nGMColAnimFighterStar, 0);
}

void ftParamSetHealDamage(FTStruct *fp, s32 heal)
{
    if (fp == NULL)
    {
        return;
    }
    fp->damage_heal += heal;
    ftParamCheckSetFighterColAnimID(fp->fighter_gobj,
                                    nGMColAnimFighterHeal, 0);
}

void ftParamStopVoiceRunProcDamage(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCaptureActive != FALSE))
    {
        gNdsStageMPPassiveLoopCaptureVoiceStopCount++;
    }
    ndsFtParamStopVoice(fp);
    if (fp->proc_damage != NULL)
    {
        fp->proc_damage(fighter_gobj);
    }
}

#if !NDS_P2_DONKEY
/* Compile seam for builds without Donkey Kong, where `battleship_donkey.c` --
 * and therefore the source `ftDonkeyThrowFDamageSetStatus` -- is not compiled.
 * Nothing can reach it there: ftCommonDamageUpdateCatchResist only takes this
 * arm for a Donkey Kong holding a fighter or a heavy item.  With
 * NDS_P2_DONKEY=1 the source setter is linked and battleship_ftcommon_damage.c
 * calls it directly (P2-3r10). */
void ndsCompatFTDonkeyThrowFDamageSetStatus(GObj *fighter_gobj)
{
    ftCommonDamageGotoDamageStatus(fighter_gobj);
}
#endif

void ftSetupDropItem(FTStruct *fp)
{
    if (fp != NULL)
    {
        fp->item_gobj = NULL;
    }
}

void ftCommonThrownSetStatusDamageRelease(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonThrownSetStatusDamageRelease(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        ndsBaseFTCommonThrownSetStatusDamageRelease(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseStatusActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowReleaseStatusDamageReleaseCount++;
        ndsBaseFTCommonThrownSetStatusDamageRelease(fighter_gobj);
        return;
    }
    (void)fighter_gobj;
}

void ftCommonThrownUpdateDamageStats(FTStruct *this_fp)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonThrownUpdateDamageStats(this_fp);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        ndsBaseFTCommonThrownUpdateDamageStats(this_fp);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseStatusActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowReleaseStatusUpdateDamageStatsCount++;
        ndsBaseFTCommonThrownUpdateDamageStats(this_fp);
        return;
    }
    (void)this_fp;
}

void ftCommonThrownSetStatusNoDamageRelease(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonThrownSetStatusNoDamageRelease(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        ndsBaseFTCommonThrownSetStatusNoDamageRelease(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseStatusActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageReleaseCount++;
        ndsBaseFTCommonThrownSetStatusNoDamageRelease(fighter_gobj);
        return;
    }
    (void)fighter_gobj;
}

static sb32 ndsFTCommonThrownLoseGripCanCallOriginal(GObj *fighter_gobj)
{
    FTStruct *this_fp;
    GObj *interact_gobj;

    if (fighter_gobj == NULL)
    {
        return FALSE;
    }
    this_fp = ftGetStruct(fighter_gobj);
    if ((this_fp == NULL) || (DObjGetStruct(fighter_gobj) == NULL))
    {
        return FALSE;
    }
    interact_gobj = (this_fp->is_catch_or_capture != FALSE) ?
        this_fp->catch_gobj : this_fp->capture_gobj;
    if ((interact_gobj == NULL) || (ftGetStruct(interact_gobj) == NULL) ||
        (DObjGetStruct(interact_gobj) == NULL))
    {
        return FALSE;
    }
    if ((this_fp->status_id >= nFTCommonStatusThrownStart) &&
        (this_fp->status_id <= nFTCommonStatusThrownEnd) &&
        (this_fp->joints[nFTPartsJointCommonStart] == NULL))
    {
        return FALSE;
    }
    return TRUE;
}

static void ndsFTCommonThrownLoseGripRecordOriginal(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    gNdsFighterDashRunDamageLoseGripReleaseCount++;
    gNdsFighterDashRunDamageLoseGripMask |=
        NDS_DAMAGE_LOSEGRIP_SELECT | NDS_DAMAGE_LOSEGRIP_LINKS |
        NDS_DAMAGE_LOSEGRIP_ORIGINAL;
    if ((fp != NULL) &&
        (fp->status_id >= nFTCommonStatusThrownStart) &&
        (fp->status_id <= nFTCommonStatusThrownEnd))
    {
        gNdsFighterDashRunDamageLoseGripMask |=
            NDS_DAMAGE_LOSEGRIP_POSITION;
    }
}

static void ndsFTCommonThrownReleaseFighterLoseGripBounded(
    GObj *fighter_gobj, GObj *fallback_gobj)
{
    FTStruct *this_fp;
    GObj *interact_gobj;
    FTStruct *interact_fp;
    DObj *fighter_dobj;
    DObj *interact_dobj;
    Vec3f pos;

    if (fighter_gobj == NULL)
    {
        return;
    }

    this_fp = ftGetStruct(fighter_gobj);
    if (this_fp == NULL)
    {
        return;
    }

    interact_gobj = (this_fp->is_catch_or_capture != FALSE) ?
        this_fp->catch_gobj : this_fp->capture_gobj;
    if (interact_gobj == NULL)
    {
        interact_gobj = fallback_gobj;
    }
    if (interact_gobj == NULL)
    {
        return;
    }

    interact_fp = ftGetStruct(interact_gobj);
    fighter_dobj = DObjGetStruct(fighter_gobj);
    interact_dobj = DObjGetStruct(interact_gobj);
    if ((interact_fp == NULL) || (fighter_dobj == NULL) ||
        (interact_dobj == NULL))
    {
        return;
    }

    gNdsFighterDashRunDamageLoseGripReleaseCount++;
    gNdsFighterDashRunDamageLoseGripMask |=
        NDS_DAMAGE_LOSEGRIP_SELECT | NDS_DAMAGE_LOSEGRIP_LINKS;

    if ((this_fp->status_id >= nFTCommonStatusThrownStart) &&
        (this_fp->status_id <= nFTCommonStatusThrownEnd) &&
        (this_fp->joints[nFTPartsJointCommonStart] != NULL))
    {
        pos.x = 0.0F;
        pos.y = 0.0F;
        pos.z = 0.0F;
        gmCollisionGetFighterPartsWorldPosition(
            this_fp->joints[nFTPartsJointCommonStart], &pos);
        pos.y -= 300.0F;
        fighter_dobj->translate.vec.f = pos;
        gNdsFighterDashRunDamageLoseGripMask |=
            NDS_DAMAGE_LOSEGRIP_POSITION;
    }

    mpCommonRunFighterCollisionDefault(
        fighter_gobj, &interact_dobj->translate.vec.f,
        &interact_fp->coll_data);

    if ((this_fp->ga == nMPKineticsGround) &&
        ((this_fp->coll_data.floor_line_id == -1) ||
         (this_fp->coll_data.floor_dist != 0.0F)))
    {
        mpCommonSetFighterAir(this_fp);
    }
}

void ftCommonThrownDecideFighterLoseGrip(GObj *fighter_gobj,
                                         GObj *interact_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonThrownDecideFighterLoseGrip(fighter_gobj, interact_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        FTStruct *this_fp = ftGetStruct(fighter_gobj);
        FTStruct *interact_fp = ftGetStruct(interact_gobj);
        GObj *release_gobj = ((this_fp != NULL) &&
            (this_fp->is_catch_or_capture != FALSE)) ?
            fighter_gobj : interact_gobj;

        if ((this_fp != NULL) && (interact_fp != NULL) &&
            (ndsFTCommonThrownLoseGripCanCallOriginal(release_gobj) != FALSE))
        {
            ndsFTCommonThrownLoseGripRecordOriginal(release_gobj);
            ndsBaseFTCommonThrownDecideFighterLoseGrip(fighter_gobj,
                                                       interact_gobj);
        }
        else if (this_fp != NULL)
        {
            if (this_fp->is_catch_or_capture != FALSE)
            {
                ndsFTCommonThrownReleaseFighterLoseGripBounded(
                    fighter_gobj, interact_gobj);
            }
            else
            {
                ndsFTCommonThrownReleaseFighterLoseGripBounded(
                    interact_gobj, fighter_gobj);
            }
        }
        if (interact_fp != NULL)
        {
            interact_fp->capture_gobj = NULL;
        }
        if (this_fp != NULL)
        {
            this_fp->catch_gobj = NULL;
        }
        if (((interact_fp == NULL) || (interact_fp->capture_gobj == NULL)) &&
            ((this_fp == NULL) || (this_fp->catch_gobj == NULL)))
        {
            gNdsFighterDashRunDamageLoseGripLinkClearCount++;
            gNdsFighterDashRunDamageLoseGripMask |=
                NDS_DAMAGE_LOSEGRIP_LINK_CLEAR;
        }
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowDeadResultActive != FALSE))
    {
        ndsBaseFTCommonThrownDecideFighterLoseGrip(fighter_gobj,
                                                   interact_gobj);
        return;
    }
    (void)fighter_gobj;
    (void)interact_gobj;
}

void ftCommonThrownDecideDeadResult(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonThrownDecideDeadResult(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowDeadResultActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowDeadResultCallCount++;
        ndsBaseFTCommonThrownDecideDeadResult(fighter_gobj);
        return;
    }
    (void)fighter_gobj;
}

void ftParamSetHitStatusAll(GObj *fighter_gobj, s32 hitstatus)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp != NULL)
    {
        fp->hitstatus = hitstatus;
        ndsFTParamSetHitStatusColAnim(fighter_gobj, hitstatus);
    }
}

extern void gmCollisionCopyMatrix(Mtx44f dst, Mtx44f src);

#if NDS_R2_POSITION_PROBE
/* BUGS.md Fox crouch-under-Blaster diagnostic.
 *
 * This is probe-only observation of the SAME source matrices the gameplay
 * collision path consumes. gmCollisionCheckWeaponAttackFighterDamageCollide
 * calls func_ovl2_800EDBA4 through func_ovl2_800EDE00 before testing each
 * FTDamageColl; calling the builder here merely fills that cache a little
 * earlier in a diagnostic ROM. No status, animation, collision descriptor,
 * weapon coordinate, or fighter coordinate is changed.
 *
 * Capture the first source Wait pose and first source SquatWait pose in one ROM
 * so standing-vs-crouching hurtbox geometry cannot be confused by a rebuild,
 * RNG seed, or a later respawn. Slots [0..10] are Wait; [11..21] are
 * SquatWait. `used` keeps the arrays readable by GDB with no in-ROM consumer. */
#define NDS_MARIO_HURT_PROBE_SLOTS (FTDAMAGECOLL_NUM_MAX * 2)
__attribute__((used)) volatile u32 gNdsMarioHurtProbeCaptureMask;
__attribute__((used)) s32 gNdsMarioHurtProbeJoint[NDS_MARIO_HURT_PROBE_SLOTS];
__attribute__((used)) f32 gNdsMarioHurtProbeCenterY[NDS_MARIO_HURT_PROBE_SLOTS];
__attribute__((used)) f32 gNdsMarioHurtProbeExtentY[NDS_MARIO_HURT_PROBE_SLOTS];
__attribute__((used)) f32 gNdsMarioHurtProbeJointY[NDS_MARIO_HURT_PROBE_SLOTS];
__attribute__((used)) f32 gNdsMarioHurtProbeRootY[2];

void ndsPositionProbeCaptureMarioHurtboxes(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    DObj *root = (fighter_gobj != NULL) ? DObjGetStruct(fighter_gobj) : NULL;
    u32 mode;
    u32 base;
    u32 i;

    if ((fp == NULL) || (root == NULL) || (fp->attr == NULL))
    {
        return;
    }
    if (fp->status_id == nFTCommonStatusWait)
    {
        mode = 0u;
    }
    else if (fp->status_id == nFTCommonStatusSquatWait)
    {
        mode = 1u;
    }
    else
    {
        return;
    }
    if ((gNdsMarioHurtProbeCaptureMask & (1u << mode)) != 0u)
    {
        return;
    }

    base = mode * FTDAMAGECOLL_NUM_MAX;
    gNdsMarioHurtProbeRootY[mode] = root->translate.vec.f.y;
    for (i = 0u; i < FTDAMAGECOLL_NUM_MAX; i++)
    {
        FTDamageColl *dc = &fp->damage_colls[i];
        FTParts *parts;
        f32 b0;
        f32 b1;
        f32 b2;
        f32 center_y;
        f32 extent_y;

        gNdsMarioHurtProbeJoint[base + i] = -1;
        gNdsMarioHurtProbeCenterY[base + i] = 0.0F;
        gNdsMarioHurtProbeExtentY[base + i] = 0.0F;
        gNdsMarioHurtProbeJointY[base + i] = 0.0F;
        if ((dc->joint_id < 0) || (dc->joint == NULL))
        {
            continue;
        }

        func_ovl2_800EDBA4(dc->joint);
        parts = ftGetParts(dc->joint);
        if (parts == NULL)
        {
            continue;
        }
        center_y =
            (parts->mtx_translate[0][1] * dc->offset.x) +
            (parts->mtx_translate[1][1] * dc->offset.y) +
            (parts->mtx_translate[2][1] * dc->offset.z) +
            parts->mtx_translate[3][1];
        b0 = parts->mtx_translate[0][1];
        b1 = parts->mtx_translate[1][1];
        b2 = parts->mtx_translate[2][1];
        if (b0 < 0.0F) b0 = -b0;
        if (b1 < 0.0F) b1 = -b1;
        if (b2 < 0.0F) b2 = -b2;
        extent_y = (b0 * dc->size.x) + (b1 * dc->size.y) +
                   (b2 * dc->size.z);

        gNdsMarioHurtProbeJoint[base + i] = dc->joint_id;
        gNdsMarioHurtProbeCenterY[base + i] = center_y;
        gNdsMarioHurtProbeExtentY[base + i] = extent_y;
        gNdsMarioHurtProbeJointY[base + i] = parts->mtx_translate[3][1];
    }
    gNdsMarioHurtProbeCaptureMask |= 1u << mode;
}
#endif /* NDS_R2_POSITION_PROBE */

/* BUGS.md #2: this returned identity plus the joint's *local* translate, so a
 * grab placed the victim at the capturer's hand offset measured from the world
 * origin instead of from the hand -- the snap the owner sees. The contract is
 * the joint's world matrix, and ftcommoncapturepulled.c's non-US branch spells
 * the same computation out: func_ovl2_800EDBA4 to rebuild the chain, then read
 * parts->mtx_translate. That cache is live here -- ndsFTParamsInvalidateFighterParts
 * clears unk_dobjtrans_word every frame -- so use it rather than recomposing.
 * ftcommoncapturepulled.c is the only caller; scsubsysfighter.c declares this
 * but never calls it. */
void func_ovl0_800C9A38(Mtx44f mtx, DObj *dobj)
{
    FTParts *parts;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            mtx[i][j] = (i == j) ? 1.0F : 0.0F;
        }
    }
    if ((dobj == NULL) || (dobj->parent_gobj == NULL))
    {
        return;
    }
    func_ovl2_800EDBA4(dobj);
    parts = ftGetParts(dobj);
    if (parts != NULL)
    {
        gmCollisionCopyMatrix(mtx, parts->mtx_translate);
    }
}

/* BUGS.md "Fox's pistol model is missing": Fox's gun is not an object or an
 * effect, it is model part 13 on joint 17 (FTFOX_BLASTER_HOLD_JOINT), and its
 * geometry is file 315 at 0x3F0 -- already shipped (Makefile
 * reloc_extern_data/MiscData315) and already resolved through
 * dFoxMain_modelparts_container, whose desc[13] points at it.
 *
 * The request has been live all along: the port #includes ftmain.c verbatim
 * (battleship_ftmain.c:81) and its decoder dispatches
 * nFTMotionEventSetModelPartID at ftmain.c:575 straight into this function.
 * Discarding the arguments here is why no gun appears.
 *
 * The old shim stopped at the state mirror. That was insufficient: the DS
 * display contract captures the source DObj's live DL/flags and its draw-plan
 * memo can persist across motion events. Restore the complete source mutation
 * below and invalidate those DS caches at the same writer seam. */
void ftParamSetModelPartID(GObj *fighter_gobj, s32 joint_id,
                           s32 modelpart_id)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    s32 slot;

    if ((fp == NULL) || (joint_id < nFTPartsJointCommonStart) ||
        ((u32)joint_id >= ARRAY_COUNT(fp->joints)))
    {
        return;
    }
    slot = joint_id - nFTPartsJointCommonStart;
    if (((u32)slot >= ARRAY_COUNT(fp->modelpart_status)) ||
        (fp->joints[joint_id] == NULL))
    {
        return;
    }
    if (fp->modelpart_status[slot].modelpart_id_curr == (s8)modelpart_id)
    {
        return;
    }
    fp->modelpart_status[slot].modelpart_id_curr = (s8)modelpart_id;
    (void)ndsFTParamApplyModelPartCurrent(fighter_gobj, fp, slot);
    fp->is_modelpart_modify = TRUE;
    gNdsFighterModelPartSetCount++;
    if (modelpart_id >= 0)
    {
        gNdsFighterModelPartOnCount++;
    }
    ndsFTParamInvalidateModelPartRenderer(fighter_gobj, fp);
}

void ftParamSetModelPartDetailAll(GObj *fighter_gobj, u8 detail)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    s32 i;

    if ((fp == NULL) || (detail == fp->detail_curr))
    {
        return;
    }
    fp->detail_curr = detail;
    for (i = nFTPartsJointCommonStart; i < (s32)ARRAY_COUNT(fp->joints); i++)
    {
        if (fp->joints[i] != NULL)
        {
            s32 slot = i - nFTPartsJointCommonStart;
            s32 modelpart_id = fp->modelpart_status[slot].modelpart_id_curr;

            if (modelpart_id != -1)
            {
                fp->modelpart_status[slot].modelpart_id_curr = -1;
                ftParamSetModelPartID(fighter_gobj, i, modelpart_id);
            }
        }
    }
    ftParamInitTexturePartAll(fighter_gobj);
}

sb32 ftCommonThrowCheckInterruptCatchWait(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonThrowCheckInterruptCatchWait(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopCatchWaitInterruptActive != FALSE))
    {
        gNdsStageMPPassiveLoopCatchWaitThrowCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowCheckCount++;
        return ndsBaseFTCommonThrowCheckInterruptCatchWait(fighter_gobj);
    }
    return FALSE;
}

void ftCommonThrownReleaseThrownUpdateStats(GObj *fighter_gobj, s32 lr,
                                            s32 script_id,
                                            sb32 is_proc_status)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonThrownReleaseThrownUpdateStats(fighter_gobj, lr, script_id, is_proc_status);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowUpdateReleaseCount++;
        gNdsStageMPPassiveLoopThrowUpdateReleaseScriptID = script_id;
        gNdsStageMPPassiveLoopThrowUpdateReleaseLR = lr;
        ndsBaseFTCommonThrownReleaseThrownUpdateStats(
            fighter_gobj, lr, script_id, is_proc_status);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowReleaseUpdateStatsCount++;
        gNdsStageMPPassiveLoopThrowReleaseScriptID = script_id;
        ndsBaseFTCommonThrownReleaseThrownUpdateStats(
            fighter_gobj, lr, script_id, is_proc_status);
        return;
    }
    (void)fighter_gobj;
    (void)lr;
    (void)script_id;
    (void)is_proc_status;
}

void ftCommonCaptureShoulderedSetStatus(GObj *fighter_gobj)
{
#if NDS_P2_DONKEY
    ndsBaseFTCommonCaptureShoulderedSetStatus(fighter_gobj);
#else
    (void)fighter_gobj;
#endif
}

#if NDS_P2_DONKEY
void ftCommonCaptureShoulderedProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonCaptureShoulderedProcInterrupt(fighter_gobj);
}
#endif

#if !NDS_P2_DONKEY
void ftDonkeyThrowFWaitSetStatus(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}
#endif

void ftCommonThrownReleaseFighterLoseGrip(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonThrownReleaseFighterLoseGrip(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        if (ndsFTCommonThrownLoseGripCanCallOriginal(fighter_gobj) != FALSE)
        {
            ndsFTCommonThrownLoseGripRecordOriginal(fighter_gobj);
            ndsBaseFTCommonThrownReleaseFighterLoseGrip(fighter_gobj);
        }
        else
        {
            ndsFTCommonThrownReleaseFighterLoseGripBounded(fighter_gobj,
                                                          NULL);
        }
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowDeadResultActive != FALSE))
    {
        ndsBaseFTCommonThrownReleaseFighterLoseGrip(fighter_gobj);
        return;
    }
    (void)fighter_gobj;
}

/* BattleShip dFTCommonDataHandicapTable from ft/ftcommondata.c:4-78.
 * The rest of that TU is item/audio data outside this combat slice. */
static const f32 sNDSFTCommonDataHandicapTable[][2] =
{
    { 0.55F, 1.8181818F },
    { 0.6F, 15.0F / 9.0F },
    { 0.7F, 10.0F / 7.0F },
    { 0.75F, 12.0F / 9.0F },
    { 0.8F, 11.25F / 9.0F },
    { 0.9F, 10.0F / 9.0F },
    { 0.95F, 10.0F / 9.5F },
    { 1.0F, 9.0F / 9.0F },
#if defined(REGION_US)
    { 1.09F, 0.9174312F },
    { 0.5F, 45.0F / 9.0F },
    { 0.55F, 36.0F / 9.0F },
    { 0.6F, 30.0F / 9.0F },
    { 0.65F, 20.0F / 7.0F },
    { 0.7F, 22.5F / 9.0F },
    { 0.75F, 1.8181818F },
    { 0.8F, 1.6949153F },
    { 0.85F, 1.5873016F },
    { 0.9F, 1.4925373F },
    { 0.95F, 1.4084507F },
    { 0.65F, 30.0F / 9.0F },
    { 0.7F, 20.0F / 7.0F },
    { 0.74F, 22.5F / 9.0F },
    { 0.77F, 20.0F / 9.0F },
    { 0.8F, 18.0F / 9.0F },
    { 1.0F, 4.0F / 7.0F },
    { 1.05F, 0.53763443F },
    { 1.1F, 0.5128205F },
    { 1.15F, 0.4761905F },
    { 1.23F, 0.45045045F },
    { 1.05F, 0.43478262F },
    { 1.1F, 3.0F / 7.5F },
    { 1.15F, (10.0F / 3.0F) / 9.0F },
    { 1.2F, 0.35714287F },
    { 1.25F, 3.0F / 9.0F },
#else
    { 1.1F, 0.9090909F },
    { 0.6F, 45.0F / 9.0F },
    { 0.6F, 36.0F / 9.0F },
    { 0.6F, 30.0F / 9.0F },
    { 0.6F, 20.0F / 7.0F },
    { 0.6F, 22.5F / 9.0F },
    { 0.9F, 1.8181818F },
    { 0.95F, 1.6949153F },
    { 1.0F, 1.5873016F },
    { 1.0F, 1.4925373F },
    { 1.0F, 1.4084507F },
    { 0.7F, 30.0F / 9.0F },
    { 0.75F, 25.0F / 8.0F },
    { 0.8F, 20.0F / 7.0F },
    { 1.0F, 25.0F / 9.5F },
    { 0.9F, 22.5F / 9.0F },
    { 1.05F, 4.0F / 7.0F },
    { 1.09F, 0.53763443F },
    { 1.13F, 0.5128205F },
    { 1.17F, 0.4761905F },
    { 1.23F, 0.45045045F },
    { 0.95F, 0.43478262F },
    { 1.0F, 3.0F / 7.5F },
    { 1.05F, (10.0F / 3.0F) / 9.0F },
    { 1.1F, 0.35714287F },
    { 1.1F, 3.0F / 9.0F },
#endif
    { 0.9F, 9.0F / 9.0F },
    { 1.0F, 9.0F / 9.0F },
    { 1.1F, 9.0F / 9.0F },
    { 1.22F, 9.0F / 9.0F },
    { 1.5F, 9.0F / 9.0F },
#if defined(REGION_US)
    { 1.08F, (200.0F / 24.0F) / 9.0F }
#else
    { 1.0F, 9.0F / 9.0F }
#endif
};

f32 ftParamGetCommonKnockback(s32 percent_damage, s32 recent_damage,
                              s32 hit_damage, s32 knockback_weight,
                              s32 knockback_scale, s32 knockback_base,
                              f32 weight, s32 attack_handicap,
                              s32 defend_handicap)
{
    const s32 default_handicap = 9;
    const s32 handicap_min = 1;
    const s32 handicap_max =
        (s32)ARRAY_COUNT(sNDSFTCommonDataHandicapTable);
    f32 damage_ratio =
        (gSCManagerBattleState != NULL) ?
            ((f32)gSCManagerBattleState->damage_ratio * 0.01F) : 1.0F;
    f32 knockback;

    if ((attack_handicap < handicap_min) || (attack_handicap > handicap_max))
    {
        attack_handicap = default_handicap;
    }
    if ((defend_handicap < handicap_min) || (defend_handicap > handicap_max))
    {
        defend_handicap = default_handicap;
    }
    if (knockback_weight != 0)
    {
        knockback = ((((((1.0F +
            (10.0F * (f32)knockback_weight * 0.05F)) * weight * 1.4F) +
            18.0F) * ((f32)knockback_scale * 0.01F)) +
            (f32)knockback_base) * damage_ratio *
            sNDSFTCommonDataHandicapTable[attack_handicap - 1][0]) *
            sNDSFTCommonDataHandicapTable[defend_handicap - 1][1];
    }
    else
    {
        f32 damage_add = (f32)(percent_damage + recent_damage);

        knockback = ((((((((damage_add * 0.1F) +
            (damage_add * (f32)hit_damage * 0.05F)) * weight * 1.4F) +
            18.0F) * ((f32)knockback_scale * 0.01F)) +
            (f32)knockback_base) * damage_ratio *
            sNDSFTCommonDataHandicapTable[attack_handicap - 1][0]) *
            sNDSFTCommonDataHandicapTable[defend_handicap - 1][1]);
    }
    if (knockback >= 2500.0F)
    {
        knockback = 2500.0F;
    }
    if ((gSCManagerBackupData.error_flags & LBBACKUP_ERROR_RANDOMKNOCKBACK) !=
        0)
    {
        knockback = syUtilsRandFloat() * 200.0F;
    }
    return knockback;
}

s32 ftParamGetHitLag(s32 damage, s32 status_id, f32 hitlag_mul)
{
#if defined(REGION_US)
    s32 hitlag_tics =
        (s32)((s32)(((f32)damage * (1.0F / 3.0F)) + 5.0F) * hitlag_mul);
#else
    s32 hitlag_tics =
        (s32)((s32)(((f32)damage * (1.0F / 3.0F)) + 4.0F) * hitlag_mul);
#endif

    /* BattleShip ftparam.c:1511-1523 applies hitlag_mul before the crouch
     * reduction, with an integer truncation at each assignment. The old DS
     * helper moved hitlag_mul after the crouch branch, which is not algebraically
     * equivalent once those truncations are included. */
    if ((status_id == nFTCommonStatusSquat) ||
        (status_id == nFTCommonStatusSquatWait))
    {
        hitlag_tics = (s32)((f32)hitlag_tics * (2.0F / 3.0F));
    }
    return hitlag_tics;
}

void ftParamSetDamageShuffle(FTStruct *fp, sb32 is_electric, s32 damage,
                             s32 status_id, f32 hitlag_mul)
{
    static const u8 sShuffleFrameIndexMax[2] = { 4u, 3u };
    u32 index = (is_electric != FALSE) ? 1u : 0u;

    if (fp == NULL)
    {
        return;
    }
    fp->shuffle_tics =
        (s32)((f32)ftParamGetHitLag(damage, status_id, hitlag_mul) * 1.3F);
    fp->shuffle_frame_index = 0;
    fp->is_shuffle_electric = (index != 0u) ? TRUE : FALSE;
    fp->shuffle_index_max = sShuffleFrameIndexMax[index];
}

s32 ftParamGetStaledDamage(s32 player, s32 damage, s32 attack_id,
                           u16 motion_count)
{
    extern f32 ftParamGetStale(s32 player, s32 attack_id, s32 motion_count);
    f32 stale = ftParamGetStale(player, attack_id, motion_count);

    /* BattleShip ftparam.c:1608-1617. This is the actual damage-side consumer
     * of the per-player stale queue restored by P2-2. Leaving this as the old
     * identity stub meant the queue could be perfectly maintained and still
     * have zero gameplay effect. Preserve the source's +0.999F rounding before
     * the implicit integer conversion. */
    if (stale != 1.0F)
    {
        damage = (damage * stale) + 0.999F;
    }
    return damage;
}

s32 ftParamGetBestHitStatusPart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 hitstatus_base;
    s32 hitstatus_best;
    s32 i;

    if (fp == NULL)
    {
        return nGMHitStatusNone;
    }

    /* BattleShip ftparam.c:645-672. Per-hurtbox state participates in the
     * fighter's effective collision status; the old DS all-status helper read
     * only fp->hitstatus and therefore ignored part invincibility/intangibility. */
    hitstatus_base = fp->hitstatus;
    hitstatus_best = fp->damage_colls[0].hitstatus;

    if (hitstatus_best != nGMHitStatusNormal)
    {
        for (i = 1; i < (s32)ARRAY_COUNT(fp->damage_colls); i++)
        {
            FTDamageColl *damage_coll = &fp->damage_colls[i];

            if (damage_coll->hitstatus == nGMHitStatusNone)
            {
                break;
            }
            else if (hitstatus_best > damage_coll->hitstatus)
            {
                hitstatus_best = damage_coll->hitstatus;
            }
        }
    }
    if (hitstatus_base < hitstatus_best)
    {
        hitstatus_base = hitstatus_best;
    }
    return hitstatus_base;
}

s32 ftParamGetBestHitStatusAll(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 hitstatus_best;

    if (fp == NULL)
    {
        return nGMHitStatusNone;
    }

    /* BattleShip ftparam.c:676-689. Respawn/star and timed special hitstatus
     * are global overrides on top of the best per-part status. Source throws,
     * captures and ground hazards all consume this aggregate result. */
    hitstatus_best = ftParamGetBestHitStatusPart(fighter_gobj);
    if (hitstatus_best < fp->star_hitstatus)
    {
        hitstatus_best = fp->star_hitstatus;
    }
    if (hitstatus_best < fp->special_hitstatus)
    {
        hitstatus_best = fp->special_hitstatus;
    }
    return hitstatus_best;
}

#ifndef FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_LOW
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_LOW 120.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID_LOW 150.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID 200.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID_HIGH 300.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_HIGH 600.0F
#define FTCOMMON_DAMAGE_EFFECT_WAIT_LOW 0
#define FTCOMMON_DAMAGE_EFFECT_WAIT_MID_LOW 8
#define FTCOMMON_DAMAGE_EFFECT_WAIT_MID 5
#define FTCOMMON_DAMAGE_EFFECT_WAIT_MID_HIGH 3
#define FTCOMMON_DAMAGE_EFFECT_WAIT_HIGH 2
#define FTCOMMON_DAMAGE_EFFECT_WAIT_DEFAULT 1
#define FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_ANGLE_LOW F_CLC_DTOR32(75.0F)
#define FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_ANGLE_HIGH F_CLC_DTOR32(115.0F)
#define FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_KNOCKBACK_MUL 0.8F
#define FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH 160.0F
#define FTCOMMON_DAMAGE_FIGHTER_FLYTOP_ANGLE_LOW F_CLC_DTOR32(70.0F)
#define FTCOMMON_DAMAGE_FIGHTER_FLYTOP_ANGLE_HIGH F_CLC_DTOR32(110.0F)
#define FTCOMMON_DAMAGE_FIGHTER_FLYROLL_DAMAGE_MIN 100
#define FTCOMMON_DAMAGE_FIGHTER_FLYROLL_RANDOM_CHANCE 0.5F
#define FTCOMMON_DAMAGE_FIGHTER_PLAYERTAG_KNOCKBACK_MIN 130.0F
#define FTCOMMON_DAMAGE_FIGHTER_PLAYERTAG_HIDE_FRAMES 10
#define FTCOMMON_WALLDAMAGE_INTANGIBLE_TIMER 15
#endif

static f32 ndsVectorAngleDiff3D(const Vec3f *a, const Vec3f *b)
{
    f32 cross_x;
    f32 cross_y;
    f32 cross_z;
    f32 cross_mag;
    f32 dot;

    if ((a == NULL) || (b == NULL))
    {
        return 0.0F;
    }

    cross_x = (a->y * b->z) - (a->z * b->y);
    cross_y = (a->z * b->x) - (a->x * b->z);
    cross_z = (a->x * b->y) - (a->y * b->x);
    cross_mag = sqrtf((cross_x * cross_x) + (cross_y * cross_y) +
                      (cross_z * cross_z));
    dot = (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
    return atan2f(cross_mag, dot);
}

static f32 ndsFTCommonDamageGetKnockbackAngle(s32 angle_i, sb32 ga,
                                              f32 knockback)
{
    f32 angle_f;

    if (angle_i != 361)
    {
        return F_CLC_DTOR32((f32)angle_i);
    }
    if (ga == nMPKineticsAir)
    {
        return F_CLC_DTOR32(43.0F);
    }
    if (knockback < 32.0F)
    {
        return 0.0F;
    }

    angle_f = F_CLC_DTOR32((((knockback - 32.0F) / 0.099998474F) *
                            42.5F) + 1.0F);
    if (angle_f > F_CLC_DTOR32(42.5F))
    {
        angle_f = F_CLC_DTOR32(42.5F);
    }
    return angle_f;
}

static void ndsFTCommonDamageSetDustEffectInterval(FTStruct *fp)
{
    f32 vel;

    if (fp == NULL)
    {
        return;
    }
    vel = (fp->ga == nMPKineticsAir) ?
        sqrtf((fp->physics.vel_damage_air.x * fp->physics.vel_damage_air.x) +
              (fp->physics.vel_damage_air.y * fp->physics.vel_damage_air.y) +
              (fp->physics.vel_damage_air.z * fp->physics.vel_damage_air.z)) :
        ABSF(fp->physics.vel_damage_ground);

    if (vel < FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_LOW)
    {
        fp->status_vars.common.damage.dust_effect_int =
            FTCOMMON_DAMAGE_EFFECT_WAIT_LOW;
    }
    else if (vel < FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID_LOW)
    {
        fp->status_vars.common.damage.dust_effect_int =
            FTCOMMON_DAMAGE_EFFECT_WAIT_MID_LOW;
    }
    else if (vel < FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID)
    {
        fp->status_vars.common.damage.dust_effect_int =
            FTCOMMON_DAMAGE_EFFECT_WAIT_MID;
    }
    else if (vel < FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID_HIGH)
    {
        fp->status_vars.common.damage.dust_effect_int =
            FTCOMMON_DAMAGE_EFFECT_WAIT_MID_HIGH;
    }
    else if (vel < FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_HIGH)
    {
        fp->status_vars.common.damage.dust_effect_int =
            FTCOMMON_DAMAGE_EFFECT_WAIT_HIGH;
    }
    else
    {
        fp->status_vars.common.damage.dust_effect_int =
            FTCOMMON_DAMAGE_EFFECT_WAIT_DEFAULT;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        sNdsFighterDashRunDamageSetupDustCount++;
    }
}

/* The dash-run damage proof, split out of the crowd stub that used to be the
 * only body of ftPublicCommonCheck. It records; it never decided anything. The
 * real actor (battleship_ftpublic.c) calls this and then the source check, so
 * importing the crowd cannot silently retire a proof. */
void ndsFighterDashRunRecordPublicCheck(GObj *fighter_gobj, f32 knockback,
                                        sb32 is_force_curr_knockback)
{
    (void)fighter_gobj;
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        sNdsFighterDashRunDamageSetupPublicCount++;
        sNdsFighterDashRunDamagePublicCheckCount++;
        sNdsFighterDashRunDamagePublicLastKnockbackMilli =
            ndsFloatToMilliSigned(knockback);
        if (is_force_curr_knockback != FALSE)
        {
            sNdsFighterDashRunDamagePublicForceCount++;
        }
    }
}

/* Declared unconditionally, defined either here or in battleship_ftpublic.c.
 * `ft/fighter.h` declares ftPublicMakeActor and not this one, so at
 * NDS_IMPORT_BATTLESHIP_FT_PUBLIC=1 the definition below disappears and the
 * call at ndsFTCommonDamageSetPublic has no prototype in scope -- which is a
 * hard error, and the reason the flag had never been compiled. */
void ftPublicCommonCheck(GObj *fighter_gobj, f32 knockback,
                         sb32 is_force_curr_knockback);

#if !NDS_IMPORT_BATTLESHIP_FT_PUBLIC
void ftPublicCommonCheck(GObj *fighter_gobj, f32 knockback,
                         sb32 is_force_curr_knockback)
{
    ndsFighterDashRunRecordPublicCheck(fighter_gobj, knockback,
                                       is_force_curr_knockback);
}
#endif

GObj *ftParamGetPlayerNumGObj(s32 player_num)
{
    return ndsFighterGetPlayerNumGObj(player_num);
}

static void ndsFTCommonDamageSetPublic(FTStruct *fp, f32 knockback,
                                       f32 angle)
{
    GObj *attacker_gobj;
    sb32 is_force_curr_knockback = FALSE;

    if (fp == NULL)
    {
        return;
    }
    fp->status_vars.common.damage.public_knockback = knockback;
    fp->public_knockback = 0.0F;
    if ((angle > FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_ANGLE_LOW) &&
        (angle < FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_ANGLE_HIGH))
    {
        fp->status_vars.common.damage.public_knockback *=
            FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_KNOCKBACK_MUL;
    }
    attacker_gobj = ndsFighterGetPlayerNumGObj(fp->damage_player_num);
    if (attacker_gobj != NULL)
    {
        FTStruct *attacker_fp = ftGetStruct(attacker_gobj);
        if ((attacker_fp != NULL) &&
            (attacker_fp->public_knockback >=
                FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH))
        {
            is_force_curr_knockback = TRUE;
        }
    }
    ftPublicCommonCheck(fp->fighter_gobj,
                        fp->status_vars.common.damage.public_knockback,
                        is_force_curr_knockback);
}

void ftCommonDamageSetPublic(FTStruct *fp, f32 knockback, f32 angle)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageSetPublic(fp, knockback, angle);
    return;
#endif

    if (fp == NULL)
    {
        return;
    }

    ndsBaseFTCommonDamageSetPublic(fp, knockback, angle);
}

void ftCommonDamageSetDustEffectInterval(FTStruct *fp)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageSetDustEffectInterval(fp);
    return;
#endif

    if (fp == NULL)
    {
        return;
    }

    ndsBaseFTCommonDamageSetDustEffectInterval(fp);
}

f32 ftCommonDamageGetKnockbackAngle(s32 angle_i, sb32 ga, f32 knockback)
{
    return ndsBaseFTCommonDamageGetKnockbackAngle(angle_i, ga, knockback);
}

s32 ftCommonDamageGetDamageLevel(f32 hitstun)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonDamageGetDamageLevel(hitstun);
#endif

    return ndsBaseFTCommonDamageGetDamageLevel(hitstun);
}

sb32 ftCommonDamageCheckCatchResist(FTStruct *fp)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonDamageCheckCatchResist(fp);
#endif

    if (fp == NULL)
    {
        return FALSE;
    }

    return ndsBaseFTCommonDamageCheckCatchResist(fp);
}

sb32 ftCommonDamageCheckCaptureKeepHold(FTStruct *fp)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonDamageCheckCaptureKeepHold(fp);
#endif

    if (fp == NULL)
    {
        return FALSE;
    }

    return ndsBaseFTCommonDamageCheckCaptureKeepHold(fp);
}

static sb32 ndsFTCommonDamageCheckElementSetColAnim(GObj *fighter_gobj,
                                                    s32 element,
                                                    s32 damage_level)
{
    sb32 result;

    switch (element)
    {
    case nGMHitElementFire:
        result = ftParamCheckSetFighterColAnimID(
            fighter_gobj,
            damage_level + nGMColAnimFighterDamageFireStart, 0);
        break;
    case nGMHitElementElectric:
        result = ftParamCheckSetSkeletonColAnimID(fighter_gobj,
                                                  damage_level);
        break;
    case nGMHitElementFreezing:
        result = ftParamCheckSetFighterColAnimID(
            fighter_gobj,
            damage_level + nGMColAnimFighterDamageIceStart, 0);
        break;
    default:
        result = ftParamCheckSetFighterColAnimID(
            fighter_gobj, nGMColAnimFighterDamageCommon, 0);
        break;
    }
    if ((result != FALSE) &&
        (ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        sNdsFighterDashRunDamageSetupColAnimCount++;
    }
    return result;
}

sb32 ftCommonDamageCheckElementSetColAnim(GObj *fighter_gobj, s32 element,
                                          s32 damage_level)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonDamageCheckElementSetColAnim(fighter_gobj, element, damage_level);
#endif

    if (fighter_gobj == NULL)
    {
        return FALSE;
    }

    return ndsBaseFTCommonDamageCheckElementSetColAnim(fighter_gobj, element,
                                                       damage_level);
}

void ifScreenFlashSetColAnimID(s32 colanim_id, s32 colanim_duration)
{
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        sNdsFighterDashRunDamageScreenFlashLastID = colanim_id;
        sNdsFighterDashRunDamageScreenFlashLastDuration = colanim_duration;
        sNdsFighterDashRunDamageSetupScreenFlashCount++;
    }
    ndsBattleShipIFScreenFlashSetColAnimID(colanim_id, colanim_duration);
}

static void ndsFTCommonDamageCheckMakeScreenFlash(f32 knockback, s32 element)
{
    if (knockback <= FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH)
    {
        return;
    }
    switch (element)
    {
    case nGMHitElementFire:
        ifScreenFlashSetColAnimID(nGMColAnimScreenFlashDamageFire, 0);
        break;
    case nGMHitElementElectric:
        ifScreenFlashSetColAnimID(nGMColAnimScreenFlashDamageElectric, 0);
        break;
    case nGMHitElementFreezing:
        ifScreenFlashSetColAnimID(nGMColAnimScreenFlashDamageIce, 0);
        break;
    default:
        ifScreenFlashSetColAnimID(nGMColAnimScreenFlashDamageNormal, 0);
        break;
    }
}

void ftCommonDamageCheckMakeScreenFlash(f32 knockback, s32 element)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageCheckMakeScreenFlash(knockback, element);
    return;
#endif

    ndsBaseFTCommonDamageCheckMakeScreenFlash(knockback, element);
}

void ftCommonDamageInitDamageVars(GObj *fighter_gobj, s32 status_id_replace,
                                  s32 damage, f32 knockback,
                                  s32 angle_start, s32 damage_lr,
                                  s32 damage_index, s32 element,
                                  s32 damage_player_num, s32 arg9,
                                  sb32 unk_bool, sb32 is_public)
{
    NDS_FREEZE_DIAGNOSTICS_MARK(NDS_FREEZE_BREADCRUMB_DAMAGE_ENTER);
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageInitDamageVars(fighter_gobj, status_id_replace, damage, knockback, angle_start, damage_lr, damage_index, element, damage_player_num, arg9, unk_bool, is_public);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 hitstun_tics;
    s32 damage_level;
    s32 status_id_set;
    s32 status_id_var;
    s32 damage_index_safe;
    f32 angle_end;
    f32 vel_x;
    f32 vel_y;
    Vec3f vel_damage;
    f32 angle_diff;

    if (fp != NULL)
    {
        if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
            (sNdsFighterDashRunDamageStatusSetupActive != FALSE) &&
            (sNdsFighterDashRunDamageOriginalInitActive != FALSE))
        {
            GObj *attacker_gobj =
                ndsFighterGetPlayerNumGObj(damage_player_num);
            FTStruct *attacker_fp =
                (attacker_gobj != NULL) ? ftGetStruct(attacker_gobj) : NULL;
            s32 attacker_attack_count =
                (attacker_fp != NULL) ? attacker_fp->attack_count : 0;
            u32 colanim_count =
                sNdsFighterDashRunDamageSetupColAnimCount;
            s32 colanim_id = sNdsFighterDashRunDamageColAnimLastID;
            s32 colanim_duration =
                sNdsFighterDashRunDamageColAnimLastDuration;
            s32 skeleton_colanim_level =
                sNdsFighterDashRunDamageSkeletonColAnimLastLevel;
            u32 screen_flash_count =
                sNdsFighterDashRunDamageSetupScreenFlashCount;

            fp->damage_queue = damage;
            fp->damage_knockback = knockback;
            fp->damage_player = damage_player_num;
            fp->damage_player_num = damage_player_num;
            fp->damage_angle = angle_start;
            fp->damage_lr = damage_lr;
            fp->damage_index = damage_index;
            fp->damage_element = element;
            fp->hit_lr = damage_lr;
            sNdsFighterDashRunDamageOriginalInitCount++;
            ndsBaseFTCommonDamageInitDamageVars(
                fighter_gobj, status_id_replace, damage, knockback,
                angle_start, damage_lr, damage_index, element,
                damage_player_num, arg9, unk_bool, is_public);
            if ((attacker_fp != NULL) &&
                (attacker_fp->attack_count == (attacker_attack_count + 1)) &&
                (attacker_fp->attack_knockback == knockback))
            {
                sNdsFighterDashRunDamageSetupAttackerCount++;
            }
            if ((damage != 0) &&
                (sNdsFighterDashRunDamageSetupColAnimCount ==
                    colanim_count) &&
                ((sNdsFighterDashRunDamageColAnimLastID != colanim_id) ||
                 (sNdsFighterDashRunDamageColAnimLastDuration !=
                    colanim_duration) ||
                 (sNdsFighterDashRunDamageSkeletonColAnimLastLevel !=
                    skeleton_colanim_level)))
            {
                sNdsFighterDashRunDamageSetupColAnimCount++;
            }
            if ((knockback > FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH) &&
                (sNdsFighterDashRunDamageSetupScreenFlashCount ==
                    screen_flash_count))
            {
                ndsFTCommonDamageCheckMakeScreenFlash(knockback, element);
            }
            goto record_throw_release_damage_init;
        }
        angle_end = ndsFTCommonDamageGetKnockbackAngle(
            angle_start, fp->ga, knockback);
        vel_x = __cosf(angle_end) * knockback;
        vel_y = __sinf(angle_end) * knockback;
        hitstun_tics = ftParamGetHitStun(knockback);
        if (hitstun_tics == 0.0F)
        {
            hitstun_tics = 1.0F;
        }
        damage_level = ndsFTCommonDamageGetDamageLevel(hitstun_tics);
        if (status_id_replace != -1)
        {
            damage_level = 3;
        }
        damage_index_safe = damage_index;
        if (damage_index_safe < 0)
        {
            damage_index_safe = 0;
        }
        if (damage_index_safe >= 3)
        {
            damage_index_safe = 2;
        }
        status_id_set =
            ndsFTCommonDamageSelectStatus(damage_level, damage_index_safe,
                                          fp->ga == nMPKineticsAir);
        status_id_var = status_id_set;
        fp->damage_queue = damage;
        fp->damage_knockback = knockback;
        fp->damage_player = damage_player_num;
        fp->damage_player_num = damage_player_num;
        fp->hit_lr = damage_lr;
        fp->lr = damage_lr;
        fp->status_vars.common.damage.hitstun_tics = (s32)hitstun_tics;
        fp->status_vars.common.damage.public_knockback = knockback;
        fp->status_vars.common.damage.is_knockback_over =
            (knockback >= 65000.0F) ? TRUE : FALSE;
        fp->physics.vel_air.x = 0.0F;
        fp->physics.vel_air.y = 0.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->physics.vel_ground.x = 0.0F;
        if (fp->ga == nMPKineticsAir)
        {
            fp->physics.vel_damage_air.x = -vel_x * fp->lr;
            fp->physics.vel_damage_air.y = vel_y;
            fp->physics.vel_damage_air.z = 0.0F;
            fp->physics.vel_damage_ground = 0.0F;
        }
        else
        {
            vel_damage.x = -vel_x * fp->lr;
            vel_damage.y = vel_y;
            vel_damage.z = 0.0F;
            angle_diff =
                ndsVectorAngleDiff3D(&fp->coll_data.floor_angle, &vel_damage);
            if (angle_diff < F_CST_DTOR32(90.0F))
            {
                status_id_set = ndsFTCommonDamageSelectStatus(
                    damage_level, damage_index_safe, FALSE);
                status_id_var = status_id_set;
                mpCommonSetFighterAir(fp);
                fp->physics.vel_damage_air = vel_damage;
                fp->physics.vel_damage_ground = 0.0F;
            }
            else if (damage_level == 3)
            {
                status_id_set = ndsFTCommonDamageSelectStatus(
                    damage_level, damage_index_safe, FALSE);
                status_id_var = status_id_set;
                mpCommonSetFighterAir(fp);
                fp->physics.vel_damage_air.x = vel_damage.x;
                fp->physics.vel_damage_air.y =
                    (angle_diff > F_CST_DTOR32(100.0F)) ?
                        (-vel_damage.y * 0.8F) : vel_damage.y;
                fp->physics.vel_damage_air.z = 0.0F;
                fp->physics.vel_damage_ground = 0.0F;
                if (angle_diff > F_CST_DTOR32(100.0F))
                {
                    ftParamMakeEffect(fighter_gobj, nEFKindImpactWave,
                                      nFTPartsJointTopN, NULL, NULL,
                                      fp->lr, 0, 0);
                    ftParamMakeEffect(fighter_gobj, nEFKindQuakeMag0,
                                      nFTPartsJointTopN, NULL, NULL,
                                      fp->lr, 0, 0);
                }
            }
            else
            {
                fp->physics.vel_damage_ground = -vel_x * fp->lr;
                fp->physics.vel_damage_air.x =
                    fp->coll_data.floor_angle.y *
                    fp->physics.vel_damage_ground;
                fp->physics.vel_damage_air.y =
                    -fp->coll_data.floor_angle.x *
                    fp->physics.vel_damage_ground;
                fp->physics.vel_damage_air.z = 0.0F;
            }
        }
        if ((damage_level == 3) && (fp->ga == nMPKineticsAir))
        {
            if ((angle_end > FTCOMMON_DAMAGE_FIGHTER_FLYTOP_ANGLE_LOW) &&
                (angle_end < FTCOMMON_DAMAGE_FIGHTER_FLYTOP_ANGLE_HIGH))
            {
                status_id_var = status_id_set = nFTCommonStatusDamageFlyTop;
            }
            else if ((fp->percent_damage >=
                        FTCOMMON_DAMAGE_FIGHTER_FLYROLL_DAMAGE_MIN) &&
                     (syUtilsRandFloat() <
                        FTCOMMON_DAMAGE_FIGHTER_FLYROLL_RANDOM_CHANCE))
            {
                status_id_var = status_id_set = nFTCommonStatusDamageFlyRoll;
            }
        }
        if (status_id_replace != -1)
        {
            status_id_set = status_id_replace;
        }
        if ((element == nGMHitElementElectric) &&
            ndsFTCommonDamageIsStatus(status_id_set))
        {
            status_id_var = status_id_set;
            status_id_set = (damage_level == 3) ?
                nFTCommonStatusDamageE2 : nFTCommonStatusDamageE1;
        }
        fp->status_vars.common.damage.status_id = status_id_set;
        fp->damage_knockback_stack = knockback;
        fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
        fp->tics_since_last_z = FTINPUT_ZTRIGLAST_TICS_MAX;
        ndsFTCommonDamageSetPublic(fp, knockback, angle_end);
        if (damage != 0)
        {
            (void)ndsFTCommonDamageCheckElementSetColAnim(
                fighter_gobj, element, damage_level);
        }
        ndsFTCommonDamageCheckMakeScreenFlash(knockback, element);

        if ((damage_level == 3) && (is_public != FALSE))
        {
            ftKirbySpecialNDamageCheckLoseCopy(fighter_gobj);
        }

        if (sNdsFighterDashRunDamageStatusSetupActive != FALSE)
        {
            ftMainSetStatus(fighter_gobj, status_id_set, 0.0F, 1.0F,
                            FTSTATUS_PRESERVE_DAMAGEPLAYER);
            ftMainPlayAnimEventsAll(fighter_gobj);
            fp->is_hitstun = TRUE;
            fp->proc_lagupdate = ftCommonDamageCommonProcLagUpdate;
        }
        if ((status_id_set == nFTCommonStatusDamageE1) ||
            (status_id_set == nFTCommonStatusDamageE2))
        {
            fp->proc_passive = ftCommonDamageSetStatus;
            fp->status_vars.common.damage.status_id = status_id_var;
        }
        else
        {
            fp->proc_passive = ftCommonDamageCheckSetInvincible;
        }
        if ((damage_level == 3) || (arg9 != FALSE))
        {
            ftParamMakeRumble(fp, 2, 0);
        }
        ndsFTCommonDamageSetDustEffectInterval(fp);
        if (fp->status_vars.common.damage.dust_effect_int != 0)
        {
            fp->status_vars.common.damage.dust_effect_int = 1;
        }
        if ((fp->attr != NULL) &&
            ((((hitstun_tics >= 80.0F) &&
               (fp->attr->damage_sfx != nSYAudioFGMVoiceEnd))) ||
             (unk_bool != FALSE)))
        {
            (void)func_800269C0_275C0(fp->attr->damage_sfx);
        }
        if ((damage_level == 3) &&
            (knockback >= FTCOMMON_DAMAGE_FIGHTER_PLAYERTAG_KNOCKBACK_MIN))
        {
            ftParamSetPlayerTagWait(
                fighter_gobj, FTCOMMON_DAMAGE_FIGHTER_PLAYERTAG_HIDE_FRAMES);
        }
        fp->status_vars.common.damage.coll_mask_curr = 0;
        {
            GObj *attacker_gobj =
                ndsFighterGetPlayerNumGObj(damage_player_num);
            if (attacker_gobj != NULL)
            {
                FTStruct *attacker_fp = ftGetStruct(attacker_gobj);
                if (attacker_fp != NULL)
                {
                    attacker_fp->attack_count++;
                    attacker_fp->attack_knockback = knockback;
                    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
                        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
                    {
                        sNdsFighterDashRunDamageSetupAttackerCount++;
                    }
                }
            }
        }
    }
record_throw_release_damage_init:
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowReleaseDamageInitCount++;
        gNdsStageMPPassiveLoopThrowReleaseDamageInitDamage = (u32)damage;
        gNdsStageMPPassiveLoopThrowReleaseKnockbackMilli =
            ndsFloatToMilliSigned(knockback);
        gNdsStageMPPassiveLoopThrowReleaseLR = damage_lr;
        (void)angle_start;
    }
}

void ftCommonDamageGotoDamageStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageGotoDamageStatus(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        sNdsFighterDashRunDamageOriginalGotoCount++;
        ndsBaseFTCommonDamageGotoDamageStatus(fighter_gobj);
        if (fp->proc_update == ndsBaseFTCommonDamageCommonProcUpdate)
        {
            fp->proc_update = ftCommonDamageCommonProcUpdate;
        }
        else if (fp->proc_update == ndsBaseFTCommonDamageAirCommonProcUpdate)
        {
            fp->proc_update = ftCommonDamageAirCommonProcUpdate;
        }
        if (fp->proc_interrupt == ndsBaseFTCommonDamageCommonProcInterrupt)
        {
            fp->proc_interrupt = ftCommonDamageCommonProcInterrupt;
        }
        else if (fp->proc_interrupt ==
                 ndsBaseFTCommonDamageAirCommonProcInterrupt)
        {
            fp->proc_interrupt = ftCommonDamageAirCommonProcInterrupt;
        }
        if (fp->proc_physics == ndsBaseFTCommonDamageCommonProcPhysics)
        {
            fp->proc_physics = ftCommonDamageCommonProcPhysics;
        }
        if (fp->proc_lagupdate == ndsBaseFTCommonDamageCommonProcLagUpdate)
        {
            fp->proc_lagupdate = ftCommonDamageCommonProcLagUpdate;
        }
        if (fp->proc_map == ndsBaseFTCommonDamageAirCommonProcMap)
        {
            fp->proc_map = ftCommonDamageAirCommonProcMap;
        }
        if (fp->proc_passive == ndsBaseFTCommonDamageCheckSetInvincible)
        {
            fp->proc_passive = ftCommonDamageCheckSetInvincible;
        }
        else if (fp->proc_passive == ndsBaseFTCommonDamageSetStatus)
        {
            fp->proc_passive = ftCommonDamageSetStatus;
        }
        return;
    }
    if (fp->is_cliff_hold != FALSE)
    {
        fp->cliffcatch_wait = FTCOMMON_CLIFF_CATCH_WAIT;
    }
    if (fp->damage_element == nGMHitElementSleep)
    {
        ftCommonFuraSleepSetStatus(fighter_gobj);
        return;
    }
    ftCommonDamageInitDamageVars(fighter_gobj, -1, fp->damage_queue,
                                 fp->damage_knockback, fp->damage_angle,
                                 fp->damage_lr, fp->damage_index,
                                 fp->damage_element,
                                 fp->damage_player_num, FALSE, FALSE,
                                 TRUE);
}

#if NDS_IMPORT_BATTLESHIP_FTMAIN
extern void battleship_ftMainRunUpdateColAnim(GObj *fighter_gobj);

void ftMainRunUpdateColAnim(GObj *fighter_gobj)
{
    battleship_ftMainRunUpdateColAnim(fighter_gobj);
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        sNdsFighterDashRunDamageRunUpdateColAnimCount++;
    }
}
#else
void ftMainRunUpdateColAnim(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageStatusSetupActive != FALSE))
    {
        sNdsFighterDashRunDamageRunUpdateColAnimCount++;
    }
}
#endif

void ftCommonDamageUpdateDamageColAnim(GObj *fighter_gobj, f32 knockback,
                                       s32 element)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageUpdateDamageColAnim(fighter_gobj, knockback, element);
    return;
#endif

    if (fighter_gobj == NULL)
    {
        return;
    }

    ndsBaseFTCommonDamageUpdateDamageColAnim(fighter_gobj, knockback,
                                             element);
}

void ftCommonDamageSetDamageColAnim(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageSetDamageColAnim(fighter_gobj);
    return;
#endif

    FTStruct *fp;

    if (fighter_gobj == NULL)
    {
        return;
    }

    fp = ftGetStruct(fighter_gobj);
    if (fp == NULL)
    {
        return;
    }

    ndsBaseFTCommonDamageSetDamageColAnim(fighter_gobj);
}

void ftCommonDamageUpdateMain(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageUpdateMain(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }

    ndsBaseFTCommonDamageUpdateMain(fighter_gobj);
}

void ftParamUpdate1PGameDamageStats(FTStruct *fp, s32 damage_player,
                                    s32 damage_object_class,
                                    s32 damage_object_kind, u16 flags,
                                    u16 damage_stat_count)
{
    if (fp != NULL)
    {
        /* BattleShip ftparam.c:1757-1780. These first fields are ordinary
         * gameplay attribution despite the function's 1P-oriented name; VS
         * death/scoring code consumes damage_player later.  Keep the attacker's
         * stat metadata in damage_stat_flags -- the old shim overwrote the
         * VICTIM'S live stat_flags, which can change its own next attack. */
        fp->damage_player = damage_player;
        fp->damage_object_class = damage_object_class;
        fp->damage_object_kind = damage_object_kind;
        fp->damage_count++;
        if ((damage_stat_count == 0u) ||
            (fp->damage_stat_count != damage_stat_count))
        {
            fp->damage_stat_flags.halfword = flags;
            fp->damage_stat_count = damage_stat_count;
            /* The source's remaining body updates 1P bonus counters only.
             * Those counters belong to P2-6 and are not present in the port;
             * there is intentionally no VS-side substitute here. */
        }
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowReleaseDamageStatsCount++;
    }
}

void ftCommonFuraSleepSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonFuraSleepSetStatus(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }

    ndsBaseFTCommonFuraSleepSetStatus(fighter_gobj);
}

void ftCommonTwisterSetStatus(GObj *fighter_gobj, GObj *tornado_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonTwisterSetStatus(fighter_gobj, tornado_gobj);
    return;
#endif

    if (ftGetStruct(fighter_gobj) == NULL)
    {
        return;
    }
    ndsBaseFTCommonTwisterSetStatus(fighter_gobj, tornado_gobj);
}

void ftCommonTaruCannProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *tarucann_gobj;
    DObj *fighter_root;
    DObj *tarucann_root;

    if (fp == NULL)
    {
        return;
    }
    tarucann_gobj = fp->status_vars.common.tarucann.tarucann_gobj;
    if (tarucann_gobj == NULL)
    {
        return;
    }
    fighter_root = DObjGetStruct(fighter_gobj);
    tarucann_root = DObjGetStruct(tarucann_gobj);
    if ((fighter_root == NULL) || (tarucann_root == NULL))
    {
        return;
    }
    fighter_root->translate.vec.f = tarucann_root->translate.vec.f;
}

void ftCommonTaruCannSetStatus(GObj *fighter_gobj, GObj *tarucann_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }

    ftParamStopVoiceRunProcDamage(fighter_gobj);
    if ((fp->item_gobj != NULL) &&
        (itGetStruct(fp->item_gobj)->weight == nITWeightHeavy))
    {
        ftSetupDropItem(fp);
    }
    if (fp->catch_gobj != NULL)
    {
        ftCommonThrownSetStatusDamageRelease(fp->catch_gobj);
        fp->catch_gobj = NULL;
    }
    else if (fp->capture_gobj != NULL)
    {
        ftCommonThrownDecideFighterLoseGrip(fp->capture_gobj, fighter_gobj);
    }

    ftMainSetStatus(fighter_gobj, nFTCommonStatusTaruCann, 0.0F, 0.0F,
                    FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftPhysicsStopVelAll(fighter_gobj);

    fp->status_vars.common.tarucann.shoot_wait = 0;
    fp->status_vars.common.tarucann.release_wait = 0;
    fp->status_vars.common.tarucann.tarucann_gobj = tarucann_gobj;

    ftParamSetHitStatusAll(fighter_gobj, nGMHitStatusIntangible);
    fp->is_invisible = TRUE;

    ftParamSetCaptureImmuneMask(fp, FTCATCHKIND_MASK_ALL);
    (void)func_800269C0_275C0(nSYAudioFGMJungleTaruCannEnter);
}

#if NDS_P2_STAGE_JUNGLE
/* P2-4s3. The fighter half of Congo Jungle's barrel cannon, transcribed from
 * decomp ft/ftcommon/ftcommontarucann.c:8-48 and :97-116. SetStatus and
 * ProcPhysics above were already here; these three are what made a captured
 * fighter sit in the cannon forever, because the status arm assigned
 * proc_update and proc_interrupt NULL for want of them.
 *
 * Constants are the source's own (ft/ftcommon.h:160-166): 180 frames before
 * the cannon fires on its own, 10 frames from armed to release, 16 frames
 * before it can be used again.
 */
#define NDS_FTCOMMON_TARUCANN_RELEASE_WAIT 180
#define NDS_FTCOMMON_TARUCANN_SHOOT_WAIT 10
#define NDS_FTCOMMON_TARUCANN_PICKUP_WAIT 16

/* decomp ft/ftparam.c:1478-1501, imported here because nothing in the port had
 * it and two stages need it: this cannon and Hyrule Castle's tornado. The
 * knockback_weight != 0 arm is the one a ground hazard takes -- its damage is
 * fixed rather than accumulated -- and the random-knockback debug arm is
 * transcribed with it rather than dropped, because gSCManagerBackupData's
 * error flags are real state this port already carries. */
f32 ftParamGetGroundHazardKnockback(s32 percent_damage, s32 recent_damage,
                                    s32 hit_damage, s32 knockback_weight,
                                    s32 knockback_scale, s32 knockback_base,
                                    f32 weight, s32 attack_handicap,
                                    s32 defend_handicap)
{
    f32 knockback;

    if (knockback_weight != 0)
    {
        knockback = ((((((1.0F + (10.0F * knockback_weight * 0.05F)) *
                         weight * 1.4F) + 18.0F) *
                       (knockback_scale * 0.01F)) + knockback_base) *
                     sNDSFTCommonDataHandicapTable[attack_handicap - 1][0]) *
                    sNDSFTCommonDataHandicapTable[defend_handicap - 1][1];
    }
    else
    {
        f32 damage_add = (f32)(percent_damage + recent_damage);

        knockback = (((((((damage_add * 0.1F) +
                          (damage_add * hit_damage * 0.05F)) *
                         weight * 1.4F) + 18.0F) *
                       (knockback_scale * 0.01F)) + knockback_base) *
                     sNDSFTCommonDataHandicapTable[attack_handicap - 1][0]) *
                    sNDSFTCommonDataHandicapTable[defend_handicap - 1][1];
    }
    if (knockback >= 2500.0F)
    {
        knockback = 2500.0F;
    }
    if ((gSCManagerBackupData.error_flags & LBBACKUP_ERROR_RANDOMKNOCKBACK) !=
        0u)
    {
        knockback = syUtilsRandFloat() * 200.0F;
    }
    return knockback;
}

/* decomp ftcommontarucann.c:97-116. The throw descriptor is read out of the
 * live ground data rather than from a port constant: map base plus the map
 * head offset gives the file, and llGRJungleMapTaruCannThrowHitDesc (0xBC in
 * decomp reloc_data.us.h:3933) is where the cannon's damage, angle and
 * knockback live inside it. */
void ftCommonTaruCannShootFighter(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTThrowHitDesc *tarucann;
    DObj *fighter_root;
    f32 knockback;
    s32 angle;

    if ((fp == NULL) || (gMPCollisionGroundData == NULL))
    {
        return;
    }
    tarucann = (FTThrowHitDesc *)(((uintptr_t)gMPCollisionGroundData -
                                   (uintptr_t)0x14u) + (uintptr_t)0xbcu);

    fighter_root = DObjGetStruct(fighter_gobj);
    if (fighter_root != NULL)
    {
        fighter_root->translate.vec.f.z = 0.0F;
    }

    knockback = ftParamGetGroundHazardKnockback(
        fp->percent_damage, tarucann->damage, tarucann->damage,
        tarucann->knockback_weight, tarucann->knockback_scale,
        tarucann->knockback_base, fp->attr->weight, 9, 9);

    angle = (s32)((F_CLC_RTOD32(grJungleTaruCannGetRotate()) * -fp->lr) +
                  90.0F);
    angle -= (angle / 360) * 360;

    ftCommonDamageInitDamageVars(fighter_gobj, nFTCommonStatusDamageFlyRoll,
                                 tarucann->damage, knockback, angle, fp->lr, 0,
                                 tarucann->element, 0, TRUE, TRUE, FALSE);
    ftParamUpdate1PGameDamageStats(fp, GMCOMMON_PLAYERS_MAX,
                                   nFTHitLogObjectGround,
                                   nGMHitEnvironmentTaruCann, 0, 0);

    fp->playertag_wait = 0;
    fp->tarucann_wait = NDS_FTCOMMON_TARUCANN_PICKUP_WAIT;
}

/* decomp ftcommontarucann.c:8-33. */
void ftCommonTaruCannProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    if (fp->status_vars.common.tarucann.shoot_wait != 0)
    {
        fp->status_vars.common.tarucann.shoot_wait--;

        if (fp->status_vars.common.tarucann.shoot_wait ==
            (NDS_FTCOMMON_TARUCANN_SHOOT_WAIT / 2))
        {
            (void)func_800269C0_275C0(nSYAudioFGMJungleTaruCannShoot);
        }
        if (fp->status_vars.common.tarucann.shoot_wait == 0)
        {
            ftCommonTaruCannShootFighter(fighter_gobj);
            return;
        }
    }
    fp->status_vars.common.tarucann.release_wait++;

    if ((fp->status_vars.common.tarucann.release_wait >=
         NDS_FTCOMMON_TARUCANN_RELEASE_WAIT) &&
        (fp->status_vars.common.tarucann.shoot_wait == 0))
    {
        fp->status_vars.common.tarucann.shoot_wait =
            NDS_FTCOMMON_TARUCANN_SHOOT_WAIT;

        grJungleTaruCannAddAnimShoot(
            fp->status_vars.common.tarucann.tarucann_gobj);
    }
}

/* decomp ftcommontarucann.c:37-47. Either face button fires early. */
void ftCommonTaruCannProcInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    if ((fp->status_vars.common.tarucann.shoot_wait == 0) &&
        ((fp->input.pl.button_tap &
          (fp->input.button_mask_a | fp->input.button_mask_b)) != 0))
    {
        fp->status_vars.common.tarucann.shoot_wait =
            NDS_FTCOMMON_TARUCANN_SHOOT_WAIT;

        grJungleTaruCannAddAnimShoot(
            fp->status_vars.common.tarucann.tarucann_gobj);
    }
}
#endif /* NDS_P2_STAGE_JUNGLE */

void ftCommonCaptureTrappedInitBreakoutVars(FTStruct *fp, s32 breakout_wait)
{
    if (fp != NULL)
    {
        fp->breakout_wait = breakout_wait;
        fp->breakout_lr = 0;
        fp->breakout_ud = 0;
    }
}

sb32 ftCommonCaptureTrappedUpdateBreakoutVars(FTStruct *fp)
{
    sb32 is_mash = FALSE;
    s32 breakout_lr_prev;
    s32 breakout_ud_prev;

    if (fp == NULL)
    {
        return FALSE;
    }

    if (((fp->input.pl.button_tap & fp->input.button_mask_a) != 0) ||
        ((fp->input.pl.button_tap & fp->input.button_mask_b) != 0) ||
        ((fp->input.pl.button_tap & fp->input.button_mask_z) != 0))
    {
        is_mash = TRUE;
        fp->breakout_wait--;
    }

    breakout_lr_prev = fp->breakout_lr;
    breakout_ud_prev = fp->breakout_ud;

    if (fp->input.pl.stick_range.x < -FTCOMMON_CAPTURE_MASH_STICK_RANGE_MIN)
    {
        fp->breakout_lr = -1;
    }
    if (fp->input.pl.stick_range.x > FTCOMMON_CAPTURE_MASH_STICK_RANGE_MIN)
    {
        fp->breakout_lr = +1;
    }
    if (fp->input.pl.stick_range.y < -FTCOMMON_CAPTURE_MASH_STICK_RANGE_MIN)
    {
        fp->breakout_ud = -1;
    }
    if (fp->input.pl.stick_range.y > FTCOMMON_CAPTURE_MASH_STICK_RANGE_MIN)
    {
        fp->breakout_ud = +1;
    }
    if ((fp->breakout_lr != breakout_lr_prev) ||
        (fp->breakout_ud != breakout_ud_prev))
    {
        is_mash = TRUE;
        fp->breakout_wait--;
    }
    return is_mash;
}

void ftParamUpdateDamage(FTStruct *fp, s32 damage)
{
    if (fp != NULL)
    {
        s32 percent_before = fp->percent_damage;
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
        if (((gNdsFighterNaturalMovesetPhase == 13u) ||
             (gNdsFighterNaturalMovesetPhase == 14u)) &&
            (gNdsFighterNaturalMovesetThrowDamageAfter == 0u) &&
            (gNdsFighterNaturalMovesetThrowDamageBefore == 0u))
        {
            gNdsFighterNaturalMovesetThrowDamageBefore =
                (u32)percent_before;
        }
#endif
        fp->percent_damage += damage;
        if (gSCManagerBattleState != NULL)
        {
            gSCManagerBattleState->players[fp->player].total_damage_all +=
                damage;
            if (fp->percent_damage > 999)
            {
                fp->percent_damage = 999;
            }
            gSCManagerBattleState->players[fp->player].stock_damage_all =
                fp->percent_damage;
        }
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
        if (((gNdsFighterNaturalMovesetPhase == 13u) ||
             (gNdsFighterNaturalMovesetPhase == 14u)) &&
            (damage > 0) &&
            ((u32)fp->percent_damage >
                gNdsFighterNaturalMovesetThrowDamageAfter))
        {
            gNdsFighterNaturalMovesetThrowDamageAfter =
                (u32)fp->percent_damage;
        }
#endif
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowReleaseDamageUpdateCount++;
    }
}

void ftParamUpdatePlayerBattleStats(s32 attack_player, s32 defend_player,
                                    s32 attack_damage)
{
    if ((gSCManagerBattleState != NULL) &&
        (attack_player != GMCOMMON_PLAYERS_MAX) &&
        (attack_player != defend_player))
    {
        gSCManagerBattleState->players[attack_player].total_damage_given +=
            attack_damage;
        gSCManagerBattleState->players[defend_player].
            total_damage_players[attack_player] += attack_damage;
        gSCManagerBattleState->players[defend_player].combo_damage_foe +=
            attack_damage;
        gSCManagerBattleState->players[defend_player].combo_count_foe++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowReleasePlayerStatsCount++;
    }
}

void ftParamUpdateStaleQueue(s32 attack_player, s32 defend_player,
                             s32 attack_id, u16 motion_count)
{
    if ((gSCManagerBattleState != NULL) &&
        (attack_player != (s32)ARRAY_COUNT(gSCManagerBattleState->players)) &&
        (attack_player != defend_player) &&
        (attack_player >= 0) &&
        (attack_player < (s32)ARRAY_COUNT(gSCManagerBattleState->players)))
    {
        s32 i;
        s32 stale_id = gSCManagerBattleState->players[attack_player].stale_id;

        /* BattleShip ftparam.c:1639-1665. The queue is per ATTACKER and scans
         * the whole source-sized history before inserting, so four-way hits
         * must not share a two-fighter/global approximation. */
        for (i = 0;
             i < (s32)ARRAY_COUNT(
                 gSCManagerBattleState->players[attack_player].stale_info);
             i++)
        {
            if ((attack_id == gSCManagerBattleState->players[attack_player]
                                  .stale_info[i].attack_id) &&
                (motion_count == gSCManagerBattleState->players[attack_player]
                                     .stale_info[i].motion_count))
            {
                goto proof_record;
            }
        }
        gSCManagerBattleState->players[attack_player]
            .stale_info[stale_id].attack_id = attack_id;
        gSCManagerBattleState->players[attack_player]
            .stale_info[stale_id].motion_count = motion_count;

        if (stale_id ==
            ((s32)ARRAY_COUNT(
                 gSCManagerBattleState->players[attack_player].stale_info) - 1))
        {
            gSCManagerBattleState->players[attack_player].stale_id = 0;
        }
        else
        {
            gSCManagerBattleState->players[attack_player].stale_id = stale_id + 1;
        }
    }

proof_record:
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowReleaseStaleQueueCount++;
    }
}

void ftParamVelDamageTransferGround(FTStruct *fp)
{
    /* BattleShip ftparam.c:1426-1448.  This is gameplay state, not a proof
     * recorder: DownBounce and the passive/tech statuses call this after
     * becoming grounded so the existing air damage velocity is projected onto
     * the current floor tangent.  The old DS seam kept only the diagnostics
     * below, which left vel_damage_ground at zero and changed subsequent
     * grounded knockback for every live fighter. */
    if (fp->ga == nMPKineticsGround)
    {
        Vec3f *floor_angle = &fp->coll_data.floor_angle;

        if (fp->physics.vel_damage_ground == 0.0F)
        {
            fp->physics.vel_damage_ground = fp->physics.vel_damage_air.x;

            if (fp->physics.vel_damage_ground > 250.0F)
            {
                fp->physics.vel_damage_ground = 250.0F;
            }
            if (fp->physics.vel_damage_ground < -250.0F)
            {
                fp->physics.vel_damage_ground = -250.0F;
            }
            fp->physics.vel_damage_air.x =
                floor_angle->y * fp->physics.vel_damage_ground;
            fp->physics.vel_damage_air.y =
                -floor_angle->x * fp->physics.vel_damage_ground;
        }
    }

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDownBounceVelTransferCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive != FALSE) &&
        (fp != NULL))
    {
        if ((fp->status_id == nFTCommonStatusPassiveStandF) ||
            (fp->status_id == nFTCommonStatusPassiveStandB))
        {
            gNdsStageMPCliffWaitDamageLoopPassiveStandVelTransferCount++;
        }
        else if (fp->status_id == nFTCommonStatusPassive)
        {
            gNdsStageMPCliffWaitDamageLoopPassiveVelTransferCount++;
        }
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandSetStatusActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveStandVelTransferCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveSetStatusActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveVelTransferCount++;
    }
}

static void ndsFTParamGetVisualPosition(GObj *fighter_gobj, s32 joint_id,
                                        Vec3f *effect_pos,
                                        Vec3f *effect_scatter,
                                        sb32 is_scale_pos, Vec3f *pos)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    DObj *root = (fighter_gobj != NULL) ? DObjGetStruct(fighter_gobj) : NULL;

    pos->x = (root != NULL) ? root->translate.vec.f.x : 0.0F;
    pos->y = (root != NULL) ? root->translate.vec.f.y : 0.0F;
    pos->z = (root != NULL) ? root->translate.vec.f.z : 0.0F;
    if ((fp == NULL) || (joint_id < 0) ||
        (joint_id >= (s32)ARRAY_COUNT(fp->joints)) ||
        (fp->joints[joint_id] == NULL))
    {
        if (effect_pos != NULL)
        {
            pos->x += effect_pos->x;
            pos->y += effect_pos->y;
            pos->z += effect_pos->z;
        }
        return;
    }
    if (effect_pos != NULL)
    {
        *pos = *effect_pos;
    }
    else
    {
        pos->x = pos->y = pos->z = 0.0F;
    }
    if (effect_scatter != NULL)
    {
        if (effect_scatter->x != 0.0F)
        {
            pos->x += (syUtilsRandFloat() - 0.5F) *
                      (effect_scatter->x * 2.0F);
        }
        if (effect_scatter->y != 0.0F)
        {
            pos->y += (syUtilsRandFloat() - 0.5F) *
                      (effect_scatter->y * 2.0F);
        }
        if (effect_scatter->z != 0.0F)
        {
            pos->z += (syUtilsRandFloat() - 0.5F) *
                      (effect_scatter->z * 2.0F);
        }
    }
    if ((is_scale_pos != FALSE) && (fp->attr != NULL) &&
        (fp->attr->size != 0.0F))
    {
        f32 inverse_size = 1.0F / fp->attr->size;

        pos->x *= inverse_size;
        pos->y *= inverse_size;
        pos->z *= inverse_size;
    }
    gmCollisionGetFighterPartsWorldPosition(fp->joints[joint_id], pos);
}

#if NDS_R2_SOURCE_EFFECTS_PARTICLE
/* Verbatim from decomp/src/ef/efmanager.h, which this TU does not include.
 * Each is defined by the decomp efmanager.c that src/import/battleship_efmanager.c
 * compiles, unwrapped -- see ndsFTParamMakeSourceEffect below. */
extern LBParticle *efManagerDustLightMakeEffect(Vec3f *pos, s32 lr,
                                                f32 f_index);
extern LBParticle *efManagerDustHeavyMakeEffect(Vec3f *pos, s32 lr);
extern LBParticle *efManagerDustHeavyDoubleMakeEffect(Vec3f *pos, s32 lr,
                                                      f32 f_index);
extern LBParticle *efManagerDustExpandLargeMakeEffect(Vec3f *pos);
extern LBParticle *efManagerDustDashMakeEffect(Vec3f *pos, s32 lr, f32 scale);
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
extern LBParticle *efManagerFuraSparkleMakeEffect(Vec3f *pos);
extern LBParticle *efManagerMusicNoteMakeEffect(Vec3f *pos);
/* These five ALSO have weak stand-in definitions further down this file, which
 * NDS_R2_SOURCE_EFFECTS_PARTICLE overrides from battleship_efmanager.c. The
 * attribute has to appear here, ahead of the calls below -- a plain
 * declaration followed by a weak definition after first use is unspecified
 * behaviour, and GCC says so. */
extern __attribute__((weak)) LBParticle *
efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
extern __attribute__((weak)) LBParticle *
efManagerSparkleWhiteMakeEffect(Vec3f *pos);
extern __attribute__((weak)) LBParticle *
efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale);
extern LBParticle *ndsEFManagerChargeSparkleMakeEffect(Vec3f *pos);
extern __attribute__((weak)) LBParticle *
efManagerFlashMiddleMakeEffect(Vec3f *pos);

/* THE PARTICLE-ONLY HALF OF THE SOURCE DISPATCH (ftparam.c:1892-2112).
 *
 * ftParamMakeEffect is the seam every motion-script effect command arrives at
 * -- ftmain.c:447 for nFTMotionEventEffect, ftmain.c:1139 for the collision
 * variant -- and it is how a fighter asks for running dust, landing dust,
 * flashes, sparkles and flame. The port's own switch below answers all of them
 * with one of seven untextured DS primitives, which is the whole of BUGS.md
 * "Correct VFX isn't played for various things (running foot dust VFX, ...)".
 * Mario and Fox request effects from it 178 times across their two motion
 * files and 106 of those are dust
 * (scripts/check-battle-playable-attack-effects.ps1).
 *
 * The real makers were here the entire time. Every one of them is defined in
 * the decomp efmanager.c that src/import/battleship_efmanager.c includes, and
 * unlike the thirty in that file's rename block they were never wrapped, so
 * they compile with their source bodies and --gc-sections was dropping them
 * only because nothing called them.
 *
 * WHAT IS NOT HERE IS DELIBERATE, for two separate reasons.
 *
 * These fall through because their source maker builds a DObj tree whose
 * geometry goes out as source effect display-list links, which the battle
 * hardware path does not submit -- routing them would trade a visible
 * primitive for nothing at all:
 *   ShockSmall, DamageFlyOrbs/Sparks/MDust, FireSpark, StarRodSpark
 * Same seam that kept the respawn platform invisible. Move a kind up here when
 * that path lands, not before.
 *
 * IMPACTWAVE LEFT THIS LIST ON 2026-08-02, and how that was settled matters
 * more than the kind itself. The exclusion rule was applied to it by reading
 * the SOURCE maker, which does build a DObj tree over effect DL links. But this
 * port OVERRIDES efManagerImpactWaveMakeEffect with a substitute drawing
 * nNDSVisualEffectImpactWave, a DL-link-18 visual-effect template the hardware
 * path does submit -- so the rule's premise was false for this kind, and
 * falling through is what cost the hard landing its shockwave while its dust
 * played. Check for a port override before assuming a kind is blocked here.
 *
 * ROUTING IT THEN "FAILED" THE DETAIL GATE, AND THAT WAS NOISE. The first run
 * threw "Horizontal detail region left_bush variation 22.379% is below required
 * 40.000%", which reads exactly like a 162-unit opaque disc flattening scenery,
 * and it was nearly reverted on that single arm. The owner asked whether their
 * own interaction with the emulator window had caused it. Re-running the SAME
 * change passed. assert-melonds-horizontal-detail samples a screenshot region,
 * and capture runs on an interactive desktop (docs/VERIFYING.md), so a
 * foregrounded window or a fighter standing in that region perturbs it. One
 * failing arm of a screenshot gate is a measurement, not a verdict: re-run the
 * candidate before believing it, exactly as an A/B would.
 *
 * And these fall through because P1 cannot request them, while linking their
 * maker would still cost .text -- which the taskman arena charges one-for-one
 * against the general heap, the scarcest thing this change set spends:
 *   DamageNormal, FlameLR/Random/Static, SparkleWhiteMulti, ChargeSparkle,
 *   HealSparkles, FlashSmall/Large, Psionic, ThunderAmp
 * Mario's and Fox's two motion files request seventeen distinct kinds
 * (scripts/check-battle-playable-attack-effects.ps1) and none of those are
 * among them; they belong to Ness, Samus, Kirby, Pikachu and Mewtwo. Each is
 * three lines from ftparam.c when its fighter lands. Adding one back also
 * needs ftParamGetEffectJointPosition (ftparam.c:1783) for the four that
 * rotate through attr->effect_joint_ids.
 *
 * Position is already resolved by ndsFTParamGetVisualPosition, which is the
 * source's own scatter / inverse-size / gmCollisionGetFighterPartsWorldPosition
 * arithmetic, so these cases are the source's right-hand sides verbatim. */
/* BUGS.md fire burn, second half. These live only inside the imported
 * efmanager.c (2512/2579/2643) and are declared in no port header, so they are
 * declared here at their single point of use. Until now nothing referenced
 * them and --gc-sections dropped them entirely -- the linked ELF carried no
 * Flame symbol at all -- which is why routing to them is what makes them
 * exist, not merely what makes them run. */
extern LBParticle *efManagerFlameLRMakeEffect(Vec3f *pos, s32 lr);
extern LBParticle *efManagerFlameRandomMakeEffect(Vec3f *pos);
extern LBParticle *efManagerFlameStaticMakeEffect(Vec3f *pos);

#if NDS_R2_POSITION_PROBE
/* Far-end proof for the fire-position row. GDB cannot safely correlate a
 * just-written cached stack/global value with a breakpoint in optimized code;
 * c132 demonstrated that by observing gNdsFlameProbeCount==0 at an instruction
 * which is statically after its increment. Record the pointer's VALUE here,
 * inside the dispatch helper and immediately before the real Flame maker. This
 * is the exact argument the maker receives and can be read later after the
 * cache has naturally drained. Probe builds only. */
__attribute__((used)) u32 gNdsFlameMakerProbeCount;
__attribute__((used)) u32 gNdsFlameMakerProbeKind[8];
__attribute__((used)) u32 gNdsFlameMakerProbeSubstep[8];
__attribute__((used)) f32 gNdsFlameMakerProbeX[8];
__attribute__((used)) f32 gNdsFlameMakerProbeY[8];
__attribute__((used)) f32 gNdsFlameMakerProbeZ[8];
__attribute__((used)) uintptr_t gNdsFlameMakerProbeParticle[8];
extern volatile u32 gNdsPositionProbeUpdateInPresent;
static u32 ndsFTParamProbeFlameMakerInput(s32 effect_id, const Vec3f *pos)
{
    u32 slot;

    /* Keep a stable first-eight correspondence between maker inputs and the
     * LBParticle pointers filled after each maker returns. The old modulo ring
     * could overwrite an input before its first draw and manufacture a bogus
     * position delta in the diagnostic itself. */
    if (gNdsFlameMakerProbeCount >= 8u)
    {
        return 8u;
    }
    slot = gNdsFlameMakerProbeCount;

    gNdsFlameMakerProbeKind[slot] = (u32)effect_id;
    gNdsFlameMakerProbeSubstep[slot] = gNdsPositionProbeUpdateInPresent;
    gNdsFlameMakerProbeX[slot] = pos->x;
    gNdsFlameMakerProbeY[slot] = pos->y;
    gNdsFlameMakerProbeZ[slot] = pos->z;
    gNdsFlameMakerProbeParticle[slot] = 0u;
    gNdsFlameMakerProbeCount++;
    return slot;
}
#endif

static sb32 ndsFTParamMakeSourceEffect(s32 effect_id, s32 lr, Vec3f *pos,
                                       void **effect)
{
    switch (effect_id)
    {
    /* The fighter fire burn. The colanim scripts restored above issue FlameLR
     * and FlameRandom; taking the real makers here is what stops them
     * collapsing onto the generic HitFire burst in the substitute switch
     * below. Bank script 0x12 (FlameLR) and 0x55 (FlameRandom/Static) are
    * packed for exactly this. */
    case nEFKindFlameLR:
#if NDS_R2_POSITION_PROBE
        {
            u32 probe_slot = ndsFTParamProbeFlameMakerInput(effect_id, pos);

            *effect = efManagerFlameLRMakeEffect(pos, lr);
            if (probe_slot < 8u)
            {
                gNdsFlameMakerProbeParticle[probe_slot] =
                    (uintptr_t)*effect;
            }
        }
#else
        *effect = efManagerFlameLRMakeEffect(pos, lr);
#endif
        return TRUE;
    case nEFKindFlameRandom:
#if NDS_R2_POSITION_PROBE
        {
            u32 probe_slot = ndsFTParamProbeFlameMakerInput(effect_id, pos);

            *effect = efManagerFlameRandomMakeEffect(pos);
            if (probe_slot < 8u)
            {
                gNdsFlameMakerProbeParticle[probe_slot] =
                    (uintptr_t)*effect;
            }
        }
#else
        *effect = efManagerFlameRandomMakeEffect(pos);
#endif
        return TRUE;
    case nEFKindFlameStatic:
#if NDS_R2_POSITION_PROBE
        {
            u32 probe_slot = ndsFTParamProbeFlameMakerInput(effect_id, pos);

            *effect = efManagerFlameStaticMakeEffect(pos);
            if (probe_slot < 8u)
            {
                gNdsFlameMakerProbeParticle[probe_slot] =
                    (uintptr_t)*effect;
            }
        }
#else
        *effect = efManagerFlameStaticMakeEffect(pos);
#endif
        return TRUE;
    case nEFKindDustLight:
        *effect = efManagerDustLightMakeEffect(pos, lr, 1.0F);
        return TRUE;
    case nEFKindDustLightRapid:
        *effect = efManagerDustLightMakeEffect(pos, lr, 2.0F);
        return TRUE;
    case nEFKindDustHeavyDouble:
        *effect = efManagerDustHeavyDoubleMakeEffect(pos, lr, 1.0F);
        return TRUE;
    case nEFKindDustHeavyDoubleRapid:
        *effect = efManagerDustHeavyDoubleMakeEffect(pos, lr, 1.7F);
        return TRUE;
    /* The hard landing's shockwave -- see the ImpactWave paragraph above for
     * why this is not blocked on the DL-links seam. The source branches on
     * ground vs air and passes the floor angle so the ring lies along the
     * slope (ftparam.c:1966); this seam has no `fp`, so both arms take the
     * substitute at rotate 0. Dream Land's surfaces under a hard landing are
     * flat, so the angle term is zero there; thread it when a sloped stage
     * lands rather than widening this signature now. */
    case nEFKindImpactWave:
        *effect = efManagerImpactWaveMakeEffect(pos, 4, 0.0F);
        return TRUE;
    case nEFKindDustHeavy:
        *effect = efManagerDustHeavyMakeEffect(pos, lr);
        return TRUE;
    case nEFKindDustHeavyReverse:
        *effect = efManagerDustHeavyMakeEffect(pos, -lr);
        return TRUE;
    case nEFKindDustExpandLarge:
        pos->x += ((syUtilsRandFloat() * 160.0F) - 80.0F);
        pos->y += ((syUtilsRandFloat() * 160.0F) - 80.0F);
        *effect = efManagerDustExpandLargeMakeEffect(pos);
        return TRUE;
    case nEFKindDustExpandSmall:
        *effect = efManagerDustExpandSmallMakeEffect(pos, 1.0F);
        return TRUE;
    case nEFKindDustDashSmall:
        *effect = efManagerDustDashMakeEffect(pos, lr, 1.0F);
        return TRUE;
    case nEFKindDustDashLarge:
        *effect = efManagerDustDashMakeEffect(pos, lr, 1.5F);
        return TRUE;
    case nEFKindSparkleWhite:
        *effect = efManagerSparkleWhiteMakeEffect(pos);
        return TRUE;
    case nEFKindSparkleWhiteMultiExplode:
        *effect = efManagerSparkleWhiteMultiExplodeMakeEffect(pos);
        return TRUE;
    case nEFKindSparkleWhiteScale:
        *effect = efManagerSparkleWhiteScaleMakeEffect(pos, 1.0F);
        return TRUE;
    case nEFKindChargeSparkle:
        *effect = ndsEFManagerChargeSparkleMakeEffect(pos);
        return TRUE;
    case nEFKindFuraSparkle:
        *effect = efManagerFuraSparkleMakeEffect(pos);
        return TRUE;
    case nEFKindFlashMiddle:
        *effect = efManagerFlashMiddleMakeEffect(pos);
        return TRUE;
    case nEFKindMusicNote:
        efManagerMusicNoteMakeEffect(pos);
        *effect = NULL;
        return TRUE;
    default:
        return FALSE;
    }
}
#endif /* NDS_R2_SOURCE_EFFECTS_PARTICLE */

/* BUGS.md "Fighter burn flames spawn at the wrong places on the damaged
 * fighter." ftparam.c:1899-1912 has all three Flame kinds DISCARD the generic
 * effect position and call ftParamGetEffectJointPosition (ftparam.c:1783),
 * which advances fp->effect_joint_array_id, wraps at
 * ARRAY_COUNT(attr->effect_joint_ids), and resolves local {0,0,0} at the
 * selected joint. The burn is MEANT to walk the body; flames sharing one origin
 * is the defect, not the fix.
 *
 * This port had no such call, and the measurement (c131, 2026-08-12) found the
 * damage worse than a missing rotation. The position the shipped code actually
 * used read (1039.04, 0.000000, 0.000000) on every emission of a burn -- Y and Z
 * EXACTLY zero -- because ndsFTParamGetVisualPosition takes its early return
 * when the colanim's joint_id does not resolve and hands back the fighter's
 * root translate. Every flame spawned on the stage plane at the victim's feet.
 * The five source joints measured healthy over the same burn: ids 15, 20, 25, 9
 * and 12, Y from 110 to 338 and Z from -104 to +128.
 *
 * Source-exact, and deliberately placed in ftParamMakeEffect rather than in the
 * maker switch: ftParamMakeEffect already holds fighter_gobj, so no signature
 * widens and no global is needed, and the override then also covers the
 * NDS_R2_SOURCE_EFFECTS_PARTICLE=0 substitute arm -- which the source's single
 * path implies and which a fix inside ndsFTParamMakeSourceEffect would miss. */
static void ndsFTParamGetEffectJointPosition(FTStruct *fp, Vec3f *pos)
{
    FTAttributes *attr = fp->attr;
    s32 joint_id;

    fp->effect_joint_array_id++;

    if (fp->effect_joint_array_id == ARRAY_COUNT(attr->effect_joint_ids))
    {
        fp->effect_joint_array_id = 0;
    }
    pos->x = pos->y = pos->z = 0.0F;

    /* The source indexes fp->joints[] unguarded. This port keeps the range
     * check because an attribute table that has not been relocated yet reads as
     * a wild id here, and the failure that produces is a wild pointer inside
     * gmCollisionGetFighterPartsWorldPosition rather than a visible one. On a
     * relocated fighter the check never fires and the behaviour is the
     * source's. */
    joint_id = attr->effect_joint_ids[fp->effect_joint_array_id];
    if ((joint_id < 0) || (joint_id >= (s32)ARRAY_COUNT(fp->joints)) ||
        (fp->joints[joint_id] == NULL))
    {
        return;
    }
    gmCollisionGetFighterPartsWorldPosition(fp->joints[joint_id], pos);
}

#if NDS_R2_POSITION_PROBE
/* Records what the override replaced beside what it produced, so the row's
 * acceptance reads the shipped path rather than a shadow of it. */
/* `used` because GDB is the only consumer and --gc-sections collects a global
 * with no in-ROM reader -- which has already taken Boundary RED once. */
#define NDS_FLAME_PROBE_GLOBAL __attribute__((used))
NDS_FLAME_PROBE_GLOBAL u32 gNdsFlameProbeCount;
NDS_FLAME_PROBE_GLOBAL u32 gNdsFlameProbeKind[8];
NDS_FLAME_PROBE_GLOBAL u32 gNdsFlameProbeSelIndex[8];
NDS_FLAME_PROBE_GLOBAL s32 gNdsFlameProbeJointID[8];
NDS_FLAME_PROBE_GLOBAL f32 gNdsFlameProbeJointX[8];
NDS_FLAME_PROBE_GLOBAL f32 gNdsFlameProbeJointY[8];
NDS_FLAME_PROBE_GLOBAL f32 gNdsFlameProbeJointZ[8];
NDS_FLAME_PROBE_GLOBAL f32 gNdsFlameProbeGenericX[8];
NDS_FLAME_PROBE_GLOBAL f32 gNdsFlameProbeGenericY[8];
NDS_FLAME_PROBE_GLOBAL f32 gNdsFlameProbeGenericZ[8];
static void ndsFTParamProbeFlamePosition(FTStruct *fp, s32 effect_id,
                                         const Vec3f *generic,
                                         const Vec3f *at)
{
    u32 slot = gNdsFlameProbeCount & 7u;

    gNdsFlameProbeKind[slot] = (u32)effect_id;
    gNdsFlameProbeSelIndex[slot] = (u32)fp->effect_joint_array_id;
    gNdsFlameProbeJointID[slot] =
        fp->attr->effect_joint_ids[fp->effect_joint_array_id];
    gNdsFlameProbeJointX[slot] = at->x;
    gNdsFlameProbeJointY[slot] = at->y;
    gNdsFlameProbeJointZ[slot] = at->z;
    gNdsFlameProbeGenericX[slot] = generic->x;
    gNdsFlameProbeGenericY[slot] = generic->y;
    gNdsFlameProbeGenericZ[slot] = generic->z;
    gNdsFlameProbeCount++;
}
#endif /* NDS_R2_POSITION_PROBE */

void *ftParamMakeEffect(GObj *fighter_gobj, s32 effect_id, s32 joint_id,
                        Vec3f *effect_pos, Vec3f *effect_scatter, s32 lr,
                        sb32 is_scale_pos, u32 arg7)
{
    Vec3f pos;
    Vec3f charge_effect_pos;
    GObj *effect_gobj = NULL;

    (void)arg7;
    /* BUGS.md "Missing fire burn effects": decide whether the Flame family is
     * reachable in P1 before adding it to P1_PARTICLE_SEAMS. The alias below
     * collapses FlameLR/FlameRandom/FlameStatic AND FireSpark onto one
     * substitute, and only FireSpark is currently a listed seam, so a single
     * combined counter could not tell "the burn is requested and substituted"
     * from "only FireSpark ever arrives". Count them apart. */
    /* Enumerate WHICH effect kinds the fire path actually requests, instead of
     * testing one hypothesis per build. 128 bits as four readable scalars. */
    if (((u32)effect_id) < 128u)
    {
        u32 bit = 1u << (((u32)effect_id) & 31u);
        switch (((u32)effect_id) >> 5)
        {
        case 0u: gNdsFighterEffectKindMask0 |= bit; break;
        case 1u: gNdsFighterEffectKindMask1 |= bit; break;
        case 2u: gNdsFighterEffectKindMask2 |= bit; break;
        default: gNdsFighterEffectKindMask3 |= bit; break;
        }
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        ((sNdsFighterDashRunDamageStatusSetupActive != FALSE) ||
         (sNdsFighterDashRunDamageExpiryActive != FALSE)) &&
        (effect_id == nEFKindDustExpandLarge))
    {
        sNdsFighterDashRunDamageSetupDustEffectCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDownBounceEffectCount++;
        gNdsStageMPCliffWaitDamageLoopDownBounceEffectKind = (u32)effect_id;
    }
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW
    if ((gNdsFighterSpecialsProofPhase == 3u) &&
        (effect_id == nEFKindDustLight))
    {
        gNdsFighterSpecialsMarioLwDustEffectCount++;
    }
#endif
    /* BattleShip ftparam.c:1812-1854 gives the common full-charge sparkle a
     * fighter-specific hand joint and local X offset before the generic effect
     * position resolver runs. The colanim script intentionally names joint 0;
     * this override is the source contract that moves the sparkle to the hand. */
    if ((effect_id == nEFKindChargeSparkle) && (fighter_gobj != NULL))
    {
        FTStruct *charge_fp = ftGetStruct(fighter_gobj);

        if (charge_fp != NULL)
        {
            charge_effect_pos.y = 0.0F;
            charge_effect_pos.z = 0.0F;
            switch (charge_fp->fkind)
            {
            case nFTKindSamus:
                joint_id = 16;
                charge_effect_pos.x = 180.0F;
                effect_pos = &charge_effect_pos;
                break;
            case nFTKindDonkey:
            case nFTKindGDonkey:
                joint_id = 16;
                charge_effect_pos.x = 100.0F;
                effect_pos = &charge_effect_pos;
                break;
            case nFTKindKirby:
                joint_id = 16;
                charge_effect_pos.x = 50.0F;
                effect_pos = &charge_effect_pos;
                break;
            default:
                break;
            }
        }
    }
    ndsFTParamGetVisualPosition(fighter_gobj, joint_id, effect_pos,
                                effect_scatter, is_scale_pos, &pos);
    /* The source's Flame override, ahead of BOTH dispatch arms. See
     * ndsFTParamGetEffectJointPosition for why it lives here and what the
     * generic position it replaces was measured to be. */
    if ((effect_id == nEFKindFlameLR) || (effect_id == nEFKindFlameRandom) ||
        (effect_id == nEFKindFlameStatic))
    {
        FTStruct *flame_fp =
            (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

        if ((flame_fp != NULL) && (flame_fp->attr != NULL))
        {
#if NDS_R2_POSITION_PROBE
            Vec3f generic = pos;
#endif
            ndsFTParamGetEffectJointPosition(flame_fp, &pos);
#if NDS_R2_POSITION_PROBE
            ndsFTParamProbeFlamePosition(flame_fp, effect_id, &generic, &pos);
#endif
        }
    }
#if NDS_R2_SOURCE_EFFECTS_PARTICLE
    {
        void *source_effect = NULL;

        if (ndsFTParamMakeSourceEffect(effect_id, lr, &pos,
                                       &source_effect) != FALSE)
        {
            return source_effect;
        }
    }
#endif
    switch (effect_id)
    {
    case nEFKindDamageNormal:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectHitNormal, &pos, 0.8F, lr, NULL);
        break;
    /* The three Flame kinds now take their real makers above and never reach
     * here at NDS_R2_SOURCE_EFFECTS_PARTICLE=1. They are NOT dead: the flag is
     * switchable to 0 for attribution A/Bs, and that arm has no other route to
     * a fire burn. Deleting them would silently drop the burn from the arm
     * least likely to be looked at. */
    case nEFKindFlameLR:
    case nEFKindFlameRandom:
    case nEFKindFlameStatic:
    case nEFKindFireSpark:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectHitFire, &pos, 0.7F, lr, NULL);
        break;
    case nEFKindShockSmall:
    case nEFKindPsionic:
    case nEFKindThunderAmp:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectHitElectric, &pos, 0.7F, lr, NULL);
        break;
    case nEFKindDustLight:
    case nEFKindDustLightRapid:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 0.65F, lr, NULL);
        break;
    case nEFKindDustHeavyDouble:
    case nEFKindDustHeavyDoubleRapid:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 1.0F, lr, NULL);
        pos.x += (f32)lr * 110.0F;
        (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectDust, &pos,
                                           0.8F, -lr, NULL);
        break;
    case nEFKindDustHeavy:
    case nEFKindDustHeavyReverse:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 1.0F,
            (effect_id == nEFKindDustHeavyReverse) ? -lr : lr, NULL);
        break;
    case nEFKindDustExpandLarge:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 1.35F, lr, NULL);
        break;
    case nEFKindDustExpandSmall:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 0.7F, lr, NULL);
        break;
    case nEFKindDustDashSmall:
    case nEFKindDustDashLarge:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos,
            (effect_id == nEFKindDustDashLarge) ? 1.25F : 0.9F,
            lr, NULL);
        break;
    case nEFKindDamageFlyOrbs:
    case nEFKindStarRodSpark:
    case nEFKindDamageFlySparks:
    case nEFKindDamageFlySparksReverse:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectSparkle, &pos, 0.65F, lr, NULL);
        break;
    case nEFKindDamageFlyMDust:
    case nEFKindDamageFlyMDustReverse:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 0.55F, lr, NULL);
        break;
    case nEFKindImpactWave:
    case nEFKindRipple:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectImpactWave, &pos, 0.9F, lr, NULL);
        break;
    case nEFKindSparkleWhite:
    case nEFKindSparkleWhiteMultiExplode:
    case nEFKindSparkleWhiteMulti:
    case nEFKindSparkleWhiteScale:
    case nEFKindFuraSparkle:
    case nEFKindChargeSparkle:
    case nEFKindHealSparkles:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectSparkle, &pos, 0.75F, lr, NULL);
        break;
    case nEFKindFlashSmall:
    case nEFKindFlashMiddle:
    case nEFKindFlashLarge:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectImpactWave, &pos,
            (effect_id == nEFKindFlashLarge) ? 1.3F : 0.8F, lr, NULL);
        break;
    case nEFKindMusicNote:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectCoin, &pos, 0.65F, lr, NULL);
        break;
    case nEFKindEggBreak:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 0.8F, lr, NULL);
        break;
    case nEFKindQuakeMag0:
        return efManagerQuakeMakeEffect(0);
    case nEFKindQuakeMag1:
        return efManagerQuakeMakeEffect(1);
    case nEFKindQuakeMag2:
        return efManagerQuakeMakeEffect(2);
    default:
        break;
    }
    return effect_gobj;
}

static f32 ndsVisualDamageScale(s32 size, f32 base, f32 step)
{
    if (size < 0)
    {
        size = 0;
    }
    else if (size > 40)
    {
        size = 40;
    }
    return base + ((f32)size * step);
}

__attribute__((weak)) LBParticle *efManagerFlashMiddleMakeEffect(Vec3f *pos)
{
    if ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffCatchFloorLoopFlashCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopCliffCatchFlashCount++;
    }
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectImpactWave, pos,
                                       0.8F, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *
efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale)
{
    if ((ndsFighterMarioFoxStageInishieScaleLoopProofEnabled() != FALSE) &&
        (sNdsStageInishieScaleLoopActive != FALSE))
    {
        gNdsStageInishieScaleLoopFallSparkleCount++;
        gNdsFighterMarioFoxStageInishieScaleLoopDeferredMask |= 1u << 0;
    }
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectSparkle, pos,
                                       scale, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *
efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index)
{
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectDust, pos,
                                       0.65F * f_index, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *efManagerFireGrindMakeEffect(Vec3f *pos)
{
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectHitFire, pos,
                                       0.55F, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos)
{
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectSparkle, pos,
                                       0.75F, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *
efManagerDamageNormalLightMakeEffect(Vec3f *pos, s32 player, s32 size,
                                     sb32 is_static)
{
    NDS_FREEZE_DIAGNOSTICS_MARK(NDS_FREEZE_BREADCRUMB_EFFECT_SPAWN);
    ndsTask39EffectCensusRecord(
        NDS_TASK39_EFFECT_EF_MANAGER_DAMAGE_NORMAL_LIGHT_MAKE_EFFECT,
        NDS_TASK39_EFFECT_SUBSTITUTE);
#if NDS_TASK39_FX_SPRITES
    ndsTask39HitSparkSpawn(pos, player, size, is_static, FALSE);
#else
    (void)player;
    (void)is_static;
    (void)ndsEFManagerMakeVisualEffect(
        nNDSVisualEffectHitNormal, pos,
        ndsVisualDamageScale(size, 0.45F, 0.025F), 1, NULL);
#endif
    return NULL;
}

__attribute__((weak)) LBParticle *
efManagerDamageNormalHeavyMakeEffect(Vec3f *pos, s32 player, s32 size)
{
    NDS_FREEZE_DIAGNOSTICS_MARK(NDS_FREEZE_BREADCRUMB_EFFECT_SPAWN);
    ndsTask39EffectCensusRecord(
        NDS_TASK39_EFFECT_EF_MANAGER_DAMAGE_NORMAL_HEAVY_MAKE_EFFECT,
        NDS_TASK39_EFFECT_SUBSTITUTE);
#if NDS_TASK39_FX_SPRITES
    ndsTask39HitSparkSpawn(pos, player, size, FALSE, TRUE);
#else
    (void)player;
    (void)ndsEFManagerMakeVisualEffect(
        nNDSVisualEffectHitNormal, pos,
        ndsVisualDamageScale(size, 0.70F, 0.035F), 1, NULL);
#endif
    return NULL;
}

/* NOT the owner's "orange ball", and the correction is worth keeping because it
 * was asserted from memory and refuted by the source in one lookup. This seam
 * was briefly halved on the claim that Mario's forward smash is a flame attack.
 * It is not: nothing under src/ft/ftchar/ftmario sets an element field at all,
 * and Mario's ONLY fire-element source is Special N, whose fireball is a WEAPON
 * whose element comes from attr->element (wpmanager.c:197) and which reaches
 * this function through the weapon switch at ftmain.c:2770, not the fighter one
 * at ftmain.c:2715. A side-A attack is normal-element and never arrives here.
 *
 * The scale left alone at the source-derived value. The owner filed an orange
 * ball on side-A hits; normal-element hits go to ndsTask39HitSparkSpawn under
 * NDS_TASK39_FX_SPRITES, which is where that row has to be answered. */
__attribute__((weak)) LBParticle *efManagerDamageFireMakeEffect(Vec3f *pos,
                                                               s32 size)
{
    (void)ndsEFManagerMakeVisualEffect(
        nNDSVisualEffectHitFire, pos,
        ndsVisualDamageScale(size, 0.60F, 0.03F), 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *
efManagerDamageElectricMakeEffect(Vec3f *pos, s32 size)
{
    (void)ndsEFManagerMakeVisualEffect(
        nNDSVisualEffectHitElectric, pos,
        ndsVisualDamageScale(size, 0.55F, 0.03F), 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *efManagerDamageCoinMakeEffect(Vec3f *pos)
{
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectCoin, pos,
                                       0.75F, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *efManagerSetOffMakeEffect(Vec3f *pos,
                                                           s32 size)
{
    (void)ndsEFManagerMakeVisualEffect(
        nNDSVisualEffectImpactWave, pos,
        ndsVisualDamageScale(size, 0.55F, 0.03F), 1, NULL);
    return NULL;
}

/* The real one comes with battleship_ftcommon_hammer.c, which imports
 * fthammer.c whole. */
#if !NDS_P2_ITEM_CORE
void ftHammerUpdateStats(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}
#endif

#if !NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE
sb32 ftCommonDeadCheckInterruptCommon(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return FALSE;
}
#endif

void ftParamKirbyTryMakeMapStarEffect(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

void ftParamSetAnimLocks(FTStruct *fp)
{
    u32 flags0;
    u32 flags1;
    u32 current_flags;
    FTParts *parts;
    u32 *animlock;
    s32 i;

    if ((fp == NULL) || (fp->attr == NULL) || (fp->attr->animlock == NULL))
    {
        return;
    }
    animlock = fp->attr->animlock;
    flags0 = animlock[0];
    flags1 = animlock[1];

    for (i = nFTPartsJointCommonStart; ((flags0 != 0) || (flags1 != 0)); i++)
    {
        current_flags =
            (i < (ARRAY_COUNT(fp->joints) - 1)) ? flags0 : flags1;
        if ((current_flags & (1u << 31)) && (fp->joints[i] != NULL))
        {
            parts = fp->joints[i]->user_data.p;
            if (parts != NULL)
            {
                gmCollisionTransformMatrixAll(fp->joints[i], parts,
                                              parts->unk_dobjtrans_0x10);
                parts->transform_update_mode = 3;
                if (fp->joints[i]->xobjs[0] != NULL)
                {
                    fp->joints[i]->xobjs[0]->unk05 = 1;
                }
            }
        }
        if (i < (ARRAY_COUNT(fp->joints) - 1))
        {
            flags0 <<= 1;
        }
        else
        {
            flags1 <<= 1;
        }
    }
}

void ftParamClearAnimLocks(FTStruct *fp)
{
    FTParts *parts;
    s32 i;

    if (fp == NULL)
    {
        return;
    }
    for (i = 0; i < ARRAY_COUNT(fp->joints); i++)
    {
        if (fp->joints[i] != NULL)
        {
            parts = fp->joints[i]->user_data.p;
            if ((parts != NULL) && (parts->transform_update_mode == 3))
            {
                parts->transform_update_mode = 0;
                if (fp->joints[i]->xobjs[0] != NULL)
                {
                    fp->joints[i]->xobjs[0]->unk05 = 0;
                }
            }
        }
    }
}

void ftParamMoveDLLink(GObj *fighter_gobj, u8 dl_link)
{
    FTStruct *fp;

    if (fighter_gobj == NULL)
    {
        return;
    }
    gcMoveGObjDL(fighter_gobj, dl_link, GOBJ_PRIORITY_DEFAULT);
    fp = ftGetStruct(fighter_gobj);
    if (fp != NULL)
    {
        fp->dl_link = dl_link;
    }
}

extern void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                               f32 anim_frame);
extern void *ndsRelocResolvePointerFromFileBase(const void *file_base,
                                                const void *ptr,
                                                size_t size);
extern void *ndsRelocResolveAuthoritativeForceFile(void *file);

static DObj *ndsLBCommonGetTreeDObjNextFromRoot(DObj *dobj, DObj *root_dobj)
{
    if (dobj == NULL)
    {
        return NULL;
    }
    if (dobj->child != NULL)
    {
        return dobj->child;
    }
    /* decomp lb/lbcommon.c:757 -- a CHILDLESS ROOT terminates the walk. Without
     * this the walk leaves root_dobj's subtree through the root's own sibling
     * (and then through its parent's), which is how a fighter's TransN, parked
     * as XRotN's sibling by ftMainSetStatus' TransN reparent
     * (ft/ftmain.c:4710-4721), could be handed another subtree's script. Every
     * root used today has children, so this restores the guard rather than
     * changing an observed behaviour. */
    if (dobj == root_dobj)
    {
        return NULL;
    }
    if (dobj->sib_next != NULL)
    {
        return dobj->sib_next;
    }
    while (dobj != NULL)
    {
        if (dobj->parent == root_dobj)
        {
            return NULL;
        }
        if ((dobj->parent != NULL) && (dobj->parent->sib_next != NULL))
        {
            return dobj->parent->sib_next;
        }
        dobj = dobj->parent;
    }
    return NULL;
}

void lbCommonAddFighterPartsFigatree(DObj *root_dobj, void *figatree,
                                     f32 anim_frame)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
#if NDS_R2_BATTLEPACK
    figatree = ndsRelocResolveAuthoritativeForceFile(figatree);
#endif
    void **figatree_entries = figatree;
    DObj *current_dobj = root_dobj;
    /* P2-2p6: the fighter pose engine takes the attach when it can hold the
     * walk. Counted first, because the engine's state is carved from the arena
     * once per fighter at BattleShip's fixed fp->joints[] topology bound. That
     * includes hidden parts ftMainSetStatus can materialise later; an attach it
     * cannot hold must fall back to the generic bind as a whole -- never half a
     * fighter. */
    u32 pose_entries = 0u;
    sb32 pose_engine;
#if NDS_R2_FTANIM_TRACK
    /* Stage 3: status/motion selection binds the precompiled rows ONCE, here.
     * This is the only site that attaches a fighter figatree, and the tree walk
     * below indexes the figatree's own entry table -- which is exactly how the
     * AOT pack is keyed, so the bind is one array index per joint. */
    const s32 trk_base = ndsFtAnimTrackBeginClip(root_dobj, figatree);
    s32 trk_index = 0;
#endif

    if ((root_dobj != NULL) && (root_dobj->parent_gobj != NULL))
    {
        root_dobj->parent_gobj->anim_frame = anim_frame;
    }
    if (NDS_FT_POSE != 0)
    {
        DObj *walk = root_dobj;

        while (walk != NULL)
        {
            pose_entries++;
            walk = ndsLBCommonGetTreeDObjNextFromRoot(walk, root_dobj);
        }
    }
    /* BIND TO THE ENGINE THAT WILL ACTUALLY PLAY THIS MOTION.
     *
     * `ftMainPlayAnim` routes a motion to the pose engine only when
     * `fp->anim_desc.flags.is_anim_joint` is clear -- an event32 animjoint
     * status keeps the generic player. This attach did not ask the same
     * question, so an `is_anim_joint` motion was bound INTO the pose engine and
     * then played by the generic one: the pose engine holds the only live
     * script, the generic walk finds `AOBJ_ANIM_NULL` on every joint, and
     * nothing advances or publishes `parent_gobj->anim_frame`.
     *
     * That is what broke the entry. Mario's Appear (motion 197,
     * `nFTMarioMotionAppearR`) is an `is_anim_joint` motion -- measured,
     * `aj=1` with `joints[0]->child->anim_wait = AOBJ_ANIM_NULL` and a GObj
     * clock frozen for the whole status -- so `ftCommonAppearProcUpdate`'s
     * `anim_frame <= 0.0F` was true on the tick the status began and both
     * fighters left their entry immediately while their own effect played on.
     * The owner saw that as "the pipe only renders its rim".
     *
     * The two sites now ask the identical question, which is the property that
     * matters: whichever engine binds a motion is the engine that plays it. */
    {
        FTStruct *bind_fp = (root_dobj != NULL) ?
            ftGetStruct(root_dobj->parent_gobj) : NULL;
        const sb32 bind_anim_joint =
            ((bind_fp != NULL) && (bind_fp->anim_desc.flags.is_anim_joint)) ?
                TRUE : FALSE;

        pose_engine = ((figatree_entries != NULL) &&
                       (bind_anim_joint == FALSE)) ?
            ndsFtPoseBindBegin(root_dobj, pose_entries) : FALSE;
    }
    pose_entries = 0u;
    while ((current_dobj != NULL) && (figatree_entries != NULL))
    {
        AObjEvent32 *anim_joint = *figatree_entries;
        FTParts *parts = ftGetParts(current_dobj);

        anim_joint = ndsRelocResolvePointerFromFileBase(figatree,
                                                        anim_joint,
                                                        sizeof(*anim_joint));
        if (anim_joint != NULL)
        {
            if (pose_engine != FALSE)
            {
                ndsFtPoseBindEntry(pose_entries, current_dobj,
                                   (AObjEvent16 *)anim_joint, anim_frame);
#if NDS_FT_POSE_ORACLE
                /* The oracle keeps the generic bind on the live joint; the
                 * engine bound its shadow copy above. */
                gcAddDObjAnimJoint(current_dobj, anim_joint, anim_frame);
#endif
            }
            else
            {
                gcAddDObjAnimJoint(current_dobj, anim_joint, anim_frame);
            }
#if NDS_R2_FTANIM_TRACK
            /* AFTER the generic bind, never instead of it: `gcAddDObjAnimJoint`
             * resets every AObj to `nGCAnimKindNone`, sets `anim_wait` to
             * AOBJ_ANIM_CHANGED and stores the anim frame, and all three are
             * load-bearing. This only replaces the cursor. */
            (void)ndsFtAnimTrackBindJoint(current_dobj, trk_base, trk_index);
#endif
            gNdsFighterNaturalMotionFigatreeAttachCount++;
            if (parts != NULL)
            {
                parts->is_have_anim = TRUE;
            }
        }
        else
        {
            if (anim_joint == NULL)
            {
                gNdsFighterNaturalMotionFigatreeNullCount++;
            }
            else
            {
                gNdsFighterNaturalMotionFigatreeAnimInvalidCount++;
                gNdsFighterNaturalMotionUnsafeCount++;
            }
            if (pose_engine != FALSE)
            {
                ndsFtPoseBindEntry(pose_entries, current_dobj, NULL,
                                   anim_frame);
            }
            current_dobj->anim_wait = AOBJ_ANIM_NULL;
            if (parts != NULL)
            {
                parts->is_have_anim = FALSE;
            }
        }
        figatree_entries++;
        pose_entries++;
#if NDS_R2_FTANIM_TRACK
        trk_index++;
#endif
        current_dobj =
            ndsLBCommonGetTreeDObjNextFromRoot(current_dobj, root_dobj);
    }
    if (pose_engine != FALSE)
    {
        ndsFtPoseBindEnd(pose_entries);
    }
#else
    (void)figatree;
    if ((root_dobj != NULL) && (root_dobj->parent_gobj != NULL))
    {
        root_dobj->parent_gobj->anim_frame = anim_frame;
    }
#endif
}

/* decomp lb/lbcommon.c:785, which this port has never had a body for -- it was
 * an empty stub (`bx lr` at 0x02052eac in build-c132-stress) until 2026-08-13.
 *
 * ftCommonGuardInitJoints (ft/ftcommon/ftcommonguard1.c:275-282) sets
 * fp->anim_desc.flags.is_anim_joint = TRUE and calls this in the same breath,
 * because this is what makes the flag TRUE: it replaces every joint in
 * root_dobj's subtree with an event32 shield anim joint, or stops the joint
 * where the table entry is NULL. With no body the flag was set while every
 * joint still held the GuardOn FIGATREE that ftMainSetStatus installed
 * (ft/ftmain.c:4704), so ftParamUpdateAnimKeys' per-fighter dispatch
 * (ft/ftparam.c:386) ran the 32-bit parser over 16-bit commands -- the misread
 * attributed in artifacts/performance/2026-08-13_c-anim-anomalies/ANOMALIES.md,
 * whose `default:` drops the joint's animation entirely. The flag's clear is
 * source-exact and was never missing: ftMainSetStatus re-derives the WHOLE word
 * from the motion descriptor at ft/ftmain.c:4633
 * (`fp->anim_desc.word = motion_desc->anim_desc.word`), which a grep for the
 * field name cannot see because FTAnimDesc is a union.
 *
 * The entries are resolved exactly as lbCommonAddFighterPartsFigatree resolves
 * the figatree's: absolute pointers into a loaded file pass through, file-
 * relative ones are rebased, and anything that resolves to nothing is treated
 * as the NULL entry rather than handed to the parser. */
void lbCommonAddDObjAnimJointAll(DObj *dobj, AObjEvent32 **anim_joint,
                                 f32 anim_frame)
{
    const void *file_base = anim_joint;
    DObj *current_dobj = dobj;

    gNdsShieldAnimJointInstallCalls++;
    if ((dobj == NULL) || (anim_joint == NULL))
    {
        return;
    }
    /* P2-2p6: an event32 attach re-targets these joints; the pose engine
     * steps aside until the next figatree attach. */
    ndsFtPoseUnbind(dobj->parent_gobj);
    if (dobj->parent_gobj != NULL)
    {
        dobj->parent_gobj->anim_frame = anim_frame;
    }
    while (current_dobj != NULL)
    {
        AObjEvent32 *script =
            ndsRelocResolvePointerFromFileBase(file_base, *anim_joint,
                                               sizeof(AObjEvent32));

        if (script != NULL)
        {
            gcAddDObjAnimJoint(current_dobj, script, anim_frame);
            gNdsShieldAnimJointAttachCount++;
        }
        else
        {
            current_dobj->anim_wait = AOBJ_ANIM_NULL;
            gNdsShieldAnimJointNullCount++;
        }
        anim_joint++;
        current_dobj =
            ndsLBCommonGetTreeDObjNextFromRoot(current_dobj, dobj);
    }
}

void lbCommonInitDObj(DObj *dobj, u8 tk1, u8 tk2, u8 tk3, u8 arg4)
{
    if (dobj == NULL)
    {
        return;
    }
    if (tk1 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk1, arg4);
    }
    if (tk2 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk2, arg4);
    }
    if (tk3 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk3, arg4);
    }
    dobj->translate.vec.f.x = 0.0F;
    dobj->translate.vec.f.y = 0.0F;
    dobj->translate.vec.f.z = 0.0F;
    dobj->rotate.vec.f.x = 0.0F;
    dobj->rotate.vec.f.y = 0.0F;
    dobj->rotate.vec.f.z = 0.0F;
    dobj->scale.vec.f.x = 1.0F;
    dobj->scale.vec.f.y = 1.0F;
    dobj->scale.vec.f.z = 1.0F;
}

void lbCommonInitDObj3Transforms(DObj *dobj, u8 tk1, u8 tk2, u8 tk3)
{
    if (dobj == NULL)
    {
        return;
    }
    if (tk1 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk1, 0);
    }
    if (tk2 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk2, 0);
    }
    if (tk3 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk3, 0);
    }
    dobj->translate.vec.f.x = 0.0F;
    dobj->translate.vec.f.y = 0.0F;
    dobj->translate.vec.f.z = 0.0F;
    dobj->rotate.vec.f.x = 0.0F;
    dobj->rotate.vec.f.y = 0.0F;
    dobj->rotate.vec.f.z = 0.0F;
    dobj->scale.vec.f.x = 1.0F;
    dobj->scale.vec.f.y = 1.0F;
    dobj->scale.vec.f.z = 1.0F;
}

extern void gcDecideDObj3TransformsKind(DObj *dobj, u8 tk1, u8 tk2,
                                         u8 tk3, s32 flags);

/* P2-3f50 parts-setup guard telemetry, read by
 * scripts/probe-battle-progress.ps1 -ExtraGlobals. */
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadIdCount;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadId;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadStep;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadDescId;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadDesc;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadContainer;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadDetail;

void lbCommonSetupFighterPartsDObjs(DObj *root_dobj,
                                    FTCommonPartContainer *commonparts_container,
                                    s32 detail_curr, DObj **dobjs,
                                    u32 *setup_parts, u8 tk1, u8 tk2,
                                    u8 tk3, f32 anim_frame, u8 arg9)
{
    DObj *array_dobjs[DOBJ_ARRAY_MAX];
    DObjDesc *dobjdesc;
    u32 flags0;
    u32 flags1;
    s32 i;

    if ((root_dobj == NULL) || (commonparts_container == NULL) ||
        (setup_parts == NULL))
    {
        return;
    }
    dobjdesc = commonparts_container
                   ->commonparts[detail_curr - nFTPartsDetailStart]
                   .dobjdesc;
    if (dobjdesc == NULL)
    {
        return;
    }
    for (i = 0; i < DOBJ_ARRAY_MAX; i++)
    {
        array_dobjs[i] = NULL;
    }
    flags0 = setup_parts[0];
    flags1 = setup_parts[1];
    for (i = 0; ((flags0 != 0u) || (flags1 != 0u)) &&
                (dobjdesc->id != DOBJ_ARRAY_MAX);
         i++, dobjdesc++)
    {
        const u32 current_flags = (i < 32) ? flags0 : flags1;
        if (current_flags & (1u << 31))
        {
            const s32 id = dobjdesc->id & 0xFFF;
            s32 detail_id = nFTPartsDetailHigh - nFTPartsDetailStart;
            DObj *parent;
            DObj *current_dobj;

            /* GUARD, not a fix (P2-3f50). The source indexes
             * array_dobjs[id] with whatever the descriptor holds, so a
             * descriptor the loader left unrelocated writes outside this
             * frame and the NEXT fighter's setup faults in its own
             * prologue -- which is how Purin's abort presented, three
             * frames from its cause. Publish the first bad index and stop
             * this tree instead of smashing the stack. Valid data never
             * reaches this branch. */
            if ((u32)id >= (u32)DOBJ_ARRAY_MAX)
            {
                if (gNdsFTPartsSetupBadIdCount == 0u)
                {
                    gNdsFTPartsSetupBadId = (u32)id;
                    gNdsFTPartsSetupBadStep = (u32)i;
                    gNdsFTPartsSetupBadDescId = (u32)dobjdesc->id;
                    gNdsFTPartsSetupBadDesc = (u32)(uintptr_t)dobjdesc;
                    gNdsFTPartsSetupBadContainer =
                        (u32)(uintptr_t)commonparts_container;
                    gNdsFTPartsSetupBadDetail = (u32)detail_curr;
                }
                gNdsFTPartsSetupBadIdCount++;
                break;
            }
            parent = (id != 0) ? array_dobjs[id - 1] : root_dobj;

            if ((detail_curr != nFTPartsDetailHigh) &&
                (commonparts_container
                     ->commonparts[nFTPartsDetailLow - nFTPartsDetailStart]
                     .dobjdesc != NULL) &&
                (commonparts_container
                     ->commonparts[nFTPartsDetailLow - nFTPartsDetailStart]
                     .dobjdesc[i]
                     .dl != NULL))
            {
                detail_id = nFTPartsDetailLow - nFTPartsDetailStart;
            }
            if (parent == NULL)
            {
                goto advance_flags;
            }
            current_dobj = gcAddChildForDObj(
                parent,
                commonparts_container->commonparts[detail_id].dobjdesc[i].dl);
            if (current_dobj == NULL)
            {
                goto advance_flags;
            }
            array_dobjs[id] = current_dobj;
            if ((dobjdesc->id & 0x8000) != 0)
            {
                gcDecideDObj3TransformsKind(current_dobj, tk1, tk2, tk3,
                                             0x8000);
            }
            else
            {
                lbCommonInitDObj(current_dobj, tk1, tk2, tk3, arg9);
            }
            current_dobj->translate.vec.f = dobjdesc->translate;
            current_dobj->rotate.vec.f = dobjdesc->rotate;
            current_dobj->scale.vec.f = dobjdesc->scale;
            lbCommonAddMObjForFighterPartsDObj(
                current_dobj,
                (commonparts_container->commonparts[detail_id].p_mobjsubs !=
                 NULL)
                    ? commonparts_container->commonparts[detail_id]
                          .p_mobjsubs[i]
                    : NULL,
                (commonparts_container->commonparts[detail_id]
                     .p_costume_matanim_joints != NULL)
                    ? commonparts_container->commonparts[detail_id]
                          .p_costume_matanim_joints[i]
                    : NULL,
                NULL, anim_frame);
            if (dobjs != NULL)
            {
                *dobjs = current_dobj;
            }
        }
advance_flags:
        if (dobjs != NULL)
        {
            dobjs++;
        }
        if (i < 32)
        {
            flags0 <<= 1;
        }
        else
        {
            flags1 <<= 1;
        }
    }
}

void lbCommonAddMObjForFighterPartsDObj(DObj *dobj, MObjSub **mobjsubs,
                                        AObjEvent32 **costume_matanim_joints,
                                        AObjEvent32 **main_matanim_joints,
                                        f32 anim_frame)
{
    if ((dobj == NULL) || (mobjsubs == NULL))
    {
        return;
    }

    while (*mobjsubs != NULL)
    {
        MObjSub normalized_mobjsub = **mobjsubs;
        MObj *mobj;

        ndsRelocNormalizeMObjSubWordSwapped(&normalized_mobjsub);
        mobj = gcAddMObjForDObj(dobj, &normalized_mobjsub);

        if (mobj == NULL)
        {
            return;
        }
        if (costume_matanim_joints != NULL)
        {
            AObjEvent32 *costume_matanim_joint = *costume_matanim_joints;

            if (costume_matanim_joint != NULL)
            {
                gcAddMObjMatAnimJoint(mobj, costume_matanim_joint,
                                      anim_frame);
                gcParseMObjMatAnimJoint(mobj);
                gcPlayMObjMatAnim(mobj);
                gcRemoveAObjFromMObj(mobj);
            }
            costume_matanim_joints++;
        }
        if (main_matanim_joints != NULL)
        {
            AObjEvent32 *main_matanim_joint = *main_matanim_joints;

            if (main_matanim_joint != NULL)
            {
                gcAddMObjMatAnimJoint(mobj, main_matanim_joint, 0.0F);
                gcParseMObjMatAnimJoint(mobj);
                gcPlayMObjMatAnim(mobj);
            }
            main_matanim_joints++;
        }
        mobjsubs++;
    }
}

void mpCommonUpdateFighterSlopeContour(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

void ftCommonReboundProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonReboundProcUpdate(fighter_gobj);
    return;
#endif

    ndsBaseFTCommonReboundProcUpdate(fighter_gobj);
}

void ftCommonReboundSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonReboundSetStatus(fighter_gobj);
    return;
#endif

    ndsBaseFTCommonReboundSetStatus(fighter_gobj);
}

void ftCommonReboundWaitProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonReboundWaitProcUpdate(fighter_gobj);
    return;
#endif

    ndsBaseFTCommonReboundWaitProcUpdate(fighter_gobj);
}

void ftCommonReboundWaitSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonReboundWaitSetStatus(fighter_gobj);
    return;
#endif

    ndsBaseFTCommonReboundWaitSetStatus(fighter_gobj);
}

void ftCommonGuardSetOffProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonGuardSetOffProcUpdate(fighter_gobj);
    return;
#endif

    ndsBaseFTCommonGuardSetOffProcUpdate(fighter_gobj);
}

void ftCommonGuardSetOffSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonGuardSetOffSetStatus(fighter_gobj);
    return;
#endif

    ndsBaseFTCommonGuardSetOffSetStatus(fighter_gobj);
}

alSoundEffect *lbCommonMakePositionFGM(u16 fgm, f32 pos)
{
    s32 balance = (s32)((pos / 8000.0F) * 60.0F);

    if (balance > 60)
    {
        balance = 60;
    }
    if (balance < -60)
    {
        balance = -60;
    }
    return ndsPlayFGMAtPan(fgm, (u8)(64 - balance));
}

sb32 ftCommonCliffAttackCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonCliffAttackCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffLiveLoopInterruptActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 0;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffEscapeActionLoopAttackCheckCount++;
        return ndsBaseFTCommonCliffAttackCheckInterruptCommon(fighter_gobj);
    }
    if ((ndsFighterMarioFoxStageMPCliffAttackFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackFloorLoopInterruptActive != FALSE))
    {
        FTStruct *fp;
        sb32 result;

        gNdsStageMPCliffAttackFloorLoopAttackCheckCount++;
        result = ndsBaseFTCommonCliffAttackCheckInterruptCommon(fighter_gobj);
        if (result != FALSE)
        {
            fp = ftGetStruct(fighter_gobj);
            if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
            {
                gNdsStageMPCliffAttackFloorLoopUnsafeCount++;
            }
            else
            {
                fp->status_vars.common.cliffmotion.status_id =
                    (fp->percent_damage < FTCOMMON_CLIFF_DAMAGE_HIGH) ?
                    nFTCommonCliffKindAttackQuick :
                    nFTCommonCliffKindAttackSlow;
                fp->status_vars.common.cliffmotion.cliff_id =
                    fp->coll_data.cliff_id;
            }
        }
        return result;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffClimbFloorLoopAttackCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopAttackCheckCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitFloorLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitFloorLoopAttackCheckCount++;
    }
    (void)fighter_gobj;
    return FALSE;
}

sb32 ftCommonCliffEscapeCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonCliffEscapeCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffLiveLoopInterruptActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 1;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopInterruptActive != FALSE))
    {
        FTStruct *fp;
        sb32 result;

        gNdsStageMPCliffEscapeActionLoopEscapeCheckCount++;
        result = ndsBaseFTCommonCliffEscapeCheckInterruptCommon(fighter_gobj);
        if (result != FALSE)
        {
            fp = ftGetStruct(fighter_gobj);
            if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
            {
                gNdsStageMPCliffEscapeActionLoopUnsafeCount++;
            }
            else
            {
                fp->status_vars.common.cliffmotion.status_id =
                    (fp->percent_damage < FTCOMMON_CLIFF_DAMAGE_HIGH) ?
                    nFTCommonCliffKindEscapeQuick :
                    nFTCommonCliffKindEscapeSlow;
                fp->status_vars.common.cliffmotion.cliff_id =
                    fp->coll_data.cliff_id;
            }
        }
        return result;
    }
    if ((ndsFighterMarioFoxStageMPCliffAttackFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackFloorLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffAttackFloorLoopEscapeCheckCount++;
        return ndsBaseFTCommonCliffEscapeCheckInterruptCommon(fighter_gobj);
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffClimbFloorLoopEscapeCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopEscapeCheckCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitFloorLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitFloorLoopEscapeCheckCount++;
    }
    (void)fighter_gobj;
    return FALSE;
}

sb32 ftCommonCliffClimbOrFallCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonCliffClimbOrFallCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffLiveLoopInterruptActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        f32 angle;

        gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 2;
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffLiveLoopUnsafeCount++;
            return FALSE;
        }
        if ((ABS(fp->input.pl.stick_range.x) <
                FTCOMMON_CLIFF_MOTION_STICK_RANGE_MIN) &&
            (ABS(fp->input.pl.stick_range.y) <
                FTCOMMON_CLIFF_MOTION_STICK_RANGE_MIN))
        {
            fp->status_vars.common.cliffwait.is_allow_interrupt = TRUE;
            return FALSE;
        }
        if (fp->status_vars.common.cliffwait.is_allow_interrupt == FALSE)
        {
            return FALSE;
        }

        angle = ftParamGetStickAngleRads(fp);
        if ((angle > F_CST_DTOR32(50.0F)) ||
            ((angle > F_CST_DTOR32(-50.0F)) &&
             ((fp->input.pl.stick_range.x * fp->lr) >= 0)))
        {
            ftCommonCliffQuickOrSlowSetStatus(fighter_gobj, 0);
            fp->status_vars.common.cliffmotion.status_id =
                (fp->percent_damage < FTCOMMON_CLIFF_DAMAGE_HIGH) ?
                nFTCommonCliffKindClimbQuick : nFTCommonCliffKindClimbSlow;
            fp->status_vars.common.cliffmotion.cliff_id =
                fp->coll_data.cliff_id;
            fp->is_cliff_hold = TRUE;
            fp->proc_damage = ftCommonCliffCommonProcDamage;
            return TRUE;
        }

        fp->cliffcatch_wait = FTCOMMON_CLIFF_CATCH_WAIT;
        ftCommonCliffCommonProcDamage(fighter_gobj);
        ftCommonFallSetStatus(fighter_gobj);
        return TRUE;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffEscapeActionLoopClimbOrFallCheckCount++;
        return ndsBaseFTCommonCliffClimbOrFallCheckInterruptCommon(
            fighter_gobj);
    }
    if ((ndsFighterMarioFoxStageMPCliffAttackFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackFloorLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffAttackFloorLoopClimbOrFallCheckCount++;
        return ndsBaseFTCommonCliffClimbOrFallCheckInterruptCommon(
            fighter_gobj);
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopInterruptActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        f32 angle;

        gNdsStageMPCliffClimbFloorLoopClimbOrFallCheckCount++;
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
            return FALSE;
        }
        if ((ABS(fp->input.pl.stick_range.x) <
                FTCOMMON_CLIFF_MOTION_STICK_RANGE_MIN) &&
            (ABS(fp->input.pl.stick_range.y) <
                FTCOMMON_CLIFF_MOTION_STICK_RANGE_MIN))
        {
            fp->status_vars.common.cliffwait.is_allow_interrupt = TRUE;
            return FALSE;
        }
        if (fp->status_vars.common.cliffwait.is_allow_interrupt == FALSE)
        {
            return FALSE;
        }

        angle = ftParamGetStickAngleRads(fp);
        if ((angle > F_CST_DTOR32(50.0F)) ||
            ((angle > F_CST_DTOR32(-50.0F)) &&
             ((fp->input.pl.stick_range.x * fp->lr) >= 0)))
        {
            ftCommonCliffQuickOrSlowSetStatus(fighter_gobj, 0);
            fp->status_vars.common.cliffmotion.status_id =
                (fp->percent_damage < FTCOMMON_CLIFF_DAMAGE_HIGH) ?
                nFTCommonCliffKindClimbQuick : nFTCommonCliffKindClimbSlow;
            fp->status_vars.common.cliffmotion.cliff_id =
                fp->coll_data.cliff_id;
            return TRUE;
        }

        fp->cliffcatch_wait = FTCOMMON_CLIFF_CATCH_WAIT;
        ftCommonCliffCommonProcDamage(fighter_gobj);
        ftCommonFallSetStatus(fighter_gobj);
        return TRUE;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopClimbOrFallCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitFloorLoopInterruptActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        gNdsStageMPCliffWaitFloorLoopClimbOrFallCheckCount++;
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitFloorLoopUnsafeCount++;
            return FALSE;
        }
        if ((ABS(fp->input.pl.stick_range.x) <
                FTCOMMON_CLIFF_MOTION_STICK_RANGE_MIN) &&
            (ABS(fp->input.pl.stick_range.y) <
                FTCOMMON_CLIFF_MOTION_STICK_RANGE_MIN))
        {
            fp->status_vars.common.cliffwait.is_allow_interrupt = TRUE;
        }
    }
    (void)fighter_gobj;
    return FALSE;
}

void ftCommonDamageUpdateDustEffect(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageUpdateDustEffect(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }

    ndsBaseFTCommonDamageUpdateDustEffect(fighter_gobj);
}

void ftCommonDamageDecHitStunSetPublic(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageDecHitStunSetPublic(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }

    ndsBaseFTCommonDamageDecHitStunSetPublic(fighter_gobj);
}

void ftCommonDamageUpdateCatchResist(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageUpdateCatchResist(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }

    ndsBaseFTCommonDamageUpdateCatchResist(fighter_gobj);
}

void ftCommonDamageCommonProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageCommonProcUpdate(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == NULL) || (fighter_gobj == NULL))
    {
        return;
    }

    ndsBaseFTCommonDamageCommonProcUpdate(fighter_gobj);
}

void ftCommonDamageAirCommonProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageAirCommonProcUpdate(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == NULL) || (fighter_gobj == NULL))
    {
        return;
    }

    ndsBaseFTCommonDamageAirCommonProcUpdate(fighter_gobj);
}

void ftCommonDamageCheckSetInvincible(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageCheckSetInvincible(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == NULL) || (fighter_gobj == NULL))
    {
        return;
    }
    ndsBaseFTCommonDamageCheckSetInvincible(fighter_gobj);
}

void ftCommonDamageSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageSetStatus(fighter_gobj);
    return;
#endif

    FTStruct *fp;

    if (fighter_gobj == NULL)
    {
        return;
    }

    fp = ftGetStruct(fighter_gobj);
    if (fp == NULL)
    {
        return;
    }

    ndsBaseFTCommonDamageSetStatus(fighter_gobj);
}

void ftCommonDamageCommonProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageCommonProcInterrupt(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == NULL) || (fighter_gobj == NULL))
    {
        return;
    }

    ndsBaseFTCommonDamageCommonProcInterrupt(fighter_gobj);
}

void ftCommonDamageAirCommonProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageAirCommonProcInterrupt(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == NULL) || (fighter_gobj == NULL))
    {
        return;
    }

    ndsBaseFTCommonDamageAirCommonProcInterrupt(fighter_gobj);
}

void ftCommonDamageFlyRollUpdateModelPitch(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageFlyRollUpdateModelPitch(fighter_gobj);
    return;
#endif

    FTStruct *fp;

    if (fighter_gobj == NULL)
    {
        return;
    }

    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (fp->joints[4] == NULL))
    {
        return;
    }

    ndsBaseFTCommonDamageFlyRollUpdateModelPitch(fighter_gobj);
}

void ftCommonDamageCommonProcPhysics(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageCommonProcPhysics(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }

    if ((fp->status_id == nFTCommonStatusDamageFlyRoll) &&
        (fp->joints[4] == NULL))
    {
        return;
    }

    ndsBaseFTCommonDamageCommonProcPhysics(fighter_gobj);
}

void ftCommonDamageCommonProcLagUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageCommonProcLagUpdate(fighter_gobj);
    return;
#endif

    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == NULL) || (DObjGetStruct(fighter_gobj) == NULL))
    {
        return;
    }

    ndsBaseFTCommonDamageCommonProcLagUpdate(fighter_gobj);
}

static void ndsFTMainProcUpdateHitlagLifecycleSlice(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    if (fp->hitlag_tics != 0)
    {
        fp->hitlag_tics--;
        if (fp->hitlag_tics == 0)
        {
            fp->is_knockback_paused = FALSE;
            if (fp->proc_lagend != NULL)
            {
                fp->proc_lagend(fighter_gobj);
            }
        }
    }
    if (fp->hitlag_tics == 0)
    {
        ftMainPlayAnimEventsAll(fighter_gobj);
    }
}

static void ndsFTMainProcPhysicsLagUpdateSlice(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp != NULL) && (fp->proc_lagupdate != NULL))
    {
        fp->proc_lagupdate(fighter_gobj);
    }
}


static sb32 ndsMPCommonProcFighterDamageFloorOnly(MPCollData *coll_data,
                                                  GObj *fighter_gobj,
                                                  u32 flags)
{
    FTStruct *fp;
    sb32 is_collide = FALSE;
    f32 root_y_before;

    (void)flags;
    gNdsCollisionRuntimeDiagnostics.damage_proc_calls++;
    if ((coll_data == NULL) || (fighter_gobj == NULL) ||
        (coll_data->p_translate == NULL))
    {
        gNdsCollisionRuntimeDiagnostics.damage_invalid++;
        return FALSE;
    }
    fp = ftGetStruct(fighter_gobj);
    if (fp == NULL)
    {
        gNdsCollisionRuntimeDiagnostics.damage_invalid++;
        return FALSE;
    }
    root_y_before = coll_data->p_translate->y;

    /* This is the exact floor portion of BattleShip mpCommonProcFighterDamage.
     * Its wall and ceiling branches remain deferred until the O2R side runners
     * publish source-faithful line IDs, angles, and clamps. */
    coll_data->mask_unk &= (u16)~(MAP_FLAG_LWALL | MAP_FLAG_RWALL);
    coll_data->mask_stat &= (u16)~(MAP_FLAG_LWALL | MAP_FLAG_RWALL |
                                   MAP_FLAG_CEIL);
    gNdsCollisionRuntimeDiagnostics.damage_floor_tests++;
    if (mpProcessRunFloorCollisionAdjNewNULL(coll_data) != FALSE)
    {
        if ((mpCollisionCheckExistLineID(coll_data->floor_line_id) == FALSE) ||
            (mpCollisionGetLineTypeID(coll_data->floor_line_id) !=
                nMPLineKindFloor))
        {
            gNdsCollisionRuntimeDiagnostics.damage_invalid++;
            coll_data->mask_curr &= (u16)~MAP_FLAG_FLOOR;
            mpProcessSetCollProjectFloorID(coll_data);
            return FALSE;
        }
        gNdsCollisionRuntimeDiagnostics.damage_floor_hits++;
        if (fp->hitlag_tics > 0)
        {
            mpProcessSetCollideFloor(coll_data);
            if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
            {
                mpProcessRunFloorEdgeAdjust(coll_data);
            }
            else
            {
                mpProcessSetCollProjectFloorID(coll_data);
            }
        }
        else if (syVectorAngleDiff3D(&coll_data->pos_diff,
                     &coll_data->floor_angle) > F_CLC_DTOR32(110.0F))
        {
            mpProcessSetLandingFloor(coll_data);
            mpCommonSetFighterLandingParams(fighter_gobj);
            if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
            {
                mpProcessRunFloorEdgeAdjust(coll_data);
                fp->status_vars.common.damage.coll_mask_curr |=
                    MAP_FLAG_FLOOR;
                is_collide = TRUE;
                coll_data->is_coll_end = TRUE;
                gNdsCollisionRuntimeDiagnostics.damage_floor_landings++;
            }
        }
        else
        {
            mpProcessSetCollideFloor(coll_data);
            if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
            {
                mpProcessRunFloorEdgeAdjust(coll_data);
                if ((coll_data->mask_prev & MAP_FLAG_FLOOR) == 0u)
                {
                    fp->status_vars.common.damage.coll_mask_ignore |=
                        MAP_FLAG_FLOOR;
                    fp->status_vars.common.damage.wall_collide_angle =
                        coll_data->floor_angle;
                }
            }
            else
            {
                mpProcessSetCollProjectFloorID(coll_data);
            }
        }
    }
    else
    {
        mpProcessSetCollProjectFloorID(coll_data);
    }
    if (is_collide != FALSE)
    {
        gNdsCollisionRuntimeDiagnostics.damage_last_line =
            coll_data->floor_line_id;
        gNdsCollisionRuntimeDiagnostics.damage_last_status = fp->status_id;
        gNdsCollisionRuntimeDiagnostics.damage_last_root_y_before_milli =
            ndsFloatToMilliSigned(root_y_before);
        gNdsCollisionRuntimeDiagnostics.damage_last_root_y_after_milli =
            ndsFloatToMilliSigned(coll_data->p_translate->y);
        gNdsCollisionRuntimeDiagnostics.damage_last_pos_diff_y_milli =
            ndsFloatToMilliSigned(coll_data->pos_diff.y);
        gNdsCollisionRuntimeDiagnostics.damage_last_angle_y_milli =
            ndsFloatToMilliSigned(coll_data->floor_angle.y);
        gNdsCollisionRuntimeDiagnostics.damage_last_mask_curr =
            coll_data->mask_curr;
        gNdsCollisionRuntimeDiagnostics.damage_last_mask_stat =
            coll_data->mask_stat;
    }
    return is_collide;
}

sb32 mpCommonCheckFighterDamageCollision(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageMapActive != FALSE))
    {
        if (fp == NULL)
        {
            return FALSE;
        }
        fp->status_vars.common.damage.coll_mask_prev =
            fp->status_vars.common.damage.coll_mask_curr;
        fp->status_vars.common.damage.coll_mask_curr = 0u;
        fp->status_vars.common.damage.coll_mask_ignore = 0u;

        if (sNdsFighterDashRunDamageFallMapCollisionMode == 4u)
        {
            fp->status_vars.common.damage.coll_mask_curr = MAP_FLAG_CEIL;
            fp->coll_data.mask_stat = MAP_FLAG_CEIL;
            fp->coll_data.mask_curr = MAP_FLAG_CEIL;
            sNdsFighterDashRunDamageFallMapCollisionCount++;
            return TRUE;
        }
        if (sNdsFighterDashRunDamageFallMapCollisionMode == 3u)
        {
            fp->status_vars.common.damage.coll_mask_curr = MAP_FLAG_LWALL;
            fp->coll_data.mask_stat = MAP_FLAG_LWALL;
            fp->coll_data.mask_curr = MAP_FLAG_LWALL;
            sNdsFighterDashRunDamageFallMapCollisionCount++;
            return TRUE;
        }
        if (sNdsFighterDashRunDamageFallMapCollisionMode == 5u)
        {
            fp->status_vars.common.damage.coll_mask_curr = MAP_FLAG_RWALL;
            fp->coll_data.mask_stat = MAP_FLAG_RWALL;
            fp->coll_data.mask_curr = MAP_FLAG_RWALL;
            sNdsFighterDashRunDamageFallMapCollisionCount++;
            return TRUE;
        }
        if (sNdsFighterDashRunDamageFallMapCollisionMode == 8u)
        {
            fp->coll_data.mask_stat = 0u;
            fp->coll_data.mask_curr = 0u;
            sNdsFighterDashRunDamageFallMapCollisionCount++;
            return TRUE;
        }
        if (sNdsFighterDashRunDamageFallMapCollisionMode != 0u)
        {
            fp->status_vars.common.damage.coll_mask_curr = MAP_FLAG_FLOOR;
            fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
            fp->coll_data.mask_curr = MAP_FLAG_FLOOR;
            sNdsFighterDashRunDamageFallMapCollisionCount++;
            return TRUE;
        }
        sNdsFighterDashRunDamageFallMapNoCollisionCount++;
        return FALSE;
    }
    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        sb32 result;

        gNdsCollisionRuntimeDiagnostics.damage_check_calls++;
        if ((fp == NULL) || (fp->coll_data.p_translate == NULL))
        {
            gNdsCollisionRuntimeDiagnostics.damage_invalid++;
            return FALSE;
        }
        fp->status_vars.common.damage.coll_mask_prev =
            fp->status_vars.common.damage.coll_mask_curr;
        fp->status_vars.common.damage.coll_mask_curr = 0u;
        fp->status_vars.common.damage.coll_mask_ignore = 0u;
        result = mpProcessUpdateMain(&fp->coll_data,
                                     ndsMPCommonProcFighterDamageFloorOnly,
                                     fighter_gobj, MAP_PROC_TYPE_DEFAULT);
        if (result != FALSE)
        {
            gNdsCollisionRuntimeDiagnostics.damage_results++;
        }
        return result;
    }
    return FALSE;
}

sb32 ftCommonWallDamageCheckGoto(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonWallDamageCheckGoto(fighter_gobj);
#endif

    if ((fighter_gobj == NULL) || (ftGetStruct(fighter_gobj) == NULL))
    {
        return FALSE;
    }
    return ndsBaseFTCommonWallDamageCheckGoto(fighter_gobj);
}

void ftCommonDamageAirCommonProcMap(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageAirCommonProcMap(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageMapActive != FALSE))
    {
        sNdsFighterDashRunDamageAirMapCount++;
    }
    if ((fighter_gobj == NULL) || (ftGetStruct(fighter_gobj) == NULL))
    {
        return;
    }
    ndsBaseFTCommonDamageAirCommonProcMap(fighter_gobj);
}

f32 ftParamGetHitStun(f32 knockback)
{
    return knockback / 1.875F;
}

void ftParamSetTimedHitStatusInvincible(FTStruct *fp, s32 invincible_tics)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->invincible_tics < invincible_tics)
    {
        fp->invincible_tics = invincible_tics;
    }
    fp->special_hitstatus = (fp->intangible_tics != 0) ?
        nGMHitStatusIntangible : nGMHitStatusInvincible;
    ftParamCheckSetFighterColAnimID(fp->fighter_gobj,
                                    nGMColAnimFighterNoDamage, 0);
}

void ftParamSetTimedHitStatusIntangible(FTStruct *fp, s32 intangible_tics)
{
    if (fp == NULL)
    {
        return;
    }
    /* BattleShip ftparam.c:1732-1740 extends an existing intangible window but
     * never shortens it. The old DS shim overwrote it unconditionally. */
    if (fp->intangible_tics < intangible_tics)
    {
        fp->intangible_tics = intangible_tics;
    }
    fp->special_hitstatus = nGMHitStatusIntangible;
    ftParamCheckSetFighterColAnimID(fp->fighter_gobj,
                                    nGMColAnimFighterNoDamage, 0);
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopWallDamageActive != FALSE))
    {
        gNdsStageMPPassiveLoopWallDamageIntangibleSetCount++;
    }
}

void ftCommonDamageFallProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageFallProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageFallSourceInterruptActive != FALSE))
    {
        sNdsFighterDashRunDamageFallSourceInterruptCount++;
        ndsBaseFTCommonDamageFallProcInterrupt(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageInterruptActive != FALSE))
    {
        // ponytail: count the handoff; full DamageFall interrupt has its own proof.
        sNdsFighterDashRunDamageFallInterruptCount++;
        (void)fighter_gobj;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDamageFallInterruptTickCount++;
        ndsBaseFTCommonDamageFallProcInterrupt(fighter_gobj);
        return;
    }
    (void)fighter_gobj;
}

void ftCommonDamageFallProcMap(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
#if NDS_P2_SAMUS_TUMBLE_TOUR
    ndsSamusTumbleTourPrepareDamageFallMap(ftGetStruct(fighter_gobj));
#endif
    ndsBaseFTCommonDamageFallProcMap(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageMapActive != FALSE))
    {
        // ponytail: prove the safe no-collision map tick; collision branches are later.
        sNdsFighterDashRunDamageFallMapCount++;
        ndsBaseFTCommonDamageFallProcMap(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        gNdsStageMPCliffWaitDamageLoopDamageFallMapTickCount++;
        if (sNdsStageMPCliffWaitDamageLoopMapCollisionMode == 2u)
        {
            if (mpCommonCheckFighterCliff(fighter_gobj) != FALSE)
            {
                if ((fp != NULL) &&
                    ((fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK) != 0u))
                {
                    ftCommonCliffCatchSetStatus(fighter_gobj);
                }
                else if ((ftCommonPassiveStandCheckInterruptDamage(
                              fighter_gobj) == FALSE) &&
                         (ftCommonPassiveCheckInterruptDamage(
                              fighter_gobj) == FALSE))
                {
                    ftCommonDownBounceSetStatus(fighter_gobj);
                }
            }
            return;
        }
        ndsBaseFTCommonDamageFallProcMap(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopDamageFallMapActive != FALSE))
    {
        gNdsStageMPPassiveLoopDamageFallMapCallCount++;
        ndsBaseFTCommonDamageFallProcMap(fighter_gobj);
        return;
    }
    (void)fighter_gobj;
}

void ftCommonDamageFallClampRumble(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageFallClampRumble(fighter_gobj);
    return;
#endif

    ndsBaseFTCommonDamageFallClampRumble(fighter_gobj);
}

void ftCommonDamageFallSetStatusFromDamage(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageFallSetStatusFromDamage(fighter_gobj);
    return;
#endif

    FTStruct *fp;
    sb32 saved_dash_fall_set_status_from_damage_active;
    sb32 saved_passive_wall_fall_set_status_from_damage_active;

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageExpiryActive != FALSE) &&
        (sNdsStageMPPassiveLoopWallDamageActive == FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            return;
        }

        sNdsFighterDashRunDamageFallFTMainSetStatusCount = 0u;
        sNdsFighterDashRunDamageFallClampRumbleCount = 0u;
        saved_dash_fall_set_status_from_damage_active =
            sNdsFighterDashRunDamageFallSetStatusFromDamageActive;
        sNdsFighterDashRunDamageFallSetStatusFromDamageActive = TRUE;
        ndsBaseFTCommonDamageFallSetStatusFromDamage(fighter_gobj);
        sNdsFighterDashRunDamageFallSetStatusFromDamageActive =
            saved_dash_fall_set_status_from_damage_active;

        fp = ftGetStruct(fighter_gobj);
        if ((fp != NULL) &&
            (fp->status_id == nFTCommonStatusDamageFall) &&
            (fp->motion_id == nFTCommonMotionDamageFall) &&
            (fp->ga == nMPKineticsAir) &&
            (fp->proc_interrupt == ftCommonDamageFallProcInterrupt) &&
            (fp->proc_physics == ftPhysicsApplyAirVelDriftFastFall) &&
            (fp->proc_map == ftCommonDamageFallProcMap) &&
            (sNdsFighterDashRunDamageFallFTMainSetStatusCount == 1u) &&
            (sNdsFighterDashRunDamageFallClampRumbleCount == 1u))
        {
            sNdsFighterDashRunDamageFallSetStatusCount++;
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopWallDamageActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
            return;
        }

        sNdsStageMPPassiveLoopWallDamageFallFTMainSetStatusCount = 0u;
        sNdsStageMPPassiveLoopWallDamageFallClampRumbleCount = 0u;
        saved_passive_wall_fall_set_status_from_damage_active =
            sNdsStageMPPassiveLoopWallDamageFallSetStatusFromDamageActive;
        sNdsStageMPPassiveLoopWallDamageFallSetStatusFromDamageActive = TRUE;
        ndsBaseFTCommonDamageFallSetStatusFromDamage(fighter_gobj);
        sNdsStageMPPassiveLoopWallDamageFallSetStatusFromDamageActive =
            saved_passive_wall_fall_set_status_from_damage_active;

        fp = ftGetStruct(fighter_gobj);
        if ((fp != NULL) &&
            (fp->status_id == nFTCommonStatusDamageFall) &&
            (fp->motion_id == nFTCommonMotionDamageFall) &&
            (fp->ga == nMPKineticsAir)
#if !NDS_IMPORT_BATTLESHIP_FTMAIN
            &&
            (fp->proc_interrupt == ftCommonDamageFallProcInterrupt) &&
            (fp->proc_physics == ftPhysicsApplyAirVelDriftFastFall) &&
            (fp->proc_map == ftCommonDamageFallProcMap) &&
            (sNdsStageMPPassiveLoopWallDamageFallFTMainSetStatusCount == 1u) &&
            (sNdsStageMPPassiveLoopWallDamageFallClampRumbleCount == 1u)
#endif
            )
        {
            gNdsStageMPPassiveLoopWallDamageDamageFallCallCount++;
        }
        return;
    }
    (void)fighter_gobj;
}

/* Owned by battleship_ftcommon_hammer.c wherever the item core is on -- that
 * TU imports the whole of ftcommonhammerfall.c. */
#if !NDS_P2_ITEM_CORE
sb32 ftCommonHammerFallCheckInterruptDamageFall(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageFallSourceInterruptActive != FALSE))
    {
        sNdsFighterDashRunDamageFallHammerCheckCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDamageFallHammerCheckCount++;
    }
    return FALSE;
}
#endif

static sb32 ndsStageMPPassiveLoopRunNaturalFloorCollision(GObj *fighter_gobj,
                                                          FTStruct *fp)
{
    DObj *root;
    MPCollData coll;
    s32 line_id;
    f32 left_x;
    f32 right_x;
    f32 floor_y;
    sb32 hit;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        return FALSE;
    }
    line_id = gNdsStageFloorEdgeLoopSelectedLineID;
    if (line_id < 0)
    {
        return FALSE;
    }

    left_x = (f32)gNdsStageFloorEdgeLoopLeftXMilli / 1000.0F;
    right_x = (f32)gNdsStageFloorEdgeLoopRightXMilli / 1000.0F;
    root->translate.vec.f.x = (left_x + right_x) * 0.5F;
    if (ndsStageFloorEdgeLoopFloorYAtX(line_id, root->translate.vec.f.x,
            &floor_y) == FALSE)
    {
        return FALSE;
    }
    root->translate.vec.f.y = floor_y;
    root->translate.vec.f.z = 0.0F;

    fp->coll_data.pos_prev = root->translate.vec.f;
    fp->coll_data.floor_line_id = line_id;
    fp->coll_data.mask_curr = 0u;
    fp->coll_data.mask_stat = 0u;
    fp->coll_data.update_tic = gMPCollisionUpdateTic;
    fp->coll_data.ignore_line_id = -1;

    if (ndsStageMPProcessFloorLoopBuildCollData(fp, &coll) == FALSE)
    {
        return FALSE;
    }
    coll.pos_prev = root->translate.vec.f;
    coll.pos_diff.x = 0.0F;
    coll.pos_diff.y = 0.0F;
    coll.pos_diff.z = 0.0F;
    coll.vel_speed.x = 0.0F;
    coll.vel_speed.y = 0.0F;
    coll.vel_speed.z = 0.0F;
    coll.vel_push.x = 0.0F;
    coll.vel_push.y = 0.0F;
    coll.vel_push.z = 0.0F;
    coll.floor_line_id = line_id;
    coll.mask_curr = 0u;
    coll.mask_stat = 0u;
    coll.is_coll_end = FALSE;

    hit = mpProcessUpdateMain(&coll, mpCommonRunFighterAllCollisions,
                              fighter_gobj, MAP_PROC_TYPE_DEFAULT);
    if ((hit != FALSE) && (coll.floor_line_id == line_id) &&
        ((coll.mask_stat & MAP_FLAG_FLOOR) != 0u))
    {
        ndsStageMPProcessFloorLoopCopyBack(fp, &coll);
        sNdsStageMPPassiveLoopNaturalMapFloorHit = TRUE;
        return TRUE;
    }
    return FALSE;
}

static sb32 ndsMPCommonRunFighterCliffFloorCeilCollisions(
    MPCollData *coll_data, GObj *fighter_gobj, u32 flags);

sb32 mpCommonCheckFighterCliff(GObj *fighter_gobj)
{
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageMapActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if (sNdsFighterDashRunDamageFallMapCollisionMode != 0u)
        {
            if (fp != NULL)
            {
                if (sNdsFighterDashRunDamageFallMapCollisionMode == 8u)
                {
                    fp->coll_data.mask_stat = 0u;
                    fp->coll_data.mask_curr = 0u;
                }
                else if (sNdsFighterDashRunDamageFallMapCollisionMode == 2u)
                {
                    fp->coll_data.mask_stat = MAP_FLAG_RCLIFF;
                    fp->coll_data.mask_curr = MAP_FLAG_RCLIFF;
                }
                else
                {
                    fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
                    fp->coll_data.mask_curr = MAP_FLAG_FLOOR;
                }
            }
            sNdsFighterDashRunDamageFallMapCollisionCount++;
            return TRUE;
        }
        sNdsFighterDashRunDamageFallMapNoCollisionCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        gNdsStageMPCliffWaitDamageLoopDamageFallCliffCheckCount++;
        if (sNdsStageMPCliffWaitDamageLoopMapCollisionMode != 0u)
        {
            if (fp != NULL)
            {
                if (sNdsStageMPCliffWaitDamageLoopMapCollisionMode == 2u)
                {
                    fp->coll_data.mask_stat = MAP_FLAG_RCLIFF;
                    fp->coll_data.mask_curr = MAP_FLAG_RCLIFF;
                }
                else
                {
                    fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
                    fp->coll_data.mask_curr = MAP_FLAG_FLOOR;
                }
            }
            gNdsStageMPCliffWaitDamageLoopDamageFallCollisionHitCount++;
            return TRUE;
        }
        gNdsStageMPCliffWaitDamageLoopDamageFallNoCollisionCount++;
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopDamageFallMapActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if (ndsStageMPPassiveLoopRunNaturalFloorCollision(fighter_gobj, fp)
            != FALSE)
        {
            return TRUE;
        }
        if (fp != NULL)
        {
            fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
            fp->coll_data.mask_curr = MAP_FLAG_FLOOR;
        }
        return TRUE;
    }
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    {
        FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

        if (fp != NULL)
        {
            return mpProcessUpdateMain(
                &fp->coll_data,
                ndsMPCommonRunFighterCliffFloorCeilCollisions,
                fighter_gobj, MAP_PROC_TYPE_CLIFF);
        }
    }
#else
    (void)fighter_gobj;
#endif
    return FALSE;
}

sb32 ftCommonPassiveStandCheckInterruptDamage(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonPassiveStandCheckInterruptDamage(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageMapActive != FALSE))
    {
        sNdsFighterDashRunDamageFallPassiveStandCheckCount++;
        if ((sNdsFighterDashRunDamageFallMapCollisionMode == 6u) ||
            (sNdsFighterDashRunDamageFallMapCollisionMode == 7u))
        {
            return ndsBaseFTCommonPassiveStandCheckInterruptDamage(
                fighter_gobj);
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        s32 status_id;

        gNdsStageMPCliffWaitDamageLoopDamageFallPassiveStandCheckCount++;
        if ((fp == NULL) ||
            (fp->tics_since_last_z >= FTCOMMON_PASSIVE_BUFFER_TICS_MAX) ||
            (ABS(fp->input.pl.stick_range.x) <
                FTCOMMON_PASSIVE_F_OR_B_RANGE))
        {
            return FALSE;
        }
        if ((fp->input.pl.stick_range.x * fp->lr) >= 0)
        {
            status_id = nFTCommonStatusPassiveStandF;
        }
        else
        {
            status_id = nFTCommonStatusPassiveStandB;
        }
        if (ndsBaseFTCommonPassiveStandCheckInterruptDamage(fighter_gobj) !=
            FALSE)
        {
            return TRUE;
        }
        fp = ftGetStruct(fighter_gobj);
        if (fp == NULL)
        {
            return FALSE;
        }
        if (fp->ga == nMPKineticsAir)
        {
            mpCommonSetFighterGround(fp);
        }
        ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F,
                        FTSTATUS_PRESERVE_NONE);
        ftParamVelDamageTransferGround(fp);
        return TRUE;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPPassiveLoopPassiveStandSetStatusActive != FALSE) ||
         (sNdsStageMPPassiveLoopBranchProbeActive != FALSE) ||
         (sNdsStageMPPassiveLoopPassiveStandBActive != FALSE)))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        s32 status_id;

        if ((fp == NULL) ||
            (fp->tics_since_last_z >= FTCOMMON_PASSIVE_BUFFER_TICS_MAX) ||
            (ABS(fp->input.pl.stick_range.x) <
                FTCOMMON_PASSIVE_F_OR_B_RANGE))
        {
            return FALSE;
        }
        if ((fp->input.pl.stick_range.x * fp->lr) >= 0)
        {
            status_id = nFTCommonStatusPassiveStandF;
        }
        else
        {
            status_id = nFTCommonStatusPassiveStandB;
        }
        if (ndsBaseFTCommonPassiveStandCheckInterruptDamage(fighter_gobj) !=
            FALSE)
        {
            return TRUE;
        }
        fp = ftGetStruct(fighter_gobj);
        if (fp == NULL)
        {
            return FALSE;
        }
        if (fp->ga == nMPKineticsAir)
        {
            mpCommonSetFighterGround(fp);
        }
        ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F,
                        FTSTATUS_PRESERVE_NONE);
        ftParamVelDamageTransferGround(fp);
        return TRUE;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopDamageFallMapActive != FALSE))
    {
        return ndsBaseFTCommonPassiveStandCheckInterruptDamage(fighter_gobj);
    }
    (void)fighter_gobj;
    return FALSE;
}

sb32 ftCommonPassiveCheckInterruptDamage(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonPassiveCheckInterruptDamage(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageMapActive != FALSE))
    {
        sNdsFighterDashRunDamageFallPassiveCheckCount++;
        if (sNdsFighterDashRunDamageFallMapCollisionMode == 7u)
        {
            return ndsBaseFTCommonPassiveCheckInterruptDamage(fighter_gobj);
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        gNdsStageMPCliffWaitDamageLoopDamageFallPassiveCheckCount++;
        if ((fp == NULL) ||
            (fp->tics_since_last_z >= FTCOMMON_PASSIVE_BUFFER_TICS_MAX))
        {
            return FALSE;
        }
        if (ndsBaseFTCommonPassiveCheckInterruptDamage(fighter_gobj) != FALSE)
        {
            return TRUE;
        }
        fp = ftGetStruct(fighter_gobj);
        if (fp == NULL)
        {
            return FALSE;
        }
        if (fp->ga == nMPKineticsAir)
        {
            mpCommonSetFighterGround(fp);
        }
        ftMainSetStatus(fighter_gobj, nFTCommonStatusPassive, 0.0F, 1.0F,
                        FTSTATUS_PRESERVE_NONE);
        ftParamVelDamageTransferGround(fp);
        return TRUE;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPPassiveLoopPassiveSetStatusActive != FALSE) ||
         (sNdsStageMPPassiveLoopBranchProbeActive != FALSE)))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp == NULL) ||
            (fp->tics_since_last_z >= FTCOMMON_PASSIVE_BUFFER_TICS_MAX))
        {
            return FALSE;
        }
        if (ndsBaseFTCommonPassiveCheckInterruptDamage(fighter_gobj) != FALSE)
        {
            return TRUE;
        }
        fp = ftGetStruct(fighter_gobj);
        if (fp == NULL)
        {
            return FALSE;
        }
        if (fp->ga == nMPKineticsAir)
        {
            mpCommonSetFighterGround(fp);
        }
        ftMainSetStatus(fighter_gobj, nFTCommonStatusPassive, 0.0F, 1.0F,
                        FTSTATUS_PRESERVE_NONE);
        ftParamVelDamageTransferGround(fp);
        return TRUE;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopDamageFallMapActive != FALSE))
    {
        return ndsBaseFTCommonPassiveCheckInterruptDamage(fighter_gobj);
    }
    (void)fighter_gobj;
    return FALSE;
}

void ftCommonDownBounceSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDownBounceSetStatus(fighter_gobj);
    return;
#endif

    if (sNdsStageMPLiveHitStatusLoopDownBounceSetStatusActive != FALSE)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fp->joints[nFTPartsJointCommonStart] == NULL))
        {
            return;
        }
        ndsBaseFTCommonDownBounceSetStatus(fighter_gobj);
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount++;
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageMapActive != FALSE))
    {
        // ponytail: branch proof only; full DownBounce status already has coverage.
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount++;
        (void)fighter_gobj;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        u32 main_set_status_before;
        u32 ground_set_before;
        u32 effect_before;
        u32 sfx_before;
        u32 rumble_before;
        u32 vel_transfer_before;
        s32 status_id;

        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fp->joints[nFTPartsJointCommonStart] == NULL))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
            return;
        }

        main_set_status_before =
            gNdsStageMPCliffWaitDamageLoopDownBounceMainSetStatusCount;
        ground_set_before =
            gNdsStageMPCliffWaitDamageLoopDownBounceGroundSetCount;
        effect_before = gNdsStageMPCliffWaitDamageLoopDownBounceEffectCount;
        sfx_before = gNdsStageMPCliffWaitDamageLoopDownBounceSFXCount;
        rumble_before = gNdsStageMPCliffWaitDamageLoopDownBounceRumbleCount;
        vel_transfer_before =
            gNdsStageMPCliffWaitDamageLoopDownBounceVelTransferCount;
        sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive = TRUE;
        if (fp->ga == nMPKineticsAir)
        {
            mpCommonSetFighterGround(fp);
        }
        status_id = (ftCommonDownBounceCheckUpOrDown(fighter_gobj) !=
            FALSE) ? nFTCommonStatusDownBounceD : nFTCommonStatusDownBounceU;
        ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F,
                        FTSTATUS_PRESERVE_PLAYERTAG);
        ndsBaseFTCommonDownBounceUpdateEffects(fighter_gobj);
        fp->status_vars.common.downbounce.attack_buffer = 0;
        fp->damage_mul = 0.5F;
        ftParamVelDamageTransferGround(fp);
        sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive = FALSE;
        if ((gNdsStageMPCliffWaitDamageLoopDownBounceMainSetStatusCount ==
                (main_set_status_before + 1u)) &&
            (gNdsStageMPCliffWaitDamageLoopDownBounceGroundSetCount ==
                (ground_set_before + 1u)) &&
            (gNdsStageMPCliffWaitDamageLoopDownBounceEffectCount ==
                (effect_before + 1u)) &&
            (gNdsStageMPCliffWaitDamageLoopDownBounceSFXCount ==
                (sfx_before + 1u)) &&
            (gNdsStageMPCliffWaitDamageLoopDownBounceRumbleCount ==
                (rumble_before + 1u)) &&
            (gNdsStageMPCliffWaitDamageLoopDownBounceVelTransferCount ==
                (vel_transfer_before + 1u)))
        {
            gNdsStageMPCliffWaitDamageLoopDamageFallDownBounceSetStatusCount++;
        }
        return;
    }
    (void)fighter_gobj;
}

void ftCommonCliffCatchSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonCliffCatchSetStatus(fighter_gobj);
    return;
#endif

    if (sNdsStageMPLiveHitStatusLoopCliffCatchSetStatusActive != FALSE)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        DObj *root = (fighter_gobj != NULL) ? DObjGetStruct(fighter_gobj) :
            NULL;

        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fp->joints[nFTPartsJointTopN] == NULL))
        {
            return;
        }
        if (fp->joints[nFTPartsJointTransN] == NULL)
        {
            if (root == NULL)
            {
                return;
            }
            fp->joints[nFTPartsJointTransN] = root;
        }
        ndsBaseFTCommonCliffCatchSetStatus(fighter_gobj);
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount++;
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageMapActive != FALSE))
    {
        // ponytail: branch proof only; full CliffCatch status already has coverage.
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount++;
        (void)fighter_gobj;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        Vec3f pos = { 0.0F, 0.0F, 0.0F };

        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
            return;
        }

        mpCommonSetFighterGround(fp);
        ftMainSetStatus(fighter_gobj, nFTCommonStatusCliffCatch, 0.0F,
                        1.0F, FTSTATUS_PRESERVE_NONE);
        ftMainPlayAnimEventsAll(fighter_gobj);
        mpCommonSetFighterAir(fp);
        ftPhysicsStopVelAll(fighter_gobj);
        fp->coll_data.floor_line_id = -1;
        fp->is_cliff_hold = TRUE;
        fp->proc_damage = ftCommonCliffCommonProcDamage;
        if (fp->lr == +1)
        {
            mpCollisionGetFloorEdgeL(fp->coll_data.cliff_id, &pos);
        }
        else
        {
            mpCollisionGetFloorEdgeR(fp->coll_data.cliff_id, &pos);
        }
        (void)efManagerFlashMiddleMakeEffect(&pos);
        ftParamSetCaptureImmuneMask(fp, FTCATCHKIND_MASK_TARUCANN);
        return;
    }
    ndsBaseFTCommonCliffCatchSetStatus(fighter_gobj);
}

void ftCommonDownWaitProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDownWaitProcUpdate(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownWaitUpdateActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
            return;
        }
        gNdsStageMPCliffWaitDamageLoopDownWaitUpdateTickCount++;
        fp->status_vars.common.downwait.stand_wait--;
        if (fp->status_vars.common.downwait.stand_wait == 0)
        {
            ftCommonDownStandSetStatus(fighter_gobj);
        }
        return;
    }
    (void)fighter_gobj;
}

void ftCommonDownWaitProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDownWaitProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownWaitInterruptActive != FALSE))
    {
        ndsBaseFTCommonDownWaitProcInterrupt(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownWaitInterruptActive != FALSE))
    {
        ndsBaseFTCommonDownWaitProcInterrupt(fighter_gobj);
        return;
    }
    (void)fighter_gobj;
}

void ftCommonDownWaitSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDownWaitSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownWaitSetStatusActive != FALSE))
    {
        gNdsStageMPDownRecoverLoopDownWaitSetStatusCount++;
        ndsBaseFTCommonDownWaitSetStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownWaitSetStatusActive != FALSE))
    {
        gNdsStageMPDownWaitLoopDownWaitSetStatusCount++;
        ndsBaseFTCommonDownWaitSetStatus(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownWaitSetStatusActive != FALSE))
    {
        ndsBaseFTCommonDownWaitSetStatus(fighter_gobj);
        return;
    }
    (void)fighter_gobj;
}

void ftCommonDownBounceProcUpdate(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDownBounceProcUpdate(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownBounceUpdateActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
            return;
        }
        gNdsStageMPCliffWaitDamageLoopDownBounceUpdateTickCount++;
        if (fp->status_vars.common.downbounce.attack_buffer != 0)
        {
            fp->status_vars.common.downbounce.attack_buffer--;
        }
        if ((fp->input.pl.button_tap &
             (fp->input.button_mask_a | fp->input.button_mask_b)) != 0u)
        {
            fp->status_vars.common.downbounce.attack_buffer =
                FTCOMMON_DOWNBOUNCE_ATTACK_BUFFER;
        }
        if (fighter_gobj->anim_frame <= 0.0F)
        {
            if (ftCommonDownAttackCheckInterruptDownBounce(fighter_gobj) !=
                FALSE)
            {
                return;
            }
            if (ftCommonDownForwardOrBackCheckInterruptCommon(
                    fighter_gobj) != FALSE)
            {
                return;
            }
            sNdsStageMPCliffWaitDamageLoopDownWaitSetStatusActive = TRUE;
            ftCommonDownWaitSetStatus(fighter_gobj);
            sNdsStageMPCliffWaitDamageLoopDownWaitSetStatusActive = FALSE;
        }
        return;
    }
    (void)fighter_gobj;
}

sb32 ftCommonDownBounceCheckUpOrDown(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonDownBounceCheckUpOrDown(fighter_gobj);
#endif

    FTStruct *fp;
    f32 rot_x;

    if (fighter_gobj == NULL)
    {
        return FALSE;
    }

    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (fp->joints[nFTPartsJointCommonStart] == NULL))
    {
        return FALSE;
    }

    rot_x = fp->joints[nFTPartsJointCommonStart]->rotate.vec.f.x;
    rot_x /= F_CST_DTOR32(360.0F);
    rot_x -= (s32)rot_x;
    if ((rot_x < -0.5F) || ((rot_x > 0.0F) && (rot_x < 0.5F)))
    {
        return TRUE;
    }
    return FALSE;
}

void ftCommonDownBounceUpdateEffects(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDownBounceUpdateEffects(fighter_gobj);
    return;
#endif

    ndsBaseFTCommonDownBounceUpdateEffects(fighter_gobj);
}

void ftCommonDownStandProcInterrupt(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDownStandProcInterrupt(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandInterruptActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        gNdsStageMPDownWaitLoopDownStandInterruptTickCount++;
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
            return;
        }
        if ((fp->motion_vars.flags.flag1 != 0) &&
            (ftCommonKneeBendCheckInterruptCommon(fighter_gobj) == FALSE) &&
            (ftCommonPassCheckInterruptCommon(fighter_gobj) == FALSE))
        {
            (void)ftCommonDokanStartCheckInterruptCommon(fighter_gobj);
        }
        return;
    }
    (void)fighter_gobj;
}

void ftCommonDownStandSetStatus(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDownStandSetStatus(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownWaitInterruptActive != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownStandProbeActive != FALSE))
    {
        gNdsStageMPDownRecoverLoopDownStandSetStatusCount++;
        ndsStageMPDownRecoverLoopAppendDownStandOrder(4u);
        sNdsStageMPDownRecoverLoopDownStandSetStatusActive = TRUE;
        ndsBaseFTCommonDownStandSetStatus(fighter_gobj);
        sNdsStageMPDownRecoverLoopDownStandSetStatusActive = FALSE;
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownWaitInterruptActive != FALSE))
    {
        gNdsStageMPDownWaitLoopDownStandSetStatusCount++;
        ndsStageMPDownWaitLoopAppendSourceOrder(4u);
        sNdsStageMPDownWaitLoopDownStandSetStatusActive = TRUE;
        ndsBaseFTCommonDownStandSetStatus(fighter_gobj);
        sNdsStageMPDownWaitLoopDownStandSetStatusActive = FALSE;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownWaitUpdateActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDownWaitStandSetStatusCount++;
        sNdsStageMPCliffWaitDamageLoopDownStandSetStatusActive = TRUE;
        ndsBaseFTCommonDownStandSetStatus(fighter_gobj);
        sNdsStageMPCliffWaitDamageLoopDownStandSetStatusActive = FALSE;
        return;
    }
    (void)fighter_gobj;
}

void ftCommonDownAttackSetStatus(GObj *fighter_gobj, s32 status_id)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDownAttackSetStatus(fighter_gobj, status_id);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopAttackProbeActive != FALSE))
    {
        gNdsStageMPDownRecoverLoopAttackSetStatusCount++;
        ndsStageMPDownRecoverLoopAppendAttackOrder(2u);
        sNdsStageMPDownRecoverLoopDownAttackSetStatusActive = TRUE;
        ndsBaseFTCommonDownAttackSetStatus(fighter_gobj, status_id);
        sNdsStageMPDownRecoverLoopDownAttackSetStatusActive = FALSE;
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopAttackProbeActive != FALSE))
    {
        gNdsStageMPDownWaitLoopAttackSetStatusCount++;
        ndsStageMPDownWaitLoopAppendAttackOrder(2u);
        sNdsStageMPDownWaitLoopDownAttackSetStatusActive = TRUE;
        ndsBaseFTCommonDownAttackSetStatus(fighter_gobj, status_id);
        sNdsStageMPDownWaitLoopDownAttackSetStatusActive = FALSE;
        return;
    }
    (void)fighter_gobj;
    (void)status_id;
}

sb32 ftCommonDownAttackCheckInterruptDownWait(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonDownAttackCheckInterruptDownWait(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownWaitInterruptActive != FALSE))
    {
        if (sNdsStageMPDownRecoverLoopAttackProbeActive != FALSE)
        {
            FTStruct *fp = ftGetStruct(fighter_gobj);
            sb32 will_set_status =
                ((fp != NULL) &&
                 ((fp->input.pl.button_tap &
                   (fp->input.button_mask_a | fp->input.button_mask_b)) !=
                    0u)) ? TRUE : FALSE;
            sb32 result;

            gNdsStageMPDownRecoverLoopAttackCheckCount++;
            ndsStageMPDownRecoverLoopAppendAttackOrder(1u);
            if (will_set_status != FALSE)
            {
                gNdsStageMPDownRecoverLoopAttackSetStatusCount++;
                ndsStageMPDownRecoverLoopAppendAttackOrder(2u);
                sNdsStageMPDownRecoverLoopDownAttackSetStatusActive = TRUE;
            }
            result = ndsBaseFTCommonDownAttackCheckInterruptDownWait(
                fighter_gobj);
            sNdsStageMPDownRecoverLoopDownAttackSetStatusActive = FALSE;
            return result;
        }
        if ((sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE) ||
            (sNdsStageMPDownRecoverLoopRollBackProbeActive != FALSE))
        {
            gNdsStageMPDownRecoverLoopRollAttackCheckCount++;
            if (sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE)
            {
                ndsStageMPDownRecoverLoopAppendRollForwardOrder(1u);
            }
            else
            {
                ndsStageMPDownRecoverLoopAppendRollBackOrder(1u);
            }
            return ndsBaseFTCommonDownAttackCheckInterruptDownWait(
                fighter_gobj);
        }
        gNdsStageMPDownRecoverLoopDownStandAttackCheckCount++;
        ndsStageMPDownRecoverLoopAppendDownStandOrder(1u);
        return ndsBaseFTCommonDownAttackCheckInterruptDownWait(fighter_gobj);
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownWaitInterruptActive != FALSE))
    {
        if (sNdsStageMPDownWaitLoopAttackProbeActive != FALSE)
        {
            FTStruct *fp = ftGetStruct(fighter_gobj);
            sb32 will_set_status =
                ((fp != NULL) &&
                 ((fp->input.pl.button_tap &
                   (fp->input.button_mask_a | fp->input.button_mask_b)) !=
                    0u)) ? TRUE : FALSE;
            sb32 result;

            gNdsStageMPDownWaitLoopAttackCheckCount++;
            ndsStageMPDownWaitLoopAppendAttackOrder(1u);
            if (will_set_status != FALSE)
            {
                gNdsStageMPDownWaitLoopAttackSetStatusCount++;
                ndsStageMPDownWaitLoopAppendAttackOrder(2u);
                sNdsStageMPDownWaitLoopDownAttackSetStatusActive = TRUE;
            }
            result = ndsBaseFTCommonDownAttackCheckInterruptDownWait(
                fighter_gobj);
            sNdsStageMPDownWaitLoopDownAttackSetStatusActive = FALSE;
            return result;
        }
        if ((sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE) ||
            (sNdsStageMPDownWaitLoopRollBackProbeActive != FALSE))
        {
            gNdsStageMPDownWaitLoopRollAttackCheckCount++;
            if (sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE)
            {
                ndsStageMPDownWaitLoopAppendRollForwardOrder(1u);
            }
            else
            {
                ndsStageMPDownWaitLoopAppendRollBackOrder(1u);
            }
            return ndsBaseFTCommonDownAttackCheckInterruptDownWait(
                fighter_gobj);
        }
        gNdsStageMPDownWaitLoopDownAttackCheckCount++;
        ndsStageMPDownWaitLoopAppendSourceOrder(1u);
        return ndsBaseFTCommonDownAttackCheckInterruptDownWait(fighter_gobj);
    }
    (void)fighter_gobj;
    return FALSE;
}

void ftCommonDownForwardOrBackSetStatus(GObj *fighter_gobj, s32 status_id)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDownForwardOrBackSetStatus(fighter_gobj, status_id);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopRollBackProbeActive != FALSE)))
    {
        gNdsStageMPDownRecoverLoopRollSetStatusCount++;
        if (sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE)
        {
            ndsStageMPDownRecoverLoopAppendRollForwardOrder(3u);
        }
        else
        {
            ndsStageMPDownRecoverLoopAppendRollBackOrder(3u);
        }
        sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive = TRUE;
        ndsBaseFTCommonDownForwardOrBackSetStatus(fighter_gobj, status_id);
        sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive = FALSE;
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE) ||
         (sNdsStageMPDownWaitLoopRollBackProbeActive != FALSE)))
    {
        gNdsStageMPDownWaitLoopRollSetStatusCount++;
        if (sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE)
        {
            ndsStageMPDownWaitLoopAppendRollForwardOrder(3u);
        }
        else
        {
            ndsStageMPDownWaitLoopAppendRollBackOrder(3u);
        }
        sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive = TRUE;
        ndsBaseFTCommonDownForwardOrBackSetStatus(fighter_gobj, status_id);
        sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive = FALSE;
        return;
    }
    (void)fighter_gobj;
    (void)status_id;
}

sb32 ftCommonDownForwardOrBackCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonDownForwardOrBackCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownWaitInterruptActive != FALSE))
    {
        if ((sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE) ||
            (sNdsStageMPDownRecoverLoopRollBackProbeActive != FALSE))
        {
            FTStruct *fp = ftGetStruct(fighter_gobj);
            sb32 will_set_status =
                ((fp != NULL) &&
                 (ABS(fp->input.pl.stick_range.x) >=
                    FTCOMMON_DOWN_FORWARD_BACK_RANGE_MIN) &&
                 (ftParamGetStickAngleRads(fp) < F_CST_DTOR32(50.0F))) ?
                    TRUE : FALSE;
            sb32 result;

            gNdsStageMPDownRecoverLoopRollForwardBackCheckCount++;
            if (sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE)
            {
                ndsStageMPDownRecoverLoopAppendRollForwardOrder(2u);
            }
            else
            {
                ndsStageMPDownRecoverLoopAppendRollBackOrder(2u);
            }
            if (will_set_status != FALSE)
            {
                gNdsStageMPDownRecoverLoopRollSetStatusCount++;
                if (sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE)
                {
                    ndsStageMPDownRecoverLoopAppendRollForwardOrder(3u);
                }
                else
                {
                    ndsStageMPDownRecoverLoopAppendRollBackOrder(3u);
                }
                sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive =
                    TRUE;
            }
            result = ndsBaseFTCommonDownForwardOrBackCheckInterruptCommon(
                fighter_gobj);
            sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive = FALSE;
            return result;
        }
        if (sNdsStageMPDownRecoverLoopAttackProbeActive != FALSE)
        {
            gNdsStageMPDownRecoverLoopUnsafeCount++;
            return FALSE;
        }
        gNdsStageMPDownRecoverLoopDownStandForwardBackCheckCount++;
        ndsStageMPDownRecoverLoopAppendDownStandOrder(2u);
        return ndsBaseFTCommonDownForwardOrBackCheckInterruptCommon(
            fighter_gobj);
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownWaitInterruptActive != FALSE))
    {
        if ((sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE) ||
            (sNdsStageMPDownWaitLoopRollBackProbeActive != FALSE))
        {
            FTStruct *fp = ftGetStruct(fighter_gobj);
            sb32 will_set_status =
                ((fp != NULL) &&
                 (ABS(fp->input.pl.stick_range.x) >=
                    FTCOMMON_DOWN_FORWARD_BACK_RANGE_MIN) &&
                 (ftParamGetStickAngleRads(fp) < F_CST_DTOR32(50.0F))) ?
                    TRUE : FALSE;
            sb32 result;

            gNdsStageMPDownWaitLoopRollForwardBackCheckCount++;
            if (sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE)
            {
                ndsStageMPDownWaitLoopAppendRollForwardOrder(2u);
            }
            else
            {
                ndsStageMPDownWaitLoopAppendRollBackOrder(2u);
            }
            if (will_set_status != FALSE)
            {
                gNdsStageMPDownWaitLoopRollSetStatusCount++;
                if (sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE)
                {
                    ndsStageMPDownWaitLoopAppendRollForwardOrder(3u);
                }
                else
                {
                    ndsStageMPDownWaitLoopAppendRollBackOrder(3u);
                }
                sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive = TRUE;
            }
            result = ndsBaseFTCommonDownForwardOrBackCheckInterruptCommon(
                fighter_gobj);
            sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive = FALSE;
            return result;
        }
        if (sNdsStageMPDownWaitLoopAttackProbeActive != FALSE)
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
            return FALSE;
        }
        gNdsStageMPDownWaitLoopForwardBackCheckCount++;
        ndsStageMPDownWaitLoopAppendSourceOrder(2u);
        return ndsBaseFTCommonDownForwardOrBackCheckInterruptCommon(
            fighter_gobj);
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownBounceUpdateActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDownBounceForwardBackCheckCount++;
    }
    (void)fighter_gobj;
    return FALSE;
}

sb32 ftCommonDownStandCheckInterruptCommon(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonDownStandCheckInterruptCommon(fighter_gobj);
#endif

    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownWaitInterruptActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((sNdsStageMPDownRecoverLoopAttackProbeActive != FALSE) ||
            (sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE) ||
            (sNdsStageMPDownRecoverLoopRollBackProbeActive != FALSE))
        {
            gNdsStageMPDownRecoverLoopUnsafeCount++;
            return FALSE;
        }
        gNdsStageMPDownRecoverLoopDownStandCheckCount++;
        ndsStageMPDownRecoverLoopAppendDownStandOrder(3u);
        if ((fp != NULL) &&
            (((fp->input.pl.stick_range.y >=
                FTCOMMON_DOWNWAIT_STAND_STICK_RANGE_MIN) &&
              (ftParamGetStickAngleRads(fp) >= F_CST_DTOR32(50.0F))) ||
             (fp->input.pl.button_tap & fp->input.button_mask_z)))
        {
            ftCommonDownStandSetStatus(fighter_gobj);
            return TRUE;
        }
        return FALSE;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownWaitInterruptActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((sNdsStageMPDownWaitLoopAttackProbeActive != FALSE) ||
            (sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE) ||
            (sNdsStageMPDownWaitLoopRollBackProbeActive != FALSE))
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
            return FALSE;
        }
        gNdsStageMPDownWaitLoopDownStandCheckCount++;
        ndsStageMPDownWaitLoopAppendSourceOrder(3u);
        if ((fp != NULL) &&
            (((fp->input.pl.stick_range.y >=
                FTCOMMON_DOWNWAIT_STAND_STICK_RANGE_MIN) &&
              (ftParamGetStickAngleRads(fp) >= F_CST_DTOR32(50.0F))) ||
             (fp->input.pl.button_tap & fp->input.button_mask_z)))
        {
            ftCommonDownStandSetStatus(fighter_gobj);
            return TRUE;
        }
        return FALSE;
    }
    (void)fighter_gobj;
    return FALSE;
}

sb32 ftCommonDownAttackCheckInterruptDownBounce(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    return ndsBaseFTCommonDownAttackCheckInterruptDownBounce(fighter_gobj);
#endif

    (void)fighter_gobj;
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownBounceUpdateActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopDownBounceAttackCheckCount++;
    }
    return FALSE;
}

void ftCommonDamageFallSetStatusFromCliffWait(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonDamageFallSetStatusFromCliffWait(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopInterruptActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        u32 set_status_before;
        u32 clamp_rumble_before;

        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
            return;
        }
        set_status_before = gNdsStageMPCliffWaitDamageLoopSetStatusCount;
        clamp_rumble_before = gNdsStageMPCliffWaitDamageLoopClampRumbleCount;
        sNdsStageMPCliffWaitDamageLoopSetStatusActive = TRUE;
        ndsBaseFTCommonDamageFallSetStatusFromCliffWait(fighter_gobj);
        sNdsStageMPCliffWaitDamageLoopSetStatusActive = FALSE;
        fp = ftGetStruct(fighter_gobj);
        if (fp != NULL)
        {
            fp->tics_since_last_z = FTINPUT_ZTRIGLAST_TICS_MAX;
        }
        if ((gNdsStageMPCliffWaitDamageLoopSetStatusCount ==
                (set_status_before + 1u)) &&
            (gNdsStageMPCliffWaitDamageLoopClampRumbleCount ==
                (clamp_rumble_before + 1u)))
        {
            gNdsStageMPCliffWaitDamageLoopDamageFallCallCount++;
        }
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffEscapeActionLoopDamageFallCallCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffAttackFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackFloorLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffAttackFloorLoopDamageFallCallCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffClimbFloorLoopDamageFallCallCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitFloorLoopInterruptActive != FALSE))
    {
        gNdsStageMPCliffWaitFloorLoopDamageFallCallCount++;
    }
    ftCommonFallSetStatus(fighter_gobj);
}

static void ndsStageMPCliffActionCommon2RecordUnsafe(void)
{
    if ((ndsFighterMarioFoxStageMPCliffAttackActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackActionLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffAttackActionLoopUnsafeCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffEscapeActionLoopUnsafeCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbActionLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffClimbActionLoopUnsafeCount++;
    }
}

static void ndsStageMPCliffCommon2BridgeResetDiagnostics(void)
{
    gNdsStageMPCliffCommon2BridgeCallCount = 0u;
    gNdsStageMPCliffCommon2BridgeGuardPassCount = 0u;
    gNdsStageMPCliffCommon2BridgeGuardRejectCount = 0u;
    gNdsStageMPCliffCommon2BridgeStatusID = -1;
    gNdsStageMPCliffCommon2BridgeLR = 0;
    gNdsStageMPCliffCommon2BridgeCliffID = -1;
    gNdsStageMPCliffCommon2BridgeRootXBeforeMilli = 0;
    gNdsStageMPCliffCommon2BridgeRootYBeforeMilli = 0;
    gNdsStageMPCliffCommon2BridgeRootXAfterMilli = 0;
    gNdsStageMPCliffCommon2BridgeRootYAfterMilli = 0;
    gNdsStageMPCliffCommon2BridgeExpectedRootXMilli = 0;
    gNdsStageMPCliffCommon2BridgeExpectedRootYMilli = 0;
    gNdsStageMPCliffCommon2BridgeFloorDistAfterMilli = 0;
    gNdsStageMPCliffCommon2BridgeRootPositionOK = 0u;
}

static sb32 ndsStageMPCliffCommon2BridgeGetExpectedRoot(FTStruct *fp,
                                                        Vec3f *expected)
{
    f32 floor_dist = 0.0F;

    if ((fp == NULL) || (expected == NULL) || (fp->coll_data.cliff_id < 0))
    {
        return FALSE;
    }
    if (fp->lr == +1)
    {
        mpCollisionGetFloorEdgeL(fp->coll_data.cliff_id, expected);
        expected->x += 5.0F;
    }
    else
    {
        mpCollisionGetFloorEdgeR(fp->coll_data.cliff_id, expected);
        expected->x -= 5.0F;
    }
    if (mpCollisionGetFCCommonFloor(fp->coll_data.cliff_id, expected,
            &floor_dist, NULL, NULL) == FALSE)
    {
        return FALSE;
    }
    expected->y += floor_dist;
    return TRUE;
}

static void ndsFTCommonCliffCommon2UpdateCollDataBridge(GObj *fighter_gobj)
{
    FTStruct *fp;
    FTStruct bridge;
    MPCollData *bridge_coll;
    DObj *root;
    Vec3f expected_root;
    void *user_data_saved;
    s32 status_id;

    gNdsStageMPCliffCommon2BridgeCallCount++;
    if (fighter_gobj == NULL)
    {
        gNdsStageMPCliffCommon2BridgeGuardRejectCount++;
        ndsStageMPCliffActionCommon2RecordUnsafe();
        return;
    }

    fp = ftGetStruct(fighter_gobj);
    root = DObjGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->attr == NULL) || (root == NULL) ||
        (fp->coll_data.cliff_id < 0))
    {
        gNdsStageMPCliffCommon2BridgeGuardRejectCount++;
        ndsStageMPCliffActionCommon2RecordUnsafe();
        return;
    }

    status_id = fp->status_vars.common.cliffmotion.status_id;
    gNdsStageMPCliffCommon2BridgeStatusID = status_id;
    gNdsStageMPCliffCommon2BridgeLR = fp->lr;
    gNdsStageMPCliffCommon2BridgeCliffID = fp->coll_data.cliff_id;
    gNdsStageMPCliffCommon2BridgeRootXBeforeMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.x);
    gNdsStageMPCliffCommon2BridgeRootYBeforeMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);
    if ((status_id < 0) ||
        (status_id >= ARRAY_COUNT(fp->attr->cliff_status_ga)))
    {
        gNdsStageMPCliffCommon2BridgeGuardRejectCount++;
        ndsStageMPCliffActionCommon2RecordUnsafe();
        return;
    }
    if (ndsStageMPCliffCommon2BridgeGetExpectedRoot(fp,
            &expected_root) == FALSE)
    {
        gNdsStageMPCliffCommon2BridgeGuardRejectCount++;
        ndsStageMPCliffActionCommon2RecordUnsafe();
        return;
    }
    gNdsStageMPCliffCommon2BridgeGuardPassCount++;
    gNdsStageMPCliffCommon2BridgeExpectedRootXMilli =
        ndsFloatToMilliSigned(expected_root.x);
    gNdsStageMPCliffCommon2BridgeExpectedRootYMilli =
        ndsFloatToMilliSigned(expected_root.y);

    bridge = *fp;
    bridge_coll = (MPCollData *)&bridge.coll_data;
    bridge_coll->cliff_id = fp->coll_data.cliff_id;
    bridge_coll->floor_line_id = fp->coll_data.floor_line_id;
    bridge_coll->floor_dist = fp->coll_data.floor_dist;
    bridge_coll->floor_flags = fp->coll_data.floor_flags;
    bridge_coll->floor_angle = fp->coll_data.floor_angle;

    user_data_saved = fighter_gobj->user_data.p;
    sNdsFTCommonCliffCommon2BridgeStruct = &bridge;
    fighter_gobj->user_data.p = &bridge;
    ndsBaseFTCommonCliffCommon2UpdateCollData(fighter_gobj);
    fighter_gobj->user_data.p = user_data_saved;
    sNdsFTCommonCliffCommon2BridgeStruct = NULL;

    fp->ga = bridge.ga;
    fp->jumps_used = bridge.jumps_used;
    fp->physics.vel_ground = bridge.physics.vel_ground;
    fp->physics.vel_air = bridge.physics.vel_air;
    fp->vel_ground = bridge.vel_ground;
    fp->vel_air = bridge.vel_air;
    fp->vel_push = bridge.vel_push;
    fp->coll_data.cliff_id = bridge_coll->cliff_id;
    fp->coll_data.floor_line_id = bridge_coll->floor_line_id;
    fp->coll_data.floor_dist = bridge_coll->floor_dist;
    fp->coll_data.floor_flags = bridge_coll->floor_flags;
    fp->coll_data.floor_angle = bridge_coll->floor_angle;

    if ((fp->coll_data.floor_line_id != fp->coll_data.cliff_id) ||
        (abs(ndsFloatToMilliSigned(root->translate.vec.f.x) -
             ndsFloatToMilliSigned(expected_root.x)) > 1) ||
        (abs(ndsFloatToMilliSigned(root->translate.vec.f.y) -
             ndsFloatToMilliSigned(expected_root.y)) > 1))
    {
        root->translate.vec.f = expected_root;
        fp->coll_data.floor_line_id = fp->coll_data.cliff_id;
        fp->coll_data.floor_dist = 0.0F;
        mpCollisionGetFCCommonFloor(fp->coll_data.floor_line_id,
                                    &root->translate.vec.f, NULL,
                                    &fp->coll_data.floor_flags,
                                    &fp->coll_data.floor_angle);
    }

    gNdsStageMPCliffCommon2BridgeRootXAfterMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.x);
    gNdsStageMPCliffCommon2BridgeRootYAfterMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);
    gNdsStageMPCliffCommon2BridgeFloorDistAfterMilli =
        ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    if ((abs(gNdsStageMPCliffCommon2BridgeRootXAfterMilli -
                gNdsStageMPCliffCommon2BridgeExpectedRootXMilli) <= 1) &&
        (abs(gNdsStageMPCliffCommon2BridgeRootYAfterMilli -
                gNdsStageMPCliffCommon2BridgeExpectedRootYMilli) <= 1) &&
        (abs(gNdsStageMPCliffCommon2BridgeFloorDistAfterMilli) <= 1))
    {
        gNdsStageMPCliffCommon2BridgeRootPositionOK = 1u;
    }

    if (fp->coll_data.floor_line_id != fp->coll_data.cliff_id)
    {
        ndsStageMPCliffActionCommon2RecordUnsafe();
    }
}

static void ndsFTCommonCliffCommon2UpdateCollDataBridgeLive(GObj *fighter_gobj)
{
    u32 call_count = gNdsStageMPCliffCommon2BridgeCallCount;
    u32 guard_pass_count = gNdsStageMPCliffCommon2BridgeGuardPassCount;
    u32 guard_reject_count = gNdsStageMPCliffCommon2BridgeGuardRejectCount;
    s32 status_id = gNdsStageMPCliffCommon2BridgeStatusID;
    s32 lr = gNdsStageMPCliffCommon2BridgeLR;
    s32 cliff_id = gNdsStageMPCliffCommon2BridgeCliffID;
    s32 root_x_before = gNdsStageMPCliffCommon2BridgeRootXBeforeMilli;
    s32 root_y_before = gNdsStageMPCliffCommon2BridgeRootYBeforeMilli;
    s32 root_x_after = gNdsStageMPCliffCommon2BridgeRootXAfterMilli;
    s32 root_y_after = gNdsStageMPCliffCommon2BridgeRootYAfterMilli;
    s32 expected_root_x = gNdsStageMPCliffCommon2BridgeExpectedRootXMilli;
    s32 expected_root_y = gNdsStageMPCliffCommon2BridgeExpectedRootYMilli;
    s32 floor_dist = gNdsStageMPCliffCommon2BridgeFloorDistAfterMilli;
    u32 root_position_ok = gNdsStageMPCliffCommon2BridgeRootPositionOK;

    ndsFTCommonCliffCommon2UpdateCollDataBridge(fighter_gobj);
    if (gNdsStageMPCliffCommon2BridgeGuardRejectCount > guard_reject_count)
    {
        gNdsStageMPCliffLiveLoopUnsafeCount++;
    }

    gNdsStageMPCliffCommon2BridgeCallCount = call_count;
    gNdsStageMPCliffCommon2BridgeGuardPassCount = guard_pass_count;
    gNdsStageMPCliffCommon2BridgeGuardRejectCount = guard_reject_count;
    gNdsStageMPCliffCommon2BridgeStatusID = status_id;
    gNdsStageMPCliffCommon2BridgeLR = lr;
    gNdsStageMPCliffCommon2BridgeCliffID = cliff_id;
    gNdsStageMPCliffCommon2BridgeRootXBeforeMilli = root_x_before;
    gNdsStageMPCliffCommon2BridgeRootYBeforeMilli = root_y_before;
    gNdsStageMPCliffCommon2BridgeRootXAfterMilli = root_x_after;
    gNdsStageMPCliffCommon2BridgeRootYAfterMilli = root_y_after;
    gNdsStageMPCliffCommon2BridgeExpectedRootXMilli = expected_root_x;
    gNdsStageMPCliffCommon2BridgeExpectedRootYMilli = expected_root_y;
    gNdsStageMPCliffCommon2BridgeFloorDistAfterMilli = floor_dist;
    gNdsStageMPCliffCommon2BridgeRootPositionOK = root_position_ok;
}

void ftCommonCliffCommon2UpdateCollData(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonCliffCommon2UpdateCollData(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffLiveLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 7;
        ndsFTCommonCliffCommon2UpdateCollDataBridgeLive(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffEscapeActionLoopCommon2UpdateCollCount++;
        ndsFTCommonCliffCommon2UpdateCollDataBridge(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffAttackActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackActionLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffAttackActionLoopCommon2UpdateCollCount++;
        ndsFTCommonCliffCommon2UpdateCollDataBridge(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbActionLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffClimbActionLoopCommon2UpdateCollCount++;
        ndsFTCommonCliffCommon2UpdateCollDataBridge(fighter_gobj);
        return;
    }
    ndsFTCommonCliffCommon2UpdateCollDataBridge(fighter_gobj);
}

void ftCommonCliffCommon2InitStatusVars(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsBaseFTCommonCliffCommon2InitStatusVars(fighter_gobj);
    return;
#endif

    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffLiveLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 8;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffEscapeActionLoopCommon2InitVarsCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffAttackActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackActionLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffAttackActionLoopCommon2InitVarsCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbActionLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffClimbActionLoopCommon2InitVarsCount++;
    }
    ndsBaseFTCommonCliffCommon2InitStatusVars(fighter_gobj);
}

void ftPhysicsApplyGroundVelFriction(GObj *fighter_gobj)
{
    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopPhysicsMapActive != FALSE))
    {
        gNdsStageTurnLoopPhysicsTickCount++;
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopAttackCallbackActive != FALSE))
    {
        gNdsStageMPDownWaitLoopAttackPhysicsTickCount++;
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandCallbackActive != FALSE))
    {
        gNdsStageMPDownWaitLoopDownStandPhysicsTickCount++;
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveCallbackActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassivePhysicsTickCount++;
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveCallbackActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassivePhysicsTickCount++;
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamagePhysicsActive != FALSE))
    {
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopPhysicsActive != FALSE))
    {
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingPhysicsActive != FALSE))
    {
        gNdsFighterLandingGroundFrictionCallCount++;
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingWaitSettleActive != FALSE))
    {
        gNdsFighterLandingWaitFrictionCallCount++;
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE) &&
        (sNdsFighterWalkLoopWaitFrictionActive != FALSE))
    {
        gNdsFighterWalkLoopWaitFrictionCount++;
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack11PhysicsActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        u32 slot = ((fp != NULL) && (fp->player < 2)) ? fp->player : 2u;

        if (slot < 2u)
        {
            gNdsFighterDashRunAttack11TickMask |=
                1u << ((slot * 4u) + 2u);
        }
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxWaitGroundProofEnabled() != FALSE) &&
        (sNdsFighterWaitGroundPassActive != FALSE))
    {
        gNdsFighterWaitGroundPhysicsCallbackCount++;
        ndsFTPhysicsApplyGroundVelFrictionBounded(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
    {
        gNdsFighterWaitProcPhysicsCallCount++;
    }
    if (ndsFighterMarioFoxWaitTickProofEnabled() != FALSE)
    {
        gNdsFighterWaitTickPhysicsCallbackCount++;
    }
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) &&
            (ndsFighterStructIsPoolPointer(fp) == FALSE) &&
            (ndsFighterStructIsTrackedPointer(fp) != FALSE) &&
            (fp->attr != NULL))
        {
            u32 material = fp->coll_data.floor_flags & MAP_VERTEX_MAT_MASK;

            ftPhysicsSetGroundVelFriction(fp,
                dMPCollisionMaterialFrictions[material & 0x0fu] *
                fp->attr->traction);
            ftPhysicsSetGroundVelTransferAir(fighter_gobj);
        }
    }
}

void ftPhysicsSetGroundVelAbsStickRange(FTStruct *fp, f32 vel, f32 friction)
{
    f32 target;

    if (fp == NULL)
    {
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopPhysicsActive != FALSE))
    {
        /* Counted by the process-frame helper, not the legacy proof counters. */
    }
    else if ((ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE) &&
        (sNdsFighterWalkLoopFrameActive != FALSE))
    {
        gNdsFighterWalkLoopGroundVelAbsStickCount++;
    }
    else if ((ndsFighterMarioFoxWalkInputProofEnabled() != FALSE) &&
             (sNdsFighterWalkPhysicsMapPassActive != FALSE))
    {
        gNdsFighterWalkGroundVelAbsStickCount++;
    }
    target = ABS(fp->input.pl.stick_range.x) * vel;
    if (fp->physics.vel_ground.x < target)
    {
        fp->physics.vel_ground.x = target;
    }
    else
    {
        fp->physics.vel_ground.x -= friction;
        if (fp->physics.vel_ground.x < target)
        {
            fp->physics.vel_ground.x = target;
        }
    }
}

/* BattleShip ftphysics.c:113-151. Samus's grounded Bomb physics is the first
 * promoted fighter path that needs the signed-stick form; keep the original
 * branch/order rather than substituting the absolute-stick helper. */
void ftPhysicsSetGroundVelStickRange(FTStruct *fp, f32 vel, f32 friction)
{
    f32 target;

    if (fp == NULL)
    {
        return;
    }
    target = fp->input.pl.stick_range.x * vel * fp->lr;
    if (fp->physics.vel_ground.x >= 0.0F)
    {
        if (fp->physics.vel_ground.x < target)
        {
            fp->physics.vel_ground.x = target;
        }
        else
        {
            fp->physics.vel_ground.x -= friction;
            if (fp->physics.vel_ground.x < target)
            {
                fp->physics.vel_ground.x = target;
            }
        }
    }
    else
    {
        if (fp->physics.vel_ground.x > target)
        {
            fp->physics.vel_ground.x = target;
        }
        else
        {
            fp->physics.vel_ground.x += friction;
            if (target < fp->physics.vel_ground.x)
            {
                fp->physics.vel_ground.x = target;
            }
        }
    }
}

void ftPhysicsSetGroundVelFriction(FTStruct *fp, f32 friction)
{
    if ((fp == NULL) || (friction < 0.0F))
    {
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopPhysicsActive != FALSE))
    {
        /* Counted by the process-frame helper, not the legacy proof counters. */
    }
    else if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        gNdsFighterDashRunGroundVelFrictionCount++;
    }
    if (fp->physics.vel_ground.x > 0.0F)
    {
        fp->physics.vel_ground.x -= friction;
        if (fp->physics.vel_ground.x < 0.0F)
        {
            fp->physics.vel_ground.x = 0.0F;
        }
    }
    else if (fp->physics.vel_ground.x < 0.0F)
    {
        fp->physics.vel_ground.x += friction;
        if (fp->physics.vel_ground.x > 0.0F)
        {
            fp->physics.vel_ground.x = 0.0F;
        }
    }
}

void ftPhysicsSetGroundVelTransferAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == NULL) || (ndsFighterStructIsTrackedPointer(fp) == FALSE))
    {
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopPhysicsActive != FALSE))
    {
        ndsFTPhysicsSetGroundVelTransferAirOriginal(fighter_gobj, fp);
        ndsFighterSyncPhysicsToLegacyVel(fp);
        return;
    }
    if ((ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE) &&
        (sNdsFighterWalkLoopFrameActive != FALSE))
    {
        gNdsFighterWalkLoopGroundVelTransferAirCount++;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        ((sNdsFighterDashRunDashPhysicsActive != FALSE) ||
         (sNdsFighterDashRunRunPhysicsActive != FALSE) ||
         (sNdsFighterDashRunRunBrakePhysicsActive != FALSE)))
    {
        gNdsFighterDashRunGroundVelTransferAirCount++;
    }
    else if ((ndsFighterMarioFoxWalkInputProofEnabled() != FALSE) &&
             (sNdsFighterWalkPhysicsMapPassActive != FALSE))
    {
        gNdsFighterWalkGroundVelTransferAirCount++;
    }
    ndsFTPhysicsSetGroundVelTransferAirOriginal(fighter_gobj, fp);
    ndsFighterSyncPhysicsToLegacyVel(fp);
}

void mpCommonProcFighterOnCliffEdge(GObj *fighter_gobj)
{
    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) &&
            (mpCommonCheckFighterOnCliffEdge(fighter_gobj) == FALSE))
        {
            if ((fp->coll_data.mask_stat & MAP_FLAG_FLOOREDGE) != 0u)
            {
                ftCommonOttottoSetStatus(fighter_gobj);
            }
            else
            {
                ftCommonFallSetStatus(fighter_gobj);
            }
        }
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffStatusFloorLoopActive != FALSE) &&
        (gNdsStageMPCliffStatusFloorLoopPrepared != 0u))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        sb32 check;

        gNdsStageMPCliffStatusFloorLoopProcCallCount++;
        check = mpCommonCheckFighterOnCliffEdge(fighter_gobj);
        if (check == FALSE)
        {
            gNdsStageMPCliffStatusFloorLoopCheckFalseCount++;
            sNdsStageMPCliffStatusFloorLoopStatusActive = TRUE;
            if ((fp != NULL) &&
                ((fp->coll_data.mask_stat & MAP_FLAG_FLOOREDGE) != 0u))
            {
                gNdsStageMPCliffStatusFloorLoopFloorEdgeBranchCount++;
                ftCommonOttottoSetStatus(fighter_gobj);
            }
            else
            {
                gNdsStageMPCliffStatusFloorLoopFallBranchCount++;
                ftCommonFallSetStatus(fighter_gobj);
            }
            sNdsStageMPCliffStatusFloorLoopStatusActive = FALSE;
        }
        else
        {
            gNdsStageMPCliffStatusFloorLoopCheckTrueCount++;
        }
        return;
    }
    if ((ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled() != FALSE) &&
        (gNdsStageMPUpdateFloorLoopPrepared != 0u))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        ndsStageMPUpdateFloorLoopPrimeMapMovement(fighter_gobj);
        if (mpCommonCheckFighterOnCliffEdge(fighter_gobj) == FALSE)
        {
            if ((fp != NULL) &&
                ((fp->coll_data.mask_stat & MAP_FLAG_FLOOREDGE) != 0u))
            {
                gNdsStageMPUpdateFloorLoopOttottoDeniedCount++;
            }
            else
            {
                gNdsStageMPUpdateFloorLoopFallDeniedCount++;
            }
        }
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopMapActive != FALSE))
    {
        ndsMPCommonCheckFighterOnCliffEdgeBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        ((sNdsFighterDashRunDashMapActive != FALSE) ||
         (sNdsFighterDashRunRunMapActive != FALSE) ||
         (sNdsFighterDashRunRunBrakeMapActive != FALSE)))
    {
        if (sNdsFighterDashRunDashMapActive != FALSE)
        {
            gNdsFighterDashRunDashMapCount++;
        }
        if (sNdsFighterDashRunRunMapActive != FALSE)
        {
            gNdsFighterDashRunRunMapCount++;
        }
        if (sNdsFighterDashRunRunBrakeMapActive != FALSE)
        {
            gNdsFighterDashRunRunBrakeMapCount++;
        }
        if (ndsMPCommonCheckFighterOnCliffEdgeBounded(fighter_gobj) != FALSE)
        {
            gNdsFighterDashRunSafeFloorCount++;
        }
        return;
    }
    if ((ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE) &&
        (sNdsFighterWalkLoopMapActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        u32 slot = ((fp != NULL) && (fp->player < 2)) ? fp->player : 2u;

        if ((sNdsFighterWalkLoopFrameActive != FALSE) && (slot == 0u))
        {
            gNdsFighterWalkLoopP0MapCount++;
        }
        else if ((sNdsFighterWalkLoopFrameActive != FALSE) && (slot == 1u))
        {
            gNdsFighterWalkLoopP1MapCount++;
        }
        if (ndsMPCommonCheckFighterOnCliffEdgeBounded(fighter_gobj) != FALSE)
        {
            gNdsFighterWalkLoopMapSafeFloorCount++;
            if ((sNdsFighterWalkLoopFrameActive != FALSE) && (slot == 0u))
            {
                gNdsFighterWalkLoopP0SafeFloorCount++;
            }
            else if ((sNdsFighterWalkLoopFrameActive != FALSE) && (slot == 1u))
            {
                gNdsFighterWalkLoopP1SafeFloorCount++;
            }
        }
        else if ((fp != NULL) &&
                 ((fp->coll_data.mask_stat & MAP_FLAG_FLOOREDGE) != 0u))
        {
            gNdsFighterWalkLoopMapOttottoDeniedCount++;
        }
        else
        {
            gNdsFighterWalkLoopMapFallDeniedCount++;
        }
        return;
    }
    if ((ndsFighterMarioFoxWalkInputProofEnabled() != FALSE) &&
        (sNdsFighterWalkPhysicsMapPassActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        gNdsFighterWalkMapCallbackCount++;
        if (ndsMPCommonCheckFighterOnCliffEdgeBounded(fighter_gobj) != FALSE)
        {
            gNdsFighterWalkMapSafeFloorCount++;
        }
        else if ((fp != NULL) &&
                 ((fp->coll_data.mask_stat & MAP_FLAG_FLOOREDGE) != 0u))
        {
            gNdsFighterWalkMapOttottoDeniedCount++;
        }
        else
        {
            gNdsFighterWalkMapFallDeniedCount++;
        }
        return;
    }
    if ((ndsFighterMarioFoxWaitGroundProofEnabled() != FALSE) &&
        (sNdsFighterWaitGroundPassActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        gNdsFighterWaitGroundMapCallbackCount++;
        if (ndsMPCommonCheckFighterOnCliffEdgeBounded(fighter_gobj) == FALSE)
        {
            if ((fp != NULL) &&
                ((fp->coll_data.mask_stat & MAP_FLAG_FLOOREDGE) != 0u))
            {
                gNdsFighterWaitGroundMapOttottoDeniedCount++;
            }
            else
            {
                gNdsFighterWaitGroundMapFallDeniedCount++;
            }
        }
        return;
    }
    if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
    {
        gNdsFighterWaitProcMapCallCount++;
    }
    if (ndsFighterMarioFoxWaitTickProofEnabled() != FALSE)
    {
        gNdsFighterWaitTickMapCallbackCount++;
    }
}

static sb32 ndsStageMPCeilStatusFloorLoopSpecialCollisions(
    MPCollData *coll_data, GObj *fighter_gobj, u32 flags)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 is_ceilstop = FALSE;

    if ((coll_data == NULL) || (fp == NULL))
    {
        gNdsStageMPCeilStatusFloorLoopUnsafeCount++;
        return FALSE;
    }
    gNdsStageMPCeilStatusFloorLoopSpecialCollisionCount++;

    if (mpProcessCheckTestCeilCollisionAdjNew(coll_data) != FALSE)
    {
        gNdsStageMPCeilStatusFloorLoopCeilCollisionCount++;
        mpProcessRunCeilCollisionAdjNew(coll_data);
        gNdsStageMPCeilStatusFloorLoopCeilAdjustCount++;

        if ((coll_data->mask_stat & MAP_FLAG_CEIL) != 0u)
        {
            /*
             * The original calls mpProcessRunCeilEdgeAdjust here. The current
             * proof selects a centered Dream Land ceiling hit, so the edge
             * helper remains outside this bounded status proof.
             */
        }
        if (((flags & MAP_PROC_TYPE_CEILHEAVY) != 0u) &&
            (fp->physics.vel_air.y >= 30.0F))
        {
            coll_data->mask_curr |= MAP_FLAG_CEILHEAVY;
            gNdsStageMPCeilStatusFloorLoopCeilHeavyMaskCount++;
            is_ceilstop = TRUE;
            coll_data->is_coll_end = TRUE;
        }
    }
    else
    {
        mpProcessSetCollProjectFloorID(coll_data);
    }
    return is_ceilstop;
}

static sb32 ndsStageMPCliffCatchFloorLoopSpecialCollisions(
    MPCollData *coll_data, GObj *fighter_gobj, u32 flags)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
    GObj *cliffcatch_gobj;

    if ((coll_data == NULL) || (this_fp == NULL))
    {
        gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
        return FALSE;
    }
    gNdsStageMPCliffCatchFloorLoopSpecialCollisionCount++;

    if (mpProcessCheckTestCeilCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunCeilCollisionAdjNew(coll_data);
        if ((coll_data->mask_stat & MAP_FLAG_CEIL) != 0u)
        {
            /* Ceil edge adjust remains covered by the bounded ceiling proof. */
        }
    }
    else
    {
        mpProcessSetCollProjectFloorID(coll_data);
    }

    if (((flags & MAP_PROC_TYPE_CLIFF) == 0u) ||
        (this_fp->cliffcatch_wait != 0))
    {
        return FALSE;
    }
    if ((mpProcessCheckTestLCliffCollision(coll_data) == FALSE) &&
        (mpProcessCheckTestRCliffCollision(coll_data) == FALSE))
    {
        return FALSE;
    }

    cliffcatch_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    while (cliffcatch_gobj != NULL)
    {
        if (cliffcatch_gobj != fighter_gobj)
        {
            FTStruct *cliffcatch_fp = ftGetStruct(cliffcatch_gobj);

            if ((cliffcatch_fp != NULL) &&
                (cliffcatch_fp->is_cliff_hold != FALSE) &&
                (coll_data->cliff_id == cliffcatch_fp->coll_data.cliff_id) &&
                (this_fp->lr == cliffcatch_fp->lr))
            {
                gNdsStageMPCliffCatchFloorLoopOccupancyBlockCount++;
                return FALSE;
            }
        }
        cliffcatch_gobj = cliffcatch_gobj->link_next;
    }
    mpCommonSetFighterLandingParams(fighter_gobj);
    coll_data->is_coll_end = TRUE;
    return TRUE;
}

static sb32 ndsStageMPCliffClimbFloorLoopRecatchSpecialCollisions(
    MPCollData *coll_data, GObj *fighter_gobj, u32 flags)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
    GObj *cliffcatch_gobj;
    sb32 l_hit = FALSE;
    sb32 r_hit = FALSE;

    if ((coll_data == NULL) || (this_fp == NULL))
    {
        gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
        return FALSE;
    }
    gNdsStageMPCliffClimbFloorLoopRecatchSpecialCollisionCount++;

    if (mpProcessCheckTestCeilCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunCeilCollisionAdjNew(coll_data);
    }
    else
    {
        mpProcessSetCollProjectFloorID(coll_data);
    }

    if (((flags & MAP_PROC_TYPE_CLIFF) == 0u) ||
        (this_fp->cliffcatch_wait != 0))
    {
        return FALSE;
    }

    gNdsStageMPCliffClimbFloorLoopRecatchLCliffTestCount++;
    l_hit = mpProcessCheckTestLCliffCollision(coll_data);
    if (l_hit != FALSE)
    {
        gNdsStageMPCliffClimbFloorLoopRecatchLCliffHitCount++;
    }
    if (l_hit == FALSE)
    {
        gNdsStageMPCliffClimbFloorLoopRecatchRCliffTestCount++;
        r_hit = mpProcessCheckTestRCliffCollision(coll_data);
        if (r_hit != FALSE)
        {
            gNdsStageMPCliffClimbFloorLoopRecatchRCliffHitCount++;
        }
    }
    if ((l_hit == FALSE) && (r_hit == FALSE))
    {
        return FALSE;
    }

    cliffcatch_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    while (cliffcatch_gobj != NULL)
    {
        if (cliffcatch_gobj != fighter_gobj)
        {
            FTStruct *cliffcatch_fp = ftGetStruct(cliffcatch_gobj);

            if ((cliffcatch_fp != NULL) &&
                (cliffcatch_fp->is_cliff_hold != FALSE) &&
                (coll_data->cliff_id == cliffcatch_fp->coll_data.cliff_id) &&
                (this_fp->lr == cliffcatch_fp->lr))
            {
                gNdsStageMPCliffClimbFloorLoopRecatchOccupancyBlockCount++;
                return FALSE;
            }
        }
        cliffcatch_gobj = cliffcatch_gobj->link_next;
    }

    mpCommonSetFighterLandingParams(fighter_gobj);
    coll_data->is_coll_end = TRUE;
    return TRUE;
}

static sb32 (*sNdsMPCommonProcPass)(GObj *);

static sb32 ndsMPProcessCheckTestFloorCollisionAdjNew(
    MPCollData *coll_data, sb32 (*proc_map)(GObj *), GObj *gobj)
{
    return mpProcessCheckTestFloorCollisionAdjNew(coll_data, proc_map, gobj);
}

static sb32 ndsMPCommonRunFighterCliffFloorCeilCollisions(
    MPCollData *coll_data, GObj *fighter_gobj, u32 flags)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
    GObj *cliffcatch_gobj;
    sb32 is_ceilstop = FALSE;

    if ((coll_data == NULL) || (this_fp == NULL))
    {
        return FALSE;
    }

    /* BUGS.md #1: the wall pair and the ceiling-edge adjust were both missing
     * here, which is the "Mario/Fox special wall, ceiling, and edge adjustment
     * is incomplete" row in KNOWN_ISSUES. This runner stands in for the
     * source's mpCommonRunFighterSpecialCollisions (mpcommon.c:472), which
     * every special/project/pass/landing entry point uses, so the gap took out
     * Up B: without the wall pass coll_data->mask_unk never carries
     * MAP_FLAG_LWALL/RWALL, and that is exactly what
     * mpProcessCheckTestCeilCollisionAdjNew's fallback branches
     * (mpprocess.c:1911-1934) read to catch a rise that enters the platform
     * past the end of the ceiling segment. Mario went straight through. */
    if (mpProcessCheckTestLWallCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunLWallCollisionAdjNew(coll_data);
    }
    if (mpProcessCheckTestRWallCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunRWallCollisionAdjNew(coll_data);
    }
    if (mpProcessCheckTestCeilCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunCeilCollisionAdjNew(coll_data);
        if ((coll_data->mask_stat & MAP_FLAG_CEIL) != 0u)
        {
            mpProcessRunCeilEdgeAdjust(coll_data);
        }
        if (((flags & MAP_PROC_TYPE_CEILHEAVY) != 0u) &&
            (this_fp->physics.vel_air.y >= 30.0F))
        {
            coll_data->mask_curr |= MAP_FLAG_CEILHEAVY;
            is_ceilstop = TRUE;
            coll_data->is_coll_end = TRUE;
        }
    }

    if (ndsMPProcessCheckTestFloorCollisionAdjNew(
            coll_data,
            ((flags & MAP_PROC_TYPE_PASS) != 0u) ?
                sNdsMPCommonProcPass : NULL,
            ((flags & MAP_PROC_TYPE_PASS) != 0u) ? fighter_gobj : NULL) !=
        FALSE)
    {
        if ((flags & MAP_PROC_TYPE_PROJECT) != 0u)
        {
            mpProcessSetCollideFloor(coll_data);
            if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
            {
                mpProcessRunFloorEdgeAdjust(coll_data);
            }
            else
            {
                mpProcessSetCollProjectFloorID(coll_data);
            }
        }
        else
        {
            mpProcessSetLandingFloor(coll_data);
            mpCommonSetFighterLandingParams(fighter_gobj);
            if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
            {
                mpProcessRunFloorEdgeAdjust(coll_data);
                coll_data->is_coll_end = TRUE;
                return TRUE;
            }
        }
    }
    else
    {
        mpProcessSetCollProjectFloorID(coll_data);
    }

    if (((flags & MAP_PROC_TYPE_CLIFF) == 0u) ||
        (this_fp->cliffcatch_wait != 0))
    {
        return is_ceilstop;
    }
    if ((mpProcessCheckTestLCliffCollision(coll_data) == FALSE) &&
        (mpProcessCheckTestRCliffCollision(coll_data) == FALSE))
    {
        return is_ceilstop;
    }

    cliffcatch_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    while (cliffcatch_gobj != NULL)
    {
        if (cliffcatch_gobj != fighter_gobj)
        {
            FTStruct *cliffcatch_fp = ftGetStruct(cliffcatch_gobj);

            if ((cliffcatch_fp != NULL) &&
                (cliffcatch_fp->is_cliff_hold != FALSE) &&
                (this_fp->coll_data.cliff_id ==
                 cliffcatch_fp->coll_data.cliff_id) &&
                (this_fp->lr == cliffcatch_fp->lr))
            {
                return is_ceilstop;
            }
        }
        cliffcatch_gobj = cliffcatch_gobj->link_next;
    }
    mpCommonSetFighterLandingParams(fighter_gobj);
    coll_data->is_coll_end = TRUE;
    return TRUE;
}

void mpCommonProcFighterCliffFloorCeil(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    gNdsFighterBattlePlayableMapCallCount++;
    if ((fp != NULL) &&
        (mpProcessUpdateMain(&fp->coll_data,
                             ndsMPCommonRunFighterCliffFloorCeilCollisions,
                             fighter_gobj,
                             MAP_PROC_TYPE_CEILHEAVY | MAP_PROC_TYPE_CLIFF) !=
         FALSE))
    {
        gNdsFighterBattlePlayableMapHitCount++;
        gNdsFighterBattlePlayableMapLastMaskStat =
            (u32)fp->coll_data.mask_stat;
        gNdsFighterBattlePlayableMapLastMaskCurr =
            (u32)fp->coll_data.mask_curr;
        if ((fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK) != 0u)
        {
            gNdsFighterBattlePlayableMapCliffHitCount++;
            ftCommonCliffCatchSetStatus(fighter_gobj);
        }
        else if ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u)
        {
            gNdsFighterBattlePlayableMapFloorHitCount++;
            if (fp->physics.vel_air.y >
                FTCOMMON_ATTACKAIR_SKIPLANDING_VEL_Y_MAX)
            {
                ftCommonWaitSetStatus(fighter_gobj);
            }
            else
            {
                ftCommonLandingSetStatus(fighter_gobj);
            }
        }
        else if ((fp->coll_data.mask_curr & MAP_FLAG_CEILHEAVY) != 0u)
        {
            gNdsFighterBattlePlayableMapCeilHitCount++;
            ftCommonStopCeilSetStatus(fighter_gobj);
        }
    }
    return;
#endif

    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopRecatchMapActive != FALSE))
    {
        gNdsStageMPCliffClimbFloorLoopRecatchMapCallbackCount++;
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
            return;
        }

        gNdsStageMPCliffClimbFloorLoopRecatchCheckCeilHeavyCliffCount++;
        {
            MPCollData mp_coll;

            if (ndsStageMPUpdateFloorLoopBuildCollData(fp, &mp_coll) == FALSE)
            {
                gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
                return;
            }
            if (mpProcessUpdateMain(&mp_coll,
                ndsStageMPCliffClimbFloorLoopRecatchSpecialCollisions,
                fighter_gobj,
                MAP_PROC_TYPE_CEILHEAVY | MAP_PROC_TYPE_CLIFF) != FALSE)
            {
                ndsStageMPProcessFloorLoopCopyBack(fp, &mp_coll);
                if ((fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK) != 0u)
                {
                    gNdsStageMPCliffClimbFloorLoopRecatchCliffCatchSetStatusCount++;
                    sNdsStageMPCliffClimbFloorLoopRecatchSetStatusActive =
                        TRUE;
                    ftCommonCliffCatchSetStatus(fighter_gobj);
                    sNdsStageMPCliffClimbFloorLoopRecatchSetStatusActive =
                        FALSE;
                }
                else
                {
                    gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
                }
            }
            else
            {
                ndsStageMPProcessFloorLoopCopyBack(fp, &mp_coll);
                gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopMapActive != FALSE))
    {
        gNdsStageMPCliffCatchFloorLoopMapCallbackCount++;
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
            return;
        }

        gNdsStageMPCliffCatchFloorLoopCheckCeilHeavyCliffCount++;
        {
            MPCollData mp_coll;

            if (ndsStageMPUpdateFloorLoopBuildCollData(fp, &mp_coll) == FALSE)
            {
                gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
                return;
            }
            if (mpProcessUpdateMain(&mp_coll,
                ndsStageMPCliffCatchFloorLoopSpecialCollisions,
                fighter_gobj,
                MAP_PROC_TYPE_CEILHEAVY | MAP_PROC_TYPE_CLIFF) != FALSE)
            {
                ndsStageMPProcessFloorLoopCopyBack(fp, &mp_coll);
                if ((fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK) != 0u)
                {
                    gNdsStageMPCliffCatchFloorLoopCliffCatchSetStatusCount++;
                    sNdsStageMPCliffCatchFloorLoopSetStatusActive = TRUE;
                    ftCommonCliffCatchSetStatus(fighter_gobj);
                    sNdsStageMPCliffCatchFloorLoopSetStatusActive = FALSE;
                }
                else
                {
                    gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
                }
            }
            else
            {
                ndsStageMPProcessFloorLoopCopyBack(fp, &mp_coll);
                if ((sNdsStageMPCliffCatchFloorLoopOccupancyActive == FALSE) ||
                    (gNdsStageMPCliffCatchFloorLoopOccupancyBlockCount == 0u))
                {
                    gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
                }
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPCeilStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCeilStatusFloorLoopMapActive != FALSE))
    {
        gNdsStageMPCeilStatusFloorLoopMapCallbackCount++;
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCeilStatusFloorLoopUnsafeCount++;
            return;
        }

        gNdsStageMPCeilStatusFloorLoopCheckCeilHeavyCliffCount++;
        {
            MPCollData mp_coll;

            if (ndsStageMPUpdateFloorLoopBuildCollData(fp, &mp_coll) == FALSE)
            {
                gNdsStageMPCeilStatusFloorLoopUnsafeCount++;
                return;
            }
            if (mpProcessUpdateMain(&mp_coll,
                ndsStageMPCeilStatusFloorLoopSpecialCollisions,
                fighter_gobj,
                MAP_PROC_TYPE_CEILHEAVY | MAP_PROC_TYPE_CLIFF) != FALSE)
            {
                ndsStageMPProcessFloorLoopCopyBack(fp, &mp_coll);
                if ((fp->coll_data.mask_curr & MAP_FLAG_CEILHEAVY) != 0u)
                {
                    gNdsStageMPCeilStatusFloorLoopStopCeilSetStatusCount++;
                    sNdsStageMPCeilStatusFloorLoopSetStatusActive = TRUE;
                    ftCommonStopCeilSetStatus(fighter_gobj);
                    sNdsStageMPCeilStatusFloorLoopSetStatusActive = FALSE;
                }
                else
                {
                    gNdsStageMPCeilStatusFloorLoopUnsafeCount++;
                }
            }
            else
            {
                ndsStageMPProcessFloorLoopCopyBack(fp, &mp_coll);
                gNdsStageMPCeilStatusFloorLoopUnsafeCount++;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallLandFloorLoopMapActive != FALSE))
    {
        DObj *root = (fp != NULL) ? fp->joints[nFTPartsJointTopN] : NULL;
        f32 floor_y = 0.0F;

        gNdsStageMPFallLandFloorLoopMapCallbackCount++;
        if ((fp == NULL) || (root == NULL) ||
            (fp->coll_data.floor_line_id < 0) ||
            (ndsStageFloorEdgeLoopFloorYAtX(fp->coll_data.floor_line_id,
                                            root->translate.vec.f.x,
                                            &floor_y) == FALSE))
        {
            gNdsStageMPFallLandFloorLoopUnsafeCount++;
            return;
        }
        if ((root->translate.vec.f.y <= floor_y) &&
            (fp->physics.vel_air.y <= 0.0F) &&
            (fp->ga == nMPKineticsAir))
        {
            MPCollData coll;

            memset(&coll, 0, sizeof(coll));
            coll.p_translate = &root->translate.vec.f;
            coll.p_lr = (fp->coll_data.p_lr != NULL) ? fp->coll_data.p_lr :
                &fp->lr;
            coll.pos_prev = fp->coll_data.pos_prev;
            coll.map_coll = fp->coll_data.map_coll;
            coll.p_map_coll = &coll.map_coll;
            coll.cliffcatch_coll = fp->coll_data.cliffcatch_coll;
            coll.mask_curr = (u16)fp->coll_data.mask_curr;
            coll.mask_stat = (u16)fp->coll_data.mask_stat;
            coll.update_tic = (u16)fp->coll_data.update_tic;
            coll.ewall_line_id = -1;
            coll.is_coll_end = fp->coll_data.is_coll_end;
            coll.floor_line_id = fp->coll_data.floor_line_id;
            coll.floor_dist = floor_y;
            coll.floor_flags = fp->coll_data.floor_flags;
            coll.floor_angle = fp->coll_data.floor_angle;
            coll.ceil_line_id = -1;
            coll.lwall_line_id = -1;
            coll.rwall_line_id = -1;
            coll.cliff_id = -1;
            coll.ignore_line_id = fp->coll_data.ignore_line_id;
            coll.mask_stat &= (u16)~MAP_FLAG_FLOOR;
            coll.mask_curr &= (u16)~MAP_FLAG_FLOOR;
            fp->coll_data.p_translate = &root->translate.vec.f;
            if (fp->coll_data.p_map_coll == NULL)
            {
                fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
            }
            fp->coll_data.floor_dist = floor_y;
            fp->coll_data.mask_stat &= (u16)~MAP_FLAG_FLOOR;
            fp->coll_data.mask_curr &= (u16)~MAP_FLAG_FLOOR;
            gNdsStageMPFallLandFloorLoopMapFloorCollisionCount++;
            mpProcessSetLandingFloor(&coll);
            fp->coll_data.mask_curr = coll.mask_curr;
            fp->coll_data.mask_stat = coll.mask_stat;
            fp->coll_data.is_coll_end = coll.is_coll_end;
            fp->coll_data.floor_line_id = coll.floor_line_id;
            fp->coll_data.floor_dist = coll.floor_dist;
            fp->coll_data.floor_flags = coll.floor_flags;
            fp->coll_data.floor_angle = coll.floor_angle;
            if ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) == 0u)
            {
                gNdsStageMPFallLandFloorLoopUnsafeCount++;
                return;
            }
            fp->coll_data.mask_stat &= (u16)~MAP_FLAG_FLOOREDGE;
            fp->coll_data.is_coll_end = FALSE;
            fp->is_fastfall = FALSE;
            gNdsStageMPFallLandFloorLoopWaitOrLandingCount++;
            sNdsStageMPFallLandFloorLoopSetStatusActive = TRUE;
            ftCommonLandingSetStatus(fighter_gobj);
            sNdsStageMPFallLandFloorLoopSetStatusActive = FALSE;
        }
        else
        {
            gNdsStageMPFallLandFloorLoopUnsafeCount++;
        }
        return;
    }
    if ((ndsFighterMarioFoxStageMPFallMapFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallMapFloorLoopMapActive != FALSE))
    {
        DObj *root = (fp != NULL) ? fp->joints[nFTPartsJointTopN] : NULL;
        f32 floor_y = 0.0F;

        gNdsStageMPFallMapFloorLoopMapCallbackCount++;
        if ((fp == NULL) || (root == NULL))
        {
            gNdsStageMPFallMapFloorLoopUnsafeCount++;
            return;
        }
        if ((fp->coll_data.floor_line_id >= 0) &&
            (ndsStageFloorEdgeLoopFloorYAtX(fp->coll_data.floor_line_id,
                                            root->translate.vec.f.x,
                                            &floor_y) != FALSE) &&
            (root->translate.vec.f.y > floor_y) &&
            (fp->ga == nMPKineticsAir))
        {
            gNdsStageMPFallMapFloorLoopMapNoCollisionCount++;
        }
        else
        {
            gNdsStageMPFallMapFloorLoopUnsafeCount++;
        }
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopMapActive != FALSE))
    {
        DObj *root = (fp != NULL) ? fp->joints[nFTPartsJointTopN] : NULL;
        f32 floor_y = (fp != NULL) ? fp->coll_data.floor_dist : 0.0F;

        if ((fp != NULL) && (root != NULL) &&
            (root->translate.vec.f.y <= floor_y) &&
            (fp->physics.vel_air.y <= 0.0F))
        {
            root->translate.vec.f.y = floor_y;
            fp->is_fastfall = FALSE;
            gNdsFighterProcessLoopFallDetectCount++;
            sNdsFighterProcessLoopMapActive = TRUE;
            ftCommonLandingSetStatus(fighter_gobj);
            sNdsFighterProcessLoopMapActive = TRUE;
            gNdsFighterProcessLoopLandingDetectCount++;
        }
        return;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingFallMapActive != FALSE))
    {
        DObj *root = (fp != NULL) ? fp->joints[nFTPartsJointTopN] : NULL;
        f32 floor_y = (fp != NULL) ? fp->coll_data.floor_dist : 0.0F;
        u32 slot = ((fp != NULL) && (fp->player < 2)) ? fp->player : 2u;

        if (slot == 0u)
        {
            gNdsFighterLandingP0FallMapCount++;
        }
        else if (slot == 1u)
        {
            gNdsFighterLandingP1FallMapCount++;
        }
        if ((fp != NULL) && (root != NULL) &&
            (root->translate.vec.f.y <= floor_y) &&
            (fp->physics.vel_air.y <= 0.0F))
        {
            if (slot == 0u)
            {
                gNdsFighterLandingP0VelYBeforeLandingMilli =
                    ndsFloatToMilliSigned(fp->physics.vel_air.y);
            }
            else if (slot == 1u)
            {
                gNdsFighterLandingP1VelYBeforeLandingMilli =
                    ndsFloatToMilliSigned(fp->physics.vel_air.y);
            }
            root->translate.vec.f.y = floor_y;
            fp->is_fastfall = FALSE;
            gNdsFighterLandingFloorDetectCount++;
            gNdsFighterLandingFloorClampCount++;
            sNdsFighterLandingSetStatusActive = TRUE;
            ftCommonLandingSetStatus(fighter_gobj);
            sNdsFighterLandingSetStatusActive = FALSE;
        }
        else
        {
            gNdsFighterLandingAirNoCollisionCount++;
        }
        return;
    }
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpAirMapActive != FALSE))
    {
        gNdsFighterJumpAirMapCallCount++;
        if ((fp != NULL) && (fp->ga != nMPKineticsAir))
        {
            gNdsFighterJumpLandingDeniedCount++;
        }
        return;
    }
}

void mpCommonSetFighterFallOnGroundBreak(GObj *fighter_gobj)
{
    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        (void)mpCommonProcFighterOnFloor(
            fighter_gobj, ftCommonFallSetStatus);
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffLiveLoopCommon2MapActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 10;
        return;
    }
    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopPhysicsMapActive != FALSE))
    {
        gNdsStageTurnLoopMapTickCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandCallbackActive != FALSE))
    {
        gNdsStageMPDownWaitLoopDownStandMapTickCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveCallbackActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveMapTickCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveCallbackActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassiveMapTickCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbCommon2LoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbCommon2LoopMapActive != FALSE))
    {
        gNdsStageMPCliffClimbCommon2LoopGroundBreakCount++;
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopMapActive != FALSE))
    {
        ndsMPCommonCheckFighterOnCliffEdgeBounded(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDashMapActive != FALSE))
    {
        gNdsFighterDashRunDashMapCount++;
        if (ndsMPCommonCheckFighterOnCliffEdgeBounded(fighter_gobj) != FALSE)
        {
            gNdsFighterDashRunFallBreakSafeCount++;
            gNdsFighterDashRunSafeFloorCount++;
        }
        return;
    }
    mpCommonProcFighterOnCliffEdge(fighter_gobj);
}

void mpCommonSetFighterWaitOrFall(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowDeadResultActive != FALSE))
    {
        gNdsStageMPPassiveLoopThrowDeadResultWaitOrFallCount++;
    }
    if ((fp != NULL) && (fp->ga == nMPKineticsGround))
    {
        ftCommonWaitSetStatus(fighter_gobj);
    }
    else
    {
        ftCommonFallSetStatus(fighter_gobj);
    }
}

void mpCommonSetFighterFallOnEdgeBreak(GObj *fighter_gobj)
{
    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        if (mpCommonCheckFighterOnEdge(fighter_gobj) == FALSE)
        {
            ftCommonFallSetStatus(fighter_gobj);
        }
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunEscapeMapActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsFighterDashRunEscapeMapCount++;
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack11MapActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        u32 slot = ((fp != NULL) && (fp->player < 2)) ? fp->player : 2u;

        if (slot < 2u)
        {
            gNdsFighterDashRunAttack11TickMask |=
                1u << ((slot * 4u) + 3u);
        }
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttackDashMapActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        u32 slot = ((fp != NULL) && (fp->player < 2)) ? fp->player : 2u;

        if (slot < 2u)
        {
            gNdsFighterDashRunAttackDashTickMask |= 1u << ((slot * 3u) + 2u);
        }
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopAttackCallbackActive != FALSE))
    {
        gNdsStageMPDownWaitLoopAttackMapTickCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollForwardCallbackActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollForwardMapTickCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollBackCallbackActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollBackMapTickCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandCallbackActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveStandMapTickCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveStandCallbackActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassiveStandMapTickCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffCommon2LoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffCommon2LoopMapActive != FALSE))
    {
        gNdsStageMPCliffCommon2LoopEdgeBreakCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeCommon2LoopMapActive != FALSE))
    {
        gNdsStageMPCliffEscapeCommon2LoopEdgeBreakCount++;
        return;
    }
    mpCommonSetFighterFallOnGroundBreak(fighter_gobj);
}

sb32 mpCommonCheckFighterLanding(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    if (fp != NULL)
    {
        return mpProcessUpdateMain(&fp->coll_data,
                                   ndsMPCommonRunFighterCliffFloorCeilCollisions,
                                   fighter_gobj, MAP_PROC_TYPE_DEFAULT);
    }
#endif

    if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
        (sNdsFighterJumpAttackAirMapLandingActive != FALSE) &&
        (fp != NULL) &&
        (fp->status_id == nFTCommonStatusAttackAirN) &&
        (fp->motion_id == nFTCommonMotionAttackAirN))
    {
        gNdsFighterJumpAttackAirMapLandingMask |= 1u;
        return TRUE;
    }
    return FALSE;
}

sb32 mpCommonProcFighterOnFloor(GObj *fighter_gobj,
                                void (*proc_map)(GObj *))
{
    if (mpCommonCheckFighterOnFloor(fighter_gobj) == FALSE)
    {
        if (proc_map != NULL)
        {
            proc_map(fighter_gobj);
        }
        return FALSE;
    }
    return TRUE;
}

sb32 mpCommonCheckFighterProject(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return FALSE;
    }
    return mpProcessUpdateMain(&fp->coll_data,
                               ndsMPCommonRunFighterCliffFloorCeilCollisions,
                               fighter_gobj, MAP_PROC_TYPE_PROJECT);
}

sb32 mpCommonCheckFighterPass(GObj *fighter_gobj,
                              sb32 (*proc_map)(GObj *))
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return FALSE;
    }
    sNdsMPCommonProcPass = proc_map;
    return mpProcessUpdateMain(&fp->coll_data,
                               ndsMPCommonRunFighterCliffFloorCeilCollisions,
                               fighter_gobj, MAP_PROC_TYPE_PASS);
}

sb32 mpCommonCheckFighterPassCliff(GObj *fighter_gobj,
                                   sb32 (*proc_map)(GObj *))
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return FALSE;
    }
    sNdsMPCommonProcPass = proc_map;
    return mpProcessUpdateMain(&fp->coll_data,
                               ndsMPCommonRunFighterCliffFloorCeilCollisions,
                               fighter_gobj,
                               MAP_PROC_TYPE_PASS | MAP_PROC_TYPE_CLIFF);
}

sb32 mpCommonProcFighterCliff(GObj *fighter_gobj,
                              void (*proc_map)(GObj *))
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if ((fp != NULL) && (mpCommonCheckFighterCliff(fighter_gobj) != FALSE))
    {
        if ((fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK) != 0u)
        {
            ftCommonCliffCatchSetStatus(fighter_gobj);
        }
        else if (proc_map != NULL)
        {
            proc_map(fighter_gobj);
        }
        return TRUE;
    }
    return FALSE;
}

/* P2-3f5. BattleShip mp/mpcommon.c:668 and :684 and :697, ported at the seam
 * that reimplements mpcommon.c rather than forked into the fighter that needed
 * them first.
 *
 * `mpCommonSetFighterWaitOrLanding` did not exist here even though its three
 * lines had been copied inline into `mpCommonProcFighterWaitOrLanding` and
 * `mpCommonProcFighterCliffFloorCeil`; it is a function in the source because
 * the source passes it as a proc_map, which is exactly what
 * `mpCommonProcFighterCliffWaitOrLanding` does with it.
 *
 * `mpCommonCheckFighterCeilHeavyCliff` was DECLARED in include/mp/map.h with no
 * definition anywhere -- a link error waiting for its first caller. Falcon Dive
 * is that caller (`ftCaptainSpecialHiProcMap` takes the ceiling arm when
 * vel_air.y >= 0), and Kirby, Link and Yoshi all reach it too, so it belongs
 * here for all of them. Same collision runner and the same
 * CEILHEAVY|CLIFF process flags `mpCommonProcFighterCliffFloorCeil` above
 * already uses. */
void mpCommonSetFighterWaitOrLanding(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return;
    }
    if (fp->physics.vel_air.y > FTCOMMON_ATTACKAIR_SKIPLANDING_VEL_Y_MAX)
    {
        ftCommonWaitSetStatus(fighter_gobj);
    }
    else
    {
        ftCommonLandingSetStatus(fighter_gobj);
    }
}

void mpCommonProcFighterCliffWaitOrLanding(GObj *fighter_gobj)
{
    mpCommonProcFighterCliff(fighter_gobj, mpCommonSetFighterWaitOrLanding);
}

sb32 mpCommonCheckFighterCeilHeavyCliff(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return FALSE;
    }
    return mpProcessUpdateMain(&fp->coll_data,
                               ndsMPCommonRunFighterCliffFloorCeilCollisions,
                               fighter_gobj,
                               MAP_PROC_TYPE_CEILHEAVY | MAP_PROC_TYPE_CLIFF);
}

/* mpcommon.c:665, the same body without the cliff arm. Yoshi Bomb's start
 * (ftyoshispeciallw.c:60) is the first caller: while still rising it asks for
 * a heavy ceiling only, and only a heavy ceiling (MAP_FLAG_CEILHEAVY) drops it
 * into the falling loop. Same runner as the source's special-collision one. */
sb32 mpCommonCheckFighterCeilHeavy(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return FALSE;
    }
    return mpProcessUpdateMain(&fp->coll_data,
                               ndsMPCommonRunFighterCliffFloorCeilCollisions,
                               fighter_gobj, MAP_PROC_TYPE_CEILHEAVY);
}

f32 ftParamGetStickAngleRads(FTStruct *fp)
{
    if (fp == NULL)
    {
        return 0.0F;
    }
    return atan2f((f32)fp->input.pl.stick_range.y,
                  (f32)ABS(fp->input.pl.stick_range.x));
}

f32 lbCommonMag2D(Vec3f *vec)
{
    if (vec == NULL)
    {
        return 0.0F;
    }
    return sqrtf((vec->x * vec->x) + (vec->y * vec->y));
}

sb32 lbCommonCheckAdjustSim2D(Vec3f *a, Vec3f *b, f32 angle)
{
    f32 similarity;
    f32 orientation;
    f32 magnitude;
    f32 denom;

    if ((a == NULL) || (b == NULL))
    {
        return FALSE;
    }
    denom = lbCommonMag2D(a) + lbCommonMag2D(b);
    if (denom == 0.0F)
    {
        return FALSE;
    }
    similarity = ((b->x * a->x) + (b->y * a->y)) / denom;
    if ((similarity <= 0.0F) &&
        (similarity >= cosf(angle + F_CST_DTOR32(90.0F))))
    {
        orientation = (b->x * a->y) - (b->y * a->x);
        orientation = (orientation < 0.0F) ? -1.0F : 1.0F;
        magnitude = lbCommonMag2D(a) * orientation;
        a->x = -b->y * magnitude;
        a->y = b->x * magnitude;
        return TRUE;
    }
    return FALSE;
}

Vec3f *lbCommonAdd2D(Vec3f *a, Vec3f *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return a;
    }
    a->x += b->x;
    a->y += b->y;
    return a;
}

Vec3f *lbCommonScale2D(Vec3f *vec, f32 factor)
{
    if (vec == NULL)
    {
        return NULL;
    }
    vec->x *= factor;
    vec->y *= factor;
    return vec;
}

Vec3f *lbCommonReflect2D(Vec3f *a, Vec3f *b)
{
    f32 negative_two_dot_product;

    if ((a == NULL) || (b == NULL))
    {
        return a;
    }
    negative_two_dot_product = ((b->x * a->x) + (b->y * a->y)) * -2.0F;
    a->x += b->x * negative_two_dot_product;
    a->y += b->y * negative_two_dot_product;
    return a;
}

/* R2-07 L9. These were `return sinf(angle);` / `return cosf(angle);`, which is
 * both slower and LESS faithful than the source: SSB64 does not call a libm
 * sine at all, it indexes a 1024-entry quarter-turn table (`lb/lbcommon.c`,
 * 0x800C7840). Gameplay reads those exact quantised numbers, so the table is
 * the reference behaviour and libm was the approximation.
 *
 * The port never compiled `lbcommon.c`, so the table had nothing to link
 * against; `src/port/lbcommon_sin_table.c` carries it, extracted verbatim.
 *
 * Cost, measured on the R2-07 L6 census (`artifacts/task37-census/r207-L6-*`):
 * the libm path is ~237 cycles/call, and an over-gate frame makes ~197 calls
 * for ~46,772 cycles of trig -- `__ieee754_rem_pio2f` argument reduction plus
 * `__kernel_sinf`/`__kernel_cosf`, none of which a 4096-step table needs. The
 * table form is one multiply, one float-to-int, three masks and a load.
 *
 * Index is in 4096ths of a turn (651.8986206 = 4096 / 2pi). Bit 0x400 mirrors
 * the quarter into the second one, bit 0x800 negates for the lower half; cosine
 * is the same table read a quarter turn ahead. */
/* Read gSYSinTable, which is already resident, rather than a second table.
 *
 * L9 brought in dLBCommonSinLookup -- lbcommon.c's own 1024-entry f32 quarter
 * wave -- and that cost 4,096 bytes of main RAM. The taskman arena is sized by
 * a boot-time loop that steps DOWN 0x1000 per failed calloc, so 4,096 bytes of
 * new static RAM is exactly one step: the arena shrank by a page, Sudden Death
 * lost the free space it was already running on, and it froze at the banner.
 * docs/BUGS.md carries the measurement (42,992 -> 38,896 free, 26 -> 27 steps).
 *
 * gSYSinTable is sys/sintable.c's 2048-entry u16 half wave, and it is the SAME
 * function at the SAME resolution -- both index by angle * 2048/PI masked to
 * 0xFFF, both take the sign from bit 0x800, and both carry the source's own
 * one-sample stretch (the table spans 0..PI inclusive over 2048 samples, worth
 * about 0.0016 against a true sine; that is SSB64's sine and reproducing it is
 * the point). Only the storage differs: quarter wave in f32 against half wave
 * in Q15.
 *
 * scripts/check-lbcommon-sin-table-dedup.py compares them exhaustively over all
 * 4096 indices, not a sample: they agree to 0.00003070, one Q15 quantum, which
 * is 91x inside the 0.0028 bound E64b/E65 established. The index arithmetic
 * below is unchanged from lbcommon.c:321 and :340 -- including cos adding 90
 * degrees to the ANGLE before the multiply, which is not the same as adding
 * 0x400 to the index after it. */
f32 __attribute__((section(".itcm"))) lbCommonSin(f32 angle)
{
    s32 index = ((s32)(angle * 651.8986206F)) & 0xFFF;
    f32 sin = (f32)gSYSinTable[index & SINTABLE_MASK_ID] * (1.0F / 32768.0F);

    return (index & 0x800) ? -sin : sin;
}

f32 __attribute__((section(".itcm"))) lbCommonCos(f32 angle)
{
    s32 index =
        ((s32)((angle + F_CST_DTOR32(90.0F)) * 651.8986206F)) & 0xFFF;
    f32 cos = (f32)gSYSinTable[index & SINTABLE_MASK_ID] * (1.0F / 32768.0F);

    return (index & 0x800) ? -cos : cos;
}

static void ndsStageMPDownWaitLoopApplyRollTransN(GObj *fighter_gobj,
                                                  FTStruct *fp,
                                                  sb32 is_forward)
{
    DObj *root;
    f32 speed;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        gNdsStageMPDownWaitLoopUnsafeCount++;
        return;
    }

    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPDownWaitLoopUnsafeCount++;
        return;
    }

    speed = (is_forward != FALSE) ?
        NDS_STAGE_MPDOWNWAIT_ROLL_TRANSN_SPEED :
        -NDS_STAGE_MPDOWNWAIT_ROLL_TRANSN_SPEED;

    fp->physics.vel_ground.x = speed;
    fp->physics.vel_ground.y = 0.0F;
    fp->physics.vel_ground.z = 0.0F;
    fp->physics.vel_jostle_x = 0.0F;
    fp->physics.vel_jostle_z = 0.0F;
    fp->coll_data.floor_angle.x = 0.0F;
    fp->coll_data.floor_angle.y = 1.0F;
    fp->coll_data.floor_angle.z = 0.0F;

    ndsFTPhysicsSetGroundVelTransferAirOriginal(fighter_gobj, fp);
    root->translate.vec.f.x += fp->physics.vel_air.x;
    root->translate.vec.f.y += fp->physics.vel_air.y;
    root->translate.vec.f.z += fp->physics.vel_air.z;
    ndsFighterSyncPhysicsToLegacyVel(fp);
}

void ftPhysicsApplyGroundVelTransN(GObj *fighter_gobj)
{
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunEscapePhysicsActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsFighterDashRunEscapePhysicsCount++;
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttackDashPhysicsActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        u32 slot = ((fp != NULL) && (fp->player < 2)) ? fp->player : 2u;

        if (slot < 2u)
        {
            gNdsFighterDashRunAttackDashTickMask |= 1u << ((slot * 3u) + 1u);
        }
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffLiveLoopCommon2PhysicsActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 11;
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollForwardCallbackActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollForwardPhysicsTickCount++;
        ndsStageMPDownWaitLoopApplyRollTransN(fighter_gobj,
            ftGetStruct(fighter_gobj), TRUE);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollBackCallbackActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollBackPhysicsTickCount++;
        ndsStageMPDownWaitLoopApplyRollTransN(fighter_gobj,
            ftGetStruct(fighter_gobj), FALSE);
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandCallbackActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveStandPhysicsTickCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveStandCallbackActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassiveStandPhysicsTickCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffCommon2LoopProofEnabled() != FALSE) &&
        (sNdsStageMPCliffCommon2LoopPhysicsActive != FALSE))
    {
        gNdsStageMPCliffCommon2LoopGroundTransCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbCommon2LoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbCommon2LoopPhysicsActive != FALSE))
    {
        gNdsStageMPCliffClimbCommon2LoopGroundTransCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeCommon2LoopPhysicsActive != FALSE))
    {
        gNdsStageMPCliffEscapeCommon2LoopGroundTransCount++;
    }
    {
        FTStruct *fp = (fighter_gobj != NULL) ?
            ftGetStruct(fighter_gobj) : NULL;
        DObj *root = (fighter_gobj != NULL) ?
            DObjGetStruct(fighter_gobj) : NULL;
        DObj *transn_joint;

        if ((fp == NULL) || (root == NULL) ||
            (fp->joints[nFTPartsJointTransN] == NULL))
        {
            return;
        }
        transn_joint = fp->joints[nFTPartsJointTransN];

        /* BattleShip ftphysics.c:168-184. TransN Z owns forward ground
         * motion; TransN X owns lateral motion after facing/orientation. */
        fp->physics.vel_ground.x =
            (transn_joint->translate.vec.f.z - fp->anim_vel.z) *
            root->scale.vec.f.z;
        fp->physics.vel_ground.z =
            (transn_joint->translate.vec.f.x - fp->anim_vel.x) *
            -fp->lr * root->scale.vec.f.x;

        if ((fp->lr * root->rotate.vec.f.y) < 0.0F)
        {
            fp->physics.vel_ground.x = -fp->physics.vel_ground.x;
            fp->physics.vel_ground.z = -fp->physics.vel_ground.z;
        }
        ftPhysicsSetGroundVelTransferAir(fighter_gobj);
    }
}

void ftPhysicsGetAirVelTransN(FTStruct *fp, f32 *vel_x, f32 *vel_y,
                              f32 *vel_z)
{
    DObj *topn_joint;
    DObj *transn_joint;
    f32 anim_vel_z;
    f32 anim_vel_y;
    f32 transn_cos;
    f32 transn_sin;
    f32 next_x = 0.0F;
    f32 next_y = 0.0F;
    f32 next_z = 0.0F;

    if ((fp != NULL) &&
        (fp->joints[nFTPartsJointTopN] != NULL) &&
        (fp->joints[nFTPartsJointTransN] != NULL))
    {
        topn_joint = fp->joints[nFTPartsJointTopN];
        transn_joint = fp->joints[nFTPartsJointTransN];
        anim_vel_z =
            (transn_joint->translate.vec.f.z - fp->anim_vel.z) *
            fp->lr * topn_joint->scale.vec.f.z;
        anim_vel_y =
            (transn_joint->translate.vec.f.y - fp->anim_vel.y) *
            topn_joint->scale.vec.f.y;
        transn_cos = cosf(transn_joint->rotate.vec.f.z);
        transn_sin = __sinf(transn_joint->rotate.vec.f.z);

        /* BattleShip ftphysics.c:393-413. The original parameter names are
         * Z/Y/X, but ApplyAirVelTransNAll passes vel_air X/Y/Z in that order. */
        next_x = (anim_vel_z * transn_cos) -
            (anim_vel_y * transn_sin);
        next_y = (anim_vel_z * transn_sin) +
            (anim_vel_y * transn_cos);
        next_z =
            (transn_joint->translate.vec.f.x - fp->anim_vel.x) *
            -fp->lr * topn_joint->scale.vec.f.x;
    }
    if (vel_x != NULL)
    {
        *vel_x = next_x;
    }
    if (vel_y != NULL)
    {
        *vel_y = next_y;
    }
    if (vel_z != NULL)
    {
        *vel_z = next_z;
    }
}

void ftPhysicsApplyAirVelTransNAll(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp != NULL)
    {
        ftPhysicsGetAirVelTransN(fp, &fp->physics.vel_air.x,
                                 &fp->physics.vel_air.y,
                                 &fp->physics.vel_air.z);
    }
}

void ftPhysicsApplyAirVelTransNYZ(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp != NULL)
    {
        ftPhysicsGetAirVelTransN(fp, NULL, &fp->physics.vel_air.y,
                                 &fp->physics.vel_air.z);
    }
}

void mpCollisionGetSpeedLineID(s32 line_id, Vec3f *vel)
{
    u32 yakumono_id = 0u;
    DObj *yakumono_dobj;

    if (vel != NULL)
    {
        vel->x = 0.0F;
        vel->y = 0.0F;
        vel->z = 0.0F;
    }
    if ((vel == NULL) || (line_id == -1) || (line_id == -2) ||
        (ndsMPFindLineYakumonoID(line_id, &yakumono_id) == FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        (gMPCollisionSpeeds == NULL) ||
        (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
    {
        if (ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopProofEnabled() !=
            FALSE)
        {
            gNdsStageMPPlatformSpeedFloorLoopUnsafeCount++;
        }
        return;
    }

    yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
    if ((yakumono_dobj == NULL) ||
        (yakumono_dobj->user_data.s >= nMPYakumonoStatusOff))
    {
        if (ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopProofEnabled() !=
            FALSE)
        {
            gNdsStageMPPlatformSpeedFloorLoopUnsafeCount++;
        }
        return;
    }

    *vel = gMPCollisionSpeeds[yakumono_id];
    if (ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPPlatformSpeedFloorLoopGetSpeedCount++;
    }
}

/* The stub builder lives out of line so that `ftGetStruct` does not carry its
 * register frame.
 *
 * Decomp resolves this in one load -- `ftGetStruct` is a macro there
 * (`ftparam.c` callers dereference `user_data.p` directly). The port needs a
 * real function because three pointer provenances have to be distinguished, but
 * the profile says the distinguishing costs far less than the *frame*: over
 * 246.3 calls a frame, `push {r3-r7,lr}` costs 1,957 cyc/frame and
 * `pop {r3-r7,pc}` 5,184 -- 7,141 of the function's 18,336 cyc/frame, 38.9%,
 * spent saving five registers that only this cold tail ever needs. GCC cannot
 * shrink-wrap it away: ARMv5TE Thumb-1 has no conditional execution, so the
 * early returns cannot skip a prologue that the tail requires.
 *
 * Splitting the tail leaves the hot path a leaf that needs two registers, and
 * costs nothing -- the code is the same code, one branch further away, on a path
 * that the whole-match profile never executed once.
 *
 * The statics move with it because they are the stub's storage and nothing else
 * reads them. Callers still receive the same pointer on every path. */
static FTStruct *__attribute__((noinline)) ftGetStructBuildStub(GObj *fighter_gobj);

FTStruct *__attribute__((section(".itcm"))) ftGetStruct(GObj *fighter_gobj)
{
    if ((fighter_gobj != NULL) &&
        (sNdsFTCommonCliffCommon2BridgeStruct != NULL) &&
        (fighter_gobj->user_data.p == sNdsFTCommonCliffCommon2BridgeStruct))
    {
        return sNdsFTCommonCliffCommon2BridgeStruct;
    }

    if ((fighter_gobj != NULL) &&
        (ndsFighterStructIsPoolPointer(fighter_gobj->user_data.p) != FALSE))
    {
        return fighter_gobj->user_data.p;
    }
    if ((fighter_gobj != NULL) &&
        (fighter_gobj->id == nGCCommonKindFighter) &&
        ((uintptr_t)fighter_gobj->user_data.p > 0x1000u))
    {
        return fighter_gobj->user_data.p;
    }

    return ftGetStructBuildStub(fighter_gobj);
}

static FTStruct *__attribute__((noinline)) ftGetStructBuildStub(GObj *fighter_gobj)
{
    static FTStruct stub;
    static DObj top_joint;
    static FTParts top_parts;

    if (fighter_gobj != NULL)
    {
        DObj *stub_joint;
        FTParts *stub_parts;

        bzero(&stub, sizeof(stub));
        stub.player = (u8)fighter_gobj->user_data.s;
        stub_joint = DObjGetStruct(fighter_gobj);
        if (stub_joint == NULL)
        {
            bzero(&top_joint, sizeof(top_joint));
            top_joint.translate.vec.f.x = 0.0F;
            top_joint.translate.vec.f.y = 0.0F;
            top_joint.translate.vec.f.z = 0.0F;
            top_joint.scale.vec.f.x = 1.0F;
            top_joint.scale.vec.f.y = 1.0F;
            top_joint.scale.vec.f.z = 1.0F;
            stub_joint = &top_joint;
        }
        stub.joints[nFTPartsJointTopN] = stub_joint;
        stub_joint->parent_gobj = fighter_gobj;
        if (stub_joint->parent == NULL)
        {
            stub_joint->parent = DOBJ_PARENT_NULL;
        }
        stub_parts = &top_parts;
        bzero(stub_parts, sizeof(*stub_parts));
        stub_joint->user_data.p = stub_parts;
        stub_parts->unk_dobjtrans_0x5 = 1;
        stub_parts->unk_dobjtrans_0x6 = 1;
        stub_parts->unk_dobjtrans_0x7 = 1;
        stub_parts->transform_update_mode = 1;
        stub_parts->gobj = fighter_gobj;
        stub_parts->vec_scale.x = 1.0F;
        stub_parts->vec_scale.y = 1.0F;
        stub_parts->vec_scale.z = 1.0F;
        stub_parts->mtx_translate[0][0] = 1.0F;
        stub_parts->mtx_translate[1][1] = 1.0F;
        stub_parts->mtx_translate[2][2] = 1.0F;
        stub_parts->mtx_translate[3][0] = stub_joint->translate.vec.f.x;
        stub_parts->mtx_translate[3][1] = stub_joint->translate.vec.f.y;
        stub_parts->mtx_translate[3][2] = stub_joint->translate.vec.f.z;
    }
    return &stub;
}

/* BUGS.md #3: this discarded the push vector and only counted the call, so
 * Whispy's wind had no gameplay effect at all. The source is a single
 * assignment (ftparam.c:526-531); its consumer chain is already live here --
 * mpprocess.c:450-452 adds coll_data.vel_push to the translation each step and
 * ftmain.c:1571 clears it every frame, so nothing accumulates. The Pupupu
 * counters stay because the ground-loop proofs assert on them. */
void ftParamSetVelPush(GObj *fighter_gobj, Vec3f *vel)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp != NULL) && (vel != NULL))
    {
        fp->coll_data.vel_push = *vel;
    }
    gNdsPupupuUpdateVelPushCount++;
    gNdsPupupuGroundDeferredMask |= 1u << 2;
}

void lbCommonSetSpriteScissor(s32 xmin, s32 xmax, s32 ymin, s32 ymax)
{
    (void)xmin;
    (void)xmax;
    (void)ymin;
    (void)ymax;
}

#if !NDS_IMPORT_BATTLESHIP_FT_PUBLIC
void ftPublicMakeActor(void)
{
    gNdsSCVSBattleCompatManagerMask |= 1u << 4;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
}
#endif

void ftParamInitGame(void)
{
    extern void ftMainClearGroundElementsAll(void);

    /* BattleShip ftparam.c:2660-2664. The placement half is intentionally
     * deferred to ndsSCVSBattleBeginScenePlacement(), where the DS battle seam
     * reinitializes it after the source has populated the live player table and
     * alongside the particle placeholders. The ground-element clear has no such
     * dependency and must still happen HERE on every battle / Sudden Death
     * entry: ftmain's obstacle/hazard arrays are TU statics, so an arena rewind
     * does not clear their stale GObj pointers for us. */
    ftMainClearGroundElementsAll();
    /* The source creates a fresh fighter set later in scVSBattleStartBattle /
     * StartSuddenDeath. The DS live registry is an adapter-only mirror of that
     * set, so clear it at the same source lifetime boundary before any new
     * ftParamInitPlayerBattleStats calls repopulate slots. This is essential for
     * Sudden Death: only tied slots are recreated, and an uncleared non-tied
     * slot would otherwise retain a GObj from the arena that was just rewound. */
    ndsFighterManagerClearLiveGObjs();
    gNdsSCVSBattleCompatManagerMask |= 1u << 5;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
}

void ftParamInitPlayerBattleStats(s32 player, GObj *fighter_gobj)
{
    s32 i;

    if ((gSCManagerBattleState != NULL) &&
        (player >= 0) &&
        (player < (s32)ARRAY_COUNT(gSCManagerBattleState->players)))
    {
        /* BattleShip ftparam.c:158-183.  This is gameplay state, not merely
         * bookkeeping for the DS bridge: score/falls, the per-opponent KO and
         * damage matrices, combo totals and the stale-move queue are all read
         * later by the original N-player hit/scoring/results code.  The old
         * shim only installed fighter_gobj, which happened to be hard to notice
         * in the two-player demo because the transfer state usually arrived
         * zeroed.  Four-way/rematch play makes that assumption observably
         * wrong, so keep this wrapper byte-for-field equivalent to the source
         * initializer before adding the DS live-fighter registration below. */
        gSCManagerBattleState->players[player].place = 0;
        gSCManagerBattleState->players[player].falls = 0;
        gSCManagerBattleState->players[player].score = 0;
        for (i = 0; i < (s32)ARRAY_COUNT(gSCManagerBattleState->players); i++)
        {
            gSCManagerBattleState->players[player].total_kos_players[i] = 0;
            gSCManagerBattleState->players[player].total_damage_players[i] = 0;
        }
        gSCManagerBattleState->players[player].unk_pblock_0x28 = 0;
        gSCManagerBattleState->players[player].unk_pblock_0x2C = 0;
        gSCManagerBattleState->players[player].total_selfdestructs = 0;
        gSCManagerBattleState->players[player].total_damage_given = 0;
        gSCManagerBattleState->players[player].total_damage_all = 0;
        gSCManagerBattleState->players[player].combo_damage_foe = 0;
        gSCManagerBattleState->players[player].combo_count_foe = 0;
        gSCManagerBattleState->players[player].fighter_gobj = fighter_gobj;
        gSCManagerBattleState->players[player].stale_id = 0;
        for (i = 0;
             i < (s32)ARRAY_COUNT(gSCManagerBattleState->players[player].stale_info);
             i++)
        {
            gSCManagerBattleState->players[player].stale_info[i].attack_id = 0;
            gSCManagerBattleState->players[player].stale_info[i].motion_count = 0;
        }
    }
    ndsFighterManagerRecordCreatedFighter(fighter_gobj, player);
    gNdsSCVSBattleCompatManagerMask |= 1u << 6;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
}

void ftParamSetKey(GObj *fighter_gobj, FTKeyEvent *script)
{
    (void)fighter_gobj;
    (void)script;
}

s32 ftParamGetCostumeCommonID(s32 fkind, s32 color)
{
    /* BattleShip dFTParamCostumeIDs: Mario/Fox royal rows are both
     * {0,1,2,3}, so the source lookup is the identity for every fighter kind
     * this P2-2 build can create. Keep the fkind argument for the exact API and
     * for P2-3, where the remaining rows stop being uniformly interesting. */
    (void)fkind;
    return color;
}

s32 ftParamGetCostumeTeamID(s32 fkind, s32 color)
{
    /* BattleShip ftparam.c:56-62 / :2648-2651. Team colour is NOT the team ID:
     * the model's costume table maps Red/Blue/Green to a fighter-specific
     * costume. Keep the admitted P2 prefix source-exact rather than letting a
     * newly selectable fighter inherit the old identity approximation. */
    static const u8 mario_team[3] = { 0u, 3u, 4u };
    static const u8 fox_team[3] = { 1u, 2u, 3u };
    static const u8 donkey_team[3] = { 2u, 3u, 4u };
    static const u8 luigi_team[3] = { 3u, 2u, 0u };

    if ((color < 0) || (color >= (s32)ARRAY_COUNT(mario_team)))
    {
        return color;
    }
    if (fkind == nFTKindMario)
    {
        return mario_team[color];
    }
    if (fkind == nFTKindFox)
    {
        return fox_team[color];
    }
    if (fkind == nFTKindDonkey)
    {
        return donkey_team[color];
    }
    if (fkind == nFTKindLuigi)
    {
        return luigi_team[color];
    }
    return color;
}

#if !NDS_IMPORT_BATTLESHIP_VS_RESULTS
void scSubsysFighterSetStatus(GObj *fighter_gobj, s32 status_id)
{
    (void)fighter_gobj;
    (void)status_id;
    if (ndsFighterMarioFoxInitProofEnabled() != FALSE)
    {
        gNdsFighterInitStatusSetCount++;
    }
}
#endif

#if !NDS_R2_PARTICLE_RUNTIME
void efParticleInitAll(void)
{
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_EFFECTS;
}

s32 efParticleGetLoadBankID(uintptr_t script_lo, uintptr_t script_hi,
                            uintptr_t texture_lo, uintptr_t texture_hi)
{
    (void)script_lo;
    (void)script_hi;
    (void)texture_lo;
    (void)texture_hi;
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerBattleState != NULL) &&
        (gSCManagerBattleState->gkind == nGRKindPupupu))
    {
        gNdsPupupuGroundDeferredMask |= 1u << 1;
        gNdsPupupuGroundSetupMask |= 1u << 9;
        gNdsPupupuGroundParticleBankID = 1;
        return 1;
    }
    return 0;
}

s32 efParticleGetBankID(uintptr_t scripts_lo)
{
    (void)scripts_lo;
    return 0;
}
#endif /* !NDS_R2_PARTICLE_RUNTIME */

__attribute__((weak)) void ftKirbyCopyLinkSpecialNDestroyBoomerang(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

__attribute__((weak)) void ftKirbyCopyLinkSpecialNGetSetStatus(GObj *fighter_gobj)
{
    /* Kirby is not a production fighter yet. The boomerang source keeps this
     * cross-fighter callback, and the weak seam is replaced by Kirby's real
     * source TU when that roster row lands. No live Link-owned boomerang can
     * reach this arm because its parent fkind is Link. */
    (void)fighter_gobj;
}

__attribute__((weak)) void ftLinkSpecialNDestroyBoomerang(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

void ftBossCommonSetNextAttackWait(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

void ftBossCommonSetDefaultLineID(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

void ftParamSetModelPartDefaultID(GObj *fighter_gobj, s32 joint_id,
                                  s32 modelpart_id)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    s32 slot;

    if ((fp == NULL) || (joint_id < nFTPartsJointCommonStart) ||
        ((u32)joint_id >= ARRAY_COUNT(fp->joints)))
    {
        return;
    }
    slot = joint_id - nFTPartsJointCommonStart;
    if ((u32)slot >= ARRAY_COUNT(fp->modelpart_status))
    {
        return;
    }
    /* BattleShip ftparam.c:821-828 changes only the reset/default id. The live
     * part remains untouched until the normal reset path consumes this value. */
    fp->modelpart_status[slot].modelpart_id_base = (s8)modelpart_id;
    fp->is_modelpart_modify = TRUE;
}

void ftParamLockPlayerControl(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->input.pl.button_hold = fp->input.pl.button_tap =
        fp->input.cp.button_inputs = 0;
    fp->input.pl.stick_range.x = fp->input.pl.stick_range.y =
        fp->input.pl.stick_prev.x = fp->input.pl.stick_prev.y =
        fp->input.cp.stick_range.x = fp->input.cp.stick_range.y = 0;
    fp->tap_stick_x = fp->tap_stick_y =
        fp->hold_stick_x = fp->hold_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->is_control_disable = TRUE;
}

void ftParamUnlockPlayerControl(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->is_control_disable = FALSE;
}

#if !NDS_IMPORT_BATTLESHIP_FTCOMPUTER
void ftComputerSetupAll(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

void ftComputerProcessAll(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

void ftComputerSetFighterDamageDetectSize(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}
#endif

void battleship_ftKeyProcessKeyEvents(GObj *fighter_gobj);

void ftKeyProcessKeyEvents(GObj *fighter_gobj)
{
    battleship_ftKeyProcessKeyEvents(fighter_gobj);
}

void ftParamTryUpdateItemMusic(void)
{
}

#if !NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE
void ftCommonEntrySetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusEntry, 0.0F, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
    if (fp != NULL)
    {
        fp->is_invisible = TRUE;
        fp->is_shadow_hide = TRUE;
        fp->is_ghost = TRUE;
        fp->is_playertag_hide = TRUE;
    }
}
#endif

#if !NDS_IMPORT_BATTLESHIP_VS_RESULTS
void scSubsysFighterProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainPlayAnimEventsAll(fighter_gobj);
    ftMainRunUpdateColAnim(fighter_gobj);
    if ((fp != NULL) && (fp->proc_update != NULL))
    {
        fp->proc_update(fighter_gobj);
    }
}
#endif

GObj *ftShadowMakeShadow(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return NULL;
}

/* Skipped-particle shims. NDS_R2_PARTICLE_RUNTIME=1 compiles the original
 * lb/lbparticle.c in place (src/import/battleship_lbparticle.c), which owns
 * every symbol below. */
#if !NDS_R2_PARTICLE_RUNTIME
LBParticle *lbParticleMakeScriptID(s32 bank_id, s32 script_id)
{
    ndsTask39EffectCensusRecord(
        NDS_TASK39_EFFECT_LB_PARTICLE_MAKE_SCRIPT_I_D,
        NDS_TASK39_EFFECT_SKIPPED);
    (void)bank_id;
    (void)script_id;
    gNdsPupupuUpdateParticleScriptCount++;
    gNdsPupupuGroundDeferredMask |= 1u << 1;
    return NULL;
}

LBTransform *lbParticleAddTransformForStruct(LBParticle *pc, u8 status)
{
    (void)pc;
    (void)status;
    gNdsPupupuGroundDeferredMask |= 1u << 1;
    return NULL;
}

void LBParticleProcessStruct(LBParticle *pc)
{
    (void)pc;
    gNdsPupupuGroundDeferredMask |= 1u << 1;
}

void lbParticleEjectStruct(LBParticle *pc)
{
    (void)pc;
    gNdsPupupuGroundDeferredMask |= 1u << 1;
}

void lbParticleEjectStructID(u16 generator_id, s32 index)
{
    (void)generator_id;
    (void)index;
    gNdsPupupuGroundDeferredMask |= 1u << 1;
}
#endif /* !NDS_R2_PARTICLE_RUNTIME */

/* The source quake is the only driver of camera shake: efManagerQuakeProcUpdate
 * feeds the effect's animated translate into gmCameraSetVelAt, which
 * gmCameraApplyVel adds to the look-at.  Recording the request and returning
 * NULL therefore silenced every shake in the game -- Whispy's wind rumble
 * (grpupupu.c:313), the KO burst, and wall damage.  Run the original whenever
 * the pieces it needs are compiled in: battleship_efmanager.c supplies the
 * effect and battleship_gmcamera.c supplies gGMCameraGObj, which only
 * NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE builds have. */
#if NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE
GObj *ndsBaseEFManagerQuakeMakeEffect(s32 magnitude);
#endif

GObj *efManagerQuakeMakeEffect(s32 id)
{
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopWallDamageActive != FALSE))
    {
        gNdsStageMPPassiveLoopWallDamageQuakeCount++;
    }
    gNdsPupupuUpdateQuakeCount++;
#if NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE
    return ndsBaseEFManagerQuakeMakeEffect(id);
#else
    (void)id;
    gNdsPupupuGroundDeferredMask |= 1u << 2;
    return NULL;
#endif
}

#if !NDS_R2_PARTICLE_RUNTIME
LBGenerator *lbParticleMakeGenerator(s32 bank_id, s32 generator_id)
{
    ndsTask39EffectCensusRecord(
        NDS_TASK39_EFFECT_LB_PARTICLE_MAKE_GENERATOR,
        NDS_TASK39_EFFECT_SKIPPED);
    (void)bank_id;
    (void)generator_id;
    gNdsPupupuGroundDeferredMask |= 1u << 1;
    return NULL;
}

LBParticle *lbParticleMakeCommon(s32 bank_id, s32 script_id)
{
    ndsTask39EffectCensusRecord(
        NDS_TASK39_EFFECT_LB_PARTICLE_MAKE_COMMON,
        NDS_TASK39_EFFECT_SKIPPED);
    (void)bank_id;
    (void)script_id;
    gNdsPupupuGroundDeferredMask |= 1u << 1;
    return NULL;
}

LBParticle *lbParticleMakePosVel(s32 bank_id, s32 script_id, f32 pos_x,
                                 f32 pos_y, f32 pos_z, f32 vel_x,
                                 f32 vel_y, f32 vel_z)
{
    ndsTask39EffectCensusRecord(
        NDS_TASK39_EFFECT_LB_PARTICLE_MAKE_POS_VEL,
        NDS_TASK39_EFFECT_SKIPPED);
    (void)bank_id;
    (void)script_id;
    (void)pos_x;
    (void)pos_y;
    (void)pos_z;
    (void)vel_x;
    (void)vel_y;
    (void)vel_z;
    gNdsPupupuGroundDeferredMask |= 1u << 1;
    return NULL;
}

void lbParticleDrawTextures(GObj *gobj)
{
    (void)gobj;
}
#endif /* !NDS_R2_PARTICLE_RUNTIME */

void mpCollisionInitGroundData(void)
{
    ndsMPCollisionInvalidateTopology();
    gMPCollisionGroundData = NULL;
    ndsMPCollisionSetGeometry(NULL);
    gMPCollisionMapObjs = NULL;
    if (gMPCollisionYakumonoDObjs != NULL)
    {
        memset(gMPCollisionYakumonoDObjs->dobjs, 0,
               sizeof(gMPCollisionYakumonoDObjs->dobjs));
    }
    if (gMPCollisionSpeeds != NULL)
    {
        memset(gMPCollisionSpeeds, 0,
               NDS_MP_YAKUMONO_DOBJ_SLOTS * sizeof(*gMPCollisionSpeeds));
    }
    /* EVERY STAGE, OUT OF ONE TABLE.
     *
     * This was one fifty-line arm per stage, copied five times, and the four
     * stages that never got an arm -- Hyrule Castle, Saffron City, Mushroom
     * Kingdom and Sector Z -- reached their battle with gMPCollisionGroundData
     * still NULL and aborted on the first dereference of it, inside
     * grWallpaperMakeCommon. The owner heard the same four stages die on entry.
     *
     * The arms only ever differed in three values -- the ground kind, the map
     * file, and the header offset within it -- so three values is what they are
     * now, and a stage landing without an entry here is a missing ROW rather
     * than fifty missing lines. */
    {
        /* decomp mp/mpcollision.c:26-67 verbatim, gkind-indexed exactly as
         * mpCollisionInitGroundData indexes it there (nGRKindCastle 0 ..
         * nGRKindBonus2Ness 40): every ground the game has, in one table,
         * so a stage lands by staging its files, never by adding a row. A
         * kind whose file is not staged reads size 0 below and leaves the
         * ground data NULL, the same failure the missing-row case had,
         * but named by the file rather than by a missing arm. The row shape
         * is the source's GRFileInfo (gr/ground.h:122) spelled out, since
         * this TU does not include the ground header. */
        static const struct {
            uintptr_t *file_id;
            uintptr_t *offset;
        } dMPCollisionGroundFileInfos[] = {
            { &llGRCastleMapFileID, &llGRCastleMapMapHeader }, /*  0 nGRKindCastle */
            { &llGRSectorMapFileID, &llGRSectorMapMapHeader }, /*  1 nGRKindSector */
            { &llGRJungleMapFileID, &llGRJungleMapMapHeader }, /*  2 nGRKindJungle */
            { &llGRZebesMapFileID, &llGRZebesMapMapHeader }, /*  3 nGRKindZebes */
            { &llGRHyruleMapFileID, &llGRHyruleMapMapHeader }, /*  4 nGRKindHyrule */
            { &llGRYosterMapFileID, &llGRYosterMapMapHeader }, /*  5 nGRKindYoster */
            { &llGRPupupuMapFileID, &llGRPupupuMapMapHeader }, /*  6 nGRKindPupupu */
            { &llGRYamabukiMapFileID, &llGRYamabukiMapMapHeader }, /*  7 nGRKindYamabuki */
            { &llGRInishieMapFileID, &llGRInishieMapMapHeader }, /*  8 nGRKindInishie */
            { &llGRPupupuSmallMapFileID, &llGRPupupuSmallMapMapHeader }, /*  9 nGRKindPupupuSmall */
            { &llGRPupupuTestMapFileID, &llGRPupupuTestMapMapHeader }, /* 10 nGRKindPupupuNew */
            { &llGRExplainMapFileID, &llGRExplainMapMapHeader }, /* 11 nGRKindExplain */
            { &llGRYosterSmallMapFileID, &llGRYosterSmallMapMapHeader }, /* 12 nGRKindYosterSmall */
            { &llGRMetalMapFileID, &llGRMetalMapMapHeader }, /* 13 nGRKindMetal */
            { &llGRZakoMapFileID, &llGRZakoMapMapHeader }, /* 14 nGRKindZako */
            { &llGRBonus3MapFileID, &llGRBonus3MapMapHeader }, /* 15 nGRKindBonus3 */
            { &llGRLastMapFileID, &llGRLastMapMapHeader }, /* 16 nGRKindLast */
            { &llGRBonus1MarioMapFileID, &llGRBonus1MarioMapMapHeader }, /* 17 nGRKindBonus1Mario */
            { &llGRBonus1FoxMapFileID, &llGRBonus1FoxMapMapHeader }, /* 18 nGRKindBonus1Fox */
            { &llGRBonus1DonkeyMapFileID, &llGRBonus1DonkeyMapMapHeader }, /* 19 nGRKindBonus1Donkey */
            { &llGRBonus1SamusMapFileID, &llGRBonus1SamusMapMapHeader }, /* 20 nGRKindBonus1Samus */
            { &llGRBonus1LuigiMapFileID, &llGRBonus1LuigiMapMapHeader }, /* 21 nGRKindBonus1Luigi */
            { &llGRBonus1LinkMapFileID, &llGRBonus1LinkMapMapHeader }, /* 22 nGRKindBonus1Link */
            { &llGRBonus1YoshiMapFileID, &llGRBonus1YoshiMapMapHeader }, /* 23 nGRKindBonus1Yoshi */
            { &llGRBonus1CaptainMapFileID, &llGRBonus1CaptainMapMapHeader }, /* 24 nGRKindBonus1Captain */
            { &llGRBonus1KirbyMapFileID, &llGRBonus1KirbyMapMapHeader }, /* 25 nGRKindBonus1Kirby */
            { &llGRBonus1PikachuMapFileID, &llGRBonus1PikachuMapMapHeader }, /* 26 nGRKindBonus1Pikachu */
            { &llGRBonus1PurinMapFileID, &llGRBonus1PurinMapMapHeader }, /* 27 nGRKindBonus1Purin */
            { &llGRBonus1NessMapFileID, &llGRBonus1NessMapMapHeader }, /* 28 nGRKindBonus1Ness */
            { &llGRBonus2MarioMapFileID, &llGRBonus2MarioMapMapHeader }, /* 29 nGRKindBonus2Mario */
            { &llGRBonus2FoxMapFileID, &llGRBonus2FoxMapMapHeader }, /* 30 nGRKindBonus2Fox */
            { &llGRBonus2DonkeyMapFileID, &llGRBonus2DonkeyMapMapHeader }, /* 31 nGRKindBonus2Donkey */
            { &llGRBonus2SamusMapFileID, &llGRBonus2SamusMapMapHeader }, /* 32 nGRKindBonus2Samus */
            { &llGRBonus2LuigiMapFileID, &llGRBonus2LuigiMapMapHeader }, /* 33 nGRKindBonus2Luigi */
            { &llGRBonus2LinkMapFileID, &llGRBonus2LinkMapMapHeader }, /* 34 nGRKindBonus2Link */
            { &llGRBonus2YoshiMapFileID, &llGRBonus2YoshiMapMapHeader }, /* 35 nGRKindBonus2Yoshi */
            { &llGRBonus2CaptainMapFileID, &llGRBonus2CaptainMapMapHeader }, /* 36 nGRKindBonus2Captain */
            { &llGRBonus2KirbyMapFileID, &llGRBonus2KirbyMapMapHeader }, /* 37 nGRKindBonus2Kirby */
            { &llGRBonus2PikachuMapFileID, &llGRBonus2PikachuMapMapHeader }, /* 38 nGRKindBonus2Pikachu */
            { &llGRBonus2PurinMapFileID, &llGRBonus2PurinMapMapHeader }, /* 39 nGRKindBonus2Purin */
            { &llGRBonus2NessMapFileID, &llGRBonus2NessMapMapHeader }, /* 40 nGRKindBonus2Ness */
        };
        u32 i;

        for (i = 0u; i < ARRAY_COUNT(dMPCollisionGroundFileInfos); i++)
        {
            void *file;
            MPGroundData *ground_data;
            size_t bytes;

            if ((gSCManagerBattleState == NULL) ||
                (gSCManagerBattleState->gkind != (s32)i))
            {
                continue;
            }
            bytes = lbRelocGetFileSize(dMPCollisionGroundFileInfos[i].file_id);
            if (bytes == 0u)
            {
                break;
            }
            file = lbRelocGetExternHeapFile(dMPCollisionGroundFileInfos[i].file_id,
                                            syTaskmanMalloc(bytes, 0x10));
            ground_data = lbRelocGetFileData(MPGroundData*, file,
                                             dMPCollisionGroundFileInfos[i].offset);
            if (ground_data == NULL)
            {
                break;
            }
            gMPCollisionGroundData = ground_data;
            ndsMPCollisionSetGeometry(ground_data->map_geometry);
            gMPCollisionMapObjs = (gMPCollisionGeometry != NULL) ?
                gMPCollisionGeometry->mapobjs : NULL;
            gMPCollisionLightAngleX = ground_data->light_angle.x;
            gMPCollisionLightAngleY = ground_data->light_angle.y;
            gMPCollisionBGMDefault = ground_data->bgm_id;
            gMPCollisionBGMCurrent = ground_data->bgm_id;

            gNdsSCVSBattleStageResult = NDS_STAGE_PUPUPU_BATTLE_PASS;
            gNdsSCVSBattleStageGKind = (s32)i;
            gNdsSCVSBattleStageGroundDataReady = 1;
            /* Blob-resident stage packets load here, beside the ground they
             * draw over: Dream Land's packet stays linked and skips the
             * load, every other kind reads its relocatable blob into the
             * scene arena now, while load time is cheap. A missing or
             * corrupt blob leaves the blob packet NULL and the stage
             * declines through the existing unresolved-kind path. */
            if (i != NDS_NATIVE_STAGE_BLOB_GKIND_DREAMLAND)
            {
                ndsNativeStageBlobLoad(i);
            }
            gNdsSCVSBattleStageMask |= (1u << 0);
            gNdsSCVSBattleStageMask |= (1u << 1);
            gNdsSCVSBattleStageMask |= (1u << 2);

            if (ground_data->map_geometry != NULL)
            {
                gNdsSCVSBattleStageGeometryReady = 1;
                gNdsSCVSBattleStageMask |= (1u << 3);
            }
            if ((ground_data->map_nodes != NULL) ||
                ((ground_data->map_geometry != NULL) &&
                 (ground_data->map_geometry->mapobjs != NULL)))
            {
                gNdsSCVSBattleStageMapNodesReady = 1;
                gNdsSCVSBattleStageMask |= (1u << 4);
            }
            gNdsSCVSBattleStageLightAngleXBits =
                ndsFloatBits(ground_data->light_angle.x);
            gNdsSCVSBattleStageLightAngleYBits =
                ndsFloatBits(ground_data->light_angle.y);
            gNdsSCVSBattleStageMask |= (1u << 5);

            gNdsSCVSBattleStageBGM = ground_data->bgm_id;
            gNdsSCVSBattleStageMask |= (1u << 6);
            gNdsSCVSBattleStageDeferredMask |= (1u << 0);
            gNdsSCVSBattleStageDeferredMask |= (1u << 1);
            gNdsSCVSBattleStageMask |= (1u << 7);
            break;
        }
    }

    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_GROUND_COLLISION;
}

#if NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE
extern GObj *gGMCameraGObj;

static GObj *ndsBattleCompatMainCameraGObj(void)
{
    return gGMCameraGObj;
}
#else
void gmCameraSetViewportDimensions(s32 ulx, s32 uly, s32 lrx, s32 lry)
{
    (void)ulx;
    (void)uly;
    (void)lrx;
    (void)lry;
}

GObj *gmCameraMakeWallpaperCamera(void)
{
    return NULL;
}

static GObj *ndsMakeBattleCompatCamera(u32 bit)
{
    gNdsSCVSBattleCompatCameraMask |= bit;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_GAMEPLAY_CAMERA;

    return gcMakeCameraGObj(nGCCommonKindSceneCamera,
                            NULL,
                            nGCCommonLinkIDCamera,
                            GOBJ_PRIORITY_DEFAULT,
                            func_80017EC0,
                            10,
                            ~0,
                            ~0,
                            FALSE,
                            nGCProcessKindFunc,
                            NULL,
                            1,
                            FALSE);
}

static GObj *sNdsBattleCompatMainCameraGObj;

static GObj *ndsBattleCompatMainCameraGObj(void)
{
    return sNdsBattleCompatMainCameraGObj;
}

GObj *gmCameraMakeBattleCamera(void)
{
    GObj *camera_gobj = ndsMakeBattleCompatCamera(1u << 0);
    CObj *cobj;
    XObj *xobj;

    if (camera_gobj == NULL)
    {
        return NULL;
    }

    cobj = CObjGetStruct(camera_gobj);
    if (cobj == NULL)
    {
        return camera_gobj;
    }

    xobj = gcAddXObjForCamera(cobj, 0x4C, 0);
    (void)xobj;
    cobj->projection.persp.xobj = NULL;
    cobj->projection.persp.norm = 0;
    cobj->projection.persp.fovy = 38.0F;
    cobj->projection.persp.aspect = 15.0F / 11.0F;
    cobj->projection.persp.near = 256.0F;
    cobj->projection.persp.far = 39936.0F;
    cobj->projection.persp.scale = 1.0F;
    cobj->vec.xobj = NULL;
    cobj->vec.eye.x = 0.0F;
    cobj->vec.eye.y = 300.0F;
    cobj->vec.eye.z = 10000.0F;
    cobj->vec.at.x = 0.0F;
    cobj->vec.at.y = 300.0F;
    cobj->vec.at.z = 0.0F;
    cobj->vec.up.x = 0.0F;
    cobj->vec.up.y = 1.0F;
    cobj->vec.up.z = 0.0F;

    sNdsBattleCompatMainCameraGObj = camera_gobj;

    return camera_gobj;
}

GObj *gmCameraMakePlayerArrowsCamera(void)
{
    return ndsMakeBattleCompatCamera(1u << 1);
}

GObj *gmCameraMakePlayerMagnifyCamera(void)
{
    return ndsMakeBattleCompatCamera(1u << 2);
}

GObj *gmCameraScreenFlashMakeCamera(void)
{
    return ndsMakeBattleCompatCamera(1u << 3);
}

GObj *gmCameraMakeInterfaceCamera(void)
{
    return ndsMakeBattleCompatCamera(1u << 4);
}

GObj *gmCameraMakeEffectCamera(void)
{
    return ndsMakeBattleCompatCamera(1u << 5);
}

GObj *gmCameraMakeMovieCamera(void (*func_camera)(GObj *))
{
    (void)func_camera;
    return gcMakeCameraGObj(nGCCommonKindSceneCamera,
                            NULL,
                            16,
                            GOBJ_PRIORITY_DEFAULT,
                            func_80017EC0,
                            10,
                            ~0,
                            ~0,
                            FALSE,
                            nGCProcessKindFunc,
                            NULL,
                            1,
                            FALSE);
}
#endif

void ndsGRCompatibilityNonPupupuSetup(void)
{
    if ((gSCManagerBattleState != NULL) &&
        (gSCManagerBattleState->gkind == nGRKindPupupu) &&
        (gNdsSCVSBattleStageGroundDataReady != 0u))
    {
        gNdsSCVSBattleStageMask |= (1u << 7);
        gNdsPupupuGroundDeferredMask |= 1u << 0;
    }
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_GROUND_COLLISION;
}

void mpCollisionClearYakumonoAll(void)
{
    s32 count;
    s32 i;

    if ((gMPCollisionYakumonoDObjs == NULL) || (gMPCollisionSpeeds == NULL))
    {
        gNdsPupupuGroundDeferredMask |= 1u << 5;
        return;
    }

    count = gMPCollisionYakumonosNum;
    if (count > NDS_MP_YAKUMONO_DOBJ_SLOTS)
    {
        count = NDS_MP_YAKUMONO_DOBJ_SLOTS;
    }
    for (i = 0; i < count; i++)
    {
        if (gMPCollisionYakumonoDObjs->dobjs[i] != NULL)
        {
            gMPCollisionYakumonoDObjs->dobjs[i]->user_data.s =
                nMPYakumonoStatusNone;
        }
        gMPCollisionSpeeds[i].x = 0.0F;
        gMPCollisionSpeeds[i].y = 0.0F;
        gMPCollisionSpeeds[i].z = 0.0F;
    }
    gMPCollisionUpdateTic = 0;
}

static void ndsMPBoundsInitEmpty(MPBounds *bounds)
{
    bounds->top = -65536.0F;
    bounds->right = -65536.0F;
    bounds->bottom = 65536.0F;
    bounds->left = 65536.0F;
}

static void ndsMPBoundsInclude(MPBounds *bounds, f32 vx, f32 vy,
                               sb32 current_style)
{
    if (current_style != FALSE)
    {
        if (bounds->top < vy)
        {
            bounds->top = vy;
        }
        else if (bounds->bottom > vy)
        {
            bounds->bottom = vy;
        }
        if (bounds->right < vx)
        {
            bounds->right = vx;
        }
        else if (bounds->left > vx)
        {
            bounds->left = vx;
        }
        return;
    }

    if (bounds->top < vy)
    {
        bounds->top = vy;
    }
    if (bounds->bottom > vy)
    {
        bounds->bottom = vy;
    }
    if (bounds->right < vx)
    {
        bounds->right = vx;
    }
    if (bounds->left > vx)
    {
        bounds->left = vx;
    }
}

static void ndsMPBoundsIncludeYakumonoLines(
    NDSMPO2RHalfwordView line_info, DObj *dobj, MPBounds *bounds,
    sb32 use_translate, sb32 current_style)
{
    MPVertexLinks *links = gMPCollisionGeometry->vertex_links;
    MPVertexArray *ids = gMPCollisionGeometry->vertex_id;
    MPVertexPosContainer *verts = gMPCollisionGeometry->vertex_data;
    f32 tx = (use_translate != FALSE) ? dobj->translate.vec.f.x : 0.0F;
    f32 ty = (use_translate != FALSE) ? dobj->translate.vec.f.y : 0.0F;
    u32 kind;

    for (kind = 0u; kind < nMPLineKindEnumCount; kind++)
    {
        u32 line_id = ndsMPLineInfoGroupID(line_info, kind);
        u32 line_count = ndsMPLineInfoLineCount(line_info, kind);
        u32 i;

        if (line_count > 4096u)
        {
            line_count = 4096u;
        }
        for (i = 0u; i < line_count; i++, line_id++)
        {
            u32 first_vertex = ndsMPVertexLinkFirst(links, line_id);
            u32 vertex_count = ndsMPVertexLinkCount(links, line_id);
            u32 vertex_index;

            if ((vertex_count == 0u) || (vertex_count > 128u))
            {
                continue;
            }
            for (vertex_index = first_vertex;
                 vertex_index < (first_vertex + vertex_count);
                 vertex_index++)
            {
                u32 vertex_id = ndsMPVertexID(ids, vertex_index);
                f32 vx = (f32)ndsMPVertexX(verts, vertex_id) + tx;
                f32 vy = (f32)ndsMPVertexY(verts, vertex_id) + ty;

                ndsMPBoundsInclude(bounds, vx, vy, current_style);
            }
        }
    }
}

void mpCollisionInitYakumonoAll(void)
{
    MPBounds bounds_moved;
    MPBounds bounds_static;
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;

    if ((ndsStageCollisionLoopGeometryReady() == FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL))
    {
        gNdsPupupuGroundDeferredMask |= 1u << 5;
        return;
    }

    ndsMPBoundsInitEmpty(&bounds_moved);
    ndsMPBoundsInitEmpty(&bounds_static);
    line_info = gMPCollisionGeometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(gMPCollisionGeometry);
    if (yakumono_count > NDS_MP_YAKUMONO_DOBJ_SLOTS)
    {
        yakumono_count = NDS_MP_YAKUMONO_DOBJ_SLOTS;
    }

    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);
        u32 yakumono_id = ndsMPLineInfoYakumonoID(info);
        DObj *yakumono_dobj;

        if (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS)
        {
            continue;
        }
        yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
        if ((yakumono_dobj == NULL) ||
            (yakumono_dobj->user_data.s == nMPYakumonoStatusOn) ||
            (yakumono_dobj->user_data.s == nMPYakumonoStatusOff))
        {
            continue;
        }
        if (yakumono_dobj->anim_joint.event32 == NULL)
        {
            ndsMPBoundsIncludeYakumonoLines(info, yakumono_dobj,
                                            &bounds_static, FALSE, FALSE);
        }
        ndsMPBoundsIncludeYakumonoLines(info, yakumono_dobj, &bounds_moved,
                                        (yakumono_dobj->anim_joint.event32 !=
                                         NULL) ? TRUE : FALSE,
                                        FALSE);
    }

    gMPCollisionBounds.start = bounds_moved;
    gMPCollisionBounds.stop = bounds_static;
    gMPCollisionBounds.current = bounds_moved;
    gMPCollisionBounds.diff.top = 0.0F;
    gMPCollisionBounds.diff.bottom = 0.0F;
    gMPCollisionBounds.diff.right = 0.0F;
    gMPCollisionBounds.diff.left = 0.0F;
}

void mpCollisionUpdateBoundsCurrent(void)
{
    MPBounds bounds;
    MPLineInfo *line_info;
    u32 yakumono_count;
    u32 i;

    if ((ndsStageCollisionLoopGeometryReady() == FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL))
    {
        gNdsPupupuGroundDeferredMask |= 1u << 5;
        return;
    }

    bounds = gMPCollisionBounds.stop;
    line_info = gMPCollisionGeometry->line_info;
    yakumono_count = ndsMPGeometryYakumonoCount(gMPCollisionGeometry);
    if (yakumono_count > NDS_MP_YAKUMONO_DOBJ_SLOTS)
    {
        yakumono_count = NDS_MP_YAKUMONO_DOBJ_SLOTS;
    }

    for (i = 0u; i < yakumono_count; i++)
    {
        NDSMPO2RHalfwordView info = ndsMPLineInfoAt(line_info, i);
        u32 yakumono_id = ndsMPLineInfoYakumonoID(info);
        DObj *yakumono_dobj;

        if (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS)
        {
            continue;
        }
        yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
        if ((yakumono_dobj == NULL) ||
            ((yakumono_dobj->anim_joint.event32 == NULL) &&
             ((yakumono_dobj->user_data.s >= nMPYakumonoStatusOff) ||
              (yakumono_dobj->user_data.s == nMPYakumonoStatusNone))))
        {
            continue;
        }
        ndsMPBoundsIncludeYakumonoLines(info, yakumono_dobj, &bounds, TRUE,
                                        TRUE);
    }
    gMPCollisionBounds.current = bounds;
}

void mpCollisionUpdateBoundsDiff(void)
{
    gMPCollisionBounds.diff.top =
        gMPCollisionBounds.current.top - gMPCollisionBounds.start.top;
    gMPCollisionBounds.diff.bottom =
        gMPCollisionBounds.current.bottom - gMPCollisionBounds.start.bottom;
    gMPCollisionBounds.diff.right =
        gMPCollisionBounds.current.right - gMPCollisionBounds.start.right;
    gMPCollisionBounds.diff.left =
        gMPCollisionBounds.current.left - gMPCollisionBounds.start.left;
}

void mpCollisionAdvanceUpdateTic(GObj *ground_gobj)
{
    (void)ground_gobj;
    if ((ndsFighterMarioFoxStageMPPlatformTickFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPPlatformTickFloorLoopAdvanceActive != FALSE))
    {
        gNdsStageMPPlatformTickFloorLoopAdvanceCount++;
        gMPCollisionUpdateTic++;
        return;
    }
    gNdsPupupuGroundDeferredMask |= 1u << 5;
}

extern void gcParseDObjAnimJoint(DObj *dobj);
extern void gcPlayDObjAnimJoint(DObj *dobj);
extern void gcParseMObjMatAnimJoint(MObj *mobj);
extern void gcPlayMObjMatAnim(MObj *mobj);
extern u16 gMPCollisionUpdateTic;

static s32 ndsMPFindYakumonoDObjIndex(DObj *dobj)
{
    s32 count;
    s32 i;

    if ((dobj == NULL) || (gMPCollisionYakumonoDObjs == NULL))
    {
        return -1;
    }
    count = gMPCollisionYakumonosNum;
    if (count > NDS_MP_YAKUMONO_DOBJ_SLOTS)
    {
        count = NDS_MP_YAKUMONO_DOBJ_SLOTS;
    }
    for (i = 0; i < count; i++)
    {
        if (gMPCollisionYakumonoDObjs->dobjs[i] == dobj)
        {
            return i;
        }
    }
    return -1;
}

s32 mpCollisionSetDObjNoID(s32 line_id)
{
    u32 yakumono_id = 0u;

    if ((line_id < 0) || (ndsMPFindLineYakumonoID(line_id, &yakumono_id) ==
            FALSE))
    {
        return -1;
    }
    return (s32)yakumono_id;
}

void mpCollisionPlayYakumonoAnim(GObj *ground_gobj)
{
    DObj *dobj;

    /* THE PROOF PREDICATE WAS DISABLING THE WHOLE FUNCTION.
     *
     * NDS_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP_HARNESS is defined
     * nowhere -- one `#if` in reloc_backend_fighter_model.c:1060 and no
     * Makefile row, no -D, no config header -- so the predicate is FALSE in
     * every configuration ever built. At its four other call sites that only
     * costs a counter: the work around them (`*vel = gMPCollisionSpeeds[..]`,
     * mpCollisionUpdateBoundsCurrent) runs regardless. Here it gated a
     * `return`, so this function has never run outside a harness that does
     * not exist.
     *
     * What it does is not diagnostic. It walks the ground's DObj tree,
     * parses and plays every joint animation and material animation, and
     * derives each yakumono platform's speed from its translation delta --
     * which is what makes a moving platform move and carry a fighter. Dream
     * Land never showed the loss because its animation comes from its own
     * native stage path; every opt-in stage relies on this one, which is why
     * the owner's 2026-09-04 playtest reports Congo Jungle's platforms
     * standing still (docs/BUGS.md).
     *
     * The null guards below are real and stay, and so does the deferral
     * witness for them. Only the predicate leaves the condition. */
    if ((ground_gobj == NULL) || (gMPCollisionYakumonoDObjs == NULL) ||
        (gMPCollisionSpeeds == NULL))
    {
        gNdsPupupuGroundDeferredMask |= 1u << 5;
        return;
    }
    if (ground_gobj == gGRCommonLayerGObjs[1])
    {
        gNdsStageMPPlatformSpeedFloorLoopStageAnimCallbackCount++;
    }

    dobj = DObjGetStruct(ground_gobj);
    while (dobj != NULL)
    {
        MObj *mobj;
        s32 yakumono_id = ndsMPFindYakumonoDObjIndex(dobj);

        if (yakumono_id >= 0)
        {
            if ((dobj->user_data.s != nMPYakumonoStatusOn) &&
                (dobj->user_data.s != nMPYakumonoStatusOff))
            {
                u8 flags = dobj->flags;
                Vec3f translate;

                gcParseDObjAnimJoint(dobj);
                translate = dobj->translate.vec.f;
                gcPlayDObjAnimJoint(dobj);

                gMPCollisionSpeeds[yakumono_id].x =
                    dobj->translate.vec.f.x - translate.x;
                gMPCollisionSpeeds[yakumono_id].y =
                    dobj->translate.vec.f.y - translate.y;
                gMPCollisionSpeeds[yakumono_id].z =
                    dobj->translate.vec.f.z - translate.z;

                if (flags == DOBJ_FLAG_NONE)
                {
                    if (dobj->flags != DOBJ_FLAG_NONE)
                    {
                        dobj->user_data.s = nMPYakumonoStatusHidden;
                    }
                }
                else if (dobj->flags == DOBJ_FLAG_NONE)
                {
                    dobj->user_data.s = nMPYakumonoStatusShow;
                }

                if ((u32)yakumono_id ==
                    gNdsStageMPPlatformSpeedFloorLoopYakumonoID)
                {
                    gNdsStageMPPlatformSpeedFloorLoopAnimPlayCount++;
                    gNdsStageMPPlatformSpeedFloorLoopAnimSpeedXMilli =
                        ndsFloatToMilliSigned(
                            gMPCollisionSpeeds[yakumono_id].x);
                    gNdsStageMPPlatformSpeedFloorLoopAnimSpeedYMilli =
                        ndsFloatToMilliSigned(
                            gMPCollisionSpeeds[yakumono_id].y);
                    gNdsStageMPPlatformSpeedFloorLoopAnimSpeedZMilli =
                        ndsFloatToMilliSigned(
                            gMPCollisionSpeeds[yakumono_id].z);
                    gNdsStageMPPlatformSpeedFloorLoopAnimStatusAfter =
                        (u32)dobj->user_data.s;
                }
            }
        }
        else
        {
            gcParseDObjAnimJoint(dobj);
            gcPlayDObjAnimJoint(dobj);
        }

        mobj = dobj->mobj;
        while (mobj != NULL)
        {
            gcParseMObjMatAnimJoint(mobj);
            gcPlayMObjMatAnim(mobj);
            mobj = mobj->next;
        }

        if (dobj->child != NULL)
        {
            dobj = dobj->child;
        }
        else if (dobj->sib_next != NULL)
        {
            dobj = dobj->sib_next;
        }
        else
        {
            while (TRUE)
            {
                if (dobj->parent == DOBJ_PARENT_NULL)
                {
                    dobj = NULL;
                    break;
                }
                if (dobj->parent->sib_next != NULL)
                {
                    dobj = dobj->parent->sib_next;
                    break;
                }
                dobj = dobj->parent;
            }
        }
    }

    mpCollisionUpdateBoundsCurrent();
    mpCollisionUpdateBoundsDiff();
    if (ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopProofEnabled() != FALSE)
    {
        gNdsStageMPPlatformSpeedFloorLoopBoundsUpdateCount++;
        gNdsStageMPPlatformSpeedFloorLoopBoundsDiffTopMilli =
            ndsFloatToMilliSigned(gMPCollisionBounds.diff.top);
        gNdsStageMPPlatformSpeedFloorLoopBoundsDiffBottomMilli =
            ndsFloatToMilliSigned(gMPCollisionBounds.diff.bottom);
        gNdsStageMPPlatformSpeedFloorLoopBoundsDiffRightMilli =
            ndsFloatToMilliSigned(gMPCollisionBounds.diff.right);
        gNdsStageMPPlatformSpeedFloorLoopBoundsDiffLeftMilli =
            ndsFloatToMilliSigned(gMPCollisionBounds.diff.left);
    }
    gMPCollisionUpdateTic++;
}

void mpCollisionSetYakumonoPosID(s32 line_id, Vec3f *yakumono_pos)
{
    DObj *yakumono_dobj;

    if ((line_id < 0) || (line_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS) ||
        (yakumono_pos == NULL) || (gMPCollisionYakumonoDObjs == NULL) ||
        (gMPCollisionSpeeds == NULL) ||
        (gMPCollisionYakumonoDObjs->dobjs[line_id] == NULL))
    {
        if ((ndsFighterMarioFoxStageMPPlatformPosFloorLoopProofEnabled() !=
             FALSE) &&
            (sNdsStageInishieScaleSourceSetupActive == FALSE) &&
            (sNdsStageInishieScaleLoopActive == FALSE))
        {
            gNdsStageMPPlatformPosFloorLoopUnsafeCount++;
        }
        if ((ndsFighterMarioFoxStageInishieScaleLoopProofEnabled() != FALSE) &&
            (sNdsStageInishieScaleLoopActive != FALSE))
        {
            gNdsStageInishieScaleLoopUnsafeCount++;
        }
        return;
    }

    yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[line_id];
    gMPCollisionSpeeds[line_id].x =
        yakumono_pos->x - yakumono_dobj->translate.vec.f.x;
    gMPCollisionSpeeds[line_id].y =
        yakumono_pos->y - yakumono_dobj->translate.vec.f.y;
    gMPCollisionSpeeds[line_id].z =
        yakumono_pos->z - yakumono_dobj->translate.vec.f.z;
    yakumono_dobj->translate.vec.f.x = yakumono_pos->x;
    yakumono_dobj->translate.vec.f.y = yakumono_pos->y;
    yakumono_dobj->translate.vec.f.z = yakumono_pos->z;

    if ((ndsFighterMarioFoxStageMPPlatformPosFloorLoopProofEnabled() !=
         FALSE) &&
        (sNdsStageInishieScaleSourceSetupActive == FALSE) &&
        (sNdsStageInishieScaleLoopActive == FALSE))
    {
        gNdsStageMPPlatformPosFloorLoopSetPosCount++;
    }
    if ((ndsFighterMarioFoxStageInishieScaleLoopProofEnabled() != FALSE) &&
        (sNdsStageInishieScaleLoopActive != FALSE))
    {
        if (sNdsStageInishieScaleLoopPhase == 1u)
        {
            gNdsStageInishieScaleLoopFallSetPosCount++;
            if (line_id == gNdsStageInishieScaleLoopLeftLineID)
            {
                gNdsStageInishieScaleLoopFallLeftSpeedYMilli =
                    ndsFloatToMilliSigned(gMPCollisionSpeeds[line_id].y);
            }
            else if (line_id == gNdsStageInishieScaleLoopRightLineID)
            {
                gNdsStageInishieScaleLoopFallRightSpeedYMilli =
                    ndsFloatToMilliSigned(gMPCollisionSpeeds[line_id].y);
            }
        }
        else
        {
            if (sNdsStageInishieScaleLoopPhase == 2u)
            {
                gNdsStageInishieScaleLoopStepSetPosCount++;
                if (line_id == gNdsStageInishieScaleLoopLeftLineID)
                {
                    gNdsStageInishieScaleLoopStepSetPosMask |= 1u << 0;
                    gNdsStageInishieScaleLoopStepLeftSpeedYMilli =
                        ndsFloatToMilliSigned(gMPCollisionSpeeds[line_id].y);
                }
                else if (line_id == gNdsStageInishieScaleLoopRightLineID)
                {
                    gNdsStageInishieScaleLoopStepSetPosMask |= 1u << 1;
                    gNdsStageInishieScaleLoopStepRightSpeedYMilli =
                        ndsFloatToMilliSigned(gMPCollisionSpeeds[line_id].y);
                }
            }
            else
            {
                gNdsStageInishieScaleLoopSetPosCount++;
                if (line_id == gNdsStageInishieScaleLoopLeftLineID)
                {
                    gNdsStageInishieScaleLoopSetPosMask |= 1u << 0;
                    gNdsStageInishieScaleLoopLeftSpeedYMilli =
                        ndsFloatToMilliSigned(gMPCollisionSpeeds[line_id].y);
                }
                else if (line_id == gNdsStageInishieScaleLoopRightLineID)
                {
                    gNdsStageInishieScaleLoopSetPosMask |= 1u << 1;
                    gNdsStageInishieScaleLoopRightSpeedYMilli =
                        ndsFloatToMilliSigned(gMPCollisionSpeeds[line_id].y);
                }
            }
        }
    }
}

void mpCollisionSetYakumonoOnID(s32 line_id)
{
    if ((line_id < 0) || (line_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        (gMPCollisionYakumonoDObjs->dobjs[line_id] == NULL))
    {
        if ((ndsFighterMarioFoxStageMPPlatformTickFloorLoopProofEnabled() !=
             FALSE) &&
            (sNdsStageInishieScaleSourceSetupActive == FALSE) &&
            (sNdsStageInishieScaleLoopActive == FALSE))
        {
            gNdsStageMPPlatformTickFloorLoopUnsafeCount++;
        }
        if ((ndsFighterMarioFoxStageInishieScaleLoopProofEnabled() != FALSE) &&
            (sNdsStageInishieScaleLoopActive != FALSE))
        {
            gNdsStageInishieScaleLoopUnsafeCount++;
        }
        return;
    }
    gMPCollisionYakumonoDObjs->dobjs[line_id]->user_data.s =
        nMPYakumonoStatusOn;
    if ((ndsFighterMarioFoxStageMPPlatformTickFloorLoopProofEnabled() !=
         FALSE) &&
        (sNdsStageInishieScaleSourceSetupActive == FALSE) &&
        (sNdsStageInishieScaleLoopActive == FALSE))
    {
        gNdsStageMPPlatformTickFloorLoopSetOnCount++;
    }
    if ((ndsFighterMarioFoxStageInishieScaleLoopProofEnabled() != FALSE) &&
        (sNdsStageInishieScaleLoopActive != FALSE))
    {
        if (sNdsStageInishieScaleLoopPhase == 2u)
        {
            gNdsStageInishieScaleLoopStepSetOnCount++;
            if (line_id == gNdsStageInishieScaleLoopLeftLineID)
            {
                gNdsStageInishieScaleLoopStepSetOnMask |= 1u << 0;
            }
            else if (line_id == gNdsStageInishieScaleLoopRightLineID)
            {
                gNdsStageInishieScaleLoopStepSetOnMask |= 1u << 1;
            }
        }
        else
        {
            gNdsStageInishieScaleLoopSetOnCount++;
            if (line_id == gNdsStageInishieScaleLoopLeftLineID)
            {
                gNdsStageInishieScaleLoopSetOnMask |= 1u << 0;
            }
            else if (line_id == gNdsStageInishieScaleLoopRightLineID)
            {
                gNdsStageInishieScaleLoopSetOnMask |= 1u << 1;
            }
        }
    }
}

void mpCollisionSetYakumonoOffID(s32 line_id)
{
    if ((line_id < 0) || (line_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        (gMPCollisionYakumonoDObjs->dobjs[line_id] == NULL))
    {
        return;
    }
    gMPCollisionYakumonoDObjs->dobjs[line_id]->user_data.s =
        nMPYakumonoStatusOff;
}

/* P2-5: with the item core compiled in, itManagerMakeAppearActor is the real
 * spawn-law actor in battleship_item_link_core.c and returns the actor GObj,
 * as the source does. Without it, grCommonSetupInitAll still calls the name
 * (grcommonsetup.c:33), so the witness stub stays for exactly that case. */
#if !NDS_P2_ITEM_CORE
GObj *itManagerMakeAppearActor(void)
{
    gNdsPupupuGroundDeferredMask |= 1u << 6;
    return NULL;
}
#endif

void efGroundMakeAppearActor(void)
{
    gNdsPupupuGroundDeferredMask |= 1u << 7;
}

s32 mpCollisionGetMapObjCountKind(s32 kind)
{
    u32 i;
    u32 count = 0u;
    u32 mapobj_count;

#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
    if (sNdsStageInishieScaleSourceSetupActive != FALSE)
    {
        return ((kind == nMPMapObjKindScaleL) ||
                (kind == nMPMapObjKindScaleR)) ? 1 : 0;
    }
#endif

    mapobj_count = ndsMPGeometryMapObjCount(gMPCollisionGeometry);
    for (i = 0; i < mapobj_count; i++)
    {
        u16 mapobj_kind;

        if ((ndsMPReadMapObj((s32)i, &mapobj_kind, NULL, NULL) != FALSE) &&
            ((s32)mapobj_kind == kind))
        {
            count++;
        }
    }
    return (s32)count;
}

void mpCollisionGetMapObjIDsKind(s32 kind, s32 *ids)
{
    u32 i;
    u32 count = 0u;
    u32 mapobj_count;

    if (ids != NULL)
    {
#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
        if (sNdsStageInishieScaleSourceSetupActive != FALSE)
        {
            if (kind == nMPMapObjKindScaleL)
            {
                ids[0] = 0;
            }
            else if (kind == nMPMapObjKindScaleR)
            {
                ids[0] = 1;
            }
            else
            {
                ids[0] = -1;
            }
            return;
        }
#endif

        mapobj_count = ndsMPGeometryMapObjCount(gMPCollisionGeometry);
        for (i = 0; i < mapobj_count; i++)
        {
            u16 mapobj_kind;

            if ((ndsMPReadMapObj((s32)i, &mapobj_kind, NULL, NULL) != FALSE) &&
                ((s32)mapobj_kind == kind))
            {
                ids[count] = (s32)i;
                count++;
            }
        }
    }
}

void mpCollisionGetMapObjPositionID(s32 id, Vec3f *pos)
{
    if (pos != NULL)
    {
#if NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP
        if (sNdsStageInishieScaleSourceSetupActive != FALSE)
        {
            if (id == 0)
            {
                pos->x = -417.0F;
                pos->y = 363.0F;
                pos->z = 0.0F;
                return;
            }
            if (id == 1)
            {
                pos->x = 420.0F;
                pos->y = 362.0F;
                pos->z = 0.0F;
                return;
            }
        }
#endif

        {
            s16 x;
            s16 y;

            if (ndsMPReadMapObj(id, NULL, &x, &y) != FALSE)
            {
                pos->x = (f32)x;
                pos->y = (f32)y;
                pos->z = 0.0F;
                return;
            }
        }

        pos->x = 0.0F;
        pos->y = 0.0F;
        pos->z = 0.0F;
    }
}

void mpCollisionGetPlayerMapObjPosition(s32 player, Vec3f *pos)
{
    if (pos != NULL)
    {
        u32 i;
        u32 mapobj_count = ndsMPGeometryMapObjCount(gMPCollisionGeometry);

        /* BattleShip mpcollision.c:3405. The source zeros the output only when
         * the stage has NO map objects. If a malformed/non-VS stage has map
         * objects but no spawn kind matching this player, it leaves the caller's
         * existing FTDesc position untouched. Preserve that distinction instead
         * of silently moving an unmatched P3/P4 spawn to the world origin. */
        if (mapobj_count == 0u)
        {
            pos->x = 0.0F;
            pos->y = 0.0F;
            pos->z = 0.0F;
        }
        for (i = 0; i < mapobj_count; i++)
        {
            u16 kind;
            s16 x;
            s16 y;

            if ((ndsMPReadMapObj((s32)i, &kind, &x, &y) != FALSE) &&
                ((s32)kind == player))
            {
                pos->x = (f32)x;
                pos->y = (f32)y;
                pos->z = 0.0F;
                break;
            }
        }
    }
    gNdsSCVSBattleCompatSpawnMask |= 1u << (player & 3);
}

sb32 mpCollisionCheckProjectFloor(Vec3f *pos, s32 *floor_line_id,
                                  f32 *floor_dist, u32 *floor_flags,
                                  Vec3f *floor_angle)
{
    sb32 is_floor = FALSE;

#if (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP)
    if (ndsFighterMarioFoxStageCollisionLoopProofEnabled() != FALSE)
    {
        sb32 hit = FALSE;

        if (ndsStageCollisionLoopCheckProjectFloor(pos, floor_line_id,
                floor_dist, floor_flags, floor_angle, &hit) != FALSE)
        {
            return hit;
        }
        if (gNdsStageCollisionLoopPrepared != 0u)
        {
            gNdsStageCollisionLoopLegacyFlatFallbackCount++;
            gNdsStageCollisionLoopUnsafeFallbackAfterPrepareCount++;
        }
    }
#endif

    if (
        /* ANY stage whose geometry loaded, which is what the call below
         * already checks. This was a gkind whitelist -- Dream Land, later Dream
         * Land or Yoshi's Island -- and it is the second half of the same hole
         * as the line-group gate: a stage off the list got no floor query even
         * once it had line groups, so its fighters found nothing under them.
         * The y=0 flat fabrication further down stays Dream Land-only on
         * purpose; it is a fallback for one layout and no other stage may
         * inherit it. */
        (ndsStageCollisionLoopGeometryReady() != FALSE))
    {
        Vec3f angle = { 0.0F, 1.0F, 0.0F };
        s32 line_id = -1;
        f32 dist = 0.0F;
        u32 flags = 0u;

        is_floor = ndsMPProjectFloorGeometry(pos, &line_id, &dist,
                                             &flags, &angle);
        if (floor_line_id != NULL)
        {
            *floor_line_id = (is_floor != FALSE) ? line_id : -1;
        }
        if (floor_dist != NULL)
        {
            *floor_dist = (is_floor != FALSE) ? dist : 0.0F;
        }
        if (floor_flags != NULL)
        {
            *floor_flags = (is_floor != FALSE) ? flags : 0u;
        }
        if (floor_angle != NULL)
        {
            *floor_angle = angle;
        }
        return is_floor;
    }

    if ((gSCManagerSceneData.gkind == nGRKindPupupu) &&
        (pos != NULL) &&
        (pos->y > -1.0F) &&
        (pos->y < 1.0F))
    {
        is_floor = TRUE;
    }

    if (floor_line_id != NULL)
    {
        *floor_line_id = (is_floor != FALSE) ? 0 : -1;
    }
    if (floor_dist != NULL)
    {
        *floor_dist = 0.0F;
    }
    if (floor_flags != NULL)
    {
        *floor_flags = 0u;
    }
    if (floor_angle != NULL)
    {
        floor_angle->x = 0.0F;
        floor_angle->y = 1.0F;
        floor_angle->z = 0.0F;
    }
    return is_floor;
}

void mpCollisionSetPlayBGM(void)
{
    if (gMPCollisionGroundData != NULL)
    {
        gMPCollisionBGMDefault = gMPCollisionGroundData->bgm_id;
        gMPCollisionBGMCurrent = gMPCollisionGroundData->bgm_id;
        gNdsSCVSBattleStageBGM = gMPCollisionGroundData->bgm_id;
        gNdsStagePupupuBGM = gMPCollisionGroundData->bgm_id;
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
        syAudioPlayBGM(0, gMPCollisionBGMDefault);
#endif
    }
    gNdsSCVSBattleCompatAudioMask |= 1u << 4;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_AUDIO;
}

void gmRumbleMakeActor(void)
{
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_RUMBLE;
}

void gmRumbleInitPlayers(void)
{
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_RUMBLE;
}

__attribute__((weak)) void itManagerInitItems(void)
{
    gNdsSCVSBattleCompatManagerMask |= 1u << 8;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_ITEM_WEAPON_MANAGER;
}

void syDebugSetFuncPrint(void (*function)(void))
{
    (void)function;
}

void syDebugStartRmonThread5Hang(void)
{
}

void scManagerFuncPrint(void)
{
}

void syRdpSetViewport(Vp *viewport, f32 ulx, f32 uly, f32 lrx, f32 lry)
{
    f32 h;
    f32 v;

    if (viewport == NULL)
    {
        return;
    }

    h = (ulx + lrx) / 2.0F;
    v = (uly + lry) / 2.0F;

    viewport->vp.vscale[0] = (s16)(((s32)((lrx - h) * 4.0F)) & 0xFFFF);
    viewport->vp.vscale[1] = (s16)(((s32)((lry - v) * 4.0F)) & 0xFFFF);
    viewport->vp.vtrans[0] = (s16)(((s32)(h * 4.0F)) & 0xFFFF);
    viewport->vp.vtrans[1] = (s16)(((s32)(v * 4.0F)) & 0xFFFF);
    viewport->vp.vscale[2] = (s16)(0x03FF / 2);
    viewport->vp.vtrans[2] = (s16)(0x03FF / 2);
}

void (*dSYRdpFuncLights)(Gfx **);

void syRdpSetFuncLights(void (*func_lights)(Gfx **))
{
    dSYRdpFuncLights = func_lights;
    ndsFighterDisplayContractResetSceneLight();
}

void syRdpResetSettings(Gfx **dl)
{
    /* BattleShip sys/rdp.c:112-115 applies the scene light callback before
     * scene drawing. The DS backend consumes its GBI light state directly. */
    if ((dl != NULL) && (dSYRdpFuncLights != NULL))
    {
        dSYRdpFuncLights(dl);
    }
}
