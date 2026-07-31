#ifndef NDS_R2_COLLISION_MTX_H
#define NDS_R2_COLLISION_MTX_H

/* R2-07 L7 -- the collision joint-transform kernel in 20.12 fixed point.
 *
 * L6 measured the over-gate frame and found it is a hit-detection frame: an
 * over-gate frame runs gmCollisionSetInvertMatrix 34 times and
 * func_ovl2_800ED490 40 times, against ZERO of each on a clean frame, and
 * 66.2% of its +510,390-cycle premium is soft-float. Those two functions carry
 * 238,426 cycles/frame of it. This header is the arithmetic that replaces them.
 *
 * The kernel lives in a header, not a .c, for the same reason nds_r2_sqrtf.h
 * does: scripts/check-r2-collision-mtx.ps1 compiles THIS code on the host and
 * compares it against the decomp's float original. A falsifier that can pass
 * while the ROM differs is not a falsifier.
 *
 * NOTHING CALLS THIS YET, AND IT IS NOT YET FIT TO. Collision decides hits, and
 * a wrong answer here is a hit that lands or does not, amplified by damage,
 * knockback and hitstun. The arithmetic gets proven against a bound first;
 * wiring it into gmcollision.c's entry points is a separate, later step -- and
 * a harder one, because the `#define` include seam renames a decomp definition
 * and its internal call sites together, so these two functions cannot simply be
 * swapped underneath their callers (board, R2-07 L7 SCOPED).
 *
 * CURRENT STATE: the falsifier is RED. Against the 0.0200 world-unit bound
 * borrowed from E64b/E65, compose reaches 0.0226 and invert 0.3706. Do not wire
 * this in until that is resolved -- either by showing 0.37 units is immaterial
 * against real hurtbox dimensions and real joint scales (it is ~1-2% of a
 * hurtbox, which is not obviously safe), or by widening the representation.
 * The residual is dominated by quantising the INPUT matrices to 1/4096, not by
 * the arithmetic here, so more fractional bits in this kernel alone will not
 * move it; the fix would be carrying the joint chain in fixed point end to end,
 * which is L7's real shape anyway.
 *
 * Format. 20.12 signed fixed point, matching the renderer's
 * NDSRendererMatrix20p12 so the two representations can eventually meet:
 * one unit = 1/4096, range +/-524288. SSB64's joint matrices are a rotation
 * scaled per row plus a translation, so rows 0-2 hold values around +/-scale
 * and row 3 holds world coordinates in the hundreds -- both comfortably inside
 * the range, with 12 fractional bits against E64b's accepted 0.0028 rad.
 *
 * Layout is the source's: m[row][col], row 3 is the translation, and the
 * implied fourth column is (0,0,0,1). Row-major with the translation in the
 * last ROW is the N64 convention the decomp uses; it is not the DS geometry
 * engine's column convention, and mixing them is a whole class of silent bug.
 */

/* Deliberately plain fixed-width types so the header compiles on the host with
 * no dependency on the port's type headers. */
#include <stdint.h>

#define NDS_R2_COLLISION_MTX_FRAC_BITS 12
#define NDS_R2_COLLISION_MTX_ONE (1 << NDS_R2_COLLISION_MTX_FRAC_BITS)

typedef struct NDSR2CollisionMtx
{
    int32_t m[4][3];
} NDSR2CollisionMtx;

/* Round-half-away-from-zero, matching ndsRendererRoundShiftS64 so the two
 * fixed-point families agree on the boundary case rather than differing by one
 * unit depending on which one produced a value. */
static inline int64_t ndsR2CollisionRoundShift(int64_t value, unsigned int shift)
{
    int64_t bias = (int64_t)1 << (shift - 1u);

    if (value < 0)
    {
        return -(((-value) + bias) >> shift);
    }
    return (value + bias) >> shift;
}

static inline int32_t ndsR2CollisionClamp(int64_t value)
{
    if (value > (int64_t)INT32_MAX) { return INT32_MAX; }
    if (value < (int64_t)INT32_MIN) { return INT32_MIN; }
    return (int32_t)value;
}

/* The 20.12 form of func_ovl2_800ED490: dst = lhs applied to rhs, where rhs is
 * the child's local transform and lhs the parent's accumulated world one.
 *
 * The source computes dst[r][c] = sum_k lhs[k][c] * rhs[r][k], with row 3
 * additionally picking up lhs[3][c] -- i.e. the translation is carried through
 * the rotation and then offset by the parent's own. Term order is preserved
 * exactly; only the arithmetic type changes.
 *
 * `aliases` matters: the two call sites in func_ovl2_800EDBA4 pass
 * current->mtx_translate as dst while lhs is a DIFFERENT parts' matrix, so dst
 * never aliases lhs there -- but a temp is used anyway, because the one case
 * that would alias is silent corruption rather than a compile error. */
static inline void ndsR2CollisionCompose(NDSR2CollisionMtx *dst,
                                         const NDSR2CollisionMtx *lhs,
                                         const NDSR2CollisionMtx *rhs)
{
    NDSR2CollisionMtx temp;
    unsigned int row;
    unsigned int col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            int64_t sum = (int64_t)lhs->m[0][col] * rhs->m[row][0] +
                          (int64_t)lhs->m[1][col] * rhs->m[row][1] +
                          (int64_t)lhs->m[2][col] * rhs->m[row][2];

            temp.m[row][col] = ndsR2CollisionClamp(
                ndsR2CollisionRoundShift(sum, NDS_R2_COLLISION_MTX_FRAC_BITS));
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        int64_t sum = (int64_t)lhs->m[0][col] * rhs->m[3][0] +
                      (int64_t)lhs->m[1][col] * rhs->m[3][1] +
                      (int64_t)lhs->m[2][col] * rhs->m[3][2];

        temp.m[3][col] = ndsR2CollisionClamp(
            ndsR2CollisionRoundShift(sum, NDS_R2_COLLISION_MTX_FRAC_BITS) +
            (int64_t)lhs->m[3][col]);
    }
    *dst = temp;
}

/* The 20.12 form of gmCollisionSetInvertMatrix: a 3x3 cofactor inverse plus the
 * inverse-rotated translation, used to carry a world point into a joint's local
 * frame for the box test.
 *
 * Returns 0 and leaves dst untouched when the source matrix is singular. The
 * decomp spins forever there (`while (TRUE) syDebugPrintf(...)`), which is the
 * same give-up-by-hanging shape as the allocator, and reproducing it would be
 * reproducing a defect. The caller decides what a degenerate joint means.
 *
 * Scaling. Cofactors are products of two 20.12 values, so they are 40.24 and
 * get shifted back to 20.12. The determinant is a further product, so the
 * reciprocal is formed at 24 fractional bits and applied as a 20.12 multiply --
 * carrying the extra bits through the divide is what keeps a scale near 1 from
 * losing its low bits twice. */
/* Rows 0-2 of the inverse -- R^-1 alone -- with row 3 carrying the FORWARD
 * translation t verbatim, copied not computed.
 *
 * This is not a matrix and must not be used as one. It is the descriptor
 * ndsR2CollisionWorldToLocal consumes, and the split is the whole point: the
 * inverse's translation row is -t.R^-1, and t is a world coordinate in the
 * hundreds while R^-1 has entries around 1/scale, so forming that product in
 * 20.12 commits a large intermediate's rounding error to storage, where nothing
 * later cancels it. Subtracting t from the query point first keeps every
 * quantity small: p - t is a few units by construction (the point being tested
 * is near the joint), so it is exact to the quantum, and the single multiply by
 * R^-1 that follows is the only rounding in the path.
 *
 * Returns 0 and leaves dst untouched when src is singular. */
static inline int ndsR2CollisionInvertFrame(NDSR2CollisionMtx *dst,
                                            const NDSR2CollisionMtx *src)
{
    const unsigned int shift = NDS_R2_COLLISION_MTX_FRAC_BITS;
    int64_t c[3][3];
    int64_t det;
    int64_t recip;
    NDSR2CollisionMtx out;
    unsigned int row;
    unsigned int col;

    /* Cofactors, in the source's sign convention: it builds them all positive
     * and negates six entries afterwards, which is the same matrix. */
    c[0][0] = (int64_t)src->m[1][1] * src->m[2][2] -
              (int64_t)src->m[1][2] * src->m[2][1];
    c[1][0] = (int64_t)src->m[1][0] * src->m[2][2] -
              (int64_t)src->m[1][2] * src->m[2][0];
    c[2][0] = (int64_t)src->m[1][0] * src->m[2][1] -
              (int64_t)src->m[1][1] * src->m[2][0];

    c[0][1] = (int64_t)src->m[0][1] * src->m[2][2] -
              (int64_t)src->m[0][2] * src->m[2][1];
    c[1][1] = (int64_t)src->m[0][0] * src->m[2][2] -
              (int64_t)src->m[0][2] * src->m[2][0];
    c[2][1] = (int64_t)src->m[0][0] * src->m[2][1] -
              (int64_t)src->m[0][1] * src->m[2][0];

    c[0][2] = (int64_t)src->m[0][1] * src->m[1][2] -
              (int64_t)src->m[0][2] * src->m[1][1];
    c[1][2] = (int64_t)src->m[0][0] * src->m[1][2] -
              (int64_t)src->m[0][2] * src->m[1][0];
    c[2][2] = (int64_t)src->m[0][0] * src->m[1][1] -
              (int64_t)src->m[0][1] * src->m[1][0];

    /* det = m00*c00 - m01*c10 + m02*c20, with the cofactors still at 24
     * fractional bits, so shift one factor's worth back out. */
    det = ndsR2CollisionRoundShift(
        (int64_t)src->m[0][0] * ndsR2CollisionRoundShift(c[0][0], shift) -
        (int64_t)src->m[0][1] * ndsR2CollisionRoundShift(c[1][0], shift) +
        (int64_t)src->m[0][2] * ndsR2CollisionRoundShift(c[2][0], shift),
        shift);
    if (det == 0)
    {
        return 0;
    }

    /* 1/det at 24 fractional bits. `det` is itself 20.12, so the numerator is
     * 2^(24+12), not 2^24: (2^36 / det_raw) = (1/det_real) * 2^24. Getting this
     * wrong by one shift's worth is not a precision wobble -- the falsifier
     * caught the 2^24 form producing 124 world units of error, because the
     * final shift then discarded all twelve fractional bits and every inverse
     * came back rounded to whole units. */
    recip = (((int64_t)1 << (shift * 3u)) + (det / 2)) / det;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            /* Carry the cofactor at its native 24 fractional bits rather than
             * rounding it to 12 first: the divide amplifies by 1/det, so a
             * half-unit lost here comes back multiplied by up to ~64 on the
             * small scales a deep joint chain reaches. Worst magnitude is
             * ~2^26 against a recip of ~2^30, so the product stays well inside
             * int64. */
            int64_t cell = c[row][col];

            /* The six negated entries: (1,0) (0,1) (2,1) (1,2) and, below,
             * the two translation terms. Matches the source's sign flips. */
            if (((row == 1u) && (col == 0u)) ||
                ((row == 0u) && (col == 1u)) ||
                ((row == 2u) && (col == 1u)) ||
                ((row == 1u) && (col == 2u)))
            {
                cell = -cell;
            }
            out.m[row][col] = ndsR2CollisionClamp(
                ndsR2CollisionRoundShift(cell * recip, shift * 3u));
        }
    }

    /* The forward translation, copied. No arithmetic means no error. */
    out.m[3][0] = src->m[3][0];
    out.m[3][1] = src->m[3][1];
    out.m[3][2] = src->m[3][2];

    *dst = out;
    return 1;
}

/* local = (p - t) . R^-1, the only thing gmcollision.c ever does with the
 * inverse: gmCollisionGetWorldPosition and gmCollisionTestRectangle both just
 * carry a world point into the joint's frame. `frame` comes from
 * ndsR2CollisionInvertFrame.
 *
 * Row/column convention is the source's throughout: forward is
 * world[c] = sum_k local[k]*M[k][c] + M[3][c], so the inverse contracts over
 * the same index. */
static inline void ndsR2CollisionWorldToLocal(int32_t out[3],
                                              const NDSR2CollisionMtx *frame,
                                              const int32_t p[3])
{
    /* Exact: both operands are 20.12 and the difference of two nearby world
     * points cannot overflow the range that held either of them. */
    const int32_t d0 = p[0] - frame->m[3][0];
    const int32_t d1 = p[1] - frame->m[3][1];
    const int32_t d2 = p[2] - frame->m[3][2];
    unsigned int col;

    for (col = 0u; col < 3u; col++)
    {
        int64_t sum = (int64_t)d0 * frame->m[0][col] +
                      (int64_t)d1 * frame->m[1][col] +
                      (int64_t)d2 * frame->m[2][col];

        out[col] = ndsR2CollisionClamp(
            ndsR2CollisionRoundShift(sum, NDS_R2_COLLISION_MTX_FRAC_BITS));
    }
}

#endif /* NDS_R2_COLLISION_MTX_H */
