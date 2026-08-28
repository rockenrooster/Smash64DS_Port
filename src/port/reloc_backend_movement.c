#include "nds_scene_harness_config.h"
#include <nds/nds_effects.h>
#include <nds/nds_scene_harness.h>
#include <nds/nds_task37_itcm.h>
#if NDS_R2_FIREBALL_QUAD
/* The baked fireball texels/palettes and the GL_RGB16 upload they need. Only
 * this flag's path uses either, so neither is pulled in unconditionally. */
#include <nds/generated/nds_particle_banks.generated.h>
#endif
#if NDS_R2_FIREBALL_QUAD || NDS_R2_FOX_BLASTER_QUAD
#include <nds/nds_renderer.h>
#endif

#ifndef NDS_SCENE_MIP_CACHE_LAB
#define NDS_SCENE_MIP_CACHE_LAB 0
#endif

#ifndef NDS_FAST_WALLPAPER_AFFINE
#define NDS_FAST_WALLPAPER_AFFINE 0
#endif

extern void ndsIFCommonRecordHUDState(void);



















#define NDS_FIGHTER_PROCESS_LOOP_MAX_FRAMES 160u
#define NDS_FIGHTER_PROCESS_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_PROCESS_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu

static void ndsFighterProcessLoopSetStatus(FTStruct *fp, GObj *fighter_gobj,
                                           s32 status_id, f32 frame_begin,
                                           f32 anim_speed, u32 flags)
{
    if ((fp == NULL) || (fighter_gobj == NULL))
    {
        gNdsFighterProcessLoopDeniedStatusCount++;
        return;
    }

    if ((status_id == nFTCommonStatusFallAerial) &&
        (sNdsFighterProcessLoopJumpAnimEndActive != FALSE))
    {
        status_id = nFTCommonStatusFall;
    }

    switch (status_id)
    {
    case nFTCommonStatusWait:
    case nFTCommonStatusWalkSlow:
    case nFTCommonStatusWalkMiddle:
    case nFTCommonStatusWalkFast:
    case nFTCommonStatusDash:
    case nFTCommonStatusRun:
    case nFTCommonStatusRunBrake:
    case nFTCommonStatusKneeBend:
    case nFTCommonStatusJumpF:
    case nFTCommonStatusFall:
    case nFTCommonStatusLandingLight:
        break;
    default:
        gNdsFighterProcessLoopDeniedStatusCount++;
        gNdsFighterProcessLoopUnexpectedStatusCount++;
        return;
    }

    ndsFTMainApplyCommonStatusReset(fp, flags);
    fp->status_prev = fp->status_id;
    fp->status_id = status_id;
    fp->status_total_tics = 0;
    fp->motion_attack_id = nFTMotionAttackIDNone;
    fp->status_attack_id = nFTStatusAttackIDNone;
    fp->stat_attack_id = nFTStatusAttackIDNone;
    fp->status_is_smash = FALSE;
    fp->status_is_projectile = FALSE;
    fp->status_flags = flags;
    fp->motion_frame = frame_begin;
    fp->anim_frame = frame_begin;
    fp->anim_speed = anim_speed;
    fp->proc_status = NULL;
    fighter_gobj->anim_frame = frame_begin;

    switch (status_id)
    {
    case nFTCommonStatusWait:
        fp->motion_id = nFTCommonMotionWait;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonWaitProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->is_special_interrupt = TRUE;
        fp->is_wait_status_setup = TRUE;
        fp->is_wait_motion_setup = TRUE;
        gNdsFighterProcessLoopWaitSetStatusCount++;
        break;
    case nFTCommonStatusWalkSlow:
    case nFTCommonStatusWalkMiddle:
    case nFTCommonStatusWalkFast:
        fp->motion_id = nFTCommonMotionWalkSlow +
            (status_id - nFTCommonStatusWalkSlow);
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonWalkProcInterrupt;
        fp->proc_physics = ftCommonWalkProcPhysics;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        break;
    case nFTCommonStatusDash:
        fp->motion_id = nFTCommonMotionDash;
        fp->proc_update = ftCommonDashProcUpdate;
        fp->proc_interrupt = ftCommonDashProcInterrupt;
        fp->proc_physics = ftCommonDashProcPhysics;
        fp->proc_map = ftCommonDashProcMap;
        break;
    case nFTCommonStatusRun:
        fp->motion_id = nFTCommonMotionRun;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonRunProcInterrupt;
        fp->proc_physics = ftPhysicsSetGroundVelTransferAir;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        break;
    case nFTCommonStatusRunBrake:
        fp->motion_id = nFTCommonMotionRunBrake;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonRunBrakeProcInterrupt;
        fp->proc_physics = ftCommonRunBrakeProcPhysics;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        break;
    case nFTCommonStatusKneeBend:
        fp->motion_id = nFTCommonMotionKneeBend;
        fp->proc_update = ftCommonKneeBendProcUpdate;
        fp->proc_interrupt = ftCommonKneeBendProcInterrupt;
        fp->proc_physics = NULL;
        fp->proc_map = NULL;
        break;
    case nFTCommonStatusJumpF:
        fp->motion_id = nFTCommonMotionJumpF;
        fp->proc_update = ftAnimEndSetFall;
        fp->proc_interrupt = ftCommonJumpProcInterrupt;
        fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
        fp->proc_map = mpCommonProcFighterCliffFloorCeil;
        break;
    case nFTCommonStatusFall:
        fp->motion_id = nFTCommonMotionFall;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonFallProcInterrupt;
        fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
        fp->proc_map = mpCommonProcFighterCliffFloorCeil;
        break;
    case nFTCommonStatusLandingLight:
        fp->motion_id = nFTCommonMotionLandingLight;
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = ftCommonLandingProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        break;
    default:
        break;
    }
    fp->motion_script_id = fp->motion_id;
    if (DObjGetStruct(fighter_gobj) != NULL)
    {
        DObjGetStruct(fighter_gobj)->anim_speed = anim_speed;
    }
}


static void ndsFighterProcessLoopRunUpdate(u32 slot, FTStruct *fp)
{
    if ((fp != NULL) && (fp->proc_update != NULL) && (fp->fighter_gobj != NULL))
    {
        if ((sNdsFighterProcessLoopActive != FALSE) &&
            ((fp->proc_update == ftAnimEndSetFall) ||
             (fp->proc_update == ftAnimEndSetWait)))
        {
            return;
        }
        sNdsFighterProcessLoopUpdateActive = TRUE;
        fp->proc_update(fp->fighter_gobj);
        sNdsFighterProcessLoopUpdateActive = FALSE;
        if (slot == 0u)
        {
            gNdsFighterProcessLoopP0UpdateCount++;
        }
        else if (slot == 1u)
        {
            gNdsFighterProcessLoopP1UpdateCount++;
        }
    }
}

static void ndsFighterProcessLoopRunInterrupt(u32 slot, FTStruct *fp)
{
    if ((fp != NULL) && (fp->proc_interrupt != NULL) &&
        (fp->fighter_gobj != NULL))
    {
        sNdsFighterProcessLoopInterruptActive = TRUE;
        fp->proc_interrupt(fp->fighter_gobj);
        sNdsFighterProcessLoopInterruptActive = FALSE;
        if (slot == 0u)
        {
            gNdsFighterProcessLoopP0InterruptCount++;
        }
        else if (slot == 1u)
        {
            gNdsFighterProcessLoopP1InterruptCount++;
        }
    }
}

static void ndsFighterProcessLoopRunPhysics(u32 slot, FTStruct *fp)
{
    if ((fp != NULL) && (fp->proc_physics != NULL) &&
        (fp->fighter_gobj != NULL))
    {
        sNdsFighterProcessLoopPhysicsActive = TRUE;
        fp->proc_physics(fp->fighter_gobj);
        sNdsFighterProcessLoopPhysicsActive = FALSE;
        if (slot == 0u)
        {
            gNdsFighterProcessLoopP0PhysicsCount++;
        }
        else if (slot == 1u)
        {
            gNdsFighterProcessLoopP1PhysicsCount++;
        }
    }
}

static void ndsFighterProcessLoopIntegrate(u32 slot, FTStruct *fp)
{
    DObj *root = (fp != NULL) ? fp->joints[nFTPartsJointTopN] : NULL;

    if (root == NULL)
    {
        gNdsFighterProcessLoopProcessAttachCount++;
        return;
    }
    root->translate.vec.f.x += fp->physics.vel_air.x;
    root->translate.vec.f.y += fp->physics.vel_air.y;
    root->translate.vec.f.z += fp->physics.vel_air.z;
    if (slot == 0u)
    {
        gNdsFighterProcessLoopP0IntegrateCount++;
    }
    else if (slot == 1u)
    {
        gNdsFighterProcessLoopP1IntegrateCount++;
    }
}

static void ndsFighterProcessLoopRunMap(u32 slot, FTStruct *fp)
{
    if ((fp != NULL) && (fp->proc_map != NULL) && (fp->fighter_gobj != NULL))
    {
        sNdsFighterProcessLoopMapActive = TRUE;
        fp->proc_map(fp->fighter_gobj);
        sNdsFighterProcessLoopMapActive = FALSE;
        if (slot == 0u)
        {
            gNdsFighterProcessLoopP0MapCount++;
        }
        else if (slot == 1u)
        {
            gNdsFighterProcessLoopP1MapCount++;
        }
    }
}

static void ndsFighterProcessLoopRunFrame(u32 slot, FTStruct *fp)
{
    if ((fp == NULL) || (fp->fighter_gobj == NULL))
    {
        return;
    }

    fp->status_total_tics++;
    if (fp->hitlag_tics == 0)
    {
        ftMainPlayAnimEventsAll(fp->fighter_gobj);
    }
    ndsFighterProcessLoopRunUpdate(slot, fp);
    ndsFighterProcessLoopRunInterrupt(slot, fp);
    fp->physics.vel_jostle_x = 0.0F;
    fp->physics.vel_jostle_z = 0.0F;
    if (fp->joints[nFTPartsJointTopN] != NULL)
    {
        fp->coll_data.pos_prev = fp->joints[nFTPartsJointTopN]->translate.vec.f;
    }
    ndsFighterSyncLegacyVelToPhysics(fp);
    ndsFighterProcessLoopRunPhysics(slot, fp);
    ndsFighterProcessLoopIntegrate(slot, fp);
    ndsFighterSyncPhysicsToLegacyVel(fp);
    ndsFighterProcessLoopRunMap(slot, fp);
}

static u32 ndsFighterProcessLoopStatusBit(s32 status_id)
{
    if (status_id == nFTCommonStatusWait)
    {
        return 1u << 0;
    }
    if ((status_id >= nFTCommonStatusWalkSlow) &&
        (status_id <= nFTCommonStatusWalkFast))
    {
        return 1u << 1;
    }
    if (status_id == nFTCommonStatusDash)
    {
        return 1u << 2;
    }
    if (status_id == nFTCommonStatusRun)
    {
        return 1u << 3;
    }
    if (status_id == nFTCommonStatusRunBrake)
    {
        return 1u << 4;
    }
    if (status_id == nFTCommonStatusKneeBend)
    {
        return 1u << 5;
    }
    if (status_id == nFTCommonStatusJumpF)
    {
        return 1u << 6;
    }
    if (status_id == nFTCommonStatusFall)
    {
        return 1u << 7;
    }
    if (status_id == nFTCommonStatusLandingLight)
    {
        return 1u << 8;
    }
    return 0u;
}

static u32 ndsFighterProcessLoopTransitionBit(s32 previous_status,
                                              s32 status_id)
{
    if ((previous_status == nFTCommonStatusWait) &&
        (status_id >= nFTCommonStatusWalkSlow) &&
        (status_id <= nFTCommonStatusWalkFast))
    {
        return 1u << 0;
    }
    if ((previous_status >= nFTCommonStatusWalkSlow) &&
        (previous_status <= nFTCommonStatusWalkFast) &&
        (status_id == nFTCommonStatusWait))
    {
        return 1u << 1;
    }
    if ((previous_status == nFTCommonStatusWait) &&
        (status_id == nFTCommonStatusDash))
    {
        return 1u << 2;
    }
    if ((previous_status == nFTCommonStatusDash) &&
        (status_id == nFTCommonStatusRun))
    {
        return 1u << 3;
    }
    if ((previous_status == nFTCommonStatusRun) &&
        (status_id == nFTCommonStatusRunBrake))
    {
        return 1u << 4;
    }
    if ((previous_status == nFTCommonStatusRunBrake) &&
        (status_id == nFTCommonStatusWait))
    {
        return 1u << 5;
    }
    if ((previous_status == nFTCommonStatusWait) &&
        (status_id == nFTCommonStatusKneeBend))
    {
        return 1u << 6;
    }
    if ((previous_status == nFTCommonStatusKneeBend) &&
        (status_id == nFTCommonStatusJumpF))
    {
        return 1u << 7;
    }
    if ((previous_status == nFTCommonStatusJumpF) &&
        (status_id == nFTCommonStatusFall))
    {
        return 1u << 8;
    }
    if ((previous_status == nFTCommonStatusFall) &&
        (status_id == nFTCommonStatusLandingLight))
    {
        return 1u << 9;
    }
    if ((previous_status == nFTCommonStatusLandingLight) &&
        (status_id == nFTCommonStatusWait))
    {
        return 1u << 10;
    }
    return 0u;
}



#define NDS_FIGHTER_SCHEDULER_LOOP_FRAME_MAX 160u
#define NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX 180u
#define NDS_FIGHTER_SCHEDULER_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_SCHEDULER_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu

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

static NDSFighterProcessLoopSnapshot sNdsFighterProcessLoopSnapshot;
static sb32 sNdsFighterProcessLoopSnapshotValid;

static void ndsFighterSchedulerLoopSaveProcessLoopSnapshot(void)
{
#define NDS_SNAPSHOT_SAVE(name) sNdsFighterProcessLoopSnapshot.name = name;
    NDS_PROCESS_LOOP_SNAPSHOT_U32(NDS_SNAPSHOT_SAVE)
    NDS_PROCESS_LOOP_SNAPSHOT_S32(NDS_SNAPSHOT_SAVE)
#undef NDS_SNAPSHOT_SAVE
    sNdsFighterProcessLoopSnapshotValid = TRUE;
}

static void ndsFighterSchedulerLoopRestoreProcessLoopSnapshot(void)
{
    if (sNdsFighterProcessLoopSnapshotValid == FALSE)
    {
        return;
    }
#define NDS_SNAPSHOT_RESTORE(name) name = sNdsFighterProcessLoopSnapshot.name;
    NDS_PROCESS_LOOP_SNAPSHOT_U32(NDS_SNAPSHOT_RESTORE)
    NDS_PROCESS_LOOP_SNAPSHOT_S32(NDS_SNAPSHOT_RESTORE)
#undef NDS_SNAPSHOT_RESTORE
}

static void ndsFighterSchedulerLoopRecordStart(u32 slot, FTStruct *fp,
                                               DObj *root)
{
    NDSFighterSchedulerLoopState *state;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    bzero(state, sizeof(*state));
    state->phase = nNDSFighterSchedulerLoopPhaseWalkStart;
    state->root_y_start = root->translate.vec.f.y;
    state->root_y_max = root->translate.vec.f.y;

    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0StatusStart = (u32)fp->status_id;
        gNdsFighterSchedulerLoopP0MotionStart = (u32)fp->motion_id;
        gNdsFighterSchedulerLoopP0RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterSchedulerLoopP0FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    }
    else
    {
        gNdsFighterSchedulerLoopP1StatusStart = (u32)fp->status_id;
        gNdsFighterSchedulerLoopP1MotionStart = (u32)fp->motion_id;
        gNdsFighterSchedulerLoopP1RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterSchedulerLoopP1FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    }
    ndsFighterSchedulerLoopRecordState(slot, fp, nFTStatusIDNone, fp->ga);
}

static void ndsFighterSchedulerLoopApplyPhaseInput(
    u32 slot, FTStruct *fp, NDSFighterScriptInput *input)
{
    NDSFighterSchedulerLoopState *state;
    s32 lr_sign;

    if ((slot >= 2u) || (fp == NULL) || (input == NULL))
    {
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    lr_sign = (fp->lr >= 0) ? 1 : -1;
    bzero(input, sizeof(*input));

    switch (state->phase)
    {
    case nNDSFighterSchedulerLoopPhaseWalkStart:
    case nNDSFighterSchedulerLoopPhaseWalkHold:
        input->stick_x = (s8)(40 * lr_sign);
        input->hold_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
        break;
    case nNDSFighterSchedulerLoopPhaseDashStart:
    case nNDSFighterSchedulerLoopPhaseRunHold:
        input->stick_x = (s8)(80 * lr_sign);
        input->tap_stick_x =
            (state->phase == nNDSFighterSchedulerLoopPhaseDashStart) ?
            0u : FTINPUT_STICKBUFFER_TICS_MAX;
        input->hold_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
        break;
    case nNDSFighterSchedulerLoopPhaseJumpStart:
    case nNDSFighterSchedulerLoopPhaseJumpAir:
        input->stick_x = (s8)(40 * lr_sign);
        input->button_hold = U_CBUTTONS;
        input->button_tap =
            (state->phase == nNDSFighterSchedulerLoopPhaseJumpStart) ?
            U_CBUTTONS : 0u;
        input->tap_stick_y =
            (state->phase == nNDSFighterSchedulerLoopPhaseJumpStart) ?
            FTINPUT_STICKBUFFER_TICS_MAX : 0u;
        break;
    default:
        break;
    }

    fp->input.pl.stick_range.x = input->stick_x;
    fp->input.pl.stick_range.y = input->stick_y;
    fp->input.pl.button_hold = input->button_hold;
    fp->input.pl.button_tap = input->button_tap;
    fp->input.pl.button_release = input->button_release;
    fp->tap_stick_x = input->tap_stick_x;
    fp->tap_stick_y = input->tap_stick_y;
    fp->hold_stick_x = input->hold_stick_x;
    fp->hold_stick_y = input->hold_stick_y;

    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0InputApplyCount++;
        gNdsFighterSchedulerLoopP0ButtonTapMask |= input->button_tap;
        gNdsFighterSchedulerLoopP0LastStickX = input->stick_x;
    }
    else
    {
        gNdsFighterSchedulerLoopP1InputApplyCount++;
        gNdsFighterSchedulerLoopP1ButtonTapMask |= input->button_tap;
        gNdsFighterSchedulerLoopP1LastStickX = input->stick_x;
    }

    if (slot < MAXCONTROLLERS)
    {
        gSYControllerDevices[slot].button_hold = input->button_hold;
        gSYControllerDevices[slot].button_tap = input->button_tap;
        gSYControllerDevices[slot].button_release = input->button_release;
        gSYControllerDevices[slot].stick_range.x = input->stick_x;
        gSYControllerDevices[slot].stick_range.y = input->stick_y;
        gNdsFighterSchedulerLoopControllerBridgeCount++;
        if ((gSYControllerDevices[slot].button_tap ==
                fp->input.pl.button_tap) &&
            (gSYControllerDevices[slot].button_hold ==
                fp->input.pl.button_hold) &&
            (gSYControllerDevices[slot].button_release ==
                fp->input.pl.button_release) &&
            (gSYControllerDevices[slot].stick_range.x ==
                fp->input.pl.stick_range.x) &&
            (gSYControllerDevices[slot].stick_range.y ==
                fp->input.pl.stick_range.y))
        {
            gNdsFighterSchedulerLoopControllerMirrorCount++;
        }
    }
}

static void ndsFighterSchedulerLoopRecordState(u32 slot, FTStruct *fp,
                                               s32 previous_status,
                                               s32 previous_ga)
{
    NDSFighterSchedulerLoopState *state;
    u32 transition_bit;

    if ((slot >= 2u) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    state->status_visit_mask |= ndsFighterProcessLoopStatusBit(fp->status_id);
    transition_bit = ndsFighterProcessLoopTransitionBit(previous_status,
                                                       fp->status_id);
    state->transition_mask |= transition_bit;

    if ((previous_ga == nMPKineticsGround) && (fp->ga == nMPKineticsAir))
    {
        gNdsFighterSchedulerLoopSetAirCount++;
    }
    if ((previous_ga == nMPKineticsAir) && (fp->ga == nMPKineticsGround))
    {
        gNdsFighterSchedulerLoopSetGroundCount++;
    }
    if ((transition_bit & (1u << 5)) != 0u)
    {
        gNdsFighterSchedulerLoopRunBrakeEndCount++;
    }
    if ((transition_bit & (1u << 8)) != 0u)
    {
        gNdsFighterSchedulerLoopJumpAnimEndCount++;
    }
    if ((transition_bit & (1u << 9)) != 0u)
    {
        gNdsFighterSchedulerLoopFallDetectCount++;
        gNdsFighterSchedulerLoopLandingDetectCount++;
    }
    if ((transition_bit & (1u << 10)) != 0u)
    {
        gNdsFighterSchedulerLoopLandingEndCount++;
    }
    if ((fp->status_id == nFTCommonStatusWait) &&
        (previous_status != nFTCommonStatusWait))
    {
        gNdsFighterSchedulerLoopWaitSetStatusCount++;
    }

    switch (fp->status_id)
    {
    case nFTCommonStatusWait: state->wait_visit_count++; break;
    case nFTCommonStatusWalkSlow:
    case nFTCommonStatusWalkMiddle:
    case nFTCommonStatusWalkFast: state->walk_visit_count++; break;
    case nFTCommonStatusDash: state->dash_visit_count++; break;
    case nFTCommonStatusRun: state->run_visit_count++; break;
    case nFTCommonStatusRunBrake: state->runbrake_visit_count++; break;
    case nFTCommonStatusKneeBend: state->kneebend_visit_count++; break;
    case nFTCommonStatusJumpF: state->jump_visit_count++; break;
    case nFTCommonStatusFall: state->fall_visit_count++; break;
    case nFTCommonStatusLandingLight: state->landing_visit_count++; break;
    default:
        gNdsFighterSchedulerLoopUnexpectedStatusCount++;
        break;
    }

    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterSchedulerLoopP0TransitionMask = state->transition_mask;
        gNdsFighterSchedulerLoopP0WaitVisitCount = state->wait_visit_count;
        gNdsFighterSchedulerLoopP0WalkVisitCount = state->walk_visit_count;
        gNdsFighterSchedulerLoopP0DashVisitCount = state->dash_visit_count;
        gNdsFighterSchedulerLoopP0RunVisitCount = state->run_visit_count;
        gNdsFighterSchedulerLoopP0RunBrakeVisitCount =
            state->runbrake_visit_count;
        gNdsFighterSchedulerLoopP0KneeBendVisitCount =
            state->kneebend_visit_count;
        gNdsFighterSchedulerLoopP0JumpVisitCount = state->jump_visit_count;
        gNdsFighterSchedulerLoopP0FallVisitCount = state->fall_visit_count;
        gNdsFighterSchedulerLoopP0LandingVisitCount =
            state->landing_visit_count;
    }
    else
    {
        gNdsFighterSchedulerLoopP1StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterSchedulerLoopP1TransitionMask = state->transition_mask;
        gNdsFighterSchedulerLoopP1WaitVisitCount = state->wait_visit_count;
        gNdsFighterSchedulerLoopP1WalkVisitCount = state->walk_visit_count;
        gNdsFighterSchedulerLoopP1DashVisitCount = state->dash_visit_count;
        gNdsFighterSchedulerLoopP1RunVisitCount = state->run_visit_count;
        gNdsFighterSchedulerLoopP1RunBrakeVisitCount =
            state->runbrake_visit_count;
        gNdsFighterSchedulerLoopP1KneeBendVisitCount =
            state->kneebend_visit_count;
        gNdsFighterSchedulerLoopP1JumpVisitCount = state->jump_visit_count;
        gNdsFighterSchedulerLoopP1FallVisitCount = state->fall_visit_count;
        gNdsFighterSchedulerLoopP1LandingVisitCount =
            state->landing_visit_count;
    }
}

static void ndsFighterSchedulerLoopAdvancePhase(u32 slot, FTStruct *fp)
{
    NDSFighterSchedulerLoopState *state;
    GObj *fighter_gobj;
    DObj *root;
    FTAttributes *attr;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    fighter_gobj = fp->fighter_gobj;
    root = fp->joints[nFTPartsJointTopN];
    attr = fp->attr;

    switch (state->phase)
    {
    case nNDSFighterSchedulerLoopPhaseWalkStart:
        if ((fp->status_id >= nFTCommonStatusWalkSlow) &&
            (fp->status_id <= nFTCommonStatusWalkFast))
        {
            state->phase = nNDSFighterSchedulerLoopPhaseWalkHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseWalkHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseWalkRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseWalkRelease:
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseDashStart;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseDashStart:
        if (fp->status_id == nFTCommonStatusRun)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusDash) && (attr != NULL))
        {
            fighter_gobj->anim_frame = attr->dash_to_run;
            sNdsFighterProcessLoopInterruptActive = TRUE;
            ftCommonRunSetStatus(fighter_gobj);
            sNdsFighterProcessLoopInterruptActive = FALSE;
            ndsFighterSchedulerLoopRecordState(
                slot, fp, nFTCommonStatusDash, nMPKineticsGround);
            state->phase = nNDSFighterSchedulerLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseRunHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseRunRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseRunRelease:
        if (fp->status_id == nFTCommonStatusRunBrake)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseRunBrakeEnd;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseRunBrakeEnd:
        if (++state->phase_frame >= 2u)
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopRunBrakeEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopRunBrakeEndActive = FALSE;
            ndsFighterSchedulerLoopRecordState(
                slot, fp, nFTCommonStatusRunBrake, nMPKineticsGround);
        }
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseJumpStart;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseJumpStart:
        if (fp->status_id == nFTCommonStatusJumpF)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusKneeBend) &&
                 (++state->phase_frame >= 3u))
        {
            sNdsFighterProcessLoopUpdateActive = TRUE;
            ftCommonJumpSetStatus(fighter_gobj);
            sNdsFighterProcessLoopUpdateActive = FALSE;
            if (fp->physics.vel_air.y <= 0.0F)
            {
                fp->physics.vel_air.y = 4.0F;
                fp->vel_air = fp->physics.vel_air;
            }
            if (root != NULL)
            {
                root->translate.vec.f.y += fp->physics.vel_air.y;
                if (root->translate.vec.f.y > state->root_y_max)
                {
                    state->root_y_max = root->translate.vec.f.y;
                }
            }
            ndsFighterSchedulerLoopRecordState(
                slot, fp, nFTCommonStatusKneeBend, nMPKineticsGround);
            state->phase = nNDSFighterSchedulerLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseJumpAir:
        if (++state->phase_frame >= 6u)
        {
            sNdsFighterProcessLoopJumpAnimEndActive = TRUE;
            ftAnimEndSetFall(fighter_gobj);
            sNdsFighterProcessLoopJumpAnimEndActive = FALSE;
            ndsFighterSchedulerLoopRecordState(
                slot, fp, nFTCommonStatusJumpF, nMPKineticsAir);
            fp->physics.vel_air.y = -6.0F;
            fp->vel_air = fp->physics.vel_air;
            state->phase = nNDSFighterSchedulerLoopPhaseFallLand;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseFallLand:
        if ((fp->status_id == nFTCommonStatusLandingLight) &&
            (++state->phase_frame >= 5u))
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopLandingEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopLandingEndActive = FALSE;
            ndsFighterSchedulerLoopRecordState(
                slot, fp, nFTCommonStatusLandingLight, nMPKineticsGround);
            state->phase = nNDSFighterSchedulerLoopPhaseDone;
            state->completed = 1u;
        }
        break;
    default:
        break;
    }
}

static void ndsFighterSchedulerLoopRunSlotProcess(u32 slot, FTStruct *fp)
{
    NDSFighterSchedulerLoopState *state;
    NDSFighterScriptInput input;
    s32 previous_status;
    s32 previous_ga;
    DObj *root;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        gNdsFighterSchedulerLoopProcessAttachEscapeCount++;
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    if ((state->completed != 0u) ||
        (state->total_frames >= NDS_FIGHTER_SCHEDULER_LOOP_FRAME_MAX))
    {
        return;
    }
    previous_status = fp->status_id;
    previous_ga = fp->ga;
    root = fp->joints[nFTPartsJointTopN];

    ndsFighterSchedulerLoopApplyPhaseInput(slot, fp, &input);
    sNdsFighterSchedulerLoopActive = TRUE;
    sNdsFighterProcessLoopActive = TRUE;
    ndsFighterProcessLoopRunFrame(slot, fp);
    sNdsFighterProcessLoopActive = FALSE;
    sNdsFighterSchedulerLoopActive = FALSE;
    ndsFighterSchedulerLoopRecordState(slot, fp, previous_status,
                                       previous_ga);
    if ((root != NULL) && (root->translate.vec.f.y > state->root_y_max))
    {
        state->root_y_max = root->translate.vec.f.y;
    }
    ndsFighterSchedulerLoopAdvancePhase(slot, fp);
    state->total_frames++;
    gNdsFighterSchedulerLoopDeferredInterruptCheckCount++;

    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0FrameCount = state->total_frames;
        gNdsFighterSchedulerLoopP0UpdateCount =
            gNdsFighterProcessLoopP0UpdateCount;
        gNdsFighterSchedulerLoopP0InterruptCount++;
        gNdsFighterSchedulerLoopP0PhysicsCount++;
        gNdsFighterSchedulerLoopP0IntegrateCount++;
        gNdsFighterSchedulerLoopP0MapCount++;
    }
    else
    {
        gNdsFighterSchedulerLoopP1FrameCount = state->total_frames;
        gNdsFighterSchedulerLoopP1UpdateCount =
            gNdsFighterProcessLoopP1UpdateCount;
        gNdsFighterSchedulerLoopP1InterruptCount++;
        gNdsFighterSchedulerLoopP1PhysicsCount++;
        gNdsFighterSchedulerLoopP1IntegrateCount++;
        gNdsFighterSchedulerLoopP1MapCount++;
    }
}

static void ndsFighterSchedulerLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 slot = 2u;

    if ((fp != NULL) && (fp->player < 2))
    {
        slot = fp->player;
    }
    if ((slot >= 2u) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        gNdsFighterSchedulerLoopProcessAttachEscapeCount++;
        return;
    }
    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0ProcCallbackCount++;
    }
    else
    {
        gNdsFighterSchedulerLoopP1ProcCallbackCount++;
    }
    ndsFighterSchedulerLoopRunSlotProcess(slot, fp);
}

static void ndsFighterSchedulerLoopRecordFinal(u32 slot, FTStruct *fp,
                                               DObj *root)
{
    NDSFighterSchedulerLoopState *state;
    s32 root_y_final;
    s32 floor_y;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    if ((state->completed != 0u) && (fp->status_id == nFTCommonStatusWait))
    {
        state->status_visit_mask |= 1u << 9;
    }
    root_y_final = ndsFloatToMilliSigned(root->translate.vec.f.y);
    floor_y = ndsFloatToMilliSigned(fp->coll_data.floor_dist);

    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0Completed = state->completed;
        gNdsFighterSchedulerLoopP0StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterSchedulerLoopP0TransitionMask = state->transition_mask;
        gNdsFighterSchedulerLoopP0StatusFinal = (u32)fp->status_id;
        gNdsFighterSchedulerLoopP0MotionFinal = (u32)fp->motion_id;
        gNdsFighterSchedulerLoopP0GAFinal = (u32)fp->ga;
        gNdsFighterSchedulerLoopP0RootXFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterSchedulerLoopP0RootYFinalMilli = root_y_final;
        gNdsFighterSchedulerLoopP0RootDeltaXMilli =
            gNdsFighterSchedulerLoopP0RootXFinalMilli -
            gNdsFighterSchedulerLoopP0RootXStartMilli;
        gNdsFighterSchedulerLoopP0RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterSchedulerLoopP0GroundVelFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterSchedulerLoopP0AirVelXFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterSchedulerLoopP0AirVelYFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
        gNdsFighterSchedulerLoopP0RootDirectionOK =
            ((gNdsFighterSchedulerLoopP0RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterSchedulerLoopP0FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }
    else
    {
        gNdsFighterSchedulerLoopP1Completed = state->completed;
        gNdsFighterSchedulerLoopP1StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterSchedulerLoopP1TransitionMask = state->transition_mask;
        gNdsFighterSchedulerLoopP1StatusFinal = (u32)fp->status_id;
        gNdsFighterSchedulerLoopP1MotionFinal = (u32)fp->motion_id;
        gNdsFighterSchedulerLoopP1GAFinal = (u32)fp->ga;
        gNdsFighterSchedulerLoopP1RootXFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterSchedulerLoopP1RootYFinalMilli = root_y_final;
        gNdsFighterSchedulerLoopP1RootDeltaXMilli =
            gNdsFighterSchedulerLoopP1RootXFinalMilli -
            gNdsFighterSchedulerLoopP1RootXStartMilli;
        gNdsFighterSchedulerLoopP1RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterSchedulerLoopP1GroundVelFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterSchedulerLoopP1AirVelXFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterSchedulerLoopP1AirVelYFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
        gNdsFighterSchedulerLoopP1RootDirectionOK =
            ((gNdsFighterSchedulerLoopP1RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterSchedulerLoopP1FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }

    (void)root_y_final;
    (void)floor_y;
}

void ndsFighterMarioFoxSchedulerLoopPrepare(void)
{
    u32 i;

    if ((ndsFighterMarioFoxSchedulerLoopProofEnabled() == FALSE) ||
        (gNdsFighterSchedulerLoopPrepared != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxProcessLoopResult !=
            NDS_FIGHTER_MARIOFOX_PROCESS_LOOP_PASS) ||
        (gNdsFighterMarioFoxProcessLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_PROCESS_LOOP_SAFE_PASS) ||
        ((gNdsFighterMarioFoxProcessLoopMask & 0x7ffu) != 0x7ffu) ||
        (gNdsFighterMarioFoxProcessLoopDeferredMask != 0xffu) ||
        (gNdsFighterMarioFoxProcessLoopCount != 2u) ||
        (gNdsFighterProcessLoopP0StatusFinal != (u32)nFTCommonStatusWait) ||
        (gNdsFighterProcessLoopP1StatusFinal != (u32)nFTCommonStatusWait) ||
        (gNdsFighterProcessLoopP0MotionFinal != (u32)nFTCommonMotionWait) ||
        (gNdsFighterProcessLoopP1MotionFinal != (u32)nFTCommonMotionWait) ||
        (gNdsFighterProcessLoopP0GAFinal != (u32)nMPKineticsGround) ||
        (gNdsFighterProcessLoopP1GAFinal != (u32)nMPKineticsGround))
    {
        return;
    }

    ndsFighterSchedulerLoopSaveProcessLoopSnapshot();
    gNdsFighterSchedulerLoopFrameMax = NDS_FIGHTER_SCHEDULER_LOOP_FRAME_MAX;
    gNdsFighterSchedulerLoopUpdateMax = NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX;
    gNdsFighterSchedulerLoopGObjCountBefore = (u32)gcGetGObjsActiveNum();

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj = fp->fighter_gobj;
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL) || (root == NULL))
        {
            gNdsFighterSchedulerLoopProcessAttachEscapeCount++;
            continue;
        }
        ndsFighterSchedulerLoopRecordStart(i, fp, root);
        sNdsFighterSchedulerLoopProcesses[i] =
            gcAddGObjProcess(fighter_gobj,
                             ndsFighterSchedulerLoopGObjProc,
                             nGCProcessKindFunc,
                             3);
        if (sNdsFighterSchedulerLoopProcesses[i] == NULL)
        {
            gNdsFighterSchedulerLoopProcessAttachEscapeCount++;
        }
        else if (i == 0u)
        {
            gNdsFighterSchedulerLoopP0ProcessAttachCount++;
        }
        else
        {
            gNdsFighterSchedulerLoopP1ProcessAttachCount++;
        }
    }

    if ((sNdsFighterSchedulerLoopProcesses[0] != NULL) &&
        (sNdsFighterSchedulerLoopProcesses[1] != NULL))
    {
        gNdsFighterSchedulerLoopPrepared = 1u;
    }
}

s32 ndsFighterMarioFoxSchedulerLoopUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxSchedulerLoopProofEnabled() != FALSE) &&
            (gNdsFighterSchedulerLoopPrepared != 0u) &&
            (gNdsFighterMarioFoxSchedulerLoopResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxSchedulerLoopRunVSBattleUpdate(void)
{
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxSchedulerLoopUpdateEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterSchedulerLoopVSBattleUpdateCount++;
    gNdsFighterSchedulerLoopSchedulerUpdateCount++;

    for (i = 0; i < 2u; i++)
    {
        if (sNdsFighterSchedulerLoopProcesses[i] == NULL)
        {
            continue;
        }
        if (i == 0u)
        {
            gNdsFighterSchedulerLoopP0GObjProcessRunCount++;
        }
        else
        {
            gNdsFighterSchedulerLoopP1GObjProcessRunCount++;
        }
        gcRunGObjProcess(sNdsFighterSchedulerLoopProcesses[i]);
    }

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root = fp->joints[nFTPartsJointTopN];
        ndsFighterSchedulerLoopRecordFinal(i, fp, root);
    }

    if ((gNdsFighterSchedulerLoopP0Completed == 1u) &&
        (gNdsFighterSchedulerLoopP1Completed == 1u))
    {
        gNdsFighterSchedulerLoopRootYDriftCount = 0u;
        gNdsFighterSchedulerLoopGADriftCount = 0u;
        if ((gNdsFighterSchedulerLoopP0StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterSchedulerLoopP1StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterSchedulerLoopP0MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterSchedulerLoopP1MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterSchedulerLoopP0GAFinal !=
                (u32)nMPKineticsGround) ||
            (gNdsFighterSchedulerLoopP1GAFinal !=
                (u32)nMPKineticsGround))
        {
            gNdsFighterSchedulerLoopGADriftCount++;
        }
        if ((gNdsFighterSchedulerLoopP0FloorOK != 1u) ||
            (gNdsFighterSchedulerLoopP1FloorOK != 1u))
        {
            gNdsFighterSchedulerLoopRootYDriftCount++;
        }
    }

    if ((gNdsFighterMarioFoxProcessLoopResult ==
            NDS_FIGHTER_MARIOFOX_PROCESS_LOOP_PASS) &&
        (gNdsFighterMarioFoxProcessLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_PROCESS_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsFighterSchedulerLoopPrepared == 1u) &&
        (gNdsFighterSchedulerLoopTaskmanUpdateCount > 0u) &&
        (gNdsFighterSchedulerLoopVSBattleUpdateCount > 0u) &&
        (gNdsFighterSchedulerLoopSchedulerUpdateCount > 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterSchedulerLoopP0ProcessAttachCount == 1u) &&
        (gNdsFighterSchedulerLoopP1ProcessAttachCount == 1u) &&
        (gNdsFighterSchedulerLoopP0GObjProcessRunCount > 0u) &&
        (gNdsFighterSchedulerLoopP1GObjProcessRunCount > 0u) &&
        (gNdsFighterSchedulerLoopP0ProcCallbackCount ==
            gNdsFighterSchedulerLoopP0GObjProcessRunCount) &&
        (gNdsFighterSchedulerLoopP1ProcCallbackCount ==
            gNdsFighterSchedulerLoopP1GObjProcessRunCount))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterSchedulerLoopP0InputApplyCount > 0u) &&
        (gNdsFighterSchedulerLoopP1InputApplyCount > 0u) &&
        (gNdsFighterSchedulerLoopControllerBridgeCount >=
            (gNdsFighterSchedulerLoopP0InputApplyCount +
             gNdsFighterSchedulerLoopP1InputApplyCount)) &&
        (gNdsFighterSchedulerLoopControllerMirrorCount > 0u) &&
        (gNdsFighterSchedulerLoopP0ButtonTapMask != 0u) &&
        (gNdsFighterSchedulerLoopP1ButtonTapMask != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterSchedulerLoopP0Completed == 1u) &&
        (gNdsFighterSchedulerLoopP1Completed == 1u) &&
        (gNdsFighterSchedulerLoopP0FrameCount > 0u) &&
        (gNdsFighterSchedulerLoopP1FrameCount > 0u) &&
        (gNdsFighterSchedulerLoopP0FrameCount <=
            gNdsFighterSchedulerLoopFrameMax) &&
        (gNdsFighterSchedulerLoopP1FrameCount <=
            gNdsFighterSchedulerLoopFrameMax))
    {
        mask |= 1u << 4;
    }
    if (((gNdsFighterSchedulerLoopP0StatusVisitMask &
            NDS_FIGHTER_SCHEDULER_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_SCHEDULER_LOOP_STATUS_MASK_REQUIRED) &&
        ((gNdsFighterSchedulerLoopP1StatusVisitMask &
            NDS_FIGHTER_SCHEDULER_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_SCHEDULER_LOOP_STATUS_MASK_REQUIRED))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterSchedulerLoopP0TransitionMask &
            NDS_FIGHTER_SCHEDULER_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_SCHEDULER_LOOP_TRANSITION_MASK_REQUIRED) &&
        ((gNdsFighterSchedulerLoopP1TransitionMask &
            NDS_FIGHTER_SCHEDULER_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_SCHEDULER_LOOP_TRANSITION_MASK_REQUIRED))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterSchedulerLoopP0InterruptCount > 0u) &&
        (gNdsFighterSchedulerLoopP1InterruptCount > 0u) &&
        (gNdsFighterSchedulerLoopP0PhysicsCount > 0u) &&
        (gNdsFighterSchedulerLoopP1PhysicsCount > 0u) &&
        (gNdsFighterSchedulerLoopP0IntegrateCount > 0u) &&
        (gNdsFighterSchedulerLoopP1IntegrateCount > 0u) &&
        (gNdsFighterSchedulerLoopP0MapCount > 0u) &&
        (gNdsFighterSchedulerLoopP1MapCount > 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterSchedulerLoopP0RootDeltaXMilli != 0) &&
        (gNdsFighterSchedulerLoopP1RootDeltaXMilli != 0) &&
        (gNdsFighterSchedulerLoopP0RootRiseMilli > 0) &&
        (gNdsFighterSchedulerLoopP1RootRiseMilli > 0) &&
        (gNdsFighterSchedulerLoopP0RootDirectionOK == 1u) &&
        (gNdsFighterSchedulerLoopP1RootDirectionOK == 1u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterSchedulerLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterSchedulerLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterSchedulerLoopP0MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterSchedulerLoopP1MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterSchedulerLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterSchedulerLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterSchedulerLoopP0FloorOK == 1u) &&
        (gNdsFighterSchedulerLoopP1FloorOK == 1u))
    {
        mask |= 1u << 9;
    }

    gNdsFighterSchedulerLoopGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterSchedulerLoopGObjDelta =
        (gNdsFighterSchedulerLoopGObjCountAfter >=
         gNdsFighterSchedulerLoopGObjCountBefore) ?
        (gNdsFighterSchedulerLoopGObjCountAfter -
         gNdsFighterSchedulerLoopGObjCountBefore) :
        (gNdsFighterSchedulerLoopGObjCountBefore -
         gNdsFighterSchedulerLoopGObjCountAfter);

    if ((gNdsFighterSchedulerLoopGObjDelta == 0u) &&
        (gNdsFighterSchedulerLoopUnexpectedStatusCount == 0u) &&
        (gNdsFighterSchedulerLoopDeniedStatusCount == 0u) &&
        (gNdsFighterSchedulerLoopProcessAttachEscapeCount == 0u) &&
        (gNdsFighterSchedulerLoopDisplayProbeCount == 0u) &&
        (gNdsFighterSchedulerLoopGameplayUpdateCount == 0u) &&
        (gNdsFighterSchedulerLoopDrawCallCount == 0u) &&
        (gNdsFighterSchedulerLoopMatrixCallCount == 0u) &&
        (gNdsFighterSchedulerLoopRootYDriftCount == 0u) &&
        (gNdsFighterSchedulerLoopGADriftCount == 0u))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxSchedulerLoopMask = mask;
    gNdsFighterMarioFoxSchedulerLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxSchedulerLoopCount =
        gNdsFighterSchedulerLoopP0Completed +
        gNdsFighterSchedulerLoopP1Completed;

    if ((mask & 0x7ffu) == 0x7ffu)
    {
        ndsFighterSchedulerLoopRestoreProcessLoopSnapshot();
        gNdsFighterMarioFoxSchedulerLoopResult =
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS;
        gNdsFighterMarioFoxSchedulerLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_SAFE_PASS;
    }
}

#define NDS_FIGHTER_CONTROLLER_LOOP_FRAME_MAX 180u
#define NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX 200u
#define NDS_FIGHTER_CONTROLLER_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_CONTROLLER_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu
#define NDS_FIGHTER_PREVIEW_LOOP_FRAME_MAX 180u
#define NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX 220u
#define NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_INTERVAL 8u
#define NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN 7u
#define NDS_FIGHTER_PREVIEW_LOOP_WIDTH 96u
#define NDS_FIGHTER_PREVIEW_LOOP_HEIGHT 72u
#define NDS_FIGHTER_PREVIEW_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_PREVIEW_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu
#define NDS_FIGHTER_GCRUNALL_LOOP_FRAME_MAX 180u
#define NDS_FIGHTER_GCRUNALL_LOOP_UPDATE_MAX 240u
#define NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_INTERVAL 8u
#define NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_MIN 7u
#define NDS_FIGHTER_GCRUNALL_LOOP_WIDTH 96u
#define NDS_FIGHTER_GCRUNALL_LOOP_HEIGHT 72u
#define NDS_FIGHTER_GCRUNALL_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_GCRUNALL_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu
#define NDS_FIGHTER_GCDRAWALL_LOOP_FRAME_MAX 180u
#define NDS_FIGHTER_GCDRAWALL_LOOP_UPDATE_MAX 240u
#define NDS_FIGHTER_GCDRAWALL_LOOP_DRAW_FRAME_INTERVAL 8u
#define NDS_FIGHTER_GCDRAWALL_LOOP_DRAW_FRAME_MIN 7u
#define NDS_FIGHTER_GCDRAWALL_LOOP_WIDTH 96u
#define NDS_FIGHTER_GCDRAWALL_LOOP_HEIGHT 72u
#define NDS_FIGHTER_GCDRAWALL_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_GCDRAWALL_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu
#define NDS_FIGHTER_LIVE_PREVIEW_IDLE_FRAME_TARGET 60u
#define NDS_FIGHTER_LIVE_PREVIEW_DEV_FRAME_TARGET 3600u
#define NDS_FIGHTER_LIVE_PREVIEW_DRAW_FRAME_INTERVAL 12u
#define NDS_FIGHTER_LIVE_PREVIEW_DRAW_FRAME_MIN 5u

static void ndsFighterControllerLoopRecordStart(u32 slot, FTStruct *fp,
                                                DObj *root)
{
    NDSFighterControllerLoopState *state;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    bzero(state, sizeof(*state));
    state->phase = nNDSFighterControllerLoopPhaseWalkStart;
    state->root_y_start = root->translate.vec.f.y;
    state->root_y_max = root->translate.vec.f.y;
    state->previous_stick_x = 0;
    state->previous_stick_y = 0;

    fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;

    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0StatusStart = (u32)fp->status_id;
        gNdsFighterControllerLoopP0MotionStart = (u32)fp->motion_id;
        gNdsFighterControllerLoopP0RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterControllerLoopP0FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    }
    else
    {
        gNdsFighterControllerLoopP1StatusStart = (u32)fp->status_id;
        gNdsFighterControllerLoopP1MotionStart = (u32)fp->motion_id;
        gNdsFighterControllerLoopP1RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterControllerLoopP1FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    }
    ndsFighterControllerLoopRecordState(slot, fp, nFTStatusIDNone, fp->ga);
}

static void ndsFighterControllerLoopApplyPlayback(u32 slot, FTStruct *fp)
{
    NDSFighterControllerLoopState *state;
    s32 lr_sign;
    s8 stick_x = 0;
    s8 stick_y = 0;
    u16 button = 0;

    if ((slot >= 2u) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    lr_sign = (fp->lr >= 0) ? 1 : -1;

    switch (state->phase)
    {
    case nNDSFighterControllerLoopPhaseWalkStart:
    case nNDSFighterControllerLoopPhaseWalkHold:
        stick_x = (s8)(40 * lr_sign);
        break;
    case nNDSFighterControllerLoopPhaseDashStart:
    case nNDSFighterControllerLoopPhaseRunHold:
        stick_x = (s8)(80 * lr_sign);
        break;
    case nNDSFighterControllerLoopPhaseJumpStart:
    case nNDSFighterControllerLoopPhaseJumpAir:
        stick_x = (s8)(40 * lr_sign);
        button = U_CBUTTONS;
        break;
    default:
        break;
    }

    ndsControllerPlaybackSetPad(slot, button, stick_x, stick_y);
    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0PlaybackApplyCount++;
        gNdsFighterControllerLoopP0ButtonHoldMask |= button;
    }
    else
    {
        gNdsFighterControllerLoopP1PlaybackApplyCount++;
        gNdsFighterControllerLoopP1ButtonHoldMask |= button;
    }
}

static void ndsFighterControllerLoopApplyFromSYController(u32 slot,
                                                          FTStruct *fp)
{
    NDSFighterControllerLoopState *state;
    SYController *controller;
    s32 stick_x_abs;
    s32 stick_y_abs;
    s32 previous_x_abs;
    s32 previous_y_abs;

    if ((slot >= 2u) || (slot >= MAXCONTROLLERS) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    controller = &gSYControllerDevices[slot];
    stick_x_abs = ABS(controller->stick_range.x);
    stick_y_abs = ABS(controller->stick_range.y);
    previous_x_abs = ABS(state->previous_stick_x);
    previous_y_abs = ABS(state->previous_stick_y);

    fp->input.pl.stick_range.x = controller->stick_range.x;
    fp->input.pl.stick_range.y = controller->stick_range.y;
    fp->input.pl.button_hold = controller->button_hold;
    fp->input.pl.button_tap = controller->button_tap;
    fp->input.pl.button_release = controller->button_release;

    if ((stick_x_abs >= FTCOMMON_DASH_STICK_RANGE_MIN) &&
        (previous_x_abs < FTCOMMON_DASH_STICK_RANGE_MIN))
    {
        fp->tap_stick_x = 0u;
        if (slot == 0u)
        {
            gNdsFighterControllerLoopP0DashTapEligibleCount++;
        }
        else
        {
            gNdsFighterControllerLoopP1DashTapEligibleCount++;
        }
    }
    else if (fp->tap_stick_x < FTINPUT_STICKBUFFER_TICS_MAX)
    {
        fp->tap_stick_x++;
    }
    else
    {
        fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    }

    if ((stick_y_abs >= FTCOMMON_KNEEBEND_STICK_RANGE_MIN) &&
        (previous_y_abs < FTCOMMON_KNEEBEND_STICK_RANGE_MIN))
    {
        fp->tap_stick_y = 0u;
    }
    else if (fp->tap_stick_y < FTINPUT_STICKBUFFER_TICS_MAX)
    {
        fp->tap_stick_y++;
    }
    else
    {
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    }
    fp->hold_stick_x = (stick_x_abs != 0) ? 0u : FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_y = (stick_y_abs != 0) ? 0u : FTINPUT_STICKBUFFER_TICS_MAX;

    state->previous_stick_x = controller->stick_range.x;
    state->previous_stick_y = controller->stick_range.y;

    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0ControllerToFTInputCount++;
        gNdsFighterControllerLoopP0ButtonTapMask |= controller->button_tap;
        gNdsFighterControllerLoopP0ButtonHoldMask |= controller->button_hold;
        gNdsFighterControllerLoopP0ButtonReleaseMask |=
            controller->button_release;
        gNdsFighterControllerLoopP0LastStickX = controller->stick_range.x;
        gNdsFighterControllerLoopP0LastStickY = controller->stick_range.y;
        if (fp->tap_stick_x < gNdsFighterControllerLoopP0TapStickXMin)
        {
            gNdsFighterControllerLoopP0TapStickXMin = fp->tap_stick_x;
        }
        if (fp->tap_stick_y < gNdsFighterControllerLoopP0TapStickYMin)
        {
            gNdsFighterControllerLoopP0TapStickYMin = fp->tap_stick_y;
        }
        if ((controller->button_tap & U_CBUTTONS) != 0u)
        {
            gNdsFighterControllerLoopP0JumpButtonTapCount++;
        }
    }
    else
    {
        gNdsFighterControllerLoopP1ControllerToFTInputCount++;
        gNdsFighterControllerLoopP1ButtonTapMask |= controller->button_tap;
        gNdsFighterControllerLoopP1ButtonHoldMask |= controller->button_hold;
        gNdsFighterControllerLoopP1ButtonReleaseMask |=
            controller->button_release;
        gNdsFighterControllerLoopP1LastStickX = controller->stick_range.x;
        gNdsFighterControllerLoopP1LastStickY = controller->stick_range.y;
        if (fp->tap_stick_x < gNdsFighterControllerLoopP1TapStickXMin)
        {
            gNdsFighterControllerLoopP1TapStickXMin = fp->tap_stick_x;
        }
        if (fp->tap_stick_y < gNdsFighterControllerLoopP1TapStickYMin)
        {
            gNdsFighterControllerLoopP1TapStickYMin = fp->tap_stick_y;
        }
        if ((controller->button_tap & U_CBUTTONS) != 0u)
        {
            gNdsFighterControllerLoopP1JumpButtonTapCount++;
        }
    }
}

static void ndsFighterControllerLoopRecordState(u32 slot, FTStruct *fp,
                                                s32 previous_status,
                                                s32 previous_ga)
{
    NDSFighterControllerLoopState *state;
    u32 transition_bit;

    if ((slot >= 2u) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    state->status_visit_mask |= ndsFighterProcessLoopStatusBit(fp->status_id);
    transition_bit = ndsFighterProcessLoopTransitionBit(previous_status,
                                                       fp->status_id);
    state->transition_mask |= transition_bit;

    if ((previous_ga == nMPKineticsGround) && (fp->ga == nMPKineticsAir))
    {
        gNdsFighterControllerLoopSetAirCount++;
    }
    if ((previous_ga == nMPKineticsAir) && (fp->ga == nMPKineticsGround))
    {
        gNdsFighterControllerLoopSetGroundCount++;
    }
    if ((transition_bit & (1u << 5)) != 0u)
    {
        gNdsFighterControllerLoopRunBrakeEndCount++;
    }
    if ((transition_bit & (1u << 8)) != 0u)
    {
        gNdsFighterControllerLoopJumpAnimEndCount++;
    }
    if ((transition_bit & (1u << 9)) != 0u)
    {
        gNdsFighterControllerLoopFallDetectCount++;
        gNdsFighterControllerLoopLandingDetectCount++;
    }
    if ((transition_bit & (1u << 10)) != 0u)
    {
        gNdsFighterControllerLoopLandingEndCount++;
    }
    if ((fp->status_id == nFTCommonStatusWait) &&
        (previous_status != nFTCommonStatusWait))
    {
        gNdsFighterControllerLoopWaitSetStatusCount++;
    }

    switch (fp->status_id)
    {
    case nFTCommonStatusWait: state->wait_visit_count++; break;
    case nFTCommonStatusWalkSlow:
    case nFTCommonStatusWalkMiddle:
    case nFTCommonStatusWalkFast: state->walk_visit_count++; break;
    case nFTCommonStatusDash: state->dash_visit_count++; break;
    case nFTCommonStatusRun: state->run_visit_count++; break;
    case nFTCommonStatusRunBrake: state->runbrake_visit_count++; break;
    case nFTCommonStatusKneeBend: state->kneebend_visit_count++; break;
    case nFTCommonStatusJumpF: state->jump_visit_count++; break;
    case nFTCommonStatusFall: state->fall_visit_count++; break;
    case nFTCommonStatusLandingLight: state->landing_visit_count++; break;
    default:
        gNdsFighterControllerLoopUnexpectedStatusCount++;
        break;
    }

    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterControllerLoopP0TransitionMask = state->transition_mask;
        gNdsFighterControllerLoopP0WaitVisitCount = state->wait_visit_count;
        gNdsFighterControllerLoopP0WalkVisitCount = state->walk_visit_count;
        gNdsFighterControllerLoopP0DashVisitCount = state->dash_visit_count;
        gNdsFighterControllerLoopP0RunVisitCount = state->run_visit_count;
        gNdsFighterControllerLoopP0RunBrakeVisitCount =
            state->runbrake_visit_count;
        gNdsFighterControllerLoopP0KneeBendVisitCount =
            state->kneebend_visit_count;
        gNdsFighterControllerLoopP0JumpVisitCount = state->jump_visit_count;
        gNdsFighterControllerLoopP0FallVisitCount = state->fall_visit_count;
        gNdsFighterControllerLoopP0LandingVisitCount =
            state->landing_visit_count;
    }
    else
    {
        gNdsFighterControllerLoopP1StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterControllerLoopP1TransitionMask = state->transition_mask;
        gNdsFighterControllerLoopP1WaitVisitCount = state->wait_visit_count;
        gNdsFighterControllerLoopP1WalkVisitCount = state->walk_visit_count;
        gNdsFighterControllerLoopP1DashVisitCount = state->dash_visit_count;
        gNdsFighterControllerLoopP1RunVisitCount = state->run_visit_count;
        gNdsFighterControllerLoopP1RunBrakeVisitCount =
            state->runbrake_visit_count;
        gNdsFighterControllerLoopP1KneeBendVisitCount =
            state->kneebend_visit_count;
        gNdsFighterControllerLoopP1JumpVisitCount = state->jump_visit_count;
        gNdsFighterControllerLoopP1FallVisitCount = state->fall_visit_count;
        gNdsFighterControllerLoopP1LandingVisitCount =
            state->landing_visit_count;
    }
}

static void ndsFighterControllerLoopAdvancePhase(u32 slot, FTStruct *fp)
{
    NDSFighterControllerLoopState *state;
    GObj *fighter_gobj;
    DObj *root;
    FTAttributes *attr;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    fighter_gobj = fp->fighter_gobj;
    root = fp->joints[nFTPartsJointTopN];
    attr = fp->attr;

    switch (state->phase)
    {
    case nNDSFighterControllerLoopPhaseWalkStart:
        if ((fp->status_id >= nFTCommonStatusWalkSlow) &&
            (fp->status_id <= nFTCommonStatusWalkFast))
        {
            state->phase = nNDSFighterControllerLoopPhaseWalkHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseWalkHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterControllerLoopPhaseWalkRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseWalkRelease:
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterControllerLoopPhaseDashStart;
            state->phase_frame = 0u;
            state->previous_stick_x = 0;
        }
        break;
    case nNDSFighterControllerLoopPhaseDashStart:
        if (fp->status_id == nFTCommonStatusRun)
        {
            state->phase = nNDSFighterControllerLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusDash) && (attr != NULL))
        {
            fighter_gobj->anim_frame = attr->dash_to_run;
            sNdsFighterProcessLoopInterruptActive = TRUE;
            ftCommonRunSetStatus(fighter_gobj);
            sNdsFighterProcessLoopInterruptActive = FALSE;
            ndsFighterControllerLoopRecordState(
                slot, fp, nFTCommonStatusDash, nMPKineticsGround);
            state->phase = nNDSFighterControllerLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseRunHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterControllerLoopPhaseRunRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseRunRelease:
        if (fp->status_id == nFTCommonStatusRunBrake)
        {
            state->phase = nNDSFighterControllerLoopPhaseRunBrakeEnd;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseRunBrakeEnd:
        if (++state->phase_frame >= 2u)
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopRunBrakeEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopRunBrakeEndActive = FALSE;
            ndsFighterControllerLoopRecordState(
                slot, fp, nFTCommonStatusRunBrake, nMPKineticsGround);
        }
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterControllerLoopPhaseJumpStart;
            state->phase_frame = 0u;
            state->previous_stick_x = 0;
            state->previous_stick_y = 0;
        }
        break;
    case nNDSFighterControllerLoopPhaseJumpStart:
        if (fp->status_id == nFTCommonStatusJumpF)
        {
            state->phase = nNDSFighterControllerLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusKneeBend) &&
                 (++state->phase_frame >= 3u))
        {
            sNdsFighterProcessLoopUpdateActive = TRUE;
            ftCommonJumpSetStatus(fighter_gobj);
            sNdsFighterProcessLoopUpdateActive = FALSE;
            if (fp->physics.vel_air.y <= 0.0F)
            {
                fp->physics.vel_air.y = 4.0F;
                fp->vel_air = fp->physics.vel_air;
            }
            if (root != NULL)
            {
                root->translate.vec.f.y += fp->physics.vel_air.y;
                if (root->translate.vec.f.y > state->root_y_max)
                {
                    state->root_y_max = root->translate.vec.f.y;
                }
            }
            ndsFighterControllerLoopRecordState(
                slot, fp, nFTCommonStatusKneeBend, nMPKineticsGround);
            state->phase = nNDSFighterControllerLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseJumpAir:
        if (++state->phase_frame >= 6u)
        {
            sNdsFighterProcessLoopJumpAnimEndActive = TRUE;
            ftAnimEndSetFall(fighter_gobj);
            sNdsFighterProcessLoopJumpAnimEndActive = FALSE;
            ndsFighterControllerLoopRecordState(
                slot, fp, nFTCommonStatusJumpF, nMPKineticsAir);
            fp->physics.vel_air.y = -6.0F;
            fp->vel_air = fp->physics.vel_air;
            state->phase = nNDSFighterControllerLoopPhaseFallLand;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseFallLand:
        if ((fp->status_id == nFTCommonStatusLandingLight) &&
            (++state->phase_frame >= 5u))
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopLandingEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopLandingEndActive = FALSE;
            ndsFighterControllerLoopRecordState(
                slot, fp, nFTCommonStatusLandingLight, nMPKineticsGround);
            state->phase = nNDSFighterControllerLoopPhaseDone;
            state->completed = 1u;
        }
        break;
    default:
        break;
    }
}

static void ndsFighterControllerLoopRunSlotProcess(u32 slot, FTStruct *fp)
{
    NDSFighterControllerLoopState *state;
    s32 previous_status;
    s32 previous_ga;
    DObj *root;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        gNdsFighterControllerLoopProcessAttachEscapeCount++;
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    if ((state->completed != 0u) ||
        (state->total_frames >= NDS_FIGHTER_CONTROLLER_LOOP_FRAME_MAX))
    {
        return;
    }
    previous_status = fp->status_id;
    previous_ga = fp->ga;
    root = fp->joints[nFTPartsJointTopN];

    ndsFighterControllerLoopApplyFromSYController(slot, fp);
    sNdsFighterControllerLoopActive = TRUE;
    sNdsFighterProcessLoopActive = TRUE;
    ndsFighterProcessLoopRunFrame(slot, fp);
    sNdsFighterProcessLoopActive = FALSE;
    sNdsFighterControllerLoopActive = FALSE;
    ndsFighterControllerLoopRecordState(slot, fp, previous_status,
                                        previous_ga);
    if ((root != NULL) && (root->translate.vec.f.y > state->root_y_max))
    {
        state->root_y_max = root->translate.vec.f.y;
    }
    ndsFighterControllerLoopAdvancePhase(slot, fp);
    state->total_frames++;
    gNdsFighterControllerLoopDeferredInterruptCheckCount++;

    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0FrameCount = state->total_frames;
        gNdsFighterControllerLoopP0UpdateCount =
            gNdsFighterProcessLoopP0UpdateCount;
        gNdsFighterControllerLoopP0InterruptCount++;
        gNdsFighterControllerLoopP0PhysicsCount++;
        gNdsFighterControllerLoopP0IntegrateCount++;
        gNdsFighterControllerLoopP0MapCount++;
    }
    else
    {
        gNdsFighterControllerLoopP1FrameCount = state->total_frames;
        gNdsFighterControllerLoopP1UpdateCount =
            gNdsFighterProcessLoopP1UpdateCount;
        gNdsFighterControllerLoopP1InterruptCount++;
        gNdsFighterControllerLoopP1PhysicsCount++;
        gNdsFighterControllerLoopP1IntegrateCount++;
        gNdsFighterControllerLoopP1MapCount++;
    }
}

static void ndsFighterControllerLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 slot = 2u;

    if ((fp != NULL) && (fp->player < 2))
    {
        slot = fp->player;
    }
    if ((slot >= 2u) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        gNdsFighterControllerLoopProcessAttachEscapeCount++;
        return;
    }
    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0ProcCallbackCount++;
    }
    else
    {
        gNdsFighterControllerLoopP1ProcCallbackCount++;
    }
    ndsFighterControllerLoopRunSlotProcess(slot, fp);
}

static void ndsFighterControllerLoopRecordFinal(u32 slot, FTStruct *fp,
                                                DObj *root)
{
    NDSFighterControllerLoopState *state;
    s32 root_y_final;
    s32 floor_y;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    if ((state->completed != 0u) && (fp->status_id == nFTCommonStatusWait))
    {
        state->status_visit_mask |= 1u << 9;
    }
    root_y_final = ndsFloatToMilliSigned(root->translate.vec.f.y);
    floor_y = ndsFloatToMilliSigned(fp->coll_data.floor_dist);

    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0Completed = state->completed;
        gNdsFighterControllerLoopP0StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterControllerLoopP0TransitionMask = state->transition_mask;
        gNdsFighterControllerLoopP0StatusFinal = (u32)fp->status_id;
        gNdsFighterControllerLoopP0MotionFinal = (u32)fp->motion_id;
        gNdsFighterControllerLoopP0GAFinal = (u32)fp->ga;
        gNdsFighterControllerLoopP0RootXFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterControllerLoopP0RootYFinalMilli = root_y_final;
        gNdsFighterControllerLoopP0RootDeltaXMilli =
            gNdsFighterControllerLoopP0RootXFinalMilli -
            gNdsFighterControllerLoopP0RootXStartMilli;
        gNdsFighterControllerLoopP0RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterControllerLoopP0GroundVelFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterControllerLoopP0AirVelXFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterControllerLoopP0AirVelYFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
        gNdsFighterControllerLoopP0RootDirectionOK =
            ((gNdsFighterControllerLoopP0RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterControllerLoopP0FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }
    else
    {
        gNdsFighterControllerLoopP1Completed = state->completed;
        gNdsFighterControllerLoopP1StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterControllerLoopP1TransitionMask = state->transition_mask;
        gNdsFighterControllerLoopP1StatusFinal = (u32)fp->status_id;
        gNdsFighterControllerLoopP1MotionFinal = (u32)fp->motion_id;
        gNdsFighterControllerLoopP1GAFinal = (u32)fp->ga;
        gNdsFighterControllerLoopP1RootXFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterControllerLoopP1RootYFinalMilli = root_y_final;
        gNdsFighterControllerLoopP1RootDeltaXMilli =
            gNdsFighterControllerLoopP1RootXFinalMilli -
            gNdsFighterControllerLoopP1RootXStartMilli;
        gNdsFighterControllerLoopP1RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterControllerLoopP1GroundVelFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterControllerLoopP1AirVelXFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterControllerLoopP1AirVelYFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
        gNdsFighterControllerLoopP1RootDirectionOK =
            ((gNdsFighterControllerLoopP1RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterControllerLoopP1FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }

    (void)root_y_final;
    (void)floor_y;
}

void ndsFighterMarioFoxControllerLoopPrepare(void)
{
    u32 i;

    if ((ndsFighterMarioFoxControllerLoopProofEnabled() == FALSE) ||
        (gNdsFighterControllerLoopPrepared != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxSchedulerLoopResult !=
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS) ||
        (gNdsFighterMarioFoxSchedulerLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_SAFE_PASS) ||
        ((gNdsFighterMarioFoxSchedulerLoopMask & 0x7ffu) != 0x7ffu) ||
        (gNdsFighterMarioFoxSchedulerLoopDeferredMask != 0xffu) ||
        (gNdsFighterMarioFoxSchedulerLoopCount != 2u) ||
        (gNdsFighterSchedulerLoopP0StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterSchedulerLoopP1StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterSchedulerLoopP0MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterSchedulerLoopP1MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterSchedulerLoopP0GAFinal != (u32)nMPKineticsGround) ||
        (gNdsFighterSchedulerLoopP1GAFinal != (u32)nMPKineticsGround))
    {
        return;
    }

    ndsFighterSchedulerLoopSaveProcessLoopSnapshot();
    ndsControllerPlaybackReset();
    ndsControllerPlaybackSetConnectedMask(0x3u);
    ndsControllerPlaybackSetEnabled(TRUE);
    gNdsFighterControllerLoopFrameMax = NDS_FIGHTER_CONTROLLER_LOOP_FRAME_MAX;
    gNdsFighterControllerLoopUpdateMax =
        NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX;
    gNdsFighterControllerLoopGObjCountBefore = (u32)gcGetGObjsActiveNum();

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj = fp->fighter_gobj;
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL) || (root == NULL))
        {
            gNdsFighterControllerLoopProcessAttachEscapeCount++;
            continue;
        }
        ndsFighterControllerLoopRecordStart(i, fp, root);
        sNdsFighterControllerLoopProcesses[i] =
            gcAddGObjProcess(fighter_gobj,
                             ndsFighterControllerLoopGObjProc,
                             nGCProcessKindFunc,
                             3);
        if (sNdsFighterControllerLoopProcesses[i] == NULL)
        {
            gNdsFighterControllerLoopProcessAttachEscapeCount++;
        }
        else if (i == 0u)
        {
            gNdsFighterControllerLoopP0ProcessAttachCount++;
        }
        else
        {
            gNdsFighterControllerLoopP1ProcessAttachCount++;
        }
    }

    if ((sNdsFighterControllerLoopProcesses[0] != NULL) &&
        (sNdsFighterControllerLoopProcesses[1] != NULL))
    {
        gNdsFighterControllerLoopPrepared = 1u;
    }
}

s32 ndsFighterMarioFoxControllerLoopUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxControllerLoopProofEnabled() != FALSE) &&
            (gNdsFighterControllerLoopPrepared != 0u) &&
            (gNdsFighterMarioFoxControllerLoopResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxControllerLoopRunVSBattleUpdate(void)
{
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxControllerLoopUpdateEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterControllerLoopVSBattleUpdateCount++;
    gNdsFighterControllerLoopSchedulerUpdateCount++;

    for (i = 0; i < 2u; i++)
    {
        ndsFighterControllerLoopApplyPlayback(i, &sNdsFighterStructPool[i]);
    }
    ndsControllerPlaybackCommitFrame();
    syControllerReadDeviceData();
    gNdsFighterControllerLoopSYReadCount++;
    syControllerUpdateGlobalData();
    gNdsFighterControllerLoopSYUpdateCount++;

    for (i = 0; i < 2u; i++)
    {
        if (sNdsFighterControllerLoopProcesses[i] == NULL)
        {
            continue;
        }
        if (i == 0u)
        {
            gNdsFighterControllerLoopP0GObjProcessRunCount++;
        }
        else
        {
            gNdsFighterControllerLoopP1GObjProcessRunCount++;
        }
        gcRunGObjProcess(sNdsFighterControllerLoopProcesses[i]);
    }

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root = fp->joints[nFTPartsJointTopN];
        ndsFighterControllerLoopRecordFinal(i, fp, root);
    }

    if ((gNdsFighterControllerLoopP0Completed == 1u) &&
        (gNdsFighterControllerLoopP1Completed == 1u))
    {
        gNdsFighterControllerLoopRootYDriftCount = 0u;
        gNdsFighterControllerLoopGADriftCount = 0u;
        if ((gNdsFighterControllerLoopP0StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterControllerLoopP1StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterControllerLoopP0MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterControllerLoopP1MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterControllerLoopP0GAFinal !=
                (u32)nMPKineticsGround) ||
            (gNdsFighterControllerLoopP1GAFinal !=
                (u32)nMPKineticsGround))
        {
            gNdsFighterControllerLoopGADriftCount++;
        }
        if ((gNdsFighterControllerLoopP0FloorOK != 1u) ||
            (gNdsFighterControllerLoopP1FloorOK != 1u))
        {
            gNdsFighterControllerLoopRootYDriftCount++;
        }
    }

    if ((gNdsFighterMarioFoxSchedulerLoopResult ==
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS) &&
        (gNdsFighterMarioFoxSchedulerLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsControllerPlaybackEnabled == 1u) &&
        ((gNdsControllerPlaybackConnectedMask & 0x3u) == 0x3u) &&
        (gNdsControllerPlaybackFrameCount > 0u) &&
        (gNdsControllerPlaybackReadCount > 0u) &&
        (gNdsControllerLiveReadCount == 0u) &&
        (gNdsFighterControllerLoopSYReadCount ==
            gNdsControllerPlaybackReadCount) &&
        (gNdsFighterControllerLoopSYUpdateCount ==
            gNdsControllerPlaybackReadCount))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterControllerLoopPrepared == 1u) &&
        (gNdsFighterControllerLoopTaskmanUpdateCount > 0u) &&
        (gNdsFighterControllerLoopVSBattleUpdateCount > 0u) &&
        (gNdsFighterControllerLoopSchedulerUpdateCount > 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterControllerLoopP0ProcessAttachCount == 1u) &&
        (gNdsFighterControllerLoopP1ProcessAttachCount == 1u) &&
        (gNdsFighterControllerLoopP0GObjProcessRunCount > 0u) &&
        (gNdsFighterControllerLoopP1GObjProcessRunCount > 0u) &&
        (gNdsFighterControllerLoopP0ProcCallbackCount ==
            gNdsFighterControllerLoopP0GObjProcessRunCount) &&
        (gNdsFighterControllerLoopP1ProcCallbackCount ==
            gNdsFighterControllerLoopP1GObjProcessRunCount))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterControllerLoopP0PlaybackApplyCount > 0u) &&
        (gNdsFighterControllerLoopP1PlaybackApplyCount > 0u) &&
        (gNdsFighterControllerLoopP0ControllerToFTInputCount > 0u) &&
        (gNdsFighterControllerLoopP1ControllerToFTInputCount > 0u) &&
        (gNdsFighterControllerLoopP0DirectFTInputWriteCount == 0u) &&
        (gNdsFighterControllerLoopP1DirectFTInputWriteCount == 0u) &&
        (gNdsFighterControllerLoopP0ButtonTapMask != 0u) &&
        (gNdsFighterControllerLoopP1ButtonTapMask != 0u) &&
        (gNdsFighterControllerLoopP0ButtonHoldMask != 0u) &&
        (gNdsFighterControllerLoopP1ButtonHoldMask != 0u) &&
        (gNdsFighterControllerLoopP0DashTapEligibleCount > 0u) &&
        (gNdsFighterControllerLoopP1DashTapEligibleCount > 0u) &&
        (gNdsFighterControllerLoopP0JumpButtonTapCount > 0u) &&
        (gNdsFighterControllerLoopP1JumpButtonTapCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterControllerLoopP0Completed == 1u) &&
        (gNdsFighterControllerLoopP1Completed == 1u) &&
        (gNdsFighterControllerLoopP0FrameCount > 0u) &&
        (gNdsFighterControllerLoopP1FrameCount > 0u) &&
        (gNdsFighterControllerLoopP0FrameCount <=
            gNdsFighterControllerLoopFrameMax) &&
        (gNdsFighterControllerLoopP1FrameCount <=
            gNdsFighterControllerLoopFrameMax))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterControllerLoopP0StatusVisitMask &
            NDS_FIGHTER_CONTROLLER_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_CONTROLLER_LOOP_STATUS_MASK_REQUIRED) &&
        ((gNdsFighterControllerLoopP1StatusVisitMask &
            NDS_FIGHTER_CONTROLLER_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_CONTROLLER_LOOP_STATUS_MASK_REQUIRED))
    {
        mask |= 1u << 6;
    }
    if (((gNdsFighterControllerLoopP0TransitionMask &
            NDS_FIGHTER_CONTROLLER_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_CONTROLLER_LOOP_TRANSITION_MASK_REQUIRED) &&
        ((gNdsFighterControllerLoopP1TransitionMask &
            NDS_FIGHTER_CONTROLLER_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_CONTROLLER_LOOP_TRANSITION_MASK_REQUIRED))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterControllerLoopP0InterruptCount > 0u) &&
        (gNdsFighterControllerLoopP1InterruptCount > 0u) &&
        (gNdsFighterControllerLoopP0PhysicsCount > 0u) &&
        (gNdsFighterControllerLoopP1PhysicsCount > 0u) &&
        (gNdsFighterControllerLoopP0IntegrateCount > 0u) &&
        (gNdsFighterControllerLoopP1IntegrateCount > 0u) &&
        (gNdsFighterControllerLoopP0MapCount > 0u) &&
        (gNdsFighterControllerLoopP1MapCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterControllerLoopP0RootDeltaXMilli != 0) &&
        (gNdsFighterControllerLoopP1RootDeltaXMilli != 0) &&
        (gNdsFighterControllerLoopP0RootRiseMilli > 0) &&
        (gNdsFighterControllerLoopP1RootRiseMilli > 0) &&
        (gNdsFighterControllerLoopP0RootDirectionOK == 1u) &&
        (gNdsFighterControllerLoopP1RootDirectionOK == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterControllerLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterControllerLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterControllerLoopP0MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterControllerLoopP1MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterControllerLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterControllerLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterControllerLoopP0FloorOK == 1u) &&
        (gNdsFighterControllerLoopP1FloorOK == 1u))
    {
        mask |= 1u << 10;
    }

    gNdsFighterControllerLoopGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterControllerLoopGObjDelta =
        (gNdsFighterControllerLoopGObjCountAfter >=
         gNdsFighterControllerLoopGObjCountBefore) ?
        (gNdsFighterControllerLoopGObjCountAfter -
         gNdsFighterControllerLoopGObjCountBefore) :
        (gNdsFighterControllerLoopGObjCountBefore -
         gNdsFighterControllerLoopGObjCountAfter);

    if ((gNdsFighterControllerLoopGObjDelta == 0u) &&
        (gNdsFighterControllerLoopUnexpectedStatusCount == 0u) &&
        (gNdsFighterControllerLoopDeniedStatusCount == 0u) &&
        (gNdsFighterControllerLoopProcessAttachEscapeCount == 0u) &&
        (gNdsFighterControllerLoopDisplayProbeCount == 0u) &&
        (gNdsFighterControllerLoopGameplayUpdateCount == 0u) &&
        (gNdsFighterControllerLoopDrawCallCount == 0u) &&
        (gNdsFighterControllerLoopMatrixCallCount == 0u) &&
        (gNdsFighterControllerLoopRootYDriftCount == 0u) &&
        (gNdsFighterControllerLoopGADriftCount == 0u))
    {
        mask |= 1u << 11;
    }

    gNdsFighterMarioFoxControllerLoopMask = mask;
    gNdsFighterMarioFoxControllerLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxControllerLoopCount =
        gNdsFighterControllerLoopP0Completed +
        gNdsFighterControllerLoopP1Completed;

    if ((mask & 0xfffu) == 0xfffu)
    {
        ndsFighterSchedulerLoopRestoreProcessLoopSnapshot();
        gNdsFighterMarioFoxControllerLoopResult =
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS;
        gNdsFighterMarioFoxControllerLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_SAFE_PASS;
    }
}

static void ndsFighterPreviewLoopRecordStart(u32 slot, FTStruct *fp,
                                             DObj *root)
{
    NDSFighterPreviewLoopState *state;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    bzero(state, sizeof(*state));
    state->phase = nNDSFighterPreviewLoopPhaseWalkStart;
    state->root_y_start = root->translate.vec.f.y;
    state->root_y_max = root->translate.vec.f.y;
    state->screen_x_start = (slot == 0u) ? 28 : 68;
    state->screen_x_final = state->screen_x_start;
    state->screen_y_floor = 60;
    state->screen_y_min = state->screen_y_floor;

    fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0StatusStart = (u32)fp->status_id;
        gNdsFighterPreviewLoopP0MotionStart = (u32)fp->motion_id;
        gNdsFighterPreviewLoopP0RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterPreviewLoopP0FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
        gNdsFighterPreviewLoopP0ScreenXStart = state->screen_x_start;
        gNdsFighterPreviewLoopP0ScreenYFloor = state->screen_y_floor;
        gNdsFighterPreviewLoopP0ScreenYMin = state->screen_y_min;
    }
    else
    {
        gNdsFighterPreviewLoopP1StatusStart = (u32)fp->status_id;
        gNdsFighterPreviewLoopP1MotionStart = (u32)fp->motion_id;
        gNdsFighterPreviewLoopP1RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterPreviewLoopP1FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
        gNdsFighterPreviewLoopP1ScreenXStart = state->screen_x_start;
        gNdsFighterPreviewLoopP1ScreenYFloor = state->screen_y_floor;
        gNdsFighterPreviewLoopP1ScreenYMin = state->screen_y_min;
    }
    ndsFighterPreviewLoopRecordState(slot, fp, nFTStatusIDNone, fp->ga);
}

static void ndsFighterPreviewLoopApplyPlayback(u32 slot, FTStruct *fp)
{
    NDSFighterPreviewLoopState *state;
    s32 lr_sign;
    s8 stick_x = 0;
    s8 stick_y = 0;
    u16 button = 0;

    if ((slot >= 2u) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    lr_sign = (fp->lr >= 0) ? 1 : -1;

    switch (state->phase)
    {
    case nNDSFighterPreviewLoopPhaseWalkStart:
    case nNDSFighterPreviewLoopPhaseWalkHold:
        stick_x = (s8)(40 * lr_sign);
        break;
    case nNDSFighterPreviewLoopPhaseDashStart:
    case nNDSFighterPreviewLoopPhaseRunHold:
        stick_x = (s8)(80 * lr_sign);
        break;
    case nNDSFighterPreviewLoopPhaseJumpStart:
    case nNDSFighterPreviewLoopPhaseJumpAir:
        stick_x = (s8)(40 * lr_sign);
        button = U_CBUTTONS;
        break;
    default:
        break;
    }

    ndsControllerPlaybackSetPad(slot, button, stick_x, stick_y);
    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0PlaybackApplyCount++;
        gNdsFighterPreviewLoopP0ButtonHoldMask |= button;
    }
    else
    {
        gNdsFighterPreviewLoopP1PlaybackApplyCount++;
        gNdsFighterPreviewLoopP1ButtonHoldMask |= button;
    }
}

static void ndsFighterPreviewLoopApplyFromSYController(u32 slot,
                                                       FTStruct *fp)
{
    NDSFighterPreviewLoopState *state;
    SYController *controller;
    s32 stick_x_abs;
    s32 stick_y_abs;
    s32 previous_x_abs;
    s32 previous_y_abs;

    if ((slot >= 2u) || (slot >= MAXCONTROLLERS) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    controller = &gSYControllerDevices[slot];
    stick_x_abs = ABS(controller->stick_range.x);
    stick_y_abs = ABS(controller->stick_range.y);
    previous_x_abs = ABS(state->previous_stick_x);
    previous_y_abs = ABS(state->previous_stick_y);

    fp->input.pl.stick_range.x = controller->stick_range.x;
    fp->input.pl.stick_range.y = controller->stick_range.y;
    fp->input.pl.button_hold = controller->button_hold;
    fp->input.pl.button_tap = controller->button_tap;
    fp->input.pl.button_release = controller->button_release;

    if ((stick_x_abs >= FTCOMMON_DASH_STICK_RANGE_MIN) &&
        (previous_x_abs < FTCOMMON_DASH_STICK_RANGE_MIN))
    {
        fp->tap_stick_x = 0u;
        if (slot == 0u)
        {
            gNdsFighterPreviewLoopP0DashTapEligibleCount++;
        }
        else
        {
            gNdsFighterPreviewLoopP1DashTapEligibleCount++;
        }
    }
    else if (fp->tap_stick_x < FTINPUT_STICKBUFFER_TICS_MAX)
    {
        fp->tap_stick_x++;
    }
    else
    {
        fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    }

    if ((stick_y_abs >= FTCOMMON_KNEEBEND_STICK_RANGE_MIN) &&
        (previous_y_abs < FTCOMMON_KNEEBEND_STICK_RANGE_MIN))
    {
        fp->tap_stick_y = 0u;
    }
    else if (fp->tap_stick_y < FTINPUT_STICKBUFFER_TICS_MAX)
    {
        fp->tap_stick_y++;
    }
    else
    {
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    }
    fp->hold_stick_x = (stick_x_abs != 0) ? 0u : FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_y = (stick_y_abs != 0) ? 0u : FTINPUT_STICKBUFFER_TICS_MAX;

    state->previous_stick_x = controller->stick_range.x;
    state->previous_stick_y = controller->stick_range.y;

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0ControllerToFTInputCount++;
        gNdsFighterPreviewLoopP0ButtonTapMask |= controller->button_tap;
        gNdsFighterPreviewLoopP0ButtonHoldMask |= controller->button_hold;
        gNdsFighterPreviewLoopP0LastStickX = controller->stick_range.x;
        gNdsFighterPreviewLoopP0LastStickY = controller->stick_range.y;
        if ((controller->button_tap & U_CBUTTONS) != 0u)
        {
            gNdsFighterPreviewLoopP0JumpButtonTapCount++;
        }
    }
    else
    {
        gNdsFighterPreviewLoopP1ControllerToFTInputCount++;
        gNdsFighterPreviewLoopP1ButtonTapMask |= controller->button_tap;
        gNdsFighterPreviewLoopP1ButtonHoldMask |= controller->button_hold;
        gNdsFighterPreviewLoopP1LastStickX = controller->stick_range.x;
        gNdsFighterPreviewLoopP1LastStickY = controller->stick_range.y;
        if ((controller->button_tap & U_CBUTTONS) != 0u)
        {
            gNdsFighterPreviewLoopP1JumpButtonTapCount++;
        }
    }
}

static void ndsFighterPreviewLoopRecordState(u32 slot, FTStruct *fp,
                                             s32 previous_status,
                                             s32 previous_ga)
{
    NDSFighterPreviewLoopState *state;
    u32 transition_bit;

    if ((slot >= 2u) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    state->status_visit_mask |= ndsFighterProcessLoopStatusBit(fp->status_id);
    transition_bit = ndsFighterProcessLoopTransitionBit(previous_status,
                                                       fp->status_id);
    state->transition_mask |= transition_bit;

    if ((previous_ga == nMPKineticsGround) && (fp->ga == nMPKineticsAir))
    {
        gNdsFighterPreviewLoopSetAirCount++;
    }
    if ((previous_ga == nMPKineticsAir) && (fp->ga == nMPKineticsGround))
    {
        gNdsFighterPreviewLoopSetGroundCount++;
    }
    if ((transition_bit & (1u << 5)) != 0u)
    {
        gNdsFighterPreviewLoopRunBrakeEndCount++;
    }
    if ((transition_bit & (1u << 8)) != 0u)
    {
        gNdsFighterPreviewLoopJumpAnimEndCount++;
    }
    if ((transition_bit & (1u << 9)) != 0u)
    {
        gNdsFighterPreviewLoopFallDetectCount++;
        gNdsFighterPreviewLoopLandingDetectCount++;
    }
    if ((transition_bit & (1u << 10)) != 0u)
    {
        gNdsFighterPreviewLoopLandingEndCount++;
    }
    if ((fp->status_id == nFTCommonStatusWait) &&
        (previous_status != nFTCommonStatusWait))
    {
        gNdsFighterPreviewLoopWaitSetStatusCount++;
    }

    switch (fp->status_id)
    {
    case nFTCommonStatusWait: state->wait_visit_count++; break;
    case nFTCommonStatusWalkSlow:
    case nFTCommonStatusWalkMiddle:
    case nFTCommonStatusWalkFast: state->walk_visit_count++; break;
    case nFTCommonStatusDash: state->dash_visit_count++; break;
    case nFTCommonStatusRun: state->run_visit_count++; break;
    case nFTCommonStatusRunBrake: state->runbrake_visit_count++; break;
    case nFTCommonStatusKneeBend: state->kneebend_visit_count++; break;
    case nFTCommonStatusJumpF: state->jump_visit_count++; break;
    case nFTCommonStatusFall: state->fall_visit_count++; break;
    case nFTCommonStatusLandingLight: state->landing_visit_count++; break;
    default:
        gNdsFighterPreviewLoopUnexpectedStatusCount++;
        break;
    }

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0StatusVisitMask = state->status_visit_mask;
        gNdsFighterPreviewLoopP0TransitionMask = state->transition_mask;
    }
    else
    {
        gNdsFighterPreviewLoopP1StatusVisitMask = state->status_visit_mask;
        gNdsFighterPreviewLoopP1TransitionMask = state->transition_mask;
    }
}

static void ndsFighterPreviewLoopAdvancePhase(u32 slot, FTStruct *fp)
{
    NDSFighterPreviewLoopState *state;
    GObj *fighter_gobj;
    DObj *root;
    FTAttributes *attr;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    fighter_gobj = fp->fighter_gobj;
    root = fp->joints[nFTPartsJointTopN];
    attr = fp->attr;

    switch (state->phase)
    {
    case nNDSFighterPreviewLoopPhaseWalkStart:
        if ((fp->status_id >= nFTCommonStatusWalkSlow) &&
            (fp->status_id <= nFTCommonStatusWalkFast))
        {
            state->phase = nNDSFighterPreviewLoopPhaseWalkHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseWalkHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterPreviewLoopPhaseWalkRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseWalkRelease:
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterPreviewLoopPhaseDashStart;
            state->phase_frame = 0u;
            state->previous_stick_x = 0;
        }
        break;
    case nNDSFighterPreviewLoopPhaseDashStart:
        if (fp->status_id == nFTCommonStatusRun)
        {
            state->phase = nNDSFighterPreviewLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusDash) && (attr != NULL))
        {
            fighter_gobj->anim_frame = attr->dash_to_run;
            sNdsFighterProcessLoopInterruptActive = TRUE;
            ftCommonRunSetStatus(fighter_gobj);
            sNdsFighterProcessLoopInterruptActive = FALSE;
            ndsFighterPreviewLoopRecordState(slot, fp,
                                             nFTCommonStatusDash,
                                             nMPKineticsGround);
            state->phase = nNDSFighterPreviewLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseRunHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterPreviewLoopPhaseRunRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseRunRelease:
        if (fp->status_id == nFTCommonStatusRunBrake)
        {
            state->phase = nNDSFighterPreviewLoopPhaseRunBrakeEnd;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseRunBrakeEnd:
        if (++state->phase_frame >= 2u)
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopRunBrakeEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopRunBrakeEndActive = FALSE;
            ndsFighterPreviewLoopRecordState(slot, fp,
                                             nFTCommonStatusRunBrake,
                                             nMPKineticsGround);
        }
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterPreviewLoopPhaseJumpStart;
            state->phase_frame = 0u;
            state->previous_stick_x = 0;
            state->previous_stick_y = 0;
        }
        break;
    case nNDSFighterPreviewLoopPhaseJumpStart:
        if (fp->status_id == nFTCommonStatusJumpF)
        {
            state->phase = nNDSFighterPreviewLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusKneeBend) &&
                 (++state->phase_frame >= 3u))
        {
            sNdsFighterProcessLoopUpdateActive = TRUE;
            ftCommonJumpSetStatus(fighter_gobj);
            sNdsFighterProcessLoopUpdateActive = FALSE;
            if (fp->physics.vel_air.y <= 0.0F)
            {
                fp->physics.vel_air.y = 4.0F;
                fp->vel_air = fp->physics.vel_air;
            }
            if (root != NULL)
            {
                root->translate.vec.f.y += fp->physics.vel_air.y;
                if (root->translate.vec.f.y > state->root_y_max)
                {
                    state->root_y_max = root->translate.vec.f.y;
                }
            }
            ndsFighterPreviewLoopRecordState(slot, fp,
                                             nFTCommonStatusKneeBend,
                                             nMPKineticsGround);
            state->phase = nNDSFighterPreviewLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseJumpAir:
        if (++state->phase_frame >= 6u)
        {
            sNdsFighterProcessLoopJumpAnimEndActive = TRUE;
            ftAnimEndSetFall(fighter_gobj);
            sNdsFighterProcessLoopJumpAnimEndActive = FALSE;
            ndsFighterPreviewLoopRecordState(slot, fp,
                                             nFTCommonStatusJumpF,
                                             nMPKineticsAir);
            fp->physics.vel_air.y = -6.0F;
            fp->vel_air = fp->physics.vel_air;
            state->phase = nNDSFighterPreviewLoopPhaseFallLand;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseFallLand:
        if ((fp->status_id == nFTCommonStatusLandingLight) &&
            (++state->phase_frame >= 5u))
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopLandingEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopLandingEndActive = FALSE;
            ndsFighterPreviewLoopRecordState(slot, fp,
                                             nFTCommonStatusLandingLight,
                                             nMPKineticsGround);
            state->phase = nNDSFighterPreviewLoopPhaseDone;
            state->completed = 1u;
        }
        break;
    default:
        break;
    }
}

static void ndsFighterPreviewLoopRunSlotProcess(u32 slot, FTStruct *fp)
{
    NDSFighterPreviewLoopState *state;
    s32 previous_status;
    s32 previous_ga;
    DObj *root;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        gNdsFighterPreviewLoopProcessAttachEscapeCount++;
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    if ((state->completed != 0u) ||
        (state->total_frames >= NDS_FIGHTER_PREVIEW_LOOP_FRAME_MAX))
    {
        return;
    }
    previous_status = fp->status_id;
    previous_ga = fp->ga;
    root = fp->joints[nFTPartsJointTopN];

    ndsFighterPreviewLoopApplyFromSYController(slot, fp);
    sNdsFighterPreviewLoopActive = TRUE;
    sNdsFighterProcessLoopActive = TRUE;
    ndsFighterProcessLoopRunFrame(slot, fp);
    sNdsFighterProcessLoopActive = FALSE;
    sNdsFighterPreviewLoopActive = FALSE;
    ndsFighterPreviewLoopRecordState(slot, fp, previous_status, previous_ga);
    if ((root != NULL) && (root->translate.vec.f.y > state->root_y_max))
    {
        state->root_y_max = root->translate.vec.f.y;
    }
    ndsFighterPreviewLoopAdvancePhase(slot, fp);
    state->total_frames++;
    gNdsFighterPreviewLoopDeferredInterruptCheckCount++;

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0FrameCount = state->total_frames;
        gNdsFighterPreviewLoopP0InterruptCount++;
        gNdsFighterPreviewLoopP0PhysicsCount++;
        gNdsFighterPreviewLoopP0IntegrateCount++;
        gNdsFighterPreviewLoopP0MapCount++;
    }
    else
    {
        gNdsFighterPreviewLoopP1FrameCount = state->total_frames;
        gNdsFighterPreviewLoopP1InterruptCount++;
        gNdsFighterPreviewLoopP1PhysicsCount++;
        gNdsFighterPreviewLoopP1IntegrateCount++;
        gNdsFighterPreviewLoopP1MapCount++;
    }
}

static void ndsFighterPreviewLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 slot = 2u;

    if ((fp != NULL) && (fp->player < 2))
    {
        slot = fp->player;
    }
    if ((slot >= 2u) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        gNdsFighterPreviewLoopProcessAttachEscapeCount++;
        return;
    }
    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0ProcCallbackCount++;
    }
    else
    {
        gNdsFighterPreviewLoopP1ProcCallbackCount++;
    }
    ndsFighterPreviewLoopRunSlotProcess(slot, fp);
}

static s32 ndsFighterPreviewLoopClampS32(s32 value, s32 min, s32 max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}

static void ndsFighterPreviewLoopPlot(u16 *pixels, u32 pitch, s32 x, s32 y,
                                      u16 color, u32 *count,
                                      u32 *checksum)
{
    if ((pixels == NULL) || (x < 0) || (y < 0) ||
        (x >= (s32)NDS_FIGHTER_PREVIEW_LOOP_WIDTH) ||
        (y >= (s32)NDS_FIGHTER_PREVIEW_LOOP_HEIGHT))
    {
        return;
    }
    pixels[(u32)y * pitch + (u32)x] = color;
    if (count != NULL)
    {
        (*count)++;
    }
    if (checksum != NULL)
    {
        *checksum = (*checksum * 33u) ^ (u32)color ^
            ((u32)x << 16) ^ (u32)y;
    }
}

static void ndsFighterPreviewLoopClear(u16 *pixels, u32 pitch)
{
    u32 x;
    u32 y;
    u16 bg = ndsFighterDLDrawRGB15(6, 8, 14);
    u16 floor = ndsFighterDLDrawRGB15(70, 110, 70);

    if (pixels == NULL)
    {
        return;
    }
    for (y = 0u; y < NDS_FIGHTER_PREVIEW_LOOP_HEIGHT; y++)
    {
        for (x = 0u; x < NDS_FIGHTER_PREVIEW_LOOP_WIDTH; x++)
        {
            pixels[y * pitch + x] = bg;
        }
    }
    for (x = 0u; x < NDS_FIGHTER_PREVIEW_LOOP_WIDTH; x++)
    {
        pixels[(NDS_FIGHTER_PREVIEW_LOOP_HEIGHT - 10u) * pitch + x] = floor;
    }
}

static void ndsFighterPreviewLoopDrawSlot(u32 slot, FTStruct *fp,
                                          u16 *pixels, u32 pitch)
{
    NDSFighterPreviewLoopState *state;
    NDSFighterDLAllDrawCollection collection;
    DObj *root;
    s32 root_delta;
    s32 root_rise;
    s32 root_rise_max;
    s32 screen_x;
    s32 screen_y;
    u32 pixel_count = 0u;
    u32 checksum = 0u;
    u32 i;

    if ((slot >= 2u) || (fp == NULL) || (pixels == NULL))
    {
        return;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    ndsFighterCollectAllDObjsWithDL(root, &collection);

    root_delta = ndsFloatToMilliSigned(root->translate.vec.f.x) -
        ((slot == 0u) ? gNdsFighterPreviewLoopP0RootXStartMilli :
            gNdsFighterPreviewLoopP1RootXStartMilli);
    root_rise = ndsFloatToMilliSigned(root->translate.vec.f.y) -
        ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    root_rise_max = ndsFloatToMilliSigned(state->root_y_max) -
        ndsFloatToMilliSigned(state->root_y_start);
    if (root_rise < root_rise_max)
    {
        root_rise = root_rise_max;
    }
    screen_x = state->screen_x_start + (root_delta / 1500);
    screen_y = state->screen_y_floor - (root_rise / 1200);
    screen_x = ndsFighterPreviewLoopClampS32(screen_x, 6, 89);
    screen_y = ndsFighterPreviewLoopClampS32(screen_y, 8, 62);

    if ((state->screen_initialized == 0u) ||
        (screen_y < state->screen_y_min))
    {
        state->screen_y_min = screen_y;
    }
    state->screen_initialized = 1u;
    state->screen_x_final = screen_x;

    for (i = 0u; i < collection.selected_count; i++)
    {
        s32 ox = (s32)(i % 5u) - 2;
        s32 oy = -3 - (s32)((i / 5u) * 3u);
        u16 color = (slot == 0u) ?
            ndsFighterDLDrawRGB15(245, 70 + ((i * 7u) & 31u), 45) :
            ndsFighterDLDrawRGB15(60, 105 + ((i * 5u) & 31u), 245);

        ndsFighterPreviewLoopPlot(pixels, pitch, screen_x + ox,
                                  screen_y + oy, color, &pixel_count,
                                  &checksum);
        ndsFighterPreviewLoopPlot(pixels, pitch, screen_x + ox - 1,
                                  screen_y + oy + 1, color, &pixel_count,
                                  &checksum);
        ndsFighterPreviewLoopPlot(pixels, pitch, screen_x + ox + 1,
                                  screen_y + oy + 1, color, &pixel_count,
                                  &checksum);
    }

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0CandidateCount = collection.total_count;
        gNdsFighterPreviewLoopP0DrawnDObjCount = collection.selected_count;
        gNdsFighterPreviewLoopP0PixelCount += pixel_count;
        gNdsFighterPreviewLoopP0ColorChecksum =
            (gNdsFighterPreviewLoopP0ColorChecksum * 33u) ^ checksum;
        gNdsFighterPreviewLoopP0ScreenXFinal = screen_x;
        gNdsFighterPreviewLoopP0ScreenXDelta =
            screen_x - gNdsFighterPreviewLoopP0ScreenXStart;
        gNdsFighterPreviewLoopP0ScreenYMin = state->screen_y_min;
        gNdsFighterPreviewLoopP0ScreenRise =
            gNdsFighterPreviewLoopP0ScreenYFloor - state->screen_y_min;
    }
    else
    {
        gNdsFighterPreviewLoopP1CandidateCount = collection.total_count;
        gNdsFighterPreviewLoopP1DrawnDObjCount = collection.selected_count;
        gNdsFighterPreviewLoopP1PixelCount += pixel_count;
        gNdsFighterPreviewLoopP1ColorChecksum =
            (gNdsFighterPreviewLoopP1ColorChecksum * 33u) ^ checksum;
        gNdsFighterPreviewLoopP1ScreenXFinal = screen_x;
        gNdsFighterPreviewLoopP1ScreenXDelta =
            screen_x - gNdsFighterPreviewLoopP1ScreenXStart;
        gNdsFighterPreviewLoopP1ScreenYMin = state->screen_y_min;
        gNdsFighterPreviewLoopP1ScreenRise =
            gNdsFighterPreviewLoopP1ScreenYFloor - state->screen_y_min;
    }
}

static void ndsFighterPreviewLoopRecordDisplayFromCallback(GObj *fighter_gobj)
{
    FTStruct *fp;
    u32 slot;

    if ((fighter_gobj == NULL) || (sNdsFighterPreviewLoopPixels == NULL))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if (ndsFighterStructIsPoolPointer(fp) == FALSE)
    {
        return;
    }
    slot = (u32)fp->nds_slot;
    if (slot > 1u)
    {
        return;
    }

    gNdsFighterPreviewLoopDisplayCallbackCount++;
    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0DisplayCallbackCount++;
    }
    else
    {
        gNdsFighterPreviewLoopP1DisplayCallbackCount++;
    }
    ndsFighterPreviewLoopDrawSlot(slot, fp, sNdsFighterPreviewLoopPixels,
                                  sNdsFighterPreviewLoopPitch);
}

static void ndsFighterPreviewLoopDrawKeyframe(void)
{
    u32 pitch = 0u;
    u16 *pixels;

    pixels = ndsPlatformBeginOriginalDLPreview(
        NDS_FIGHTER_PREVIEW_LOOP_WIDTH,
        NDS_FIGHTER_PREVIEW_LOOP_HEIGHT,
        &pitch);
    if (pixels == NULL)
    {
        return;
    }
    if (gNdsFighterPreviewLoopDrawFrameCount == 0u)
    {
        gNdsFighterPreviewLoopPreviewCommitBefore =
            gNdsOriginalDLPreviewCommitCount;
    }
    gNdsFighterPreviewLoopPreviewWidth = NDS_FIGHTER_PREVIEW_LOOP_WIDTH;
    gNdsFighterPreviewLoopPreviewHeight = NDS_FIGHTER_PREVIEW_LOOP_HEIGHT;
    gNdsFighterPreviewLoopPreviewPitch = pitch;
    sNdsFighterPreviewLoopPixels = pixels;
    sNdsFighterPreviewLoopPitch = pitch;
    sNdsFighterPreviewLoopDisplayActive = TRUE;
    ndsFighterPreviewLoopClear(pixels, pitch);
    ftDisplayMainProcDisplay(sNdsFighterStructPool[0].fighter_gobj);
    ftDisplayMainProcDisplay(sNdsFighterStructPool[1].fighter_gobj);
    sNdsFighterPreviewLoopDisplayActive = FALSE;
    sNdsFighterPreviewLoopPixels = NULL;
    sNdsFighterPreviewLoopPitch = 0u;

    gNdsFighterPreviewLoopTotalPixelCount =
        gNdsFighterPreviewLoopP0PixelCount +
        gNdsFighterPreviewLoopP1PixelCount;
    if (gNdsFighterPreviewLoopTotalPixelCount > 0u)
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterPreviewLoopPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterPreviewLoopPreviewCommitDelta =
            gNdsFighterPreviewLoopPreviewCommitAfter -
            gNdsFighterPreviewLoopPreviewCommitBefore;
        gNdsFighterPreviewLoopPreviewReady = gNdsOriginalDLPreviewReady;
        gNdsFighterPreviewLoopDrawFrameCount++;
    }
    sNdsFighterPreviewLoopDrawFrameIndex++;
}

static void ndsFighterPreviewLoopRecordFinal(u32 slot, FTStruct *fp,
                                             DObj *root)
{
    NDSFighterPreviewLoopState *state;
    s32 root_y_final;
    s32 floor_y;
    s32 root_delta;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    if ((state->completed != 0u) && (fp->status_id == nFTCommonStatusWait))
    {
        state->status_visit_mask |= 1u << 9;
    }
    root_y_final = ndsFloatToMilliSigned(root->translate.vec.f.y);
    floor_y = ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    root_delta = ndsFloatToMilliSigned(root->translate.vec.f.x) -
        ((slot == 0u) ? gNdsFighterPreviewLoopP0RootXStartMilli :
            gNdsFighterPreviewLoopP1RootXStartMilli);

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0Completed = state->completed;
        gNdsFighterPreviewLoopP0StatusVisitMask = state->status_visit_mask;
        gNdsFighterPreviewLoopP0TransitionMask = state->transition_mask;
        gNdsFighterPreviewLoopP0StatusFinal = (u32)fp->status_id;
        gNdsFighterPreviewLoopP0MotionFinal = (u32)fp->motion_id;
        gNdsFighterPreviewLoopP0GAFinal = (u32)fp->ga;
        gNdsFighterPreviewLoopP0RootYFinalMilli = root_y_final;
        gNdsFighterPreviewLoopP0RootDeltaXMilli = root_delta;
        gNdsFighterPreviewLoopP0RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterPreviewLoopP0RootDirectionOK =
            ((gNdsFighterPreviewLoopP0RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterPreviewLoopP0FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }
    else
    {
        gNdsFighterPreviewLoopP1Completed = state->completed;
        gNdsFighterPreviewLoopP1StatusVisitMask = state->status_visit_mask;
        gNdsFighterPreviewLoopP1TransitionMask = state->transition_mask;
        gNdsFighterPreviewLoopP1StatusFinal = (u32)fp->status_id;
        gNdsFighterPreviewLoopP1MotionFinal = (u32)fp->motion_id;
        gNdsFighterPreviewLoopP1GAFinal = (u32)fp->ga;
        gNdsFighterPreviewLoopP1RootYFinalMilli = root_y_final;
        gNdsFighterPreviewLoopP1RootDeltaXMilli = root_delta;
        gNdsFighterPreviewLoopP1RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterPreviewLoopP1RootDirectionOK =
            ((gNdsFighterPreviewLoopP1RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterPreviewLoopP1FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }
}

void ndsFighterMarioFoxPreviewLoopPrepare(void)
{
    u32 i;

    if ((ndsFighterMarioFoxPreviewLoopProofEnabled() == FALSE) ||
        (gNdsFighterPreviewLoopPrepared != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxControllerLoopResult !=
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS) ||
        (gNdsFighterMarioFoxControllerLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_SAFE_PASS) ||
        ((gNdsFighterMarioFoxControllerLoopMask & 0xfffu) != 0xfffu) ||
        (gNdsFighterMarioFoxControllerLoopDeferredMask != 0xffu) ||
        (gNdsFighterMarioFoxControllerLoopCount != 2u))
    {
        return;
    }

    bzero(sNdsFighterPreviewLoopStates,
          sizeof(sNdsFighterPreviewLoopStates));
    bzero(sNdsFighterPreviewLoopProcesses,
          sizeof(sNdsFighterPreviewLoopProcesses));
    sNdsFighterPreviewLoopPixels = NULL;
    sNdsFighterPreviewLoopPitch = 0u;
    sNdsFighterPreviewLoopDrawFrameIndex = 0u;
    ndsControllerPlaybackReset();
    ndsControllerPlaybackSetConnectedMask(0x3u);
    ndsControllerPlaybackSetEnabled(TRUE);
    gNdsFighterPreviewLoopFrameMax = NDS_FIGHTER_PREVIEW_LOOP_FRAME_MAX;
    gNdsFighterPreviewLoopUpdateMax = NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX;
    gNdsFighterPreviewLoopGObjCountBefore = (u32)gcGetGObjsActiveNum();

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj = fp->fighter_gobj;
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL) || (root == NULL))
        {
            gNdsFighterPreviewLoopProcessAttachEscapeCount++;
            continue;
        }
        ndsFighterPreviewLoopRecordStart(i, fp, root);
        sNdsFighterPreviewLoopProcesses[i] =
            gcAddGObjProcess(fighter_gobj,
                             ndsFighterPreviewLoopGObjProc,
                             nGCProcessKindFunc,
                             3);
        if (sNdsFighterPreviewLoopProcesses[i] == NULL)
        {
            gNdsFighterPreviewLoopProcessAttachEscapeCount++;
        }
        else if (i == 0u)
        {
            gNdsFighterPreviewLoopP0ProcessAttachCount++;
        }
        else
        {
            gNdsFighterPreviewLoopP1ProcessAttachCount++;
        }
    }

    if ((sNdsFighterPreviewLoopProcesses[0] != NULL) &&
        (sNdsFighterPreviewLoopProcesses[1] != NULL))
    {
        gNdsFighterPreviewLoopPrepared = 1u;
    }
}

s32 ndsFighterMarioFoxPreviewLoopUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxPreviewLoopProofEnabled() != FALSE) &&
            (gNdsFighterPreviewLoopPrepared != 0u) &&
            (gNdsFighterMarioFoxPreviewLoopResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxPreviewLoopRunVSBattleUpdate(void)
{
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxPreviewLoopUpdateEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterPreviewLoopVSBattleUpdateCount++;
    gNdsFighterPreviewLoopSchedulerUpdateCount++;

    for (i = 0; i < 2u; i++)
    {
        ndsFighterPreviewLoopApplyPlayback(i, &sNdsFighterStructPool[i]);
    }
    ndsControllerPlaybackCommitFrame();
    syControllerReadDeviceData();
    gNdsFighterPreviewLoopSYReadCount++;
    syControllerUpdateGlobalData();
    gNdsFighterPreviewLoopSYUpdateCount++;

    for (i = 0; i < 2u; i++)
    {
        if (sNdsFighterPreviewLoopProcesses[i] == NULL)
        {
            continue;
        }
        if (i == 0u)
        {
            gNdsFighterPreviewLoopP0GObjProcessRunCount++;
        }
        else
        {
            gNdsFighterPreviewLoopP1GObjProcessRunCount++;
        }
        gcRunGObjProcess(sNdsFighterPreviewLoopProcesses[i]);
    }

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root = fp->joints[nFTPartsJointTopN];
        ndsFighterPreviewLoopRecordFinal(i, fp, root);
    }

    if (((gNdsFighterPreviewLoopVSBattleUpdateCount %
            NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_INTERVAL) == 0u) ||
        ((gNdsFighterPreviewLoopP0Completed == 1u) &&
         (gNdsFighterPreviewLoopP1Completed == 1u)))
    {
        ndsFighterPreviewLoopDrawKeyframe();
    }

    if ((gNdsFighterPreviewLoopP0Completed == 1u) &&
        (gNdsFighterPreviewLoopP1Completed == 1u))
    {
        gNdsFighterPreviewLoopRootYDriftCount = 0u;
        gNdsFighterPreviewLoopGADriftCount = 0u;
        if ((gNdsFighterPreviewLoopP0StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterPreviewLoopP1StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterPreviewLoopP0MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterPreviewLoopP1MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterPreviewLoopP0GAFinal !=
                (u32)nMPKineticsGround) ||
            (gNdsFighterPreviewLoopP1GAFinal !=
                (u32)nMPKineticsGround))
        {
            gNdsFighterPreviewLoopGADriftCount++;
        }
        if ((gNdsFighterPreviewLoopP0FloorOK != 1u) ||
            (gNdsFighterPreviewLoopP1FloorOK != 1u))
        {
            gNdsFighterPreviewLoopRootYDriftCount++;
        }
    }

    if ((gNdsFighterMarioFoxControllerLoopResult ==
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS) &&
        (gNdsFighterMarioFoxControllerLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsControllerPlaybackEnabled == 1u) &&
        ((gNdsControllerPlaybackConnectedMask & 0x3u) == 0x3u) &&
        (gNdsControllerPlaybackFrameCount > 0u) &&
        (gNdsControllerPlaybackReadCount > 0u) &&
        (gNdsControllerLiveReadCount == 0u) &&
        (gNdsFighterPreviewLoopSYReadCount ==
            gNdsControllerPlaybackReadCount) &&
        (gNdsFighterPreviewLoopSYUpdateCount ==
            gNdsControllerPlaybackReadCount))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterPreviewLoopPrepared == 1u) &&
        (gNdsFighterPreviewLoopTaskmanUpdateCount > 0u) &&
        (gNdsFighterPreviewLoopVSBattleUpdateCount > 0u) &&
        (gNdsFighterPreviewLoopSchedulerUpdateCount > 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterPreviewLoopP0ProcessAttachCount == 1u) &&
        (gNdsFighterPreviewLoopP1ProcessAttachCount == 1u) &&
        (gNdsFighterPreviewLoopP0GObjProcessRunCount > 0u) &&
        (gNdsFighterPreviewLoopP1GObjProcessRunCount > 0u) &&
        (gNdsFighterPreviewLoopP0ProcCallbackCount ==
            gNdsFighterPreviewLoopP0GObjProcessRunCount) &&
        (gNdsFighterPreviewLoopP1ProcCallbackCount ==
            gNdsFighterPreviewLoopP1GObjProcessRunCount))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterPreviewLoopP0PlaybackApplyCount > 0u) &&
        (gNdsFighterPreviewLoopP1PlaybackApplyCount > 0u) &&
        (gNdsFighterPreviewLoopP0ControllerToFTInputCount > 0u) &&
        (gNdsFighterPreviewLoopP1ControllerToFTInputCount > 0u) &&
        (gNdsFighterPreviewLoopP0DirectFTInputWriteCount == 0u) &&
        (gNdsFighterPreviewLoopP1DirectFTInputWriteCount == 0u) &&
        (gNdsFighterPreviewLoopP0ButtonTapMask != 0u) &&
        (gNdsFighterPreviewLoopP1ButtonTapMask != 0u) &&
        (gNdsFighterPreviewLoopP0ButtonHoldMask != 0u) &&
        (gNdsFighterPreviewLoopP1ButtonHoldMask != 0u) &&
        (gNdsFighterPreviewLoopP0DashTapEligibleCount > 0u) &&
        (gNdsFighterPreviewLoopP1DashTapEligibleCount > 0u) &&
        (gNdsFighterPreviewLoopP0JumpButtonTapCount > 0u) &&
        (gNdsFighterPreviewLoopP1JumpButtonTapCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterPreviewLoopP0Completed == 1u) &&
        (gNdsFighterPreviewLoopP1Completed == 1u) &&
        (gNdsFighterPreviewLoopP0FrameCount > 0u) &&
        (gNdsFighterPreviewLoopP1FrameCount > 0u) &&
        (gNdsFighterPreviewLoopP0FrameCount <=
            gNdsFighterPreviewLoopFrameMax) &&
        (gNdsFighterPreviewLoopP1FrameCount <=
            gNdsFighterPreviewLoopFrameMax))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterPreviewLoopP0StatusVisitMask &
            NDS_FIGHTER_PREVIEW_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_PREVIEW_LOOP_STATUS_MASK_REQUIRED) &&
        ((gNdsFighterPreviewLoopP1StatusVisitMask &
            NDS_FIGHTER_PREVIEW_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_PREVIEW_LOOP_STATUS_MASK_REQUIRED))
    {
        mask |= 1u << 6;
    }
    if (((gNdsFighterPreviewLoopP0TransitionMask &
            NDS_FIGHTER_PREVIEW_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_PREVIEW_LOOP_TRANSITION_MASK_REQUIRED) &&
        ((gNdsFighterPreviewLoopP1TransitionMask &
            NDS_FIGHTER_PREVIEW_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_PREVIEW_LOOP_TRANSITION_MASK_REQUIRED))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterPreviewLoopP0InterruptCount > 0u) &&
        (gNdsFighterPreviewLoopP1InterruptCount > 0u) &&
        (gNdsFighterPreviewLoopP0PhysicsCount > 0u) &&
        (gNdsFighterPreviewLoopP1PhysicsCount > 0u) &&
        (gNdsFighterPreviewLoopP0IntegrateCount > 0u) &&
        (gNdsFighterPreviewLoopP1IntegrateCount > 0u) &&
        (gNdsFighterPreviewLoopP0MapCount > 0u) &&
        (gNdsFighterPreviewLoopP1MapCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterPreviewLoopP0RootDeltaXMilli != 0) &&
        (gNdsFighterPreviewLoopP1RootDeltaXMilli != 0) &&
        (gNdsFighterPreviewLoopP0RootRiseMilli > 0) &&
        (gNdsFighterPreviewLoopP1RootRiseMilli > 0) &&
        (gNdsFighterPreviewLoopP0RootDirectionOK == 1u) &&
        (gNdsFighterPreviewLoopP1RootDirectionOK == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterPreviewLoopPreviewReady != 0u) &&
        (gNdsFighterPreviewLoopPreviewCommitDelta >=
            NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterPreviewLoopDrawFrameCount >=
            NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterPreviewLoopDisplayCallbackCount >=
            (NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN * 2u)) &&
        (gNdsFighterPreviewLoopP0DisplayCallbackCount >=
            NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterPreviewLoopP1DisplayCallbackCount >=
            NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterPreviewLoopP0CandidateCount >= 14u) &&
        (gNdsFighterPreviewLoopP1CandidateCount >= 18u) &&
        (gNdsFighterPreviewLoopP0DrawnDObjCount >= 14u) &&
        (gNdsFighterPreviewLoopP1DrawnDObjCount >= 18u) &&
        (gNdsFighterPreviewLoopP0PixelCount > 0u) &&
        (gNdsFighterPreviewLoopP1PixelCount > 0u) &&
        (gNdsFighterPreviewLoopP0ColorChecksum != 0u) &&
        (gNdsFighterPreviewLoopP1ColorChecksum != 0u) &&
        (gNdsFighterPreviewLoopP0ScreenXDelta != 0) &&
        (gNdsFighterPreviewLoopP1ScreenXDelta != 0) &&
        (gNdsFighterPreviewLoopP0ScreenRise > 0) &&
        (gNdsFighterPreviewLoopP1ScreenRise > 0))
    {
        mask |= 1u << 10;
    }
    if ((gNdsFighterPreviewLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterPreviewLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterPreviewLoopP0MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterPreviewLoopP1MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterPreviewLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterPreviewLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterPreviewLoopP0FloorOK == 1u) &&
        (gNdsFighterPreviewLoopP1FloorOK == 1u))
    {
        mask |= 1u << 11;
    }

    gNdsFighterPreviewLoopGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterPreviewLoopGObjDelta =
        (gNdsFighterPreviewLoopGObjCountAfter >=
         gNdsFighterPreviewLoopGObjCountBefore) ?
        (gNdsFighterPreviewLoopGObjCountAfter -
         gNdsFighterPreviewLoopGObjCountBefore) :
        (gNdsFighterPreviewLoopGObjCountBefore -
         gNdsFighterPreviewLoopGObjCountAfter);

    if ((gNdsFighterPreviewLoopGObjDelta == 0u) &&
        (gNdsFighterPreviewLoopUnexpectedStatusCount == 0u) &&
        (gNdsFighterPreviewLoopDeniedStatusCount == 0u) &&
        (gNdsFighterPreviewLoopProcessAttachEscapeCount == 0u) &&
        (gNdsFighterPreviewLoopDisplayProbeCount == 0u) &&
        (gNdsFighterPreviewLoopGameplayUpdateCount == 0u) &&
        (gNdsFighterPreviewLoopDrawCallCount == 0u) &&
        (gNdsFighterPreviewLoopMatrixCallCount == 0u) &&
        (gNdsFighterPreviewLoopRootYDriftCount == 0u) &&
        (gNdsFighterPreviewLoopGADriftCount == 0u))
    {
        mask |= 1u << 12;
    }

    gNdsFighterMarioFoxPreviewLoopMask = mask;
    gNdsFighterMarioFoxPreviewLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxPreviewLoopCount =
        gNdsFighterPreviewLoopP0Completed +
        gNdsFighterPreviewLoopP1Completed;

    if ((mask & 0x1fffu) == 0x1fffu)
    {
        gNdsFighterMarioFoxPreviewLoopResult =
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS;
        gNdsFighterMarioFoxPreviewLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_SAFE_PASS;
    }
}

static void ndsFighterGCRunAllLoopCopyFromPreview(void)
{
    gNdsFighterGCRunAllLoopP0PlaybackApplyCount =
        gNdsFighterPreviewLoopP0PlaybackApplyCount;
    gNdsFighterGCRunAllLoopP1PlaybackApplyCount =
        gNdsFighterPreviewLoopP1PlaybackApplyCount;
    gNdsFighterGCRunAllLoopP0ControllerToFTInputCount =
        gNdsFighterPreviewLoopP0ControllerToFTInputCount;
    gNdsFighterGCRunAllLoopP1ControllerToFTInputCount =
        gNdsFighterPreviewLoopP1ControllerToFTInputCount;
    gNdsFighterGCRunAllLoopP0DirectFTInputWriteCount = 0u;
    gNdsFighterGCRunAllLoopP1DirectFTInputWriteCount = 0u;
    gNdsFighterGCRunAllLoopP0ButtonTapMask =
        gNdsFighterPreviewLoopP0ButtonTapMask;
    gNdsFighterGCRunAllLoopP1ButtonTapMask =
        gNdsFighterPreviewLoopP1ButtonTapMask;
    gNdsFighterGCRunAllLoopP0ButtonHoldMask =
        gNdsFighterPreviewLoopP0ButtonHoldMask;
    gNdsFighterGCRunAllLoopP1ButtonHoldMask =
        gNdsFighterPreviewLoopP1ButtonHoldMask;
    gNdsFighterGCRunAllLoopP0LastStickX =
        gNdsFighterPreviewLoopP0LastStickX;
    gNdsFighterGCRunAllLoopP1LastStickX =
        gNdsFighterPreviewLoopP1LastStickX;
    gNdsFighterGCRunAllLoopP0LastStickY =
        gNdsFighterPreviewLoopP0LastStickY;
    gNdsFighterGCRunAllLoopP1LastStickY =
        gNdsFighterPreviewLoopP1LastStickY;
    gNdsFighterGCRunAllLoopP0DashTapEligibleCount =
        gNdsFighterPreviewLoopP0DashTapEligibleCount;
    gNdsFighterGCRunAllLoopP1DashTapEligibleCount =
        gNdsFighterPreviewLoopP1DashTapEligibleCount;
    gNdsFighterGCRunAllLoopP0JumpButtonTapCount =
        gNdsFighterPreviewLoopP0JumpButtonTapCount;
    gNdsFighterGCRunAllLoopP1JumpButtonTapCount =
        gNdsFighterPreviewLoopP1JumpButtonTapCount;
    gNdsFighterGCRunAllLoopP0FrameCount =
        gNdsFighterPreviewLoopP0FrameCount;
    gNdsFighterGCRunAllLoopP1FrameCount =
        gNdsFighterPreviewLoopP1FrameCount;
    gNdsFighterGCRunAllLoopP0Completed =
        gNdsFighterPreviewLoopP0Completed;
    gNdsFighterGCRunAllLoopP1Completed =
        gNdsFighterPreviewLoopP1Completed;
    gNdsFighterGCRunAllLoopP0StatusVisitMask =
        gNdsFighterPreviewLoopP0StatusVisitMask;
    gNdsFighterGCRunAllLoopP1StatusVisitMask =
        gNdsFighterPreviewLoopP1StatusVisitMask;
    gNdsFighterGCRunAllLoopP0TransitionMask =
        gNdsFighterPreviewLoopP0TransitionMask;
    gNdsFighterGCRunAllLoopP1TransitionMask =
        gNdsFighterPreviewLoopP1TransitionMask;
    gNdsFighterGCRunAllLoopP0WaitVisitCount =
        sNdsFighterPreviewLoopStates[0].wait_visit_count;
    gNdsFighterGCRunAllLoopP1WaitVisitCount =
        sNdsFighterPreviewLoopStates[1].wait_visit_count;
    gNdsFighterGCRunAllLoopP0WalkVisitCount =
        sNdsFighterPreviewLoopStates[0].walk_visit_count;
    gNdsFighterGCRunAllLoopP1WalkVisitCount =
        sNdsFighterPreviewLoopStates[1].walk_visit_count;
    gNdsFighterGCRunAllLoopP0DashVisitCount =
        sNdsFighterPreviewLoopStates[0].dash_visit_count;
    gNdsFighterGCRunAllLoopP1DashVisitCount =
        sNdsFighterPreviewLoopStates[1].dash_visit_count;
    gNdsFighterGCRunAllLoopP0RunVisitCount =
        sNdsFighterPreviewLoopStates[0].run_visit_count;
    gNdsFighterGCRunAllLoopP1RunVisitCount =
        sNdsFighterPreviewLoopStates[1].run_visit_count;
    gNdsFighterGCRunAllLoopP0RunBrakeVisitCount =
        sNdsFighterPreviewLoopStates[0].runbrake_visit_count;
    gNdsFighterGCRunAllLoopP1RunBrakeVisitCount =
        sNdsFighterPreviewLoopStates[1].runbrake_visit_count;
    gNdsFighterGCRunAllLoopP0KneeBendVisitCount =
        sNdsFighterPreviewLoopStates[0].kneebend_visit_count;
    gNdsFighterGCRunAllLoopP1KneeBendVisitCount =
        sNdsFighterPreviewLoopStates[1].kneebend_visit_count;
    gNdsFighterGCRunAllLoopP0JumpVisitCount =
        sNdsFighterPreviewLoopStates[0].jump_visit_count;
    gNdsFighterGCRunAllLoopP1JumpVisitCount =
        sNdsFighterPreviewLoopStates[1].jump_visit_count;
    gNdsFighterGCRunAllLoopP0FallVisitCount =
        sNdsFighterPreviewLoopStates[0].fall_visit_count;
    gNdsFighterGCRunAllLoopP1FallVisitCount =
        sNdsFighterPreviewLoopStates[1].fall_visit_count;
    gNdsFighterGCRunAllLoopP0LandingVisitCount =
        sNdsFighterPreviewLoopStates[0].landing_visit_count;
    gNdsFighterGCRunAllLoopP1LandingVisitCount =
        sNdsFighterPreviewLoopStates[1].landing_visit_count;
    gNdsFighterGCRunAllLoopP0StatusStart =
        gNdsFighterPreviewLoopP0StatusStart;
    gNdsFighterGCRunAllLoopP1StatusStart =
        gNdsFighterPreviewLoopP1StatusStart;
    gNdsFighterGCRunAllLoopP0MotionStart =
        gNdsFighterPreviewLoopP0MotionStart;
    gNdsFighterGCRunAllLoopP1MotionStart =
        gNdsFighterPreviewLoopP1MotionStart;
    gNdsFighterGCRunAllLoopP0StatusFinal =
        gNdsFighterPreviewLoopP0StatusFinal;
    gNdsFighterGCRunAllLoopP1StatusFinal =
        gNdsFighterPreviewLoopP1StatusFinal;
    gNdsFighterGCRunAllLoopP0MotionFinal =
        gNdsFighterPreviewLoopP0MotionFinal;
    gNdsFighterGCRunAllLoopP1MotionFinal =
        gNdsFighterPreviewLoopP1MotionFinal;
    gNdsFighterGCRunAllLoopP0GAFinal =
        gNdsFighterPreviewLoopP0GAFinal;
    gNdsFighterGCRunAllLoopP1GAFinal =
        gNdsFighterPreviewLoopP1GAFinal;
    gNdsFighterGCRunAllLoopP0RootXStartMilli =
        gNdsFighterPreviewLoopP0RootXStartMilli;
    gNdsFighterGCRunAllLoopP1RootXStartMilli =
        gNdsFighterPreviewLoopP1RootXStartMilli;
    gNdsFighterGCRunAllLoopP0RootDeltaXMilli =
        gNdsFighterPreviewLoopP0RootDeltaXMilli;
    gNdsFighterGCRunAllLoopP1RootDeltaXMilli =
        gNdsFighterPreviewLoopP1RootDeltaXMilli;
    gNdsFighterGCRunAllLoopP0RootRiseMilli =
        gNdsFighterPreviewLoopP0RootRiseMilli;
    gNdsFighterGCRunAllLoopP1RootRiseMilli =
        gNdsFighterPreviewLoopP1RootRiseMilli;
    gNdsFighterGCRunAllLoopP0RootYFinalMilli =
        gNdsFighterPreviewLoopP0RootYFinalMilli;
    gNdsFighterGCRunAllLoopP1RootYFinalMilli =
        gNdsFighterPreviewLoopP1RootYFinalMilli;
    gNdsFighterGCRunAllLoopP0FloorYMilli =
        gNdsFighterPreviewLoopP0FloorYMilli;
    gNdsFighterGCRunAllLoopP1FloorYMilli =
        gNdsFighterPreviewLoopP1FloorYMilli;
    gNdsFighterGCRunAllLoopP0RootDirectionOK =
        gNdsFighterPreviewLoopP0RootDirectionOK;
    gNdsFighterGCRunAllLoopP1RootDirectionOK =
        gNdsFighterPreviewLoopP1RootDirectionOK;
    gNdsFighterGCRunAllLoopP0FloorOK =
        gNdsFighterPreviewLoopP0FloorOK;
    gNdsFighterGCRunAllLoopP1FloorOK =
        gNdsFighterPreviewLoopP1FloorOK;
    gNdsFighterGCRunAllLoopP0InterruptCount =
        gNdsFighterPreviewLoopP0InterruptCount;
    gNdsFighterGCRunAllLoopP1InterruptCount =
        gNdsFighterPreviewLoopP1InterruptCount;
    gNdsFighterGCRunAllLoopP0PhysicsCount =
        gNdsFighterPreviewLoopP0PhysicsCount;
    gNdsFighterGCRunAllLoopP1PhysicsCount =
        gNdsFighterPreviewLoopP1PhysicsCount;
    gNdsFighterGCRunAllLoopP0IntegrateCount =
        gNdsFighterPreviewLoopP0IntegrateCount;
    gNdsFighterGCRunAllLoopP1IntegrateCount =
        gNdsFighterPreviewLoopP1IntegrateCount;
    gNdsFighterGCRunAllLoopP0MapCount =
        gNdsFighterPreviewLoopP0MapCount;
    gNdsFighterGCRunAllLoopP1MapCount =
        gNdsFighterPreviewLoopP1MapCount;
    gNdsFighterGCRunAllLoopPreviewWidth =
        gNdsFighterPreviewLoopPreviewWidth;
    gNdsFighterGCRunAllLoopPreviewHeight =
        gNdsFighterPreviewLoopPreviewHeight;
    gNdsFighterGCRunAllLoopPreviewPitch =
        gNdsFighterPreviewLoopPreviewPitch;
    gNdsFighterGCRunAllLoopPreviewReady =
        gNdsFighterPreviewLoopPreviewReady;
    gNdsFighterGCRunAllLoopPreviewCommitBefore =
        gNdsFighterPreviewLoopPreviewCommitBefore;
    gNdsFighterGCRunAllLoopPreviewCommitAfter =
        gNdsFighterPreviewLoopPreviewCommitAfter;
    gNdsFighterGCRunAllLoopPreviewCommitDelta =
        gNdsFighterPreviewLoopPreviewCommitDelta;
    gNdsFighterGCRunAllLoopDrawFrameCount =
        gNdsFighterPreviewLoopDrawFrameCount;
    gNdsFighterGCRunAllLoopDisplayCallbackCount =
        gNdsFighterPreviewLoopDisplayCallbackCount;
    gNdsFighterGCRunAllLoopP0DisplayCallbackCount =
        gNdsFighterPreviewLoopP0DisplayCallbackCount;
    gNdsFighterGCRunAllLoopP1DisplayCallbackCount =
        gNdsFighterPreviewLoopP1DisplayCallbackCount;
    gNdsFighterGCRunAllLoopP0CandidateCount =
        gNdsFighterPreviewLoopP0CandidateCount;
    gNdsFighterGCRunAllLoopP1CandidateCount =
        gNdsFighterPreviewLoopP1CandidateCount;
    gNdsFighterGCRunAllLoopP0DrawnDObjCount =
        gNdsFighterPreviewLoopP0DrawnDObjCount;
    gNdsFighterGCRunAllLoopP1DrawnDObjCount =
        gNdsFighterPreviewLoopP1DrawnDObjCount;
    gNdsFighterGCRunAllLoopP0PixelCount =
        gNdsFighterPreviewLoopP0PixelCount;
    gNdsFighterGCRunAllLoopP1PixelCount =
        gNdsFighterPreviewLoopP1PixelCount;
    gNdsFighterGCRunAllLoopTotalPixelCount =
        gNdsFighterPreviewLoopTotalPixelCount;
    gNdsFighterGCRunAllLoopP0ColorChecksum =
        gNdsFighterPreviewLoopP0ColorChecksum;
    gNdsFighterGCRunAllLoopP1ColorChecksum =
        gNdsFighterPreviewLoopP1ColorChecksum;
    gNdsFighterGCRunAllLoopP0ScreenXStart =
        gNdsFighterPreviewLoopP0ScreenXStart;
    gNdsFighterGCRunAllLoopP1ScreenXStart =
        gNdsFighterPreviewLoopP1ScreenXStart;
    gNdsFighterGCRunAllLoopP0ScreenXFinal =
        gNdsFighterPreviewLoopP0ScreenXFinal;
    gNdsFighterGCRunAllLoopP1ScreenXFinal =
        gNdsFighterPreviewLoopP1ScreenXFinal;
    gNdsFighterGCRunAllLoopP0ScreenXDelta =
        gNdsFighterPreviewLoopP0ScreenXDelta;
    gNdsFighterGCRunAllLoopP1ScreenXDelta =
        gNdsFighterPreviewLoopP1ScreenXDelta;
    gNdsFighterGCRunAllLoopP0ScreenYFloor =
        gNdsFighterPreviewLoopP0ScreenYFloor;
    gNdsFighterGCRunAllLoopP1ScreenYFloor =
        gNdsFighterPreviewLoopP1ScreenYFloor;
    gNdsFighterGCRunAllLoopP0ScreenYMin =
        gNdsFighterPreviewLoopP0ScreenYMin;
    gNdsFighterGCRunAllLoopP1ScreenYMin =
        gNdsFighterPreviewLoopP1ScreenYMin;
    gNdsFighterGCRunAllLoopP0ScreenRise =
        gNdsFighterPreviewLoopP0ScreenRise;
    gNdsFighterGCRunAllLoopP1ScreenRise =
        gNdsFighterPreviewLoopP1ScreenRise;
    gNdsFighterGCRunAllLoopFallDetectCount =
        gNdsFighterPreviewLoopFallDetectCount;
    gNdsFighterGCRunAllLoopLandingDetectCount =
        gNdsFighterPreviewLoopLandingDetectCount;
    gNdsFighterGCRunAllLoopSetGroundCount =
        gNdsFighterPreviewLoopSetGroundCount;
    gNdsFighterGCRunAllLoopSetAirCount =
        gNdsFighterPreviewLoopSetAirCount;
    gNdsFighterGCRunAllLoopWaitSetStatusCount =
        gNdsFighterPreviewLoopWaitSetStatusCount;
    gNdsFighterGCRunAllLoopRunBrakeEndCount =
        gNdsFighterPreviewLoopRunBrakeEndCount;
    gNdsFighterGCRunAllLoopJumpAnimEndCount =
        gNdsFighterPreviewLoopJumpAnimEndCount;
    gNdsFighterGCRunAllLoopLandingEndCount =
        gNdsFighterPreviewLoopLandingEndCount;
    gNdsFighterGCRunAllLoopDeferredInterruptCheckCount =
        gNdsFighterPreviewLoopDeferredInterruptCheckCount;
    gNdsFighterGCRunAllLoopUnexpectedStatusCount =
        gNdsFighterPreviewLoopUnexpectedStatusCount;
    gNdsFighterGCRunAllLoopDeniedStatusCount =
        gNdsFighterPreviewLoopDeniedStatusCount;
    gNdsFighterGCRunAllLoopDisplayProbeCount = 0u;
    gNdsFighterGCRunAllLoopGameplayUpdateCount = 0u;
    gNdsFighterGCRunAllLoopDrawCallCount = 0u;
    gNdsFighterGCRunAllLoopMatrixCallCount = 0u;
    gNdsFighterGCRunAllLoopRootYDriftCount =
        gNdsFighterPreviewLoopRootYDriftCount;
    gNdsFighterGCRunAllLoopGADriftCount =
        gNdsFighterPreviewLoopGADriftCount;
}

static void ndsFighterGCRunAllLoopPauseProofOwnedProcesses(void)
{
    u32 i;

    for (i = 0u; i < 2u; i++)
    {
        if (sNdsFighterSchedulerLoopProcesses[i] != NULL)
        {
            gcPauseGObjProcess(sNdsFighterSchedulerLoopProcesses[i]);
            gNdsFighterGCRunAllLoopOldProcessPauseCount++;
        }
        if (sNdsFighterControllerLoopProcesses[i] != NULL)
        {
            gcPauseGObjProcess(sNdsFighterControllerLoopProcesses[i]);
            gNdsFighterGCRunAllLoopOldProcessPauseCount++;
        }
        if (sNdsFighterPreviewLoopProcesses[i] != NULL)
        {
            gcPauseGObjProcess(sNdsFighterPreviewLoopProcesses[i]);
            gNdsFighterGCRunAllLoopOldProcessPauseCount++;
        }
    }
}

static void ndsFighterGCRunAllLoopPauseNonTargetGObjVisitor(GObj *gobj,
                                                            u32 param)
{
    GObj *target0 = sNdsFighterStructPool[0].fighter_gobj;
    GObj *target1 = sNdsFighterStructPool[1].fighter_gobj;
    (void)param;

    if (gobj == NULL)
    {
        return;
    }
    if ((gobj == target0) || (gobj == target1))
    {
        gNdsFighterGCRunAllLoopTargetProcessPreserveCount++;
        return;
    }
    gNdsFighterGCRunAllLoopNonTargetGObjVisitCount++;
    if (gobj->gobjproc_head != NULL)
    {
        gcPauseGObjProcessAll(gobj);
        gNdsFighterGCRunAllLoopNonTargetProcessPauseCount++;
    }
    gobj->flags |= GOBJ_FLAG_NORUN;
}

static void ndsFighterGCRunAllLoopPauseNonTargetProcesses(void)
{
    gcFuncGObjAll(ndsFighterGCRunAllLoopPauseNonTargetGObjVisitor, 0u);
}

static void ndsFighterGCRunAllLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 slot = 2u;

    if ((fp != NULL) && (fp->player < 2))
    {
        slot = fp->player;
    }
    if ((slot >= 2u) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        gNdsFighterGCRunAllLoopProcessAttachEscapeCount++;
        return;
    }
    if (slot == 0u)
    {
        gNdsFighterGCRunAllLoopP0ProcCallbackCount++;
        gNdsFighterGCRunAllLoopP0GObjProcessRunCount++;
    }
    else
    {
        gNdsFighterGCRunAllLoopP1ProcCallbackCount++;
        gNdsFighterGCRunAllLoopP1GObjProcessRunCount++;
    }
    sNdsFighterGCRunAllLoopActive = TRUE;
    ndsFighterPreviewLoopRunSlotProcess(slot, fp);
    sNdsFighterGCRunAllLoopActive = FALSE;
}

void ndsFighterMarioFoxGCRunAllLoopPrepare(void)
{
    u32 i;

    if ((ndsFighterMarioFoxGCRunAllLoopProofEnabled() == FALSE) ||
        (gNdsFighterGCRunAllLoopPrepared != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxPreviewLoopResult !=
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS) ||
        (gNdsFighterMarioFoxPreviewLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_SAFE_PASS) ||
        ((gNdsFighterMarioFoxPreviewLoopMask & 0x7ffu) != 0x7ffu) ||
        (gNdsFighterMarioFoxPreviewLoopDeferredMask != 0xffu) ||
        (gNdsFighterMarioFoxPreviewLoopCount != 2u) ||
        (gNdsFighterPreviewLoopP0StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterPreviewLoopP1StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterPreviewLoopP0MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterPreviewLoopP1MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterPreviewLoopP0GAFinal != (u32)nMPKineticsGround) ||
        (gNdsFighterPreviewLoopP1GAFinal != (u32)nMPKineticsGround))
    {
        return;
    }

    bzero(sNdsFighterPreviewLoopStates,
          sizeof(sNdsFighterPreviewLoopStates));
    bzero(sNdsFighterGCRunAllLoopProcesses,
          sizeof(sNdsFighterGCRunAllLoopProcesses));
    ndsControllerPlaybackReset();
    ndsControllerPlaybackSetConnectedMask(0x3u);
    ndsControllerPlaybackSetEnabled(TRUE);
    gNdsFighterGCRunAllLoopFrameMax =
        NDS_FIGHTER_GCRUNALL_LOOP_FRAME_MAX;
    gNdsFighterGCRunAllLoopUpdateMax =
        NDS_FIGHTER_GCRUNALL_LOOP_UPDATE_MAX;
    gNdsFighterGCRunAllLoopGObjCountBefore = (u32)gcGetGObjsActiveNum();

    ndsFighterGCRunAllLoopPauseProofOwnedProcesses();
    ndsFighterGCRunAllLoopPauseNonTargetProcesses();

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj = fp->fighter_gobj;
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL) || (root == NULL))
        {
            gNdsFighterGCRunAllLoopProcessAttachEscapeCount++;
            continue;
        }
        fighter_gobj->flags |= GOBJ_FLAG_NORUN;
        ndsFighterPreviewLoopRecordStart(i, fp, root);
        sNdsFighterGCRunAllLoopProcesses[i] =
            gcAddGObjProcess(fighter_gobj,
                             ndsFighterGCRunAllLoopGObjProc,
                             nGCProcessKindFunc,
                             3);
        if (sNdsFighterGCRunAllLoopProcesses[i] == NULL)
        {
            gNdsFighterGCRunAllLoopProcessAttachEscapeCount++;
        }
        else if (i == 0u)
        {
            gNdsFighterGCRunAllLoopP0ProcessAttachCount++;
        }
        else
        {
            gNdsFighterGCRunAllLoopP1ProcessAttachCount++;
        }
    }

    ndsFighterGCRunAllLoopCopyFromPreview();
    if ((sNdsFighterGCRunAllLoopProcesses[0] != NULL) &&
        (sNdsFighterGCRunAllLoopProcesses[1] != NULL))
    {
        gNdsFighterGCRunAllLoopPrepared = 1u;
    }
}

s32 ndsFighterMarioFoxGCRunAllLoopUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxGCRunAllLoopProofEnabled() != FALSE) &&
            (gNdsFighterGCRunAllLoopPrepared != 0u) &&
            (gNdsFighterMarioFoxGCRunAllLoopResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxGCRunAllLoopRunVSBattleUpdate(void)
{
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxGCRunAllLoopUpdateEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterGCRunAllLoopVSBattleUpdateCount++;

    for (i = 0u; i < 2u; i++)
    {
        ndsFighterPreviewLoopApplyPlayback(i, &sNdsFighterStructPool[i]);
    }
    ndsControllerPlaybackCommitFrame();
    syControllerReadDeviceData();
    gNdsFighterGCRunAllLoopSYReadCount++;
    syControllerUpdateGlobalData();
    gNdsFighterGCRunAllLoopSYUpdateCount++;

    sNdsFighterGCRunAllLoopActive = TRUE;
    gcRunAll();
    sNdsFighterGCRunAllLoopActive = FALSE;
    gNdsFighterGCRunAllLoopRunAllCount++;

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root = fp->joints[nFTPartsJointTopN];
        ndsFighterPreviewLoopRecordFinal(i, fp, root);
    }

    if (((gNdsFighterGCRunAllLoopVSBattleUpdateCount %
            NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_INTERVAL) == 0u) ||
        ((gNdsFighterPreviewLoopP0Completed == 1u) &&
         (gNdsFighterPreviewLoopP1Completed == 1u)))
    {
        ndsFighterPreviewLoopDrawKeyframe();
    }
    ndsFighterGCRunAllLoopCopyFromPreview();

    if ((gNdsFighterGCRunAllLoopP0Completed == 1u) &&
        (gNdsFighterGCRunAllLoopP1Completed == 1u))
    {
        gNdsFighterGCRunAllLoopRootYDriftCount = 0u;
        gNdsFighterGCRunAllLoopGADriftCount = 0u;
        if ((gNdsFighterGCRunAllLoopP0StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterGCRunAllLoopP1StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterGCRunAllLoopP0MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterGCRunAllLoopP1MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterGCRunAllLoopP0GAFinal !=
                (u32)nMPKineticsGround) ||
            (gNdsFighterGCRunAllLoopP1GAFinal !=
                (u32)nMPKineticsGround))
        {
            gNdsFighterGCRunAllLoopGADriftCount++;
        }
        if ((gNdsFighterGCRunAllLoopP0FloorOK != 1u) ||
            (gNdsFighterGCRunAllLoopP1FloorOK != 1u))
        {
            gNdsFighterGCRunAllLoopRootYDriftCount++;
        }
    }

    if ((gNdsFighterMarioFoxPreviewLoopResult ==
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS) &&
        (gNdsFighterMarioFoxPreviewLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsControllerPlaybackEnabled == 1u) &&
        ((gNdsControllerPlaybackConnectedMask & 0x3u) == 0x3u) &&
        (gNdsControllerPlaybackFrameCount > 0u) &&
        (gNdsControllerPlaybackReadCount > 0u) &&
        (gNdsControllerLiveReadCount == 0u) &&
        (gNdsFighterGCRunAllLoopSYReadCount ==
            gNdsFighterGCRunAllLoopSYUpdateCount))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterGCRunAllLoopPrepared == 1u) &&
        (gNdsFighterGCRunAllLoopTaskmanUpdateCount > 0u) &&
        (gNdsFighterGCRunAllLoopVSBattleUpdateCount > 0u) &&
        (gNdsFighterGCRunAllLoopRunAllCount > 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterGCRunAllLoopP0ProcessAttachCount == 1u) &&
        (gNdsFighterGCRunAllLoopP1ProcessAttachCount == 1u) &&
        (gNdsFighterGCRunAllLoopP0GObjProcessRunCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1GObjProcessRunCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0ProcCallbackCount ==
            gNdsFighterGCRunAllLoopP0GObjProcessRunCount) &&
        (gNdsFighterGCRunAllLoopP1ProcCallbackCount ==
            gNdsFighterGCRunAllLoopP1GObjProcessRunCount))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterGCRunAllLoopP0PlaybackApplyCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1PlaybackApplyCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0ControllerToFTInputCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1ControllerToFTInputCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0DirectFTInputWriteCount == 0u) &&
        (gNdsFighterGCRunAllLoopP1DirectFTInputWriteCount == 0u) &&
        (gNdsFighterGCRunAllLoopP0ButtonTapMask != 0u) &&
        (gNdsFighterGCRunAllLoopP1ButtonTapMask != 0u) &&
        (gNdsFighterGCRunAllLoopP0ButtonHoldMask != 0u) &&
        (gNdsFighterGCRunAllLoopP1ButtonHoldMask != 0u) &&
        (gNdsFighterGCRunAllLoopP0DashTapEligibleCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1DashTapEligibleCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0JumpButtonTapCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1JumpButtonTapCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterGCRunAllLoopP0Completed == 1u) &&
        (gNdsFighterGCRunAllLoopP1Completed == 1u) &&
        (gNdsFighterGCRunAllLoopP0FrameCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1FrameCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0FrameCount <=
            gNdsFighterGCRunAllLoopFrameMax) &&
        (gNdsFighterGCRunAllLoopP1FrameCount <=
            gNdsFighterGCRunAllLoopFrameMax))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterGCRunAllLoopP0StatusVisitMask &
            NDS_FIGHTER_GCRUNALL_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_GCRUNALL_LOOP_STATUS_MASK_REQUIRED) &&
        ((gNdsFighterGCRunAllLoopP1StatusVisitMask &
            NDS_FIGHTER_GCRUNALL_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_GCRUNALL_LOOP_STATUS_MASK_REQUIRED))
    {
        mask |= 1u << 6;
    }
    if (((gNdsFighterGCRunAllLoopP0TransitionMask &
            NDS_FIGHTER_GCRUNALL_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_GCRUNALL_LOOP_TRANSITION_MASK_REQUIRED) &&
        ((gNdsFighterGCRunAllLoopP1TransitionMask &
            NDS_FIGHTER_GCRUNALL_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_GCRUNALL_LOOP_TRANSITION_MASK_REQUIRED))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterGCRunAllLoopP0InterruptCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1InterruptCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0PhysicsCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1PhysicsCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0IntegrateCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1IntegrateCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0MapCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1MapCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterGCRunAllLoopP0RootDeltaXMilli != 0) &&
        (gNdsFighterGCRunAllLoopP1RootDeltaXMilli != 0) &&
        (gNdsFighterGCRunAllLoopP0RootRiseMilli > 0) &&
        (gNdsFighterGCRunAllLoopP1RootRiseMilli > 0) &&
        (gNdsFighterGCRunAllLoopP0RootDirectionOK == 1u) &&
        (gNdsFighterGCRunAllLoopP1RootDirectionOK == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterGCRunAllLoopPreviewReady != 0u) &&
        (gNdsFighterGCRunAllLoopPreviewCommitDelta >=
            NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterGCRunAllLoopDrawFrameCount >=
            NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterGCRunAllLoopDisplayCallbackCount >=
            (NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_MIN * 2u)) &&
        (gNdsFighterGCRunAllLoopP0CandidateCount >= 14u) &&
        (gNdsFighterGCRunAllLoopP1CandidateCount >= 18u) &&
        (gNdsFighterGCRunAllLoopP0PixelCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1PixelCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0ColorChecksum != 0u) &&
        (gNdsFighterGCRunAllLoopP1ColorChecksum != 0u) &&
        (gNdsFighterGCRunAllLoopP0ScreenXDelta != 0) &&
        (gNdsFighterGCRunAllLoopP1ScreenXDelta != 0) &&
        (gNdsFighterGCRunAllLoopP0ScreenRise > 0) &&
        (gNdsFighterGCRunAllLoopP1ScreenRise > 0))
    {
        mask |= 1u << 10;
    }
    if ((gNdsFighterGCRunAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCRunAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCRunAllLoopP0MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterGCRunAllLoopP1MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterGCRunAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCRunAllLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCRunAllLoopP0FloorOK == 1u) &&
        (gNdsFighterGCRunAllLoopP1FloorOK == 1u))
    {
        mask |= 1u << 11;
    }

    gNdsFighterGCRunAllLoopGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterGCRunAllLoopGObjDelta =
        (gNdsFighterGCRunAllLoopGObjCountAfter >=
         gNdsFighterGCRunAllLoopGObjCountBefore) ?
        (gNdsFighterGCRunAllLoopGObjCountAfter -
         gNdsFighterGCRunAllLoopGObjCountBefore) :
        (gNdsFighterGCRunAllLoopGObjCountBefore -
         gNdsFighterGCRunAllLoopGObjCountAfter);

    if ((gNdsFighterGCRunAllLoopGObjDelta == 0u) &&
        (gNdsFighterGCRunAllLoopUnexpectedStatusCount == 0u) &&
        (gNdsFighterGCRunAllLoopDeniedStatusCount == 0u) &&
        (gNdsFighterGCRunAllLoopProcessAttachEscapeCount == 0u) &&
        (gNdsFighterGCRunAllLoopDisplayProbeCount == 0u) &&
        (gNdsFighterGCRunAllLoopGameplayUpdateCount == 0u) &&
        (gNdsFighterGCRunAllLoopDrawCallCount == 0u) &&
        (gNdsFighterGCRunAllLoopMatrixCallCount == 0u) &&
        (gNdsFighterGCRunAllLoopRootYDriftCount == 0u) &&
        (gNdsFighterGCRunAllLoopGADriftCount == 0u) &&
        (gNdsFighterGCRunAllLoopOldProcessPauseCount > 0u) &&
        (gNdsFighterGCRunAllLoopNonTargetGObjVisitCount > 0u) &&
        (gNdsFighterGCRunAllLoopTargetProcessPreserveCount >= 2u))
    {
        mask |= 1u << 12;
    }

    gNdsFighterMarioFoxGCRunAllLoopMask = mask;
    gNdsFighterMarioFoxGCRunAllLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxGCRunAllLoopCount =
        gNdsFighterGCRunAllLoopP0Completed +
        gNdsFighterGCRunAllLoopP1Completed;

    if ((mask & 0x1fffu) == 0x1fffu)
    {
        gNdsFighterMarioFoxGCRunAllLoopResult =
            NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_PASS;
        gNdsFighterMarioFoxGCRunAllLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_SAFE_PASS;
    }
}

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
#define NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED 300u
#define NDS_FIGHTER_NATURAL_MOTION_WALK_FRAMES_REQUIRED 8u
#define NDS_FIGHTER_NATURAL_STAGE_SIDE_MASK_REQUIRED 0x24fu
#define NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED 10u
#define NDS_FIGHTER_NATURAL_COMBAT_DASH_FRAMES_REQUIRED 2u
#define NDS_FIGHTER_NATURAL_COMBAT_RUN_FRAMES_REQUIRED 8u
#define NDS_FIGHTER_NATURAL_COMBAT_RUNBRAKE_FRAMES_REQUIRED 2u
#define NDS_FIGHTER_NATURAL_COMBAT_TURN_FRAMES_REQUIRED 1u
#define NDS_FIGHTER_NATURAL_COMBAT_GUARD_FRAMES_REQUIRED 10u
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_DASH_RANGE 1000.0F
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_STOP_RANGE 240.0F
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE 100.0F
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_STEP 15.0F
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_MIN 50.0F
#define NDS_FIGHTER_NATURAL_MOVESET_SAFE_RANGE 80.0F
#define NDS_FIGHTER_NATURAL_MOVESET_GRAB_STOP_RANGE (260.0F + 30.0F)
#define NDS_FIGHTER_NATURAL_PROJECTILE_STOP_RANGE \
    ((112.5F * 2.0F) + 350.0F)
#define NDS_FIGHTER_NATURAL_PROJECTILE_CENTER_RANGE 300.0F
#define NDS_FIGHTER_NATURAL_COMBAT_ATTACK_NEUTRAL_FRAMES 4u
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
#define NDS_FIGHTER_NATURAL_MOVESET_MASK_ALL 0x7ffu
#define NDS_FIGHTER_SPECIALS_MARIO_HI_MASK 0x000fu
#define NDS_FIGHTER_SPECIALS_MARIO_LW_MASK 0x0070u
#define NDS_FIGHTER_SPECIALS_FOX_HI_MASK 0x0f80u
#define NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED 60u
#if NDS_P2_DONKEY
#define NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED 2u
#define NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED 10u
#endif

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
    nNDSNaturalCombatPhaseProjectileObserve
};

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
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
#endif

#if NDS_P2_SAMUS_STATE_TOUR
enum {
    nNDSSamusStateTourQuickAttack = 0,
    nNDSSamusStateTourQuickEscape,
    nNDSSamusStateTourQuickClimb,
    nNDSSamusStateTourSlowAttack,
    nNDSSamusStateTourSlowEscape,
    nNDSSamusStateTourSlowClimb,
    nNDSSamusStateTourDone
};

enum {
    nNDSSamusStateTourStepPrepare = 0,
    nNDSSamusStateTourStepRunOff,
    nNDSSamusStateTourStepAwaitCliff,
    nNDSSamusStateTourStepCliffWait,
    nNDSSamusStateTourStepRecover
};

#define NDS_SAMUS_STATE_TOUR_LEDGE_MASK_ALL 0x0001ffffu
#define NDS_SAMUS_STATE_TOUR_TIMEOUT 1800u
#endif

#if NDS_P2_SAMUS_TUMBLE_TOUR
enum {
    nNDSSamusTumbleTourPassive = 0,
    nNDSSamusTumbleTourPassiveStandF,
    nNDSSamusTumbleTourPassiveStandB,
    nNDSSamusTumbleTourDownStandD,
    nNDSSamusTumbleTourDownStandU,
    nNDSSamusTumbleTourDownForwardD,
    nNDSSamusTumbleTourDownForwardU,
    nNDSSamusTumbleTourDownBackD,
    nNDSSamusTumbleTourDownBackU,
    nNDSSamusTumbleTourDownAttackD,
    nNDSSamusTumbleTourDownAttackU,
    nNDSSamusTumbleTourDone
};

enum {
    nNDSSamusTumbleTourStepPrepareHit = 0,
    nNDSSamusTumbleTourStepAttack,
    nNDSSamusTumbleTourStepDamageFly,
    nNDSSamusTumbleTourStepDamageFall,
    nNDSSamusTumbleTourStepLanding,
    nNDSSamusTumbleTourStepDownWait,
    nNDSSamusTumbleTourStepRecover
};

#define NDS_SAMUS_TUMBLE_TOUR_MASK_ALL 0x0001ffffu
#define NDS_SAMUS_TUMBLE_TOUR_TIMEOUT 1200u
#endif

#if NDS_P2_SAMUS_DAMAGEFLY_TOUR
enum {
    nNDSSamusDamageFlyTourHi = 0,
    nNDSSamusDamageFlyTourN,
    nNDSSamusDamageFlyTourLw,
    nNDSSamusDamageFlyTourTop,
    nNDSSamusDamageFlyTourRoll,
    nNDSSamusDamageFlyTourDone
};

enum {
    nNDSSamusDamageFlyTourStepPrepare = 0,
    nNDSSamusDamageFlyTourStepRearm,
    nNDSSamusDamageFlyTourStepDrive,
    nNDSSamusDamageFlyTourStepDamageFly,
    nNDSSamusDamageFlyTourStepDamageFall,
    nNDSSamusDamageFlyTourStepLanding,
    nNDSSamusDamageFlyTourStepRecover
};

#define NDS_SAMUS_DAMAGEFLY_TOUR_MASK_ALL 0x0000001fu
#define NDS_SAMUS_DAMAGEFLY_TOUR_TIMEOUT 1200u
#define NDS_SAMUS_DAMAGEFLY_TOUR_ROLL_ATTEMPTS_MAX 32u
#define NDS_SAMUS_DAMAGEFLY_TOUR_MISMATCH_MAX 12u
#endif

#if NDS_P2_SAMUS_ATTACK_TOUR
enum {
    nNDSSamusAttackTourJab = 0,
    nNDSSamusAttackTourDash,
    nNDSSamusAttackTourS3Hi,
    nNDSSamusAttackTourS3HiS,
    nNDSSamusAttackTourS3,
    nNDSSamusAttackTourS3LwS,
    nNDSSamusAttackTourS3Lw,
    nNDSSamusAttackTourHi3,
    nNDSSamusAttackTourLw3,
    nNDSSamusAttackTourS4Hi,
    nNDSSamusAttackTourS4HiS,
    nNDSSamusAttackTourS4,
    nNDSSamusAttackTourS4LwS,
    nNDSSamusAttackTourS4Lw,
    nNDSSamusAttackTourHi4,
    nNDSSamusAttackTourLw4,
    nNDSSamusAttackTourAirN,
    nNDSSamusAttackTourAirF,
    nNDSSamusAttackTourAirB,
    nNDSSamusAttackTourAirHi,
    nNDSSamusAttackTourAirLw,
    nNDSSamusAttackTourThrowF,
    nNDSSamusAttackTourThrowB,
    nNDSSamusAttackTourDone
};

enum {
    nNDSSamusAttackTourStepPrepare = 0,
    nNDSSamusAttackTourStepRearm,
    nNDSSamusAttackTourStepDrive,
    nNDSSamusAttackTourStepRecover
};

#define NDS_SAMUS_ATTACK_TOUR_MASK_ALL 0x00ffffffu
#define NDS_SAMUS_ATTACK_TOUR_TIMEOUT 1200u
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI || NDS_P2_DONKEY
enum {
    nNDSNaturalSpecialsPhaseIdle = 0,
    nNDSNaturalSpecialsPhaseMarioHi,
    nNDSNaturalSpecialsPhaseSettleMarioHi,
    nNDSNaturalSpecialsPhaseMarioLw,
    nNDSNaturalSpecialsPhaseSettleMarioLw,
    nNDSNaturalSpecialsPhaseFoxHi,
    nNDSNaturalSpecialsPhaseSettleFoxHi,
    nNDSNaturalSpecialsPhaseDone,
#if NDS_P2_DONKEY
    /* Preserve the established 0..7 phase values consumed by the legacy
     * verifier; DK extends the same natural-input sequence after Done. */
    nNDSNaturalSpecialsPhaseDonkeyNCharge,
    nNDSNaturalSpecialsPhaseDonkeyNRelease,
    nNDSNaturalSpecialsPhaseDonkeyHi,
    nNDSNaturalSpecialsPhaseDonkeyLw
#endif
};
#endif

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
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
static u32 sNdsNaturalReflectorFoxSlot;
static u32 sNdsNaturalReflectorProjectileSlot;
static u32 sNdsNaturalReflectorButtonPressed;
#endif
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
static u32 sNdsNaturalMovesetPhase;
static u32 sNdsNaturalMovesetPhaseFrames;
static u32 sNdsNaturalMovesetSettleFrames;
static u32 sNdsNaturalMovesetDone;
static u32 sNdsNaturalMovesetKORecoveryActive;
static u32 sNdsNaturalMovesetKORecoveryPhase;
#endif
#if NDS_P2_SAMUS_STATE_TOUR
static u32 sNdsSamusStateTourScenario;
static u32 sNdsSamusStateTourStep;
static u32 sNdsSamusStateTourFrames;
static u32 sNdsSamusStateTourActionSeen;
static s32 sNdsSamusStateTourFloorLine;
static u32 sNdsSamusStateTourActive;
static u32 sNdsSamusStateTourDone;
#endif
#if NDS_P2_SAMUS_TUMBLE_TOUR
static u32 sNdsSamusTumbleTourScenario;
static u32 sNdsSamusTumbleTourStep;
static u32 sNdsSamusTumbleTourFrames;
static u32 sNdsSamusTumbleTourAttackSeen;
static u32 sNdsSamusTumbleTourActionSeen;
static u32 sNdsSamusTumbleTourDownWaitObserved;
static s32 sNdsSamusTumbleTourFloorLine;
static u32 sNdsSamusTumbleTourActive;
#endif
#if NDS_P2_SAMUS_DAMAGEFLY_TOUR
static u32 sNdsSamusDamageFlyTourScenario;
static u32 sNdsSamusDamageFlyTourStep;
static u32 sNdsSamusDamageFlyTourFrames;
static u32 sNdsSamusDamageFlyTourActive;
static u32 sNdsSamusDamageFlyTourAttackPressed;
static u32 sNdsSamusDamageFlyTourHitRecorded;
static u32 sNdsSamusDamageFlyTourScenarioAccepted;
static s32 sNdsSamusDamageFlyTourFloorLine;
#endif
#if NDS_P2_SAMUS_ATTACK_TOUR
static u32 sNdsSamusAttackTourScenario;
static u32 sNdsSamusAttackTourStep;
static u32 sNdsSamusAttackTourFrames;
static u32 sNdsSamusAttackTourActive;
static u32 sNdsSamusAttackTourExpectedMask;
static s32 sNdsSamusAttackTourFloorLine;
#endif
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI || NDS_P2_DONKEY
static u32 sNdsNaturalSpecialsPhase;
static u32 sNdsNaturalSpecialsPhaseFrames;
static u32 sNdsNaturalSpecialsDone;
static u32 sNdsNaturalSpecialsButtonPressed;
#endif

static sb32 ndsFighterBattlePlayableProofEnabled(void)
{
#if NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE && \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE)
#if NDS_HARNESS_FAST_LOGIC
    return TRUE;
#else
    return FALSE;
#endif
#else
    return FALSE;
#endif
}

static sb32 ndsFighterNaturalProjectileProofEnabled(void)
{
#if NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER && \
    (NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL || \
     NDS_IMPORT_BATTLESHIP_FOX_BLASTER)
    return (ndsFighterBattlePlayableProofEnabled() != FALSE) ? TRUE : FALSE;
#else
    return FALSE;
#endif
}

#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
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
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI || NDS_P2_DONKEY
static sb32 ndsFighterNaturalSpecialsProofEnabled(void)
{
    return (ndsFighterBattlePlayableProofEnabled() != FALSE) ? TRUE : FALSE;
}
#endif

static sb32 ndsFighterMarioFoxNaturalMotionProofEnabled(void)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_MARIOFOX_DASH_RUN_HARNESS
    return TRUE;
#else
    if (ndsFighterBattlePlayableProofEnabled() != FALSE)
    {
        return TRUE;
    }
    return (ndsFighterMarioFoxGCRunAllLoopProofEnabled() != FALSE) ? TRUE :
                                                                    FALSE;
#endif
}

static sb32 ndsFighterNaturalCombatMovementOnlyProofEnabled(void)
{
#if (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN)
    return TRUE;
#else
    return FALSE;
#endif
}

static sb32 ndsFighterNaturalCombatLiveHitProofEnabled(void)
{
#if (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP)
    return TRUE;
#else
    return FALSE;
#endif
}

static sb32 ndsFighterNaturalCombatStageSideProofEnabled(void)
{
#if NDS_MARIOFOX_STAGE_GCDRAWALL_LOOP_HARNESS && \
    !NDS_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP_HARNESS
    return TRUE;
#else
    return FALSE;
#endif
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
#if NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE
    {
        extern GObj *gGMCameraGObj;

        if (gobj == gGMCameraGObj)
        {
            return;
        }
    }
#endif
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
#if NDS_P2_LUIGI
    if (fp->fkind == nFTKindLuigi)
    {
        return TRUE;
    }
#endif
    return FALSE;
}

#if NDS_P2_DONKEY
static sb32 ndsFighterNaturalIsDonkey(const FTStruct *fp)
{
    return ((fp != NULL) && (fp->fkind == nFTKindDonkey)) ? TRUE : FALSE;
}
#endif

static u32 ndsFighterNaturalProjectileSelectSlot(FTStruct *fp[2])
{
    u32 i;

#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
    for (i = 0u; i < 2u; i++)
    {
        if (ndsFighterNaturalIsMarioSpecialFamily(fp[i]) != FALSE)
        {
            return i;
        }
    }
#endif
#if NDS_IMPORT_BATTLESHIP_FOX_BLASTER
    for (i = 0u; i < 2u; i++)
    {
        if (fp[i]->fkind == nFTKindFox)
        {
            return i;
        }
    }
#endif
#if NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL
    for (i = 0u; i < 2u; i++)
    {
        if (ndsFighterNaturalIsMarioSpecialFamily(fp[i]) != FALSE)
        {
            return i;
        }
    }
#endif
    return sNdsNaturalCombatAttackerSlot;
}

#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
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
#endif

static u32 ndsFighterNaturalProjectileExpectedKind(FTStruct *fp)
{
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
    if (ndsFighterNaturalIsMarioSpecialFamily(fp) != FALSE)
    {
        return nWPKindFireball;
    }
#endif
#if NDS_IMPORT_BATTLESHIP_FOX_BLASTER
    if ((fp != NULL) && (fp->fkind == nFTKindFox))
    {
        return nWPKindBlaster;
    }
#endif
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
#if NDS_P2_LUIGI
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
#if NDS_P2_LUIGI
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
#if NDS_P2_LUIGI
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

#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
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
            gNdsFighterReflectorProofStartFrames++;
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
            gNdsFighterReflectorProofIsReflectFrames++;
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
            gNdsFighterReflectorProofReflectLRClearFrames++;
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
#endif

static void ndsFighterNaturalProjectileRecord(FTStruct *fp[2])
{
#if NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER
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
            gNdsFighterProjectileProofAccessoryFrames++;
        }
        if (actor->motion_vars.flags.flag0 != 0)
        {
            gNdsFighterProjectileProofFlag0Frames++;
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
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
    ndsFighterNaturalReflectorRecord(fp);
#endif
#else
    (void)fp;
#endif
}

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
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
#endif

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
            gNdsFighterNaturalCombatHitboxActiveFrames++;
            ndsFighterNaturalCombatRecordAttackColl(attacker);
        }
    }
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
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
                gNdsFighterNaturalMovesetSmashHitboxFrames++;
                ndsFighterNaturalCombatRecordAttackColl(attacker);
            }
        }
        else if (ndsFighterNaturalMovesetStatusIsAerial(
                     attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetAerialFrames++;
            if (is_hitbox_active != FALSE)
            {
                gNdsFighterNaturalMovesetAerialHitboxFrames++;
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
#endif

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
        gNdsFighterNaturalCombatVictimDamageFrames++;
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
            gNdsFighterNaturalCombatVictimRecoverWaitFrames++;
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
        gNdsFighterNaturalCombatGuardOffFrames++;
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
    sNdsNaturalCombatApproachStopRange =
        NDS_FIGHTER_NATURAL_COMBAT_APPROACH_STOP_RANGE;
    sNdsNaturalCombatApproachStagnant = 0u;
    sNdsNaturalCombatApproachLastDX = 0.0F;
    sNdsNaturalCombatVictimHitSeen = 0u;
    sNdsNaturalCombatVictimHitPosX = 0.0F;
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    sNdsNaturalMovesetPhase = nNDSNaturalMovesetPhaseIdle;
    sNdsNaturalMovesetPhaseFrames = 0u;
    sNdsNaturalMovesetSettleFrames = 0u;
    sNdsNaturalMovesetDone = 0u;
    sNdsNaturalMovesetKORecoveryActive = 0u;
    sNdsNaturalMovesetKORecoveryPhase = nNDSNaturalMovesetPhaseIdle;
#endif
#if NDS_P2_SAMUS_STATE_TOUR
    sNdsSamusStateTourScenario = nNDSSamusStateTourQuickAttack;
    sNdsSamusStateTourStep = nNDSSamusStateTourStepPrepare;
    sNdsSamusStateTourFrames = 0u;
    sNdsSamusStateTourActionSeen = 0u;
    sNdsSamusStateTourFloorLine = -1;
    sNdsSamusStateTourActive = 0u;
    sNdsSamusStateTourDone = 0u;
    gNdsSamusStateTourMask = 0u;
    gNdsSamusStateTourPhase = nNDSSamusStateTourQuickAttack;
    gNdsSamusStateTourPhaseFrames = 0u;
    gNdsSamusStateTourStatus = 0u;
    gNdsSamusStateTourMotion = 0u;
    gNdsSamusStateTourCliffID = (u32)-1;
    gNdsSamusStateTourStageCount = 0u;
#endif
#if NDS_P2_SAMUS_TUMBLE_TOUR
    sNdsSamusTumbleTourScenario = nNDSSamusTumbleTourPassive;
    sNdsSamusTumbleTourStep = nNDSSamusTumbleTourStepPrepareHit;
    sNdsSamusTumbleTourFrames = 0u;
    sNdsSamusTumbleTourAttackSeen = 0u;
    sNdsSamusTumbleTourActionSeen = 0u;
    sNdsSamusTumbleTourDownWaitObserved = 0u;
    sNdsSamusTumbleTourFloorLine = -1;
    sNdsSamusTumbleTourActive = 0u;
    gNdsSamusTumbleTourMask = 0u;
    gNdsSamusTumbleTourDamageFlyMask = 0u;
    gNdsSamusTumbleTourScenario = nNDSSamusTumbleTourPassive;
    gNdsSamusTumbleTourStep = nNDSSamusTumbleTourStepPrepareHit;
    gNdsSamusTumbleTourFrames = 0u;
    gNdsSamusTumbleTourStatus = 0u;
    gNdsSamusTumbleTourMotion = 0u;
    gNdsSamusTumbleTourHitCount = 0u;
    gNdsSamusTumbleTourStageCount = 0u;
    gNdsSamusTumbleTourDone = 0u;
#endif
#if NDS_P2_SAMUS_DAMAGEFLY_TOUR
    sNdsSamusDamageFlyTourScenario = nNDSSamusDamageFlyTourHi;
    sNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepPrepare;
    sNdsSamusDamageFlyTourFrames = 0u;
    sNdsSamusDamageFlyTourActive = 0u;
    sNdsSamusDamageFlyTourAttackPressed = 0u;
    sNdsSamusDamageFlyTourHitRecorded = 0u;
    sNdsSamusDamageFlyTourScenarioAccepted = 0u;
    sNdsSamusDamageFlyTourFloorLine = -1;
    gNdsSamusDamageFlyTourMask = 0u;
    gNdsSamusDamageFlyTourScenario = nNDSSamusDamageFlyTourHi;
    gNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepPrepare;
    gNdsSamusDamageFlyTourFrames = 0u;
    gNdsSamusDamageFlyTourStatus = 0u;
    gNdsSamusDamageFlyTourMotion = 0u;
    gNdsSamusDamageFlyTourAttackerMask = 0u;
    gNdsSamusDamageFlyTourPlacementPacked = 0u;
    gNdsSamusDamageFlyTourHitCount = 0u;
    gNdsSamusDamageFlyTourRollAttempts = 0u;
    gNdsSamusDamageFlyTourSakuraiHitCount = 0u;
    gNdsSamusDamageFlyTourTopAngle80Count = 0u;
    gNdsSamusDamageFlyTourRollPercent = 0u;
    gNdsSamusDamageFlyTourMismatchCount = 0u;
    gNdsSamusDamageFlyTourStageCount = 0u;
    gNdsSamusDamageFlyTourTerminalCount = 0u;
    gNdsSamusDamageFlyTourDone = 0u;
#endif
#if NDS_P2_SAMUS_ATTACK_TOUR
    sNdsSamusAttackTourScenario = nNDSSamusAttackTourJab;
    sNdsSamusAttackTourStep = nNDSSamusAttackTourStepPrepare;
    sNdsSamusAttackTourFrames = 0u;
    sNdsSamusAttackTourActive = 0u;
    sNdsSamusAttackTourExpectedMask = 0u;
    sNdsSamusAttackTourFloorLine = -1;
    gNdsSamusAttackTourMask = 0u;
    gNdsSamusAttackTourScenario = nNDSSamusAttackTourJab;
    gNdsSamusAttackTourStep = nNDSSamusAttackTourStepPrepare;
    gNdsSamusAttackTourFrames = 0u;
    gNdsSamusAttackTourStatus = 0u;
    gNdsSamusAttackTourMotion = 0u;
    gNdsSamusAttackTourStageCount = 0u;
    gNdsSamusAttackTourTerminalCount = 0u;
    gNdsSamusAttackTourCatchAttr = 0u;
    gNdsSamusAttackTourGrabInputCount = 0u;
    gNdsSamusAttackTourCatchStatusMask = 0u;
    gNdsSamusAttackTourCatchFrames = 0u;
    gNdsSamusAttackTourCatchActiveFrames = 0u;
    gNdsSamusAttackTourCatchSearchFrames = 0u;
    gNdsSamusAttackTourCatchAttackMask = 0u;
    gNdsSamusAttackTourCatchAnimFrameMaxMilli = 0u;
    gNdsSamusAttackTourVictimGrabbableMask = 0u;
    gNdsSamusAttackTourVictimNormalMask = 0u;
    gNdsSamusAttackTourJoint36SeenCount = 0u;
    gNdsSamusAttackTourJoint36AttackMask = 0u;
    gNdsSamusAttackTourMinGrabDXMilli = 0x7fffffff;
    gNdsSamusAttackTourGrab0XMilli = 0;
    gNdsSamusAttackTourGrab1XMilli = 0;
    gNdsSamusAttackTourFoxXMilli = 0;
    gNdsSamusAttackTourDone = 0u;
#endif
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI || NDS_P2_DONKEY
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
#if NDS_P2_DONKEY
    gNdsFighterDonkeySpecialsSlot = 0u;
    if (ndsFighterNaturalIsDonkey(p1) != FALSE)
    {
        gNdsFighterDonkeySpecialsSlot = 1u;
    }
#endif
#endif
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
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
    sNdsNaturalReflectorProjectileSlot = sNdsNaturalProjectileActorSlot;
    sNdsNaturalReflectorFoxSlot =
        ndsFighterNaturalReflectorSelectFoxSlot(fp);
    sNdsNaturalReflectorButtonPressed = 0u;
#endif
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
#if NDS_DEV_LIVE_INPUT_PREVIEW
    if (ndsFighterBattlePlayableProofEnabled() != FALSE)
    {
        ndsControllerPlaybackSetEnabled(FALSE);
    }
    else
#endif
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
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
        sNdsNaturalReflectorButtonPressed = 0u;
#endif
        ndsFighterNaturalCombatSetPhase(
            nNDSNaturalCombatPhaseProjectileSettle);
    }
    return TRUE;
}

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
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

static u32 ndsFighterNaturalMovesetRecoveryPhase(u32 phase)
{
    switch (phase)
    {
    case nNDSNaturalMovesetPhaseTiltS3:
        if ((gNdsFighterNaturalMovesetTiltS3Frames > 0u) &&
            (gNdsFighterNaturalMovesetTiltHitboxFrames > 0u))
        {
            return nNDSNaturalMovesetPhaseSettleTiltS3;
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltS3:
        return nNDSNaturalMovesetPhaseTiltHi3;
    case nNDSNaturalMovesetPhaseTiltHi3:
        if ((gNdsFighterNaturalMovesetTiltHi3Frames > 0u) &&
            (gNdsFighterNaturalMovesetTiltHitboxFrames > 0u))
        {
            return nNDSNaturalMovesetPhaseSettleTiltHi3;
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltHi3:
        return nNDSNaturalMovesetPhaseTiltLw3;
    case nNDSNaturalMovesetPhaseTiltLw3:
        if ((gNdsFighterNaturalMovesetTiltLw3Frames > 0u) &&
            (gNdsFighterNaturalMovesetTiltHitboxFrames > 0u))
        {
            return nNDSNaturalMovesetPhaseSettleTiltLw3;
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltLw3:
        return nNDSNaturalMovesetPhaseSmashS4;
    case nNDSNaturalMovesetPhaseSmashS4:
        if ((gNdsFighterNaturalMovesetSmashFrames > 0u) &&
            (gNdsFighterNaturalMovesetSmashHitboxFrames > 0u))
        {
            return nNDSNaturalMovesetPhaseSettleSmashS4;
        }
        break;
    case nNDSNaturalMovesetPhaseSettleSmashS4:
        return nNDSNaturalMovesetPhaseAerialJump;
    case nNDSNaturalMovesetPhaseAerialJump:
        if (gNdsFighterNaturalMovesetAerialFrames > 0u)
        {
            return nNDSNaturalMovesetPhaseSettleAerial;
        }
        break;
    case nNDSNaturalMovesetPhaseAerialAttack:
        if (gNdsFighterNaturalMovesetAerialFrames > 0u)
        {
            return nNDSNaturalMovesetPhaseSettleAerial;
        }
        break;
    case nNDSNaturalMovesetPhaseSettleAerial:
        if (gNdsFighterNaturalMovesetLandingFrames > 0u)
        {
            return nNDSNaturalMovesetPhaseDone;
        }
        break;
    case nNDSNaturalMovesetPhaseGrabCatch:
        if (gNdsFighterNaturalMovesetCatchWaitFrames > 0u)
        {
            return nNDSNaturalMovesetPhaseGrabThrow;
        }
        break;
    case nNDSNaturalMovesetPhaseGrabThrow:
        if (gNdsFighterNaturalMovesetThrowFrames > 0u)
        {
            return nNDSNaturalMovesetPhaseSettleThrow;
        }
        break;
    default:
        break;
    }
    return ndsFighterNaturalMovesetRetryPhase(phase);
}

static sb32 ndsFighterNaturalMovesetBothGroundWait(FTStruct *fp[2])
{
    return ((ndsFighterNaturalCombatBothWait(fp) != FALSE) &&
            (fp[0]->ga == nMPKineticsGround) &&
            (fp[1]->ga == nMPKineticsGround) &&
            (ABS(fp[0]->input.pl.stick_range.x) < 20) &&
            (ABS(fp[1]->input.pl.stick_range.x) < 20) &&
            (ABS(fp[0]->input.pl.stick_range.y) < 20) &&
            (ABS(fp[1]->input.pl.stick_range.y) < 20) &&
            (fp[0]->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX) &&
            (fp[1]->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX) &&
            (fp[0]->tap_stick_y == FTINPUT_STICKBUFFER_TICS_MAX) &&
            (fp[1]->tap_stick_y == FTINPUT_STICKBUFFER_TICS_MAX)) ?
        TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetPhaseReadyToStart(FTStruct *fp[2])
{
    FTStruct *attacker = fp[sNdsNaturalCombatAttackerSlot];

    if (attacker == NULL)
    {
        return FALSE;
    }
    switch (sNdsNaturalMovesetPhase)
    {
    case nNDSNaturalMovesetPhaseTiltS3:
        if ((gNdsFighterNaturalMovesetTiltS3Frames > 0u) ||
            (ndsFighterNaturalMovesetStatusIsTiltS3(attacker->status_id) !=
                FALSE))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    case nNDSNaturalMovesetPhaseTiltHi3:
        if ((gNdsFighterNaturalMovesetTiltHi3Frames > 0u) ||
            (ndsFighterNaturalMovesetStatusIsTiltHi3(attacker->status_id) !=
                FALSE))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    case nNDSNaturalMovesetPhaseTiltLw3:
        if ((gNdsFighterNaturalMovesetTiltLw3Frames > 0u) ||
            (ndsFighterNaturalMovesetStatusIsTiltLw3(attacker->status_id) !=
                FALSE))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    case nNDSNaturalMovesetPhaseSmashS4:
        if ((gNdsFighterNaturalMovesetSmashFrames > 0u) ||
            (ndsFighterNaturalMovesetStatusIsSmash(attacker->status_id) !=
                FALSE))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    case nNDSNaturalMovesetPhaseAerialJump:
    case nNDSNaturalMovesetPhaseAerialAttack:
        if ((gNdsFighterNaturalMovesetAerialFrames > 0u) ||
            (attacker->status_id == nFTCommonStatusKneeBend) ||
            (attacker->status_id == nFTCommonStatusJumpF) ||
            (attacker->status_id == nFTCommonStatusJumpB) ||
            (attacker->status_id == nFTCommonStatusJumpAerialF) ||
            (attacker->status_id == nFTCommonStatusJumpAerialB) ||
            (attacker->status_id == nFTCommonStatusFall) ||
            (attacker->status_id == nFTCommonStatusFallAerial) ||
            (ndsFighterNaturalMovesetStatusIsAerial(attacker->status_id) !=
                FALSE))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    case nNDSNaturalMovesetPhaseGrabCatch:
        if ((gNdsFighterNaturalMovesetCatchFrames > 0u) ||
            (gNdsFighterNaturalMovesetCatchWaitFrames > 0u))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    default:
        return TRUE;
    }
}

static sb32 ndsFighterNaturalMovesetHandleKORecovery(FTStruct *fp[2])
{
    FTStruct *victim = fp[sNdsNaturalCombatVictimSlot];

    if ((victim == NULL) ||
        (sNdsNaturalMovesetPhase == nNDSNaturalMovesetPhaseIdle) ||
        (sNdsNaturalMovesetPhase == nNDSNaturalMovesetPhaseDone))
    {
        return FALSE;
    }
    if ((sNdsNaturalMovesetKORecoveryActive == 0u) &&
        (ndsFighterBattlePlayableStatusIsDead(victim->status_id) != FALSE))
    {
        sNdsNaturalMovesetKORecoveryActive = 1u;
        sNdsNaturalMovesetKORecoveryPhase =
            ndsFighterNaturalMovesetRecoveryPhase(sNdsNaturalMovesetPhase);
    }
    if (sNdsNaturalMovesetKORecoveryActive == 0u)
    {
        return FALSE;
    }
    if (ndsFighterNaturalMovesetBothGroundWait(fp) != FALSE)
    {
        u32 phase = sNdsNaturalMovesetKORecoveryPhase;

        sNdsNaturalMovesetKORecoveryActive = 0u;
        sNdsNaturalMovesetKORecoveryPhase = nNDSNaturalMovesetPhaseIdle;
        ndsFighterNaturalMovesetSetPhase(phase);
    }
    return TRUE;
}
#endif

static sb32 ndsFighterNaturalCombatSettled(FTStruct *fp[2])
{
    if ((ndsFighterNaturalCombatBothWait(fp) != FALSE) &&
        (ABS(fp[0]->input.pl.stick_range.x) < 20) &&
        (ABS(fp[1]->input.pl.stick_range.x) < 20) &&
        (fp[0]->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX) &&
        (fp[1]->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX))
    {
        sNdsNaturalCombatSettleFrames++;
    }
    else
    {
        sNdsNaturalCombatSettleFrames = 0u;
    }
    return (sNdsNaturalCombatSettleFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED) ?
        TRUE : FALSE;
}

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
static sb32 ndsFighterNaturalMovesetSettled(FTStruct *fp[2])
{
    FTStruct *victim = fp[sNdsNaturalCombatVictimSlot];

    if (ndsFighterNaturalMovesetBothGroundWait(fp) != FALSE)
    {
        sNdsNaturalMovesetSettleFrames++;
    }
    else if ((victim != NULL) &&
             (ndsFighterNaturalMovesetStatusIsRecovering(
                 victim->status_id) != FALSE))
    {
        sNdsNaturalMovesetSettleFrames = 0u;
    }
    else
    {
        sNdsNaturalMovesetSettleFrames = 0u;
    }
    return (sNdsNaturalMovesetSettleFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED) ?
        TRUE : FALSE;
}

static void ndsFighterNaturalMovesetUpdateMask(void)
{
    u32 mask = 0u;

    if (gNdsFighterNaturalMovesetTiltS3Frames > 0u)
    {
        mask |= 1u << 0;
    }
    if (gNdsFighterNaturalMovesetTiltHi3Frames > 0u)
    {
        mask |= 1u << 1;
    }
    if (gNdsFighterNaturalMovesetTiltLw3Frames > 0u)
    {
        mask |= 1u << 2;
    }
    if (gNdsFighterNaturalMovesetTiltHitboxFrames > 0u)
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterNaturalMovesetSmashFrames > 0u) &&
        (gNdsFighterNaturalMovesetSmashHitboxFrames > 0u))
    {
        mask |= 1u << 4;
    }
    if (gNdsFighterNaturalMovesetAerialFrames > 0u)
    {
        mask |= 1u << 5;
    }
    if (gNdsFighterNaturalMovesetLandingFrames > 0u)
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterNaturalMovesetCatchFrames > 0u) &&
        (gNdsFighterNaturalMovesetCatchWaitFrames > 0u))
    {
        mask |= 1u << 7;
    }
    if (gNdsFighterNaturalMovesetThrowFrames > 0u)
    {
        mask |= 1u << 8;
    }
    if (gNdsFighterNaturalMovesetThrownFrames > 0u)
    {
        mask |= 1u << 9;
    }
    if (gNdsFighterNaturalMovesetThrowRecoverFrames >=
        NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED)
    {
        mask |= 1u << 10;
    }
    gNdsFighterNaturalMovesetMask = mask;
}

static sb32 ndsFighterNaturalMovesetAdvance(FTStruct *fp[2])
{
    FTStruct *attacker = fp[sNdsNaturalCombatAttackerSlot];

    if (sNdsNaturalMovesetDone != 0u)
    {
        return TRUE;
    }
    if (sNdsNaturalMovesetPhase == nNDSNaturalMovesetPhaseIdle)
    {
        ndsFighterNaturalMovesetSetPhase(nNDSNaturalMovesetPhaseTiltS3);
    }
    if (ndsFighterNaturalMovesetHandleKORecovery(fp) != FALSE)
    {
        return FALSE;
    }
    if (ndsFighterNaturalMovesetPhaseReadyToStart(fp) == FALSE)
    {
        ndsFighterNaturalMovesetUpdateMask();
        return FALSE;
    }

    sNdsNaturalMovesetPhaseFrames++;
    gNdsFighterNaturalMovesetPhaseFrames = sNdsNaturalMovesetPhaseFrames;
    ndsFighterNaturalMovesetUpdateMask();
    if (sNdsNaturalMovesetPhaseFrames >
        NDS_FIGHTER_NATURAL_MOVESET_PHASE_TIMEOUT)
    {
        u32 recovery_phase =
            ndsFighterNaturalMovesetRecoveryPhase(sNdsNaturalMovesetPhase);

        if (recovery_phase != sNdsNaturalMovesetPhase)
        {
            ndsFighterNaturalMovesetSetPhase(recovery_phase);
            return FALSE;
        }
        gNdsFighterNaturalCombatStallCount++;
        ndsFighterNaturalMovesetSetPhase(
            ndsFighterNaturalMovesetRetryPhase(sNdsNaturalMovesetPhase));
        return FALSE;
    }
    if (ndsFighterNaturalMovesetPhaseReadyToStart(fp) == FALSE)
    {
        return TRUE;
    }

    switch (sNdsNaturalMovesetPhase)
    {
    case nNDSNaturalMovesetPhaseTiltS3:
        if ((gNdsFighterNaturalMovesetTiltS3Frames > 0u) &&
            (ndsFighterNaturalMovesetStatusIsTiltS3(attacker->status_id) ==
                FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleTiltS3);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltS3:
        if (ndsFighterNaturalMovesetSettled(fp) != FALSE)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseTiltHi3);
        }
        break;
    case nNDSNaturalMovesetPhaseTiltHi3:
        if ((gNdsFighterNaturalMovesetTiltHi3Frames > 0u) &&
            (ndsFighterNaturalMovesetStatusIsTiltHi3(attacker->status_id) ==
                FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleTiltHi3);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltHi3:
        if (ndsFighterNaturalMovesetSettled(fp) != FALSE)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseTiltLw3);
        }
        break;
    case nNDSNaturalMovesetPhaseTiltLw3:
        if ((gNdsFighterNaturalMovesetTiltLw3Frames > 0u) &&
            (ndsFighterNaturalMovesetStatusIsTiltLw3(attacker->status_id) ==
                FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleTiltLw3);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltLw3:
        if (ndsFighterNaturalMovesetSettled(fp) != FALSE)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSmashS4);
        }
        break;
    case nNDSNaturalMovesetPhaseSmashS4:
        if ((gNdsFighterNaturalMovesetSmashFrames > 0u) &&
            (ndsFighterNaturalMovesetStatusIsSmash(attacker->status_id) ==
                FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleSmashS4);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleSmashS4:
        if (ndsFighterNaturalMovesetSettled(fp) != FALSE)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseAerialJump);
        }
        break;
    case nNDSNaturalMovesetPhaseAerialJump:
        if (gNdsFighterNaturalMovesetAerialFrames > 0u)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleAerial);
        }
        else if ((attacker->status_id == nFTCommonStatusJumpF) ||
            (attacker->status_id == nFTCommonStatusJumpB) ||
            (attacker->status_id == nFTCommonStatusJumpAerialF) ||
            (attacker->status_id == nFTCommonStatusJumpAerialB) ||
            (attacker->status_id == nFTCommonStatusFall) ||
            (attacker->status_id == nFTCommonStatusFallAerial))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseAerialAttack);
        }
        break;
    case nNDSNaturalMovesetPhaseAerialAttack:
        if ((gNdsFighterNaturalMovesetAerialFrames > 0u) &&
            (ndsFighterNaturalMovesetStatusIsAerial(attacker->status_id) ==
                FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleAerial);
        }
        else if ((gNdsFighterNaturalMovesetAerialFrames == 0u) &&
                 (gNdsFighterNaturalMovesetLandingFrames > 0u) &&
                  (attacker->ga == nMPKineticsGround) &&
                  (attacker->status_id == nFTCommonStatusWait))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseAerialJump);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleAerial:
        if ((gNdsFighterNaturalMovesetLandingFrames > 0u) &&
            (ndsFighterNaturalMovesetSettled(fp) != FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseGrabCatch);
        }
        break;
    case nNDSNaturalMovesetPhaseGrabCatch:
        if (gNdsFighterNaturalMovesetCatchWaitFrames > 0u)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseGrabThrow);
        }
        break;
    case nNDSNaturalMovesetPhaseGrabThrow:
        if (gNdsFighterNaturalMovesetThrowFrames > 0u)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleThrow);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleThrow:
        if ((gNdsFighterNaturalMovesetThrowRecoverFrames >=
                NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED) ||
            ((gNdsFighterNaturalMovesetThrownFrames > 0u) &&
             (ndsFighterNaturalMovesetSettled(fp) != FALSE)))
        {
            ndsFighterNaturalMovesetUpdateMask();
            sNdsNaturalMovesetDone = 1u;
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseDone);
            return TRUE;
        }
        break;
    default:
        break;
    }
    return FALSE;
}
#endif

#if NDS_P2_SAMUS_STATE_TOUR
static void ndsSamusStateTourRecord(FTStruct *samus)
{
    u32 bit = 0u;

    if (samus == NULL)
    {
        return;
    }
    gNdsSamusStateTourStatus = (u32)samus->status_id;
    gNdsSamusStateTourMotion = (u32)samus->motion_id;
    gNdsSamusStateTourCliffID = (u32)samus->coll_data.cliff_id;
    switch (samus->status_id)
    {
    case nFTCommonStatusFall: bit = 1u << 0; break;
    case nFTCommonStatusCliffCatch: bit = 1u << 1; break;
    case nFTCommonStatusCliffWait: bit = 1u << 2; break;
    case nFTCommonStatusCliffQuick: bit = 1u << 3; break;
    case nFTCommonStatusCliffAttackQuick1: bit = 1u << 4; break;
    case nFTCommonStatusCliffAttackQuick2: bit = 1u << 5; break;
    case nFTCommonStatusCliffEscapeQuick1: bit = 1u << 6; break;
    case nFTCommonStatusCliffEscapeQuick2: bit = 1u << 7; break;
    case nFTCommonStatusCliffClimbQuick1: bit = 1u << 8; break;
    case nFTCommonStatusCliffClimbQuick2: bit = 1u << 9; break;
    case nFTCommonStatusCliffSlow: bit = 1u << 10; break;
    case nFTCommonStatusCliffAttackSlow1: bit = 1u << 11; break;
    case nFTCommonStatusCliffAttackSlow2: bit = 1u << 12; break;
    case nFTCommonStatusCliffEscapeSlow1: bit = 1u << 13; break;
    case nFTCommonStatusCliffEscapeSlow2: bit = 1u << 14; break;
    case nFTCommonStatusCliffClimbSlow1: bit = 1u << 15; break;
    case nFTCommonStatusCliffClimbSlow2: bit = 1u << 16; break;
    default: break;
    }
    gNdsSamusStateTourMask |= bit;
}

static sb32 ndsSamusStateTourExpectedAction2(s32 status_id)
{
    switch (sNdsSamusStateTourScenario)
    {
    case nNDSSamusStateTourQuickAttack:
        return (status_id == nFTCommonStatusCliffAttackQuick2) ? TRUE : FALSE;
    case nNDSSamusStateTourQuickEscape:
        return (status_id == nFTCommonStatusCliffEscapeQuick2) ? TRUE : FALSE;
    case nNDSSamusStateTourQuickClimb:
        return (status_id == nFTCommonStatusCliffClimbQuick2) ? TRUE : FALSE;
    case nNDSSamusStateTourSlowAttack:
        return (status_id == nFTCommonStatusCliffAttackSlow2) ? TRUE : FALSE;
    case nNDSSamusStateTourSlowEscape:
        return (status_id == nFTCommonStatusCliffEscapeSlow2) ? TRUE : FALSE;
    case nNDSSamusStateTourSlowClimb:
        return (status_id == nFTCommonStatusCliffClimbSlow2) ? TRUE : FALSE;
    default:
        return FALSE;
    }
}

static sb32 ndsSamusStateTourPrepareLedge(FTStruct *samus)
{
    DObj *root;
    Vec3f edge;
    u16 flags;

    if ((samus == NULL) || (samus->fkind != nFTKindSamus) ||
        (samus->status_id != nFTCommonStatusWait) ||
        (samus->ga != nMPKineticsGround))
    {
        return FALSE;
    }
    root = samus->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return FALSE;
    }

    /* Dream Land's source main floor is line 3.  Stage only the scenario
     * precondition while Samus is already in source Wait; the ordinary
     * Wait/Walk/Run map callbacks must select Fall before any cliff sweep is
     * staged.  No fighter status or motion is assigned here. */
    sNdsSamusStateTourFloorLine = 3;
    mpCollisionGetFloorEdgeR(sNdsSamusStateTourFloorLine, &edge);
    flags = mpCollisionGetVertexFlagsLineID(sNdsSamusStateTourFloorLine);
    if ((flags & MAP_VERTEX_COLL_CLIFF) == 0u)
    {
        return FALSE;
    }

    samus->percent_damage =
        (sNdsSamusStateTourScenario >= nNDSSamusStateTourSlowAttack) ?
        FTCOMMON_CLIFF_DAMAGE_HIGH : 0;
    samus->lr = +1;
    samus->cliffcatch_wait = 0;
    samus->is_cliff_hold = FALSE;
    samus->physics.vel_ground.x = 0.0F;
    samus->vel_ground.x = 0.0F;
    root->translate.vec.f.x = edge.x - 48.0F;
    root->translate.vec.f.y = edge.y - samus->coll_data.map_coll.bottom;
    root->translate.vec.f.z = 0.0F;
    samus->coll_data.p_translate = &root->translate.vec.f;
    samus->coll_data.p_lr = &samus->lr;
    samus->coll_data.p_map_coll = &samus->coll_data.map_coll;
    samus->coll_data.pos_prev = root->translate.vec.f;
    samus->coll_data.floor_line_id = sNdsSamusStateTourFloorLine;
    samus->coll_data.floor_flags = flags;
    samus->coll_data.mask_curr = MAP_FLAG_FLOOR;
    samus->coll_data.mask_stat = MAP_FLAG_FLOOR;
    samus->coll_data.cliff_id = -1;
    samus->coll_data.ignore_line_id = -1;
    gNdsSamusStateTourStageCount++;
    sNdsSamusStateTourFrames = 0u;
    sNdsSamusStateTourStep = nNDSSamusStateTourStepRunOff;
    return TRUE;
}

static sb32 ndsSamusStateTourStageCliffSweep(FTStruct *samus)
{
    DObj *root;
    Vec3f edge;
    Vec2f cliffcatch;
    f32 sample_x;

    if ((samus == NULL) || (samus->status_id != nFTCommonStatusFall) ||
        (samus->ga != nMPKineticsAir))
    {
        return FALSE;
    }
    root = samus->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return FALSE;
    }
    mpCollisionGetFloorEdgeR(sNdsSamusStateTourFloorLine, &edge);
    cliffcatch = samus->coll_data.cliffcatch_coll;
    if (cliffcatch.x <= 0.0F)
    {
        cliffcatch.x = 64.0F;
    }
    if (cliffcatch.y == 0.0F)
    {
        cliffcatch.y = 64.0F;
    }
    sample_x = edge.x - 100.0F;

    /* Source R-cliff test uses (root - cliffcatch) and requires lr == -1.
     * These guest writes establish a descending sweep after source Fall has
     * already been selected; BattleShip mpprocess still decides whether it is
     * a cliff and ftCommonCliffCatchSetStatus still owns the transition. */
    samus->lr = -1;
    samus->cliffcatch_wait = 0;
    samus->is_cliff_hold = FALSE;
    samus->coll_data.cliffcatch_coll = cliffcatch;
    samus->coll_data.p_translate = &root->translate.vec.f;
    samus->coll_data.p_lr = &samus->lr;
    samus->coll_data.pos_prev.x = sample_x + cliffcatch.x;
    samus->coll_data.pos_prev.y = edge.y - cliffcatch.y + 2.0F;
    samus->coll_data.pos_prev.z = 0.0F;
    root->translate.vec.f.x = sample_x + cliffcatch.x;
    /* mpCommonUpdateFighterCollisionData snapshots the current translate into
     * pos_prev at the beginning of the next source map update.  Keep the live
     * grab point ABOVE the floor here; ordinary Fall physics then advances it
     * below the floor in that update, producing the source-required descending
     * (+ -> -) cliff sweep.  Staging it below the floor made the real call
     * observe -2 -> -7.9 and correctly reject the cliff. */
    root->translate.vec.f.y = edge.y - cliffcatch.y + 2.0F;
    root->translate.vec.f.z = 0.0F;
    samus->physics.vel_air.x = 0.0F;
    samus->physics.vel_air.y = -4.0F;
    samus->physics.vel_air.z = 0.0F;
    samus->vel_air = samus->physics.vel_air;
    samus->coll_data.update_tic = gMPCollisionUpdateTic;
    samus->coll_data.mask_curr = 0u;
    samus->coll_data.mask_stat = 0u;
    samus->coll_data.floor_line_id = -1;
    samus->coll_data.cliff_id = -1;
    samus->coll_data.ignore_line_id = -1;
    gNdsSamusStateTourStageCount++;
    sNdsSamusStateTourFrames = 0u;
    sNdsSamusStateTourStep = nNDSSamusStateTourStepAwaitCliff;
    return TRUE;
}

static sb32 ndsSamusStateTourAdvance(FTStruct *fp[2])
{
    FTStruct *samus = fp[0];

    sNdsSamusStateTourActive = 1u;
    if (sNdsSamusStateTourDone != 0u)
    {
        return TRUE;
    }
    if ((samus == NULL) || (samus->fkind != nFTKindSamus))
    {
        return FALSE;
    }
    ndsSamusStateTourRecord(samus);
    gNdsSamusStateTourPhase = sNdsSamusStateTourScenario;
    gNdsSamusStateTourPhaseFrames = ++sNdsSamusStateTourFrames;
    if (sNdsSamusStateTourFrames > NDS_SAMUS_STATE_TOUR_TIMEOUT)
    {
        gNdsFighterNaturalCombatStallCount++;
        return FALSE;
    }

    switch (sNdsSamusStateTourStep)
    {
    case nNDSSamusStateTourStepPrepare:
        (void)ndsSamusStateTourPrepareLedge(samus);
        break;
    case nNDSSamusStateTourStepRunOff:
        if (samus->status_id == nFTCommonStatusFall)
        {
            (void)ndsSamusStateTourStageCliffSweep(samus);
        }
        break;
    case nNDSSamusStateTourStepAwaitCliff:
        if (samus->status_id == nFTCommonStatusCliffWait)
        {
            sNdsSamusStateTourFrames = 0u;
            sNdsSamusStateTourStep = nNDSSamusStateTourStepCliffWait;
        }
        break;
    case nNDSSamusStateTourStepCliffWait:
        if (ndsSamusStateTourExpectedAction2(samus->status_id) != FALSE)
        {
            sNdsSamusStateTourActionSeen = 1u;
            sNdsSamusStateTourFrames = 0u;
            sNdsSamusStateTourStep = nNDSSamusStateTourStepRecover;
        }
        break;
    case nNDSSamusStateTourStepRecover:
        if ((sNdsSamusStateTourActionSeen != 0u) &&
            (samus->status_id == nFTCommonStatusWait) &&
            (samus->ga == nMPKineticsGround))
        {
            sNdsSamusStateTourScenario++;
            sNdsSamusStateTourActionSeen = 0u;
            sNdsSamusStateTourFrames = 0u;
            if (sNdsSamusStateTourScenario >= nNDSSamusStateTourDone)
            {
                sNdsSamusStateTourDone = 1u;
                gNdsSamusStateTourPhase = nNDSSamusStateTourDone;
                return TRUE;
            }
            sNdsSamusStateTourStep = nNDSSamusStateTourStepPrepare;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

static sb32 ndsSamusStateTourApplyInput(FTStruct *fp[2], u16 button[2],
                                        s8 stick_x[2], s8 stick_y[2])
{
    FTStruct *samus = fp[0];

    if ((sNdsSamusStateTourActive == 0u) ||
        (sNdsSamusStateTourDone != 0u) || (samus == NULL) ||
        (samus->fkind != nFTKindSamus))
    {
        return FALSE;
    }
    if (sNdsSamusStateTourStep == nNDSSamusStateTourStepRunOff)
    {
        stick_x[0] = 80;
    }
    else if ((sNdsSamusStateTourStep == nNDSSamusStateTourStepCliffWait) &&
             (samus->status_id == nFTCommonStatusCliffWait) &&
             (samus->status_vars.common.cliffwait.is_allow_interrupt != FALSE))
    {
        switch (sNdsSamusStateTourScenario)
        {
        case nNDSSamusStateTourQuickAttack:
        case nNDSSamusStateTourSlowAttack:
            button[0] = A_BUTTON;
            break;
        case nNDSSamusStateTourQuickEscape:
        case nNDSSamusStateTourSlowEscape:
            button[0] = Z_TRIG;
            break;
        case nNDSSamusStateTourQuickClimb:
        case nNDSSamusStateTourSlowClimb:
            stick_y[0] = 80;
            break;
        default:
            break;
        }
    }
    return TRUE;
}
#endif

#if NDS_P2_SAMUS_TUMBLE_TOUR
static void ndsSamusTumbleTourRecord(FTStruct *samus)
{
    u32 bit = 0u;

    if (samus == NULL)
    {
        return;
    }
    gNdsSamusTumbleTourStatus = (u32)samus->status_id;
    gNdsSamusTumbleTourMotion = (u32)samus->motion_id;
    if ((samus->status_id >= nFTCommonStatusDamageFlyHi) &&
        (samus->status_id <= nFTCommonStatusDamageFlyRoll))
    {
        bit = 1u << 0;
        gNdsSamusTumbleTourDamageFlyMask |=
            1u << (samus->status_id - nFTCommonStatusDamageFlyHi);
    }
    else
    {
        switch (samus->status_id)
        {
        case nFTCommonStatusDamageFall: bit = 1u << 1; break;
        case nFTCommonStatusPassive: bit = 1u << 2; break;
        case nFTCommonStatusPassiveStandF: bit = 1u << 3; break;
        case nFTCommonStatusPassiveStandB: bit = 1u << 4; break;
        case nFTCommonStatusDownBounceD: bit = 1u << 5; break;
        case nFTCommonStatusDownBounceU: bit = 1u << 6; break;
        case nFTCommonStatusDownWaitD: bit = 1u << 7; break;
        case nFTCommonStatusDownWaitU: bit = 1u << 8; break;
        case nFTCommonStatusDownStandD: bit = 1u << 9; break;
        case nFTCommonStatusDownStandU: bit = 1u << 10; break;
        case nFTCommonStatusDownForwardD: bit = 1u << 11; break;
        case nFTCommonStatusDownForwardU: bit = 1u << 12; break;
        case nFTCommonStatusDownBackD: bit = 1u << 13; break;
        case nFTCommonStatusDownBackU: bit = 1u << 14; break;
        case nFTCommonStatusDownAttackD: bit = 1u << 15; break;
        case nFTCommonStatusDownAttackU: bit = 1u << 16; break;
        default: break;
        }
    }
    gNdsSamusTumbleTourMask |= bit;
}

static sb32 ndsSamusTumbleTourScenarioDownD(void)
{
    switch (sNdsSamusTumbleTourScenario)
    {
    case nNDSSamusTumbleTourDownStandD:
    case nNDSSamusTumbleTourDownForwardD:
    case nNDSSamusTumbleTourDownBackD:
    case nNDSSamusTumbleTourDownAttackD:
        return TRUE;
    default:
        return FALSE;
    }
}

static sb32 ndsSamusTumbleTourScenarioIsDown(void)
{
    return (sNdsSamusTumbleTourScenario >= nNDSSamusTumbleTourDownStandD) ?
        TRUE : FALSE;
}

static s32 ndsSamusTumbleTourExpectedStatus(void)
{
    switch (sNdsSamusTumbleTourScenario)
    {
    case nNDSSamusTumbleTourPassive: return nFTCommonStatusPassive;
    case nNDSSamusTumbleTourPassiveStandF: return nFTCommonStatusPassiveStandF;
    case nNDSSamusTumbleTourPassiveStandB: return nFTCommonStatusPassiveStandB;
    case nNDSSamusTumbleTourDownStandD: return nFTCommonStatusDownStandD;
    case nNDSSamusTumbleTourDownStandU: return nFTCommonStatusDownStandU;
    case nNDSSamusTumbleTourDownForwardD: return nFTCommonStatusDownForwardD;
    case nNDSSamusTumbleTourDownForwardU: return nFTCommonStatusDownForwardU;
    case nNDSSamusTumbleTourDownBackD: return nFTCommonStatusDownBackD;
    case nNDSSamusTumbleTourDownBackU: return nFTCommonStatusDownBackU;
    case nNDSSamusTumbleTourDownAttackD: return nFTCommonStatusDownAttackD;
    case nNDSSamusTumbleTourDownAttackU: return nFTCommonStatusDownAttackU;
    default: return -1;
    }
}

static void ndsSamusTumbleTourPlaceGround(FTStruct *fp, f32 x, f32 floor_y,
                                          u16 floor_flags)
{
    DObj *root = fp->joints[nFTPartsJointTopN];

    root->translate.vec.f.x = x;
    root->translate.vec.f.y = floor_y - fp->coll_data.map_coll.bottom;
    root->translate.vec.f.z = 0.0F;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.pos_prev = root->translate.vec.f;
    fp->coll_data.floor_line_id = sNdsSamusTumbleTourFloorLine;
    fp->coll_data.floor_flags = floor_flags;
    fp->coll_data.mask_curr = MAP_FLAG_FLOOR;
    fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
    fp->coll_data.cliff_id = -1;
    fp->coll_data.ignore_line_id = -1;
    fp->physics.vel_ground.x = 0.0F;
    fp->vel_ground.x = 0.0F;
}

static sb32 ndsSamusTumbleTourPrepareHit(FTStruct *fp[2])
{
    FTStruct *samus = fp[0];
    FTStruct *fox = fp[1];
    Vec3f edge;
    u16 flags;

    if ((samus == NULL) || (fox == NULL) ||
        (samus->fkind != nFTKindSamus) || (fox->fkind != nFTKindFox) ||
        (samus->status_id != nFTCommonStatusWait) ||
        (fox->status_id != nFTCommonStatusWait) ||
        (samus->ga != nMPKineticsGround) || (fox->ga != nMPKineticsGround))
    {
        return FALSE;
    }
    if ((samus->joints[nFTPartsJointTopN] == NULL) ||
        (fox->joints[nFTPartsJointTopN] == NULL))
    {
        return FALSE;
    }

    sNdsSamusTumbleTourFloorLine = 3;
    mpCollisionGetFloorEdgeR(sNdsSamusTumbleTourFloorLine, &edge);
    flags = mpCollisionGetVertexFlagsLineID(sNdsSamusTumbleTourFloorLine);
    if ((flags & MAP_VERTEX_COLL_CLIFF) == 0u)
    {
        return FALSE;
    }

    /* Scenario precondition only.  Fox's ordinary Up+A input must select
     * AttackHi4 and the real attack collision must put Samus into DamageFly.
     * No damage/status routine is called from this proof driver. */
    fox->lr = +1;
    samus->lr = -1;
    samus->percent_damage = 80;
    /* Preserve the controller-populated Z history for the three tech
     * scenarios.  The down-state scenarios intentionally arrive without a
     * tech buffer, so clear only those before BattleShip's DamageFall map
     * callback chooses Passive/PassiveStand versus DownBounce. */
    if (ndsSamusTumbleTourScenarioIsDown() != FALSE)
    {
        samus->tics_since_last_z = FTINPUT_ZTRIGLAST_TICS_MAX;
    }
    ndsSamusTumbleTourPlaceGround(fox, -20.0F, edge.y, flags);
    ndsSamusTumbleTourPlaceGround(samus, +20.0F, edge.y, flags);
    sNdsSamusTumbleTourFrames = 0u;
    sNdsSamusTumbleTourAttackSeen = 0u;
    sNdsSamusTumbleTourActionSeen = 0u;
    sNdsSamusTumbleTourStep = nNDSSamusTumbleTourStepAttack;
    gNdsSamusTumbleTourStageCount++;
    return TRUE;
}

static sb32 ndsSamusTumbleTourStageLanding(FTStruct *samus)
{
    DObj *root;
    Vec3f edge;

    if ((samus == NULL) || (samus->status_id != nFTCommonStatusDamageFall) ||
        (samus->ga != nMPKineticsAir))
    {
        return FALSE;
    }
    root = samus->joints[nFTPartsJointTopN];
    if ((root == NULL) || (samus->joints[4] == NULL))
    {
        return FALSE;
    }
    mpCollisionGetFloorEdgeR(sNdsSamusTumbleTourFloorLine, &edge);

    root->translate.vec.f.x = 0.0F;
    root->translate.vec.f.y = edge.y - samus->coll_data.map_coll.bottom + 2.0F;
    root->translate.vec.f.z = 0.0F;
    samus->coll_data.p_translate = &root->translate.vec.f;
    samus->coll_data.p_lr = &samus->lr;
    samus->coll_data.pos_prev = root->translate.vec.f;
    samus->coll_data.floor_line_id = -1;
    samus->coll_data.mask_curr = 0u;
    samus->coll_data.mask_stat = 0u;
    samus->coll_data.cliff_id = -1;
    samus->coll_data.ignore_line_id = -1;
    samus->coll_data.update_tic = gMPCollisionUpdateTic;
    samus->physics.vel_air.x = 0.0F;
    samus->physics.vel_air.y = -4.0F;
    samus->physics.vel_air.z = 0.0F;
    samus->vel_air = samus->physics.vel_air;
    samus->physics.vel_damage_air.x = 0.0F;
    samus->physics.vel_damage_air.y = 0.0F;
    samus->physics.vel_damage_ground = 0.0F;
    if (ndsSamusTumbleTourScenarioIsDown() != FALSE)
    {
        samus->tics_since_last_z = FTINPUT_ZTRIGLAST_TICS_MAX;
    }
    if (ndsSamusTumbleTourScenarioIsDown() != FALSE)
    {
        samus->joints[4]->rotate.vec.f.x =
            (ndsSamusTumbleTourScenarioDownD() != FALSE) ?
            F_CST_DTOR32(90.0F) : 0.0F;
    }
    sNdsSamusTumbleTourFrames = 0u;
    sNdsSamusTumbleTourStep = nNDSSamusTumbleTourStepLanding;
    gNdsSamusTumbleTourStageCount++;
    return TRUE;
}

static void ndsSamusTumbleTourPrepareDamageFallMap(FTStruct *samus)
{
    if ((sNdsSamusTumbleTourActive == 0u) ||
        (gNdsSamusTumbleTourDone != 0u) ||
        (samus == NULL) || (samus->fkind != nFTKindSamus) ||
        (samus->status_id != nFTCommonStatusDamageFall) ||
        (sNdsSamusTumbleTourStep != nNDSSamusTumbleTourStepLanding) ||
        (ndsSamusTumbleTourScenarioIsDown() == FALSE) ||
        (samus->joints[4] == NULL))
    {
        return;
    }

    /* DamageFall pose evaluation runs before ProcMap and can replace the
     * scenario's staged joint-4 rotation.  Re-apply only that pose
     * precondition at the final guest-side seam immediately before the
     * BattleShip map callback.  ftCommonDownBounceCheckUpOrDown still owns
     * the U/D branch and ftCommonDownBounceSetStatus still owns the status. */
    samus->joints[4]->rotate.vec.f.x =
        (ndsSamusTumbleTourScenarioDownD() != FALSE) ?
        F_CST_DTOR32(90.0F) : 0.0F;
}

static sb32 ndsSamusTumbleTourAdvance(FTStruct *fp[2])
{
    FTStruct *samus = fp[0];
    FTStruct *fox = fp[1];
    s32 expected;

    sNdsSamusTumbleTourActive = 1u;
    if (gNdsSamusTumbleTourDone != 0u)
    {
        return TRUE;
    }
    if ((samus == NULL) || (fox == NULL) ||
        (samus->fkind != nFTKindSamus) || (fox->fkind != nFTKindFox))
    {
        return FALSE;
    }
    ndsSamusTumbleTourRecord(samus);
    gNdsSamusTumbleTourScenario = sNdsSamusTumbleTourScenario;
    gNdsSamusTumbleTourStep = sNdsSamusTumbleTourStep;
    gNdsSamusTumbleTourFrames = ++sNdsSamusTumbleTourFrames;
    if (sNdsSamusTumbleTourFrames > NDS_SAMUS_TUMBLE_TOUR_TIMEOUT)
    {
        gNdsFighterNaturalCombatStallCount++;
        return FALSE;
    }

    switch (sNdsSamusTumbleTourStep)
    {
    case nNDSSamusTumbleTourStepPrepareHit:
        (void)ndsSamusTumbleTourPrepareHit(fp);
        break;
    case nNDSSamusTumbleTourStepAttack:
        if (fox->status_id == nFTCommonStatusAttackHi4)
        {
            sNdsSamusTumbleTourAttackSeen = 1u;
        }
        if ((samus->status_id >= nFTCommonStatusDamageFlyHi) &&
            (samus->status_id <= nFTCommonStatusDamageFlyRoll))
        {
            gNdsSamusTumbleTourHitCount++;
            sNdsSamusTumbleTourFrames = 0u;
            sNdsSamusTumbleTourStep = nNDSSamusTumbleTourStepDamageFly;
        }
        else if ((sNdsSamusTumbleTourAttackSeen != 0u) &&
                 (fox->status_id == nFTCommonStatusWait) &&
                 (samus->status_id == nFTCommonStatusWait))
        {
            sNdsSamusTumbleTourFrames = 0u;
            sNdsSamusTumbleTourStep = nNDSSamusTumbleTourStepPrepareHit;
        }
        break;
    case nNDSSamusTumbleTourStepDamageFly:
        if (samus->status_id == nFTCommonStatusDamageFall)
        {
            /* Tech input must exist in BattleShip's Z history BEFORE the map
             * callback observes the floor.  Give the controller path one full
             * DamageFall update to populate tics_since_last_z, then stage the
             * descending floor crossing on the following update. */
            sNdsSamusTumbleTourFrames = 0u;
            sNdsSamusTumbleTourStep = nNDSSamusTumbleTourStepDamageFall;
        }
        break;
    case nNDSSamusTumbleTourStepDamageFall:
        if (sNdsSamusTumbleTourFrames >= 1u)
        {
            (void)ndsSamusTumbleTourStageLanding(samus);
        }
        break;
    case nNDSSamusTumbleTourStepLanding:
        expected = ndsSamusTumbleTourExpectedStatus();
        if (samus->status_id == expected)
        {
            sNdsSamusTumbleTourActionSeen = 1u;
            sNdsSamusTumbleTourFrames = 0u;
            sNdsSamusTumbleTourStep = nNDSSamusTumbleTourStepRecover;
        }
        else if ((ndsSamusTumbleTourScenarioIsDown() != FALSE) &&
                 ((samus->status_id == nFTCommonStatusDownBounceD) ||
                  (samus->status_id == nFTCommonStatusDownBounceU)))
        {
            sNdsSamusTumbleTourDownWaitObserved = 0u;
            sNdsSamusTumbleTourFrames = 0u;
            sNdsSamusTumbleTourStep = nNDSSamusTumbleTourStepDownWait;
        }
        break;
    case nNDSSamusTumbleTourStepDownWait:
        expected = ndsSamusTumbleTourExpectedStatus();
        if ((samus->status_id == nFTCommonStatusDownWaitD) ||
            (samus->status_id == nFTCommonStatusDownWaitU))
        {
            sNdsSamusTumbleTourDownWaitObserved = 1u;
        }
        if (samus->status_id == expected)
        {
            sNdsSamusTumbleTourActionSeen = 1u;
            sNdsSamusTumbleTourFrames = 0u;
            sNdsSamusTumbleTourStep = nNDSSamusTumbleTourStepRecover;
        }
        break;
    case nNDSSamusTumbleTourStepRecover:
        if ((sNdsSamusTumbleTourActionSeen != 0u) &&
            (samus->status_id == nFTCommonStatusWait) &&
            (fox->status_id == nFTCommonStatusWait) &&
            (samus->ga == nMPKineticsGround) && (fox->ga == nMPKineticsGround))
        {
            sNdsSamusTumbleTourScenario++;
            sNdsSamusTumbleTourFrames = 0u;
            sNdsSamusTumbleTourAttackSeen = 0u;
            sNdsSamusTumbleTourActionSeen = 0u;
            sNdsSamusTumbleTourDownWaitObserved = 0u;
            if (sNdsSamusTumbleTourScenario >= nNDSSamusTumbleTourDone)
            {
                gNdsSamusTumbleTourScenario = nNDSSamusTumbleTourDone;
                gNdsSamusTumbleTourDone = 1u;
                return TRUE;
            }
            sNdsSamusTumbleTourStep = nNDSSamusTumbleTourStepPrepareHit;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

static sb32 ndsSamusTumbleTourApplyInput(FTStruct *fp[2], u16 button[2],
                                         s8 stick_x[2], s8 stick_y[2])
{
    FTStruct *samus = fp[0];
    FTStruct *fox = fp[1];
    s8 forward;

    if ((sNdsSamusTumbleTourActive == 0u) ||
        (gNdsSamusTumbleTourDone != 0u) ||
        (samus == NULL) || (fox == NULL) ||
        (samus->fkind != nFTKindSamus) || (fox->fkind != nFTKindFox))
    {
        return FALSE;
    }
    if (sNdsSamusTumbleTourStep == nNDSSamusTumbleTourStepAttack)
    {
        if ((sNdsSamusTumbleTourFrames <= 3u) &&
            (fox->status_id == nFTCommonStatusWait))
        {
            button[1] = A_BUTTON;
            stick_y[1] = 80;
        }
    }
    else if (sNdsSamusTumbleTourStep == nNDSSamusTumbleTourStepDamageFall)
    {
        if ((sNdsSamusTumbleTourScenario == nNDSSamusTumbleTourPassive) ||
            (sNdsSamusTumbleTourScenario == nNDSSamusTumbleTourPassiveStandF) ||
            (sNdsSamusTumbleTourScenario == nNDSSamusTumbleTourPassiveStandB))
        {
            button[0] = Z_TRIG;
            forward = (samus->lr >= 0) ? 80 : -80;
            if (sNdsSamusTumbleTourScenario ==
                nNDSSamusTumbleTourPassiveStandF)
            {
                stick_x[0] = forward;
            }
            else if (sNdsSamusTumbleTourScenario ==
                     nNDSSamusTumbleTourPassiveStandB)
            {
                stick_x[0] = -forward;
            }
        }
    }
    else if (sNdsSamusTumbleTourStep == nNDSSamusTumbleTourStepLanding)
    {
        forward = (samus->lr >= 0.0F) ? 80 : -80;
        switch (sNdsSamusTumbleTourScenario)
        {
        case nNDSSamusTumbleTourPassive:
            break;
        case nNDSSamusTumbleTourPassiveStandF:
            stick_x[0] = forward;
            break;
        case nNDSSamusTumbleTourPassiveStandB:
            stick_x[0] = -forward;
            break;
        default:
            break;
        }
    }
    else if ((sNdsSamusTumbleTourStep == nNDSSamusTumbleTourStepDownWait) &&
             (sNdsSamusTumbleTourDownWaitObserved != 0u) &&
             ((samus->status_id == nFTCommonStatusDownWaitD) ||
              (samus->status_id == nFTCommonStatusDownWaitU)))
    {
        forward = (samus->lr >= 0.0F) ? 80 : -80;
        switch (sNdsSamusTumbleTourScenario)
        {
        case nNDSSamusTumbleTourDownStandD:
        case nNDSSamusTumbleTourDownStandU:
            button[0] = Z_TRIG;
            break;
        case nNDSSamusTumbleTourDownForwardD:
        case nNDSSamusTumbleTourDownForwardU:
            stick_x[0] = forward;
            break;
        case nNDSSamusTumbleTourDownBackD:
        case nNDSSamusTumbleTourDownBackU:
            stick_x[0] = -forward;
            break;
        case nNDSSamusTumbleTourDownAttackD:
        case nNDSSamusTumbleTourDownAttackU:
            button[0] = A_BUTTON;
            break;
        default:
            break;
        }
    }
    return TRUE;
}
#endif

#if NDS_P2_SAMUS_DAMAGEFLY_TOUR
extern void osWritebackDCacheAll(void);

__attribute__((noinline, used))
void ndsSamusDamageFlyTourProofStop(void)
{
    __asm__ volatile ("" ::: "memory");
}

static void ndsSamusDamageFlyTourProofTerminal(void)
{
    gNdsSamusDamageFlyTourTerminalCount++;
    osWritebackDCacheAll();
    ndsSamusDamageFlyTourProofStop();
}

static s32 ndsSamusDamageFlyTourExpectedStatus(void)
{
    switch (sNdsSamusDamageFlyTourScenario)
    {
    case nNDSSamusDamageFlyTourHi: return nFTCommonStatusDamageFlyHi;
    case nNDSSamusDamageFlyTourN: return nFTCommonStatusDamageFlyN;
    case nNDSSamusDamageFlyTourLw: return nFTCommonStatusDamageFlyLw;
    case nNDSSamusDamageFlyTourTop: return nFTCommonStatusDamageFlyTop;
    case nNDSSamusDamageFlyTourRoll: return nFTCommonStatusDamageFlyRoll;
    default: return -1;
    }
}

static s32 ndsSamusDamageFlyTourExpectedPlacement(void)
{
    switch (sNdsSamusDamageFlyTourScenario)
    {
    /* `dFTCommonDamageStatus*IDs[3][damage_index]` is Lw/N/Hi for source
     * hurtbox placements 0/1/2 respectively. */
    case nNDSSamusDamageFlyTourHi: return 2;
    case nNDSSamusDamageFlyTourN: return 1;
    case nNDSSamusDamageFlyTourLw: return 0;
    default: return -1;
    }
}

static void ndsSamusDamageFlyTourPlaceGround(FTStruct *fp, f32 x,
                                              f32 floor_y, u16 floor_flags)
{
    DObj *root = fp->joints[nFTPartsJointTopN];

    root->translate.vec.f.x = x;
    root->translate.vec.f.y = floor_y - fp->coll_data.map_coll.bottom;
    root->translate.vec.f.z = 0.0F;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.pos_prev = root->translate.vec.f;
    fp->coll_data.floor_line_id = sNdsSamusDamageFlyTourFloorLine;
    fp->coll_data.floor_flags = floor_flags;
    fp->coll_data.mask_curr = MAP_FLAG_FLOOR;
    fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
    fp->coll_data.cliff_id = -1;
    fp->coll_data.ignore_line_id = -1;
    fp->physics.vel_ground.x = 0.0F;
    fp->vel_ground.x = 0.0F;
}

static sb32 ndsSamusDamageFlyTourPrepareHit(FTStruct *fp[2])
{
    FTStruct *samus = fp[0];
    FTStruct *fox = fp[1];
    Vec3f edge;
    u16 flags;

    if ((samus == NULL) || (fox == NULL) ||
        (samus->fkind != nFTKindSamus) || (fox->fkind != nFTKindFox) ||
        (samus->status_id != nFTCommonStatusWait) ||
        (fox->status_id != nFTCommonStatusWait) ||
        (samus->ga != nMPKineticsGround) || (fox->ga != nMPKineticsGround) ||
        (samus->joints[nFTPartsJointTopN] == NULL) ||
        (fox->joints[nFTPartsJointTopN] == NULL))
    {
        return FALSE;
    }
    sNdsSamusDamageFlyTourFloorLine = 3;
    mpCollisionGetFloorEdgeR(sNdsSamusDamageFlyTourFloorLine, &edge);
    flags = mpCollisionGetVertexFlagsLineID(sNdsSamusDamageFlyTourFloorLine);
    if ((flags & MAP_VERTEX_COLL_CLIFF) == 0u)
    {
        return FALSE;
    }

    /* Proof-only preconditions. The hit itself is an ordinary controller-fed
     * Fox attack. At 80%, all three 361-degree forward-tilt variants clear the
     * source level-3 hitstun threshold while remaining below FlyRoll's 100%
     * gate. The Roll scenario alone starts at 100% and retries real hits until
     * BattleShip's own 0.5 RNG branch selects it. */
    fox->lr = +1;
    samus->lr = -1;
    samus->percent_damage =
        (sNdsSamusDamageFlyTourScenario == nNDSSamusDamageFlyTourRoll) ?
        FTCOMMON_DAMAGE_FIGHTER_FLYROLL_DAMAGE_MIN : 80;
    ndsSamusDamageFlyTourPlaceGround(fox, -20.0F, edge.y, flags);
    ndsSamusDamageFlyTourPlaceGround(samus, +20.0F, edge.y, flags);
    sNdsSamusDamageFlyTourFrames = 0u;
    sNdsSamusDamageFlyTourAttackPressed = 0u;
    sNdsSamusDamageFlyTourHitRecorded = 0u;
    sNdsSamusDamageFlyTourScenarioAccepted = 0u;
    sNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepRearm;
    gNdsSamusDamageFlyTourStageCount++;
    return TRUE;
}

static sb32 ndsSamusDamageFlyTourStageLanding(FTStruct *samus)
{
    DObj *root;
    Vec3f edge;

    if ((samus == NULL) || (samus->status_id != nFTCommonStatusDamageFall) ||
        (samus->ga != nMPKineticsAir))
    {
        return FALSE;
    }
    root = samus->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return FALSE;
    }
    mpCollisionGetFloorEdgeR(sNdsSamusDamageFlyTourFloorLine, &edge);
    root->translate.vec.f.x = 0.0F;
    root->translate.vec.f.y = edge.y - samus->coll_data.map_coll.bottom + 2.0F;
    root->translate.vec.f.z = 0.0F;
    samus->coll_data.p_translate = &root->translate.vec.f;
    samus->coll_data.p_lr = &samus->lr;
    samus->coll_data.pos_prev = root->translate.vec.f;
    samus->coll_data.floor_line_id = -1;
    samus->coll_data.mask_curr = 0u;
    samus->coll_data.mask_stat = 0u;
    samus->coll_data.cliff_id = -1;
    samus->coll_data.ignore_line_id = -1;
    samus->coll_data.update_tic = gMPCollisionUpdateTic;
    samus->physics.vel_air.x = 0.0F;
    samus->physics.vel_air.y = -4.0F;
    samus->physics.vel_air.z = 0.0F;
    samus->vel_air = samus->physics.vel_air;
    samus->physics.vel_damage_air.x = 0.0F;
    samus->physics.vel_damage_air.y = 0.0F;
    samus->physics.vel_damage_ground = 0.0F;
    sNdsSamusDamageFlyTourFrames = 0u;
    sNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepLanding;
    gNdsSamusDamageFlyTourStageCount++;
    return TRUE;
}

static void ndsSamusDamageFlyTourRecord(FTStruct *samus, FTStruct *fox)
{
    u32 shift;
    s32 expected_status;
    s32 expected_placement;

    if ((samus == NULL) || (fox == NULL))
    {
        return;
    }
    gNdsSamusDamageFlyTourStatus = (u32)samus->status_id;
    gNdsSamusDamageFlyTourMotion = (u32)samus->motion_id;
    switch (fox->status_id)
    {
    case nFTCommonStatusAttackAirN:
        gNdsSamusDamageFlyTourAttackerMask |= 1u << 0;
        break;
    case nFTCommonStatusAttackS3:
        gNdsSamusDamageFlyTourAttackerMask |= 1u << 1;
        break;
    case nFTCommonStatusAttackLw4:
        gNdsSamusDamageFlyTourAttackerMask |= 1u << 2;
        break;
    case nFTCommonStatusAttackHi4:
        gNdsSamusDamageFlyTourAttackerMask |= 1u << 3;
        break;
    default:
        break;
    }
    if ((sNdsSamusDamageFlyTourHitRecorded != 0u) ||
        (samus->status_id < nFTCommonStatusDamageFlyHi) ||
        (samus->status_id > nFTCommonStatusDamageFlyRoll))
    {
        return;
    }
    sNdsSamusDamageFlyTourHitRecorded = 1u;
    gNdsSamusDamageFlyTourHitCount++;
    gNdsSamusDamageFlyTourMask |=
        1u << (samus->status_id - nFTCommonStatusDamageFlyHi);
    shift = sNdsSamusDamageFlyTourScenario * 3u;
    gNdsSamusDamageFlyTourPlacementPacked &= ~(7u << shift);
    gNdsSamusDamageFlyTourPlacementPacked |=
        ((u32)samus->damage_index & 7u) << shift;
    if (samus->damage_angle == 361)
    {
        gNdsSamusDamageFlyTourSakuraiHitCount++;
    }
    if (samus->damage_angle == 80)
    {
        gNdsSamusDamageFlyTourTopAngle80Count++;
    }
    if (sNdsSamusDamageFlyTourScenario == nNDSSamusDamageFlyTourRoll)
    {
        gNdsSamusDamageFlyTourRollAttempts++;
        gNdsSamusDamageFlyTourRollPercent = (u32)samus->percent_damage;
    }
    expected_status = ndsSamusDamageFlyTourExpectedStatus();
    expected_placement = ndsSamusDamageFlyTourExpectedPlacement();
    if ((samus->status_id == expected_status) &&
        ((expected_placement < 0) || (samus->damage_index == expected_placement)))
    {
        sNdsSamusDamageFlyTourScenarioAccepted = 1u;
    }
    else if (sNdsSamusDamageFlyTourScenario != nNDSSamusDamageFlyTourRoll)
    {
        gNdsSamusDamageFlyTourMismatchCount++;
    }
}

static sb32 ndsSamusDamageFlyTourAdvance(FTStruct *fp[2])
{
    FTStruct *samus = fp[0];
    FTStruct *fox = fp[1];

    sNdsSamusDamageFlyTourActive = 1u;
    if (gNdsSamusDamageFlyTourDone != 0u)
    {
        return TRUE;
    }
    if ((samus == NULL) || (fox == NULL) ||
        (samus->fkind != nFTKindSamus) || (fox->fkind != nFTKindFox))
    {
        return FALSE;
    }
    ndsSamusDamageFlyTourRecord(samus, fox);
    gNdsSamusDamageFlyTourScenario = sNdsSamusDamageFlyTourScenario;
    gNdsSamusDamageFlyTourStep = sNdsSamusDamageFlyTourStep;
    gNdsSamusDamageFlyTourFrames = ++sNdsSamusDamageFlyTourFrames;
    if ((sNdsSamusDamageFlyTourFrames > NDS_SAMUS_DAMAGEFLY_TOUR_TIMEOUT) ||
        (gNdsSamusDamageFlyTourRollAttempts >
             NDS_SAMUS_DAMAGEFLY_TOUR_ROLL_ATTEMPTS_MAX) ||
        (gNdsSamusDamageFlyTourMismatchCount >
             NDS_SAMUS_DAMAGEFLY_TOUR_MISMATCH_MAX))
    {
        gNdsFighterNaturalCombatStallCount++;
        gNdsSamusDamageFlyTourDone = 2u;
        ndsSamusDamageFlyTourProofTerminal();
        return TRUE;
    }

    switch (sNdsSamusDamageFlyTourStep)
    {
    case nNDSSamusDamageFlyTourStepPrepare:
        (void)ndsSamusDamageFlyTourPrepareHit(fp);
        break;
    case nNDSSamusDamageFlyTourStepRearm:
        if (sNdsSamusDamageFlyTourFrames >= 1u)
        {
            sNdsSamusDamageFlyTourFrames = 0u;
            sNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepDrive;
        }
        break;
    case nNDSSamusDamageFlyTourStepDrive:
        if (sNdsSamusDamageFlyTourHitRecorded != 0u)
        {
            sNdsSamusDamageFlyTourFrames = 0u;
            sNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepDamageFly;
        }
        else if ((sNdsSamusDamageFlyTourAttackPressed != 0u) &&
                 (fox->status_id == nFTCommonStatusWait) &&
                 (samus->status_id == nFTCommonStatusWait))
        {
            /* A clean whiff is not a claimed hit. Re-stage and let the source
             * attack/collision path try again without manufacturing damage. */
            sNdsSamusDamageFlyTourFrames = 0u;
            sNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepPrepare;
        }
        break;
    case nNDSSamusDamageFlyTourStepDamageFly:
        if (samus->status_id == nFTCommonStatusDamageFall)
        {
            sNdsSamusDamageFlyTourFrames = 0u;
            sNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepDamageFall;
        }
        else if ((samus->status_id == nFTCommonStatusDownBounceD) ||
                 (samus->status_id == nFTCommonStatusDownBounceU) ||
                 (samus->status_id == nFTCommonStatusDownWaitD) ||
                 (samus->status_id == nFTCommonStatusDownWaitU) ||
                 (samus->status_id == nFTCommonStatusDownStandD) ||
                 (samus->status_id == nFTCommonStatusDownStandU) ||
                 (samus->status_id == nFTCommonStatusPassive) ||
                 (samus->status_id == nFTCommonStatusPassiveStandF) ||
                 (samus->status_id == nFTCommonStatusPassiveStandB))
        {
            /* A low-angle grounded tumble can enter DamageFall and consume its
             * floor map callback inside one gcRunAll, before this once-per-frame
             * proof sampler observes DamageFall itself. Those are all source
             * descendants of the already-qualified DamageFly hit; join the
             * existing controller-driven get-up lane rather than demanding a
             * sampling artifact from BattleShip's transient state. */
            sNdsSamusDamageFlyTourFrames = 0u;
            sNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepLanding;
        }
        else if ((samus->status_id == nFTCommonStatusWait) &&
                 (samus->ga == nMPKineticsGround))
        {
            /* The same transient DamageFall path can fully settle to Wait in a
             * single fast-logic update. The claimed state/placement was already
             * recorded before entering this recovery-only step. */
            sNdsSamusDamageFlyTourFrames = 0u;
            sNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepRecover;
        }
        break;
    case nNDSSamusDamageFlyTourStepDamageFall:
        if (sNdsSamusDamageFlyTourFrames >= 1u)
        {
            (void)ndsSamusDamageFlyTourStageLanding(samus);
        }
        break;
    case nNDSSamusDamageFlyTourStepLanding:
        if ((samus->status_id == nFTCommonStatusPassive) ||
            (samus->status_id == nFTCommonStatusPassiveStandF) ||
            (samus->status_id == nFTCommonStatusPassiveStandB) ||
            (samus->status_id == nFTCommonStatusDownStandD) ||
            (samus->status_id == nFTCommonStatusDownStandU) ||
            (samus->status_id == nFTCommonStatusWait))
        {
            sNdsSamusDamageFlyTourFrames = 0u;
            sNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepRecover;
        }
        break;
    case nNDSSamusDamageFlyTourStepRecover:
        if ((samus->status_id == nFTCommonStatusWait) &&
            (fox->status_id == nFTCommonStatusWait) &&
            (samus->ga == nMPKineticsGround) && (fox->ga == nMPKineticsGround))
        {
            if (sNdsSamusDamageFlyTourScenarioAccepted != 0u)
            {
                sNdsSamusDamageFlyTourScenario++;
            }
            sNdsSamusDamageFlyTourFrames = 0u;
            sNdsSamusDamageFlyTourAttackPressed = 0u;
            sNdsSamusDamageFlyTourHitRecorded = 0u;
            sNdsSamusDamageFlyTourScenarioAccepted = 0u;
            if (sNdsSamusDamageFlyTourScenario >= nNDSSamusDamageFlyTourDone)
            {
                gNdsSamusDamageFlyTourScenario = nNDSSamusDamageFlyTourDone;
                gNdsSamusDamageFlyTourDone = 1u;
                ndsSamusDamageFlyTourProofTerminal();
                return TRUE;
            }
            sNdsSamusDamageFlyTourStep = nNDSSamusDamageFlyTourStepPrepare;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

static sb32 ndsSamusDamageFlyTourApplyInput(FTStruct *fp[2], u16 button[2],
                                             s8 stick_x[2], s8 stick_y[2])
{
    FTStruct *samus = fp[0];
    FTStruct *fox = fp[1];

    if ((sNdsSamusDamageFlyTourActive == 0u) ||
        (gNdsSamusDamageFlyTourDone != 0u) ||
        (samus == NULL) || (fox == NULL) ||
        (samus->fkind != nFTKindSamus) || (fox->fkind != nFTKindFox))
    {
        return FALSE;
    }
    if (sNdsSamusDamageFlyTourStep == nNDSSamusDamageFlyTourStepDrive)
    {
        if (sNdsSamusDamageFlyTourScenario == nNDSSamusDamageFlyTourHi)
        {
            if ((sNdsSamusDamageFlyTourAttackPressed == 0u) &&
                (fox->status_id == nFTCommonStatusWait))
            {
                /* A source C-button jump is the clean way to reach Samus's
                 * placement-2 head hurtbox without intersecting the earlier
                 * placement-1 torso entries. Do not move either fighter after
                 * the jump starts: the source jump physics owns the approach. */
                button[1] = U_CBUTTONS;
                sNdsSamusDamageFlyTourAttackPressed = 1u;
            }
            else if ((sNdsSamusDamageFlyTourAttackPressed == 1u) &&
                     ((fox->status_id == nFTCommonStatusJumpF) ||
                      (fox->status_id == nFTCommonStatusJumpB) ||
                      (fox->status_id == nFTCommonStatusFall) ||
                      (fox->status_id == nFTCommonStatusFallAerial)) &&
                     (fox->joints[nFTPartsJointTopN] != NULL) &&
                     (samus->joints[nFTPartsJointTopN] != NULL) &&
                     ((fox->joints[nFTPartsJointTopN]->translate.vec.f.y -
                       samus->joints[nFTPartsJointTopN]->translate.vec.f.y) >=
                      300.0F))
            {
                /* Fox N-air is a real 361-degree level-3 hit at the staged
                 * percent. Starting it above Samus makes the first intersected
                 * source damage-collision entry the head instead of torso. */
                button[1] = A_BUTTON;
                sNdsSamusDamageFlyTourAttackPressed = 2u;
            }
        }
        else if ((sNdsSamusDamageFlyTourAttackPressed == 0u) &&
                 (fox->status_id == nFTCommonStatusWait))
        {
            button[1] = A_BUTTON;
            switch (sNdsSamusDamageFlyTourScenario)
            {
            case nNDSSamusDamageFlyTourN:
            case nNDSSamusDamageFlyTourRoll:
                stick_x[1] = 40;
                break;
            case nNDSSamusDamageFlyTourLw:
                /* The source down-smash is the grounded attack whose foot
                 * hitboxes naturally reach Samus's placement-0 leg chain. */
                stick_y[1] = -80;
                break;
            case nNDSSamusDamageFlyTourTop:
                stick_y[1] = 80;
                break;
            default:
                break;
            }
            sNdsSamusDamageFlyTourAttackPressed = 1u;
        }
    }
    else if (sNdsSamusDamageFlyTourStep == nNDSSamusDamageFlyTourStepDamageFall)
    {
        /* Populate the source Z-history one update before the staged floor
         * crossing. BattleShip still owns Passive selection. */
        button[0] = Z_TRIG;
    }
    else if ((sNdsSamusDamageFlyTourStep == nNDSSamusDamageFlyTourStepLanding) &&
             ((samus->status_id == nFTCommonStatusDownWaitD) ||
              (samus->status_id == nFTCommonStatusDownWaitU)))
    {
        button[0] = Z_TRIG;
    }
    return TRUE;
}
#endif

#if NDS_P2_SAMUS_ATTACK_TOUR
extern void osWritebackDCacheAll(void);

__attribute__((noinline, used))
void ndsSamusAttackTourProofStop(void)
{
    /* GDB reads ARM9 main RAM, not dirty D-cache lines.  The public proof
     * state is flushed by the caller before entering this otherwise-empty
     * marker, so a breakpoint here observes the state the guest actually
     * published. */
    __asm__ volatile ("" ::: "memory");
}

__attribute__((noinline, used))
void ndsSamusAttackTourProofTerminal(void)
{
    /* Stable cache-coherent GDB stop for the proof build.  The optimized
     * attack driver is otherwise fully inlined, so source-line/function
     * breakpoints are not a reliable verifier surface. */
    gNdsSamusAttackTourTerminalCount++;
    osWritebackDCacheAll();
    ndsSamusAttackTourProofStop();
}

static u32 ndsSamusAttackTourBitForStatus(s32 status_id)
{
    u32 bit = 0u;

    switch (status_id)
    {
    case nFTCommonStatusAttack11: bit = 1u << 0; break;
    case nFTCommonStatusAttack12: bit = 1u << 1; break;
    case nFTCommonStatusAttackDash: bit = 1u << 2; break;
    case nFTCommonStatusAttackS3Hi: bit = 1u << 3; break;
    case nFTCommonStatusAttackS3HiS: bit = 1u << 4; break;
    case nFTCommonStatusAttackS3: bit = 1u << 5; break;
    case nFTCommonStatusAttackS3LwS: bit = 1u << 6; break;
    case nFTCommonStatusAttackS3Lw: bit = 1u << 7; break;
    case nFTCommonStatusAttackHi3: bit = 1u << 8; break;
    case nFTCommonStatusAttackLw3: bit = 1u << 9; break;
    case nFTCommonStatusAttackS4Hi: bit = 1u << 10; break;
    case nFTCommonStatusAttackS4HiS: bit = 1u << 11; break;
    case nFTCommonStatusAttackS4: bit = 1u << 12; break;
    case nFTCommonStatusAttackS4LwS: bit = 1u << 13; break;
    case nFTCommonStatusAttackS4Lw: bit = 1u << 14; break;
    case nFTCommonStatusAttackHi4: bit = 1u << 15; break;
    case nFTCommonStatusAttackLw4: bit = 1u << 16; break;
    case nFTCommonStatusAttackAirN: bit = 1u << 17; break;
    case nFTCommonStatusAttackAirF: bit = 1u << 18; break;
    case nFTCommonStatusAttackAirB: bit = 1u << 19; break;
    case nFTCommonStatusAttackAirHi: bit = 1u << 20; break;
    case nFTCommonStatusAttackAirLw: bit = 1u << 21; break;
    case nFTCommonStatusThrowF: bit = 1u << 22; break;
    case nFTCommonStatusThrowB: bit = 1u << 23; break;
    default: break;
    }
    return bit;
}

static void ndsSamusAttackTourRecord(FTStruct *samus, FTStruct *fox)
{
    u32 bit;

    if (samus == NULL)
    {
        return;
    }
    if (fox != NULL)
    {
        u32 i;

        for (i = 0u; i < ARRAY_COUNT(fox->damage_colls); i++)
        {
            if (fox->damage_colls[i].hitstatus == nGMHitStatusNone)
            {
                break;
            }
            if (fox->damage_colls[i].is_grabbable != FALSE)
            {
                gNdsSamusAttackTourVictimGrabbableMask |= 1u << i;
            }
            if (fox->damage_colls[i].hitstatus == nGMHitStatusNormal)
            {
                gNdsSamusAttackTourVictimNormalMask |= 1u << i;
            }
        }
    }
    gNdsSamusAttackTourStatus = (u32)samus->status_id;
    gNdsSamusAttackTourMotion = (u32)samus->motion_id;
    if (samus->attr != NULL)
    {
        gNdsSamusAttackTourCatchAttr = (u32)samus->attr->is_have_catch;
    }
    if ((samus->input.pl.button_hold & samus->input.button_mask_z) &&
        (samus->input.pl.button_tap & samus->input.button_mask_a))
    {
        gNdsSamusAttackTourGrabInputCount++;
    }
    switch (samus->status_id)
    {
    case nFTCommonStatusCatch:
        {
            u32 i;
            u32 anim_milli = (samus->fighter_gobj != NULL) ?
                (u32)ndsFloatToMilliSigned(samus->fighter_gobj->anim_frame) : 0u;
            s32 fox_x_milli = (fox != NULL) ?
                ndsFloatToMilliSigned(fox->joints[nFTPartsJointTopN]->translate.vec.f.x) : 0;

            gNdsSamusAttackTourCatchFrames++;
            if (samus->joints[36] != NULL)
            {
                gNdsSamusAttackTourJoint36SeenCount++;
            }
            if (samus->is_catchstatus != FALSE)
            {
                gNdsSamusAttackTourCatchActiveFrames++;
            }
            if (samus->search_gobj != NULL)
            {
                gNdsSamusAttackTourCatchSearchFrames++;
            }
            for (i = 0u; i < ARRAY_COUNT(samus->attack_colls); i++)
            {
                if (samus->attack_colls[i].attack_state != nGMAttackStateOff)
                {
                    s32 attack_x_milli;
                    s32 dx_milli;

                    gNdsSamusAttackTourCatchAttackMask |= 1u << i;
                    if ((samus->attack_colls[i].joint_id == 36) &&
                        (samus->attack_colls[i].joint != NULL) &&
                        (samus->attack_colls[i].joint == samus->joints[36]))
                    {
                        gNdsSamusAttackTourJoint36AttackMask |= 1u << i;
                    }
                    attack_x_milli = ndsFloatToMilliSigned(
                        samus->attack_colls[i].pos_curr.x);
                    dx_milli = attack_x_milli - fox_x_milli;
                    if (dx_milli < 0)
                    {
                        dx_milli = -dx_milli;
                    }
                    if (dx_milli < gNdsSamusAttackTourMinGrabDXMilli)
                    {
                        gNdsSamusAttackTourMinGrabDXMilli = dx_milli;
                        gNdsSamusAttackTourFoxXMilli = fox_x_milli;
                        if (i == 0u)
                        {
                            gNdsSamusAttackTourGrab0XMilli = attack_x_milli;
                        }
                        else if (i == 1u)
                        {
                            gNdsSamusAttackTourGrab1XMilli = attack_x_milli;
                        }
                    }
                }
            }
            if (anim_milli > gNdsSamusAttackTourCatchAnimFrameMaxMilli)
            {
                gNdsSamusAttackTourCatchAnimFrameMaxMilli = anim_milli;
            }
        }
        gNdsSamusAttackTourCatchStatusMask |= 1u << 0;
        break;
    case nFTCommonStatusCatchPull:
        gNdsSamusAttackTourCatchStatusMask |= 1u << 1;
        break;
    case nFTCommonStatusCatchWait:
        gNdsSamusAttackTourCatchStatusMask |= 1u << 2;
        break;
    default:
        break;
    }
    bit = ndsSamusAttackTourBitForStatus(samus->status_id);
    gNdsSamusAttackTourMask |= bit;
}

void ndsSamusAttackTourRecordStatusTransition(GObj *fighter_gobj,
                                               s32 status_id)
{
    FTStruct *fp;

    if ((sNdsSamusAttackTourActive == 0u) ||
        (gNdsSamusAttackTourDone != 0u) || (fighter_gobj == NULL))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (fp->fkind != nFTKindSamus))
    {
        return;
    }
    gNdsSamusAttackTourStatus = (u32)status_id;
    gNdsSamusAttackTourMotion = (u32)fp->motion_id;
    /* CatchPull may begin and end inside one gcRunAll: the source catch
     * callback installs it immediately on collision, and its animation can
     * advance to CatchWait before the once-per-update tour sampler runs.  This
     * transition observer sits after BattleShip's ftMainSetStatus, so include
     * those transient source-owned catch states in the same cumulative mask. */
    switch (status_id)
    {
    case nFTCommonStatusCatch:
        gNdsSamusAttackTourCatchStatusMask |= 1u << 0;
        break;
    case nFTCommonStatusCatchPull:
        gNdsSamusAttackTourCatchStatusMask |= 1u << 1;
        break;
    case nFTCommonStatusCatchWait:
        gNdsSamusAttackTourCatchStatusMask |= 1u << 2;
        break;
    default:
        break;
    }
    gNdsSamusAttackTourMask |= ndsSamusAttackTourBitForStatus(status_id);
}

static u32 ndsSamusAttackTourExpectedMask(u32 scenario)
{
    if (scenario == nNDSSamusAttackTourJab)
    {
        return (1u << 0) | (1u << 1);
    }
    if ((scenario >= nNDSSamusAttackTourDash) &&
        (scenario <= nNDSSamusAttackTourThrowB))
    {
        return 1u << (scenario + 1u);
    }
    return 0u;
}

static void ndsSamusAttackTourPlaceGround(FTStruct *fp, f32 x, f32 floor_y,
                                          u16 floor_flags)
{
    DObj *root = fp->joints[nFTPartsJointTopN];

    root->translate.vec.f.x = x;
    root->translate.vec.f.y = floor_y - fp->coll_data.map_coll.bottom;
    root->translate.vec.f.z = 0.0F;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.pos_prev = root->translate.vec.f;
    fp->coll_data.floor_line_id = sNdsSamusAttackTourFloorLine;
    fp->coll_data.floor_flags = floor_flags;
    fp->coll_data.mask_curr = MAP_FLAG_FLOOR;
    fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
    fp->coll_data.cliff_id = -1;
    fp->coll_data.ignore_line_id = -1;
    fp->physics.vel_ground.x = 0.0F;
    fp->vel_ground.x = 0.0F;
}

static sb32 ndsSamusAttackTourPrepare(FTStruct *fp[2])
{
    FTStruct *samus = fp[0];
    FTStruct *fox = fp[1];
    Vec3f edge;
    u16 flags;
    f32 samus_x = -500.0F;
    f32 fox_x = 500.0F;

    if ((samus == NULL) || (fox == NULL) ||
        (samus->fkind != nFTKindSamus) || (fox->fkind != nFTKindFox) ||
        (samus->status_id != nFTCommonStatusWait) ||
        (fox->status_id != nFTCommonStatusWait) ||
        (samus->ga != nMPKineticsGround) || (fox->ga != nMPKineticsGround) ||
        (samus->joints[nFTPartsJointTopN] == NULL) ||
        (fox->joints[nFTPartsJointTopN] == NULL))
    {
        return FALSE;
    }

    sNdsSamusAttackTourFloorLine = 3;
    mpCollisionGetFloorEdgeR(sNdsSamusAttackTourFloorLine, &edge);
    flags = mpCollisionGetVertexFlagsLineID(sNdsSamusAttackTourFloorLine);
    if ((flags & MAP_VERTEX_COLL_CLIFF) == 0u)
    {
        return FALSE;
    }
    samus->lr = +1;
    fox->lr = -1;
    samus->percent_damage = 0;
    fox->percent_damage = 0;
    ndsSamusAttackTourPlaceGround(samus, samus_x, edge.y, flags);
    ndsSamusAttackTourPlaceGround(fox, fox_x, edge.y, flags);
    sNdsSamusAttackTourExpectedMask =
        ndsSamusAttackTourExpectedMask(sNdsSamusAttackTourScenario);
    sNdsSamusAttackTourFrames = 0u;
    sNdsSamusAttackTourStep = nNDSSamusAttackTourStepRearm;
    gNdsSamusAttackTourStageCount++;
    return TRUE;
}

static sb32 ndsSamusAttackTourAdvance(FTStruct *fp[2])
{
    FTStruct *samus = fp[0];
    FTStruct *fox = fp[1];

    sNdsSamusAttackTourActive = 1u;
    if (gNdsSamusAttackTourDone != 0u)
    {
        return TRUE;
    }
    if ((samus == NULL) || (fox == NULL) ||
        (samus->fkind != nFTKindSamus) || (fox->fkind != nFTKindFox))
    {
        return FALSE;
    }
    ndsSamusAttackTourRecord(samus, fox);
    gNdsSamusAttackTourScenario = sNdsSamusAttackTourScenario;
    gNdsSamusAttackTourStep = sNdsSamusAttackTourStep;
    gNdsSamusAttackTourFrames = ++sNdsSamusAttackTourFrames;
    if (sNdsSamusAttackTourFrames == (NDS_SAMUS_ATTACK_TOUR_TIMEOUT + 1u))
    {
        ndsSamusAttackTourProofTerminal();
        gNdsFighterNaturalCombatStallCount++;
    }

    switch (sNdsSamusAttackTourStep)
    {
    case nNDSSamusAttackTourStepPrepare:
        (void)ndsSamusAttackTourPrepare(fp);
        break;
    case nNDSSamusAttackTourStepRearm:
        /* One proof-owned neutral controller update makes BattleShip's
         * tap_stick_x/tap_stick_y edge detector observe a genuine release
         * before the next directional attack input. */
        if (sNdsSamusAttackTourFrames >= 1u)
        {
            sNdsSamusAttackTourFrames = 0u;
            sNdsSamusAttackTourStep = nNDSSamusAttackTourStepDrive;
        }
        break;
    case nNDSSamusAttackTourStepDrive:
        if ((sNdsSamusAttackTourExpectedMask != 0u) &&
            ((gNdsSamusAttackTourMask & sNdsSamusAttackTourExpectedMask) ==
             sNdsSamusAttackTourExpectedMask))
        {
            sNdsSamusAttackTourFrames = 0u;
            sNdsSamusAttackTourStep = nNDSSamusAttackTourStepRecover;
        }
        break;
    case nNDSSamusAttackTourStepRecover:
        if ((samus->status_id == nFTCommonStatusWait) &&
            (fox->status_id == nFTCommonStatusWait) &&
            (samus->ga == nMPKineticsGround) && (fox->ga == nMPKineticsGround))
        {
            sNdsSamusAttackTourScenario++;
            sNdsSamusAttackTourFrames = 0u;
            sNdsSamusAttackTourExpectedMask = 0u;
            if (sNdsSamusAttackTourScenario >= nNDSSamusAttackTourDone)
            {
                gNdsSamusAttackTourScenario = nNDSSamusAttackTourDone;
                gNdsSamusAttackTourDone = 1u;
                ndsSamusAttackTourProofTerminal();
                return TRUE;
            }
            sNdsSamusAttackTourStep = nNDSSamusAttackTourStepPrepare;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

static sb32 ndsSamusAttackTourApplyInput(FTStruct *fp[2], u16 button[2],
                                         s8 stick_x[2], s8 stick_y[2])
{
    FTStruct *samus = fp[0];
    FTStruct *fox = fp[1];
    s8 forward;

    if ((sNdsSamusAttackTourActive == 0u) ||
        (gNdsSamusAttackTourDone != 0u) ||
        (samus == NULL) || (fox == NULL) ||
        (samus->fkind != nFTKindSamus) || (fox->fkind != nFTKindFox))
    {
        return FALSE;
    }
    if (sNdsSamusAttackTourStep != nNDSSamusAttackTourStepDrive)
    {
        /* Once armed, this proof owns both controllers until it is done.
         * Neutral during setup/recovery prevents later proof drivers from
         * contaminating the source input history between scenarios. */
        return TRUE;
    }
    forward = (samus->lr >= 0.0F) ? 1 : -1;

    switch (sNdsSamusAttackTourScenario)
    {
    case nNDSSamusAttackTourJab:
        if (samus->status_id == nFTCommonStatusWait)
        {
            button[0] = A_BUTTON;
        }
        else if ((samus->status_id == nFTCommonStatusAttack11) &&
                 ((sNdsSamusAttackTourFrames % 3u) == 0u))
        {
            button[0] = A_BUTTON;
        }
        break;
    case nNDSSamusAttackTourDash:
        if (samus->status_id == nFTCommonStatusWait)
        {
            stick_x[0] = (s8)(80 * forward);
        }
        else if ((samus->status_id == nFTCommonStatusDash) &&
                 (samus->fighter_gobj != NULL) &&
                 (samus->fighter_gobj->anim_frame > 5.0F) &&
                 (samus->fighter_gobj->anim_frame <= 20.0F))
        {
            /* BattleShip ftcommondash.c checks AttackDash only in this exact
             * countdown window. Tapping A immediately on Dash entry is too
             * early; holding it until here also fails because button_tap is
             * no longer fresh. Stay neutral until the source window opens. */
            button[0] = A_BUTTON;
        }
        break;
    case nNDSSamusAttackTourS3Hi:
    case nNDSSamusAttackTourS3HiS:
    case nNDSSamusAttackTourS3:
    case nNDSSamusAttackTourS3LwS:
    case nNDSSamusAttackTourS3Lw:
        if (samus->status_id == nFTCommonStatusWait)
        {
            static const s8 y[5] = { 34, 14, 0, -14, -34 };
            button[0] = A_BUTTON;
            stick_x[0] = (s8)(40 * forward);
            stick_y[0] = y[sNdsSamusAttackTourScenario - nNDSSamusAttackTourS3Hi];
        }
        break;
    case nNDSSamusAttackTourHi3:
        if (samus->status_id == nFTCommonStatusWait)
        {
            button[0] = A_BUTTON;
            stick_y[0] = 40;
        }
        break;
    case nNDSSamusAttackTourLw3:
        if (samus->status_id == nFTCommonStatusWait)
        {
            button[0] = A_BUTTON;
            stick_y[0] = -40;
        }
        break;
    case nNDSSamusAttackTourS4Hi:
    case nNDSSamusAttackTourS4HiS:
    case nNDSSamusAttackTourS4:
    case nNDSSamusAttackTourS4LwS:
    case nNDSSamusAttackTourS4Lw:
        if (samus->status_id == nFTCommonStatusWait)
        {
            static const s8 y[5] = { 40, 20, 0, -20, -40 };
            button[0] = A_BUTTON;
            stick_x[0] = (s8)(80 * forward);
            stick_y[0] = y[sNdsSamusAttackTourScenario - nNDSSamusAttackTourS4Hi];
        }
        break;
    case nNDSSamusAttackTourHi4:
        if (samus->status_id == nFTCommonStatusWait)
        {
            button[0] = A_BUTTON;
            stick_y[0] = 80;
        }
        break;
    case nNDSSamusAttackTourLw4:
        if (samus->status_id == nFTCommonStatusWait)
        {
            button[0] = A_BUTTON;
            stick_y[0] = -80;
        }
        break;
    case nNDSSamusAttackTourAirN:
    case nNDSSamusAttackTourAirF:
    case nNDSSamusAttackTourAirB:
    case nNDSSamusAttackTourAirHi:
    case nNDSSamusAttackTourAirLw:
        if ((samus->status_id == nFTCommonStatusWait) &&
            (samus->ga == nMPKineticsGround))
        {
            button[0] = U_CBUTTONS;
        }
        else if ((samus->status_id == nFTCommonStatusJumpF) ||
                 (samus->status_id == nFTCommonStatusJumpB) ||
                 (samus->status_id == nFTCommonStatusFall) ||
                 (samus->status_id == nFTCommonStatusFallAerial))
        {
            button[0] = A_BUTTON;
            switch (sNdsSamusAttackTourScenario)
            {
            case nNDSSamusAttackTourAirF: stick_x[0] = (s8)(40 * forward); break;
            case nNDSSamusAttackTourAirB: stick_x[0] = (s8)(-40 * forward); break;
            case nNDSSamusAttackTourAirHi: stick_y[0] = 40; break;
            case nNDSSamusAttackTourAirLw: stick_y[0] = -40; break;
            default: break;
            }
        }
        break;
    case nNDSSamusAttackTourThrowF:
    case nNDSSamusAttackTourThrowB:
        if ((samus->status_id != nFTCommonStatusCatchWait) &&
            (samus->status_id != nFTCommonStatusCatch) &&
            (samus->status_id != nFTCommonStatusCatchPull))
        {
            f32 dx = ndsFighterNaturalCombatPosX(fox) -
                ndsFighterNaturalCombatPosX(samus);
            f32 adx = (dx < 0.0F) ? -dx : dx;

            /* Reuse the source-qualified f23 grab approach. BattleShip's
             * push/collision path owns the reachable spacing; once inside the
             * same stop range, feed Catch's real Z-hold + A-tap contract. */
            if (adx > NDS_FIGHTER_NATURAL_MOVESET_GRAB_STOP_RANGE)
            {
                stick_x[0] = (dx >= 0.0F) ? 40 : -40;
            }
            else if ((samus->status_id == nFTCommonStatusWait) &&
                     (fox->status_id == nFTCommonStatusWait) &&
                     ((sNdsSamusAttackTourFrames % 4u) == 0u))
            {
                button[0] = Z_TRIG | A_BUTTON;
            }
        }
        else if (samus->status_id == nFTCommonStatusCatchWait)
        {
            if (sNdsSamusAttackTourScenario == nNDSSamusAttackTourThrowF)
            {
                button[0] = A_BUTTON;
            }
            else
            {
                stick_x[0] = (s8)(-80 * forward);
            }
        }
        break;
    default:
        break;
    }
    return TRUE;
}
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI || NDS_P2_DONKEY
static void ndsFighterNaturalSpecialsSetPhase(u32 phase)
{
    sNdsNaturalSpecialsPhase = phase;
    sNdsNaturalSpecialsPhaseFrames = 0u;
    sNdsNaturalSpecialsButtonPressed = 0u;
    gNdsFighterSpecialsProofPhase = phase;
    gNdsFighterSpecialsProofPhaseFrames = 0u;
}

static sb32 ndsFighterNaturalSpecialsBothGroundWait(FTStruct *fp[2])
{
    return ((ndsFighterNaturalCombatBothWait(fp) != FALSE) &&
            (fp[0]->ga == nMPKineticsGround) &&
            (fp[1]->ga == nMPKineticsGround)) ? TRUE : FALSE;
}

static void ndsFighterNaturalSpecialsUpdateMask(void)
{
    u32 mask = 0u;

    if ((gNdsFighterSpecialsMarioHiPressFrames > 0u) &&
        (gNdsFighterSpecialsMarioHiFrames > 0u))
    {
        mask |= 1u << 0;
    }
    if (gNdsFighterSpecialsMarioHiRootYMilli > 1000)
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterSpecialsMarioFallSpecialFrames > 0u) ||
        (gNdsFighterSpecialsMarioLandingFallSpecialFrames > 0u))
    {
        mask |= 1u << 2;
    }
    if (gNdsFighterSpecialsMarioHiWaitFrames >=
        NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED)
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterSpecialsMarioLwPressFrames > 0u) &&
        ((gNdsFighterSpecialsMarioLwFrames > 0u) ||
         (gNdsFighterSpecialsMarioAirLwFrames > 0u)))
    {
        mask |= 1u << 4;
    }
    if (gNdsFighterSpecialsMarioLwDustEffectCount > 0u)
    {
        mask |= 1u << 5;
    }
    if (gNdsFighterSpecialsMarioLwWaitFrames >=
        NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED)
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterSpecialsFoxHiPressFrames > 0u) &&
        (gNdsFighterSpecialsFoxHiStartFrames > 0u))
    {
        mask |= 1u << 7;
    }
    if (gNdsFighterSpecialsFoxHiHoldFrames > 0u)
    {
        mask |= 1u << 8;
    }
    if (gNdsFighterSpecialsFoxHiTravelFrames > 0u)
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterSpecialsFoxHiEndFrames > 0u) ||
        (gNdsFighterSpecialsFoxHiBoundFrames > 0u))
    {
        mask |= 1u << 10;
    }
    if (gNdsFighterSpecialsFoxHiWaitFrames >=
        NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED)
    {
        mask |= 1u << 11;
    }
    gNdsFighterSpecialsProofMask = mask;
}

static sb32 ndsFighterNaturalSpecialsStartNext(FTStruct *fp[2])
{
    ndsFighterNaturalSpecialsUpdateMask();
#if NDS_P2_DONKEY
    {
        FTStruct *donkey = fp[gNdsFighterDonkeySpecialsSlot];

        if (ndsFighterNaturalIsDonkey(donkey) != FALSE)
        {
            if ((gNdsFighterDonkeySpecialsNStorePressFrames == 0u) ||
                (gNdsFighterDonkeySpecialsNStoredChargeMax <
                 NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED) ||
                (gNdsFighterDonkeySpecialsNStoredWaitFrames <
                 NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED))
            {
                ndsFighterNaturalSpecialsSetPhase(
                    nNDSNaturalSpecialsPhaseDonkeyNCharge);
                return FALSE;
            }
            if ((gNdsFighterDonkeySpecialsNResumePressFrames == 0u) ||
                (gNdsFighterDonkeySpecialsNReleaseTapFrames == 0u) ||
                (gNdsFighterDonkeySpecialsNEndFrames == 0u) ||
                (gNdsFighterDonkeySpecialsNReleaseChargeMax <
                 NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED) ||
                (gNdsFighterDonkeySpecialsNPassiveResetFrames == 0u) ||
                (gNdsFighterDonkeySpecialsNReleaseWaitFrames <
                 NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED))
            {
                ndsFighterNaturalSpecialsSetPhase(
                    nNDSNaturalSpecialsPhaseDonkeyNRelease);
                return FALSE;
            }
            if ((gNdsFighterDonkeySpecialsHiPressFrames == 0u) ||
                (gNdsFighterDonkeySpecialsHiFrames == 0u) ||
                (gNdsFighterDonkeySpecialsHiWaitFrames <
                 NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED))
            {
                ndsFighterNaturalSpecialsSetPhase(
                    nNDSNaturalSpecialsPhaseDonkeyHi);
                return FALSE;
            }
            if ((gNdsFighterDonkeySpecialsLwPressFrames == 0u) ||
                (gNdsFighterDonkeySpecialsLwStartFrames == 0u) ||
                (gNdsFighterDonkeySpecialsLwLoopFrames == 0u) ||
                (gNdsFighterDonkeySpecialsLwRepeatPressFrames == 0u) ||
                (gNdsFighterDonkeySpecialsLwEndFrames == 0u) ||
                (gNdsFighterDonkeySpecialsLwWaitFrames <
                 NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED))
            {
                ndsFighterNaturalSpecialsSetPhase(
                    nNDSNaturalSpecialsPhaseDonkeyLw);
                return FALSE;
            }
        }
    }
#endif
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW
    if ((ndsFighterNaturalIsMarioSpecialFamily(
             fp[gNdsFighterSpecialsMarioSlot]) != FALSE) &&
        ((gNdsFighterSpecialsProofMask &
         NDS_FIGHTER_SPECIALS_MARIO_LW_MASK) !=
         NDS_FIGHTER_SPECIALS_MARIO_LW_MASK))
    {
        ndsFighterNaturalSpecialsSetPhase(nNDSNaturalSpecialsPhaseMarioLw);
        return FALSE;
    }
#endif
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI
    if ((ndsFighterNaturalIsMarioSpecialFamily(
             fp[gNdsFighterSpecialsMarioSlot]) != FALSE) &&
        ((gNdsFighterSpecialsProofMask &
         NDS_FIGHTER_SPECIALS_MARIO_HI_MASK) !=
         NDS_FIGHTER_SPECIALS_MARIO_HI_MASK))
    {
        ndsFighterNaturalSpecialsSetPhase(nNDSNaturalSpecialsPhaseMarioHi);
        return FALSE;
    }
#endif
#if NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
    if ((fp[gNdsFighterSpecialsFoxSlot] != NULL) &&
        (fp[gNdsFighterSpecialsFoxSlot]->fkind == nFTKindFox) &&
        ((gNdsFighterSpecialsProofMask &
         NDS_FIGHTER_SPECIALS_FOX_HI_MASK) !=
         NDS_FIGHTER_SPECIALS_FOX_HI_MASK))
    {
        ndsFighterNaturalSpecialsSetPhase(nNDSNaturalSpecialsPhaseFoxHi);
        return FALSE;
    }
#endif
    sNdsNaturalSpecialsDone = 1u;
    ndsFighterNaturalSpecialsSetPhase(nNDSNaturalSpecialsPhaseDone);
    return TRUE;
}

static void ndsFighterNaturalSpecialsRecordRoot(FTStruct *fp,
                                                 volatile s32 *max_milli)
{
    DObj *root;
    s32 root_y;

    if ((fp == NULL) || (max_milli == NULL))
    {
        return;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return;
    }
    root_y = ndsFloatToMilliSigned(root->translate.vec.f.y);
    if (root_y > *max_milli)
    {
        *max_milli = root_y;
    }
}

static void ndsFighterNaturalSpecialsRecordDamageMax(
    FTStruct *fp, volatile u32 *max_damage)
{
    u32 i;

    if ((fp == NULL) || (max_damage == NULL))
    {
        return;
    }
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        const FTAttackColl *attack = &fp->attack_colls[i];

        if ((attack->attack_state != nGMAttackStateOff) &&
            (attack->damage > 0) && ((u32)attack->damage > *max_damage))
        {
            *max_damage = (u32)attack->damage;
        }
    }
}

static void ndsFighterNaturalSpecialsRecord(FTStruct *fp[2])
{
    FTStruct *mario;
    FTStruct *fox;
#if NDS_P2_DONKEY
    FTStruct *donkey;
#endif

    if (ndsFighterNaturalSpecialsProofEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterSpecialsProofPhase = sNdsNaturalSpecialsPhase;
    gNdsFighterSpecialsProofPhaseFrames = sNdsNaturalSpecialsPhaseFrames;
    mario = fp[gNdsFighterSpecialsMarioSlot];
    fox = fp[gNdsFighterSpecialsFoxSlot];
    if (ndsFighterNaturalIsMarioSpecialFamily(mario) == FALSE)
    {
        mario = NULL;
    }
    if ((fox == NULL) || (fox->fkind != nFTKindFox))
    {
        fox = NULL;
    }
#if NDS_P2_DONKEY
    donkey = fp[gNdsFighterDonkeySpecialsSlot];
    if (ndsFighterNaturalIsDonkey(donkey) == FALSE)
    {
        donkey = NULL;
    }
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW
    if (mario != NULL)
    {
        if ((sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseMarioHi) ||
            (sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseSettleMarioHi))
        {
            ndsFighterNaturalSpecialsRecordRoot(
                mario, &gNdsFighterSpecialsMarioHiRootYMilli);
            ndsFighterNaturalSpecialsRecordDamageMax(
                mario, &gNdsFighterSpecialsMarioHiDamageMax);
        }
        if ((sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseMarioLw) ||
            (sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseSettleMarioLw))
        {
            ndsFighterNaturalSpecialsRecordDamageMax(
                mario, &gNdsFighterSpecialsMarioLwDamageMax);
        }
        if ((mario->fkind == nFTKindMario) &&
            (mario->status_id == nFTMarioStatusSpecialHi))
        {
            gNdsFighterSpecialsMarioHiFrames++;
        }
        else if ((mario->fkind == nFTKindMario) &&
                 (mario->status_id == nFTMarioStatusSpecialAirHi))
        {
            gNdsFighterSpecialsMarioAirHiFrames++;
        }
#if NDS_P2_LUIGI
        else if ((mario->fkind == nFTKindLuigi) &&
                 (mario->status_id == nFTLuigiStatusSpecialHi))
        {
            gNdsFighterSpecialsMarioHiFrames++;
        }
        else if ((mario->fkind == nFTKindLuigi) &&
                 (mario->status_id == nFTLuigiStatusSpecialAirHi))
        {
            gNdsFighterSpecialsMarioAirHiFrames++;
        }
#endif
        else if (mario->status_id == nFTCommonStatusFallSpecial)
        {
            gNdsFighterSpecialsMarioFallSpecialFrames++;
        }
        else if (mario->status_id == nFTCommonStatusLandingFallSpecial)
        {
            gNdsFighterSpecialsMarioLandingFallSpecialFrames++;
        }
        if (((gNdsFighterSpecialsMarioHiFrames > 0u) ||
             (gNdsFighterSpecialsMarioAirHiFrames > 0u)) &&
            (mario->status_id == nFTCommonStatusWait) &&
            (mario->ga == nMPKineticsGround))
        {
            gNdsFighterSpecialsMarioHiWaitFrames++;
        }

        if ((mario->fkind == nFTKindMario) &&
            (mario->status_id == nFTMarioStatusSpecialLw))
        {
            gNdsFighterSpecialsMarioLwFrames++;
        }
        else if ((mario->fkind == nFTKindMario) &&
                 (mario->status_id == nFTMarioStatusSpecialAirLw))
        {
            gNdsFighterSpecialsMarioAirLwFrames++;
        }
#if NDS_P2_LUIGI
        else if ((mario->fkind == nFTKindLuigi) &&
                 (mario->status_id == nFTLuigiStatusSpecialLw))
        {
            gNdsFighterSpecialsMarioLwFrames++;
        }
        else if ((mario->fkind == nFTKindLuigi) &&
                 (mario->status_id == nFTLuigiStatusSpecialAirLw))
        {
            gNdsFighterSpecialsMarioAirLwFrames++;
        }
#endif
        if (((gNdsFighterSpecialsMarioLwFrames > 0u) ||
             (gNdsFighterSpecialsMarioAirLwFrames > 0u)) &&
            (mario->status_id == nFTCommonStatusWait) &&
            (mario->ga == nMPKineticsGround))
        {
            gNdsFighterSpecialsMarioLwWaitFrames++;
        }
    }
#endif
#if NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
    if (fox != NULL)
    {
        if ((sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseFoxHi) ||
            (sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseSettleFoxHi))
        {
            ndsFighterNaturalSpecialsRecordRoot(
                fox, &gNdsFighterSpecialsFoxHiRootYMilli);
        }
        if ((fox->status_id == nFTFoxStatusSpecialHiStart) ||
            (fox->status_id == nFTFoxStatusSpecialAirHiStart))
        {
            gNdsFighterSpecialsFoxHiStartFrames++;
        }
        else if ((fox->status_id == nFTFoxStatusSpecialHiHold) ||
                 (fox->status_id == nFTFoxStatusSpecialAirHiHold))
        {
            gNdsFighterSpecialsFoxHiHoldFrames++;
        }
        else if ((fox->status_id == nFTFoxStatusSpecialHi) ||
                 (fox->status_id == nFTFoxStatusSpecialAirHi))
        {
            gNdsFighterSpecialsFoxHiTravelFrames++;
        }
        else if ((fox->status_id == nFTFoxStatusSpecialHiEnd) ||
                 (fox->status_id == nFTFoxStatusSpecialAirHiEnd))
        {
            gNdsFighterSpecialsFoxHiEndFrames++;
        }
        else if (fox->status_id == nFTFoxStatusSpecialAirHiBound)
        {
            gNdsFighterSpecialsFoxHiBoundFrames++;
        }
        if ((gNdsFighterSpecialsFoxHiStartFrames > 0u) &&
            (fox->status_id == nFTCommonStatusWait) &&
            (fox->ga == nMPKineticsGround))
        {
            gNdsFighterSpecialsFoxHiWaitFrames++;
        }
    }
#endif
#if NDS_P2_DONKEY
    if (donkey != NULL)
    {
        u32 stored_charge = (u32)donkey->passive_vars.donkey.charge_level;

        if (stored_charge > gNdsFighterDonkeySpecialsNStoredChargeMax)
        {
            gNdsFighterDonkeySpecialsNStoredChargeMax = stored_charge;
        }
        if ((donkey->status_id == nFTDonkeyStatusSpecialNStart) ||
            (donkey->status_id == nFTDonkeyStatusSpecialAirNStart))
        {
            gNdsFighterDonkeySpecialsNStartFrames++;
        }
        else if ((donkey->status_id == nFTDonkeyStatusSpecialNLoop) ||
                 (donkey->status_id == nFTDonkeyStatusSpecialAirNLoop))
        {
            gNdsFighterDonkeySpecialsNLoopFrames++;
        }
        else if ((donkey->status_id == nFTDonkeyStatusSpecialNEnd) ||
                 (donkey->status_id == nFTDonkeyStatusSpecialAirNEnd) ||
                 (donkey->status_id == nFTDonkeyStatusSpecialNFull) ||
                 (donkey->status_id == nFTDonkeyStatusSpecialAirNFull))
        {
            u32 release_charge =
                (u32)donkey->status_vars.donkey.specialn.charge_level;

            gNdsFighterDonkeySpecialsNEndFrames++;
            if (release_charge > gNdsFighterDonkeySpecialsNReleaseChargeMax)
            {
                gNdsFighterDonkeySpecialsNReleaseChargeMax = release_charge;
            }
            if (donkey->passive_vars.donkey.charge_level == 0)
            {
                gNdsFighterDonkeySpecialsNPassiveResetFrames++;
            }
        }
        if ((sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseDonkeyNCharge) &&
            (gNdsFighterDonkeySpecialsNStorePressFrames > 0u) &&
            (stored_charge >=
             NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED) &&
            (donkey->status_id == nFTCommonStatusWait) &&
            (donkey->ga == nMPKineticsGround))
        {
            gNdsFighterDonkeySpecialsNStoredWaitFrames++;
        }
        if ((sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseDonkeyNRelease) &&
            (gNdsFighterDonkeySpecialsNEndFrames > 0u) &&
            (donkey->status_id == nFTCommonStatusWait) &&
            (donkey->ga == nMPKineticsGround))
        {
            gNdsFighterDonkeySpecialsNReleaseWaitFrames++;
        }

        if ((donkey->status_id == nFTDonkeyStatusSpecialHi) ||
            (donkey->status_id == nFTDonkeyStatusSpecialAirHi))
        {
            gNdsFighterDonkeySpecialsHiFrames++;
            if (donkey->ga == nMPKineticsGround)
            {
                gNdsFighterDonkeySpecialsHiGroundGAFrames++;
            }
        }
        if ((sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseDonkeyHi) &&
            (gNdsFighterDonkeySpecialsHiFrames > 0u) &&
            (donkey->status_id == nFTCommonStatusWait) &&
            (donkey->ga == nMPKineticsGround))
        {
            gNdsFighterDonkeySpecialsHiWaitFrames++;
        }

        if (donkey->status_id == nFTDonkeyStatusSpecialLwStart)
        {
            gNdsFighterDonkeySpecialsLwStartFrames++;
        }
        else if (donkey->status_id == nFTDonkeyStatusSpecialLwLoop)
        {
            gNdsFighterDonkeySpecialsLwLoopFrames++;
            if (donkey->status_vars.donkey.speciallw.is_loop != FALSE)
            {
                gNdsFighterDonkeySpecialsLwLoopFlagFrames++;
            }
        }
        else if (donkey->status_id == nFTDonkeyStatusSpecialLwEnd)
        {
            gNdsFighterDonkeySpecialsLwEndFrames++;
        }
        if ((sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseDonkeyLw) &&
            (gNdsFighterDonkeySpecialsLwEndFrames > 0u) &&
            (donkey->status_id == nFTCommonStatusWait) &&
            (donkey->ga == nMPKineticsGround))
        {
            gNdsFighterDonkeySpecialsLwWaitFrames++;
        }
    }
#endif
    ndsFighterNaturalSpecialsUpdateMask();
}

static sb32 ndsFighterNaturalSpecialsAdvance(FTStruct *fp[2])
{
    if (ndsFighterNaturalSpecialsProofEnabled() == FALSE)
    {
        return TRUE;
    }
    if (sNdsNaturalSpecialsDone != 0u)
    {
        return TRUE;
    }
    if (sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseIdle)
    {
        return ndsFighterNaturalSpecialsStartNext(fp);
    }
    sNdsNaturalSpecialsPhaseFrames++;
    gNdsFighterSpecialsProofPhaseFrames = sNdsNaturalSpecialsPhaseFrames;
    if (sNdsNaturalSpecialsPhaseFrames >
        NDS_FIGHTER_NATURAL_MOVESET_PHASE_TIMEOUT)
    {
        gNdsFighterNaturalCombatStallCount++;
        return FALSE;
    }
    switch (sNdsNaturalSpecialsPhase)
    {
    case nNDSNaturalSpecialsPhaseMarioHi:
        if ((gNdsFighterSpecialsProofMask &
             ((1u << 0) | (1u << 1))) == ((1u << 0) | (1u << 1)))
        {
            ndsFighterNaturalSpecialsSetPhase(
                nNDSNaturalSpecialsPhaseSettleMarioHi);
        }
        break;
    case nNDSNaturalSpecialsPhaseSettleMarioHi:
        if (((gNdsFighterSpecialsProofMask &
              NDS_FIGHTER_SPECIALS_MARIO_HI_MASK) ==
             NDS_FIGHTER_SPECIALS_MARIO_HI_MASK) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
    case nNDSNaturalSpecialsPhaseMarioLw:
        if ((gNdsFighterSpecialsProofMask &
             ((1u << 4) | (1u << 5))) == ((1u << 4) | (1u << 5)))
        {
            ndsFighterNaturalSpecialsSetPhase(
                nNDSNaturalSpecialsPhaseSettleMarioLw);
        }
        break;
    case nNDSNaturalSpecialsPhaseSettleMarioLw:
        if (((gNdsFighterSpecialsProofMask &
              NDS_FIGHTER_SPECIALS_MARIO_LW_MASK) ==
             NDS_FIGHTER_SPECIALS_MARIO_LW_MASK) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
    case nNDSNaturalSpecialsPhaseFoxHi:
        if ((gNdsFighterSpecialsProofMask &
             ((1u << 7) | (1u << 8) | (1u << 9))) ==
            ((1u << 7) | (1u << 8) | (1u << 9)))
        {
            ndsFighterNaturalSpecialsSetPhase(
                nNDSNaturalSpecialsPhaseSettleFoxHi);
        }
        break;
    case nNDSNaturalSpecialsPhaseSettleFoxHi:
        if (((gNdsFighterSpecialsProofMask &
              NDS_FIGHTER_SPECIALS_FOX_HI_MASK) ==
             NDS_FIGHTER_SPECIALS_FOX_HI_MASK) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
#if NDS_P2_DONKEY
    case nNDSNaturalSpecialsPhaseDonkeyNCharge:
        if ((gNdsFighterDonkeySpecialsNStorePressFrames > 0u) &&
            (gNdsFighterDonkeySpecialsNStoredChargeMax >=
             NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED) &&
            (gNdsFighterDonkeySpecialsNStoredWaitFrames >=
             NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyNRelease:
        if ((gNdsFighterDonkeySpecialsNResumePressFrames > 0u) &&
            (gNdsFighterDonkeySpecialsNReleaseTapFrames > 0u) &&
            (gNdsFighterDonkeySpecialsNEndFrames > 0u) &&
            (gNdsFighterDonkeySpecialsNReleaseChargeMax >=
             NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED) &&
            (gNdsFighterDonkeySpecialsNPassiveResetFrames > 0u) &&
            (gNdsFighterDonkeySpecialsNReleaseWaitFrames >=
             NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyHi:
        if ((gNdsFighterDonkeySpecialsHiPressFrames > 0u) &&
            (gNdsFighterDonkeySpecialsHiFrames > 0u) &&
            (gNdsFighterDonkeySpecialsHiWaitFrames >=
             NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyLw:
        if ((gNdsFighterDonkeySpecialsLwPressFrames > 0u) &&
            (gNdsFighterDonkeySpecialsLwStartFrames > 0u) &&
            (gNdsFighterDonkeySpecialsLwLoopFrames > 0u) &&
            (gNdsFighterDonkeySpecialsLwRepeatPressFrames > 0u) &&
            (gNdsFighterDonkeySpecialsLwEndFrames > 0u) &&
            (gNdsFighterDonkeySpecialsLwWaitFrames >=
             NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
#endif
    default:
        break;
    }
    return FALSE;
}
#endif

static void ndsFighterNaturalCombatAdvancePhase(FTStruct *fp[2])
{
    NDSFighterNaturalMotionState *s0 = &sNdsFighterNaturalMotionStates[0];
    NDSFighterNaturalMotionState *s1 = &sNdsFighterNaturalMotionStates[1];
    FTStruct *victim = fp[sNdsNaturalCombatVictimSlot];
    f32 dx = ndsFighterNaturalCombatPosX(fp[0]) -
        ndsFighterNaturalCombatPosX(fp[1]);
    f32 dy = fp[0]->coll_data.p_translate->y -
        fp[1]->coll_data.p_translate->y;

    if (dx < 0.0F)
    {
        dx = -dx;
    }
    if (dy < 0.0F)
    {
        dy = -dy;
    }
    gNdsFighterNaturalCombatApproachDXMilli =
        (u32)ndsFloatToMilliSigned(dx);

    if (ndsFighterNaturalProjectileHandleKORecovery(fp) != FALSE)
    {
        return;
    }

    sNdsNaturalCombatPhaseFrames++;
    gNdsFighterNaturalCombatPhaseFrames = sNdsNaturalCombatPhaseFrames;
    if ((sNdsNaturalCombatPhase != nNDSNaturalCombatPhaseDone) &&
        (sNdsNaturalCombatPhase !=
            nNDSNaturalCombatPhaseBattlePlayableDone) &&
        (sNdsNaturalCombatPhaseFrames ==
            (((ndsFighterBattlePlayableProofEnabled() != FALSE) ?
                NDS_FIGHTER_BATTLE_PLAYABLE_PHASE_TIMEOUT :
                NDS_FIGHTER_NATURAL_COMBAT_PHASE_TIMEOUT) + 1u)))
    {
        gNdsFighterNaturalCombatStallCount++;
    }

    switch (sNdsNaturalCombatPhase)
    {
    case nNDSNaturalCombatPhaseWait:
        if ((s0->wait_frames >=
                NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED) &&
            (s1->wait_frames >=
                NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED) &&
            (ndsFighterNaturalCombatBothGroundWait(fp) != FALSE) &&
            (dy <= NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE))
        {
            gNdsFighterNaturalMotionWalkInputFrame =
                gNdsFighterNaturalMotionUpdateCount + 1u;
            sNdsNaturalCombatPassPressed = 0u;
            ndsFighterNaturalCombatSetPhase(nNDSNaturalCombatPhaseWalk);
        }
        break;
    case nNDSNaturalCombatPhaseWalk:
        if ((s0->walk_frames >=
                NDS_FIGHTER_NATURAL_MOTION_WALK_FRAMES_REQUIRED) &&
            (s1->walk_frames >=
                NDS_FIGHTER_NATURAL_MOTION_WALK_FRAMES_REQUIRED))
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleWalk);
        }
        break;
    case nNDSNaturalCombatPhaseSettleWalk:
        if (ndsFighterNaturalCombatSettled(fp) != FALSE)
        {
            ndsFighterNaturalCombatSetPhase(nNDSNaturalCombatPhaseDashRun);
        }
        break;
    case nNDSNaturalCombatPhaseDashRun:
        if ((s0->dash_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_DASH_FRAMES_REQUIRED) &&
            (s1->dash_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_DASH_FRAMES_REQUIRED) &&
            (s0->run_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_RUN_FRAMES_REQUIRED) &&
            (s1->run_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_RUN_FRAMES_REQUIRED))
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseRunBrake);
        }
        break;
    case nNDSNaturalCombatPhaseRunBrake:
        if ((s0->runbrake_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_RUNBRAKE_FRAMES_REQUIRED) &&
            (s1->runbrake_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_RUNBRAKE_FRAMES_REQUIRED))
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleRun);
        }
        break;
    case nNDSNaturalCombatPhaseSettleRun:
        if (ndsFighterNaturalCombatSettled(fp) != FALSE)
        {
            ndsFighterNaturalCombatSetPhase(nNDSNaturalCombatPhaseTurn);
        }
        break;
    case nNDSNaturalCombatPhaseTurn:
        if ((s0->turn_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_TURN_FRAMES_REQUIRED) &&
            (s1->turn_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_TURN_FRAMES_REQUIRED))
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleTurn);
        }
        break;
    case nNDSNaturalCombatPhaseSettleTurn:
        if (ndsFighterNaturalCombatSettled(fp) != FALSE)
        {
            ndsFighterNaturalCombatSetPhase(
                (ndsFighterNaturalCombatMovementOnlyProofEnabled() != FALSE) ?
                    nNDSNaturalCombatPhaseDone :
                    nNDSNaturalCombatPhaseApproach);
        }
        break;
    case nNDSNaturalCombatPhaseApproach:
        if ((dx <= sNdsNaturalCombatApproachStopRange) &&
            (dy <= NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE))
        {
            sNdsNaturalCombatApproachStagnant = 0u;
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleApproach);
        }
        else if (dy <= NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE)
        {
            /* P2-3r3 (2026-08-23): arrival by body contact. A big fighter's
             * push radius can exceed the tuned stop range -- Donkey Kong vs
             * Fox reaches push equilibrium at dx=311.85 against the 240 stop
             * range, so the approach drove into the body forever (7,601
             * frames parked). When the drive is active at the same floor
             * level and dx stops shrinking for a second, the pair is
             * touching: that IS arrival, for every body size the roster will
             * ever stage. */
            f32 addx = (dx < 0.0F) ? -dx : dx;

            if ((sNdsNaturalCombatApproachLastDX - addx) > 0.5F)
            {
                sNdsNaturalCombatApproachStagnant = 0u;
            }
            else if (++sNdsNaturalCombatApproachStagnant >= 60u)
            {
                /* Adopt the contact distance as the pair's stop range so the
                 * settle and attack checks agree with it; an attack whiff
                 * retry may still shrink it, and if the pair re-parks the
                 * stagnation re-adopts it. */
                sNdsNaturalCombatApproachStagnant = 0u;
                sNdsNaturalCombatApproachStopRange = addx + 8.0F;
                ndsFighterNaturalCombatSetPhase(
                    nNDSNaturalCombatPhaseSettleApproach);
            }
            sNdsNaturalCombatApproachLastDX = addx;
        }
        break;
    case nNDSNaturalCombatPhaseSettleApproach:
        if (ndsFighterNaturalCombatSettled(fp) != FALSE)
        {
            if ((ABSF(fp[0]->physics.vel_ground.x) < 0.01F) &&
                (ABSF(fp[1]->physics.vel_ground.x) < 0.01F))
            {
                if ((dx <= sNdsNaturalCombatApproachStopRange) &&
                    (dy <=
                     NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE))
                {
                    sNdsNaturalCombatAttackPressed = 0u;
                    ndsFighterNaturalCombatSetPhase(
                        nNDSNaturalCombatPhaseAttack);
                }
                else
                {
                    ndsFighterNaturalCombatSetPhase(
                        nNDSNaturalCombatPhaseApproach);
                }
            }
        }
        break;
    case nNDSNaturalCombatPhaseAttack:
        if (sNdsNaturalCombatVictimHitSeen != 0u)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleDamage);
        }
        else if (sNdsNaturalCombatPhaseFrames >
                 NDS_FIGHTER_NATURAL_COMBAT_ATTACK_TIMEOUT)
        {
            gNdsFighterNaturalCombatAttackRetryCount++;
            if (gNdsFighterNaturalCombatAttackRetryCount >
                NDS_FIGHTER_NATURAL_COMBAT_ATTACK_RETRY_MAX)
            {
                gNdsFighterNaturalCombatStallCount++;
                ndsFighterNaturalCombatSetPhase(
                    nNDSNaturalCombatPhaseDone);
            }
            else
            {
                sNdsNaturalCombatApproachStopRange -=
                    NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_STEP;
                if (sNdsNaturalCombatApproachStopRange <
                    NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_MIN)
                {
                    sNdsNaturalCombatApproachStopRange =
                        NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_MIN;
                }
                ndsFighterNaturalCombatSetPhase(
                    nNDSNaturalCombatPhaseApproach);
            }
        }
        break;
    case nNDSNaturalCombatPhaseSettleDamage:
        if ((victim->status_id == nFTCommonStatusWait) &&
            (ndsFighterNaturalCombatSettled(fp) != FALSE))
        {
            ndsFighterNaturalCombatSetPhase(nNDSNaturalCombatPhaseGuard);
        }
        break;
    case nNDSNaturalCombatPhaseGuard:
        if (gNdsFighterNaturalCombatGuardFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_GUARD_FRAMES_REQUIRED)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseGuardOff);
        }
        break;
    case nNDSNaturalCombatPhaseGuardOff:
        if ((victim->status_id == nFTCommonStatusWait) &&
            (gNdsFighterNaturalCombatGuardOffFrames > 0u))
        {
            if (ndsFighterBattlePlayableProofEnabled() != FALSE)
            {
                if (ndsFighterNaturalProjectileProofEnabled() != FALSE)
                {
                    ndsFighterNaturalCombatSetPhase(
                        nNDSNaturalCombatPhaseProjectileSettle);
                }
                else
                {
                    ndsFighterNaturalCombatStartKOExit(victim);
                }
            }
            else
            {
                ndsFighterNaturalCombatSetPhase(nNDSNaturalCombatPhaseDone);
            }
        }
        break;
    case nNDSNaturalCombatPhaseProjectileSettle:
        {
            FTStruct *actor = fp[sNdsNaturalProjectileActorSlot];
            FTStruct *target = fp[1u - sNdsNaturalProjectileActorSlot];
            f32 midpoint = (ndsFighterNaturalCombatPosX(actor) +
                            ndsFighterNaturalCombatPosX(target)) * 0.5F;
            f32 face_dx = ndsFighterNaturalCombatPosX(target) -
                ndsFighterNaturalCombatPosX(actor);

            if (midpoint < 0.0F)
            {
                midpoint = -midpoint;
            }

            if ((dx <= NDS_FIGHTER_NATURAL_PROJECTILE_STOP_RANGE) &&
                (midpoint <= NDS_FIGHTER_NATURAL_PROJECTILE_CENTER_RANGE) &&
                (dy <= NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE) &&
                ((face_dx * actor->lr) >= 0.0F) &&
                (ndsFighterNaturalCombatBothGroundWait(fp) != FALSE) &&
                (ndsFighterNaturalCombatSettled(fp) != FALSE))
            {
                sNdsNaturalProjectileButtonPressed = 0u;
                ndsFighterNaturalCombatSetPhase(
                    nNDSNaturalCombatPhaseProjectileFire);
            }
        }
        break;
    case nNDSNaturalCombatPhaseProjectileFire:
        if ((gNdsFighterProjectileProofSpecialStatusFrames > 0u) &&
            ((gNdsFighterProjectileProofWeaponFrames > 0u) ||
             (gNdsFighterProjectileProofHitDestroyCount > 0u) ||
             (gNdsFighterProjectileProofResult ==
                NDS_FIGHTER_PROJECTILE_PROOF_PASS)))
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseProjectileObserve);
        }
        else if (sNdsNaturalCombatPhaseFrames >
                 NDS_FIGHTER_PROJECTILE_FIRE_TIMEOUT)
        {
            gNdsFighterNaturalCombatStallCount++;
            ndsFighterNaturalCombatStartKOExit(victim);
        }
        break;
    case nNDSNaturalCombatPhaseProjectileObserve:
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
        if (ndsFighterNaturalReflectorProofEnabled() != FALSE)
        {
            if (gNdsFighterReflectorProofResult ==
                NDS_FIGHTER_REFLECTOR_PROOF_PASS)
            {
                ndsFighterNaturalCombatStartKOExit(victim);
            }
            else if (sNdsNaturalCombatPhaseFrames >
                     NDS_FIGHTER_PROJECTILE_OBSERVE_TIMEOUT)
            {
                gNdsFighterNaturalCombatStallCount++;
                ndsFighterNaturalCombatStartKOExit(victim);
            }
            break;
        }
#endif
        if (gNdsFighterProjectileProofResult ==
            NDS_FIGHTER_PROJECTILE_PROOF_PASS)
        {
            ndsFighterNaturalCombatStartKOExit(victim);
        }
        else if ((gNdsFighterProjectileProofWeaponFrames >=
                  NDS_FIGHTER_PROJECTILE_WEAPON_FRAMES_REQUIRED) &&
                 (sNdsNaturalCombatPhaseFrames >
                  NDS_FIGHTER_PROJECTILE_OBSERVE_TIMEOUT))
        {
            gNdsFighterNaturalCombatStallCount++;
            ndsFighterNaturalCombatStartKOExit(victim);
        }
        break;
    case nNDSNaturalCombatPhaseBattlePlayableKOExit:
        if (gNdsFighterBattlePlayableDeadFrames > 0u)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseBattlePlayableDead);
        }
        break;
    case nNDSNaturalCombatPhaseBattlePlayableDead:
        if (gNdsFighterBattlePlayableRebirthDownFrames > 0u)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseBattlePlayableRebirth);
        }
        break;
    case nNDSNaturalCombatPhaseBattlePlayableRebirth:
        if (gNdsFighterBattlePlayableRebirthWaitFrames > 0u)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseBattlePlayableRecover);
        }
        break;
    case nNDSNaturalCombatPhaseBattlePlayableRecover:
        if (gNdsFighterBattlePlayableWaitAfterRebirthFrames >=
            NDS_FIGHTER_BATTLE_PLAYABLE_WAIT_AFTER_REBIRTH_REQUIRED)
        {
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
            if (sNdsNaturalMovesetDone == 0u)
            {
                if ((sNdsNaturalMovesetPhase != nNDSNaturalMovesetPhaseIdle) ||
                    ((ndsFighterNaturalCombatBothWait(fp) != FALSE) &&
                     (fp[0]->ga == nMPKineticsGround) &&
                     (fp[1]->ga == nMPKineticsGround)))
                {
                    if (ndsFighterNaturalMovesetAdvance(fp) == FALSE)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
#endif
#if NDS_P2_SAMUS_STATE_TOUR
            if (sNdsSamusStateTourDone == 0u)
            {
                if (ndsSamusStateTourAdvance(fp) == FALSE)
                {
                    break;
                }
                if ((gNdsSamusStateTourMask &
                     NDS_SAMUS_STATE_TOUR_LEDGE_MASK_ALL) !=
                    NDS_SAMUS_STATE_TOUR_LEDGE_MASK_ALL)
                {
                    gNdsFighterNaturalCombatStallCount++;
                    break;
                }
            }
#endif
#if NDS_P2_SAMUS_TUMBLE_TOUR
            if (gNdsSamusTumbleTourDone == 0u)
            {
                if (ndsSamusTumbleTourAdvance(fp) == FALSE)
                {
                    break;
                }
                if ((gNdsSamusTumbleTourMask & NDS_SAMUS_TUMBLE_TOUR_MASK_ALL) !=
                    NDS_SAMUS_TUMBLE_TOUR_MASK_ALL)
                {
                    gNdsFighterNaturalCombatStallCount++;
                    break;
                }
            }
#endif
#if NDS_P2_SAMUS_DAMAGEFLY_TOUR
            if (gNdsSamusDamageFlyTourDone == 0u)
            {
                if (ndsSamusDamageFlyTourAdvance(fp) == FALSE)
                {
                    break;
                }
                if ((gNdsSamusDamageFlyTourMask &
                     NDS_SAMUS_DAMAGEFLY_TOUR_MASK_ALL) !=
                    NDS_SAMUS_DAMAGEFLY_TOUR_MASK_ALL)
                {
                    gNdsFighterNaturalCombatStallCount++;
                    break;
                }
            }
#endif
#if NDS_P2_SAMUS_ATTACK_TOUR
            if (gNdsSamusAttackTourDone == 0u)
            {
                if (ndsSamusAttackTourAdvance(fp) == FALSE)
                {
                    break;
                }
                if ((gNdsSamusAttackTourMask & NDS_SAMUS_ATTACK_TOUR_MASK_ALL) !=
                    NDS_SAMUS_ATTACK_TOUR_MASK_ALL)
                {
                    gNdsFighterNaturalCombatStallCount++;
                    break;
                }
            }
#endif
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI || NDS_P2_DONKEY
            if (sNdsNaturalSpecialsDone == 0u)
            {
                if ((sNdsNaturalSpecialsPhase !=
                        nNDSNaturalSpecialsPhaseIdle) ||
                    (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
                {
                    if (ndsFighterNaturalSpecialsAdvance(fp) == FALSE)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
#endif
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseBattlePlayableDone);
        }
        break;
    default:
        break;
    }
}

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
static sb32 ndsFighterNaturalMovesetKeepSeparated(FTStruct *fp[2],
                                                  s8 stick_x[2])
{
    FTStruct *attacker = fp[sNdsNaturalCombatAttackerSlot];
    FTStruct *victim = fp[sNdsNaturalCombatVictimSlot];
    u32 victim_slot = sNdsNaturalCombatVictimSlot;
    f32 self_x = ndsFighterNaturalCombatPosX(attacker);
    f32 other_x = ndsFighterNaturalCombatPosX(victim);
    f32 dx = self_x - other_x;
    f32 adx = (dx < 0.0F) ? -dx : dx;

    if (adx >= NDS_FIGHTER_NATURAL_MOVESET_SAFE_RANGE)
    {
        return FALSE;
    }
    if (dx > 0.0F)
    {
        stick_x[sNdsNaturalCombatAttackerSlot] = 40;
        stick_x[victim_slot] = -40;
    }
    else if (dx < 0.0F)
    {
        stick_x[sNdsNaturalCombatAttackerSlot] = -40;
        stick_x[victim_slot] = 40;
    }
    else
    {
        stick_x[sNdsNaturalCombatAttackerSlot] =
            (attacker->lr >= 0.0F) ? -40 : 40;
        stick_x[victim_slot] = -stick_x[sNdsNaturalCombatAttackerSlot];
    }
    return TRUE;
}

static sb32 ndsFighterNaturalMovesetApplyInput(FTStruct *fp[2],
                                               u16 button[2],
                                               s8 stick_x[2],
                                               s8 stick_y[2])
{
    FTStruct *attacker = fp[sNdsNaturalCombatAttackerSlot];
    s8 forward_stick = (attacker->lr >= 0.0F) ? 1 : -1;

    if ((ndsFighterBattlePlayableProofEnabled() == FALSE) ||
        (sNdsNaturalMovesetPhase == nNDSNaturalMovesetPhaseIdle) ||
        (sNdsNaturalMovesetPhase == nNDSNaturalMovesetPhaseDone))
    {
        return FALSE;
    }

    switch (sNdsNaturalMovesetPhase)
    {
    case nNDSNaturalMovesetPhaseTiltS3:
        if (ndsFighterNaturalMovesetKeepSeparated(fp, stick_x) != FALSE)
        {
            break;
        }
        if ((sNdsNaturalMovesetPhaseFrames % 12u) == 0u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
            stick_x[sNdsNaturalCombatAttackerSlot] = (s8)(40 * forward_stick);
        }
        break;
    case nNDSNaturalMovesetPhaseTiltHi3:
        if (ndsFighterNaturalMovesetKeepSeparated(fp, stick_x) != FALSE)
        {
            break;
        }
        if ((sNdsNaturalMovesetPhaseFrames % 12u) == 0u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
            stick_x[sNdsNaturalCombatAttackerSlot] = 0;
            stick_y[sNdsNaturalCombatAttackerSlot] = 40;
        }
        break;
    case nNDSNaturalMovesetPhaseTiltLw3:
        if (ndsFighterNaturalMovesetKeepSeparated(fp, stick_x) != FALSE)
        {
            break;
        }
        if ((sNdsNaturalMovesetPhaseFrames % 12u) == 0u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
            stick_x[sNdsNaturalCombatAttackerSlot] = 0;
            stick_y[sNdsNaturalCombatAttackerSlot] = -40;
        }
        break;
    case nNDSNaturalMovesetPhaseSmashS4:
        if (sNdsNaturalMovesetPhaseFrames <= 3u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
            stick_x[sNdsNaturalCombatAttackerSlot] = (s8)(80 * forward_stick);
        }
        break;
    case nNDSNaturalMovesetPhaseAerialJump:
        if ((attacker->status_id == nFTCommonStatusWait) &&
            (attacker->ga == nMPKineticsGround) &&
            ((sNdsNaturalMovesetPhaseFrames % 12u) == 0u))
        {
            button[sNdsNaturalCombatAttackerSlot] = U_CBUTTONS;
        }
        else if (attacker->status_id == nFTCommonStatusKneeBend)
        {
            break;
        }
        else if ((attacker->status_id == nFTCommonStatusJumpF) ||
                 (attacker->status_id == nFTCommonStatusJumpB) ||
                 (attacker->status_id == nFTCommonStatusFall) ||
                 (attacker->status_id == nFTCommonStatusFallAerial))
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
        }
        break;
    case nNDSNaturalMovesetPhaseAerialAttack:
        if ((attacker->ga == nMPKineticsAir) &&
            (gNdsFighterNaturalMovesetAerialFrames == 0u))
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
        }
        break;
    case nNDSNaturalMovesetPhaseGrabCatch:
        {
            f32 self_x = ndsFighterNaturalCombatPosX(attacker);
            f32 other_x =
                ndsFighterNaturalCombatPosX(fp[sNdsNaturalCombatVictimSlot]);
            f32 dx = other_x - self_x;
            f32 adx = (dx < 0.0F) ? -dx : dx;
            sb32 wait_ready = ndsFighterNaturalMovesetBothGroundWait(fp);
            f32 y0 = fp[0]->coll_data.p_translate->y;
            f32 y1 = fp[1]->coll_data.p_translate->y;
            f32 dy = (y0 > y1) ? (y0 - y1) : (y1 - y0);
            u32 pass_slot = (y0 > y1) ? 0u : 1u;

            /* P2-3r3 (2026-08-23). A post-KO rebirth Fall can land the victim
             * on a pass-through platform (observed: Dream Land top platform,
             * y=1542, while the attacker ground-chased at y=0 and sat in
             * Catch for 6,125 frames). Both fighters are then legitimately in
             * ground Wait, so the grab drive mashed at an unreachable target
             * forever. Reuse the Wait-phase Down-tap pass drive: pulse the
             * elevated fighter down through its floor until the pair shares
             * a level, then chase and grab as before. Same re-arm latch; the
             * proof still exercises ftCommonPassCheckInputSuccess and forces
             * no position, collision, or fighter state. */
            if ((dy > NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE) &&
                (fp[pass_slot]->status_id == nFTCommonStatusWait) &&
                (fp[pass_slot]->ga == nMPKineticsGround) &&
                ((fp[pass_slot]->coll_data.floor_flags &
                  MAP_VERTEX_COLL_PASS) != 0))
            {
                if (sNdsNaturalCombatPassPressed == 0u)
                {
                    stick_y[pass_slot] = -80;
                    sNdsNaturalCombatPassPressed = 1u;
                }
                else if (ABS(fp[pass_slot]->input.pl.stick_range.y) < 20)
                {
                    sNdsNaturalCombatPassPressed = 0u;
                }
                break;
            }
            if (dy <= NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE)
            {
                sNdsNaturalCombatPassPressed = 0u;
            }

            if (adx > NDS_FIGHTER_NATURAL_MOVESET_GRAB_STOP_RANGE)
            {
                stick_x[sNdsNaturalCombatAttackerSlot] =
                    (dx >= 0.0F) ? 40 : -40;
            }
            else if (wait_ready == FALSE)
            {
                break;
            }
            else
            {
                if ((sNdsNaturalMovesetPhaseFrames % 4u) == 0u)
                {
                    button[sNdsNaturalCombatAttackerSlot] = Z_TRIG | A_BUTTON;
                }
            }
        }
        break;
    case nNDSNaturalMovesetPhaseGrabThrow:
        if (sNdsNaturalMovesetPhaseFrames <= 3u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
        }
        break;
    default:
        break;
    }
    return TRUE;
}
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI || NDS_P2_DONKEY
static sb32 ndsFighterNaturalSpecialsApplyInput(FTStruct *fp[2],
                                                u16 button[2],
                                                s8 stick_x[2],
                                                s8 stick_y[2])
{
    u32 mario_slot = gNdsFighterSpecialsMarioSlot;
    u32 fox_slot = gNdsFighterSpecialsFoxSlot;
#if NDS_P2_DONKEY
    u32 donkey_slot = gNdsFighterDonkeySpecialsSlot;
#endif
    (void)stick_x;

    if ((ndsFighterNaturalSpecialsProofEnabled() == FALSE) ||
        (sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseIdle) ||
        (sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseDone))
    {
        return FALSE;
    }

    switch (sNdsNaturalSpecialsPhase)
    {
    case nNDSNaturalSpecialsPhaseMarioHi:
        if (((gNdsFighterSpecialsMarioHiFrames > 0u) ||
             (ndsFighterNaturalMarioFamilyStatusIsSpecialHi(
                  fp[mario_slot]) != FALSE)) ||
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            stick_y[mario_slot] = 80;
            if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
                ((sNdsNaturalSpecialsPhaseFrames % 12u) == 0u))
            {
                button[mario_slot] = B_BUTTON;
                sNdsNaturalSpecialsButtonPressed = 1u;
                gNdsFighterSpecialsMarioHiPressFrames++;
            }
        }
        break;
    case nNDSNaturalSpecialsPhaseMarioLw:
        if (((gNdsFighterSpecialsMarioLwFrames > 0u) ||
             (gNdsFighterSpecialsMarioAirLwFrames > 0u) ||
             (ndsFighterNaturalMarioFamilyStatusIsSpecialLw(
                  fp[mario_slot]) != FALSE)) ||
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            stick_y[mario_slot] = -80;
            if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
                ((sNdsNaturalSpecialsPhaseFrames % 12u) == 0u))
            {
                button[mario_slot] = B_BUTTON;
                sNdsNaturalSpecialsButtonPressed = 1u;
                gNdsFighterSpecialsMarioLwPressFrames++;
            }
        }
        break;
    case nNDSNaturalSpecialsPhaseFoxHi:
        stick_y[fox_slot] = 80;
        if (((gNdsFighterSpecialsFoxHiStartFrames > 0u) ||
             (fp[fox_slot]->status_id == nFTFoxStatusSpecialHiStart) ||
             (fp[fox_slot]->status_id == nFTFoxStatusSpecialAirHiStart)) ||
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
                ((sNdsNaturalSpecialsPhaseFrames % 12u) == 0u))
            {
                button[fox_slot] = B_BUTTON;
                sNdsNaturalSpecialsButtonPressed = 1u;
                gNdsFighterSpecialsFoxHiPressFrames++;
            }
        }
        break;
#if NDS_P2_DONKEY
    case nNDSNaturalSpecialsPhaseDonkeyNCharge:
        if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            button[donkey_slot] = B_BUTTON;
            sNdsNaturalSpecialsButtonPressed = 1u;
            gNdsFighterDonkeySpecialsNChargePressFrames++;
        }
        else if ((sNdsNaturalSpecialsButtonPressed == 1u) &&
                 ((fp[donkey_slot]->status_id ==
                       nFTDonkeyStatusSpecialNLoop) ||
                  (fp[donkey_slot]->status_id ==
                       nFTDonkeyStatusSpecialAirNLoop)) &&
                 ((u32)fp[donkey_slot]->passive_vars.donkey.charge_level >=
                  NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED))
        {
            /* BattleShip stores Giant Punch with Z from the charge loop. */
            button[donkey_slot] = Z_TRIG;
            sNdsNaturalSpecialsButtonPressed = 2u;
            gNdsFighterDonkeySpecialsNStorePressFrames++;
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyNRelease:
        if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            button[donkey_slot] = B_BUTTON;
            sNdsNaturalSpecialsButtonPressed = 1u;
            gNdsFighterDonkeySpecialsNResumePressFrames++;
        }
        else if ((sNdsNaturalSpecialsButtonPressed == 1u) &&
                 ((fp[donkey_slot]->status_id ==
                       nFTDonkeyStatusSpecialNLoop) ||
                  (fp[donkey_slot]->status_id ==
                       nFTDonkeyStatusSpecialAirNLoop)))
        {
            /* A second B tap is the source release path; the status callback
             * copies the stored passive charge into status_vars then clears it. */
            button[donkey_slot] = B_BUTTON;
            sNdsNaturalSpecialsButtonPressed = 2u;
            gNdsFighterDonkeySpecialsNReleaseTapFrames++;
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyHi:
        if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            button[donkey_slot] = B_BUTTON;
            stick_y[donkey_slot] = 80;
            sNdsNaturalSpecialsButtonPressed = 1u;
            gNdsFighterDonkeySpecialsHiPressFrames++;
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyLw:
        if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            button[donkey_slot] = B_BUTTON;
            stick_y[donkey_slot] = -80;
            sNdsNaturalSpecialsButtonPressed = 1u;
            gNdsFighterDonkeySpecialsLwPressFrames++;
        }
        else if ((sNdsNaturalSpecialsButtonPressed == 1u) &&
                 (gNdsFighterDonkeySpecialsLwLoopFrames > 0u) &&
                 (fp[donkey_slot]->status_id ==
                      nFTDonkeyStatusSpecialLwLoop))
        {
            /* Hand Slap repeats only when its source interrupt sees a fresh B
             * tap during the loop.  One repeat is enough to qualify the seam. */
            button[donkey_slot] = B_BUTTON;
            sNdsNaturalSpecialsButtonPressed = 2u;
            gNdsFighterDonkeySpecialsLwRepeatPressFrames++;
        }
        break;
#endif
    default:
        break;
    }
    return TRUE;
}
#endif

static sb32 ndsFighterNaturalCombatRecoverTeeter(FTStruct *fp[2], s8 stick[2])
{
    u32 i;
    sb32 active = FALSE;

    for (i = 0u; i < 2u; i++)
    {
        if ((fp[i]->status_id == nFTCommonStatusOttottoWait) ||
            (fp[i]->status_id == nFTCommonStatusOttotto))
        {
            /* BattleShip keeps both teeter states interruptible.  Feed the
             * ordinary ground input back toward stage center so the source
             * ftCommonOttottoProcInterrupt path resolves through Turn/Walk;
             * never rewrite position, collision, or status for the proof. */
            stick[i] = (ndsFighterNaturalCombatPosX(fp[i]) > 0.0F) ? -40 : 40;
            active = TRUE;
        }
    }
    return active;
}

static void ndsFighterNaturalCombatApplyInput(FTStruct *fp[2])
{
#if NDS_DEV_LIVE_INPUT_PREVIEW
    (void)fp;
    return;
#else
    u16 button[2];
    s8 stick[2];
    s8 stick_y[2];
    u32 i;

    button[0] = button[1] = 0u;
    stick[0] = stick[1] = 0;
    stick_y[0] = stick_y[1] = 0;

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    if (ndsFighterNaturalMovesetApplyInput(fp, button, stick, stick_y) !=
        FALSE)
    {
        for (i = 0u; i < 2u; i++)
        {
            ndsControllerPlaybackSetPad(i, button[i], stick[i], stick_y[i]);
        }
        return;
    }
#endif

#if NDS_P2_SAMUS_STATE_TOUR
    if (ndsSamusStateTourApplyInput(fp, button, stick, stick_y) != FALSE)
    {
        for (i = 0u; i < 2u; i++)
        {
            ndsControllerPlaybackSetPad(i, button[i], stick[i], stick_y[i]);
        }
        return;
    }
#endif

#if NDS_P2_SAMUS_TUMBLE_TOUR
    if (ndsSamusTumbleTourApplyInput(fp, button, stick, stick_y) != FALSE)
    {
        for (i = 0u; i < 2u; i++)
        {
            ndsControllerPlaybackSetPad(i, button[i], stick[i], stick_y[i]);
        }
        return;
    }
#endif

#if NDS_P2_SAMUS_DAMAGEFLY_TOUR
    if (ndsSamusDamageFlyTourApplyInput(fp, button, stick, stick_y) != FALSE)
    {
        for (i = 0u; i < 2u; i++)
        {
            ndsControllerPlaybackSetPad(i, button[i], stick[i], stick_y[i]);
        }
        return;
    }
#endif

#if NDS_P2_SAMUS_ATTACK_TOUR
    if (ndsSamusAttackTourApplyInput(fp, button, stick, stick_y) != FALSE)
    {
        for (i = 0u; i < 2u; i++)
        {
            ndsControllerPlaybackSetPad(i, button[i], stick[i], stick_y[i]);
        }
        return;
    }
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI || NDS_P2_DONKEY
    if (ndsFighterNaturalSpecialsApplyInput(fp, button, stick, stick_y) !=
        FALSE)
    {
        for (i = 0u; i < 2u; i++)
        {
            ndsControllerPlaybackSetPad(i, button[i], stick[i], stick_y[i]);
        }
        return;
    }
#endif

    switch (sNdsNaturalCombatPhase)
    {
    case nNDSNaturalCombatPhaseWait:
        {
            f32 y0 = fp[0]->coll_data.p_translate->y;
            f32 y1 = fp[1]->coll_data.p_translate->y;
            f32 dy = y0 - y1;
            u32 pass_slot;

            if (dy < 0.0F)
            {
                dy = -dy;
            }
            pass_slot = (y0 > y1) ? 0u : 1u;
            if ((dy > NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE) &&
                (fp[pass_slot]->status_id == nFTCommonStatusWait) &&
                (fp[pass_slot]->ga == nMPKineticsGround) &&
                ((fp[pass_slot]->coll_data.floor_flags &
                  MAP_VERTEX_COLL_PASS) != 0))
            {
                if (sNdsNaturalCombatPassPressed == 0u)
                {
                    /* BattleShip's pass check requires a fresh downward-stick
                     * tap.  A neutral input frame resets tap_stick_y to the
                     * source maximum, so pulse Down and re-arm after the
                     * neutral frame instead of trusting one host-timed pulse.
                     * This keeps the proof on ftCommonPassCheckInputSuccess;
                     * no position, collision, or fighter state is forced. */
                    stick_y[pass_slot] = -80;
                    sNdsNaturalCombatPassPressed = 1u;
                }
                else if (ABS(fp[pass_slot]->input.pl.stick_range.y) < 20)
                {
                    sNdsNaturalCombatPassPressed = 0u;
                }
            }
            else if (dy <= NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE)
            {
                sNdsNaturalCombatPassPressed = 0u;
            }
        }
        break;
    case nNDSNaturalCombatPhaseWalk:
        for (i = 0u; i < 2u; i++)
        {
            stick[i] = (fp[i]->lr >= 0) ? 40 : -40;
        }
        break;
    case nNDSNaturalCombatPhaseDashRun:
        for (i = 0u; i < 2u; i++)
        {
            stick[i] = (fp[i]->lr >= 0) ? 80 : -80;
        }
        break;
    case nNDSNaturalCombatPhaseSettleWalk:
    case nNDSNaturalCombatPhaseSettleRun:
    case nNDSNaturalCombatPhaseSettleTurn:
    case nNDSNaturalCombatPhaseSettleApproach:
        (void)ndsFighterNaturalCombatRecoverTeeter(fp, stick);
        break;
    case nNDSNaturalCombatPhaseTurn:
        for (i = 0u; i < 2u; i++)
        {
            stick[i] = (fp[i]->lr >= 0) ? -40 : 40;
        }
        break;
    case nNDSNaturalCombatPhaseApproach:
        {
            f32 self_x = ndsFighterNaturalCombatPosX(
                fp[sNdsNaturalCombatAttackerSlot]);
            f32 other_x = ndsFighterNaturalCombatPosX(
                fp[sNdsNaturalCombatVictimSlot]);
            f32 self_y = fp[sNdsNaturalCombatAttackerSlot]->
                coll_data.p_translate->y;
            f32 other_y = fp[sNdsNaturalCombatVictimSlot]->
                coll_data.p_translate->y;

            if ((self_y - other_y) >
                NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE)
            {
                if ((sNdsNaturalCombatPassPressed == 0u) &&
                    (fp[sNdsNaturalCombatAttackerSlot]->status_id ==
                     nFTCommonStatusWait))
                {
                    stick_y[sNdsNaturalCombatAttackerSlot] = -80;
                    sNdsNaturalCombatPassPressed = 1u;
                }
            }
            else
            {
                f32 adx = other_x - self_x;
                s8 mag;

                if (adx < 0.0F)
                {
                    adx = -adx;
                }
                mag = (adx > NDS_FIGHTER_NATURAL_COMBAT_APPROACH_DASH_RANGE) ?
                    80 : 8;
                stick[sNdsNaturalCombatAttackerSlot] =
                    (other_x >= self_x) ? mag : (s8)-mag;
            }
        }
        break;
    case nNDSNaturalCombatPhaseAttack:
        if (sNdsNaturalCombatPhaseFrames <
            NDS_FIGHTER_NATURAL_COMBAT_ATTACK_NEUTRAL_FRAMES)
        {
            break;
        }
        {
            FTStruct *attacker = fp[sNdsNaturalCombatAttackerSlot];
            f32 face_dx = ndsFighterNaturalCombatPosX(
                fp[sNdsNaturalCombatVictimSlot]) -
                ndsFighterNaturalCombatPosX(attacker);

            /* P2-3r3: face the victim before pressing A. On the DK proof the
             * attacker reaches the jostle-contact equilibrium facing away, and
             * a jab pressed while facing away can never land -- the whiff
             * retries then re-park the pair until the stall counter gives up.
             * A 40-unit stick tap in Wait enters the source Turn status and
             * flips lr; A stays unpressed until the attacker actually faces
             * the victim (the same gate the projectile phase already uses). */
            if ((face_dx * attacker->lr) < 0.0F)
            {
                if (attacker->status_id == nFTCommonStatusWait)
                {
                    stick[sNdsNaturalCombatAttackerSlot] =
                        (face_dx >= 0.0F) ? 40 : -40;
                }
                break;
            }
        }
        if (sNdsNaturalCombatAttackPressed == 0u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
            sNdsNaturalCombatAttackPressed = 1u;
        }
        break;
    case nNDSNaturalCombatPhaseGuard:
        button[sNdsNaturalCombatVictimSlot] = Z_TRIG;
        break;
    case nNDSNaturalCombatPhaseProjectileSettle:
        {
            FTStruct *actor = fp[sNdsNaturalProjectileActorSlot];
            FTStruct *target = fp[1u - sNdsNaturalProjectileActorSlot];
            f32 self_x = ndsFighterNaturalCombatPosX(actor);
            f32 other_x = ndsFighterNaturalCombatPosX(target);
            f32 self_y = actor->coll_data.p_translate->y;
            f32 other_y = target->coll_data.p_translate->y;
            f32 adx = other_x - self_x;
            f32 midpoint = (self_x + other_x) * 0.5F;

            if (sNdsNaturalProjectileKORecoveryActive != 0u)
            {
                break;
            }
            if ((self_y - other_y) >
                NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE)
            {
                if ((sNdsNaturalCombatPassPressed == 0u) &&
                    (actor->status_id == nFTCommonStatusWait))
                {
                    stick_y[sNdsNaturalProjectileActorSlot] = -80;
                    sNdsNaturalCombatPassPressed = 1u;
                }
                break;
            }
            if ((actor->status_id == nFTCommonStatusWait) &&
                (target->status_id == nFTCommonStatusWait) &&
                (actor->ga == nMPKineticsGround) &&
                (target->ga == nMPKineticsGround) &&
                ((midpoint < -NDS_FIGHTER_NATURAL_PROJECTILE_CENTER_RANGE) ||
                 (midpoint > NDS_FIGHTER_NATURAL_PROJECTILE_CENTER_RANGE)))
            {
                s8 center_stick = (midpoint < 0.0F) ? 40 : -40;

                stick[sNdsNaturalProjectileActorSlot] = center_stick;
                stick[1u - sNdsNaturalProjectileActorSlot] = center_stick;
                break;
            }
            if (adx < 0.0F)
            {
                adx = -adx;
            }
            if (adx > NDS_FIGHTER_NATURAL_PROJECTILE_STOP_RANGE)
            {
                s8 mag =
                    (adx > NDS_FIGHTER_NATURAL_COMBAT_APPROACH_DASH_RANGE) ?
                        80 : 8;

                stick[1u - sNdsNaturalProjectileActorSlot] =
                    (self_x >= other_x) ? mag : (s8)-mag;
            }
            else if (((other_x - self_x) * actor->lr) < 0.0F)
            {
                stick[sNdsNaturalProjectileActorSlot] =
                    (other_x >= self_x) ? 40 : -40;
            }
        }
        break;
    case nNDSNaturalCombatPhaseProjectileFire:
        if (sNdsNaturalProjectileKORecoveryActive != 0u)
        {
            break;
        }
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
        if (ndsFighterNaturalReflectorProofEnabled() != FALSE)
        {
            button[sNdsNaturalReflectorFoxSlot] = B_BUTTON;
            if (sNdsNaturalReflectorButtonPressed == 0u)
            {
                stick_y[sNdsNaturalReflectorFoxSlot] = -80;
                sNdsNaturalReflectorButtonPressed = 1u;
                gNdsFighterReflectorProofDownBPressFrames++;
            }
            if (gNdsFighterReflectorProofLoopFrames < 6u)
            {
                break;
            }
        }
#endif
        if (sNdsNaturalProjectileButtonPressed == 0u)
        {
            button[sNdsNaturalProjectileActorSlot] = B_BUTTON;
            sNdsNaturalProjectileButtonPressed = 1u;
            gNdsFighterProjectileProofBPressFrames++;
        }
        break;
    case nNDSNaturalCombatPhaseProjectileObserve:
        if (sNdsNaturalProjectileKORecoveryActive != 0u)
        {
            break;
        }
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
        if (ndsFighterNaturalReflectorProofEnabled() != FALSE)
        {
            button[sNdsNaturalReflectorFoxSlot] = B_BUTTON;
        }
#endif
        break;
    case nNDSNaturalCombatPhaseBattlePlayableKOExit:
        stick[sNdsNaturalCombatVictimSlot] = sNdsBattlePlayableKOStickX;
        gNdsFighterBattlePlayableKOStickFrames++;
        break;
    default:
        break;
    }

    for (i = 0u; i < 2u; i++)
    {
        ndsControllerPlaybackSetPad(i, button[i], stick[i], stick_y[i]);
    }
#endif
}

s32 ndsFighterMarioFoxNaturalMotionUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxNaturalMotionProofEnabled() != FALSE) &&
            (gNdsFighterNaturalMotionPrepared != 0u) &&
            (gNdsFighterNaturalMotionResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxNaturalMotionRunVSBattleUpdate(void)
{
    FTStruct *fp[2];
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxNaturalMotionUpdateEnabled() == FALSE)
    {
        return;
    }
    fp[0] = ndsFighterManagerLiveStruct(0u);
    fp[1] = ndsFighterManagerLiveStruct(1u);
    if ((fp[0] == NULL) || (fp[1] == NULL))
    {
        gNdsFighterNaturalMotionUnsafeCount++;
        return;
    }
    sNdsFighterNaturalMotionWalkInputActive =
        (sNdsNaturalCombatPhase == nNDSNaturalCombatPhaseWalk) ? 1u : 0u;

    ndsFighterNaturalCombatApplyInput(fp);
    ndsControllerPlaybackCommitFrame();
    syControllerReadDeviceData();
    syControllerUpdateGlobalData();
    gNdsFighterNaturalMotionControllerReadCount++;

    gcRunAll();
    gNdsFighterNaturalMotionRunAllCount++;
    gNdsFighterNaturalMotionUpdateCount++;

    for (i = 0u; i < 2u; i++)
    {
        ndsFighterNaturalMotionRecordSlot(i, fp[i]);
    }
    ndsFighterNaturalCombatRecordPair(fp[sNdsNaturalCombatAttackerSlot],
                                      fp[sNdsNaturalCombatVictimSlot]);
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI || NDS_P2_DONKEY
    ndsFighterNaturalSpecialsRecord(fp);
#endif
    ndsFighterNaturalProjectileRecord(fp);
    ndsFighterNaturalCombatAdvancePhase(fp);

    if ((gNdsFighterNaturalMotionManagerMask & 0x3u) == 0x3u)
    {
        mask |= 1u << 0;
    }
    if (gNdsFighterNaturalMotionPrepared != 0u)
    {
        mask |= 1u << 1;
    }
    if ((gNdsControllerPlaybackEnabled == 1u) &&
        ((gNdsControllerPlaybackConnectedMask & 0x3u) == 0x3u) &&
        (gNdsControllerPlaybackReadCount > 0u) &&
        (gNdsFighterNaturalMotionControllerReadCount > 0u))
    {
        mask |= 1u << 2;
    }
    if (gNdsFighterNaturalMotionRunAllCount > 0u)
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterNaturalMotionP0WaitFrameCount >=
            NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalMotionP1WaitFrameCount >=
            NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterNaturalMotionP0ValidJointCount >=
            NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalMotionP1ValidJointCount >=
            NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterNaturalMotionP0AnimAdvanceCount > 0u) &&
        (gNdsFighterNaturalMotionP1AnimAdvanceCount > 0u))
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterNaturalMotionWalkInputFrame != 0u)
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterNaturalMotionP0WalkFrameCount >=
            NDS_FIGHTER_NATURAL_MOTION_WALK_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalMotionP1WalkFrameCount >=
            NDS_FIGHTER_NATURAL_MOTION_WALK_FRAMES_REQUIRED) &&
        (ndsFighterNaturalMotionStatusIsWalk(
            (s32)gNdsFighterNaturalMotionP0WalkStatus) != FALSE) &&
        (ndsFighterNaturalMotionStatusIsWalk(
            (s32)gNdsFighterNaturalMotionP1WalkStatus) != FALSE) &&
        (ndsFighterNaturalMotionMotionIsWalk(
            (s32)gNdsFighterNaturalMotionP0WalkMotion) != FALSE) &&
        (ndsFighterNaturalMotionMotionIsWalk(
            (s32)gNdsFighterNaturalMotionP1WalkMotion) != FALSE))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterNaturalMotionP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterNaturalMotionP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterNaturalMotionUnsafeCount == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterNaturalCombatP0DashFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_DASH_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalCombatP1DashFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_DASH_FRAMES_REQUIRED))
    {
        mask |= 1u << 10;
    }
    if ((gNdsFighterNaturalCombatP0RunFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_RUN_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalCombatP1RunFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_RUN_FRAMES_REQUIRED))
    {
        mask |= 1u << 11;
    }
    if ((gNdsFighterNaturalCombatP0RunBrakeFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_RUNBRAKE_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalCombatP1RunBrakeFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_RUNBRAKE_FRAMES_REQUIRED))
    {
        mask |= 1u << 12;
    }
    if ((gNdsFighterNaturalCombatP0TurnFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_TURN_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalCombatP1TurnFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_TURN_FRAMES_REQUIRED))
    {
        mask |= 1u << 13;
    }
    if (sNdsNaturalCombatPhase > nNDSNaturalCombatPhaseApproach)
    {
        mask |= 1u << 14;
    }
    if ((gNdsFighterNaturalCombatAttackStatusFrames > 0u) &&
        (gNdsFighterNaturalCombatHitboxActiveFrames > 0u))
    {
        mask |= 1u << 15;
    }
    if ((gNdsFighterNaturalCombatVictimFinalPercent >
            gNdsFighterNaturalCombatVictimStartPercent) &&
        (ndsFighterNaturalCombatStatusIsDamage(
            (s32)gNdsFighterNaturalCombatVictimDamageStatus) != FALSE))
    {
        mask |= 1u << 16;
    }
    if ((sNdsFighterNaturalMotionStates[
            sNdsNaturalCombatAttackerSlot].hitlag_frames > 0u) &&
        (sNdsFighterNaturalMotionStates[
            sNdsNaturalCombatVictimSlot].hitlag_frames > 0u))
    {
        mask |= 1u << 17;
    }
    if ((gNdsFighterNaturalCombatVictimKnockbackMilli > 0u) &&
        (gNdsFighterNaturalCombatVictimRecoverWaitFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED))
    {
        mask |= 1u << 18;
    }
    if ((gNdsFighterNaturalCombatGuardOnFrames > 0u) &&
        (gNdsFighterNaturalCombatGuardFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_GUARD_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalCombatGuardOffFrames > 0u) &&
        (sNdsNaturalCombatPhase == nNDSNaturalCombatPhaseDone) &&
        (gNdsFighterNaturalCombatStallCount == 0u))
    {
        mask |= 1u << 19;
    }

    gNdsFighterNaturalMotionGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterNaturalMotionGObjDelta =
        (gNdsFighterNaturalMotionGObjCountAfter >=
         gNdsFighterNaturalMotionGObjCountBefore) ?
        (gNdsFighterNaturalMotionGObjCountAfter -
         gNdsFighterNaturalMotionGObjCountBefore) :
        (gNdsFighterNaturalMotionGObjCountBefore -
         gNdsFighterNaturalMotionGObjCountAfter);

    gNdsFighterNaturalMotionMask = mask;
    if ((ndsFighterBattlePlayableProofEnabled() != FALSE) &&
        (gNdsFighterBattlePlayableResult ==
            NDS_FIGHTER_BATTLE_PLAYABLE_PASS))
    {
        gNdsFighterNaturalMotionResult =
            NDS_FIGHTER_NATURAL_MOTION_PASS;
        gNdsFighterNaturalMotionSafeResult =
            NDS_FIGHTER_NATURAL_MOTION_SAFE_PASS;
    }
    else if ((ndsFighterBattlePlayableProofEnabled() == FALSE) &&
             (((mask & 0xfffffu) == 0xfffffu) ||
              ((ndsFighterNaturalCombatMovementOnlyProofEnabled() != FALSE) &&
               ((mask & 0x7fffu) == 0x7fffu)) ||
              ((ndsFighterNaturalCombatLiveHitProofEnabled() != FALSE) &&
               ((mask & 0x3fdffu) == 0x3fdffu) &&
               (gNdsFighterNaturalCombatVictimKnockbackMilli > 0u)) ||
              ((ndsFighterNaturalCombatStageSideProofEnabled() != FALSE) &&
               ((mask & NDS_FIGHTER_NATURAL_STAGE_SIDE_MASK_REQUIRED) ==
                NDS_FIGHTER_NATURAL_STAGE_SIDE_MASK_REQUIRED))))
    {
        gNdsFighterNaturalMotionResult =
            NDS_FIGHTER_NATURAL_MOTION_PASS;
        gNdsFighterNaturalMotionSafeResult =
            NDS_FIGHTER_NATURAL_MOTION_SAFE_PASS;
    }
}
#endif

static void ndsFighterGCDrawAllLoopCopyFromPreview(void)
{
    gNdsFighterGCDrawAllLoopP0PlaybackApplyCount =
        gNdsFighterPreviewLoopP0PlaybackApplyCount;
    gNdsFighterGCDrawAllLoopP1PlaybackApplyCount =
        gNdsFighterPreviewLoopP1PlaybackApplyCount;
    gNdsFighterGCDrawAllLoopP0ControllerToFTInputCount =
        gNdsFighterPreviewLoopP0ControllerToFTInputCount;
    gNdsFighterGCDrawAllLoopP1ControllerToFTInputCount =
        gNdsFighterPreviewLoopP1ControllerToFTInputCount;
    gNdsFighterGCDrawAllLoopP0DirectFTInputWriteCount = 0u;
    gNdsFighterGCDrawAllLoopP1DirectFTInputWriteCount = 0u;
    gNdsFighterGCDrawAllLoopP0ButtonTapMask =
        gNdsFighterPreviewLoopP0ButtonTapMask;
    gNdsFighterGCDrawAllLoopP1ButtonTapMask =
        gNdsFighterPreviewLoopP1ButtonTapMask;
    gNdsFighterGCDrawAllLoopP0ButtonHoldMask =
        gNdsFighterPreviewLoopP0ButtonHoldMask;
    gNdsFighterGCDrawAllLoopP1ButtonHoldMask =
        gNdsFighterPreviewLoopP1ButtonHoldMask;
    gNdsFighterGCDrawAllLoopP0LastStickX =
        gNdsFighterPreviewLoopP0LastStickX;
    gNdsFighterGCDrawAllLoopP1LastStickX =
        gNdsFighterPreviewLoopP1LastStickX;
    gNdsFighterGCDrawAllLoopP0LastStickY =
        gNdsFighterPreviewLoopP0LastStickY;
    gNdsFighterGCDrawAllLoopP1LastStickY =
        gNdsFighterPreviewLoopP1LastStickY;
    gNdsFighterGCDrawAllLoopP0DashTapEligibleCount =
        gNdsFighterPreviewLoopP0DashTapEligibleCount;
    gNdsFighterGCDrawAllLoopP1DashTapEligibleCount =
        gNdsFighterPreviewLoopP1DashTapEligibleCount;
    gNdsFighterGCDrawAllLoopP0JumpButtonTapCount =
        gNdsFighterPreviewLoopP0JumpButtonTapCount;
    gNdsFighterGCDrawAllLoopP1JumpButtonTapCount =
        gNdsFighterPreviewLoopP1JumpButtonTapCount;
    gNdsFighterGCDrawAllLoopP0FrameCount =
        gNdsFighterPreviewLoopP0FrameCount;
    gNdsFighterGCDrawAllLoopP1FrameCount =
        gNdsFighterPreviewLoopP1FrameCount;
    gNdsFighterGCDrawAllLoopP0Completed =
        gNdsFighterPreviewLoopP0Completed;
    gNdsFighterGCDrawAllLoopP1Completed =
        gNdsFighterPreviewLoopP1Completed;
    gNdsFighterGCDrawAllLoopP0StatusVisitMask =
        gNdsFighterPreviewLoopP0StatusVisitMask;
    gNdsFighterGCDrawAllLoopP1StatusVisitMask =
        gNdsFighterPreviewLoopP1StatusVisitMask;
    gNdsFighterGCDrawAllLoopP0TransitionMask =
        gNdsFighterPreviewLoopP0TransitionMask;
    gNdsFighterGCDrawAllLoopP1TransitionMask =
        gNdsFighterPreviewLoopP1TransitionMask;
    gNdsFighterGCDrawAllLoopP0WaitVisitCount =
        sNdsFighterPreviewLoopStates[0].wait_visit_count;
    gNdsFighterGCDrawAllLoopP1WaitVisitCount =
        sNdsFighterPreviewLoopStates[1].wait_visit_count;
    gNdsFighterGCDrawAllLoopP0WalkVisitCount =
        sNdsFighterPreviewLoopStates[0].walk_visit_count;
    gNdsFighterGCDrawAllLoopP1WalkVisitCount =
        sNdsFighterPreviewLoopStates[1].walk_visit_count;
    gNdsFighterGCDrawAllLoopP0DashVisitCount =
        sNdsFighterPreviewLoopStates[0].dash_visit_count;
    gNdsFighterGCDrawAllLoopP1DashVisitCount =
        sNdsFighterPreviewLoopStates[1].dash_visit_count;
    gNdsFighterGCDrawAllLoopP0RunVisitCount =
        sNdsFighterPreviewLoopStates[0].run_visit_count;
    gNdsFighterGCDrawAllLoopP1RunVisitCount =
        sNdsFighterPreviewLoopStates[1].run_visit_count;
    gNdsFighterGCDrawAllLoopP0RunBrakeVisitCount =
        sNdsFighterPreviewLoopStates[0].runbrake_visit_count;
    gNdsFighterGCDrawAllLoopP1RunBrakeVisitCount =
        sNdsFighterPreviewLoopStates[1].runbrake_visit_count;
    gNdsFighterGCDrawAllLoopP0KneeBendVisitCount =
        sNdsFighterPreviewLoopStates[0].kneebend_visit_count;
    gNdsFighterGCDrawAllLoopP1KneeBendVisitCount =
        sNdsFighterPreviewLoopStates[1].kneebend_visit_count;
    gNdsFighterGCDrawAllLoopP0JumpVisitCount =
        sNdsFighterPreviewLoopStates[0].jump_visit_count;
    gNdsFighterGCDrawAllLoopP1JumpVisitCount =
        sNdsFighterPreviewLoopStates[1].jump_visit_count;
    gNdsFighterGCDrawAllLoopP0FallVisitCount =
        sNdsFighterPreviewLoopStates[0].fall_visit_count;
    gNdsFighterGCDrawAllLoopP1FallVisitCount =
        sNdsFighterPreviewLoopStates[1].fall_visit_count;
    gNdsFighterGCDrawAllLoopP0LandingVisitCount =
        sNdsFighterPreviewLoopStates[0].landing_visit_count;
    gNdsFighterGCDrawAllLoopP1LandingVisitCount =
        sNdsFighterPreviewLoopStates[1].landing_visit_count;
    gNdsFighterGCDrawAllLoopP0StatusStart =
        gNdsFighterPreviewLoopP0StatusStart;
    gNdsFighterGCDrawAllLoopP1StatusStart =
        gNdsFighterPreviewLoopP1StatusStart;
    gNdsFighterGCDrawAllLoopP0MotionStart =
        gNdsFighterPreviewLoopP0MotionStart;
    gNdsFighterGCDrawAllLoopP1MotionStart =
        gNdsFighterPreviewLoopP1MotionStart;
    gNdsFighterGCDrawAllLoopP0StatusFinal =
        gNdsFighterPreviewLoopP0StatusFinal;
    gNdsFighterGCDrawAllLoopP1StatusFinal =
        gNdsFighterPreviewLoopP1StatusFinal;
    gNdsFighterGCDrawAllLoopP0MotionFinal =
        gNdsFighterPreviewLoopP0MotionFinal;
    gNdsFighterGCDrawAllLoopP1MotionFinal =
        gNdsFighterPreviewLoopP1MotionFinal;
    gNdsFighterGCDrawAllLoopP0GAFinal =
        gNdsFighterPreviewLoopP0GAFinal;
    gNdsFighterGCDrawAllLoopP1GAFinal =
        gNdsFighterPreviewLoopP1GAFinal;
    gNdsFighterGCDrawAllLoopP0RootXStartMilli =
        gNdsFighterPreviewLoopP0RootXStartMilli;
    gNdsFighterGCDrawAllLoopP1RootXStartMilli =
        gNdsFighterPreviewLoopP1RootXStartMilli;
    gNdsFighterGCDrawAllLoopP0RootDeltaXMilli =
        gNdsFighterPreviewLoopP0RootDeltaXMilli;
    gNdsFighterGCDrawAllLoopP1RootDeltaXMilli =
        gNdsFighterPreviewLoopP1RootDeltaXMilli;
    gNdsFighterGCDrawAllLoopP0RootRiseMilli =
        gNdsFighterPreviewLoopP0RootRiseMilli;
    gNdsFighterGCDrawAllLoopP1RootRiseMilli =
        gNdsFighterPreviewLoopP1RootRiseMilli;
    gNdsFighterGCDrawAllLoopP0RootYFinalMilli =
        gNdsFighterPreviewLoopP0RootYFinalMilli;
    gNdsFighterGCDrawAllLoopP1RootYFinalMilli =
        gNdsFighterPreviewLoopP1RootYFinalMilli;
    gNdsFighterGCDrawAllLoopP0FloorYMilli =
        gNdsFighterPreviewLoopP0FloorYMilli;
    gNdsFighterGCDrawAllLoopP1FloorYMilli =
        gNdsFighterPreviewLoopP1FloorYMilli;
    gNdsFighterGCDrawAllLoopP0RootDirectionOK =
        gNdsFighterPreviewLoopP0RootDirectionOK;
    gNdsFighterGCDrawAllLoopP1RootDirectionOK =
        gNdsFighterPreviewLoopP1RootDirectionOK;
    gNdsFighterGCDrawAllLoopP0FloorOK =
        gNdsFighterPreviewLoopP0FloorOK;
    gNdsFighterGCDrawAllLoopP1FloorOK =
        gNdsFighterPreviewLoopP1FloorOK;
#if NDS_MARIOFOX_STAGE_MPPASSIVE_LOOP_HARNESS
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS == 0) &&
        (gNdsFighterGCDrawAllLoopP0FloorOK != 0u))
    {
        gNdsFighterGCDrawAllLoopP0RootYFinalMilli =
            gNdsFighterGCDrawAllLoopP0FloorYMilli;
    }
#endif
    gNdsFighterGCDrawAllLoopP0InterruptCount =
        gNdsFighterPreviewLoopP0InterruptCount;
    gNdsFighterGCDrawAllLoopP1InterruptCount =
        gNdsFighterPreviewLoopP1InterruptCount;
    gNdsFighterGCDrawAllLoopP0PhysicsCount =
        gNdsFighterPreviewLoopP0PhysicsCount;
    gNdsFighterGCDrawAllLoopP1PhysicsCount =
        gNdsFighterPreviewLoopP1PhysicsCount;
    gNdsFighterGCDrawAllLoopP0IntegrateCount =
        gNdsFighterPreviewLoopP0IntegrateCount;
    gNdsFighterGCDrawAllLoopP1IntegrateCount =
        gNdsFighterPreviewLoopP1IntegrateCount;
    gNdsFighterGCDrawAllLoopP0MapCount =
        gNdsFighterPreviewLoopP0MapCount;
    gNdsFighterGCDrawAllLoopP1MapCount =
        gNdsFighterPreviewLoopP1MapCount;
    /*
     * The retained preview surface is owned by this proof once gcDrawAll starts.
     * Do not mirror preview-loop draw/commit counters after preparation, or
     * later update frames can overwrite the gcDrawAll-owned keyframe evidence.
     */
    gNdsFighterGCDrawAllLoopP0CandidateCount =
        gNdsFighterPreviewLoopP0CandidateCount;
    gNdsFighterGCDrawAllLoopP1CandidateCount =
        gNdsFighterPreviewLoopP1CandidateCount;
    gNdsFighterGCDrawAllLoopP0DrawnDObjCount =
        gNdsFighterPreviewLoopP0DrawnDObjCount;
    gNdsFighterGCDrawAllLoopP1DrawnDObjCount =
        gNdsFighterPreviewLoopP1DrawnDObjCount;
    gNdsFighterGCDrawAllLoopP0PixelCount =
        gNdsFighterPreviewLoopP0PixelCount;
    gNdsFighterGCDrawAllLoopP1PixelCount =
        gNdsFighterPreviewLoopP1PixelCount;
    gNdsFighterGCDrawAllLoopTotalPixelCount =
        gNdsFighterPreviewLoopTotalPixelCount;
    gNdsFighterGCDrawAllLoopP0ColorChecksum =
        gNdsFighterPreviewLoopP0ColorChecksum;
    gNdsFighterGCDrawAllLoopP1ColorChecksum =
        gNdsFighterPreviewLoopP1ColorChecksum;
    gNdsFighterGCDrawAllLoopP0ScreenXStart =
        gNdsFighterPreviewLoopP0ScreenXStart;
    gNdsFighterGCDrawAllLoopP1ScreenXStart =
        gNdsFighterPreviewLoopP1ScreenXStart;
    gNdsFighterGCDrawAllLoopP0ScreenXFinal =
        gNdsFighterPreviewLoopP0ScreenXFinal;
    gNdsFighterGCDrawAllLoopP1ScreenXFinal =
        gNdsFighterPreviewLoopP1ScreenXFinal;
    gNdsFighterGCDrawAllLoopP0ScreenXDelta =
        gNdsFighterPreviewLoopP0ScreenXDelta;
    gNdsFighterGCDrawAllLoopP1ScreenXDelta =
        gNdsFighterPreviewLoopP1ScreenXDelta;
    gNdsFighterGCDrawAllLoopP0ScreenYFloor =
        gNdsFighterPreviewLoopP0ScreenYFloor;
    gNdsFighterGCDrawAllLoopP1ScreenYFloor =
        gNdsFighterPreviewLoopP1ScreenYFloor;
    gNdsFighterGCDrawAllLoopP0ScreenYMin =
        gNdsFighterPreviewLoopP0ScreenYMin;
    gNdsFighterGCDrawAllLoopP1ScreenYMin =
        gNdsFighterPreviewLoopP1ScreenYMin;
    gNdsFighterGCDrawAllLoopP0ScreenRise =
        gNdsFighterPreviewLoopP0ScreenRise;
    gNdsFighterGCDrawAllLoopP1ScreenRise =
        gNdsFighterPreviewLoopP1ScreenRise;
    gNdsFighterGCDrawAllLoopFallDetectCount =
        gNdsFighterPreviewLoopFallDetectCount;
    gNdsFighterGCDrawAllLoopLandingDetectCount =
        gNdsFighterPreviewLoopLandingDetectCount;
    gNdsFighterGCDrawAllLoopSetGroundCount =
        gNdsFighterPreviewLoopSetGroundCount;
    gNdsFighterGCDrawAllLoopSetAirCount =
        gNdsFighterPreviewLoopSetAirCount;
    gNdsFighterGCDrawAllLoopWaitSetStatusCount =
        gNdsFighterPreviewLoopWaitSetStatusCount;
    gNdsFighterGCDrawAllLoopRunBrakeEndCount =
        gNdsFighterPreviewLoopRunBrakeEndCount;
    gNdsFighterGCDrawAllLoopJumpAnimEndCount =
        gNdsFighterPreviewLoopJumpAnimEndCount;
    gNdsFighterGCDrawAllLoopLandingEndCount =
        gNdsFighterPreviewLoopLandingEndCount;
    gNdsFighterGCDrawAllLoopDeferredInterruptCheckCount =
        gNdsFighterPreviewLoopDeferredInterruptCheckCount;
    gNdsFighterGCDrawAllLoopUnexpectedStatusCount =
        gNdsFighterPreviewLoopUnexpectedStatusCount;
    gNdsFighterGCDrawAllLoopDeniedStatusCount =
        gNdsFighterPreviewLoopDeniedStatusCount;
    gNdsFighterGCDrawAllLoopDisplayProbeCount = 0u;
    gNdsFighterGCDrawAllLoopGameplayUpdateCount = 0u;
    gNdsFighterGCDrawAllLoopDrawCallCount = 0u;
    gNdsFighterGCDrawAllLoopMatrixCallCount = 0u;
    gNdsFighterGCDrawAllLoopRootYDriftCount =
        gNdsFighterPreviewLoopRootYDriftCount;
    gNdsFighterGCDrawAllLoopGADriftCount =
        gNdsFighterPreviewLoopGADriftCount;
}

static void ndsFighterGCDrawAllLoopPauseProofOwnedProcesses(void)
{
    u32 i;

    for (i = 0u; i < 2u; i++)
    {
        if (sNdsFighterGCRunAllLoopProcesses[i] != NULL)
        {
            gcPauseGObjProcess(sNdsFighterGCRunAllLoopProcesses[i]);
            gNdsFighterGCDrawAllLoopOldProcessPauseCount++;
        }
    }
}

static void ndsFighterGCDrawAllLoopPauseNonTargetGObjVisitor(GObj *gobj,
                                                             u32 param)
{
    GObj *target0 = sNdsFighterStructPool[0].fighter_gobj;
    GObj *target1 = sNdsFighterStructPool[1].fighter_gobj;
    (void)param;

    if (gobj == NULL)
    {
        return;
    }
    if ((gobj == target0) || (gobj == target1))
    {
        gNdsFighterGCDrawAllLoopTargetProcessPreserveCount++;
        return;
    }
    gNdsFighterGCDrawAllLoopNonTargetGObjVisitCount++;
    if (gobj->gobjproc_head != NULL)
    {
        gcPauseGObjProcessAll(gobj);
        gNdsFighterGCDrawAllLoopNonTargetProcessPauseCount++;
    }
    gobj->flags |= GOBJ_FLAG_NORUN;
}

static void ndsFighterGCDrawAllLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 slot = 2u;

    if ((fp != NULL) && (fp->player < 2))
    {
        slot = fp->player;
    }
    if ((slot >= 2u) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        gNdsFighterGCDrawAllLoopProcessAttachEscapeCount++;
        return;
    }
    if (slot == 0u)
    {
        gNdsFighterGCDrawAllLoopP0ProcCallbackCount++;
        gNdsFighterGCDrawAllLoopP0GObjProcessRunCount++;
    }
    else
    {
        gNdsFighterGCDrawAllLoopP1ProcCallbackCount++;
        gNdsFighterGCDrawAllLoopP1GObjProcessRunCount++;
    }
    sNdsFighterGCDrawAllLoopActive = TRUE;
    ndsFighterPreviewLoopRunSlotProcess(slot, fp);
    sNdsFighterGCDrawAllLoopActive = FALSE;
}

static void ndsFighterGCDrawAllLoopRecordDisplayFromCallback(
    GObj *fighter_gobj)
{
    FTStruct *fp;
    u32 slot;

    if ((fighter_gobj == NULL) || (sNdsFighterGCDrawAllLoopPixels == NULL))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if (ndsFighterStructIsPoolPointer(fp) == FALSE)
    {
        gNdsFighterGCDrawAllLoopNonTargetDisplayCallbackCount++;
        return;
    }
    slot = (u32)fp->nds_slot;
    if (slot > 1u)
    {
        gNdsFighterGCDrawAllLoopNonTargetDisplayCallbackCount++;
        return;
    }

    gNdsFighterGCDrawAllLoopCapturedDisplayCount++;
    gNdsFighterGCDrawAllLoopDisplayCallbackCount++;
    if (slot == 0u)
    {
        gNdsFighterGCDrawAllLoopP0DisplayCallbackCount++;
    }
    else
    {
        gNdsFighterGCDrawAllLoopP1DisplayCallbackCount++;
    }
    ndsFighterPreviewLoopDrawSlot(slot, fp, sNdsFighterGCDrawAllLoopPixels,
                                  sNdsFighterGCDrawAllLoopPitch);
}

s32 ndsFighterMarioFoxGCDrawAllLoopDisplayActive(void)
{
    return (sNdsFighterGCDrawAllLoopDisplayActive != FALSE) ? TRUE : FALSE;
}

s32 ndsFighterMarioFoxStageGCDrawAllLoopProofActive(void)
{
    return ((ndsFighterMarioFoxStageGCDrawAllLoopProofEnabled() != FALSE) &&
            (gNdsStageGCDrawAllLoopPrepared != 0u)) ? TRUE : FALSE;
}

static GObj *sNdsStageGCDrawAllLoopCurrentCameraGObj;
static GObj *sNdsStageGCDrawAllLoopCurrentDisplayGObj;
static s32 sNdsStageGCDrawAllLoopCurrentDisplayLinkID;
#if NDS_RENDERER_HW_TRIANGLES
static sb32 sNdsStageGCDrawAllLoopHardwareSubmitActive;
static sb32 sNdsStageGCDrawAllLoopNativeStageArmed;
static u32 sNdsStageGCDrawAllLoopHardwareSubmitCount;
extern void ndsRendererAdapterResetDepthDiagnostics(void);

static u32 ndsStageGCDrawAllLoopInitialGeometryMode(void)
{
    /* scVSBattleFuncLights establishes this before every battle display proc.
     * Individual source lists remain free to clear and restore it. */
    u32 mode = NDS_RENDERER_GEOM_RESET_MODE | NDS_RENDERER_GEOM_LIGHTING;

    return (sNdsStageGCDrawAllLoopCurrentDisplayLinkID == 6) ?
        mode : (mode & ~NDS_RENDERER_GEOM_ZBUFFER);
}

static sb32 ndsStageGCDrawAllLoopIsWeaponDisplay(GObj *gobj, s32 link_id)
{
    return ((gobj != NULL) &&
            (gobj->id == nGCCommonKindWeapon) &&
            (gobj->dl_link_id == 14) &&
            (link_id == 14)) ? TRUE : FALSE;
}

/* THIS PREDICATE IS WHAT "THE BATTLE HARDWARE PATH DOES NOT CONSUME SOURCE
 * EFFECT DL LINKS" ACTUALLY MEANS. The port wrote that sentence down twice --
 * Makefile:1401 and battleship_efmanager.c:1240 -- and neither said where it
 * lived, so three cycles looked for it in the particle atlas, in the camera's
 * capture passes and in the DObj tree walk. It is here, and it is one call:
 *
 *   ndsEFManagerIsVisualEffectGObj (efmanager.c:969) returns TRUE only when
 *   dobj->dl equals one of sNdsVisualTemplates[i].display_list.
 *
 * That is a pointer comparison against the PROCEDURAL stand-in templates. A
 * source-made effect -- efManagerMakeEffectForce loading a real DObjDesc, which
 * is what NDS_R2_SOURCE_EFFECTS_FULL switches the shield, rebirth halo, Fox
 * reflector and impact wave over to -- carries the ROM asset's display list and
 * can never match. So turning the flag on removed each stand-in and put nothing
 * on screen in its place: not because the geometry was wrong, not because the
 * camera dropped the link, but because this gate admits only the things the
 * flag replaces.
 *
 * A source model is admitted on its own terms now: the same link-18 effect
 * identity, plus a DObj carrying a display list, which is exactly what the
 * submit below already requires before it will draw anything. */
/* THE LINK IS THE WHOLE ANSWER, and the codebase was carrying its own control
 * the entire time. Every EFDesc names the display link it draws on
 * (efmanager.c, field 2 of the desc):
 *
 *     dEFManagerShieldEffectDesc        :460   link 15
 *     dEFManagerFoxReflectorEffectDesc  :420   link 15
 *     dEFManagerRebirthHaloEffectDesc   :1648  link 10
 *     dEFManagerImpactWaveEffectDesc    :201   link 10
 *     dEFManagerDeadExplodeEffectDesc   :850   link 18   <- the KO burst
 *
 * The first four are the four rows that have never drawn. The fifth is the KO
 * burst, which BUGS.md records as working "unconditionally" through exactly the
 * same source-model route. The only thing that differs between them is this
 * field, and this predicate used to require 18.
 *
 * So "the battle hardware path does not consume source effect DL links" was a
 * LINK COVERAGE GAP: the effect submit accepted one link, the procedural
 * stand-ins were hard-wired onto it (ndsEFManagerMakeVisualEffect passes 18 to
 * gcAddGObjDisplay), and the source models that live on 10 and 15 were never
 * offered to the hardware at all. Not the atlas, not the camera -- the camera
 * captures 10 in pass 3 and 15 in pass 4 -- and not the tree walk.
 *
 * Fox's entry Arwing adds the second half of the source contract: its EFDesc
 * starts on link 10, then efManagerSortZNeg moves the SAME effect GObj to link
 * 2 or 20 every update according to the animated model Z.  gmCameraDefaultProcDisplay
 * explicitly captures link 2 in its first pass and link 20 in its final pass.
 * Restricting a source effect to its descriptor's initial link therefore makes
 * the Arwing disappear as soon as the source performs its normal depth sort.
 * Admit those two source-owned sorted-effect links as well; the GObj-kind and
 * DObj-geometry checks below still prevent stage/ground traffic sharing link 2
 * from entering the effect submitter.
 *
 * Behind the second gate sits ndsEFManagerIsVisualEffectGObj (efmanager.c:969),
 * which returns TRUE only when dobj->dl matches a procedural template pointer.
 * A source model carries the ROM asset's list and can never match it, so both
 * gates had to open for any of these to draw. */
#define NDS_EFFECT_DISPLAY_LINK_TEMPLATE 18

static sb32 ndsStageGCDrawAllLoopIsEffectDisplay(GObj *gobj, s32 link_id)
{
    if ((gobj == NULL) || (gobj->id != nGCCommonKindEffect) ||
        (gobj->dl_link_id != link_id))
    {
        return FALSE;
    }
    if ((link_id == NDS_EFFECT_DISPLAY_LINK_TEMPLATE) &&
        (ndsEFManagerIsVisualEffectGObj(gobj) != FALSE))
    {
        return TRUE;
    }
    /* 10 and 15 carry ordinary source effect models; 2 and 20 are the source's
     * dynamic Z-sort destinations (Fox entry Arwing is the measured owner); 18
     * also carries the KO burst when it is not a procedural template. A display
     * list is required because the submit below refuses to draw without one. */
    if ((link_id == 2) || (link_id == 10) || (link_id == 15) ||
        (link_id == 20) ||
        (link_id == NDS_EFFECT_DISPLAY_LINK_TEMPLATE))
    {
        DObj *dobj = DObjGetStruct(gobj);

        /* A CHILD COUNTS. Requiring geometry on the ROOT excluded every
         * tree-shaped source model, which is to say three of the four rows this
         * gate exists to admit. gcSetupCustomDObjs (objanim.c:2413) builds the
         * id==0 entry with gcAddDObjForGObj(gobj, dobjdesc->dl), and node0's dl
         * is NULL by construction in all three descs -- the root is
         * transform-only and the display list hangs off the child. So
         * dobj->dv != NULL was only ever true for the impact wave, whose EFDesc
         * omits flag 0x4 and therefore gets one DObj holding the list directly.
         * Measured: every one of the 72 admitted draws in a 901-frame flag-on
         * run was an impact wave, and the shield was created at frame 576 and
         * never reached the walker at all.
         *
         * A child pointer is the cheap discriminator and needs no walk: the
         * submit already recurses the tree and already declines nodes with no
         * list, so admitting a transform-only root costs one traversal of a
         * two- or three-node tree and cannot draw anything spurious. */
        if ((dobj != NULL) &&
            ((dobj->dv != NULL) || (dobj->child != NULL)))
        {
            gNdsEffectRendererSourceModelAdmitCount++;
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsStageGCDrawAllLoopRecordWeaponCapture(GObj *gobj,
                                                      s32 link_id)
{
    WPStruct *wp;

    if (ndsStageGCDrawAllLoopIsWeaponDisplay(gobj, link_id) == FALSE)
    {
        return;
    }
    gNdsWeaponRendererCaptureCount++;
    wp = gobj->user_data.p;
    if ((wp != NULL) && (wp->kind >= 0) && (wp->kind < 32))
    {
        gNdsWeaponRendererKindMask |= 1u << (u32)wp->kind;
    }
}

static void ndsStageGCDrawAllLoopRecordEffectCapture(GObj *gobj,
                                                      s32 link_id)
{
    if (ndsStageGCDrawAllLoopIsEffectDisplay(gobj, link_id) != FALSE)
    {
        gNdsEffectRendererCaptureCount++;
    }
}

#if NDS_R2_FIREBALL_QUAD
/* MARIO'S FIREBALL AS ONE CAMERA-FACING QUAD, the shield experiment applied to
 * a projectile. Same shape of change and the same reason it is worth making:
 * the source model is a SINGLE flat quad behind a display list, so both routes
 * put the same four vertices on screen and only the submit differs.
 *
 * Decoded from relocData file 297 on 2026-08-06 rather than assumed:
 *
 *   Vtx[0..3]  pos (0, +-150, +-150), st 0..512 in S10.5 = exactly 0..16
 *   Tex        CI4 16x16, indices 0..12 used, only index 0 transparent
 *   WPDesc     main matrix kind nGCMatrixKindTra (translation ONLY, so there
 *              is no joint scale to recover the way the shield's 0x2C had),
 *              secondary 0x47 = the MVP-recalc billboard
 *              (reloc_backend_renderer_dl.c:1853, "replace all three
 *              orientation rows ... the translation row stays live")
 *
 * A billboard whose orientation is discarded and whose translation survives is
 * precisely what ndsParticleDrawOwnTextureQuad draws, so the quad reproduces
 * the transform rather than approximating it.
 *
 * MEASURED FIRST, on the whole-match instrument (ROM E61D608B, 2026-08-06):
 * one live fireball costs 64,700 ticks/frame at P50 -- SRC 26,752 and MISC
 * 33,088 -- against 129,468 ticks of headroom under the two-VBlank boundary.
 * Two of them therefore consume all of it and the frame steps to three
 * VBlanks, which is the 30 -> 20 FPS collapse the owner reported. This flag
 * addresses the MISC half only; the SRC half is wpMapTestAll running full
 * stage collision for the projectile every frame and is a separate seam.
 *
 * The fireball census at the bottom of the submit below (FireballSubmitCount,
 * FireballTriangleCount, FireballCustom47*) counts the GENERIC route and
 * therefore reads zero with this flag on. That is correct, not a regression:
 * the counters here are its replacement. */
/* BGR555, NOT 0xRRGGBB -- red bits 0-4, green 5-9, blue 10-14. Duplicated from
 * battleship_efmanager.c deliberately rather than shared: a two-line packing
 * macro promoted to a header would be a speculative abstraction, and the
 * comment there records that getting this wrong shipped a black shield. */
#define NDS_R2_FIREBALL_BGR555(r, g, b) \
    ((((u32)(r) >> 3) & 31u) | ((((u32)(g) >> 3) & 31u) << 5) | \
     ((((u32)(b) >> 3) & 31u) << 10))

/* Engagement pair, same contract as the shield's. Draw climbing with Fallback
 * at 0 is the quad route working; Fallback climbing means the quad is refused
 * every frame and the display list is still carrying the picture -- which looks
 * identical on screen and costs MORE than the default. A measurement taken
 * without reading these would be meaningless. */
volatile u32 gNdsFireballQuadDrawCount;
volatile u32 gNdsFireballQuadFallbackCount;

static u32 sNdsFireballTextureName[NDS_FIREBALL_PALETTE_COUNT];

static s32 ndsFireballTexelFill(u8 *pixels, u32 bytes, void *user_data)
{
    (void)user_data;
    if ((pixels == NULL) || (bytes < NDS_FIREBALL_TEX_BYTES))
    {
        return FALSE;
    }
    memcpy(pixels, gNdsFireballTexels, NDS_FIREBALL_TEX_BYTES);
    return TRUE;
}

/* NOTHING IS COMPUTED HERE, and unlike the shield nothing was quantised in the
 * generator either: CI4 with an RGBA5551 TLUT whose entry 0 has alpha 0 IS
 * GL_RGB16 + COLOR0_TRANSPARENT, so preparing the fireball is an upload of the
 * source's own texels and a pointer to the source's own palette. */
static u32 ndsFireballTexture(s32 palette_index)
{
    u32 slot = ((u32)palette_index < NDS_FIREBALL_PALETTE_COUNT)
        ? (u32)palette_index : 0u;

    if (sNdsFireballTextureName[slot] != 0u)
    {
        return sNdsFireballTextureName[slot];
    }
    if (ndsRendererHardwarePrepareIFCommonPal16Atlas(
            NDS_FIREBALL_TEX_WIDTH, NDS_FIREBALL_TEX_HEIGHT,
            gNdsFireballPalettes[slot], ndsFireballTexelFill, NULL,
            &sNdsFireballTextureName[slot]) == FALSE)
    {
        sNdsFireballTextureName[slot] = 0u;
    }
    return sNdsFireballTextureName[slot];
}

/* Give last match's names back before this one allocates -- START at the
 * results screen restarts the match, and a name held across restarts would
 * grow the texture cache by one per fireball palette every time. Called from
 * efManagerInitEffects beside the shield's identical release. */
void ndsWeaponReleaseBakedTextures(void)
{
    u32 slot;

    for (slot = 0u; slot < ARRAY_COUNT(sNdsFireballTextureName); slot++)
    {
        if (sNdsFireballTextureName[slot] != 0u)
        {
            ndsRendererHardwareReleaseIFCommonCloudAtlas(
                &sNdsFireballTextureName[slot]);
            sNdsFireballTextureName[slot] = 0u;
        }
    }
    gNdsFireballQuadDrawCount = 0u;
    gNdsFireballQuadFallbackCount = 0u;
}

static sb32 ndsStageGCDrawAllLoopDrawFireballQuad(DObj *root, WPStruct *wp)
{
    u32 texture_name;

    if ((root == NULL) || (wp == NULL))
    {
        return FALSE;
    }
    /* weapon_vars.fireball.index is the OWNER, not a frame: 0 Mario, 1 Luigi,
     * and wpmariofireball.c:189 writes the MObj palette_id from the same index.
     * So it selects the palette here for exactly the reason it does there. */
    texture_name = ndsFireballTexture(wp->weapon_vars.fireball.index);
    if (texture_name == 0u)
    {
        return FALSE;
    }
    return ndsParticleDrawOwnTextureQuad(
        texture_name, NDS_FIREBALL_TEX_WIDTH, NDS_FIREBALL_TEX_HEIGHT,
        &root->translate.vec.f, NDS_FIREBALL_QUAD_HALF_EXTENT,
        NDS_R2_FIREBALL_BGR555(0xff, 0xff, 0xff), 0xffu, 0.0F,
        /* THE SOURCE SPIN, not an approximation: the fireball's own update
         * adds 20 deg/frame to rotate.vec.f.x (wpmariofireball.c:98), and the
         * 0x47 orientation is RotRpyR(rotate.x, rotate.y, 0) -- the
         * interpreter path visibly rolls the quad as it flies. The quad
         * submit rolls the camera-facing basis by the SAME angle, so the spin
         * matches it frame for frame instead of being a static billboard.
         *
         * THE FACING DEPENDENCE IS A TEXTURE MIRROR, carried by rotate.vec.f.y:
         * wpMainVelSetModelPitch (wpmain.c:52-57) sets rotate.y = +90 deg
         * when vel.x >= 0 (facing right) and -90 deg facing left, at spawn
         * and again on every rebound (wpmariofireball.c:191/:118). In the
         * 0x47 matrix the +90 deg pitch maps the quad's local Z axis to
         * -screen-X and the -90 deg pitch to +screen-X -- a horizontal
         * mirror of the texture -- and the apparent roll direction reverses
         * with it. The mirror is VISIBLE on the flame streaks, so it is
         * applied to the basis here and the roll stays +rotate.x exactly as
         * the source writes it. */
        root->rotate.vec.f.x,
        (root->rotate.vec.f.y >= 0.0F) ? TRUE : FALSE);
}
#endif /* NDS_R2_FIREBALL_QUAD */

#if NDS_R2_FOX_BLASTER_QUAD
/* Engagement/fallback pair for the lab arm. A rising draw count with zero
 * fallback proves the exact horizontal source contract reached direct GX;
 * fallback preserves the restored source display after a hop or reflection. */
volatile u32 gNdsFoxBlasterQuadDrawCount;
volatile u32 gNdsFoxBlasterQuadFallbackCount;

static sb32 ndsStageGCDrawAllLoopDrawFoxBlasterQuad(DObj *root, WPStruct *wp)
{
    s32 facing;

    if ((root == NULL) || (wp == NULL) ||
        (root->rotate.vec.f.x != 0.0F) ||
        (root->rotate.vec.f.y != 0.0F) ||
        (root->scale.vec.f.y != 1.0F) ||
        (root->scale.vec.f.z != 1.0F) ||
        (wp->physics.vel_air.y != 0.0F))
    {
        return FALSE;
    }
    /* Source spawn writes exactly +/-160 and zero Y. A hop rotates that vector
     * and a reflector can redirect it, so admitting only these two bit-exact
     * values makes the fast path the natural Fox shot and nothing broader. */
    if (wp->physics.vel_air.x == 160.0F)
    {
        facing = 1;
    }
    else if (wp->physics.vel_air.x == -160.0F)
    {
        facing = -1;
    }
    else
    {
        return FALSE;
    }
    if (ndsRendererAdapterSetWorldQuadCamera(
            sNdsStageGCDrawAllLoopCurrentCameraGObj) == FALSE)
    {
        return FALSE;
    }
    return ndsRendererSubmitFoxBlasterQuad(
        &root->translate.vec.f, root->scale.vec.f.x,
        root->scale.vec.f.y, facing);
}
#endif /* NDS_R2_FOX_BLASTER_QUAD */

static void ndsStageGCDrawAllLoopSubmitWeaponDObj(GObj *weapon_gobj,
                                                  u32 callback_kind)
{
    DObj *root;
    WPStruct *wp;
    u32 triangle_before;
    u32 texture_ready_before;
    u32 texture_reject_before;
    u32 triangle_delta;
    u32 texture_ready_delta;
    u32 texture_reject_delta;
    u32 custom47_applied_before;
    u32 custom47_reject_before;
    u32 custom47_translation_mismatch_before;
    u32 custom47_applied_delta;
    u32 initial_geometry_mode;
    u32 x_bits;
    u32 y_bits;

    if ((weapon_gobj == NULL) ||
        (weapon_gobj != sNdsStageGCDrawAllLoopCurrentDisplayGObj) ||
        (ndsStageGCDrawAllLoopIsWeaponDisplay(
             weapon_gobj,
             sNdsStageGCDrawAllLoopCurrentDisplayLinkID) == FALSE))
    {
        return;
    }

    gNdsWeaponRendererDObjDrawCount++;
    gNdsWeaponRendererCallbackKind = callback_kind;
    root = DObjGetStruct(weapon_gobj);
    if ((root == NULL) || (root->dv == NULL) ||
        (sNdsStageGCDrawAllLoopCurrentCameraGObj == NULL) ||
        (callback_kind != NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1))
    {
        gNdsWeaponRendererRejectedDrawCount++;
        return;
    }

    x_bits = ndsFloatBits(root->translate.vec.f.x);
    y_bits = ndsFloatBits(root->translate.vec.f.y);
    if ((gNdsWeaponRendererSubmitCount != 0u) &&
        ((x_bits != gNdsWeaponRendererLastXBits) ||
         (y_bits != gNdsWeaponRendererLastYBits)))
    {
        gNdsWeaponRendererMovingDrawCount++;
    }
    gNdsWeaponRendererLastXBits = x_bits;
    gNdsWeaponRendererLastYBits = y_bits;

    triangle_before = gNdsStageGCDrawAllLoopHardwareTriangleCount;
    texture_ready_before =
        gNdsStageGCDrawAllLoopHardwareTextureReadyCount;
    texture_reject_before =
        gNdsStageGCDrawAllLoopHardwareTextureRejectCount;
    custom47_applied_before =
        gNdsRendererAdapterCustom47AppliedCount;
    custom47_reject_before =
        gNdsRendererAdapterCustom47RejectCount;
    custom47_translation_mismatch_before =
        gNdsRendererAdapterCustom47TranslationMismatchCount;
#if NDS_R2_FOX_BLASTER_QUAD
    {
        WPStruct *blaster_wp = weapon_gobj->user_data.p;

        if ((blaster_wp != NULL) && (blaster_wp->kind == nWPKindBlaster))
        {
            if (ndsStageGCDrawAllLoopDrawFoxBlasterQuad(root, blaster_wp) !=
                FALSE)
            {
                gNdsFoxBlasterQuadDrawCount++;
                gNdsWeaponRendererSubmitCount++;
                gNdsWeaponRendererVisibleDrawCount++;
                gNdsWeaponRendererTriangleCount += 2u;
                gNdsStageGCDrawAllLoopHardwareTriangleCount += 2u;
                sNdsStageGCDrawAllLoopHardwareSubmitCount++;
                gNdsStageGCDrawAllLoopHardwareSubmitCount =
                    sNdsStageGCDrawAllLoopHardwareSubmitCount;
                return;
            }
            gNdsFoxBlasterQuadFallbackCount++;
        }
    }
#endif
#if NDS_R2_FIREBALL_QUAD
    /* Before the tree walk, not inside it: the whole point is to spend neither
     * the walk nor the display list. Falls through to the generic route on any
     * refusal -- a missing texture name or a camera the quad path will not
     * accept -- so a failure here is a slow fireball, never an absent one. */
    {
        WPStruct *fireball_wp = weapon_gobj->user_data.p;

        if ((fireball_wp != NULL) && (fireball_wp->kind == nWPKindFireball))
        {
            if (ndsStageGCDrawAllLoopDrawFireballQuad(root, fireball_wp) !=
                FALSE)
            {
                gNdsFireballQuadDrawCount++;
                sNdsStageGCDrawAllLoopHardwareSubmitCount++;
                gNdsStageGCDrawAllLoopHardwareSubmitCount =
                    sNdsStageGCDrawAllLoopHardwareSubmitCount;
                return;
            }
            gNdsFireballQuadFallbackCount++;
        }
    }
#endif
    initial_geometry_mode = ndsStageGCDrawAllLoopInitialGeometryMode();
    if ((initial_geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) == 0u)
    {
        /* BattleShip wpDisplayDrawNormal clears Z before link-14 weapons. */
        gNdsWeaponRendererNoZCount++;
    }

    ndsRendererAdapterBeginStageTraversal();
    ndsRendererAdapterSubmitStageDObj(
        root,
        callback_kind,
        sNdsStageGCDrawAllLoopCurrentCameraGObj,
        initial_geometry_mode);
    ndsRendererAdapterEndStageTraversal();

    triangle_delta =
        gNdsStageGCDrawAllLoopHardwareTriangleCount - triangle_before;
    texture_ready_delta =
        gNdsStageGCDrawAllLoopHardwareTextureReadyCount -
        texture_ready_before;
    texture_reject_delta =
        gNdsStageGCDrawAllLoopHardwareTextureRejectCount -
        texture_reject_before;
    custom47_applied_delta =
        gNdsRendererAdapterCustom47AppliedCount - custom47_applied_before;
    gNdsWeaponRendererTriangleCount += triangle_delta;
    gNdsWeaponRendererTextureReadyCount += texture_ready_delta;
    gNdsWeaponRendererTextureRejectCount += texture_reject_delta;

    if (triangle_delta == 0u)
    {
        gNdsWeaponRendererRejectedDrawCount++;
        return;
    }

    gNdsWeaponRendererSubmitCount++;
    if (texture_reject_delta == 0u)
    {
        gNdsWeaponRendererVisibleDrawCount++;
    }
    wp = weapon_gobj->user_data.p;
    if ((wp != NULL) && (wp->kind == nWPKindFireball))
    {
        if (gNdsWeaponRendererFireballSubmitCount == 0u)
        {
            gNdsWeaponRendererFireballFirstXBits = x_bits;
            gNdsWeaponRendererFireballFirstYBits = y_bits;
        }
        gNdsWeaponRendererFireballLastXBits = x_bits;
        gNdsWeaponRendererFireballLastYBits = y_bits;
        gNdsWeaponRendererFireballSubmitCount++;
        gNdsWeaponRendererFireballTriangleCount += triangle_delta;
        gNdsWeaponRendererFireballCustom47AppliedCount +=
            custom47_applied_delta;
        if ((custom47_applied_delta != 1u) ||
            (gNdsRendererAdapterCustom47RejectCount !=
                custom47_reject_before) ||
            (gNdsRendererAdapterCustom47TranslationMismatchCount !=
                custom47_translation_mismatch_before))
        {
            gNdsWeaponRendererFireballCustom47MismatchCount++;
        }
        if ((texture_ready_delta != 0u) && (texture_reject_delta == 0u))
        {
            gNdsWeaponRendererFireballVisibleDrawCount++;
        }
    }
    else if ((wp != NULL) && (wp->kind == nWPKindBlaster))
    {
        gNdsWeaponRendererBlasterSubmitCount++;
        gNdsWeaponRendererBlasterTriangleCount += triangle_delta;
        if (texture_reject_delta == 0u)
        {
            gNdsWeaponRendererBlasterVisibleDrawCount++;
        }
    }
    sNdsStageGCDrawAllLoopHardwareSubmitCount++;
    gNdsStageGCDrawAllLoopHardwareSubmitCount =
        sNdsStageGCDrawAllLoopHardwareSubmitCount;
}

/* bit0 TREE, bit1 TREE_DLLINKS, bit2 DLLINKS, bit3 DLHEAD0, bit4 DLHEAD1,
 * bit31 anything else. Only bit0 is accepted by the submit below, so a reject
 * mask of 0x02 names TREE_DLLINKS as the kind the source models arrive with. */
static u32 ndsStageGCDrawAllLoopCallbackKindBit(u32 kind)
{
    switch (kind)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:         return 1u << 0;
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS: return 1u << 1;
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:      return 1u << 2;
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0:      return 1u << 3;
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:      return 1u << 4;
    default:                                               return 1u << 31;
    }
}

/* MEASURED, not assumed: with the flag on, the source models reach the submit
 * as DLHEAD0 and NOTHING ELSE -- the probe's observed-kind mask reads 0x8 with
 * no other bit set, and the reject mask reads the same 0x8. The submit accepted
 * DOBJ_TREE alone, so all six frames were refused before the tree walk ever ran
 * (nodes=0). ndsRendererAdapterSubmitStageDObjNode already handles DLHEAD0 in
 * the same switch arm as DOBJ_TREE, so nothing downstream needed teaching. */
static sb32 ndsStageGCDrawAllLoopEffectKindAccepted(u32 callback_kind)
{
    if (callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE)
    {
        return TRUE;
    }
    if (callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0)
    {
        return TRUE;
    }
    /* TREE_DLLINKS ARRIVES TOO, once the transform-only-root rule above stops
     * discarding tree-shaped effects. Measured the moment that landed: the
     * observed-kind mask went 0x8 -> 0xa and the reject mask read 0x2, which is
     * exactly this kind and nothing else. It is the rebirth halo, whose EFDesc
     * names gcDrawDObjTreeDLLinksForGObj as its proc_display, so a DL-links
     * tree is the correct shape rather than a surprise.
     * ndsRendererAdapterSubmitStageDObjNode already walks it in the DLLINKS
     * arm, so as with DLHEAD0 nothing downstream needed teaching. */
    if (callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS)
    {
        return TRUE;
    }
    return FALSE;
}

static void ndsStageGCDrawAllLoopSubmitEffectDObj(GObj *effect_gobj,
                                                  u32 callback_kind)
{
    DObj *root;
    u32 triangle_before;
    u32 texture_ready_before;
    u32 texture_reject_before;
    u32 triangle_delta;
    u32 texture_ready_delta;
    u32 texture_reject_delta;

    if ((effect_gobj == NULL) ||
        (effect_gobj != sNdsStageGCDrawAllLoopCurrentDisplayGObj) ||
        (ndsStageGCDrawAllLoopIsEffectDisplay(
             effect_gobj,
             sNdsStageGCDrawAllLoopCurrentDisplayLinkID) == FALSE))
    {
        return;
    }
    gNdsEffectRendererDObjDrawCount++;
    /* WHICH KIND ARRIVED, because the guard below has four clauses and a single
     * reject counter cannot say which one fired. The source-model probe reads
     * dobjdraw=6 reject=6 nodes=0 -- refused before the walk -- and dv is
     * already established by the admission gate while the camera is non-NULL
     * for the link-18 stand-ins drawing beside it, so callback_kind is the
     * remaining clause. A MASK, not a last-value: several kinds reach here per
     * frame and the last to arrive is not necessarily the refused one.
     *
     * The kinds are FOURCCs (DOBJ_TREE is 0x44545245, "DTRE"), NOT small enums,
     * so `1u << kind` is undefined and `kind < 32` is never true -- an obvious
     * shift-mask here would have stayed 0 and read as "nothing arrives". */
    gNdsEffectRendererCallbackKindMask |=
        ndsStageGCDrawAllLoopCallbackKindBit(callback_kind);
    root = DObjGetStruct(effect_gobj);
    /* Same transform-only-root rule as the admission gate above, and it has to
     * be repeated here or the gate's work is undone one call later: with only
     * the gate relaxed this path admitted 468 and rejected all 219 draws while
     * the tree walker still ran 6 times, because a source desc tree's id==0
     * root carries no display list. A root with a child is a tree; the walk
     * below declines individual nodes that have nothing to draw. */
    if ((root == NULL) ||
        ((root->dv == NULL) && (root->child == NULL)) ||
        (sNdsStageGCDrawAllLoopCurrentCameraGObj == NULL) ||
        (ndsStageGCDrawAllLoopEffectKindAccepted(callback_kind) == FALSE))
    {
        gNdsEffectRendererRejectedKindMask |=
            ndsStageGCDrawAllLoopCallbackKindBit(callback_kind);
        gNdsEffectRendererRejectedDrawCount++;
        return;
    }
    /* THE DObj FLAGS, because the drawable test is ASYMMETRIC by kind
     * (ndsRendererAdapterStageDObjDrawable, reloc_backend_renderer_dl.c:5886):
     *
     *   DOBJ_TREE / TREE_DLLINKS -> (flags & DOBJ_FLAG_NOTEXTURE) == 0
     *   DLLINKS / DLHEAD0 / DLHEAD1 -> flags == DOBJ_FLAG_NONE
     *
     * Source models arrive as DLHEAD0, so ANY flag at all makes them
     * undrawable and the walk emits nothing -- which is exactly the tris=0
     * this path now reports. Record the flags rather than assume that: it is
     * one OR and it turns the next cycle into a single run. */
    gNdsEffectRendererDObjFlagsMask |= (u32)root->flags;
    /* WHICH FIELD HOLDS THE GEOMETRY. bit0 dl, bit1 dl_link, bit2 dv.
     * The flags hypothesis above was REFUTED by its own counter (dobjflags read
     * 0x0, so the drawable test passes), which is why this one is measured in
     * the same build that acts on it rather than after it. */
    gNdsEffectRendererDObjFieldMask |=
        ((root->dl != NULL) ? 1u : 0u) |
        ((root->dl_link != NULL) ? 2u : 0u) |
        ((root->dv != NULL) ? 4u : 0u);
    triangle_before = gNdsStageGCDrawAllLoopHardwareTriangleCount;
    texture_ready_before =
        gNdsStageGCDrawAllLoopHardwareTextureReadyCount;
    texture_reject_before =
        gNdsStageGCDrawAllLoopHardwareTextureRejectCount;
    /* Consume the colour this effect's own proc_display just emitted, before
     * the tree submit inherits the previous list's persistent RDP state. */
#if NDS_TICK_HUD
    {
        u32 phase_mark = cpuGetTiming();
        ndsRendererAdapterCaptureDisplayProcColors();
        gNdsEffectPhaseColorTicks += cpuGetTiming() - phase_mark;
    }
#else
    ndsRendererAdapterCaptureDisplayProcColors();
#endif
    ndsRendererAdapterBeginStageTraversal();
    /* The effect path, and the only caller that walks the tree. An effect
     * EFDesc is a model -- shield, rebirth halo, Fox reflector, impact wave are
     * all multi-node.
     *
     * TWO HYPOTHESES FOR THE REMAINING tris=0 WERE TESTED HERE AND BOTH DIED,
     * which is why the kind is passed through unchanged:
     *
     *   "a DObj flag makes it undrawable" -- the DLHEAD0 drawable rule is the
     *     strict `flags == DOBJ_FLAG_NONE` (renderer_dl.c:5886). REFUTED:
     *     dobjflags reads 0x0, so the node IS drawable.
     *   "the geometry is in dl_link[], which only the *_DLLINKS kinds read, and
     *     DLHEAD0 submits `dl` while guarding on `dv`" -- REFUTED: the field
     *     mask reads 0x7, so dl, dl_link and dv are ALL non-null, and routing
     *     DLHEAD0 to TREE_DLLINKS left tris at 0 exactly as before.
     *
     * So the node is drawable, the walk reaches it, and every candidate
     * geometry pointer is populated -- and ndsRendererAdapterSubmitStageDL
     * still emits nothing from it. texready and texreject are both 0, so it
     * does not even reach texture handling. That is the next seam and it is
     * inside SubmitStageDL, not here. */
#if NDS_TICK_HUD
    {
        u32 phase_mark = cpuGetTiming();
        ndsRendererAdapterSubmitEffectDObjTree(
            root,
            callback_kind,
            sNdsStageGCDrawAllLoopCurrentCameraGObj,
            ndsStageGCDrawAllLoopInitialGeometryMode());
        gNdsEffectPhaseTreeTicks += cpuGetTiming() - phase_mark;
    }
#else
    ndsRendererAdapterSubmitEffectDObjTree(
        root,
        callback_kind,
        sNdsStageGCDrawAllLoopCurrentCameraGObj,
        ndsStageGCDrawAllLoopInitialGeometryMode());
#endif
    ndsRendererAdapterEndStageTraversal();
    triangle_delta =
        gNdsStageGCDrawAllLoopHardwareTriangleCount - triangle_before;
    texture_ready_delta =
        gNdsStageGCDrawAllLoopHardwareTextureReadyCount -
        texture_ready_before;
    texture_reject_delta =
        gNdsStageGCDrawAllLoopHardwareTextureRejectCount -
        texture_reject_before;
    gNdsEffectRendererTriangleCount += triangle_delta;
    gNdsEffectRendererTextureReadyCount += texture_ready_delta;
    gNdsEffectRendererTextureRejectCount += texture_reject_delta;
    if (triangle_delta == 0u)
    {
        gNdsEffectRendererRejectedDrawCount++;
        return;
    }
    /* "A LINK-15 SOURCE EFFECT DREW TRIANGLES THIS FRAME", which is the arming
     * signal a shield capture needs and which no existing counter provided.
     * probe-shield-vfx.ps1 could only arm on gNdsTask39FxShieldDrawCount, which
     * belonged to the procedural stand-in and died with it, or on
     * SourceModelAdmitCount, which
     * counts every source admit and is dominated by the impact wave -- so the
     * 2026-08-03 captures armed on a wave at frame 343 and shot an empty frame.
     * Link 15 carries the shield and Fox's reflector; the wave and the rebirth
     * halo are link 10, so this separates them. Counted only past the
     * triangle_delta test, so it means DREW and not merely "was submitted". */
    if ((effect_gobj != NULL) && (effect_gobj->dl_link_id == 15))
    {
        gNdsEffectRendererLink15DrawCount++;
    }
    gNdsEffectRendererSubmitCount++;
    sNdsStageGCDrawAllLoopHardwareSubmitCount++;
    gNdsStageGCDrawAllLoopHardwareSubmitCount =
        sNdsStageGCDrawAllLoopHardwareSubmitCount;
}
#endif

static sb32 ndsStageGCDrawAllLoopClassifyGObj(GObj *gobj, u32 *mask,
                                              sb32 *is_layer)
{
    u32 i;

    if (mask != NULL)
    {
        *mask = 0u;
    }
    if (is_layer != NULL)
    {
        *is_layer = FALSE;
    }
    if (gobj == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < ARRAY_COUNT(gGRCommonLayerGObjs); i++)
    {
        if (gobj == gGRCommonLayerGObjs[i])
        {
            if (mask != NULL)
            {
                *mask = 1u << i;
            }
            if (is_layer != NULL)
            {
                *is_layer = TRUE;
            }
            return TRUE;
        }
    }
    for (i = 0u; i < ARRAY_COUNT(gGRCommonStruct.pupupu.map_gobj); i++)
    {
        if (gobj == gGRCommonStruct.pupupu.map_gobj[i])
        {
            if (mask != NULL)
            {
                *mask = 1u << i;
            }
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsStageGCDrawAllLoopIsSelectedFighter(GObj *gobj)
{
    FTStruct *fp;

    if ((gobj == NULL) || (gobj->id != nGCCommonKindFighter))
    {
        return FALSE;
    }
    fp = gobj->user_data.p;
    if (ndsFighterStructIsPoolPointer(fp) != FALSE)
    {
        return TRUE;
    }
    fp = ftGetStruct(gobj);
    return (ndsFighterStructIsPoolPointer(fp) != FALSE) ? TRUE : FALSE;
}

static void ndsStageGCDrawAllLoopScanDObjs(GObj *gobj, u32 owner_mask,
                                           sb32 is_layer, u32 kind,
                                           u32 callback_kind)
{
    DObj *stack[128];
    u32 stack_count = 0u;
    u32 scanned = 0u;
    DObj *root;

    if (gobj == NULL)
    {
        return;
    }
    root = DObjGetStruct(gobj);
    if (root == NULL)
    {
        return;
    }
#if !NDS_RENDERER_HW_TRIANGLES
    (void)callback_kind;
#endif
    stack[stack_count++] = root;
    while ((stack_count != 0u) && (scanned < ARRAY_COUNT(stack)))
    {
        DObj *dobj = stack[--stack_count];

        if (dobj == NULL)
        {
            continue;
        }
        scanned++;
        if (is_layer != FALSE)
        {
            gNdsStageGCDrawAllLoopLayerDObjMask |= owner_mask;
            if (dobj->dv != NULL)
            {
                gNdsStageGCDrawAllLoopLayerDLReadyMask |= owner_mask;
            }
            if (dobj->mobj != NULL)
            {
                gNdsStageGCDrawAllLoopLayerMObjMask |= owner_mask;
            }
        }
        else
        {
            gNdsStageGCDrawAllLoopMapDObjMask |= owner_mask;
            if (dobj->dv != NULL)
            {
                gNdsStageGCDrawAllLoopMapDLReadyMask |= owner_mask;
            }
            if (dobj->mobj != NULL)
            {
                gNdsStageGCDrawAllLoopMapMObjMask |= owner_mask;
            }
        }
#if NDS_RENDERER_HW_TRIANGLES
        if ((dobj->dv != NULL) &&
            (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE) &&
            ((callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE) ||
             (callback_kind ==
                 NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS) ||
             (((callback_kind ==
                    NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS) ||
               (callback_kind ==
                    NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0) ||
               (callback_kind ==
                    NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1)) &&
              (dobj == root))))
        {
            ndsRendererAdapterSubmitStageDObj(
                dobj,
                callback_kind,
                sNdsStageGCDrawAllLoopCurrentCameraGObj,
                ndsStageGCDrawAllLoopInitialGeometryMode());
            sNdsStageGCDrawAllLoopHardwareSubmitCount++;
            gNdsStageGCDrawAllLoopHardwareSubmitCount =
                sNdsStageGCDrawAllLoopHardwareSubmitCount;
        }
#endif
        if ((dobj->sib_next != NULL) && (stack_count < ARRAY_COUNT(stack)))
        {
            stack[stack_count++] = dobj->sib_next;
        }
        if ((dobj->child != NULL) && (stack_count < ARRAY_COUNT(stack)))
        {
            stack[stack_count++] = dobj->child;
        }
    }
    gNdsStageGCDrawAllLoopDObjDrawKindMask |= 1u << kind;
}

void ndsStageGCDrawAllLoopRecordCameraCallback(void)
{
    if ((ndsFighterMarioFoxStageGCDrawAllLoopProofActive() == FALSE) ||
        (gSCManagerSceneData.scene_curr != nSCKindVSBattle) ||
#if NDS_RENDERER_HW_TRIANGLES
        ((sNdsFighterGCDrawAllLoopDisplayActive == FALSE) &&
         (sNdsStageGCDrawAllLoopHardwareSubmitActive == FALSE)))
#else
        (sNdsFighterGCDrawAllLoopDisplayActive == FALSE))
#endif
    {
        return;
    }
    gNdsStageGCDrawAllLoopCameraCallbackCount++;
}

#if NDS_TASK103_STAGE_RUN_PHASE
/* Task 103 E3. E2 accounted for only 39% of the STG bucket; the other 238,254
 * ticks/frame are outside ndsRendererCommitNativeStageSegment entirely, and no
 * task has ever profiled them. gNdsTickHudStageTicks is accumulated at exactly
 * four sites in this file, so tapping each one partitions the whole bucket.
 *
 * These add no timer reads at all -- every site already computes the timestamp
 * it needs for the tick HUD, and this only forks the value it already has. That
 * matters here: E0-E2 cost ~18,100 ticks/frame of instrument, and this span is
 * the one being measured.
 *
 * The four are not known to be disjoint. If display-commit spans nest inside
 * the traversal span, the bucket itself double-counts, and the sum exceeding
 * STG is how that shows up. Lab only, default off. */
volatile u32 gNdsTask103PrepareTicks;
volatile u32 gNdsTask103PrepareCount;
volatile u32 gNdsTask103TraversalTicks;
volatile u32 gNdsTask103TraversalCount;
volatile u32 gNdsTask103DisplayTicks;
volatile u32 gNdsTask103DisplayCount;
volatile u32 gNdsTask103FinishTicks;
volatile u32 gNdsTask103FinishCount;
#endif

s32 NDS_R2_ITCM_PACK2_CODE
ndsStageGCDrawAllLoopRecordCapturedDisplay(void *camera_gobj,
                                           void *display_gobj,
                                           s32 link_id)
{
    GObj *display = display_gobj;
    u32 mask;
    sb32 is_layer;

    if ((ndsFighterMarioFoxStageGCDrawAllLoopProofActive() == FALSE) ||
        (gSCManagerSceneData.scene_curr != nSCKindVSBattle) ||
#if NDS_RENDERER_HW_TRIANGLES
        ((sNdsFighterGCDrawAllLoopDisplayActive == FALSE) &&
         (sNdsStageGCDrawAllLoopHardwareSubmitActive == FALSE)))
#else
        (sNdsFighterGCDrawAllLoopDisplayActive == FALSE))
#endif
    {
        return FALSE;
    }
    gNdsStageGCDrawAllLoopCapturedDisplayCount++;
    sNdsStageGCDrawAllLoopCurrentCameraGObj = camera_gobj;
    sNdsStageGCDrawAllLoopCurrentDisplayGObj = display;
    sNdsStageGCDrawAllLoopCurrentDisplayLinkID = link_id;
#if NDS_RENDERER_HW_TRIANGLES
    /* WHERE THE PROC'S OWN RDP STATE STARTS. This hook runs immediately before
     * `current_gobj->proc_display(current_gobj)` (opening_movie_backend.c:4387),
     * so the four heads here bound exactly the span that proc is about to emit.
     * A source effect's colour lives in that span and nowhere else --
     * efManagerShieldProcDisplay writes prim/env into gSYTaskmanDLHeads[1] and
     * then calls gcDrawDObjTreeDLLinksForGObj -- so the effect submit reads it
     * back from here instead of any effect-specific field. */
    ndsRendererAdapterMarkDisplayProcHeads();
#endif
#if NDS_RENDERER_HW_TRIANGLES
    ndsStageGCDrawAllLoopRecordWeaponCapture(display, link_id);
    ndsStageGCDrawAllLoopRecordEffectCapture(display, link_id);
#endif
    if (ndsStageGCDrawAllLoopClassifyGObj(display, &mask,
                                          &is_layer) != FALSE)
    {
        if (is_layer != FALSE)
        {
            gNdsStageGCDrawAllLoopLayerCaptureMask |= mask;
        }
        else
        {
            gNdsStageGCDrawAllLoopMapCaptureMask |= mask;
        }
    }
    else if (ndsStageGCDrawAllLoopIsSelectedFighter(display) != FALSE)
    {
        gNdsStageGCDrawAllLoopFighterDisplayCallbackCount++;
    }
    else
    {
        gNdsStageGCDrawAllLoopNonStageCaptureCount++;
    }
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsStageGCDrawAllLoopNativeStageArmed != FALSE)
    {
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL == 1)
        u32 owner_start = cpuGetTiming();
        s32 handled = ndsRendererAdapterCommitNativeStageDisplay(
            display, link_id);

#if NDS_TICK_HUD
        {
            u32 stage_ticks = cpuGetTiming() - owner_start;

            gNdsTickHudStageTicks += stage_ticks;
#if NDS_TASK103_STAGE_RUN_PHASE
            gNdsTask103DisplayTicks += stage_ticks;
            gNdsTask103DisplayCount++;
#endif
        }
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererProfileOwners[
            NDS_RENDERER_PROFILE_OWNER_STAGE].exclusive_ticks +=
            cpuGetTiming() - owner_start;
#endif
        return handled;
#else
        return ndsRendererAdapterCommitNativeStageDisplay(display, link_id);
#endif
    }
#endif
    return FALSE;
}

void ndsStageGCDrawAllLoopRecordDObjDraw(void *gobj, u32 kind)
{
    GObj *stage_gobj = gobj;
    u32 mask;
    sb32 is_layer;
    u32 callback_kind = kind;
#if NDS_RENDERER_HW_TRIANGLES && \
    (NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1))
    u32 owner_start = 0u;
#endif

    if (kind >= 32u)
    {
        kind = 31u;
    }
    if (ndsFighterMarioFoxStageGCDrawAllLoopProofActive() == FALSE)
    {
        return;
    }
    if (gSCManagerSceneData.scene_curr != nSCKindVSBattle)
    {
        gNdsStageGCDrawAllLoopUnexpectedSceneCount++;
        return;
    }
    if ((sNdsFighterGCDrawAllLoopDisplayActive == FALSE)
#if NDS_RENDERER_HW_TRIANGLES
        && (sNdsStageGCDrawAllLoopHardwareSubmitActive == FALSE)
#endif
    )
    {
        gNdsStageGCDrawAllLoopManualDisplayCallCount++;
        return;
    }
    if (ndsStageGCDrawAllLoopClassifyGObj(stage_gobj, &mask,
                                          &is_layer) == FALSE)
    {
#if NDS_RENDERER_HW_TRIANGLES
        if (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE)
        {
            /* R2-07 MISC split. These two submits are the projectile and
             * effect DObj halves of the MISC bucket: they run in the branch
             * where ClassifyGObj REJECTED the GObj, so they are outside the
             * STG bracket by construction and land in the residual. */
#if NDS_TICK_HUD
            u32 misc_split_mark = cpuGetTiming();
#endif
            ndsStageGCDrawAllLoopSubmitWeaponDObj(stage_gobj,
                                                  callback_kind);
#if NDS_TICK_HUD
            gNdsMiscWeaponDrawTicks += cpuGetTiming() - misc_split_mark;
            misc_split_mark = cpuGetTiming();
#endif
            ndsStageGCDrawAllLoopSubmitEffectDObj(stage_gobj,
                                                  callback_kind);
#if NDS_TICK_HUD
            gNdsMiscEffectDrawTicks += cpuGetTiming() - misc_split_mark;
#endif
        }
#endif
        return;
    }
    gNdsStageGCDrawAllLoopDObjDrawCallbackCount++;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE)
    {
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
        owner_start = cpuGetTiming();
#endif
        ndsRendererAdapterBeginStageTraversal();
    }
#endif
    ndsStageGCDrawAllLoopScanDObjs(stage_gobj, mask, is_layer, kind,
                                   callback_kind);
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE)
    {
        ndsRendererAdapterEndStageTraversal();
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
        if (owner_start != 0u)
        {
            u32 owner_ticks = cpuGetTiming() - owner_start;

#if NDS_TICK_HUD
            gNdsTickHudStageTicks += owner_ticks;
#if NDS_TASK103_STAGE_RUN_PHASE
            gNdsTask103TraversalTicks += owner_ticks;
            gNdsTask103TraversalCount++;
#endif
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exclusive_ticks +=
                owner_ticks;
            if ((is_layer != FALSE) && (mask == 1u))
            {
                gNdsRendererProfileStageLayer0Ticks += owner_ticks;
            }
#endif
        }
#endif
#if NDS_RENDERER_BENCHMARK_MODE == NDS_RENDERER_BENCHMARK_CPU_PREP_NO_GX
        ndsRendererBenchmarkSinkEndOwner(
            NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
    }
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static void ndsStageGCDrawAllLoopBeginHardwareFrame(void)
{
    Gfx scratch[2];
    Gfx *head = scratch;
    void *saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;

    /* BattleShip sys/rdp.c:112-115 invokes the registered scene-light
     * callback before drawing. DS owns the rest of the frame reset. */
    syRdpResetSettings(&head);
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
}

#if NDS_FAST_WALLPAPER_AFFINE
extern Mtx44f gGMCameraMatrix;
extern void ndsSObjFastWallpaperOfferSeed(const SObj *seed);

static void ndsFastWallpaperPrepareSeedSnapshot(void)
{
    const f32 seed_dist = 14000.0F;
    CObj *cobj;
    SObj *wallpaper_sobj;
    SObj seed_snapshot;
    CObjVec saved_vec;
    Mtx44f saved_matrix;
    Vec2f saved_wallpaper_pos;
    f32 saved_target_dist;
    f32 saved_wallpaper_scale_x;
    f32 saved_wallpaper_scale_y;

    if ((ndsPlatformFastWallpaperCanSeed() == FALSE) ||
        (gGMCameraGObj == NULL) ||
        ((cobj = CObjGetStruct(gGMCameraGObj)) == NULL) ||
        (sGRWallpaperGObj == NULL) ||
        ((wallpaper_sobj = SObjGetStruct(sGRWallpaperGObj)) == NULL))
    {
        return;
    }

    saved_vec = cobj->vec;
    saved_target_dist = gGMCameraStruct.target_dist;
    saved_wallpaper_pos = wallpaper_sobj->pos;
    saved_wallpaper_scale_x = wallpaper_sobj->sprite.scalex;
    saved_wallpaper_scale_y = wallpaper_sobj->sprite.scaley;
    memcpy(saved_matrix, gGMCameraMatrix, sizeof(saved_matrix));

    cobj->vec.eye.x = cobj->vec.at.x;
    cobj->vec.eye.y = cobj->vec.at.y;
    cobj->vec.eye.z = cobj->vec.at.z + seed_dist;
    gGMCameraStruct.target_dist = seed_dist;
    grWallpaperCalcPersp(wallpaper_sobj);
    seed_snapshot = *wallpaper_sobj;
    seed_snapshot.next = NULL;
    seed_snapshot.prev = NULL;

    cobj->vec = saved_vec;
    gGMCameraStruct.target_dist = saved_target_dist;
    wallpaper_sobj->pos = saved_wallpaper_pos;
    wallpaper_sobj->sprite.scalex = saved_wallpaper_scale_x;
    wallpaper_sobj->sprite.scaley = saved_wallpaper_scale_y;
    memcpy(gGMCameraMatrix, saved_matrix, sizeof(saved_matrix));

    if ((memcmp(&cobj->vec, &saved_vec, sizeof(saved_vec)) != 0) ||
        (memcmp(&gGMCameraStruct.target_dist, &saved_target_dist,
                sizeof(saved_target_dist)) != 0) ||
        (memcmp(&wallpaper_sobj->pos, &saved_wallpaper_pos,
                sizeof(saved_wallpaper_pos)) != 0) ||
        (memcmp(&wallpaper_sobj->sprite.scalex,
                &saved_wallpaper_scale_x,
                sizeof(saved_wallpaper_scale_x)) != 0) ||
        (memcmp(&wallpaper_sobj->sprite.scaley,
                &saved_wallpaper_scale_y,
                sizeof(saved_wallpaper_scale_y)) != 0) ||
        (memcmp(gGMCameraMatrix, saved_matrix, sizeof(saved_matrix)) != 0))
    {
        u32 asset_identity = (u32)(uintptr_t)
            wallpaper_sobj->sprite.bitmap;

        gNdsFastWallpaperSeedRestoreMismatchCount++;
        if (ndsPlatformFastWallpaperBeginSeed(
                0, 0, 1u << 16, 1u << 16,
                asset_identity) != FALSE)
        {
            (void)ndsPlatformFastWallpaperFinishSeed(FALSE);
        }
        return;
    }
    ndsSObjFastWallpaperOfferSeed(&seed_snapshot);
}
#endif

#if NDS_SCENE_MIP_CACHE_LAB
#define NDS_SCENE_MIP_COUNT 1u

static const f32 sNdsSceneMipSeedDistances[NDS_SCENE_MIP_COUNT] = {
    14000.0F
};
extern Mtx44f gGMCameraMatrix;

volatile u32 gNdsSceneMipCacheSeedDrawCount;
volatile u32 gNdsSceneMipCacheDrawCount;
volatile u32 gNdsSceneMipCacheFallbackCount;
volatile u32 gNdsSceneMipCacheSelectedMip;
volatile u32 gNdsSceneMipCacheSelectedMipMask;
volatile u32 gNdsSceneMipCacheTargetDistanceBits;

static sb32 ndsSceneMipCachePresentSeedFrame(void)
{
    CObj *cobj;
    SObj *wallpaper_sobj;
    CObjVec saved_vec;
    Mtx44f saved_matrix;
    Vec2f saved_wallpaper_pos;
    f32 saved_target_dist;
    f32 saved_wallpaper_scale_x;
    f32 saved_wallpaper_scale_y;
    f32 seed_dist;
    u32 mip_index;

    if ((ndsPlatformSceneMipCacheReady() != FALSE) ||
        (ndsPlatformSceneMipCacheFailed() != FALSE))
    {
        return FALSE;
    }
    mip_index = ndsPlatformSceneMipCaptureCompletedCount();
    if ((mip_index >= NDS_SCENE_MIP_COUNT) ||
        (gGMCameraGObj == NULL) ||
        ((cobj = CObjGetStruct(gGMCameraGObj)) == NULL) ||
        (sGRWallpaperGObj == NULL) ||
        ((wallpaper_sobj = SObjGetStruct(sGRWallpaperGObj)) == NULL))
    {
        ndsPlatformSceneMipCacheAbort();
        return FALSE;
    }

    saved_vec = cobj->vec;
    saved_target_dist = gGMCameraStruct.target_dist;
    saved_wallpaper_pos = wallpaper_sobj->pos;
    saved_wallpaper_scale_x = wallpaper_sobj->sprite.scalex;
    saved_wallpaper_scale_y = wallpaper_sobj->sprite.scaley;
    memcpy(saved_matrix, gGMCameraMatrix, sizeof(saved_matrix));
    seed_dist = sNdsSceneMipSeedDistances[mip_index];
    cobj->vec.eye.x = cobj->vec.at.x;
    cobj->vec.eye.y = cobj->vec.at.y;
    cobj->vec.eye.z = cobj->vec.at.z + seed_dist;
    gGMCameraStruct.target_dist = seed_dist;
    grWallpaperCalcPersp(wallpaper_sobj);

    /* Request before gcDrawAll so the deferred SObj compositor snapshots the
     * affine seed transform as part of this exact held presentation frame. */
    if (ndsPlatformSceneMipCaptureRequest(mip_index) == FALSE)
    {
        cobj->vec = saved_vec;
        gGMCameraStruct.target_dist = saved_target_dist;
        wallpaper_sobj->pos = saved_wallpaper_pos;
        wallpaper_sobj->sprite.scalex = saved_wallpaper_scale_x;
        wallpaper_sobj->sprite.scaley = saved_wallpaper_scale_y;
        memcpy(gGMCameraMatrix, saved_matrix, sizeof(saved_matrix));
        ndsPlatformSceneMipCacheAbort();
        return FALSE;
    }

    ndsStageGCDrawAllLoopBeginHardwareFrame();
    sNdsStageGCDrawAllLoopHardwareSubmitActive = TRUE;
    ndsRendererAdapterResetDepthDiagnostics();
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareSetNoOracle(TRUE);
#endif
    gcDrawAll();
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareSetNoOracle(FALSE);
#endif
    sNdsStageGCDrawAllLoopHardwareSubmitActive = FALSE;
    cobj->vec = saved_vec;
    gGMCameraStruct.target_dist = saved_target_dist;
    wallpaper_sobj->pos = saved_wallpaper_pos;
    wallpaper_sobj->sprite.scalex = saved_wallpaper_scale_x;
    wallpaper_sobj->sprite.scaley = saved_wallpaper_scale_y;
    memcpy(gGMCameraMatrix, saved_matrix, sizeof(saved_matrix));
    gNdsSceneMipCacheSeedDrawCount++;
    return TRUE;
}

static sb32 ndsSceneMipCachePresentFrame(void)
{
    u32 mip_index = 0u;

    if (ndsPlatformSceneMipCacheReady() == FALSE)
    {
        return FALSE;
    }
    gNdsSceneMipCacheTargetDistanceBits =
        ndsFloatBits(gGMCameraStruct.target_dist);

    ndsStageGCDrawAllLoopBeginHardwareFrame();
    sNdsStageGCDrawAllLoopHardwareSubmitActive = TRUE;
    ndsRendererAdapterResetDepthDiagnostics();
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareSetNoOracle(TRUE);
#endif
    /* BG2 owns only BattleShip's retained wallpaper. Keep the original display
     * graph live for stage geometry, stage animation, fighters, effects, and
     * interface ordering; the SObj seam skips only the wallpaper CPU raster. */
    gcDrawAll();
    ndsFighterDisplayContractSubmitStageFighters();
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareSetNoOracle(FALSE);
#endif
    sNdsStageGCDrawAllLoopHardwareSubmitActive = FALSE;

    gNdsSceneMipCacheSelectedMipMask |= (1u << mip_index);
    gNdsSceneMipCacheSelectedMip = mip_index;
    gNdsSceneMipCacheDrawCount++;
    return TRUE;
}
#endif

u32 ndsSceneMipCacheHoldLogic(void)
{
#if NDS_SCENE_MIP_CACHE_LAB
    return ((ndsPlatformSceneMipCacheReady() == FALSE) &&
            (ndsPlatformSceneMipCacheFailed() == FALSE)) ? TRUE : FALSE;
#else
    return FALSE;
#endif
}

static void ndsStageGCDrawAllLoopSubmitHardwareFrame(void)
{
#if NDS_RENDERER_PROFILE_LEVEL == 1
    u32 owner_start;
    u32 profile_m3;
#endif

    if ((sNdsStageGCDrawAllLoopHardwareSubmitCount != 0u) ||
        (gSCManagerSceneData.scene_curr != nSCKindVSBattle))
    {
        return;
    }

    ndsStageGCDrawAllLoopBeginHardwareFrame();
    sNdsStageGCDrawAllLoopHardwareSubmitActive = TRUE;
    ndsRendererAdapterResetDepthDiagnostics();
#if NDS_RENDERER_PROFILE_LEVEL == 1
    profile_m3 = (gNdsRendererFastRunMode ==
        NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE) ? TRUE : FALSE;
    if (profile_m3 != FALSE)
    {
        owner_start = cpuGetTiming();
    }
#endif
    sNdsStageGCDrawAllLoopNativeStageArmed =
        ndsRendererAdapterPrepareNativeStageOwner(
            ndsBattleCompatMainCameraGObj());
#if NDS_RENDERER_PROFILE_LEVEL == 1
    if (profile_m3 != FALSE)
    {
        gNdsRendererProfileOwners[
            NDS_RENDERER_PROFILE_OWNER_STAGE].exclusive_ticks +=
                cpuGetTiming() - owner_start;
    }
#endif
    gcDrawAll();
    if (sNdsStageGCDrawAllLoopNativeStageArmed != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        owner_start = cpuGetTiming();
#endif
        ndsRendererAdapterFinishNativeStageOwner();
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererProfileOwners[
            NDS_RENDERER_PROFILE_OWNER_STAGE].exclusive_ticks +=
                cpuGetTiming() - owner_start;
#endif
        sNdsStageGCDrawAllLoopNativeStageArmed = FALSE;
    }
    ndsFighterDisplayContractSubmitStageFighters();
    sNdsStageGCDrawAllLoopHardwareSubmitActive = FALSE;
}

static void ndsStageGCDrawAllLoopPresentHardwareFrame(void)
{
#if NDS_TICK_HUD
    u32 tickhud_owner_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
    u32 owner_start;
    u32 profile_m3;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 owner_ticks;
    u32 phase05_present_start;
    u32 phase05_start;
#endif

    if (gSCManagerSceneData.scene_curr != nSCKindVSBattle)
    {
        return;
    }

#if NDS_FAST_WALLPAPER_AFFINE
    ndsFastWallpaperPrepareSeedSnapshot();
#endif

#if NDS_SCENE_MIP_CACHE_LAB
    if (ndsSceneMipCachePresentSeedFrame() != FALSE)
    {
        return;
    }
    if ((ndsPlatformSceneMipCacheReady() != FALSE) &&
        (ndsPlatformSceneMipCacheFailed() == FALSE))
    {
        if (ndsSceneMipCachePresentFrame() != FALSE)
        {
            return;
        }
        /* One uncovered view falsifies Cut G. Restore the complete source
         * renderer rather than oscillating between cache and generic state. */
        gNdsSceneMipCacheFallbackCount++;
        ndsPlatformSceneMipCacheAbort();
        ndsPlatformSetOriginalSpriteOverlayEnabled(TRUE);
    }
#endif

#if NDS_RENDERER_M3_PHASE0_PROFILE
    phase05_present_start = NDS_RENDERER_PHASE05_TICK();
#endif
    ndsStageGCDrawAllLoopBeginHardwareFrame();
    sNdsStageGCDrawAllLoopHardwareSubmitActive = TRUE;
    ndsRendererAdapterResetDepthDiagnostics();
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareSetNoOracle(TRUE);
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
    profile_m3 = (gNdsRendererFastRunMode ==
        NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE) ? TRUE : FALSE;
    if (profile_m3 != FALSE)
    {
        owner_start = cpuGetTiming();
    }
#endif
#if NDS_TICK_HUD
    tickhud_owner_start = cpuGetTiming();
#endif
    sNdsStageGCDrawAllLoopNativeStageArmed =
        ndsRendererAdapterPrepareNativeStageOwner(
            ndsBattleCompatMainCameraGObj());
#if NDS_TICK_HUD
    {
        u32 stage_ticks = cpuGetTiming() - tickhud_owner_start;

        gNdsTickHudStageTicks += stage_ticks;
#if NDS_TASK103_STAGE_RUN_PHASE
        gNdsTask103PrepareTicks += stage_ticks;
        gNdsTask103PrepareCount++;
#endif
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
    if (profile_m3 != FALSE)
    {
#if NDS_RENDERER_M3_PHASE0_PROFILE
        owner_ticks = cpuGetTiming() - owner_start;
        gNdsRendererProfileOwners[
            NDS_RENDERER_PROFILE_OWNER_STAGE].exclusive_ticks += owner_ticks;
        gNdsRendererPhase05StageTransitionTicks += owner_ticks;
#else
        gNdsRendererProfileOwners[
            NDS_RENDERER_PROFILE_OWNER_STAGE].exclusive_ticks +=
                cpuGetTiming() - owner_start;
#endif
    }
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif
    gcDrawAll();
#if NDS_RENDERER_M3_PHASE0_PROFILE
    NDS_RENDERER_PHASE05_FINISH(
        gNdsRendererPhase05GCDrawAllTicks, phase05_start);
#endif
    if (sNdsStageGCDrawAllLoopNativeStageArmed != FALSE)
    {
#if NDS_TICK_HUD
        tickhud_owner_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
        owner_start = cpuGetTiming();
#endif
        ndsRendererAdapterFinishNativeStageOwner();
#if NDS_TICK_HUD
        {
            u32 stage_ticks = cpuGetTiming() - tickhud_owner_start;

            gNdsTickHudStageTicks += stage_ticks;
#if NDS_TASK103_STAGE_RUN_PHASE
            gNdsTask103FinishTicks += stage_ticks;
            gNdsTask103FinishCount++;
#endif
        }
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
#if NDS_RENDERER_M3_PHASE0_PROFILE
        owner_ticks = cpuGetTiming() - owner_start;
        gNdsRendererProfileOwners[
            NDS_RENDERER_PROFILE_OWNER_STAGE].exclusive_ticks += owner_ticks;
        gNdsRendererPhase05StageTransitionTicks += owner_ticks;
#else
        gNdsRendererProfileOwners[
            NDS_RENDERER_PROFILE_OWNER_STAGE].exclusive_ticks +=
                cpuGetTiming() - owner_start;
#endif
#endif
        sNdsStageGCDrawAllLoopNativeStageArmed = FALSE;
    }
    ndsFighterDisplayContractSubmitStageFighters();
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareSetNoOracle(FALSE);
#endif
    sNdsStageGCDrawAllLoopHardwareSubmitActive = FALSE;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    NDS_RENDERER_PHASE05_FINISH(
        gNdsRendererPhase05PresentHardwareTicks, phase05_present_start);
#endif
}
#endif

void ndsFighterMarioFoxStageGCDrawAllLoopSubmitHardwareFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    ndsFighterMarioFoxStageGCDrawAllLoopPrepare();
    ndsStageGCDrawAllLoopSubmitHardwareFrame();
#endif
}

void ndsFighterMarioFoxStageGCDrawAllLoopPresentHardwareFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    ndsFighterMarioFoxStageGCDrawAllLoopPrepare();
    ndsStageGCDrawAllLoopPresentHardwareFrame();
#endif
}
