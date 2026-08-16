#ifndef NDS_R2_HWMATH_H
#define NDS_R2_HWMATH_H

/* A correctly-rounded IEEE-754 single-precision divide built on the ARM9's
 * 64/32 divide unit.
 *
 * WHY THIS IS NOT A FIDELITY QUESTION. It returns the bit-identical result
 * __aeabi_fdiv returns, so no caller, consumer, decision or state hash can
 * observe it. That is `SIMSIDE.md` section 6's column N, and it is the one
 * shape on this board whose CONVERSIONS PER DELETED OPERATION is zero: nothing
 * crosses a representation boundary, because the operands were already f32
 * going in and are f32 coming out. `EXCHANGE_LEAF.md` section 5.1 is the record
 * of what that term costs when it is not zero -- 31 to 42 cycles per edge, one
 * whole soft-float operation, enough to sink a leaf conversion at 1.00 conv/op.
 *
 * The arithmetic lives in this header rather than in the .c for the reason
 * include/nds/nds_r2_sqrtf.h gives: scripts/check-r2-hwmath.c compiles THIS
 * file on the host and grades it against the host's own IEEE divide, so the
 * falsifier cannot pass while the ROM differs.
 *
 * The including translation unit must define, before the include:
 *
 *     static int64_t ndsR2HwMathDivideUnit(int64_t numerator,
 *                                          int32_t denominator,
 *                                          int32_t *remainder_out);
 *
 * returning the exact truncated-toward-zero quotient and writing the remainder
 * -- the DS hardware unit in the ROM, a plain C divide on the host. Both
 * operands are positive by construction here, so the sign rules do not enter.
 *
 * THE TYPES ARE int32_t AND NOT int, AND THAT IS LOAD-BEARING. On devkitARM
 * `int32_t` is `long int`, not `int` -- `_Static_assert(__builtin_types_compatible_p(
 * int32_t, int))` fails and the `long` one passes. The first version of this
 * header took `int *` and the ROM wrapper handed it `(int32_t *)`, which is a
 * cast between incompatible pointer types, and GCC took the strict-aliasing
 * licence: in build-c210-hwmath's schedule it treated the store through the
 * `long *` as unable to touch the `int` object, folded `remainder` back to its
 * initialiser and DELETED the `| remainder` term from the sticky expression
 * (visible at 0x0208bec8, where the register holding DIVREM_RESULT is
 * overwritten by the significand shift and never read). The result was 110
 * wrong IEEE quotients in 65,536 -- deterministic, reproduced twice, and
 * present in exactly one of three builds of the same source. Every operand
 * type here is now one type end to end so the mismatch is a hard compile error
 * rather than silent undefined behaviour.
 *
 * EXACTNESS, for normal finite a and b:
 *
 *   a = 1.fa * 2^(ea-127), significand ma = 2^23 + fa, ma in [2^23, 2^24)
 *   b likewise with mb
 *   Q = floor(ma * 2^32 / mb), R = (ma * 2^32) mod mb
 *
 *   ma/mb lies in (1/2, 2), so Q lies in (2^31, 2^33) and carries at least 32
 *   significant bits against the 24 the result needs. The exact quotient is
 *   (Q + R/mb) / 2^32, so the true significand is Q >> s plus a strictly
 *   positive tail whose leading bit is Q's bit (s-1) and whose remainder is
 *   non-zero exactly when Q's low (s-1) bits or R are non-zero. s is 9 when
 *   ma >= mb and 8 otherwise, which is also the branch that decides whether the
 *   result exponent is ea-eb+127 or one less.
 *
 *   Guard and sticky are therefore exact, and round-to-nearest-ties-to-even is
 *   the textbook decision on them.
 *
 *   AND THE TIE IS UNREACHABLE, which the falsifier discovered rather than the
 *   author: scripts/check-r2-hwmath.c counted zero over 58 million graded
 *   pairs, and the reason is structural. A tie needs the exact quotient to have
 *   exactly 25 significant bits. ma/mb terminates in binary only when
 *   mb/gcd(ma,mb) is a power of two, and then ma/mb = (ma/g) / 2^j with
 *   ma/g < 2^24 -- at most 24 significant bits, one short. When it does NOT
 *   terminate, R is non-zero, so sticky is set and there is no tie either. The
 *   `significand & 1` term is therefore dead code that costs one AND, and it is
 *   kept because "dead by an argument" is not the same as "dead", and the
 *   falsifier asserts the count stays zero with two live neighbouring states as
 *   its controls.
 *
 *   R still has to be read: it is the sticky bit for every non-terminating
 *   quotient whose expansion runs past Q's low bits, which is the common case,
 *   not the rare one.
 *
 * DECLINED INPUTS fall back to the caller's own soft-float divide, so the
 * returned bit pattern stays libgcc's on every path this does not answer:
 * either operand zero, denormal, infinity or NaN, and any result that would be
 * denormal, zero, infinite or overflowing. Those are all decided from the
 * exponent fields alone, before and after the rounding step.
 */

#include <stdint.h>

#define NDS_R2_HWMATH_FDIV_FALLBACK 0
#define NDS_R2_HWMATH_FDIV_EXACT    1

static inline int ndsR2HwMathDivBits(unsigned int a_bits, unsigned int b_bits,
                                     unsigned int *out)
{
    unsigned int ea = (a_bits >> 23) & 0xffu;
    unsigned int eb = (b_bits >> 23) & 0xffu;
    unsigned int ma;
    unsigned int mb;
    unsigned int sign;
    unsigned int significand;
    unsigned int guard;
    unsigned int sticky;
    unsigned long long quotient;
    int32_t remainder = 0;
    int exponent;

    if ((ea == 0u) || (ea == 0xffu) || (eb == 0u) || (eb == 0xffu))
    {
        return NDS_R2_HWMATH_FDIV_FALLBACK;
    }

    sign = (a_bits ^ b_bits) & 0x80000000u;
    ma = (a_bits & 0x007fffffu) | 0x00800000u;
    mb = (b_bits & 0x007fffffu) | 0x00800000u;

    quotient = (unsigned long long)ndsR2HwMathDivideUnit(
        (int64_t)((unsigned long long)ma << 32), (int32_t)mb, &remainder);

    exponent = (int)ea - (int)eb + 127;
    if (ma >= mb)
    {
        significand = (unsigned int)(quotient >> 9);
        guard = (unsigned int)(quotient >> 8) & 1u;
        sticky = ((((unsigned int)quotient) & 0xffu) |
                  (unsigned int)remainder) != 0u;
    }
    else
    {
        significand = (unsigned int)(quotient >> 8);
        guard = (unsigned int)(quotient >> 7) & 1u;
        sticky = ((((unsigned int)quotient) & 0x7fu) |
                  (unsigned int)remainder) != 0u;
        exponent--;
    }

    if ((guard != 0u) && ((sticky != 0u) || ((significand & 1u) != 0u)))
    {
        significand++;
        if (significand == 0x01000000u)
        {
            significand = 0x00800000u;
            exponent++;
        }
    }

    if ((exponent <= 0) || (exponent >= 0xff))
    {
        return NDS_R2_HWMATH_FDIV_FALLBACK;
    }

    *out = sign | ((unsigned int)exponent << 23) |
           (significand & 0x007fffffu);
    return NDS_R2_HWMATH_FDIV_EXACT;
}

#endif /* NDS_R2_HWMATH_H */
