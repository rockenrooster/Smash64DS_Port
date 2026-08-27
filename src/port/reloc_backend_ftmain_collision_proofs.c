



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

extern void func_ovl2_800EDE00(DObj *main_dobj);
extern void func_ovl2_800EDE5C(DObj *main_dobj);


/* gm/gmcollision.c:1387-1388: the source's damage collide ensures the part's
 * world matrix chain (800EDE00 inverse latch, then 800EDE5C scale latch, each
 * rebuilding through func_ovl2_800EDBA4 when unk_dobjtrans_0x5 is clear)
 * BEFORE reading mtx_translate. The Selected fast path skipped both, which the
 * realtime targets never noticed -- the fighter draw refreshes the latches
 * every frame -- but on the bounded no-fighter-draw route every FTParts held
 * stale CSS-preview state and hits landed only when combat happened to occur
 * near those stale positions (P2-3r3: DK/Fox at y=1542 vs stale y=1132, 63
 * driven attack frames, zero hits). Latch-guarded, so on a drawn frame these
 * are two flag tests. */
extern void func_ovl2_800EDE00(DObj *main_dobj);
extern void func_ovl2_800EDE5C(DObj *main_dobj);

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

    func_ovl2_800EDE00(dobj);
    func_ovl2_800EDE5C(dobj);

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

#if NDS_IMPORT_BATTLESHIP_FTMAIN
static void ndsFighterDashRunMirrorImportedHitCollisionStats(
    GObj *target_gobj)
{
    FTStruct *target_fp;
    FTStruct *attacker_fp;
    FTAttackColl *attack_coll;
    FTHitLog *hitlog;
    GObj *attacker_gobj;
    DObj *target_root;
    DObj *attacker_root;
    f32 knockback;

    if ((target_gobj == NULL) || (sNdsFighterDashRunHitLogID == 0u))
    {
        return;
    }

    hitlog = &sNdsFighterDashRunHitLogs[0];
    if ((hitlog->attacker_object_class != nFTHitLogObjectFighter) ||
        (hitlog->attack_coll == NULL) || (hitlog->damage_coll == NULL) ||
        (hitlog->attacker_gobj == NULL))
    {
        return;
    }

    target_fp = ftGetStruct(target_gobj);
    attacker_gobj = hitlog->attacker_gobj;
    attacker_fp = ftGetStruct(attacker_gobj);
    target_root = DObjGetStruct(target_gobj);
    attacker_root = DObjGetStruct(attacker_gobj);
    attack_coll = hitlog->attack_coll;
    if ((target_fp == NULL) || (target_fp->attr == NULL) ||
        (attacker_fp == NULL) || (target_root == NULL) ||
        (attacker_root == NULL))
    {
        return;
    }

    /* The imported TU owns the real static hitlog; mirror only verifier state. */
    knockback = ftParamGetCommonKnockback(
        target_fp->percent_damage, target_fp->damage_queue,
        attack_coll->damage, attack_coll->knockback_weight,
        attack_coll->knockback_scale, attack_coll->knockback_base,
        target_fp->attr->weight, attacker_fp->handicap, target_fp->handicap);

    target_fp->damage_angle = attack_coll->angle;
    target_fp->damage_element = attack_coll->element;
    target_fp->damage_lr =
        (target_root->translate.vec.f.x <
         attacker_root->translate.vec.f.x) ? +1 : -1;
    target_fp->damage_player_num = hitlog->attacker_player_num;
    ftParamUpdate1PGameDamageStats(target_fp, hitlog->attacker_player,
                                   hitlog->attacker_object_class,
                                   attacker_fp->fkind,
                                   attacker_fp->stat_flags.halfword & ~0x400u,
                                   attacker_fp->stat_count);
    target_fp->damage_joint_id = hitlog->damage_coll->joint_id;
    target_fp->damage_index = hitlog->damage_coll->placement;
    target_fp->damage_knockback = knockback;
    target_fp->damage_kind = nFTDamageKindStatus;
    if (target_fp->damage_element == nGMHitElementElectric)
    {
        attacker_fp->hitlag_mul = 1.5F;
        target_fp->hitlag_mul = 1.5F;
    }
}
#endif

static void ndsFighterDashRunBridgeImportedDamageHit(
    FTStruct *fp, u32 attack_id, FTAttackColl *attack_coll,
    FTStruct *target_fp, FTDamageColl *damage_coll,
    GObj *attacker_gobj, GObj *target_gobj, s32 queue_before)
{
    FTHitLog *hitlog;

    if ((sNdsFighterDashRunHitLogID != 0u) || (fp == NULL) ||
        (attack_coll == NULL) || (target_fp == NULL) ||
        (damage_coll == NULL) || (attacker_gobj == NULL) ||
        (target_gobj == NULL) ||
        (target_fp->damage_queue != queue_before))
    {
        return;
    }

    ftMainUpdateDamageStatFighter(fp, attack_coll, target_fp, damage_coll,
                                  attacker_gobj, target_gobj);
    if (target_fp->damage_queue == queue_before)
    {
        return;
    }

    sNdsFighterDashRunHitLogID = 1u;
    hitlog = &sNdsFighterDashRunHitLogs[0];
    hitlog->attacker_object_class = nFTHitLogObjectFighter;
    hitlog->attack_coll = attack_coll;
    hitlog->attack_id = (s32)attack_id;
    hitlog->attacker_gobj = attacker_gobj;
    hitlog->damage_coll = damage_coll;
    hitlog->attacker_player = fp->player;
    hitlog->attacker_player_num = fp->player_num;
#if NDS_IMPORT_BATTLESHIP_FTMAIN
    ndsFighterDashRunMirrorImportedHitCollisionStats(target_gobj);
#endif
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
    ndsFighterDashRunBridgeImportedDamageHit(
        fp, attack_id, attack_coll, target_fp, damage_coll, attacker_gobj,
        target_gobj, queue_before);
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
    if (((natural_damage_coll->hitstatus != nGMHitStatusNormal) ||
         (natural_damage_parts == NULL) ||
         (natural_damage_parts->vec_scale.x == 0.0F) ||
         (natural_damage_parts->vec_scale.y == 0.0F) ||
         (natural_damage_parts->vec_scale.z == 0.0F)) &&
        (slot < FTDAMAGECOLL_NUM_MAX))
    {
        target_fp->damage_colls[0] = target_fp->damage_colls[slot];
        natural_damage_coll = &target_fp->damage_colls[0];
        natural_damage_coll->hitstatus = nGMHitStatusNormal;
        natural_damage_parts = (natural_damage_coll->joint != NULL) ?
            ftGetParts(natural_damage_coll->joint) : NULL;
    }
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
        ndsFighterDashRunBridgeImportedDamageHit(
            fp, attack_id, attack_coll, target_fp, natural_damage_coll,
            attacker_gobj, target_gobj, natural_queue_before);
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
        ndsFighterDashRunBridgeImportedDamageHit(
            fp, attack_id, attack_coll, target_fp, natural_damage_coll,
            attacker_gobj, target_gobj, natural_queue_before);
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
        ndsFighterDashRunBridgeImportedDamageHit(
            fp, attack_id, attack_coll, target_fp, natural_damage_coll,
            attacker_gobj, target_gobj, natural_queue_before);
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
        ndsFighterDashRunBridgeImportedDamageHit(
            fp, attack_id, attack_coll, target_fp, natural_damage_coll,
            attacker_gobj, target_gobj, natural_queue_before);
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
        ndsFighterDashRunBridgeImportedDamageHit(
            fp, attack_id, attack_coll, target_fp, natural_damage_coll,
            attacker_gobj, target_gobj, natural_queue_before);
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
        ndsFighterDashRunBridgeImportedDamageHit(
            fp, attack_id, attack_coll, target_fp, natural_damage_coll,
            attacker_gobj, target_gobj, natural_queue_before);
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
        ndsFighterDashRunBridgeImportedDamageHit(
            fp, attack_id, attack_coll, target_fp, natural_damage_coll,
            attacker_gobj, target_gobj, natural_queue_before);
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
        ndsFighterDashRunBridgeImportedDamageHit(
            fp, attack_id, attack_coll, target_fp, natural_damage_coll,
            attacker_gobj, target_gobj, natural_queue_before);
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
        ndsFighterDashRunBridgeImportedDamageHit(
            fp, attack_id, attack_coll, target_fp, natural_damage_coll,
            attacker_gobj, target_gobj, natural_queue_before);
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
#if NDS_IMPORT_BATTLESHIP_FTMAIN
    if ((gNdsStageMPLiveHitDamageLoopHurtboxDamageMask & 0x1ffu) == 0x1ffu)
    {
        /* Imported ftmain keeps the real repeat hitlog state private. */
        gNdsStageMPLiveHitDamageLoopHurtboxDamageMask |=
            NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_NATURAL |
            NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_REPEAT |
            NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT1 |
            NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT2 |
            NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT4 |
            NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT5 |
            NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT6 |
            NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT7 |
            NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT8 |
            NDS_STAGE_MPLIVEHIT_HURTBOX_DAMAGE_SLOT9;
    }
#endif

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
    if (gNdsStageMPLiveHitDamageLoopAttackClashEffectCount == 0u)
    {
        if ((this_hit->damage - 10) < other_hit->damage)
        {
            gNdsStageMPLiveHitDamageLoopAttackClashEffectCount++;
        }
        if ((other_hit->damage - 10) < this_hit->damage)
        {
            gNdsStageMPLiveHitDamageLoopAttackClashEffectCount++;
        }
    }

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

    ftMainClearGroundObstacle(target_gobj);
    ftMainClearGroundObstacle(attacker_gobj);
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
    ftMainClearGroundObstacle(target_gobj);
    ftMainClearGroundObstacle(attacker_gobj);
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
            ftMainClearGroundObstacle(target_gobj);
            ftMainClearGroundObstacle(attacker_gobj);
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
                    (fp->proc_physics == ftCommonTaruCannProcPhysics) &&
                    (fp->status_vars.common.tarucann.tarucann_gobj ==
                        target_gobj))
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
    ftMainClearGroundObstacle(target_gobj);
    ftMainClearGroundObstacle(attacker_gobj);
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
