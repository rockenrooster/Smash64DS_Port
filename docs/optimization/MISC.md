# Smash64DS: MISC optimization re-evaluation

**Date:** September 17, 2026  
**Repository:** `rockenrooster/Smash64DS_Port`  
**Reviewed branch/snapshot:** `master` at `db0d088bc61ac3e85f07a349857a1b3ec7eef55b`  
**Earlier review snapshot:** `430aca2879e9071dc2b22f944f5c2909c9ce7aa4`  
**Deliverable:** revised research and implementation candidates, not a measured optimization patch.  
**Final branch check:** `master` still resolved to the reviewed SHA when this report was completed.

## 1. Revised conclusion

**Keep the architectural direction, but change the priorities and tighten the claims.** The strongest direction is a native renderer that separates resource preparation from drawing, retains immutable native bindings, caches genuinely shared transform results, and submits small, specialized draw records in the required order. It is not a wholesale replacement of gameplay objects or another runtime interpreter.

My earlier response correctly identified several real repeated operations. It did not establish that they account for most of MISC, or that packetization would produce a large whole-frame win. Those distinctions matter. There is still no measured combination of the new candidates in this report demonstrating the 30 FPS gate.

The revised order is:

| Priority | Stable candidate ID | Decision and next bounded implementation |
|---|---|---|
| First | **M3a — DamageSlash preconversion** | Remove the existing per-texel conversion from drawing. Compare against the current native implementation without simultaneously changing primitive emission. |
| First, resource-dependent | **M3b — immutable DamageSlash residency** | Eliminate frame-change image uploads after proving the texture-memory allocation and sampling contract. Evaluate the smaller hardware-mirror representation below. |
| Newly identified | **M7 — prepared KO palette variants** | Remove palette construction/allocation from ordinary drawing; protect resource generations while queued draws still reference them. Measure its actual burst cost before broadening the implementation. |
| Main structural pilot | **M2 — specialized native effect/weapon submission** | Eliminate intermediate traversal/material reconstruction for one measured owner. Charge Shot is a simple correctness pilot, not a proven dominant cost. |
| Small independent experiment | **M1 — shared transform invariants** | Cache signed affine-axis lengths, then optionally camera-relative basis vectors. Do not duplicate the affine matrix already cached by the source. |
| Supporting change | **M5 — prebound frame/bank/basis descriptors** | Preserve existing frame-selection semantics while removing repeated interpretation and conversion around the already-fixed emitter. |
| Conditional extension | **M4 — ordered packet consolidation** | Proceed only if nonempty cross-path flushes or repeated state transitions have material measured cost. Fewer GX words alone are not the objective. |
| Supporting change | **M6 — hot/cold state, counters, small kernels** | Preserve the existing locality improvements; reduce working-set traffic before trying small C/ARM kernels. Do not count banked work as a new saving. |

This ranking reflects the specificity of the removable work, implementation scope, and correctness/resource risk. It is **not a ranking of measured speedups**.

### Binding constraints

The goal remains stable 30 FPS with four fighters and native rendering in every built ROM. The current owner ruling retains 60 Hz simulation. The September 17 board reopens SRC work, but does not reopen 30 Hz simulation. This MISC proposal does not change gameplay update rates, RNG order, particle creation order, hitboxes, or CPU behavior. Required content cannot silently disappear on a resource or binding failure. [S1][S2]

No runtime source, build configuration, repository document, or published ROM was modified in this review. I inspected code and recorded evidence, checked supporting hardware/library behavior, and ran the small arithmetic/address-model checks in section 12. I did not build an NDS ROM, inspect a newly linked ELF, run the project's emulator, or generate new gameplay timings.

## 2. Replace the old baseline and repair the sizing logic

### 2.1 Latest qualified checkpoint found in the reviewed snapshot

The board identifies `a4eb24c9a85` as its latest qualified three-arm Boundary checkpoint. The associated sizing artifact reports the following four-fighter distribution. Boundary correctness being green is distinct from the product performance target, which remains red. These are recorded measurements from that checkpoint, **not measurements of every later commit up to the review SHA**. [S2][S3]

| Metric | P50 ticks | P95 ticks |
|---|---:|---:|
| WORK-H | 1,600,960 | 2,320,576 |
| MISC | 238,720 | 464,000 |
| FTR | 350,144 | 736,960 |
| STG | 385,088 | 427,648 |
| SRC | 543,040 | 1,027,520 |

Using the artifact's rounded operational gate of 1,120,000 ticks, the whole-frame gaps are **480,960 at P50** and **1,200,576 at P95**. These correspond to reductions of approximately **30.04%** and **51.74%** of the respective whole-frame costs. MISC's P95 is about **41.43% of the target frame budget**, which makes it important, but does not establish what fraction is recoverable.

The executable verifier may use the more precise nominal budget rather than this rounded value. An implementation must use that verifier's actual gate consistently; do not change the acceptance threshold to match a document's rounding.

### 2.2 Separate later experimental evidence

A subsequent DTCM experiment reports a different control/candidate pair: [S4]

| Metric | Its control | Its DTCM candidate | Difference |
|---|---:|---:|---:|
| WORK-H P50 | 1,580,544 | 1,537,344 | -43,200 |
| WORK-H P95 | 2,320,768 | 2,277,696 | -43,072 |
| MISC P50 | 249,856 | 238,144 | -11,712 |

That is useful evidence that locality matters across subsystems. It is not a new MISC optimization proposed here, and its control is not the table in section 2.1. The report also records a sample-window/first-label assertion failure. Do not describe that experiment as fully qualified merely because its native-render and selected workload witnesses match. Repair or explain iteration identity in the collector, then rerun the required acceptance checks; do not simply relax a failing assertion.

### 2.3 The new repository sizing artifact makes two invalid inferences

The September 17 FTR/STG/MISC sizing document argues that its lane percentages sum above ALL, therefore the lanes overlap; it also subtracts individual lane medians from the whole-frame median to declare deletion ceilings. Neither inference follows from those summary statistics. [S3]

**Separate percentiles are not additive.** Consider three frames whose disjoint components are:

| Frame | A | B | Total A+B |
|---|---:|---:|---:|
| 1 | 100 | 0 | 100 |
| 2 | 0 | 100 | 100 |
| 3 | 100 | 100 | 200 |
| Median | 100 | 100 | 100 |

The medians add to 200 while the median total is 100. Nothing overlaps. Furthermore, the artifact's displayed collection includes WAIT, which is already represented within OTHR; that intentionally nested quantity must not be added as another top-level owner.

**Subtracting medians does not compute the median after deletion.** For `W = [100, 200, 200]` and `M = [0, 100, 0]`, `median(W) - median(M)` is 200, while `median(W - M)` is 100.

Therefore, **238,720 / 480,960 = 49.6% is a ratio of two summaries, not a proven ceiling on how much deleting MISC could change the median**. The same objection applies to P95 and the other buckets. This does not prove that MISC can close the gate; it invalidates that particular proof that it cannot.

For an optimistic fixed-workload counterfactual, use aligned per-iteration data:

```text
remaining[i] = work_minus_hud[i] - removable_misc_work[i]
result       = percentile(remaining, 95)
```

Use exclusive removable costs, not overlapping call-tree totals. Re-rank all rows after the subtraction. Then distinguish this bookkeeping experiment from a real implementation: CPU work removed during a GPU-limited interval may be replaced by waiting, whereas reduced cache pressure may also help other work. Actual A/B timing remains necessary.

### 2.4 What MISC actually means

The current code in `ndsBattlePlayableFinalizePresentedIteration()` constructs: [S5]

```text
MISC = max(DRAW - FTR - STG - BG - FOREGROUND, 0) + FLUSH
HUD  = FOREGROUND + DEBUG_HUD_SPAN
OTHR = max(ALL - sum(top_level_named_buckets), 0)
WORK = max(ALL - WAIT, 0)
```

Its intended accounting identity is:

```text
WORK-H = FTR + STG + BG + AUD + SRC + MISC + (OTHR - WAIT)
```

Validate that identity on individual rows and count any clamping/conservation anomaly. The formula is an accounting construction, not proof that every nested instrument is perfect.

MISC is a **draw residual plus flush**, not a synonym for particles, effects, or unclassified simulation. The source particle-update work normally belongs to SRC. There are port-specific exceptions to a perfectly clean update/draw split, such as FireGrind's update in the draw-side particle seam; inspect actual placement before assigning a saving. [S5][S6]

Likewise, `ndsFighterDisplayContractSubmit()` brackets contract capture and fighter submission inside FTR. Their presence in a whole-program profile does not make them MISC candidates. [S7]

**A remaining attribution gap:** the inspected evidence does not provide a current, complete, exclusive partition of MISC into particle arithmetic, effects, weapons, resource preparation, adapter work, and inline waits. Optimize the concrete deletions below, but do not assume those named families explain the entire 464,000-tick P95.

## 3. M3 — DamageSlash: remove conversion and mutable texture residency

### 3.1 Confirmed current work

`src/nds/nds_native_damage_slash.exec.inc` still does the following: [S8]

- `ndsDamageSlashTextureFill()` clears scratch memory, iterates output texels, mirrors/clamps source coordinates, reads packed nibbles, and repacks the image.
- `ndsDamageSlashEnsureTexture()` keeps two texture names, one per drawable child. A different requested source frame causes preparation/upload into that child's texture.
- The submit path calls this resource preparation during drawing.

A historical September 16 profile records **3,079,828 cycles of TextureFill self-time**. Under that report's `cycles / (2 × 129 regions)` normalization, this is approximately **11,937 ticks per reported region**. It establishes executed, nontrivial work; it is not a new measurement, a full inclusive DamageSlash cost, or a current P95 saving. [S10]

This concrete evidence makes DamageSlash a better first target than assuming that shared particle square roots are the largest remaining issue.

### 3.2 M3a: prepared pixels, unchanged residency

Generate final DS-format payloads offline, or convert the complete finite set once during scene preparation. Initially retain the current two texture allocations and existing native draw topology.

The first experiment should delete only conversion work. Texture uploads remain, so its acceptance report must not claim that it solved streaming or synchronization.

The generated asset contains eight 32×64 converted 4-bit frames and five 32×32 converted 4-bit frames. Keeping all prepared payloads in ordinary RAM costs **10,752 bytes**, before metadata, unless they replace other resident data or can be loaded through a separately budgeted resource mechanism. [S9]

Do not add a per-draw file read to avoid that RAM cost. Do not assume a generated `const` array is free, directly accessed cartridge storage; verify its linked/load address and resident footprint. Preparation time and ROM size are legitimate trades, but working-set and arena costs still count.

**Proof:** compare all 13 prepared outputs byte-for-byte with the old native converter for the admitted source layout. Preserve nibble order, mirrored rows, edge replication, palette interpretation, and source-frame mapping. Measure conversion calls and uploaded bytes separately. Expected mechanism: conversion calls become zero during gameplay; uploads do not yet become zero.

### 3.3 M3b: immutable complete texture set

Prepare every required DamageSlash frame before drawing and switch immutable bindings as animation advances. Multiple instances at different ages can then reference different images without rewriting a shared child slot.

The exact payload arithmetic from the generated shape table is: [S9]

| Representation | Texture payload | Additional payload versus current 1,536-byte pair |
|---|---:|---:|
| Current two streaming allocations | 1,536 B | — |
| All 13 expanded frames resident | 10,752 B | 9,216 B |
| Proposed hardware-mirrored complete set | 6,656 B | 5,120 B |

These figures exclude allocation metadata, alignment, palette allocations, and optional CPU staging copies. They are not a claim that the allocator has sufficient contiguous space.

### 3.4 New alternative: use hardware S mirroring

For the first eight frames, the converter expands a 16×48 source into a 32×64 upload by reflecting X and repeating the bottom row. A candidate representation stores **16×64** instead, retaining the vertical padding and using the DS texture's S-wrap/S-flip behavior to provide the mirrored half. The remaining five frames stay 32×32:

```text
8 × (16 × 64 / 2) + 5 × (32 × 32 / 2) = 6,656 bytes
```

This removes **4,096 bytes** from the expanded complete set. The project already uses an analogous native texture-mirroring approach for the Fox blaster glow, so the mechanism is not foreign to this renderer. [S11][S16]

**This compact variant is not yet proved lossless.** The integer coordinate model matches within X=0..31, but the real test must include interpolated texture coordinates, negative/overshooting coordinates, exact boundary values, logical versus physical texture width, texture offsets, and clamp behavior outside the intended domain. Preserve the original effective tile/UV contract; simply halving the upload-width field is insufficient.

Prefer individual native texture regions or another representation that preserves wrap semantics. An atlas cell does not automatically mirror at its own boundaries.

### 3.5 Important correction: “fenced” does not mean rendering has retired

`ndsRendererHardwareFencedGlTexImage2D()` records a diagnostic fence class, invalidates cached texture-parameter state, and calls `glTexImage2D()`. The wrapper itself does **not** wait for queued texture consumers to finish. [S12]

Two child slots shared by multiple instances can request alternating image frames. That creates a potential mutation/lifetime problem as well as repeated conversion. It is an **inference requiring an instance/frame/address trace**, not a confirmed visual bug from this review.

Use these distinct lifetimes:

```text
Command buffer: reusable after its DMA/CPU submission consumer finishes.
Texture/palette storage: reusable after the rendering consumers finish.
```

A finished FIFO DMA, `glEnd`, or a wrapper named “fenced” does not establish the second property. Plan uploads at a platform-defined safe boundary with verified bank ownership and resource retirement. The DS resource mapping rules also make changing texture/palette bank mappings during use unsafe. [S15][S17]

**Falsifiers:** the extra residency displaces required resources; the compact mirror changes sampling; or the removed conversion/upload work produces no meaningful whole-frame improvement after accounting for cache and GPU waits. In those cases retain only the independently demonstrated useful subchange.

## 4. M7 — Newly identified: compile and prebind KO palette variants

### 4.1 What the first review missed

In `nds_renderer_textures_effects.c`, `ndsRendererParticleEnvVariant()` uses an eight-entry round-robin cache keyed by sheet, primitive color, and environment color. Alpha is deliberately separate. On a miss it constructs 32 palette entries, may create palette-only names, and calls `glColorTableEXT()` for a 64-byte palette. It also retains a base palette name so later ordinary particles can restore the sheet's original palette. [S11]

This reproduces the source's environment-color blend rather than reducing it to a single vertex tint. Removing ENV would change the KO presentation; that is not the proposed optimization.

The important correction is that a miss is not necessarily “just copy 64 bytes.” In the inspected **libnds v2.0.2** implementation, `glColorTableEXT()` detaches the old palette and creates a palette allocation; associated reference-count and allocator work is involved. Verify the actual linked SDK before relying on these implementation details. [S15]

### 4.2 Proposed replacement

For source scripts with a provably finite, deterministic palette sequence, generate the exact reachable palette states and their key-to-binding mapping. Include source quantization and interpolation semantics, not merely colors observed in one test window.

At scene preparation, admit the relevant palette closure and keep it immutable. During drawing, select a prepared texture/palette binding rather than constructing colors and changing library resource ownership.

For genuinely dynamic inputs that do not admit a small complete closure, use a bounded resource-preparation phase: collect the frame's unique keys, compute each required palette once, allocate only from a pre-established pool, and pin every generation until its consumers retire. Do not introduce a runtime filesystem dependency.

The existing eight variants consume **512 bytes of palette payload**, plus base palettes for the sheets that need them and allocation/handle overhead. That is a baseline footprint, not evidence that eight entries cover all four-player overlap. Size from the maximum simultaneously live resource closure, including duplicate fighters and concurrent KO effects. [S11]

### 4.3 Why resource lifetime belongs in this candidate

`glAssignColorTable()` tracks software references between texture names and palettes. Those reference counts are not an independent record of every in-flight rendering use. A cache miss may allocate a new address or later recycle an old one; do not assume that replacement is always safe, or that it always overwrites the same physical address. [S15]

This is especially important before M4: a longer queued command stream must not retain palette addresses that the resource cache can recycle midway through preparing that stream.

**Measure:** unique palette keys per frame and in-flight generation, cache misses, palette allocations, uploaded bytes, binding changes, and exclusive palette-preparation time. Compare both ordinary active frames and KO-heavy tails. No current tick-saving estimate is justified by the inspected evidence.

**Falsifiers:** misses are too rare to matter; the exact finite closure is too large; or prebinding increases working-set cost more than it removes. Keep the correctness/lifetime fix separate from the speed claim.

## 5. M2 — Specialize complete native submission, not just the command encoding

### 5.1 Confirmed opportunity

`ndsRendererSubmitNativeSamusChargeShot()` reconstructs input vertices, initializes traversal state, executes generated setup mutations, obtains texture/material decisions through shared helpers, converts each corner, and restores state. Its source identifies a fixed graphics program with live transform matrices. It is already native; there is no N64 display-list scan to claim as a deletion. [S13]

DamageSlash has a similar intermediate-state path, with additional live material fields. Its generated setup still mutates source-style tile/combine/mode records before the executor derives native state. [S8][S9]

Replace repeated interpretation of those fixed relationships with a specialized native submission contract:

```text
prepared geometry + prepared binding + current dynamic inputs
    → required state changes + unchanged primitive topology
```

### 5.2 The contract must include more than its introductory comment

Before baking a field, trace what each helper reads. Potential dependencies include inherited render state, material colors, alpha, texture origin/filter offsets, dynamic images, palette generation, painter depth, camera normalization, and renderer-global defaults. A comment saying “only matrices are live” is a useful lead, not a substitute for that dependency check.

The native owner must establish the right incoming state **and leave the right outgoing state** for subsequent owners. Bake immutable fields; patch genuinely live ones; preserve logical shadow state or explicitly invalidate the subset a subsequent consumer will re-establish.

Start by preserving the two source triangles and six submitted corners of the Charge Shot program. Do not turn a CPU-state experiment into a simultaneous primitive-topology/fidelity experiment. [S13]

### 5.3 Minimal implementation

Use generated C with small shared native kernels and immutable packed geometry. For a tiny owner, direct stores can be the first implementation; a DMA packet per quad is not a requirement.

Prepare texture handles and native descriptors at the existing residency boundary. Retain complete native implementations for all required admitted states; unsupported content must not quietly vanish. Avoid a generic packet scanner, runtime graphics bytecode VM, or a second compatibility-rendering path.

Inspect the linked binary when pricing the candidate. C source reconstruction loops can already be partially constant-folded. Count the actual loads, clears, calls, and arithmetic that disappear rather than assuming the source-level statement count is the cost.

### 5.4 What prior failed experiments establish

The September 17 sizing artifact reports earlier stage packet compression with negligible frame benefit and a failed fighter pose-to-packet experiment. Those are relevant warnings against repeating their mechanisms. They do **not** measure this particular MISC owner-state deletion. [S3]

A fair new hypothesis is “remove these executed CPU conversions and state reconstructions on these effect owners,” not “GX packets are universally faster.” Measure CPU work and inline GPU stalls separately, then measure the whole frame.

**Pilot choice:** Charge Shot is a small, understandable program. Choose a different first performance owner when actual MISC attribution shows that Charge Shot is rarely present. A clean prototype with low engagement is an architecture test, not a campaign-sized speedup.

## 6. M1 — Cache particle transform invariants, with a narrower first version

`ndsParticleTransformForDraw()` already caches the affine matrix through the source transform state. Its generic transformed-particle path still calculates two affine-axis lengths using square roots and scales the camera right/up vectors for every particle. The position transform changes per particle; the axis lengths do not change while the affine transform is unchanged. Whispy already has specialized routes that avoid some of this work. [S6]

**First implementation:** cache only the two signed lengths for the current transform/pass. That avoids copying another affine matrix into a new cache. Add the camera-scaled basis only when its reuse and storage economics are favorable.

If N particles use K distinct applicable transforms in one camera pass, the norm evaluations can change from `2N` to `2K`. The useful saving is approximately:

```text
2 × (N-K) × cost_of_norm_evaluation
    - cache_lookup_and_maintenance
```

This is a mechanism model, not a speed prediction. It applies only to particles that actually enter the generic non-null-transform path and share reusable transforms.

Preserve the existing sign rule based on affine diagonal entries. Replacing the lengths with `xf->scale.x/y` without a proof can change rotated or negatively scaled results. Compute the same values first; change numerical representation only in a separate experiment.

Use a pass-local cache or an explicit full-width generation scheme with correct object lifetime. The existing `dLBParticleCurrentTransformID` is a one-byte source field; its eventual wrap cannot make an old pointer-keyed entry valid forever. The DTCM report confirms the field's one-byte representation. [S4][S6]

Cache basis vectors by transform **and camera version**. Preserve Ready/Finished transform semantics, same-pass identity, scene resets, and slot reuse. A cache-capacity miss must use a correct native calculation, not drop the particle.

**Measure:** generic transformed particles, distinct transforms, norm evaluations, cache hits/misses, and owner-local time. Keep simulation/RNG unchanged. This remains a good bounded candidate, but the earlier recommendation that it should automatically be first was insufficiently supported.

## 7. M5 and M4 — Make the surrounding hot loop smaller, then consolidate submission

### 7.1 M5: preserve semantic lookup behavior

`ndsParticleQuadFrameFor()` already uses a first-row index for each texture, then searches only that texture's retained frames. It is not a full-atlas scan. It supports nearest-earlier-frame selection for decimated animations. [S6]

A generated direct mapping can replace the remaining search where the reachable domain is small. It must preserve exact matches, earlier-frame holds, below-first-frame failures, source frame interpretation, and canonical bank identity. Use compact reachable tables rather than a huge sparse bank×texture×frame array.

Bind the bank/texture descriptor when the bank or instance is admitted. Keep mutable binding identity separate from immutable frame data and include scene/residency generation in the former. Raw runtime bank-slot numbers can be reused, so they are not permanent asset identities.

### 7.2 M5: avoid repeated basis conversions

The ordinary emitter is already fixed-point internally. It converts center, size, and six right/up components at its boundary. A prepared camera/transform context can retain the six basis values in the exact format that emitter consumes, avoiding repeated conversion for identical bases. [S14]

Do not describe this as newly converting the generic emitter to fixed point. That work has already landed. Keep the current coordinate-range selection and rounding behavior, especially for distant KO effects and large transforms.

An additional small candidate is moving the **exact alpha==0** rejection before expensive draw preparation. Currently the generic submitter performs its fixed conversions before that check. Alpha values 1..7 remain visible through a promoted hardware-alpha value of 1, so testing `alpha >> 3 == 0` would be wrong. Preserve any observed bookkeeping and required transform-state side effects when moving the test. [S14]

### 7.3 M4: only consolidate meaningful submission boundaries

The generic particle path already batches and tracks texture/alpha state. It flushes the Whispy packet at its entry, but a flush called on an empty packet performs no transfer. Source link ordering may naturally cluster the families. Counting flush function calls alone would exaggerate the opportunity. [S14]

Measure **nonempty flushes**, words per flush, submission transitions, state changes, and wait time. Extend a common ordered native command buffer only when those costs are material.

Preserve original order rather than globally sorting translucent particles by texture. A type change need not force a transfer when both types can append correct state transitions to the same stream. A camera/stack/scale transition, resource-preparation boundary, capacity limit, or incompatible immediate owner may still require an explicit boundary.

The packet implementation must understand texture **and palette** binding identity, alpha, camera matrices, dynamic range scale, and all owner state it relies on. Fixing M7's resource preparation/lifetimes comes before queuing pointers to its changing palettes.

### 7.4 DMA rules

Keep DMA packet storage in aligned, DMA-visible memory. CPU-only control records may use DTCM; the DMA source cannot. ITCM/DTCM are inaccessible to DMA. Cache maintenance and ownership transfer are explicit parts of submission, not optional overhead to delete. [S17]

Do not add double buffering by default. First compare modest ordered batches with the current immediate/native paths. Hardware vertex throughput can dominate, and cache-maintenance plus buffer traffic can offset fewer calls. Asynchronous submission is justified only when actual useful overlap survives the whole-frame measurement.

**M4 is now conditional, not a presumed major win.** The useful architecture is compatible with either direct native stores or prepared DMA submission, selected by measured cost.

## 8. M6 — Locality is part of the design; assembly comes after deleted work

The new hot-scalar experiment moves 112 statics totaling 508 bytes into DTCM and reports improvements across MISC, SRC, FTR, and STG. Its instruction-level follow-up also reports lower stalls on the relocated data accesses. This supports keeping the new MISC state compact and avoiding scattered per-particle state traffic. It does not make its already-recorded -43,200-tick result a new item to add to this report's candidates. [S4]

Prefer a small CPU-only hot context for current bindings, camera generation, resource generation, packet cursor, and range state. Keep cold validation metadata, source asset identity, and historical diagnostics out of the per-corner working set. Store immutable records compactly and avoid adding a large cache simply to remove a short indexed lookup.

The experiment reports 1,484 bytes remaining beneath its DTCM assertion ceiling. That is a specific ELF's remaining space, not a standing guarantee that a new cache fits. Check the current map, loaded versus zero-initialized sections, stack reserve, and every required consumer before assigning it.

Aggregate diagnostic counts locally where the semantics permit, then publish at an existing pass boundary. Preserve native failures, capacity high-water marks, resource misses, and acceptance witnesses. Do not remove real lower-screen HUD state as supposed profiler overhead.

`gNdsMiscSplitAccountedTicks` is particularly misleading: the current expression mixes tick counts, byte counts, operation counts, masks, and nested phase values to keep diagnostics referenced. **It is not an exclusive time total**, and must not be subtracted from MISC to calculate a residual. [S5]

After these structural changes, small C/ARM experiments are reasonable for fixed affine position transforms, billboard corner arithmetic, and command packing. Keep a C reference, inspect generated code, test negative rounding and range limits, and check that an integer “optimization” has not introduced expensive division helpers. Move code into ITCM only with an explicit displacement/budget comparison, not by assuming unused capacity.

I would not begin with an ARM7 offload or a whole-renderer assembly rewrite. Neither is required to eliminate the confirmed conversion, palette preparation, and duplicated transform work.

## 9. Recommended runtime architecture

Separate **three kinds of work**, not three new generic frameworks:

```text
BUILD / SCENE PREPARATION
    Convert immutable assets to final DS representations.
    Derive finite material/frame mappings.
    Allocate required texture and palette closure.
    Resolve native bindings and owner contracts.

PER-PRESENT PREPARATION
    Refresh camera and transform-derived data only when needed.
    Select live frames/material keys without changing simulation.
    Prepare any admitted dynamic resource generations at a safe boundary.
    Pin resources until their consumers retire.

ORDERED DRAW
    Visit existing draw order.
    Read a bound native record.
    Compute only the changing per-instance values.
    Emit the required GX state and geometry.
    Do not allocate, convert texture pixels, or interpret source graphics state.
```

Do not add a second large scene graph. Existing source objects can continue to own behavior and ordering. A native draw record can refer to those objects' necessary live fields through a typed binding, without re-deriving a generic rendering representation every frame.

Three specialized kernels are sufficient for the initial scope: billboard particles, fixed-geometry objects, and fixed-geometry objects with animated material inputs. Generated owner-specific code can share those kernels without becoming a runtime graphics interpreter.

Keep immutable geometry separate from changing binding addresses. Texture/palette allocation generations must invalidate only the affected bindings; they should not require reconstructing source vertices and material interpretation that never changed.

### Reference-tree lessons, with limits

The inspected `sm64-nds` renderer separates queued texture requests from a later `glTexSync()` upload loop. That is useful evidence for separating resource operations from geometry emission. Its direct use of library internals and its synchronization assumptions should not be copied without a separate proof for this port. [S18]

The inspected `sm64ds-decomp` `ARMMathSaveState()` saves divider and square-root register state. It is a narrow reminder that hardware arithmetic has shared mutable state; replacing software calculations with device arithmetic requires ownership discipline. It is **not** evidence that SM64DS used the proposed effect-resource architecture. [S19]

The BattleShip-derived particle seam remains the semantic reference for transform use, source frame selection, alpha, bank identity, and ordering. Preserve those results while changing their DS representation. [S6]

## 10. Execution sequence that avoids another repetitive campaign

### Batch A — implement the resource deletion

Implement M3a with one controlled native A/B route. Compare all 13 converted payloads and verify that the old converter actually disappears from the active draw path. Reuse existing source assets and the current native geometry path.

In the same investigation, record requested `(instance, child, source frame, texture address)` transitions. That decides whether M3b primarily removes ordinary animation uploads or also repeated same-frame replacement across instances. Do not spend another cycle inferring that from a single cumulative counter.

Price both complete residency representations before implementing one. Keep main RAM, arena, texture VRAM, palette VRAM, and metadata costs separate. Roster-specific main-RAM savings do not automatically pay a texture-VRAM allocation.

### Batch B — prepare palette resources and specialize one owner

Measure M7 on ordinary and KO-heavy frames, then choose the finite precomputed or bounded prepared-resource variant. Prove its resource-generation lifetime independently of packet batching.

Implement M2 for one engaged owner. Delete concrete intermediate state/vertex work, preserve topology and dynamic inputs, and inspect the linked removal. Broaden the generator only after the pilot improves total work at acceptable memory cost.

### Batch C — small transform and descriptor changes

Try M1's signed-length cache. Stop or narrow it if the relevant generic transform population lacks reuse. Then price M5's basis/descriptor changes using actual lookup/conversion counts.

These are independent of a new packet transport; do not hold them hostage to building M4 first.

### Batch D — measured submission consolidation

Only after resources and state are stable, use the nonempty-flush evidence to decide M4. Compare direct stores with modest batches and account for all waits. Add assembly only for the remaining measured kernels that justify it.

Retain one result record per stable candidate ID: premise, affected workload, old/new representation, byte cost, engagement, paired timing, fidelity result, decision, and the specific condition that could reopen a rejection. An already-fixed operation is not a new candidate under a renamed batch.

## 11. Measurement and acceptance requirements

### Correct attribution

Use the current accurate melonDS configuration with interpreter/JIT settings required by the project's verifier. Freeze the ROM, ELF, generated assets, configuration, workload, and tool versions before comparison. Use the executable verification process as the procedure authority. [S1][S2]

Partition MISC using exclusive scopes around actual current owners and operations. Distinguish resource preparation, draw preparation, geometry submission, packet handoff, and flush. Nested upload/emit timers must not be added to their enclosing owner totals.

A whole-program PC census establishes which code ran. Static reachability does not establish exclusive subsystem ownership. For M2 and M1, associate the execution with the correct draw owner/pass rather than assigning every shared math helper to MISC.

Keep timing instrumentation bounded and separate from per-corner production code. Report instrumentation cost instead of assuming the measured program is identical to shipment.

### Stable row identity

Pair by genuine iteration identity plus workload state, not only a stitched display-frame label. Include scene/match identity, logical tick progression, input/RNG or equivalent simulation witnesses, and draw counters. A faster renderer must not make the sampler compare different logical windows.

Record missing/duplicate labels and ring overrun explicitly. The DTCM experiment's label mismatch is a reason to resolve this machinery, not permission to mix windows or silently discard inconvenient rows. [S4]

### Whole-frame proof

Report each arm's P50/P95/P99, paired mean/median change, number of improved/regressed iterations, all-presented-frame cadence distribution, and relevant content/resource witnesses. Recompute whole-frame percentiles from the candidate rows; do not sum bucket-percentile improvements.

Do not demand that a small deterministic optimization exceed a cross-build noise threshold in isolation when a same-binary test can distinguish it. Conversely, an owner-local saving does not establish a whole-frame benefit if stalls or cache pressure cancel it. Confirm the final retained production build, not only an instrumented selector build.

### Required workloads

The standing four-CPU configuration is the main performance workload, but its particular roster is not the whole product. Cover legal repeated fighter kinds and slot orders, simultaneous effects at different ages, concurrent KO bursts, shields, projectiles, mirrored/nonuniform transforms, distant effects, stage-specific particle banks, palette transitions back to ordinary particles, pause, scene teardown, rematch, and resource-slot reuse.

For immutable residency, admission must cover the required selected content, including dynamically available/copied effects where applicable. An undersized pool that simply refuses required effects is not a successful speed optimization.

### Rejection conditions

Reject or narrow a candidate when the presumed work does not execute, whole-frame improvement disappears under controlled pairing, memory growth causes content loss or cache regressions, the native state contract is incomplete, or a representation changes visible/gameplay behavior outside the accepted contract. Record the specific failed mechanism; do not generalize one failed packet or cache experiment into a proof against every structural redesign.

## 12. Checks actually run during this re-evaluation

I ran local host checks for the following limited claims:

| Check | Result | What it does not establish |
|---|---|---|
| Qualified P50/P95 gap arithmetic | 480,960 / 1,200,576 ticks against 1,120,000 | Any implemented saving |
| Expanded and mirrored texture sizes | 10,752 / 6,656 payload bytes | Allocation fit or visual equivalence |
| Marginal-median counterexamples | Both examples reproduce exactly | Current row-by-row bucket conservation |
| Integer DamageSlash mirror addressing | 2,048 X/Y coordinate comparisons agree within the specified domain | Fractional UV, hardware rasterization, texture-boundary equivalence |

The address check models only the converter's integer source index for X=0..31 and Y=0..63. No source texels, ROM rendering, emulator timings, or screenshots were synthesized or presented as target evidence.

A minimal reproducible form is:

```python
from statistics import median

assert 1_600_960 - 1_120_000 == 480_960
assert 2_320_576 - 1_120_000 == 1_200_576

expanded = 8 * (32 * 64 // 2) + 5 * (32 * 32 // 2)
mirrored = 8 * (16 * 64 // 2) + 5 * (32 * 32 // 2)
assert (expanded, mirrored) == (10_752, 6_656)
assert (expanded - 1_536, mirrored - 1_536) == (9_216, 5_120)

A, B = [100, 0, 100], [0, 100, 100]
assert median(A) + median(B) == 200
assert median([a + b for a, b in zip(A, B)]) == 100
W, M = [100, 200, 200], [0, 100, 0]
assert median(W) - median(M) == 200
assert median([w - m for w, m in zip(W, M)]) == 100

for y in range(64):
    for x in range(32):
        period, column = divmod(x, 16)
        converter_x = column if period % 2 == 0 else 15 - column
        proposed_x = x if x < 16 else 31 - x
        assert converter_x == proposed_x
        assert min(y, 47) == (y if y < 48 else 47)
```

## 13. Final engineering recommendation

**Start with DamageSlash's confirmed conversion/resource work, add the missed KO-palette resource path, and specialize one native effect submission contract.** Keep shared transform caching as a small measured experiment, not the presumed largest remaining win. Treat packet consolidation as conditional on measured submission overhead, and treat locality as a design constraint from the start.

The desired endpoint is straightforward: **prepared immutable resources, compact bindings, shared calculations performed once, and ordered native emission with no asset conversion or generic source-state reconstruction in the hot loop.**

This report does not claim that these candidates alone achieve 30 FPS. It also does not accept the marginal-percentile arithmetic as proof that optimization is exhausted. The path to the product gate must be demonstrated through actual integrated reductions across MISC and the other owners, with unchanged required content and correctly paired whole-frame measurements.

---

## Sources and exact review scope

Repository links below are pinned to the reviewed commit unless an artifact explicitly identifies an earlier measured ROM. A pinned document can contain older evidence: its presence in this snapshot does not make its measurement a measurement of this snapshot.

**[S1] Project contract.** [`PROJECT_GOAL.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/PROJECT_GOAL.md), especially native rendering, preparation, resources, performance, and current milestone requirements.

**[S2] Current queue and owner rulings.** [`docs/P2_EXECUTION_BOARD.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/docs/P2_EXECUTION_BOARD.md), qualified checkpoint and execution cursor. Broad impossibility conclusions in this document are not adopted as proofs.

**[S3] New lane sizing and historical experiment summaries.** [`2026-09-17_p2-2p8-ftr-stg-misc-sizing/README.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-17_p2-2p8-ftr-stg-misc-sizing/README.md). Its recorded checkpoint table is used; its marginal-percentile ceiling/overlap argument is corrected in section 2.

**[S4] Later, separately controlled locality experiment.** [`2026-09-17_p2-2p8-dtcm-hot-scalars/README.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-17_p2-2p8-dtcm-hot-scalars/README.md), including the sample-window issue and per-access analysis.

**[S5] Actual bucket construction.** [`src/port/taskman_seam_battle_host.c`, lines 900–1070](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/port/taskman_seam_battle_host.c#L900-L1070): `ndsBattlePlayableFinalizePresentedIteration` and diagnostic retention fold.

**[S6] Source-derived particle rendering seam.** [`src/import/battleship_lbparticle.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/import/battleship_lbparticle.c): `ndsParticleQuadFrameFor`, camera preparation, `ndsParticleTransformForDraw` (lines 3510–3635), Whispy transforms, and `lbParticleDrawTextures`.

**[S7] Fighter timing boundary.** [`src/port/renderer_adapter_fighter.c`, lines 4900–5155](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/port/renderer_adapter_fighter.c#L4900-L5155): fighter contract capture and submission are inside the FTR bracket.

**[S8] DamageSlash runtime.** [`src/nds/nds_native_damage_slash.exec.inc`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/nds/nds_native_damage_slash.exec.inc): conversion, two-slot texture residency, native submission, and material handling.

**[S9] DamageSlash generated geometry and shapes.** [`src/nds/generated/nds_native_damage_slash.generated.inc`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/nds/generated/nds_native_damage_slash.generated.inc): 13 texture shapes, palette, vertices, triangles, and generated setup/finish state.

**[S10] Historical PC census.** [`2026-09-16_p2-2p8-n0409-profile/census.txt`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-16_p2-2p8-n0409-profile/census.txt). Region-normalization context: [`2026-09-16_p2-2p8-gap-sizing/README.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/artifacts/performance/2026-09-16_p2-2p8-gap-sizing/README.md). These are historical measurements, not new timings or general impossibility proofs.

**[S11] Native mirroring and palette variant runtime.** [`src/nds/nds_renderer_textures_effects.c`, lines 5380–5760](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/nds/nds_renderer_textures_effects.c#L5380-L5760): Fox glow texture preparation and the eight-entry environment-palette cache.

**[S12] Texture wrapper and diagnostics.** [`src/nds/nds_renderer_preamble.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/nds/nds_renderer_preamble.c): `ndsRendererHardwareFencedGlTexImage2D` and `ndsRendererHardwareRecordBattleTextureFence`.

**[S13] Fixed native weapon pilot.** [`src/nds/nds_native_samus_chargeshot.exec.inc`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/nds/nds_native_samus_chargeshot.exec.inc): `ndsRendererSubmitNativeSamusChargeShot`.

**[S14] Current particle submit and packet implementation.** [`src/nds/nds_renderer_textures_effects.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/src/nds/nds_renderer_textures_effects.c): `ndsRendererSubmitParticleQuad` around lines 6480 onward, `ndsRendererPrepareWhispyQuadState`, `ndsRendererFlushWhispyNativePacket`, and packet reserve/finish helpers.

**[S15] Versioned library implementation.** [`devkitPro/libnds v2.0.2: source/arm9/videoGL.c`](https://github.com/devkitPro/libnds/blob/v2.0.2/source/arm9/videoGL.c): `glTexImage2D`, `glColorTableEXT`, `removePaletteFromTexture`, and `glAssignColorTable`. The tag is the library version cited by the inspected DamageSlash code; a new target build must verify its actual SDK/link provenance.

**[S16] Native texture parameter definitions.** [`devkitPro/libnds: include/nds/arm9/videoGL.h`](https://github.com/devkitPro/libnds/blob/master/include/nds/arm9/videoGL.h): wrap/flip flags. This external header link is not a pinned project dependency; the analogous project implementation is independently pinned in S11.

**[S17] Hardware/library documentation checked September 17, 2026.** [BlocksDS memory map](https://blocksds.skylyrac.net/docs/internal/memory_map/) for TCM/DMA visibility; [BlocksDS 3D graphics tutorial](https://blocksds.skylyrac.net/tutorial/intermediate/3d_graphics/) for texture/palette VRAM mapping and loading. BlocksDS default memory addresses/layout are not assumed to be this project's linker layout.

**[S18] Read-only DS port reference.** [`decomp/sm64-nds/src/nds/nds_renderer.c`, lines 120–221](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/decomp/sm64-nds/src/nds/nds_renderer.c#L120-L221): queued texture loading and `glTexSync`.

**[S19] Read-only DS decomp reference.** [`decomp/sm64ds-decomp/src/ARMMathSaveState.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/db0d088bc61ac3e85f07a349857a1b3ec7eef55b/decomp/sm64ds-decomp/src/ARMMathSaveState.c): explicit hardware arithmetic register-state capture; no broader renderer architecture inferred.
