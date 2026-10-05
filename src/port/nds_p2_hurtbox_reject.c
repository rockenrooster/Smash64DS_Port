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
/* Same-ROM A/B word for the local build's zero-angle and unit-scale paths. */
extern volatile u32 gNdsR2CfxFastPaths;
#define NDS_R2_CFX_FAST_PATHS() (gNdsR2CfxFastPaths != 0u)
#include <nds/nds_r2_collision_fixed.h>

#define NDS_P2_HB_CHAIN_MAX 18
/* Four world units, Q12. */
#define NDS_P2_HB_MARGIN_Q12 (INT32_C(4) << NDS_R2_CFX_POS_BITS)
#define NDS_P2_HB_CACHE_SLOTS 64u

volatile u32 gNdsR2CfxFastPaths __attribute__((used, section(".data"))) = 1u;

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

/* A world of this epoch for a DObj is in the slot its pointer hashes to. A
 * fighter's DObjs are 136 bytes apart, so (ptr >> 4) & 63 put a joint and the
 * joints eight or nine places along the pool on one slot; the multiplicative
 * hash spreads them. */
static inline NDSP2HbWorld *ndsP2HbSlot(const void *dobj)
{
    return &sNdsP2HbCache[((u32)(uintptr_t)dobj * 0x9E3779B1u) >> 26];
}

static inline u32 ndsP2HbBits(f32 value)
{
    u32 bits;

    __builtin_memcpy(&bits, &value, sizeof(bits));
    return bits;
}

#if NDS_P2_JOINT_RESIDENT
/* The relaxed local below. lbCommonSin's table index, (s32)(angle * K) with
 * K = 0xa2f983 * 2^-14, as mantissa * 0xa2f983 * 2^(exponent - 164) truncated:
 * the lean kernel's form (src/nds/nds_ftr_lean_kernel.c), without the float
 * multiply's rounding step. 0 = decline (|angle| >= 2^14 or not finite). */
static int ndsP2HbAngleIndex(f32 angle, s32 *out)
{
    const u32 bits = ndsP2HbBits(angle);
    const u32 exponent = (bits >> 23) & 0xffu;
    u32 magnitude;

    if (exponent <= 116u)
    {
        *out = 0;
        return 1;
    }
    if (exponent > 140u)
    {
        return 0;
    }
    magnitude = (u32)(((u64)((bits & 0x7fffffu) | 0x800000u) * 0xa2f983u) >>
                      (164u - exponent));
    *out = ((bits & 0x80000000u) != 0u) ? -(s32)magnitude : (s32)magnitude;
    return 1;
}

static inline s32 ndsP2HbSinQ15(s32 index)
{
    const u32 id = (u32)index & 0xfffu;
    const s32 value = (s32)gSYSinTable[id & 0x7ffu];

    return ((id & 0x800u) != 0u) ? -value : value;
}

/* One copy of the float -> fixed edge for the six conversions a local takes. */
static int32_t __attribute__((noinline)) ndsP2HbToFixed(f32 value,
                                                        u32 frac_bits)
{
    return ndsR2CollisionF32ToFixed(value, frac_bits);
}

/* gmCollisionTransformMatrixAll's local from the DObj TRS in Q26 / Q12 (owner
 * ruling D13, 2026-10-04: the fighter joint chain is fixed point end to end).
 * The rotation terms are the source's, in its order, at Q30 from the Q15 table
 * the port's lbCommonSin reads. Two relaxations against the exact builder
 * (ndsR2CfxBuildLocal, about five times this code): the index truncates as
 * above, and the cosine reads the index a quarter turn on rather than indexing
 * angle + 90 degrees -- each one table step (1/4096 turn) at a rounding edge,
 * far inside the reject's four-unit margin. 0 = decline (a scale past 4, a
 * translation past 131,072 units, an angle the index declines). */
static int ndsP2HbLocalFromDObj(NDSR2CfxMtx *dst, const DObj *dobj)
{
    const f32 scale[3] = { dobj->scale.vec.f.x, dobj->scale.vec.f.y,
                           dobj->scale.vec.f.z };
    const f32 translate[3] = { dobj->translate.vec.f.x,
                               dobj->translate.vec.f.y,
                               dobj->translate.vec.f.z };
    s32 ix;
    s32 iy;
    s32 iz;
    s32 sx;
    s32 cx;
    s32 sy;
    s32 cy;
    s32 sz;
    s32 cz;
    s32 sxsy;
    s32 cxsy;
    s32 rot[3][3]; /* Q30 */
    u32 row;
    u32 col;

    if ((ndsP2HbAngleIndex(dobj->rotate.vec.f.x, &ix) == 0) ||
        (ndsP2HbAngleIndex(dobj->rotate.vec.f.y, &iy) == 0) ||
        (ndsP2HbAngleIndex(dobj->rotate.vec.f.z, &iz) == 0))
    {
        return 0;
    }
    sx = ndsP2HbSinQ15(ix);
    cx = ndsP2HbSinQ15(ix + 0x400);
    sy = ndsP2HbSinQ15(iy);
    cy = ndsP2HbSinQ15(iy + 0x400);
    sz = ndsP2HbSinQ15(iz);
    cz = ndsP2HbSinQ15(iz + 0x400);
    /* Pair products of Q15 entries are at most 2^30; each sum below is a
     * rotation cell, so it stays within 2^30 plus the table's rounding. */
    sxsy = sx * sy;
    cxsy = cx * sy;
    rot[0][0] = cy * cz;
    rot[0][1] = cy * sz;
    rot[0][2] = -sy * (1 << 15);
    rot[1][0] = (s32)(((s64)sxsy * cz) >> 15) - (cx * sz);
    rot[1][1] = (s32)(((s64)sxsy * sz) >> 15) + (cx * cz);
    rot[1][2] = sx * cy;
    rot[2][0] = (s32)(((s64)cxsy * cz) >> 15) + (sx * sz);
    rot[2][1] = (s32)(((s64)cxsy * sz) >> 15) - (sx * cz);
    rot[2][2] = cx * cy;
    for (row = 0u; row < 3u; row++)
    {
        int32_t s;

        if (ndsP2HbBits(scale[row]) == 0x3f800000u)
        {
            for (col = 0u; col < 3u; col++)
            {
                dst->r[row][col] = (rot[row][col] + 8) >> 4;
            }
            continue;
        }
        s = ndsP2HbToFixed(scale[row], NDS_R2_CFX_ROT_BITS);
        if ((s == NDS_R2_COLLISION_F32_OVERFLOW) ||
            (ndsR2CfxAbs32(s) > NDS_R2_CFX_ROT_MAX))
        {
            return 0;
        }
        for (col = 0u; col < 3u; col++)
        {
            /* Q30 x Q26 -> Q26: at most 2^58 before the shift. */
            dst->r[row][col] =
                (int32_t)ndsR2CfxShr((int64_t)rot[row][col] * s, 30u);
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        const int32_t t = ndsP2HbToFixed(translate[col], NDS_R2_CFX_POS_BITS);

        if ((t == NDS_R2_COLLISION_F32_OVERFLOW) ||
            (ndsR2CfxAbs32(t) >= NDS_R2_CFX_POS_MAX))
        {
            return 0;
        }
        dst->t[col] = t;
    }
    return 1;
}
#else
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
#endif

/* The joint's local, as the source would use it: its cached float local when
 * transform_update_mode is set, otherwise built from the DObj TRS in fixed
 * point (gmCollisionTransformMatrixAll's terms). */
static int ndsP2HbLocal(NDSR2CfxMtx *dst, const DObj *dobj,
                        const FTParts *parts)
{
    if (parts->transform_update_mode != 0)
    {
        return ndsR2CfxLoadF32(dst, (float (*)[4])parts->unk_dobjtrans_0x10);
    }
    return ndsP2HbLocalFromDObj(dst, dobj);
}

/* An upper bound on 2^39 / sqrt(s2) -- 1/s at Q26 for s^2 = s2 at Q26 -- for
 * s2 in [NDS_R2_CFX_S2_MIN, NDS_R2_CFX_S2_MAX] = [2^22, 2^30], without the
 * divide and square-root units (P2-2p8, 2026-09-29: their busy-waits were ~18%
 * of the kernel's hottest rows). With s2 = m * 2^q + r, m the top six bits
 * (32..63), s2 >= m * 2^q, so
 *   2^39 / sqrt(s2) <= 2^(39 - q/2) / sqrt(m)
 * and the tables hold ceil(2^24 / sqrt(m)) (q even) and ceil(2^24 * sqrt(2/m))
 * (q odd); the shift is exact. At most 1.56% above the exact value.
 * artifacts/performance/2026-09-29_p2-2p8-hurtbox-box/invsqrt_test.c checks
 * inv^2 * s2 >= 2^78 for every s2 in the range (1,069,547,521 values). */
static const uint32_t sNdsP2HbInvSqrtEven[32] = {
    2965821u, 2920539u, 2877269u, 2835868u,
    2796203u, 2758158u, 2721624u, 2686505u,
    2652711u, 2620161u, 2588781u, 2558502u,
    2529261u, 2501000u, 2473666u, 2447209u,
    2421583u, 2396746u, 2372657u, 2349281u,
    2326582u, 2304528u, 2283090u, 2262240u,
    2241950u, 2222197u, 2202957u, 2184208u,
    2165930u, 2148103u, 2130709u, 2113731u,
};
static const uint32_t sNdsP2HbInvSqrtOdd[32] = {
    4194304u, 4130266u, 4069073u, 4010522u,
    3954428u, 3900624u, 3848958u, 3799292u,
    3751500u, 3705468u, 3661089u, 3618268u,
    3576915u, 3536948u, 3498292u, 3460876u,
    3424635u, 3389510u, 3355444u, 3322384u,
    3290283u, 3259095u, 3228777u, 3199290u,
    3170596u, 3142661u, 3115451u, 3088936u,
    3063087u, 3037876u, 3013277u, 2989267u,
};

static int32_t ndsP2HbInvSqrtQ26(uint32_t s2)
{
    const uint32_t q = 26u - (uint32_t)__builtin_clz(s2);
    const uint32_t m = s2 >> q;
    const uint32_t t = ((q & 1u) != 0u) ? sNdsP2HbInvSqrtOdd[m - 32u]
                                        : sNdsP2HbInvSqrtEven[m - 32u];

    return (int32_t)(t << (15u - ((q + 1u) >> 1)));
}

/* Same-ROM A/B word: 0 takes 1/s_min from the divide and root units. */
volatile u32 gNdsP2HbInvTable __attribute__((used, section(".data"))) = 1u;

/* 1/s_min for the world `w` (the caller keeps it in the joint's slot).
 * 0 = decline. */
static int32_t ndsP2HbInvSMinOf(const NDSR2CfxMtx *w)
{
    int32_t s2[3];
    int32_t s2_min;
    uint32_t s_q24;
    int32_t inv;

    if (ndsR2CfxRowScales(w, s2, NULL, NULL, NULL) == 0)
    {
        return 0;
    }
    s2_min = s2[0];
    if (s2[1] < s2_min) { s2_min = s2[1]; }
    if (s2[2] < s2_min) { s2_min = s2[2]; }
    if (gNdsP2HbInvTable != 0u)
    {
        /* RowScales' guard already put s2_min in the table's range. */
        inv = ndsP2HbInvSqrtQ26((uint32_t)s2_min);
    }
    else
    {
        /* floor(sqrt(s2 << 22)) = s at Q24, rounded DOWN, so its reciprocal
         * rounded UP bounds 1/s from above. s2 >= 1/16 (the guard) keeps the
         * quotient inside int32. */
        s_q24 = NDS_R2_CFX_ISQRT64((uint64_t)s2_min << 22);
        if (s_q24 == 0u)
        {
            return 0;
        }
        inv = NDS_R2_CFX_DIV64((int64_t)1 << 50, (int64_t)s_q24) + 1;
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

/* The four float -> Q12 edge conversions are ~100 cycles each, and a test
 * repeats most of them: one attack is tried against every damage box of a
 * victim in turn, and a damage box's offset and size never change. Both are
 * memoized on the float bits they convert, so a hit returns exactly what the
 * conversion would (P2-2p8, 2026-09-27: Saffron's hazard frames made ~190
 * conversions a frame; same-ROM A/B there: top 5% -6.9K, replay identical). */
typedef struct NDSP2HbAttackMemo
{
    u32 bits[7];
    int32_t p0[3];
    int32_t p1[3];
    int32_t radius;
    u32 ok;
} NDSP2HbAttackMemo;
/* An FTDamageColl is 11 words, so a victim's eleven colls take eleven
 * distinct slots of sixteen under (ptr >> 2) & 15; eight slots put three pairs
 * of one victim's colls on the same slot (P2-2p8, 2026-09-29: memo misses on
 * the gate 8,499 -> 2,150 a match). */
#define NDS_P2_HB_DAMAGE_MEMO_SLOTS 16u
typedef struct NDSP2HbDamageMemo
{
    const FTDamageColl *damage;
    u32 bits[6];
    int32_t off[3];
    int32_t size[3];
    u32 ok;
    /* The world-axis box this coll's world gave it in epoch box_epoch
     * (box_valid; cleared whenever the entry is refilled): the test's own
     * pieces, so a re-test in the epoch runs the same arithmetic on them. */
    u32 box_valid;
    u32 box_epoch;
    const FTStruct *box_fp;
    int32_t box_center[3];
    int32_t box_sum_abs[3];
    int32_t box_inv_smin;
    int64_t box_ext0[3];
} NDSP2HbDamageMemo;
static NDSP2HbAttackMemo sNdsP2HbAttackMemo;
static NDSP2HbDamageMemo sNdsP2HbDamageMemo[NDS_P2_HB_DAMAGE_MEMO_SLOTS];

/* The float test's own frame (P2-2p8, 2026-09-29). The world-axis test below
 * bounds the box by its world-aligned extents, which for a rotated joint are up
 * to sqrt(3) wider than the box; the tests it passes on to the float path
 * (654 a gate match, ~2.4 a P90-98 frame, each a soft-float world chain,
 * inverse and three sqrtf) are mostly such corner misses.
 *
 * The source takes both attack points into the joint's frame (the inverse of
 * W, minus the offset) and clips the segment against +/- (size + radius / s_k).
 * Both points beyond the same face of one local axis is a miss whatever the
 * clip loop then does: an x or y face shares an outcode bit and returns at the
 * loop's first test, and every point the loop clips lies on the segment, so
 * for a z face its final z outcodes still share the bit.
 *
 * No inverse is formed. With W's rows R_k and n_k = R_i x R_j ((k, i, j)
 * cyclic), local_k(p) = (p - t) . n_k / (R_k . n_k), so a point is beyond the
 * face local_k = h when (p - t) . n_k - h * det_k > 0 (det_k = R_k . n_k, made
 * positive by flipping n_k), and that difference over |n_k| is its world
 * distance to the face plane. Requiring it to exceed NDS_P2_HB_MARGIN_Q12 *
 * |n_k|_1 keeps the world-axis test's four-unit margin; h uses the table bound
 * on 1/s_k, which is never below the float's. Values outside the guards
 * decline. A/B word gNdsP2HbLocalTest (0 = world axes only). */
volatile u32 gNdsP2HbLocalTest __attribute__((used, section(".data"))) = 1u;
__attribute__((used)) volatile u32 gNdsP2HbLocalRejects;

static int ndsP2HbRejectLocal(const NDSR2CfxMtx *w, const int32_t off[3],
                              const int32_t size[3], int32_t radius,
                              const int32_t p0[3], const int32_t p1[3])
{
    static const u8 next[3] = { 1u, 2u, 0u };
    static const u8 prev[3] = { 2u, 0u, 1u };
    int32_t s2[3];
    int64_t v0[3];
    int64_t v1[3];
    u32 k;
    u32 c;

    if (ndsR2CfxRowScales(w, s2, NULL, NULL, NULL) == 0)
    {
        return 0;
    }
    for (c = 0u; c < 3u; c++)
    {
        v0[c] = (int64_t)p0[c] - w->t[c];
        v1[c] = (int64_t)p1[c] - w->t[c];
    }
    for (k = 0u; k < 3u; k++)
    {
        const u32 i = next[k];
        const u32 j = prev[k];
        int64_t n[3];
        int64_t n1 = 0;
        int64_t det;
        int64_t num0;
        int64_t num1;
        int64_t reach;
        int64_t hi;
        int64_t lo;
        int64_t thr;

        for (c = 0u; c < 3u; c++)
        {
            const u32 a = next[c];
            const u32 b = prev[c];

            n[c] = ndsR2CfxShr((int64_t)w->r[i][a] * w->r[j][b] -
                                   (int64_t)w->r[i][b] * w->r[j][a],
                               NDS_R2_CFX_ROT_BITS);
        }
        det = ndsR2CfxShr((int64_t)w->r[k][0] * n[0] +
                              (int64_t)w->r[k][1] * n[1] +
                              (int64_t)w->r[k][2] * n[2],
                          NDS_R2_CFX_ROT_BITS);
        if (det < 0)
        {
            det = -det;
            n[0] = -n[0];
            n[1] = -n[1];
            n[2] = -n[2];
        }
        /* det > 2^-10 at Q26 and below 2^7; |n| below 2^4 (Q26). */
        if ((det < ((int64_t)1 << 16)) || (det >= ((int64_t)1 << 33)))
        {
            return 0;
        }
        for (c = 0u; c < 3u; c++)
        {
            const int64_t m = (n[c] < 0) ? -n[c] : n[c];

            if (m >= ((int64_t)1 << 30))
            {
                return 0;
            }
            n1 += m;
        }
        /* size + radius / s_k, the radius term rounded up (Q12). */
        reach = (int64_t)size[k] +
                (((int64_t)radius * ndsP2HbInvSqrtQ26((uint32_t)s2[k]) +
                  (((int64_t)1 << NDS_R2_CFX_ROT_BITS) - 1)) >>
                 NDS_R2_CFX_ROT_BITS);
        hi = (int64_t)off[k] + reach;
        lo = (int64_t)off[k] - reach;
        if ((hi >= ((int64_t)1 << 26)) || (hi <= -((int64_t)1 << 26)) ||
            (lo >= ((int64_t)1 << 26)) || (lo <= -((int64_t)1 << 26)))
        {
            return 0;
        }
        num0 = v0[0] * n[0] + v0[1] * n[1] + v0[2] * n[2];
        num1 = v1[0] * n[0] + v1[1] * n[1] + v1[2] * n[2];
        thr = (int64_t)NDS_P2_HB_MARGIN_Q12 * n1;
        if (((num0 - hi * det) > thr) && ((num1 - hi * det) > thr))
        {
            return 1;
        }
        if (((lo * det - num0) > thr) && ((lo * det - num1) > thr))
        {
            return 1;
        }
    }
    return 0;
}

/* World matrix of `joint`: func_ovl2_800EDBA4's walk (is_use_animlocks FALSE)
 * -- up to the first ancestor with a latched world or to the root -- composed in
 * fixed point, with each composed level cached for this latch epoch.
 *
 * Returns the joint's world -- in the joint's cache slot, or in *scratch when
 * the walk stopped on a latched world at the joint itself -- or NULL to
 * decline; *slot_out is the joint's slot when the world is in one. The slot is
 * checked before the DObj's FTParts is read, each level is composed straight
 * into its slot, and a slot's tag is cleared before its world is written, so a
 * failed build never leaves a tag on a stale world (P2-2p8, 2026-09-29: the
 * previous walk read FTParts first and copied every level three times). */
static const NDSR2CfxMtx *ndsP2HbWorldOf(DObj *joint, NDSR2CfxMtx *scratch,
                                        NDSP2HbWorld **slot_out)
{
    DObj *chain[NDS_P2_HB_CHAIN_MAX];
    const FTParts *chain_parts[NDS_P2_HB_CHAIN_MAX];
    const u32 epoch = gNdsP2HurtboxLatchEpoch;
    const NDSR2CfxMtx *acc;
    NDSP2HbWorld *slot;
    DObj *cursor = joint;
    s32 depth = 0;

    *slot_out = NULL;
    for (;;)
    {
        const FTParts *parts;

        slot = ndsP2HbSlot(cursor);
        if ((slot->dobj == cursor) && (slot->epoch == epoch))
        {
            acc = &slot->world;
            break;
        }
        parts = ftGetParts(cursor);
        if (parts == NULL)
        {
            return NULL;
        }
        if (parts->unk_dobjtrans_0x5 != 0)
        {
            if (ndsR2CfxLoadF32(scratch,
                                (float (*)[4])parts->mtx_translate) == 0)
            {
                return NULL;
            }
            if (depth == 0)
            {
                return scratch;
            }
            acc = scratch;
            break;
        }
        if (cursor->parent == DOBJ_PARENT_NULL)
        {
            /* The root's world is its local (gmCollisionCopyMatrix). */
            slot->dobj = NULL;
            if (ndsP2HbLocal(&slot->world, cursor, parts) == 0)
            {
                return NULL;
            }
            slot->dobj = cursor;
            slot->epoch = epoch;
            slot->inv_smin_q26 = 0;
            acc = &slot->world;
            break;
        }
        if (depth >= NDS_P2_HB_CHAIN_MAX)
        {
            return NULL;
        }
        chain[depth] = cursor;
        chain_parts[depth] = parts;
        depth++;
        cursor = cursor->parent;
    }
    while (depth > 0)
    {
        NDSR2CfxMtx local;

        depth--;
        cursor = chain[depth];
        slot = ndsP2HbSlot(cursor);
        if (ndsP2HbLocal(&local, cursor, chain_parts[depth]) == 0)
        {
            return NULL;
        }
        /* Compose reads lhs into a temp before writing dst, so acc may be
         * this very slot (a parent on the same slot). */
        slot->dobj = NULL;
        if (ndsR2CfxCompose(&slot->world, acc, &local) == 0)
        {
            return NULL;
        }
        slot->dobj = cursor;
        slot->epoch = epoch;
        slot->inv_smin_q26 = 0;
        acc = &slot->world;
    }
    *slot_out = slot;
    return acc;
}

/* The attack's two points and radius, as ndsP2HbVec / the radius conversion
 * give them; NULL when any is out of range. */
static const NDSP2HbAttackMemo *ndsP2HbAttackPoints(const Vec3f *pos_curr,
                                                     const Vec3f *pos_prev,
                                                     f32 attack_size)
{
    NDSP2HbAttackMemo *m = &sNdsP2HbAttackMemo;
    const u32 b0 = ndsP2HbBits(pos_curr->x);
    const u32 b1 = ndsP2HbBits(pos_curr->y);
    const u32 b2 = ndsP2HbBits(pos_curr->z);
    const u32 b3 = ndsP2HbBits(pos_prev->x);
    const u32 b4 = ndsP2HbBits(pos_prev->y);
    const u32 b5 = ndsP2HbBits(pos_prev->z);
    const u32 b6 = ndsP2HbBits(attack_size);

    if (((b0 ^ m->bits[0]) | (b1 ^ m->bits[1]) | (b2 ^ m->bits[2]) |
         (b3 ^ m->bits[3]) | (b4 ^ m->bits[4]) | (b5 ^ m->bits[5]) |
         (b6 ^ m->bits[6])) != 0u)
    {
        const int32_t radius =
            ndsR2CollisionF32ToFixed(attack_size, NDS_R2_CFX_POS_BITS);

        m->ok = ((radius != NDS_R2_COLLISION_F32_OVERFLOW) &&
                 (ndsR2CfxAbs32(radius) < NDS_R2_CFX_POS_MAX) &&
                 (ndsP2HbVec(m->p0, pos_curr) != 0) &&
                 (ndsP2HbVec(m->p1, pos_prev) != 0)) ? 1u : 0u;
        m->radius = radius;
        m->bits[0] = b0; m->bits[1] = b1; m->bits[2] = b2; m->bits[3] = b3;
        m->bits[4] = b4; m->bits[5] = b5; m->bits[6] = b6;
    }
    return (m->ok != 0u) ? m : NULL;
}

/* The damage box's offset and size in Q12; NULL when either is out of range. */
static NDSP2HbDamageMemo *ndsP2HbDamageBox(const FTDamageColl *damage)
{
    NDSP2HbDamageMemo *m =
        &sNdsP2HbDamageMemo[((u32)(uintptr_t)damage >> 2) &
                            (NDS_P2_HB_DAMAGE_MEMO_SLOTS - 1u)];
    const u32 b0 = ndsP2HbBits(damage->offset.x);
    const u32 b1 = ndsP2HbBits(damage->offset.y);
    const u32 b2 = ndsP2HbBits(damage->offset.z);
    const u32 b3 = ndsP2HbBits(damage->size.x);
    const u32 b4 = ndsP2HbBits(damage->size.y);
    const u32 b5 = ndsP2HbBits(damage->size.z);

    if ((m->damage != damage) ||
        (((b0 ^ m->bits[0]) | (b1 ^ m->bits[1]) | (b2 ^ m->bits[2]) |
          (b3 ^ m->bits[3]) | (b4 ^ m->bits[4]) | (b5 ^ m->bits[5])) != 0u))
    {
        m->ok = ((ndsP2HbVec(m->off, &damage->offset) != 0) &&
                 (ndsP2HbVec(m->size, &damage->size) != 0)) ? 1u : 0u;
        m->damage = damage;
        m->bits[0] = b0; m->bits[1] = b1; m->bits[2] = b2;
        m->bits[3] = b3; m->bits[4] = b4; m->bits[5] = b5;
        m->box_valid = 0u;
    }
    return (m->ok != 0u) ? m : NULL;
}

/* The world-axis separation test on its pieces: ext0[c] = margin + sum_k
 * |W[k][c]| * size_k (each term rounded up), sum_abs[c] = sum_k |W[k][c]|. */
static inline int ndsP2HbAxisReject(const int32_t center[3],
                                    const int64_t ext0[3],
                                    const int32_t sum_abs[3],
                                    int32_t inv_smin, int32_t radius,
                                    const int32_t p0[3], const int32_t p1[3])
{
    u32 c;

    for (c = 0u; c < 3u; c++)
    {
        const int32_t a = p0[c];
        const int32_t b = p1[c];
        const int32_t lo = (a < b) ? a : b;
        const int32_t hi = (a < b) ? b : a;
        int64_t rad_term;
        int64_t ext;

        /* radius * sum_abs * inv_smin, each reduction rounded up. */
        rad_term = ((int64_t)radius * sum_abs[c] +
                    (((int64_t)1 << NDS_R2_CFX_ROT_BITS) - 1)) >>
                   NDS_R2_CFX_ROT_BITS;
        rad_term = (rad_term * inv_smin +
                    (((int64_t)1 << NDS_R2_CFX_ROT_BITS) - 1)) >>
                   NDS_R2_CFX_ROT_BITS;
        ext = ext0[c] + rad_term;
        if (((int64_t)hi < (int64_t)center[c] - ext) ||
            ((int64_t)lo > (int64_t)center[c] + ext))
        {
            return 1;
        }
    }
    return 0;
}

/* A coll is tested once per attack coll that reaches its fighter, so within an
 * epoch most tests after the first find the joint's world cached and still
 * paid the head, the walk, 1/s_min, the transform and the extents (4,550 of
 * the gate's ~14,000 tests a match). The pieces the separation test reads are
 * kept in the coll's damage memo entry for the epoch; a re-test runs the same
 * separation test on them and goes on to the full path only when it does not
 * separate (the local test needs the world). Same-ROM A/B word
 * gNdsP2HbBoxCache (0 = always the full path). */
volatile u32 gNdsP2HbBoxCache __attribute__((used, section(".data"))) = 1u;
__attribute__((used)) volatile u32 gNdsP2HbBoxHits;

/* 1 = the float test would certainly miss; 0 = let it decide.
 *
 * The float test clips the attack's segment (pos_curr..pos_prev, or pos_curr
 * alone for a transfer attack) against the joint-local box offset +/-
 * (size + radius / s_k), s_k being the world matrix's row lengths. In world
 * space that box's half extent along world axis c is
 *   sum_k |W[k][c]| * size_k + radius * sum_k |W[k][c]| / s_k
 * and s_k >= s_min bounds the second sum. The segment lies inside the box its
 * two ends span. Separation of the two boxes on one world axis, by more than
 * the margin, proves the miss.
 *
 * The memos and the world are read in place: nothing between their lookup and
 * the last use below writes another memo entry or cache slot. */
static int ndsP2HbRejectPoints(const Vec3f *pos_curr, const Vec3f *pos_prev,
                               f32 attack_size, const FTDamageColl *damage)
{
    const u32 epoch = gNdsP2HurtboxLatchEpoch;
    DObj *joint = damage->joint;
    const FTStruct *fp;
    const NDSP2HbAttackMemo *am;
    NDSP2HbDamageMemo *dm;
    const NDSR2CfxMtx *w;
    NDSP2HbWorld *slot;
    NDSR2CfxMtx scratch;
    int32_t center[3];
    int32_t sum_abs[3];
    int64_t ext0[3];
    int32_t radius;
    int32_t inv_smin;
    u32 c;

    /* Every step below is a pure function of its inputs (the memos only
     * cache), so taking the damage memo first changes no result: a failure
     * anywhere returns 0 whichever step finds it. A box of this epoch was
     * built only after the head below passed, and nothing the head reads
     * (the joint, its fighter) changes within an epoch but the animlocks
     * flag, which is read again. */
    dm = ndsP2HbDamageBox(damage);
    if (dm == NULL)
    {
        return 0;
    }
    if ((gNdsP2HbBoxCache != 0u) && (dm->box_valid != 0u) &&
        (dm->box_epoch == epoch) && (dm->box_fp->is_use_animlocks == FALSE))
    {
        am = ndsP2HbAttackPoints(pos_curr, pos_prev, attack_size);
        if (am == NULL)
        {
            return 0;
        }
        if (ndsP2HbAxisReject(dm->box_center, dm->box_ext0, dm->box_sum_abs,
                              dm->box_inv_smin, ndsR2CfxAbs32(am->radius),
                              am->p0, am->p1) != 0)
        {
            gNdsP2HbBoxHits++;
            return 1;
        }
        /* Not separated: the full path below repeats the test and goes on
         * to the local test. */
    }
    if ((joint == NULL) || (joint->parent_gobj == NULL))
    {
        return 0;
    }
    fp = ftGetStruct(joint->parent_gobj);
    if ((fp == NULL) || (fp->is_use_animlocks != FALSE))
    {
        return 0;
    }
    am = ndsP2HbAttackPoints(pos_curr, pos_prev, attack_size);
    if (am == NULL)
    {
        return 0;
    }
    w = ndsP2HbWorldOf(joint, &scratch, &slot);
    if (w == NULL)
    {
        return 0;
    }
    if ((slot != NULL) && (slot->inv_smin_q26 != 0))
    {
        inv_smin = slot->inv_smin_q26;
    }
    else
    {
        inv_smin = ndsP2HbInvSMinOf(w);
        if (inv_smin <= 0)
        {
            return 0;
        }
        if (slot != NULL)
        {
            slot->inv_smin_q26 = inv_smin;
        }
    }
    ndsR2CfxTransformPoint(center, w, dm->off);
    radius = ndsR2CfxAbs32(am->radius);
    for (c = 0u; c < 3u; c++)
    {
        int64_t ext = NDS_P2_HB_MARGIN_Q12;
        int32_t abs_sum = 0;
        u32 k;

        /* |W| <= 2^28 (the guards), so three cells sum inside an int32. */
        for (k = 0u; k < 3u; k++)
        {
            int64_t cell = ndsR2CfxAbs32(w->r[k][c]);

            abs_sum += (int32_t)cell;
            ext += (cell * ndsR2CfxAbs32(dm->size[k]) +
                    (((int64_t)1 << NDS_R2_CFX_ROT_BITS) - 1)) >>
                   NDS_R2_CFX_ROT_BITS;
        }
        ext0[c] = ext;
        sum_abs[c] = abs_sum;
    }
    dm->box_valid = 1u;
    dm->box_epoch = epoch;
    dm->box_fp = fp;
    dm->box_inv_smin = inv_smin;
    for (c = 0u; c < 3u; c++)
    {
        dm->box_center[c] = center[c];
        dm->box_sum_abs[c] = sum_abs[c];
        dm->box_ext0[c] = ext0[c];
    }
    if (ndsP2HbAxisReject(center, ext0, sum_abs, inv_smin, radius, am->p0,
                          am->p1) != 0)
    {
        return 1;
    }
    if ((gNdsP2HbLocalTest != 0u) &&
        (ndsP2HbRejectLocal(w, dm->off, dm->size, radius, am->p0,
                            am->p1) != 0))
    {
        gNdsP2HbLocalRejects++;
        return 1;
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

#if NDS_P2_JOINT_RESIDENT
/* P2-2p8 (2026-10-04), owner ruling D13: fighter joint worlds stay in fixed
 * point end to end. The cache above holds one world per joint per latch
 * epoch; gmCollisionGetFighterPartsWorldPosition (src/import/
 * battleship_gmcollision.c) and the held item's 0x52 matrix
 * (src/port/renderer_adapter_matrix.c) read it in place of the float latch walk
 * (func_ovl2_800EDBA4 and the locals it latches), and write no FTParts latch.
 * A joint already latched by a float walk this epoch is read from its latch,
 * as before. Re-baselines the replay digest: the hitbox positions and the held
 * item's latches feed it. Same-ROM A/B word: 0 = the float paths everywhere. */
volatile u32 gNdsP2JointResident __attribute__((used, section(".data"))) = 1u;

/* Animation locks decline: gmCollisionSetMatrixNcs is not converted. */
static const NDSR2CfxMtx *ndsP2JointWorldOf(DObj *joint, NDSR2CfxMtx *scratch)
{
    const FTStruct *fp;
    NDSP2HbWorld *slot;

    if ((joint == NULL) || (joint == DOBJ_PARENT_NULL) ||
        (joint->parent_gobj == NULL))
    {
        return NULL;
    }
    fp = ftGetStruct(joint->parent_gobj);
    if ((fp == NULL) || (fp->is_use_animlocks != FALSE))
    {
        return NULL;
    }
    return ndsP2HbWorldOf(joint, scratch, &slot);
}

/* gmCollisionGetFighterPartsWorldPosition (gm/gmcollision.c:491): *vec carried
 * by the joint's world. The source carries it up the chain through each
 * joint's float local, latching the locals on the way. 1 = *vec written;
 * 0 = declined with *vec untouched (the caller runs the float body). */
int ndsP2JointWorldPosition(DObj *joint, Vec3f *vec)
{
    NDSR2CfxMtx scratch;
    const NDSR2CfxMtx *w;
    int32_t p[3];
    int64_t out[3];
    u32 col;

    if (ndsP2HbVec(p, vec) == 0)
    {
        return 0;
    }
    w = ndsP2JointWorldOf(joint, &scratch);
    if (w == NULL)
    {
        return 0;
    }
    for (col = 0u; col < 3u; col++)
    {
        out[col] = ndsR2CfxShr((int64_t)p[0] * w->r[0][col] +
                                   (int64_t)p[1] * w->r[1][col] +
                                   (int64_t)p[2] * w->r[2][col],
                               NDS_R2_CFX_ROT_BITS) +
                   (int64_t)w->t[col];
        if ((out[col] >= (int64_t)NDS_R2_CFX_POS_MAX) ||
            (out[col] <= -(int64_t)NDS_R2_CFX_POS_MAX))
        {
            return 0;
        }
    }
    vec->x = ndsR2CollisionFixedToF32(out[0], NDS_R2_CFX_POS_BITS);
    vec->y = ndsR2CollisionFixedToF32(out[1], NDS_R2_CFX_POS_BITS);
    vec->z = ndsR2CollisionFixedToF32(out[2], NDS_R2_CFX_POS_BITS);
    return 1;
}

/* lbcommon.c func_ovl0_800C9A38's branch for a held item on a non-root joint
 * without animation locks: the attach joint's local with its rows normalized,
 * each column c then divided by the length of the parent world's row c, the
 * result carried by the parent's world -- so the item keeps the joint's
 * orientation and drops every scale on the chain -- and the hitlag shuffle
 * added to the translation. Written as the Q20.12 matrix the GX takes
 * (m[r][c]: rotation rows 0-2, translation row 3, column 3 = 0, 0, 0, 1).
 * 0 = decline, nothing written (the caller runs the float builder). */
int ndsP2JointItemAttach(DObj *attach, const Vec2f *shuffle, s32 m[4][4])
{
    NDSR2CfxMtx scratch;
    NDSR2CfxMtx local;
    NDSR2CfxMtx f;
    NDSR2CfxMtx item;
    const NDSR2CfxMtx *w;
    int32_t inv_l[3];
    int32_t inv_w[3];
    int32_t shuffle_q12[2] = { 0, 0 };
    u32 row;
    u32 col;

    if ((attach == NULL) || (attach->parent == NULL) ||
        (attach->parent == DOBJ_PARENT_NULL))
    {
        return 0;
    }
    if (shuffle != NULL)
    {
        /* A few units (dFTDisplayMainShufflePositions); OVERFLOW is
         * INT32_MIN, which the magnitude test alone would pass. */
        shuffle_q12[0] = ndsP2HbToFixed(shuffle->x, NDS_R2_CFX_POS_BITS);
        shuffle_q12[1] = ndsP2HbToFixed(shuffle->y, NDS_R2_CFX_POS_BITS);
        if ((shuffle_q12[0] == NDS_R2_COLLISION_F32_OVERFLOW) ||
            (shuffle_q12[1] == NDS_R2_COLLISION_F32_OVERFLOW) ||
            (ndsR2CfxAbs32(shuffle_q12[0]) >= NDS_R2_CFX_POS_ONE * 64) ||
            (ndsR2CfxAbs32(shuffle_q12[1]) >= NDS_R2_CFX_POS_ONE * 64))
        {
            return 0;
        }
    }
    w = ndsP2JointWorldOf(attach->parent, &scratch);
    if ((w == NULL) || (ndsP2HbLocalFromDObj(&local, attach) == 0) ||
        (ndsR2CfxRowScales(&local, NULL, NULL, NULL, inv_l) == 0) ||
        (ndsR2CfxRowScales(w, NULL, NULL, NULL, inv_w) == 0))
    {
        return 0;
    }
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            /* A unit cell times 1/s (s >= 1/4 by the guard): within 4. */
            const int32_t unit = (int32_t)ndsR2CfxShr(
                (int64_t)local.r[row][col] * inv_l[row], NDS_R2_CFX_ROT_BITS);

            f.r[row][col] = (int32_t)ndsR2CfxShr((int64_t)unit * inv_w[col],
                                                 NDS_R2_CFX_ROT_BITS);
        }
        f.t[row] = local.t[row];
    }
    if (ndsR2CfxCompose(&item, w, &f) == 0)
    {
        return 0;
    }
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            m[row][col] = (item.r[row][col] + (1 << 13)) >> 14;
        }
        m[row][3] = 0;
    }
    m[3][0] = item.t[0] + shuffle_q12[0];
    m[3][1] = item.t[1] + shuffle_q12[1];
    m[3][2] = item.t[2];
    m[3][3] = NDS_R2_CFX_POS_ONE;
    return 1;
}
#endif /* NDS_P2_JOINT_RESIDENT */
