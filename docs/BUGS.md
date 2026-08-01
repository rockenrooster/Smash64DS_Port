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
  Narrowed 2026-08-01, and the weapon make is NOT the owner. The import already
  counts both sides of the call (`battleship_mario_fireball.c`), and a clean
  one-match soak reports `SpawnCallCount 9` against `SpawnSuccessCount 9`:
  every request the special-N state machine made produced a weapon. So this is
  upstream of the make -- the input or the status transition not reaching
  `wpMarioFireballMakeWeapon` at all -- or downstream of it, a weapon that
  exists and is not drawn. Both counters are on the soak's reported list now,
  so the next owner-observed miss can be attributed from the run that produced
  it rather than reproduced first.
  The downstream half is now measurable too: `gNdsWeaponRendererSubmitCount`,
  `...VisibleDrawCount`, `...RejectedDrawCount` and the fireball-specific
  `gNdsWeaponRendererFireballSubmitCount` / `...FireballVisibleDrawCount` are
  unconditional counters in the shipping build
  (`reloc_backend_movement.c:12925`), so "a weapon exists and is not drawn"
  separates from "a weapon was never made" without a new probe.

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
  **BOTH HALVES IMPLEMENTED 2026-08-01; needs an ear check.**
  *Trigger side:* `ft/ftpublic.c` is compiled in place
  (`src/import/battleship_ftpublic.c`, `NDS_IMPORT_BATTLESHIP_FT_PUBLIC`), so
  the thresholds, cooldowns, repeat limits and defeated-voice queue are the
  source's rather than a translation. **It COMPILES AND LINKS as of 2026-08-01;
  before that the flag had never been built, and the claim below that "its whole
  external surface already existed" was half wrong.** The functions exist, but
  not under the declarations the decomp TU expects, and one dependency did not
  exist at all:
  `func_800269C0_275C0` is declared `void *` by `sys/audio.h:71` and
  `alSoundEffect *` by `ftpublic.c:4` (a hard conflict, resolved by renaming --
  which moves the declaration and every call site together, the one case where
  the `#define` seam's usual limitation is what is wanted); `U16_MAX`,
  `DObjGetStruct` and `ftParamGetPlayerNumGObj` had no port declaration; and
  `ftPublicCommonCheck` lost its prototype the moment its port definition
  compiled out, which broke its own caller. The real dependency was
  **`dFTCommonDataPublicFighterCallFGMs`** -- `ft/ftcommondata.c` is not
  compiled here and `nm` finds no such symbol -- now transcribed entry for entry
  beside the import, with the ten missing `nSYAudioVoicePublic*` constants added
  to `gmsound.h` at the decomp's values.
  Still default 0 until a natural match proves the reactions reach the speaker:
  it costs `.text`, and `.text` costs taskman arena one for one here, with only
  about five kilobytes of margin before the GObj latch fires.
  *Cue side:* the eleven a P1 Mario-vs-Fox match reaches are packed -- chants
  605 Fox / 609 Mario, reactions 615/616/617 Gasp L/M/S, 618 Cheer, 619 Amazed,
  620 GaspClap, 622/623/625 Damage L/M/S. 624 NoContest is unreachable in a
  two-fighter timed match. Pack 535,280 -> 672,528 B, 63 -> 75 entries.
  **The earlier "eight of the twelve are LOOPED or FORKED" was half wrong**:
  `--derive` reports `loop: False` for all eleven, so no loop machinery was
  needed at all. Five do carry fork voices (650, 627, 676, 684, 625) and those
  are omitted with the debt recorded, exactly as the shipping punch/kick cues
  do. 96 `GroundGrind2` is still out -- it has no `pitch` op, so
  `articulation_pitch_cents` derives as `None` -- and it is not a crowd cue;
  it belongs with 85 and 153 `AltitudeWarn` on the miss ring, below.
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

-FIXED (2026-08-01, needs an ear check) Three cues a natural match still asked
  for and did not get (2026-08-01 miss ring, control soak): 96 `GroundGrind2`
  x6, 85 x2, 153 `AltitudeWarn` x2. Not a row of their own before now because
  nothing had read the ring on a clean run. All three are packed.
  - **96** had no `pitch` op, so `articulation_pitch_cents` derived as `None`
    and `validate_articulation` rejected it. No pitch op means zero cents;
    the validator says so now. It is NOT a looped cue in practice -- its
    schedule (55 ticks, 0.316 s) runs out before its 13,040-sample wave
    (0.457 s at 28,509 Hz), so a one-shot plays the whole audible cue.
  - **153** did need the loop -- 300 ticks is 1.725 s against a wave that
    plays out in 0.757 s -- and it is the pack's SECOND DS hardware repeat.
    Whispy's `WHISPY_WIND_*` module constants are a per-cue `hardware_loop`
    spec now (five numbers; four are the same for every cue on the path), so
    the "machinery that needs generalising" this row kept describing was a
    dict. PNT 1 word, LEN 396, SNR 32.9 dB, pinned in the checker like 285.
  - **85 `UnkGrind4` is FIXED too (2026-08-01); needs an ear check.** The row
    used to say it "needs a decision about how the DS should represent a rate
    above the register's range". There was no such decision to make: the
    register is fine. 32000 * 2^(1800/1200) = 90,510 Hz is past the `u16
    frequency` field of the PACK ENTRY (`nds_audio_fgm.c:46`), and the DS
    channel timer reaches roughly a megahertz. `source_rate_above_u16` is a
    statement about one field of ours.
    189, 190 and 219 carry the same blocker and were already answered: render
    the whole source program AOT at `FGM_OUTPUT_RATE` and bake the note
    schedule into the samples, so the entry stores 32,000. 85 has exactly that
    shape -- three notes, no forks -- so it joined `FULL_PROGRAM_AOT_IDS` and
    needed no new machinery. 2,576 samples, 1,292 IMA bytes, SNR 25.07 dB.
    One real gap did have to be closed: its articulation spawns modulator 24,
    `shape 7` (`ramp_down_oneshot`) on `target 28`. The renderer raised on
    both. Per the decomp's own field notes
    (`decomp/tools/extract_fgm.py`), target 24+ is **cross-mod another
    voice** -- and 85 has no fork voices, so it has no destination at all.
    Cross-voice targets are now skipped *before* evaluation, which is why the
    shape never has to be interpreted, and only for a cue with no forks; with
    forks it stays a hard error, because there the modulation is real.

-FIXED (2026-08-01, needs an ear check) Five more cues, and these are the ones
  a player would notice first: **11 `Escape`** (the dodge) x3, **13 `GuardOn`**
  x2 and **14 `GuardOff`** (the shield going up and down), **278 `GamePause`**,
  and **369 `FoxOttotto`** (the noise Fox makes teetering on a ledge). All five
  were silent.
  They were invisible until now because **every previous miss-ring read was on a
  SINGLE-CPU soak**, where Mario stands still: nobody dodges, nobody shields,
  nobody teeters. The first both-CPU stress soak (2026-08-01, crowd-actor ROM,
  full match to Results) reported them immediately -- 198 FGM play calls, 190
  supported, 8 unsupported, and those 8 are exactly these five ids.
  All five are bounded multi-note schedules with no fork voices, so all five
  take the same full-program AOT render 85 does. Pack 682,036 -> 700,892 B,
  78 -> 83 entries.
  One piece of machinery was genuinely missing and is now source-transcribed:
  **modulator shapes 6 and 7** (`ramp_up_oneshot` / `ramp_down_oneshot`), which
  FGM 11's articulation spawns. They are shapes 2 and 3 with the phase CLAMPED
  at the period instead of wrapped -- `n_env.c:4158` and `:4172` both assign
  `phase = period` past the end rather than subtracting it, and the value
  expressions are identical to the periodic pair's. Shapes 4, 5 and 8 remain
  unsupported on purpose: they call `randFloat1`/`randFloat2` and are not
  reproducible offline.

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
  VFX still open, but the stub it used to name is GONE. `NDS_R2_PARTICLE_RUNTIME`
  and `NDS_R2_PARTICLE_DRAW` both default 1 as of 2026-08-01, so the real
  `lbParticleMakeScriptID` from `lb/lbparticle.c` is live and textured quads are
  emitted. What remains for *this* row specifically is that Dream Land's Pupupu
  bank -- Whispy's leaves (script 0) and dust (script 1) -- still registers
  EMPTY: `ndsParticleLoadEFCommonBank` covers only the common bank and every
  other bank takes `ndsParticleRegisterEmptyBank`
  (`battleship_lbparticle.c:705-725`), so a Pupupu script request fails closed
  with reject reason 3 or 4. It needs the pack step extended to the grpupupu
  bank and its texture closure admitted to the atlas -- and the atlas is at a
  measured hard bound of 8,192 bytes, so admitting more needs a second sheet or
  a smaller per-texture format, not a bigger sheet.
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
  **The root cause below is HALF CLOSED as of 2026-08-01.** The original
  particle scripts DO run now and they ARE textured: `NDS_R2_PARTICLE_RUNTIME`
  and `NDS_R2_PARTICLE_DRAW` both default 1, the imported `lb/lbparticle.c` owns
  `lbParticleMakeScriptID`, the common EF bank is resident, and a soak measured
  117,937 textured quads emitted with zero atlas misses. What is NOT closed is
  COVERAGE: the quad atlas admits **six of 31 textures** (source ids 0, 3, 9, 22,
  27, 37 -- 7 frames, 5,376 texel bytes) because 8,192 bytes is a measured hard
  VRAM bound, and a particle whose texture is absent draws nothing rather than
  drawing wrong. `gNdsParticleQuadMissCount` is what names the gap per effect,
  and it is on the soak's reported list. The original text of this row follows,
  and its first two sentences are now historical:
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
  (2026-08-01: the "particle script is not resident" half of this row now
  depends only on atlas COVERAGE -- see the VFX row above. The interpreter runs
  and draws textured; six of 31 textures are admitted, and the KO burst's own
  texture is one of the ones a soak's `gNdsParticleTextureUseMask` /
  `gNdsParticleQuadMissCount` pair will settle.)
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
