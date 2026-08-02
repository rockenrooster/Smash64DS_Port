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
  RETRACTED 2026-08-02, both halves, within the hour of publishing them. A "root cause" was
  committed here claiming (a) the wind is drawn 2,238 units off by a recycled transform and (b)
  the port never reference-counts transforms. (b) is simply false and (a) is an over-read of my
  own instrument. Neither should be relied on.
  (b) WHY IT WAS WRONG: `rg users_num src/` returns nothing, so the port looked like it had
  dropped decomp's refcount. It has not. battleship_lbparticle.c:133 `#include`s
  decomp/.../lb/lbparticle.c textually -- that is the port's whole interposition pattern, with
  #define/#undef around it -- so all ten users_num sites compile into battleship_lbparticle.o,
  which is the only particle TU in the build. Greping `src/` for behaviour that lives in an
  included decomp file will keep producing this answer; check the #include list first.
  (a) WHY IT WAS WRONG: the probe reads `sLBParticleStructsAllocLinks[1]->xf`, and that is a LIST
  HEAD, not a population. xf_seen=554 counts 554 SAMPLES OF THE HEAD across frames -- possibly a
  different particle each frame, and not necessarily Whispy's dust at all, since slot 1 is
  whatever GENLINK(0) put there. "All 554 Whispy particles hold a stale transform" does not
  follow from it. The arithmetic built on top (drawn at 2656, off past the 3148 half-width) is
  therefore unsupported too.
  WHAT ACTUALLY SURVIVES, and it is still worth having: at the maker, grPupupuWhispyDustMakeEffect
  sets xf->translate to dGRPupupuWhispyDustEffectPositions[lr] and the probe reads exactly
  (-715, 100) with rotate.y 180deg, so creation is source-exact. At draw time SOME slot-1 particle
  holds a transform reading (2238.58, 134.93) -- the same value an earlier probe saw in 653 of 900
  dust_xf samples, so it is reproducible and it is not a Whispy position. Transform pressure is
  NOT the explanation: the pool is 6 and the run used 1-2, nowhere near saturation.
  THE WALK WAS RUN, and it refutes the transform hypothesis outright. Iterating the whole slot-1
  list every frame instead of reading its head:
      walked=5661  xf_seen=5661  xf_null=0  at_emitter=3085  elsewhere=2576
      xf_t=2238.583008,134.929138  gen=168
  3,085 of 5,661 slot-1 particle-visits hold a transform sitting exactly at the emitter's -715 --
  Whispy's own, correctly placed. The other 2,576 belong to generator 168 at (2238.58, 134.93).
  Slot 1 simply carries TWO effects, and the head read had been sampling the other one. Nothing is
  dangling: xf_null is 0 across all 5,661 visits, and more than half are correct.
  So Whispy's particles are created, alive, sized 260, inside a 3,148 half-width camera, textured
  (absent from the quad-miss mask), dispatched (draw_masks bit 1 set), AND correctly transformed
  -- and still nothing is visible. Four mechanisms have now been proposed and all four refuted by
  measurement: camera frustum, layer-15 dispatch, atlas exclusion, stale transform.
  NO CAUSE IS ESTABLISHED. Do not accept a fifth mechanism on this row without a measurement that
  could have refuted it, and do not re-test the four above -- each is refuted here with numbers.
  A REAL SECONDARY DEFECT FOUND IN THE SUBMIT, stated as arithmetic and explicitly NOT claimed as
  the root cause. ndsRendererParticleWorldToV16 (nds_renderer.c:11138-11146) scales a world
  coordinate by 1 << (12 - NDS_RENDERER_HW_WORLD_UNIT_SHIFT) = 16 and clamps to the v16 range, so
  THE RENDERER CANNOT EXPRESS A PARTICLE VERTEX BEYOND +/-2047.9 WORLD UNITS -- while the camera
  measured on the same frame sees +/-3148. Anything a particle does between 2048 and 3148 is
  silently clamped onto the rail instead of drawn where it is.
  Whispy is the effect that reaches it. Its dust carries rotate.y = 180deg, so a particle's drawn
  x is -(pc->pos.x) - 715; at the measured slot1_absmax of 1352.46 that is -2067.5, past the rail,
  and each quad corner extends a further +/-260 of `size` to about -2327. The far tail of the wind
  therefore provably clamps, and a clamped corner collapses the quad.
  BUT IT IS ONLY THE TAIL, so it cannot be the whole row: a particle at pos.x 417.69 draws at
  -1132.7, comfortably representable, and those are invisible too. Fix it anyway -- it is a real
  bound violation that will bite any effect crossing the stage, and it is cheap -- but do not
  close row 1 on it, and re-measure afterwards rather than assuming.
  STILL UNTESTED, and the only step left between a representable world position and a pixel: what
  the GPU does with these quads once submitted. Instrument INSIDE the submit for final screen-space
  coordinates and survival to the FIFO, rather than inferring from upstream state again -- that
  inference is what produced all four wrong answers above.

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
  0x0 -- so the callback is running somewhere r0 is not the argument.
  THREE FIXES TRIED, ALL STILL READ 0.0, and the instrument is not to be trusted until this is
  understood. (1) Break on ndsBaseEFManagerDamageNormalLightMakeEffect instead: this resolved the
  split -- the ROM carries both the plain name at 0x0209384c and the ndsBase implementation at
  0x020919d0, which is decomp's efmanager.c:1766 reached through the #define at
  battleship_efmanager.c:136 -- and gdb now sets ONE breakpoint at a real address. Still 0.0.
  (2) Read through $r0 rather than `pos`, correct in principle since pos is argument one. Still
  0.0. (3) `break *FUNC` with the star, to stop at the exact entry before the prologue can reuse
  r0. Still 0.0, and gdb reports the same address either way.
  (4) FIXED THE INSTRUMENT, AND THE ANSWER IS THAT THE POSITION IS REALLY ZERO. Breaking at the
  caller instead -- battleship_efmanager.c:1379, where `pos` is a named parameter in a TU with its
  own debug info -- resolves to ONE address (0x209384c) and reads cleanly.
      spark_calls=6  spark_x=0.0  spark_y=0.0  spark_absmax=0.0
      spark_player=1  spark_size=17  spark_ptr=022a8b74
  WITH A CONTROL, because four zeroes and four failed reads look identical: `player` and `size`
  are siblings of `pos` in the same frame and read 1 and 17 -- live, sensible, and matching
  hitlog->attacker_player and ft_attack_coll->damage. So the frame IS readable. And spark_ptr is
  022a8b74, a valid EWRAM address, so `pos` is NOT NULL -- it is a real Vec3f containing (0,0,0).
  ROW 4's SYMPTOM IS EXPLAINED: every hit spark is spawned at the world origin, stage centre,
  regardless of where the attack connected. That is "stray VFX played across the stage when
  attacks are landed" -- the sparks are not scattered, they are all in the same wrong place while
  the fight happens elsewhere. It also retires the earlier "17 sparks at |x| <= 1344" reading,
  which came from a different build.
  WHERE IT COMES FROM, not yet isolated: ftmain.c:2711 fills that Vec3f with
  gmCollisionGetFighterAttackDamagePosition (gmcollision.c:1967), which composes three steps --
  gmCollisionGetFighterAttackPosition, then gmCollisionGetWorldPosition(parts->mtx_translate, ...)
  for the damage side, then gmCollisionGetCommonImpactPosition to blend them. The middle step
  reads a fighter PART MATRIX, which this port maintains through its own matrix pipeline, so that
  is the natural suspect -- but it is a suspect, not a finding, and four mechanisms were published
  and refuted on row 1 today for exactly this kind of reasoning.
  NEXT, one measurement: break at gmcollision.c:1979 and latch attack_pos and damage_pos
  SEPARATELY. Whichever is zero names the step; if both are sane the fault is in the blend.

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
