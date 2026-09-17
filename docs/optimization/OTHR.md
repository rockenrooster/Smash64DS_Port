# Smash64DS: OTHR optimization and frame-service redesign

**Review date:** September 17, 2026  
**Repository:** rockenrooster/Smash64DS_Port  
**Source revision reviewed:** `35531554bc0aacc6b09cacad84be8f99b4200c68` (`master` when the review began)  
**Deliverable:** Source-grounded optimization candidates, implementation sketches, exclusions, and an executable validation plan. No repository files were changed.

## Executive decision

**Do not treat OTHR as another large renderer or simulation subsystem. Its displayed value includes intentional VBlank waiting. The useful targets are active frame-service overhead, an unaccounted publication tail, and unnecessary service-thread residency.** The best architecture is a small, explicit frame-service layer—not another general-purpose scheduler, renderer, or runtime cache.

The strongest new candidates found in this review are:

| Priority | Candidate | Concrete opportunity | Classification |
|---|---|---|---|
| 1 | Eliminate the permanently dormant audio boot service | Thread 4 acknowledges startup, then blocks forever on a private queue with no producer. Avoid its requested **16,384-byte coroutine stack**, and stop revisiting it during frame dispatch. | Source-confirmed mechanism; ROM memory/timing savings unmeasured |
| 2 | Publish debugger state by unique cache lines, not separately by every member | Frame-complete publication runs even without the tick HUD. Its macro invokes `DC_FlushRange` per symbol, potentially revisiting shared cache lines and paying repeated call/setup costs. | Source-confirmed mechanism; unique-line count and cost need the linked ELF |
| 3 | Make the cooperative OS pump predicate-aware | The pump currently resumes every `WAITING` coroutine, including those still waiting on an empty/full message queue. | Source-confirmed mechanism; savings depend on live waiter population |
| 4 | Remove queue division and linear current-thread lookup | Three queue index operations use variable `%`; current-thread resolution scans a 64-entry registry. | Arithmetic independently tested; target-ROM benefit unmeasured |
| 5 | Separate frame accounting from FPS presentation | Pacing computes two 64-bit divisions each presented frame. Raw counters can remain exact while derived display statistics are produced less often or off-target. | Source-confirmed mechanism; consumer-contract audit required |
| 6 | Replace the profiler's strided ring with contiguous frame records | The tick-HUD recorder scatters one frame's 20 words across a bucket-major 128-frame ring. Use the same payload bytes in frame-major order, with explicit sample identity. | Measurement-build optimization, not a shipping gameplay win |
| 7 | Make native post-VBlank ownership explicit only where measured | Preserve existing dirty guards; optimize actual commit work, not the mere presence of multiple guarded calls. | Conditional candidate; not yet sized |
| 8 | Investigate avoidable presentation-boundary slips | An unconditional wait could be redundant only in a narrowly proven phase/latch case. Never remove required VBlank synchronization just to lower OTHR. | Falsification experiment; not an approved implementation |

**Recommended first implementation package:** one-shot audio boot acknowledgement, predicate-aware queue waiting, O(1) current-thread identity, and division-free queue indices. Measure debugger publication separately because it crosses the current timing boundary. These changes can be implemented in ordinary C. Use assembly only when the actual target disassembly demonstrates a remaining problem.

The 16 KiB opportunity is particularly useful because it attacks residency as well as CPU overhead. It is not permission to allocate a new 16 KiB cache: first establish whether those bytes improve the taskman arena, libc reserve, or neither. [S05][S06][S07][S08][S09][S10]

### What has and has not been proved

This review inspected the relevant frame host, platform, OS/coroutine implementation, boot stub, original startup/scheduler, native commit entry guards, measurement scripts, recent performance receipts, and DS reference implementations. It also ran **5,691,218 host-side arithmetic comparisons** for proposed queue index replacements and compiled an isolated example for ARM946E-S using Clang 17.

**No new game ROM was built, no new emulator performance run was performed, and no new FPS/tick reduction is claimed.** The host arithmetic test is not a scheduler test, and the isolated Clang assembly is not the project's devkitARM linked binary. Existing repository measurements are identified below by their original artifact and configuration.

## 1. The goal this work must actually serve

The project goal is the whole game on the original DS, with native DS rendering and mechanically equivalent gameplay. The central performance requirement is nominal 30 FPS, approximately **1.12 million timer ticks per presented frame**, with P95 at or below that work budget and only rare exceptional overruns. The P2 stress requirement also includes four CPUs, items enabled, and at least 95% two-VBlank presentation cadence. Current scene cadence, visible content, audio, and correctness remain part of the acceptance conditions. [S01]

This OTHR proposal does not reduce source updates, skip AI, remove effects, drop geometry, mute audio, or introduce an N64 graphics interpreter. It preserves the existing source-update and presentation schedule. The project permits broader compensated-rate changes under its own equivalence/approval rules, but none is needed to implement these overhead candidates. [S01][S02]

The accuracy-focused melonDS configuration remains the development measurement reference. Do not switch emulator timing models or enable JIT between control and candidate. Do not use a hardware-measurement requirement to stall this investigation; use the project's configured authoritative measurement workflow. [S01]

## 2. OTHR's actual accounting

### 2.1 The top-level identity

`ndsBattlePlayableFinalizePresentedIteration()` in `src/port/taskman_seam_battle_host.c` assembles the buckets approximately as follows. The names below describe the existing calculation, not a proposed profiler:

```text
misc_draw = max(total_draw_ticks
                - fighter_ticks - stage_ticks
                - background_ticks - foreground_ticks, 0)
            + flush_ticks

HUD  = foreground_ticks + platform_HUD_ticks
MISC = misc_draw

NAMED = FTR + STG + BG + AUD + HUD + SRC + MISC
ALL   = timestamp_at_finalizer - iteration_start_timestamp
OTHR  = max(ALL - NAMED, 0)
WAIT  = measured scheduled-VBlank wait
WORK  = max(ALL - WAIT, 0)
```

Where the partition closes without saturation:

```text
ACTIVE_OTHR[i] = OTHR[i] - WAIT[i]
WORK_H[i]      = ALL[i] - WAIT[i] - HUD[i]
              = FTR[i] + STG[i] + BG[i] + AUD[i]
                + SRC[i] + MISC[i] + ACTIVE_OTHR[i]
```

`SHDT`, `SWRM`, `GCRA`, `SCPU`, and the other appended source-detail buckets are nested source measurements. They are not additional terms in the top-level work sum. The code specifically appends them without adding them again to the named total. [S02]

**Consequences:**

- A faster renderer can produce more WAIT and therefore more OTHR. That can represent more spare frame time, not a regression.
- `ndsRendererFighterPacketDmaWait()` and `glFlush()` are inside the measured flush region, which is assigned to **MISC**, not generically to OTHR.
- An inline FIFO or memory stall belongs to whichever measured interval contains it. There is no mechanism that automatically transfers all GPU backpressure into OTHR.
- Moving a call to a different timing bracket is not a speedup.
- Negative residuals or saturated `max(...,0)` paths must be investigated, not silently accepted as clean accounting. [S02][S04]

### 2.2 The timing tail the existing ALL bracket misses

The finalizer captures `ALL` **before** some of its bookkeeping, ring sampling, and debugger publication. The subsequent iteration establishes a fresh loop-start timestamp. Consequently, some work between those timestamps is not charged to the current ALL/WORK/OTHR sample—or to the next sample—but can still affect presentation cadence. [S02][S04]

A useful conceptual timeline is:

```text
iteration start timestamp
  source updates + audio
  frame/graphics preparation
  native draw
  foreground/HUD
  packet completion + GX flush             -> MISC's flush component
  scheduled VBlank wait                    -> WAIT, also inside OTHR
  native post-VBlank commits               -> active OTHR
  OS retrace publication / coroutine pump  -> active OTHR
  pacing / other bookkeeping               -> active OTHR
ALL snapshot
  bucket construction / diagnostic reads
  tick-HUD ring sample                     -> currently excluded tail
  coherent debugger-group publication      -> currently excluded tail
  frame-complete marker / heartbeat
next iteration start timestamp
```

The exact instructions on either side of a boundary are configuration-dependent; preserve the existing measurement ABI while adding a separate tail measurement. Do not claim every finalizer instruction is inside OTHR.

**Add a contiguous-boundary metric:** timestamp the same phase on successive completed iterations, then separately record WAIT and capture/publication overhead. Also measure time from the previous presentation boundary to submission readiness, because a short tail can consume the next frame's deadline margin. Calibrate the measurement's own overhead with both arms carrying identical instrumentation.

This distinction is why debugger-publication optimization deserves attention even when the displayed OTHR barely changes.

## 3. What the available measurements say

### 3.1 A qualified September 17 checkpoint

The September 17 `ftr-stg-misc-sizing` receipt reports this qualified four-fighter checkpoint, identifying `a4eb24c9a85` as its reference. These are the receipt's recorded measurements, **not a new benchmark of the review revision**. [S03]

| Metric | P50 ticks | P95 ticks |
|---|---:|---:|
| ALL | 1,677,952 | 2,798,144 |
| FTR | 350,144 | 736,960 |
| STG | 385,088 | 427,648 |
| SRC | 543,040 | 1,027,520 |
| MISC | 238,720 | 464,000 |
| OTHR | 284,096 | 560,384 |
| WAIT | 256,512 | 532,224 |
| HUD | 20,160 | 455,168 |
| AUD | 3,712 | 122,240 |
| WORK-H | 1,600,960 | 2,320,576 |

Against a 1,120,000-tick work gate, this checkpoint has a **480,960-tick P50 gap** and a **1,200,576-tick P95 gap**. Those are simple differences of the reported whole-work statistics, not savings estimates for OTHR.

The table makes clear why raw OTHR is misleading. Nevertheless, **284,096 − 256,512 = 27,584 is not a measured median of active OTHR**. The subtraction must be done on matching frames before taking the percentile.

### 3.2 A real per-frame active-OTHR measurement exists

The September 14 `fourcpu-attribution/ring-analysis.txt` is especially relevant because it analyzes matching rows rather than subtracting independent percentiles. In its 1,972-row workload:

- `OTHR-WAIT` has a **28,032-tick median**.
- The top-level accounting identity closes with per-frame maximum error **0**.
- The active-OTHR hot-frame mean is **1,763 ticks below** its clean-frame mean.
- Its flatten-to-median experiment reports **zero P95 improvement** from flattening active OTHR in that particular trace. [S11]

This was an older and materially slower workload, so do not transplant 28,032 as an exact current-head cost. It is direct evidence that the apparent hundreds-of-thousands-of-ticks OTHR bucket can conceal only tens of thousands of active overhead, and that active OTHR did not cause the large over-budget excursions in that trace.

The zero result is also narrow: flattening a lane to its median is not deleting its steady cost. It does not prove that queue/pump work is free, and it says nothing about the excluded tail or the consequences of recovering resident memory.

### 3.3 Important correction to the sizing logic

The September 17 sizing receipt argues that marginal bucket percentages exceeding 100% demonstrate overlapping lanes. **That inference is not valid from medians.** Even disjoint per-frame components need not satisfy:

```text
median(A + B) = median(A) + median(B)
```

For example, with three frames:

```text
A = [0, 100, 100]
B = [100, 0, 100]
A+B = [100, 100, 200]

median(A) + median(B) = 200
median(A+B)            = 100
```

These lanes are completely disjoint. The same issue applies to P95 subtraction and to statements such as “deleting subsystem X leaves median(WORK) − median(X).” That is not a rigorous per-frame deletion bound. This is a mathematical correction, not a claim that arbitrary rewrites will recover their whole bucket.

Use the existing source identity and validate it on rows. For a **hypothetical** perfect deletion ceiling:

```text
counterfactual[i] = WORK_H[i] - recoverable_candidate_cost[i]
P95_ceiling_gain  = P95(WORK_H) - P95(counterfactual)
```

Then label it as a counterfactual upper-bound model, not a measured speedup. Do not include WAIT, required commits, safety checks, or nested child costs twice in `recoverable_candidate_cost`.

### 3.4 Do not re-sell the already implemented DTCM experiment

The separate September 17 `dtcm-hot-scalars` receipt reports a 508-byte placement experiment with WORK-H reductions of 43,200 / 43,072 ticks in one configuration. It subsequently records an IRQ-table alignment correction, sample-label complications, and a final status of **`IMPLEMENTED_NOT_ACCEPTED`**. Its configurations differ from the qualified table above. [S12]

Therefore “move hot globals to DTCM” is neither a new OTHR candidate nor a number to add to this report's prospective savings. Its useful lesson is narrower: compact state placement can have cross-bucket effects, but actual initialization, IRQ alignment, stack reserve, and acceptance status must be checked. The receipt's final alignment-corrected layout leaves 1,460 bytes below its DTCM ceiling; that is configuration-specific, not a general allocation entitlement.

## 4. Candidate O1 — replace the dormant audio boot coroutine with a one-shot service

### Source finding

`src/port/boot_stubs.c` implements `syAudioThreadMain()` as a call to `ndsBootServiceThread(NDS_BOOT_AUDIO_READY)`. That helper:

1. Creates a one-message queue on its own stack.
2. Sets the audio-ready boot bit.
3. Sends one acknowledgement to `gSYMainThreadingMesgQueue`.
4. Blocks forever receiving from its private queue.

The private queue has no published producer. The helper never does mixing, sample delivery, or music advancement. The original startup creates this as **thread 4**; the port imports that startup translation unit unchanged. [S05][S06][S07]

`osStartThread()` requests a **16 KiB heap stack** for a service thread whose coroutine has not been created. Actual audio backend updates are called from the source-update host path. This makes the dormant boot service a concrete residency and dispatch candidate, not a proposal to remove sound. [S02][S08]

### Proposed implementation

Replace the port-side thread-4 startup with a **one-shot native boot acknowledgement**. Preserve the boot-ready bit and acknowledgement ordering. Do not create the unused coroutine stack in the first place.

Prefer an explicit port seam for this particular startup operation. Do not scatter numeric `thread->id == 4` exceptions throughout the OS implementation. The decompilation remains unchanged; the port-side startup adaptation owns the difference.

Preserve or intentionally adapt the thread-4 descriptor and legacy stack/canary expectations that remain externally observed. Check all readers of the thread descriptor before omitting registry membership. The original startup's create/start/receive sequence is the ordering reference.

### Critical trap

**Changing the stub's infinite loop into `return` is not sufficient to recover the allocation.** `portCoroutineResume()` can mark a coroutine finished without freeing its stack. `portCoroutineDestroy()` is a separate operation. Never free the stack while executing on it. [S09]

Avoiding allocation is cleaner than returning and adding a deferred-free protocol. It also gives the scene arena a chance to benefit before it is sized, rather than leaving an allocator hole behind.

### Expected benefit and limits

Source-sized opportunity: one requested **16,384-byte service stack**, plus whatever coroutine allocation overhead and dispatch visits are demonstrably removed. This is not a measured 16 KiB increase in the taskman arena.

The allocator can absorb savings into libc free space, fragmentation, or a different arena rounding decision. Record `gNdsTaskmanArenaChosenSize`, libc high-water/top-chunk reserve, scene free minimum, and actual live thread/stack census before crediting arena recovery. Do not claim that this recovers all service stacks; the gameplay, controller, scheduler, and nested caller contexts have different lifetimes.

### Acceptance / kill criteria

Cold boot, shell entry, match entry, BGM/SFX, pause, Results, rematch, and return-to-shell must remain correct. Boot acknowledgement count and ordering must match. Verify that thread 4 no longer allocates a coroutine stack and no longer enters the runnable/waiting pump.

Kill the proposed implementation if it changes audio service cadence, startup ordering, live stack ownership, or scene lifecycle. A verified memory saving can be valuable even when its direct CPU saving is below the cross-build timing noise floor; report those as separate outcomes.

## 5. Candidate O2 — coalesce coherent debugger publication

### Source finding

`ndsPlatformPublishBattleFrameCompleteGroups()` publishes several groups unconditionally, including pacing, pacing histograms, taskman state, and native-effect witnesses. This is not just a tick-HUD feature. The implementation deliberately preserves coherence for the debugger, which reads main memory rather than dirty ARM9 cache contents. [S04][S10]

The X-macro definition expands each member into its own:

```c
DC_FlushRange((const void *)&symbol, sizeof(symbol));
```

This is correct but potentially expensive. Separate symbols can occupy the same cache line, so a single completed frame can pay repeated cache-range setup and repeated operations on that line. The number of duplicate lines cannot be established from declarations alone: it depends on the linked addresses. [S10]

### First implementation: preserve the existing debugger ABI

At startup, expand the existing group lists into address ranges, round each range to the actual cache-line granularity, and build a deduplicated set of cache-line spans. Do this once, not once per frame. Publish each unique span once immediately before the existing stop marker.

Keep the existing group lists as the single source of truth. Handle members that are arrays or cross a line boundary. Do not assume linker declaration order, do not flush the giant interval between the lowest and highest unrelated symbol, and do not omit the final write-buffer/coherency action required by the SDK operation.

Price the address list against RAM. A runtime list costs metadata; a post-link generated table complicates the build; a linker-defined contiguous publication section can avoid some metadata but changes layout. Choose the smallest measured implementation, not the most abstract one.

### Second implementation, only when justified: one publication block

A compact publication record can replace scattered debugger-only state or hold a snapshot of required runtime counters. It must identify the completed frame and preserve cross-field consistency. A main-RAM block can be cache-cleaned once; a carefully bounded TCM block has different access/coherency properties.

Do not blindly duplicate all globals into a large shadow structure. Snapshot-copy reads, extra residency, and dirtying cache lines can consume the expected saving. Do not maintain both the old flush path and the new publication path indefinitely and call that an optimization.

The legacy-marker-compatible unique-line path is the lower-risk first experiment. A packed-record ABI requires coordinated changes to every reader and separate proof of sample identity.

### What to measure

Record publication ticks, number of `DC_FlushRange` calls, unique cache lines, bytes/spans cleaned, and excluded-tail ticks. Run an unchanged game with both publications in a temporary shadow-equivalence test; compare all members at the same completed frame. Then benchmark a single publication owner, not both.

A lower tail cost with unchanged ALL can be a real improvement. A smaller OTHR number caused by moving publication outside a timer is not. The objective is less actual work and no torn/stale debugger observations.

### Safety boundaries

`volatile` alone is not cache coherence. Do not delete the flushes, disable the data cache, remove native failure witnesses, or alter the published-frame marker to evade validation. The project has already documented real torn-observation problems here. [S04][S10][E01]

## 6. Candidate O3 — stop waking threads whose queue predicate is still false

### Source finding

`ndsOsRunThreads()` scans 64 registry slots. For each valid coroutine, it resumes both `OS_STATE_RUNNABLE` and `OS_STATE_WAITING` threads. `ndsOsWaitForQueue()` sets WAITING, yields, and rechecks the queue when resumed. No queue predicate is retained for the pump to inspect first. [S08]

Thus a thread blocked on an empty queue can pay resume, state updates, queue recheck, current-thread resolution, and yield on every frame despite having nothing to do. The dormant audio boot service is a provable instance. Other live services must be counted, not guessed.

### Minimal redesign

Retain the current fixed registry and traversal order. Add a compact port-owned wait descriptor for each blocked OS thread:

```text
wait_queue: pointer, or null
wait_kind:  not-waiting / needs-message / needs-space
```

When a blocking queue operation is about to yield, record the predicate. On return, clear or update the descriptor as the original wait loop requires. Immediately before a would-be resume, test the actual queue state:

```c
/* Concept sketch: integrate with the real OSThread/queue types and lifecycle. */
if (thread->state == OS_STATE_WAITING && thread_has_queue_wait(thread)) {
    if (!recorded_queue_predicate_is_satisfied(thread)) {
        continue;
    }
}
resume_thread_as_before(thread);
```

**Do not snapshot readiness once at the beginning of the pump.** A service earlier in the existing slot order can enqueue a message for one later in the same pump. Checking immediately before that later slot preserves the current opportunity to run. Conversely, do not immediately execute a woken earlier-slot thread when the old pump would wait until the next pass.

Unknown/non-queue WAITING states retain the old behavior until explicitly understood. This makes the first change a removal of fruitless resumes, rather than a new scheduling policy.

### Memory choices

An added pointer plus compact state can round to roughly 8 bytes per descriptor on a 32-bit layout, but calculate the actual struct/pool impact. A 64-entry side table would be about 512 bytes before other metadata. Avoid allocating that by default when an existing port-owned field or compact side representation suffices.

Never trade a tiny dispatch saving for an arena-size regression. Preserve data/BSS initialization rules and the current DTCM/IRQ layout. [S08][S12]

### Semantic tests

Exercise empty receive, full send, nonblocking operations, jam-to-front, two consumers, multiple senders, recursive `osStartThread`, stop/restart, destruction, and scene-arena rewind. Clear descriptors before a referenced queue or thread becomes invalid. Retain `ndsOsForgetThreadsInArena()`'s lifecycle protection.

Original retrace processing sends scheduler-client messages and runs scheduler work; it is not just an idle notification. Preserve the same source tick restoration after presentation. [S02][S08][S13][S14]

### More aggressive second stage

A native fixed service pump can replace selected always-live services with explicit handlers only after their message contracts and yield boundaries are enumerated. The one-shot audio boot stub is suitable now. A wholesale scheduler/controller rewrite is not justified by the evidence gathered here.

The DS references support the architectural direction, not a drop-in implementation: `sm64-nds` directly enters its game loop and routes selected services through explicit callbacks, while the inspected SM64DS VBlank handler wakes specific wait sites. Neither proves that SSB's scheduler clients can be discarded. [S15][S16]

## 7. Candidate O4 — division-free queues and O(1) current-thread identity

### 7.1 Queue index arithmetic

The current message queues use runtime modulo for send, jam, and receive indices. The queue invariants allow exact conditional wrapping. This works for **non-power-of-two capacities** and needs no reciprocal table or hardware divider. [S08]

```c
#include <stdint.h>

/* Preconditions established by the queue implementation:
 * n > 0; first < n; count < n for a successful send.
 * n originates from a positive signed-32-bit queue capacity.
 */
static inline uint32_t queue_send_slot(uint32_t first,
                                       uint32_t count,
                                       uint32_t n)
{
    uint32_t slot = first + count;
    if (slot >= n) slot -= n;
    return slot;
}

static inline uint32_t queue_next_slot(uint32_t first, uint32_t n)
{
    uint32_t slot = first + 1u;
    return slot == n ? 0u : slot;
}

static inline uint32_t queue_prev_slot(uint32_t first, uint32_t n)
{
    return first == 0u ? n - 1u : first - 1u;
}
```

For send, `first + count < 2*n`, so at most one subtraction is required. The unsigned addition remains representable for the stated signed-32-bit capacity range. Preserve the original validation, blocking, count updates, message storage, and message ordering around these replacements.

**Do not replace `% n` with `& (n-1)` unless that particular queue's capacity is proven to be a power of two.** These generic queues support arbitrary valid capacities.

### Independent test performed in this review

The three helpers were compared to mathematical/modulo references for all valid index states at capacities **1 through 256**, with additional boundary cases at capacities 257, 1,024, 65,535, 65,536, 1,073,741,824, and 2,147,483,647. The host test used GCC, assertions, and undefined-behavior sanitization:

```text
PASS: 5691218 arithmetic comparisons; capacities 1..256 exhaustive,
six larger capacities boundary-tested.
```

This proves the tested arithmetic behavior, not message-queue scheduling or DS timing.

For an assembly-only demonstration, use external, non-inline definitions of the helpers (remove both `static` and `inline`). An isolated freestanding Clang 17 compile using:

```sh
clang --target=arm-none-eabi -mcpu=arm946e-s -marm -O2 -S \
      -ffreestanding queue_wrap.c -o queue_wrap_arm.s
```

produced this send-slot implementation:

```asm
add     r1, r1, r0
subs    r0, r1, r2
movlo   r0, r1
bx      lr
```

The isolated unsigned modulo equivalent called `__aeabi_uidivmod`. That is evidence that ordinary C can express the desired small ARM sequence. **Inspect the project's own GCC-generated code before attributing that helper call or instruction sequence to the shipped ROM.** Its original queue operands are signed, and inlining/constant propagation can change the actual result.

### 7.2 Current-thread identity

`ndsOsCurrentThread()` gets the current coroutine and then scans the registry for the matching `port_coroutine`. The coroutine layer already maintains a current-coroutine pointer through nested resume/yield operations. [S08][S09]

Introduce an O(1) OS-thread identity at the OS resume wrapper, saving the caller identity and restoring it after the child yields or finishes. Alternatively expose a typed, validated owner association from the coroutine. Do not blindly cast arbitrary coroutine arguments to `OSThread *`.

A simple wrapper can require only one 32-bit current-thread pointer, with the previous pointer held in the caller's existing stack frame. Every OS resume path—both `osStartThread()` and `ndsOsRunThreads()`—must use it. Nested resumes and returns must restore the parent, and non-OS coroutine execution must not leave a stale OS identity.

There is no need to replace the small register-save assembly first. Removing needless resumptions and scans is structurally better than shaving instructions from resumptions that should not occur.

### Attribution and acceptance

These queue/identity functions also run from source-update contexts. Credit their savings to the brackets where they actually execute. A combined queue/pump change must not add separately measured parent and child savings together.

Count queue operations, unsatisfied resumes avoided, registry entries examined, context switches, and whole-work/tail changes. Small local wins may require same-ROM A/B and repeated measurements before a production bundle can clear the repository's historically used 14,080-tick cross-build significance floor. That floor is a property of the recorded experiment, not a universal constant of DS hardware. [S03][S12]

## 8. Candidate O5 — keep exact counters, stop recomputing long-term FPS every frame

### Source finding

`ndsBattlePlayablePacingUpdate()` calculates both presented FPS and logic FPS with a 64-bit numerator divided by elapsed ticks. It is invoked on the presented-frame path. This is distinct from the visible FPS HUD's already-throttled refresh: optimizing the latter does not remove these per-frame calculations. [S02][S04]

### Proposed redesign

Keep the authoritative time, frame counts, source-update counts, cadence histogram, and lifecycle state updated exactly as today. Separate their derived display/diagnostic values:

```text
Every frame: counters and timestamps required for correctness/validation.
At an existing publication/display interval: derived FPS values.
At a debugger stop: host computes additional ratios from exact raw counters.
```

Audit the readers of `gNdsBattlePlayablePacingPresentFpsX10` and its logic counterpart first. A harness that requires a fresh ratio at every frame must either receive an equivalent fresh result or be deliberately adapted to calculate it from matching raw values. Silently leaving a stale field while claiming the same ABI is incorrect.

An alternative is to change only the measurement contract, replacing redundant derived fields with raw data while keeping the production game loop independent. A constant-time or reciprocal arithmetic replacement should be considered only after the much simpler consumer separation is evaluated; do not introduce an approximate gameplay clock.

### Expected scale and rejection rule

This is a steady overhead reduction, not a hundreds-of-thousands-of-ticks subsystem replacement. Its ceiling is the measured cost of these divisions and related derivation work, less whatever replacement publication work remains. Keep the exact logic/presentation relationship and marker semantics.

## 9. Candidate O6 — a contiguous, self-identifying profiler record

### Source finding

The tick-HUD ring is declared as:

```c
static volatile u32
    sBattleTickHudRing[nNDSTickHudBucketCount][128];
```

There are 20 bucket entries at the reviewed revision. The per-frame writer iterates buckets and writes `[bucket][head]`: **80 bytes of data scattered with a 512-byte stride**. The ring payload is **10,240 bytes**. [S04]

### Proposed layout

Use frame-major storage for capture-only operation:

```c
/* Layout concept; keep ABI/version and reader updates explicit. */
struct TickFrame {
    u32 bucket[nNDSTickHudBucketCount];
};
static volatile struct TickFrame frames[128];
```

This changes no payload size. The writer streams one frame's contiguous 80 bytes. The host can transpose for per-bucket statistics after collection.

An explicit frame/iteration identifier is worth considering because the recent DTCM receipt documents derived sample-label problems. One extra 32-bit identifier costs **512 bytes across 128 records**. It does not automatically fix the sampler: records still need coherent publication, wrap handling, and proof that the identifier labels the exact stored sample. [S12]

Do not automatically round every record to 96 bytes: that would grow an 80-byte-by-128 payload by **2,048 bytes**. Price any added fields and alignment deliberately.

### Important cache qualification

The strided layout touches more distinct addresses/cache lines, but that is **not proof of 20 cache linefills per frame**. The project's ARM946E-S coherency notes explicitly describe no write allocation. Store misses can reach memory without filling cache lines. Actual cost depends on whether a line is already resident, HUD reading, cache cleaning, bus behavior, and compiler-generated stores. [S10]

This is a source-grounded locality experiment, not a measured cache-miss claim. Compare the exact writer and publication sequence on the target.

### Keep the benefit honest

This code exists in tick-HUD capture configurations. Removing its cost is a profiler-quality improvement unless the shipped configuration contains the same path. Do not credit a capture-only improvement as a production gameplay optimization.

Do not simply disable the recorder: paired, per-frame evidence is necessary. Update all readers to the explicit format/version, retain complete sample coverage, and prove there are no dropped, duplicated, torn, or mislabeled iterations. The DTCM receipt's label ambiguity is a reason to strengthen sample identity, not to weaken the performance gate.

## 10. Candidate O7 — native post-VBlank commits: optimize measured work, not guarded call sites

`ndsPlatformEndFrame()` waits, then applies fades/blackout, wallpaper affine state, pending texture refreshes, IFCommon OAM, Results OAM, and the UI kit. This is active frame-service work outside the flush timer. [S04]

However, the IFCommon commit already checks its needs-commit flag, and Results checks both active state and its needs-commit flag. The platform also documents that UI and battle OAM tenants are not simultaneously live in the same scene. Merely seeing multiple commit calls is **not evidence of multiple full OAM transfers every battle frame**. [S17][S18][S19]

### Conditional redesign

First measure the costs and actual writes of each owner. If meaningful overhead remains, make the currently active scene's commit responsibility explicit, with a small dirty mask and a single commit phase. Keep generation/scene-transition invalidation: scene entry, scene exit, clear/hide operations, and the first visible frame must still commit even when ordinary content is unchanged.

Partial OAM or palette updates are candidates only after an actual redundant-copy population is demonstrated. Preserve affine/OAM slot structure, alignment, transfer ownership, and old-sprite clearing. Do not blindly replace a tested SDK commit with byte stores to hardware.

Pending texture refresh is also not automatically removable. This review traced its call boundary but did not establish a new recoverable cost inside the large renderer implementation. Retain required animated state; measure queue length, actual uploads, and post-VBlank transfer cost before proposing a redesign.

A renderer-owned texture-DMA barrier before `glFlush()` belongs to MISC. Do not count its removal again here. DMA cannot read TCM or dirty ARM9 cache contents, and DMA/CPU work is not freely parallel when both require contested external memory. Any DMA experiment must include coherence and bus-stall costs. [S04][E01][E02]

**Current ranking:** below the source-confirmed boot, OS, and publication candidates. Existing dirty guards make “only commit when dirty” an already-partially-implemented idea, not a new large win.

## 11. Candidate O8 — a phase-aware presentation experiment, not “remove WAIT”

The current scheduled wait performs `swiWaitForVBlank()` at least once, then repeats until the earliest permitted VBlank is reached. The realtime host schedules at least two VBlanks after the previous presentation. [S02][S04]

An unconditional wait might be avoidable only if the application is already in the correct safe interval **and** the frame's GX submission/latch and 2D commits can still meet that interval. Those conditions cannot be established from `current_vblank >= target` alone.

Before attempting a change, capture a phase trace around submission readiness, final packet completion, flush, VBlank entry, commit completion, and actual presentation. Keep rendering correctness and the actual visible-frame boundary in the trace. Determine whether any overrun is an unnecessary whole-blank wait rather than genuine missed submission work.

Only then consider a narrowly guarded wait path. It must handle counter wrap, a target already missed, a frame submitted too late for the current latch, long commits, and the next scene's first frame.

**Not acceptable:** removing the wait, advancing the presented counter without a newly presented native frame, changing the budget definition, repeatedly showing the previous frame while reporting 30 FPS, or lowering source updates to make the measurement look green.

The required cadence is an output property. WAIT is often the remaining capacity inside that cadence. There is currently no measured evidence in this review that a safe wait-path change would recover one extra presentation interval.

## 12. The proposed end-state architecture

A small frame-service architecture is enough:

```text
BOOT
  Original required boot ordering
  One-shot acknowledgement for services with no runtime job
  Allocate only genuinely live coroutine stacks
  Build compact publication spans once, when used

SOURCE UPDATE(S) — existing schedule preserved
  Existing controller ownership and source callbacks
  Existing audio backend update
  Existing gameplay state and RNG behavior

NATIVE FRAME PRODUCTION
  Reset required bounded graphics/stream state
  Draw native stage, fighters, effects, foreground/HUD
  Complete required packet DMA and submit GX flush

PRESENT
  Wait for the legitimate target interval
  Commit required native 2D/texture/fade state once per owner
  Post retrace/client messages
  Resume only queue-ready/runnable services in existing order
  Restore the established source-clock semantics

ACCOUNT / PUBLISH
  Update exact raw counters
  Record one correctly identified measurement sample, when enabled
  Publish debugger-visible state coherently with minimal repeated work
  Derive human-readable statistics outside the hot per-frame path where permitted
```

This is not a request for another general event bus, job system, dynamic registration framework, or cache manager. Keep fixed bounds, explicit ownership, and very small state. The large architectural win is **not running or retaining services that have no work**.

### Boundaries with the other buckets

OS queue/identity improvements may also reduce SRC or other callers. Memory recovered from the dormant service can affect arena residency and indirectly change FTR/STG/MISC costs. These are useful cross-bucket effects, but they must be measured in the same whole-frame run rather than estimated by adding bucket medians.

The current sizing receipt records an owner SRC NO-GO decision. This OTHR investigation does not reinterpret that as permission to replace the source simulation. It offers overhead and residency candidates while leaving broader bucket campaigns under their own decisions. [S03]

## 13. Approaches rejected or deliberately not sold as new wins

| Attractive idea | Why it is not a supported new OTHR optimization |
|---|---|
| “Delete the 280K OTHR bucket” | WAIT is inside it; active OTHR must be calculated per row. |
| “FIFO stalls automatically land in OTHR” | They are charged where the blocked instructions execute. The final DMA wait/flush is explicitly timed into MISC. |
| “The bucket medians add past ALL, so the top-level lanes overlap” | Marginal quantiles are not additive. Validate the source partition per frame. |
| “Move all hot globals into DTCM” | A specific 508-byte experiment already exists, with final status `IMPLEMENTED_NOT_ACCEPTED` and alignment/sample-label caveats. |
| “Just return from the audio stub and recover 16 KiB” | A finished coroutine's allocation is not automatically freed. Avoid the allocation or destroy it safely from another live stack. |
| “Free every parked service stack” | Nested caller links and live scheduler/controller/gameplay contexts can still depend on them. Only thread 4's dormant private-queue role was established here. |
| “Avoid the full framebuffer clear in begin-frame” | Native `ndsPlatformBeginFrame()` already returns without it. |
| “Reduce the VBlank IRQ body” | Its inspected body is already just the VBlank counter increment. |
| “Only update OAM when dirty” | IFCommon and Results already have dirty/activity guards. Need actual redundant transfer evidence. |
| “Disable expensive tick-HUD drawing” | The source already supports capture without drawing, and historical diagnostic cost is not shipping gameplay cost. |
| “Remove all debug counter publication” | The harness relies on coherent cross-field observations even without HUD. Replace the mechanism without losing the witnesses. |
| “Remove graphics heap and DL resets” | The host documents corruption/leak history when those resets were absent. They are bounded-state correctness work, not disposable overhead. |
| “Poll controllers again in the seam” | The host documents a recent duplicate-publication bug that consumed input edges. Preserve the established single ownership. |
| “Use async DMA everywhere” | Coherency, external-bus contention, DMA source lifetime, and TCM accessibility constrain actual overlap. |
| “Rewrite coroutine context switching in assembly first” | The existing coroutine machinery already calls a register-swap primitive; avoid unnecessary switches before optimizing the primitive. |
| “Repackage old pose/packet experiments as new OTHR redesigns” | The recent sizing receipt lists failed or already-banked fighter experiments. They do not become new opportunities by relabeling their bucket. |

Source basis: frame host/platform [S02][S04], coroutine/OS [S08][S09], publication [S10], recent receipts [S03][S12], OAM guards [S17][S18], and hardware transfer constraints [E01][E02].

## 14. A bounded validation campaign

### Step A — obtain a clean current baseline, without changing the goalposts

Use the project's four-CPU stress target and exact configured accuracy-focused emulator. Keep interpreter/JIT/timing settings fixed from boot. Record the source revision, ROM and ELF hashes, generated configuration, roster, stage, item law, input/RNG conditions, memory counters, and exact sample identity.

The inspected runner exposes `Build`, `NoBuild`, `SetGlobals`, `Samples`, `StartFrame`, and output-path parameters. Its default measurement window is 1,972 timing samples, with source/clock coverage collected from the same match. It explicitly guards item-rate and spawn-law defaults. [S20]

Example from the repository root, using the existing configured emulator:

```powershell
New-Item -ItemType Directory -Force artifacts/performance/othr-control | Out-Null

./scripts/verify-p2-four-fighter-stress.ps1 `
  -Samples 1972 -StartFrame 2 `
  -JsonOut artifacts/performance/othr-control/run.json `
  -RowsCsv artifacts/performance/othr-control/rows.csv `
  -CoverageJsonOut artifacts/performance/othr-control/coverage.json `
  -MemoryJsonOut artifacts/performance/othr-control/memory.json
```

Confirm that the runner's configured `MelonDS` path resolves to the intended accuracy build before running it. An exit success for a particular harness phase is not by itself proof of the final 30 FPS project goal: inspect work percentiles, cadence, content, and memory evidence.

### Step B — close the OTHR and tail ledger once

Reuse existing high-level timing where available. Add only the missing coarse intervals, identically to both arms:

| Measurement | Purpose |
|---|---|
| `OTHR-WAIT` per row | Actual active residual inside the current bracket |
| Post-VBlank commit ticks | Distinguish required publication from empty-owner dispatch |
| OS pump ticks / resumes / unsatisfied predicates | Size the scheduler opportunity |
| Pacing-stat derivation ticks | Size the per-frame division opportunity |
| Finalizer + ring + debugger publication tail | Capture cost absent from current ALL |
| Consecutive equivalent-boundary timestamps | Detect timing that falls between brackets |
| Actual two-/three-/four-/five-plus-VBlank histogram | Establish output cadence |
| Thread-stack and allocator witnesses | Verify the 16 KiB service allocation actually disappeared |

Avoid building a giant instrument that perturbs the arena or dcache enough to swamp the work being measured. Sum coarse spans and test their overhead on the control. Do not put volatile timer reads inside every small queue operation in the acceptance build.

### Step C — run the smallest decisive experiments

**C1:** Verify the dormant-service allocation and baseline wake count; implement only the one-shot startup path. Prove boot/audio/lifecycle and memory recovery before touching another service.

**C2:** Compare the queue arithmetic/current-thread identity changes in the same linked layout where practical. Inspect target disassembly. Then add predicate-aware waiting, preserving slot order and testing nested resumes.

**C3:** Measure current debugger publication's unique-line/call count; compare a coherent deduplicated publisher. Account for its tail and metadata costs. Keep the old ABI for this first version.

**C4:** Evaluate per-frame FPS derivation and capture layout as separately labeled diagnostic/frame-service changes, not as automatic gameplay savings.

Do not keep rerunning dead candidates because another document lists them. Record candidate engagement, the replaced work, rejection cause, and the exact revision/configuration that established the result.

### Step D — acceptance is whole-frame and content-preserving

For a candidate to be retained, demonstrate the intended operation was eliminated or shortened; identical native content and source behavior; no native failure increase; valid sample identity; safe memory/stack/IRQ layout; and improved whole work, useful deadline margin, or explicitly measured residency without unacceptable regression.

Test scene entry, pause/unpause, KO/effects, Results, rematch, and shell return. Queue changes are global and can break menus even if a direct battle boot passes. Preserve controller edges, message order, logical tic count, and exactly-once frame/commit ownership.

Report **P50/P95 WORK-H, unadjusted WORK/ALL, actual cadence, excluded tail, and memory** together. WORK-H is useful for separating a measured HUD component; it does not authorize omitting required game-visible HUD cost from the final playable experience. Compare an appropriate lean/native production build as well as the capture build.

### Stop / continue rule

Continue OTHR work when it has a demonstrated runtime population, meaningful measured tail cost, or verified useful memory recovery. Stop speculative OTHR expansion once the measured residual is small and stable, and return the primary optimization effort to the larger actual critical paths.

This is not abandoning 30 FPS. It prevents spending another long campaign trying to recover waiting time while the important frame work remains elsewhere.

## 15. Bottom line

**The strongest newly identified OTHR redesign is to remove a service that has no runtime job, not to rewrite a renderer.** Thread 4's private-queue audio boot stub makes a specific, testable 16 KiB residency opportunity. The next best structural changes are queue-readiness dispatch, constant-time thread identity, division-free queue indexing, and coherent publication with fewer repeated cache operations.

The code and the existing per-frame analysis do **not** support promising hundreds of thousands of direct OTHR ticks or saying these candidates alone achieve 30 FPS. They do support a compact overhead-reduction and memory-recovery package, plus an important fix to how the surrounding frame tail is measured.

**Implement one-shot boot service + minimal OS fast path first; independently measure and simplify publication; preserve all native rendering and gameplay.** Bank only verified whole-run or residency results.

## Appendix A — arithmetic test, included for reproducibility

This is the host-side test used in this review. Save the three helpers from section 7 as `queue_wrap.c` (remove both `static` and `inline` when compiling the separate assembly demonstration), and this block as `test_queue_wrap.c`. It does not require project assets or a ROM.

```c
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include "queue_wrap.c"

int main(void)
{
    uint64_t checks = 0;
    for (uint32_t n = 1; n <= 256; ++n) {
        for (uint32_t first = 0; first < n; ++first) {
            assert(queue_next_slot(first,n) == (first+1u)%n);
            ++checks;
            assert(queue_prev_slot(first,n) == (first+n-1u)%n);
            ++checks;
            for (uint32_t count = 0; count < n; ++count) {
                assert(queue_send_slot(first,count,n) == (first+count)%n);
                ++checks;
            }
        }
    }
    const uint32_t big[] = {
        257, 1024, 65535, 65536, 1073741824, 2147483647
    };
    for (unsigned i=0; i < sizeof big / sizeof big[0]; ++i) {
        uint32_t n = big[i];
        const uint32_t edge[] = {0, 1, n/2, n-2, n-1};
        for (unsigned j=0; j<5; ++j) {
            uint32_t first = edge[j];
            assert(queue_next_slot(first,n) == ((uint64_t)first+1)%n);
            ++checks;
            assert(queue_prev_slot(first,n) == ((uint64_t)first+n-1)%n);
            ++checks;
            for (unsigned k=0; k<5; ++k) {
                uint32_t count = edge[k];
                assert(queue_send_slot(first,count,n) ==
                       ((uint64_t)first+count)%n);
                ++checks;
            }
        }
    }
    printf("PASS: %" PRIu64 " arithmetic comparisons\n", checks);
    return 0;
}
```

```sh
gcc -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=undefined \
    test_queue_wrap.c -o test_queue_wrap
./test_queue_wrap
```

Keep assertions enabled. The output count should be `5691218`. Whole-queue equivalence needs separate tests covering successful/failed operations and coroutine scheduling; it is not established by index arithmetic alone.

## Appendix B — source map and evidence provenance

Every repository reference below is pinned to the reviewed revision. Function names are included so a coding agent can relocate the relevant code after the repository advances. Historical receipts are evidence about their own runs, not measurements performed by this review.

**S01 — `PROJECT_GOAL.md`.** Current goal, native-only constraint, timing/cadence, fidelity and precomputation policy. [Open pinned source][S01]

**S02 — `src/port/taskman_seam_battle_host.c`.** Update/presentation host; pacing statistics; native draw boundaries; finalizer bucket accounting; markers; next-iteration timestamp. Key regions: approximately lines 200–440 and 570–1150. [Open pinned source][S02]

**S03 — `artifacts/performance/2026-09-17_p2-2p8-ftr-stg-misc-sizing/README.md`.** Recorded qualified four-fighter checkpoint, existing owner decisions, and prior candidate rejections. Quantile-overlap inference corrected in section 3. [Open pinned source][S03]

**S04 — `src/nds/nds_platform.c`.** Native begin-frame, input scan, VBlank IRQ, 128-frame tick ring, frame-complete publication, scheduled wait, flush timer, and post-VBlank commits. Relevant regions include 400–600, 1945–2130, 2660–2720, and 3320 onward. [Open pinned source][S04]

**S05 — `src/port/boot_stubs.c`.** ndsBootServiceThread and syAudioThreadMain: one-shot acknowledgement followed by a permanently empty private-queue receive loop. [Open pinned source][S05]

**S06 — `decomp/BattleShip-main/decomp/src/sys/main.c`.** Original startup: creation/start/acknowledgement of scheduler thread 3, audio thread 4, and controller thread 6. [Open pinned source][S06]

**S07 — `src/import/battleship_sys_main.c`.** Port translation unit imports the original startup source unchanged. [Open pinned source][S07]

**S08 — `src/port/libultra_os.c`.** 64-slot registry; 16 KiB service-stack request; current-thread scan; queue modulo; unconditional WAITING/RUNNABLE pump; scene-arena deregistration. [Open pinned source][S08]

**S09 — `src/port/coroutine.c`.** Coroutine context, nested caller/current pointer, resume/yield, finished state, and separate destruction. [Open pinned source][S09]

**S10 — `include/nds/nds_platform.h`.** Debugger-coherency contract; X-macro groups; per-member DC_FlushRange publication; explanation of no write allocation and debugger cache bypass. [Open pinned source][S10]

**S11 — `artifacts/performance/2026-09-14_p2-2p8-fourcpu-attribution/ring-analysis.txt`.** 1,972-row matching-frame attribution: 28,032-tick median active OTHR, negative hot-clean excursion, exact top-level identity, and flat-lane counterfactual. [Open pinned source][S11]

**S12 — `artifacts/performance/2026-09-17_p2-2p8-dtcm-hot-scalars/README.md`.** Already implemented 508-byte hot-scalar experiment; configuration-dependent measurements; sample-label and IRQ-alignment corrections; final IMPLEMENTED_NOT_ACCEPTED status. [Open pinned source][S12]

**S13 — `src/port/scheduler_backend.c`.** ndsOsPostVBlank, VI message posting, framebuffer state, and native cache/OS backends. [Open pinned source][S13]

**S14 — `decomp/BattleShip-main/decomp/src/sys/scheduler.c`.** Original sySchedulerVRetrace: source tic increment, client messages, buffer handling, and task execution. Relevant region approximately lines 1030–1070. [Open pinned source][S14]

**S15 — `decomp/sm64-nds/src/nds/main.c`.** DS port reference: direct main game-loop entry and explicit audio/IPC callbacks. Architecture reference only, not code to import wholesale. [Open pinned source][S15]

**S16 — `decomp/sm64ds-decomp/src/_ZN3IRQ13VBlankHandlerEv.cpp`.** Native DS reference: VBlank bookkeeping and explicit wakeups of specific wait sites. [Open pinned source][S16]

**S17 — `src/nds/nds_ifcommon_oam.c`.** ndsIFCommonNativeOamCommit already checks the frame-needs-commit flag. [Open pinned source][S17]

**S18 — `src/nds/nds_results_oam.c`.** ndsResultsOamCommit already checks active state and the frame-needs-commit flag. [Open pinned source][S18]

**S19 — `include/nds/nds_ui_kit.h`.** UI-kit native shadow-OAM publication interface; see platform source for its commit ordering and tenant ownership comment. [Open pinned source][S19]

**S20 — `scripts/verify-p2-four-fighter-stress.ps1`.** Existing four-CPU runner parameters, same-match timing/coverage/memory collection, and item-law preconditions. [Open pinned source][S20]

**E01 — BlocksDS, TCM and Cache.** Primary SDK documentation on ARM9 memory visibility, cache/TCM constraints, and DMA overlap. [Read documentation][E01]

**E02 — BlocksDS, DMA.** Primary SDK documentation on source visibility, TCM limitations, transfer safety, and cache-maintenance costs. [Read documentation][E02]

### Reference links

[S01]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/PROJECT_GOAL.md
[S02]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/src/port/taskman_seam_battle_host.c
[S03]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/artifacts/performance/2026-09-17_p2-2p8-ftr-stg-misc-sizing/README.md
[S04]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/src/nds/nds_platform.c
[S05]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/src/port/boot_stubs.c
[S06]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/decomp/BattleShip-main/decomp/src/sys/main.c
[S07]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/src/import/battleship_sys_main.c
[S08]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/src/port/libultra_os.c
[S09]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/src/port/coroutine.c
[S10]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/include/nds/nds_platform.h
[S11]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/artifacts/performance/2026-09-14_p2-2p8-fourcpu-attribution/ring-analysis.txt
[S12]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/artifacts/performance/2026-09-17_p2-2p8-dtcm-hot-scalars/README.md
[S13]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/src/port/scheduler_backend.c
[S14]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/decomp/BattleShip-main/decomp/src/sys/scheduler.c
[S15]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/decomp/sm64-nds/src/nds/main.c
[S16]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/decomp/sm64ds-decomp/src/_ZN3IRQ13VBlankHandlerEv.cpp
[S17]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/src/nds/nds_ifcommon_oam.c
[S18]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/src/nds/nds_results_oam.c
[S19]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/include/nds/nds_ui_kit.h
[S20]: https://github.com/rockenrooster/Smash64DS_Port/blob/35531554bc0aacc6b09cacad84be8f99b4200c68/scripts/verify-p2-four-fighter-stress.ps1
[E01]: https://blocksds.skylyrac.net/tutorial/intermediate/tcm_and_cache/
[E02]: https://blocksds.skylyrac.net/tutorial/intermediate/dma/
