# Smash64DS_Port — Stable-30 Exact Performance Queue

**Reconciled:** 2026-07-18<br>
**Target:** stable **30 unique presentations/s** with exactly **two unchanged source updates per presentation** (approximately 60 source updates/s) on stock retail Nintendo DS.<br>
**Exactness:** identical gameplay/update state and lifecycle, identical geometry/material/texture/depth/order semantics, and **0/49,152 changed synchronized native top-screen pixels**.<br>
**Authority:** this file supersedes only the **post-demo Tasks 20–25 queue** in `docs/optimization/ClaudeFable5_JumpABC_Tasks_20260715_2326.md`. Completed Tasks 1–19 and their ledger history remain authoritative.

Source basis:

- `ClaudeFable5_JumpABC_Tasks_20260715_2326.md`
- `Smash64DS_Port_Exact_Performance_Investigation_20260718(1).md`
- `Smash64DS_Port_Exhaustive_Performance_Investigation_20260718(1).md`
- `Smash64DS_Port_Performance_Investigation_20260718(1).md`
- `Smash64DS_Port_Performance_Investigation_2026-07-18(1).md`

---

## Immediate steer for the active agent

```text
/task Smash64DS_Port — Reconcile the in-flight Fable Tasks 20–25 queue with tasks.md

GOAL
Preserve every valid in-flight change while switching the post-demo performance queue to the
stable-30 critical path in tasks.md. Do not reset, clean, overwrite, or abandon existing work.

FIRST — IDENTIFY THE LIVE ATOMIC UNIT
1. Read AGENTS.md, the current handoff/board/ledger, this tasks.md, and the focused diff.
2. Record current HEAD, status, worktree, task number, sub-cut, and files owned.
3. Classify the live work:
   A. Task 20–23 optimization candidate already has implementation edits.
   B. Task 20–23 has measurement/census edits only.
   C. Task 24 has an evidence-cleared deletion batch in progress.
   D. Task 25 measurement work is in progress.
   E. No post-demo work is active.

PRESERVE THE CURRENT UNIT
- A: finish only that atomic candidate through exact A/B and independent KEEP or REVERT;
  document and commit it. Do not begin another old-queue sub-cut.
- B: preserve the instrumentation and fold it into TASK 25R or TASK 23R Phase 0 as applicable;
  do not add the old cache/optimization before the new baseline exists.
- C: finish only the already evidence-cleared deletion batch, run its safety gates, document it,
  then stop cleanup work until a quiet slot.
- D: amend the work in place to TASK 25R before collecting more measurements. Do not publish
  the smaller old matrix.
- E: start TASK 25R.

QUEUE AMENDMENTS
- Old TASK 20 becomes TASK 20R. Its staged scratch/stack mechanism remains valid; retail DS is
  the performance referee.
- Old TASK 21 becomes TASK 21R. Cuts 21A/B/C remain valid, but 21C is the compact-table
  foundation for TASK 27; it is not the completed generated fighter architecture.
- Old TASK 22 becomes TASK 22R and remains valid. Emulator proves pixels; retail decides VRAM,
  cache-maintenance, and DMA performance.
- Old TASK 23 becomes TASK 23R. Phase 0 is a consumed-field/invalidation manifest feeding
  TASK 26. Any residual cache is attempted only after TASK 26 and only for work TASK 26 leaves.
- Old TASK 24 is deferred unless disk pressure blocks implementation or verification.
- Old TASK 25 is replaced by TASK 25R.
- Add TASK 26 (generated M3 stage program), TASK 27 (generated M2 fighter program), TASK 28
  (exact ARMv5TE leaves), TASK 29 (exact GX state/templates), and TASK 30 (final stable-30 gate).

MEASUREMENT POLICY
- Never use one universal melonDS-to-device multiplier.
- Emulator is authoritative for deterministic state, semantic/GX traces, arithmetic, and pixels.
- Retail DS is authoritative for DTCM, ARM/Thumb, code/data/cache layout, generated-program
  footprint, direct VRAM stores, DMA, GX FIFO behavior, and final pacing.
- A pure computation deletion with unchanged working-set shape may be provisionally retained
  from a clear emulator A/B, but final hardware-speed claims and any cache-sensitive promotion
  require device falsification.

KEEP POLICY
Keep every independently repeatable correctness-preserving gain with:
- exact state, audio, lifecycle, geometry, material, texture, depth, and ownership behavior;
- 0/49,152 changed synchronized native pixels;
- no relevant phase or P95 regression;
- no reserve, stack, IRQ, DMA, GX, fallback, fence, or conservation failure.
An architecture-expansion gate may stop widening a mechanism; it does not force reversion of a
smaller independent exact win.

NEXT AFTER THE ATOMIC UNIT
Run TASK 25R. Then follow the priority it reports:
- default M3 lane: TASK 23R Phase 0 -> TASK 26 -> TASK 23R residual Phase 1;
- M2 lane: TASK 21R -> TASK 27;
- disjoint lane: TASK 20R -> TASK 22R;
- after representations stabilize: TASK 28 -> TASK 29;
- TASK 24 only in a quiet slot;
- TASK 30 is the final acceptance gate.
One-writer ownership of src/nds/nds_renderer.c is mandatory.

FINAL COMMAND
For every completed implementation task, after verification, docs, status, and commit:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be the final project command, with nothing run afterward.
```

---

## Governing performance contract

The scheduler remains fixed-two and source-faithful:

- Exactly two unchanged source updates for every unique presentation.
- No update debt, catch-up, skipped logic, delta scaling, interpolation, or speculative visual state.
- A presentation holds 30 FPS only when the complete production loop fits the two-VBlank slot.
- The final goal is not a median-only result. P95, maximum, interval histogram, and slip count matter.

Stable 30 is accepted only when the current profile-0 ROM passes all of the following on retail DS:

1. Three cold-boot canonical full-lifecycle runs through Countdown, combat, natural KO/rebirth, Time Up, Results, and exactly one teardown.
2. Exactly two source updates per presentation throughout.
3. **Zero presentation intervals of three or more VBlanks.**
4. **Zero pacing-slip events.**
5. 0/49,152 changed synchronized native top-screen pixels and all semantic/state/audio/geometry/material/texture/depth gates green.
6. At least 128 KiB reserve throughout the lifecycle.
7. Published P50, P95, maximum, interval histogram, presentations/s, and updates/s for every canonical phase.

---

## Reconciled queue map

| Old Fable item | Reconciled decision |
|---|---|
| Task 20 — DTCM stack | **Keep as Task 20R.** Scratch first, full stack only after complete capacity/escape proof; retail performance verdict. |
| Task 21 — M2 structural pass | **Keep as Task 21R.** Valid foundation. Its compact records/tables feed Task 27 and do not replace it. |
| Task 22 — wallpaper writer | **Keep as Task 22R.** Exact map retention and dirty spans; retail decides DMA/store crossover. |
| Task 23 — M3 live stamps | **Split/reposition as Task 23R.** Phase 0 feeds Task 26; residual cache only after Task 26. |
| Task 24 — repository diet | **Defer.** Run only when disk pressure blocks work or in a quiet integration slot. |
| Task 25 — phase matrix | **Replace with Task 25R.** Add artifact identity, profile-0 sibling, P50/P95/max, interval histogram, nested-timer rules, and stable-30 deficit. |
| Missing | **Task 26:** generated M3 stage live-value program. |
| Missing | **Task 27:** generated M2 fighter root/epoch program. |
| Later | **Task 28:** one-at-a-time exact ARMv5TE arithmetic leaves. |
| Later | **Task 29:** exact redundant GX state and one immutable-run stream. |
| Closure | **Task 30:** full retail stable-30 qualification. |

### Execution order

```text
current atomic sub-cut, if any
-> TASK 25R same-artifact truth
-> renderer priority selected by 25R:
     M3 path: TASK 23R Phase 0 -> TASK 26 -> TASK 23R residual Phase 1
     M2 path: TASK 21R -> TASK 27
-> disjoint lane: TASK 20R -> TASK 22R
-> after generated representations stabilize: TASK 28 -> TASK 29
-> TASK 24 only when lane-free or disk-blocked
-> TASK 30 final qualification
```

Default to M3 first when Task 25R confirms stage/Whispy remains the decisive owner. Reverse only when the same-artifact matrix clearly makes M2 the larger actionable blocker.

### Planning ranges — not additive ceilings

| Task/family | Credible first retained range | Broad successful envelope |
|---|---:|---:|
| Task 20R DTCM | 0–30K scratch; 10–50K stack | ~50–60K combined, hardware-dependent |
| Task 21R M2 foundation | 15–60K | ~70K before Task 27, overlap-sensitive |
| Task 22R wallpaper | 15–100K | ~100–125K including a positive DMA crossover |
| Task 23R residual cache | 0–35K | Must not count work removed by Task 26 |
| Task 26 generated M3 | 40–110K | ~160–180K after controlled expansion |
| Task 27 generated M2 | 35–95K | ~130–150K after controlled expansion |
| Task 28 exact arithmetic | 3–30K per useful leaf | ~40K per family |
| Task 29 GX state/templates | 5–30K state; 20–70K template envelope | Overlaps Tasks 26/27; do not sum ceilings |

The ordinary planning value for the whole queue remains about 245K ticks. Stable all-phase 30 requires the generated M3/M2 work to expand successfully toward the structural range; small microcuts alone cannot close Countdown and Whispy.

---

## TASK 25R — authoritative same-artifact phase matrix

```text
/task Smash64DS_Port — TASK 25R: authoritative same-artifact phase matrix and stable-30 deficit

ROLE
Own artifact identity, bounded instrumentation, benchmark capture, parsing, phase attribution,
and the stable-30 checker. Make no renderer, wallpaper, update, arithmetic, scheduler, memory-
placement, asset, or gameplay optimization in this task.

LIVE BASELINE
Reconcile the current tree rather than trusting task names. The intended baseline contains:
- ARM renderer restored by Task 19;
- Task 14 generation-gated dense first-visit cache;
- Task 16 exact compare/i2f/add-sub leaves with stock fmul;
- update-only hot-text placement;
- fixed-two locked-30 scheduler.
Record every difference explicitly.

ARTIFACT PAIR
Build from one exact HEAD and one exact toolchain/configuration source state:
A. One profile-1 ROM used for every detailed phase row.
B. One profile-0 sibling used for production pacing/FPS and code-size comparison.
The profile-1 ROM is the only source of detailed owner rows. Never mix rows from the sibling,
a historical ROM, or an oracle selector. Profile-1 instrumentation must not select a different
production executor.

IDENTITY PACKET
Record and archive:
- Git HEAD, status, focused diff, submodules if any;
- all configuration macros and exact build commands;
- GCC/binutils/devkitARM/Calico/libnds identities available from the live build;
- ROM, ELF, map, relevant object, and generated-table SHA-256 hashes;
- arm-none-eabi-nm -S --size-sort;
- arm-none-eabi-readelf -W -S -s -r -A;
- arm-none-eabi-objdump -d -S -r -w;
- linker map/load-range report;
- ARM/Thumb states, ITCM/DTCM/main-RAM sizes, .text.hot order, veneers/stubs, largest hot stack
  frames, spills, and current reserve.

PHASES
Capture all phases from the same profile-1 ROM:
1. Countdown / GO: completed frames 438–445
2. Early combat: 600–607
3. Mid-combat / Whispy: 1398–1405
4. Late combat: 3300–3307
5. Natural KO: event-owned eight-frame window corresponding to 708–715
6. Rebirth: event-owned eight-frame window corresponding to 730–737
7. Time Up through Results and exactly one teardown

Also record the matching profile-0 sibling's presentations/s, source updates/s, presentation-
interval histogram, and slip count for the natural lifecycle.

REQUIRED ROWS
For each phase publish P50, P95, maximum, and sample count for:
- input sampling;
- source update 1;
- source update 2;
- aggregate source-update owner;
- audio/update shell;
- total fixed-two update batch;
- frame begin;
- wallpaper;
- stage;
- Mario;
- Fox;
- foreground/effects;
- draw residual;
- draw;
- GX/flush;
- post-VBlank work;
- runnable thread work;
- present active;
- VBlank wait;
- loop residual;
- whole loop wall;
- presentations/s;
- source updates/s;
- presentation interval counts for 2, 3, 4, and 5+ VBlanks;
- pacing/slip counters.

NO-FALSE-ACCOUNTING RULES
1. Mark every timer as sibling, child, or nested.
2. Never sum nested M3/M2 diagnostic spans into an owner total.
3. Never combine a profile-1 owner row with a profile-0 timing row.
4. Never combine rows from different ROMs.
5. Production wallpaper is the current incremental selector. Full-raster/oracle modes are
   exactness controls only and may never be performance baselines.
6. Input/audio/residual remain visible; never present an owner-only sum as whole loop.
7. Report maximum and interval histogram, not only P50/P95.

EXACTNESS GATES
- exactly two source updates per presentation;
- no debt/catch-up accounting;
- exact full-lifecycle update/presentation counts and exactly one teardown;
- exact 121 runs / 828 triangles and 202/320/306 ownership;
- exact current M3 and M2 topology/state counters;
- zero fallback, fence, conservation, unexpected conversion/upload, or residency faults;
- exact state, audio, geometry, material, texture, depth, and ordering rows;
- 0/49,152 changed synchronized native top-screen pixels;
- reserve >=128 KiB.

OUTPUTS
1. Machine-readable same-artifact phase matrix.
2. Human-readable phase table replacing the mixed-ROM board table.
3. P50/P95/max deficit from the two-VBlank deadline for every phase.
4. Per-phase ranked owner list.
5. Presentation-interval/slip histogram and stable-30 readiness summary.
6. M3-first or M2-first recommendation based only on this artifact.
7. Updated non-overlapping bounds for Tasks 20R–23R, 26, and 27.
8. Reusable checker modes:
   - report-only for the current baseline;
   - -RequireStable30 hard-fails on any 3+ VBlank interval or slip.

KEEP / REVERT
Keep evidence, parsers, and bounded profile-only instrumentation only. Profile-0 production
behavior must remain unchanged. Revert instrumentation that leaks into profile 0 or changes
owner selection, code placement, data layout, or timing behavior.

DOCUMENTATION
Update PERF_LEDGER.md and P1_EXECUTION_BOARD.md. Archive the old mixed-ROM table as historical
rather than silently deleting it.

FINAL COMMAND
After checks, documentation, status, and commit:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be last.
```

---

## TASK 20R — DTCM scratch first, bounded coroutine stack second

```text
/task Smash64DS_Port — TASK 20R: retail DTCM scratch-first and bounded coroutine-stack locality

ROLE
Own one data-locality mechanism at a time: first a small measured hot scratch block, then the
canonical gameplay coroutine stack only when complete safety and capacity proofs succeed. Do
not change algorithms, arithmetic, renderer representation, code placement, scheduler, or
source gameplay behavior.

LIVE BASELINE
Preserve the current Task-19 ARM renderer, Task-16 exact arithmetic, and update-hot placement.
Emulator timing is not a keep/revert vote for this task; retail DS is the performance referee.

SOURCE-BACKED HYPOTHESIS
The canonical gameplay coroutine uses a 16 KiB main-RAM stack with approximately 8.1 KiB
observed high-water, while renderer/update spill traffic is measurable. DTCM may reduce repeated
main-RAM accesses, but nominal 16 KiB capacity is shared with application data and user/IRQ/SVC
stacks and cannot be assumed available.

PRE-CODE BOUNDS
- First 1–2 KiB scratch placement: 0–30K whole-loop, with zero credible.
- Full bounded stack/overlay: 10–50K working range.
Never apply a universal main-RAM-to-DTCM multiplier.

PHASE A — COMPLETE MEMORY AUDIT
1. Record exact current DTCM owners, addresses, alignment, application data, user/IRQ/SVC stack
   boundaries, BIOS/Calico reservations, and free ranges from the current ELF/map.
2. Measure high-water with red zones through Countdown, heavy combat, Whispy, KO, rebirth,
   Time Up, Results, teardown, repeated execution, and a full one-minute match.
3. Audit every address escape from the coroutine stack and candidate scratch object: globals,
   callbacks, stored pointers, DMA, GX source pointers, IPC, ARM7/audio, interrupts, and async use.
4. Prove no DMA source/destination is moved into inaccessible TCM.
5. Add linker assertions and runtime canaries before moving anything.

PHASE B — ONE SMALL SCRATCH BLOCK
1. Select one bounded 1–2 KiB object with high accesses/frame from Task 25R evidence.
2. Move only that object into an explicitly named and aligned DTCM section.
3. Preserve all values, aliases, lifetimes, and iteration order.
4. Run exactness first, then retail A/B across early, Whispy, and KO.
5. KEEP or REVERT this object independently before considering the stack.

PHASE C — BOUNDED COROUTINE STACK
Proceed only when full-lifecycle high-water plus guards fits DTCM without colliding with any
application data or user/IRQ/SVC stack.
1. Reserve an explicitly sized DTCM stack/overlay with linker assertions.
2. Keep red zones and high-water reporting in diagnostic builds.
3. Fail safely before gameplay to the current main-RAM allocation if capacity/setup validation
   fails.
4. Do not shrink the stack solely from one observed route.
5. Prove interrupt, audio, lifecycle, pointer-escape, and DMA safety.
6. Measure stack placement separately from the retained scratch placement.

EXACTNESS GATES
- full state rows and exactly two updates per presentation;
- exact audio events, lifecycle, teardown, and scheduler counters;
- exact renderer geometry/material/texture/depth/pixel output;
- no guard touch, overflow, overlap, stale pointer, or changed pointer-visible state;
- no DMA/IPC/ARM7 access to inaccessible DTCM;
- reserve >=128 KiB and explicit DTCM free-margin report.

MEASUREMENT
- Emulator: state/pixel/canary falsification only.
- Retail: identical control/candidate, cold boots, same phases.
- Source-update or selected-owner P50/P95/max is primary; whole loop and interval/slips are
  mandatory context.
- A2 only for close, noisy, or phase-inconsistent results.

KEEP / REVERT
- Bank any repeatable exact retail gain with no phase/P95 loss.
- Revert the selected object or stack independently when hardware is flat/regressive.
- Immediate revert on guard/IRQ/audio/lifecycle/DMA failure or insufficient DTCM margin.
- Never generalize one object's result into a universal DTCM multiplier.

DOCUMENTATION
Extend the memory checker to print named DTCM owners, free space, stack boundaries, canaries,
and forbidden DMA references. Record scratch and stack verdicts as separate ledger entries.

FINAL COMMAND
After retained implementation, widest relevant verification, docs, status, and commit:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be last.
```

---

## TASK 21R — M2 structural foundation for the generated fighter program

```text
/task Smash64DS_Port — TASK 21R: M2 structural preparation deletion and compact-table foundation

ROLE
Own the canonical Mario/Fox production capture and M2 preparation representation. Preserve
BattleShip callbacks, selected display lists, matrices, CPU lighting, material progression,
root/run order, and persistent vertex-cache/cross-matrix semantics. Treat decomp/ as read-only.

PRECONDITION
Use Task 25R's current artifact and owner decomposition. This task is a foundation for Task 27,
not the final generated fighter architecture.

SOURCE-BACKED HYPOTHESIS
The exact owner still processes 32 roots, 49 epochs, 67 runs, 626 triangles, 1,878 corners,
120 root light preambles, and 28 intra-root light changes. Some live arithmetic is irreducible,
but pointer-rich records, cold validation fields, repeated immutable binding, and selected shade
work can be reduced before generating the fixed root/epoch program.

PHASE 0 — CURRENT FIELD, LUT, AND COST CENSUS
1. Rebind current disassembly, stack-frame, call-count, and owner timing evidence.
2. Generate a consumed-field manifest for root selection, matrix input, material progression,
   light preambles, run class, texture/alpha state, and vertex-cache ownership.
3. Identify work already covered by the existing shade LUT and prepared-light-direction cache.
4. Do not create a second cache over already-retained work.

CUT 21A — SHADE/COLOR/UV BIT-KEY CENSUS, THEN MAYBE ONE CACHE
1. Instrument complete per-epoch keys: light direction/colors, material color/alpha, selectors,
   geometry mode, generation, and every consumed bit.
2. Measure hits over Countdown, early, Whispy/global-light, KO, and a natural window.
3. If useful natural-window hits are below 20%, stop and record no cache.
4. If hits survive, cache one final output class only, with profile-2 recomputation proving byte
   identity and fail-closed fallback before GX.

CUT 21B — HOT/COLD RECORD SPLIT AND IMMUTABLE PREBIND
1. Separate production-hot capture/submit fields from cold validation/proof metadata.
2. Prebind immutable root/epoch/material indices once per generation.
3. Use poison/full-assignment tests before deleting clears or narrowing fields.
4. Report 32-byte line footprint, object size, stack traffic, and reserve.

CUT 21C — COMPACT DIRECT-INDEX ROOT/EPOCH/RUN TABLES
1. Replace pointer-rich walking with checked narrow indices in exact source order.
2. Build the tables at generation transition under a fail-closed certificate.
3. Consume them with fixed typed loops/direct indexing, not a runtime opcode interpreter.
4. Expose the resulting tables and manifests as the required input to Task 27.
5. Do not declare the generated fighter architecture complete in this task.

EXACTNESS GATES
- exact 32 roots, 49 epochs, and 67 runs;
- exact 626 triangles / 1,878 corners and 320 Mario / 306 Fox ownership;
- exact 54 raw and 13 cross runs;
- exact 120 root and 28 intra-root light events;
- exact selected display lists, matrices, material progression, texture IDs, alpha, color,
  depth, clipping, and persistent vertex-cache/cross-matrix behavior;
- zero fallback after GX mutation, zero fences/conservation/upload faults;
- 0/49,152 changed pixels and full state/audio/lifecycle equality;
- no heap/I/O; reserve >=128 KiB.

MEASUREMENT
- Mode 8 owner-isolation window and Task 25R early phase.
- Mode 9 integration, Countdown, Whispy/global-light, and KO/rebirth.
- Record Mario, Fox, combined fighter, draw, active, loop, P50/P95/max, code/data size,
  stack traffic, veneers, and hot-line footprint.
- Emulator can provisionally retain a pure work deletion with unchanged footprint.
- Retail DS is required before final promotion of record-layout, code-size, cache, or table-
  footprint-sensitive results.

KEEP / REVERT
- Bank every independently repeatable exact positive cut with no relevant P95/phase loss.
- Kill only the cache mechanism when hit rate or key cost fails; do not discard independent
  record/table wins.
- Revert on one root/epoch/material/matrix/output mismatch, unkeyed live dependency,
  post-GX failure, reserve loss, or retail working-set regression.

DOCUMENTATION
Extend the fighter checker to prove consumed-field coverage, root/epoch/run order, field widths,
table provenance, and live-input ownership. Record 21A/B/C separately and identify which exact
outputs Task 27 may consume.

FINAL COMMAND
After retained implementation, widest relevant verification, docs, status, and commit:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be last.
```

---

## TASK 22R — exact wallpaper map retention and dirty-span writer

```text
/task Smash64DS_Port — TASK 22R: exact wallpaper map retention and dirty-span writer

ROLE
Own only the current production incremental wallpaper compositor. Preserve camera meaning,
source texture, exact RGB555 results, layer priority, VRAM ownership, presentation timing, and
all 3D output. Do not revive affine, correction-bitmap, scanline-affine, or display-capture labs.

PRECONDITION
Use Task 25R's same-artifact wallpaper and whole-frame rows. Measure before and after any Task
20R DTCM span scratch separately so the gains remain attributable.

SOURCE-BACKED HYPOTHESIS
The current path already uses exact recurrences, map comparison, packed stores, repeated-row
copying, and full-row DMA. The remaining opportunity is to avoid complete map/key work on exact
hits and replace sparse per-pixel dispatch with maximal contiguous exact destination spans.

PHASE A — CURRENT COST AND SPAN CENSUS
1. Separate timers/counters for key derivation, X map, Y map, changed-map comparison, span
   construction, physical writes, cache flush, DMA setup/wait, and repeated-row copy.
2. Enumerate maximal contiguous changed destination spans from the current exact changed map.
3. Initially pass those spans to the existing writer so behavior is unchanged.
4. Record span count, lengths, alignment, changed pixels, repeated rows, and writer choices for
   Countdown, early, Whispy, late, KO, rebirth, and camera-motion stress routes.

PHASE B — COMPLETE MAP-KEY RETENTION
1. Define every consumed camera/source/size/stride/format/generation field.
2. Retain exact X and Y maps only on complete bitwise key identity.
3. Rebuild one axis independently only when complete field ownership proves independence.
4. Profile 2 recomputes and byte-compares retained maps on sampled hits.
5. Fail closed to current map construction on uncertainty.

PHASE C — CPU DIRTY-SPAN WRITER
1. Add one writer for long aligned spans using packed 32-bit stores.
2. Preserve scalar edges and current exact source lookup.
3. Select writer by measured length/alignment only; no pixel tolerance.
4. Do not create or copy a second whole framebuffer.
5. Keep the full-raster oracle and exact destination address/value trace.

PHASE D — RETAIL DMA CROSSOVER
Only after the CPU span writer is a retained exact win:
1. Test one long, already-final, coherent span class.
2. Include source cache maintenance, DMA setup, bus contention, and completion in the owner.
3. Do not use a mutable whole-frame staging packet or many small DMA jobs.
4. Retain direct stores unless retail DS proves a consistent crossover.

EXACTNESS GATES
- 0/49,152 changed native pixels on every synchronized frame;
- exact X/Y-map hashes and complete destination address/value trace in profile 2;
- no stale rows, holes, out-of-range writes, or changed bank/layer priority;
- same production incremental selector in profile 0;
- exact gameplay/state/geometry/material/texture/audio/lifecycle behavior;
- no allocation, runtime I/O, asset conversion, or delayed visual state;
- reserve >=128 KiB.

MEASUREMENT
- Task 25R phase windows plus natural KO/rebirth and camera pan/zoom stress.
- Emulator for full-screen exactness.
- Retail DS for VRAM stores, cache maintenance, and DMA thresholds.
- Record wallpaper, draw, active, loop, P50/P95/max, interval/slips, and every writer counter.

KEEP / REVERT
- Bank each independently repeatable exact positive writer/key cut.
- Continue span expansion when the first representative writer saves at least 10K wallpaper P50
  or the measured histogram projects at least 40K across the owner.
- A smaller independent exact local cut may remain banked even when broad expansion stops.
- Revert on one stale pixel/address mismatch, key omission, camera/KO P95 regression, cache-
  flush loss, or DMA contention.

DOCUMENTATION
Extend the wallpaper checker to report production-vs-oracle identity, map-key fields, span
histogram, final-write bytes, and direct/DMA selection. Record every threshold with retail data.

FINAL COMMAND
After retained implementation, widest relevant verification, docs, status, and commit:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be last.
```

---

## TASK 23R — M3 consumed-field manifest and residual live-state reuse

```text
/task Smash64DS_Port — TASK 23R: M3 consumed-field certificate and residual exact reuse

ROLE
Own the M3 live-input manifest, invalidation proof, and at most one residual exact reuse class.
Preserve all BattleShip callback-visible state and the current Task-14 generation-gated topology
cache. Do not build a competing stage representation.

SEQUENCING
- Phase 0 is required before Task 26 and is an input to Task 26.
- Phase 1 runs only after Task 26 stabilizes.
- Do not cache work that Task 26 removes or precomputes.

PHASE 0 — CONSUMED-FIELD AND HIT-RATE CENSUS, NO REUSE
1. Enumerate every field consumed by stage validation, matrix/near-plane preparation,
   material/color/alpha/texture selection, visibility, run classification, phase boundaries,
   and callback-visible restoration.
2. Classify every field as:
   - immutable for the admitted generation;
   - live camera-dependent;
   - live camera-independent;
   - callback-visible mutation/output.
3. Split counters by all 8 segments and 4 material snapshots across Countdown, early,
   Whispy/material-change, late, KO, and rebirth.
4. Camera-dependent matrices/near-plane stay live-computed. Do not cache a low-hit lane.
5. Publish the complete invalidation manifest and live operand list consumed by Task 26.

PHASE 1 — ONE RESIDUAL CAMERA-INDEPENDENT CACHE, ONLY IF STILL MATERIAL
After Task 26:
1. Re-measure the residual owner and avoided computation.
2. Attempt one cache only when useful natural-window hits are >=20% and key cost is less than
   half the avoided computation.
3. Key every consumed material/color/alpha/selector/visibility/phase/generation bit.
4. Profile 2 recomputes and byte-compares sampled hits.
5. Fail closed to the current residual computation before GX mutation.
6. A one-shot mutation must produce full/hit/mismatch/inject/revalidate counters; lab-only
   symbols must be absent from production.

EXACTNESS GATES
- exact stage 8/255/57/42/54/202/49/4 and cross 5/10/15;
- exact dense/near/matrix counts and current depth bands;
- exact callback order, material progression, texture IDs, matrices, visibility, alpha, color,
  depth, and restoration state;
- zero fallback after GX mutation;
- 0/49,152 changed pixels and full state/audio/lifecycle equality;
- reserve >=128 KiB.

MEASUREMENT
Record residual stage, draw, active, loop, P50/P95/max, hit/miss/key/recompute costs, code/data
footprint, and interval/slip effects. Emulator proves exactness. Retail is required when cache
or record layout changes the working set.

KEEP / REVERT
- Phase 0 evidence is always kept.
- Phase 1 is killed when hit rate, key cost, or avoided work fails; no cache is a valid result.
- Bank a smaller exact residual win independently when repeatable and phase-safe.
- Revert on any missing invalidator, stale output, pixel/state mismatch, or retail regression.

DOCUMENTATION
Bind the consumed-field manifest to a checker so a new stage read fails the build until assigned
to immutable validation or the live operand block. Record Task 26 overlap explicitly.

FINAL COMMAND
After retained implementation, widest relevant verification, docs, status, and commit:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be last.
```

---

## TASK 26 — generated M3 stage live-value execution program

```text
/task Smash64DS_Port — TASK 26: generated M3 stage live-value execution program

ROLE
Own the canonical Dream Land mode-9 stage control representation and exact live-value binding.
Preserve every BattleShip callback-visible mutation, callback order, draw order, material
progression, matrix/depth class, texture, and triangle. Treat decomp/ as read-only.

PRECONDITIONS
- Task 25R current artifact and phase matrix exist.
- Task 23R Phase 0 consumed-field manifest exists.
- Consume the current Task-14 generation/stamp cache and any retained Task-23 evidence; do not
  reconstruct or double-count them.

WHY THIS IS NOT THE REJECTED TYPED EXECUTOR
This task generates a fixed scene-specific sequence:
- no runtime opcode scanner;
- no generic packet VM;
- no whole-owner packet copy/patch;
- no per-frame command-list construction;
- no material/triangle sorting;
- no post-GX fallback.
The output is direct typed C/records or straight-line generated code with explicit live operands.

SOURCE-BACKED HYPOTHESIS
Immutable stage topology/control is known, while runtime still binds and dispatches across
8 callback segments, 57 DObjs, 42 bindings, 54 runs, 49 epochs, 4 live material snapshots, and
202 triangles. A generated program can delete immutable traversal/policy while retaining live
camera matrices, near-plane state, material/color/alpha/visibility, texture selectors, and
callback-visible outputs.

PRE-CODE BOUNDS
- First architecture range: 40–110K stage ticks.
- Broad successful expansion may approach 160–180K; this is not initial budget credit.
- Compute a segment/run-proportional bound from Task 25R before choosing the first segment.

PHASE A — GENERATION CERTIFICATE AND HOT/COLD DATA
1. Validate immutable assets, offsets, policies, run classes, triangle ownership, callback order,
   and table checksums at construction/generation transition.
2. Retain every live per-frame admissibility check.
3. Split cold proof/debug metadata from compact hot execution rows.
4. Use narrow indices only where generated range/provenance assertions prove them.
5. Complete all owner validation before the first GX mutation.
6. Any uncertainty fails closed to the complete current path before GX.

PHASE B — ONE GENERATED SEGMENT
1. Select the segment with the largest current prepare/control burden.
2. Generate its exact fixed sequence:
   - immutable run order/classes;
   - immutable state effects;
   - indices into a compact live operand block;
   - exact callback start/end ownership.
3. The live operand block may contain only consumed live values identified by Task 23R.
4. Retain current exact raw/cross/source-Z/no-Z emitters in the first cut.
5. Add a diagnostic semantic/GX-word sink comparing current and generated execution before
   enabling the candidate.

PHASE C — CONTROLLED EXPANSION
Expand one segment or run class per independent commit/A/B. Do not combine arithmetic kernels,
DTCM, ARM/Thumb, DMA, GX state suppression, or new cross-frame caches with the structural A/B.

EXACTNESS GATES
- exact 8 callback segments and original callback order;
- exact 57 DObjs, 42 bindings, 54 runs, 202 triangles, 49 epochs, 4 materials;
- exact 121/828 frame contract and 202/320/306 ownership;
- exact cross/source-Z/no-Z classes and depth bands;
- exact matrix ownership, clipping, source-Z behavior, texture progression, alpha, light/color,
  polygon state, and depth;
- exact semantic/GX word trace for every generated segment;
- forced generation/live mutation fails or revalidates before GX;
- zero post-GX fallback, fence, conservation, conversion, or upload faults;
- 0/49,152 changed synchronized pixels;
- full gameplay/audio/lifecycle equality;
- reserve >=128 KiB.

MEASUREMENT
Use Task 25R control and measure Countdown, early, Whispy, KO, and material-transition phases.
Record segment and whole-stage P50/P95/max, draw/active/loop, code/table/BSS, 32-byte code/data
footprint, ARM/Thumb state, veneers, stack/spills, and interval/slip changes. Emulator proves
exactness. Retail DS decides generated code/data/cache performance.

KEEP / REVERT
- Bank every independently repeatable exact positive segment with no relevant P95/phase loss.
- Continue architectural expansion when the first representative segment saves >=8K whole-stage
  P50 or its measured projection supports >=40K whole-stage saving.
- A smaller exact local segment may remain banked even when broad expansion stops.
- Revert on one trace/state/pixel mismatch, post-GX failure, scanner-like runtime dispatch,
  whole-packet copying, or retail cache regression.

CHECKER
Prove complete consumed-field coverage, callback/segment/run order, table provenance/checksums,
field widths/ranges, live-operand indices, and absence of runtime scanning/whole-owner packets.

FINAL COMMAND
After retained implementation, widest relevant verification, docs, status, and commit:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be last.
```

---

## TASK 27 — generated M2 Mario/Fox root-and-epoch program

```text
/task Smash64DS_Port — TASK 27: generated M2 Mario/Fox root-and-epoch execution program

ROLE
Own the canonical mode-8 fighter control representation and exact live binding for Mario and
Fox. Preserve callback order, selected display lists, matrices, CPU lighting, material
progression, root/run order, and persistent vertex-cache/cross-matrix semantics. Treat decomp/
as read-only.

PRECONDITIONS
- Task 25R exists.
- Task 21R results are reconciled.
- Consume retained Task-21 hot/cold records, direct indices, and exact live outputs rather than
  rebuilding them.
- Any Task-21 cache remains an independent live-value source and is not double-counted.

WHY THIS IS NOT THE REJECTED TYPED EXECUTOR OR GX SKELETON
This task creates a fixed generated root/epoch/run sequence with direct live operands. It does
not create a general opcode interpreter, generic per-root dispatcher, whole-owner packet, GX
hierarchy with per-joint restores, hardware-lighting replacement, or blanket-inline owner.

SOURCE-BACKED HYPOTHESIS
The exact owner still processes immutable control across 32 roots, 49 material/light epochs,
67 runs, 626 triangles, 1,878 corners, 120 root light preambles, and 28 intra-root changes.
Root/run/material control is largely immutable, while matrices, materials, light values,
texture IDs, selection, and animation state remain live.

PRE-CODE BOUNDS
- First two-fighter architecture range: 35–95K.
- Broad structural result may approach 130–150K.
- Do not count Task-21 retained savings again.
- Calculate a Mario-only bound from Task 25R before implementation.

PHASE A — GENERATED MANIFEST
1. Generate and verify exact root order, epoch progression, run spans/classes, immutable state
   effects, expected triangle ownership, source provenance, and table checksums.
2. Maintain a consumed-field manifest for projection/modelview/composed matrices, selected DLs,
   material colors/alpha, texture IDs/progression, light values/preambles, geometry modes, and
   raw/cross vertex-cache state.
3. Every new read must fail the checker until assigned to immutable validation or the live block.

PHASE B — MARIO GENERATED PROGRAM
1. Generate Mario's fixed root/epoch/run sequence only.
2. Bind exact live operands from the current owner preparation.
3. Retain current raw/cross emitters and CPU lighting.
4. Fox remains on the complete current path as an in-frame control.
5. Validate Mario's complete owner before its first GX word.
6. Fail closed before GX only; no post-mutation fallback.

PHASE C — FOX EXPANSION
Expand to Fox only after Mario is a retained exact win. Keep Mario and Fox measurements
independently attributable.

PHASE D — OPTIONAL RESIDUAL SPECIALIZATION
Only after both generated programs stabilize, consider one remaining high-cost run class or
root matrix/light output. Do not add a new cache without complete-key hit census and avoided-
cost proof. Do not combine arithmetic or placement experiments with the generated A/B.

EXACTNESS GATES
- exact 32 roots, 49 epochs, 67 runs;
- exact 626 triangles / 1,878 corners and 320/306 ownership;
- exact 54 raw and 13 cross runs;
- exact 120 root and 28 intra-root light events;
- exact selected display lists, matrices, material progression, texture IDs, alpha, light/color,
  depth, clipping, and persistent vertex-cache/cross-matrix behavior;
- exact semantic/GX word trace for the generated fighter;
- zero post-GX fallback, fence, conservation, conversion, or upload faults;
- 0/49,152 changed synchronized pixels;
- full gameplay/audio/lifecycle equality;
- reserve >=128 KiB.

MEASUREMENT
Measure Mode 8 owner isolation, Mode 9 early interaction, Countdown, Whispy/global-light, and
KO/rebirth. Record Mario, Fox, combined fighter, draw/active/loop P50/P95/max, code/data/BSS,
stack traffic, frame size/spills, ARM/Thumb state, veneers, hot footprint, and interval/slips.
Emulator proves exactness. Retail DS decides code/data/cache working-set performance.

KEEP / REVERT
- Bank every independently repeatable exact positive slice with no relevant P95/phase loss.
- Continue broad expansion when Mario saves >=8K combined-fighter P50 or projects to >=35K for
  both fighters from measured counts.
- A smaller exact Mario-local win may remain banked when broad expansion stops.
- Revert on any root/epoch/material/matrix/word mismatch, unkeyed live dependency, post-GX
  failure, working-set regression, or retail phase regression.

CHECKER
Prove root/epoch/run sequence and ownership, exact field widths/ranges, table provenance,
complete live-input coverage, stack/veneer/code-size drift, and absence of runtime interpreters
or whole-owner packets.

FINAL COMMAND
After retained implementation, widest relevant verification, docs, status, and commit:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be last.
```

---

## TASK 28 — one-at-a-time machine-proved ARMv5TE exact kernels

```text
/task Smash64DS_Port — TASK 28: one-at-a-time machine-proved ARMv5TE exact kernels

ROLE
Own one measured arithmetic leaf or callsite per commit. Do not alter gameplay formulas,
renderer semantics, floating behavior, constants, scheduler, generated owner representation,
or unrelated helpers.

PRECONDITION
Task 25R current ELF/map/disassembly exists and the relevant Task 26/27 representation is stable,
so avoided work is not benchmarked twice. Use current Task-19 placement and live ITCM budget.

TARGET ORDER
1. Highest-cost exact matrix/affine dot product still present after generated programs.
2. Exact packed-halfword lighting/color dot product.
3. One bounded 64-bit-to-32-bit transform/rounding callsite.
4. One remaining high-value helper demonstrated by the current census.
Never select the global generic fmul wrapper merely from historical call count.

PRE-CODE BOUND
For each candidate derive calls/frame and inclusive ticks from the current artifact. Reject a
candidate with less than a 2K whole-owner theoretical ceiling unless it also removes code or a
helper dependency. Working range: 3–30K for one leaf, no more than 40K for one family.

MINIMAL CHANGE
1. Freeze the exact current linked result as golden.
2. Specify complete operand domains, sign extension, intermediate widths, overflow, shifts,
   rounding, saturation, and floating special values where applicable.
3. Inspect current GCC output; do not replace an equivalent sequence.
4. Implement one ARM leaf/callsite fast path with literal current fallback outside proved domain.
5. Preserve ABI, callee-saves, flags, alignment, interworking, and section budget.
6. Do not combine two arithmetic families in the first A/B.

PROOF GATES
- exhaustive equality over finite admitted axes;
- SMT or complete mathematical partition where practical;
- >=100 million directed/random vectors for broader domains;
- literal ARM9 candidate-vs-current golden corpus;
- exact state rows, matrix/light hashes, geometry, materials, textures, audio, and pixels;
- no helper call, stack frame, divide, or unintended veneer unless explicitly measured;
- ITCM/DTCM/main-text report and no displacement regression;
- reserve >=128 KiB.

MEASUREMENT
Representative microbenchmark and natural owner path. Emulator for arithmetic/state/pixel
falsification; retail DS for scheduling/mode/placement. Record bytes, instructions, calls/frame,
ticks/call, owner/draw/active/loop P50/P95/max.

KEEP / REVERT
- Revert immediately on one arithmetic mismatch.
- Bank every repeatable natural-path exact gain with no P95 or displacement regression.
- A faster microbenchmark with flat/slower natural owner is REVERT.
- Do not revive the rejected finite-zero/global fmul mechanism.

CHECKER
After a winning leaf stabilizes, add machine-code shape/hash and golden-object drift checks bound
to the current compiler/libgcc identity.

FINAL COMMAND
After retained implementation, widest relevant verification, docs, status, and commit:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be last.
```

---

## TASK 29 — exact redundant GX state and one immutable-run stream

```text
/task Smash64DS_Port — TASK 29: exact redundant GX state and one immutable-run stream

ROLE
Own only equality-proven GX state suppression and, afterward, one long immutable run whose
generated representation is final. Preserve command order, owner order, source callbacks,
matrix/material progression, and every emitted value.

PRECONDITION
Run only after Tasks 26 and 27 stabilize. Otherwise generated AOT changes invalidate counts,
layouts, and attribution.

SOURCE-BACKED HYPOTHESIS
GX transport is not the primary wall, but 121 runs and repeated state/material epochs may contain
identical consecutive writes. A software shadow can remove side-effect-free identical writes. A
long immutable run may stream prepacked constant words while binding sparse live fields without
reproducing the rejected whole-owner packet mechanism.

WORKING BOUNDS
- exact redundant-state suppression: 5–30K draw ticks;
- immutable-run template envelope: 20–70K;
- these ranges overlap Tasks 26/27 and each other; never add all ceilings.

PHASE A — NO-BEHAVIOR CENSUS
1. Count every GX command by class and owner.
2. Count identical consecutive values within exact reset/owner/flush lifetime.
3. Mark side-effectful or uncertain commands as never suppressible.
4. Hash the emitted semantic/GX stream and owner boundaries.
5. Make no suppression in the census build.

PHASE B — ONE STATE CLASS
1. Select one frequent side-effect-free command class.
2. Maintain an exact software shadow with explicit begin-frame, owner, fallback, and flush
   invalidation.
3. Suppress only a command whose hardware-visible value is proven identical.
4. Do not reorder, batch across owners, or change translucent ordering.
5. Add forced reset/state-mutation tests.

PHASE C — ONE LONG IMMUTABLE RUN
Proceed only when the final generated programs identify a long span with a small live-field set.
1. Prepack immutable command words offline/build-time.
2. Bind live words directly while streaming to the current FIFO path.
3. Do not construct, copy, patch, or cache-flush a whole mutable owner packet.
4. Do not use many small lists, DMA, or a generic scanner in the first cut.
5. Complete whole-owner validation before the first specialized GX word.
6. Keep current direct emission as control and pre-mutation fallback.

EXACTNESS GATES
- exact semantic/GX sequence after accounting only for removed identical side-effect-free writes;
- exact 121 runs / 828 triangles and 202/320/306 ownership;
- exact matrix/material/texture/color/alpha/depth/translucent order;
- exact GX status/fence/conservation/reset behavior;
- 0/49,152 pixels and full state/audio/lifecycle equality;
- no post-GX fallback, packet copy, runtime allocation, or I/O;
- reserve >=128 KiB.

MEASUREMENT
Task 25R early, Whispy, and KO windows. Emulator exactness first; retail DS is the transport/
MMIO/FIFO referee. Record command counts, suppressed writes, FIFO stalls, owner/draw/active/loop
P50/P95/max, code/data bytes, and interval/slips.

KEEP / REVERT
- Bank each exact command-class reduction only when retail shows a repeatable positive result.
- Continue immutable-run work when the first long segment saves >=5K whole-frame or projects to
  >=20K across qualifying spans.
- Revert on one word/state/pixel mismatch, reset-lifetime bug, increased FIFO stalls, packet-copy
  behavior, or retail P95 regression.

FORBIDDEN REOPENINGS
- whole-owner FIFO copy/patch/DMA;
- many small GX lists or generic glCallList conversion;
- generic packet scanner/interpreter;
- per-joint GX hierarchy/restore traffic;
- whole-fighter GX skeleton;
- hardware lighting without full bit parity.

FINAL COMMAND
After retained implementation, widest relevant verification, docs, status, and commit:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be last.
```

---

## TASK 24R — repository diet, deferred quiet-slot work

```text
/task Smash64DS_Port — TASK 24R: evidence-safe repository diet in a quiet slot

PRIORITY
This task has zero frame-time value. Do not run it while Task 25R, a renderer structural task,
or a hardware packet is blocked only by lane availability. Run it only when disk pressure blocks
implementation/verification or all performance lanes are waiting on external device evidence.

SCOPE
Use the original Task-24 evidence-first deletion rules, with these amendments:
- never remove the Task 25R baseline packet, current generated-program A/Bs, open device pairs,
  artifacts/performance, or artifacts/visibility;
- preserve every file cited by an OPEN ledger decision;
- remove only closed/superseded worktrees, branches, lab builds, telemetry, and logs after proof;
- no aggressive git gc;
- run focused evidence/hash/checker checks plus the final profile-0 Boundary gate
  after the deletion batch;
- record before/after sizes and exact retained evidence roots.

KEEP / REVERT
A deletion batch is kept only when all cited evidence remains resolvable and the complete safety
gates pass. Restore any batch that breaks a checker, build, citation, artifact lookup, or current
A/B workflow.

FINAL COMMAND
After docs, status, and commit:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be last.
```

---

## TASK 30 — final retail stable-30 qualification and freeze

```text
/task Smash64DS_Port — TASK 30: final retail stable-30 qualification and performance freeze

ROLE
Own final evidence and release gating only. Make no optimization in this task except repairing a
measurement/checker defect that does not change profile-0 behavior.

PRECONDITION
Task 25R exists and all retained Tasks 20R–29 results are integrated. The current same-artifact
matrix predicts every phase has sufficient P95/max margin to justify retail qualification.

QUALIFICATION ROM
Build one profile-0 production ROM and one profile-1 diagnostic sibling from the same exact HEAD,
toolchain, and production selectors. Record complete identity and map/section/reserve data.

RETAIL RUNS
Run three cold-boot canonical full lifecycles on stock retail DS:
- Countdown / GO;
- early, mid/Whispy, and late combat;
- natural KO and rebirth;
- Time Up, Results, and exactly one teardown.
No manual phase skipping or oracle selector is allowed.

HARD PASS GATES
- exactly two source updates per presentation for every sample window and full lifecycle;
- zero debt/catch-up behavior;
- zero presentation intervals of 3+ VBlanks;
- zero pacing-slip events;
- approximately 30 presentations/s and 60 updates/s in every canonical phase;
- P50, P95, and maximum all consistent with the two-VBlank deadline;
- exact state/audio/lifecycle/geometry/material/texture/depth/order counters;
- 0/49,152 synchronized native pixels;
- zero fallback/fence/conservation/conversion/upload faults;
- reserve >=128 KiB throughout;
- no stack, IRQ, DMA, GX, or teardown fault.

FAILURE HANDLING
A failed phase is not averaged away. Bind it to the exact owner/interval in the Task 25R matrix,
reopen the smallest responsible task, and preserve all other retained exact wins. Do not weaken
the stable-30 checker to pass.

OUTPUT
- signed-off final phase matrix with P50/P95/max and interval histogram;
- ROM/ELF/map hashes and device identity;
- exact stable-30 checker output;
- final performance ledger and board status;
- list of all retained and rejected mechanisms;
- release freeze note preventing stale experimental selectors from becoming canonical.

FINAL COMMAND
After the final docs/status/commit and no further project commands:
  .\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
must be last.
```

---

## Closed mechanisms — do not spend another cycle without a materially different design

- Whole-owner FIFO copy/patch/DMA or mutable packet caching.
- Generic packet scanners, bytecode VMs, or new per-root interpreters.
- Many small GX lists / generic `glCallList` conversion.
- Per-joint GX hierarchy/restore traffic and the whole-fighter GX skeleton.
- Approximate hardware lighting or any non-bit-identical lighting substitution.
- Global finite-zero or generic `fmul` wrapper experiments already measured slower.
- Affine wallpaper, affine-plus-correction, scanline-affine, or stale KO-spike work.
- Runtime texture conversion, NitroFS/on-demand loading, or new M4 residency work.
- Visual owner skipping, mesh decimation, LOD, lower visual cadence, or pixel-tolerance gates.
- Reimplementing constant-depth no-Z GX painting, hardware divider migration, or Task-14 dense
  topology discovery: those mechanisms are already canonical.

The performance-critical path is now explicit: establish one artifact, generate fixed exact M3
and M2 control programs with compact live operands, bank disjoint DTCM/wallpaper wins, then use
machine-proved arithmetic and equality-only GX reductions to close the remaining margin.
