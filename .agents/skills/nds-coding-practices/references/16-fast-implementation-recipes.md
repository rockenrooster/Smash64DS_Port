# Fast First-Pass Implementation

Use one applicable row, not this entire chapter as a mandatory workflow. Hard
hardware constraints are requirements; baseline representations are defaults;
placement and scheduling optimizations are experiments. Change the latter when
measurements or project constraints justify it.

## Start with the smallest suitable design

| Need | First implementation | Escalate only when needed |
|---|---|---|
| Scrolling static 2D scenery | Offline tiles/map/palette, one upload, change BG scroll registers. | Dirty map/tile streaming when the world does not fit. |
| Small video-memory edit | Aligned halfword CPU stores; see `video_copy16.h`. | DMA for a sufficiently large or hardware-timed transfer. No universal byte threshold. |
| Repeated object updates | Bounded dense array; split hot/cold fields only when useful. | SoA, compact indexes, pool compaction after identifying the access pattern. |
| Temporary scene data | Reuse an existing arena or bounded scratch buffer. | A new allocator only when existing lifetime handling cannot express the task. |
| Static 3D geometry | DS-packed data, stable texture residency, explicit material batches. | Prebuilt GX commands when they remove interpretation rather than add copying. |
| Transform work | Setup-time constants and appropriate fixed-point formats; use GX transforms when only GX needs results. | CPU transforms for collision/culling that actually consume them, specialized kernels for hot paths. |
| Loading assets | Sequential setup/load reads into a final or bounded staging format. | Read-ahead/state machine or a supported low-priority blocking worker for real streaming. |
| Tiny IRQ handoff | Flag/counter, or a short protected copy for compound data. | Sequence locks/rings only when their extra ownership rules earn their complexity. |

Defaults are not bans: setup-time floating point, a small C++ abstraction, or a
simple pointer can be entirely appropriate. Do not invent approximation,
content reduction, custom ARM7 work, assembly, or engine-wide rewrites as the
first response to an ordinary feature request.

## API effects worth checking before copying a call

The compact current-runtime lookup is `17-libnds2-calico-facts.md`; use it for
PXI, scheduling, default stack/memory placement, formats, and debug APIs.


The following describes the reviewed devkitPro libnds/Calico implementation,
not all SDKs. Recheck the consuming project's source when it differs.

| Call/path | Important contract |
|---|---|
| `dmaCopy` | Uses DMA3 and halfword units; byte count must be even; waits for completion. Caller handles cache visibility and zero/count validation. |
| `dmaCopyWordsAsynch` | Explicit channel and byte count; caller flushes cached source and owns lifetime. Nonblocking launch does not guarantee useful CPU overlap. |
| `glCallList` | Leading word is payload length in words, excluding itself. Flushes payload, waits for all DMA channels, uses DMA0, waits for that DMA. Do not assume return ends texture use. |
| `glFlush` | Writes the swap command; do not treat return as proof that presentation completed. FIFO backpressure is a separate source of CPU waiting. |
| `DC_InvalidateRange` | Base and length are 32-byte aligned, owned cache lines; invalidation can discard dirty data. |
| `tickGetCount` | Monotonic tick units at `TICK_FREQ`, not CPU cycles. Does not require claiming the runtime's timers. |

Avoid redundant flushes just because two layers each say "flush before DMA".
Assign that operation to one owner. Conversely, never remove a required flush
because a different API happens to perform it internally. Immutable GX command
submission optimizations belong behind a compatible renderer owner, not a raw
DMA bypass added to each model.

## Arithmetic without accidental semantic changes

Choose a representation from the real range and acceptable quantization. A
32-bit fixed value may require a 64-bit multiply intermediate; retaining it can
be both correct and efficient in ARM mode. `examples/fixed_math.h` is an explicit
rounding reference. Its truncation-toward-zero is not automatically equivalent
to libnds `mulf32`'s signed-shift convention for negative products. Check negative
fractions, extrema, and overflow before substituting helpers.

For a repeatedly divided value, first ask whether setup-time computation or
algebra can remove the divide. For a necessary ARM9 divide/sqrt, evaluate the
installed hardware-math API and its shared-unit ownership. Do not mechanically
replace all divisions, or replace a correct multiply with a narrower overflow.

## Finish in proportion to the risk

For a small feature: compile/link the real target when available, check affected
capacities and lifetime, exercise the changed behavior, and report limitations.
For an optimization: also inspect the generated path and compare representative
before/after costs. A CPU helper microbenchmark does not prove frame improvement.
For a new asynchronous boundary: check completion, reuse, reset, and failure.

No claimed speedup without measurements. No mandatory benchmark infrastructure
just to choose a sensible first implementation. Source contracts and further
hardware detail are in the task-specific chapters and `SOURCES.md`.
