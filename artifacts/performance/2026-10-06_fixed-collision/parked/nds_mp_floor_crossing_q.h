#ifndef SSB64_NDS_MP_FLOOR_CROSSING_Q_H
#define SSB64_NDS_MP_FLOOR_CROSSING_Q_H

#include <stdint.h>
#include <nds/nds_r2_hwmath_unit.h>

/*
 * nds_mp_floor_crossing.h's kernel in Q12 (P2-2p8, 2026-10-06; owner:
 * "Software floating point should not exist, fixed point only"; ruling D13
 * re-baselines the digest). Positions and the hit are Q12 world units; the
 * segment's endpoints are the map's integers. Every test is the float
 * kernel's, in its order, on integer arithmetic: differences of Q12 values,
 * 64-bit cross products at Q24, the 0.001 epsilon compared exactly (1000 d
 * against 4096) and, against the cross products, as 4.096 Q24 steps per Q12
 * unit of |sx|; the two intersection parameters at Q28 from one math-unit
 * divide each, after both operands are brought under 2^33 together (the
 * ranges the float kernel accepts keep |t| and |u| near [0, 1]). The sweeps
 * convert their two points once per group, so the per-segment float
 * subtracts, multiplies and divides are gone. Returns 0/1 as it does.
 * Host check against the float kernel (scratchpad kerneltest.py, 300,000
 * random motions near random segments): every float hit is a Q hit, 26 more
 * Q hits sit on the 0.001 epsilon boundary (the inputs' Q12 rounding), and
 * hit points agree within 0.1 units (a grazing flat crossing).
 */
static inline int ndsMPFCQBitLen64(uint64_t v)
{
    return (v == 0u) ? 0 : (64 - __builtin_clzll(v));
}

/* a / b at Q28 for |a| <= 2|b| (both 64-bit, b != 0). */
static inline int32_t ndsMPFCQRatioQ28(int64_t a, int64_t b)
{
    const uint64_t mag = (uint64_t)((b < 0) ? -b : b);
    const int n = ndsMPFCQBitLen64(mag) - 32;

    if (n > 0)
    {
        a >>= n;
        b >>= n;
    }
    return (int32_t)ndsR2HwMathDivideFast(a * ((int64_t)1 << 28), b);
}

/* d > 0.001 and d < -0.001 on a Q12 difference, exactly: d / 4096 against
 * 1 / 1000, as 1000 d against 4096 in 64 bits. */
#define NDS_MPFC_Q_ABOVE_EPS(d) (((int64_t)(d) * 1000) > 4096)
#define NDS_MPFC_Q_BELOW_EPS(d) (((int64_t)(d) * 1000) < -4096)

/* ARM state and out of line: its 64-bit products and CLZ are single
 * instructions there (in a Thumb caller they were __aeabi_lmul and __clzdi2
 * calls, fifteen and one a call). */
static int __attribute__((noinline, target("arm"))) ndsMPFCSegmentCrossesKernelQ(
    int32_t px, int32_t py, int32_t tx, int32_t ty,
    int32_t v1x_i, int32_t v1y_i, int32_t v2x_i, int32_t v2y_i,
    int ud, int32_t *hit_x, int32_t *hit_y)
{
    const int side_positive = (ud > 0);
    const int32_t v1x = v1x_i << 12;
    const int32_t v1y = v1y_i << 12;
    const int32_t v2x = v2x_i << 12;
    const int32_t v2y = v2y_i << 12;
    const int32_t sx = v2x - v1x;
    const int32_t sy = v2y - v1y;
    int32_t min_x;
    int32_t max_x;
    int32_t min_y;
    int32_t max_y;

    if ((hit_x == 0) || (hit_y == 0) || (ud == 0) || (sx == 0))
    {
        return 0;
    }
    min_x = (v1x < v2x) ? v1x : v2x;
    max_x = (v1x > v2x) ? v1x : v2x;
    min_y = (v1y < v2y) ? v1y : v2y;
    max_y = (v1y > v2y) ? v1y : v2y;

    if (sy == 0)
    {
        int64_t delta_y;
        int32_t x;

        if (side_positive)
        {
            if (((py - ty) <= 0) || NDS_MPFC_Q_BELOW_EPS(py - v1y) ||
                ((v1y - ty) <= 0))
            {
                return 0;
            }
        }
        else if (((py - ty) >= 0) || NDS_MPFC_Q_ABOVE_EPS(py - v1y) ||
                 ((v1y - ty) >= 0))
        {
            return 0;
        }
        delta_y = (int64_t)py - ty;
        x = px + (int32_t)ndsR2HwMathDivideFast(
            ((int64_t)v1y - py) * ((int64_t)px - tx), delta_y);
        if ((x < min_x) || (x > max_x))
        {
            return 0;
        }
        *hit_x = x;
        *hit_y = v1y;
        return 1;
    }
    else
    {
        const int32_t motion_dx = px - tx;
        const int32_t motion_dy = py - ty;
        const int flip = (side_positive == 0) != (sx <= 0);
        const int64_t abs_sx = (sx < 0) ? -(int64_t)sx : (int64_t)sx;
        /* 0.001 * |sx| at Q24: |sx| (Q12) x 4.096. */
        const int64_t extent_epsilon = (abs_sx * 4194) >> 10;
        int64_t raw_prev;
        int64_t raw_curr;
        int64_t prev_height_scaled;
        int64_t curr_height_scaled;

        /* max + 0.001 < v is v - max > 0.001; v < min - 0.001 is
         * v - min < -0.001. */
        if (motion_dy > 0)
        {
            if (NDS_MPFC_Q_ABOVE_EPS(ty - max_y) ||
                NDS_MPFC_Q_BELOW_EPS(py - min_y))
            {
                return 0;
            }
        }
        else if (NDS_MPFC_Q_ABOVE_EPS(py - max_y) ||
                 NDS_MPFC_Q_BELOW_EPS(ty - min_y))
        {
            return 0;
        }
        if (motion_dx > 0)
        {
            if ((max_x < tx) || (px < min_x))
            {
                return 0;
            }
        }
        else if ((max_x < px) || (tx < min_x))
        {
            return 0;
        }
        raw_prev = ((int64_t)sx * ((int64_t)py - v1y)) -
                   ((int64_t)sy * ((int64_t)px - v1x));
        raw_curr = ((int64_t)sx * ((int64_t)ty - v1y)) -
                   ((int64_t)sy * ((int64_t)tx - v1x));
        prev_height_scaled = flip ? -raw_prev : raw_prev;
        curr_height_scaled = flip ? -raw_curr : raw_curr;
        if (curr_height_scaled > -extent_epsilon)
        {
            return 0;
        }
        if (prev_height_scaled < extent_epsilon)
        {
            if ((prev_height_scaled > -extent_epsilon) &&
                (px >= min_x) && (px <= max_x))
            {
                *hit_x = px;
                *hit_y = v1y + (int32_t)ndsR2HwMathDivideFast(
                    ((int64_t)px - v1x) * sy, sx);
                return 1;
            }
            return 0;
        }
        else
        {
            const int64_t denominator = raw_prev - raw_curr;
            const int64_t abs_den =
                (denominator < 0) ? -denominator : denominator;
            const int64_t numerator =
                (((int64_t)v1x - px) * ((int64_t)ty - py)) -
                (((int64_t)v1y - py) * ((int64_t)tx - px));
            const int32_t one = (int32_t)1 << 28;
            const int32_t eps28 = 268435;  /* 0.001 at Q28 */
            int32_t t;
            int32_t u;

            if (denominator == 0)
            {
                return 0;
            }
            /* |t| or |u| past 2 is far outside the accepted range. */
            if ((((raw_prev < 0) ? -raw_prev : raw_prev) > (abs_den << 1)) ||
                (((numerator < 0) ? -numerator : numerator) > (abs_den << 1)))
            {
                return 0;
            }
            t = ndsMPFCQRatioQ28(raw_prev, denominator);
            u = ndsMPFCQRatioQ28(numerator, denominator);
            if ((t < -eps28) || (t > (one + eps28)) ||
                (u < -eps28) || (u > (one + eps28)))
            {
                return 0;
            }
            if (u < 0)
            {
                u = 0;
            }
            else if (u > one)
            {
                u = one;
            }
            *hit_x = v1x + (int32_t)(((int64_t)sx * u) >> 28);
            *hit_y = v1y + (int32_t)(((int64_t)sy * u) >> 28);
            return 1;
        }
    }
}


#endif
