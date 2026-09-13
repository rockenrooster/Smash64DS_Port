# 03 — C/C++ and emitted code

Specify widths and units at interfaces; use aligned native objects for hot state and explicit byte decoding for wire/file data. Validate before allocation or narrowing: `count <= limit`, `offset <= size`, `length <= size-offset`, and checked products. Avoid typed loads through misaligned or alias-incompatible pointers.

Signed overflow and left shifts of negative values are not portable wrapping arithmetic. Use unsigned packing, checked wider intermediates, and named rounding rules. Signed right-shift behavior is not a substitute for a documented negative rounding policy. `volatile` applies to externally changing accesses/MMIO; it is neither an inter-CPU barrier nor a lock.

ARM9/ARM7 have no FPU. Literals such as `0.5` can introduce double operations; library calls can hide conversions/divisions. Do not globally replace floats or enable `-ffast-math`: first locate dynamic cost and required numerical behavior. Setup-time float may be acceptable; a required 64-bit multiply may compile efficiently; variable 64-bit division deserves closer scrutiny.

Inspect the changed function **and callers** for soft-float helpers, division, spills, indirect calls, unexpected memcpy, alignment fixes, and code growth. ARM versus Thumb, inlining, lookup tables, hot/cold splitting, and TCM placement are workload-dependent. Include instruction-cache and data traffic, not just instruction counts. Inspect the final linker map for placement claims.

C++ RAII can improve ownership without per-frame allocation. Avoid hidden allocations in hot containers/strings, destruction from IRQ context, or large stack temporaries. Prefer bounded pools and stable handles where lifetimes require them; generation counters prevent stale reuse only when all consumers validate them.

Use `assert` for internal proven preconditions, not as the only validation of asset bytes or external counts: release builds remove it. The pack's arithmetic examples document their valid domains. Test debug, optimized, `NDEBUG`, negative extrema, and sanitizers; target codegen checks do not prove timing. See [10](10-fixed-point-and-hardware-math.md) and [tests](../tests/README.md).
