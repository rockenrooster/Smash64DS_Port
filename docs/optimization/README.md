The next major effort should be a smaller DS-native runtime—not another layer of renderer caches

The source supports a substantial simplification campaign. The important distinction is that native rendering does not necessarily mean native preparation. A path can submit exclusively to the DS geometry engine while still spending too much CPU time rebuilding source-style state, translating representations, checking immutable properties, and preparing data that should already be ready.

I audited master at 75f7f6b4b4864c82c01872d0fd2771d171005272, examined the repository’s recent performance evidence, and compared selected implementations against the SM64DS reference and DS hardware documentation. This is a source-and-recorded-measurement audit—not a new ROM benchmark.

The full report includes source references, architecture proposals, ten bounded implementation work packages, dependencies, correctness gates, and performance acceptance criteria:

Download the detailed optimization plan

The main findings and recommended direction follow.

1. The actual target is approximately a 60% reduction, not merely 50%

The latest inspected September 15 hard-on four-CPU checkpoint reports:

Metric	Recorded result
WORK-H P50	1,680,384 ticks
WORK-H P95	2,389,376 ticks
Two-VBlank presentation cadence	103 / 1,973 frames — 5.22%
Native failures / rejects	0 / 0
General-heap low-water	108,096 bytes

That checkpoint already includes several recent optimizations; it is not an old unoptimized baseline.

Using the repository’s 1,120,380-tick gate:

$$ 1-\frac{1,120,380}{2,389,376}=53.11\% $$

A 50% reduction would leave 1,194,688 ticks, still 74,308 ticks over budget. I recommend an internal design objective around 950,000 ticks, which would require a 60.24% reduction and provide approximately 170,000 ticks of headroom. That is a proposed engineering target, not a predicted result. The cadence requirement must pass independently.

This is large enough that ITCM rearrangement, compiler switches, or a handful of faster math functions will not constitute the whole solution. The campaign needs to eliminate categories of work.

Do not count existing improvements twice

The retained checkpoint already has fighter packet replay, direct BGM/FGM ranges, resident/early BPS1 work, and pose active-track/running-joint masks. The recent running-joint-mask experiment improved same-ROM WORK-H P95 by 9,408 ticks—valuable, but much smaller than the remaining gap.

The next plan should not repeatedly rediscover “use hardware math,” “cache animation,” or “replay geometry.” It should ask what expensive machinery those improvements still leave running.

2. What I found that deserves architectural attention
A. ITCM placement is still too coupled to code organization

A September 15 diagnostic census, preceding the latest final checkpoint, reports 32,640 bytes of ITCM usage, with 96 bytes free in the configured region. It also identifies 4,650 bytes across 20 residents that did not execute in that profiling window. Those are investigation candidates—not proof of globally dead code.

Two measured renderer residents illustrate the pressure:

Function	Size in that census
ndsRendererCommitNativeStageSegment	2,820 bytes
ndsRendererNativeStageBeginRun	2,352 bytes
Combined	5,172 bytes

The next question should be whether their binding, validation, setup, and error-handling portions can leave the hot kernel, not simply whether the entire functions should be evicted.

There is also a structural placement problem. The linker contains *.32.o policies affecting both ITCM and DTCM, while renderer macros explicitly place functions into shared sections such as .itcm and .itcm.native_fighter. This can make unrelated functions compete as an indivisible group. Choosing ARM instructions should not automatically choose fast-memory residency.

My recommendation is unique sections for selected kernels, explicit data placement, and an input-section-level ownership report. Account for aliases, literal pools, veneers, padding, and the actual eviction cost.

Also, nds_renderer.c textually includes implementation .c files. Breaking a large file into included fragments does not necessarily create independent translation units or simpler runtime interfaces. More files are not the same thing as less runtime complexity.

B. “Native” paths still reconstruct too much source-style state

In the production fighter path, ndsRendererNativePreflightProductionOwner is called before ndsFighterPacketTryReplay. That does not prove all preflight work is redundant—its internals may already cache some checks—but it gives a concrete question to answer with the existing preflight timing counter:

What does a successful replay still pay to establish facts that have not changed since model binding?

The shared native code also contains packed GBI triangle-index decoding, multi-field eligibility checks, and per-slot color/texture-coordinate preparation. These should be classified by actual reachability and frequency, then moved offline where their inputs are immutable.

A particularly clear example is ndsRendererSubmitNativeTaruCann. This is a two-joint textured-quad actor, but its inspected submission path constructs generic configuration/traversal state and vertex structures, checks hierarchy relationships and matrix structure, prepares texture state, and converts matrices.

For a fixed actor like that, the intended runtime should be closer to:

Update its genuinely dynamic transform.
Patch its genuinely dynamic attributes.
Submit already-native geometry.

This barrel is not proven to be the dominant four-CPU bottleneck. It is a concrete example of an interface that makes simple content do unnecessarily general work.

C. Fixed-point code still lives inside a mixed-representation system

The pose engine contains fixed-point values alongside source floating-point fields. More importantly, its animation clock uses binary32 bit patterns advanced by an integer implementation of IEEE addition/subtraction. That avoids ordinary floating-point helper calls, but it is still floating-point arithmetic semantically—not the fixed-point endpoint you requested.

The endpoint should be:

Fixed producer → fixed state → fixed consumers → native hardware data

Not:

Source float → fixed helper → source float field → fixed renderer input

Historical project measurements already found that conversion-heavy fixed-point replacements can lose to the original operation. The lesson is to convert complete producer/consumer chains, not to abandon fixed point.

D. Some apparent complexity protects real semantics

For example, ftMainSetStatus can allocate, remove, or reparent grab/throw-related DObjs without changing the fighter root pointer or heap generation. The wrapper explicitly invalidates both flat-walk and renderer caches because gameplay attachments can otherwise become stale.

A lean replacement must therefore have topology-owned invalidation, not simply fewer checks.

This is the distinction I would apply throughout the cleanup: remove repeated discovery, but retain the underlying ownership rule.

3. What the SM64DS reference actually teaches

The strongest examples are not clever instruction tricks. They demonstrate that data is prepared at the right time and kept in the representation its consumers need.

Inspected SM64DS implementation	Pattern worth adopting
Model::UpdateFileOffsets	Resolve file-relative model/material/texture/display-list pointers during loading
CrossVec3	Explicit fixed-point operands, wide products, and defined rounding
Camera_UpdateMatrices	Fixed vectors and native matrix construction with angle lookup data
ModelAnim::SetAnim	Same-animation fast path updates flags/speed without full rebinding

These patterns are visible directly in the inspected implementations.

The model format also exposes flat tables and packed DS display-list records. That is the useful direction: assets that arrive close to executable form, rather than assets that require a substantial translation process every time they are drawn.

One caution: the reference tree contains modern hosted code as well as DS-oriented reconstructed source. A host renderer using floating-point matrix vectors should not be mistaken for the original DS execution architecture. Borrow the verified patterns, not every modern implementation detail.

4. The architecture I recommend

Keep the working gameplay, assets, tests, and existing native coverage. Do not restart the project. Replace internal representations in bounded vertical slices.

BUILD TOOLS
    Original assets and behavior descriptions
        ↓
    Fixed constants, compact tracks, event metadata,
    native GX geometry, bounds, dependencies, patch tables

LOAD / SPAWN / TOPOLOGY CHANGE
    Validate and relocate once
        ↓
    Bind native resource handles and precise generations

LOGIC UPDATE
    Compact fixed fighter state
        ↓
    Events, decisions, physics, collision, required sockets

PRESENTATION UPDATE
    Changed visual pose/material state
        ↓
    Small native patches
        ↓
    GX / BG / OAM submission

The key is one authoritative representation per domain.

During migration, a compatibility bridge may be necessary. Completing the domain includes deleting that bridge and its old authoritative state—not leaving both systems synchronized forever.

Four small ownership boundaries

An immutable model bank owns geometry, material identities, parent indices, static channels, transform classes, bounds, and dependency tables.

A bound instance owns resolved resources, selected model variant, topology/material generations, and typed patch locations.

A live fighter state owns fixed movement, status, inputs, event timing, and compact collision data.

A presentation state owns visual pose and native matrices without forcing camera changes to invalidate animation or palette changes to invalidate topology.

These are conceptual boundaries, not a proposal to add a new class framework.

5. Renderer: remove work before optimizing its instructions
Bind immutable facts outside the draw

Validate asset ranges, native program identity, material count, hierarchy relationships, and constant transform properties at admission or the relevant state change.

The hot draw should consume a bound descriptor. It should not reconstruct a source-style configuration merely to recover those facts.

Keep checks for things that genuinely change: packet capacity, resource generation, dynamic count bounds, current ownership, and buffer lifetime. Move immutable validation; do not make invalid data silently acceptable.

Generate native packets instead of recording them where practical

Packet replay already works. The next question is whether the build tools can produce the packet, patch table, and bounds directly, removing first-use recording and its retained scaffolding.

Factor:

Immutable geometry
+
Dynamic matrices
+
Visibility
+
Material/palette selections
+
A small set of exceptional dynamic coordinates

Do not precompute the cross-product of pose × camera × costume × material animation × visibility. That creates a new storage and residency problem.

Choose packet ownership deliberately. A writable packet per active instance may avoid copies but costs RAM. Shared immutable geometry plus dynamic runs saves memory but can increase submission overhead. Measure the entire path, including patching, cache maintenance, and synchronization.

Use native transform classes, not one giant generic matrix adapter

The existing adapter documents real special cases: billboards, orientation replacement while preserving translation, scale accumulation, and camera-dependent recalculation. These cannot all be replaced by a generic TRS operation.

Instead, classify the required cases at generation/bind time and use a small number of specialized kernels.

Use 4×3 affine matrices where the transform is affine. Retain proper perspective handling where needed. Choose consistent model/world units so submission does not repeatedly copy a matrix just to rescale its translation.

Avoid computing a complete CPU hierarchy and then independently repeating the same hierarchy work on GX. Compute CPU transforms for gameplay consumers and exceptional semantics; let the native graphics path handle the rest.

Keep depth and transparency behavior explicit

Do not globally sort everything by material or force everything into one depth mode.

The replacement needs native classes for ordinary depth-tested geometry, translucent geometry, source painter-order/no-Z layers, and special billboard/depth behavior. Otherwise the optimization will reintroduce exactly the kinds of shield, impact-wave, and stage-ordering failures the project has been fighting.

The objective is less CPU work for the same render rule, not a cheaper but incorrect rule.

6. Fixed point: convert the whole pipeline, and redesign the clock correctly
Numeric formats must be chosen by domain

I would establish an inventory of units, ranges, precision, rounding, overflow behavior, and consumers before changing field definitions.

World coordinates, velocities, matrices, angles, normals, texture coordinates, and event times do not all want the same format.

Use 32-bit native values where possible, wider intermediates where required, and explicit narrowing rules. Avoid replacing every operation with a 64-bit divide or normalizing every vector merely because the new code is “fixed point.”

The DS’s ARM and Thumb tradeoffs also matter: ARM supports long multiply operations useful for fixed math, while smaller code can reduce instruction-fetch cost. Blanket ARM and blanket -O3 are not substitutes for measuring the resulting kernel.

The animation clock is a genuine trap

The source gives this example:

Q12 representation of 1/3: 1365
Three increments:         4095
One whole frame:          4096

A boundary that should be reached after three updates may instead be reached after four.

The file records 635 live mismatching cases across 44 speed sources, including actual move timing differences, and explains why simply increasing fixed precision did not eliminate every mismatch with rounded binary32 behavior.

My recommendation is to separate authored event timing from visual interpolation.

For event timing, define an integer/rational accumulator or generated deadline scheme, including remainder handling, speed changes, hitlag, loops, interruption, and attach/end behavior. Where matching the required event boundaries needs a correction, generate or test that correction explicitly.

Exact rational arithmetic is not automatically equivalent to repeated IEEE rounding. Do not replace one mistaken exactness argument with another, and do not use a global epsilon.

The clock helper alone is not established as a major current bottleneck. Its replacement belongs in the fixed-native endpoint; the large performance opportunity must come from removing the complete chain of mixed representations and repeated processing.

Separate gameplay-required joints from visual-only work

Root motion, active hit/hurt volumes, grabs, held items, weapon sockets, and other gameplay consumers must remain current at the required simulation rate.

Compute their ancestor closure at bind time and update it when dependencies or topology change. Visual-only pose work can follow the presentation schedule where fidelity permits.

The current pose system already has held-body evaluation and active masks. The new work is therefore eliminating duplicated transforms and unnecessary representations, not simply adding another skip mask.

Precompute selectively—full matrix baking is too expensive

An illustrative one-second bank with four fighters, 32 joints each, 60 samples, and 48-byte affine matrices costs:

$$ 4 \times 32 \times 60 \times 48 = 368,640\text{ bytes} $$

That is 360 KiB, compared with the latest recorded heap low-water of about 106 KiB. The example uses hypothetical joint/sample counts, but it demonstrates the tradeoff.

Prioritize constant channels, compact keys, interpolation coefficients, event tables, static local transforms, dependency closures, and bounded active-clip residency.

Bake invariant local work—not final results that depend on live facing, scale, world position, attachments, or procedural state.

7. Game logic, collision, and CPU players need the same treatment

The current fighter and CPU wrappers still import substantial source-game behavior and retain source-style interfaces. That gives the native rewrite useful behavioral references, but it also means renderer-only work cannot complete the fixed-point goal.

I recommend a compact four-fighter hot-state representation, separated from cold descriptions, plus bounded active lists for weapons, items, and effects.

Share facts, not decisions. Once per logic tick, prepare reusable positions, conservative bounds, relevant stage neighborhoods, and target eligibility where consumers currently rediscover them. Let CPU decisions consume those facts without changing their timing.

For collision, compile static stage-line metadata and use inexpensive conservative rejection before narrow work. A small ordered list or a few bins may outperform an elaborate tree on a small stage.

Four fighters give six unordered fighter pairs, but hit ownership, grabs, shields, and damage resolution remain directed. Preserve those semantics rather than treating “one pair result” as the whole interaction.

A compact scheduler is also possible, but only after documenting source phase order, same-tick spawning/removal, RNG consumption, and hit priority. A faster scheduler that changes those rules is not a transparent optimization.

Do not assume AI dominates just because four CPUs are enabled. Measure SCPU exclusively first.

8. Offloading: prioritize the host and fixed-function hardware before ARM7
Build-time computation has the cleanest opportunity

Asset interpretation, constant folding, native command generation, interpolation coefficients, static collision metadata, and immutable resource binding decisions should leave the DS whenever their inputs permit it.

This removes work without creating a second runtime processor dependency.

GX and the 2D engines should receive suitable native work

Use GX for qualified transforms, geometry, lighting, and texture generation. Use BG/OAM for HUD, digits, menu text, and screen-space layers whose priority and blending can be represented correctly.

Do not move an effect that must intersect a fighter into a fixed 2D layer and call it equivalent.

Likewise, Link’s existing texture-coordinate packet patch is already working. Hardware texture generation is a candidate only after validating its mapping against the source’s LookAt and normal/coordinate behavior—not merely because the DS exposes a similarly named feature.

DMA is not free parallelism

DMA cannot consume dirty ARM9 cache contents directly, and main-RAM transfers can interfere with CPU memory access. Include setup, cache maintenance, waiting, and bus contention in the comparison. A small CPU copy can beat DMA after those costs. TCM is not accessible to DMA or ARM7.

Also distinguish command-buffer lifetime from texture lifetime: DMA completing its command read does not mean a texture is no longer needed by rendering.

ARM7 should receive only independently useful jobs

Preserve audio, input, storage/wireless, and SDK services. Prefer coarse jobs with compact immutable inputs and no immediate ARM9 wait. Shared buffers need explicit ownership, publication, completion, generation, and cancellation rules.

Same-tick AI and collision are poor first candidates because communication or a frame of latency can eliminate the gain or change behavior.

Hardware divide/square-root pipelining is another bounded opportunity, but those units are already used. Any asynchronous schedule must account for other writers and interrupt/thread ownership; a result can be overwritten by another user.

9. Repack ITCM and DTCM around the smaller runtime

The intended sequence is:

Delete work → shrink kernels/state → measure → place → tune instructions.

Not:

Fill ITCM differently → hope the architecture becomes cheap.

For ITCM, keep tiny repeatedly executed emitters, animation/event kernels, and gameplay math where they deliver the best measured whole-frame benefit per byte. Separate cold binding and reporting from these kernels.

For DTCM, favor compact CPU-only mutable state repeatedly touched during a frame. Do not spend it on large immutable geometry or permanent synchronized mirrors.

The current linker’s qualified DTCM data ceiling is 12 KiB, not the entire physical 16 KiB. Increasing that ceiling requires renewed stack/layout proof.

Also, .text.hot and .text.hot.draw do not create separate hardware caches. Both compete for the ARM9’s instruction cache. The DS has 8 KiB of instruction cache and 4 KiB of data cache, making compact working sets important.

Only after these structural changes should hand-written assembly become a major focus.

10. Implementation order and proof requirements

The downloadable report expands each package into source anchors, deleted work, implementation steps, dependencies, gates, and stop conditions.

Package	Deliverable
WP0	Reproduce current baseline; establish exclusive costs and semantic acceptance
WP1	Separate ISA/residency policies; define shipping instrumentation boundary
WP2	Fixed event/pose producer-to-consumer vertical slice
WP3	Bound models and generated native GX packets
WP4	Static stage, particle, and UI execution reduction
WP5	Compact fixed gameplay and shared collision/AI query work
WP6	Remove remaining runtime float families and transitional representations
WP7	Final DTCM/ITCM packing and measured bare-metal kernel tuning
WP8	Optional offload experiments with an end-to-end timing verdict
WP9	Full content, memory, and four-CPU cadence closure

WP2 and WP3 should be coordinated early, with one owner for their shared native matrix/state contracts. A simple actor can prove the binding interface; a current hot fighter path must prove the performance case. Do not convert the whole roster before the pilot’s total cost and memory behavior are understood.

The measurements must prevent misleading wins

The existing scripts already distinguish important relationships: SRC contains GCRA, SINT contains SCPU, and WORK-H subtracts the HUD bracket. These cannot be added as independent costs. Removing the visible HUD cannot be credited a second time against WORK-H.

Measure integrated per-frame results and rerank all frames after substantial changes. Do not add independent P95 savings or permanently optimize only the old worst-frame set.

Keep both same-ROM route comparisons and a final hard-on shipping-build test. The recent mask work demonstrated that the location of experimental control code itself could perturb performance.

Use the project’s accuracy-focused melonDS workflow as its admitted development reference. New DMA/ARM7/bus-heavy paths may need additional calibration of that model; this audit did not independently verify its accuracy.

Finally, a fast frame must still contain four engaged fighters/CPUs, required effects, correct collision behavior, and complete native rendering. Missing work is not optimization.

Bottom line

The strongest direction is not “make the current generic machinery faster.” It is “stop running so much generic machinery.”

The largest architectural candidates are:

Authoritative fixed game/pose state, so conversions and duplicate representations disappear.

Load-time and state-change binding, so draws stop rediscovering immutable facts.

Generated native geometry and small dynamic patches, so the runtime stops translating source-shaped rendering state.

Compact gameplay data and shared queries, so four fighters do not repeatedly traverse cold, pointer-heavy structures to recover the same information.

ITCM reclamation then becomes a consequence of simplification, and the reclaimed space can accelerate the smaller remaining gameplay kernels.

The existing evidence does not yet prove a 60% reduction is achievable with these particular replacements. It does justify testing these larger changes instead of expecting another collection of small masks, caches, and placement tweaks to close a roughly 1.27-million-tick gap.