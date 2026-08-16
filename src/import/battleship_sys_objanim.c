/* Compile the original BattleShip object-animation/setup translation unit.
 * Its public add entrypoints normalize O2R AObjEvent32 command words before
 * the unchanged original parser sees them. */
#include <nds/nds_fcmp.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_anim_fixed.h>

/* Outside every configuration gate on purpose: `nds_anim_fixed.h` declares it
 * and `battleship_ftanim.c` includes that header whether or not this file's
 * fixed-point player is compiled in. Four bytes against a link error that only
 * appears in the P2 configuration nobody measures. */
volatile u32 gNdsR2CubicSaturations;

#define gcAddDObjAnimJoint ndsBaseGcAddDObjAnimJoint
#define gcAddMObjMatAnimJoint ndsBaseGcAddMObjMatAnimJoint
#define gcAddMObjAll ndsBaseGcAddMObjAll
#define gcAddAnimJointAll ndsBaseGcAddAnimJointAll
#define gcAddMatAnimJointAll ndsBaseGcAddMatAnimJointAll
#define gcAddAnimAll ndsBaseGcAddAnimAll
#define gcAddCObjCamAnimJoint ndsBaseGcAddCObjCamAnimJoint
#define gcPlayMObjMatAnim ndsBaseGcPlayMObjMatAnim
#define gcPlayAnimAll ndsBaseGcPlayAnimAll
#if NDS_R2_ANIM_CENSUS || NDS_R2_CUBIC_FIXED
/* R2-03 E61/E64. Frees the name so the port-side player below is reached. Task
 * 95 proved this exact interposition end to end: the hot call is INTERNAL to
 * objanim.c (inside gcPlayAnimAll), so renaming the definition renames that
 * call with it and a port-side replacement actually runs. */
#define gcPlayDObjAnimJoint ndsBaseGcPlayDObjAnimJoint
#endif

#include <battleship_overlay/src/sys/objanim.c>

#undef gcAddDObjAnimJoint
#undef gcAddMObjMatAnimJoint
#undef gcAddMObjAll
#undef gcAddAnimJointAll
#undef gcAddMatAnimJointAll
#undef gcAddAnimAll
#undef gcAddCObjCamAnimJoint
#undef gcPlayMObjMatAnim
#undef gcPlayAnimAll

#if NDS_R2_CUBIC_FIXED
#undef gcPlayDObjAnimJoint

/* R2-03 E64 — the cubic in fixed point. Owner-authorized 2026-07-29.
 *
 * E60/E61 measured this: `gcPlayDObjAnimJoint` is 94,531 ticks/frame inclusive,
 * 60,509 of it soft-float, and 99.6% of that float is the CUBIC branch — 149.4
 * evaluations a frame at ~405 ticks each, which is 14 soft-float operations at
 * ~29 ticks apiece. Step (43.6% of nodes) and Linear (1.7%) are left exactly as
 * the original wrote them; they cost no float worth removing.
 *
 * The rewrite. With `t = length * length_invert` and `L = length`, the original
 *
 *     f16 = li², f12 = L², f18 = li·L², f14 = L·L²·li²,
 *     f20 = 2·f14·li, f22 = 3·f12·f16, f24 = f14 - f18
 *     value = vb·((f20-f22)+1) + vt·(f22-f20)
 *           + rb·((f24-f18)+L) + rt·f24
 *
 * is, exactly in real arithmetic, the standard cubic Hermite:
 *
 *     f20 = 2t³            f22 = 3t²            f18 = L·t
 *     f24 = L·t·(t-1)      (f24-f18)+L = L·(1-t)²
 *     value = vb·(2t³-3t²+1) + vt·(3t²-2t³) + rb·L·(1-t)² + rt·L·(t²-t)
 *
 * so this is a change of *representation*, not of the curve. What differs from
 * the original is rounding: Q12 truncation instead of MIPS single-precision at
 * every step. `PROJECT_GOAL.md` requires mechanical equivalence and lists
 * "fixed-point replacements" as an allowed technique; it does not require bit
 * exactness. The Task 9 state hash asserts the stronger property and is expected
 * to move — that is the authorized part of this change, not an accident.
 *
 * Why the cache is not optional. `value_base`/`value_target`/`rate_base`/
 * `rate_target` are `f32` in the AObj, so converting them per evaluation costs
 * four soft-float round trips and eats the entire win. They are constant between
 * parse events, which are rare, so they are converted once and reused. The
 * validity test is a compare of the five source float BIT PATTERNS — integer
 * work, no float — which is exact and needs no cooperation from the parser.
 * `length` does change every tick, so `t` still costs one real multiply. */

/* E64 arm A had a 256-entry Q12 conversion cache keyed on the source float bit
 * patterns. It WORKED -- 135,871 evaluations, 86.4% hit rate, zero saturations --
 * and the frame got worse anyway: WORK-H P95 +21,632, SRC P50 +17,792. Two
 * reasons, both about footprint rather than arithmetic, and both already written
 * down in this repo:
 *
 *   - 10,240 bytes of new BSS. "The noise floor is not measurement error, it is
 *     the price of adding data" -- and the floor here is 5,000-7,000.
 *   - the `.text.hot` member grew from 500 bytes to 1,824. Task 94's comment in
 *     `linker/nds_hot_text.ld` says that list is a curated 8 KiB working set and
 *     that perturbing one member re-addresses the other ten, which it measured
 *     at 6,144 WORK-H P50 on 122 of 128 frames.
 *
 * Arm B therefore spends nothing: no cache, no new BSS, 32-bit arithmetic
 * wherever the range allows, and the four per-node conversions done inline. The
 * hand-rolled converter is what makes that affordable -- `(s32)(v * 4096.0f)` is
 * two soft-float calls where this is a dozen integer ops. */

volatile u32 gNdsR2CubicEvals;

/* Cycle 109 standing-rule-7 route for the two animation cuts in this file.
 *
 * Why a runtime route rather than two builds: this cycle PROVED the sampler is
 * bit-deterministic -- the same ROM sampled twice returns byte-identical
 * buckets, variance 0 on every percentile. The 14,080-tick "cross-build floor"
 * is therefore not noise but deterministic *placement* sensitivity, and the
 * distinction decides the method. Noise averages down over runs; placement does
 * not, ever. No number of repeats can separate a 3,000-tick code win from a
 * 6,000-tick re-addressing shift across two binaries. Measuring ONE binary two
 * ways is the only method that can, which is what this global is for.
 *
 *   bit 0 (1) -- the loop-invariant hoist in gcPlayDObjAnimJoint
 *   bit 1 (2) -- the fused length*length_invert multiply in the cubic kernel
 *   bit 2 (4) -- the port-side figatree parser (reloc_backend_compat_shims.c
 *                selects it; the body is in battleship_ftanim.c)
 *   bit 3 (8) -- Requirement 4: the fighter AObj stored in fixed point. Reads
 *                in BOTH files at once, because the parser writes the Q form
 *                and the player reads it; poke it before the first parse, not
 *                mid-match, or a list ends up half converted. It is safe if you
 *                do -- every node carries its own format in `kind` -- but the
 *                arm you measure is then a mixture and prices nothing.
 *
 * Default 15 is the shipped behaviour, so an unpoked ROM is unaffected.
 * `-SetGlobals gNdsR2AnimCutRoute=0` is the all-pre-cut arm; bit-wise values
 * price one cut at a time (7 = fixed-point AObj off only, 3 = that plus the
 * parser, 13 = fused multiply off only).
 *
 * .data, not .bss, and aligned(32) so it OWNS its cache line. Both are load
 * bearing: an uninitialised route would place differently between arms, and
 * gNdsFtrPlanVerify's comment in diagnostics.c records a poke being stamped
 * back to 0 by a neighbouring counter's write-back -- gNdsR2CubicEvals here
 * increments on every single cubic node, so that hazard is a certainty rather
 * than a possibility.
 *
 * Compile-time gated because the route is an INSTRUMENT, not a feature. At
 * NDS_R2_ANIM_CUT_ROUTE=0 (the default, and what every published ROM builds)
 * NDS_R2_ANIM_CUT_ON folds to a constant 1, GCC dead-codes both pre-cut arms,
 * and the shipped program is exactly the no-route one -- no per-node test, no
 * spill, no footprint. "Replace, don't coexist" is a board rule with a price
 * attached, and a permanent runtime selector would pay it forever to answer a
 * question that is already answered. */
#ifndef NDS_R2_ANIM_CUT_ROUTE
#define NDS_R2_ANIM_CUT_ROUTE 0
#endif
#if NDS_R2_ANIM_CUT_ROUTE
volatile u32 gNdsR2AnimCutRoute
    __attribute__((section(".data"), aligned(32))) = 15u;
#define NDS_R2_ANIM_CUT_ON(bit) ((gNdsR2AnimCutRoute & (bit)) != 0u)
#else
#define NDS_R2_ANIM_CUT_ON(bit) (1)
#endif

/* NDS_R2_CUBIC_FIXED_KERNEL_BEGIN — `scripts/check_r2_cubic_error_bound.py`
 * extracts everything between this marker and the matching END, prepends
 * `include/nds/nds_anim_fixed.h`, and compiles both on the host against the
 * decomp's own `gcGetInterpValueCubic`. Extraction rather than a copy so the
 * bound can never be measured against stale code. Do not put anything between
 * the markers that needs a DS header. */

/* Two fixed-point scales, and the split is what makes the bound affordable.
 *
 * `NDS_R2_CUBIC_VF` (12) holds joint VALUES. Their own quantum lands straight in
 * the result, so 1/4096 of a radian or a world unit is already far finer than
 * anything gameplay can see.
 *
 * `NDS_R2_CUBIC_BF` (16) holds the four Hermite BASIS terms. Two of them carry a
 * factor of `length`, so their quantum reaches the result multiplied by
 * L*|rate| -- the curve's steepness in value units per t. At Q12 that amplifier
 * put a 60-unit translation crossed in 13 frames 0.107 units off the float
 * original, measured by `check_r2_cubic_error_bound.py`. Four more bits divide
 * that by 16 and cost two 32x32->64 multiplies (SMULL, one instruction each).
 *
 * They are the Q-form scales under their original names: the whole point of
 * Requirement 4 is that the AObj now STORES what this kernel used to convert. */
#define NDS_R2_CUBIC_VF NDS_R2_AQ_VF
#define NDS_R2_CUBIC_BF NDS_R2_AQ_BF
#define NDS_R2_CUBIC_BONE NDS_R2_AQ_BONE

/* (a * b) -> Q`bits` in one integer multiply, without `__aeabi_fmul`.
 *
 * `t = length * length_invert` is the kernel's last soft-float call: 60,582
 * executions, 1,524,849 cycles, and it exists only to build an f32 that the
 * next line immediately quantises to Q16. Multiplying the two 24-bit
 * significands directly produces that number without the intermediate float.
 *
 * NOT bit-identical to the old path, and more accurate rather than less: the
 * old path rounded twice (once when `fmul` packed the product into an f32
 * significand, once in `ndsR2F32ToFixed`), this rounds once.
 * `check_r2_cubic_error_bound.py` measures the whole kernel against the decomp
 * float reference, so an accuracy gain shows up as a smaller deviation.
 *
 * Saturation matches `ndsR2F32ToFixed`: clamp, never wrap, because a wrapped
 * `t` is a visible joint teleport where a clamped one is a held pose.
 *
 * A first attempt at this was reverted as a "hang". That verdict was retracted
 * -- the harness had a 30-second marker budget that had gone marginal, and the
 * unchanged control failed too. See the board. */
static inline s32 ndsR2F32MulToFixed(f32 a, f32 b, s32 bits)
{
    u32 ba = ndsR2FloatBits(a);
    u32 bb = ndsR2FloatBits(b);
    s32 ea = (s32)((ba >> 23) & 0xffu);
    s32 eb = (s32)((bb >> 23) & 0xffu);
    u32 sign = (ba ^ bb) & 0x80000000u;
    s64 p;      /* non-negative: the sign is carried separately, and the host
                 * error-bound harness has no u64 typedef */
    s32 shift;

    if ((ea == 0) || (eb == 0))
    {
        return 0;       /* zero or subnormal operand: below resolution anyway */
    }
    if ((ea == 0xff) || (eb == 0xff))
    {
        gNdsR2CubicSaturations++;
        return (sign != 0u) ? -0x7fffffff : 0x7fffffff;
    }
    /* Two 1.23 significands multiply to a 2.46 product in [2^46, 2^48).
     * value = p * 2^(ea-127-23) * 2^(eb-127-23) = p * 2^(ea+eb-300);
     * the caller wants that scaled by 2^bits. */
    p = (s64)((ba & 0x7fffffu) | 0x800000u) *
        (s64)((bb & 0x7fffffu) | 0x800000u);
    shift = ((ea + eb) - 300) + bits;
    if (shift >= 0)
    {
        /* p is already ~2^47, so any non-negative shift is far out of range. */
        gNdsR2CubicSaturations++;
        return (sign != 0u) ? -0x7fffffff : 0x7fffffff;
    }
    if (shift < -63)
    {
        return 0;       /* everything, including the leading one, falls off */
    }
    p = (p + ((s64)1 << (-shift - 1))) >> (-shift);   /* round to nearest */
    if (p > (s64)0x7fffffff)
    {
        gNdsR2CubicSaturations++;
        p = (s64)0x7fffffff;
    }
    return (sign != 0u) ? -(s32)p : (s32)p;
}

/* s32 -> f32 bit pattern, without `__aeabi_i2f`.
 *
 * The call was 60,582 executions and 1,017,778 cycles in the cycle-106
 * whole-match profile -- one per cubic evaluation, and after the comparison
 * helpers went it was the last soft-float call left in this kernel apart from
 * the `length * length_invert` multiply. There is nothing to win inside libgcc's
 * routine; the win is not making the call.
 *
 * This is EXACT, not an approximation: it reproduces `__aeabi_i2f` bit for bit
 * including round-to-nearest-even for the |q| >= 2^24 inputs where an s32 does
 * not fit a 24-bit significand. `scripts/check_s32tof32_exact.py` proves it over
 * all 2^32 inputs; it is not argued from this comment.
 *
 * `__builtin_clz` is only free because the sole caller is the `target("arm")`
 * kernel below -- Thumb-1 has no `CLZ` and would turn this into a `__clzsi2`
 * call, which is the trap `thumb-hides-64bit-cost` records. Do not inline this
 * into a Thumb function. */
static inline u32 ndsR2S32ToF32Bits(s32 q)
{
    u32 sign;
    u32 m;
    u32 lz;
    u32 exp;
    u32 mant;
    u32 rem;

    if (q == 0)
    {
        return 0u;
    }
    sign = (q < 0) ? 0x80000000u : 0u;
    /* Negating INT_MIN in u32 gives 0x80000000, which is the magnitude wanted;
     * doing it in s32 would be undefined. */
    m = (q < 0) ? (0u - (u32)q) : (u32)q;
    lz = (u32)__builtin_clz(m);
    exp = 31u - lz;
    m <<= lz;                       /* leading one now at bit 31 */
    mant = m >> 8;                  /* 24 significant bits, implicit one at 23 */
    rem = m & 0xffu;                /* the bits that fall off */
    if ((rem > 0x80u) || ((rem == 0x80u) && ((mant & 1u) != 0u)))
    {
        mant++;                     /* round to nearest, ties to even */
        if ((mant & 0x1000000u) != 0u)
        {
            mant >>= 1;             /* rounding carried out of the significand */
            exp++;
        }
    }
    return sign | ((exp + 127u) << 23) | (mant & 0x7fffffu);
}

/* Q`bits` -> f32. The integer conversion above plus an exact exponent adjust;
 * subtracting `bits` biased exponents is an exact divide by 2^bits, so no
 * second multiply and no extra rounding. */
static inline f32 ndsR2FixedToF32(s32 q, s32 bits)
{
    f32 f;
    u32 raw = ndsR2S32ToF32Bits(q);

    if ((raw & 0x7f800000u) != 0u)
    {
        raw -= ((u32)bits << 23);
    }
    __builtin_memcpy(&f, &raw, sizeof(f));
    return f;
}

/* Compile this one function as ARM, not Thumb.
 *
 * The measurement that forced it: lifting the basis to Q16 needs 64-bit squares
 * for t^2 and t^3, and this TU builds Thumb, which has no SMULL. GCC therefore
 * emitted `bl __aeabi_lmul` -- eleven call sites in `gcPlayDObjAnimJoint`, eight
 * of them on the executed path -- and the Q16 arm measured SRC P50 +17,728 /
 * WORK-H P50 +25,472 against E64b. In ARM mode every one of those is a single
 * SMULL/SMLAL, including the six E64b was already paying for. `noinline` keeps
 * the six inlined float->fixed conversions to one copy instead of one per call
 * site, which matters because this lands in `.text.hot`'s curated 8 KiB.
 *
 * `__arm__` guards the attribute so `check_r2_cubic_error_bound.py` can still
 * compile the extracted kernel on the host. */
#if defined(__arm__)
#define NDS_R2_CUBIC_ATTR __attribute__((noinline, target("arm")))
#define NDS_R2_ANIM_Q_BODY_ATTR \
    inline __attribute__((always_inline, target("arm")))
#else
#define NDS_R2_CUBIC_ATTR
/* Host side (check_r2_cubic_error_bound.py extracts this region and compiles it
 * natively): an ordinary static function. Only the DS build needs the body
 * duplicated, and always_inline at the harness's -O level is a needless way to
 * fail a numeric gate that does not care where the code lives. */
#define NDS_R2_ANIM_Q_BODY_ATTR
#endif

/* ITCM residency for the Q evaluator. Zero-wait and never evicted, against
 * 21,719 tk/fr of instruction fetch measured on the marginal-80. The 1,028
 * bytes come from three ITCM residents that execute ZERO instructions across
 * the gate window -- `_arm_cmpsf2.o` + `_arm_unordsf2.o` (332 B, dead because
 * the port defines the fcmp helpers itself) and
 * ndsRendererHardwareGetLightShadeLut (404 B, the miss-path LUT builder) --
 * plus the 512 B that were already free on the tick-HUD instrument. */
#if NDS_R2_ANIM_Q_ITCM_ON && defined(__arm__)
#define NDS_R2_ANIM_Q_ITCM __attribute__((section(".itcm")))
#else
#define NDS_R2_ANIM_Q_ITCM
#endif

/* Both kernels square `t` and then cube it, and t^3 wraps an s32 well before
 * `t` itself does. Inside a block `t` is in [0,1]; outside it the Hermite is
 * extrapolating nonsense either way, so the only question is whether the
 * nonsense is bounded or a wrap -- and a wrapped joint is a visible teleport.
 *
 * This is not theoretical and it is not new. `gNdsR2CubicSaturations` read
 * **337 a match** on the float arm of the cycle-116 A/B: `length_invert` still
 * holding a Step FRAME COUNT when a Cubic event arrives with a zero payload
 * (the parser only overwrites it when the payload is non-zero) makes
 * `t = length * length_invert` enormous, the conversion clamps it to
 * 0x7fffffff, and `t*t` then wraps anyway. Clamping `t` instead of its inputs
 * is what actually bounds the chain, and it costs two compares.
 *
 * With |t| <= 2 and |length| <= 1024 frames (17 seconds -- longer than any
 * block) every intermediate fits an s32 with room: t2 <= 2^18, t3 <= 2^19,
 * omt2 <= 2^19, length*omt2 >> 12 <= 2^29, and the s64 accumulator reaches
 * 2^54 against its 2^63. The result is clamped once on the way out. */
#define NDS_R2_AQ_T_MAX  (2 * NDS_R2_AQ_BONE)
#define NDS_R2_AQ_LEN_MAX (1024 << NDS_R2_AQ_LF)

static inline s32 ndsR2AnimClamp(s32 v, s32 limit)
{
    if (v > limit)
    {
        gNdsR2CubicSaturations++;
        return limit;
    }
    if (v < -limit)
    {
        gNdsR2CubicSaturations++;
        return -limit;
    }
    return v;
}

static inline s32 ndsR2AnimClamp64(s64 v)
{
    if (v > (s64)0x7fffffff)
    {
        gNdsR2CubicSaturations++;
        return 0x7fffffff;
    }
    if (v < -(s64)0x7fffffff)
    {
        gNdsR2CubicSaturations++;
        return -0x7fffffff;
    }
    return (s32)v;
}

static NDS_R2_CUBIC_ATTR f32 ndsR2CubicValueFixed(const AObj *aobj)
{
    /* The one unavoidable real multiply: `length` advances every tick, so `t`
     * cannot be carried across evaluations.
     *
     * Route bit 1 selects the pre-cut form -- a soft-float multiply followed by
     * a separate convert -- so the fused version can be priced on this same
     * binary. The route test is a register `tst` present in both arms, so it
     * cancels out of the difference; it does mean the lab ROM's fused arm is
     * marginally slower than the shipped one, which is the accepted cost. */
    s32 t = ndsR2AnimClamp(NDS_R2_ANIM_CUT_ON(2u) ?
        ndsR2F32MulToFixed(aobj->length, aobj->length_invert,
                           NDS_R2_CUBIC_BF) :
        ndsR2F32ToFixed(aobj->length * aobj->length_invert, NDS_R2_CUBIC_BF),
        NDS_R2_AQ_T_MAX);
    s32 length_q = ndsR2AnimClamp(
        ndsR2F32ToFixed(aobj->length, NDS_R2_CUBIC_VF), NDS_R2_AQ_LEN_MAX);
    /* t is Q16 and normally in [0,1], so t*t reaches 2^32 and needs the 64-bit
     * product. Every requantising shift rounds rather than truncates. */
    s32 t2 = (s32)((((s64)t * t) + (NDS_R2_CUBIC_BONE / 2)) >> NDS_R2_CUBIC_BF);
    s32 t3 = (s32)((((s64)t2 * t) + (NDS_R2_CUBIC_BONE / 2)) >> NDS_R2_CUBIC_BF);
    /* (1-t)^2 == 1 - 2t + t^2 exactly, so reuse t2 instead of squaring (1-t).
     * Cheaper by a multiply AND a shift, and it deletes that rounding step
     * rather than merely rounding it -- which matters because near t=1 the
     * square is small and a truncated one loses most of its significance. */
    s32 omt2 = (t2 - (2 * t)) + NDS_R2_CUBIC_BONE;
    s32 h_vb = ((2 * t3) - (3 * t2)) + NDS_R2_CUBIC_BONE;
    s32 h_vt = (3 * t2) - (2 * t3);
    /* These two carry a factor of `length`, which is unbounded, so they are the
     * only places that need a wider intermediate for range as well as rounding.
     * Q12 value x Q16 basis, shifted by VF, is Q16 again. SMULL/SMLAL make a
     * 32x32->64 on ARM9 a single instruction; it is the 64-bit ADDS/ADCS chains
     * that arm A paid for, and there are none left here. */
    s32 h_rb = (s32)((((s64)length_q * omt2) + (1 << (NDS_R2_CUBIC_VF - 1))) >>
        NDS_R2_CUBIC_VF);
    s32 h_rt = (s32)((((s64)length_q * (t2 - t)) +
        (1 << (NDS_R2_CUBIC_VF - 1))) >> NDS_R2_CUBIC_VF);
    /* Q12 x Q16 = Q28 in the accumulator, shifted back to Q12 at the end. */
    s64 acc = (s64)ndsR2F32ToFixed(aobj->value_base, NDS_R2_CUBIC_VF) * h_vb;

    gNdsR2CubicEvals++;
    acc += (s64)ndsR2F32ToFixed(aobj->value_target, NDS_R2_CUBIC_VF) * h_vt;
    acc += (s64)ndsR2F32ToFixed(aobj->rate_base, NDS_R2_CUBIC_VF) * h_rb;
    acc += (s64)ndsR2F32ToFixed(aobj->rate_target, NDS_R2_CUBIC_VF) * h_rt;
    return ndsR2FixedToF32(
        (s32)((acc + (NDS_R2_CUBIC_BONE / 2)) >> NDS_R2_CUBIC_BF),
        NDS_R2_CUBIC_VF);
}

/* Requirement 4. The same three curves over an AObj that is ALREADY fixed point.
 *
 * Every conversion above is gone -- not moved, not cached, not memoised. The
 * four value inputs are loads, `length` is a load, and `t` is one SMULL against
 * the Q30 reciprocal the parser wrote instead of an f32 one it has to unpack.
 * The single `ndsR2FixedToF32` at the bottom stays: `DObj`'s pose vectors are
 * decomp fields that collision, camera and the matrix builder all read, so the
 * float boundary moves here rather than disappearing.
 *
 * All three kinds share ONE function, and that is a saving in itself: today
 * Cubic pays a `bl` to this kernel, Linear pays two to `__aeabi_fmul`/`fadd`,
 * and Step pays one to `__aeabi_fcmple`. After, each pays exactly one `bl`
 * here. Step's compare is Q12 against Q12 -- `length_invert` carries a frame
 * count for Step and a reciprocal for Cubic, which is the original's own
 * double meaning for that field, kept rather than tidied.
 *
 * ARM, not Thumb, for the same reason `ndsR2CubicValueFixed` is: SMULL and CLZ.
 * See `thumb-hides-64bit-cost` -- a pure-precision change cost +36,032 P95
 * until one `target("arm")` attribute won -71,616 back.
 *
 * WHERE IT LIVES IS NOW THE EXPENSIVE PART OF IT (2026-08-16). The marginal-80
 * per-PC census charges this kernel 26,664 tk/fr, of which 21,719 -- 81.4% --
 * is `icache_fill`: 1,028 bytes entered 370.6 times a frame, 117 cycles of
 * fetch stall per entry, i.e. its lines do not survive between entries at all.
 * The arithmetic below is already as cheap as it gets and the fetch is bigger
 * than any arithmetic left to delete, so the lever is ITCM residency, which is
 * zero-wait and never evicted. It is not "fetched whole by construction"
 * either: 162 of its 257 instruction slots and 23 of its 33 cache lines carry
 * any execution at all, which is why the whole body is moved rather than a
 * hand-split hot half -- the cold 320 bytes are already free of fills.
 *
 * The body lives in an always_inline impl so the route below can emit TWO
 * out-of-line copies at different addresses from ONE source. `target("arm")`
 * is repeated on the impl because GCC refuses always_inline across a target
 * mismatch and this TU is built -mthumb. */
static NDS_R2_ANIM_Q_BODY_ATTR f32 ndsR2AnimValueQBody(const AObj *aobj)
{
    s32 len = ndsR2AQLoad(aobj->length);            /* Q12 frames */
    s32 inv = ndsR2AQLoad(aobj->length_invert);     /* Q30 recip, or Q12 frames */
    s32 vb = ndsR2AQLoad(aobj->value_base);         /* Q12 */
    u32 kind = aobj->kind;
    s32 out;

    /* Each arm keeps its OWN result in an s32 and its own `s64` local, and the
     * cubic tests first so it is the fall-through.
     *
     * That shape is load-bearing, not style. Written with one `s64 acc` shared
     * across the three arms (cycle 116's first cut, measured on the shipped
     * ROM) GCC sign-extended `length`, `value_base` and `rate_target` to 64 bits
     * BEFORE the branch, expanded every `(s64)a * b` from one SMLAL into a
     * mul/mla/umull triple, and spilled 20 bytes to the stack to hold the
     * doubled register pressure -- 241 ARM instructions where the float kernel
     * it replaced needed far fewer for the same curve. Narrowing the arms is
     * worth more than any arithmetic left in them. Check the disassembly for
     * `umull`/`adc` pairs after touching this. */
    if (kind == NDS_R2_AQ_KIND_STEP)
    {
        /* `len` unclamped on purpose: Step is a compare and a select, and
         * clamping it would change which of the two values a block longer than
         * the clamp selects. Neither value can be out of range -- both were
         * written by the parser from an s16 -- so no clamp on the way out. */
        out = (inv <= len) ? ndsR2AQLoad(aobj->value_target) : vb;
    }
    else if (kind == NDS_R2_AQ_KIND_LINEAR)
    {
        /* Q16 rate, not Q12 -- see NDS_R2_AQ_RF. */
        s64 lin = (s64)vb + ((((s64)len * ndsR2AQLoad(aobj->rate_base)) +
            (1 << (NDS_R2_AQ_RF - 1))) >> NDS_R2_AQ_RF);

        out = ndsR2AnimClamp64(lin);
    }
    else
    {
        /* Q12 length x Q30 reciprocal, requantised to the Q16 basis scale.
         * The 64-bit product cannot overflow, so the clamp goes on `t` rather
         * than on its inputs -- see NDS_R2_AQ_T_MAX. */
        s32 lenc = ndsR2AnimClamp(len, NDS_R2_AQ_LEN_MAX);
        s32 t = ndsR2AnimClamp(
            (s32)((((s64)len * inv) + ((s64)1 << (NDS_R2_AQ_TSH - 1))) >>
                NDS_R2_AQ_TSH), NDS_R2_AQ_T_MAX);
        s32 t2 = (s32)((((s64)t * t) + (NDS_R2_AQ_BONE / 2)) >> NDS_R2_AQ_BF);
        s32 t3 = (s32)((((s64)t2 * t) + (NDS_R2_AQ_BONE / 2)) >> NDS_R2_AQ_BF);
        s32 omt2 = (t2 - (2 * t)) + NDS_R2_AQ_BONE;
        s32 h_vb = ((2 * t3) - (3 * t2)) + NDS_R2_AQ_BONE;
        s32 h_vt = (3 * t2) - (2 * t3);
        s32 h_rb = (s32)((((s64)lenc * omt2) + (1 << (NDS_R2_AQ_VF - 1))) >>
            NDS_R2_AQ_VF);
        s32 h_rt = (s32)((((s64)lenc * (t2 - t)) + (1 << (NDS_R2_AQ_VF - 1))) >>
            NDS_R2_AQ_VF);
        s64 acc = (s64)vb * h_vb;

        acc += (s64)ndsR2AQLoad(aobj->value_target) * h_vt;
        acc += (s64)ndsR2AQLoad(aobj->rate_base) * h_rb;
        acc += (s64)ndsR2AQLoad(aobj->rate_target) * h_rt;
        /* The float kernel saturated at each of its six conversions and so
         * could not reach here out of range; with the conversions gone this
         * accumulator is the only place left that can. */
        out = ndsR2AnimClamp64((acc + (NDS_R2_AQ_BONE / 2)) >> NDS_R2_AQ_BF);
        gNdsR2CubicEvals++;
    }
    return ndsR2FixedToF32(out, NDS_R2_AQ_VF);
}

#if NDS_R2_ANIM_ITCM_ROUTE
/* Lab SAME-BINARY route for the placement. Two out-of-line copies of one body,
 * one in .itcm and one in .main; a `.data` word picks which `bl` the caller
 * takes. Placement is a link-time property of a symbol, so it cannot be routed
 * on one copy -- but it CAN be routed between two, and that turns a cross-build
 * question with a >=14,080 rank-80 floor into a zero-repeat-floor difference.
 *
 * .data AND NOT .bss, per nds_r2_sqrtf.c: a zero-initialised route word with no
 * explicit section lands in .bss and drags a ~10,000 tk/fr placement floor.
 *
 * Both arms run identical instructions on identical inputs, so the engagement
 * control is an EQUALITY: gNdsR2CubicEvals must read the same on both arms. */
volatile u32 gNdsR2AnimItcmRoute
    __attribute__((used, section(".data"))) = 1u;

static NDS_R2_CUBIC_ATTR f32 ndsR2AnimValueQMain(const AObj *aobj)
{
    return ndsR2AnimValueQBody(aobj);
}

static NDS_R2_CUBIC_ATTR NDS_R2_ANIM_Q_ITCM f32
ndsR2AnimValueQItcm(const AObj *aobj)
{
    return ndsR2AnimValueQBody(aobj);
}

static f32 ndsR2AnimValueQ(const AObj *aobj)
{
    return (gNdsR2AnimItcmRoute != 0u) ? ndsR2AnimValueQItcm(aobj)
                                       : ndsR2AnimValueQMain(aobj);
}
#else
static NDS_R2_CUBIC_ATTR NDS_R2_ANIM_Q_ITCM f32
ndsR2AnimValueQ(const AObj *aobj)
{
    return ndsR2AnimValueQBody(aobj);
}
#endif
/* NDS_R2_CUBIC_FIXED_KERNEL_END */

/* The original body with the arithmetic replaced twice over: E64's fixed cubic
 * for float AObjs, and Requirement 4's Q dispatch for the fighter ones. The
 * float arms below are still the decomp's own expressions verbatim and still
 * run for every non-fighter DObj this player is called for. */
void gcPlayDObjAnimJoint(DObj *dobj)
{
    f32 value = 0.0f;
    AObj *aobj;

    /* R2-06 E15. `anim_wait` is an f32 and both sentinels are compile-time
     * constants, so these were `bl __aeabi_fcmpeq` -- and the two outer ones run
     * per DObj while the `!= AOBJ_ANIM_END` runs per AObj node. This function is
     * the single largest caller of the comparison helpers in the whole profile:
     * 227,040 calls, 2,582,802 cycles. Both sentinels are F32_MIN-derived and so
     * non-zero, which makes a bit-pattern compare exact -- IEEE-754 gives every
     * value except zero a unique representation. Proven over all 2^32 patterns
     * by scripts/check_fcmp_exact.py, not argued from here. The Step arm's
     * `length_invert <= length` is two RUNTIME floats and stays a call. */
    if (NDS_FCMP_NE_C(dobj->anim_wait, AOBJ_ANIM_NULL))
    {
        /* Cycle 109: hoist the two loop-invariant conditions and the speed.
         *
         * Neither test depends on `aobj`, yet the cycle-106 profile shows both
         * running once per NODE, 110,110 times: `ldr r1,[pc,#224]` -- reloading
         * the `AOBJ_ANIM_END` literal -- costs **9.7 cyc/ex and 1,069,318
         * cycles, 6.4% of this function**, and the `parent_gobj->flags` chain
         * (`ldr r3,[r3,#124]` then `ldr r3,[r2,r3]`) adds 886,637 more.
         *
         * GCC cannot hoist them itself: the loop body calls `syInterpCubic` and
         * the `noinline` cubic kernel, and a call may clobber memory, so every
         * `dobj` field has to be re-read after it. Doing it by hand is safe
         * because nothing reachable from this loop writes `anim_wait`,
         * `anim_speed` or the GObj flags -- the body only writes `aobj->length`
         * and `dobj`'s own rotate/translate/scale vectors.
         *
         * Pure loop-invariant code motion: no struct, format or arithmetic
         * change, so the pose is bit-identical. */
        /* One mask, not two booleans. With them separate, GCC kept the flags
         * test in a register but rematerialised the wait test inside the loop
         * as `ldr r3,[pc,#296]` + `cmp` -- the literal reload this exists to
         * delete. Folding both into a single computed word makes
         * rematerialising strictly more expensive than keeping it, because it
         * would have to redo the load AND the mask. */
        const u32 play_hoisted =
            (NDS_FCMP_NE_C(dobj->anim_wait, AOBJ_ANIM_END) ? 1u : 0u) |
            (((dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM) == 0) ? 2u : 0u);
        const f32 speed_hoisted = dobj->anim_speed;
        /* Requirement 4: the per-node `aobj->length += speed` was an
         * `__aeabi_fadd` on EVERY node of every frame, Q or not. Converting the
         * speed once per DObj turns it into an integer add on the Q nodes. */
        const s32 speed_q_hoisted =
            ndsR2F32ToFixed(speed_hoisted, NDS_R2_AQ_LF);
        /* Route bit 0. Read once per DObj -- reading it per node would put a
         * volatile load in the very loop being measured. The pre-cut arm
         * re-derives both from `dobj` per node, which is the decomp's own shape
         * (objanim.c reads anim_wait, anim_speed and parent_gobj->flags inside
         * the loop); it keeps NDS_FCMP_NE_C rather than restoring the
         * __aeabi_fcmpeq call, because the fcmp->bit-compare change is a
         * separate landed cut and conflating the two would price neither. */
        const u32 hoist = NDS_R2_ANIM_CUT_ON(1u) ? 1u : 0u;

        aobj = dobj->aobj;

        while (aobj != NULL)
        {
            if (aobj->kind != nGCAnimKindNone)
            {
                /* One byte load, two compares against a constant. This is the
                 * whole cost of letting fixed-point and float AObjs coexist in
                 * the same list -- which they must, because this player runs
                 * for stage and item DObjs too and only the fighter parser
                 * writes Q. */
                const u32 kind = aobj->kind;
                u32 play;
                f32 speed;
                s32 speed_q;

                if (hoist != 0u)
                {
                    play = play_hoisted;
                    speed = speed_hoisted;
                    speed_q = speed_q_hoisted;
                }
                else
                {
                    play =
                        (NDS_FCMP_NE_C(dobj->anim_wait, AOBJ_ANIM_END) ?
                            1u : 0u) |
                        (((dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM) == 0) ?
                            2u : 0u);
                    speed = dobj->anim_speed;
                    speed_q = ndsR2F32ToFixed(speed, NDS_R2_AQ_LF);
                }
                if ((play & 1u) != 0u)
                {
                    if (kind >= NDS_R2_AQ_KIND_BASE)
                    {
                        aobj->length =
                            ndsR2AQStore(ndsR2AQLoad(aobj->length) + speed_q);
                    }
                    else
                    {
                        aobj->length += speed;
                    }
                }
                if ((play & 2u) != 0u)
                {
                    if (kind >= NDS_R2_AQ_KIND_BASE)
                    {
                        value = ndsR2AnimValueQ(aobj);
                    }
                    else
                    {
                        switch (kind)
                        {
                        case nGCAnimKindLinear:
                            value = aobj->value_base +
                                (aobj->length * aobj->rate_base);
                            break;

                        case nGCAnimKindCubic:
                            value = ndsR2CubicValueFixed(aobj);
                            break;

                        case nGCAnimKindStep:
                            value = (aobj->length_invert <= aobj->length) ?
                                aobj->value_target : aobj->value_base;
                            break;

                        default:
                            break;
                        }
                    }
                    switch (aobj->track)
                    {
                    case nGCAnimTrackRotX: dobj->rotate.vec.f.x = value; break;
                    case nGCAnimTrackRotY: dobj->rotate.vec.f.y = value; break;
                    case nGCAnimTrackRotZ: dobj->rotate.vec.f.z = value; break;

                    case nGCAnimTrackTraI:
                        /* Clamp to [0,1]. LT0 is `> 0x80000000u` rather than
                         * `>=` so that -0.0f does not count as negative, which
                         * is what IEEE says and what the exhaustive check
                         * enforces. */
                        if (NDS_FCMP_LT0(value))
                        {
                            value = 0.0F;
                        }
                        else if (NDS_FCMP_GT_C(value, 1.0F))
                        {
                            value = 1.0F;
                        }
                        syInterpCubic(&dobj->translate.vec.f,
                                      aobj->interpolate, value);
                        break;

                    case nGCAnimTrackTraX: dobj->translate.vec.f.x = value; break;
                    case nGCAnimTrackTraY: dobj->translate.vec.f.y = value; break;
                    case nGCAnimTrackTraZ: dobj->translate.vec.f.z = value; break;
                    case nGCAnimTrackScaX: dobj->scale.vec.f.x = value; break;
                    case nGCAnimTrackScaY: dobj->scale.vec.f.y = value; break;
                    case nGCAnimTrackScaZ: dobj->scale.vec.f.z = value; break;
                    default: break;
                    }
                }
            }
            aobj = aobj->next;
        }
        if (NDS_FCMP_EQ_C(dobj->anim_wait, AOBJ_ANIM_END))
        {
            dobj->anim_wait = AOBJ_ANIM_NULL;
        }
    }
}
#endif

#if NDS_R2_ANIM_CENSUS
#undef gcPlayDObjAnimJoint

/* R2-03 E61. E60 priced this path at 146,942 ticks/frame inclusive -- larger
 * than the whole gap to the gate -- and 280 ticks per AObj node, which is what
 * a ~14-operation cubic costs in software float. Before any of that is
 * rewritten, three integers decide WHICH rewrite:
 *
 *   1. the kind mix. Cubic is ~14 float ops, Linear is 2, Step is 0. If the
 *      nodes are mostly Linear the arithmetic is not the target and E60's
 *      per-node arithmetic reading is wrong.
 *   2. anim_speed. `length` is a pure accumulator of it, so if it only ever
 *      takes 0 or 1 the pose is a function of an INTEGER frame index and a
 *      load-time table is bit-exact. Any other value and the index is
 *      continuous and no table can be exact.
 *   3. how many evaluations are discarded. The original computes `value`
 *      before checking GOBJ_FLAG_NOANIM... it does not, but it DOES skip the
 *      whole evaluation under that flag, so counting the skips separates
 *      "poses computed" from "poses used".
 *
 * Counting only -- the real work is delegated unchanged, so this cannot change
 * a value. Task 96 measured the chain (337.8 nodes/frame over 104.1 calls) with
 * the same interposition and its numbers are the cross-check. */
extern void ndsBaseGcPlayDObjAnimJoint(DObj *dobj);

volatile u32 gNdsR2AnimCensusCalls;
volatile u32 gNdsR2AnimCensusNodes;
volatile u32 gNdsR2AnimCensusKindNone;
volatile u32 gNdsR2AnimCensusKindLinear;
volatile u32 gNdsR2AnimCensusKindCubic;
volatile u32 gNdsR2AnimCensusKindStep;
volatile u32 gNdsR2AnimCensusKindOther;
volatile u32 gNdsR2AnimCensusSpeedOne;
volatile u32 gNdsR2AnimCensusSpeedZero;
volatile u32 gNdsR2AnimCensusSpeedOther;
volatile u32 gNdsR2AnimCensusSpeedOtherBits;
volatile u32 gNdsR2AnimCensusNoAnimSkips;
volatile u32 gNdsR2AnimCensusAnimEnd;
volatile u32 gNdsR2AnimCensusLongestChain;

void gcPlayDObjAnimJoint(DObj *dobj)
{
    const AObj *aobj;
    u32 chain = 0u;
    f32 speed;
    u32 speed_bits;
    u32 noanim;

    gNdsR2AnimCensusCalls++;
    if (dobj->anim_wait != AOBJ_ANIM_NULL)
    {
        speed = dobj->anim_speed;
        __builtin_memcpy(&speed_bits, &speed, sizeof(speed_bits));
        if (speed_bits == 0x3f800000u)
        {
            gNdsR2AnimCensusSpeedOne++;
        }
        else if ((speed_bits & 0x7fffffffu) == 0u)
        {
            gNdsR2AnimCensusSpeedZero++;
        }
        else
        {
            gNdsR2AnimCensusSpeedOther++;
            gNdsR2AnimCensusSpeedOtherBits = speed_bits;
        }
        if (dobj->anim_wait == AOBJ_ANIM_END)
        {
            gNdsR2AnimCensusAnimEnd++;
        }
        noanim = ((dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM) != 0) ? 1u : 0u;
        for (aobj = dobj->aobj; aobj != NULL; aobj = aobj->next)
        {
            chain++;
            if (aobj->kind == nGCAnimKindNone)
            {
                gNdsR2AnimCensusKindNone++;
                continue;
            }
            gNdsR2AnimCensusNodes++;
            if (noanim != 0u)
            {
                gNdsR2AnimCensusNoAnimSkips++;
            }
            switch (aobj->kind)
            {
            case nGCAnimKindLinear: gNdsR2AnimCensusKindLinear++; break;
            case nGCAnimKindCubic:  gNdsR2AnimCensusKindCubic++;  break;
            case nGCAnimKindStep:   gNdsR2AnimCensusKindStep++;   break;
            default:                gNdsR2AnimCensusKindOther++;  break;
            }
        }
        if (chain > gNdsR2AnimCensusLongestChain)
        {
            gNdsR2AnimCensusLongestChain = chain;
        }
    }
    ndsBaseGcPlayDObjAnimJoint(dobj);
}
#endif

/* THE NORMALIZED SET IS A LEDGER, NOT A CACHE, so its capacity has to cover
 * the whole corpus a scene can reach -- it cannot be evicted from.
 *
 * The repack (source opcode[31:25] flags[24:15] payload[14:0] -> native
 * opcode[6:0] flags[16:7] payload[31:17]) is a bit permutation with no spare
 * bit, so a word cannot say which layout it is in. This table is the only
 * record. Evicting an entry, or overflowing and re-normalizing later, applies
 * the permutation TWICE to a live script, which is unrecoverable corruption --
 * so no LRU, no reclaim, and reclaiming at file unload cannot help a long
 * match, because a match unloads nothing. `ndsAObjEvent32ResetNormalizedScripts`
 * is called from `ndsRelocResetLoadedFiles` (`reloc_backend_assets.c:2093`) and
 * is the ONLY correct discard point.
 *
 * 1024 was too small, measured, not estimated
 * (`artifacts/performance/2026-08-13_c-anim-anomalies/ANOMALIES.md`):
 *
 *   1-minute both-CPU gate arm  889 of 1024 commands (119 scripts, 294 reuses)
 *   5-minute both-CPU match   1,019 of 1024 commands
 *
 * i.e. the SHIPPING match length already stands at 87% of capacity and the
 * acceptance match at 99.5%, with `gNdsAObjEvent32NormalizeFailCount` 0 in both
 * -- the cliff has never been fallen off, and there are five slots left when it
 * would be. Growth is coverage of a finite corpus, not a leak: the per-stop
 * trajectory has four consecutive zero-growth stops (frames 1302-1686) while
 * the reuse path keeps firing 16-19 times per stop.
 *
 * Overflow is reason 12, and its consequence is NOT a dropped command: every
 * `gcAdd*Anim*` wrapper below skips the whole `ndsBaseGcAdd*` on a FALSE, so
 * one over-cap script silently cancels an entire animation attach -- and for
 * `ndsAObjEvent32NormalizeDObjTable` it cancels every joint of the GObj, not
 * just the failing one.
 *
 * 2048 was chosen as 2x the largest corpus known WHEN IT WAS CHOSEN (1,019, the
 * pre-anim-joint-fix five-minute figure), and that arithmetic is stale: the
 * anim-joint fix gave the shield's joint installer a body, and the five-minute
 * corpus re-measured at 1,598 (`gNdsAObjEvent32NormalizedHighWater`,
 * `../2026-08-13_c-ledger-index/LEDGER_INDEX.md` section 4). The real margin is
 * therefore 450 spare slots, 1.28x, NOT 1,029 spare and 2x -- for 8,192 bytes of
 * bss against 176,128 of proven boot headroom. `NormalizeFailCount` is still 0
 * on both lengths and the ledger index did not move this number (it removed
 * repeated FINDING, not repeated normalizing), so capacity remains the open
 * question at exactly 1,598 of 2,048. Re-derive it from the high-water counter
 * rather than from this comment. */
#define NDS_AOBJ_EVENT32_NORMALIZED_MAX 2048u
#define NDS_AOBJ_EVENT32_PLAN_MAX 128u
#define NDS_AOBJ_EVENT32_BRANCH_DEPTH_MAX 16u

typedef enum NDSAObjEvent32OwnerKind
{
    nNDSAObjEvent32OwnerDObj,
    nNDSAObjEvent32OwnerMObj,
    nNDSAObjEvent32OwnerCObj
} NDSAObjEvent32OwnerKind;

typedef struct NDSAObjEvent32Normalized
{
    AObjEvent32 *command;
    u32 native_word;
} NDSAObjEvent32Normalized;

typedef struct NDSAObjEvent32Plan
{
    AObjEvent32 *command;
    u32 source_word;
    u32 native_word;
} NDSAObjEvent32Plan;

static NDSAObjEvent32Normalized
    sNdsAObjEvent32Normalized[NDS_AOBJ_EVENT32_NORMALIZED_MAX];
static NDSAObjEvent32Plan sNdsAObjEvent32Plan[NDS_AOBJ_EVENT32_PLAN_MAX];
static u32 sNdsAObjEvent32NormalizedCount;
static u32 sNdsAObjEvent32PlanCount;

/* AN INDEX OVER THE LEDGER ABOVE -- not a second cache, and the distinction is
 * the whole of its safety argument.
 *
 * ndsAObjEvent32FindNormalized was a linear scan of sNdsAObjEvent32Normalized,
 * and after the 2026-08-13 shield anim-joint fix (607d3697455) the shield's
 * install path calls it once per attached joint: 1,344 times a minute on the
 * gate arm, 9,154 in a five-minute match, each one a hit near the far end of a
 * table that reaches 1,177 entries in a minute and 1,598 in five. That scan is
 * ~88% of the 5,123 ticks each attach costs (the fix's whole price, isolated by
 * the one-variable five-minute pair in
 * artifacts/performance/2026-08-13_c-animjoint-fix/, re-derived per attach in
 * ../2026-08-13_c-ledger-index/LEDGER_INDEX.md).
 *
 * SwitchPlan 3.12 bans keying anything on a pointer that survives a scene
 * boundary. This does not introduce such a key: the LEDGER is already keyed on
 * the command pointer, and this array holds nothing but positions inside it.
 * Same array, same contents, same single reset seam
 * (ndsAObjEvent32ResetNormalizedScripts, from ndsRelocResetLoadedFiles). There
 * is no new lifetime to get wrong, no invalidation to forget, and no state that
 * can disagree with the ledger -- which is exactly what the five 3.12 incidents
 * did have. The ledger's own `script->u == native_word` re-check is untouched.
 *
 * Exactness: the ledger's keys are unique. ndsAObjEvent32PlanStream returns as
 * soon as it reaches an already-normalized command and ndsAObjEvent32FindPlanned
 * blocks duplicates inside one plan, so no command is ever appended twice. With
 * unique keys an open-addressed probe returns the same index the scan returned,
 * bit for bit. NDS_AOBJ_EVENT32_HASH_ORACLE proves that rather than asserting it.
 *
 * Slots hold index+1 so that a zeroed .bss reads as EMPTY. That is not a style
 * choice: the first normalize can precede the first ndsRelocResetLoadedFiles,
 * and with a 0xffff sentinel an unreset table would be 4,096 occupied slots that
 * match nothing, i.e. a probe that never terminates -- 3.11's freeze class, in
 * the one subsystem whose failure mode is already a freeze. */
#define NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS 4096u

_Static_assert((NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS &
                (NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS - 1u)) == 0u,
               "AObj event-32 ledger index must be a power of two");
_Static_assert(NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS >
               NDS_AOBJ_EVENT32_NORMALIZED_MAX,
               "AObj event-32 ledger index must keep a free slot per entry");
_Static_assert(NDS_AOBJ_EVENT32_NORMALIZED_MAX < 0xffffu,
               "AObj event-32 ledger index stores index+1 in a u16");

static u16
    sNdsAObjEvent32NormalizedHash[NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS];

/* NO SECOND INDEX OVER sNdsAObjEvent32Plan -- MEASURED, 2026-08-13, and this
 * note exists so the next cycle does not build the one that was briefed.
 *
 * `../2026-08-13_c-ledger-index/LEDGER_INDEX.md` section 1 named
 * ndsAObjEvent32FindPlanned as "a second O(n^2) scan, 1,064,828 tk/match, ~665
 * tk/frame", from the c123 per-PC profile's PC range 0x02065cb6-0x02065cc2 at
 * 161,203 iterations. That attribution is WRONG, and the same index was built
 * here and instrumented to find out: over a whole one-minute both-CPU match the
 * function is entered 1,188 times -- 1,177 misses (one per appended command,
 * which is exactly gNdsAObjEvent32NormalizeCommandCount) and 11 hits -- against
 * 183 scripts. The plan table is reset per script and capped at 128, so the
 * scan it replaces is a few entries deep, not a thousand: 13 tk/frame at the
 * uniform 6.4 commands per script this run measured, and ~302 tk/frame even at
 * the most concentrated distribution 183 scripts and 1,177 commands allow.
 * 161,203 iterations cannot happen in 1,188 calls over a 128-entry table; that
 * profile PC range is the FindNormalized scan, which PlanStream also inlines,
 * once per command, over a ledger that reaches 1,177 entries.
 *
 * So the index was reverted rather than shipped: 256 bytes of bss and its text
 * for at most a P50 crumb on the normalize frames, which sit at low gate ranks.
 * Numbers in ../../artifacts/performance/2026-08-13_c-collision-stack/. */

/* Engagement, both directions. Probes/Lookups is the load-factor readout that
 * says whether the index is behaving; Overflow must stay 0 and its non-zero
 * meaning is "the fallback below ran", never "a lookup was wrong". */
volatile u32 gNdsAObjEvent32HashHitCount;
volatile u32 gNdsAObjEvent32HashMissCount;
volatile u32 gNdsAObjEvent32HashProbeCount;
volatile u32 gNdsAObjEvent32HashInsertProbeCount;
volatile u32 gNdsAObjEvent32HashOverflowCount;
volatile u32 gNdsAObjEvent32HashOracleRuns;
volatile u32 gNdsAObjEvent32HashOracleMismatch;

/* Commands are 4-byte objects inside one loaded file, so the interesting bits
 * are low and adjacent; the two folds spread them over the whole slot range
 * without a multiply. */
static u32 ndsAObjEvent32HashSlot(const AObjEvent32 *command)
{
    u32 h = (u32)(uintptr_t)command >> 2;

    h ^= h >> 7;
    h ^= h >> 13;
    return h & (NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS - 1u);
}

static s32 ndsAObjEvent32ScanNormalized(const AObjEvent32 *command)
{
    u32 i;

    for (i = 0u; i < sNdsAObjEvent32NormalizedCount; i++)
    {
        if (sNdsAObjEvent32Normalized[i].command == command)
        {
            return (s32)i;
        }
    }
    return -1;
}

static void ndsAObjEvent32IndexNormalized(u32 index)
{
    u32 slot = ndsAObjEvent32HashSlot(sNdsAObjEvent32Normalized[index].command);
    u32 probes;

    for (probes = 0u; probes < NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS;
         probes++)
    {
        if (sNdsAObjEvent32NormalizedHash[slot] == 0u)
        {
            sNdsAObjEvent32NormalizedHash[slot] = (u16)(index + 1u);
            gNdsAObjEvent32HashInsertProbeCount += probes + 1u;
            return;
        }
        slot = (slot + 1u) & (NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS - 1u);
    }
    /* Unreachable while the static assert above holds: the ledger cannot hold
     * more entries than the index has slots. Counted rather than asserted so
     * that if it ever does happen the lookup below degrades to the scan it
     * replaced instead of dropping an entry. */
    gNdsAObjEvent32HashOverflowCount++;
}

/* sNdsAObjEvent32NormalizedCount is reset on every scene teardown, so reading
 * it at the end of a run reports the LAST scene only. That is how a five-entry
 * chain reported 297 and got written up as "a chain cannot fill the table"
 * while a single match was standing at 889 (2026-08-13 stress battery). This is
 * the peak across resets, and it is what a soak should read. */
volatile u32 gNdsAObjEvent32NormalizedHighWater;
volatile u32 gNdsAObjEvent32NormalizeScriptCount;
volatile u32 gNdsAObjEvent32NormalizeCommandCount;
volatile u32 gNdsAObjEvent32NormalizeReuseCount;
volatile u32 gNdsAObjEvent32NormalizeFailCount;
volatile u32 gNdsAObjEvent32NormalizeFirstSourceWord;
volatile u32 gNdsAObjEvent32NormalizeFirstNativeWord;
volatile u32 gNdsAObjEvent32NormalizeLastFailReason;
volatile u32 gNdsAObjEvent32NormalizeLastFailOwner;
volatile u32 gNdsAObjEvent32NormalizeLastFailAddress;
volatile u32 gNdsAObjEvent32NormalizeLastFailWord;
volatile u32 gNdsAObjEvent32NormalizeLastFailOpcode;
volatile u32 gNdsAObjEvent32NormalizeLastFailFlags;
volatile u32 gNdsAObjEvent32ColorCorrectionCount;

volatile u32 gNdsMObjSubAttachNormalizeCount;
volatile u32 gNdsMObjSubAttachNativeCount;
volatile u32 gNdsMObjSubAttachFailCount;
volatile u32 gNdsMObjSubAttachFirstSourceFlags;
volatile u32 gNdsMObjSubAttachFirstNativeFlags;

/* Preserve objanim.c:2429-2455 exactly except for the compatibility copy at
 * the MObj attachment boundary. gcAddMObjForDObj copies the full MObjSub, so
 * the normalized stack record cannot escape this call. */
void gcAddMObjAll(GObj *gobj, MObjSub ***p_mobjsubs)
{
    DObj *dobj = DObjGetStruct(gobj);

    while (dobj != NULL)
    {
        if (p_mobjsubs != NULL)
        {
            if (*p_mobjsubs != NULL)
            {
                MObjSub **mobjsubs = *p_mobjsubs;
                MObjSub *mobjsub = *mobjsubs;

                while (mobjsub != NULL)
                {
                    MObjSub normalized_mobjsub;
                    s32 normalize_result =
                        ndsRelocCopyMObjSubForAttachment(
                            &normalized_mobjsub, mobjsub);

                    if (normalize_result > 0)
                    {
                        if (gNdsMObjSubAttachNormalizeCount == 0u)
                        {
                            gNdsMObjSubAttachFirstSourceFlags =
                                mobjsub->flags;
                            gNdsMObjSubAttachFirstNativeFlags =
                                normalized_mobjsub.flags;
                        }
                        gNdsMObjSubAttachNormalizeCount++;
                    }
                    else if (normalize_result == 0)
                    {
                        gNdsMObjSubAttachNativeCount++;
                    }
                    else
                    {
                        gNdsMObjSubAttachFailCount++;
                        /* A malformed loaded record is neither a safe native
                         * attachment nor a valid O2R conversion. Fail closed;
                         * the canonical scene requires this path to stay at
                         * zero, so no source material is silently omitted. */
                        mobjsubs++;
                        mobjsub = *mobjsubs;
                        continue;
                    }

                    gcAddMObjForDObj(dobj, &normalized_mobjsub);

                    mobjsubs++;
                    mobjsub = *mobjsubs;
                }
            }
            p_mobjsubs++;
        }
        dobj = gcGetTreeDObjNext(dobj);
    }
}

extern s32 ndsRelocPointerRangeInLoadedFiles(const void *ptr, size_t size);
extern s32 ndsRelocPointerIsFighterAObj16(const void *ptr);

static u32 ndsAObjEvent32CountFlags(u32 flags)
{
    u32 count = 0u;

    while (flags != 0u)
    {
        count += flags & 1u;
        flags >>= 1;
    }
    return count;
}

static s32 ndsAObjEvent32FindNormalized(AObjEvent32 *command)
{
#if !NDS_AOBJ_EVENT32_LEDGER_INDEX
    /* Falsifier arm: the index is still built and still occupies its bss, so
     * every section places as it does in the shipping arm; only the lookup
     * reverts. See the Makefile flag for why a rebuilt control cannot do this. */
    return ndsAObjEvent32ScanNormalized(command);
#else
    u32 slot = ndsAObjEvent32HashSlot(command);
    s32 found = -1;
    u32 probes;

    for (probes = 0u; probes < NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS;
         probes++)
    {
        u32 entry = sNdsAObjEvent32NormalizedHash[slot];

        if (entry == 0u)
        {
            gNdsAObjEvent32HashMissCount++;
            break;
        }
        if (sNdsAObjEvent32Normalized[entry - 1u].command == command)
        {
            gNdsAObjEvent32HashHitCount++;
            found = (s32)(entry - 1u);
            break;
        }
        slot = (slot + 1u) & (NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS - 1u);
    }
    gNdsAObjEvent32HashProbeCount += probes + 1u;
    if (probes == NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS)
    {
        gNdsAObjEvent32HashOverflowCount++;
        return ndsAObjEvent32ScanNormalized(command);
    }
#if NDS_AOBJ_EVENT32_HASH_ORACLE
    {
        s32 scanned = ndsAObjEvent32ScanNormalized(command);

        gNdsAObjEvent32HashOracleRuns++;
        if (scanned != found)
        {
            gNdsAObjEvent32HashOracleMismatch++;
            gNdsAObjEvent32NormalizeLastFailAddress = (u32)(uintptr_t)command;
            found = scanned;
        }
    }
#endif
    return found;
#endif
}

static s32 ndsAObjEvent32FindPlanned(AObjEvent32 *command)
{
    u32 i;

    for (i = 0u; i < sNdsAObjEvent32PlanCount; i++)
    {
        if (sNdsAObjEvent32Plan[i].command == command)
        {
            return (s32)i;
        }
    }
    return -1;
}

static sb32 ndsAObjEvent32Reject(u32 reason, AObjEvent32 *command,
                                 NDSAObjEvent32OwnerKind owner_kind,
                                 u32 source_word)
{
    gNdsAObjEvent32NormalizeLastFailReason = reason;
    gNdsAObjEvent32NormalizeLastFailOwner = (u32)owner_kind;
    gNdsAObjEvent32NormalizeLastFailAddress = (u32)(uintptr_t)command;
    gNdsAObjEvent32NormalizeLastFailWord = source_word;
    gNdsAObjEvent32NormalizeLastFailOpcode =
        (source_word >> 25) & 0x7fu;
    gNdsAObjEvent32NormalizeLastFailFlags =
        (source_word >> 15) & 0x03ffu;
    return FALSE;
}

/* objdef.h:272-281 defines the source word as opcode[31:25], flags[24:15],
 * payload[14:0]. ARM GCC allocates objtypes.h:94-107 bitfields from the low
 * bit, so only command words are repacked; following values and pointers stay
 * in their already-relocated O2R representation. */
static sb32 ndsAObjEvent32PlanStream(AObjEvent32 *script,
                                     NDSAObjEvent32OwnerKind owner_kind,
                                     u32 branch_depth)
{
    AObjEvent32 *command = script;

    if ((script == NULL) ||
        (branch_depth > NDS_AOBJ_EVENT32_BRANCH_DEPTH_MAX))
    {
        return ndsAObjEvent32Reject(1u, script, owner_kind, 0u);
    }

    while (TRUE)
    {
        AObjEvent32 *branch_target = NULL;
        u32 source_word;
        u32 opcode;
        u32 flags;
        u32 payload;
        u32 value_words = 0u;
        s32 normalized_index;
        sb32 is_end = FALSE;
        sb32 is_branch = FALSE;

        if (ndsRelocPointerRangeInLoadedFiles(command, sizeof(*command)) ==
            FALSE)
        {
            return ndsAObjEvent32Reject(2u, command, owner_kind, 0u);
        }

        normalized_index = ndsAObjEvent32FindNormalized(command);
        if (normalized_index >= 0)
        {
            return (command->u ==
                    sNdsAObjEvent32Normalized[normalized_index].native_word) ?
                       TRUE : ndsAObjEvent32Reject(3u, command, owner_kind,
                                                   command->u);
        }
        if (ndsAObjEvent32FindPlanned(command) >= 0)
        {
            return TRUE;
        }

        source_word = command->u;
        opcode = (source_word >> 25) & 0x7fu;
        flags = (source_word >> 15) & 0x03ffu;
        payload = source_word & 0x7fffu;

        switch (opcode)
        {
        case nGCAnimEvent32End:
            is_end = TRUE;
            break;

        case nGCAnimEvent32Jump:
        case nGCAnimEvent32SetAnim:
            value_words = 1u;
            is_branch = TRUE;
            break;

        case nGCAnimEvent32Wait:
        case ANIM_CMD_12:
            break;

        case nGCAnimEvent32SetValBlock:
        case nGCAnimEvent32SetVal:
        case nGCAnimEvent32SetTargetRate:
        case nGCAnimEvent32SetVal0RateBlock:
        case nGCAnimEvent32SetVal0Rate:
        case nGCAnimEvent32SetValAfterBlock:
        case nGCAnimEvent32SetValAfter:
            value_words = ndsAObjEvent32CountFlags(flags);
            break;

        case nGCAnimEvent32SetValRateBlock:
        case nGCAnimEvent32SetValRate:
            value_words = ndsAObjEvent32CountFlags(flags) * 2u;
            break;

        case nGCAnimEvent32SetInterp:
            if (owner_kind == nNDSAObjEvent32OwnerDObj)
            {
                value_words = 1u;
            }
            else if (owner_kind == nNDSAObjEvent32OwnerCObj)
            {
                value_words = ((flags & 0x08u) != 0u) +
                              ((flags & 0x80u) != 0u);
            }
            else
            {
                return ndsAObjEvent32Reject(4u, command, owner_kind,
                                            source_word);
            }
            break;

        case nGCAnimEvent32SetFlags:
        case ANIM_CMD_16:
            if (owner_kind != nNDSAObjEvent32OwnerDObj)
            {
                return ndsAObjEvent32Reject(5u, command, owner_kind,
                                            source_word);
            }
            break;

        case ANIM_CMD_17:
            if (owner_kind != nNDSAObjEvent32OwnerDObj)
            {
                return ndsAObjEvent32Reject(6u, command, owner_kind,
                                            source_word);
            }
            value_words = ndsAObjEvent32CountFlags(flags);
            break;

        case nGCAnimEvent32SetExtValAfterBlock:
        case nGCAnimEvent32SetExtValAfter:
        case nGCAnimEvent32SetExtValBlock:
        case nGCAnimEvent32SetExtVal:
            if (owner_kind != nNDSAObjEvent32OwnerMObj)
            {
                return ndsAObjEvent32Reject(7u, command, owner_kind,
                                            source_word);
            }
            value_words = ndsAObjEvent32CountFlags(flags);
            break;

        case ANIM_CMD_22:
            if (owner_kind != nNDSAObjEvent32OwnerMObj)
            {
                return ndsAObjEvent32Reject(8u, command, owner_kind,
                                            source_word);
            }
            value_words = ndsAObjEvent32CountFlags(flags & 0x1fu);
            break;

        case ANIM_CMD_23:
            if (owner_kind != nNDSAObjEvent32OwnerCObj)
            {
                return ndsAObjEvent32Reject(9u, command, owner_kind,
                                            source_word);
            }
            /* objanim.c:2811-2813 consumes this command, then skips two
             * payload words before parsing the next command. */
            value_words = 2u;
            break;

        default:
            return ndsAObjEvent32Reject(10u, command, owner_kind,
                                        source_word);
        }

        if ((sNdsAObjEvent32PlanCount >= NDS_AOBJ_EVENT32_PLAN_MAX) ||
            (ndsRelocPointerRangeInLoadedFiles(
                 command, (1u + value_words) * sizeof(*command)) == FALSE))
        {
            return ndsAObjEvent32Reject(11u, command, owner_kind,
                                        source_word);
        }

        sNdsAObjEvent32Plan[sNdsAObjEvent32PlanCount].command = command;
        sNdsAObjEvent32Plan[sNdsAObjEvent32PlanCount].source_word = source_word;
        sNdsAObjEvent32Plan[sNdsAObjEvent32PlanCount].native_word =
            opcode | (flags << 7) | (payload << 17);
        sNdsAObjEvent32PlanCount++;

        if (is_end != FALSE)
        {
            return TRUE;
        }
        if (is_branch != FALSE)
        {
            branch_target = command[1].p;
            return ndsAObjEvent32PlanStream(branch_target, owner_kind,
                                             branch_depth + 1u);
        }
        command += 1u + value_words;
    }
}

static sb32 ndsAObjEvent32NormalizeScript(
    AObjEvent32 *script, NDSAObjEvent32OwnerKind owner_kind)
{
    u32 i;
    s32 normalized_index;

    if (script == NULL)
    {
        return TRUE;
    }

    normalized_index = ndsAObjEvent32FindNormalized(script);
    if (normalized_index >= 0)
    {
        if (script->u ==
            sNdsAObjEvent32Normalized[normalized_index].native_word)
        {
            gNdsAObjEvent32NormalizeReuseCount++;
            return TRUE;
        }
        (void)ndsAObjEvent32Reject(3u, script, owner_kind, script->u);
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }

    sNdsAObjEvent32PlanCount = 0u;
    if (ndsAObjEvent32PlanStream(script, owner_kind, 0u) == FALSE)
    {
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }
    if ((sNdsAObjEvent32NormalizedCount + sNdsAObjEvent32PlanCount) >
        NDS_AOBJ_EVENT32_NORMALIZED_MAX)
    {
        (void)ndsAObjEvent32Reject(12u, script, owner_kind, script->u);
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }

    if ((gNdsAObjEvent32NormalizeCommandCount == 0u) &&
        (sNdsAObjEvent32PlanCount != 0u))
    {
        gNdsAObjEvent32NormalizeFirstSourceWord =
            sNdsAObjEvent32Plan[0].source_word;
        gNdsAObjEvent32NormalizeFirstNativeWord =
            sNdsAObjEvent32Plan[0].native_word;
    }

    for (i = 0u; i < sNdsAObjEvent32PlanCount; i++)
    {
        sNdsAObjEvent32Plan[i].command->u =
            sNdsAObjEvent32Plan[i].native_word;
        sNdsAObjEvent32Normalized[sNdsAObjEvent32NormalizedCount].command =
            sNdsAObjEvent32Plan[i].command;
        sNdsAObjEvent32Normalized[sNdsAObjEvent32NormalizedCount].native_word =
            sNdsAObjEvent32Plan[i].native_word;
        ndsAObjEvent32IndexNormalized(sNdsAObjEvent32NormalizedCount);
        sNdsAObjEvent32NormalizedCount++;
    }

    if (sNdsAObjEvent32NormalizedCount > gNdsAObjEvent32NormalizedHighWater)
    {
        gNdsAObjEvent32NormalizedHighWater = sNdsAObjEvent32NormalizedCount;
    }
    gNdsAObjEvent32NormalizeScriptCount++;
    gNdsAObjEvent32NormalizeCommandCount += sNdsAObjEvent32PlanCount;
    return TRUE;
}

void ndsAObjEvent32ResetNormalizedScripts(void)
{
    u32 slot;

    /* The index is discarded with the ledger it indexes, at the ledger's one
     * correct discard point, in the same breath. Nothing else may clear either
     * one: SwitchPlan 3.12's whole lesson is that a cache with its own
     * invalidation schedule eventually disagrees with the thing it caches. */
    for (slot = 0u; slot < NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS; slot++)
    {
        sNdsAObjEvent32NormalizedHash[slot] = 0u;
    }
    sNdsAObjEvent32NormalizedCount = 0u;
    sNdsAObjEvent32PlanCount = 0u;
}

static u32 ndsAObjEvent32FloatBits(f32 value)
{
    union
    {
        f32 f;
        u32 u;
    } bits;

    bits.f = value;
    return bits.u;
}

static u8 ndsAObjEvent32LerpColorChannel(u32 base, u32 target,
                                         u32 shift, s32 interp)
{
    s32 base_channel = (s32)((base >> shift) & 0xffu);
    s32 target_channel = (s32)((target >> shift) & 0xffu);

    return (u8)((base_channel * (256 - interp) +
                 target_channel * interp) >> 8);
}

/* objanim.c:1363-1388 interpolates packed RGBA by multiplying carefully
 * spaced bytes inside a u32. That arithmetic depends on N64 big-endian byte
 * lanes. Keep the original player for timing/state, then rewrite only the
 * five color outputs from the source 0xRRGGBBAA payload bits. */
static void ndsAObjEvent32CorrectMObjColors(MObj *mobj, sb32 force)
{
    AObj *aobj;

    if ((mobj == NULL) ||
        ((force == FALSE) && (mobj->anim_wait == AOBJ_ANIM_NULL)))
    {
        return;
    }

    for (aobj = mobj->aobj; aobj != NULL; aobj = aobj->next)
    {
        SYColorPack color;
        u32 base;
        u32 target;
        s32 interp;

        if ((aobj->kind == nGCAnimKindNone) ||
            (aobj->track < nGCAnimTrackPrimColor) ||
            (aobj->track > nGCAnimTrackLight2Color))
        {
            continue;
        }

        base = ndsAObjEvent32FloatBits(aobj->value_base);
        target = ndsAObjEvent32FloatBits(aobj->value_target);

        if (aobj->kind == nGCAnimKindLinear)
        {
            interp = (s32)(aobj->length * aobj->length_invert * 256.0F);
            if (interp < 0)
            {
                interp = 0;
            }
            else if (interp > 256)
            {
                interp = 256;
            }

            color.s.r = ndsAObjEvent32LerpColorChannel(
                base, target, 24u, interp);
            color.s.g = ndsAObjEvent32LerpColorChannel(
                base, target, 16u, interp);
            color.s.b = ndsAObjEvent32LerpColorChannel(
                base, target, 8u, interp);
            color.s.a = ndsAObjEvent32LerpColorChannel(
                base, target, 0u, interp);
        }
        else if (aobj->kind == nGCAnimKindStep)
        {
            u32 packed = (aobj->length_invert <= aobj->length) ?
                             target : base;

            color.s.r = (u8)(packed >> 24);
            color.s.g = (u8)(packed >> 16);
            color.s.b = (u8)(packed >> 8);
            color.s.a = (u8)packed;
        }
        else
        {
            continue;
        }

        switch (aobj->track)
        {
        case nGCAnimTrackPrimColor:
            mobj->sub.primcolor = color;
            break;
        case nGCAnimTrackEnvColor:
            mobj->sub.envcolor = color;
            break;
        case nGCAnimTrackBlendColor:
            mobj->sub.blendcolor = color;
            break;
        case nGCAnimTrackLight1Color:
            mobj->sub.light1color = color;
            break;
        case nGCAnimTrackLight2Color:
            mobj->sub.light2color = color;
            break;
        default:
            continue;
        }
        gNdsAObjEvent32ColorCorrectionCount++;
    }
}

void gcPlayMObjMatAnim(MObj *mobj)
{
    sb32 was_active = ((mobj != NULL) &&
                       (mobj->anim_wait != AOBJ_ANIM_NULL));

    ndsBaseGcPlayMObjMatAnim(mobj);
    if (was_active != FALSE)
    {
        ndsAObjEvent32CorrectMObjColors(mobj, TRUE);
    }
}

static u32 ndsAObjEvent32CollectActiveMObjs(GObj *gobj, MObj **active_mobjs)
{
    DObj *dobj;
    u32 count = 0u;

    for (dobj = (gobj != NULL) ? DObjGetStruct(gobj) : NULL;
         dobj != NULL;
         dobj = gcGetTreeDObjNext(dobj))
    {
        MObj *mobj;

        for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
        {
            if (mobj->anim_wait != AOBJ_ANIM_NULL)
            {
                if (active_mobjs != NULL)
                {
                    active_mobjs[count] = mobj;
                }
                count++;
            }
        }
    }
    return count;
}

void gcPlayAnimAll(GObj *gobj)
{
    DObj *dobj;
    u32 active_count = ndsAObjEvent32CollectActiveMObjs(gobj, NULL);
    MObj *active_mobjs[(active_count != 0u) ? active_count : 1u];
    u32 i;

    (void)ndsAObjEvent32CollectActiveMObjs(gobj, active_mobjs);

    ndsBaseGcPlayAnimAll(gobj);

    for (dobj = (gobj != NULL) ? DObjGetStruct(gobj) : NULL;
         dobj != NULL;
         dobj = gcGetTreeDObjNext(dobj))
    {
        MObj *mobj;

        for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
        {
            ndsAObjEvent32CorrectMObjColors(mobj, FALSE);
        }
    }

    /* The original player changes END to NULL after writing the last frame.
     * Correct only objects that crossed that edge during this call. */
    for (i = 0u; i < active_count; i++)
    {
        MObj *mobj = active_mobjs[i];

        if (mobj->anim_wait == AOBJ_ANIM_NULL)
        {
            ndsAObjEvent32CorrectMObjColors(mobj, TRUE);
        }
    }
}

static sb32 ndsAObjEvent32NormalizeDObjTable(GObj *gobj,
                                             AObjEvent32 **anim_joints)
{
    DObj *dobj = DObjGetStruct(gobj);

    while ((dobj != NULL) && (anim_joints != NULL))
    {
        if ((*anim_joints != NULL) &&
            (ndsRelocPointerIsFighterAObj16(*anim_joints) == FALSE) &&
            (ndsAObjEvent32NormalizeScript(
                 *anim_joints, nNDSAObjEvent32OwnerDObj) == FALSE))
        {
            return FALSE;
        }
        anim_joints++;
        dobj = gcGetTreeDObjNext(dobj);
    }
    return TRUE;
}

static sb32 ndsAObjEvent32NormalizeMObjTable(
    GObj *gobj, AObjEvent32 ***p_matanim_joints)
{
    DObj *dobj = DObjGetStruct(gobj);

    while (dobj != NULL)
    {
        if ((p_matanim_joints != NULL) && (*p_matanim_joints != NULL))
        {
            AObjEvent32 **matanim_joints = *p_matanim_joints;
            MObj *mobj = dobj->mobj;

            while (mobj != NULL)
            {
                if (ndsAObjEvent32NormalizeScript(
                        *matanim_joints, nNDSAObjEvent32OwnerMObj) == FALSE)
                {
                    return FALSE;
                }
                matanim_joints++;
                mobj = mobj->next;
            }
        }
        if (p_matanim_joints != NULL)
        {
            p_matanim_joints++;
        }
        dobj = gcGetTreeDObjNext(dobj);
    }
    return TRUE;
}

#if NDS_R2_LOADFRAME_TIMING
/* R2-06 E10. E8 attributed 8 of the 9 over-gate frames to the 16 frames that load a
 * fighter animation, showed the +139,072 premium is entirely `SRC` (the source
 * update), and showed the in-frame relocation is only 21.5% of it. The other ~78% is
 * the ACTION CHANGE that causes the load, and its two obvious costs are the O2R
 * script normalization and the decomp's own animation setup -- both of which happen
 * to run inside these two already-interposed wrappers, so pricing them needs no new
 * seam. Lab only, default off; `Max` is per-call, not per-frame. */
volatile u32 gNdsR2AddDObjAnimCalls;
volatile u32 gNdsR2AddDObjAnimTicks;
volatile u32 gNdsR2AddDObjAnimMaxTicks;
volatile u32 gNdsR2AddDObjNormalizeTicks;
volatile u32 gNdsR2AddDObjBaseTicks;
volatile u32 gNdsR2AddAnimAllCalls;
volatile u32 gNdsR2AddAnimAllTicks;
volatile u32 gNdsR2AddAnimAllMaxTicks;
/* This TU pulls in no DS headers -- the decomp objanim.c includes only <sys/obj.h>
 * -- so declare the one libnds function the brackets need rather than dragging
 * nds/timers.h through the whole translation unit. */
u32 cpuGetTiming(void);
#endif

void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                        f32 anim_frame)
{
#if NDS_R2_LOADFRAME_TIMING
    u32 enter = cpuGetTiming();
    u32 phase = enter;
    sb32 admit;

    gNdsR2AddDObjAnimCalls++;
    admit = ((anim_joint == NULL) ||
             (ndsRelocPointerIsFighterAObj16(anim_joint) != FALSE) ||
             (ndsAObjEvent32NormalizeScript(
                  anim_joint, nNDSAObjEvent32OwnerDObj) != FALSE)) ? TRUE : FALSE;
    gNdsR2AddDObjNormalizeTicks += cpuGetTiming() - phase;
    if (admit != FALSE)
    {
        phase = cpuGetTiming();
        ndsBaseGcAddDObjAnimJoint(dobj, anim_joint, anim_frame);
        gNdsR2AddDObjBaseTicks += cpuGetTiming() - phase;
    }
    {
        u32 total = cpuGetTiming() - enter;

        gNdsR2AddDObjAnimTicks += total;
        if (total > gNdsR2AddDObjAnimMaxTicks)
        {
            gNdsR2AddDObjAnimMaxTicks = total;
        }
    }
#else
    if ((anim_joint == NULL) ||
        (ndsRelocPointerIsFighterAObj16(anim_joint) != FALSE) ||
        (ndsAObjEvent32NormalizeScript(
             anim_joint, nNDSAObjEvent32OwnerDObj) != FALSE))
    {
        ndsBaseGcAddDObjAnimJoint(dobj, anim_joint, anim_frame);
    }
#endif
}

void gcAddMObjMatAnimJoint(MObj *mobj, AObjEvent32 *matanim_joint,
                           f32 anim_frame)
{
    if (ndsAObjEvent32NormalizeScript(
            matanim_joint, nNDSAObjEvent32OwnerMObj) != FALSE)
    {
        ndsBaseGcAddMObjMatAnimJoint(mobj, matanim_joint, anim_frame);
    }
}

void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints,
                       f32 anim_frame)
{
#if NDS_R2_LOADFRAME_TIMING
    /* The whole-GObj variant, which is what a fighter action change goes through:
     * it walks the DObj tree and re-adds every joint's animation. If the action
     * change is the excursion, this is where it should show. */
    u32 enter = cpuGetTiming();

    gNdsR2AddAnimAllCalls++;
#endif
    if ((gobj != NULL) &&
        (ndsAObjEvent32NormalizeDObjTable(gobj, anim_joints) != FALSE))
    {
        ndsBaseGcAddAnimJointAll(gobj, anim_joints, anim_frame);
    }
#if NDS_R2_LOADFRAME_TIMING
    {
        u32 total = cpuGetTiming() - enter;

        gNdsR2AddAnimAllTicks += total;
        if (total > gNdsR2AddAnimAllMaxTicks)
        {
            gNdsR2AddAnimAllMaxTicks = total;
        }
    }
#endif
}

void gcAddMatAnimJointAll(GObj *gobj, AObjEvent32 ***p_matanim_joints,
                          f32 anim_frame)
{
    if ((gobj != NULL) &&
        (ndsAObjEvent32NormalizeMObjTable(gobj, p_matanim_joints) != FALSE))
    {
        ndsBaseGcAddMatAnimJointAll(gobj, p_matanim_joints, anim_frame);
    }
}

void gcAddAnimAll(GObj *gobj, AObjEvent32 **anim_joints,
                  AObjEvent32 ***p_matanim_joints, f32 anim_frame)
{
    if ((gobj != NULL) &&
        (ndsAObjEvent32NormalizeDObjTable(gobj, anim_joints) != FALSE) &&
        (ndsAObjEvent32NormalizeMObjTable(gobj, p_matanim_joints) != FALSE))
    {
        ndsBaseGcAddAnimAll(gobj, anim_joints, p_matanim_joints, anim_frame);
    }
}

void gcAddCObjCamAnimJoint(CObj *cobj, AObjEvent32 *camanim_joint,
                           f32 anim_frame)
{
    if (ndsAObjEvent32NormalizeScript(
            camanim_joint, nNDSAObjEvent32OwnerCObj) != FALSE)
    {
        ndsBaseGcAddCObjCamAnimJoint(cobj, camanim_joint, anim_frame);
    }
}
