#ifndef SSB64_NDS_MP_FLOOR_CROSSING_H
#define SSB64_NDS_MP_FLOOR_CROSSING_H

#include <nds/nds_fcmp.h>

/*
 * O2R-safe scalar adapter for BattleShip's mpCollisionCheckFCSurfaceFlat plus
 * mpCollisionCheckFloorSurfaceTilt / mpCollisionCheckCeilSurfaceTilt policies.
 * The stored endpoint order is preserved by callers so source vertex flags and
 * normals retain provenance.
 *
 * `ud` selects the surface the way mpCollisionGetFCCommon does: +1 = floor, so
 * the segment must be crossed downward (previous point on or above the line,
 * current point below it); -1 = ceiling, so it must be crossed upward.  The two
 * source tilt functions differ only by that sign, and the source line loops fold
 * the same sign into their flat-segment gate (`vtdist_y < vpdist_y` for floors,
 * `vtdist_y > vpdist_y` for ceilings).  A collision therefore requires real
 * motion through the surface: proximity alone must never report a hit, or the
 * caller's `translate->y += dist` clamp re-fires every frame and pins the object
 * to the line (BUGS.md: fighters floating under the stage).
 *
 * Cycle 109: the comparisons against zero and against positive compile-time
 * constants go through `nds_fcmp.h` rather than `__aeabi_fcmp*`. This kernel
 * runs 224,280 times a match and spends 1,401,629 cycles in the comparison
 * helpers alone (fcmpeq 39,448 calls, fcmplt 27,024, fcmple 21,260,
 * fcmpgt 19,834), and the helpers are libgcc's hand-written ARM already resident
 * in ITCM -- there is nothing to win inside them, only in not making the call.
 * The predicates are integer bit tests proven equivalent over all 2^32 patterns
 * by `scripts/check_fcmp_exact.py`.
 *
 * The `< -epsilon` tests deliberately KEEP their calls: `nds_fcmp.h`'s ordered
 * `_C` forms are only exact for a POSITIVE constant, because the signed-integer
 * ordering of bit patterns inverts below zero. Same for the runtime-float
 * comparisons against `extent_epsilon` and the min/max pairs.
 *
 * Cycle 117: `side` and `orient` are BOTH +-1, so five of the six float
 * multiplies in the tilt block were sign flips written as `__aeabi_fmul`.
 *
 * This kernel is the #1 caller of the whole `__aeabi_fadd`+`__aeabi_fmul`
 * class -- 16.2% of it, 9,476 ticks/frame -- measured by a 46,856-sample GDB
 * attribution on the both-CPU gate arm, on top of 8,415 cyc/frame of self time
 * at 1.98 cyc/insn, which says instruction count rather than stall. So:
 *
 *   - `side * X` compared against zero becomes the OPPOSITE zero predicate when
 *     `ud` is negative. No multiply, no negation, no call.
 *   - `orient * sx` IS `fabsf(sx)`: `orient` is the sign of `sx` and `sx == 0`
 *     already returned above. `fabsf` is a bit clear, not a call.
 *   - `side * (orient * raw)` is `raw` or `-raw` by the XOR of two known signs,
 *     and unary `-` on a float is one `eor` in GCC, never a helper.
 *
 * All three are exact for every finite input by construction -- multiplying by
 * +-1.0f IS the sign flip, for zeroes and infinities too -- so gameplay is
 * bit-identical and no error bound is needed or claimed.
 *
 * `surface_prev` moved inside the branch that reads it. It costs an
 * `__aeabi_fdiv` (109.4 cycles a call, the most expensive helper in the build)
 * and was computed on every call that reached it, including the crossing path
 * that never looks at it. Pure code motion; the divide's operand `sx` is
 * non-zero on every path that reaches it, so sinking it cannot introduce or
 * remove a fault.
 */
static inline int ndsMPFCSegmentCrossesKernel(
    float position_x, float position_y,
    float translate_x, float translate_y,
    float v1_x, float v1_y, float v2_x, float v2_y,
    int ud,
    float *hit_x, float *hit_y)
{
    const float epsilon = 0.001F;
    /* `ud` is +1 or -1 at every caller. Zero would have made `side` 0.0f, and
     * every use of it below is a comparison against zero or a term that decides
     * a return -- with `side` zero the kernel returns 0 on both branches, so
     * folding that case here preserves the answer and keeps `side_positive`
     * meaningful. */
    const int side_positive = (ud > 0);
    float sx;
    float sy;
    float min_x;
    float max_x;
    float min_y;
    float max_y;

    if ((hit_x == 0) || (hit_y == 0) || (ud == 0))
    {
        return 0;
    }
    sx = v2_x - v1_x;
    sy = v2_y - v1_y;
    if (NDS_FCMP_EQ0(sx))
    {
        return 0;
    }
    min_x = (v1_x < v2_x) ? v1_x : v2_x;
    max_x = (v1_x > v2_x) ? v1_x : v2_x;
    min_y = (v1_y < v2_y) ? v1_y : v2_y;
    max_y = (v1_y > v2_y) ? v1_y : v2_y;

    if (NDS_FCMP_EQ0(sy))
    {
        float delta_y;
        float x;

        /* `side * X <= 0` is `X <= 0` for a positive side and `X >= 0` for a
         * negative one, and `side * X < -epsilon` is `X < -epsilon` or
         * `-X < -epsilon` i.e. `X > epsilon`. Three multiplies deleted, and the
         * two zero tests stay integer predicates. */
        if (side_positive)
        {
            if (NDS_FCMP_LE0(position_y - translate_y) ||
                ((position_y - v1_y) < -epsilon) ||
                NDS_FCMP_LE0(v1_y - translate_y))
            {
                return 0;
            }
        }
        else if (NDS_FCMP_GE0(position_y - translate_y) ||
                 NDS_FCMP_GT_C(position_y - v1_y, epsilon) ||
                 NDS_FCMP_GE0(v1_y - translate_y))
        {
            return 0;
        }
        delta_y = position_y - translate_y;
        x = (((v1_y - position_y) / delta_y) *
            (position_x - translate_x)) + position_x;
        if ((x < min_x) || (x > max_x))
        {
            return 0;
        }
        *hit_x = x;
        *hit_y = v1_y;
        return 1;
    }
    else
    {
        float motion_dx = position_x - translate_x;
        float motion_dy = position_y - translate_y;
        /* `orient` is the sign of `sx`, so `orient * sx` is `fabsf(sx)` and
         * `side * orient` is the XOR of two known signs -- a negation, not a
         * multiply. `sx != 0` was established above, so `orient` is never 0. */
        const int flip = (side_positive == 0) != (NDS_FCMP_GT0(sx) == 0);
        float raw_prev;
        float raw_curr;
        float extent_epsilon;
        float prev_height_scaled;
        float curr_height_scaled;

        if (NDS_FCMP_GT0(motion_dy))
        {
            if (((max_y + epsilon) < translate_y) ||
                (position_y < (min_y - epsilon)))
            {
                return 0;
            }
        }
        else if (((max_y + epsilon) < position_y) ||
                 (translate_y < (min_y - epsilon)))
        {
            return 0;
        }
        if (NDS_FCMP_GT0(motion_dx))
        {
            if ((max_x < translate_x) || (position_x < min_x))
            {
                return 0;
            }
        }
        else if ((max_x < position_x) || (translate_x < min_x))
        {
            return 0;
        }

        raw_prev = (sx * (position_y - v1_y)) -
            (sy * (position_x - v1_x));
        raw_curr = (sx * (translate_y - v1_y)) -
            (sy * (translate_x - v1_x));
        extent_epsilon = epsilon * __builtin_fabsf(sx);
        prev_height_scaled = flip ? -raw_prev : raw_prev;
        curr_height_scaled = flip ? -raw_curr : raw_curr;
        if (curr_height_scaled > -extent_epsilon)
        {
            return 0;
        }
        if (prev_height_scaled < extent_epsilon)
        {
            if ((prev_height_scaled > -extent_epsilon) &&
                (position_x >= min_x) && (position_x <= max_x))
            {
                /* The one site that reads it, so the divide lives here. */
                *hit_x = position_x;
                *hit_y = v1_y + (((position_x - v1_x) / sx) * sy);
                return 1;
            }
            return 0;
        }
        else
        {
            float denominator = raw_prev - raw_curr;
            float numerator;
            float t;
            float u;

            if (NDS_FCMP_EQ0(denominator))
            {
                return 0;
            }
            t = raw_prev / denominator;
            numerator = ((v1_x - position_x) *
                    (translate_y - position_y)) -
                ((v1_y - position_y) *
                    (translate_x - position_x));
            u = numerator / denominator;
            if ((t < -epsilon) || NDS_FCMP_GT_C(t, 1.0F + epsilon) ||
                (u < -epsilon) || NDS_FCMP_GT_C(u, 1.0F + epsilon))
            {
                return 0;
            }
            if (NDS_FCMP_LT0(u))
            {
                u = 0.0F;
            }
            else if (NDS_FCMP_GT_C(u, 1.0F))
            {
                u = 1.0F;
            }
            *hit_x = v1_x + (sx * u);
            *hit_y = v1_y + (sy * u);
            return 1;
        }
    }
}

#endif
