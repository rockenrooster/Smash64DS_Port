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

OPEN.
  * BLOCKS GATE 5: the flag-0 tickhud arm HANGS between frame 175 and 200.
    Not slowness -- timed budgets 5f/21s 35f/23s 100f/25s 150f/26s 177f/27s,
    then 200f never in 201s, and the flag-1 arm matches those times exactly, so
    both arms run at the same speed until the control stops dead. A regression:
    a pre-campaign flag-0 tickhud RingDump completed
    (artifacts/performance/2026-08-03_dobjtree_control_ringdump.json), so the
    range is (4d1015b75 .. bca626a758].
    THREE OF FOUR SUSPECTS REFUTED BY COUNTERS, no builds spent. At frame 175,
    the last healthy frame: runaway=0 mask=0 panic=0 (ae7c3e735's bounded
    parsers and objman never fire) and resolve=4 disabled=0 unknownfile=0
    (d4c7d3d7b's validation never disables anything at flag 0, because only
    dEFManagerDeadExplodeEffectDesc is resolved there and its file is in the
    span table). 8508fc8d6 was flag-gated in cycle 15 and the hang persisted.
    BISECTED TO ae7c3e735, THE FREEZE FIX (cycle 24). Budget-200 flag-0 tickhud
    builds: 4d1015b75 reaches 200 in 28s; ae7c3e735 hangs; d4c7d3d7b hangs;
    current hangs. The freeze fix is the first bad commit.
    ITS RUNAWAY COUNTERS ARE NOT THE MECHANISM -- they read 0 at frame 175, and
    that refutation stands. But the commit's ONLY runtime change outside the
    bounded parsers is src/port/reloc_backend_assets.c (+48): the misalignment
    rejection that makes ndsRelocResolvePointerFromFileBase return NULL for a
    result that is not 4-aligned. That is fail-closed behaviour on a resolver,
    it was never covered by the counters checked so far, and it is the prime
    suspect.
    THE RESOLVER IS EXONERATED TOO (cycle 25), on the right build this time:
    RESOLVE@175 misalign=0 misalignval=0x0 offsetcalls=0. The alignment
    rejection never fires and the offset fallback is never even entered, so it
    must NOT be deleted -- it is inert here and still guards the 2-mod-4 script
    pointer that caused the original freeze.
    SO EVERY LOGIC PATH IN ae7c3e735 IS CLEAN AND BISECT STILL NAMES IT. What is
    left in that commit is not behaviour: +7 BSS symbols (8391 -> 8398 across
    the two builds; 4 runaway + 3 resolver counters) and the parser code-size
    change. LEADING HYPOTHESIS is therefore LAYOUT, not logic -- ae7c3e735 is
    the trigger, and the real defect is latent and older. That fits everything
    seen: a reproducible frame, no counter movement, and a ROM whose pacing is
    already documented as cache-placement sensitive.
    BSS LAYOUT REFUTED (cycle 26): a clean 4d1015b75 plus 28 bytes of live BSS
    padding reaches 200 frames in 28s. Data-layout growth alone does not
    reproduce the hang.
    PARSER REVERT ON HEAD STILL HANGS (cycle 27): reverse-applying both decomp
    patches on the current tree and probing at 200 stalls exactly as before. So
    the parsers are not the SOLE cause of today's hang.
    That does NOT exonerate the range, and the inference matters: this build
    still carries d4c7d3d7b, 8508fc8d6 and 4c29b9615a, so an independent later
    cause would produce the same result. Bisect already showed ae7c3e735 ALONE
    hangs, so within that commit the remaining candidate is the resolver hunk --
    despite its counters reading 0 -- or its .text displacement.
    DECISIVE NEXT TEST: revert the parsers AT ae7c3e735, not at HEAD. That is
    the only build that separates that commit's parsers from its resolver. Pass
    means the parsers; hang means the resolver (and its zero counters then mean
    the mechanism is placement, not the rejection itself).
    HARNESS TRAP WORTH REMEMBERING: unreferenced BSS is collected by
    --gc-sections, so the first discriminant build was byte-identical to the
    anchor and would have "passed" meaninglessly. Any padding probe must be
    referenced from live code (cliff_ledge.c's reset is the repo's own idiom)
    and the symbol verified present with nm before the result is believed.
    BUILDING AN OLD COMMIT NEEDS THE DECOMP PATCHES REVERSED TOO: decomp/ is
    gitignored, so a checkout leaves the tracked patches applied and the build
    fails on undeclared runaway counters. Reverse-apply
    scripts/decomp-patches/battleship/*.patch before building any pre-ae7c3e735
    commit, and re-apply them on return.
    HARNESS LIMIT: once hung, a second GDB cannot attach; capturing the hang PC
    needs an interrupt on an already-attached session, not a re-attach.
  * One unpaired flag-on reading suggests the cost is high (FPS 20.0, ALL
    1.68M/2.24M against the 1.12M gate). Warning, not a verdict -- gate 6 must
    not be proposed until gate 5 prices it. See 4c29b9615a.
  * Reflector desc is disabled at startup because gFTDataFoxSpecial2 loads
    after efManagerInitEffects. Deferred-retry fix written but UNVERIFIED --
    proof is gNdsEFDescDeferRecoverCount>0, needs a Fox down-B. See 48fe59693c.
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
    STAGE: draws flag-on (tree fix, 8508fc8d6). Awaiting a respawn capture. Flag still 0.

-Fox down B VFX is not correct or using correct asset.
    Owner: you are still not using the correct asset for Fox's down B reflector.
    CAUSE: dEFManagerFoxReflectorEffectDesc is an animated model from gFTDataFoxSpecial2, not a sprite.
    STAGE: desc disabled at startup -- its file loads after effect init. Deferred-retry fix written but
    UNVERIFIED (needs a down-B; proof is gNdsEFDescDeferRecoverCount>0). See 48fe59693c.

-Shield VFX not correct
    Owner: texture looks cut in half: `artifacts/visibility/2026-08-03_owner_shield-cut-in-half.png`
    CAUSE: dEFManagerShieldEffectDesc is a model; 'cut in half' is a 1:2 source cell on a square quad.
    STAGE: builds and draws flag-on (8508fc8d6). The capture arms on link 15, but link 15 also carries
    the reflector, so 2026-08-03_shield-vfx.png does not yet prove which drew. Flag still 0.

-Hard landing vfx not not using correct asset.
    Owner: incorrect asset for the impact wave is being used
    CAUSE: dEFManagerImpactWaveEffectDesc is a material-animated model, so no single atlas texture can
    be its 'correct asset'. The row also has a particle half (dust, script 0x58, efmanager.c:2982).
    STAGE: built correctly as a single DObj on link 10 -- its EFDesc omits flag 0x4, so a raw display
    list is right. Executes fully, but its triangles do not reach the effect counter. See 4c29b9615a.

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
