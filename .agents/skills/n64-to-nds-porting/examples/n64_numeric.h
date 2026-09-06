#ifndef PORT_N64_NUMERIC_H
#define PORT_N64_NUMERIC_H

/* Explicit conversion policies, not bit-exact replacements for N64 arithmetic.
 * Asset/setup helpers unless the selected generated code is suitable for a hot
 * path. No floating point, undefined signed shifts, or saturating-away errors.
 */
#include <stdbool.h>
#include <stdint.h>

/* Mathematical floor(v / 2^shift). Precondition: shift <= 31.
 * Works for INT32_MIN without negating it or shifting a negative signed value.
 */
static inline int32_t port_floor_shift32(int32_t v, unsigned shift)
{
    if (v >= 0)
        return (int32_t)((uint32_t)v >> shift);
    return -1 - (int32_t)((uint32_t)(-(v + 1)) >> shift);
}

/* Q16.16 -> Q20.12; nearest, ties to even. All int32 inputs are representable.
 * This is deliberately not an assertion that source RSP rounding is identical.
 */
static inline int32_t port_q16_to_q12_rne(int32_t v)
{
    const int32_t q = port_floor_shift32(v, 4u);
    const uint32_t rem = (uint32_t)v & 15u;
    return q + (int32_t)(rem > 8u || (rem == 8u && ((uint32_t)q & 1u)));
}

/* Integer source coordinate -> DS signed Q4.12 vertex, after scaling source
 * local units by 2^-scale_log2. Accepted scale_log2 is 0..27.
 * Rounding is floor when bits are discarded. The caller must compensate the
 * geometry's scale in the appropriate transform, including translated origins.
 * Reject out-of-range coordinates instead of wrapping/clamping silhouettes.
 */
static inline bool port_s16_to_v16(int16_t source, unsigned scale_log2,
                                  int16_t *out)
{
    if (out == 0 || scale_log2 > 27u)
        return false;
    int32_t raw;
    if (scale_log2 <= 12u)
        raw = (int32_t)source * (int32_t)(UINT32_C(1) << (12u - scale_log2));
    else
        raw = port_floor_shift32((int32_t)source, scale_log2 - 12u);
    if (raw < INT16_MIN || raw > INT16_MAX)
        return false;
    *out = (int16_t)raw;
    return true;
}
#endif
