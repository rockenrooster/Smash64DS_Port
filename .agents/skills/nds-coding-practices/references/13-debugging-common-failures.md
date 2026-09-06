# Debugging Common Nintendo DS Failures

## Debug by ownership boundary

When symptoms look random, identify the last boundary crossed:

- ARM9 cache to physical RAM;
- CPU to DMA;
- ARM9 to ARM7;
- main RAM to VRAM;
- shadow OAM to hardware OAM;
- CPU command stream to GX FIFO;
- filesystem to parser;
- scene allocator to asynchronous consumer;
- main code to IRQ.

Most “hardware weirdness” is a violated lifetime, format, state, or visibility
contract.

## Symptom table

| Symptom | First checks |
|---|---|
| DMA output contains old data | source range flushed, correct pointer/size, source not changed early |
| ARM9 reads old DMA/ARM7 result | dedicated aligned destination, completion wait, invalidate after completion |
| Neighbor object corrupt after invalidate | invalidation rounded across shared dirty cache line |
| Works in emulator, fails on DS | cache, alignment, timing, storage, stack, unsupported undefined behavior |
| Sprite garbage | OBJ bank/map mode, color depth, copy size, engine palette, graphics allocation |
| CPU copy into VRAM leaves stale/striped bytes | byte-path `memcpy`/`memset`; video memory ignores 8-bit writes — use 16/32-bit stores or `dmaCopy` |
| Random old sprites | unused OAM not hidden, shadow index overwrite, missing commit/reset |
| Sprites vanish only on crowded rows | per-scanline OBJ budget exhausted; wide/affine sprites sharing the row |
| BG scroll/affine jumps | mixed API/register owner, missing update, stale layer state |
| Texture colors wrong | texture palette slot/binding, format, palette upload, stale VRAM generation |
| Later 3D objects disappear | matrix-stack leak, incomplete primitive, malformed list, inherited polygon state |
| Geometry vanishes only in heavy scenes | polygon/vertex RAM overflow (2048/6144 caps); read `GL_GET_POLYGON_RAM_COUNT`/`GL_GET_VERTEX_RAM_COUNT` |
| Isolated 3D glitch/gap lines on heavy rows | per-scanline raster capacity exceeded; reduce per-line crossings/translucent fill |
| Model inside-out | handedness, winding, strip parity, culling, matrix order |
| Audio crackle | buffer underflow/lifetime, cache publication, ARM7 load, DMA/timer conflict |
| Both CPUs hang | circular wait, full queue, IRQ-disabled spin, startup handshake deadlock |
| Crash on scene exit | async DMA/GX/audio/ARM7 still references reset arena |
| Rare input double-fire | multiple `scanKeys`, duplicated catch-up events, edge consumed twice |
| Periodic frame spikes | storage/decompression, allocator, logging, GX swap/FIFO, batch rollover |
| Stack-like random corruption | large local arrays, recursion, IRQ nesting, format-string bug |

## Immediate tools for current libnds

For a development build, call `defaultExceptionHandler()` during initialization
to get an exception/register dump. `consoleDebugInit(DebugDevice_NOCASH)` directs
supported debug output to the emulator channel; `nocashMessage("literal")`
emits a message without using the visible console. Verify emulator support,
keep logging out of IRQs and timed hot paths, and retain the matching ELF/map
for symbolization. These tools reveal failures; they do not prove timing accuracy.

```sh
arm-none-eabi-addr2line -e app.elf -f -C 0xYOUR_PC
arm-none-eabi-nm -u hot.o | grep __aeabi_
arm-none-eabi-nm -S --size-sort app.elf
arm-none-eabi-objdump -dr app.elf
```

Replace `0xYOUR_PC` with the captured address. The `nm -u` check only lists
unresolved symbols in that object; linked helpers may already be resolved.
Inspect code reached with nonconstant inputs, not uncalled static functions or
unused headers. An empty optimized object proves nothing about the runtime.

See `17-libnds2-calico-facts.md` for pinned API sources and
`../tests/run_target_checks.py` for optional maintainer compile/codegen checks.

## Build a minimal boundary probe

A useful probe changes one uncertainty without replacing the subsystem:

- fill a DMA source with a sequence and validate destination/checksum;
- pin one VRAM layout and disable remaps;
- render one owner with a complete GX baseline;
- hide all OAM then enable one known sprite;
- replace streamed data with a static buffer;
- replace ARM7 command queue with one acknowledged message;
- replace fixed math with a host-generated oracle table;
- disable IRQ writer and publish a constant snapshot.

Keep probes compile-time removable and avoid changing multiple ownership rules at
once.

## Cache/coherency debugging

Add debug assertions/logging for:

- 32-byte alignment and rounded ownership;
- flushed/invalidation byte ranges;
- DMA channel and busy state;
- source/destination generations;
- producer sequence and consumer acknowledgement;
- payload checksums before publish and after consume.

Do not “fix” a coherency bug with `DC_FlushAll()` in final code. It can hide
ownership errors, destroy performance, and still not solve inbound invalidation
correctly.

## Memory corruption debugging

- Enable compiler warnings and stack protector options supported by the build.
- Add canaries around fixed pools and transfer buffers.
- Assert indexes before pointer arithmetic.
- Track arena high-water and generation.
- Validate binary file offsets/lengths before reads.
- Initialize unused OAM and render handles to invalid values.
- Poison freed/reset debug memory when practical.
- Inspect map/stack sizes and large automatic objects.

When corruption appears delayed, find the asynchronous last consumer rather than
only the point where bad bytes are observed.

## GX debugging

Reduce to one draw owner and establish a full baseline:

1. known projection/modelview;
2. balanced matrix stack;
3. known polygon format;
4. texture disabled or known texture/palette;
5. known color/normal/light state;
6. one primitive with checked coordinates/winding;
7. one frame finalization.

Then re-enable owners one at a time. A debug state wrapper that records intended
matrix depth, material key, and primitive type/vertex count can catch command-construction
bugs before they reach hardware.

## 2D debugging

- Draw a calculated diagram of bank/base intervals.
- Verify main versus sub addresses and palettes.
- Hide all 128 sprites before enabling one.
- Use a generated checkerboard and known palette.
- Disable blend/window/mosaic/master brightness.
- Stop affine motion and set identity.
- Commit shadow OAM exactly once.
- Avoid remapping during the probe.

## ARM7/audio debugging

Record sequence numbers and timestamps for:

- ARM9 submit;
- ARM7 receive;
- playback/start or buffer consume;
- ARM7 completion;
- ARM9 recycle.

Count queue full, stale generation, underflow, late completion, and dropped
optional command events separately.

Never debug a deadlock by adding arbitrary sleeps. Draw the wait-for graph and
remove the cycle.

## Timing debugging

Distinguish work from wait:

- time before/after GX writes and finalization;
- time spent waiting VBlank;
- DMA start versus first required completion;
- filesystem read versus decode/upload;
- ARM9 submit versus ARM7 acknowledgement;
- update count versus presented frames.

A profiler can perturb bus and timing behavior; compare instrumented and minimal
builds.

## Undefined behavior checks

Hardware-only failures often expose C/C++ undefined or implementation-defined
behavior:

- signed overflow;
- invalid shifts;
- unaligned typed access;
- strict-aliasing violations;
- use-after-free/stack lifetime;
- out-of-bounds arrays;
- missing `volatile` on MMIO;
- data races between IRQ/main or CPUs;
- wrong `printf` format;
- uninitialized padding/state.

Use host sanitizers/tests for pure logic and parsers even though the DS build
itself cannot run those sanitizers.

## Crash reporting

A practical debug build should capture, as available:

- build/hash and mode;
- exception type/registers/PC/LR/SP;
- symbolized address offline;
- current scene/state generation;
- recent bounded event ring;
- DMA/GX/audio/IPC owner states;
- stack watermark;
- last successful frame/update marker.

Keep crash reporting bounded and avoid recursive failure through filesystem or
complex rendering.

## Review checklist

- [ ] The failing ownership boundary is identified.
- [ ] A minimal probe changes one uncertainty.
- [ ] Async lifetime and generations are inspected.
- [ ] Cache flush/invalidate ranges are exact and safe.
- [ ] Persistent 2D/3D state is reset to a known baseline.
- [ ] Undefined behavior and stack pressure are considered.
- [ ] Timing separates active work from hardware/service waits.
