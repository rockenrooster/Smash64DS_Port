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
  RE-OBSERVED 2026-08-02 on the current ROM (build-r2-bothcpu tickhud, 12:20 AM, newer than the
  last source change). I stopped asserting and looked:
    artifacts/visibility/atlas-2026-08-02/whispy-trunk-zoom.png     (the trunk, 4x)
    artifacts/visibility/atlas-2026-08-02/whispy-topscreen-2x.png   (whole top screen, 2x)
  THE FACE IS THERE, visibly: two eyes, the snout, the mouth, on the left trunk. That is now
  settled by an image rather than by a counter, and it agrees with the PrepareBuildCount reading.
  The mouth faces LEFT toward stage centre and the dust emitter sits at world -715, 190 units left
  of Whispy's -525 -- i.e. directly in front of the mouth, which is what the owner expected.
  Emitters re-confirmed source-exact this run: whispy_x=-715 y=100 rotY=3.141593 (180 deg, lr=0),
  leaf_x=-715, dust_frames=3 leaf_frames=3 over 3 forced wind cycles.
  WHAT IS STILL WRONG, and this is a REOPEN, not a settle: no wind particle is visible anywhere in
  a capture taken on a forced-blow frame, while the simulation reports slot1_frames=554 live
  particle-frames in Whispy's alloc-link slot at slot1_size=260 -- real size, not a denormal.
  So they exist, they are the right size, and nothing draws where the eye can find it.
  The atlas is NOT the cause and that is now proven, not assumed: gNdsParticleQuadMissMask reads
  22020000,00002106 = texture ids 17, 25, 29, 33, 34, 40 and 45, every one of them in row 11's
  excluded 13, with ZERO refusals outside that list -- and Whispy's dust and leaf textures are not
  among them. Their textures are admitted and drawable.
  OFF-SCREEN HYPOTHESIS REFUTED, same day, by the measurement it asked for. gGMCameraStruct reads
  target_dist=6704.98, fovy=38, viewport 300x220, so the visible half-width at the stage plane is
  6704.98 * tan(19deg) * 300/220 = 3,148 units against a furthest particle |x| of 1,352 -- inside
  the frustum by 1,796 units, and the emitter at -715 is nowhere near an edge. The wind is not
  blowing out of view. Recorded because it is worth exactly as much as a confirmation: nobody
  should spend another run on camera bounds or on particle velocity scaling.
  So the surviving contradiction is sharp and small. The wind particles are CREATED (3 dust and 3
  leaf makers over 3 forced blows), ALIVE (554 particle-frames in alloc-link slot 1), CORRECTLY
  SIZED (260, not a denormal), INSIDE THE CAMERA, and their TEXTURES ARE ADMITTED -- and still
  nothing appears. Every "is it there at all" question is now answered yes, which means the defect
  is in the draw itself.
  LAYER-DISPATCH HYPOTHESIS ALSO REFUTED, same run. efDisplayInitAll (efdisplay.c:79-98) builds
  four draw GObjs and gives each an alloc-link camera_mask -- links 0|2 on layer 18, LINK 1 ALONE
  ON LAYER 15, link 3 on 25, link 4 on 10 -- and lbParticleDrawTextures skips any link whose bit
  is clear in the GObj it was handed (battleship_lbparticle.c:1246). Since every effect repaired
  this session lives on layer 18 or 10 and Whispy is the only thing on 15, an unwalked layer 15
  would have explained the whole row. It is walked: ORing every camera_mask that reaches the draw
  gives draw_masks=0x1f over draw_calls=3600 (4 per frame x 900), so bits 0,1,2,3,4 all arrive and
  link 1 is iterated like the rest.
  THAT PUTS THE DEFECT DOWNSTREAM OF SUBMISSION. Link 1 is dispatched; its particles carry size
  260 so they pass the `pc->size == 0.0F` skip; their textures are admitted so ndsParticleQuadFrameFor
  returns non-NULL rather than taking the miss branch. Nothing between the loop head and
  ndsRendererSubmitParticleQuad can drop them except the transform step.
  SURVIVING LEAD, and it is not a new guess -- it is a defect already observed in this exact
  subsystem this session: ndsParticleTransformForDraw builds the quad basis from the particle's
  attached transform, and grPupupuFlowersFrontLoopEnd EJECTS that transform without nulling the
  pointer. An earlier Whispy probe read a dangling dust_xf for 653 of 900 samples because of it.
  The same run reports gLBParticleTransformsUsedNum=1 of 6 and a ground transform reading
  -1.9e20,-7.4e20 -- the shape of a freed slot, not a position. If Whispy's particles dereference
  an ejected transform their quad basis is garbage and they rasterize nowhere visible, which fits
  every surviving fact: created, alive, sized, in-camera, textured, dispatched, invisible.
  NEXT: latch the transform pointer each slot-1 particle draws with, and whether the slot it
  points at is still owned. Do NOT resume the legacy-DObj archaeology; it produced two withdrawn
  answers already, and do not re-test the camera or the layer -- both are refuted above with
  numbers.

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
  2026-08-02: THIS RUN PRODUCED NO USABLE SPARK DATA and the earlier reading still stands alone.
  The probe latched spark_calls=6 with every position exactly 0.0, which looks like "sparks spawn
  at the origin" and is not: `pos` is optimized out at -Os. Reading it through $r0 instead (it is
  the first parameter of efManagerDamageNormalLightMakeEffect(Vec3f *, s32, s32, sb32), so it is
  in r0 at entry) did NOT fix it, because gdb resolves that symbol to "2 locations" at address
  0x0 -- so the callback is running somewhere r0 is not the argument. Fix the breakpoint before
  trusting any number from it: break on a specific location, or on the call site in the caller.
  Until then this row has exactly one measurement behind it, the earlier 17 sparks at |x| <= 1344.

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

-BLOCKED(decision: particle atlas RESOLUTION, not VRAM). Owner's call. One approval unblocks it.
  CORRECTION: this row called itself a VRAM decision and offered "evict-and-swap or leave 0.6%
  of particles undrawn". Both framings were wrong. Every option below is the SAME 128x64 = 8,192
  byte allocation, so VRAM does not move and there is nothing to evict. The real axis is cell
  resolution against how many textures exist at all, and the generator's own docstring named the
  lever the whole time: "the honest answer for those is halving 64x64x10 rather than growing the
  atlas". Nobody had measured what halving actually buys.
  Measured by sweeping the two cell caps and re-running the real packer (quad_cell_dims snaps to
  powers of two, so cap 12 behaves as 8 and cap 6 as 4 -- only four settings are distinct):
    cell/long   admitted  excluded  frames   used  free
      16 / 8      23        13        53     7744   448   <- shipping today
      16 / 4      30         6       152     6928  1264
       8 / 8      29         7        86     6592  1600
       8 / 4      36         0       168     6032  2160   <- nothing missing
  Per-texture sizing beats every global cap: start at the 8/4 floor, then upgrade textures back
  toward 16x16 live-first while the set still packs. That admits ALL 36 and still keeps the four
  most-drawn live textures (0, 10, 18, 19) at full 16x16 plus one at 16x8, using 7,328 of 8,192.
  That is the recommended implementation, and it is strictly better than a uniform 8/4.
  Cheaper equivalents exhausted first, as PROJECT_GOAL requires: frame dedup across all 168
  candidate frames finds only 2 duplicates (1.2%) and zero empty frames, so sharing cells saves
  nothing. A second page and 16,384/32,768-byte sheets were already tried and broke stage resolves.
  Why it matters more than 0.6%: none of the 13 excluded textures is in QUAD_MEASURED_LIVE, so the
  greedy admission never had to weigh them -- but the runtime FAILS CLOSED on a missing texture,
  which is 528 draws a match rendering nothing. AGENTS.md forbids accepting "missing/corrupt
  presentation" outright, while a smaller particle is exactly the "cheapest acceptable
  source-derived approximation" it asks for. Particles are a few pixels on a 256x192 screen.
  CONFIRMED AT RUNTIME 2026-08-02, and it makes this row the sole cause rather than a suspect:
  gNdsParticleQuadMissMask on a live battle reads 22020000,00002106 = texture ids 17, 25, 29, 33,
  34, 40, 45 across 366 refused draws of 5,454 emits. Every one is in the excluded 13 and there
  are ZERO refusals outside it, so no second defect is hiding behind this one and admitting the
  set removes every missing particle draw in the game. Six of the 13 (28, 31, 35, 36, 38, 41) were
  never even requested in that run, so approving this repairs 7 textures now and insures 6 more.
  Look at the two renders before deciding -- they are the same 8,192 bytes, and the second holds
  roughly half an effect vocabulary the first does not have at all (the coloured notes and
  confetti are simply absent from baseline):
    artifacts/visibility/atlas-2026-08-02/atlas-baseline-23of36.png
    artifacts/visibility/atlas-2026-08-02/atlas-all-36-admitted.png
  NOT IMPLEMENTED: this changes how 8 textures look, and PROJECT_GOAL requires owner approval for
  a permanent visual-fidelity trade. Say go and it is a generator constant plus a repack.
