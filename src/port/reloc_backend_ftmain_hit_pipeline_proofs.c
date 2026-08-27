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

    /* Same source ensure pair as the Selected damage collide (see the
     * comment above ndsGMCollisionCheckFighterAttackDamageCollideSelected). */
    func_ovl2_800EDE00(damage_coll->joint);
    func_ovl2_800EDE5C(damage_coll->joint);

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
    target_fp->damage_angle = attack_coll->angle;
    target_fp->damage_element = attack_coll->element;
    target_fp->damage_lr = expected_lr;
    target_fp->damage_player_num = hitlog->attacker_player_num;
    ftParamUpdate1PGameDamageStats(target_fp, hitlog->attacker_player,
                                   hitlog->attacker_object_class,
                                   fp->fkind,
                                   fp->stat_flags.halfword & ~0x400u,
                                   fp->stat_count);
    target_fp->damage_joint_id = damage_coll->joint_id;
    target_fp->damage_index = damage_coll->placement;
    target_fp->damage_knockback = knockback;
    target_fp->damage_kind = nFTDamageKindStatus;
    if (target_fp->damage_element == nGMHitElementElectric)
    {
        fp->hitlag_mul = 1.5F;
        target_fp->hitlag_mul = 1.5F;
    }

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
    if (((fp->proc_update == ndsBaseFTCommonReboundWaitProcUpdate) ||
            (fp->proc_update == ftCommonReboundWaitProcUpdate)) &&
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

    if ((mask & NDS_FTMAIN_PROCPARAMS_SELECTED_REQUIRED_MASK) ==
        NDS_FTMAIN_PROCPARAMS_SELECTED_REQUIRED_MASK)
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
