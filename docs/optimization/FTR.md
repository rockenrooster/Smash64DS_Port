# Smash64DS — FTR optimization re-evaluation

**Date:** September 17, 2026  
**Repository:** `rockenrooster/Smash64DS_Port`  
**Reviewed branch:** `master`  
**Pinned source:** `db0d088bc61ac3e85f07a349857a1b3ec7eef55b`  
**Commit timestamp:** September 17, 2026, 13:27:07 UTC / 08:27:07 America/Chicago  
**Previous review:** `430aca2879e9071dc2b22f944f5c2909c9ce7aa4`  
**Deliverable:** Research, corrected candidate selection, and implementation/test guidance. No repository edits or new ROM benchmarks.

## Executive decision

**Keep the objective of a simpler, prebound native fighter renderer, but substantially narrow the earlier implementation recommendation.** Start with a compact packet-hit representation and right-sized patch metadata, not a wholesale pose rewrite or a large resident variant bank.

The most useful newly identified opportunity is concrete: **every one of the four fighter packets reserves 1,248 bytes of texgen group/site metadata, for 4,992 bytes total, regardless of whether that instance uses texgen.** The packet also reserves maximum-sized shade and root tables. Right-sizing these cold tables can return resident memory while a compact hot header improves the organization of the replay path. The amount actually reclaimable depends on the admitted roster and supported states; 4,992 bytes is the existing texgen reservation, not a promised saving. [R8]

This matters because the latest repository evidence shows a gameplay-arena page boundary so tight that an addition of 204 bytes of effect texels **plus its associated roots/code** caused a 4,096-byte arena reduction and native rejects. Large caches cannot be proposed as though their only cost were their nominal byte size. [R14]

My revised priorities are:

| Priority | Candidate | Revised disposition |
|---|---|---|
| 1 | Compact hot packet-hit state and right-sized cold patch metadata | **Best first engineering slice.** Concrete storage opportunities; CPU savings remain unmeasured. |
| 2 | Direct-index Link texgen patching | **Keep, as a bounded supporting optimization.** Remove searches without changing UV arithmetic. |
| 3 | Dirty-range cache cleaning plus versioned light/tint parameters | **Measure as a small integrated replay improvement.** Do not remove cache maintenance or lifecycle barriers. |
| 4 | Root-local packet variants | **Conditional P95 candidate.** First correlate rebuilds with expensive presented frames; no speculative variant bank. |
| 5 | Fighter GX execution-work reduction | **Host feasibility first.** Count actual vertex submissions, matrix operations, lighting work, and barriers—not merely packet words. |
| 6 | Producer-owned pose/matrix output | **Defer the broad rewrite.** Existing adverse pilot evidence must be recovered and distinguished from the proposed replacement. |

**There is no newly measured FTR package here that can honestly be declared sufficient for 30 FPS. There is also no valid architectural impossibility proof in the evidence reviewed.** FTR should deliver measured contributions to the combined FTR/SRC/STG/MISC campaign, with 60 Hz simulation and native-only rendering retained. [R2], [R3], [R4]

---

## 1. Scope and evidence quality

The branch collection returned only `master`. A subsequent comparison of the pinned commit against `master` returned `identical`, with no intervening commits. Some code-search results advertised another revision; all substantive current-source checks below use the explicit pin rather than those search-result revisions. [R1]

Evidence labels used throughout:

- **Source-confirmed:** a mechanism or allocation is visible in the inspected code.
- **Repository-measured:** a measurement is reported in a retrieved repository artifact; it was not rerun here.
- **Reported but primary receipt unavailable:** a later summary names an experiment whose original artifact could not be retrieved.
- **Derived:** arithmetic or reasoning from stated inputs, not a runtime measurement.
- **Candidate:** a proposed change that still needs implementation and measurement.

The audit includes the current execution board, the new critique of the prior FTR report, current packet/replay and display-contract code, matrix construction, existing negative experiments, the DTCM experiment, the sampler's percentile implementation, and primary libnds/emulator implementation references. This is a targeted re-evaluation of FTR and its boundaries, not a claim to have inspected every line of the repository.

### Non-negotiable constraints

Four-player stable 30 FPS remains the target. Preserve 60 Hz simulation: the newer owner ruling specifically refuses the otherwise broadly permitted 30 Hz simulation option in `PROJECT_GOAL.md`. Keep required content and source gameplay behavior, retain HIGH detail where required, and do not add a target-side N64 graphics interpreter or generic compatibility-renderer fallback. Unsupported content rejected before drawing is contained failure, not completed content. [R2], [R3]

Use the approved accuracy-focused melonDS configuration, with interpreter/JIT-disabled profiling, for ordinary development. No new retail-DS measurement campaign is part of this plan. Source references under `decomp/` remain read-only; adaptations belong in the port/import/generator layers. [R2], [R19]

---

## 2. What changed since the previous answer

### 2.1 The qualified baseline moved

The current board identifies `a4eb24c9a85` as its last qualified integration checkpoint, with Boundary green on all three arms on September 17. These are **the board's recorded figures**, not measurements of a ROM built from every subsequent documentation commit. [R3]

| Metric | P50 | P95 |
|---|---:|---:|
| WORK-H | 1,600,960 | 2,320,576 |
| FTR | 350,144 | 736,960 |
| STG | 385,088 | 427,648 |
| SRC | 543,040 | 1,027,520 |
| MISC | 238,720 | 464,000 |

Gate: **1,120,000 ticks per presented frame**. Direct differences from the WORK-H distribution are **480,960 at P50** and **1,200,576 at P95**. These quantify the problem; they do not identify which instructions must be removed. [R3], [R4]

The last qualified checkpoint reports 111,200 bytes of heap low-water and a 1,351,424-byte arena. Those numbers do **not** mean an arbitrary 100 KB renderer cache is available: reserve requirements, allocation order, legal content combinations, alignment, and the arena page boundary still apply. [R3], [R14]

### 2.2 A newer locality experiment is important, but is not a new FTR win to bank

The DTCM hot-scalar artifact reports moving 112 statics totaling 508 bytes. Its own matched comparison is: [R5]

| Metric | Its control | Its candidate | Difference |
|---|---:|---:|---:|
| WORK-H P50 | 1,580,544 | 1,537,344 | −43,200 |
| WORK-H P95 | 2,320,768 | 2,277,696 | −43,072 |
| FTR P50 | 354,432 | 349,184 | −5,248 |
| ALL P50 | 1,678,016 | 1,677,888 | −128 |

The artifact also records an unresolved sample-window assertion, repeated runs, and instruction-level corroboration. It should therefore be described as a supported measured candidate with its exact verification status—not silently substituted for the separate qualified checkpoint above. [R5]

Two lessons survive that qualification. Small hot-data changes can influence several subsystems, and a real WORK-H reduction can leave VBlank-quantized ALL unchanged. Neither lesson authorizes adding the 43,200 to another baseline or crediting all of it to FTR. [R5]

### 2.3 Several earlier suggestions need demotion

The previous answer correctly recognized that packet replay and prechecked execution already exist, but it still presented a broad bound-renderer/pose/compiler program too readily as the strongest implementation path. It did not sufficiently constrain that program by the remaining cost, memory headroom, and adverse experiments.

The revised plan therefore separates **a small replacement of the hot replay contract**, **conditional tail work**, and **larger research directions**. They are not equally ready to implement.

---

## 3. Corrections to the new “all candidates exhausted” sizing report

The September 17 sizing artifact is valuable: it identifies work already banked, quotes negative experiments, and highlights resident-memory costs. However, some of its conclusions exceed the evidence. Do not replace optimism with equally unsupported impossibility claims. [R4]

### 3.1 Separate medians cannot prove overlapping timers

The report says that bucket percentages totaling 124% of ALL prove the lanes are not disjoint. But these are separately computed percentiles, not per-row totals. Medians are not additive.

A counterexample with completely disjoint work:

| Frame | A | B | C | Total |
|---|---:|---:|---:|---:|
| 1 | 10 | 10 | 0 | 20 |
| 2 | 10 | 0 | 10 | 20 |
| 3 | 0 | 10 | 10 | 20 |
| Median | 10 | 10 | 10 | 20 |

The component medians total 30, or 150% of the total median, with no overlap whatsoever. This is a mathematical counterexample, not game data.

Some project timers really are nested; the ledger explicitly warns against adding nested diagnostic timers. Establish their relationships from the bracket locations and same-row data. The median sum is not evidence either way. [R13], [R19]

### 3.2 “Delete FTR's median” is not a valid percentile ceiling

Likewise, `median(WORK-H) − median(FTR)` is not the median of a frame with its FTR contribution removed.

For example, let whole-frame work be `[50, 100, 150]` and one exclusive component be `[0, 0, 100]`. Its median is zero, but deleting it changes whole-frame work to `[50, 100, 50]`, reducing the median from 100 to 50.

For a purely arithmetic what-if, the correct operation is:

```text
For each valid, aligned frame i:
    optimistic_work[i] = work[i] − removable_component[i]
Then calculate percentiles of optimistic_work[].
```

Even that is not a hardware speedup prediction. Removing producer work can expose a later FIFO wait, change cache behavior, or alter overlap. An arithmetic deletion scenario is an informative budget screen only. The actual A/B must measure whole-frame work and cadence.

The sampler already computes WORK-H by subtraction **per sample before percentile calculation**. Apply the same discipline to optimization sizing. [R13]

### 3.3 Task 55 is not a universal fighter-compiler refutation

Task 55 tested **stage** COLOR/TEX_COORD elision in July. Its recorded packet shrank from 3,916 to 3,561 words, while ALL P50 moved +64 ticks. That is strong negative evidence for expecting word compression alone to deliver a major improvement on that tested workload. [R6]

It does not establish that every fighter has the same critical path, or that eliminating expensive matrix/lighting/primitive operations is identical to deleting cheap state writes.

There is also a correctness correction the newer summary omits: the Task 55 artifact's **post-STOP owner follow-up reports pulsating colors**, contradicting its earlier “lossless” characterization. The experiment remains rejected for both performance and visual reasons. A future compiler must explicitly test cross-frame state, not copy the original abstract losslessness argument. [R6]

### 3.4 The adverse pose pilot needs its actual receipt

The newer sizing artifact reports a `N05.05` pose/draw pilot with FTR median **+14,336**, WORK median **+12,864**, 10,554 native-transform draws, 48,720 misses, and 12,861 stale observations. That is meaningful adverse evidence and must not be ignored. [R4]

However, both the named directory and its `README.md` returned 404 at the reviewed pin. Searches found the claim in the sizing summary, not the original implementation/measurement receipt. Accordingly:

**Treat that implementation as reported rejected; do not repeat it. Do not claim this audit independently verified that it implemented exactly the proposed producer-to-final-packet replacement.** Miss/stale counts suggest a reuse/validity mechanism, but its exact structure cannot be established from the summary alone.

A future reopening needs the original patch, flags, counters, and paired results, followed by an explicit explanation of what is different. This is a prerequisite for the broad matrix lane, not a reason to spend another build recreating the old experiment.

### 3.5 A 2.6% draw-level record rate can still matter to P95

The sizing report quotes **178 records and 6,673 hits**, or 2.598% records per draw. It also reports zero root-count and texture-residency misses in the inspected captures. The original frame-correlated population is not supplied by that summary. [R4]

With four independent draws per frame, a purely illustrative probability of at least one record would be:

```text
p = 178 / (178 + 6673) = 0.0259816
P(at least one record in four draws) = 1 − (1 − p)^4 = 0.0999459
```

That is approximately **10% of frames**, not 2.6%. Independence is not established, and frames do not necessarily all have four eligible draws. The calculation is not an estimate of the actual run; it shows why a draw-level rate cannot dismiss a frame-level P95 candidate.

A root-program switch with unchanged root count can still alter other key fields. Zero root-count misses is not equivalent to zero program-switch misses. Inspect the actual key construction and correlate all causes with frames. [R7]

---

## 4. Current runtime: optimize what actually executes

The current production executor first consumes a successful same-frame precheck and attempts replay **before whole-owner preflight**. That bypass is already implemented. A prechecked replay does not rebuild the full key or re-run texgen; the adapter's precheck already did that work. Do not propose these deletions again. [R7], [R11]

A representative hit path is:

```text
Source display-head behavior and display-contract handling
  → draw-plan/binding handling
  → matrix production
  → material identity and live input refresh
  → packet precheck: key, residency, applicable texgen
  → consume same-frame precheck
  → texture-use bookkeeping, tint, matrix/light patches
  → cache maintenance
  → asynchronous FIFO DMA
  → next FIFO writer honors completion/order
```

The display head is not just a renderer query. Current source comments identify off-screen player-arrow HUD behavior, fog/light state, and scale state there. The cached walk already relies on the head's output and on status-generation invalidation; a DObj-tree-only key was refuted. A new bound path must preserve these behaviors. [R9]

The packet records geometry and patches dynamic parameters. It already handles hurt-flash tint without re-recording, tracks individual texture identity, and refreshes replayed textures' use timestamps. Those earlier fixes are baseline functionality. [R7], [R8]

Finally, “fighter-related” does not mean “inside FTR.” Pose updates and gameplay transform validity can execute under SRC; later waits can be charged outside the submitting fighter. Attribute at the actual execution site and avoid charging an improvement twice.

---

## 5. Candidate FTR-R1 — compact replay state and right-sized cold metadata

**Priority:** first implementation slice.  
**Evidence:** source-confirmed storage and hot-path structure; performance gain unmeasured.  
**Main files:** `src/nds/nds_renderer_preamble.c`, `src/nds/nds_renderer_native_common.c`, `src/port/renderer_adapter_fighter.c`.

### 5.1 The concrete storage opportunity

`NDSFighterPacket` unconditionally embeds, for each of four instances: [R8]

| Embedded table | Current capacity | Element size, derived from fixed-width fields | Bytes per packet |
|---|---:|---:|---:|
| Texgen groups | 8 | 28 | 224 |
| Texgen patch sites | 256 | 4 | 1,024 |
| **Texgen total** | | | **1,248** |
| Shade patch sites | 64 | 20 | 1,280 |

Thus texgen reservation totals **4,992 bytes**, and shade-site reservation totals **5,120 bytes**, before root, texture, and header storage. The fixed-field texgen sizes were checked with a host layout calculation; confirm them with target `sizeof`/static assertions before editing the ABI. [R8]

These are allocations to investigate, not quantities that can all be deleted. Four legal texgen-capable fighters still need four independent live parameter sets. A roster with only one such instance would have 3,744 bytes in the other three maximum texgen reservations before accounting for replacement descriptors and allocation overhead. That is an illustrative storage opportunity, not a qualification of any particular roster.

### 5.2 Proposed organization

Separate a small **hot replay header** from **cold or feature-specific tails**:

```text
Per-instance hot header:
    packet pointer / length / validity
    lifetime and program identity
    exact replay shape
    pointers or indices to active matrix patches
    current parameter versions

Cold/optional storage:
    recorder bookkeeping
    feature-specific texgen sites
    shade patch descriptors
    infrequent diagnostics and miss history
```

Only the required tails are resident. Determine their capacities at fighter/scene binding from generated maxima over every reachable state, rather than allocating during a combat draw.

Preserve one contiguous GX stream per fighter initially. This proposal is not a many-fragment DMA architecture.

### 5.3 Two possible allocation strategies

**Strategy A: replace the static maximum arrays with a scene-owned compact pool.** Size it from the admitted instances and all their reachable root programs. Construct it before gameplay; release it with the scene generation. Account for allocation headers and alignment. The net result must return memory, not merely move it.

**Strategy B: use verified spare capacity inside each existing packet region.** The code reserves **141,440 bytes**, divided into four **35,360-byte** regions, stopping before an aliased framebuffer tail. Metadata placed there reduces that region's available command capacity; it is safe only after all relevant packet sizes, including CSS HIGH and alternate roots, are bounded. The full 147,840-byte backing framebuffer is not all available. [R8]

Strategy B must not become “try it and fail closed on a large legal fighter.” Insufficient capacity is a failed candidate until required content fits.

### 5.4 CPU-side simplification

Specialize the replay by its already-validated shape. A split-matrix packet can use a compact split-patch loop; a GX-chain packet uses its own loop. Do not scan the large producer structure just to learn the shape again for every root.

Initially keep current key checks and all numerical producers unchanged. This isolates representation/locality effects. After that version is proved, delete only reconstructed inputs with no remaining consumer.

A compact header may be a DTCM candidate, but **the 508-byte hot-scalar move is already banked in its own experiment**. Do not move its symbols again or claim their gain again. The reported remaining DTCM headroom of 1,484 bytes is specific to that build and must be rechecked. DMA packet words must remain in DMA-visible storage. [R5], [R8]

### 5.5 Promotion and rejection tests

Promote when target layout proves the intended storage return, legal content remains covered, old hot reads actually disappear, and matched WORK-H improves without a tail/cadence regression.

Reject or revise when the pool adds allocations during gameplay, worst-case metadata negates the saving, command capacity is reduced below a required state, or pointer indirection outweighs locality benefits. Unused cold arrays are a memory cost; their mere size does not prove they were causing cache misses every frame.

**Why this is first:** it addresses an immediate resource constraint and creates a small, measurable base for the rest of the replay work without changing simulation, geometry, or pose math.

---

## 6. Candidate FTR-R2 — direct-index Link texgen

**Priority:** implementable supporting slice, preferably sharing R1's compact metadata.  
**Evidence:** current source confirms the search loop.  
**Main function:** `ndsFighterPacketPatchTexgen`.

The function currently visits sites, linearly searches `cached_dense[]`, computes each unique dense vertex's UV once, and copies its word to repeated sites. Its comments establish a 28-unique-vertex bound for Link's generated run in both details. [R7]

Build a local-index mapping once:

```text
unique_inputs[local_index] = source dense ID or exact normal inputs
patch_sites[j] = { packet_word_index, local_index }
```

Then evaluate each unique input once and scatter by local index. Preserve first-use order and the exact existing `ndsRendererNativePrepareTexgenDirectionQ15` / `ndsRendererNativeTexgenCoord` arithmetic. Keep group identity, source-table identity, root modelview, LookAt, and texture parameters separate; the same dense ID in two different groups is not automatically the same result. [R7]

For U unique inputs and S sites, the lookup structure changes from up to O(S×U) comparisons to O(U+S) operations. That is an algorithmic reduction, not a tick estimate.

The preceding profile attributed roughly 8.1K exclusive ticks/frame to this function. That is neither current standalone savings nor the cost of only its searches. UV evaluation, validation, and packet writes remain. Expect a supporting contribution, not a 30-FPS breakthrough. [R4], [R18]

A proof-oriented implementation can replace each site's dense ID with its local index while keeping a compact unique-input table per active group. Measure the resulting net metadata size; do not create a second maximum-sized 256-site copy.

**Checks:** compare every patched UV word against the current path over moving cameras, poses, details, material changes, and CSS/battle transitions. The host-only synthetic tests performed for this report verify the indexing transformation, not the source UV formula or target timing.

**Stop condition:** the searches disappear but whole-frame work does not improve, or the metadata causes a larger cache/residency regression. Keep the old arithmetic; do not combine a data-layout test with approximate texgen.

---

## 7. Candidate FTR-R3 — cache-maintenance scope and parameter versions

**Priority:** bounded measurement after R1; combine tiny compatible improvements rather than creating a long micro-optimization campaign.

### 7.1 Clean only the packet ranges that can have changed

A successful replay currently patches a subset of the packet, then calls `DC_FlushRange` over the entire word stream. [R7]

Build a sorted, merged set of possible dirty cache-line ranges from the patch descriptors at bind/record time. The initial recording still cleans the full packet. Later replays can clean the applicable ranges after all writes, followed by the required write-buffer ordering before DMA.

This is a **candidate**, not evidence of wasted full-buffer writeback: a range clean can inspect clean lines without writing their payloads. The benefit depends on line traversal cost, dirty density, and the overhead of multiple calls. The libnds implementation/documentation confirms that CPU-cache visibility must be handled before DMA; skipping the clean entirely is not correct. [R15], [R16]

Use a generated/static policy when a packet is effectively fully dirty. Do not sort ranges, allocate memory, or run a large dirty-bit scan each frame just to avoid a short clean.

Cost model:

```text
net saving ≈ saved cache-line maintenance work
             − range-dispatch overhead
             − version/dirty bookkeeping
             − any lost CPU/GX overlap
```

Validate actual memory consumed by DMA after the clean, not just the CPU's cached view. Partial writes, lines containing both static and dynamic words, recording-to-replay transitions, and reused scene storage all need coverage.

### 7.2 Light/tint versions: small, exact supporting work

Current code already returns early from tint application when modulation and a root-color hash are unchanged. What remains is the hash walk and light normalization work; those are not a fresh whole-subsystem opportunity. [R7]

A safe first change is to memoize the exact input light direction and its resulting packed word. It must still be emitted/applied under the correct hardware state. Eliminating the CPU normalization does not eliminate the hardware light command automatically.

More aggressive versions require every material/tint writer to participate. A version that fails to change on one source path is a stale-rendering bug. Keep content comparison in development until mutation coverage is established.

### 7.3 DMA lifetime is part of the contract

There is already asynchronous FIFO DMA. Do not call that a new optimization. The packet cannot be mutated while DMA can still read it, and no other FIFO writer may interleave words with that DMA. [R7], [R8]

A compact replay refactor must preserve the precheck's validity from validation through submission. Introducing resource changes, callbacks, or deferred queueing between those points invalidates the current immediate-handoff proof.

---

## 8. Candidate FTR-R4 — root-local variants, only if the tail evidence supports them

**Priority:** conditional.  
**Objective:** lower expensive packet rebuilds, primarily P95 rather than steady-state P50.

Whole-owner packets currently invalidate for a changed key. A source root-program switch can therefore require reconstructing unchanged roots. Factoring variants by affected root is a legitimate architectural alternative, but its value must come from the actual rebuild population. [R4], [R7]

### Required population before implementation

For each logical presented-frame record, collect each fighter instance's replay/record/direct mode, miss causes, program identity, and relevant costs. Correlate them with **the same frames' WORK-H and FTR**, not a separately sorted top-5% FTR population.

Answer three questions:

1. What fraction of expensive whole frames contains a record?
2. How much incremental CPU cost does that record add relative to an equivalent hit?
3. Which changes could actually be serviced by local replacement?

The reported zero root-count/residency misses makes those two particular explanations less likely in that captured population. It does not remove same-count program switches, changed material identities, intentional direct rendering, or other key changes from consideration. [R4], [R7]

### Proposed mechanism

Retain one assembled per-instance packet. Precompute local variants or patch recipes only for the few causes proven to dominate rebuild cost. Share immutable geometry descriptions; keep instance-specific live matrices, tint, and texture placement independent.

Do not create a full fighter packet for every combination of animation, face, costume, held object, and root variant. That multiplies storage and invalidation complexity.

Rebuild affected slices only at valid command/primitive boundaries, reconstruct packed headers, update patch offsets, and preserve preceding state that later slices inherit. Transparent ordering, matrix-slot lifetimes, and material side effects are part of the slice contract.

Captain HIGH is explicitly excluded from current FIFO-only replay because its alpha-test behavior includes non-FIFO register state. A new universal packet abstraction must model that case or retain its complete qualified native direct implementation. It cannot simply force replay eligibility. [R7]

**Stop condition:** records do not substantially overlap the relevant tail, or the memory/copy cost of variants is not repaid. No large resident bank is authorized by this research recommendation.

---

## 9. Candidate FTR-R5 — optimize GX execution work, not the byte count alone

**Priority:** host-side feasibility and stream census before target implementation.

Split the previous broad “lossless packet compilation” proposal into two materially different classes.

### Class A: shorter encodings of the same operations

Examples include exact `VTX_XY`/`VTX_XZ`/`VTX_YZ`/`VTX_DIFF` substitution, and `VTX_10` only for coordinates representable without losing required precision. These may reduce ROM/RAM/transfer size, but all still submit a vertex. The upstream emulator implementation routes these forms through vertex submission; it is supporting implementation evidence, not a replacement for profiling the approved fork. [R17]

**Demote this class as a primary FPS lever.** Task 55 is strong cautionary evidence. Retain it only when storage is valuable or a current fighter-specific measurement proves transfer cost is limiting. [R6]

### Class B: fewer expensive operations at equivalent output

Potentially different mechanisms include fewer actual submitted vertices using already-supported primitive forms, fewer unnecessary `BEGIN` barriers, fewer matrix restores/loads when their state is truly redundant, and safely fewer repeated lighting evaluations.

Primitive grouping already exists in the production path. “Use strips” is not a new task. A proposal must count a remaining opportunity in the **current emitted streams**, explain why the generator did not already exploit it, and preserve source triangle coverage and rendering order. [R7]

Treat the emulator's pipeline behavior as a reason to inspect command order: its code models `BEGIN` as a pipeline-stalling operation and `NORMAL` as a lighting computation. It does not justify assigning universal isolated-command timings to the full stream. [R17]

### Correctness boundaries

A repeated `NORMAL` word is not a redundant lighting result when vector matrices, lights, material state, or relevant texgen state changed. A repeated texture coordinate command may have transformation-state effects. Primitive reuse must preserve the complete transformed vertex attributes, not position alone.

Do not freely reorder transparent triangles, merge non-coplanar triangles into quads under an “exact” label, or use degenerate connectors without proving their hardware consequences. Preserve polygon IDs, winding, clipping behavior, depth rules, and per-primitive attribute latching.

Establish explicit packet entry and exit state across frames and writers. The Task 55 visual follow-up is a direct warning against a state optimizer that is correct only in a freshly reset single-frame test. [R6]

### Promotion criterion

Require a current host inventory proving fewer expensive operations or an identified transfer bottleneck; then require a native target A/B. Track actual vertex submissions, not just `VTX_16` opcode count—a rename to another vertex opcode is not a deleted transform.

No percentage reduction in packet words should be translated directly into a percentage FTR or whole-frame improvement.

---

## 10. Candidate FTR-R6 — producer-owned pose/matrix output, not another reuse cache

**Priority:** deferred architectural research, not the next implementation batch.

The original recommendation was to eliminate representation round trips and redundant matrix serialization. That remains a meaningful design objective, but the later reported adverse pilot is enough to reject an unqualified “start here” instruction. [R4]

### What would make a genuinely different reopening

A new proposal must identify the precise producer, the final consumers, and the old work it deletes. It should produce a render-only output directly in the representation the packet needs, rather than adding a side cache that checks source objects, misses, falls back, and leaves the original pipeline intact.

Before a ROM build, demonstrate that the target producer covers the intended draws without a steady-state miss/stale path. Retrieve and explain the rejected pilot first. A matching idea name or a shared function is not evidence that two implementations are identical, but neither is a different name evidence that they are different.

### Preserve separate correctness domains

Gameplay joint and attachment data cannot be removed because a visual mesh does not draw those joints. The board reports that a joint-cap experiment aborted AI and that a large pose-evaluation deletion did not reduce WORK-H. This rules out a blind cap, not the general possibility of a separate renderer-owned representation. [R3]

Current matrix code has source-precision and animation-lock paths in addition to ordinary native composition. Those consumers and accumulated-scale semantics have to be mapped before bypassing source data. Preserve their arithmetic order initially. [R10]

Do not assume body pose holding is new savings: it is already part of the implementation. Do not reduce the 60 Hz event/control clock or move source event boundaries as a render optimization. [R2], [R3]

### The 4×3 trap

The current split matrix serialization scales the **complete homogeneous row** by the world-unit shift. Consequently its resulting matrix is not necessarily an ordinary affine matrix with a bottom-right element of 1. Replacing its 4×4 load with a 4×3 load can silently restore the wrong implicit homogeneous value. [R10], [R20]

A redesigned projection/matrix convention might permit a different representation, but that is a full derivation and multi-frame equivalence test—not a four-word serialization shortcut.

### ASM decision

The existing 4×4 copy already has an ARM block-transfer implementation, and the recorded copy experiments were small or adverse. Do not schedule another generic “replace loops with ASM” task. [R4], [R8]

Assembly becomes reasonable only for a surviving fused producer/serializer with a demonstrated instruction/spill bottleneck. Keep a portable reference and compare final words, especially negative values, rounding ties, overflow limits, animation locks, and scale changes.

---

## 11. Why the target architecture should stay small

The useful long-term shape is still:

```text
Source gameplay / event state (60 Hz)
    + required source display-head behavior
    ↓
Bound per-instance native render contract
    ↓
Small shape-specific patch/evaluation kernels
    ↓
One resident DMA-safe packet per instance
    ↓
Ordered native submission
```

Generation, asset validation, and maximum-capacity derivation should do the general work. Runtime should not rediscover topology, build a broad generic request object, or carry maximum data for unused features merely to share one giant executor.

This is **not** a mandate to replace every current subsystem at once. The safe first step is R1 with existing validity and math, followed by small exact consumers such as R2. Only delete the old route when required content has a complete native replacement. [R2]

The DS references already reviewed provide useful contrasts: the vendored `sm64-nds` keeps compact matrix-stack/animation state, while the inspected `sm64ds-decomp` texture binder writes typed hardware parameters directly. These are design examples, not evidence that their complete architectures or performance transfer to this game. Their cited snapshots are the earlier inspected pin, not newly benchmarked programs. [R21], [R22]

### Cache ownership is an architectural issue

The DTCM result supports keeping truly hot scalar state compact, but not relocating every table. Cache lines read per replay, main-memory packet writes, literal-pool loads, and instruction footprint all compete. A faster arithmetic kernel that expands instruction/data traffic can lose overall. The board's reported layout sensitivity makes a matched-control measurement essential. [R5]

A memory-returning change can enable another candidate without itself saving ticks. Record those as separate outcomes. The 8,458 bytes of entry-effect texels identified as unrelated to the restricted four-CPU build are a separate residency opportunity, not an FTR FPS gain. Removing content needed by another shipped configuration is not an acceptable way to claim it. [R3], [R14]

---

## 12. Implementation sequence with explicit stop conditions

### Batch A — stabilize the measurement population and size storage

Use the current qualified baseline and separately tracked local candidates. Verify the requested window against actual logical sample identity. Do not silence the DTCM candidate's assertion solely because its medians look good; preserve the check's same-match purpose and fix identity bookkeeping explicitly. [R5], [R13]

Inventory actual packet lengths and patch-table high-water by instance, owner, detail, and root program. Include legal duplicate-character lineups, CSS HIGH, copied hats, hidden parts, and scene transitions. Establish target `sizeof` and the exact linker/arena layout.

**Deliverable:** a bounded memory plan for R1 and a frame-aligned replay/record cost population. No new broad profiling framework is required if existing counters can be collected correctly.

### Batch B — compact replay storage

Implement R1 alone with unchanged UV/matrix math and existing validity checks. Capture section sizes, arena size, packet capacity, native failures, and equivalent outputs. Verify that header/tail separation does not create hot allocation or loader work.

**Stop:** legal content no longer fits, capacity is only guessed, or the supposed memory saving becomes a duplicate allocation.

### Batch C — exact replay micro-kernels

Add R2's direct local indices. Then test dirty-range cleaning and exact light/tint memoization as independently attributable toggles or patches. Their combined retained version should get a final normal shipping-config comparison.

**Stop:** cost migrates to another bucket, an instruction/data layout regression exceeds the local win, or a parameter can become stale. A tiny same-ROM improvement may be real; do not misapply an old cross-build threshold as a universal minimum. Conversely, do not promote a cross-build delta merely because it exceeds one historical noise number. [R5], [R13]

### Batch D — select one structural follow-up from evidence

If record events explain the relevant tail, implement a bounded R4 variant mechanism for the dominant cause. If a current command trace instead shows expensive redundant geometry operations, pursue R5. Do not implement both speculative systems merely because they appear in this report.

R6 remains blocked on the primary pilot receipt and an explicit deleted-work design. Do not repeat the rejected GX-compose configuration: its recorded four-fighter regression was +22,848 WORK-H P50 and +67,456 P95. [R12]

### Reopening rule

A previously rejected idea requires a concrete invalidator: changed bottleneck, different workload, corrected non-engagement, a materially different representation, or a fixed confound. “Try harder,” a new task ID, or another narrative is not an invalidator.

---

## 13. Qualification: what counts as a win

For each candidate record the source pin, ROM/ELF identity, generated assets/configuration, emulator identity/settings, workload, and sample-window convention. Use the same admitted content and source time; runtime counters need logical sequence IDs where frame labels can collide. [R3], [R13]

Report WORK-H P50/P95, all-presented cadence, the 2/3/4/5+ VBlank histogram, maximum interval, and per-frame deadline misses. Report FTR and neighboring buckets as attribution, not independent speedup claims. Distinguish reducing actual work from merely moving where an asynchronous wait is charged. [R2], [R3], [R5]

Use host-side source/GX comparison and native-only target validation. Profile-level diagnostic builds do not get an exception to the native-only product rule. Do not put a forbidden compatibility renderer in the ROM to perform the A/B. [R2]

Required correctness cases include:

| Area | Cases that must remain correct |
|---|---|
| Source timing | Motion/event boundaries, hitlag, pause, status re-entry, 60 Hz simulation |
| Topology | Hidden parts, grabs/throws, Samus alternate roots, Kirby donor assets |
| Appearance | Costume/shade, hurt flash, material animation, fog, alpha/cutout states |
| Geometry | Root bindings, normal transforms, clipping, winding, transparent order, matrix-slot lifetime |
| Scenes | CSS HIGH, battle detail policy, Results, rematches, arena reuse |
| Resources | Texture eviction/re-upload, packet capacity, DMA lifetime, no new draw-time allocation |
| Completeness | Zero unexpected native failures/direct rejects and no disappearing required content |

Current summary counters sometimes credit recorded logical work on a replay rather than recounting every actual command. If a compiler changes command count, distinguish source-equivalent triangles, actual GX commands, and credited logical statistics. A counter designed for equivalence is not automatically a hardware-throughput counter. [R7], [R8]

The end criterion remains the product gate, not a named optimization being implemented. No single roster/window establishes all landed content's worst case. [R2], [R3]

---

## 14. A small analysis tool for honest budget screening

The following is original host-side analysis code, not a game patch. It uses the sampler's floor-index quantile convention. It analyzes **one** component at a time to avoid accidentally summing nested timers.

`WORK-H − fraction×FTR` is only an optimistic arithmetic scenario, not a predicted speedup or a proof of a physical lower bound. Confirm that the component is included once in work on the same logical row before using the calculation. This script refuses malformed or component-greater-than-work rows rather than hiding them. It does not repair frame labels, timer wraps, or torn records.

```python
#!/usr/bin/env python3
"""Screen one frame-aligned optimization budget; never a runtime benchmark."""
from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path
from typing import Sequence


def quantile(values: Sequence[float], p: float) -> float:
    if not values:
        raise ValueError("No samples")
    ordered = sorted(values)
    return ordered[math.floor((len(ordered) - 1) * p)]


def load_rows(path: Path, work_col: str, lane_col: str) -> tuple[list[float], list[float]]:
    work: list[float] = []
    lane: list[float] = []
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        fields = set(reader.fieldnames or [])
        missing = {work_col, lane_col} - fields
        if missing:
            raise ValueError(f"Missing columns: {sorted(missing)}")
        for row_number, row in enumerate(reader, start=2):
            try:
                w = float(row[work_col])
                f = float(row[lane_col])
            except (TypeError, ValueError) as exc:
                raise ValueError(f"Invalid number on CSV line {row_number}") from exc
            if not (math.isfinite(w) and math.isfinite(f)):
                raise ValueError(f"Non-finite number on CSV line {row_number}")
            if w < 0 or f < 0 or f > w:
                raise ValueError(
                    f"Invalid component/work relationship on line {row_number}: {f}, {w}. "
                    "Check timer nesting, row integrity, and measurement scope."
                )
            work.append(w)
            lane.append(f)
    if not work:
        raise ValueError("CSV contains no data rows")
    return work, lane


def describe(label: str, values: Sequence[float], gate: float) -> None:
    misses = sum(v > gate for v in values)
    print(
        f"{label:24} mean={sum(values)/len(values):12,.1f} "
        f"P50={quantile(values,.50):12,.1f} "
        f"P95={quantile(values,.95):12,.1f} "
        f"over_gate={misses}/{len(values)} ({100*misses/len(values):.2f}%)"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path)
    parser.add_argument("--work", default="WORK-H")
    parser.add_argument("--lane", default="FTR")
    parser.add_argument("--gate", type=float, default=1_120_000)
    parser.add_argument("--fraction", type=float, default=.50)
    args = parser.parse_args()
    if not math.isfinite(args.gate) or args.gate <= 0:
        parser.error("--gate must be finite and positive")
    if not math.isfinite(args.fraction) or not 0 <= args.fraction <= 1:
        parser.error("--fraction must lie in [0, 1]")
    try:
        work, lane = load_rows(args.csv, args.work, args.lane)
    except (OSError, ValueError) as exc:
        parser.exit(2, f"error: {exc}\n")

    scenario = [w - args.fraction*f for w, f in zip(work, lane)]
    deletion = [w - f for w, f in zip(work, lane)]
    describe("Observed work", work, args.gate)
    describe("Arithmetic partial cut", scenario, args.gate)
    describe("Arithmetic full deletion", deletion, args.gate)

    # A conditional description on whole-frame tail rows, not a sum of percentiles.
    threshold = quantile(work, .95)
    selected = [f for w, f in zip(work, lane) if w >= threshold]
    print(f"Mean {args.lane} on whole-frame P95-or-worse rows: "
          f"{sum(selected)/len(selected):,.1f}")
    print("Arithmetic scenarios only: downstream stalls, cache changes, and "
          "overlap are not modeled. Do not bank these values as savings.")


if __name__ == "__main__":
    main()
```

Example after saving the code as `analyze_ftr_budget.py`:

```sh
python analyze_ftr_budget.py matched_rows.csv --lane FTR --fraction 0.50
```

This script must consume a validated CSV. It is not an alternative to the project's qualification harness.

---

## 15. Work actually performed for this re-evaluation

**Performed:** live branch/pin verification; targeted current-source inspection; retrieval and comparison of current measurements and negative evidence; primary implementation checks for DMA/GX behavior; arithmetic/layout checks; and 2,000 synthetic tests of the texgen dense-ID-to-local-index transformation.

The synthetic tests preserve the exact assigned per-unique-input 32-bit value at every output site. They do **not** validate the game's UV arithmetic, its source assets, target code generation, or rendering.

**Not performed:** target compilation, new ROM execution, approved-fork profiling, captured-frame comparison, live gameplay differential, or new Boundary qualification. No new FPS/tick improvement is claimed.

**Evidence gap:** the pose/draw pilot named by the September 17 sizing summary was not retrievable at its stated repository path. Its reported regression is retained as adverse evidence, with provenance explicitly limited to that summary.

---

## 16. Coding-agent handoff

> Work from the current repository tip, first reconciling it with this report's pinned revision. Preserve 60 Hz simulation, source gameplay, required content, native-only rendering, and qualification gates. Begin with the packet header/cold-tail inventory in FTR-R1; derive capacities over required states before allocating. Do not reimplement existing prechecked replay, asynchronous DMA, tint patching, or texture-use accounting. Keep existing numerical producers and validity checks for the first layout comparison. Add direct-index texgen without changing arithmetic. Measure matched whole-frame work and cadence, not just FTR or packet size. Root-local variants require frame-correlated miss evidence; the broad pose lane requires the original rejected pilot receipt and a materially different deleted-work design. Reuse existing evidence, stop rejected mechanisms, and record qualified improvements separately from memory enablement and unmeasured hypotheses.

## Final assessment

**The revised recommendation is a compact, shape-specific, memory-conscious replay path—not a blanket rewrite and not another cache layered on top.** The 4,992-byte texgen reservation provides a concrete place to start sizing memory return. Direct-index texgen and narrower cache maintenance are plausible exact supporting cuts. Variant and GX-operation work should be selected by the actual expensive-frame population.

The 30-FPS objective is not satisfied by this report. It is a research target with specific remaining implementation candidates and honest tests. The current evidence justifies continuing structural optimization, but not inventing a combined speedup, subtracting unrelated medians, or treating one failed implementation as proof that every equivalent representation is exhausted.

---

## Source register

Repository links use the reviewed commit unless explicitly identified as an earlier inspected reference. The two external implementation references are supporting sources, not substitute measurements of the project's canonical emulator.

| Ref. | Evidence |
|---|---|
| R1 | [Branch collection; also compared db0d088 against master][R1] |
| R2 | [Product, native-rendering, performance and fidelity contract][R2] |
| R3 | [Current qualified checkpoint, owner decisions, and resource constraints][R3] |
| R4 | [New critique/sizing of the prior FTR/STG/MISC reports][R4] |
| R5 | [DTCM candidate, measurements, sample-window failure and corroboration][R5] |
| R6 | [Task 55 stage elision; includes later owner visual contradiction][R6] |
| R7 | [Packet key, texgen, precheck, replay, flush, DMA and invalidation; especially around 8674–9590][R7] |
| R8 | [Packet layouts, capacities, arena, recorder and DMA synchronization; around 3260–3650][R8] |
| R9 | [Display head, draw-contract memo, invalidation and native adapter][R9] |
| R10 | [Ordinary/source-precision/animation-lock matrix paths and GX-route selection][R10] |
| R11 | [Prechecked replay already precedes whole-owner preflight][R11] |
| R12 | [Previously retrieved four-fighter GX-compose regression; explicitly earlier pin][R12] |
| R13 | [Per-row WORK-H and floor-index quantiles; around 1180–1320][R13] |
| R14 | [Vulcan Jab experiment and arena page-edge consequence][R14] |
| R15 | [Primary libnds glCallList implementation, checked September 17, 2026][R15] |
| R16 | [Primary libnds DMA/cache documentation, checked September 17, 2026][R16] |
| R17 | [Primary upstream implementation: vertex forms, NORMAL, BEGIN and pipeline interactions; checked September 17, 2026][R17] |
| R18 | [Earlier retrieved exclusive-symbol profile; not a current candidate speedup][R18] |
| R19 | [Measurement history and warning about nested timers][R19] |
| R20 | [Earlier inspected split-matrix homogeneous-row scaling, around 12100–12580][R20] |
| R21 | [Earlier inspected vendored DS reference: matrix stack and animation state][R21] |
| R22 | [Earlier inspected vendored DS reference: typed texture hardware binding][R22] |


[R1]: https://api.github.com/repos/rockenrooster/Smash64DS_Port/branches?per_page=100 "Branch collection; also compared db0d088 against master"
[R2]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/PROJECT_GOAL.md "Product, native-rendering, performance and fidelity contract"
[R3]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/docs/P2_EXECUTION_BOARD.md "Current qualified checkpoint, owner decisions, and resource constraints"
[R4]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-17_p2-2p8-ftr-stg-misc-sizing/README.md "New critique/sizing of the prior FTR/STG/MISC reports"
[R5]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-17_p2-2p8-dtcm-hot-scalars/README.md "DTCM candidate, measurements, sample-window failure and corroboration"
[R6]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-07-24_task55-stage-geom-e2.md "Task 55 stage elision; includes later owner visual contradiction"
[R7]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/nds/nds_renderer_native_common.c "Packet key, texgen, precheck, replay, flush, DMA and invalidation; especially around 8674–9590"
[R8]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/nds/nds_renderer_preamble.c "Packet layouts, capacities, arena, recorder and DMA synchronization; around 3260–3650"
[R9]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/port/renderer_adapter_fighter.c "Display head, draw-contract memo, invalidation and native adapter"
[R10]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/port/renderer_adapter_matrix.c "Ordinary/source-precision/animation-lock matrix paths and GX-route selection"
[R11]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/nds/nds_renderer_native_fighter_production.c "Prechecked replay already precedes whole-owner preflight"
[R12]: https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/artifacts/performance/2026-09-16_p2-2p8-gx-compose-decline/README.md "Previously retrieved four-fighter GX-compose regression; explicitly earlier pin"
[R13]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/scripts/sample-tick-hud-buckets.ps1 "Per-row WORK-H and floor-index quantiles; around 1180–1320"
[R14]: https://github.com/rockenrooster/Smash64DS_Port/commit/db0d088bc61ac3e85f07a349857a1b3ec7eef55b "Vulcan Jab experiment and arena page-edge consequence"
[R15]: https://github.com/devkitPro/libnds/blob/master/include/nds/arm9/videoGL.h "Primary libnds glCallList implementation, checked September 17, 2026"
[R16]: https://github.com/devkitPro/libnds/blob/master/include/nds/dma.h "Primary libnds DMA/cache documentation, checked September 17, 2026"
[R17]: https://raw.githubusercontent.com/melonDS-emu/melonDS/master/src/GPU3D.cpp "Primary upstream implementation: vertex forms, NORMAL, BEGIN and pipeline interactions; checked September 17, 2026"
[R18]: https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/artifacts/performance/2026-09-16_p2-2p8-n0409-profile/census.txt "Earlier retrieved exclusive-symbol profile; not a current candidate speedup"
[R19]: https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/docs/PERF_LEDGER.md "Measurement history and warning about nested timers"
[R20]: https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/src/nds/nds_renderer_textures_effects.c "Earlier inspected split-matrix homogeneous-row scaling, around 12100–12580"
[R21]: https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/decomp/sm64-nds/src/game/rendering_graph_node.c "Earlier inspected vendored DS reference: matrix stack and animation state"
[R22]: https://github.com/rockenrooster/Smash64DS_Port/blob/430aca2879e9071dc2b22f944f5c2909c9ce7aa4/decomp/sm64ds-decomp/src/func_0204af3c.c "Earlier inspected vendored DS reference: typed texture hardware binding"
