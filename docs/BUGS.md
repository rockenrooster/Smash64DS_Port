**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

This board carries verdicts and the numbers that check them. The forensics for each
fix live next to the code that owns it -- the particle generator and its checker,
`battleship_lbparticle.c`, `battleship_efmanager.c`, `render-audio-fgm-phase-pack.py`
-- so a row here should not need to be an essay.

=============================================================================
ONE FINDING EXPLAINS FOUR OF THESE ROWS, AND IT IS NOT THE ATLAS (2026-08-03)
=============================================================================
The respawn platform, the shield, Fox's down B and the hard-landing impact wave
are NOT particle sprites in the source. Each is a full EFDesc drawn as an
ANIMATED DObj MODEL TREE, with its own display-list link, geometry, joint
animation and material:

  efmanager.c:1648  dEFManagerRebirthHaloEffectDesc
                    -> llEFCommonEffects3RebirthHaloDObjDesc + ...AnimJoint
                       made by efManagerRebirthHaloMakeEffect(gobj, scale):5994
  efmanager.c:460   dEFManagerShieldEffectDesc
                    -> llFTManagerCommonShieldDObjDesc
  efmanager.c:420   dEFManagerFoxReflectorEffectDesc
                    -> llFoxSpecial2ReflectorDObjDesc + ...StartAnimJoint,
                       texture file gFTDataFoxSpecial2, render gcDrawDObjTreeForGObj
  efmanager.c:201   dEFManagerImpactWaveEffectDesc   <- the hard-landing wave
                    -> llEFCommonEffects1ImpactWaveDObjDesc + MObjSub
                       + AnimJoint + MatAnimJoint, render efManagerImpactWaveProcDisplay

The port draws all four as flat camera-facing quads out of the particle atlas,
using ONE texture lifted from each model's file (e.g. dEFCommonEffects3_
RebirthHalo_glow for the platform). That is why three cycles of atlas work --
bigger cells, more palette entries, source resolution -- never satisfied any of
these rows: the parameters were wrong because the MECHANISM is wrong. "The Halo
is not the correct asset to use" is exactly right; the halo glow is one texture
belonging to a model the port never draws.

WHERE THE SEAM ACTUALLY IS, which is narrower than "implement the DObj path":
the port ALREADY draws effect model trees. battleship_efmanager.c:1080 hands
gcDrawDObjTreeForGObj to gcAddGObjDisplay for effects generally, and it works.
What it substitutes for these rows is the GEOMETRY SOURCE -- :484 builds the
shield with ndsEFManagerBuildDisc(...) and routes it to a hand-written
ndsEFManagerShieldProcDisplay, and the rebirth halo gets the same treatment.
A procedurally generated disc is standing in for a loaded model.

THE EXACT MECHANISM, and the port already wrote it down twice:

  Makefile:1401 (NDS_R2_SOURCE_EFFECTS_FULL ?= 0)
    "these submit their geometry as source effect DL links, which the battle
     hardware path does not consume, so routing them trades a visible
     untextured primitive for nothing on screen. That is the same seam that
     kept the respawn platform invisible."
  battleship_efmanager.c:1240
    "source effect DL links are not submitted by the battle hardware path,
     which is why the respawn platform was invisible."

So the machinery is all present and PROVEN: efManagerMakeEffectForce loads a
source DObjDesc and gcDrawDObjTreeForGObj draws it -- that is exactly how the
KO burst (dEFManagerDeadExplodeEffectDesc) already works, unconditionally, with
its offsets resolved by ndsEFManagerResolveAllDescOffsets. dEFManagerShieldEffectDesc
and dEFManagerImpactWaveEffectDesc are ALREADY in that resolve list; they are
just gated off.

WHAT IS MISSING IS DOWNSTREAM OF THE CAMERA, and an earlier version of this
note said otherwise -- it claimed the battle camera does not capture these
links. That is FALSE and was written without reading gmcamera.c. The source
battle camera captures them in five passes (gmcamera.c:1057-1099):

    pass 1: links 2, 1
    pass 2: link 4
    pass 3: links 12, 11, 10, 9, 7, 6      <- 10 is the impact wave
    pass 4: links 15, 14, 13               <- 15 is the Fox reflector
    pass 5: links 18, 17, 16               <- 18 is where the stand-ins live
    pass 6: links 20, 19

and the port compiles that function. So the camera mask is not the gap. Pass 5
demonstrably reaches the screen, because the procedural stand-ins on link 18
draw.

ANSWERED 2026-08-03, AND IT WAS NEVER A PASS QUESTION. "Which capture passes
does the hardware path submit" was the wrong shape of question: the gate is
PER-GObj, not per-pass, and it tests a display-list pointer. See "THE THIRD
HALF" below. The pass table stays because it is correct and it is what rules the
camera out; it is not where the work is.

THE WORK, in order:
  1. Admit source models at the hardware submit gate (done -- third half below).
  2. Set NDS_R2_SOURCE_EFFECTS_FULL = 1.
  3. Retire the ndsEFManagerBuildDisc stand-ins for shield and rebirth.
MEASURED 2026-08-03, one build: with NDS_R2_SOURCE_EFFECTS_FULL=1 the
**Boundary profile PASSES**. That is half the acceptance test and it is the half
everyone assumed would fail -- the flag has been sitting at 0 partly on the
memory fear below. The ROM was rebuilt back to the tracked default afterwards so
the published artifact matches the tracked configuration; nothing is shipped on
a half-measured gate.

ACCEPTANCE, already defined at Makefile:1409, and what is LEFT of it: a soak
whose gNdsTaskmanGeneralHeapFreeMin stays above 25,600, AND a capture showing
they draw. The heap half is unmeasured -- a passing Boundary run does not print
MEMARENA, so it needs a soak or a marker dump. Get that number, take a capture
of a shield and a respawn, and if both hold, flip the default. The 25,600 matters because ifCommonSetMaxNumGObj latches the GObj pool
below it -- an earlier attempt at this flag capped the pool for a whole match,
so this is a memory-risk change and not a cosmetic one.

A REAL DEFECT, FOUND 2026-08-03:
ndsRendererAdapterSubmitStageDObj (reloc_backend_renderer_dl.c) submitted the
DObj it was handed AND STOPPED. The source's gcDrawDObjTree (objdisplay.c)
recurses into `child` and walks the whole `sib_next` chain from the first
sibling. So every multi-node effect model was drawn ONE NODE DEEP.

Attributing the owner's "1/4 slice of the complete circle" to THIS is a guess
and is not evidence: that row already has a measured explanation four lines
down -- a 1:2 source cell drawn on a square atlas quad. Both are real; which one
the owner was looking at is unsettled and only a capture decides it.

Three halves are needed and ALL THREE are now written, all behind
NDS_R2_SOURCE_EFFECTS_FULL:
  1. Routing. The makers reached weak shims that call ndsEFManagerMakeVisualEffect
     (a procedural disc, tagged NDS_TASK39_EFFECT_SUBSTITUTE in its own census).
     Strong definitions in battleship_efmanager.c now forward shield, rebirth
     halo and Fox reflector to their ndsBase* source makers.
  2. Traversal. The tree walk above, bounded at depth 16 / 64 siblings with
     overrun counters, because these trees come from resolved offsets that have
     held garbage before. Effect call site ONLY -- see measurement 1 below.
  3. Admission. The submit gate, immediately below, which is the one that had
     never been found.

THE THIRD HALF, FOUND 2026-08-03 AFTER MEASURING THE FIRST TWO, AND IT IS A
LINK COVERAGE GAP. The codebase was carrying its own control the whole time.
Every EFDesc names the display link it draws on (efmanager.c, desc field 2):

    dEFManagerShieldEffectDesc        :460   link 15
    dEFManagerFoxReflectorEffectDesc  :420   link 15
    dEFManagerRebirthHaloEffectDesc   :1648  link 10
    dEFManagerImpactWaveEffectDesc    :201   link 10
    dEFManagerDeadExplodeEffectDesc   :850   link 18   <- the KO burst

The first four are the four rows that have never drawn. The fifth is the KO
burst, which this file already records as working "unconditionally" through the
SAME source-model route. The only thing that differs between them is that
field -- and ndsStageGCDrawAllLoopIsEffectDisplay required 18.

So the hardware effect submit accepted exactly one link, the procedural
stand-ins were hard-wired onto it (ndsEFManagerMakeVisualEffect passes 18 to
gcAddGObjDisplay), and the source models on 10 and 15 were never offered to the
hardware at all. THAT is what "the battle hardware path does not consume source
effect DL links" meant. It was written twice, neither copy said where, and three
cycles looked in the atlas, then in the camera's capture passes, then in the
tree walk. The camera was never the gap: it captures 10 in pass 3 and 15 in
pass 4, per the table above.

A SECOND GATE SITS BEHIND IT and also had to open: ndsEFManagerIsVisualEffectGObj
(efmanager.c:969) returns TRUE only when dobj->dl matches a procedural TEMPLATE
pointer, which a ROM-asset display list can never do. Both are open now, flag-
gated, counted by gNdsEffectRendererSourceModelAdmitCount.

THE CALLBACK KIND WAS THE FOURTH GATE, and it is measured rather than guessed.
The source models reach the submit as DLHEAD0 and as NOTHING ELSE -- observed
kind mask 0x8, no other bit -- while the submit accepted DOBJ_TREE alone. That
refused all six frames before the tree walk ever ran.
ndsRendererAdapterSubmitStageDObjNode already handles DLHEAD0 in the same switch
arm as DOBJ_TREE, so only the guard needed changing, and it is flag-gated
because the default configuration carries the procedural template effects and
their accepted kind is not being changed on a measurement taken with those
effects absent.

(The mask itself nearly lied: these kinds are FOURCCs -- DOBJ_TREE is
0x44545245, "DTRE" -- so the obvious `1u << kind` shift-mask is undefined and
`kind < 32` never true. It would have read 0 and been reported as "nothing
arrives". ndsStageGCDrawAllLoopCallbackKindBit maps them explicitly.)

WHERE IT STANDS, probe-shield-vfx.ps1 -DrawCounter
gNdsEffectRendererSourceModelAdmitCount, flag on, three successive runs:

    gate opened      admit=12 capture=6 dobjdraw=6 reject=6 tris=0 nodes=0
    kind accepted    admit=12 capture=6 dobjdraw=6 reject=6 tris=0 nodes=6
                     rejkinds 0x8 -> 0x0
    kindmask=0 throughout: no procedural stand-in exists, so all twelve
    admits are source models.

Structurally-zero to twelve admits, past the guard, and THE WALK NOW RUNS. The
shield still does not appear, and TWO HYPOTHESES FOR WHY WERE TESTED AND BOTH
DIED. Recorded because each cost a build and neither should be tried again:

  "a DObj flag makes it undrawable" -- the DLHEAD0 branch of
  ndsRendererAdapterStageDObjDrawable (renderer_dl.c:5886) is the STRICT
  `flags == DOBJ_FLAG_NONE`, where DOBJ_TREE only tests `(flags & NOTEXTURE)`.
  REFUTED: dobjflags reads 0x0. The node is drawable.

  "the geometry lives in dl_link[], which only the *_DLLINKS kinds read, while
  DLHEAD0 submits `dl` and guards on `dv` -- a different field" -- REFUTED:
  the field mask reads 0x7, so dl, dl_link and dv are ALL non-null, and routing
  DLHEAD0 to TREE_DLLINKS left tris at 0 exactly as before. That routing was
  reverted; it bought nothing.

WHERE IT ACTUALLY STOPS: the node is drawable, the walk reaches it, every
candidate geometry pointer is populated, and ndsRendererAdapterSubmitStageDL
emits nothing from the display list. texready=0 AND texreject=0, so it does not
reach texture handling at all. (The follow-on "instrument inside SubmitStageDL"
is SUPERSEDED -- see the cycle-3 section below. The DL it is handed is one
shared stand-in list, identical on every effect instance, so what SubmitStageDL
does with it was never the interesting question.)

SEPARATELY, and it is a different defect: nodes=6 over 6 frames is ONE NODE PER
FRAME, but llFTManagerCommonShieldDObjDesc is a THREE-node desc (efmanager.c's
own note: three nodes, one of which carries a 21-command DL over four vertices).
So the effect GObj's DObj has no child and no sibling chain -- the tree was
never built. Having a correct walk is what made that visible.

WHAT THE MEASUREMENTS SAID, and one of them retracts a claim above:

1. The tree walk's cost was REAL and was MINE. Recursing inside
   ndsRendererAdapterSubmitStageDObj -- the STAGE entry -- re-walked all 57
   stage DObjs. Synchronized tick-HUD, identical frames:
       frame 441  control 1,119,936  candidate 1,120,000
       frame 443  control 1,119,872  candidate 1,120,000
       frame 447  control 1,119,488  candidate 1,680,384   2 VBlanks -> 3
       frame 449  control 1,119,872  candidate 1,680,256   2 VBlanks -> 3
   +560,896 is one VBlank (560,190) on frames that were inside the gate. The
   recursion is on the EFFECT call site only now, and the stage entry is a
   single-node submit exactly as it was. Re-measured on one instrument:
   p50 1,119,872 vs 1,119,936, p95 1,679,872 vs 1,680,128, VBI 388/67/11/4 both
   arms, max 19 both. No pacing cost.

2. THE "380-SECOND TIMEOUT" WAS NOT A PERFORMANCE RESULT and the sentence that
   called it one is withdrawn. probe-shield-vfx.ps1 armed on
   gNdsTask39FxShieldDrawCount, which belongs to the PROCEDURAL stand-in's
   display proc. With the flag on that stand-in is gone, the counter never
   advances, and the probe spends its whole budget reporting "no shield drawn"
   about a build whose shield it cannot see. It takes -DrawCounter now.

3. THE PACING HARNESS CANNOT JUDGE THESE ROWS AT ALL.
   sample-tick-hud-buckets.ps1 over its window reads
   gNdsVisualEffectCreateCount=0 and gNdsVisualEffectKindMask=0 -- ZERO effects
   of any kind are created. Its A/B therefore only ever priced the flag with no
   effect on screen. (The stage-path regression still showed up there because
   that recursion fired on stage DObjs every frame regardless.) Any future
   verdict on these four rows needs a harness that produces the effect.

THE SILENT RETURN IS RETRACTED -- IT NEVER FIRES (2026-08-03, cycle 3).

The "confirmation" below was a WRONG BREAKPOINT ADDRESS, and the lesson is that
gdb's line table is not evidence about which instruction you stopped on.
`*0x2039592` was chosen because gdb answered "line 8572". Disassembled, it is

    203958a  ldr r2,[r5,#0xa0] / cmp against a global / store the max
    2039592  bhi 2039596      <-- the breakpoint
    2039594  b   2039300      <-- 0x2039300 is the EPILOGUE

i.e. a high-water-mark update on the COMPLETED path, where r5 is a stack address
and r4 is long since clobbered. That is why `dobj` read 0x23c8a58 on all eight
hits across many frames and many effects: a real DObj* cannot be constant. The
readings `dl=0x14006`, `dl=0x3f800000` and the Vec3f-as-display-list inference
drawn from them are all VOID. This also closes the (a)/(b)/(c) split recorded
below in favour of (a): r5 was not dobj.

The real early return, taken from the disassembly instead of the line table:

    20395ec  movs r1,#8 / movs r0,r4 / bl ndsFighterDLScanRangeInTaskmanArena
    20395f4  cmp r0,#0
    20395f6  beq 20395fa
    20395fa  b   2039300          <-- renderer_dl.c:8573, r4=dl and r5=dobj

Measured at `*0x20395fa` over 901 frames on the same flag-on ROM: rejects=0.
The silent return does not fire at all, so it was never the seam and nothing
downstream of it needs instrumenting for that reason.

TWO OF THE FOUR DESCS WERE NEVER RESOLVED
(2026-08-03, two GDB reads on the existing flag-on ROM, no rebuild).

The descs themselves, read live:

    dEFManagerRebirthHaloEffectDesc  o_dobjsetup = 0x20E8610   <- a RAM ADDRESS
    dEFManagerImpactWaveEffectDesc   o_dobjsetup = 0x7C28
    dEFManagerShieldEffectDesc       o_dobjsetup = 0x300

Only a desc named in battleship_efmanager.c's NDS_EF_MANAGER_DESCS list is ever
passed to ndsEFManagerResolveDescOffsets, and the rebirth halo and Fox's
reflector were not in it. An unlisted desc keeps the generated symbol's ADDRESS
in a field efManagerMakeEffect uses as a byte offset, so it computes
*file_head + 0x20E8610 and hands gcSetupCustomDObjs a DObjDesc megabytes past
the file. The one-node tree the note below calls "a different defect" is not
different -- it is that same value.

Both are in the list now, and ndsEFManagerFileSpan knows gFTManagerCommonFile
and gFTDataFoxSpecial2 as well as the three EF-common files, because span 0 made
ndsEFManagerResolveDescOffsets return BEFORE validating anything and the shield
and the reflector were both in that hole. Measured: resolved 38 -> 42.

AND IT IS STILL NOT ENOUGH. On the fixed flag-on ROM the probe reads
admit=12 dobjdraw=6 submit=0 reject=6 tris=0 texready=0 texreject=0 nodes=6,
i.e. unchanged. Two new facts came with it and both are new information rather
than the old dead ends: disabled=1 (a desc failed validation and was neutralised)
and unknownfile=1 (a desc's file is still unknown to the span table, which for
this list can only be gFTDataFoxSpecial2 -- every other desc uses
gEFManagerFiles[0..2]). gNdsEFDescDisabledLast/UnknownFileLast now hold the
EFDesc address so one printf names them; they are unread as of this note.

The impact wave complicates the story and the next cycle should start there
rather than assume: the DObj dumped at the reject is WELL FORMED -- dl=0x2367D98
inside EFCommonEffects1 (asset 83, 52,736 bytes), sane translate/rotate/scale,
an anim_joint and an MObj -- so the rejected 0x14006 is NOT that node's dl. Some
caller is handing SubmitStageDL a pointer that is not dobj->dl, and that is the
open question. Do not re-run the desc read; it is above.

AND THE FLAG-ON ARM NOW CRASHES, WHICH IS THE NEXT BLOCKER AND IS ATTRIBUTED.
A 4-minute both-CPU soak of the flag-on ROM stopped after 1,032 presented frames
with the PC in armWaitForIrq and every general register 0 -- calico's idle
thread, i.e. the game thread is gone -- and sp_abt/lr_abt populated
(lr_abt = threadSwitchTo+120), so it took a DATA ABORT rather than stalling.

It is NOT the memory bound this flag has been feared for, and that fear can now
be retired with a number: GENERALHEAP free = 130,080 against the 25,600 latch,
COMMONSMAX = -1 (the GObj cap never fired), MALLOCOVF = 0, TASKARENA 1,253,376.
Nor is it either closed freeze class: gNdsObjmanPanicCount and
gNdsObjAnimRunawayCount both read 0.

The honest attribution is this change. Resolving the rebirth halo's desc means
efManagerMakeEffect finally builds its REAL DObj tree instead of a garbage
one-node one, and KOBURST att=1 says a KO -- and therefore a respawn -- happened
in that run. A construction that never ran before now runs and faults. That is
progress in the sense that it is the next real seam, and it is why the flag
stays at 0: nothing shipped is affected, because both new descs are inside
NDS_EF_MANAGER_DESCS_FULL and the default build resolves only
dEFManagerDeadExplodeEffectDesc, whose file (&gEFManagerFiles[1]) the span table
already knew. Boundary passes at the tracked default.

TWO MORE HYPOTHESES DIED ON 2026-08-03. Neither should be tried again.

  "the files are loaded WITHOUT their internal fixups applied, because nothing
  ever drew from them before" -- REFUTED WITHOUT SPENDING A RUN, from the
  loaded-file table already captured in
  artifacts/verification/2026-08-03_effectfile-fixups-probe.txt. All 38 resident
  files read internal_fixups_applied=1 AND external_fixups_applied=1, including
  163 (gFTManagerCommonFile, 912 B, intern_count 4), 83/84/85
  (EFCommonEffects1/2/3) and 346. Zero files read 0. Unapplied fixups cannot
  explain the foreign pointers or the respawn abort.

  "the foreign pointers come from the DLLINKS arm, which walks dobj->dl_link as
  DObjDLLink{list_id, dl} pairs -- and dl_link is the SAME UNION MEMBER as dl,
  so over a real display list it would read DL words as {list_id, dl} and submit
  any whose list_id lands in 0..3, which is exactly 0x14006 and 0x3F800000" --
  REFUTED BY ITS OWN BREAKPOINT. `break *0x203971e` resolved to renderer_dl.c
  :9011 and NEVER FIRED in 300 seconds. That arm does not run for these effects.
  The mechanism is real and the arithmetic works; it simply is not what happens.

A THIRD HYPOTHESIS DIED OFFLINE, WITHOUT AN EMULATOR (2026-08-03, cycle 3).

  "fixups are applied, but the table does not COVER every pointer -- the root
  node's dl offset is in it and the child/sibling nodes' are not" -- REFUTED by
  walking the o2r internal reloc chains on disk. A DObjDesc tree is a packed
  {id, dl, translate, rotate, scale} array of stride 0x2C terminated by id==18
  (DOBJ_ARRAY_MAX, lbCommonSetupTreeDObjs, lbcommon.c:914), so every node's dl
  offset is computable without a run:

    163 FTManagerCommon  shield   @0x300   2 nodes, 4 chain slots
                                  node0 dl=NULL   node1 dl@0x0330 COVERED
    85  EFCommonEffects3 rebirth  @0x2AC0  3 nodes, 143 chain slots
                                  node0 dl=NULL   node1 @0x2AF0 and node2 @0x2B1C COVERED
    346 FoxSpecial2      reflector@0x2B0   2 nodes, 17 chain slots
                                  node0 dl=NULL   node1 dl@0x02E0 COVERED

  Every non-NULL dl is a chain slot and every uncovered one is genuinely NULL --
  the id==0 root node is transform-only and carries no geometry, which is also
  why "three-node desc" and "two desc entries" are both right: the DObj tree is
  root + 2 children, of which exactly ONE draws.

  THE IMPACT WAVE IS NOT A DESC AT ALL, and this is why its offsets looked
  sparse at first. dEFManagerImpactWaveEffectDesc's flags are EFFECT_FLAG_USERDATA
  WITHOUT 0x4, so efmanager.c:1999 passes o_dobjsetup to
  gcAddChildForDObj(dobj, (void*)(addr + o_dobjsetup)) -- the argument IS the
  display list. Decoded at 0x7C28 it is ordinary F3DEX2 (E7 pipesync, E3
  setothermode, FC setcombine, FD settimg, 01 vtx, 06 tri2), and all three of
  its pointer words -- both FD texture images and the 01 vertex array -- are
  chain slots. The generator's fixup emission is correct for all four rows; do
  not go looking there again.

WHAT THE FLAG-ON ROM ACTUALLY DOES, measured at the effect-only walker
(2026-08-03, `*0x2039778` = ndsRendererAdapterSubmitStageDObjTreeDepth, reached
only when NDS_R2_SOURCE_EFFECTS_FULL=1, so it is cheap and unambiguous; r0=dobj
r1=kind are argument registers). 72 hits over 901 frames, and every one of them
identical:

    kind=0x444C4830 ("DLH0")  child=NULL  sib_next=NULL  sib_prev=NULL
    ptr=0x2367F58  flags=0x0  gobjid=1011  dllink=10
    CHAIN admit=144 dobjdraw=72 submit=0 reject=72 tris=0 texready=0 texreject=0
    WALK  nodes=72 depthovr=0 sibovr=0 kinds=0x8 rejkinds=0x0

Three things follow directly and none of them need another argument:

  * nodes == dobjdraw == 72 is ONE NODE PER DRAW. The walker never recurses,
    because child and BOTH sibling links are NULL. The 2- and 3-node trees the
    static walk above proves are in the assets are still not being built.
  * flags=0x0 and ptr!=NULL, so the drawable test passes and the DLH0 arm IS
    taken -- SubmitStageDL is called (renderer_dl.c:8989, `bl` at 0x203974A) and
    returns without emitting. rejkinds=0x0 confirms the entry guard at
    movement.c:13114 never fired, so reject=72 is the SECOND site, :13181,
    which is simply `triangle_delta == 0`. submit=0 means "no triangles", NOT
    "never called" -- it is only incremented past the submit.
  * ptr is the SAME 0x2367F58 on all 72 draws, across two different DObj
    addresses. One display list is standing in for every effect instance.

`fields=0x7` was never a refutation of anything. dl, dl_link and dv are the SAME
UNION MEMBER, so one non-NULL pointer sets all three bits; the mask can only
ever read 0x0 or 0x7 and says nothing about which field the geometry is in.

CYCLE 4 NAMED THE 72 DRAWS AND THE ONE-NODE PARAGRAPH ABOVE IS WITHDRAWN
(2026-08-03, one GDB session, both measurements, no rebuild).

The Task39 census could not answer this: gNdsTask39Effect{Names,SpawnCount,
OriginalCount,SubstituteCount,SkippedCount,Routes} are ALL ABSENT from the
flag-on ELF (only gNdsTask39Fx* survive). Naming one would have lost the run --
nm-check first. gcAddGObjDisplay is the better instrument anyway, because
efmanager.c:1974 registers proc_display and dl_link there and every candidate
maker has a distinct proc, so `info symbol $r1` names it outright:

    DISP link=10 proc=0x20928CD -> efManagerImpactWaveProcDisplay + 1
                 lr             -> efManagerMakeEffect + 83        (x8)

ALL 72 LINK-10 DRAWS ARE THE IMPACT WAVE, AND IT IS ALREADY ON THE SOURCE PATH.
Not a stand-in -- the substitute registers nNDSVisualEffectImpactWave on link 18
and never appeared. So the "add a strong override for the impact wave"
suggestion is RETRACTED: efManagerMakeEffect already builds it.

AND ITS ONE-NODE SHAPE IS CORRECT BY CONSTRUCTION, which is what invalidates the
paragraph this replaces. dEFManagerImpactWaveEffectDesc's flags are
EFFECT_FLAG_USERDATA and nothing else -- and EFFECT_FLAG_USERDATA is 0x2, NOT
0x1 (efdef.h:7). So efManagerMakeEffect takes `effect_flags & 0x1` FALSE and
then `effect_flags & 0x4` FALSE, landing on

    lbCommonInitDObj3Transforms(gcAddDObjForGObj(effect_gobj,
                                (void*)(addr + o_dobjsetup)), ...)

ONE DObj holding the raw display list, no children, no siblings. That is exactly
the child=NULL sib=NULL ptr!=NULL DLH0 node the cycle-3 walk measured 72 times.
Nothing was flattened. `ptr` is identical on every draw because every impact
wave is the same file at the same offset: 0x2367F58 = EFCommonEffects1 + 0x7C28.

THE TREE BUILDER IS FINE, MEASURED DIRECTLY. Breaking gcSetupCustomDObjs by name
(objanim.c:2413, r0=gobj r1=dobjdesc) catches it running from
efManagerMakeEffect+145 on effect GObjs with correct pointers and correct bytes:

    SETUP gobj=0x23C6880 desc=0x235ED40  n0={id=0 dl=0x0} n1={id=1 dl=0x235ED30} n2={id=18}
    SETUP gobj=0x23C8D08 desc=0x2372518  n0={id=0 dl=0x0} n1={id=1 dl=0x23724E8} n2={id=1 dl=0x23724F8}

which is the on-disk shape exactly. It walks id==0 -> gcAddDObjForGObj(gobj,
dl) and id!=0 -> gcAddChildForDObj, so those descs DO produce a proper tree
whose root is transform-only. No byte-order defect, no wild desc pointer, no
stubbed callee. Do not re-measure this.

WHAT THIS MEANS FOR THE LADDER, and it is the important part: the shield, the
rebirth halo and the reflector NEVER SPAWNED in either 901-frame run. No
efManagerMakeEffect registration on link 15 appeared, and no rebirth occurred.
Every tris=0 / texready=0 number on this board is IMPACT-WAVE data. Those three
rows have still never been observed, so gate 1 cannot be judged from any capture
taken so far, and the reflector's missing strong override
(ndsBaseEFManagerFoxReflectorMakeEffect, battleship_efmanager.c:1441, still
unreferenced) has never had a chance to matter.

CYCLE 5 SPAWNED ALL OF THEM, AND THE BLOCKER IS NOT THE HARNESS (2026-08-03).

A 6001-frame flag-on session with gcAddGObjDisplay armed. It never reached its
budget, and how it stopped is the finding:

    DISP15 f=576 gobj=0x23C6880 proc -> efManagerShieldProcDisplay + 1
                                lr   -> efManagerMakeEffect + 83
    DISP15 f=646..653 (x6)      proc -> lbCommonDObjScaleXProcDisplay + 1
    DISP18 f=816 gobj=0x23C8D08 proc -> gcDrawDObjTreeDLLinksForGObj + 1
    DISP10 f=838 gobj=0x23C8D08 proc -> gcDrawDObjTreeDLLinksForGObj + 1
    <nothing, ever again -- gdb CPU flat at 1.66s for the next 1,580 seconds
     while melonDS burned a full core>

THE SHIELD IS ON THE SOURCE PATH AND ITS OVERRIDE WORKS. Frame 576, link 15,
efManagerShieldProcDisplay, from efManagerMakeEffect -- not a stand-in. And its
GObj is 0x23C6880, the SAME GObj cycle 4 caught in gcSetupCustomDObjs with
correct desc bytes (n0={id=0 dl=0} n1={id=1 dl=0x235ED30} n2={id=18}). So the
shield is constructed correctly, from the source desc, with a proper 2-node
tree. Construction is finished as a suspect.

BUT IT IS NEVER DRAWN. The effect tree walker breakpoint (*0x2039778, which
fires for every effect submit) printed NOT ONE line for a non-link-10 node in
the 262 frames between the shield's creation and the hang. The shield exists,
has a tree, and never enters ndsRendererAdapterSubmitStageDObjTreeDepth at all.
The refusal is upstream of the walker, at the admission gate in movement.c --
that is the next seam for the shield, and it is a different seam from anything
this board has chased so far.

THE HANG FOLLOWS THE REBIRTH HALO BY ~20 FRAMES. Both halo registrations land
(f=816, f=838) and then every breakpoint stops firing forever while the guest
keeps executing. This is the cycle-1 "flag-on arm crashes" defect, localised
from "stopped after 1,032 frames" to "within ~20 frames of the rebirth halo's
display registration". It is why no flag-on run has ever passed ~1,000 frames,
and it -- not the run length -- is what has kept the ladder shut.

THE REFLECTOR IS STILL UNMEASURED. No gcDrawDObjTreeForGObj registration
appeared before the hang, which is NOT evidence that its missing strong
override matters; the run simply died first. Do not add that override on this
run's strength.

lr_abt WAS NOT CAPTURED, and the reason is a probe defect worth not repeating:
Invoke-GdbMarkerScript THROWS on timeout, so an abort read placed after the
call never executes and the finally block kills the emulator first. When the
thing being hunted is a hang, timeout is the EXPECTED path -- the abort read
belongs in a catch.

THE REFLECTOR DOES NOT NEED A STRONG OVERRIDE. ITS DESC IS DISABLED (cycle 8).

The shield capture run answered the override question as a side effect, and the
answer is the opposite of the standing plan:

    EFDESC     resolved=42 disabled=1 unknownfile=1 span=52736/28352/13616
    EFDESCLAST disabled=dEFManagerFoxReflectorEffectDesc

ndsEFManagerResolveDescOffsets fails its span lookup for gFTDataFoxSpecial2,
takes the fail-closed branch and sets desc->proc_display = NULL. efManagerMakeEffect
returns at `if (effect_desc->proc_display == NULL) return effect_gobj;`
(efmanager.c:1972) BEFORE it creates a single DObj. So a strong
efManagerFoxReflectorMakeEffect would call a maker that cannot build anything --
adding it would have produced no change and looked like a failed hypothesis.
DO NOT ADD THAT OVERRIDE. Fix the span/validation hole first; the override
question only becomes real once the desc resolves.

This is the same silent hole as the span-0 validator that hid the shield and
the reflector once already, and it is the last of that family still open.

THE FLAG-ON HANG IS GONE, AND THE SURVIVING RUN IS THE EVIDENCE (cycle 7).

The same probe that stopped at ~838 frames for three cycles now reaches its
full budget and prints its own tail:

    frames=1801  admit=7813 dobjdraw=3887 submit=3845 reject=42
                 tris=82,408 texready=37,908 texreject=0
                 nodes=9,262 capture=3,926 rejkinds=0x0 blocker=0

1,801 frames against a previous death at ~838, a 98.9% submit rate, and the
timeout/abort path never armed. No separate fix was needed: the hang was a
consequence of the refusal, not an independent defect.

HONEST LIMIT ON THAT CLAIM: this run did not carry a KO counter, so "it crossed
a respawn" is inferred from surviving well past the frame where the rebirth
halo previously spawned and killed it, not measured directly. A KO counter in
the next soak would close it properly.

CYCLE 6 FIXED IT. THE SOURCE EFFECT MODELS DRAW (2026-08-03).

THREE DEFECTS, ALL THE SAME CLASS: a source DObjDesc tree's id==0 root is
TRANSFORM-ONLY. gcSetupCustomDObjs builds it with gcAddDObjForGObj(gobj,
dobjdesc->dl) and node0's dl is NULL in every one of these descs, so the
geometry hangs off the child. Three separate places demanded geometry at the
root or refused the shape outright:

  movement.c:12864  admission gate   required root->dv != NULL
  movement.c:13125  submit guard     required root->dv != NULL (identical test,
                                     and relaxing only the gate did nothing --
                                     admit went 12 -> 468 while reject went
                                     6 -> 219 and the walker still ran 6 times)
  movement.c:13082  accepted kinds   took DOBJ_TREE and DLHEAD0 only

Only the impact wave ever passed, because its EFDesc omits flag 0x4 and it
therefore gets ONE DObj holding the display list directly. That is why every
admitted draw for three cycles was an impact wave.

The third one was found by measurement rather than reading: the moment the root
rule was relaxed the observed-kind mask went 0x8 -> 0xa with reject mask 0x2,
which is DOBJ_TREE_DLLINKS exactly. That is the rebirth halo, whose EFDesc names
gcDrawDObjTreeDLLinksForGObj, so a DL-links tree is its correct shape.
ndsRendererAdapterSubmitStageDObjNode already walked it.

GATE 1, one 781-frame flag-on run, before -> after:

    submit    0 -> 213        reject   219 -> 6
    tris      0 -> 10,551     texready   0 -> 5,070
    nodes     6 -> 663        rejkinds 0x2 -> 0x0

Boundary passes at the tracked default. Note the submit-guard relaxation is NOT
flag-gated -- it is a correctness fix for any tree-shaped effect -- which is
why the default arm was re-verified rather than assumed.

THE CYCLE-5 MICROCODE CONCLUSION BELOW IS WITHDRAWN, AND IT IS THE SAME TRAP
THIS DOC ALREADY WARNS ABOUT. "vtxcmd=0 tricmd=0" was read as "the geometry
commands are not recognised". Every site incrementing vertex_command_count and
triangle_command_count is wrapped in NDS_RENDERER_RECORD_PROOF_ONLY, which is
((void)0) whenever NDS_RENDERER_HW_TRIANGLES is set -- dead in every build that
can draw. The live fields are vertex_count/triangle_count, and on the same wave
they read vtx=18 tri=16: THE EXECUTOR DECODED THE GEOMETRY ALL ALONG. There is
no F3DEX/F3DEX2 variant mismatch and no config seam to fix. blocker=0,
commands=39 and unsupop=0 from that run remain valid; only the two *_command_
counts were dead. gNdsEffectDL* now publish the live fields.

--- superseded, kept for the reasoning only ---
THE IMPACT WAVE EMITS NOTHING BECAUSE ITS GEOMETRY COMMANDS ARE NOT RECOGNISED
(2026-08-03, cycle 5, the executor's own verdict on the effect submit).

gNdsEffectDL* publish the stats of ndsRendererExecuteDisplayListWithVertexCache
whenever an effect tree submit is on the stack. On the wave:

    publish=6 blocker=0 commands=39 firstop=0xE7 unsupop=0x0 vtxcmd=0 tricmd=0

firstop 0xE7 is the wave list's own first command, so this is the right list and
publish=6 is its engagement proof. blocker=0 is NDS_RENDERER_BLOCKER_NONE and
commands=39 is far past index 19 -- the list RUNS TO COMPLETION and does not
bail. unsupop=0, so nothing was rejected as unknown. And yet vtxcmd=0 and
tricmd=0: it counted ZERO vertex and ZERO triangle commands while the bytes hold
one G_VTX at index 19 and four G_TRI2 at 20-23.

So it is neither segment E nor an early stop. SEGMENT E IS FULLY EXCLUDED, from
the bytes rather than a story: the G_DL at index 14 is 0xDE000000, param byte 0,
which is G_DL_PUSH -- call and return -- and every triangle-emitting command
sits INLINE AFTER it. An empty segment-E stub cannot remove them.

What is left is a MICROCODE-VARIANT DECODE MISMATCH. 0x01 and 0x06 are G_VTX and
G_TRI2 under F3DEX2, which is what this list is (it carries 0xD9 G_GEOMETRYMODE
and 0xDE G_DL, both F3DEX2-only). Under F3DEX v1 the same bytes are G_MTX and
G_DL -- both VALID, which is exactly why unsupop stays 0 and no blocker fires
while the geometry silently never becomes geometry. That is the seam for this
row: which GBI variant the effect submit hands the executor versus the stage.

STILL OPEN, in order: (1) why the shield is refused before the effect walker --
it is built correctly and never submitted;
(2) why the impact wave, correctly constructed with a dl SubmitStageDL accepts,
still yields tris=0. Its DL does contain `G_DL -> segment 0x0E` (0xDE000000
0E000000) before its G_VTX, but that is a call-and-return and an unresolved
segment E returns sNdsRendererAdapterEmptySegmentEDL rather than aborting, so
segment E is NOT a sufficient explanation on its own.
=============================================================================

-Respawn floating platform isn't visible when respawning after KO.
    Owner: is don't see the floating revival platform at all. the Halo is not the correct asset to use
    ROOT CAUSE FOUND (see banner): it is dEFManagerRebirthHaloEffectDesc, a MODEL with joint animation.
    The port draws one texture from that model's file on a flat quad. Resolution was never the problem.
    LOCALIZED: desc was never offset-resolved; listed now, and its real tree build data-aborts flag-on.

-Fox down B VFX is not correct or using correct asset.
    Owner: you are still not using the correct asset for Fox's down B reflector.
    ROOT CAUSE FOUND (see banner): dEFManagerFoxReflectorEffectDesc renders via gcDrawDObjTreeForGObj
    from gFTDataFoxSpecial2. It is an animated model, not a sprite. relocData/346 was the wrong lead.
    LOCALIZED: desc was unresolved and unvalidated; listed now, and its file is the one still unknown.

-Shield VFX not correct
    Owner: texture looks cut in half: `artifacts/visibility/2026-08-03_owner_shield-cut-in-half.png`
    ROOT CAUSE FOUND (see banner): dEFManagerShieldEffectDesc is a model (llFTManagerCommonShieldDObjDesc).
    'Cut in half' is a 1:2 source cell on a square quad -- a symptom of drawing a model as a sprite.
    LOCALIZED: desc resolves (0x300), its one drawing dl IS fixup-covered, and SubmitStageDL never
    rejects. The drawn DObj is a single node with no children, so the desc tree is still not built.

-Hard landing vfx not not using correct asset.
    Owner: incorrect asset for the impact wave is being used
    ROOT CAUSE FOUND (see banner): it is dEFManagerImpactWaveEffectDesc -- DObjDesc + MObjSub + AnimJoint
    + MatAnimJoint. A material-animated model. No single atlas texture can be the 'correct asset' for it.
    A hard landing also emits efManagerDustHeavyDoubleMakeEffect (efmanager.c:2982, script 0x58), which IS
    a particle effect -- so this row has a model half and a particle half, and only the wave is the model.
    LOCALIZED: o_dobjsetup is a raw F3DEX2 display list, not a desc (its EFDesc flags lack 0x4), and
    every pointer in it is fixup-covered. It IS already built by the source efManagerMakeEffect as a
    single DObj on link 10 -- correct by construction -- and still yields tris=0. That is the open part.

-KO VFX not drawing correctly.
    Owner: Not fixed yet, the "blast pillar" VFX isn't drawing, and doesn't seen to draw on the same z axis as the fighters
    TRACED: efManagerSparkleWhiteDeadMakeEffect (efmanager.c:3725) runs particle script 0x5C, driven from
    ftCommonDeadUpStarProcUpdate. Script 0x5C asks for texture 24, which IS admitted (32x32, source size),
    so the sheet is not the blocker here -- unlike the four rows above, this one IS a particle effect.

-Some "hard hit" (side A attacks that hit) VFX look too big, please apply correct scaling to VFX.
    MEASURED: scale is source-exact (efmanager.c:2175/2197). Last cycle's clamp was in UNREACHABLE code.
    
-Shield Freeze is back
    **FIXED** (2026-08-03) all three captures spun in gcParseDObjAnimJoint's event loop, whose `default:` case is
    the one branch that consumes nothing -- an invalid opcode re-reads one word forever with interrupts still on,
    which is exactly a frozen picture that keeps presenting. Measured opcode 100 (the captures' r6=0x64) on a
    script pointer 2 mod 4, which cannot be an AObjEvent32*. All four animation parsers now record the fault and
    end that one animation. 7-min both-CPU soak NO-FREEZE with gNdsObjAnimRunawayCount=10 on mask bit 0.
    Open and no longer fatal: what hands the parser a misaligned script. Not the shield table (49 installs probed
    clean) and not ndsRelocResolvePointerFromFileBase (its offset fallback measured 0 calls).
