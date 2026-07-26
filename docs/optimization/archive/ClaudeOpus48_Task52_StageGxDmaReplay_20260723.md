/task Task 52 — Dream Land stage: Task 36 FIFO transport census + GX DMA replay

Read first, in order: docs/optimization/TASK_STANDING_RULES.md (standing rules apply in
full — filename convention is ClaudeOpus48_), artifacts/performance/
2026-07-23_task51-stage-native.md, docs/optimization/PROFILE0_NATIVE_CAMPAIGN.md §5,
the current Task 36 replay implementation in src/nds/nds_renderer.c, the Task 49 GX
differ, and decomp/sm64-nds/src/nds/nds_renderer.c as a read-only algorithm reference.
Do not restate this task back; execute it.

Branch NDS_TASK52 from current master. Record the exact parent SHA before changing code.
Expected current parent is a4ca08e, but verify rather than assuming.

New mode selector:

NDS_TASK52_STAGE_GXDMA_MODE ?= 0

Modes:
- 0 = existing Task 36 CPU FIFO replay; canonical control.
- 1 = GX FIFO DMA replay with an immediate completion barrier.
- 2 = GX FIFO DMA replay with a deferred completion barrier at the latest proven-safe
  point, allowing genuine CPU-only overlap.

Do NOT touch or reactivate the killed Task 51 flag/table/runtime
(NDS_TASK51_STAGE_NATIVE). Leave it archived and default-off.

PRIMARY OBJECTIVE

Determine whether the profile-0 Dream Land STG cost can be reduced substantially by
replacing Task 36's per-word ARM9 GFX_FIFO replay loop with direct DMA_FIFO transport.

Do not assume DMA is a win. The central question is whether the existing elapsed cost
comes from:

A. ARM9 issue overhead:
   loads, loop control, peripheral stores, and associated software bookkeeping.

B. GX FIFO / geometry-engine backpressure:
   ARM9 stalls because GX cannot consume the words quickly enough.

DMA can remove much of A.

DMA cannot remove B. It may merely move the same wait into DMA completion or a later
GX synchronization barrier.

E0 must separate these before any DMA implementation is admitted.

GROUND TRUTH — VERIFY IN THE EXACT PARENT CODE

Task 36 should already do most of the immutable-replay work:

- NDS_TASK36_HW_COMPOSE==2 captures eligible rigid-stage GX command words into
  sNdsRendererTask36ReplayOwner.words[].
- The buffer is aligned and DC_FlushRange'd once when capture completes successfully.
- Steady-state replay occurs through ndsRendererTask36ReplayRun().
- The current per-frame transport is a CPU loop equivalent to:

    words = &owner->words[run->word_offset];

    for (i = 0; i < run->word_count; i++)
    {
        GFX_FIFO = words[i];
    }

- The eight active rigid layer-0 Dream Land bindings are expected to use this replay.
- Historical observations suggest approximately 2,996 replay words per presented frame.
- The original Task 51 failed because it targeted matrix work that was not active:
  the drawing stage stream was approximately 81% vertex-related words and only 0.4%
  matrix commands.

Verify every claim against the parent source and runtime counters. Do not proceed on
documentation alone.

HISTORICAL BASELINES — CONTEXT ONLY

Two valid captures exist from different contexts:

- Original Task 51 controlled A-side:
  STG P50 569,216 / P95 574,208.

- Newer HUD capture:
  STG P50 approximately 610,624 / P95 approximately 618,112.

Do not call either one "the baseline."

Capture a fresh mode-0 baseline from the exact parent commit, exact target, exact input,
and exact frame window. Only same-ROM-window A/B measurements count.

E0 — PROVE THE LIVE REPLAY PATH

Before adding transport instrumentation:

1. Confirm profile-0 steady-state Dream Land executes through
   ndsRendererTask36ReplayRun(), not the compose or generic fallback path.

2. Record at minimum:

   - gNdsRendererTask36ReplayState
   - capture attempt/success/failure counts
   - replay frame count
   - replay segment count
   - replay run count
   - replay word count
   - replay fallback count
   - arena reject count
   - material reject count
   - active binding mask
   - words per active run
   - total replay words per presented frame

3. Confirm all expected eight rigid layer-0 bindings replay in the canonical windows.

4. Confirm owner->words is not modified after successful capture.

5. Confirm the capture-time DC_FlushRange covers owner->word_count * sizeof(u32).

6. Confirm ordinary steady-state replay performs zero per-frame cache flushes on this
   buffer.

STOP at E0 and report if the active stage does not actually use Task 36 replay in the
canonical profile-0 path.

Do not implement DMA against an inactive or fallback-only path.

E0 — FRESH BASELINE

Capture a fresh profile-0 tick-HUD baseline before changing transport behavior.

Requirements:

- NDS_TASK52_STAGE_GXDMA_MODE=0.
- Same emulator binary and configuration for all emulator comparisons.
- Same ROM boot flow and input.
- Same presented-frame window.
- At least 128 presented-frame samples.
- Prefer canonical windows 438-445, 600-607, and 1200-1207 where practical.
- Do not compare half-second HUD snapshots as though they were presented frames.

Report for ALL, STG, FTR, SRC, HUD, AUD, MISC, and OTHR:

- min
- mean
- P50
- P95
- max

Also report:

- 2-VBlank count
- 3-VBlank count
- 4-VBlank count
- 5+-VBlank count
- maximum VBlank interval
- cadence violations

E0 — TRANSPORT COST SPLIT

Instrument Task 36 replay into these spans:

1. Stage-owner entry and camera/projection setup.
2. ndsRendererNativeStageBeginRun().
3. The actual CPU GFX_FIFO replay loop.
4. Post-loop software shadow-state bookkeeping.
5. ndsRendererHardwareEndBatch().
6. Inter-run or owner-end GX synchronization/waiting.
7. Remaining STG-owner work.

Use existing M3 phase-profiler infrastructure where practical. Keep all instrumentation
default-off. Do not use the instrumented ROM for the final A/B numbers.

Add or reuse a diagnostic no-GX control that consumes the same source words and executes
equivalent loop/read/control work without writing GFX_FIFO.

Preferred controls:

- existing CPU_PREP_NO_GX benchmark sink, if it reaches this exact replay path;
- an aligned volatile memory sink;
- a bounded rolling checksum plus volatile sink that prevents optimization.

The no-GX control estimates software iteration/read cost only. It does not reproduce
peripheral-store timing or GX backpressure. State this limitation explicitly.

Where safely observable, record GX FIFO status around the replay loop:

- FIFO less-than-half / half / full conditions;
- geometry-engine busy state;
- time spent before the first write;
- time from first write to final write;
- time from final write to the next proven-safe GX barrier.

Do not poll GX status per word in the performance build. Any detailed polling belongs
only in a diagnostic ROM because the observation itself can perturb timing.

E0 must estimate:

- raw software loop/read cost;
- actual CPU GFX_FIFO-loop elapsed cost;
- probable FIFO/backpressure share;
- BeginRun and EndBatch cost;
- non-transport STG floor;
- maximum plausible mode-1 saving;
- maximum plausible mode-2 saving.

E0 — WRITTEN PROCEED/STOP VERDICT

Before implementing DMA, write a short E0 result containing:

- fresh STG P50/P95;
- FIFO-loop P50/P95;
- no-GX software-loop estimate;
- estimated CPU issue cost;
- estimated FIFO/backpressure cost;
- BeginRun/EndBatch cost;
- remaining stage cost outside transport;
- mode-1 theoretical floor;
- mode-2 theoretical floor;
- expected ALL P95 ceiling.

Proceed to E1 only if at least one condition is met:

1. Removable CPU transport cost is at least 150,000 ticks at both P50 and P95.

2. A proven asynchronous overlap interval could plausibly hide at least 200,000 ticks
   of transfer/backpressure time.

3. FIFO transport plus immediately associated per-run overhead owns at least 40% of
   STG, and the estimated final result would be a material profile-0 gain.

STOP at E0 if:

- the CPU FIFO loop is a minor owner;
- most elapsed time is unavoidable GX backpressure;
- no useful safe overlap interval exists;
- estimated ALL P95 improvement is below 100,000 ticks;
- the estimated result cannot materially advance the 1.12M ALL-P95 goal.

Do not build DMA merely because it is architecturally cleaner.

HISTORICAL PACKET/DMA WARNING

Previous whole-owner packet/copy/patch/DMA experiments regressed.

This task differs only because Task 36 already provides:

- final GX command words;
- stable run boundaries;
- immutable storage;
- one-time cache flushing;
- owner-level admission and fallback.

The Task 52 path must add none of the previous frame-time costs:

- no list construction;
- no packet copy;
- no runtime patch;
- no texture-address patch;
- no command repack;
- no per-frame relocation;
- no per-frame cache flush;
- no duplicate semantic validation;
- no ownership rediscovery.

If any of those become necessary, STOP and report that the Task 36 replay buffer is not
suitable for direct DMA transport.

E1 — DMA OWNERSHIP CENSUS

Before selecting a channel, census every ARM9 DMA use reachable during:

- battle initialization;
- ordinary profile-0 updates;
- texture staging;
- stage rendering;
- fighter rendering;
- effects;
- HUD;
- VBlank;
- interrupt handlers;
- audio-related ARM9 work;
- scene transitions.

For every use, record:

- DMA channel;
- synchronous or asynchronous;
- source and destination;
- timing mode;
- interrupt context or main-thread context;
- whether it can overlap stage presentation;
- whether it can execute after Task 36 admission.

The renderer currently uses DMA channel 0 for some texture staging, but "not channel 0"
is not a sufficient ownership policy.

Choose and document one channel that is provably available for stage FIFO transport.

Requirements:

- explicit centralized ownership;
- no overwriting another subsystem's DMA registers;
- no collision with interrupt-driven DMA;
- no waiting on unrelated DMA channels;
- no assumption that a channel is free merely because current captures did not observe
  it;
- diagnostic counter/assertion plus safe mode-0 fallback if the chosen channel is
  unexpectedly busy.

E1 — MODE 1: DMA WITH IMMEDIATE COMPLETION BARRIER

Implement the smallest exact transport replacement first.

For each existing Task 36 replay run:

- reuse owner->words directly;
- reuse run->word_offset and run->word_count;
- submit those exact words to GFX_FIFO with DMA_FIFO timing;
- do not copy, repack, patch, rebuild, or flush the source;
- explicitly wait for completion before any subsequent GX writer or software action
  whose correctness depends on the completed run.

Preserve:

- ndsRendererNativeStageBeginRun();
- existing material/prepared state;
- run ordering;
- matrix ordering;
- renderer shadow state;
- ndsRendererHardwareEndBatch();
- Task 36 admission;
- Task 36 fallback.

Implement a small dedicated helper, for example:

ndsRendererTask52SubmitGXRunDMA(
    const u32 *words,
    u32 word_count);

Do not use stock glCallList() in the measured path. It performs its own cache flush and
conservative waits, which would contaminate the experiment.

MODE 1 PURPOSE

Mode 1 answers:

Does DMA setup plus an immediate wait cost materially less than the current ARM9 FIFO
store loop?

Mode 1 does not claim or attempt asynchronous overlap.

Measure mode 0 vs mode 1 before writing mode 2.

If mode 1 shows that elapsed time is almost entirely retained at the DMA barrier, record
that GX backpressure—not CPU issue overhead—is the dominant owner.

E1 — MODE 2: DEFERRED BARRIER / REAL OVERLAP

Implement mode 2 only if E0 or mode 1 proves that useful overlap could provide a large
gain.

Mode 2 must:

1. Start GX FIFO DMA.
2. Return without immediately waiting.
3. Execute only work that:
   - performs no GX register writes;
   - does not modify owner->words;
   - does not reuse the selected DMA channel;
   - does not depend on the run having completed;
   - does not mutate renderer state required by the in-flight command stream.
4. Wait at the latest proven-safe barrier before:
   - the next GX register writer;
   - the next stage run needing ordered state;
   - fighter GX submission;
   - effect GX submission;
   - glFlush;
   - frame completion;
   - reuse of the DMA channel.

Candidate overlap work must be identified from the actual call graph. Do not invent
overlap by moving state bookkeeping whose correctness depends on completed GX commands.

Document:

- exact DMA launch location;
- exact deferred-wait location;
- exact work performed between them;
- why every operation in the overlap region is safe;
- overlap ticks available at P50/P95;
- residual wait ticks at the final barrier.

If the existing per-run BeginRun/EndBatch structure leaves no useful overlap interval,
report that result. Do not broadly rewrite owner ordering inside this task.

OPTIONAL WHOLE-OWNER COALESCING

Do not automatically concatenate all stage runs.

Investigate whole-owner or multi-run coalescing only if:

- mode 1 proves per-run DMA setup/barriers dominate the residual;
- all required GX state is already represented in exact command order;
- no frame-time copy, patch, repack, or flush is introduced;
- Task 49 can observe and compare the resulting full owner stream;
- owner-level fallback remains all-or-nothing.

Any coalescing experiment must use a separate mode/flag and separate A/B result.

Do not let optional coalescing expand the initial DMA transport task into another stage
renderer rewrite.

E2 — CORRECTNESS

Task 49 word equality is necessary but not sufficient.

Required:

- Task 49 STAGE-owner Tier 1 differences exactly 0.
- Task 49 Tier 2 maximum screen-space deviation exactly 0.0 pixels.
- Task 9 gameplay state hash EXACT.
- Zero FIFO lockups.
- Zero GX faults.
- Zero DMA ownership collisions.
- Zero partial-run submissions.
- Zero missing stage geometry.
- Zero material, texture, palette, or depth-order corruption.
- No pause-camera swimming.
- No normal-camera or wide-zoom regression.
- No stage/fighter ordering regression.
- No death, rebirth, shield, hit-flash, or effect ordering regression.
- Default-off published ROM byte-identical to the exact parent build.

Important: the Task 49 differ can remain green even if identical words are submitted with
unsafe timing. Therefore it does not prove FIFO ordering or DMA correctness.

Capture synchronized mode-0/mode-1/mode-2 screenshots into artifacts/visibility/ for
human owner approval across:

- ordinary match camera;
- maximum expected zoom;
- pause-camera orbit;
- shield activity;
- damage and hit effects;
- death and rebirth;
- projectile-heavy activity.

Run an extended soak test to expose intermittent DMA/FIFO races.

E2 — EMULATOR AND RETAIL HARDWARE

melonDS is useful for functional checks and preliminary timing, but it is not the final
authority for DMA-to-GX-FIFO timing, FIFO backpressure, or hardware arbitration.

For every mode that passes emulator correctness:

- produce a clearly named retail A/B ROM pair;
- place it in builds/device-queue/;
- record ROM SHA-256 values;
- record exact flags;
- include a simple real-hardware measurement procedure;
- capture retail results before declaring KEEP.

A melonDS-only performance win is provisional.

Retail DS measurements decide the final transport verdict.

E2 — FINAL PERFORMANCE A/B

Compare:

A:
NDS_TASK52_STAGE_GXDMA_MODE=0

B:
NDS_TASK52_STAGE_GXDMA_MODE=1

C, only when justified:
NDS_TASK52_STAGE_GXDMA_MODE=2

Requirements:

- same parent;
- same target;
- same emulator/hardware configuration;
- same boot flow;
- same input;
- same presented-frame windows;
- at least 128 samples;
- no final measurement instrumentation enabled;
- prove the requested mode actually compiled and executed before trusting numbers.

Report ALL, STG, FTR, SRC, HUD, AUD, MISC, and OTHR:

- min
- mean
- P50
- P95
- max
- VBlank distribution

Also report:

- replay words/frame;
- replay runs/frame;
- DMA launches/frame;
- DMA words/frame;
- per-frame cache flush count for owner->words — must be 0;
- per-frame CPU FIFO writes — mode 0 approximately current count, DMA modes approximately 0;
- DMA setup ticks;
- immediate/deferred wait ticks;
- measured overlap ticks;
- residual STG outside DMA transport.

PERFORMANCE VERDICT

Derive the expected result from E0. Do not promise a result below E0's measured floor.

FULL WIN:

- STG P50 <=200,000 and P95 <=220,000; or
- STG P50/P95 improve at least 45%;
- ALL P95 improves at least 200,000;
- no material regression in another owner;
- retail DS confirms the gain.

USEFUL PARTIAL WIN:

- retail-confirmed ALL P95 improvement >=100,000;
- STG improvement matches the E0-predicted removable transport share;
- residual STG is proven to be GX transfer/backpressure or non-transport work rather
  than retained CPU FIFO emission;
- no owner regression offsets the gain.

KILL:

- E0 predicts ALL P95 improvement below 100,000;
- mode 1 merely relocates nearly all FIFO-loop time into DMA wait;
- mode 2 has no safe useful overlap;
- retail hardware shows no repeatable improvement;
- STG P50 remains above 350,000 after a correct implementation and ALL P95 improves
  less than 100,000;
- implementation requires per-frame copy, patch, repack, or cache flush;
- DMA ownership cannot be made safe.

Before KILL, report the final split:

- camera/projection;
- BeginRun;
- DMA setup;
- DMA active transfer;
- DMA wait;
- useful overlap;
- EndBatch;
- residual stage CPU;
- estimated GX FIFO/backpressure.

Do not respond to failure by returning to matrix optimization.

TRAPS

- OVERRIDE TRAP: the tickhud and published targets override flags. Thread
  NDS_TASK52_STAGE_GXDMA_MODE through their actual target blocks. Prove the B/C ROM took
  the requested mode through build config, objdump, boot marker, or mode counters.
- CACHE TRAP: owner->words was flushed once at capture. Do not flush it every frame.
- GL_CALL_LIST TRAP: do not use stock glCallList() for the measured path.
- DMA CHANNEL TRAP: avoiding channel 0 alone does not prove ownership.
- DIFFER TRAP: identical captured words do not prove safe timing.
- PROFILER TRAP: do not use the instrumented cost-split ROM for final A/B.
- FIFO TRAP: DMA completion can still represent GX backpressure.
- OVERLAP TRAP: deferred wait is useful only when real safe work runs in between.
- PACKET TRAP: no frame-time packet construction/copy/patch/repack.
- BASELINE TRAP: do not mix the historical 569K and 610K windows.
- ONE WRITER: keep one writer on nds_renderer.c; isolate the DMA helper where practical.
- BUILD: use the devkitPro msys2 bash. Git Bash direct make hits the documented
  /opt/devkitpro recursive sub-make problem.
- TIME BOX: approximately 10 experimental runs or one hour of open-ended debugging per
  unresolved failure, then checkpoint and report.
- Cite file:line for every behavioral claim.
- NEVER push.

SEPARATE COMMITS

1. E0 live-path proof, diagnostic instrumentation, cost split, and written proceed/stop
   verdict.
2. Mode selector, DMA ownership policy, and mode-1 submitter.
3. Mode-1 correctness and performance evidence.
4. Mode-2 implementation and evidence, only if justified.
5. Final docs, retail device queue, PERF_LEDGER, HANDOFF, and PORTING updates.

DELIVERABLES

- E0 proof that Task 36 replay is active.
- Fresh mode-0 baseline.
- CPU issue vs GX backpressure cost split.
- Written E0 proceed/stop verdict.
- DMA-channel ownership census.
- NDS_TASK52_STAGE_GXDMA_MODE implementation.
- Mode-1 immediate-barrier result.
- Mode-2 deferred-barrier result only if justified.
- Task 49 differ certificate.
- Task 9 state-hash result.
- Visual A/B screenshots.
- Extended FIFO/DMA soak result.
- Retail A/B ROM pair and hashes in builds/device-queue/.
- Emulator and retail STG/ALL P50/P95 plus VBlank distributions.
- Words, launches, cache flushes, CPU FIFO writes, waits, and overlap counts.
- Final KEEP / USEFUL PARTIAL WIN / KILL verdict.
- PERF_LEDGER, HANDOFF, and PORTING updates.
- New-Smash64DSSnapshot.ps1 -Mode Lean.

If a task spec file is created, name it:

ClaudeOpus48_Task52_StageGxDmaReplay_20260723.md

---

## Result (2026-07-23): E0 STOP — Task 36 replay is structurally disabled in the shipping ROM

**Verdict: STOP at E0. No DMA admitted. Nothing merges.**

The spec's first proof is "confirm profile-0 steady-state Dream Land executes
through `ndsRendererTask36ReplayRun()`, not the compose or generic fallback
path." It does not. The FIFO-replay loop this task was chartered to DMA-replace
**does not run** in the shipping program.

### The measurement

The `gNdsRendererTask36Replay*` counter globals are compile-gated to
`NDS_RENDERER_PROFILE_LEVEL==1` (src/nds/nds_renderer.c:2076, inside the
profile-1 block at :2041), so the profile-0 tick-HUD ROM exports no counter
globals. But the internal owner struct `sNdsRendererTask36ReplayOwner` (gated
only on `NDS_TASK36_HW_COMPOSE==2`, which both shipping targets set) is always
compiled in, and GDB resolves its fields by name from the ELF debug info. That
struct is the ground truth for "is replay active."

Probed at `ndsBattlePlayableFrameCompleteMarker`, frames 438–445, on the
profile-0 tick-HUD ROM and the published `1818AA77…` ROM:

- tick-HUD ROM: `state=DISABLED`, `word_count=0`, `frame_replay=0`,
  `arena_chosen_size=0x14C000` (short of `0x150000` by 16 KiB), 4 alloc fails.
- published ROM `1818AA77`: `state=DISABLED`, `arena_chosen_size=0x14E000`
  (short by 8 KiB), 2 alloc fails.
- `rigid_binding_mask=0x00000381c00fffff == NDS_RENDERER_TASK36_RIGID_BINDING_MASK`
  (include/nds/nds_renderer.h:100) — that gate passes.

### Root cause

`ndsRendererTask36ReplayBeginFrame` (src/nds/nds_renderer.c:4195) admits replay
only when `gNdsTaskmanArenaChosenSize == 0x150000u`. But
`gNdsTaskmanArenaChosenSize` is set by a **downward-stepping allocator**
(src/port/diagnostics.c:7368-7381) that starts at `NDS_TASKMAN_ARENA_SIZE` and
decreases by 4 KiB per failed `calloc` until one succeeds — deliberately, to
preserve the verified 128 KiB post-BGM reserve (diagnostics.c:7364-7367). On
the DS heap the full `0x150000` allocation does not fit, so the allocator steps
down (to `0x14C000`/`0x14E000`) and the exact-`0x150000` admission guard
disables replay.

With `state==DISABLED`, `frame_replay` is never set, so at the segment call
site (nds_renderer.c:21220-21222) `task36_replay_segment` is FALSE and
`ndsRendererTask36ReplayRun` is **never called**. The 8 rigid layer0 bindings
draw through the **generic per-word emit loop** (nds_renderer.c:21241-21375),
not any replay FIFO. There is no replay loop to DMA-replace.

Task 36 replay has been dead code in the shipping profile-0 ROM since the arena
allocator was made robust. The STG P50 569,280 is owned by the generic emit
path, not Task 36 replay.

### E0 fresh mode-0 baseline (captured before the path proof, for the record)

128 samples, frame 438, profile-0 tick-HUD target, ROM `9B0A295D…`, melonDS
`DE80E46B…`, git `a4ca08e`. STG P50 569,280 / P95 575,744 (reconciles with the
original Task 51 controlled A-side, not the 610K window). ALL P50 1,680,256 /
P95 2,241,024. VBlank histogram 2:0 3:426 4:122 5+:17, max 18, slips 0.

### E0 cost split — NOT PERFORMED (path inactive)

The cost split instruments the FIFO-replay loop. That loop does not execute.
Removable CPU transport cost: **0 ticks.** Proceed condition 1 (≥150,000 ticks
removable at P50/P95) is not remotely met.

### DMA ownership census (completed, retained as recoverable evidence)

Channels 1 and 2 are unused throughout the entire tree (no src/, ISR, ELF
symbol, or reservation). Channel 0 is live during stage draw (texture staging +
wallpaper overlay); channel 3 has mid-frame fills. Channel 1 is the provably-free
choice — retained here should a future task revive the replay path.

### Decisive open question (for the owner)

This STOP reframes the campaign's STG premise. Task 36 replay is disabled by
the arena guard; either (1) the guard is a latent bug and replay was meant to
ship (fix: admit replay when the buffer fits the *actual* chosen arena, not
only at exactly `0x150000` — a Task-36-correctness fix, not a DMA task), or
(2) the replay path is intentionally retired and the generic emit is the stage
draw (DMA-replay cannot help). This also corrects Task 51's attribution of STG
cost to "Task 36 replay" — Task 36 replay was disabled, so those bindings drew
via the generic path.

Full certificate: `artifacts/performance/2026-07-23_task52-stage-gxdma-e0.md`.
Branch `codex/task52-stage-gxdma-replay` holds E0 evidence + the probe script.
Published ROM stays `1818AA77…`.