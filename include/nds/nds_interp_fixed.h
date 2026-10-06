#ifndef SSB64_NDS_INTERP_FIXED_H
#define SSB64_NDS_INTERP_FIXED_H

#include <stdint.h>
#include <string.h>

/* syInterpGetFracFrame's Bezier/Catrom arm in fixed point (2026-10-05).
 *
 * WHY. The owner, 2026-10-05: "Software floating point should not exist,
 * fixed point only" (ruling D13 re-baselines the replay digest). This arm is
 * the arc-length reparametrisation of a moving path -- Sector Z's Arwing (a
 * platform, so gameplay state on every tick it flies), Samus's rolls, the
 * Board the Platforms boards -- and was the largest soft-float site in the
 * Sector Z census: ~17 Simpson integrals of sqrt(quartic) a call, each sample
 * ~9 library calls and a square root.
 *
 * WHAT. The source's algorithm (decomp sys/interp.c) is unchanged: bisect
 * [0,1] for the frame whose arc-length integral from the segment's start
 * matches time_scale, each node a nine-sample Simpson integral, with the
 * source's 1e-5 tolerances and both of its exit tests. The arithmetic moves:
 *   - frame positions are Q22, so every node bound and Simpson sample the
 *     bisection visits (dyadics down to 2^-20) is exact;
 *   - the segment's five coefficients share one even power-of-two scale
 *     (|m| < 2^28), the quartic runs as a Q30 Horner on 32x32->64 products,
 *     and its square root is the DS math unit's 64-bit sqrt, started before
 *     the next sample's Horner and collected after it;
 *   - integrals, time_scale and the tolerance share one int64 unit,
 *     24 * 2^(half + 37) per world unit, so Simpson's /24 and the node widths
 *     are exact shifts and the loop never divides;
 *   - the result (id + frac) / (points_num - 1) is the source's binary32
 *     quotient (its dividend is exact), rounded to nearest even from the
 *     hardware divider's quotient and remainder.
 * A frame is a bisection dyadic either way, so a result differs from the
 * float source only where a branch or exit test lands within float rounding
 * of its tolerance: 99.7% of a Sector Z flight's calls are bit-identical, the
 * rest within the source's own 1e-5 (scripts/test_interp_fixed.c).
 * Inputs outside the data's domain -- non-finite or negative values, t below
 * keyframes[id] or above 1, a scale the unit cannot hold -- return FALSE and
 * the caller takes the source call.
 *
 * REUSE. Every node is a pure function of (segment, depth, min), so a path
 * remembers the nodes the previous call for the same segment visited (a
 * moving owner's consecutive ticks walk the same first nodes), and inside a
 * call a child integral takes the samples it shares with its parent (after a
 * step left, its even samples are the parent's first five; after a step right
 * its first is the parent's last). Neither can change a result. */

#if defined(__arm__) && defined(ARM9)
#include <nds/nds_r2_hwmath_unit.h>
#define NDS_IFX_ARM __attribute__((target("arm")))

/* The math unit's 64-bit square root as a pipeline: start one, run the next
 * sample's Horner while the unit works, then collect it. */
static inline void ndsIfxSqrtMode(void)
{
    NDS_R2_HWMATH_SQRTCNT = (uint16_t)NDS_R2_HWMATH_SQRT_64;
}

static inline void ndsIfxSqrtStart(uint64_t value)
{
    NDS_R2_HWMATH_SQRT_PARAM = value;
}

static inline uint32_t ndsIfxSqrtResult(void)
{
    while ((NDS_R2_HWMATH_SQRTCNT & NDS_R2_HWMATH_BUSY) != 0u)
    {
    }
    return NDS_R2_HWMATH_SQRT_RESULT;
}

static inline uint64_t ndsIfxDiv6432(uint64_t numerator, uint32_t denominator,
                                     uint32_t *remainder)
{
    int32_t rem;
    const uint64_t q = (uint64_t)ndsR2HwMathDivide6432(
        (int64_t)numerator, (int32_t)denominator, &rem);

    *remainder = (uint32_t)rem;
    return q;
}
#else
#define NDS_IFX_ARM

/* floor(sqrt(value)), bit by bit (host proof only). */
static inline uint32_t ndsIfxSqrt64(uint64_t value)
{
    uint64_t root = 0u;
    uint64_t bit = 1ull << 62;

    while (bit > value)
    {
        bit >>= 2;
    }
    while (bit != 0u)
    {
        if (value >= root + bit)
        {
            value -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)root;
}

static uint64_t sNdsIfxSqrtParam;

static inline void ndsIfxSqrtMode(void)
{
}

static inline void ndsIfxSqrtStart(uint64_t value)
{
    sNdsIfxSqrtParam = value;
}

static inline uint32_t ndsIfxSqrtResult(void)
{
    return ndsIfxSqrt64(sNdsIfxSqrtParam);
}

static inline uint64_t ndsIfxDiv6432(uint64_t numerator, uint32_t denominator,
                                     uint32_t *remainder)
{
    *remainder = (uint32_t)(numerator % denominator);
    return numerator / denominator;
}
#endif

#define NDS_IFX_ONE (1u << 22)
/* |min - max| < 0.00001F: 1e-5 * 2^22 = 41.9, so a Q22 width of 32 exits. */
#define NDS_IFX_DIFF_EXIT 42u
/* 24 * 0.00001F (binary32 0x1.4f8b58p-17) * 2^40. */
#define NDS_IFX_EPS_Q40 263882784ull
#define NDS_IFX_NODES 20u
/* From this depth on a node spans at most 2^-6 of the segment, where one
 * Simpson panel agrees with the source's four to ~1e-9 of the integral (the
 * error falls as the width to the fourth) -- below the float rounding the
 * source's own result carries -- so its first, middle and last samples are
 * enough. The host proof's match rate is unchanged from depth 5 on and first
 * moves at depth 4. */
#define NDS_IFX_DEEP 5u
#define NDS_IFX_FROM_NONE 0u
#define NDS_IFX_FROM_LEFT 1u
#define NDS_IFX_FROM_RIGHT 2u

typedef struct NDSIfxNode
{
    uint32_t min;
    uint32_t res_lo;
    uint32_t res_hi;
} NDSIfxNode;

/* The nodes the last call on `cof` visited, in order. */
typedef struct NDSIfxPath
{
    uint32_t cof[5];
    uint32_t depth;
    uint32_t age;
    NDSIfxNode node[NDS_IFX_NODES];
} NDSIfxPath;

/* A coefficient's bits times 2^shift, truncated; |result| < 2^28 when
 * shift <= 154 - its biased exponent. Zero and subnormals are 0. */
static inline NDS_IFX_ARM int32_t ndsIfxCoef(uint32_t bits, int32_t shift)
{
    const uint32_t e = (bits >> 23) & 0xffu;
    const int32_t x = (int32_t)e - 150 + shift;
    uint32_t mag;

    if (e == 0u)
    {
        return 0;
    }
    mag = (bits & 0x7fffffu) | 0x800000u;
    if (x >= 0)
    {
        mag <<= x;
    }
    else
    {
        mag = (x > -24) ? (mag >> -x) : 0u;
    }
    return ((bits & 0x80000000u) != 0u) ? -(int32_t)mag : (int32_t)mag;
}

/* A binary32 in [0, 1] as Q31 (truncated); 0 outside. */
static inline NDS_IFX_ARM int ndsIfxUnitQ31(uint32_t bits, uint32_t *out)
{
    const uint32_t e = bits >> 23; /* the sign lands above the exponent */
    const uint32_t mant = (bits & 0x7fffffu) | 0x800000u;

    if ((bits << 1) == 0u)
    {
        *out = 0u;
        return 1;
    }
    if ((e > 127u) || ((e == 127u) && ((bits & 0x7fffffu) != 0u)))
    {
        return 0;
    }
    if (e == 0u)
    {
        *out = 0u;
    }
    else if (e >= 119u)
    {
        *out = mant << (e - 119u);
    }
    else
    {
        *out = ((119u - e) < 32u) ? (mant >> (119u - e)) : 0u;
    }
    return 1;
}

/* c0 x^4 + c1 x^3 + c2 x^2 + c3 x + c4 at a Q22 frame, scaled by 2^shift, as
 * a Q30 Horner. */
static inline __attribute__((always_inline)) NDS_IFX_ARM int32_t
ndsIfxQuartic(const int32_t *m, uint32_t x_q22)
{
    const int32_t x = (int32_t)(x_q22 << 8);
    int32_t acc = m[0];

    acc = (int32_t)(((int64_t)acc * x) >> 30) + m[1];
    acc = (int32_t)(((int64_t)acc * x) >> 30) + m[2];
    acc = (int32_t)(((int64_t)acc * x) >> 30) + m[3];
    return (int32_t)(((int64_t)acc * x) >> 30) + m[4];
}

/* The speed operand: sqrt(P * 2^30) is a speed times 2^(half + 15). The
 * source clamps (-0.001, 0) to 0; any negative P is 0 here (a sum below
 * -0.001 would be a NaN in the source, which the data never reaches). */
static inline __attribute__((always_inline)) NDS_IFX_ARM uint64_t
ndsIfxSpeedOperand(int32_t p)
{
    return (p > 0) ? ((uint64_t)(uint32_t)p << 30) : 0u;
}

/* s[j] = the speed at min + j * h for j = j0, j0 + dj, ... (n >= 1 samples),
 * each square root running while the next sample's Horner does. */
static inline __attribute__((always_inline)) NDS_IFX_ARM void
ndsIfxSpeeds(const int32_t *m, uint32_t min, uint32_t h, uint32_t j0,
             uint32_t dj, uint32_t n, uint32_t *s)
{
    uint32_t j = j0;

    ndsIfxSqrtMode();
    ndsIfxSqrtStart(ndsIfxSpeedOperand(ndsIfxQuartic(m, min + j * h)));
    while (--n != 0u)
    {
        const uint64_t next =
            ndsIfxSpeedOperand(ndsIfxQuartic(m, min + (j + dj) * h));

        s[j] = ndsIfxSqrtResult();
        ndsIfxSqrtStart(next);
        j += dj;
    }
    s[j] = ndsIfxSqrtResult();
}

/* syInterpGetCubicIntegralApprox(min, min + 2^(21-k)) for node depth k, in
 * the int64 unit: sum(w_i s_i) * 2^(21-k). `parent` holds the previous
 * node's nine speeds when `from` says how this node sits in it. */
static NDS_IFX_ARM __attribute__((noinline)) int64_t
ndsIfxNode(const int32_t *m, uint32_t min, uint32_t k, const uint32_t *parent,
           uint32_t from, uint32_t *s)
{
    const uint32_t h = 1u << (18u - k);
    uint64_t odd;
    uint64_t even;
    uint32_t j;

    if (k >= NDS_IFX_DEEP)
    {
        /* One Simpson panel: s[0], s[4], s[8] (the 9-slot layout, so a
         * parent at either depth hands down its first, middle and last). */
        if (from == NDS_IFX_FROM_LEFT)
        {
            s[0] = parent[0];
            s[8] = parent[4];
            ndsIfxSpeeds(m, min, h, 4u, 4u, 1u, s);
        }
        else if (from == NDS_IFX_FROM_RIGHT)
        {
            s[0] = parent[8];
            ndsIfxSpeeds(m, min, h, 4u, 4u, 2u, s);
        }
        else
        {
            ndsIfxSpeeds(m, min, h, 0u, 4u, 3u, s);
        }
        return (int64_t)((((uint64_t)s[0] + s[8] + ((uint64_t)s[4] << 2))
                          << 2) << (21u - k));
    }
    if (from == NDS_IFX_FROM_LEFT)
    {
        for (j = 0u; j < 9u; j += 2u)
        {
            s[j] = parent[j >> 1];
        }
        ndsIfxSpeeds(m, min, h, 1u, 2u, 4u, s);
    }
    else if (from == NDS_IFX_FROM_RIGHT)
    {
        s[0] = parent[8];
        ndsIfxSpeeds(m, min, h, 1u, 1u, 8u, s);
    }
    else
    {
        ndsIfxSpeeds(m, min, h, 0u, 1u, 9u, s);
    }
    odd = (uint64_t)s[1] + s[3] + s[5] + s[7];
    even = (uint64_t)s[2] + s[4] + s[6];
    return (int64_t)(((uint64_t)s[0] + s[8] + (odd << 2) + (even << 1))
                     << (21u - k));
}

/* The source's bisection loop on (ts, eps) in the int64 unit; returns
 * frac_frame in Q22 and leaves `path` describing this call. */
static NDS_IFX_ARM __attribute__((noinline)) uint32_t
ndsIfxBisect(const int32_t *m, int64_t ts, int64_t eps, NDSIfxPath *path)
{
    uint32_t speeds[2][9];
    uint32_t min = 0u;
    uint32_t max = NDS_IFX_ONE;
    uint32_t frac;
    uint32_t known = path->depth;
    uint32_t k = 0u;
    uint32_t cur = 0u;
    uint32_t reuse = 1u;
    uint32_t have_parent = 0u;
    uint32_t from = NDS_IFX_FROM_NONE;
    int64_t res;

    do
    {
        frac = min + ((max - min) >> 1);
        if ((reuse != 0u) && (k < known) && (path->node[k].min == min))
        {
            res = (int64_t)(((uint64_t)path->node[k].res_hi << 32) |
                            path->node[k].res_lo);
            have_parent = 0u;
        }
        else
        {
            reuse = 0u;
            res = ndsIfxNode(m, min, k, speeds[cur],
                             (have_parent != 0u) ? from : NDS_IFX_FROM_NONE,
                             speeds[cur ^ 1u]);
            cur ^= 1u;
            have_parent = 1u;
            if (k < NDS_IFX_NODES)
            {
                path->node[k].min = min;
                path->node[k].res_lo = (uint32_t)(uint64_t)res;
                path->node[k].res_hi = (uint32_t)((uint64_t)res >> 32);
            }
        }
        k++;

        if (ts < res + eps)
        {
            max = frac;
            from = NDS_IFX_FROM_LEFT;
        }
        else
        {
            min = frac;
            ts -= res;
            from = NDS_IFX_FROM_RIGHT;
        }
        if ((max - min) < NDS_IFX_DIFF_EXIT)
        {
            break;
        }
    } while (((res + eps) < ts) || (ts < (res - eps)));

    path->depth = (k < NDS_IFX_NODES) ? k : NDS_IFX_NODES;
    return frac;
}

/* binary32 bits of num / (2^22 * den), rounded to nearest even. num > 0,
 * 1 <= den <= 63. */
static NDS_IFX_ARM uint32_t ndsIfxQuotientBits(uint32_t num, uint32_t den)
{
    uint32_t rem;
    const uint64_t q = ndsIfxDiv6432((uint64_t)num << 32, den, &rem);
    const uint32_t hi = (uint32_t)(q >> 32);
    const uint32_t p = (hi != 0u) ? (63u - (uint32_t)__builtin_clz(hi)) :
                                    (31u - (uint32_t)__builtin_clz((uint32_t)q));
    const uint32_t sh = p - 23u; /* q >= 2^32 / 63, so p >= 26 */
    const uint64_t below = q & ((1ull << sh) - 1ull);
    const uint64_t halfway = 1ull << (sh - 1u);
    uint32_t mant = (uint32_t)(q >> sh);
    uint32_t exp = p + 73u; /* the value is q * 2^-54 */

    if ((below > halfway) ||
        ((below == halfway) && ((rem != 0u) || ((mant & 1u) != 0u))))
    {
        mant++;
        if (mant == (1u << 24))
        {
            mant >>= 1;
            exp++;
        }
    }
    return (exp << 23) | (mant & 0x7fffffu);
}

/* syInterpGetFracFrame's Bezier/Catrom arm from the source's segment scan on:
 * the result's binary32 bits in *out_bits, or 0 (outside the domain).
 * noinline: inlined into its Thumb caller it compiles as Thumb, and its
 * 64-bit products and CLZs become libgcc calls. */
static NDS_IFX_ARM __attribute__((unused, noinline)) int
ndsIfxFracFrame(uint32_t t_bits, uint32_t keyframe_bits, uint32_t length_bits,
                const uint32_t cof_bits[5], uint32_t id, uint32_t points_num,
                NDSIfxPath *path, uint32_t *out_bits)
{
    const uint32_t e_length = (length_bits >> 23) & 0xffu;
    uint32_t t_q31;
    uint32_t kf_q31;
    uint32_t e_max = 0u;
    uint32_t frac;
    uint64_t prod;
    int64_t ts;
    int64_t eps;
    int32_t m[5];
    int32_t shift;
    int32_t half;
    int32_t sh;
    uint32_t i;

    for (i = 0u; i < 5u; i++)
    {
        const uint32_t e = (cof_bits[i] >> 23) & 0xffu;

        if (e == 0xffu)
        {
            return 0;
        }
        if (e > e_max)
        {
            e_max = e;
        }
    }
    if (((length_bits & 0x80000000u) != 0u) || (e_length == 0u) ||
        (e_length == 0xffu) || (points_num < 2u) || (points_num > 64u) ||
        (ndsIfxUnitQ31(t_bits, &t_q31) == 0) ||
        (ndsIfxUnitQ31(keyframe_bits, &kf_q31) == 0) || (t_q31 < kf_q31))
    {
        return 0;
    }
    shift = (e_max != 0u) ? (154 - (int32_t)e_max) : 0;
    shift &= ~1;
    half = shift >> 1;
    if ((half > 34) || (half < -60))
    {
        return 0;
    }
    for (i = 0u; i < 5u; i++)
    {
        m[i] = ndsIfxCoef(cof_bits[i], shift);
    }

    /* time_scale = (t - keyframes[id]) * length, times 24 * 2^(half + 37) */
    prod = (uint64_t)(t_q31 - kf_q31) *
           (uint64_t)((length_bits & 0x7fffffu) | 0x800000u);
    prod = (prod << 4) + (prod << 3);
    sh = (int32_t)e_length - 144 + half;
    if (sh >= 0)
    {
        if ((sh >= 62) || ((prod >> (62 - sh)) != 0u))
        {
            return 0;
        }
        ts = (int64_t)(prod << sh);
    }
    else
    {
        ts = (sh > -64) ? (int64_t)(prod >> -sh) : 0;
    }
    eps = (half >= 3) ? (int64_t)(NDS_IFX_EPS_Q40 << (half - 3)) :
          ((3 - half) < 64) ? (int64_t)(NDS_IFX_EPS_Q40 >> (3 - half)) : 0;

    frac = ndsIfxBisect(m, ts, eps, path);
    *out_bits = ndsIfxQuotientBits((id << 22) + frac, points_num - 1u);
    return 1;
}

#endif
