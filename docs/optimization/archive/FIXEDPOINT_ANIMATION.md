After reading the current code, I would rewrite fighter animation as a DS-native fixed-point animation runtime, not just rewrite the cubic equation again.
The important distinction is that your cubic evaluator is already fixed-point. The remaining problem is that it is sitting on top of a float-based AObj animation system.
Right now the path is basically:
BattleShip figatree
    ↓
float parser
battleship_ftAnimParseDObjFigatree
    ↓
36-byte linked AObj nodes full of f32 state
    ↓
gcPlayDObjAnimJoint
    ↓
float → fixed conversions
fixed cubic evaluation
    ↓
fixed → float
    ↓
DObj float rotation/translation/scale
I would change that to:
BattleShip figatree
    ↓
AOT / load-time DS animation compiler
    ↓
compact fixed-point animation tracks
    ↓
fixed animation clock + fixed segment evaluator
    ↓
fixed pose
    ↓
temporary float boundary for existing DObj consumers
             ↓ later
      direct fixed matrix/collision consumers
That is a much more interesting optimization than another arithmetic tweak.
Why the current fixed cubic isn't the end state
Your existing ndsR2CubicValueFixed is already quite good. It uses:
Q12 joint values
Q16 Hermite basis
ARM mode so 32×32→64 multiplies get SMULL
the numerically validated Hermite transformation
no giant conversion cache
And it already passed the existing error bound: about 0.0028 rad rotation / 0.0067 world units translation. KNOWN_ISSUES.md
But every cubic evaluation still does:
C
t = fixed(aobj->length * aobj->length_invert); // float multiply
length_q = fixed(aobj->length);

vb = fixed(aobj->value_base);
vt = fixed(aobj->value_target);
rb = fixed(aobj->rate_base);
rt = fixed(aobj->rate_target);

// fixed Hermite

return fixed_to_float(result);
So you're paying six float→fixed conversions, one float multiply, and one fixed→float conversion for every cubic track evaluation.
More importantly, the input comes from this:
AObj
C
struct AObj
{
    AObj *next;
    u8 track;
    u8 kind;

    f32 length_invert;
    f32 length;
    f32 value_base;
    f32 value_target;
    f32 rate_base;
    f32 rate_target;

    void *interpolate;
};
That's 36 bytes and the evaluator touches essentially the entire structure.
And your new profile makes this even more important: the hottest instruction in gcPlayDObjAnimJoint is just loading aobj->kind at 24.1 cycles/execution, accounting for roughly 23% of that function. P1_EXECUTION_BOARD.md
So simply making more arithmetic fixed-point doesn't address the largest structural problem.
What I would actually build
I'd specialize the fighter path first:
ftParamUpdateAnimKeys
  → battleship_ftAnimParseDObjFigatree
  → gcPlayDObjAnimJoint
Do not globally replace AObj initially. Material animation, camera animation, stage animation, effects, etc. all share that infrastructure.
Fighter joint animation is the hot, bounded, high-value target.
1. Compile figatrees to a DS-native animation format
BattleShip's ftAnimGetTargetValue is already screaming for AOT conversion.
Its animation data starts as s16, then does things such as:
rotation value   = s16 / 512
translation      = s16 / 4
scale            = s16 / 4096

rotation rate    = s16 / 512
translation rate = s16 / 32
scale rate       = s16 / 8192
These are almost entirely power-of-two scales.
There is no good reason for the DS runtime to turn these into IEEE floats and then immediately turn them back into fixed point.
Your host-side converter can instead produce something like conceptually:
C
enum NDSAnimKind
{
    NDS_ANIM_STEP,
    NDS_ANIM_LINEAR,
    NDS_ANIM_CUBIC
};

struct NDSAnimSegment
{
    uint16_t duration;
    uint8_t  track;
    uint8_t  kind;

    // Representation depends on track.
    int16_t value0;
    int16_t value1;
    int16_t rate0;
    int16_t rate1;
};
I wouldn't lock that exact layout in yet. The significant part is:
keep the original compact s16 information compact for as long as possible.
For many tracks, conversion to your evaluator's Q format can be a shift rather than a multiply.
For example conceptually:
C
rotation_s16 → Q12
translation_s16 → Q12
scale_s16 → Q12
can be emitted using preselected integer shifts/scales determined AOT.
You could alternatively AOT-generate Q12 coefficients, but I'd measure ROM footprint versus runtime shifts before deciding. The original s16 representation is already wonderfully compact.
2. Make animation time fixed-point too
This is where I'd diverge more substantially from the current implementation.
Today you've got:
C
f32 anim_frame;
f32 anim_speed;

f32 length;
f32 length_invert;
I'd use a fixed animation clock.
A reasonable starting candidate would be:
C
typedef int32_t AnimTimeQ16;
meaning Q16.16 frames.
Not because Q16.16 is magically correct—it needs a range census—but because it provides:
fractional playback speeds
huge frame range
cheap addition
direct fixed fractional extraction
Interestingly, the DS port of SM64 already uses an integer frame plus a 16.16-style accumulator assist:
C
s16 animFrame;
s32 animFrameAccelAssist;
So the general approach is very DS-appropriate even though BattleShip remains our behavior oracle.
Runtime becomes approximately:
C
anim_time_q16 += anim_speed_q16;
No float addition.
3. Don't store length_invert
This is one of the bigger architectural opportunities.
Current cubic evaluation calculates:
C
t = length * length_invert;
every evaluation.
Instead, when entering a segment, calculate its fixed phase increment once:
phase = starting phase
phase_step = anim_speed / duration
Then normal frames simply do:
C
phase += phase_step;
with phase represented as Q0.16/Q16.
So the common cubic path no longer needs:
C
length * length_invert
at all.
There are caveats: the original parser can explicitly alter length, playback speed can change, and commands such as the figatree adjustment command add payloads to the current length. Those events need to recompute/adjust phase accordingly.
But those are event transitions, not every-track-every-frame work.
That's exactly where you want the expensive calculation to live.
4. Fixed evaluation becomes tiny
Then your three evaluator types become conceptually:
Step
C
value = phase >= ONE ? target : base;
Pure integer compare.
Linear
C
value_q12 =
    base_q12 +
    ((s64)slope_q12 * phase_q16 >> 16);
Or even better, depending on how the AOT coefficients are represented:
C
value += delta_per_tick;
when playback-rate semantics permit it.
Cubic
Reuse the already-proven math from ndsR2CubicValueFixed.
But instead of:
C
fixed(float);
fixed(float);
fixed(float);
fixed(float);
fixed(float);
it receives:
C
phase_q16
value_base_q12
value_target_q12
rate_base_q12
rate_target_q12
directly.
Something like:
C
static int32_t
ndsAnimCubicQ12(
    uint32_t t_q16,
    int32_t length_q12,
    int32_t vb_q12,
    int32_t vt_q12,
    int32_t rb_q12,
    int32_t rt_q12)
{
    int32_t t2 = mul_q16(t_q16, t_q16);
    int32_t t3 = mul_q16(t2, t_q16);

    int32_t h00 =  2*t3 - 3*t2 + Q16_ONE;
    int32_t h01 = -2*t3 + 3*t2;

    ...
}
The important point is that there are zero IEEE conversions inside it.
I'd retain ARM mode for this kernel unless the final range proof lets you reformulate it entirely with safe 32-bit multiplies.
5. Get rid of per-track linked AObjs for fighters
This is the part I think matters at least as much as the fixed math.
Your current machine profile says you're memory-bound, and AObj is a particularly bad DS runtime structure:
pointer
tiny fields
padding
six floats
pointer
36 bytes, linked, read-modify-written every frame.
Rather than:
C
for (aobj = dobj->aobj; aobj; aobj = aobj->next)
I want something resembling:
C
const NDSAnimTrack *tracks = motion->tracks;
NDSAnimTrackState *state = fighter->anim_state;

for (i = 0; i < active_count; i++)
{
    ...
}
Dense sequential memory.
Even better, if the AOT representation makes the current segment derivable directly from motion/frame, the runtime mutable state becomes mostly:
motion
time
event cursor
rather than one mutable object per animated property.
That does three things at once:
removes float arithmetic,
removes pointer chasing,
dramatically reduces the animation working set.
That combination is what makes fixed-point animation interesting under your new profile.
Don't use a u8 global track index
An older repo analysis proposed this because there were 221 live AObjs in that particular capture.
Your current profiling is now around 360 live AObj nodes, so I would not bake a global 8-bit index into the new architecture.
Either:
C
uint16_t index;
or have separate per-fighter bounded pools where you've independently proved <256.
That older optimization assumption is stale.
What about writing the result to DObj?
I would deliberately keep one float boundary during the first implementation.
First-generation version:
fixed animation data
      ↓
fixed evaluator
      ↓
Q12 pose
      ↓
ONE conversion at output
      ↓
existing DObj Vec3f
That means you can replace the parser/evaluator without simultaneously rewriting:
fighter collision transforms
matrix construction
miscellaneous DObj consumers
debugging paths
effects expecting floats
So instead of the current:
float state
→ fixed
→ evaluate
→ float
you have:
fixed state
→ evaluate
→ float
Already a substantial reduction.
Then, if profiling justifies it:
fixed state
→ fixed pose
→ fixed local matrix
→ fixed world matrix
→ GX / collision
and the float representation disappears completely from fighter skeleton evaluation.
That second stage would be extremely attractive given your “DS native all the way through” direction.
I would not use the old sidecar-cache idea
The existing source already documents the experiment: the 256-entry conversion cache worked functionally and had an 86.4% hit rate, but the extra ~10 KB BSS and code footprint made the frame slower.
That is perfectly consistent with the profile you just generated.
So I would specifically avoid:
Float AObj
+
Fixed AObj cache
+
lookup/hash/validity
That gives the memory-bound ARM9 more state to touch.
The new representation needs to replace the old runtime state.
The really attractive end state
For Mario/Fox, I'd ideally have the build produce something like:
Mario
 ├── Idle
 │    ├── event stream
 │    ├── joint 4 segments
 │    ├── joint 5 segments
 │    └── ...
 ├── Run
 ├── Jump
 ├── AttackN
 └── ...

Fox
 └── ...
All already:
endian-correct
validated
pointer-free
fixed-point
compact
indexed
ready for ARM9
Runtime:
C
ndsFighterAnimSetMotion(fp, MOTION_ATTACKN);

...

ndsFighterAnimAdvance(fp);
ndsFighterAnimEvaluatePose(fp);
No BattleShip figatree interpretation during normal frame execution.
You still preserve the BattleShip command semantics—loops, waits, flags, callback points, interpolation—but compile them into a much cheaper representation ahead of time.
This is basically a tiny animation bytecode → DS animation IR compiler.
And unlike a dense per-frame pose table, which you've already found to be too large, it remains a sparse keyframe/segment representation.
Why this fits the new gate findings
This is the part that changes my view after the data you posted.
A small arithmetic rewrite isn't very exciting anymore.
A full fixed animation representation is, because it's simultaneously a body-wide compute reduction and a memory-layout reduction.
Your current board has roughly the two hot fighter-animation functions at about 5.3% of non-idle work, while your sensitivity curve now shows that even tens of thousands of repeatable body-wide ticks can move a surprising number of frames over the 30-FPS line.
And unlike merely replacing fadd:
this design also attacks the 24.1-cycle-per-node AObj cache miss.
That's what makes it worth investigating despite the overall machine being 65% stall-bound.
I would therefore define “Fixed-point animation eval” as:
Replace the fighter figatree parser + linked float AObj runtime with an AOT-compiled, compact, contiguous fixed-point fighter animation representation; retain the proven Q16/Q12 cubic math, move time/phase to fixed point, and cross to float only once at the legacy DObj boundary.

Not:
“Make ndsR2CubicValueFixed more fixed-point.”

That second problem is basically already solved.
And I would still do your free SPHD/SHDT/SCPU split before committing an implementation, because the new over-gate analysis says those three together are ~79.7K of discriminator and may change sequencing. But if animation becomes a campaign item, this is the version I would pursue.


Five things I'd change
RAM will kill this if it's an afterthought. Static headroom proven is 34,816 bytes, and +14KB of .bss once stopped the ROM booting. Linked-in AOT arrays are not viable. Ship the compiled tracks as files through the existing anim-cache arena (200,704 B, 32,768 keep-free) in the slot the figatrees already occupy — same bytes, better content, and it reuses the loader path cycle 105 just fixed.
Derive the phase, don't accumulate it. This is the one part of your design that isn't equivalent-by-construction. t = length * length_invert recomputes from scratch each frame so error is bounded per-frame; phase += phase_step drifts, and animation drives hitbox positions and therefore knockback. Keep an integer frame counter and compute phase = (frame * step) >> k. Same cost, drift class gone.
Ship the parser AOT as its own slice first. It's the biggest single item (24.1M), the only one AOT deletes outright rather than reduces, and it carries no numerical risk at all — pure build-time transform of data that never changes. The evaluator rewrite is the riskier half; don't couple them.
length_invert has readers outside the evaluator — written at reloc_backend_mp_collision.c:11918, read at battleship_sys_objanim.c:214/292/921/942, including two length_invert <= length runtime-float compares. Deleting the field is wider than deleting its use in the cubic.
Replace, don't coexist. 1.85 cycles of FTR mean per byte of added ARM text — a runtime selector between old and new evaluator pays its footprint twice and wins nothing.