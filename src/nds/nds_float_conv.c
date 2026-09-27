/* P2-2p8 (2026-09-27): integer-to-float conversions in ITCM, so that libgcc's
 * _arm_addsubsf3.o can leave ITCM.
 *
 * That member carried 684 B of ITCM, most of it Task 16 goldens that nothing
 * runs (the stock fadd/fsub/frsub, replaced by the port's __aeabi_fadd), kept
 * there only because its unsigned and 64-bit conversions and __floatsisf are
 * live. These are those conversions: IEEE-754 binary32, round to nearest even,
 * as libgcc's. The u32 path was checked against the host's own conversion for
 * all 2^32 inputs and the s64 path for 4e8 random and every power-of-two edge
 * and halfway case, 0 mismatches (artifacts/performance/
 * 2026-09-27_p2-2p8-fast-mem/, section 37). __floatsisf forwards to the Task
 * 16 __aeabi_i2f, the same function under its libgcc name. The Makefile
 * renames the member's own copies (NDS_P2_FLOAT_CONV) and places it in main
 * RAM. ARM state: CLZ is an ARM instruction, and a Thumb-1 __builtin_clz would
 * be a libgcc call. */
#include <stdint.h>

#define NDS_FLOAT_CONV_CODE __attribute__((section(".itcm"), target("arm")))

typedef union NDSFloatConvBits
{
    uint32_t u;
    float f;
} NDSFloatConvBits;

static inline __attribute__((always_inline, target("arm"))) uint32_t
ndsConvU32MagToF32Bits(uint32_t m) /* m != 0 */
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

static inline __attribute__((always_inline, target("arm"))) uint32_t
ndsConvU64MagToF32Bits(uint32_t hi, uint32_t lo) /* hi != 0 */
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

float NDS_FLOAT_CONV_CODE __aeabi_ui2f(unsigned int x)
{
    NDSFloatConvBits bits;

    bits.u = (x == 0u) ? 0u : ndsConvU32MagToF32Bits(x);
    return bits.f;
}

float __floatunsisf(unsigned int x) __attribute__((alias("__aeabi_ui2f")));

/* Main RAM: the four-CPU census never ran it in a match. */
float __attribute__((section(".text.ndsFloatConvL2f"), target("arm")))
__aeabi_l2f(long long v)
{
    uint64_t u = (uint64_t)v;
    uint32_t sign = (uint32_t)(u >> 63) << 31;
    NDSFloatConvBits bits;

    if (sign != 0u)
    {
        u = 0u - u;
    }
    if ((uint32_t)(u >> 32) == 0u)
    {
        bits.u = ((uint32_t)u == 0u) ? 0u :
            (sign | ndsConvU32MagToF32Bits((uint32_t)u));
    }
    else
    {
        bits.u = sign | ndsConvU64MagToF32Bits((uint32_t)(u >> 32),
                                               (uint32_t)u);
    }
    return bits.f;
}

float __floatdisf(long long v) __attribute__((alias("__aeabi_l2f")));

extern float __aeabi_i2f(int x);

float NDS_FLOAT_CONV_CODE __floatsisf(int x)
{
    return __aeabi_i2f(x);
}
