/* What one operation on the ARM9 divide and square-root units actually costs,
 * measured on the machine rather than read off GBATEK, and whether it returns
 * the same answer as the software form it would replace.
 *
 * WHY A MICROBENCHMARK AND NOT A WHOLE-MATCH A/B. The question here is a
 * PRICE PER OPERATION for four primitives, not a gate delta for a shipped
 * change. A whole-match arm would cost two builds and two hour-long runs to
 * answer it, would answer it only for the one lane that happens to call the
 * primitive, and -- for the collision ring, which is the lane that named these
 * hooks -- would be spent on a route already measured closed at 2.68x with a
 * lane ceiling of 15,217 tk/fr at rank-80 against a +94,481 requirement. The
 * per-operation price is the transferable number: it re-prices EXCHANGE.md's
 * measured __udivmoddi4 row directly, and it is also the engine cost of
 * SIMSIDE.md's column N, which is a different lane entirely.
 *
 * WHAT THIS PRICE IS AND IS NOT. Every arm runs its kernel back to back in a
 * tight loop, so the instruction cache is fully warm and the branch history is
 * ideal. That is the RIGHT basis for "which of these two implementations of the
 * same operation is cheaper", because both arms get the same favour, and it is
 * the WRONG basis for "what will this cost in situ" -- a cold kernel entered a
 * few times a frame pays compulsory fetch on top, which this campaign has
 * measured at 23-51 cycles per line and has twice seen invert a win. Read every
 * number below as a lower bound on the in-situ cost and an upper bound on the
 * saving.
 *
 * THE BASELINE ARM IS NOT OPTIONAL. gNdsR2HwMathBenchBaseTicks times the loop,
 * the operand generator and the sink store with no kernel at all. Every other
 * arm's honest cost is its own ticks minus that one. Reporting an arm's raw
 * ticks would charge each kernel for an LCG it does not have.
 *
 * cpuGetTiming() ticks are the project's own unit -- the same clock the tick
 * HUD's buckets and the +94,481 requirement are quoted in -- so a figure here
 * is directly comparable with SIMSIDE.md section 3's tk/call column
 * (__aeabi_fdiv 59.19, sqrtf 144.62, __aeabi_fmul 13.23).
 *
 * LAB ONLY. NDS_R2_HWMATH_BENCH defaults to 0 and at 0 this translation unit is
 * not linked at all, so a published ROM is byte-identical with or without it.
 */

#include "nds_scene_harness_config.h"

#if NDS_R2_HWMATH_BENCH

#include <math.h>
#include <nds.h>

#include <nds/nds_r2_collision_fixed.h>
#include <nds/nds_r2_hwmath_bench.h>
#include <nds/nds_r2_hwmath_unit.h>

/* The algorithm header wants this primitive by name, exactly as the ROM's own
 * conversion would supply it. */
/* The FAST 64/32 form, i.e. SM64DS's sequence. Build c210 measured the f32
 * divide on the leading-poll form at 136.5 tk against __aeabi_fdiv's 71.5, and
 * measured the leading poll itself at 41.0 tk in the 64/64 mode. Rather than
 * subtract one measurement from another and call the difference a price -- the
 * shape recorded as "a residual divided by a count is not a price" -- the
 * kernel is rebuilt on the cheaper primitive and re-timed directly. */
/* NO POINTER CAST. The first version of this wrapper wrote
 * `(int32_t *)remainder_out` on an `int *`, and on devkitARM int32_t is `long`,
 * so that was a strict-aliasing violation which cost 110 wrong IEEE quotients
 * in build-c210-hwmath. The types now match end to end; see the note in
 * include/nds/nds_r2_hwmath.h.
 *
 * The LEADING-POLL 64/32 form is deliberately back: c210 measured the f32
 * divide on it and produced those 110, and the point of this build is to
 * re-run that exact arm with the defect removed. Its price is also the one
 * that matters, because the poll is what the in-tree idiom carries. */
static int64_t ndsR2HwMathDivideUnit(int64_t numerator, int32_t denominator,
                                     int32_t *remainder_out)
{
    return ndsR2HwMathDivide6432(numerator, denominator, remainder_out);
}

#include <nds/nds_r2_hwmath.h>

/* The ARM-state square root. src/nds/r2/nds_r2_sqrtf.c ships the identical
 * arithmetic but its object is built -mthumb, and ARMv5TE Thumb has no UMULL,
 * so nds_r2_sqrtf.h's 48-bit `root * root` becomes a call to __aeabi_lmul --
 * visible at 0x0208b10c in build-c206-shipgx0's own disassembly. This arm is
 * that same header compiled in an object the Makefile builds -marm, so the
 * difference between the two arms is exactly the price of that instruction
 * selection and nothing else. */
static unsigned int ndsR2SqrtfHardware(unsigned long long value)
{
    unsigned int ime = REG_IME;
    unsigned int result;

    REG_IME = 0u;
    result = ndsR2HwMathSqrt64Lead(value);
    REG_IME = ime;
    return result;
}

#include <nds/nds_r2_sqrtf.h>

extern float __ieee754_sqrtf(float x);

static float ndsR2HwMathBenchSqrtfArm(float x)
{
    unsigned int bits;
    unsigned int out;

    __builtin_memcpy(&bits, &x, sizeof(bits));
    if (ndsR2SqrtfBits(bits, &out) == NDS_R2_SQRTF_FALLBACK)
    {
        return __ieee754_sqrtf(x);
    }
    __builtin_memcpy(&x, &out, sizeof(x));
    return x;
}

/* ------------------------------------------------------------------------
 * Published counters. `used` because --gc-sections collects a global whose only
 * consumer is a debugger, which is how a Boundary profile once went red on
 * "Missing ELF symbol" after dead code was compiled out.
 */

volatile u32 gNdsR2HwMathBenchRan __attribute__((used));
volatile u32 gNdsR2HwMathBenchIters __attribute__((used));
volatile u32 gNdsR2HwMathBenchGradeIters __attribute__((used));

volatile u32 gNdsR2HwMathBenchBaseTicks __attribute__((used));
volatile u32 gNdsR2HwMathBenchDivSoftTicks __attribute__((used));
volatile u32 gNdsR2HwMathBenchDivLeadTicks __attribute__((used));
volatile u32 gNdsR2HwMathBenchDivFastTicks __attribute__((used));
volatile u32 gNdsR2HwMathBenchSqrtSoftTicks __attribute__((used));
volatile u32 gNdsR2HwMathBenchSqrtLeadTicks __attribute__((used));
volatile u32 gNdsR2HwMathBenchSqrtFastTicks __attribute__((used));
volatile u32 gNdsR2HwMathBenchFdivSoftTicks __attribute__((used));
volatile u32 gNdsR2HwMathBenchFdivHwTicks __attribute__((used));
volatile u32 gNdsR2HwMathBenchSqrtfShipTicks __attribute__((used));
volatile u32 gNdsR2HwMathBenchSqrtfArmTicks __attribute__((used));

/* Equivalence. Each is a count of DISAGREEMENTS with the software form on the
 * same operand, so 0 is the claim and any other value is the retraction. */
volatile u32 gNdsR2HwMathBenchDivMismatch __attribute__((used));
volatile u32 gNdsR2HwMathBenchDivFastMismatch __attribute__((used));
volatile u32 gNdsR2HwMathBenchSqrtMismatch __attribute__((used));
volatile u32 gNdsR2HwMathBenchSqrtFastMismatch __attribute__((used));
volatile u32 gNdsR2HwMathBenchFdivMismatch __attribute__((used));
volatile u32 gNdsR2HwMathBenchFdivFallback __attribute__((used));
volatile u32 gNdsR2HwMathBenchSqrtfMismatch __attribute__((used));

/* SPLIT COUNTERS for the 64/32 mode specifically. The first run of this bench
 * read FdivMismatch = 110 of 65,536 while every 64/64 divide and every root
 * matched exactly, so the disagreement is inside DIV_64_32 and it is either the
 * quotient or the remainder. These two grade each half of that mode separately
 * against software on the identical operand, which is the only way to say which
 * -- and 110 is close to half of the ~192 operands per 65,536 whose low bits
 * land exactly on the rounding half, which is the signature a wrong REMAINDER
 * would produce and a wrong quotient would not. Predicted before the run. */
volatile u32 gNdsR2HwMathBenchQuotMismatch __attribute__((used));
volatile u32 gNdsR2HwMathBenchRemMismatch __attribute__((used));
/* THE SAME OPERAND, THE SAME MODE, THE LEADING POLL PUT BACK. Build c211 moved
 * the f32 kernel from the leading-poll 64/32 primitive to SM64DS's and the 110
 * disagreements went to 0 -- but it also added the split counters, so two
 * things changed and the attribution was not earned. These two isolate the poll
 * itself: same 65,536 operands, same DIV_64_32 mode, same remainder read, one
 * `while (DIVCNT & BUSY)` apart. If the leading form is the cause, this reads
 * non-zero here and the finding is a stale read in the sequence libnds's
 * div64/sqrt64, nds_renderer.c and battleship_gmcamera.c ALL use. */
volatile u32 gNdsR2HwMathBenchQuotLeadMismatch __attribute__((used));
volatile u32 gNdsR2HwMathBenchRemLeadMismatch __attribute__((used));
volatile u32 gNdsR2HwMathBenchHalfCases __attribute__((used));
volatile u32 gNdsR2HwMathBenchFdivFirstA __attribute__((used));
volatile u32 gNdsR2HwMathBenchFdivFirstB __attribute__((used));
volatile u32 gNdsR2HwMathBenchFdivFirstGot __attribute__((used));
volatile u32 gNdsR2HwMathBenchFdivFirstWant __attribute__((used));

/* The controls. An equivalence counter that reads 0 proves nothing unless the
 * population it graded was non-empty and non-trivial, so each class publishes
 * how many operands it actually compared and how many of them were negative
 * (the divide) or rounded (the f32 divide). */
volatile u32 gNdsR2HwMathBenchDivGraded __attribute__((used));
volatile u32 gNdsR2HwMathBenchDivNegative __attribute__((used));
volatile u32 gNdsR2HwMathBenchSqrtGraded __attribute__((used));
volatile u32 gNdsR2HwMathBenchFdivGraded __attribute__((used));
volatile u32 gNdsR2HwMathBenchSqrtfGraded __attribute__((used));

/* External linkage and NOT volatile, for the reason
 * src/port/nds_r2_sim_mac_fixed.c gives: the result has to be observably stored
 * so -O2 cannot delete the arithmetic being priced, and a plain store costs one
 * instruction where a volatile one costs a load and a store. */
u32 gNdsR2HwMathBenchSink __attribute__((used));

#define NDS_R2_HWMATH_BENCH_ITERS 4096u
#define NDS_R2_HWMATH_BENCH_GRADE 65536u
#define NDS_R2_HWMATH_BENCH_SEED  0x5f3759dfu

static inline u32 ndsR2HwMathBenchNext(u32 *state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

/* The ring's own operand shapes, so the price is the price of the real call and
 * not of a convenient one.
 *
 *   divide      2^52 / d with |d| in [2^22, 2^30) -- the numerator
 *               nds_r2_collision_fixed.h:546 and :729 both use, and the
 *               denominator range its s^2 and |det| guards admit. Both signs.
 *   square root s2 << 22 with s2 in [2^22, 2^30] -- literally :554.
 *   f32 divide  normal operands across a wide exponent span, which is what
 *               gmCollisionTestRectangle's three fdiv per entry see. */
static inline s64 ndsR2HwMathBenchDenominator(u32 word)
{
    s64 magnitude = (s64)(s32)((word & 0x3fffffffu) | 0x00400000u);

    return ((word & 0x80000000u) != 0u) ? -magnitude : magnitude;
}

static inline u64 ndsR2HwMathBenchRadicand(u32 word)
{
    return (u64)(u32)((word & 0x3fffffffu) | 0x00400000u) << 22;
}

static inline u32 ndsR2HwMathBenchFloatBits(u32 word)
{
    /* Exponent 96..159. The span is deliberately narrow enough that every
     * quotient of two such operands is itself a normal -- ea-eb is in
     * [-63, 63], so the result exponent lands in [63, 190] and the decline
     * branch is unreachable. The timed arm therefore prices the answered case
     * rather than a mixture, and gNdsR2HwMathBenchFdivFallback reading 0 is the
     * proof of that rather than an assumption. */
    return (word & 0x807fffffu) | ((96u + ((word >> 24) & 0x3fu)) << 23);
}

void ndsR2HwMathBenchRun(void)
{
    const s64 numerator = (s64)1 << 52;
    u32 state;
    u32 i;
    u32 start;
    u32 sink = 0u;

    if (gNdsR2HwMathBenchRan != 0u)
    {
        return;
    }

    /* Warm every path once so no arm pays first-touch fetch for the others. */
    for (i = 0u; i < 64u; i++)
    {
        state = NDS_R2_HWMATH_BENCH_SEED + i;
        sink += (u32)(s32)(numerator /
                           ndsR2HwMathBenchDenominator(state));
        sink += (u32)ndsR2HwMathCfxDiv64(numerator,
                                         ndsR2HwMathBenchDenominator(state));
        sink += (u32)ndsR2HwMathDivideFast(numerator,
                                           ndsR2HwMathBenchDenominator(state));
        sink += ndsR2CfxIsqrt64Portable(ndsR2HwMathBenchRadicand(state));
        sink += ndsR2HwMathSqrt64Lead(ndsR2HwMathBenchRadicand(state));
        sink += ndsR2HwMathSqrt64Fast(ndsR2HwMathBenchRadicand(state));
    }
    gNdsR2HwMathBenchSink = sink;

#define NDS_R2_HWMATH_BENCH_ARM(counter, body)                                \
    do                                                                        \
    {                                                                         \
        state = NDS_R2_HWMATH_BENCH_SEED;                                     \
        sink = 0u;                                                            \
        start = cpuGetTiming();                                               \
        for (i = 0u; i < NDS_R2_HWMATH_BENCH_ITERS; i++)                      \
        {                                                                     \
            u32 word = ndsR2HwMathBenchNext(&state);                          \
            body;                                                             \
        }                                                                     \
        (counter) = cpuGetTiming() - start;                                   \
        gNdsR2HwMathBenchSink += sink;                                        \
    } while (0)

    NDS_R2_HWMATH_BENCH_ARM(gNdsR2HwMathBenchBaseTicks, sink += word);

    NDS_R2_HWMATH_BENCH_ARM(
        gNdsR2HwMathBenchDivSoftTicks,
        sink += (u32)(s32)(numerator / ndsR2HwMathBenchDenominator(word)));
    NDS_R2_HWMATH_BENCH_ARM(
        gNdsR2HwMathBenchDivLeadTicks,
        sink += (u32)ndsR2HwMathCfxDiv64(numerator,
                                         ndsR2HwMathBenchDenominator(word)));
    NDS_R2_HWMATH_BENCH_ARM(
        gNdsR2HwMathBenchDivFastTicks,
        sink += (u32)ndsR2HwMathDivideFast(numerator,
                                           ndsR2HwMathBenchDenominator(word)));

    NDS_R2_HWMATH_BENCH_ARM(
        gNdsR2HwMathBenchSqrtSoftTicks,
        sink += ndsR2CfxIsqrt64Portable(ndsR2HwMathBenchRadicand(word)));
    NDS_R2_HWMATH_BENCH_ARM(
        gNdsR2HwMathBenchSqrtLeadTicks,
        sink += ndsR2HwMathSqrt64Lead(ndsR2HwMathBenchRadicand(word)));
    NDS_R2_HWMATH_BENCH_ARM(
        gNdsR2HwMathBenchSqrtFastTicks,
        sink += ndsR2HwMathSqrt64Fast(ndsR2HwMathBenchRadicand(word)));

    NDS_R2_HWMATH_BENCH_ARM(gNdsR2HwMathBenchFdivSoftTicks, {
        u32 a_bits = ndsR2HwMathBenchFloatBits(word);
        u32 b_bits = ndsR2HwMathBenchFloatBits(word * 2654435761u);
        float a;
        float b;
        float q;

        __builtin_memcpy(&a, &a_bits, sizeof(a));
        __builtin_memcpy(&b, &b_bits, sizeof(b));
        q = a / b;
        __builtin_memcpy(&a_bits, &q, sizeof(a_bits));
        sink += a_bits;
    });
    NDS_R2_HWMATH_BENCH_ARM(gNdsR2HwMathBenchFdivHwTicks, {
        u32 a_bits = ndsR2HwMathBenchFloatBits(word);
        u32 b_bits = ndsR2HwMathBenchFloatBits(word * 2654435761u);
        u32 out = 0u;

        (void)ndsR2HwMathDivBits(a_bits, b_bits, &out);
        sink += out;
    });

    NDS_R2_HWMATH_BENCH_ARM(gNdsR2HwMathBenchSqrtfShipTicks, {
        u32 a_bits = ndsR2HwMathBenchFloatBits(word) & 0x7fffffffu;
        float a;
        float r;

        __builtin_memcpy(&a, &a_bits, sizeof(a));
        r = sqrtf(a);
        __builtin_memcpy(&a_bits, &r, sizeof(a_bits));
        sink += a_bits;
    });
    NDS_R2_HWMATH_BENCH_ARM(gNdsR2HwMathBenchSqrtfArmTicks, {
        u32 a_bits = ndsR2HwMathBenchFloatBits(word) & 0x7fffffffu;
        float a;
        float r;

        __builtin_memcpy(&a, &a_bits, sizeof(a));
        r = ndsR2HwMathBenchSqrtfArm(a);
        __builtin_memcpy(&a_bits, &r, sizeof(a_bits));
        sink += a_bits;
    });

#undef NDS_R2_HWMATH_BENCH_ARM

    /* ------------------------------------------------------------------
     * Equivalence, untimed, on a larger stream. This is the half the host
     * falsifier cannot do: scripts/check-r2-hwmath.c proves the ALGORITHM with
     * the unit modelled by a C divide, and this proves the UNIT.
     */
    state = NDS_R2_HWMATH_BENCH_SEED ^ 0x9e3779b9u;
    for (i = 0u; i < NDS_R2_HWMATH_BENCH_GRADE; i++)
    {
        u32 word = ndsR2HwMathBenchNext(&state);
        s64 denominator = ndsR2HwMathBenchDenominator(word);
        u64 radicand = ndsR2HwMathBenchRadicand(word);
        s32 want_div = (s32)(numerator / denominator);
        u32 want_sqrt = ndsR2CfxIsqrt64Portable(radicand);
        u32 a_bits = ndsR2HwMathBenchFloatBits(word);
        u32 b_bits = ndsR2HwMathBenchFloatBits(word * 2654435761u);
        u32 got_fdiv = 0u;
        u32 want_fdiv;
        float a;
        float b;
        float q;

        gNdsR2HwMathBenchDivGraded++;
        if (denominator < 0)
        {
            gNdsR2HwMathBenchDivNegative++;
        }
        if (ndsR2HwMathCfxDiv64(numerator, denominator) != want_div)
        {
            gNdsR2HwMathBenchDivMismatch++;
        }
        if ((s32)ndsR2HwMathDivideFast(numerator, denominator) != want_div)
        {
            gNdsR2HwMathBenchDivFastMismatch++;
        }

        gNdsR2HwMathBenchSqrtGraded++;
        if (ndsR2HwMathSqrt64Lead(radicand) != want_sqrt)
        {
            gNdsR2HwMathBenchSqrtMismatch++;
        }
        if (ndsR2HwMathSqrt64Fast(radicand) != want_sqrt)
        {
            gNdsR2HwMathBenchSqrtFastMismatch++;
        }

        __builtin_memcpy(&a, &a_bits, sizeof(a));
        __builtin_memcpy(&b, &b_bits, sizeof(b));
        q = a / b;
        __builtin_memcpy(&want_fdiv, &q, sizeof(want_fdiv));
        gNdsR2HwMathBenchFdivGraded++;
        if (ndsR2HwMathDivBits(a_bits, b_bits, &got_fdiv) ==
            NDS_R2_HWMATH_FDIV_FALLBACK)
        {
            gNdsR2HwMathBenchFdivFallback++;
        }
        else if (got_fdiv != want_fdiv)
        {
            if (gNdsR2HwMathBenchFdivMismatch == 0u)
            {
                gNdsR2HwMathBenchFdivFirstA = a_bits;
                gNdsR2HwMathBenchFdivFirstB = b_bits;
                gNdsR2HwMathBenchFdivFirstGot = got_fdiv;
                gNdsR2HwMathBenchFdivFirstWant = want_fdiv;
            }
            gNdsR2HwMathBenchFdivMismatch++;
        }

        /* DIV_64_32, both halves, graded separately on the same operand, on the
         * SAME primitive the kernel above used. */
        {
            u32 ma = (a_bits & 0x007fffffu) | 0x00800000u;
            u32 mb = (b_bits & 0x007fffffu) | 0x00800000u;
            u64 wide = (u64)ma << 32;
            u64 want_quotient = wide / mb;
            u32 want_remainder = (u32)(wide % mb);
            s32 hw_remainder = 0;
            s64 hw_quotient = ndsR2HwMathDivide6432Fast((s64)wide, (s32)mb,
                                                        &hw_remainder);
            u32 shift = (ma >= mb) ? 9u : 8u;
            u64 low = want_quotient & (((u64)1 << shift) - 1u);

            if ((u64)hw_quotient != want_quotient)
            {
                gNdsR2HwMathBenchQuotMismatch++;
            }
            if ((u32)hw_remainder != want_remainder)
            {
                gNdsR2HwMathBenchRemMismatch++;
            }
            if (low == ((u64)1 << (shift - 1u)))
            {
                gNdsR2HwMathBenchHalfCases++;
            }

            hw_remainder = 0;
            hw_quotient = ndsR2HwMathDivide6432((s64)wide, (s32)mb,
                                                &hw_remainder);
            if ((u64)hw_quotient != want_quotient)
            {
                gNdsR2HwMathBenchQuotLeadMismatch++;
            }
            if ((u32)hw_remainder != want_remainder)
            {
                gNdsR2HwMathBenchRemLeadMismatch++;
            }
        }

        {
            u32 root_bits = a_bits & 0x7fffffffu;
            float root_in;
            float ship;
            float arm;
            u32 ship_bits;
            u32 arm_bits;

            __builtin_memcpy(&root_in, &root_bits, sizeof(root_in));
            ship = sqrtf(root_in);
            arm = ndsR2HwMathBenchSqrtfArm(root_in);
            __builtin_memcpy(&ship_bits, &ship, sizeof(ship_bits));
            __builtin_memcpy(&arm_bits, &arm, sizeof(arm_bits));
            gNdsR2HwMathBenchSqrtfGraded++;
            if (ship_bits != arm_bits)
            {
                gNdsR2HwMathBenchSqrtfMismatch++;
            }
        }
    }

    gNdsR2HwMathBenchIters = NDS_R2_HWMATH_BENCH_ITERS;
    gNdsR2HwMathBenchGradeIters = NDS_R2_HWMATH_BENCH_GRADE;
    gNdsR2HwMathBenchRan = 1u;
}

#endif /* NDS_R2_HWMATH_BENCH */
