# Nintendo DS Performance Practices

## Correctness first, mechanism second

Optimization begins with a concrete owner and mechanism, not a fashionable
technique. “The renderer is slow” is too broad. Useful mechanisms include:

- repeated matrix construction;
- asset conversion in the active frame;
- GX FIFO backpressure;
- cache misses from pointer-heavy data;
- large state-change command streams;
- storage stalls;
- ARM9/ARM7 queue waits;
- redundant full-map/OAM/palette updates;
- software float/divide helpers;
- overdraw or fill pressure;
- code/data placement displacement.

Preserve a known-correct baseline and change one mechanism at a time.

## Optimization order

Prefer:

1. **Delete work** — cull, cache, stop duplicate service paths, skip unchanged
   state, reduce update rate where behavior permits.
2. **Move work earlier** — build-time conversion, setup-time preprocessing,
   precomputed tables, prepared render data.
3. **Use the correct hardware primitive** — BG scrolling, OAM, GX matrices,
   DMA for suitable transfers, hardware math.
4. **Reduce bytes and state** — compact formats, dirty ranges, batching,
   stable VRAM mappings, dense indices.
5. **Specialize dominant paths** — fixed assets, bounded entity types, hot
   kernels.
6. **Tune placement/instructions** — ARM/Thumb, TCM, inlining, assembly, DMA
   overlap, ARM7 offload.

The later steps are not inherently better; they are easier to misuse.

## Measure representative work

Use a deterministic scenario that exercises the changed path. Record:

- exact build/configuration;
- emulator/device and settings;
- input or replay;
- warm-up and capture window;
- frame/update count;
- median and tail percentiles;
- missed VBlanks/presentation intervals;
- correctness engagement flags.

A best-case microbenchmark cannot establish whole-frame improvement.

## Separate owner costs

Distinguish:

- logic/update;
- visibility/list construction;
- matrix/animation preparation;
- command construction;
- GX FIFO/geometry waits;
- 2D commits;
- texture/asset upload;
- filesystem/decompression;
- ARM7/audio/IPC waits;
- VBlank wait/idle.

A timer around a function may include hardware backpressure caused by earlier
commands. Interpret timing causally.

## Verify engagement

Every optimized path should have a debug-only way to prove it ran:

- counter;
- trace marker;
- generated-path ID;
- asset version/hash;
- rendered debug signature;
- fallback counter expected to remain zero.

A faster capture is meaningless if it skipped content or stayed on the old path.

## CPU/code generation

Inspect map/disassembly for hot paths containing:

- software floating-point helpers;
- 64-bit divide/multiply helpers;
- variable division/modulo;
- unexpected `memcpy`/`memset` from structure operations;
- out-of-line virtual/interface calls;
- spills from excessive inlining/register pressure;
- interworking/section-call overhead;
- large switch tables or cold error paths in hot code.

Use compiler reports and symbol sizes, but connect them to runtime ownership.

## Approximate cost model

Exact cycle tables are in GBATEK; the stable ratios that shape DS optimization
are:

- Cache hits and TCM accesses avoid external-memory latency. This is not a
  promise that an entire instruction, dependent load, or loop takes one cycle.
- Main RAM sits behind a 16-bit bus at half the ARM9 clock: a cache-line miss
  costs on the order of tens of ARM9 cycles, so a hot loop that misses on
  every iteration is memory-bound regardless of its instructions.
- Uncached accesses pay external access costs; batch them and prepare data in
  cached RAM or TCM when appropriate. Old uncached main-RAM mirror tricks are
  not valid on Calico-based libnds 2.x; use the runtime's supported mapping.
- The write buffer hides isolated stores but stalls on bursts and drains.

Bytes moved and working-set size are high-value starting points, not a
universal bottleneck diagnosis. A GX-, raster-, math-, or storage-bound path
needs a different fix. Recomputing a small value can beat another memory read;
check the actual access pattern and generated instructions.

## Data locality

The DS benefits from compact, sequential data:

- dense arrays instead of linked structures;
- hot/cold field splitting;
- small indices instead of pointers where capacity allows;
- sorted/batched draw items;
- fixed-capacity queues;
- contiguous generated command/data blocks;
- fewer simultaneously live representations.

Do not convert blindly to structure-of-arrays; choose based on fields consumed
together.

## DMA

DMA helps only when transfer size, alignment, cache maintenance, channel setup,
and useful overlap beat the CPU alternative. Compare:

- CPU copy/fill;
- synchronous DMA;
- asynchronous DMA with real overlap;
- conversion fused into a CPU copy;
- no copy through better ownership/layout.

“No copy” is usually the strongest option.

## TCM and ARM/Thumb

TCM placement and ARM-mode code are capacity decisions. Measure whole workload,
not one symbol.

Potential losses:

- displacement of a better resident;
- larger ARM code increasing I-cache footprint;
- veneers/interworking/call overhead;
- hot code calling cold data or vice versa;
- changed layout affecting unrelated paths.

Keep placement changes reversible and inspect the final map.

## Graphics optimization

### 2D

- update dirty map/tile/palette ranges;
- move animation to offsets/affine/OAM where appropriate;
- reuse tiles/palettes offline;
- commit OAM once;
- avoid repeated bank remaps;
- hide/cull before building entries;
- keep worst-case per-scanline OBJ load within the engine budget.

### 3D

- preconvert vertices/normals/UVs;
- remove generic interpretation from fixed assets;
- coarse-cull owners before per-part work;
- reduce matrix and material-state churn;
- batch when transparency/order allows;
- distinguish GX submission stalls from CPU preparation;
- reduce overdraw/fill as well as triangles;
- stay under the 2048-polygon/6144-vertex frame caps and the per-line raster
  budget in the worst case, not the average;
- evaluate prebuilt command streams by copy+patch+wait cost.

## Update-rate decoupling

Different systems may run at different rates when behavior permits:

- input/gameplay/hitboxes;
- skeleton pose;
- particles;
- background animation;
- lighting;
- AI;
- audio parameter updates;
- rendering/presentation.

Define interpolation, event, and timer semantics. Do not simply skip calls and
hope the game remains equivalent.

## Emulator versus hardware

Classify claims:

- logic/format correctness may be emulator-sufficient;
- exact cache behavior, storage latency, bus contention, audio stability,
  presentation timing, and certain hardware quirks require hardware or a
  validated timing model;
- one emulator cannot prove retail performance.

State the evidence class honestly.

## Reversible experiment template

For a disputed optimization, keep this lightweight:

```text
Baseline:
Changed mechanism:
Expected observable effect:
Correctness/engagement check:
Representative measurement:
Whole-frame result:
Decision:
```

This is an optional experiment aid, not a mandatory format for routine coding.

## Review checklist

- [ ] The optimized owner and mechanism are named.
- [ ] Correctness baseline and path engagement are verified.
- [ ] Representative median/tail and missed-frame behavior are compared.
- [ ] Whole-frame effects, code size, RAM, and contention are considered.
- [ ] Work was removed or moved earlier before micro-tuning.
- [ ] Emulator-only evidence is not presented as hardware proof.
