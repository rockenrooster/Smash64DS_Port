/* Compile the original BattleShip object-animation/setup translation unit.
 * Its public add entrypoints normalize O2R AObjEvent32 command words before
 * the unchanged original parser sees them. */
#include <nds/nds_fcmp.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_anim_fixed.h>
#include <sc/scene.h>
#include <string.h>
#include <sys/taskman.h>

extern sb32 ndsSyMallocWouldFit(const SYMallocRegion *bp, size_t size,
                                u32 alignment);

/* Outside every configuration gate on purpose: `nds_anim_fixed.h` declares it
 * and `battleship_ftanim.c` includes that header whether or not this file's
 * fixed-point player is compiled in. Four bytes against a link error that only
 * appears in the P2 configuration nobody measures. */
volatile u32 gNdsR2CubicSaturations;

/* The overlay's sentinel and zero tests as bit compares (the objanim.c
 * import patch, NDS_OA_*). Same-ROM A/B word: 0 = the float compares. */
volatile u32 gNdsObjAnimBitCompare __attribute__((used, section(".data"))) = 1u;

#define gcAddDObjAnimJoint ndsBaseGcAddDObjAnimJoint
#define gcAddMObjMatAnimJoint ndsBaseGcAddMObjMatAnimJoint
#define gcAddMObjAll ndsBaseGcAddMObjAll
#define gcAddAnimJointAll ndsBaseGcAddAnimJointAll
#define gcAddMatAnimJointAll ndsBaseGcAddMatAnimJointAll
#define gcAddAnimAll ndsBaseGcAddAnimAll
#define gcAddCObjCamAnimJoint ndsBaseGcAddCObjCamAnimJoint
#define gcPlayMObjMatAnim ndsBaseGcPlayMObjMatAnim
#define gcParseMObjMatAnimJoint ndsBaseGcParseMObjMatAnimJoint
#define gcPlayAnimAll ndsBaseGcPlayAnimAll
#define gcSetupCustomDObjsWithMObj ndsBaseGcSetupCustomDObjsWithMObj
#if NDS_R2_ANIM_CENSUS || NDS_R2_CUBIC_FIXED
/* R2-03 E61/E64. Frees the name so the port-side player below is reached. Task
 * 95 proved this exact interposition end to end: the hot call is INTERNAL to
 * objanim.c (inside gcPlayAnimAll), so renaming the definition renames that
 * call with it and a port-side replacement actually runs. */
#define gcPlayDObjAnimJoint ndsBaseGcPlayDObjAnimJoint
#endif
#if NDS_R2_CUBIC_FIXED
/* 2026-10-05: the event32 parser and the value getter in Q form (the port
 * definitions follow the player below). */
#define gcParseDObjAnimJoint ndsBaseGcParseDObjAnimJoint
#define gcGetAObjValue ndsBaseGcGetAObjValue
#endif

/* Campaign 01 re-knapsack, 2026-08-17, gave ndsBaseGcPlayAnimAll 80 ITCM bytes
 * for 1,070 I-cache-fill tk/fr. P2-2p8 N04.08 removed its last call site --
 * gcPlayAnimAll now runs ndsGcPlayAnimAllStableSkip, which needs to reach
 * between the parser and the player -- and the linker does not drop it, so the
 * attribute is gone and the dead decomp body sits in main RAM. That is 80 bytes
 * back into an ITCM region with 16 free at the measured route build. The type
 * pull-in stays: the remaining attribute below still has to precede the decomp
 * body, same declaration mechanism as gcPlayDObjAnimJoint. */
#include <sys/obj.h>
extern s32 ndsRelocPointerRangeInLoadedFiles(const void *ptr, size_t size);
sb32 ndsTraIDescUsable(DObj *dobj, const AObj *aobj, u32 site);
void ndsBaseGcPlayAnimAll(GObj *gobj);
/* 732 bytes / 2,950 I-cache-fill tk/fr on the gate's rank-80 frames. It is also
 * the largest single soft-float caller in the build -- 633,842 helper calls over
 * those 80 frames, 7,923 per frame (SHIPPING_REBANK.md softfloat-callers) -- and
 * its ITCM-resident wrapper gcPlayMObjMatAnim already sits beside it. */
void ndsBaseGcPlayMObjMatAnim(MObj *mobj) __attribute__((section(".itcm")));

#include <battleship_overlay/src/sys/objanim.c>

#undef gcAddDObjAnimJoint
#undef gcAddMObjMatAnimJoint
#undef gcAddMObjAll
#undef gcAddAnimJointAll
#undef gcAddMatAnimJointAll
#undef gcAddAnimAll
#undef gcAddCObjCamAnimJoint
#undef gcPlayMObjMatAnim
#undef gcParseMObjMatAnimJoint
#undef gcPlayAnimAll
#undef gcSetupCustomDObjsWithMObj
#if NDS_R2_CUBIC_FIXED
#undef gcParseDObjAnimJoint
#undef gcGetAObjValue
#endif

/* P2-2p8 (2026-10-05): material animations at the presentation rate. An
 * MObj's animation is render state only: its tracks write the material's
 * colours, texture and palette indices and scroll, which the draw reads and
 * the replay digest does not fold. The source parses and plays every MObj on
 * every 60 Hz tick, but a presented frame shows only the state its last tick
 * left. The battle host publishes, per tick, how many ticks this one stands
 * for (src/port/taskman_seam_battle_host.c): 0 on a batch's earlier ticks --
 * the parser and player return without touching the MObj -- and on the drawn
 * tick the batch's tick count, which the parser and player run as one step of
 * that many times the MObj's speed. The parser subtracts `speed` from the wait
 * and seeds each new segment at `-wait - speed`, and the player adds `speed`
 * to each length, so one step of 2 * speed leaves every wait, length and
 * value where two steps of `speed` would (up to float rounding of the wait):
 * the drawn frame shows what it showed. An animation set on an earlier tick
 * starts one tick late. Outside a battle the word is 1 (the source rate).
 * Exceptions keep the source rate: the one-shot costume bake
 * (lbCommonAddMObjForFighterPartsDObj, src/port/reloc_backend_compat_shims.c),
 * which removes its AObjs right after its play, and Yoshi's Island's clouds,
 * whose MObj `anim_wait` gryoster.c reads to make cloud lines solid. */
volatile u32 gNdsMObjTickMul __attribute__((used, section(".data"))) = 1u;

/* speed * mul; exact for mul 2 on a normal speed (one exponent step). */
static inline f32 ndsMObjSpeedTimes(f32 speed, u32 mul)
{
    u32 bits = ndsFcmpBits(speed);
    const u32 exponent = (bits >> 23) & 0xffu;

    if ((bits << 1) == 0u)
    {
        return speed;
    }
    if ((mul == 2u) && (exponent != 0u) && (exponent < 0xfeu))
    {
        bits += 1u << 23;
        __builtin_memcpy(&speed, &bits, sizeof(speed));
        return speed;
    }
    return speed * (f32)mul;
}

/* One parse (or play) of `mul` ticks: the MObj's speed scaled around the
 * source body, then restored unless the body itself set a new speed. */
static void __attribute__((noinline)) ndsMObjStepScaled(MObj *mobj, u32 mul,
                                                        sb32 play)
{
    const f32 speed = mobj->anim_speed;
    const f32 scaled = ndsMObjSpeedTimes(speed, mul);

    mobj->anim_speed = scaled;
    if (play != FALSE)
    {
        ndsBaseGcPlayMObjMatAnim(mobj);
    }
    else
    {
        ndsBaseGcParseMObjMatAnimJoint(mobj);
    }
    if (ndsFcmpBits(mobj->anim_speed) == ndsFcmpBits(scaled))
    {
        mobj->anim_speed = speed;
    }
}

void gcParseMObjMatAnimJoint(MObj *mobj)
{
    const u32 mul = gNdsMObjTickMul;

    if (mul == 1u)
    {
        ndsBaseGcParseMObjMatAnimJoint(mobj);
    }
    else if ((mul != 0u) && (mobj != NULL))
    {
        ndsMObjStepScaled(mobj, mul, FALSE);
    }
}

/* The source-rate pair, for one-shot setups that must take effect now. */
void ndsGcParseMObjMatAnimJointNow(MObj *mobj)
{
    ndsBaseGcParseMObjMatAnimJoint(mobj);
}

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

/* `ndsR2S32ToF32Bits` / `ndsR2FixedToF32` moved to `nds_anim_fixed.h`
 * (P2-2p6) so the fighter pose engine shares them; exact, proven over all 2^32
 * inputs by `scripts/check_s32tof32_exact.py`. */

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

/* `NDS_R2_AQ_T_MAX`, `NDS_R2_AQ_LEN_MAX`, `ndsR2AnimClamp` and
 * `ndsR2AnimClamp64` moved to `nds_anim_fixed.h` (P2-2p6) with the field
 * kernel; the saturation reasoning is kept beside them there. */

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
    s32 t2;
    s32 t3;
    s32 omt2;
    s32 tmt;
    s32 h_vb;
    s32 h_vt;
    s32 h_rb;
    s32 h_rt;
    s64 acc;

    /* NDS_R2_AQ_OPAQUE (nds_anim_fixed.h): keeps each product a single
     * SMULL/SMLAL instead of the 64 x 64 sequence GCC emits once it has proved
     * the truncations below are no-ops. Values unchanged. */
    NDS_R2_AQ_OPAQUE(t);
    NDS_R2_AQ_OPAQUE(length_q);
    /* t is Q16 and normally in [0,1], so t*t reaches 2^32 and needs the 64-bit
     * product. Every requantising shift rounds rather than truncates. */
    t2 = (s32)((((s64)t * t) + (NDS_R2_CUBIC_BONE / 2)) >> NDS_R2_CUBIC_BF);
    NDS_R2_AQ_OPAQUE(t2);
    t3 = (s32)((((s64)t2 * t) + (NDS_R2_CUBIC_BONE / 2)) >> NDS_R2_CUBIC_BF);
    NDS_R2_AQ_OPAQUE(t3);
    /* (1-t)^2 == 1 - 2t + t^2 exactly, so reuse t2 instead of squaring (1-t).
     * Cheaper by a multiply AND a shift, and it deletes that rounding step
     * rather than merely rounding it -- which matters because near t=1 the
     * square is small and a truncated one loses most of its significance. */
    omt2 = (t2 - (2 * t)) + NDS_R2_CUBIC_BONE;
    tmt = t2 - t;
    h_vb = ((2 * t3) - (3 * t2)) + NDS_R2_CUBIC_BONE;
    h_vt = (3 * t2) - (2 * t3);
    NDS_R2_AQ_OPAQUE(omt2);
    NDS_R2_AQ_OPAQUE(tmt);
    /* These two carry a factor of `length`, which is unbounded, so they are the
     * only places that need a wider intermediate for range as well as rounding.
     * Q12 value x Q16 basis, shifted by VF, is Q16 again. SMULL/SMLAL make a
     * 32x32->64 on ARM9 a single instruction; it is the 64-bit ADDS/ADCS chains
     * that arm A paid for, and there are none left here. */
    h_rb = (s32)((((s64)length_q * omt2) + (1 << (NDS_R2_CUBIC_VF - 1))) >>
        NDS_R2_CUBIC_VF);
    h_rt = (s32)((((s64)length_q * tmt) +
        (1 << (NDS_R2_CUBIC_VF - 1))) >> NDS_R2_CUBIC_VF);
    NDS_R2_AQ_OPAQUE(h_vb);
    NDS_R2_AQ_OPAQUE(h_vt);
    NDS_R2_AQ_OPAQUE(h_rb);
    NDS_R2_AQ_OPAQUE(h_rt);
    /* Q12 x Q16 = Q28 in the accumulator, shifted back to Q12 at the end. */
    acc = (s64)ndsR2F32ToFixed(aobj->value_base, NDS_R2_CUBIC_VF) * h_vb;

    NDS_DIAG(gNdsR2CubicEvals++);
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
    /* P2-2p6: the arithmetic lives in `ndsR2AnimEvalQ` (nds_anim_fixed.h),
     * ONE body shared with the fighter pose engine. This wrapper only loads the
     * AObj's Q slots, counts the cubic evaluations and converts the Q12 result
     * to the f32 the DObj pose vectors still carry. The per-arm narrowing the
     * cycle-116 comment insisted on is preserved inside the kernel. */
    u32 kind = aobj->kind;
    s32 out = ndsR2AnimEvalQ(ndsR2AQLoad(aobj->length),
                             ndsR2AQLoad(aobj->length_invert),
                             ndsR2AQLoad(aobj->value_base),
                             ndsR2AQLoad(aobj->value_target),
                             ndsR2AQLoad(aobj->rate_base),
                             ndsR2AQLoad(aobj->rate_target), kind);

    if (kind == NDS_R2_AQ_KIND_CUBIC)
    {
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
 * run for every non-fighter DObj this player is called for.
 *
 * One body, two players. `tra_scale` is the per-joint translate scale of
 * lb/lbcommon.c:1261 `lbCommonPlayTranslateScaledDObjAnim`, whose ONLY
 * difference from sys/objanim.c:714 `gcPlayDObjAnimJoint` is `*= scale->x/y/z`
 * on the four translate tracks. `ftParamUpdateAnimKeys` (ft/ftparam.c:364)
 * selects it for every joint of a fighter whose attributes carry
 * `translate_scales` -- Luigi, who plays Mario's figatrees at his own limb
 * lengths. Until 2026-08-23 the port's copy was an empty stub, so Luigi's
 * joints were parsed (anim_wait counted down) and never played: no pose ever
 * moved, which the owner reported as "no animations". The unscaled wrapper
 * passes a constant NULL, so GCC folds the scale arm out of the ITCM-resident
 * player. `always_inline` is load-bearing: at -O2 GCC outlined the body as one
 * shared main-RAM copy and left the ITCM symbol a 10-byte tail call, which
 * would have evicted the player from ITCM without any other symptom. */
static inline __attribute__((always_inline)) void
ndsPlayDObjAnimJointBody(DObj *dobj, const Vec3f *tra_scale)
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
                        if (ndsTraIDescUsable(dobj, aobj, 1u) != FALSE)
                        {
                            syInterpCubic(&dobj->translate.vec.f,
                                          aobj->interpolate, value);
                        }
                        if (tra_scale != NULL)
                        {
                            dobj->translate.vec.f.x *= tra_scale->x;
                            dobj->translate.vec.f.y *= tra_scale->y;
                            dobj->translate.vec.f.z *= tra_scale->z;
                        }
                        break;

                    case nGCAnimTrackTraX:
                        dobj->translate.vec.f.x =
                            (tra_scale != NULL) ? value * tra_scale->x : value;
                        break;
                    case nGCAnimTrackTraY:
                        dobj->translate.vec.f.y =
                            (tra_scale != NULL) ? value * tra_scale->y : value;
                        break;
                    case nGCAnimTrackTraZ:
                        dobj->translate.vec.f.z =
                            (tra_scale != NULL) ? value * tra_scale->z : value;
                        break;
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

/* P2-2p8: out of ITCM into plain .text (not .text.hot, which is closed).
 * The fighter pose engine took its fighter joints; the four-CPU census ran
 * ~9 instructions a frame here against 612 B of ITCM, which the lean
 * kernel's DTCM sine lookup needed. */
void gcPlayDObjAnimJoint(DObj *dobj)
    __attribute__((section(".text.ndsColdPlayDObjAnimJoint")));
void gcPlayDObjAnimJoint(DObj *dobj)
{
    ndsPlayDObjAnimJointBody(dobj, NULL);
}

/* Main RAM, not ITCM: ITCM is full (census section D reads 168 B free) and
 * this player only runs for fighters that carry translate scales. */
void lbCommonPlayTranslateScaledDObjAnim(DObj *dobj, Vec3f *scale)
{
    ndsPlayDObjAnimJointBody(dobj, scale);
}

/* P2-2p8 (2026-10-05, owner: "Software floating point should not exist, fixed
 * point only"; ruling D13 re-baselines the digest): the stage and item joint
 * AObjs in Q form, as the fighter parser has written them since Requirement 4
 * (battleship_ftanim.c). The source's event32 parser below writes every
 * rotate, translate and scale track as a Q kind -- value and rate Q12, Linear's
 * rate Q16, length Q12, the cubic's reciprocal Q30 -- so the player's per-node
 * length add, its Linear/Step arms and the cubic run in integers instead of
 * converting six floats a node (Sector Z's census: the player and this parser
 * were 25K soft-float cycles a frame). The DObj's own clock (anim_wait,
 * anim_frame, anim_speed) stays f32: stage code reads it. The path-parameter
 * track TraI stays float too -- Q12 of a path's [0, 1] would step Sector Z's
 * Arwing up to a couple of units along its loop. Readers of a Q node go
 * through the player or gcGetAObjValue (below), which dispatch on `kind`. */

/* One AObj into Q form before this parser writes a Q kind on it (the fighter
 * parser's ndsR2AnimAObjToQConvert): the arms carry fields forward, so a node
 * written half in each form would read an f32 bit pattern as Q. */
static void __attribute__((noinline)) ndsOAObjToQConvert(AObj *a, s32 kind)
{
    if (kind == nGCAnimKindNone)
    {
        /* gcAddAObjForDObj's zeros are the same word in both forms; its
         * length_invert of 1.0F is not, and a zero-payload event keeps it. */
        a->length_invert = ndsR2AQStore(1 << NDS_R2_AQ_IF);
        return;
    }
    if (kind > nGCAnimKindCubic)
    {
        return;
    }
    a->length_invert = ndsR2AQStore(ndsR2F32ToFixed(a->length_invert,
        (kind == nGCAnimKindStep) ? NDS_R2_AQ_LF : NDS_R2_AQ_IF));
    a->length = ndsR2AQStore(ndsR2F32ToFixed(a->length, NDS_R2_AQ_LF));
    a->value_base = ndsR2AQStore(ndsR2F32ToFixed(a->value_base, NDS_R2_AQ_VF));
    a->value_target =
        ndsR2AQStore(ndsR2F32ToFixed(a->value_target, NDS_R2_AQ_VF));
    a->rate_base = ndsR2AQStore(ndsR2F32ToFixed(a->rate_base,
        (kind == nGCAnimKindLinear) ? NDS_R2_AQ_RF : NDS_R2_AQ_VF));
    a->rate_target =
        ndsR2AQStore(ndsR2F32ToFixed(a->rate_target, NDS_R2_AQ_VF));
    a->kind = (u8)(kind + ((s32)NDS_R2_AQ_KIND_BASE - nGCAnimKindStep));
}

/* TRUE when this parser writes `track` in Q form. */
static inline u32 ndsOATrackQ(s32 track)
{
    return (track != nGCAnimTrackTraI) ? 1u : 0u;
}

/* The node for joint track `i`, created on first use (the source's), and in
 * the form `tq` asks for. */
static inline AObj *ndsOATrack(DObj *dobj, AObj **track_aobjs, s32 i, u32 tq)
{
    AObj *a = track_aobjs[i];

    if (a == NULL)
    {
        a = gcAddAObjForDObj(dobj, i + nGCAnimTrackJointStart);
        track_aobjs[i] = a;
    }
    if ((tq != 0u) && ((s32)a->kind < (s32)NDS_R2_AQ_KIND_BASE))
    {
        ndsOAObjToQConvert(a, (s32)a->kind);
    }
    return a;
}

/* 1/n as the Q30 reciprocal, rounded to nearest (n is a 15-bit payload). */
static inline f32 ndsOARecipQ(u32 n)
{
    return ndsR2AQStore((s32)(((1u << NDS_R2_AQ_IF) + (n >> 1)) / n));
}

/* `-anim_wait - anim_speed`, a new segment's starting length, in Q12. */
static inline s32 ndsOASegmentStartQ(const DObj *dobj)
{
    return -(ndsR2F32ToFixed(dobj->anim_wait, NDS_R2_AQ_LF) +
             ndsR2F32ToFixed(dobj->anim_speed, NDS_R2_AQ_LF));
}

/* Every live node's length advanced by `speed + wait` (the two exits that
 * leave the script), in each node's own form. */
static void ndsOAAdvanceTail(DObj *dobj)
{
    const f32 tail = dobj->anim_speed + dobj->anim_wait;
    const s32 tail_q = ndsR2F32ToFixed(dobj->anim_speed, NDS_R2_AQ_LF) +
                       ndsR2F32ToFixed(dobj->anim_wait, NDS_R2_AQ_LF);
    AObj *a;

    for (a = dobj->aobj; a != NULL; a = a->next)
    {
        if (a->kind == nGCAnimKindNone)
        {
            continue;
        }
        if (a->kind >= NDS_R2_AQ_KIND_BASE)
        {
            a->length = ndsR2AQStore(ndsR2AQLoad(a->length) + tail_q);
        }
        else
        {
            a->length += tail;
        }
    }
}

/* sys/objanim.c:268, the event32 parser, writing Q kinds (see above). Its
 * control flow, callbacks and DObj clock are the source's line for line. */
void gcParseDObjAnimJoint(DObj *dobj)
{
    AObj *track_aobjs[nGCAnimTrackJointEnd - nGCAnimTrackJointStart + 1];
    AObj *aobj;
    s32 i;
    u32 command_kind;
    u32 flags;
    u32 payload_u;
    f32 payload;

    if (NDS_FCMP_EQ_C(dobj->anim_wait, AOBJ_ANIM_NULL))
    {
        return;
    }
    if (NDS_FCMP_EQ_C(dobj->anim_wait, AOBJ_ANIM_CHANGED))
    {
        dobj->anim_wait = -dobj->anim_frame;
    }
    else
    {
        dobj->anim_wait -= dobj->anim_speed;
        dobj->anim_frame += dobj->anim_speed;
        dobj->parent_gobj->anim_frame = dobj->anim_frame;

        if (NDS_FCMP_GT0(dobj->anim_wait))
        {
            return;
        }
    }
    for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs); i++)
    {
        track_aobjs[i] = NULL;
    }
    for (aobj = dobj->aobj; aobj != NULL; aobj = aobj->next)
    {
        if ((aobj->track >= nGCAnimTrackJointStart) &&
            (aobj->track <= nGCAnimTrackJointEnd))
        {
            track_aobjs[aobj->track - nGCAnimTrackJointStart] = aobj;
        }
    }
    do
    {
        if (dobj->anim_joint.event32 == NULL)
        {
            ndsOAAdvanceTail(dobj);
            dobj->anim_frame = dobj->anim_wait;
            dobj->parent_gobj->anim_frame = dobj->anim_wait;
            dobj->anim_wait = AOBJ_ANIM_END;
            return;
        }
        command_kind = dobj->anim_joint.event32->command.opcode;

        switch (command_kind)
        {
        case nGCAnimEvent32SetVal0RateBlock:
        case nGCAnimEvent32SetVal0Rate:
        case nGCAnimEvent32SetValRateBlock:
        case nGCAnimEvent32SetValRate:
        {
            /* Cubic: the target value, then (SetValRate) the target rate. */
            const u32 with_rate =
                ((command_kind == nGCAnimEvent32SetValRateBlock) ||
                 (command_kind == nGCAnimEvent32SetValRate)) ? 1u : 0u;
            const s32 len_q = ndsOASegmentStartQ(dobj);

            payload_u = dobj->anim_joint.event32->command.payload;
            payload = (f32)payload_u;
            flags = AObjAnimAdvance(dobj->anim_joint.event32)->command.flags;

            for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                 i++, flags = flags >> 1)
            {
                AObj *a;
                u32 tq;

                if (!(flags))
                {
                    break;
                }
                if (!(flags & 1))
                {
                    continue;
                }
                tq = ndsOATrackQ(i + nGCAnimTrackJointStart);
                a = ndsOATrack(dobj, track_aobjs, i, tq);
                a->value_base = a->value_target;
                a->rate_base = a->rate_target;
                if (tq != 0u)
                {
                    a->value_target = ndsR2AQStore(ndsR2F32ToFixed(
                        dobj->anim_joint.event32->f, NDS_R2_AQ_VF));
                    AObjAnimAdvance(dobj->anim_joint.event32);
                    a->rate_target = (with_rate != 0u) ?
                        ndsR2AQStore(ndsR2F32ToFixed(
                            dobj->anim_joint.event32->f, NDS_R2_AQ_VF)) :
                        ndsR2AQStore(0);
                    a->kind = NDS_R2_AQ_KIND_CUBIC;
                    if (payload_u != 0u)
                    {
                        a->length_invert = ndsOARecipQ(payload_u);
                    }
                    a->length = ndsR2AQStore(len_q);
                }
                else
                {
                    a->value_target = dobj->anim_joint.event32->f;
                    AObjAnimAdvance(dobj->anim_joint.event32);
                    a->rate_target = (with_rate != 0u) ?
                        dobj->anim_joint.event32->f : 0.0F;
                    a->kind = nGCAnimKindCubic;
                    if (payload_u != 0u)
                    {
                        a->length_invert = 1.0F / payload;
                    }
                    a->length = -dobj->anim_wait - dobj->anim_speed;
                }
                if (with_rate != 0u)
                {
                    AObjAnimAdvance(dobj->anim_joint.event32);
                }
            }
            if ((command_kind == nGCAnimEvent32SetVal0RateBlock) ||
                (command_kind == nGCAnimEvent32SetValRateBlock))
            {
                dobj->anim_wait += payload;
            }
            break;
        }

        case nGCAnimEvent32SetValBlock:
        case nGCAnimEvent32SetVal:
        {
            const s32 len_q = ndsOASegmentStartQ(dobj);

            payload_u = dobj->anim_joint.event32->command.payload;
            payload = (f32)payload_u;
            flags = AObjAnimAdvance(dobj->anim_joint.event32)->command.flags;

            for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                 i++, flags = flags >> 1)
            {
                AObj *a;
                u32 tq;

                if (!(flags))
                {
                    break;
                }
                if (!(flags & 1))
                {
                    continue;
                }
                tq = ndsOATrackQ(i + nGCAnimTrackJointStart);
                a = ndsOATrack(dobj, track_aobjs, i, tq);
                a->value_base = a->value_target;
                if (tq != 0u)
                {
                    const u32 was_linear =
                        (a->kind == NDS_R2_AQ_KIND_LINEAR) ? 1u : 0u;

                    a->value_target = ndsR2AQStore(ndsR2F32ToFixed(
                        dobj->anim_joint.event32->f, NDS_R2_AQ_VF));
                    AObjAnimAdvance(dobj->anim_joint.event32);
                    a->kind = NDS_R2_AQ_KIND_LINEAR;
                    if (payload_u != 0u)
                    {
                        /* (target - base) / payload, Q16, the magnitude
                         * rounded to nearest (the fighter parser's). */
                        s32 d = (ndsR2AQLoad(a->value_target) -
                                 ndsR2AQLoad(a->value_base))
                                    << (NDS_R2_AQ_RF - NDS_R2_AQ_VF);
                        u32 h = payload_u >> 1;
                        s32 r = (d < 0) ?
                            -(s32)(((u32)(-d) + h) / payload_u) :
                            (s32)(((u32)d + h) / payload_u);

                        a->rate_base = ndsR2AQStore(r);
                    }
                    else if (was_linear == 0u)
                    {
                        /* The source keeps the old rate as the line's, and
                         * a Linear node holds its rate at Q16. */
                        a->rate_base = ndsR2AQStore(ndsR2AQLoad(a->rate_base)
                            << (NDS_R2_AQ_RF - NDS_R2_AQ_VF));
                    }
                    a->length = ndsR2AQStore(len_q);
                    a->rate_target = ndsR2AQStore(0);
                }
                else
                {
                    a->value_target = dobj->anim_joint.event32->f;
                    AObjAnimAdvance(dobj->anim_joint.event32);
                    a->kind = nGCAnimKindLinear;
                    if (payload_u != 0u)
                    {
                        a->rate_base =
                            (a->value_target - a->value_base) / payload;
                    }
                    a->length = -dobj->anim_wait - dobj->anim_speed;
                    a->rate_target = 0.0F;
                }
            }
            if (command_kind == nGCAnimEvent32SetValBlock)
            {
                dobj->anim_wait += payload;
            }
            break;
        }

        case nGCAnimEvent32SetTargetRate:
            flags = AObjAnimAdvance(dobj->anim_joint.event32)->command.flags;

            for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                 i++, flags = flags >> 1)
            {
                AObj *a;
                u32 tq;

                if (!(flags))
                {
                    break;
                }
                if (!(flags & 1))
                {
                    continue;
                }
                tq = ndsOATrackQ(i + nGCAnimTrackJointStart);
                a = ndsOATrack(dobj, track_aobjs, i, tq);
                a->rate_target = (tq != 0u) ?
                    ndsR2AQStore(ndsR2F32ToFixed(dobj->anim_joint.event32->f,
                                                 NDS_R2_AQ_VF)) :
                    dobj->anim_joint.event32->f;
                AObjAnimAdvance(dobj->anim_joint.event32);
            }
            break;

        case nGCAnimEvent32Wait:
            dobj->anim_wait += (f32)AObjAnimAdvance(
                dobj->anim_joint.event32)->command.payload;
            break;

        case nGCAnimEvent32SetValAfterBlock:
        case nGCAnimEvent32SetValAfter:
        {
            const s32 len_q = ndsOASegmentStartQ(dobj);

            payload_u = dobj->anim_joint.event32->command.payload;
            payload = (f32)payload_u;
            flags = AObjAnimAdvance(dobj->anim_joint.event32)->command.flags;

            for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                 i++, flags = flags >> 1)
            {
                AObj *a;
                u32 tq;

                if (!(flags))
                {
                    break;
                }
                if (!(flags & 1))
                {
                    continue;
                }
                tq = ndsOATrackQ(i + nGCAnimTrackJointStart);
                a = ndsOATrack(dobj, track_aobjs, i, tq);
                a->value_base = a->value_target;
                if (tq != 0u)
                {
                    a->value_target = ndsR2AQStore(ndsR2F32ToFixed(
                        dobj->anim_joint.event32->f, NDS_R2_AQ_VF));
                    AObjAnimAdvance(dobj->anim_joint.event32);
                    a->kind = NDS_R2_AQ_KIND_STEP;
                    /* Step's length_invert is a frame count (Q12). */
                    a->length_invert =
                        ndsR2AQStore((s32)payload_u << NDS_R2_AQ_LF);
                    a->length = ndsR2AQStore(len_q);
                    a->rate_target = ndsR2AQStore(0);
                }
                else
                {
                    a->value_target = dobj->anim_joint.event32->f;
                    AObjAnimAdvance(dobj->anim_joint.event32);
                    a->kind = nGCAnimKindStep;
                    a->length_invert = payload;
                    a->length = -dobj->anim_wait - dobj->anim_speed;
                    a->rate_target = 0.0F;
                }
            }
            if (command_kind == nGCAnimEvent32SetValAfterBlock)
            {
                dobj->anim_wait += payload;
            }
            break;
        }

        case nGCAnimEvent32SetAnim:
            AObjAnimAdvance(dobj->anim_joint.event32);
            dobj->anim_joint.event32 = dobj->anim_joint.event32->p;
            dobj->anim_frame = -dobj->anim_wait;
            dobj->parent_gobj->anim_frame = -dobj->anim_wait;

            if ((dobj->is_anim_root != FALSE) &&
                (dobj->parent_gobj->func_anim != NULL))
            {
                dobj->parent_gobj->func_anim(dobj, -2, 0);
            }
            break;

        case nGCAnimEvent32Jump:
            AObjAnimAdvance(dobj->anim_joint.event32);
            dobj->anim_joint.event32 = dobj->anim_joint.event32->p;

            if ((dobj->is_anim_root != FALSE) &&
                (dobj->parent_gobj->func_anim != NULL))
            {
                dobj->parent_gobj->func_anim(dobj, -2, 0);
            }
            break;

        case ANIM_CMD_12:
            /* Every flagged node's length advanced by the payload. */
            payload_u = dobj->anim_joint.event32->command.payload;
            flags = AObjAnimAdvance(dobj->anim_joint.event32)->command.flags;

            for (i = 0; i < (s32)ARRAY_COUNT(track_aobjs);
                 i++, flags = flags >> 1)
            {
                AObj *a;

                if (!(flags))
                {
                    break;
                }
                if (!(flags & 1))
                {
                    continue;
                }
                a = ndsOATrack(dobj, track_aobjs, i, 0u);
                if (a->kind >= NDS_R2_AQ_KIND_BASE)
                {
                    a->length = ndsR2AQStore(ndsR2AQLoad(a->length) +
                        ((s32)payload_u << NDS_R2_AQ_LF));
                }
                else
                {
                    a->length += (f32)payload_u;
                }
            }
            break;

        case nGCAnimEvent32SetInterp:
            AObjAnimAdvance(dobj->anim_joint.event32);

            if (track_aobjs[nGCAnimTrackTraI - nGCAnimTrackJointStart] == NULL)
            {
                track_aobjs[nGCAnimTrackTraI - nGCAnimTrackJointStart] =
                    gcAddAObjForDObj(dobj, nGCAnimTrackTraI);
            }
            track_aobjs[nGCAnimTrackTraI - nGCAnimTrackJointStart]->interpolate =
                dobj->anim_joint.event32->p;

            AObjAnimAdvance(dobj->anim_joint.event32);
            break;

        case nGCAnimEvent32End:
            ndsOAAdvanceTail(dobj);
            dobj->anim_frame = dobj->anim_wait;
            dobj->parent_gobj->anim_frame = dobj->anim_wait;
            dobj->anim_wait = AOBJ_ANIM_END;

            if ((dobj->is_anim_root != FALSE) &&
                (dobj->parent_gobj->func_anim != NULL))
            {
                dobj->parent_gobj->func_anim(dobj, -1, 0);
            }
            return;

        case nGCAnimEvent32SetFlags:
            dobj->flags = dobj->anim_joint.event32->command.flags;
            dobj->anim_wait += (f32)AObjAnimAdvance(
                dobj->anim_joint.event32)->command.payload;
            break;

        case ANIM_CMD_16:
            if (dobj->parent_gobj->func_anim != NULL)
            {
                dobj->parent_gobj->func_anim(
                    dobj, dobj->anim_joint.event32->command.flags >> 8,
                    (u8)dobj->anim_joint.event32->command.flags);
            }
            dobj->anim_wait += (f32)AObjAnimAdvance(
                dobj->anim_joint.event32)->command.payload;
            break;

        case ANIM_CMD_17:
            flags = dobj->anim_joint.event32->command.flags;
            dobj->anim_wait += (f32)AObjAnimAdvance(
                dobj->anim_joint.event32)->command.payload;

            for (i = 4; i < 14; i++, flags = flags >> 1)
            {
                if (!(flags))
                {
                    break;
                }
                if (flags & 1)
                {
                    if (dobj->parent_gobj->func_anim != NULL)
                    {
                        dobj->parent_gobj->func_anim(
                            dobj, i, dobj->anim_joint.event32->f);
                    }
                    AObjAnimAdvance(dobj->anim_joint.event32);
                }
            }
            break;

        default:
            break;
        }
    } while (!NDS_FCMP_GT0(dobj->anim_wait));
}

/* sys/objanim.c:672 for either form: grsector.c reads the Arwing's TraI value
 * through it, and the scaled-translate player once did. */
f32 gcGetAObjValue(AObj *aobj)
{
    if (aobj->kind >= NDS_R2_AQ_KIND_BASE)
    {
        return ndsR2AnimValueQ(aobj);
    }
    return ndsBaseGcGetAObjValue(aobj);
}
#else
/* lb/lbcommon.c:1261, verbatim, for the NDS_R2_LAB_CUBIC_OFF escape hatch where
 * the decomp player above is the one in use. */
void lbCommonPlayTranslateScaledDObjAnim(DObj *dobj, Vec3f *scale)
{
    f32 interp;

    if (dobj->anim_wait != AOBJ_ANIM_NULL)
    {
        AObj *aobj = dobj->aobj;

        while (aobj != NULL)
        {
            if (aobj->kind != nGCAnimKindNone)
            {
                if (dobj->anim_wait != AOBJ_ANIM_END)
                {
                    aobj->length += dobj->anim_speed;
                }
                if (!(dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM))
                {
                    switch (aobj->track)
                    {
                    case nGCAnimTrackRotX:
                        dobj->rotate.vec.f.x = gcGetAObjValue(aobj);
                        break;
                    case nGCAnimTrackRotY:
                        dobj->rotate.vec.f.y = gcGetAObjValue(aobj);
                        break;
                    case nGCAnimTrackRotZ:
                        dobj->rotate.vec.f.z = gcGetAObjValue(aobj);
                        break;
                    case nGCAnimTrackTraI:
                        interp = gcGetAObjValue(aobj);
                        if (interp < 0.0F)
                        {
                            interp = 0.0F;
                        }
                        else if (interp > 1.0F)
                        {
                            interp = 1.0F;
                        }
                        if (ndsTraIDescUsable(dobj, aobj, 2u) != FALSE)
                        {
                            syInterpCubic(&dobj->translate.vec.f,
                                          aobj->interpolate, interp);
                        }
                        dobj->translate.vec.f.x *= scale->x;
                        dobj->translate.vec.f.y *= scale->y;
                        dobj->translate.vec.f.z *= scale->z;
                        break;
                    case nGCAnimTrackTraX:
                        dobj->translate.vec.f.x = gcGetAObjValue(aobj) * scale->x;
                        break;
                    case nGCAnimTrackTraY:
                        dobj->translate.vec.f.y = gcGetAObjValue(aobj) * scale->y;
                        break;
                    case nGCAnimTrackTraZ:
                        dobj->translate.vec.f.z = gcGetAObjValue(aobj) * scale->z;
                        break;
                    case nGCAnimTrackScaX:
                        dobj->scale.vec.f.x = gcGetAObjValue(aobj);
                        break;
                    case nGCAnimTrackScaY:
                        dobj->scale.vec.f.y = gcGetAObjValue(aobj);
                        break;
                    case nGCAnimTrackScaZ:
                        dobj->scale.vec.f.z = gcGetAObjValue(aobj);
                        break;
                    }
                }
            }
            aobj = aobj->next;
        }
        if (dobj->anim_wait == AOBJ_ANIM_END)
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

void gcPlayDObjAnimJoint(DObj *dobj) __attribute__((section(".itcm")));
void gcPlayDObjAnimJoint(DObj *dobj)
{
    const AObj *aobj;
    u32 chain = 0u;
    f32 speed;
    u32 speed_bits;
    u32 noanim;

    NDS_DIAG(gNdsR2AnimCensusCalls++);
    if (dobj->anim_wait != AOBJ_ANIM_NULL)
    {
        speed = dobj->anim_speed;
        __builtin_memcpy(&speed_bits, &speed, sizeof(speed_bits));
        if (speed_bits == 0x3f800000u)
        {
            NDS_DIAG(gNdsR2AnimCensusSpeedOne++);
        }
        else if ((speed_bits & 0x7fffffffu) == 0u)
        {
            NDS_DIAG(gNdsR2AnimCensusSpeedZero++);
        }
        else
        {
            NDS_DIAG(gNdsR2AnimCensusSpeedOther++);
            gNdsR2AnimCensusSpeedOtherBits = speed_bits;
        }
        if (dobj->anim_wait == AOBJ_ANIM_END)
        {
            NDS_DIAG(gNdsR2AnimCensusAnimEnd++);
        }
        noanim = ((dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM) != 0) ? 1u : 0u;
        for (aobj = dobj->aobj; aobj != NULL; aobj = aobj->next)
        {
            chain++;
            if (aobj->kind == nGCAnimKindNone)
            {
                NDS_DIAG(gNdsR2AnimCensusKindNone++);
                continue;
            }
            NDS_DIAG(gNdsR2AnimCensusNodes++);
            if (noanim != 0u)
            {
                NDS_DIAG(gNdsR2AnimCensusNoAnimSkips++);
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
 * record. Evicting an entry while its bytes are still live, or overflowing and
 * re-normalizing later, applies the permutation TWICE to a live script, which
 * is unrecoverable corruption -- so there is no LRU or speculative reclaim.
 *
 * Fighter figatrees have a narrower SOURCE lifetime than a scene, though.
 * ftMainSetStatus calls lbRelocGetForceExternHeapFile for every ordinary action
 * and overwrites that fighter's SAME `figatree_heap` in place (decomp
 * ftmain.c:4621-4624, lbreloc.c:363-369). P2-2's four-CPU mirror stress exposed
 * the old assumption: by 00:55 the ledger had 7,467 reason-3 rejects (same
 * command address, different word), then a stale translate-interp binding
 * reached syInterpGetFracFrame as 0x38000000 and data-aborted. The reloc owner
 * therefore calls `ndsAObjEvent32ForgetRange` immediately BEFORE it overwrites
 * that source-owned heap. Other fighters/files remain indexed; scene teardown
 * still uses `ndsAObjEvent32ResetNormalizedScripts` for the full reset.
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
 * rather than from this comment.
 *
 * P2-2 DID re-derive it. The source-correct four-CPU M/F/M/F one-minute stress
 * match reaches all 2,048 slots and records 8 reason-12 normalization failures.
 * Reason 12 is a DS-only capacity failure: BattleShip has no policy that drops
 * an animation attach after an arbitrary number of previously-normalized event
 * words. The wrapper therefore cancels source behavior when this table fills.
 *
 * Raise the ledger by 1,024 entries rather than weakening the atomic attach or
 * evicting live pointer keys. Each entry was then {pointer,native_word} = 8 B
 * (since 2026-09-07 a pointer plus a one-byte signature), so this cost 8,192 B. The P2-2 wallpaper-row reclamation recovered 131,552 B
 * of .main.bss first; the same four-CPU run measured 40,400 B general-heap
 * low-water against the 25,600 B hard floor, leaving 6,608 B even under the
 * conservative 1:1 static-RAM exchange. The existing 4,096-slot hash remains
 * strictly larger than the ledger and needs no extra RAM. The standing stress
 * arm is the proof that 3,072 covers the finite four-fighter corpus with zero
 * NormalizeFailCount before this row may close.
 *
 * P2-4 re-derived it again (2026-09-07, admission-zebes-l1): Planet Zebes
 * enters the battle with 699 entries carried from the shell and its own
 * layer-1 material animations -- eleven 484-word palette scripts
 * (105_StageZebesFile2.c Layer1MatAnim) plus the acid -- ledger ~2,372 more,
 * so the table stood at 3,071 of 3,072 thirty presents in, with 27 reason-12
 * refusals by then and 46 by present 60. Every refused attach left its GObj
 * without the animation the source gave it, and one of them (Mario's entry
 * pipe effect, efManagerMakeEffect) reached gcParseDObjAnimJoint through a
 * joint whose event32 pointer was never written: data abort at objanim.c:366,
 * the owner's "Zebes crashes". 4,096 entries (+8,192 B) plus the 8,192-slot
 * index below (+8,192 B) hold Zebes' measured 3,071 with 1,025 spare for the
 * match corpus; gNdsAObjEvent32NormalizedHighWater on a full Zebes match is
 * the number that right-sizes this next. The +16,384 B is NOT paid for by
 * the P2-2 margin above (6,608 B under the 1:1 model): the four-CPU stress
 * heap floor is already the open P2-2 row, and this crash outranks it
 * (docs/BUGS.md, owner order 2026-09-07). Bake-time pre-normalization of the
 * O2R scripts would retire the ledger and its 49,152 B outright
 * (builds/resume-20260905/agents-0906/event32_prenormalize.final.md). */
/* 4,096 read a high-water of 4,035 on the very next Zebes probe (zebes-z1,
 * shell path plus the stage), 61 entries from the cliff; 5,120 keeps the
 * 8,192-slot index and a real margin. At 5 B per entry (pointer plus
 * signature, see sNdsAObjEvent32NormalizedSig) the ledger is 25,600 B plus
 * the 16,384 B index. */
#define NDS_AOBJ_EVENT32_NORMALIZED_MAX 5120u
#define NDS_AOBJ_EVENT32_BONUS2_FOX_LIMIT 7168u
/* ONE SCRIPT'S COMMAND PLAN LIVES IN THE LEDGER'S OWN FREE TAIL (2026-10-02).
 *
 * The plan was a static array: 128 entries, then 640 after Congo Jungle's
 * layer-1 platform script (509 commands) refused with reason 11 and silently
 * declined gcAddAnimAll for the whole layer (2026-09-06). Fox's Board the
 * Platforms hit the same cliff: its layer-1 MatAnimJoint table holds four
 * texture-cycle scripts of 1,326 commands each (138_GRBonus2FoxFile2.c), the
 * fourth-of-a-table refusal declined the DObj joints with it, and the platform
 * the 1P player spawns over (DObj 11, whose script places it on its path at
 * frame 0) stayed at its DObjDesc pose, so Fox fell to a FAILURE 0.9 s after
 * GO with no input. A plan entry is a command pointer; the source word is
 * still in place until commit and the native word is computed from it. So the
 * plan is written straight into sNdsAObjEvent32Normalized[Count..Count+Plan),
 * above the committed entries and outside the hash index, and committing an
 * entry is indexing the slot it already occupies. Any script that fits the
 * ledger now plans, and the 7,680 B of static RAM the array took is free. */
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
} NDSAObjEvent32Normalized;

/* One PlanStream frame: the straight run of commands it appended between its
 * entry and its branch. Within a frame the walk only moves forward, so a
 * command can repeat an earlier plan entry only inside an earlier frame's
 * [first, last]; FindPlanned scans just that frame instead of the plan. */
typedef struct NDSAObjEvent32PlanSegment
{
    AObjEvent32 *first;
    AObjEvent32 *last;
    u32 plan_start;
    u32 plan_end;
} NDSAObjEvent32PlanSegment;

/* The source command corpus is stage-dependent, but the old fixed BSS paid the
 * Planet Zebes worst case in every match. Keep 5120 as the hard/default ceiling
 * and allocate only the selected stage's proven bound from the scene taskman
 * heap. The allocation dies with that heap; ResetNormalizedScripts therefore
 * discards these pointers instead of touching storage after a scene rewind. */
static NDSAObjEvent32Normalized *sNdsAObjEvent32Normalized;
/* One byte of the committed native word per entry. The word itself is
 * committed in place (Plan commit below), so the ledger only has to answer
 * "was this pointer normalized" and re-check that the word it committed is
 * still there. The byte is a multiplicative hash, NOT an XOR fold: source
 * and native layouts are bit permutations of the same 32 bits, so an XOR
 * fold agreed for every flags-only End word (a one-byte rotation) and missed
 * one random word in 128 (review, 2026-09-07); the multiply mixes across
 * byte lanes and misses about one in 256. That is what the reason-3 witness
 * needs, at 5,120 B where the full word cost 20,480 B (2026-09-07 shell-loop
 * floor: 19,220 B free against the 32,768 B minimum after the 5,120-entry
 * raise). */
static u8 *sNdsAObjEvent32NormalizedSig;
static NDSAObjEvent32PlanSegment
    sNdsAObjEvent32PlanSegments[NDS_AOBJ_EVENT32_BRANCH_DEPTH_MAX + 1u];
static u32 sNdsAObjEvent32PlanSegmentCount;

static inline u8 ndsAObjEvent32WordSig(u32 word)
{
    return (u8)((word * 0x9E3779B1u) >> 24);
}
static u32 sNdsAObjEvent32NormalizedCount;
static u32 sNdsAObjEvent32PlanCount;

/* Plan entry i (see the plan note above NDS_AOBJ_EVENT32_BRANCH_DEPTH_MAX). */
static inline AObjEvent32 *ndsAObjEvent32PlanCommand(u32 i)
{
    return sNdsAObjEvent32Normalized[sNdsAObjEvent32NormalizedCount + i]
        .command;
}

/* objdef.h:272-281 source word to the ARM GCC bitfield layout. */
static inline u32 ndsAObjEvent32NativeWord(u32 source_word)
{
    return ((source_word >> 25) & 0x7fu) |
           (((source_word >> 15) & 0x03ffu) << 7) |
           ((source_word & 0x7fffu) << 17);
}
/* 32 - log2(hash slots): ndsAObjEvent32HashSlot's multiplicative form. */
static u32 sNdsAObjEvent32NormalizedHashShift = 32u;

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
 * Slots hold index+1 so that freshly allocated, zeroed hash storage reads as
 * EMPTY. That is not a style choice: with a 0xffff sentinel an uninitialized
 * table would look occupied and a probe could never terminate -- 3.11's freeze
 * class, in the one subsystem whose failure mode is already a freeze. */
#define NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS 8192u

_Static_assert((NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS &
                (NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS - 1u)) == 0u,
               "AObj event-32 ledger index must be a power of two");
_Static_assert(NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS >
               NDS_AOBJ_EVENT32_NORMALIZED_MAX,
               "AObj event-32 ledger index must keep a free slot per entry");
_Static_assert(NDS_AOBJ_EVENT32_NORMALIZED_MAX < 0xffffu,
               "AObj event-32 ledger index stores index+1 in a u16");

static u16
    *sNdsAObjEvent32NormalizedHash;
static u32 sNdsAObjEvent32NormalizedLimit;
static u32 sNdsAObjEvent32NormalizedHashSlots;
static u32 sNdsEvent32InterpDescFixedCount;

/* A RANGE FORGET THAT LEAVES HOLES (P2-2p8, 2026-09-30).
 *
 * ForgetRange compacted the ledger in order and rebuilt the whole index
 * (8,192 slots cleared, every entry re-hashed) whenever it removed anything:
 * ~150-200K ticks for a retired file's few hundred commands, about four times
 * a match, landing on whichever load reuses the heap (a 1.5M Dream Land frame,
 * Samus's second entry clip). Removing entries one by one with linear
 * probing's backward-shift deletion was slower still (the index is dense
 * around a file's addresses). Now a removed entry leaves a HOLE: its ledger
 * command becomes NULL and its index slot a TOMBSTONE that lookups probe past
 * and inserts reuse. Every live entry keeps its index and its slot, so every
 * lookup returns what it returned before. Holes are compacted out (with the
 * one rebuild) only when an append would not fit, and tombstones are
 * rebuilt away once they fill a quarter of the index. Same-ROM A/B word
 * gNdsAObjEvent32ForgetHoles (0 = compact and rebuild at every forget). */
#define NDS_AOBJ_EVENT32_TOMB 0xffffu
__attribute__((used, section(".data"))) volatile u32 gNdsAObjEvent32ForgetHoles = 1u;
__attribute__((used)) volatile u32 gNdsAObjEvent32ForgetHoleRemovals;
__attribute__((used)) volatile u32 gNdsAObjEvent32ForgetHoleRebuilds;
__attribute__((used)) volatile u32 gNdsAObjEvent32LedgerCompactions;
static u32 sNdsAObjEvent32Holes;
static u32 sNdsAObjEvent32Tombs;

/* Ledger entries per 4 KiB page of main RAM. ForgetRange runs at every fighter
 * motion load over a range (the figatree heap) whose AObj16 commands never
 * enter this ledger; with these counts it skips its whole-ledger scan when no
 * page of the range holds an entry. An entry outside main RAM counts in
 * Outside and disables the skip. */
#define NDS_AOBJ_EVENT32_PAGE_SHIFT 12u
#define NDS_AOBJ_EVENT32_RAM_BASE 0x02000000u
#define NDS_AOBJ_EVENT32_RAM_END 0x02400000u
#define NDS_AOBJ_EVENT32_PAGES     ((NDS_AOBJ_EVENT32_RAM_END - NDS_AOBJ_EVENT32_RAM_BASE) >>      NDS_AOBJ_EVENT32_PAGE_SHIFT)
static u16 sNdsAObjEvent32PageEntries[NDS_AOBJ_EVENT32_PAGES];
/* The 16-byte blocks of each page that can hold an entry: [lo, hi] is set by
 * the first entry after the count reaches zero and widens with each add, so it
 * bounds every live entry; a removal leaves it wide until a ForgetRange scan
 * re-derives it for the range's two edge pages. A figatree buffer starts and
 * ends inside pages shared with the files around it, so the page count alone
 * refused most skips (237 of 722 calls scanned the 1,508-entry ledger, 6 KB
 * through a 4 KB D-cache, to remove nothing). */
#define NDS_AOBJ_EVENT32_BLOCK_SHIFT 4u
static u8 sNdsAObjEvent32PageLo[NDS_AOBJ_EVENT32_PAGES];
static u8 sNdsAObjEvent32PageHi[NDS_AOBJ_EVENT32_PAGES];
static u32 sNdsAObjEvent32OutsideEntries;
__attribute__((used)) volatile u32 gNdsAObjEvent32ForgetSkips;

static void ndsAObjEvent32PageAdd(const void *command, s32 delta)
{
    uintptr_t address = (uintptr_t)command;

    if ((address >= NDS_AOBJ_EVENT32_RAM_BASE) &&
        (address < NDS_AOBJ_EVENT32_RAM_END))
    {
        u32 offset = (u32)(address - NDS_AOBJ_EVENT32_RAM_BASE);
        u32 page = offset >> NDS_AOBJ_EVENT32_PAGE_SHIFT;
        u8 block = (u8)((offset & ((1u << NDS_AOBJ_EVENT32_PAGE_SHIFT) - 1u)) >>
                        NDS_AOBJ_EVENT32_BLOCK_SHIFT);

        if (delta > 0)
        {
            if (sNdsAObjEvent32PageEntries[page] == 0u)
            {
                sNdsAObjEvent32PageLo[page] = block;
                sNdsAObjEvent32PageHi[page] = block;
            }
            else if (block < sNdsAObjEvent32PageLo[page])
            {
                sNdsAObjEvent32PageLo[page] = block;
            }
            else if (block > sNdsAObjEvent32PageHi[page])
            {
                sNdsAObjEvent32PageHi[page] = block;
            }
        }
        sNdsAObjEvent32PageEntries[page] += (u16)delta;
    }
    else
    {
        sNdsAObjEvent32OutsideEntries += (u32)delta;
    }
}

static void ndsAObjEvent32PageReset(void)
{
    memset(sNdsAObjEvent32PageEntries, 0, sizeof(sNdsAObjEvent32PageEntries));
    sNdsAObjEvent32OutsideEntries = 0u;
}

/* TRUE when no ledger entry can lie in [start, end). */
static sb32 ndsAObjEvent32RangeHoldsNone(uintptr_t start, uintptr_t end)
{
    u32 page;
    u32 last;
    u32 first_block;
    u32 last_block;

    if ((sNdsAObjEvent32OutsideEntries != 0u) ||
        (start < NDS_AOBJ_EVENT32_RAM_BASE) ||
        (end > NDS_AOBJ_EVENT32_RAM_END) || (end <= start))
    {
        return FALSE;
    }
    page = (u32)((start - NDS_AOBJ_EVENT32_RAM_BASE) >>
                 NDS_AOBJ_EVENT32_PAGE_SHIFT);
    last = (u32)((end - 1u - NDS_AOBJ_EVENT32_RAM_BASE) >>
                 NDS_AOBJ_EVENT32_PAGE_SHIFT);
    /* The range's blocks inside its first and last pages; every page between
     * is covered whole. */
    first_block = (u32)(((start - NDS_AOBJ_EVENT32_RAM_BASE) &
                         ((1u << NDS_AOBJ_EVENT32_PAGE_SHIFT) - 1u)) >>
                        NDS_AOBJ_EVENT32_BLOCK_SHIFT);
    last_block = (u32)(((end - 1u - NDS_AOBJ_EVENT32_RAM_BASE) &
                        ((1u << NDS_AOBJ_EVENT32_PAGE_SHIFT) - 1u)) >>
                       NDS_AOBJ_EVENT32_BLOCK_SHIFT);
    for (; page <= last; page++)
    {
        if (sNdsAObjEvent32PageEntries[page] != 0u)
        {
            u32 range_hi = (page == last) ? last_block :
                     ((1u << (NDS_AOBJ_EVENT32_PAGE_SHIFT -
                              NDS_AOBJ_EVENT32_BLOCK_SHIFT)) - 1u);

            /* The page's entries lie in blocks [PageLo, PageHi]; the range
             * covers blocks [first_block, range_hi] of it. */
            if ((sNdsAObjEvent32PageHi[page] >= first_block) &&
                (sNdsAObjEvent32PageLo[page] <= range_hi))
            {
                return FALSE;
            }
        }
        first_block = 0u;
    }
    return TRUE;
}

/* Live capacity and its verifier-facing witness. Every new published diagnostic
 * is both `used` and volatile because --gc-sections has removed otherwise
 * unreferenced globals in this target. A valid VS-stage row sets Applied=1;
 * the 5120 default sets it to 0, so a silent fallback cannot masquerade as the
 * per-stage fix. */
__attribute__((used)) volatile u32 gNdsAObjEvent32CapacityGKind = 0xffffffffu;
__attribute__((used)) volatile u32 gNdsAObjEvent32CapacityLimit;
__attribute__((used)) volatile u32 gNdsAObjEvent32CapacityHashSlots;
__attribute__((used)) volatile u32 gNdsAObjEvent32CapacityBytes;
__attribute__((used)) volatile u32 gNdsAObjEvent32CapacityStageBoundApplied;
__attribute__((used)) volatile u32 gNdsAObjEvent32CapacityRefusedCount;
__attribute__((used)) volatile u32 gNdsAObjEvent32StageBoundGKind = 0xffffffffu;
__attribute__((used)) volatile u32 gNdsAObjEvent32StageBoundLimit;
__attribute__((used)) volatile u32 gNdsAObjEvent32StageBoundBytes;
__attribute__((used)) volatile u32 gNdsAObjEvent32StageBoundApplyCount;
/* The roster term of the VS bound (0 before the first VS battle). */
__attribute__((used)) volatile u32 gNdsAObjEvent32RosterNeed;
/* Scripts whose normalization failed; their joints were detached. */
__attribute__((used)) volatile u32 gNdsAObjEvent32DetachCount;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
#include <nds/arm9/cache.h>
/* LAB: the last detached DObj script: [0] script, [1] caller, [2] parent
 * GObj id, [3] the refused command. Flushed, since gdb reads memory. */
__attribute__((used, aligned(32))) volatile u32 gNdsLabDetachWitness[8];
extern volatile u32 gNdsAObjEvent32NormalizeLastFailAddress;

static void ndsLabNoteDetach(DObj *dobj, AObjEvent32 *script, void *caller)
{
    gNdsLabDetachWitness[0] = (u32)(uintptr_t)script;
    gNdsLabDetachWitness[1] = (u32)(uintptr_t)caller;
    gNdsLabDetachWitness[2] = ((dobj != NULL) && (dobj->parent_gobj != NULL)) ?
                                  (u32)dobj->parent_gobj->id : 0xffffffffu;
    gNdsLabDetachWitness[3] = gNdsAObjEvent32NormalizeLastFailAddress;
    DC_FlushRange((const void *)gNdsLabDetachWitness,
                  sizeof(gNdsLabDetachWitness));
}
#endif

/* Static event32 command census + conservative 901-command common-shell and
 * 512-command match corpus. Dream Land deliberately keeps 3072: its measured
 * four-fighter high-water is 2330, so 3072 preserves >512 commands of measured
 * margin and is the 18,432-byte gate recovery requested for P2-2. Zebes' 4035
 * measured high-water similarly requires 4608; it is the only VS stage above
 * 3328. The remaining rows are the 2026-09-09 static-census bounds. */
static u32 ndsAObjEvent32CapacityForGKind(u32 gkind, sb32 *stage_bound)
{
    *stage_bound = TRUE;
    switch (gkind)
    {
    case nGRKindCastle:
        return 1536u;
    case nGRKindSector:
        return 2560u;
    case nGRKindJungle:
        return 2560u;
    case nGRKindZebes:
        return 4608u;
    case nGRKindHyrule:
        return 1536u;
    case nGRKindYoster:
        return 3328u;
    case nGRKindPupupu:
        return 3072u;
    case nGRKindYamabuki:
        return 2560u;
    case nGRKindInishie:
        return 3072u;
    case nGRKindBonus2Fox:
        /* Fox's Board the Platforms: its layer-1 texture cycles alone are
         * four 1,326-command scripts (5,304 entries, see the plan note above
         * NDS_AOBJ_EVENT32_BRANCH_DEPTH_MAX), more than the 5,120 default
         * holds before the board's joints and the fighter's clips. Not a VS
         * bound: no roster formula, Applied stays 0. */
        *stage_bound = FALSE;
        return NDS_AOBJ_EVENT32_BONUS2_FOX_LIMIT;
    default:
        *stage_bound = FALSE;
        return NDS_AOBJ_EVENT32_NORMALIZED_MAX;
    }
}

/* P2-2p8 S1 (2026-09-28): THE BOUNDS ABOVE COVER THE ROSTERS THEY WERE
 * MEASURED ON, NOT EVERY ROSTER. Each fighter's entry motion is an event32
 * clip, and Pikachu's (758 commands), Yoshi's (728), Captain's (599) and
 * Link's (557) are two to three times Kirby's (214). Ness/Yoshi/Pikachu/Purin
 * reached 4,041 on Yoshi's Island against its 3,328 bound; the 19 scripts
 * that no longer fit kept their joints' previous event32 pointers, into the
 * figatree buffer the new motion had just overwritten, and the next parse
 * read a float as a command pointer (the frame-79 crash). So a VS ledger
 * also covers the roster that is actually in the match:
 *
 *   limit = max(stage bound, stage part + sum of the players' entry clips
 *               + margin)
 *
 * Stage part is the largest measured high-water less its roster's clip sum,
 * over four rosters (Ness/Yoshi/Pikachu/Purin, Pikachu/Yoshi/Captain/Link,
 * DK/Samus/Link/Kirby, Mario/Fox/Samus/Captain) on all nine stages
 * (artifacts/performance/2026-09-28_p2-2p8-s1-event32-ledger/). The stress
 * roster itself overflowed the old bounds on Castle, Sector Z (the first S1
 * witness), Jungle and Hyrule.
 *
 * 2026-09-30: a margin of 384 no longer held. Every Ness/Yoshi/Pikachu/Purin
 * motion is an event32 clip, and the elastic motion cache and the idle-time
 * clip prefetch now keep more of them resident (so normalized) than the 09-28
 * census saw. Full-match lab rows at the old margin: Dream Land with
 * Fox/Pikachu/Ness/Samus refused 84 scripts (23 detached joints); Jungle with
 * Captain/Yoshi/Kirby/DK filled at 3,288 and a Poke Ball's rays lost their
 * animation (the ball then faulted, see battleship_item_mball.c); Mushroom
 * Kingdom, Saffron, Castle and Yoshi's Island ran within 3..17 entries of
 * their limits. So the margin also counts the roster's event32-motion
 * fighters: 768 more for each Ness, Yoshi, Pikachu or Purin. Live high-water
 * (count less holes) against the limit this gives, full-match lab rows: Jungle
 * Captain/Yoshi/Kirby/DK 3,406..3,500 of 4,056; Dream Land
 * Fox/Pikachu/Ness/Samus 3,303 of 4,082; Zebes Pikachu x4 6,449 of 8,191;
 * Dream Land and Sector Z DK/Samus/Link/Kirby keep their old limits (2,546
 * of 3,072 and 2,752 of 3,358). A flat 2,048 held too, but its heap on
 * Sector Z's default roster pushed the Arwing's flight table and DMA arena
 * under their heap checks (STG +26K a frame). Receipt
 * 2026-09-30_p2-2p8-event32-margin. */
#define NDS_AOBJ_EVENT32_ROSTER_MARGIN 384u
#define NDS_AOBJ_EVENT32_MOTION_MARGIN 768u
static const u16 sNdsAObjEvent32EntryClipCommands[nFTKindPlayableEnd + 1] = {
    378u, /* Mario 0x279 */
    318u, /* Fox 0x309 */
    268u, /* Donkey 0x3A5 */
    508u, /* Samus 0x443 */
    378u, /* Luigi (Mario's pipe entry) */
    557u, /* Link 0x4DE */
    728u, /* Yoshi 0x7A2 */
    599u, /* Captain 0x670 */
    214u, /* Kirby 0x585 */
    758u, /* Pikachu 0x821 */
    498u, /* Purin 0x5E2 */
    373u, /* Ness 0x70B */
};

static u32 ndsAObjEvent32StagePart(u32 gkind)
{
    switch (gkind)
    {
    case nGRKindCastle:
        return 978u;
    case nGRKindSector:
        return 1427u;
    case nGRKindJungle:
        return 1095u;
    case nGRKindZebes:
        return 3116u;
    case nGRKindHyrule:
        return 72u;
    case nGRKindYoster:
        return 1979u;
    case nGRKindPupupu:
        return 205u;
    case nGRKindYamabuki:
        return 1245u;
    case nGRKindInishie:
        return 1069u;
    default:
        return 0u;
    }
}

/* The players' entry clips plus the event32-motion margins, over `players`. */
static u32 ndsAObjEvent32RosterClips(const SCPlayerData *players)
{
    u32 need = 0u;
    u32 i;

    for (i = 0u; i < GMCOMMON_PLAYERS_MAX; i++)
    {
        const SCPlayerData *player = &players[i];

        if ((player->pkind != nFTPlayerKindNot) &&
            (player->fkind <= nFTKindPlayableEnd))
        {
            need += sNdsAObjEvent32EntryClipCommands[player->fkind];
            if ((player->fkind == nFTKindYoshi) ||
                (player->fkind == nFTKindPikachu) ||
                (player->fkind == nFTKindPurin) ||
                (player->fkind == nFTKindNess))
            {
                need += NDS_AOBJ_EVENT32_MOTION_MARGIN;
            }
        }
    }
    return need;
}

static u32 ndsAObjEvent32RosterLimit(u32 gkind, u32 limit)
{
    u32 need = ndsAObjEvent32StagePart(gkind) + NDS_AOBJ_EVENT32_ROSTER_MARGIN;

    if (gSCManagerBattleState == NULL)
    {
        return limit;
    }
    need += ndsAObjEvent32RosterClips(gSCManagerBattleState->players);
    if (need > (NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS - 1u))
    {
        need = NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS - 1u;
    }
    gNdsAObjEvent32RosterNeed = need;
    return (need > limit) ? need : limit;
}

/* VS RESULTS IS SIZED BY ITS PODIUM, NOT BY THE 5,120 DEFAULT (2026-10-02).
 * Results normalizes the emblem and labels (46..75 entries before tic 120) and
 * then the four fighters' victory and clap motions; the whole scene measured
 * 572 (Mario/Kirby/Fox/Yoshi), 669 (Pikachu/Yoshi/Ness/Purin) and 743
 * (DK/Samus/Captain/Link) live entries at tic 400 and 1,200. The default
 * reserved 41,984 B of its general heap for 5,120, and with the static image
 * 84 KB larger than on 09-30 the second podium fighter's owner image no
 * longer fit (ndsSyMallocOverflowHalt at tic 120). The bound is the battle
 * formula with no stage part: the players' entry clips, the event32-motion
 * margins and the roster margin, 3.4x..9x what each roster used. A refused
 * script is not a soft failure (see S1 above), which is why this is a
 * roster bound and not a flat cut. The players are the transfer state's,
 * the same rows the Results scene builds its podium from. */
#define NDS_AOBJ_EVENT32_RESULTS_PART 128u

static u32 ndsAObjEvent32ResultsLimit(void)
{
    u32 need = NDS_AOBJ_EVENT32_RESULTS_PART + NDS_AOBJ_EVENT32_ROSTER_MARGIN +
        ndsAObjEvent32RosterClips(gSCManagerTransferBattleState.players);

    gNdsAObjEvent32RosterNeed = need;
    return (need < NDS_AOBJ_EVENT32_NORMALIZED_MAX) ?
        need : NDS_AOBJ_EVENT32_NORMALIZED_MAX;
}

static u32 ndsAObjEvent32HashSlotsForLimit(u32 limit)
{
    u32 slots = 1u;

    while (slots <= limit)
    {
        slots <<= 1;
    }
    return slots;
}

/* Called once from the relocation scene-prep seam, before that scene begins
 * normalizing O2R scripts. One allocation keeps the ledger, signature bytes,
 * and hash index under one taskman lifetime and avoids partial-allocation
 * states. Non-VS scenes pass 0xffffffff and retain the conservative 5120
 * default; only a recognized VS-stage gkind publishes Applied=1. */
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB: a VS stage's ledger limit, forced (0 = the per-stage bound). Sizes the
 * bound against rosters the static census did not cover. */
volatile u32 gNdsLabAObjEvent32CapacityOverride
    __attribute__((used, section(".data"))) = 0u;
#endif

/* A VS stage's bound, raised to cover the roster in the match. */
static u32 ndsAObjEvent32VSLimit(u32 gkind, sb32 *stage_bound)
{
    u32 limit = ndsAObjEvent32CapacityForGKind(gkind, stage_bound);

    if (*stage_bound == FALSE)
    {
        if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
        {
            return ndsAObjEvent32ResultsLimit();
        }
        return limit;
    }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    if (gNdsLabAObjEvent32CapacityOverride != 0u)
    {
        return gNdsLabAObjEvent32CapacityOverride;
    }
#endif
    return ndsAObjEvent32RosterLimit(gkind, limit);
}

sb32 ndsAObjEvent32ConfigureNormalizedCapacity(u32 gkind)
{
    sb32 stage_bound;
    u32 limit = ndsAObjEvent32VSLimit(gkind, &stage_bound);
    u32 hash_slots = ndsAObjEvent32HashSlotsForLimit(limit);
    u32 ledger_bytes = limit * (u32)sizeof(NDSAObjEvent32Normalized);
    u32 sig_bytes = limit * (u32)sizeof(u8);
    u32 hash_offset = (ledger_bytes + sig_bytes + 1u) & ~1u;
    u32 hash_bytes = hash_slots * (u32)sizeof(u16);
    u32 alloc_bytes = hash_offset + hash_bytes;
    u8 *storage;

    gNdsAObjEvent32CapacityGKind = gkind;
    gNdsAObjEvent32CapacityLimit = limit;
    gNdsAObjEvent32CapacityHashSlots = hash_slots;
    gNdsAObjEvent32CapacityBytes = alloc_bytes;
    gNdsAObjEvent32CapacityStageBoundApplied = (u32)stage_bound;

    if ((limit == 0u) || (limit >= 0xffffu) ||
        (hash_slots > NDS_AOBJ_EVENT32_NORMALIZED_HASH_SLOTS) ||
        (hash_slots <= limit) || ((hash_slots & (hash_slots - 1u)) != 0u))
    {
        sNdsAObjEvent32Normalized = NULL;
        sNdsAObjEvent32NormalizedSig = NULL;
        sNdsAObjEvent32NormalizedHash = NULL;
        sNdsAObjEvent32NormalizedLimit = 0u;
        sNdsAObjEvent32NormalizedHashSlots = 0u;
        NDS_DIAG(gNdsAObjEvent32CapacityRefusedCount++);
        return FALSE;
    }

    /* This is scene-wide ownership, so bypass ndsTaskmanSwapMallocRegion's
     * short-lived override and take the bytes from the scene general heap
     * explicitly. A nested reloc load may temporarily redirect syTaskmanMalloc;
     * putting the ledger there would leave dangling pointers when that subarena
     * is restored. */
    if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                            (size_t)alloc_bytes, 4u) == FALSE)
    {
        sNdsAObjEvent32Normalized = NULL;
        sNdsAObjEvent32NormalizedSig = NULL;
        sNdsAObjEvent32NormalizedHash = NULL;
        sNdsAObjEvent32NormalizedLimit = 0u;
        sNdsAObjEvent32NormalizedHashSlots = 0u;
        NDS_DIAG(gNdsAObjEvent32CapacityRefusedCount++);
        return FALSE;
    }
    storage = syMallocSet(&gSYTaskmanGeneralHeap, (size_t)alloc_bytes, 4u);
    if (storage == NULL)
    {
        sNdsAObjEvent32Normalized = NULL;
        sNdsAObjEvent32NormalizedSig = NULL;
        sNdsAObjEvent32NormalizedHash = NULL;
        sNdsAObjEvent32NormalizedLimit = 0u;
        sNdsAObjEvent32NormalizedHashSlots = 0u;
        NDS_DIAG(gNdsAObjEvent32CapacityRefusedCount++);
        return FALSE;
    }

    sNdsAObjEvent32Normalized = (NDSAObjEvent32Normalized *)(void *)storage;
    sNdsAObjEvent32NormalizedSig = storage + ledger_bytes;
    sNdsAObjEvent32NormalizedHash = (u16 *)(void *)(storage + hash_offset);
    sNdsAObjEvent32NormalizedLimit = limit;
    sNdsAObjEvent32NormalizedHashSlots = hash_slots;
    /* hash_slots is a power of two above limit >= 1, so the shift is 1..31. */
    sNdsAObjEvent32NormalizedHashShift = 32u;
    while ((1u << (32u - sNdsAObjEvent32NormalizedHashShift)) < hash_slots)
    {
        sNdsAObjEvent32NormalizedHashShift--;
    }
    memset(sNdsAObjEvent32NormalizedHash, 0, (size_t)hash_bytes);
    sNdsAObjEvent32NormalizedCount = 0u;
    sNdsAObjEvent32Holes = 0u;
    sNdsAObjEvent32Tombs = 0u;
    ndsAObjEvent32PageReset();
    sNdsAObjEvent32PlanCount = 0u;
    sNdsAObjEvent32PlanSegmentCount = 0u;
    sNdsEvent32InterpDescFixedCount = 0u;
    if (stage_bound != FALSE)
    {
        gNdsAObjEvent32StageBoundGKind = gkind;
        gNdsAObjEvent32StageBoundLimit = limit;
        gNdsAObjEvent32StageBoundBytes = alloc_bytes;
        NDS_DIAG(gNdsAObjEvent32StageBoundApplyCount++);
    }
    return TRUE;
}

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

/* Commands are 4-byte objects inside one loaded file, so a script's commands
 * are adjacent words. The two folds used before kept adjacent words on
 * adjacent slots, and linear probing then walked whole scripts: on the gate a
 * lookup took 12.2 probes and an insert ~37, at a load under 20% (P2-2p8,
 * 2026-09-29). The multiplicative hash takes the product's top bits, which
 * every input bit reaches. Placement only: the ledger's keys are unique, so a
 * probe returns the same index whatever the hash. */

static u32 ndsAObjEvent32HashSlot(const AObjEvent32 *command)
{
    const u32 h = (u32)(uintptr_t)command >> 2;

    return (h * 0x9E3779B1u) >> sNdsAObjEvent32NormalizedHashShift;
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

    for (probes = 0u; probes < sNdsAObjEvent32NormalizedHashSlots;
         probes++)
    {
        const u32 entry = sNdsAObjEvent32NormalizedHash[slot];

        if ((entry == 0u) || (entry == NDS_AOBJ_EVENT32_TOMB))
        {
            if (entry == NDS_AOBJ_EVENT32_TOMB)
            {
                sNdsAObjEvent32Tombs--;
            }
            sNdsAObjEvent32NormalizedHash[slot] = (u16)(index + 1u);
            NDS_DIAG(gNdsAObjEvent32HashInsertProbeCount += probes + 1u);
            return;
        }
        slot = (slot + 1u) & (sNdsAObjEvent32NormalizedHashSlots - 1u);
    }
    /* Unreachable while the static assert above holds: the ledger cannot hold
     * more entries than the index has slots. Counted rather than asserted so
     * that if it ever does happen the lookup below degrades to the scan it
     * replaced instead of dropping an entry. */
    gNdsAObjEvent32HashOverflowCount++;
}

/* Rebuild the pointer index after an owning buffer is retired.  This is not a
 * normal lookup/insert operation, so deliberately do not charge the rebuild to
 * the hash engagement counters: those counters price parser work, while this is
 * relocation-lifetime maintenance at the force-load seam. */
static void ndsAObjEvent32RebuildNormalizedIndex(void)
{
    u32 i;

    if ((sNdsAObjEvent32NormalizedHash == NULL) ||
        (sNdsAObjEvent32NormalizedHashSlots == 0u))
    {
        return;
    }
    for (i = 0u; i < sNdsAObjEvent32NormalizedHashSlots; i++)
    {
        sNdsAObjEvent32NormalizedHash[i] = 0u;
    }
    sNdsAObjEvent32Tombs = 0u;
    for (i = 0u; i < sNdsAObjEvent32NormalizedCount; i++)
    {
        u32 slot;
        u32 probes;

        if (sNdsAObjEvent32Normalized[i].command == NULL)
        {
            continue;
        }
        slot = ndsAObjEvent32HashSlot(sNdsAObjEvent32Normalized[i].command);

        for (probes = 0u; probes < sNdsAObjEvent32NormalizedHashSlots;
             probes++)
        {
            if (sNdsAObjEvent32NormalizedHash[slot] == 0u)
            {
                sNdsAObjEvent32NormalizedHash[slot] = (u16)(i + 1u);
                break;
            }
            slot = (slot + 1u) &
                   (sNdsAObjEvent32NormalizedHashSlots - 1u);
        }
        if (probes == sNdsAObjEvent32NormalizedHashSlots)
        {
            /* The static load-factor assertion makes this unreachable.  Keep
             * the same fail-open diagnostic as ordinary index insertion if a
             * future capacity edit violates that invariant. */
            gNdsAObjEvent32HashOverflowCount++;
        }
    }
}

/* The index slot naming ledger entry `index`; the slot count when absent. */
static u32 ndsAObjEvent32IndexSlotOf(u32 index)
{
    const u32 mask = sNdsAObjEvent32NormalizedHashSlots - 1u;
    u32 slot = ndsAObjEvent32HashSlot(sNdsAObjEvent32Normalized[index].command);
    u32 probes;

    for (probes = 0u; probes < sNdsAObjEvent32NormalizedHashSlots; probes++)
    {
        const u32 entry = sNdsAObjEvent32NormalizedHash[slot];

        if (entry == 0u)
        {
            break;
        }
        if (entry == index + 1u)
        {
            return slot;
        }
        slot = (slot + 1u) & mask;
    }
    return sNdsAObjEvent32NormalizedHashSlots;
}

/* Drop the holes (every live entry moves down, in order) and rebuild. */
static void ndsAObjEvent32CompactLedger(void)
{
    u32 read_index;
    u32 write_index = 0u;

    for (read_index = 0u; read_index < sNdsAObjEvent32NormalizedCount;
         read_index++)
    {
        if (sNdsAObjEvent32Normalized[read_index].command == NULL)
        {
            continue;
        }
        if (write_index != read_index)
        {
            sNdsAObjEvent32Normalized[write_index] =
                sNdsAObjEvent32Normalized[read_index];
            sNdsAObjEvent32NormalizedSig[write_index] =
                sNdsAObjEvent32NormalizedSig[read_index];
        }
        write_index++;
    }
    sNdsAObjEvent32NormalizedCount = write_index;
    sNdsAObjEvent32Holes = 0u;
    ndsAObjEvent32RebuildNormalizedIndex();
    NDS_DIAG(gNdsAObjEvent32LedgerCompactions++);
}

/* Sector Z Arwing flight-path descriptors (asset 0x99, MiscDataBank153).
 *
 * decomp 153_StageSectorFile3.c holds 8 GRSectorDesc rows whose TraI scripts
 * (aobjEvent32SetInterp) each name one SYInterpDesc block: 14 scripts, 14
 * distinct descs (0x00E8, 0x0210, 0x0338, 0x0510, 0x0698, 0x09B0, 0x0CD8,
 * 0x0DF8, 0x1004, 0x1188, 0x1324, 0x14C8, 0x17F0, 0x1AF4), played through
 * grsector.c:1054-1059 via gcAddDObjAnimJoint, i.e. through the normalizer
 * below. The blanket u32 swap is correct for every full-width descriptor
 * field (f32 unk04/length, the three pointers the reloc chain patches, and
 * the points/keyframes/quartics tables) but wrong for word 0's mixed widths
 * ({u8 kind; u8 pad; s16 points_num}, decomp sys/interp.h:8-18), exactly the
 * fighter AObj16 family's bug. The AObj16 pass cannot cover it: it only walks
 * fighter figatree assets, and the per-stage ground normalizer only touches
 * the 0x14 MPGroundData header. The GRSectorDesc rows themselves need no lane
 * fix (relocated script pointers plus the source's own zero filler).
 *
 * Fix word 0 with ndsRelocSYInterpDescHeaderNative (reloc_backend_assets.c)
 * when the owning SetInterp script first normalizes. The transform is not
 * idempotent, so the ledger below is the exactly-once precondition: one entry
 * per fixed descriptor, holding the word the fix wrote. A path can be shared:
 * Kirby's Board the Platforms runs two rail platforms on one SYInterpDesc,
 * two scripts with different phases. A later script (or a second SetInterp in
 * the same plan) naming a ledgered descriptor whose word 0 still reads as
 * fixed is admitted without fixing again; a word that no longer matches is a
 * stale entry and is refused (reason 13), as is ledger overflow. Purged with
 * the command ledger it shadows (ForgetRange per retired buffer, Reset on
 * scene teardown), so a reused address fixes exactly once again. 32 slots
 * cover the 14 Sector Z descs with margin; growth is coverage of a finite
 * corpus, not a leak. */
#define NDS_AOBJ_EVENT32_INTERP_DESC_FIXED_MAX 32u
static void *sNdsEvent32InterpDescFixed[NDS_AOBJ_EVENT32_INTERP_DESC_FIXED_MAX];
static u32 sNdsEvent32InterpDescFixedWord[NDS_AOBJ_EVENT32_INTERP_DESC_FIXED_MAX];
/* Definition site: src/port/reloc_backend_assets.c (non-static for this use). */
extern u32 ndsRelocSYInterpDescHeaderNative(u32 swapped);
__attribute__((used)) volatile u32 gNdsEvent32SYInterpDescFixCount;
__attribute__((used)) volatile u32 gNdsEvent32SYInterpDescUnresolvedCount;
__attribute__((used)) volatile u32 gNdsEvent32SYInterpDescUnresolvedAddr;

static s32 ndsEvent32InterpDescFindFixed(const void *desc)
{
    u32 i;

    for (i = 0u; i < sNdsEvent32InterpDescFixedCount; i++)
    {
        if (sNdsEvent32InterpDescFixed[i] == desc)
        {
            return (s32)i;
        }
    }
    return -1;
}

void ndsAObjEvent32ForgetRange(const void *base, size_t size)
{
    uintptr_t range_start;
    uintptr_t range_end;
    u32 read_index;
    u32 write_index = 0u;
    u32 edge_page[2];
    u32 edge_lo[2];
    u32 edge_hi[2];
    u32 side;
    const sb32 holes = ((gNdsAObjEvent32ForgetHoles != 0u) &&
                        (sNdsAObjEvent32NormalizedHash != NULL)) ? TRUE : FALSE;
    sb32 rebuild = FALSE;

    if ((base == NULL) || (size == 0u))
    {
        return;
    }
    range_start = (uintptr_t)base;
    range_end = range_start + size;
    if (range_end < range_start)
    {
        return;
    }

    if (ndsAObjEvent32RangeHoldsNone(range_start, range_end) != FALSE)
    {
        NDS_DIAG(gNdsAObjEvent32ForgetSkips++);
        read_index = write_index = sNdsAObjEvent32NormalizedCount;
    }
    else
    {
        /* The range's first and last pages (an index past the table when the
         * edge is outside main RAM): the scan re-derives their block interval
         * from the entries it keeps, so a stale wide one refuses one skip,
         * not every later one. */
        edge_page[0] = edge_page[1] = NDS_AOBJ_EVENT32_PAGES;
        if ((range_start >= NDS_AOBJ_EVENT32_RAM_BASE) &&
            (range_start < NDS_AOBJ_EVENT32_RAM_END))
        {
            edge_page[0] = (u32)((range_start - NDS_AOBJ_EVENT32_RAM_BASE) >>
                                 NDS_AOBJ_EVENT32_PAGE_SHIFT);
        }
        if ((range_end > NDS_AOBJ_EVENT32_RAM_BASE) &&
            (range_end <= NDS_AOBJ_EVENT32_RAM_END))
        {
            edge_page[1] = (u32)((range_end - 1u - NDS_AOBJ_EVENT32_RAM_BASE) >>
                                 NDS_AOBJ_EVENT32_PAGE_SHIFT);
        }
        edge_lo[0] = edge_lo[1] = 0xffu;
        edge_hi[0] = edge_hi[1] = 0u;
        for (read_index = 0u; read_index < sNdsAObjEvent32NormalizedCount;
             read_index++)
        {
            uintptr_t command =
                (uintptr_t)sNdsAObjEvent32Normalized[read_index].command;
            u32 page;
            u32 block;
            u32 edge;

            if (command == 0u)
            {
                /* A hole: the ordered pass drops it with the rest. */
                continue;
            }
            if ((command >= range_start) && (command < range_end))
            {
                ndsAObjEvent32PageAdd((const void *)command, -1);
                if (holes != FALSE)
                {
                    const u32 slot = ndsAObjEvent32IndexSlotOf(read_index);

                    if (slot < sNdsAObjEvent32NormalizedHashSlots)
                    {
                        sNdsAObjEvent32NormalizedHash[slot] =
                            NDS_AOBJ_EVENT32_TOMB;
                        sNdsAObjEvent32Tombs++;
                    }
                    else
                    {
                        rebuild = TRUE;
                    }
                    sNdsAObjEvent32Normalized[read_index].command = NULL;
                    sNdsAObjEvent32Holes++;
                    NDS_DIAG(gNdsAObjEvent32ForgetHoleRemovals++);
                }
                continue;
            }
            if (holes != FALSE)
            {
                /* Kept in place: nothing moves. */
            }
            else if (write_index != read_index)
            {
                sNdsAObjEvent32Normalized[write_index] =
                    sNdsAObjEvent32Normalized[read_index];
                sNdsAObjEvent32NormalizedSig[write_index] =
                    sNdsAObjEvent32NormalizedSig[read_index];
            }
            write_index++;
            page = (u32)(command - NDS_AOBJ_EVENT32_RAM_BASE) >>
                   NDS_AOBJ_EVENT32_PAGE_SHIFT;
            edge = (page == edge_page[0]) ? 0u :
                   (page == edge_page[1]) ? 1u : 2u;
            if (edge < 2u)
            {
                block = (u32)(command &
                              ((1u << NDS_AOBJ_EVENT32_PAGE_SHIFT) - 1u)) >>
                        NDS_AOBJ_EVENT32_BLOCK_SHIFT;
                if (block < edge_lo[edge])
                {
                    edge_lo[edge] = block;
                }
                if (block > edge_hi[edge])
                {
                    edge_hi[edge] = block;
                }
            }
        }
        /* A single-page range accumulated everything in slot 0. */
        for (side = 0u; side < 2u; side++)
        {
            u32 page = edge_page[side];

            if ((page < NDS_AOBJ_EVENT32_PAGES) &&
                (sNdsAObjEvent32PageEntries[page] != 0u) &&
                (edge_lo[side] <= edge_hi[side]))
            {
                sNdsAObjEvent32PageLo[page] = (u8)edge_lo[side];
                sNdsAObjEvent32PageHi[page] = (u8)edge_hi[side];
            }
        }
    }
    if (holes != FALSE)
    {
        if ((rebuild != FALSE) ||
            (sNdsAObjEvent32Tombs > (sNdsAObjEvent32NormalizedHashSlots >> 2)))
        {
            NDS_DIAG(gNdsAObjEvent32ForgetHoleRebuilds++);
            ndsAObjEvent32RebuildNormalizedIndex();
        }
    }
    else if (write_index != sNdsAObjEvent32NormalizedCount)
    {
        sNdsAObjEvent32NormalizedCount = write_index;
        sNdsAObjEvent32Holes = 0u;
        ndsAObjEvent32RebuildNormalizedIndex();
    }

    /* The descriptor ledger keys the same buffer lifetime: a retired file's
     * descriptors must fix exactly once again if their addresses are reused. */
    {
        u32 scan;
        u32 kept = 0u;

        for (scan = 0u;
             scan < sNdsEvent32InterpDescFixedCount;
             scan++)
        {
            uintptr_t desc =
                (uintptr_t)sNdsEvent32InterpDescFixed[scan];

            if ((desc >= range_start) && (desc < range_end))
            {
                continue;
            }
            if (kept != scan)
            {
                sNdsEvent32InterpDescFixed[kept] =
                    sNdsEvent32InterpDescFixed[scan];
                sNdsEvent32InterpDescFixedWord[kept] =
                    sNdsEvent32InterpDescFixedWord[scan];
            }
            kept++;
        }
        sNdsEvent32InterpDescFixedCount = kept;
    }
}

/* sNdsAObjEvent32NormalizedCount is reset on every scene teardown, so reading
 * it at the end of a run reports the LAST scene only. That is how a five-entry
 * chain reported 297 and got written up as "a chain cannot fill the table"
 * while a single match was standing at 889 (2026-08-13 stress battery). This is
 * the peak across resets, and it is what a soak should read. */

/* TraI descriptor guard and witness. BattleShip never evaluates a TraI track
 * whose SYInterpDesc pointer is not inside the animation file that set it; the
 * port did, on Planet Zebes (2026-09-07): Mario's Appear (an event32 motion
 * bound after the stage's 56-frame entry pan) reached syInterpGetFracFrame with
 * desc 0 or 0x98000000 and data-aborted. Refuse the evaluation, keep the
 * joint's translate, and record enough to name the owner. Exact, cheap (one
 * loaded-file range lookup per TraI evaluation) and read by the stage probes. */
__attribute__((used)) volatile u32 gNdsTraIBadDescCount;
__attribute__((used)) volatile u32 gNdsTraIBadDescPtr;
__attribute__((used)) volatile u32 gNdsTraIBadDescDObj;
__attribute__((used)) volatile u32 gNdsTraIBadDescGObj;
__attribute__((used)) volatile u32 gNdsTraIBadDescKind;
__attribute__((used)) volatile u32 gNdsTraIBadDescScript;
__attribute__((used)) volatile u32 gNdsTraIBadDescSite;

sb32 ndsTraIDescUsable(DObj *dobj, const AObj *aobj, u32 site)
{
    if ((aobj->interpolate != NULL) &&
        (ndsRelocPointerRangeInLoadedFiles(aobj->interpolate, 16u) != FALSE))
    {
        return TRUE;
    }
    NDS_DIAG(gNdsTraIBadDescCount++);
    gNdsTraIBadDescPtr = (u32)(uintptr_t)aobj->interpolate;
    gNdsTraIBadDescDObj = (u32)(uintptr_t)dobj;
    gNdsTraIBadDescGObj = (u32)(uintptr_t)dobj->parent_gobj;
    gNdsTraIBadDescKind = aobj->kind;
    gNdsTraIBadDescScript = (u32)(uintptr_t)dobj->anim_joint.event32;
    gNdsTraIBadDescSite = site;
    return FALSE;
}


volatile u32 gNdsAObjEvent32NormalizedHighWater;
volatile u32 gNdsAObjEvent32LiveHighWater __attribute__((used));
/* Longest single script plan seen (commands); it now borrows ledger tail
 * room, so this is the free ledger a script needs, not a static array. */
volatile u32 gNdsAObjEvent32PlanHighWater;
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

/* objanim.c:2372-2427 exactly, with the same attachment-boundary copy as
 * gcAddMObjAll above. Items whose ITAttributes say is_item_dobjs == 0 (the
 * Mushroom Kingdom Piranha Plant and POW block, 260_GRInishieMap.c:131-137)
 * build their tree here instead of through gcAddMObjAll, and the source
 * function attached the stage file's MObjSub raw: the loader's blanket u32
 * swap leaves fmt/siz/flags/block and the colour bytes in the wrong lanes,
 * so the plant prepared as the wrong texture format and colours
 * (docs/BUGS.md "pihrana plants garbled", 2026-09-07). */
void gcSetupCustomDObjsWithMObj(GObj *gobj, DObjDesc *dobjdesc,
                                MObjSub ***p_mobjsubs, DObj **dobjs,
                                u8 tk1, u8 tk2, u8 tk3)
{
    s32 i;
    DObj *dobj;
    s32 id;
    DObj *array_dobjs[DOBJ_ARRAY_MAX];

    for (i = 0; i < ARRAY_COUNT(array_dobjs); i++)
    {
        array_dobjs[i] = NULL;
    }
    while (dobjdesc->id != ARRAY_COUNT(array_dobjs))
    {
        id = dobjdesc->id & 0xFFF;

        if (id != 0)
        {
            dobj = array_dobjs[id] =
                gcAddChildForDObj(array_dobjs[id - 1], dobjdesc->dl);
        }
        else
        {
            dobj = array_dobjs[0] = gcAddDObjForGObj(gobj, dobjdesc->dl);
        }
        if (dobjdesc->id & 0xF000)
        {
            gcDecideDObj3TransformsKind(dobj, tk1, tk2, tk3,
                                        dobjdesc->id & 0xF000);
        }
        else
        {
            gcAddDObj3TransformsKind(dobj, tk1, tk2, tk3);
        }
        dobj->translate.vec.f = dobjdesc->translate;
        dobj->rotate.vec.f = dobjdesc->rotate;
        dobj->scale.vec.f = dobjdesc->scale;

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
                        gNdsMObjSubAttachNormalizeCount++;
                    }
                    else if (normalize_result == 0)
                    {
                        gNdsMObjSubAttachNativeCount++;
                    }
                    else
                    {
                        gNdsMObjSubAttachFailCount++;
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
        if (dobjs != NULL)
        {
            *dobjs++ = dobj;
        }
        dobjdesc++;
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

    for (probes = 0u; probes < sNdsAObjEvent32NormalizedHashSlots;
         probes++)
    {
        u32 entry = sNdsAObjEvent32NormalizedHash[slot];

        if (entry == 0u)
        {
            NDS_DIAG(gNdsAObjEvent32HashMissCount++);
            break;
        }
        if ((entry != NDS_AOBJ_EVENT32_TOMB) &&
            (sNdsAObjEvent32Normalized[entry - 1u].command == command))
        {
            NDS_DIAG(gNdsAObjEvent32HashHitCount++);
            found = (s32)(entry - 1u);
            break;
        }
        slot = (slot + 1u) & (sNdsAObjEvent32NormalizedHashSlots - 1u);
    }
    NDS_DIAG(gNdsAObjEvent32HashProbeCount += probes + 1u);
    if (probes == sNdsAObjEvent32NormalizedHashSlots)
    {
        gNdsAObjEvent32HashOverflowCount++;
        return ndsAObjEvent32ScanNormalized(command);
    }
#if NDS_AOBJ_EVENT32_HASH_ORACLE
    {
        s32 scanned = ndsAObjEvent32ScanNormalized(command);

        NDS_DIAG(gNdsAObjEvent32HashOracleRuns++);
        if (scanned != found)
        {
            NDS_DIAG(gNdsAObjEvent32HashOracleMismatch++);
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
    u32 s;

    for (s = 0u; s < sNdsAObjEvent32PlanSegmentCount; s++)
    {
        const NDSAObjEvent32PlanSegment *segment =
            &sNdsAObjEvent32PlanSegments[s];
        u32 i;

        if ((segment->plan_end == segment->plan_start) ||
            (command < segment->first) || (command > segment->last))
        {
            continue;
        }
        for (i = segment->plan_start; i < segment->plan_end; i++)
        {
            if (ndsAObjEvent32PlanCommand(i) == command)
            {
                return (s32)i;
            }
        }
    }
    return -1;
}

/* Room for one more plan entry above the committed ledger. When the ledger is
 * full, compacting its holes is what the old post-plan check did; the plan
 * sits above the committed entries, so it follows them down. */
static sb32 ndsAObjEvent32PlanReserve(void)
{
    u32 committed;

    if ((sNdsAObjEvent32NormalizedCount + sNdsAObjEvent32PlanCount) <
        sNdsAObjEvent32NormalizedLimit)
    {
        return TRUE;
    }
    if (sNdsAObjEvent32Holes == 0u)
    {
        return FALSE;
    }
    committed = sNdsAObjEvent32NormalizedCount;
    ndsAObjEvent32CompactLedger();
    memmove(&sNdsAObjEvent32Normalized[sNdsAObjEvent32NormalizedCount],
            &sNdsAObjEvent32Normalized[committed],
            sNdsAObjEvent32PlanCount * sizeof(sNdsAObjEvent32Normalized[0]));
    return ((sNdsAObjEvent32NormalizedCount + sNdsAObjEvent32PlanCount) <
            sNdsAObjEvent32NormalizedLimit) ? TRUE : FALSE;
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
    NDSAObjEvent32PlanSegment *segment;

    if ((script == NULL) ||
        (branch_depth > NDS_AOBJ_EVENT32_BRANCH_DEPTH_MAX))
    {
        return ndsAObjEvent32Reject(1u, script, owner_kind, 0u);
    }
    /* One frame per branch depth, so depth 0..BRANCH_DEPTH_MAX fills it. */
    segment = &sNdsAObjEvent32PlanSegments[sNdsAObjEvent32PlanSegmentCount++];
    segment->first = script;
    segment->last = script;
    segment->plan_start = sNdsAObjEvent32PlanCount;
    segment->plan_end = sNdsAObjEvent32PlanCount;

    while (TRUE)
    {
        AObjEvent32 *branch_target = NULL;
        u32 source_word;
        u32 opcode;
        u32 flags;
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
            return (ndsAObjEvent32WordSig(command->u) ==
                    sNdsAObjEvent32NormalizedSig[normalized_index]) ?
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

        if (ndsRelocPointerRangeInLoadedFiles(
                command, (1u + value_words) * sizeof(*command)) == FALSE)
        {
            return ndsAObjEvent32Reject(11u, command, owner_kind,
                                        source_word);
        }
        if (ndsAObjEvent32PlanReserve() == FALSE)
        {
            return ndsAObjEvent32Reject(12u, command, owner_kind,
                                        source_word);
        }

        sNdsAObjEvent32Normalized[sNdsAObjEvent32NormalizedCount +
                                  sNdsAObjEvent32PlanCount].command = command;
        sNdsAObjEvent32PlanCount++;
        segment->last = command;
        segment->plan_end = sNdsAObjEvent32PlanCount;
        if (sNdsAObjEvent32PlanCount > gNdsAObjEvent32PlanHighWater)
        {
            gNdsAObjEvent32PlanHighWater = sNdsAObjEvent32PlanCount;
        }

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

/* Read-only mirror of the fix loop below. Every DObj SetInterp (TraI) command
 * in the plan must name a resident 24-byte SYInterpDesc (sizeof(SYInterpDesc)
 * is 24; only word 0 is rewritten, but the whole struct must be resident for
 * the path to be usable). A descriptor an earlier plan entry names is fixed
 * by that entry; one already in the ledger is admitted while its word 0 still
 * holds what the fix wrote. Every other descriptor needs room left in the
 * fixed ledger. Writes nothing, touches no fix counter; unresolved/refusal
 * accounting only. */
static sb32 ndsAObjEvent32ValidateInterpDescs(NDSAObjEvent32OwnerKind owner_kind)
{
    u32 i;
    u32 fresh = 0u;
    s32 fixed_index;

    if (owner_kind != nNDSAObjEvent32OwnerDObj)
    {
        return TRUE;
    }
    for (i = 0u; i < sNdsAObjEvent32PlanCount; i++)
    {
        u32 opcode =
            (ndsAObjEvent32PlanCommand(i)->u >> 25) & 0x7fu;
        const void *desc;
        u32 j;

        if (opcode != (u32)nGCAnimEvent32SetInterp)
        {
            continue;
        }
        desc = (const void *)ndsAObjEvent32PlanCommand(i)[1].p;
        if (ndsRelocPointerRangeInLoadedFiles(desc, 24u) == FALSE)
        {
            gNdsEvent32SYInterpDescUnresolvedAddr = (u32)(uintptr_t)desc;
            NDS_DIAG(gNdsEvent32SYInterpDescUnresolvedCount++);
            (void)ndsAObjEvent32Reject(13u, ndsAObjEvent32PlanCommand(i),
                                       owner_kind,
                                       ndsAObjEvent32PlanCommand(i)->u);
            return FALSE;
        }
        for (j = 0u; j < i; j++)
        {
            u32 jopcode =
                (ndsAObjEvent32PlanCommand(j)->u >> 25) & 0x7fu;

            if ((jopcode == (u32)nGCAnimEvent32SetInterp) &&
                ((const void *)ndsAObjEvent32PlanCommand(j)[1].p == desc))
            {
                break;
            }
        }
        if (j < i)
        {
            continue;
        }
        fixed_index = ndsEvent32InterpDescFindFixed(desc);
        if (fixed_index >= 0)
        {
            if (*(const u32 *)desc ==
                sNdsEvent32InterpDescFixedWord[fixed_index])
            {
                continue;
            }
            gNdsEvent32SYInterpDescUnresolvedAddr = (u32)(uintptr_t)desc;
            NDS_DIAG(gNdsEvent32SYInterpDescUnresolvedCount++);
            (void)ndsAObjEvent32Reject(13u, ndsAObjEvent32PlanCommand(i),
                                       owner_kind,
                                       ndsAObjEvent32PlanCommand(i)->u);
            return FALSE;
        }
        fresh++;
        if ((sNdsEvent32InterpDescFixedCount + fresh) >
            NDS_AOBJ_EVENT32_INTERP_DESC_FIXED_MAX)
        {
            gNdsEvent32SYInterpDescUnresolvedAddr = (u32)(uintptr_t)desc;
            NDS_DIAG(gNdsEvent32SYInterpDescUnresolvedCount++);
            (void)ndsAObjEvent32Reject(13u, ndsAObjEvent32PlanCommand(i),
                                       owner_kind,
                                       ndsAObjEvent32PlanCommand(i)->u);
            return FALSE;
        }
    }
    return TRUE;
}

/* Applies the validated fixes. Runs only after ValidateInterpDescs approved
 * the whole plan, so it cannot overflow the ledger it just budgeted. A shared
 * descriptor is already in the ledger (from an earlier script, or from the
 * plan entry that fixed it a moment ago) and is left as it is. Runs before
 * the commit, while the plan still reads the source command words; the
 * commit after it cannot fail. */
static void ndsAObjEvent32FixInterpDescs(void)
{
    u32 i;

    for (i = 0u; i < sNdsAObjEvent32PlanCount; i++)
    {
        u32 opcode =
            (ndsAObjEvent32PlanCommand(i)->u >> 25) & 0x7fu;

        if (opcode == (u32)nGCAnimEvent32SetInterp)
        {
            u32 *word =
                (u32 *)(void *)ndsAObjEvent32PlanCommand(i)[1].p;

            if (ndsEvent32InterpDescFindFixed(word) >= 0)
            {
                continue;
            }
            *word = ndsRelocSYInterpDescHeaderNative(*word);
            sNdsEvent32InterpDescFixed[sNdsEvent32InterpDescFixedCount] =
                (void *)word;
            sNdsEvent32InterpDescFixedWord[sNdsEvent32InterpDescFixedCount] =
                *word;
            sNdsEvent32InterpDescFixedCount++;
            NDS_DIAG(gNdsEvent32SYInterpDescFixCount++);
        }
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

    /* Scene prep owns this allocation. If that seam was skipped or allocation
     * failed, refuse loudly before dereferencing a NULL ledger. There is no
     * hidden 5120 fallback here: CapacityStageBoundApplied is the witness that
     * a VS stage actually received its row. */
    if ((sNdsAObjEvent32Normalized == NULL) ||
        (sNdsAObjEvent32NormalizedSig == NULL) ||
        (sNdsAObjEvent32NormalizedHash == NULL) ||
        (sNdsAObjEvent32NormalizedLimit == 0u) ||
        (sNdsAObjEvent32NormalizedHashSlots == 0u))
    {
        NDS_DIAG(gNdsAObjEvent32CapacityRefusedCount++);
        (void)ndsAObjEvent32Reject(14u, script, owner_kind, script->u);
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }

    normalized_index = ndsAObjEvent32FindNormalized(script);
    if (normalized_index >= 0)
    {
        if (ndsAObjEvent32WordSig(script->u) ==
            sNdsAObjEvent32NormalizedSig[normalized_index])
        {
            gNdsAObjEvent32NormalizeReuseCount++;
            return TRUE;
        }
        (void)ndsAObjEvent32Reject(3u, script, owner_kind, script->u);
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }

    /* The plan reserves its ledger room entry by entry (reason 12 when the
     * ledger, compacted, is full), so a planned script always fits. */
    sNdsAObjEvent32PlanCount = 0u;
    sNdsAObjEvent32PlanSegmentCount = 0u;
    if (ndsAObjEvent32PlanStream(script, owner_kind, 0u) == FALSE)
    {
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }

    /* Sector Z TraI descriptors: validated read-only first, because the
     * header lane fix is not idempotent and a refusal must leave commands
     * and descriptors pristine. */
    if (ndsAObjEvent32ValidateInterpDescs(owner_kind) == FALSE)
    {
        gNdsAObjEvent32NormalizeFailCount++;
        return FALSE;
    }
    if (owner_kind == nNDSAObjEvent32OwnerDObj)
    {
        ndsAObjEvent32FixInterpDescs();
    }

    if ((gNdsAObjEvent32NormalizeCommandCount == 0u) &&
        (sNdsAObjEvent32PlanCount != 0u))
    {
        gNdsAObjEvent32NormalizeFirstSourceWord =
            ndsAObjEvent32PlanCommand(0u)->u;
        gNdsAObjEvent32NormalizeFirstNativeWord =
            ndsAObjEvent32NativeWord(gNdsAObjEvent32NormalizeFirstSourceWord);
    }

    /* Each plan entry already occupies the ledger slot it commits to: rewrite
     * its word, sign and index that slot, and the next entry is the tail. */
    for (i = 0u; i < sNdsAObjEvent32PlanCount; i++)
    {
        AObjEvent32 *command =
            sNdsAObjEvent32Normalized[sNdsAObjEvent32NormalizedCount].command;
        u32 native_word = ndsAObjEvent32NativeWord(command->u);

        command->u = native_word;
        sNdsAObjEvent32NormalizedSig[sNdsAObjEvent32NormalizedCount] =
            ndsAObjEvent32WordSig(native_word);
        ndsAObjEvent32IndexNormalized(sNdsAObjEvent32NormalizedCount);
        ndsAObjEvent32PageAdd(command, 1);
        sNdsAObjEvent32NormalizedCount++;
    }

    if (sNdsAObjEvent32NormalizedCount > gNdsAObjEvent32NormalizedHighWater)
    {
        gNdsAObjEvent32NormalizedHighWater = sNdsAObjEvent32NormalizedCount;
    }
    /* The count keeps the holes ForgetRange leaves until an append needs the
     * room, so it is not the working set; live entries are what a limit must
     * hold (2026-09-30: Jungle, Captain/Yoshi/Kirby/DK, live ~3,300 against a
     * 3,288 limit). */
    if ((sNdsAObjEvent32NormalizedCount - sNdsAObjEvent32Holes) >
        gNdsAObjEvent32LiveHighWater)
    {
        gNdsAObjEvent32LiveHighWater =
            sNdsAObjEvent32NormalizedCount - sNdsAObjEvent32Holes;
    }
    gNdsAObjEvent32NormalizeScriptCount++;
    gNdsAObjEvent32NormalizeCommandCount += sNdsAObjEvent32PlanCount;
    return TRUE;
}

void ndsAObjEvent32ResetNormalizedScripts(void)
{
    /* The three arrays now live in the taskman arena. By the time a scene-reset
     * owner calls here, that arena may already have been rewound, so touching
     * the old hash would write into the next scene. Discard the complete owner
     * tuple; ConfigureNormalizedCapacity installs and clears fresh storage. */
    sNdsAObjEvent32Normalized = NULL;
    sNdsAObjEvent32NormalizedSig = NULL;
    sNdsAObjEvent32NormalizedHash = NULL;
    sNdsAObjEvent32NormalizedLimit = 0u;
    sNdsAObjEvent32NormalizedHashSlots = 0u;
    sNdsAObjEvent32NormalizedCount = 0u;
    sNdsAObjEvent32Holes = 0u;
    sNdsAObjEvent32Tombs = 0u;
    ndsAObjEvent32PageReset();
    sNdsAObjEvent32PlanCount = 0u;
    sNdsAObjEvent32PlanSegmentCount = 0u;
    /* Discarded with the ledger it shadows, in the same breath. */
    sNdsEvent32InterpDescFixedCount = 0u;
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
        NDS_DIAG(gNdsAObjEvent32ColorCorrectionCount++);
    }
}

void gcPlayMObjMatAnim(MObj *mobj) __attribute__((section(".itcm")));
void gcPlayMObjMatAnim(MObj *mobj)
{
    const u32 mul = gNdsMObjTickMul;
    sb32 was_active;

    if (mul == 0u)
    {
        return;
    }
    was_active = ((mobj != NULL) &&
                  NDS_FCMP_NE_C(mobj->anim_wait, AOBJ_ANIM_NULL));
    if ((mul == 1u) || (mobj == NULL))
    {
        ndsBaseGcPlayMObjMatAnim(mobj);
    }
    else
    {
        ndsMObjStepScaled(mobj, mul, TRUE);
    }
    if (was_active != FALSE)
    {
        ndsAObjEvent32CorrectMObjColors(mobj, TRUE);
    }
}

/* gcPlayMObjMatAnim at the source rate (see ndsGcParseMObjMatAnimJointNow). */
void ndsGcPlayMObjMatAnimNow(MObj *mobj)
{
    sb32 was_active = ((mobj != NULL) &&
                       NDS_FCMP_NE_C(mobj->anim_wait, AOBJ_ANIM_NULL));

    ndsBaseGcPlayMObjMatAnim(mobj);
    if (was_active != FALSE)
    {
        ndsAObjEvent32CorrectMObjColors(mobj, TRUE);
    }
}

/* Every MObj of the tree whose animation is live, in tree order. Stores the
 * first `capacity` and returns the total, so a caller whose buffer was large
 * enough walks the tree once (P2-2p8, 2026-10-04: the count-then-collect pair
 * walked every DObj and MObj twice per gcPlayAnimAll, ~14K cycles a frame). */
static u32 ndsAObjEvent32CollectActiveMObjs(GObj *gobj, MObj **active_mobjs,
                                            u32 capacity)
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
                if (count < capacity)
                {
                    active_mobjs[count] = mobj;
                }
                count++;
            }
        }
    }
    return count;
}

/* P2-2p8 N04.08. A zero-speed MObj whose anim_wait was already positive before
 * gcParseMObjMatAnimJoint is stable for this call: the parser still runs, so its
 * exact float and state effects are preserved, but it returns before consuming
 * an event, and the material player would then only add +/-0 to each AObj length
 * and recompute byte-identical outputs. Skipping that player call is therefore
 * work removal, not a behaviour change.
 *
 * TWO INVARIANTS CARRY THAT CLAIM, and breaking either one makes this unsafe:
 *
 *   1. gcPlayAnimAll's unconditional ndsAObjEvent32CorrectMObjColors(mobj,
 *      FALSE) pass below must keep running over every MObj. The player and that
 *      pass are the only writers of the five colour tracks in the whole tree,
 *      and the pass re-derives all five from the live AObj chain, so it -- not
 *      this skip's own reasoning -- is what makes colour safe. Narrowing it
 *      narrows this.
 *   2. The ten scalar material tracks (13..22) have no such restore pass, so
 *      they are safe only while no non-player writer touches a track that also
 *      carries a live matanim AObj on the same MObj. The gameplay setters that
 *      do write texture_id_curr/next and palette_id target MObjs with no
 *      matanim on that track, and the renderer's MOBJ_FLAG_FRAC path is
 *      idempotent because lfrac keeps the value the last player call wrote.
 *
 * Engagement is stage-local, and deliberately so. MObjs start at anim_speed
 * 1.0f and only an explicit speed setter can zero them; the single MObj-targeted
 * one in the tree is Dream Land's frozen water (battleship_grpupupu_ground.c),
 * which bit-pins these exact fields in its freeze fingerprint before zeroing the
 * speed. So this removes the replay of an animation the port already froze on
 * purpose, and on a stage with no frozen material animation it fires zero times
 * and costs one predicate. Do not bank its gate movement as a whole-roster or
 * whole-stage win.
 *
 * Direct gcPlayMObjMatAnim callers are deliberately NOT changed: the argument
 * depends on the parser->player ordering that only gcPlayAnimAll owns. Reaching
 * between those two calls is also why this is an explicit copy of the decomp
 * gcPlayAnimAll traversal rather than a call to it -- the decision point is
 * inside that loop. The DObj side calls the same player the decomp body would:
 * the gcPlayDObjAnimJoint -> ndsBaseGcPlayDObjAnimJoint rename at the top of
 * this file is conditional, so this mirrors its condition exactly.
 *
 * A census measured 7,892 of 14,059 active calls (56.1%) and 47,352 of 60,263
 * live nodes (78.6%) in this state, and a same-ROM A/B against this same
 * traversal with the skip disabled attributed WORK-H P50 -8,640 and GCRA P50
 * -5,440 ticks to it. Evidence:
 * artifacts/performance/2026-09-16_p2-2p8-mobj-stable-skip/.
 *
 * gNdsMObjMatAnimStableSkipCount is permanent engagement proof, not scaffolding.
 * A skip that silently stops firing -- because a material animation's speed or
 * wait convention changes -- is otherwise indistinguishable from one that fires
 * and saves nothing, and this campaign has already shipped that mistake once. */
volatile u32 gNdsMObjMatAnimStableSkipCount;

static inline sb32 ndsMObjMatAnimWasStableZero(const MObj *mobj)
{
    u32 speed_bits;
    u32 wait_bits;

    if (mobj == NULL)
    {
        return FALSE;
    }
    /* Exactly the two zero encodings, +0.0f and -0.0f. Any non-zero mantissa
     * survives the shift, so denormals and NaN speeds are rejected, and
     * x + (-0.0f) carries the same bit identity as x + (+0.0f). */
    speed_bits = ndsFcmpBits(mobj->anim_speed);

    /* Positive, finite and non-zero. This has to agree with the parser's own
     * `anim_wait > 0.0F` early return rather than with NDS_FCMP_GT0, which
     * orders a positive NaN above zero where the float compare does not: on a
     * NaN wait the parser would fall through and consume an event while the
     * skip suppressed the player, which is the one input that can make the two
     * disagree. +Inf is excluded with it; refusing to skip is always safe.
     * All three AObj sentinels are strictly negative (AOBJ_ANIM_NULL is
     * F32_MIN, CHANGED F32_MIN/2, END F32_MIN/3), so this also excludes every
     * sentinel edge, and the END -> NULL transition can never be skipped. */
    wait_bits = ndsFcmpBits(mobj->anim_wait);

    return (((speed_bits << 1) == 0u) && ((wait_bits - 1u) < 0x7f7fffffu)) ?
        TRUE : FALSE;
}

#if NDS_P2_STAGE_YOSTER
/* P2-2p8 (2026-09-28). Yoshi's Island's three cloud GObjs run gcPlayAnimAll
 * every tick over AnimJoint 0x1E0, whose tracks drive the root's and the three
 * mids' rotation and scale. gryoster.c builds those DObjs with
 * nGCMatrixKindTra alone ("Make this nGCMatrixKindTraRotRpyRSca to see cloud
 * scale animation"), so no matrix -- source or port -- reads the values, and
 * no gameplay code does: gryoster.c reads only the root's translate and the
 * drawables' MObj anim_wait, collision uses the yakumono DObjs, and the replay
 * digest folds no stage DObj. The cubic evaluation was ~30K ticks a frame
 * (about 60 evaluations of 13 fmul + 9 fadd, soft float).
 *
 * For such a DObj the player below keeps everything the source player does to
 * state -- each live AObj's `length` advance and the END -> NULL step -- and
 * drops only the value writes nobody reads. Any live translation track or any
 * other matrix kind takes the source player. Only the clouds reach it:
 * battleship_gryoster_ground.c renames gryoster.c's gcPlayAnimAll to
 * ndsGRYosterCloudPlayAnimAll, so no other GObj pays a test and the ITCM
 * gcPlayAnimAll is unchanged. */
volatile u32 gNdsGcDObjTraOnlySkips;

static sb32 ndsGcDObjAnimValuesUnread(const DObj *dobj)
{
    const AObj *aobj;

    if ((dobj->anim_wait == AOBJ_ANIM_NULL) ||
        (dobj->xobjs_num != 1) || (dobj->xobjs[0] == NULL) ||
        (dobj->xobjs[0]->kind != nGCMatrixKindTra))
    {
        return FALSE;
    }
    for (aobj = dobj->aobj; aobj != NULL; aobj = aobj->next)
    {
        if ((aobj->kind != nGCAnimKindNone) &&
            ((aobj->track < nGCAnimTrackRotX) ||
             ((aobj->track > nGCAnimTrackRotZ) &&
              (aobj->track < nGCAnimTrackScaX)) ||
             (aobj->track > nGCAnimTrackScaZ)))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* decomp objanim.c gcPlayDObjAnimJoint without the value computation. */
static void ndsGcAdvanceDObjAnimJoint(DObj *dobj)
{
    AObj *aobj;

    if (dobj->anim_wait != AOBJ_ANIM_END)
    {
        for (aobj = dobj->aobj; aobj != NULL; aobj = aobj->next)
        {
            if (aobj->kind == nGCAnimKindNone)
            {
                continue;
            }
#if NDS_R2_CUBIC_FIXED
            /* A Q node (the stage parser's since 2026-10-05) advances in Q. */
            if (aobj->kind >= NDS_R2_AQ_KIND_BASE)
            {
                aobj->length = ndsR2AQStore(ndsR2AQLoad(aobj->length) +
                    ndsR2F32ToFixed(dobj->anim_speed, NDS_R2_AQ_LF));
                continue;
            }
#endif
            aobj->length += dobj->anim_speed;
        }
    }
    else
    {
        dobj->anim_wait = AOBJ_ANIM_NULL;
    }
    NDS_DIAG(gNdsGcDObjTraOnlySkips++);
}
#endif

/* `fixed_cubic` is a compile-time constant at every caller, so the ITCM
 * gcPlayAnimAll (FALSE) folds the arm away; only ndsGcPlayAnimAllFixedCubic
 * below, in main RAM, carries it. */
static inline __attribute__((always_inline)) void
ndsGcPlayAnimAllStableSkip(GObj *gobj, sb32 tra_only, sb32 fixed_cubic,
                           u32 mul)
{
    DObj *dobj = (gobj != NULL) ? DObjGetStruct(gobj) : NULL;

    (void)tra_only;
    (void)fixed_cubic;
    while (dobj != NULL)
    {
        MObj *mobj;

        gcParseDObjAnimJoint(dobj);
#if NDS_P2_STAGE_YOSTER
        if ((tra_only != FALSE) && (ndsGcDObjAnimValuesUnread(dobj) != FALSE))
        {
            ndsGcAdvanceDObjAnimJoint(dobj);
        }
        else
#endif
#if !NDS_R2_ANIM_CENSUS && NDS_R2_CUBIC_FIXED
        if (fixed_cubic != FALSE)
        {
            gcPlayDObjAnimJoint(dobj);
        }
        else
        {
            ndsBaseGcPlayDObjAnimJoint(dobj);
        }
#elif NDS_R2_ANIM_CENSUS || NDS_R2_CUBIC_FIXED
        ndsBaseGcPlayDObjAnimJoint(dobj);
#else
        gcPlayDObjAnimJoint(dobj);
#endif

        /* mul 0: a batch's earlier tick, no material work (see
         * gNdsMObjTickMul). */
        for (mobj = (mul != 0u) ? dobj->mobj : NULL; mobj != NULL;
             mobj = mobj->next)
        {
            sb32 was_stable_zero = ndsMObjMatAnimWasStableZero(mobj);

            if (mul == 1u)
            {
                ndsBaseGcParseMObjMatAnimJoint(mobj);
            }
            else
            {
                ndsMObjStepScaled(mobj, mul, FALSE);
            }
            if (was_stable_zero != FALSE)
            {
                NDS_DIAG(gNdsMObjMatAnimStableSkipCount++);
            }
            else if (mul == 1u)
            {
                ndsBaseGcPlayMObjMatAnim(mobj);
            }
            else
            {
                ndsMObjStepScaled(mobj, mul, TRUE);
            }
        }
        dobj = gcGetTreeDObjNext(dobj);
    }
}

/* The MObjs live before the play, collected in one walk into this many slots
 * on the stack; a tree with more takes ndsGcPlayAnimAllLarge. */
#define NDS_AOBJ_ACTIVE_LOCAL 24u

static inline __attribute__((always_inline)) void
ndsGcPlayAnimAllFinish(GObj *gobj, MObj **active_mobjs, u32 active_count);

/* More live MObjs than the stack slots hold: collect them again into a buffer
 * of the exact size (the walk the old count-then-collect pair always made). */
static void __attribute__((noinline, cold))
ndsGcPlayAnimAllLarge(GObj *gobj, sb32 tra_only, sb32 fixed_cubic,
                      u32 active_count, u32 mul)
{
    MObj *active_mobjs[active_count];

    (void)ndsAObjEvent32CollectActiveMObjs(gobj, active_mobjs, active_count);
    ndsGcPlayAnimAllStableSkip(gobj, tra_only, fixed_cubic, mul);
    ndsGcPlayAnimAllFinish(gobj, active_mobjs, active_count);
}

static inline __attribute__((always_inline)) void
ndsGcPlayAnimAllBody(GObj *gobj, sb32 tra_only, sb32 fixed_cubic, u32 mul)
{
    MObj *active_mobjs[NDS_AOBJ_ACTIVE_LOCAL];
    u32 active_count;

    if (mul == 0u)
    {
        /* The DObjs only: no MObj collect, play or colour pass. */
        ndsGcPlayAnimAllStableSkip(gobj, tra_only, fixed_cubic, 0u);
        return;
    }
    active_count = ndsAObjEvent32CollectActiveMObjs(
        gobj, active_mobjs, NDS_AOBJ_ACTIVE_LOCAL);
    if (active_count > NDS_AOBJ_ACTIVE_LOCAL)
    {
        ndsGcPlayAnimAllLarge(gobj, tra_only, fixed_cubic, active_count, mul);
        return;
    }
    ndsGcPlayAnimAllStableSkip(gobj, tra_only, fixed_cubic, mul);
    ndsGcPlayAnimAllFinish(gobj, active_mobjs, active_count);
}

/* After the play: the unconditional colour pass over every MObj, then the
 * objects whose animation crossed END this call. */
static inline __attribute__((always_inline)) void
ndsGcPlayAnimAllFinish(GObj *gobj, MObj **active_mobjs, u32 active_count)
{
    DObj *dobj;
    u32 i;

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

/* P2-2p8 (2026-10-05, owner: "Software floating point should not exist, fixed
 * point only"; ruling D13 re-baselines the digest): every stage and item DObj
 * now takes the port player, whose float AObjs evaluate the cubic in fixed
 * point (E64) -- until today only the Master Hand wallpaper did (below). The
 * float census put the decomp body at 2.3K ticks a frame on Dream Land and
 * 6.7K on Sector Z. */
void gcPlayAnimAll(GObj *gobj) __attribute__((section(".itcm")));
void gcPlayAnimAll(GObj *gobj)
{
    ndsGcPlayAnimAllBody(gobj, FALSE, TRUE, gNdsMObjTickMul);
}

#if NDS_P2_STAGE_YOSTER
/* Yoshi's Island's cloud GObjs (see ndsGcDObjAnimValuesUnread). */
void ndsGRYosterCloudPlayAnimAll(GObj *gobj)
{
    /* Source rate: gryoster.c reads these MObjs' anim_wait. */
    ndsGcPlayAnimAllBody(gobj, TRUE, FALSE, 1u);
}
#endif

/* P2-6 (2026-10-02). The Master Hand stage's background is ~24 boss
 * wallpaper GObjs (sc1pgameboss.c:865, link nGCCommonLinkIDWallpaperEffect),
 * 4-8 animated DObjs each, and gcPlayAnimAll played them through the decomp's
 * float body: 40K ticks a frame of soft float, the largest single caller in
 * that battle. Nothing but their own display reads those poses, so
 * battleship_sc1pgameboss.c renames that TU's gcPlayAnimAll (it plays nothing
 * else) to this, which takes the port player: its float AObjs evaluate the
 * cubic in fixed point (E64, owner-authorized for non-fighter DObjs). The
 * parse, every `length` advance and the END -> NULL step are the same code
 * either way; only the drawn values round differently. */
void ndsGcPlayAnimAllFixedCubic(GObj *gobj)
{
    ndsGcPlayAnimAllBody(gobj, FALSE, TRUE, gNdsMObjTickMul);
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

    NDS_DIAG(gNdsR2AddDObjAnimCalls++);
    admit = ((anim_joint == NULL) ||
             (ndsRelocPointerIsFighterAObj16(anim_joint) != FALSE) ||
             (ndsAObjEvent32NormalizeScript(
                  anim_joint, nNDSAObjEvent32OwnerDObj) != FALSE)) ? TRUE : FALSE;
    NDS_DIAG(gNdsR2AddDObjNormalizeTicks += cpuGetTiming() - phase);
    if (admit != FALSE)
    {
        phase = cpuGetTiming();
        ndsBaseGcAddDObjAnimJoint(dobj, anim_joint, anim_frame);
        NDS_DIAG(gNdsR2AddDObjBaseTicks += cpuGetTiming() - phase);
    }
    else
    {
        ndsBaseGcAddDObjAnimJoint(dobj, NULL, anim_frame);
        NDS_DIAG(gNdsAObjEvent32DetachCount++);
    }
    {
        u32 total = cpuGetTiming() - enter;

        NDS_DIAG(gNdsR2AddDObjAnimTicks += total);
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
    else
    {
        /* S1: a refused script must not leave the joint's previous one in
         * place. For a fighter that pointer is into the figatree buffer the
         * new motion just overwrote; parsing it read a float as a command
         * pointer. The joint plays no script instead. */
        ndsBaseGcAddDObjAnimJoint(dobj, NULL, anim_frame);
        NDS_DIAG(gNdsAObjEvent32DetachCount++);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        ndsLabNoteDetach(dobj, anim_joint, __builtin_return_address(0));
#endif
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
    else
    {
        ndsBaseGcAddMObjMatAnimJoint(mobj, NULL, anim_frame);
        NDS_DIAG(gNdsAObjEvent32DetachCount++);
    }
}

/* Witnesses for the silent-refusal arm below. `used` and volatile so
 * --gc-sections keeps them and a debugger reads them without a consumer. */
__attribute__((used)) volatile u32 gNdsGcAddAnimJointAllRefusedCount;
__attribute__((used)) volatile u32 gNdsGcAddAnimJointAllRefusedLastGObj;
__attribute__((used)) volatile u32 gNdsGcAddAnimJointAllRefusedLastTable;

void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints,
                       f32 anim_frame)
{
#if NDS_R2_LOADFRAME_TIMING
    /* The whole-GObj variant, which is what a fighter action change goes through:
     * it walks the DObj tree and re-adds every joint's animation. If the action
     * change is the excursion, this is where it should show. */
    u32 enter = cpuGetTiming();

    NDS_DIAG(gNdsR2AddAnimAllCalls++);
#endif
    if ((gobj != NULL) &&
        (ndsAObjEvent32NormalizeDObjTable(gobj, anim_joints) != FALSE))
    {
        ndsBaseGcAddAnimJointAll(gobj, anim_joints, anim_frame);
    }
    else if (gobj != NULL)
    {
        /* A REFUSED INSTALL IS INVISIBLE, AND SOMETHING IS LIVING IN IT.
         *
         * When the normalizer refuses any one entry of the table this wrapper
         * silently installs NOTHING, and the caller has no way to tell that
         * from a successful install. The object then keeps whatever pose it
         * already held, forever, while everything around it -- its state
         * machine, its collision, its sound -- carries on normally. That is
         * indistinguishable, on screen, from an object that is simply frozen.
         *
         * Saffron's gate is the open case. gNdsAObjEvent32NormalizeFailCount
         * already counts the inner refusal, but it is global and pools every
         * caller in the scene, so it cannot say whether the gate's own
         * open/close install was the one that failed. `Last` is overwritten
         * rather than latched on purpose: the question is what refused most
         * recently, not what refused first. */
        NDS_DIAG(gNdsGcAddAnimJointAllRefusedCount++);
        gNdsGcAddAnimJointAllRefusedLastGObj = (u32)(uintptr_t)gobj;
        gNdsGcAddAnimJointAllRefusedLastTable = (u32)(uintptr_t)anim_joints;
    }
#if NDS_R2_LOADFRAME_TIMING
    {
        u32 total = cpuGetTiming() - enter;

        NDS_DIAG(gNdsR2AddAnimAllTicks += total);
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
