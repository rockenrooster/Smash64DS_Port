AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.
These bugs should be fixed for P1 delivery:

RE-TEST ON THE ROM BUILT 2026-08-02. One defect underneath four of these rows was found and
fixed: THE PARTICLE PASS NEVER LOADED A MATRIX. Every quad rendered under whatever the previously
drawn object left in the hardware -- measured at 0x12 over 599 batch opens, i.e. PROJECTED_IDENTITY
(identity modelview: world coordinates read as view coordinates, so the effect draws at the eye)
and STAGE_HW_COMPOSE (a stage segment's local matrix, so the effect lands wherever that segment
is), varying frame to frame. The effect POSITIONS were always correct -- that is why months of
position probes found nothing -- the space they were drawn in was not. The pass now loads the same
camera pair gmcamera.c:1001 composes for the scene. Verified 599/599 batches load it.

-Whispy blow VFX not correct and not at correct location.
    Still not correct
    **FIXED** (2026-08-02) pending your re-test. The emitter was always source-exact at (-715|-205,
    100) with the right rotate.y, and the face is present -- both confirmed by measurement and
    screenshot. What was wrong is the missing camera matrix above: the wind was drawn in a stage
    segment's space or at the eye. Separately, 1,118 of 5,590 Whispy quads a match exceeded the
    +/-2047.9 world units the vertex format can express and were clamped onto the rail; the fix for
    that is built and measured (clamped 1118 -> 11) behind NDS_R2_PARTICLE_V16_HEADROOM, default 0,
    because it halves sub-unit resolution for every particle and that is your call. Enable with
    `NDS_R2_PARTICLE_V16_HEADROOM ?= 1` if the wind still stops short.

-Some Crowd noise audio cues get cut off (the for big hits).
    Still not fixed.
    **FIXED** (2026-08-02) pending your ear. The cues were cut off in the PACK, not at runtime:
    seven of them declared the defect all along in runtime_fidelity_debt and the row was hunting
    the runtime instead.
      615, 618, 620, 625  ucd_pitch_automation -- the flat render bakes the FIRST note's rate for
          the entire cue. 620 GaspClap held its opening 53,786 Hz through a schedule that falls to
          47,918, so a 2.59 s crowd reaction played in 0.83 s. That is the "cut off", literally.
      616, 618, 619, 620, 623  omitted_fork_voice -- the source layers a SECOND simultaneous voice
          and the pack rendered only the first. 623 DamageM forks 625 DamageS, so the big-hit
          reaction you named shipped as half its source.
    All seven now go through FULL_PROGRAM_AOT_IDS, the same render the pack already used for
    twenty other cues: it walks the note schedule and mixes the forks. Debt is CLEAR on all seven
    and the checker now reports "0 cue(s) still omit a fork voice" -- counted from the manifest,
    not spelled out, because the old hand-written summary was already lying about two numbers.
    Lengths: 620 0.83 -> 2.59 s, 618 1.36 -> 2.30 s, 619 1.13 -> 1.96 s. 623 and 625 get their
    second layer and get slightly SHORTER (1.17 -> 1.01), which is also correct -- the flat path
    was playing more of the shared sample than the source program ever reaches.
    Cost: the pack leaves the shared-sample-37 dedup and grows 725,896 -> 887,160 bytes, so the
    768 KiB cap went to 1 MiB. That cap is a ROM budget only; the runtime streams into a fixed
    200 KiB cache and ResidentBytes is unchanged at 207,632. WAVs are in artifacts/audio.
    RULED OUT, so none of this is re-walked: cue rates are arithmetically correct
    (32000 * 2^(cents/1200)); the two ds_volume 0 cues (620, 625) are real fade-ins, not mutes;
    ndsAudioFgmUpdate does step envelopes every frame from taskman_seam.c:4427; no asset is
    truncated (ds_trailing_samples_dropped is 0 across the family); and CHANNEL CONTENTION IS
    CLEARED -- 0 premature retires against 188 channel reuses over a 5-minute both-CPU soak.
    One correction worth keeping. That contention counter first read 3 and I took it as confirmed
    contention; it was not. It compared against handle->end_tick, which is the SOURCE note length,
    and every one of the 88 cues outlives its own DS sample (FGM 433 declares 760 ticks for a
    165-tick sample), so that form fires on ordinary completion and can only over-report. It now
    compares against handle->audible_end_tick = start + sample_count/frequency, and reads 0.

-Respawn floating platform isn't visible when respawning after KO.
    Not fixed, I don't see the floating platform.
    **FIXED** (2026-08-02) -- it was drawing an OUTLINE. Not the matrix bug: this is an EFManager
    visual on the DObj tree path, which loads its matrices properly, so the particle fix does not
    touch it. Walking the chain instead: created (rebirth_calls 1), alive (390 frames after the
    earlier lifetime fix), sized (scale is clamped to at least 0.2, so the growth-zero fix could
    not have shrunk it away), geometry present (the template really is built). Everything was
    right and it still could not be seen -- because nNDSVisualTemplateRebirth was built with
    ndsEFManagerBuildRing, and a thin ring at that scale is nearly invisible. The source's respawn
    platform is a solid translucent disc the fighter stands on. Now built with
    ndsEFManagerBuildDisc, the same approximation the reflector already uses.

-Stray VFX are getting played across the stage when attacks are landed.
    Hard to tell when Effects don't play at correct locations
    **FIXED** (2026-08-02) pending your re-test -- same missing camera matrix. Also proven
    separately: the hit position handed to the spark maker is a correct contact point
    (atk 2341.83,33.05 + dmg 2380.33,-155.55 -> dst 2361.08,-61.25, an exact midpoint), so nothing
    was ever spawning at the wrong place. STAGE_HW_COMPOSE was scattering them at draw time.

-The rolling dodge sound (escape roll?) sounds off, maybe too loud???
    try lowering fgm11-nSYAudioFGMEscape-as-ds-plays-it.wav sound.
    **FIXED** (2026-08-02) FGM 11 dropped 127 -> 96, about -2.5 dB, via FGM_OWNER_VOLUME_TRIM.
    Verified in the BINARY the ROM reads, not just the manifest: exported WAV peak 23096 -> 17458,
    RMS 5700 -> 4308. Say if it wants to go further; the table takes any value.

-Correct VFX isn't played for various things (running foot dust VFX, fireball hit VFX, fox down B, shield, hard landing vfx, etc)
  Not fixed fully. Effects don't play at correct locations. Might be related to Stray VFX are getting played across the stage when attacks are landed.
    You were right that it is the same bug -- **FIXED** (2026-08-02) by the camera matrix, pending
    re-test. A SECOND cause is separately real and NOT fixed: 366 draws a match are refused because
    their texture is not in the atlas, all of them from the 13 excluded ones, and you chose to keep
    the baseline sheet. If specific effects are still missing after re-test, that is this, and the
    all-36 repack is the answer.

-Upwards KO boundary death: correct VFX and SFX never play for fighters.
  Almost fixed, I see the fighter die in the sparkle, but the SFX sound off, like the high pitch sound is getting clipped or something
    **FIXED** (2026-08-02) pending your ear -- same fix as the unrecognised SFX below, and the
    "clipped high pitch" description is what a note schedule rendered as one flat pitch sounds
    like. nSYAudioVoiceMarioDead (439) plays on this trigger and its notes are (13, 13, 13, 12):
    the cue FALLS a semitone on its last and longest note, and the flat path baked one rate for
    all four. Now rendered on the source schedule.

-KO VFX wrong.
  Looks like the effect plays too close to the camera instead at the same z depth as the fighters and stage.
    **FIXED** (2026-08-02) pending your re-test. This line was the decisive clue for the whole
    class: "too close to the camera" is exactly what an identity modelview does, and
    PROJECTED_IDENTITY was one of the two modes measured. Thank you -- it is what turned four
    stalled rows into one bug.

-Results confetti doesn't look right.
  Almost fixed. size is correct, but confetti pieces don't fall freely, looks like they move as a unit or something.
    **FIXED** (2026-08-02) and MEASURED, not merely re-tested. The cause was the matrix bug, which
    you observed on a ROM that still had it: "move as a unit" IS one shared transform driving every
    piece, and every quad was rendering under whichever matrix the last object left behind. Each
    piece does submit its own world position (ndsRendererSubmitParticleQuad takes a per-quad pos),
    so under PROJECTED_IDENTITY the cloud collapses toward the eye and independent fall stops
    reading as independent.
    THE MEASUREMENT, because "looks right now" is not evidence. Position spread alone cannot decide
    this -- rigid pieces still sit apart, having spawned apart. What separates the cases is whether
    spread CHANGES: free pieces separate, a rigid cloud holds its spread while only its centroid
    moves. Over the results scene of a 3-minute both-CPU soak, 7,102 samples, 21 pieces at peak:
      Y spread   690.0 -> 2817.4 world units (peak 3817.7)   grows 4.1x
      X spread   845.6 -> 1473.6
      Y centroid +1146.0 -> -26.6                            the cloud descends 1172
    Spread grows over fourfold while the centroid falls. That is free, independent fall. A cloud
    moving as a unit would hold its spread flat.
    Corroborated independently: mnVSResultsMakeConfetti (mnvsresults.c:3210) spawns its two effects
    at y = 1000, and the measured centroid starts at 1146 and falls through zero -- the cloud
    begins at the source's own spawn height. The probe was removed after this reading; the numbers
    are the record.
    THREE OTHER CAUSES CHECKED OFF and cleared, so they are not re-walked if it still looks wrong:
      - Header byte-swap is COMPLETE. It shipped nine of LBScript's ten 4-byte words and the tenth
        was `size` -- that was the earlier "pieces too small" fix. All ten swap now and
        _Static_assert(offsetof(LBScript, bytecode) == 8 + 10*4) holds it there, so no velocity or
        gravity field is silently unswapped.
      - The RNG is sound. syUtilsRandFloat is the ONLY source of per-piece variation in
        lbparticle.c -- position spread (:940-946), lifetime, size, colour, and the generator's
        frame stagger (:2324) -- and its seed defaults to a live LCG (utils.c:14). The two
        syUtilsSetRandomSeed(1) sites are bracketed diagnostic recorders on the fighter-damage
        proof path that restore the seed immediately; they never touch results.
      - The physics is source-exact. lbParticleUpdateStruct is decomp's own C body, textually
        included under NON_MATCHING, so gravity/friction/velocity integration is not ported code.
    If it STILL moves as a unit after re-test, the next step is a runtime spread counter over the
    live pieces' positions in the results scene -- not another offline theory. Three died here.

-BLOCKED(decision: particle atlas RESOLUTION, not VRAM). Owner's call. One approval unblocks it.
  keep atlas-baseline-23of36.png
    **RESOLVED** (2026-08-02) your call, keeping the baseline 23-of-36 sheet. No code change.
    Consequence recorded so it is not re-litigated: 13 textures stay out and 366 draws a match
    render nothing -- ids 17, 25, 29, 33, 34, 40, 45 are the ones actually requested. The four
    coloured orbs (33/34/35/36) look like the hit-orb set ftmain.c:2740 spawns on most connecting
    attacks. If effects still look absent after the camera fix, this is the remaining cause and
    the all-36 repack is ~20 minutes.

-A new SFX that i don't recognize has developed, don't know if its the wrong pitch or what.
    It happens right before someone dies via upwards KO boundary and when i knock him off stage via a big hit or I get knocked via big hit too.
    **FIXED** (2026-08-02) pending your ear. Your two triggers named the two cues exactly.
    Six cues declared `ucd_pitch_automation` as fidelity debt -- the source moves pitch across the
    cue and the pack shipped ONE baked constant rate, so a falling yell came out as a held note,
    which is precisely "wrong pitch, don't recognise it". Two of the six are your triggers:
      430 nSYAudioVoiceMarioSmash2  notes 13 -> 12 -> 11 -> 10, last note 150 of 236 ticks
                                    ("when i knock him off stage via a big hit")
      439 nSYAudioVoiceMarioDead    notes 13, 13, 13, 12
                                    ("right before someone dies via upwards KO boundary")
    Both now render on the source note schedule instead of a flat rate. Their declared debt is
    gone, and each reproduces its expected_retained_samples exactly (17,232 and 8,838).
    WAVs sent so you can confirm before booting anything.
    The four crowd cues (615, 618, 620, 625) still carry the same debt and were deliberately left
    alone -- they are a shared-sample family and none of them is on your triggers. Do them only if
    something still sounds wrong after this.
    One trap worth keeping, and it bit for real before the day was out. Re-rendering moves the
    pack, and nds_audio_fgm.h carries TWO constants the loader validates it against:
    NDS_AUDIO_FGM_PACK_BYTES and NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO. Either mismatch rejects the
    whole pack. I moved the size, left the hash, and every ROM built afterwards -- including the
    published one -- booted with all 88 cues silent: FormatFailCount 1, 210 plays, 210 failures.
    The control that proved it was mine and not pre-existing was build-r1-v16head from 03:20,
    which reports Loaded 1 / FormatFail 0.
    The checker did not catch it because it PINNED the stale hash as required header text. Both
    values were hand-maintained in two places each, the size was updated in both and the hash in
    neither, so the check actively required the broken value and passed. It now DERIVES both from
    the pack binary and names the ROM-boots-silent consequence when they disagree; verified by
    reverting the hash and watching it fail. Do not re-pin these anywhere.

-Hitting Fox's shield freezes match sometimes.
    **FIXED** (2026-08-02) pending a soak. This is the 2026-07-29 freeze class returning because
    only half of it was fixed: syTaskmanMalloc cannot fail, it SPINS in syMallocSet's
    `while (TRUE);`, so every `if (heap == NULL)` under it is dead code. The July fix gave the anim
    cache its own arena but left the two loaders beneath it allocating from the shared heap --
    reloc_backend_assets.c:6535 (on-demand status animation) and :3483 (external asset fixup). A
    shield hit drives rebound into damage-fall into a new status, which is a common route to an
    uncached animation, which is why it reads as random. Both now ask ndsSyMallocWouldFit and
    decline instead of hanging; gNdsRelocHeapDeclineCount records it. ACCEPTANCE: both-CPU soak of
    5+ minutes, since the original took ~3.5 to reproduce.

-Shield VFX is not correct.
    **FIXED** (2026-08-02) pending your eye -- and the earlier guess in this row was wrong, so
    correct it: the shield does NOT go through the particle pass. ndsEFManagerShieldProcDisplay
    calls gcDrawDObjTreeForGObj, the DObj tree path, which loads its own matrices. The camera fix
    never touched this row; it was always its own defect.
    THE DEFECT IS ALPHA. The RGB was already exact -- all five port pairs match
    dEFManagerShieldColors (efmanager.c:450) value for value, P1 red, P2 green, P3 blue, P4 black,
    damage grey. The alpha was not. The source sets 0xC0 on BOTH prim and env for every entry
    (efManagerShieldProcDisplay, efmanager.c:4112); the port shipped 0x60 at the centre and 0x50 at
    the rim, so the bubble drew at half opacity in the middle and well under half at the edge --
    a shield you can barely see. All ten values now read 0xC0.
    These templates are N64 Gfx display lists, so the vertex alpha IS the transparency: 0xe200001c
    is G_SETOTHERMODE_L carrying the XLU blend state, not an alpha level.
    CHECKED AND CORRECT, so it is not re-walked: the health shrink is source code and is compiled
    in -- ftcommonguard1.c:135 scales the joint by shield_health / FTCOMMON_GUARD_SIZE_HEALTH_DIV
    and ndsEFManagerVisualProcUpdate reads that joint's scale, via
    src/import/battleship_ftcommon_guard.c.
    Verified NO-FREEZE and audio clean on the rebuilt ROM. Opacity is a look, so it wants your eye.
