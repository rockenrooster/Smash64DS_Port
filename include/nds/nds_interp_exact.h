#ifndef SSB64_NDS_INTERP_EXACT_H
#define SSB64_NDS_INTERP_EXACT_H

#include <math.h>
#include <stdint.h>
#include <string.h>

/* syInterpGetFracFrame's arc-length bisection on binary32 BIT PATTERNS.
 *
 * WHY. The bisection (decomp sys/interp.c) is ~17 nine-sample Simpson
 * integrals of sqrt(quartic): per computed node ~41 adds, ~43 multiplies, six
 * square roots and a divide, every one a libgcc soft-float call from Thumb
 * code. Sector Z's Arwing -- a moving platform, so its path is gameplay state
 * on every tick -- makes ~1,950 calls a match at ~10 new nodes each, and the
 * float operations are under half of what a node cost: the rest was Thumb
 * call marshalling, a linear sample search and seven multiplies by 4.0 or 2.0.
 *
 * WHAT. Every operation below is the source's, in the source's order, with the
 * same rounding (IEEE-754 binary32, round to nearest even):
 *   - multiply and add are the library's on the ARM9 (see the ARM9 note at
 *     ndsIxMul); on the host they run as integer arithmetic when both
 *     operands and the result are normal (zero operands handled exactly),
 *     and every other class -- subnormal, infinity, NaN, overflow,
 *     underflow -- takes the compiler's own float operation;
 *   - x * 4.0F, x * 2.0F, x / 8 and x / 2.0F are exact scalings of a normal x
 *     to a normal result and are done on the exponent, otherwise by the same
 *     library operation;
 *   - compares are IEEE (NaN unordered, -0 == +0) on the bits;
 *   - the square root is the build's own sqrtf and the divide by 3.0F the
 *     library's divide, exactly the calls the source makes.
 * Samples are reused only where the source would compute the same x bits:
 * after a step left the child's even samples are the parent's first five,
 * after a step right its first sample is the parent's last.
 *
 * PROOF. scripts/test_interp_exact_kernel.c compiles this header on the host
 * and compares every piece, and the whole bisection, bit for bit with the
 * source's float code (x86-64 SSE binary32). The ARM9 build keeps the
 * run-time oracle (gNdsInterpFracOracle) that runs the source beside it.
 * Keep both green before changing a line here. */

#if defined(__arm__)
#define NDS_IX_ARM __attribute__((target("arm")))
#else
#define NDS_IX_ARM
#endif
#define NDS_IX_INLINE static inline __attribute__((always_inline)) NDS_IX_ARM
#define NDS_IX_OUTLINE static __attribute__((noinline)) NDS_IX_ARM

#define NDS_IX_F32_ZERO 0x00000000u
#define NDS_IX_F32_ONE 0x3f800000u
#define NDS_IX_F32_THREE 0x40400000u
#define NDS_IX_F32_EPS 0x3727c5acu         /* 0.00001F */
#define NDS_IX_F32_NEG_MILLI 0xba83126fu   /* -0.001F */
#define NDS_IX_SIGN 0x80000000u

/* The bisection stops once the interval is under 1e-5: seventeen halvings. */
#define NDS_IX_PATH_DEPTH 20u

typedef struct NDSIxPathNode
{
    uint32_t min_bits;
    uint32_t frac_bits;
    uint32_t res_bits;
} NDSIxPathNode;

/* The previous call's bisection for one set of segment coefficients: its
 * first `depth` nodes are exactly the ones that call visited. */
typedef struct NDSIxPath
{
    uint32_t cof[5];
    uint32_t depth; /* 0 = empty */
    uint32_t age;
    NDSIxPathNode node[NDS_IX_PATH_DEPTH];
} NDSIxPath;

/* One Simpson integral's nine samples, in the source's x order: t, then
 * t + k * factor for k = 1..7, then f. */
typedef struct NDSIxSamples
{
    uint32_t valid;
    uint32_t x[9];
    uint32_t q[9];
} NDSIxSamples;

#define NDS_IX_PARENT_NONE 0u
#define NDS_IX_PARENT_LEFT 1u  /* child is the left half: child x[2i] = parent x[i] */
#define NDS_IX_PARENT_RIGHT 2u /* child starts at the parent's end: x[0] = x[8] */

static inline float ndsIxF(uint32_t bits)
{
    float f;

    memcpy(&f, &bits, sizeof(f));
    return f;
}

static inline uint32_t ndsIxU(float f)
{
    uint32_t bits;

    memcpy(&bits, &f, sizeof(bits));
    return bits;
}

/* The library's own operations, for every operand class the integer forms
 * decline. Out of line so the rare path stays out of the hot code. */
NDS_IX_OUTLINE __attribute__((unused)) uint32_t ndsIxLibMul(uint32_t a,
                                                             uint32_t b)
{
    volatile float r = ndsIxF(a) * ndsIxF(b);

    return ndsIxU(r);
}

NDS_IX_OUTLINE __attribute__((unused)) uint32_t ndsIxLibAdd(uint32_t a,
                                                             uint32_t b)
{
    volatile float r = ndsIxF(a) + ndsIxF(b);

    return ndsIxU(r);
}

NDS_IX_OUTLINE uint32_t ndsIxLibDiv3(uint32_t a)
{
    volatile float r = ndsIxF(a) / 3.0F;

    return ndsIxU(r);
}

NDS_IX_OUTLINE uint32_t ndsIxLibSqrt(uint32_t a)
{
    volatile float r = sqrtf(ndsIxF(a));

    return ndsIxU(r);
}

/* x * 2^k, the source's `4.0F * q`, `2.0F * q`, `(f - t) / 8` and
 * `(min + max) / 2.0F`. */
NDS_IX_OUTLINE uint32_t ndsIxLibScale(uint32_t a, int32_t k)
{
    volatile float r;

    switch (k)
    {
    case 2:
        r = 4.0F * ndsIxF(a);
        break;
    case 1:
        r = 2.0F * ndsIxF(a);
        break;
    case -1:
        r = ndsIxF(a) / 2.0F;
        break;
    default:
        r = ndsIxF(a) / 8;
        break;
    }
    return ndsIxU(r);
}

#if defined(__arm__) && defined(ARM9)
/* ARM9 (2026-09-30): the library's own binary32 multiply and add. The
 * integer forms below are IEEE round-to-nearest-even operations on this
 * operand class (the host proof), and so are __aeabi_fmul / __aeabi_fadd --
 * which run from ITCM, where the inlined integer forms grew this kernel to
 * ~12.5 KB of ARM code in main RAM, over the 8 KB instruction cache: the
 * Sector Z Arwing's P95 went UP 48K with them. Soft-float passes floats in
 * core registers, so the reinterpretation is free. The host build keeps the
 * integer forms, so the proof still covers every other line here. */
NDS_IX_INLINE uint32_t ndsIxMul(uint32_t a, uint32_t b)
{
    return ndsIxU(ndsIxF(a) * ndsIxF(b));
}

NDS_IX_INLINE uint32_t ndsIxAdd(uint32_t a, uint32_t b)
{
    return ndsIxU(ndsIxF(a) + ndsIxF(b));
}
#else
NDS_IX_OUTLINE uint32_t ndsIxMulSpecial(uint32_t a, uint32_t b)
{
    const uint32_t ea = (a >> 23) & 0xffu;
    const uint32_t eb = (b >> 23) & 0xffu;

    /* Zero times a finite value is the signed zero; zero times infinity or
     * NaN, and every subnormal, is the library's. */
    if ((((a << 1) == 0u) && (eb != 0xffu)) ||
        (((b << 1) == 0u) && (ea != 0xffu)))
    {
        return (a ^ b) & NDS_IX_SIGN;
    }
    return ndsIxLibMul(a, b);
}

/* a * b. */
NDS_IX_INLINE uint32_t ndsIxMul(uint32_t a, uint32_t b)
{
    const uint32_t ea = (a >> 23) & 0xffu;
    const uint32_t eb = (b >> 23) & 0xffu;
    uint64_t p;
    uint32_t hi, lo, m, rem, half;
    uint32_t e;

    if (((ea - 1u) >= 254u) || ((eb - 1u) >= 254u))
    {
        return ndsIxMulSpecial(a, b);
    }
    p = (uint64_t)((a & 0x7fffffu) | 0x800000u) *
        (uint64_t)((b & 0x7fffffu) | 0x800000u);
    hi = (uint32_t)(p >> 32);
    lo = (uint32_t)p;
    e = ea + eb - 127u;
    /* p is in [2^46, 2^48): keep 24 bits, round on the rest. */
    if ((hi & 0x8000u) != 0u)
    {
        m = (hi << 8) | (lo >> 24);
        rem = lo & 0xffffffu;
        half = 0x800000u;
        e++;
    }
    else
    {
        m = (hi << 9) | (lo >> 23);
        rem = lo & 0x7fffffu;
        half = 0x400000u;
    }
    if ((rem > half) || ((rem == half) && ((m & 1u) != 0u)))
    {
        m++;
        if (m == (1u << 24))
        {
            m = 1u << 23;
            e++;
        }
    }
    /* e is biased and unsigned: a product below the normal range wrapped. */
    if ((e - 1u) >= 254u)
    {
        return ndsIxLibMul(a, b);
    }
    return ((a ^ b) & NDS_IX_SIGN) | (e << 23) | (m & 0x7fffffu);
}

NDS_IX_OUTLINE uint32_t ndsIxAddSpecial(uint32_t a, uint32_t b)
{
    /* x + 0 = x exactly; -0 + -0 = -0 and every other zero sum is +0. */
    if ((a & 0x7fffffffu) == 0u)
    {
        return ((b & 0x7fffffffu) != 0u) ? b : (a & b);
    }
    if ((b & 0x7fffffffu) == 0u)
    {
        return a;
    }
    return ndsIxLibAdd(a, b);
}

/* a + b. The jam argument (as in SoftFloat's shiftRightJam): significands
 * carry three zero bits below the unit, an aligned-away remainder sets bit 0,
 * so an inexact difference never shows the exact tie pattern 100b. */
NDS_IX_INLINE uint32_t ndsIxAdd(uint32_t a, uint32_t b)
{
    uint32_t ea = (a >> 23) & 0xffu;
    uint32_t eb = (b >> 23) & 0xffu;
    uint32_t ma, mb, m, e, d, grs;

    if (((ea - 1u) >= 254u) || ((eb - 1u) >= 254u))
    {
        return ndsIxAddSpecial(a, b);
    }
    /* Larger magnitude first; for normals the bit order is the value order. */
    if ((a & 0x7fffffffu) < (b & 0x7fffffffu))
    {
        uint32_t t = a;

        a = b;
        b = t;
        t = ea;
        ea = eb;
        eb = t;
    }
    ma = ((a & 0x7fffffu) | 0x800000u) << 3;
    mb = ((b & 0x7fffffu) | 0x800000u) << 3;
    d = ea - eb;
    if (d != 0u)
    {
        if (d >= 27u)
        {
            mb = 1u;
        }
        else
        {
            mb = (mb >> d) | (((mb << (32u - d)) != 0u) ? 1u : 0u);
        }
    }
    e = ea;
    if (((a ^ b) & NDS_IX_SIGN) == 0u)
    {
        m = ma + mb;
        if ((m & (1u << 27)) != 0u)
        {
            m = (m >> 1) | (m & 1u);
            e++;
        }
    }
    else
    {
        uint32_t lz;

        m = ma - mb;
        if (m == 0u)
        {
            return 0u; /* exact cancellation is +0 */
        }
        /* The leading one belongs at bit 26. */
        lz = (uint32_t)__builtin_clz(m) - 5u;
        m <<= lz;
        e -= lz;
    }
    grs = m & 7u;
    m >>= 3;
    if ((grs > 4u) || ((grs == 4u) && ((m & 1u) != 0u)))
    {
        m++;
        if (m == (1u << 24))
        {
            m >>= 1;
            e++;
        }
    }
    if ((e - 1u) >= 254u)
    {
        return ndsIxLibAdd(a, b); /* overflow or a subnormal sum */
    }
    return (a & NDS_IX_SIGN) | (e << 23) | (m & 0x7fffffu);
}
#endif

/* x * 2^k for k in {2, 1, -1, -3}: exact on the exponent while x and the
 * result are normal. */
NDS_IX_INLINE uint32_t ndsIxScale(uint32_t x, int32_t k)
{
    const uint32_t e = (x >> 23) & 0xffu;

    if ((x & 0x7fffffffu) == 0u)
    {
        return x;
    }
    if (((e - 1u) < 254u) && (((e + (uint32_t)k) - 1u) < 254u))
    {
        return x + ((uint32_t)k << 23);
    }
    return ndsIxLibScale(x, k);
}

/* a < b, IEEE: NaN is unordered and -0 == +0. */
NDS_IX_INLINE uint32_t ndsIxLt(uint32_t a, uint32_t b)
{
    if (((a & 0x7fffffffu) > 0x7f800000u) ||
        ((b & 0x7fffffffu) > 0x7f800000u))
    {
        return 0u;
    }
    if (((a | b) << 1) == 0u)
    {
        return 0u;
    }
    if (((a ^ b) & NDS_IX_SIGN) != 0u)
    {
        return a >> 31;
    }
    return ((a & NDS_IX_SIGN) != 0u) ? ((a > b) ? 1u : 0u) :
                                       ((a < b) ? 1u : 0u);
}

/* syInterpGetQuartSum: sqrt(c0 x^4 + c1 x^3 + c2 x^2 + c3 x + c4), with the
 * source's macro products (((x x) x) x) and its left-to-right sum, and its
 * clamp of (-0.001, 0) to zero. */
NDS_IX_OUTLINE uint32_t ndsIxQuart(uint32_t x, const uint32_t *c)
{
    const uint32_t x2 = ndsIxMul(x, x);
    const uint32_t x3 = ndsIxMul(x2, x);
    const uint32_t x4 = ndsIxMul(x3, x);
    uint32_t s;

    s = ndsIxAdd(ndsIxMul(c[0], x4), ndsIxMul(c[1], x3));
    s = ndsIxAdd(s, ndsIxMul(c[2], x2));
    s = ndsIxAdd(s, ndsIxMul(c[3], x));
    s = ndsIxAdd(s, c[4]);
    if ((ndsIxLt(s, NDS_IX_F32_ZERO) != 0u) &&
        (ndsIxLt(NDS_IX_F32_NEG_MILLI, s) != 0u))
    {
        s = NDS_IX_F32_ZERO;
    }
    return ndsIxLibSqrt(s);
}

/* syInterpGetCubicIntegralApprox(t, f, cof). `parent` holds the samples of
 * the integral the bisection computed one step earlier and `relation` says
 * how this interval sits in it; `out` receives this integral's samples. */
NDS_IX_OUTLINE uint32_t ndsIxIntegral(uint32_t t, uint32_t f,
                                      const uint32_t *cof,
                                      const NDSIxSamples *parent,
                                      uint32_t relation, NDSIxSamples *out)
{
    const uint32_t factor = ndsIxScale(ndsIxAdd(f, t ^ NDS_IX_SIGN), -3);
    uint32_t sum = NDS_IX_F32_ZERO;
    uint32_t ts = ndsIxAdd(t, factor);
    uint32_t q;
    uint32_t k;

    if ((parent == NULL) || (parent->valid == 0u))
    {
        relation = NDS_IX_PARENT_NONE;
    }
    for (k = 1u; k < 8u; k++)
    {
        if ((relation == NDS_IX_PARENT_LEFT) && ((k & 1u) == 0u) &&
            (parent->x[k >> 1] == ts))
        {
            q = parent->q[k >> 1];
        }
        else
        {
            q = ndsIxQuart(ts, cof);
        }
        out->x[k] = ts;
        out->q[k] = q;
        /* source i = k + 1: even i adds 4.0F * q, odd i adds 2.0F * q */
        sum = ndsIxAdd(sum, ndsIxScale(q, ((k & 1u) != 0u) ? 2 : 1));
        ts = ndsIxAdd(ts, factor);
    }
    if ((relation == NDS_IX_PARENT_LEFT) && (parent->x[0] == t))
    {
        q = parent->q[0];
    }
    else if ((relation == NDS_IX_PARENT_RIGHT) && (parent->x[8] == t))
    {
        q = parent->q[8];
    }
    else
    {
        q = ndsIxQuart(t, cof);
    }
    out->x[0] = t;
    out->q[0] = q;
    if ((relation == NDS_IX_PARENT_LEFT) && (parent->x[4] == f))
    {
        out->q[8] = parent->q[4];
    }
    else
    {
        out->q[8] = ndsIxQuart(f, cof);
    }
    out->x[8] = f;
    out->valid = 1u;
    /* ((q(t) + sum + q(f)) * factor) / 3.0F */
    return ndsIxLibDiv3(ndsIxMul(ndsIxAdd(ndsIxAdd(q, sum), out->q[8]),
                                 factor));
}

/* The Bezier/Catrom arm of syInterpGetFracFrame from `time_scale` on:
 * returns frac_frame's bits. `path` answers the nodes the previous call for
 * these coefficients already computed and is rewritten to describe this call. */
NDS_IX_OUTLINE uint32_t ndsIxBisect(const uint32_t *cof, uint32_t time_scale,
                                    NDSIxPath *path, uint32_t *nodes_reused,
                                    uint32_t *nodes_computed)
{
    NDSIxSamples samples[2];
    uint32_t min = NDS_IX_F32_ZERO;
    uint32_t max = NDS_IX_F32_ONE;
    uint32_t frac, res, diff;
    uint32_t known = path->depth;
    uint32_t depth = 0u;
    uint32_t cur = 0u;
    uint32_t reuse = 1u;
    uint32_t relation = NDS_IX_PARENT_NONE;

    samples[0].valid = 0u;
    samples[1].valid = 0u;
    do
    {
        frac = ndsIxScale(ndsIxAdd(min, max), -1);
        if ((reuse != 0u) && (depth < known) &&
            (path->node[depth].min_bits == min) &&
            (path->node[depth].frac_bits == frac))
        {
            res = path->node[depth].res_bits;
            /* A reused node carries no samples: the next computed integral
             * starts without a parent. */
            samples[cur].valid = 0u;
            (*nodes_reused)++;
        }
        else
        {
            reuse = 0u;
            res = ndsIxIntegral(min, frac, cof, &samples[cur], relation,
                                &samples[cur ^ 1u]);
            cur ^= 1u;
            if (depth < NDS_IX_PATH_DEPTH)
            {
                path->node[depth].min_bits = min;
                path->node[depth].frac_bits = frac;
                path->node[depth].res_bits = res;
            }
            (*nodes_computed)++;
        }
        depth++;

        if (ndsIxLt(time_scale, ndsIxAdd(res, NDS_IX_F32_EPS)) != 0u)
        {
            max = frac;
            relation = NDS_IX_PARENT_LEFT;
        }
        else
        {
            min = frac;
            time_scale = ndsIxAdd(time_scale, res ^ NDS_IX_SIGN);
            relation = NDS_IX_PARENT_RIGHT;
        }
        /* diff = (min < max) ? -(min - max) : min - max; */
        diff = ndsIxAdd(min, max ^ NDS_IX_SIGN);
        if (ndsIxLt(min, max) != 0u)
        {
            diff ^= NDS_IX_SIGN;
        }
        if (ndsIxLt(diff, NDS_IX_F32_EPS) != 0u)
        {
            break;
        }
    } while ((ndsIxLt(ndsIxAdd(res, NDS_IX_F32_EPS), time_scale) != 0u) ||
             (ndsIxLt(time_scale, ndsIxAdd(res, NDS_IX_F32_EPS ^ NDS_IX_SIGN)) !=
              0u));

    path->depth = (depth < NDS_IX_PATH_DEPTH) ? depth : NDS_IX_PATH_DEPTH;
    return frac;
}

#endif
