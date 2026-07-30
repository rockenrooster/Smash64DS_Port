AI Agent should mark fixed items with FIXED prefix

-Sometimes hitting a shielded player causes a freeze.
  Not a shield bug. The shield hit was one trigger of a general out-of-memory
  hang, which is also the owner's "lots of freeze bugs that seem random".
  Mechanism: `decomp/src/sys/malloc.c:30` answers a full arena with
  `while (TRUE);`. On the N64 that was a developer assert beside a devkit
  printf; here it is shipped code with nowhere to print, so heap exhaustion
  presents as a total, silent, permanent hang -- interrupts still enabled and
  still serviced, VBlanks still counting, the main loop simply never returning.
  Confirmed by disassembly: the PC sits on `e7fe  b.n <self>`.
  Ruled out by register reads rather than by argument: REG_IME=1, REG_IE has
  VBlank, REG_IF=0 and the CPSR I-bit clear (not the interrupts-disabled
  swiWaitForVBlank hang); GXSTAT not FIFO-stalled (not a geometry deadlock);
  FGM enter count == return count (not the audio/IPC handshake).
  Two instances captured 2026-07-29, both on gSYTaskmanGeneralHeap:
  - Mid-match, and this is the one the owner hit. `ndsR2AnimCacheStore`
    (reloc_backend_assets.c) asked syTaskmanMalloc for 3,472 bytes at 3.5
    minutes of both-CPU play, from Mario's AttackAirD load via
    ftMainSetStatus <- AttackAir interrupt <- DamageFall. A shield hit drives
    rebound/damage-fall, which interrupts into a new status, which force-loads
    an animation -- shield hits are simply a common way to reach an uncached
    status, which is why it looked shield-specific and why it was "sometimes".
    The cache bounded its ENTRY COUNT and never its BYTES, and never freed.
    FIXED: the cache now owns a fixed 128 KiB static arena and bump-allocates
    from it, so its exhaustion returns NULL through the `payload == NULL`
    reject path that was already written and had been dead code, and the asset
    takes the on-demand path it would have taken anyway.
  - At battle start, `ftManagerSetupFilesMainKind(fkind=1)` loading Fox's
    files: request 116,752 against 57,936 free in a 1,048,576-byte arena,
    short by 58,816. Seen with `NDS_R2_ANIM_CACHE=0`, so it is independent of
    the cache. OPEN, and specific to the `NDS_R2_BOTH_CPU=1` stress config so
    far; the shipped ROM measured 1,286,144 bytes of arena and clears the same
    load with ~178 KB spare.
  Class-level, and it applies to every arena in the port: an overflow can no
  longer be anonymous. `src/import/battleship_sys_malloc.c` wraps syMallocSet,
  latches arena id / request / alignment / headroom / caller LR into
  `gNdsSyMallocOverflow*`, and halts in the named `ndsSyMallocOverflowHalt`.
  It still halts by design -- syTaskmanMalloc's decomp callers do not check for
  NULL, so returning NULL globally would trade a hang for a wild write. An
  optional allocation must ask `ndsSyMallocWouldFit` first. Most call sites
  still commit blind; see docs/optimization/TASK_STANDING_RULES.md.

-Wind hazard not working, (SFX, VFX, gameplay effects)  [gameplay+SFX FIXED]
  Gameplay FIXED: ftParamSetVelPush was a counter-only stub that dropped the
  push vector on the floor, so Whispy's gust had no effect at all. It now does
  the source assignment (ftparam.c:526-531); the consumer chain was already
  live (mpprocess.c:450 adds coll_data.vel_push to the translation,
  ftmain.c:1571 clears it each frame).
  SFX FIXED: nSYAudioFGMPupupuWhispyWind (285) was never packed. Added, and
  then it still puffed once and stopped -- the sample is 0.88 s but the blow
  is 470 ticks x 5750 us = 2.70 s, so two thirds of the gust was silence.
  (An earlier note here said 7.8 s; that used the wrong tick rate.)
  Root cause, and it was not the runtime:
  - The DS side was already complete. ndsAudioFgmValidateCachedEntry accepts
    flags & ~1u == 0, so a looping entry already passed validation, and the
    play path already passed the loop bit and loop point through to
    soundPlaySample (nds_audio_fgm.c:1158-1162), with the duration clock
    releasing the handle at 470 ticks.
  - The generator never emitted one. All four branches in
    render-audio-fgm-phase-pack.py hardcoded `flags = 0` and
    `loop_point_words = 0`. The `"loop": True, loop_start: 48,
    loop_end: 13348` on cue 285's selector was descriptive source metadata
    that nothing acted on. My earlier note claiming it "ships with the loop
    live" was wrong.
  Fixed as a DS hardware loop, not an AOT render. The DS ADPCM channel
  latches predictor/index when it reaches PNT and restores them on every
  repeat, so putting PNT at the first word after the IMA header makes the
  latched state the header state and every cycle decodes bit-identically by
  construction. Cost is zero extra bytes -- the pack got 28 B smaller.
  AOT was the other option and would have fit (~20 KB for 2.70 s), but it
  buys nothing over a loop the hardware does for free, and 626 only renders
  AOT because its loop feeds a volume ramp that a hardware repeat cannot.
  Shape: body = pcm[48:13352], 13304 samples = 1663 words after a 1-word
  PNT; the 4 samples past loop_end are the source's own tail, taken as
  alignment debt so the seam is real audio rather than synthetic guard
  nibbles. 6656 B, SNR 34.6 dB, ~3.07 cycles per blow.
  The unused ima_encode_loop_body / ima_ds_repeat_cycles / ima_repeat_oracle
  machinery is now live and proves it: three restored cycles hash identically,
  and all three negative controls fire (carried decoder state diverges, PNT
  at the header rejected, LEN counting the whole buffer rejected).
  check-audio-fgm-phase-pack.ps1 pins the flag, the PNT/LEN geometry and the
  three proofs so the loop cannot be dropped again silently.
  Latest profile green. Needs an ear check.
  VFX still open: the same particle-bank gap as the other VFX rows below --
  lbParticleMakeScriptID is a skipped stub.
  
-some VFX are wrong/don't look right, running foot dust VFX, fireball hit VFX, fox down B, hard landing vfx
  Root cause, measured: every named effect does spawn -- the verifier reports
  178/178 Mario/Fox motion calls with bounded DS presentation -- but the
  original particle scripts never run. lbParticleMakeScriptID is a stub
  (reloc_backend_compat_shims.c:12963) and the common particle script/texture
  banks are not resident, so nothing here is textured. In their place,
  13 NDSVisualEffectKind values (nds_effects.h:9-25) share a small set of
  template slots (battleship_efmanager.c) built from only FOUR untextured
  16-vertex primitives: BuildDust, BuildStar, BuildRing, BuildDisc.
  This splits the row in two:
  (a) A real defect -- before this pass, five kinds rendered as a *different
      effect*: Coin->Sparkle, Catch->ImpactWave, Slash->HitNormal,
      Rebirth->Death, and Reflector->Shield. "fox down B" is
      that last one: efManagerFoxReflectorMakeEffect asks for
      nNDSVisualEffectReflector and gets the shield disc, white->red -- P1
      Mario's shield colors on Fox's reflector. The port also never reads
      effect_vars.reflector.status, which ftfoxspeciallw.c:29 sets every frame,
      so the reflector cannot react to its own state either.
      FIXED for the two collapses a Mario-vs-Fox match actually shows:
      Reflector and Rebirth now have their own template slots. Fox's down B
      draws a blue barrier disc instead of Mario's red shield. The hues reuse
      pairs already in the file, so this is an approximation and the owner's
      eye is the gate. Coin/Catch/Slash still share slots -- coins need items
      (off for P1) and neither of the other two was reported, so they stay
      collapsed rather than getting invented colors.
  (b) Running foot dust, fireball hit, and hard landing each DO have their own
      template (Dust, Fire, Dust). They look wrong because a recolored
      16-vertex primitive is standing in for a textured particle script.
      This is P1, not P2 -- the contract is that this exact match is identical
      to the full game with these settings, so the real scripts have to run.
      Sized 2026-07-27 and it fits: efmanager reaches 26 of 119 efcommon
      scripts naming 18 of 47 textures = 129,768 B, plus 4,896 B for Dream
      Land's grpupupu bank, against 210,320 B measured arena headroom. The
      banks are position-independent so no relocData tooling is needed.
      OPEN: port lb/lbparticle.c (2,961 lines), ef/efparticle.c,
      ef/efdisplay.c, add a DS pack step, and draw textured quads.
      See KNOWN_ISSUES.md for the full measurement.

-Upwards KO boundary:  VFX and SFX never play for fighters
  SFX half FIXED: the star-KO path asks for nSYAudioFGMDeadUpStar (12) and the
  per-fighter deadup_sfx (Fox 360 / Mario 433); none of the three was packed.
  All three added.
  VFX half: same root cause as the VFX row above. efManagerSparkleWhiteDead
  spawns nNDSVisualEffectSparkle at scale 5.0 and efManagerDeadExplode spawns
  nNDSVisualEffectDeath, so something does draw -- a white star and a red ring.
  The original star-KO is a textured particle script that is not resident.
-KO VFX wrong.
  Partly FIXED. nNDSVisualEffectDeath and nNDSVisualEffectRebirth shared one
  template, a red->white ring, so the KO burst and the respawn flash were the
  same effect. Rebirth now has its own white/cyan ring and Death keeps the red
  one. What remains is the untextured-primitive gap: the original KO burst is a
  particle script, and a ring is the stand-in. That half is P2.