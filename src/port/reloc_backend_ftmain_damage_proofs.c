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

















static void ndsFighterDashRunProcParamsLagStart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == &sNdsFighterStructPool[0]) && (fp->player == 0))
    {
        sNdsFighterDashRunProcParamsLagStartCount++;
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
