> **CORRECTIONS, 2026-08-01.** Both reviews below are kept verbatim as dated
> reviews. Four of their statements have since been measured false, and a reader
> who acts on them will waste a build:
>
> 1. **Review 1's hierarchy table ranks "L7 complete fixed-point collision --
>    very high confidence, gate-closing".** That leaf conversion was wired,
>    measured and reverted: +534 cycles/frame won in `SRC`, 6,481 lost in
>    `FTR`+`STG`. Review 2 already says so; Review 1 was never updated.
> 2. **Both baselines (1,208,960 / 88,960 gap) are stale twice over** -- taken
>    particles-off AND with `ifCommonSetMaxNumGObj` holding the GObj pool capped
>    for the whole match. The live figure is **WORK-H P95 1,257,280, gap
>    137,280** (`artifacts/performance/r207-baseline-2026-08-01-nocap-128.json`).
> 3. **Review 2's "Do not reopen" list contains a false entry:** *"Particle VRAM
>    as the blocker; actual natural use was two textures/1,280 bytes."* That was
>    read off a SINGLE-CPU mask. The both-CPU mask is `0x08400007` -- five
>    textures -- and atlas admission had genuinely evicted a live one, costing
>    **127,989 misses in one match**. Coverage is now measured clean (zero misses
>    over 560,419 quads), but the entry as written says do not look at the thing
>    that was broken.
> 4. **Review 2's Phase 0 and Phase 1 are DONE** (2026-08-01): the particle draw
>    seam, atlas, Dream Land's bank, and the full-content baseline. Start at the
>    Phase 1 attribution lane, whose first result is below.
>
> **Phase 1 attribution lane, first run** (`r207-attrib-2026-08-01-128.csv`, 127
> frames, per-frame stops, 19 over-gate / 108 clean):
>
> | per-frame counter | clean mean | over mean | % frames nonzero (clean/over) | r vs WORK-H |
> |---|---:|---:|---|---:|
> | particle quads emitted | 0.01 | 0.21 | 0.9% / 15.8% | +0.327 |
> | particle structs live | 0.01 | 0.21 | 0.9% / 21.1% | +0.366 |
> | weapon DObj submits | 0.18 | 0.32 | 17.6% / 31.6% | +0.234 |
> | asset loads | 0.13 | 0.11 | 11.1% / 10.5% | -0.039 |
>
> - **Asset loads are refuted as the tail owner for the third time**, now by
>   occurrence rate rather than by a run total: they are *slightly less* common on
>   over-gate frames, and P95 across no-load frames alone is 1,309,248.
> - **Particles and weapon draws are genuine MARKERS** -- the owner's intuition
>   that fireballs and VFX coincide with bad frames is directionally right, and
>   they carry the only positive correlations in the set. But **0.21 quads per
>   frame cannot be 229,280 ticks of `SRC`**. They mark the frame; they are not
>   its cost. This is the doc's own "correlation is not attribution", confirmed.
> - **The L6-versus-census contradiction is NOT settled by this run.** L6 called
>   the over-gate frame a hit-detection frame with 66.2% soft float; the
>   2026-08-01 symbol census put collision at 2.9%. The only collision counters
>   that exist are `gNdsCollisionRuntimeDiagnostics.default_fighter_calls` and
>   `.default_weapon_calls`, which are the **map**-collision diagnostics, and both
>   read zero on over-gate frames. The hit-detection path has no counter at all.
>   Adding one is the next step, and every Phase 4 recommendation depends on it.
> - **Window caveat:** frames 439-567 are a quiet stretch -- a whole match emits
>   110,976 quads, this window a handful. A second lane over a combat-dense window
>   is needed before pricing particle work.

## Review 1
Several meaningful levers remain, including one already measured large enough to close the current gate.

The latest properly attested control on the branch is WORK-H P95 1,208,960, or 88,960 ticks above the 1.12M target. The previously documented 1,147,200 figure had no supporting artifact.

The current shape is:

Bucket	Typical cost	P95 behavior	Meaning
Fighter	~381–392K	Almost flat	Large steady cost
Source update	~269–279K	~521–522K	Main P95 spike owner
Stage	~172K	~178K	Essentially finished
Background	~4K	Flat	Finished
Audio	~2–4K P95	Rare large max	Not the current gate

The 128-frame artifact makes the distinction clear: FTR was 380,800 P50 / 383,872 P95, while SRC was 278,784 P50 / 522,368 P95. The fighters are expensive, but they are not what creates most of the tail.

Your observation that fireballs, lasers and combat VFX coincide with bad frames is probably correct. But the evidence says they are usually the visible marker of a collision-heavy frame, not proof that drawing the effect is the main expense. The over-gate split found:

gmCollisionSetInvertMatrix: 34 calls on every expensive frame, zero on clean frames
Soft-float: 66.2% of the expensive-frame premium
Renderer matrix building: only 1.06×
Relocation work: 0.5%
Rendering and animation: effectively flat in that comparison

So there are two different goals:

Close the P95 gate: attack SRC collision.
Finish the DS-native architecture: remove the remaining generic weapon, effect, particle and renderer paths.
1. The largest remaining lever: L7 fixed-point collision

This remains the strongest measured lever by a large margin.

The proposed L7 conversion replaces the soft-float body of gm/gmcollision.c with DS-friendly fixed-point transforms. Its point-transform kernel is now inside the accepted error bound:

Measured worst error: 0.016609
Allowed bound:        0.020000

The estimated recovery is approximately:

238,000 ARM9 cycles
≈119,000 project ticks on affected frames

That is larger than the currently attested 88,960-tick P95 gap.

Why fireballs and lasers implicate this path

A projectile frame can activate several expensive systems at once:

Weapon-vs-fighter hit tests
Fighter hurtbox transforms
Shield and reflector tests
Hit effect attachment
Fireball floor/wall collision
Damage and knockback processing
New action/status transitions

The weapon subsystem still executes the original wpmanager, wpmain, wpmap, wpprocess and wpdisplay code, including floating-point vector and collision helpers.

L7 should therefore be completed before judging how expensive the projectiles themselves are. It attacks the common collision cost shared by melee attacks, fireballs, lasers, shields and reflectors.

L7 should be wider than one matrix function

A narrow replacement of gmCollisionSetInvertMatrix is unlikely to be the best final shape. The DS-native implementation should:

Maintain compact 20.12 joint world transforms once per gameplay tick.
Transform hitbox points directly rather than manufacture a general inverse matrix.
Let fighter hitboxes, weapon hit tests and attached visual effects consume the same generated transform representation.
Replace the full relevant gmcollision.c cluster rather than crossing between float and fixed representations repeatedly.
Keep a bounded oracle mode that runs old and new calculations together for qualification.

That is both a performance optimization and a reduction in duplicated transform work.

2. Fireballs and Fox lasers are not native render owners

The renderer currently recognizes only these production owners:

Dream Land stage
Mario
Fox

The architecture document states directly that effects and weapons outside an owner remain in original display order.

This means the core fighters and stage are native, but the transient combat objects—the things most likely to exist on burst frames—remain outside the specialized architecture.

Mario fireball

Mario’s fireball uses the imported BattleShip weapon subsystem. Its gameplay is source-based, which is correct, but its presentation still travels through the weapon GObj/DObj and generic weapon display machinery rather than a generated DS projectile owner.

The direct-native version should be:

Source weapon simulation
        ↓
compact DS projectile render record
        ↓
preloaded texture/mesh
        ↓
direct GX submission

No generic DObj traversal, no display-list interpretation, no runtime texture discovery, and no new GObj merely for presentation.

A fireball record needs little more than:

position
rotation or velocity direction
scale
animation frame
palette/material state
active flag

Its geometry and texture are immutable and should be compiled offline.

Fox blaster

The Fox wrapper explicitly substitutes the original glow with a bounded untextured DS shape:

ndsEFManagerMakeVisualEffect(nNDSVisualEffectHitElectric, ...)

The laser bolt and its glow should instead use:

A direct GX quad or narrow strip for the world-space laser.
A packed atlas frame for the impact glow.
A fixed event queue rather than an effect GObj.
One batched draw for all laser/glow instances sharing the atlas.

This should improve both architecture and burst-frame predictability.

3. The current replacement VFX system is still generic DObj rendering

The existing substitute effect system creates:

An EFStruct
A GObj
A DObj
An XObj
A process callback
A display callback
Float position, rotation and scale updates

Then it calls gcDrawDObjTreeForGObj. Attached effects also call gmCollisionGetFighterPartsWorldPosition every update.

That is exactly the kind of generic runtime scaffolding Runtime 2 is intended to remove.

The effect header describes the current presentations as bounded, untextured DS presentations, covering dust, hits, sparkle, shield, reflector, death and rebirth.

Replace it with a native fixed-pool effect runtime

A DS-native cosmetic instance could be about 24–32 bytes:

typedef struct {
    s32 x_q12;
    s32 y_q12;
    s32 z_q12;
    s16 vx_q8;
    s16 vy_q8;
    u16 scale_q8;
    u8 age;
    u8 lifetime;
    u8 kind;
    u8 frame;
    u8 flags;
    u8 joint_index;
} NDSFxInstance;

Use fixed pools such as:

16 hit effects
8 dust effects
4 attached effects
4 death/rebirth effects

The source effect calls become event producers. The DS backend owns compact update and draw.

That removes:

GObj allocation and traversal
DObj/XObj allocation
Generic process dispatch
Generic display callbacks
Float scale/rotation updates
Generic display-list decoding
Per-effect matrix construction
Repeated world-position walks for attached effects

This is one of the clearest remaining architectural wins.

4. The real particle runtime still draws nothing

The original BattleShip particle interpreter has now been imported and can run behind NDS_R2_PARTICLE_RUNTIME=1. However, its DS draw seam is still unfinished.

The current lbParticleDrawTextures path:

Walks source particle lists.
Applies camera masks.
Resolves script texture/frame state.
Counts draw engagement.
Does not submit textured quads to DS hardware.

The code comments explicitly separate the imported interpreter from the still-missing DS textured-quad renderer.

Also, only the common effect bank is meaningfully registered; non-common banks such as Dream Land’s stage particle bank are currently empty.

Required native particle path

The correct design is not to translate each particle into a tiny N64 display list. It should be:

BattleShip particle scripts
    → source-faithful particle state
    → compact DS quad instances
    → atlas-sorted batches
    → direct GX FIFO/DMA

Each visible particle becomes one record containing:

world position
screen-facing size
atlas frame
color
alpha
rotation
depth/polygon class

Then:

Convert required textures offline to A3I5/A5I3 or another measured DS format.
Pack them into as few atlases as possible.
Group instances by atlas and blend mode.
Bind once per group.
Emit direct billboard quads.
Never allocate, decode, upload or convert during gameplay.

The Runtime 2 plan notes that the clean-frame budget affords only around fourteen texture binds at the previously measured ~1,621 ticks each. Therefore batching is not optional.

Particle work still required

The remaining particle-native coverage includes:

Mario fireball bounce and hit scripts
Fire-element hit effects
Fox laser impact glow
Running dust
Hard-landing dust
KO/death effects
Rebirth effects
Results confetti
Dream Land leaves/dust
Whispy-related effects

These are precisely the transient workloads likely to concentrate on combat-heavy frames.

5. Fox’s Reflector is still the wrong presentation

The live Fox Reflector gameplay source is imported, but the presentation is currently created as the generic nNDSVisualEffectReflector substitute.

The real Reflector is not merely a generic particle. It has an animated effect model associated with Fox’s special assets and status progression.

The native solution should be a dedicated direct effect owner:

Fox status state
    → reflector animation index/frame
    → precompiled reflector mesh/material packet
    → direct GX draw

It should not create a generic disc GObj and should not use the common particle path unless the source asset is actually particle-based.

6. The fighter renderer is native, but the fully precompiled fighter path is not

The Makefile still states:

NDS_BATTLE_PROFILE=0
DS-native precompiled path
NOT IMPLEMENTED

The shipping configuration remains profile 1, described as the current translation path/correctness oracle.

Mario and Fox do reach direct GX hardware through generated owners, hardware matrices and hardware lighting. But every frame still has portions of generic preparation around them:

Build runtime NDSRendererConfig records.
Build root records.
Validate file and display-list ranges.
Reconstruct material/preamble state.
Traverse selected DObjs.
Revalidate texture/cache identities.
Apply generated state spans.
Submit individual vertex commands from the CPU.

The current adapter code still constructs and clears production configurations and roots at runtime before invoking the native owner.

The final native fighter shape

A true profile-0 fighter renderer should look closer to:

load time:
    resolve textures
    generate final packet layout
    bind immutable run/epoch records
    prepare packet variants

per presented frame:
    update joint matrices
    patch dynamic matrix words
    patch a few material/flash words
    DMA/call the GX packet

The geometry, normals, UVs, primitive structure, culling, material transitions and texture identities are all known before the match.

A fighter packet can contain:

Static geometry commands
Static texture coordinates
Static normals
Static polygon format
Static texture binds or stable slots
Fixed locations for dynamic joint matrices
Fixed locations for palette/material variants
Hurt-flash packet variants

This would remove much of the remaining steady ~380–392K FTR cost instead of trying to optimize each generic check individually.

7. Task 56 fighter stripification deserves a fresh test—but not blind graduation

There is already a compiled DS-native primitive-stream implementation behind:

NDS_TASK56_FIGHTER_PRIMITIVES=2

It turns the current 626 fighter triangles from 1,878 individual GL_TRIANGLES vertex submissions into approximately 996 strip vertices—a 47% reduction, or around 882 fewer VERTEX16 submissions. The code remains default-off.

It was originally killed because ALL stayed flat. Later research proved that ALL was VBlank-quantized wall time: real work could disappear and simply reappear as additional wait time. That later review explicitly identified Task 56 as a lever rejected using a metric it could not move alone.

However, that same review also notes the old candidate’s FTR rose about 1%, despite reducing GX vertices. That suggests its original runtime implementation paid too much per primitive group or moved work elsewhere.

So Task 56 should be reopened under the current architecture, but with this gate:

A: current triangle path
B: current strip path
same source, same build placement, same ROM configuration

Measure:
FTR work ticks
WORK-H
GX FIFO words
VERTEX16 count
GX stall time
CPU submit time
primitive-group begin/end overhead

It is not automatically a win. The offline topology is excellent; the runtime submission implementation may need to be folded into the proposed prebuilt packet/DMA path before the vertex reduction becomes useful.

8. Native packet/DMA submission is the strongest remaining fighter-render direction

Task 36 already demonstrates the basic DS-native pattern for static stage work:

prepare once
retain command stream
submit through GXFIFO DMA

Fighters cannot replay identical matrices, but their command structure is static. Only a bounded set of payload words changes.

A fighter packet compiler could:

Build the final GX packet at load time.
Reserve patch offsets for each joint matrix.
Reserve offsets for dynamic color/material state.
Patch those small regions each presented frame.
Flush only the patched ranges.
DMA the complete packet into the FIFO.
Combine this with Task 56 strips.

This eliminates hundreds or thousands of per-vertex ARM9 stores and branches while still using the DS geometry engine for transforms and lighting.

It is the natural completion of NDS_BATTLE_PROFILE=0.

9. Stage rendering is close enough that it is no longer the priority

The current stage is around 172K P50 / 178K P95, compared with fighter ~381K and SRC ~522K P95.

Still-default-off stage levers include:

NDS_TASK55_STAGE_GEOM: removes 20.6% of redundant replay words.
NDS_TASK51_STAGE_NATIVE: full 42-binding native matrix path.
NDS_DREAMLAND_DS_MESH: generated DS-native simplified mesh.

They matter to the “everything native” architecture, but none should outrank:

L7 collision
Projectile/effect owners
Particle quad renderer
Fighter packet path

The stage already meets its intended budget.

10. The current 2D hardware path is mostly complete

The important battle 2D systems are already DS-native:

Dream Land wallpaper through hardware affine BG
Countdown and GO through prepared OAM assets
Traffic-light components through hardware atlases
HUD through change-driven DS text/OAM/BG machinery
Results wallpaper through hardware affine BG

The native IFCommon interface also exposes explicit prepare, begin-frame, draw and commit phases, with counters for fallbacks and hot conversion.

Remaining 2D work is mainly transient presentation:

Time Up visual completion
Results confetti
Stock snap/steal effects
Battle score effects

Several of those are still explicit skipped weak stubs.

11. Audio is hardware-backed, but BGM streaming still touches the ARM9 critical path

FGM playback is already mapped to the DS audio backend and ARM7 playback. The architecture still records synchronous ARM9 BGM file reads, ring-buffer refill and cache flushes during update processing.

Audio is not the current P95 owner: the measured AUD P95 was only about 3.5K, although occasional maximums reached around 127K.

A future cleanup could:

Increase predictable prebuffering.
Schedule refills away from known combat-heavy frames.
Use a fixed refill cadence.
Avoid DLDI reads during active gameplay where RAM permits.

But this is below the collision, fighter and VFX work.

What is actually not yet direct-native
Core battle presentation audit
Subsystem	Current status	Remaining gap
Dream Land wallpaper	Native BG affine	Essentially complete
Dream Land stage	Native mode-9 owner, replay/DMA	Full profile-0 path and some default-off native variants
Mario body	Native GX owner	Generic preparation and CPU command submission remain
Fox body	Native GX owner	Same
Fighter matrices	DS hardware	Complete
Fighter lighting	DS hardware	Complete
Hurt flash	Native owner active, visual state incomplete	Bake correct flash colors/material variant
Mario fireball model	Generic weapon/DObj presentation	Dedicated native projectile owner
Fox laser model	Generic weapon presentation	Dedicated native projectile owner
Fox laser glow	Generic untextured effect substitute	Native atlas quad
Fireball hit/bounce VFX	Missing/substitute particle scripts	Native particle quad path
Hit/dust effects	Generic DObj templates or Task39 approximations	Fixed-pool native effect renderer
Particle scripts	Interpreter present	No DS textured-quad draw
Dream Land particle bank	Effectively absent/empty	Pack and register native bank
Fox Reflector	Generic substitute disc	Native reflector DObj packet
Shield	Approximate presentation	Exact native source-derived version if required
Results confetti	Missing	Native particle/OAM path
Stock/battle score effects	Skipped stubs	Native OAM/particle path
Crowd reaction	Trigger actor incomplete	Source event → DS audio trigger
Full profile-0 battle	Build explicitly rejects it	Complete precompiled DS path

The core stage/fighter/background pipeline is close. The unfinished portion is concentrated in transient combat objects, exactly where burst-frame problems usually live.

Recommended order
1. Attribute the projectile/VFX frames correctly

Add a same-binary per-frame ring containing:

WORK-H
FTR
SRC
active visual-effect count
visual-effect creates/destroys/draws
particle visible count
fireball active/spawn/map/hit/reflect counts
laser active/spawn/hit/glow counts
weapon collision calls
gmCollisionSetInvertMatrix calls
fighter action changes
asset loads

Produce event-conditioned distributions:

no projectile
fireball active
fireball collision/hit
laser active
laser hit/glow
melee hit
shield/reflector hit
particle burst

This will answer whether “fireball frame” means:

Expensive fireball rendering
Weapon map collision
Fighter hit detection
Status transition
Effect spawning
Several of them together

Do not optimize the visible effect based only on temporal correlation.

2. Finish L7

This is the only currently measured lever likely to close the P95 gate alone.

3. Create a native projectile/effect instance system

Start with:

Mario fireball
Fox laser
laser glow
fireball hit/bounce
normal/fire/electric hit spark
dust

Remove their generic GObj/DObj display path.

4. Finish the particle DS quad backend

Use one or very few packed atlases, fixed pools and direct batched GX submission.

5. Implement fighter packet/DMA profile 0

Patch matrices and dynamic colors into prebuilt GX packets. Combine it with a new Task 56 test.

6. Complete the remaining presentation gaps

Reflector, Whispy particles, Results confetti, Time Up, stock effects and crowd trigger.

Performance expectation

The realistic hierarchy is:

Lever	Confidence	Likely importance
L7 complete fixed-point collision	Very high	Gate-closing
Native fighter packet/profile 0	High architectural value	Potentially large steady FTR reduction
Native projectile/effect fixed pools	High	Burst-frame and architecture improvement
Particle atlas + batched quad renderer	Required	Prevents real VFX from destroying P95
Task 56 retest integrated with packets	Medium	Large GX reduction; CPU benefit unproven
15 Hz round-robin particle simulation	Medium-high	P95 flattening after particles work
Remaining stage-native flags	Low priority	Stage already in budget
More animation micro-optimization	Low	Prior investigations exhausted it
More relocation/load micro-cuts	Low	Only ~18% of expensive frames

The big conclusion is that direct-native rendering is not finished at the exact places where combat creates temporary objects. Fighters and Dream Land are native. Fireballs, lasers, generic effects and particles are not. At the same time, the current P95 tail is primarily collision computation, so replacing VFX drawing alone will not close the performance gate. L7 plus native transient-object presentation is the path that addresses both goals.


Compact visual fighter
  generated DS render skeleton
  precomputed quantized pose matrices
  precompiled GX packet
  matrix/material patch table
  DMA submission
  no DObj traversal
  no animation interpreter
  no runtime topology/material discovery
  
  At the latest measured two-fighter baseline, FTR is roughly 380,800 ticks P50 / 383,872 P95. Linear scaling would put four fighters near 762K fighter-rendering ticks alone, before Dream Land, gameplay, HUD, audio, particles, or VFX. That cannot fit comfortably inside the 1.12M frame budget.

One correction to my previous report: the branch’s latest handoff says the L7 collision fixed-point implementation was wired, measured, and reverted. Soft-float was much cheaper than expected, while the added code footprint slowed unrelated fighter and stage work. So blanket float-to-fixed conversion is not the answer.

The scalable answer is:

Precompute fighter poses, prebuild GX packets, patch only the changing words, and dynamically reduce visual detail as fighter count rises.

1. The biggest fighter lever: precompiled pose packets

The current fighter path is native GX rendering, but it still constructs substantial generic state around every fighter:

DObj traversal
Local/world matrix construction
Root and configuration records
Material/state validation
Texture/run preparation
Per-run submission
Individual CPU writes for normals, UVs and vertices

The adapter still maintains large generic workspaces containing matrix bindings, material DObjs, composed matrices, production configs, production roots and hierarchy arrays.

The finished fighter path should instead resemble this:

BUILD TIME
  SSB64 fighter model + animations
        ↓
  DS render skeleton
        ↓
  quantized pose matrices
        ↓
  precompiled GX packet template
        ↓
  matrix/color/texture patch locations

MATCH LOAD
  load chosen fighters and LODs
  expand pose tables into contiguous RAM
  build final packet buffers

EACH PRESENTED FRAME
  choose animation pose index
  patch root transform
  patch bone matrices
  patch flash/face/material words
  DMA packet to GXFIFO

The runtime should not discover or reconstruct anything about the fighter’s geometry.

Packet contents that remain static

These can be compiled permanently:

Geometry topology
Vertex positions relative to each rigid body part
Normals
UV coordinates
Texture bindings
Polygon format
Culling
Lighting setup
Primitive groups
Bone-to-part assignments
GX command order

Only these should normally change:

Root position and facing
Bone matrices
Animation frame
Damage flash color
Face or texture-animation frame
Visibility mask

That can turn hundreds of runtime decisions and thousands of individual stores into a few hundred contiguous matrix/color word patches followed by DMA.

2. Stop constructing world matrices on the ARM9

The renderer already outputs DS 20.12 fixed-point matrices, and much of the current work is matrix copying and local/world composition rather than raw floating-point math. The current code builds local matrices, walks parent chains, composes world matrices and stores them in caches.

A better DS path is:

load fighter root matrix once

for generated skeleton node:
    MTX_PUSH
    MTX_MULT_4x3 precomputed_local_pose_matrix
    draw rigid part
    recurse
    MTX_POP

The DS geometry engine then performs the hierarchy composition.

The ARM9 only selects or patches the local pose matrices. It no longer needs to:

Walk upward through DObj parents
Rebuild local matrices from float translation/rotation/scale
Multiply local matrices into world matrices
Copy 64-byte matrices through generic caches
Associate matrices with renderer roots each frame

This is particularly attractive because Mario and Fox have relatively shallow rigid-part skeletons that fit naturally into a generated push/multiply/pop program.

3. Replace runtime animation evaluation with quantized pose tables

The cubic animation branch is already Q12 fixed point, so converting that same interpolation routine again will not produce another major win.

The larger move is to delete runtime interpolation from the visual path.

For every fighter animation, compile poses at the presentation rate:

animation 27:
  pose 0
  pose 1
  pose 2
  ...

Each pose could store final local 4×3 bone matrices directly.

For 16 render bones:

16 bones × 12 matrix elements × 2 bytes
= 384 bytes per pose

Even several hundred visual poses per fighter are practical in ROM, and the project explicitly allows large precomputed tables, quantized animation poses, precomputed matrices and heavy loading-time preparation.

Runtime animation then becomes approximately:

pose_index = animation_pose_table[action][visual_frame];
matrix_block = fighter_pose_data + pose_index;

No AObj linked-list walk. No cubic/linear/step selection. No per-track float conversion. No matrix construction from individual translation, rotation and scale values.

Transition handling

For action changes:

Precompute common transition blends.
Quantize uncommon transitions to the nearest pose.
Permit several generated blend frames where the visual difference is noticeable.
Never fall back to the full interpreter during gameplay, because that would recreate P95 spikes.

Gameplay animation state can remain completely source-faithful. Only the visual representation changes.


## Review 2
there are still meaningful levers, but the evidence changes the diagnosis:
Fighter rendering is already direct-native. It is a large, stable floor, not the spike source.
SRC owns the measured tail. The last event-matched profile attributes that tail primarily to collision work triggered during combat, not to fighter drawing.
The remaining actual rendering interpreters are particles, weapon/effect DObjs, and production fallbacks still linked into the ROM.
The fighter animation/event parser is also still interpreted on ARM9 inside SRC, although it is not rendering.
The newest performance baseline does not include the particle runtime, so the final P95 target must be re-established after the missing content is live.
I made no edits, builds, or snapshots. Current branch is codex/r2-runtime2, HEAD 875de9a; while I researched, another agent began an eight-file direct-particle WIP that must be preserved.
What the current numbers actually say
Newest attested DLDI-on evidence is still commit 800a934, not current HEAD:
Bucket	P50	P95	Max	Interpretation
WORK-H	921,664	1,208,960	1,507,968	88,960 over the 1.12M gate
FTR	382,976	386,176	387,200	Large but nearly perfectly flat
STG	171,520	179,392	181,760	Flat and within its budget
SRC	281,856	523,008	681,856	The real tail
MISC	44,672	155,456	192,768	Possible event/dynamic-render contribution, but secondary


Source: [r207-L7-control-128.json (line 105)](D:/Stuff/DevFolder/Smash64DS_Port/artifacts/performance/r207-L7-control-128.json:105). VBlank histogram was 2:461, 3:91, 4:10, 5+:4, max 19, zero cadence violations.
The stronger event-matched evidence is:
Over-gate versus clean: FTR −1,344, STG +896, SRC +235,456, WORK-H +272,576. The rendering owners are flat or cheaper on expensive frames. [Board evidence (line 2934)](D:/Stuff/DevFolder/Smash64DS_Port/docs/P1_EXECUTION_BOARD.md:2934)
Per-PC profiling found a collision workload switching on at 34 inverse calls and 40 compose calls per expensive frame; 66.2% of its premium was soft-float. Rendering and animation remained flat in that comparison. [L6 profile (line 2864)](D:/Stuff/DevFolder/Smash64DS_Port/docs/P1_EXECUTION_BOARD.md:2864)
The attempted fixed inverse won only 534 cycles/frame in SRC but added enough ARM text to regress FTR+STG by 6,481 cycles/frame. That exact leaf conversion is finished and rejected. [L7 result (line 2302)](D:/Stuff/DevFolder/Smash64DS_Port/docs/P1_EXECUTION_BOARD.md:2302)
So the owner observation that attacks, fireballs, and lasers coincide with spikes is useful—but correlation is not attribution. Those events also activate hit detection, collision matrices, particles, audio, and dynamic draws. The coding agent should separate them within one run before assigning the spike to VFX.
What is already direct-native
Do not redo these:
Fighters use the generated production owner in canonical mode 9. On success, the generic display-list loop is skipped. [Native selection (line 12484)](D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_renderer_dl.c:12484) and [native execution (line 12864)](D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_renderer_dl.c:12864).
The latest fallback census was live: calls:1132, eligible:1132, every rejection reason zero. [Fallback proof (line 2990)](D:/Stuff/DevFolder/Smash64DS_Port/docs/P1_EXECUTION_BOARD.md:2990)
Dream Land’s static stage is direct generated/replay rendering. STG is not the tail owner.
Results now selects the same native fighter owner as battle and meets its gate.
HUD/background and Task 39 hit-spark presentation already use DS 2D/OAM paths.
The remaining fighter cost is legitimate dynamic preparation and GX emission: pose-derived matrices, material snapshots, native-run preparation, and vertices. “Native” cannot mean zero ARM9 work.
What is not yet native
1. Particle rendering and simulation
The checked-in implementation compiles BattleShip’s original particle bytecode interpreter unchanged. Its draw seam merely walks visible particles and records what would have drawn. [Current ownership description (line 1)](D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_lbparticle.c:1).
An active dirty WIP is now adding:
One 256×128 RGB555+A1 atlas.
A direct camera-facing GX quad emitter.
Battle-entry atlas upload/reset ownership.
Texture/frame lookup and emit/miss counters.
That is the right direction and should be finished, not restarted. However, it is not yet a fidelity-complete particle renderer:
The emitter API currently carries position, size, camera axes, and UVs only.
It does not yet carry source prim/environment color, per-particle alpha, dither/noise, or other draw flags.
It admits 20 of 31 textures/55 frames; excluded P1 textures would silently draw nothing.
RGB555+A1 may be too harsh for smoke/glow/fire gradients; only owner visual approval can settle it.
NDS_R2_PARTICLE_RUNTIME remains off in the measured tick-HUD build. Therefore the 1,208,960 P95 excludes this entire workload.
After pixels work, the original lbParticleUpdateStruct and lbParticleGeneratorFuncRun bytecode interpreters still remain. That is the largest unambiguous runtime interpreter left.
2. Weapons and generic effect DObjs
Mario’s fireball and Fox’s laser weapon DObjs still enter:
SubmitWeapon/EffectDObj → SubmitStageDObj → material/matrix preparation → ndsRendererExecuteDisplayListWithVertexCache → ndsRendererScanList.
See [weapon/effect submission (line 12829)](D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_movement.c:12829), [generic adapter (line 8533)](D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_renderer_dl.c:8533), and [scanner (line 26455)](D:/Stuff/DevFolder/Smash64DS_Port/src/nds/nds_renderer.c:26455).
Additionally, ndsEFManagerMakeVisualEffect builds N64-style display lists from a handful of 16-vertex star/dust/ring/disc templates and attaches gcDrawDObjTreeForGObj. [Effect constructor (line 727)](D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:727).
Most of those templates should not be optimized. They are stand-ins that should be deleted when the real particle scripts become visible.
Remaining special cases:
Fireball and laser projectile meshes: AOT-compile to fixed DS-native mesh/material descriptors.
Fireball bounce/hit/damage and Fox blaster glow: route through the restored particle system.
Shield: retain only if required, but emit its bounded primitive directly through GX rather than scanning a synthetic Gfx list.
Fox Reflector: not a particle. Restore its original animated special2 DObj with all four statuses through a native model path. [Current wrong seam (line 901)](D:/Stuff/DevFolder/Smash64DS_Port/src/import/battleship_efmanager.c:901).
3. Fighter animation source interpreter
Fighter graphics are native, but SRC still runs:
ftParamUpdateAnimKeys → battleship_ftAnimParseDObjFigatree → gcPlayDObjAnimJoint plus material animation.
[Live update loop (line 1553)](D:/Stuff/DevFolder/Smash64DS_Port/src/port/reloc_backend_compat_shims.c:1553).
Prior exact call counting priced animation evaluation at roughly 52K ticks per presented frame, paid across two 60 Hz logical updates. Current profiles still rank the parser/player highly.
Already exhausted:
Full pose tables: approximately 2.62 MB resident or transition-time streaming hitches.
Moving only render-only joints to 30 Hz: 84% of the skeleton is collision-load-bearing; ceiling only ~11.7K.
AObj reordering/flattening: ~2.9–5.8K ceiling.
Reintroducing an AObj pool or blindly moving it to DTCM.
Reworking the fixed cubic again.
The remaining plausible form is an offline-normalized animation-track representation: compile FIGATREE event words into compact per-motion/per-joint keyframe rows, then advance typed cursors directly without interpreting N64 command words and flag masks.
4. Generic fallback code still linked
Although fighter fallback executes zero times, its large generic loop remains compiled inside the 8,936-byte fighter owner function. Diagnostic preview interpreters are also linked.
Once all P1 dynamic objects have direct paths, a production-native-only configuration can compile out:
Fighter generic fallback loop.
Dynamic weapon/effect Gfx scanning.
Software preview/diagnostic readers.
Eventually ndsRendererScanList and ndsRendererExecuteDisplayList* entirely from published mode 163.
This may become a real performance lever through reduced text/placement pressure, but it must be measured. Do not multiply every removed byte by L7’s 1.85-cycle figure; that constant was measured for added hot ARM text, not arbitrary cold code.
Recommended coding-agent plan
Phase 0 — preserve and finish the active particle WIP
Do not reset or duplicate the current eight dirty files.
Review the existing atlas generator, renderer API, lbParticleDrawTextures, and scene texture lifecycle as one change.
Run mapdiff before any ROM execution; this checkout is highly sensitive to text/BSS movement.
Complete source draw semantics:Camera/projection equivalence.
Texture frame and UV orientation.
Prim/environment color and alpha.
Blend, dither/noise and relevant particle flags.
Correct size/transform modes.
Source list order and translucent overlap.
Scene entry/rematch/Sudden Death reset.

Classify every excluded texture against the exact P1 script closure. Only add a second atlas, alternate DS texture format, or source-derived downscale for textures that P1 actually requires.
Require zero unexpected atlas misses, pool drops, upload failures, and stale texture names.
Phase 1 — establish the real full-content baseline
The current 1,208,960 baseline is particles-off. Do not optimize against it after enabling particles.
Use one verifier-covered, DLDI-on ROM with particle simulation and draw both enabled:
Same 128-frame settled window and identical configuration.
Full one-minute match, GAME SET, Results, rematch, and Sudden Death.
Record WORK-H/FTR/STG/SRC/MISC, mean/P50/P95/max, and the 2/3/4/5+ histogram/max.
Record ROM SHA, map/text/BSS sizes, particle update ticks, quad ticks, quads, atlas binds, misses, pool high-water and drops.
Add one temporary per-frame attribution lane around:
Collision calls.
Particle update/generator work.
Particle visible/emitted count.
Weapon/effect generic scans by kind.
Fireball and blaster lifetimes.
Task 39/OAM work.
Within that run, compare expensive versus clean frames. If VFX counters do not co-occur with the tail, stop calling VFX the spike owner.
Phase 2 — eliminate generic dynamic rendering
Generate direct native mesh/material descriptors for nWPKindFireball and Fox blaster.
Dispatch by weapon kind at ndsStageGCDrawAllLoopSubmitWeaponDObj; consume live position/scale/orientation, but bypass material-segment construction and ndsRendererScanList.
Route source particle effects to the direct particle renderer and delete their primitive stand-ins.
Add a tiny direct GX shield primitive only if it remains part of the accepted design.
Restore Fox Reflector through its source special2 model/status animation.
Keep fallback only while qualifying. After natural proof, remove the covered production route rather than retaining two renderers.
The separate “fireball sometimes does not spawn” bug needs trigger → constructor → pool/GObj → weapon update → draw evidence. A native renderer only fixes the last step.
Phase 3 — strip the generic graphics interpreter from production
Once battle, Results, rematch, and Sudden Death all prove direct coverage:
Compile the fighter generic loop and preview interpreters only in diagnostic/oracle targets.
Make native contract failure visible and fail closed before touching GX; do not silently return to the generic renderer.
Use nm, map inspection, and callsite checks to prove the published mode-163 ELF no longer contains reachable ndsRendererScanList/ndsRendererExecuteDisplayList*.
Run matched A/B. Keep if code is smaller and performance is non-worse; it may improve FTR through placement even though fallback previously executed zero times.
This is the cleanest remaining fighter-side experiment because it deletes code instead of adding another fast path.
Phase 4 — reprofile, then attack the measured P95 owner
If full-content profiling still identifies the same collision burst, the only collision experiment worth running is a whole replacement:
Replace both func_ovl2_800ED490 compose and gmCollisionSetInvertMatrix.
Do not wrap them; ensure the original float bodies disappear from the map.
Use the measured row-scaled near-orthogonal domain:
R^-1[c][r] = M[r][c] / s_r²
with three reciprocals, nine multiplies, and an orthogonality/domain guard.
Preserve source hit-test ordering and decisions.
Differentially test attacks, shields, grabs, ledges and boundary cases against the float oracle.
Reject before running if mapdiff shows an additive implementation comparable to L7.
Keep only if the net candidate clears at least the practical 16–20K placement floor and improves P95 without raising the clean body. The target is the full 88,960 gap, not a 534-cycle local win.
Phase 5 — retire source-format interpreters
Particle AOT:
Extend the generator’s existing opcode decoder and reachability analysis.
Begin with naturally hit scripts such as Fox glow and Results confetti, then cover the complete P1 closure.
Dispatch once at spawn to a generated native program/evaluator.
Preserve RNG order, child/generator spawn order, loops, attachment, lifetimes, colors, interpolation and pool behavior.
Use the original interpreter only as a lab oracle.
Require shipping-map absence of lbParticleUpdateStruct, lbParticleGeneratorFuncRun, and the original bytecode bank.
Fighter animation AOT:
Compile reachable Mario/Fox FIGATREE streams into compact normalized track/keyframe rows.
Keep the existing fixed cubic kernel; eliminate command-word decoding and repeated track discovery.
Preserve 60 Hz collision-bearing pose updates.
Compare every joint TRS/material field and collision decision after every logical update.
Avoid generated per-motion C if it bloats text; compact typed rows plus one small native walker are preferable.
Both experiments must include map size. An AOT conversion that enlarges hot text enough to offset its saved work is not a performance win.
Phase 6 — only then consider rate changes
The uncompensated 30 Hz simulation test saved 119,744 P95, nominally enough to clear today’s gap, but it runs gameplay at half speed and leaves the tail shape largely intact. It is only a ceiling.
Order:
Particle updates at the permitted 15 Hz with source-faithful integration/interpolation and staggered work.
Other cosmetic VFX rates if visually acceptable.
Fully compensated 30 Hz gameplay only as an explicit owner-approved last resort.
Do not reopen
Fighter fallback as the current spike cause.
Static stage optimization as the current gate answer.
gmCollisionSetInvertMatrix leaf conversion.
Pose tables, render-only joint splitting, AObj reordering/pool/DTCM by themselves.
Another cubic/Horner rewrite.
Generic compact renderer programs; those already exist.
Broad GX DMA, cache lockdown, LTO/PGO, ARM7 renderer offload, or 2D-composition rewrites.
Particle VRAM as the blocker; actual natural use was two textures/1,280 bytes.
Direct-native optimization of placeholder VFX that should be deleted.
Uncompensated 30 Hz as a candidate.
Any probe that adds substantial BSS/text before a map check.
Definition of done
The final “everything direct-native” claim should require:
Zero production fighter fallback.
Zero production weapon/effect Gfx scans.
No reachable generic display-list scanner in published mode 163.
Particles emitted directly through GX with complete P1 texture/effect coverage.
Particle and fighter animation source bytecode replaced or explicitly retained only because a measured native replacement is slower.
Full-content DLDI-on P95 ≤ 1.12M.
2/3/4/5+ histogram and max interval reported.
Boundary plus Latest where scene/startup texture ownership changes.
Full match, Results, rematch and Sudden Death without drops, corruption, hangs or stale state.
Synchronized visual evidence and owner approval for particle alpha/blending, fireball, laser, shield and all four Reflector statuses.
Final retail-DS cadence/visual acceptance after accuracy-melonDS qualification.
Historical memory was used only to recover earlier dead ends and acceptance cautions; all checkout state and performance numbers above were reverified against the live tree.

## Opened 2026-08-01 by the source-effect routing

Two items the effect work created rather than found. Both are bounded and both
have a named measurement; neither blocks the bug row that produced them.

**Cache the particle quad's axis magnitudes.** `ndsParticleTransformForDraw`
(src/import/battleship_lbparticle.c) calls `sqrtf` twice per particle per frame
to recover the transform's X/Y scale. The source computes the same two numbers
once per transform per frame and stores them in `xf->pc0_magnitude` /
`xf->pc1_magnitude` — fields this port's LBTransform already carries — inside
the `transform_id != dLBParticleCurrentTransformID` guard that is already there.
Moving the two `sqrtf` calls inside that guard is a few lines and reuses
existing storage. Worth roughly 200,000 `sqrtf` calls a match at the measured
2026-08-01 volume (`gNdsParticleDrawVisibleCount` 103,322 over 2,043 presented
frames), and the number to watch is `gNdsTickHudForegroundTicks`, not P95 alone.

**Re-derive the particle atlas against the seams that are now live.** With the
motion-script effects routed to source, `gNdsParticleQuadMissCount` is 1,381 of
103,322 visible particles — 1.3% that draw nothing rather than draw the wrong
cell. Every missing texture id is in `reach.packed_textures`, so this is not an
unpacked texture: the admitted set is keyed on (texture, frame) pairs and
`gNdsParticleQuadMissFrameMask` shows frames 0-14, 16 and 18 all missing, i.e.
whole animations the generator did not admit because those scripts were not
reachable when it last ran. Re-run `scripts/generate_nds_particle_banks.py` and
price the result: the atlas is presently 64x64 / 8,192 bytes, and the earlier
128x128 fits `sNdsRendererHardwareTextureScratch` exactly, so the question is
VRAM against `NDS_RENDERER_HW_TEXTURE_CACHE_COUNT` and the 24 entries the
battle's static set pins — not new RAM.

### The atlas admission set is now measurably stale

Quantified 2026-08-01 after the routing, and this supersedes the vaguer note
above. `gNdsParticleTextureUseMask` came back `996076583 / 8294`, i.e. the
match now draws **twenty-four** distinct particle textures where the mask used
to name about five. The 64x64 / 8,192-byte sheet admits **fourteen of
forty-seven**, so these fourteen draw nothing at all:

    1, 5, 11, 14, 15, 17, 25, 28, 29, 33, 34, 37, 38, 45

Texture **1 is in `QUAD_MEASURED_LIVE` and still lost** its place to the greedy
admission, which is the clearest sign the priority list no longer matches the
game. Thirteen of the rest are not candidates at all, because
`QUAD_MEASURED_LIVE` was measured before the source effects were routed and
the constant is defined as the measured set.

Re-deriving it is necessary but is not a fix: the budget is fixed, so admitting
fourteen more means evicting fourteen. The question underneath is whether the
sheet can exceed 8,192 bytes. `PORTING.md` records 16,384 and 32,768 both
failing, and the failure was specific rather than fundamental -- a larger
resident atlas made `ndsRendererHardwareResolveStageSourceFrameTexture` fail
about one frame in ten, which dropped `PrepareRun` and forced ~197 stage
rebuilds a match. That is VRAM cache contention against the 24 entries the
battle's static set pins in `NDS_RENDERER_HW_TEXTURE_CACHE_COUNT`, not a limit
of the packer. Options worth pricing, cheapest first: raise `QUAD_KO_CELL_MAX`
off 8 only for the burst's own frames; a second small sheet bound to a
different cache entry; evicting a static entry the battle does not draw.
