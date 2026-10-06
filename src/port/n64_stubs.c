#include <math.h>

#include <PR/os.h>
#include <PR/ucode.h>
#include <nds/timers.h>
#include <nds/nds_platform.h>

OSTime osGetTime(void)
{
    return (OSTime)cpuGetTiming();
}

/* sinf and cosf in fixed point (P2-2p8, 2026-10-05; owner: "Software floating
 * point should not exist, fixed point only"). newlib's versions reduce the
 * argument with __ieee754_rem_pio2f and evaluate __kernel_sinf/cosf in soft
 * float, ~1,000 cycles a call. Here the angle becomes a binary angle: the
 * 24-bit significand times 2^32/2pi (at 2^10, 40 bits) gives turns with 56
 * fraction bits -- the quadrant and a Q32 position in it -- and the reduced
 * argument (folded to [0, pi/4]) goes through a Q32 Taylor polynomial, sine
 * to y^9 and cosine to y^10 (truncation below 2e-9). Host check against libm
 * in double over 4M arguments: 3.16e-8 from the true value at worst, the same
 * as newlib's sinf/cosf. These definitions replace newlib's for every caller,
 * the decomp's __sinf/__cosf included. */
#include <stdint.h>
#include <nds/nds_r2_collision_mtx.h>

#define NDS_TRIG_K_FIX UINT64_C(699970842190) /* 2^32 / 2pi at 2^10 */
#define NDS_TRIG_PI2_Q31 UINT64_C(3373259426) /* pi / 2 at Q31 */

/* Main RAM: 888 B of ARM, past ITCM's room; the four wrappers stay there. */
static f32 __attribute__((noinline, target("arm")))
ndsTrigF32(f32 x, u32 want_cos)
{
    u32 bits;
    u32 mag;
    s32 exponent;
    uint64_t product;
    uint64_t turns;
    s32 shift;
    u32 k;
    u32 r;
    u32 negate;
    u32 use_cos;
    uint64_t y;
    uint64_t z;
    uint64_t t;
    int64_t v;

    __builtin_memcpy(&bits, &x, sizeof(bits));
    mag = bits & 0x7FFFFFFFu;
    exponent = (s32)(mag >> 23);
    if (exponent >= 0xFF)
    {
        const u32 nan_bits = 0x7FC00000u;
        f32 nan;

        __builtin_memcpy(&nan, &nan_bits, sizeof(nan));
        return nan;
    }
    if (exponent < 115)
    {
        /* |x| < 2^-12: sin x rounds to x and cos x to 1. */
        return (want_cos != 0u) ? 1.0F : x;
    }
    product = (uint64_t)((mag & 0x7FFFFFu) | 0x800000u) * NDS_TRIG_K_FIX;
    shift = 136 - exponent;
    if (shift >= 0)
    {
        turns = product >> shift;
    }
    else
    {
        turns = (shift > -64) ? (product << -shift) : 0u;
    }
    /* Bits 55-54: the quadrant; 53-22: the position in it at Q32. */
    k = (u32)(turns >> 54) & 3u;
    if (want_cos != 0u)
    {
        k = (k + 1u) & 3u; /* cos t = sin(t + pi/2) */
    }
    r = (u32)(turns >> 22);
    use_cos = k & 1u;
    if (r >= 0x80000000u)
    {
        r = (u32)(UINT64_C(0x100000000) - r);
        use_cos ^= 1u;
    }
    negate = (k >= 2u) ? 1u : 0u;
    if ((want_cos == 0u) && ((bits & 0x80000000u) != 0u))
    {
        negate ^= 1u;
    }
    y = ((uint64_t)r * NDS_TRIG_PI2_Q31) >> 31; /* radians, Q32, <= pi/4 */
    z = (y * y) >> 32;
    if (use_cos != 0u)
    {
        /* 1 - z/2 + z^2/24 - z^3/720 + z^4/40320 - z^5/3628800 */
        t = UINT64_C(1184);
        t = UINT64_C(106522) - ((t * z) >> 32);
        t = UINT64_C(5965232) - ((t * z) >> 32);
        t = UINT64_C(178956971) - ((t * z) >> 32);
        t = UINT64_C(2147483648) - ((t * z) >> 32);
        v = (int64_t)(UINT64_C(0x100000000) - ((t * z) >> 32));
    }
    else
    {
        /* y (1 - z/6 + z^2/120 - z^3/5040 + z^4/362880) */
        t = UINT64_C(11836);
        t = UINT64_C(852176) - ((t * z) >> 32);
        t = UINT64_C(35791394) - ((t * z) >> 32);
        t = UINT64_C(715827883) - ((t * z) >> 32);
        t = UINT64_C(0x100000000) - ((t * z) >> 32);
        v = (int64_t)((y * t) >> 32);
    }
    return ndsR2CollisionFixedToF32((negate != 0u) ? -v : v, 32u);
}

f32 sinf(f32 value)
{
    return ndsTrigF32(value, 0u);
}

f32 cosf(f32 value)
{
    return ndsTrigF32(value, 1u);
}

f32 __sinf(f32 value)
{
    return ndsTrigF32(value, 0u);
}

f32 __cosf(f32 value)
{
    return ndsTrigF32(value, 1u);
}

/* N64 RSP microcode placeholder storage. On the N64 these are real F3DEX2
 * microcode blobs linked into the ROM; the imported sys/taskman.c references
 * them when building its task ucode table. The DS has no RSP and the bounded
 * taskman seam returns before task_draw, so these are zero-sized placeholders
 * that satisfy the link without occupying significant DS memory. The Start !=
 * End pairs are distinct symbols so the original size arithmetic is defined. */
long long int gspF3DEX2_fifoTextStart[1];
long long int gspF3DEX2_fifoTextEnd[1];
long long int gspF3DEX2_fifoDataStart[1];
long long int gspF3DEX2_fifoDataEnd[1];
