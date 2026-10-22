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
    return FALSE;
}

sb32 ndsDiagnosticsHandleImportedFTMainSetStatusBefore(GObj *fighter_gobj,
                                                       s32 status_id,
                                                       f32 frame_begin,
                                                       f32 anim_speed,
                                                       u32 flags)
{
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
    FTStruct *moveset_fp;
    u32 moveset_i;


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

}

void ndsDiagnosticsRecordImportedFTMainSetStatus(GObj *fighter_gobj,
                                                s32 status_id,
                                                f32 frame_begin,
                                                f32 anim_speed, u32 flags)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 handled_status = FALSE;

    (void)frame_begin;

    if (fp == NULL)
    {
        return;
    }

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

    if ((handled_status == FALSE) &&
        (ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE) &&
        (status_id == nFTCommonStatusAttackAirLw) &&
        (fp->status_id == nFTCommonStatusAttackAirLw) &&
        (fp->motion_id == nFTCommonMotionAttackAirLw))
    {
        handled_status = TRUE;
    }

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

    if (handled_status != FALSE)
    {
        return;
    }

    if (status_id != nFTCommonStatusWait)
    {
        return;
    }

    if ((fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait))
    {
        return;
    }

    fp->is_wait_status_setup = TRUE;
    fp->is_wait_motion_setup = TRUE;

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
#if NDS_P2_STAGE_JUNGLE
        /* P2-4s3: the two procs exist now (compat_shims), so the captured
         * fighter counts down, hears the cue at half the shoot wait, fires
         * on either face button, and is launched automatically after 180
         * frames. Before they landed both were NULL and a fighter that
         * entered the cannon stayed in it. */
        fp->proc_update = ftCommonTaruCannProcUpdate;
        fp->proc_interrupt = ftCommonTaruCannProcInterrupt;
#else
        /* No Congo Jungle in this build, so nothing can reach this status;
         * the physics hook alone keeps the shape of the arm. */
        fp->proc_update = NULL;
        fp->proc_interrupt = NULL;
#endif
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

    return;
}

