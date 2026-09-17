Smash64DS STG Optimization — Re-evaluation

Date: September 17, 2026
Repository snapshot: rockenrooster/Smash64DS_Port, master, db0d088bc61ac3e85f07a349857a1b3ec7eef55b
Previous review: snapshot 430aca2879e9071dc2b22f944f5c2909c9ce7aa4
Deliverable: corrected research and candidate-selection memo, not an implementation or a qualified performance result.

The branch API returned only master when this review began. All current-code references below are pinned to the snapshot above; search-result snippets from other commits were used only for navigation. The last qualified runtime checkpoint and the later measured candidate are identified separately. No repository files, build settings, or verifier assertions were changed. R1 R3

Executive decision

Change the implementation order from the previous answer. Do not start by treating the entire stage as a captured replay stream that merely needs larger DMA batches.

The current stage has three different execution situations: captured command replay, live submission using previously prepared native runs, and rebuilding the prepared representation. Crucially, the ordinary captured-replay mask covers Dream Land segments 5 and 7 only. The other stages do not inherit that mask, and Dream Land's other segments are not automatically replayed. The source explicitly documents earlier failed attempts to replay camera-dependent actors. R6

My revised recommendation is:

First pilot: reduce the hot working set and repeated work of the live native stage executor. Keep geometry, source ordering, and simulation unchanged. Target concrete reads, writes, scans, and runtime decisions, not an assumed 385K-tick block of removable CPU work.

First architectural replacement: compile each stage into compact, ordered native draw records with explicit dynamic inputs. Generate the ordering and representation decisions offline; use small native kernels at runtime. Reuse the existing residency and generation machinery instead of adding another overlapping cache.

Then test patchable GX chunks where the first pilot shows they can pay for themselves. Static command structure is reusable; camera-dependent matrix operands and animated materials are not automatically constant.

Keep painter-depth regrouping and specialized mixed-binding deformation as separate experiments. The deformation case is a credible stage-specific opportunity. General painter-depth regrouping is a difficult correctness problem, not the obvious first optimization.

The strongest new evidence is a repository-reported locality win, not a newly proven stage compiler: moving 508 bytes of scalar state into DTCM reduced the measured arm's WORK-H P50 by 43,200 ticks and P95 by 43,072. That work already exists; it is not additional savings available to this proposal. Its verifier still had a sample-window assertion issue in the reviewed record. R5

No new STG candidate in this memo has a measured tick saving. No evidence here proves that STG alone closes four-player 30 FPS. Equally, the historical failed implementations and arithmetic on marginal percentiles do not prove that all equivalent-result architecture changes are exhausted.

1. Requirements and the actual baseline

The product contract favors the fastest correct DS implementation, permits substantial precomputation and specialization, and requires native-only rendering in all built ROMs. It does not require retaining the N64 rendering architecture. The current execution board specifically retains 60 Hz simulation and records that SRC was reopened on September 17; the older SRC NO-GO in the sizing memo is therefore superseded. This STG review does not propose changing simulation frequency. R2 R3 R4

Use the project's approved accurate melonDS configuration for performance work. Do not substitute upstream melonDS performance numbers or new retail-DS measurement requirements. Upstream source in this memo is corroborating implementation evidence, not the project's performance authority. R2

1.1 Last qualified checkpoint, as reported by the repository

The September 17 board names a4eb24c9a85 as the last qualified checkpoint. Its first-line N04.08 summary is older than that detailed checkpoint section. The values below are reported results, not measurements executed during this review. R3 R4

Metric

P50 ticks

P95 ticks

WORK-H

1,600,960

2,320,576

STG

385,088

427,648

FTR

350,144

736,960

SRC

543,040

1,027,520

MISC

238,720

464,000

ALL

1,677,952

2,798,144

Against the operational 1,120,000-tick comparison threshold, WORK-H is 480,960 ticks over at P50 and 1,200,576 over at P95. These are differences between each whole-work percentile and a constant budget. They are not sums of component percentiles.

The approximately 1,120,380-tick physical cadence figure sometimes used elsewhere differs slightly from this operational threshold. Preserve the verifier's stated unit and threshold when comparing artifacts; do not silently mix timer ticks, ARM9 instruction cycles, geometry-engine cycles, and upstream emulator AddCycles() values.

1.2 Later locality candidate — keep this comparison separate

The DTCM report compares its own matched control and candidate, not the checkpoint in the preceding table. R5

Metric

Matched control

DTCM candidate

Delta

WORK-H P50

1,580,544

1,537,344

−43,200

WORK-H P95

2,320,768

2,277,696

−43,072

STG P50

337,472

324,992

−12,480

ALL P50

1,678,016

1,677,888

−128

Three reported runs reproduce the WORK-H improvement closely. A separate instruction-address analysis reports unchanged access counts for the moved data and approximately 25,042 ticks/frame less dereference stall. The report proposes eviction relief for part of the remaining whole-frame improvement, but correctly labels that mechanism as an interpretation rather than a measured decomposition. R5

The candidate still has 417,344 ticks of P50 work above the threshold. Its sample-window assertion must be resolved without weakening the guarantee that the samples and identity witnesses describe the same match. It should not be described as a fully qualified new shipping baseline simply because its performance result is encouraging.

2. Corrections to the previous review

2.1 Captured replay is much narrower than the recommendation implied

Previous emphasis: make complete ordered stage command programs by eliminating scaffolding around the existing replay path.

Correction: the ordinary capture/replay mask is stage-specific and selects only Dream Land segments 5 and 7. The source distinguishes a reused prepared native run table from a replayed GX command stream. Those are not the same optimization. R6

This changes the first question from “how expensive is replay setup?” to:

Which segments and submit classes consume the current stage's time, and how much of that time is live native emission, state preparation, diagnostic traffic, or hardware waiting?

The ordered-program idea survives, but broadening it to camera-dependent segments is a new compiler and dynamic-input problem, not a simple batching change.

2.2 Do not replay camera-dependent matrices unchanged

The source explains why prior actor replay failed: a captured LOAD4x4 containing projection × view × model fixes geometry to the capture camera. Widening the replay mask produced misplaced actor imagery; widening the assumed rigid set also failed. R6

“Static vertex data” does not mean “static final matrix.” Billboarding, special source matrix kinds, camera recalculation, dynamic ancestors, and projection changes must all be represented explicitly. A new program must patch or regenerate these operands, or use a different proven factorization.

2.3 The earlier word-elision implementation was not qualified as lossless

Task 55 E2 initially called its redundant COLOR/TEX_COORD elision lossless and reported a 3,916-to-3,561-word replay-buffer reduction. Its later owner follow-up records color pulsation during normal play and explicitly contradicts the initial blanket losslessness claim. R10

Correct wording: it is an unaccepted implementation with an observed multi-frame visual problem, plus a disappointing performance result. The report's suggested mechanism is not a proven diagnosis.

A future independently executable fragment must initialize the state its vertices require. Do not let correctness depend on the last color, UV, matrix, or texture state left by an unrelated owner or a previous frame.

2.4 Flat ALL does not establish zero useful work saved

The later DTCM experiment is a direct counterexample: WORK-H improved substantially while ALL P50 was effectively unchanged. Work can decrease without crossing a VBlank interval boundary. R5

The July experiments remain legitimate negative evidence about their tested implementations. What does not follow is a universal claim that removing CPU work or changing command representation cannot help. Prefer matched WORK-H distributions, cadence histograms, and actual wait attribution over a single cadence-quantized ALL statistic.

2.5 Marginal percentiles are not subsystem budgets

The September 17 sizing memo subtracts STG P50 from WORK-H P50 to describe the result of deleting the stage, and treats lane percentages summing above 100% as evidence of overlap. Neither inference follows from marginal medians alone. R4

For exclusive synthetic frame costs:

Frame

A

B

C

Total

1

1

1

0

2

2

1

0

1

2

3

0

1

1

2

Median

1

1

1

2

The component medians sum to 3 although the total median is 2. There is no overlap at all.

Similarly, median(W) − median(S) need not equal median(W − S). This is a mathematical issue independent of whether the real counters also contain nested spans.

Therefore, 95,872 ticks remaining after “deleting STG” is an illustrative subtraction, not a rigorous measured ceiling. Proper component sizing uses matched per-frame data with audited ownership; a real counterfactual also includes scheduling and cache effects. This correction does not demonstrate that STG can solve the target. It removes an invalid proof that it cannot.

2.6 Do not size current work from old Task 103 comments

Current source still carries historical phase totals in comments. The September 16 closeout describes a broken current Task 103 instrumentation attempt. A historically collected result, a current comment, and a currently runnable instrument are separate pieces of evidence. None licenses assigning those old totals to this snapshot's execution. R7 R9

2.7 Visibility separation partly exists already

The current commit loop reads a per-frame hidden-binding mask while prepared runs remain valid. “Separate visibility from prepared geometry” is therefore not entirely new. The remaining opportunity is more specific: an offline execution schedule and optional chunk culling must preserve replay/residency proofs, effective graphics state, and depth progression. R7

2.8 The VTX_10 range correction stands, but not an FPS prediction

The prior correction to the old raw-integer range comparison was valid. A ten-bit vertex coordinate has different fractional precision from a sixteen-bit coordinate. For example, raw v16 30272 can be represented exactly by signed ten-bit 473; both describe 7.390625. The decoder expands the ten-bit value by six bits. R11 H1

That does not prove a meaningful speedup. Exact compact encoding reduces transported data, not necessarily the number of transformed vertices. Keep it an optional compiler lowering, not a major standalone campaign.

3. What the live stage path actually does

The relevant chain is:

presentation
  prepare native stage owner
    validate/reuse topology and residency
    prepare camera/binding matrices
    prepare live material state
    assemble frame inputs and hidden-binding mask
    reuse or rebuild prepared native run tables

  source display traversal
    identify native stage segment
    commit that segment in its required order
      choose display-head pass when applicable
      test current visibility
      record per-run diagnostic snapshot
      captured replay, when eligible
        BeginRun -> transfer words -> publish state
      otherwise live native execution
        BeginRun
        per triangle: submit-class dispatch
        per corner: native source/prepared data lookup and emission
      record emitted counts and account the run

  finish native stage owner

Sources: renderer_adapter_stage.c and nds_renderer_native_owners.c. R7 R8

Here “live native” means native DS execution over generated data. It is not permission to revive the forbidden N64 graphics interpreter.

Three concrete findings deserve immediate attention:

Repeated head scans. When a packet supplies binding-head data, the current commit loop scans its run list in four passes for head order {0, 2, 1, 3}. A generated execution-order vector can retain that order while visiting each scheduled run once. Packets without head data already use one pass, so this is not a universal fourfold loop reduction. R7

Unguarded diagnostic traffic. ndsRendererNativeStageBindingHidden() records a multi-field shortfall snapshot for visible runs. ndsRendererNativeStagePublishRunEmission() stores emitted counts and serials for each committed run. These operations are visible outside the Task 103 timing guards in the inspected source. Their linked-build cost and their correctness consumers still need verification; they are not automatically removable just because their names sound diagnostic. R7

Live per-triangle decisions and data gathering. The native loop selects no-Z/cross-matrix/ordinary emission per triangle and gathers source and prepared vertex records for each corner. Stable aspects of these decisions can be generated into smaller schedules and specialized kernels. That is a different target from speeding up a cached DMA transfer. R7

4. Revised candidate ranking

The ranking below is for investigation and implementation effort, not a list of promised savings. “Compatible” means designed to preserve the current requirements, subject to validation.

Rank

Candidate

Scope

Evidence strength

Main risk

1

Compact hot stage state and lean success-path accounting

Live and replayed stage runs

Concrete source targets; locality mechanism has measured precedent

Moving required failure detection or merely relocating cost

2

Generated ordered live-run schedule and specialized emission kernels

All stage packets, according to their submit classes

Concrete repeated scans/dispatch/data gathers in current source

Cache footprint and compiler/code-size regressions

3

Patchable, state-complete GX chunks

Selected live or replayed segments

Architecturally plausible; not yet sized

Dynamic matrices, register ordering, patch/flush cost, memory growth

4

Explicit dirty inputs for remaining stage transforms/materials

Mutating visual bindings only

Useful design; substantial caching already exists

Incomplete invalidation and redundant bookkeeping

5

Direct endpoint deformation for mixed-binding geometry

Particularly Inishie scale strings

Expensive current representation is identifiable

Clipping, range, and endpoint-transform equivalence

6

Painter-depth grouping or a new native depth representation

Eligible no-Z subsets

Real operation to eliminate; correctness is difficult

Occlusion/transparency changes across cameras

7

Conservative chunk visibility and exact topology lowering

Demonstrably eligible geometry

Historical negative evidence limits easy opportunities

Invalidating caches or silently removing visible content

8

Exact compact vertex encoding

Eligible emitted coordinates

Representation correction verified on host

Minimal whole-frame benefit

Candidate 1 — Compact hot state and lean success-path accounting

Purpose: reduce scattered data-cache traffic before investing in a large renderer rewrite.

The DTCM result supports investigating small frequently accessed state. It does not mean that every field belongs in DTCM, or that the existing 43,200-tick benefit can be counted twice. The report gives 1,484 bytes below its DTCM assert ceiling after the move; that is a property of its linked arm, not a fresh reservation for another feature. Re-read the actual shipping and profiling maps. R5

A useful first pilot would keep the existing draw algorithms and examine:

The repeatedly read stage cursor, active packet pointers, visibility words, state-shadow fields, and current-run identity.

The per-run diagnostic snapshot and emission publication.

Repeated success-path writes to values that are only needed to explain a later failure.

For diagnostics, preserve the distinction between deciding a run is correct and recording detailed history about why a run failed. A compact current-run record, direct failure-reason propagation, or a first-failure latch may replace broad before/after counter snapshots. But first trace every consumer. Keep native-failure, missing-geometry, rejected-run, and required verifier witnesses intact.

Do not merely guard everything with NDS_TICK_HUD and call the gate faster. Measure the shipping-shaped configuration as well. Eliminating profiler-only work is not the same as accelerating the shipped game.

Budget rule: relocate or replace existing state; do not keep duplicate authoritative representations. Preserve initialized versus zero-initialized storage. DMA source buffers remain separate from CPU-private hot state.

Discriminating test: same effective draw output and simulation state, with lower success-path accesses/stalls and lower WORK-H. A lower STG counter alone is insufficient.

Candidate 2 — Generated ordered live-run schedule and specialized kernels

Purpose: replace the repeated interpretation of a known native layout with an execution-ready layout.

At build/load time, produce the ordered run vector after applying the packet's required head ordering. Attach the invariant submission class, binding, coordinate-packing policy, material handle, and vertex/span offsets to each record.

At presentation time, the runtime should read the run's genuinely dynamic visibility/material/transform inputs and enter the corresponding small native kernel. It should not rediscover four head partitions, recalculate invariant coordinate-shift choices, or repeatedly decode the same submit-class decision for every vertex.

A conceptual interface is:

/* Design sketch; these are proposed interfaces, not existing project APIs. */
void StagePrepareChangedInputs(StageInstance *stage, const CameraState *camera);
void StageSubmitOrderedSegment(StageInstance *stage, unsigned segment_id);

Internally, the first implementation needs only a few explicit classes, for example ordinary/range geometry, painter-depth geometry, and mixed-binding deformation. Generate native stage-specific data; keep shared kernels small. A compact native draw record is not an N64 graphics command stream.

For vertex emission, derive whether the existing dense/corner indirection or an execution-ready vertex span is cheaper. Expanded per-corner data can eliminate dependent loads but increase the footprint. Calculate actual bytes from the selected stage before choosing. Do not assume “flattening” always improves locality.

The initial pilot should preserve the current effective hardware work. In particular, it can retain per-triangle painter-depth matrices while removing CPU-side recomputation and general dispatch. That cleanly tests the CPU/data-layout thesis before changing rendering semantics.

Output invariants: source segment boundaries, within-head order, external owner boundaries, alpha behavior, matrix binding, visibility behavior, near-plane behavior, depth progression, and honest submitted-work counters.

Falsifier: the schedule executes fewer CPU decisions but total work does not improve because the new layout displaces hotter data or increases hardware waits. Measure that outcome; do not add a second cache to rescue it by assumption.

Candidate 3 — Patchable, state-complete GX chunks

This is the corrected version of the previous top recommendation.

Compile chunks whose command structure is immutable. Resolve resident texture/palette addresses at setup, and patch the operands that actually change. For camera-dependent geometry, the patch set must include the required camera and binding results. For animated materials, it must include texture-frame/UV/color inputs as applicable.

Begin with one expensive live segment identified by the census, not every stage at once. There are two distinct experiments:

Packaging-only experiment: retain equivalent command operations but reduce CPU run setup and submission boundaries.

Operation-reduction experiment: reduce hardware work through a separately justified depth, topology, or transform representation.

Never conflate them. A smaller buffer does not establish lower geometry work.

A segment cannot be fused across an intervening required owner. Moreover, the current source explicitly notes that its per-head stage ordering does not fully reproduce all cross-owner interleaving in a camera group. Preserve the known boundary and qualify that existing visual issue rather than silently fossilizing it in a new packet. R7

Not every rendering register belongs in the GX stream: glAlphaFunc() and glEnable()/glDisable() address rendering controls rather than behaving like arbitrary packed GX commands. Packet generation must distinguish these from geometry command state. H2

Packet lifecycle requirements: initialize incoming state explicitly; preserve matrix-stack balance; finish before any competing FIFO writer or source-buffer reuse; flush CPU-written cached source ranges correctly; account for alignment, DMA setup, and bus contention. Never assume DMA makes the underlying geometry work disappear. Keep command storage in the established main-RAM DMA buffer domain, separate from CPU-private hot-state placement, and follow the selected library's transfer and coherency contract. The inspected libnds glCallList explicitly flushes its command data and waits for the transfer. R6 H2

Memory rule: keep one selected-stage representation and bounded patch storage. Replace old persistent tables/buffers where possible. Whole-stage double buffering or a bank of every possible visual state is not approved merely because the product contract permits spending RAM.

Falsifier: patching, cache maintenance, or extra resident pages consume the saved preparation work. Stop expansion when the matched full-frame result is neutral or worse.

Candidate 4 — Explicit dirty inputs, but only where work remains

Do not reimplement the already-existing steady admission, topology generation, prepared-run reuse, texture-proof epochs, or rigid/dynamic split. The adapter still prepares frame inputs and the renderer still consumes live changes; optimize the measured residue. R6 R8

Use separate invalidation domains for topology/residency, camera, binding transforms, material animation, and visibility. Camera movement need not invalidate immutable local geometry. A flower's material frame need not invalidate an unrelated static run.

A dirty system must cover every writer, including actor callbacks, animation attachment, flags, hierarchy changes, and scene/heap replacement. Pointer equality alone is not an identity proof after arena reuse. Inherited source nodes with camera-dependent transform kinds must not be labeled static merely because they have no ordinary animation track.

For visual animation, phase advances and emitted events may still be required at 60 Hz even when visible state is sampled less often. Preserve hazard and collision updates. The source assigns layer 1 to the yakumono update path while other layers use ordinary animation; it is not safe to decimate a generic stage-animation call indiscriminately. R13

Falsifier: most expensive preparation already reuses its outputs, or marking/consuming dirty records costs as much as the small remaining check. This is an adjunct to the compact executor, not a new scene-graph framework.

Candidate 5 — Direct endpoint deformation for mixed-binding geometry

The current cross-matrix stage path can transform each corner, clip in software, and then load a matrix containing a corner's clip coordinates before emitting a zero-position vertex. It exists to retain geometry whose corners originate from different binding matrices. That semantic requirement is real; the matrix-per-corner representation is not sacred. R7

For a scale string, derive a native endpoint recipe from the actual source transforms. Express each endpoint in one common coordinate frame, update the small number of affected vertices, and submit the primitive through a normal native transform path.

Start with an actor whose exact transform family is known. Translation-only relationships are much simpler than arbitrary rotation/nonuniform-scale chains. Preserve both endpoint motion, visibility, UVs, attachment positions, and collision behavior. Do not freeze the string into a static mesh.

Use the existing native path as a differential reference where appropriate. Hardware clipping must be shown equivalent for the accepted input range; replacing pre-clipped homogeneous vertices with Cartesian vertices without preserving projection and interpolation is not automatically correct.

This work must be sized on the stage that executes it. It contributes nothing to a Dream Land-only timing window when the actor does not exist.

Falsifier: too few executions to justify the machinery, or the relative-transform/range repair costs more than the current path. A small matrix palette is an alternative to measure, not an assumed faster solution.

Candidate 6 — Painter-depth grouping: retain, but demote

The current no-Z path replaces a matrix's Z column with a value derived from its W column and a synthetic depth. It can do this per triangle. This is a genuine operation-reduction target. R7

However, BattleShip explicitly disables source Z-buffering for layers 0, 2, and 3, while layer 1 uses Z-buffered modes. Turning every stage triangle into ordinary depth-tested geometry is not a representation-only rewrite. R13

A conservative grouping prototype can let compatible triangles share a painter-depth operation when their coverage and all relevant ordering interactions make that safe. This requires more than matching materials or seeing no overlap in one screenshot. Check shared edges, translucent composition, polygon IDs, clipping, external owners, moving cameras, and the depth range left for later submissions.

If T eligible triangles can be represented by G valid groups, the intended structural saving is T−G depth operations. That is an operation count, not a tick estimate. Establish the groups first; then measure.

A more aggressive DS-native depth redesign is allowed as a visual-fidelity experiment under the product's approval process, but is not the first low-risk optimization. Rejecting a failed exact-output certificate does not authorize deleting scenery or silently changing occlusion.

Falsifier: conservative groups collapse to almost one triangle each, or correctness requires expensive per-frame overlap reasoning. Do not build a general runtime graph solver to save a small static stage's draw cost.

Candidate 7 — Visibility and topology: narrow the claim

Task 63 found no never-visible triangles across the fixture sets it tested, only 9.1% constrained reduction under its material/silhouette rules, and a severe metric problem: full-frame coverage overlap could hide a badly damaged fighting surface. Its later culling experiment also disturbed texture/material coherence, making CPU work worse. R12

These results strongly discourage “just simplify Dream Land” as a cheap first move. They do not establish a theorem about every stage or an execution schedule that does not invalidate material preparation.

A justified reopening requires a specific new fact: a correct camera envelope, a new packet/caching boundary, or identified redundant submitted work. Keep visibility independent of packet identity and preserve any state/depth transitions skipped geometry used to provide.

Never relabel deleting a small visible bush as culling invisible work. Compare the changed object/region, not merely a whole-frame mask dominated by the unchanged backdrop.

Candidate 8 — Exact compact vertices

An offline compiler can choose VTX_10 only when all coordinates reproduce the original emitted v16 values exactly; otherwise keep VTX_16. Exact axis-reuse encodings have their own state requirements.

The useful application is a lowering pass in candidate 2 or 3, after real emitted-coordinate and command-size counts exist. It should not trigger a standalone campaign on the strength of the old range-analysis error.

Host arithmetic checks in this review examined all 65,536 signed v16 axis values: 1,024 are exactly representable by the simple signed-ten-bit expansion. That verifies the eligibility arithmetic only; it does not count how many vertices in the current game qualify, validate a packet encoder, or measure performance.

5. A simple target architecture

The runtime should have one selected-stage instance with three conceptual pieces:

Stage immutable program
    ordered segment/run schedule
    native geometry payloads
    material descriptors and residency requirements
    transform/deformation recipes
    explicit patch locations where needed

Stage live inputs
    current camera
    changing bindings/material phases
    visibility and actor state
    scene/residency generations

Small native executor
    prepare changed inputs
    execute ordered segment at its existing display boundary
    retain only necessary renderer state and failure witnesses

The first migration should remove old data/decisions as it adds new ones. A new StageProgram beside an unchanged source graph, all old prepared tables, a full replay owner, and a new double buffer is not the intended design.

C versus assembly: use generated data and small C kernels first. Consider assembly only for a measured remaining kernel. Preserve the compiler/linker map and inspect literal-pool and data dependencies; an apparent instruction reduction can still worsen cache behavior. The native SM64 DS reference uses explicit fixed-point affine matrices, and the carried sm64-nds renderer places small rendering state in DTCM. Those are useful design precedents, not proof that their complete render architectures or performance transfer to Smash. R14 R15

Do not copy the reference interpreter. The source-format renderer in sm64-nds is not compatible with this project's native-only target contract merely because its state placement is informative. R2 R14

Do not enable the old NDS_DREAMLAND_DS_MESH path as a shortcut. Task 63 records that path as known broken and default-off. A new implementation must address its documented provenance/topology problems rather than inheriting its name and assuming the problem was solved. R12

6. Stage-specific scope and regression priorities

Stage/content

Most relevant first question

Required preservation

Dream Land

Which live segments dominate after the existing 5/7 replay and prepared-table reuse?

Camera-dependent scenery, Whispy/flowers, water material behavior, layer ordering

Peach's Castle

Which live submit classes and head schedules are costly?

Roof coverage, winding, clipping, backdrop/foreground order

Mushroom Kingdom / Inishie

Does mixed-binding scale-string emission materially contribute?

Both moving endpoints, scale-platform motion, real depth and clipping

Planet Zebes

How much cost belongs to the stage owner versus actor/effect accounting?

Acid timing, collision/hazard activation, visibility, transparency

Yoshi's Island

What is ordinary stage draw versus separate cloud actor work?

Cloud motion/visibility synchronized with its gameplay state

Other admitted stages

Classify current generated packet and execution route before choosing a kernel

Native coverage, all supported camera/actor states, legal four-fighter matches

This is a testing map, not a claim to have profiled each stage in this review. Several actor paths are accounted outside STG, so improvements must be assigned by the actual timed owner, not by the English name of the object. R7 R8

7. Measurement plan that can discriminate the candidates

7.1 Pin a reproducible executable before changing it

Record source SHA, ROM/ELF hashes, selected stage and roster, build configuration, approved emulator identity/settings, native route engagement, scene/residency generation, and linked section sizes. Use the project's approved profiling configuration unchanged; do not treat results from different emulator timing or JIT settings as interchangeable.

Clean the derived payload and check its membership. The repository previously recorded a large apparent STG improvement from removing stale NitroFS contents with an unchanged ELF; an unclean directory can confound a renderer A/B. R9

The reported 14,080-tick cross-build floor is not a universal confidence interval. The locality report itself discusses larger placement swings. Prefer same-executable route comparisons for compatible native paths and repeated matched controls. A source-level optimization claim should survive a layout-controlled comparison, not merely exceed one historical threshold. R5

7.2 Collect a short route census, not a new giant timer framework

For each segment, record counts for prepared-table rebuild/reuse, captured replay, live emission, ordinary/range/no-Z/cross-matrix triangles, head passes, material updates, and matrix operations.

Count emitted work separately from CPU work. A packet hit can submit many triangles without executing their old CPU preparation loops. Do not credit those skipped loops as actual CPU work merely to satisfy a liveness counter.

Use existing low-impact instrumentation or host-side approved-emulator hooks where available. If additional emulator hooks are required, they are an implementation task, not an available measurement already performed here. Avoid the currently broken Task 103 path until it has an independent build/runtime qualification. R9

7.3 Pilot one mechanism at a time

Recommended sequence:

Pilot A: preserve current geometry and scheduling; replace or relocate a bounded set of hot stage state and audit the per-run diagnostic success path.

Pilot B: preserve hardware work; replace one live segment's general loop with generated ordering and a specialized native kernel.

Pilot C: preserve effective operations; represent that same segment as a patchable chunk and include all patch/flush/submission costs.

Pilot D: only after the previous results identify a geometry-side bottleneck, test a narrowly certified depth/deformation change.

Run focused checks after each pilot. Once a batch is worth integrating, run the project's required focused four-CPU and broader regression coverage for that change. Do not spend a full campaign repeatedly rebuilding an unsized idea whose first counterfactual is already negative.

7.4 Use aligned per-frame data and whole-work outcomes

For audited exclusive CPU spans in frame f, a useful first sizing model is:

estimated_saved[f] = old_target_cpu_work[f]
                   - new_cpu_work[f]
                   - added_patch_and_cache_work[f]
                   - added_synchronization[f]

This is only a sizing model: real replacement execution can change cache behavior and hardware waits elsewhere. The final verdict is the actual matched WORK-H distribution and cadence histogram.

Do not calculate P95(total − component) by subtracting their separately ranked P95 values. Do not add the P50 improvements from unrelated builds. Do not claim bucket overlap from the sum of marginal percentiles alone. Audit timing anchors and nesting directly.

7.5 Correctness must cover time, not one frame

Compare required gameplay state at aligned source ticks, and compare rendering across a sequence of camera, material, visibility, and actor states. Preserve the relevant state-hash rules; pointer addresses must not be mistaken for gameplay divergence when a permitted representation change moves storage.

Exercise camera movement and pause/resume; stage actors; transparent overlaps and shared edges; entry, KO, and scene re-entry; texture lifetime; and worst-memory legal combinations. Check the changed surface independently of the large unchanged backdrop.

For packet work, validate incoming/outgoing graphics state at fragment boundaries, not merely the words inside an isolated capture. Task 55's multi-frame failure and the actor-replay failures are the reasons these tests are required. R6 R10

7.6 Promotion requires all of the following

A candidate must preserve required content and behavior, remain native-only, show route engagement, stay within measured memory constraints, and improve the actual work/cadence objective without a hidden cost transfer. Resolve sample-window provenance before declaring a gate pass. Owner approval remains required for permanent fidelity changes. R2 R3

No new whole-stage budget or percentage-reduction promise is invented in this report. Establish the representative stage distribution first, then allocate the remaining game-wide budget from measured data.

8. What is established, plausible, and not established

Established by inspected source or retained evidence

Current ordinary captured replay is restricted to Dream Land segments 5/7; the live native path is essential. R6 R7

Prepared-run reuse, generation-based admission, texture proofs, and dynamic visibility already exist. R6 R8

Current stage code contains repeated head scans, per-run snapshots/publication, and live per-triangle/per-corner decisions. R7

The repository reports a reproducible DTCM-arm WORK-H improvement with a separate sample-window acceptance issue. R5

Previous replay broadening and word-elision attempts have relevant visual failures, not just neutral timings. R6 R10

Marginal percentile addition/subtraction cannot establish the claimed subsystem ceilings. This was checked with synthetic host cases.

Plausible, but requiring a pilot

Compact live state; a generated ordered native run schedule; specialized emission kernels; state-complete patchable chunks; direct endpoint deformation; narrowly certified painter-depth groups.

Not established

The current exclusive STG CPU/stall partition; a new candidate's savings; an all-stage/all-roster stage budget; a proof that STG alone closes the game-wide target; or a proof that equivalent-result architectural options are exhausted.

Final recommendation

Start with a compact live-stage executor, not a universal replay rewrite. First remove unnecessary hot-state traffic and invariant runtime decisions while keeping rendering operations equivalent. Use that measurement to decide whether patchable chunks or a depth/deformation redesign are worth their additional correctness and memory cost.

The project has demonstrated that a small change in data access can matter more than a large apparent reduction in arithmetic. The next STG architecture should exploit that lesson without simply renaming old caches or counting already-landed wins again.

Appendix A — Host checks performed during this review

These checks executed successfully in the analysis environment. They are mathematical checks, not renderer tests:

Three exclusive synthetic timing components have medians 1/1/1 while total median is 2. This disproves inferring overlap merely from marginal medians summing above the total median.

With W=[100,101,1000] and S=[0,100,0], median(W)-median(S)=101 but median(W-S)=100. This disproves the general residual-percentile subtraction rule.

All 65,536 signed v16 axis values were checked against exact signed-ten-bit expansion. Exactly 1,024 passed; every eligible value round-tripped.

The constant-budget gaps above were recomputed: 480,960 / 1,200,576 for the qualified checkpoint and 417,344 P50 for the DTCM candidate.

No ROM was built, no approved emulator run was executed, no current geometry asset corpus was decoded, and no new visual equivalence or performance result is claimed.

Appendix B — Sources and implementation locations

Repository sources below are pinned to db0d088bc61ac3e85f07a349857a1b3ec7eef55b. Historical documents retain their own dated experiment provenance. External source URLs are implementation references retrieved September 17, 2026, not substitutes for the approved emulator's timing model.

Reference

What was used

R1

Branch selection at review start

R2

Product, native-only, memory, performance, and fidelity rules

R3

September 17 checkpoint, current simulation/SRC decisions, remaining acceptance

R4

Reported checkpoint timing table and the ceiling arguments re-evaluated here

R5

Matched DTCM comparison, repeat runs, access/stall analysis, memory and window caveats

R6

Replay mask, failed actor replay history, prepared-table versus GX replay distinction

R7

ndsRendererCommitNativeStageSegment; ndsRendererNativeStageBindingHidden; ndsRendererNativeStagePublishRunEmission; ndsRendererTask36ReplayRun; ndsRendererNativeStageTask36LoadNoZProjection; cross-matrix/depth-vertex emission

R8

Stage preparation, live frame inputs, segment dispatch and finish

R9

Historical clean-build effects and currently broken phase-instrument warning

R10–R12

Historical command-elision, encoding, and Dream Land simplification experiments, including their negative findings

R13

Original layer depth modes, display links, and animation/collision update distinction

R14–R15

DS-reference hot-state placement and explicit fixed-point affine layout

H1–H2

Vertex encoding, rendering-control versus GX-command distinction, and the inspected command-transfer implementation