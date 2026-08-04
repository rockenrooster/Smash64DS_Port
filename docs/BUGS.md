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
  * shield bubble    -- OWNER-QUEUED 2026-08-04 (cycle 59). Six seams later it
                        is a round translucent per-player-coloured bubble on the
                        guarding fighter that dies with the guard. This row now
                        counts FOR the flip, subject to the owner's eye.
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

1b. NARROWED 2026-08-04 BY THIS CYCLE'S OWN PAIR: a cross-build pair IS possible
when both arms present at the same rate. builds/build-c50-flag1 and
build-c53-flag1 are different renderer code, both NDS_R2_SOURCE_EFFECTS_FULL=1,
both ~29 FPS, and at EXACT_LOCK 1988,1986 they are the SAME fight -- shield
spawn at exactly tic 1994 in both, Mario 30% / Fox 0% in both, guest viewport
identical on 120,000 of 120,000 pixels. What diverged in item 1 was a 29 FPS arm
against a 20 FPS one. The blocker is therefore the FPS GAP, not cross-build
comparison as such: a renderer-only change judged at the SAME flag is measurable
today, and only a flag-0/flag-1 pair needs deterministic input.

4b. INPUT PLAYBACK EXISTS AND IS ALREADY THE SEAM (2026-08-04, traced, no build).
osContGetReadData (controller_backend.c:243) serves sControllerPlaybackPads[]
whenever sControllerPlaybackEnabled, bypassing the host keys entirely;
ndsControllerPlaybackReset/SetEnabled/SetConnectedMask/SetPad/CommitFrame are the
whole API and gNdsControllerPlaybackFrameCount/ReadCount its engagement counters.
ndsFighterMarioFoxNaturalMotionPrepare (reloc_backend_movement.c:10381) already
drives it -- and explicitly turns it OFF for the canonical harness: under
NDS_DEV_LIVE_INPUT_PREVIEW the proof arm calls
ndsControllerPlaybackSetEnabled(FALSE), so mode 163 reads live pads. Making two
arms take identical input therefore needs no new harness mode; it is one branch
in that block plus a constant idle pad. There is no RECORDER, though: SetPad is
fed computed values by the natural-motion state machine, not a stored stream, so
"playback" here means programmatic input, not replay. Given 1b, spend this only
if an FPS-gap pair is genuinely required.

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
ATTACHMENT: TWO SEAMS FIXED, A THIRD FOUND, PICTURE UNCHANGED (2026-08-04).
Kind 0x4F is not an XObjTransformKind. Source routes every kind >= 66 through
sGCMatrixFuncList[kind - 66] (objdisplay.c:1161); the battle task installs
dLBCommonFuncMatrixList, whose pair 13 is func_ovl0_800C994C (lbcommon.c:1445).
That callback ignores the DObj's own vectors and loads the matrix from the world
matrix of the joint in dobj->user_data.p -- func_ovl2_800EDBA4 first, then
parts->mtx_translate. That is the entire attachment mechanism.
  1. FIXED. The port had no 0x4F case, so the root fell to
     ndsRendererAdapterBuildDObjFallbackMtx, which for its 0/0/1 vectors builds
     the IDENTITY -- the bubble sat at the world origin.
     ndsRendererAdapterBuildJointAttachMtx implements the callback now. Measured
     live inside the 1994-1982 guard: k0=0x4f, user_data.p bound, and the matrix
     it reads tracks the fighter (x 109.7 -> 123.2 -> 129.9 over three tics).
     The func_ovl2_800EDBA4 call is load-bearing, not ceremony: mtx_translate
     read all zeros without it, so a plain copy collapses the quad to a point.
  2. FIXED, and it is why fix 1 alone changed nothing.
     ndsRendererAdapterCaptureStageWorldSourceKey keys the persistent
     stage-world cache on the DObj's OWN translate/rotate/scale. For an attached
     effect root those are the constants 0/0/1, so the entry matched forever and
     the world matrix was built ONCE per guard -- one local build against ten
     submits. 0x4F joins 0x4B on that function's refusal list, by the rule its
     own comment already states; rebuilds are 2 per frame and tracking.
  3. RETRACTED, THEN MEASURED FROM CODE; THE EXECUTOR IS INNOCENT (2026-08-04,
     cycle 55). The cycle-54 candidate was "the persistent vertex cache's
     matrix_snapshots outrank config->initial_modelview". REFUTED by the code
     and by counters: ndsRendererInitTraversalState installs matrix_snapshots as
     a POOL and then seeds state->modelview from config->initial_modelview
     regardless (nds_renderer.c:6463-6522); the vertex_cache assignment below it
     copies vertex arrays only.
     THE WHOLE MATRIX CHAIN IS GREEN, published by the submitter from its own
     locals (gNdsEffectDLCfgMask and friends, nds_effects.h): cfg=3 (both
     matrices non-NULL), mvt 449217/7544832/-147456 -> 504513 -> 532161, i.e.
     the bound joint's world translation tracking the fighter to the tick;
     seed=1, so InitTraversalState composed a VALID traversal matrix from them;
     mcmd=0, so the list issues no matrix command of its own; xf=4 hwv=6 hwt=2,
     so the quad's four vertices are transformed and submitted. Prep mask stays
     0x7d; bit 1 is legitimately clear because the battle camera's 0x4C folds
     LookAt into the projection.
     AND THE READ THAT STARTED ALL THIS IS DEAD FOR GOOD. At the executor's
     symbol address the first instruction IS the `push`, so r1 IS the config by
     the ABI -- and on the same run config->user read back equal to $r3 while
     the five words before it read depth=0 cmds=0 proj=NULL mv=NULL against the
     code's own 8/8192/non-NULL/non-NULL. A stack object is not readable through
     this gdb stub; same lesson as the 0.0 stack locals, now with a positive
     control. Do not spend another cycle reading a config.
     artifacts/verification/2026-08-04_c55-shield-cfgid.txt and -dlcfg.txt.
  4. THE SEAM WAS NEVER IN THE EXECUTOR: TWO SOURCE LINES, BOTH MISSING
     (2026-08-04, cycles 56-57), and fixing them puts the bubble on the fighter.
     4a. The shield's non-root nodes carry main matrix kind 0x2C = 44 =
         nGCMatrixKindRecalcRotRpyRSca (efmanager.c, desc transform struct 2).
         Despite the enum name it applies NO rotation: gcPrepDObjMatrix case 44
         (objdisplay.c:876) writes a pure SCALED PERSPECTIVE block into
         sGCMatrixMvpF, emits gSPMvpRecalc plus gMoveWd for rows 0-2 ONLY, and
         `continue`s WITHOUT a gSPMatrix -- so the composed MVP's orientation is
         REPLACED while its translation row, which is where the 0x4F attachment
         lives, survives. That is a billboard. The port built it as an ordinary
         local rotate/scale and multiplied it into the world chain, so the quad
         wore the joint's yaw and lay flat in the ground plane. That is why
         fixing 0x4F could not change the picture: it moved a quad within the
         wrong plane. Now implemented in ndsRendererAdapterApplyMvpRecalc,
         selected by the DObj's own XObj kind exactly as the existing 0x47 case
         is -- not by effect/stage mode, and the native stage path defers the
         same kinds to the slow path it already used for 0x47.
     4b. The SIZE is the other callback's middle line. func_ovl0_800C994C sets
         gGCScaleX = |row 0 of the joint's world matrix| (lbcommon.c:1454), and
         case 44 multiplies the perspective by it. For the shield that length IS
         THE GUARD SIZE: ftCommonGuardUpdateShieldCollision writes
         ((0.65 * shield_health/55) + 0.35) * attr->shield_size / 30 into
         fp->joints[nFTPartsJointYRotN]->scale (ftcommonguard1.c:125), which for
         Fox (shield_size 280) at full health is 280/30 = 9.3333 -- exactly the
         row-0 length the probe reads as r0=0,0,-38229 in 20.12. Without it the
         billboard scales by 1.0: cycle 56 drew the quad correctly positioned and
         camera-facing at 4.6 GUEST PIXELS across, a 101-pixel delta on the whole
         viewport. The port now captures gGCScaleX in
         ndsRendererAdapterBuildJointAttachMtx and resets it per prep, mirroring
         gcDrawDObjTreeDLLinksForGObj's own reset.
     ENGAGEMENT: gNdsRendererAdapterMvpRecalcPerspScaCount steps 0->1->2, once
     per shield submit, reject=0 mismatch=0; the prep mask moves 0x7d -> 0x5d
     because the rewrite hands over the completed MVP and NULLs the projection.
     artifacts/verification/2026-08-04_c56-shield-recalc44.txt.
EVIDENCE, single arm, EXACT_LOCK 1988,1986, inside the 1994-1982 guard window:
the quad is now a large camera-facing square CENTRED ON THE GUARDING FIGHTER and
comfortably enclosing him, about 45x38 guest pixels against Fox's ~30 -- 4,965
viewport pixels (2.07%) against the cycle-56 arm. Position, scale, attachment
and orientation are all correct by eye for the first time.
artifacts/visibility/2026-08-04_c5{5,6,7}-shield-guarding-t1988-flag1-a.png plus
the -zoom.png crops; the earlier flat white smear on the platform is unchanged
across all three arms and is therefore NOT the shield.
  5. COLOUR: THE SEAM IS THE gDP MACRO, NOT THE HEAD STREAM (2026-08-04, cycle
     58). The suspicion that the port never executes gSYTaskmanDLHeads[1] was
     right about the symptom and wrong about where it starts. It never gets that
     far: gDPSetPrimColor and gDPSetEnvColor were NDS_GBI_ZERO_PACKET stubs
     (include/PR/gbi.h), so a source proc_display's colour was discarded at the
     MACRO and the head held two zero words -- opcode 0x00, ignored by anything
     that reads it. Every source effect therefore drew in whatever prim/env the
     PREVIOUS list left in the renderer's persistent RDP state, which for an
     effect submitted after the stage is the stage's dark green. Nothing about
     it was effect-specific, which is why it belongs here and not in one row.
     The two macros now carry their ordinary F3DEX2 words, and the port reads
     the span back where source writes it: ndsRendererAdapterMarkDisplayProcHeads
     runs in the captured-display hook, immediately before
     `current_gobj->proc_display(current_gobj)`
     (opening_movie_backend.c:4387), and ndsRendererAdapterCaptureDisplayProcColors
     scans the span the proc appended and hands prim/env to the effect submit,
     which seeds render_stats BEFORE the model list runs -- so the list's own
     colour commands still win, exactly as they do on the RSP. General, not
     per-effect: the rebirth halo, the impact wave and Yoshi's shield all express
     colour the same way. Every other gDP macro still zeroes; only these two were
     changed, and a zero packet was already being ignored, so nothing that
     previously worked can start failing.
EVIDENCE, single arm, EXACT_LOCK 1988,1986, inside the 1994-1982 guard window.
Cycle 57 put the bubble on the fighter: a large camera-facing square CENTRED ON
THE GUARDING FIGHTER and comfortably enclosing him, about 45x38 guest pixels
against Fox's ~30, 4,965 viewport pixels (2.07%) against cycle 56. Cycle 58 then
coloured it: bright GREEN at alpha 0xC0, which is dEFManagerShieldColors[1] --
Fox's player index -- and the fighter is visibly through it, another 5,018
pixels (2.09%). Position, scale, attachment, orientation, per-player colour and
translucency are all correct by eye.
artifacts/visibility/2026-08-04_c5{5,6,7,8}-shield-guarding-t1988-flag1-a.png
plus the -zoom.png crops; the flat white smear on the platform is unchanged
across all four arms and is therefore NOT the shield.
  6. SHAPE: THE TEXTURE WAS REFUSED BY AN sm64-nds COMBINE RULE (2026-08-04,
     cycle 59), and this was the last dimension. ready=0 AND reject=0 said no
     bind was ever ATTEMPTED, which is a gate above the texture code, and it is
     ndsRendererHardwareUseDecal: `b0 == d0` forces POLY_DECAL and, when
     `a0 == PRIMITIVE`, also sets use_texture = false. That is sm64-nds's
     g_setcombine rule transcribed whole (4e0ae66703). SSB64's shield DL is
     G_CC_BLENDPE -- (PRIMITIVE - ENV) * TEXEL0 + ENV, alpha TEXEL0 * PRIM --
     which trips both halves, so the port threw away the 16x32 IA8 circular
     alpha at 163_FTManagerCommon.c:18 and drew the flat env colour: a green
     SQUARE. `(a - b) * TEXEL0 + b` is a LERP whose weight is the texture, not a
     decal, so the rule now requires the multiplier NOT to be TEXEL0/TEXEL1.
     THE SAME LINE OWNED THE HALO AND THE WAVE. The two affected word pairs
     (0xFC309661/0x552EFF7F, 0xFC30FE61/0x55FEF379) are SSB64's standard
     translucent-effect combine; in the P1 battle only FTManagerCommon,
     EFCommonEffects1/2/3, FoxSpecial3 and ITCommonObject carry one. Dream Land,
     MarioModel/Main and FoxModel/Main carry none, and the flag-0 stand-ins emit
     no G_SETCOMBINE at all, so the shipping default's exposure is Fox's ~2 s
     entry Arwing plus items, which P1 has off. Corpus census (2,132 files,
     3,210 b0==d0 combines, 284 changed):
     artifacts/verification/2026-08-04_c59-combine-decal-census.txt.
EVIDENCE, PAIRED, IDENTICAL RUN. probe-effect-texture.ps1 on both arms breaks at
efManagerShieldProcDisplay and reads the published counters; the two runs hit the
SAME tics with the SAME draw and triangle counts, so this is one variable:
  tic   1994 1992 1990 1988 1986 1984 1950 ... 1916
  c58   texready 5070 5070 5070 5070 5070 5070 5070 ... 5070   (frozen)
  c59   texready 5160 5161 5162 5163 5164 5165 5166 ... 5176   (+1 per draw)
  both  link15 0..16, tris 11127..11159 (+2 = one quad per draw), texreject 0
Every shield submit now admits exactly one texture and rejects none, and the c59
arm is already 90 admits ahead at tic 1994 -- effects that spawned earlier, i.e.
the fix transfers off the shield on its own.
artifacts/verification/2026-08-04_c5{8,9}-effect-texture*.txt.
BY EYE, EXACT_LOCK 1988,1986 inside the 1994-1982 guard: the square is now a
ROUND translucent bubble, bright at the centre and deep green at the rim --
which is what (PRIM - ENV) * TEXEL0 + ENV looks like with white prim, Fox's
green env and the texture's radial intensity -- with the canopy visible through
it. artifacts/visibility/2026-08-04_c59-shield-guarding-t1988-flag1-a.png and
-zoom.png against the c58 pair.
The pixel metric confirms only that it cannot gate this: on the shield crop the
same-build adjacent-present floor is 43.1% (c58) and 45.5% (c59) against a 26.9%
cross-build delta, exactly as item 4 below predicts.
GATE 6: the shield row is OWNER-QUEUED. Prediction, one sentence: while Fox
guards you will see a round translucent green bubble centred on him, big enough
to enclose him, with the stage visible through it, and it disappears the instant
he stops guarding.

TRANSFER CENSUS ON THE cycle-59 ARM (2026-08-04), because the combine fix is
scene-wide and every appearance conclusion older than it is stale. Both arms
captured at the same EXACT_LOCK, so each is a paired before/after:
  * impact wave, tic 3272 (spawn 3277). CHANGED, and the old picture was
    clearly wrong: c58 draws a hard-edged solid BLACK rectangle over the right
    flower bed and over Fox standing in it; on c59 that rectangle is gone and
    the region is textured foliage with the effect's own ragged shape. Crop
    delta 19.6% against a same-build floor of 95.6-98.8% -- the number is
    meaningless here, the rectangle's disappearance is not. Still an owner call
    on whether what replaced it is the right asset.
    artifacts/visibility/2026-08-04_c5{8,9}-wave-t3272-flag1-a.png.
  * rebirth halo, tic 2814 (spawn 2819). NO CHANGE, and nothing halo-shaped is
    on screen in EITHER arm: whole-viewport cross-build delta 395 pixels
    (0.16%) against a 42.9% same-build floor. So the halo's remaining problem is
    NOT texture admission -- it is upstream, at "does it draw where the camera
    is at all", and the next cycle should spend its budget on the existence
    chain rather than re-capturing this tic.
    artifacts/visibility/2026-08-04_c5{8,9}-halo-t2814-flag1-a.png.
  * Fox reflector: unchanged and unreachable here; it still never spawns.
OWED AT THE PUBLISH POINT: the combine fix is in src/nds/nds_renderer.c and is
NOT flag-gated, so it ships. Its flag-0 exposure is censused above (Fox's entry
Arwing and items) but has not been run; Boundary at the tracked default is owed
before the next publish, as its own deliberate step.
ALSO OWED, AND NOT MEASURED: the fix adds real active-frame work -- effects that
drew untextured now bind and upload a texture. The capture HUD's running FPS
readout differs on frames carrying one (shield 29.1 -> 27.9, wave 28.3 -> 27.0,
halo 29.9 -> 29.9), which is suggestive, not a measurement: it is a host-side
average on two arms that are not paced-matched. A paired A/B is owed if the flag
is flipped, and the flag-0 arm's exposure is two seconds of Arwing. Do not read
those three numbers as a priced regression.

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
    STAGE: LOCALIZED -- not a texture or colour seam; nothing halo-shaped is on screen at its own spawn tic.

-Fox down B VFX is not correct or using correct asset.
    Owner: you are still not using the correct asset for Fox's down B reflector.
    CAUSE: dEFManagerFoxReflectorEffectDesc is an animated model from gFTDataFoxSpecial2, not a sprite.
    STAGE: LOCALIZED. Retry counter is compiled out at flag 0 and read 0 at flag 1; needs a flag-1 down-B.

-Shield VFX not correct
    Owner: texture looks cut in half: `artifacts/visibility/2026-08-03_owner_shield-cut-in-half.png`
    CAUSE: dEFManagerShieldEffectDesc is a model; 'cut in half' is a 1:2 source cell on a square quad.
    STAGE: OWNER-QUEUED -- round translucent per-player bubble on the guarding fighter; needs the owner's eye.

-Hard landing vfx not not using correct asset.
    Owner: incorrect asset for the impact wave is being used
    CAUSE: dEFManagerImpactWaveEffectDesc is a material-animated model, so no single atlas texture can
    be its 'correct asset'. The row also has a particle half (dust, script 0x58, efmanager.c:2982).
    STAGE: MEASURED -- the black rectangle over the flower bed is gone now that its texture is admitted.
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
