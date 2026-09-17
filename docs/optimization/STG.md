The strongest STG redesign candidate is to replace the remaining per-run execution machinery with complete, ordered native stage command programs—not merely make the existing vertex submission loop faster.

The next substantial target is how painter depth is represented: the current native path can reload an entire projection matrix for each triangle just to assign its synthetic depth. There are also narrower opportunities in mutation-driven stage preparation and the per-vertex matrix path used by mixed-binding stage geometry.

I reviewed the repository at 430aca2879e9071dc2b22f944f5c2909c9ce7aa4, including the stage adapter, native stage executor, replay path, frame accounting, current performance board, and earlier stage experiments. These are source-grounded candidates, not newly measured speedups. I did not build or run a ROM.

My priority order is:

Priority	Candidate	Work it would actually remove	Assessment
1	Complete ordered stage command programs	Per-run CPU setup, repeated state handling, bookkeeping, and submission boundaries	Best first implementation candidate
2	Mutation-driven compact stage state	Repeated inspection and reconstruction of unchanged stage inputs	Complements #1; do not add another generic cache
3	Painter-depth grouping with correctness certificates	Redundant per-triangle projection changes; potentially unlocks better batching	Most interesting geometry-side redesign
4	Direct deformation recipes for mixed-binding geometry	Per-corner CPU projection and matrix-as-a-vertex submission	Strong stage-specific opportunity, especially Inishie strings
5	Visibility-independent packet storage plus offline mesh lowering	Submission of noncontributing geometry and redundant transformed vertices	Conditional; must avoid earlier failed probe designs
6	Exact compact vertex encoding	Command-buffer traffic, not necessarily geometry execution time	Low priority; an earlier rejection contains a technical error

I would implement #1 first, use #2 to simplify its inputs, and investigate #3 as the larger representation change.

1. First, the existing evidence does not justify “STG is solved” or “STG is purely GX-bound”
The current gap is substantial

The current execution board’s last qualified retained boundary reports:

Metric	Reported work	Gap against 1,120,000 ticks
WORK-H P50	1,584,128	464,128
WORK-H P95	2,310,848	1,190,848

Those gaps are approximately 29.3% of median work and 51.5% of P95 work. The board also distinguishes this qualified result from later payload changes still owing runtime proof. These should not be presented as fresh measurements of every change at the pinned commit.

Consequently, a legitimate STG improvement is useful, but we should not claim that a modest STG optimization alone closes the four-fighter 30 FPS goal.

STG is not simply “time spent drawing stage triangles”

The current presentation path charges ndsRendererAdapterPrepareNativeStageOwner() to STG, then accumulates stage traversal and finish work. Preparation includes admission, matrix handling, materials, configuration, and visibility state—not just mesh submission. Conversely, several stage-related actors enter the rejected-stage-classification branch and are charged through MISC.

That distinction matters: optimizing an acid/barrel/effect path might improve the game without reducing the bucket called STG.

The historical stage conclusions contradict one another

The July Task 54 analysis interpreted Task 53’s results as a largely fixed GX-throughput floor:

STG decreased by 187,648 ticks.
OTHR increased by 174,720 ticks.
ALL P50 barely changed.

But subsequent experiments weakened that explanation. Task 55 removed redundant command words without the predicted frame improvement. Task 100 then documented that neither word count, triangle count, nor pixel coverage explained the large remaining fixed cost; it specifically identified per-operation/per-run scaffolding as unresolved. Its quarter-resolution experiment changed WORK-H P50 by +256 ticks, not an improvement.

My conclusion: the old experiments reject particular implementations and predictions. They do not establish that all remaining stage work is irreducible.

There is another measurement issue: ALL results clustered around a presentation interval cannot, by themselves, prove that useful CPU work did not decrease. The decisive evidence must include WORK-H and the presentation-interval distribution, not just an unchanged cadence-quantized median.

Also, I would not use the Task 103 phase figures in source comments as established current measurements. The current closeout explicitly describes that instrument as broken and unproven.

2. Candidate #1 — Compile complete ordered stage command programs
The specific remaining inefficiency

The current replay is not a complete replacement for the stage execution machinery.

ndsRendererTask36ReplayRun() still:

Validates the selected replay run.
Calls ndsRendererNativeStageBeginRun().
Transfers that run’s command buffer.
Reconstructs renderer bookkeeping.
Calls ndsRendererHardwareEndBatch().

BeginRun() itself still handles texture state, polygon state, alpha-test state, matrix-path decisions, and batch transitions. In the DMA arm, each replay run starts a transfer and immediately waits for completion. Geometry replay exists, but the CPU still drives the surrounding run protocol.

This is why “add command caching” is not a new recommendation. The new boundary should be larger.

Proposed architecture

Compile a complete ordered program for each stage segment that can execute without an intervening external display callback.

The program contains the actual required GX commands, including texture/palette parameters, polygon attributes, matrix operations, and geometry. It also contains compact relocation/patch records for genuinely dynamic values.

The runtime becomes:

Stage load:
    Validate the generated stage program.
    Resolve resident texture/palette handles.
    Resolve dynamic bindings and patch locations.

Each presentation:
    Update changed stage inputs.
    Patch only affected command fields.
    Submit ordered segment programs.
    Publish the segment's final renderer state.

The first version should preserve the existing geometry and draw order. This is not a global material sort.

The critical change is:

Run boundaries stop being mandatory CPU function-call boundaries.

A segment containing several material runs can still contain several BEGIN/polygon/texture changes in its GX stream. It simply does not need to reconstruct those transitions through the general CPU-side run interface every frame.

What this could remove

The target is the sum of:

Repeated CPU-side state interpretation.
Texture-entry metadata touches repeated across runs.
Repeated batch bookkeeping.
Multiple DMA setup/completion boundaries.
Repeated reconstruction of the hardware-state shadow.

This differs from removing redundant COLOR words. It targets CPU execution and memory traffic around the stream, which the historical word-elision experiment did not isolate.

Important implementation constraints

Preserve display ordering. Fuse only across boundaries with no intervening fighter/effect/interface work.

Do not treat all graphics registers as FIFO commands. Frame-wide render controls and GX-stream state need different treatment. Hoist controls only when their values are proven compatible across the segment.

Preserve texture residency. Resolve/pin required resources before submitting the segment. A packet cannot retain stale palette or texture addresses after eviction.

Publish correct exit state. Subsequent owners must see an accurate matrix, texture, polygon, and batch state—or an explicitly invalidated shadow.

Reuse existing packet storage where possible. Do not keep a second permanent copy of the stage’s entire command stream merely to avoid changing the existing representation.

The DS has one geometry command path, and FIFO-full CPU writes can stall. Fewer CPU submission boundaries therefore do not automatically imply faster geometry processing; they must produce a measured reduction in total work or better overlap.

How I would qualify it

The first prototype should retain the same effective GX operations and geometry, changing only their CPU packaging.

That gives a clean test:

Does removing live per-run setup reduce WORK-H when the rendered work remains equivalent?

If it does, that is a real architectural result. If STG falls but equivalent work appears elsewhere, stop expanding it.

Priority: highest. It attacks an identifiable live path without first changing stage appearance.

3. Candidate #2 — Replace remaining stage polling with mutation-driven compact state
What already exists—and should not be rebuilt

The adapter already has substantial optimization:

Generation-based steady-state admission.
Cached topology.
Separate rigid/dynamic binding lists.
Rigid-world validation with a stride option.
Specialized matrix preparation.
Prepared/replayed segment reuse.

The current owner preparation also already avoids some initialization work on replay hits. Therefore, “cache static matrices” and “skip all preflight on hits” would largely repeat existing work.

What still happens

The live preparation path still calls world validation, matrix preparation, and material preparation; reconstructs resolver/configuration fields; republishes frame pointers; and scans binding flags to build hidden-binding state. Material preparation walks the stage’s material slots and follows their source bindings.

The next simplification is not another memo around those functions. It is to change who tells the renderer what changed.

Proposed architecture

Maintain a compact native stage instance with separate dirty domains:

Topology/residency changed
Camera changed
Binding transform changed
Material/texture frame changed
Visibility changed

Stage animation and stage-actor update seams mark those domains. Presentation consumes the dirty set and clears it after updating the native state.

For example, camera movement should not imply rebuilding stage-local transforms. A flower animation should not imply revisiting every rigid binding. A material frame change should not imply rereading unrelated topology.

For the command program in candidate #1, the mapping becomes direct:

changed binding -> matrix patch locations
changed material -> texture/UV/color patch locations
changed visibility -> execution mask
changed camera -> camera-dependent patch locations
Why this is a redesign rather than another cache

The current system often discovers change by inspecting the source representation.

The proposed system receives change notifications from its owners and touches only the native fields affected by those changes. Ideally, it replaces overlapping source-key/cache machinery instead of adding another layer beside it.

The difficult part: invalidation completeness

This is safe only when every relevant writer is covered.

That includes stage animation, actor callbacks, hierarchy changes, source visibility flags, scene replacement, and address reuse after a heap reset. Recycled pointers must not resurrect a previous match’s state.

For initial qualification, I would retain a lab-only comparison against the current preparation path and exercise every supported stage’s animation/hazard transitions.

Do not reduce simulation frequency to obtain this saving. The current campaign explicitly retains 60 Hz simulation.

Priority: high, alongside #1. The gain is unmeasured, but the intended runtime is simpler: small dirty sets feeding a small native program.

4. Candidate #3 — Stop assigning a distinct projection matrix to every painter triangle

This is the most interesting geometry-side opportunity I found.

The exact source behavior

ndsRendererNativeStageTask36LoadNoZProjection(projected_z) copies the projection matrix, replaces its Z column using the W column and the requested synthetic depth, loads the complete 4×4 projection, then switches back to modelview.

ndsRendererNativeStageEmitNoZTriangle() calls that path for eligible triangles. In other words, a full projection update can exist primarily to encode one triangle’s painter-order depth.

Replay avoids recomputing some of this on the CPU, but replaying an expensive representation does not remove its commands.

Proposed architecture: certified painter-depth groups

Instead of automatically allocating a different effective depth to every triangle, generate groups of triangles that can share depth without changing observable occlusion.

The most conservative starting case is:

Compatible material and transform state.
Compatible source depth behavior.
No coverage overlap requiring painter precedence.
No relevant interaction with another owner inside the group’s depth interval.

For those groups, load the painter projection once and emit multiple triangles.

An offline compiler could progressively consider larger groups using conservative overlap/order constraints. The runtime should not build a triangle overlap graph every frame.

Why this is not “turn depth testing off”

A blanket equal-depth conversion is unsafe. The DS distinguishes less-than and equal-depth tests, and polygon attributes take effect at primitive boundaries. Opaque/translucent ordering and polygon IDs introduce additional constraints.

The proposed compiler must preserve the required ordering relationships. It must not simply give an entire stage one depth.

Also:

Keep real-Z geometry separate.
Preserve source alpha and texture behavior.
Preserve external ordering against fighters/effects.
Preserve the depth allocator’s externally visible progression where later owners depend on it.
Treat shared-edge/antialiasing behavior conservatively.
Why this could outperform ordinary command compression

If T triangles currently require individual projection changes but can legally be represented by G groups, the structural saving is:

$$ T-G \quad \text{projection changes} $$

That removes complete matrix operations and associated transitions—not merely a few redundant state words.

It may also permit longer primitive batches and better strip construction within the safe groups.

What would disqualify it

If the apparent groups overlap under ordinary camera movement, interact with foreground effects, or depend on distinct depths for correct translucent composition, they are not eligible.

Priority: medium-to-high research, after #1. I would begin with one certified static stage region, not rewrite all painter-depth behavior at once. It is not automatically lossless merely because the grouping looks correct in one screenshot.

5. Candidate #4 — Direct deformation recipes for mixed-binding geometry
A particularly expensive current representation

The native owner file contains ndsRendererNativeStageEmitCrossMatrixTriangle() for geometry such as Inishie’s scale strings.

The path transforms each corner with its binding’s matrix, clips the triangle, then uses ndsRendererNativeStageEmitProjectedDepthVertex() to submit each resulting corner.

That vertex helper constructs a matrix whose translation contains the corner’s clip-space coordinates, loads that matrix, and emits a zero-position vertex.

The renderer is using a full matrix load as a way to submit one already-transformed vertex.

The reason is valid: corners can originate from different source DObj matrices. But that does not require this representation for every specialized actor.

Proposed architecture: generated live endpoint patches

For the string case, derive a native deformation recipe from its actual moving endpoints.

Choose a common coordinate frame, keep its ordinary camera/projection handling, and update the string’s corner positions from the live endpoint transforms. The triangles then use normal vertex submission under that common frame.

The important distinction:

This is not flattening the string into a static mesh. Its endpoint-dependent vertices remain live.

For a certified translation-only relationship, the update may reduce to a few scalar coordinate patches. More general relationships can use a small relative transform, still avoiding a clip-space matrix load per emitted corner.

Why this is attractive

It can remove all three of the following for the specialized geometry:

General per-corner clip-space transformation.
CPU clipping where normal hardware clipping is semantically equivalent.
Per-corner 4×4 matrix submission.

The original gameplay objects, collision, scale movement, and attachment relationships remain authoritative.

A secondary option: a small matrix palette

Where corners genuinely require different complete transforms, a few stored coordinate matrices and per-corner restores are another candidate. The DS supports indexed matrix store/restore, but this is not a guarantee of lower geometry execution time merely because the command payload is shorter.

I would prefer the direct endpoint recipe when its assumptions can be proven.

Priority: strong stage-specific candidate. It does not improve a Dream Land-only benchmark when this path never executes, so its benefit must be reported on the stages that use it.

6. Candidate #5 — Offline visibility and topology optimization without invalidating replay
Why earlier geometry experiments are not sufficient

Task 100 records that an earlier run-culling probe changed the run set and disarmed capture-once replay, producing a regression. That experiment did not cleanly measure the benefit of skipping already-prepared geometry.

A redesigned stage program should separate:

What geometry is stored from which stored chunks execute this frame.

Proposed architecture

Keep immutable command chunks keyed by geometry/material identity. Keep visibility in a separate execution mask.

At build time, generate conservative bounds and identify geometry that cannot contribute within a supported camera envelope. At runtime, select chunks without rebuilding or invalidating their stored commands.

Start with large, useful units. Do not replace a cheap draw with hundreds of tiny CPU culling decisions.

For source-transparent or two-sided geometry, visibility proof must include the relevant semantics. Hiding a flower’s back face because a solid wall’s back face was safely culled is not valid.

Combine with actual vertex reuse, not merely shorter encoding

After preserving depth/material/transform constraints, the offline compiler can consider triangle strips and equivalent topology lowering.

However, the old Dream Land census is an important warning. In its current-order analysis, 202 triangles used 606 vertices, and a greedy strip conversion projected approximately 522 vertices. That was a limited opportunity, not a dramatic mesh-wide reduction. The census also does not establish that position equality alone is sufficient: UV, color, matrix binding, and depth semantics must agree.

Candidate #3 could change which triangles can legally share a primitive group, but that expanded opportunity must be demonstrated—not assumed.

Priority: conditional. Useful when it removes substantial submitted work while leaving existing packets valid. Not a license for arbitrary decimation.

7. Candidate #6 — Revisit exact compact vertex encoding, but not as a major FPS promise

I found a concrete technical problem in the old VTX_10 rejection.

Task 55 E0 compares raw VTX_16 coordinate integers against a signed ten-bit range and concludes that most stage coordinates are out of range. That comparison ignores the formats’ different fractional precision.

The ten-bit vertex format is expanded into the internal coordinate representation with a six-bit shift. For example:

$$ 30272 / 4096 = 473 / 64 = 7.390625 $$

That particular VTX_16 value is exactly representable as a ten-bit coordinate. The relevant question is precision and exact representability, not whether the raw sixteen-bit integer exceeds 511. The emulator’s vertex decoder explicitly performs this expansion.

A valid implementation

An offline encoder can select a one-word vertex representation only when decoding it reproduces the original coordinate exactly. Otherwise it retains VTX_16.

It can similarly consider exact axis reuse, without changing vertex order or geometry.

Why this stays low priority

A shorter vertex encoding does not eliminate the vertex transformation. The earlier redundant-word experiment already warns against predicting frame savings proportional to bytes removed.

Conclusion: the historical range-based rejection is unsound, but that does not establish a meaningful FPS win. Treat this as a low-risk packet-size optimization to test after the larger architecture work.

8. Where C and assembly actually belong

I would use C for the new execution architecture and generated data for most specialization.

The first objective should be to make runtime work disappear:

No repeated interpretation of an unchanged run.
No reconstruction of immutable configuration.
No polling unchanged stage state when an owner can identify its mutation.
No full matrix-per-corner representation for a specialized deforming quad.
No per-triangle depth change where a certified group suffices.

Assembly is appropriate only after that, for a remaining measured kernel such as sparse command patching or a compact fixed-point transform.

I would not begin with an assembly rewrite of FIFO replay. The hardware can stall the issuer when the FIFO is full, so faster stores are not automatically faster frames. Nor would I move whole stage functions into ITCM without a placement-controlled comparison. The repository already records regressions from seemingly attractive code-placement changes.

Similarly, I would not repeat the four-fighter hardware-compose experiment. The current board records that enabling that existing mechanism regressed WORK-H P50 by 22,848 ticks and P95 by 67,456 ticks. A narrowly scoped two-endpoint stage recipe is a different proposition from adding matrix-stack traffic across all fighters.

9. How to turn these into evidence without another repetitive optimization campaign
Measure the current path without changing its placement

The failed Task 103 instrument is a good reason to put the initial attribution in the approved accurate emulator, rather than inserting another collection of hot ROM-side timers.

Trace the pinned executable’s stage function ranges and submission events using emulated timestamps. Distinguish CPU execution, memory/cache stalls, FIFO stalls, and DMA completion waits. Measure host-side tracing overhead only to ensure it does not alter emulated timing.

Do not infer current cost from the old comments’ phase numbers.

Use one small counterfactual for each candidate

For #1, retain equivalent rendering operations and replace only run-level CPU orchestration.

For #2, compare compact dirty-state output with the existing preparation result.

For #3, validate one painter-depth group against the full reference path across its camera envelope.

For #4, validate the endpoint recipe across the full scale/string motion and clipping range.

These tests should answer different questions. They should not be bundled into one rewrite whose result is impossible to attribute.

Keep both performance and correctness gates

The performance result must include WORK-H P50/P95, presentation cadence, and whole-frame bucket movement. Do not add independently calculated bucket percentiles together.

Correctness needs identical simulation/input state, camera and stage-animation coverage, transparency/depth checks, and proof that the native path engaged. Timing-dependent CPU matches can diverge, so independent screenshots hundreds of frames into two different runs are not a sufficient image comparison.

Finally, retain the project’s native-only and 60 Hz simulation requirements. A faster fallback renderer or skipped gameplay work is not an acceptable success.

My recommendation

Build an “ordered native stage program” executor first. Make its first version preserve geometry, material order, depth behavior, and external display boundaries. Its purpose is to eliminate the CPU machinery surrounding already-cached geometry.

Then replace its remaining source-graph polling with compact mutation-driven inputs.

After that, pursue painter-depth grouping and direct mixed-binding deformation as targeted representation changes. Those are more promising than repeatedly shortening the same FIFO loop, because they can remove operations the present representation requires.

The critical distinction is:

Do not optimize STG by making every existing step slightly cheaper. Reduce the number of steps that must exist at presentation time.

These candidates have concrete implementation seams and falsifiable tests. Their speedups remain unmeasured, and STG alone is not yet demonstrated to close the 30 FPS gap—but the source does contain architectural opportunities beyond the already-rejected “more caching, faster stores, fewer pixels” approaches.