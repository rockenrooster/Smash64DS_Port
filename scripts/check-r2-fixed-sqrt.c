/* Falsifier for NDS_R2_FIXED_SQRT (R2-03 E1).
 *
 * Includes the shipped kernel from include/nds/nds_r2_sqrtf.h -- not a
 * transcription of it -- with the DS hardware square-root unit stood in for by
 * an exact integer root. The host's sqrtf is IEEE correctly rounded, so a
 * correctly rounded implementation must agree on every bit of every normal
 * input. Any mismatch is a pose that would differ on hardware.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* floor(sqrt(a)) -- what the DS unit returns, computed exactly. */
static unsigned int ndsR2SqrtfHardware(unsigned long long a)
{
    unsigned long long rem = 0, root = 0;
    int i;

    for (i = 0; i < 32; i++) {
        root <<= 1;
        rem = (rem << 2) | (a >> 62);
        a <<= 2;
        if (rem > root) {
            rem -= root | 1;
            root += 2;
        }
    }
    return (unsigned int)(root >> 1);
}

#include "../include/nds/nds_r2_sqrtf.h"

static unsigned long long checked, mismatches, fallbacks;

static void check(uint32_t bits)
{
    unsigned int out;
    float x, mine, ref;
    uint32_t mine_bits, ref_bits;

    memcpy(&x, &bits, sizeof(x));
    ref = sqrtf(x);
    memcpy(&ref_bits, &ref, sizeof(ref_bits));
    checked++;

    if (ndsR2SqrtfBits(bits, &out) == NDS_R2_SQRTF_FALLBACK) {
        /* Declined on purpose; the ROM hands these to newlib unchanged, so
         * there is nothing to compare. Counted so a kernel that silently
         * declined everything could not pass. */
        fallbacks++;
        return;
    }
    memcpy(&mine, &out, sizeof(mine));
    mine_bits = out;
    if (mine_bits != ref_bits) {
        if (mismatches < 10)
            printf("MISMATCH x=%08lx (%.9g)  fast=%08lx (%.9g)  ref=%08lx (%.9g)\n",
                   (unsigned long)bits, (double)x,
                   (unsigned long)mine_bits, (double)mine,
                   (unsigned long)ref_bits, (double)ref);
        mismatches++;
    }
}

int main(void)
{
    static const uint32_t edge_mantissas[] = {
        0u, 1u, 2u, 0x3fffffu, 0x400000u, 0x7ffffeu, 0x7fffffu
    };
    uint32_t seed = 12345u;
    uint32_t exponent, n, m;
    unsigned k;
    unsigned long long i;

    /* Every exponent, including the declined ones, at boundary mantissas. */
    for (exponent = 0; exponent < 256u; exponent++)
        for (k = 0; k < sizeof(edge_mantissas) / sizeof(edge_mantissas[0]); k++)
            check((exponent << 23) | edge_mantissas[k]);

    /* Perfect squares and their immediate neighbours: where an off-by-one in
     * the rounding test shows up and nowhere else. */
    for (n = 1; n < 4096u; n++) {
        float f = (float)((double)n * (double)n);
        uint32_t b;
        memcpy(&b, &f, sizeof(b));
        check(b);
        check(b - 1u);
        check(b + 1u);
    }

    /* Dense sweep across four exponents, both parities. */
    for (exponent = 100u; exponent < 104u; exponent++)
        for (m = 0; m < (1u << 23); m += 7u)
            check((exponent << 23) | m);

    for (i = 0; i < 8000000ull; i++) {
        seed = seed * 1103515245u + 12345u;
        check(seed);
    }

    printf("R2 fixed sqrt: checked %llu inputs, %llu handled, %llu declined, "
           "%llu mismatches\n",
           checked, checked - fallbacks, fallbacks, mismatches);
    if (checked - fallbacks < 1000000ull) {
        printf("FAIL: the kernel declined almost everything; it is not being "
               "exercised.\n");
        return 1;
    }
    return mismatches != 0ull;
}
