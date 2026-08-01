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
 * NOTHING CALLS THIS, AND THAT IS A MEASURED DECISION, not an unfinished one.
 * ndsR2CollisionInvertMatrix44 WAS wired into the eight gmCollisionCheck* entry
 * points on 2026-07-31 and reverted the same night: on a deterministic harness
 * (two control runs bit-identical in every bucket) it won 534 cycles/frame in
 * the SRC bucket and lost 6,481 in FTR and STG, which contain no collision code
 * at all. The loss scales with the code and not with the work -- 2,332 bytes of
 * ARM text cost +4,264 FTR mean, 1,840 bytes cost +3,434, i.e. 1.85
 * cycles/frame per byte both times. See the board, "R2-07 L7 WIRED, MEASURED,
 * REVERTED", before wiring any of this again: the conclusion there is that a
 * fixed-point collision path only pays if it converts the compose as well and
 * DELETES the float versions rather than sitting beside them.
 *
 * Kept because the arithmetic is proven and the next attempt starts from it.
 *
 * CURRENT STATE, from scripts/check-r2-collision-mtx.ps1 (bound 0.0200 world
 * units, borrowed from E64b/E65; same binary and RNG stream across arms):
 *
 *   domain                       compose    frame    matrix
 *   near-unit 0.90-1.10 (gated)  0.018184  0.014758  0.000283   GREEN
 *   moderate  0.50-1.50          0.019064  0.069663  0.000488
 *   conservative 0.25-2.00       0.024414  0.367933  0.000900
 *
 * 'frame' is ndsR2CollisionInvertFrame, which reads the whole matrix at 20.12
 * and dodges the translation amplifier by never forming -t.R^-1. 'matrix' is
 * ndsR2CollisionInvertMatrix44, which forms it and beats the frame everywhere
 * anyway, because it reads the ROTATION BLOCK AT 6.26. That is the whole
 * lesson of this file: the dominant error was never the output rounding, it was
 * quantising the input, and rows 0-2 of a joint matrix span +/-scale so they
 * never needed twenty integer bits.
 *
 * MEASURED ON THE RUNNING GAME 2026-07-31, which is what that clearance was
 * waiting on (NDS_R2_COLLISION_L7_ORACLE, 460 samples over a natural mode-163
 * match, every one a joint collision actually inverted that frame):
 *
 *   real joint scale         1.1138 - 1.1199   (ScaleMin/MaxQ12 4562 - 4587)
 *   deviation, probe  1 unit  0.00049 world units   (MaxDevQ12 2)
 *   deviation, probe  4 units 0.00122               (MaxDevQ12 5)
 *   deviation, probe 16 units 0.00513               (MaxDevQ12 21)
 *   over the 0.0200 bound     0 of 460
 *   singular joints           0
 *
 * Two things settled. The scale domain is a SINGLE scale, ~1.12, spanning
 * 0.006 -- so the 0.25-2.00 sweep that reads 0.427738 is not a domain SSB64
 * visits, and the gated 0.90-1.10 sweep is near-right but centred slightly low;
 * it should be re-centred on 1.11-1.12. And the synthetic figure is
 * PESSIMISTIC: 0.016609 gated against 0.00513 measured, so the real margin is
 * about 4x the bound at the furthest probe rather than the 1.2x the falsifier
 * implies. The arithmetic is cleared.
 *
 * THE HOOK, since it was built and proven and should not be re-derived.
 * gmCollisionSetInvertMatrix cannot be intercepted: the `#define` include seam
 * renames a decomp definition and its internal call sites together, and its
 * only caller (func_ovl2_800EDE00) is in the same file, as are that function's
 * nine callers. The first externally-visible ring is the eight
 * gmCollisionCheck* entry points plus func_ovl2_800EE018. Wrapping those and
 * filling unk_dobjtrans_0x9C before delegating makes the decomp's float prepare
 * early-return on its own unk_dobjtrans_0x7 latch -- same joints, same order,
 * no speculative work, every hit-test decision still in decomp code. Measured
 * engagement over 128 frames: 691 fills, 0 declines, 41 already-prepared.
 *
 * Earlier revisions of this comment quoted 0.0226 compose and 0.3706 invert,
 * both stale by a revision, because the harness this file names did not exist
 * and the numbers were pasted in by hand. It exists now and dev-fast runs it.
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

/* ------------------------------------------------------------------------
 * The wired form: a FULL inverse matrix, in the layout the decomp's consumers
 * already read.
 *
 * ndsR2CollisionInvertFrame above is the better representation and it is what
 * the falsifier graded, but it cannot be wired without rewriting every consumer
 * of unk_dobjtrans_0x9C, because the frame is not a matrix. This one produces
 * the matrix, so gmCollisionGetWorldPosition and gmCollisionTestRectangle keep
 * working untouched -- collision decisions stay entirely in decomp code.
 *
 * THE AMPLIFIER IS THE INPUT, NOT THE OUTPUT. The frame form exists because
 * the inverse's translation row is -t.R^-1, and t is a world coordinate in the
 * hundreds, so it multiplies R^-1's error by ~400. The first attempt at this
 * matrix form assumed the fix was to keep R^-1 unrounded until after that
 * multiply; the falsifier measured 0.132935 anyway, right back at the frame's
 * old 0.126987. Carrying more bits forward cannot help, because the error is
 * already committed at the INPUT: quantising the rotation block to 20.12 puts
 * 2^-13 into every cell, the cofactors carry it into R^-1, and t multiplies it
 * by 400. The information was gone before the first multiply.
 *
 * So the rotation block is read at 6.26 instead. Rows 0-2 of a joint matrix are
 * a rotation scaled per row -- the measured live domain is a single scale of
 * 1.114-1.120 -- so two integer bits are ample and the other twenty-four buy
 * precision: 2^-27 * 400 is 3e-6 against the 0.0200 bound. Row 3 stays 20.12
 * because it holds world coordinates in the hundreds and needs the range. The
 * two rows of the same matrix are therefore in DIFFERENT formats on purpose; a
 * joint whose rotation block exceeds +/-32 is declined, not clamped.
 *
 * Scale chain, since every one of these is a place a shift can be wrong by
 * twelve bits and still look plausible (the frame form lost 124 world units to
 * exactly that):
 *
 *   rows 0-2 in        Q26          cofactor = product of two    -> Q52
 *   cofactor rounded   Q26          determinant = m * c          -> Q52 -> Q26
 *   recip = 2^56 / D                                             -> Q30
 *   cofactor rounded   Q30, times recip                          -> Q60 -> Q30
 *   stored rows 0-2    Q30 >> 18                                 -> Q12
 *   row 3   = -sum t(Q12) * inv(Q30)                             -> Q42 -> Q12
 */
#define NDS_R2_COLLISION_MTX_ROT_BITS 26
#define NDS_R2_COLLISION_MTX_INV_BITS 26

/* EVERY INTERMEDIATE FITS IN INT32, AND THAT IS THE PERFORMANCE DESIGN, not a
 * neatness one. The first wired revision carried the inverse at Q30, which does
 * not fit, so `cofactor * recip` and `t * inv` became 64x64 multiplies -- three
 * MULs and two adds each instead of one SMULL -- and the whole function came
 * out at 583 ARM instructions and measured +50,368 WORK-H P95, i.e. DEARER than
 * the 295-instruction soft-float original it replaces. Q26 keeps both operands
 * inside int32 so each product is a single SMULL, and it costs nothing that
 * matters: 2^-27 against a 0.0200 bound.
 *
 * Same reasoning for the shifts. The internal reductions truncate rather than
 * round-half-away-from-zero, because a rounded 64-bit shift is a compare, a
 * branch, a 64-bit negate, a 64-bit add and a 64-bit shift -- about ten
 * instructions, and there are two dozen of them. Truncation biases by half a
 * quantum of 2^-27; the falsifier grades the result either way. */

/* Signed truncating division. The port overrides this with the DS hardware
 * divider (64/32 -> 32, ~36 cycles) because libgcc's __aeabi_ldivmod is a
 * bit-by-bit loop -- and one of those per call is a large fraction of the
 * budget this kernel exists to save. Both truncate toward zero, so the host
 * falsifier grading the C form grades the same arithmetic. */
#ifndef NDS_R2_COLLISION_DIV64
#define NDS_R2_COLLISION_DIV64(numerator, denominator) \
    ((int32_t)((numerator) / (int64_t)(denominator)))
#endif

/* Round-half-away-from-zero float -> 20.12, by exponent arithmetic on the IEEE
 * bits rather than `(int)(v * 4096 + 0.5f)`.
 *
 * This is not micro-optimisation for its own sake: the whole point of the
 * kernel is that a soft-float multiply costs ~64 cycles here (L6: 238,426
 * cycles over 74 calls), and the naive conversion spends two of them per cell.
 * Twelve cells in and twelve out would be ~3,000 cycles of conversion against
 * a ~3,200-cycle call -- it would hand back the entire win. The integer form is
 * ~15 instructions.
 *
 * Zero, denormals and anything below half a quantum return 0. Values past the
 * 20.12 range saturate, which is the same fail-safe ndsR2CollisionClamp uses;
 * SSB64 joint matrices do not reach it (rows 0-2 are ~scale, row 3 is world
 * coordinates in the hundreds). */
#define NDS_R2_COLLISION_F32_OVERFLOW INT32_MIN

static inline int32_t ndsR2CollisionF32ToFixed(float value,
                                               unsigned int frac_bits)
{
    uint32_t bits;
    int32_t exponent;
    uint32_t mantissa;
    int32_t shift;
    uint32_t magnitude;

    __builtin_memcpy(&bits, &value, sizeof(bits));
    exponent = (int32_t)((bits >> 23) & 0xFFu);
    if (exponent == 0)
    {
        return 0; /* zero or denormal: below the quantum either way */
    }
    if (exponent == 0xFF)
    {
        return NDS_R2_COLLISION_F32_OVERFLOW; /* inf or NaN */
    }
    mantissa = (bits & 0x7FFFFFu) | 0x800000u;

    /* value = mantissa * 2^(exponent - 127 - 23), so the fixed magnitude is
     * mantissa >> (23 + 127 - frac_bits - exponent). */
    shift = (int32_t)(23u + 127u - frac_bits) - exponent;
    if (shift >= 25)
    {
        return 0; /* strictly under half a quantum */
    }
    if (shift <= 0)
    {
        /* mantissa occupies 24 bits, so a left shift past 7 leaves the
         * signed 32-bit range. Report it rather than wrap: the caller
         * declines the joint and the decomp's float path takes it. */
        if (shift <= -7)
        {
            return NDS_R2_COLLISION_F32_OVERFLOW;
        }
        magnitude = mantissa << (unsigned int)(-shift);
    }
    else
    {
        magnitude = (mantissa + (1u << (unsigned int)(shift - 1))) >>
                    (unsigned int)shift;
    }
    if (magnitude > 0x7FFFFFFFu)
    {
        return NDS_R2_COLLISION_F32_OVERFLOW;
    }
    return (bits & 0x80000000u) ? -(int32_t)magnitude : (int32_t)magnitude;
}

static inline int32_t ndsR2CollisionF32ToQ12(float value)
{
    return ndsR2CollisionF32ToFixed(value, NDS_R2_COLLISION_MTX_FRAC_BITS);
}

/* Fixed -> float at any fraction width, round-to-nearest-even. CLZ is a single
 * ARMv5TE instruction, so this is a handful of integer ops against ~80 cycles
 * for __aeabi_i2f plus __aeabi_fmul.
 *
 * The fraction width is a parameter and not fixed at 12 for a precision
 * reason, not a tidiness one: the destination is a float, whose 24-bit mantissa
 * resolves an inverse cell to 2^-24, and rounding to 20.12 on the way out would
 * throw away eleven of those bits. The consumer multiplies each cell by a world
 * coordinate of ~400, so 2^-13 there costs 0.05 world units -- two and a half
 * times the whole bound, from the STORE alone. Rows 0-2 are therefore converted
 * from the Q30 intermediate and never pass through Q12. */
static inline float ndsR2CollisionFixedToF32(int64_t value,
                                             unsigned int frac_bits)
{
    uint32_t sign;
    uint64_t magnitude;
    int32_t exponent;
    uint32_t mantissa;
    uint32_t bits;
    float out;

    if (value == 0)
    {
        return 0.0f;
    }
    sign = (value < 0) ? 0x80000000u : 0u;
    magnitude = (value < 0) ? (uint64_t)(-value) : (uint64_t)value;

    exponent = 63 - (int32_t)__builtin_clzll(magnitude);
    if (exponent > 23)
    {
        unsigned int drop = (unsigned int)(exponent - 23);
        uint64_t half = (uint64_t)1 << (drop - 1u);
        uint64_t remainder = magnitude & ((half << 1) - (uint64_t)1);
        uint32_t rounded = (uint32_t)(magnitude >> drop);

        if ((remainder > half) || ((remainder == half) && (rounded & 1u)))
        {
            rounded++;
            if (rounded == (1u << 24))
            {
                rounded >>= 1;
                exponent++;
            }
        }
        mantissa = rounded & 0x7FFFFFu;
    }
    else
    {
        mantissa = (uint32_t)(magnitude << (unsigned int)(23 - exponent)) &
                   0x7FFFFFu;
    }
    bits = sign |
           ((uint32_t)(exponent - (int32_t)frac_bits + 127) << 23) | mantissa;
    __builtin_memcpy(&out, &bits, sizeof(out));
    return out;
}

static inline float ndsR2CollisionQ12ToF32(int32_t q)
{
    return ndsR2CollisionFixedToF32((int64_t)q, NDS_R2_COLLISION_MTX_FRAC_BITS);
}

/* The fixed-point replacement for gmCollisionSetInvertMatrix, float in and
 * float out so that every consumer of unk_dobjtrans_0x9C is untouched. Only
 * columns 0-2 are written, matching the source, which leaves column 3 alone.
 *
 * Returns 0 and writes nothing when the source is singular or outside the
 * declared domain; the caller then lets the decomp's float path have the joint.
 * The decomp spins forever on a singular matrix, and reproducing that would be
 * reproducing a defect. */
static inline int ndsR2CollisionInvertMatrix44(float dst[4][4],
                                               const float src[4][4])
{
    const unsigned int rot_bits = NDS_R2_COLLISION_MTX_ROT_BITS;
    const unsigned int inv_bits = NDS_R2_COLLISION_MTX_INV_BITS;
    const unsigned int frac_bits = NDS_R2_COLLISION_MTX_FRAC_BITS;
    int32_t r[3][3];
    int32_t t[3];
    int32_t c[3][3];
    int32_t inv[3][3];
    int64_t row3[3];
    int32_t det;
    int32_t recip;
    unsigned int row;
    unsigned int col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            r[row][col] = ndsR2CollisionF32ToFixed(src[row][col], rot_bits);
            if (r[row][col] == NDS_R2_COLLISION_F32_OVERFLOW)
            {
                return 0; /* rotation cell past +/-32 */
            }
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        t[col] = ndsR2CollisionF32ToFixed(src[3][col], frac_bits);
        /* Row 3 forms t * inv at Q30 and int64 must hold it: |t_raw| < 2^24
         * is |t| < 4096 world units, comfortably past any SSB64 blast zone. */
        if ((t[col] == NDS_R2_COLLISION_F32_OVERFLOW) ||
            (t[col] >= (int32_t)(1 << 24)) || (t[col] <= -(int32_t)(1 << 24)))
        {
            return 0;
        }
    }

    /* Cofactors, in the source's sign convention: it builds them all positive
     * and negates six entries afterwards, which is the same matrix. Q26 times
     * Q26 is Q52, reduced straight back to Q26 so the next stage is another
     * SMULL rather than a 64-bit multiply. Each product is one SMULL and each
     * reduction one arithmetic shift pair. */
#define NDS_R2_COFACTOR(ar, ac, br, bc, cr, cc, dr, dc)                      \
    (int32_t)((((int64_t)r[ar][ac] * r[br][bc]) -                            \
               ((int64_t)r[cr][cc] * r[dr][dc])) >> rot_bits)

    c[0][0] = NDS_R2_COFACTOR(1, 1, 2, 2, 1, 2, 2, 1);
    c[1][0] = NDS_R2_COFACTOR(1, 0, 2, 2, 1, 2, 2, 0);
    c[2][0] = NDS_R2_COFACTOR(1, 0, 2, 1, 1, 1, 2, 0);

    c[0][1] = NDS_R2_COFACTOR(0, 1, 2, 2, 0, 2, 2, 1);
    c[1][1] = NDS_R2_COFACTOR(0, 0, 2, 2, 0, 2, 2, 0);
    c[2][1] = NDS_R2_COFACTOR(0, 0, 2, 1, 0, 1, 2, 0);

    c[0][2] = NDS_R2_COFACTOR(0, 1, 1, 2, 0, 2, 1, 1);
    c[1][2] = NDS_R2_COFACTOR(0, 0, 1, 2, 0, 2, 1, 0);
    c[2][2] = NDS_R2_COFACTOR(0, 0, 1, 1, 0, 1, 1, 0);
#undef NDS_R2_COFACTOR

    /* det = m00*c00 - m01*c10 + m02*c20, Q26 by Q26 reduced to Q26. */
    det = (int32_t)((((int64_t)r[0][0] * c[0][0]) -
                     ((int64_t)r[0][1] * c[1][0]) +
                     ((int64_t)r[0][2] * c[2][0])) >> rot_bits);
    /* det is Q26, so 2^52 / det_raw is (1/det) at Q26 -- which leaves int32
     * once |det_raw| drops below 2^21, i.e. a determinant under 1/32. That is
     * not theoretical: the conservative 0.25-2.00 sweep hit it and the
     * falsifier reported 160.755318 world units before this test existed,
     * because the DS hardware divider returns a truncated 32-bit result and
     * has no way to say "did not fit". Live play measures det near 1.4 (three
     * scales of ~1.117), so the declined region is nowhere SSB64 goes, and a
     * declined joint takes the decomp's float path, which has no range limit. */
    if ((det < (int32_t)(1 << 21)) && (det > -(int32_t)(1 << 21)))
    {
        return 0;
    }
    recip = NDS_R2_COLLISION_DIV64((int64_t)1 << (rot_bits + inv_bits), det);

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            int32_t cell = c[row][col];
            int64_t scaled;

            if (((row == 1u) && (col == 0u)) ||
                ((row == 0u) && (col == 1u)) ||
                ((row == 2u) && (col == 1u)) ||
                ((row == 1u) && (col == 2u)))
            {
                cell = -cell;
            }
            scaled = ((int64_t)cell * recip) >> inv_bits;
            /* |inv| < 2^30 at Q26 is an inverse cell under 16, i.e. a joint
             * scale above 1/16. Beyond it the row-3 product and the int32 cell
             * both stop being safe, and the joint goes back to the float
             * path -- which has no range limit at all. */
            if ((scaled >= ((int64_t)1 << 30)) ||
                (scaled <= -((int64_t)1 << 30)))
            {
                return 0;
            }
            inv[row][col] = (int32_t)scaled;
        }
    }

    /* Row 3 is -t . R^-1, kept at its native Q38 rather than reduced, because
     * the float conversion takes an arbitrary fraction width anyway and the
     * reduction would be three more shifts for nothing. Deriving it from the
     * inverse rather than transcribing the source's sign gymnastics is the
     * same matrix by construction: forward is world = local.R + t, so
     * local = (world - t).R^-1 and the constant term is -t.R^-1. */
    for (col = 0u; col < 3u; col++)
    {
        row3[col] = -((int64_t)t[0] * inv[0][col] +
                      (int64_t)t[1] * inv[1][col] +
                      (int64_t)t[2] * inv[2][col]);
    }

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            dst[row][col] = ndsR2CollisionFixedToF32(inv[row][col], inv_bits);
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        dst[3][col] = ndsR2CollisionFixedToF32(row3[col],
                                               frac_bits + inv_bits);
    }
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
