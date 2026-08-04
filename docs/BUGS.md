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
  * shield bubble    -- a 1:2 source cell stretched on a square quad becomes the
                        source's animated model. Fixes "cut in half".
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
    STAGE: BLOCKED(decision: flip NDS_R2_SOURCE_EFFECTS_FULL). Gate 5 priced it free; capture owed.

-Fox down B VFX is not correct or using correct asset.
    Owner: you are still not using the correct asset for Fox's down B reflector.
    CAUSE: dEFManagerFoxReflectorEffectDesc is an animated model from gFTDataFoxSpecial2, not a sprite.
    STAGE: LOCALIZED. Retry counter is compiled out at flag 0 and read 0 at flag 1; needs a flag-1 down-B.

-Shield VFX not correct
    Owner: texture looks cut in half: `artifacts/visibility/2026-08-03_owner_shield-cut-in-half.png`
    CAUSE: dEFManagerShieldEffectDesc is a model; 'cut in half' is a 1:2 source cell on a square quad.
    STAGE: BLOCKED(decision: flip NDS_R2_SOURCE_EFFECTS_FULL). Gate 5 priced it free; link-15 capture owed.

-Hard landing vfx not not using correct asset.
    Owner: incorrect asset for the impact wave is being used
    CAUSE: dEFManagerImpactWaveEffectDesc is a material-animated model, so no single atlas texture can
    be its 'correct asset'. The row also has a particle half (dust, script 0x58, efmanager.c:2982).
    STAGE: BLOCKED(decision: flip NDS_R2_SOURCE_EFFECTS_FULL). Gate 5 priced it free; capture owed.
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
