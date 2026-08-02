AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.
These bugs should be fixed for P1 delivery:

-Whispy blow VFX not correct and not at correct location.
  Owner: still not right, spawning too far away from Whispy the Tree.
  RETRACTED: "Whispy has no face" was wrong and is withdrawn. The empty DObj trees are BY DESIGN.
  What still stands: both emitters are source-exact -- dust (-715|-205, y=100), leaves y=450
  z=-696/-762, each with the rotate.y its lr_players demands -- and all of it agrees with the
  owner's N64 capture. The effect is not the defect.
  What was wrong: map_gobj[0]/[1] do carry dl=0 and mobj=0 (dl is a union over every display-list
  form, objtypes.h:429-438, so that read was sound), and their relocData descs at map_head+0x10f0
  and +0x1770 do read 0,0 -- GRPupupuMap on disk is 290 BYTES, so both offsets are far past its
  end. But this port does not draw the stage from those descs at all. The generated native stage
  owns it: NDS_NATIVE_STAGE_SEGMENT_COUNT is 8 and generate_nds_native_stage.py:4136 lists them as
  layer0, whispy_eyes, whispy_mouth, flowers_back, layer1, layer2, flowers_front, layer3. Segments
  1 and 2 ARE the face; they are indexed, not named, in the generated .inc, which is why grepping
  it for "whispy" finds nothing. The map_gobj GObjs exist to carry animation state for that path.
  SETTLED: THE FACE IS PRESENT. The map_gobj reading was of the wrong tree entirely.
  ndsRendererAdapterPrepareNativeStageMaterials (reloc_backend_renderer_dl.c:7417) resolves the
  stage's four materials from binding_dobjs[{20,22,31,32}] in the LAYER-0 tree -- nothing to do
  with map_gobj -- and bindings 20 and 22 are Whispy's eyes and mouth (generator material_snapshots
  slots 0 and 1). It returns FALSE on any NULL mobj or any flag word != its expected 0x0001, and
  that FALSE "rejects the whole native stage owner and drops the stage onto the generic path"
  (:7445). Measured: gNdsR2StagePrepareBuildCount 2, gNdsR2StagePrepareReuseCount 2041 -- it
  succeeded and stayed valid for the whole match, so both face materials resolved every frame, and
  CommitNativeStageMaterials (:7471-7490) writes their texture_id_curr/next from
  whispy_eyes_texture/whispy_mouth_texture on segments 1 and 2.
  So NO DEFECT SURVIVES on this row: emitters source-exact, face present and animating. Same
  standing as the stray-VFX row -- the report does not reproduce against any available
  measurement, and the ROM has changed a great deal since it was written (the denormal fix alone
  made a large fraction of particles visible for the first time). Needs re-observation.

-Some Crowd noise audio cues get cut off.
  OWNER-QUEUED: release ramp replaces the mid-waveform soundKill; 486 ramp steps measured.

-**FIXED** (2026-08-02) Respawn floating platform isn't visible when respawning.
  It lived 8 frames against the source's 390; alive at +24 now. Growth zeroed so it holds size.

-Stray VFX are getting played across the stage when attacks are landed.
  NO SURVIVING MECHANISM. Measured: 17 sparks/match, |x| max 1344, all on stage. Two candidate
  causes raised and both eliminated: (1) a denormal size draws in the RIGHT place, just sub-pixel,
  so it explains invisibility and never displacement; (2) the xf==NULL fallback to raw pc->pos is
  CORRECT -- the makers that take it (FlashMiddle, FuraSparkle, via lbParticleMakeCommon) assign
  pc->pos in world coordinates and attach no transform on purpose, which is exactly what that
  branch expects. Every maker that uses script-local coordinates does attach a transform and set
  xf->translate; that was checked across all twelve routed kinds.
  Most likely the report predates the denormal fix, which had many effects appearing and vanishing
  at once. Needs re-observation on the current ROM.

-The rolling dodge sound (escape roll?) sounds off, maybe too loud???
  Owner: still doesn't sound right. Check Source.
  OWNER-QUEUED: offline verification EXHAUSTED -- every mechanical dimension is source-exact and
  nothing measurable supports "too loud". Listen to the WAVs below; that is now the cheapest step.
  What the roll actually is (202_MarioMainMotion.c RollF/RollB, 208_FoxMainMotion.c RollB/TechB):
  Mario's roll plays ONE cue, FGM 11 nSYAudioFGMEscape. FOX'S ROLL PLAYS TWO -- FGM 11 plus voice
  364 nSYAudioVoiceFoxEscape, simultaneously. That is source behaviour, not a port defect, and it
  is the most likely reason a roll can read as louder than expected. Which fighter was rolling
  matters; the report says "escape roll?" so the cue was never pinned down.
  Verified source-exact, each against decomp: root UCD program is 3 sequential notes (12,13,12 for
  30+40+20 = 90 ticks = source_duration_ticks); articulation 54; modulator 92 (shape 3, target 10
  volume, period 11, amp 110, offset 50) modelled; modulator 93 (target 28) correctly skipped --
  24+ is cross-voice and FGM 11 has no forks, so there is no destination; modulator 94 (shape 7,
  target 12 pitch, period 90, amp 260, offset -350) IS modelled -- it ramps pitch -93 -> -350 cents
  across the cue, and _fgm_modulator_value does implement shape 7 via `if shape in (3, 7)`. The
  comment at render-audio-fgm-phase-pack.py:3660 saying shape 7 is unimplemented is STALE; it was
  written for FGM 85 where shape 7 sat on a cross-voice target and was skipped before evaluation.
  Volume envelope is a single constant point, so packed_envelope_count 0 is correct. Shipping
  ds_volume 127 over that point's 124 is the pack's normal baked-render convention, not an FGM 11
  quirk -- 46 of 88 entries differ the same way.
  Measured, not argued: decoded peak 23096 of 32767, zero clipped samples. Effective RMS ranks
  33rd of 88 cues, 1.19x the pack median -- mid-pack, not an outlier. Fox's two cues time-aligned
  and summed peak at 32768 with ONE hard-clipped sample in 16560 -- inaudible, but not the "30351,
  zero clipped" this row briefly claimed: that first figure summed a 32000 Hz cue against a
  16000 Hz one index-by-index, which plays the voice at double speed and misaligns the mix, so it
  was answering nothing. `export-fgm-cue-wav.py --sum` resamples to the fastest rate now.
  A decoder written independently of the pack reproduces decoded_peak for all 88 entries, so these
  WAVs are bit-accurate to what the hardware reconstructs. Regenerate with
  `python scripts/sfx/export-fgm-cue-wav.py 11 364 --volume --sum` (artifacts/ is gitignored):
    artifacts/audio/fgm11-nSYAudioFGMEscape-as-ds-plays-it.wav      (Mario's roll, the whole sound)
    artifacts/audio/fgm364-nSYAudioVoiceFoxEscape-as-ds-plays-it.wav
    artifacts/audio/fgm-11-364-summed.wav                           (Fox's roll, as DS sums it)
  This bisects the row without booting a ROM: wrong in the WAV means the render is wrong and it is
  mine to fix; right in the WAV but wrong in game means the defect is in the in-game mix level, and
  a deliberate DS-side attenuation is then an owner call under the sacrifice order.

-**FIXED** (2026-08-01) the KO burst freezes the game.

-**FIXED** (2026-08-02) Correct VFX isn't played for various things (running foot dust VFX, fireball hit VFX, fox down B, shield, hard landing vfx, etc)
  Two causes: four seams had no packed scripts (rejects 49->0, starts 137->226) and header sizes were denormal. Reject count 0 over a full match means nothing now asks for a script the pack lacks.

-**FIXED** (2026-08-01) Upwards KO boundary death: correct VFX and SFX never play for fighters.
  ftPhysicsStopVelAll never cleared vel_damage_air; sparkle 79222 -> 2399.99 against a 2400 target.

-KO VFX wrong.
  Owner: I can kind of see that its trying to play the effect but it gets clipped or something so I can't see it fully. Check Source.
  **FIXED** (2026-08-02) all 8 burst scripts take size from the header, so the particle half was denormal-invisible. Now 12 live at 713.

-**FIXED** (2026-08-02) Results confetti doesn't look right.
  Owner: not Fixed. Confetti pieces do not look like there are large enough and don't look like they are falling freely. Check Source.
  Header `size` was never byte-swapped: 20.0 read as a 5.7e-41 denormal. maxsize 0.000000 -> 20.000000, and the pieces are visible in the capture.

-BLOCKED(decision: particle atlas byte budget). Owner's call, not a defect.
  The sheet is 8,192 bytes because 16,384, 32,768 and a second page each broke stage
  texture resolves with VRAM free -- that bounds the ALLOCATION, not the texel count.
  A5I3 doubled the texels inside the same allocation: admission 14 -> 23 of 47, misses
  1,343 -> 528. THIRTEEN textures are still out (28 is 20 frames, 25 is 15, 17 and 29
  are 10) and no packing wins them inside 8,192; 448 of the 8,192 bytes are free and the
  smallest excluded cell is 512. So the choice is evict-and-swap or leave 0.6% of
  particles undrawn. Options and measured costs in `docs/optimization/OPTIMIZATION_IDEAS.md`.
