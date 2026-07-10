#include <ef/effect.h>
#include <it/item.h>
#include <nds/nds_gbi_decode.h>
#include <nds/nds_fighter_display.h>
#include <nds/nds_scene_harness.h>
#include "nds_scene_harness_config.h"
#include <sys/utils.h>
#include <wp/weapon.h>

extern void syUtilsSetRandomSeed(s32 seed);
extern s32 syUtilsRandSeed(void);

static u32 ndsFloatBits(f32 value);
static f32 ndsFloatFromBits(u32 value);
static s32 ndsFloatToMilliSigned(f32 value);
static f32 ndsVectorAngleDiff3D(const Vec3f *a, const Vec3f *b);
static void ndsFTPhysicsSetGroundVelTransferAirOriginal(GObj *fighter_gobj,
                                                        FTStruct *fp);
static void ndsFighterSyncPhysicsToLegacyVel(FTStruct *fp);
static s32 ndsFTCommonDamageGetDamageLevel(f32 hitstun);
static s32 ndsFTCommonDamageSelectStatus(s32 damage_level, s32 damage_index,
                                         sb32 is_air);
static s32 ndsFTCommonDamageMotionForStatus(s32 status_id);
static sb32 ndsFTCommonDamageIsStatus(s32 status_id);
static sb32 ndsFTMainSetStatusDamageHarness(GObj *fighter_gobj,
                                            s32 status_id, f32 frame_begin,
                                            f32 anim_speed, u32 flags);
typedef struct NDSFighterScriptInput NDSFighterScriptInput;
static sb32 ndsFighterMarioFoxModelProofEnabled(void);
static sb32 ndsFighterMarioFoxStructProofEnabled(void);
static sb32 ndsFighterMarioFoxInitProofEnabled(void);
static sb32 ndsFighterMarioFoxWaitProofEnabled(void);
static sb32 ndsFighterMarioFoxWaitTickProofEnabled(void);
static sb32 ndsFighterMarioFoxWaitGroundProofEnabled(void);
static sb32 ndsFighterMarioFoxDisplayProofEnabled(void);
static sb32 ndsFighterMarioFoxDLScanProofEnabled(void);
static sb32 ndsFighterMarioFoxDLExecuteProofEnabled(void);
static sb32 ndsFighterMarioFoxDLDrawProofEnabled(void);
static sb32 ndsFighterMarioFoxDLMultiDrawProofEnabled(void);
static sb32 ndsFighterMarioFoxDLAllDrawProofEnabled(void);
static sb32 ndsFighterMarioFoxWalkInputProofEnabled(void);
static sb32 ndsFighterMarioFoxWalkLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxDashRunProofEnabled(void);
static sb32 ndsFighterMarioFoxJumpLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxJumpAttackAirProofEnabled(void);
static sb32 ndsFighterMarioFoxLandingLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxProcessLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxSchedulerLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxControllerLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxPreviewLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxGCRunAllLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxGCDrawAllLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageGCDrawAllLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageCollisionLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageFloorFollowLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageFloorEdgeLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPProcessFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPUpdateFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPSweepFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCrossFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPAdjustFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPEdgeFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPWallFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPWallHitFloorLoopProofEnabled(void);
static void ndsFighterMarioFoxStageMPWallHitFloorLoopFinalize(void);
static sb32 ndsFighterMarioFoxStageMPWallCopyFloorLoopProofEnabled(void);
void ndsFighterMarioFoxStageMPCliffLiveLoopFinalize(void);
static sb32 ndsFighterMarioFoxStageMPPassFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPPlatformFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPPlatformActiveFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPPlatformTickFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPPassInputLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPPlatformPosFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageInishieScaleLoopProofEnabled(void);
extern void gcDrawDObjTreeForGObj(GObj *gobj);
extern void gcDrawDObjDLHead0(GObj *gobj);
extern const u32 *ndsBattleShipMarioFoxMainMotionScript(s32 fkind,
                                                        s32 motion_id);
extern void ndsBaseFTCommonGuardOnProcUpdate(GObj *fighter_gobj);
extern void ndsBaseFTCommonGuardProcUpdate(GObj *fighter_gobj);
extern void ndsBaseFTCommonGuardSetStatus(GObj *fighter_gobj);
extern void ndsBaseFTCommonGuardOffProcUpdate(GObj *fighter_gobj);
extern void ndsBaseFTCommonGuardSetOffProcUpdate(GObj *fighter_gobj);
extern void ndsBaseFTCommonGuardSetOffSetStatus(GObj *fighter_gobj);
extern void ndsBaseFTCommonGuardCommonProcInterrupt(GObj *fighter_gobj);
extern sb32 ndsBaseFTCommonGuardOnCheckInterruptCommon(GObj *fighter_gobj);
extern sb32 ndsBaseFTCommonGuardOnCheckInterruptDashRun(GObj *fighter_gobj,
                                                       s32 slide_tics);
extern void ndsBaseFTCommonAttackAirLwProcHit(GObj *fighter_gobj);
extern void ndsBaseFTCommonAttackAirLwProcUpdate(GObj *fighter_gobj);
extern void ndsBaseFTCommonAttackAirProcMap(GObj *fighter_gobj);
extern sb32 ndsBaseFTCommonAttackAirCheckInterruptCommon(GObj *fighter_gobj);
extern void ndsBaseFTCommonLandingAirSetStatus(GObj *fighter_gobj);
extern void ndsBaseFTCommonEscapeProcUpdate(GObj *fighter_gobj);
extern void ndsBaseFTCommonEscapeProcInterrupt(GObj *fighter_gobj);
extern void ndsBaseFTCommonEscapeProcStatus(GObj *fighter_gobj);
extern sb32 ndsBaseFTCommonEscapeCheckInterruptGuard(GObj *fighter_gobj);
extern void ndsBaseFTCommonAppealProcInterrupt(GObj *fighter_gobj);
extern sb32 ndsBaseFTCommonAppealCheckInterruptCommon(GObj *fighter_gobj);
extern void ndsBaseFTCommonCatchProcUpdate(GObj *fighter_gobj);
extern void ndsBaseFTCommonCatchProcMap(GObj *fighter_gobj);
extern sb32 ndsBaseFTCommonCatchCheckInterruptCommon(GObj *fighter_gobj);
extern void ndsBaseFTCommonCatchPullProcUpdate(GObj *fighter_gobj);
extern void ndsBaseFTCommonCatchPullProcCatch(GObj *fighter_gobj);
extern void ndsBaseFTCommonCatchWaitProcInterrupt(GObj *fighter_gobj);
extern void ndsBaseFTCommonCatchWaitSetStatus(GObj *fighter_gobj);
extern void ndsBaseFTCommonCapturePulledProcPhysics(GObj *fighter_gobj);
extern void ndsBaseFTCommonCapturePulledProcMap(GObj *fighter_gobj);
extern void ndsBaseFTCommonCapturePulledProcCapture(GObj *fighter_gobj,
                                                    GObj *capture_gobj);
extern void ndsBaseFTCommonCaptureWaitProcMap(GObj *fighter_gobj);
extern void ndsBaseFTCommonCaptureWaitSetStatus(GObj *fighter_gobj);
extern void ndsBaseFTCommonThrownProcUpdate(GObj *fighter_gobj);
extern void ndsBaseFTCommonThrownProcPhysics(GObj *fighter_gobj);
extern void ndsBaseFTCommonThrownProcMap(GObj *fighter_gobj);
extern void ndsBaseFTCommonThrownProcStatus(GObj *fighter_gobj);
extern void ndsBaseFTCommonThrownReleaseFighterLoseGrip(GObj *fighter_gobj);
extern void ndsBaseFTCommonThrownDecideFighterLoseGrip(GObj *fighter_gobj,
                                                       GObj *interact_gobj);
extern void ndsBaseFTCommonThrownDecideDeadResult(GObj *fighter_gobj);
extern void ndsBaseFTCommonThrownReleaseThrownUpdateStats(GObj *fighter_gobj,
                                                          s32 lr,
                                                          s32 script_id,
                                                          sb32 is_proc_status);
extern void ndsBaseFTCommonThrownUpdateDamageStats(FTStruct *this_fp);
extern void ndsBaseFTCommonThrownSetStatusDamageRelease(GObj *fighter_gobj);
extern void ndsBaseFTCommonThrownSetStatusNoDamageRelease(GObj *fighter_gobj);
extern void ndsBaseFTCommonThrowProcUpdate(GObj *fighter_gobj);
extern sb32 ndsBaseFTCommonThrowCheckInterruptCatchWait(GObj *fighter_gobj);
extern void ndsBaseFTCommonWallDamageProcUpdate(GObj *fighter_gobj);
extern sb32 ndsBaseFTCommonWallDamageCheckGoto(GObj *fighter_gobj);
static sb32 ndsStageCollisionLoopGeometryReady(void);
static u16 ndsMPO2RReadU16(const void *base, u32 half_index);
static s16 ndsMPO2RReadS16(const void *base, u32 half_index);
static u32 ndsMPGeometryYakumonoCount(MPGeometryData *geometry);
static u32 ndsMPGeometryMapObjCount(MPGeometryData *geometry);
static MPLineInfo *ndsMPLineInfoAt(MPLineInfo *line_info, u32 index);
static u32 ndsMPLineInfoYakumonoID(MPLineInfo *line_info);
static u32 ndsMPLineInfoGroupID(MPLineInfo *line_info, u32 kind);
static u32 ndsMPLineInfoLineCount(MPLineInfo *line_info, u32 kind);
static u32 ndsMPVertexLinkFirst(MPVertexLinks *links, u32 line_id);
static u32 ndsMPVertexLinkCount(MPVertexLinks *links, u32 line_id);
static u32 ndsMPVertexID(MPVertexArray *ids, u32 index);
static s32 ndsMPVertexX(MPVertexPosContainer *verts, u32 vertex_id);
static s32 ndsMPVertexY(MPVertexPosContainer *verts, u32 vertex_id);
static sb32 ndsMPProjectFloorGeometry(Vec3f *position,
                                      s32 *project_line_id,
                                      f32 *ga_dist,
                                      u32 *stand_coll_flags,
                                      Vec3f *angle);
static sb32 ndsFighterMarioFoxStageMPStaleFloorLoopProofEnabled(void);
static sb32 ndsMPFindLineYakumonoID(s32 line_id, u32 *yakumono_id);
static sb32 ndsFighterMarioFoxStageMPLiveStaleFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPFallMapFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCeilFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCeilStatusFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffWaitFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffAttackFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffClimbActionLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffClimbCommon2LoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffClimbFinishLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPPassiveLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled(void);
static void ndsFighterDashRunProbeSecondaryLiveHitbox(
    FTStruct *fp, u32 attack_id, const FTAttackColl *attack_coll);
static sb32 ndsFighterDashRunProbeSourceOrderHurtboxes(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp);
static sb32 ndsFighterDashRunProbeHurtboxDamageConsume(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp);
static sb32 ndsFighterDashRunCheckAttackInFighterRange(
    const Vec3f *attack_position, const Vec3f *obj_position,
    const Vec3f *range, f32 size);
static sb32 ndsGMCollisionCheckFighterAttackDamageCollideSelected(
    FTAttackColl *attack_coll, FTDamageColl *damage_coll);
static sb32 ndsGMCollisionCheckFighterAttacksCollideSelected(
    FTAttackColl *attack_coll1, FTAttackColl *attack_coll2);
static sb32 ndsGMCollisionCheckFighterAttackShieldCollideSelected(
    FTAttackColl *attack_coll, DObj *shield_joint, f32 *p_angle);
static sb32 ndsFighterDashRunProbeThrowAttribution(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp);
static sb32 ndsFighterDashRunProbeAttackClashStats(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp);
static sb32 ndsFighterDashRunProbeCatchStats(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp);
static sb32 ndsFighterDashRunProbeCatchSearch(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp);
static sb32 ndsFighterDashRunProbeSearchHitAllGhostGate(FTStruct *fp);
static sb32 ndsFighterDashRunProbeDamageEffectOnly(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp);
static sb32 ndsFighterDashRunProbeDamageResist(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp);
static sb32 ndsFighterDashRunSetDamageAttackRecord(FTStruct *fp,
                                                   u32 attack_group_id,
                                                   GObj *victim_gobj);
static s32 ndsFighterDashRunGetCapturedDamage(FTStruct *fp, s32 damage);
static sb32 ndsFighterDashRunCheckGetUpdateDamageNormal(FTStruct *fp,
                                                        s32 *damage);
static sb32 ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageTurnLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffLedgeLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffAttackActionLoopProofEnabled(void);
static void ndsStageMPDownWaitLoopAppendSourceOrder(u32 code);
static void ndsStageMPDownWaitLoopAppendAttackOrder(u32 code);
static void ndsStageMPDownWaitLoopAppendRollForwardOrder(u32 code);
static void ndsStageMPDownWaitLoopAppendRollBackOrder(u32 code);
static void ndsStageMPDownRecoverLoopAppendDownStandOrder(u32 code);
static void ndsStageMPDownRecoverLoopAppendAttackOrder(u32 code);
static void ndsStageMPDownRecoverLoopAppendRollForwardOrder(u32 code);
static void ndsStageMPDownRecoverLoopAppendRollBackOrder(u32 code);
static sb32 ndsFighterMarioFoxStageMPCliffCommon2LoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopProofEnabled(void);
static sb32 ndsFighterMarioFoxLivePreviewProofEnabled(void);
static sb32 __attribute__((unused)) ndsStageCollisionLoopCheckProjectFloor(
    Vec3f *pos, s32 *floor_line_id, f32 *floor_dist, u32 *floor_flags,
    Vec3f *floor_angle, sb32 *hit);
static void ndsStageFloorEdgeLoopRecordPreClamp(u32 slot,
                                                s32 pre_drift_milli);
static void ndsStageFloorEdgeLoopRecordFighter(GObj *fighter_gobj);
static sb32 ndsStageFloorEdgeLoopFloorYAtX(s32 line_id, f32 x,
                                           f32 *floor_y);
static sb32 ndsStageMPProcessFloorLoopUpdateFighter(GObj *fighter_gobj);
static sb32 ndsStageMPProcessFloorLoopBuildCollData(FTStruct *fp,
                                                    MPCollData *mp_coll);
static void ndsStageMPProcessFloorLoopCopyBack(FTStruct *fp,
                                               MPCollData *mp_coll);
static sb32 ndsStageMPUpdateFloorLoopUpdateFighter(GObj *fighter_gobj);
static sb32 ndsStageMPUpdateFloorLoopBuildCollData(FTStruct *fp,
                                                   MPCollData *mp_coll);
static void ndsStageMPUpdateFloorLoopPrimeMapMovement(GObj *fighter_gobj);
static sb32 ndsStageMPCeilStatusFloorLoopSpecialCollisions(
    MPCollData *coll_data, GObj *fighter_gobj, u32 flags);
static sb32 ndsStageMPCliffCatchFloorLoopSpecialCollisions(
    MPCollData *coll_data, GObj *fighter_gobj, u32 flags);
static sb32 ndsStageMPStaleFloorLoopChoosePair(FTStruct *fp,
                                               s32 *stale_line_id,
                                               s32 *target_line_id,
                                               Vec3f *target_translate,
                                               Vec3f *target_pos_prev);
static void ndsStageMPWallCopyFloorLoopGObjProc(GObj *fighter_gobj);
static void ndsStageMPPassFloorLoopGObjProc(GObj *fighter_gobj);
static void ndsStageMPPlatformFloorLoopGObjProc(GObj *fighter_gobj);
static void ndsFighterMarioFoxRunWaitStatusProbe(GObj *fighter_gobj,
                                                 FTDesc *desc);
static void ndsFighterMarioFoxRunWaitCallbackTickProbe(void);
static void ndsFighterMarioFoxRunWaitGroundProof(void);
static void ndsFighterMarioFoxRunDisplayProbe(void);
static void ndsFighterMarioFoxRunDLScanProbe(void);
static void ndsFighterMarioFoxRunDLExecuteProbe(void);
static void ndsFighterMarioFoxRunDLDrawProbe(void);
static void ndsFighterMarioFoxRunDLMultiDrawProbe(void);
static void ndsFighterMarioFoxRunDLAllDrawProbe(void);
static void ndsFighterMarioFoxRunWalkInputProof(void);
static void ndsFighterMarioFoxRunWalkLoopProof(void);
static void ndsFighterMarioFoxRunDashRunProof(void);
static void ndsFighterManagerRefreshProof(void);
static void ndsFighterManagerRecordCreatedFighter(GObj *fighter_gobj,
                                                  s32 player);
static void ndsFighterJumpAttackAirProbeMapLanding(GObj *fighter_gobj,
                                                    FTStruct *fp, DObj *root);
static void ndsFighterJumpAttackAirProbeDirections(GObj *fighter_gobj,
                                                   FTStruct *fp, DObj *root);
static void ndsFighterMarioFoxRunJumpLoopProof(void);
static void ndsFighterMarioFoxRunLandingLoopProof(void);
static void ndsFighterMarioFoxRunProcessLoopProof(void);
static void ndsFighterSchedulerLoopGObjProc(GObj *fighter_gobj);
static void ndsFighterSchedulerLoopRunSlotProcess(u32 slot, FTStruct *fp);
static void ndsFighterSchedulerLoopApplyPhaseInput(u32 slot, FTStruct *fp,
                                                   NDSFighterScriptInput *input);
static void ndsFighterSchedulerLoopAdvancePhase(u32 slot, FTStruct *fp);
static void ndsFighterSchedulerLoopRecordState(u32 slot, FTStruct *fp,
                                               s32 previous_status,
                                               s32 previous_ga);
static void ndsFighterSchedulerLoopRecordStart(u32 slot, FTStruct *fp,
                                               DObj *root);
static void ndsFighterSchedulerLoopRecordFinal(u32 slot, FTStruct *fp,
                                               DObj *root);
static void ndsFighterControllerLoopGObjProc(GObj *fighter_gobj);
static void ndsFighterControllerLoopRunSlotProcess(u32 slot, FTStruct *fp);
static void ndsFighterControllerLoopApplyPlayback(u32 slot, FTStruct *fp);
static void ndsFighterControllerLoopApplyFromSYController(u32 slot,
                                                          FTStruct *fp);
static void ndsFighterControllerLoopAdvancePhase(u32 slot, FTStruct *fp);
static void ndsFighterControllerLoopRecordState(u32 slot, FTStruct *fp,
                                                s32 previous_status,
                                                s32 previous_ga);
static void ndsFighterControllerLoopRecordStart(u32 slot, FTStruct *fp,
                                                DObj *root);
static void ndsFighterControllerLoopRecordFinal(u32 slot, FTStruct *fp,
                                                DObj *root);
static void ndsFighterPreviewLoopGObjProc(GObj *fighter_gobj);
static void ndsFighterPreviewLoopRecordState(u32 slot, FTStruct *fp,
                                             s32 previous_status,
                                             s32 previous_ga);
static void ndsFighterPreviewLoopRecordDisplayFromCallback(GObj *fighter_gobj);
static void ndsFighterGCDrawAllLoopRecordDisplayFromCallback(
    GObj *fighter_gobj);
static void ndsFighterLivePreviewGObjProc(GObj *fighter_gobj);
static void ndsFighterLivePreviewApplyFromSYController(u32 slot,
                                                       FTStruct *fp);
static void ndsFighterLivePreviewRunSlotProcess(u32 slot, FTStruct *fp);
static void ndsFighterLivePreviewRecordStart(u32 slot, FTStruct *fp,
                                             DObj *root);
static void ndsFighterLivePreviewRecordFinal(u32 slot, FTStruct *fp,
                                             DObj *root);
static void ndsFighterLivePreviewRecordState(u32 slot, FTStruct *fp,
                                             s32 previous_status,
                                             s32 previous_ga);
static void ndsFighterLivePreviewDrawKeyframe(void);
static void ndsFighterLivePreviewCopyDrawFromPreview(void);
static void ndsFighterProcessLoopApplyScriptInput(
    u32 slot, FTStruct *fp, const NDSFighterScriptInput *input);
static void ndsFighterProcessLoopRunFrame(u32 slot, FTStruct *fp);
static void ndsFighterProcessLoopRunUpdate(u32 slot, FTStruct *fp);
static void ndsFighterProcessLoopRunInterrupt(u32 slot, FTStruct *fp);
static void ndsFighterProcessLoopRunPhysics(u32 slot, FTStruct *fp);
static void ndsFighterProcessLoopIntegrate(u32 slot, FTStruct *fp);
static void ndsFighterProcessLoopRunMap(u32 slot, FTStruct *fp);
static void ndsFTMainApplyCommonStatusReset(FTStruct *fp, u32 flags);
static void ndsFighterProcessLoopSetStatus(FTStruct *fp, GObj *fighter_gobj,
                                           s32 status_id, f32 frame_begin,
                                           f32 anim_speed, u32 flags);
static void ndsFighterProcessLoopRecordState(u32 slot, FTStruct *fp,
                                             s32 previous_status,
                                             s32 previous_ga);
static void ndsFighterWalkLoopRunHeldFrame(u32 slot, FTStruct *fp);
static void ndsFighterWalkLoopRunReleaseToWait(u32 slot, FTStruct *fp);
static void ndsFighterWalkLoopRunWaitSettleFrame(u32 slot, FTStruct *fp);
static void ndsFighterWalkLoopRecordStart(u32 slot, FTStruct *fp,
                                           DObj *root);
static void ndsFighterWalkLoopRecordAfterHeld(u32 slot, FTStruct *fp,
                                              DObj *root);
static void ndsFighterWalkLoopRecordAfterRelease(u32 slot, FTStruct *fp,
                                                 DObj *root);
static void ndsFighterWalkLoopRecordAfterSettle(u32 slot, FTStruct *fp,
                                                DObj *root);
static void ndsFighterDashRunRunGuardProof(u32 slot, FTStruct *fp);
static void ndsFighterMarioFoxRecordDisplayProbe(GObj *fighter_gobj);
static void ndsFighterMarioFoxRecordDLAllDrawFromDisplayCallback(
    GObj *fighter_gobj);
static void ndsFTPhysicsApplyGroundVelFrictionBounded(GObj *fighter_gobj);
static void ndsFTPhysicsSetGroundVelTransferAirOriginal(GObj *fighter_gobj,
                                                        FTStruct *fp);
static void ndsFighterSyncPhysicsToLegacyVel(FTStruct *fp);
static sb32 ndsMPCommonCheckFighterOnCliffEdgeBounded(GObj *fighter_gobj);
static sb32 ndsStageFloorFollowLoopUpdateFighter(GObj *fighter_gobj);
static void ndsFighterMarioFoxSetupManagerFiles(void);
static void ndsFighterMarioFoxSetupFilesKind(s32 fkind);
static GObj *ndsFighterMarioFoxMakeFighter(FTDesc *desc);
static void ndsFighterMarioFoxRecordStubFighter(FTDesc *desc,
                                                GObj *fighter_gobj);
static void ndsFighterMarioFoxResetFileSlots(void);
static void ndsFighterStructResetPool(void);
static sb32 ndsFighterStructIsPoolPointer(const void *ptr);
static sb32 ndsFighterStructIsTrackedPointer(const void *ptr);
static GObj *ndsFighterGetPlayerNumGObj(s32 player_num);
static void ndsFighterPartsSyncDObj(FTStruct *fp, DObj *dobj, u32 joint_id);

static sb32 sNdsFighterWaitGroundPassActive;
static sb32 sNdsFighterDisplayProbeActive;
static sb32 sNdsFighterDLAllDrawProbeActive;
static sb32 sNdsFighterWalkInputProbeActive;
static sb32 sNdsFighterWalkLoopProbeActive;
static sb32 sNdsFighterWalkPhysicsMapPassActive;
static sb32 sNdsFighterWalkLoopFrameActive;
static sb32 sNdsFighterWalkLoopMapActive;
static sb32 sNdsFighterWalkLoopWaitReturnActive;
static sb32 sNdsFighterWalkLoopWaitFrictionActive;
static sb32 sNdsFighterDashRunWaitInterruptActive;
static sb32 sNdsFighterDashRunDashInterruptActive;
static sb32 sNdsFighterDashRunRunInterruptActive;
static sb32 sNdsFighterDashRunRunBrakeInterruptActive;
static sb32 sNdsFighterDashRunAttack1Active;
static sb32 sNdsFighterDashRunGuardOnActive;
static sb32 sNdsFighterDashRunEscapeActive;
static sb32 sNdsFighterDashRunEscapeInterruptActive;
static sb32 sNdsFighterDashRunEscapePhysicsActive;
static sb32 sNdsFighterDashRunEscapeMapActive;
static sb32 sNdsFighterDashRunAttack11UpdateActive;
static sb32 sNdsFighterDashRunAttack11InterruptActive;
static sb32 sNdsFighterDashRunAttack11PhysicsActive;
static sb32 sNdsFighterDashRunAttack11MapActive;
static sb32 sNdsFighterDashRunAttackDashActive;
static sb32 sNdsFighterDashRunAttackDashUpdateActive;
static sb32 sNdsFighterDashRunAttackDashPhysicsActive;
static sb32 sNdsFighterDashRunAttackDashMapActive;
static sb32 sNdsFighterDashRunTurnRunActive;
static sb32 sNdsFighterDashRunTurnRunUpdateActive;
static sb32 sNdsFighterDashRunDashPhysicsActive;
static sb32 sNdsFighterDashRunRunPhysicsActive;
static sb32 sNdsFighterDashRunRunBrakePhysicsActive;
static sb32 sNdsFighterDashRunDashMapActive;
static sb32 sNdsFighterDashRunRunMapActive;
static sb32 sNdsFighterDashRunRunBrakeMapActive;
static sb32 sNdsFighterDashRunDamageStatusSetupActive;
static sb32 sNdsFighterDashRunDamagePhysicsActive;
static sb32 sNdsFighterDashRunDamageInterruptActive;
static sb32 sNdsFighterDashRunDamageFallSourceInterruptActive;
static sb32 sNdsFighterDashRunDamageExpiryActive;
static sb32 sNdsFighterDashRunDamageFallPhysicsActive;
static sb32 sNdsFighterDashRunDamageMapActive;
static sb32 sNdsStageMPLiveHitStatusLoopDownBounceSetStatusActive;
static sb32 sNdsStageMPLiveHitStatusLoopCliffCatchSetStatusActive;
static sb32 sNdsFighterDashRunDamageHammerCheckActive;
static sb32 sNdsFighterDashRunDamageHammerHold;
static u32 sNdsFighterDashRunDamageAirMapCount;
static u32 sNdsFighterDashRunDamageFallInterruptCount;
static u32 sNdsFighterDashRunDamageFallSourceInterruptCount;
static u32 sNdsFighterDashRunDamageFallSpecialAirCheckCount;
static u32 sNdsFighterDashRunDamageFallAttackAirCheckCount;
static u32 sNdsFighterDashRunDamageFallJumpAerialCheckCount;
static u32 sNdsFighterDashRunDamageFallHammerCheckCount;
static sb32 sNdsFighterDashRunDamageFallSetStatusFromDamageActive;
static u32 sNdsFighterDashRunDamageFallSetStatusCount;
static u32 sNdsFighterDashRunDamageFallFTMainSetStatusCount;
static u32 sNdsFighterDashRunDamageFallClampRumbleCount;
static u32 sNdsFighterDashRunDamageFallPhysicsCount;
static u32 sNdsFighterDashRunDamageFallMapCollisionMode;
static u32 sNdsFighterDashRunDamageFallMapCount;
static u32 sNdsFighterDashRunDamageFallMapNoCollisionCount;
static u32 sNdsFighterDashRunDamageFallMapCollisionCount;
static u32 sNdsFighterDashRunDamageFallPassiveStandCheckCount;
static u32 sNdsFighterDashRunDamageFallPassiveCheckCount;
static u32 sNdsFighterDashRunDamageFallDownBounceSetStatusCount;
static u32 sNdsFighterDashRunDamageFallCliffCatchSetStatusCount;
static u32 sNdsFighterDashRunDamageCommonFallInterruptCount;
static u32 sNdsFighterDashRunDamageSetupPublicCount;
static u32 sNdsFighterDashRunDamageSetupColAnimCount;
static u32 sNdsFighterDashRunDamageRunUpdateColAnimCount;
static s32 sNdsFighterDashRunDamageColAnimLastID;
static s32 sNdsFighterDashRunDamageColAnimLastDuration;
static s32 sNdsFighterDashRunDamageSkeletonColAnimLastLevel;
static u32 sNdsFighterDashRunDamageSetupScreenFlashCount;
static s32 sNdsFighterDashRunDamageScreenFlashLastID;
static s32 sNdsFighterDashRunDamageScreenFlashLastDuration;
static u32 sNdsFighterDashRunDamageSetupRumbleCount;
static u32 sNdsFighterDashRunDamageSetupDustCount;
static u32 sNdsFighterDashRunDamageSetupDustEffectCount;
static u32 sNdsFighterDashRunDamageSetupPlayerTagCount;
static u32 sNdsFighterDashRunDamageSetupAttackerCount;
static u32 sNdsFighterDashRunDamageOriginalInitCount;
static u32 sNdsFighterDashRunDamageOriginalGotoCount;
static sb32 sNdsFighterDashRunDamageOriginalInitActive;
static u32 sNdsFighterDashRunDamagePublicCheckCount;
static u32 sNdsFighterDashRunDamagePublicForceCount;
static s32 sNdsFighterDashRunDamagePublicLastKnockbackMilli;
static sb32 sNdsFighterDashRunDamageVoiceActive;
static u32 sNdsFighterDashRunDamageVoiceCount;
static u32 sNdsFighterDashRunDamageVoiceLastFGM;
static sb32 sNdsFighterDashRunProcParamsRumbleActive;
static u32 sNdsFighterDashRunDamageHammerCheckCount;
static u32 sNdsFighterDashRunDamageHammerGroundCount;
static u32 sNdsFighterDashRunDamageHammerAirCount;
static AObjEvent32 *sNdsFighterDashRunGuardAnimJoints[nFTPartsJointNumMax];
static DObjDesc sNdsFighterDashRunGuardDObjLookup[nFTPartsJointNumMax];
static sb32 sNdsFighterJumpRunBrakeEndActive;
static sb32 sNdsFighterJumpWaitProbeActive;
static sb32 sNdsFighterJumpKneeBendUpdateActive;
static sb32 sNdsFighterJumpKneeBendInterruptActive;
static sb32 sNdsFighterJumpSetStatusActive;
static sb32 sNdsFighterJumpAirInterruptActive;
static sb32 sNdsFighterJumpAirPhysicsActive;
static sb32 sNdsFighterJumpAirMapActive;
static sb32 sNdsFighterJumpAttackAirActive;
static sb32 sNdsFighterJumpAttackAirRefreshActive;
static sb32 sNdsFighterJumpAttackAirMapLandingActive;
static sb32 sNdsFighterJumpAttackAirDirectionActive;
static sb32 sNdsStageMPLiveHitOriginalRehitRefreshActive;
static sb32 sNdsFighterLandingJumpAnimEndActive;
static sb32 sNdsFighterLandingFallInterruptActive;
static sb32 sNdsFighterLandingFallPhysicsActive;
static sb32 sNdsFighterLandingFallMapActive;
static sb32 sNdsFighterLandingSetStatusActive;
static sb32 sNdsFighterLandingProcInterruptActive;
static sb32 sNdsFighterLandingPhysicsActive;
static sb32 sNdsFighterLandingEndActive;
static sb32 sNdsFighterLandingWaitSettleActive;
static sb32 sNdsFighterProcessLoopActive;
static sb32 sNdsFighterProcessLoopUpdateActive;
static sb32 sNdsFighterProcessLoopInterruptActive;
static sb32 sNdsFighterProcessLoopPhysicsActive;
static sb32 sNdsFighterProcessLoopMapActive;
static sb32 sNdsFighterProcessLoopRunBrakeEndActive;
static sb32 sNdsFighterProcessLoopJumpAnimEndActive;
static sb32 sNdsFighterProcessLoopLandingEndActive;
static sb32 sNdsFighterSchedulerLoopActive;
static GObjProcess *sNdsFighterSchedulerLoopProcesses[2];
static sb32 sNdsFighterControllerLoopActive;
static GObjProcess *sNdsFighterControllerLoopProcesses[2];
static sb32 sNdsFighterPreviewLoopActive;
static sb32 sNdsFighterPreviewLoopDisplayActive;
static GObjProcess *sNdsFighterPreviewLoopProcesses[2];
static u16 *sNdsFighterPreviewLoopPixels;
static u32 sNdsFighterPreviewLoopPitch;
static u32 sNdsFighterPreviewLoopDrawFrameIndex;
static sb32 sNdsFighterGCRunAllLoopActive;
static GObjProcess *sNdsFighterGCRunAllLoopProcesses[2];
static sb32 sNdsFighterGCDrawAllLoopActive;
static sb32 sNdsFighterGCDrawAllLoopDisplayActive;
static GObjProcess *sNdsFighterGCDrawAllLoopProcesses[2];
static u16 *sNdsFighterGCDrawAllLoopPixels;
static u32 sNdsFighterGCDrawAllLoopPitch;
static sb32 sNdsFighterLivePreviewActive;
static GObjProcess *sNdsFighterLivePreviewProcesses[2];
static u16 *sNdsFighterDLAllDrawPixels;
static u32 sNdsFighterDLAllDrawPitch;
static Vec3f sNdsStageMPUpdateFloorLoopPrevRoot[2];
static u8 sNdsStageMPUpdateFloorLoopPrevRootValid[2];
static s32 sNdsStageMPCrossFloorLoopLiveSlot = -1;
static sb32 sNdsStageMPLiveStaleFloorLoopProbeActive = FALSE;
static sb32 sNdsStageMPMotionStaleFloorLoopUpdateActive = FALSE;
static sb32 sNdsStageMPCliffStatusFloorLoopActive = FALSE;
static sb32 sNdsStageMPCliffStatusFloorLoopStatusActive = FALSE;
static sb32 sNdsStageMPCliffTickFloorLoopStatusActive = FALSE;
static sb32 sNdsStageMPFallMapFloorLoopPhysicsActive = FALSE;
static sb32 sNdsStageMPFallMapFloorLoopMapActive = FALSE;
static sb32 sNdsStageMPFallLandFloorLoopPhysicsActive = FALSE;
static sb32 sNdsStageMPFallLandFloorLoopMapActive = FALSE;
static sb32 sNdsStageMPFallLandFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCeilStatusFloorLoopMapActive = FALSE;
static sb32 sNdsStageMPCeilStatusFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffCatchFloorLoopMapActive = FALSE;
static sb32 sNdsStageMPCliffCatchFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffCatchFloorLoopOccupancyActive = FALSE;
static sb32 sNdsStageMPCliffWaitFloorLoopUpdateActive = FALSE;
static sb32 sNdsStageMPCliffWaitFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffWaitFloorLoopInterruptActive = FALSE;
static sb32 sNdsStageMPCliffAttackFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffAttackFloorLoopInterruptActive = FALSE;
static sb32 sNdsStageMPCliffClimbFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffClimbFloorLoopInterruptActive = FALSE;
static sb32 sNdsStageMPCliffClimbFloorLoopRecatchMapActive = FALSE;
static sb32 sNdsStageMPCliffClimbFloorLoopRecatchSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffClimbActionLoopQuickUpdateActive = FALSE;
static sb32 sNdsStageMPCliffClimbActionLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffClimbActionLoopAnimEndActive = FALSE;
static volatile sb32 sNdsStageMPCliffClimbCommon2LoopUpdateActive = FALSE;
static volatile sb32 sNdsStageMPCliffClimbCommon2LoopPhysicsActive = FALSE;
static volatile sb32 sNdsStageMPCliffClimbCommon2LoopMapActive = FALSE;
static sb32 sNdsStageMPCliffClimbFinishLoopUpdateActive = FALSE;
static sb32 sNdsStageMPCliffLiveLoopWaitUpdateActive = FALSE;
static sb32 sNdsStageMPCliffLiveLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffLiveLoopInterruptActive = FALSE;
static sb32 sNdsStageMPCliffLiveLoopQuickUpdateActive = FALSE;
static sb32 sNdsStageMPCliffLiveLoopQuick1UpdateActive = FALSE;
static sb32 sNdsStageMPCliffLiveLoopCommon2UpdateActive = FALSE;
static sb32 sNdsStageMPCliffLiveLoopCommon2PhysicsActive = FALSE;
static sb32 sNdsStageMPCliffLiveLoopCommon2MapActive = FALSE;
static GObjProcess *sNdsStageMPCliffLiveLoopProcess = NULL;
static u32 sNdsStageMPCliffLiveLoopPhase = 0u;
static GObjProcess *sNdsStageMPWallCopyFloorLoopProcess = NULL;
static GObjProcess *sNdsStageMPPassFloorLoopProcess = NULL;
static GObjProcess *sNdsStageMPPlatformFloorLoopProcess = NULL;
static GObj *sNdsStageMPPlatformActiveFloorLoopGObj = NULL;
static sb32 sNdsStageMPPlatformTickFloorLoopAdvanceActive = FALSE;
static sb32 sNdsStageMPPassInputLoopInputActive = FALSE;
static sb32 sNdsStageMPPassInputLoopStatusActive = FALSE;
static sb32 sNdsStageInishieScaleLoopActive = FALSE;
static u32 sNdsStageInishieScaleLoopPhase = 0u;
static sb32 sNdsStageInishieScaleSourceSetupActive = FALSE;
static void *sNdsStageInishieMapFile = NULL;
static MPGroundData *sNdsStageInishieGroundData = NULL;
static size_t sNdsStageInishieMapDataSize = 0u;
static f32 sNdsStageMPWallHitScoutWidth = 0.0F;
static f32 sNdsStageMPWallHitScoutCenter = 0.0F;
static sb32 sNdsStageMPCliffWaitDamageLoopInterruptActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDamageFallInterruptActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDamageFallPhysicsActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopPassiveStandCallbackActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopPassiveCallbackActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDownBounceUpdateActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDownWaitSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDownWaitUpdateActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDownStandSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive = FALSE;
static u32 sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 0u;
static sb32 sNdsStageMPPassiveLoopPassiveStandSetStatusActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveSetStatusActive = FALSE;
static sb32 sNdsStageMPPassiveLoopBranchProbeActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveStandBActive = FALSE;
static sb32 sNdsStageMPPassiveLoopDamageFallMapActive = FALSE;
static sb32 sNdsStageMPPassiveLoopNaturalMapFloorHit = FALSE;
static sb32 sNdsStageMPPassiveLoopAppealActive = FALSE;
static sb32 sNdsStageMPPassiveLoopAppealGuardActive = FALSE;
static sb32 sNdsStageMPPassiveLoopCatchActive = FALSE;
static sb32 sNdsStageMPPassiveLoopCatchMapActive = FALSE;
static sb32 sNdsStageMPPassiveLoopCatchUpdateActive = FALSE;
static sb32 sNdsStageMPPassiveLoopCatchPullActive = FALSE;
static sb32 sNdsStageMPPassiveLoopCatchPullUpdateActive = FALSE;
static sb32 sNdsStageMPPassiveLoopCatchWaitInterruptActive = FALSE;
static sb32 sNdsStageMPPassiveLoopCaptureActive = FALSE;
static sb32 sNdsStageMPPassiveLoopCapturePhysicsActive = FALSE;
static sb32 sNdsStageMPPassiveLoopCaptureMapActive = FALSE;
static sb32 sNdsStageMPPassiveLoopCaptureWaitMapActive = FALSE;
static sb32 sNdsStageMPPassiveLoopThrowActive = FALSE;
static sb32 sNdsStageMPPassiveLoopThrowCallbackImmediateActive = FALSE;
static sb32 sNdsStageMPPassiveLoopThrowUpdateActive = FALSE;
static sb32 sNdsStageMPPassiveLoopThrowReleaseActive = FALSE;
static sb32 sNdsStageMPPassiveLoopThrowReleaseStatusActive = FALSE;
static sb32 sNdsStageMPPassiveLoopThrowProcStatusActive = FALSE;
static sb32 sNdsStageMPPassiveLoopThrowDeadResultActive = FALSE;
static sb32 sNdsStageMPPassiveLoopWallDamageActive = FALSE;
static sb32 sNdsStageMPPassiveLoopWallDamageFallSetStatusFromDamageActive;
static u32 sNdsStageMPPassiveLoopWallDamageFallFTMainSetStatusCount;
static u32 sNdsStageMPPassiveLoopWallDamageFallClampRumbleCount;
static sb32 sNdsStageMPPassiveLoopReboundActive = FALSE;
static sb32 sNdsStageMPPassiveLoopReboundUpdateActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveStandCallbackActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveCallbackActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveStandUpdateActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveUpdateActive = FALSE;
static FTThrownStatusArray
    sNdsStageMPPassiveLoopThrownStatus[nFTKindNull + 1];
static FTThrowHitDesc sNdsStageMPPassiveLoopThrowReleaseDesc[2];
static sb32 sNdsStageMPDownWaitLoopDownWaitSetStatusActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownWaitInterruptActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownStandSetStatusActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownAttackSetStatusActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopAttackProbeActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopRollForwardProbeActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopRollBackProbeActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopAttackCallbackActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopAttackUpdateActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopRollForwardCallbackActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopRollForwardUpdateActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopRollBackCallbackActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopRollBackUpdateActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownStandInterruptActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownStandCallbackActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownStandUpdateActive = FALSE;
static sb32 sNdsStageTurnLoopSetStatusActive = FALSE;
static sb32 sNdsStageTurnLoopUpdateActive = FALSE;
static sb32 sNdsStageTurnLoopFinalUpdateActive = FALSE;
static sb32 sNdsStageTurnLoopPhysicsMapActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownWaitSetStatusActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownWaitInterruptActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownStandSetStatusActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownAttackSetStatusActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownStandProbeActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopAttackProbeActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopRollForwardProbeActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopRollBackProbeActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownStandUpdateActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopAttackUpdateActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopRollForwardUpdateActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopRollBackUpdateActive = FALSE;
static sb32 sNdsStageMPCliffAttackActionLoopQuickUpdateActive = FALSE;
static sb32 sNdsStageMPCliffAttackActionLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffAttackActionLoopAnimEndActive = FALSE;
static volatile sb32 sNdsStageMPCliffCommon2LoopUpdateActive = FALSE;
static volatile sb32 sNdsStageMPCliffCommon2LoopPhysicsActive = FALSE;
static volatile sb32 sNdsStageMPCliffCommon2LoopMapActive = FALSE;
static sb32 sNdsStageMPCliffEscapeActionLoopInterruptActive = FALSE;
static sb32 sNdsStageMPCliffEscapeActionLoopQuickUpdateActive = FALSE;
static sb32 sNdsStageMPCliffEscapeActionLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffEscapeActionLoopAnimEndActive = FALSE;
static volatile sb32 sNdsStageMPCliffEscapeCommon2LoopUpdateActive = FALSE;
static volatile sb32 sNdsStageMPCliffEscapeCommon2LoopPhysicsActive = FALSE;
static volatile sb32 sNdsStageMPCliffEscapeCommon2LoopMapActive = FALSE;
static FTStruct *sNdsFTCommonCliffCommon2BridgeStruct = NULL;

#define NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES 8u
#define NDS_STAGE_MPDOWNWAIT_ROLL_TRANSN_SPEED 1.25F

struct NDSFighterScriptInput
{
    s8 stick_x;
    s8 stick_y;
    u16 button_hold;
    u16 button_tap;
    u16 button_release;
    u8 tap_stick_x;
    u8 tap_stick_y;
    u8 hold_stick_x;
    u8 hold_stick_y;
};

typedef enum NDSFighterProcessLoopPhase
{
    nNDSFighterProcessLoopPhaseWalkStart,
    nNDSFighterProcessLoopPhaseWalkHold,
    nNDSFighterProcessLoopPhaseWalkRelease,
    nNDSFighterProcessLoopPhaseDashStart,
    nNDSFighterProcessLoopPhaseRunHold,
    nNDSFighterProcessLoopPhaseRunRelease,
    nNDSFighterProcessLoopPhaseRunBrakeEnd,
    nNDSFighterProcessLoopPhaseJumpStart,
    nNDSFighterProcessLoopPhaseJumpAir,
    nNDSFighterProcessLoopPhaseFallLand,
    nNDSFighterProcessLoopPhaseDone
} NDSFighterProcessLoopPhase;

typedef struct NDSFighterProcessLoopState
{
    NDSFighterProcessLoopPhase phase;
    u32 phase_frame;
    u32 total_frames;
    u32 completed;
    u32 status_visit_mask;
    u32 transition_mask;
    u32 wait_visit_count;
    u32 walk_visit_count;
    u32 dash_visit_count;
    u32 run_visit_count;
    u32 runbrake_visit_count;
    u32 kneebend_visit_count;
    u32 jump_visit_count;
    u32 fall_visit_count;
    u32 landing_visit_count;
} NDSFighterProcessLoopState;

typedef enum NDSFighterSchedulerLoopPhase
{
    nNDSFighterSchedulerLoopPhaseWalkStart,
    nNDSFighterSchedulerLoopPhaseWalkHold,
    nNDSFighterSchedulerLoopPhaseWalkRelease,
    nNDSFighterSchedulerLoopPhaseDashStart,
    nNDSFighterSchedulerLoopPhaseRunHold,
    nNDSFighterSchedulerLoopPhaseRunRelease,
    nNDSFighterSchedulerLoopPhaseRunBrakeEnd,
    nNDSFighterSchedulerLoopPhaseJumpStart,
    nNDSFighterSchedulerLoopPhaseJumpAir,
    nNDSFighterSchedulerLoopPhaseFallLand,
    nNDSFighterSchedulerLoopPhaseDone
} NDSFighterSchedulerLoopPhase;

typedef struct NDSFighterSchedulerLoopState
{
    NDSFighterSchedulerLoopPhase phase;
    u32 phase_frame;
    u32 total_frames;
    u32 completed;
    u32 status_visit_mask;
    u32 transition_mask;
    u32 wait_visit_count;
    u32 walk_visit_count;
    u32 dash_visit_count;
    u32 run_visit_count;
    u32 runbrake_visit_count;
    u32 kneebend_visit_count;
    u32 jump_visit_count;
    u32 fall_visit_count;
    u32 landing_visit_count;
    f32 root_y_start;
    f32 root_y_max;
} NDSFighterSchedulerLoopState;

static NDSFighterSchedulerLoopState sNdsFighterSchedulerLoopStates[2];

typedef enum NDSFighterControllerLoopPhase
{
    nNDSFighterControllerLoopPhaseWalkStart,
    nNDSFighterControllerLoopPhaseWalkHold,
    nNDSFighterControllerLoopPhaseWalkRelease,
    nNDSFighterControllerLoopPhaseDashStart,
    nNDSFighterControllerLoopPhaseRunHold,
    nNDSFighterControllerLoopPhaseRunRelease,
    nNDSFighterControllerLoopPhaseRunBrakeEnd,
    nNDSFighterControllerLoopPhaseJumpStart,
    nNDSFighterControllerLoopPhaseJumpAir,
    nNDSFighterControllerLoopPhaseFallLand,
    nNDSFighterControllerLoopPhaseDone
} NDSFighterControllerLoopPhase;

typedef struct NDSFighterControllerLoopState
{
    NDSFighterControllerLoopPhase phase;
    u32 phase_frame;
    u32 total_frames;
    u32 completed;
    u32 status_visit_mask;
    u32 transition_mask;
    u32 wait_visit_count;
    u32 walk_visit_count;
    u32 dash_visit_count;
    u32 run_visit_count;
    u32 runbrake_visit_count;
    u32 kneebend_visit_count;
    u32 jump_visit_count;
    u32 fall_visit_count;
    u32 landing_visit_count;
    s8 previous_stick_x;
    s8 previous_stick_y;
    f32 root_y_start;
    f32 root_y_max;
} NDSFighterControllerLoopState;

static NDSFighterControllerLoopState sNdsFighterControllerLoopStates[2];

typedef enum NDSFighterPreviewLoopPhase
{
    nNDSFighterPreviewLoopPhaseWalkStart,
    nNDSFighterPreviewLoopPhaseWalkHold,
    nNDSFighterPreviewLoopPhaseWalkRelease,
    nNDSFighterPreviewLoopPhaseDashStart,
    nNDSFighterPreviewLoopPhaseRunHold,
    nNDSFighterPreviewLoopPhaseRunRelease,
    nNDSFighterPreviewLoopPhaseRunBrakeEnd,
    nNDSFighterPreviewLoopPhaseJumpStart,
    nNDSFighterPreviewLoopPhaseJumpAir,
    nNDSFighterPreviewLoopPhaseFallLand,
    nNDSFighterPreviewLoopPhaseDone
} NDSFighterPreviewLoopPhase;

typedef struct NDSFighterPreviewLoopState
{
    NDSFighterPreviewLoopPhase phase;
    u32 phase_frame;
    u32 total_frames;
    u32 completed;
    u32 status_visit_mask;
    u32 transition_mask;
    u32 wait_visit_count;
    u32 walk_visit_count;
    u32 dash_visit_count;
    u32 run_visit_count;
    u32 runbrake_visit_count;
    u32 kneebend_visit_count;
    u32 jump_visit_count;
    u32 fall_visit_count;
    u32 landing_visit_count;
    s8 previous_stick_x;
    s8 previous_stick_y;
    f32 root_y_start;
    f32 root_y_max;
    s32 screen_x_start;
    s32 screen_x_final;
    s32 screen_y_floor;
    s32 screen_y_min;
    u32 screen_initialized;
} NDSFighterPreviewLoopState;

static NDSFighterPreviewLoopState sNdsFighterPreviewLoopStates[2];

typedef struct NDSFighterLivePreviewState
{
    u32 total_frames;
    u32 completed;
    u32 status_visit_mask;
    u32 transition_mask;
    u32 wait_visit_count;
    s8 previous_stick_x;
    s8 previous_stick_y;
    f32 root_y_start;
} NDSFighterLivePreviewState;

static NDSFighterLivePreviewState sNdsFighterLivePreviewStates[2];
static sb32 sNdsFTMainSetStatusCompatReplayActive = FALSE;
static void ndsRelocNormalizeMObjSubWordSwapped(MObjSub *mobjsub);

/* Compatibility/proof shims. */
#include "reloc_backend_compat_shims.c"

/* Relocation asset loading. */
#include "reloc_backend_assets.c"

/* Fighter model/struct proofs. */
#include "reloc_backend_fighter_model.c"

/* Renderer/DL helpers. */
#include "reloc_backend_renderer_dl.c"

/* Movement proofs. */
#include "reloc_backend_movement.c"

/* MP collision proofs. */
#include "reloc_backend_mp_collision.c"

/* Cliff/ledge proofs. */
#include "reloc_backend_cliff_ledge.c"

/* Diagnostic recorders. */
#include "reloc_backend_diagnostic_recorders.c"
