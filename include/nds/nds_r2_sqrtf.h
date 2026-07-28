#ifndef NDS_R2_SQRTF_H
#define NDS_R2_SQRTF_H

/* R2-03 E1 -- the correctly-rounded sqrtf kernel, shared by the ROM and its
 * host falsifier.
 *
 * The arithmetic lives here rather than in the .c so that
 * `scripts/check-r2-fixed-sqrt.ps1` verifies the code that actually ships
 * instead of a transcription of it. This project has been bitten by
 * reimplemented checkers before; a falsifier that can pass while the ROM
 * differs is not a falsifier.
 *
 * The including translation unit must define, before the include:
 *
 *     static unsigned int ndsR2SqrtfHardware(unsigned long long value);
 *
 * returning floor(sqrt(value)) -- the DS hardware unit in the ROM, an exact
 * integer root on the host. Passing it as a function pointer instead would
 * defeat inlining on a path measured at 64 calls a frame.
 *
 * Deliberately typed in plain unsigned int / unsigned long long so the header
 * has no dependency on the port's type headers and the host can compile it.
 *
 * Exactness, for a normal positive x:
 *
 *   x = (1.f) * 2^e,  M = 2^23 + f  (24 bits),  e = 2q + r,  r in {0,1}
 *   A = M << (r + 23)                            so A in [2^46, 2^48)
 *   R = floor(sqrt(A))                           so R in [2^23, 2^24)
 *
 * R is exactly the truncated 24-bit significand of sqrt(x), and the result is
 * R * 2^(q-23). Round-to-nearest wants R+1 when the exact root exceeds
 * R + 1/2, i.e. A > (R + 1/2)^2 = R^2 + R + 1/4. A and R are integers, so that
 * is A > R*R + R. A tie is impossible -- (R + 1/2)^2 always carries a 1/4
 * fraction no integer A can equal -- so there is no tie-break rule to get
 * wrong, and the result is round-to-nearest-even for free.
 */

#define NDS_R2_SQRTF_FALLBACK 0
#define NDS_R2_SQRTF_EXACT    1

/* Returns NDS_R2_SQRTF_EXACT and writes the result bit pattern, or
 * NDS_R2_SQRTF_FALLBACK for the inputs the fast path declines: negative, zero,
 * denormal, infinity and NaN. Those are left to newlib so the returned bit
 * pattern stays newlib's own on every path. */
static inline int ndsR2SqrtfBits(unsigned int bits, unsigned int *out)
{
    unsigned int exponent = (bits >> 23) & 0xffu;
    unsigned int mantissa;
    unsigned int odd;
    unsigned int root;
    unsigned long long scaled;
    int unbiased;
    int quotient;

    if (((bits & 0x80000000u) != 0u) || (exponent == 0u) ||
        (exponent == 0xffu))
    {
        return NDS_R2_SQRTF_FALLBACK;
    }

    mantissa = (bits & 0x007fffffu) | 0x00800000u;
    unbiased = (int)exponent - 127;
    /* Arithmetic shift, not division: e / 2 truncates toward zero in C, which
     * gives the wrong quotient/remainder pair for negative exponents. e >> 1
     * floors, so 2q + r == e holds for both signs. */
    quotient = unbiased >> 1;
    odd = (unsigned int)(unbiased & 1);

    scaled = (unsigned long long)mantissa << (odd + 23u);
    root = ndsR2SqrtfHardware(scaled);

    /* R*R needs 48 bits, so this is a real 64-bit multiply. */
    if (scaled > (((unsigned long long)root * (unsigned long long)root) +
                  (unsigned long long)root))
    {
        root++;
        if (root == 0x01000000u)
        {
            /* Rounded out of the 24-bit significand: renormalise rather than
             * letting the carry corrupt the exponent field. */
            root = 0x00800000u;
            quotient++;
        }
    }

    *out = ((unsigned int)(quotient + 127) << 23) | (root & 0x007fffffu);
    return NDS_R2_SQRTF_EXACT;
}

#endif /* NDS_R2_SQRTF_H */
