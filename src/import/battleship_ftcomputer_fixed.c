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

extern sb32 func_ovl2_800F8FFC(Vec3f *position);

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

/* ftcomputer.c:3716 */
sb32 ftComputerCheckFindTarget(FTStruct *this_fp)
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
 * against 1500 and 2500 is the squared distance against their squares. */
sb32 ftComputerCheckEvadeDistance(FTStruct *this_fp)
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
