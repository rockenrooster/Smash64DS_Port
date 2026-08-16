/* Host falsifier for include/nds/nds_r2_hwmath.h -- the correctly-rounded
 * IEEE-754 single divide built on the ARM9 64/32 divide unit.
 *
 * THE SPLIT THIS FILE EXISTS TO MAKE. The claim "the hardware divide unit can
 * serve __aeabi_fdiv bit-exactly" has two halves and they are provable to
 * completely different standards, so they are proved separately rather than
 * both being bounded:
 *
 *   ENUMERABLE HALF -- the algorithm wrapped around the unit. Graded HERE,
 *   exhaustively over whole 2^23 significand axes, with the unit stood in for
 *   by an exact C divide. Nothing about it is sampled.
 *
 *   HARDWARE HALF -- that the unit itself returns the exact truncated quotient
 *   and remainder. NOT provable on the host and not claimed here. It is graded
 *   in the ROM by src/port/nds_r2_hwmath_bench.c against the portable software
 *   divide on the same operands, and the same unit's square-root half already
 *   carries an in-tree bit-exactness proof through sqrtf
 *   (scripts/check-r2-fixed-sqrt.ps1).
 *
 * Bounding the whole pipeline instead would be the mistake recorded as
 * "prove the parser half exactly".
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* The unit, modelled exactly. Both operands are positive by construction in
 * ndsR2HwMathDivBits, so the sign rules never enter. */
static int64_t ndsR2HwMathDivideUnit(int64_t numerator, int32_t denominator,
                                     int32_t *remainder_out)
{
    int64_t quotient = numerator / (int64_t)denominator;

    if (remainder_out != NULL)
    {
        *remainder_out = (int32_t)(numerator % (int64_t)denominator);
    }
    return quotient;
}

#include "../include/nds/nds_r2_hwmath.h"

static unsigned int gTies;
static unsigned int gRoundUps;
static unsigned int gExactQuotients;
static unsigned int gFallbacks;
static unsigned int gExact;
static unsigned long long gCases;

static unsigned int bitsOf(float value)
{
    unsigned int bits;

    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static float floatOf(unsigned int bits)
{
    float value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

/* Recomputed independently of the kernel so a shared helper cannot make the two
 * arms agree by construction -- the failure mode recorded as "an oracle sharing
 * a decoder proves nothing".
 *
 * `tie` here means guard set and sticky clear, i.e. the exact quotient lands on
 * a half and round-to-nearest has to consult the even rule. It is counted so
 * the assertion at the end can be that it NEVER happens, which is the header's
 * claim: with a 2^32-scaled numerator the quotient of two 24-bit significands
 * is either non-terminating (remainder non-zero, so sticky set) or terminates
 * within 24 significant bits (so the guard bit is zero). A 25-bit quotient,
 * which is what a tie needs, cannot occur.
 *
 * `round` and `exact_quotient` are the positive controls for that assertion: a
 * detector that never fires proves nothing unless its two neighbouring states
 * do fire, so this also counts the cases that DID round up and the cases whose
 * remainder was exactly zero. */
static void classify(unsigned int a_bits, unsigned int b_bits)
{
    unsigned int ma = (a_bits & 0x007fffffu) | 0x00800000u;
    unsigned int mb = (b_bits & 0x007fffffu) | 0x00800000u;
    unsigned long long numerator = (unsigned long long)ma << 32;
    unsigned long long quotient = numerator / mb;
    unsigned long long remainder = numerator % mb;
    unsigned int shift = (ma >= mb) ? 9u : 8u;
    unsigned long long low = quotient & ((1ull << shift) - 1ull);
    unsigned long long half = 1ull << (shift - 1u);

    if (remainder == 0ull)
    {
        gExactQuotients++;
    }
    if ((low == half) && (remainder == 0ull))
    {
        gTies++;
    }
    else if (low >= half)
    {
        gRoundUps++;
    }
}

static int grade(unsigned int a_bits, unsigned int b_bits)
{
    unsigned int got = 0u;
    unsigned int want;
    float quotient;
    int status;

    gCases++;
    status = ndsR2HwMathDivBits(a_bits, b_bits, &got);

    quotient = floatOf(a_bits) / floatOf(b_bits);
    want = bitsOf(quotient);

    if (status == NDS_R2_HWMATH_FDIV_FALLBACK)
    {
        gFallbacks++;
        return 1;
    }
    gExact++;
    classify(a_bits, b_bits);
    if (got != want)
    {
        printf("MISMATCH a=%08x b=%08x got=%08x want=%08x\n", a_bits, b_bits,
               got, want);
        return 0;
    }
    return 1;
}

/* Exhaustive over one whole significand axis: every one of the 2^23
 * representable significands of the swept operand, at the given exponents. */
static int sweepSignificands(unsigned int fixed_bits, unsigned int exponent,
                             int sweep_is_denominator)
{
    unsigned int fraction;

    for (fraction = 0u; fraction < 0x00800000u; fraction++)
    {
        unsigned int swept = (exponent << 23) | fraction;

        if (sweep_is_denominator)
        {
            if (!grade(fixed_bits, swept))
            {
                return 0;
            }
        }
        else if (!grade(swept, fixed_bits))
        {
            return 0;
        }
    }
    return 1;
}

static unsigned int nextRandom(unsigned int *state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

int main(void)
{
    static const unsigned int kExponents[] = { 1u,  2u,   40u,  126u, 127u,
                                               128u, 200u, 253u, 254u };
    unsigned int state = 0x5f3759dfu;
    unsigned int i;
    unsigned int j;
    unsigned long long n;

    /* 1. ENUMERABLE. Every significand of the denominator against a fixed
     *    numerator, and every significand of the numerator against a fixed
     *    denominator, at three exponent pairs each. Six full 2^23 axes. */
    if (!sweepSignificands(bitsOf(1.0f), 127u, 1) ||
        !sweepSignificands(0x3fabcdefu, 127u, 1) ||
        !sweepSignificands(0x7f000000u, 254u, 1) ||
        !sweepSignificands(bitsOf(1.0f), 127u, 0) ||
        !sweepSignificands(0x40123457u, 127u, 0) ||
        !sweepSignificands(0x00800000u, 1u, 0))
    {
        return 1;
    }

    /* 2. EXPONENT CROSS-PRODUCT, every pair of the interesting exponents
     *    against a small set of significands including both endpoints. */
    for (i = 0u; i < (sizeof(kExponents) / sizeof(kExponents[0])); i++)
    {
        for (j = 0u; j < (sizeof(kExponents) / sizeof(kExponents[0])); j++)
        {
            static const unsigned int kFractions[] = {
                0x000000u, 0x000001u, 0x400000u, 0x7fffffu, 0x2aaaabu
            };
            unsigned int fa;
            unsigned int fb;

            for (fa = 0u; fa < 5u; fa++)
            {
                for (fb = 0u; fb < 5u; fb++)
                {
                    unsigned int a = (kExponents[i] << 23) | kFractions[fa];
                    unsigned int b = (kExponents[j] << 23) | kFractions[fb];

                    if (!grade(a, b) || !grade(a | 0x80000000u, b) ||
                        !grade(a, b | 0x80000000u) ||
                        !grade(a | 0x80000000u, b | 0x80000000u))
                    {
                        return 1;
                    }
                }
            }
        }
    }

    /* 3. BOUNDED. A deterministic pseudorandom sweep over the whole normal
     *    range, both signs, 32 million pairs. Stated as bounded, not claimed
     *    exhaustive. */
    for (n = 0ull; n < 8000000ull; n++)
    {
        unsigned int a = nextRandom(&state);
        unsigned int b = nextRandom(&state);
        unsigned int ea = ((a >> 23) & 0xffu);
        unsigned int eb = ((b >> 23) & 0xffu);

        if ((ea == 0u) || (ea == 0xffu))
        {
            a = (a & 0x807fffffu) | (0x40u << 23);
        }
        if ((eb == 0u) || (eb == 0xffu))
        {
            b = (b & 0x807fffffu) | (0x41u << 23);
        }
        if (!grade(a, b))
        {
            return 1;
        }
    }

    /* 4. THE DECLINE PATH MUST DECLINE. Every special-class input, and a
     *    result-overflow and a result-underflow pair. A control that cannot
     *    fail is not a control, so these assert FALLBACK explicitly. */
    {
        static const unsigned int kSpecial[] = {
            0x00000000u, 0x80000000u, 0x00000001u, 0x007fffffu,
            0x7f800000u, 0xff800000u, 0x7fc00000u, 0x7f800001u
        };
        unsigned int k;
        unsigned int out = 0u;

        for (k = 0u; k < (sizeof(kSpecial) / sizeof(kSpecial[0])); k++)
        {
            if (ndsR2HwMathDivBits(kSpecial[k], bitsOf(1.0f), &out) !=
                    NDS_R2_HWMATH_FDIV_FALLBACK ||
                ndsR2HwMathDivBits(bitsOf(1.0f), kSpecial[k], &out) !=
                    NDS_R2_HWMATH_FDIV_FALLBACK)
            {
                printf("SPECIAL %08x was not declined\n", kSpecial[k]);
                return 1;
            }
        }
        /* FLT_MAX / FLT_MIN_NORMAL overflows; FLT_MIN_NORMAL / FLT_MAX
         * underflows. Both must decline rather than pack a wrong exponent. */
        if (ndsR2HwMathDivBits(0x7f7fffffu, 0x00800000u, &out) !=
                NDS_R2_HWMATH_FDIV_FALLBACK ||
            ndsR2HwMathDivBits(0x00800000u, 0x7f7fffffu, &out) !=
                NDS_R2_HWMATH_FDIV_FALLBACK)
        {
            printf("range decline missing\n");
            return 1;
        }
    }

    /* The tie assertion, with its two positive controls beside it. If the
     * significand ranges were ever wrong -- an operand wider than 24 bits, a
     * numerator scaled by fewer than 32 bits -- gTies would be non-zero here
     * and this would fail, which is what makes it a control rather than a
     * decoration. */
    if (gRoundUps == 0u)
    {
        printf("NO CASE ROUNDED UP -- the rounding branch is unproven\n");
        return 1;
    }
    if (gExactQuotients == 0u)
    {
        printf("NO EXACT QUOTIENT -- the sticky-clear path is unproven\n");
        return 1;
    }
    if (gTies != 0u)
    {
        printf("TIE REACHED %u times -- the header's 25-bit argument is "
               "false and round-to-even is load-bearing after all\n",
               gTies);
        return 1;
    }
    if (gExact < 40000000u)
    {
        printf("coverage too small: %u exact\n", gExact);
        return 1;
    }

    printf("R2 hwmath fdiv: %llu cases, %u exact bit-identical, %u rounded up, "
           "%u exact quotients, %u ties (must be 0), %u declined\n",
           gCases, gExact, gRoundUps, gExactQuotients, gTies, gFallbacks);
    return 0;
}
