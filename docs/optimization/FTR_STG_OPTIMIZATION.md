FTR is still enormous: about 390K ticks/frame. The latest measurements break it down roughly like this:
FTR component	Cost	Share
Display-contract capture	~54K	14%
Actual fighter draw	~331K	86%
↳ pre-submission work	~172K	~52% of draw
↳ state replay + geometry submission	~159K	~48% of draw


That makes FTR one of the clearest remaining places where the architecture itself is too expensive.
The especially interesting piece is the ~172K pre-submission half. It contains things like collection/traversal, owner preparation, matrix preparation, material preparation, validation/policy, etc. Much of that exists because we're adapting the N64-style object/rendering model to the DS every frame.
We've already proven several important things:
Fighter DObj topology is constant throughout the match.
Owner validation already hits ~99.95%, so re-validating it isn't useful.
The baked structural fighter draw plan is valid and already landed, although that first step only saved roughly 6–9K on ordinary frames.
Shading is basically spent.
Actual triangle emission isn't grotesquely inefficient at ~262 ticks/triangle.
The expensive part is increasingly the architecture around preparing those triangles.
So I would eventually stop trying to optimize the existing FTR renderer one function at a time.
I think its desired end state should look much closer to:
Fighter asset at build time
        ↓
DS-native fighter draw program
        ↓
compact immutable topology/material data
        ↓
fixed animation pose/matrices
        ↓
patch only genuinely dynamic values
        ↓
direct GX submission
Rather than:
DObj/MObj trees
   ↓
walk
   ↓
validate
   ↓
discover parts
   ↓
build renderer state
   ↓
resolve materials
   ↓
prepare matrices
   ↓
emit
That's where I think a real FTR rewrite could plausibly remove tens of thousands to >100K ticks/frame.
But there is an important warning: I would not revive the old giant fighter FIFO-template mechanism unchanged. That experiment already regressed by about +124K ticks. A successful rewrite has to delete working-set traversal and policy work, not replace it with huge patch tables and a mountain of per-frame patching.
The fixed-point animation representation we just discussed fits directly into this architecture: fixed pose → compact matrix preparation → direct native fighter draw.
STG: optimize/specialize, probably don't wholesale rewrite
STG is a different story.
Current steady-state STG is roughly:
200–215K ticks/frame
and is amazingly flat:
under-gate: ~200,013
over-gate: ~201,512
discriminator: only +1,499
So STG absolutely consumes meaningful CPU, but it isn't producing the expensive-frame population.
The old stage architecture has also already been stripped down considerably:
Stage prepare cache: 99.9% reuse.
Generic stage DObj traversal: only 8 calls over the entire match.
Existing NDS_R2_STAGE_DIRECT, stage view/projection specialization and preflight are already enabled.
The remaining steady-state STG owner is primarily ndsRendererAdapterCommitNativeStageDisplay, called about 27.5 times/frame.
That's encouraging because Dream Land is an almost perfect AOT target.
I would push STG toward:
Dream Land source assets
        ↓ build time
final DS geometry
final DS textures
final DS material state
baked world transforms
pre-sorted draw segments
        ↓
very small per-frame dynamic patch
        ↓
GX submission
The CPU should know practically nothing about the original Dream Land rendering structure during gameplay.
There are even two existing candidates pointing in this direction:
NDS_TASK51_STAGE_NATIVE — 42 baked world matrices, currently gated on visual equivalence.
NDS_DREAMLAND_DS_MESH — further DS-native stage geometry specialization, also awaiting visual qualification.
So for STG I would say finish the transition to a native stage asset, rather than write a brand-new general stage renderer.
A realistic first target is probably getting it from ~200K toward or below the existing 180K budget. If a truly static Dream Land path can go substantially below that, great, but I wouldn't currently assume there's another 100K hiding there.
Where I'd ultimately like them
My architectural targets would be approximately:
Bucket	Current	Reasonable next goal	Aggressive eventual goal
FTR	~390K	<300K	~200–250K
STG	~200K	<180K	~120–160K


Getting FTR to ~250K and STG to ~150K would save roughly 190K ticks on essentially every frame.
And that matters more now than it appeared earlier. With nearly half the frames over the 1.12M line, flat savings are valuable again even though neither FTR nor STG explains the difference between cheap and expensive frames.
So I'd separate the objectives:
To attack P95 immediately: simulation/SINT → SPHD/SHDT/SCPU.
To make the whole engine genuinely DS-native and create another ~150–200K of permanent headroom: rewrite/specialize FTR first, then finish stripping STG down to an AOT static-stage submission path.