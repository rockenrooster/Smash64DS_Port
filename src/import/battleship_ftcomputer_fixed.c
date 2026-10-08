/* CPU target search in fixed point (P2-2p8, 2026-10-05; owner: "Software
 * floating point should not exist, fixed point only"; ruling D13 re-baselines
 * the digest).
 *
 * ft/ftcomputer.c's ftComputerCheckFindTarget and ftComputerCheckEvadeDistance
 * run for every CPU against every opponent each tick: squared distances in
 * soft float, a sqrtf each, and the stage-bounds tests as float compares --
 * 2.4K ticks a frame on the gate roster in the float census
 * (artifacts/performance/2026-10-05_float-census). battleship_ftcomputer.c
 * makes the decomp definitions weak; these are the strong ones. Positions go
 * to Q12 once per use (saturated at 2^16 units, far past every blast zone),
 * squared distances are exact 64-bit integers, the bounds tests are the
 * port's exact bit-pattern compares (nds_fcmp.h), and the one square root the
 * result keeps (target_dist) comes from the hardware unit. Every branch and
 * every write is the source's, in the source's order. */
#include <ft/ftcomputer.h>
#include <ft/fighter.h>
#include <wp/weapon.h>
#include <it/item.h>
#include <gr/ground.h>
#include <sc/scene.h>
#include <stdint.h>

#include <nds/nds_fcmp.h>
#include <nds/nds_r2_collision_mtx.h>
#include <nds/nds_r2_hwmath_unit.h>
#include <nds/nds_p4.h>

extern sb32 func_ovl2_800F8FFC(Vec3f *position);
extern FTComputerAttack *dFTComputerAttackList[];
void ftComputerSetCommandWaitShort(FTStruct *fp, s32 index);
void ftComputerSetCommandImmediate(FTStruct *fp, s32 index);

#define NDS_COM_Q12_LIMIT (INT32_C(1) << 28)

/* A coordinate at Q12, saturated at +-2^16 units. One ARM copy: inlined at
 * its six uses it grew ftComputerCheckEvadeDistance from 236 to 1,240 B. */
static int32_t __attribute__((noinline, target("arm"))) ndsComQ12(f32 value)
{
    int32_t q = ndsR2CollisionF32ToFixed(value, 12u);

    if ((q == NDS_R2_COLLISION_F32_OVERFLOW) || (q >= NDS_COM_Q12_LIMIT) ||
        (q <= -NDS_COM_Q12_LIMIT))
    {
        return ((ndsFcmpBits(value) & 0x80000000u) != 0u) ?
            -NDS_COM_Q12_LIMIT : NDS_COM_Q12_LIMIT;
    }
    return q;
}

/* r units, squared, at Q24. */
#define NDS_COM_RANGE_SQ_Q24(r) ((uint64_t)((r) * 4096) * (uint64_t)((r) * 4096))

/* ftcomputer.c:3716. ARM state (2026-10-06): Thumb has no CLZ (__clzdi2
 * calls). */
sb32 __attribute__((target("arm")))
ftComputerCheckFindTarget(FTStruct *this_fp)
{
    FTComputer *com = &this_fp->computer;
    FTStruct *other_fp;
    const int32_t this_x =
        ndsComQ12(this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x);
    const int32_t this_y =
        ndsComQ12(this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y);
    GObj *other_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    uint64_t distance = UINT64_MAX;
    f32 other_pos_x;
    f32 other_pos_y;

    while (other_gobj != NULL)
    {
        if (other_gobj != this_fp->fighter_gobj)
        {
            other_fp = ftGetStruct(other_gobj);

            if (this_fp->team != other_fp->team)
            {
                other_pos_x =
                    other_fp->joints[nFTPartsJointTopN]->translate.vec.f.x;
                other_pos_y =
                    other_fp->joints[nFTPartsJointTopN]->translate.vec.f.y;

                if ((other_fp->status_id >= nFTCommonStatusWait) &&
                    (((func_ovl2_800F8FFC(
                           &other_fp->joints[nFTPartsJointTopN]->translate.vec.f) !=
                       FALSE) &&
                      NDS_FCMP_LE(other_pos_x,
                                  gMPCollisionBounds.current.right) &&
                      NDS_FCMP_GE(other_pos_x,
                                  gMPCollisionBounds.current.left) &&
                      NDS_FCMP_GE(other_pos_y,
                                  gMPCollisionBounds.current.bottom) &&
                      NDS_FCMP_LT(other_pos_y,
                                  gMPCollisionGroundData->camera_bound_top)) ||
                     ((this_fp->ga == nMPKineticsGround) &&
                      ((other_fp->status_id == nFTCommonStatusCliffCatch) ||
                       (other_fp->status_id == nFTCommonStatusCliffWait)))) &&
                    ((this_fp->fkind != nFTKindMMario) ||
                     (other_fp->ga == nMPKineticsGround)))
                {
                    const int64_t dx = (int64_t)this_x - ndsComQ12(other_pos_x);
                    const int64_t dy = (int64_t)this_y - ndsComQ12(other_pos_y);
                    const uint64_t square_xy =
                        (uint64_t)(dx * dx) + (uint64_t)(dy * dy);

                    if (square_xy < distance)
                    {
                        com->target_pos.x = other_pos_x;
                        com->target_pos.y = other_pos_y;
                        com->target_user = other_fp;

                        distance = square_xy;
                    }
                }
            }
        }
        other_gobj = other_gobj->link_next;
    }

    if (distance == UINT64_MAX)
    {
        com->target_line_id = -1;
        com->target_dist = F32_MAX;
        com->ftcom_flags_0x4A_b1 = FALSE;

        return FALSE;
    }
    com->ftcom_flags_0x4A_b1 = TRUE;
    com->target_dist = ndsR2CollisionFixedToF32(
        (int64_t)ndsR2HwMathSqrt64Fast(distance), 12u);

    if (ftGetComTargetFighter(com)->ga == nMPKineticsGround)
    {
        com->target_line_id = ftGetComTargetFighter(com)->coll_data.floor_line_id;
    }
    else com->target_line_id = -1;

    return TRUE;
}

/* ftcomputer.c:3796. The source leads the opponent by three ticks of
 * vel_air.x on BOTH axes (it reads .x for y too); kept. Its sqrt compared
 * against 1500 and 2500 is the squared distance against their squares. ARM
 * state: the squares were __aeabi_lmul calls in Thumb. */
sb32 __attribute__((target("arm")))
ftComputerCheckEvadeDistance(FTStruct *this_fp)
{
    GObj *other_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

    while (other_gobj != NULL)
    {
        if (other_gobj != this_fp->fighter_gobj)
        {
            FTStruct *other_fp = ftGetStruct(other_gobj);

            if (this_fp->team != other_fp->team)
            {
                DObj *other_joint = other_fp->joints[nFTPartsJointTopN];
                DObj *this_joint = this_fp->joints[nFTPartsJointTopN];
                const int64_t lead =
                    3 * (int64_t)ndsComQ12(other_fp->physics.vel_air.x);
                const int64_t dx =
                    (int64_t)ndsComQ12(this_joint->translate.vec.f.x) -
                    ((int64_t)ndsComQ12(other_joint->translate.vec.f.x) + lead);
                const int64_t dy =
                    (int64_t)ndsComQ12(this_joint->translate.vec.f.y) -
                    ((int64_t)ndsComQ12(other_joint->translate.vec.f.y) + lead);
                const uint64_t square_xy =
                    (uint64_t)(dx * dx) + (uint64_t)(dy * dy);

                if ((other_fp->star_hitstatus == nGMHitStatusInvincible) &&
                    (square_xy < NDS_COM_RANGE_SQ_Q24(1500)))
                {
                    return TRUE;
                }
                else if ((other_fp->item_gobj != NULL) &&
                         (itGetStruct(other_fp->item_gobj)->kind == nITKindHammer) &&
                         (square_xy < NDS_COM_RANGE_SQ_Q24(2500)))
                {
                    return TRUE;
                }
            }
        }
        other_gobj = other_gobj->link_next;
    }
    return FALSE;
}

/* ftcomputer.c:3896, the attack pick, with its prediction at Q12. Per attack
 * the source projects both fighters hit_start_frame ticks ahead -- ~30 soft-
 * float operations an attack, ~150 a frame on every roster in the float census
 * -- and compares the offset with the attack's detect box widened by the
 * target's hurtbox. Here the inputs enter at Q12 once, the two apex frames are
 * computed once (the source recomputes them per attack from the same inputs),
 * and the projection is computed where the source compares it (it has no
 * side effects). The detect boxes stay floats until that compare: their item
 * and giant scalings and the cliff-ledge widening are the source's float
 * code, and the cliff-catch walk reads them as floats. Every branch, write and
 * random draw is the source's, in its order. */
/* ARM state (2026-10-06): its int64 branches called __aeabi_lmul in Thumb. */
sb32 __attribute__((target("arm")))
ftComputerCheckDetectTarget(FTStruct *this_fp, f32 detect_range_base)
{
    // This was a wildddddddddd match...
    FTComputer *com = &this_fp->computer;
    FTStruct *target_fp = ftGetComTargetFighter(com);
    /* Q12. Positions and sums are 64-bit (adds only); the factors are
     * 32-bit so every per-attack product is one Thumb MUL (a 64-bit product
     * is a libgcc call there). Bounds: |vel| < 2^23, frames < 2^7, gravity
     * < 2^15 at Q12. */
    int64_t this_pos_x = 0;
    int64_t this_pos_y = 0;
    int32_t this_vel_x = 0;
    int32_t this_vel_y = 0;
    int32_t this_tvel_base = 0;
    int32_t this_gravity = 0;
    int64_t target_pos_x = 0;
    int64_t target_pos_y = 0;
    int32_t target_vel_x = 0;
    int32_t target_vel_y = 0;
    int32_t target_tvel_base = 0;
    int32_t target_gravity = 0;
    void *user_data; // Originally "reflect_fp" but need to rename it appropriately for the silly hack in the item kind check
    int64_t hurtbox_detect_width = 0;
    int64_t hurtbox_detect_height = 0;
    f32 hurtbox_detect_width_f;
    f32 hurtbox_detect_height_f;
    sb32 inputs_q12;
    int64_t predict_pos_x;
    int64_t predict_pos_y;
    f32 damage_coll_size_mul;
    f32 detect_far_x;
    f32 detect_near_x;
    f32 detect_near_y;
    s32 input_kinds[20];
    f32 detect_ranges_x[20];
    s32 hit_frame;
    s32 i;
    sb32 is_attempt_cliffcatch;     // Might be very much incorrectly nanmed
    s32 target_predict_frame = 0;
    s32 this_predict_frame = 0;
    s32 attack_count;
    f32 detect_far_y;
    FTComputerAttack *comattack;
    Vec3f this_detect_pos;
    int64_t predict_adjust_y;
    s32 fkind;
    sb32 target_falls = FALSE;
#if NDS_P4
    const NDSP4Computer *p4_com;
#endif

    if (gSCManagerBattleState->gkind == nGRKindInishie)
    {
        if ((this_fp->coll_data.floor_line_id >= 0) && (mpCollisionCheckExistPlatformLineID(this_fp->coll_data.floor_line_id) != FALSE))
        {
            return FALSE;
        }
    }
    if ((gSCManagerBattleState->gkind == nGRKindYamabuki) && (this_fp->ga != nMPKineticsGround))
    {
        if (this_fp->physics.vel_air.x > 0.0F)
        {
            detect_near_x = this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x;
            detect_far_x = this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x + (this_fp->physics.vel_air.x * 40.0F);
        }
        else
        {
            detect_far_x = this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x;
            detect_near_x = this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x + (this_fp->physics.vel_air.x * 40.0F);
        }
        this_detect_pos.y = this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y;
        this_detect_pos.x = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x + detect_near_x) - 100.0F;

        while (this_detect_pos.x < ((this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x + detect_far_x) + 100.0F))
        {
            if (func_ovl2_800F8FFC(&this_detect_pos) == FALSE)
            {
                return FALSE;
            }
            this_detect_pos.x += 100.0F;
        }
    }
    damage_coll_size_mul = (((syUtilsRandFloat() - 0.5F) * (FTCOMPUTER_LEVEL_MAX - this_fp->level)) * 0.1F) + 1.0F;

    hurtbox_detect_width_f = target_fp->damage_coll_size.x * damage_coll_size_mul;
    hurtbox_detect_height_f = target_fp->damage_coll_size.y * damage_coll_size_mul;
    /* The Q12 inputs are taken at the first attack that reaches the
     * prediction (inputs_q12, below): most calls skip every attack first. */
    inputs_q12 = FALSE;

#if NDS_P4
    /* P4: the content's attack list (Remix ai_behaviour). */
    p4_com = ndsP4Computer(this_fp);
    comattack = ((p4_com != NULL) && (p4_com->attacks != NULL)) ?
        (FTComputerAttack *)p4_com->attacks : dFTComputerAttackList[this_fp->fkind];
#else
    comattack = dFTComputerAttackList[this_fp->fkind];
#endif

    if (this_fp->ga != nMPKineticsGround)
    {
        while (comattack->input_kind != -1)
        {
            comattack++;
        }
        comattack++;
    }
    for (attack_count = 0; TRUE; comattack++) // Does not match as while (TRUE) loop
    {
        if (comattack->input_kind == -1)
        {
            break;
        }
        else if (comattack->hit_start_frame == 0)
        {
            continue;
        }
        else
        {
            hit_frame = comattack->hit_start_frame;
            is_attempt_cliffcatch = FALSE;

            if (this_fp->lr > 0)
            {
                detect_near_x = comattack->detect_near_x;
                detect_far_x = comattack->detect_far_x;
            }
            else
            {
                detect_near_x = -comattack->detect_far_x;
                detect_far_x = -comattack->detect_near_x;
            }
            detect_near_y = comattack->detect_near_y;
            detect_far_y = comattack->detect_far_y;

            if (this_fp->fkind == nFTKindGDonkey)
            {
                detect_near_x *= 1.4F;
                detect_far_x *= 1.4F;
                detect_near_y *= 1.4F;
                detect_far_y *= 1.4F;
            }
            if (this_fp->ga == nMPKineticsGround)
            {
                switch (comattack->input_kind)
                {
                case nFTComputerInputStickNButtonA:
                case nFTComputerInputStickTiltAutoXButtonA:
                case nFTComputerInputStickSmashAutoXNYButtonA:
                    if (this_fp->item_gobj != NULL)
                    {
                        user_data = itGetStruct(this_fp->item_gobj);

                        if (user_data != NULL)
                        {
                            // B R U H
                            if
                            (
                                (((ITStruct*)user_data)->kind == nITKindSword)  ||
                                (((ITStruct*)user_data)->kind == nITKindBat)    ||
                                (((ITStruct*)user_data)->kind == nITKindStarRod)
                            )
                            {
                                detect_near_x *= 1.3F;
                                detect_far_x *= 1.3F;
                            }
                        }
                    }
                    break;

                default:
                    break;
                }
            }
#if NDS_P4
            /* P4: the content's guard replaces the parent's switch (Remix
             * ai_attack_prevent, the jump table at 0x801334E4). */
            if ((p4_com != NULL) && (p4_com->prevent != NULL))
            {
                s32 rule = p4_com->prevent(this_fp, comattack->input_kind);

                if (rule == NDS_P4_COMPUTER_SKIP)
                {
                    goto l_continue;
                }
                is_attempt_cliffcatch = (rule == NDS_P4_COMPUTER_CHECK_GROUND);
            }
            else
#endif
            switch (this_fp->fkind)
            {
            case nFTKindMario:
            case nFTKindLuigi:
            case nFTKindMMario:
            case nFTKindNMario:
            case nFTKindNLuigi:
                if (comattack->input_kind == nFTComputerInputStickSmashHiButtonB)
                {
                    is_attempt_cliffcatch = TRUE;
                }
                break;

            case nFTKindKirby:
            case nFTKindNKirby:
                if (comattack->input_kind == nFTComputerInputStickSmashHiButtonB)
                {
                    is_attempt_cliffcatch = TRUE;
                }
                /* fallthrough */
            case nFTKindYoshi:
            case nFTKindCaptain:
            case nFTKindNYoshi:
            case nFTKindNCaptain:
                if (comattack->input_kind == nFTComputerInputStickSmashLwButtonB)
                {
                    is_attempt_cliffcatch = TRUE;
                }
                break;

            case nFTKindBoss:
            case nFTKindNFox:
            case nFTKindNDonkey:
            case nFTKindNSamus:
            case nFTKindNLink:
            case nFTKindNPikachu:
            case nFTKindNNess:
            case nFTKindGDonkey:
                break;
            }
            if (is_attempt_cliffcatch != FALSE)
            {
                this_detect_pos.y = this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y;
                this_detect_pos.x = (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x + detect_near_x) - 100.0F;

                while (this_detect_pos.x < ((this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x + detect_far_x) + 100.0F))
                {
                    if (func_ovl2_800F8FFC(&this_detect_pos) == FALSE)
                    {
                        goto l_continue;
                    }
                    this_detect_pos.x += 100.0F;
                }
                if (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x < com->target_pos.x)
                {
                    if (com->cliff_right_pos.x < (com->target_pos.x + 1200.0F))
                    {
                        goto l_continue;
                    }
                }
                else if (com->cliff_left_pos.x > (com->target_pos.x - 1200.0F))
                {
                    goto l_continue;
                }
            }
            if (this_fp->fkind == nFTKindGDonkey)
            {
                if
                (
                    (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x < (gMPCollisionBounds.current.left + 500.0F)) ||
                    (this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x > (gMPCollisionBounds.current.right - 500.0F))
                )
                {
                    switch (comattack->input_kind)
                    {
                    case nFTComputerInputStickSmashHiButtonB:
                        goto l_continue;

                    case nFTComputerInputStickSmashAutoXButtonB:
                        if ((this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x * this_fp->lr) > 0.0F)
                        {
                            goto l_continue;
                        }
                        break;

                    default:
                        if (this_fp->ga != nMPKineticsGround)
                        {
                            goto l_continue;
                        }
                        break;
                    }
                }
            }
            if ((this_fp->fkind != nFTKindGDonkey) || (this_fp->ga == nMPKineticsGround) || (comattack->input_kind != nFTComputerInputStickSmashHiButtonB))
            {
                if (com->ftcom_flags_0x4A_b1)
                {
                    if
                    (
                        (ftGetComTargetFighter(com)->status_id == nFTCommonStatusCliffCatch) ||
                        (ftGetComTargetFighter(com)->status_id == nFTCommonStatusCliffWait)
                    )
                    {
                        if (detect_near_y < 0.0F)
                        {
                            detect_near_y -= 500.0F;
                        }
                    }
                }
                /* The source's prediction, at Q12 (see above for why here),
                 * on inputs read once a call (they do not change in it). */
                if (inputs_q12 == FALSE)
                {
                    inputs_q12 = TRUE;
                    this_pos_x = ndsComQ12(this_fp->joints[nFTPartsJointTopN]->translate.vec.f.x);
                    this_pos_y = ndsComQ12(this_fp->joints[nFTPartsJointTopN]->translate.vec.f.y);
                    hurtbox_detect_width = ndsComQ12(hurtbox_detect_width_f);
                    hurtbox_detect_height = ndsComQ12(hurtbox_detect_height_f);
                    target_pos_x = ndsComQ12(target_fp->joints[nFTPartsJointTopN]->translate.vec.f.x);
                    target_pos_y = ndsComQ12(target_fp->joints[nFTPartsJointTopN]->translate.vec.f.y);
                    target_vel_x = ndsComQ12(target_fp->physics.vel_air.x);
                    target_vel_y = ndsComQ12(target_fp->physics.vel_air.y);
                    target_tvel_base = -ndsComQ12(target_fp->attr->tvel_base);
                    target_gravity = ndsComQ12(target_fp->attr->gravity);
                    this_vel_x = ndsComQ12(this_fp->physics.vel_air.x);
                    this_vel_y = ndsComQ12(this_fp->physics.vel_air.y);
                    this_tvel_base = -ndsComQ12(this_fp->attr->tvel_base);
                    this_gravity = ndsComQ12(this_fp->attr->gravity);
                    /* The two apex frames, recomputed per attack by the
                     * source from these same inputs: the quotient truncated
                     * toward zero, as its float -> s32 assignment does. A
                     * zero gravity (no fighter has one) leaves 0. */
                    this_predict_frame = (this_gravity != 0) ?
                        (s32)((int32_t)(this_pos_y - this_tvel_base) / this_gravity) : 0;
                    target_falls = ((target_fp->status_id != nFTCommonStatusPass) &&
                                    (target_fp->ga != nMPKineticsGround)) ? TRUE : FALSE;
                    target_predict_frame = ((target_falls != FALSE) && (target_gravity != 0)) ?
                        (s32)((target_vel_y - target_tvel_base) / target_gravity) : 0;
                }
                predict_pos_x = (target_pos_x + (target_vel_x * hit_frame)) -
                                (this_pos_x + (this_vel_x * hit_frame));
                /* `* lr`, lr being -1, 0 or +1. */
                if (this_fp->lr < 0)
                {
                    predict_pos_x = -predict_pos_x;
                }
                else if (this_fp->lr == 0)
                {
                    predict_pos_x = 0;
                }

                if ((this_fp->status_id == nFTCommonStatusPass) || (this_predict_frame <= 0))
                {
                    predict_adjust_y = (this_vel_y * hit_frame) + this_pos_y;
                }
                else if (hit_frame < this_predict_frame)
                {
                    predict_adjust_y = ((this_vel_y * hit_frame) -
                                        ((this_gravity * SQUARE(hit_frame)) / 2)) +
                                       this_pos_y;
                }
                else predict_adjust_y = (((int64_t)(hit_frame * this_vel_y) -
                                          (((int64_t)this_gravity * SQUARE((int64_t)this_predict_frame)) / 2)) +
                                         ((int64_t)this_tvel_base * (hit_frame - this_predict_frame))) +
                                        this_pos_y;

                if (target_falls != FALSE)
                {
                    if (target_predict_frame <= 0)
                    {
                        predict_pos_y = ((hit_frame * target_vel_y) + target_pos_y) - predict_adjust_y;
                    }
                    else if (hit_frame < target_predict_frame)
                    {
                        predict_pos_y = (((hit_frame * target_vel_y) + target_pos_y) -
                                         ((target_gravity * SQUARE(hit_frame)) / 2)) -
                                        predict_adjust_y;
                    }
                    else predict_pos_y = ((((hit_frame * target_vel_y) + target_pos_y) -
                                           (((int64_t)target_gravity * SQUARE((int64_t)target_predict_frame)) / 2)) +
                                          ((int64_t)target_tvel_base * (hit_frame - target_predict_frame))) -
                                         predict_adjust_y;
                }
                else predict_pos_y = ((hit_frame * target_vel_y) + target_pos_y) - predict_adjust_y;

                if
                (
                    (predict_pos_y < (int64_t)ndsComQ12(detect_far_y)) &&
                    (((int64_t)ndsComQ12(detect_near_y) - hurtbox_detect_height) < predict_pos_y) &&
                    (((int64_t)ndsComQ12(detect_near_x) - hurtbox_detect_width) < predict_pos_x) &&
                    (predict_pos_x < ((int64_t)ndsComQ12(detect_far_x) + hurtbox_detect_width))
                )
                {
                    input_kinds[attack_count] = comattack->input_kind;

                    switch (comattack->input_kind)
                    {
                    case nFTComputerInputStickSmashAutoXNYButtonA:
                        if (this_fp->item_gobj != NULL)
                        {
                            if (itGetStruct(this_fp->item_gobj) != NULL)
                            {
                                if (itGetStruct(this_fp->item_gobj)->type == nITTypeSwing)
                                {
                                    detect_range_base = -0.8F;
                                }
                            }
                            if (itGetStruct(this_fp->item_gobj) != NULL)
                            {
                                if (itGetStruct(this_fp->item_gobj)->kind == nITKindBat)
                                {
                                    if (this_fp->ga == nMPKineticsGround)
                                    {
                                        detect_ranges_x[attack_count++] = 4.0F;
                                        break;
                                    }
                                }
                            }
                        }
                        detect_ranges_x[attack_count++] = 1.0F;
                        break;

                    case nFTComputerInputStickSmashAutoXButtonB:
                        user_data = com->target_user;

                        if (this_fp->level >= 5)
                        {
                            if (user_data != NULL)
                            {
                                if ((((FTStruct*)user_data)->fkind == nFTKindNess) || (((FTStruct*)user_data)->fkind == nFTKindFox))
                                {
                                    fkind = (this_fp->fkind == nFTKindKirby) ? this_fp->passive_vars.kirby.copy_id : this_fp->fkind;

                                    switch (fkind)
                                    {
                                    case nFTKindMario:
                                    case nFTKindFox:
                                    case nFTKindSamus:
                                    case nFTKindLuigi:
                                    case nFTKindLink:
                                    case nFTKindPikachu:
                                    case nFTKindMMario:
                                        goto l_continue;
                                    }
                                }
                            }
                        }
                        switch (this_fp->fkind)
                        {
                        case nFTKindDonkey:
                        case nFTKindGDonkey:
                            if (this_fp->passive_vars.donkey.charge_level == FTDONKEY_GIANTPUNCH_CHARGE_MAX)
                            {
                                detect_ranges_x[attack_count++] = 4.0F;
                            }
                            else detect_ranges_x[attack_count++] = 1.0F + detect_range_base;
                            break;

                        case nFTKindSamus:
                            if (this_fp->passive_vars.samus.charge_level == FTSAMUS_CHARGE_MAX)
                            {
                                detect_ranges_x[attack_count++] = 4.0F;
                            }
                            else detect_ranges_x[attack_count++] = 1.0F + detect_range_base;
                            break;

                        case nFTKindKirby:
                            switch (this_fp->passive_vars.kirby.copy_id)
                            {
                            case nFTKindDonkey:
                                if (this_fp->passive_vars.kirby.copydonkey_charge_level == FTKIRBY_COPYDONKEY_GIANTPUNCH_CHARGE_MAX)
                                {
                                    detect_ranges_x[attack_count++] = 4.0F;
                                }
                                else detect_ranges_x[attack_count++] = 1.0F + detect_range_base;
                                break;

                            case nFTKindSamus:
                                if (this_fp->passive_vars.kirby.copysamus_charge_level == FTKIRBY_COPYSAMUS_CHARGE_MAX)
                                {
                                    detect_ranges_x[attack_count++] = 4.0F;
                                }
                                else detect_ranges_x[attack_count++] = 1.0F + detect_range_base;
                                break;

                            default:
                                detect_ranges_x[attack_count++] = 4.0F;
                                break;
                            }
                            break;

                        default:
                            detect_ranges_x[attack_count++] = 1.0F + detect_range_base;
                            break;
                        }
                        break;

                    case nFTComputerInputStickSmashHiButtonB:
                    case nFTComputerInputStickSmashLwButtonB:
                        detect_ranges_x[attack_count++] = (detect_range_base * 0.5F) + 1.0F;
                        break;

                    case nFTComputerInputStickNButtonZButtonA:
                        if ((this_fp->fkind != nFTKindLink) && (this_fp->fkind != nFTKindSamus))
                        {
                            detect_ranges_x[attack_count++] = 4.0F;
                            break;
                        }
                        /* fallthrough */
                    default:
                        detect_ranges_x[attack_count++] = 1.0F;
                        break;
                    }
                }
            }
        }
    l_continue:
        continue;
    }
    if (attack_count != 0)
    {
        if (com->trait == nFTComputerTraitLink)
        {
            if (this_fp->percent_damage < ((syUtilsRandFloat() * 100.0F) + 1.0F))
            {
                com->fighter_follow_since = 0;
                return FALSE;
            }
        }
        if (this_fp->level < 3)
        {
            if ((this_fp->percent_damage + 5.0F) < (syUtilsRandFloat() * (200.0F - (this_fp->level * 50.0F))))
            {
                com->fighter_follow_since = 0;
                return FALSE;
            }
        }
        detect_far_x = 0.0F;

        for (i = 0; i < attack_count; i++)
        {
            if (com->input_kind == input_kinds[i])
            {
                detect_ranges_x[i] *= 0.25F;
            }
            switch (input_kinds[i])
            {
            case nFTComputerInputStickNButtonA:
                detect_ranges_x[i] += com->stickn_button_a_count * 0.2F;
                com->stickn_button_a_count++;
                break;

            case nFTComputerInputStickTiltAutoXButtonA:
                detect_ranges_x[i] += com->sticktilts_button_a_count * 0.2F;
                com->sticktilts_button_a_count++;
                break;

            case nFTComputerInputStickSmashAutoXNYButtonA:
                detect_ranges_x[i] += com->sticksmashs_button_a_count * 0.2F;
                com->sticksmashs_button_a_count++;
                break;

            case nFTComputerInputStickTiltHiButtonA:
                detect_ranges_x[i] += com->sticktilthi_button_a_count * 0.2F;
                com->sticktilthi_button_a_count++;
                break;

            case nFTComputerInputStickSmashHiButtonA:
                detect_ranges_x[i] += com->sticksmashhi_button_a_count * 0.2F;
                com->sticksmashhi_button_a_count++;
                break;

            case nFTComputerInputStickTiltLwButtonA:
                detect_ranges_x[i] += com->sticktiltlw_button_a_count * 0.2F;
                com->sticktiltlw_button_a_count++;
                break;

            case nFTComputerInputStickSmashLwButtonA:
                detect_ranges_x[i] += com->sticksmashlw_button_a_count * 0.2F;
                com->sticksmashlw_button_a_count++;
                break;

            case nFTComputerInputStickSmashAutoXButtonB:
                detect_ranges_x[i] += com->sticksmashs_button_b_count * 0.2F;
                com->sticksmashs_button_b_count++;
                break;

            case nFTComputerInputStickSmashHiButtonB:
                detect_ranges_x[i] += com->sticksmashhi_button_b_count * 0.2F;
                com->sticksmashhi_button_b_count++;
                break;

            case nFTComputerInputStickSmashLwButtonB:
                detect_ranges_x[i] += com->sticksmashlw_button_b_count * 0.2F;
                com->sticksmashlw_button_b_count++;
                break;

            case nFTComputerInputStickNButtonZButtonA:
                detect_ranges_x[i] += com->stickn_button_z_button_a_count * 0.2F;
                com->stickn_button_z_button_a_count++;
                break;

            default:
                break;
            }
            detect_ranges_x[i] += detect_far_x;
            detect_far_x = detect_ranges_x[i];
        }
        detect_far_x = syUtilsRandFloat() * detect_far_x;

        for (i = 0; i < attack_count; i++)
        {
            if (detect_far_x < detect_ranges_x[i])
            {
                if (com->input_kind == input_kinds[i])
                {
                    com->input_repeat_count++;

                    if (com->input_repeat_count >= 4)
                    {
                        ftComputerSetCommandImmediate(this_fp, nFTComputerInputMoveAutoStickTiltHiReleaseZ);

                        return TRUE;
                    }
                }
                else com->input_repeat_count = 0;

#if NDS_P4
                ndsP4ComputerSetCommandWaitShort(this_fp, input_kinds[i]);
#else
                ftComputerSetCommandWaitShort(this_fp, input_kinds[i]);
#endif

                com->input_kind = input_kinds[i];

                switch (input_kinds[i])
                {
                case nFTComputerInputStickNButtonA:
                    com->stickn_button_a_count = 0;
                    break;

                case nFTComputerInputStickTiltAutoXButtonA:
                    com->sticktilts_button_a_count = 0;
                    break;

                case nFTComputerInputStickSmashAutoXNYButtonA:
                    com->sticksmashs_button_a_count = 0;
                    break;

                case nFTComputerInputStickTiltHiButtonA:
                    com->sticktilthi_button_a_count = 0;
                    break;

                case nFTComputerInputStickSmashHiButtonA:
                    com->sticksmashhi_button_a_count = 0;
                    break;

                case nFTComputerInputStickTiltLwButtonA:
                    com->sticktiltlw_button_a_count = 0;
                    break;

                case nFTComputerInputStickSmashLwButtonA:
                    com->sticksmashlw_button_a_count = 0;
                    break;

                case nFTComputerInputStickSmashAutoXButtonB:
                    com->sticksmashs_button_b_count = 0;
                    break;

                case nFTComputerInputStickSmashHiButtonB:
                    com->sticksmashhi_button_b_count = 0;
                    break;

                case nFTComputerInputStickSmashLwButtonB:
                    com->sticksmashlw_button_b_count = 0;
                    break;

                case nFTComputerInputStickNButtonZButtonA:
                    com->stickn_button_z_button_a_count = 0;
                    break;

                default:
                    break;
                }
                if ((this_fp->fkind == nFTKindPurin) && (com->input_kind == nFTComputerInputStickSmashHiButtonB) && (syUtilsRandFloat() < 0.9F))
                {
                    return FALSE;
                }
                return TRUE;
            }
        }
    }
    else return FALSE;

    /* The source falls off its end here (the cumulative pick always lands
     * inside the table); said explicitly. */
    return FALSE;
}
