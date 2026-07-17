#ifndef SSB64_NDS_FIGHTER_MATRIX_INDEX_H
#define SSB64_NDS_FIGHTER_MATRIX_INDEX_H

#include <stddef.h>
#include <stdint.h>

#define NDS_FIGHTER_MATRIX_INDEX_MULTIPLIER_BITS UINT32_C(0x4422f983)

/* Reproduce truncf(angle * 651.89862060546875f) without soft-float calls.
 * Inputs outside the proven fighter-angle range fail closed to BattleShip. */
static inline int ndsFighterMatrixAngleToIndexExact(
    float angle, int32_t *out)
{
    union
    {
        float f;
        uint32_t u;
    } bits = { angle };
    uint32_t exponent = (bits.u >> 23) & UINT32_C(0xff);
    uint64_t product;
    uint64_t remainder;
    uint64_t halfway;
    uint32_t mantissa;
    uint32_t rounded;
    uint32_t shift;
    uint32_t top_bit;
    int32_t product_exponent;
    int32_t magnitude;

    if (out == NULL)
    {
        return 0;
    }
    if (exponent <= UINT32_C(116))
    {
        /* Even the largest value in these exponent classes multiplies to
         * less than one. This also covers signed zero and subnormals. */
        *out = 0;
        return 1;
    }
    if (exponent > UINT32_C(130))
    {
        return 0;
    }

    mantissa = (bits.u & UINT32_C(0x7fffff)) | UINT32_C(0x800000);
    product = (uint64_t)mantissa * UINT64_C(0xa2f983);
    top_bit = ((product & (UINT64_C(1) << 47)) != 0u) ? 47u : 46u;
    shift = top_bit - 23u;
    rounded = (uint32_t)(product >> shift);
    remainder = product & ((UINT64_C(1) << shift) - UINT64_C(1));
    halfway = UINT64_C(1) << (shift - 1u);
    if ((remainder > halfway) ||
        ((remainder == halfway) && ((rounded & 1u) != 0u)))
    {
        rounded++;
        if (rounded == UINT32_C(0x1000000))
        {
            rounded >>= 1;
            top_bit++;
        }
    }

    product_exponent = (int32_t)exponent + (int32_t)top_bit - 164;
    if (product_exponent < 0)
    {
        magnitude = 0;
    }
    else
    {
        magnitude = (int32_t)(rounded >> (23 - product_exponent));
    }
    *out = ((bits.u & UINT32_C(0x80000000)) != 0u) ? -magnitude : magnitude;
    return 1;
}

#endif
