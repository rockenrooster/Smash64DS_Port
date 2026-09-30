/* Host proof of include/nds/nds_interp_exact.h against the source's float code.
 *
 * Build and run (x86-64 SSE arithmetic is IEEE binary32 with round to nearest
 * even, the ARM9 soft-float semantics; contraction must stay off):
 *
 *   gcc -O2 -std=c99 -ffp-contract=off -I include \
 *       scripts/test_interp_exact_kernel.c -o build/test_interp_exact_kernel -lm
 *   build/test_interp_exact_kernel
 *
 * Every integer form is compared, bit for bit, with the float operation it
 * replaces over random and structured operands (including zeros, subnormals,
 * infinities, NaNs, overflow and underflow, which must reach the library
 * fallbacks), then the quartic, the Simpson integral and the whole bisection
 * are compared with decomp sys/interp.c's own text on random Bezier segments
 * walked the way the game walks them (small monotonic steps of t, so the path
 * and sample reuse are exercised as they are on the device). Exit status 0
 * only when every comparison agrees. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nds/nds_interp_exact.h"

static uint64_t sRng = 0x9e3779b97f4a7c15ull;
static unsigned long long sChecked;
static unsigned long long sMismatch;

static uint32_t rnd32(void)
{
    sRng ^= sRng << 13;
    sRng ^= sRng >> 7;
    sRng ^= sRng << 17;
    return (uint32_t)(sRng >> 16);
}

static double rndu(void)
{
    return (double)rnd32() / 4294967296.0;
}

static int is_nan(uint32_t x)
{
    return (x & 0x7fffffffu) > 0x7f800000u;
}

/* Bits must agree, except that any two NaNs agree: a NaN's sign and payload
 * never reach the bisection's result (frac is built from min and max, which
 * are never NaN, and every compare with a NaN is false whatever its bits),
 * and this test's `a - b` = `a + (-b)` model flips a NaN operand's sign where
 * the host subtract does not. */
static void report(const char *what, uint32_t a, uint32_t b, uint32_t ref,
                   uint32_t got)
{
    sChecked++;
    if ((ref != got) && !(is_nan(ref) && is_nan(got)))
    {
        if (sMismatch < 20u)
        {
            printf("MISMATCH %s a=%08x b=%08x ref=%08x got=%08x\n", what, a, b,
                   ref, got);
        }
        sMismatch++;
    }
}

/* A random operand: mostly normals over a wide exponent band, with zeros,
 * subnormals, extremes, infinities and NaNs mixed in. */
static uint32_t rnd_operand(void)
{
    uint32_t r = rnd32() & 63u;
    uint32_t s = rnd32() & NDS_IX_SIGN;

    if (r == 0u) return s;                                   /* +-0 */
    if (r == 1u) return s | (rnd32() & 0x7fffffu);           /* subnormal */
    if (r == 2u) return s | 0x7f800000u;                     /* inf */
    if (r == 3u) return 0x7fc00000u | (rnd32() & 0x3fffffu); /* quiet NaN */
    if (r == 4u) return s | (1u << 23) | (rnd32() & 0x7fffffu);    /* min exp */
    if (r == 5u) return s | (254u << 23) | (rnd32() & 0x7fffffu);  /* max exp */
    if (r < 20u) return s | (((rnd32() % 254u) + 1u) << 23) | (rnd32() & 0x7fffffu);
    /* the kernel's own magnitude band */
    return s | (((rnd32() % 80u) + 87u) << 23) | (rnd32() & 0x7fffffu);
}

static void test_ops(unsigned long long n)
{
    unsigned long long i;

    for (i = 0; i < n; i++)
    {
        uint32_t a = rnd_operand();
        uint32_t b;
        volatile float r;

        /* near-cancellation and exponent-adjacent pairs for the adder */
        switch (rnd32() & 3u)
        {
        case 0:
            b = (a ^ NDS_IX_SIGN) + ((rnd32() & 15u) - 8u);
            break;
        case 1:
            b = a + ((rnd32() & 0xffffu) << 12);
            break;
        default:
            b = rnd_operand();
            break;
        }
        r = ndsIxF(a) * ndsIxF(b);
        report("mul", a, b, ndsIxU(r), ndsIxMul(a, b));
        r = ndsIxF(a) + ndsIxF(b);
        report("add", a, b, ndsIxU(r), ndsIxAdd(a, b));
        r = ndsIxF(a) - ndsIxF(b);
        report("sub", a, b, ndsIxU(r), ndsIxAdd(a, b ^ NDS_IX_SIGN));
        report("lt", a, b, (ndsIxF(a) < ndsIxF(b)) ? 1u : 0u, ndsIxLt(a, b));
        r = 4.0F * ndsIxF(a);
        report("x4", a, 0, ndsIxU(r), ndsIxScale(a, 2));
        r = 2.0F * ndsIxF(a);
        report("x2", a, 0, ndsIxU(r), ndsIxScale(a, 1));
        r = ndsIxF(a) / 2.0F;
        report("d2", a, 0, ndsIxU(r), ndsIxScale(a, -1));
        r = ndsIxF(a) / 8;
        report("d8", a, 0, ndsIxU(r), ndsIxScale(a, -3));
    }
}

/* ---- decomp sys/interp.c, word for word ---- */
#define BIQUAD(x) ((x) * (x) * (x) * (x))
#define CUBE(x) ((x) * (x) * (x))
#define SQUARE(x) ((x) * (x))

static float syInterpGetQuartSum(float x, float *cof)
{
    float sum = cof[0] * BIQUAD(x) + cof[1] * CUBE(x) + cof[2] * SQUARE(x) + cof[3] * x + cof[4];

    if ((sum < 0.0F) && (sum > -0.001F))
    {
        sum = 0.0F;
    }
    return sqrtf(sum);
}

static float syInterpGetCubicIntegralApprox(float t, float f, float *cof)
{
    float factor = (f - t) / 8;
    float sum = 0.0F;
    float time_scale = t + factor;
    int i;

    for (i = 2; i < 9; i++)
    {
        if (!(i & 1))
        {
            sum += 4.0F * syInterpGetQuartSum(time_scale, cof);
        }
        else sum += 2.0F * syInterpGetQuartSum(time_scale, cof);

        time_scale += factor;
    }
    return ((syInterpGetQuartSum(t, cof) + sum + syInterpGetQuartSum(f, cof)) * factor) / 3.0F;
}

static float source_bisect(float *cof, float time_scale)
{
    float frac_frame;
    float min = 0.0F;
    float max = 1.0F;
    float res;
    float diff;

    do
    {
        frac_frame = (min + max) / 2.0F;
        res = syInterpGetCubicIntegralApprox(min, frac_frame, cof);

        if (time_scale < (res + 0.00001F))
        {
            max = frac_frame;
        }
        else
        {
            min = frac_frame;
            time_scale -= res;
        }
        diff = (min < max) ? -(min - max) : min - max;

        if (diff < 0.00001F)
        {
            break;
        }
    }
    while ((res + 0.00001F) < time_scale || time_scale < (res - 0.00001F));

    return frac_frame;
}

/* speed^2 of a cubic Bezier segment, as a quartic in u (float coefficients) */
static void bezier_quartic(float *cof, double scale)
{
    double p[4][3];
    double A[3], B[3], C[3];
    double c4 = 0, c3 = 0, c2 = 0, c1 = 0, c0 = 0;
    int i, k;

    for (i = 0; i < 4; i++)
    {
        for (k = 0; k < 3; k++)
        {
            p[i][k] = (rndu() * 2.0 - 1.0) * scale;
        }
    }
    for (k = 0; k < 3; k++)
    {
        double a = -p[0][k] + 3 * p[1][k] - 3 * p[2][k] + p[3][k];
        double b = 3 * p[0][k] - 6 * p[1][k] + 3 * p[2][k];
        double c = -3 * p[0][k] + 3 * p[1][k];

        A[k] = 3 * a;
        B[k] = 2 * b;
        C[k] = c;
        c4 += A[k] * A[k];
        c3 += 2 * A[k] * B[k];
        c2 += B[k] * B[k] + 2 * A[k] * C[k];
        c1 += 2 * B[k] * C[k];
        c0 += C[k] * C[k];
    }
    cof[0] = (float)c4;
    cof[1] = (float)c3;
    cof[2] = (float)c2;
    cof[3] = (float)c1;
    cof[4] = (float)c0;
}

static void test_quart(unsigned long long n)
{
    unsigned long long i;

    for (i = 0; i < n; i++)
    {
        float cof[5];
        uint32_t cb[5];
        uint32_t x;
        int k;

        if ((rnd32() & 7u) == 0u)
        {
            for (k = 0; k < 5; k++) cof[k] = ndsIxF(rnd_operand());
        }
        else
        {
            bezier_quartic(cof, pow(10.0, rndu() * 5.0));
        }
        for (k = 0; k < 5; k++) cb[k] = ndsIxU(cof[k]);
        /* dyadic x (the bisection's) or any float in [0, 1] */
        if (rnd32() & 1u)
        {
            uint32_t d = rnd32() % 22u;
            x = ndsIxU((float)((double)(rnd32() & ((1u << d) - 1u)) / (double)(1u << d)));
        }
        else
        {
            x = ndsIxU((float)rndu());
        }
        report("quart", x, cb[0], ndsIxU(syInterpGetQuartSum(ndsIxF(x), cof)),
               ndsIxQuart(x, cb));
    }
}

static void test_bisect(unsigned segments, unsigned calls)
{
    unsigned s, c;
    NDSIxPath path;
    uint32_t reused = 0, computed = 0;

    for (s = 0; s < segments; s++)
    {
        float cof[5];
        uint32_t cb[5];
        float total;
        double step;
        double pos;
        int k;

        bezier_quartic(cof, pow(10.0, rndu() * 4.5));
        if ((s % 97u) == 0u)
        {
            cof[rnd32() % 5u] = ndsIxF(rnd_operand()); /* odd segments too */
        }
        for (k = 0; k < 5; k++) cb[k] = ndsIxU(cof[k]);
        total = syInterpGetCubicIntegralApprox(0.0F, 1.0F, cof);
        memset(&path, 0, sizeof(path));
        memcpy(path.cof, cb, sizeof(cb));
        step = (1.05 / (double)calls) * (0.5 + rndu());
        pos = rndu() * 0.1;
        for (c = 0; c < calls; c++)
        {
            float ts = (float)(pos * (double)total);
            uint32_t ref = ndsIxU(source_bisect(cof, ts));
            uint32_t got = ndsIxBisect(cb, ndsIxU(ts), &path, &reused, &computed);

            report("bisect", ndsIxU(ts), cb[0], ref, got);
            pos += step;
            if ((rnd32() & 63u) == 0u) pos = rndu() * 1.05; /* a jump */
        }
    }
    printf("bisect nodes reused %u computed %u\n", reused, computed);
}

int main(void)
{
    if ((ndsIxU(0.00001F) != NDS_IX_F32_EPS) ||
        (ndsIxU(-0.001F) != NDS_IX_F32_NEG_MILLI) ||
        (ndsIxU(3.0F) != NDS_IX_F32_THREE) || (ndsIxU(1.0F) != NDS_IX_F32_ONE))
    {
        printf("FAIL: constant bit patterns\n");
        return 1;
    }
    test_ops(20000000ull);
    printf("ops: %llu checked, %llu mismatches\n", sChecked, sMismatch);
    test_quart(4000000ull);
    printf("quart: %llu checked, %llu mismatches\n", sChecked, sMismatch);
    test_bisect(3000u, 300u);
    printf("total: %llu checked, %llu mismatches\n", sChecked, sMismatch);
    return (sMismatch == 0u) ? 0 : 1;
}
