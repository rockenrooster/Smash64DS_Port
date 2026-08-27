
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


static s32 ndsFighterDashRunGetCapturedDamage(FTStruct *fp, s32 damage)
{
    return ftParamGetCapturedDamage(fp, damage);
}

static sb32 ndsFighterDashRunCheckGetUpdateDamageNormal(FTStruct *fp,
                                                        s32 *damage)
{
    return ftMainCheckGetUpdateDamage(fp, damage);
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
