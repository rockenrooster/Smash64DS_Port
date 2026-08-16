/* R2-03 E1 -- correctly-rounded sqrtf on the DS hardware square-root unit.
 *
 * R2-03 E0 measured newlib's __ieee754_sqrtf at 223.1 ticks per call, 64 calls
 * a frame, 14,258 ticks/frame. It is a generic bit-by-bit root running on a
 * core with no FPU, and the DS has a hardware integer square root sitting
 * three registers away (0x040002B0). nds_renderer.c:7988 already uses it, so
 * this is not a new hardware dependency.
 *
 * Bit-exact against newlib for every input, and that is the point rather than
 * a bonus: sqrtf is on the gameplay path -- syVectorMag3D feeds collision and
 * the camera -- so an answer differing in the last bit is a pose differing,
 * and PROJECT_GOAL.md's mechanical equivalence would become a judgement call
 * instead of a hash compare. Correctly rounded means the Task 37 state hash
 * must not move at all.
 *
 * The arithmetic is in include/nds/nds_r2_sqrtf.h so that
 * scripts/check-r2-fixed-sqrt.ps1 exercises this exact code on the host.
 */

#include "nds_scene_harness_config.h"

#include <nds.h>

#include <nds/nds_r2_hwmath_unit.h>

#if NDS_R2_FIXED_SQRT

/* The square-root unit is one set of global registers, and the sequence is
 * write-param / poll-busy / read-result. newlib's sqrtf was pure software and
 * therefore reentrant; this one is not, so an interrupt handler that also took
 * a square root between the write and the read would silently return the wrong
 * value -- exactly the class AGENTS.md calls a failure rather than a risk.
 *
 * nds_renderer.c:7988 uses sqrt64 unguarded and gets away with it because that
 * one call site is not reachable from an ISR. sqrtf is reachable from far more
 * places, including imported audio, so it masks instead of assuming. Two
 * register writes against a ~150-tick saving is not a trade worth thinking
 * about twice.
 *
 * THE MASK IS UNPRICED HEAD-ROOM AND IS DELIBERATELY LEFT IN. The ELF survey in
 * nds_r2_hwmath_unit.h shows this binary has no interrupt-context user of
 * either unit at all -- the port's one registered handler is `sVBlankCount++`
 * -- so the reachability it guards against does not currently exist. Its three
 * I/O accesses cost 698 tk/fr at marginal-80 (111,705 cycles over 80 frames in
 * build-c200-trackprof-off's per-PC profile; 1,294 tk/fr if the register setup
 * around them is charged too). Removing a safety property is an owner call, not
 * an optimisation, and it is recorded rather than taken.
 *
 * ndsR2HwMathSqrt64 rather than libnds's sqrt64: same registers, same 64-bit
 * mode, WITHOUT the leading poll that waits out a stale root nobody reads. Both
 * forms are graded bit-identical over 65,536 operands on four builds. */
static unsigned int ndsR2SqrtfHardware(unsigned long long value)
{
    unsigned int ime = REG_IME;
    unsigned int result;

    REG_IME = 0u;
    result = ndsR2HwMathSqrt64(value);
    REG_IME = ime;
    return result;
}

#include <nds/nds_r2_sqrtf.h>

/* newlib's kernel routine, for the cases the fast path declines. Calling this
 * rather than sqrtf avoids recursing into the function being defined. */
extern float __ieee754_sqrtf(float x);

/* Not `static inline`: at NDS_R2_HWMATH_ROUTE 0 this has exactly one caller and
 * GCC inlines it, so the shipped `sqrtf` is the same function it always was. In
 * the route build it must stay out of line so BOTH arms pay one call and the
 * measured delta is the instruction selection, not the call. */
#if defined(NDS_R2_HWMATH_ROUTE) && NDS_R2_HWMATH_ROUTE
__attribute__((noinline))
#endif
static float ndsR2SqrtfBody(float x)
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

#if defined(NDS_R2_HWMATH_ROUTE) && NDS_R2_HWMATH_ROUTE

/* THE SAME-BINARY ROUTE. The change being priced is a codegen flag -- this
 * object built -marm instead of -mthumb, so nds_r2_sqrtf.h's 48-bit
 * `root * root` becomes one UMULL instead of `bl __aeabi_lmul` (ARMv5TE Thumb
 * has no UMULL). Expected 37%-41% of an 11,608 tk/fr lane = 4,300-4,750, which
 * is FAR under the >=14,080 rank-80 cross-build floor, so a two-build A/B
 * cannot decide it. Instead both bodies live in one binary -- this TU stays
 * -mthumb and nds_r2_sqrtf_arm.c is built -marm -- and one `.data` word picks
 * the arm, giving identical ROM placement on every arm.
 *
 * .data AND NOT .bss: a zero-initialised route word without an explicit section
 * attribute lands in .bss and drags a ~10,000 tk/fr placement floor with it. */
volatile uint32_t gNdsR2HwMathRoute
    __attribute__((used, section(".data"))) = 0u;

extern float ndsR2SqrtfArmBody(float x);

float sqrtf(float x)
{
    if ((gNdsR2HwMathRoute & NDS_R2_HWMATH_ROUTE_SQRTF_ARM) != 0u)
    {
        return ndsR2SqrtfArmBody(x);
    }
    return ndsR2SqrtfBody(x);
}

#else

float sqrtf(float x)
{
    return ndsR2SqrtfBody(x);
}

#endif

#endif /* NDS_R2_FIXED_SQRT */
