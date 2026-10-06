/* BattleShip sys/utils.c with the arctangent family in fixed point.
 *
 * P2-2p8 (2026-10-05, owner: "Software floating point should not exist,
 * fixed point only"). syUtilsArcTan is a continued fraction with seven float
 * divides (~900 soft-float ticks a call), and ArcTan2 / ArcSin / ArcCos add a
 * divide or a square root each (artifacts/performance/2026-10-05_float-census).
 * Here the argument's magnitude is read at Q30 from its float bits (its
 * reciprocal from the hardware divider when it exceeds one), atan of [0, 1]
 * comes from a 257-entry Q30 table and the arctangent difference identity
 * (ndsAtanUnitQ30), and the result goes back to float from its bits: no float
 * operation remains. Mechanical equivalence (owner ruling D13): the source's
 * continued fraction is itself an approximation of the same function, and
 * every quadrant, rail and zero case below follows the source's branches. */

#define syUtilsArcTan ndsBaseSyUtilsArcTan
#define syUtilsArcTan2 ndsBaseSyUtilsArcTan2
#define syUtilsArcSin ndsBaseSyUtilsArcSin
#define syUtilsArcCos ndsBaseSyUtilsArcCos
#include "../../decomp/BattleShip-main/decomp/src/sys/utils.c"
#undef syUtilsArcTan
#undef syUtilsArcTan2
#undef syUtilsArcSin
#undef syUtilsArcCos

#include <stdint.h>

#include <nds/nds_r2_collision_mtx.h>
#include <nds/nds_r2_hwmath_unit.h>

f32 syUtilsArcTan(f32 div);
f32 syUtilsArcTan2(f32 y, f32 x);
f32 syUtilsArcSin(f32 x);
f32 syUtilsArcCos(f32 x);

/* atan(i / 256) at Q30, i = 0..256. */
static const int32_t sNdsAtanQ30[257] = {
    0, 4194283, 8388437, 12582336, 16775851, 20968854, 25161218, 29352814,
    33543516, 37733196, 41921726, 46108981, 50294833, 54479155, 58661822, 62842708,
    67021687, 71198634, 75373424, 79545932, 83716036, 87883610, 92048532, 96210679,
    100369930, 104526161, 108679253, 112829084, 116975536, 121118487, 125257820, 129393416,
    133525159, 137652930, 141776614, 145896097, 150011262, 154121996, 158228185, 162329719,
    166426484, 170518371, 174605269, 178687069, 182763663, 186834944, 190900805, 194961140,
    199015846, 203064818, 207107953, 211145151, 215176309, 219201328, 223220110, 227232556,
    231238569, 235238055, 239230917, 243217063, 247196400, 251168835, 255134279, 259092643,
    263043837, 266987774, 270924369, 274853536, 278775192, 282689253, 286595638, 290494267,
    294385059, 298267937, 302142824, 306009643, 309868320, 313718782, 317560955, 321394768,
    325220151, 329037035, 332845353, 336645037, 340436023, 344218245, 347991640, 351756148,
    355511705, 359258254, 362995735, 366724092, 370443267, 374153206, 377853855, 381545162,
    385227074, 388899541, 392562515, 396215946, 399859787, 403493994, 407118521, 410733324,
    414338361, 417933591, 421518973, 425094468, 428660037, 432215645, 435761254, 439296830,
    442822340, 446337750, 449843028, 453338145, 456823070, 460297774, 463762232, 467216414,
    470660297, 474093856, 477517067, 480929907, 484332355, 487724391, 491105994, 494477146,
    497837829, 501188027, 504527723, 507856902, 511175551, 514483656, 517781204, 521068185,
    524344587, 527610402, 530865619, 534110231, 537344232, 540567613, 543780370, 546982499,
    550173994, 553354853, 556525073, 559684652, 562833591, 565971887, 569099543, 572216558,
    575322936, 578418678, 581503788, 584578271, 587642129, 590695370, 593737999, 596770023,
    599791448, 602802283, 605802536, 608792216, 611771334, 614739898, 617697921, 620645413,
    623582386, 626508854, 629424828, 632330323, 635225352, 638109930, 640984073, 643847795,
    646701114, 649544044, 652376604, 655198810, 658010682, 660812236, 663603492, 666384468,
    669155185, 671915663, 674665921, 677405981, 680135863, 682855589, 685565182, 688264663,
    690954054, 693633380, 696302662, 698961924, 701611191, 704250487, 706879836, 709499262,
    712108791, 714708448, 717298260, 719878250, 722448447, 725008876, 727559563, 730100536,
    732631822, 735153448, 737665442, 740167831, 742660643, 745143906, 747617650, 750081902,
    752536690, 754982045, 757417995, 759844569, 762261796, 764669707, 767068330, 769457696,
    771837835, 774208776, 776570551, 778923188, 781266719, 783601175, 785926586, 788242982,
    790550395, 792848855, 795138394, 797419043, 799690833, 801953796, 804207961, 806453363,
    808690030, 810917996, 813137292, 815347949, 817549999, 819743474, 821928406, 824104826,
    826272767, 828432260, 830583337, 832726030, 834860371, 836986393, 839104126, 841213603,
    843314857,
};

#define NDS_ATAN_HALF_PI_Q30 INT64_C(1686629713) /* pi / 2 at Q30 */
#define NDS_ATAN_PI_Q30 INT64_C(3373259426)      /* pi at Q30 */
#define NDS_ATAN_ONE_Q30 (INT64_C(1) << 30)

/* The source's +-0.99999F rails: that float is exactly 1,073,731,072 at Q30. */
#define NDS_ATAN_RAIL_Q30 INT64_C(1073731072)

static inline u32 ndsUtilsBits(f32 value)
{
    u32 bits;

    __builtin_memcpy(&bits, &value, sizeof(bits));
    return bits;
}

/* atan(t) for t in [0, 1] at Q30: the nearest table point x0 plus the exact
 * identity atan(t) = atan(x0) + atan((t - x0) / (1 + t * x0)), whose second
 * argument is within 2^-9, where d - d^3 / 3 is exact to 2^-45. Host check
 * (2026-10-05, 2M random arguments a function): atan 6.2e-8 rad from the true
 * value against the source continued fraction's 1.7e-7; atan2 2.2e-7 against
 * 2.8e-7. */
/* ARM state (2026-10-06): its 64-bit products were __aeabi_lmul calls in
 * Thumb. */
static int64_t __attribute__((target("arm"))) ndsAtanUnitQ30(int64_t t)
{
    int64_t num;
    int64_t den;
    int64_t delta;
    int64_t d3;
    u32 index;

    if (t >= NDS_ATAN_ONE_Q30)
    {
        return (int64_t)sNdsAtanQ30[256];
    }
    if (t <= 0)
    {
        return 0;
    }
    index = (u32)((t + (1 << 21)) >> 22);
    num = t - ((int64_t)index << 22);
    if (num == 0)
    {
        return (int64_t)sNdsAtanQ30[index];
    }
    den = NDS_ATAN_ONE_Q30 + ((t * (int64_t)index) >> 8);
    delta = ndsR2HwMathDivideFast(num << 30, den);
    d3 = (((delta * delta) >> 30) * delta) >> 30;
    return (int64_t)sNdsAtanQ30[index] + delta - (d3 / 3);
}

/* atan(|x|) at Q30 from x's float bits (the sign bit is ignored). */
static int64_t ndsAtanMagQ30(u32 bits)
{
    const u32 mag = bits & 0x7FFFFFFFu;
    const s32 exponent = (s32)(mag >> 23);
    const u32 mantissa = (mag & 0x7FFFFFu) | 0x800000u;

    if ((mag == 0u) || (exponent == 0))
    {
        return 0; /* zero or denormal */
    }
    if (exponent >= 0xFF)
    {
        return NDS_ATAN_HALF_PI_Q30; /* inf */
    }
    if (exponent < 127)
    {
        /* |x| < 1: mantissa x 2^(exponent - 150), so at Q30 the mantissa
         * shifts by exponent - 120. */
        const s32 shift = exponent - 120;
        const int64_t t = (shift >= 0) ? ((int64_t)mantissa << shift) :
                          ((shift > -24) ? (int64_t)(mantissa >> -shift) : 0);

        return ndsAtanUnitQ30(t);
    }
    else
    {
        /* |x| >= 1: pi/2 - atan(1/|x|); 1/|x| at Q30 is
         * 2^(180 - exponent) / mantissa. */
        const s32 numer_shift = 180 - exponent;
        const int64_t t = (numer_shift >= 0) ?
            ndsR2HwMathDivideFast((int64_t)1 << numer_shift,
                                  (int64_t)mantissa) : 0;

        return NDS_ATAN_HALF_PI_Q30 - ndsAtanUnitQ30(t);
    }
}

f32 syUtilsArcTan(f32 div)
{
    const u32 bits = ndsUtilsBits(div);
    const int64_t a = ndsAtanMagQ30(bits);

    return ndsR2CollisionFixedToF32(((bits & 0x80000000u) != 0u) ? -a : a,
                                    30u);
}

f32 syUtilsArcTan2(f32 y, f32 x)
{
    const u32 ybits = ndsUtilsBits(y);
    const u32 xbits = ndsUtilsBits(x);
    const u32 ymag = ybits & 0x7FFFFFFFu;
    const u32 xmag = xbits & 0x7FFFFFFFu;
    /* The source's `y < 0.0F`: negative zero is not below zero. */
    const sb32 y_negative = ((ybits & 0x80000000u) != 0u) && (ymag != 0u);
    int64_t a;

    if (xmag == 0u)
    {
        /* x == 0 of either sign: +-pi/2 for y != 0, else 0. */
        if (ymag == 0u)
        {
            return 0.0F;
        }
        return ndsR2CollisionFixedToF32((y_negative != FALSE) ?
                                            -NDS_ATAN_HALF_PI_Q30 :
                                            NDS_ATAN_HALF_PI_Q30,
                                        30u);
    }
    if ((ymag == 0u) || ((ymag >> 23) == 0u))
    {
        a = 0;
    }
    else if ((xmag >> 23) == 0u)
    {
        a = NDS_ATAN_HALF_PI_Q30; /* denormal x: |y / x| overflows */
    }
    else
    {
        /* |y / x| as a float's bits: the exponents subtract and the hardware
         * divider takes the 24-bit mantissas (a 47-bit over 24-bit divide). */
        const u32 ym = (ymag & 0x7FFFFFu) | 0x800000u;
        const u32 xm = (xmag & 0x7FFFFFu) | 0x800000u;
        u32 q = (u32)ndsR2HwMathDivideFast((int64_t)ym << 23, (int64_t)xm);
        s32 e = (s32)(ymag >> 23) - (s32)(xmag >> 23) + 127;

        /* ym / xm is in (1/2, 2), so q is in (2^22, 2^24). */
        if (q < (1u << 23))
        {
            q <<= 1;
            e--;
        }
        if (e <= 0)
        {
            a = 0;
        }
        else if (e >= 0xFF)
        {
            a = NDS_ATAN_HALF_PI_Q30;
        }
        else
        {
            a = ndsAtanMagQ30(((u32)e << 23) | (q & 0x7FFFFFu));
        }
    }
    if ((xbits & 0x80000000u) == 0u)
    {
        /* x > 0: atan(y / x), signed as the quotient. */
        if ((ybits & 0x80000000u) != 0u)
        {
            a = -a;
        }
    }
    else
    {
        /* x < 0: (pi - atan(|y / x|)) * (y < 0 ? -1 : 1). */
        a = NDS_ATAN_PI_Q30 - a;
        if (y_negative != FALSE)
        {
            a = -a;
        }
    }
    return ndsR2CollisionFixedToF32(a, 30u);
}

/* |x| <= 1 at Q30, signed (x outside the unit range clamps; the callers below
 * have already railed it). */
static int64_t ndsUtilsUnitQ30(u32 bits)
{
    const u32 mag = bits & 0x7FFFFFFFu;
    const s32 exponent = (s32)(mag >> 23);
    const u32 mantissa = (mag & 0x7FFFFFu) | 0x800000u;
    const s32 shift = exponent - 120;
    int64_t v;

    if ((mag == 0u) || (exponent == 0))
    {
        return 0;
    }
    if (exponent >= 127)
    {
        v = NDS_ATAN_ONE_Q30;
    }
    else
    {
        v = (shift >= 0) ? ((int64_t)mantissa << shift) :
            ((shift > -24) ? (int64_t)(mantissa >> -shift) : 0);
    }
    return ((bits & 0x80000000u) != 0u) ? -v : v;
}

/* asin(v) at Q30 for |v| < 1 at Q30: atan(v / sqrt(1 - v^2)), the square root
 * and the divide on the hardware units. */
static int64_t ndsUtilsArcSinQ30(int64_t v)
{
    const int64_t mag = (v < 0) ? -v : v;
    const uint64_t rem = ((uint64_t)1 << 60) - (uint64_t)(mag * mag);
    const int64_t root = (int64_t)ndsR2HwMathSqrt64Fast(rem); /* Q30 */
    int64_t a;

    if (mag == 0)
    {
        return 0;
    }
    if (root <= mag)
    {
        /* ratio >= 1: pi/2 - atan(root / mag). */
        a = NDS_ATAN_HALF_PI_Q30 -
            ndsAtanUnitQ30(ndsR2HwMathDivideFast(root << 30, mag));
    }
    else
    {
        a = ndsAtanUnitQ30(ndsR2HwMathDivideFast(mag << 30, root));
    }
    return (v < 0) ? -a : a;
}

f32 syUtilsArcSin(f32 x)
{
    const int64_t v = ndsUtilsUnitQ30(ndsUtilsBits(x));

    if (v > NDS_ATAN_RAIL_Q30)
    {
        return ndsR2CollisionFixedToF32(NDS_ATAN_HALF_PI_Q30, 30u);
    }
    if (v < -NDS_ATAN_RAIL_Q30)
    {
        return ndsR2CollisionFixedToF32(-NDS_ATAN_HALF_PI_Q30, 30u);
    }
    return ndsR2CollisionFixedToF32(ndsUtilsArcSinQ30(v), 30u);
}

f32 syUtilsArcCos(f32 x)
{
    const int64_t v = ndsUtilsUnitQ30(ndsUtilsBits(x));

    if (v > NDS_ATAN_RAIL_Q30)
    {
        return 0.0F;
    }
    if (v < -NDS_ATAN_RAIL_Q30)
    {
        return ndsR2CollisionFixedToF32(NDS_ATAN_PI_Q30, 30u);
    }
    return ndsR2CollisionFixedToF32(NDS_ATAN_HALF_PI_Q30 - ndsUtilsArcSinQ30(v),
                                    30u);
}
