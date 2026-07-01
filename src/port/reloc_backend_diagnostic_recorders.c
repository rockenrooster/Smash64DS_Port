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

void ftMainSetStatus(GObj *fighter_gobj, s32 status_id,
                     f32 frame_begin, f32 anim_speed, u32 flags)
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

    if (fighter_gobj != NULL)
    {
        fighter_gobj->anim_frame = frame_begin;
    }
}

#define NDS_FTMOTION_EVENT_END 0u
#define NDS_FTMOTION_EVENT_MAKE_ATTACK_COLL 3u
#define NDS_FTMOTION_EVENT_MAKE_ATTACK_COLL_SCALED 4u
#define NDS_FTMOTION_EVENT_CLEAR_ATTACK_COLL_ALL 6u
#define NDS_FTMOTION_EVENT_SET_ATTACK_COLL_OFFSET 7u
#define NDS_FTMOTION_EVENT_SET_ATTACK_COLL_DAMAGE 8u
#define NDS_FTMOTION_EVENT_SET_ATTACK_COLL_SIZE 9u
#define NDS_FTMOTION_EVENT_SET_THROW 12u
#define NDS_FTMOTION_EVENT_SET_DAMAGE_THROWN 13u
#define NDS_FTMOTION_EVENT_SET_DAMAGE_COLL_PART_ID 31u
#define NDS_FTMOTION_EVENT_SUBROUTINE 34u
#define NDS_FTMOTION_EVENT_GOTO 36u
#define NDS_FTMOTION_EVENT_PAUSE_SCRIPT 37u
#define NDS_FTMOTION_EVENT_EFFECT 38u
#define NDS_FTMOTION_EVENT_EFFECT_ITEM_HOLD 39u
#define NDS_FTMOTION_EVENT_SET_PARALLEL_SCRIPT 46u
#define NDS_FTMOTION_SCRIPT_SCAN_WORDS_MAX 128u
#define NDS_FTMOTION_ATTACK_EVENT_CMD_DAMAGE 0x1u
#define NDS_FTMOTION_ATTACK_EVENT_CMD_SIZE 0x2u
#define NDS_FTMOTION_ATTACK_EVENT_CMD_CLEAR 0x4u
#define NDS_FTMOTION_ATTACK_EVENT_CMD_CLEAR_OFF 0x8u
#define NDS_FTMOTION_ATTACK_EVENT_POS_NEW 0x1u
#define NDS_FTMOTION_ATTACK_EVENT_POS_WORLD 0x2u
#define NDS_FTMOTION_ATTACK_EVENT_POS_TRANSFER 0x4u
#define NDS_FTMOTION_ATTACK_EVENT_POS_MATRIX_RESET 0x8u
#define NDS_FTMOTION_ATTACK_EVENT_POS_JOINT_READY 0x10u
#define NDS_FTMOTION_ATTACK_EVENT_POS_WRITEBACK 0x20u
#define NDS_FTMOTION_ATTACK_EVENT_POS_INTERPOLATE 0x40u
#define NDS_FTMOTION_ATTACK_EVENT_POS_PREV_COPY 0x80u
#define NDS_FTMOTION_ATTACK_EVENT_POS_RANGE_CURR 0x100u
#define NDS_FTMOTION_ATTACK_EVENT_POS_RANGE_PREV 0x200u
#define NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_RECT 0x400u
#define NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_COLLIDE 0x800u
#define NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_RECORD 0x1000u
#define NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_HITLOG 0x2000u
#define NDS_FTMOTION_ATTACK_EVENT_POS_HIT_SFX 0x4000u
#define NDS_FTMOTION_ATTACK_EVENT_POS_HIT_STATS 0x8000u
#define NDS_FTMOTION_ATTACK_EVENT_POS_PROCPARAMS 0x10000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_INPUT 0x1u
#define NDS_FTMAIN_PROCPARAMS_UPDATE_DAMAGE 0x2u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS 0x4u
#define NDS_FTMAIN_PROCPARAMS_SHUFFLE 0x8u
#define NDS_FTMAIN_PROCPARAMS_RUMBLE 0x10u
#define NDS_FTMAIN_PROCPARAMS_HITLAG 0x20u
#define NDS_FTMAIN_PROCPARAMS_CLEAR 0x40u
#define NDS_FTMAIN_PROCPARAMS_LAGSTART 0x80u
#define NDS_FTMAIN_PROCPARAMS_ATTACK_DAMAGE 0x100u
#define NDS_FTMAIN_PROCPARAMS_ATTACK_SHIELD_PUSH 0x200u
#define NDS_FTMAIN_PROCPARAMS_SHIELD_DAMAGE 0x400u
#define NDS_FTMAIN_PROCPARAMS_SHIELD_BREAK 0x800u
#define NDS_FTMAIN_PROCPARAMS_REFLECT_BREAK 0x1000u
#define NDS_FTMAIN_PROCPARAMS_REFLECT_HIT 0x2000u
#define NDS_FTMAIN_PROCPARAMS_REFLECT_SOUND 0x4000u
#define NDS_FTMAIN_PROCPARAMS_ABSORB 0x8000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS_SELECT 0x10000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS_SETUP 0x20000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH 0x40000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_RELEASE 0x80000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE 0x100000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_ZERO 0x200000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_ZERO 0x400000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_STATS 0x800000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_STATS 0x1000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_RELEASE 0x2000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_NODAMAGE 0x4000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_NODAMAGE 0x8000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_TAIL_COLANIM 0x10000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_TAIL_STATUS 0x20000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_ITEM_RESIST 0x40000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_ITEM_DROP 0x80000000u
#define NDS_FTMAIN_PROCPARAMS_REBOUND_STATUS 0x1u
#define NDS_FTMAIN_PROCPARAMS_REBOUND_CALLBACKS 0x2u
#define NDS_FTMAIN_PROCPARAMS_REBOUND_VECTOR 0x4u
#define NDS_FTMAIN_PROCPARAMS_REBOUND_HITLAG 0x8u
#define NDS_FTMAIN_PROCPARAMS_REBOUND_CLEAR 0x10u
#define NDS_FTMAIN_DAMAGE_SLEEP_ELEMENT 0x1u
#define NDS_FTMAIN_DAMAGE_SLEEP_ZERO_KNOCKBACK 0x2u
#define NDS_FTMAIN_DAMAGE_SLEEP_FURA_COLANIM 0x4u
#define NDS_FTMAIN_DAMAGE_SLEEP_STATUS 0x8u
#define NDS_FTMAIN_DAMAGE_SLEEP_MOTION 0x10u
#define NDS_FTMAIN_DAMAGE_SLEEP_CLIFF_WAIT 0x20u
#define NDS_FTMAIN_DAMAGE_SLEEP_RESTORE 0x40u
#define NDS_DAMAGE_ITEM_BYPASS_LIGHT 0x1u
#define NDS_DAMAGE_ITEM_BYPASS_HEAVY_NON_DK 0x2u
#define NDS_DAMAGE_ITEM_BYPASS_TAIL_COLANIM 0x4u
#define NDS_DAMAGE_ITEM_BYPASS_KEEP_ITEM 0x8u
#define NDS_DAMAGE_ITEM_BYPASS_RESTORE 0x10u
#define NDS_DAMAGE_ITEM_HEAVY_BRANCH 0x1u
#define NDS_DAMAGE_ITEM_HEAVY_RESIST 0x2u
#define NDS_DAMAGE_ITEM_HEAVY_DROP 0x4u
#define NDS_DAMAGE_ITEM_HEAVY_RETURN 0x8u
#define NDS_DAMAGE_ITEM_HEAVY_RESTORE 0x10u
#define NDS_DAMAGE_KIND_PRESERVE_BEFORE 0x1u
#define NDS_DAMAGE_KIND_PRESERVE_AFTER 0x2u
#define NDS_DAMAGE_KIND_PRESERVE_RESTORE 0x4u
#define NDS_DAMAGE_KIND_PRESERVE_ORIGINAL_INIT 0x8u
#define NDS_DAMAGE_KIND_PRESERVE_ORIGINAL_GOTO 0x10u
#define NDS_DAMAGE_KIND_PROCPARAMS_TWISTER 0x20u
#define NDS_DAMAGE_KIND_PROCPARAMS_TRAP 0x40u
#define NDS_DAMAGE_VOICE_ATTR 0x1u
#define NDS_DAMAGE_VOICE_THRESHOLD_CALL 0x2u
#define NDS_DAMAGE_VOICE_FORCE_CALL 0x4u
#define NDS_DAMAGE_VOICE_RESTORE 0x8u
#define NDS_DAMAGE_FLYROLL_PERCENT 0x1u
#define NDS_DAMAGE_FLYROLL_ANGLE 0x2u
#define NDS_DAMAGE_FLYROLL_RNG 0x4u
#define NDS_DAMAGE_FLYROLL_STATUS 0x8u
#define NDS_DAMAGE_FLYROLL_RESTORE 0x10u
#define NDS_DAMAGE_FLYTOP_LEVEL 0x1u
#define NDS_DAMAGE_FLYTOP_ANGLE 0x2u
#define NDS_DAMAGE_FLYTOP_STATUS 0x4u
#define NDS_DAMAGE_FLYTOP_RESTORE 0x8u
#define NDS_DAMAGE_REPLACE_STATUS 0x1u
#define NDS_DAMAGE_REPLACE_ELECTRIC 0x2u
#define NDS_DAMAGE_REPLACE_PASSIVE 0x4u
#define NDS_DAMAGE_REPLACE_RESTORE 0x8u
#define NDS_DAMAGE_REPLACE_DISPATCH 0x10u
#define NDS_DAMAGE_REPLACE_HITLAG_BLOCK 0x20u
#define NDS_DAMAGE_STATUS_SELECT_LEVEL 0x1u
#define NDS_DAMAGE_STATUS_SELECT_GROUND 0x2u
#define NDS_DAMAGE_STATUS_SELECT_AIR 0x4u
#define NDS_DAMAGE_STATUS_SELECT_ELECTRIC 0x8u
#define NDS_DAMAGE_STATUS_SELECT_PARKED 0x10u
#define NDS_DAMAGE_STATUS_SETUP_INIT 0x1u
#define NDS_DAMAGE_STATUS_SETUP_STATUS 0x2u
#define NDS_DAMAGE_STATUS_SETUP_MOTION 0x4u
#define NDS_DAMAGE_STATUS_SETUP_CALLBACKS 0x8u
#define NDS_DAMAGE_STATUS_SETUP_HITSTUN 0x10u
#define NDS_DAMAGE_STATUS_SETUP_UPDATE 0x20u
#define NDS_DAMAGE_STATUS_SETUP_RESTORE 0x40u
#define NDS_DAMAGE_STATUS_SETUP_PHYSICS 0x80u
#define NDS_DAMAGE_STATUS_SETUP_INTERRUPT 0x100u
#define NDS_DAMAGE_STATUS_SETUP_EXPIRE 0x200u
#define NDS_DAMAGE_STATUS_SETUP_MAP 0x400u
#define NDS_DAMAGE_STATUS_SETUP_MAP_FLOOR 0x800u
#define NDS_DAMAGE_STATUS_SETUP_MAP_CLIFF 0x1000u
#define NDS_DAMAGE_STATUS_SETUP_FALL_PHYSICS 0x2000u
#define NDS_DAMAGE_STATUS_SETUP_FASTFALL 0x4000u
#define NDS_DAMAGE_STATUS_SETUP_AIR_MAP 0x8000u
#define NDS_DAMAGE_STATUS_SETUP_FLYROLL_PHYSICS 0x10000u
#define NDS_DAMAGE_STATUS_SETUP_KNOCKBACK_INVINCIBLE 0x20000u
#define NDS_DAMAGE_STATUS_SETUP_LAGUPDATE 0x40000u
#define NDS_DAMAGE_STATUS_SETUP_HITLAG_LIFECYCLE 0x80000u
#define NDS_DAMAGE_STATUS_SETUP_PUBLIC 0x100000u
#define NDS_DAMAGE_STATUS_SETUP_COLANIM 0x200000u
#define NDS_DAMAGE_STATUS_SETUP_SCREENFLASH 0x400000u
#define NDS_DAMAGE_STATUS_SETUP_RUMBLE 0x800000u
#define NDS_DAMAGE_STATUS_SETUP_DUST 0x1000000u
#define NDS_DAMAGE_STATUS_SETUP_PLAYERTAG 0x2000000u
#define NDS_DAMAGE_STATUS_SETUP_ATTACKER 0x4000000u
#define NDS_DAMAGE_STATUS_SETUP_PROC_PASSIVE 0x8000000u
#define NDS_DAMAGE_STATUS_SETUP_PROC_PASSIVE_TICK 0x10000000u
#define NDS_DAMAGE_STATUS_SETUP_PROC_PASSIVE_STATUS 0x20000000u
#define NDS_DAMAGE_STATUS_SETUP_SLEEP_STATUS 0x40000000u
#define NDS_DAMAGE_STATUS_SETUP_HAMMER_INTERRUPT 0x80000000u
#define NDS_DAMAGE_COMMON_PHYSICS_GROUND 0x1u
#define NDS_DAMAGE_COMMON_PHYSICS_AIR_FRICTION 0x2u
#define NDS_DAMAGE_COMMON_PHYSICS_AIR_DRIFT 0x4u
#define NDS_DAMAGE_COMMON_PHYSICS_CLEAR_ATTACK 0x8u
#define NDS_DAMAGE_COMMON_PHYSICS_RESTORE 0x10u
#define NDS_DAMAGE_COMMON_PHYSICS_ORIGINAL 0x20u
#define NDS_DAMAGE_COMMON_CALLBACK_GROUND_UPDATE 0x1u
#define NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE 0x2u
#define NDS_DAMAGE_COMMON_CALLBACK_GROUND_INTERRUPT 0x4u
#define NDS_DAMAGE_COMMON_CALLBACK_AIR_INTERRUPT 0x8u
#define NDS_DAMAGE_COMMON_CALLBACK_HAMMER_GROUND 0x10u
#define NDS_DAMAGE_COMMON_CALLBACK_HAMMER_AIR 0x20u
#define NDS_DAMAGE_COMMON_CALLBACK_RESTORE 0x40u
#define NDS_DAMAGE_COMMON_CALLBACK_GROUND_STAY 0x80u
#define NDS_DAMAGE_COMMON_CALLBACK_AIR_STAY 0x100u
#define NDS_DAMAGE_COMMON_CALLBACK_GROUND_UPDATE_ORIGINAL 0x200u
#define NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE_ORIGINAL 0x400u
#define NDS_DAMAGE_COMMON_CALLBACK_COMMON_INTERRUPT_ORIGINAL 0x800u
#define NDS_DAMAGE_COMMON_CALLBACK_AIR_INTERRUPT_ORIGINAL 0x1000u
#define NDS_DAMAGE_COMMON_CALLBACK_FALL_INTERRUPT 0x2000u
#define NDS_DAMAGE_AIR_MAP_WALL_COLLISION 0x1u
#define NDS_DAMAGE_AIR_MAP_WALL_HELPER 0x2u
#define NDS_DAMAGE_AIR_MAP_WALL_SHORT_CIRCUIT 0x4u
#define NDS_DAMAGE_AIR_MAP_WALL_KNOCKBACK 0x8u
#define NDS_DAMAGE_AIR_MAP_WALL_RESTORE 0x10u
#define NDS_DAMAGE_AIR_MAP_WALL_ORIGINAL 0x20u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_FIXED 0x1u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_AIR_361 0x2u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_LOW_361 0x4u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_HIGH_361 0x8u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_CAP_361 0x10u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_ORIGINAL 0x20u
#define NDS_DAMAGE_FALL_INTERRUPT_CALL 0x1u
#define NDS_DAMAGE_FALL_INTERRUPT_SPECIAL 0x2u
#define NDS_DAMAGE_FALL_INTERRUPT_ATTACK 0x4u
#define NDS_DAMAGE_FALL_INTERRUPT_JUMP 0x8u
#define NDS_DAMAGE_FALL_INTERRUPT_HAMMER 0x10u
#define NDS_DAMAGE_FALL_INTERRUPT_RESTORE 0x20u
#define NDS_DAMAGE_DUST_LOW 0x1u
#define NDS_DAMAGE_DUST_MID_LOW 0x2u
#define NDS_DAMAGE_DUST_MID 0x4u
#define NDS_DAMAGE_DUST_MID_HIGH 0x8u
#define NDS_DAMAGE_DUST_HIGH 0x10u
#define NDS_DAMAGE_DUST_DEFAULT_AIR 0x20u
#define NDS_DAMAGE_DUST_RESTORE 0x40u
#define NDS_DAMAGE_DUST_ORIGINAL 0x80u
#define NDS_DAMAGE_DUST_UPDATE_DEC 0x1u
#define NDS_DAMAGE_DUST_UPDATE_EFFECT 0x2u
#define NDS_DAMAGE_DUST_UPDATE_RESET 0x4u
#define NDS_DAMAGE_DUST_UPDATE_RESTORE 0x8u
#define NDS_DAMAGE_DUST_UPDATE_ORIGINAL 0x10u
#define NDS_DAMAGE_HITSTUN_PUBLIC_DEC 0x1u
#define NDS_DAMAGE_HITSTUN_PUBLIC_TRANSFER 0x2u
#define NDS_DAMAGE_HITSTUN_PUBLIC_RESTORE 0x4u
#define NDS_DAMAGE_HITSTUN_PUBLIC_ORIGINAL 0x8u
#define NDS_DAMAGE_COLANIM_FIRE 0x1u
#define NDS_DAMAGE_COLANIM_ELECTRIC 0x2u
#define NDS_DAMAGE_COLANIM_FREEZE 0x4u
#define NDS_DAMAGE_COLANIM_NORMAL 0x8u
#define NDS_DAMAGE_COLANIM_RESTORE 0x10u
#define NDS_DAMAGE_COLANIM_ORIGINAL 0x20u
#define NDS_DAMAGE_COLANIM_UPDATE_DIRECT 0x1u
#define NDS_DAMAGE_COLANIM_UPDATE_SET 0x2u
#define NDS_DAMAGE_COLANIM_UPDATE_NOOP 0x4u
#define NDS_DAMAGE_COLANIM_UPDATE_RESTORE 0x8u
#define NDS_DAMAGE_COLANIM_UPDATE_ORIGINAL 0x10u
#define NDS_DAMAGE_INVINCIBLE_HITLAG_BLOCK 0x1u
#define NDS_DAMAGE_INVINCIBLE_FLAG_BLOCK 0x2u
#define NDS_DAMAGE_INVINCIBLE_SET 0x4u
#define NDS_DAMAGE_INVINCIBLE_RESTORE 0x8u
#define NDS_DAMAGE_INVINCIBLE_ORIGINAL 0x10u
#define NDS_DAMAGE_LAGUPDATE_HITLAG_BLOCK 0x1u
#define NDS_DAMAGE_LAGUPDATE_STICK_BLOCK 0x2u
#define NDS_DAMAGE_LAGUPDATE_TAP_BLOCK 0x4u
#define NDS_DAMAGE_LAGUPDATE_APPLY 0x8u
#define NDS_DAMAGE_LAGUPDATE_RESTORE 0x10u
#define NDS_DAMAGE_LAGUPDATE_ORIGINAL 0x20u
#define NDS_DAMAGE_FLASH_LOW_NOOP 0x1u
#define NDS_DAMAGE_FLASH_FIRE 0x2u
#define NDS_DAMAGE_FLASH_ELECTRIC 0x4u
#define NDS_DAMAGE_FLASH_FREEZE 0x8u
#define NDS_DAMAGE_FLASH_NORMAL 0x10u
#define NDS_DAMAGE_FLASH_RESTORE 0x20u
#define NDS_DAMAGE_FLASH_ORIGINAL 0x40u
#define NDS_DAMAGE_PUBLIC_ANGLE_REDUCE 0x1u
#define NDS_DAMAGE_PUBLIC_TARGET_RESET 0x2u
#define NDS_DAMAGE_PUBLIC_FORCE 0x4u
#define NDS_DAMAGE_PUBLIC_RESTORE 0x8u
#define NDS_DAMAGE_PUBLIC_NO_FORCE 0x10u
#define NDS_DAMAGE_PUBLIC_ORIGINAL 0x20u
#define NDS_DAMAGE_LEVEL_LOW 0x1u
#define NDS_DAMAGE_LEVEL_MID 0x2u
#define NDS_DAMAGE_LEVEL_HIGH 0x4u
#define NDS_DAMAGE_LEVEL_FLY 0x8u
#define NDS_DAMAGE_LEVEL_ORIGINAL 0x10u
#define NDS_DAMAGE_HOLD_RESIST_SLEEP_FALSE 0x1u
#define NDS_DAMAGE_HOLD_RESIST_ZERO_TRUE 0x2u
#define NDS_DAMAGE_HOLD_RESIST_PAUSED_TRUE 0x4u
#define NDS_DAMAGE_HOLD_RESIST_DONKEY_TRUE 0x8u
#define NDS_DAMAGE_HOLD_RESIST_DEFAULT_FALSE 0x10u
#define NDS_DAMAGE_HOLD_RESIST_KEEP_TRUE 0x20u
#define NDS_DAMAGE_HOLD_RESIST_KEEP_FALSE 0x40u
#define NDS_DAMAGE_HOLD_RESIST_RESTORE 0x80u
#define NDS_DAMAGE_UPDATE_CATCH_RESIST_ZERO_COLANIM 0x1u
#define NDS_DAMAGE_UPDATE_CATCH_RESIST_PAUSED_COLANIM 0x2u
#define NDS_DAMAGE_UPDATE_CATCH_RESIST_STATUS 0x4u
#define NDS_DAMAGE_UPDATE_CATCH_RESIST_RESTORE 0x8u
#define NDS_DAMAGE_UPDATE_CATCH_RESIST_ORIGINAL 0x10u
#define NDS_FTMAIN_HITLOG_NUM_MAX 10u

static u32 sNdsFighterDashRunProcParamsHitCount;
static u32 sNdsFighterDashRunProcParamsShieldCount;
static u32 sNdsFighterDashRunProcParamsLagStartCount;
static u32 sNdsFighterDashRunProcParamsTrapCount;
static u32 sNdsFighterDashRunDamageLagEndCount;
static FTHitLog sNdsFighterDashRunHitLogs[NDS_FTMAIN_HITLOG_NUM_MAX];
static u32 sNdsFighterDashRunHitLogID;
static void ndsFighterDashRunDamageLagEnd(GObj *fighter_gobj);
static const u16 sNdsFighterDashRunHitCollisionFGMs[8][3] = {
    { 40u, 38u, 37u },    /* Punch: S/M/L */
    { 34u, 32u, 31u },    /* Kick: S/M/L */
    { 216u, 216u, 216u }, /* Coin */
    { 28u, 27u, 25u },    /* Burn: S/M/L */
    { 24u, 23u, 22u },    /* Shock: S/M/L */
    { 263u, 262u, 261u }, /* Slash: S/M/L */
    { 51u, 51u, 51u },    /* Fan / slap */
    { 38u, 37u, 52u }     /* Bat */
};

s32 ftParamGetCapturedDamage(FTStruct *fp, s32 damage)
{
    if (fp == NULL)
    {
        return 0;
    }
    if (fp->capture_gobj != NULL)
    {
        damage = (s32)((damage * 0.5F) + 0.999F);
    }
    return (s32)((damage * fp->damage_mul) + 0.999F);
}

sb32 ftMainCheckGetUpdateDamage(FTStruct *fp, s32 *damage)
{
    if ((fp == NULL) || (damage == NULL))
    {
        return FALSE;
    }
    if (fp->is_damage_resist)
    {
        fp->damage_resist -= *damage;
        if (fp->damage_resist <= 0)
        {
            fp->is_damage_resist = FALSE;
            *damage = -fp->damage_resist;
        }
    }
    if (!fp->is_damage_resist)
    {
        fp->damage_queue += *damage;
        if (fp->damage_lag < *damage)
        {
            fp->damage_lag = *damage;
        }
        return TRUE;
    }
    return FALSE;
}

void ftMainPlayHitSFX(FTStruct *fp, FTAttackColl *attack_coll)
{
    u32 fgm_id;

    if ((fp == NULL) || (attack_coll == NULL) ||
        (attack_coll->fgm_kind >=
            ARRAY_COUNT(sNdsFighterDashRunHitCollisionFGMs)) ||
        (attack_coll->fgm_level >=
            ARRAY_COUNT(sNdsFighterDashRunHitCollisionFGMs[0])))
    {
        return;
    }

    fgm_id = sNdsFighterDashRunHitCollisionFGMs[attack_coll->fgm_kind]
                                                 [attack_coll->fgm_level];
    /* ponytail: FTStruct lacks BattleShip p_sfx/sfx_id; play collision FGM. */
    (void)func_800269C0_275C0((u16)fgm_id);
}

void ftMainUpdateDamageStatFighter(FTStruct *attacker_fp,
                                   FTAttackColl *attack_coll,
                                   FTStruct *victim_fp,
                                   FTDamageColl *damage_coll,
                                   GObj *attacker_gobj,
                                   GObj *victim_gobj)
{
    FTHitLog *hitlog;
    s32 damage;
    s32 attacker_player;
    s32 attacker_player_num;
    u32 attack_id;

    if ((attacker_fp == NULL) || (attack_coll == NULL) ||
        (victim_fp == NULL) || (damage_coll == NULL) ||
        (attacker_gobj == NULL) || (victim_gobj == NULL))
    {
        return;
    }

    ftMainSetHitInteractStats(attacker_fp, attack_coll->group_id,
                              victim_gobj, nNDSGMHitTypeDamage, 0u, FALSE);

    damage = ftParamGetCapturedDamage(victim_fp, attack_coll->damage);
    if (attacker_fp->attack_damage < damage)
    {
        attacker_fp->attack_damage = damage;
    }
    if ((victim_fp->special_hitstatus == nGMHitStatusNormal) &&
        (victim_fp->star_hitstatus == nGMHitStatusNormal) &&
        (victim_fp->hitstatus == nGMHitStatusNormal) &&
        (damage_coll->hitstatus == nGMHitStatusNormal) &&
        (ftMainCheckGetUpdateDamage(victim_fp, &damage) != FALSE))
    {
        if (attacker_fp->throw_gobj != NULL)
        {
            attacker_player = attacker_fp->throw_player;
            attacker_player_num = attacker_fp->throw_player_num;
        }
        else
        {
            attacker_player = attacker_fp->player;
            attacker_player_num = attacker_fp->player_num;
        }
        if (sNdsFighterDashRunHitLogID < NDS_FTMAIN_HITLOG_NUM_MAX)
        {
            hitlog = &sNdsFighterDashRunHitLogs[sNdsFighterDashRunHitLogID];
            hitlog->attacker_object_class = nFTHitLogObjectFighter;
            hitlog->attack_coll = attack_coll;
            hitlog->attack_id = 0;
            for (attack_id = 0u; attack_id < FTATTACKCOLL_NUM_MAX; attack_id++)
            {
                if (&attacker_fp->attack_colls[attack_id] == attack_coll)
                {
                    hitlog->attack_id = (s32)attack_id;
                    break;
                }
            }
            hitlog->attacker_gobj = attacker_gobj;
            hitlog->damage_coll = damage_coll;
            hitlog->attacker_player = (u8)attacker_player;
            hitlog->attacker_player_num = attacker_player_num;
            sNdsFighterDashRunHitLogID++;
        }
        ftParamUpdatePlayerBattleStats(attacker_player, victim_fp->player,
                                       damage);
        ftParamUpdateStaleQueue(attacker_player, victim_fp->player,
                                (s32)attack_coll->motion_attack_id,
                                attack_coll->motion_count);
    }
    ftMainPlayHitSFX(attacker_fp, attack_coll);
}

void ftMainSetHitRebound(GObj *attacker_gobj, FTStruct *fp,
                         FTAttackColl *attack_coll, GObj *victim_gobj)
{
    DObj *attacker_root;
    DObj *victim_root;

    if ((attacker_gobj == NULL) || (fp == NULL) ||
        (attack_coll == NULL) || (victim_gobj == NULL))
    {
        return;
    }
    if (fp->attack_shield_push < attack_coll->damage)
    {
        fp->attack_shield_push = attack_coll->damage;
        if ((attack_coll->can_rebound != FALSE) &&
            (fp->ga == nMPKineticsGround))
        {
            attacker_root = DObjGetStruct(attacker_gobj);
            victim_root = DObjGetStruct(victim_gobj);
            if ((attacker_root == NULL) || (victim_root == NULL))
            {
                return;
            }
#if defined(REGION_US)
            fp->attack_rebound = (fp->attack_shield_push * 1.62F) + 4.0F;
#else
            fp->attack_rebound = (fp->attack_shield_push * 1.75F) + 4.0F;
#endif
            fp->hit_lr =
                (attacker_root->translate.vec.f.x <
                 victim_root->translate.vec.f.x) ? +1 : -1;
        }
    }
}

void ftMainUpdateAttackStatFighter(FTStruct *other_fp,
                                   FTAttackColl *other_hit,
                                   FTStruct *this_fp,
                                   FTAttackColl *this_hit,
                                   GObj *other_gobj,
                                   GObj *this_gobj)
{
    if ((other_fp == NULL) || (other_hit == NULL) ||
        (this_fp == NULL) || (this_hit == NULL) ||
        (other_gobj == NULL) || (this_gobj == NULL))
    {
        return;
    }
    if ((this_hit->damage - 10) < other_hit->damage)
    {
        ftMainSetHitInteractStats(this_fp, this_hit->group_id, other_gobj,
                                  nNDSGMHitTypeAttack, other_hit->group_id,
                                  TRUE);
        ftMainSetHitRebound(this_gobj, this_fp, this_hit, other_gobj);
        gNdsStageMPLiveHitDamageLoopAttackClashEffectCount++;
    }
    if ((other_hit->damage - 10) < this_hit->damage)
    {
        ftMainSetHitInteractStats(other_fp, other_hit->group_id, this_gobj,
                                  nNDSGMHitTypeAttack, this_hit->group_id,
                                  FALSE);
        ftMainSetHitRebound(other_gobj, other_fp, other_hit, this_gobj);
        gNdsStageMPLiveHitDamageLoopAttackClashEffectCount++;
    }
}

void ftMainUpdateShieldStatFighter(FTStruct *attacker_fp,
                                   FTAttackColl *attack_coll,
                                   FTStruct *victim_fp,
                                   GObj *attacker_gobj,
                                   GObj *victim_gobj)
{
    DObj *attacker_root;
    DObj *victim_root;

    if ((attacker_fp == NULL) || (attack_coll == NULL) ||
        (victim_fp == NULL) || (attacker_gobj == NULL) ||
        (victim_gobj == NULL))
    {
        return;
    }

    ftMainSetHitInteractStats(attacker_fp, attack_coll->group_id,
                              victim_gobj, nNDSGMHitTypeShield, 0u, FALSE);
    if (attacker_fp->attack_shield_push < attack_coll->damage)
    {
        attacker_fp->attack_shield_push = attack_coll->damage;
    }
    victim_fp->shield_damage_total +=
        attack_coll->damage + attack_coll->shield_damage;
    if (victim_fp->shield_damage < attack_coll->damage)
    {
        victim_fp->shield_damage = attack_coll->damage;
        attacker_root = DObjGetStruct(attacker_gobj);
        victim_root = DObjGetStruct(victim_gobj);
        if ((attacker_root != NULL) && (victim_root != NULL))
        {
            victim_fp->shield_lr =
                (victim_root->translate.vec.f.x <
                 attacker_root->translate.vec.f.x) ? +1 : -1;
        }
        gNdsStageMPLiveHitDamageLoopShieldPlayer = attacker_fp->player;
    }
    gNdsStageMPLiveHitDamageLoopShieldEffectCount++;
    gNdsStageMPLiveHitDamageLoopShieldEffectSize = attack_coll->damage;
}

void ftMainUpdateCatchStatFighter(FTStruct *attacker_fp,
                                  FTAttackColl *attack_coll,
                                  FTStruct *victim_fp,
                                  GObj *attacker_gobj,
                                  GObj *victim_gobj)
{
    DObj *attacker_root;
    DObj *victim_root;
    f32 dist;

    if ((attacker_fp == NULL) || (attack_coll == NULL) ||
        (victim_fp == NULL) || (attacker_gobj == NULL) ||
        (victim_gobj == NULL))
    {
        return;
    }

    ftMainSetHitInteractStats(attacker_fp, attack_coll->group_id,
                              victim_gobj, nNDSGMHitTypeDamage, 0u, TRUE);
    attacker_root = DObjGetStruct(attacker_gobj);
    victim_root = DObjGetStruct(victim_gobj);
    if ((attacker_root == NULL) || (victim_root == NULL))
    {
        return;
    }
    dist = victim_root->translate.vec.f.x - attacker_root->translate.vec.f.x;
    if (dist < 0.0F)
    {
        dist = -dist;
    }
    if (dist < attacker_fp->search_gobj_dist)
    {
        attacker_fp->search_gobj_dist = dist;
        attacker_fp->search_gobj = victim_gobj;
    }
}

void ftMainProcessHitCollisionStatsMain(GObj *fighter_gobj)
{
    FTStruct *this_fp;
    FTStruct *attacker_fp;
    FTAttackColl *attack_coll;
    FTHitLog *hitlog;
    FTHitLog *best_hitlog = NULL;
    GObj *attacker_gobj;
    DObj *this_root;
    DObj *attacker_root;
    f32 knockback = -1.0F;
    f32 knockback_temp;
    u32 i;

    if ((fighter_gobj == NULL) || (sNdsFighterDashRunHitLogID == 0u))
    {
        return;
    }
    this_fp = ftGetStruct(fighter_gobj);
    this_root = DObjGetStruct(fighter_gobj);
    if ((this_fp == NULL) || (this_fp->attr == NULL) ||
        (this_root == NULL))
    {
        return;
    }

    for (i = 0u; i < sNdsFighterDashRunHitLogID; i++)
    {
        hitlog = &sNdsFighterDashRunHitLogs[i];
        if ((hitlog->attacker_object_class != nFTHitLogObjectFighter) ||
            (hitlog->attack_coll == NULL) ||
            (hitlog->damage_coll == NULL) ||
            (hitlog->attacker_gobj == NULL))
        {
            continue;
        }
        attack_coll = hitlog->attack_coll;
        attacker_fp = ftGetStruct(hitlog->attacker_gobj);
        if (attacker_fp == NULL)
        {
            continue;
        }
        knockback_temp =
            ftParamGetCommonKnockback(this_fp->percent_damage,
                                      this_fp->damage_queue,
                                      attack_coll->damage,
                                      attack_coll->knockback_weight,
                                      attack_coll->knockback_scale,
                                      attack_coll->knockback_base,
                                      this_fp->attr->weight,
                                      attacker_fp->handicap,
                                      this_fp->handicap);
        if (knockback < knockback_temp)
        {
            knockback = knockback_temp;
            best_hitlog = hitlog;
        }
    }
    if (best_hitlog == NULL)
    {
        return;
    }

    attack_coll = best_hitlog->attack_coll;
    attacker_gobj = best_hitlog->attacker_gobj;
    attacker_fp = ftGetStruct(attacker_gobj);
    attacker_root = DObjGetStruct(attacker_gobj);
    if ((attack_coll == NULL) || (attacker_fp == NULL) ||
        (attacker_root == NULL))
    {
        return;
    }

    this_fp->damage_angle = attack_coll->angle;
    this_fp->damage_element = attack_coll->element;
    this_fp->damage_lr =
        (this_root->translate.vec.f.x <
         attacker_root->translate.vec.f.x) ? +1 : -1;
    this_fp->damage_player_num = best_hitlog->attacker_player_num;
    ftParamUpdate1PGameDamageStats(this_fp, best_hitlog->attacker_player,
                                   best_hitlog->attacker_object_class,
                                   attacker_fp->fkind,
                                   attacker_fp->stat_flags.halfword & ~0x400u,
                                   attacker_fp->stat_count);
    this_fp->damage_joint_id = best_hitlog->damage_coll->joint_id;
    this_fp->damage_index = best_hitlog->damage_coll->placement;
    this_fp->damage_knockback = knockback;
    this_fp->damage_kind = nFTDamageKindStatus;
    if (this_fp->damage_element == nGMHitElementElectric)
    {
        attacker_fp->hitlag_mul = 1.5F;
        this_fp->hitlag_mul = 1.5F;
    }
}

#define NDS_FTMAIN_GROUND_OBSTACLE_COUNT 2u

typedef struct NDSFTMainGroundObstacle {
    GObj *gobj;
    sb32 (*proc_update)(GObj *, GObj *, s32 *);
} NDSFTMainGroundObstacle;

static NDSFTMainGroundObstacle
    sNdsFTMainGroundObstacles[NDS_FTMAIN_GROUND_OBSTACLE_COUNT];
static u32 sNdsFTMainGroundObstaclesNum;

sb32 ftMainCheckAddGroundObstacle(GObj *gobj,
                                  sb32 (*proc_update)(GObj *, GObj *, s32 *))
{
    u32 i;

    for (i = 0u; i < NDS_FTMAIN_GROUND_OBSTACLE_COUNT; i++)
    {
        if (sNdsFTMainGroundObstacles[i].gobj == NULL)
        {
            sNdsFTMainGroundObstacles[i].gobj = gobj;
            sNdsFTMainGroundObstacles[i].proc_update = proc_update;
            sNdsFTMainGroundObstaclesNum++;
            return TRUE;
        }
    }
    return FALSE;
}

void ftMainClearGroundObstacle(GObj *gobj)
{
    u32 i;

    for (i = 0u; i < NDS_FTMAIN_GROUND_OBSTACLE_COUNT; i++)
    {
        if (sNdsFTMainGroundObstacles[i].gobj == gobj)
        {
            sNdsFTMainGroundObstacles[i].gobj = NULL;
            sNdsFTMainGroundObstacles[i].proc_update = NULL;
            if (sNdsFTMainGroundObstaclesNum != 0u)
            {
                sNdsFTMainGroundObstaclesNum--;
            }
            break;
        }
    }
}

void ftMainSetHitHazard(GObj *gobj, GObj *fighter_gobj, FTStruct *fp,
                        s32 kind)
{
    (void)fp;

    if (kind == nGMHitEnvironmentTwister)
    {
        ftCommonTwisterSetStatus(fighter_gobj, gobj);
    }
    else if (kind == nGMHitEnvironmentTaruCann)
    {
        ftCommonTaruCannSetStatus(fighter_gobj, gobj);
    }
}

void ftMainSearchHitHazard(GObj *fighter_gobj)
{
    FTStruct *fp;
    NDSFTMainGroundObstacle *obstacle;
    u32 i;

    if (fighter_gobj == NULL)
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (fp->is_ghost != FALSE))
    {
        return;
    }

    if (fp->hitlag_tics == 0)
    {
        if (fp->twister_wait != 0)
        {
            fp->twister_wait--;
        }
        if (fp->tarucann_wait != 0)
        {
            fp->tarucann_wait--;
        }
    }
    obstacle = &sNdsFTMainGroundObstacles[0];
    for (i = 0u; i < sNdsFTMainGroundObstaclesNum; i++, obstacle++)
    {
        if ((obstacle->gobj != NULL) && (obstacle->proc_update != NULL))
        {
            s32 kind;

            if (obstacle->proc_update(obstacle->gobj, fighter_gobj, &kind) !=
                FALSE)
            {
                ftMainSetHitHazard(obstacle->gobj, fighter_gobj, fp, kind);
            }
        }
    }
}

void ftMainSearchHitFighter(GObj *this_gobj)
{
    GObj *other_gobj;
    FTStruct *this_fp;
    FTStruct *other_fp;
    DObj *this_root;
    FTAttackColl *attack_coll;
    FTAttackColl *this_attack_coll;
    FTDamageColl *damage_coll;
    GMHitFlags flags;
    f32 angle;
    u32 i;
    u32 j;
    u32 slot;
    u32 detect_count;
    u32 attack_detect_count;
    sb32 is_team_blocked;
    sb32 is_check_self;

    if (this_gobj == NULL)
    {
        return;
    }
    this_fp = ftGetStruct(this_gobj);
    this_root = DObjGetStruct(this_gobj);
    if ((this_fp == NULL) || (this_fp->attr == NULL) ||
        (this_root == NULL))
    {
        return;
    }

    other_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    is_check_self = FALSE;
    while (other_gobj != NULL)
    {
        if (other_gobj == this_gobj)
        {
            is_check_self = TRUE;
            other_gobj = other_gobj->link_next;
            continue;
        }
        if (other_gobj == this_fp->capture_gobj)
        {
            other_gobj = other_gobj->link_next;
            continue;
        }

        other_fp = ftGetStruct(other_gobj);
        if ((other_fp == NULL) ||
            (other_fp->is_catch_or_capture != FALSE))
        {
            other_gobj = other_gobj->link_next;
            continue;
        }
        if ((other_fp->throw_gobj != NULL) &&
            (this_gobj == other_fp->throw_gobj))
        {
            other_gobj = other_gobj->link_next;
            continue;
        }

        is_team_blocked =
            ((gSCManagerBattleState != NULL) &&
             (gSCManagerBattleState->is_team_battle != FALSE) &&
             (gSCManagerBattleState->is_team_attack == FALSE) &&
             (((other_fp->throw_gobj != NULL) ? other_fp->throw_team :
                                               other_fp->team) ==
              this_fp->team)) ? TRUE : FALSE;
        if (is_team_blocked != FALSE)
        {
            other_gobj = other_gobj->link_next;
            continue;
        }

        detect_count = 0u;
        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            attack_coll = &other_fp->attack_colls[i];
            gFTMainIsDamageDetect[i] = FALSE;
            if (attack_coll->attack_state == nGMAttackStateOff)
            {
                continue;
            }
            if (((this_fp->ga == nMPKineticsAir) &&
                 (attack_coll->is_hit_air == FALSE)) ||
                ((this_fp->ga == nMPKineticsGround) &&
                 (attack_coll->is_hit_ground == FALSE)))
            {
                continue;
            }

            flags.is_interact_hurt = FALSE;
            flags.is_interact_shield = FALSE;
            flags.timer_rehit = 0;
            flags.group_id = 7u;
            for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
            {
                if (attack_coll->attack_records[slot].victim_gobj ==
                    this_gobj)
                {
                    flags = attack_coll->attack_records[slot].victim_flags;
                    break;
                }
            }
            if ((flags.is_interact_hurt != FALSE) ||
                (flags.is_interact_shield != FALSE) ||
                (flags.group_id != 7u))
            {
                continue;
            }
            /* ponytail: current proof already checks the range shim separately. */
            if ((ndsFighterDashRunCheckAttackInFighterRange(
                    &attack_coll->pos_curr, &this_root->translate.vec.f,
                    &this_fp->attr->hit_detect_range,
                    attack_coll->size) == FALSE) &&
                (ndsFighterDashRunCheckAttackInFighterRange(
                    &attack_coll->pos_prev, &this_root->translate.vec.f,
                    &this_fp->attr->hit_detect_range,
                    attack_coll->size) == FALSE) &&
                (ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() ==
                    FALSE))
            {
                continue;
            }

            gFTMainIsDamageDetect[i] = TRUE;
            detect_count++;
        }
        if (detect_count == 0u)
        {
            other_gobj = other_gobj->link_next;
            continue;
        }

        if ((is_check_self != FALSE) &&
            (this_fp->is_catch_or_capture == FALSE) &&
            (this_fp->ga == nMPKineticsGround) &&
            (other_fp->ga == nMPKineticsGround))
        {
            attack_detect_count = 0u;
            for (j = 0u; j < FTATTACKCOLL_NUM_MAX; j++)
            {
                this_attack_coll = &this_fp->attack_colls[j];
                gFTMainIsAttackDetect[j] = FALSE;
                if (this_attack_coll->attack_state == nGMAttackStateOff)
                {
                    continue;
                }
                if (((other_fp->ga == nMPKineticsAir) &&
                     (this_attack_coll->is_hit_air == FALSE)) ||
                    ((other_fp->ga == nMPKineticsGround) &&
                     (this_attack_coll->is_hit_ground == FALSE)))
                {
                    continue;
                }

                flags.is_interact_hurt = FALSE;
                flags.is_interact_shield = FALSE;
                flags.timer_rehit = 0;
                flags.group_id = 7u;
                for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
                {
                    if (this_attack_coll->attack_records[slot].victim_gobj ==
                        other_gobj)
                    {
                        flags =
                            this_attack_coll->attack_records[slot].victim_flags;
                        break;
                    }
                }
                if ((flags.is_interact_hurt != FALSE) ||
                    (flags.is_interact_shield != FALSE) ||
                    (flags.group_id != 7u))
                {
                    continue;
                }
                gFTMainIsAttackDetect[j] = TRUE;
                attack_detect_count++;
            }

            if (attack_detect_count != 0u)
            {
                for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
                {
                    if (gFTMainIsDamageDetect[i] == FALSE)
                    {
                        continue;
                    }
                    attack_coll = &other_fp->attack_colls[i];
                    for (j = 0u; j < FTATTACKCOLL_NUM_MAX; j++)
                    {
                        if (gFTMainIsAttackDetect[j] == FALSE)
                        {
                            continue;
                        }
                        this_attack_coll = &this_fp->attack_colls[j];
                        if (ndsGMCollisionCheckFighterAttacksCollideSelected(
                                attack_coll, this_attack_coll) != FALSE)
                        {
                            ftMainUpdateAttackStatFighter(other_fp,
                                                          attack_coll,
                                                          this_fp,
                                                          this_attack_coll,
                                                          other_gobj,
                                                          this_gobj);
                            if (gFTMainIsDamageDetect[i] == FALSE)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (this_fp->is_shield != FALSE)
        {
            for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
            {
                if (gFTMainIsDamageDetect[i] == FALSE)
                {
                    continue;
                }
                attack_coll = &other_fp->attack_colls[i];
                angle = 0.0F;
                if (ndsGMCollisionCheckFighterAttackShieldCollideSelected(
                        attack_coll, this_fp->joints[nFTPartsJointYRotN],
                        &angle) != FALSE)
                {
                    ftMainUpdateShieldStatFighter(other_fp, attack_coll,
                                                  this_fp, other_gobj,
                                                  this_gobj);
                }
            }
        }

        if ((this_fp->special_hitstatus != nGMHitStatusIntangible) &&
            (this_fp->star_hitstatus != nGMHitStatusIntangible) &&
            (this_fp->hitstatus != nGMHitStatusIntangible))
        {
            for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
            {
                if (gFTMainIsDamageDetect[i] == FALSE)
                {
                    continue;
                }
                attack_coll = &other_fp->attack_colls[i];
                for (j = 0u; j < FTDAMAGECOLL_NUM_MAX; j++)
                {
                    damage_coll = &this_fp->damage_colls[j];
                    if (damage_coll->hitstatus == nGMHitStatusNone)
                    {
                        break;
                    }
                    if (damage_coll->hitstatus == nGMHitStatusIntangible)
                    {
                        continue;
                    }
                    if (ndsGMCollisionCheckFighterAttackDamageCollideSelected(
                            attack_coll, damage_coll) != FALSE)
                    {
                        ftMainUpdateDamageStatFighter(other_fp, attack_coll,
                                                      this_fp, damage_coll,
                                                      other_gobj, this_gobj);
                        break;
                    }
                }
            }
        }

        other_gobj = other_gobj->link_next;
    }
}

void ftMainSearchFighterCatch(GObj *this_gobj)
{
    GObj *other_gobj;
    FTStruct *this_fp;
    FTStruct *other_fp;
    FTAttackColl *attack_coll;
    FTDamageColl *damage_coll;
    GMHitFlags flags;
    u32 i;
    u32 j;
    u32 slot;

    if (this_gobj == NULL)
    {
        return;
    }
    this_fp = ftGetStruct(this_gobj);
    if (this_fp == NULL)
    {
        return;
    }

    this_fp->search_gobj = NULL;
    this_fp->search_gobj_dist = F32_MAX;

    other_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    while (other_gobj != NULL)
    {
        if (other_gobj == this_gobj)
        {
            other_gobj = other_gobj->link_next;
            continue;
        }
        other_fp = ftGetStruct(other_gobj);
        if ((other_fp == NULL) || (other_fp->is_ghost != FALSE) ||
            (other_fp->fkind == nFTKindBoss))
        {
            other_gobj = other_gobj->link_next;
            continue;
        }
        if ((gSCManagerBattleState != NULL) &&
            (gSCManagerBattleState->is_team_battle != FALSE) &&
            (gSCManagerBattleState->is_team_attack == FALSE) &&
            (this_fp->team == other_fp->team))
        {
            other_gobj = other_gobj->link_next;
            continue;
        }
        if ((other_fp->capture_immune_mask & this_fp->catch_mask) != 0u)
        {
            other_gobj = other_gobj->link_next;
            continue;
        }
        if ((other_fp->special_hitstatus != nGMHitStatusNormal) ||
            (other_fp->star_hitstatus != nGMHitStatusNormal) ||
            (other_fp->hitstatus != nGMHitStatusNormal))
        {
            other_gobj = other_gobj->link_next;
            continue;
        }

        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            attack_coll = &this_fp->attack_colls[i];
            if (attack_coll->attack_state == nGMAttackStateOff)
            {
                continue;
            }
            if (((other_fp->ga == nMPKineticsAir) &&
                 (attack_coll->is_hit_air == FALSE)) ||
                ((other_fp->ga == nMPKineticsGround) &&
                 (attack_coll->is_hit_ground == FALSE)))
            {
                continue;
            }

            flags.is_interact_hurt = FALSE;
            flags.is_interact_shield = FALSE;
            flags.timer_rehit = 0;
            flags.group_id = 7u;
            for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
            {
                if (attack_coll->attack_records[slot].victim_gobj ==
                    other_gobj)
                {
                    flags = attack_coll->attack_records[slot].victim_flags;
                    break;
                }
            }
            if ((flags.is_interact_hurt != FALSE) ||
                (flags.is_interact_shield != FALSE) ||
                (flags.group_id != 7u))
            {
                continue;
            }

            for (j = 0u; j < FTDAMAGECOLL_NUM_MAX; j++)
            {
                damage_coll = &other_fp->damage_colls[j];
                if (damage_coll->hitstatus == nGMHitStatusNone)
                {
                    break;
                }
                if ((damage_coll->hitstatus == nGMHitStatusIntangible) ||
                    (damage_coll->hitstatus == nGMHitStatusInvincible) ||
                    (damage_coll->is_grabbable == FALSE))
                {
                    continue;
                }
                if (ndsGMCollisionCheckFighterAttackDamageCollideSelected(
                        attack_coll, damage_coll) != FALSE)
                {
                    ftMainUpdateCatchStatFighter(this_fp, attack_coll,
                                                 other_fp, this_gobj,
                                                 other_gobj);
                    break;
                }
            }
        }

        other_gobj = other_gobj->link_next;
    }
}

void ftMainProcSearchCatch(GObj *fighter_gobj)
{
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

    ftMainSearchHitHazard(fighter_gobj);

    if (fp->is_catchstatus != FALSE)
    {
        ftMainSearchFighterCatch(fighter_gobj);
        if (fp->search_gobj != NULL)
        {
            if (fp->proc_catch != NULL)
            {
                fp->proc_catch(fighter_gobj);
            }
            if (fp->proc_capture != NULL)
            {
                fp->proc_capture(fp->search_gobj, fighter_gobj);
            }
        }
    }
}

void ftMainSearchHitItem(GObj *fighter_gobj)
{
    if ((fighter_gobj != NULL) &&
        (ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE))
    {
        gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount++;
    }
}

void ftMainSearchHitWeapon(GObj *fighter_gobj)
{
    if ((fighter_gobj != NULL) &&
        (ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE))
    {
        gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount++;
    }
}

void ftMainSearchGroundHit(GObj *fighter_gobj)
{
    if ((fighter_gobj != NULL) &&
        (ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE))
    {
        gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount++;
    }
}

void ftMainProcSearchHitAll(GObj *fighter_gobj)
{
    FTStruct *fp;

    if (fighter_gobj == NULL)
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (fp->is_ghost != FALSE))
    {
        return;
    }

    sNdsFighterDashRunHitLogID = 0u;
    ftMainSearchHitFighter(fighter_gobj);
    ftMainSearchHitItem(fighter_gobj);
    ftMainSearchHitWeapon(fighter_gobj);
    ftMainSearchGroundHit(fighter_gobj);
    if (sNdsFighterDashRunHitLogID != 0u)
    {
        ftMainProcessHitCollisionStatsMain(fighter_gobj);
    }
}

void ftMainProcParams(GObj *fighter_gobj)
{
    FTStruct *fp;
    s32 damage = 0;
    s32 status_id;
    f32 knockback_resist;
    sb32 is_shieldbreak = FALSE;
    u32 hitlag_tics;
    sb32 is_knockback_paused = FALSE;
    void (*proc_lagstart)(GObj *);

    if (fighter_gobj == NULL)
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if (fp == NULL)
    {
        return;
    }

    status_id = fp->status_id;
    hitlag_tics = fp->hitlag_tics;
    proc_lagstart = fp->proc_lagstart;

    if (!(fp->is_shield) && (fp->shield_health < 55))
    {
        fp->shield_heal_wait--;
        if (fp->shield_heal_wait == 0.0F)
        {
            fp->shield_health++;
            fp->shield_heal_wait = 10.0F;
        }
    }
    fp->shield_health -= fp->shield_damage_total;
    if (fp->shield_health <= 0)
    {
        fp->shield_health = 30;
        is_shieldbreak = TRUE;
    }

    if (fp->damage_knockback != 0.0F)
    {
        if ((fp->status_id == nFTCommonStatusSquat) ||
            (fp->status_id == nFTCommonStatusSquatWait))
        {
            fp->damage_knockback *= (2.0F / 3.0F);
        }
        knockback_resist =
            (fp->knockback_resist_status < fp->knockback_resist_passive) ?
                fp->knockback_resist_passive : fp->knockback_resist_status;
        fp->damage_knockback -= knockback_resist;
        if (fp->damage_knockback <= 0.0F)
        {
            fp->damage_knockback = 0.0F;
        }
        if (fp->status_id == nFTCommonStatusTwister)
        {
            fp->damage_kind = nFTDamageKindColAnim;
        }

        ftParamUpdateDamage(fp, fp->damage_queue);

        if (fp->proc_trap != NULL)
        {
            fp->proc_trap(fighter_gobj);
        }
        if (fp->fkind != nFTKindBoss)
        {
            switch (fp->damage_kind)
            {
            case nFTDamageKindNone:
                break;

            case nFTDamageKindStatus:
                ftParamStopVoiceRunProcDamage(fighter_gobj);
                ftCommonDamageGotoDamageStatus(fighter_gobj);
                break;

            case nFTDamageKindColAnim:
                ftCommonDamageSetDamageColAnim(fighter_gobj);
                break;

            case nFTDamageKindCatch:
                ftParamStopVoiceRunProcDamage(fighter_gobj);
                ftCommonDamageUpdateCatchResist(fighter_gobj);
                break;

            default:
                ftCommonDamageUpdateMain(fighter_gobj);
                break;
            }
        }

        damage = fp->damage_lag;
        is_knockback_paused = TRUE;
        ftParamSetDamageShuffle(
            fp, (fp->damage_element == nGMHitElementElectric) ? TRUE : FALSE,
            damage, status_id, fp->hitlag_mul);
        if ((s32)(((f32)fp->damage_queue * 0.75F) + 4.0F) > 0)
        {
            ftParamMakeRumble(
                fp, 0, (s32)(((f32)fp->damage_queue * 0.75F) + 4.0F));
        }
    }
    else if (fp->shield_damage != 0)
    {
        damage = fp->shield_damage;
        if (is_shieldbreak != FALSE)
        {
            ftCommonShieldBreakFlyCommonSetStatus(fighter_gobj);
        }
        else
        {
            ndsBaseFTCommonGuardSetOffSetStatus(fighter_gobj);
        }
    }
    else if (fp->attack_shield_push != 0)
    {
        if (fp->proc_shield != NULL)
        {
            fp->proc_shield(fighter_gobj);
        }
        if ((fp->attack_rebound != 0.0F) && (fp->catch_gobj == NULL) &&
            (fp->capture_gobj == NULL) &&
            (ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE))
        {
            u32 saved_rebound_wait_count =
                gNdsStageMPPassiveLoopReboundWaitSetStatusCount;
            sb32 saved_rebound_active = sNdsStageMPPassiveLoopReboundActive;

            ftParamStopVoiceRunProcDamage(fighter_gobj);
            /* ponytail: use the existing proof gate until common status routing is broader. */
            sNdsStageMPPassiveLoopReboundActive = TRUE;
            ndsBaseFTCommonReboundWaitSetStatus(fighter_gobj);
            sNdsStageMPPassiveLoopReboundActive = saved_rebound_active;
            gNdsStageMPPassiveLoopReboundWaitSetStatusCount =
                saved_rebound_wait_count;
        }
        damage = fp->attack_shield_push;
    }
    else if (fp->attack_damage != 0)
    {
        if (fp->proc_hit != NULL)
        {
            fp->proc_hit(fighter_gobj);
        }
        damage = fp->attack_damage;
        if (fp->stat_flags.attack_id == nFTStatusAttackIDBatSwing4)
        {
            ftParamMakeRumble(fp, 10, 0);
        }
        else if ((s32)(((f32)fp->attack_damage * 0.5F) + 2.0F) > 0)
        {
            ftParamMakeRumble(
                fp, 5, (s32)(((f32)fp->attack_damage * 0.5F) + 2.0F));
        }
    }
    else if (fp->reflect_damage != 0)
    {
        ftCommonShieldBreakFlyReflectorSetStatus(fighter_gobj);
    }
    else if ((fp->reflect_lr != 0) && (fp->special_coll != NULL))
    {
        switch (fp->special_coll->kind)
        {
        case nFTSpecialCollKindFoxReflector:
            ftFoxSpecialLwHitSetStatus(fighter_gobj);
            break;

        case nFTSpecialCollKindNessReflector:
            (void)func_800269C0_275C0(nSYAudioFGMBatHit);
            break;
        }
    }
    else if (fp->absorb_lr != 0)
    {
        ftNessSpecialLwProcAbsorb(fighter_gobj);
    }

    if (damage != 0)
    {
        fp->hitlag_tics = ftParamGetHitLag(damage, status_id,
                                           fp->hitlag_mul);
        if ((fp->hitlag_tics != 0) && (is_knockback_paused != FALSE))
        {
            fp->is_knockback_paused = TRUE;
        }
        fp->input.pl.button_tap = 0;
        fp->input.pl.button_release = 0;
        if (fp->proc_lagstart != NULL)
        {
            fp->proc_lagstart(fighter_gobj);
        }
        else if (proc_lagstart != NULL)
        {
            proc_lagstart(fighter_gobj);
        }
    }

    fp->attack_damage = 0;
    fp->attack_shield_push = 0;
    fp->shield_damage = 0;
    fp->shield_damage_total = 0;
    fp->damage_lag = 0;
    fp->damage_queue = 0;
    fp->damage_kind = nFTDamageKindDefault;
    fp->reflect_lr = 0;
    fp->reflect_damage = 0;
    fp->absorb_lr = 0;
    fp->attack_rebound = 0.0F;
    fp->damage_knockback = 0.0F;
    fp->hitlag_mul = 1.0F;

    (void)hitlag_tics;
}

static const s32 sNdsFTCommonDamageStatusGroundIDs[4][3] = {
    { nFTCommonStatusDamageLw1,   nFTCommonStatusDamageN1,
      nFTCommonStatusDamageHi1 },
    { nFTCommonStatusDamageLw2,   nFTCommonStatusDamageN2,
      nFTCommonStatusDamageHi2 },
    { nFTCommonStatusDamageLw3,   nFTCommonStatusDamageN3,
      nFTCommonStatusDamageHi3 },
    { nFTCommonStatusDamageFlyLw, nFTCommonStatusDamageFlyN,
      nFTCommonStatusDamageFlyHi }
};
static const s32 sNdsFTCommonDamageStatusAirIDs[4][3] = {
    { nFTCommonStatusDamageAir1,  nFTCommonStatusDamageAir1,
      nFTCommonStatusDamageAir1 },
    { nFTCommonStatusDamageAir2,  nFTCommonStatusDamageAir2,
      nFTCommonStatusDamageAir2 },
    { nFTCommonStatusDamageAir3,  nFTCommonStatusDamageAir3,
      nFTCommonStatusDamageAir3 },
    { nFTCommonStatusDamageFlyLw, nFTCommonStatusDamageFlyN,
      nFTCommonStatusDamageFlyHi }
};

static s32 ndsFTCommonDamageGetDamageLevel(f32 hitstun)
{
    if (hitstun < 12.0F)
    {
        return 0;
    }
    if (hitstun < 24.0F)
    {
        return 1;
    }
    if (hitstun < 32.0F)
    {
        return 2;
    }
    return 3;
}

static s32 ndsFTCommonDamageSelectStatus(s32 damage_level, s32 damage_index,
                                         sb32 is_air)
{
    if (damage_level < 0)
    {
        damage_level = 0;
    }
    if (damage_level >= 4)
    {
        damage_level = 3;
    }
    if (damage_index < 0)
    {
        damage_index = 0;
    }
    if (damage_index >= 3)
    {
        damage_index = 2;
    }
    return (is_air != FALSE) ?
        sNdsFTCommonDamageStatusAirIDs[damage_level][damage_index] :
        sNdsFTCommonDamageStatusGroundIDs[damage_level][damage_index];
}

static s32 ndsFTCommonDamageMotionForStatus(s32 status_id)
{
    if ((status_id >= nFTCommonStatusDamageHi1) &&
        (status_id <= nFTCommonStatusDamageAir3))
    {
        return nFTCommonMotionDamageHi1 +
               (status_id - nFTCommonStatusDamageHi1);
    }
    if ((status_id == nFTCommonStatusDamageE1) ||
        (status_id == nFTCommonStatusDamageE2))
    {
        return nFTCommonMotionDamageE;
    }
    if ((status_id >= nFTCommonStatusDamageFlyHi) &&
        (status_id <= nFTCommonStatusDamageFlyRoll))
    {
        return nFTCommonMotionDamageFlyHi +
               (status_id - nFTCommonStatusDamageFlyHi);
    }
    if (status_id == nFTCommonStatusWallDamage)
    {
        return nFTCommonMotionWallDamage;
    }
    return -1;
}

static sb32 ndsFTCommonDamageIsStatus(s32 status_id)
{
    return ((status_id >= nFTCommonStatusDamageStart) &&
            (status_id <= nFTCommonStatusWallDamage)) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageLevels(void)
{
    u32 mask = 0u;

    if (ftCommonDamageGetDamageLevel(0.0F) == 0)
    {
        mask |= NDS_DAMAGE_LEVEL_LOW;
    }
    if (ftCommonDamageGetDamageLevel(12.0F) == 1)
    {
        mask |= NDS_DAMAGE_LEVEL_MID;
    }
    if (ftCommonDamageGetDamageLevel(24.0F) == 2)
    {
        mask |= NDS_DAMAGE_LEVEL_HIGH;
    }
    if (ftCommonDamageGetDamageLevel(32.0F) == 3)
    {
        mask |= NDS_DAMAGE_LEVEL_FLY;
    }
    if ((mask & (NDS_DAMAGE_LEVEL_LOW | NDS_DAMAGE_LEVEL_MID |
                 NDS_DAMAGE_LEVEL_HIGH | NDS_DAMAGE_LEVEL_FLY)) ==
        (NDS_DAMAGE_LEVEL_LOW | NDS_DAMAGE_LEVEL_MID |
         NDS_DAMAGE_LEVEL_HIGH | NDS_DAMAGE_LEVEL_FLY))
    {
        mask |= NDS_DAMAGE_LEVEL_ORIGINAL;
    }
    gNdsFighterDashRunDamageLevelMask = mask;
    return ((mask & 0x1fu) == 0x1fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageStatusSelect(FTStruct *fp,
                                                     s32 status_before)
{
    s32 damage_level;
    s32 damage_index;
    s32 ground_status;
    s32 air_status;
    s32 electric_status;
    u32 mask = 0u;

    if ((fp == NULL) || (fp->damage_knockback == 0.0F))
    {
        return FALSE;
    }

    damage_level =
        ndsFTCommonDamageGetDamageLevel(ftParamGetHitStun(fp->damage_knockback));
    damage_index = fp->damage_index;
    if ((damage_level < 0) || (damage_level >= 4) ||
        (damage_index < 0) || (damage_index >= 3))
    {
        return FALSE;
    }

    ground_status =
        sNdsFTCommonDamageStatusGroundIDs[damage_level][damage_index];
    air_status = sNdsFTCommonDamageStatusAirIDs[damage_level][damage_index];
    electric_status = (damage_level == 3) ?
        nFTCommonStatusDamageE2 : nFTCommonStatusDamageE1;

    mask |= NDS_DAMAGE_STATUS_SELECT_LEVEL;
    if (ground_status ==
        sNdsFTCommonDamageStatusGroundIDs[damage_level][damage_index])
    {
        mask |= NDS_DAMAGE_STATUS_SELECT_GROUND;
    }
    if (air_status ==
        sNdsFTCommonDamageStatusAirIDs[damage_level][damage_index])
    {
        mask |= NDS_DAMAGE_STATUS_SELECT_AIR;
    }
    if (((electric_status == nFTCommonStatusDamageE1) ||
         (electric_status == nFTCommonStatusDamageE2)) &&
        (electric_status == ((damage_level == 3) ?
            nFTCommonStatusDamageE2 : nFTCommonStatusDamageE1)))
    {
        mask |= NDS_DAMAGE_STATUS_SELECT_ELECTRIC;
    }
    if (fp->status_id == status_before)
    {
        mask |= NDS_DAMAGE_STATUS_SELECT_PARKED;
    }

    gNdsFighterDashRunDamageStatusMask = mask;
    gNdsFighterDashRunDamageStatusLevel = (u32)damage_level;
    gNdsFighterDashRunDamageStatusIndex = (u32)damage_index;
    gNdsFighterDashRunDamageStatusGround = (u32)ground_status;
    gNdsFighterDashRunDamageStatusAir = (u32)air_status;
    gNdsFighterDashRunDamageStatusElectric = (u32)electric_status;

    return ((mask & 0x1fu) == 0x1fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageKnockbackAngle(void)
{
    f32 fixed = ftCommonDamageGetKnockbackAngle(
        45, nMPKineticsGround, 10.0F);
    f32 air_361 = ftCommonDamageGetKnockbackAngle(
        361, nMPKineticsAir, 10.0F);
    f32 ground_low = ftCommonDamageGetKnockbackAngle(
        361, nMPKineticsGround, 10.0F);
    f32 ground_high = ftCommonDamageGetKnockbackAngle(
        361, nMPKineticsGround, 32.05F);
    f32 ground_cap = ftCommonDamageGetKnockbackAngle(
        361, nMPKineticsGround, 100.0F);
    u32 mask = 0u;

    if (ndsFloatToMilliSigned(fixed) ==
        ndsFloatToMilliSigned(F_CLC_DTOR32(45.0F)))
    {
        mask |= NDS_DAMAGE_KNOCKBACK_ANGLE_FIXED;
    }
    if (ndsFloatToMilliSigned(air_361) ==
        ndsFloatToMilliSigned(F_CLC_DTOR32(43.0F)))
    {
        mask |= NDS_DAMAGE_KNOCKBACK_ANGLE_AIR_361;
    }
    if (ndsFloatToMilliSigned(ground_low) == 0)
    {
        mask |= NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_LOW_361;
    }
    if ((ground_high > 0.0F) &&
        (ground_high < F_CLC_DTOR32(42.5F)))
    {
        mask |= NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_HIGH_361;
    }
    if (ndsFloatToMilliSigned(ground_cap) ==
        ndsFloatToMilliSigned(F_CLC_DTOR32(42.5F)))
    {
        mask |= NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_CAP_361;
    }
    if ((mask & (NDS_DAMAGE_KNOCKBACK_ANGLE_FIXED |
                 NDS_DAMAGE_KNOCKBACK_ANGLE_AIR_361 |
                 NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_LOW_361 |
                 NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_HIGH_361 |
                 NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_CAP_361)) ==
        (NDS_DAMAGE_KNOCKBACK_ANGLE_FIXED |
         NDS_DAMAGE_KNOCKBACK_ANGLE_AIR_361 |
         NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_LOW_361 |
         NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_HIGH_361 |
         NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_CAP_361))
    {
        mask |= NDS_DAMAGE_KNOCKBACK_ANGLE_ORIGINAL;
    }

    gNdsFighterDashRunDamageKnockbackAngleMask = mask;

    return ((mask & 0x3fu) == 0x3fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageCommonPhysics(GObj *fighter_gobj,
                                                      FTStruct *fp)
{
    FTStruct saved_fp;
    FTAttributes attr;
    f32 saved_anim_frame;
    s32 ground_after = 0;
    s32 air_friction_after = 0;
    s32 air_drift_after = 0;
    u32 clear_state_after = nGMAttackStateNew;
    u32 mask = 0u;

    if ((fighter_gobj == NULL) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE) || (fp->attr == NULL))
    {
        return FALSE;
    }

    saved_fp = *fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    attr = *fp->attr;
    attr.traction = 1.0F;
    attr.air_friction = 0.5F;
    attr.air_accel = 0.0F;
    attr.air_speed_max_x = 40.0F;
    attr.gravity = 1.0F;
    attr.tvel_base = 10.0F;
    attr.tvel_fast = 20.0F;

    fp->attr = &attr;
    fp->proc_physics = ftCommonDamageCommonProcPhysics;
    fp->ga = nMPKineticsGround;
    fp->status_id = nFTCommonStatusDamageN1;
    fp->status_vars.common.damage.hitstun_tics = 1;
    fp->throw_gobj = NULL;
    fp->coll_data.floor_flags = 0u;
    fp->physics.vel_ground.x = 12.0F;
    fp->physics.vel_ground.y = 0.0F;
    fp->physics.vel_ground.z = 0.0F;
    fp->vel_ground = fp->physics.vel_ground;
    sNdsFighterDashRunDamagePhysicsActive = TRUE;
    ftCommonDamageCommonProcPhysics(fighter_gobj);
    sNdsFighterDashRunDamagePhysicsActive = FALSE;
    ground_after = ndsFloatToMilliSigned(fp->physics.vel_ground.x);
    if ((ground_after != 0) && (ABS(ground_after) < 12000))
    {
        mask |= NDS_DAMAGE_COMMON_PHYSICS_GROUND;
    }

    *fp = saved_fp;
    fp->attr = &attr;
    fp->proc_physics = ftCommonDamageCommonProcPhysics;
    fp->ga = nMPKineticsAir;
    fp->status_id = nFTCommonStatusDamageAir1;
    fp->status_vars.common.damage.hitstun_tics = 2;
    fp->throw_gobj = NULL;
    fp->physics.vel_air.x = 12.0F;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->vel_air = fp->physics.vel_air;
    ftCommonDamageCommonProcPhysics(fighter_gobj);
    air_friction_after = ndsFloatToMilliSigned(fp->physics.vel_air.x);
    if ((air_friction_after != 0) &&
        (ABS(air_friction_after) < 12000))
    {
        mask |= NDS_DAMAGE_COMMON_PHYSICS_AIR_FRICTION;
    }

    *fp = saved_fp;
    fp->attr = &attr;
    fp->proc_physics = ftCommonDamageCommonProcPhysics;
    fp->ga = nMPKineticsAir;
    fp->status_id = nFTCommonStatusDamageAir1;
    fp->status_vars.common.damage.hitstun_tics = 0;
    fp->throw_gobj = NULL;
    fp->is_fastfall = FALSE;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->vel_air = fp->physics.vel_air;
    ftCommonDamageCommonProcPhysics(fighter_gobj);
    air_drift_after = ndsFloatToMilliSigned(fp->physics.vel_air.y);
    if (air_drift_after < 0)
    {
        mask |= NDS_DAMAGE_COMMON_PHYSICS_AIR_DRIFT;
    }

    *fp = saved_fp;
    fp->attr = &attr;
    fp->proc_physics = ftCommonDamageCommonProcPhysics;
    fp->ga = nMPKineticsAir;
    fp->status_id = nFTCommonStatusDamageAir1;
    fp->status_vars.common.damage.hitstun_tics = 1;
    fp->throw_gobj = fighter_gobj;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->physics.vel_damage_air.x = 10.0F;
    fp->physics.vel_damage_air.y = 0.0F;
    fp->physics.vel_damage_air.z = 0.0F;
    fp->attack_colls[0].attack_state = nGMAttackStateNew;
    fp->attack_colls[0].damage = 7;
    ftCommonDamageCommonProcPhysics(fighter_gobj);
    clear_state_after = fp->attack_colls[0].attack_state;
    if ((clear_state_after == nGMAttackStateOff) &&
        (fp->attack_colls[0].damage == 0))
    {
        mask |= NDS_DAMAGE_COMMON_PHYSICS_CLEAR_ATTACK;
    }

    *fp = saved_fp;
    fighter_gobj->anim_frame = saved_anim_frame;
    sNdsFighterDashRunDamagePhysicsActive = FALSE;
    if ((fp->status_id == saved_fp.status_id) &&
        (fp->proc_physics == saved_fp.proc_physics) &&
        (fp->attr == saved_fp.attr))
    {
        mask |= NDS_DAMAGE_COMMON_PHYSICS_RESTORE;
    }
    if ((mask & (NDS_DAMAGE_COMMON_PHYSICS_GROUND |
                 NDS_DAMAGE_COMMON_PHYSICS_AIR_FRICTION |
                 NDS_DAMAGE_COMMON_PHYSICS_AIR_DRIFT |
                 NDS_DAMAGE_COMMON_PHYSICS_CLEAR_ATTACK |
                 NDS_DAMAGE_COMMON_PHYSICS_RESTORE)) ==
        (NDS_DAMAGE_COMMON_PHYSICS_GROUND |
         NDS_DAMAGE_COMMON_PHYSICS_AIR_FRICTION |
         NDS_DAMAGE_COMMON_PHYSICS_AIR_DRIFT |
         NDS_DAMAGE_COMMON_PHYSICS_CLEAR_ATTACK |
         NDS_DAMAGE_COMMON_PHYSICS_RESTORE))
    {
        mask |= NDS_DAMAGE_COMMON_PHYSICS_ORIGINAL;
    }

    gNdsFighterDashRunDamageCommonPhysicsMask = mask;
    gNdsFighterDashRunDamageCommonPhysicsGroundMilli = ground_after;
    gNdsFighterDashRunDamageCommonPhysicsAirFrictionXMilli =
        air_friction_after;
    gNdsFighterDashRunDamageCommonPhysicsAirDriftYMilli = air_drift_after;
    gNdsFighterDashRunDamageCommonPhysicsClearState = clear_state_after;

    return ((mask & 0x3fu) == 0x3fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageCommonCallbacks(GObj *fighter_gobj,
                                                        FTStruct *fp)
{
    FTStruct saved_fp;
    f32 saved_anim_frame;
    sb32 saved_wait_interrupt_active;
    sb32 saved_interrupt_active;
    sb32 saved_expiry_active;
    sb32 saved_hammer_check_active;
    sb32 saved_hammer_hold;
    u32 saved_wait_interrupt_count;
    u32 saved_ground_check_count;
    u32 saved_fall_interrupt_count;
    u32 saved_fall_set_status_count;
    u32 saved_common_fall_interrupt_count;
    u32 saved_hammer_check_count;
    u32 saved_hammer_ground_count;
    u32 saved_hammer_air_count;
    u32 mask = 0u;

    if ((fighter_gobj == NULL) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        return FALSE;
    }

    saved_fp = *fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    saved_wait_interrupt_active = sNdsFighterDashRunWaitInterruptActive;
    saved_interrupt_active = sNdsFighterDashRunDamageInterruptActive;
    saved_expiry_active = sNdsFighterDashRunDamageExpiryActive;
    saved_hammer_check_active = sNdsFighterDashRunDamageHammerCheckActive;
    saved_hammer_hold = sNdsFighterDashRunDamageHammerHold;
    saved_wait_interrupt_count = gNdsFighterDashRunWaitInterruptCallCount;
    saved_ground_check_count = gNdsFighterDashRunGroundCheckCallCount;
    saved_fall_interrupt_count = sNdsFighterDashRunDamageFallInterruptCount;
    saved_fall_set_status_count =
        sNdsFighterDashRunDamageFallSetStatusCount;
    saved_common_fall_interrupt_count =
        sNdsFighterDashRunDamageCommonFallInterruptCount;
    saved_hammer_check_count = sNdsFighterDashRunDamageHammerCheckCount;
    saved_hammer_ground_count = sNdsFighterDashRunDamageHammerGroundCount;
    saved_hammer_air_count = sNdsFighterDashRunDamageHammerAirCount;

    gNdsFighterDashRunDamageCommonCallbackMask = 0u;

    *fp = saved_fp;
    fp->ga = nMPKineticsGround;
    fp->status_id = nFTCommonStatusDamageN1;
    fp->motion_id = ndsFTCommonDamageMotionForStatus(fp->status_id);
    fp->status_vars.common.damage.hitstun_tics = 2;
    fp->status_vars.common.damage.public_knockback = 19.0F;
    fp->public_knockback = 0.0F;
    fighter_gobj->anim_frame = 0.0F;
    ftCommonDamageCommonProcUpdate(fighter_gobj);
    if ((fp->status_id == nFTCommonStatusDamageN1) &&
        (fp->motion_id == ndsFTCommonDamageMotionForStatus(fp->status_id)) &&
        (fp->ga == nMPKineticsGround) &&
        (fp->status_vars.common.damage.hitstun_tics == 1) &&
        (fp->public_knockback == 0.0F))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_GROUND_STAY;
    }

    *fp = saved_fp;
    fp->ga = nMPKineticsGround;
    fp->status_id = nFTCommonStatusDamageN1;
    fp->motion_id = ndsFTCommonDamageMotionForStatus(fp->status_id);
    fp->status_vars.common.damage.hitstun_tics = 1;
    fp->status_vars.common.damage.public_knockback = 17.0F;
    fp->public_knockback = 0.0F;
    fighter_gobj->anim_frame = 0.0F;
    ftCommonDamageCommonProcUpdate(fighter_gobj);
    if ((fp->status_id == nFTCommonStatusWait) &&
        (fp->motion_id == nFTCommonMotionWait) &&
        (fp->ga == nMPKineticsGround) &&
        (fp->status_vars.common.damage.hitstun_tics == 0) &&
        (ndsFloatToMilliSigned(fp->public_knockback) == 17000))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_GROUND_UPDATE;
    }
    if ((mask & (NDS_DAMAGE_COMMON_CALLBACK_GROUND_STAY |
                 NDS_DAMAGE_COMMON_CALLBACK_GROUND_UPDATE)) ==
        (NDS_DAMAGE_COMMON_CALLBACK_GROUND_STAY |
         NDS_DAMAGE_COMMON_CALLBACK_GROUND_UPDATE))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_GROUND_UPDATE_ORIGINAL;
    }

    *fp = saved_fp;
    fp->ga = nMPKineticsAir;
    fp->status_id = nFTCommonStatusDamageAir1;
    fp->motion_id = ndsFTCommonDamageMotionForStatus(fp->status_id);
    fp->status_vars.common.damage.hitstun_tics = 1;
    fp->status_vars.common.damage.dust_effect_int = 0;
    fighter_gobj->anim_frame = 0.0F;
    sNdsFighterDashRunDamageFallSetStatusCount = 0u;
    sNdsFighterDashRunDamageExpiryActive = TRUE;
    ftCommonDamageAirCommonProcUpdate(fighter_gobj);
    sNdsFighterDashRunDamageExpiryActive = FALSE;
    if ((fp->status_id == nFTCommonStatusDamageFall) &&
        (fp->motion_id == nFTCommonMotionDamageFall) &&
        (fp->ga == nMPKineticsAir) &&
        (sNdsFighterDashRunDamageFallSetStatusCount == 1u))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE;
    }

    *fp = saved_fp;
    fp->ga = nMPKineticsAir;
    fp->status_id = nFTCommonStatusDamageAir1;
    fp->motion_id = ndsFTCommonDamageMotionForStatus(fp->status_id);
    fp->status_vars.common.damage.hitstun_tics = 0;
    fp->status_vars.common.damage.dust_effect_int = 0;
    fighter_gobj->anim_frame = 5.0F;
    sNdsFighterDashRunDamageFallSetStatusCount = 0u;
    sNdsFighterDashRunDamageExpiryActive = TRUE;
    ftCommonDamageAirCommonProcUpdate(fighter_gobj);
    sNdsFighterDashRunDamageExpiryActive = FALSE;
    if ((fp->status_id == nFTCommonStatusDamageAir1) &&
        (fp->motion_id == ndsFTCommonDamageMotionForStatus(fp->status_id)) &&
        (fp->ga == nMPKineticsAir) &&
        (fp->status_vars.common.damage.hitstun_tics == 0) &&
        (sNdsFighterDashRunDamageFallSetStatusCount == 0u))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_AIR_STAY;
    }
    if ((mask & (NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE |
                 NDS_DAMAGE_COMMON_CALLBACK_AIR_STAY)) ==
        (NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE |
         NDS_DAMAGE_COMMON_CALLBACK_AIR_STAY))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE_ORIGINAL;
    }

    *fp = saved_fp;
    fp->ga = nMPKineticsGround;
    fp->status_id = nFTCommonStatusDamageN1;
    fp->status_vars.common.damage.hitstun_tics = 0;
    fp->is_hitstun = TRUE;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    sNdsFighterDashRunWaitInterruptActive = TRUE;
    ftCommonDamageCommonProcInterrupt(fighter_gobj);
    sNdsFighterDashRunWaitInterruptActive = FALSE;
    if ((fp->is_hitstun == FALSE) &&
        (gNdsFighterDashRunWaitInterruptCallCount ==
            (saved_wait_interrupt_count + 1u)) &&
        (gNdsFighterDashRunGroundCheckCallCount ==
            (saved_ground_check_count + 1u)))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_GROUND_INTERRUPT;
    }

    *fp = saved_fp;
    fp->ga = nMPKineticsAir;
    fp->status_id = nFTCommonStatusDamageAir1;
    fp->status_vars.common.damage.hitstun_tics = 0;
    fp->is_hitstun = TRUE;
    sNdsFighterDashRunDamageFallInterruptCount = 0u;
    sNdsFighterDashRunDamageInterruptActive = TRUE;
    ftCommonDamageAirCommonProcInterrupt(fighter_gobj);
    sNdsFighterDashRunDamageInterruptActive = FALSE;
    if ((fp->is_hitstun == FALSE) &&
        (sNdsFighterDashRunDamageFallInterruptCount == 1u))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_AIR_INTERRUPT;
    }
    if (mask & NDS_DAMAGE_COMMON_CALLBACK_AIR_INTERRUPT)
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_AIR_INTERRUPT_ORIGINAL;
    }

    *fp = saved_fp;
    fp->ga = nMPKineticsAir;
    fp->status_id = nFTCommonStatusDamageN1;
    fp->motion_id = ndsFTCommonDamageMotionForStatus(fp->status_id);
    fp->status_vars.common.damage.hitstun_tics = 0;
    fp->is_hitstun = TRUE;
    sNdsFighterDashRunDamageCommonFallInterruptCount = 0u;
    sNdsFighterDashRunDamageInterruptActive = TRUE;
    ftCommonDamageCommonProcInterrupt(fighter_gobj);
    sNdsFighterDashRunDamageInterruptActive = FALSE;
    if ((fp->is_hitstun == FALSE) &&
        (sNdsFighterDashRunDamageCommonFallInterruptCount == 1u))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_FALL_INTERRUPT;
    }

    *fp = saved_fp;
    fp->ga = nMPKineticsGround;
    fp->status_vars.common.damage.hitstun_tics = 0;
    fp->is_hitstun = TRUE;
    sNdsFighterDashRunDamageHammerCheckCount = 0u;
    sNdsFighterDashRunDamageHammerGroundCount = 0u;
    sNdsFighterDashRunDamageHammerAirCount = 0u;
    sNdsFighterDashRunDamageHammerHold = TRUE;
    sNdsFighterDashRunDamageHammerCheckActive = TRUE;
    ftCommonDamageCommonProcInterrupt(fighter_gobj);
    if ((fp->is_hitstun == FALSE) &&
        (sNdsFighterDashRunDamageHammerCheckCount == 1u) &&
        (sNdsFighterDashRunDamageHammerGroundCount == 1u))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_HAMMER_GROUND;
    }

    *fp = saved_fp;
    fp->ga = nMPKineticsAir;
    fp->status_vars.common.damage.hitstun_tics = 0;
    fp->is_hitstun = TRUE;
    ftCommonDamageCommonProcInterrupt(fighter_gobj);
    sNdsFighterDashRunDamageHammerCheckActive = FALSE;
    sNdsFighterDashRunDamageHammerHold = FALSE;
    if ((fp->is_hitstun == FALSE) &&
        (sNdsFighterDashRunDamageHammerCheckCount == 2u) &&
        (sNdsFighterDashRunDamageHammerAirCount == 1u))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_HAMMER_AIR;
    }
    if ((mask & (NDS_DAMAGE_COMMON_CALLBACK_GROUND_INTERRUPT |
                 NDS_DAMAGE_COMMON_CALLBACK_FALL_INTERRUPT |
                 NDS_DAMAGE_COMMON_CALLBACK_HAMMER_GROUND |
                 NDS_DAMAGE_COMMON_CALLBACK_HAMMER_AIR)) ==
        (NDS_DAMAGE_COMMON_CALLBACK_GROUND_INTERRUPT |
         NDS_DAMAGE_COMMON_CALLBACK_FALL_INTERRUPT |
         NDS_DAMAGE_COMMON_CALLBACK_HAMMER_GROUND |
         NDS_DAMAGE_COMMON_CALLBACK_HAMMER_AIR))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_COMMON_INTERRUPT_ORIGINAL;
    }

    *fp = saved_fp;
    fighter_gobj->anim_frame = saved_anim_frame;
    sNdsFighterDashRunWaitInterruptActive = saved_wait_interrupt_active;
    sNdsFighterDashRunDamageInterruptActive = saved_interrupt_active;
    sNdsFighterDashRunDamageExpiryActive = saved_expiry_active;
    sNdsFighterDashRunDamageHammerCheckActive =
        saved_hammer_check_active;
    sNdsFighterDashRunDamageHammerHold = saved_hammer_hold;
    gNdsFighterDashRunWaitInterruptCallCount =
        saved_wait_interrupt_count;
    gNdsFighterDashRunGroundCheckCallCount = saved_ground_check_count;
    sNdsFighterDashRunDamageFallInterruptCount =
        saved_fall_interrupt_count;
    sNdsFighterDashRunDamageFallSetStatusCount =
        saved_fall_set_status_count;
    sNdsFighterDashRunDamageCommonFallInterruptCount =
        saved_common_fall_interrupt_count;
    sNdsFighterDashRunDamageHammerCheckCount = saved_hammer_check_count;
    sNdsFighterDashRunDamageHammerGroundCount =
        saved_hammer_ground_count;
    sNdsFighterDashRunDamageHammerAirCount = saved_hammer_air_count;
    if ((fp->status_id == saved_fp.status_id) &&
        (fp->motion_id == saved_fp.motion_id) &&
        (fp->ga == saved_fp.ga) &&
        (fighter_gobj->anim_frame == saved_anim_frame))
    {
        mask |= NDS_DAMAGE_COMMON_CALLBACK_RESTORE;
    }

    gNdsFighterDashRunDamageCommonCallbackMask = mask;

    return ((mask & 0x3fffu) == 0x3fffu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageHoldResist(FTStruct *fp)
{
    FTStruct saved_fp;
    u32 mask = 0u;

    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        return FALSE;
    }

    saved_fp = *fp;
    gNdsFighterDashRunDamageHoldResistMask = 0u;

    fp->damage_element = nGMHitElementSleep;
    fp->damage_knockback = 0.0F;
    fp->hitlag_tics = 2;
    fp->is_knockback_paused = TRUE;
    if (ftCommonDamageCheckCatchResist(fp) == FALSE)
    {
        mask |= NDS_DAMAGE_HOLD_RESIST_SLEEP_FALSE;
    }

    fp->damage_element = nGMHitElementNormal;
    fp->damage_knockback = 0.0F;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    if (ftCommonDamageCheckCatchResist(fp) != FALSE)
    {
        mask |= NDS_DAMAGE_HOLD_RESIST_ZERO_TRUE;
    }

    fp->damage_knockback = 25.0F;
    fp->damage_knockback_stack = 100.0F;
    fp->hitlag_tics = 2;
    fp->is_knockback_paused = TRUE;
    if (ftCommonDamageCheckCatchResist(fp) != FALSE)
    {
        mask |= NDS_DAMAGE_HOLD_RESIST_PAUSED_TRUE;
    }

    fp->fkind = nFTKindDonkey;
    fp->status_id = nFTCommonStatusSpecialStart + 15;
    fp->damage_knockback = 20.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    if (ftCommonDamageCheckCatchResist(fp) != FALSE)
    {
        mask |= NDS_DAMAGE_HOLD_RESIST_DONKEY_TRUE;
    }

    fp->fkind = nFTKindMario;
    fp->status_id = nFTCommonStatusWait;
    fp->damage_knockback = 90.0F;
    if (ftCommonDamageCheckCatchResist(fp) == FALSE)
    {
        mask |= NDS_DAMAGE_HOLD_RESIST_DEFAULT_FALSE;
    }

    fp->damage_queue = FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD - 1;
    if (ftCommonDamageCheckCaptureKeepHold(fp) != FALSE)
    {
        mask |= NDS_DAMAGE_HOLD_RESIST_KEEP_TRUE;
    }
    fp->damage_queue = FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD;
    if (ftCommonDamageCheckCaptureKeepHold(fp) == FALSE)
    {
        mask |= NDS_DAMAGE_HOLD_RESIST_KEEP_FALSE;
    }

    *fp = saved_fp;
    if ((fp->fkind == saved_fp.fkind) &&
        (fp->status_id == saved_fp.status_id) &&
        (fp->damage_element == saved_fp.damage_element) &&
        (fp->damage_knockback == saved_fp.damage_knockback) &&
        (fp->damage_queue == saved_fp.damage_queue))
    {
        mask |= NDS_DAMAGE_HOLD_RESIST_RESTORE;
    }

    gNdsFighterDashRunDamageHoldResistMask = mask;
    return ((mask & 0xffu) == 0xffu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateCatchResist(GObj *fighter_gobj,
                                                          FTStruct *fp)
{
    DObj *root;
    FTStruct saved_fp;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    u32 colanim_before;
    u32 run_update_before;
    s32 status_before;
    s32 status_after;
    u32 mask = 0u;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }

    root = DObjGetStruct(fighter_gobj);
    saved_fp = *fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    if (root != NULL)
    {
        saved_anim_speed = root->anim_speed;
    }

    gNdsFighterDashRunDamageUpdateCatchResistMask = 0u;

    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->item_gobj = NULL;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_knockback = 0.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    colanim_before = sNdsFighterDashRunDamageSetupColAnimCount;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    if ((fp->damage_knockback == 0.0F) ||
        ((fp->hitlag_tics > 0) && (fp->is_knockback_paused != FALSE) &&
         (fp->damage_knockback < (fp->damage_knockback_stack + 30.0F))))
    {
        (void)ndsFTCommonDamageCheckElementSetColAnim(
            fighter_gobj, fp->damage_element,
            ndsFTCommonDamageGetDamageLevel(
                ftParamGetHitStun(fp->damage_knockback)));
    }
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    if (sNdsFighterDashRunDamageSetupColAnimCount > colanim_before)
    {
        mask |= NDS_DAMAGE_UPDATE_CATCH_RESIST_ZERO_COLANIM;
    }

    *fp = saved_fp;
    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->item_gobj = NULL;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_knockback = 0.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    run_update_before = sNdsFighterDashRunDamageRunUpdateColAnimCount;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateCatchResist(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    if ((sNdsFighterDashRunDamageRunUpdateColAnimCount > run_update_before) &&
        (fp->status_id == saved_fp.status_id))
    {
        mask |= NDS_DAMAGE_UPDATE_CATCH_RESIST_ORIGINAL;
    }

    *fp = saved_fp;
    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->item_gobj = NULL;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_knockback = 25.0F;
    fp->damage_knockback_stack = 100.0F;
    fp->hitlag_tics = 2;
    fp->is_knockback_paused = TRUE;
    colanim_before = sNdsFighterDashRunDamageSetupColAnimCount;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    if ((fp->damage_knockback == 0.0F) ||
        ((fp->hitlag_tics > 0) && (fp->is_knockback_paused != FALSE) &&
         (fp->damage_knockback < (fp->damage_knockback_stack + 30.0F))))
    {
        (void)ndsFTCommonDamageCheckElementSetColAnim(
            fighter_gobj, fp->damage_element,
            ndsFTCommonDamageGetDamageLevel(
                ftParamGetHitStun(fp->damage_knockback)));
    }
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    if (sNdsFighterDashRunDamageSetupColAnimCount > colanim_before)
    {
        mask |= NDS_DAMAGE_UPDATE_CATCH_RESIST_PAUSED_COLANIM;
    }

    *fp = saved_fp;
    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->item_gobj = NULL;
    fp->is_catch_or_capture = FALSE;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_angle.x = 0.0F;
    fp->coll_data.floor_angle.y = 1.0F;
    fp->coll_data.floor_angle.z = 0.0F;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_queue = 8;
    fp->damage_knockback = 90.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->damage_angle = 60;
    fp->damage_lr = 1;
    fp->damage_index = 1;
    fp->damage_player_num = 1;
    fp->damage_lag = 7;
    fp->hitlag_mul = 1.0F;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    status_before = fp->status_id;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    if ((fp->damage_knockback != 0.0F) &&
        ((fp->hitlag_tics <= 0) || (fp->is_knockback_paused == FALSE) ||
         (fp->damage_knockback >= (fp->damage_knockback_stack + 30.0F))))
    {
        ftParamStopVoiceRunProcDamage(fighter_gobj);
        ftCommonDamageGotoDamageStatus(fighter_gobj);
    }
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    status_after = fp->status_id;
    if ((status_after != status_before) &&
        (ndsFTCommonDamageIsStatus(status_after) != FALSE) &&
        (fp->is_hitstun != FALSE))
    {
        mask |= NDS_DAMAGE_UPDATE_CATCH_RESIST_STATUS;
    }

    *fp = saved_fp;
    fighter_gobj->anim_frame = saved_anim_frame;
    if (root != NULL)
    {
        root->anim_speed = saved_anim_speed;
    }
    if ((fp->status_id == saved_fp.status_id) &&
        (fp->motion_id == saved_fp.motion_id) &&
        (fp->damage_knockback == saved_fp.damage_knockback) &&
        (fp->hitlag_tics == saved_fp.hitlag_tics))
    {
        mask |= NDS_DAMAGE_UPDATE_CATCH_RESIST_RESTORE;
    }

    gNdsFighterDashRunDamageUpdateCatchResistMask = mask;
    return ((mask & 0x1fu) == 0x1fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageDustIntervals(FTStruct *fp)
{
    FTStruct saved_fp;
    u32 mask = 0u;
    u32 waits = 0u;

    if (fp == NULL)
    {
        return FALSE;
    }

    saved_fp = *fp;

#define NDS_DAMAGE_DUST_CHECK(bit, shift, ground_vel, air_x, expected_wait) \
    do { \
        fp->ga = ((bit) == NDS_DAMAGE_DUST_DEFAULT_AIR) ? \
            nMPKineticsAir : nMPKineticsGround; \
        fp->physics.vel_damage_ground = (ground_vel); \
        fp->physics.vel_damage_air.x = (air_x); \
        fp->physics.vel_damage_air.y = 0.0F; \
        fp->physics.vel_damage_air.z = 0.0F; \
        fp->status_vars.common.damage.dust_effect_int = -1; \
        ftCommonDamageSetDustEffectInterval(fp); \
        if (fp->status_vars.common.damage.dust_effect_int == (expected_wait)) \
        { \
            mask |= (bit); \
            waits |= (((u32)(expected_wait) & 0xfu) << (shift)); \
        } \
    } while (0)

    NDS_DAMAGE_DUST_CHECK(NDS_DAMAGE_DUST_LOW, 0u, 100.0F, 0.0F,
                          FTCOMMON_DAMAGE_EFFECT_WAIT_LOW);
    NDS_DAMAGE_DUST_CHECK(NDS_DAMAGE_DUST_MID_LOW, 4u, -130.0F, 0.0F,
                          FTCOMMON_DAMAGE_EFFECT_WAIT_MID_LOW);
    NDS_DAMAGE_DUST_CHECK(NDS_DAMAGE_DUST_MID, 8u, 175.0F, 0.0F,
                          FTCOMMON_DAMAGE_EFFECT_WAIT_MID);
    NDS_DAMAGE_DUST_CHECK(NDS_DAMAGE_DUST_MID_HIGH, 12u, -250.0F, 0.0F,
                          FTCOMMON_DAMAGE_EFFECT_WAIT_MID_HIGH);
    NDS_DAMAGE_DUST_CHECK(NDS_DAMAGE_DUST_HIGH, 16u, 400.0F, 0.0F,
                          FTCOMMON_DAMAGE_EFFECT_WAIT_HIGH);
    NDS_DAMAGE_DUST_CHECK(NDS_DAMAGE_DUST_DEFAULT_AIR, 20u, 0.0F, 700.0F,
                          FTCOMMON_DAMAGE_EFFECT_WAIT_DEFAULT);

#undef NDS_DAMAGE_DUST_CHECK

    *fp = saved_fp;
    if ((fp->ga == saved_fp.ga) &&
        (fp->status_vars.common.damage.dust_effect_int ==
            saved_fp.status_vars.common.damage.dust_effect_int))
    {
        mask |= NDS_DAMAGE_DUST_RESTORE;
    }
    if ((mask & (NDS_DAMAGE_DUST_LOW | NDS_DAMAGE_DUST_MID_LOW |
                 NDS_DAMAGE_DUST_MID | NDS_DAMAGE_DUST_MID_HIGH |
                 NDS_DAMAGE_DUST_HIGH | NDS_DAMAGE_DUST_DEFAULT_AIR)) ==
        (NDS_DAMAGE_DUST_LOW | NDS_DAMAGE_DUST_MID_LOW |
         NDS_DAMAGE_DUST_MID | NDS_DAMAGE_DUST_MID_HIGH |
         NDS_DAMAGE_DUST_HIGH | NDS_DAMAGE_DUST_DEFAULT_AIR))
    {
        mask |= NDS_DAMAGE_DUST_ORIGINAL;
    }

    gNdsFighterDashRunDamageDustMask = mask;
    gNdsFighterDashRunDamageDustWaits = waits;

    return ((mask & 0xffu) == 0xffu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageDustUpdate(GObj *fighter_gobj,
                                                   FTStruct *fp)
{
    s32 saved_ga;
    s32 saved_dust_effect_int;
    f32 saved_vel_damage_ground;
    Vec3f saved_vel_damage_air;
    sb32 saved_status_setup_active;
    u32 saved_dust_count;
    u32 saved_dust_effect_count;
    u32 effect_count = 0u;
    s32 wait_after = 0;
    u32 mask = 0u;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }

    saved_ga = fp->ga;
    saved_dust_effect_int = fp->status_vars.common.damage.dust_effect_int;
    saved_vel_damage_ground = fp->physics.vel_damage_ground;
    saved_vel_damage_air = fp->physics.vel_damage_air;
    saved_status_setup_active = sNdsFighterDashRunDamageStatusSetupActive;
    saved_dust_count = sNdsFighterDashRunDamageSetupDustCount;
    saved_dust_effect_count = sNdsFighterDashRunDamageSetupDustEffectCount;

    gNdsFighterDashRunDamageDustUpdateMask = 0u;
    gNdsFighterDashRunDamageDustUpdateEffectCount = 0u;
    gNdsFighterDashRunDamageDustUpdateWaitAfter = 0;

    sNdsFighterDashRunDamageSetupDustCount = 0u;
    sNdsFighterDashRunDamageSetupDustEffectCount = 0u;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    fp->status_vars.common.damage.dust_effect_int = 2;
    ftCommonDamageUpdateDustEffect(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = saved_status_setup_active;
    if ((fp->status_vars.common.damage.dust_effect_int == 1) &&
        (sNdsFighterDashRunDamageSetupDustCount == 0u) &&
        (sNdsFighterDashRunDamageSetupDustEffectCount == 0u))
    {
        mask |= NDS_DAMAGE_DUST_UPDATE_DEC;
    }

    sNdsFighterDashRunDamageSetupDustCount = 0u;
    sNdsFighterDashRunDamageSetupDustEffectCount = 0u;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    fp->ga = nMPKineticsGround;
    fp->physics.vel_damage_ground = 175.0F;
    fp->status_vars.common.damage.dust_effect_int = 1;
    ftCommonDamageUpdateDustEffect(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = saved_status_setup_active;

    effect_count = sNdsFighterDashRunDamageSetupDustEffectCount;
    wait_after = fp->status_vars.common.damage.dust_effect_int;
    if (effect_count == 1u)
    {
        mask |= NDS_DAMAGE_DUST_UPDATE_EFFECT;
    }
    if (wait_after == FTCOMMON_DAMAGE_EFFECT_WAIT_MID)
    {
        mask |= NDS_DAMAGE_DUST_UPDATE_RESET;
    }

    fp->ga = saved_ga;
    fp->status_vars.common.damage.dust_effect_int = saved_dust_effect_int;
    fp->physics.vel_damage_ground = saved_vel_damage_ground;
    fp->physics.vel_damage_air = saved_vel_damage_air;
    sNdsFighterDashRunDamageStatusSetupActive = saved_status_setup_active;
    sNdsFighterDashRunDamageSetupDustCount = saved_dust_count;
    sNdsFighterDashRunDamageSetupDustEffectCount = saved_dust_effect_count;

    if ((fp->ga == saved_ga) &&
        (fp->status_vars.common.damage.dust_effect_int ==
            saved_dust_effect_int) &&
        (fp->physics.vel_damage_ground == saved_vel_damage_ground) &&
        (fp->physics.vel_damage_air.x == saved_vel_damage_air.x) &&
        (fp->physics.vel_damage_air.y == saved_vel_damage_air.y) &&
        (fp->physics.vel_damage_air.z == saved_vel_damage_air.z) &&
        (sNdsFighterDashRunDamageStatusSetupActive ==
            saved_status_setup_active))
    {
        mask |= NDS_DAMAGE_DUST_UPDATE_RESTORE;
    }
    if ((mask & (NDS_DAMAGE_DUST_UPDATE_DEC |
                 NDS_DAMAGE_DUST_UPDATE_EFFECT |
                 NDS_DAMAGE_DUST_UPDATE_RESET |
                 NDS_DAMAGE_DUST_UPDATE_RESTORE)) ==
        (NDS_DAMAGE_DUST_UPDATE_DEC |
         NDS_DAMAGE_DUST_UPDATE_EFFECT |
         NDS_DAMAGE_DUST_UPDATE_RESET |
         NDS_DAMAGE_DUST_UPDATE_RESTORE))
    {
        mask |= NDS_DAMAGE_DUST_UPDATE_ORIGINAL;
    }

    gNdsFighterDashRunDamageDustUpdateMask = mask;
    gNdsFighterDashRunDamageDustUpdateEffectCount = effect_count;
    gNdsFighterDashRunDamageDustUpdateWaitAfter = wait_after;

    return ((mask & 0x1fu) == 0x1fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageHitstunPublic(GObj *fighter_gobj,
                                                       FTStruct *fp)
{
    s32 saved_hitstun_tics;
    f32 saved_public_knockback;
    f32 saved_damage_public_knockback;
    s32 hitstun_after = -1;
    s32 public_milli = 0;
    u32 mask = 0u;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }

    saved_hitstun_tics = fp->status_vars.common.damage.hitstun_tics;
    saved_public_knockback = fp->public_knockback;
    saved_damage_public_knockback =
        fp->status_vars.common.damage.public_knockback;

    gNdsFighterDashRunDamageHitstunPublicMask = 0u;
    gNdsFighterDashRunDamageHitstunPublicAfter = 0;
    gNdsFighterDashRunDamageHitstunPublicKnockbackMilli = 0;

    fp->status_vars.common.damage.hitstun_tics = 3;
    fp->public_knockback = 123.0F;
    fp->status_vars.common.damage.public_knockback = 456.0F;
    ftCommonDamageDecHitStunSetPublic(fighter_gobj);
    if ((fp->status_vars.common.damage.hitstun_tics == 2) &&
        (fp->public_knockback == 123.0F))
    {
        mask |= NDS_DAMAGE_HITSTUN_PUBLIC_DEC;
    }

    fp->status_vars.common.damage.hitstun_tics = 1;
    fp->public_knockback = 123.0F;
    fp->status_vars.common.damage.public_knockback = 456.0F;
    ftCommonDamageDecHitStunSetPublic(fighter_gobj);
    hitstun_after = fp->status_vars.common.damage.hitstun_tics;
    public_milli = ndsFloatToMilliSigned(fp->public_knockback);
    if ((hitstun_after == 0) && (public_milli == 456000))
    {
        mask |= NDS_DAMAGE_HITSTUN_PUBLIC_TRANSFER;
    }

    fp->status_vars.common.damage.hitstun_tics = saved_hitstun_tics;
    fp->public_knockback = saved_public_knockback;
    fp->status_vars.common.damage.public_knockback =
        saved_damage_public_knockback;

    if ((fp->status_vars.common.damage.hitstun_tics == saved_hitstun_tics) &&
        (fp->public_knockback == saved_public_knockback) &&
        (fp->status_vars.common.damage.public_knockback ==
            saved_damage_public_knockback))
    {
        mask |= NDS_DAMAGE_HITSTUN_PUBLIC_RESTORE;
    }
    if ((mask & (NDS_DAMAGE_HITSTUN_PUBLIC_DEC |
                 NDS_DAMAGE_HITSTUN_PUBLIC_TRANSFER |
                 NDS_DAMAGE_HITSTUN_PUBLIC_RESTORE)) ==
        (NDS_DAMAGE_HITSTUN_PUBLIC_DEC |
         NDS_DAMAGE_HITSTUN_PUBLIC_TRANSFER |
         NDS_DAMAGE_HITSTUN_PUBLIC_RESTORE))
    {
        mask |= NDS_DAMAGE_HITSTUN_PUBLIC_ORIGINAL;
    }

    gNdsFighterDashRunDamageHitstunPublicMask = mask;
    gNdsFighterDashRunDamageHitstunPublicAfter = hitstun_after;
    gNdsFighterDashRunDamageHitstunPublicKnockbackMilli = public_milli;

    return ((mask & 0xFu) == 0xFu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageColAnim(GObj *fighter_gobj)
{
    sb32 saved_status_setup_active =
        sNdsFighterDashRunDamageStatusSetupActive;
    u32 saved_colanim_count = sNdsFighterDashRunDamageSetupColAnimCount;
    s32 saved_colanim_id = sNdsFighterDashRunDamageColAnimLastID;
    s32 saved_colanim_duration = sNdsFighterDashRunDamageColAnimLastDuration;
    s32 saved_skeleton_level =
        sNdsFighterDashRunDamageSkeletonColAnimLastLevel;
    u32 mask = 0u;
    u32 ids = 0u;
    u32 routed = 0u;
    const s32 damage_level = 2;

    if (fighter_gobj == NULL)
    {
        return FALSE;
    }

    gNdsFighterDashRunDamageColAnimMask = 0u;
    gNdsFighterDashRunDamageColAnimIDs = 0u;
    gNdsFighterDashRunDamageColAnimCount = 0u;

    sNdsFighterDashRunDamageStatusSetupActive = TRUE;

#define NDS_DAMAGE_COLANIM_CHECK(bit, shift, element, expected_id, is_skeleton) \
    do { \
        sb32 is_set_colanim; \
        sNdsFighterDashRunDamageSetupColAnimCount = 0u; \
        sNdsFighterDashRunDamageColAnimLastID = -1; \
        sNdsFighterDashRunDamageColAnimLastDuration = -1; \
        sNdsFighterDashRunDamageSkeletonColAnimLastLevel = -1; \
        is_set_colanim = ftCommonDamageCheckElementSetColAnim( \
            fighter_gobj, (element), damage_level); \
        if ((is_set_colanim != FALSE) && \
            (((is_skeleton) != FALSE) ? \
                (sNdsFighterDashRunDamageSkeletonColAnimLastLevel == \
                    (expected_id)) : \
                ((sNdsFighterDashRunDamageColAnimLastID == (expected_id)) && \
                 (sNdsFighterDashRunDamageColAnimLastDuration == 0)))) \
        { \
            mask |= (bit); \
            ids |= (((u32)(expected_id) & 0xffu) << (shift)); \
            routed++; \
        } \
    } while (0)

    NDS_DAMAGE_COLANIM_CHECK(NDS_DAMAGE_COLANIM_FIRE, 0u,
                             nGMHitElementFire,
                             damage_level + nGMColAnimFighterDamageFireStart,
                             FALSE);
    NDS_DAMAGE_COLANIM_CHECK(NDS_DAMAGE_COLANIM_ELECTRIC, 8u,
                             nGMHitElementElectric, damage_level, TRUE);
    NDS_DAMAGE_COLANIM_CHECK(NDS_DAMAGE_COLANIM_FREEZE, 16u,
                             nGMHitElementFreezing,
                             damage_level + nGMColAnimFighterDamageIceStart,
                             FALSE);
    NDS_DAMAGE_COLANIM_CHECK(NDS_DAMAGE_COLANIM_NORMAL, 24u,
                             nGMHitElementSleep,
                             nGMColAnimFighterDamageCommon, FALSE);

#undef NDS_DAMAGE_COLANIM_CHECK

    sNdsFighterDashRunDamageStatusSetupActive =
        saved_status_setup_active;
    sNdsFighterDashRunDamageSetupColAnimCount = saved_colanim_count;
    sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
    sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
    sNdsFighterDashRunDamageSkeletonColAnimLastLevel = saved_skeleton_level;

    if ((sNdsFighterDashRunDamageStatusSetupActive ==
            saved_status_setup_active) &&
        (sNdsFighterDashRunDamageSetupColAnimCount == saved_colanim_count) &&
        (sNdsFighterDashRunDamageColAnimLastID == saved_colanim_id) &&
        (sNdsFighterDashRunDamageColAnimLastDuration ==
            saved_colanim_duration) &&
        (sNdsFighterDashRunDamageSkeletonColAnimLastLevel ==
            saved_skeleton_level))
    {
        mask |= NDS_DAMAGE_COLANIM_RESTORE;
    }
    if ((mask & (NDS_DAMAGE_COLANIM_FIRE | NDS_DAMAGE_COLANIM_ELECTRIC |
                 NDS_DAMAGE_COLANIM_FREEZE | NDS_DAMAGE_COLANIM_NORMAL |
                 NDS_DAMAGE_COLANIM_RESTORE)) ==
        (NDS_DAMAGE_COLANIM_FIRE | NDS_DAMAGE_COLANIM_ELECTRIC |
         NDS_DAMAGE_COLANIM_FREEZE | NDS_DAMAGE_COLANIM_NORMAL |
         NDS_DAMAGE_COLANIM_RESTORE))
    {
        mask |= NDS_DAMAGE_COLANIM_ORIGINAL;
    }

    gNdsFighterDashRunDamageColAnimMask = mask;
    gNdsFighterDashRunDamageColAnimIDs = ids;
    gNdsFighterDashRunDamageColAnimCount = routed;

    return ((mask & 0x3fu) == 0x3fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageColAnimUpdate(GObj *fighter_gobj,
                                                      FTStruct *fp)
{
    sb32 saved_status_setup_active =
        sNdsFighterDashRunDamageStatusSetupActive;
    u32 saved_colanim_count = sNdsFighterDashRunDamageSetupColAnimCount;
    u32 saved_run_update_count =
        sNdsFighterDashRunDamageRunUpdateColAnimCount;
    s32 saved_colanim_id = sNdsFighterDashRunDamageColAnimLastID;
    s32 saved_colanim_duration = sNdsFighterDashRunDamageColAnimLastDuration;
    s32 saved_skeleton_level =
        sNdsFighterDashRunDamageSkeletonColAnimLastLevel;
    f32 saved_damage_knockback;
    s32 saved_damage_element;
    u32 mask = 0u;
    u32 ids = 0u;
    u32 update_count = 0u;
    const f32 knockback = 45.0F;
    const s32 damage_level = 2;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }

    saved_damage_knockback = fp->damage_knockback;
    saved_damage_element = fp->damage_element;
    gNdsFighterDashRunDamageColAnimUpdateMask = 0u;
    gNdsFighterDashRunDamageColAnimUpdateIDs = 0u;
    gNdsFighterDashRunDamageColAnimUpdateCount = 0u;

    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    sNdsFighterDashRunDamageSetupColAnimCount = 0u;
    sNdsFighterDashRunDamageRunUpdateColAnimCount = 0u;
    sNdsFighterDashRunDamageColAnimLastID = -1;
    sNdsFighterDashRunDamageColAnimLastDuration = -1;
    sNdsFighterDashRunDamageSkeletonColAnimLastLevel = -1;

    ftCommonDamageUpdateDamageColAnim(fighter_gobj, knockback,
                                      nGMHitElementFire);
    if ((sNdsFighterDashRunDamageRunUpdateColAnimCount == 1u) &&
        (sNdsFighterDashRunDamageColAnimLastID ==
            (damage_level + nGMColAnimFighterDamageFireStart)) &&
        (sNdsFighterDashRunDamageColAnimLastDuration == 0))
    {
        mask |= NDS_DAMAGE_COLANIM_UPDATE_DIRECT;
        ids |= (u32)(damage_level + nGMColAnimFighterDamageFireStart);
        update_count++;
    }

    fp->damage_knockback = knockback;
    fp->damage_element = nGMHitElementElectric;
    sNdsFighterDashRunDamageSetupColAnimCount = 0u;
    sNdsFighterDashRunDamageColAnimLastID = -1;
    sNdsFighterDashRunDamageColAnimLastDuration = -1;
    sNdsFighterDashRunDamageSkeletonColAnimLastLevel = -1;
    ftCommonDamageSetDamageColAnim(fighter_gobj);
    if ((sNdsFighterDashRunDamageRunUpdateColAnimCount == 2u) &&
        (sNdsFighterDashRunDamageSkeletonColAnimLastLevel == damage_level))
    {
        mask |= NDS_DAMAGE_COLANIM_UPDATE_SET;
        ids |= ((u32)damage_level << 8);
        update_count++;
    }

    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    sNdsFighterDashRunDamageSetupColAnimCount = 0u;
    ftCommonDamageUpdateDamageColAnim(fighter_gobj, knockback,
                                      nGMHitElementFire);
    if ((sNdsFighterDashRunDamageRunUpdateColAnimCount == 2u) &&
        (sNdsFighterDashRunDamageSetupColAnimCount == 0u))
    {
        mask |= NDS_DAMAGE_COLANIM_UPDATE_NOOP;
    }

    fp->damage_knockback = saved_damage_knockback;
    fp->damage_element = saved_damage_element;
    sNdsFighterDashRunDamageStatusSetupActive =
        saved_status_setup_active;
    sNdsFighterDashRunDamageSetupColAnimCount = saved_colanim_count;
    sNdsFighterDashRunDamageRunUpdateColAnimCount = saved_run_update_count;
    sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
    sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
    sNdsFighterDashRunDamageSkeletonColAnimLastLevel = saved_skeleton_level;

    if ((fp->damage_knockback == saved_damage_knockback) &&
        (fp->damage_element == saved_damage_element) &&
        (sNdsFighterDashRunDamageStatusSetupActive ==
            saved_status_setup_active) &&
        (sNdsFighterDashRunDamageSetupColAnimCount == saved_colanim_count) &&
        (sNdsFighterDashRunDamageRunUpdateColAnimCount ==
            saved_run_update_count))
    {
        mask |= NDS_DAMAGE_COLANIM_UPDATE_RESTORE;
    }
    if ((mask & (NDS_DAMAGE_COLANIM_UPDATE_DIRECT |
                 NDS_DAMAGE_COLANIM_UPDATE_SET |
                 NDS_DAMAGE_COLANIM_UPDATE_NOOP |
                 NDS_DAMAGE_COLANIM_UPDATE_RESTORE)) ==
        (NDS_DAMAGE_COLANIM_UPDATE_DIRECT |
         NDS_DAMAGE_COLANIM_UPDATE_SET |
         NDS_DAMAGE_COLANIM_UPDATE_NOOP |
         NDS_DAMAGE_COLANIM_UPDATE_RESTORE))
    {
        mask |= NDS_DAMAGE_COLANIM_UPDATE_ORIGINAL;
    }

    gNdsFighterDashRunDamageColAnimUpdateMask = mask;
    gNdsFighterDashRunDamageColAnimUpdateIDs = ids;
    gNdsFighterDashRunDamageColAnimUpdateCount = update_count;

    return ((mask & 0x1fu) == 0x1fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageInvincibleGate(GObj *fighter_gobj,
                                                       FTStruct *fp)
{
    s32 saved_hitlag_tics;
    sb32 saved_is_knockback_over;
    s32 saved_invincible_tics;
    s32 saved_intangible_tics;
    s32 saved_special_hitstatus;
    u32 mask = 0u;
    s32 invincible_after = 0;
    s32 hitstatus_after = 0;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }

    saved_hitlag_tics = fp->hitlag_tics;
    saved_is_knockback_over =
        fp->status_vars.common.damage.is_knockback_over;
    saved_invincible_tics = fp->invincible_tics;
    saved_intangible_tics = fp->intangible_tics;
    saved_special_hitstatus = fp->special_hitstatus;

    gNdsFighterDashRunDamageInvincibleMask = 0u;
    gNdsFighterDashRunDamageInvincibleTicsAfter = 0;
    gNdsFighterDashRunDamageInvincibleHitStatusAfter = 0;

    fp->hitlag_tics = 1;
    fp->status_vars.common.damage.is_knockback_over = TRUE;
    fp->invincible_tics = 0;
    fp->intangible_tics = 0;
    fp->special_hitstatus = nGMHitStatusNormal;
    ftCommonDamageCheckSetInvincible(fighter_gobj);
    if ((fp->status_vars.common.damage.is_knockback_over != FALSE) &&
        (fp->invincible_tics == 0) &&
        (fp->special_hitstatus == nGMHitStatusNormal))
    {
        mask |= NDS_DAMAGE_INVINCIBLE_HITLAG_BLOCK;
    }

    fp->hitlag_tics = 0;
    fp->status_vars.common.damage.is_knockback_over = FALSE;
    fp->invincible_tics = 0;
    fp->intangible_tics = 0;
    fp->special_hitstatus = nGMHitStatusNormal;
    ftCommonDamageCheckSetInvincible(fighter_gobj);
    if ((fp->status_vars.common.damage.is_knockback_over == FALSE) &&
        (fp->invincible_tics == 0) &&
        (fp->special_hitstatus == nGMHitStatusNormal))
    {
        mask |= NDS_DAMAGE_INVINCIBLE_FLAG_BLOCK;
    }

    fp->hitlag_tics = 0;
    fp->status_vars.common.damage.is_knockback_over = TRUE;
    fp->invincible_tics = 0;
    fp->intangible_tics = 0;
    fp->special_hitstatus = nGMHitStatusNormal;
    ftCommonDamageCheckSetInvincible(fighter_gobj);
    invincible_after = fp->invincible_tics;
    hitstatus_after = fp->special_hitstatus;
    if ((fp->status_vars.common.damage.is_knockback_over == FALSE) &&
        (invincible_after >= 1) &&
        (hitstatus_after == nGMHitStatusInvincible))
    {
        mask |= NDS_DAMAGE_INVINCIBLE_SET;
    }

    fp->hitlag_tics = saved_hitlag_tics;
    fp->status_vars.common.damage.is_knockback_over =
        saved_is_knockback_over;
    fp->invincible_tics = saved_invincible_tics;
    fp->intangible_tics = saved_intangible_tics;
    fp->special_hitstatus = saved_special_hitstatus;
    if ((fp->hitlag_tics == saved_hitlag_tics) &&
        (fp->status_vars.common.damage.is_knockback_over ==
            saved_is_knockback_over) &&
        (fp->invincible_tics == saved_invincible_tics) &&
        (fp->intangible_tics == saved_intangible_tics) &&
        (fp->special_hitstatus == saved_special_hitstatus))
    {
        mask |= NDS_DAMAGE_INVINCIBLE_RESTORE;
    }
    if ((mask & (NDS_DAMAGE_INVINCIBLE_HITLAG_BLOCK |
                 NDS_DAMAGE_INVINCIBLE_FLAG_BLOCK |
                 NDS_DAMAGE_INVINCIBLE_SET |
                 NDS_DAMAGE_INVINCIBLE_RESTORE)) ==
        (NDS_DAMAGE_INVINCIBLE_HITLAG_BLOCK |
         NDS_DAMAGE_INVINCIBLE_FLAG_BLOCK |
         NDS_DAMAGE_INVINCIBLE_SET |
         NDS_DAMAGE_INVINCIBLE_RESTORE))
    {
        mask |= NDS_DAMAGE_INVINCIBLE_ORIGINAL;
    }

    gNdsFighterDashRunDamageInvincibleMask = mask;
    gNdsFighterDashRunDamageInvincibleTicsAfter = invincible_after;
    gNdsFighterDashRunDamageInvincibleHitStatusAfter = hitstatus_after;

    return ((mask & 0x1fu) == 0x1fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageLagUpdate(GObj *fighter_gobj,
                                                  FTStruct *fp)
{
    DObj *root;
    FTStruct saved_fp;
    Vec3f saved_translate;
    s32 apply_dx = 0;
    s32 apply_dy = 0;
    u32 mask = 0u;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        return FALSE;
    }

    saved_fp = *fp;
    saved_translate = root->translate.vec.f;
    gNdsFighterDashRunDamageLagUpdateMask = 0u;
    gNdsFighterDashRunDamageLagUpdateDeltaXMilli = 0;
    gNdsFighterDashRunDamageLagUpdateDeltaYMilli = 0;

    fp->hitlag_tics = 0;
    fp->input.pl.stick_range.x = 80;
    fp->input.pl.stick_range.y = 0;
    fp->tap_stick_x = 0;
    fp->tap_stick_y = FTCOMMON_DAMAGE_SMASH_DI_BUFFER_TICS_MAX;
    root->translate.vec.f = saved_translate;
    ftCommonDamageCommonProcLagUpdate(fighter_gobj);
    if ((ndsFloatToMilliSigned(root->translate.vec.f.x -
            saved_translate.x) == 0) &&
        (fp->tap_stick_x == 0) &&
        (fp->tap_stick_y == FTCOMMON_DAMAGE_SMASH_DI_BUFFER_TICS_MAX))
    {
        mask |= NDS_DAMAGE_LAGUPDATE_HITLAG_BLOCK;
    }

    fp->hitlag_tics = 2;
    fp->input.pl.stick_range.x = 10;
    fp->input.pl.stick_range.y = 0;
    fp->tap_stick_x = 0;
    fp->tap_stick_y = FTCOMMON_DAMAGE_SMASH_DI_BUFFER_TICS_MAX;
    root->translate.vec.f = saved_translate;
    ftCommonDamageCommonProcLagUpdate(fighter_gobj);
    if ((ndsFloatToMilliSigned(root->translate.vec.f.x -
            saved_translate.x) == 0) &&
        (fp->tap_stick_x == 0) &&
        (fp->tap_stick_y == FTCOMMON_DAMAGE_SMASH_DI_BUFFER_TICS_MAX))
    {
        mask |= NDS_DAMAGE_LAGUPDATE_STICK_BLOCK;
    }

    fp->hitlag_tics = 2;
    fp->input.pl.stick_range.x = 80;
    fp->input.pl.stick_range.y = 0;
    fp->tap_stick_x = FTCOMMON_DAMAGE_SMASH_DI_BUFFER_TICS_MAX;
    fp->tap_stick_y = FTCOMMON_DAMAGE_SMASH_DI_BUFFER_TICS_MAX;
    root->translate.vec.f = saved_translate;
    ftCommonDamageCommonProcLagUpdate(fighter_gobj);
    if ((ndsFloatToMilliSigned(root->translate.vec.f.x -
            saved_translate.x) == 0) &&
        (fp->tap_stick_x == FTCOMMON_DAMAGE_SMASH_DI_BUFFER_TICS_MAX) &&
        (fp->tap_stick_y == FTCOMMON_DAMAGE_SMASH_DI_BUFFER_TICS_MAX))
    {
        mask |= NDS_DAMAGE_LAGUPDATE_TAP_BLOCK;
    }

    fp->hitlag_tics = 2;
    fp->input.pl.stick_range.x = 80;
    fp->input.pl.stick_range.y = 0;
    fp->tap_stick_x = 0;
    fp->tap_stick_y = FTCOMMON_DAMAGE_SMASH_DI_BUFFER_TICS_MAX;
    root->translate.vec.f = saved_translate;
    ftCommonDamageCommonProcLagUpdate(fighter_gobj);
    apply_dx = ndsFloatToMilliSigned(root->translate.vec.f.x -
                                     saved_translate.x);
    apply_dy = ndsFloatToMilliSigned(root->translate.vec.f.y -
                                     saved_translate.y);
    if ((apply_dx == ndsFloatToMilliSigned(
            80.0F * FTCOMMON_DAMAGE_SMASH_DI_RANGE_MUL)) &&
        (apply_dy == 0) &&
        (fp->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX) &&
        (fp->tap_stick_y == FTINPUT_STICKBUFFER_TICS_MAX))
    {
        mask |= NDS_DAMAGE_LAGUPDATE_APPLY;
    }

    *fp = saved_fp;
    root->translate.vec.f = saved_translate;
    if ((fp->hitlag_tics == saved_fp.hitlag_tics) &&
        (fp->tap_stick_x == saved_fp.tap_stick_x) &&
        (fp->tap_stick_y == saved_fp.tap_stick_y) &&
        (ndsFloatToMilliSigned(root->translate.vec.f.x -
            saved_translate.x) == 0) &&
        (ndsFloatToMilliSigned(root->translate.vec.f.y -
            saved_translate.y) == 0))
    {
        mask |= NDS_DAMAGE_LAGUPDATE_RESTORE;
    }
    if ((mask & (NDS_DAMAGE_LAGUPDATE_HITLAG_BLOCK |
                 NDS_DAMAGE_LAGUPDATE_STICK_BLOCK |
                 NDS_DAMAGE_LAGUPDATE_TAP_BLOCK |
                 NDS_DAMAGE_LAGUPDATE_APPLY |
                 NDS_DAMAGE_LAGUPDATE_RESTORE)) ==
        (NDS_DAMAGE_LAGUPDATE_HITLAG_BLOCK |
         NDS_DAMAGE_LAGUPDATE_STICK_BLOCK |
         NDS_DAMAGE_LAGUPDATE_TAP_BLOCK |
         NDS_DAMAGE_LAGUPDATE_APPLY |
         NDS_DAMAGE_LAGUPDATE_RESTORE))
    {
        mask |= NDS_DAMAGE_LAGUPDATE_ORIGINAL;
    }

    gNdsFighterDashRunDamageLagUpdateMask = mask;
    gNdsFighterDashRunDamageLagUpdateDeltaXMilli = apply_dx;
    gNdsFighterDashRunDamageLagUpdateDeltaYMilli = apply_dy;

    return ((mask & 0x3fu) == 0x3fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageScreenFlash(void)
{
    sb32 saved_status_setup_active =
        sNdsFighterDashRunDamageStatusSetupActive;
    u32 saved_flash_count = sNdsFighterDashRunDamageSetupScreenFlashCount;
    s32 saved_flash_id = sNdsFighterDashRunDamageScreenFlashLastID;
    s32 saved_flash_duration =
        sNdsFighterDashRunDamageScreenFlashLastDuration;
    u32 mask = 0u;
    u32 ids = 0u;
    u32 routed = 0u;

    gNdsFighterDashRunDamageScreenFlashMask = 0u;
    gNdsFighterDashRunDamageScreenFlashIDs = 0u;
    gNdsFighterDashRunDamageScreenFlashCount = 0u;

    sNdsFighterDashRunDamageSetupScreenFlashCount = 0u;
    sNdsFighterDashRunDamageScreenFlashLastID = -1;
    sNdsFighterDashRunDamageScreenFlashLastDuration = -1;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageCheckMakeScreenFlash(
        FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH, nGMHitElementFire);
    if ((sNdsFighterDashRunDamageSetupScreenFlashCount == 0u) &&
        (sNdsFighterDashRunDamageScreenFlashLastID == -1))
    {
        mask |= NDS_DAMAGE_FLASH_LOW_NOOP;
    }

#define NDS_DAMAGE_FLASH_CHECK(bit, shift, element, expected_id) \
    do { \
        sNdsFighterDashRunDamageSetupScreenFlashCount = 0u; \
        sNdsFighterDashRunDamageScreenFlashLastID = -1; \
        sNdsFighterDashRunDamageScreenFlashLastDuration = -1; \
        ftCommonDamageCheckMakeScreenFlash( \
            FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH + 1.0F, (element)); \
        if ((sNdsFighterDashRunDamageSetupScreenFlashCount == 1u) && \
            (sNdsFighterDashRunDamageScreenFlashLastID == (expected_id)) && \
            (sNdsFighterDashRunDamageScreenFlashLastDuration == 0)) \
        { \
            mask |= (bit); \
            ids |= (((u32)(expected_id) & 0xffu) << (shift)); \
            routed++; \
        } \
    } while (0)

    NDS_DAMAGE_FLASH_CHECK(NDS_DAMAGE_FLASH_FIRE, 0u, nGMHitElementFire,
                           nGMColAnimScreenFlashDamageFire);
    NDS_DAMAGE_FLASH_CHECK(NDS_DAMAGE_FLASH_ELECTRIC, 8u,
                           nGMHitElementElectric,
                           nGMColAnimScreenFlashDamageElectric);
    NDS_DAMAGE_FLASH_CHECK(NDS_DAMAGE_FLASH_FREEZE, 16u,
                           nGMHitElementFreezing,
                           nGMColAnimScreenFlashDamageIce);
    NDS_DAMAGE_FLASH_CHECK(NDS_DAMAGE_FLASH_NORMAL, 24u,
                           nGMHitElementSleep,
                           nGMColAnimScreenFlashDamageNormal);

#undef NDS_DAMAGE_FLASH_CHECK

    sNdsFighterDashRunDamageStatusSetupActive =
        saved_status_setup_active;
    sNdsFighterDashRunDamageSetupScreenFlashCount = saved_flash_count;
    sNdsFighterDashRunDamageScreenFlashLastID = saved_flash_id;
    sNdsFighterDashRunDamageScreenFlashLastDuration = saved_flash_duration;

    if ((sNdsFighterDashRunDamageStatusSetupActive ==
            saved_status_setup_active) &&
        (sNdsFighterDashRunDamageSetupScreenFlashCount == saved_flash_count) &&
        (sNdsFighterDashRunDamageScreenFlashLastID == saved_flash_id) &&
        (sNdsFighterDashRunDamageScreenFlashLastDuration ==
            saved_flash_duration))
    {
        mask |= NDS_DAMAGE_FLASH_RESTORE;
    }
    if ((mask & (NDS_DAMAGE_FLASH_LOW_NOOP | NDS_DAMAGE_FLASH_FIRE |
                 NDS_DAMAGE_FLASH_ELECTRIC | NDS_DAMAGE_FLASH_FREEZE |
                 NDS_DAMAGE_FLASH_NORMAL | NDS_DAMAGE_FLASH_RESTORE)) ==
        (NDS_DAMAGE_FLASH_LOW_NOOP | NDS_DAMAGE_FLASH_FIRE |
         NDS_DAMAGE_FLASH_ELECTRIC | NDS_DAMAGE_FLASH_FREEZE |
         NDS_DAMAGE_FLASH_NORMAL | NDS_DAMAGE_FLASH_RESTORE))
    {
        mask |= NDS_DAMAGE_FLASH_ORIGINAL;
    }

    gNdsFighterDashRunDamageScreenFlashMask = mask;
    gNdsFighterDashRunDamageScreenFlashIDs = ids;
    gNdsFighterDashRunDamageScreenFlashCount = routed;

    return ((mask & 0x7fu) == 0x7fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamagePublic(FTStruct *target_fp,
                                               FTStruct *attacker_fp,
                                               s32 attacker_player)
{
    s32 saved_target_damage_player_num;
    f32 saved_target_public_knockback;
    f32 saved_target_damage_public_knockback;
    f32 saved_attacker_public_knockback;
    sb32 saved_status_setup_active;
    s32 force_knockback_milli;
    u32 force_count;
    u32 mask = 0u;

    if ((target_fp == NULL) || (attacker_fp == NULL))
    {
        return FALSE;
    }

    saved_target_damage_player_num = target_fp->damage_player_num;
    saved_target_public_knockback = target_fp->public_knockback;
    saved_target_damage_public_knockback =
        target_fp->status_vars.common.damage.public_knockback;
    saved_attacker_public_knockback = attacker_fp->public_knockback;
    saved_status_setup_active = sNdsFighterDashRunDamageStatusSetupActive;

    gNdsFighterDashRunDamagePublicMask = 0u;
    gNdsFighterDashRunDamagePublicKnockbackMilli = 0;
    gNdsFighterDashRunDamagePublicForceCount = 0u;
    sNdsFighterDashRunDamagePublicCheckCount = 0u;
    sNdsFighterDashRunDamagePublicForceCount = 0u;
    sNdsFighterDashRunDamagePublicLastKnockbackMilli = 0;

    target_fp->damage_player_num = attacker_player;
    target_fp->public_knockback = 123.0F;
    target_fp->status_vars.common.damage.public_knockback = 0.0F;
    attacker_fp->public_knockback = FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH;

    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageSetPublic(target_fp, 200.0F, F_CLC_DTOR32(80.0F));
    sNdsFighterDashRunDamageStatusSetupActive = saved_status_setup_active;

    if (target_fp->status_vars.common.damage.public_knockback == 160.0F)
    {
        mask |= NDS_DAMAGE_PUBLIC_ANGLE_REDUCE;
    }
    if (target_fp->public_knockback == 0.0F)
    {
        mask |= NDS_DAMAGE_PUBLIC_TARGET_RESET;
    }
    if ((sNdsFighterDashRunDamagePublicCheckCount == 1u) &&
        (sNdsFighterDashRunDamagePublicForceCount == 1u) &&
        (sNdsFighterDashRunDamagePublicLastKnockbackMilli == 160000))
    {
        mask |= NDS_DAMAGE_PUBLIC_FORCE;
    }
    force_knockback_milli =
        sNdsFighterDashRunDamagePublicLastKnockbackMilli;
    force_count = sNdsFighterDashRunDamagePublicForceCount;

    target_fp->public_knockback = 456.0F;
    target_fp->status_vars.common.damage.public_knockback = 0.0F;
    attacker_fp->public_knockback =
        FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH - 1.0F;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageSetPublic(target_fp, 111.0F, F_CLC_DTOR32(0.0F));
    sNdsFighterDashRunDamageStatusSetupActive = saved_status_setup_active;
    if ((sNdsFighterDashRunDamagePublicForceCount == force_count) &&
        (sNdsFighterDashRunDamagePublicLastKnockbackMilli == 111000) &&
        (target_fp->status_vars.common.damage.public_knockback == 111.0F) &&
        (target_fp->public_knockback == 0.0F))
    {
        mask |= NDS_DAMAGE_PUBLIC_NO_FORCE;
    }

    target_fp->damage_player_num = saved_target_damage_player_num;
    target_fp->public_knockback = saved_target_public_knockback;
    target_fp->status_vars.common.damage.public_knockback =
        saved_target_damage_public_knockback;
    attacker_fp->public_knockback = saved_attacker_public_knockback;

    if ((target_fp->damage_player_num == saved_target_damage_player_num) &&
        (target_fp->public_knockback == saved_target_public_knockback) &&
        (target_fp->status_vars.common.damage.public_knockback ==
            saved_target_damage_public_knockback) &&
        (attacker_fp->public_knockback == saved_attacker_public_knockback) &&
        (sNdsFighterDashRunDamageStatusSetupActive ==
            saved_status_setup_active))
    {
        mask |= NDS_DAMAGE_PUBLIC_RESTORE;
    }
    if ((mask & (NDS_DAMAGE_PUBLIC_ANGLE_REDUCE |
                 NDS_DAMAGE_PUBLIC_TARGET_RESET |
                 NDS_DAMAGE_PUBLIC_FORCE |
                 NDS_DAMAGE_PUBLIC_RESTORE |
                 NDS_DAMAGE_PUBLIC_NO_FORCE)) ==
        (NDS_DAMAGE_PUBLIC_ANGLE_REDUCE |
         NDS_DAMAGE_PUBLIC_TARGET_RESET |
         NDS_DAMAGE_PUBLIC_FORCE |
         NDS_DAMAGE_PUBLIC_RESTORE |
         NDS_DAMAGE_PUBLIC_NO_FORCE))
    {
        mask |= NDS_DAMAGE_PUBLIC_ORIGINAL;
    }

    gNdsFighterDashRunDamagePublicMask = mask;
    gNdsFighterDashRunDamagePublicKnockbackMilli =
        force_knockback_milli;
    gNdsFighterDashRunDamagePublicForceCount =
        force_count;

    return ((mask & 0x3fu) == 0x3fu) ? TRUE : FALSE;
}

static void ndsFighterDashRunProcParamsTrap(GObj *fighter_gobj);

static sb32 ndsFighterDashRunProbeDamageStatusSetup(GObj *target_gobj,
                                                    FTStruct *target_fp,
                                                    s32 status_before)
{
    DObj *target_root;
    FTStruct saved_target;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    s32 selected_status;
    s32 hitstun_before;
    s32 hitstun_after;
    s32 vel_before_physics;
    s32 vel_after_physics;
    s32 fall_vel_y_before;
    s32 fall_vel_y_after;
    s32 fastfall_vel_y_after;
    u32 installed_status;
    u32 installed_motion;
    u32 installed_ga;
    GObj *saved_throw_gobj;
    u32 mask = 0u;

    if ((target_gobj == NULL) || (target_fp == NULL) ||
        (ndsFighterStructIsPoolPointer(target_fp) == FALSE))
    {
        return FALSE;
    }

    target_root = DObjGetStruct(target_gobj);
    saved_target = *target_fp;
    saved_anim_frame = target_gobj->anim_frame;
    gNdsFighterDashRunDamageDustMask = 0u;
    gNdsFighterDashRunDamageDustWaits = 0u;
    gNdsFighterDashRunDamageDustUpdateMask = 0u;
    gNdsFighterDashRunDamageDustUpdateEffectCount = 0u;
    gNdsFighterDashRunDamageDustUpdateWaitAfter = 0;
    gNdsFighterDashRunDamageHitstunPublicMask = 0u;
    gNdsFighterDashRunDamageHitstunPublicAfter = 0;
    gNdsFighterDashRunDamageHitstunPublicKnockbackMilli = 0;
    gNdsFighterDashRunDamageColAnimMask = 0u;
    gNdsFighterDashRunDamageColAnimIDs = 0u;
    gNdsFighterDashRunDamageColAnimCount = 0u;
    gNdsFighterDashRunDamageColAnimUpdateMask = 0u;
    gNdsFighterDashRunDamageColAnimUpdateIDs = 0u;
    gNdsFighterDashRunDamageColAnimUpdateCount = 0u;
    gNdsFighterDashRunDamageInvincibleMask = 0u;
    gNdsFighterDashRunDamageInvincibleTicsAfter = 0;
    gNdsFighterDashRunDamageInvincibleHitStatusAfter = 0;
    gNdsFighterDashRunDamageLagUpdateMask = 0u;
    gNdsFighterDashRunDamageLagUpdateDeltaXMilli = 0;
    gNdsFighterDashRunDamageLagUpdateDeltaYMilli = 0;
    gNdsFighterDashRunDamageScreenFlashMask = 0u;
    gNdsFighterDashRunDamageScreenFlashIDs = 0u;
    gNdsFighterDashRunDamageScreenFlashCount = 0u;
    gNdsFighterDashRunDamagePublicMask = 0u;
    gNdsFighterDashRunDamagePublicKnockbackMilli = 0;
    gNdsFighterDashRunDamagePublicForceCount = 0u;
    gNdsFighterDashRunDamageCommonPhysicsMask = 0u;
    gNdsFighterDashRunDamageCommonPhysicsGroundMilli = 0;
    gNdsFighterDashRunDamageCommonPhysicsAirFrictionXMilli = 0;
    gNdsFighterDashRunDamageCommonPhysicsAirDriftYMilli = 0;
    gNdsFighterDashRunDamageCommonPhysicsClearState = nGMAttackStateNew;
    gNdsFighterDashRunDamageCommonCallbackMask = 0u;
    gNdsFighterDashRunDamageLevelMask = 0u;
    gNdsFighterDashRunDamageHoldResistMask = 0u;
    gNdsFighterDashRunDamageAirMapWallMask = 0u;
    gNdsFighterDashRunDamageKnockbackAngleMask = 0u;
    gNdsFighterDashRunDamageFallInterruptMask = 0u;
    gNdsFighterDashRunDamageFlyTopMask = 0u;
    gNdsFighterDashRunDamageFlyTopStatus = 0u;
    gNdsFighterDashRunDamageFlyTopMotion = 0u;
    gNdsFighterDashRunDamageFlyTopAngle = 0u;
    gNdsFighterDashRunDamageReplaceElectricMask = 0u;
    gNdsFighterDashRunDamageReplaceElectricStatus = 0u;
    gNdsFighterDashRunDamageReplaceElectricStoredStatus = 0u;
    gNdsFighterDashRunDamageReplaceElectricMotion = 0u;
    gNdsFighterDashRunDamageReplaceElectricDispatchStatus = 0u;
    gNdsFighterDashRunDamageReplaceElectricDispatchMotion = 0u;
    gNdsFighterDashRunDamageKindPreserveMask = 0u;
    sNdsFighterDashRunDamageOriginalInitCount = 0u;
    sNdsFighterDashRunDamageOriginalGotoCount = 0u;
    sNdsFighterDashRunDamageOriginalInitActive = FALSE;
    gNdsFighterDashRunDamageLoseGripMask = 0u;
    gNdsFighterDashRunDamageLoseGripReleaseCount = 0u;
    gNdsFighterDashRunDamageLoseGripCollisionCount = 0u;
    gNdsFighterDashRunDamageLoseGripSetAirCount = 0u;
    gNdsFighterDashRunDamageLoseGripTargetX = 0;
    gNdsFighterDashRunDamageLoseGripTargetY = 0;
    gNdsFighterDashRunDamageLoseGripLinkClearCount = 0u;
    if (target_root != NULL)
    {
        saved_anim_speed = target_root->anim_speed;
    }

    selected_status = target_fp->status_vars.common.damage.status_id;
    if (ndsFTCommonDamageIsStatus(selected_status) != FALSE)
    {
        mask |= NDS_DAMAGE_STATUS_SETUP_INIT;
    }
    (void)ndsFighterDashRunProbeDamageDustIntervals(target_fp);
    (void)ndsFighterDashRunProbeDamageDustUpdate(target_gobj, target_fp);
    (void)ndsFighterDashRunProbeDamageHitstunPublic(target_gobj, target_fp);
    (void)ndsFighterDashRunProbeDamageColAnim(target_gobj);
    (void)ndsFighterDashRunProbeDamageColAnimUpdate(target_gobj, target_fp);
    (void)ndsFighterDashRunProbeDamageInvincibleGate(target_gobj, target_fp);
    (void)ndsFighterDashRunProbeDamageLagUpdate(target_gobj, target_fp);
    (void)ndsFighterDashRunProbeDamageScreenFlash();
    (void)ndsFighterDashRunProbeDamageCommonPhysics(target_gobj, target_fp);
    (void)ndsFighterDashRunProbeDamageCommonCallbacks(target_gobj, target_fp);
    (void)ndsFighterDashRunProbeDamageLevels();
    (void)ndsFighterDashRunProbeDamageHoldResist(target_fp);
    (void)ndsFighterDashRunProbeDamageKnockbackAngle();

    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageGotoDamageStatus(target_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;

    hitstun_before = target_fp->status_vars.common.damage.hitstun_tics;
    if ((target_fp->status_id == selected_status) &&
        (target_fp->status_prev == status_before))
    {
        mask |= NDS_DAMAGE_STATUS_SETUP_STATUS;
    }
    if (target_fp->motion_id ==
        ndsFTCommonDamageMotionForStatus(target_fp->status_id))
    {
        mask |= NDS_DAMAGE_STATUS_SETUP_MOTION;
    }
    if ((target_fp->proc_update != NULL) &&
        (target_fp->proc_interrupt != NULL) &&
        (target_fp->proc_physics == ftCommonDamageCommonProcPhysics) &&
        (target_fp->proc_map != NULL) &&
        (target_fp->proc_lagupdate == ftCommonDamageCommonProcLagUpdate))
    {
        mask |= NDS_DAMAGE_STATUS_SETUP_CALLBACKS;
    }
    if (((target_fp->status_id == nFTCommonStatusDamageE1) ||
         (target_fp->status_id == nFTCommonStatusDamageE2)) ?
            ((target_fp->proc_passive == ftCommonDamageSetStatus) &&
             (ndsFTCommonDamageIsStatus(
                target_fp->status_vars.common.damage.status_id) != FALSE) &&
             (target_fp->status_vars.common.damage.status_id !=
                target_fp->status_id)) :
            (target_fp->proc_passive == ftCommonDamageCheckSetInvincible))
    {
        mask |= NDS_DAMAGE_STATUS_SETUP_PROC_PASSIVE;
    }
    if (target_fp->proc_passive == ftCommonDamageCheckSetInvincible)
    {
        FTStruct saved_passive_tick = *target_fp;

        target_fp->hitlag_tics = 0;
        target_fp->status_vars.common.damage.is_knockback_over = TRUE;
        target_fp->invincible_tics = 0;
        target_fp->intangible_tics = 0;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->proc_update = NULL;
        target_fp->proc_interrupt = NULL;

        ndsFTMainProcUpdateInterruptPassiveSlice(target_gobj);

        if ((target_fp->status_vars.common.damage.is_knockback_over ==
                FALSE) &&
            (target_fp->invincible_tics >= 1) &&
            (target_fp->special_hitstatus == nGMHitStatusInvincible))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_PROC_PASSIVE_TICK;
        }
        *target_fp = saved_passive_tick;
        target_gobj->anim_frame = saved_anim_frame;
        if (target_root != NULL)
        {
            target_root->anim_speed = saved_anim_speed;
        }
    }
    if ((ndsFTCommonDamageIsStatus(selected_status) != FALSE) &&
        (selected_status != nFTCommonStatusDamageE1) &&
        (selected_status != nFTCommonStatusDamageE2))
    {
        FTStruct saved_passive_status = *target_fp;

        target_fp->status_id = nFTCommonStatusDamageE1;
        target_fp->motion_id = nFTCommonMotionDamageE;
        target_fp->status_vars.common.damage.status_id = selected_status;
        target_fp->status_vars.common.damage.is_knockback_over = TRUE;
        target_fp->hitlag_tics = 0;
        target_fp->is_hitstun = FALSE;
        target_fp->invincible_tics = 0;
        target_fp->intangible_tics = 0;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->proc_passive = ftCommonDamageSetStatus;
        target_fp->proc_update = NULL;
        target_fp->proc_interrupt = NULL;

        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ndsFTMainProcUpdateInterruptPassiveSlice(target_gobj);
        sNdsFighterDashRunDamageStatusSetupActive = FALSE;

        if ((target_fp->status_id == selected_status) &&
            (target_fp->motion_id ==
                ndsFTCommonDamageMotionForStatus(selected_status)) &&
            (target_fp->is_hitstun != FALSE) &&
            (target_fp->status_vars.common.damage.is_knockback_over ==
                FALSE) &&
            (target_fp->invincible_tics >= 1) &&
            (target_fp->special_hitstatus == nGMHitStatusInvincible))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_PROC_PASSIVE_STATUS;
        }
        *target_fp = saved_passive_status;
        target_gobj->anim_frame = saved_anim_frame;
        if (target_root != NULL)
        {
            target_root->anim_speed = saved_anim_speed;
        }
    }
    {
        FTStruct saved_sleep = *target_fp;
        s32 expected_sleep_breakout =
            FTCOMMON_FURASLEEP_BREAKOUT_WAIT_DEFAULT -
            target_fp->percent_damage;
        s32 sleep_breakout_before;
        s32 sleep_breakout_after;
        s32 sleep_breakout_mash;

        if (expected_sleep_breakout <= 0)
        {
            expected_sleep_breakout = 0;
        }
        expected_sleep_breakout += FTCOMMON_FURASLEEP_BREAKOUT_WAIT_MIN;
        target_fp->damage_element = nGMHitElementSleep;
        target_fp->is_cliff_hold = TRUE;
        target_fp->cliffcatch_wait = 0;

        sNdsFighterDashRunDamageSetupColAnimCount = 0u;
        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageGotoDamageStatus(target_gobj);
        sNdsFighterDashRunDamageStatusSetupActive = FALSE;
        sleep_breakout_before = target_fp->breakout_wait;
        target_fp->input.pl.button_tap = 0;
        target_fp->input.pl.stick_range.x = 0;
        target_fp->input.pl.stick_range.y = 0;
        if (target_fp->proc_update != NULL)
        {
            target_fp->proc_update(target_gobj);
        }
        sleep_breakout_after = target_fp->breakout_wait;

        if ((target_fp->status_id == nFTCommonStatusFuraSleep) &&
            (target_fp->motion_id == nFTCommonMotionFuraSleep) &&
            (target_fp->motion_script_id == nFTCommonMotionFuraSleep) &&
            (target_fp->proc_update == ndsBaseFTCommonFuraSleepProcUpdate) &&
            (sleep_breakout_before == expected_sleep_breakout) &&
            (sleep_breakout_after == (expected_sleep_breakout - 1)) &&
            (target_fp->cliffcatch_wait == FTCOMMON_CLIFF_CATCH_WAIT) &&
            (sNdsFighterDashRunDamageSetupColAnimCount == 1u))
        {
            if (target_fp->input.button_mask_a == 0u)
            {
                target_fp->input.button_mask_a = A_BUTTON;
            }
            target_fp->breakout_wait = expected_sleep_breakout;
            target_fp->breakout_lr = 0;
            target_fp->breakout_ud = 0;
            target_fp->input.pl.button_tap = target_fp->input.button_mask_a;
            target_fp->input.pl.stick_range.x =
                FTCOMMON_CAPTURE_MASH_STICK_RANGE_MIN + 1;
            target_fp->input.pl.stick_range.y = 0;
            target_fp->proc_update(target_gobj);
            sleep_breakout_mash = target_fp->breakout_wait;

            target_fp->breakout_wait = 1;
            target_fp->input.pl.button_tap = 0;
            target_fp->input.pl.stick_range.x = 0;
            target_fp->proc_update(target_gobj);
            if ((target_fp->status_id == nFTCommonStatusWait) &&
                (target_fp->motion_id == nFTCommonMotionWait) &&
                (sleep_breakout_mash == (expected_sleep_breakout - 9)) &&
                (target_fp->breakout_lr == 1))
            {
                mask |= NDS_DAMAGE_STATUS_SETUP_SLEEP_STATUS;
            }
        }
        *target_fp = saved_sleep;
        target_gobj->anim_frame = saved_anim_frame;
        if (target_root != NULL)
        {
            target_root->anim_speed = saved_anim_speed;
        }
    }
    if ((target_fp->is_hitstun != FALSE) && (hitstun_before > 0) &&
        (target_fp->status_vars.common.damage.public_knockback > 0.0F) &&
        (target_fp->status_vars.common.damage.public_knockback <=
         target_fp->damage_knockback))
    {
        mask |= NDS_DAMAGE_STATUS_SETUP_HITSTUN;
    }

    if (ndsFTCommonDamageIsStatus(selected_status) != FALSE)
    {
        FTStruct saved_invincible = *target_fp;
        f32 saved_invincible_anim_frame = target_gobj->anim_frame;
        f32 saved_invincible_anim_speed = saved_anim_speed;

        target_fp->hitlag_tics = 0;
        target_fp->status_vars.common.damage.status_id = selected_status;
        target_fp->status_vars.common.damage.is_knockback_over = TRUE;
        target_fp->invincible_tics = 0;
        target_fp->intangible_tics = 0;
        target_fp->special_hitstatus = nGMHitStatusNormal;

        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageSetStatus(target_gobj);
        sNdsFighterDashRunDamageStatusSetupActive = FALSE;

        if ((target_fp->status_id == selected_status) &&
            (target_fp->is_hitstun != FALSE) &&
            (target_fp->status_vars.common.damage.is_knockback_over ==
                FALSE) &&
            (target_fp->invincible_tics >= 1) &&
            (target_fp->special_hitstatus == nGMHitStatusInvincible))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_KNOCKBACK_INVINCIBLE;
        }
        *target_fp = saved_invincible;
        target_gobj->anim_frame = saved_invincible_anim_frame;
        if (target_root != NULL)
        {
            target_root->anim_speed = saved_invincible_anim_speed;
        }
    }

    if ((target_root != NULL) &&
        (ndsFTCommonDamageIsStatus(selected_status) != FALSE))
    {
        FTStruct saved_lagupdate = *target_fp;
        Vec3f saved_lagupdate_translate = target_root->translate.vec.f;
        s32 lagupdate_dx;
        s32 lagupdate_dy;

        target_fp->hitlag_tics = 2;
        target_fp->input.pl.stick_range.x = 80;
        target_fp->input.pl.stick_range.y = 0;
        target_fp->tap_stick_x = 0;
        target_fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;

        ftCommonDamageCommonProcLagUpdate(target_gobj);

        lagupdate_dx =
            ndsFloatToMilliSigned(target_root->translate.vec.f.x -
                                  saved_lagupdate_translate.x);
        lagupdate_dy =
            ndsFloatToMilliSigned(target_root->translate.vec.f.y -
                                  saved_lagupdate_translate.y);

        if ((target_fp->hitlag_tics == 2) &&
            (target_fp->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX) &&
            (target_fp->tap_stick_y == FTINPUT_STICKBUFFER_TICS_MAX) &&
            (lagupdate_dx == ndsFloatToMilliSigned(
                80.0F * FTCOMMON_DAMAGE_SMASH_DI_RANGE_MUL)) &&
            (lagupdate_dy == 0))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_LAGUPDATE;
        }
        *target_fp = saved_lagupdate;
        target_root->translate.vec.f = saved_lagupdate_translate;
        target_gobj->anim_frame = saved_anim_frame;
        target_root->anim_speed = saved_anim_speed;
    }

    if ((target_root != NULL) &&
        (target_fp->proc_lagupdate == ftCommonDamageCommonProcLagUpdate))
    {
        FTStruct saved_lifecycle = *target_fp;
        Vec3f saved_lifecycle_translate = target_root->translate.vec.f;
        u32 lagend_count_before = sNdsFighterDashRunDamageLagEndCount;
        u32 anim_events_before = gNdsFighterDashRunAnimEventsCallCount;
        s32 lifecycle_dx;

        target_fp->hitlag_tics = 2;
        target_fp->is_knockback_paused = TRUE;
        target_fp->proc_lagend = ndsFighterDashRunDamageLagEnd;
        target_fp->input.pl.stick_range.x = 80;
        target_fp->input.pl.stick_range.y = 0;
        target_fp->tap_stick_x = 0;
        target_fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;

        ndsFTMainProcUpdateHitlagLifecycleSlice(target_gobj);
        ndsFTMainProcPhysicsLagUpdateSlice(target_gobj);
        lifecycle_dx =
            ndsFloatToMilliSigned(target_root->translate.vec.f.x -
                                  saved_lifecycle_translate.x);
        ndsFTMainProcUpdateHitlagLifecycleSlice(target_gobj);

        if ((lifecycle_dx == ndsFloatToMilliSigned(
                80.0F * FTCOMMON_DAMAGE_SMASH_DI_RANGE_MUL)) &&
            (target_fp->hitlag_tics == 0) &&
            (target_fp->is_knockback_paused == FALSE) &&
            (sNdsFighterDashRunDamageLagEndCount ==
                (lagend_count_before + 1u)) &&
            (gNdsFighterDashRunAnimEventsCallCount >
                anim_events_before))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_HITLAG_LIFECYCLE;
        }
        *target_fp = saved_lifecycle;
        target_root->translate.vec.f = saved_lifecycle_translate;
        target_gobj->anim_frame = saved_anim_frame;
        target_root->anim_speed = saved_anim_speed;
        sNdsFighterDashRunDamageLagEndCount = lagend_count_before;
        gNdsFighterDashRunAnimEventsCallCount = anim_events_before;
    }

    if (ndsFTCommonDamageIsStatus(selected_status) != FALSE)
    {
        FTStruct saved_tail = *target_fp;
        FTStruct *attacker_fp = NULL;
        FTStruct saved_attacker;
        FTAttributes *target_attr = target_fp->attr;
        u32 anim_events_before = gNdsFighterDashRunAnimEventsCallCount;
        s32 attacker_player = 1;
        s32 attack_count_before = 0;
        u16 saved_damage_sfx = 0u;
        u32 voice_mask = 0u;
        u32 voice_count_total = 0u;
        u32 damage_kind_mask = 0u;

        if ((sNdsFighterStructPoolUsedMask & (1u << 1)) != 0u)
        {
            attacker_fp = &sNdsFighterStructPool[1];
            saved_attacker = *attacker_fp;
            attacker_player = attacker_fp->player;
            attack_count_before = attacker_fp->attack_count;
            attacker_fp->public_knockback =
                FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH;
        }
        if (target_attr != NULL)
        {
            saved_damage_sfx = target_attr->damage_sfx;
            target_attr->damage_sfx = nSYAudioVoiceAnnounceMario;
            voice_mask |= NDS_DAMAGE_VOICE_ATTR;
        }

        sNdsFighterDashRunDamageSetupPublicCount = 0u;
        sNdsFighterDashRunDamageSetupColAnimCount = 0u;
        sNdsFighterDashRunDamageSetupScreenFlashCount = 0u;
        sNdsFighterDashRunDamageSetupRumbleCount = 0u;
        sNdsFighterDashRunDamageSetupDustCount = 0u;
        sNdsFighterDashRunDamageSetupDustEffectCount = 0u;
        sNdsFighterDashRunDamageSetupPlayerTagCount = 0u;
        sNdsFighterDashRunDamageSetupAttackerCount = 0u;
        sNdsFighterDashRunDamageVoiceCount = 0u;
        sNdsFighterDashRunDamageVoiceLastFGM = 0u;

        if (attacker_fp != NULL)
        {
            (void)ndsFighterDashRunProbeDamagePublic(
                target_fp, attacker_fp, attacker_player);
            attacker_fp->public_knockback =
                FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH;
        }

        sNdsFighterDashRunDamageVoiceActive = TRUE;
        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        sNdsFighterDashRunDamageOriginalInitActive = TRUE;
        target_fp->damage_kind = nFTDamageKindCatch;
        if (target_fp->damage_kind == nFTDamageKindCatch)
        {
            damage_kind_mask |= NDS_DAMAGE_KIND_PRESERVE_BEFORE;
        }
        ftCommonDamageInitDamageVars(target_gobj, -1, 9, 400.0F, 80, 1, 1,
                                     nGMHitElementFreezing, attacker_player,
                                     TRUE, FALSE, TRUE);
        sNdsFighterDashRunDamageOriginalInitActive = FALSE;
        sNdsFighterDashRunDamageStatusSetupActive = FALSE;
        sNdsFighterDashRunDamageVoiceActive = FALSE;
        if (target_fp->damage_kind == nFTDamageKindCatch)
        {
            damage_kind_mask |= NDS_DAMAGE_KIND_PRESERVE_AFTER;
        }
        if ((target_attr != NULL) &&
            (sNdsFighterDashRunDamageVoiceCount >= 1u) &&
            (sNdsFighterDashRunDamageVoiceLastFGM ==
                nSYAudioVoiceAnnounceMario))
        {
            voice_count_total += sNdsFighterDashRunDamageVoiceCount;
            voice_mask |= NDS_DAMAGE_VOICE_THRESHOLD_CALL;
        }

        if ((sNdsFighterDashRunDamageSetupPublicCount >= 1u) &&
            (target_fp->public_knockback == 0.0F) &&
            (target_fp->status_vars.common.damage.public_knockback <=
                400.0F) &&
            (target_fp->status_vars.common.damage.public_knockback > 0.0F))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_PUBLIC;
        }
        if (sNdsFighterDashRunDamageSetupColAnimCount >= 1u)
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_COLANIM;
        }
        if (sNdsFighterDashRunDamageSetupScreenFlashCount >= 1u)
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_SCREENFLASH;
        }
        if (sNdsFighterDashRunDamageSetupRumbleCount >= 1u)
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_RUMBLE;
        }
        if ((sNdsFighterDashRunDamageSetupPlayerTagCount >= 1u) &&
            (target_fp->playertag_wait ==
                FTCOMMON_DAMAGE_FIGHTER_PLAYERTAG_HIDE_FRAMES))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_PLAYERTAG;
        }
        if ((attacker_fp != NULL) &&
            (sNdsFighterDashRunDamageSetupAttackerCount >= 1u) &&
            (attacker_fp->attack_count == (attack_count_before + 1)) &&
            (attacker_fp->attack_knockback == 400.0F))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_ATTACKER;
        }
        if (target_attr != NULL)
        {
            *target_fp = saved_tail;
            if (attacker_fp != NULL)
            {
                *attacker_fp = saved_attacker;
            }
            target_gobj->anim_frame = saved_anim_frame;
            if (target_root != NULL)
            {
                target_root->anim_speed = saved_anim_speed;
            }
            target_attr->damage_sfx = nSYAudioVoiceAnnounceFox;
            sNdsFighterDashRunDamageVoiceCount = 0u;
            sNdsFighterDashRunDamageVoiceLastFGM = 0u;
            sNdsFighterDashRunDamageVoiceActive = TRUE;
            sNdsFighterDashRunDamageStatusSetupActive = TRUE;
            ftCommonDamageInitDamageVars(target_gobj, -1, 1, 10.0F, 45, 1,
                                         0, nGMHitElementNormal,
                                         attacker_player, FALSE, TRUE,
                                         TRUE);
            sNdsFighterDashRunDamageStatusSetupActive = FALSE;
            sNdsFighterDashRunDamageVoiceActive = FALSE;
            if ((sNdsFighterDashRunDamageVoiceCount >= 1u) &&
                (sNdsFighterDashRunDamageVoiceLastFGM ==
                    nSYAudioVoiceAnnounceFox))
            {
                voice_count_total += sNdsFighterDashRunDamageVoiceCount;
                voice_mask |= NDS_DAMAGE_VOICE_FORCE_CALL;
            }
            target_attr->damage_sfx = saved_damage_sfx;
        }
        if ((target_attr == NULL) ||
            (target_attr->damage_sfx == saved_damage_sfx))
        {
            voice_mask |= NDS_DAMAGE_VOICE_RESTORE;
        }
        gNdsFighterDashRunDamageVoiceMask = voice_mask;
        gNdsFighterDashRunDamageVoiceCount = voice_count_total;
        gNdsFighterDashRunDamageVoiceThresholdFGM =
            nSYAudioVoiceAnnounceMario;
        gNdsFighterDashRunDamageVoiceForceFGM =
            nSYAudioVoiceAnnounceFox;

        *target_fp = saved_tail;
        if (attacker_fp != NULL)
        {
            *attacker_fp = saved_attacker;
        }
        target_gobj->anim_frame = saved_anim_frame;
        if (target_root != NULL)
        {
            target_root->anim_speed = saved_anim_speed;
        }
        if (target_fp->damage_kind == saved_tail.damage_kind)
        {
            damage_kind_mask |= NDS_DAMAGE_KIND_PRESERVE_RESTORE;
        }
        {
            u32 trap_count_before = sNdsFighterDashRunProcParamsTrapCount;
            u32 colanim_count_before =
                sNdsFighterDashRunDamageSetupColAnimCount;
            u32 run_update_colanim_before =
                sNdsFighterDashRunDamageRunUpdateColAnimCount;
            s32 colanim_id_before = sNdsFighterDashRunDamageColAnimLastID;
            s32 colanim_duration_before =
                sNdsFighterDashRunDamageColAnimLastDuration;
            s32 skeleton_level_before =
                sNdsFighterDashRunDamageSkeletonColAnimLastLevel;
            sb32 status_setup_active_before =
                sNdsFighterDashRunDamageStatusSetupActive;

            target_fp->status_id = nFTCommonStatusTwister;
            target_fp->fkind = nFTKindMario;
            target_fp->damage_kind = nFTDamageKindStatus;
            target_fp->damage_knockback = 12.0F;
            target_fp->damage_queue = 3;
            target_fp->damage_lag = 3;
            target_fp->knockback_resist_status = 0.0F;
            target_fp->knockback_resist_passive = 0.0F;
            target_fp->hitlag_mul = 1.0F;
            target_fp->shield_damage = 0;
            target_fp->shield_damage_total = 0;
            target_fp->proc_trap = ndsFighterDashRunProcParamsTrap;

            sNdsFighterDashRunDamageStatusSetupActive = TRUE;
            ftMainProcParams(target_gobj);
            sNdsFighterDashRunDamageStatusSetupActive =
                status_setup_active_before;

            if (sNdsFighterDashRunDamageRunUpdateColAnimCount >
                    run_update_colanim_before)
            {
                damage_kind_mask |= NDS_DAMAGE_KIND_PROCPARAMS_TWISTER;
            }
            if (sNdsFighterDashRunProcParamsTrapCount ==
                    (trap_count_before + 1u))
            {
                damage_kind_mask |= NDS_DAMAGE_KIND_PROCPARAMS_TRAP;
            }

            *target_fp = saved_tail;
            target_gobj->anim_frame = saved_anim_frame;
            if (target_root != NULL)
            {
                target_root->anim_speed = saved_anim_speed;
            }
            sNdsFighterDashRunProcParamsTrapCount = trap_count_before;
            sNdsFighterDashRunDamageSetupColAnimCount = colanim_count_before;
            sNdsFighterDashRunDamageRunUpdateColAnimCount =
                run_update_colanim_before;
            sNdsFighterDashRunDamageColAnimLastID = colanim_id_before;
            sNdsFighterDashRunDamageColAnimLastDuration =
                colanim_duration_before;
            sNdsFighterDashRunDamageSkeletonColAnimLastLevel =
                skeleton_level_before;
            sNdsFighterDashRunDamageStatusSetupActive =
                status_setup_active_before;
        }
        if (sNdsFighterDashRunDamageOriginalInitCount > 0u)
        {
            damage_kind_mask |= NDS_DAMAGE_KIND_PRESERVE_ORIGINAL_INIT;
        }
        if (sNdsFighterDashRunDamageOriginalGotoCount > 0u)
        {
            damage_kind_mask |= NDS_DAMAGE_KIND_PRESERVE_ORIGINAL_GOTO;
        }
        gNdsFighterDashRunDamageKindPreserveMask = damage_kind_mask;
        gNdsFighterDashRunAnimEventsCallCount = anim_events_before;
    }

    if (target_root != NULL)
    {
        FTStruct saved_flytop_select = *target_fp;
        f32 saved_flytop_anim_frame = target_gobj->anim_frame;
        f32 saved_flytop_anim_speed = target_root->anim_speed;
        u32 saved_flytop_anim_events =
            gNdsFighterDashRunAnimEventsCallCount;
        f32 flytop_angle = ndsFTCommonDamageGetKnockbackAngle(
            90, nMPKineticsAir, 400.0F);
        u32 flytop_mask = 0u;

        target_fp->ga = nMPKineticsAir;
        target_fp->percent_damage = 0;
        target_fp->status_id = status_before;
        target_fp->motion_id = nFTCommonMotionWait;
        target_fp->status_vars.common.damage.status_id = status_before;

        if (ndsFTCommonDamageGetDamageLevel(ftParamGetHitStun(400.0F)) == 3)
        {
            flytop_mask |= NDS_DAMAGE_FLYTOP_LEVEL;
        }
        if ((flytop_angle > FTCOMMON_DAMAGE_FIGHTER_FLYTOP_ANGLE_LOW) &&
            (flytop_angle < FTCOMMON_DAMAGE_FIGHTER_FLYTOP_ANGLE_HIGH))
        {
            flytop_mask |= NDS_DAMAGE_FLYTOP_ANGLE;
        }

        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageInitDamageVars(target_gobj, -1, 9, 400.0F, 90, 1, 1,
                                     nGMHitElementNormal, 1, FALSE, FALSE,
                                     TRUE);
        sNdsFighterDashRunDamageStatusSetupActive = FALSE;

        if ((target_fp->status_id == nFTCommonStatusDamageFlyTop) &&
            (target_fp->motion_id == nFTCommonMotionDamageFlyTop) &&
            (target_fp->ga == nMPKineticsAir))
        {
            flytop_mask |= NDS_DAMAGE_FLYTOP_STATUS;
        }

        gNdsFighterDashRunDamageFlyTopStatus = (u32)target_fp->status_id;
        gNdsFighterDashRunDamageFlyTopMotion = (u32)target_fp->motion_id;
        gNdsFighterDashRunDamageFlyTopAngle = 90u;

        *target_fp = saved_flytop_select;
        target_gobj->anim_frame = saved_flytop_anim_frame;
        target_root->anim_speed = saved_flytop_anim_speed;
        gNdsFighterDashRunAnimEventsCallCount = saved_flytop_anim_events;

        if ((target_fp->status_id == saved_flytop_select.status_id) &&
            (target_fp->motion_id == saved_flytop_select.motion_id))
        {
            flytop_mask |= NDS_DAMAGE_FLYTOP_RESTORE;
        }
        gNdsFighterDashRunDamageFlyTopMask = flytop_mask;
    }

    if (target_root != NULL)
    {
        FTStruct saved_replace = *target_fp;
        f32 saved_replace_anim_frame = target_gobj->anim_frame;
        f32 saved_replace_anim_speed = target_root->anim_speed;
        u32 saved_replace_anim_events =
            gNdsFighterDashRunAnimEventsCallCount;
        u32 replace_mask = 0u;

        target_fp->ga = nMPKineticsAir;
        target_fp->percent_damage = 0;
        target_fp->status_id = status_before;
        target_fp->motion_id = nFTCommonMotionWait;
        target_fp->status_vars.common.damage.status_id = status_before;

        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageInitDamageVars(target_gobj, nFTCommonStatusDamageFlyRoll,
                                     9, 400.0F, 90, 1, 1,
                                     nGMHitElementElectric, 1, FALSE, FALSE,
                                     TRUE);
        sNdsFighterDashRunDamageStatusSetupActive = FALSE;

        if (target_fp->status_vars.common.damage.status_id ==
            nFTCommonStatusDamageFlyRoll)
        {
            replace_mask |= NDS_DAMAGE_REPLACE_STATUS;
        }
        if ((target_fp->status_id == nFTCommonStatusDamageE2) &&
            (target_fp->motion_id == nFTCommonMotionDamageE))
        {
            replace_mask |= NDS_DAMAGE_REPLACE_ELECTRIC;
        }
        if (target_fp->proc_passive == ftCommonDamageSetStatus)
        {
            replace_mask |= NDS_DAMAGE_REPLACE_PASSIVE;
        }

        gNdsFighterDashRunDamageReplaceElectricStatus =
            (u32)target_fp->status_id;
        gNdsFighterDashRunDamageReplaceElectricStoredStatus =
            (u32)target_fp->status_vars.common.damage.status_id;
        gNdsFighterDashRunDamageReplaceElectricMotion =
            (u32)target_fp->motion_id;

        if (target_fp->proc_passive == ftCommonDamageSetStatus)
        {
            u32 block_status = (u32)target_fp->status_id;
            u32 block_motion = (u32)target_fp->motion_id;
            u32 block_stored_status =
                (u32)target_fp->status_vars.common.damage.status_id;
            u32 block_anim_events = gNdsFighterDashRunAnimEventsCallCount;

            target_fp->hitlag_tics = 2;
            sNdsFighterDashRunDamageStatusSetupActive = TRUE;
            target_fp->proc_passive(target_gobj);
            sNdsFighterDashRunDamageStatusSetupActive = FALSE;

            if (((u32)target_fp->status_id == block_status) &&
                ((u32)target_fp->motion_id == block_motion) &&
                ((u32)target_fp->status_vars.common.damage.status_id ==
                    block_stored_status) &&
                (gNdsFighterDashRunAnimEventsCallCount == block_anim_events))
            {
                replace_mask |= NDS_DAMAGE_REPLACE_HITLAG_BLOCK;
            }
            target_fp->hitlag_tics = 0;
            sNdsFighterDashRunDamageStatusSetupActive = TRUE;
            target_fp->proc_passive(target_gobj);
            sNdsFighterDashRunDamageStatusSetupActive = FALSE;

            if ((target_fp->status_id == nFTCommonStatusDamageFlyRoll) &&
                (target_fp->motion_id == nFTCommonMotionDamageFlyRoll) &&
                (target_fp->is_hitstun != FALSE))
            {
                replace_mask |= NDS_DAMAGE_REPLACE_DISPATCH;
            }
        }
        gNdsFighterDashRunDamageReplaceElectricDispatchStatus =
            (u32)target_fp->status_id;
        gNdsFighterDashRunDamageReplaceElectricDispatchMotion =
            (u32)target_fp->motion_id;

        *target_fp = saved_replace;
        target_gobj->anim_frame = saved_replace_anim_frame;
        target_root->anim_speed = saved_replace_anim_speed;
        gNdsFighterDashRunAnimEventsCallCount = saved_replace_anim_events;

        if ((target_fp->status_id == saved_replace.status_id) &&
            (target_fp->motion_id == saved_replace.motion_id))
        {
            replace_mask |= NDS_DAMAGE_REPLACE_RESTORE;
        }
        gNdsFighterDashRunDamageReplaceElectricMask = replace_mask;
    }

    if (target_root != NULL)
    {
        FTStruct saved_flyroll_select = *target_fp;
        f32 saved_flyroll_anim_frame = target_gobj->anim_frame;
        f32 saved_flyroll_anim_speed = target_root->anim_speed;
        s32 saved_rng_seed = syUtilsRandSeed();
        u32 saved_flyroll_anim_events =
            gNdsFighterDashRunAnimEventsCallCount;
        u32 flyroll_mask = 0u;

        target_fp->ga = nMPKineticsAir;
        target_fp->percent_damage = FTCOMMON_DAMAGE_FIGHTER_FLYROLL_DAMAGE_MIN;
        target_fp->status_id = status_before;
        target_fp->motion_id = nFTCommonMotionWait;
        target_fp->status_vars.common.damage.status_id = status_before;
        syUtilsSetRandomSeed(1);

        if (target_fp->percent_damage >=
            FTCOMMON_DAMAGE_FIGHTER_FLYROLL_DAMAGE_MIN)
        {
            flyroll_mask |= NDS_DAMAGE_FLYROLL_PERCENT;
        }
        /* 45 degrees is intentionally outside the original FlyTop range. */
        flyroll_mask |= NDS_DAMAGE_FLYROLL_ANGLE;

        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageInitDamageVars(target_gobj, -1, 9, 400.0F, 45, 1, 1,
                                     nGMHitElementNormal, 1, FALSE, FALSE,
                                     TRUE);
        sNdsFighterDashRunDamageStatusSetupActive = FALSE;

        if (syUtilsRandSeed() != 1)
        {
            flyroll_mask |= NDS_DAMAGE_FLYROLL_RNG;
        }
        if ((target_fp->status_id == nFTCommonStatusDamageFlyRoll) &&
            (target_fp->motion_id == nFTCommonMotionDamageFlyRoll) &&
            (target_fp->ga == nMPKineticsAir))
        {
            flyroll_mask |= NDS_DAMAGE_FLYROLL_STATUS;
        }

        gNdsFighterDashRunDamageFlyRollStatus = (u32)target_fp->status_id;
        gNdsFighterDashRunDamageFlyRollMotion = (u32)target_fp->motion_id;
        gNdsFighterDashRunDamageFlyRollPercent =
            (u32)target_fp->percent_damage;

        *target_fp = saved_flyroll_select;
        target_gobj->anim_frame = saved_flyroll_anim_frame;
        target_root->anim_speed = saved_flyroll_anim_speed;
        gNdsFighterDashRunAnimEventsCallCount = saved_flyroll_anim_events;
        syUtilsSetRandomSeed(saved_rng_seed);

        if ((target_fp->status_id == saved_flyroll_select.status_id) &&
            (target_fp->motion_id == saved_flyroll_select.motion_id) &&
            (syUtilsRandSeed() == saved_rng_seed))
        {
            flyroll_mask |= NDS_DAMAGE_FLYROLL_RESTORE;
        }
        gNdsFighterDashRunDamageFlyRollMask = flyroll_mask;
    }

    if (target_root != NULL)
    {
        FTStruct saved_kirby_copy = *target_fp;
        f32 saved_kirby_anim_frame = target_gobj->anim_frame;
        f32 saved_kirby_anim_speed = target_root->anim_speed;
        s32 saved_rng_seed = syUtilsRandSeed();
        u32 saved_kirby_anim_events =
            gNdsFighterDashRunAnimEventsCallCount;
        u32 kirby_helper_mask;
        u32 kirby_mask = 0u;

        gNdsFighterDashRunDamageKirbyCopyMask = 0u;
        gNdsFighterDashRunDamageKirbyCopyBefore = nFTKindNull;
        gNdsFighterDashRunDamageKirbyCopyAfter = nFTKindNull;
        gNdsFighterDashRunDamageKirbyCopyFGM = 0u;

        target_fp->fkind = nFTKindKirby;
        target_fp->ga = nMPKineticsAir;
        target_fp->percent_damage = 0;
        target_fp->passive_vars.kirby.copy_id = nFTKindFox;
        target_fp->passive_vars.kirby.is_ignore_losecopy = FALSE;
        target_fp->status_id = status_before;
        target_fp->motion_id = nFTCommonMotionWait;
        target_fp->status_vars.common.damage.status_id = status_before;
        syUtilsSetRandomSeed(1);

        ftCommonDamageInitDamageVars(target_gobj, -1, 9, 400.0F, 45, 1, 1,
                                     nGMHitElementNormal, 1, FALSE, FALSE,
                                     TRUE);
        kirby_helper_mask = gNdsFighterDashRunDamageKirbyCopyMask;

        if ((gNdsFighterDashRunDamageKirbyCopyBefore != nFTKindKirby) &&
            (gNdsFighterDashRunDamageKirbyCopyBefore != nFTKindNull))
        {
            kirby_mask |= 0x1u;
        }
        if (gNdsFighterDashRunDamageKirbyCopyAfter == nFTKindKirby)
        {
            kirby_mask |= 0x2u;
        }
        if (gNdsFighterDashRunDamageKirbyCopyFGM ==
            nSYAudioFGMKirbySpecialNLoseCopy)
        {
            kirby_mask |= 0x4u;
        }
        if ((kirby_helper_mask & 0x38u) == 0x38u)
        {
            kirby_mask |= 0x8u;
        }
        if (syUtilsRandSeed() != 1)
        {
            kirby_mask |= 0x10u;
        }

        *target_fp = saved_kirby_copy;
        target_gobj->anim_frame = saved_kirby_anim_frame;
        target_root->anim_speed = saved_kirby_anim_speed;
        gNdsFighterDashRunAnimEventsCallCount = saved_kirby_anim_events;
        syUtilsSetRandomSeed(saved_rng_seed);

        if ((target_fp->status_id == saved_kirby_copy.status_id) &&
            (target_fp->motion_id == saved_kirby_copy.motion_id) &&
            (target_fp->fkind == saved_kirby_copy.fkind) &&
            (target_fp->passive_vars.kirby.copy_id ==
                saved_kirby_copy.passive_vars.kirby.copy_id) &&
            (syUtilsRandSeed() == saved_rng_seed))
        {
            kirby_mask |= 0x20u;
        }
        gNdsFighterDashRunDamageKirbyCopyMask = kirby_mask;
    }

    if (target_fp->proc_update != NULL)
    {
        target_fp->proc_update(target_gobj);
    }
    hitstun_after = target_fp->status_vars.common.damage.hitstun_tics;
    if ((hitstun_before > 0) && (hitstun_after == (hitstun_before - 1)))
    {
        FTStruct saved_ground_update = *target_fp;
        f32 saved_ground_update_anim_frame = target_gobj->anim_frame;
        s32 ground_status =
            ndsFTCommonDamageSelectStatus(0, 1, FALSE);

        target_fp->status_id = ground_status;
        target_fp->motion_id = ndsFTCommonDamageMotionForStatus(ground_status);
        target_fp->ga = nMPKineticsGround;
        target_fp->status_vars.common.damage.hitstun_tics = 1;
        target_fp->status_vars.common.damage.public_knockback = 23.0F;
        target_fp->public_knockback = 0.0F;
        target_fp->proc_update = ftCommonDamageCommonProcUpdate;
        target_gobj->anim_frame = 0.0F;

        target_fp->proc_update(target_gobj);
        if ((target_fp->status_id == nFTCommonStatusWait) &&
            (target_fp->motion_id == nFTCommonMotionWait) &&
            (target_fp->ga == nMPKineticsGround) &&
            (target_fp->status_vars.common.damage.hitstun_tics == 0) &&
            (target_fp->public_knockback == 23.0F))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_UPDATE;
        }
        *target_fp = saved_ground_update;
        target_gobj->anim_frame = saved_ground_update_anim_frame;
    }

    if ((target_fp->proc_physics == ftCommonDamageCommonProcPhysics) &&
        (target_fp->ga == nMPKineticsGround) &&
        (target_fp->physics.vel_damage_ground != 0.0F))
    {
        saved_throw_gobj = target_fp->throw_gobj;
        target_fp->throw_gobj = NULL;
        target_fp->physics.vel_ground.x =
            target_fp->physics.vel_damage_ground;
        target_fp->vel_ground = target_fp->physics.vel_ground;
        vel_before_physics =
            ndsFloatToMilliSigned(target_fp->physics.vel_ground.x);
        sNdsFighterDashRunDamagePhysicsActive = TRUE;
        target_fp->proc_physics(target_gobj);
        sNdsFighterDashRunDamagePhysicsActive = FALSE;
        target_fp->throw_gobj = saved_throw_gobj;
        vel_after_physics =
            ndsFloatToMilliSigned(target_fp->physics.vel_ground.x);
        if ((vel_before_physics != 0) &&
            (ABS(vel_after_physics) < ABS(vel_before_physics)))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_PHYSICS;
        }
    }
    else if ((target_fp->proc_physics == ftCommonDamageCommonProcPhysics) &&
             (target_fp->ga == nMPKineticsAir) &&
             (target_fp->physics.vel_damage_air.x != 0.0F))
    {
        saved_throw_gobj = target_fp->throw_gobj;
        target_fp->throw_gobj = NULL;
        target_fp->physics.vel_air = target_fp->physics.vel_damage_air;
        target_fp->vel_air = target_fp->physics.vel_air;
        vel_before_physics =
            ndsFloatToMilliSigned(target_fp->physics.vel_air.x);
        target_fp->proc_physics(target_gobj);
        target_fp->throw_gobj = saved_throw_gobj;
        vel_after_physics =
            ndsFloatToMilliSigned(target_fp->physics.vel_air.x);
        if ((vel_before_physics != 0) &&
            (ABS(vel_after_physics) < ABS(vel_before_physics)))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_PHYSICS;
        }
    }
    else
    {
        vel_before_physics = 0;
        vel_after_physics = 0;
    }

    if ((target_fp->proc_physics == ftCommonDamageCommonProcPhysics) &&
        (target_root != NULL))
    {
        FTStruct saved_flyroll = *target_fp;
        DObj *saved_joint4 = target_fp->joints[4];
        Vec3f saved_joint_rotate;
        s32 expected_pitch;

        if (target_fp->joints[4] == NULL)
        {
            target_fp->joints[4] = target_root;
        }
        saved_joint_rotate = target_fp->joints[4]->rotate.vec.f;
        target_fp->status_id = nFTCommonStatusDamageFlyRoll;
        target_fp->motion_id = nFTCommonMotionDamageFlyRoll;
        target_fp->ga = nMPKineticsAir;
        target_fp->lr = 1;
        target_fp->status_vars.common.damage.hitstun_tics = 1;
        target_fp->physics.vel_air.x = 1.0F;
        target_fp->physics.vel_air.y = 2.0F;
        target_fp->physics.vel_air.z = 0.0F;
        target_fp->physics.vel_damage_air.x = 10.0F;
        target_fp->physics.vel_damage_air.y = 20.0F;
        target_fp->physics.vel_damage_air.z = 0.0F;
        target_fp->throw_gobj = target_gobj;
        target_fp->attack_colls[0].attack_state = nGMAttackStateNew;
        target_fp->attack_colls[0].damage = 7;
        target_fp->proc_physics(target_gobj);
        expected_pitch = ndsFloatToMilliSigned(
            syUtilsArcTan2(target_fp->physics.vel_air.x +
                               target_fp->physics.vel_damage_air.x,
                           target_fp->physics.vel_air.y +
                               target_fp->physics.vel_damage_air.y));
        if ((target_fp->status_id == nFTCommonStatusDamageFlyRoll) &&
            (target_fp->motion_id == nFTCommonMotionDamageFlyRoll) &&
            (ndsFloatToMilliSigned(target_fp->joints[4]->rotate.vec.f.x) ==
                expected_pitch) &&
            (target_fp->attack_colls[0].attack_state ==
                nGMAttackStateOff) &&
            (target_fp->attack_colls[0].damage == 0))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_FLYROLL_PHYSICS;
        }
        target_fp->joints[4]->rotate.vec.f = saved_joint_rotate;
        target_fp->joints[4] = saved_joint4;
        *target_fp = saved_flyroll;
    }

    if ((target_fp->ga == nMPKineticsAir) &&
        (target_fp->proc_map == ftCommonDamageAirCommonProcMap))
    {
        sNdsFighterDashRunDamageAirMapCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionMode = 1u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageMapActive = TRUE;
        target_fp->proc_map(target_gobj);
        sNdsFighterDashRunDamageMapActive = FALSE;
        sNdsFighterDashRunDamageFallMapCollisionMode = 0u;
        if ((target_fp->status_id == selected_status) &&
            (target_fp->motion_id ==
                ndsFTCommonDamageMotionForStatus(selected_status)) &&
            (sNdsFighterDashRunDamageAirMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveStandCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 1u) &&
            ((target_fp->status_vars.common.damage.coll_mask_curr &
              MAP_FLAG_FLOOR) != 0u))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_AIR_MAP;
        }
    }

    if ((target_root != NULL) &&
        (target_fp->proc_map == ftCommonDamageAirCommonProcMap))
    {
        FTStruct saved_air_wall = *target_fp;
        Vec3f saved_air_wall_translate = target_root->translate.vec.f;
        f32 saved_air_wall_anim_frame = target_gobj->anim_frame;
        f32 saved_air_wall_anim_speed = saved_anim_speed;
        u32 air_wall_mask = 0u;

        target_root->translate.vec.f.x = 0.0F;
        target_root->translate.vec.f.y = 0.0F;
        target_root->translate.vec.f.z = 0.0F;
        target_fp->status_id = selected_status;
        target_fp->motion_id = ndsFTCommonDamageMotionForStatus(selected_status);
        target_fp->motion_script_id = target_fp->motion_id;
        target_fp->ga = nMPKineticsAir;
        target_fp->lr = +1;
        target_fp->coll_data.map_coll.width = 30.0F;
        target_fp->coll_data.map_coll.center = 0.0F;
        target_fp->coll_data.map_coll.top = 60.0F;
        target_fp->coll_data.map_coll.bottom = 0.0F;
        target_fp->coll_data.p_map_coll = &target_fp->coll_data.map_coll;
        target_fp->coll_data.lwall_angle.x = 1.0F;
        target_fp->coll_data.lwall_angle.y = 0.0F;
        target_fp->coll_data.lwall_angle.z = 0.0F;
        target_fp->coll_data.mask_curr = 0u;
        target_fp->coll_data.mask_stat = 0u;
        target_fp->status_vars.common.damage.coll_mask_curr = 0u;
        target_fp->status_vars.common.damage.coll_mask_prev = 0u;
        target_fp->status_vars.common.damage.public_knockback = 6400.0F;
        target_fp->physics.vel_air.x = -12.0F;
        target_fp->physics.vel_air.y = 0.0F;
        target_fp->physics.vel_air.z = 0.0F;
        target_fp->physics.vel_damage_air.x = 0.0F;
        target_fp->physics.vel_damage_air.y = 0.0F;
        target_fp->physics.vel_damage_air.z = 0.0F;
        target_fp->proc_map = ftCommonDamageAirCommonProcMap;

        sNdsFighterDashRunDamageAirMapCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionMode = 3u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageMapActive = TRUE;
        target_fp->proc_map(target_gobj);
        sNdsFighterDashRunDamageMapActive = FALSE;
        sNdsFighterDashRunDamageFallMapCollisionMode = 0u;

        if ((sNdsFighterDashRunDamageAirMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            ((target_fp->status_vars.common.damage.coll_mask_curr &
              MAP_FLAG_LWALL) != 0u))
        {
            air_wall_mask |= NDS_DAMAGE_AIR_MAP_WALL_COLLISION;
        }
        if ((target_fp->damage_knockback_stack > 0.0F) &&
            (target_fp->intangible_tics >= FTCOMMON_WALLDAMAGE_INTANGIBLE_TIMER))
        {
            air_wall_mask |= NDS_DAMAGE_AIR_MAP_WALL_HELPER;
        }
        if ((sNdsFighterDashRunDamageFallPassiveStandCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 0u))
        {
            air_wall_mask |= NDS_DAMAGE_AIR_MAP_WALL_SHORT_CIRCUIT;
        }
        if ((target_fp->physics.vel_damage_air.x > 0.0F) &&
            (target_fp->lr == -1))
        {
            air_wall_mask |= NDS_DAMAGE_AIR_MAP_WALL_KNOCKBACK;
        }
        if ((sNdsFighterDashRunDamageAirMapCount == 1u) &&
            ((air_wall_mask & (NDS_DAMAGE_AIR_MAP_WALL_COLLISION |
                               NDS_DAMAGE_AIR_MAP_WALL_HELPER |
                               NDS_DAMAGE_AIR_MAP_WALL_SHORT_CIRCUIT |
                               NDS_DAMAGE_AIR_MAP_WALL_KNOCKBACK)) ==
             (NDS_DAMAGE_AIR_MAP_WALL_COLLISION |
              NDS_DAMAGE_AIR_MAP_WALL_HELPER |
              NDS_DAMAGE_AIR_MAP_WALL_SHORT_CIRCUIT |
              NDS_DAMAGE_AIR_MAP_WALL_KNOCKBACK)))
        {
            air_wall_mask |= NDS_DAMAGE_AIR_MAP_WALL_ORIGINAL;
        }

        *target_fp = saved_air_wall;
        target_root->translate.vec.f = saved_air_wall_translate;
        target_gobj->anim_frame = saved_air_wall_anim_frame;
        target_root->anim_speed = saved_air_wall_anim_speed;
        if ((target_fp->status_id == saved_air_wall.status_id) &&
            (target_fp->motion_id == saved_air_wall.motion_id) &&
            (target_fp->ga == saved_air_wall.ga))
        {
            air_wall_mask |= NDS_DAMAGE_AIR_MAP_WALL_RESTORE;
        }
        gNdsFighterDashRunDamageAirMapWallMask = air_wall_mask;
    }

    if ((target_fp->ga == nMPKineticsAir) &&
        (target_fp->proc_interrupt == ftCommonDamageAirCommonProcInterrupt))
    {
        target_fp->status_vars.common.damage.hitstun_tics = 0;
        target_fp->is_hitstun = TRUE;
        sNdsFighterDashRunDamageFallInterruptCount = 0u;
        sNdsFighterDashRunDamageInterruptActive = TRUE;
        target_fp->proc_interrupt(target_gobj);
        sNdsFighterDashRunDamageInterruptActive = FALSE;
        if ((target_fp->is_hitstun == FALSE) &&
            (sNdsFighterDashRunDamageFallInterruptCount == 1u))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_INTERRUPT;
        }
    }

    {
        FTStruct saved_hammer = *target_fp;

        sNdsFighterDashRunDamageHammerCheckCount = 0u;
        sNdsFighterDashRunDamageHammerGroundCount = 0u;
        sNdsFighterDashRunDamageHammerAirCount = 0u;
        sNdsFighterDashRunDamageHammerHold = TRUE;
        sNdsFighterDashRunDamageHammerCheckActive = TRUE;

        target_fp->ga = nMPKineticsGround;
        target_fp->status_vars.common.damage.hitstun_tics = 0;
        target_fp->is_hitstun = TRUE;
        ftCommonDamageCommonProcInterrupt(target_gobj);

        target_fp->ga = nMPKineticsAir;
        target_fp->status_vars.common.damage.hitstun_tics = 0;
        target_fp->is_hitstun = TRUE;
        ftCommonDamageCommonProcInterrupt(target_gobj);

        sNdsFighterDashRunDamageHammerCheckActive = FALSE;
        sNdsFighterDashRunDamageHammerHold = FALSE;

        if ((sNdsFighterDashRunDamageHammerCheckCount == 2u) &&
            (sNdsFighterDashRunDamageHammerGroundCount == 1u) &&
            (sNdsFighterDashRunDamageHammerAirCount == 1u) &&
            (target_fp->is_hitstun == FALSE))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_HAMMER_INTERRUPT;
        }
        *target_fp = saved_hammer;
        target_gobj->anim_frame = saved_anim_frame;
        if (target_root != NULL)
        {
            target_root->anim_speed = saved_anim_speed;
        }
    }

    installed_status = (u32)target_fp->status_id;
    installed_motion = (u32)target_fp->motion_id;
    installed_ga = (u32)target_fp->ga;

    if ((target_fp->ga == nMPKineticsAir) &&
        (target_fp->proc_update == ftCommonDamageAirCommonProcUpdate))
    {
        target_fp->status_vars.common.damage.hitstun_tics = 1;
        target_fp->status_vars.common.damage.dust_effect_int = 1;
        target_fp->is_hitstun = TRUE;
        target_gobj->anim_frame = 0.0F;
        sNdsFighterDashRunDamageFallSetStatusCount = 0u;
        sNdsFighterDashRunDamageExpiryActive = TRUE;
        target_fp->proc_update(target_gobj);
        sNdsFighterDashRunDamageExpiryActive = FALSE;
        if ((target_fp->status_id == nFTCommonStatusDamageFall) &&
            (target_fp->motion_id == nFTCommonMotionDamageFall) &&
            (target_fp->ga == nMPKineticsAir) &&
            (target_fp->proc_interrupt == ftCommonDamageFallProcInterrupt) &&
            (target_fp->proc_physics == ftPhysicsApplyAirVelDriftFastFall) &&
            (target_fp->proc_map == ftCommonDamageFallProcMap) &&
            (sNdsFighterDashRunDamageFallSetStatusCount == 1u))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_EXPIRE;
        }
        if ((sNdsFighterDashRunDamageSetupDustCount >= 1u) &&
            (sNdsFighterDashRunDamageSetupDustEffectCount == 1u) &&
            (target_fp->status_vars.common.damage.dust_effect_int != 0))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_DUST;
        }
    }

    if ((target_fp->status_id == nFTCommonStatusDamageFall) &&
        (target_fp->proc_interrupt == ftCommonDamageFallProcInterrupt))
    {
        FTStruct saved_fall_interrupt = *target_fp;
        f32 saved_fall_interrupt_anim_frame = target_gobj->anim_frame;
        u32 fall_interrupt_mask = 0u;

        sNdsFighterDashRunDamageFallSourceInterruptCount = 0u;
        sNdsFighterDashRunDamageFallSpecialAirCheckCount = 0u;
        sNdsFighterDashRunDamageFallAttackAirCheckCount = 0u;
        sNdsFighterDashRunDamageFallJumpAerialCheckCount = 0u;
        sNdsFighterDashRunDamageFallHammerCheckCount = 0u;
        sNdsFighterDashRunDamageFallSourceInterruptActive = TRUE;
        target_fp->proc_interrupt(target_gobj);
        sNdsFighterDashRunDamageFallSourceInterruptActive = FALSE;

        if (sNdsFighterDashRunDamageFallSourceInterruptCount == 1u)
        {
            fall_interrupt_mask |= NDS_DAMAGE_FALL_INTERRUPT_CALL;
        }
        if (sNdsFighterDashRunDamageFallSpecialAirCheckCount == 1u)
        {
            fall_interrupt_mask |= NDS_DAMAGE_FALL_INTERRUPT_SPECIAL;
        }
        if (sNdsFighterDashRunDamageFallAttackAirCheckCount == 1u)
        {
            fall_interrupt_mask |= NDS_DAMAGE_FALL_INTERRUPT_ATTACK;
        }
        if (sNdsFighterDashRunDamageFallJumpAerialCheckCount == 1u)
        {
            fall_interrupt_mask |= NDS_DAMAGE_FALL_INTERRUPT_JUMP;
        }
        if (sNdsFighterDashRunDamageFallHammerCheckCount == 1u)
        {
            fall_interrupt_mask |= NDS_DAMAGE_FALL_INTERRUPT_HAMMER;
        }

        *target_fp = saved_fall_interrupt;
        target_gobj->anim_frame = saved_fall_interrupt_anim_frame;
        if ((target_fp->status_id == saved_fall_interrupt.status_id) &&
            (target_fp->proc_interrupt ==
                saved_fall_interrupt.proc_interrupt) &&
            (target_fp->ga == saved_fall_interrupt.ga))
        {
            fall_interrupt_mask |= NDS_DAMAGE_FALL_INTERRUPT_RESTORE;
        }
        gNdsFighterDashRunDamageFallInterruptMask = fall_interrupt_mask;
    }

    fall_vel_y_before = 0;
    fall_vel_y_after = 0;
    fastfall_vel_y_after = 0;
    if ((target_fp->status_id == nFTCommonStatusDamageFall) &&
        (target_fp->proc_physics == ftPhysicsApplyAirVelDriftFastFall))
    {
        target_fp->physics.vel_air.y = 0.0F;
        target_fp->vel_air = target_fp->physics.vel_air;
        target_fp->is_fastfall = FALSE;
        fall_vel_y_before = ndsFloatToMilliSigned(target_fp->physics.vel_air.y);
        sNdsFighterDashRunDamageFallPhysicsCount = 0u;
        sNdsFighterDashRunDamageFallPhysicsActive = TRUE;
        target_fp->proc_physics(target_gobj);
        sNdsFighterDashRunDamageFallPhysicsActive = FALSE;
        fall_vel_y_after = ndsFloatToMilliSigned(target_fp->physics.vel_air.y);
        if ((target_fp->status_id == nFTCommonStatusDamageFall) &&
            (target_fp->motion_id == nFTCommonMotionDamageFall) &&
            (target_fp->ga == nMPKineticsAir) &&
            (sNdsFighterDashRunDamageFallPhysicsCount == 1u) &&
            (fall_vel_y_after < fall_vel_y_before))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_FALL_PHYSICS;
        }
        if (target_fp->attr != NULL)
        {
            target_fp->physics.vel_air.y = -1.0F;
            target_fp->vel_air = target_fp->physics.vel_air;
            target_fp->is_fastfall = FALSE;
            target_fp->tap_stick_y = 0;
            target_fp->input.pl.stick_range.y =
                FTCOMMON_FASTFALL_STICK_RANGE_MIN;
            sNdsFighterDashRunDamageFallPhysicsCount = 0u;
            sNdsFighterDashRunDamageFallPhysicsActive = TRUE;
            target_fp->proc_physics(target_gobj);
            sNdsFighterDashRunDamageFallPhysicsActive = FALSE;
            fastfall_vel_y_after =
                ndsFloatToMilliSigned(target_fp->physics.vel_air.y);
            if ((target_fp->status_id == nFTCommonStatusDamageFall) &&
                (target_fp->motion_id == nFTCommonMotionDamageFall) &&
                (target_fp->ga == nMPKineticsAir) &&
                (target_fp->is_fastfall != FALSE) &&
                (target_fp->tap_stick_y == FTINPUT_STICKBUFFER_TICS_MAX) &&
                (sNdsFighterDashRunDamageFallPhysicsCount == 1u) &&
                (fastfall_vel_y_after ==
                    ndsFloatToMilliSigned(-target_fp->attr->tvel_fast)))
            {
                mask |= NDS_DAMAGE_STATUS_SETUP_FASTFALL;
            }
        }
    }

    if ((target_fp->status_id == nFTCommonStatusDamageFall) &&
        (target_fp->proc_map == ftCommonDamageFallProcMap))
    {
        sNdsFighterDashRunDamageFallMapCollisionMode = 0u;
        sNdsFighterDashRunDamageFallMapCount = 0u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount = 0u;
        sNdsFighterDashRunDamageMapActive = TRUE;
        target_fp->proc_map(target_gobj);
        if ((target_fp->status_id == nFTCommonStatusDamageFall) &&
            (target_fp->motion_id == nFTCommonMotionDamageFall) &&
            (target_fp->ga == nMPKineticsAir) &&
            (sNdsFighterDashRunDamageFallMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapNoCollisionCount == 1u))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_MAP;
        }
        sNdsFighterDashRunDamageFallMapCollisionMode = 1u;
        target_fp->proc_map(target_gobj);
        sNdsFighterDashRunDamageFallMapCollisionMode = 0u;
        if ((target_fp->status_id == nFTCommonStatusDamageFall) &&
            (target_fp->motion_id == nFTCommonMotionDamageFall) &&
            (target_fp->ga == nMPKineticsAir) &&
            (sNdsFighterDashRunDamageFallMapCount == 2u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveStandCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 1u) &&
            ((target_fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_MAP_FLOOR;
        }
        sNdsFighterDashRunDamageFallMapCollisionMode = 2u;
        target_fp->proc_map(target_gobj);
        sNdsFighterDashRunDamageMapActive = FALSE;
        sNdsFighterDashRunDamageFallMapCollisionMode = 0u;
        if ((target_fp->status_id == nFTCommonStatusDamageFall) &&
            (target_fp->motion_id == nFTCommonMotionDamageFall) &&
            (target_fp->ga == nMPKineticsAir) &&
            (sNdsFighterDashRunDamageFallMapCount == 3u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 2u) &&
            (sNdsFighterDashRunDamageFallCliffCatchSetStatusCount == 1u) &&
            ((target_fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK) != 0u))
        {
            mask |= NDS_DAMAGE_STATUS_SETUP_MAP_CLIFF;
        }
    }

    gNdsFighterDashRunDamageSetupMask = mask;
    gNdsFighterDashRunDamageSetupStatusBefore = (u32)status_before;
    gNdsFighterDashRunDamageSetupStatusAfter = installed_status;
    gNdsFighterDashRunDamageSetupMotionAfter = installed_motion;
    gNdsFighterDashRunDamageSetupGAAfter = installed_ga;
    gNdsFighterDashRunDamageSetupHitstunBefore = hitstun_before;
    gNdsFighterDashRunDamageSetupHitstunAfter = hitstun_after;
    gNdsFighterDashRunDamageSetupVelGroundMilli =
        ndsFloatToMilliSigned(target_fp->physics.vel_damage_ground);
    gNdsFighterDashRunDamageSetupVelAirXMilli =
        ndsFloatToMilliSigned(target_fp->physics.vel_damage_air.x);
    gNdsFighterDashRunDamageSetupVelAirYMilli =
        ndsFloatToMilliSigned(target_fp->physics.vel_damage_air.y);
    gNdsFighterDashRunDamageSetupVelPhysicsMilli = vel_after_physics;

    *target_fp = saved_target;
    target_gobj->anim_frame = saved_anim_frame;
    if (target_root != NULL)
    {
        target_root->anim_speed = saved_anim_speed;
    }
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    sNdsFighterDashRunDamagePhysicsActive = FALSE;
    sNdsFighterDashRunDamageInterruptActive = FALSE;
    sNdsFighterDashRunDamageFallSourceInterruptActive = FALSE;
    sNdsFighterDashRunDamageExpiryActive = FALSE;
    sNdsFighterDashRunDamageFallPhysicsActive = FALSE;
    sNdsFighterDashRunDamageMapActive = FALSE;
    sNdsFighterDashRunDamageHammerCheckActive = FALSE;
    sNdsFighterDashRunDamageHammerHold = FALSE;
    sNdsFighterDashRunDamageSetupDustEffectCount = 0u;
    sNdsFighterDashRunDamageFallMapCollisionMode = 0u;
    if (target_fp->status_id == status_before)
    {
        gNdsFighterDashRunDamageSetupMask |=
            NDS_DAMAGE_STATUS_SETUP_RESTORE;
        mask |= NDS_DAMAGE_STATUS_SETUP_RESTORE;
    }

    return ((mask & 0xffffffffu) == 0xffffffffu) ? TRUE : FALSE;
}

static void ndsFighterDashRunProcParamsLagStart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == &sNdsFighterStructPool[0]) && (fp->player == 0))
    {
        sNdsFighterDashRunProcParamsLagStartCount++;
    }
}

static void ndsFighterDashRunDamageLagEnd(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == &sNdsFighterStructPool[0]) && (fp->player == 0))
    {
        sNdsFighterDashRunDamageLagEndCount++;
    }
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainCatch(GObj *fighter_gobj,
                                                        FTStruct *fp,
                                                        GObj *grab_gobj,
                                                        FTStruct *grab_fp)
{
    FTStruct saved_fp;
    FTStruct saved_grab;

    if ((fighter_gobj == NULL) || (fp == NULL) || (grab_gobj == NULL) ||
        (grab_fp == NULL))
    {
        return FALSE;
    }

    saved_fp = *fp;
    saved_grab = *grab_fp;

    fp->catch_gobj = grab_gobj;
    fp->capture_gobj = NULL;
    fp->is_catch_or_capture = FALSE;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_knockback = 10.0F;
    fp->damage_knockback_stack = 100.0F;
    fp->hitlag_tics = 2;
    fp->is_knockback_paused = TRUE;
    fp->damage_lag = 7;
    fp->hitlag_mul = 1.5F;
    fp->damage_kind = nFTDamageKindStatus;

    grab_fp->catch_gobj = NULL;
    grab_fp->capture_gobj = fighter_gobj;
    grab_fp->is_catch_or_capture = TRUE;
    grab_fp->damage_knockback = 20.0F;
    grab_fp->damage_queue = FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD - 1;
    grab_fp->damage_lag = 0;
    grab_fp->hitlag_mul = 1.0F;
    grab_fp->damage_kind = nFTDamageKindDefault;

    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;

    if ((grab_fp->damage_lag == 7) &&
        (grab_fp->hitlag_mul == 1.5F) &&
        (grab_fp->damage_kind == nFTDamageKindColAnim) &&
        (fp->status_id == saved_fp.status_id) &&
        (fp->damage_kind == nFTDamageKindStatus))
    {
        *fp = saved_fp;
        *grab_fp = saved_grab;
        return TRUE;
    }

    *fp = saved_fp;
    *grab_fp = saved_grab;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainRelease(GObj *fighter_gobj,
                                                          FTStruct *fp,
                                                          GObj *grab_gobj,
                                                          FTStruct *grab_fp)
{
    DObj *root;
    DObj *grab_root;
    FTStruct saved_fp;
    FTStruct saved_grab;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    s32 status_before;
    s32 status_after;

    if ((fighter_gobj == NULL) || (fp == NULL) || (grab_gobj == NULL) ||
        (grab_fp == NULL))
    {
        return FALSE;
    }

    root = DObjGetStruct(fighter_gobj);
    grab_root = DObjGetStruct(grab_gobj);
    saved_fp = *fp;
    saved_grab = *grab_fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    if (root != NULL)
    {
        saved_anim_speed = root->anim_speed;
    }

    fp->catch_gobj = grab_gobj;
    fp->capture_gobj = NULL;
    fp->is_catch_or_capture = FALSE;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_angle.x = 0.0F;
    fp->coll_data.floor_angle.y = 1.0F;
    fp->coll_data.floor_angle.z = 0.0F;
    fp->coll_data.floor_line_id = -1;
    fp->coll_data.floor_dist = 1.0F;
    if (root != NULL)
    {
        fp->coll_data.p_translate = &root->translate.vec.f;
    }
    fp->damage_element = nGMHitElementNormal;
    fp->damage_queue = 9;
    fp->damage_knockback = 25.0F;
    fp->damage_knockback_stack = 100.0F;
    fp->damage_angle = 70;
    fp->damage_lr = 1;
    fp->damage_index = 1;
    fp->damage_player_num = grab_fp->player_num;
    fp->hitlag_tics = 2;
    fp->is_knockback_paused = TRUE;
    fp->damage_lag = 7;
    fp->hitlag_mul = 1.0F;
    fp->damage_kind = nFTDamageKindStatus;

    grab_fp->catch_gobj = NULL;
    grab_fp->capture_gobj = fighter_gobj;
    grab_fp->is_catch_or_capture = TRUE;
    grab_fp->ga = nMPKineticsGround;
    grab_fp->coll_data.floor_line_id = -1;
    grab_fp->coll_data.floor_dist = 1.0F;
    if (grab_root != NULL)
    {
        grab_fp->coll_data.p_translate = &grab_root->translate.vec.f;
    }
    grab_fp->damage_knockback = 20.0F;
    grab_fp->damage_queue = FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD;
    grab_fp->damage_kind = nFTDamageKindDefault;

    status_before = fp->status_id;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    status_after = fp->status_id;

    if ((grab_fp->damage_kind == nFTDamageKindStatus) &&
        (status_after != status_before) &&
        (ndsFTCommonDamageIsStatus(status_after) != FALSE) &&
        (fp->is_hitstun != FALSE))
    {
        *fp = saved_fp;
        *grab_fp = saved_grab;
        fighter_gobj->anim_frame = saved_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_anim_speed;
        }
        return TRUE;
    }

    *fp = saved_fp;
    *grab_fp = saved_grab;
    fighter_gobj->anim_frame = saved_anim_frame;
    if (root != NULL)
    {
        root->anim_speed = saved_anim_speed;
    }
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainCatchStats(GObj *fighter_gobj,
                                                             FTStruct *fp,
                                                             GObj *grab_gobj,
                                                             FTStruct *grab_fp)
{
    DObj *root;
    FTStruct saved_fp;
    FTStruct saved_grab;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    FTThrowHitDesc throw_desc[2];
    s32 damage_before;
    s32 status_before;
    s32 status_after;

    if ((fighter_gobj == NULL) || (fp == NULL) || (grab_gobj == NULL) ||
        (grab_fp == NULL))
    {
        return FALSE;
    }

    root = DObjGetStruct(fighter_gobj);
    saved_fp = *fp;
    saved_grab = *grab_fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    if (root != NULL)
    {
        saved_anim_speed = root->anim_speed;
    }

    bzero(throw_desc, sizeof(throw_desc));
    throw_desc[1].damage = 6;

    fp->catch_gobj = grab_gobj;
    fp->capture_gobj = NULL;
    fp->is_catch_or_capture = FALSE;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_angle.x = 0.0F;
    fp->coll_data.floor_angle.y = 1.0F;
    fp->coll_data.floor_angle.z = 0.0F;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_queue = 9;
    fp->damage_knockback = 100.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->damage_angle = 70;
    fp->damage_lr = 1;
    fp->damage_index = 1;
    fp->damage_player_num = grab_fp->player_num;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    fp->damage_lag = 7;
    fp->hitlag_mul = 1.0F;
    fp->damage_kind = nFTDamageKindStatus;
    fp->throw_desc = throw_desc;
    fp->motion_attack_id = 0x3456;
    fp->motion_count = 13;

    grab_fp->catch_gobj = NULL;
    grab_fp->capture_gobj = fighter_gobj;
    grab_fp->is_catch_or_capture = TRUE;
    grab_fp->damage_knockback = 20.0F;
    grab_fp->damage_queue = FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD - 1;
    grab_fp->damage_kind = nFTDamageKindDefault;
    grab_fp->percent_damage = 30;

    damage_before = grab_fp->percent_damage;
    status_before = fp->status_id;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    status_after = fp->status_id;

    if ((grab_fp->percent_damage == (damage_before + 6)) &&
        (grab_fp->damage_kind == nFTDamageKindStatus) &&
        (status_after != status_before) &&
        (ndsFTCommonDamageIsStatus(status_after) != FALSE) &&
        (fp->is_hitstun != FALSE))
    {
        *fp = saved_fp;
        *grab_fp = saved_grab;
        fighter_gobj->anim_frame = saved_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_anim_speed;
        }
        return TRUE;
    }

    *fp = saved_fp;
    *grab_fp = saved_grab;
    fighter_gobj->anim_frame = saved_anim_frame;
    if (root != NULL)
    {
        root->anim_speed = saved_anim_speed;
    }
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainCatchNoDamage(GObj *fighter_gobj,
                                                                FTStruct *fp,
                                                                GObj *grab_gobj,
                                                                FTStruct *grab_fp)
{
    DObj *root;
    DObj *grab_root;
    FTStruct saved_fp;
    FTStruct saved_grab;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    f32 saved_grab_anim_frame;
    f32 saved_grab_anim_speed = 0.0F;
    FTThrowHitDesc throw_desc[2];
    s32 damage_before;
    s32 status_before;
    s32 status_after;

    if ((fighter_gobj == NULL) || (fp == NULL) || (grab_gobj == NULL) ||
        (grab_fp == NULL))
    {
        return FALSE;
    }

    root = DObjGetStruct(fighter_gobj);
    grab_root = DObjGetStruct(grab_gobj);
    if ((root == NULL) || (grab_root == NULL))
    {
        return FALSE;
    }

    saved_fp = *fp;
    saved_grab = *grab_fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    saved_grab_anim_frame = grab_gobj->anim_frame;
    saved_anim_speed = root->anim_speed;
    saved_grab_anim_speed = grab_root->anim_speed;

    bzero(throw_desc, sizeof(throw_desc));
    throw_desc[1].status_id = -1;
    throw_desc[1].damage = 5;
    throw_desc[1].angle = 60;
    throw_desc[1].knockback_base = 30;

    fp->catch_gobj = grab_gobj;
    fp->capture_gobj = NULL;
    fp->is_catch_or_capture = TRUE;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_angle.x = 0.0F;
    fp->coll_data.floor_angle.y = 1.0F;
    fp->coll_data.floor_angle.z = 0.0F;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_queue = 9;
    fp->damage_knockback = 100.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->damage_angle = 70;
    fp->damage_lr = 1;
    fp->damage_index = 1;
    fp->damage_player_num = grab_fp->player_num;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    fp->damage_lag = 7;
    fp->hitlag_mul = 1.0F;
    fp->damage_kind = nFTDamageKindStatus;
    fp->throw_desc = throw_desc;
    fp->motion_attack_id = 0x5678;
    fp->motion_count = 19;
    fp->handicap = 9;

    grab_fp->catch_gobj = NULL;
    grab_fp->capture_gobj = fighter_gobj;
    grab_fp->is_catch_or_capture = TRUE;
    grab_fp->ga = nMPKineticsGround;
    grab_fp->coll_data.floor_angle.x = 0.0F;
    grab_fp->coll_data.floor_angle.y = 1.0F;
    grab_fp->coll_data.floor_angle.z = 0.0F;
    grab_fp->damage_knockback = 0.0F;
    grab_fp->damage_kind = nFTDamageKindDefault;
    grab_fp->percent_damage = 30;
    grab_fp->damage = 30;
    grab_fp->hitstatus = nGMHitStatusNormal;
    grab_fp->special_hitstatus = nGMHitStatusNormal;
    grab_fp->knockback_resist_status = 0.0F;
    grab_fp->knockback_resist_passive = 0.0F;
    grab_fp->handicap = 9;

    damage_before = grab_fp->percent_damage;
    status_before = fp->status_id;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    status_after = fp->status_id;

    if ((grab_fp->percent_damage == (damage_before + 5)) &&
        (grab_fp->capture_gobj == NULL) &&
        (fp->catch_gobj == NULL) &&
        (status_after != status_before) &&
        (ndsFTCommonDamageIsStatus(status_after) != FALSE) &&
        (fp->is_hitstun != FALSE))
    {
        *fp = saved_fp;
        *grab_fp = saved_grab;
        fighter_gobj->anim_frame = saved_anim_frame;
        grab_gobj->anim_frame = saved_grab_anim_frame;
        root->anim_speed = saved_anim_speed;
        grab_root->anim_speed = saved_grab_anim_speed;
        return TRUE;
    }

    *fp = saved_fp;
    *grab_fp = saved_grab;
    fighter_gobj->anim_frame = saved_anim_frame;
    grab_gobj->anim_frame = saved_grab_anim_frame;
    root->anim_speed = saved_anim_speed;
    grab_root->anim_speed = saved_grab_anim_speed;
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainCapture(GObj *fighter_gobj,
                                                          FTStruct *fp,
                                                          GObj *grab_gobj,
                                                          FTStruct *grab_fp)
{
    FTStruct saved_fp;
    FTStruct saved_grab;
    s32 saved_colanim_id;
    s32 saved_colanim_duration;

    if ((fighter_gobj == NULL) || (fp == NULL) || (grab_gobj == NULL) ||
        (grab_fp == NULL))
    {
        return FALSE;
    }

    saved_fp = *fp;
    saved_grab = *grab_fp;
    saved_colanim_id = sNdsFighterDashRunDamageColAnimLastID;
    saved_colanim_duration = sNdsFighterDashRunDamageColAnimLastDuration;

    fp->catch_gobj = NULL;
    fp->capture_gobj = grab_gobj;
    fp->is_catch_or_capture = TRUE;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_queue = FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD - 1;
    fp->damage_knockback = 10.0F;
    fp->damage_lag = 0;
    fp->hitlag_mul = 1.0F;

    grab_fp->catch_gobj = fighter_gobj;
    grab_fp->capture_gobj = NULL;
    grab_fp->is_catch_or_capture = FALSE;
    grab_fp->damage_element = nGMHitElementNormal;
    grab_fp->damage_knockback = 20.0F;
    grab_fp->damage_knockback_stack = 100.0F;
    grab_fp->hitlag_tics = 2;
    grab_fp->is_knockback_paused = TRUE;
    grab_fp->damage_lag = 6;
    grab_fp->hitlag_mul = 1.25F;
    grab_fp->damage_kind = nFTDamageKindDefault;

    sNdsFighterDashRunDamageColAnimLastID = -1;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;

    if ((fp->damage_lag == 6) &&
        (fp->hitlag_mul == 1.25F) &&
        (grab_fp->damage_kind == nFTDamageKindCatch) &&
        (sNdsFighterDashRunDamageColAnimLastID ==
            nGMColAnimFighterDamageCommon) &&
        (fp->status_id == saved_fp.status_id) &&
        (grab_fp->status_id == saved_grab.status_id))
    {
        *fp = saved_fp;
        *grab_fp = saved_grab;
        sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
        sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
        return TRUE;
    }

    *fp = saved_fp;
    *grab_fp = saved_grab;
    sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
    sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainCaptureStats(GObj *fighter_gobj,
                                                               FTStruct *fp,
                                                               GObj *grab_gobj,
                                                               FTStruct *grab_fp)
{
    DObj *root;
    FTStruct saved_fp;
    FTStruct saved_grab;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    FTThrowHitDesc throw_desc[2];
    s32 damage_before;
    s32 status_before;
    s32 status_after;

    if ((fighter_gobj == NULL) || (fp == NULL) || (grab_gobj == NULL) ||
        (grab_fp == NULL))
    {
        return FALSE;
    }

    root = DObjGetStruct(fighter_gobj);
    saved_fp = *fp;
    saved_grab = *grab_fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    if (root != NULL)
    {
        saved_anim_speed = root->anim_speed;
    }

    bzero(throw_desc, sizeof(throw_desc));
    throw_desc[1].damage = 5;

    fp->catch_gobj = NULL;
    fp->capture_gobj = grab_gobj;
    fp->is_catch_or_capture = TRUE;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_angle.x = 0.0F;
    fp->coll_data.floor_angle.y = 1.0F;
    fp->coll_data.floor_angle.z = 0.0F;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_queue = FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD - 1;
    fp->damage_knockback = 10.0F;
    fp->damage_lag = 7;
    fp->damage_angle = 70;
    fp->damage_lr = 1;
    fp->damage_index = 1;
    fp->damage_player_num = grab_fp->player_num;
    fp->damage_kind = nFTDamageKindStatus;
    fp->percent_damage = 20;

    grab_fp->catch_gobj = fighter_gobj;
    grab_fp->capture_gobj = NULL;
    grab_fp->is_catch_or_capture = FALSE;
    grab_fp->damage_element = nGMHitElementNormal;
    grab_fp->damage_knockback = 100.0F;
    grab_fp->damage_knockback_stack = 0.0F;
    grab_fp->hitlag_tics = 0;
    grab_fp->is_knockback_paused = FALSE;
    grab_fp->damage_kind = nFTDamageKindDefault;
    grab_fp->throw_desc = throw_desc;
    grab_fp->motion_attack_id = 0x4567;
    grab_fp->motion_count = 17;

    damage_before = fp->percent_damage;
    status_before = fp->status_id;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    status_after = fp->status_id;

    if ((fp->percent_damage == (damage_before + 5)) &&
        (grab_fp->damage_kind == nFTDamageKindStatus) &&
        (status_after != status_before) &&
        (ndsFTCommonDamageIsStatus(status_after) != FALSE) &&
        (fp->is_hitstun != FALSE))
    {
        *fp = saved_fp;
        *grab_fp = saved_grab;
        fighter_gobj->anim_frame = saved_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_anim_speed;
        }
        return TRUE;
    }

    *fp = saved_fp;
    *grab_fp = saved_grab;
    fighter_gobj->anim_frame = saved_anim_frame;
    if (root != NULL)
    {
        root->anim_speed = saved_anim_speed;
    }
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainCaptureRelease(GObj *fighter_gobj,
                                                                 FTStruct *fp,
                                                                 GObj *grab_gobj,
                                                                 FTStruct *grab_fp)
{
    DObj *root;
    FTStruct saved_fp;
    FTStruct saved_grab;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    s32 damage_before;
    s32 status_before;
    s32 status_after;

    if ((fighter_gobj == NULL) || (fp == NULL) || (grab_gobj == NULL) ||
        (grab_fp == NULL))
    {
        return FALSE;
    }

    root = DObjGetStruct(fighter_gobj);
    saved_fp = *fp;
    saved_grab = *grab_fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    if (root != NULL)
    {
        saved_anim_speed = root->anim_speed;
    }

    fp->catch_gobj = NULL;
    fp->capture_gobj = grab_gobj;
    fp->is_catch_or_capture = TRUE;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_angle.x = 0.0F;
    fp->coll_data.floor_angle.y = 1.0F;
    fp->coll_data.floor_angle.z = 0.0F;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_queue = FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD;
    fp->damage_knockback = 10.0F;
    fp->damage_lag = 7;
    fp->damage_angle = 70;
    fp->damage_lr = 1;
    fp->damage_index = 1;
    fp->damage_player_num = grab_fp->player_num;
    fp->damage_kind = nFTDamageKindStatus;
    fp->percent_damage = 24;

    grab_fp->catch_gobj = fighter_gobj;
    grab_fp->capture_gobj = NULL;
    grab_fp->is_catch_or_capture = FALSE;
    grab_fp->damage_knockback = 100.0F;
    grab_fp->damage_kind = nFTDamageKindDefault;

    damage_before = fp->percent_damage;
    status_before = fp->status_id;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    status_after = fp->status_id;

    if ((fp->percent_damage == damage_before) &&
        (grab_fp->damage_kind == nFTDamageKindStatus) &&
        (status_after != status_before) &&
        (ndsFTCommonDamageIsStatus(status_after) != FALSE) &&
        (fp->is_hitstun != FALSE))
    {
        *fp = saved_fp;
        *grab_fp = saved_grab;
        fighter_gobj->anim_frame = saved_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_anim_speed;
        }
        return TRUE;
    }

    *fp = saved_fp;
    *grab_fp = saved_grab;
    fighter_gobj->anim_frame = saved_anim_frame;
    if (root != NULL)
    {
        root->anim_speed = saved_anim_speed;
    }
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainCaptureNoDamage(GObj *fighter_gobj,
                                                                  FTStruct *fp,
                                                                  GObj *grab_gobj,
                                                                  FTStruct *grab_fp)
{
    DObj *root;
    DObj *grab_root;
    FTStruct saved_fp;
    FTStruct saved_grab;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    f32 saved_grab_anim_frame;
    f32 saved_grab_anim_speed = 0.0F;
    s32 status_before;
    s32 status_after;

    if ((fighter_gobj == NULL) || (fp == NULL) || (grab_gobj == NULL) ||
        (grab_fp == NULL))
    {
        return FALSE;
    }

    root = DObjGetStruct(fighter_gobj);
    grab_root = DObjGetStruct(grab_gobj);
    if ((root == NULL) || (grab_root == NULL))
    {
        return FALSE;
    }

    saved_fp = *fp;
    saved_grab = *grab_fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    saved_grab_anim_frame = grab_gobj->anim_frame;
    saved_anim_speed = root->anim_speed;
    saved_grab_anim_speed = grab_root->anim_speed;

    fp->catch_gobj = NULL;
    fp->capture_gobj = grab_gobj;
    fp->is_catch_or_capture = TRUE;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_angle.x = 0.0F;
    fp->coll_data.floor_angle.y = 1.0F;
    fp->coll_data.floor_angle.z = 0.0F;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_queue = FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD;
    fp->damage_knockback = 12.0F;
    fp->damage_lag = 8;
    fp->damage_angle = 70;
    fp->damage_lr = 1;
    fp->damage_index = 1;
    fp->damage_player_num = grab_fp->player_num;
    fp->damage_kind = nFTDamageKindStatus;

    grab_fp->catch_gobj = fighter_gobj;
    grab_fp->capture_gobj = NULL;
    grab_fp->is_catch_or_capture = FALSE;
    grab_fp->ga = nMPKineticsGround;
    grab_fp->coll_data.floor_angle.x = 0.0F;
    grab_fp->coll_data.floor_angle.y = 1.0F;
    grab_fp->coll_data.floor_angle.z = 0.0F;
    grab_fp->damage_knockback = 0.0F;
    grab_fp->damage_queue = 99;
    grab_fp->percent_damage = 40;
    grab_fp->lr = 1;
    grab_fp->handicap = 9;
    grab_fp->knockback_resist_status = 0.0F;
    grab_fp->knockback_resist_passive = 0.0F;

    status_before = fp->status_id;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    status_after = fp->status_id;

    if ((status_after != status_before) &&
        (ndsFTCommonDamageIsStatus(status_after) != FALSE) &&
        (fp->is_hitstun != FALSE) &&
        (grab_fp->damage_queue == 0) &&
        (grab_fp->damage_knockback > 0.0F) &&
        (grab_fp->damage_player_num == 0) &&
        (grab_fp->status_vars.common.damage.hitstun_tics > 0))
    {
        *fp = saved_fp;
        *grab_fp = saved_grab;
        fighter_gobj->anim_frame = saved_anim_frame;
        grab_gobj->anim_frame = saved_grab_anim_frame;
        root->anim_speed = saved_anim_speed;
        grab_root->anim_speed = saved_grab_anim_speed;
        return TRUE;
    }

    *fp = saved_fp;
    *grab_fp = saved_grab;
    fighter_gobj->anim_frame = saved_anim_frame;
    grab_gobj->anim_frame = saved_grab_anim_frame;
    root->anim_speed = saved_anim_speed;
    grab_root->anim_speed = saved_grab_anim_speed;
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainCatchZero(GObj *fighter_gobj,
                                                            FTStruct *fp,
                                                            GObj *grab_gobj,
                                                            FTStruct *grab_fp)
{
    FTStruct saved_fp;
    FTStruct saved_grab;
    s32 saved_colanim_id;
    s32 saved_colanim_duration;

    if ((fighter_gobj == NULL) || (fp == NULL) || (grab_gobj == NULL) ||
        (grab_fp == NULL))
    {
        return FALSE;
    }

    saved_fp = *fp;
    saved_grab = *grab_fp;
    saved_colanim_id = sNdsFighterDashRunDamageColAnimLastID;
    saved_colanim_duration = sNdsFighterDashRunDamageColAnimLastDuration;

    fp->catch_gobj = grab_gobj;
    fp->capture_gobj = NULL;
    fp->is_catch_or_capture = FALSE;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_knockback = 0.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    fp->damage_kind = nFTDamageKindStatus;

    grab_fp->catch_gobj = NULL;
    grab_fp->capture_gobj = fighter_gobj;
    grab_fp->is_catch_or_capture = TRUE;
    grab_fp->damage_knockback = 0.0F;
    grab_fp->damage_kind = nFTDamageKindDefault;

    sNdsFighterDashRunDamageColAnimLastID = -1;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;

    if ((sNdsFighterDashRunDamageColAnimLastID ==
            nGMColAnimFighterDamageCommon) &&
        (fp->status_id == saved_fp.status_id) &&
        (grab_fp->status_id == saved_grab.status_id) &&
        (grab_fp->damage_kind == nFTDamageKindDefault))
    {
        *fp = saved_fp;
        *grab_fp = saved_grab;
        sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
        sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
        return TRUE;
    }

    *fp = saved_fp;
    *grab_fp = saved_grab;
    sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
    sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainCaptureZero(GObj *fighter_gobj,
                                                              FTStruct *fp,
                                                              GObj *grab_gobj,
                                                              FTStruct *grab_fp)
{
    FTStruct saved_fp;
    FTStruct saved_grab;
    u32 lagstart_before;
    s32 saved_colanim_id;
    s32 saved_colanim_duration;

    if ((fighter_gobj == NULL) || (fp == NULL) || (grab_gobj == NULL) ||
        (grab_fp == NULL))
    {
        return FALSE;
    }

    saved_fp = *fp;
    saved_grab = *grab_fp;
    lagstart_before = sNdsFighterDashRunProcParamsLagStartCount;
    saved_colanim_id = sNdsFighterDashRunDamageColAnimLastID;
    saved_colanim_duration = sNdsFighterDashRunDamageColAnimLastDuration;

    fp->catch_gobj = NULL;
    fp->capture_gobj = grab_gobj;
    fp->is_catch_or_capture = TRUE;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_queue = FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD - 1;
    fp->damage_knockback = 10.0F;
    fp->damage_lag = 9;
    fp->input.pl.button_tap = 0xffffu;
    fp->input.pl.button_release = 0xffffu;
    fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;

    grab_fp->catch_gobj = fighter_gobj;
    grab_fp->capture_gobj = NULL;
    grab_fp->is_catch_or_capture = FALSE;
    grab_fp->status_id = nFTCommonStatusCatchWait;
    grab_fp->damage_knockback = 0.0F;
    grab_fp->hitlag_mul = 1.5F;

    sNdsFighterDashRunDamageColAnimLastID = -1;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;

    if ((grab_fp->hitlag_tics > 0) &&
        (fp->input.pl.button_tap == 0u) &&
        (fp->input.pl.button_release == 0u) &&
        (sNdsFighterDashRunProcParamsLagStartCount ==
            (lagstart_before + 1u)) &&
        (sNdsFighterDashRunDamageColAnimLastID ==
            nGMColAnimFighterDamageCommon) &&
        (fp->status_id == saved_fp.status_id) &&
        (grab_fp->status_id == nFTCommonStatusCatchWait))
    {
        *fp = saved_fp;
        *grab_fp = saved_grab;
        sNdsFighterDashRunProcParamsLagStartCount = lagstart_before;
        sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
        sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
        return TRUE;
    }

    *fp = saved_fp;
    *grab_fp = saved_grab;
    sNdsFighterDashRunProcParamsLagStartCount = lagstart_before;
    sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
    sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainTailColAnim(GObj *fighter_gobj,
                                                              FTStruct *fp)
{
    FTStruct saved_fp;
    s32 saved_colanim_id;
    s32 saved_colanim_duration;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }

    saved_fp = *fp;
    saved_colanim_id = sNdsFighterDashRunDamageColAnimLastID;
    saved_colanim_duration = sNdsFighterDashRunDamageColAnimLastDuration;

    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->item_gobj = NULL;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_knockback = 0.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    fp->damage_kind = nFTDamageKindStatus;

    sNdsFighterDashRunDamageColAnimLastID = -1;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;

    if ((sNdsFighterDashRunDamageColAnimLastID ==
            nGMColAnimFighterDamageCommon) &&
        (fp->status_id == saved_fp.status_id))
    {
        *fp = saved_fp;
        sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
        sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
        return TRUE;
    }

    *fp = saved_fp;
    sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
    sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainTailStatus(GObj *fighter_gobj,
                                                             FTStruct *fp)
{
    DObj *root;
    FTStruct saved_fp;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    s32 status_before;
    s32 status_after;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }

    root = DObjGetStruct(fighter_gobj);
    saved_fp = *fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    if (root != NULL)
    {
        saved_anim_speed = root->anim_speed;
    }

    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->item_gobj = NULL;
    fp->is_catch_or_capture = FALSE;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_angle.x = 0.0F;
    fp->coll_data.floor_angle.y = 1.0F;
    fp->coll_data.floor_angle.z = 0.0F;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_queue = 8;
    fp->damage_knockback = 90.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->damage_angle = 60;
    fp->damage_lr = 1;
    fp->damage_index = 1;
    fp->damage_player_num = 1;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    fp->damage_lag = 7;
    fp->hitlag_mul = 1.0F;
    fp->damage_kind = nFTDamageKindStatus;

    status_before = fp->status_id;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    status_after = fp->status_id;

    if ((status_after != status_before) &&
        (ndsFTCommonDamageIsStatus(status_after) != FALSE) &&
        (fp->is_hitstun != FALSE))
    {
        *fp = saved_fp;
        fighter_gobj->anim_frame = saved_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_anim_speed;
        }
        return TRUE;
    }

    *fp = saved_fp;
    fighter_gobj->anim_frame = saved_anim_frame;
    if (root != NULL)
    {
        root->anim_speed = saved_anim_speed;
    }
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainSleepStatus(
    GObj *fighter_gobj, FTStruct *fp)
{
    DObj *root;
    FTStruct saved_fp;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    u32 colanim_before;
    u32 colanim_after;
    u32 mask = 0u;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }

    root = DObjGetStruct(fighter_gobj);
    saved_fp = *fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    if (root != NULL)
    {
        saved_anim_speed = root->anim_speed;
    }
    colanim_before = sNdsFighterDashRunDamageSetupColAnimCount;

    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->item_gobj = NULL;
    fp->is_catch_or_capture = FALSE;
    fp->is_cliff_hold = TRUE;
    fp->cliffcatch_wait = 0;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_angle.x = 0.0F;
    fp->coll_data.floor_angle.y = 1.0F;
    fp->coll_data.floor_angle.z = 0.0F;
    fp->damage_element = nGMHitElementSleep;
    fp->damage_queue = 5;
    fp->damage_knockback = 0.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->damage_angle = 60;
    fp->damage_lr = 1;
    fp->damage_index = 1;
    fp->damage_player_num = 1;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    fp->damage_lag = 7;
    fp->hitlag_mul = 1.0F;
    fp->damage_kind = nFTDamageKindStatus;

    gNdsFighterDashRunDamageUpdateSleepStatusBefore = (u32)fp->status_id;
    if (fp->damage_element == nGMHitElementSleep)
    {
        mask |= NDS_FTMAIN_DAMAGE_SLEEP_ELEMENT;
    }
    if ((fp->damage_knockback == 0.0F) &&
        (fp->hitlag_tics == 0) &&
        (fp->is_knockback_paused == FALSE))
    {
        mask |= NDS_FTMAIN_DAMAGE_SLEEP_ZERO_KNOCKBACK;
    }

    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;

    colanim_after = sNdsFighterDashRunDamageSetupColAnimCount;
    gNdsFighterDashRunDamageUpdateSleepStatusAfter = (u32)fp->status_id;
    gNdsFighterDashRunDamageUpdateSleepMotionAfter = (u32)fp->motion_id;
    gNdsFighterDashRunDamageUpdateSleepColAnimDelta =
        colanim_after - colanim_before;

    if (gNdsFighterDashRunDamageUpdateSleepColAnimDelta != 0u)
    {
        mask |= NDS_FTMAIN_DAMAGE_SLEEP_FURA_COLANIM;
    }
    if (fp->status_id == nFTCommonStatusFuraSleep)
    {
        mask |= NDS_FTMAIN_DAMAGE_SLEEP_STATUS;
    }
    if (fp->motion_id == nFTCommonMotionFuraSleep)
    {
        mask |= NDS_FTMAIN_DAMAGE_SLEEP_MOTION;
    }
    if (fp->cliffcatch_wait == FTCOMMON_CLIFF_CATCH_WAIT)
    {
        mask |= NDS_FTMAIN_DAMAGE_SLEEP_CLIFF_WAIT;
    }

    *fp = saved_fp;
    fighter_gobj->anim_frame = saved_anim_frame;
    if (root != NULL)
    {
        root->anim_speed = saved_anim_speed;
    }
    if ((fp->status_id == saved_fp.status_id) &&
        (fp->motion_id == saved_fp.motion_id))
    {
        mask |= NDS_FTMAIN_DAMAGE_SLEEP_RESTORE;
    }

    gNdsFighterDashRunDamageUpdateSleepMask = mask;
    return ((mask & 0x7fu) == 0x7fu) ? TRUE : FALSE;
}

static u32 ndsFighterDashRunProbeDamageUpdateMainItemBypassCase(
    GObj *fighter_gobj, FTStruct *fp, s32 fkind, s32 weight)
{
    GObj item_gobj;
    ITStruct item_struct;
    FTStruct saved_fp;
    sb32 saved_active;
    u32 update_before;
    s32 status_before;
    s32 saved_colanim_id;
    s32 saved_colanim_duration;
    u32 mask = 0u;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return 0u;
    }

    saved_fp = *fp;
    saved_active = sNdsFighterDashRunDamageStatusSetupActive;
    update_before = sNdsFighterDashRunDamageRunUpdateColAnimCount;
    status_before = fp->status_id;
    saved_colanim_id = sNdsFighterDashRunDamageColAnimLastID;
    saved_colanim_duration = sNdsFighterDashRunDamageColAnimLastDuration;
    bzero(&item_gobj, sizeof(item_gobj));
    bzero(&item_struct, sizeof(item_struct));
    item_struct.weight = weight;
    item_gobj.user_data.p = &item_struct;

    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->item_gobj = &item_gobj;
    fp->fkind = fkind;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_knockback = 0.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    fp->damage_kind = nFTDamageKindStatus;

    sNdsFighterDashRunDamageColAnimLastID = -1;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = saved_active;

    if ((sNdsFighterDashRunDamageColAnimLastID ==
            nGMColAnimFighterDamageCommon) ||
        (sNdsFighterDashRunDamageRunUpdateColAnimCount > update_before))
    {
        mask |= NDS_DAMAGE_ITEM_BYPASS_TAIL_COLANIM;
    }
    if ((fp->item_gobj == &item_gobj) && (fp->status_id == status_before))
    {
        mask |= NDS_DAMAGE_ITEM_BYPASS_KEEP_ITEM;
    }

    *fp = saved_fp;
    sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
    sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
    sNdsFighterDashRunDamageStatusSetupActive = saved_active;
    if ((fp->item_gobj == saved_fp.item_gobj) &&
        (fp->status_id == saved_fp.status_id))
    {
        mask |= NDS_DAMAGE_ITEM_BYPASS_RESTORE;
    }
    return mask;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainItemBypass(GObj *fighter_gobj,
                                                             FTStruct *fp)
{
    u32 light_mask;
    u32 non_dk_mask;
    u32 mask = 0u;

    gNdsFighterDashRunDamageItemBypassMask = 0u;

    light_mask = ndsFighterDashRunProbeDamageUpdateMainItemBypassCase(
        fighter_gobj, fp, nFTKindGDonkey, nITWeightLight);
    non_dk_mask = ndsFighterDashRunProbeDamageUpdateMainItemBypassCase(
        fighter_gobj, fp, nFTKindMario, nITWeightHeavy);

    if ((light_mask & (NDS_DAMAGE_ITEM_BYPASS_TAIL_COLANIM |
                       NDS_DAMAGE_ITEM_BYPASS_KEEP_ITEM |
                       NDS_DAMAGE_ITEM_BYPASS_RESTORE)) ==
        (NDS_DAMAGE_ITEM_BYPASS_TAIL_COLANIM |
         NDS_DAMAGE_ITEM_BYPASS_KEEP_ITEM |
         NDS_DAMAGE_ITEM_BYPASS_RESTORE))
    {
        mask |= NDS_DAMAGE_ITEM_BYPASS_LIGHT;
    }
    if ((non_dk_mask & (NDS_DAMAGE_ITEM_BYPASS_TAIL_COLANIM |
                        NDS_DAMAGE_ITEM_BYPASS_KEEP_ITEM |
                        NDS_DAMAGE_ITEM_BYPASS_RESTORE)) ==
        (NDS_DAMAGE_ITEM_BYPASS_TAIL_COLANIM |
         NDS_DAMAGE_ITEM_BYPASS_KEEP_ITEM |
         NDS_DAMAGE_ITEM_BYPASS_RESTORE))
    {
        mask |= NDS_DAMAGE_ITEM_BYPASS_HEAVY_NON_DK;
    }
    if (((light_mask & non_dk_mask) & NDS_DAMAGE_ITEM_BYPASS_TAIL_COLANIM) !=
        0u)
    {
        mask |= NDS_DAMAGE_ITEM_BYPASS_TAIL_COLANIM;
    }
    if (((light_mask & non_dk_mask) & NDS_DAMAGE_ITEM_BYPASS_KEEP_ITEM) != 0u)
    {
        mask |= NDS_DAMAGE_ITEM_BYPASS_KEEP_ITEM;
    }
    if (((light_mask & non_dk_mask) & NDS_DAMAGE_ITEM_BYPASS_RESTORE) != 0u)
    {
        mask |= NDS_DAMAGE_ITEM_BYPASS_RESTORE;
    }

    gNdsFighterDashRunDamageItemBypassMask = mask;
    return ((mask & 0x1fu) == 0x1fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainItemResist(GObj *fighter_gobj,
                                                             FTStruct *fp)
{
    GObj item_gobj;
    ITStruct item_struct;
    FTStruct saved_fp;
    s32 saved_colanim_id;
    s32 saved_colanim_duration;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }

    saved_fp = *fp;
    saved_colanim_id = sNdsFighterDashRunDamageColAnimLastID;
    saved_colanim_duration = sNdsFighterDashRunDamageColAnimLastDuration;
    bzero(&item_gobj, sizeof(item_gobj));
    bzero(&item_struct, sizeof(item_struct));
    item_struct.weight = nITWeightHeavy;
    item_gobj.user_data.p = &item_struct;

    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->item_gobj = &item_gobj;
    fp->fkind = nFTKindDonkey;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_knockback = 0.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    fp->damage_kind = nFTDamageKindStatus;

    sNdsFighterDashRunDamageColAnimLastID = -1;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;

    if ((sNdsFighterDashRunDamageColAnimLastID ==
            nGMColAnimFighterDamageCommon) &&
        (fp->item_gobj == &item_gobj) &&
        (fp->status_id == saved_fp.status_id))
    {
        *fp = saved_fp;
        sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
        sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
        return TRUE;
    }

    *fp = saved_fp;
    sNdsFighterDashRunDamageColAnimLastID = saved_colanim_id;
    sNdsFighterDashRunDamageColAnimLastDuration = saved_colanim_duration;
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainItemDrop(GObj *fighter_gobj,
                                                           FTStruct *fp)
{
    GObj item_gobj;
    ITStruct item_struct;
    DObj *root;
    FTStruct saved_fp;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    s32 status_before;
    s32 status_after;

    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        return FALSE;
    }

    root = DObjGetStruct(fighter_gobj);
    saved_fp = *fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    if (root != NULL)
    {
        saved_anim_speed = root->anim_speed;
    }
    bzero(&item_gobj, sizeof(item_gobj));
    bzero(&item_struct, sizeof(item_struct));
    item_struct.weight = nITWeightHeavy;
    item_gobj.user_data.p = &item_struct;

    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->item_gobj = &item_gobj;
    fp->is_catch_or_capture = FALSE;
    fp->fkind = nFTKindGDonkey;
    fp->ga = nMPKineticsGround;
    fp->coll_data.floor_angle.x = 0.0F;
    fp->coll_data.floor_angle.y = 1.0F;
    fp->coll_data.floor_angle.z = 0.0F;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_queue = 8;
    fp->damage_knockback = 90.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->damage_angle = 60;
    fp->damage_lr = 1;
    fp->damage_index = 1;
    fp->damage_player_num = 1;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    fp->damage_lag = 7;
    fp->hitlag_mul = 1.0F;
    fp->damage_kind = nFTDamageKindStatus;

    status_before = fp->status_id;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageUpdateMain(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    status_after = fp->status_id;

    if ((fp->item_gobj == NULL) &&
        (status_after != status_before) &&
        (ndsFTCommonDamageIsStatus(status_after) != FALSE) &&
        (fp->is_hitstun != FALSE))
    {
        *fp = saved_fp;
        fighter_gobj->anim_frame = saved_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_anim_speed;
        }
        return TRUE;
    }

    *fp = saved_fp;
    fighter_gobj->anim_frame = saved_anim_frame;
    if (root != NULL)
    {
        root->anim_speed = saved_anim_speed;
    }
    sNdsFighterDashRunDamageStatusSetupActive = FALSE;
    return FALSE;
}

static sb32 ndsFighterDashRunProbeDamageUpdateMainItemHeavy(GObj *fighter_gobj,
                                                            FTStruct *fp)
{
    GObj *saved_item_gobj;
    s32 saved_status;
    u32 mask = 0u;

    if (fp == NULL)
    {
        return FALSE;
    }

    saved_item_gobj = fp->item_gobj;
    saved_status = fp->status_id;
    gNdsFighterDashRunDamageItemHeavyMask = 0u;

    if (ndsFighterDashRunProbeDamageUpdateMainItemResist(fighter_gobj,
                                                         fp) != FALSE)
    {
        mask |= NDS_DAMAGE_ITEM_HEAVY_BRANCH |
                NDS_DAMAGE_ITEM_HEAVY_RESIST |
                NDS_DAMAGE_ITEM_HEAVY_RETURN;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainItemDrop(fighter_gobj,
                                                       fp) != FALSE)
    {
        mask |= NDS_DAMAGE_ITEM_HEAVY_BRANCH |
                NDS_DAMAGE_ITEM_HEAVY_DROP |
                NDS_DAMAGE_ITEM_HEAVY_RETURN;
    }
    if ((fp->item_gobj == saved_item_gobj) && (fp->status_id == saved_status))
    {
        mask |= NDS_DAMAGE_ITEM_HEAVY_RESTORE;
    }

    gNdsFighterDashRunDamageItemHeavyMask = mask;
    return ((mask & 0x1fu) == 0x1fu) ? TRUE : FALSE;
}

static void ndsFighterDashRunProcParamsHit(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == &sNdsFighterStructPool[1]) && (fp->player == 1))
    {
        sNdsFighterDashRunProcParamsHitCount++;
    }
}

static void ndsFighterDashRunProcParamsShield(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == &sNdsFighterStructPool[1]) && (fp->player == 1))
    {
        sNdsFighterDashRunProcParamsShieldCount++;
    }
}

static void ndsFighterDashRunProcParamsTrap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == &sNdsFighterStructPool[0]) && (fp->player == 0))
    {
        sNdsFighterDashRunProcParamsTrapCount++;
    }
}

static void ndsFighterMarioFoxStageMPLiveHitDamageLoopShieldLagStart(
    GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ndsFighterStructIsPoolPointer(fp) != FALSE)
    {
        sNdsStageMPLiveHitDamageLoopShieldLagStartCount++;
    }
}

#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_IS_SHIELD (1u << 0)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETECT    (1u << 1)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_ACTIVE    (1u << 2)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_JOINT     (1u << 3)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SPHERE    (1u << 4)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_STATS     (1u << 5)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_CLEAR     (1u << 6)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_RESTORE   (1u << 7)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_OFF  (1u << 8)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_DET  (1u << 9)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_HIT  (1u << 10)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_REST (1u << 11)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETOFF    (1u << 12)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETOFF_HIT (1u << 13)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETOFF_REST (1u << 14)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_HEALTH    (1u << 15)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_BREAK     (1u << 16)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_HEAL      (1u << 17)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_BREAK_CLEAR (1u << 18)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_TAIL_CLEAR (1u << 19)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SPECIAL_CLEAR (1u << 20)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_HITLAG_MUL_CLEAR (1u << 21)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_REPEAT    (1u << 22)

static u32
ndsFighterMarioFoxStageMPLiveHitDamageLoopProbeShieldSetOffTick(
    GObj *victim_gobj, FTStruct *victim_fp)
{
    static FTStruct saved_victim;
    DObj *victim_root;
    f32 saved_anim_frame;
    f32 saved_anim_speed;
    sb32 is_root_saved;
    sb32 saved_guard_on_active;
    u32 mask;

    if ((victim_gobj == NULL) || (victim_fp == NULL))
    {
        return 0u;
    }

    victim_root = DObjGetStruct(victim_gobj);
    saved_victim = *victim_fp;
    saved_anim_frame = victim_gobj->anim_frame;
    saved_anim_speed = 0.0F;
    is_root_saved = FALSE;
    mask = 0u;
    if (victim_root != NULL)
    {
        saved_anim_speed = victim_root->anim_speed;
        is_root_saved = TRUE;
    }
    saved_guard_on_active = sNdsFighterDashRunGuardOnActive;

    victim_fp->status_id = nFTCommonStatusGuardSetOff;
    victim_fp->motion_id = nFTCommonMotionGuardOn;
    victim_fp->motion_script_id = nFTCommonMotionGuardOn;
    victim_fp->proc_update = ndsBaseFTCommonGuardSetOffProcUpdate;
    victim_fp->input.pl.button_hold = victim_fp->input.button_mask_z;
    victim_fp->status_vars.common.guard.is_release = FALSE;
    victim_fp->status_vars.common.guard.setoff_frames = 2.0F;
    victim_fp->is_shield = TRUE;

    sNdsFighterDashRunGuardOnActive = TRUE;
    ndsBaseFTCommonGuardSetOffProcUpdate(victim_gobj);
    sNdsFighterDashRunGuardOnActive = saved_guard_on_active;

    gNdsStageMPLiveHitDamageLoopShieldSetOffTickStatusHeld =
        (u32)victim_fp->status_id;
    gNdsStageMPLiveHitDamageLoopShieldSetOffTickFramesMilli =
        ndsFloatToMilliSigned(
            victim_fp->status_vars.common.guard.setoff_frames);

    if ((victim_fp->status_id == nFTCommonStatusGuardSetOff) &&
        (victim_fp->motion_id == nFTCommonMotionGuardOn))
    {
        mask |= 1u;
    }
    if ((gNdsStageMPLiveHitDamageLoopShieldSetOffTickFramesMilli == 1000) &&
        (victim_fp->status_vars.common.guard.is_release == FALSE))
    {
        mask |= 1u << 1u;
    }

    *victim_fp = saved_victim;
    victim_gobj->anim_frame = saved_anim_frame;
    if (is_root_saved != FALSE)
    {
        victim_root->anim_speed = saved_anim_speed;
    }

    victim_fp->status_id = nFTCommonStatusGuardSetOff;
    victim_fp->motion_id = nFTCommonMotionGuardOn;
    victim_fp->motion_script_id = nFTCommonMotionGuardOn;
    victim_fp->proc_update = ndsBaseFTCommonGuardSetOffProcUpdate;
    victim_fp->input.pl.button_hold = 0u;
    victim_fp->status_vars.common.guard.is_release = FALSE;
    victim_fp->status_vars.common.guard.setoff_frames = 1.0F;
    victim_fp->is_shield = TRUE;

    sNdsFighterDashRunGuardOnActive = TRUE;
    ndsBaseFTCommonGuardSetOffProcUpdate(victim_gobj);
    sNdsFighterDashRunGuardOnActive = saved_guard_on_active;

    gNdsStageMPLiveHitDamageLoopShieldSetOffTickStatusRelease =
        (u32)victim_fp->status_id;
    if ((victim_fp->status_id == nFTCommonStatusGuardOff) &&
        (victim_fp->motion_id == nFTCommonMotionGuardOff))
    {
        mask |= 1u << 2u;
    }
    if (victim_fp->proc_update == ndsBaseFTCommonGuardOffProcUpdate)
    {
        mask |= 1u << 3u;
    }

    *victim_fp = saved_victim;
    victim_gobj->anim_frame = saved_anim_frame;
    if (is_root_saved != FALSE)
    {
        victim_root->anim_speed = saved_anim_speed;
    }
    if ((victim_fp->status_id == saved_victim.status_id) &&
        (victim_fp->motion_id == saved_victim.motion_id))
    {
        mask |= 1u << 4u;
    }
    return mask;
}

static sb32 ndsFighterMarioFoxStageMPLiveHitDamageLoopRunShieldStatProof(
    FTStruct *attacker_fp, FTAttackColl *attack_coll, FTStruct *victim_fp,
    GObj *attacker_gobj, GObj *victim_gobj, u32 attack_id)
{
    static FTStruct saved_attacker;
    static FTStruct saved_victim;
    DObj *attacker_root;
    DObj *victim_root;
    DObj *shield_joint;
    GObj *saved_fighter_link_head;
    GObj *saved_attacker_link_next;
    FTParts *shield_parts;
    Vec3f shield_center;
    Vec3f shield_delta;
    f32 attacker_x_saved;
    f32 victim_x_saved;
    f32 victim_anim_frame_saved;
    f32 victim_anim_speed_saved;
    f32 shield_radius_x;
    f32 shield_radius_y;
    f32 shield_radius_z;
    f32 shield_dist;
    f32 shield_angle;
    sb32 is_attacker_x_saved;
    sb32 is_victim_root_saved;
    u32 saved_lagstart_count;
    u32 saved_guard_setoff_count;
    u32 saved_guard_setoff_ftmain_count;
    u32 saved_guard_setoff_mask;
    u32 saved_guard_setoff_callback_mask;
    u32 saved_anim_events_count;
    u32 saved_last_fgm;
    s32 saved_guard_setoff_frames;
    s32 saved_guard_setoff_vel;
    sb32 saved_guard_on_active;
    s32 damage;
    s32 shield_damage;
    s32 expected_total;
    s32 expected_lr;
    s32 hitlag;
    s32 shield_break_hitlag;
    s32 attack_push_after;
    s32 shield_damage_after;
    s32 shield_damage_total_after;
    u32 contact_mask;
    u32 collision_count_before_skip;
    u32 hit_count_before_skip;
    u32 shield_collision_count_after;
    u32 shield_effect_count_after;
    u32 shield_hit_count_after;
    u32 i;

    if ((attacker_fp == NULL) || (attack_coll == NULL) ||
        (victim_fp == NULL) || (attacker_gobj == NULL) ||
        (victim_gobj == NULL) || (attack_coll->damage <= 0) ||
        (attack_id >= FTATTACKCOLL_NUM_MAX))
    {
        return FALSE;
    }

    attacker_root = DObjGetStruct(attacker_gobj);
    victim_root = DObjGetStruct(victim_gobj);
    is_attacker_x_saved = FALSE;
    is_victim_root_saved = FALSE;
    attacker_x_saved = 0.0F;
    victim_x_saved = 0.0F;
    victim_anim_frame_saved = victim_gobj->anim_frame;
    victim_anim_speed_saved = 0.0F;
    if (attacker_root != NULL)
    {
        attacker_x_saved = attacker_root->translate.vec.f.x;
        attacker_root->translate.vec.f.x = 40.0F;
        is_attacker_x_saved = TRUE;
    }
    if (victim_root != NULL)
    {
        victim_x_saved = victim_root->translate.vec.f.x;
        victim_anim_speed_saved = victim_root->anim_speed;
        victim_root->translate.vec.f.x = -40.0F;
        is_victim_root_saved = TRUE;
    }

    saved_attacker = *attacker_fp;
    saved_victim = *victim_fp;
    saved_lagstart_count =
        sNdsStageMPLiveHitDamageLoopShieldLagStartCount;
    saved_guard_setoff_count = gNdsFighterDashRunGuardSetOffSetStatusCount;
    saved_guard_setoff_ftmain_count =
        gNdsFighterDashRunFtMainGuardSetOffStatusCount;
    saved_guard_setoff_mask = gNdsFighterDashRunGuardSetOffMask;
    saved_guard_setoff_callback_mask =
        gNdsFighterDashRunGuardSetOffCallbackMask;
    saved_anim_events_count = gNdsFighterDashRunAnimEventsCallCount;
    saved_last_fgm = gNdsSCVSBattleLastFGM;
    saved_guard_setoff_frames = gNdsFighterDashRunGuardSetOffFramesMilli;
    saved_guard_setoff_vel = gNdsFighterDashRunGuardSetOffVelMilli;
    saved_guard_on_active = sNdsFighterDashRunGuardOnActive;
    saved_fighter_link_head = gGCCommonLinks[nGCCommonLinkIDFighter];
    saved_attacker_link_next =
        (attacker_gobj != NULL) ? attacker_gobj->link_next : NULL;

    damage = attack_coll->damage;
    shield_damage = attack_coll->shield_damage;
    expected_total = damage + shield_damage;
    expected_lr = ((victim_root != NULL) && (attacker_root != NULL) &&
                   (victim_root->translate.vec.f.x <
                    attacker_root->translate.vec.f.x)) ? 1 : -1;
    contact_mask = 0u;

    victim_fp->is_shield = FALSE;
    victim_fp->shield_health = 54;
    victim_fp->shield_heal_wait = 1.0F;
    victim_fp->shield_damage_total = 0;
    if (!(victim_fp->is_shield) && (victim_fp->shield_health < 55))
    {
        victim_fp->shield_heal_wait--;
        if (victim_fp->shield_heal_wait == 0.0F)
        {
            victim_fp->shield_health++;
            victim_fp->shield_heal_wait = 10.0F;
        }
    }
    if ((victim_fp->shield_health == 55) &&
        (victim_fp->shield_heal_wait == 10.0F))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_HEAL;
    }

    attacker_fp->attack_shield_push = 0;
    victim_fp->status_id = nFTCommonStatusGuard;
    victim_fp->motion_id = nFTCommonMotionGuardOn;
    victim_fp->motion_script_id = nFTCommonMotionGuardOn;
    victim_fp->lr = 1;
    victim_fp->shield_lr = 0;
    victim_fp->shield_health = 55;
    victim_fp->shield_damage = 0;
    victim_fp->shield_damage_total = 0;
    victim_fp->hitlag_tics = 0;
    victim_fp->hitlag_mul = 1.0F;
    victim_fp->is_shield = TRUE;
    if ((victim_fp->joints[nFTPartsJointYRotN] == NULL) &&
        (victim_root != NULL))
    {
        victim_fp->joints[nFTPartsJointYRotN] = victim_root;
    }
    victim_fp->input.pl.button_tap = 0xffffu;
    victim_fp->input.pl.button_release = 0xffffu;

    shield_angle = 0.0F;
    shield_center.x = 0.0F;
    shield_center.y = 0.0F;
    shield_center.z = 0.0F;
    if (victim_fp->is_shield != FALSE)
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_IS_SHIELD;
    }
    gFTMainIsDamageDetect[attack_id] = TRUE;
    gNdsStageMPLiveHitDamageLoopShieldContactDetectBefore =
        (gFTMainIsDamageDetect[attack_id] != FALSE) ? 1u : 0u;
    if (gFTMainIsDamageDetect[attack_id] != FALSE)
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETECT;
    }
    if (attack_coll->attack_state != nGMAttackStateOff)
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_ACTIVE;
    }
    shield_joint = victim_fp->joints[nFTPartsJointYRotN];
    shield_parts = (shield_joint != NULL) ? ftGetParts(shield_joint) : NULL;
    if ((shield_parts != NULL) &&
        (shield_parts->vec_scale.x != 0.0F) &&
        (shield_parts->vec_scale.y != 0.0F) &&
        (shield_parts->vec_scale.z != 0.0F))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_JOINT;
        gmCollisionGetWorldPosition(shield_parts->mtx_translate,
                                    &shield_center);
        shield_radius_x = 30.0F + (attack_coll->size /
                                   shield_parts->vec_scale.x);
        shield_radius_y = 30.0F + (attack_coll->size /
                                   shield_parts->vec_scale.y);
        shield_radius_z = 30.0F + (attack_coll->size /
                                   shield_parts->vec_scale.z);
        if ((shield_radius_x > 0.0F) && (shield_radius_y > 0.0F) &&
            (shield_radius_z > 0.0F))
        {
            shield_delta.x = 0.0F;
            shield_delta.y = 0.0F;
            shield_delta.z = 0.0F;
            shield_dist =
                (SQUARE(shield_delta.x / shield_radius_x) +
                 SQUARE(shield_delta.y / shield_radius_y) +
                 SQUARE(shield_delta.z / shield_radius_z));
            gNdsStageMPLiveHitDamageLoopShieldContactCollisionCount++;
            if (shield_dist <= 1.0F)
            {
                shield_angle = F_CLC_DTOR32(180.0F);
                contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SPHERE;
                gNdsStageMPLiveHitDamageLoopShieldContactHitCount++;
            }
        }
    }

    collision_count_before_skip =
        gNdsStageMPLiveHitDamageLoopShieldContactCollisionCount;
    hit_count_before_skip =
        gNdsStageMPLiveHitDamageLoopShieldContactHitCount;
    victim_fp->is_shield = FALSE;
    gFTMainIsDamageDetect[attack_id] = TRUE;
    if (victim_fp->is_shield == FALSE)
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_OFF;
    }
    if (gFTMainIsDamageDetect[attack_id] != FALSE)
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_DET;
    }
    if ((gNdsStageMPLiveHitDamageLoopShieldContactCollisionCount ==
            collision_count_before_skip) &&
        (gNdsStageMPLiveHitDamageLoopShieldContactHitCount ==
            hit_count_before_skip))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_HIT;
    }
    victim_fp->is_shield = TRUE;
    if (victim_fp->is_shield != FALSE)
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_REST;
    }

    collision_count_before_skip =
        gNdsStageMPLiveHitDamageLoopShieldContactCollisionCount;
    hit_count_before_skip =
        gNdsStageMPLiveHitDamageLoopShieldContactHitCount;
    gFTMainIsDamageDetect[attack_id] = FALSE;
    if ((victim_fp->is_shield != FALSE) &&
        (gFTMainIsDamageDetect[attack_id] == FALSE))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETOFF;
    }
    if ((gNdsStageMPLiveHitDamageLoopShieldContactCollisionCount ==
            collision_count_before_skip) &&
        (gNdsStageMPLiveHitDamageLoopShieldContactHitCount ==
            hit_count_before_skip))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETOFF_HIT;
    }
    gFTMainIsDamageDetect[attack_id] = TRUE;
    if ((victim_fp->is_shield != FALSE) &&
        (gFTMainIsDamageDetect[attack_id] != FALSE))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETOFF_REST;
    }

    gNdsStageMPLiveHitDamageLoopShieldDamageBefore =
        victim_fp->shield_damage;
    gNdsStageMPLiveHitDamageLoopShieldEffectCount = 0u;
    gNdsStageMPLiveHitDamageLoopShieldEffectSize = 0;
    victim_fp->capture_gobj = NULL;
    victim_fp->ga = nMPKineticsGround;
    attacker_fp->is_catch_or_capture = FALSE;
    attacker_fp->throw_gobj = NULL;
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        if (i != attack_id)
        {
            attacker_fp->attack_colls[i].attack_state = nGMAttackStateOff;
        }
    }
    attack_coll->attack_state = nGMAttackStateInterpolate;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->is_hit_air = TRUE;
    attack_coll->pos_curr = shield_center;
    attack_coll->pos_prev = shield_center;
    ftParamClearAttackRecordID(attacker_fp, (s32)attack_id);
    gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
    attacker_gobj->link_next = NULL;
    ftMainSearchHitFighter(victim_gobj);
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;

    gNdsStageMPLiveHitDamageLoopShieldAttackPushAfter =
        attacker_fp->attack_shield_push;
    gNdsStageMPLiveHitDamageLoopShieldDamageAfter =
        victim_fp->shield_damage;
    gNdsStageMPLiveHitDamageLoopShieldDamageTotalAfter =
        victim_fp->shield_damage_total;
    gNdsStageMPLiveHitDamageLoopShieldLR = victim_fp->shield_lr;
    if ((attacker_fp->attack_shield_push == damage) &&
        (victim_fp->shield_damage == damage) &&
        (victim_fp->shield_damage_total == expected_total) &&
        (victim_fp->shield_lr == expected_lr) &&
        (gNdsStageMPLiveHitDamageLoopShieldPlayer ==
            (s32)attacker_fp->player) &&
        (gNdsStageMPLiveHitDamageLoopShieldEffectSize == damage))
    {
        gNdsStageMPLiveHitDamageLoopShieldStatCount = 1u;
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_STATS;
    }
    if (gFTMainIsDamageDetect[attack_id] == FALSE)
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_CLEAR;
    }
    attack_push_after = attacker_fp->attack_shield_push;
    shield_damage_after = victim_fp->shield_damage;
    shield_damage_total_after = victim_fp->shield_damage_total;
    shield_effect_count_after =
        gNdsStageMPLiveHitDamageLoopShieldEffectCount;
    shield_collision_count_after =
        gNdsStageMPLiveHitDamageLoopShieldContactCollisionCount;
    shield_hit_count_after =
        gNdsStageMPLiveHitDamageLoopShieldContactHitCount;
    gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
    attacker_gobj->link_next = NULL;
    ftMainSearchHitFighter(victim_gobj);
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    if ((attack_coll->attack_records[0].victim_gobj == victim_gobj) &&
        (attack_coll->attack_records[0].victim_flags.is_interact_shield !=
            FALSE) &&
        (gFTMainIsDamageDetect[attack_id] == FALSE) &&
        (attacker_fp->attack_shield_push == attack_push_after) &&
        (victim_fp->shield_damage == shield_damage_after) &&
        (victim_fp->shield_damage_total == shield_damage_total_after) &&
        (gNdsStageMPLiveHitDamageLoopShieldEffectCount ==
            shield_effect_count_after) &&
        (gNdsStageMPLiveHitDamageLoopShieldContactCollisionCount ==
            shield_collision_count_after) &&
        (gNdsStageMPLiveHitDamageLoopShieldContactHitCount ==
            shield_hit_count_after))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_REPEAT;
    }
    victim_fp->shield_health -= victim_fp->shield_damage_total;
    if (victim_fp->shield_health == (55 - expected_total))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_HEALTH;
    }

    sNdsFighterDashRunGuardOnActive = TRUE;
    ndsBaseFTCommonGuardSetOffSetStatus(victim_gobj);
    sNdsFighterDashRunGuardOnActive = saved_guard_on_active;
    victim_fp->proc_lagstart =
        ndsFighterMarioFoxStageMPLiveHitDamageLoopShieldLagStart;
    victim_fp->is_knockback_paused = FALSE;
    victim_fp->attack_damage = damage + 1;
    victim_fp->attack_shield_push = damage + 2;
    victim_fp->damage_lag = damage + 3;
    victim_fp->damage_queue = damage + 4;
    victim_fp->damage_kind = nFTDamageKindStatus;
    victim_fp->reflect_lr = 1;
    victim_fp->reflect_damage = damage + 5;
    victim_fp->absorb_lr = -1;
    victim_fp->attack_rebound = 1.0F;
    victim_fp->damage_knockback = 2.0F;
    victim_fp->hitlag_mul = 2.0F;

    if (victim_fp->shield_damage != 0)
    {
        hitlag = ftParamGetHitLag(victim_fp->shield_damage,
                                  nFTCommonStatusGuard,
                                  victim_fp->hitlag_mul);
        victim_fp->hitlag_tics = hitlag;
        victim_fp->input.pl.button_tap = 0;
        victim_fp->input.pl.button_release = 0;
        if (victim_fp->proc_lagstart != NULL)
        {
            victim_fp->proc_lagstart(victim_gobj);
        }
        victim_fp->shield_damage = 0;
        victim_fp->shield_damage_total = 0;
    }
    else
    {
        hitlag = 0;
    }
    victim_fp->attack_damage = 0;
    victim_fp->attack_shield_push = 0;
    victim_fp->damage_lag = 0;
    victim_fp->damage_queue = 0;
    victim_fp->damage_kind = nFTDamageKindDefault;
    victim_fp->reflect_lr = 0;
    victim_fp->reflect_damage = 0;
    victim_fp->absorb_lr = 0;
    victim_fp->attack_rebound = 0.0F;
    victim_fp->damage_knockback = 0.0F;
    victim_fp->hitlag_mul = 1.0F;

    gNdsStageMPLiveHitDamageLoopShieldSetOffStatusAfter =
        (u32)victim_fp->status_id;
    gNdsStageMPLiveHitDamageLoopShieldSetOffMotionAfter =
        victim_fp->motion_id;
    gNdsStageMPLiveHitDamageLoopShieldSetOffHitlag = hitlag;
    if (victim_fp->status_id == nFTCommonStatusGuardSetOff)
    {
        gNdsStageMPLiveHitDamageLoopShieldSetOffClearMask |= 1u;
    }
    if (victim_fp->motion_id == nFTCommonMotionGuardOn)
    {
        gNdsStageMPLiveHitDamageLoopShieldSetOffClearMask |= 1u << 1u;
    }
    if ((hitlag > 0) && (victim_fp->hitlag_tics == hitlag))
    {
        gNdsStageMPLiveHitDamageLoopShieldSetOffClearMask |= 1u << 2u;
    }
    if ((victim_fp->input.pl.button_tap == 0u) &&
        (victim_fp->input.pl.button_release == 0u))
    {
        gNdsStageMPLiveHitDamageLoopShieldSetOffClearMask |= 1u << 3u;
    }
    if ((victim_fp->shield_damage == 0) &&
        (victim_fp->shield_damage_total == 0))
    {
        gNdsStageMPLiveHitDamageLoopShieldSetOffClearMask |= 1u << 4u;
    }
    if (sNdsStageMPLiveHitDamageLoopShieldLagStartCount ==
        (saved_lagstart_count + 1u))
    {
        gNdsStageMPLiveHitDamageLoopShieldSetOffClearMask |= 1u << 5u;
    }
    gNdsStageMPLiveHitDamageLoopShieldSetOffTickMask =
        ndsFighterMarioFoxStageMPLiveHitDamageLoopProbeShieldSetOffTick(
            victim_gobj, victim_fp);
    if ((victim_fp->is_knockback_paused == FALSE) &&
        (victim_fp->attack_damage == 0) &&
        (victim_fp->attack_shield_push == 0) &&
        (victim_fp->damage_lag == 0) &&
        (victim_fp->damage_queue == 0) &&
        (victim_fp->damage_kind == nFTDamageKindDefault))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_TAIL_CLEAR;
    }
    if ((victim_fp->reflect_lr == 0) &&
        (victim_fp->reflect_damage == 0) &&
        (victim_fp->absorb_lr == 0) &&
        (victim_fp->attack_rebound == 0.0F) &&
        (victim_fp->damage_knockback == 0.0F))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SPECIAL_CLEAR;
    }
    if (victim_fp->hitlag_mul == 1.0F)
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_HITLAG_MUL_CLEAR;
    }

    victim_fp->status_id = nFTCommonStatusGuard;
    victim_fp->motion_id = nFTCommonMotionGuardOn;
    victim_fp->motion_script_id = nFTCommonMotionGuardOn;
    victim_fp->shield_health = expected_total;
    victim_fp->shield_damage = damage;
    victim_fp->shield_damage_total = expected_total;
    victim_fp->hitlag_tics = 0;
    victim_fp->input.pl.button_tap = 0xffffu;
    victim_fp->input.pl.button_release = 0xffffu;
    victim_fp->shield_health -= victim_fp->shield_damage_total;
    if (victim_fp->shield_health <= 0)
    {
        victim_fp->shield_health = 30;
        ftCommonShieldBreakFlyCommonSetStatus(victim_gobj);
    }
    if ((victim_fp->shield_health == 30) &&
        (victim_fp->status_id == nFTCommonStatusShieldBreakFly) &&
        (victim_fp->motion_id == nFTCommonMotionShieldBreakFly) &&
        (victim_fp->ga == nMPKineticsAir) &&
        (gNdsSCVSBattleLastFGM == (u32)nSYAudioFGMShieldBreak))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_BREAK;
    }
    victim_fp->proc_lagstart =
        ndsFighterMarioFoxStageMPLiveHitDamageLoopShieldLagStart;
    shield_break_hitlag = 0;
    if (victim_fp->shield_damage != 0)
    {
        shield_break_hitlag =
            ftParamGetHitLag(victim_fp->shield_damage,
                             nFTCommonStatusGuard,
                             victim_fp->hitlag_mul);
        victim_fp->hitlag_tics = shield_break_hitlag;
        victim_fp->input.pl.button_tap = 0;
        victim_fp->input.pl.button_release = 0;
        if (victim_fp->proc_lagstart != NULL)
        {
            victim_fp->proc_lagstart(victim_gobj);
        }
        victim_fp->shield_damage = 0;
        victim_fp->shield_damage_total = 0;
    }
    if ((shield_break_hitlag > 0) &&
        (victim_fp->hitlag_tics == shield_break_hitlag) &&
        (victim_fp->status_id == nFTCommonStatusShieldBreakFly) &&
        (victim_fp->shield_damage == 0) &&
        (victim_fp->shield_damage_total == 0) &&
        (victim_fp->input.pl.button_tap == 0u) &&
        (victim_fp->input.pl.button_release == 0u) &&
        (sNdsStageMPLiveHitDamageLoopShieldLagStartCount ==
            (saved_lagstart_count + 2u)))
    {
        contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_BREAK_CLEAR;
    }

    *attacker_fp = saved_attacker;
    *victim_fp = saved_victim;
    sNdsStageMPLiveHitDamageLoopShieldLagStartCount = saved_lagstart_count;
    gNdsFighterDashRunGuardSetOffSetStatusCount = saved_guard_setoff_count;
    gNdsFighterDashRunFtMainGuardSetOffStatusCount =
        saved_guard_setoff_ftmain_count;
    gNdsFighterDashRunGuardSetOffMask = saved_guard_setoff_mask;
    gNdsFighterDashRunGuardSetOffCallbackMask =
        saved_guard_setoff_callback_mask;
    gNdsFighterDashRunAnimEventsCallCount = saved_anim_events_count;
    gNdsSCVSBattleLastFGM = saved_last_fgm;
    gNdsFighterDashRunGuardSetOffFramesMilli = saved_guard_setoff_frames;
    gNdsFighterDashRunGuardSetOffVelMilli = saved_guard_setoff_vel;
    sNdsFighterDashRunGuardOnActive = saved_guard_on_active;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    victim_gobj->anim_frame = victim_anim_frame_saved;
    if (is_attacker_x_saved != FALSE)
    {
        attacker_root->translate.vec.f.x = attacker_x_saved;
    }
    if (is_victim_root_saved != FALSE)
    {
        victim_root->translate.vec.f.x = victim_x_saved;
        victim_root->anim_speed = victim_anim_speed_saved;
    }
    contact_mask |= NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_RESTORE;
    gNdsStageMPLiveHitDamageLoopShieldContactMask = contact_mask;
    gNdsStageMPLiveHitDamageLoopShieldContactAttackID = attack_id;
    gNdsStageMPLiveHitDamageLoopShieldContactAngleMilli =
        ndsFloatToMilliSigned(shield_angle);

    return ((gNdsStageMPLiveHitDamageLoopShieldStatCount != 0u) &&
            (gNdsStageMPLiveHitDamageLoopShieldEffectCount != 0u) &&
            ((gNdsStageMPLiveHitDamageLoopShieldSetOffClearMask &
                0x3fu) == 0x3fu) &&
            ((gNdsStageMPLiveHitDamageLoopShieldSetOffTickMask &
                0x1fu) == 0x1fu) &&
            ((gNdsStageMPLiveHitDamageLoopShieldContactMask & 0x7fffffu) ==
                0x7fffffu)) ? TRUE : FALSE;
}

static s32 ndsFTMotionSignExtend(u32 value, u32 bits)
{
    const u32 sign = 1u << (bits - 1u);

    return (s32)((value ^ sign) - sign);
}

static u32 ndsFighterDashRunMotionEventWords(u32 opcode)
{
    switch (opcode)
    {
    case NDS_FTMOTION_EVENT_MAKE_ATTACK_COLL:
    case NDS_FTMOTION_EVENT_MAKE_ATTACK_COLL_SCALED:
        return 5u;
    case NDS_FTMOTION_EVENT_SET_ATTACK_COLL_OFFSET:
    case NDS_FTMOTION_EVENT_SET_THROW:
    case NDS_FTMOTION_EVENT_SET_DAMAGE_THROWN:
    case NDS_FTMOTION_EVENT_SUBROUTINE:
    case NDS_FTMOTION_EVENT_GOTO:
    case NDS_FTMOTION_EVENT_SET_PARALLEL_SCRIPT:
        return 2u;
    case NDS_FTMOTION_EVENT_SET_DAMAGE_COLL_PART_ID:
    case NDS_FTMOTION_EVENT_EFFECT:
    case NDS_FTMOTION_EVENT_EFFECT_ITEM_HOLD:
        return 4u;
    default:
        return 1u;
    }
}

static sb32 ndsFighterDashRunShouldWriteBackAttackPosition(
    FTStruct *fp, u32 attack_id, const FTAttackColl *attack_coll)
{
    if ((fp == NULL) || (attack_coll == NULL))
    {
        return FALSE;
    }
    return (fp->player == 1) &&
           (fp->status_id == nFTCommonStatusAttack12) &&
           (attack_id == 1u) &&
           (attack_coll->joint_id == 14) &&
           (attack_coll->damage == 4) &&
           ((s32)attack_coll->size == 100);
}

static sb32 ndsFighterDashRunStepAttackCollPosition(FTStruct *fp,
                                                    u32 attack_id,
                                                    sb32 is_writeback_selected)
{
    FTAttackColl attack_work;
    FTAttackColl *attack_coll;

    if ((fp == NULL) || (attack_id >= FTATTACKCOLL_NUM_MAX))
    {
        return FALSE;
    }
    attack_coll = &fp->attack_colls[attack_id];
    if ((attack_coll->attack_state != nGMAttackStateNew) &&
        (is_writeback_selected == FALSE))
    {
        return FALSE;
    }

    attack_work = *attack_coll;
    gNdsFighterDashRunAttackEventPositionMask |=
        NDS_FTMOTION_ATTACK_EVENT_POS_NEW;
    attack_work.pos_curr = attack_work.offset;

    if (attack_work.is_scale_pos != FALSE)
    {
        f32 size_mul;

        if ((fp->attr == NULL) || (fp->attr->size == 0.0F))
        {
            return FALSE;
        }
        size_mul = 1.0F / fp->attr->size;
        attack_work.pos_curr.x *= size_mul;
        attack_work.pos_curr.y *= size_mul;
        attack_work.pos_curr.z *= size_mul;
    }
    if (attack_work.joint != NULL)
    {
        gNdsFighterDashRunAttackEventPositionMask |=
            NDS_FTMOTION_ATTACK_EVENT_POS_JOINT_READY;
    }
    gmCollisionGetFighterPartsWorldPosition(attack_work.joint,
                                            &attack_work.pos_curr);
    gNdsFighterDashRunAttackEventPositionMask |=
        NDS_FTMOTION_ATTACK_EVENT_POS_WORLD;

    attack_work.attack_state = nGMAttackStateTransfer;
    gNdsFighterDashRunAttackEventPositionMask |=
        NDS_FTMOTION_ATTACK_EVENT_POS_TRANSFER;
    attack_work.attack_matrix.unk_fthitmtx_0x0 = FALSE;
    attack_work.attack_matrix.unk_fthitmtx_0x44 = 0.0F;
    gNdsFighterDashRunAttackEventPositionMask |=
        NDS_FTMOTION_ATTACK_EVENT_POS_MATRIX_RESET;

    if (is_writeback_selected != FALSE)
    {
        *attack_coll = attack_work;
        gNdsFighterDashRunAttackEventPositionMask |=
            NDS_FTMOTION_ATTACK_EVENT_POS_WRITEBACK;
    }

    gNdsFighterDashRunAttackEventPositionState =
        (u32)attack_work.attack_state;
    gNdsFighterDashRunAttackEventPositionAttackID = attack_id;
    gNdsFighterDashRunAttackEventPositionJointID =
        (u32)attack_work.joint_id;
    gNdsFighterDashRunAttackEventPositionX =
        ndsFloatToMilliSigned(attack_work.pos_curr.x);
    gNdsFighterDashRunAttackEventPositionY =
        ndsFloatToMilliSigned(attack_work.pos_curr.y);
    gNdsFighterDashRunAttackEventPositionZ =
        ndsFloatToMilliSigned(attack_work.pos_curr.z);
    gNdsFighterDashRunAttackEventPositionMatrixFlag =
        attack_work.attack_matrix.unk_fthitmtx_0x0;
    gNdsFighterDashRunAttackEventPositionMatrixValue =
        ndsFloatToMilliSigned(attack_work.attack_matrix.unk_fthitmtx_0x44);
    return TRUE;
}

static sb32 ndsFighterDashRunStepAttackCollInterpolate(FTStruct *fp,
                                                       u32 attack_id)
{
    FTAttackColl *attack_coll;
    Vec3f pos_prev_source;

    if ((fp == NULL) || (attack_id >= FTATTACKCOLL_NUM_MAX))
    {
        return FALSE;
    }
    attack_coll = &fp->attack_colls[attack_id];
    if (attack_coll->attack_state != nGMAttackStateTransfer)
    {
        return FALSE;
    }

    pos_prev_source = attack_coll->pos_curr;
    attack_coll->attack_state = nGMAttackStateInterpolate;
    gNdsFighterDashRunAttackEventPositionMask |=
        NDS_FTMOTION_ATTACK_EVENT_POS_INTERPOLATE;

    attack_coll->pos_prev = attack_coll->pos_curr;
    if ((attack_coll->pos_prev.x == pos_prev_source.x) &&
        (attack_coll->pos_prev.y == pos_prev_source.y) &&
        (attack_coll->pos_prev.z == pos_prev_source.z))
    {
        gNdsFighterDashRunAttackEventPositionMask |=
            NDS_FTMOTION_ATTACK_EVENT_POS_PREV_COPY;
    }
    attack_coll->pos_curr = attack_coll->offset;

    if (attack_coll->is_scale_pos != FALSE)
    {
        f32 size_mul;

        if ((fp->attr == NULL) || (fp->attr->size == 0.0F))
        {
            return FALSE;
        }
        size_mul = 1.0F / fp->attr->size;
        attack_coll->pos_curr.x *= size_mul;
        attack_coll->pos_curr.y *= size_mul;
        attack_coll->pos_curr.z *= size_mul;
    }
    gmCollisionGetFighterPartsWorldPosition(attack_coll->joint,
                                            &attack_coll->pos_curr);
    attack_coll->attack_matrix.unk_fthitmtx_0x0 = FALSE;
    attack_coll->attack_matrix.unk_fthitmtx_0x44 = 0.0F;

    gNdsFighterDashRunAttackEventPositionState =
        (u32)attack_coll->attack_state;
    gNdsFighterDashRunAttackEventPositionAttackID = attack_id;
    gNdsFighterDashRunAttackEventPositionJointID =
        (u32)attack_coll->joint_id;
    gNdsFighterDashRunAttackEventPositionX =
        ndsFloatToMilliSigned(attack_coll->pos_curr.x);
    gNdsFighterDashRunAttackEventPositionY =
        ndsFloatToMilliSigned(attack_coll->pos_curr.y);
    gNdsFighterDashRunAttackEventPositionZ =
        ndsFloatToMilliSigned(attack_coll->pos_curr.z);
    gNdsFighterDashRunAttackEventPositionMatrixFlag =
        attack_coll->attack_matrix.unk_fthitmtx_0x0;
    gNdsFighterDashRunAttackEventPositionMatrixValue =
        ndsFloatToMilliSigned(attack_coll->attack_matrix.unk_fthitmtx_0x44);
    return TRUE;
}

static sb32 ndsFighterDashRunCheckAttackInFighterRange(
    const Vec3f *attack_position, const Vec3f *obj_position,
    const Vec3f *range, f32 size)
{
    f32 distx;
    f32 disty;

    if ((attack_position == NULL) || (obj_position == NULL) ||
        (range == NULL))
    {
        return FALSE;
    }

    distx = attack_position->x - obj_position->x;
    disty = attack_position->y - obj_position->y;
    if ((distx < (-range->z - size)) ||
        (distx > (range->z + size)) ||
        (disty < (-range->y - size)) ||
        (disty > (range->x + size)))
    {
        return FALSE;
    }
    return TRUE;
}

static u32 ndsGMCollisionRectangleXYFlags(const Vec3f *lhs,
                                          const Vec3f *rhs)
{
    u32 flags = 0u;

    if ((lhs == NULL) || (rhs == NULL))
    {
        return 0xfu;
    }
    if (lhs->x < -rhs->x)
    {
        flags |= 1u;
    }
    if (lhs->x > rhs->x)
    {
        flags |= 2u;
    }
    if (lhs->y < -rhs->y)
    {
        flags |= 4u;
    }
    if (lhs->y > rhs->y)
    {
        flags |= 8u;
    }
    return flags;
}

static u32 ndsGMCollisionRectangleZFlags(const Vec3f *lhs,
                                         const Vec3f *rhs)
{
    u32 flags = 0u;

    if ((lhs == NULL) || (rhs == NULL))
    {
        return 0x3u;
    }
    if (lhs->z < -rhs->z)
    {
        flags |= 1u;
    }
    if (lhs->z > rhs->z)
    {
        flags |= 2u;
    }
    return flags;
}

static sb32 ndsGMCollisionTestRectangle(Vec3f *pos_curr,
                                        Vec3f *pos_prev,
                                        f32 radius,
                                        s32 opkind,
                                        Mtx44f mtx,
                                        const Vec3f *offset,
                                        const Vec3f *size,
                                        const Vec3f *scale)
{
    Vec3f center;
    Vec3f clipped;
    Vec3f curr;
    Vec3f prev;
    u32 curr_flags;
    u32 prev_flags;
    u32 clip_flags;
    f32 distx;
    f32 disty;
    f32 distz;

    if ((pos_curr == NULL) || (pos_prev == NULL) || (offset == NULL) ||
        (size == NULL) || (scale == NULL) || (scale->x == 0.0F) ||
        (scale->y == 0.0F) || (scale->z == 0.0F))
    {
        return FALSE;
    }

    center.x = size->x + (radius / scale->x);
    center.y = size->y + (radius / scale->y);
    center.z = size->z + (radius / scale->z);

    if (opkind == nGMAttackStateTransfer)
    {
        curr = *pos_curr;

        if (mtx != NULL)
        {
            gmCollisionGetWorldPosition(mtx, &curr);
        }
        curr.x -= offset->x;
        curr.y -= offset->y;
        curr.z -= offset->z;

        return ((-center.x <= curr.x) && (curr.x <= center.x) &&
                (-center.y <= curr.y) && (curr.y <= center.y) &&
                (-center.z <= curr.z) && (curr.z <= center.z))
                   ? TRUE
                   : FALSE;
    }

    curr = *pos_curr;
    prev = *pos_prev;

    if (mtx != NULL)
    {
        gmCollisionGetWorldPosition(mtx, &curr);
        gmCollisionGetWorldPosition(mtx, &prev);
    }
    curr.x -= offset->x;
    curr.y -= offset->y;
    curr.z -= offset->z;
    prev.x -= offset->x;
    prev.y -= offset->y;
    prev.z -= offset->z;

    distx = prev.x - curr.x;
    disty = prev.y - curr.y;
    distz = prev.z - curr.z;

    curr_flags = ndsGMCollisionRectangleXYFlags(&curr, &center);
    prev_flags = ndsGMCollisionRectangleXYFlags(&prev, &center);

    while ((curr_flags != 0u) || (prev_flags != 0u))
    {
        if ((curr_flags & prev_flags) != 0u)
        {
            return FALSE;
        }
        clip_flags = (curr_flags != 0u) ? curr_flags : prev_flags;

        if ((clip_flags & 1u) != 0u)
        {
            if (distx == 0.0F)
            {
                return FALSE;
            }
            clipped.x = -center.x;
            clipped.y = (((clipped.x - curr.x) / distx) * disty) + curr.y;
            clipped.z = (((clipped.x - curr.x) / distx) * distz) + curr.z;
        }
        else if ((clip_flags & 2u) != 0u)
        {
            if (distx == 0.0F)
            {
                return FALSE;
            }
            clipped.x = center.x;
            clipped.y = (((clipped.x - curr.x) / distx) * disty) + curr.y;
            clipped.z = (((clipped.x - curr.x) / distx) * distz) + curr.z;
        }
        else if ((clip_flags & 4u) != 0u)
        {
            if (disty == 0.0F)
            {
                return FALSE;
            }
            clipped.y = -center.y;
            clipped.x = (((clipped.y - curr.y) / disty) * distx) + curr.x;
            clipped.z = (((clipped.y - curr.y) / disty) * distz) + curr.z;
        }
        else
        {
            if (disty == 0.0F)
            {
                return FALSE;
            }
            clipped.y = center.y;
            clipped.x = (((clipped.y - curr.y) / disty) * distx) + curr.x;
            clipped.z = (((clipped.y - curr.y) / disty) * distz) + curr.z;
        }

        if (clip_flags == curr_flags)
        {
            curr = clipped;
            curr_flags = ndsGMCollisionRectangleXYFlags(&curr, &center);
        }
        else
        {
            prev = clipped;
            prev_flags = ndsGMCollisionRectangleXYFlags(&prev, &center);
        }
    }

    curr_flags = ndsGMCollisionRectangleZFlags(&curr, &center);
    prev_flags = ndsGMCollisionRectangleZFlags(&prev, &center);

    return ((curr_flags & prev_flags) != 0u) ? FALSE : TRUE;
}

static sb32 ndsFighterDashRunStepAttackDamageRectangle(FTStruct *fp,
                                                       u32 attack_id)
{
    FTAttackColl *attack_coll;
    FTStruct *target_fp;
    FTDamageColl *damage_coll;
    FTParts *parts;
    Vec3f pos_curr;
    Vec3f pos_prev;

    if ((fp == NULL) || (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (fp->player != 1) || (attack_id != 1u))
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    if (attack_coll->attack_state == nGMAttackStateOff)
    {
        return FALSE;
    }

    target_fp = &sNdsFighterStructPool[0];
    if ((ndsFighterStructIsPoolPointer(target_fp) == FALSE) ||
        (target_fp->attr == NULL))
    {
        return FALSE;
    }

    damage_coll = &target_fp->damage_colls[0];
    if ((damage_coll->hitstatus != nGMHitStatusNormal) ||
        (damage_coll->joint == NULL))
    {
        return FALSE;
    }

    parts = ftGetParts(damage_coll->joint);
    if ((parts == NULL) || (parts->vec_scale.x == 0.0F) ||
        (parts->vec_scale.y == 0.0F) || (parts->vec_scale.z == 0.0F))
    {
        return FALSE;
    }

    pos_curr = damage_coll->offset;
    gmCollisionGetWorldPosition(parts->mtx_translate, &pos_curr);
    pos_prev = pos_curr;

    if (ndsGMCollisionTestRectangle(&pos_curr,
                                    &pos_prev,
                                    attack_coll->size,
                                    attack_coll->attack_state,
                                    parts->unk_dobjtrans_0x9C,
                                    &damage_coll->offset,
                                    &damage_coll->size,
                                    &parts->vec_scale) == FALSE)
    {
        return FALSE;
    }

    gNdsFighterDashRunAttackEventPositionMask |=
        NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_RECT;
    return TRUE;
}

static sb32 ndsGMCollisionCheckFighterAttackDamageCollideSelected(
    FTAttackColl *attack_coll, FTDamageColl *damage_coll)
{
    FTParts *parts;
    DObj *dobj;

    if ((attack_coll == NULL) || (damage_coll == NULL) ||
        (damage_coll->hitstatus != nGMHitStatusNormal))
    {
        return FALSE;
    }

    dobj = damage_coll->joint;
    if (dobj == NULL)
    {
        return FALSE;
    }

    parts = ftGetParts(dobj);
    if ((parts == NULL) || (parts->vec_scale.x == 0.0F) ||
        (parts->vec_scale.y == 0.0F) || (parts->vec_scale.z == 0.0F))
    {
        return FALSE;
    }

    return ndsGMCollisionTestRectangle(&attack_coll->pos_curr,
                                       &attack_coll->pos_prev,
                                       attack_coll->size,
                                       attack_coll->attack_state,
                                       parts->unk_dobjtrans_0x9C,
                                       &damage_coll->offset,
                                       &damage_coll->size,
                                       &parts->vec_scale);
}

static sb32 ndsGMCollisionCheckFighterAttacksCollideSelected(
    FTAttackColl *attack_coll1, FTAttackColl *attack_coll2)
{
    Vec3f delta;
    f32 radius;
    f32 dist;

    if ((attack_coll1 == NULL) || (attack_coll2 == NULL) ||
        (attack_coll1->attack_state == nGMAttackStateOff) ||
        (attack_coll2->attack_state == nGMAttackStateOff))
    {
        return FALSE;
    }

    radius = attack_coll1->size + attack_coll2->size;
    delta.x = attack_coll1->pos_curr.x - attack_coll2->pos_curr.x;
    delta.y = attack_coll1->pos_curr.y - attack_coll2->pos_curr.y;
    delta.z = attack_coll1->pos_curr.z - attack_coll2->pos_curr.z;
    dist = SQUARE(delta.x) + SQUARE(delta.y) + SQUARE(delta.z);

    return (dist <= SQUARE(radius)) ? TRUE : FALSE;
}

static sb32 ndsGMCollisionCheckFighterAttackShieldCollideSelected(
    FTAttackColl *attack_coll, DObj *shield_joint, f32 *p_angle)
{
    FTParts *parts;
    Vec3f shield_center;
    Vec3f delta;
    Vec3f radius;
    f32 dist;

    if ((attack_coll == NULL) || (shield_joint == NULL) ||
        (attack_coll->attack_state == nGMAttackStateOff))
    {
        return FALSE;
    }

    parts = ftGetParts(shield_joint);
    if ((parts == NULL) || (parts->vec_scale.x == 0.0F) ||
        (parts->vec_scale.y == 0.0F) || (parts->vec_scale.z == 0.0F))
    {
        return FALSE;
    }

    shield_center.x = 0.0F;
    shield_center.y = 0.0F;
    shield_center.z = 0.0F;
    gmCollisionGetWorldPosition(parts->mtx_translate, &shield_center);

    radius.x = 30.0F + (attack_coll->size / parts->vec_scale.x);
    radius.y = 30.0F + (attack_coll->size / parts->vec_scale.y);
    radius.z = 30.0F + (attack_coll->size / parts->vec_scale.z);
    if ((radius.x <= 0.0F) || (radius.y <= 0.0F) ||
        (radius.z <= 0.0F))
    {
        return FALSE;
    }

    delta.x = attack_coll->pos_curr.x - shield_center.x;
    delta.y = attack_coll->pos_curr.y - shield_center.y;
    delta.z = attack_coll->pos_curr.z - shield_center.z;
    dist = SQUARE(delta.x / radius.x) + SQUARE(delta.y / radius.y) +
           SQUARE(delta.z / radius.z);
    gNdsStageMPLiveHitDamageLoopShieldContactCollisionCount++;

    if (dist > 1.0F)
    {
        return FALSE;
    }
    if (p_angle != NULL)
    {
        *p_angle = F_CLC_DTOR32(180.0F);
    }
    gNdsStageMPLiveHitDamageLoopShieldContactHitCount++;
    return TRUE;
}

#define NDS_STAGE_MPLIVEHIT_HURTBOX_ACTIVE      (1u << 0)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_NONE_STOP   (1u << 1)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_SKIP        (1u << 2)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_TEST        (1u << 3)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_HIT         (1u << 4)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_BREAK       (1u << 5)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_FIRST_SLOT  (1u << 6)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_FIRST_JOINT (1u << 7)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_GLOBAL      (1u << 8)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_SPEC_SKIP   (1u << 9)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_STAR_SKIP   (1u << 10)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_BASE_SKIP   (1u << 11)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_GLOB_REST   (1u << 12)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DETOFF      (1u << 13)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DETOFF_SKIP (1u << 14)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DETOFF_REST (1u << 15)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_MISS        (1u << 16)

#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_RECORD  (1u << 0)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_HITLOG  (1u << 1)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_STATS   (1u << 2)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_QUEUE   (1u << 3)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_PERCENT (1u << 4)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_HITLAG  (1u << 5)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT    (1u << 6)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_RESTORE (1u << 7)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SEARCH  (1u << 8)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_NATURAL (1u << 9)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_REPEAT  (1u << 10)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT1   (1u << 11)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT2   (1u << 12)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT4   (1u << 13)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT5   (1u << 14)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT6   (1u << 15)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT7   (1u << 16)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT8   (1u << 17)
#define NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT9   (1u << 18)
#define NDS_STAGE_MPLIVEHIT_EFFECTONLY_RECORD      (1u << 0)
#define NDS_STAGE_MPLIVEHIT_EFFECTONLY_STATUS      (1u << 1)
#define NDS_STAGE_MPLIVEHIT_EFFECTONLY_ATTACK_DMG  (1u << 2)
#define NDS_STAGE_MPLIVEHIT_EFFECTONLY_NO_QUEUE    (1u << 3)
#define NDS_STAGE_MPLIVEHIT_EFFECTONLY_NO_PERCENT  (1u << 4)
#define NDS_STAGE_MPLIVEHIT_EFFECTONLY_NO_HITLOG   (1u << 5)
#define NDS_STAGE_MPLIVEHIT_EFFECTONLY_EFFECT      (1u << 6)
#define NDS_STAGE_MPLIVEHIT_EFFECTONLY_SFX         (1u << 7)
#define NDS_STAGE_MPLIVEHIT_EFFECTONLY_RESTORE     (1u << 8)
#define NDS_STAGE_MPLIVEHIT_RESIST_RECORD          (1u << 0)
#define NDS_STAGE_MPLIVEHIT_RESIST_STATUS          (1u << 1)
#define NDS_STAGE_MPLIVEHIT_RESIST_SEED            (1u << 2)
#define NDS_STAGE_MPLIVEHIT_RESIST_CHECK_FALSE     (1u << 3)
#define NDS_STAGE_MPLIVEHIT_RESIST_AFTER           (1u << 4)
#define NDS_STAGE_MPLIVEHIT_RESIST_NO_QUEUE        (1u << 5)
#define NDS_STAGE_MPLIVEHIT_RESIST_NO_PERCENT      (1u << 6)
#define NDS_STAGE_MPLIVEHIT_RESIST_NO_HITLOG       (1u << 7)
#define NDS_STAGE_MPLIVEHIT_RESIST_EFFECT          (1u << 8)
#define NDS_STAGE_MPLIVEHIT_RESIST_SFX             (1u << 9)
#define NDS_STAGE_MPLIVEHIT_RESIST_ATTACK_DMG      (1u << 10)
#define NDS_STAGE_MPLIVEHIT_RESIST_RESTORE         (1u << 11)
#define NDS_STAGE_MPLIVEHIT_RESIST_BREAK_SEED      (1u << 0)
#define NDS_STAGE_MPLIVEHIT_RESIST_BREAK_TRUE      (1u << 1)
#define NDS_STAGE_MPLIVEHIT_RESIST_BREAK_CLEAR     (1u << 2)
#define NDS_STAGE_MPLIVEHIT_RESIST_BREAK_LEFTOVER  (1u << 3)
#define NDS_STAGE_MPLIVEHIT_RESIST_BREAK_QUEUE     (1u << 4)
#define NDS_STAGE_MPLIVEHIT_RESIST_BREAK_LAG       (1u << 5)
#define NDS_STAGE_MPLIVEHIT_RESIST_BREAK_RESTORE   (1u << 6)
#define NDS_STAGE_MPLIVEHIT_THROW_ATTRIB_SEED      (1u << 0)
#define NDS_STAGE_MPLIVEHIT_THROW_ATTRIB_SOURCE    (1u << 1)
#define NDS_STAGE_MPLIVEHIT_THROW_ATTRIB_HITLOG    (1u << 2)
#define NDS_STAGE_MPLIVEHIT_THROW_ATTRIB_STATS     (1u << 3)
#define NDS_STAGE_MPLIVEHIT_THROW_ATTRIB_RESTORE   (1u << 4)
#define NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_THIS      (1u << 0)
#define NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_OTHER     (1u << 1)
#define NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_THIS_REB  (1u << 2)
#define NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_OTHER_REB (1u << 3)
#define NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_EFFECT    (1u << 4)
#define NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_RESTORE   (1u << 5)
#define NDS_STAGE_MPLIVEHIT_CATCH_STAT_RECORD      (1u << 0)
#define NDS_STAGE_MPLIVEHIT_CATCH_STAT_DETECT      (1u << 1)
#define NDS_STAGE_MPLIVEHIT_CATCH_STAT_DIST        (1u << 2)
#define NDS_STAGE_MPLIVEHIT_CATCH_STAT_SEARCH      (1u << 3)
#define NDS_STAGE_MPLIVEHIT_CATCH_STAT_RESTORE     (1u << 4)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_RESET     (1u << 0)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_TARGET    (1u << 1)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_GA        (1u << 2)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_SKIP  (1u << 3)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_PASS  (1u << 4)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_STATUS    (1u << 5)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_GRAB      (1u << 6)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_COLLIDE   (1u << 7)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_UPDATE    (1u << 8)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_RESTORE   (1u << 9)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REPEAT    (1u << 10)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_NATURAL   (1u << 11)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_IMMUNE    (1u << 12)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_TEAM      (1u << 13)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_GHOST     (1u << 14)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_BOSS      (1u << 15)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_TGT_STAT  (1u << 16)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_ATK_OFF   (1u << 17)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_GA_SKIP   (1u << 18)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_HURT  (1u << 19)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_SHLD  (1u << 20)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_GROUP (1u << 21)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_NONE_BRK  (1u << 22)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_NO_HIT    (1u << 23)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_PROC_GATE (1u << 24)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_PROC_FIND (1u << 25)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_PROC_CB   (1u << 26)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_OBS_REG   (1u << 27)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_OBS_CB    (1u << 28)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_OBS_TWIST (1u << 29)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_OBS_TICK  (1u << 30)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_OBS_GHOST (1u << 31)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_STAT (1u << 0)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_GRAB (1u << 1)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_INV  (1u << 2)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_NONE (1u << 3)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_MISS (1u << 4)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_ATK  (1u << 5)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_GA   (1u << 6)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_REC  (1u << 7)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_IMM  (1u << 8)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_GHOST (1u << 9)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_BOSS (1u << 10)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_TSTAT (1u << 11)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_TEAM (1u << 12)
#define NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_SELF (1u << 13)

static u32 sNdsCatchSearchProcCatchCount;
static u32 sNdsCatchSearchProcCaptureCount;
static GObj *sNdsCatchSearchProcCatchGObj;
static GObj *sNdsCatchSearchProcCaptureTargetGObj;
static GObj *sNdsCatchSearchProcCaptureFighterGObj;
static u32 sNdsCatchSearchHazardProbeCalls;
static u32 sNdsCatchSearchHazardProbeMask;
static GObj *sNdsCatchSearchHazardExpectedFighter;
static GObj *sNdsCatchSearchHazardExpectedFirst;
static GObj *sNdsCatchSearchHazardExpectedSecond;

static u32 ndsCatchSearchCountVictimRecords(FTAttackColl *attack_coll,
                                            GObj *victim_gobj)
{
    u32 count = 0u;
    u32 i;

    if ((attack_coll == NULL) || (victim_gobj == NULL))
    {
        return 0u;
    }
    for (i = 0u; i < GMATTACKREC_NUM_MAX; i++)
    {
        if (attack_coll->attack_records[i].victim_gobj == victim_gobj)
        {
            count++;
        }
    }
    return count;
}

static void ndsCatchSearchDisableSiblingAttackColls(FTStruct *fp,
                                                    u32 attack_id)
{
    u32 i;

    if (fp == NULL)
    {
        return;
    }
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        if (i != attack_id)
        {
            fp->attack_colls[i].attack_state = nGMAttackStateOff;
        }
    }
}

static void ndsCatchSearchSeedEligibleTarget(FTStruct *fp,
                                             FTStruct *target_fp)
{
    if ((fp == NULL) || (target_fp == NULL))
    {
        return;
    }
    fp->catch_mask = 1u;
    fp->team = 1u;
    target_fp->team = 2u;
    target_fp->capture_immune_mask = 0u;
    target_fp->is_ghost = FALSE;
    target_fp->fkind = nFTKindMario;
    target_fp->special_hitstatus = nGMHitStatusNormal;
    target_fp->star_hitstatus = nGMHitStatusNormal;
    target_fp->hitstatus = nGMHitStatusNormal;
    if (gSCManagerBattleState != NULL)
    {
        gSCManagerBattleState->is_team_battle = FALSE;
    }
}

static sb32 ndsCatchSearchSeedSelectedDamage(FTStruct *target_fp,
                                             u32 selected_slot)
{
    FTDamageColl *damage_coll;
    u32 i;

    if ((target_fp == NULL) || (selected_slot >= FTDAMAGECOLL_NUM_MAX))
    {
        return FALSE;
    }
    for (i = 0u; i < FTDAMAGECOLL_NUM_MAX; i++)
    {
        damage_coll = &target_fp->damage_colls[i];
        if (damage_coll->hitstatus == nGMHitStatusNone)
        {
            break;
        }
        damage_coll->hitstatus = nGMHitStatusIntangible;
        damage_coll->is_grabbable = FALSE;
    }
    damage_coll = &target_fp->damage_colls[selected_slot];
    if ((damage_coll->joint == NULL) || (ftGetParts(damage_coll->joint) == NULL))
    {
        return FALSE;
    }
    damage_coll->hitstatus = nGMHitStatusNormal;
    damage_coll->is_grabbable = TRUE;
    return TRUE;
}

static sb32 ndsCatchSearchPlaceAttackOnDamage(FTAttackColl *attack_coll,
                                              FTDamageColl *damage_coll)
{
    FTParts *parts;

    if ((attack_coll == NULL) || (damage_coll == NULL) ||
        (damage_coll->joint == NULL))
    {
        return FALSE;
    }
    parts = ftGetParts(damage_coll->joint);
    if ((parts == NULL) || (parts->vec_scale.x == 0.0F) ||
        (parts->vec_scale.y == 0.0F) || (parts->vec_scale.z == 0.0F))
    {
        return FALSE;
    }
    attack_coll->pos_curr = damage_coll->offset;
    gmCollisionGetWorldPosition(parts->mtx_translate, &attack_coll->pos_curr);
    attack_coll->pos_prev = attack_coll->pos_curr;
    return TRUE;
}

static void ndsCatchSearchRunTwoFighterSearch(GObj *attacker_gobj,
                                              GObj *target_gobj)
{
    if ((attacker_gobj == NULL) || (target_gobj == NULL))
    {
        return;
    }
    gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
    attacker_gobj->link_next = target_gobj;
    target_gobj->link_next = NULL;
    ftMainSearchFighterCatch(attacker_gobj);
}

static void ndsCatchSearchRunTwoFighterProcSearch(GObj *attacker_gobj,
                                                  GObj *target_gobj)
{
    if ((attacker_gobj == NULL) || (target_gobj == NULL))
    {
        return;
    }
    gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
    attacker_gobj->link_next = target_gobj;
    target_gobj->link_next = NULL;
    ftMainProcSearchCatch(attacker_gobj);
}

static void ndsCatchSearchProcCatchCallback(GObj *fighter_gobj)
{
    sNdsCatchSearchProcCatchCount++;
    sNdsCatchSearchProcCatchGObj = fighter_gobj;
}

static void ndsCatchSearchProcCaptureCallback(GObj *target_gobj,
                                              GObj *fighter_gobj)
{
    sNdsCatchSearchProcCaptureCount++;
    sNdsCatchSearchProcCaptureTargetGObj = target_gobj;
    sNdsCatchSearchProcCaptureFighterGObj = fighter_gobj;
}

static sb32 ndsCatchSearchHazardProbeCallback(GObj *ground_gobj,
                                              GObj *fighter_gobj,
                                              s32 *kind)
{
    sNdsCatchSearchHazardProbeCalls++;
    if ((fighter_gobj == sNdsCatchSearchHazardExpectedFighter) &&
        (kind != NULL))
    {
        sNdsCatchSearchHazardProbeMask |= 1u << 0;
        *kind = 0;
    }
    if ((sNdsCatchSearchHazardProbeCalls == 1u) &&
        (ground_gobj == sNdsCatchSearchHazardExpectedFirst))
    {
        sNdsCatchSearchHazardProbeMask |= 1u << 1;
    }
    if ((sNdsCatchSearchHazardProbeCalls == 2u) &&
        (ground_gobj == sNdsCatchSearchHazardExpectedSecond))
    {
        sNdsCatchSearchHazardProbeMask |= 1u << 2;
    }
    return FALSE;
}

static sb32 ndsCatchSearchHazardTwisterCallback(GObj *ground_gobj,
                                                GObj *fighter_gobj,
                                                s32 *kind)
{
    sNdsCatchSearchHazardProbeCalls++;
    if ((ground_gobj == sNdsCatchSearchHazardExpectedFirst) &&
        (fighter_gobj == sNdsCatchSearchHazardExpectedFighter) &&
        (kind != NULL))
    {
        *kind = nGMHitEnvironmentTwister;
        sNdsCatchSearchHazardProbeMask |= 1u;
        return TRUE;
    }
    return FALSE;
}

static sb32 ndsCatchSearchHazardTaruCannCallback(GObj *ground_gobj,
                                                 GObj *fighter_gobj,
                                                 s32 *kind)
{
    sNdsCatchSearchHazardProbeCalls++;
    if ((ground_gobj == sNdsCatchSearchHazardExpectedFirst) &&
        (fighter_gobj == sNdsCatchSearchHazardExpectedFighter) &&
        (kind != NULL))
    {
        *kind = nGMHitEnvironmentTaruCann;
        sNdsCatchSearchHazardProbeMask |= 1u;
        return TRUE;
    }
    return FALSE;
}

static sb32 ndsFighterDashRunProbeSourceOrderHurtboxes(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp)
{
    FTAttackColl probe;
    FTAttackColl *attack_coll;
    FTDamageColl *damage_coll;
    FTParts *parts;
    u32 active_count = 0u;
    u32 none_stop_slot = FTDAMAGECOLL_NUM_MAX;
    u32 skip_count = 0u;
    u32 test_count = 0u;
    u32 hit_count = 0u;
    u32 first_hit_slot = FTDAMAGECOLL_NUM_MAX;
    u32 first_hit_joint = 0u;
    u32 first_hit_status = 0u;
    u32 mask = 0u;
    u32 i;
    s32 saved_hitstatus0;
    s32 saved_hitstatus1;
    s32 saved_special_hitstatus;
    s32 saved_star_hitstatus;
    s32 saved_hitstatus;
    sb32 saved_damage_detect;

    if ((fp == NULL) || (target_fp == NULL) ||
        (attack_id >= FTATTACKCOLL_NUM_MAX))
    {
        return FALSE;
    }
    attack_coll = &fp->attack_colls[attack_id];
    if (attack_coll->attack_state == nGMAttackStateOff)
    {
        return FALSE;
    }

    for (i = 0u; i < FTDAMAGECOLL_NUM_MAX; i++)
    {
        damage_coll = &target_fp->damage_colls[i];
        if (damage_coll->hitstatus == nGMHitStatusNone)
        {
            none_stop_slot = i;
            break;
        }
        active_count++;
    }

    if (active_count >= 2u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_ACTIVE;
    }
    if (none_stop_slot == active_count)
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_NONE_STOP;
    }

    saved_special_hitstatus = target_fp->special_hitstatus;
    saved_star_hitstatus = target_fp->star_hitstatus;
    saved_hitstatus = target_fp->hitstatus;
    target_fp->special_hitstatus = nGMHitStatusNormal;
    target_fp->star_hitstatus = nGMHitStatusNormal;
    target_fp->hitstatus = nGMHitStatusNormal;
    if ((target_fp->special_hitstatus != nGMHitStatusIntangible) &&
        (target_fp->star_hitstatus != nGMHitStatusIntangible) &&
        (target_fp->hitstatus != nGMHitStatusIntangible))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_GLOBAL;
    }
    target_fp->special_hitstatus = nGMHitStatusIntangible;
    if (!((target_fp->special_hitstatus != nGMHitStatusIntangible) &&
          (target_fp->star_hitstatus != nGMHitStatusIntangible) &&
          (target_fp->hitstatus != nGMHitStatusIntangible)) &&
        (skip_count == 0u) && (test_count == 0u) && (hit_count == 0u))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_SPEC_SKIP;
    }
    target_fp->special_hitstatus = nGMHitStatusNormal;
    target_fp->star_hitstatus = nGMHitStatusIntangible;
    if (!((target_fp->special_hitstatus != nGMHitStatusIntangible) &&
          (target_fp->star_hitstatus != nGMHitStatusIntangible) &&
          (target_fp->hitstatus != nGMHitStatusIntangible)) &&
        (skip_count == 0u) && (test_count == 0u) && (hit_count == 0u))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_STAR_SKIP;
    }
    target_fp->star_hitstatus = nGMHitStatusNormal;
    target_fp->hitstatus = nGMHitStatusIntangible;
    if (!((target_fp->special_hitstatus != nGMHitStatusIntangible) &&
          (target_fp->star_hitstatus != nGMHitStatusIntangible) &&
          (target_fp->hitstatus != nGMHitStatusIntangible)) &&
        (skip_count == 0u) && (test_count == 0u) && (hit_count == 0u))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_BASE_SKIP;
    }
    target_fp->special_hitstatus = nGMHitStatusNormal;
    target_fp->star_hitstatus = nGMHitStatusNormal;
    target_fp->hitstatus = nGMHitStatusNormal;
    if ((target_fp->special_hitstatus != nGMHitStatusIntangible) &&
        (target_fp->star_hitstatus != nGMHitStatusIntangible) &&
        (target_fp->hitstatus != nGMHitStatusIntangible))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_GLOB_REST;
    }

    saved_damage_detect = gFTMainIsDamageDetect[attack_id];
    gFTMainIsDamageDetect[attack_id] = FALSE;
    if (gFTMainIsDamageDetect[attack_id] == FALSE)
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_DETOFF;
    }
    if ((skip_count == 0u) && (test_count == 0u) && (hit_count == 0u))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_DETOFF_SKIP;
    }
    gFTMainIsDamageDetect[attack_id] = TRUE;
    if (gFTMainIsDamageDetect[attack_id] != FALSE)
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_DETOFF_REST;
    }

    saved_hitstatus0 = target_fp->damage_colls[0].hitstatus;
    saved_hitstatus1 = target_fp->damage_colls[1].hitstatus;
    target_fp->damage_colls[0].hitstatus = nGMHitStatusIntangible;
    target_fp->damage_colls[1].hitstatus = nGMHitStatusIntangible;

    for (i = 0u; i < FTDAMAGECOLL_NUM_MAX; i++)
    {
        damage_coll = &target_fp->damage_colls[i];
        if (damage_coll->hitstatus == nGMHitStatusNone)
        {
            break;
        }
        if (damage_coll->hitstatus == nGMHitStatusIntangible)
        {
            skip_count++;
            continue;
        }

        test_count++;
        parts = (damage_coll->joint != NULL) ?
            ftGetParts(damage_coll->joint) : NULL;
        if ((parts == NULL) || (parts->vec_scale.x == 0.0F) ||
            (parts->vec_scale.y == 0.0F) || (parts->vec_scale.z == 0.0F))
        {
            continue;
        }

        probe = *attack_coll;
        probe.pos_curr = damage_coll->offset;
        gmCollisionGetWorldPosition(parts->mtx_translate, &probe.pos_curr);
        if (i == 2u)
        {
            /* Proof-local miss: keep slot 2 normal/tested, then continue. */
            probe.pos_curr.x += 1000000.0F;
        }
        probe.pos_prev = probe.pos_curr;
        if (ndsGMCollisionCheckFighterAttackDamageCollideSelected(
                &probe, damage_coll) != FALSE)
        {
            hit_count++;
            first_hit_slot = i;
            first_hit_joint = (u32)damage_coll->joint_id;
            first_hit_status = (u32)damage_coll->hitstatus;
            break;
        }
    }

    target_fp->damage_colls[0].hitstatus = saved_hitstatus0;
    target_fp->damage_colls[1].hitstatus = saved_hitstatus1;
    target_fp->special_hitstatus = saved_special_hitstatus;
    target_fp->star_hitstatus = saved_star_hitstatus;
    target_fp->hitstatus = saved_hitstatus;
    gFTMainIsDamageDetect[attack_id] = saved_damage_detect;

    if (skip_count != 0u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_SKIP;
    }
    if (test_count != 0u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_TEST;
    }
    if (hit_count != 0u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_HIT;
    }
    if (hit_count != 0u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_BREAK;
    }
    if ((hit_count != 0u) && (test_count > hit_count))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_MISS;
    }
    if (first_hit_slot == 3u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_FIRST_SLOT;
    }
    if (first_hit_joint != 0u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_FIRST_JOINT;
    }

    gNdsStageMPLiveHitDamageLoopHurtboxMask = mask;
    gNdsStageMPLiveHitDamageLoopHurtboxActiveCount = active_count;
    gNdsStageMPLiveHitDamageLoopHurtboxNoneStopSlot = none_stop_slot;
    gNdsStageMPLiveHitDamageLoopHurtboxIntangibleSkipCount = skip_count;
    gNdsStageMPLiveHitDamageLoopHurtboxTestCount = test_count;
    gNdsStageMPLiveHitDamageLoopHurtboxHitCount = hit_count;
    gNdsStageMPLiveHitDamageLoopHurtboxFirstHitSlot = first_hit_slot;
    gNdsStageMPLiveHitDamageLoopHurtboxFirstHitJoint = first_hit_joint;
    gNdsStageMPLiveHitDamageLoopHurtboxFirstHitStatus = first_hit_status;

    return ((mask & 0x1ffffu) == 0x1ffffu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeHurtboxDamageConsume(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp)
{
    static FTStruct saved_attacker;
    static FTStruct saved_target;
    FTHitLog saved_hitlog;
    FTAttackColl *attack_coll;
    FTDamageColl *damage_coll;
    FTDamageColl *natural_damage_coll;
    FTHitLog *hitlog;
    GObj *target_gobj;
    GObj *attacker_gobj;
    GObj *saved_fighter_link_head;
    GObj *saved_attacker_link_next;
    DObj *target_root;
    DObj *attacker_root;
    FTParts *damage_parts;
    FTParts *natural_damage_parts;
    u32 saved_hitlog_id;
    u32 saved_lagstart_count;
    u32 slot = gNdsStageMPLiveHitDamageLoopHurtboxFirstHitSlot;
    u32 mask = 0u;
    s32 damage;
    s32 queue_before;
    s32 queue_after;
    s32 percent_before;
    s32 percent_after;
    s32 hitlag;
    s32 natural_queue_before;
    s32 natural_queue_after;
    s32 natural_percent_before;
    s32 natural_percent_after;
    s32 natural_hitlag;
    s32 expected_lr;
    u32 i;

    if ((fp == NULL) || (target_fp == NULL) ||
        (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (slot >= FTDAMAGECOLL_NUM_MAX))
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    damage_coll = &target_fp->damage_colls[slot];
    attacker_gobj = fp->fighter_gobj;
    target_gobj = target_fp->fighter_gobj;
    attacker_root = (attacker_gobj != NULL) ? DObjGetStruct(attacker_gobj) : NULL;
    target_root = (target_gobj != NULL) ? DObjGetStruct(target_gobj) : NULL;

    if ((attack_coll->attack_state == nGMAttackStateOff) ||
        (target_fp->attr == NULL) ||
        (attacker_gobj == NULL) || (target_gobj == NULL) ||
        (attacker_root == NULL) || (target_root == NULL) ||
        (damage_coll->hitstatus != nGMHitStatusNormal) ||
        (damage_coll->joint == NULL))
    {
        return FALSE;
    }
    damage_parts = ftGetParts(damage_coll->joint);
    if (damage_parts == NULL)
    {
        return FALSE;
    }

    saved_attacker = *fp;
    saved_target = *target_fp;
    saved_fighter_link_head = gGCCommonLinks[nGCCommonLinkIDFighter];
    saved_attacker_link_next = attacker_gobj->link_next;
    saved_hitlog = sNdsFighterDashRunHitLogs[0];
    saved_hitlog_id = sNdsFighterDashRunHitLogID;
    saved_lagstart_count = sNdsFighterDashRunProcParamsLagStartCount;

    target_fp->damage_queue = 0;
    target_fp->damage_lag = 0;
    target_fp->hitlag_mul = 1.0F;
    target_fp->special_hitstatus = nGMHitStatusNormal;
    target_fp->star_hitstatus = nGMHitStatusNormal;
    target_fp->hitstatus = nGMHitStatusNormal;
    target_fp->is_shield = FALSE;
    target_fp->ga = nMPKineticsGround;
    target_fp->capture_gobj = NULL;
    fp->is_catch_or_capture = FALSE;
    fp->throw_gobj = NULL;
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        if (i != attack_id)
        {
            fp->attack_colls[i].attack_state = nGMAttackStateOff;
        }
    }
    for (i = 0u; (i < FTDAMAGECOLL_NUM_MAX) && (i < slot); i++)
    {
        target_fp->damage_colls[i].hitstatus = nGMHitStatusIntangible;
    }
    attack_coll->attack_state = nGMAttackStateInterpolate;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->is_hit_air = TRUE;
    attack_coll->pos_curr = damage_coll->offset;
    gmCollisionGetWorldPosition(damage_parts->mtx_translate,
                                &attack_coll->pos_curr);
    attack_coll->pos_prev = attack_coll->pos_curr;
    queue_before = target_fp->damage_queue;
    percent_before = target_fp->percent_damage;

    sNdsFighterDashRunHitLogID = 0u;
    ftParamClearAttackRecordID(fp, (s32)attack_id);
    gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
    attacker_gobj->link_next = NULL;
    ftMainProcSearchHitAll(target_gobj);
    if (sNdsFighterDashRunHitLogID != 0u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SEARCH;
    }
    damage = ftParamGetCapturedDamage(target_fp, attack_coll->damage);
    queue_after = target_fp->damage_queue;
    if ((attack_coll->attack_records[0].victim_gobj == target_gobj) &&
        (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
            FALSE))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_RECORD;
    }
    if ((queue_after == (queue_before + damage)) &&
        (target_fp->damage_lag >= damage))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_QUEUE;
    }

    hitlog = (sNdsFighterDashRunHitLogID != 0u) ?
        &sNdsFighterDashRunHitLogs[0] : NULL;
    if ((sNdsFighterDashRunHitLogID == 1u) &&
        (hitlog != NULL) &&
        (hitlog->damage_coll == damage_coll) &&
        (hitlog->attack_coll == attack_coll) &&
        (hitlog->attacker_gobj == attacker_gobj) &&
        (hitlog->attacker_player == fp->player) &&
        (hitlog->attacker_player_num == fp->player_num))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_HITLOG;
    }

    expected_lr =
        (target_root->translate.vec.f.x <
         attacker_root->translate.vec.f.x) ? +1 : -1;
    if ((target_fp->damage_angle == attack_coll->angle) &&
        (target_fp->damage_element == attack_coll->element) &&
        (target_fp->damage_lr == expected_lr) &&
        (target_fp->damage_player_num == fp->player_num) &&
        (target_fp->damage_joint_id == damage_coll->joint_id) &&
        (target_fp->damage_index == damage_coll->placement) &&
        (target_fp->damage_knockback != 0.0F) &&
        (target_fp->damage_kind == nFTDamageKindStatus) &&
        (target_fp->damage_object_class == nFTHitLogObjectFighter) &&
        (target_fp->damage_object_kind == fp->fkind))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_STATS;
    }

    target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
    ftMainProcParams(target_gobj);
    percent_after = target_fp->percent_damage;
    if (percent_after == (percent_before + queue_after))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_PERCENT;
    }

    hitlag = target_fp->hitlag_tics;
    if ((hitlag > 0) &&
        (sNdsFighterDashRunProcParamsLagStartCount ==
            (saved_lagstart_count + 1u)) &&
        (target_fp->is_knockback_paused != FALSE) &&
        (target_fp->damage_lag == 0) &&
        (target_fp->damage_queue == 0) &&
        (target_fp->damage_kind == nFTDamageKindDefault) &&
        (target_fp->damage_knockback == 0.0F))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_HITLAG;
    }
    if ((slot == 3u) && (damage_coll->joint_id != 0))
    {
        mask |= NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT;
    }

    gNdsStageMPLiveHitDamageLoopHurtboxDamageMask = mask;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageSlot = slot;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageJoint =
        (u32)damage_coll->joint_id;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageQueueBefore = queue_before;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageQueueAfter = queue_after;
    gNdsStageMPLiveHitDamageLoopHurtboxDamagePercentBefore = percent_before;
    gNdsStageMPLiveHitDamageLoopHurtboxDamagePercentAfter = percent_after;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageHitlag = (u32)hitlag;

    *fp = saved_attacker;
    *target_fp = saved_target;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    sNdsFighterDashRunProcParamsLagStartCount = saved_lagstart_count;

    attack_coll = &fp->attack_colls[attack_id];
    natural_damage_coll = &target_fp->damage_colls[0];
    natural_damage_parts = (natural_damage_coll->joint != NULL) ?
        ftGetParts(natural_damage_coll->joint) : NULL;
    if ((natural_damage_coll->hitstatus == nGMHitStatusNormal) &&
        (natural_damage_parts != NULL) &&
        (natural_damage_parts->vec_scale.x != 0.0F) &&
        (natural_damage_parts->vec_scale.y != 0.0F) &&
        (natural_damage_parts->vec_scale.z != 0.0F))
    {
        target_fp->damage_queue = 0;
        target_fp->damage_lag = 0;
        target_fp->hitlag_mul = 1.0F;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        target_fp->is_shield = FALSE;
        target_fp->ga = nMPKineticsGround;
        target_fp->capture_gobj = NULL;
        fp->is_catch_or_capture = FALSE;
        fp->throw_gobj = NULL;
        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            if (i != attack_id)
            {
                fp->attack_colls[i].attack_state = nGMAttackStateOff;
            }
        }
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        attack_coll->pos_curr = natural_damage_coll->offset;
        gmCollisionGetWorldPosition(natural_damage_parts->mtx_translate,
                                    &attack_coll->pos_curr);
        attack_coll->pos_prev = attack_coll->pos_curr;

        natural_queue_before = target_fp->damage_queue;
        natural_percent_before = target_fp->percent_damage;
        sNdsFighterDashRunHitLogID = 0u;
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = NULL;
        ftMainProcSearchHitAll(target_gobj);
        damage = ftParamGetCapturedDamage(target_fp, attack_coll->damage);
        natural_queue_after = target_fp->damage_queue;
        hitlog = (sNdsFighterDashRunHitLogID != 0u) ?
            &sNdsFighterDashRunHitLogs[0] : NULL;

        target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
        ftMainProcParams(target_gobj);
        natural_percent_after = target_fp->percent_damage;
        natural_hitlag = target_fp->hitlag_tics;

        if ((sNdsFighterDashRunHitLogID == 1u) &&
            (hitlog != NULL) &&
            (hitlog->damage_coll == natural_damage_coll) &&
            (hitlog->attack_coll == attack_coll) &&
            (attack_coll->attack_records[0].victim_gobj == target_gobj) &&
            (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
                FALSE) &&
            (natural_queue_after == (natural_queue_before + damage)) &&
            (natural_percent_after ==
                (natural_percent_before + natural_queue_after)) &&
            (natural_hitlag > 0) &&
            (natural_damage_coll->joint_id != 0))
        {
            gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
                NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_NATURAL;
            sNdsFighterDashRunHitLogID = 0u;
            ftMainProcSearchHitAll(target_gobj);
            if ((sNdsFighterDashRunHitLogID == 0u) &&
                (target_fp->damage_queue == 0) &&
                (target_fp->percent_damage == natural_percent_after) &&
                (attack_coll->attack_records[0].victim_gobj ==
                    target_gobj) &&
                (attack_coll->attack_records[0].victim_flags.is_interact_hurt
                    != FALSE))
            {
                gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
                    NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_REPEAT;
            }
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    sNdsFighterDashRunProcParamsLagStartCount = saved_lagstart_count;

    attack_coll = &fp->attack_colls[attack_id];
    natural_damage_coll = &target_fp->damage_colls[1];
    natural_damage_parts = (natural_damage_coll->joint != NULL) ?
        ftGetParts(natural_damage_coll->joint) : NULL;
    if ((target_fp->damage_colls[0].hitstatus != nGMHitStatusNone) &&
        (natural_damage_coll->hitstatus == nGMHitStatusNormal) &&
        (natural_damage_parts != NULL) &&
        (natural_damage_parts->vec_scale.x != 0.0F) &&
        (natural_damage_parts->vec_scale.y != 0.0F) &&
        (natural_damage_parts->vec_scale.z != 0.0F))
    {
        target_fp->damage_queue = 0;
        target_fp->damage_lag = 0;
        target_fp->hitlag_mul = 1.0F;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        target_fp->is_shield = FALSE;
        target_fp->ga = nMPKineticsGround;
        target_fp->capture_gobj = NULL;
        target_fp->damage_colls[0].hitstatus = nGMHitStatusIntangible;
        fp->is_catch_or_capture = FALSE;
        fp->throw_gobj = NULL;
        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            if (i != attack_id)
            {
                fp->attack_colls[i].attack_state = nGMAttackStateOff;
            }
        }
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        attack_coll->pos_curr = natural_damage_coll->offset;
        gmCollisionGetWorldPosition(natural_damage_parts->mtx_translate,
                                    &attack_coll->pos_curr);
        attack_coll->pos_prev = attack_coll->pos_curr;

        natural_queue_before = target_fp->damage_queue;
        natural_percent_before = target_fp->percent_damage;
        sNdsFighterDashRunHitLogID = 0u;
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = NULL;
        ftMainProcSearchHitAll(target_gobj);
        damage = ftParamGetCapturedDamage(target_fp, attack_coll->damage);
        natural_queue_after = target_fp->damage_queue;
        hitlog = (sNdsFighterDashRunHitLogID != 0u) ?
            &sNdsFighterDashRunHitLogs[0] : NULL;

        target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
        ftMainProcParams(target_gobj);
        natural_percent_after = target_fp->percent_damage;
        natural_hitlag = target_fp->hitlag_tics;

        if ((sNdsFighterDashRunHitLogID == 1u) &&
            (hitlog != NULL) &&
            (hitlog->damage_coll == natural_damage_coll) &&
            (hitlog->attack_coll == attack_coll) &&
            (attack_coll->attack_records[0].victim_gobj == target_gobj) &&
            (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
                FALSE) &&
            (natural_queue_after == (natural_queue_before + damage)) &&
            (natural_percent_after ==
                (natural_percent_before + natural_queue_after)) &&
            (natural_hitlag > 0) &&
            (natural_damage_coll->joint_id != 0))
        {
            gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
                NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT1;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    sNdsFighterDashRunProcParamsLagStartCount = saved_lagstart_count;

    attack_coll = &fp->attack_colls[attack_id];
    natural_damage_coll = &target_fp->damage_colls[2];
    natural_damage_parts = (natural_damage_coll->joint != NULL) ?
        ftGetParts(natural_damage_coll->joint) : NULL;
    if ((target_fp->damage_colls[0].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[1].hitstatus != nGMHitStatusNone) &&
        (natural_damage_coll->hitstatus == nGMHitStatusNormal) &&
        (natural_damage_parts != NULL) &&
        (natural_damage_parts->vec_scale.x != 0.0F) &&
        (natural_damage_parts->vec_scale.y != 0.0F) &&
        (natural_damage_parts->vec_scale.z != 0.0F))
    {
        target_fp->damage_queue = 0;
        target_fp->damage_lag = 0;
        target_fp->hitlag_mul = 1.0F;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        target_fp->is_shield = FALSE;
        target_fp->ga = nMPKineticsGround;
        target_fp->capture_gobj = NULL;
        target_fp->damage_colls[0].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[1].hitstatus = nGMHitStatusIntangible;
        fp->is_catch_or_capture = FALSE;
        fp->throw_gobj = NULL;
        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            if (i != attack_id)
            {
                fp->attack_colls[i].attack_state = nGMAttackStateOff;
            }
        }
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        attack_coll->pos_curr = natural_damage_coll->offset;
        gmCollisionGetWorldPosition(natural_damage_parts->mtx_translate,
                                    &attack_coll->pos_curr);
        attack_coll->pos_prev = attack_coll->pos_curr;

        natural_queue_before = target_fp->damage_queue;
        natural_percent_before = target_fp->percent_damage;
        sNdsFighterDashRunHitLogID = 0u;
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = NULL;
        ftMainProcSearchHitAll(target_gobj);
        damage = ftParamGetCapturedDamage(target_fp, attack_coll->damage);
        natural_queue_after = target_fp->damage_queue;
        hitlog = (sNdsFighterDashRunHitLogID != 0u) ?
            &sNdsFighterDashRunHitLogs[0] : NULL;

        target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
        ftMainProcParams(target_gobj);
        natural_percent_after = target_fp->percent_damage;
        natural_hitlag = target_fp->hitlag_tics;

        if ((sNdsFighterDashRunHitLogID == 1u) &&
            (hitlog != NULL) &&
            (hitlog->damage_coll == natural_damage_coll) &&
            (hitlog->attack_coll == attack_coll) &&
            (attack_coll->attack_records[0].victim_gobj == target_gobj) &&
            (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
                FALSE) &&
            (natural_queue_after == (natural_queue_before + damage)) &&
            (natural_percent_after ==
                (natural_percent_before + natural_queue_after)) &&
            (natural_hitlag > 0) &&
            (natural_damage_coll->joint_id != 0))
        {
            gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
                NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT2;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    sNdsFighterDashRunProcParamsLagStartCount = saved_lagstart_count;

    attack_coll = &fp->attack_colls[attack_id];
    natural_damage_coll = &target_fp->damage_colls[4];
    natural_damage_parts = (natural_damage_coll->joint != NULL) ?
        ftGetParts(natural_damage_coll->joint) : NULL;
    if ((target_fp->damage_colls[0].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[1].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[2].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[3].hitstatus != nGMHitStatusNone) &&
        (natural_damage_coll->hitstatus == nGMHitStatusNormal) &&
        (natural_damage_parts != NULL) &&
        (natural_damage_parts->vec_scale.x != 0.0F) &&
        (natural_damage_parts->vec_scale.y != 0.0F) &&
        (natural_damage_parts->vec_scale.z != 0.0F))
    {
        target_fp->damage_queue = 0;
        target_fp->damage_lag = 0;
        target_fp->hitlag_mul = 1.0F;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        target_fp->is_shield = FALSE;
        target_fp->ga = nMPKineticsGround;
        target_fp->capture_gobj = NULL;
        target_fp->damage_colls[0].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[1].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[2].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[3].hitstatus = nGMHitStatusIntangible;
        fp->is_catch_or_capture = FALSE;
        fp->throw_gobj = NULL;
        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            if (i != attack_id)
            {
                fp->attack_colls[i].attack_state = nGMAttackStateOff;
            }
        }
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        attack_coll->pos_curr = natural_damage_coll->offset;
        gmCollisionGetWorldPosition(natural_damage_parts->mtx_translate,
                                    &attack_coll->pos_curr);
        attack_coll->pos_prev = attack_coll->pos_curr;

        natural_queue_before = target_fp->damage_queue;
        natural_percent_before = target_fp->percent_damage;
        sNdsFighterDashRunHitLogID = 0u;
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = NULL;
        ftMainProcSearchHitAll(target_gobj);
        damage = ftParamGetCapturedDamage(target_fp, attack_coll->damage);
        natural_queue_after = target_fp->damage_queue;
        hitlog = (sNdsFighterDashRunHitLogID != 0u) ?
            &sNdsFighterDashRunHitLogs[0] : NULL;

        target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
        ftMainProcParams(target_gobj);
        natural_percent_after = target_fp->percent_damage;
        natural_hitlag = target_fp->hitlag_tics;

        if ((sNdsFighterDashRunHitLogID == 1u) &&
            (hitlog != NULL) &&
            (hitlog->damage_coll == natural_damage_coll) &&
            (hitlog->attack_coll == attack_coll) &&
            (attack_coll->attack_records[0].victim_gobj == target_gobj) &&
            (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
                FALSE) &&
            (natural_queue_after == (natural_queue_before + damage)) &&
            (natural_percent_after ==
                (natural_percent_before + natural_queue_after)) &&
            (natural_hitlag > 0) &&
            (natural_damage_coll->joint_id != 0))
        {
            gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
                NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT4;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    sNdsFighterDashRunProcParamsLagStartCount = saved_lagstart_count;

    attack_coll = &fp->attack_colls[attack_id];
    natural_damage_coll = &target_fp->damage_colls[5];
    natural_damage_parts = (natural_damage_coll->joint != NULL) ?
        ftGetParts(natural_damage_coll->joint) : NULL;
    if ((target_fp->damage_colls[0].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[1].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[2].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[3].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[4].hitstatus != nGMHitStatusNone) &&
        (natural_damage_coll->hitstatus == nGMHitStatusNormal) &&
        (natural_damage_parts != NULL) &&
        (natural_damage_parts->vec_scale.x != 0.0F) &&
        (natural_damage_parts->vec_scale.y != 0.0F) &&
        (natural_damage_parts->vec_scale.z != 0.0F))
    {
        target_fp->damage_queue = 0;
        target_fp->damage_lag = 0;
        target_fp->hitlag_mul = 1.0F;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        target_fp->is_shield = FALSE;
        target_fp->ga = nMPKineticsGround;
        target_fp->capture_gobj = NULL;
        target_fp->damage_colls[0].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[1].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[2].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[3].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[4].hitstatus = nGMHitStatusIntangible;
        fp->is_catch_or_capture = FALSE;
        fp->throw_gobj = NULL;
        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            if (i != attack_id)
            {
                fp->attack_colls[i].attack_state = nGMAttackStateOff;
            }
        }
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        attack_coll->pos_curr = natural_damage_coll->offset;
        gmCollisionGetWorldPosition(natural_damage_parts->mtx_translate,
                                    &attack_coll->pos_curr);
        attack_coll->pos_prev = attack_coll->pos_curr;

        natural_queue_before = target_fp->damage_queue;
        natural_percent_before = target_fp->percent_damage;
        sNdsFighterDashRunHitLogID = 0u;
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = NULL;
        ftMainProcSearchHitAll(target_gobj);
        damage = ftParamGetCapturedDamage(target_fp, attack_coll->damage);
        natural_queue_after = target_fp->damage_queue;
        hitlog = (sNdsFighterDashRunHitLogID != 0u) ?
            &sNdsFighterDashRunHitLogs[0] : NULL;

        target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
        ftMainProcParams(target_gobj);
        natural_percent_after = target_fp->percent_damage;
        natural_hitlag = target_fp->hitlag_tics;

        if ((sNdsFighterDashRunHitLogID == 1u) &&
            (hitlog != NULL) &&
            (hitlog->damage_coll == natural_damage_coll) &&
            (hitlog->attack_coll == attack_coll) &&
            (attack_coll->attack_records[0].victim_gobj == target_gobj) &&
            (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
                FALSE) &&
            (natural_queue_after == (natural_queue_before + damage)) &&
            (natural_percent_after ==
                (natural_percent_before + natural_queue_after)) &&
            (natural_hitlag > 0) &&
            (natural_damage_coll->joint_id != 0))
        {
            gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
                NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT5;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    sNdsFighterDashRunProcParamsLagStartCount = saved_lagstart_count;

    attack_coll = &fp->attack_colls[attack_id];
    natural_damage_coll = &target_fp->damage_colls[6];
    natural_damage_parts = (natural_damage_coll->joint != NULL) ?
        ftGetParts(natural_damage_coll->joint) : NULL;
    if ((target_fp->damage_colls[0].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[1].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[2].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[3].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[4].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[5].hitstatus != nGMHitStatusNone) &&
        (natural_damage_coll->hitstatus == nGMHitStatusNormal) &&
        (natural_damage_parts != NULL) &&
        (natural_damage_parts->vec_scale.x != 0.0F) &&
        (natural_damage_parts->vec_scale.y != 0.0F) &&
        (natural_damage_parts->vec_scale.z != 0.0F))
    {
        target_fp->damage_queue = 0;
        target_fp->damage_lag = 0;
        target_fp->hitlag_mul = 1.0F;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        target_fp->is_shield = FALSE;
        target_fp->ga = nMPKineticsGround;
        target_fp->capture_gobj = NULL;
        target_fp->damage_colls[0].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[1].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[2].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[3].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[4].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[5].hitstatus = nGMHitStatusIntangible;
        fp->is_catch_or_capture = FALSE;
        fp->throw_gobj = NULL;
        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            if (i != attack_id)
            {
                fp->attack_colls[i].attack_state = nGMAttackStateOff;
            }
        }
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        attack_coll->pos_curr = natural_damage_coll->offset;
        gmCollisionGetWorldPosition(natural_damage_parts->mtx_translate,
                                    &attack_coll->pos_curr);
        attack_coll->pos_prev = attack_coll->pos_curr;

        natural_queue_before = target_fp->damage_queue;
        natural_percent_before = target_fp->percent_damage;
        sNdsFighterDashRunHitLogID = 0u;
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = NULL;
        ftMainProcSearchHitAll(target_gobj);
        damage = ftParamGetCapturedDamage(target_fp, attack_coll->damage);
        natural_queue_after = target_fp->damage_queue;
        hitlog = (sNdsFighterDashRunHitLogID != 0u) ?
            &sNdsFighterDashRunHitLogs[0] : NULL;

        target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
        ftMainProcParams(target_gobj);
        natural_percent_after = target_fp->percent_damage;
        natural_hitlag = target_fp->hitlag_tics;

        if ((sNdsFighterDashRunHitLogID == 1u) &&
            (hitlog != NULL) &&
            (hitlog->damage_coll == natural_damage_coll) &&
            (hitlog->attack_coll == attack_coll) &&
            (attack_coll->attack_records[0].victim_gobj == target_gobj) &&
            (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
                FALSE) &&
            (natural_queue_after == (natural_queue_before + damage)) &&
            (natural_percent_after ==
                (natural_percent_before + natural_queue_after)) &&
            (natural_hitlag > 0) &&
            (natural_damage_coll->joint_id != 0))
        {
            gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
                NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT6;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    sNdsFighterDashRunProcParamsLagStartCount = saved_lagstart_count;

    attack_coll = &fp->attack_colls[attack_id];
    natural_damage_coll = &target_fp->damage_colls[7];
    natural_damage_parts = (natural_damage_coll->joint != NULL) ?
        ftGetParts(natural_damage_coll->joint) : NULL;
    if ((target_fp->damage_colls[0].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[1].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[2].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[3].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[4].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[5].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[6].hitstatus != nGMHitStatusNone) &&
        (natural_damage_coll->hitstatus == nGMHitStatusNormal) &&
        (natural_damage_parts != NULL) &&
        (natural_damage_parts->vec_scale.x != 0.0F) &&
        (natural_damage_parts->vec_scale.y != 0.0F) &&
        (natural_damage_parts->vec_scale.z != 0.0F))
    {
        target_fp->damage_queue = 0;
        target_fp->damage_lag = 0;
        target_fp->hitlag_mul = 1.0F;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        target_fp->is_shield = FALSE;
        target_fp->ga = nMPKineticsGround;
        target_fp->capture_gobj = NULL;
        target_fp->damage_colls[0].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[1].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[2].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[3].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[4].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[5].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[6].hitstatus = nGMHitStatusIntangible;
        fp->is_catch_or_capture = FALSE;
        fp->throw_gobj = NULL;
        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            if (i != attack_id)
            {
                fp->attack_colls[i].attack_state = nGMAttackStateOff;
            }
        }
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        attack_coll->pos_curr = natural_damage_coll->offset;
        gmCollisionGetWorldPosition(natural_damage_parts->mtx_translate,
                                    &attack_coll->pos_curr);
        attack_coll->pos_prev = attack_coll->pos_curr;

        natural_queue_before = target_fp->damage_queue;
        natural_percent_before = target_fp->percent_damage;
        sNdsFighterDashRunHitLogID = 0u;
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = NULL;
        ftMainProcSearchHitAll(target_gobj);
        damage = ftParamGetCapturedDamage(target_fp, attack_coll->damage);
        natural_queue_after = target_fp->damage_queue;
        hitlog = (sNdsFighterDashRunHitLogID != 0u) ?
            &sNdsFighterDashRunHitLogs[0] : NULL;

        target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
        ftMainProcParams(target_gobj);
        natural_percent_after = target_fp->percent_damage;
        natural_hitlag = target_fp->hitlag_tics;

        if ((sNdsFighterDashRunHitLogID == 1u) &&
            (hitlog != NULL) &&
            (hitlog->damage_coll == natural_damage_coll) &&
            (hitlog->attack_coll == attack_coll) &&
            (attack_coll->attack_records[0].victim_gobj == target_gobj) &&
            (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
                FALSE) &&
            (natural_queue_after == (natural_queue_before + damage)) &&
            (natural_percent_after ==
                (natural_percent_before + natural_queue_after)) &&
            (natural_hitlag > 0) &&
            (natural_damage_coll->joint_id != 0))
        {
            gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
                NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT7;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    sNdsFighterDashRunProcParamsLagStartCount = saved_lagstart_count;

    attack_coll = &fp->attack_colls[attack_id];
    natural_damage_coll = &target_fp->damage_colls[8];
    natural_damage_parts = (natural_damage_coll->joint != NULL) ?
        ftGetParts(natural_damage_coll->joint) : NULL;
    if ((target_fp->damage_colls[0].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[1].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[2].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[3].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[4].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[5].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[6].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[7].hitstatus != nGMHitStatusNone) &&
        (natural_damage_coll->hitstatus == nGMHitStatusNormal) &&
        (natural_damage_parts != NULL) &&
        (natural_damage_parts->vec_scale.x != 0.0F) &&
        (natural_damage_parts->vec_scale.y != 0.0F) &&
        (natural_damage_parts->vec_scale.z != 0.0F))
    {
        target_fp->damage_queue = 0;
        target_fp->damage_lag = 0;
        target_fp->hitlag_mul = 1.0F;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        target_fp->is_shield = FALSE;
        target_fp->ga = nMPKineticsGround;
        target_fp->capture_gobj = NULL;
        target_fp->damage_colls[0].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[1].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[2].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[3].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[4].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[5].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[6].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[7].hitstatus = nGMHitStatusIntangible;
        fp->is_catch_or_capture = FALSE;
        fp->throw_gobj = NULL;
        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            if (i != attack_id)
            {
                fp->attack_colls[i].attack_state = nGMAttackStateOff;
            }
        }
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        attack_coll->pos_curr = natural_damage_coll->offset;
        gmCollisionGetWorldPosition(natural_damage_parts->mtx_translate,
                                    &attack_coll->pos_curr);
        attack_coll->pos_prev = attack_coll->pos_curr;

        natural_queue_before = target_fp->damage_queue;
        natural_percent_before = target_fp->percent_damage;
        sNdsFighterDashRunHitLogID = 0u;
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = NULL;
        ftMainProcSearchHitAll(target_gobj);
        damage = ftParamGetCapturedDamage(target_fp, attack_coll->damage);
        natural_queue_after = target_fp->damage_queue;
        hitlog = (sNdsFighterDashRunHitLogID != 0u) ?
            &sNdsFighterDashRunHitLogs[0] : NULL;

        target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
        ftMainProcParams(target_gobj);
        natural_percent_after = target_fp->percent_damage;
        natural_hitlag = target_fp->hitlag_tics;

        if ((sNdsFighterDashRunHitLogID == 1u) &&
            (hitlog != NULL) &&
            (hitlog->damage_coll == natural_damage_coll) &&
            (hitlog->attack_coll == attack_coll) &&
            (attack_coll->attack_records[0].victim_gobj == target_gobj) &&
            (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
                FALSE) &&
            (natural_queue_after == (natural_queue_before + damage)) &&
            (natural_percent_after ==
                (natural_percent_before + natural_queue_after)) &&
            (natural_hitlag > 0) &&
            (natural_damage_coll->joint_id != 0))
        {
            gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
                NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT8;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    sNdsFighterDashRunProcParamsLagStartCount = saved_lagstart_count;

    attack_coll = &fp->attack_colls[attack_id];
    natural_damage_coll = &target_fp->damage_colls[9];
    natural_damage_parts = (natural_damage_coll->joint != NULL) ?
        ftGetParts(natural_damage_coll->joint) : NULL;
    if ((target_fp->damage_colls[0].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[1].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[2].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[3].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[4].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[5].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[6].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[7].hitstatus != nGMHitStatusNone) &&
        (target_fp->damage_colls[8].hitstatus != nGMHitStatusNone) &&
        (natural_damage_coll->hitstatus == nGMHitStatusNormal) &&
        (natural_damage_parts != NULL) &&
        (natural_damage_parts->vec_scale.x != 0.0F) &&
        (natural_damage_parts->vec_scale.y != 0.0F) &&
        (natural_damage_parts->vec_scale.z != 0.0F))
    {
        target_fp->damage_queue = 0;
        target_fp->damage_lag = 0;
        target_fp->hitlag_mul = 1.0F;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        target_fp->is_shield = FALSE;
        target_fp->ga = nMPKineticsGround;
        target_fp->capture_gobj = NULL;
        target_fp->damage_colls[0].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[1].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[2].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[3].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[4].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[5].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[6].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[7].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[8].hitstatus = nGMHitStatusIntangible;
        fp->is_catch_or_capture = FALSE;
        fp->throw_gobj = NULL;
        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            if (i != attack_id)
            {
                fp->attack_colls[i].attack_state = nGMAttackStateOff;
            }
        }
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        attack_coll->pos_curr = natural_damage_coll->offset;
        gmCollisionGetWorldPosition(natural_damage_parts->mtx_translate,
                                    &attack_coll->pos_curr);
        attack_coll->pos_prev = attack_coll->pos_curr;

        natural_queue_before = target_fp->damage_queue;
        natural_percent_before = target_fp->percent_damage;
        sNdsFighterDashRunHitLogID = 0u;
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = NULL;
        ftMainProcSearchHitAll(target_gobj);
        damage = ftParamGetCapturedDamage(target_fp, attack_coll->damage);
        natural_queue_after = target_fp->damage_queue;
        hitlog = (sNdsFighterDashRunHitLogID != 0u) ?
            &sNdsFighterDashRunHitLogs[0] : NULL;

        target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
        ftMainProcParams(target_gobj);
        natural_percent_after = target_fp->percent_damage;
        natural_hitlag = target_fp->hitlag_tics;

        if ((sNdsFighterDashRunHitLogID == 1u) &&
            (hitlog != NULL) &&
            (hitlog->damage_coll == natural_damage_coll) &&
            (hitlog->attack_coll == attack_coll) &&
            (attack_coll->attack_records[0].victim_gobj == target_gobj) &&
            (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
                FALSE) &&
            (natural_queue_after == (natural_queue_before + damage)) &&
            (natural_percent_after ==
                (natural_percent_before + natural_queue_after)) &&
            (natural_hitlag > 0) &&
            (natural_damage_coll->joint_id != 0))
        {
            gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
                NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT9;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    attacker_gobj->link_next = saved_attacker_link_next;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    sNdsFighterDashRunProcParamsLagStartCount = saved_lagstart_count;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
        NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_RESTORE;

    return ((gNdsStageMPLiveHitDamageLoopHurtboxDamageMask & 0x7ffffu) ==
            0x7ffffu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeThrowAttribution(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp)
{
    static FTStruct saved_attacker;
    static FTStruct saved_target;
    FTHitLog saved_hitlog;
    FTAttackColl *attack_coll;
    FTDamageColl *damage_coll;
    FTHitLog *hitlog;
    GObj *target_gobj;
    u32 saved_hitlog_id;
    u32 slot = gNdsStageMPLiveHitDamageLoopHurtboxFirstHitSlot;
    u32 mask = 0u;
    s32 damage;
    u8 direct_player;
    s32 direct_player_num;
    u8 attacker_player;
    s32 attacker_player_num;

    if ((fp == NULL) || (target_fp == NULL) ||
        (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (slot >= FTDAMAGECOLL_NUM_MAX) ||
        (fp->fighter_gobj == NULL))
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    damage_coll = &target_fp->damage_colls[slot];
    target_gobj = target_fp->fighter_gobj;
    if ((attack_coll->attack_state == nGMAttackStateOff) ||
        (target_gobj == NULL) ||
        (damage_coll->hitstatus != nGMHitStatusNormal) ||
        (damage_coll->joint == NULL))
    {
        return FALSE;
    }

    saved_attacker = *fp;
    saved_target = *target_fp;
    saved_hitlog = sNdsFighterDashRunHitLogs[0];
    saved_hitlog_id = sNdsFighterDashRunHitLogID;
    direct_player = fp->player;
    direct_player_num = fp->player_num;

    ftParamSetThrowParams(fp, target_gobj);
    if ((fp->throw_gobj == target_gobj) &&
        (fp->throw_player == target_fp->player) &&
        (fp->throw_player_num == target_fp->player_num) &&
        (fp->throw_player != direct_player))
    {
        mask |= NDS_STAGE_MPLIVEHIT_THROW_ATTRIB_SEED;
    }

    if (fp->throw_gobj != NULL)
    {
        attacker_player = fp->throw_player;
        attacker_player_num = fp->throw_player_num;
    }
    else
    {
        attacker_player = fp->player;
        attacker_player_num = fp->player_num;
    }
    if ((attacker_player == fp->throw_player) &&
        (attacker_player_num == fp->throw_player_num) &&
        (attacker_player != direct_player))
    {
        mask |= NDS_STAGE_MPLIVEHIT_THROW_ATTRIB_SOURCE;
    }

    target_fp->damage_queue = 0;
    target_fp->damage_lag = 0;
    target_fp->is_damage_resist = FALSE;
    damage = ndsFighterDashRunGetCapturedDamage(target_fp,
                                                attack_coll->damage);
    if (fp->attack_damage < damage)
    {
        fp->attack_damage = damage;
    }
    if (ndsFighterDashRunCheckGetUpdateDamageNormal(target_fp,
                                                    &damage) == FALSE)
    {
        goto done;
    }

    sNdsFighterDashRunHitLogID = 0u;
    hitlog = &sNdsFighterDashRunHitLogs[sNdsFighterDashRunHitLogID++];
    hitlog->attacker_object_class = nFTHitLogObjectFighter;
    hitlog->attack_coll = attack_coll;
    hitlog->attack_id = (s32)attack_id;
    hitlog->attacker_gobj = fp->fighter_gobj;
    hitlog->damage_coll = damage_coll;
    hitlog->attacker_player = attacker_player;
    hitlog->attacker_player_num = attacker_player_num;
    if ((sNdsFighterDashRunHitLogID == 1u) &&
        (hitlog->attacker_player == fp->throw_player) &&
        (hitlog->attacker_player_num == fp->throw_player_num) &&
        (hitlog->attacker_player != direct_player))
    {
        mask |= NDS_STAGE_MPLIVEHIT_THROW_ATTRIB_HITLOG;
    }

    ftParamUpdatePlayerBattleStats(attacker_player, target_fp->player,
                                   damage);
    ftParamUpdateStaleQueue(attacker_player, target_fp->player,
                            (s32)attack_coll->motion_attack_id,
                            attack_coll->motion_count);
    mask |= NDS_STAGE_MPLIVEHIT_THROW_ATTRIB_STATS;

done:
    gNdsStageMPLiveHitDamageLoopThrowAttribMask = mask;
    gNdsStageMPLiveHitDamageLoopThrowAttribDirectPlayer =
        (u32)direct_player;
    gNdsStageMPLiveHitDamageLoopThrowAttribDirectPlayerNum =
        direct_player_num;
    gNdsStageMPLiveHitDamageLoopThrowAttribOwnerPlayer =
        (u32)fp->throw_player;
    gNdsStageMPLiveHitDamageLoopThrowAttribOwnerPlayerNum =
        fp->throw_player_num;
    gNdsStageMPLiveHitDamageLoopThrowAttribHitLogPlayer =
        (sNdsFighterDashRunHitLogID != 0u) ?
            (u32)sNdsFighterDashRunHitLogs[0].attacker_player : 0u;
    gNdsStageMPLiveHitDamageLoopThrowAttribHitLogPlayerNum =
        (sNdsFighterDashRunHitLogID != 0u) ?
            sNdsFighterDashRunHitLogs[0].attacker_player_num : -1;

    *fp = saved_attacker;
    *target_fp = saved_target;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    gNdsStageMPLiveHitDamageLoopThrowAttribMask |=
        NDS_STAGE_MPLIVEHIT_THROW_ATTRIB_RESTORE;

    return ((gNdsStageMPLiveHitDamageLoopThrowAttribMask & 0x1fu) ==
            0x1fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeAttackClashStats(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp)
{
    static FTStruct saved_this;
    static FTStruct saved_other;
    FTAttackColl *this_hit;
    FTAttackColl *other_hit;
    GObj *this_gobj;
    GObj *other_gobj;
    GObj *saved_fighter_link_head;
    GObj *saved_this_link_next;
    GObj *saved_other_link_next;
    DObj *this_root;
    DObj *other_root;
    f32 saved_this_x;
    f32 saved_other_x;
    sb32 saved_attack_detect[FTATTACKCOLL_NUM_MAX];
    sb32 saved_damage_detect[FTATTACKCOLL_NUM_MAX];
    u32 other_id = 0u;
    u32 i;
    u32 mask = 0u;
    u32 effect_count = 0u;
    f32 this_expected_rebound;
    f32 other_expected_rebound;

    if ((fp == NULL) || (target_fp == NULL) ||
        (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (fp->fighter_gobj == NULL) || (target_fp->fighter_gobj == NULL))
    {
        return FALSE;
    }

    this_gobj = fp->fighter_gobj;
    other_gobj = target_fp->fighter_gobj;
    this_root = DObjGetStruct(this_gobj);
    other_root = DObjGetStruct(other_gobj);
    if ((this_root == NULL) || (other_root == NULL))
    {
        return FALSE;
    }

    saved_this = *fp;
    saved_other = *target_fp;
    saved_fighter_link_head = gGCCommonLinks[nGCCommonLinkIDFighter];
    saved_this_link_next = this_gobj->link_next;
    saved_other_link_next = other_gobj->link_next;
    saved_this_x = this_root->translate.vec.f.x;
    saved_other_x = other_root->translate.vec.f.x;
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        saved_attack_detect[i] = gFTMainIsAttackDetect[i];
        saved_damage_detect[i] = gFTMainIsDamageDetect[i];
    }

    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        fp->attack_colls[i].attack_state = nGMAttackStateOff;
        target_fp->attack_colls[i].attack_state = nGMAttackStateOff;
    }
    this_hit = &fp->attack_colls[attack_id];
    other_hit = &target_fp->attack_colls[other_id];
    ftParamClearAttackRecordID(fp, (s32)attack_id);
    ftParamClearAttackRecordID(target_fp, (s32)other_id);

    this_hit->attack_state = nGMAttackStateInterpolate;
    this_hit->group_id = 2u;
    this_hit->damage = 24;
    this_hit->can_rebound = TRUE;
    this_hit->is_hit_ground = TRUE;
    this_hit->is_hit_air = TRUE;
    this_hit->pos_curr.x = 0.0F;
    this_hit->pos_curr.y = 0.0F;
    this_hit->pos_curr.z = 0.0F;
    this_hit->pos_prev = this_hit->pos_curr;
    other_hit->attack_state = nGMAttackStateInterpolate;
    other_hit->group_id = 4u;
    other_hit->damage = 18;
    other_hit->can_rebound = TRUE;
    other_hit->is_hit_ground = TRUE;
    other_hit->is_hit_air = TRUE;
    other_hit->pos_curr = this_hit->pos_curr;
    other_hit->pos_prev = other_hit->pos_curr;

    fp->ga = nMPKineticsGround;
    target_fp->ga = nMPKineticsGround;
    fp->is_catch_or_capture = FALSE;
    fp->throw_gobj = NULL;
    target_fp->is_catch_or_capture = FALSE;
    target_fp->throw_gobj = NULL;
    fp->special_hitstatus = nGMHitStatusIntangible;
    fp->star_hitstatus = nGMHitStatusIntangible;
    fp->hitstatus = nGMHitStatusIntangible;
    fp->attack_shield_push = 0;
    target_fp->attack_shield_push = 0;
    fp->attack_rebound = 0.0F;
    target_fp->attack_rebound = 0.0F;
    fp->hit_lr = 0;
    target_fp->hit_lr = 0;
    this_root->translate.vec.f.x = 80.0F;
    other_root->translate.vec.f.x = -80.0F;
    gFTMainIsAttackDetect[attack_id] = TRUE;
    gFTMainIsDamageDetect[other_id] = TRUE;
    gNdsStageMPLiveHitDamageLoopAttackClashEffectCount = 0u;

    gGCCommonLinks[nGCCommonLinkIDFighter] = this_gobj;
    this_gobj->link_next = other_gobj;
    other_gobj->link_next = NULL;
    ftMainSearchHitFighter(this_gobj);

#if defined(REGION_US)
    this_expected_rebound = (this_hit->damage * 1.62F) + 4.0F;
    other_expected_rebound = (other_hit->damage * 1.62F) + 4.0F;
#else
    this_expected_rebound = (this_hit->damage * 1.75F) + 4.0F;
    other_expected_rebound = (other_hit->damage * 1.75F) + 4.0F;
#endif

    gNdsStageMPLiveHitDamageLoopAttackClashThisGroup =
        fp->attack_colls[attack_id].attack_records[0].victim_flags.group_id;
    gNdsStageMPLiveHitDamageLoopAttackClashOtherGroup =
        target_fp->attack_colls[other_id].attack_records[0]
            .victim_flags.group_id;
    gNdsStageMPLiveHitDamageLoopAttackClashThisPush =
        fp->attack_shield_push;
    gNdsStageMPLiveHitDamageLoopAttackClashOtherPush =
        target_fp->attack_shield_push;
    gNdsStageMPLiveHitDamageLoopAttackClashThisReboundMilli =
        ndsFloatToMilliSigned(fp->attack_rebound);
    gNdsStageMPLiveHitDamageLoopAttackClashOtherReboundMilli =
        ndsFloatToMilliSigned(target_fp->attack_rebound);
    gNdsStageMPLiveHitDamageLoopAttackClashThisLR = fp->hit_lr;
    gNdsStageMPLiveHitDamageLoopAttackClashOtherLR = target_fp->hit_lr;
    effect_count = gNdsStageMPLiveHitDamageLoopAttackClashEffectCount;

    if ((gNdsStageMPLiveHitDamageLoopAttackClashThisGroup ==
            other_hit->group_id) &&
        (gFTMainIsAttackDetect[attack_id] == FALSE))
    {
        mask |= NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_THIS;
    }
    if ((gNdsStageMPLiveHitDamageLoopAttackClashOtherGroup ==
            this_hit->group_id) &&
        (gFTMainIsDamageDetect[other_id] == FALSE))
    {
        mask |= NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_OTHER;
    }
    if ((fp->attack_shield_push == this_hit->damage) &&
        (fp->attack_rebound == this_expected_rebound) &&
        (fp->hit_lr == -1))
    {
        mask |= NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_THIS_REB;
    }
    if ((target_fp->attack_shield_push == other_hit->damage) &&
        (target_fp->attack_rebound == other_expected_rebound) &&
        (target_fp->hit_lr == +1))
    {
        mask |= NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_OTHER_REB;
    }
    if (effect_count == 2u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_EFFECT;
    }

    *fp = saved_this;
    *target_fp = saved_other;
    this_root->translate.vec.f.x = saved_this_x;
    other_root->translate.vec.f.x = saved_other_x;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    this_gobj->link_next = saved_this_link_next;
    other_gobj->link_next = saved_other_link_next;
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        gFTMainIsAttackDetect[i] = saved_attack_detect[i];
        gFTMainIsDamageDetect[i] = saved_damage_detect[i];
    }
    mask |= NDS_STAGE_MPLIVEHIT_ATTACK_CLASH_RESTORE;

    gNdsStageMPLiveHitDamageLoopAttackClashMask = mask;
    return ((mask & 0x3fu) == 0x3fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeCatchStats(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp)
{
    static FTStruct saved_attacker;
    static FTStruct saved_target;
    FTAttackColl *attack_coll;
    GObj *target_gobj;
    DObj *attacker_root;
    DObj *target_root;
    f32 saved_attacker_x;
    f32 saved_target_x;
    sb32 saved_attack_detect[FTATTACKCOLL_NUM_MAX];
    u32 mask = 0u;
    u32 i;
    f32 dist;

    if ((fp == NULL) || (target_fp == NULL) ||
        (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (fp->fighter_gobj == NULL) || (target_fp->fighter_gobj == NULL))
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    target_gobj = target_fp->fighter_gobj;
    attacker_root = DObjGetStruct(fp->fighter_gobj);
    target_root = DObjGetStruct(target_gobj);
    if ((attack_coll->attack_state == nGMAttackStateOff) ||
        (attacker_root == NULL) || (target_root == NULL))
    {
        return FALSE;
    }

    saved_attacker = *fp;
    saved_target = *target_fp;
    saved_attacker_x = attacker_root->translate.vec.f.x;
    saved_target_x = target_root->translate.vec.f.x;
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        saved_attack_detect[i] = gFTMainIsAttackDetect[i];
    }

    ftParamClearAttackRecordID(fp, (s32)attack_id);
    gFTMainIsAttackDetect[attack_id] = TRUE;
    fp->search_gobj = NULL;
    fp->search_gobj_dist = 9999.0F;
    attacker_root->translate.vec.f.x = 120.0F;
    target_root->translate.vec.f.x = -40.0F;

    gNdsStageMPLiveHitDamageLoopCatchStatBeforeMilli =
        ndsFloatToMilliSigned(fp->search_gobj_dist);
    dist = target_root->translate.vec.f.x - attacker_root->translate.vec.f.x;
    if (dist < 0.0F)
    {
        dist = -dist;
    }
    gNdsStageMPLiveHitDamageLoopCatchStatDistMilli =
        ndsFloatToMilliSigned(dist);

    ftMainUpdateCatchStatFighter(fp, attack_coll, target_fp,
                                 fp->fighter_gobj, target_gobj);
    if ((attack_coll->attack_records[0].victim_gobj == target_gobj) &&
        (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
            FALSE))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_STAT_RECORD;
        gNdsStageMPLiveHitDamageLoopCatchStatRecordHurt = 1u;
    }
    if (gFTMainIsAttackDetect[attack_id] == FALSE)
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_STAT_DETECT;
    }

    if (gNdsStageMPLiveHitDamageLoopCatchStatDistMilli == 160000)
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_STAT_DIST;
    }
    gNdsStageMPLiveHitDamageLoopCatchStatAfterMilli =
        ndsFloatToMilliSigned(fp->search_gobj_dist);
    if ((fp->search_gobj == target_gobj) &&
        (gNdsStageMPLiveHitDamageLoopCatchStatAfterMilli ==
            gNdsStageMPLiveHitDamageLoopCatchStatDistMilli))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_STAT_SEARCH;
        gNdsStageMPLiveHitDamageLoopCatchStatSearchSet = 1u;
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    attacker_root->translate.vec.f.x = saved_attacker_x;
    target_root->translate.vec.f.x = saved_target_x;
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        gFTMainIsAttackDetect[i] = saved_attack_detect[i];
    }
    mask |= NDS_STAGE_MPLIVEHIT_CATCH_STAT_RESTORE;

    gNdsStageMPLiveHitDamageLoopCatchStatMask = mask;
    return ((mask & 0x1fu) == 0x1fu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeSearchHitAllGhostGate(FTStruct *fp)
{
    static FTStruct saved_fighter;
    FTHitLog saved_hitlog;
    GObj *fighter_gobj;
    GObj *saved_fighter_link_head;
    GObj *saved_link_next;
    u32 saved_hitlog_id;
    u32 saved_deferred_count;
    sb32 pass;

    if ((fp == NULL) || (fp->fighter_gobj == NULL) ||
        (fp->is_ghost != FALSE))
    {
        return FALSE;
    }

    fighter_gobj = fp->fighter_gobj;
    saved_fighter = *fp;
    saved_hitlog = sNdsFighterDashRunHitLogs[0];
    saved_hitlog_id = sNdsFighterDashRunHitLogID;
    saved_deferred_count =
        gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount;
    saved_fighter_link_head = gGCCommonLinks[nGCCommonLinkIDFighter];
    saved_link_next = fighter_gobj->link_next;

    fp->is_ghost = TRUE;
    sNdsFighterDashRunHitLogID = 1u;
    gGCCommonLinks[nGCCommonLinkIDFighter] = fighter_gobj;
    fighter_gobj->link_next = NULL;

    ftMainProcSearchHitAll(fighter_gobj);
    pass = ((sNdsFighterDashRunHitLogID == 1u) &&
            (gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount ==
             saved_deferred_count)) ? TRUE : FALSE;

    *fp = saved_fighter;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount =
        saved_deferred_count;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    fighter_gobj->link_next = saved_link_next;

    return pass;
}

static sb32 ndsFighterDashRunProbeCatchSearch(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp)
{
    static FTStruct saved_attacker;
    static FTStruct saved_target;
    FTAttackColl *attack_coll;
    FTDamageColl *damage_coll;
    FTDamageColl *natural_damage_coll;
    GObj *attacker_gobj;
    GObj *target_gobj;
    GObj *saved_fighter_link_head;
    GObj *saved_attacker_link_next;
    GObj *saved_target_link_next;
    DObj *attacker_root;
    DObj *target_root;
    FTParts *parts;
    FTParts *natural_parts;
    GMHitFlags catch_mask;
    NDSFTMainGroundObstacle
        saved_ground_obstacles[NDS_FTMAIN_GROUND_OBSTACLE_COUNT];
    sb32 saved_attack_detect[FTATTACKCOLL_NUM_MAX];
    f32 saved_attacker_x;
    f32 saved_target_x;
    f32 saved_attacker_y;
    f32 saved_target_y;
    f32 saved_attacker_z;
    f32 saved_target_z;
    f32 saved_attacker_anim_frame;
    f32 saved_attacker_root_anim_speed;
    f32 saved_attacker_root_rotate_y;
    u32 mask = 0u;
    u32 skip_mask = 0u;
    u32 slot;
    u32 selected_slot;
    u32 i;
    s32 dist_before_repeat;
    s32 natural_dist_milli;
    s32 self_dist_milli;
    u32 immune_record_count;
    u32 team_record_count;
    u32 ghost_record_count;
    u32 boss_record_count;
    u32 target_status_record_count;
    u32 target_status_rejects;
    u32 hitstatus_probe;
    u32 ga_probe;
    u32 ga_rejects;
    u32 record_probe;
    GMAttackRecord *record;
    ub8 saved_is_team_battle;
    ub8 saved_is_team_attack;
    u32 saved_ground_obstacles_num;

    if ((fp == NULL) || (target_fp == NULL) ||
        (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (fp->fighter_gobj == NULL) || (target_fp->fighter_gobj == NULL))
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    if (attack_coll->attack_state == nGMAttackStateOff)
    {
        return FALSE;
    }

    attacker_gobj = fp->fighter_gobj;
    target_gobj = target_fp->fighter_gobj;
    attacker_root = DObjGetStruct(attacker_gobj);
    target_root = DObjGetStruct(target_gobj);
    if ((attacker_root == NULL) || (target_root == NULL))
    {
        return FALSE;
    }

    selected_slot = gNdsStageMPLiveHitDamageLoopHurtboxFirstHitSlot;
    if (selected_slot >= FTDAMAGECOLL_NUM_MAX)
    {
        selected_slot = 3u;
    }
    damage_coll = &target_fp->damage_colls[selected_slot];
    if ((damage_coll->joint == NULL) || (damage_coll->hitstatus == nGMHitStatusNone))
    {
        return FALSE;
    }

    saved_attacker = *fp;
    saved_target = *target_fp;
    saved_fighter_link_head = gGCCommonLinks[nGCCommonLinkIDFighter];
    saved_attacker_link_next = attacker_gobj->link_next;
    saved_target_link_next = target_gobj->link_next;
    saved_attacker_x = attacker_root->translate.vec.f.x;
    saved_target_x = target_root->translate.vec.f.x;
    saved_attacker_y = attacker_root->translate.vec.f.y;
    saved_target_y = target_root->translate.vec.f.y;
    saved_attacker_z = attacker_root->translate.vec.f.z;
    saved_target_z = target_root->translate.vec.f.z;
    saved_attacker_anim_frame = attacker_gobj->anim_frame;
    saved_attacker_root_anim_speed = attacker_root->anim_speed;
    saved_attacker_root_rotate_y = attacker_root->rotate.vec.f.y;
    saved_is_team_battle = (gSCManagerBattleState != NULL) ?
        gSCManagerBattleState->is_team_battle : FALSE;
    saved_is_team_attack = (gSCManagerBattleState != NULL) ?
        gSCManagerBattleState->is_team_attack : FALSE;
    saved_ground_obstacles_num = sNdsFTMainGroundObstaclesNum;
    for (i = 0u; i < NDS_FTMAIN_GROUND_OBSTACLE_COUNT; i++)
    {
        saved_ground_obstacles[i] = sNdsFTMainGroundObstacles[i];
    }
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        saved_attack_detect[i] = gFTMainIsAttackDetect[i];
        if (i != attack_id)
        {
            fp->attack_colls[i].attack_state = nGMAttackStateOff;
        }
    }

    fp->search_gobj = target_gobj;
    fp->search_gobj_dist = 1.0F;
    fp->search_gobj = NULL;
    fp->search_gobj_dist = F32_MAX;
    if ((fp->search_gobj == NULL) && (fp->search_gobj_dist == F32_MAX))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_RESET;
    }

    fp->catch_mask = 1u;
    target_fp->capture_immune_mask = 0u;
    target_fp->is_ghost = FALSE;
    target_fp->special_hitstatus = nGMHitStatusNormal;
    target_fp->star_hitstatus = nGMHitStatusNormal;
    target_fp->hitstatus = nGMHitStatusNormal;
    if ((attacker_gobj != target_gobj) &&
        (target_fp->is_ghost == FALSE) &&
        (target_fp->fkind != nFTKindBoss) &&
        ((target_fp->capture_immune_mask & fp->catch_mask) == 0u) &&
        (target_fp->special_hitstatus == nGMHitStatusNormal) &&
        (target_fp->star_hitstatus == nGMHitStatusNormal) &&
        (target_fp->hitstatus == nGMHitStatusNormal))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_TARGET;
    }

    target_fp->ga = nMPKineticsGround;
    attack_coll->is_hit_ground = TRUE;
    if ((target_fp->ga == nMPKineticsGround) && (attack_coll->is_hit_ground != FALSE))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_GA;
    }

    ftParamClearAttackRecordID(fp, (s32)attack_id);
    attack_coll->attack_records[0].victim_gobj = target_gobj;
    attack_coll->attack_records[0].victim_flags.is_interact_hurt = TRUE;
    attack_coll->attack_records[0].victim_flags.group_id = 7u;
    catch_mask.is_interact_hurt = catch_mask.is_interact_shield = FALSE;
    catch_mask.group_id = 7u;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        if (attack_coll->attack_records[slot].victim_gobj == target_gobj)
        {
            catch_mask = attack_coll->attack_records[slot].victim_flags;
            break;
        }
    }
    if (catch_mask.is_interact_hurt != FALSE)
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_SKIP;
    }

    ftParamClearAttackRecordID(fp, (s32)attack_id);
    catch_mask.is_interact_hurt = catch_mask.is_interact_shield = FALSE;
    catch_mask.group_id = 7u;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        if (attack_coll->attack_records[slot].victim_gobj == target_gobj)
        {
            catch_mask = attack_coll->attack_records[slot].victim_flags;
            break;
        }
    }
    if ((catch_mask.is_interact_hurt == FALSE) &&
        (catch_mask.is_interact_shield == FALSE) &&
        (catch_mask.group_id == 7u))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_PASS;
    }

    for (i = 0u; i < selected_slot; i++)
    {
        target_fp->damage_colls[i].hitstatus = nGMHitStatusIntangible;
        target_fp->damage_colls[i].is_grabbable = FALSE;
    }
    if (selected_slot > 1u)
    {
        target_fp->damage_colls[1].hitstatus = nGMHitStatusNormal;
    }
    damage_coll->hitstatus = nGMHitStatusNormal;
    damage_coll->is_grabbable = TRUE;
    attacker_root->translate.vec.f.x = 120.0F;
    target_root->translate.vec.f.x = -40.0F;
    parts = ftGetParts(damage_coll->joint);
    if (parts == NULL)
    {
        goto done;
    }
    attack_coll->pos_curr = damage_coll->offset;
    gmCollisionGetWorldPosition(parts->mtx_translate, &attack_coll->pos_curr);
    attack_coll->pos_prev = attack_coll->pos_curr;
    gFTMainIsAttackDetect[attack_id] = TRUE;

    if ((target_fp->damage_colls[0].hitstatus == nGMHitStatusIntangible) ||
        (target_fp->damage_colls[0].hitstatus == nGMHitStatusInvincible))
    {
        skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_STAT;
    }
    if ((selected_slot > 1u) &&
        (target_fp->damage_colls[1].is_grabbable == FALSE))
    {
        skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_GRAB;
    }

    gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
    attacker_gobj->link_next = target_gobj;
    target_gobj->link_next = NULL;
    ftMainSearchFighterCatch(attacker_gobj);

    if (fp->search_gobj == target_gobj)
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_COLLIDE;
        gNdsStageMPLiveHitDamageLoopCatchSearchSlot = selected_slot;
        gNdsStageMPLiveHitDamageLoopCatchSearchJoint =
            (u32)target_fp->damage_colls[selected_slot].joint_id;
        gNdsStageMPLiveHitDamageLoopCatchSearchDistMilli =
            ndsFloatToMilliSigned(fp->search_gobj_dist);
        if (gNdsStageMPLiveHitDamageLoopCatchSearchDistMilli == 160000)
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_UPDATE;
        }
    }

    dist_before_repeat = gNdsStageMPLiveHitDamageLoopCatchSearchDistMilli;
    ftMainSearchFighterCatch(attacker_gobj);
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        if (attack_coll->attack_records[slot].victim_gobj == target_gobj)
        {
            break;
        }
    }
    if ((slot < GMATTACKREC_NUM_MAX) &&
        (attack_coll->attack_records[slot].victim_flags.is_interact_hurt != FALSE) &&
        (fp->search_gobj == NULL) &&
        (fp->search_gobj_dist == F32_MAX) &&
        (gNdsStageMPLiveHitDamageLoopCatchSearchDistMilli == dist_before_repeat))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REPEAT;
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    attack_coll = &fp->attack_colls[attack_id];
    natural_damage_coll = &target_fp->damage_colls[0];
    natural_parts = (natural_damage_coll->joint != NULL) ?
        ftGetParts(natural_damage_coll->joint) : NULL;
    if ((natural_damage_coll->hitstatus == nGMHitStatusNormal) &&
        (natural_damage_coll->is_grabbable != FALSE) &&
        (natural_parts != NULL) &&
        (natural_parts->vec_scale.x != 0.0F) &&
        (natural_parts->vec_scale.y != 0.0F) &&
        (natural_parts->vec_scale.z != 0.0F))
    {
        fp->catch_mask = 1u;
        target_fp->capture_immune_mask = 0u;
        target_fp->is_ghost = FALSE;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        target_fp->ga = nMPKineticsGround;
        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            if (i != attack_id)
            {
                fp->attack_colls[i].attack_state = nGMAttackStateOff;
            }
        }
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        attack_coll->pos_curr = natural_damage_coll->offset;
        gmCollisionGetWorldPosition(natural_parts->mtx_translate,
                                    &attack_coll->pos_curr);
        attack_coll->pos_prev = attack_coll->pos_curr;
        ftParamClearAttackRecordID(fp, (s32)attack_id);

        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = target_gobj;
        target_gobj->link_next = NULL;
        ftMainSearchFighterCatch(attacker_gobj);
        natural_dist_milli = ndsFloatToMilliSigned(fp->search_gobj_dist);
        if ((fp->search_gobj == target_gobj) &&
            (natural_dist_milli == 160000) &&
            (attack_coll->attack_records[0].victim_gobj == target_gobj) &&
            (attack_coll->attack_records[0].victim_flags.is_interact_hurt !=
                FALSE) &&
            (natural_damage_coll->joint_id != 0))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_NATURAL;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    attack_coll = &fp->attack_colls[attack_id];
    damage_coll = &target_fp->damage_colls[selected_slot];
    fp->catch_mask = 1u;
    ndsCatchSearchSeedEligibleTarget(fp, target_fp);
    target_fp->ga = nMPKineticsGround;
    ndsCatchSearchDisableSiblingAttackColls(fp, attack_id);
    attack_coll->attack_state = nGMAttackStateInterpolate;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->is_hit_air = TRUE;
    if ((gSCManagerBattleState != NULL) &&
        (ndsCatchSearchSeedSelectedDamage(target_fp, selected_slot) != FALSE) &&
        (ndsCatchSearchPlaceAttackOnDamage(attack_coll, damage_coll) != FALSE))
    {
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        ndsCatchSearchRunTwoFighterSearch(attacker_gobj, target_gobj);
        self_dist_milli = ndsFloatToMilliSigned(fp->search_gobj_dist);
        if ((fp->search_gobj == target_gobj) &&
            (self_dist_milli == 160000) &&
            (ndsCatchSearchCountVictimRecords(attack_coll, attacker_gobj) ==
                0u) &&
            (ndsCatchSearchCountVictimRecords(attack_coll, target_gobj) == 1u))
        {
            skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_SELF;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    attack_coll = &fp->attack_colls[attack_id];
    fp->catch_mask = 1u;
    target_fp->capture_immune_mask = fp->catch_mask;
    target_fp->is_ghost = FALSE;
    target_fp->special_hitstatus = nGMHitStatusNormal;
    target_fp->star_hitstatus = nGMHitStatusNormal;
    target_fp->hitstatus = nGMHitStatusNormal;
    target_fp->ga = nMPKineticsGround;
    attack_coll->attack_state = nGMAttackStateInterpolate;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->is_hit_air = TRUE;
    ftParamClearAttackRecordID(fp, (s32)attack_id);
    gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
    attacker_gobj->link_next = target_gobj;
    target_gobj->link_next = NULL;
    ftMainSearchFighterCatch(attacker_gobj);
    immune_record_count = 0u;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        if (attack_coll->attack_records[slot].victim_gobj == target_gobj)
        {
            immune_record_count++;
        }
    }
    if ((fp->search_gobj == NULL) &&
        (fp->search_gobj_dist == F32_MAX) &&
        (immune_record_count == 0u))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_IMMUNE;
        skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_IMM;
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    attack_coll = &fp->attack_colls[attack_id];
    fp->catch_mask = 1u;
    target_fp->capture_immune_mask = 0u;
    target_fp->is_ghost = TRUE;
    target_fp->special_hitstatus = nGMHitStatusNormal;
    target_fp->star_hitstatus = nGMHitStatusNormal;
    target_fp->hitstatus = nGMHitStatusNormal;
    target_fp->ga = nMPKineticsGround;
    attack_coll->attack_state = nGMAttackStateInterpolate;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->is_hit_air = TRUE;
    ftParamClearAttackRecordID(fp, (s32)attack_id);
    gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
    attacker_gobj->link_next = target_gobj;
    target_gobj->link_next = NULL;
    ftMainSearchFighterCatch(attacker_gobj);
    ghost_record_count = 0u;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        if (attack_coll->attack_records[slot].victim_gobj == target_gobj)
        {
            ghost_record_count++;
        }
    }
    if ((fp->search_gobj == NULL) &&
        (fp->search_gobj_dist == F32_MAX) &&
        (ghost_record_count == 0u))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_GHOST;
        skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_GHOST;
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    attack_coll = &fp->attack_colls[attack_id];
    fp->catch_mask = 1u;
    target_fp->capture_immune_mask = 0u;
    target_fp->is_ghost = FALSE;
    target_fp->fkind = nFTKindBoss;
    target_fp->special_hitstatus = nGMHitStatusNormal;
    target_fp->star_hitstatus = nGMHitStatusNormal;
    target_fp->hitstatus = nGMHitStatusNormal;
    target_fp->ga = nMPKineticsGround;
    attack_coll->attack_state = nGMAttackStateInterpolate;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->is_hit_air = TRUE;
    ftParamClearAttackRecordID(fp, (s32)attack_id);
    gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
    attacker_gobj->link_next = target_gobj;
    target_gobj->link_next = NULL;
    ftMainSearchFighterCatch(attacker_gobj);
    boss_record_count = 0u;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        if (attack_coll->attack_records[slot].victim_gobj == target_gobj)
        {
            boss_record_count++;
        }
    }
    if ((fp->search_gobj == NULL) &&
        (fp->search_gobj_dist == F32_MAX) &&
        (boss_record_count == 0u))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_BOSS;
        skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_BOSS;
    }

    target_status_rejects = 0u;
    for (hitstatus_probe = 0u; hitstatus_probe < 3u; hitstatus_probe++)
    {
        *fp = saved_attacker;
        *target_fp = saved_target;
        attack_coll = &fp->attack_colls[attack_id];
        damage_coll = &target_fp->damage_colls[selected_slot];
        parts = (damage_coll->joint != NULL) ? ftGetParts(damage_coll->joint) :
            NULL;
        if (parts == NULL)
        {
            continue;
        }

        fp->catch_mask = 1u;
        fp->team = 1u;
        target_fp->team = 2u;
        target_fp->capture_immune_mask = 0u;
        target_fp->is_ghost = FALSE;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        if (hitstatus_probe == 0u)
        {
            target_fp->special_hitstatus = nGMHitStatusIntangible;
        }
        else if (hitstatus_probe == 1u)
        {
            target_fp->star_hitstatus = nGMHitStatusIntangible;
        }
        else
        {
            target_fp->hitstatus = nGMHitStatusIntangible;
        }
        target_fp->ga = nMPKineticsGround;
        damage_coll->hitstatus = nGMHitStatusNormal;
        damage_coll->is_grabbable = TRUE;
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        attack_coll->pos_curr = damage_coll->offset;
        gmCollisionGetWorldPosition(parts->mtx_translate,
                                    &attack_coll->pos_curr);
        attack_coll->pos_prev = attack_coll->pos_curr;
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        if (gSCManagerBattleState != NULL)
        {
            gSCManagerBattleState->is_team_battle = FALSE;
            gSCManagerBattleState->is_team_attack = saved_is_team_attack;
        }
        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = target_gobj;
        target_gobj->link_next = NULL;
        ftMainSearchFighterCatch(attacker_gobj);

        target_status_record_count = 0u;
        for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
        {
            if (attack_coll->attack_records[slot].victim_gobj == target_gobj)
            {
                target_status_record_count++;
            }
        }
        if ((fp->search_gobj == NULL) &&
            (fp->search_gobj_dist == F32_MAX) &&
            (target_status_record_count == 0u))
        {
            target_status_rejects++;
        }
    }
    if (target_status_rejects == 3u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_TGT_STAT;
        skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_TSTAT;
    }

    if (gSCManagerBattleState != NULL)
    {
        *fp = saved_attacker;
        *target_fp = saved_target;
        attack_coll = &fp->attack_colls[attack_id];
        fp->catch_mask = 1u;
        fp->team = 1u;
        target_fp->team = 1u;
        target_fp->capture_immune_mask = 0u;
        target_fp->is_ghost = FALSE;
        target_fp->special_hitstatus = nGMHitStatusNormal;
        target_fp->star_hitstatus = nGMHitStatusNormal;
        target_fp->hitstatus = nGMHitStatusNormal;
        target_fp->ga = nMPKineticsGround;
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        gSCManagerBattleState->is_team_battle = TRUE;
        gSCManagerBattleState->is_team_attack = FALSE;
        gGCCommonLinks[nGCCommonLinkIDFighter] = attacker_gobj;
        attacker_gobj->link_next = target_gobj;
        target_gobj->link_next = NULL;
        ftMainSearchFighterCatch(attacker_gobj);
        team_record_count = 0u;
        for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
        {
            if (attack_coll->attack_records[slot].victim_gobj == target_gobj)
            {
                team_record_count++;
            }
        }
        if ((fp->search_gobj == NULL) &&
            (fp->search_gobj_dist == F32_MAX) &&
            (team_record_count == 0u))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_TEAM;
            skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_TEAM;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    attack_coll = &fp->attack_colls[attack_id];
    damage_coll = &target_fp->damage_colls[selected_slot];
    ndsCatchSearchSeedEligibleTarget(fp, target_fp);
    target_fp->ga = nMPKineticsGround;
    ndsCatchSearchDisableSiblingAttackColls(fp, attack_id);
    attack_coll->attack_state = nGMAttackStateOff;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->is_hit_air = TRUE;
    if ((ndsCatchSearchSeedSelectedDamage(target_fp, selected_slot) != FALSE) &&
        (ndsCatchSearchPlaceAttackOnDamage(attack_coll, damage_coll) != FALSE))
    {
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        ndsCatchSearchRunTwoFighterSearch(attacker_gobj, target_gobj);
        if ((fp->search_gobj == NULL) &&
            (fp->search_gobj_dist == F32_MAX) &&
            (ndsCatchSearchCountVictimRecords(attack_coll, target_gobj) == 0u))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_ATK_OFF;
            skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_ATK;
        }
    }

    ga_rejects = 0u;
    for (ga_probe = 0u; ga_probe < 2u; ga_probe++)
    {
        *fp = saved_attacker;
        *target_fp = saved_target;
        attack_coll = &fp->attack_colls[attack_id];
        damage_coll = &target_fp->damage_colls[selected_slot];
        ndsCatchSearchSeedEligibleTarget(fp, target_fp);
        ndsCatchSearchDisableSiblingAttackColls(fp, attack_id);
        attack_coll->attack_state = nGMAttackStateInterpolate;
        if (ga_probe == 0u)
        {
            target_fp->ga = nMPKineticsGround;
            attack_coll->is_hit_ground = FALSE;
            attack_coll->is_hit_air = TRUE;
        }
        else
        {
            target_fp->ga = nMPKineticsAir;
            attack_coll->is_hit_ground = TRUE;
            attack_coll->is_hit_air = FALSE;
        }
        if ((ndsCatchSearchSeedSelectedDamage(target_fp, selected_slot) == FALSE) ||
            (ndsCatchSearchPlaceAttackOnDamage(attack_coll, damage_coll) == FALSE))
        {
            continue;
        }
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        ndsCatchSearchRunTwoFighterSearch(attacker_gobj, target_gobj);
        if ((fp->search_gobj == NULL) &&
            (fp->search_gobj_dist == F32_MAX) &&
            (ndsCatchSearchCountVictimRecords(attack_coll, target_gobj) == 0u))
        {
            ga_rejects++;
        }
    }
    if (ga_rejects == 2u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_GA_SKIP;
        skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_GA;
    }

    for (record_probe = 0u; record_probe < 3u; record_probe++)
    {
        *fp = saved_attacker;
        *target_fp = saved_target;
        attack_coll = &fp->attack_colls[attack_id];
        damage_coll = &target_fp->damage_colls[selected_slot];
        ndsCatchSearchSeedEligibleTarget(fp, target_fp);
        target_fp->ga = nMPKineticsGround;
        ndsCatchSearchDisableSiblingAttackColls(fp, attack_id);
        attack_coll->attack_state = nGMAttackStateInterpolate;
        attack_coll->is_hit_ground = TRUE;
        attack_coll->is_hit_air = TRUE;
        if ((ndsCatchSearchSeedSelectedDamage(target_fp, selected_slot) == FALSE) ||
            (ndsCatchSearchPlaceAttackOnDamage(attack_coll, damage_coll) == FALSE))
        {
            continue;
        }
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        record = &attack_coll->attack_records[0];
        record->victim_gobj = target_gobj;
        record->victim_flags.is_interact_hurt = FALSE;
        record->victim_flags.is_interact_shield = FALSE;
        record->victim_flags.is_interact_reflect = FALSE;
        record->victim_flags.is_interact_absorb = FALSE;
        record->victim_flags.timer_rehit = 0u;
        record->victim_flags.group_id = 7u;
        if (record_probe == 0u)
        {
            record->victim_flags.is_interact_hurt = TRUE;
        }
        else if (record_probe == 1u)
        {
            record->victim_flags.is_interact_shield = TRUE;
        }
        else
        {
            record->victim_flags.group_id = 6u;
        }

        ndsCatchSearchRunTwoFighterSearch(attacker_gobj, target_gobj);
        if (!((fp->search_gobj == NULL) &&
              (fp->search_gobj_dist == F32_MAX) &&
              (ndsCatchSearchCountVictimRecords(attack_coll, target_gobj) == 1u) &&
              (record->victim_gobj == target_gobj) &&
              (record->victim_flags.timer_rehit == 0u)))
        {
            continue;
        }
        if ((record_probe == 0u) &&
            (record->victim_flags.is_interact_hurt != FALSE) &&
            (record->victim_flags.is_interact_shield == FALSE) &&
            (record->victim_flags.group_id == 7u))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_HURT;
        }
        else if ((record_probe == 1u) &&
                 (record->victim_flags.is_interact_hurt == FALSE) &&
                 (record->victim_flags.is_interact_shield != FALSE) &&
                 (record->victim_flags.group_id == 7u))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_SHLD;
        }
        else if ((record_probe == 2u) &&
                 (record->victim_flags.is_interact_hurt == FALSE) &&
                 (record->victim_flags.is_interact_shield == FALSE) &&
                 (record->victim_flags.group_id == 6u))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_GROUP;
        }
    }
    if ((mask & (NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_HURT |
                 NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_SHLD |
                 NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_GROUP)) ==
        (NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_HURT |
         NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_SHLD |
         NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_REC_GROUP))
    {
        skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_REC;
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    attack_coll = &fp->attack_colls[attack_id];
    damage_coll = &target_fp->damage_colls[selected_slot];
    ndsCatchSearchSeedEligibleTarget(fp, target_fp);
    target_fp->ga = nMPKineticsGround;
    ndsCatchSearchDisableSiblingAttackColls(fp, attack_id);
    attack_coll->attack_state = nGMAttackStateInterpolate;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->is_hit_air = TRUE;
    if ((selected_slot > 0u) &&
        (ndsCatchSearchSeedSelectedDamage(target_fp, selected_slot) != FALSE) &&
        (ndsCatchSearchPlaceAttackOnDamage(attack_coll, damage_coll) != FALSE))
    {
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        target_fp->damage_colls[0].hitstatus = nGMHitStatusNone;
        ndsCatchSearchRunTwoFighterSearch(attacker_gobj, target_gobj);
        if ((fp->search_gobj == NULL) &&
            (fp->search_gobj_dist == F32_MAX) &&
            (ndsCatchSearchCountVictimRecords(attack_coll, target_gobj) == 0u))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_NONE_BRK;
            skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_NONE;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    attack_coll = &fp->attack_colls[attack_id];
    damage_coll = &target_fp->damage_colls[selected_slot];
    ndsCatchSearchSeedEligibleTarget(fp, target_fp);
    target_fp->ga = nMPKineticsGround;
    ndsCatchSearchDisableSiblingAttackColls(fp, attack_id);
    attack_coll->attack_state = nGMAttackStateInterpolate;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->is_hit_air = TRUE;
    if ((ndsCatchSearchSeedSelectedDamage(target_fp, selected_slot) != FALSE) &&
        (ndsCatchSearchPlaceAttackOnDamage(attack_coll, damage_coll) != FALSE))
    {
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        attack_coll->pos_curr.x += 1000.0F;
        attack_coll->pos_prev = attack_coll->pos_curr;
        ndsCatchSearchRunTwoFighterSearch(attacker_gobj, target_gobj);
        if ((fp->search_gobj == NULL) &&
            (fp->search_gobj_dist == F32_MAX) &&
            (ndsCatchSearchCountVictimRecords(attack_coll, target_gobj) == 0u))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_NO_HIT;
            skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_MISS;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    attack_coll = &fp->attack_colls[attack_id];
    damage_coll = &target_fp->damage_colls[selected_slot];
    ndsCatchSearchSeedEligibleTarget(fp, target_fp);
    target_fp->ga = nMPKineticsGround;
    ndsCatchSearchDisableSiblingAttackColls(fp, attack_id);
    attack_coll->attack_state = nGMAttackStateInterpolate;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->is_hit_air = TRUE;
    if ((ndsCatchSearchSeedSelectedDamage(target_fp, selected_slot) != FALSE) &&
        (ndsCatchSearchPlaceAttackOnDamage(attack_coll, damage_coll) != FALSE))
    {
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        damage_coll->hitstatus = nGMHitStatusInvincible;
        ndsCatchSearchRunTwoFighterSearch(attacker_gobj, target_gobj);
        if ((fp->search_gobj == NULL) &&
            (fp->search_gobj_dist == F32_MAX) &&
            (ndsCatchSearchCountVictimRecords(attack_coll, target_gobj) == 0u))
        {
            skip_mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_INV;
        }
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    attack_coll = &fp->attack_colls[attack_id];
    damage_coll = &target_fp->damage_colls[selected_slot];
    ndsCatchSearchSeedEligibleTarget(fp, target_fp);
    target_fp->ga = nMPKineticsGround;
    ndsCatchSearchDisableSiblingAttackColls(fp, attack_id);
    attack_coll->attack_state = nGMAttackStateInterpolate;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->is_hit_air = TRUE;
    if ((ndsCatchSearchSeedSelectedDamage(target_fp, selected_slot) != FALSE) &&
        (ndsCatchSearchPlaceAttackOnDamage(attack_coll, damage_coll) != FALSE))
    {
        ftParamClearAttackRecordID(fp, (s32)attack_id);
        fp->is_catchstatus = FALSE;
        fp->proc_catch = ndsCatchSearchProcCatchCallback;
        fp->proc_capture = ndsCatchSearchProcCaptureCallback;
        fp->twister_wait = 2;
        fp->tarucann_wait = 3;
        fp->hitlag_tics = 0;
        fp->search_gobj = NULL;
        fp->search_gobj_dist = 123.0F;
        sNdsCatchSearchProcCatchCount = 0u;
        sNdsCatchSearchProcCaptureCount = 0u;
        sNdsCatchSearchProcCatchGObj = NULL;
        sNdsCatchSearchProcCaptureTargetGObj = NULL;
        sNdsCatchSearchProcCaptureFighterGObj = NULL;
        gFTMainIsAttackDetect[attack_id] = TRUE;
        ndsCatchSearchRunTwoFighterProcSearch(attacker_gobj, target_gobj);
        if ((fp->twister_wait == 1) &&
            (fp->tarucann_wait == 2) &&
            (fp->search_gobj == NULL) &&
            (fp->search_gobj_dist == 123.0F) &&
            (sNdsCatchSearchProcCatchCount == 0u) &&
            (sNdsCatchSearchProcCaptureCount == 0u) &&
            (ndsCatchSearchCountVictimRecords(attack_coll, target_gobj) == 0u))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_PROC_GATE;
        }

        ftParamClearAttackRecordID(fp, (s32)attack_id);
        fp->is_catchstatus = TRUE;
        fp->search_gobj = NULL;
        fp->search_gobj_dist = F32_MAX;
        sNdsCatchSearchProcCatchCount = 0u;
        sNdsCatchSearchProcCaptureCount = 0u;
        sNdsCatchSearchProcCatchGObj = NULL;
        sNdsCatchSearchProcCaptureTargetGObj = NULL;
        sNdsCatchSearchProcCaptureFighterGObj = NULL;
        gFTMainIsAttackDetect[attack_id] = TRUE;
        ndsCatchSearchRunTwoFighterProcSearch(attacker_gobj, target_gobj);
        if ((fp->search_gobj == target_gobj) &&
            (ndsCatchSearchCountVictimRecords(attack_coll, target_gobj) == 1u))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_PROC_FIND;
        }
        if ((sNdsCatchSearchProcCatchCount == 1u) &&
            (sNdsCatchSearchProcCaptureCount == 1u) &&
            (sNdsCatchSearchProcCatchGObj == attacker_gobj) &&
            (sNdsCatchSearchProcCaptureTargetGObj == target_gobj) &&
            (sNdsCatchSearchProcCaptureFighterGObj == attacker_gobj))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_PROC_CB;
        }
    }

    sNdsFTMainGroundObstaclesNum = 0u;
    for (i = 0u; i < NDS_FTMAIN_GROUND_OBSTACLE_COUNT; i++)
    {
        sNdsFTMainGroundObstacles[i].gobj = NULL;
        sNdsFTMainGroundObstacles[i].proc_update = NULL;
    }
    if ((ftMainCheckAddGroundObstacle(target_gobj,
                                      ndsCatchSearchHazardProbeCallback) !=
            FALSE) &&
        (ftMainCheckAddGroundObstacle(attacker_gobj,
                                      ndsCatchSearchHazardProbeCallback) !=
            FALSE) &&
        (ftMainCheckAddGroundObstacle(target_gobj,
                                      ndsCatchSearchHazardProbeCallback) ==
            FALSE))
    {
        ftMainClearGroundObstacle(target_gobj);
        if ((ftMainCheckAddGroundObstacle(target_gobj,
                                          ndsCatchSearchHazardProbeCallback) !=
                FALSE) &&
            (sNdsFTMainGroundObstaclesNum == 2u) &&
            (sNdsFTMainGroundObstacles[0].gobj == target_gobj) &&
            (sNdsFTMainGroundObstacles[1].gobj == attacker_gobj))
        {
            mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_OBS_REG;
        }
    }
    sNdsCatchSearchHazardExpectedFighter = attacker_gobj;
    sNdsCatchSearchHazardExpectedFirst = target_gobj;
    sNdsCatchSearchHazardExpectedSecond = attacker_gobj;
    sNdsCatchSearchHazardProbeCalls = 0u;
    sNdsCatchSearchHazardProbeMask = 0u;
    fp->is_ghost = FALSE;
    fp->hitlag_tics = 0;
    fp->twister_wait = 4;
    fp->tarucann_wait = 5;
    ftMainSearchHitHazard(attacker_gobj);
    if ((sNdsCatchSearchHazardProbeCalls == 2u) &&
        ((sNdsCatchSearchHazardProbeMask & 0x7u) == 0x7u) &&
        (fp->twister_wait == 3) &&
        (fp->tarucann_wait == 4))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_OBS_CB;
    }

    sNdsCatchSearchHazardExpectedFighter = attacker_gobj;
    sNdsCatchSearchHazardExpectedFirst = target_gobj;
    sNdsCatchSearchHazardExpectedSecond = attacker_gobj;
    sNdsCatchSearchHazardProbeCalls = 0u;
    sNdsCatchSearchHazardProbeMask = 0u;
    fp->is_ghost = TRUE;
    fp->hitlag_tics = 0;
    fp->twister_wait = 7;
    fp->tarucann_wait = 8;
    ftMainSearchHitHazard(attacker_gobj);
    if ((sNdsCatchSearchHazardProbeCalls == 0u) &&
        (sNdsCatchSearchHazardProbeMask == 0u) &&
        (fp->twister_wait == 7) &&
        (fp->tarucann_wait == 8))
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_OBS_GHOST;
    }

    *fp = saved_attacker;
    attacker_gobj->anim_frame = saved_attacker_anim_frame;
    attacker_root->anim_speed = saved_attacker_root_anim_speed;
    sNdsFTMainGroundObstaclesNum = 0u;
    for (i = 0u; i < NDS_FTMAIN_GROUND_OBSTACLE_COUNT; i++)
    {
        sNdsFTMainGroundObstacles[i].gobj = NULL;
        sNdsFTMainGroundObstacles[i].proc_update = NULL;
    }
    if (ftMainCheckAddGroundObstacle(target_gobj,
                                     ndsCatchSearchHazardTwisterCallback) !=
        FALSE)
    {
        sNdsCatchSearchHazardExpectedFighter = attacker_gobj;
        sNdsCatchSearchHazardExpectedFirst = target_gobj;
        sNdsCatchSearchHazardProbeCalls = 0u;
        sNdsCatchSearchHazardProbeMask = 0u;
        fp->is_ghost = FALSE;
        fp->hitlag_tics = 0;
        fp->twister_wait = 2;
        fp->tarucann_wait = 0;
        fp->item_gobj = NULL;
        fp->catch_gobj = NULL;
        fp->capture_gobj = NULL;
        fp->proc_damage = NULL;
        fp->ga = nMPKineticsGround;
        ftMainSearchHitHazard(attacker_gobj);
        if ((sNdsCatchSearchHazardProbeCalls == 1u) &&
            ((sNdsCatchSearchHazardProbeMask & 1u) != 0u) &&
            (fp->twister_wait == 1) &&
            (fp->status_id == nFTCommonStatusTwister) &&
            (fp->motion_id == nFTCommonMotionTwister) &&
            (fp->proc_update == ndsBaseFTCommonTwisterProcUpdate) &&
            (fp->proc_physics == ndsBaseFTCommonTwisterProcPhysics) &&
            (fp->status_vars.common.twister.release_wait == 0) &&
            (fp->status_vars.common.twister.tornado_gobj == target_gobj) &&
            (fp->capture_immune_mask == FTCATCHKIND_MASK_ALL))
        {
            fp->physics.vel_air.x = 0.0F;
            fp->physics.vel_air.y = 0.0F;
            fp->physics.vel_air.z = 0.0F;
            attacker_root->rotate.vec.f.y = 0.0F;
            fp->proc_update(attacker_gobj);
            fp->proc_physics(attacker_gobj);
            if ((fp->status_vars.common.twister.release_wait == 1) &&
                ((fp->physics.vel_air.x != 0.0F) ||
                 (fp->physics.vel_air.y != 0.0F) ||
                 (fp->physics.vel_air.z != 0.0F)) &&
                (attacker_root->rotate.vec.f.y != 0.0F))
            {
                mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_OBS_TICK;
            }

            *fp = saved_attacker;
            attacker_gobj->anim_frame = saved_attacker_anim_frame;
            attacker_root->anim_speed = saved_attacker_root_anim_speed;
            attacker_root->rotate.vec.f.y = saved_attacker_root_rotate_y;
            sNdsFTMainGroundObstaclesNum = 0u;
            for (i = 0u; i < NDS_FTMAIN_GROUND_OBSTACLE_COUNT; i++)
            {
                sNdsFTMainGroundObstacles[i].gobj = NULL;
                sNdsFTMainGroundObstacles[i].proc_update = NULL;
            }
            if (ftMainCheckAddGroundObstacle(
                    target_gobj, ndsCatchSearchHazardTaruCannCallback) !=
                FALSE)
            {
                sNdsCatchSearchHazardExpectedFighter = attacker_gobj;
                sNdsCatchSearchHazardExpectedFirst = target_gobj;
                sNdsCatchSearchHazardProbeCalls = 0u;
                sNdsCatchSearchHazardProbeMask = 0u;
                fp->is_ghost = FALSE;
                fp->hitlag_tics = 0;
                fp->twister_wait = 0;
                fp->tarucann_wait = 0;
                fp->item_gobj = NULL;
                fp->catch_gobj = NULL;
                fp->capture_gobj = NULL;
                fp->proc_damage = NULL;
                fp->capture_immune_mask = 0u;
                fp->is_invisible = FALSE;
                fp->hitstatus = nGMHitStatusNormal;
                fp->special_hitstatus = nGMHitStatusNormal;
                fp->ga = nMPKineticsGround;
                ftMainSearchHitHazard(attacker_gobj);
                if ((sNdsCatchSearchHazardProbeCalls == 1u) &&
                    ((sNdsCatchSearchHazardProbeMask & 1u) != 0u) &&
                    (fp->status_id == nFTCommonStatusTaruCann) &&
                    (fp->motion_id == nFTCommonMotionNull) &&
                    (fp->motion_script_id == nFTCommonMotionNull) &&
                    (fp->proc_update == NULL) &&
                    (fp->proc_interrupt == NULL) &&
                    (fp->proc_physics == ftCommonTaruCannProcPhysics) &&
                    (fp->ga == nMPKineticsGround) &&
                    (fp->status_vars.common.tarucann.release_wait == 0) &&
                    (fp->status_vars.common.tarucann.shoot_wait == 0) &&
                    (fp->status_vars.common.tarucann.tarucann_gobj ==
                        target_gobj) &&
                    (fp->capture_immune_mask == FTCATCHKIND_MASK_ALL) &&
                    (fp->is_invisible != FALSE) &&
                    (fp->hitstatus == nGMHitStatusIntangible) &&
                    (fp->special_hitstatus == nGMHitStatusIntangible))
                {
                    attacker_root->translate.vec.f.x = -111.0F;
                    attacker_root->translate.vec.f.y = -222.0F;
                    attacker_root->translate.vec.f.z = -333.0F;
                    target_root->translate.vec.f.x = 321.0F;
                    target_root->translate.vec.f.y = 654.0F;
                    target_root->translate.vec.f.z = 987.0F;
                    fp->proc_physics(attacker_gobj);
                    if ((attacker_root->translate.vec.f.x ==
                            target_root->translate.vec.f.x) &&
                        (attacker_root->translate.vec.f.y ==
                            target_root->translate.vec.f.y) &&
                        (attacker_root->translate.vec.f.z ==
                            target_root->translate.vec.f.z))
                    {
                        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_OBS_TWIST;
                    }
                }
            }
        }
    }

    if ((skip_mask & NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_STAT) != 0u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_STATUS;
    }
    if ((skip_mask & NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_SKIP_GRAB) != 0u)
    {
        mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_GRAB;
    }

done:
    *fp = saved_attacker;
    *target_fp = saved_target;
    attacker_root->translate.vec.f.x = saved_attacker_x;
    target_root->translate.vec.f.x = saved_target_x;
    attacker_root->translate.vec.f.y = saved_attacker_y;
    target_root->translate.vec.f.y = saved_target_y;
    attacker_root->translate.vec.f.z = saved_attacker_z;
    target_root->translate.vec.f.z = saved_target_z;
    attacker_gobj->anim_frame = saved_attacker_anim_frame;
    attacker_root->anim_speed = saved_attacker_root_anim_speed;
    attacker_root->rotate.vec.f.y = saved_attacker_root_rotate_y;
    gGCCommonLinks[nGCCommonLinkIDFighter] = saved_fighter_link_head;
    if (gSCManagerBattleState != NULL)
    {
        gSCManagerBattleState->is_team_battle = saved_is_team_battle;
        gSCManagerBattleState->is_team_attack = saved_is_team_attack;
    }
    attacker_gobj->link_next = saved_attacker_link_next;
    target_gobj->link_next = saved_target_link_next;
    sNdsFTMainGroundObstaclesNum = saved_ground_obstacles_num;
    for (i = 0u; i < NDS_FTMAIN_GROUND_OBSTACLE_COUNT; i++)
    {
        sNdsFTMainGroundObstacles[i] = saved_ground_obstacles[i];
    }
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        gFTMainIsAttackDetect[i] = saved_attack_detect[i];
    }
    mask |= NDS_STAGE_MPLIVEHIT_CATCH_SEARCH_RESTORE;

    gNdsStageMPLiveHitDamageLoopCatchSearchMask = mask;
    gNdsStageMPLiveHitDamageLoopCatchSearchSkipMask = skip_mask;
    return ((mask & 0xffffffffu) == 0xffffffffu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunStepAttackDamageCollide(FTStruct *fp,
                                                     u32 attack_id)
{
    FTAttackColl attack_probe;
    FTAttackColl *attack_coll;
    FTStruct *target_fp;
    FTDamageColl *damage_coll;
    FTParts *parts;

    if ((fp == NULL) || (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (fp->player != 1) || (attack_id != 1u))
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    if (attack_coll->attack_state == nGMAttackStateOff)
    {
        return FALSE;
    }

    target_fp = &sNdsFighterStructPool[0];
    if ((ndsFighterStructIsPoolPointer(target_fp) == FALSE) ||
        (target_fp->attr == NULL))
    {
        return FALSE;
    }

    damage_coll = &target_fp->damage_colls[0];
    if ((damage_coll->hitstatus != nGMHitStatusNormal) ||
        (damage_coll->joint == NULL))
    {
        return FALSE;
    }

    parts = ftGetParts(damage_coll->joint);
    if (parts == NULL)
    {
        return FALSE;
    }

    attack_probe = *attack_coll;
    attack_probe.pos_curr = damage_coll->offset;
    gmCollisionGetWorldPosition(parts->mtx_translate,
                                &attack_probe.pos_curr);
    attack_probe.pos_prev = attack_probe.pos_curr;

    if (ndsGMCollisionCheckFighterAttackDamageCollideSelected(
            &attack_probe, damage_coll) == FALSE)
    {
        return FALSE;
    }

    gNdsFighterDashRunAttackEventPositionMask |=
        NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_COLLIDE;
    return TRUE;
}

static sb32 ndsFighterDashRunSetDamageAttackRecord(FTStruct *fp,
                                                   u32 attack_group_id,
                                                   GObj *victim_gobj)
{
    GMAttackRecord *record;
    u32 i;
    u32 slot;
    sb32 is_recorded = FALSE;

    if ((fp == NULL) || (victim_gobj == NULL))
    {
        return FALSE;
    }

    for (i = 0; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        FTAttackColl *attack_coll = &fp->attack_colls[i];

        if ((attack_coll->attack_state == nGMAttackStateOff) ||
            (attack_coll->group_id != attack_group_id))
        {
            continue;
        }

        for (slot = 0; slot < GMATTACKREC_NUM_MAX; slot++)
        {
            record = &attack_coll->attack_records[slot];
            if (record->victim_gobj == victim_gobj)
            {
                record->victim_flags.is_interact_hurt = TRUE;
                is_recorded = TRUE;
                break;
            }
        }
        if (slot != GMATTACKREC_NUM_MAX)
        {
            continue;
        }

        for (slot = 0; slot < GMATTACKREC_NUM_MAX; slot++)
        {
            if (attack_coll->attack_records[slot].victim_gobj == NULL)
            {
                break;
            }
        }
        if (slot == GMATTACKREC_NUM_MAX)
        {
            slot = 0u;
        }

        record = &attack_coll->attack_records[slot];
        record->victim_gobj = victim_gobj;
        record->victim_flags.is_interact_hurt = TRUE;
        is_recorded = TRUE;
    }
    return is_recorded;
}

static sb32 ndsFighterDashRunStepAttackDamageRecord(FTStruct *fp,
                                                    u32 attack_id)
{
    FTAttackColl *attack_coll;
    FTStruct *target_fp;
    GObj *target_gobj;
    GMAttackRecord *record;
    u32 slot;

    if ((fp == NULL) || (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (fp->player != 1) || (attack_id != 1u))
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    if (attack_coll->attack_state == nGMAttackStateOff)
    {
        return FALSE;
    }

    target_fp = &sNdsFighterStructPool[0];
    target_gobj = target_fp->fighter_gobj;
    if ((ndsFighterStructIsPoolPointer(target_fp) == FALSE) ||
        (target_fp->attr == NULL) || (target_gobj == NULL))
    {
        return FALSE;
    }

    if (ndsFighterDashRunSetDamageAttackRecord(
            fp, attack_coll->group_id, target_gobj) == FALSE)
    {
        return FALSE;
    }

    for (slot = 0; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        record = &attack_coll->attack_records[slot];
        if ((record->victim_gobj == target_gobj) &&
            (record->victim_flags.is_interact_hurt != FALSE) &&
            (record->victim_flags.is_interact_shield == FALSE) &&
            (record->victim_flags.group_id == 7u) &&
            (record->victim_flags.timer_rehit == 0u))
        {
            gNdsFighterDashRunAttackEventPositionMask |=
                NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_RECORD;
            return TRUE;
        }
    }

    if (slot == GMATTACKREC_NUM_MAX)
    {
        return FALSE;
    }
    return FALSE;
}

static s32 ndsFighterDashRunGetCapturedDamage(FTStruct *fp, s32 damage)
{
    return ftParamGetCapturedDamage(fp, damage);
}

static sb32 ndsFighterDashRunCheckGetUpdateDamageNormal(FTStruct *fp,
                                                        s32 *damage)
{
    return ftMainCheckGetUpdateDamage(fp, damage);
}

static sb32 ndsFighterDashRunStepAttackDamageHitLog(FTStruct *fp,
                                                    u32 attack_id)
{
    FTAttackColl *attack_coll;
    FTStruct *target_fp;
    GObj *target_gobj;
    FTDamageColl *damage_coll;
    FTHitLog *hitlog;
    s32 damage;
    s32 expected_attack_damage;
    s32 expected_damage_lag;
    s32 old_attack_damage;
    s32 old_damage_lag;
    s32 old_damage_queue;

    if ((fp == NULL) || (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (fp->player != 1) || (attack_id != 1u) ||
        (fp->fighter_gobj == NULL) || (fp->throw_gobj != NULL))
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    if (attack_coll->attack_state == nGMAttackStateOff)
    {
        return FALSE;
    }

    target_fp = &sNdsFighterStructPool[0];
    target_gobj = target_fp->fighter_gobj;
    if ((ndsFighterStructIsPoolPointer(target_fp) == FALSE) ||
        (target_fp->attr == NULL) || (target_gobj == NULL))
    {
        return FALSE;
    }

    damage_coll = &target_fp->damage_colls[0];
    if ((target_fp->special_hitstatus != nGMHitStatusNormal) ||
        (target_fp->star_hitstatus != nGMHitStatusNormal) ||
        (target_fp->hitstatus != nGMHitStatusNormal) ||
        (damage_coll->hitstatus != nGMHitStatusNormal))
    {
        return FALSE;
    }

    if (ndsFighterDashRunSetDamageAttackRecord(
            fp, attack_coll->group_id, target_gobj) == FALSE)
    {
        return FALSE;
    }

    old_attack_damage = fp->attack_damage;
    old_damage_lag = target_fp->damage_lag;
    old_damage_queue = target_fp->damage_queue;

    damage = ndsFighterDashRunGetCapturedDamage(target_fp,
                                                attack_coll->damage);
    if (fp->attack_damage < damage)
    {
        fp->attack_damage = damage;
    }
    if (ndsFighterDashRunCheckGetUpdateDamageNormal(target_fp,
                                                    &damage) == FALSE)
    {
        return FALSE;
    }

    sNdsFighterDashRunHitLogID = 0u;
    hitlog = &sNdsFighterDashRunHitLogs[sNdsFighterDashRunHitLogID++];
    hitlog->attacker_object_class = nFTHitLogObjectFighter;
    hitlog->attack_coll = attack_coll;
    hitlog->attack_id = 0;
    hitlog->attacker_gobj = fp->fighter_gobj;
    hitlog->damage_coll = damage_coll;
    hitlog->attacker_player = fp->player;
    hitlog->attacker_player_num = fp->player_num;

    ftParamUpdatePlayerBattleStats(fp->player, target_fp->player, damage);
    ftParamUpdateStaleQueue(fp->player, target_fp->player,
                            (s32)attack_coll->motion_attack_id,
                            attack_coll->motion_count);

    expected_attack_damage = old_attack_damage;
    if (expected_attack_damage < damage)
    {
        expected_attack_damage = damage;
    }
    expected_damage_lag = old_damage_lag;
    if (expected_damage_lag < damage)
    {
        expected_damage_lag = damage;
    }

    if ((sNdsFighterDashRunHitLogID == 1u) &&
        (hitlog->attacker_object_class == nFTHitLogObjectFighter) &&
        (hitlog->attack_coll == attack_coll) &&
        (hitlog->attacker_gobj == fp->fighter_gobj) &&
        (hitlog->damage_coll == damage_coll) &&
        (hitlog->attacker_player == fp->player) &&
        (hitlog->attacker_player_num == fp->player_num) &&
        (damage == attack_coll->damage) &&
        (fp->attack_damage == expected_attack_damage) &&
        (target_fp->damage_queue == (old_damage_queue + damage)) &&
        (target_fp->damage_lag == expected_damage_lag))
    {
        gNdsFighterDashRunAttackEventPositionMask |=
            NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_HITLOG;
        return TRUE;
    }
    return FALSE;
}

static sb32 ndsFighterDashRunStepAttackDamageHitSFX(FTStruct *fp,
                                                    u32 attack_id)
{
    FTAttackColl *attack_coll;
    DObj *top_joint;
    u32 fgm_id;

    if ((fp == NULL) || (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (fp->player != 1) || (attack_id != 1u))
    {
        return FALSE;
    }
    top_joint = fp->joints[nFTPartsJointTopN];
    if (top_joint == NULL)
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    if ((attack_coll->attack_state == nGMAttackStateOff) ||
        (attack_coll->fgm_kind >= ARRAY_COUNT(sNdsFighterDashRunHitCollisionFGMs)) ||
        (attack_coll->fgm_level >=
         ARRAY_COUNT(sNdsFighterDashRunHitCollisionFGMs[0])))
    {
        return FALSE;
    }

    fgm_id = sNdsFighterDashRunHitCollisionFGMs[attack_coll->fgm_kind]
                                                 [attack_coll->fgm_level];
    /* ponytail: positional balance waits for a real DS audio backend. */
    (void)top_joint;
    (void)func_800269C0_275C0((u16)fgm_id);

    if (gNdsSCVSBattleLastFGM == fgm_id)
    {
        gNdsFighterDashRunAttackEventPositionMask |=
            NDS_FTMOTION_ATTACK_EVENT_POS_HIT_SFX;
        return TRUE;
    }
    return FALSE;
}

static void ndsFighterDashRunGetFighterAttackDamagePosition(
    Vec3f *dst, FTAttackColl *attack_coll, FTDamageColl *damage_coll)
{
    FTParts *parts;
    Vec3f attack_pos;
    Vec3f damage_pos;

    if ((dst == NULL) || (attack_coll == NULL) || (damage_coll == NULL))
    {
        return;
    }

    attack_pos = attack_coll->pos_curr;
    parts = ftGetParts(damage_coll->joint);
    damage_pos = damage_coll->offset;
    if (parts != NULL)
    {
        gmCollisionGetWorldPosition(parts->mtx_translate, &damage_pos);
    }

    dst->x = (attack_pos.x + damage_pos.x) * 0.5F;
    dst->y = (attack_pos.y + damage_pos.y) * 0.5F;
    dst->z = (attack_pos.z + damage_pos.z) * 0.5F;
}

static sb32 ndsFighterDashRunProbeDamageEffectOnly(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp)
{
    static FTStruct saved_attacker;
    static FTStruct saved_target;
    FTHitLog saved_hitlog;
    FTAttackColl *attack_coll;
    FTDamageColl *damage_coll;
    GObj *target_gobj;
    u32 saved_hitlog_id;
    u32 slot = gNdsStageMPLiveHitDamageLoopHurtboxFirstHitSlot;
    u32 mask = 0u;
    s32 damage;
    Vec3f impact_pos;

    if ((fp == NULL) || (target_fp == NULL) ||
        (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (slot >= FTDAMAGECOLL_NUM_MAX))
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    damage_coll = &target_fp->damage_colls[slot];
    target_gobj = target_fp->fighter_gobj;
    if ((attack_coll->attack_state == nGMAttackStateOff) ||
        (target_gobj == NULL) || (damage_coll->joint == NULL) ||
        (target_fp->attr == NULL))
    {
        return FALSE;
    }

    saved_attacker = *fp;
    saved_target = *target_fp;
    saved_hitlog = sNdsFighterDashRunHitLogs[0];
    saved_hitlog_id = sNdsFighterDashRunHitLogID;

    fp->attack_damage = 0;
    target_fp->damage_queue = 0;
    target_fp->damage_lag = 0;
    target_fp->special_hitstatus = nGMHitStatusNormal;
    target_fp->star_hitstatus = nGMHitStatusNormal;
    target_fp->hitstatus = nGMHitStatusNormal;
    damage_coll->hitstatus = nGMHitStatusInvincible;

    gNdsStageMPLiveHitDamageLoopEffectOnlyQueueBefore =
        target_fp->damage_queue;
    gNdsStageMPLiveHitDamageLoopEffectOnlyPercentBefore =
        target_fp->percent_damage;
    gNdsStageMPLiveHitDamageLoopEffectOnlyHitLogBefore =
        sNdsFighterDashRunHitLogID;

    if (ndsFighterDashRunSetDamageAttackRecord(
            fp, attack_coll->group_id, target_gobj) != FALSE)
    {
        mask |= NDS_STAGE_MPLIVEHIT_EFFECTONLY_RECORD;
    }

    damage = ndsFighterDashRunGetCapturedDamage(target_fp,
                                                attack_coll->damage);
    if (fp->attack_damage < damage)
    {
        fp->attack_damage = damage;
    }
    gNdsStageMPLiveHitDamageLoopEffectOnlyStatus =
        (u32)damage_coll->hitstatus;
    gNdsStageMPLiveHitDamageLoopEffectOnlyDamage = damage;
    gNdsStageMPLiveHitDamageLoopEffectOnlyAttackDamageAfter =
        fp->attack_damage;

    if (damage_coll->hitstatus != nGMHitStatusNormal)
    {
        mask |= NDS_STAGE_MPLIVEHIT_EFFECTONLY_STATUS;
    }
    if ((damage > 0) && (fp->attack_damage == damage))
    {
        mask |= NDS_STAGE_MPLIVEHIT_EFFECTONLY_ATTACK_DMG;
    }

    ndsFighterDashRunGetFighterAttackDamagePosition(&impact_pos,
                                                    attack_coll,
                                                    damage_coll);
    (void)impact_pos;
    if (damage > 0)
    {
        gNdsStageMPLiveHitDamageLoopEffectOnlyEffectCount = 1u;
        mask |= NDS_STAGE_MPLIVEHIT_EFFECTONLY_EFFECT;
    }
    if (ndsFighterDashRunStepAttackDamageHitSFX(fp, attack_id) != FALSE)
    {
        gNdsStageMPLiveHitDamageLoopEffectOnlySFXCount = 1u;
        mask |= NDS_STAGE_MPLIVEHIT_EFFECTONLY_SFX;
    }

    gNdsStageMPLiveHitDamageLoopEffectOnlyQueueAfter =
        target_fp->damage_queue;
    gNdsStageMPLiveHitDamageLoopEffectOnlyPercentAfter =
        target_fp->percent_damage;
    gNdsStageMPLiveHitDamageLoopEffectOnlyHitLogAfter =
        sNdsFighterDashRunHitLogID;

    if (gNdsStageMPLiveHitDamageLoopEffectOnlyQueueAfter ==
        gNdsStageMPLiveHitDamageLoopEffectOnlyQueueBefore)
    {
        mask |= NDS_STAGE_MPLIVEHIT_EFFECTONLY_NO_QUEUE;
    }
    if (gNdsStageMPLiveHitDamageLoopEffectOnlyPercentAfter ==
        gNdsStageMPLiveHitDamageLoopEffectOnlyPercentBefore)
    {
        mask |= NDS_STAGE_MPLIVEHIT_EFFECTONLY_NO_PERCENT;
    }
    if (gNdsStageMPLiveHitDamageLoopEffectOnlyHitLogAfter ==
        gNdsStageMPLiveHitDamageLoopEffectOnlyHitLogBefore)
    {
        mask |= NDS_STAGE_MPLIVEHIT_EFFECTONLY_NO_HITLOG;
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    mask |= NDS_STAGE_MPLIVEHIT_EFFECTONLY_RESTORE;

    gNdsStageMPLiveHitDamageLoopEffectOnlyMask = mask;
    return ((mask & 0x1ffu) == 0x1ffu) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunProbeDamageResist(
    FTStruct *fp, u32 attack_id, FTStruct *target_fp)
{
    static FTStruct saved_attacker;
    static FTStruct saved_target;
    FTHitLog saved_hitlog;
    FTAttackColl *attack_coll;
    FTDamageColl *damage_coll;
    GObj *target_gobj;
    u32 saved_hitlog_id;
    u32 slot = gNdsStageMPLiveHitDamageLoopHurtboxFirstHitSlot;
    u32 mask = 0u;
    u32 break_mask = 0u;
    s32 damage;
    s32 break_damage;
    s32 break_expected;
    Vec3f impact_pos;

    if ((fp == NULL) || (target_fp == NULL) ||
        (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (slot >= FTDAMAGECOLL_NUM_MAX))
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    damage_coll = &target_fp->damage_colls[slot];
    target_gobj = target_fp->fighter_gobj;
    if ((attack_coll->attack_state == nGMAttackStateOff) ||
        (target_gobj == NULL) || (damage_coll->joint == NULL) ||
        (target_fp->attr == NULL))
    {
        return FALSE;
    }

    saved_attacker = *fp;
    saved_target = *target_fp;
    saved_hitlog = sNdsFighterDashRunHitLogs[0];
    saved_hitlog_id = sNdsFighterDashRunHitLogID;

    fp->attack_damage = 0;
    target_fp->damage_queue = 0;
    target_fp->damage_lag = 0;
    target_fp->special_hitstatus = nGMHitStatusNormal;
    target_fp->star_hitstatus = nGMHitStatusNormal;
    target_fp->hitstatus = nGMHitStatusNormal;
    damage_coll->hitstatus = nGMHitStatusNormal;

    gNdsStageMPLiveHitDamageLoopDamageResistQueueBefore =
        target_fp->damage_queue;
    gNdsStageMPLiveHitDamageLoopDamageResistPercentBefore =
        target_fp->percent_damage;
    gNdsStageMPLiveHitDamageLoopDamageResistHitLogBefore =
        sNdsFighterDashRunHitLogID;

    if (ndsFighterDashRunSetDamageAttackRecord(
            fp, attack_coll->group_id, target_gobj) != FALSE)
    {
        mask |= NDS_STAGE_MPLIVEHIT_RESIST_RECORD;
    }
    if ((target_fp->special_hitstatus == nGMHitStatusNormal) &&
        (target_fp->star_hitstatus == nGMHitStatusNormal) &&
        (target_fp->hitstatus == nGMHitStatusNormal) &&
        (damage_coll->hitstatus == nGMHitStatusNormal))
    {
        mask |= NDS_STAGE_MPLIVEHIT_RESIST_STATUS;
    }

    damage = ndsFighterDashRunGetCapturedDamage(target_fp,
                                                attack_coll->damage);
    target_fp->is_damage_resist = TRUE;
    target_fp->damage_resist = damage + 3;
    gNdsStageMPLiveHitDamageLoopDamageResistDamage = damage;
    gNdsStageMPLiveHitDamageLoopDamageResistBefore =
        target_fp->damage_resist;
    if (target_fp->is_damage_resist && (target_fp->damage_resist > damage))
    {
        mask |= NDS_STAGE_MPLIVEHIT_RESIST_SEED;
    }
    if (fp->attack_damage < damage)
    {
        fp->attack_damage = damage;
    }
    gNdsStageMPLiveHitDamageLoopDamageResistAttackDamageAfter =
        fp->attack_damage;
    if (fp->attack_damage == damage)
    {
        mask |= NDS_STAGE_MPLIVEHIT_RESIST_ATTACK_DMG;
    }

    if (ndsFighterDashRunCheckGetUpdateDamageNormal(target_fp,
                                                    &damage) == FALSE)
    {
        mask |= NDS_STAGE_MPLIVEHIT_RESIST_CHECK_FALSE;
    }
    gNdsStageMPLiveHitDamageLoopDamageResistAfter =
        target_fp->damage_resist;
    gNdsStageMPLiveHitDamageLoopDamageResistFlagAfter =
        (target_fp->is_damage_resist != FALSE) ? 1u : 0u;
    if ((target_fp->is_damage_resist != FALSE) &&
        (target_fp->damage_resist ==
            (gNdsStageMPLiveHitDamageLoopDamageResistBefore -
             gNdsStageMPLiveHitDamageLoopDamageResistDamage)))
    {
        mask |= NDS_STAGE_MPLIVEHIT_RESIST_AFTER;
    }

    ndsFighterDashRunGetFighterAttackDamagePosition(&impact_pos,
                                                    attack_coll,
                                                    damage_coll);
    (void)impact_pos;
    if (damage > 0)
    {
        gNdsStageMPLiveHitDamageLoopDamageResistEffectCount = 1u;
        mask |= NDS_STAGE_MPLIVEHIT_RESIST_EFFECT;
    }
    if (ndsFighterDashRunStepAttackDamageHitSFX(fp, attack_id) != FALSE)
    {
        gNdsStageMPLiveHitDamageLoopDamageResistSFXCount = 1u;
        mask |= NDS_STAGE_MPLIVEHIT_RESIST_SFX;
    }

    gNdsStageMPLiveHitDamageLoopDamageResistQueueAfter =
        target_fp->damage_queue;
    gNdsStageMPLiveHitDamageLoopDamageResistPercentAfter =
        target_fp->percent_damage;
    gNdsStageMPLiveHitDamageLoopDamageResistHitLogAfter =
        sNdsFighterDashRunHitLogID;

    if (gNdsStageMPLiveHitDamageLoopDamageResistQueueAfter ==
        gNdsStageMPLiveHitDamageLoopDamageResistQueueBefore)
    {
        mask |= NDS_STAGE_MPLIVEHIT_RESIST_NO_QUEUE;
    }
    if (gNdsStageMPLiveHitDamageLoopDamageResistPercentAfter ==
        gNdsStageMPLiveHitDamageLoopDamageResistPercentBefore)
    {
        mask |= NDS_STAGE_MPLIVEHIT_RESIST_NO_PERCENT;
    }
    if (gNdsStageMPLiveHitDamageLoopDamageResistHitLogAfter ==
        gNdsStageMPLiveHitDamageLoopDamageResistHitLogBefore)
    {
        mask |= NDS_STAGE_MPLIVEHIT_RESIST_NO_HITLOG;
    }

    fp->attack_damage = 0;
    target_fp->damage_queue = 0;
    target_fp->damage_lag = 0;
    target_fp->is_damage_resist = TRUE;
    break_damage = ndsFighterDashRunGetCapturedDamage(target_fp,
                                                      attack_coll->damage);
    break_expected = break_damage / 2;
    if (break_expected <= 0)
    {
        break_expected = 1;
    }
    target_fp->damage_resist = break_expected;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakBefore =
        target_fp->damage_resist;
    if ((target_fp->is_damage_resist != FALSE) &&
        (target_fp->damage_resist > 0) &&
        (target_fp->damage_resist < break_damage))
    {
        break_mask |= NDS_STAGE_MPLIVEHIT_RESIST_BREAK_SEED;
    }
    if (ndsFighterDashRunCheckGetUpdateDamageNormal(target_fp,
                                                    &break_damage) != FALSE)
    {
        break_mask |= NDS_STAGE_MPLIVEHIT_RESIST_BREAK_TRUE;
    }
    gNdsStageMPLiveHitDamageLoopDamageResistBreakAfter =
        target_fp->damage_resist;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakFlagAfter =
        (target_fp->is_damage_resist != FALSE) ? 1u : 0u;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakDamageAfter =
        break_damage;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakQueueAfter =
        target_fp->damage_queue;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakLagAfter =
        target_fp->damage_lag;
    if ((target_fp->is_damage_resist == FALSE) &&
        (target_fp->damage_resist < 0))
    {
        break_mask |= NDS_STAGE_MPLIVEHIT_RESIST_BREAK_CLEAR;
    }
    if (break_damage == -target_fp->damage_resist)
    {
        break_mask |= NDS_STAGE_MPLIVEHIT_RESIST_BREAK_LEFTOVER;
    }
    if (target_fp->damage_queue == break_damage)
    {
        break_mask |= NDS_STAGE_MPLIVEHIT_RESIST_BREAK_QUEUE;
    }
    if (target_fp->damage_lag == break_damage)
    {
        break_mask |= NDS_STAGE_MPLIVEHIT_RESIST_BREAK_LAG;
    }

    *fp = saved_attacker;
    *target_fp = saved_target;
    sNdsFighterDashRunHitLogs[0] = saved_hitlog;
    sNdsFighterDashRunHitLogID = saved_hitlog_id;
    mask |= NDS_STAGE_MPLIVEHIT_RESIST_RESTORE;
    break_mask |= NDS_STAGE_MPLIVEHIT_RESIST_BREAK_RESTORE;

    gNdsStageMPLiveHitDamageLoopDamageResistMask = mask;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakMask = break_mask;
    return (((mask & 0xfffu) == 0xfffu) &&
            ((break_mask & 0x7fu) == 0x7fu)) ? TRUE : FALSE;
}

static sb32 ndsFighterDashRunStepAttackDamageHitStats(FTStruct *fp,
                                                      u32 attack_id)
{
    FTAttackColl *attack_coll;
    FTStruct *target_fp;
    GObj *target_gobj;
    DObj *target_root;
    FTDamageColl *damage_coll;
    FTHitLog *hitlog;
    Vec3f impact_pos;
    f32 knockback;
    s32 expected_lr;

    if ((fp == NULL) || (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (fp->player != 1) || (attack_id != 1u) ||
        (sNdsFighterDashRunHitLogID == 0u))
    {
        return FALSE;
    }

    attack_coll = &fp->attack_colls[attack_id];
    if (attack_coll->attack_state == nGMAttackStateOff)
    {
        return FALSE;
    }

    target_fp = &sNdsFighterStructPool[0];
    target_gobj = target_fp->fighter_gobj;
    target_root = (target_gobj != NULL) ? DObjGetStruct(target_gobj) : NULL;
    damage_coll = &target_fp->damage_colls[0];
    if ((ndsFighterStructIsPoolPointer(target_fp) == FALSE) ||
        (target_fp->attr == NULL) || (target_gobj == NULL) ||
        (target_root == NULL) || (damage_coll->hitstatus != nGMHitStatusNormal))
    {
        return FALSE;
    }

    hitlog = &sNdsFighterDashRunHitLogs[0];
    if ((hitlog->attacker_object_class != nFTHitLogObjectFighter) ||
        (hitlog->attack_coll != attack_coll) ||
        (hitlog->attacker_gobj != fp->fighter_gobj) ||
        (hitlog->damage_coll != damage_coll))
    {
        return FALSE;
    }

    knockback = ftParamGetCommonKnockback(target_fp->percent_damage,
                                          target_fp->damage_queue,
                                          attack_coll->damage,
                                          attack_coll->knockback_weight,
                                          attack_coll->knockback_scale,
                                          attack_coll->knockback_base,
                                          target_fp->attr->weight,
                                          fp->handicap,
                                          target_fp->handicap);
    ndsFighterDashRunGetFighterAttackDamagePosition(&impact_pos,
                                                    attack_coll,
                                                    damage_coll);
    (void)impact_pos;

    expected_lr = (target_root->translate.vec.f.x <
                   DObjGetStruct(fp->fighter_gobj)->translate.vec.f.x)
                      ? +1
                      : -1;
    ftMainProcessHitCollisionStatsMain(target_gobj);

    if ((target_fp->damage_angle == attack_coll->angle) &&
        (target_fp->damage_element == attack_coll->element) &&
        (target_fp->damage_lr == expected_lr) &&
        (target_fp->damage_player_num == fp->player_num) &&
        (target_fp->damage_joint_id == damage_coll->joint_id) &&
        (target_fp->damage_index == damage_coll->placement) &&
        (target_fp->damage_knockback == knockback) &&
        (target_fp->damage_kind == nFTDamageKindStatus) &&
        (target_fp->damage_object_class == nFTHitLogObjectFighter) &&
        (target_fp->damage_object_kind == fp->fkind))
    {
        gNdsFighterDashRunAttackEventPositionMask |=
            NDS_FTMOTION_ATTACK_EVENT_POS_HIT_STATS;
        return TRUE;
    }
    return FALSE;
}

static sb32 ndsFighterDashRunStepAttackDamageProcParams(FTStruct *fp,
                                                        u32 attack_id)
{
    FTStruct *target_fp;
    GObj *target_gobj;
    s32 damage_before;
    s32 queue_before;
    s32 lag_before;
    s32 status_before;
    s32 attack_damage_before;
    s32 attack_hitlag;
    s32 attack_rumble_length;
    s32 saved_status_attack_id;
    s32 attack_shield_push_before;
    s32 attack_shield_hitlag = 0;
    s32 shield_damage_before;
    s32 shield_damage_hitlag = 0;
    s32 shield_break_hitlag = 0;
    s32 rebound_hitlag = 0;
    f32 saved_attack_rebound;
    f32 saved_rebound_anim_length = 0.0F;
    f32 saved_target_anim_frame;
    f32 saved_attacker_anim_frame;
    f32 saved_target_anim_speed = 0.0F;
    f32 saved_attacker_anim_speed = 0.0F;
    static FTStruct saved_target;
    static FTStruct saved_attacker;
    DObj *target_dobj;
    DObj *attacker_dobj;
    FTSpecialColl special_coll;
    void (*saved_proc_shield)(GObj *);
    void (*saved_proc_hit)(GObj *);
    void (*saved_proc_lagstart)(GObj *);
    u32 saved_guard_setoff_count;
    u32 saved_guard_setoff_ftmain_count;
    u32 saved_guard_setoff_mask;
    u32 saved_guard_setoff_callback_mask;
    s32 saved_guard_setoff_frames;
    s32 saved_guard_setoff_vel;
    u32 shield_count_before;
    u32 hit_count_before;
    u32 lagstart_count_before;
    u32 shield_lagstart_count_before;
    u32 shield_break_lagstart_count_before;
    u32 saved_rebound_wait_count;
    u32 saved_last_fgm;
    u32 rumble_count_before;
    u32 rebound_mask = 0u;
    sb32 saved_rebound_active;
    s32 hitlag;
    s32 rumble_length;
    u32 mask = 0u;

    if ((fp == NULL) || (attack_id != 1u) || (fp->player != 1) ||
        (fp->fighter_gobj == NULL) ||
        ((gNdsFighterDashRunAttackEventPositionMask &
          NDS_FTMOTION_ATTACK_EVENT_POS_HIT_STATS) == 0u))
    {
        return FALSE;
    }

    target_fp = &sNdsFighterStructPool[0];
    target_gobj = target_fp->fighter_gobj;
    if ((ndsFighterStructIsPoolPointer(target_fp) == FALSE) ||
        (target_gobj == NULL) || (target_fp->damage_knockback == 0.0F) ||
        (target_fp->damage_queue <= 0) || (target_fp->damage_lag <= 0))
    {
        return FALSE;
    }

    attack_damage_before = fp->attack_damage;
    if (attack_damage_before <= 0)
    {
        return FALSE;
    }

    saved_proc_hit = fp->proc_hit;
    hit_count_before = sNdsFighterDashRunProcParamsHitCount;
    fp->proc_hit = ndsFighterDashRunProcParamsHit;
    gNdsFighterDashRunProcParamsRumbleMask = 0u;
    gNdsFighterDashRunProcParamsRumbleCount = 0u;
    gNdsFighterDashRunProcParamsRumbleLastID = 0u;
    gNdsFighterDashRunProcParamsRumbleLastLength = 0;

    attack_hitlag = ftParamGetHitLag(fp->attack_damage, fp->status_id,
                                     fp->hitlag_mul);
    attack_rumble_length = (s32)(((f32)fp->attack_damage * 0.5F) + 2.0F);
    sNdsFighterDashRunProcParamsRumbleActive = TRUE;
    ftMainProcParams(fp->fighter_gobj);
    sNdsFighterDashRunProcParamsRumbleActive = FALSE;
    if ((attack_rumble_length > 0) &&
        (gNdsFighterDashRunProcParamsRumbleCount == 1u) &&
        (gNdsFighterDashRunProcParamsRumbleLastID == 5u) &&
        (gNdsFighterDashRunProcParamsRumbleLastLength ==
            attack_rumble_length))
    {
        gNdsFighterDashRunProcParamsRumbleMask |= 1u;
    }
    if ((sNdsFighterDashRunProcParamsHitCount == (hit_count_before + 1u)) &&
        (attack_hitlag > 0) && (fp->hitlag_tics == attack_hitlag) &&
        (fp->input.pl.button_tap == 0u) &&
        (fp->input.pl.button_release == 0u) &&
        (fp->attack_damage == 0))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_ATTACK_DAMAGE;
    }
    fp->proc_hit = saved_proc_hit;

    saved_status_attack_id = fp->stat_flags.attack_id;
    rumble_count_before = gNdsFighterDashRunProcParamsRumbleCount;
    fp->attack_damage = attack_damage_before;
    fp->stat_flags.attack_id = nFTStatusAttackIDBatSwing4;
    sNdsFighterDashRunProcParamsRumbleActive = TRUE;
    ftMainProcParams(fp->fighter_gobj);
    sNdsFighterDashRunProcParamsRumbleActive = FALSE;
    if ((gNdsFighterDashRunProcParamsRumbleCount ==
            (rumble_count_before + 1u)) &&
        (gNdsFighterDashRunProcParamsRumbleLastID == 10u) &&
        (gNdsFighterDashRunProcParamsRumbleLastLength == 0))
    {
        gNdsFighterDashRunProcParamsRumbleMask |= 1u << 1u;
    }
    fp->stat_flags.attack_id = saved_status_attack_id;

    saved_proc_shield = fp->proc_shield;
    saved_attack_rebound = fp->attack_rebound;
    shield_count_before = sNdsFighterDashRunProcParamsShieldCount;
    attack_shield_push_before = attack_damage_before;
    fp->attack_shield_push = attack_shield_push_before;
    fp->attack_rebound = 0.0F;
    fp->proc_shield = ndsFighterDashRunProcParamsShield;
    attack_shield_hitlag =
        ftParamGetHitLag(fp->attack_shield_push, fp->status_id,
                         fp->hitlag_mul);
    ftMainProcParams(fp->fighter_gobj);
    if ((sNdsFighterDashRunProcParamsShieldCount ==
            (shield_count_before + 1u)) &&
        (attack_shield_push_before > 0) &&
        (attack_shield_hitlag > 0) &&
        (fp->hitlag_tics == attack_shield_hitlag) &&
        (fp->input.pl.button_tap == 0u) &&
        (fp->input.pl.button_release == 0u) &&
        (fp->attack_shield_push == 0))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_ATTACK_SHIELD_PUSH;
    }
    fp->proc_shield = saved_proc_shield;
    fp->attack_rebound = saved_attack_rebound;

    saved_attacker = *fp;
    attacker_dobj = DObjGetStruct(fp->fighter_gobj);
    saved_attacker_anim_frame = fp->fighter_gobj->anim_frame;
    if (attacker_dobj != NULL)
    {
        saved_attacker_anim_speed = attacker_dobj->anim_speed;
    }
    if (fp->attr != NULL)
    {
        saved_rebound_anim_length = fp->attr->rebound_anim_length;
        fp->attr->rebound_anim_length = 18.0F;
    }
    saved_rebound_wait_count = gNdsStageMPPassiveLoopReboundWaitSetStatusCount;
    saved_rebound_active = sNdsStageMPPassiveLoopReboundActive;
    fp->proc_shield = NULL;
    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->attack_shield_push = attack_damage_before;
    fp->attack_rebound = 6.0F;
    fp->lr = +1;
    fp->hit_lr = +1;
    fp->ga = nMPKineticsGround;
    fp->hitlag_mul = 1.0F;
    fp->shield_damage = 0;
    fp->shield_damage_total = 0;
    fp->damage_lag = 0;
    fp->damage_queue = 0;
    fp->damage_kind = nFTDamageKindDefault;
    fp->damage_knockback = 0.0F;
    rebound_hitlag = ftParamGetHitLag(fp->attack_shield_push, fp->status_id,
                                      fp->hitlag_mul);
    ftMainProcParams(fp->fighter_gobj);
    if ((fp->status_id == nFTCommonStatusReboundWait) &&
        (fp->motion_id == nFTCommonMotionNull))
    {
        rebound_mask |= NDS_FTMAIN_PROCPARAMS_REBOUND_STATUS;
    }
    if ((fp->proc_update == ndsBaseFTCommonReboundWaitProcUpdate) &&
        (fp->proc_interrupt == NULL) &&
        (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
        (fp->proc_map == mpCommonSetFighterFallOnGroundBreak))
    {
        rebound_mask |= NDS_FTMAIN_PROCPARAMS_REBOUND_CALLBACKS;
    }
    if ((ndsFloatToMilliSigned(fp->physics.vel_ground.x) == -12000) &&
        (ndsFloatToMilliSigned(fp->status_vars.common.rebound.anim_speed) ==
            3000) &&
        (ndsFloatToMilliSigned(fp->status_vars.common.rebound.rebound_timer) ==
            6000))
    {
        rebound_mask |= NDS_FTMAIN_PROCPARAMS_REBOUND_VECTOR;
    }
    if ((rebound_hitlag > 0) && (fp->hitlag_tics == rebound_hitlag))
    {
        rebound_mask |= NDS_FTMAIN_PROCPARAMS_REBOUND_HITLAG;
    }
    if ((fp->attack_rebound == 0.0F) && (fp->attack_shield_push == 0) &&
        (fp->shield_damage == 0) && (fp->shield_damage_total == 0))
    {
        rebound_mask |= NDS_FTMAIN_PROCPARAMS_REBOUND_CLEAR;
    }
    gNdsFighterDashRunProcParamsRumbleMask |= (rebound_mask << 2);
    *fp = saved_attacker;
    fp->fighter_gobj->anim_frame = saved_attacker_anim_frame;
    if (attacker_dobj != NULL)
    {
        attacker_dobj->anim_speed = saved_attacker_anim_speed;
    }
    if (fp->attr != NULL)
    {
        fp->attr->rebound_anim_length = saved_rebound_anim_length;
    }
    gNdsStageMPPassiveLoopReboundWaitSetStatusCount =
        saved_rebound_wait_count;
    sNdsStageMPPassiveLoopReboundActive = saved_rebound_active;

    saved_target = *target_fp;
    target_dobj = DObjGetStruct(target_gobj);
    saved_target_anim_frame = target_gobj->anim_frame;
    if (target_dobj != NULL)
    {
        saved_target_anim_speed = target_dobj->anim_speed;
    }
    saved_guard_setoff_count = gNdsFighterDashRunGuardSetOffSetStatusCount;
    saved_guard_setoff_ftmain_count =
        gNdsFighterDashRunFtMainGuardSetOffStatusCount;
    saved_guard_setoff_mask = gNdsFighterDashRunGuardSetOffMask;
    saved_guard_setoff_callback_mask =
        gNdsFighterDashRunGuardSetOffCallbackMask;
    saved_guard_setoff_frames = gNdsFighterDashRunGuardSetOffFramesMilli;
    saved_guard_setoff_vel = gNdsFighterDashRunGuardSetOffVelMilli;

    target_fp->status_id = nFTCommonStatusGuard;
    target_fp->motion_id = nFTCommonMotionGuardOn;
    target_fp->lr = 1;
    target_fp->shield_lr = 1;
    target_fp->shield_health = 55;
    target_fp->shield_damage = 10;
    target_fp->shield_damage_total = 0;
    target_fp->damage_lag = 0;
    target_fp->damage_queue = 0;
    target_fp->damage_kind = nFTDamageKindDefault;
    target_fp->damage_knockback = 0.0F;
    target_fp->hitlag_mul = 1.0F;
    target_fp->input.pl.button_tap = 0xffffu;
    target_fp->input.pl.button_release = 0xffffu;
    shield_lagstart_count_before = sNdsFighterDashRunProcParamsLagStartCount;

    shield_damage_before = target_fp->shield_damage;
    sNdsFighterDashRunGuardOnActive = TRUE;
    target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
    shield_damage_hitlag =
        ftParamGetHitLag(target_fp->shield_damage,
                         nFTCommonStatusGuard,
                         target_fp->hitlag_mul);
    ftMainProcParams(target_gobj);
    sNdsFighterDashRunGuardOnActive = FALSE;
    if ((shield_damage_before == 10) &&
        (shield_damage_hitlag > 0) &&
        (target_fp->status_id == nFTCommonStatusGuardSetOff) &&
        (target_fp->motion_id == nFTCommonMotionGuardOn) &&
        (target_fp->is_shield != FALSE) &&
        (target_fp->status_vars.common.guard.is_setoff != FALSE) &&
        (ndsFloatToMilliSigned(
             target_fp->status_vars.common.guard.setoff_frames) == 20200) &&
        (ndsFloatToMilliSigned(target_fp->physics.vel_ground.x) == -40400) &&
        (target_fp->hitlag_tics == shield_damage_hitlag) &&
        (target_fp->is_knockback_paused == saved_target.is_knockback_paused) &&
        (target_fp->input.pl.button_tap == 0u) &&
        (target_fp->input.pl.button_release == 0u) &&
        (target_fp->shield_damage == 0) &&
        (target_fp->shield_damage_total == 0) &&
        (sNdsFighterDashRunProcParamsLagStartCount ==
            (shield_lagstart_count_before + 1u)))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_SHIELD_DAMAGE;
    }
    *target_fp = saved_target;
    target_gobj->anim_frame = saved_target_anim_frame;
    if (target_dobj != NULL)
    {
        target_dobj->anim_speed = saved_target_anim_speed;
    }
    gNdsFighterDashRunGuardSetOffSetStatusCount = saved_guard_setoff_count;
    gNdsFighterDashRunFtMainGuardSetOffStatusCount =
        saved_guard_setoff_ftmain_count;
    gNdsFighterDashRunGuardSetOffMask = saved_guard_setoff_mask;
    gNdsFighterDashRunGuardSetOffCallbackMask =
        saved_guard_setoff_callback_mask;
    gNdsFighterDashRunGuardSetOffFramesMilli = saved_guard_setoff_frames;
    gNdsFighterDashRunGuardSetOffVelMilli = saved_guard_setoff_vel;

    saved_target = *target_fp;
    saved_target_anim_frame = target_gobj->anim_frame;
    if (target_dobj != NULL)
    {
        saved_target_anim_speed = target_dobj->anim_speed;
    }
    saved_last_fgm = gNdsSCVSBattleLastFGM;

    target_fp->status_id = nFTCommonStatusGuard;
    target_fp->motion_id = nFTCommonMotionGuardOn;
    target_fp->lr = 1;
    target_fp->shield_lr = 1;
    target_fp->shield_health = 5;
    target_fp->shield_damage = 10;
    target_fp->shield_damage_total = 10;
    target_fp->damage_lag = 0;
    target_fp->damage_queue = 0;
    target_fp->damage_kind = nFTDamageKindDefault;
    target_fp->damage_knockback = 0.0F;
    target_fp->hitlag_mul = 1.0F;
    target_fp->hitlag_tics = 0;
    target_fp->input.pl.button_tap = 0xffffu;
    target_fp->input.pl.button_release = 0xffffu;
    shield_break_lagstart_count_before =
        sNdsFighterDashRunProcParamsLagStartCount;

    shield_damage_before = target_fp->shield_damage;
    target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
    shield_break_hitlag =
        ftParamGetHitLag(target_fp->shield_damage,
                         nFTCommonStatusGuard,
                         target_fp->hitlag_mul);
    ftMainProcParams(target_gobj);
    if ((shield_damage_before == 10) &&
        (shield_break_hitlag > 0) &&
        (target_fp->shield_health == 30) &&
        (target_fp->status_id == nFTCommonStatusShieldBreakFly) &&
        (target_fp->motion_id == nFTCommonMotionShieldBreakFly) &&
        (target_fp->ga == nMPKineticsAir) &&
        (target_fp->hitlag_tics == shield_break_hitlag) &&
        (target_fp->input.pl.button_tap == 0u) &&
        (target_fp->input.pl.button_release == 0u) &&
        (target_fp->shield_damage == 0) &&
        (target_fp->shield_damage_total == 0) &&
        (gNdsSCVSBattleLastFGM == (u32)nSYAudioFGMShieldBreak) &&
        (sNdsFighterDashRunProcParamsLagStartCount ==
            (shield_break_lagstart_count_before + 1u)))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_SHIELD_BREAK;
    }
    *target_fp = saved_target;
    target_gobj->anim_frame = saved_target_anim_frame;
    if (target_dobj != NULL)
    {
        target_dobj->anim_speed = saved_target_anim_speed;
    }
    gNdsSCVSBattleLastFGM = saved_last_fgm;

    special_coll.kind = nFTSpecialCollKindFoxReflector;
    special_coll.joint_id = nFTPartsJointTopN;
    special_coll.offset.x = 0.0F;
    special_coll.offset.y = 0.0F;
    special_coll.offset.z = 0.0F;
    special_coll.size.x = 100.0F;
    special_coll.size.y = 100.0F;
    special_coll.size.z = 100.0F;
    special_coll.damage_resist = 0;

    saved_target = *target_fp;
    saved_target_anim_frame = target_gobj->anim_frame;
    if (target_dobj != NULL)
    {
        saved_target_anim_speed = target_dobj->anim_speed;
    }
    saved_last_fgm = gNdsSCVSBattleLastFGM;
    target_fp->special_coll = &special_coll;
    target_fp->reflect_lr = -1;
    target_fp->reflect_damage = 6;
    target_fp->shield_damage = 0;
    target_fp->shield_damage_total = 0;
    target_fp->damage_lag = 0;
    target_fp->damage_queue = 0;
    target_fp->damage_kind = nFTDamageKindDefault;
    target_fp->damage_knockback = 0.0F;
    ftMainProcParams(target_gobj);
    if ((target_fp->status_id == nFTCommonStatusShieldBreakFly) &&
        (target_fp->motion_id == nFTCommonMotionShieldBreakFly) &&
        (target_fp->ga == nMPKineticsAir) &&
        (target_fp->reflect_lr == 0) &&
        (target_fp->reflect_damage == 0) &&
        (gNdsSCVSBattleLastFGM == (u32)nSYAudioFGMShieldBreak))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_REFLECT_BREAK;
    }
    *target_fp = saved_target;
    target_gobj->anim_frame = saved_target_anim_frame;
    if (target_dobj != NULL)
    {
        target_dobj->anim_speed = saved_target_anim_speed;
    }
    gNdsSCVSBattleLastFGM = saved_last_fgm;

    saved_target = *target_fp;
    special_coll.kind = nFTSpecialCollKindFoxReflector;
    target_fp->special_coll = &special_coll;
    target_fp->reflect_lr = -1;
    target_fp->is_reflect = FALSE;
    target_fp->shield_damage = 0;
    target_fp->shield_damage_total = 0;
    target_fp->damage_lag = 0;
    target_fp->damage_queue = 0;
    target_fp->damage_kind = nFTDamageKindDefault;
    target_fp->damage_knockback = 0.0F;
    ftMainProcParams(target_gobj);
    if ((target_fp->lr == -1) && (target_fp->is_reflect != FALSE) &&
        (target_fp->reflect_lr == 0))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_REFLECT_HIT;
    }
    *target_fp = saved_target;

    saved_target = *target_fp;
    saved_last_fgm = gNdsSCVSBattleLastFGM;
    special_coll.kind = nFTSpecialCollKindNessReflector;
    target_fp->special_coll = &special_coll;
    target_fp->reflect_lr = 1;
    target_fp->shield_damage = 0;
    target_fp->shield_damage_total = 0;
    target_fp->damage_lag = 0;
    target_fp->damage_queue = 0;
    target_fp->damage_kind = nFTDamageKindDefault;
    target_fp->damage_knockback = 0.0F;
    ftMainProcParams(target_gobj);
    if ((gNdsSCVSBattleLastFGM == (u32)nSYAudioFGMBatHit) &&
        (target_fp->reflect_lr == 0))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_REFLECT_SOUND;
    }
    *target_fp = saved_target;
    gNdsSCVSBattleLastFGM = saved_last_fgm;

    saved_target = *target_fp;
    target_fp->absorb_lr = 1;
    target_fp->is_absorb = FALSE;
    target_fp->shield_damage = 0;
    target_fp->shield_damage_total = 0;
    target_fp->damage_lag = 0;
    target_fp->damage_queue = 0;
    target_fp->damage_kind = nFTDamageKindDefault;
    target_fp->damage_knockback = 0.0F;
    ftMainProcParams(target_gobj);
    if ((target_fp->lr == 1) && (target_fp->is_absorb != FALSE) &&
        (target_fp->absorb_lr == 0))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_ABSORB;
    }
    *target_fp = saved_target;

    damage_before = target_fp->percent_damage;
    queue_before = target_fp->damage_queue;
    lag_before = target_fp->damage_lag;
    status_before = target_fp->status_id;
    saved_proc_lagstart = target_fp->proc_lagstart;
    lagstart_count_before = sNdsFighterDashRunProcParamsLagStartCount;
    target_fp->proc_lagstart = ndsFighterDashRunProcParamsLagStart;
    saved_target = *target_fp;
    saved_target_anim_frame = target_gobj->anim_frame;
    if (target_dobj != NULL)
    {
        saved_target_anim_speed = target_dobj->anim_speed;
    }

    mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_INPUT;
    ftParamUpdateDamage(target_fp, queue_before);
    if (target_fp->percent_damage == (damage_before + queue_before))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_UPDATE_DAMAGE;
    }

    ftParamStopVoiceRunProcDamage(target_gobj);
    ftCommonDamageGotoDamageStatus(target_gobj);
    if (ndsFighterDashRunProbeDamageStatusSelect(target_fp,
                                                 status_before) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS;
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS_SELECT;
    }
    if (ndsFighterDashRunProbeDamageStatusSetup(target_gobj, target_fp,
                                                status_before) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS_SETUP;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainCatch(
            target_gobj, target_fp, fp->fighter_gobj, fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainRelease(
            target_gobj, target_fp, fp->fighter_gobj, fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_RELEASE;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainCatchStats(
            target_gobj, target_fp, fp->fighter_gobj, fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_STATS;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainCatchNoDamage(
            target_gobj, target_fp, fp->fighter_gobj, fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_NODAMAGE;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainCapture(
            target_gobj, target_fp, fp->fighter_gobj, fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainCaptureStats(
            target_gobj, target_fp, fp->fighter_gobj, fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_STATS;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainCaptureRelease(
            target_gobj, target_fp, fp->fighter_gobj, fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_RELEASE;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainCaptureNoDamage(
            target_gobj, target_fp, fp->fighter_gobj, fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_NODAMAGE;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainCatchZero(
            target_gobj, target_fp, fp->fighter_gobj, fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_ZERO;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainCaptureZero(
            target_gobj, target_fp, fp->fighter_gobj, fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_ZERO;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainTailColAnim(
            target_gobj, target_fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_TAIL_COLANIM;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainTailStatus(
            target_gobj, target_fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_TAIL_STATUS;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainItemResist(
            target_gobj, target_fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_ITEM_RESIST;
    }
    if (ndsFighterDashRunProbeDamageUpdateMainItemDrop(
            target_gobj, target_fp) != FALSE)
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_ITEM_DROP;
    }
    (void)ndsFighterDashRunProbeDamageUpdateCatchResist(target_gobj,
                                                        target_fp);
    (void)ndsFighterDashRunProbeDamageUpdateMainItemHeavy(target_gobj,
                                                          target_fp);
    (void)ndsFighterDashRunProbeDamageUpdateMainItemBypass(target_gobj,
                                                           target_fp);
    (void)ndsFighterDashRunProbeDamageUpdateMainSleepStatus(target_gobj,
                                                            target_fp);

    *target_fp = saved_target;
    target_gobj->anim_frame = saved_target_anim_frame;
    if (target_dobj != NULL)
    {
        target_dobj->anim_speed = saved_target_anim_speed;
    }
    rumble_count_before = gNdsFighterDashRunProcParamsRumbleCount;
    sNdsFighterDashRunProcParamsRumbleActive = TRUE;
    ftMainProcParams(target_gobj);
    sNdsFighterDashRunProcParamsRumbleActive = FALSE;

    if (target_fp->percent_damage == (damage_before + queue_before))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_UPDATE_DAMAGE;
    }
    if ((target_fp->status_id != status_before) &&
        (ndsFTCommonDamageIsStatus(target_fp->status_id) != FALSE))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS;
    }
    if ((target_fp->shuffle_tics > 0) &&
        (target_fp->shuffle_index_max > 0))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_SHUFFLE;
    }

    rumble_length = (s32)(((f32)queue_before * 0.75F) + 4.0F);
    if ((rumble_length > 0) &&
        (gNdsFighterDashRunProcParamsRumbleCount > rumble_count_before) &&
        (gNdsFighterDashRunProcParamsRumbleLastID == 0u) &&
        (gNdsFighterDashRunProcParamsRumbleLastLength == rumble_length))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_RUMBLE;
    }

    hitlag = target_fp->hitlag_tics;
    if (sNdsFighterDashRunProcParamsLagStartCount ==
        (lagstart_count_before + 1u))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_LAGSTART;
    }
    target_fp->proc_lagstart = saved_proc_lagstart;
    if ((target_fp->hitlag_tics == hitlag) &&
        (target_fp->is_knockback_paused != FALSE))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_HITLAG;
    }

    if ((target_fp->damage_lag == 0) &&
        (target_fp->damage_queue == 0) &&
        (target_fp->damage_kind == nFTDamageKindDefault) &&
        (target_fp->damage_knockback == 0.0F) &&
        (target_fp->shield_damage_total == 0) &&
        (target_fp->hitlag_mul == 1.0F))
    {
        mask |= NDS_FTMAIN_PROCPARAMS_CLEAR;
    }

    gNdsFighterDashRunProcParamsMask = mask;
    gNdsFighterDashRunProcParamsDamageBefore = damage_before;
    gNdsFighterDashRunProcParamsDamageAfter = target_fp->percent_damage;
    gNdsFighterDashRunProcParamsQueueBefore = queue_before;
    gNdsFighterDashRunProcParamsLagBefore = lag_before;
    gNdsFighterDashRunProcParamsHitlag = target_fp->hitlag_tics;
    gNdsFighterDashRunProcParamsPaused =
        (target_fp->is_knockback_paused != FALSE) ? 1u : 0u;
    gNdsFighterDashRunProcParamsStatusBefore = (u32)status_before;
    gNdsFighterDashRunProcParamsStatusAfter = (u32)target_fp->status_id;

    if ((mask & (NDS_FTMAIN_PROCPARAMS_DAMAGE_INPUT |
                 NDS_FTMAIN_PROCPARAMS_UPDATE_DAMAGE |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS |
                 NDS_FTMAIN_PROCPARAMS_SHUFFLE |
                 NDS_FTMAIN_PROCPARAMS_RUMBLE |
                 NDS_FTMAIN_PROCPARAMS_HITLAG |
                 NDS_FTMAIN_PROCPARAMS_CLEAR |
                 NDS_FTMAIN_PROCPARAMS_LAGSTART |
                 NDS_FTMAIN_PROCPARAMS_ATTACK_DAMAGE |
                 NDS_FTMAIN_PROCPARAMS_ATTACK_SHIELD_PUSH |
                 NDS_FTMAIN_PROCPARAMS_SHIELD_DAMAGE |
                 NDS_FTMAIN_PROCPARAMS_SHIELD_BREAK |
                 NDS_FTMAIN_PROCPARAMS_REFLECT_BREAK |
                 NDS_FTMAIN_PROCPARAMS_REFLECT_HIT |
                 NDS_FTMAIN_PROCPARAMS_REFLECT_SOUND |
                 NDS_FTMAIN_PROCPARAMS_ABSORB |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS_SELECT |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS_SETUP |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_RELEASE |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_ZERO |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_ZERO |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_STATS |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_STATS |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_RELEASE |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_NODAMAGE |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_NODAMAGE |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_TAIL_COLANIM |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_TAIL_STATUS |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_ITEM_RESIST |
                 NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_ITEM_DROP)) ==
        (NDS_FTMAIN_PROCPARAMS_DAMAGE_INPUT |
         NDS_FTMAIN_PROCPARAMS_UPDATE_DAMAGE |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS |
         NDS_FTMAIN_PROCPARAMS_SHUFFLE |
         NDS_FTMAIN_PROCPARAMS_RUMBLE |
         NDS_FTMAIN_PROCPARAMS_HITLAG |
         NDS_FTMAIN_PROCPARAMS_CLEAR |
         NDS_FTMAIN_PROCPARAMS_LAGSTART |
         NDS_FTMAIN_PROCPARAMS_ATTACK_DAMAGE |
         NDS_FTMAIN_PROCPARAMS_ATTACK_SHIELD_PUSH |
         NDS_FTMAIN_PROCPARAMS_SHIELD_DAMAGE |
         NDS_FTMAIN_PROCPARAMS_SHIELD_BREAK |
         NDS_FTMAIN_PROCPARAMS_REFLECT_BREAK |
         NDS_FTMAIN_PROCPARAMS_REFLECT_HIT |
         NDS_FTMAIN_PROCPARAMS_REFLECT_SOUND |
         NDS_FTMAIN_PROCPARAMS_ABSORB |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS_SELECT |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS_SETUP |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_RELEASE |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_ZERO |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_ZERO |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_STATS |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_STATS |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_RELEASE |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_NODAMAGE |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_NODAMAGE |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_TAIL_COLANIM |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_TAIL_STATUS |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_ITEM_RESIST |
         NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_ITEM_DROP))
    {
        gNdsFighterDashRunAttackEventPositionMask |=
            NDS_FTMOTION_ATTACK_EVENT_POS_PROCPARAMS;
        return TRUE;
    }
    return FALSE;
}

static sb32 ndsFighterDashRunStepAttackCollFighterRange(FTStruct *fp,
                                                        u32 attack_id)
{
    FTAttackColl *attack_coll;
    FTStruct *target_fp;
    GObj *target_gobj;
    DObj *target_root;
    Vec3f target_pos_saved;
    u32 target_player;
    sb32 is_in_range = FALSE;

    if ((fp == NULL) || (attack_id >= FTATTACKCOLL_NUM_MAX) ||
        (fp->player >= 2))
    {
        return FALSE;
    }
    attack_coll = &fp->attack_colls[attack_id];
    if (attack_coll->attack_state == nGMAttackStateOff)
    {
        return FALSE;
    }

    target_player = (fp->player == 0) ? 1u : 0u;
    target_fp = &sNdsFighterStructPool[target_player];
    target_gobj = target_fp->fighter_gobj;
    target_root = (target_gobj != NULL) ? DObjGetStruct(target_gobj) : NULL;
    if ((ndsFighterStructIsPoolPointer(target_fp) == FALSE) ||
        (target_fp->attr == NULL) || (target_root == NULL))
    {
        return FALSE;
    }

    target_pos_saved = target_root->translate.vec.f;
    target_root->translate.vec.f = attack_coll->pos_curr;

    if (ndsFighterDashRunCheckAttackInFighterRange(
            &attack_coll->pos_curr, &target_root->translate.vec.f,
            &target_fp->attr->hit_detect_range, attack_coll->size) != FALSE)
    {
        gNdsFighterDashRunAttackEventPositionMask |=
            NDS_FTMOTION_ATTACK_EVENT_POS_RANGE_CURR;
        is_in_range = TRUE;
    }
    if ((attack_coll->attack_state != nGMAttackStateTransfer) &&
        (ndsFighterDashRunCheckAttackInFighterRange(
            &attack_coll->pos_prev, &target_root->translate.vec.f,
            &target_fp->attr->hit_detect_range, attack_coll->size) != FALSE))
    {
        gNdsFighterDashRunAttackEventPositionMask |=
            NDS_FTMOTION_ATTACK_EVENT_POS_RANGE_PREV;
        is_in_range = TRUE;
    }

    target_root->translate.vec.f = target_pos_saved;
    return is_in_range;
}

#define NDS_STAGE_MPLIVEHIT_SECONDARY_DECODED    (1u << 0)
#define NDS_STAGE_MPLIVEHIT_SECONDARY_METADATA   (1u << 1)
#define NDS_STAGE_MPLIVEHIT_SECONDARY_TRANSFER   (1u << 2)
#define NDS_STAGE_MPLIVEHIT_SECONDARY_INTERP     (1u << 3)
#define NDS_STAGE_MPLIVEHIT_SECONDARY_RANGE      (1u << 4)
#define NDS_STAGE_MPLIVEHIT_SECONDARY_RECT       (1u << 5)
#define NDS_STAGE_MPLIVEHIT_SECONDARY_COLLIDE    (1u << 6)

#define NDS_FTMOTION_ATTACK_RECORD_CARRY_CLEAR   (1u << 0)
#define NDS_FTMOTION_ATTACK_RECORD_CARRY_SEED    (1u << 1)
#define NDS_FTMOTION_ATTACK_RECORD_CARRY_COPY    (1u << 2)
#define NDS_FTMOTION_ATTACK_RECORD_CARRY_RESTORE (1u << 3)

static void ndsFighterDashRunProbeAttackRecordCarry(FTStruct *fp)
{
    FTAttackColl *source_coll;
    FTAttackColl *dest_coll;
    FTAttackColl saved_source;
    FTAttackColl saved_dest;
    GObj *victim_gobj;
    sb32 saved_is_attack_active;
    u32 group_id;
    u32 i;
    u32 j;

    if ((ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() == FALSE) ||
        ((gNdsFighterDashRunAttackEventRecordCarryMask & 0xfu) == 0xfu) ||
        (fp == NULL) || (fp->player != 1u) ||
        (ndsFighterStructIsPoolPointer(&sNdsFighterStructPool[0]) == FALSE) ||
        (sNdsFighterStructPool[0].fighter_gobj == NULL))
    {
        return;
    }

    source_coll = &fp->attack_colls[2];
    dest_coll = &fp->attack_colls[3];
    saved_source = *source_coll;
    saved_dest = *dest_coll;
    saved_is_attack_active = fp->is_attack_active;
    victim_gobj = sNdsFighterStructPool[0].fighter_gobj;

    ftParamClearAttackRecordID(fp, 2);
    ftParamClearAttackRecordID(fp, 3);
    source_coll->attack_state = nGMAttackStateInterpolate;
    source_coll->group_id = 5u;
    source_coll->attack_records[0].victim_gobj = victim_gobj;
    source_coll->attack_records[0].victim_flags.is_interact_hurt = FALSE;
    source_coll->attack_records[0].victim_flags.is_interact_shield = FALSE;
    source_coll->attack_records[0].victim_flags.timer_rehit = 0u;
    source_coll->attack_records[0].victim_flags.group_id = 7u;
    gNdsFighterDashRunAttackEventRecordCarryMask |=
        NDS_FTMOTION_ATTACK_RECORD_CARRY_SEED;

    group_id = 5u;
    dest_coll->attack_state = nGMAttackStateOff;
    if ((dest_coll->attack_state == nGMAttackStateOff) ||
        (dest_coll->group_id != group_id))
    {
        dest_coll->group_id = group_id;
        dest_coll->attack_state = nGMAttackStateNew;
        fp->is_attack_active = TRUE;

        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            FTAttackColl *other_coll = &fp->attack_colls[i];

            if ((i != 3u) &&
                (other_coll->attack_state != nGMAttackStateOff) &&
                (dest_coll->group_id == other_coll->group_id))
            {
                for (j = 0u; j < GMATTACKREC_NUM_MAX; j++)
                {
                    dest_coll->attack_records[j] =
                        other_coll->attack_records[j];
                }
                break;
            }
        }
        if ((i != FTATTACKCOLL_NUM_MAX) &&
            (dest_coll->attack_records[0].victim_gobj == victim_gobj) &&
            (dest_coll->attack_records[0].victim_flags.group_id == 7u))
        {
            gNdsFighterDashRunAttackEventRecordCarryMask |=
                NDS_FTMOTION_ATTACK_RECORD_CARRY_COPY;
        }
    }

    source_coll->attack_state = nGMAttackStateOff;
    dest_coll->attack_state = nGMAttackStateOff;
    group_id = 6u;
    if ((dest_coll->attack_state == nGMAttackStateOff) ||
        (dest_coll->group_id != group_id))
    {
        dest_coll->group_id = group_id;
        dest_coll->attack_state = nGMAttackStateNew;
        fp->is_attack_active = TRUE;

        for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            FTAttackColl *other_coll = &fp->attack_colls[i];

            if ((i != 3u) &&
                (other_coll->attack_state != nGMAttackStateOff) &&
                (dest_coll->group_id == other_coll->group_id))
            {
                break;
            }
        }
        if (i == FTATTACKCOLL_NUM_MAX)
        {
            ftParamClearAttackRecordID(fp, 3);
            if ((dest_coll->attack_records[0].victim_gobj == NULL) &&
                (dest_coll->attack_records[0].victim_flags.group_id == 7u))
            {
                gNdsFighterDashRunAttackEventRecordCarryMask |=
                    NDS_FTMOTION_ATTACK_RECORD_CARRY_CLEAR;
            }
        }
    }

    *source_coll = saved_source;
    *dest_coll = saved_dest;
    fp->is_attack_active = saved_is_attack_active;
    gNdsFighterDashRunAttackEventRecordCarryMask |=
        NDS_FTMOTION_ATTACK_RECORD_CARRY_RESTORE;
}

static void ndsFighterDashRunProbeSecondaryLiveHitbox(
    FTStruct *fp, u32 attack_id, const FTAttackColl *attack_coll)
{
    FTAttackColl work;
    FTAttackColl probe;
    FTStruct *target_fp;
    GObj *target_gobj;
    DObj *target_root;
    FTDamageColl *damage_coll;
    FTParts *parts;
    Vec3f target_pos_saved;
    Vec3f pos_curr;
    Vec3f pos_prev;
    u32 flags;
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() == FALSE) ||
        (gNdsStageMPLiveHitDamageLoopSecondaryMask != 0u) ||
        (fp == NULL) || (attack_coll == NULL) ||
        (fp->player != 1) ||
        ((fp->status_id != nFTCommonStatusAttack12) &&
         (gNdsFighterDashRunAttackEventLastStatus !=
             (u32)nFTCommonStatusAttack12)) ||
        (attack_id != 0u))
    {
        return;
    }

    flags = (attack_coll->is_hit_air ? 0x1u : 0u) |
            (attack_coll->is_hit_ground ? 0x2u : 0u) |
            (attack_coll->can_rebound ? 0x4u : 0u) |
            (attack_coll->is_scale_pos ? 0x8u : 0u);
    gNdsStageMPLiveHitDamageLoopSecondaryAttackID = attack_id;
    gNdsStageMPLiveHitDamageLoopSecondaryJointID =
        (u32)attack_coll->joint_id;
    gNdsStageMPLiveHitDamageLoopSecondaryDamage = attack_coll->damage;
    gNdsStageMPLiveHitDamageLoopSecondarySize = (s32)attack_coll->size;
    gNdsStageMPLiveHitDamageLoopSecondaryOffsetX =
        (s32)attack_coll->offset.x;
    gNdsStageMPLiveHitDamageLoopSecondaryAngle = attack_coll->angle;
    gNdsStageMPLiveHitDamageLoopSecondaryFlags = flags;
    mask |= NDS_STAGE_MPLIVEHIT_SECONDARY_DECODED;

    if ((attack_coll->group_id == 0u) &&
        (attack_coll->joint_id == 14) &&
        (attack_coll->damage == 4) &&
        ((s32)attack_coll->size == 100) &&
        ((s32)attack_coll->offset.x == 140) &&
        (attack_coll->angle == 70) &&
        (attack_coll->knockback_scale == 100) &&
        (attack_coll->knockback_base == 0) &&
        ((flags & 0x7u) == 0x7u))
    {
        mask |= NDS_STAGE_MPLIVEHIT_SECONDARY_METADATA;
    }

    work = *attack_coll;
    if (work.attack_state == nGMAttackStateNew)
    {
        work.pos_curr = work.offset;
        if (work.is_scale_pos != FALSE)
        {
            f32 size_mul;

            if ((fp->attr == NULL) || (fp->attr->size == 0.0F))
            {
                gNdsStageMPLiveHitDamageLoopSecondaryMask |= mask;
                return;
            }
            size_mul = 1.0F / fp->attr->size;
            work.pos_curr.x *= size_mul;
            work.pos_curr.y *= size_mul;
            work.pos_curr.z *= size_mul;
        }
        gmCollisionGetFighterPartsWorldPosition(work.joint,
                                                &work.pos_curr);
        work.attack_state = nGMAttackStateTransfer;
        work.attack_matrix.unk_fthitmtx_0x0 = FALSE;
        work.attack_matrix.unk_fthitmtx_0x44 = 0.0F;
        mask |= NDS_STAGE_MPLIVEHIT_SECONDARY_TRANSFER;
    }
    if (work.attack_state == nGMAttackStateTransfer)
    {
        work.pos_prev = work.pos_curr;
        work.pos_curr = work.offset;
        if (work.is_scale_pos != FALSE)
        {
            f32 size_mul;

            if ((fp->attr == NULL) || (fp->attr->size == 0.0F))
            {
                gNdsStageMPLiveHitDamageLoopSecondaryMask |= mask;
                return;
            }
            size_mul = 1.0F / fp->attr->size;
            work.pos_curr.x *= size_mul;
            work.pos_curr.y *= size_mul;
            work.pos_curr.z *= size_mul;
        }
        gmCollisionGetFighterPartsWorldPosition(work.joint,
                                                &work.pos_curr);
        work.attack_state = nGMAttackStateInterpolate;
        work.attack_matrix.unk_fthitmtx_0x0 = FALSE;
        work.attack_matrix.unk_fthitmtx_0x44 = 0.0F;
        mask |= NDS_STAGE_MPLIVEHIT_SECONDARY_INTERP;
    }

    target_fp = &sNdsFighterStructPool[0];
    target_gobj = target_fp->fighter_gobj;
    target_root = (target_gobj != NULL) ? DObjGetStruct(target_gobj) : NULL;
    if ((ndsFighterStructIsPoolPointer(target_fp) == FALSE) ||
        (target_fp->attr == NULL) || (target_root == NULL))
    {
        gNdsStageMPLiveHitDamageLoopSecondaryMask |= mask;
        return;
    }

    target_pos_saved = target_root->translate.vec.f;
    target_root->translate.vec.f = work.pos_curr;
    if ((ndsFighterDashRunCheckAttackInFighterRange(
            &work.pos_curr, &target_root->translate.vec.f,
            &target_fp->attr->hit_detect_range, work.size) != FALSE) ||
        (ndsFighterDashRunCheckAttackInFighterRange(
            &work.pos_prev, &target_root->translate.vec.f,
            &target_fp->attr->hit_detect_range, work.size) != FALSE))
    {
        mask |= NDS_STAGE_MPLIVEHIT_SECONDARY_RANGE;
    }
    target_root->translate.vec.f = target_pos_saved;

    damage_coll = &target_fp->damage_colls[0];
    if ((damage_coll->hitstatus != nGMHitStatusNormal) ||
        (damage_coll->joint == NULL))
    {
        gNdsStageMPLiveHitDamageLoopSecondaryMask |= mask;
        return;
    }
    parts = ftGetParts(damage_coll->joint);
    if ((parts == NULL) || (parts->vec_scale.x == 0.0F) ||
        (parts->vec_scale.y == 0.0F) || (parts->vec_scale.z == 0.0F))
    {
        gNdsStageMPLiveHitDamageLoopSecondaryMask |= mask;
        return;
    }

    pos_curr = damage_coll->offset;
    gmCollisionGetWorldPosition(parts->mtx_translate, &pos_curr);
    pos_prev = pos_curr;
    if (ndsGMCollisionTestRectangle(&pos_curr,
                                    &pos_prev,
                                    work.size,
                                    work.attack_state,
                                    parts->unk_dobjtrans_0x9C,
                                    &damage_coll->offset,
                                    &damage_coll->size,
                                    &parts->vec_scale) != FALSE)
    {
        mask |= NDS_STAGE_MPLIVEHIT_SECONDARY_RECT;
    }

    probe = work;
    probe.pos_curr = damage_coll->offset;
    gmCollisionGetWorldPosition(parts->mtx_translate, &probe.pos_curr);
    probe.pos_prev = probe.pos_curr;
    if (ndsGMCollisionCheckFighterAttackDamageCollideSelected(
            &probe, damage_coll) != FALSE)
    {
        mask |= NDS_STAGE_MPLIVEHIT_SECONDARY_COLLIDE;
    }

    gNdsStageMPLiveHitDamageLoopSecondaryMask |= mask;
}

static const u32 *ndsFighterDashRunInstallOriginalMotionScript(FTStruct *fp)
{
    const u32 *script;

    if (fp == NULL)
    {
        return NULL;
    }

    script = ndsBattleShipMarioFoxMainMotionScript(fp->fkind, fp->motion_id);
    fp->motion_scripts[0][0].p_script = (u32 *)script;
    fp->motion_scripts[1][0].p_script = (u32 *)script;
    fp->motion_scripts[0][0].script_wait = 0.0F;
    fp->motion_scripts[1][0].script_wait = 0.0F;
    fp->motion_scripts[0][0].script_id = 0;
    fp->motion_scripts[1][0].script_id = 0;
    return script;
}

static void ndsFighterDashRunDecodeAttackWords(FTStruct *fp,
                                               const u32 *words,
                                               u32 event_bit,
                                               sb32 is_scaled)
{
    u32 word;
    u32 attack_id;
    u32 group_id;
    u32 i;
    u32 j;
    FTAttackColl *attack_coll;
    s32 joint_id;
    sb32 is_writeback_selected;

    if ((fp == NULL) || (words == NULL) || (event_bit >= 32u))
    {
        return;
    }

    word = words[0];
    if ((((word >> 26) & 0x3fu) != NDS_FTMOTION_EVENT_MAKE_ATTACK_COLL) &&
        (((word >> 26) & 0x3fu) !=
            NDS_FTMOTION_EVENT_MAKE_ATTACK_COLL_SCALED))
    {
        return;
    }

    attack_id = (word >> 23) & 0x7u;
    if (attack_id >= FTATTACKCOLL_NUM_MAX)
    {
        return;
    }

    attack_coll = &fp->attack_colls[attack_id];
    group_id = (word >> 20) & 0x7u;
    if ((attack_coll->attack_state == nGMAttackStateOff) ||
        (attack_coll->group_id != group_id))
    {
        attack_coll->group_id = group_id;
        attack_coll->attack_state = nGMAttackStateNew;
        fp->is_attack_active = TRUE;

        for (i = 0; i < FTATTACKCOLL_NUM_MAX; i++)
        {
            FTAttackColl *other_coll = &fp->attack_colls[i];

            if ((i != attack_id) &&
                (other_coll->attack_state != nGMAttackStateOff) &&
                (attack_coll->group_id == other_coll->group_id))
            {
                for (j = 0; j < GMATTACKREC_NUM_MAX; j++)
                {
                    attack_coll->attack_records[j] =
                        other_coll->attack_records[j];
                }
                break;
            }
        }
        if (i == FTATTACKCOLL_NUM_MAX)
        {
            ftParamClearAttackRecordID(fp, (s32)attack_id);
        }
    }

    joint_id = ndsFTMotionSignExtend((word >> 13) & 0x7fu, 7u);
    if ((joint_id < 0) || (joint_id >= nFTPartsJointNumMax))
    {
        joint_id = nFTPartsJointTopN;
    }

    attack_coll->joint_id = joint_id;
    attack_coll->joint = fp->joints[joint_id];
    attack_coll->damage = (s32)((word >> 5) & 0xffu);
    attack_coll->can_rebound = ((word >> 4) & 0x1u) ? TRUE : FALSE;
    attack_coll->element = (s32)(word & 0xfu);

    word = words[1];
    attack_coll->size = (f32)((word >> 16) & 0xffffu) * 0.5F;
    attack_coll->offset.x = (f32)ndsFTMotionSignExtend(word & 0xffffu, 16u);

    word = words[2];
    attack_coll->offset.y =
        (f32)ndsFTMotionSignExtend((word >> 16) & 0xffffu, 16u);
    attack_coll->offset.z =
        (f32)ndsFTMotionSignExtend(word & 0xffffu, 16u);

    word = words[3];
    attack_coll->angle =
        ndsFTMotionSignExtend((word >> 22) & 0x3ffu, 10u);
    attack_coll->knockback_scale = (s32)((word >> 12) & 0x3ffu);
    attack_coll->knockback_weight = (s32)((word >> 2) & 0x3ffu);
    attack_coll->is_hit_air = (word & 0x1u) ? TRUE : FALSE;
    attack_coll->is_hit_ground = (word & 0x2u) ? TRUE : FALSE;

    word = words[4];
    attack_coll->shield_damage =
        ndsFTMotionSignExtend((word >> 24) & 0xffu, 8u);
    attack_coll->fgm_level = (word >> 21) & 0x7u;
    attack_coll->fgm_kind = (word >> 17) & 0xfu;
    attack_coll->knockback_base = (s32)((word >> 7) & 0x3ffu);
    attack_coll->is_scale_pos = is_scaled;
    attack_coll->motion_attack_id = (u32)fp->motion_attack_id & 0x3fu;
    attack_coll->motion_count = (u16)fp->motion_count;

    gNdsFighterDashRunAttackEventMask |= 1u << event_bit;
    gNdsFighterDashRunAttackEventParseCount++;
    gNdsFighterDashRunAttackEventAttackIDMask |= 1u << attack_id;
    gNdsFighterDashRunAttackEventLastPlayer = fp->player;
    gNdsFighterDashRunAttackEventLastStatus = (u32)fp->status_id;
    gNdsFighterDashRunAttackEventLastState =
        (u32)attack_coll->attack_state;
    gNdsFighterDashRunAttackEventLastAttackID = attack_id;
    gNdsFighterDashRunAttackEventLastGroupID = attack_coll->group_id;
    gNdsFighterDashRunAttackEventLastJointID = (u32)attack_coll->joint_id;
    gNdsFighterDashRunAttackEventLastDamage = attack_coll->damage;
    gNdsFighterDashRunAttackEventLastSize = (s32)attack_coll->size;
    gNdsFighterDashRunAttackEventLastOffsetX = (s32)attack_coll->offset.x;
    gNdsFighterDashRunAttackEventLastOffsetY = (s32)attack_coll->offset.y;
    gNdsFighterDashRunAttackEventLastOffsetZ = (s32)attack_coll->offset.z;
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
    ndsFighterDashRunProbeAttackRecordCarry(fp);
    ndsFighterDashRunProbeSecondaryLiveHitbox(fp, attack_id, attack_coll);
    is_writeback_selected =
        ndsFighterDashRunShouldWriteBackAttackPosition(
            fp, attack_id, attack_coll);
    if (ndsFighterDashRunStepAttackCollPosition(
            fp, attack_id, is_writeback_selected) != FALSE)
    {
        if ((ndsFighterDashRunStepAttackCollInterpolate(
                fp, attack_id) != FALSE) &&
            (is_writeback_selected != FALSE))
        {
            (void)ndsFighterDashRunStepAttackCollFighterRange(fp,
                                                              attack_id);
            (void)ndsFighterDashRunStepAttackDamageRectangle(fp,
                                                             attack_id);
            if (ndsFighterDashRunStepAttackDamageCollide(fp,
                                                         attack_id) != FALSE)
            {
                (void)ndsFighterDashRunStepAttackDamageRecord(fp,
                                                              attack_id);
                (void)ndsFighterDashRunStepAttackDamageHitLog(fp,
                                                              attack_id);
                (void)ndsFighterDashRunStepAttackDamageHitSFX(fp,
                                                              attack_id);
                (void)ndsFighterDashRunStepAttackDamageHitStats(fp,
                                                                attack_id);
                (void)ndsFighterDashRunStepAttackDamageProcParams(fp,
                                                                  attack_id);
            }
        }
    }
}

static void ndsFighterDashRunSetAttackCollDamage(FTStruct *fp, u32 word)
{
    u32 attack_id;

    if (fp == NULL)
    {
        return;
    }

    attack_id = (word >> 23) & 0x7u;
    if (attack_id >= FTATTACKCOLL_NUM_MAX)
    {
        return;
    }
    if (fp->pkind != nFTPlayerKindDemo)
    {
        fp->attack_colls[attack_id].damage = (s32)((word >> 15) & 0xffu);
    }
    gNdsFighterDashRunAttackEventCommandMask |=
        NDS_FTMOTION_ATTACK_EVENT_CMD_DAMAGE;
}

static void ndsFighterDashRunSetAttackCollSize(FTStruct *fp, u32 word)
{
    u32 attack_id;

    if (fp == NULL)
    {
        return;
    }

    attack_id = (word >> 23) & 0x7u;
    if (attack_id >= FTATTACKCOLL_NUM_MAX)
    {
        return;
    }
    fp->attack_colls[attack_id].size =
        (f32)((word >> 7) & 0xffffu) * 0.5F;
    gNdsFighterDashRunAttackEventCommandMask |=
        NDS_FTMOTION_ATTACK_EVENT_CMD_SIZE;
}

static void ndsFighterDashRunClearAttackCollAll(GObj *fighter_gobj,
                                                FTStruct *fp)
{
    u32 i;
    sb32 is_clear = TRUE;

    ftParamClearAttackCollAll(fighter_gobj);
    gNdsFighterDashRunAttackEventCommandMask |=
        NDS_FTMOTION_ATTACK_EVENT_CMD_CLEAR;

    if (fp == NULL)
    {
        return;
    }
    for (i = 0; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        if (fp->attack_colls[i].attack_state != nGMAttackStateOff)
        {
            is_clear = FALSE;
            break;
        }
    }
    if ((is_clear != FALSE) && (fp->is_attack_active == FALSE))
    {
        gNdsFighterDashRunAttackEventCommandMask |=
            NDS_FTMOTION_ATTACK_EVENT_CMD_CLEAR_OFF;
    }
}

static void ndsFighterDashRunDecodeAttackEvent(GObj *fighter_gobj,
                                               FTStruct *fp,
                                               u32 event_bit)
{
    const u32 *script;
    u32 index;
    sb32 decoded_attack = FALSE;

    if ((fp == NULL) || (event_bit >= 32u))
    {
        return;
    }

    script = ndsFighterDashRunInstallOriginalMotionScript(fp);
    if (script == NULL)
    {
        gNdsFighterDashRunAttackEventNoHitMask |= 1u << event_bit;
        return;
    }

    gNdsFighterDashRunAttackEventScriptMask |= 1u << event_bit;

    for (index = 0u; index < NDS_FTMOTION_SCRIPT_SCAN_WORDS_MAX;)
    {
        u32 word = script[index];
        u32 opcode = (word >> 26) & 0x3fu;

        if ((opcode == NDS_FTMOTION_EVENT_MAKE_ATTACK_COLL) ||
            (opcode == NDS_FTMOTION_EVENT_MAKE_ATTACK_COLL_SCALED))
        {
            if ((index + 4u) < NDS_FTMOTION_SCRIPT_SCAN_WORDS_MAX)
            {
                ndsFighterDashRunDecodeAttackWords(
                    fp, &script[index], event_bit,
                    (opcode == NDS_FTMOTION_EVENT_MAKE_ATTACK_COLL_SCALED));
                decoded_attack = TRUE;
            }
            index += ndsFighterDashRunMotionEventWords(opcode);
            continue;
        }
        if (opcode == NDS_FTMOTION_EVENT_SET_ATTACK_COLL_DAMAGE)
        {
            ndsFighterDashRunSetAttackCollDamage(fp, word);
            index += ndsFighterDashRunMotionEventWords(opcode);
            continue;
        }
        if (opcode == NDS_FTMOTION_EVENT_SET_ATTACK_COLL_SIZE)
        {
            ndsFighterDashRunSetAttackCollSize(fp, word);
            index += ndsFighterDashRunMotionEventWords(opcode);
            continue;
        }
        if (opcode == NDS_FTMOTION_EVENT_CLEAR_ATTACK_COLL_ALL)
        {
            ndsFighterDashRunClearAttackCollAll(fighter_gobj, fp);
            index += ndsFighterDashRunMotionEventWords(opcode);
            continue;
        }
        if ((opcode == NDS_FTMOTION_EVENT_END) ||
            (opcode == NDS_FTMOTION_EVENT_PAUSE_SCRIPT))
        {
            if (decoded_attack == FALSE)
            {
                gNdsFighterDashRunAttackEventNoHitMask |= 1u << event_bit;
            }
            return;
        }
        index += ndsFighterDashRunMotionEventWords(opcode);
    }

    if (decoded_attack == FALSE)
    {
        gNdsFighterDashRunAttackEventNoHitMask |= 1u << event_bit;
    }
}

void ftMainPlayAnimEventsAll(GObj *fighter_gobj)
{
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
                ndsFighterDashRunDecodeAttackEvent(fighter_gobj, fp,
                                                   event_bit);
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

static void ndsFighterMarioFoxRunWaitStatusProbe(GObj *fighter_gobj,
                                                 FTDesc *desc)
{
    u32 i;

    (void)fighter_gobj;
    (void)desc;

    if ((ndsFighterMarioFoxWaitProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxInitResult != NDS_FIGHTER_MARIOFOX_INIT_PASS) ||
        (gNdsFighterMarioFoxInitCount < 2u))
    {
        return;
    }

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *probe_gobj;

        if (ndsFighterStructIsPoolPointer(fp) == FALSE)
        {
            continue;
        }
        probe_gobj = fp->fighter_gobj;
        if (probe_gobj == NULL)
        {
            continue;
        }

        if (fp->is_wait_status_setup == FALSE)
        {
            gNdsFighterWaitOriginalSetStatusCallCount++;
            ftCommonWaitSetStatus(probe_gobj);
            fp->is_special_interrupt = TRUE;
            gNdsFighterMarioFoxWaitCount++;
        }
        else
        {
            ndsFighterMarioFoxRecordInstalledWaitState(fp);
        }
        ndsFighterStructRecord(fp);
        ndsFighterMarioFoxRecordWaitStatus(fp);
    }

    ndsFighterMarioFoxRunWaitCallbackTickProbe();
    ndsFighterMarioFoxRunWaitGroundProof();
    ndsFighterMarioFoxRunDisplayProbe();
    ndsFighterMarioFoxRunDLScanProbe();
    ndsFighterMarioFoxRunDLExecuteProbe();
    ndsFighterMarioFoxRunDLDrawProbe();
    ndsFighterMarioFoxRunDLMultiDrawProbe();
    ndsFighterMarioFoxRunDLAllDrawProbe();
    ndsFighterMarioFoxRunWalkInputProof();
}

void ndsFighterMarioFoxRunImmediateProofChain(void)
{
    u32 i;

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];

        if ((ndsFighterStructIsPoolPointer(fp) != FALSE) &&
            (fp->fighter_gobj != NULL))
        {
            ndsFighterMarioFoxRunWaitStatusProbe(fp->fighter_gobj, NULL);
            break;
        }
    }
}

static void ndsFighterMarioFoxInitStateFromOriginalOrder(
    GObj *fighter_gobj, FTDesc *desc, FTAttributes *attr, DObj *root_dobj)
{
    FTStruct *fp;
    sb32 is_floor;

    if ((fighter_gobj == NULL) || (desc == NULL) || (attr == NULL) ||
        (root_dobj == NULL) ||
        (ndsFighterMarioFoxInitProofEnabled() == FALSE))
    {
        return;
    }

    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        return;
    }

    fp->lr = desc->lr;
    fp->percent_damage = desc->damage;
    fp->shield_health = 55;
    fp->unk_ft_0x38 = 0.0F;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    ftPhysicsStopVelAll(fighter_gobj);

    fp->jumps_used = 0;
    fp->is_reflect = FALSE;
    fp->is_absorb = FALSE;
    fp->is_shield = FALSE;
    fp->is_effect_attach = FALSE;
    fp->is_jostle_ignore = FALSE;
    fp->cliffcatch_wait = 0;
    fp->tics_since_last_z = 0;
    fp->acid_wait = 0;
    fp->twister_wait = 0;
    fp->tarucann_wait = 0;
    fp->damagefloor_wait = 0;

    fp->attack_damage = 0;
    fp->attack_count = 0;
    fp->attack_shield_push = 0;
    fp->shield_damage = 0;
    fp->damage_lag = 0;
    fp->damage_queue = 0;
    fp->damage_angle = 0;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_lr = 0;
    fp->damage_index = 0;
    fp->damage_player_num = 0;
    fp->damage_player = -1;
    fp->damage_object_class = 0;
    fp->damage_object_kind = 0;
    fp->damage_count = 0;
    fp->damage_kind = nFTDamageKindDefault;
    fp->damage_heal = 0;
    fp->damage_joint_id = 0;
    fp->invincible_tics = 0;
    fp->intangible_tics = 0;
    fp->star_invincible_tics = 0;

    fp->hitstatus = nGMHitStatusNormal;
    fp->star_hitstatus = nGMHitStatusNormal;
    fp->special_hitstatus = nGMHitStatusNormal;
    fp->throw_gobj = NULL;
    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->is_catch_or_capture = FALSE;
    fp->item_gobj = NULL;
    fp->reflect_lr = 0;
    fp->absorb_lr = 0;
    fp->reflect_damage = 0;

    fp->attack1_followup_frames = 0.0F;
    fp->attack_knockback = 0.0F;
    fp->attack_rebound = 0.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->knockback_resist_status = 0.0F;
    fp->knockback_resist_passive = 0.0F;
    fp->damage_knockback = 0.0F;
    fp->hitlag_mul = 1.0F;
    fp->shield_heal_wait = 10.0F;
    fp->is_fastfall = FALSE;
    fp->player_num = fp->player;
    fp->public_knockback = 0.0F;
    fp->is_hitstun = FALSE;
    fp->is_use_animlocks = FALSE;
    fp->shuffle_frame_index = 0;
    fp->shuffle_index_max = 0;
    fp->is_use_fogcolor = FALSE;
    fp->is_shuffle_electric = FALSE;
    fp->shuffle_tics = 0;
    fp->motion_attack_id = nFTMotionAttackIDNone;
    fp->motion_count = 0;
    fp->stat_attack_id = nFTStatusAttackIDNone;
    fp->stat_count = 0;
    fp->damage_stat_count = 0;

    root_dobj->translate.vec.f = desc->pos;
    root_dobj->scale.vec.f.x = attr->size;
    root_dobj->scale.vec.f.y = attr->size;
    root_dobj->scale.vec.f.z = attr->size;
    ndsFighterPartsSyncDObj(fp, root_dobj, nFTPartsJointTopN);
    fp->coll_data.p_translate = &root_dobj->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.map_coll = attr->map_coll;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.cliffcatch_coll = attr->cliffcatch_coll;
    fp->coll_data.ignore_line_id = -1;
    fp->coll_data.update_tic = gMPCollisionUpdateTic;
    fp->coll_data.mask_curr = 0;
    fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
    fp->coll_data.is_coll_end = FALSE;

    fp->nds_init_floor_project_attempted = 1u;
    is_floor = mpCollisionCheckProjectFloor(
        &root_dobj->translate.vec.f,
        &fp->coll_data.floor_line_id,
        &fp->coll_data.floor_dist,
        &fp->coll_data.floor_flags,
        &fp->coll_data.floor_angle);
    fp->nds_init_floor_project_result = (is_floor != FALSE) ? 1u : 0u;

    if ((is_floor != FALSE) && (fp->coll_data.floor_dist > -300.0F))
    {
        fp->ga = nMPKineticsGround;
        root_dobj->translate.vec.f.y += fp->coll_data.floor_dist;
        fp->coll_data.floor_dist = 0.0F;
    }
    else
    {
        fp->ga = nMPKineticsAir;
        fp->jumps_used = 1;
        fp->coll_data.floor_line_id = -1;
    }
    fp->coll_data.pos_prev = root_dobj->translate.vec.f;
    if (fp->ga == nMPKineticsGround)
    {
        fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
        fp->coll_data.is_coll_end = FALSE;
    }

    if (fp->fkind == nFTKindMario)
    {
        fp->passive_vars.mario.is_expend_tornado = FALSE;
    }
    else if (fp->fkind == nFTKindFox)
    {
        fp->passive_vars.fox.reserved = 0;
    }

    ftParamClearAttackCollAll(fighter_gobj);
    ndsFighterMarioFoxSeedDamageColls(fp);
    ftParamSetHitStatusPartAll(fighter_gobj, nGMHitStatusNormal);
    ftParamResetFighterColAnim(fighter_gobj);

    fp->nds_init_mask = 0x3fffu;
    gNdsFighterMarioFoxInitCount++;
    ndsFighterStructRecord(fp);
    ndsFighterMarioFoxRecordInitState(fp);
}

static GObj *ndsFighterMarioFoxMakeFighter(FTDesc *desc)
{
    FTAttributes *attr;
    FTCommonPartContainer *commonparts;
    FTCommonPart *commonpart;
    DObjDesc *dobjdesc;
    GObj *fighter_gobj;
    DObj *root_dobj;
    s32 detail_index;

    if ((desc == NULL) ||
        ((desc->fkind != nFTKindMario) && (desc->fkind != nFTKindFox)))
    {
        return NULL;
    }

    gNdsFighterModelLastPlayer = (u32)desc->player;
    gNdsFighterModelLastFKind = (u32)desc->fkind;
    gNdsFighterModelLastStageMask = 1u;
    gNdsFighterModelLastAttrPtr = 0u;
    gNdsFighterModelLastCommonPartsPtr = 0u;
    gNdsFighterModelLastCommonPartPtr = 0u;
    gNdsFighterModelLastDObjDescPtr = 0u;
    gNdsFighterModelLastGObjPtr = 0u;

    ndsFighterMarioFoxSetupFilesKind(desc->fkind);
    gNdsFighterModelLastStageMask |= 1u << 1;
    attr = ndsFighterMarioFoxGetAttributes(desc->fkind);
    gNdsFighterModelLastAttrPtr = (uintptr_t)attr;
    gNdsFighterModelLastStageMask |= 1u << 2;
    if (desc->fkind == nFTKindMario)
    {
        gNdsFighterMarioAttrPtrReady = (attr != NULL) ? 1u : 0u;
        gNdsFighterMarioFoxSetupMask |=
            (attr != NULL) ? NDS_FIGHTER_MARIOFOX_SETUP_MARIO_ATTR : 0u;
    }
    else
    {
        gNdsFighterFoxAttrPtrReady = (attr != NULL) ? 1u : 0u;
        gNdsFighterMarioFoxSetupMask |=
            (attr != NULL) ? NDS_FIGHTER_MARIOFOX_SETUP_FOX_ATTR : 0u;
    }
    if (attr == NULL)
    {
        return NULL;
    }

    commonparts = attr->commonparts_container;
    gNdsFighterModelLastCommonPartsPtr = (uintptr_t)commonparts;
    gNdsFighterModelLastStageMask |= 1u << 3;
    detail_index = (desc->detail >= nFTPartsDetailStart) ?
        (s32)(desc->detail - nFTPartsDetailStart) : 0;
    if (detail_index > 1)
    {
        detail_index = 0;
    }
    commonpart = (commonparts != NULL) ?
        &commonparts->commonparts[detail_index] : NULL;
    gNdsFighterModelLastCommonPartPtr = (uintptr_t)commonpart;
    gNdsFighterModelLastStageMask |= 1u << 4;
    dobjdesc = (commonpart != NULL) ? commonpart->dobjdesc : NULL;
    gNdsFighterModelLastDObjDescPtr = (uintptr_t)dobjdesc;
    gNdsFighterModelLastStageMask |= 1u << 5;
    if (desc->fkind == nFTKindMario)
    {
        gNdsFighterMarioCommonPartsReady =
            (dobjdesc != NULL) ? 1u : 0u;
        gNdsFighterMarioFoxSetupMask |=
            (dobjdesc != NULL) ?
                NDS_FIGHTER_MARIOFOX_SETUP_MARIO_COMMONPART : 0u;
    }
    else
    {
        gNdsFighterFoxCommonPartsReady =
            (dobjdesc != NULL) ? 1u : 0u;
        gNdsFighterMarioFoxSetupMask |=
            (dobjdesc != NULL) ?
                NDS_FIGHTER_MARIOFOX_SETUP_FOX_COMMONPART : 0u;
    }
    if (dobjdesc == NULL)
    {
        return NULL;
    }

    fighter_gobj = gcMakeGObjSPAfter(nGCCommonKindFighter,
                                     NULL,
                                     nGCCommonLinkIDFighter,
                                     GOBJ_PRIORITY_DEFAULT);
    gNdsFighterModelLastGObjPtr = (uintptr_t)fighter_gobj;
    gNdsFighterModelLastStageMask |= 1u << 6;
    if (fighter_gobj == NULL)
    {
        return NULL;
    }

    fighter_gobj->user_data.s = desc->player;
    gcAddGObjDisplay(fighter_gobj,
                     ftDisplayMainProcDisplay,
                     FTDISPLAY_DLLINK_DEFAULT,
                     GOBJ_PRIORITY_DEFAULT,
                     ~0u);
    gcSetupCustomDObjs(fighter_gobj,
                       dobjdesc,
                       NULL,
                       0x4B,
                       nGCMatrixKindNull,
                       nGCMatrixKindNull);
    root_dobj = DObjGetStruct(fighter_gobj);
    if (root_dobj != NULL)
    {
        root_dobj->translate.vec.f = desc->pos;
    }

    gNdsFighterModelRealGObjCount++;
    gNdsFighterModelProcessDeferredCount++;
    gNdsFighterMarioFoxSetupMask |=
        NDS_FIGHTER_MARIOFOX_SETUP_DISPLAY |
        NDS_FIGHTER_MARIOFOX_SETUP_PROCESS_DEFER;
    if (desc->fkind == nFTKindMario)
    {
        gNdsFighterMarioFoxSetupMask |=
            NDS_FIGHTER_MARIOFOX_SETUP_MARIO_GOBJ;
    }
    else
    {
        gNdsFighterMarioFoxSetupMask |=
            NDS_FIGHTER_MARIOFOX_SETUP_FOX_GOBJ;
    }
    if (gNdsFighterModelRealGObjCount >= 2u)
    {
        gNdsFighterMarioFoxGObjResult = NDS_FIGHTER_MARIOFOX_GOBJ_PASS;
    }
    if ((gNdsFighterMarioFoxSetupMask & 0x0ffu) == 0x0ffu)
    {
        gNdsFighterMarioFoxModelResult =
            NDS_FIGHTER_MARIOFOX_MODEL_PASS;
    }

    gNdsSCVSBattleOriginalFighterGObjCount++;
    gNdsSCVSBattleOriginalFighterCreateCount++;
    gNdsSCVSBattleOriginalActivePlayerCount++;
    gNdsSCVSBattleOriginalActivePlayerMask |= 1u << (desc->player & 3);
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
    ndsFighterRecordModelGObj(desc, fighter_gobj, root_dobj);
    if (ndsFighterMarioFoxStructProofEnabled() != FALSE)
    {
        FTStruct *fp = ndsFighterStructInitFromDesc(fighter_gobj, desc, attr,
                                                    root_dobj);

        if ((fp != NULL) && (ndsFighterMarioFoxInitProofEnabled() != FALSE))
        {
            ndsFighterMarioFoxInitStateFromOriginalOrder(
                fighter_gobj, desc, attr, root_dobj);
            if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
            {
                ndsFighterMarioFoxRunWaitStatusProbe(fighter_gobj, desc);
            }
        }
    }
    return fighter_gobj;
}

static void ndsFighterMarioFoxRecordStubFighter(FTDesc *desc,
                                                GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if ((ndsFighterMarioFoxModelProofEnabled() != FALSE) &&
        (desc != NULL) &&
        ((desc->fkind == nFTKindMario) || (desc->fkind == nFTKindFox)))
    {
        gNdsFighterModelStubGObjCount++;
    }
}

void ftDisplayMainProcDisplay(GObj *fighter_gobj)
{
    if ((ndsFighterMarioFoxDisplayProofEnabled() != FALSE) &&
        (sNdsFighterDisplayProbeActive != FALSE))
    {
        ndsFighterMarioFoxRecordDisplayProbe(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxDLAllDrawProofEnabled() != FALSE) &&
        (sNdsFighterDLAllDrawProbeActive != FALSE))
    {
        ndsFighterMarioFoxRecordDLAllDrawFromDisplayCallback(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxPreviewLoopProofEnabled() != FALSE) &&
        (sNdsFighterPreviewLoopDisplayActive != FALSE))
    {
        ndsFighterPreviewLoopRecordDisplayFromCallback(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxGCDrawAllLoopProofEnabled() != FALSE) &&
        (sNdsFighterGCDrawAllLoopDisplayActive != FALSE))
    {
        ndsFighterGCDrawAllLoopRecordDisplayFromCallback(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE)
    {
        gNdsFighterWalkLoopDisplayProbeCount++;
        return;
    }
    if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
    {
        gNdsFighterWaitDisplayProbeCount++;
    }
    if (ndsFighterMarioFoxWaitTickProofEnabled() != FALSE)
    {
        gNdsFighterWaitTickDisplayProbeCount++;
    }
    if (ndsFighterMarioFoxWaitGroundProofEnabled() != FALSE)
    {
        gNdsFighterWaitGroundDisplayProbeCount++;
    }
    if (ndsFighterMarioFoxWalkInputProofEnabled() != FALSE)
    {
        gNdsFighterWalkDisplayProbeCount++;
    }
    /*
     * The Mario/Fox model milestone proves original asset-backed DObj creation.
     * Full fighter display traversal is a later renderer boundary; running it
     * here can escape the bounded setup proof when the menu-chain harness draws.
     */
}

sb32 (*dLBCommonFuncMatrixList[])(void) = { NULL };

f32 dSCSubsysFighterScales[] =
{
    1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F,
    1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F,
    1.0F, 1.0F, 1.0F, 1.0F
};

f32 scSubsysFighterGetLightAngleX(void)
{
    return 0.0F;
}

f32 scSubsysFighterGetLightAngleY(void)
{
    return 0.0F;
}

void scSubsysFighterSetLightParams(f32 light_angle_x, f32 light_angle_y,
                                    u8 r, u8 g, u8 b, u8 a)
{
    (void)light_angle_x;
    (void)light_angle_y;
    (void)r;
    (void)g;
    (void)b;
    (void)a;
}
