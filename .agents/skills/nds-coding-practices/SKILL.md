---
name: nds-coding-practices
description: Write, port, debug, review, and optimize fast, correct Nintendo DS C/C++ for ARM9 and ARM7. Use for devkitARM, libnds, Calico, cache/DMA, RAM/TCM/VRAM, BG/OAM, GX 3D, textures, matrices, IRQ/timers, frame loops, audio/PXI, fixed-point math, NitroFS, asset conversion, linker sections, and DS code generation. Prefer concrete implementations and DS-specific corrections over generic audit workflows. Detect the project's SDK generation before selecting APIs; do not force a Calico migration on another SDK.
---

# Nintendo DS Coding Practices

## Mission

Write an efficient first implementation, not a slow generic implementation that
must later be optimized. Start with DS-native data, bounded storage, the right
hardware operation, and the existing subsystem owner. Preserve required behavior.

This skill is project-independent. Frame rate, memory reservations, acceptable
approximations, testing authority, and compatibility policy belong to the project.
Do not import another game's budgets, architecture, or completion gates.

## Working style

For implementation, inspect the owning code, make the smallest complete change,
and build/test the affected target when available. For review, lead with concrete
defects and fixes. For architecture, resolve capacity, ownership, lifetime, and
failure behavior before choosing APIs. For a port, preserve the required behavior,
not the source machine's architecture.

Use three categories deliberately:

- **Requirements:** hardware access, bounds, cache visibility, and lifetime must
  be correct. Performance does not excuse a violation.
- **Defaults:** native asset formats, direct hardware operations, bounded arrays,
  and setup-time preparation are sensible starting choices, not benchmark gates.
- **Experiments:** DMA crossover, TCM placement, alternate layouts, offloading,
  assembly, and approximations need proportionate evidence before claiming a win.

Do not create a resource manager, manifest system, tracing framework, or policy
document for a small change that existing code can express. A local invariant or
comment usually suffices. Ask a question only when inspection cannot resolve a
fact needed for a safe choice.

## Select the API generation first

Check the project's build flags, wrappers, installed headers, linker scripts,
and ARM7 core. Use project/installed APIs and source at the matching version;
consult compatible official examples next. Hardware documentation governs actual
hardware behavior, regardless of what a wrapper or old tutorial implies.

Calico-based devkitPro libnds 2.x differs materially from legacy libnds and
BlocksDS. Read `references/00-source-priority-and-versioning.md` when generations
are mixed. For new Calico code or unfamiliar APIs, use
`references/17-libnds2-calico-facts.md`: memory layout, stack placement, PXI,
threads, IRQs, math formats, API side effects, and debugging calls.

The pins document a checked baseline, not a command to upgrade dependencies.
Never guess an API signature when installed source can answer it. Check blocking,
cache maintenance, and channel use as well as names and parameters.

## DS constraints that change code

### Memory and transfers

The physical DS has 4 MiB main RAM; that is not the application's free heap.
Account for its SDK layout, loaded code/data, ARM7/runtime reservations, stacks,
transfers, and peak transition overlap. `const` does not make linked data
cartridge-backed or hardware-write-protected.

For a hardware-visible buffer, know its addressability, exact byte range,
alignment, producer, last consumer, and completion/reuse point. DMA cannot access
ARM9 ITCM or DTCM; the default Calico ARM9 main stack is in DTCM. A flush or wait
cannot repair an inaccessible address. Prefer explicit main-RAM staging for DMA.

Publish dirty ARM9 cache lines before external reads; prepare and invalidate
owned full cache lines for external writes. `volatile` is not cache coherence,
mutual exclusion, or a lifetime guarantee. Validate zero counts, transfer units,
count limits, alignment, and channel availability before DMA.

### Video and persistent state

ARM9 byte writes to VRAM, palette RAM, and OAM are ignored. Use verified
halfword/word accesses or a suitable transfer API; a generic byte-tail copy is
not safe. Avoid unaligned typed accesses: legacy ARM behavior can corrupt values
silently rather than trap.

Keep VRAM mapping and BG/OAM/GX state under coherent owners. Do not mix direct
register writes with library shadow state without synchronization. Establish the
state a draw relies on; check bank intervals, sprite/affine slots, geometry
capacity, and matrix depth. DMA/list transfer completion is not the end of
texture or display use.

### Runtime and arithmetic

Keep IRQ callbacks tiny and nonblocking. In Calico, tick/PXI callbacks also run
in IRQ context; wake a worker using documented IRQ-safe primitives for heavier
work. Workers must block when idle: priority scheduling has no timeslicing, and
`threadYield()` does not yield to lower-priority work. Timers 2/3 belong to Calico.

Neither CPU has an FPU. Prefer native fixed formats for hot math, with explicit
range and rounding. A wide multiply intermediate is not the same cost as a
variable 64-bit divide. Native helpers are defaults only when their semantics
match; do not silently change negative rounding or overflow behavior.

Use stock ARM7 services unless a requirement actually needs custom ARM7 code.
Offloading is not automatically faster, and a second CPU requires compatible
startup, services, memory visibility, and a bounded protocol.

## Load only the relevant material

Start with one task chapter; expand when the change crosses an ownership boundary.
Do not load or recite the whole pack for every request.

| Task | Reference |
|---|---|
| SDK generation / compatibility | `references/00-source-priority-and-versioning.md` |
| CPU, bus, memory, display model | `references/01-hardware-model.md` |
| Build, linker, sections, assets | `references/02-build-link-and-assets.md` |
| C/C++, ARM/Thumb, emitted code | `references/03-c-cpp-and-codegen.md` |
| Cache, DMA, shared memory | `references/04-cache-dma-and-shared-memory.md` |
| RAM/VRAM residency and formats | `references/05-vram-memory-and-assets.md` |
| Backgrounds, sprites, OAM | `references/06-2d-video-bg-oam.md` |
| GX, matrices, textures, lists | `references/07-3d-gx.md` |
| Input, frame loop, IRQ, timers | `references/08-input-irq-timers-frame-loop.md` |
| ARM7, audio, IPC | `references/09-arm7-audio-ipc.md` |
| Fixed-point and hardware math | `references/10-fixed-point-and-hardware-math.md` |
| Storage and streaming | `references/11-storage-filesystems-streaming.md` |
| Profiling and optimization | `references/12-performance-practices.md` |
| Crashes and visual/audio failures | `references/13-debugging-common-failures.md` |
| Porting architecture | `references/14-porting-design-patterns.md` |
| A targeted final review | `references/15-review-checklists.md` |
| Efficient first implementation | `references/16-fast-implementation-recipes.md` |
| Concrete libnds 2.x / Calico APIs and traps | `references/17-libnds2-calico-facts.md` |

Reusable examples and their integration limits are in `examples/README.md`.
Source pins are in `references/SOURCES.md`. `tests/` and `CHANGELOG.md` are
maintenance material, not required context for ordinary coding.

## Finish in proportion to the change

Use exact includes, explicit units, bounded indexes, and compile-time layout
checks for binary interfaces. Handle allocation and externally supplied invalid
inputs in release builds. Preserve startup, repeated entry, teardown, and reset
behavior. Mark integration snippets and assumptions clearly.

For optimization, first remove work, precompute invariant conversion, use the
right hardware primitive, reduce data movement/state changes, and specialize the
actual hot path. Then consider placement, offload, or assembly. Do not equate
fewer polygons, more DMA, or more ITCM with a speedup.

Exercise the affected behavior and build/link the real target when possible.
For claimed improvements, compare representative before/after costs and verify
that the intended path executes. Report static checks, host tests, target builds,
visual/device tests, and timings separately. Never invent a successful test or a
performance gain.
