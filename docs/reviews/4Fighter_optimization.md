Verdict

Reject the choice between “halve simulation” and “optimize rendering.” Neither one, by itself, explains a 50% total-frame reduction.

The right final architecture is likely:

A 30 Hz presentation loop containing two much smaller 60 Hz combat substeps, backed by match-resident data and fully packetized DS-native rendering.

That is materially different from both:

running the current entire source process graph twice per present; and
replacing two 60 Hz ticks with one naïve dt = 2 tick.

There is also a more immediate conclusion from the current repository:

Do not make the simulation-rate decision from the quoted profile. On current master, native-stage rejection and animation-cache starvation dominate the measurement.

Those have to be removed first. After that, packetized presentation and selective rate reduction should be measured together. A true one-sample-per-present 30 Hz combat simulation belongs later.

The supplied baseline is no longer the current baseline

The quoted WORK-H P50/P95 of 1,482,752 / 1,963,648 is the accepted August 21 Mario/Fox mirror measurement. The owning document now explicitly says that every timing number in that section is a mirror-roster figure and that the standing stress roster was later changed to four distinct fighter kinds.

Two structural mitigations have also landed since then:

Change	Four-CPU P50	Four-CPU P95
Fighter packet path	1,600,832 → 1,281,728	2,069,824 → 1,866,432
Compact pose engine + 30 Hz pose hold	1,264,512 → 1,244,608	1,836,800 → 1,777,408

The fighter packet therefore already demonstrated a 319,104-tick median reduction and a 203,392-tick P95 reduction. The pose work added another approximately 59K at P95.

More importantly, the latest banked four-distinct-kind run is currently much worse:

Bucket	P50	P95
FTR	965,440	1,013,248
STG	4,090,880	4,098,368
SRC	735,424	2,095,168
MISC	334,912	803,968
WORK-H	6,313,728	7,906,176

These bucket percentiles are not additive because their P95 observations need not be the same frames, but the scale is unambiguous. The same artifact reports a native-stage preparation rejection, reason 6, as the next lead.

That run also had:

battlePackResidentBytes = 0
1,425 animation-cache misses
1,420 animation-cache rejects
only a 163,840-byte animation-cache reservation

So the current timing run is simultaneously measuring a stage-owner route failure and an almost completely starved animation cache.

That does not invalidate the old mirror result. It means it is no longer the measurement from which to select the next architecture.

“Zero cadence violations” is misleading terminology

The histogram is the cadence result. Two VBlanks is on-time; every 3, 4, or 5+ is a missed 30 Hz presentation.

The counter named PacingCadenceViolationCount only increments when an interval is less than two VBlanks. It detects an illegally early presentation, not a late frame. Therefore, “zero cadence violations” alongside only 6.1% two-VBlank frames is internally consistent: no frame was presented too early, but almost all were late.

I would rename it conceptually in reports to something like:

early_present_violation_count

and treat the histogram as the performance acceptance metric.

The arithmetic rules out either single-lever answer

Using the handoff’s older WORK-H P95:

Current P95       1,963,648
Required P95      1,120,380
Required saving     843,268
Required reduction    42.94%

A 2× speedup of one subsystem saves only half that subsystem’s share. Therefore, for halving simulation alone to save 843,268 ticks, simulation would have to consume:

2 × 843,268 / 1,963,648 = 85.9% of the P95 frame

But the handoff already assigns approximately 26% to rendering, before audio, HUD, effects and miscellaneous work. So by its own attribution, simulation cannot be 85.9%.

Similarly, completely deleting a 26% renderer lane—an impossible upper bound—would save only about:

1,963,648 × 0.26 ≈ 510,548 ticks

leaving another approximately 333K ticks to recover.

For ALL P95, the requested reduction is almost exactly 50%. A 2× simulation speedup could deliver a 50% total-frame reduction only if essentially the entire frame were simulation, which it plainly is not.

So the structural answer must combine:

removal of fallback and gameplay-time I/O;
packetized presentation;
shared animation/matrix/collision preparation;
selective simulation-rate reduction;
smaller order-preserving collision work.
First priority: remove the fallback-contaminated profile

On current master, the first 50% is not a clever optimization. It is getting the intended fast paths back.

1. Restore the native stage owner

A steady stage lane around 4.09M ticks is not the normal price of Dream Land. The 128-frame discriminator records 60 failures at ndsRendererPrepareNativeStageOwner, all as reject reason 6. The current source-refactor plan explicitly identifies that rejection as the active performance lead and warns against casually changing the seam before it is understood.

The architecture should also be hardened so one changing component cannot drop the entire stage to generic rendering:

Dream Land immutable packet
    always native

Whispy / flowers / moving or animated pieces
    separate small dynamic owners

source effect and weapon DObjs
    separate transient owners

A dynamic leaf failing validation should not invalidate thousands of static stage commands. This is the stage equivalent of treating Fox’s gun as a sidecar rather than making the whole fighter generic.

2. Make post-GO resource reads impossible

The match-resident pack from the RAM answer is a prerequisite to meaningful performance profiling.

The acceptance condition should be:

native_stage_rejects_after_go == 0
mandatory_owner_fallbacks_after_go == 0
animation_cache_rejects_after_go == 0
asset_reads_after_go == 0

The current 1,420 / 1,425 reject count means status-transition timing is partly a storage benchmark. The current source breakdown reinforces that interpretation:

SHDT P95: approximately 9.7K
SINT P95: approximately 1.84M
SRC P95: approximately 2.10M

SINT contains much more than loading, but this shape is consistent with expensive status/interrupt work rather than ordinary pair engagement being the dominant current tail.

The pack should be optimized for cache behavior as well as capacity:

SIM-HOT
  status descriptors
  active move data
  collision and hurtbox descriptions
  compact animation/event tracks
  gameplay-joint tracks

RENDER-HOT
  packet patch tables
  final matrix arrays
  dynamic material words
  texture handles

COLD
  rare specials
  entry/results data
  infrequent model-part variants

One four-CPU profile attributed approximately 41.9% of .main execution cycles to memory stalls. Contiguous, pointer-light match data is therefore a performance feature, not merely a way to avoid an allocator failure.

Where the remaining reduction should come from

After stage and residency are clean, I would expect the remaining work to come from three structural changes.

1. Finish packetizing presentation

The current fighter packet already proves the direction. It prebuilds immutable GX command structure and patches the dynamic portions rather than reconstructing every command every frame. It saved about 319K at the median and 203K at P95 on the measured four-CPU arm.

The final fighter path should be approximately:

load time:
    choose low-detail owner
    load selected costume
    bind textures
    construct final packet
    build patch-offset table
    validate once

per presented frame:
    calculate final joint matrices
    patch matrix words
    patch flash/material/color words
    submit packet

The per-frame path should not repeat:

reloc-file ownership discovery;
model/display-list range validation;
full draw-plan construction;
source display-list scanning;
material reconstruction;
immutable packet-key work.

For a four-player match, only the low-detail fighter image needs battle residency. High-detail CSS or Results data can belong to their respective scenes.

Transient presentation needs the same architecture

The old effect-tail finding should not be discarded. It simply should not be confused with the current stage and cache failures.

Effects, weapons and particles should produce compact rendering records:

typedef struct NDSFxInstance {
    s32 x_q12;
    s32 y_q12;
    s32 z_q12;
    u16 scale_q8;
    u16 angle;
    u8 kind;
    u8 frame;
    u8 age;
    u8 flags;
} NDSFxInstance;

Then:

source gameplay effect/weapon event
    ↓
fixed DS instance pool
    ↓
bucket by atlas/material
    ↓
prebuilt quad/mesh packet
    ↓
few GX submissions

For effects whose GObj lifecycle is gameplay-relevant, keep the source GObj and replace only its display half. For purely cosmetic effects, the entire GObj/DObj/XObj presentation scaffold can be replaced by the compact pool.

Attached effects should consume the fighter’s already-computed joint position instead of independently traversing the fighter DObj tree.

The effect-pool peak—17 of 38 in the older arm—says only that capacity is sufficient. It says nothing about the cost of submitting each active effect. The repository has already refuted texture uploading as the steady effect cost: one upload in roughly 1,408 frames was only about 0.0071% of the effect lane.

2. Separate the gameplay skeleton from the render skeleton

A full visual skeleton does not need to run at 60 Hz merely because combat does.

Generate, per fighter, the dependency closure of joints used by:

hurtboxes;
attack hitboxes;
grabs and throw anchors;
reflectors and absorbers;
held-item/weapon anchors;
floor/contact tests;
gameplay-relevant attached objects.

Update that small gameplay skeleton at each 60 Hz combat substep. Update the complete visual skeleton only on the second substep, immediately before the 30 Hz present.

That gives:

substep A:
    root motion
    gameplay joints only
    hitboxes/hurtboxes
    physics and collision

substep B:
    root motion
    gameplay joints
    physics and collision
    remaining visible joints
    final render matrices

The existing pose engine already evaluates the body pose only on the final source update before presentation, so this is an extension of an accepted direction rather than a wholly new timing model.

The final matrices should be shared by:

collision;
rendering;
item/weapon attachments;
attached effects;
shadows and tags.

Do not independently rebuild the same hierarchy for each consumer.

3. Replace two full source updates with two slim combat substeps

This is the most important simulation distinction.

Current architecture
full gcRunAll / source update A
full gcRunAll / source update B
draw
Recommended architecture
capture substep-stamped input

combat substep A:
    gameplay timers/events
    status and interrupt logic
    gameplay-joint animation
    physics
    hit/catch/reflect/projectile resolution

combat substep B:
    same 60 Hz gameplay semantics

presentation update:
    full visual pose
    camera
    HUD
    audio service
    visual effects
    particles/backgrounds at selected rates

packetized draw

The two combat substeps retain 60 Hz gameplay time, but they no longer walk and update every visual, camera, audio and generic object process twice.

A load-time-generated process schedule can preserve source ordering while avoiding a general discovery/dispatch pass:

sim60_vector
present30_vector
cosmetic15_vector
event_driven_vector

Any process that affects gameplay remains in sim60_vector. Mixed processes must be split into gameplay and presentation halves before they can be decimated.

The existing source already exposes the two individual source-update timings at profile level, so the next clean run can measure update A and update B separately instead of approximating each as half of SRC.

Should the authoritative simulation become 30 Hz?

Probably not as the first implementation.

There are three distinct architectures hiding behind the phrase “30 Hz simulation”:

Architecture	Gameplay timing	Risk	Recommendation
Two complete source updates	Exact 60 Hz	Low, too expensive	Current
Two slim/fused combat substeps	60 Hz discrete semantics	Moderate	Preferred
One dt=2 combat update	Actual 30 Hz collision/events	High	Last resort

The middle option captures much of the savings without changing the discrete rules that make a fighting game feel correct.

The repository’s earlier two-fighter counterfactual found that subtracting half of SRC would move P95 by approximately 291,488 ticks. A selective variant that held AI and interrupt work at 60 Hz still projected a 121,248-tick improvement. Those results prove that rate separation is worth implementing, but they were calculated on a two-fighter, 94K-gap arm and did not include compensation cost. They are not a prediction for the current four-fighter configuration.

The existing NDS_TASK106_UPDATES_PER_PRESENT=1 route is explicitly only a half-speed pricing experiment. It does not implement compensated gameplay.

What a naïve 30 Hz tick breaks
Input

Sampling only once per 33.3 ms presentation:

removes one-frame input windows;
increases input quantization;
can miss a tap between presents;
changes shield release, jump, tech and attack timing.

Compensation: collect controller state on every VBlank into a tiny ring and associate each edge with substep A or B. Consume those samples in the two combat phases even though only one frame is rendered.

Frame-defined counters

These cannot safely become:

counter -= 2;
if (counter <= 0) expire();

A value of one expires in substep A; a value of two expires in substep B. Those can have different consequences relative to attacks and collisions later in the presented frame.

This includes:

hitlag;
hitstun and shieldstun;
invulnerability and intangibility;
landing and recovery lag;
ledge lockout;
jump squat;
rehit timers;
respawn timing;
match timer events.

Compensation: retain counters in original 60 Hz units and process the two expiry boundaries in order.

Physics and knockback

Multiplying movement by two is not equivalent to two source integrations.

For a source update like:

v1 = v0 + a
x1 = x0 + v1

two ticks produce:

v2 = v0 + 2a
x2 = x0 + 2v0 + 3a

A common naïve dt=2 implementation produces x0 + 2v0 + 4a, which is already different before friction, terminal velocity, floor contact or knockback are considered.

Compensation: either retain two cheap physics microsteps or generate an exact composed two-step operator for simple states. Any state containing a clamp, collision, callback or status transition should fall back to the two microsteps.

A hit during substep A must alter substep B. Processing the impulse only at the end of the present changes knockback position, landing, wall contact and subsequent interactions.

Hitboxes, grabs, shields and reflectors

A two-frame hitbox has two opportunities to connect in the source. A fast projectile or limb can cross a target entirely between 30 Hz samples.

Safest compensation: perform two narrow-phase collision samples.

A swept test is possible, but it must:

find the earliest contact substep/time;
preserve attack and target iteration order;
apply the result before later pairs are processed;
respect shields, grabs, reflectors and already-hit records;
then continue with the state produced by that contact.

That is substantially harder to qualify than two slim collision microsteps.

Hitlag and shield behavior

If a hit occurs in substep A, the participants may already be frozen during substep B. If it occurs in B, they are not.

The same issue applies to:

shield break;
reflector activation;
grab/capture;
armor;
damage state selection;
multi-hit rehit exclusion.

One end-of-present resolution cannot reproduce both cases.

Animation events

The render pose can be evaluated once, but animation script events still need to cross two source-frame boundaries. Otherwise a hitbox-start command, sound event, model-part change or motion flag can be skipped.

A good AOT animation representation separates:

event track: processed at 60 Hz boundaries
pose track: fully evaluated only for final presentation
gameplay-joint track: evaluated for each combat substep
RNG

This is the most easily overlooked correctness problem. The project’s earlier audit found one source LCG shared across AI, effects and particles. Skipping a visual or AI update changes the subsequent random values consumed by gameplay.

The safe order is:

Keep gameplay RNG sites at 60 Hz.
Have source event generation produce the random values a decimated visual system needs.
For purely cosmetic systems, either consume the skipped source draws or explicitly accept and isolate a cosmetic RNG stream.
Do not move CPU AI to 30 Hz until its RNG and behavior traces have been qualified.
Global process order

A fused executor must preserve:

tick A: all objects/processes in source order
tick B: all objects/processes in source order

It must not do:

fighter 1 tick A + B
fighter 2 tick A + B
...

because fighter 1’s second-tick decisions would then occur before fighter 2’s first-tick decisions.

The same rule applies to the six fighter pairs: all source-ordered pair work for A, then all source-ordered pair work for B.

The O(n²) engagement is not the first target

At four fighters, O(n²) means six pairs. This is a small fixed problem, not a scaling catastrophe.

In the latest full run, the instrumented SHDT lane is approximately:

P50     9,152 ticks
P95     9,664 ticks
max   608,960 ticks

Meanwhile SINT reaches approximately 1.84M at P95. This does not prove all collision is cheap—weapon and stage collision may be charged elsewhere—but it makes fighter-pair engagement a poor candidate for the first 50% reduction.

It is still worth adding an order-preserving broadphase once the clean P95 set says the lane matters.

Safe design

Keep the canonical source pair sequence:

(0,1), (0,2), (0,3), (1,2), (1,3), (2,3)

For each pair, in that order:

if (!active_interaction_mask(pair))
    continue;

if (!conservative_bounds_overlap(pair))
    continue;

source_narrow_phase_and_resolution(pair);

Useful rejection data includes:

fighter has no active attack, grab, shield or reflector;
target has no relevant hurtbox state;
union attack AABB cannot overlap union hurtbox AABB;
projectile owner/team rules reject the interaction;
vertical or horizontal coarse ranges cannot overlap.

The broadphase may generate a candidate mask in any convenient way, but the mask must be consumed in canonical order.

Two cautions:

It must have no false negatives.
If an earlier pair can mutate state used by a later pair, either test the later pair against current state at its source position or make the precomputed bound conservative enough to cover that mutation.

Do not sort pairs by distance or iterate spatial-cell order. That would alter hit/catch/reflect resolution.

The larger opportunity is usually not changing six into fewer than six. It is computing transforms and coarse bounds once per fighter per substep and sharing them across all six pairs, weapons, grabs and reflectors.

The 26% / 8% decomposition is not suitable for deciding architecture

There are three different quantities being mixed:

Quantity	What it answers
Accounting bucket	Where elapsed ticks were charged
Correlation/marker	What is present on expensive frames
Causal A/B reduction	What disappears when a path is removed

An effect DObj being present on P95 frames does not prove its draw cost owns the entire premium. Conversely, a renderer bucket with a flat global P95 may still contain a large removable steady cost.

Also:

P95(FTR) + P95(SRC) + P95(MISC)

has no useful meaning unless those values came from the same frame. Usually they did not.

The next clean census should record, for every presented frame:

update A
update B

fighter simulation
AI
hit/catch/reflect
physics and map collision
status/interrupt
animation events
gameplay-joint pose

stage draw
fighter draw
weapon draw
effect draw
particle draw
HUD/background
GX FIFO wait/backpressure
asset I/O

Then evaluate each proposed change in two stages:

Subtract its measured per-frame lane, re-rank all frames, and inspect the new rank-80 frame.
Confirm with a same-binary runtime route A/B so code placement does not masquerade as a win.

For tail diagnosis, report the median and distribution of each lane on the 80 frames that define P95, not merely that lane’s independent P95.

Ranked implementation plan
0. Restore a valid performance baseline
Diagnose and eliminate native-stage reject reason 6.
Make static stage ownership resilient to dynamic-owner failure.
Build the match-resident four-fighter pack.
Require zero animation rejects and zero mandatory reads after GO.
Run the real four-distinct-kind argmax roster.
Capture update A and B separately in the same run.
Re-establish the actual gap before approving a rate change.
1. Finish the 30 Hz presentation architecture
Keep the existing fighter packet path and extend it to every fighter.
Load only the detail level used by the match.
Eliminate per-frame immutable validation and reloc discovery.
Convert effect, weapon and particle drawing to compact native records.
Batch by texture/material and use prebuilt GX streams.
Split static stage packets from dynamic stage actors.

This is the highest-confidence fidelity-neutral work after the broken routes are fixed.

2. Unify pose, matrices and collision inputs
Generate each fighter’s gameplay-joint dependency closure.
Evaluate those joints at 60 Hz.
Evaluate the remaining visible joints at 30 Hz.
Build each world transform once.
Share it with hitboxes, attachments, rendering and visual effects.
AOT-compile animation events and collision descriptions into the match pack.

This attacks the measured per-fighter matrix cost without reducing combat sampling.

3. Introduce the dual-rate process schedule

Keep two 60 Hz combat phases, but move proven presentation-only work to 30 or 15 Hz:

Rate	Systems
60 Hz	input edges, gameplay timers, animation events, physics, hit/catch/reflect, projectiles, gameplay hazards
30 Hz	final pose, camera, HUD, audio service, most material animation, visual effect updates
15 Hz	cosmetic particles, backgrounds, non-gameplay stage decoration
Event-driven	texture binding changes, effect spawn, audio cues, palette changes

AI strategy can later move to 30 or 15 Hz while its generated controls are applied at 60 Hz, but that is a gameplay/simulation concession and must be qualified separately.

4. Optimize the remaining 60 Hz combat kernel
Cache current fighter bounds once per substep.
Use active attack/grab/reflect masks.
Add canonical-order broadphase rejection.
Use compact fixed-point data internally where the conversion boundaries do not erase the benefit.
Generate specialized per-fighter collision and status routines where profiling supports them.
5. Consider a true one-step 30 Hz combat simulation only if still necessary

At that point, implement it as a deliberate gameplay-fidelity change with:

substep-stamped input;
two event/counter boundaries;
composed physics operators;
swept or substepped collision;
exact hitlag/shield/rehit ordering;
preserved gameplay RNG calls;
a deterministic source-versus-candidate trace suite.

If most of those compensations remain necessary, the implementation has effectively become the preferred “two slim 60 Hz combat phases” architecture anyway.

Required equivalence suite

For each fighter and major interaction, compare the source and optimized paths at every 60 Hz semantic boundary:

status and motion
animation-event index
active hitboxes and hurtboxes
first hit/catch/reflect substep
damage and shield damage
hitlag, hitstun and shieldstun
position and velocity
ground/air/ledge state
invulnerability and respawn counters
RNG call index and gameplay RNG state
stocks, KOs and match clock

Discrete events should match exactly. Position, velocity and matrix values may use a defined numerical tolerance.

The corpus should include:

fast aerials and projectiles;
two-frame hitboxes;
multi-hit moves;
shields and shield break;
reflectors;
grabs and throws;
simultaneous three- and four-fighter contacts;
moving platforms and stage hazards;
items;
KO, respawn, Sudden Death and Results.
What I would send back to the agent

The binary choice is false. On current master, do not implement compensated 30 Hz yet: the banked four-distinct run is dominated by native-stage reject reason 6 and by 1,420 animation-cache rejects with no resident battle pack. Restore the native stage route and require zero post-GO resource reads first, then re-profile the true argmax roster in one unified per-frame census. After that, keep 60 Hz discrete combat semantics but replace the two full source updates with two slim combat substeps inside one 30 Hz host frame. Run final pose, camera, HUD, audio service, effects and particles at lower rates; packetize all fighter/stage/effect/weapon rendering; and share one generated transform set between collision, attachments and rendering. A true one-sample-per-present combat simulation is the last resort. The six fighter pairs are not the primary 50% lever; use conservative broadphase but consume candidates in exact source order.