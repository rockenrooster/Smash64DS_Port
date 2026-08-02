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
    NOT FIXED. Earlier release-ramp work covered the mid-waveform kill; the big-hit case still
    stands. RULED OUT 2026-08-02 so it is not re-walked: the whole reaction family (616 GaspM,
    619 Amazed, 620 GaspClap, 622 DamageL, 623 DamageM, 625 DamageS) shares source sample 37 and
    differs only by net_pitch_cents baked into the playback rate, and every rate checks out
    exactly as 32000 * 2^(cents/1200) -- so the odd-looking 27-53 kHz figures are correct, not
    corruption. The two cues shipping ds_volume 0 (620, 625) are ALSO correct: articulation 79
    opens `vol 0` for 18 ticks and both carry packed_envelope_count 2, so that is a deliberate
    fade-in, not a mute. And the runtime does step envelopes -- ndsAudioFgmUpdate runs per frame
    from taskman_seam.c:4427, not only on a new play.
    So the defect is not in cue data or envelope stepping. Next: latch WHICH cue is playing and
    what releases it at the moment of a big hit, rather than auditing the pack again.

-Respawn floating platform isn't visible when respawning after KO.
    Not fixed, I don't see the floating platform.
    NOT FIXED. Lifetime and growth were corrected and the halo measured alive, so it exists;
    it may simply be another victim of the matrix bug. Re-test first.

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
    NOT FIXED (audio half). The VFX half is done. Likely the same cue as the unrecognised SFX below
    -- treat them as one investigation.

-KO VFX wrong.
  Looks like the effect plays too close to the camera instead at the same z depth as the fighters and stage.
    **FIXED** (2026-08-02) pending your re-test. This line was the decisive clue for the whole
    class: "too close to the camera" is exactly what an identity modelview does, and
    PROJECTED_IDENTITY was one of the two modes measured. Thank you -- it is what turned four
    stalled rows into one bug.

-Results confetti doesn't look right.
  Almost fixed. size is correct, but confetti pieces don't fall freely, looks like they move as a unit or something.
    NOT FIXED. "Move as a unit" is the signature of one shared transform driving every piece.
    Re-test first -- the matrix bug could produce it -- then check per-piece xf.

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
    NOT FIXED. Both triggers are big-knockback events, so treat it as one cue with the KO SFX row.
    The crowd family is cleared as the source (see the crowd row for what was ruled out and how),
    so the candidate is now a cue whose PITCH AUTOMATION is not modelled: 615, 618, 620 and 625
    carry `ucd_pitch_automation` as declared fidelity debt, meaning the source sweeps pitch across
    the cue and the pack ships one baked constant rate instead. A swept cue rendered flat is
    exactly "wrong pitch, don't recognise it". Next: play those four in isolation with
    export-fgm-cue-wav.py and see whether one of them is the sound.

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
    NOT FIXED, but re-test first -- shield effects go through the same particle pass, so the
    camera fix may cover it. If not, ndsEFManagerShieldProcDisplay is its own draw path.
