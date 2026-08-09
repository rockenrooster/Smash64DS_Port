#ifndef SSB64_NDS_FCMP_H
#define SSB64_NDS_FCMP_H

/* Bit-exact integer replacements for the soft-float comparison helpers.
 *
 * The ARM9 has no FPU, so every `x != 0.0f` in this codebase is a call to
 * `__aeabi_fcmpeq`. Measured over the cycle-106 whole-match profile
 * (`scripts/analyze-leaf-helper-attribution.py`), the five comparison helpers
 * cost **12,909,690 cycles, 1.32% of all non-idle work**, across 1,045,094
 * calls: fcmpeq 574,895 at 10.6 cycles each, then fcmplt/fcmpgt/fcmple/fcmpge
 * at 14.2-14.6. The helpers are libgcc's hand-written ARM assembly and already
 * resident in ITCM, so there is nothing to win inside them -- the win is not
 * making the call.
 *
 * `-ffinite-math-only` does NOT do this for you. Checked before writing any of
 * it: GCC emits the same `bl __aeabi_fcmp*` with and without it, in both ARM
 * and Thumb. There is no build-flag shortcut.
 *
 * These are EXACT, not approximations. IEEE-754 binary32 has a unique
 * representation for every value except zero, so for a non-zero non-NaN
 * constant `c`, `x == c` holds exactly when the bit patterns match; and because
 * negative floats map to negative int32 while positives map to positive int32
 * in the same relative order, an ordered comparison against a POSITIVE constant
 * is a signed integer comparison of the bit patterns. Zero is the one case that
 * needs care -- `-0.0f == +0.0f` is true but the patterns differ -- which is why
 * the zero predicates test `bits << 1` and drop the sign rather than comparing
 * the pattern directly. `scripts/check_fcmp_exact.py` proves all of it over
 * every one of the 2^32 bit patterns; it is not argued from this comment.
 *
 * NaN is the documented exclusion. IEEE makes every comparison with NaN false
 * (and `!=` true); these predicates order NaN instead. Gameplay float data
 * never holds one -- a NaN in an AObj or a collision segment is already a bug
 * that would be visible long before it reached a comparison -- and the checker
 * reports NaN separately rather than hiding it.
 *
 * Only use the `_C` forms with a POSITIVE compile-time constant. There is no
 * predicate for comparing two arbitrary runtime floats: that needs the full
 * sign-magnitude-to-two's-complement key, which is more work than it saves at
 * the sites this exists for. Those calls stay as they are.
 */

#include <PR/ultratypes.h>

static inline u32 ndsFcmpBits(f32 v)
{
    u32 bits;

    __builtin_memcpy(&bits, &v, sizeof(bits));
    return bits;
}

/* Inline functions rather than macros, so an argument is evaluated exactly
 * once. `NDS_FCMP_GE0`/`LE0` need their operand twice and would have expanded
 * it twice as macros -- harmless at today's call sites and a live trap at the
 * next one. Everything below folds to the same two or three instructions at
 * -O2 that a macro would.
 *
 * x != 0.0f and x == 0.0f: the shift drops the sign bit so that -0.0f and
 * +0.0f both read as zero, which is what IEEE equality says. */
static inline s32 ndsFcmpNe0(f32 x) { return (ndsFcmpBits(x) << 1) != 0u; }
static inline s32 ndsFcmpEq0(f32 x) { return (ndsFcmpBits(x) << 1) == 0u; }

/* x < 0.0f. Strictly negative, so -0.0f (0x80000000) must NOT qualify -- hence
 * `>` rather than `>=`. x > 0.0f: every negative pattern has the top bit set
 * and so reads negative as s32, and +0.0f is 0 and fails `> 0`. */
static inline s32 ndsFcmpLt0(f32 x) { return ndsFcmpBits(x) > 0x80000000u; }
static inline s32 ndsFcmpGt0(f32 x) { return ((s32)ndsFcmpBits(x)) > 0; }

static inline s32 ndsFcmpGe0(f32 x)
{
    u32 b = ndsFcmpBits(x);

    return (((s32)b) >= 0) || (b == 0x80000000u);
}

static inline s32 ndsFcmpLe0(f32 x)
{
    u32 b = ndsFcmpBits(x);

    return (b >= 0x80000000u) || (b == 0u);
}

/* Ordered comparisons against a POSITIVE constant. `c` must be a literal the
 * compiler folds to its bit pattern; a runtime float still works but costs a
 * conversion and defeats the point. */
static inline s32 ndsFcmpGtC(f32 x, f32 c)
{
    return ((s32)ndsFcmpBits(x)) > ((s32)ndsFcmpBits(c));
}

static inline s32 ndsFcmpLtC(f32 x, f32 c)
{
    return ((s32)ndsFcmpBits(x)) < ((s32)ndsFcmpBits(c));
}

/* x == c and x != c for a NON-ZERO constant. Unique representation makes this a
 * plain pattern compare; for c == 0 use the zero forms above, because -0.0f and
 * +0.0f are equal in IEEE and differ in bits. */
static inline s32 ndsFcmpEqC(f32 x, f32 c)
{
    return ndsFcmpBits(x) == ndsFcmpBits(c);
}

static inline s32 ndsFcmpNeC(f32 x, f32 c)
{
    return ndsFcmpBits(x) != ndsFcmpBits(c);
}

#define NDS_FCMP_NE0(x) ndsFcmpNe0(x)
#define NDS_FCMP_EQ0(x) ndsFcmpEq0(x)
#define NDS_FCMP_LT0(x) ndsFcmpLt0(x)
#define NDS_FCMP_GT0(x) ndsFcmpGt0(x)
#define NDS_FCMP_GE0(x) ndsFcmpGe0(x)
#define NDS_FCMP_LE0(x) ndsFcmpLe0(x)
#define NDS_FCMP_GT_C(x, c) ndsFcmpGtC((x), (c))
#define NDS_FCMP_LT_C(x, c) ndsFcmpLtC((x), (c))
#define NDS_FCMP_EQ_C(x, c) ndsFcmpEqC((x), (c))
#define NDS_FCMP_NE_C(x, c) ndsFcmpNeC((x), (c))

#endif /* SSB64_NDS_FCMP_H */
