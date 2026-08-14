#ifndef NDS_R2_COLLISION_FIXED_H
#define NDS_R2_COLLISION_FIXED_H

/* The fighter hurtbox narrow phase, end to end, in fixed point.
 *
 * WHAT THIS REPLACES, and why it is a CLUSTER and not a leaf. The cycle-6 band
 * attribution (artifacts/performance/2026-08-13_shdt-band-owner/BAND_OWNER.md)
 * named the 88-frame band that contains the gate arm's P95 frame: it is the
 * fighter world-transform + hurtbox-overlap chain, +67,230 tk/frame on those
 * frames, of which 65% is soft float a per-PC profiler charges to
 * __aeabi_fmul. The chain, with its band call rates:
 *
 *   func_ovl2_800ED490            14.50/fr   3x4 affine compose
 *   gmCollisionGetWorldPosition   26.49/fr   point x matrix
 *   gmCollisionSetInvertMatrix     9.02/fr   3x3 cofactor inverse
 *   gmCollisionTestRectangle      17.60/fr   3 fdiv + 2 transforms + clip
 *   func_ovl2_800EDE5C            17.60/fr   three sqrtf for the axis scales
 *   gmCollisionTransformMatrixAll 16.17/fr   local matrix from the DObj TRS
 *   lbCommonSin + lbCommonCos     64.69/fr
 *
 * R2-07 L7 already converted ONE of those (the inverse) and was reverted the
 * same night: it won 534 cycles/frame in SRC and lost 6,481 in FTR+STG, because
 * 2,332 bytes of ADDED ARM text cost 1.85 cycles/frame per byte and the float
 * body it duplicated stayed in the map. The lesson is in
 * include/nds/nds_r2_collision_mtx.h and it is a FOOTPRINT lesson, not an
 * arithmetic one: a fixed-point collision path pays only if it converts the
 * whole cluster and DELETES the float versions rather than sitting beside them.
 * So this header carries every stage of the chain, and the representation
 * crosses the float boundary exactly twice per joint per frame -- once in at the
 * animation's Vec3f, once out at FTParts::vec_scale -- instead of once per call.
 *
 * The float freeze over gmcollision, the mp and ftMain families and ftComputer
 * was lifted for this cluster by the owner on 2026-08-13.
 *
 * NOTHING CALLS THIS YET. It is compiled and unit-proven; wiring, the map
 * check that the float bodies left, and the gate A/B are a separate cycle.
 * src/port/nds_r2_collision_fixed.c instantiates the out-of-line ARM entry
 * points; scripts/check-r2-collision-fixed.c compiles THIS file on the host and
 * grades it against a transcription of the decomp float originals, so the
 * falsifier cannot pass while the ROM differs.
 *
 * ---------------------------------------------------------------------------
 * FORMATS, one per value class. Every one of these is a place a shift can be
 * wrong by twelve bits and still look plausible, so each carries its range.
 *
 *   class                    format  raw bound        why
 *   rotation rows M[0..2]    Q26     |cell| <= 4      L7's finding: the
 *                                                     dominant error is
 *                                                     quantising the INPUT, and
 *                                                     rows 0-2 span +/-scale, so
 *                                                     six integer bits is ample
 *                                                     and twenty-six buy
 *                                                     precision. Live scale is
 *                                                     1.1138-1.1199 (460
 *                                                     samples, L7 oracle).
 *   translation row M[3]     Q12     |t| < 2^17       world coordinates. The
 *                                                     stage's own bounds are
 *                                                     s16 fields (MPGroundData
 *                                                     map_bound_*), so 131,072
 *                                                     world units strictly
 *                                                     contains the reachable
 *                                                     range with 4x to spare --
 *                                                     no live coordinate can
 *                                                     decline on range.
 *   sin/cos                  Q15     |v| <= 2^15      gSYSinTable is u16 Q15
 *                                                     already; reading it as an
 *                                                     integer removes the
 *                                                     __aeabi_i2f + fmul that
 *                                                     lbCommonSin pays per call.
 *   row square s^2           Q26     1/16..16         guarded; feeds both the
 *                                                     inverse and vec_scale.
 *   1/s^2, 1/s               Q26     <= 16, <= 4      derived from s^2.
 *   axis scale s             Q12     <= 4             the value FTParts
 *                                                     ::vec_scale holds.
 *   points, sizes, offsets   Q12     |v| < 2^17       same class as M[3].
 *
 * Intermediate widths, so no reduction is silent:
 *   compose rot   Q26 x Q26 -> Q52 (int64, |.| <= 3*16*2^52 -> 2^58)  >> 26
 *   compose trans Q26 x Q12 -> Q38 (int64)                            >> 26
 *   s^2           Q26 x Q26 -> Q52 (int64, 3*16*2^52)                 >> 26
 *   1/s^2         2^52 / s2_raw                                       -> Q26
 *   inverse cell  Q26 x Q26 -> Q52 (int64)                            >> 26
 *   world->local  Q12 x Q26 -> Q38 (int64, |d|<2^30 x |inv|<2^28
 *                                   x3 -> 2^59.6, inside int64)       >> 26
 *   radius/scale  Q12 x Q26 -> Q38 (int64)                            >> 26
 *
 * ---------------------------------------------------------------------------
 * MEASURED, 2026-08-13, scripts/check-r2-collision-fixed.ps1. Every figure is
 * max world-unit error against the decomp float original over the live domain.
 *
 *   sine table, all 4,096 indices                        EXACT, 0
 *   sin/cos index arithmetic, 800,001 angles             0 mismatches
 *   isqrt64, 0..2^20 and every live s^2 raw value        0 failures
 *   forward chain, depth 6                     0.0014219  (bound 0.0200)
 *   forward chain, depth 12                    0.0018318
 *   frame, COFACTOR, depth 6, reach +/-64      0.0012375
 *   frame, COFACTOR, depth 12, reach +/-64     0.0016533
 *   frame, ROW-SCALED, depth 6, reach +/-64    0.1035117  REFUTED, 5.2x over
 *   vec_scale                                  0.0001223
 *   TestRectangle decisions, 200,000 cases     0 flips
 *   TestRectangle decisions, 100,000, depth 12 1 flip, at 0.00057 margin
 *
 * THE PHASE 4 SKETCH'S INVERSE IS REFUTED, and the reason is the GAME'S OWN
 * SINE TABLE, not fixed point. docs/optimization/OPTIMIZATION_IDEAS.md's Phase
 * 4 prescribes the row-scaled near-orthogonal inverse: SSB64's joint matrix is
 * a rotation with each ROW scaled, M = S.Q with Q orthonormal, so
 * M^-1[c][r] = M[r][c] / s_r^2 -- three reciprocals and nine multiplies against
 * the cofactor form's 61 float operations, with s_r^2 already in hand because
 * func_ovl2_800EDE5C needs its square root for vec_scale.
 *
 * That identity is exact only for exactly orthonormal Q. The falsifier measured
 * how orthonormal the game's actually is, in double, off the source's own
 * table:
 *
 *   relative row skew |row_i . row_j| / (s_i s_j)   max         mean
 *     one local matrix                              0.00157     0.00027
 *     after 6 composes                              0.00882     0.00146
 *     after 12 composes                             0.01049     0.00174
 *
 * gSYSinTable spans 0..PI inclusive over 2048 samples -- a one-sample stretch
 * the port's own shim documents as worth about 0.0016 against a true sine -- so
 * sin^2 + cos^2 is not 1 and the rows are up to 1% out of square after a joint
 * chain. The skew reaches the local coordinate as roughly skew * |p - t| / s,
 * which at a hurtbox reach of 64 units is 0.06 to 0.6 world units. Measured
 * 0.1035. The orthogonality guard below correctly declines 92% of live cases,
 * which is the same finding read from the other side: there is no tolerance
 * that both admits the game's matrices and holds the bound.
 *
 * So the CO-FACTOR form ships. It is the source's own arithmetic at Q26,
 * correct for any invertible M, and it measures 0.0012 -- sixteen times inside
 * the bound and, at reach +/-4096, more accurate than the f32 original it
 * replaces (0.0012 against the reference's own 0.0021 from exact). The
 * row-scaled form is kept here, unexported, purely so this refutation stays
 * reproducible.
 *
 * ---------------------------------------------------------------------------
 * ROW/COLUMN CONVENTION is the source's throughout, and mixing it with the DS
 * geometry engine's column convention is a whole class of silent bug:
 *
 *     world[c] = sum_k local[k] * M[k][c] + M[3][c]
 *
 * i.e. row-major with the translation in the last ROW. gm/gmcollision.c:196 is
 * the definition.
 */

#include <stddef.h>
#include <stdint.h>

/* ndsR2CollisionF32ToFixed / ndsR2CollisionFixedToF32 are the proven edge
 * conversions from R2-07 L7 -- exponent arithmetic on the IEEE bits rather than
 * a multiply and a float-to-int, because at ~64 cycles per soft-float multiply
 * the naive conversion hands the whole win back. Reused rather than restated;
 * scripts/check-r2-collision-mtx.ps1 already grades them. Everything from that
 * header is static inline, so the unused half costs nothing. */
#include <nds/nds_r2_collision_mtx.h>

#define NDS_R2_CFX_ROT_BITS  26
#define NDS_R2_CFX_POS_BITS  12
#define NDS_R2_CFX_TRIG_BITS 15

#define NDS_R2_CFX_ROT_ONE (INT32_C(1) << NDS_R2_CFX_ROT_BITS)
#define NDS_R2_CFX_POS_ONE (INT32_C(1) << NDS_R2_CFX_POS_BITS)

/* Domain guards. Each one is a range the arithmetic above is proven inside;
 * outside it the kernel writes nothing and returns 0, and the caller lets the
 * decomp float path have the joint. Declining is not a failure mode to be
 * avoided -- it is the reason a fixed-point path may narrow its domain at all. */
#define NDS_R2_CFX_ROT_MAX (INT32_C(4) * NDS_R2_CFX_ROT_ONE)
#define NDS_R2_CFX_POS_MAX (INT32_C(1) << 29)
#define NDS_R2_CFX_S2_MIN  (NDS_R2_CFX_ROT_ONE >> 4)
#define NDS_R2_CFX_S2_MAX  (INT32_C(16) * NDS_R2_CFX_ROT_ONE)

/* Orthogonality tolerance for the row-scaled inverse, as |row_i . row_j| in
 * Q26 against s_i * s_j in Q26 -- i.e. a relative skew of 2^-10. Set from the
 * error budget rather than from taste: the skew propagates into the local
 * coordinate as roughly skew * |p - t| / s, so at the reachable |p - t| of a
 * hurtbox test (tens of units) a 2^-10 skew is ~0.06 world units, which is over
 * the 0.020 bound -- meaning this tolerance is deliberately LOOSER than the
 * bound and the falsifier had to show the real skew was far inside it.
 *
 * It is not. The measured skew is 0.0088 after six composes, so this guard
 * declines 92% of live cases and the cases it admits are still over bound.
 * That is the refutation, and the constant is kept at the value that produced
 * it so the number can be reproduced. Read it as "how much slack the guard
 * gives", never as "how much error the result carries". */
#define NDS_R2_CFX_SKEW_SHIFT 10

/* Signed 64/32 division. Split out for the same reason L7 split its own: the
 * port may override it with the DS hardware divider (64/32 -> 32, ~36 cycles)
 * because libgcc's __aeabi_ldivmod is a bit-by-bit loop. Both truncate toward
 * zero, so the host falsifier grading the C form grades the same arithmetic.
 * The hardware unit is SHARED STATE and the default deliberately is not: these
 * kernels are pure functions, which is what makes charter 3.11/3.12 satisfied
 * by construction rather than by a guard. */
#ifndef NDS_R2_CFX_DIV64
#define NDS_R2_CFX_DIV64(numerator, denominator) \
    ((int32_t)((numerator) / (int64_t)(denominator)))
#endif

/* Integer square root of a 64-bit value, floor. Overridable for the same reason
 * as the divide (the DS has a hardware unit at ~26 cycles), and portable by
 * default. The default is the restoring digit-by-digit root: no division, no
 * table, exactly 32 iterations, and correct by the loop invariant
 * root^2 <= value at every step. */
#ifndef NDS_R2_CFX_ISQRT64
#define NDS_R2_CFX_ISQRT64(value) ndsR2CfxIsqrt64Portable(value)
#endif

typedef struct NDSR2CfxMtx
{
    int32_t r[3][3]; /* Q26, rows 0-2 of the source matrix */
    int32_t t[3];    /* Q12, row 3 of the source matrix */
} NDSR2CfxMtx;

/* The joint's world->local descriptor. This is NOT a matrix and must not be
 * used as one: row 3 carries the FORWARD translation t, copied rather than
 * computed.
 *
 * The split is the whole point and it is L7's measured lesson. A true inverse
 * matrix's translation row is -t.R^-1, and t is a world coordinate in the
 * hundreds while R^-1 has entries near 1/scale, so forming that product commits
 * a large intermediate's rounding to storage where nothing later cancels it.
 * Subtracting t from the query point FIRST keeps every quantity small: (p - t)
 * is a few units by construction, exact to the quantum, and the single multiply
 * that follows is the only rounding in the path. */
typedef struct NDSR2CfxFrame
{
    int32_t inv[3][3];    /* Q26. local[c] = sum_k (p - t)[k] * inv[k][c] */
    int32_t t[3];         /* Q12, copied from the forward matrix */
    int32_t scale[3];     /* Q12, |row_c| -- the value FTParts::vec_scale holds */
    int32_t inv_scale[3]; /* Q26, 1/|row_c| -- radius/scale with no divide */
} NDSR2CfxFrame;

/* ------------------------------------------------------------------------
 * Scalar helpers
 */

/* Round-half-up on an arithmetic right shift: one add and one shift.
 *
 * Not round-half-away-from-zero. That form needs a compare, a branch and a
 * 64-bit negate, and there are two dozen reductions in this file; the bias it
 * saves is half a quantum of 2^-27, which is four orders under the bound.
 * ndsR2CollisionRoundShift in the L7 header is the away-from-zero form and is
 * still used at the float edges, where the two representations have to agree on
 * the boundary case. */
static inline int64_t ndsR2CfxShr(int64_t value, unsigned int shift)
{
    return (value + ((int64_t)1 << (shift - 1u))) >> shift;
}

static inline int32_t ndsR2CfxAbs32(int32_t value)
{
    return (value < 0) ? -value : value;
}

/* floor(sqrt(value)) for value < 2^62. Restoring digit-by-digit: at each step
 * `root` holds the bits decided so far and `bit` the power of four under test,
 * and the invariant is that root is the largest value whose square is <= the
 * part of `value` consumed. No division and no table. */
static inline uint32_t ndsR2CfxIsqrt64Portable(uint64_t value)
{
    uint64_t remainder = value;
    uint64_t root = 0;
    uint64_t bit = (uint64_t)1 << 62;

    while (bit > remainder)
    {
        bit >>= 2;
    }
    while (bit != 0)
    {
        if (remainder >= (root + bit))
        {
            remainder -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)root;
}

/* ------------------------------------------------------------------------
 * Stage 1 -- lbCommonSin / lbCommonCos, without the float round trip
 *
 * gm/gmcollision.c:37 asks for six of these per local matrix. lb/lbcommon.c:321
 * indexes a table in 4096ths of a turn, mirrors the quarter with bit 0x400 and
 * negates the lower half with bit 0x800; the port's shim
 * (src/port/reloc_backend_compat_shims.c:13519) reads gSYSinTable, sys's own
 * 2048-entry u16 Q15 half wave, and multiplies by 1/32768 to hand back an f32.
 *
 * That last multiply and its __aeabi_i2f are pure loss here, because everything
 * downstream is about to become integer. These return the table entry itself.
 * The index arithmetic is byte-for-byte the shim's, INCLUDING cosine adding 90
 * degrees to the ANGLE before the multiply rather than 0x400 to the index after
 * it -- those are not the same function and the difference is visible in the
 * last table step.
 *
 * `table` is gSYSinTable. Passing it keeps this header host-compilable with no
 * dependency on the port's symbols, which is what lets the falsifier grade the
 * shipping code instead of a transcription of it. */
#define NDS_R2_CFX_RAD_TO_ID 651.8986206f
#define NDS_R2_CFX_DEG90_RAD 1.5707963268f

static inline int32_t ndsR2CfxTableQ15(const uint16_t *table, int32_t index)
{
    int32_t value = (int32_t)table[index & 0x7FF];

    return (index & 0x800) ? -value : value;
}

static inline int32_t ndsR2CfxSinQ15(const uint16_t *table, float angle)
{
    return ndsR2CfxTableQ15(table,
                            ((int32_t)(angle * NDS_R2_CFX_RAD_TO_ID)) & 0xFFF);
}

static inline int32_t ndsR2CfxCosQ15(const uint16_t *table, float angle)
{
    return ndsR2CfxTableQ15(
        table,
        ((int32_t)((angle + NDS_R2_CFX_DEG90_RAD) * NDS_R2_CFX_RAD_TO_ID)) &
            0xFFF);
}

/* ------------------------------------------------------------------------
 * Stage 2 -- gmCollisionTransformMatrixAll (gm/gmcollision.c:29) in fixed point
 *
 * Scope note, because the sibling is deliberately absent. gmCollisionSetMatrixNcs
 * (:82) builds the same matrix for the is_use_animlocks branch and additionally
 * divides column c by scale_mul[c], which is what can make the rows
 * non-orthogonal. It is NOT converted here and that is a measured cut, not an
 * oversight: the band table has gmCollisionTransformMatrixAll at 16.17 calls per
 * band frame and gmCollisionSetMatrixNcs nowhere in it. The animlocks branch
 * keeps the float path.
 *
 * Term order is the source's exactly; only the arithmetic type changes. The
 * rotation is accumulated at Q30 -- one bit of headroom over the Q15 x Q15
 * product, which peaks at exactly 2^30 -- and reduced to Q26 only after the
 * per-row scale multiply, so the scale never rounds twice.
 *
 * Returns 0 without writing when a scale or a translation leaves its guard. */
static inline int ndsR2CfxBuildLocal(NDSR2CfxMtx *dst, const uint16_t *table,
                                     const float rotate[3],
                                     const float scale[3],
                                     const float translate[3])
{
    const unsigned int rot_bits = NDS_R2_CFX_ROT_BITS;
    int64_t sinx = ndsR2CfxSinQ15(table, rotate[0]);
    int64_t cosx = ndsR2CfxCosQ15(table, rotate[0]);
    int64_t siny = ndsR2CfxSinQ15(table, rotate[1]);
    int64_t cosy = ndsR2CfxCosQ15(table, rotate[1]);
    int64_t sinz = ndsR2CfxSinQ15(table, rotate[2]);
    int64_t cosz = ndsR2CfxCosQ15(table, rotate[2]);
    int64_t rot[3][3]; /* Q30 */
    int32_t row_scale[3];
    NDSR2CfxMtx out;
    unsigned int row;
    unsigned int col;

    /* The four triple products are formed at Q45 and reduced ONCE, not
     * reduced to Q15 between the two multiplies. Reducing in the middle is a
     * whole quantum of the TABLE -- 2^-15 -- and the falsifier measured what
     * that costs after six composes: 0.0108 world units against a 0.0200
     * bound, i.e. most of the budget, spent on a shift that saves nothing
     * (the Q45 product fits int64 with eighteen bits to spare). */
    rot[0][0] = cosy * cosz;
    rot[0][1] = cosy * sinz;
    rot[0][2] = -siny * (INT64_C(1) << NDS_R2_CFX_TRIG_BITS);

    rot[1][0] = ndsR2CfxShr(sinx * siny * cosz, NDS_R2_CFX_TRIG_BITS) -
                (cosx * sinz);
    rot[1][1] = ndsR2CfxShr(sinx * siny * sinz, NDS_R2_CFX_TRIG_BITS) +
                (cosx * cosz);
    rot[1][2] = sinx * cosy;

    rot[2][0] = ndsR2CfxShr(cosx * siny * cosz, NDS_R2_CFX_TRIG_BITS) +
                (sinx * sinz);
    rot[2][1] = ndsR2CfxShr(cosx * siny * sinz, NDS_R2_CFX_TRIG_BITS) -
                (sinx * cosz);
    rot[2][2] = cosx * cosy;

    for (row = 0u; row < 3u; row++)
    {
        row_scale[row] = ndsR2CollisionF32ToFixed(scale[row], rot_bits);
        if ((row_scale[row] == NDS_R2_COLLISION_F32_OVERFLOW) ||
            (ndsR2CfxAbs32(row_scale[row]) > NDS_R2_CFX_ROT_MAX))
        {
            return 0;
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        out.t[col] = ndsR2CollisionF32ToFixed(translate[col],
                                              NDS_R2_CFX_POS_BITS);
        if ((out.t[col] == NDS_R2_COLLISION_F32_OVERFLOW) ||
            (ndsR2CfxAbs32(out.t[col]) >= NDS_R2_CFX_POS_MAX))
        {
            return 0;
        }
    }

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            /* Q30 x Q26 -> Q56, reduced to Q26. |rot| <= 2^30 and
             * |row_scale| <= 2^28, so the product peaks at 2^58. */
            int64_t cell = ndsR2CfxShr(rot[row][col] * (int64_t)row_scale[row],
                                       30u);

            if ((cell > (int64_t)NDS_R2_CFX_ROT_MAX) ||
                (cell < -(int64_t)NDS_R2_CFX_ROT_MAX))
            {
                return 0;
            }
            out.r[row][col] = (int32_t)cell;
        }
    }
    *dst = out;
    return 1;
}

/* ------------------------------------------------------------------------
 * Stage 3 -- func_ovl2_800ED490 (gm/gmcollision.c:208), the 3x4 affine compose
 *
 * dst[r][c] = sum_k lhs[k][c] * rhs[r][k], with row 3 additionally picking up
 * lhs[3][c]: the child's translation is carried through the parent's rotation
 * and then offset by the parent's own.
 *
 * A temp is used even though the two live call sites in func_ovl2_800EDBA4 pass
 * a dst that never aliases lhs, because the case that would alias is silent
 * corruption rather than a compile error. */
static inline int ndsR2CfxCompose(NDSR2CfxMtx *dst, const NDSR2CfxMtx *lhs,
                                  const NDSR2CfxMtx *rhs)
{
    const unsigned int rot_bits = NDS_R2_CFX_ROT_BITS;
    NDSR2CfxMtx out;
    unsigned int row;
    unsigned int col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            int64_t cell = ndsR2CfxShr(
                (int64_t)lhs->r[0][col] * rhs->r[row][0] +
                    (int64_t)lhs->r[1][col] * rhs->r[row][1] +
                    (int64_t)lhs->r[2][col] * rhs->r[row][2],
                rot_bits);

            if ((cell > (int64_t)NDS_R2_CFX_ROT_MAX) ||
                (cell < -(int64_t)NDS_R2_CFX_ROT_MAX))
            {
                return 0;
            }
            out.r[row][col] = (int32_t)cell;
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        /* Q26 x Q12 -> Q38, reduced to Q12, then the parent offset. */
        int64_t cell = ndsR2CfxShr((int64_t)lhs->r[0][col] * rhs->t[0] +
                                       (int64_t)lhs->r[1][col] * rhs->t[1] +
                                       (int64_t)lhs->r[2][col] * rhs->t[2],
                                   rot_bits) +
                       (int64_t)lhs->t[col];

        if ((cell >= (int64_t)NDS_R2_CFX_POS_MAX) ||
            (cell <= -(int64_t)NDS_R2_CFX_POS_MAX))
        {
            return 0;
        }
        out.t[col] = (int32_t)cell;
    }
    *dst = out;
    return 1;
}

/* ------------------------------------------------------------------------
 * Stage 4a -- func_ovl2_800EDE5C's axis scales, fused with the inverse
 *
 * The source computes three sqrtf of the row magnitudes for vec_scale (:482)
 * and, in a different function on a different latch, a 61-operation cofactor
 * inverse of the same matrix. Both want the same nine products. This computes
 * s^2 once and hands back s (Q12, for vec_scale), 1/s^2 (Q26, for the inverse)
 * and 1/s (Q26, so gmCollisionTestRectangle's three `radius / scale` divides
 * become multiplies -- those divides are 79% of the band's whole __aeabi_fdiv
 * premium).
 *
 * Returns 0 without writing when a row leaves the s^2 guard. */
static inline int ndsR2CfxRowScales(const NDSR2CfxMtx *src, int32_t s2_q26[3],
                                    int32_t inv_s2_q26[3], int32_t s_q12[3],
                                    int32_t inv_s_q26[3])
{
    const unsigned int rot_bits = NDS_R2_CFX_ROT_BITS;
    unsigned int row;

    for (row = 0u; row < 3u; row++)
    {
        int64_t sum = (int64_t)src->r[row][0] * src->r[row][0] +
                      (int64_t)src->r[row][1] * src->r[row][1] +
                      (int64_t)src->r[row][2] * src->r[row][2];
        int64_t s2 = ndsR2CfxShr(sum, rot_bits);
        uint32_t root;

        if ((s2 < (int64_t)NDS_R2_CFX_S2_MIN) ||
            (s2 > (int64_t)NDS_R2_CFX_S2_MAX))
        {
            return 0;
        }
        s2_q26[row] = (int32_t)s2;

        /* 2^52 / s2_raw is (1/s^2) at Q26. |s2_raw| >= 2^22 by the guard, so
         * the quotient is at most 2^30 and fits int32 -- the guard is what
         * makes that true, and L7 lost 160 world units once to a quotient that
         * silently did not fit. */
        inv_s2_q26[row] = NDS_R2_CFX_DIV64((int64_t)1 << (rot_bits * 2u), s2);

        /* s = sqrt(s^2). isqrt(s2_raw << 22) = sqrt(s^2 * 2^48) = s * 2^24. */
        root = NDS_R2_CFX_ISQRT64((uint64_t)s2 << 22);
        s_q12[row] = (int32_t)ndsR2CfxShr((int64_t)root, 12u);

        /* 1/s = s * (1/s^2): Q24 x Q26 -> Q50, reduced to Q26. Cheaper than a
         * second divide and no less accurate at this magnitude. */
        inv_s_q26[row] = (int32_t)ndsR2CfxShr(
            (int64_t)root * inv_s2_q26[row], 24u);
    }
    return 1;
}

/* ------------------------------------------------------------------------
 * Stage 4b -- the world->local frame, ROW-SCALED (OPTIMIZATION_IDEAS Phase 4)
 *
 * M = S.Q with Q orthonormal gives M^-1 = Q^T.S^-1, i.e.
 * M^-1[c][r] = M[r][c] / s_r^2. This stores it transposed into `inv` so the
 * consumer contracts over k:  local[c] = sum_k (p - t)[k] * inv[k][c].
 *
 * inv[k][c] = M^-1[k][c] = M[c][k] / s_c^2.
 *
 * THE GUARD. The identity is exact only for exactly orthonormal Q, and the
 * game's Q comes out of a quantised sine table, so the three row dot products
 * are checked against NDS_R2_CFX_SKEW_SHIFT before the result is accepted. A
 * declined joint takes the float path. Cheap: nine multiplies, and it reuses
 * s2 which is already in hand. */
static inline int ndsR2CfxMakeFrameRowScaled(NDSR2CfxFrame *dst,
                                             const NDSR2CfxMtx *src)
{
    const unsigned int rot_bits = NDS_R2_CFX_ROT_BITS;
    int32_t s2[3];
    int32_t inv_s2[3];
    NDSR2CfxFrame out;
    unsigned int i;
    unsigned int j;
    unsigned int c;

    for (c = 0u; c < 3u; c++)
    {
        if (ndsR2CfxAbs32(src->r[c][0]) > NDS_R2_CFX_ROT_MAX ||
            ndsR2CfxAbs32(src->r[c][1]) > NDS_R2_CFX_ROT_MAX ||
            ndsR2CfxAbs32(src->r[c][2]) > NDS_R2_CFX_ROT_MAX ||
            ndsR2CfxAbs32(src->t[c]) >= NDS_R2_CFX_POS_MAX)
        {
            return 0;
        }
    }
    if (ndsR2CfxRowScales(src, s2, inv_s2, out.scale, out.inv_scale) == 0)
    {
        return 0;
    }

    /* Orthogonality: |row_i . row_j| <= (s_i * s_j) >> SKEW_SHIFT. Both sides
     * are Q26 after the reduction, so the comparison is a plain integer one. */
    for (i = 0u; i < 3u; i++)
    {
        for (j = i + 1u; j < 3u; j++)
        {
            int64_t dot = ndsR2CfxShr((int64_t)src->r[i][0] * src->r[j][0] +
                                          (int64_t)src->r[i][1] * src->r[j][1] +
                                          (int64_t)src->r[i][2] * src->r[j][2],
                                      rot_bits);
            /* scale is Q12, so the product is Q24 and reaching Q26 is a LEFT
             * shift of two. Both sides of the compare are then Q26. */
            int64_t tolerance =
                (((int64_t)out.scale[i] * out.scale[j]) << 2) >>
                NDS_R2_CFX_SKEW_SHIFT;

            if (dot < 0) { dot = -dot; }
            if (dot > tolerance)
            {
                return 0;
            }
        }
    }

    for (c = 0u; c < 3u; c++)
    {
        unsigned int k;

        for (k = 0u; k < 3u; k++)
        {
            /* Q26 x Q26 -> Q52, reduced to Q26. |M[c][k]| <= s_c and
             * |1/s_c^2| <= 16, so |inv| <= 1/s_c <= 4. */
            out.inv[k][c] = (int32_t)ndsR2CfxShr(
                (int64_t)src->r[c][k] * inv_s2[c], rot_bits);
        }
        out.t[c] = src->t[c];
    }
    *dst = out;
    return 1;
}

/* ------------------------------------------------------------------------
 * Stage 4c -- the world->local frame, EXACT COFACTOR
 *
 * gmCollisionSetInvertMatrix's own arithmetic, at Q26, restructured to produce
 * the frame rather than a matrix (so -t.R^-1 is never formed). Correct for any
 * invertible M, orthogonal or not, at the cost of the cofactors: nine 2x2
 * determinants, one 3x3 determinant, one reciprocal, nine multiplies.
 *
 * This is the fallback the falsifier will promote to primary if the row-scaled
 * form's skew turns out to matter. The decomp spins forever on a singular
 * matrix (`while (TRUE) syDebugPrintf`), which is a give-up-by-hanging defect;
 * reproducing it would be reproducing the defect, so this returns 0 instead. */
static inline int ndsR2CfxMakeFrameCofactor(NDSR2CfxFrame *dst,
                                            const NDSR2CfxMtx *src)
{
    const unsigned int rot_bits = NDS_R2_CFX_ROT_BITS;
    int32_t s2[3];
    int32_t inv_s2[3];
    int32_t cof[3][3];
    NDSR2CfxFrame out;
    int64_t det;
    int32_t recip;
    unsigned int row;
    unsigned int col;
    unsigned int c;

    for (c = 0u; c < 3u; c++)
    {
        if (ndsR2CfxAbs32(src->r[c][0]) > NDS_R2_CFX_ROT_MAX ||
            ndsR2CfxAbs32(src->r[c][1]) > NDS_R2_CFX_ROT_MAX ||
            ndsR2CfxAbs32(src->r[c][2]) > NDS_R2_CFX_ROT_MAX ||
            ndsR2CfxAbs32(src->t[c]) >= NDS_R2_CFX_POS_MAX)
        {
            return 0;
        }
    }
    if (ndsR2CfxRowScales(src, s2, inv_s2, out.scale, out.inv_scale) == 0)
    {
        return 0;
    }

/* Q26 x Q26 -> Q52, straight back to Q26 so the next stage is another single
 * long multiply rather than a 64x64 one. L7 measured what happens otherwise:
 * carrying the inverse at Q30 made every product a 64x64 multiply, the function
 * came out at 583 ARM instructions, and it measured DEARER than the 295-
 * instruction soft-float original it replaced. */
#define NDS_R2_CFX_COFACTOR(ar, ac, br, bc, cr, cc, dr, dc)                   \
    (int32_t)ndsR2CfxShr(((int64_t)src->r[ar][ac] * src->r[br][bc]) -         \
                             ((int64_t)src->r[cr][cc] * src->r[dr][dc]),      \
                         rot_bits)

    cof[0][0] = NDS_R2_CFX_COFACTOR(1, 1, 2, 2, 1, 2, 2, 1);
    cof[1][0] = NDS_R2_CFX_COFACTOR(1, 0, 2, 2, 1, 2, 2, 0);
    cof[2][0] = NDS_R2_CFX_COFACTOR(1, 0, 2, 1, 1, 1, 2, 0);

    cof[0][1] = NDS_R2_CFX_COFACTOR(0, 1, 2, 2, 0, 2, 2, 1);
    cof[1][1] = NDS_R2_CFX_COFACTOR(0, 0, 2, 2, 0, 2, 2, 0);
    cof[2][1] = NDS_R2_CFX_COFACTOR(0, 0, 2, 1, 0, 1, 2, 0);

    cof[0][2] = NDS_R2_CFX_COFACTOR(0, 1, 1, 2, 0, 2, 1, 1);
    cof[1][2] = NDS_R2_CFX_COFACTOR(0, 0, 1, 2, 0, 2, 1, 0);
    cof[2][2] = NDS_R2_CFX_COFACTOR(0, 0, 1, 1, 0, 1, 1, 0);
#undef NDS_R2_CFX_COFACTOR

    det = ndsR2CfxShr(((int64_t)src->r[0][0] * cof[0][0]) -
                          ((int64_t)src->r[0][1] * cof[1][0]) +
                          ((int64_t)src->r[0][2] * cof[2][0]),
                      rot_bits);

    /* det is the product of the three row scales up to the row skew, so the
     * s^2 guard above already bounds it -- but only up to that skew, so it is
     * re-checked here rather than assumed. 2^52 / det_raw leaves int32 once
     * |det_raw| drops below 2^21. */
    if ((det < (int64_t)(INT32_C(1) << 21)) &&
        (det > -(int64_t)(INT32_C(1) << 21)))
    {
        return 0;
    }
    recip = NDS_R2_CFX_DIV64((int64_t)1 << (rot_bits * 2u), det);

    /* The source builds every cofactor positive and negates six entries
     * afterwards, which is the same matrix. Those six are (1,0) (0,1) (2,1)
     * (1,2) and the two translation terms it forms and this one does not. */
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            int32_t cell = cof[row][col];
            int64_t scaled;

            if (((row == 1u) && (col == 0u)) ||
                ((row == 0u) && (col == 1u)) ||
                ((row == 2u) && (col == 1u)) ||
                ((row == 1u) && (col == 2u)))
            {
                cell = -cell;
            }
            scaled = ndsR2CfxShr((int64_t)cell * recip, rot_bits);
            if ((scaled > (int64_t)NDS_R2_CFX_ROT_MAX) ||
                (scaled < -(int64_t)NDS_R2_CFX_ROT_MAX))
            {
                return 0;
            }
            out.inv[row][col] = (int32_t)scaled;
        }
    }
    for (c = 0u; c < 3u; c++)
    {
        out.t[c] = src->t[c];
    }
    *dst = out;
    return 1;
}

/* ------------------------------------------------------------------------
 * Stage 5 -- the two point transforms
 *
 * ndsR2CfxWorldToLocal is what gmCollisionGetWorldPosition does when it is
 * handed unk_dobjtrans_0x9C, which is every call the hurtbox test makes.
 * ndsR2CfxTransformPoint is the same function handed a FORWARD matrix, which is
 * what gmCollisionGetFighterPartsWorldPosition does.
 *
 * (p - t) is exact: both are Q12 and the difference of two points inside the
 * position guard cannot leave int32. */
static inline void ndsR2CfxWorldToLocal(int32_t out[3],
                                        const NDSR2CfxFrame *frame,
                                        const int32_t p[3])
{
    const int32_t d0 = p[0] - frame->t[0];
    const int32_t d1 = p[1] - frame->t[1];
    const int32_t d2 = p[2] - frame->t[2];
    unsigned int col;

    for (col = 0u; col < 3u; col++)
    {
        /* Q12 x Q26 -> Q38, reduced to Q12. |d| < 2^30 and |inv| <= 2^28, so
         * the three-term sum peaks near 2^60, inside int64. */
        out[col] = (int32_t)ndsR2CfxShr((int64_t)d0 * frame->inv[0][col] +
                                            (int64_t)d1 * frame->inv[1][col] +
                                            (int64_t)d2 * frame->inv[2][col],
                                        NDS_R2_CFX_ROT_BITS);
    }
}

static inline void ndsR2CfxTransformPoint(int32_t out[3],
                                          const NDSR2CfxMtx *mtx,
                                          const int32_t p[3])
{
    unsigned int col;

    for (col = 0u; col < 3u; col++)
    {
        out[col] = (int32_t)(ndsR2CfxShr((int64_t)p[0] * mtx->r[0][col] +
                                             (int64_t)p[1] * mtx->r[1][col] +
                                             (int64_t)p[2] * mtx->r[2][col],
                                         NDS_R2_CFX_ROT_BITS) +
                             (int64_t)mtx->t[col]);
    }
}

/* ------------------------------------------------------------------------
 * Stage 6 -- gmCollisionTestRectangle (gm/gmcollision.c:661)
 *
 * The attacker's swept segment against the joint's local box. Structure is the
 * source's exactly: build the half extents, carry both endpoints into the
 * joint's frame, subtract the hurtbox offset, then Cohen-Sutherland clip in X
 * and Y and finish with a Z overlap test.
 *
 * `inv_scale` is Q26 1/s from ndsR2CfxRowScales, so `radius / scale` -- the
 * three fdiv the source pays per call, and 79% of the band's whole __aeabi_fdiv
 * premium -- becomes three multiplies of values already in hand.
 *
 * `frame` may be NULL, matching the source's `if (mtx != NULL)`: the item path
 * (gmCollisionCheckFighterAttackItemDamageCollide) passes no matrix.
 *
 * Returns 1 (hit), 0 (miss), or NDS_R2_CFX_DECLINE. The clip's interpolation
 * divides by the segment's extent along the axis being clipped, and that
 * extent cannot be zero when the branch is reachable -- two endpoints with the
 * same X share an X outcode bit, so `flags_a & flags_b` returns FALSE first --
 * but the check is here anyway, because a division by zero is a hang and the
 * cost of proving it unreachable at runtime is one compare. */
#define NDS_R2_CFX_DECLINE (-1)

static inline uint32_t ndsR2CfxOutcodeXY(const int32_t p[3],
                                         const int32_t center[3])
{
    uint32_t flags = 0u;

    if (p[0] < -center[0]) { flags |= 1u; }
    if (p[0] > center[0]) { flags |= 2u; }
    if (p[1] < -center[1]) { flags |= 4u; }
    if (p[1] > center[1]) { flags |= 8u; }
    return flags;
}

static inline uint32_t ndsR2CfxOutcodeZ(const int32_t p[3],
                                        const int32_t center[3])
{
    uint32_t flags = 0u;

    if (p[2] < -center[2]) { flags |= 1u; }
    if (p[2] > center[2]) { flags |= 2u; }
    return flags;
}

static inline int ndsR2CfxTestRectangle(const int32_t pos_curr[3],
                                        const int32_t pos_prev[3],
                                        int32_t radius,
                                        int is_transfer,
                                        const NDSR2CfxFrame *frame,
                                        const int32_t offset[3],
                                        const int32_t size[3],
                                        const int32_t inv_scale[3])
{
    int32_t center[3];
    int32_t a[3];
    int32_t b[3];
    int32_t clipped[3];
    int32_t dist[3];
    uint32_t flags_a;
    uint32_t flags_b;
    unsigned int axis;
    unsigned int guard;

    for (axis = 0u; axis < 3u; axis++)
    {
        /* Q12 + (Q12 x Q26 >> 26). */
        center[axis] = size[axis] +
                       (int32_t)ndsR2CfxShr((int64_t)radius * inv_scale[axis],
                                            NDS_R2_CFX_ROT_BITS);
    }

    if (is_transfer)
    {
        if (frame != NULL)
        {
            ndsR2CfxWorldToLocal(a, frame, pos_curr);
        }
        else
        {
            a[0] = pos_curr[0]; a[1] = pos_curr[1]; a[2] = pos_curr[2];
        }
        for (axis = 0u; axis < 3u; axis++)
        {
            a[axis] -= offset[axis];
            if ((a[axis] < -center[axis]) || (a[axis] > center[axis]))
            {
                return 0;
            }
        }
        return 1;
    }

    if (frame != NULL)
    {
        ndsR2CfxWorldToLocal(a, frame, pos_curr);
        ndsR2CfxWorldToLocal(b, frame, pos_prev);
    }
    else
    {
        for (axis = 0u; axis < 3u; axis++)
        {
            a[axis] = pos_curr[axis];
            b[axis] = pos_prev[axis];
        }
    }
    for (axis = 0u; axis < 3u; axis++)
    {
        a[axis] -= offset[axis];
        b[axis] -= offset[axis];
        dist[axis] = b[axis] - a[axis];
    }

    flags_a = ndsR2CfxOutcodeXY(a, center);
    flags_b = ndsR2CfxOutcodeXY(b, center);

    /* Cohen-Sutherland terminates in at most four clips; the cap turns a
     * hypothetical non-terminating case into a decline rather than a hang. */
    for (guard = 0u; guard <= 8u; guard++)
    {
        uint32_t flags_main;
        unsigned int fixed_axis;
        unsigned int other0;
        unsigned int other1;
        int64_t ratio_num;

        if ((flags_a == 0u) && (flags_b == 0u))
        {
            break;
        }
        if ((flags_a & flags_b) != 0u)
        {
            return 0;
        }
        if (guard == 8u)
        {
            return NDS_R2_CFX_DECLINE;
        }
        flags_main = (flags_a != 0u) ? flags_a : flags_b;

        if ((flags_main & 3u) != 0u)
        {
            fixed_axis = 0u; other0 = 1u; other1 = 2u;
            clipped[0] = (flags_main & 1u) ? -center[0] : center[0];
        }
        else
        {
            fixed_axis = 1u; other0 = 0u; other1 = 2u;
            clipped[1] = (flags_main & 4u) ? -center[1] : center[1];
        }
        if (dist[fixed_axis] == 0)
        {
            return NDS_R2_CFX_DECLINE;
        }
        /* (target - a) / dist, applied to the other two components. Forming
         * the numerator at Q24 before the divide keeps the quotient's twelve
         * fractional bits; dividing first would discard them. */
        ratio_num = (int64_t)(clipped[fixed_axis] - a[fixed_axis]);
        clipped[other0] =
            (int32_t)(NDS_R2_CFX_DIV64(ratio_num * dist[other0],
                                       dist[fixed_axis]) +
                      a[other0]);
        clipped[other1] =
            (int32_t)(NDS_R2_CFX_DIV64(ratio_num * dist[other1],
                                       dist[fixed_axis]) +
                      a[other1]);

        if (flags_main == flags_a)
        {
            a[0] = clipped[0]; a[1] = clipped[1]; a[2] = clipped[2];
            flags_a = ndsR2CfxOutcodeXY(a, center);
        }
        else
        {
            b[0] = clipped[0]; b[1] = clipped[1]; b[2] = clipped[2];
            flags_b = ndsR2CfxOutcodeXY(b, center);
        }
    }

    return ((ndsR2CfxOutcodeZ(a, center) & ndsR2CfxOutcodeZ(b, center)) != 0u)
               ? 0
               : 1;
}

/* ------------------------------------------------------------------------
 * The out-of-line ARM entry points, defined in src/port/nds_r2_collision_fixed.c
 * and built -marm. Callers may be Thumb; the interworking branch is free next
 * to what these replace, and the alternative -- inlining 64-bit products into a
 * Thumb translation unit -- turns every one of them into __aeabi_lmul.
 *
 * ndsR2CfxMakeFrameRowScaled is deliberately absent: it is refuted, and an
 * entry point is how a refuted kernel gets wired by accident. */
int ndsR2CollisionFixedBuildLocal(NDSR2CfxMtx *dst, const uint16_t *sin_table,
                                  const float rotate[3], const float scale[3],
                                  const float translate[3]);
int ndsR2CollisionFixedCompose(NDSR2CfxMtx *dst, const NDSR2CfxMtx *lhs,
                               const NDSR2CfxMtx *rhs);
int ndsR2CollisionFixedMakeFrame(NDSR2CfxFrame *dst, const NDSR2CfxMtx *src);
void ndsR2CollisionFixedWorldToLocal(int32_t out[3],
                                     const NDSR2CfxFrame *frame,
                                     const int32_t point[3]);
void ndsR2CollisionFixedTransformPoint(int32_t out[3], const NDSR2CfxMtx *mtx,
                                       const int32_t point[3]);
int ndsR2CollisionFixedTestRectangle(const int32_t pos_curr[3],
                                     const int32_t pos_prev[3], int32_t radius,
                                     int is_transfer,
                                     const NDSR2CfxFrame *frame,
                                     const int32_t offset[3],
                                     const int32_t size[3],
                                     const int32_t inv_scale[3]);

#endif /* NDS_R2_COLLISION_FIXED_H */
