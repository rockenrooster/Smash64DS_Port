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
reach texture handling at all. The next seam is inside SubmitStageDL -- what it
does with a display list that came from a resolved source DObjDesc rather than
from the stage's own baked program. Instrument THERE; everything upstream of it
is now proven.

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

THE SILENT RETURN IS CONFIRMED, AND TWO OF THE FOUR DESCS WERE NEVER RESOLVED
(2026-08-03, two GDB reads on the existing flag-on ROM, no rebuild).

Breaking on SubmitStageDL's own early return -- the one at renderer_dl.c:8571
taken when the DL pointer is inside neither a loaded reloc file nor the taskman
arena, and the ONLY exit that leaves tris, texready and texreject all 0 with no
stat of any kind -- fires, on an nGCCommonKindEffect GObj on link 10:

    dl=0x14006   dobj=0x23c8a58 gobjid=1011 dllink=10 child=NULL sib=NULL
    dl=0x3f800000  (same dobj, same frame)

Neither is an address. 0x3F800000 is 1.0f, and DObjDesc is {id, dl, translate,
rotate, scale}, so that is a Vec3f component being submitted as a display list.

Then the descs themselves, read live:

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

So the submitted pointer comes from renderer_dl.c:8989, the only other call
site, and it is `dobj->dl`. Which leaves the contradiction sharper than before
and it is the whole remaining question: the DObj dumped AT a reject has
dl=0x2367D98, inside asset 83 (0x2360170 + 52,736), and
ndsRelocFindLoadedFileContaining is plain address containment with no generation
or ownership test -- so that pointer cannot be refused, yet the breakpoint on
the refusal fired with r5 holding that DObj.

One of these is false and the next cycle should settle WHICH before theorising
further: (a) r5 is not dobj at that PC, (b) the dumped node is not the node that
was rejected -- the address was printed only for rejects 1-4 and the struct dump
was taken at reject 6, (c) dobj->dl is mutated between the submit and the stop.
Print dobj AND dl together on every reject, and dump the struct in the same
breakpoint body rather than after the run. That is one probe.

STILL OPEN: whether the gate fix makes them draw. That is a capture, not an
argument, and it has not been taken yet.
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
    LOCALIZED: desc resolves (0x300) and is validated now; SubmitStageDL's unloaded-DL return is the seam.

-Hard landing vfx not not using correct asset.
    Owner: incorrect asset for the impact wave is being used
    ROOT CAUSE FOUND (see banner): it is dEFManagerImpactWaveEffectDesc -- DObjDesc + MObjSub + AnimJoint
    + MatAnimJoint. A material-animated model. No single atlas texture can be the 'correct asset' for it.
    A hard landing also emits efManagerDustHeavyDoubleMakeEffect (efmanager.c:2982, script 0x58), which IS
    a particle effect -- so this row has a model half and a particle half, and only the wave is the model.
    LOCALIZED: its DObj is well formed with a valid dl; the pointer SubmitStageDL rejects is not that one.

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
