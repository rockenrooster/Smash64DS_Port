AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
These bugs should be fixed for P1 delivery:

This board carries verdicts and the numbers that check them. The forensics for each
fix live next to the code that owns it -- the particle generator and its checker,
`battleship_lbparticle.c`, `battleship_efmanager.c`, `render-audio-fgm-phase-pack.py`
-- so a row here should not need to be an essay.

-Whispy blow VFX not correct and not at correct location.
    Re-test. Emitter position is source-exact; the particle pass was drawing in the wrong
    space until the 2026-08-02 camera fix. Untested since.

-fgm12-nSYAudioFGMDeadUpStar-as-ds-plays-it.wav doesn't sound right
    **FIXED** (2026-08-02) pending your ear. Two defects, one move: it CLIPPED (decoded peak
    32768, worst SNR in the pack) and it dropped its infinite source loop, sounding for
    0.425 s of a 0.863 s note. Rendering it through FULL_PROGRAM_AOT_IDS bakes the whole
    source program and fixes both -- 27600 samples at 32 kHz = 0.863 s, peak 15605, SNR
    16.086, fidelity debt CLEAR.
    Verified: checker PASS, decode PASS (88 entries), soak reads FgmLoaded 1, FormatFail 0,
    210 plays, 0 misses, 0 premature retires.
    SYSTEMIC: 36 of 88 cues decode at full scale. Those with ds_volume headroom can take
    FGM_ENCODE_HEADROOM; those already at 127 need the AOT route, as this one did.

-Some Crowd noise audio cues get cut off (like for big hits or upper bound KO).
    Not fixed, mechanism unidentified. Eight excluded by measurement -- do not re-walk them.
    Cue rates are correct; the two ds_volume 0 cues are real fade-ins; envelopes step every
    frame; no asset is truncated; the handle pool never stole (PoolExhaust 0, PrematureRetire
    0, GenerationMismatch 0 across 210 plays, even at 8 of 8 active); StopAll's 4 calls all
    come from the GAME SET proc, not a per-hit path; and the end-sound queue does drain
    (func_800269C0_275C0, reloc_backend_compat_shims.c:650).
    Seven cues (615/616/618/619/620/623/625) DID have real defects -- flat first-note rate,
    dropped fork voices -- and now render AOT, 620 going 0.83 -> 2.59 s. Re-test against that.
    CORRECTION: my "all 88 cues outlive their note" used a 60 Hz frame; the FGM tick is
    5,750 us. The real figure is 22 of 88, and only 609/96/605 by more than rounding -- all
    chants, not big-hit reactions, so that is not the row either.
    Next evidence has to come from the ear: which cue, roughly when. The counters are spent.

-Respawn floating platform isn't visible when respawning after KO.
    Not fixed, I don't see the CORRECT floating platform.
    **FIXED** (2026-08-02) pending re-test, read off the source asset. dEFManagerRebirthHalo
    EffectDesc (reloc file 0x55 offset 0x2AC0) is a CHAIN of DObjDescs: node[1]'s display list
    sits at translate (0, -60, 0). The port drew one template pinned to the joint's own world
    position, so the pad rendered inside the fighter instead of sixty units under their feet.
    Still approximate: node[2] is not drawn, and the pad's own texture is a radial falloff
    meant for ring or cone geometry, not a flat disc.

-Stray VFX are getting played across the stage when attacks are landed.
    Re-test. The hit position handed to the spark maker is a measured, correct contact
    midpoint, so nothing spawns in the wrong place; the pass was drawing in the wrong space.

-The rolling dodge sound (escape roll?) sounds off, maybe too loud???
    **FIXED** (2026-08-02) pending your ear. FGM 11 96 -> 68, a second -3.0 dB for -5.4 dB
    total. In the binary: peak 17,458 -> 12,366, RMS 4,308 -> 3,052. Another pass if still off.

-Correct VFX isn't played for various things (running foot dust VFX, fireball hit VFX, fox down B, shield, hard landing vfx, etc)
    **FIXED** (2026-08-02) pending re-test, both causes.
    The dominant one was the camera bug below -- it collapsed every particle off screen, so
    everything on your list read as missing.
    The second turned out to BE your list. The soak's QuadMissMask named nine textures refused
    100% of the time: 17 DustDash (running foot dust), 40 DamageFire (fireball hit), 33/34
    DamageNormalLight and 41 DamageNormalHeavy (the hit flashes), 45/29 SparkleWhite(Multi),
    25 DamageCoin, 38 SetOff.
    They were excluded for FRAME COUNT, not size, so the fix is decimation rather than VRAM:
    the generator packs an evenly-spaced subset of each animation and the runtime lookup now
    returns the nearest EARLIER packed frame instead of nothing. Admitted 23 -> 33 of 36, KO
    closure and Pupupu intact, and **atlas_bytes is UNCHANGED at 8,192** -- growing that
    allocation is what broke the ROM twice. Only 28/35/36 stay out; none of them drew.
    Verified on a 3.5-minute both-CPU soak: QuadMissCount 684 -> 0, both mask words 0, and
    QuadEmitCount 376,474 exactly equals DrawVisibleCount -- every visible particle draws,
    against 362,075 before. AtlasBytes 8208 unchanged, AtlasFail 0, Violation 0, stage fast
    path 2358 reuse / 4 build, sGCCommonsMaxNum still -1, MallocOverflow 0, NO-FREEZE.
    ONE LESSON WORTH KEEPING: 684 refused of 362,759 visible is 0.19% and reads as a rounding
    error. It was not a rate -- those effects were binary-absent. Grade an atlas by which
    effects lose their texture, never by share of draws.

-Upwards KO boundary death: correct VFX and SFX never play for fighters.
  Almost fixed, I see the fighter die in the sparkle, but the SFX sound off, like the high pitch sound is getting clipped or something and another SFX is played that i don't recognize.
    Both halves answered. The unrecognised one is FGM 153, see below. The clipping is FGM 12,
    see above -- literally clipping, now removed.

-KO VFX wrong.
    not visible
    **FIXED** (2026-08-02) -- MY regression, and the reason nothing was visible. gmcamera.c:1001
    composes lookAt x perspective INTO gGMCameraMatrix; I loaded that as the modelview while
    also loading the perspective again as the projection. Applied twice, it collapsed every
    particle off screen. Projection is identity now, confirmed by picture.

-Results confetti doesn't look right.
    not visible
    **FIXED** (2026-08-02) pending your eye: 8 live pieces -> 112, drawn per seam 20 -> 62,
    pacing unchanged (present-interval max 6), NO-FREEZE, 0 malloc overflows.
    Nothing was broken -- the sparseness is what the source parameters produce. Each of the
    four emitters carries update_rate 0.07, which is one piece per ~29 frames, against a fall
    that clears the screen in ~30. Specializing them to 0.42 plus the Results-scene pool at
    the source's 112/24/16 fixes it; BOTH were needed, which is why the pool alone did nothing.
    TWO HARNESS NOTES: the results-lab CANNOT test confetti (mnvsresults.c:3248 gates it on
    sMNVSResultsKind != NoContest and the lab is NoContest), and probe-results-confetti does
    NOT rebuild, so byte-identical floats can mean it measured the old ELF. Both cost a cycle.

-A new SFX that i don't recognize has developed, don't know if its the wrong pitch or what.
    **FIXED** (2026-08-02) -- you identified it as FGM 153 AltitudeWarn. Not new and not
    misfiring: it triggers on being knocked high. Articulation 150 sweeps pitch 550 -> 2390
    cents, but 153 sat on the DS hardware-repeat path, which cannot ramp, so a 1.725 s siren
    played as a 0.108 s monotone blip. Now AOT: 55,200 samples at 32 kHz, ZCR 1820 -> 1063 Hz.
    NOT COMPLETE: it decays at ~1.05 s and no longer repeats while the source loops
    infinitely. Only worth another pass if the warning must persist while out of bounds.

-Hitting Fox's shield freezes match sometimes.
    **FIXED** (2026-08-02). syTaskmanMalloc cannot fail -- it SPINS in syMallocSet's
    `while (TRUE);` -- so every `if (heap == NULL)` under it is dead code. July's fix gave the
    anim cache its own arena but left two loaders on the shared heap
    (reloc_backend_assets.c:3483 and :6535). A shield hit drives rebound into damage-fall into
    a new status, a common route to an uncached animation, which is why it read as random.
    Both ask ndsSyMallocWouldFit and decline now. Verified: 5-minute both-CPU soak, NO-FREEZE.

-Shield VFX is not correct.
    **PARTLY FIXED** (2026-08-02), confirmed against the extracted N64 asset, not just code.
    Shield is reloc file 0xa3: display list at 0x248, DObjDesc at 0x300.
      ALPHA -- `gDPSetPrimColor ffffffc0` is in the SHIPPED N64 data. The port had 0x60 centre
      and 0x50 rim, up to 2.4x too transparent. All ten values now 0xC0.
      SHAPE -- gSPVertex loads FOUR vertices, so the source shield is a textured billboard quad
      and not a sphere. The port's flat disc is structurally faithful; no sphere is owed.
      GLINT -- your capture puts it top middle at Mario's ear; the patch was upper left. The
      five shield templates now pass +40. Reflector and respawn pad pass 0 and stay
      byte-identical, since there is no reference for those two.
    NOT FIXED and dominant: the RIM. The source texture fades to zero alpha over its last few
    texels; BuildDisc emits 8 flat segments and is visibly faceted.
    I WITHDRAW an earlier claim here -- "only a texture can make a soft edge" is wrong.
    ndsRendererHardwareAlpha (nds_renderer.c:8054) takes polygon alpha from each triangle's
    FIRST vertex, so concentric rings with descending v0 alpha would fade the rim with no
    texture at all. The current disc cannot use it because v0 is always the centre.
    BLOCKED ON HEAP, not on the idea: the ring needs +8 vertices and +6 commands, and
    NDSVisualTemplate is a fixed 16/12 shared by all 14 templates, so raising both costs 2,464
    bytes. The soak put gNdsTaskmanGeneralHeapFreeMin at 24,404 -- ALREADY UNDER the 25,600
    ifCommonSetMaxNumGObj threshold, so the cap may well have latched during the battle already.
    (sGCCommonsMaxNum reading -1 does NOT clear that: it is not sticky across a scene change and
    the soak samples it in Results.) There is no margin to spend here. Splitting the template
    array so only the seven disc templates pay costs ~1,008; that is the route if revisited.
    THE ATLAS ROUTE IS CLOSED AND MEASURED: 128x128 uploaded fine but broke everything
    allocating after it (Violation 0 -> 1, StagePrepareBuild 2 -> 244, Results scoreboard
    gone). A retry must solve CONTIGUITY, not byte count. The shield is not on that sheet.
