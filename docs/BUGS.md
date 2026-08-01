AI Agent should mark fixed items with FIXED prefix.
These bugs should be fixed for P1 delivery.

-FIXED (2026-08-01, needs an ear check) SFX for "GAME SET" sounds really low
  pitched. Thirteen semitones low: the pack plays one sample at one rate, and
  the generator took `notes[0]` for that rate. Pitch code 0 is a REST, and FGM
  488 is the only P1 cue whose program opens with one -- `((0,7,60),(13,7,150))`,
  a 60-tick rest then the line -- so it rendered at 7,565 Hz where every other
  announcer line renders at 16,000. Now takes the first SOUNDING note; the
  sample count is unchanged (the trim was already bounded by the source PCM), so
  only the frequency field moves. Nothing caught it because the pack self-checks
  against its own derivation and the derivation had the same bug, so the new
  guard is external: `check-audio-fgm-phase-pack.ps1` rejects any entry under
  12,000 Hz, between the 7,565 a rest produces and the 15,102 lowest real one.
  Left alone deliberately: the rest still counts toward `duration_ticks`, so the
  line starts one second earlier than the source. Timing, not pitch, unreported.
-Still get intermittent freezes when attacking (maybe collision/animation/heap
  related?). Owner evidence:
  `artifacts/visibility/2026-07-31_attack-freeze-owner-302caae.png` -- build
  `302caae`, TIME 00:37, both fighters 12%/1 stock, Mario inside a shield
  bubble. The HUD is the useful part: `ALL 1119872` current against `1680320`
  max, `WORK 1034496 / 1497856` over n:128, VBlank histogram `2:690 3:163 4:33
  5+:4` with `max:19`. So the stall is four 5+-VBlank frames and one 19-VBlank
  frame in a 128-frame window -- an event, not a slow body.
-Sometimes Mario's fireballs don't spawn.

-Results screen. VFX and SFX/BGM/FGM.  [ALL FOUR FGM cues PACKED 2026-08-01;
  VFX remains]
  **534 WinnerIs, 499 Mario and 486 Fox** were packed 2026-07-31 -- derived with
  `render-audio-fgm-phase-pack.py --derive 534,499,486` and proven by their
  disappearance from the natural-match miss ring (see the TIME UP row).
  **621 PublicWin is now packed too** (2026-08-01), and the machinery it was
  said to need turned out to be one line. It is the second cue on 626's wave
  (both articulation 460 / sound 320 / wave 2966600+15876, both looping
  1..28215), differing only in note (9 vs 12), duration (950 vs 1200 ticks) and
  UCD volume (190 vs 223) -- so it takes 626's AOT loop-then-quadratic-ramp
  render unchanged. FGM 285's hardware-repeat strategy would NOT have worked for
  either: articulation 460 ramps volume across the loop, and a hardware repeat
  reproduces every cycle bit-identically by construction, which is exactly why
  it cannot ramp. The only 626-shaped thing in the renderer was a pinned sample
  count, now derived from the note's own duration (`looped_fanfare_sample_count`,
  which reproduces 626's 104,204 exactly and gives 621 69,369) and still pinned.
  Pack 535,280 -> 570,000 B, 63 -> 64 entries, cap 786,432.
  One real defect fell out: the `missing_preroll` negative control was unclamped
  while the render clamps, so 621's lower hardware gain overflowed int16 and
  crashed the generator. Clamped now -- it was also a WEAKER control unclamped,
  differing from the render in two ways instead of one. **Needs an ear check.**
  Research (2026-07-30, Sol Max match-end/audio):
  - Source contract: the Results sequence queues PublicWin 621 at scene start,
    WinnerIs 534 at tick 81, Mario 499 or Fox 486 at tick 210, and
    PublicExcited 626 at tick 270. It starts the winner fanfare, transitions to
    looping Results BGM, and creates two confetti instances from particle
    script 112 at tick 120
    (`decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsresults.c`).
  - Current split: the core Results scene and Mario/Fox fanfare plus Results
    loop entries already exist (`src/nds/nds_audio_bgm.c:41-85`). FGM 626 is
    packed, but 621/534/499/486 are absent from
    `src/nds/nds_audio_fgm.c:182-252`. Confetti cannot appear while
    `lbParticleMakeScriptID` returns NULL
    (`src/port/reloc_backend_compat_shims.c:12963-12972`).
  - Preserved WIP commits
    `17e6cd0b301fe77402f352ec0c96cd6fc6ce19b7` (generator/checker) and
    `ef5d25268678070a6f1dd452ecbb0e1ad9194519` (runtime) are useful review
    inputs, not fixes: neither has been built or runtime-qualified, and the
    generator does not include Results script 112.
  - Proposed fix: complete the shared particle work with script 112 and its
    exact texture closure; add only source cues 621/534/499/486 to the existing
    FGM path and retain 626. Leave the working BGM seam alone unless a measured
    natural Results run exposes a BGM defect.
  - Required proof: natural match end with source-order cue/channel evidence,
    both tick-120 confetti instances, fanfare-to-loop transition, screenshots,
    owner visual/listen approval, and pacing after the missing content is live.
    Status: OPEN.
-Crowd noise is missing from results and match gameplay.
  **All twelve reachable crowd cues now DERIVE cleanly** (2026-07-31,
  `render-audio-fgm-phase-pack.py --derive 605,609,615..625`), which removes the
  extraction unknown from this row and leaves two real obstacles. **(1) Eight of
  the twelve are LOOPED or FORKED**: 605/609/615 and 621/624 are loops on
  mid-wave loop points, and 616/618/619/620/623 carry fork voices (650, 627, 676,
  684, 625). 96 `GroundGrind2` -- which a live match already requests six times
  per minute -- has no `pitch` op at all, so `validate_articulation` rejects it as
  written. **(2) The trigger side is still absent** and that is the larger half:
  see the root cause below. Packing a cue nothing requests would be dead ROM, so
  the actor comes first.
  Research (2026-07-30, Sol Max match-end/audio):
  - Source contract: `decomp/BattleShip-main/decomp/src/ft/ftpublic.c` owns the
    crowd actor, thresholds, cooldowns, repeat limits, and event queue. The
    reachable Mario/Fox set includes chants 609/605, reactions 615-625, Results
    win cue 621, and PublicExcited 626.
  - Root cause: only the isolated 626 path is currently packed. Reactive crowd
    behavior is structurally absent: `ftPublicCommonCheck` is diagnostic-only
    (`src/port/reloc_backend_compat_shims.c:6530-6548`),
    `ftPublicMakeActor` only marks bits (`:12768-12772`), the battle queue is a
    no-op (`src/port/battle_playable_compat_stubs.c:216-219`), and the original
    `ftpublic.c` is not in the forced source list (`Makefile:1150-1163`).
  - Proposed fix: import the source actor/state machine, or the smallest
    mechanically equivalent DS translation, at that shared seam. Preserve its
    thresholds/cooldowns/repeats/queue ordering and add only reachable source
    cues 605, 609, 615-625, and 621 to the existing FGM backend; retain 626.
  - Required proof: natural gameplay reactions and Results win events with
    event-to-ID-to-ARM7/channel traces, source timing/queue guards, and owner
    listen approval. A marker or packed cue alone is not completion. Status:
    OPEN.

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
  Research (2026-07-30, Sol Max wind/particles):
  - Source contract: Whispy uses Dream Land's separate Pupupu particle bank,
    with script 0 for leaves and script 1 for dust
    (`decomp/BattleShip-main/decomp/src/gr/grcommon/grpupupu.c`,
    `particles/grpupupu_scb.c`, and `particles/grpupupu_txb.c`). Its source
    bank is 4,896 bytes; that is source representation size, not a promised
    final DS residency cost.
  - Remaining root cause: the particle constructor above is skipped and the
    draw seam counts particles without drawing textured quads. The two
    preserved WIP commits are incomplete together:
    `17e6cd0b301fe77402f352ec0c96cd6fc6ce19b7` generates only the common EF
    bank, while `ef5d25268678070a6f1dd452ecbb0e1ad9194519` registers every
    non-common bank, including Pupupu, as empty and has no live draw
    (`src/import/battleship_lbparticle.c:507-560,669-677`).
  - Proposed fix: reconcile those WIPs into explicit common-plus-Pupupu pack
    contracts, pin script/texture closure and bytes in one checker, complete
    only the required interpreter operations, and submit one DS textured quad
    through the existing renderer texture fencing. Measure RAM, VRAM, pool
    high-water, and drops; keep the current fallback until natural proof.
  - Required proof: a natural Whispy cycle showing source leaves/dust alongside
    the already-live push and cue 285, zero unexplained particle drops,
    synchronized screenshots plus owner visual/ear approval, and an eight-frame
    A/B with the 2/3/4/5+ VBlank histogram and max interval. Status: gameplay
    FIXED; SFX implementation landed but acoustic acceptance is OPEN; VFX OPEN.

-Correct VFX isn't played for various things (running foot dust VFX, fireball hit VFX, fox down B, shield, hard landing vfx, etc)
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
  Research (2026-07-30, Sol Max wind/particles):
  - Exact common-particle targets are run/expiry dust scripts 0x55/0x56, hard
    landing/double scripts 0x58/0x59, fireball bounce 0x0B, fire damage 0x4D,
    and fireball-hit sparkle 0x73
    (`decomp/BattleShip-main/decomp/src/ef/efmanager.c` and
    `wp/wpmario/wpmariofireball.c`). The preserved generator's reachable list
    currently omits 0x58/0x59, so it cannot fix hard landing as-is.
  - Fox Reflector is not part of that particle bank. It is an animated DObj
    from Fox's special2 assets with four status animations
    (`decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxspeciallw.c`);
    `src/import/battleship_efmanager.c:901-905` currently maps it to a generic
    disc. Treating Reflector as another particle script would fix the wrong
    seam.
  - Proposed fix: finish one shared, bounded particle interpreter/pack and one
    textured-quad draw path for the exact reachable scripts above. Restore
    Reflector separately through the existing fighter DObj/model-animation
    path. Retire each primitive fallback only after its natural replacement is
    verifier- and owner-approved; do not add a second effect architecture.
  - Required proof: natural run/expiry dust, both landing variants, distinct
    fireball bounce/damage/hit events, and all four Reflector statuses, with
    exact-ROM screenshots, owner visual approval, pool/drop guards, and
    synchronized eight-frame pacing evidence. Status: OPEN except for the
    explicitly documented primitive-category separations.

-Upwards KO boundary:  VFX and SFX never play for fighters
  SFX implementation landed; natural qualification remains OPEN. The star-KO
  path asks for nSYAudioFGMDeadUpStar (12) and the per-fighter deadup_sfx (Fox
  360 / Mario 433); all three are now packed and allowlisted.
  VFX half: same root cause as the VFX row above. efManagerSparkleWhiteDead
  spawns nNDSVisualEffectSparkle at scale 5.0 and efManagerDeadExplode spawns
  nNDSVisualEffectDeath, so something does draw -- a white star and a red ring.
  The original star-KO is a textured particle script that is not resident.
  Research (2026-07-30, Sol Max Sudden Death/KO):
  - Source contract: crossing the top boundary has a 1-in-6 falling-KO branch
    and otherwise enters the 180-tick star-KO sequence. The star branch plays
    fighter voice 433/360 immediately, FGM 12 at the source point, and sparkle
    script 0x5C; the falling branch intentionally omits the star cue
    (`decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondead.c`).
  - Current weak effect code
    (`src/port/battle_playable_compat_stubs.c:130-138`) substitutes a generic
    untextured sparkle and returns NULL. That establishes a missing source
    effect seam, but the report that VFX/SFX “never play” still needs one
    natural trace to distinguish wrong branch selection, effect create/drop,
    draw visibility, and speaker delivery.
  - Proposed fix: first trace the natural branch/status, audio requests,
    particle create/drop, and draw submission. Fix the measured owner: retain
    the already-packed audio if it reaches the speaker, and restore script 0x5C
    through the shared particle work rather than adding a KO-only renderer.
  - Required proof: natural Mario and Fox star KOs plus the 1-in-6 falling
    branch, with trigger-to-speaker and trigger-to-pixel evidence and owner
    listen/visual approval. Status: OPEN.
    
-KO VFX wrong.
  Partly FIXED. nNDSVisualEffectDeath and nNDSVisualEffectRebirth shared one
  template, a red->white ring, so the KO burst and the respawn flash were the
  same effect. Rebirth now has its own white/cyan ring and Death keeps the red
  one. What remains is the untextured-primitive gap: the original KO burst is a
  particle script, and a ring is the stand-in. That remaining half is P1.
  Research (2026-07-30, Sol Max Sudden Death/KO):
  - Source contract: down/left/right deaths select player- and type-specific
    DeathExplode generators, orientation, DObj/material animation, and player
    colors (`decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondead.c` and
    `decomp/BattleShip-main/decomp/src/ef/efmanager.c`).
  - Root cause: the weak fallback
    (`src/port/battle_playable_compat_stubs.c:118-128`) discards the player
    argument and maps every type to one untextured ring, changing only scale.
    Separating Rebirth fixed one category collision but cannot restore KO
    orientation, ownership color, material animation, or timing.
  - Proposed fix: review the preserved particle WIP, then graduate only the
    reachable KO subset as a strong source-derived path that preserves
    player/type, side orientation, colors, and DObj/material timing. A full
    generic effect interpreter is unnecessary if the generated DS-native
    specialization is mechanically equivalent. Remove the ring only after the
    replacement passes natural proof.
  - Required proof: strong-symbol and asset checks plus natural down/left/right
    Mario and Fox KOs, source-referenced visual A/B and owner approval, and
    pacing/memory cleanup guards. Status: PARTLY FIXED; source-faithful KO burst
    remains OPEN.
