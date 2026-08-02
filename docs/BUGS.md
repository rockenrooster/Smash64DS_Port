AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.
These bugs should be fixed for P1 delivery:

-Whispy blow VFX not correct and not at correct location.
  Owner: still not right, spawning too far away from Whispy the Tree.
  ROOT CAUSE FOUND, not the effect: WHISPY HAS NO FACE. Both emitters measure source-exact
  (-715/-205 at y=100, leaves y=450 z=-696, each with the matching rotate.y) and agree with the
  owner's own N64 capture. But map_gobj[0] (eyes) and map_gobj[1] (mouth) carry dl=0 AND mobj=0
  across their root and every child/sibling -- built, positioned and animated by grPupupuProcUpdate,
  with no geometry and nothing for whispy_eyes_texture/whispy_mouth_texture to drive. Three
  identical trunks and no anchor saying which one is Whispy, so correct dust beside one of them
  reads as coming from nowhere. Stage-geometry work, not effect work. (Walk covered root + one
  level; geometry deeper than that would not have been seen.)
  Localized for whoever takes it: grpupupu.c:666-667 builds both from relocData descs --
  llGRPupupuMapWhispyEyesTransformKindsDObjDesc/MObjSub and the Mouth pair, declared as file
  offsets in include/reloc_data.h:466-470 (0x0f00/0x10f0/0x13b0/0x1770 in the Dream Land map).
  grPupupuMakeMapGObj is linked and runs; the descs are offsets the port resolves at runtime, so
  the question is whether that resolve returns them and whether the DS submits the resulting tree.

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
  OWNER-QUEUED: gain law, master gain and modulator 92 (shape 3) all source-exact and modelled. Needs your ear.

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
