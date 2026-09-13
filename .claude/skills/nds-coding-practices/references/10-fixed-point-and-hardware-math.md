# 10 — Fixed-point and hardware math

Choose range, units, fractional bits, intermediate width, rounding, overflow policy and consumers before selecting a type. Vertex, world, squared distance and matrix values rarely share one safe format. Quantization must preserve the required predicate/visual boundary, not merely have small average error.

Signed shifts and overflow need explicit treatment. Use unsigned packing for bitfields, checked wider intermediates for multiply/accumulate, and a named negative rounding rule. Saturation is an intentional behavior change unless the domain proves it never occurs. Test zero, extrema, negative ties, products near bounds and division-by-zero handling in release as well as debug.

The [fixed_math.h](../examples/fixed_math.h) helpers demonstrate Q20.12 operations with documented input preconditions and rounding. Internal assertions are not external validation. N64 source Q16.16 decoding and model-space rebasing belong to the companion; do not narrow every source position into a DS `v16` without scaling its transform correctly.

ARM9 hardware divide/sqrt units require exclusive ownership from start through result collection. A thread or IRQ using the same unit can corrupt an in-flight calculation; libraries may also use it. Respect installed APIs, input/result width, busy state and ordering. These units are not ARM7 arithmetic engines. Avoid unsigned/signed mode mismatches and accidental 64-bit software division.

Prefer removing work first: hoist invariant divisors, precompute coefficients, share a pose or reciprocal with correct invalidation, and use squared comparisons only when sign/range/rounding and zero cases remain equivalent. Lookup tables trade arithmetic for ROM/RAM, cache misses and interpolation error. Broad `-ffast-math` or global float-to-fixed replacement is not a local optimization.

Inspect generated ARM code for the actual compiler/flags and dynamic call frequency. A 64-bit multiply can be cheaper and more correct than an overflowing narrow workaround. TCM placement helps only a measured hot working set and must respect stacks and external buffer visibility. Report target timing separately from host correctness and code-generation inspection.
