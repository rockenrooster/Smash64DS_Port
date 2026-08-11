#ifndef NDS_ANIM_FIXED_H
#define NDS_ANIM_FIXED_H

/* Requirement 4 -- the fighter AObj's runtime state IN fixed point, not a cache
 * beside it.
 *
 * `FIXEDPOINT_ANIMATION.md` is right that `AObj` is the problem: six `f32`
 * fields that the parser writes from `s16` figatree arguments and the evaluator
 * converts back to fixed point on every node of every frame. The c115 census
 * priced the round trip exactly -- `ndsR2CubicValueFixed` runs 176.4 times a
 * frame at 220 instructions a call, and roughly 200 of those 220 are the six
 * inlined `ndsR2F32ToFixed`, the one `ndsR2F32MulToFixed` and the one
 * `ndsR2FixedToF32`. A Hermite evaluation is a dozen multiply-accumulates; the
 * rest is format conversion on values that started life as `s16` with
 * power-of-two scales.
 *
 * The replacement discriminates on the `kind` field the evaluator ALREADY
 * switches on. `AObjAnimKind` runs 0..4 and `AObj.kind` is a `u8`, so the Q
 * kinds below extend it without touching `decomp/`, and the `f32` slots carry
 * the Q values bit-cast in place: no new struct, no parallel array, no second
 * representation alive at the same time for the same node. Non-fighter AObjs
 * (material, camera, stage, `mp_collision`'s own Linear writer) keep the float
 * kinds and the decomp's own expressions, bit-identical.
 *
 * WHAT IS BIT-IDENTICAL. `value_base`, `value_target`, `rate_base` and
 * `rate_target` reach the shipped cubic today as `ndsR2F32ToFixed(arg * 2^-k,
 * 12)`, and `arg * 2^-k` is exact (`check_ftanim_target_exact.py`, all 65,536 x
 * 8 inputs). Writing `round(arg * 2^(12-k))` straight from the `s16` produces
 * the same Q12 integer, so the cubic's four value inputs do not move at all.
 * `nGCAnimTrackTraI`'s scale is not a power of two, so those two ids keep the
 * float expression and go through the same converter -- also unchanged.
 *
 * WHAT MOVES. `t = length / payload` is a Q30 reciprocal multiply instead of an
 * f32 one (MORE accurate: 2.3e-8 relative at payload 60 against f32's 6e-8),
 * and `length` accumulates in Q12 instead of f32. `check_r2_cubic_error_bound.py`
 * bounds the whole pipeline against the decomp float reference and is the gate.
 *
 * This header is dependency-free on purpose -- it assumes only the `f32`/`s32`/
 * `u32`/`s64` typedefs its includer already has -- because
 * `check_r2_cubic_error_bound.py` inlines it textually into the host harness.
 * Do not add an `#include` to it. */

/* Q12 holds joint VALUES and rates: 1/4096 of a radian or a world unit, already
 * far finer than anything gameplay can see, and the scale the shipped cubic
 * quantises to today. */
#define NDS_R2_AQ_VF   12
/* `length` in frames, and Step's `length_invert` (which holds a frame count,
 * not a reciprocal -- the same field carries both meanings in the original). */
#define NDS_R2_AQ_LF   12
/* Cubic's `length_invert`, the reciprocal 1/payload. Q30 because `t`'s error is
 * amplified by `length * rate` at the segment ends, where the Hermite rate
 * basis has slope 1: a Q24 reciprocal would have put a 60-frame segment 1e-4
 * out at t=1, which is a third of the whole error budget for nothing. */
#define NDS_R2_AQ_IF   30
/* The four Hermite BASIS terms. Two carry a factor of `length`, so their
 * quantum reaches the result multiplied by L*|rate|; the four extra bits over
 * Q12 divide that by 16 and cost nothing on ARM, where 32x32->64 is one SMULL. */
#define NDS_R2_AQ_BF   16
/* Linear's `rate_base` ONLY. Its quantum is amplified by the full block length,
 * and at Q12 the host bound measured 0.011 of the 0.02 budget going to that one
 * rounding; four more bits take it to 0.0007. Safe because Linear's rate is
 * never read by another arm -- the Cubic arms carry `rate_target` forward, and
 * Linear writes that to zero. The 4 extra bits also cannot overflow: the widest
 * value an s16 argument can reach is Q12 2^25, and 2^26 << 4 still fits s32. */
#define NDS_R2_AQ_RF   16
#define NDS_R2_AQ_BONE (1 << NDS_R2_AQ_BF)
/* Q12 length x Q30 reciprocal -> Q16 `t`. */
#define NDS_R2_AQ_TSH  ((NDS_R2_AQ_LF + NDS_R2_AQ_IF) - NDS_R2_AQ_BF)

/* Q kinds. `>= NDS_R2_AQ_KIND_BASE` is the discriminator, and it is a compare
 * against a constant on a byte the evaluator already loads. */
#define NDS_R2_AQ_KIND_BASE    5u
#define NDS_R2_AQ_KIND_STEP    5u
#define NDS_R2_AQ_KIND_LINEAR  6u
#define NDS_R2_AQ_KIND_CUBIC   7u

/* Saturation counter. Defined in `src/import/battleship_sys_objanim.c`; both
 * the parser and the evaluator increment it, so a clamped joint is visible in
 * `-ExtraGlobals` rather than silent. */
extern volatile u32 gNdsR2CubicSaturations;

static inline u32 ndsR2FloatBits(f32 v)
{
    u32 bits;

    __builtin_memcpy(&bits, &v, sizeof(bits));
    return bits;
}

/* The Q value carried in an `f32` slot. A bit-cast, not a conversion: GCC emits
 * nothing for these, and keeping them named makes every punned access greppable
 * instead of a raw `memcpy` that reads like a bug. */
static inline s32 ndsR2AQLoad(f32 slot)
{
    s32 v;

    __builtin_memcpy(&v, &slot, sizeof(v));
    return v;
}

static inline f32 ndsR2AQStore(s32 v)
{
    f32 slot;

    __builtin_memcpy(&slot, &v, sizeof(slot));
    return slot;
}

/* f32 -> Q`bits`, rounding to nearest. Hand-rolled rather than
 * `(s32)(v * 4096.0f)` because that is two soft-float calls (~50 ticks) where
 * this is a dozen integer ops. Saturates instead of wrapping: a wrapped joint
 * angle would be a visible teleport, a saturated one is a clamp. `bits` is
 * always a literal at the call sites, so the shifts fold. */
static inline s32 ndsR2F32ToFixed(f32 v, s32 bits)
{
    u32 bits_in = ndsR2FloatBits(v);
    s32 exp = (s32)((bits_in >> 23) & 0xffu);
    s32 mant;
    s32 shift;

    if (exp == 0)
    {
        return 0;   /* zero or subnormal: below the resolution either way */
    }
    if (exp == 0xff)
    {
        gNdsR2CubicSaturations++;
        return ((bits_in & 0x80000000u) != 0u) ? -0x7fffffff : 0x7fffffff;
    }
    mant = (s32)((bits_in & 0x7fffffu) | 0x800000u);   /* 1.23 fixed */
    /* value = mant * 2^(exp-127-23); the result wants that scaled by 2^bits. */
    shift = (exp - 127) - 23 + bits;
    if (shift >= 0)
    {
        if (shift > 7)
        {
            gNdsR2CubicSaturations++;
            mant = 0x7fffffff;
        }
        else
        {
            mant <<= shift;
        }
    }
    else if (shift < -24)
    {
        mant = 0;   /* rounds to zero: even the leading 1 bit falls off */
    }
    else
    {
        /* Round to nearest rather than toward zero. Truncation here is a
         * systematic pull toward zero on every one of the six conversions, and
         * the bound measured its mean signed error at -9.1e-5 before this. */
        mant = (mant + (1 << (-shift - 1))) >> -shift;
    }
    return ((bits_in & 0x80000000u) != 0u) ? -mant : mant;
}

/* `s16 arg * 2^-k` straight to Q`NDS_R2_AQ_VF`, without ever building the f32.
 *
 * `shift` is `NDS_R2_AQ_VF - k` from the decomp's own frac table read as a power
 * of two. Non-negative shifts are exact -- an s16 scaled up cannot lose a bit --
 * and the two negative ones round the MAGNITUDE half away from zero, which is
 * what `ndsR2F32ToFixed` does, so the Q12 integer matches the one the shipped
 * float path produces for the same argument. */
static inline s32 ndsR2AnimArgToQ(s32 arg, s32 shift)
{
    s32 mag;

    if (shift >= 0)
    {
        return arg << shift;
    }
    mag = (arg < 0) ? -arg : arg;
    mag = (mag + (1 << (-shift - 1))) >> -shift;
    return (arg < 0) ? -mag : mag;
}

#endif /* NDS_ANIM_FIXED_H */
