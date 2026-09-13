#ifndef NDS_PRACTICES_FIXED_MATH_H
#define NDS_PRACTICES_FIXED_MATH_H

#include <assert.h>
#include <limits.h>
#include <stdint.h>

/* Reference arithmetic with explicit rounding, not a universally fastest path.
 * Range/nonzero assertions are caller preconditions and disappear with NDEBUG.
 * Validate untrusted inputs separately. General division may call a software
 * helper; see the math chapter before using it in a hot ARM9 loop.
 * Signed Q20.12 value. Physical units belong in the field/function name.
 */
typedef int32_t q20_12_t;

enum {
    Q20_12_FRACTION_BITS = 12,
    Q20_12_ONE = 1 << Q20_12_FRACTION_BITS,
};

static inline q20_12_t q20_12_from_int(int32_t value)
{
    const int64_t scaled = (int64_t)value * (int64_t)Q20_12_ONE;
    assert(scaled >= INT32_MIN && scaled <= INT32_MAX);
    return (q20_12_t)scaled;
}

static inline int32_t q20_12_to_int_trunc_zero(q20_12_t value)
{
    return value / Q20_12_ONE;
}

static inline q20_12_t q20_12_mul_trunc_zero(q20_12_t a, q20_12_t b)
{
    const int64_t product = (int64_t)a * (int64_t)b;
    const int64_t result = product / Q20_12_ONE;
    assert(result >= INT32_MIN && result <= INT32_MAX);
    return (q20_12_t)result;
}

/* Round to nearest with halves away from zero. */
static inline q20_12_t q20_12_mul_round_away(q20_12_t a, q20_12_t b)
{
    const int64_t product = (int64_t)a * (int64_t)b;
    const int64_t half = (int64_t)1 << (Q20_12_FRACTION_BITS - 1);
    const int64_t magnitude = (product >= 0) ? product : -product;
    const int64_t rounded = (magnitude + half) / Q20_12_ONE;
    const int64_t result = (product >= 0) ? rounded : -rounded;

    assert(result >= INT32_MIN && result <= INT32_MAX);
    return (q20_12_t)result;
}

static inline q20_12_t q20_12_div_trunc(q20_12_t numerator,
                                        q20_12_t denominator)
{
    assert(denominator != 0);
    const int64_t scaled = (int64_t)numerator * Q20_12_ONE;
    const int64_t result = scaled / denominator;
    assert(result >= INT32_MIN && result <= INT32_MAX);
    return (q20_12_t)result;
}

static inline int16_t saturate_s16(int32_t value)
{
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

#endif
