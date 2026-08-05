# Fixed-Point and Hardware Math

## Name the format

Never pass around an unexplained `int32_t` that “contains fixed point.” Put the
Q format in the type, function, field, or comment.

```c
#include <stdint.h>

typedef int32_t q20_12_t;  // 20 signed integer bits including sign, 12 fraction bits

enum { Q20_12_FRAC_BITS = 12 };
```

Document the real-world unit too: pixels, degrees, radians, meters, texels,
frames, or normalized value.

## Choose range before precision

For every fixed value, determine:

- minimum and maximum input;
- intermediate range for add/multiply/accumulate;
- required resolution/error;
- saturation, wrap, or assertion behavior;
- rounding rule;
- serialization or hardware format at boundaries.

Do not select Q12 merely because a library helper uses it elsewhere.

## Conversion

For constants, convert offline or at compile time where practical.

```c
#define Q20_12_ONE ((q20_12_t)(1 << Q20_12_FRAC_BITS))

static inline q20_12_t q20_12_from_int(int32_t x)
{
    // Multiplication avoids undefined left-shift behavior for negative x.
    const int64_t scaled = (int64_t)x * ((int64_t)1 << Q20_12_FRAC_BITS);
    return (q20_12_t)scaled; // Caller must prove the result fits.
}

static inline int32_t q20_12_to_int_trunc_zero(q20_12_t x)
{
    // C signed division truncates toward zero; the constant divisor is normally
    // strength-reduced by the compiler.
    return x / ((int32_t)1 << Q20_12_FRAC_BITS);
}
```

Do not use a macro that evaluates arguments twice. Avoid runtime float
conversion in active loops.

## Multiplication

Use a wide intermediate and make rounding deliberate.

```c
static inline q20_12_t q20_12_mul_trunc_zero(q20_12_t a, q20_12_t b)
{
    const int64_t product = (int64_t)a * (int64_t)b;
    const int64_t scale = (int64_t)1 << Q20_12_FRAC_BITS;
    return (q20_12_t)(product / scale);
}
```

This is mathematically clear but 64-bit arithmetic can be expensive on ARM9.
After correctness is established, inspect generated code and exploit proven
operand ranges where a narrower/high-word formulation is safe.

### Symmetric rounding

Adding a positive half before shifting is wrong for negative values. Define the
rule explicitly:

```c
static inline q20_12_t q20_12_mul_round_away(q20_12_t a, q20_12_t b)
{
    const int64_t product = (int64_t)a * (int64_t)b;
    const int64_t half = (int64_t)1 << (Q20_12_FRAC_BITS - 1);
    const int64_t scale = (int64_t)1 << Q20_12_FRAC_BITS;

    if (product >= 0) {
        return (q20_12_t)((product + half) / scale);
    }
    return (q20_12_t)(-(((-product) + half) / scale));
}
```

Verify desired behavior at exact negative halves; “nearest” has multiple tie
policies.

## Division

A conventional fixed divide is:

```c
static inline q20_12_t q20_12_div(q20_12_t numerator,
                                   q20_12_t denominator)
{
    // Production code must define divide-by-zero and overflow policy.
    const int64_t scale = (int64_t)1 << Q20_12_FRAC_BITS;
    const int64_t scaled = (int64_t)numerator * scale;
    return (q20_12_t)(scaled / denominator);
}
```

Before using it in a hot path:

- prove denominator nonzero;
- handle `INT_MIN / -1`-style overflow;
- inspect the helper calls/codegen;
- consider reciprocal tables or constant-divisor transforms with bounded error;
- consider the ARM9 hardware divide wrapper when serialization and latency fit.

Never replace signed division with a right shift without matching truncation for
negative values.

## Saturation and narrowing

Use a wide accumulator, then clamp before narrowing when overflow should not
wrap.

```c
static inline int16_t saturate_s16(int32_t x)
{
    if (x > INT16_MAX) return INT16_MAX;
    if (x < INT16_MIN) return INT16_MIN;
    return (int16_t)x;
}
```

Do not rely on implementation-defined narrowing to produce a portable wrap.
For intentional two's-complement wrap under the project ABI, document it and
use unsigned operations where possible.

## Trigonometry

Prefer the runtime's established fixed-angle/trig representation or a generated
lookup table. A table must define:

- angle unit and period;
- index conversion and wrap;
- output Q format;
- interpolation method if any;
- maximum error;
- table size/alignment;
- behavior for negative angles.

Range-reduce before indexing. A masked index works only when table length is a
power of two and the chosen integer representation wraps as intended.

## Matrices and vectors

Use one documented convention across CPU math and GX submission:

- row versus column vectors;
- multiplication order;
- world/view/model order;
- translation and scale formats;
- normal/vector matrix treatment;
- rounding after each operation versus after accumulation.

Accumulate dot products in a wide type. Normalize only where necessary; static
normals and matrices should often be generated offline.

## Hardware divide and square root

The ARM9 math units are shared, stateful hardware. Wrap them in one API if they
can be called from multiple systems or interrupt contexts. Do not interleave a
start/read sequence from different owners.

Rules:

- never use the same unit concurrently from an IRQ and main code without a
  protocol;
- define operand widths and signedness;
- handle divide by zero and overflow status as the hardware/API specifies;
- wait only where the result is first needed;
- benchmark against compiler helpers for the actual workload.

## Floating-point boundary

Floating point may be acceptable in host tools, tests, or infrequent setup code.
On DS runtime code:

- keep it out of frame-critical paths;
- avoid accidental double constants such as `1.0` when `float` was intended;
- inspect ELF symbols for software helper routines;
- precompute constants/assets on the host;
- compare fixed-point output against a high-precision oracle.

## Testing fixed math

Test:

- zero, one, minus one;
- min/max representable values;
- just below/above integer boundaries;
- negative half-rounding cases;
- divide-by-zero policy;
- overflow/saturation;
- angle wrap and negative angles;
- long accumulations/drift;
- exact source-game edge cases for a port.

Use a host-side reference implementation and randomized/property tests, then
run representative DS cases to catch codegen and width assumptions.

## Review checklist

- [ ] Every value has a named Q format and physical unit.
- [ ] Input and intermediate ranges are proven.
- [ ] Negative rounding and division semantics are explicit.
- [ ] Divide-by-zero and overflow have defined behavior.
- [ ] Tables are bounds-safe and have documented error.
- [ ] Hardware math units cannot be concurrently corrupted.
- [ ] Hot math has been checked for software float/64-bit/divide helpers.
