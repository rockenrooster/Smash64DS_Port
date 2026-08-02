AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
These bugs should be fixed for P1 delivery:

-Whispy blow VFX not correct and not at correct location.
    Re-test. Emitter position is source-exact; the particle pass was drawing in the wrong space
    until the 2026-08-02 camera fix, which was itself broken for one build. Untested since.

-fgm12-nSYAudioFGMDeadUpStar-as-ds-plays-it.wav doesn't sound right
    **PARTLY FIXED** (2026-08-02). It was CLIPPING, which is your "high pitch getting clipped" on
    the KO row too: decoded peak 32768, one past int16, and the worst SNR in the pack at 17.4 dB.
    IMA predicts forward, so full-scale input makes it rail. Scaled the PCM 0.5 before encoding and
    raised ds_volume 41 -> 82, which is loudness-neutral: peak 32768 -> 16998, no clipping. SNR
    16.1 dB, 1.3 lower, which is the trade -- inaudible against audible clipping.
    STILL WRONG and it needs the next pass: this cue declares source_loop_infinite and ships
    ds_loop_flag 0, debt `source_loop_not_reproduced`. It sounds for 0.425 s of a 0.863 s note, so
    it stops halfway. Also `spawn_mod 95` in articulation 83 is not applied.
    SYSTEMIC, recorded so it is not rediscovered: 36 of 88 cues decode at full scale. The other 35
    are 19-39 dB and mostly one or two samples; the ones with ds_volume headroom can take the same
    treatment via FGM_ENCODE_HEADROOM, the ones already at 127 cannot be compensated.

-Some Crowd noise audio cues get cut off (like for big hits or upper bound KO).
    Not fixed. Mechanism still unidentified. Ruled out by measurement so they are not re-walked:
    cue rates are arithmetically correct; the two ds_volume 0 cues are real fade-ins; envelopes do
    step every frame; no asset is truncated; channel contention reads 0 premature retires against
    188 reuses. Seven cues (615/616/618/619/620/623/625) DID have real defects -- a flat first-note
    rate and dropped fork voices -- and now render through FULL_PROGRAM_AOT_IDS, 620 going 0.83 ->
    2.59 s. If it still cuts off, that was not the cause.
    ONE CORRECTION: I reported all 88 cues outlive their note using a 60 Hz frame. The FGM tick is
    the 5,750 us timer. The real figure is 22 of 88, of which only 609 (0.283 s), 96 (0.141 s) and
    605 (0.129 s) are cut by more than rounding -- and those three are chants, not big-hit
    reactions, so this still does not explain the row.

-Respawn floating platform isn't visible when respawning after KO.
    Not fixed, I don't see the CORRECT floating platform.
    **FIXED** (2026-08-02) pending re-test, read off the source asset rather than guessed at again.
    dEFManagerRebirthHaloEffectDesc -> reloc file 0x55 offset 0x2AC0 is a CHAIN of DObjDescs:
    node[1] has a display list at translate (0, -60, 0) and node[2] one at the origin. The port drew
    a single template pinned to the joint's own world position, so the pad rendered centred ON the
    fighter instead of sixty units under their feet -- it was there and it was inside them.
    Node[2] is still not drawn, and the pad's own texture (file 85 offset 8, IA8 16x16) is a 1-D
    radial falloff for ring or cone geometry, so the flat disc remains an approximation.

-Stray VFX are getting played across the stage when attacks are landed.
    Re-test. The hit position handed to the spark maker is a correct contact midpoint, measured, so
    nothing spawns in the wrong place; the pass was drawing in the wrong space.

-The rolling dodge sound (escape roll?) sounds off, maybe too loud???
    **FIXED** (2026-08-02) pending your ear. FGM 11 96 -> 68, a second -3.0 dB for -5.4 dB total.
    Verified in the binary: exported peak 17,458 -> 12,366, RMS 4,308 -> 3,052. Table takes another
    pass if still wrong.

-Correct VFX isn't played for various things (running foot dust VFX, fireball hit VFX, fox down B, shield, hard landing vfx, etc)
    Partly. Re-test after the camera fix. A SECOND cause is real and unfixed: 366 draws a match are
    refused because their texture is not on the 23-of-36 atlas sheet.

-Upwards KO boundary death: correct VFX and SFX never play for fighters.
  Almost fixed, I see the fighter die in the sparkle, but the SFX sound off, like the high pitch sound is getting clipped or something and another SFX is played that i don't recognize.
    Both halves answered. The unrecognised one is FGM 153, see below. The clipping is FGM 12, see
    above -- literally clipping, now removed.

-KO VFX wrong.
    not visible
    **FIXED** (2026-08-02) -- this was MY regression and it is the reason nothing was visible.
    gmcamera.c:1001 composes lookAt x perspective INTO gGMCameraMatrix, and I loaded that as the
    modelview while also loading the perspective again as the projection. Perspective applied twice
    collapsed every particle off screen. Projection is identity now. Confirmed by picture: the same
    Results crop has zero confetti on the broken build and coloured pieces on the fixed one.

-Results confetti doesn't look right.
    not visible
    **FIXED** (2026-08-02) pending your eye: 8 live pieces -> 112, drawn per seam 20 -> 62.
    Nothing was broken -- the density is what the SOURCE PARAMETERS produce, and it took measuring
    the right things to see that. The entry point is script 0x70 = 112, not 108-111: 112 is a pure
    spawner (`a5 006c a5 006d a5 006e a5 006f ff`) that creates the four emitters, and
    probe-results-confetti confirms both sheets live in slots 0 and 4, 4 pieces each, size 20,
    texture 22, 8 generators. Nothing missing, nothing mis-sized, nothing mis-placed.
    What is sparse is arrival against fall. Each emitter carries update_rate 0.07 and
    lbparticle.c:2324 emits one piece per whole unit of `frame += rand() * rate`, so about one per
    29 frames each. Gravity is 4.0 per frame squared, so a piece spawned at y = 1000 is past
    y = -224 when the probe reads it and gone within ~30 frames, far short of its 136-frame life.
    0.14 arrivals per frame against ~30 frames of residency is the twenty pieces you saw.
    Fixed by specializing the four emitters to update_rate 0.42 at bank load
    (NDS_R2_CONFETTI_UPDATE_RATE), which PROJECT_GOAL explicitly allows, plus the Results-scene
    pool at the source's 112/24/16. BOTH were needed and that is why the pool alone did nothing
    when tried first: at 0.07 the generators never asked for more than 40 structs.
    Verified: StructsLive 112, DrawVisibleMax 62, Results present-interval max 6 and bucket[2]
    576/713 -- identical pacing to before -- NO-FREEZE, 0 malloc overflows.
    TWO HARNESS NOTES so they are not re-learned: the results-lab CANNOT test confetti, because
    mnvsresults.c:3248 gates it on sMNVSResultsKind != NoContest and the lab is NoContest; and
    probe-results-confetti does NOT rebuild, so a run whose floats are byte-identical to the
    previous one measured the old ELF. Both cost a cycle here.

-A new SFX that i don't recognize has developed, don't know if its the wrong pitch or what.
    **FIXED** (2026-08-02) -- you identified it as FGM 153 AltitudeWarn. Not new and not misfiring:
    it triggers on being knocked high, which is why it read as unfamiliar rather than missing.
    Articulation 150 sweeps pitch 550 -> 2390 cents inside mark_loop/jump_loop, and 153 sat on the
    DS hardware-repeat path, which cannot ramp -- so a 1.725 s siren played as a 0.108 s monotone
    blip on loop, and only the loop tail was packed at that. Now on FULL_PROGRAM_AOT_IDS: 55,200
    samples at 32 kHz = 1.725 s, and measured zero-crossing rate falls 1820 -> 1063 Hz.
    NOT COMPLETE: it decays to silence at ~1.05 s and no longer repeats, while the source loops
    infinitely. If the warning must persist while out of bounds, that is the next piece.

-Hitting Fox's shield freezes match sometimes.
    **FIXED** (2026-08-02). syTaskmanMalloc cannot fail, it SPINS in syMallocSet's `while (TRUE);`,
    so every `if (heap == NULL)` under it is dead code. The July fix gave the anim cache its own
    arena but left two loaders on the shared heap -- reloc_backend_assets.c:3483 and :6535. A shield
    hit drives rebound into damage-fall into a new status, a common route to an uncached animation,
    which is why it read as random. Both ask ndsSyMallocWouldFit and decline now.
    Verified: 5-minute both-CPU soak, NO-FREEZE.

-Shield VFX is not correct.
    **PARTLY FIXED** (2026-08-02), and confirmed against the extracted N64 asset, not just the code.
    The shield is reloc file 0xa3 (gFTManagerCommonFile): display list at 0x248, DObjDesc at 0x300.
      ALPHA -- `gDPSetPrimColor ffffffc0` is in the SHIPPED N64 DATA. The port had 0x60 centre and
      0x50 rim, up to 2.4x too transparent. All ten values now 0xC0.
      SHAPE -- gSPVertex loads FOUR vertices, so the source shield is a textured billboard quad and
      NOT a sphere. The port's flat disc is structurally faithful; no sphere is owed.
      GLINT -- your capture puts it top middle at Mario's ear. BuildDisc's patch was centred at
      (-39.5, 111), upper left. The five shield templates now pass +40; reflector and respawn pad
      pass 0 and stay byte-identical, since there is no reference for those two.
    NOT FIXED, and it is the dominant difference: the RIM. The N64 quad's IA texture (file 0xa3
    offset 8, 16x32 IA8 mirrored in S by cmS=3 into 32x32) falls to zero alpha over its last few
    texels, so the bubble is soft-edged. BuildDisc emits 8 flat segments and is visibly faceted.
    Vertex colours cannot make a soft edge -- only the texture can, and it needs texture space.
    THE ATLAS ROUTE IS MEASURED AND CLOSED: 128x64 -> 128x128 was tried on 2026-08-02. The atlas
    uploaded fine (AtlasFailCount 0, 16,400 bytes) and quad misses fell 684 -> 0, but everything
    allocating after it broke -- ViolationCount 0 -> 1, StagePrepareBuildCount 2 -> 244, and the
    Results scoreboard vanished. Reverted. A retry must solve CONTIGUITY, not byte count.
