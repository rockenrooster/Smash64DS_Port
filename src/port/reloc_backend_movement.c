/* Fighter movement for the lab drivers and proofs: the battle-playable and
 * natural-combat gates, combat statuses (guard, reflector, projectile,
 * tilts, recovery) and the recorded pairs. A slice of reloc_backend.c; its
 * slices follow it there: movement_moveset, stage_draw_loop. */
#include "nds_scene_harness_config.h"
#include <nds/nds_effects.h>
#include <nds/nds_scene_harness.h>
#include <nds/nds_scene_manager.h>
#include <nds/nds_itcm_placement.h>
#include <nds/nds_p5.h>
#include <it/item.h>
/* The baked fireball texels/palettes and the GL_RGB16 upload they need. Only
 * this flag's path uses either, so neither is pulled in unconditionally. */
#include <nds/generated/nds_particle_banks.generated.h>
#include <nds/nds_renderer.h>
#include <sc/scene.h>
/* R01-B: the Results winner-emblem series roots, generated from the source
 * selection tables by scripts/menus/generate_nds_native_vs_emblem.py. */
#include <nds/generated/nds_native_vs_emblem.generated.h>
#include <nds/generated/nds_native_actor_tarucann.generated.h>
/* YOSTER-CLOUDS: route beside IsTaruCann. */
#if NDS_STAGE_YOSTER
#include <nds/nds_native_actor_yoster_cloud.h>
#endif
/* GROUND-ACTORS: Ground-kind stage actors with source display callbacks the
 * classify branch rejects (it admits only layer + Pupupu map GObjs). Each
 * imported stage TU owns a pure GObj-pointer accessor; the recognizers below
 * pair it with ground-kind/gkind/link checks after the IsTaruCann pattern. */
#if NDS_STAGE_ZEBES
extern void *ndsGRZebesAcidGObj(void);
#endif
#if NDS_STAGE_SECTOR
extern void *ndsGRSectorArwingGObj(void);
#endif
#if NDS_STAGE_YAMABUKI
extern void *ndsGRYamabukiGateGObj(void);
#endif
#if NDS_STAGE_INISHIE
extern void *ndsGRInishieScaleStringGObj(u32 index);
extern void *ndsGRInishieScalePlatformGObj(u32 index);
#endif


extern void ndsIFCommonRecordHUDState(void);
























#define NDS_PROCESS_LOOP_SNAPSHOT_U32(X) \
    X(gNdsFighterMarioFoxProcessLoopResult) \
    X(gNdsFighterMarioFoxProcessLoopSafeResult) \
    X(gNdsFighterMarioFoxProcessLoopMask) \
    X(gNdsFighterMarioFoxProcessLoopDeferredMask) \
    X(gNdsFighterMarioFoxProcessLoopCount) \
    X(gNdsFighterProcessLoopFrameMax) \
    X(gNdsFighterProcessLoopP0FrameCount) \
    X(gNdsFighterProcessLoopP1FrameCount) \
    X(gNdsFighterProcessLoopP0Completed) \
    X(gNdsFighterProcessLoopP1Completed) \
    X(gNdsFighterProcessLoopP0StatusVisitMask) \
    X(gNdsFighterProcessLoopP1StatusVisitMask) \
    X(gNdsFighterProcessLoopP0TransitionMask) \
    X(gNdsFighterProcessLoopP1TransitionMask) \
    X(gNdsFighterProcessLoopP0InputApplyCount) \
    X(gNdsFighterProcessLoopP1InputApplyCount) \
    X(gNdsFighterProcessLoopControllerBridgeCount) \
    X(gNdsFighterProcessLoopControllerMirrorCount) \
    X(gNdsFighterProcessLoopP0ButtonTapMask) \
    X(gNdsFighterProcessLoopP1ButtonTapMask) \
    X(gNdsFighterProcessLoopP0UpdateCount) \
    X(gNdsFighterProcessLoopP1UpdateCount) \
    X(gNdsFighterProcessLoopP0InterruptCount) \
    X(gNdsFighterProcessLoopP1InterruptCount) \
    X(gNdsFighterProcessLoopP0PhysicsCount) \
    X(gNdsFighterProcessLoopP1PhysicsCount) \
    X(gNdsFighterProcessLoopP0IntegrateCount) \
    X(gNdsFighterProcessLoopP1IntegrateCount) \
    X(gNdsFighterProcessLoopP0MapCount) \
    X(gNdsFighterProcessLoopP1MapCount) \
    X(gNdsFighterProcessLoopP0WaitVisitCount) \
    X(gNdsFighterProcessLoopP1WaitVisitCount) \
    X(gNdsFighterProcessLoopP0WalkVisitCount) \
    X(gNdsFighterProcessLoopP1WalkVisitCount) \
    X(gNdsFighterProcessLoopP0DashVisitCount) \
    X(gNdsFighterProcessLoopP1DashVisitCount) \
    X(gNdsFighterProcessLoopP0RunVisitCount) \
    X(gNdsFighterProcessLoopP1RunVisitCount) \
    X(gNdsFighterProcessLoopP0RunBrakeVisitCount) \
    X(gNdsFighterProcessLoopP1RunBrakeVisitCount) \
    X(gNdsFighterProcessLoopP0KneeBendVisitCount) \
    X(gNdsFighterProcessLoopP1KneeBendVisitCount) \
    X(gNdsFighterProcessLoopP0JumpVisitCount) \
    X(gNdsFighterProcessLoopP1JumpVisitCount) \
    X(gNdsFighterProcessLoopP0FallVisitCount) \
    X(gNdsFighterProcessLoopP1FallVisitCount) \
    X(gNdsFighterProcessLoopP0LandingVisitCount) \
    X(gNdsFighterProcessLoopP1LandingVisitCount) \
    X(gNdsFighterProcessLoopP0StatusStart) \
    X(gNdsFighterProcessLoopP1StatusStart) \
    X(gNdsFighterProcessLoopP0MotionStart) \
    X(gNdsFighterProcessLoopP1MotionStart) \
    X(gNdsFighterProcessLoopP0StatusFinal) \
    X(gNdsFighterProcessLoopP1StatusFinal) \
    X(gNdsFighterProcessLoopP0MotionFinal) \
    X(gNdsFighterProcessLoopP1MotionFinal) \
    X(gNdsFighterProcessLoopP0GAFinal) \
    X(gNdsFighterProcessLoopP1GAFinal) \
    X(gNdsFighterProcessLoopP0RootDirectionOK) \
    X(gNdsFighterProcessLoopP1RootDirectionOK) \
    X(gNdsFighterProcessLoopP0FloorOK) \
    X(gNdsFighterProcessLoopP1FloorOK) \
    X(gNdsFighterProcessLoopFallDetectCount) \
    X(gNdsFighterProcessLoopLandingDetectCount) \
    X(gNdsFighterProcessLoopSetGroundCount) \
    X(gNdsFighterProcessLoopSetAirCount) \
    X(gNdsFighterProcessLoopWaitSetStatusCount) \
    X(gNdsFighterProcessLoopRunBrakeEndCount) \
    X(gNdsFighterProcessLoopJumpAnimEndCount) \
    X(gNdsFighterProcessLoopLandingEndCount) \
    X(gNdsFighterProcessLoopDeferredInterruptCheckCount) \
    X(gNdsFighterProcessLoopGObjDelta) \
    X(gNdsFighterProcessLoopUnexpectedStatusCount) \
    X(gNdsFighterProcessLoopDeniedStatusCount) \
    X(gNdsFighterProcessLoopProcessAttachCount) \
    X(gNdsFighterProcessLoopDisplayProbeCount) \
    X(gNdsFighterProcessLoopGameplayUpdateCount) \
    X(gNdsFighterProcessLoopDrawCallCount) \
    X(gNdsFighterProcessLoopMatrixCallCount) \
    X(gNdsFighterProcessLoopRootYDriftCount) \
    X(gNdsFighterProcessLoopGADriftCount)

#define NDS_PROCESS_LOOP_SNAPSHOT_S32(X) \
    X(gNdsFighterProcessLoopP0LastStickX) \
    X(gNdsFighterProcessLoopP1LastStickX) \
    X(gNdsFighterProcessLoopP0FloorYMilli) \
    X(gNdsFighterProcessLoopP1FloorYMilli) \
    X(gNdsFighterProcessLoopP0RootXStartMilli) \
    X(gNdsFighterProcessLoopP1RootXStartMilli) \
    X(gNdsFighterProcessLoopP0RootXFinalMilli) \
    X(gNdsFighterProcessLoopP1RootXFinalMilli) \
    X(gNdsFighterProcessLoopP0RootDeltaXMilli) \
    X(gNdsFighterProcessLoopP1RootDeltaXMilli) \
    X(gNdsFighterProcessLoopP0RootYFinalMilli) \
    X(gNdsFighterProcessLoopP1RootYFinalMilli) \
    X(gNdsFighterProcessLoopP0RootRiseMilli) \
    X(gNdsFighterProcessLoopP1RootRiseMilli) \
    X(gNdsFighterProcessLoopP0GroundVelFinalMilli) \
    X(gNdsFighterProcessLoopP1GroundVelFinalMilli) \
    X(gNdsFighterProcessLoopP0AirVelXFinalMilli) \
    X(gNdsFighterProcessLoopP1AirVelXFinalMilli) \
    X(gNdsFighterProcessLoopP0AirVelYFinalMilli) \
    X(gNdsFighterProcessLoopP1AirVelYFinalMilli)

typedef struct NDSFighterProcessLoopSnapshot
{
#define NDS_SNAPSHOT_U32_FIELD(name) u32 name;
#define NDS_SNAPSHOT_S32_FIELD(name) s32 name;
    NDS_PROCESS_LOOP_SNAPSHOT_U32(NDS_SNAPSHOT_U32_FIELD)
    NDS_PROCESS_LOOP_SNAPSHOT_S32(NDS_SNAPSHOT_S32_FIELD)
#undef NDS_SNAPSHOT_U32_FIELD
#undef NDS_SNAPSHOT_S32_FIELD
} NDSFighterProcessLoopSnapshot;


#define NDS_FIGHTER_PREVIEW_LOOP_WIDTH 96u
#define NDS_FIGHTER_PREVIEW_LOOP_HEIGHT 72u
#define NDS_FIGHTER_GCDRAWALL_LOOP_WIDTH 96u
#define NDS_FIGHTER_GCDRAWALL_LOOP_HEIGHT 72u

#define NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED 300u
#define NDS_FIGHTER_NATURAL_MOTION_WALK_FRAMES_REQUIRED 8u
#define NDS_FIGHTER_NATURAL_STAGE_SIDE_MASK_REQUIRED 0x24fu
#define NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED 10u
#define NDS_FIGHTER_NATURAL_COMBAT_DASH_FRAMES_REQUIRED 2u
#define NDS_FIGHTER_NATURAL_COMBAT_RUN_FRAMES_REQUIRED 8u
#define NDS_FIGHTER_NATURAL_COMBAT_RUNBRAKE_FRAMES_REQUIRED 2u
#define NDS_FIGHTER_NATURAL_COMBAT_TURN_FRAMES_REQUIRED 1u
#define NDS_FIGHTER_NATURAL_COMBAT_GUARD_FRAMES_REQUIRED 10u
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_STOP_RANGE 240.0F
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE 100.0F
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_STEP 15.0F
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_MIN 50.0F
#define NDS_FIGHTER_NATURAL_MOVESET_SAFE_RANGE 80.0F
#define NDS_FIGHTER_NATURAL_MOVESET_GRAB_STOP_RANGE (260.0F + 30.0F)
#define NDS_FIGHTER_NATURAL_PROJECTILE_STOP_RANGE \
    ((112.5F * 2.0F) + 350.0F)
#define NDS_FIGHTER_NATURAL_PROJECTILE_CENTER_RANGE 300.0F
#define NDS_FIGHTER_NATURAL_COMBAT_ATTACK_TIMEOUT 45u
#define NDS_FIGHTER_NATURAL_COMBAT_ATTACK_RETRY_MAX 6u
#define NDS_FIGHTER_PROJECTILE_FIRE_TIMEOUT 120u
#define NDS_FIGHTER_PROJECTILE_OBSERVE_TIMEOUT 180u
#define NDS_FIGHTER_PROJECTILE_WEAPON_FRAMES_REQUIRED 3u
#define NDS_FIGHTER_REFLECTOR_PROOF_PASS 0x52464c43u
#define NDS_FIGHTER_NATURAL_COMBAT_PHASE_TIMEOUT 600u
#define NDS_FIGHTER_BATTLE_PLAYABLE_PHASE_TIMEOUT 3600u
#define NDS_FIGHTER_NATURAL_MOVESET_PHASE_TIMEOUT 1200u
#define NDS_FIGHTER_BATTLE_PLAYABLE_WAIT_AFTER_REBIRTH_REQUIRED 8u
#define NDS_FIGHTER_BATTLE_PLAYABLE_MASK_ALL 0xffu
#define NDS_FIGHTER_SPECIALS_MARIO_HI_MASK 0x000fu
#define NDS_FIGHTER_SPECIALS_MARIO_LW_MASK 0x0070u
#define NDS_FIGHTER_SPECIALS_FOX_HI_MASK 0x0f80u
#define NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED 60u
#if NDS_DONKEY
#define NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED 2u
#define NDS_FIGHTER_DONKEY_GIANTPUNCH_FULL_CHARGE_REQUIRED 10u
/* nEFKindChargeSparkle == 73, recorded in effect-kind mask lane 2 (64..95). */
#define NDS_FIGHTER_DONKEY_CHARGE_SPARKLE_MASK2 (1u << (73u - 64u))
#define NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED 10u
#endif
/* BattleShip ftsamus.h:8. Keep the source constant local to this proof rather
 * than importing ftcomputer.h solely for its duplicate convenience macro. */
#define NDS_FIGHTER_SAMUS_CHARGE_MAX 7u

/* Scripted input phases for the natural original-runtime combat chain.
 * Input only flows through controller playback into the original
 * syController/ftkey path; state is observed, never written. */
enum {
    nNDSNaturalCombatPhaseWait = 0,
    nNDSNaturalCombatPhaseWalk,
    nNDSNaturalCombatPhaseSettleWalk,
    nNDSNaturalCombatPhaseDashRun,
    nNDSNaturalCombatPhaseRunBrake,
    nNDSNaturalCombatPhaseSettleRun,
    nNDSNaturalCombatPhaseTurn,
    nNDSNaturalCombatPhaseSettleTurn,
    nNDSNaturalCombatPhaseApproach,
    nNDSNaturalCombatPhaseSettleApproach,
    nNDSNaturalCombatPhaseAttack,
    nNDSNaturalCombatPhaseSettleDamage,
    nNDSNaturalCombatPhaseGuard,
    nNDSNaturalCombatPhaseGuardOff,
    nNDSNaturalCombatPhaseDone,
    nNDSNaturalCombatPhaseBattlePlayableKOExit,
    nNDSNaturalCombatPhaseBattlePlayableDead,
    nNDSNaturalCombatPhaseBattlePlayableRebirth,
    nNDSNaturalCombatPhaseBattlePlayableRecover,
    nNDSNaturalCombatPhaseBattlePlayableDone,
    nNDSNaturalCombatPhaseProjectileSettle,
    nNDSNaturalCombatPhaseProjectileFire,
    nNDSNaturalCombatPhaseProjectileObserve,
    /* Source-path regression guard for BUGS.md Samus shield-roll report.
     * Keep these appended so all existing proof phase IDs remain stable. */
    nNDSNaturalCombatPhaseGuardRoll,
    nNDSNaturalCombatPhaseSettleGuardRoll,
    /* BattleShip's Appeal is a common-status L-trigger interrupt.  This proof
     * phase supplies only that source input; it never writes fighter status. */
    nNDSNaturalCombatPhaseAppeal,
    nNDSNaturalCombatPhaseSettleAppeal
};

enum {
    nNDSNaturalMovesetPhaseIdle = 0,
    nNDSNaturalMovesetPhaseTiltS3,
    nNDSNaturalMovesetPhaseSettleTiltS3,
    nNDSNaturalMovesetPhaseTiltHi3,
    nNDSNaturalMovesetPhaseSettleTiltHi3,
    nNDSNaturalMovesetPhaseTiltLw3,
    nNDSNaturalMovesetPhaseSettleTiltLw3,
    nNDSNaturalMovesetPhaseSmashS4,
    nNDSNaturalMovesetPhaseSettleSmashS4,
    nNDSNaturalMovesetPhaseAerialJump,
    nNDSNaturalMovesetPhaseAerialAttack,
    nNDSNaturalMovesetPhaseSettleAerial,
    nNDSNaturalMovesetPhaseGrabCatch,
    nNDSNaturalMovesetPhaseGrabThrow,
    nNDSNaturalMovesetPhaseSettleThrow,
    nNDSNaturalMovesetPhaseDone
};





enum {
    nNDSNaturalSpecialsPhaseIdle = 0,
    nNDSNaturalSpecialsPhaseMarioHi,
    nNDSNaturalSpecialsPhaseSettleMarioHi,
    nNDSNaturalSpecialsPhaseMarioLw,
    nNDSNaturalSpecialsPhaseSettleMarioLw,
    nNDSNaturalSpecialsPhaseFoxHi,
    nNDSNaturalSpecialsPhaseSettleFoxHi,
    nNDSNaturalSpecialsPhaseDone,
#if NDS_DONKEY
    /* Preserve the established 0..7 phase values consumed by the legacy
     * verifier; DK extends the same natural-input sequence after Done. */
    nNDSNaturalSpecialsPhaseDonkeyNCharge,
    nNDSNaturalSpecialsPhaseDonkeyNRelease,
    nNDSNaturalSpecialsPhaseDonkeyHi,
    nNDSNaturalSpecialsPhaseDonkeyLw,
    /* Appended so the established DK phase numbers above remain stable. */
    nNDSNaturalSpecialsPhaseDonkeyNFullCharge,
#endif
    /* Append after every established phase so legacy verifier values remain
     * stable. These are controller-driven legs in the existing proof. */
    nNDSNaturalSpecialsPhaseSamusNFullCharge,
    nNDSNaturalSpecialsPhaseSamusLw
};

typedef struct NDSFighterNaturalMotionState {
    f32 first_wait_anim;
    f32 prev_anim;
    u32 has_wait_anim;
    u32 wait_frames;
    u32 anim_advance_count;
    u32 valid_joint_count;
    u32 walk_frames;
    u32 dash_frames;
    u32 run_frames;
    u32 runbrake_frames;
    u32 turn_frames;
    u32 hitlag_frames;
} NDSFighterNaturalMotionState;

static NDSFighterNaturalMotionState sNdsFighterNaturalMotionStates[2];
static u32 sNdsFighterNaturalMotionWalkInputActive;
static u32 sNdsNaturalCombatPhase;
static u32 sNdsNaturalCombatPhaseFrames;
static u32 sNdsNaturalCombatSettleFrames;
static u32 sNdsNaturalCombatAttackerSlot;
static u32 sNdsNaturalCombatVictimSlot;
static u32 sNdsNaturalCombatAttackFrames;
static u32 sNdsNaturalCombatAttackPressed;
static u32 sNdsNaturalCombatPassPressed;
static u32 sNdsNaturalCombatRollPressed;
static u32 sNdsNaturalCombatAppealPressed;
static f32 sNdsNaturalCombatApproachStopRange;
/* Arrival-by-contact tracker (P2-3r3): frames the driven approach dx has not
 * shrunk, and the last |dx| it measured. */
static u32 sNdsNaturalCombatApproachStagnant;
static f32 sNdsNaturalCombatApproachLastDX;
static u32 sNdsNaturalCombatVictimStartPercent;
static f32 sNdsNaturalCombatVictimHitPosX;
static u32 sNdsNaturalCombatVictimHitSeen;
static u32 sNdsBattlePlayableVictimStockStart;
static u32 sNdsBattlePlayableBattleStockStart;
static u32 sNdsBattlePlayableFallsStart;
static u32 sNdsBattlePlayableRebirthSeen;
static s8 sNdsBattlePlayableKOStickX;
static u32 sNdsNaturalProjectileActorSlot;
static u32 sNdsNaturalProjectileButtonPressed;
static u32 sNdsNaturalProjectileExpectedKind;
static u32 sNdsNaturalProjectileKORecoveryActive;
static u32 sNdsNaturalReflectorFoxSlot;
static u32 sNdsNaturalReflectorProjectileSlot;
static u32 sNdsNaturalReflectorButtonPressed;
static u32 sNdsNaturalMovesetPhase;
static u32 sNdsNaturalMovesetPhaseFrames;
static u32 sNdsNaturalMovesetSettleFrames;
static u32 sNdsNaturalMovesetDone;
static u32 sNdsNaturalMovesetKORecoveryActive;
static u32 sNdsNaturalMovesetKORecoveryPhase;

static u32 sNdsNaturalSpecialsPhase;
static u32 sNdsNaturalSpecialsPhaseFrames;
static u32 sNdsNaturalSpecialsDone;
static u32 sNdsNaturalSpecialsButtonPressed;

static sb32 ndsFighterBattlePlayableProofEnabled(void)
{
#if NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE
    return FALSE;
#else
    return FALSE;
#endif
}

static sb32 ndsFighterNaturalProjectileProofEnabled(void)
{
    return (ndsFighterBattlePlayableProofEnabled() != FALSE) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalReflectorProofEnabled(void)
{
    /* P2-3r3: the reflector proof needs a second, projectile-capable fighter
     * to shoot INTO the reflector -- the source arrangement is Mario firing
     * while Fox shines. On a build with no Mario-family fighter the projectile
     * actor IS the reflector Fox, the two roles collapse onto one pad, the
     * down-B hijacks the fire phase, the blaster never fires, and the
     * projectile stage times out into a stall. With one slot the stage is
     * structurally impossible, so disable it cleanly rather than half-run it. */
    return ((ndsFighterNaturalProjectileProofEnabled() != FALSE) &&
            (sNdsNaturalReflectorFoxSlot !=
             sNdsNaturalProjectileActorSlot)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalSpecialsProofEnabled(void)
{
    return (ndsFighterBattlePlayableProofEnabled() != FALSE) ? TRUE : FALSE;
}

static sb32 ndsFighterMarioFoxNaturalMotionProofEnabled(void)
{
    if (ndsFighterBattlePlayableProofEnabled() != FALSE)
    {
        return TRUE;
    }
    return FALSE;
}

static sb32 ndsFighterNaturalCombatMovementOnlyProofEnabled(void)
{
    return FALSE;
}

static sb32 ndsFighterNaturalCombatLiveHitProofEnabled(void)
{
    return FALSE;
}

static sb32 ndsFighterNaturalCombatStageSideProofEnabled(void)
{
    return FALSE;
}

static sb32 ndsFighterNaturalMotionStatusIsWalk(s32 status_id)
{
    return ((status_id == nFTCommonStatusWalkSlow) ||
            (status_id == nFTCommonStatusWalkMiddle) ||
            (status_id == nFTCommonStatusWalkFast)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMotionMotionIsWalk(s32 motion_id)
{
    return ((motion_id == nFTCommonMotionWalkSlow) ||
            (motion_id == nFTCommonMotionWalkMiddle) ||
            (motion_id == nFTCommonMotionWalkFast)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMotionHasValidJoints(FTStruct *fp)
{
    return ((fp != NULL) &&
            (fp->fighter_gobj != NULL) &&
            (fp->joints[nFTPartsJointTopN] != NULL) &&
            (fp->joints[nFTPartsJointCommonStart] != NULL)) ? TRUE : FALSE;
}

static void ndsFighterNaturalMotionPauseNonTargetVisitor(GObj *gobj,
                                                         u32 param)
{
    GObj *target0 = ndsFighterManagerLiveGObj(0u);
    GObj *target1 = ndsFighterManagerLiveGObj(1u);
    (void)param;

    if (gobj == NULL)
    {
        return;
    }
    if ((gobj == target0) || (gobj == target1))
    {
        return;
    }
    {
        extern GObj *gGMCameraGObj;

        if (gobj == gGMCameraGObj)
        {
            return;
        }
    }
    if (gobj->gobjproc_head != NULL)
    {
        gcPauseGObjProcessAll(gobj);
    }
    gobj->flags |= GOBJ_FLAG_NORUN;
}

static void ndsFighterNaturalMotionRecordSlot(u32 slot, FTStruct *fp)
{
    NDSFighterNaturalMotionState *state;
    f32 anim_frame;

    if ((slot >= ARRAY_COUNT(sNdsFighterNaturalMotionStates)) ||
        (fp == NULL) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsFighterNaturalMotionUnsafeCount++;
        return;
    }
    state = &sNdsFighterNaturalMotionStates[slot];
    anim_frame = fp->fighter_gobj->anim_frame;

    if (slot == 0u)
    {
        gNdsFighterNaturalMotionP0StatusFinal = (u32)fp->status_id;
        gNdsFighterNaturalMotionP0MotionFinal = (u32)fp->motion_id;
        gNdsFighterNaturalMotionP0GAFinal = (u32)fp->ga;
        gNdsFighterNaturalMotionP0AnimFinalBits =
            ndsFloatBits(anim_frame);
    }
    else
    {
        gNdsFighterNaturalMotionP1StatusFinal = (u32)fp->status_id;
        gNdsFighterNaturalMotionP1MotionFinal = (u32)fp->motion_id;
        gNdsFighterNaturalMotionP1GAFinal = (u32)fp->ga;
        gNdsFighterNaturalMotionP1AnimFinalBits =
            ndsFloatBits(anim_frame);
    }

    if (ndsFighterNaturalMotionHasValidJoints(fp) != FALSE)
    {
        state->valid_joint_count++;
    }
    if ((fp->status_id == nFTCommonStatusWait) &&
        (fp->motion_id == nFTCommonMotionWait))
    {
        if (state->has_wait_anim == 0u)
        {
            state->first_wait_anim = anim_frame;
            state->prev_anim = anim_frame;
            state->has_wait_anim = 1u;
            if (slot == 0u)
            {
                gNdsFighterNaturalMotionP0AnimStartBits =
                    ndsFloatBits(anim_frame);
            }
            else
            {
                gNdsFighterNaturalMotionP1AnimStartBits =
                    ndsFloatBits(anim_frame);
            }
        }
        else if (anim_frame != state->prev_anim)
        {
            state->anim_advance_count++;
            state->prev_anim = anim_frame;
        }
        state->wait_frames++;
    }
    else if ((sNdsFighterNaturalMotionWalkInputActive != 0u) &&
             (ndsFighterNaturalMotionStatusIsWalk(fp->status_id) != FALSE) &&
             (ndsFighterNaturalMotionMotionIsWalk(fp->motion_id) != FALSE))
    {
        state->walk_frames++;
        if (slot == 0u)
        {
            gNdsFighterNaturalMotionP0WalkStatus = (u32)fp->status_id;
            gNdsFighterNaturalMotionP0WalkMotion = (u32)fp->motion_id;
        }
        else
        {
            gNdsFighterNaturalMotionP1WalkStatus = (u32)fp->status_id;
            gNdsFighterNaturalMotionP1WalkMotion = (u32)fp->motion_id;
        }
    }

    if (fp->status_id == nFTCommonStatusDash)
    {
        state->dash_frames++;
    }
    else if (fp->status_id == nFTCommonStatusRun)
    {
        state->run_frames++;
    }
    else if (fp->status_id == nFTCommonStatusRunBrake)
    {
        state->runbrake_frames++;
    }
    else if ((fp->status_id == nFTCommonStatusTurn) ||
             (fp->status_id == nFTCommonStatusTurnRun))
    {
        state->turn_frames++;
    }
    if (fp->hitlag_tics > 0u)
    {
        state->hitlag_frames++;
    }

    if (slot == 0u)
    {
        gNdsFighterNaturalMotionP0WaitFrameCount = state->wait_frames;
        gNdsFighterNaturalMotionP0AnimAdvanceCount =
            state->anim_advance_count;
        gNdsFighterNaturalMotionP0ValidJointCount =
            state->valid_joint_count;
        gNdsFighterNaturalMotionP0WalkFrameCount = state->walk_frames;
        gNdsFighterNaturalCombatP0DashFrames = state->dash_frames;
        gNdsFighterNaturalCombatP0RunFrames = state->run_frames;
        gNdsFighterNaturalCombatP0RunBrakeFrames = state->runbrake_frames;
        gNdsFighterNaturalCombatP0TurnFrames = state->turn_frames;
        gNdsFighterNaturalCombatP0HitlagFrames = state->hitlag_frames;
    }
    else
    {
        gNdsFighterNaturalMotionP1WaitFrameCount = state->wait_frames;
        gNdsFighterNaturalMotionP1AnimAdvanceCount =
            state->anim_advance_count;
        gNdsFighterNaturalMotionP1ValidJointCount =
            state->valid_joint_count;
        gNdsFighterNaturalMotionP1WalkFrameCount = state->walk_frames;
        gNdsFighterNaturalCombatP1DashFrames = state->dash_frames;
        gNdsFighterNaturalCombatP1RunFrames = state->run_frames;
        gNdsFighterNaturalCombatP1RunBrakeFrames = state->runbrake_frames;
        gNdsFighterNaturalCombatP1TurnFrames = state->turn_frames;
        gNdsFighterNaturalCombatP1HitlagFrames = state->hitlag_frames;
    }
}

static f32 ndsFighterNaturalCombatPosX(FTStruct *fp)
{
    if ((fp == NULL) || (fp->coll_data.p_translate == NULL))
    {
        return 0.0F;
    }
    return fp->coll_data.p_translate->x;
}

static sb32 ndsFighterNaturalCombatStatusIsDamage(s32 status_id)
{
    return ((status_id >= nFTCommonStatusDamageStart) &&
            (status_id <= nFTCommonStatusDamageEnd)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalCombatStatusIsGuard(s32 status_id)
{
    return ((status_id >= nFTCommonStatusGuardStart) &&
            (status_id <= nFTCommonStatusGuardEnd)) ? TRUE : FALSE;
}

/* BattleShip's Luigi special-status table deliberately points N/Hi/Lw at the
 * Mario state-machine callbacks while retaining Luigi motion/event/attribute
 * data. Treat that source family as one proof-owner selection without aliasing
 * the status constants themselves: the predicates below still dispatch on the
 * actual fighter kind and compare that fighter's enum. */
static sb32 ndsFighterNaturalIsMarioSpecialFamily(const FTStruct *fp)
{
    if (fp == NULL)
    {
        return FALSE;
    }
    if (fp->fkind == nFTKindMario)
    {
        return TRUE;
    }
#if NDS_LUIGI
    if (fp->fkind == nFTKindLuigi)
    {
        return TRUE;
    }
#endif
    return FALSE;
}

#if NDS_DONKEY
static sb32 ndsFighterNaturalIsDonkey(const FTStruct *fp)
{
    return ((fp != NULL) && (fp->fkind == nFTKindDonkey)) ? TRUE : FALSE;
}
#endif

static sb32 ndsFighterNaturalIsSamus(const FTStruct *fp)
{
    return ((fp != NULL) &&
            ((fp->fkind == nFTKindSamus) || (fp->fkind == nFTKindNSamus))) ?
        TRUE : FALSE;
}

static u32 ndsFighterNaturalProjectileSelectSlot(FTStruct *fp[2])
{
    u32 i;

    for (i = 0u; i < 2u; i++)
    {
        if (ndsFighterNaturalIsMarioSpecialFamily(fp[i]) != FALSE)
        {
            return i;
        }
    }
    for (i = 0u; i < 2u; i++)
    {
        if (fp[i]->fkind == nFTKindFox)
        {
            return i;
        }
    }
    for (i = 0u; i < 2u; i++)
    {
        if (ndsFighterNaturalIsMarioSpecialFamily(fp[i]) != FALSE)
        {
            return i;
        }
    }
    return sNdsNaturalCombatAttackerSlot;
}

static u32 ndsFighterNaturalReflectorSelectFoxSlot(FTStruct *fp[2])
{
    u32 i;

    for (i = 0u; i < 2u; i++)
    {
        if (fp[i]->fkind == nFTKindFox)
        {
            return i;
        }
    }
    return 1u - sNdsNaturalProjectileActorSlot;
}

static u32 ndsFighterNaturalProjectileExpectedKind(FTStruct *fp)
{
    if (ndsFighterNaturalIsMarioSpecialFamily(fp) != FALSE)
    {
        return nWPKindFireball;
    }
    if ((fp != NULL) && (fp->fkind == nFTKindFox))
    {
        return nWPKindBlaster;
    }
    return nWPKindFireball;
}

static sb32 ndsFighterNaturalProjectileStatusIsSpecialN(FTStruct *fp)
{
    if (fp == NULL)
    {
        return FALSE;
    }
    if (fp->fkind == nFTKindFox)
    {
        return ((fp->status_id == nFTFoxStatusSpecialN) ||
                (fp->status_id == nFTFoxStatusSpecialAirN)) ? TRUE : FALSE;
    }
    if (fp->fkind == nFTKindMario)
    {
        return ((fp->status_id == nFTMarioStatusSpecialN) ||
                (fp->status_id == nFTMarioStatusSpecialAirN)) ? TRUE : FALSE;
    }
#if NDS_LUIGI
    if (fp->fkind == nFTKindLuigi)
    {
        return ((fp->status_id == nFTLuigiStatusSpecialN) ||
                (fp->status_id == nFTLuigiStatusSpecialAirN)) ? TRUE : FALSE;
    }
#endif
    return FALSE;
}

static sb32 ndsFighterNaturalMarioFamilyStatusIsSpecialHi(FTStruct *fp)
{
    if (fp == NULL)
    {
        return FALSE;
    }
    if (fp->fkind == nFTKindMario)
    {
        return ((fp->status_id == nFTMarioStatusSpecialHi) ||
                (fp->status_id == nFTMarioStatusSpecialAirHi)) ? TRUE : FALSE;
    }
#if NDS_LUIGI
    if (fp->fkind == nFTKindLuigi)
    {
        return ((fp->status_id == nFTLuigiStatusSpecialHi) ||
                (fp->status_id == nFTLuigiStatusSpecialAirHi)) ? TRUE : FALSE;
    }
#endif
    return FALSE;
}

static sb32 ndsFighterNaturalMarioFamilyStatusIsSpecialLw(FTStruct *fp)
{
    if (fp == NULL)
    {
        return FALSE;
    }
    if (fp->fkind == nFTKindMario)
    {
        return ((fp->status_id == nFTMarioStatusSpecialLw) ||
                (fp->status_id == nFTMarioStatusSpecialAirLw)) ? TRUE : FALSE;
    }
#if NDS_LUIGI
    if (fp->fkind == nFTKindLuigi)
    {
        return ((fp->status_id == nFTLuigiStatusSpecialLw) ||
                (fp->status_id == nFTLuigiStatusSpecialAirLw)) ? TRUE : FALSE;
    }
#endif
    return FALSE;
}

static sb32 ndsFighterNaturalProjectileStatusIsSpecialLw(FTStruct *fp)
{
    if (fp == NULL)
    {
        return FALSE;
    }
    if (fp->fkind == nFTKindFox)
    {
        return ((fp->status_id == nFTFoxStatusSpecialLwStart) ||
                (fp->status_id == nFTFoxStatusSpecialLwHit) ||
                (fp->status_id == nFTFoxStatusSpecialLwEnd) ||
                (fp->status_id == nFTFoxStatusSpecialLwLoop) ||
                (fp->status_id == nFTFoxStatusSpecialLwTurn) ||
                (fp->status_id == nFTFoxStatusSpecialAirLwStart) ||
                (fp->status_id == nFTFoxStatusSpecialAirLwHit) ||
                (fp->status_id == nFTFoxStatusSpecialAirLwEnd) ||
                (fp->status_id == nFTFoxStatusSpecialAirLwLoop) ||
                (fp->status_id == nFTFoxStatusSpecialAirLwTurn)) ? TRUE :
                                                                  FALSE;
    }
    return FALSE;
}

static void ndsFighterNaturalReflectorRecordFireball(FTStruct *fox)
{
    GObj *weapon_gobj;
    DObj *fox_root;

    if (fox == NULL)
    {
        return;
    }
    if (fox->special_coll != NULL)
    {
        gNdsFighterReflectorProofSpecialSizeMilli =
            (u32)ndsFloatToMilliSigned(fox->special_coll->size.x);
        gNdsFighterReflectorProofSpecialResist =
            (u32)fox->special_coll->damage_resist;
    }
    weapon_gobj = gGCCommonLinks[nGCCommonLinkIDWeapon];
    while (weapon_gobj != NULL)
    {
        WPStruct *wp = wpGetStruct(weapon_gobj);

        if ((wp != NULL) && (wp->kind == nWPKindFireball))
        {
            gNdsFighterReflectorProofFireballCanReflect =
                (wp->attack_coll.can_reflect != FALSE) ? 1u : 0u;
            gNdsFighterReflectorProofFireballCanAbsorb =
                (wp->attack_coll.can_absorb != FALSE) ? 1u : 0u;
            gNdsFighterReflectorProofFireballCanShield =
                (wp->attack_coll.can_shield != FALSE) ? 1u : 0u;
            gNdsFighterReflectorProofFireballAttackCount =
                (u32)wp->attack_coll.attack_count;
            gNdsFighterReflectorProofFireballDamage =
                (u32)wp->attack_coll.damage;
            gNdsFighterReflectorProofFireballSizeMilli =
                (u32)ndsFloatToMilliSigned(wp->attack_coll.size);
            fox_root = DObjGetStruct(fox->fighter_gobj);
            if ((fox_root != NULL) && (DObjGetStruct(weapon_gobj) != NULL))
            {
                Vec3f *weapon_pos = &DObjGetStruct(weapon_gobj)->translate.vec.f;
                Vec3f *fox_pos = &fox_root->translate.vec.f;

                gNdsFighterReflectorProofFireballDXMilli =
                    ndsFloatToMilliSigned(weapon_pos->x - fox_pos->x);
                gNdsFighterReflectorProofFireballDYMilli =
                    ndsFloatToMilliSigned(weapon_pos->y - fox_pos->y);
            }
        }
        weapon_gobj = weapon_gobj->link_next;
    }
}

static void ndsFighterNaturalReflectorRecord(FTStruct *fp[2])
{
    FTStruct *fox;
    u32 mask;

    if (ndsFighterNaturalReflectorProofEnabled() == FALSE)
    {
        return;
    }
    fox = fp[sNdsNaturalReflectorFoxSlot];
    gNdsFighterReflectorProofFoxSlot = sNdsNaturalReflectorFoxSlot;
    gNdsFighterReflectorProofProjectileSlot =
        sNdsNaturalReflectorProjectileSlot;
    mask = gNdsFighterReflectorProofMask;
    ndsFighterNaturalReflectorRecordFireball(fox);
    if (gNdsFighterReflectorProofDownBPressFrames > 0u)
    {
        mask |= 1u << 0;
    }
    if (fox != NULL)
    {
        if ((fox->status_id == nFTFoxStatusSpecialLwStart) ||
            (fox->status_id == nFTFoxStatusSpecialAirLwStart))
        {
            NDS_DIAG(gNdsFighterReflectorProofStartFrames++);
        }
        if ((fox->status_id == nFTFoxStatusSpecialLwLoop) ||
            (fox->status_id == nFTFoxStatusSpecialAirLwLoop))
        {
            gNdsFighterReflectorProofLoopFrames++;
        }
        if ((fox->status_id == nFTFoxStatusSpecialLwHit) ||
            (fox->status_id == nFTFoxStatusSpecialAirLwHit))
        {
            gNdsFighterReflectorProofHitFrames++;
        }
        if (ndsFighterNaturalProjectileStatusIsSpecialLw(fox) != FALSE)
        {
            mask |= 1u << 1;
        }
        if ((fox->is_reflect != FALSE) &&
            (fox->special_coll != NULL) &&
            (fox->special_coll->kind == nFTSpecialCollKindFoxReflector))
        {
            NDS_DIAG(gNdsFighterReflectorProofIsReflectFrames++);
            mask |= 1u << 2;
        }
        if ((gNdsFighterReflectorProofHitSetCallCount > 0u) &&
            (gNdsFighterReflectorProofReflectLRBeforeHit != 0))
        {
            mask |= 1u << 3;
        }
        if (gNdsFighterReflectorProofHitFrames > 0u)
        {
            mask |= 1u << 4;
        }
        if ((gNdsFighterReflectorProofHitSetCallCount > 0u) &&
            (fox->reflect_lr == 0))
        {
            NDS_DIAG(gNdsFighterReflectorProofReflectLRClearFrames++);
            mask |= 1u << 7;
        }
    }
    if ((gNdsFighterReflectorProofFireballProcCount > 0u) &&
        (gNdsFighterReflectorProofFireballOwnerKind == nFTKindFox))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterReflectorProofFireballVelXBefore < 0) &&
         (gNdsFighterReflectorProofFireballVelXAfter > 0)) ||
        ((gNdsFighterReflectorProofFireballVelXBefore > 0) &&
         (gNdsFighterReflectorProofFireballVelXAfter < 0)))
    {
        mask |= 1u << 6;
    }
    gNdsFighterReflectorProofMask = mask;
    if ((mask & 0xffu) == 0xffu)
    {
        gNdsFighterReflectorProofResult =
            NDS_FIGHTER_REFLECTOR_PROOF_PASS;
    }
}

static void ndsFighterNaturalProjectileRecord(FTStruct *fp[2])
{
    GObj *weapon_gobj;
    FTStruct *actor;
    u32 mask;
    u32 count = 0u;

    if (ndsFighterNaturalProjectileProofEnabled() == FALSE)
    {
        return;
    }
    actor = fp[sNdsNaturalProjectileActorSlot];
    mask = gNdsFighterProjectileProofMask;
    if (actor != NULL)
    {
        gNdsFighterProjectileProofActorSlot =
            sNdsNaturalProjectileActorSlot;
        gNdsFighterProjectileProofActorKind = (u32)actor->fkind;
        mask |= 1u << 0;
        if (ndsFighterNaturalProjectileStatusIsSpecialN(actor) != FALSE)
        {
            gNdsFighterProjectileProofSpecialStatusFrames++;
            gNdsFighterProjectileProofSpecialMotion = (u32)actor->motion_id;
            mask |= 1u << 2;
        }
        if (actor->proc_accessory != NULL)
        {
            NDS_DIAG(gNdsFighterProjectileProofAccessoryFrames++);
        }
        if (actor->motion_vars.flags.flag0 != 0)
        {
            NDS_DIAG(gNdsFighterProjectileProofFlag0Frames++);
        }
    }
    if (gNdsFighterProjectileProofBPressFrames > 0u)
    {
        mask |= 1u << 1;
    }

    weapon_gobj = gGCCommonLinks[nGCCommonLinkIDWeapon];
    while (weapon_gobj != NULL)
    {
        WPStruct *wp = wpGetStruct(weapon_gobj);

        if (wp != NULL)
        {
            count++;
            if ((wp->kind >= 0) && (wp->kind < 32))
            {
                gNdsFighterProjectileProofKindMask |= 1u << wp->kind;
            }
            if ((wp->attack_coll.attack_state >= 0) &&
                (wp->attack_coll.attack_state < 32))
            {
                gNdsFighterProjectileProofAttackStateMask |=
                    1u << wp->attack_coll.attack_state;
            }
            if ((u32)wp->attack_coll.damage >
                gNdsFighterProjectileProofDamageMax)
            {
                gNdsFighterProjectileProofDamageMax =
                    (u32)wp->attack_coll.damage;
            }
            if ((u32)wp->lifetime > gNdsFighterProjectileProofLifetimeMax)
            {
                gNdsFighterProjectileProofLifetimeMax = (u32)wp->lifetime;
            }
            gNdsFighterProjectileProofMapMask |=
                wp->coll_data.mask_prev | wp->coll_data.mask_curr |
                wp->coll_data.mask_stat;
        }
        weapon_gobj = weapon_gobj->link_next;
    }
    if (count > 0u)
    {
        gNdsFighterProjectileProofWeaponFrames++;
        mask |= 1u << 3;
    }
    else if (gNdsFighterProjectileProofHitDestroyCount > 0u)
    {
        mask |= 1u << 3;
    }
    if (count > gNdsFighterProjectileProofWeaponCountMax)
    {
        gNdsFighterProjectileProofWeaponCountMax = count;
    }
    if ((gNdsFighterProjectileProofKindMask &
         (1u << sNdsNaturalProjectileExpectedKind)) != 0u)
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterProjectileProofAttackStateMask != 0u) &&
        (gNdsFighterProjectileProofDamageMax > 0u))
    {
        mask |= 1u << 5;
    }
    gNdsFighterProjectileProofMask = mask;
    if ((mask & 0x3fu) == 0x3fu)
    {
        gNdsFighterProjectileProofResult =
            NDS_FIGHTER_PROJECTILE_PROOF_PASS;
    }
    ndsFighterNaturalReflectorRecord(fp);
}

static sb32 ndsFighterNaturalMovesetStatusIsTiltS3(s32 status_id)
{
    return ((status_id >= nFTCommonStatusAttackS3Hi) &&
            (status_id <= nFTCommonStatusAttackS3Lw)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsTiltHi3(s32 status_id)
{
    return ((status_id >= nFTCommonStatusAttackHi3F) &&
            (status_id <= nFTCommonStatusAttackHi3B)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsTiltLw3(s32 status_id)
{
    return (status_id == nFTCommonStatusAttackLw3) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsSmash(s32 status_id)
{
    return ((status_id >= nFTCommonStatusAttackS4Hi) &&
            (status_id <= nFTCommonStatusAttackLw4)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsAerial(s32 status_id)
{
    return ((status_id >= nFTCommonStatusAttackAirStart) &&
            (status_id <= nFTCommonStatusAttackAirEnd)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsLandingAir(s32 status_id)
{
    return (((status_id >= nFTCommonStatusLandingAirStart) &&
             (status_id <= nFTCommonStatusLandingAirEnd)) ||
            (status_id == nFTCommonStatusLandingLight) ||
            (status_id == nFTCommonStatusLandingHeavy)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsRecovering(s32 status_id)
{
    return ((status_id == nFTCommonStatusWait) ||
            (status_id == nFTCommonStatusFall) ||
            (status_id == nFTCommonStatusFallAerial) ||
            (status_id == nFTCommonStatusDamageFall) ||
            (status_id == nFTCommonStatusFallSpecial) ||
            (status_id == nFTCommonStatusLandingFallSpecial) ||
            (status_id == nFTCommonStatusDownBounceD) ||
            (status_id == nFTCommonStatusDownBounceU) ||
            (status_id == nFTCommonStatusDownWaitD) ||
            (status_id == nFTCommonStatusDownWaitU) ||
            (status_id == nFTCommonStatusDownStandD) ||
            (status_id == nFTCommonStatusDownStandU) ||
            (status_id == nFTCommonStatusPassiveStandF) ||
            (status_id == nFTCommonStatusPassiveStandB) ||
            (status_id == nFTCommonStatusPassive) ||
            (ndsFighterNaturalCombatStatusIsDamage(status_id) != FALSE) ||
            (ndsFighterNaturalMovesetStatusIsLandingAir(status_id) !=
                FALSE)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalCombatHitboxActive(FTStruct *fp)
{
    u32 i;

    for (i = 0u; i < ARRAY_COUNT(fp->attack_colls); i++)
    {
        if (fp->attack_colls[i].attack_state != nGMAttackStateOff)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsFighterNaturalCombatRecordAttackColl(FTStruct *fp)
{
    u32 i;

    for (i = 0u; i < ARRAY_COUNT(fp->attack_colls); i++)
    {
        FTAttackColl *attack_coll = &fp->attack_colls[i];

        if (attack_coll->attack_state == nGMAttackStateOff)
        {
            continue;
        }
        gNdsFighterDashRunAttackEventLastPlayer = fp->player;
        gNdsFighterDashRunAttackEventLastStatus = (u32)fp->status_id;
        gNdsFighterDashRunAttackEventLastState =
            (u32)attack_coll->attack_state;
        gNdsFighterDashRunAttackEventLastAttackID = i;
        gNdsFighterDashRunAttackEventLastGroupID = attack_coll->group_id;
        gNdsFighterDashRunAttackEventLastJointID =
            (u32)attack_coll->joint_id;
        gNdsFighterDashRunAttackEventLastDamage = attack_coll->damage;
        gNdsFighterDashRunAttackEventLastSize = (s32)attack_coll->size;
        gNdsFighterDashRunAttackEventLastOffsetX =
            (s32)attack_coll->offset.x;
        gNdsFighterDashRunAttackEventLastOffsetY =
            (s32)attack_coll->offset.y;
        gNdsFighterDashRunAttackEventLastOffsetZ =
            (s32)attack_coll->offset.z;
        gNdsFighterDashRunAttackEventLastAngle = attack_coll->angle;
        gNdsFighterDashRunAttackEventLastKBG =
            attack_coll->knockback_scale;
        gNdsFighterDashRunAttackEventLastKBW =
            attack_coll->knockback_weight;
        gNdsFighterDashRunAttackEventLastBKB =
            attack_coll->knockback_base;
        gNdsFighterDashRunAttackEventLastShield =
            attack_coll->shield_damage;
        gNdsFighterDashRunAttackEventLastFlags =
            (attack_coll->is_hit_air ? 0x1u : 0u) |
            (attack_coll->is_hit_ground ? 0x2u : 0u) |
            (attack_coll->can_rebound ? 0x4u : 0u) |
            (attack_coll->is_scale_pos ? 0x8u : 0u);
        break;
    }
}

static sb32 ndsFighterBattlePlayableStatusIsDead(s32 status_id)
{
    return ((status_id >= nFTCommonStatusDeadDown) &&
            (status_id <= nFTCommonStatusDeadUpFall)) ? TRUE : FALSE;
}

static void ndsFighterBattlePlayableRecordVictim(FTStruct *victim)
{
    u32 mask = 0u;

    if ((ndsFighterBattlePlayableProofEnabled() == FALSE) ||
        (victim == NULL))
    {
        return;
    }

    gNdsFighterBattlePlayableVictimStockFinal = (u32)victim->stock_count;
    gNdsFighterBattlePlayableBattleStockFinal =
        (u32)gSCManagerBattleState->players[victim->player].stock_count;
    gNdsFighterBattlePlayableFallsFinal =
        (u32)gSCManagerBattleState->players[victim->player].falls;
    gNdsFighterBattlePlayableFinalStatus = (u32)victim->status_id;
    gNdsFighterBattlePlayableFinalGA = (u32)victim->ga;
    gNdsFighterBattlePlayableFinalFloor =
        (u32)victim->coll_data.floor_line_id;
    gNdsFighterBattlePlayableFinalIsRebirth = (u32)victim->is_rebirth;
    gNdsFighterBattlePlayableFinalIsGhost = (u32)victim->is_ghost;
    gNdsFighterBattlePlayableFinalCameraMode = (u32)victim->camera_mode;
    gNdsFighterBattlePlayableFinalVelXMilli =
        ndsFloatToMilliSigned(victim->physics.vel_air.x);
    gNdsFighterBattlePlayableFinalVelYMilli =
        ndsFloatToMilliSigned(victim->physics.vel_air.y);
    gNdsFighterBattlePlayableFinalFloorDistMilli =
        ndsFloatToMilliSigned(victim->coll_data.floor_dist);
    if (victim->coll_data.p_translate != NULL)
    {
        gNdsFighterBattlePlayableFinalXMilli =
            ndsFloatToMilliSigned(victim->coll_data.p_translate->x);
        gNdsFighterBattlePlayableFinalYMilli =
            ndsFloatToMilliSigned(victim->coll_data.p_translate->y);
    }

    if ((gNdsFighterBattlePlayableMask & 0x7fu) == 0x7fu)
    {
        if ((gNdsFighterNaturalMotionUnsafeCount == 0u) &&
            (gNdsFighterNaturalCombatStallCount == 0u) &&
            (sNdsNaturalCombatPhase ==
                nNDSNaturalCombatPhaseBattlePlayableDone))
        {
            gNdsFighterBattlePlayableMask |= 1u << 7;
            gNdsFighterBattlePlayableResult =
                NDS_FIGHTER_BATTLE_PLAYABLE_PASS;
        }
        return;
    }

    if ((gSCManagerBattleState->game_rules & SCBATTLE_GAMERULE_STOCK) != 0)
    {
        mask |= 1u << 0;
    }
    if (gNdsFighterBattlePlayableKOStickFrames > 0u)
    {
        mask |= 1u << 1;
    }
    if (ndsFighterBattlePlayableStatusIsDead(victim->status_id) != FALSE)
    {
        gNdsFighterBattlePlayableDeadFrames++;
    }
    if (gNdsFighterBattlePlayableDeadFrames > 0u)
    {
        mask |= 1u << 2;
    }
    if ((sNdsBattlePlayableVictimStockStart >=
            gNdsFighterBattlePlayableVictimStockFinal) &&
        (sNdsBattlePlayableBattleStockStart >=
            gNdsFighterBattlePlayableBattleStockFinal) &&
        (gNdsFighterBattlePlayableFallsFinal >=
            sNdsBattlePlayableFallsStart))
    {
        u32 victim_stock_delta =
            sNdsBattlePlayableVictimStockStart -
            gNdsFighterBattlePlayableVictimStockFinal;
        u32 battle_stock_delta =
            sNdsBattlePlayableBattleStockStart -
            gNdsFighterBattlePlayableBattleStockFinal;
        u32 falls_delta =
            gNdsFighterBattlePlayableFallsFinal -
            sNdsBattlePlayableFallsStart;

        if ((victim_stock_delta > 0u) &&
            (victim_stock_delta == battle_stock_delta) &&
            (victim_stock_delta == falls_delta))
        {
            mask |= 1u << 3;
        }
    }

    if (victim->status_id == nFTCommonStatusRebirthDown)
    {
        gNdsFighterBattlePlayableRebirthDownFrames++;
        sNdsBattlePlayableRebirthSeen = 1u;
    }
    else if (victim->status_id == nFTCommonStatusRebirthStand)
    {
        gNdsFighterBattlePlayableRebirthStandFrames++;
        sNdsBattlePlayableRebirthSeen = 1u;
    }
    else if (victim->status_id == nFTCommonStatusRebirthWait)
    {
        gNdsFighterBattlePlayableRebirthWaitFrames++;
        sNdsBattlePlayableRebirthSeen = 1u;
    }
    if ((gNdsFighterBattlePlayableRebirthDownFrames > 0u) &&
        (gNdsFighterBattlePlayableRebirthStandFrames > 0u) &&
        (gNdsFighterBattlePlayableRebirthWaitFrames > 0u))
    {
        mask |= 1u << 4;
    }
    if ((sNdsBattlePlayableRebirthSeen != 0u) &&
        (victim->status_id == nFTCommonStatusFall))
    {
        gNdsFighterBattlePlayableFallAfterRebirthFrames++;
    }
    if ((gNdsFighterBattlePlayableFallAfterRebirthFrames > 0u) ||
        (gNdsFighterBattlePlayableWaitAfterRebirthFrames > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterBattlePlayableRebirthWaitFrames > 0u) &&
        (victim->status_id == nFTCommonStatusWait))
    {
        gNdsFighterBattlePlayableWaitAfterRebirthFrames++;
    }
    if (gNdsFighterBattlePlayableWaitAfterRebirthFrames >=
        NDS_FIGHTER_BATTLE_PLAYABLE_WAIT_AFTER_REBIRTH_REQUIRED)
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterNaturalMotionUnsafeCount == 0u) &&
        (gNdsFighterNaturalCombatStallCount == 0u) &&
        (sNdsNaturalCombatPhase ==
            nNDSNaturalCombatPhaseBattlePlayableDone))
    {
        mask |= 1u << 7;
    }

    gNdsFighterBattlePlayableMask = mask;
    if ((mask & NDS_FIGHTER_BATTLE_PLAYABLE_MASK_ALL) ==
        NDS_FIGHTER_BATTLE_PLAYABLE_MASK_ALL)
    {
        gNdsFighterBattlePlayableResult =
            NDS_FIGHTER_BATTLE_PLAYABLE_PASS;
    }
}

/* Observe the attacker/victim pair while the combat phases run. All state
 * transitions below happen inside the original imported runtime; this only
 * counts what it sees. */
static void ndsFighterNaturalCombatRecordPair(FTStruct *attacker,
                                              FTStruct *victim)
{
    if ((attacker == NULL) || (victim == NULL))
    {
        gNdsFighterNaturalMotionUnsafeCount++;
        return;
    }

    ndsRelocUpdateMemoryLedger();
    ndsFighterBattlePlayableRecordVictim(victim);
    ndsIFCommonRecordHUDState();

    if (attacker->status_id == nFTCommonStatusAttack11)
    {
        gNdsFighterNaturalCombatAttackStatusFrames++;
        gNdsFighterNaturalCombatAttackMotionFinal =
            (u32)attacker->motion_id;
        if (ndsFighterNaturalCombatHitboxActive(attacker) != FALSE)
        {
            NDS_DIAG(gNdsFighterNaturalCombatHitboxActiveFrames++);
            ndsFighterNaturalCombatRecordAttackColl(attacker);
        }
    }
    if (ndsFighterBattlePlayableProofEnabled() != FALSE)
    {
        sb32 is_hitbox_active =
            ndsFighterNaturalCombatHitboxActive(attacker);
        DObj *attacker_root = attacker->joints[nFTPartsJointTopN];
        DObj *victim_root = victim->joints[nFTPartsJointTopN];

        gNdsFighterNaturalMovesetAttackerStatus = (u32)attacker->status_id;
        gNdsFighterNaturalMovesetAttackerMotion = (u32)attacker->motion_id;
        gNdsFighterNaturalMovesetAttackerGA = (u32)attacker->ga;
        gNdsFighterNaturalMovesetAttackerRootYMilli =
            (attacker_root != NULL) ?
                ndsFloatToMilliSigned(attacker_root->translate.vec.f.y) : 0;
        gNdsFighterNaturalMovesetVictimStatus = (u32)victim->status_id;
        gNdsFighterNaturalMovesetVictimMotion = (u32)victim->motion_id;
        gNdsFighterNaturalMovesetVictimGA = (u32)victim->ga;
        gNdsFighterNaturalMovesetVictimRootYMilli =
            (victim_root != NULL) ?
                ndsFloatToMilliSigned(victim_root->translate.vec.f.y) : 0;

        if (ndsFighterNaturalMovesetStatusIsTiltS3(
                attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetTiltS3Frames++;
            if (is_hitbox_active != FALSE)
            {
                gNdsFighterNaturalMovesetTiltHitboxFrames++;
                ndsFighterNaturalCombatRecordAttackColl(attacker);
            }
        }
        else if (ndsFighterNaturalMovesetStatusIsTiltHi3(
                     attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetTiltHi3Frames++;
            if (is_hitbox_active != FALSE)
            {
                gNdsFighterNaturalMovesetTiltHitboxFrames++;
                ndsFighterNaturalCombatRecordAttackColl(attacker);
            }
        }
        else if (ndsFighterNaturalMovesetStatusIsTiltLw3(
                     attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetTiltLw3Frames++;
            if (is_hitbox_active != FALSE)
            {
                gNdsFighterNaturalMovesetTiltHitboxFrames++;
                ndsFighterNaturalCombatRecordAttackColl(attacker);
            }
        }
        else if (ndsFighterNaturalMovesetStatusIsSmash(
                     attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetSmashFrames++;
            if (is_hitbox_active != FALSE)
            {
                NDS_DIAG(gNdsFighterNaturalMovesetSmashHitboxFrames++);
                ndsFighterNaturalCombatRecordAttackColl(attacker);
            }
        }
        else if (ndsFighterNaturalMovesetStatusIsAerial(
                     attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetAerialFrames++;
            if (is_hitbox_active != FALSE)
            {
                NDS_DIAG(gNdsFighterNaturalMovesetAerialHitboxFrames++);
                ndsFighterNaturalCombatRecordAttackColl(attacker);
            }
        }
        else if (ndsFighterNaturalMovesetStatusIsLandingAir(
                     attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetLandingFrames++;
        }
        else if (attacker->status_id == nFTCommonStatusCatch)
        {
            gNdsFighterNaturalMovesetCatchFrames++;
        }
        else if (attacker->status_id == nFTCommonStatusCatchWait)
        {
            gNdsFighterNaturalMovesetCatchWaitFrames++;
        }
        else if ((attacker->status_id == nFTCommonStatusThrowF) ||
                 (attacker->status_id == nFTCommonStatusThrowB))
        {
            gNdsFighterNaturalMovesetThrowFrames++;
        }
        if ((victim->status_id >= nFTCommonStatusThrownStart) &&
            (victim->status_id <= nFTCommonStatusThrownEnd))
        {
            gNdsFighterNaturalMovesetThrownFrames++;
        }
        if ((gNdsFighterNaturalMovesetThrownFrames > 0u) &&
            (victim->status_id == nFTCommonStatusWait))
        {
            gNdsFighterNaturalMovesetThrowRecoverFrames++;
        }
    }

    if (ndsFighterNaturalCombatStatusIsDamage(victim->status_id) != FALSE)
    {
        if (sNdsNaturalCombatVictimHitSeen == 0u)
        {
            sNdsNaturalCombatVictimHitSeen = 1u;
            sNdsNaturalCombatVictimHitPosX =
                ndsFighterNaturalCombatPosX(victim);
            gNdsFighterNaturalCombatVictimDamageStatus =
                (u32)victim->status_id;
        }
        NDS_DIAG(gNdsFighterNaturalCombatVictimDamageFrames++);
    }
    if (sNdsNaturalCombatVictimHitSeen != 0u)
    {
        f32 delta = ndsFighterNaturalCombatPosX(victim) -
            sNdsNaturalCombatVictimHitPosX;

        if (delta < 0.0F)
        {
            delta = -delta;
        }
        if (ndsFloatToMilliSigned(delta) >
            (s32)gNdsFighterNaturalCombatVictimKnockbackMilli)
        {
            gNdsFighterNaturalCombatVictimKnockbackMilli =
                (u32)ndsFloatToMilliSigned(delta);
        }
        if ((victim->status_id == nFTCommonStatusWait) &&
            (ndsFighterNaturalCombatStatusIsGuard(victim->status_id) ==
                FALSE))
        {
            NDS_DIAG(gNdsFighterNaturalCombatVictimRecoverWaitFrames++);
        }
    }
    gNdsFighterNaturalCombatVictimFinalPercent =
        (u32)victim->percent_damage;

    if (victim->status_id == nFTCommonStatusGuardOn)
    {
        gNdsFighterNaturalCombatGuardOnFrames++;
    }
    else if (victim->status_id == nFTCommonStatusGuard)
    {
        gNdsFighterNaturalCombatGuardFrames++;
    }
    else if ((victim->status_id == nFTCommonStatusGuardOff) ||
             (victim->status_id == nFTCommonStatusGuardSetOff))
    {
        NDS_DIAG(gNdsFighterNaturalCombatGuardOffFrames++);
    }
    else if ((victim->status_id == nFTCommonStatusEscapeF) ||
             (victim->status_id == nFTCommonStatusEscapeB))
    {
        gNdsFighterNaturalCombatRollFrames++;
        gNdsFighterNaturalCombatRollStatus = (u32)victim->status_id;
    }
    if (attacker->status_id == nFTCommonStatusAppeal)
    {
        gNdsFighterNaturalCombatAppealFrames++;
        gNdsFighterNaturalCombatAppealStatus = (u32)attacker->status_id;
    }
}

void ndsFighterMarioFoxNaturalMotionPrepare(void)
{
    FTStruct *p0 = ndsFighterManagerLiveStruct(0u);
    FTStruct *p1 = ndsFighterManagerLiveStruct(1u);
    FTStruct *fp[2];

    if ((ndsFighterMarioFoxNaturalMotionProofEnabled() == FALSE) ||
        (gNdsFighterNaturalMotionPrepared != 0u))
    {
        return;
    }
    gNdsFighterNaturalMotionManagerMask = ndsFighterManagerLiveMask();
    if ((gNdsFighterNaturalMotionManagerMask & 0x3u) != 0x3u)
    {
        return;
    }
    fp[0] = p0;
    fp[1] = p1;


    bzero(sNdsFighterNaturalMotionStates,
          sizeof(sNdsFighterNaturalMotionStates));
    sNdsFighterNaturalMotionWalkInputActive = 0u;
    sNdsNaturalCombatPhase = nNDSNaturalCombatPhaseWait;
    sNdsNaturalCombatPhaseFrames = 0u;
    sNdsNaturalCombatSettleFrames = 0u;
    sNdsNaturalCombatAttackFrames = 0u;
    sNdsNaturalCombatAttackPressed = 0u;
    sNdsNaturalCombatPassPressed = 0u;
    sNdsNaturalCombatRollPressed = 0u;
    sNdsNaturalCombatAppealPressed = 0u;
    sNdsNaturalCombatApproachStopRange =
        NDS_FIGHTER_NATURAL_COMBAT_APPROACH_STOP_RANGE;
    sNdsNaturalCombatApproachStagnant = 0u;
    sNdsNaturalCombatApproachLastDX = 0.0F;
    sNdsNaturalCombatVictimHitSeen = 0u;
    sNdsNaturalCombatVictimHitPosX = 0.0F;
    sNdsNaturalMovesetPhase = nNDSNaturalMovesetPhaseIdle;
    sNdsNaturalMovesetPhaseFrames = 0u;
    sNdsNaturalMovesetSettleFrames = 0u;
    sNdsNaturalMovesetDone = 0u;
    sNdsNaturalMovesetKORecoveryActive = 0u;
    sNdsNaturalMovesetKORecoveryPhase = nNDSNaturalMovesetPhaseIdle;
    sNdsNaturalSpecialsPhase = nNDSNaturalSpecialsPhaseIdle;
    sNdsNaturalSpecialsPhaseFrames = 0u;
    sNdsNaturalSpecialsDone = 0u;
    sNdsNaturalSpecialsButtonPressed = 0u;
    gNdsFighterSpecialsMarioSlot = 0u;
    gNdsFighterSpecialsFoxSlot = 1u;
    if (ndsFighterNaturalIsMarioSpecialFamily(p1) != FALSE)
    {
        gNdsFighterSpecialsMarioSlot = 1u;
    }
    if (p0->fkind == nFTKindFox)
    {
        gNdsFighterSpecialsFoxSlot = 0u;
    }
    if (p1->fkind == nFTKindFox)
    {
        gNdsFighterSpecialsFoxSlot = 1u;
    }
#if NDS_DONKEY
    gNdsFighterDonkeySpecialsSlot = 0u;
    if (ndsFighterNaturalIsDonkey(p1) != FALSE)
    {
        gNdsFighterDonkeySpecialsSlot = 1u;
    }
#endif
    gNdsFighterSamusSpecialsSlot = 0u;
    if (ndsFighterNaturalIsSamus(p1) != FALSE)
    {
        gNdsFighterSamusSpecialsSlot = 1u;
    }
    gNdsFighterSamusSpecialsNPressFrames = 0u;
    gNdsFighterSamusSpecialsNStartFrames = 0u;
    gNdsFighterSamusSpecialsNLoopFrames = 0u;
    gNdsFighterSamusSpecialsNChargeMax = 0u;
    gNdsFighterSamusSpecialsNFullWaitFrames = 0u;
    gNdsFighterSamusSpecialsNReleasePressFrames = 0u;
    gNdsFighterSamusSpecialsNEndFrames = 0u;
    gNdsFighterSamusSpecialsNReleaseWaitFrames = 0u;
    gNdsFighterSamusSpecialsLwPressFrames = 0u;
    gNdsFighterSamusSpecialsLwFrames = 0u;
    gNdsFighterSamusSpecialsLwWaitFrames = 0u;
    sNdsNaturalCombatAttackerSlot =
        ((gNdsBattlePlayableFoxCpuEnabled != 0u) &&
         (p1->fkind == nFTKindFox)) ? 1u : 0u;
    sNdsNaturalCombatVictimSlot = 1u - sNdsNaturalCombatAttackerSlot;
    sNdsNaturalCombatVictimStartPercent =
        (u32)((sNdsNaturalCombatVictimSlot == 0u) ? p0 : p1)->percent_damage;
    sNdsBattlePlayableRebirthSeen = 0u;
    sNdsBattlePlayableKOStickX = 80;
    sNdsNaturalProjectileButtonPressed = 0u;
    sNdsNaturalProjectileKORecoveryActive = 0u;
    sNdsNaturalProjectileActorSlot =
        ndsFighterNaturalProjectileSelectSlot(fp);
    sNdsNaturalProjectileExpectedKind =
        ndsFighterNaturalProjectileExpectedKind(fp[sNdsNaturalProjectileActorSlot]);
    sNdsNaturalReflectorProjectileSlot = sNdsNaturalProjectileActorSlot;
    sNdsNaturalReflectorFoxSlot =
        ndsFighterNaturalReflectorSelectFoxSlot(fp);
    sNdsNaturalReflectorButtonPressed = 0u;
    if (ndsFighterBattlePlayableProofEnabled() != FALSE)
    {
        FTStruct *victim =
            (sNdsNaturalCombatVictimSlot == 0u) ? p0 : p1;

        sNdsBattlePlayableVictimStockStart = (u32)victim->stock_count;
        sNdsBattlePlayableBattleStockStart =
            (u32)gSCManagerBattleState->players[victim->player].stock_count;
        sNdsBattlePlayableFallsStart =
            (u32)gSCManagerBattleState->players[victim->player].falls;
        sNdsBattlePlayableKOStickX =
            (ndsFighterNaturalCombatPosX(victim) < 0.0F) ? -80 : 80;
        gNdsFighterBattlePlayableVictimSlot = sNdsNaturalCombatVictimSlot;
        gNdsFighterBattlePlayableVictimStockStart =
            sNdsBattlePlayableVictimStockStart;
        gNdsFighterBattlePlayableBattleStockStart =
            sNdsBattlePlayableBattleStockStart;
        gNdsFighterBattlePlayableFallsStart =
            sNdsBattlePlayableFallsStart;
    }
    gNdsFighterNaturalCombatAttackerSlot = sNdsNaturalCombatAttackerSlot;
    gNdsFighterNaturalCombatVictimSlot = sNdsNaturalCombatVictimSlot;
    gNdsFighterNaturalCombatVictimStartPercent =
        sNdsNaturalCombatVictimStartPercent;
    ndsControllerPlaybackReset();
    ndsControllerPlaybackSetConnectedMask(0x3u);
    if (ndsFighterBattlePlayableProofEnabled() != FALSE)
    {
        ndsControllerPlaybackSetEnabled(FALSE);
    }
    else
    {
        ndsControllerPlaybackSetEnabled(TRUE);
    }
    gNdsFighterNaturalMotionGObjCountBefore = (u32)gcGetGObjsActiveNum();

    if (ndsFighterBattlePlayableProofEnabled() == FALSE)
    {
        gcFuncGObjAll(ndsFighterNaturalMotionPauseNonTargetVisitor, 0u);
    }

    gNdsFighterNaturalMotionP0StatusStart = (u32)p0->status_id;
    gNdsFighterNaturalMotionP1StatusStart = (u32)p1->status_id;
    gNdsFighterNaturalMotionPrepared = 1u;
}

static sb32 ndsFighterNaturalCombatBothWait(FTStruct *fp[2])
{
    return ((fp[0]->status_id == nFTCommonStatusWait) &&
            (fp[1]->status_id == nFTCommonStatusWait)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalCombatBothGroundWait(FTStruct *fp[2])
{
    return ((ndsFighterNaturalCombatBothWait(fp) != FALSE) &&
            (fp[0]->ga == nMPKineticsGround) &&
            (fp[1]->ga == nMPKineticsGround)) ? TRUE : FALSE;
}

static void ndsFighterNaturalCombatSetPhase(u32 phase)
{
    sNdsNaturalCombatPhase = phase;
    sNdsNaturalCombatPhaseFrames = 0u;
    sNdsNaturalCombatSettleFrames = 0u;
    gNdsFighterNaturalCombatPhase = phase;
}

static sb32 ndsFighterBattlePlayableHasRecoveredKO(void)
{
    return ((gNdsFighterBattlePlayableDeadFrames > 0u) &&
            (gNdsFighterBattlePlayableRebirthDownFrames > 0u) &&
            (gNdsFighterBattlePlayableRebirthStandFrames > 0u) &&
            (gNdsFighterBattlePlayableRebirthWaitFrames > 0u) &&
            (gNdsFighterBattlePlayableWaitAfterRebirthFrames >=
                NDS_FIGHTER_BATTLE_PLAYABLE_WAIT_AFTER_REBIRTH_REQUIRED)) ?
        TRUE : FALSE;
}

static void ndsFighterNaturalCombatStartKOExit(FTStruct *victim)
{
    /* P2-3r3: skip to Recover only when the DRIVEN exit already produced its
     * own evidence. A victim can be KO'd accidentally mid-flow (DK is thrown
     * off during the moveset phases and rebirths transparently); that fills
     * the dead/rebirth counters but never exercises the KO stick drive, and
     * skipping here left BattlePlayable bit 1 permanently unreachable. */
    if ((ndsFighterBattlePlayableHasRecoveredKO() != FALSE) &&
        (gNdsFighterBattlePlayableKOStickFrames > 0u))
    {
        ndsFighterNaturalCombatSetPhase(
            nNDSNaturalCombatPhaseBattlePlayableRecover);
        return;
    }
    /* An accidental KO leaves the dead/rebirth counters non-zero, and the
     * KOExit -> Dead -> Rebirth -> Recover ladder keys on exactly those
     * counters -- stale values would fast-forward the ladder before the
     * driven KO happens. Clear them so the drive is witnessed for real; the
     * per-frame mask is rebuilt from these counters, and the stock/falls
     * delta equality (bit 3) holds for any KO count. */
    gNdsFighterBattlePlayableDeadFrames = 0u;
    gNdsFighterBattlePlayableRebirthDownFrames = 0u;
    gNdsFighterBattlePlayableRebirthStandFrames = 0u;
    gNdsFighterBattlePlayableRebirthWaitFrames = 0u;
    gNdsFighterBattlePlayableFallAfterRebirthFrames = 0u;
    gNdsFighterBattlePlayableWaitAfterRebirthFrames = 0u;
    sNdsBattlePlayableRebirthSeen = 0u;
    sNdsBattlePlayableKOStickX =
        (ndsFighterNaturalCombatPosX(victim) < 0.0F) ? -80 : 80;
    ndsFighterNaturalCombatSetPhase(
        nNDSNaturalCombatPhaseBattlePlayableKOExit);
}

static sb32 ndsFighterNaturalProjectileHandleKORecovery(FTStruct *fp[2])
{
    u32 i;

    if ((sNdsNaturalCombatPhase != nNDSNaturalCombatPhaseProjectileSettle) &&
        (sNdsNaturalCombatPhase != nNDSNaturalCombatPhaseProjectileFire) &&
        (sNdsNaturalCombatPhase != nNDSNaturalCombatPhaseProjectileObserve))
    {
        return FALSE;
    }
    if (sNdsNaturalProjectileKORecoveryActive == 0u)
    {
        for (i = 0u; i < 2u; i++)
        {
            if (ndsFighterBattlePlayableStatusIsDead(fp[i]->status_id) !=
                FALSE)
            {
                sNdsNaturalProjectileKORecoveryActive = 1u;
                break;
            }
        }
    }
    if (sNdsNaturalProjectileKORecoveryActive == 0u)
    {
        return FALSE;
    }
    if (ndsFighterNaturalCombatBothGroundWait(fp) != FALSE)
    {
        sNdsNaturalProjectileKORecoveryActive = 0u;
        sNdsNaturalProjectileButtonPressed = 0u;
        sNdsNaturalCombatPassPressed = 0u;
        sNdsNaturalReflectorButtonPressed = 0u;
        ndsFighterNaturalCombatSetPhase(
            nNDSNaturalCombatPhaseProjectileSettle);
    }
    return TRUE;
}

static void ndsFighterNaturalMovesetSetPhase(u32 phase)
{
    sNdsNaturalMovesetPhase = phase;
    sNdsNaturalMovesetPhaseFrames = 0u;
    sNdsNaturalMovesetSettleFrames = 0u;
    gNdsFighterNaturalMovesetPhase = phase;
}

static u32 ndsFighterNaturalMovesetRetryPhase(u32 phase)
{
    switch (phase)
    {
    case nNDSNaturalMovesetPhaseSettleTiltS3:
        return nNDSNaturalMovesetPhaseTiltS3;
    case nNDSNaturalMovesetPhaseSettleTiltHi3:
        return nNDSNaturalMovesetPhaseTiltHi3;
    case nNDSNaturalMovesetPhaseSettleTiltLw3:
        return nNDSNaturalMovesetPhaseTiltLw3;
    case nNDSNaturalMovesetPhaseSettleSmashS4:
        return nNDSNaturalMovesetPhaseSmashS4;
    case nNDSNaturalMovesetPhaseAerialAttack:
    case nNDSNaturalMovesetPhaseSettleAerial:
        return nNDSNaturalMovesetPhaseAerialJump;
    case nNDSNaturalMovesetPhaseGrabThrow:
    case nNDSNaturalMovesetPhaseSettleThrow:
        return nNDSNaturalMovesetPhaseGrabCatch;
    default:
        return phase;
    }
}
