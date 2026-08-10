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
 */
static inline int ndsMPFCSegmentCrossesKernel(
    float position_x, float position_y,
    float translate_x, float translate_y,
    float v1_x, float v1_y, float v2_x, float v2_y,
    int ud,
    float *hit_x, float *hit_y)
{
    const float epsilon = 0.001F;
    const float side = (float)ud;
    float sx;
    float sy;
    float min_x;
    float max_x;
    float min_y;
    float max_y;

    if ((hit_x == 0) || (hit_y == 0))
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

        if (NDS_FCMP_LE0(side * (position_y - translate_y)) ||
            ((side * (position_y - v1_y)) < -epsilon) ||
            NDS_FCMP_LE0(side * (v1_y - translate_y)))
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
        float raw_prev;
        float raw_curr;
        float orient;
        float extent_epsilon;
        float prev_height_scaled;
        float curr_height_scaled;
        float surface_prev;

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
        orient = NDS_FCMP_GT0(sx) ? 1.0F : -1.0F;
        extent_epsilon = epsilon * (orient * sx);
        prev_height_scaled = side * (orient * raw_prev);
        curr_height_scaled = side * (orient * raw_curr);
        if (curr_height_scaled > -extent_epsilon)
        {
            return 0;
        }
        surface_prev = v1_y + (((position_x - v1_x) / sx) * sy);
        if (prev_height_scaled < extent_epsilon)
        {
            if ((prev_height_scaled > -extent_epsilon) &&
                (position_x >= min_x) && (position_x <= max_x))
            {
                *hit_x = position_x;
                *hit_y = surface_prev;
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
