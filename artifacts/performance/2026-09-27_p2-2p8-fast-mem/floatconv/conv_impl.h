/* Shared by the host test and (copied into) src/nds/nds_float_conv.c. */
#include <stdint.h>

static inline uint32_t ndsConvU32MagToF32Bits(uint32_t m) /* m != 0 */
{
    uint32_t msb = 31u - (uint32_t)__builtin_clz(m);
    uint32_t mant;
    uint32_t rem;
    uint32_t s;

    if (msb <= 23u)
    {
        return (m << (23u - msb)) + ((msb + 126u) << 23);
    }
    s = msb - 23u;               /* 1..8 */
    mant = m >> s;
    rem = m << (32u - s);        /* the dropped bits, top-aligned */
    if ((rem > 0x80000000u) || ((rem == 0x80000000u) && ((mant & 1u) != 0u)))
    {
        mant++;
    }
    return mant + ((msb + 126u) << 23);
}

static inline uint32_t ndsConvU64MagToF32Bits(uint32_t hi, uint32_t lo) /* hi != 0 */
{
    uint32_t lz = (uint32_t)__builtin_clz(hi);
    uint32_t msb = 63u - lz;
    uint32_t top = (lz == 0u) ? hi : ((hi << lz) | (lo >> (32u - lz)));
    uint32_t low = lo << lz;
    uint32_t mant = top >> 8;
    uint32_t rem = top << 24;
    uint32_t sticky = (low != 0u) ? 1u : 0u;

    if ((rem > 0x80000000u) ||
        ((rem == 0x80000000u) && ((sticky | (mant & 1u)) != 0u)))
    {
        mant++;
    }
    return mant + ((msb + 126u) << 23);
}

static inline uint32_t ndsConvU32ToF32Bits(uint32_t x)
{
    return (x == 0u) ? 0u : ndsConvU32MagToF32Bits(x);
}

static inline uint32_t ndsConvS64ToF32Bits(int64_t v)
{
    uint64_t u = (uint64_t)v;
    uint32_t sign = (uint32_t)(u >> 63) << 31;
    uint32_t hi;
    uint32_t lo;

    if (sign != 0u)
    {
        u = 0u - u;
    }
    hi = (uint32_t)(u >> 32);
    lo = (uint32_t)u;
    if (hi == 0u)
    {
        return (lo == 0u) ? 0u : (sign | ndsConvU32MagToF32Bits(lo));
    }
    return sign | ndsConvU64MagToF32Bits(hi, lo);
}
