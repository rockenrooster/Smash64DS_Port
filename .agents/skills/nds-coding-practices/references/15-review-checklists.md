# Nintendo DS Code Review Checklists

Use only the sections relevant to the change. These are final safety nets, not a
mandatory report format.

## Universal correctness

- [ ] The hardware/software owner is unambiguous.
- [ ] Buffer lifetime extends through the last asynchronous consumer.
- [ ] Units, formats, sizes, and alignment are explicit.
- [ ] Bounds/capacities and overflow policy are checked.
- [ ] Repeated initialization, scene exit, reset, and error paths are safe.
- [ ] Persistent hardware state is established rather than inherited.
- [ ] Current project headers/API generation were checked.
- [ ] No build, hardware, visual, or performance result is claimed without
      evidence.

## C/C++ and code generation

- [ ] No signed overflow, invalid shifts, aliasing, or unaligned typed access.
- [ ] Narrowing and signedness are deliberate.
- [ ] Large stack objects and recursion are bounded.
- [ ] Per-frame heap churn is absent or justified.
- [ ] Float, 64-bit arithmetic, divide/modulo, virtual calls, and large copies in
      hot paths were inspected.
- [ ] Final map/sections/assets match intent.

## Cache and DMA

- [ ] ARM9-produced cached source is flushed before external read.
- [ ] ARM9 destination invalidation owns complete aligned cache lines.
- [ ] Inbound producer completion precedes final invalidation/read.
- [ ] DMA channel, timing mode, unit, alignment, and size multiple are correct.
- [ ] Source/destination cannot be changed, freed, or remapped early.
- [ ] No unsupported overlap.
- [ ] Async start has either real overlap or a clear scheduling reason.

## ARM9/ARM7 and audio

- [ ] Current runtime service API is preferred over obsolete/custom paths.
- [ ] Messages validate type, length, units, index, and generation.
- [ ] Shared pointers have address, cache, mapping, and lifetime proof.
- [ ] Queues are bounded with explicit overflow/backpressure.
- [ ] No circular wait or IRQ-disabled wait.
- [ ] Audio sample/stream buffers remain alive until completion.
- [ ] Channel handles cannot target a reused voice accidentally.

## VRAM and assets

- [ ] All banks A-I have declared non-conflicting roles.
- [ ] No byte-granularity CPU write targets VRAM, palette RAM, or OAM.
- [ ] Raw VRAM pointers do not survive remapping.
- [ ] Handles validate layout generation/residency.
- [ ] BG/OBJ/texture/palette intervals fit and do not overlap.
- [ ] Assets are in final DS-native formats before active use.
- [ ] Palette/transparency behavior is explicit.
- [ ] Peak transition RAM and duplicate representations are budgeted.

## 2D graphics

- [ ] Main/sub engine destinations and palettes are correct.
- [ ] BG type, size, map/tile/bitmap bases, priority, and wrap are correct.
- [ ] BG API/direct-register ownership is not mixed accidentally.
- [ ] All unused OAM entries are hidden.
- [ ] Sprite indices and affine slots have one allocator and bounds checks.
- [ ] Mapping mode, sprite size, color depth, and byte count agree.
- [ ] Shadow OAM commits once at the intended boundary.
- [ ] Blend/window/mosaic/brightness state is initialized deterministically.
- [ ] Worst-case per-scanline OBJ load fits the engine budget.
- [ ] Display capture, when used, has one owner, a stable LCDC-mapped bank,
      and explicit frame sequencing.

## 3D graphics

- [ ] Projection/model/vector/texture matrix conventions are documented.
- [ ] Matrix mode and stack depth are balanced on all paths.
- [ ] Primitive begin/end, counts, indexes, winding, and strip parity are valid.
- [ ] Each batch sets polygon, texture, palette, color/normal/material state.
- [ ] Texture handles remain valid for the current VRAM generation.
- [ ] Transparency/depth/polygon-ID behavior matches DS hardware.
- [ ] Worst-case polygon/vertex counts fit the 2048/6144 frame caps, verified
      rather than assumed.
- [ ] Frame finalization has one owner and occurs once.
- [ ] Prebuilt streams have format, patch, cache, lifetime, and engagement proof.

## Input, IRQ, timers, lifecycle

- [ ] `scanKeys` occurs once per logical update.
- [ ] Edge events are not duplicated by catch-up/render paths.
- [ ] IRQ handlers are bounded and avoid allocation, I/O, waits, and heavy work.
- [ ] Compound IRQ/main state uses a coherent snapshot protocol.
- [ ] VBlank performs bounded commits only.
- [ ] Timer channels, units, rollover, and library conflicts are handled.
- [ ] Sleep/resume and repeated scene entry restore state.

## Fixed-point math

- [ ] Q format and physical unit are named.
- [ ] Input/intermediate ranges and narrowing are proven.
- [ ] Negative rounding/division behavior is explicit.
- [ ] Divide-by-zero/overflow policy exists.
- [ ] Lookup indexes and angle wrapping are safe.
- [ ] Hardware divide/sqrt has serialized ownership.
- [ ] Results are compared against a high-precision oracle.

## Storage

- [ ] No filesystem operation occurs in an IRQ.
- [ ] Open/read/seek/write/close errors and partial transfers are handled.
- [ ] File counts/offsets/sizes are validated without integer overflow.
- [ ] First-use I/O/decode is outside visible critical paths.
- [ ] Stream slots use state, generation, valid length, and end/error markers.
- [ ] Save updates preserve a prior valid copy until successful commit.

## Performance change

- [ ] Behavior and visual/audio correctness remain acceptable.
- [ ] The owner and expected mechanism are named.
- [ ] Optimized path engagement is visible.
- [ ] Representative median/tail/missed-frame behavior is compared.
- [ ] Whole-frame time, code/RAM/VRAM size, and contention are considered.
- [ ] Work deletion/precompute/layout was considered before placement tricks.
- [ ] Emulator evidence is not overstated as hardware proof.

## Release readiness

- [ ] Clean build from generated sources/assets succeeds.
- [ ] Warning policy is clean or deviations documented.
- [ ] Debug probes/logging are disabled or bounded.
- [ ] Stack/arena/pool high-water is acceptable.
- [ ] Cold start, repeated scene entry, sleep/resume, reset, and failure paths run.
- [ ] Representative emulator and target-hardware checks are completed as
      required by the claim.
- [ ] Dependency/toolchain versions and reproducible build instructions exist.
