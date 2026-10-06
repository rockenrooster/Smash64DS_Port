/* Host check of include/nds/nds_interp_fixed.h against the source's float
 * syInterpGetFracFrame (decomp sys/interp.c).
 *
 * Build and run (x86-64 SSE arithmetic is IEEE binary32 with round to nearest
 * even, the ARM9 soft-float semantics; contraction must stay off):
 *
 *   gcc -O2 -std=gnu99 -ffp-contract=off -I include \
 *       scripts/test_interp_fixed.c -o build/test_interp_fixed -lm
 *   build/test_interp_fixed
 *
 * Random Bezier segments over a wide range of scales are walked the way the
 * game walks a path (small monotonic steps of t, one path per walk, so node
 * and sample reuse are exercised), and every result is compared with the
 * float source's. The fixed arm keeps the source's bisection, so a result
 * may differ only where a branch or exit test lands within float rounding of
 * its 1e-5 tolerance. Exit status 0 when at least 99% of the results are
 * bit-identical at every scale, no in-domain call declines, and the
 * quotient's rounding agrees with the float divide on every frame. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nds/nds_interp_fixed.h"

#define BIQUAD(x) ((x) * (x) * (x) * (x))
#define CUBE(x) ((x) * (x) * (x))
#define SQUARE(x) ((x) * (x))

static uint32_t bits_of(float f)
{
    uint32_t b;

    memcpy(&b, &f, sizeof(b));
    return b;
}

static float float_of(uint32_t b)
{
    float f;

    memcpy(&f, &b, sizeof(f));
    return f;
}

/* decomp sys/interp.c, verbatim apart from names */
static float src_quart(float x, const float *cof)
{
    float sum = cof[0] * BIQUAD(x) + cof[1] * CUBE(x) + cof[2] * SQUARE(x) +
                cof[3] * x + cof[4];

    if ((sum < 0.0F) && (sum > -0.001F))
    {
        sum = 0.0F;
    }
    return sqrtf(sum);
}

static float src_integral(float t, float f, const float *cof)
{
    float factor = (f - t) / 8;
    float sum = 0.0F;
    float time_scale = t + factor;
    int i;

    for (i = 2; i < 9; i++)
    {
        if (!(i & 1))
        {
            sum += 4.0F * src_quart(time_scale, cof);
        }
        else sum += 2.0F * src_quart(time_scale, cof);

        time_scale += factor;
    }
    return ((src_quart(t, cof) + sum + src_quart(f, cof)) * factor) / 3.0F;
}

static float src_frac(const float *keyframes, const float *quartics,
                      int points_num, float length, float t, int *id_out)
{
    const float *point = keyframes;
    int id = 0;
    float frac_frame, time_scale, min = 0.0F, max = 1.0F, res, diff;

    while (point[1] < t)
    {
        id++;
        point++;
    }
    time_scale = (t - keyframes[id]) * length;
    do
    {
        frac_frame = (min + max) / 2.0F;
        res = src_integral(min, frac_frame, quartics + (id * 5));

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
    } while ((res + 0.00001F) < time_scale || time_scale < (res - 0.00001F));

    *id_out = id;
    return ((float)id + frac_frame) / ((float)points_num - 1.0F);
}

static uint64_t sRng = 0x9e3779b97f4a7c15ull;

static uint32_t rnd32(void)
{
    sRng ^= sRng << 13;
    sRng ^= sRng >> 7;
    sRng ^= sRng << 17;
    return (uint32_t)(sRng >> 16);
}

static double rnd_coord(int scale)
{
    return ((int)(rnd32() % 2001u) - 1000) / 100.0 * ldexp(1.0, scale);
}

int main(void)
{
    int failed = 0;
    int scale;
    uint32_t den, j;
    unsigned long quotient_checks = 0;

    /* The final divide: every frame the bisection can produce, every count. */
    for (den = 1u; den <= 63u; den++)
    {
        for (j = 0u; j < 2000u; j++)
        {
            const uint32_t id = rnd32() % den;
            const uint32_t depth = 1u + rnd32() % 17u;
            const uint32_t frac = ((rnd32() % (1u << depth)) | 1u)
                                  << (22u - depth);
            const float ref = ((float)id + float_of(0) +
                               (float)frac / (float)NDS_IFX_ONE) /
                              ((float)(den + 1u) - 1.0F);
            const uint32_t got = ndsIfxQuotientBits((id << 22) + frac, den);

            quotient_checks++;
            if (bits_of(ref) != got)
            {
                if (failed < 10)
                {
                    printf("QUOTIENT id=%u frac=%u den=%u ref=%08x got=%08x\n",
                           id, frac, den, bits_of(ref), got);
                }
                failed++;
            }
        }
    }
    printf("quotient: %lu checks\n", quotient_checks);

    for (scale = -8; scale <= 16; scale += 4)
    {
        unsigned long calls = 0, same = 0, declined = 0;
        double max_diff = 0.0;
        int r;

        for (r = 0; r < 200; r++)
        {
            /* Two Bezier segments: B'(u) = a u^2 + b u + c per axis, and the
             * speed^2 quartic |B'(u)|^2 as the data stores it. */
            float quartics[10];
            float keyframes[3];
            float seg_len[2];
            float length;
            NDSIfxPath path;
            float t, step;
            int s;

            for (s = 0; s < 2; s++)
            {
                double a[3], b[3], c[3];
                double c0 = 0, c1 = 0, c2 = 0, c3 = 0, c4 = 0;
                int k;

                for (k = 0; k < 3; k++)
                {
                    a[k] = rnd_coord(scale);
                    b[k] = rnd_coord(scale);
                    c[k] = rnd_coord(scale);
                    c0 += a[k] * a[k];
                    c1 += 2 * a[k] * b[k];
                    c2 += b[k] * b[k] + 2 * a[k] * c[k];
                    c3 += 2 * b[k] * c[k];
                    c4 += c[k] * c[k];
                }
                quartics[s * 5 + 0] = (float)c0;
                quartics[s * 5 + 1] = (float)c1;
                quartics[s * 5 + 2] = (float)c2;
                quartics[s * 5 + 3] = (float)c3;
                quartics[s * 5 + 4] = (float)c4;
                seg_len[s] = src_integral(0.0F, 0.5F, quartics + s * 5) +
                             src_integral(0.5F, 1.0F, quartics + s * 5);
            }
            length = seg_len[0] + seg_len[1];
            keyframes[0] = 0.0F;
            keyframes[1] = seg_len[0] / length;
            keyframes[2] = 1.0F;
            memset(&path, 0, sizeof(path));
            step = (1.0F / 180.0F) * (0.5F + (float)(rnd32() & 1023u) / 1024.0F);
            for (t = 0.0F; t <= 1.0F; t += step)
            {
                int id;
                uint32_t cof[5];
                uint32_t got;
                const float ref = src_frac(keyframes, quartics, 3, length, t,
                                           &id);

                memcpy(cof, quartics + id * 5, sizeof(cof));
                if (memcmp(path.cof, cof, sizeof(cof)) != 0)
                {
                    memcpy(path.cof, cof, sizeof(cof));
                    path.depth = 0u;
                }
                if (ndsIfxFracFrame(bits_of(t), bits_of(keyframes[id]),
                                    bits_of(length), cof, (uint32_t)id, 3u,
                                    &path, &got) == 0)
                {
                    declined++;
                    continue;
                }
                calls++;
                if (bits_of(ref) == got)
                {
                    same++;
                }
                else if (fabs((double)ref - float_of(got)) > max_diff)
                {
                    max_diff = fabs((double)ref - float_of(got));
                }
            }
        }
        printf("scale 2^%-3d calls %lu identical %.2f%% declined %lu "
               "max|diff| %.3g\n",
               scale, calls, 100.0 * same / (calls ? calls : 1), declined,
               max_diff);
        if ((declined != 0u) || (same * 100u < calls * 99u))
        {
            failed++;
        }
    }
    printf("%s\n", failed ? "FAIL" : "PASS");
    return failed ? 1 : 0;
}
