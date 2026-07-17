#include <stdint.h>
#include <stdio.h>

#include "../include/nds/nds_fighter_matrix_index.h"

typedef union FloatBits
{
    float f;
    uint32_t u;
} FloatBits;

int ndsFighterMatrixAngleToIndexCodegenProbe(float angle, int32_t *out)
{
    return ndsFighterMatrixAngleToIndexExact(angle, out);
}

static int check_value(uint32_t bits, uint64_t *checks)
{
    const FloatBits multiplier = {
        .u = NDS_FIGHTER_MATRIX_INDEX_MULTIPLIER_BITS
    };
    FloatBits value = { .u = bits };
    volatile float product = value.f * multiplier.f;
    int32_t expected = (int32_t)product;
    int32_t actual = 0;

    (*checks)++;
    if (!ndsFighterMatrixAngleToIndexExact(value.f, &actual))
    {
        fprintf(stderr, "unexpected rejection: bits=%08x\n",
                (unsigned int)bits);
        return 0;
    }
    if (actual != expected)
    {
        fprintf(stderr,
                "angle-index mismatch: bits=%08x expected=%d actual=%d\n",
                (unsigned int)bits, (int)expected, (int)actual);
        return 0;
    }
    return 1;
}

int main(void)
{
    uint64_t checks = 0;
    uint32_t exponent;
    uint32_t fraction;
    uint32_t sign;
    int32_t output;

    if ((sizeof(float) != 4u) ||
        !check_value(UINT32_C(0x00000000), &checks) ||
        !check_value(UINT32_C(0x80000000), &checks) ||
        !check_value(UINT32_C(0x007fffff), &checks) ||
        !check_value(UINT32_C(0x807fffff), &checks))
    {
        return 1;
    }
    for (exponent = 1u; exponent <= 116u; exponent++)
    {
        for (sign = 0u; sign <= 1u; sign++)
        {
            uint32_t sign_bits = sign << 31;

            if (!check_value(sign_bits | (exponent << 23), &checks) ||
                !check_value(sign_bits | (exponent << 23) |
                             UINT32_C(0x7fffff), &checks))
            {
                return 1;
            }
        }
    }
    for (exponent = 117u; exponent <= 130u; exponent++)
    {
        for (sign = 0u; sign <= 1u; sign++)
        {
            uint32_t prefix = (sign << 31) | (exponent << 23);

            for (fraction = 0u; fraction <= UINT32_C(0x7fffff);
                 fraction++)
            {
                if (!check_value(prefix | fraction, &checks))
                {
                    return 1;
                }
            }
        }
    }
    if (ndsFighterMatrixAngleToIndexExact(16.0f, &output) ||
        ndsFighterMatrixAngleToIndexExact(-16.0f, &output))
    {
        fputs("out-of-range fighter angle did not fail closed\n", stderr);
        return 1;
    }

    printf("FIGHTER_MATRIX_ANGLE_INDEX=PASS checks=%llu range=[-16,16)\n",
           (unsigned long long)checks);
    return 0;
}
