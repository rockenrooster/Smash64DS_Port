# N08 — Audio, storage service and measured CPU offload

> **Revision 2 coverage:** Universal scope: deadline and buffer analysis includes duplicate and mixed simultaneous cues, stage music/hazards and item/summon bursts. Offload may not change supported combinations, skip required audio or hide blocking service time. See [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md).

**Purpose:** reduce non-overlapped work and deadline tails without moving them into an unmeasured worker. Preserve the existing direct BGM/FGM paths; the plan does not rediscover them. [S21, S22, S27, H03–H06]

## Priority

Host precomputation and native fixed-function execution come first. An ARM7 job is optional and requires measured independent work and service slack. Read the actual SDK/Calico service ownership: storage/audio work may already execute partly on ARM7. Do not propose moving the same service there a second time.

Offload benefit is the critical-path work removed minus publication/copies/cache maintenance, queueing, non-overlapped completion, contention and added memory. Measure whole-frame work and cadence, not only ARM9 function self-time. Keep accurate melonDS policy; newly used bus/concurrency behavior needs model validation, not an emulator tweak to improve the result.

## Audio rules

Compile source audio into the existing supported native playback representation host-side. Convert event/pitch/pan/volume/control arithmetic to native fixed/integer chains; this does not authorize missing voice/SFX/crowd/announcer cues or fewer simultaneous audible sources. BGM has reserved buffers/service deadlines. Required one-shot cues need a distinct resident or proved deadline-safe policy. Direct file/range reads are still I/O.

## ARM7 protocol, only if a job qualifies

A descriptor contains protocol version, request ID, scene generation, job kind, validated input/output ranges/counts, deadline and bounded result/error state. One owner writes each buffer state. Enqueue only complete cache-visible input; worker output publication and ARM9 invalidation obey the chosen memory protocol. Separate control/data cache lines where they have different writers. `volatile` alone is not coherency.

States are FREE→QUEUED→RUNNING→DONE→RECLAIMED, with explicit cancellation acknowledgement/quarantine. Timeout cannot free memory while a late worker may still write. Scene changes retire generations only after all old writers are acknowledged or safely isolated. No unbounded queue and no unchecked pointer-to-object protocol.

Audio/input/storage priorities remain protected. Same-tick AI/collision is not a first offload candidate: waiting can erase the gain, and delayed results can change mechanics. A native worker failure may trigger a defined safe error or an already-proved native CPU execution before the deadline, but not missing work or an N64 graphics fallback.

### N08.01 — Attribute remaining service tails and ownership

**Depends on:** N00.03, N02.01

**Edit/inspect boundary:** `src/nds/nds_audio_bgm.c`; `src/nds/nds_audio_fgm.c`; `src/nds/nds_audio_assets.c`; `actual SDK/Calico ARM7 service configuration`.

**Implementation sequence**

1. Identify each service thread/core, storage client, synchronization point and buffer owner in the current build.
2. Classify BGM refill, FGM startup, remaining motion demand and other storage events by bytes, event frequency, blocked work and deadlines.
3. Price event-window totals instead of medians that discard infrequent refills; separate unused idle from blocking service.
4. Choose the next service mechanism only from remaining measured cost, preserving the direct-read optimizations already retained.

**Required tests/evidence:** T-AUDIO event/underrun/late-cue witness and T-MEAS exclusive service attribution; no double-counting background work.

**Work or dependency retired:** Unclassified storage clients and repeated ownership guesses; no speed credit yet.

**Done:** All recurring services have a declared owner, budget/deadline and measured interference.

**Stop/revert:** Do not claim zero I/O from zero libfat calls or assume ARM7 has spare capacity without measurement.

### N08.02 — Convert audio control and eliminate unnecessary decode work

**Depends on:** N08.01, N04.03, N04.05

**Edit/inspect boundary:** `src/nds/nds_audio_bgm.c`; `src/nds/nds_audio_fgm.c`; `src/nds/nds_audio_assets.c`; `existing host SFX/BGM generators`.

**Implementation sequence**

1. Convert pitch/rate, envelope/volume, pan and event/control arithmetic through fixed/native integer consumers.
2. Move invariant source-format conversion or sample preparation to host generators using the current supported playback API/format.
3. Keep cue identity, duration, loop/seam behavior, voice priority and audible simultaneous events correct under four-way bursts.
4. Remove completed-domain float/libm paths and repeated runtime setup that the native asset/control representation replaces.

**Required tests/evidence:** T-AUDIO source cue events, seam/finite-track completion, pitch/pan/volume bounds, burst priorities and T-FLOAT audio roots.

**Work or dependency retired:** Audio-control float and host-convertible runtime decode/setup, not audio content.

**Done:** Native control and assets meet audible/timing contracts without missing cues or added service stalls.

**Stop/revert:** Audio quality/rate/channel reductions require approval and cannot be counted as transparent code savings.

### N08.03 — Bound buffers, prefetch and storage scheduling

**Depends on:** N08.02, N02.04

**Edit/inspect boundary:** `src/nds/nds_audio_bgm.c`; `src/nds/nds_audio_fgm.c`; `scene admission and storage service seam`.

**Implementation sequence**

1. Reserve BGM and required cue buffers with explicit lifetime and worst-case service interference.
2. Choose resident or deadline-safe one-shot policy from measured latency and legal cue bursts; do not copy the BGM streaming exemption to every cue.
3. Prioritize/coalesce declared range requests only where order/cursor/loop semantics permit, and avoid filesystem discovery inside playback starts.
4. Prove fallback/error handling does not duplicate playback, reuse a live buffer or silently lose a required cue.
5. Service envelopes include legal simultaneous cues/projectiles from repeated and distinct fighters on each stage's music/hazard workload. Keep audio deadline leaders separate from CPU P95 and heap leaders; no required cue is silently dropped to hold timing.

**Required tests/evidence:** T-AUDIO delayed/short/failed read injection, simultaneous cues, loop boundaries, cancellation and source event order; T-RES reserves and zero undeclared demand.

**Work or dependency retired:** Unnecessary per-event allocation/discovery and avoidable non-overlapped service tails.

**Done:** Declared services meet deadlines under the hardest complete scene with explicit memory cost.

**Stop/revert:** A bigger buffer that causes battle admission OOM or a rare missing sound is not a win.

### N08.04 — Overlap hardware math only under proven ownership

**Depends on:** N04.03, N08.01

**Edit/inspect boundary:** `include/nds/nds_r2_hwmath_unit.h`; `src/import/battleship_gmcamera.c`; `actual IRQ/thread math-unit users`.

**Implementation sequence**

1. Inventory every divide/sqrt register writer in the current linked binary including library, thread and IRQ paths; discard old no-preemption assumptions unless re-proven.
2. For a measured remaining independent operation, split start/consume and schedule useful native work between them without allowing another writer to overwrite the result.
3. Define ownership/save-restore or bounded critical section at the shared-unit boundary; do not add broad interrupt masking at every call.
4. Compare with the already-existing synchronous native helper, including ownership overhead and interrupt/service effects.

**Required tests/evidence:** T-HWMATH overlapping/cancelled/preempted operations and exact domain results; T-AUDIO IRQ deadline integrity; whole-frame timing.

**Work or dependency retired:** Only actual exposed wait latency for a qualified math chain.

**Done:** A retained asynchronous schedule improves total cost with correct cross-user ownership, or the synchronous route remains.

**Stop/revert:** Do not count hardware math as newly implemented or adopt overlap on a stale register-writer audit.

### N08.05 — Select and price one ARM7 candidate

**Depends on:** N08.01, N08.03

**Edit/inspect boundary:** `actual ARM7 build/service entry points`; `existing shared-buffer/IPC APIs`; `proposed bounded worker job descriptor`.

**Implementation sequence**

1. Select one coarse independent job with measured critical-path cost and demonstrable ARM7 service slack, such as preparation of non-immediate service data.
2. Compute input/output bytes, copy/cache cost, queue latency, deadline, cancellation and resident-memory budget before writing the worker.
3. Prototype an equivalent native CPU control and one bounded worker implementation without displacing SDK audio/input/storage services.
4. If no job qualifies, close this optional task as NOT_SELECTED with evidence; do not manufacture an offload requirement.

**Required tests/evidence:** T-IPC protocol schema/ranges/generations and baseline slack; T-MEAS end-to-end work/deadline evidence.

**Work or dependency retired:** A named non-overlapped job only if measured benefit exceeds communication/contention costs.

**Done:** One candidate has a defensible protocol and total-time case, or ARM7 offload is explicitly not needed.

**Stop/revert:** No same-tick AI/collision offload with new latency; no second ARM7 storage service that duplicates an existing one.

### N08.06 — Prove worker coherency, cancellation and net gain

**Depends on:** N08.05

**Edit/inspect boundary:** `bounded worker/IPC implementation if selected`; `shared-buffer allocator/publication seam`; `actual ARM7 service scheduling`.

**Implementation sequence**

1. Implement C7/C5-style explicit ownership with completed cache-visible inputs and uniquely owned output buffers outside TCM.
2. Handle queue-full, timeout, late completion, scene-generation retirement and cancellation acknowledgement without freeing a possible live writer.
3. Inject ordering/delay/failure races and prove services remain responsive; a late result cannot change another scene’s state.
4. Measure CPU work, total frame/cadence, deadlines and memory versus the control. Retain only a whole-system winner; otherwise remove the worker/queue.

**Required tests/evidence:** T-IPC stale reply, same-address reuse, cancellation race, full queue and double publication; T-AUDIO deadlines; serial uncontaminated timing.

**Work or dependency retired:** Actual critical-path ARM9 work after all worker overheads, or removal of a failed optional prototype.

**Done:** A selected offload is safe and faster end to end; an unselected/failed route leaves no permanent framework.

**Stop/revert:** Never report a faster ARM9 bracket while the join, DMA contention or audio underrun moved elsewhere.

### N08.07 — Close audio/service runtime numerics and reachability

**Depends on:** N08.03, N04.06

**Edit/inspect boundary:** `src/nds/nds_audio_bgm.c`; `src/nds/nds_audio_fgm.c`; `actual ARM7/shared runtime sources`; `proposed no-float gate`.

**Implementation sequence**

1. Audit both processor/service build roots and all cold error/transition/control paths for remaining floating arithmetic.
2. Remove completed-domain native/legacy duplicate service state and experimental routes not retained.
3. Preserve platform startup/shutdown/return-to-loader and established service lifecycle while narrowing the arithmetic/runtime surface.
4. Publish declared service classes, deadline evidence and the final fixed-runtime service result with matching build identity.

**Required tests/evidence:** T-FLOAT both target cores/services, T-AUDIO cue/loop/shutdown and T-LIFE transitions; no unknown arithmetic roots.

**Work or dependency retired:** Remaining reachable audio/service float and retired worker/control scaffolding.

**Done:** Services satisfy the owner’s runtime-fixed requirement, independently of battle-loop arithmetic.

**Stop/revert:** Do not exempt cold audio/control code from the all-runtime endpoint or alter library internals without a reproducible build.

