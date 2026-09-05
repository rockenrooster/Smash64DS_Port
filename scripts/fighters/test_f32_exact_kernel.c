/* Host proof of include/nds/nds_f32_exact.h against the machine's IEEE adder.
 *
 * Build and run (any host C compiler; x86-64 SSE arithmetic is binary32 with
 * round-to-nearest-even, exactly the ARM9's semantics for these operands):
 *
 *   gcc -O2 -std=c99 -I include scripts/fighters/test_f32_exact_kernel.c \
 *       -o build/test_f32_exact_kernel && build/test_f32_exact_kernel
 *
 * It compares ndsF32AddBits with `a + b` on float, bit for bit, over:
 *   1. the structured set the pose clock actually produces -- every live
 *      speed and flag1 sweep against waits 1..1024, walked the way the parser
 *      walks them (subtract until <= 0, add the next payload), so every
 *      intermediate wait the chain can reach is covered;
 *   2. the two review counterexamples (wait 1 / speed 1/3, wait 16 / 16/3);
 *   3. 40 million random pairs of finite normals across the clock's magnitude
 *      range and both signs, including exact-cancellation and carry cases.
 * Exit status 0 only when every comparison agrees. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "nds/nds_f32_exact.h"

static uint32_t bits_of(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static float float_of(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }

static unsigned long long checked;
static unsigned long long mismatches;

static void check(float a, float b)
{
    volatile float sum = a + b;             /* volatile: no fused/extended tricks */
    uint32_t ref = bits_of(sum);
    uint32_t got = ndsF32AddBits(bits_of(a), bits_of(b));

    checked++;
    if (ref != got)
    {
        if (mismatches < 10u)
        {
            printf("MISMATCH %.9g + %.9g: ref %08x got %08x\n",
                   (double)a, (double)b, ref, got);
        }
        mismatches++;
    }
}

/* One parser chain: wait starts at 0, payloads are added while wait <= 0,
 * speed is subtracted once per tick. Every add and subtract is checked. */
static void chain(float speed, const int *waits, int n)
{
    float wait = 0.0f;
    float frame = 0.0f;
    int idx = 0;
    int tick;

    while (wait <= 0.0f && idx < n)
    {
        check(wait, (float)waits[idx]);
        wait = wait + (float)waits[idx];
        idx++;
    }
    for (tick = 0; tick < 20000; tick++)
    {
        check(wait, -speed);
        check(frame, speed);
        wait = wait - speed;
        frame = frame + speed;
        while (wait <= 0.0f)
        {
            if (idx == n)
            {
                return;
            }
            check(wait, (float)waits[idx]);
            wait = wait + (float)waits[idx];
            idx++;
        }
    }
}

static uint64_t rng_state = 0x9E3779B97F4A7C15ull;
static uint32_t rng(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (uint32_t)(rng_state >> 11);
}

/* A random finite normal with exponent in [emin, emax] (unbiased). */
static float random_normal(int emin, int emax)
{
    uint32_t e = (uint32_t)(emin + (int)(rng() % (uint32_t)(emax - emin + 1)) + 127);
    uint32_t m = rng() & 0x7fffffu;
    uint32_t s = rng() & 1u;
    return float_of((s << 31) | (e << 23) | m);
}

int main(void)
{
    /* Live speeds: DObj default 1, landing heavy/light, up-special landing
     * lags, item swing rates, flag1 = 1..100 percent, rebound 16/(d*1.62+4). */
    static const float named[] = {
        1.0f, 0.5f, 1.5f, 2.0f, 0.75f, 0.28f, 0.34f, 0.3f, 0.65f, 0.17f,
        0.24f, 0.4f, 1.0f / 3.0f, 16.0f / 3.0f
    };
    float speeds[400];
    int speed_count = 0;
    int waits_single[1];
    int waits_pair[2];
    int waits_triple[3];
    size_t i;
    int w, w2, w3, d;
    unsigned long long r;

    for (i = 0; i < sizeof(named) / sizeof(named[0]); i++)
    {
        speeds[speed_count++] = named[i];
    }
    for (d = 1; d <= 100; d++)
    {
        speeds[speed_count++] = (float)d * 0.01f;                /* F_PCT_TO_DEC */
    }
    for (d = 1; d <= 100; d++)
    {
        speeds[speed_count++] = 16.0f / ((float)d * 1.62f + 4.0f); /* rebound */
    }

    /* 1. structured chains */
    for (i = 0; i < (size_t)speed_count; i++)
    {
        for (w = 1; w <= 1024; w++)
        {
            waits_single[0] = w;
            chain(speeds[i], waits_single, 1);
        }
        for (w = 1; w <= 16; w++)
        {
            for (w2 = 1; w2 <= 16; w2++)
            {
                waits_pair[0] = w; waits_pair[1] = w2;
                chain(speeds[i], waits_pair, 2);
            }
        }
        for (w = 1; w <= 8; w++)
        {
            for (w2 = 1; w2 <= 8; w2++)
            {
                for (w3 = 1; w3 <= 8; w3++)
                {
                    waits_triple[0] = w; waits_triple[1] = w2; waits_triple[2] = w3;
                    chain(speeds[i], waits_triple, 3);
                }
            }
        }
    }
    printf("structured chains: %llu operations, %llu mismatches\n", checked, mismatches);

    /* 2. the review counterexamples, explicitly */
    waits_single[0] = 1;  chain(1.0f / 3.0f, waits_single, 1);
    waits_single[0] = 16; chain(16.0f / 3.0f, waits_single, 1);

    /* 2b. the integer-to-float conversion the payload adds use, and the
     *     sign test the parser's `wait > 0` / `wait <= 0` become */
    {
        uint32_t n;
        for (n = 0; n < (1u << 24); n++)
        {
            if (ndsF32FromU32(n) != bits_of((float)n))
            {
                printf("MISMATCH ndsF32FromU32(%u): ref %08x got %08x\n",
                       n, bits_of((float)n), ndsF32FromU32(n));
                mismatches++;
            }
            checked++;
        }
        if (ndsF32BitsNonPositive(bits_of(0.0f)) != 1u ||
            ndsF32BitsNonPositive(bits_of(-0.0f)) != 1u ||
            ndsF32BitsNonPositive(bits_of(-1e-30f)) != 1u ||
            ndsF32BitsNonPositive(bits_of(1e-30f)) != 0u ||
            ndsF32BitsNonPositive(bits_of(3.0f)) != 0u)
        {
            printf("MISMATCH ndsF32BitsNonPositive\n");
            mismatches++;
        }
    }

    /* 3. random sweep */
    for (r = 0; r < 40000000ull; r++)
    {
        float a = random_normal(-24, 12);
        float b = random_normal(-24, 12);
        check(a, b);
        if ((r & 7ull) == 0ull)
        {
            check(a, -a);                 /* exact cancellation */
            check(a, a);                  /* carry out */
        }
    }
    printf("total: %llu operations, %llu mismatches\n", checked, mismatches);
    if (mismatches != 0ull)
    {
        printf("F32_EXACT_KERNEL_FAIL\n");
        return 1;
    }
    printf("F32_EXACT_KERNEL_OK the kernel matches the IEEE adder bit for bit\n");
    return 0;
}
