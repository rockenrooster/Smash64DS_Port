AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.
These bugs should be fixed for P1 delivery:

-Whispy blow VFX not correct and not at correct location.
  Particle draw now applies the source LBTransform; it drew at the script-local origin. Owner look pending. Owner: still not right, spawning too far away from Whispy the Tree.
  Source puts the dust at x -715 or -205 either side of Whispy at -525 (ground.h:19), picked by
  gGRCommonStruct.pupupu.lr_players. Those two differ by 510 units, so an inverted lr_players is the
  cheapest explanation of "too far" -- read lr_players and dust_xf->translate at the spawn before
  suspecting the tree's own placement. The leaves are a separate effect 700 units behind the play
  plane (z -696/-762) and would also read as displaced if that is what is being seen.

-Some Crowd noise audio cues get cut off.
  NOT a stolen channel: measured 0 GenerationMismatch, 0 PoolExhaust, 6 of 8 handles peak, and all 174
  stops were DURATION. Six of the thirteen crowd cues instead PLAY TOO FAST and so end early --
  ds_frequency_hz against the rate their own source duration implies: Cheer 1.61x (2185 ms of cue in
  1360 ms), DamageL 1.52x, Amazed 1.33x, GaspS 1.26x, GaspClap and DamageS ~1.2x; GaspL/Fox/Mario/Win/
  Excited are 1.00x and sound right. All six short ones share one 44,800-sample wave
  (trim_strategy untrimmed_shared_source_reuse, trim_applied false), so nothing is being truncated --
  the rate is wrong. Next: check ds_frequency_hz for that shared sound against the N64 program.

-Respawn floating platform isn't visible when respawning.
  Rebirth moved to the DS visual seam: the battle hardware path submits no source effect DL links.

-Stray VFX are getting played across the stage when attacks are landed.
  Same owner as Whispy: untransformed particles drew at the world origin, not on the victim.

-The rolling dodge sound (escape roll?) sounds off, maybe too loud???
  nSYAudioVoiceFoxEscape is the pack's loudest cue: peak DS volume 106 against a median 81.
  Owner: still doesn't sound right. Check Source.
  Checked: its own gain is source-exact. ucd_volume 220 is asserted against the original set_volume, and
  the AOT render divides the PCM by channel_volume and hands that same value to the hardware, so the
  product is exactly source x gain/32767. If it is still wrong the divergence is the BGM-vs-FGM master
  balance, not this cue. It also renders at 16 kHz where most cues are 32 kHz, which is the other thing
  that would read as "off" rather than "loud".

-**FIXED** (2026-08-01) the KO burst freezes the game.  

-Correct VFX isn't played for various things (running foot dust VFX, fireball hit VFX, fox down B, shield, hard landing vfx, etc)
  Motion-script effects now reach their source makers; 1.3% of particle frames still miss the atlas.

-Upwards KO boundary death: correct VFX and SFX never play for fighters
  Cues packed, sparkle source-routed; the transform fix should now place it on the fighter.

-KO VFX wrong.
  Burst builds its full tree and particle now, drop mask 0. Appearance needs an owner look.
  Owner: I can kind of see that its trying to play the effect but it gets clipped or something so I can't see it fully. Check Source.
  Not clipping: QUAD_KO_CELL_MAX halves KO textures to 8x8, and several of the burst's fourteen child scripts are unadmitted, so it draws in pieces. Atlas row below.

-Results confetti doesn't look right
  Confetti asks for GENLINK(3), which the broken macro sent out of bounds. Fixed with the burst.
  Owner: not Fixed. Confetti pieces do not look like there are large enough and don't look like they are falling freely. Check Source.
  Confetti's four child scripts are outside QUAD_MEASURED_LIVE, so their textures lose the greedy admission. Atlas row below.

-SHARED OWNER for the three rows above, found 2026-08-01. The particle atlas is
  64x64 / 8,192 bytes and admits 14 of 47 textures at 8x8-16x16 cells. Routing
  the source effects took the count a match actually draws from about five to
  TWENTY-FOUR, and fourteen of those draw nothing at all:
  1, 5, 11, 14, 15, 17, 25, 28, 29, 33, 34, 37, 38, 45. That is the 1,343
  missed quads per match. QUAD_MEASURED_LIVE in generate_nds_particle_banks.py
  is the admission priority and was measured BEFORE the routing, so thirteen of
  the newly-drawn textures are not even candidates. Re-deriving it from the new
  gNdsParticleTextureUseMask (996076583 / 8294) only reshuffles a fixed budget;
  the real question is whether the sheet can exceed 8,192 bytes, which PORTING
  records failing at 16,384 and 32,768 because a larger resident atlas made
  ndsRendererHardwareResolveStageSourceFrameTexture fail and forced 197 stage
  rebuilds a match. OWNER DECISION: VRAM for particle fidelity. Not started.

