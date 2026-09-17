FTR: the redesign I would pursue

The strongest direction is a prebound native fighter program that receives pose data directly—not another cache wrapped around the existing adapter.

I found several concrete opportunities in the current code. The important distinction is that fighter geometry is already native and packet replay already exists, but the surrounding work still reconstructs information, validates identities, converts representations, and sometimes rebuilds an entire packet. Those are the remaining architectural targets.

I reviewed master at 430aca2. The performance figures below come from the repository’s recorded measurements; I did not build or benchmark a new ROM in this review. My recommendations distinguish confirmed work in the current implementation from savings that still require measurement.

The shortlist
Priority	Candidate	What changes	Main opportunity
1	Prebound fighter submission	Replace recurring adapter reconstruction and content hashing with a persistent, typed render binding	Lower steady-state CPU work
2	Direct pose-to-packet matrix production	Produce the renderer’s required matrix representation without repeatedly passing through intermediate objects and formats	Remove conversion, copying, and traversal
3	Lossless GX packet compilation	Optimize the actual command stream, not merely the code that submits it	Reduce FIFO traffic and geometry-engine work
4	Root-local variants and invalidation	A changing face, material, or model part updates its own fragment rather than rebuilding the whole fighter packet	Reduce expensive miss frames and P95
5	Direct-index Link texgen patches	Replace repeated small-cache searches with generated local indices	Concrete, bounded, relatively low-risk CPU reduction
6	Versioned lighting and tint parameters	Recompute derived values only when their actual inputs change	Smaller supporting reduction

Candidates 1–4 form one coherent architecture. Candidates 5–6 are independently testable pieces—not substitutes for that redesign.

1. What the current evidence actually establishes

The latest qualified checkpoint recorded in the repository is:

Metric	P50	P95
Whole-frame WORK-H	1,584,128 ticks	2,310,848 ticks
FTR	356,608 ticks	740,352 ticks

Against the documented 1,120,000-tick gate, the whole-frame gaps are 464,128 ticks at P50 and 1,190,848 ticks at P95. These are calculations from that checkpoint, not new measurements. The project also explicitly requires four-player 30 FPS without switching simulation to 30 Hz.

Two conclusions follow.

First, FTR needs a substantial reduction, particularly in expensive frames. A sequence of tiny changes is not a sufficient strategy for this gap.

Second, FTR is only part of the solution. Its optimization must contribute to the combined STG/SRC/FTR/MISC effort. We cannot subtract independent bucket percentiles from whole-frame percentiles to predict the result: those percentiles need not describe the same frames.

I also do not accept the repository’s “optimization is exhausted” conclusion as an architectural proof. Its experiments establish that particular implementations failed or were too small. They do not establish a lower bound for a different representation and execution model. The measured GX-compose regression is valuable evidence; it is not proof that every possible fighter pipeline is exhausted.

Several obvious suggestions would repeat completed work

The current implementation already has asynchronous fighter-packet DMA, zero-copy display-memo hits, replay prechecks that skip material preparation, a prechecked path that bypasses whole-owner preflight, fixed-point pose evaluation, body-pose holding, hardware lighting, and an ARM block-copy matrix helper. Those are not new candidates.

The current hot path is closer to:

Existing source/pose state
    → display-contract / draw-plan handling
    → native matrix preparation
    → live material identity
    → packet input refresh
    → packet key and residency checks
    → dynamic matrix / texgen / tint patches
    → cache flush and asynchronous DMA

Packet miss:
    → additional material preparation / native execution / recording

That means optimizing the native triangle emitter alone can miss the ordinary replay path entirely.

Some remaining costs are directly visible

The recorded profile contains 129 regions. Applying its documented cycles / (2 × regions) conversion gives these approximate exclusive CPU-symbol averages:

Symbol	Ticks per profiled frame
ndsFighterMarioFoxDLAllDrawForSlot	45,692
ndsFighterPacketTryReplay	24,370
ndsFighterPacketStoreSplitModelview	12,040
ndsFighterDisplayContractSubmit	11,687
ndsFighterPacketPatchTexgen	8,072
ndsRendererAdapterMaterialAnimHash	7,866
ndsFighterPacketBuildKey	7,700

These are not guaranteed savings, inclusive subsystem budgets, or a complete accounting of FTR. They do show that substantial CPU work remains around the packet itself.

2. Candidate 1: prebound fighter submission
The confirmed problem

Even after the existing draw-plan and packet optimizations, the adapter still prepares matrices, computes live material identity, refreshes packet inputs, and asks the packet system to validate them. ndsFighterPacketBuildKey then walks roots and hashes properties such as root offsets, render modes, material counts, and hierarchy shape.

Much of this is information about what the fighter is, not what changed this frame.

The architecture should separate those two categories.

Proposed replacement

Create a persistent, per-instance bound fighter render object when the fighter’s render topology is established. Bind directly to:

The selected native root programs and their matrix producers.
Material and texture resources.
Dynamic parameter destinations inside the packet.
Explicit lifetime and mutation generations.

The ordinary draw becomes conceptually:

Bound fighter program
    + current pose outputs
    + changed material parameters
    + current camera/light parameters
    → patch existing native packet
    → submit

This is not “add another cache.” The objective is to remove the old reconstruction path from ordinary draws once the binding is authoritative.

I would implement this with generated typed arrays and small C executors, not a new general-purpose command interpreter and not giant generated functions for every animation.

The essential difference from today’s draw plan

The existing plan caches useful derivation results, but those results still feed the current adapter. The new object should be the renderer-facing contract itself.

For example, instead of rebuilding production_roots[] sufficiently to prove a packet match, retain the relevant root bindings and update only their dynamic fields. Instead of hashing unchanged static preambles, change a generation when a writer changes that state.

The current packet key identifies exactly which dependency classes need to be represented: owner/detail/appearance, generated tables and arena lifetime, root preambles and shape, and texture placement.

Correctness requirements

A generation number is only useful when every relevant writer participates. The implementation must cover model-part changes, costume/shade changes, material animation, detail changes, copied Kirby assets, asset replacement, and scene-arena reuse.

During development, retain the current content-derived checks as a shadow validator. A supposedly unchanged generation that disagrees with the old predicate is a bug—not permission to draw stale state.

Expected benefit: recurring identity reconstruction, repeated input publication, adapter branches, and related cache traffic.

Main uncertainty: how much of the driver’s measured cost remains after the bound path is genuinely independent. Adding generation checks while retaining all the old walks could easily make it slower.

First implementation target: ordinary packet-hit battle draws through ndsFighterMarioFoxDLAllDrawForSlot, with an explicit proof that the old material-identity and packet-key walks no longer execute on that path.

3. Candidate 2: direct pose-to-packet matrix production
The confirmed problem

The code already has a compact pose engine, but it publishes pose values into DObj fields. The renderer subsequently prepares matrices from the object hierarchy and writes matrix words into the packet. There are also distinct matrix paths: ordinary flat composition and a source-precision path needed for cases such as animation locks and qualified source-world rendering.

The current replay then copies or converts each root’s matrices again. ndsFighterPacketStoreSplitModelview alone accounts for about 12,040 exclusive ticks per profiled frame in the recorded window. That figure excludes work performed earlier to produce its input.

Proposed replacement

Give the renderer a compact, indexed pose/matrix interface and produce the final packet-compatible matrix words as the output of matrix preparation.

The first version should be deliberately narrow:

Keep gameplay-facing source data and all simulation timing unchanged. Replace only the renderer’s consumption path.

The bound program identifies the required joints, the required intermediate parents, their arithmetic class, and their packet destinations. Matrix evaluation then writes its final result directly into those destinations, subject to the packet’s DMA lifetime.

This can remove a combination of:

Reading scattered object fields to rediscover transform inputs.
Repeated conversion through renderer-irrelevant representations.
Publishing arrays of pointers solely for another function to consume.
Copying finished matrices into a second home.
Scaling the same final homogeneous row in a later pass.

The benefit comes from fusing producer and consumer, not merely replacing memcpy.

Preserve the difficult paths explicitly

The source-precision and animation-lock paths are not interchangeable with ordinary TRS composition. The current implementation explicitly handles warm gameplay matrices, locked transforms, accumulated scale, and conversion at the selected binding boundary. A redesign must preserve those semantics rather than forcing all joints through a cheaper but incorrect kernel.

I would use a few explicit producer classes—for example, ordinary native TRS, source-precision locked transforms, and qualified attachment transforms—with generated bindings to each class.

There is another trap: the current world-scaled split modelview is not simply an ordinary affine matrix. Its complete homogeneous row is scaled, including the final element. Blindly replacing its 4×4 load with a 4×3 load would change the implicit homogeneous component and can change the image.

Similarly, bypassing a Q-to-float-to-angle conversion requires proving that the direct conversion produces the intended result. “Both are fixed point eventually” is not sufficient.

Why this is different from the rejected GX-compose experiment

This proposal keeps composition on ARM9 where appropriate and removes representation traffic around it. It does not add matrix-stack operations to move the hierarchy into GX.

That distinction matters: enabling the existing GX-compose path on the measured four-fighter workload regressed WORK-H by 22,848 P50 and 67,456 P95 ticks.

Expected benefit: matrix preparation plus serialization, with possible later sharing of qualified attachment results.

Main uncertainty: the remaining conversion and memory costs versus the additional persistent state. This must replace existing storage/work rather than add another full palette beside it.

4. Candidate 3: compile a smaller, lossless GX packet

This is the strongest candidate for improving the work being sent to hardware, rather than only reducing ARM9 overhead.

The confirmed problem

The packet recorder emits normal/color information and uses VTX_16 for its vertex tails. Its prepare hook also records explicit run state. Packet replay then sends that recorded stream without a whole-stream optimization pass.

A native stream is not necessarily a minimal native stream.

Proposed compiler passes

Exact vertex-command selection. At build time, select a shorter command when it reproduces the exact decoded coordinate sequence: VTX_XY, VTX_XZ, VTX_YZ, or VTX_DIFF; use VTX_10 only when its precision is sufficient without changing coordinates. Otherwise retain VTX_16.

The DS command set supports these alternatives; VTX_16 takes two parameter words, while these alternatives take one. The documented geometry timings also differ slightly.

This does not require removing triangles or reducing model detail.

State-aware command elimination. Remove redundant texture, palette, color, and material writes when the complete relevant state proves them redundant.

Normal-command elimination where semantically valid. A repeated normal word is not sufficient proof. NORMAL computes lighting-dependent color and interacts with matrix and texture-generation state, so the optimizer must track those dependencies.

Projection and matrix-state simplification. Exploit invariance within a compiled program, without adding a per-frame content comparison. Preserve every matrix change that can affect a following vertex or normal.

Packed-header reconstruction. After optimization, rebuild command headers and every dynamic patch offset. A correct geometry stream with stale matrix or tint patch indices is still a broken packet.

Important restrictions

Do not reorder translucent geometry to improve batching. Do not merge primitive boundaries merely because two runs share a texture. Do not assume a matrix restore preserves the current vertex’s relevant interpretation without checking the full command semantics.

Also, this is not a new suggestion to “use strips.” The production emitter already has primitive-group support. The proposed work is an optimizer over the resulting stream and its state dependencies.

The repository previously tried runtime projection-load elision and recorded a roughly 3,008-tick FTR P50 improvement that was rejected as too small. I would not repeat that as a standalone project. Compile-time elimination across the actual packet is a different mechanism and belongs in the broader stream optimizer.

Expected benefit: fewer packet words, less transfer traffic, less flush traversal from a smaller packet, and potentially fewer geometry operations.

Main uncertainty: how much of the current workload is sensitive to those reductions. A 20% smaller packet is not automatically a 20% faster FTR bucket.

First experiment: optimize captured packet programs on the host, verify the decoded vertex/state sequence, and compare whole-frame timing with the original stream. Keep runtime search and optimization out of the shipping draw path.

5. Candidate 4: stop treating a local change as a whole-fighter packet miss
The confirmed problem

Each battle slot has a resident packet. When its key fails, the current path invalidates that packet, arms recording, and executes the native production path to rebuild it. The miss accounting explicitly distinguishes changes in key words, root count, and texture residency.

That creates an architectural opportunity:

A face, texture selection, or one model part should not automatically require reconstructing unrelated geometry and state for the whole fighter.

The code proves that the mechanism exists. It does not, by itself, prove that this mechanism explains all of the current FTR P95 tail. That must be established by correlating misses with expensive frames.

Proposed replacement

Compile the fighter as root-local native variants plus dynamic parameters.

Separate changes into three categories:

Change	Proposed response
Matrix, light direction, tint, supported UV parameters	Patch parameter words
Texture/material binding change	Update the affected resource binding and command fields
Actual root-program or model-part change	Replace the affected compiled fragment

Avoid a whole-fighter template for every possible combination. That creates a combinatorial ROM/RAM problem. Share the unchanged geometry and factor variants by the actual source root or material dependency.

For initial submission, I would keep one contiguous packet per fighter, assembled into the existing per-slot storage. Do not introduce hundreds of small DMA transfers to obtain fragment sharing.

Memory and compatibility boundaries

The recorded four-fighter checkpoint has only 111,680 bytes of heap low-water. A template bank cannot be justified by saying the ROM can be large; the active data must fit alongside gameplay and required reserves. Replace existing representations where possible, and measure the worst legal roster—not just Mario/Fox.

Also, Captain HIGH is explicitly excluded from FIFO-only replay because its alpha-test state is not represented by those FIFO words. A universal packet redesign must either represent that requirement correctly or retain a separately qualified native execution path. It must not silently force that case into the ordinary replay contract.

Expected benefit: fewer expensive rebuild frames and less work per necessary rebuild.

First measurement: for each expensive FTR frame, record which fighter missed, the reason, affected roots, and rebuild cost. That tells us whether root-local variants are a major tail fix or a smaller completeness improvement.

6. Candidate 5: replace Link’s texgen cache search with direct indexing

This is a particularly concrete finding.

What the code does now

ndsFighterPacketPatchTexgen walks patch sites and, for each site, linearly searches cached_dense[] for its dense vertex ID. It computes each unique UV only once, but repeatedly searches for previously computed values.

The code documents 28 unique dense vertices for Link’s generated texgen run in both details, with a 32-entry local cache. The recorded profile attributes about 8,072 exclusive ticks per frame to this function.

The replacement

Generate two arrays:

Unique inputs:
    local index → source normal / required texgen inputs

Patch sites:
    packet word index → local index

At runtime, calculate the 28 unique UV words using the existing math, then patch every site by direct index.

This changes the lookup portion from repeated searches to a linear evaluation-and-copy pass. Validation of indices belongs at generation/binding time, with development assertions retained.

Do not change the UV math in the first implementation. This candidate should isolate data access and lookup removal.

Expected benefit: a fraction of the measured 8,072 ticks, not all of it—the actual coordinate calculation remains.

Why I would do it: it is specific, bounded, and easy to distinguish from a placebo. It also exercises the exact generated-patch machinery needed by the larger program redesign.

7. Candidate 6: make lighting and tint genuinely change-driven

The replay path currently derives a light word with a square root and three divisions when its recorded light site is active. Tint handling first hashes root primitive colors to determine whether its derived material words need updating.

I would make these explicit derived parameters of the bound program.

For lighting, retain the last exact input direction and resulting normalized word. Reuse it when the input is unchanged; share it across fighters only when the coordinate-space contract and inputs really are identical.

For tint, advance a version when primitive color or modulation actually changes. Patch the affected shade sites without first walking and hashing every root merely to rediscover that nothing changed.

This must preserve the distinction between a light’s input direction and the matrix state under which the hardware consumes it.

Expected benefit: smaller than the structural candidates.

Recommendation: fold it into prebound submission. Do not make “cache one normalization” the next sprawling optimization campaign.

8. Where C and ASM should be used

Use assembly at a surviving, measured kernel—not as the architecture.

The pose evaluator is already compiled in ARM mode specifically so its SMULL and CLZ operations inline. The packet matrix-copy helper already uses ARM block transfers. A proposal to “rewrite those in ARM” would miss the current implementation.

The most defensible new assembly target is a fused final matrix producer/serializer: retain intermediate values in registers, apply the required final rounding/scaling, and store directly to packet destinations.

Start with a small C implementation and inspect the emitted code. Write assembly only where it removes identified spills, redundant loads, or representation work.

Likewise, avoid generating enormous per-character functions. The current profile shows a 9,752-byte draw driver and an almost-full 32,632-byte ITCM section. Replacing data traffic with an instruction-cache problem would repeat an existing failure mode.

9. Work I would not reopen

GX hierarchy compose, unchanged: measured slower on the relevant four-fighter workload.

Another wrapped float-to-fixed collision conversion: the repository already implemented and measured that route; its arithmetic improvement was canceled by other costs. The corrected evidence also shrinks the previously claimed collision-math opportunity substantially.

Lowering simulation to 30 Hz: explicitly disallowed by the current owner instruction. Existing body-pose holding is not new savings.

Assuming packet texture use is missing from LRU accounting: the current replay already touches its texture entries; that earlier tail problem has an explicit fix.

The generation-based transform invalidation work is still relevant, but it is already identified on the board—not a new discovery from this review. Its recorded 474.5 clears versus 14.3 recomputes per frame justify investigating the scattered writes. However, replacing the clear requires handling all validity readers, not merely changing the port-side invalidation function. Treat it as adjacent SRC/gameplay work and do not double-count it as a new FTR saving.

10. Implementation order and the evidence required

I would execute the work in this order:

Step	Deliverable	Decisive evidence
1	Direct-index texgen plus a host packet inventory/optimizer prototype	Same UV/vertex/state results; actual removed searches and commands
2	Bound packet-hit submission	Ordinary hits no longer execute the old identity/input-reconstruction path
3	Root-local variants	Measurably cheaper miss frames and improved tail, without excessive residency
4	Direct pose-to-packet matrix producers	Old conversion/copy stages disappear; visual and attachment equivalence remain
5	Targeted C/ASM and placement work	Improvement survives the whole-frame gate, not just a leaf benchmark

The final gate should evaluate the same presented frames, with the same gameplay workload, and report whole-frame timing alongside FTR. Packet hits and misses need separate distributions, followed by the combined distribution that the player actually experiences.

Validation must include four-player stress, native failure/rejection counters, resource lifetimes, detail changes, animation locks, hitlag, copied assets, attachments, material animation, and scene reuse. A lower FTR number does not count when the cost moves into STG/MISC or the next FIFO writer. The repository already contains examples where a large bucket improvement barely moved the whole-frame result.

Bottom line

There is still identifiable work to remove from FTR. The most promising target is the compatibility and reconstruction machinery surrounding an already-native packet renderer.

My recommendation is to build toward:

A bound fighter program, fed by direct pose outputs, with root-local variants and a losslessly optimized GX stream.

That is materially different from repeating packet DMA, matrix-stack offload, another memo, or another numeric wrapper. It attacks steady-state overhead, packet-miss cost, and hardware command volume together—while preserving the native-only renderer and 60 Hz simulation.

The evidence supports pursuing those candidates. It does not yet support promising that their combined savings will close the 30 FPS gate. The next useful result is a replacement path that demonstrably deletes work from real four-fighter frames, not another theoretical speedup assigned to an existing function.