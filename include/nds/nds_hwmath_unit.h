#ifndef NDS_HWMATH_UNIT_H
#define NDS_HWMATH_UNIT_H

/* The ARM9 divide and square-root coprocessors, as one owner.
 *
 * WHO ELSE HOLDS THESE REGISTERS, measured rather than assumed. The linked
 * ELF is the oracle here, not grep: builds/build-c206-shipgx0 (the shipping
 * basis, GX_COMPOSE 0) reaches the 0x04000280-0x040002BF block from exactly
 * eight functions, and every one of them is mainline draw-phase or
 * simulation-phase code --
 *
 *   ndsRendererR2WriteLightVector         0x298 0x2a0 0x2b4  (div64 + sqrt64)
 *   ndsRendererHardwareClipVertex         0x298 0x2a0
 *   ndsRendererHardwareSubmitVertex       0x298 0x2a0
 *   ndsRendererHardwareClipVertexNdcDepth 0x298 0x2a0
 *   ndsRendererSubmitNativeImpactWave     0x2a0
 *   div64 (libnds, outlined)              0x298 0x2a0
 *   ndsCamDiv64 / ndsCamSqrt64        0x298 0x2a0 / 0x2b4
 *   sqrtf (src/nds/nds_sqrtf.c)     0x2b0 0x2b4 0x2b8 (literal-pool form)
 *
 * -- and NOTHING ELSE IN THE BINARY, libnds and calico included. The port
 * registers exactly one interrupt handler (nds_platform.c:397,
 * ndsPlatformVBlankInterrupt) and its whole body is `sVBlankCount++`. So the
 * units have no interrupt-context user in this tree and a mainline
 * write/poll/read sequence cannot be interleaved.
 *
 * THAT IS A PROPERTY OF THIS BINARY, NOT OF THE HARDWARE. SM64DS -- the DS
 * title this project reads for architecture -- treats both units as *thread
 * context*: decomp/sm64ds-decomp/src/ARMMathSaveState.c saves DIVCNT&3,
 * DIV_NUMER, DIV_DENOM, SQRTCNT&1 and SQRT_PARAM, ARMMathLoadState.c restores
 * them, and ARMSaveContext/ARMRestoreContext call them across a context switch.
 * If this port ever gains preemption, or a divide/root appears inside an
 * interrupt handler, that is the shape the seam has to take -- not an IME mask
 * bolted onto each call site. src/nds/nds_sqrtf.c already masks IME
 * around its own use, for a reachability that this survey says no longer
 * exists; it is left alone here because removing a safety property is not this
 * file's business.
 *
 * SEQUENCE. GBATEK: writing DIVCNT, DIV_NUMER or DIV_DENOM restarts the
 * division and raises DIVCNT bit 15, and the same holds for SQRTCNT /
 * SQRT_PARAM. Only the LAST write matters, so the poll that PRECEDES the
 * parameter writes is protecting nothing -- it waits out a result nobody is
 * going to read. libnds's div64/sqrt64 and src/import/battleship_gmcamera.c's
 * kernels all carry that leading poll; SM64DS's cstd::div and cstd::sqrt do not
 * (decomp/sm64ds-decomp/src/_ZN4cstd3divEii.c). Both forms are here and both
 * are priced by src/port/nds_hwmath_bench.c; the *Lead form is the
 * in-tree-proven one and is what the CFX hook aliases at the bottom select.
 *
 * SHIPPED SINCE 2026-08-16: src/nds/nds_renderer.c and src/nds/nds_sqrtf.c
 * call ndsHwMathDiv64 / ndsHwMathSqrt64 below instead of libnds's inlines.
 * src/import/battleship_gmcamera.c's own two kernels lost theirs the same day,
 * when the owner accepted NDS_CAMERA_FIXED and its default flipped to 1 --
 * until then they executed zero times and removing them would only have staled
 * the price the decision was pending on. THE LEADING POLL IS NOW GONE FROM EVERY
 * SITE IN THE BINARY: all sixteen found. The camera TU keeps its own copy of the
 * register sequence rather than including this header, because it is a decomp
 * translation unit and its include set is part of what makes it one.
 */

#include <stddef.h>
#include <stdint.h>

#if defined(__arm__)

#define NDS_HWMATH_DIVCNT       (*(volatile uint16_t *)0x04000280)
#define NDS_HWMATH_DIV_NUMER    (*(volatile int64_t *)0x04000290)
#define NDS_HWMATH_DIV_DENOM    (*(volatile int64_t *)0x04000298)
#define NDS_HWMATH_DIV_DENOM_L  (*(volatile int32_t *)0x04000298)
#define NDS_HWMATH_DIV_RESULT   (*(volatile int64_t *)0x040002A0)
#define NDS_HWMATH_DIV_RESULT_L (*(volatile int32_t *)0x040002A0)
#define NDS_HWMATH_DIVREM_L     (*(volatile int32_t *)0x040002A8)
#define NDS_HWMATH_SQRTCNT      (*(volatile uint16_t *)0x040002B0)
#define NDS_HWMATH_SQRT_RESULT  (*(volatile uint32_t *)0x040002B4)
#define NDS_HWMATH_SQRT_PARAM   (*(volatile uint64_t *)0x040002B8)

#define NDS_HWMATH_BUSY      0x8000u
#define NDS_HWMATH_DIV_64_32 1u
#define NDS_HWMATH_DIV_64_64 2u
#define NDS_HWMATH_SQRT_64   1u

/* Signed 64/64 -> 64 division, full quotient. GBATEK gives 64/64 and 64/32 the
 * same 34-cycle unit latency, so the wider mode costs one extra 32-bit store
 * and buys an UNRESTRICTED denominator -- which is the difference between "this
 * is the same arithmetic" and "this is the same arithmetic on the domain I
 * happened to check". The narrow mode is used only where the operand range is
 * part of the algorithm (ndsHwMathDivide6432, for the f32 divide).
 *
 * The unit truncates toward zero and gives the remainder the numerator's sign,
 * which is C99's rule, so the portable `(numerator) / (int64_t)(denominator)`
 * this replaces is the SAME arithmetic and not merely a close one.
 *
 * DENOMINATOR MUST BE NON-ZERO. Every NDS_CFX_DIV64 call site proves that
 * before it divides -- the s^2 guard at nds_collision_fixed.h:530, the
 * |det| >= 2^21 guard at :724, and the `dist[fixed_axis] == 0` decline at :961
 * -- and C's own divide is undefined there anyway, so this adds no obligation
 * the callers did not already carry. */
static inline int64_t ndsHwMathDivideLead(int64_t numerator,
                                            int64_t denominator)
{
    NDS_HWMATH_DIVCNT = (uint16_t)NDS_HWMATH_DIV_64_64;
    while ((NDS_HWMATH_DIVCNT & NDS_HWMATH_BUSY) != 0u)
    {
    }
    NDS_HWMATH_DIV_NUMER = numerator;
    NDS_HWMATH_DIV_DENOM = denominator;
    while ((NDS_HWMATH_DIVCNT & NDS_HWMATH_BUSY) != 0u)
    {
    }
    return NDS_HWMATH_DIV_RESULT;
}

/* SM64DS's sequence: no leading poll. Priced separately; see the header note. */
static inline int64_t ndsHwMathDivideFast(int64_t numerator,
                                            int64_t denominator)
{
    NDS_HWMATH_DIVCNT = (uint16_t)NDS_HWMATH_DIV_64_64;
    NDS_HWMATH_DIV_NUMER = numerator;
    NDS_HWMATH_DIV_DENOM = denominator;
    while ((NDS_HWMATH_DIVCNT & NDS_HWMATH_BUSY) != 0u)
    {
    }
    return NDS_HWMATH_DIV_RESULT;
}

/* 64/32 with the remainder, for ndsHwMathDivBits. The f32 divide needs the
 * remainder as its sticky bit and its operands are 56-bit over 24-bit by
 * construction, so the narrow mode is exactly right there and the range
 * restriction is discharged by the caller's own unpack. */
static inline int64_t ndsHwMathDivide6432(int64_t numerator,
                                            int32_t denominator,
                                            int32_t *remainder_out)
{
    NDS_HWMATH_DIVCNT = (uint16_t)NDS_HWMATH_DIV_64_32;
    while ((NDS_HWMATH_DIVCNT & NDS_HWMATH_BUSY) != 0u)
    {
    }
    NDS_HWMATH_DIV_NUMER = numerator;
    NDS_HWMATH_DIV_DENOM_L = denominator;
    while ((NDS_HWMATH_DIVCNT & NDS_HWMATH_BUSY) != 0u)
    {
    }
    if (remainder_out != NULL)
    {
        *remainder_out = NDS_HWMATH_DIVREM_L;
    }
    return NDS_HWMATH_DIV_RESULT;
}

static inline int64_t ndsHwMathDivide6432Fast(int64_t numerator,
                                                int32_t denominator,
                                                int32_t *remainder_out)
{
    NDS_HWMATH_DIVCNT = (uint16_t)NDS_HWMATH_DIV_64_32;
    NDS_HWMATH_DIV_NUMER = numerator;
    NDS_HWMATH_DIV_DENOM_L = denominator;
    while ((NDS_HWMATH_DIVCNT & NDS_HWMATH_BUSY) != 0u)
    {
    }
    if (remainder_out != NULL)
    {
        *remainder_out = NDS_HWMATH_DIVREM_L;
    }
    return NDS_HWMATH_DIV_RESULT;
}

/* floor(sqrt(value)) for an unsigned 64-bit operand, exactly. This is the same
 * unit and the same 64-bit mode src/nds/nds_sqrtf.c has shipped on since
 * R2-03 E1, where scripts/check-fixed-sqrt.ps1 grades the whole result
 * bit-identical to newlib's __ieee754_sqrtf. That is an existing in-tree proof
 * that the unit returns the exact floor, and it is cited rather than redone. */
static inline uint32_t ndsHwMathSqrt64Lead(uint64_t value)
{
    NDS_HWMATH_SQRTCNT = (uint16_t)NDS_HWMATH_SQRT_64;
    while ((NDS_HWMATH_SQRTCNT & NDS_HWMATH_BUSY) != 0u)
    {
    }
    NDS_HWMATH_SQRT_PARAM = value;
    while ((NDS_HWMATH_SQRTCNT & NDS_HWMATH_BUSY) != 0u)
    {
    }
    return NDS_HWMATH_SQRT_RESULT;
}

static inline uint32_t ndsHwMathSqrt64Fast(uint64_t value)
{
    NDS_HWMATH_SQRTCNT = (uint16_t)NDS_HWMATH_SQRT_64;
    NDS_HWMATH_SQRT_PARAM = value;
    while ((NDS_HWMATH_SQRTCNT & NDS_HWMATH_BUSY) != 0u)
    {
    }
    return NDS_HWMATH_SQRT_RESULT;
}

/* ==========================================================================
 * THE SHIPPED DROP-INS FOR libnds `div64` / `sqrt64`, WITHOUT THE LEADING POLL.
 *
 * libnds's div64/sqrt64 are `static inline` in nds/arm9/math.h, so their
 * leading poll is compiled into OUR objects, not into a system library: this is
 * a port-side change and libnds is not touched. Both forms are graded
 * bit-identical over 65,536 live-shaped operands per class on four builds
 * (src/port/nds_hwmath_bench.c: DivMismatch / DivFastMismatch /
 * SqrtMismatch / SqrtFastMismatch / QuotMismatch / QuotLeadMismatch /
 * RemMismatch / RemLeadMismatch all 0, with 32,914 negative denominators and
 * 223 rounding half-cases as live controls).
 *
 * ndsHwMathDiv64 is `div64` to the register: same DIV_64_32 mode, same single
 * 32-bit result read (NOT the 64-bit read ndsHwMathDivideFast does, which
 * would add an I/O access). ndsHwMathSqrt64 is `sqrt64` the same way.
 *
 * PRICE, measured two independent ways. In situ, from the per-PC execution
 * profile of build-c200-trackprof-off over its 80 marginal frames, the leading
 * polls that these two delete cost 398,615 cycles = 2,491.3 tk/fr:
 * sqrtf 295,320, ndsRendererR2WriteLightVector 46,191,
 * ndsRendererSubmitNativeImpactWave 25,088, ndsRendererHardwareSubmitVertex
 * 32,016. On the microbenchmark, 41.0 tk per divide and 20.0 per root.
 * ========================================================================== */

/* The lab-only same-binary route. At NDS_HWMATH_ROUTE 0 -- every shipped
 * build -- these expand to nothing and no route word is read, so the shipped
 * text is the poll-free form with no selector in it. At 1 the leading poll is
 * present but skipped by a `.data` word, which is how the change is priced
 * against the gate without a cross-build placement term (the expected win is
 * far under the >=14,080 rank-80 cross-build floor). */

static inline void ndsHwMathDivLeadPoll(void)
{
}

static inline void ndsHwMathSqrtLeadPoll(void)
{
}


static inline int32_t ndsHwMathDiv64(int64_t numerator, int32_t denominator)
{
    NDS_HWMATH_DIVCNT = (uint16_t)NDS_HWMATH_DIV_64_32;
    ndsHwMathDivLeadPoll();
    NDS_HWMATH_DIV_NUMER = numerator;
    NDS_HWMATH_DIV_DENOM_L = denominator;
    while ((NDS_HWMATH_DIVCNT & NDS_HWMATH_BUSY) != 0u)
    {
    }
    return NDS_HWMATH_DIV_RESULT_L;
}

static inline uint32_t ndsHwMathSqrt64(uint64_t value)
{
    NDS_HWMATH_SQRTCNT = (uint16_t)NDS_HWMATH_SQRT_64;
    ndsHwMathSqrtLeadPoll();
    NDS_HWMATH_SQRT_PARAM = value;
    while ((NDS_HWMATH_SQRTCNT & NDS_HWMATH_BUSY) != 0u)
    {
    }
    return NDS_HWMATH_SQRT_RESULT;
}

#else /* !__arm__ */

/* HOST BUILDS (the test harnesses that compile production units) have no
 * divider or root unit at 0x04000280. The portable forms below are the same
 * arithmetic the header notes above establish for the unit: division
 * truncates toward zero with the numerator-signed remainder (C99), a 64/32
 * quotient's low word is the quotient narrowed, and the root is the exact
 * floor. */

static inline uint32_t ndsHwMathHostSqrt64(uint64_t value)
{
    uint64_t root = 0u;
    uint64_t bit = (uint64_t)1u << 62;

    while (bit > value)
    {
        bit >>= 2;
    }
    while (bit != 0u)
    {
        if (value >= root + bit)
        {
            value -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)root;
}

static inline int64_t ndsHwMathDivideLead(int64_t numerator,
                                            int64_t denominator)
{
    return numerator / denominator;
}

static inline int64_t ndsHwMathDivideFast(int64_t numerator,
                                            int64_t denominator)
{
    return numerator / denominator;
}

static inline int64_t ndsHwMathDivide6432(int64_t numerator,
                                            int32_t denominator,
                                            int32_t *remainder_out)
{
    if (remainder_out != NULL)
    {
        *remainder_out = (int32_t)(numerator % denominator);
    }
    return numerator / denominator;
}

static inline int64_t ndsHwMathDivide6432Fast(int64_t numerator,
                                                int32_t denominator,
                                                int32_t *remainder_out)
{
    return ndsHwMathDivide6432(numerator, denominator, remainder_out);
}

static inline uint32_t ndsHwMathSqrt64Lead(uint64_t value)
{
    return ndsHwMathHostSqrt64(value);
}

static inline uint32_t ndsHwMathSqrt64Fast(uint64_t value)
{
    return ndsHwMathHostSqrt64(value);
}

static inline int32_t ndsHwMathDiv64(int64_t numerator, int32_t denominator)
{
    return (int32_t)(numerator / denominator);
}

static inline uint32_t ndsHwMathSqrt64(uint64_t value)
{
    return ndsHwMathHostSqrt64(value);
}

#endif /* __arm__ */

/* The two NDS_CFX_* hooks, in the shape those macros want. The narrowing
 * cast is the portable form's own -- it takes the low 32 bits of the 64-bit
 * quotient, and DIV_RESULT's low word is the same low 32 bits, so a quotient
 * that does not fit narrows identically rather than saturating differently. */
static inline int32_t ndsHwMathCfxDiv64(int64_t numerator,
                                          int64_t denominator)
{
    return (int32_t)ndsHwMathDivideLead(numerator, denominator);
}

static inline uint32_t ndsHwMathCfxIsqrt64(uint64_t value)
{
    return ndsHwMathSqrt64Lead(value);
}

#endif /* NDS_HWMATH_UNIT_H */
