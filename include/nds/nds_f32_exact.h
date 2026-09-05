#ifndef SSB64_NDS_F32_EXACT_H
#define SSB64_NDS_F32_EXACT_H

#include <stdint.h>

/* binary32 addition on bit patterns, integer operations only, round to
 * nearest even -- the source's own float arithmetic, without the two libgcc
 * soft-float calls the ARM9 would otherwise pay per operation.
 *
 * WHY IT EXISTS. The fighter pose clock runs `anim_wait -= anim_speed` and
 * `anim_wait += payload` in binary32 in the source (ft/ftanim.c), and a
 * command boundary is crossed when the wait reaches zero. Q12 integer
 * arithmetic crosses those boundaries on different ticks for every
 * non-dyadic speed (every FallSpecial landing lag, the aerial smooth-landing
 * flag1 sweep, the rebound ratio), and scripts/fighters/test_pose_clock_
 * differential.py showed no fixed-point precision closes it: binary32 rounds
 * at a position that moves with the wait's magnitude. The only exact clock
 * is one that rounds the way binary32 does, and this is that add.
 *
 * OPERAND CLASS. Finite normals and zero. The clock cannot produce anything
 * else -- waits are integer frame counts and sums of a positive speed,
 * speeds are normal floats -- and the host proof
 * (scripts/fighters/prove_pose_clock_exact.py, 16.9 million operations, 0
 * mismatches) covers exactly that class. NaN, infinity and subnormals are
 * outside it; the ARM9 build does not check for them.
 *
 * COST. About 25 integer instructions in ARM state: CLZ is one instruction
 * on the ARM946E-S (ARMv5TE), and the function is meant to be compiled in
 * ARM mode (`__attribute__((target("arm")))` at its consumer, the way the
 * 64-bit multiplies are) -- Thumb has no CLZ and would call a helper.
 *
 * PROOF. scripts/fighters/test_f32_exact_kernel.c compiles this header on
 * the host and compares it, bit for bit, with the host's own IEEE adder over
 * the structured operand set and a large random sweep. Keep that test green
 * before changing a line here. */
static inline uint32_t ndsF32AddBits(uint32_t a, uint32_t b)
{
    uint32_t sa, sb, ea, eb, ma, mb, m, e, s, d, grs;

    /* Zero operands: x + 0 = x; -0 + -0 = -0, every other zero sum is +0. */
    if ((a & 0x7fffffffu) == 0u)
    {
        return ((b & 0x7fffffffu) != 0u) ? b : (a & b);
    }
    if ((b & 0x7fffffffu) == 0u)
    {
        return a;
    }
    sa = a >> 31;
    sb = b >> 31;
    ea = (a >> 23) & 0xffu;
    eb = (b >> 23) & 0xffu;
    /* 1.23 significands with three extra bits below: guard, round, sticky. */
    ma = ((a & 0x7fffffu) | 0x800000u) << 3;
    mb = ((b & 0x7fffffu) | 0x800000u) << 3;
    /* Align on the larger exponent. */
    if (ea < eb)
    {
        uint32_t t;
        t = sa; sa = sb; sb = t;
        t = ea; ea = eb; eb = t;
        t = ma; ma = mb; mb = t;
    }
    d = ea - eb;
    if (d != 0u)
    {
        if (d >= 27u)
        {
            mb = 1u;   /* entirely below the guard bit: only stickiness survives */
        }
        else
        {
            uint32_t sticky = ((mb & ((1u << d) - 1u)) != 0u) ? 1u : 0u;
            mb = (mb >> d) | sticky;
        }
    }
    e = ea;
    if (sa == sb)
    {
        m = ma + mb;
        s = sa;
        if ((m & (1u << 27)) != 0u)      /* carry out: renormalise right */
        {
            m = (m >> 1) | (m & 1u);
            e++;
        }
    }
    else
    {
        uint32_t lz;

        if (ma >= mb)
        {
            m = ma - mb;
            s = sa;
        }
        else
        {
            m = mb - ma;
            s = sb;
        }
        if (m == 0u)
        {
            return 0u;                   /* exact cancellation is +0 (RNE) */
        }
        /* Normalise left: the leading one belongs at bit 26 (23 + GRS). */
        lz = (uint32_t)__builtin_clz(m) - 5u;
        m <<= lz;
        e -= lz;
    }
    /* Round to nearest even on the three extra bits. */
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
    return (s << 31) | (e << 23) | (m & 0x7fffffu);
}

/* a - b: the clock's subtract, as the source writes it. */
static inline uint32_t ndsF32SubBits(uint32_t a, uint32_t b)
{
    return ndsF32AddBits(a, b ^ 0x80000000u);
}

#endif
