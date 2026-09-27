/* P2-2p8 A5: the fighter hurtbox test's conservative reject.
 *
 * gmCollisionCheckFighterAttackDamageCollide (gm/gmcollision.c:1379) builds the
 * hurtbox joint's float world chain, its cofactor inverse and three axis-scale
 * square roots -- ~6,000 cycles of soft float the first time a joint is tested
 * in a tick -- and then gmCollisionTestRectangle decides. On the four-CPU stress
 * 12,376 such tests end in 39 hits: 99.7% are misses, and the joints tested are
 * mostly far from the attack.
 *
 * This kernel decides only "certainly a miss". It composes the joint's world
 * matrix in fixed point (include/nds/nds_r2_collision_fixed.h, host-graded to
 * ~0.002 world units at depth 12) from the source's own cached locals, places
 * the hurtbox's world axis-aligned bounds around it, and rejects when the
 * attack's swept segment bounds are separated from them on some world axis by
 * more than NDS_P2_HB_MARGIN_Q12 -- a thousand times the measured error. Every
 * other case returns 0 and the decomp float path decides exactly as before.
 *
 * Nothing here writes a source latch (FTParts unk_dobjtrans_0x5/6/7, locals,
 * matrices): a rejected test leaves the joint's latches as they were, and the
 * next consumer that needs them builds them itself through func_ovl2_800EDBA4.
 *
 * ARM state (Makefile -marm): the compose is 64-bit products, which Thumb would
 * turn into __aeabi_lmul calls. */

#include <ft/fighter.h>
#include <macros.h>

/* The DS divide and square-root units for the kernel's two overridable
 * hooks, as src/port/nds_r2_collision_fixed.c binds them under
 * NDS_R2_CFX_HWMATH: the same truncating 64-bit divide and floor root, proven
 * identical to the portable forms (scripts/check-r2-hwmath.ps1). The portable
 * defaults were a bit-by-bit __aeabi_ldivmod and a 32-step digit root on every
 * slot miss. */
#include <nds/nds_r2_hwmath_unit.h>
#define NDS_R2_CFX_DIV64(numerator, denominator) \
    ndsR2HwMathCfxDiv64((int64_t)(numerator), (int64_t)(denominator))
#define NDS_R2_CFX_ISQRT64(value) ndsR2HwMathCfxIsqrt64(value)
#include <nds/nds_r2_collision_fixed.h>

#define NDS_P2_HB_CHAIN_MAX 18
/* Four world units, Q12. */
#define NDS_P2_HB_MARGIN_Q12 (INT32_C(4) << NDS_R2_CFX_POS_BITS)
#define NDS_P2_HB_CACHE_SLOTS 64u

/* 0 off, 1 reject, 2 shadow: decide, count, and still run the float path so a
 * wrong reject shows up as a flip. `.data` so both values occupy one word. */
volatile u32 gNdsP2HurtboxRejectMode __attribute__((used, section(".data"))) = 1u;
__attribute__((used)) volatile u32 gNdsP2HurtboxRejects;
__attribute__((used)) volatile u32 gNdsP2HurtboxPasses;
__attribute__((used)) volatile u32 gNdsP2HurtboxDeclines;
__attribute__((used)) volatile u32 gNdsP2HurtboxFlips;
/* Bumped wherever the source clears FTParts latches (the port's
 * ftParamsUpdateFighterPartsTransform shims and the flat-walk topology reset),
 * so a cached world never outlives the latch state it stands beside. */
__attribute__((used)) volatile u32 gNdsP2HurtboxLatchEpoch;

typedef struct NDSP2HbWorld
{
    const DObj *dobj;
    u32 epoch;
    NDSR2CfxMtx world;
    /* 1/s_min at Q26, rounded up (0 = not yet derived): s_min is the shortest
     * world row, so radius * |W[k][c]| / s_k <= radius * |W[k][c]| * this. */
    int32_t inv_smin_q26;
} NDSP2HbWorld;

static NDSP2HbWorld sNdsP2HbCache[NDS_P2_HB_CACHE_SLOTS];

static int ndsP2HbLocalFromDObj(NDSR2CfxMtx *dst, const DObj *dobj)
{
    const float rotate[3] = { dobj->rotate.vec.f.x, dobj->rotate.vec.f.y,
                              dobj->rotate.vec.f.z };
    const float scale[3] = { dobj->scale.vec.f.x, dobj->scale.vec.f.y,
                             dobj->scale.vec.f.z };
    const float translate[3] = { dobj->translate.vec.f.x,
                                 dobj->translate.vec.f.y,
                                 dobj->translate.vec.f.z };

    return ndsR2CfxBuildLocal(dst, gSYSinTable, rotate, scale, translate);
}

/* The joint's local, as the source would use it: its cached float local when
 * transform_update_mode is set, otherwise built from the DObj TRS in fixed
 * point (gmCollisionTransformMatrixAll's terms) without caching it. */
static int ndsP2HbLocal(NDSR2CfxMtx *dst, const DObj *dobj,
                        const FTParts *parts)
{
    if (parts->transform_update_mode != 0)
    {
        return ndsR2CfxLoadF32(dst, (float (*)[4])parts->unk_dobjtrans_0x10);
    }
    return ndsP2HbLocalFromDObj(dst, dobj);
}

static NDSP2HbWorld *ndsP2HbSlot(const DObj *dobj)
{
    return &sNdsP2HbCache[((u32)(uintptr_t)dobj >> 4) &
                          (NDS_P2_HB_CACHE_SLOTS - 1u)];
}

/* World matrix of `joint`: func_ovl2_800EDBA4's walk (is_use_animlocks FALSE)
 * -- up to the first ancestor with a latched world or to the root -- composed in
 * fixed point, with each composed level cached for this latch epoch. */
static int ndsP2HbWorldOf(NDSR2CfxMtx *out, DObj *joint)
{
    DObj *chain[NDS_P2_HB_CHAIN_MAX];
    const u32 epoch = gNdsP2HurtboxLatchEpoch;
    NDSR2CfxMtx acc;
    DObj *cursor = joint;
    s32 depth = 0;
    s32 i;

    for (;;)
    {
        NDSP2HbWorld *slot = ndsP2HbSlot(cursor);
        const FTParts *parts = ftGetParts(cursor);

        if (parts == NULL)
        {
            return 0;
        }
        if ((slot->dobj == cursor) && (slot->epoch == epoch))
        {
            acc = slot->world;
            break;
        }
        if (parts->unk_dobjtrans_0x5 != 0)
        {
            if (ndsR2CfxLoadF32(&acc, (float (*)[4])parts->mtx_translate) == 0)
            {
                return 0;
            }
            break;
        }
        if (cursor->parent == DOBJ_PARENT_NULL)
        {
            /* The root's world is its local (gmCollisionCopyMatrix). */
            if (ndsP2HbLocal(&acc, cursor, parts) == 0)
            {
                return 0;
            }
            slot->dobj = cursor;
            slot->epoch = epoch;
            slot->world = acc;
            break;
        }
        if (depth >= NDS_P2_HB_CHAIN_MAX)
        {
            return 0;
        }
        chain[depth++] = cursor;
        cursor = cursor->parent;
    }
    for (i = depth - 1; i >= 0; i--)
    {
        const FTParts *parts = ftGetParts(chain[i]);
        NDSR2CfxMtx local;
        NDSR2CfxMtx world;
        NDSP2HbWorld *slot;

        if ((parts == NULL) || (ndsP2HbLocal(&local, chain[i], parts) == 0) ||
            (ndsR2CfxCompose(&world, &acc, &local) == 0))
        {
            return 0;
        }
        slot = ndsP2HbSlot(chain[i]);
        slot->dobj = chain[i];
        slot->epoch = epoch;
        slot->world = world;
        slot->inv_smin_q26 = 0;
        acc = world;
    }
    *out = acc;
    return 1;
}

/* 1/s_min for `joint`'s world (just produced by ndsP2HbWorldOf, so its slot
 * holds it unless another DObj evicted it). 0 = decline. */
static int32_t ndsP2HbInvSMin(const DObj *joint, const NDSR2CfxMtx *w)
{
    NDSP2HbWorld *slot = ndsP2HbSlot(joint);
    int32_t s2[3];
    int32_t s2_min;
    uint32_t s_q24;
    int32_t inv;

    if ((slot->dobj == joint) && (slot->epoch == gNdsP2HurtboxLatchEpoch) &&
        (slot->inv_smin_q26 != 0))
    {
        return slot->inv_smin_q26;
    }
    if (ndsR2CfxRowScales(w, s2, NULL, NULL, NULL) == 0)
    {
        return 0;
    }
    s2_min = s2[0];
    if (s2[1] < s2_min) { s2_min = s2[1]; }
    if (s2[2] < s2_min) { s2_min = s2[2]; }
    /* floor(sqrt(s2 << 22)) = s at Q24, rounded DOWN, so its reciprocal
     * rounded UP bounds 1/s from above. s2 >= 1/16 (the guard) keeps the
     * quotient inside int32. */
    s_q24 = NDS_R2_CFX_ISQRT64((uint64_t)s2_min << 22);
    if (s_q24 == 0u)
    {
        return 0;
    }
    inv = NDS_R2_CFX_DIV64((int64_t)1 << 50, (int64_t)s_q24) + 1;
    if ((slot->dobj == joint) && (slot->epoch == gNdsP2HurtboxLatchEpoch))
    {
        slot->inv_smin_q26 = inv;
    }
    return inv;
}

static int ndsP2HbVec(int32_t out[3], const Vec3f *v)
{
    out[0] = ndsR2CollisionF32ToFixed(v->x, NDS_R2_CFX_POS_BITS);
    out[1] = ndsR2CollisionF32ToFixed(v->y, NDS_R2_CFX_POS_BITS);
    out[2] = ndsR2CollisionF32ToFixed(v->z, NDS_R2_CFX_POS_BITS);
    return ((out[0] != NDS_R2_COLLISION_F32_OVERFLOW) &&
            (out[1] != NDS_R2_COLLISION_F32_OVERFLOW) &&
            (out[2] != NDS_R2_COLLISION_F32_OVERFLOW) &&
            (ndsR2CfxAbs32(out[0]) < NDS_R2_CFX_POS_MAX) &&
            (ndsR2CfxAbs32(out[1]) < NDS_R2_CFX_POS_MAX) &&
            (ndsR2CfxAbs32(out[2]) < NDS_R2_CFX_POS_MAX)) ? 1 : 0;
}

/* 1 = the float test would certainly miss; 0 = let it decide.
 *
 * The float test clips the attack's segment (pos_curr..pos_prev, or pos_curr
 * alone for a transfer attack) against the joint-local box offset +/-
 * (size + radius / s_k), s_k being the world matrix's row lengths. In world
 * space that box's half extent along world axis c is
 *   sum_k |W[k][c]| * size_k + radius * sum_k |W[k][c]| / s_k
 * and s_k >= s_min bounds the second sum. The segment lies inside the box its
 * two ends span. Separation of the two boxes on one world axis, by more than
 * the margin, proves the miss. */
static int ndsP2HbRejectPoints(const Vec3f *pos_curr, const Vec3f *pos_prev,
                               f32 attack_size, const FTDamageColl *damage)
{
    DObj *joint = damage->joint;
    const FTStruct *fp;
    NDSR2CfxMtx w;
    int32_t off[3];
    int32_t size[3];
    int32_t p0[3];
    int32_t p1[3];
    int32_t center[3];
    int32_t radius;
    int32_t inv_smin;
    u32 c;

    if ((joint == NULL) || (joint->parent_gobj == NULL))
    {
        return 0;
    }
    fp = ftGetStruct(joint->parent_gobj);
    if ((fp == NULL) || (fp->is_use_animlocks != FALSE))
    {
        return 0;
    }
    radius = ndsR2CollisionF32ToFixed(attack_size, NDS_R2_CFX_POS_BITS);
    if ((radius == NDS_R2_COLLISION_F32_OVERFLOW) ||
        (ndsR2CfxAbs32(radius) >= NDS_R2_CFX_POS_MAX) ||
        (ndsP2HbVec(p0, pos_curr) == 0) ||
        (ndsP2HbVec(p1, pos_prev) == 0) ||
        (ndsP2HbVec(off, &damage->offset) == 0) ||
        (ndsP2HbVec(size, &damage->size) == 0) ||
        (ndsP2HbWorldOf(&w, joint) == 0))
    {
        return 0;
    }
    inv_smin = ndsP2HbInvSMin(joint, &w);
    if (inv_smin <= 0)
    {
        return 0;
    }
    ndsR2CfxTransformPoint(center, &w, off);
    radius = ndsR2CfxAbs32(radius);
    for (c = 0u; c < 3u; c++)
    {
        int64_t ext = NDS_P2_HB_MARGIN_Q12;
        int64_t sum_abs = 0;
        int64_t rad_term;
        int32_t lo = (p0[c] < p1[c]) ? p0[c] : p1[c];
        int32_t hi = (p0[c] < p1[c]) ? p1[c] : p0[c];
        u32 k;

        for (k = 0u; k < 3u; k++)
        {
            int64_t cell = ndsR2CfxAbs32(w.r[k][c]);

            sum_abs += cell;
            ext += (cell * ndsR2CfxAbs32(size[k]) +
                    (((int64_t)1 << NDS_R2_CFX_ROT_BITS) - 1)) >>
                   NDS_R2_CFX_ROT_BITS;
        }
        /* radius * sum_abs * inv_smin, each reduction rounded up. */
        rad_term = ((int64_t)radius * sum_abs +
                    (((int64_t)1 << NDS_R2_CFX_ROT_BITS) - 1)) >>
                   NDS_R2_CFX_ROT_BITS;
        rad_term = (rad_term * inv_smin +
                    (((int64_t)1 << NDS_R2_CFX_ROT_BITS) - 1)) >>
                   NDS_R2_CFX_ROT_BITS;
        ext += rad_term;
        if (((int64_t)hi < (int64_t)center[c] - ext) ||
            ((int64_t)lo > (int64_t)center[c] + ext))
        {
            return 1;
        }
    }
    return 0;
}

int ndsP2HurtboxRejectTest(const FTAttackColl *attack,
                           const FTDamageColl *damage)
{
    return ndsP2HbRejectPoints(&attack->pos_curr, &attack->pos_prev,
                               attack->size, damage);
}

/* The weapon and item attack paths (gmCollisionCheckWeaponAttackFighterDamage
 * Collide / ...ItemAttack...) run the same joint frame and TestRectangle on
 * attack_pos[attack_id]. */
int ndsP2HurtboxRejectPoints(const Vec3f *pos_curr, const Vec3f *pos_prev,
                             f32 attack_size, const FTDamageColl *damage)
{
    return ndsP2HbRejectPoints(pos_curr, pos_prev, attack_size, damage);
}
