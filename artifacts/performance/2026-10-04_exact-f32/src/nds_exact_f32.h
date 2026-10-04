/* P2-2p8 (2026-10-04): IEEE-754 binary32 multiply and add, round to nearest
 * even, on unpacked operands -- bit for bit what the soft-float calls
 * (__aeabi_fmul / __aeabi_fadd) return, for the operands the callers admit.
 *
 * A chain like the source's matrix compose, ((a*b + c*d) + e*f) + t, makes
 * one libgcc call per operation, and each call unpacks both operands, rounds,
 * and packs the result again (~27 cycles a multiply, ~34 an add, plus the
 * call and the spills around it). Here the operands are unpacked once, each
 * product and sum is rounded exactly as IEEE requires, and only the chain's
 * result is packed.
 *
 * Domain. The caller admits only zeros and normals whose unbiased exponent
 * is within +/-NDS_XF32_EXP_RANGE (ndsXf32Admit). Then every product of two
 * admitted values has an exponent in [-100, 101] and every sum of a few of
 * them stays below 2^104, and a cancelled sum is a multiple of 2^-123, so
 * nothing in a short chain overflows, and nothing is subnormal: no special
 * case remains but signed zero, which follows IEEE (x * 0 keeps the xor of
 * the signs; +0 + -0 = +0; -0 + -0 = -0; x + -x = +0). Anything else goes to
 * the caller's float path. scripts/check-exact-f32.c tests these against the
 * host's binary32 arithmetic.
 *
 * A value is m * 2^(e - 150), m in [2^23, 2^24) or m == 0, and the sign. */
#ifndef NDS_EXACT_F32_H
#define NDS_EXACT_F32_H

#include <stdint.h>

#define NDS_XF32_EXP_RANGE 50

typedef struct NDSXf32
{
    uint32_t m;
    int32_t e;
    uint32_t s;
} NDSXf32;

#if defined(__arm__)
#define NDS_XF32_INLINE static inline __attribute__((always_inline))
#else
#define NDS_XF32_INLINE static inline
#endif

/* Nonzero when `bits` is a zero or a normal within the domain. */
NDS_XF32_INLINE uint32_t ndsXf32Admit(uint32_t bits)
{
    const uint32_t e = (bits >> 23) & 0xffu;

    return (((bits << 1) == 0u) ||
            ((e - (127u - NDS_XF32_EXP_RANGE)) <=
             (2u * NDS_XF32_EXP_RANGE))) ? 1u : 0u;
}

NDS_XF32_INLINE NDSXf32 ndsXf32Unpack(uint32_t bits)
{
    NDSXf32 v;

    v.s = bits & 0x80000000u;
    v.e = (int32_t)((bits >> 23) & 0xffu);
    v.m = ((bits << 1) == 0u) ? 0u : ((bits & 0x007fffffu) | 0x00800000u);
    return v;
}

NDS_XF32_INLINE uint32_t ndsXf32Pack(NDSXf32 v)
{
    if (v.m == 0u)
    {
        return v.s;
    }
    return v.s | ((uint32_t)v.e << 23) | (v.m & 0x007fffffu);
}

NDS_XF32_INLINE NDSXf32 ndsXf32Mul(NDSXf32 a, NDSXf32 b)
{
    NDSXf32 r;
    uint64_t p;
    uint32_t lo;
    uint32_t hi;
    uint32_t m;
    uint32_t rem;
    uint32_t half;

    r.s = a.s ^ b.s;
    if ((a.m == 0u) || (b.m == 0u))
    {
        r.m = 0u;
        r.e = 0;
        return r;
    }
    p = (uint64_t)a.m * b.m; /* [2^46, 2^48) */
    lo = (uint32_t)p;
    hi = (uint32_t)(p >> 32);
    r.e = a.e + b.e - 127;
    if ((hi & 0x8000u) != 0u)
    {
        m = (hi << 8) | (lo >> 24);
        rem = lo & 0x00ffffffu;
        half = 0x00800000u;
        r.e += 1;
    }
    else
    {
        m = (hi << 9) | (lo >> 23);
        rem = lo & 0x007fffffu;
        half = 0x00400000u;
    }
    if ((rem > half) || ((rem == half) && ((m & 1u) != 0u)))
    {
        m++;
        if (m == 0x01000000u)
        {
            m = 0x00800000u;
            r.e += 1;
        }
    }
    r.m = m;
    return r;
}

NDS_XF32_INLINE NDSXf32 ndsXf32Add(NDSXf32 a, NDSXf32 b)
{
    NDSXf32 r;
    uint32_t big_m;
    uint32_t small_m;
    int32_t big_e;
    uint32_t d;
    uint32_t sum;
    uint32_t low;
    uint32_t m;
    int32_t lead;

    if (b.m == 0u)
    {
        if (a.m == 0u)
        {
            a.s &= b.s;
        }
        return a;
    }
    if (a.m == 0u)
    {
        return b;
    }
    /* Order by magnitude: the result takes the larger operand's sign. */
    if ((a.e < b.e) || ((a.e == b.e) && (a.m < b.m)))
    {
        r = a;
        a = b;
        b = r;
    }
    d = (uint32_t)(a.e - b.e);
    /* |b| < 2^(b.e - 126) <= 2^(a.e - 152): under a quarter of a's ulp, which
     * is under half the spacing on either side of a, so a + b rounds to a. */
    if (d > 25u)
    {
        return a;
    }
    big_m = a.m << 6;
    small_m = b.m << 6;
    big_e = a.e;
    if (d != 0u)
    {
        small_m = (small_m >> d) |
                  (((small_m & ((1u << d) - 1u)) != 0u) ? 1u : 0u);
    }
    if (a.s == b.s)
    {
        sum = big_m + small_m; /* < 2^31 */
    }
    else
    {
        sum = big_m - small_m;
        if (sum == 0u)
        {
            r.m = 0u;
            r.e = 0;
            r.s = 0u;
            return r;
        }
    }
    /* Leading one to bit 29. A carry into bit 30 shifts right keeping the
     * sticky bit; a left shift of more than one only follows d <= 1, when no
     * sticky bit was set, so it is exact. */
    lead = 31 - __builtin_clz(sum);
    if (lead == 30)
    {
        sum = (sum >> 1) | (sum & 1u);
        big_e += 1;
    }
    else if (lead < 29)
    {
        sum <<= (uint32_t)(29 - lead);
        big_e -= 29 - lead;
    }
    low = sum & 63u;
    m = sum >> 6;
    if ((low > 32u) || ((low == 32u) && ((m & 1u) != 0u)))
    {
        m++;
        if (m == 0x01000000u)
        {
            m = 0x00800000u;
            big_e += 1;
        }
    }
    r.m = m;
    r.e = big_e;
    r.s = a.s;
    return r;
}

#endif /* NDS_EXACT_F32_H */
