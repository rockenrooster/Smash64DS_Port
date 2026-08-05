---
name: nds-coding-practices
description: Write, port, debug, review, and optimize correct Nintendo DS code using modern devkitARM, libnds, Calico, and DS hardware practices. Use for ARM9 or ARM7 C/C++, memory ownership, cache coherency, DMA, VRAM, backgrounds, sprites, OAM, GX 3D, textures, matrices, input, interrupts, timers, frame loops, audio, IPC, fixed-point math, NitroFS, FAT/DLDI, asset conversion, linker sections, code generation, performance, or low-level DS bugs. Prefer implementation and concrete corrections over generic audit workflows.
---

# Nintendo DS Coding Practices

## Mission

Produce code that is correct for the Nintendo DS hardware first, then make it
small and fast. Give every hardware-visible object a clear owner, lifetime,
format, synchronization point, and memory-visibility rule.

Do not turn ordinary coding requests into a mandatory investigation ritual.
Implement the requested change when the repository provides enough context.
Use measurement only when performance is actually disputed or the change is an
optimization.

## Operating style

- For an implementation request, inspect the owning code, write the smallest
  complete correction, and build/test it when tools are available.
- For a review request, lead with concrete DS correctness defects and their
  fixes; do not bury them beneath a generic checklist.
- For an architecture request, define owners, lifetimes, formats, capacities,
  synchronization, and failure policy before naming APIs.
- For a port, preserve the source behavior that matters while replacing
  source-platform architecture with DS-native representations.
- Explain only the hardware constraints that affect the decision. Do not paste
  reference chapters into the answer.
- Ask for clarification only when a missing fact prevents a safe choice; first
  inspect project headers, wrappers, build files, and repository instructions.

## Source priority

Before writing API calls, determine the project's actual toolchain generation.
Use sources in this order:

1. project headers, wrappers, linker scripts, and build configuration;
2. installed devkitARM, libnds, Calico, libfat, Maxmod, or BlocksDS headers;
3. current upstream library source and official examples;
4. hardware documentation such as GBATEK;
5. third-party tutorials only as historical context.

Never guess a function signature from memory when the repository or installed
header can answer it. Modern libnds is built on Calico and differs materially
from older tutorials, especially around ARM7 services and inter-processor APIs.
Read `references/00-source-priority-and-versioning.md` when legacy and current
code are mixed.

## Non-negotiable rules

### Know the owner

Identify which component owns each operation:

- ARM9: gameplay, most rendering preparation, 2D/3D register programming,
  cached main-RAM work, and application logic.
- ARM7: sound hardware and low-level services exposed through the current
  runtime; custom ARM7 code only when the project genuinely needs it.
- 2D main/sub engines: backgrounds, sprites, windows, blending, and OAM.
- Main 3D engine: geometry, textures, matrices, polygon state, and GX FIFO.
- DMA: a transfer engine, not a cache-coherent worker thread.
- VBlank/IRQ: synchronization and bounded state capture, not general work loops.

Do not move work to another owner merely because that owner exists.

### Define memory visibility

For every buffer crossing CPU, DMA, ARM7, GX, display, audio, or storage
boundaries, establish:

- exact byte range and alignment;
- producer and consumer;
- allocation and last-consumer lifetime;
- cached or uncached location;
- flush, invalidate, wait, or message-passing requirement;
- whether an asynchronous user can still access it.

`volatile` does not flush the ARM9 data cache, extend object lifetime, make a
compound update atomic, or create an ARM9/ARM7 protocol.

### Match access width to video memory

VRAM, OAM, and palette RAM ignore 8-bit writes (GBA duplicated some byte
writes; the DS drops them). A `memcpy`, `memset`, or struct copy that takes a
byte path silently loses data while the same code works against main RAM. Use
halfword/word stores, `dmaCopy`/`swiCopy`, or copy helpers whose access width
is verified. Unaligned typed loads are equally silent: an unaligned ARM9 `LDR`
rotates the loaded word instead of faulting.

### Use DS-native data

Prefer assets and tables generated at build time in their final hardware
format. Avoid active-frame decoding, palette conversion, texture swizzling,
floating-point transforms, generic scene interpretation, and duplicate source
plus runtime representations unless required.

### Respect persistent hardware state

The 2D and 3D engines retain state. Set or restore all state a draw owner relies
on: VRAM mapping, display mode, background control, OAM mapping, affine slots,
blend state, matrix mode, matrix stack, polygon format, texture and palette,
color, normal, primitive type, and frame-finalization order.

### Keep interrupts bounded

Interrupt handlers may acknowledge hardware, capture a fixed-size value,
advance a tiny counter, or publish a flag. They must not allocate, print, access
the filesystem, wait for another subsystem, perform large copies, or run game
logic.

### Inspect expensive language operations

The DS CPUs have no hardware floating-point unit. Treat floating point, 64-bit
arithmetic, division/modulo, large structure copies, virtual dispatch, dynamic
allocation, exceptions, RTTI, and unaligned packed access as deliberate design
choices. Inspect emitted ARM code for hot paths.

## Load the relevant references

| Task | Read |
|---|---|
| API age, modern versus legacy code | `references/00-source-priority-and-versioning.md` |
| CPU, bus, displays, memory ownership | `references/01-hardware-model.md` |
| Build, linker, sections, assets | `references/02-build-link-and-assets.md` |
| C/C++, ARM/Thumb, code generation | `references/03-c-cpp-and-codegen.md` |
| Cache, DMA, shared buffers | `references/04-cache-dma-and-shared-memory.md` |
| RAM, VRAM banks, formats, residency | `references/05-vram-memory-and-assets.md` |
| Backgrounds, sprites, OAM, blending | `references/06-2d-video-bg-oam.md` |
| GX 3D, matrices, textures, display lists | `references/07-3d-gx.md` |
| Frame loop, input, VBlank, IRQ, timers | `references/08-input-irq-timers-frame-loop.md` |
| ARM7, audio, IPC, service ownership | `references/09-arm7-audio-ipc.md` |
| Fixed-point, trig, divide, sqrt | `references/10-fixed-point-and-hardware-math.md` |
| NitroFS, FAT/DLDI, streaming | `references/11-storage-filesystems-streaming.md` |
| Optimization and profiling | `references/12-performance-practices.md` |
| Crashes, corruption, visual/audio bugs | `references/13-debugging-common-failures.md` |
| Porting desktop/console code to DS | `references/14-porting-design-patterns.md` |
| Final correctness review | `references/15-review-checklists.md` |

Load only the chapters needed for the current task. Do not recite the entire
pack to the user.

Concrete reusable patterns are in `examples/README.md`. Treat them as small
ownership examples, not one application to combine blindly. Source pins and
maintenance policy are in `references/SOURCES.md`.

## Coding expectations

When producing code:

- use exact includes and project naming conventions;
- preserve existing wrapper ownership instead of bypassing it casually;
- make units explicit in names or types: bytes, halfwords, words, pixels,
  texels, frames, ticks, Q format;
- use fixed-width integer types for hardware data;
- add compile-time assertions for binary layouts and capacity limits;
- guard array, palette, OAM, matrix-stack, texture, and queue bounds;
- keep hardware writes centralized where possible;
- make startup, repeated entry, scene exit, and reset deterministic;
- provide a correct fallback only when the fallback is intentionally supported;
- mark illustrative snippets when repository-specific symbols are unknown.

Do not invent successful builds, device tests, visual approval, or performance
measurements. State which parts were verified statically and which still need a
DS toolchain or hardware run.

## Optimization expectations

For optimization work, preserve behavior unless the user explicitly accepts an
approximation. Prefer this order:

1. delete unnecessary work;
2. move invariant conversion or interpretation to build/setup time;
3. use the correct DS hardware primitive;
4. reduce state changes, bytes moved, and working set;
5. specialize hot paths and data layouts;
6. only then consider DMA, TCM, ARM/Thumb placement, ARM7 offload, or assembly.

A smaller polygon count, DMA transfer, or TCM placement is not automatically
faster. Verify the path engaged and compare the owning cost and whole-frame
cost.
