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
        (fp->attack_colls[0].damage == 7))
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
    extern GMColAnim sIFScreenFlashColAnim;
    sb32 saved_status_setup_active =
        sNdsFighterDashRunDamageStatusSetupActive;
    GMColAnim saved_colanim = sIFScreenFlashColAnim;
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
    sIFScreenFlashColAnim = saved_colanim;
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

        if ((ndsFighterStructUsedMask() & (1u << 1)) != 0u)
        {
            attacker_fp = ndsFighterMarioFoxProofStructForSlot(1);
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
#if NDS_IMPORT_BATTLESHIP_FTMAIN
    sNdsStageMPLiveHitDamageLoopShieldStatProofActive = TRUE;
#endif
    ftMainSearchHitFighter(victim_gobj);
#if NDS_IMPORT_BATTLESHIP_FTMAIN
    sNdsStageMPLiveHitDamageLoopShieldStatProofActive = FALSE;
#endif
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
#if NDS_IMPORT_BATTLESHIP_FTMAIN
    sNdsStageMPLiveHitDamageLoopShieldStatProofActive = TRUE;
#endif
    ftMainSearchHitFighter(victim_gobj);
#if NDS_IMPORT_BATTLESHIP_FTMAIN
    sNdsStageMPLiveHitDamageLoopShieldStatProofActive = FALSE;
#endif
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
    if ((victim_fp->motion_id == nFTCommonMotionGuardOn) ||
        (victim_fp->motion_id == nFTCommonMotionNull))
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
