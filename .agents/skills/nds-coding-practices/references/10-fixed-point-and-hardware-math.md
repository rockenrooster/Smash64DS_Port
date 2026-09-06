# Fixed-Point and Hardware Math

## Start with the native boundary format

For libnds/GX code, use native formats when their range and semantics fit.
Do not invent an incompatible fixed-point framework just to wrap the API.

| Value | Storage and fraction bits | Scale / native helpers |
|---|---|---|
| f32 scalar/matrix convention | signed 32-bit, 12 fraction bits | 4096; `inttof32`, `mulf32`, `divf32`, `sqrtf32` |
| `v16` vertex | signed 16-bit, 12 fraction bits | 4096; -8..32767/4096; `inttov16`, `glVertex3v16` |
| `t16` UV | signed 16-bit, 4 fraction bits | 16; -2048..32767/16 texels; `inttot16`, `TEXTURE_PACK` |
| `v10` normal component | **signed packed 10-bit, 9 fraction bits** | **512**; -1..511/512; `f32tov10`, `NORMAL_PACK` |
| Binary angle | 32768 units per full turn | `DEGREES_IN_CIRCLE`, `degreesToAngle` |
| Interpolated trig result | 12 fraction bits | `sinLerp`, `cosLerp` |

The scalar name “f32” does not imply a typedef; the APIs take `s32`/`int` values.
The normal header's “.10” comment refers to the ten-bit component format, not
ten fractional bits. Pack signed components with care; unvalidated narrowing
and signed shifts can be unsafe C even when the hardware format is correct.

Hardware vertex/UV ranges are not an appropriate world-coordinate range for
all applications. Keep broader simulation coordinates where needed and transform
or quantize at the actual submission boundary.

Sources: pinned libnds `math.h`, `videoGL.h`, and `trig_lut.h`, linked in
`17-libnds2-calico-facts.md` and `SOURCES.md`.

## Range and physical units precede Q notation

Name units (pixels, texels, ticks, normalized values) as well as fraction bits.
For a hot kernel, establish operand range, intermediate range, precision, rounding,
and overflow policy. Q20.12 here counts the sign among the 20 integer bits.

A 32-bit fixed value often needs a 64-bit product or accumulation. In ARM mode,
32x32-to-64 multiplication can use `SMULL`; it is not comparable to a variable
64-bit software divide. Keep the intermediate required for correctness and
inspect the actual generated kernel before narrowing it.

Precompute constants, static normals, curves, and matrices offline or at setup.
Move invariants out of loops before adding lookup tables or approximation.

## Match rounding before substituting helpers

`mulf32` computes a signed wide product followed by `>> 12`; on the DS target,
negative fractional results round down. The portable
`q20_12_mul_trunc_zero` reference divides by 4096, so it rounds toward zero.
For raw operands `-1` and `1`, these produce `-1` and `0` respectively. Both are
fixed-point operations, but they are not behaviorally interchangeable.

`examples/fixed_math.h` deliberately supplies explicit-rounding reference
functions for host tests and semantic comparison. It is **not** a recommendation
to use generic software division in every ARM9 hot path. Its range and nonzero
assertions are caller preconditions, not release-mode validation of untrusted data.

Signed right shift is implementation-defined in older C standards; left-shifting
a negative signed value is undefined in C11. Some native conversion/math macros
use shifts. Match the project's compiler contract, avoid negative signed left
shifts in new portable helpers, and test negative inputs when wrapping native code.
Multiplication by a wide positive scale avoids the signed-left-shift issue.

## Necessary divisions and square roots

First ask whether a divide can be eliminated or performed once at setup. Constant
divisors may already compile into efficient instructions. For a necessary ARM9
fixed-point divide, current `divf32` uses the hardware divider in 64/32 mode;
`div32`, `div64`, `mod32`, `sqrt32`, and `sqrt64` expose related native operations.
Their headers specify operands and result widths. Do not confuse `div64`'s
64-bit numerator with an unrestricted 64-bit returned quotient.

Before using a native or portable helper:

- Reject or explicitly define zero divisors and out-of-range results.
- Match signedness and negative rounding to the required behavior.
- Keep stateful hardware operands/results from being interleaved by another
  math-unit user. Follow verified runtime/wrapper synchronization; do not add
  global locks to a single-owner path just because multiple threads exist.

A generic variable `int64_t` division can pull in `__aeabi_ldivmod`. That is a
review trigger in a hot path, not an automatic failure in cold reference code.
Likewise, absence of a helper symbol does not prove a kernel is fast or even
present in the optimized object. Inspect exercised call sites and the linked ELF.

## Accumulation, narrowing, and approximation

Use wide accumulators, then clamp before narrowing when saturation is required.
Do not replace deliberate saturation with implementation-defined wrap. For
intentional two's-complement wrap, prefer explicit unsigned arithmetic at the
boundary and document the ABI expectation.

A reciprocal or trig table needs a domain, scale, wrap rule, and bounded error.
Do not change exact behavior without authorization. A power-of-two mask is only
a correct wrap when the representation and table period agree. Binary-angle
functions and degree/radian wrappers are not interchangeable.

For matrices, keep one convention for vector orientation, multiplication order,
translation/scale, vector versus position matrices, and rounding. Accumulate dot
products with sufficient width. Normalize only when a consumer requires it.

## Floating point and testing

Setup-time float, host conversion, and high-precision reference tests can be
appropriate. The DS has no FPU; avoid unnecessary runtime float/double in critical
loops and inspect helpers/stack use rather than banning all floating literals.

Test zero, +/-1, extrema, negative fractions, exact half ties, boundaries after
narrowing, zero-divisor policy, angle wrap, and long accumulation. Keep nonconstant
inputs in codegen probes so the compiler cannot fold away the operation being
inspected. See `../tests/portable_codegen.c`, `../tests/helper_codegen.c`, and
`../tests/run_target_checks.py`.

Run pure logic on a host with sanitizers, then compile the actual target and
exercise representative DS cases. Host success does not establish hardware-math
reentrancy, MMIO behavior, or target performance.
