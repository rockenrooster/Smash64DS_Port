# C, C++, ARM/Thumb, and Code Generation

## Use the language as a hardware description aid

Prefer code whose cost and memory accesses are visible. Abstract interfaces are
acceptable when they compile away or sit outside hot paths; abstraction is not
an excuse to ignore generated ARM code.

## Integer rules

- Use `uint8_t`, `uint16_t`, `uint32_t`, and signed counterparts for binary
  formats and hardware payloads.
- Use `size_t` for host-language object sizes, then range-check before narrowing
  to a hardware count field.
- Make signedness deliberate. Mixed signed/unsigned comparisons are a common
  source of invisible bounds bugs.
- Signed overflow is undefined C/C++ behavior. Use wider intermediates and
  explicit saturation/wrap semantics.
- Do not assume `char` signedness.

```c
static inline int16_t clamp_s16(int32_t x) {
    if (x > INT16_MAX) return INT16_MAX;
    if (x < INT16_MIN) return INT16_MIN;
    return (int16_t)x;
}
```

## Alignment and aliasing

The ARM CPUs are not modern x86 processors. In the normal DS configuration an
unaligned ARM9 `LDR` does not fault: it loads from the rounded-down address
and rotates the result, and unaligned halfword loads are similarly wrong, so
the defect compiles cleanly and corrupts data silently at runtime. Avoid
unaligned typed loads and stores. Packed structures can force expensive byte
sequences or unsafe direct access.

Wrong:

```c
uint32_t value = *(const uint32_t *)(bytes + 1); // alignment and aliasing risk
```

Safer:

```c
uint32_t value;
memcpy(&value, bytes + 1, sizeof value);
```

For a serialized little-endian format, decode bytes explicitly when host/tool
portability matters.

Respect strict aliasing. Use unions only under a project/compiler contract, and
prefer `memcpy` for representation changes.

## `volatile`

Use `volatile` for memory-mapped I/O and values asynchronously changed by an
IRQ when the access pattern is otherwise safe. It does **not** provide:

- cache flush/invalidate;
- inter-CPU barriers;
- atomic compound operations;
- queue consistency;
- lifetime guarantees;
- mutual exclusion.

Keep MMIO access narrow and centralized. Re-reading a volatile register may
have side effects or return a new value.

## Floating point

The DS has no FPU. Floating-point operations can call software helpers and
inflate code, latency, and stack use. Prefer:

- fixed-point values with named Q formats;
- integer lookup tables generated offline;
- precomputed matrices/curves;
- host-side conversion;
- constants already expressed in the target fixed format.

Do not replace float mechanically. Establish range, precision, rounding,
overflow, and source behavior first.

## Division, modulo, and 64-bit arithmetic

Variable division/modulo and 64-bit operations can be costly or call helpers.
Good options, when mathematically valid:

- constant divisors optimized by the compiler;
- reciprocal multiply with proven range/error;
- power-of-two shifts for unsigned or carefully defined signed behavior;
- bounded lookup tables;
- the ARM9 hardware divide wrapper with serialized ownership;
- reducing precision before entering a hot loop.

Never replace signed division with a shift without handling negative rounding.

## ARM versus Thumb

- ARM code offers richer instructions and may be faster for dense hot kernels.
- Thumb can reduce code footprint and instruction-cache pressure.
- Interworking and call frequency matter.
- Measure at function or translation-unit granularity.

Do not mark every hot-looking function ARM or place it in ITCM. Inspect the
actual instruction stream, call graph, and working set.

## Branches and tables

- Prefer predictable branches over elaborate branchless code that adds loads.
- Range-check indexes before lookup.
- Keep hot tables compact and aligned.
- Separate hot fields from cold metadata.
- Avoid pointer-rich structures when a dense index or structure-of-arrays layout
  improves locality and size.

## Functions and inlining

Inlining can remove call overhead but increase code size and I-cache pressure.
Use `static inline` for tiny type-safe helpers. Do not force-inline large state
machines or renderers without disassembly and whole-frame evidence.

## Stack and recursion

The DS has little memory and limited stack headroom.

- Avoid large automatic arrays.
- Avoid recursion in runtime code unless depth is tightly bounded and proven.
- Include interrupt nesting and library calls in stack estimates.
- Move persistent or large scratch buffers into an owned arena.
- Do not return pointers to stack storage used by DMA, GX, audio, or ARM7.

## Dynamic allocation

Do not allocate/free in an IRQ. Avoid per-frame heap churn. Prefer:

- scene arenas reset as a unit;
- fixed-capacity pools;
- slab allocators for uniform objects;
- startup allocation for persistent systems;
- explicit failure handling.

## Codegen review triggers

Inspect disassembly when code contains:

- float/double;
- 64-bit multiply/divide;
- variable divide/modulo;
- large switch statements;
- packed or unaligned data;
- structure assignment/memcpy;
- virtual calls;
- atomics or locks;
- forced inlining or section placement;
- supposedly eliminated compatibility paths.
