/* Source-backed BattleShip matrix helpers used by the DS renderer adapters.
 *
 * P2-2p8 (2026-10-05, owner: "Software floating point should not exist, fixed
 * point only"): the Mtx builders' float-to-integer conversions -- FTOFIX32,
 * SINTABLE_RAD_TO_ID and syMatrixTraRotRpyRSca's `scale * 256` -- are taken
 * from the float's bits instead of a float multiply and __aeabi_f2iz. All
 * three are exact. FTOFIX32 and the scale multiply by a power of two, which
 * only moves the exponent, and __aeabi_f2iz truncates toward zero, saturating
 * past 2^31 and returning 0 for NaN (ndsMatrixFloatToFix reproduces each
 * case). The angle index is ndsR2CfxAngleIndex, proven equal to the source's
 * rounded binary32 product (it evaluates the float expression itself where the
 * proof does not reach). The source text is untouched; only the two macros
 * are redefined for this translation unit. */
#include <sys/audio.h>
#include <macros.h>
#include <PR/gu.h>
#include <nds/nds_r2_collision_fixed.h>

/* (s32)(value * 2^shift) exactly as the float multiply and __aeabi_f2iz give
 * it, for shift <= 16. One ARM copy, called: inlined at every FTOFIX32 the
 * Thumb builders grew to 1.6-1.7 KB each (syMatrixF2L, TraRotRpyRSca). */
static s32 __attribute__((noinline, target("arm")))
ndsMatrixFloatToFix(f32 value, u32 shift)
{
    u32 bits;
    u32 exponent;
    u32 magnitude;
    s32 rshift;

    __builtin_memcpy(&bits, &value, sizeof(bits));
    exponent = (bits >> 23) & 0xFFu;
    if (exponent == 0u)
    {
        return 0; /* zero or denormal: below 2^-110 after the multiply */
    }
    if (exponent == 0xFFu)
    {
        if ((bits & 0x7FFFFFu) != 0u)
        {
            return 0; /* NaN */
        }
        return ((bits & 0x80000000u) != 0u) ? (s32)0x80000000 : 0x7FFFFFFF;
    }
    /* The product's exponent; 255 and above is the overflow to infinity,
     * which saturates like any value past 2^31. */
    exponent += shift;
    if (exponent < 127u)
    {
        return 0;
    }
    rshift = 158 - (s32)exponent;
    if (rshift <= 0)
    {
        return ((bits & 0x80000000u) != 0u) ? (s32)0x80000000 : 0x7FFFFFFF;
    }
    magnitude = ((bits << 8) | 0x80000000u) >> rshift;
    return ((bits & 0x80000000u) != 0u) ? -(s32)magnitude : (s32)magnitude;
}

#undef FTOFIX32
/* Constants (FTOFIX32(0.0F), FTOFIX32(1.0F)) still fold at compile time. */
#define FTOFIX32(x)                                                         \
    (__builtin_constant_p(x) ? (long)((x) * (float)0x00010000) :           \
                               ndsMatrixFloatToFix((x), 16u))
#undef SINTABLE_RAD_TO_ID
#define SINTABLE_RAD_TO_ID(x) ndsR2CfxAngleIndex(x)

#define syMatrixTraRotRpyRSca ndsBaseSyMatrixTraRotRpyRSca
#define syMatrixF2L ndsBaseSyMatrixF2L
#include "../../decomp/BattleShip-main/decomp/src/sys/matrix.c"
#undef syMatrixTraRotRpyRSca
#undef syMatrixF2L

void syMatrixTraRotRpyRSca(Mtx *m, f32 tx, f32 ty, f32 tz, f32 r, f32 p,
                           f32 y, f32 sx, f32 sy, f32 sz);
void syMatrixF2L(Mtx44f *src, Mtx *dst);

/* matrix.c:5 as a loop: element pairs (2i, 2i + 1) of the row-major source
 * fill the integral words m[0][0..3], m[1][0..3] and the fractional words
 * m[2][0..3], m[3][0..3] in order -- the source's sixteen statements. */
void syMatrixF2L(Mtx44f *src, Mtx *dst)
{
    const f32 *s = &(*src)[0][0];
    s32 *integral = &dst->m[0][0];
    s32 *fractional = &dst->m[2][0];
    u32 i;

    for (i = 0u; i < 8u; i++)
    {
        const u32 e1 = (u32)ndsMatrixFloatToFix(s[2u * i], 16u);
        const u32 e2 = (u32)ndsMatrixFloatToFix(s[(2u * i) + 1u], 16u);

        integral[i] = (s32)COMBINE_INTEGRAL(e1, e2);
        fractional[i] = (s32)COMBINE_FRACTIONAL(e1, e2);
    }
}

/* matrix.c:1086 with `sx * 256` (a float multiply by 2^8 truncated into an
 * s32) taken from the bits; every other line is the source's. */
void syMatrixTraRotRpyRSca(Mtx *m, f32 tx, f32 ty, f32 tz, f32 r, f32 p,
                           f32 y, f32 sx, f32 sy, f32 sz)
{
    s32 sinr, sinp, siny;
    s32 cosr, cosp, cosy;
    s32 scalex, scaley, scalez;
    u16 indexr, indexp, indexy;
    u32 e1, e2;

    syGetSinCosUShort(sinr, cosr, r, indexr);
    syGetSinCosUShort(sinp, cosp, p, indexp);
    syGetSinCosUShort(siny, cosy, y, indexy);

    scalex = ndsMatrixFloatToFix(sx, 8u);
    scaley = ndsMatrixFloatToFix(sy, 8u);
    scalez = ndsMatrixFloatToFix(sz, 8u);

    e1         = (((cosp * cosy) >> 14) * scalex) >> 8;
    e2         = (((cosp * siny) >> 14) * scalex) >> 8;
    m->m[0][0] = COMBINE_INTEGRAL(e1, e2);
    m->m[2][0] = COMBINE_FRACTIONAL(e1, e2);

    e1         = (-sinp * scalex) >> 7;
    m->m[0][1] = COMBINE_INTEGRAL(e1, FTOFIX32(0.0F));
    m->m[2][1] = COMBINE_FRACTIONAL(e1, FTOFIX32(0.0F));

    e1         = ((((((sinr * sinp) >> 15) * cosy) >> 14) - ((cosr * siny) >> 14)) * scaley) >> 8;
    e2         = ((((((sinr * sinp) >> 15) * siny) >> 14) + ((cosr * cosy) >> 14)) * scaley) >> 8;
    m->m[0][2] = COMBINE_INTEGRAL(e1, e2);
    m->m[2][2] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (((sinr * cosp) >> 14) * scaley) >> 8;
    m->m[0][3] = COMBINE_INTEGRAL(e1, FTOFIX32(0.0F));
    m->m[2][3] = COMBINE_FRACTIONAL(e1, FTOFIX32(0.0F));

    e1         = ((((((cosr * sinp) >> 15) * cosy) >> 14) + ((sinr * siny) >> 14)) * scalez) >> 8;
    e2         = ((((((cosr * sinp) >> 15) * siny) >> 14) - ((sinr * cosy) >> 14)) * scalez) >> 8;
    m->m[1][0] = COMBINE_INTEGRAL(e1, e2);
    m->m[3][0] = COMBINE_FRACTIONAL(e1, e2);

    e1         = (((cosr * cosp) >> 14) * scalez) >> 8;
    m->m[1][1] = COMBINE_INTEGRAL(e1, FTOFIX32(0.0F));
    m->m[3][1] = COMBINE_FRACTIONAL(e1, FTOFIX32(0.0F));

    e1         = FTOFIX32(tx);
    e2         = FTOFIX32(ty);
    m->m[1][2] = COMBINE_INTEGRAL(e1, e2);
    m->m[3][2] = COMBINE_FRACTIONAL(e1, e2);

    e1         = FTOFIX32(tz);
    m->m[1][3] = COMBINE_INTEGRAL(e1, FTOFIX32(1.0F));
    m->m[3][3] = COMBINE_FRACTIONAL(e1, FTOFIX32(1.0F));
}
