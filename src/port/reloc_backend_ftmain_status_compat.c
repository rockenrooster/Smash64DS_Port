static void ndsFTMainApplyCommonStatusReset(FTStruct *fp, u32 flags)
{
    if (fp == NULL)
    {
        return;
    }

    fp->is_reflect = FALSE;
    fp->is_absorb = FALSE;
    fp->is_shield = FALSE;
    if ((flags & FTSTATUS_PRESERVE_FASTFALL) == 0u)
    {
        fp->is_fastfall = FALSE;
    }
    fp->is_invisible = FALSE;
    fp->is_shadow_hide = FALSE;
    fp->is_playertag_hide = FALSE;
    fp->is_cliff_hold = FALSE;
    fp->is_jostle_ignore = FALSE;
    fp->is_hitstun = FALSE;
    fp->damage_mul = 1.0F;
    if ((fp->ga == nMPKineticsGround) &&
        ((flags & FTSTATUS_PRESERVE_DAMAGEPLAYER) == 0u))
    {
        fp->damage_player = -1;
    }
    fp->coll_data.ignore_line_id = -1;
    fp->capture_immune_mask = 0u;
    fp->is_ghost = FALSE;
    fp->camera_zoom_range = 1.0F;
    if ((flags & FTSTATUS_PRESERVE_PLAYERTAG) == 0u)
    {
        fp->playertag_wait = 0;
    }
    fp->is_special_interrupt = FALSE;
    fp->is_catchstatus = FALSE;
    fp->proc_damage = NULL;
    fp->proc_trap = NULL;
    fp->proc_shield = NULL;
    fp->proc_hit = NULL;
    fp->proc_lagstart = NULL;
    fp->proc_lagupdate = NULL;
    fp->proc_lagend = NULL;
    if ((flags & FTSTATUS_PRESERVE_SHUFFLETIME) == 0u)
    {
        fp->shuffle_tics = 0;
    }
    fp->knockback_resist_status = 0.0F;
    fp->damage_knockback_stack = 0.0F;
}

static sb32 ndsFTMainSetStatusNaturalCombatCovered(s32 status_id)
{
    return ((status_id == nFTCommonStatusWait) ||
            ((status_id >= nFTCommonStatusWalkSlow) &&
             (status_id <= nFTCommonStatusWalkFast)) ||
            (status_id == nFTCommonStatusDash) ||
            (status_id == nFTCommonStatusRun) ||
            (status_id == nFTCommonStatusRunBrake) ||
            (status_id == nFTCommonStatusTurn) ||
            (status_id == nFTCommonStatusTurnRun) ||
            (status_id == nFTCommonStatusAttack11) ||
            ((status_id >= nFTCommonStatusDamageStart) &&
             (status_id <= nFTCommonStatusDamageEnd)) ||
            (status_id == nFTCommonStatusGuardOn) ||
            (status_id == nFTCommonStatusGuard) ||
            (status_id == nFTCommonStatusGuardOff)) ? TRUE : FALSE;
}

static sb32 ndsFTMainSetStatusCliffLive(GObj *fighter_gobj, s32 status_id,
                                        f32 frame_begin, f32 anim_speed,
                                        u32 flags)
{
    FTStruct *fp;
    DObj *root;

    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() == FALSE) ||
        (sNdsStageMPCliffLiveLoopSetStatusActive == FALSE))
    {
        return FALSE;
    }
    if (ndsFTMainSetStatusNaturalCombatCovered(status_id) != FALSE)
    {
        return FALSE;
    }

    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        gNdsStageMPCliffLiveLoopUnsafeCount++;
        return TRUE;
    }
    if ((status_id != nFTCommonStatusCliffWait) &&
        (status_id != nFTCommonStatusCliffQuick) &&
        (status_id != nFTCommonStatusCliffSlow) &&
        (status_id != nFTCommonStatusFall) &&
        (status_id != nFTCommonStatusCliffClimbQuick1) &&
        (status_id != nFTCommonStatusCliffClimbQuick2) &&
        (status_id != nFTCommonStatusWait))
    {
        gNdsStageMPCliffLiveLoopUnsafeCount++;
        return TRUE;
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

    switch (status_id)
    {
    case nFTCommonStatusCliffWait:
        fp->motion_id = nFTCommonMotionCliffWait;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonCliffWaitProcInterrupt;
        fp->proc_physics = ftCommonCliffCommonProcPhysics;
        fp->proc_map = ftCommonCliffCommonProcMap;
        fp->ga = nMPKineticsGround;
        break;

    case nFTCommonStatusCliffQuick:
    case nFTCommonStatusCliffSlow:
        fp->motion_id = (status_id == nFTCommonStatusCliffQuick) ?
            nFTCommonMotionCliffQuick : nFTCommonMotionCliffSlow;
        fp->proc_update = (status_id == nFTCommonStatusCliffQuick) ?
            ftCommonCliffQuickProcUpdate : ftCommonCliffSlowProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftCommonCliffCommonProcPhysics;
        fp->proc_map = ftCommonCliffCommonProcMap;
        fp->ga = nMPKineticsGround;
        break;

    case nFTCommonStatusFall:
        fp->motion_id = nFTCommonMotionFall;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonFallProcInterrupt;
        fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
        fp->proc_map = mpCommonProcFighterCliffFloorCeil;
        fp->ga = nMPKineticsAir;
        break;

    case nFTCommonStatusCliffClimbQuick1:
        fp->motion_id = nFTCommonMotionCliffClimbQuick1;
        fp->proc_update = ftCommonCliffClimbQuick1ProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftCommonCliffCommonProcPhysics;
        fp->proc_map = ftCommonCliffCommonProcMap;
        fp->ga = nMPKineticsGround;
        break;

    case nFTCommonStatusCliffClimbQuick2:
        fp->motion_id = nFTCommonMotionCliffClimbQuick2;
        fp->proc_update = ftCommonCliffCommon2ProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftCommonCliffCommon2ProcPhysics;
        fp->proc_map = ftCommonCliffClimbCommon2ProcMap;
        fp->ga = nMPKineticsGround;
        break;

    case nFTCommonStatusWait:
    default:
        fp->motion_id = nFTCommonMotionWait;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonWaitProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->ga = nMPKineticsGround;
        break;
    }

    fp->motion_script_id = fp->motion_id;
    if (fighter_gobj != NULL)
    {
        fighter_gobj->anim_frame = frame_begin;
        root = DObjGetStruct(fighter_gobj);
        if (root != NULL)
        {
            root->anim_speed = anim_speed;
        }
    }
    gNdsStageMPCliffLiveLoopCallbackSourceMask |= 1u << 6;
    return TRUE;
}

sb32 ndsDiagnosticsHandleImportedFTMainSetStatusBefore(GObj *fighter_gobj,
                                                       s32 status_id,
                                                       f32 frame_begin,
                                                       f32 anim_speed,
                                                       u32 flags)
{
#if NDS_FIGHTER_ANIM_AUDIT
    if ((sNdsFighterAnimAuditStarted != FALSE) &&
        (sNdsFighterAnimAuditInternalSetStatus == FALSE) &&
        (fighter_gobj == sNdsFighterAnimAuditTarget))
    {
        return TRUE;
    }
#endif
    return ndsFTMainSetStatusCliffLive(fighter_gobj, status_id, frame_begin,
                                       anim_speed, flags);
}

static sb32 ndsFTMainSetStatusDamageHarness(GObj *fighter_gobj,
                                            s32 status_id, f32 frame_begin,
                                            f32 anim_speed, u32 flags)
{
    FTStruct *fp;
    DObj *root;
    s32 motion_id;
    sb32 is_air_status;

    if ((ndsFighterMarioFoxDashRunProofEnabled() == FALSE) ||
        (sNdsFighterDashRunDamageStatusSetupActive == FALSE) ||
        (ndsFTCommonDamageIsStatus(status_id) == FALSE))
    {
        return FALSE;
    }

    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        return TRUE;
    }

    motion_id = ndsFTCommonDamageMotionForStatus(status_id);
    if (motion_id < 0)
    {
        return TRUE;
    }

    is_air_status =
        ((status_id >= nFTCommonStatusDamageAir1) &&
         (status_id <= nFTCommonStatusDamageFlyRoll)) ? TRUE : FALSE;

    ndsFTMainApplyCommonStatusReset(fp, flags);
    fp->status_prev = fp->status_id;
    fp->status_id = status_id;
    fp->status_total_tics = 0;
    fp->motion_id = motion_id;
    fp->motion_script_id = motion_id;
    fp->motion_attack_id = nFTMotionAttackIDNone;
    fp->status_attack_id = nFTStatusAttackIDNone;
    fp->stat_attack_id = nFTStatusAttackIDNone;
    fp->status_is_smash = FALSE;
    fp->status_is_projectile = FALSE;
    fp->status_flags = flags;
    fp->motion_frame = frame_begin;
    fp->anim_frame = frame_begin;
    fp->anim_speed = anim_speed;
    fp->proc_update = (is_air_status != FALSE) ?
        ftCommonDamageAirCommonProcUpdate : ftCommonDamageCommonProcUpdate;
    fp->proc_interrupt = (is_air_status != FALSE) ?
        ftCommonDamageAirCommonProcInterrupt : ftCommonDamageCommonProcInterrupt;
    fp->proc_physics = ftCommonDamageCommonProcPhysics;
    fp->proc_map = (is_air_status != FALSE) ?
        ftCommonDamageAirCommonProcMap : mpCommonProcFighterOnCliffEdge;
    fp->proc_damage = NULL;
    fp->proc_status = NULL;
    fp->ga = (is_air_status != FALSE) ? nMPKineticsAir : nMPKineticsGround;

    if (fighter_gobj != NULL)
    {
        fighter_gobj->anim_frame = frame_begin;
        root = DObjGetStruct(fighter_gobj);
        if (root != NULL)
        {
            root->anim_speed = anim_speed;
        }
    }
    return TRUE;
}

static void ndsFTMainSetStatusCompatHarness(GObj *fighter_gobj,
                                            s32 status_id, f32 frame_begin,
                                            f32 anim_speed, u32 flags);

static sb32 ndsFTMainSetStatusStageCompatActive(void)
{
    return
        (sNdsStageMPCeilStatusFloorLoopSetStatusActive != FALSE) ||
        (sNdsStageMPCliffAttackActionLoopSetStatusActive != FALSE) ||
        (sNdsStageMPCliffAttackFloorLoopSetStatusActive != FALSE) ||
        (sNdsStageMPCliffCatchFloorLoopSetStatusActive != FALSE) ||
        (sNdsStageMPCliffClimbActionLoopSetStatusActive != FALSE) ||
        (sNdsStageMPCliffClimbFinishLoopUpdateActive != FALSE) ||
        (sNdsStageMPCliffClimbFloorLoopRecatchSetStatusActive != FALSE) ||
        (sNdsStageMPCliffClimbFloorLoopSetStatusActive != FALSE) ||
        (sNdsStageMPCliffEscapeActionLoopSetStatusActive != FALSE) ||
        (sNdsStageMPCliffStatusFloorLoopStatusActive != FALSE) ||
        (sNdsStageMPCliffTickFloorLoopStatusActive != FALSE) ||
        (sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive != FALSE) ||
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive != FALSE) ||
        (sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive != FALSE) ||
        (sNdsStageMPCliffWaitDamageLoopDownStandSetStatusActive != FALSE) ||
        (sNdsStageMPCliffWaitDamageLoopDownWaitSetStatusActive != FALSE) ||
        (sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive != FALSE) ||
        (sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive != FALSE) ||
        (sNdsStageMPCliffWaitDamageLoopSetStatusActive != FALSE) ||
        (sNdsStageMPCliffWaitFloorLoopSetStatusActive != FALSE) ||
        (sNdsStageMPDownRecoverLoopAttackUpdateActive != FALSE) ||
        (sNdsStageMPDownRecoverLoopDownAttackSetStatusActive != FALSE) ||
        (sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive != FALSE) ||
        (sNdsStageMPDownRecoverLoopDownStandSetStatusActive != FALSE) ||
        (sNdsStageMPDownRecoverLoopDownStandUpdateActive != FALSE) ||
        (sNdsStageMPDownRecoverLoopDownWaitSetStatusActive != FALSE) ||
        (sNdsStageMPDownRecoverLoopRollBackUpdateActive != FALSE) ||
        (sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE) ||
        (sNdsStageMPDownRecoverLoopRollForwardUpdateActive != FALSE) ||
        (sNdsStageMPDownWaitLoopAttackUpdateActive != FALSE) ||
        (sNdsStageMPDownWaitLoopDownAttackSetStatusActive != FALSE) ||
        (sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive != FALSE) ||
        (sNdsStageMPDownWaitLoopDownStandSetStatusActive != FALSE) ||
        (sNdsStageMPDownWaitLoopDownStandUpdateActive != FALSE) ||
        (sNdsStageMPDownWaitLoopDownWaitSetStatusActive != FALSE) ||
        (sNdsStageMPDownWaitLoopRollBackUpdateActive != FALSE) ||
        (sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE) ||
        (sNdsStageMPDownWaitLoopRollForwardUpdateActive != FALSE) ||
        (sNdsStageMPFallLandFloorLoopSetStatusActive != FALSE) ||
        (sNdsStageMPLiveHitStatusLoopCliffCatchSetStatusActive != FALSE) ||
        (sNdsStageMPLiveHitStatusLoopDownBounceSetStatusActive != FALSE) ||
        (sNdsStageMPPassInputLoopStatusActive != FALSE) ||
        (sNdsStageMPPassiveLoopAppealActive != FALSE) ||
        (sNdsStageMPPassiveLoopAppealGuardActive != FALSE) ||
        (sNdsStageMPPassiveLoopBranchProbeActive != FALSE) ||
        (sNdsStageMPPassiveLoopCaptureActive != FALSE) ||
        (sNdsStageMPPassiveLoopCaptureMapActive != FALSE) ||
        (sNdsStageMPPassiveLoopCapturePhysicsActive != FALSE) ||
        (sNdsStageMPPassiveLoopCaptureWaitMapActive != FALSE) ||
        (sNdsStageMPPassiveLoopCatchActive != FALSE) ||
        (sNdsStageMPPassiveLoopCatchMapActive != FALSE) ||
        (sNdsStageMPPassiveLoopCatchPullActive != FALSE) ||
        (sNdsStageMPPassiveLoopCatchPullUpdateActive != FALSE) ||
        (sNdsStageMPPassiveLoopCatchUpdateActive != FALSE) ||
        (sNdsStageMPPassiveLoopCatchWaitInterruptActive != FALSE) ||
        (sNdsStageMPPassiveLoopDamageFallMapActive != FALSE) ||
        (sNdsStageMPPassiveLoopPassiveCallbackActive != FALSE) ||
        (sNdsStageMPPassiveLoopPassiveSetStatusActive != FALSE) ||
        (sNdsStageMPPassiveLoopPassiveStandBActive != FALSE) ||
        (sNdsStageMPPassiveLoopPassiveStandCallbackActive != FALSE) ||
        (sNdsStageMPPassiveLoopPassiveStandSetStatusActive != FALSE) ||
        (sNdsStageMPPassiveLoopPassiveStandUpdateActive != FALSE) ||
        (sNdsStageMPPassiveLoopPassiveUpdateActive != FALSE) ||
        (sNdsStageMPPassiveLoopReboundActive != FALSE) ||
        (sNdsStageMPPassiveLoopReboundUpdateActive != FALSE) ||
        (sNdsStageMPPassiveLoopThrowActive != FALSE) ||
        (sNdsStageMPPassiveLoopThrowCallbackImmediateActive != FALSE) ||
        (sNdsStageMPPassiveLoopThrowDeadResultActive != FALSE) ||
        (sNdsStageMPPassiveLoopThrowProcStatusActive != FALSE) ||
        (sNdsStageMPPassiveLoopThrowReleaseActive != FALSE) ||
        (sNdsStageMPPassiveLoopThrowReleaseStatusActive != FALSE) ||
        (sNdsStageMPPassiveLoopThrowUpdateActive != FALSE) ||
        (sNdsStageMPPassiveLoopWallDamageActive != FALSE) ||
        (sNdsStageTurnLoopFinalUpdateActive != FALSE) ||
        (sNdsStageTurnLoopSetStatusActive != FALSE);
}


void ndsDiagnosticsRecordImportedFTMainAnimEvents(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    FTStruct *moveset_fp;
    u32 moveset_i;
#endif

#if NDS_FIGHTER_ANIM_AUDIT
    ndsFighterAnimAuditUpdate(fighter_gobj);
#endif

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    moveset_fp = ftGetStruct(fighter_gobj);
    if (((gNdsFighterNaturalMovesetPhase == 9u) ||
         (gNdsFighterNaturalMovesetPhase == 10u)) &&
        (moveset_fp != NULL) &&
        (moveset_fp->status_id >= nFTCommonStatusAttackAirStart) &&
        (moveset_fp->status_id <= nFTCommonStatusAttackAirEnd))
    {
        for (moveset_i = 0u; moveset_i < ARRAY_COUNT(moveset_fp->attack_colls);
             moveset_i++)
        {
            if (moveset_fp->attack_colls[moveset_i].attack_state !=
                nGMAttackStateOff)
            {
                gNdsFighterNaturalMovesetAerialHitboxFrames++;
                break;
            }
        }
    }
#endif

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowCallbackImmediateActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPPassiveLoopThrowCallbackImmediateAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPPassiveLoopThrowAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
        (sNdsFighterJumpAttackAirActive != FALSE))
    {
        gNdsFighterJumpAttackAirAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunGuardOnActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) &&
            (fp->status_id == nFTCommonStatusGuardOn) &&
            (fp->motion_id == nFTCommonMotionGuardOn))
        {
            if (fp == &sNdsFighterStructPool[0])
            {
                gNdsFighterDashRunGuardAnimEventsMask |= 1u << 0u;
            }
            else if (fp == &sNdsFighterStructPool[1])
            {
                gNdsFighterDashRunGuardAnimEventsMask |= 1u << 1u;
            }
        }
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) && (fp->player < 2))
        {
            u32 event_bit = 0xffffffffu;

            if (fp->status_id == nFTCommonStatusAttack11)
            {
                event_bit = fp->player;
            }
            else if (fp->status_id == nFTCommonStatusAttack12)
            {
                event_bit = fp->player + 2u;
            }
            else if ((fp->status_id == nFTMarioStatusAttack13) &&
                (fp->player == 0))
            {
                event_bit = 4u;
            }
            else if ((fp->status_id == nFTFoxStatusAttack100Start) &&
                (fp->player == 1))
            {
                event_bit = 5u;
            }
            if (event_bit != 0xffffffffu)
            {
                gNdsFighterDashRunAttackAnimEventsMask |=
                    1u << event_bit;
            }
        }
    }
    if ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffCatchFloorLoopPlayAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopCliffCatchPlayAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCeilStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCeilStatusFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCeilStatusFloorLoopPlayAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffAttackFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffAttackFloorLoopAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffClimbFloorLoopAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopRecatchSetStatusActive != FALSE))
    {
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffEscapeActionLoopAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownAttackSetStatusActive != FALSE))
    {
        gNdsStageMPDownWaitLoopAttackAnimEventsCount++;
        ndsStageMPDownWaitLoopAppendAttackOrder(4u);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollAnimEventsCount++;
        if (sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE)
        {
            ndsStageMPDownWaitLoopAppendRollForwardOrder(5u);
        }
        else
        {
            ndsStageMPDownWaitLoopAppendRollBackOrder(5u);
        }
        return;
    }
    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopSetStatusActive != FALSE))
    {
        gNdsStageTurnLoopAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownAttackSetStatusActive != FALSE))
    {
        gNdsStageMPDownRecoverLoopAttackAnimEventsCount++;
        ndsStageMPDownRecoverLoopAppendAttackOrder(4u);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive != FALSE))
    {
        gNdsStageMPDownRecoverLoopRollAnimEventsCount++;
        if (sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE)
        {
            ndsStageMPDownRecoverLoopAppendRollForwardOrder(5u);
        }
        else
        {
            ndsStageMPDownRecoverLoopAppendRollBackOrder(5u);
        }
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopActive != FALSE))
    {
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        gNdsFighterDashRunAnimEventsCallCount++;
    }

    if (ndsFighterMarioFoxWalkInputProofEnabled() != FALSE)
    {
        gNdsFighterWalkAnimEventsCallCount++;
    }
}

void ndsDiagnosticsRecordImportedFTMainSetStatus(GObj *fighter_gobj,
                                                s32 status_id,
                                                f32 frame_begin,
                                                f32 anim_speed, u32 flags)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 motion_id;
    s32 attackair_index;
    sb32 handled_status = FALSE;

    (void)frame_begin;

    if (fp == NULL)
    {
        return;
    }

#if NDS_FIGHTER_ANIM_AUDIT
    ndsFighterAnimAuditRecordSetStatus(fp);
#endif

    if ((status_id != nFTCommonStatusWait) &&
        (ndsFTMainSetStatusNaturalCombatCovered(status_id) == FALSE) &&
        (ndsFTMainSetStatusStageCompatActive() != FALSE))
    {
        sNdsFTMainSetStatusCompatReplayActive = TRUE;
        ndsFTMainSetStatusCompatHarness(fighter_gobj, status_id,
                                        frame_begin, anim_speed, flags);
        sNdsFTMainSetStatusCompatReplayActive = FALSE;
        fp = ftGetStruct(fighter_gobj);
        if (fp == NULL)
        {
            return;
        }
    }

    if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
    {
        gNdsFighterWaitFtMainSetStatusCallCount++;
    }

    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        ((status_id == nFTCommonStatusFall) ||
        (status_id == nFTCommonStatusFallAerial) ||
        (status_id == nFTCommonStatusLandingLight) ||
        (status_id == nFTCommonStatusLandingHeavy)))
    {
        if ((status_id == nFTCommonStatusFall) &&
            (fp->status_id == nFTCommonStatusFall) &&
            (fp->motion_id == nFTCommonMotionFall))
        {
            gNdsFighterLandingFtMainFallStatusCount++;
            handled_status = TRUE;
        }
        else if ((status_id == nFTCommonStatusLandingLight) &&
            (fp->status_id == nFTCommonStatusLandingLight) &&
            (fp->motion_id == nFTCommonMotionLandingLight))
        {
            gNdsFighterLandingFtMainLandingLightStatusCount++;
            handled_status = TRUE;
        }
        else if ((status_id == nFTCommonStatusLandingHeavy) &&
            (fp->status_id == nFTCommonStatusLandingHeavy) &&
            (fp->motion_id == nFTCommonMotionLandingHeavy))
        {
            gNdsFighterLandingFtMainLandingHeavyStatusCount++;
            gNdsFighterLandingHeavyDeniedCount++;
            handled_status = TRUE;
        }
        else if (status_id == nFTCommonStatusFallAerial)
        {
            gNdsFighterLandingFallAerialDeniedCount++;
            gNdsFighterLandingDeniedStatusCount++;
            handled_status = TRUE;
        }
    }

    if (ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE)
    {
        if ((status_id == nFTCommonStatusKneeBend) &&
            (fp->status_id == nFTCommonStatusKneeBend) &&
            (fp->motion_id == nFTCommonMotionKneeBend))
        {
            gNdsFighterJumpFtMainKneeBendStatusCount++;
            handled_status = TRUE;
        }
        else if (((status_id == nFTCommonStatusJumpF) &&
            (fp->status_id == nFTCommonStatusJumpF) &&
            (fp->motion_id == nFTCommonMotionJumpF)) ||
            ((status_id == nFTCommonStatusJumpB) &&
            (fp->status_id == nFTCommonStatusJumpB) &&
            (fp->motion_id == nFTCommonMotionJumpB)))
        {
            if (status_id == nFTCommonStatusJumpB)
            {
                gNdsFighterJumpUnexpectedStatusCount++;
            }
            gNdsFighterJumpFtMainJumpStatusCount++;
            handled_status = TRUE;
        }
        else if ((status_id >= nFTCommonStatusAttackAirStart) &&
            (status_id <= nFTCommonStatusAttackAirEnd) &&
            (ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE))
        {
            attackair_index = status_id - nFTCommonStatusAttackAirStart;
            if ((fp->status_id == status_id) &&
                (fp->motion_id ==
                    (nFTCommonMotionAttackAirStart + attackair_index)))
            {
                handled_status = TRUE;
                if ((status_id == nFTCommonStatusAttackAirN) &&
                    (sNdsFighterJumpAttackAirActive != FALSE))
                {
                    gNdsFighterJumpAttackAirSetStatusCount++;
                    gNdsFighterJumpFtMainAttackAirStatusCount++;
                }
            }
        }
        else if (((status_id == nFTCommonStatusLandingAirN) ||
            (status_id == nFTCommonStatusLandingAirNull)) &&
            (ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
            (sNdsFighterJumpAttackAirMapLandingActive != FALSE))
        {
            gNdsFighterJumpAttackAirMapLandingMask |= 1u << 2u;
            handled_status = TRUE;
        }
    }

    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        if ((status_id == nFTMarioStatusAttack13) &&
            (fp->fkind == nFTKindMario) &&
            (fp->status_id == nFTMarioStatusAttack13) &&
            (fp->motion_id == nFTMarioMotionAttack13))
        {
            gNdsFighterDashRunAttack13SetStatusCount++;
            gNdsFighterDashRunFtMainAttack13StatusCount++;
            handled_status = TRUE;
        }
        else if ((status_id == nFTFoxStatusAttack100Start) &&
            ((fp->fkind == nFTKindFox) ||
            (fp->fkind == nFTKindNFox)) &&
            (fp->status_id == nFTFoxStatusAttack100Start) &&
            (fp->motion_id == nFTFoxMotionAttack100Start))
        {
            gNdsFighterDashRunFtMainAttack100StartStatusCount++;
            handled_status = TRUE;
        }
        else if ((status_id == nFTFoxStatusAttack100Loop) &&
            ((fp->fkind == nFTKindFox) ||
            (fp->fkind == nFTKindNFox)) &&
            (fp->status_id == nFTFoxStatusAttack100Loop) &&
            (fp->motion_id == nFTFoxMotionAttack100Loop))
        {
            gNdsFighterDashRunFtMainAttack100LoopStatusCount++;
            handled_status = TRUE;
        }
        else if ((status_id == nFTFoxStatusAttack100End) &&
            ((fp->fkind == nFTKindFox) ||
            (fp->fkind == nFTKindNFox)) &&
            (fp->status_id == nFTFoxStatusAttack100End) &&
            (fp->motion_id == nFTFoxMotionAttack100End))
        {
            handled_status = TRUE;
        }

        switch (status_id)
        {
        case nFTCommonStatusDash:
            if ((fp->status_id == nFTCommonStatusDash) &&
                (fp->motion_id == nFTCommonMotionDash))
            {
                gNdsFighterDashRunDashSetStatusCount++;
                gNdsFighterDashRunFtMainDashStatusCount++;
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusRun:
            if ((fp->status_id == nFTCommonStatusRun) &&
                (fp->motion_id == nFTCommonMotionRun))
            {
                gNdsFighterDashRunRunSetStatusCount++;
                gNdsFighterDashRunFtMainRunStatusCount++;
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusTurnRun:
            if ((fp->status_id == nFTCommonStatusTurnRun) &&
                (fp->motion_id == nFTCommonMotionTurnRun))
            {
                gNdsFighterDashRunTurnRunSetStatusCount++;
                gNdsFighterDashRunFtMainTurnRunStatusCount++;
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusRunBrake:
            if ((fp->status_id == nFTCommonStatusRunBrake) &&
                (fp->motion_id == nFTCommonMotionRunBrake))
            {
                gNdsFighterDashRunRunBrakeSetStatusCount++;
                gNdsFighterDashRunFtMainRunBrakeStatusCount++;
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusAttack11:
            if ((fp->status_id == nFTCommonStatusAttack11) &&
                (fp->motion_id == nFTCommonMotionAttack11))
            {
                gNdsFighterDashRunAttack11SetStatusCount++;
                gNdsFighterDashRunFtMainAttack11StatusCount++;
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusAttack12:
            if ((fp->status_id == nFTCommonStatusAttack12) &&
                (fp->motion_id == nFTCommonMotionAttack12))
            {
                gNdsFighterDashRunAttack12SetStatusCount++;
                gNdsFighterDashRunFtMainAttack12StatusCount++;
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusAttackDash:
            if ((fp->status_id == nFTCommonStatusAttackDash) &&
                (fp->motion_id == nFTCommonMotionAttackDash))
            {
                gNdsFighterDashRunAttackDashSetStatusCount++;
                gNdsFighterDashRunFtMainAttackDashStatusCount++;
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusGuardOn:
            if ((fp->status_id == nFTCommonStatusGuardOn) &&
                (fp->motion_id == nFTCommonMotionGuardOn))
            {
                gNdsFighterDashRunGuardSetStatusCount++;
                gNdsFighterDashRunFtMainGuardOnStatusCount++;
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusGuard:
            if ((fp->status_id == nFTCommonStatusGuard) &&
                (fp->motion_id == nFTCommonMotionGuardOn))
            {
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusGuardOff:
            if ((fp->status_id == nFTCommonStatusGuardOff) &&
                (fp->motion_id == nFTCommonMotionGuardOff))
            {
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusGuardSetOff:
            if (fp->status_id == nFTCommonStatusGuardSetOff)
            {
                gNdsFighterDashRunGuardSetOffSetStatusCount++;
                gNdsFighterDashRunFtMainGuardSetOffStatusCount++;
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusEscapeF:
            if ((fp->status_id == nFTCommonStatusEscapeF) &&
                (fp->motion_id == nFTCommonMotionEscapeF))
            {
                fp->proc_status = ndsBaseFTCommonEscapeProcStatus;
                gNdsFighterDashRunEscapeSetStatusCount++;
                gNdsFighterDashRunFtMainEscapeStatusCount++;
                handled_status = TRUE;
            }
            break;

        case nFTCommonStatusEscapeB:
            if ((fp->status_id == nFTCommonStatusEscapeB) &&
                (fp->motion_id == nFTCommonMotionEscapeB))
            {
                fp->proc_status = ndsBaseFTCommonEscapeProcStatus;
                gNdsFighterDashRunEscapeSetStatusCount++;
                gNdsFighterDashRunFtMainEscapeStatusCount++;
                handled_status = TRUE;
            }
            break;

        default:
            break;
        }
    }

    if ((handled_status == FALSE) &&
        (ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE) &&
        (status_id == nFTCommonStatusAttackAirLw) &&
        (fp->status_id == nFTCommonStatusAttackAirLw) &&
        (fp->motion_id == nFTCommonMotionAttackAirLw))
    {
        handled_status = TRUE;
    }

#if NDS_IMPORT_BATTLESHIP_FTMAIN
    if ((handled_status == FALSE) &&
        (ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageFallSetStatusFromDamageActive != FALSE) &&
        (status_id == nFTCommonStatusDamageFall) &&
        (fp->status_id == nFTCommonStatusDamageFall) &&
        (fp->motion_id == nFTCommonMotionDamageFall) &&
        (fp->ga == nMPKineticsAir) &&
        (fp->proc_interrupt == ftCommonDamageFallProcInterrupt) &&
        (fp->proc_physics == ftPhysicsApplyAirVelDriftFastFall) &&
        (fp->proc_map == ftCommonDamageFallProcMap))
    {
        sNdsFighterDashRunDamageFallFTMainSetStatusCount++;
        handled_status = TRUE;
    }
    if ((handled_status == FALSE) &&
        (ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopWallDamageFallSetStatusFromDamageActive !=
            FALSE) &&
        (status_id == nFTCommonStatusDamageFall) &&
        (fp->status_id == nFTCommonStatusDamageFall) &&
        (fp->motion_id == nFTCommonMotionDamageFall) &&
        (fp->ga == nMPKineticsAir) &&
        (fp->proc_interrupt == ftCommonDamageFallProcInterrupt) &&
        (fp->proc_physics == ftPhysicsApplyAirVelDriftFastFall) &&
        (fp->proc_map == ftCommonDamageFallProcMap))
    {
        sNdsStageMPPassiveLoopWallDamageFallFTMainSetStatusCount++;
        handled_status = TRUE;
    }
#endif

    if (handled_status != FALSE)
    {
        return;
    }

    if ((ndsFighterMarioFoxWalkInputProofEnabled() != FALSE) &&
        (status_id >= nFTCommonStatusWalkSlow) &&
        (status_id <= nFTCommonStatusWalkFast))
    {
        motion_id = nFTCommonMotionWalkSlow +
            (status_id - nFTCommonStatusWalkSlow);
        if ((fp->status_id == status_id) && (fp->motion_id == motion_id))
        {
            gNdsFighterWalkSetStatusCallCount++;
            gNdsFighterWalkFtMainSetStatusCallCount++;
        }
        return;
    }

    if (status_id != nFTCommonStatusWait)
    {
        if (ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE)
        {
            gNdsFighterWalkLoopDeniedStatusCount++;
            gNdsFighterWalkLoopUnexpectedStatusCount++;
        }
        if (ndsFighterMarioFoxWalkInputProofEnabled() != FALSE)
        {
            gNdsFighterWalkDeniedStatusCount++;
            gNdsFighterWalkUnexpectedStatusCount++;
        }
        if (ndsFighterMarioFoxWaitTickProofEnabled() != FALSE)
        {
            gNdsFighterWaitTickDeniedStatusCount++;
        }
        return;
    }

    if ((fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait))
    {
        return;
    }

    fp->is_wait_status_setup = TRUE;
    fp->is_wait_motion_setup = TRUE;

    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpRunBrakeEndActive != FALSE))
    {
        gNdsFighterJumpWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingEndActive != FALSE))
    {
        gNdsFighterLandingWaitSetStatusCount++;
        gNdsFighterLandingWaitSetStatusSuccessCount++;
    }
    if ((ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE) &&
        (sNdsFighterWalkLoopWaitReturnActive != FALSE))
    {
        gNdsFighterWalkLoopWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopAttackUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopAttackWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollForwardUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollForwardWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollBackUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollBackWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopDownStandWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopFinalUpdateActive != FALSE))
    {
        gNdsStageTurnLoopWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPDownRecoverLoopDownStandUpdateActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopAttackUpdateActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopRollForwardUpdateActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopRollBackUpdateActive != FALSE)))
    {
        gNdsStageMPDownRecoverLoopWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveStandWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopReboundUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopReboundFinalWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassiveStandWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassiveWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFinishLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFinishLoopUpdateActive != FALSE))
    {
        gNdsStageMPCliffClimbFinishLoopWaitSetStatusCount++;
    }
}

static void ndsFTMainSetStatusCompatHarness(GObj *fighter_gobj,
                                            s32 status_id, f32 frame_begin,
                                            f32 anim_speed, u32 flags)
{
    FTStruct *fp;

    if (ndsFTMainSetStatusCliffLive(fighter_gobj, status_id, frame_begin,
            anim_speed, flags) != FALSE)
    {
        return;
    }

    if (status_id == nFTCommonStatusShieldBreakFly)
    {
        DObj *root;

        fp = ftGetStruct(fighter_gobj);
        if (fp == NULL)
        {
            return;
        }
        ndsFTMainApplyCommonStatusReset(fp, flags);
        fp->status_prev = fp->status_id;
        fp->status_id = status_id;
        fp->status_total_tics = 0;
        fp->motion_id = nFTCommonMotionShieldBreakFly;
        fp->motion_script_id = nFTCommonMotionShieldBreakFly;
        fp->motion_attack_id = nFTMotionAttackIDNone;
        fp->status_attack_id = nFTStatusAttackIDNone;
        fp->stat_attack_id = nFTStatusAttackIDNone;
        fp->status_is_smash = FALSE;
        fp->status_is_projectile = FALSE;
        fp->status_flags = flags;
        fp->motion_frame = frame_begin;
        fp->anim_frame = frame_begin;
        fp->anim_speed = anim_speed;
        fp->proc_update = NULL;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
        fp->proc_map = ftCommonDamageFallProcMap;
        fp->proc_damage = NULL;
        fp->proc_status = NULL;
        fp->ga = nMPKineticsAir;
        if (fighter_gobj != NULL)
        {
            fighter_gobj->anim_frame = frame_begin;
            root = DObjGetStruct(fighter_gobj);
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if (status_id == nFTCommonStatusFuraSleep)
    {
        DObj *root;

        fp = ftGetStruct(fighter_gobj);
        if (fp == NULL)
        {
            return;
        }
        ndsFTMainApplyCommonStatusReset(fp, flags);
        fp->status_prev = fp->status_id;
        fp->status_id = status_id;
        fp->status_total_tics = 0;
        fp->motion_id = nFTCommonMotionFuraSleep;
        fp->motion_script_id = nFTCommonMotionFuraSleep;
        fp->motion_attack_id = nFTMotionAttackIDNone;
        fp->status_attack_id = nFTStatusAttackIDNone;
        fp->stat_attack_id = nFTStatusAttackIDNone;
        fp->status_is_smash = FALSE;
        fp->status_is_projectile = FALSE;
        fp->status_flags = flags;
        fp->motion_frame = frame_begin;
        fp->anim_frame = frame_begin;
        fp->anim_speed = anim_speed;
        fp->proc_update = ndsBaseFTCommonFuraSleepProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->proc_damage = NULL;
        fp->proc_status = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            fighter_gobj->anim_frame = frame_begin;
            root = DObjGetStruct(fighter_gobj);
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if (status_id == nFTCommonStatusTwister)
    {
        DObj *root;

        fp = ftGetStruct(fighter_gobj);
        if (fp == NULL)
        {
            return;
        }
        ndsFTMainApplyCommonStatusReset(fp, flags);
        fp->status_prev = fp->status_id;
        fp->status_id = status_id;
        fp->status_total_tics = 0;
        fp->motion_id = nFTCommonMotionTwister;
        fp->motion_script_id = nFTCommonMotionTwister;
        fp->motion_attack_id = nFTMotionAttackIDNone;
        fp->status_attack_id = nFTStatusAttackIDNone;
        fp->stat_attack_id = nFTStatusAttackIDNone;
        fp->status_is_smash = FALSE;
        fp->status_is_projectile = FALSE;
        fp->status_flags = flags;
        fp->motion_frame = frame_begin;
        fp->anim_frame = frame_begin;
        fp->anim_speed = anim_speed;
        fp->proc_update = ndsBaseFTCommonTwisterProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ndsBaseFTCommonTwisterProcPhysics;
        /* ponytail: Twister map projection waits for mpCommonProcFighterProject. */
        fp->proc_map = NULL;
        fp->proc_damage = NULL;
        fp->proc_status = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            fighter_gobj->anim_frame = frame_begin;
            root = DObjGetStruct(fighter_gobj);
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if (status_id == nFTCommonStatusTaruCann)
    {
        DObj *root;

        fp = ftGetStruct(fighter_gobj);
        if (fp == NULL)
        {
            return;
        }
        ndsFTMainApplyCommonStatusReset(fp, flags);
        fp->status_prev = fp->status_id;
        fp->status_id = status_id;
        fp->status_total_tics = 0;
        fp->motion_id = nFTCommonMotionNull;
        fp->motion_script_id = nFTCommonMotionNull;
        fp->motion_attack_id = nFTMotionAttackIDNone;
        fp->status_attack_id = nFTStatusAttackIDNone;
        fp->stat_attack_id = nFTStatusAttackIDNone;
        fp->status_is_smash = FALSE;
        fp->status_is_projectile = FALSE;
        fp->status_flags = flags;
        fp->motion_frame = frame_begin;
        fp->anim_frame = frame_begin;
        fp->anim_speed = anim_speed;
        /* ponytail: TaruCannon update/shoot waits for Jungle barrel runtime. */
        fp->proc_update = NULL;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftCommonTaruCannProcPhysics;
        fp->proc_map = NULL;
        fp->proc_damage = NULL;
        fp->proc_status = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            fighter_gobj->anim_frame = frame_begin;
            root = DObjGetStruct(fighter_gobj);
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if (ndsFTMainSetStatusDamageHarness(fighter_gobj, status_id, frame_begin,
            anim_speed, flags) != FALSE)
    {
        return;
    }

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunDamageExpiryActive != FALSE) &&
        (status_id == nFTCommonStatusDamageFall))
    {
        DObj *root;

        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
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
        fp->motion_id = nFTCommonMotionDamageFall;
        fp->motion_script_id = nFTCommonMotionDamageFall;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonDamageFallProcInterrupt;
        fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
        fp->proc_map = ftCommonDamageFallProcMap;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsAir;
        if (fighter_gobj != NULL)
        {
            fighter_gobj->anim_frame = frame_begin;
            root = DObjGetStruct(fighter_gobj);
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        sNdsFighterDashRunDamageFallFTMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopActive != FALSE) &&
        ((ndsFighterMarioFoxJumpLoopProofEnabled() == FALSE) ||
         ((status_id != nFTCommonStatusKneeBend) &&
          (status_id != nFTCommonStatusJumpF) &&
          (status_id != nFTCommonStatusJumpB) &&
          (status_id != nFTCommonStatusAttackAirN))))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsFighterProcessLoopDeniedStatusCount++;
            return;
        }
        ndsFighterProcessLoopSetStatus(fp, fighter_gobj, status_id,
                                       frame_begin, anim_speed, flags);
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassInputLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassInputLoopStatusActive != FALSE))
    {
        DObj *root;

        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassInputLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusWait) &&
            (status_id != nFTCommonStatusSquat) &&
            (status_id != nFTCommonStatusSquatWait) &&
            (status_id != nFTCommonStatusSquatRv) &&
            (status_id != nFTCommonStatusPass))
        {
            gNdsStageMPPassInputLoopUnsafeCount++;
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
        fp->proc_damage = NULL;

        if (status_id == nFTCommonStatusWait)
        {
            fp->motion_id = nFTCommonMotionWait;
            fp->proc_update = NULL;
            fp->proc_interrupt = ftCommonWaitProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonProcFighterOnCliffEdge;
            fp->ga = nMPKineticsGround;
        }
        else if (status_id == nFTCommonStatusSquat)
        {
            fp->motion_id = nFTCommonMotionSquat;
            fp->proc_update = NULL;
            fp->proc_interrupt = ndsBaseFTCommonSquatProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonProcFighterOnCliffEdge;
            fp->ga = nMPKineticsGround;
            gNdsStageMPPassInputLoopSquatSetCount++;
        }
        else if (status_id == nFTCommonStatusSquatWait)
        {
            fp->motion_id = nFTCommonMotionSquatWait;
            fp->proc_update = ndsBaseFTCommonSquatWaitProcUpdate;
            fp->proc_interrupt = ndsBaseFTCommonSquatWaitProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
            fp->ga = nMPKineticsGround;
            gNdsStageMPPassInputLoopSquatWaitSetCount++;
        }
        else if (status_id == nFTCommonStatusSquatRv)
        {
            fp->motion_id = nFTCommonMotionSquatRv;
            fp->proc_update = ftAnimEndSetWait;
            fp->proc_interrupt = ndsBaseFTCommonSquatRvProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
            fp->ga = nMPKineticsGround;
            gNdsStageMPPassInputLoopSquatRvSetCount++;
        }
        else
        {
            fp->motion_id = nFTCommonMotionPass;
            fp->proc_update = NULL;
            fp->proc_interrupt = NULL;
            fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
            fp->proc_map = mpCommonProcFighterCliffFloorCeil;
            fp->ga = nMPKineticsAir;
            gNdsStageMPPassInputLoopPassSetCount++;
        }
        fp->motion_script_id = fp->motion_id;

        if (fighter_gobj != NULL)
        {
            fighter_gobj->anim_frame = frame_begin;
            root = DObjGetStruct(fighter_gobj);
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
            return;
        }
        if (status_id != nFTCommonStatusDamageFall)
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
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
        fp->motion_id = nFTCommonMotionDamageFall;
        fp->motion_script_id = nFTCommonMotionDamageFall;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonDamageFallProcInterrupt;
        fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
        fp->proc_map = ftCommonDamageFallProcMap;
        fp->ga = nMPKineticsAir;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffWaitDamageLoopSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopWallDamageActive != FALSE) &&
        (status_id == nFTCommonStatusDamageFall))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
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
        fp->motion_id = nFTCommonMotionDamageFall;
        fp->motion_script_id = nFTCommonMotionDamageFall;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonDamageFallProcInterrupt;
        fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
        fp->proc_map = ftCommonDamageFallProcMap;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsAir;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        sNdsStageMPPassiveLoopWallDamageFallFTMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopWallDamageActive != FALSE) &&
        (status_id == nFTCommonStatusWallDamage))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
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
        fp->motion_id = nFTCommonMotionWallDamage;
        fp->motion_script_id = nFTCommonMotionWallDamage;
        fp->proc_update = ndsBaseFTCommonWallDamageProcUpdate;
        fp->proc_interrupt = ftCommonDamageAirCommonProcInterrupt;
        fp->proc_physics = ftCommonDamageCommonProcPhysics;
        fp->proc_map = ftCommonDamageAirCommonProcMap;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPPassiveLoopWallDamageSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopReboundActive != FALSE) &&
        ((status_id == nFTCommonStatusReboundWait) ||
         (status_id == nFTCommonStatusRebound)))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
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
        fp->motion_id = (status_id == nFTCommonStatusReboundWait) ?
            nFTCommonMotionNull : nFTCommonMotionRebound;
        fp->motion_script_id = fp->motion_id;
        fp->proc_update = (status_id == nFTCommonStatusReboundWait) ?
            ndsBaseFTCommonReboundWaitProcUpdate :
            ndsBaseFTCommonReboundProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        if (status_id == nFTCommonStatusReboundWait)
        {
            gNdsStageMPPassiveLoopReboundWaitSetStatusCount++;
        }
        else
        {
            gNdsStageMPPassiveLoopReboundSetStatusCount++;
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPPassiveLoopCatchActive != FALSE) ||
         (sNdsStageMPPassiveLoopCatchPullActive != FALSE) ||
         (sNdsStageMPPassiveLoopCatchPullUpdateActive != FALSE)) &&
        ((status_id == nFTCommonStatusCatch) ||
         (status_id == nFTCommonStatusCatchPull) ||
         (status_id == nFTCommonStatusCatchWait)))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
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
        if (status_id == nFTCommonStatusCatch)
        {
            fp->motion_id = nFTCommonMotionCatch;
            fp->proc_update = ndsBaseFTCommonCatchProcUpdate;
            fp->proc_interrupt = NULL;
            gNdsStageMPPassiveLoopCatchSetStatusCount++;
        }
        else if (status_id == nFTCommonStatusCatchPull)
        {
            fp->motion_id = nFTCommonMotionCatchPull;
            fp->proc_update = ndsBaseFTCommonCatchPullProcUpdate;
            fp->proc_interrupt = NULL;
            gNdsStageMPPassiveLoopCatchPullSetStatusCount++;
        }
        else
        {
            fp->motion_id = -2;
            fp->proc_update = NULL;
            fp->proc_interrupt = ndsBaseFTCommonCatchWaitProcInterrupt;
            gNdsStageMPPassiveLoopCatchWaitSetStatusCount++;
        }
        fp->motion_script_id = fp->motion_id;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = ndsBaseFTCommonCatchProcMap;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        ((((sNdsStageMPPassiveLoopThrowActive != FALSE) &&
           ((status_id == nFTCommonStatusThrowF) ||
            (status_id == nFTCommonStatusThrowB) ||
            (status_id == nFTCommonStatusThrownCommon)))) ||
         ((sNdsStageMPPassiveLoopThrowCallbackImmediateActive != FALSE) &&
          (status_id == nFTCommonStatusThrownCommon))))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
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
        fp->proc_interrupt = NULL;
        fp->proc_damage = NULL;
        if (status_id == nFTCommonStatusThrownCommon)
        {
            fp->motion_id = nFTCommonMotionThrownCommon;
            fp->proc_update = ndsBaseFTCommonThrownProcUpdate;
            fp->proc_physics = ndsBaseFTCommonThrownProcPhysics;
            fp->proc_map = ndsBaseFTCommonThrownProcMap;
            fp->proc_status = NULL;
            fp->ga = nMPKineticsAir;
            if (sNdsStageMPPassiveLoopThrowCallbackImmediateActive != FALSE)
            {
                gNdsStageMPPassiveLoopThrowCallbackImmediateSetStatusCount++;
            }
            else
            {
                gNdsStageMPPassiveLoopThrowTargetSetStatusCount++;
            }
        }
        else
        {
            fp->motion_id = (status_id == nFTCommonStatusThrowF) ?
                nFTCommonMotionThrowF : nFTCommonMotionThrowB;
            fp->proc_update = ndsBaseFTCommonThrowProcUpdate;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
            fp->proc_status = NULL;
            fp->ga = nMPKineticsGround;
            gNdsStageMPPassiveLoopThrowSetStatusCount++;
        }
        fp->motion_script_id = fp->motion_id;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPPassiveLoopCaptureActive != FALSE) ||
         (sNdsStageMPPassiveLoopCapturePhysicsActive != FALSE)) &&
        ((status_id == nFTCommonStatusCapturePulled) ||
         (status_id == nFTCommonStatusCaptureWait)))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
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
        fp->proc_update = NULL;
        fp->proc_interrupt = NULL;
        if (status_id == nFTCommonStatusCapturePulled)
        {
            fp->motion_id = nFTCommonMotionCapturePulled;
            fp->proc_physics = ndsBaseFTCommonCapturePulledProcPhysics;
            fp->proc_map = ndsBaseFTCommonCapturePulledProcMap;
            gNdsStageMPPassiveLoopCapturePulledSetStatusCount++;
        }
        else
        {
            fp->motion_id = -2;
            fp->proc_physics = NULL;
            fp->proc_map = ndsBaseFTCommonCaptureWaitProcMap;
            gNdsStageMPPassiveLoopCaptureWaitSetStatusCount++;
        }
        fp->motion_script_id = fp->motion_id;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopAppealActive != FALSE) &&
        (status_id == nFTCommonStatusAppeal))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
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
        fp->motion_id = nFTCommonMotionAppeal;
        fp->motion_script_id = nFTCommonMotionAppeal;
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = ndsBaseFTCommonAppealProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPPassiveLoopAppealSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopAppealGuardActive != FALSE) &&
        (status_id == nFTCommonStatusGuardOn))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
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
        fp->motion_id = nFTCommonMotionGuardOn;
        fp->motion_script_id = nFTCommonMotionGuardOn;
        fp->proc_update = ndsBaseFTCommonGuardOnProcUpdate;
        fp->proc_interrupt = ndsBaseFTCommonGuardCommonProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPPassiveLoopAppealGuardSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
            return;
        }
        if (status_id != nFTCommonStatusCliffCatch)
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
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
        fp->proc_update = ftCommonCliffCatchProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftCommonCliffCommonProcPhysics;
        fp->proc_map = ftCommonCliffCommonProcMap;
        fp->ga = nMPKineticsGround;
        fp->motion_id = nFTCommonMotionCliffCatch;
        fp->motion_script_id = nFTCommonMotionCliffCatch;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffWaitDamageLoopDamageFallCliffCatchSetStatusCount++;
        gNdsStageMPCliffWaitDamageLoopCliffCatchMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPPassiveLoopPassiveStandSetStatusActive != FALSE) ||
         (sNdsStageMPPassiveLoopBranchProbeActive != FALSE)) &&
        ((status_id == nFTCommonStatusPassiveStandF) ||
         (status_id == nFTCommonStatusPassiveStandB)))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
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
        fp->motion_id = (status_id == nFTCommonStatusPassiveStandF) ?
            nFTCommonMotionPassiveStandF : nFTCommonMotionPassiveStandB;
        fp->motion_script_id = fp->motion_id;
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyGroundVelTransN;
        fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if ((sNdsStageMPPassiveLoopPassiveStandSetStatusActive != FALSE) &&
            (gNdsStageMPPassiveLoopPassiveStandGroundSetCount == 0u))
        {
            gNdsStageMPPassiveLoopPassiveStandGroundSetCount++;
        }
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        if (sNdsStageMPPassiveLoopPassiveStandSetStatusActive != FALSE)
        {
            gNdsStageMPPassiveLoopPassiveStandSetStatusCount++;
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPPassiveLoopPassiveSetStatusActive != FALSE) ||
         (sNdsStageMPPassiveLoopBranchProbeActive != FALSE)) &&
        (status_id == nFTCommonStatusPassive))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
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
        fp->motion_id = nFTCommonMotionPassive;
        fp->motion_script_id = nFTCommonMotionPassive;
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if ((sNdsStageMPPassiveLoopPassiveSetStatusActive != FALSE) &&
            (gNdsStageMPPassiveLoopPassiveGroundSetCount == 0u))
        {
            gNdsStageMPPassiveLoopPassiveGroundSetCount++;
        }
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        if (sNdsStageMPPassiveLoopPassiveSetStatusActive != FALSE)
        {
            gNdsStageMPPassiveLoopPassiveSetStatusCount++;
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive != FALSE) &&
        ((status_id == nFTCommonStatusPassiveStandF) ||
         (status_id == nFTCommonStatusPassiveStandB)))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
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
        fp->motion_id = (status_id == nFTCommonStatusPassiveStandF) ?
            nFTCommonMotionPassiveStandF : nFTCommonMotionPassiveStandB;
        fp->motion_script_id = fp->motion_id;
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyGroundVelTransN;
        fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (gNdsStageMPCliffWaitDamageLoopPassiveStandGroundSetCount == 0u)
        {
            gNdsStageMPCliffWaitDamageLoopPassiveStandGroundSetCount++;
        }
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffWaitDamageLoopPassiveStandMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive != FALSE) &&
        (status_id == nFTCommonStatusPassive))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
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
        fp->motion_id = nFTCommonMotionPassive;
        fp->motion_script_id = nFTCommonMotionPassive;
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (gNdsStageMPCliffWaitDamageLoopPassiveGroundSetCount == 0u)
        {
            gNdsStageMPCliffWaitDamageLoopPassiveGroundSetCount++;
        }
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffWaitDamageLoopPassiveMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPPassiveLoopPassiveStandUpdateActive != FALSE) ||
         (sNdsStageMPPassiveLoopPassiveUpdateActive != FALSE) ||
         (sNdsStageMPPassiveLoopPassiveStandBActive != FALSE) ||
         (sNdsStageMPPassiveLoopReboundUpdateActive != FALSE)))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
            return;
        }
        if (status_id != nFTCommonStatusWait)
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
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
        fp->motion_id = nFTCommonMotionWait;
        fp->motion_script_id = nFTCommonMotionWait;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonWaitProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        fp->is_special_interrupt = TRUE;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        if (sNdsStageMPPassiveLoopPassiveStandUpdateActive != FALSE)
        {
            gNdsStageMPPassiveLoopPassiveStandWaitSetStatusCount++;
        }
        if (sNdsStageMPPassiveLoopPassiveUpdateActive != FALSE)
        {
            gNdsStageMPPassiveLoopPassiveWaitSetStatusCount++;
        }
        if (sNdsStageMPPassiveLoopReboundUpdateActive != FALSE)
        {
            gNdsStageMPPassiveLoopReboundFinalWaitSetStatusCount++;
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        ((sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive != FALSE) ||
         (sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive != FALSE)))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
            return;
        }
        if (status_id != nFTCommonStatusWait)
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
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
        fp->motion_id = nFTCommonMotionWait;
        fp->motion_script_id = nFTCommonMotionWait;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonWaitProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        fp->is_special_interrupt = TRUE;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        if (sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive != FALSE)
        {
            gNdsStageMPCliffWaitDamageLoopPassiveStandWaitSetStatusCount++;
        }
        if (sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive != FALSE)
        {
            gNdsStageMPCliffWaitDamageLoopPassiveWaitSetStatusCount++;
        }
        return;
    }

    if (sNdsStageMPLiveHitStatusLoopDownBounceSetStatusActive != FALSE)
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            return;
        }
        if ((status_id != nFTCommonStatusDownBounceD) &&
            (status_id != nFTCommonStatusDownBounceU))
        {
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
        fp->motion_id = (status_id == nFTCommonStatusDownBounceD) ?
            nFTCommonMotionDownBounceD : nFTCommonMotionDownBounceU;
        fp->motion_script_id = fp->motion_id;
        fp->proc_update = ftCommonDownBounceProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        fp->ga = nMPKineticsGround;
        fp->status_vars.common.downbounce.attack_buffer = 0;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if (sNdsStageMPLiveHitStatusLoopCliffCatchSetStatusActive != FALSE)
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            return;
        }
        if (status_id != nFTCommonStatusCliffCatch)
        {
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
        fp->proc_update = ftCommonCliffCatchProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftCommonCliffCommonProcPhysics;
        fp->proc_map = ftCommonCliffCommonProcMap;
        fp->ga = nMPKineticsGround;
        fp->motion_id = nFTCommonMotionCliffCatch;
        fp->motion_script_id = nFTCommonMotionCliffCatch;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusDownBounceD) &&
            (status_id != nFTCommonStatusDownBounceU))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
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
        fp->motion_id = (status_id == nFTCommonStatusDownBounceD) ?
            nFTCommonMotionDownBounceD : nFTCommonMotionDownBounceU;
        fp->motion_script_id = fp->motion_id;
        fp->proc_update = ftCommonDownBounceProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        fp->ga = nMPKineticsGround;
        fp->status_vars.common.downbounce.attack_buffer = 0;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffWaitDamageLoopDownBounceMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPDownRecoverLoopDownWaitSetStatusActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopDownStandSetStatusActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopDownAttackSetStatusActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive != FALSE)))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPDownRecoverLoopUnsafeCount++;
            return;
        }
        if ((sNdsStageMPDownRecoverLoopDownWaitSetStatusActive != FALSE) &&
            (status_id != nFTCommonStatusDownWaitD))
        {
            gNdsStageMPDownRecoverLoopUnsafeCount++;
            return;
        }
        if ((sNdsStageMPDownRecoverLoopDownStandSetStatusActive != FALSE) &&
            (status_id != nFTCommonStatusDownStandD))
        {
            gNdsStageMPDownRecoverLoopUnsafeCount++;
            return;
        }
        if ((sNdsStageMPDownRecoverLoopDownAttackSetStatusActive != FALSE) &&
            (status_id != nFTCommonStatusDownAttackD))
        {
            gNdsStageMPDownRecoverLoopUnsafeCount++;
            return;
        }
        if ((sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive !=
                FALSE) &&
            (status_id != nFTCommonStatusDownForwardD) &&
            (status_id != nFTCommonStatusDownBackD))
        {
            gNdsStageMPDownRecoverLoopUnsafeCount++;
            return;
        }

        ndsFTMainApplyCommonStatusReset(fp, flags);
        fp->status_prev = fp->status_id;
        fp->status_id = status_id;
        fp->status_total_tics = 0;
        fp->motion_attack_id = nFTMotionAttackIDNone;
        fp->status_attack_id = nFTStatusAttackIDNone;
        fp->stat_attack_id = nFTStatusAttackIDNone;
        if (status_id == nFTCommonStatusDownAttackD)
        {
            fp->motion_attack_id = nFTMotionAttackIDDownAttackD;
            fp->status_attack_id = nFTStatusAttackIDDownAttackD;
            fp->stat_attack_id = nFTStatusAttackIDDownAttackD;
        }
        fp->status_is_smash = FALSE;
        fp->status_is_projectile = FALSE;
        fp->status_flags = flags;
        fp->motion_frame = frame_begin;
        fp->anim_frame = frame_begin;
        fp->anim_speed = anim_speed;
        fp->proc_status = NULL;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (status_id == nFTCommonStatusDownWaitD)
        {
            fp->motion_id = -2;
            fp->motion_script_id = -2;
            fp->proc_update = ftCommonDownWaitProcUpdate;
            fp->proc_interrupt = ftCommonDownWaitProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
            gNdsStageMPDownRecoverLoopDownWaitMainSetStatusCount++;
        }
        else if (status_id == nFTCommonStatusDownStandD)
        {
            fp->motion_id = nFTCommonMotionDownStandD;
            fp->motion_script_id = fp->motion_id;
            fp->proc_update = ftAnimEndSetWait;
            fp->proc_interrupt = ftCommonDownStandProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
            gNdsStageMPDownRecoverLoopDownStandMainSetStatusCount++;
            ndsStageMPDownRecoverLoopAppendDownStandOrder(5u);
        }
        else if (status_id == nFTCommonStatusDownAttackD)
        {
            fp->motion_id = nFTCommonMotionDownAttackD;
            fp->motion_script_id = fp->motion_id;
            fp->proc_update = ftAnimEndSetWait;
            fp->proc_interrupt = NULL;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
            gNdsStageMPDownRecoverLoopAttackMainSetStatusCount++;
            ndsStageMPDownRecoverLoopAppendAttackOrder(3u);
        }
        else
        {
            fp->motion_id = (status_id == nFTCommonStatusDownForwardD) ?
                nFTCommonMotionDownForwardD : nFTCommonMotionDownBackD;
            fp->motion_script_id = fp->motion_id;
            fp->proc_update = ftAnimEndSetWait;
            fp->proc_interrupt = NULL;
            fp->proc_physics = ftPhysicsApplyGroundVelTransN;
            fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
            fp->is_jostle_ignore = TRUE;
            gNdsStageMPDownRecoverLoopRollMainSetStatusCount++;
            if (sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE)
            {
                ndsStageMPDownRecoverLoopAppendRollForwardOrder(4u);
            }
            else
            {
                ndsStageMPDownRecoverLoopAppendRollBackOrder(4u);
            }
        }
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownAttackSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusDownAttackD) &&
            (status_id != nFTCommonStatusDownAttackU))
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
            return;
        }

        ndsFTMainApplyCommonStatusReset(fp, flags);
        fp->status_prev = fp->status_id;
        fp->status_id = status_id;
        fp->status_total_tics = 0;
        if (status_id == nFTCommonStatusDownAttackD)
        {
            fp->motion_attack_id = nFTMotionAttackIDDownAttackD;
            fp->status_attack_id = nFTStatusAttackIDDownAttackD;
            fp->stat_attack_id = nFTStatusAttackIDDownAttackD;
        }
        else
        {
            fp->motion_attack_id = nFTMotionAttackIDDownAttackU;
            fp->status_attack_id = nFTStatusAttackIDDownAttackU;
            fp->stat_attack_id = nFTStatusAttackIDDownAttackU;
        }
        fp->status_is_smash = FALSE;
        fp->status_is_projectile = FALSE;
        fp->status_flags = flags;
        fp->motion_frame = frame_begin;
        fp->anim_frame = frame_begin;
        fp->anim_speed = anim_speed;
        fp->proc_status = NULL;
        fp->motion_id = (status_id == nFTCommonStatusDownAttackD) ?
            nFTCommonMotionDownAttackD : nFTCommonMotionDownAttackU;
        fp->motion_script_id = fp->motion_id;
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPDownWaitLoopAttackMainSetStatusCount++;
        ndsStageMPDownWaitLoopAppendAttackOrder(3u);
        return;
    }

    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusDownForwardD) &&
            (status_id != nFTCommonStatusDownForwardU) &&
            (status_id != nFTCommonStatusDownBackD) &&
            (status_id != nFTCommonStatusDownBackU))
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
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
        if (status_id == nFTCommonStatusDownForwardD)
        {
            fp->motion_id = nFTCommonMotionDownForwardD;
        }
        else if (status_id == nFTCommonStatusDownForwardU)
        {
            fp->motion_id = nFTCommonMotionDownForwardU;
        }
        else if (status_id == nFTCommonStatusDownBackD)
        {
            fp->motion_id = nFTCommonMotionDownBackD;
        }
        else
        {
            fp->motion_id = nFTCommonMotionDownBackU;
        }
        fp->motion_script_id = fp->motion_id;
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyGroundVelTransN;
        fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        fp->is_jostle_ignore = TRUE;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPDownWaitLoopRollMainSetStatusCount++;
        if (sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE)
        {
            ndsStageMPDownWaitLoopAppendRollForwardOrder(4u);
        }
        else
        {
            ndsStageMPDownWaitLoopAppendRollBackOrder(4u);
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownWaitSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusDownWaitD) &&
            (status_id != nFTCommonStatusDownWaitU))
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
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
        fp->motion_id = -2;
        fp->motion_script_id = -2;
        fp->proc_update = ftCommonDownWaitProcUpdate;
        fp->proc_interrupt = ftCommonDownWaitProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        fp->status_vars.common.downwait.stand_wait =
            FTCOMMON_DOWNWAIT_STAND_WAIT;
        gNdsStageMPDownWaitLoopDownWaitMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownWaitSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusDownWaitD) &&
            (status_id != nFTCommonStatusDownWaitU))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
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
        fp->motion_id = -2;
        fp->motion_script_id = -2;
        fp->proc_update = ftCommonDownWaitProcUpdate;
        fp->proc_interrupt = ftCommonDownWaitProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        fp->status_vars.common.downwait.stand_wait =
            FTCOMMON_DOWNWAIT_STAND_WAIT;
        gNdsStageMPCliffWaitDamageLoopDownWaitMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusDownStandD) &&
            (status_id != nFTCommonStatusDownStandU))
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
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
        fp->motion_id = (status_id == nFTCommonStatusDownStandD) ?
            nFTCommonMotionDownStandD : nFTCommonMotionDownStandU;
        fp->motion_script_id = fp->motion_id;
        fp->motion_vars.flags.flag1 = 0;
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = ftCommonDownStandProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPDownWaitLoopDownStandMainSetStatusCount++;
        ndsStageMPDownWaitLoopAppendSourceOrder(5u);
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownStandSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusDownStandD) &&
            (status_id != nFTCommonStatusDownStandU))
        {
            gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
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
        fp->motion_id = (status_id == nFTCommonStatusDownStandD) ?
            nFTCommonMotionDownStandD : nFTCommonMotionDownStandU;
        fp->motion_script_id = fp->motion_id;
        fp->motion_vars.flags.flag1 = 0;
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = ftCommonDownStandProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffWaitDamageLoopDownStandMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffTickFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffTickFloorLoopStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffTickFloorLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusOttotto) &&
            (status_id != nFTCommonStatusOttottoWait) &&
            (status_id != nFTCommonStatusFall) &&
            (status_id != nFTCommonStatusWait))
        {
            gNdsStageMPCliffTickFloorLoopUnsafeCount++;
            return;
        }
        if (status_id == nFTCommonStatusWait)
        {
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
        if (status_id == nFTCommonStatusFall)
        {
            fp->motion_id = nFTCommonMotionFall;
            fp->motion_script_id = nFTCommonMotionFall;
            fp->proc_update = NULL;
            fp->proc_interrupt = ftCommonFallProcInterrupt;
            fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
            fp->proc_map = mpCommonProcFighterCliffFloorCeil;
            fp->ga = nMPKineticsAir;
        }
        else if (status_id == nFTCommonStatusWait)
        {
            fp->motion_id = nFTCommonMotionWait;
            fp->motion_script_id = nFTCommonMotionWait;
            fp->proc_update = NULL;
            fp->proc_interrupt = ftCommonWaitProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonProcFighterOnCliffEdge;
            fp->ga = nMPKineticsGround;
        }
        else
        {
            fp->motion_id = (status_id == nFTCommonStatusOttotto) ?
                nFTCommonMotionOttotto : nFTCommonMotionOttottoWait;
            fp->motion_script_id = fp->motion_id;
            fp->proc_update = ftCommonOttottoProcUpdate;
            fp->proc_interrupt = ftCommonOttottoProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = ftCommonOttottoProcMap;
            fp->ga = nMPKineticsGround;
        }
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffTickFloorLoopStatusSetCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffStatusFloorLoopStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffStatusFloorLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusOttotto) &&
            (status_id != nFTCommonStatusOttottoWait) &&
            (status_id != nFTCommonStatusFall))
        {
            gNdsStageMPCliffStatusFloorLoopUnsafeCount++;
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
        if (status_id == nFTCommonStatusFall)
        {
            fp->motion_id = nFTCommonMotionFall;
            fp->motion_script_id = nFTCommonMotionFall;
            fp->proc_update = NULL;
            fp->proc_interrupt = ftCommonFallProcInterrupt;
            fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
            fp->proc_map = mpCommonProcFighterCliffFloorCeil;
        }
        else
        {
            fp->motion_id = (status_id == nFTCommonStatusOttotto) ?
                nFTCommonMotionOttotto : nFTCommonMotionOttottoWait;
            fp->motion_script_id = fp->motion_id;
            fp->proc_update = ftCommonOttottoProcUpdate;
            fp->proc_interrupt = ftCommonOttottoProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = ftCommonOttottoProcMap;
            fp->ga = nMPKineticsGround;
        }
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffStatusFloorLoopStatusSetCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPCeilStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCeilStatusFloorLoopSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCeilStatusFloorLoopUnsafeCount++;
            return;
        }
        if (status_id != nFTCommonStatusStopCeil)
        {
            gNdsStageMPCeilStatusFloorLoopUnsafeCount++;
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
        fp->proc_update = ftAnimEndSetFall;
        fp->proc_interrupt = NULL;
        fp->proc_physics = NULL;
        fp->proc_map = mpCommonProcFighterCliffFloorCeil;
        fp->ga = nMPKineticsGround;
        fp->motion_id = nFTCommonMotionStopCeil;
        fp->motion_script_id = nFTCommonMotionStopCeil;
        fp->physics.vel_air.y = 0.0F;
        fp->physics.vel_air.z = 0.0F;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCeilStatusFloorLoopFtMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
            return;
        }
        if (status_id != nFTCommonStatusCliffCatch)
        {
            gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
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
        fp->proc_update = ftCommonCliffCatchProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftCommonCliffCommonProcPhysics;
        fp->proc_map = ftCommonCliffCommonProcMap;
        fp->ga = nMPKineticsGround;
        fp->motion_id = nFTCommonMotionCliffCatch;
        fp->motion_script_id = nFTCommonMotionCliffCatch;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffCatchFloorLoopFtMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopRecatchSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
            return;
        }
        if (status_id != nFTCommonStatusCliffCatch)
        {
            gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
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
        fp->proc_update = ftCommonCliffCatchProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftCommonCliffCommonProcPhysics;
        fp->proc_map = ftCommonCliffCommonProcMap;
        fp->ga = nMPKineticsGround;
        fp->motion_id = nFTCommonMotionCliffCatch;
        fp->motion_script_id = nFTCommonMotionCliffCatch;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffClimbFloorLoopRecatchFtMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffAttackFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackFloorLoopSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffAttackFloorLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusCliffQuick) &&
            (status_id != nFTCommonStatusCliffSlow))
        {
            gNdsStageMPCliffAttackFloorLoopUnsafeCount++;
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
        fp->proc_update = (status_id == nFTCommonStatusCliffQuick) ?
            ftCommonCliffQuickProcUpdate : ftCommonCliffSlowProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftCommonCliffCommonProcPhysics;
        fp->proc_map = ftCommonCliffCommonProcMap;
        fp->is_cliff_hold = TRUE;
        fp->proc_damage = ftCommonCliffCommonProcDamage;
        fp->ga = nMPKineticsGround;
        fp->motion_id = (status_id == nFTCommonStatusCliffQuick) ?
            nFTCommonMotionCliffQuick : nFTCommonMotionCliffSlow;
        fp->motion_script_id = fp->motion_id;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffAttackFloorLoopQuickStatusSetCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusCliffQuick) &&
            (status_id != nFTCommonStatusCliffSlow) &&
            (status_id != nFTCommonStatusFall))
        {
            gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
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
        if (status_id == nFTCommonStatusFall)
        {
            fp->motion_id = nFTCommonMotionFall;
            fp->proc_update = NULL;
            fp->proc_interrupt = ftCommonFallProcInterrupt;
            fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
            fp->proc_map = mpCommonProcFighterCliffFloorCeil;
            fp->ga = nMPKineticsAir;
        }
        else
        {
            fp->proc_update = (status_id == nFTCommonStatusCliffQuick) ?
                ftCommonCliffQuickProcUpdate : ftCommonCliffSlowProcUpdate;
            fp->proc_interrupt = NULL;
            fp->proc_physics = ftCommonCliffCommonProcPhysics;
            fp->proc_map = ftCommonCliffCommonProcMap;
            fp->is_cliff_hold = TRUE;
            fp->proc_damage = ftCommonCliffCommonProcDamage;
            fp->ga = nMPKineticsGround;
            fp->motion_id = (status_id == nFTCommonStatusCliffQuick) ?
                nFTCommonMotionCliffQuick : nFTCommonMotionCliffSlow;
            gNdsStageMPCliffClimbFloorLoopQuickStatusSetCount++;
        }
        fp->motion_script_id = fp->motion_id;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffClimbActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbActionLoopSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffClimbActionLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusCliffClimbQuick1) &&
            (status_id != nFTCommonStatusCliffClimbQuick2))
        {
            gNdsStageMPCliffClimbActionLoopUnsafeCount++;
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
        fp->proc_interrupt = NULL;
        fp->ga = nMPKineticsGround;
        if (status_id == nFTCommonStatusCliffClimbQuick1)
        {
            fp->motion_id = nFTCommonMotionCliffClimbQuick1;
            fp->proc_update = ftCommonCliffClimbQuick1ProcUpdate;
            fp->proc_physics = ftCommonCliffCommonProcPhysics;
            fp->proc_map = ftCommonCliffCommonProcMap;
            fp->is_cliff_hold = TRUE;
            fp->proc_damage = ftCommonCliffCommonProcDamage;
            gNdsStageMPCliffClimbActionLoopQuick1SetStatusCount++;
        }
        else
        {
            fp->motion_id = nFTCommonMotionCliffClimbQuick2;
            fp->proc_update = ftCommonCliffCommon2ProcUpdate;
            fp->proc_physics = ftCommonCliffCommon2ProcPhysics;
            fp->proc_map = ftCommonCliffClimbCommon2ProcMap;
            fp->coll_data.floor_line_id = fp->coll_data.cliff_id;
            fp->coll_data.floor_dist = 0.0F;
            fp->is_jostle_ignore = TRUE;
            gNdsStageMPCliffClimbActionLoopQuick2SetStatusCount++;
        }
        fp->motion_script_id = fp->motion_id;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffClimbFinishLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFinishLoopUpdateActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffClimbFinishLoopUnsafeCount++;
            return;
        }
        if (status_id != nFTCommonStatusWait)
        {
            gNdsStageMPCliffClimbFinishLoopUnsafeCount++;
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
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonWaitProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->ga = nMPKineticsGround;
        fp->motion_id = nFTCommonMotionWait;
        fp->motion_script_id = nFTCommonMotionWait;
        fp->is_special_interrupt = TRUE;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffClimbFinishLoopWaitSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffAttackActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackActionLoopSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffAttackActionLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusCliffAttackQuick1) &&
            (status_id != nFTCommonStatusCliffAttackQuick2))
        {
            gNdsStageMPCliffAttackActionLoopUnsafeCount++;
            return;
        }

        ndsFTMainApplyCommonStatusReset(fp, flags);
        fp->status_prev = fp->status_id;
        fp->status_id = status_id;
        fp->status_total_tics = 0;
        fp->motion_attack_id = nFTMotionAttackIDCliffAttackQuick;
        fp->status_attack_id = nFTStatusAttackIDCliffAttackQuick;
        fp->stat_attack_id = nFTStatusAttackIDCliffAttackQuick;
        fp->status_is_smash = FALSE;
        fp->status_is_projectile = FALSE;
        fp->status_flags = flags;
        fp->motion_frame = frame_begin;
        fp->anim_frame = frame_begin;
        fp->anim_speed = anim_speed;
        fp->proc_status = NULL;
        fp->proc_interrupt = NULL;
        fp->ga = nMPKineticsGround;
        if (status_id == nFTCommonStatusCliffAttackQuick1)
        {
            fp->motion_id = nFTCommonMotionCliffAttackQuick1;
            fp->proc_update = ftCommonCliffAttackQuick1ProcUpdate;
            fp->proc_physics = ftCommonCliffCommonProcPhysics;
            fp->proc_map = ftCommonCliffCommonProcMap;
            fp->is_cliff_hold = TRUE;
            fp->proc_damage = ftCommonCliffCommonProcDamage;
            gNdsStageMPCliffAttackActionLoopQuick1SetStatusCount++;
        }
        else
        {
            fp->motion_id = nFTCommonMotionCliffAttackQuick2;
            fp->proc_update = ftCommonCliffCommon2ProcUpdate;
            fp->proc_physics = ftCommonCliffCommon2ProcPhysics;
            fp->proc_map = ftCommonCliffAttackEscape2ProcMap;
            fp->coll_data.floor_line_id = fp->coll_data.cliff_id;
            fp->coll_data.floor_dist = 0.0F;
            fp->is_jostle_ignore = TRUE;
            gNdsStageMPCliffAttackActionLoopQuick2SetStatusCount++;
        }
        fp->motion_script_id = fp->motion_id;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffEscapeActionLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusCliffQuick) &&
            (status_id != nFTCommonStatusCliffEscapeQuick1) &&
            (status_id != nFTCommonStatusCliffEscapeQuick2))
        {
            gNdsStageMPCliffEscapeActionLoopUnsafeCount++;
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
        fp->proc_interrupt = NULL;
        fp->ga = nMPKineticsGround;
        if (status_id == nFTCommonStatusCliffQuick)
        {
            fp->motion_id = nFTCommonMotionCliffQuick;
            fp->proc_update = ftCommonCliffQuickProcUpdate;
            fp->proc_physics = ftCommonCliffCommonProcPhysics;
            fp->proc_map = ftCommonCliffCommonProcMap;
            fp->is_cliff_hold = TRUE;
            fp->proc_damage = ftCommonCliffCommonProcDamage;
            gNdsStageMPCliffEscapeActionLoopQuickStatusSetCount++;
        }
        else if (status_id == nFTCommonStatusCliffEscapeQuick1)
        {
            fp->motion_id = nFTCommonMotionCliffEscapeQuick1;
            fp->proc_update = ftCommonCliffEscapeQuick1ProcUpdate;
            fp->proc_physics = ftCommonCliffCommonProcPhysics;
            fp->proc_map = ftCommonCliffCommonProcMap;
            fp->is_cliff_hold = TRUE;
            fp->proc_damage = ftCommonCliffCommonProcDamage;
            gNdsStageMPCliffEscapeActionLoopQuick1SetStatusCount++;
        }
        else
        {
            fp->motion_id = nFTCommonMotionCliffEscapeQuick2;
            fp->proc_update = ftCommonCliffCommon2ProcUpdate;
            fp->proc_physics = ftCommonCliffCommon2ProcPhysics;
            fp->proc_map = ftCommonCliffAttackEscape2ProcMap;
            fp->coll_data.floor_line_id = fp->coll_data.cliff_id;
            fp->coll_data.floor_dist = 0.0F;
            fp->is_jostle_ignore = TRUE;
            gNdsStageMPCliffEscapeActionLoopQuick2SetStatusCount++;
        }
        fp->motion_script_id = fp->motion_id;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPCliffWaitFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitFloorLoopSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPCliffWaitFloorLoopUnsafeCount++;
            return;
        }
        if (status_id != nFTCommonStatusCliffWait)
        {
            gNdsStageMPCliffWaitFloorLoopUnsafeCount++;
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
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonCliffWaitProcInterrupt;
        fp->proc_physics = ftCommonCliffCommonProcPhysics;
        fp->proc_map = ftCommonCliffCommonProcMap;
        fp->ga = nMPKineticsGround;
        fp->motion_id = nFTCommonMotionCliffWait;
        fp->motion_script_id = nFTCommonMotionCliffWait;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPCliffWaitFloorLoopFtMainSetStatusCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageMPFallLandFloorLoopProofEnabled() != FALSE) &&
        (sNdsStageMPFallLandFloorLoopSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageMPFallLandFloorLoopUnsafeCount++;
            return;
        }
        if ((status_id != nFTCommonStatusLandingLight) &&
            (status_id != nFTCommonStatusLandingHeavy))
        {
            gNdsStageMPFallLandFloorLoopUnsafeCount++;
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
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = ftCommonLandingProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->ga = nMPKineticsGround;
        fp->motion_id = (status_id == nFTCommonStatusLandingHeavy) ?
            nFTCommonMotionLandingHeavy : nFTCommonMotionLandingLight;
        fp->motion_script_id = fp->motion_id;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageMPFallLandFloorLoopStatusSetCallCount++;
        return;
    }

    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopSetStatusActive != FALSE))
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            gNdsStageTurnLoopUnsafeCount++;
            return;
        }
        if (status_id != nFTCommonStatusTurn)
        {
            gNdsStageTurnLoopUnsafeCount++;
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
        fp->proc_update = ftCommonTurnProcUpdate;
        fp->proc_interrupt = ftCommonTurnProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        fp->motion_id = nFTCommonMotionTurn;
        fp->motion_script_id = nFTCommonMotionTurn;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        gNdsStageTurnLoopMainSetStatusCount++;
        return;
    }

    if (ndsFighterMarioFoxWaitProofEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterWaitFtMainSetStatusCallCount++;

    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        if ((ndsFighterMarioFoxWaitTickProofEnabled() != FALSE) &&
            (status_id != nFTCommonStatusWait))
        {
            gNdsFighterWaitTickDeniedStatusCount++;
        }
        return;
    }

    if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
        (sNdsFighterJumpAttackAirMapLandingActive != FALSE) &&
        ((status_id == nFTCommonStatusLandingLight) ||
         (status_id == nFTCommonStatusLandingHeavy)))
    {
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
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = ftCommonLandingProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->ga = nMPKineticsGround;
        fp->motion_id = (status_id == nFTCommonStatusLandingHeavy) ?
            nFTCommonMotionLandingHeavy : nFTCommonMotionLandingLight;
        fp->motion_script_id = fp->motion_id;
        if (fighter_gobj != NULL)
        {
            DObj *root = DObjGetStruct(fighter_gobj);

            fighter_gobj->anim_frame = frame_begin;
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        ((status_id == nFTCommonStatusFall) ||
        (status_id == nFTCommonStatusFallAerial) ||
        (status_id == nFTCommonStatusLandingLight) ||
        (status_id == nFTCommonStatusLandingHeavy)))
    {
        if ((status_id == nFTCommonStatusFallAerial) &&
            ((sNdsFighterLandingJumpAnimEndActive != FALSE) ||
             (sNdsFighterProcessLoopJumpAnimEndActive != FALSE)))
        {
            status_id = nFTCommonStatusFall;
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

        if (status_id == nFTCommonStatusFallAerial)
        {
            gNdsFighterLandingFallAerialDeniedCount++;
            gNdsFighterLandingDeniedStatusCount++;
            return;
        }
        if (status_id == nFTCommonStatusFall)
        {
            gNdsFighterLandingFtMainFallStatusCount++;
            fp->motion_id = nFTCommonMotionFall;
            fp->proc_update = NULL;
            fp->proc_interrupt = ftCommonFallProcInterrupt;
            fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
            fp->proc_map = mpCommonProcFighterCliffFloorCeil;
        }
        else
        {
            fp->proc_update = ftAnimEndSetWait;
            fp->proc_interrupt = ftCommonLandingProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonProcFighterOnCliffEdge;
            if (status_id == nFTCommonStatusLandingHeavy)
            {
                gNdsFighterLandingFtMainLandingHeavyStatusCount++;
                gNdsFighterLandingHeavyDeniedCount++;
                fp->motion_id = nFTCommonMotionLandingHeavy;
            }
            else
            {
                gNdsFighterLandingFtMainLandingLightStatusCount++;
                fp->motion_id = nFTCommonMotionLandingLight;
            }
        }
        fp->motion_script_id = fp->motion_id;
        if (fighter_gobj != NULL)
        {
            fighter_gobj->anim_frame = frame_begin;
            if (DObjGetStruct(fighter_gobj) != NULL)
            {
                DObjGetStruct(fighter_gobj)->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        ((status_id == nFTCommonStatusKneeBend) ||
        (status_id == nFTCommonStatusJumpF) ||
        (status_id == nFTCommonStatusJumpB) ||
        (((status_id >= nFTCommonStatusAttackAirStart) &&
          (status_id <= nFTCommonStatusAttackAirEnd)) &&
            (ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
            ((sNdsFighterJumpAttackAirActive != FALSE) ||
             (sNdsFighterJumpAttackAirDirectionActive != FALSE))) ||
        (((status_id == nFTCommonStatusLandingAirN) ||
          (status_id == nFTCommonStatusLandingAirNull)) &&
            (ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
            (sNdsFighterJumpAttackAirMapLandingActive != FALSE))))
    {
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
        if (status_id == nFTCommonStatusKneeBend)
        {
            gNdsFighterJumpFtMainKneeBendStatusCount++;
            fp->motion_id = nFTCommonMotionKneeBend;
            fp->proc_update = ftCommonKneeBendProcUpdate;
            fp->proc_interrupt = ftCommonKneeBendProcInterrupt;
            fp->proc_physics = NULL;
            fp->proc_map = NULL;
        }
        else
        {
            if ((status_id >= nFTCommonStatusAttackAirStart) &&
                (status_id <= nFTCommonStatusAttackAirEnd))
            {
                s32 attackair_index =
                    status_id - nFTCommonStatusAttackAirStart;

                if ((status_id == nFTCommonStatusAttackAirN) &&
                    (sNdsFighterJumpAttackAirActive != FALSE))
                {
                    gNdsFighterJumpAttackAirSetStatusCount++;
                    gNdsFighterJumpFtMainAttackAirStatusCount++;
                }
                fp->motion_id =
                    nFTCommonMotionAttackAirStart + attackair_index;
                fp->motion_attack_id =
                    nFTMotionAttackIDAttackAirN + attackair_index;
                fp->status_attack_id =
                    nFTStatusAttackIDAttackAirN + attackair_index;
                fp->stat_attack_id =
                    nFTStatusAttackIDAttackAirN + attackair_index;
                fp->proc_update =
                    (status_id == nFTCommonStatusAttackAirLw) ?
                        ndsBaseFTCommonAttackAirLwProcUpdate :
                        ftAnimEndSetFall;
                fp->proc_interrupt = NULL;
                fp->proc_physics = ftPhysicsApplyAirVelDrift;
                fp->proc_map = ftCommonAttackAirProcMap;
                fp->ga = nMPKineticsAir;
            }
            else if ((status_id == nFTCommonStatusLandingAirN) ||
                     (status_id == nFTCommonStatusLandingAirNull))
            {
                gNdsFighterJumpAttackAirMapLandingMask |= 1u << 2u;
                fp->motion_id = (status_id == nFTCommonStatusLandingAirN) ?
                    nFTCommonMotionLandingAirN :
                    nFTCommonMotionLandingAirNull;
                fp->proc_update = ftAnimEndSetWait;
                fp->proc_interrupt = ftCommonLandingProcInterrupt;
                fp->proc_physics = ftPhysicsApplyGroundVelFriction;
                fp->proc_map = mpCommonProcFighterOnCliffEdge;
                fp->ga = nMPKineticsGround;
            }
            else
            {
                if (status_id == nFTCommonStatusJumpB)
                {
                    gNdsFighterJumpUnexpectedStatusCount++;
                }
                gNdsFighterJumpFtMainJumpStatusCount++;
                fp->motion_id = (status_id == nFTCommonStatusJumpF) ?
                    nFTCommonMotionJumpF : nFTCommonMotionJumpB;
                fp->proc_update = ftAnimEndSetFall;
                fp->proc_interrupt = ftCommonJumpProcInterrupt;
                fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
                fp->proc_map = mpCommonProcFighterCliffFloorCeil;
            }
        }
        fp->motion_script_id = fp->motion_id;
        if (fighter_gobj != NULL)
        {
            fighter_gobj->anim_frame = frame_begin;
            if (DObjGetStruct(fighter_gobj) != NULL)
            {
                DObjGetStruct(fighter_gobj)->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE) &&
        (status_id == nFTCommonStatusAttackAirLw))
    {
        DObj *root;

        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
        {
            return;
        }

        ndsFTMainApplyCommonStatusReset(fp, flags);
        fp->status_prev = fp->status_id;
        fp->status_id = status_id;
        fp->status_total_tics = 0;
        fp->motion_id = nFTCommonMotionAttackAirLw;
        fp->motion_script_id = nFTCommonMotionAttackAirLw;
        fp->motion_attack_id = nFTMotionAttackIDAttackAirLw;
        fp->status_attack_id = nFTStatusAttackIDAttackAirLw;
        fp->stat_attack_id = nFTStatusAttackIDAttackAirLw;
        fp->status_is_smash = FALSE;
        fp->status_is_projectile = FALSE;
        fp->status_flags = flags;
        fp->motion_frame = frame_begin;
        fp->anim_frame = frame_begin;
        fp->anim_speed = anim_speed;
        fp->proc_update = ndsBaseFTCommonAttackAirLwProcUpdate;
        fp->proc_interrupt = NULL;
        fp->proc_physics = ftPhysicsApplyAirVelDrift;
        fp->proc_map = ftCommonAttackAirProcMap;
        fp->proc_status = NULL;
        fp->ga = nMPKineticsAir;
        if (fighter_gobj != NULL)
        {
            fighter_gobj->anim_frame = frame_begin;
            root = DObjGetStruct(fighter_gobj);
            if (root != NULL)
            {
                root->anim_speed = anim_speed;
            }
        }
        return;
    }

    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        ((status_id == nFTCommonStatusDash) ||
        (status_id == nFTCommonStatusRun) ||
        (status_id == nFTCommonStatusTurnRun) ||
        (status_id == nFTCommonStatusRunBrake) ||
        ((status_id == nFTCommonStatusAttack11) &&
            (sNdsFighterDashRunAttack1Active != FALSE)) ||
        ((status_id == nFTCommonStatusAttack12) &&
            (sNdsFighterDashRunAttack1Active != FALSE)) ||
        ((status_id == nFTMarioStatusAttack13) &&
            (fp->fkind == nFTKindMario) &&
            (sNdsFighterDashRunAttack1Active != FALSE)) ||
        ((status_id == nFTFoxStatusAttack100Start) &&
            ((fp->fkind == nFTKindFox) || (fp->fkind == nFTKindNFox)) &&
            (sNdsFighterDashRunAttack1Active != FALSE)) ||
        ((status_id == nFTFoxStatusAttack100Loop) &&
            ((fp->fkind == nFTKindFox) || (fp->fkind == nFTKindNFox)) &&
            (sNdsFighterDashRunAttack1Active != FALSE)) ||
        ((status_id == nFTFoxStatusAttack100End) &&
            ((fp->fkind == nFTKindFox) || (fp->fkind == nFTKindNFox)) &&
            (sNdsFighterDashRunAttack1Active != FALSE)) ||
        ((status_id == nFTCommonStatusAttackDash) &&
            (sNdsFighterDashRunAttackDashActive != FALSE)) ||
        (((status_id == nFTCommonStatusGuardOn) ||
            (status_id == nFTCommonStatusGuard)) &&
            (sNdsFighterDashRunGuardOnActive != FALSE)) ||
        ((status_id == nFTCommonStatusGuardSetOff) &&
            (sNdsFighterDashRunGuardOnActive != FALSE)) ||
        ((status_id == nFTCommonStatusGuardOff) &&
            ((fp->status_id == nFTCommonStatusGuard) ||
            (fp->status_id == nFTCommonStatusGuardSetOff))) ||
        ((status_id == nFTCommonStatusWait) &&
            (sNdsFighterDashRunEscapeActive != FALSE) &&
            ((fp->status_id == nFTCommonStatusEscapeF) ||
            (fp->status_id == nFTCommonStatusEscapeB))) ||
        (((status_id == nFTCommonStatusEscapeF) ||
            (status_id == nFTCommonStatusEscapeB)) &&
            (sNdsFighterDashRunEscapeActive != FALSE))))
    {
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
        fp->proc_update = NULL;
        fp->proc_status = NULL;
        if (status_id == nFTCommonStatusWait)
        {
            fp->motion_id = nFTCommonMotionWait;
            fp->proc_interrupt = ftCommonWaitProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonProcFighterOnCliffEdge;
            fp->ga = nMPKineticsGround;
        }
        else if (status_id == nFTCommonStatusDash)
        {
            gNdsFighterDashRunDashSetStatusCount++;
            gNdsFighterDashRunFtMainDashStatusCount++;
            fp->motion_id = nFTCommonMotionDash;
            fp->proc_update = ftCommonDashProcUpdate;
            fp->proc_interrupt = ftCommonDashProcInterrupt;
            fp->proc_physics = ftCommonDashProcPhysics;
            fp->proc_map = ftCommonDashProcMap;
        }
        else if (status_id == nFTCommonStatusRun)
        {
            gNdsFighterDashRunRunSetStatusCount++;
            gNdsFighterDashRunFtMainRunStatusCount++;
            fp->motion_id = nFTCommonMotionRun;
            fp->proc_interrupt = ftCommonRunProcInterrupt;
            fp->proc_physics = ftPhysicsSetGroundVelTransferAir;
            fp->proc_map = mpCommonProcFighterOnCliffEdge;
        }
        else if (status_id == nFTCommonStatusTurnRun)
        {
            gNdsFighterDashRunTurnRunSetStatusCount++;
            gNdsFighterDashRunFtMainTurnRunStatusCount++;
            fp->motion_id = nFTCommonMotionTurnRun;
            fp->proc_update = ndsBaseFTCommonTurnRunProcUpdate;
            fp->proc_interrupt = ndsBaseFTCommonTurnRunProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelTransN;
            fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        }
        else if (status_id == nFTCommonStatusRunBrake)
        {
            gNdsFighterDashRunRunBrakeSetStatusCount++;
            gNdsFighterDashRunFtMainRunBrakeStatusCount++;
            fp->motion_id = nFTCommonMotionRunBrake;
            fp->proc_interrupt = ftCommonRunBrakeProcInterrupt;
            fp->proc_physics = ftCommonRunBrakeProcPhysics;
            fp->proc_map = mpCommonProcFighterOnCliffEdge;
        }
        else if (status_id == nFTCommonStatusAttack11)
        {
            gNdsFighterDashRunAttack11SetStatusCount++;
            gNdsFighterDashRunFtMainAttack11StatusCount++;
            fp->motion_id = nFTCommonMotionAttack11;
            fp->motion_attack_id = nFTMotionAttackIDAttack11;
            fp->status_attack_id = nFTStatusAttackIDAttack11;
            fp->stat_attack_id = nFTStatusAttackIDAttack11;
            fp->proc_update = ftCommonAttack11ProcUpdate;
            fp->proc_interrupt = ftCommonAttack11ProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
        }
        else if (status_id == nFTCommonStatusAttack12)
        {
            gNdsFighterDashRunAttack12SetStatusCount++;
            gNdsFighterDashRunFtMainAttack12StatusCount++;
            fp->motion_id = nFTCommonMotionAttack12;
            fp->motion_attack_id = nFTMotionAttackIDAttack12;
            fp->status_attack_id = nFTStatusAttackIDAttack12;
            fp->stat_attack_id = nFTStatusAttackIDAttack12;
            fp->proc_update = ftCommonAttack12ProcUpdate;
            fp->proc_interrupt = ftCommonAttack12ProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
        }
        else if ((status_id == nFTMarioStatusAttack13) &&
            (fp->fkind == nFTKindMario))
        {
            gNdsFighterDashRunAttack13SetStatusCount++;
            gNdsFighterDashRunFtMainAttack13StatusCount++;
            fp->motion_id = nFTMarioMotionAttack13;
            fp->motion_attack_id = nFTMotionAttackIDAttack13;
            fp->status_attack_id = nFTStatusAttackIDAttack13;
            fp->stat_attack_id = nFTStatusAttackIDAttack13;
            fp->proc_update = ftAnimEndSetWait;
            fp->proc_interrupt = NULL;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
        }
        else if ((status_id == nFTFoxStatusAttack100Start) &&
            ((fp->fkind == nFTKindFox) || (fp->fkind == nFTKindNFox)))
        {
            gNdsFighterDashRunFtMainAttack100StartStatusCount++;
            fp->motion_id = nFTFoxMotionAttack100Start;
            fp->motion_attack_id = nFTMotionAttackIDAttack100;
            fp->status_attack_id = nFTStatusAttackIDAttack100;
            fp->stat_attack_id = nFTStatusAttackIDAttack100;
            fp->proc_update = ftCommonAttack100StartProcUpdate;
            fp->proc_interrupt = NULL;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
        }
        else if ((status_id == nFTFoxStatusAttack100Loop) &&
            ((fp->fkind == nFTKindFox) || (fp->fkind == nFTKindNFox)))
        {
            gNdsFighterDashRunFtMainAttack100LoopStatusCount++;
            fp->motion_id = nFTFoxMotionAttack100Loop;
            fp->motion_attack_id = nFTMotionAttackIDAttack100;
            fp->status_attack_id = nFTStatusAttackIDAttack100;
            fp->stat_attack_id = nFTStatusAttackIDAttack100;
            fp->proc_update = ftCommonAttack100LoopProcUpdate;
            fp->proc_interrupt = ftCommonAttack100LoopProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
        }
        else if ((status_id == nFTFoxStatusAttack100End) &&
            ((fp->fkind == nFTKindFox) || (fp->fkind == nFTKindNFox)))
        {
            fp->motion_id = nFTFoxMotionAttack100End;
            fp->motion_attack_id = nFTMotionAttackIDAttack100;
            fp->status_attack_id = nFTStatusAttackIDAttack100;
            fp->stat_attack_id = nFTStatusAttackIDAttack100;
            fp->proc_update = ftAnimEndSetWait;
            fp->proc_interrupt = NULL;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
        }
        else if (status_id == nFTCommonStatusGuardOn)
        {
            gNdsFighterDashRunGuardSetStatusCount++;
            gNdsFighterDashRunFtMainGuardOnStatusCount++;
            fp->motion_id = nFTCommonMotionGuardOn;
            fp->proc_update = ndsBaseFTCommonGuardOnProcUpdate;
            fp->proc_interrupt = ndsBaseFTCommonGuardCommonProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        }
        else if (status_id == nFTCommonStatusGuard)
        {
            fp->motion_id = nFTCommonMotionGuardOn;
            fp->proc_update = ndsBaseFTCommonGuardProcUpdate;
            fp->proc_interrupt = ndsBaseFTCommonGuardCommonProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        }
        else if (status_id == nFTCommonStatusGuardOff)
        {
            fp->motion_id = nFTCommonMotionGuardOff;
            fp->proc_update = ndsBaseFTCommonGuardOffProcUpdate;
            fp->proc_interrupt = NULL;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        }
        else if (status_id == nFTCommonStatusGuardSetOff)
        {
            gNdsFighterDashRunGuardSetOffSetStatusCount++;
            gNdsFighterDashRunFtMainGuardSetOffStatusCount++;
            fp->proc_update = ndsBaseFTCommonGuardSetOffProcUpdate;
            fp->proc_interrupt = NULL;
            fp->proc_physics = ftPhysicsApplyGroundVelFriction;
            fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
        }
        else if ((status_id == nFTCommonStatusEscapeF) ||
            (status_id == nFTCommonStatusEscapeB))
        {
            gNdsFighterDashRunEscapeSetStatusCount++;
            gNdsFighterDashRunFtMainEscapeStatusCount++;
            fp->motion_id = (status_id == nFTCommonStatusEscapeF) ?
                nFTCommonMotionEscapeF : nFTCommonMotionEscapeB;
            fp->proc_update = ndsBaseFTCommonEscapeProcUpdate;
            fp->proc_interrupt = ndsBaseFTCommonEscapeProcInterrupt;
            fp->proc_physics = ftPhysicsApplyGroundVelTransN;
            fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
            fp->proc_status = ndsBaseFTCommonEscapeProcStatus;
        }
        else
        {
            gNdsFighterDashRunAttackDashSetStatusCount++;
            gNdsFighterDashRunFtMainAttackDashStatusCount++;
            fp->motion_id = nFTCommonMotionAttackDash;
            fp->proc_update = ftAnimEndSetWait;
            fp->proc_interrupt = NULL;
            fp->proc_physics = ftPhysicsApplyGroundVelTransN;
            fp->proc_map = mpCommonSetFighterFallOnEdgeBreak;
        }
        fp->motion_script_id = fp->motion_id;
        if (fighter_gobj != NULL)
        {
            fighter_gobj->anim_frame = frame_begin;
            DObjGetStruct(fighter_gobj)->anim_speed = anim_speed;
        }
        return;
    }

    fp = ftGetStruct(fighter_gobj);
    if (fp == NULL)
    {
        return;
    }

    if ((ndsFighterMarioFoxWalkInputProofEnabled() != FALSE) &&
        (status_id >= nFTCommonStatusWalkSlow) &&
        (status_id <= nFTCommonStatusWalkFast))
    {
        gNdsFighterWalkSetStatusCallCount++;
        gNdsFighterWalkFtMainSetStatusCallCount++;
        ndsFTMainApplyCommonStatusReset(fp, flags);
        fp->status_prev = fp->status_id;
        fp->status_id = status_id;
        fp->status_total_tics = 0;
        fp->motion_id = nFTCommonMotionWalkSlow +
            (status_id - nFTCommonStatusWalkSlow);
        fp->motion_script_id = fp->motion_id;
        fp->motion_attack_id = nFTMotionAttackIDNone;
        fp->status_attack_id = nFTStatusAttackIDNone;
        fp->stat_attack_id = nFTStatusAttackIDNone;
        fp->status_is_smash = FALSE;
        fp->status_is_projectile = FALSE;
        fp->status_flags = flags;
        fp->motion_frame = frame_begin;
        fp->anim_frame = frame_begin;
        fp->anim_speed = anim_speed;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonWalkProcInterrupt;
        fp->proc_physics = ftCommonWalkProcPhysics;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->proc_status = NULL;
        if (fighter_gobj != NULL)
        {
            fighter_gobj->anim_frame = frame_begin;
        }
        return;
    }

    if (status_id != nFTCommonStatusWait)
    {
        if (ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE)
        {
            gNdsFighterWalkLoopDeniedStatusCount++;
            gNdsFighterWalkLoopUnexpectedStatusCount++;
        }
        if (ndsFighterMarioFoxWalkInputProofEnabled() != FALSE)
        {
            gNdsFighterWalkDeniedStatusCount++;
            gNdsFighterWalkUnexpectedStatusCount++;
        }
        if (ndsFighterMarioFoxWaitTickProofEnabled() != FALSE)
        {
            gNdsFighterWaitTickDeniedStatusCount++;
        }
        return;
    }

    ndsFTMainApplyCommonStatusReset(fp, flags);
    fp->status_prev = fp->status_id;
    fp->status_id = status_id;
    fp->status_total_tics = 0;
    fp->motion_id = nFTCommonMotionWait;
    fp->motion_script_id = nFTCommonMotionWait;
    fp->motion_attack_id = nFTMotionAttackIDNone;
    fp->status_attack_id = nFTStatusAttackIDNone;
    fp->stat_attack_id = nFTStatusAttackIDNone;
    fp->status_is_smash = FALSE;
    fp->status_is_projectile = FALSE;
    fp->status_flags = flags;
    fp->motion_frame = frame_begin;
    fp->anim_frame = frame_begin;
    fp->anim_speed = anim_speed;
    fp->proc_update = NULL;
    fp->proc_interrupt = ftCommonWaitProcInterrupt;
    fp->proc_physics = ftPhysicsApplyGroundVelFriction;
    fp->proc_map = mpCommonProcFighterOnCliffEdge;
    fp->proc_damage = NULL;
    fp->proc_status = NULL;
    fp->is_special_interrupt = TRUE;
    fp->is_wait_status_setup = TRUE;
    fp->is_wait_motion_setup = TRUE;
    if ((ndsFighterMarioFoxJumpLoopProofEnabled() != FALSE) &&
        (sNdsFighterJumpRunBrakeEndActive != FALSE))
    {
        gNdsFighterJumpWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxLandingLoopProofEnabled() != FALSE) &&
        (sNdsFighterLandingEndActive != FALSE))
    {
        gNdsFighterLandingWaitSetStatusCount++;
        if ((fp->status_id == nFTCommonStatusWait) &&
            (fp->motion_id == nFTCommonMotionWait))
        {
            gNdsFighterLandingWaitSetStatusSuccessCount++;
        }
    }
    if ((ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE) &&
        (sNdsFighterWalkLoopWaitReturnActive != FALSE))
    {
        gNdsFighterWalkLoopWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopAttackUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopAttackWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollForwardUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollForwardWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopRollBackUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollBackWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandUpdateActive != FALSE))
    {
        gNdsStageMPDownWaitLoopDownStandWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopFinalUpdateActive != FALSE))
    {
        gNdsStageTurnLoopWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        ((sNdsStageMPDownRecoverLoopDownStandUpdateActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopAttackUpdateActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopRollForwardUpdateActive != FALSE) ||
         (sNdsStageMPDownRecoverLoopRollBackUpdateActive != FALSE)))
    {
        gNdsStageMPDownRecoverLoopWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveStandWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopPassiveUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopPassiveWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopReboundUpdateActive != FALSE))
    {
        gNdsStageMPPassiveLoopReboundFinalWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassiveStandWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopPassiveWaitSetStatusCount++;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFinishLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFinishLoopUpdateActive != FALSE))
    {
        gNdsStageMPCliffClimbFinishLoopWaitSetStatusCount++;
    }

    if (fighter_gobj != NULL)
    {
        fighter_gobj->anim_frame = frame_begin;
    }
}

#if !NDS_IMPORT_BATTLESHIP_FTMAIN
void ftMainSetStatus(GObj *fighter_gobj, s32 status_id,
                     f32 frame_begin, f32 anim_speed, u32 flags)
{
    ndsFTMainSetStatusCompatHarness(fighter_gobj, status_id, frame_begin,
                                    anim_speed, flags);
}
#endif
