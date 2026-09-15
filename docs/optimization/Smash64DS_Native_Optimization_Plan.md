# Smash64DS: native-runtime simplification and four-fighter performance plan

**Research date:** September 15, 2026
**Repository:** `rockenrooster/Smash64DS_Port`
**Audited master snapshot:** `75f7f6b4b4864c82c01872d0fd2771d171005272`
**Purpose:** remove enough runtime work to sustain nominal 30 FPS with four active fighters/CPUs on the original Nintendo DS, while retaining the game's mechanics and required content.

## Scope and evidence standard

This is a source audit and synthesis of the repository's recorded measurements, checked against DS hardware and SDK documentation. It is not a newly measured optimization result. No new ROM was built or benchmarked for this report. Source inspection covered selected renderer, animation, gameplay-import, memory-layout and hardware-math paths, the current execution board and performance evidence, historical optimization reports, and concrete SM64DS reference implementations. It did not establish that every file is unnecessary or that every candidate executes in every match.

Findings below distinguish **observed source structure**, **repository-recorded measurements**, and **proposed engineering changes**. All proposed savings remain unbanked until the shipping implementation is measured. References [R1]–[R31] identify immutable repository paths; [H1]–[H7] identify hardware/SDK documentation. The source index is at the end.

## 1. Executive decision

The primary project should be **replacement of source-shaped runtime work with compact DS-native state and assets**, not another round of generic caching around that work. ITCM repacking should support the replacement, not substitute for it.

The target is not merely a renderer that eventually submits to the DS geometry engine. It is a runtime that does very little before submitting: data has already been compiled, ownership has already been bound, numeric representations already match, and only genuinely changed fields are recomputed.

The latest inspected hard-on four-CPU checkpoint reports the following. These are recorded values, not a fresh benchmark. [R2, R3]

| Metric | Recorded value |
|---|---:|
| WORK-H P50 | 1,680,384 ticks |
| WORK-H P95 | 2,389,376 ticks |
| Presented-frame cadence, 2 / 3 / 4 / 5+ VBlanks | 103 / 749 / 881 / 240 |
| Two-VBlank share | 103 / 1,973 = 5.22% |
| Native failures / direct rejects | 0 / 0 |
| General-heap low-water | 108,096 bytes |
| Heap above the report's safety floor | 82,496 bytes |

The existing scripts use a 1,120,380-tick gate, rounded to 1.12 million in the board. Therefore:

```text
Required reduction to the gate = 1 - 1,120,380 / 2,389,376 = 53.11%
A 50% reduction leaves          = 1,194,688 ticks
Remaining excess after 50%      =    74,308 ticks
Proposed internal target        =   950,000 ticks
Reduction to that target        = 60.24%
Headroom below the gate         =   170,380 ticks
```

The 950,000 target is a proposed design objective, not a prediction. A result barely below the limit is fragile when content, placement, storage latency or concurrent activity changes. The repository's cadence requirement must also pass; passing an arithmetic work budget alone does not establish presentation cadence. [R2, R4]

**Do not start from the premise that four-fighter support is absent.** The board distinguishes capacity, which has passed scoped tests, from performance, which remains red. Missing visual/native coverage elsewhere also means that future performance tests must prove actual content engagement rather than merely zero error counters. [R3]

### 1.1 Current requirements versus historical restrictions

The current project goal permits major native rewrites and does not require bit-identical numerical intermediates or pixels. It does require the intended mechanics, behavior, fidelity and native-only rendering. Some historical optimization notes contain more restrictive numerical or collision rules; those should not silently overrule the current project goal and the owner's request for fixed-point runtime. [R1, R10]

Resolve this in implementation documents before work starts: test mechanics, event timing and bounded numerical error where appropriate; do not require an obsolete floating-point representation merely because an old test used it. Conversely, permission to change representation is not permission to remove fighters, reduce hit-test frequency, change CPU reaction timing, or omit difficult effects.

## 2. What already works and must not be sold as a new optimization

The inspected checkpoint already retains native fighter packet replay, split-root replay support, Link texture-generation packet patching, direct BGM/FGM range reads, resident/early BPS1 animation-directory work, compact pose tracks and active-track/running-joint masks. Fixed camera math, hardware division/square root and previous ITCM reclamation also exist. [R2, R3, R10, R21, R29, R30]

The latest running-joint-mask same-ROM experiment improved whole-match WORK-H P95 by **9,408 ticks**, from 2,366,528 to 2,357,120. That is useful, but it is not the scale of change now required. The hard-on shipping-shape measurement was separately reported at 2,389,376 P95. Do not mix these experimental identities. [R2]

The next campaign should retire broader work categories: repeated source-asset interpretation, conversion bridges, redundant state authority, large mutable traversal scaffolding, cold resource work inside draws, and generic game-object processing where a compact exact-order representation can replace it.

## 3. Evidence-backed problem map

### 3.1 ITCM is nearly full, but cold in one trace does not mean dead

A September 15 diagnostic census preceding the final running-joint-mask checkpoint reports `.itcm` at **32,640 bytes**, with 92 named residents and **4,650 bytes across 20 residents not executed in that window**. The linked usable region is smaller than the physical 32 KiB because of the reserved prefix. This census is evidence for candidate investigation, not the exact current shipping ELF or proof that those 20 functions are globally dead. [R5, R6]

Named residents include `ndsRendererCommitNativeStageSegment` at 2,820 bytes and `ndsRendererNativeStageBeginRun` at 2,352 bytes. Together those two bodies occupy 5,172 bytes in that census. They are candidates for separating persistent bindings and cold setup from tiny execution kernels, not automatic eviction candidates. [R5]

Never evict interrupt, cache-maintenance, startup, rare-state or another-roster code merely because a 128-frame combat window did not execute it. Even true ITCM eviction is not source deletion: a necessary cold function can reside in main RAM.

### 3.2 Source organization still couples instruction choice and residency

`linker/nds_hot_text.ld` contains special handling for `*.32.o`, including text/rodata placement in ITCM and data/BSS placement in DTCM. Meanwhile, renderer macros explicitly assign functions to shared `.itcm` and `.itcm.native_fighter` input sections. Those policies make unrelated code and data compete as groups. [R6, R7]

An output linker section can combine input sections, but it cannot independently evict functions that the compiler has put into the same input section. `-ffunction-sections` does not solve an explicit shared-section attribute. Use unique kernel sections and inspect the actual ELF, including aliases, padding, veneers and literal pools.

`nds_renderer.c` includes implementation `.c` files textually. Splitting text into more files has not necessarily produced separate translation units or clean runtime interfaces. A genuine module boundary should own small native inputs and outputs; it should not export a sprawling mutable traversal state. File count alone is not a speed metric. [R8]

### 3.3 Native output still has expensive source-shaped preparation

The inspected production fighter path invokes `ndsRendererNativePreflightProductionOwner` before attempting `ndsFighterPacketTryReplay`. That establishes the call order, not the entire price or redundancy of the preflight: some internal checks may already be cached. Use its existing preflight timing counter to establish what remains. [R11]

The shared native code also contains packed GBI triangle-index decoding, eligibility checks across many renderer flags and per-slot preparation of colors and texture coordinates. Reachability and dynamic frequency must be verified for each proposed deletion. The structural opportunity is to generate immutable topology/material decisions offline rather than re-derive them through generic state. [R12]

A particularly concrete example is `ndsRendererSubmitNativeTaruCann`, a two-joint textured-quad actor. The inspected path constructs generic configuration/traversal state and vertex structures, validates hierarchy relationships and affine matrix structure, prepares texture state and converts matrices during submission. Much of a fixed actor's immutable work could be bound at load or spawn time. This is evidence of an over-general execution interface, **not evidence that the barrel is the dominant four-CPU benchmark cost**. [R13]

### 3.4 Software float remains, including float implemented with integer instructions

The diagnostic census includes `__aeabi_fadd`, `__aeabi_fmul` and `__aeabi_fdiv` among significant executed symbols. These three alone do not represent the full floating-point cost: conversions, compares, caller code and data-format bridges also matter. Conversely, their recorded self-times do not justify claiming that float removal alone halves the frame. [R5]

The pose engine uses fixed-point pose values alongside DObj floating-point fields and an integer implementation of exact binary32 clock arithmetic. The absence of an FPU instruction or a literal `float` spelling is therefore not proof of a native fixed-point pipeline. [R14]

The goal should be **one authoritative native representation from producer to consumer**, not `float -> fixed -> float -> fixed` chains. Existing fixed helpers and hardware-assisted IEEE helpers are useful transitional components, but they are not the requested endpoint.

### 3.5 Dynamic topology is a real invalidation boundary

`ftMainSetStatus` can change grab/throw-related DObj topology without changing the fighter's root pointer or heap generation. The import wrapper explicitly invalidates both its flat-walk and renderer caches because gameplay attachment transforms can otherwise become stale. [R15]

Any replacement binding, pose closure, matrix cache or packet key needs a topology generation or equivalent precise ownership mechanism. “Root pointer unchanged” is not a sufficient validity test. Correct simplification removes repeated discovery by assigning lifetime ownership; it does not remove the lifetime rule.

### 3.6 Instrumentation and percentile attribution can mislead

The repository's decomposition states that SRC contains GCRA, which contains SINT, which contains SCPU. WORK-H is derived per frame as WORK minus HUD. These buckets and their separate percentiles cannot be added together. Removing the visible HUD cannot be credited a second time against a metric that already subtracts its bracket, although its layout/cache perturbation still merits a shipping-build check. [R4]

The census also charges substantial time to `armWaitForIrq`. Waiting for the intended presentation deadline is not wasted game computation. Classify idle, blocked I/O, GX backpressure, cache stalls and executed game work before ranking opportunities. [R5]

### 3.7 No single small symbol explains the required reduction

The trace has important costs spread across animation, matrix handling, packet preparation, memory operations, stage emission, particles, software float and game code. This argues for removing shared infrastructure and repeated passes. It does not prove the exact distribution in the latest hard-on frame tail. Rebuild that distribution once, then use it to price concrete alternatives rather than repeatedly collecting an unbounded census. [R5]

## 4. What to borrow from SM64DS

The useful reference is the original DS-oriented execution pattern recovered in `src/`, not every modern hosted implementation under `port/`. For example, a host BMD renderer using vectors of floating-point matrices is not evidence that the DS game uses that representation. Decompiled field names and partially reconstructed routines also need provenance checks. [R22, R28]

| Inspected reference | Demonstrated pattern | Application to Smash64DS |
|---|---|---|
| `Model::UpdateFileOffsets` | Rebase model, texture, palette and display-list offsets during loading | Resolve and validate immutable asset structure before the match |
| `BMD_File.h` | Flat counts/tables and packed DS display-list records | Make the asset format directly consumable by the native backend |
| `CrossVec3` | Explicit 12-fraction-bit integer vectors, wide products and rounding | Choose native numeric formats and propagate them through consumers |
| `Camera_UpdateMatrices` | Fixed-point vectors, lookup-based angles and native matrix construction | Remove residual camera/billboard conversion seams; do not reimplement already-fixed camera work |
| `ModelAnim::SetAnim` | Same-file fast path updates flags/speed without full rebinding | Make state changes own preparation rather than repeating it each draw |
| `dBgCh_Gnd::DetectClsn` | Bounded actor set and cheap rejections before narrow work | Use bounded small-world collision/query structures rather than an elaborate general engine |

These examples are directly represented in the inspected files. They are patterns to adapt, not drop-in substitutes for Smash's transform semantics or collision rules. [R22–R27]

The lesson is **do expensive interpretation once, keep live state small, and store data in the form that the hardware and game logic actually consume**. It is not “all C++ is slow,” “all branches are bad,” or “copy this game's entire engine.”

## 5. Target runtime architecture

```text
HOST / BUILD
  original assets and behavior descriptions
       -> validate and classify
       -> fixed numeric constants and compact tracks
       -> native GX geometry/material packets
       -> event schedules, dependency closures, bounds and patch tables

SCENE LOAD / SPAWN / TOPOLOGY CHANGE
  load selected scene + fighter banks
       -> range-check, relocate, upload and bind once
       -> reserve bounded live pools
       -> publish native handles + precise generations

LOGIC UPDATE
  compact fixed fighter state
       -> source-ordered events and decisions
       -> physics / collision / required joint sockets
       -> changed-state masks

PRESENTATION UPDATE
  visual pose and material changes
       -> small fixed matrix/visibility/texture/alpha patch sets
       -> native command submission
       -> explicit DMA, geometry and rendering lifetime boundaries
```

This does not require a new general-purpose engine, entity framework or intermediate virtual machine. It requires fewer live representations and fewer passes. A packed **DS hardware command list** is the target format; a new runtime N64 display-list interpreter is not.

### 5.1 Suggested ownership contracts

Use names appropriate to the current tree; the following are conceptual contracts, not proposed compulsory class hierarchies.

**Immutable model bank:** geometry, compact parent indices, material identities, draw order, transform classes, local bounds, static channels, required-joint dependencies and offsets into the bank. Its format is validated once and versioned.

**Bound instance:** selected model variant, resolved texture/palette handles, topology generation, material generation, geometry references and small typed patch tables. Resource ownership and VRAM-generation checks are explicit.

**Live fighter state:** fixed world motion, status, event clock, damage/flags/input, compact active collision state and required sockets. Cold descriptions and debug history live elsewhere.

**Presentation state:** pose generation, native matrices, visibility, colors and dynamic texture coordinates. Camera-only changes must not force a complete pose re-evaluation. Costume-only changes must not rebuild geometry.

**No permanent duplicate authority:** during migration a bridge may be required, but completion of a domain includes deleting its old authoritative representation and bridge. A permanent second state mirror that must be synchronized every frame is not the final design.

### 5.2 Change ownership instead of hashing everything

A mutation that can invalidate a binding increments the relevant generation or rebuilds the affected descriptor. Examples include status-driven reparenting, model/detail selection, Kirby copy attachment changes, Samus morph changes, material animation changes and VRAM eviction.

Do not collapse every change into one global dirty flag: a palette update should not invalidate joint topology, and a moving camera should not invalidate model-local animation. Also do not replace one giant hash with dozens of speculative caches. Start with the smallest set of generations that the actual producers can maintain correctly.

## 6. Renderer simplification campaign

### 6.1 Bind immutable properties before entering the hot draw path

For each admitted fighter or actor model, validate asset ranges, hierarchy relationships, material count, native program identity and constant transform properties at load/spawn/topology change. Produce a small bound descriptor. A draw consumes that descriptor and dynamic state; it does not rebuild a source-like configuration just to recover those properties. [R11–R13, R15]

Keep runtime checks for facts that can actually change: ownership/generation, packet length, available buffer space, live texture residency, dynamic counts and valid dispatch. Move exhaustive immutable validation to host generation and binding. Bad asset input must fail clearly at admission rather than turn into a slow fallback or missing geometry.

**First instrumented question:** how much of `ndsRendererNativePreflightProductionOwner` still executes on a packet hit, and which predicates are immutable for the lifetime of the bound instance? Price that exact set. Do not assume all preflight work is redundant.

### 6.2 Replace first-use recording with generated native templates where profitable

Packet replay already exists. The next step is not merely “cache the draw.” It is to determine whether the host can produce the packet, patch locations and bounds directly, eliminating first-use recording, cold path scaffolding and repeated key construction.

Factor the representation into immutable geometry plus a small dynamic patch set. Patch matrices, object transforms, visibility, polygon attributes, selected material words and genuinely dynamic texture coordinates. Do not generate the cross-product of every pose, camera, costume, material frame and effect state.

Choose packet ownership deliberately. One writable packet per active instance may avoid a full-frame copy, but costs resident RAM and must not be patched while DMA is reading it. A shared immutable packet with separate dynamic runs reduces RAM but may increase submission overhead. Measure both with buffer lifetime and cache maintenance included; the DS does not provide a free scatter-gather abstraction that makes arbitrarily many tiny chunks costless.

### 6.3 Preserve unusual source transform semantics explicitly

The matrix adapter contains genuine semantic differences: joint-attached billboards, orientation replacement while preserving translation, scale accumulation and camera-dependent recalculation. A single generic TRS rule is not sufficient. [R16]

Classify these into a small set of native transform kernels at generation/bind time. Use affine 4x3 matrices for affine bone/world transforms; retain a proper perspective representation where necessary. Pre-scale immutable geometry and choose coherent world units so each draw does not copy a full matrix merely to shift its translation terms.

Avoid duplicating both complete CPU world matrices and the same hierarchy work on GX. Compute CPU matrices only for required gameplay consumers and any source semantics that cannot be expressed by the chosen GX path. Do not read GX matrices back to obtain same-tick hitboxes; that turns offloading into a synchronization dependency.

### 6.4 Reduce material and texture machinery, not its correctness

Use pre-resolved native register words where texture allocation and material interpretation are immutable. Recompute only fields owned by material animation, palette changes, lighting, visibility or resource-generation changes. A live texture cache can still manage residency; its lookup and allocation paths should not be the default cost of every draw.

Link already has a live texture-coordinate packet patch. A hardware normal-based texture-generation route is an experiment only after proving its mapping against the N64 LookAt, normal transform, scaling and wrapping behavior. A similarly named DS feature is not proof of equivalent output. Keep the current functioning path as the measured baseline until replacement is qualified. [R29, H3]

### 6.5 Handle depth and order as native render classes

Separate normal depth-tested opaque geometry, qualified cutout geometry, translucent geometry, source painter-order/no-Z layers and special billboard/depth behavior. Resolve constant state transitions offline. Do not globally sort by material or force all geometry into one depth mode: that can fix CPU cost by breaking shields, intersecting effects, stage overlays or transparency.

The existing no-Z triangle and stage commit functions appear in the census and deserve targeted pricing. The question is whether a generated native depth/order policy can remove repeated CPU projection and state work for a qualified class. It is not a license to delete the source's depth semantics. [R5, R16]

### 6.6 Make stage and actor paths share a small executor, not large generic state

A static stage chunk should usually require bounds selection, its current transform/material state and a native packet submission. The two-joint barrel example is a good pilot for the binding interface because its immutable topology is easy to prove. Then apply the same small executor to actors whose source transform class is compatible. [R13]

Generated assets should usually be data, not repeated large C executors per fighter/actor. Keep a finite number of specialized kernels for genuinely different cases. Measure code growth as additional characters and stages are enabled; linear copies of a generic executor are not useful specialization.

## 7. Native fixed-point, animation and event timing

### 7.1 Convert complete domains

The migration unit is a producer/consumer chain, not a math helper. A strong first vertical slice is:

```text
native motion clock and authored events
    -> compact fixed pose
    -> fixed local/world transforms where required
    -> native hitbox/socket consumers
    -> native render matrices and patches
```

Follow with world physics/collision/AI, material/particle state and remaining menus/UI/startup paths. Floating-point computation can remain in offline tools and host-only reference tests. The requested endpoint is no floating-point arithmetic on DS runtime paths, including cold scenes; an integer routine emulating IEEE arithmetic does not count as completing this endpoint.

During migration, each remaining bridge must have an owner, call count and deletion condition. An isolated fixed helper that immediately converts its result back to float can lose to the original routine. Historical project measurements already document that failure mode; their exact cycle costs are historical rather than universal constants. [R10]

### 7.2 Numeric formats must follow ranges, not fashion

Create a checked format inventory before changing field definitions. For each field record physical/source units, minimum and maximum admitted values, fractional precision, rounding policy, overflow policy and all consumers.

| Domain | Starting design to evaluate, not a universal mandate |
|---|---|
| World positions and velocities | Signed 32-bit fixed; choose fractional bits from complete world/motion bounds |
| Affine matrices | Native signed 32-bit entries with 12 fractional bits where compatible with GX |
| Angles | Compact integer turns or another explicitly defined cyclic integer scale |
| Authored event times | Integer ticks or a defined rational accumulator, separate from visual interpolation |
| Colors, alpha, IDs, masks | Bounded integers, not floating-point containers |
| Normals and texture coordinates | Native packed formats or compact fixed intermediates with explicit conversion boundaries |
| Products and accumulated dot products | Wide intermediates only where proven necessary |

Do not blindly adopt 16.16 for everything. Do not confuse matrix range with vertex-command range. A correctly represented world position can still overflow a packed DS vertex field unless world/model scaling is designed with the asset compiler. [H3, H4]

Use explicit narrowing checks in host tests, defined negative rounding, and range proofs for multiplications and divisions. Avoid signed-overflow undefined behavior and negative signed shifts whose semantics are not the intended numerical operation. Specify the supported compiler behavior where a deliberately low-level kernel depends on it. Do not saturate gameplay values silently just to pass tests.

Replacing every operation with 64-bit arithmetic is not optimization. Choose narrow kernels where bounds permit, use wider products for dot/matrix accumulation, and precompute reciprocals or constants when that preserves the required rule. Inspect emitted code for unexpected divide/conversion helpers. Global `-ffast-math` and a soft-float ABI option are not substitutes for native representation.

### 7.3 The animation clock requires its own design

The pose source documents a real failure of naive Q12 timing:

```text
round(4096 / 3) = 1365
3 * 1365       = 4095
wait = 4096 therefore remains positive after three updates
```

An event can occur one logic tick later. The repository records 635 mismatching cases across 44 speed sources in its earlier differential test, including actual Mario/Fox move timing differences, and explains why merely raising fixed precision did not eliminate every binary32 boundary difference. That motivated the current integer-coded IEEE clock. [R14]

Replace that mechanism with a **native event-clock specification**, not a global epsilon. For example, represent a known speed as numerator/denominator with remainder carry, or compile event deadlines for the admitted rate classes. Preserve loop, pause/hitlag, attach/end, interruption and speed-change semantics.

Crucially, exact rational arithmetic is not automatically identical to a chain of rounded IEEE operations. Generate or test boundary corrections against the required event behavior where necessary. Arbitrary speed changes must have a specified conversion of residual phase. A separate visual interpolation clock can be approximate within its accepted tolerance; the event schedule must not silently move attack starts, landing recovery or status transitions.

Move expensive source-event analysis into the build tools. The runtime should process a compact event stream and update a small clock state. Do not add a large interpreter to avoid a small IEEE helper. Compare the complete event-clock plus pose path, not only the helper that disappeared. The source comment gives only a historical approximate cost for the integer IEEE fix; that is not evidence that this helper alone is a major current bottleneck. Its removal is part of the native endpoint, while the performance case must come from the complete producer/consumer simplification.

### 7.4 Separate required gameplay joints from presentation joints

A fighter's root, active hit/hurt volumes, grab/throw attachments, held items, weapon sockets and any other gameplay consumer must be current at the required simulation rate. Visual-only bones can follow the presentation schedule when the project's fidelity rules permit it.

Compute the ancestor closure of gameplay-required joints at bind time. Recompute that closure when topology or active gameplay dependencies change. Do not assume that off-screen or hidden geometry has no gameplay consumers.

The existing implementation already supports held body evaluation and active masks, so this proposal is broader: eliminate duplicated transforms and unnecessary consumer representations, not merely skip inactive tracks again. [R2, R14, R15]

### 7.5 Use compressed tracks and selective precomputation

Useful offline products include constant-channel masks, compact keyframes, precomputed interpolation coefficients, angle lookup data, fixed material frames, event tables, static local transforms, parent/dependency tables, conservative bounds and qualified collision/socket templates.

A full matrix bake has a clear memory trap. An illustrative one-second block with four fighters, 32 joints each, 60 samples and 48 bytes per affine matrix costs:

```text
4 * 32 * 60 * 48 = 368,640 bytes = 360 KiB
```

That exceeds the inspected run's 108,096-byte heap low-water, before accommodating other live demands. This calculation uses illustrative joint/sample counts, not a measured current pose-bank size. Conversely, two compact 16-byte keys per joint in that example would require 4,096 bytes, plus descriptors and other state. This illustrates why selective representation matters; 16 bytes is a design example, not a claim about the current format. [R2]

Prefer constant channels, compact keys, resident active-clip working sets and bounded coefficient evaluation. Large precomputed banks can live in storage, but an unpredictable combat state cannot require an unbounded blocking read to make its next hitbox valid. Budget and admit the required live working set before GO.

Do not bake final collision results that depend on live world position, scale, facing, damage state, attachments or procedural motion. Bake the invariant local portion and apply the dynamic transform. Interpolation and compression errors need bounds for the gameplay consumers, not only a screenshot test.

## 8. Simulation, collision, CPU decisions and scheduling

### 8.1 Make four-fighter state compact without building a new general framework

The current import wrappers still bring in substantial BattleShip fighter and CPU logic, with floating-point status/animation interfaces and source process structure. [R15, R17]

A suitable destination is a bounded four-fighter hot-state array with cold descriptions separated. Keep frequently co-accessed position, velocity, status flags, input and small collision summaries near each other. Choose array-of-structures versus structure-of-arrays from actual co-access patterns; neither is universally faster.

Items, weapons and effects need their own bounded live lists or pools. Pooling should preserve the admitted content capacity and spawn semantics. Reducing a weapon cap or quietly dropping effects is not an optimization result.

Avoid adding an ECS, job system, generic serialization layer or polymorphic query interface merely to optimize a small fixed roster in one match. Compact typed loops are the baseline to beat.

### 8.2 Replace redundant query work before changing behavior

Build shared world facts once per logic tick where consumers currently rediscover them: fighter positions, conservative bounds, eligible targets, relevant stage-line neighborhoods, facing and distances. Let multiple CPUs consume those facts instead of rebuilding equivalent structures independently.

Keep direction-sensitive information explicit. Four fighters produce six unordered pairs, but attack ownership, hit priority, shields, grabs and damage resolution still require their proper directed semantics. A shared broad-phase result cannot collapse distinct narrow-phase rules.

Price SCPU exclusively before making AI the main rewrite. Four active CPUs do not imply that AI decisions dominate the frame. The current wrapper contains harness controls that can suppress a CPU for visual investigation, so performance capture must also prove all four CPUs are engaged. [R17]

Do not halve decision frequency or move decisions one frame late under the label of offloading. Those are behavioral changes, not transparent optimizations.

### 8.3 Compile stage collision metadata

For static collision, compile floor/wall/ceiling classification, line extents, neighbors, normals or coefficients and inexpensive candidate ranges. Select a structure appropriate to each stage's actual line count: a short ordered list or small bins may beat a complex tree.

Use cheap conservative rejections before expensive collision math. Reuse candidates while the fighter remains in the same valid neighborhood, but invalidate on movement across its bounds, moving-platform updates, hazard changes or a relevant topology/state transition. Preserve one-way platforms, edge/ledge cases, grazing contacts, slope behavior and the original accepted collision ordering.

For fighter/weapon collision, retain all dimensions and special rules that matter to the original game. Calling the game 2.5D is not permission to discard a depth-dependent condition. Squared distances avoid square roots only when the same ordering and range guarantees hold.

### 8.4 Simplify the scheduler only with an ordering specification

A source engine's process list carries semantics: status interrupts, animation events, physics, map collision, hit/catch searches, parameter updates and newly spawned/ejected objects can interact within one logic update.

Before replacing generic GObj/process traversal with typed phase loops, record that order and its mutation semantics. Preserve stable ordering where it affects RNG consumption, attack priority, target choice or same-tick spawn behavior. A compact scheduler that changes those details can make the game faster by making it a different game.

A useful test is an event log for selected controlled scenarios: update phases, status transitions, input samples, RNG call sequence, collision candidates and accepted hits. Compare meaningful events rather than requiring all temporary fields to retain old binary layouts.

## 9. Stage, particles, UI, audio and storage

### 9.1 Static stage work should scale with change, not source graph size

Keep immutable geometry, local transforms and state transitions in generated banks. Update only moving actors, animated materials and camera-dependent state that actually change. Separate camera motion from stage-local animation; a camera change should not cause full source hierarchy rediscovery.

Test stage-specific transparency, source no-Z layering, dynamic platforms and camera-dependent actors. Previous or current visual failures must remain in the regression suite so a faster but incomplete stage does not become the new baseline. [R3, R16]

### 9.2 Particles need both a render plan and a simulation plan

Render compatible particles as native batches with shared state and compact per-particle data. Precompute invariant quad corners, material selection and curve coefficients. Reuse a current camera/billboard basis where semantics permit, but recognize that a particle-camera cache already existed in earlier work; establish what remains rather than adding another cache around it. [R10]

Do not skip emitter simulation, RNG calls or gameplay-affecting effects when changing visual cadence. Culling a draw is different from culling an object's state update. Preserve ordering and depth behavior for effects that intersect fighters.

### 9.3 Move suitable UI layers to the DS 2D engines

Use BG/OAM for HUD elements, digits, static menu text and qualified screen-space layers when their blending and priority requirements can be represented. Update digit tiles or formatted text when the value changes, not through a general formatting path every frame.

Do not move an effect that must geometrically intersect a fighter into a fixed 2D priority layer and call it equivalent. UI/2D engine usage is an ownership decision constrained by the required composition, not a blanket solution for all quads.

### 9.4 Audio and storage: retain the direct-read improvements and attack remaining tails

Direct BGM and FGM read routes are already retained. Keep their range validation, ownership and fallback/cursor correctness while simplifying surrounding work. Do not rebuild an audio offload system before measuring the remaining AUD and storage tail costs. [R2, R30]

Animation first-use misses, clip binds, file metadata discovery and resource uploads can determine bad-frame cadence even when their average is small. The next steps are bounded residency and scheduling: identify the critical working set, validate contiguous/range reads where supported, prepare descriptors outside the hot frame, and reuse known offsets.

A prefetch prediction is not a correctness guarantee. Mandatory data for an immediate state transition must be resident or have a bounded fallback that still meets the frame and behavior contract. A slow generic renderer is not an acceptable fallback.

### 9.5 Use memory savings strategically

Delete redundant source/native representations, diagnostic histories and cold asset residency before adding large lookup or matrix banks. Use scene arenas and explicit resource lifetimes where that reduces fragmentation and repeated allocation.

Cold scene or code overlays are a possible RAM project only after measuring the call graph and ownership boundaries. They should not introduce card reads on ordinary combat status transitions or callbacks into unloaded code. Original-DS operation should not depend on DSi memory or a Slot-2 expansion unless the owner explicitly changes the target.

## 10. CPU offloading: choose work that actually leaves the critical path

The most dependable order to investigate is build-time computation, existing fixed-function graphics/2D hardware, carefully batched transfer, then independent ARM7 jobs. This is a proposed priority based on the architecture, not a measured ranking of current savings.

| Destination | Candidate work | Required proof |
|---|---|---|
| Host build tools | Asset interpretation, constant folding, native packet generation, track coefficients, static collision metadata | Output size, admitted-format bounds and unchanged required semantics |
| GX | Native vertex transformation, qualified hierarchy composition, lighting and texture generation | Mapping fidelity, matrix/geometry capacity, no synchronous gameplay readback |
| BG/OAM | Qualified HUD/menu and screen-space layers | Equivalent priority, blending and resource capacity |
| DMA | Larger native command or upload streams | Channel ownership, cache cost, source/destination visibility, actual wall-time improvement |
| ARM7 | Bounded independent service or preparation jobs with compact input/output | Slack after existing services, communication cost, bus contention, deadline and cancellation safety |

### 10.1 DMA is not free parallelism

DMA and ARM9 can contend for main RAM, and DMA does not consume dirty cache lines directly. Benchmark transfer plus synchronization/cache maintenance, not only the transfer instruction. TCM cannot be a DMA source or destination. A small CPU copy can beat DMA after setup and coherency costs. [H1, H2, H5]

Design CPU-only work islands with hot code/data independent of the DMA buffer where overlap is possible. Avoid scattering many tiny DMA jobs through a traversal loop. Maintain an explicit channel allocation for GX, audio/other existing services and resource uploads.

Distinguish three lifetimes: a command source buffer is reusable once its DMA reader is finished; geometry processing has its own completion state; textures and palettes must remain valid until their rendering consumers are finished. A DMA-complete flag is not a universal frame/resource fence.

### 10.2 ARM7 is a separate service processor, not an RSP replacement

Keep audio, input, storage/wireless and SDK services functioning. Do not assume the ARM7 has an entire spare core's budget available, and do not move ARM9 floating-point code there expecting a speedup. Prefer coarse, independent jobs that consume a small immutable snapshot and can complete without the ARM9 waiting immediately. [H6]

Use bounded message queues and shared buffers with request IDs and scene generations. Specify producer, consumer, publication, cache maintenance, completion and reclamation. A timeout does not stop an old writer; require cancellation acknowledgement or quarantine the buffer before reuse.

Same-tick AI and collision are weak first candidates because communication or a frame of latency can erase the gain or change behavior. Prove independent slack and measure end-to-end cadence before retaining an offload.

### 10.3 Hardware divide/square root: improve scheduling, not just calls

These units are already used in the port. Where a loop has unavoidable independent operations, starting a division/root and doing unrelated work before collecting its result may reduce polling. There is only one result stream per hardware unit, so another user can clobber an outstanding operation. [R21, H7]

Re-audit all writers and any interrupt/thread usage before pipelining calls. The repository's existing ownership proof is tied to a particular binary, not a permanent hardware guarantee. Its SM64DS reference discussion notes save/restore of math state during thread-context changes. Retain this ownership discipline rather than adding a large critical section to every scalar operation. [R21]

## 11. ITCM, DTCM, code size and bare-metal kernels

### 11.1 Reclaim precisely, then admit by measured value

The ITCM campaign should produce an input-section ownership report: address, emitted bytes, aliases, alignment, literal pools, veneers, caller families, supported-scenario reachability and measured marginal benefit. Do not sum alias sizes or assume a symbol can be removed independently of its input section.

Replace broad residency macros with explicit unique kernel sections. Separate instruction-set selection from placement. Move load/bind/error/reporting code to cold main-RAM functions without silently changing its behavior. Keep hot math fast until its callers have actually been retired. [R6, R7, R10]

After renderer simplification, place the remaining small emission kernels, compact animation/event operations and collision/gameplay kernels by measured benefit per byte. Include the opportunity cost of any eviction. A raw cycle/byte ranking can favor idle or hardware-wait code; a stall estimate is not a guarantee that placement removes the stall.

A renderer byte target may be useful as a design pressure, but do not impose an arbitrary partition that evicts a valuable renderer kernel to admit less valuable game code. The product target is lower whole-frame cost, not a particular subsystem's percentage of ITCM.

### 11.2 Treat DTCM as a separate optimization problem

The inspected linker asserts `__dtcm_bss_end <= 0x02ff3000`: the qualified data ceiling is 12 KiB under the existing stack/layout proof, not all 16 KiB of physical DTCM. The runtime's actual thread-stack placement also matters; do not import a generic SDK stack assumption into this linker. [R6, R9]

Candidate DTCM occupants are compact repeatedly accessed CPU-only state: selected pose/event state, dense fighter hot fields, small collision scratch and a small native draw-instance state block. Rank measured repeated access savings per byte. Do not place large immutable geometry or DMA/ARM7-shared buffers there. Keep one authoritative state copy rather than paying to synchronize a main-RAM mirror permanently. [H2]

Re-measure existing residents before moving anything: the space is not empty, and the historical data-placement campaign is not a current occupancy report. Never increase the data ceiling without repeating stack and reset-path safety tests.

### 11.3 Reduce instruction and data working sets together

The DS ARM9 has an 8 KiB instruction cache and a 4 KiB data cache. The repository's `.text.hot` and `.text.hot.draw` labels do not create separate physical instruction caches. Shrinking a frequently alternating set of functions can matter more than making one function faster in isolation. [R6, H1]

Test ARM/Thumb and size/speed optimization choices per meaningful kernel or module. ARM long-multiply capability can help fixed-point math, while compact Thumb or size-oriented code can reduce fetch cost elsewhere. Inspect disassembly rather than assuming blanket ARM or `-O3` is optimal. [H1]

Avoid over-inlining large helpers into multiple call sites and over-aligning every small field. Prefer compact hot/cold layouts with naturally aligned accesses. A 4-fighter compact state block or a short active list is more promising than a broad rewrite into many tiny abstraction objects.

Only write assembly after the runtime design and layout are stable and a kernel remains hot. Suitable experiments include bounded fixed dot/matrix operations, small packet patches and proven common-case emitters. Keep portable reference implementations in host tests, and verify overflow/rounding as well as speed. Do not introduce instruction extensions unavailable to ARM946E-S or assume modern SIMD.

### 11.4 Remove diagnostic overhead without destroying observability

Classify diagnostics into compile-time-out shipping traces, small retained safety witnesses and validation-only detailed traces. Many current imports already guard telemetry correctly; do not credit their removal when they are absent from the shipping binary. Some pose and actor witnesses are unconditional and merit an emitted-code/call-frequency census. [R13, R14, R17]

Keep the ability to detect missing content, invalid ownership, buffer overflow and unsupported native paths. Replace per-element volatile writes with bounded local aggregation or host-side tracing where appropriate. Confirm the shipping ELF contains neither an accidentally linked HUD/trace subsystem nor a required guard that has been optimized away.

Remember that WORK-H already subtracts HUD. Observer removal can improve the actual build and reduce perturbation, but it cannot be booked twice against the reported gate. [R4]

## 12. Performance proof and acceptance

### 12.1 Establish a repeatable baseline, not a new profiling framework

Use the existing harness, tick-HUD decomposition and recorded scenario as the starting point. Freeze source SHA, ROM hash, build flags, toolchain, emulator revision/settings, input/seed, stage, fighter identities, CPU settings, items and measurement window. Verify the four active fighters and required effects actually perform work.

Reproduce the latest hard-on result before replacing it. Also record a low-instrumentation shipping configuration. The custom accuracy-focused melonDS fork is the project's admitted development reference; use its required interpreter/JIT settings. New DMA, ARM7 or bus behavior may require extending its existing calibration rather than assuming accuracy transfers automatically from a CPU-only workload. This report did not independently verify the fork's calibration. [R1, R3]

### 12.2 Derive exclusive costs and two different failure sets

For each frame retain the whole work result, presentation interval, exclusive subsystem times, resource events and useful work counts. Follow the repository's nesting rule: take a parent bucket or disjoint children, never both. [R4]

Investigate both the costly-work tail and the frames near the missed-presentation boundary. They need not be the same set. After a large optimization, recompute the ordering over **all frames**; the old worst-frame set is not permanently authoritative.

Track P50, P95, P99, maximum, threshold-overrun fraction, two-VBlank fraction and consecutive missed-frame clusters. Keep the official gates intact. A proposed tighter engineering target can guide headroom, but it must not be confused with an already passed product gate.

### 12.3 Separate logical work, hardware waits and instrumentation

Distinguish CPU instructions, instruction/data-memory stalls, synchronous I/O, GX/DMA backpressure, intended VBlank waiting and observer overhead. The profiler's cycle counters and the game's timer ticks are not automatically the same unit. State the conversion where a comparison uses both.

Do not carry an old fixed apparatus subtraction into a newly instrumented build without measuring its validity. Do not exclude unpleasant frames from a publishable percentile. Attribution-only experiments must be labeled separately from release gates. [R4]

### 12.4 Use same-ROM tests where useful, but also qualify the final binary

The September 15 mask experiment shows why this matters: putting the historical control in the hot loop perturbed its P95; moving the control to a cold helper yielded a better controlled route comparison. The final hard-on configuration was tested separately. [R2]

Keep route selection outside the repeated kernel when possible. Then remove the experiment route and measure the final linked layout. For placement changes, hold code generation stable when testing placement itself. For architectural changes, measure the complete new owner rather than just one fast leaf.

### 12.5 Correctness tests must reflect the permitted rewrite

Bitwise host comparisons remain valuable for changes intended to be bitwise equivalent. For fixed/native changes where representation is intentionally different, test the properties that matter: authored event timing, status transitions, input sampling, movement envelopes, collision/ledge edge cases, hitlag, hit priority, grabs/throws, projectile timing and required visual behavior.

A deterministic match can diverge after small authorized numeric differences, changing its later workload. Use both long natural matches and controlled, repeatable stress scenarios. A single matching global state hash is not the only valid oracle, and a different hash is not automatically evidence that mechanics are wrong.

Retain targeted regression cases for shields, intersecting impact waves, stage transparency/order, CSS selected poses, dynamic attachments, copy hats, morph states, first-use animation, simultaneous KOs and rematches. Source-only native rejection counters do not prove that every expected visible object was submitted.

### 12.6 Prove a deletion budget before promising the total

At 2,389,376 P95, the reduction required to meet the existing gate is 1,268,996 ticks. Use exclusive per-frame attribution and controlled replacement/ablation to bound what each candidate can remove. An ablation that omits work is only a diagnostic upper bound, never a qualified game optimization.

Do not add independent package P95 deltas as though they commute. They can overlap in the same code and change cache behavior or frame ranking. Combine actual per-frame results, integrate, then rerank.

A useful accounting identity is:

```text
Frame work = shared logic + per-fighter logic + interactions
           + required pose/sockets + presentation preparation
           + native submission + non-overlapped I/O/hardware/service cost
```

Assign every cost to one owner. For each replacement state what disappears, what remains and what new cost is introduced. When the aggregate plausible removal is smaller than the required gap, expand the architectural cut; do not spend indefinitely optimizing small leaves.

### 12.7 Proposed internal budget

The following is a provisional design allocation, **not a measurement or sum of independent percentiles**. It is per presented frame, including all logic updates required by the intended cadence. Reallocate after exclusive attribution.

| Exclusive owner | Design allocation, ticks |
|---|---:|
| Simulation, AI, collision and authored events | 420,000 |
| Pose evaluation and gameplay sockets | 130,000 |
| Fighter presentation and native submission | 200,000 |
| Stage, effects and UI presentation | 100,000 |
| Audio, storage and resource service | 60,000 |
| Scheduling, interrupts and remaining overhead | 40,000 |
| **Total proposed design objective** | **950,000** |

This table is a way to challenge oversized owners, not an assertion that each target is attainable in isolation. The measured combined runtime decides.

## 13. Implementable work packages

These packages are intended to become a short master plan plus bounded implementation tasks. The source paths are anchors at the audited SHA; update line references after rebasing. Every task must identify **work deleted**, not only new code added.

### WP0 — Baseline, semantic contract and opportunity ledger

**Anchors:** `PROJECT_GOAL.md`, `docs/HANDOFF.md`, `docs/P2_EXECUTION_BOARD.md`, the latest pose-joint-mask report, `scripts/census-tick-hud-p95-set.py`, existing capture and verification scripts. [R1–R4]

**Implementation:** reproduce the hard-on four-CPU scenario; record the current low-instrumentation shipping result, exact build identity, per-frame exclusive ledger, ITCM/DTCM input-section inventory, CPU engagement and content witnesses. Reconcile historical exactness restrictions with the current approved native/fixed endpoint.

**Outputs:** one baseline manifest, one scenario list, one exclusive-cost ledger and one prioritized deletion ledger. Reuse existing tools; do not create another large profiling subsystem.

**Gate:** the baseline is repeatable enough to distinguish the planned intervention, measurement units/nesting are unambiguous and missing content is explicitly tracked. No speedup claimed.

**Stop condition:** a harness that omits CPU activity or required native content cannot be used to bank performance. Fix that specific validity issue; do not widen WP0 into unrelated test infrastructure.

### WP1 — Explicit code/data placement and shipping instrumentation boundary

**Anchors:** `linker/nds_hot_text.ld`, `nds_renderer_preamble.c`, `nds_renderer.c`, current TCM scripts and telemetry macros. [R6–R9, R13, R14, R17]

**Implementation:** inventory shared input sections and `.32.o` side effects; make ISA and residency independent; create unique sections for selected kernels; keep cold code out of hot sections; compile detailed diagnostic observers out of shipping while retaining required safety checks.

**Work deleted:** accidental linked residents, unnecessary shipping observer instructions and unnecessary section coupling. Source comments or disabled branches do not count as deleted runtime work.

**Gate:** startup, interrupts, all admitted scene transitions and native-only link checks pass; no DTCM ceiling regression; current code generation and placement deltas are separately visible. Whole-frame measurement determines whether a particular placement is retained.

**Dependencies:** WP0. This is bounded enabling work, not the main 60% campaign.

### WP2 — Fixed event clock and compact pose vertical slice

**Anchors:** `nds_ft_pose.c`, `nds_ft_pose.h`, `nds_ftanim_track.c`, pose oracle scripts, animation helpers, `battleship_ftmain.c` and their consumers. [R14, R15]

**Implementation:** define native event timing; compile event/track metadata; retain remainder and speed-change semantics; make fixed pose authoritative for a selected fighter's required sockets and rendering; eliminate that slice's float publishing/re-reading and integer IEEE clock work where qualified.

**Work deleted:** the selected slice's binary32 clock emulation, representation bridges, redundant hierarchy processing and duplicated required transforms.

**Gate:** event-boundary cases, hitlag, loops, interruption, grabs/throws, attachments and rendering regressions pass. Measure the entire slice in the four-fighter workload. Host tests explicitly document authorized numerical tolerances rather than inheriting arbitrary bitwise field checks.

**Stop condition:** a version that only replaces one float helper while retaining all bridges is not WP2 completion. Revise the ownership boundary rather than accepting a nominal fixed-point conversion.

**Dependencies:** WP0, interface agreement with WP3. Expand across the roster only after the pilot's total cost and memory are understood.

### WP3 — Bound models and generated native GX packets

**Anchors:** `nds_renderer_native_fighter_production.c`, `nds_renderer_native_common.c`, `nds_renderer_native_owners.c`, `renderer_adapter_matrix.c`, existing generators and packet structures. [R11–R13, R16, R29]

**Implementation:** first price preflight on hits and misses. Move immutable validation and binding to model admission/status-driven topology change. Generate native packet templates and typed dynamic patches, factor geometry from pose/material state, and define exact VRAM/packet ownership.

**Work deleted:** proven redundant preflight/discovery, source-state reconstruction, first-use recording where AOT replaces it, repeated immutable vertex conversion and unnecessary full packet/matrix copies.

**Pilot:** a simple admitted actor such as TaruCann can prove the binding contract, but a current hot fighter path must prove the performance case. Preserve split roots and dynamic texgen coverage already working.

**Gate:** zero unsupported native paths, correct depth/material/attachment behavior, packet lifetime checks, explicit RAM budget and a whole-owner speed improvement in realistic four-fighter scenes. No new source display-list interpreter in any DS build.

**Dependencies:** WP0; coordinate native matrix/pose format with WP2. Do not block packet-format tools on final TCM packing.

### WP4 — Static stage, particle and UI execution reduction

**Anchors:** stage begin/commit/no-Z functions from the census, matrix adapter, particle submission, HUD and stage generators. [R5, R13, R16]

**Implementation:** compile invariant stage runs and transform classes; update only changed dynamic owners; batch compatible particles; move qualified screen-space UI to BG/OAM; make formatting value-driven.

**Work deleted:** repeated static hierarchy/material work, redundant camera-basis computation where still present, repeated generic quad setup and unnecessary UI updates.

**Gate:** stage geometry and transparency regressions pass; effects that should intersect fighters do so; no simulation/RNG/emitter omissions; hardware geometry and VRAM limits remain within bounds.

**Dependencies:** WP3's small native executor contract. Rendering and event-state changes must have separate tests.

### WP5 — Compact fixed gameplay and shared query work

**Anchors:** `battleship_ftmain.c`, `battleship_ftcomputer.c`, fighter state/physics/collision consumers, existing stage-collision wrappers and source ordering. [R15, R17]

**Implementation:** establish fixed hot fighter state, shared once-per-tick query facts, stage collision metadata and compact active lists. Preserve directed hit/catch behavior and source process ordering. Convert complete physics/collision/AI chains rather than isolated leaves.

**Work deleted:** redundant per-CPU facts, broad repeated scans where bounded candidates suffice, float bridges and excessive pointer-based access to cold object state.

**Gate:** movement/collision thresholds, one-way platforms, edge cases, status timing, CPU engagement and RNG/order-sensitive scenarios pass. Measure SCPU and other exclusive subowners without double-counting SRC/SINT.

**Dependencies:** WP2 numeric/event contract. Scheduler replacement is optional inside this package and requires explicit order proof; do not expand it into a new general engine.

### WP6 — Close remaining runtime float and legacy ownership families

**Anchors:** existing soft-float census scripts, custom IEEE assembly/C helpers, imported callers, menus/UI, material/particle code and linker helper placement. [R10, R14, R21]

**Implementation:** build a caller-family graph covering adds, multiplies, divides, compares, conversions, doubles, math-library entry points and custom bit-pattern emulation. Convert remaining domains, including cold DS runtime paths. Update assets/constants offline. Remove transitional state mirrors and their bridges.

**Work deleted:** entire obsolete helper families, conversions and source-shaped data dependencies, not just a reduced count of named library calls.

**Gate:** structural call-graph/disassembly checks plus exercised scene coverage establish the native endpoint. A grep for `float` is neither necessary nor sufficient: inspect function types, helpers and custom implementations. No runtime binary32 arithmetic disguised behind integer names.

**Dependencies:** WP2/WP4/WP5. Do not evict still-hot float helpers early merely to make the residency report look good.

### WP7 — Final hot-state placement and bare-metal kernel tuning

**Anchors:** updated linker/map, current per-PC and data-access census, compact owners produced above. [R5–R9]

**Implementation:** measure small DTCM state candidates, pack unique ITCM kernels by actual whole-frame value, then evaluate ARM/Thumb, optimization level, inlining and limited assembly where code remains hot.

**Work deleted:** avoidable fetch/data stalls, oversized hot kernels and leftover generality that the new invariant contracts no longer require.

**Gate:** same-operation placement comparisons, final hard-on linked-layout proof, no boot/IRQ/stack regression and no unsupported instruction use. Data shared with DMA/ARM7 stays outside TCM.

**Dependencies:** first structural owners stable. Re-run after large roster/content additions; do not preserve an old pack indefinitely by superstition.

### WP8 — Optional offload experiments with a wall-time verdict

**Anchors:** existing GX packet/DMA path, ARM7 service ownership, audio/storage services and `nds_r2_hwmath_unit.h`. [R21, H3, H5–H7]

**Implementation:** select only jobs whose measured critical-path cost and independence justify offload. Compare a CPU route against a hardware/service route including queueing, copying, coherency, contention and completion.

**Work deleted:** actual non-overlapped ARM9 work. Moving a counter or hiding work in another processor is not a win.

**Gate:** faster whole-frame cadence, deadlines met, no audio/input/service regression, safe cancellation/reset and no new missing-work fallback. Extend emulator calibration for the newly exercised bus/concurrency behavior when needed.

**Dependencies:** an independent job with measured slack. This package is not a prerequisite for the architectural wins above and must not become the primary plan without evidence.

### WP9 — Full content, memory and cadence closure

**Implementation:** integrate winners, delete old runtime routes, test the latest shipping shape across the supported roster/stage/item matrix, and run focused worst-case stress plus long natural matches and scene-transition cycles.

**Gate:** original-DS memory contract, complete required native rendering, correct mechanics, the official P95 work and two-VBlank targets, and reported P99/max/consecutive-miss behavior. The proposed 950k objective is tracked as headroom, not substituted for the official criteria.

**No completion by omission:** fewer visible objects, disabled CPUs, reduced gameplay pool sizes, missing effects, untested cold states or a benchmark-only configuration do not qualify. If approved asset representation or level-of-detail options are used, report that configuration explicitly rather than treating a quality change as a transparent code optimization.

## 14. Sequencing and agent boundaries

A practical dependency graph is:

```text
WP0 -> bounded WP1
WP0 -> shared native data/event contracts
              |-> WP2 pose/events -----------|
              |-> WP3 binding/packets -> WP4 |
              |-> WP5 gameplay/query work ---|-> WP6 -> WP7 -> WP9
                                               \-> WP8 only where justified
```

Some work can overlap, but one integrator should own the linker, shared numeric formats, descriptor lifetimes and benchmark identity. Separate agent ownership by implementation boundary, not by arbitrary files that all include the same mutable renderer state.

A task handoff should contain: exact baseline, source anchors, the work to delete, invariants to preserve, allowed edits, proof command/scenario, memory budget, expected change in complexity and rollback/stop condition. It should not say merely “optimize this file” or reward net additions to an already large adapter.

Use short-lived experimental routes. After a winner is accepted, remove the losing route from the DS build and retain only the host test/reference needed to preserve its knowledge. Avoid accumulating an endless family of permanent feature toggles, ownership layers and generic fallbacks.

## 15. Non-negotiable anti-regression rules

**No fake speedup:** never disable required content, CPU decisions, collision updates, emitter state or instrumentation validity to pass a frame target.

**No blanket ITCM purge:** cold-window evidence is not global dead-code proof. Preserve system paths and measure the opportunity cost of eviction.

**No float sandwiches:** a native domain ends with native consumers, not permanent conversions through source fields.

**No universal fixed format:** source units, world range, matrices, vertices, normals and event clocks have different requirements.

**No new runtime interpreter:** build tools may understand original formats; DS builds consume qualified native assets and commands.

**No cache without an owner:** invalidation belongs to actual mutation, including dynamic topology changes that leave the root pointer unchanged.

**No unlimited precompute:** every bake reports storage bytes, resident bytes, first-use costs and its minimum guaranteed live set.

**No offload without ownership:** cache visibility, request generations, cancellation and distinct DMA/render lifetimes are part of correctness.

**No percentile arithmetic:** measure the integrated per-frame result and rerank all frames. Do not sum nested buckets or independent P95 savings.

**No claim of 60% until measured:** this is the proposed engineering target. A smaller runtime gives a credible path to test, not a guarantee.

## 16. Final recommendation

Start with two coordinated architectural slices: **authoritative fixed event/pose/gameplay data** and **bound native draw descriptors with generated GX packets**. Use a small actor to validate the renderer contract and a current hot fighter path to validate its performance value. Apply the same ownership model to static stages, particles and UI; convert the remaining gameplay domains; then reclaim and repack TCM around the smaller live kernels.

The end-state should be easy to describe: fixed native game state, compact event/animation data, precise mutation-owned generations, bounded typed game loops, native assets and a small hardware submission layer. That is the SM64DS lesson worth carrying into this port.

The key acceptance question is not “How much code was rewritten?” It is: **Which entire categories of work stopped happening on each frame, and did the complete four-fighter game remain correct while meeting both work and cadence targets?**


## 17. Source index and reproducibility notes

All repository URLs below are pinned to the audit SHA, not mutable `master`. Some sources were inspected in selected ranges or search excerpts, as explicitly noted. Their existence is not a claim that every instruction or call path in the file was reviewed. Hardware documentation was consulted on September 15, 2026.

### [R1] Current project contract

Native-only rendering, permitted DS-native redesign, fidelity and performance goals. Current contract, not an old campaign restriction.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/PROJECT_GOAL.md`

### [R2] Latest inspected hard-on four-CPU checkpoint

Whole-match results, same-ROM mask comparison, shipping-shape checkpoint, heap and cadence evidence. No new measurement was made for this report.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/performance/2026-09-15_p2-2p8-pose-joint-mask/README.md`

### [R3] Current status and integration record

Retained optimizations and distinction between four-slot capacity and incomplete performance/content closure. Selected current sections inspected.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/HANDOFF.md`

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/P2_EXECUTION_BOARD.md`

### [R4] Measurement definitions and nesting

Gate 1,120,380, VBlank divisor, exclusive/subordinate bucket relationships, WORK minus HUD identity and separate cadence/work-tail analysis.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/census-tick-hud-p95-set.py`

### [R5] September 15 diagnostic PC census

A preceding diagnostic window, not the final shipping ELF: symbol costs, section sizes, cold-in-window residents and placement candidates.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/performance/2026-09-15_p2-2p8-pose-track-mask/poseplay-pc.txt`

### [R6] Actual linker policy

Inspected section and memory definitions, .32.o coupling and DTCM ceiling. Recheck full current map before changing placement.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/linker/nds_hot_text.ld`

### [R7] Renderer placement and historical configuration macros

Inspected opening section. Explicit shared section attributes and history of renderer placement experiments; comments alone do not establish current reachability.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_preamble.c`

### [R8] Renderer translation-unit organization

Textual implementation inclusion. Source splitting is not necessarily translation-unit splitting.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer.c`

### [R9] DTCM design and stack/layout constraints

Historical design guidance; do not treat historical occupancy or candidate savings as a current measurement.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/optimization/review/02_DTCM_RECLAMATION_AND_HOT_DATA_PLACEMENT.md`

### [R10] Historical software-float campaign

Helper reachability, conversion-sandwich failures, earlier ITCM reclamation and already-existing caches. Historical restrictions/numbers must be reconciled with R1 and a current build.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/optimization/review/06_SYSTEMATIC_SOFTWARE_FLOAT_ELIMINATION_ITCM_DIVIDEND.md`

### [R11] Native fighter production path

Opening production path inspected: preflight precedes replay attempt, plus native state and hierarchy preparation. Preflight internal redundancy was not fully established.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_native_fighter_production.c`

### [R12] Shared native state and vertex preparation

Inspected eligibility, packed-triangle decoding and color/UV preparation code. Per-scenario execution frequency requires profiling.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_native_common.c`

### [R13] Native actor execution example

Inspected TaruCann two-joint quad path: generic traversal/config preparation, immutable-vertex reconstruction, hierarchy checks and native matrix submission.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_native_owners.c`

### [R14] Compact pose and clock implementation

Inspected clock/numeric design and update sections, oracle/mask behavior and fixed/source-field interface. Wider file not exhaustively reviewed.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_ft_pose.c`

### [R15] Fighter update/status import and topology invalidation

Inspected status wrapper and source import: status changes can mutate topology without root or heap-generation changes.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/import/battleship_ftmain.c`

### [R16] Transform adapters and special semantics

Inspected opening declarations and transform-kind explanations; source search and census establish persistent world-matrix cache/function anchors. Full cache implementation was not reviewed.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/port/renderer_adapter_matrix.c`

### [R17] CPU decision import, harness controls and telemetry

Inspected per-CPU process wrapper and guarded diagnostic work. AI semantics come from the imported source; its full decision body was not exhaustively reviewed.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/import/battleship_ftcomputer.c`

### [R18] Historical ITCM reclamation campaign

Earlier reclamation, grouping and placement work. Prevents double-counting gains already retained.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/optimization/review/01_ITCM_RECLAMATION_AND_HOT_CODE_REPACKING.md`

### [R19] Pose interface anchor

Selected indexed excerpts inspected for pose counters and ownership interface; not a full interface audit.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/include/nds/nds_ft_pose.h`

### [R20] Fixed camera implementation anchor

Selected search excerpts inspected for native divider code; current operation also corroborated by R5 and R21.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/import/battleship_gmcamera.c`

### [R21] Existing hardware-math owner and fast/legacy routes

Inspected ownership discussion and hardware register helpers. The documented no-interrupt-writer proof is explicitly binary-specific.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/include/nds/nds_r2_hwmath_unit.h`

### [R22] SM64DS model-bank layout

Flat model/material/texture/display-list records; comments distinguish matched layout evidence from host-inferred names.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/include/BMD_File.h`

### [R23] SM64DS model relocation

Load-time rebasing of file-relative pointers.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/_ZN5Model17UpdateFileOffsetsER8BMD_File.cpp`

### [R24] SM64DS fixed cross product

Explicit fixed operands, wide products and rounding.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/CrossVec3.c`

### [R25] SM64DS fixed camera construction

Inspected fixed vector/matrix and angle-table path; file reports matched provenance.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/Camera_UpdateMatrices.c`

### [R26] SM64DS unchanged-animation fast path

Same-file case updates flags/speed; changed file resets animation.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/_ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj.c`

### [R27] SM64DS bounded collision filtering example

Bounded actor scan and preliminary rejection. A reference pattern, not Smash collision code.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/_ZN9dBgCh_Gnd10DetectClsnEv.cpp`

### [R28] Hosted SM64DS renderer distinction

Selected indexed excerpt shows hosted floating-point matrix representation. Do not attribute that host representation to the original DS execution path.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/port/ntr/bmd.cpp`

### [R29] Retained Link packet texture-generation improvement

Recorded packet patching and its own measured baseline; gains are already incorporated in later checkpoints.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/performance/2026-09-14_p2-2p8-link-texgen-packet/README.md`

### [R30] Retained BGM direct-range improvement

Recorded direct-read implementation and same-campaign measurement, preceding the latest checkpoint.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/performance/2026-09-14_p2-2p8-bgm-direct/README.md`

### [R31] Split-root replay support record

Selected indexed excerpts establish recorded split-root replay support. Current retained replay is also corroborated by R3.

`https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/performance/2026-09-14_p2-2p8-split-packet-replay/README.md`

### [H1] BlocksDS: Optimizing code

ARM/Thumb tradeoffs, long multiply, cache working sets and DMA-versus-CPU transfer considerations.

`https://blocksds.skylyrac.net/tutorial/advanced/optimizing_code/`

### [H2] BlocksDS: TCM and Cache

Physical TCM/cache behavior and TCM visibility restrictions. The project linker, not generic examples, determines its usable layout.

`https://blocksds.skylyrac.net/tutorial/intermediate/tcm_and_cache/`

### [H3] libnds: videoGL.h reference

Native geometry/matrix/texture/polygon APIs. Similar API names do not prove source-game semantic equivalence.

`https://blocksds.skylyrac.net/libnds/videoGL_8h.html`

### [H4] GBATEK hardware reference, mGBA-hosted mirror

DS hardware and geometry-format/capacity reference. Indexed material and SDK references consulted; use relevant register sections during implementation.

`https://mgba-emu.github.io/gbatek/`

### [H5] BlocksDS: DMA

Transfer setup, visibility and coherency requirements.

`https://blocksds.skylyrac.net/tutorial/intermediate/dma/`

### [H6] BlocksDS: Using the ARM7

Service ownership, communication and shared-memory considerations.

`https://blocksds.skylyrac.net/tutorial/intermediate/using_the_arm7/`

### [H7] libnds: math.h reference

Hardware arithmetic APIs, asynchronous/result separation and shared-unit constraints.

`https://blocksds.skylyrac.net/libnds/math_8h.html`

### Evidence not produced by this report

There is no new ELF/map, source patch, ROM, emulator recording, physical-hardware measurement or measured package speedup attached to this research. The recorded baselines belong to the repository reports cited above. All architectural alternatives require their stated implementation and acceptance steps.
