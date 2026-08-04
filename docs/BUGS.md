**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

This board carries verdicts and the numbers that check them. The forensics for each
fix live next to the code that owns it -- the particle generator and its checker,
`battleship_lbparticle.c`, `battleship_efmanager.c`, `render-audio-fgm-phase-pack.py`
-- so a row here should not need to be an essay.

=============================================================================
FOUR SOURCE-MODEL EFFECT ROWS -- CURRENT STATE (2026-08-03)
=============================================================================
The respawn platform, shield, Fox reflector and hard-landing wave are not
sprites: each is an EFDesc drawn as an animated DObj model tree. The port drew
flat atlas quads instead, which is why three cycles of atlas work never
satisfied any of them. All four sit behind NDS_R2_SOURCE_EFFECTS_FULL
(Makefile:1411, default 0).

FIXED. A source desc tree's id==0 root is transform-only, and three places
demanded geometry at the root or refused the shape: the admission gate, the
submit guard, and the accepted-kind list (all reloc_backend_movement.c). With
those corrected the models draw -- gate 1 green, submit 0->213, tris 0->10,551,
texready 0->5,070 on a 781-frame flag-on run -- and the long-standing flag-on
hang went with them (1,801 frames, 98.9% submit rate). See 8508fc8d6, fc905460d.

FIXED (2026-08-04) -- the flag-0 tickhud freeze. The port allocated a GObj
thread's coroutine LAZILY, at its first osStartThread, and returned SILENTLY
when malloc failed -- while gcRunGObjProcess had already committed to a
blocking osRecvMesg only that thread could satisfy. The battle announcer is the
first start of its thread (logic frame 390) and its 4,208-byte request failed on
a heap whose sbrk had reached its ceiling exactly, headroom 0. The freeze-fix
parsers had taken 768 bytes of that heap (__heap_start_ntr 0x022915b0 ->
0x022918b0), which is why they read as the trigger and why the defect was never
theirs.
BattleShip cannot fail there: objman.c:882 hands osCreateThread a pooled,
arena-backed, recycled stack, and each GObjThread carries it inline. The port
now builds the coroutine INSIDE that block at osCreateThread -- gcSetupObjman
interposed in battleship_sys_objman.c to size it (4,208 B), and
portCoroutineCreateStatic to carve the context off its top -- so nothing is
allocated at thread-start time at all. Cost is arena, not heap, which is where
the headroom is: 143,072 B of arena high-water headroom remain, and
gobjthread->stack[7] == 0xFEDCBA98 becomes a live overflow guard for the port
too.
Both arms now reach presented frame 540, 345 past the freeze, with
ProvisionFail=0, StartCreateFail=0, StartNoEntry=0 and the heap path down to the
7 boot service threads. Boundary passed; 2.5-min soak NO-FREEZE at 2,043 frames
through GAME SET and Results. WORK-H P95 1,079,680 (flag 0) / 1,057,536 (flag 1),
both inside the 1.12M gate -- gate 5 finally has its control arm.
artifacts/performance/2026-08-04_gobjpool-flag{0,1}-tickhud.json; the sizing and
margin probes are artifacts/verification/2026-08-0{3,4}_gobj*.

GATE 5 PRICED (2026-08-04). The paired A/B the freeze was blocking is done:
128 frames, one tree, HUD draw off, identical melonDS/window/config.
**Flipping the flag is free at the median and slightly worse in the tail, and
neither setting meets the gate.** WORK-H P50 983,104 -> 988,544 (+5,440, inside
the placement floor); P95 1,247,040 -> 1,283,072 (+36,032); paired 47/128
better, 81 worse, median +7,680; over-gate frames 16 -> 17 of 128; FPS <=27.75
vs <=27.71; VBI max 20 vs 19; slips 0 both.
The earlier "flag-on costs FPS 20.0 / ALL 1.68M" warning is SUPERSEDED -- it was
unpaired. So is this row's own first n=40 pass, which read flag 1 as 81,344
AHEAD; at n=128 that reverses, because a 40-sample P95 is the 3rd-worst frame.
Full table, retraction of the cycle-43 percentiles, and the owed
per-frame-paired tooling: docs/PERF_LEDGER.md "GATE 5".

BLOCKED(decision: flip NDS_R2_SOURCE_EFFECTS_FULL default, keep, or optimize
first). Performance no longer decides it -- the two settings cost the same, so
this is a fidelity call and the owner is the oracle. What changes per row:
  * shield bubble    -- WITHDRAWN 2026-08-04. Measured, the flip does not give
                        this row its model; it leaks a detached quad per guard.
                        See the gate-6 block below. Do not count this row for.
  * rebirth platform -- one flat quad becomes the joint-animated model, i.e. the
                        platform appears at all on respawn.
  * Fox reflector    -- NO CHANGE EITHER WAY, see below. Do not count this row.
  * impact wave      -- a static atlas quad becomes the material-animated model.
Against: content-completeness doctrine says stand-ins are a temporary audit
state, not a shipping state, which argues for the flip; the tail is 36,032
worse on a frame already 127,040 over gate, which argues for optimize-first.
EVIDENCE STILL MISSING, and it is what the decision actually needs: no
synchronized before/after capture exists for any of the four. Blocking reason is
tooling, not effort -- a cross-build render comparison must lock on
gSCManagerBattleState->time_remain and no harness can (capture-melonds.ps1
-ExactFirstFrame locks the presented-frame counter, which R2-02 E8 measured into
a 57% false delta). Land -ExactTimeRemain first, then the four captures.

EFFECT MOMENTS ARE NOW KNOWN (2026-08-04), which is what "gate 2 captures not
closed" was actually blocked on -- a capture needs a tic and nobody had ever
recorded one. One no-build probe, breaking on efManagerMakeEffect conditioned on
the four desc addresses, over 150 s of natural flag-1 play:
    rebirth halo  time_remain 2819, 875
    shield        1994, 1951, 1922, 130, 110, 77, 40, 18
    impact wave   3277, 2121, 2110, 2104, 2099, 2088, 1810, 113
    reflector     NEVER -- 0 spawns
Capture a few tics BELOW a spawn tic (time_remain counts down) so the effect is
on screen. artifacts/verification/2026-08-04_gate6-effect-tics.txt.

=============================================================================
GATE 6 STOPS HERE: THE TWO ARMS ARE NOT THE SAME FIGHT (2026-08-04)
=============================================================================
No gate-6 row can move to OWNER-QUEUED, and the blocker is not tooling.

1. THE CROSS-BUILD PAIR IS IMPOSSIBLE AS BUILT. Both arms locked on the
identical simulation tic -- EXACT_LOCK=gSCManagerBattleState->time_remain,
1700,1698 printed by both -- and they are DIFFERENT GAMES at that tic: Mario
44% (flag 1) versus 77% (flag 0), different fighter positions, different camera,
FPS 20.0 versus 29.0. -ExactTimeRemain synchronizes the CLOCK, not the fight; a
realtime-paced port whose arms present at different rates diverges, so a
"before/after" pair at one tic compares two unrelated moments. Every cross-build
pixel number for these rows is therefore meaningless, including this cycle's
48.43%. Closing this needs deterministic input playback, not a capture rerun.
artifacts/visibility/2026-08-04_g6-shield-t1700-flag{0,1}-a.png.

2. THE FLAG-1 SHIELD IS A REGRESSION, NOT THE FIX THE PACKET ASSUMED. Proven on
the capture ROM itself, same build, no cross-build inference: link-15 draws begin
at exactly tic 1994 (the measured shield spawn), step 1 -> 2 -> 3 at 1950 and
1922 (the other two spawns), and NEVER STEP BACK DOWN -- still +3/frame at tic
1500, 494 tics later. Each instance contributes exactly 2 triangles, i.e. one
quad. On screen that is a large translucent quad with a pink crescent parked in
mid-air between the trunks, attached to neither fighter, occluding the stage;
absent at tic 2010 in the same run, before the first spawn. So flipping the flag
does not give the shield its model -- it leaks a detached quad per guard.
Exhibit: artifacts/visibility/2026-08-04_g6-shield-flag1-t1700-detached-blob-
zoom.png, control 2026-08-04_g6-shield-t2010-flag1-preshield-a.png.
Also measured: the flag-0 procedural stand-in drew on ONE frame in a whole
match (tic 3264), so "the flag-0 shield" is close to absent too.
Census both arms: artifacts/verification/2026-08-04_gate6-onscreen-flag{0,1}.txt.

3. A SPAWN TIC DOES TRANSFER; AN OFFSET WAS NEVER THE PROBLEM. The cycle-47
guess that the offset below a spawn tic had to be tuned is refuted -- the shield
draws on every frame from its spawn onward. What the earlier captures lacked was
a recognizable shield, not a tic.

5. WHY THE FLAG-1 SHIELD IS IMMORTAL, WITH THE SOURCE ORACLE (2026-08-04, no
build spent). Source kills an attached effect through the status machine, not
through the effect: ftMainSetStatus, when the caller does not pass
FTSTATUS_PRESERVE_EFFECT and fp->is_effect_attach is set, calls
ftParamProcStopEffect (ftmain.c:4449). That walks
gGCCommonLinks[nGCCommonLinkIDEffect] and, for EVERY effect whose
ep->fighter_gobj matches, runs ftParamStopEffect -> gcEjectGObj
(ftparam.c, ftParamRunProcEffect / ftParamStopEffect). There is NO test of what
kind of effect it is. Guard uses exactly this: ftCommonGuardOnSetStatus passes
FTSTATUS_PRESERVE_NONE and makes the shield (ftcommonguard1.c:430); the setoff
transition passes FTSTATUS_PRESERVE_EFFECT to keep it; leaving guard by any
other status therefore ejects it.
THE PORT ADDS A FILTER SOURCE DOES NOT HAVE. ftParamProcStopEffect is a port
shim (reloc_backend_compat_shims.c:1459) forwarding to
ndsEFManagerStopAttachedVisualEffects (battleship_efmanager.c:1133), whose match
is `ep->fighter_gobj == fighter_gobj && ndsEFManagerIsVisualEffectGObj(...)`.
That predicate (:969) is true only when dobj->dl is one of the
sNdsVisualTemplates[] display lists -- i.e. only for the PROCEDURAL stand-ins. A
source EFDesc effect carries the source model's display list, fails the test, and
is never ejected by anything. That is the whole defect, and it predicts the
measured census exactly: three guards, three shields, draw count 1 -> 2 -> 3 and
never down. It also predicts the admit/frame climb 1 -> 10, since every
fighter-attached SOURCE effect is immortal, not only the shield -- so verify the
climb falls out of this fix rather than fixing it twice.
FIXED (2026-08-04). The kind test demoted from a MATCH filter to a TEARDOWN
discriminator, and the walk renamed ndsEFManagerStopAttachedEffects for what it
actually contracts to do. Source-kind effects now tear down the source way
(lbParticleEjectStructID when ep->xf is non-NULL, efManagerSetPrevStructAlloc,
gcEjectGObj); stand-ins keep ndsEFManagerDestroyVisualEffect.
RE-CENSUS ON THE SAME RUN, before -> after, identical tics:
  link15/frame  1994: 1 -> 1   1982: 1 -> 0   1950: 2 -> 1   1936: 2 -> 0
                1922: 3 -> 1   1914: 3 -> 0
  link15 total at tic 1500      686 -> 17
  admit/frame at tic 1810        10 -> 2, and it now returns to 0 after each
                                 guard instead of ratcheting
  source admits over the window 2851 -> 583
Guard-on steps the count up, guard-end steps it back to baseline, three times.
gNdsEFManagerSourceEffectStopCount = 4 is the engagement control (three guard
ends plus the guard-on that clears whatever was attached, which is source's
FTSTATUS_PRESERVE_NONE doing its job); the setoff transition still passes
FTSTATUS_PRESERVE_EFFECT and is untouched. THE ADMIT CLIMB FELL OUT OF THIS FIX
-- it was the same seam, not its own.
artifacts/verification/2026-08-04_c50-onscreen-flag1-fixed.txt.
ATTACHMENT IS A SEPARATE SEAM, and the "one cause, two symptoms" guess is wrong
for it. Source does not move the shield from its update proc --
efManagerShieldProcUpdate only clears is_damage_shield. It attaches through the
MATRIX: dEFManagerShieldEffectDesc's first DObj transform is main matrix kind
0x4F, and efManagerShieldMakeEffect stores fp->joints[nFTPartsJointYRotN] in the
effect root's user_data.p (efmanager.c:460, :4119). So "parked in world space"
means that matrix kind is not honoured for source effect DObjs on the port; it is
not a dead update process. STILL OPEN, and now confirmed by eye on a live guard:
with the lifetime fixed, the shield is a white quad sitting BESIDE and BELOW Fox
on the platform rather than a bubble enclosing him
(artifacts/visibility/2026-08-04_c50-shield-guarding-t1988-flag1.png, captured
inside the 1994-1982 guard window, EXACT_LOCK 1988/1986). It no longer drifts
across the stage only because it now dies with the guard. Renderer-side, so it
gates on the fidelity budget and the owner's eye.
ATTACHMENT, TWO STARTING FACTS ONLY (2026-08-04, not a confirmation): grep finds
ZERO reads of user_data.p anywhere in src/port/reloc_backend_renderer_dl.c, and
gcSetupCustomDObjs -- the function that builds an effect's DObj tree from the
desc -- is defined at src/import/battleship_grpupupu_ground.c:35, not in the
renderer. Consistent with "nothing binds the effect DObj to its joint", NOT proof
of it: the matrix may be resolved elsewhere. Next cycle starts by reading
gcSetupCustomDObjs for the desc's matrix-kind fields, then does the gdb read of
the effect DObj's resolved matrix against fp->joints[nFTPartsJointYRotN] inside a
guard window BEFORE any edit.
GATE 6: the shield row's flip argument re-arms only when this second seam lands.
Correct lifetime alone does not make the shield correct.

FLAG-0 RE-MEASURED AFTER THE FIX (2026-08-04), because changing the walk could
have changed stand-in lifetime too. It did not: the flag-0 census is unchanged
from before the fix -- gNdsTask39FxShieldDrawCount totals 1 for a whole match
(one frame, tic 3264), same stand-in pool oscillation, and
gNdsEFManagerSourceEffectStopCount reads 0. That zero is the NEGATIVE CONTROL and
it is the useful part: no source effect is attached at flag 0, so the new
teardown branch never fires and the fix is inert on the shipping default.
CONSEQUENCE: the stand-in's one-frame-per-match shield is NOT the lifetime
seam's other face. It is its own defect on the stand-in SPAWN path, unlocalized,
handed forward. artifacts/verification/2026-08-04_c51-onscreen-flag0-fixed.txt.

4. CROPPING DOES NOT RESCUE THE PIXEL METRIC EITHER. compare-capture-pair.ps1
now takes -CropX/-CropY/-CropW/-CropH (viewport-relative, applied to both images,
partial or out-of-bounds crops throw). Measured on the shield's own region at tic
1700: same-build adjacent-present floor 26.74% on the crop against 17.99% on the
whole viewport -- the crop is NOISIER, because the effect sits over the scrolling
canopy. The tool gap is closed and the conclusion is unchanged: these rows are
judged by eye, not by a fraction.
=============================================================================

THE FLOOR IS THE STAGE, NOT THE INSTRUMENT (2026-08-04, corrects the entry
below). Re-measured on a console-reduced ROM and it did NOT collapse: 36.5% and
35.8% same-build adjacent-present, against 49.3% cross-build, at tic 1992.
Higher than the tickhud ROM's ~32%, not lower. Looking at the frame gives the
real cause -- Dream Land's canopy is a dense, high-frequency, SCROLLING texture,
so a one-present shift repaints a third of the screen no matter what ROM it is.
CONSEQUENCE: a whole-viewport pixel metric can never be this row's instrument.
(The crop that was owed here landed; it does not help either -- see item 4 of
the gate-6 block below.)
ALSO: the published target CANNOT be built to a lab dir. Makefile:54-56 --
a PUBLISHED TARGET NAME publishes whatever BUILD says, so
`smash64ds-battle-playable-hwtri` always writes the project root. Use
`smash64ds-battle-playable-proof-hwtri`: same Makefile block, same scene
harness and renderer flags (Makefile:1043-1074), NDS_TICK_HUD=0, and not a
published name, so it lands in builds/. It still carries a small FPS/damage
console; only the bucket table goes away.

TWO CAPTURE-METHOD CORRECTIONS, both found by looking at the first attempt
instead of trusting its pixel count:
  1. Gate-6 captures must come from the PUBLISHED battle target, never the
     tickhud lab ROM. The tick HUD's text console owns the whole bottom screen
     and rewrites FPS/TIME every frame, so it is roughly half the 400x600
     compare area and it is churning. NDS_TICK_HUD_DRAW=0 does not remove it --
     it only blanks the bucket table to dashes.
  2. That console is what the same-build floor is made of: 31.8%/33.4% at
     tic 1990 against a 49.1% cross-build delta. On the published ROM the floor
     should collapse; until it does, no pixel metric here means anything.
  Exhibit: artifacts/visibility/2026-08-04_gate6-shield-REJECTED-tickhud-
  console-noshield.png -- it also shows no shield at tic 1990, so the offset
  below a spawn tic has to be chosen and then VERIFIED BY EYE, not assumed.

OPEN.
  * Reflector: measured, it never gets the chance. Zero dEFManagerFoxReflector
    EffectDesc spawns in 150 s of natural level-3 Fox play, so the down-B simply
    does not occur in an automated window -- the engagement gap is not proof of
    a broken retry. AND the desc is disabled anyway (EFDescDisabledCount=1), so
    a down-B alone would still draw nothing until the retry recovers it. Cheapest
    honest path is the owner's play session, not a new harness mode.
  * Reflector: the deferred-retry fix CANNOT ENGAGE AT THE TRACKED DEFAULT.
    gNdsEFDescDeferRecoverCount is absent from the flag-0 ELF entirely (nm on
    both gate-5 arms) -- it is compiled inside NDS_R2_SOURCE_EFFECTS_FULL. At
    flag 1 it exists and read 0 over 540 frames with EFDescDisabledCount=1, so
    the desc is still disabled and the retry still never fired. Needs a Fox
    down-B on a flag-1 ROM; a flag-0 run can never prove it. See 48fe59693c.
  * Gate 2 captures not closed: link-15 arms the probe correctly now
    (artifacts/visibility/2026-08-03_shield-vfx.png) but link 15 carries the
    shield AND the reflector, so the frame does not prove which drew.
  * Impact wave draws but its triangles do not reach the effect counter.
    Segment E is excluded -- its G_DL is call-and-return and the geometry is
    inline after it. See 4c29b9615a.

Forensics live in the commit messages and artifacts/verification/2026-08-03_*;
do not restate them here.
=============================================================================

-Respawn floating platform isn't visible when respawning after KO.
    Owner: is don't see the floating revival platform at all. the Halo is not the correct asset to use
    CAUSE: dEFManagerRebirthHaloEffectDesc is a joint-animated MODEL, drawn as one flat quad.
    STAGE: BLOCKED(decision: flip NDS_R2_SOURCE_EFFECTS_FULL). No cross-build pair exists; arms diverge.

-Fox down B VFX is not correct or using correct asset.
    Owner: you are still not using the correct asset for Fox's down B reflector.
    CAUSE: dEFManagerFoxReflectorEffectDesc is an animated model from gFTDataFoxSpecial2, not a sprite.
    STAGE: LOCALIZED. Retry counter is compiled out at flag 0 and read 0 at flag 1; needs a flag-1 down-B.

-Shield VFX not correct
    Owner: texture looks cut in half: `artifacts/visibility/2026-08-03_owner_shield-cut-in-half.png`
    CAUSE: dEFManagerShieldEffectDesc is a model; 'cut in half' is a 1:2 source cell on a square quad.
    STAGE: PARTLY FIXED (2026-08-04) -- lifetime correct now. Remaining: it does not ride the fighter's joint.

-Hard landing vfx not not using correct asset.
    Owner: incorrect asset for the impact wave is being used
    CAUSE: dEFManagerImpactWaveEffectDesc is a material-animated model, so no single atlas texture can
    be its 'correct asset'. The row also has a particle half (dust, script 0x58, efmanager.c:2982).
    STAGE: BLOCKED(decision: flip NDS_R2_SOURCE_EFFECTS_FULL). No cross-build pair exists; arms diverge.
    Built correctly as a single DObj on link 10 (EFDesc omits flag 0x4, so a raw display list is right);
    executes fully, but its triangles still do not reach the effect counter. See 4c29b9615a.

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
