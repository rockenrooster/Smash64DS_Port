/* Host proof: ndsR2CfxAngleIndexBits(bits) == (int32_t)(x * 651.8986206f)
 * for every binary32 x whose product stays inside int32 (the emulation
 * reports the others so the caller keeps the float path for them). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Returns 1 and the index when the exact emulation applies; 0 otherwise. */
static inline int ndsR2CfxAngleIndexBits(uint32_t bits, int32_t *out)
{
    const uint32_t exponent = (bits >> 23) & 0xffu;
    uint32_t mantissa;
    uint64_t product;
    uint32_t rounded;
    uint32_t rem;
    uint32_t half;
    int32_t biased;
    uint32_t value;

    if (exponent == 0u)
    {
        /* Zero or subnormal: |x * K| < 2^-116, truncates to 0. */
        *out = 0;
        return 1;
    }
    if (exponent == 0xffu)
    {
        return 0;
    }
    mantissa = (bits & 0x7fffffu) | 0x800000u;
    product = (uint64_t)mantissa * 0xa2f983u; /* K's mantissa, K = 651.8986206f */
    /* x = m * 2^(exponent - 150), K = 0xa2f983 * 2^(136 - 150). */
    biased = (int32_t)exponent + 136 - 127; /* the product's exponent if it
                                             * normalises at bit 46 */
    if ((product >> 47) != 0u)
    {
        rounded = (uint32_t)(product >> 24);
        rem = (uint32_t)(product & 0xffffffu);
        half = 0x800000u;
        biased += 1;
    }
    else
    {
        rounded = (uint32_t)(product >> 23);
        rem = (uint32_t)(product & 0x7fffffu);
        half = 0x400000u;
    }
    if ((rem > half) || ((rem == half) && ((rounded & 1u) != 0u)))
    {
        rounded++;
        if (rounded == 0x1000000u)
        {
            rounded = 0x800000u;
            biased += 1;
        }
    }
    /* result = rounded * 2^(biased - 150); biased is the float's exponent field */
    if (biased >= 158)
    {
        return 0; /* |result| >= 2^31: f2iz saturates; keep the float path */
    }
    if (biased < 127)
    {
        value = 0u;
    }
    else if (biased >= 150)
    {
        value = rounded << (biased - 150);
    }
    else
    {
        value = rounded >> (150 - biased);
    }
    *out = ((bits >> 31) != 0u) ? -(int32_t)value : (int32_t)value;
    return 1;
}

int main(void)
{
    uint64_t checked = 0, skipped = 0, bad = 0;
    uint32_t bits = 0;

    do
    {
        float x;
        memcpy(&x, &bits, sizeof(x));
        if (((bits >> 23) & 0xffu) != 0xffu)
        {
            volatile float prod = x * 651.8986206f;
            int32_t emu;

            if (ndsR2CfxAngleIndexBits(bits, &emu))
            {
                /* Only compare where the host conversion is defined. */
                if (prod < 2147483648.0f && prod > -2147483648.0f)
                {
                    int32_t ref = (int32_t)prod;
                    if (ref != emu)
                    {
                        if (bad < 10)
                        {
                            printf("MISMATCH bits=%08x x=%g ref=%d emu=%d\n",
                                   bits, (double)x, ref, emu);
                        }
                        bad++;
                    }
                    checked++;
                }
                else
                {
                    printf("EMU CLAIMED OUT-OF-RANGE bits=%08x\n", bits);
                    bad++;
                }
            }
            else
            {
                skipped++;
            }
        }
        bits++;
    } while (bits != 0u);
    printf("checked %llu skipped %llu bad %llu\n",
           (unsigned long long)checked, (unsigned long long)skipped,
           (unsigned long long)bad);
    return bad != 0;
}
