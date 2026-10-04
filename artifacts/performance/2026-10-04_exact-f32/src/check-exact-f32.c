/* Host check for include/nds/nds_exact_f32.h: every multiply and add, and the
 * matrix compose / point transform chains built from them, must return the
 * same bits as the host's binary32 arithmetic (round to nearest even) for
 * admitted operands.
 *
 *   gcc -O2 -ffp-contract=off -Iinclude -o check-exact-f32 scripts/check-exact-f32.c
 *   ./check-exact-f32 [iterations]
 *
 * x86-64 float arithmetic is SSE (one IEEE rounding per operation), which is
 * what libgcc's soft float returns on the DS for these operands. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nds/nds_exact_f32.h"

static uint64_t sState = 0x9e3779b97f4a7c15ull;

static uint32_t rnd32(void)
{
    sState ^= sState << 13;
    sState ^= sState >> 7;
    sState ^= sState << 17;
    return (uint32_t)(sState >> 16);
}

static uint32_t f2b(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}

static float b2f(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* An admitted operand: zero now and then, otherwise a normal whose exponent
 * is drawn either across the whole domain or near `near_e` (so additions
 * meet equal and adjacent exponents, cancellation and ties often). */
static uint32_t operand(int near_e)
{
    uint32_t r = rnd32();
    uint32_t sign = (r & 1u) << 31;
    uint32_t mant;
    int e;

    if ((r & 0x3f0u) == 0u)
    {
        return sign;
    }
    switch ((r >> 10) & 3u)
    {
    case 0:
        e = 127 - NDS_XF32_EXP_RANGE + (int)(rnd32() % (2u * NDS_XF32_EXP_RANGE + 1u));
        break;
    default:
        e = near_e + (int)(rnd32() % 5u) - 2;
        break;
    }
    if (e < 127 - NDS_XF32_EXP_RANGE) e = 127 - NDS_XF32_EXP_RANGE;
    if (e > 127 + NDS_XF32_EXP_RANGE) e = 127 + NDS_XF32_EXP_RANGE;
    mant = rnd32() & 0x007fffffu;
    switch ((r >> 12) & 7u)
    {
    case 0: mant = 0u; break;                      /* powers of two */
    case 1: mant |= 0x007ff000u; break;            /* near the top */
    case 2: mant &= 0x00000fffu; break;            /* near the bottom */
    case 3: mant &= 0x007fff00u; break;            /* short mantissas: ties */
    default: break;
    }
    return sign | ((uint32_t)e << 23) | mant;
}

static int sFailures;

static void fail(const char *what, uint32_t a, uint32_t b, uint32_t got,
                 uint32_t want)
{
    if (sFailures++ < 20)
    {
        printf("%s %08x %08x -> %08x, want %08x\n", what, a, b, got, want);
    }
}

int main(int argc, char **argv)
{
    long iterations = (argc > 1) ? atol(argv[1]) : 20000000L;
    long i;

    for (i = 0; i < iterations; i++)
    {
        int near_e = 127 - NDS_XF32_EXP_RANGE / 2 + (int)(rnd32() % NDS_XF32_EXP_RANGE);
        uint32_t a = operand(near_e);
        uint32_t b = operand(near_e);
        uint32_t c = operand(near_e);
        uint32_t d = operand(near_e);
        volatile float fa = b2f(a), fb = b2f(b), fc = b2f(c), fd = b2f(d);
        uint32_t got;
        uint32_t want;

        /* Products: single operations. */
        got = ndsXf32Pack(ndsXf32Mul(ndsXf32Unpack(a), ndsXf32Unpack(b)));
        want = f2b(fa * fb);
        if (got != want) fail("mul", a, b, got, want);

        /* Sums of admitted operands. */
        got = ndsXf32Pack(ndsXf32Add(ndsXf32Unpack(a), ndsXf32Unpack(b)));
        want = f2b(fa + fb);
        if (got != want) fail("add", a, b, got, want);

        /* Sums of products, the shape the chains use. */
        {
            NDSXf32 p = ndsXf32Mul(ndsXf32Unpack(a), ndsXf32Unpack(b));
            NDSXf32 q = ndsXf32Mul(ndsXf32Unpack(c), ndsXf32Unpack(d));
            volatile float fp = fa * fb;
            volatile float fq = fc * fd;

            got = ndsXf32Pack(ndsXf32Add(p, q));
            want = f2b(fp + fq);
            if (got != want) fail("mul+mul", f2b(fp), f2b(fq), got, want);

            got = ndsXf32Pack(ndsXf32Add(ndsXf32Add(p, q), ndsXf32Unpack(c)));
            want = f2b((float)(fp + fq) + fc);
            if (got != want) fail("mul+mul+c", f2b(fp), f2b(fq), got, want);
        }
    }
    /* Signed zeros and exact cancellation. */
    {
        static const uint32_t z[] = { 0x00000000u, 0x80000000u, 0x3f800000u, 0xbf800000u };
        unsigned x, y;

        for (x = 0; x < 4; x++)
        {
            for (y = 0; y < 4; y++)
            {
                volatile float fx = b2f(z[x]), fy = b2f(z[y]);
                uint32_t got = ndsXf32Pack(ndsXf32Add(ndsXf32Unpack(z[x]), ndsXf32Unpack(z[y])));
                uint32_t want = f2b(fx + fy);

                if (got != want) fail("zero-add", z[x], z[y], got, want);
                got = ndsXf32Pack(ndsXf32Mul(ndsXf32Unpack(z[x]), ndsXf32Unpack(z[y])));
                want = f2b(fx * fy);
                if (got != want) fail("zero-mul", z[x], z[y], got, want);
            }
        }
    }
    printf("%ld iterations, %d failures\n", iterations, sFailures);
    return (sFailures == 0) ? 0 : 1;
}
