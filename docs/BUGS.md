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
  * BLOCKS GATE 5: the flag-0 tickhud arm HANGS between frame 175 and 200, so
    there is no control arm to measure against. Not slowness -- both arms are
    the same speed up to 175 (5f/21s .. 177f/27s) and the flag-0 one then stops
    dead. Regression against a pre-campaign RingDump that completed
    (artifacts/performance/2026-08-03_dobjtree_control_ringdump.json).
    MECHANISM: placement, not logic. Three arms at one commit --
      4d1015b75 anchor            200f/28s
      ae7c3e735 with parsers      hangs
      ae7c3e735 parsers reverted  200f/28s
    The freeze-fix parser patches add 312 bytes to .main (0xca050 -> 0xca188),
    shifting .main.rw from 0x020cd728 to 0x020cd860. They are the TRIGGER; the
    defect is whatever those bytes displace, is older than this campaign, and
    must be fixed at its own seam -- never by re-padding .main, and the parsers
    stay bounded (the freeze row depends on them).
    Refuted, do not re-run: runaway/panic counters 0 (21); desc validation
    disabled=0 (22); resolver misalign=0, offsetcalls=0 (25); 28-byte BSS pad
    does not reproduce (26); parser revert at HEAD does not fix, which
    exonerates nothing since later commits remain (27); shift is non-uniform
    with no symbol crossing a region edge, and stale absolute addresses are
    noise at 10 hits vs ~8 expected by chance (29).
    CAPTURED (31): there is no spinning PC. Four stops spanning 36s all read
    PC armWaitForIrq+4 (0x1fff390, nm-confirmed 8-byte fn), bt armWaitForIrq
    <- armContextLoad, cpsr 0x1f, while presented stayed frozen at 195 --
    inside the 175-200 window. The ARM9 is IDLE in calico's wait-for-IRQ, so
    this is a blocked wait / lost wakeup, not a runaway loop, which is why
    every counter-based probe came back clean. Attachment does not perturb
    the repro. OPEN: name the blocked waiter and the event it misses.
    artifacts/verification/2026-08-03_flag0-hang-pc.txt.
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
