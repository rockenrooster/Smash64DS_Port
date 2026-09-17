MISC: the strongest remaining optimization candidates

I found several concrete opportunities—not just “use more assembly.” The main redesign should be to make MISC consume prebound native draw records and emit prepared GX commands, instead of repeatedly reconstructing render state, transforming shared data, and preparing textures during drawing.

The strongest findings are:

Candidate	What it removes	Assessment
M1. Cache particle transform invariants	Repeated square roots and billboard-basis calculations for particles sharing a transform	Best small, low-risk first change
M2. Fully compile native effect/weapon packets	Reconstructed vertices, source-style state updates, generic material decisions, per-corner conversion	Strongest architectural direction
M3. Preconvert and make DamageSlash frames resident	Per-texel conversion and texture replacement inside drawing	Concrete tail-latency candidate; requires a VRAM allocation trade
M4. Extend ordered GX batching beyond Whispy	Packet flushes and immediate submission between compatible particle draws	Promising, but ordering and synchronization must remain explicit
M5. Prebind particle frame/material descriptors	Repeated bank classification, frame searches, binding resolution, and representation conversions	Supporting redesign that makes the hot loop smaller
M6. Aggregate diagnostics and specialize the remaining kernels	Per-particle bookkeeping traffic and avoidable kernel overhead	Follow after the larger deletions

These assessments are based on the source at 430aca2879e9071dc2b22f944f5c2909c9ce7aa4, which was the master snapshot resolved during this review. I have not built or benchmarked these new candidates. Their source-level opportunities are identified below; their tick savings still need controlled measurements.

1. What MISC actually contains—and what it must achieve

MISC is not a miscellaneous simulation subsystem. The diagnostics define it as the residual drawing time after subtracting FTR, STG, BG, and HUD, plus the flush. That makes effect drawing, weapon drawing, particle submission, and unclaimed renderer work the relevant starting points—not an indiscriminate rewrite of audio, scheduling, or all particle simulation.

The September 15 retained particle-submit checkpoint reports:

Metric	P50	P95
Whole-frame WORK-H	1,654,208	2,375,296
MISC	253,824	481,024

Those are the full 1,972-sample results, not the more favorable focused 128-frame experiment. The same artifact explicitly says the product performance target remains unmet.

A newer September 16 MObj checkpoint reports WORK-H P50 1,587,328 and P95 2,323,584. That is a separate capture; I am not combining its whole-frame numbers with September 15’s MISC distribution as though they were one measurement.

Against the project’s approximately 1.12 million ticks per presented frame, the September 15 MISC P95 alone consumes roughly 43% of the target budget. The September 16 whole-frame P95 would need approximately a 52% reduction to reach that budget. These calculations show why MISC deserves substantial redesign, but they do not justify promising that MISC alone will deliver 30 FPS. Also, independent bucket percentiles cannot be added or subtracted to predict whole-frame P95.

The objective should be a much cheaper MISC implementation, not a smaller reported MISC bucket obtained by moving work elsewhere.

2. M1 — Cache the particle transform’s invariant work
The concrete finding

In:

src/import/battleship_lbparticle.c
ndsParticleTransformForDraw() — approximately lines 3510–3780

the affine matrix is already cached using the particle transform epoch. However, the generic transformed-particle path still calculates two affine-axis lengths using sqrtf and scales the camera’s right/up vectors for each particle. Those lengths depend on the shared transform, not on the individual particle’s position.

This is important because “cache the matrix” would be redundant advice. The remaining opportunity is to cache the derived data around the matrix.

Proposed replacement

Use a small render-side transform context containing:

Transform identity + lifetime/generation
Current transform epoch
Camera/pass identity
Cached affine matrix
Signed axis lengths
Scaled billboard right/up vectors

Calculate the axis lengths once when the transform changes. Calculate the camera-relative basis once per transform/camera combination.

The particle loop should then perform only the position-dependent transform and quad construction.

If N particles share K transforms during a pass, the two square-root calculations change from:

2 × N square roots

to:

2 × K square roots

For an illustrative—not measured—case of 64 particles sharing four transforms, that is 128 calls reduced to eight.

Why I would implement this first

The first version can retain the existing floating-point calculations exactly and merely change their frequency. It does not require rewriting simulation, changing particle lifetimes, or introducing a new graphics backend.

After that is proven, the cached draw context can hold fixed-point values suitable for the existing integer quad emitter.

Correctness details that matter

The current code derives the scale sign from affine diagonal entries. Replacing the calculation with xf->scale.x/y without proving equivalence is unsafe, especially with rotations and negative scales. Cache the existing result first.

Use a transform generation or pass-scoped cache—not an indefinitely retained raw-pointer cache—because transform slots can be reused. Include camera identity where the cached result incorporates camera basis vectors.

Do not drop particles when the cache fills. Either size it to the admitted transform pool or compute an uncached native result for that draw.

Acceptance evidence: axis-norm evaluation count tracks distinct transforms, visual output is unchanged, and paired MISC/whole-frame timings improve on transformed-particle frames.

3. M2 — Finish compiling native effects into actual GX packets

This is the main architectural opportunity. Native rendering is necessary, but “native” does not automatically mean the runtime representation is efficient.

A particularly clean starting point: Samus Charge Shot

In:

src/nds/nds_native_samus_chargeshot.exec.inc
ndsRendererSubmitNativeSamusChargeShot()

the source describes the entire graphics program as fixed, with the DObj transform supplied through the configuration matrices as its live input. Nevertheless, the executor still reconstructs an input-vertex array, initializes traversal state, applies source-style setup, resolves texture state, derives material/color decisions, and runs conversion helpers for each emitted corner.

That is a strong candidate because the source itself identifies an unusually narrow dynamic contract.

Replace this runtime shape
Reconstruct input vertices
    → initialize general traversal state
    → apply source-style render-state mutations
    → resolve material/color/texture interpretation
    → convert each corner
    → issue GX writes

with:

Load prebound native packet
    → patch current matrices
    → emit required state changes
    → submit prepared geometry words

For this owner, the build tool should precompute the final texture coordinates, vertex colors, geometry command sequence, and fixed material state. Scene preparation should resolve texture/palette bindings into native handles.

Do not create another runtime bytecode interpreter to execute the “compiled” representation. Emit generated C and prepared GX data for the few genuinely different native draw families.

DamageSlash demonstrates the same problem with a slightly richer contract

The generated DamageSlash setup functions still call operations such as:

RecordOtherMode
RecordSetCombine
RecordSetTile
RecordSetImage
RecordLoadTlut
RecordTextureState
RecordSetTileSize

The executor then derives the native drawing decisions from that state. This is already native code rather than N64 display-list interpretation, but it still reconstructs an intermediate representation that can be specialized further.

For DamageSlash, generate a packet whose variable inputs are explicit: current image/frame, material color/alpha, and matrices. Everything fixed should already be expressed in the packet’s native state and geometry.

Keep the architecture small

I would start with three direct native emitter families:

Family	Runtime inputs
Billboard particle	Position, size, prepared basis, color/alpha, bound frame
Fixed-geometry weapon/effect	Transform, bound texture/material
Animated-material fixed geometry	Transform, material-frame selection, dynamic color/alpha

These can share small GX-writing kernels without becoming a generic compatibility renderer. The project goal expressly allows typed bindings, shared native kernels, generated code, and precompiled command streams.

Main hazards

The packet must preserve both its incoming assumptions and its outgoing state: depth behavior, polygon ID, alpha, projection/modelview normalization, texture/palette state, and any state inherited by the next owner.

Also, preserve primitive topology initially. The existing Fox blaster owner documents that replacing its two source triangles with a quad changed the appearance of the thin beam. “Fewer commands” is not automatically an equivalent rendering result.

Acceptance evidence: the selected owner no longer constructs temporary vertices or source-style traversal/material state during ordinary drawing; its packet remains native-only; owner-local cost and whole-frame cost both improve.

4. M3 — Remove DamageSlash texture conversion and streaming from drawing

This is the most concrete resource-lifetime and frame-time-spike candidate I found.

What the code currently does

In:

src/nds/nds_native_damage_slash.exec.inc

ndsDamageSlashTextureFill() clears a scratch buffer, loops over output texels, performs source-coordinate mirroring/clamping, reads packed source nibbles, and repacks the converted image.

ndsDamageSlashEnsureTexture() maintains two texture names—one per drawable child—and replaces a child’s texels when its requested frame changes, using the fenced texture-upload path during drawing. The source explains that this limits texture VRAM to 1.5 KiB because the battle allocator was already full.

That means a native effect can still perform asset conversion and texture mutation in the active draw path.

The exact storage trade

The generated shape table contains:

Frames	Converted extent	Format	Total texels
Eight	32 × 64	4-bit indexed	8,192 bytes
Five	32 × 32	4-bit indexed	2,560 bytes
Complete set			10,752 bytes

That is 10.5 KiB, or 9 KiB more texel storage than the current 1.5 KiB pair of streaming slots. This calculation excludes allocator/binding overhead and any extra atlas padding.

Two useful candidates—not an all-or-nothing rewrite

M3a: Preconvert the complete frame set offline.

Extend the generator to emit the final DS nibble order, mirroring, and padded edges. Load the prepared payload before active gameplay.

Keep the two VRAM slots initially.

This removes the per-texel conversion work but does not remove uploads or their synchronization cost. It is a clean intermediate experiment with a smaller VRAM impact.

M3b: Make all frames immutable and resident.

Reserve space for the full admitted DamageSlash set during scene preparation. A frame change then selects a binding or texture region rather than rewriting texture memory.

This removes conversion, repeated uploads, and the need to mutate these textures during drawing.

A potentially important amplification

The current loaded-frame state is shared by child slot, not by every live DamageSlash instance. My inference is that simultaneous instances at different animation ages could repeatedly request different frames for the same slot, potentially causing multiple replacements within one presented frame. That behavior needs an instance/frame/upload trace; it is not established by the existing timing artifacts.

The restriction is real

Do not simply add 13 texture allocations and hope they fit. The current implementation explicitly records VRAM pressure. The resident candidate needs a scene-level allocation plan and proof under the four-fighter/item workload.

Likewise, deferring an upload to another bucket is not itself a saving. The target is fewer conversions, fewer uploaded bytes, and fewer synchronization requirements.

Acceptance evidence: conversion count becomes zero during gameplay; the resident variant’s frame-change upload count becomes zero; overlapping instances display correctly; no other required texture is displaced or degraded.

5. M4 — Extend ordered GX batching to the general particle path
What is already implemented

Whispy already has a packed native GX path with prepared texture bindings and state-change handling. Its flush routine performs cache maintenance, starts FIFO DMA, waits for completion, and reconciles renderer state. That is useful existing machinery, not a new feature to reimplement.

But ndsRendererSubmitParticleQuad() flushes the Whispy packet before submitting a generic particle quad. The general particle path then uses its own immediate submission path.

Proposed redesign

Let compatible generic particles and Whispy particles append to a common source-order-preserving native command stream.

A change of particle family should not require a flush merely because one family uses prepared packets and the other uses immediate calls. Append the required texture, palette, alpha, and geometry state transitions in order.

Flush at actual boundaries:

Packet capacity reached
Camera or incompatible pass transition
Immediate owner takes control
A resource mutation requires synchronization
End of the relevant ordered draw pass

This is not a proposal to globally sort effects by texture. It is a proposal to serialize the existing order more cheaply.

Why this has more value than changing individual GX writes

It can eliminate repeated transitions between submission mechanisms, reduce state reconciliation, and amortize dispatch and cache-maintenance overhead across more work.

However, the generic path already retains some batch state. I am not claiming that it currently performs a complete texture bind and begin/end cycle for every particle. The opportunity is at the boundaries between native paths and the remaining per-quad setup.

DMA is a mechanism, not the promised saving

The existing Whispy flush waits for DMA completion. Therefore, its presence does not demonstrate CPU/GPU overlap. First compare a larger ordered packet submitted through the existing mechanism against direct CPU submission at small packet sizes.

Only introduce double buffering or asynchronous submission after measuring that it helps. Buffers must remain immutable until consumed, and DMA sources must be in DMA-visible memory: the ARM9’s ITCM and DTCM are not DMA-accessible.

Acceptance evidence: fewer packet flushes/state-boundary transitions for equivalent draws, lower submission cost, preserved translucent ordering, and no increase in FIFO wait or resource hazards that cancels the saving.

6. M5 — Prebind frame descriptors and keep render data in its native representation

This is how I would simplify the general particle loop around M1 and M4.

Replace repeated interpretation with a bound descriptor

The particle frame resolver already has a first-row index by texture, so it does not scan the entire atlas. It still searches that texture’s frame rows, and it implements meaningful semantics: an exact match when available, otherwise an earlier retained frame for decimated animation.

Generate a compact lookup for the reachable source frames:

canonical bank + texture + source frame
    → native frame descriptor

The descriptor should contain or directly reference:

Texture/palette binding
Prepared UVs
Source mirroring rules
Native render-family identity
Required material interpretation

For decimated animations, generate the same hold-previous-frame mapping, including missing/invalid cases. A direct lookup that silently changes animation timing is not acceptable.

Use compact tables sized to actual reachable frames—not a giant sparse Cartesian product of every bank, texture, and possible frame number.

Bind stable identity at preparation time

Resolve dynamic source bank identities to a canonical render descriptor once when the bank is admitted. Preserve a lifetime/generation check for scene changes and relocated assets.

That allows the hot path to stop repeatedly asking what kind of source bank a particle belongs to.

Eliminate conversion round trips

The particle camera support already has a two-entry input-keyed cache. The generic render path also contains bridges between the camera’s fixed representation and floating-point consumers, followed by conversion into the fixed quad-submission representation. A native pass context can retain the camera basis in the format the emitter actually needs.

But this is not another request to convert ndsRendererSubmitParticleQuad() to fixed point. That landed on September 15: its retained implementation uses Q8 center/size, Q13 basis, integer corner construction, and no linked __aeabi_f* calls.

The new work is to prepare and retain appropriate inputs upstream, especially those shared across particles.

Keep the existing range-selection behavior for distant effects and its safety guard. Do not gain speed by reintroducing coordinate saturation near stage edges or during far-away KO effects.

Acceptance evidence: ordinary particle draws use a direct descriptor and prepared basis; bank/frame resolution scales with changes rather than every draw; RAM/cache costs do not outweigh the removed work.

7. M6 — Reduce bookkeeping, then use assembly selectively
Aggregate diagnostics at pass boundaries

The source contains extensive particle/effect counters, and the Whispy specialization already demonstrates local aggregation for some of its diagnostics. Extend that approach where equivalent: accumulate counts locally and publish them once per pass rather than repeatedly updating scattered volatile globals. Keep failure, capacity, and native-rendering witnesses intact.

A measurement trap deserves explicit attention: gNdsMiscSplitAccountedTicks is not a usable exclusive-time total. Its fold includes tick counters, upload counts, and byte counts. Do not subtract it from MISC to claim an unexplained duration.

This is supporting work, not my proposed main source of savings.

The assembly targets that make sense

After the architecture removes redundant work, inspect the linked code for three small kernels:

Kernel	What to inspect
Fixed affine position transform	Signed wide multiply/accumulate, loads, shifts, spills
Billboard corner construction	Repeated conversions, duplicated arithmetic, packing
GX packet append/patch	Bounds checks, state branches, stores, function-call overhead

ARM provides signed long multiply and multiply-accumulate instructions suitable for carefully bounded fixed-point kernels. That supports trying a small ARM implementation—not assuming that handwritten assembly must beat the compiler.

Keep a C reference and differential-test negative values, rounding boundaries, large scales, and coordinate limits. Do not accidentally introduce division helpers or change signed rounding while “simplifying” fixed-point arithmetic; the earlier rejected Q12 particle experiment already encountered that problem.

Also, the September 16 measured build had only 104 bytes of ITCM headroom. Moving a large MISC subsystem there is not a free option. A kernel must justify replacing another occupant or fit after deliberate reclamation.

I would not begin by moving MISC rendering to ARM7. The graphics engines belong to ARM9; ARM7 offload would require shared-memory work and synchronization while competing with its existing services. CPU-only preparation could be investigated later, but it is more complex than deleting the identified redundant calculations.

8. The resulting architecture should be simpler

I would aim for this runtime shape:

BUILD TIME
    Source assets
        → final DS texture payloads
        → native geometry packets
        → compact frame/material mappings
        → explicit dynamic-input contracts

SCENE PREPARATION
    Admit required resources
        → allocate texture/palette residency
        → resolve native bindings
        → prepare packet templates
        → establish generation-safe handles

DRAW PASS
    Prepare camera context once
    Prepare each distinct transform context once

    Visit visible instances in required order:
        select bound descriptor
        calculate only changing position/material data
        append native commands

    Submit at actual synchronization boundaries

There is no requirement here to replace all gameplay objects, particle scripts, or simulation pools. The first redesign is a render-side separation of immutable data, per-pass data, per-transform data, and per-instance data.

The inspected sm64-nds renderer separates texture handling into a queued upload/synchronization mechanism and uses compact hot renderer state. Its code is useful evidence for separating resource operations from geometry emission, not an architecture to copy wholesale. The inspected sm64ds-decomp resource helper also reuses matching model texture/palette-associated entries; that supports investigating shared resource bindings without assuming the unidentified fields are already your desired GX descriptors.

The key change is not another abstraction layer. It is removing work from the draw loop by deciding more things earlier.

9. What should not be rediscovered

Several obvious suggestions are already present:

Suggestion	Existing implementation
Convert generic particle-quad submission to fixed point	Retained September 15 implementation
Cache particle camera calculations	Existing two-entry camera cache
Add a specialized Whispy particle route	Existing AOT transform and packet paths
Put particles in an atlas	Existing prepared multi-sheet atlas
Introduce an effect pool	Existing bounded allocation
Skip stable material animation work	September 16 stage-local MObj optimization; principally SRC work

These should be foundations or exclusions, not new campaigns with new names.

10. Implementation order and proof

Start with M1, DamageSlash preconversion, and the Charge Shot packet prototype. They are separate, identifiable changes with clear mechanisms and comparatively bounded scopes. Use their results to decide whether to extend the packet architecture broadly.

For every candidate, require three kinds of evidence:

Engagement and deleted work. Show that the relevant path actually executes in the tested workload. Count distinct transforms versus norm evaluations, texture conversions/uploads, owner draws, packet flushes, and frame-descriptor lookups. A faster routine that scarcely runs is not a major MISC win.

Paired performance. Compare equivalent frame sequences, preferably native A/B routes in the same experimental ROM where practical. Measure owner-local time, MISC, whole-frame WORK-H, and presented-frame cadence. Use the project’s accurate melonDS configuration, and do not turn a focused favorable window into a full-match claim. The retained particle experiment itself demonstrates why this matters: its focused improvement was materially larger than its full-run paired mean saving.

Correctness and resources. Exercise simultaneous effects at different ages, camera transitions, distant KO effects, mirrored/rotated transforms, translucent overlaps, scene teardown/reload, and the full admitted fighter/item workload. Preserve all required native content, allocation margins, and failure witnesses. Final target builds must not retain a compatibility-renderer fallback.

Bottom line

The best MISC redesign is a prebound, ordered native-packet renderer with per-transform cached draw data and immutable prepared textures.

The first concrete work is to remove repeated particle square roots, stop rebuilding fixed native effect programs during each draw, and stop converting DamageSlash texture frames inside drawing. Those are identifiable deletions in the current source—not hypothetical benefits from a wholesale engine rewrite.

Then extend the existing Whispy packet approach to the general particle path, and use C or small ARM kernels for what remains. That is the direction I would pursue before sacrificing effects, reducing content, or spending another optimization cycle on work that already landed.