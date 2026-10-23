/* The relocation backend: the forward declarations its slices share, then
 * the slices in order -- the compatibility shims, the relocation asset
 * loading (registry, tables, file IDs, resolution, normalizers, symbols and
 * extern trees, the motion cache, the force loads), the fighter model, the
 * renderer adapters, movement and the stage draw loop, map collision, the
 * cliff and ledge seam and the diagnostic recorders. Part of the
 * scene_backend.c unit. */

#include <ef/effect.h>
#include <it/item.h>
#include <nds/nds_fcmp.h>
#include <nds/nds_gbi_decode.h>
#include <nds/nds_fighter_display.h>
#include <nds/nds_mp_topology.h>
#include <nds/nds_renderer.h>
#include <nds/nds_scene_harness.h>
#include "nds_scene_harness_config.h"
#include <sys/utils.h>
#include <wp/weapon.h>

extern void syUtilsSetRandomSeed(s32 seed);
extern s32 syUtilsRandSeed(void);

static u32 ndsFloatBits(f32 value);
static s32 ndsFloatToMilliSigned(f32 value);
static void ndsFTPhysicsSetGroundVelTransferAirOriginal(GObj *fighter_gobj,
                                                        FTStruct *fp);
static void ndsFighterSyncPhysicsToLegacyVel(FTStruct *fp);
static s32 ndsFTCommonDamageMotionForStatus(s32 status_id);
static sb32 ndsFTCommonDamageIsStatus(s32 status_id);
static sb32 ndsFTMainSetStatusDamageHarness(GObj *fighter_gobj,
                                            s32 status_id, f32 frame_begin,
                                            f32 anim_speed, u32 flags);
typedef struct NDSFighterScriptInput NDSFighterScriptInput;
static sb32 ndsFighterMarioFoxDashRunProofEnabled(void);
static sb32 ndsFighterMarioFoxJumpAttackAirProofEnabled(void);
static sb32 ndsFighterMarioFoxStageGCDrawAllLoopProofEnabled(void);
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
static NDSMPO2RHalfwordView ndsMPLineInfoAt(MPLineInfo *line_info,
                                             u32 index);
static u32 ndsMPLineInfoYakumonoID(NDSMPO2RHalfwordView line_info);
static u32 ndsMPLineInfoGroupID(NDSMPO2RHalfwordView line_info, u32 kind);
static u32 ndsMPLineInfoLineCount(NDSMPO2RHalfwordView line_info, u32 kind);
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
static sb32 ndsMPFindLineYakumonoID(s32 line_id, u32 *yakumono_id);
static sb32 ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPPassiveLoopProofEnabled(void);
static sb32 ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled(void);
static sb32 __attribute__((unused)) ndsStageCollisionLoopCheckProjectFloor(
    Vec3f *pos, s32 *floor_line_id, f32 *floor_dist, u32 *floor_flags,
    Vec3f *floor_angle, sb32 *hit);
static sb32 ndsStageMPUpdateFloorLoopUpdateFighter(GObj *fighter_gobj);
static void ndsFighterManagerRefreshProof(void);
static void ndsFighterManagerRecordCreatedFighter(GObj *fighter_gobj,
                                                  s32 player);
static void ndsFTMainApplyCommonStatusReset(FTStruct *fp, u32 flags);
static void ndsFTPhysicsSetGroundVelTransferAirOriginal(GObj *fighter_gobj,
                                                        FTStruct *fp);
static void ndsFighterSyncPhysicsToLegacyVel(FTStruct *fp);
static sb32 ndsMPCommonCheckFighterOnCliffEdgeBounded(GObj *fighter_gobj);
static sb32 ndsStageFloorFollowLoopUpdateFighter(GObj *fighter_gobj);
static void ndsFighterMarioFoxResetFileSlots(void);
static void ndsFighterManagerClearLiveGObjs(void);
static sb32 ndsFighterStructIsPoolPointer(const void *ptr);
static sb32 ndsFighterStructIsTrackedPointer(const void *ptr);
static GObj *ndsFighterGetPlayerNumGObj(s32 player_num);

static sb32 sNdsFighterDashRunDamageStatusSetupActive;
static sb32 sNdsStageMPLiveHitStatusLoopDownBounceSetStatusActive;
static sb32 sNdsStageMPLiveHitStatusLoopCliffCatchSetStatusActive;
static sb32 sNdsFighterDashRunDamageFallSetStatusFromDamageActive;
static u32 sNdsFighterDashRunDamageFallFTMainSetStatusCount;
static sb32 sNdsFighterJumpAttackAirMapLandingActive;
static sb32 sNdsFighterPreviewLoopDisplayActive;
static u16 *sNdsFighterPreviewLoopPixels;
static u32 sNdsFighterPreviewLoopPitch;
static u32 sNdsFighterPreviewLoopDrawFrameIndex;
static sb32 sNdsFighterGCDrawAllLoopDisplayActive;
static u16 *sNdsFighterGCDrawAllLoopPixels;
static u32 sNdsFighterGCDrawAllLoopPitch;
static sb32 sNdsStageMPLiveStaleFloorLoopProbeActive = FALSE;
static sb32 sNdsStageMPCliffStatusFloorLoopStatusActive = FALSE;
static sb32 sNdsStageMPCliffTickFloorLoopStatusActive = FALSE;
static sb32 sNdsStageMPFallLandFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCeilStatusFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffCatchFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffWaitFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffAttackFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffClimbFloorLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffClimbFloorLoopRecatchSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffClimbActionLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffClimbFinishLoopUpdateActive = FALSE;
static sb32 sNdsStageMPPassInputLoopStatusActive = FALSE;
static sb32 sNdsStageInishieScaleSourceSetupActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDownWaitSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopDownStandSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveStandSetStatusActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveSetStatusActive = FALSE;
static sb32 sNdsStageMPPassiveLoopBranchProbeActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveStandBActive = FALSE;
static sb32 sNdsStageMPPassiveLoopDamageFallMapActive = FALSE;
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
static sb32 sNdsStageMPPassiveLoopReboundActive = FALSE;
static sb32 sNdsStageMPPassiveLoopReboundUpdateActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveStandCallbackActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveCallbackActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveStandUpdateActive = FALSE;
static sb32 sNdsStageMPPassiveLoopPassiveUpdateActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownWaitSetStatusActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownStandSetStatusActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownAttackSetStatusActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopRollForwardProbeActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopAttackUpdateActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopRollForwardUpdateActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopRollBackUpdateActive = FALSE;
static sb32 sNdsStageMPDownWaitLoopDownStandUpdateActive = FALSE;
static sb32 sNdsStageTurnLoopSetStatusActive = FALSE;
static sb32 sNdsStageTurnLoopFinalUpdateActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownWaitSetStatusActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownStandSetStatusActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownAttackSetStatusActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopRollForwardProbeActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopDownStandUpdateActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopAttackUpdateActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopRollForwardUpdateActive = FALSE;
static sb32 sNdsStageMPDownRecoverLoopRollBackUpdateActive = FALSE;
static sb32 sNdsStageMPCliffAttackActionLoopSetStatusActive = FALSE;
static sb32 sNdsStageMPCliffEscapeActionLoopSetStatusActive = FALSE;
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

static sb32 sNdsFTMainSetStatusCompatReplayActive = FALSE;
static void ndsRelocNormalizeMObjSubWordSwapped(MObjSub *mobjsub);
static void ndsRendererAdapterResetSceneCaches(void);
static void ndsRendererAdapterPrepareWorldCaches(void);

/* Every write to `gMPCollisionGeometry` goes through this so the vertex/extent
 * caches keyed on it are dropped at the moment the data they describe stops
 * being current. Defined with those caches in `reloc_backend_mp_collision.c`,
 * declared here because the shims below are included first. */
static void ndsMPCollisionSetGeometry(MPGeometryData *geometry);

/* Compatibility/proof shims. */
#include "reloc_backend_compat_shims.c"
#include "reloc_backend_shims_fighter_status.c"
#include "reloc_backend_shims_fighter_effects.c"
#include "reloc_backend_shims_fighter_parts.c"
#include "reloc_backend_shims_map_collision.c"

/* Relocation asset loading: the loaded-file registry, then its tables, file
 * IDs, pointer resolution, normalizers, symbols and extern trees, the motion
 * cache and the force loads. */
#include "reloc_backend_assets.c"
#include "reloc_backend_symbol_tables.c"
#include "reloc_backend_asset_ids.c"
#include "reloc_backend_resolve.c"
#include "reloc_backend_normalize.c"
#include "reloc_backend_extern_trees.c"
#include "reloc_backend_motion_cache.c"
#include "reloc_backend_force_load.c"

/* Fighter model/struct proofs. */
#include "reloc_backend_fighter_model.c"

/* Renderer/DL helpers. */
#include "reloc_backend_renderer_dl.c"

/* Movement proofs. */
#include "reloc_backend_movement.c"
#include "reloc_backend_movement_moveset.c"
#include "reloc_backend_stage_draw_loop.c"

/* MP collision proofs. */
#include "reloc_backend_mp_collision.c"
#include "reloc_backend_mp_lines.c"
#include "reloc_backend_mp_walls.c"
#include "reloc_backend_mp_floors.c"

/* Cliff/ledge proofs. */
#include "reloc_backend_cliff_ledge.c"

/* Diagnostic recorders. */
#include "reloc_backend_diagnostic_recorders.c"
