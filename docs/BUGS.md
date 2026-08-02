AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.
These bugs should be fixed for P1 delivery:

-Whispy blow VFX not correct and not at correct location.
  Owner: still not right, spawning too far away from Whispy the Tree.
  MEASURED: both emitters source-exact and match the owner's N64 reference; dust y=100, leaves y=450 z=-696.

-Some Crowd noise audio cues get cut off.
  OWNER-QUEUED: release ramp replaces the mid-waveform soundKill; 486 ramp steps measured.

-**FIXED** (2026-08-02) Respawn floating platform isn't visible when respawning.
  It lived 8 frames against the source's 390; alive at +24 now. Growth zeroed so it holds size.

-Stray VFX are getting played across the stage when attacks are landed.
  MEASURED: 17 hit sparks over a full match, |x| max 1344 -- all on the stage. Not the spark position.

-The rolling dodge sound (escape roll?) sounds off, maybe too loud???
  Owner: still doesn't sound right. Check Source.
  OWNER-QUEUED: gain law, master gain and modulator 92 (shape 3) all source-exact and modelled. Needs your ear.

-**FIXED** (2026-08-01) the KO burst freezes the game.

-Correct VFX isn't played for various things (running foot dust VFX, fireball hit VFX, fox down B, shield, hard landing vfx, etc)
  MEASURED: dust scripts now packed, rejects 49->0, script starts 137->226; other kinds unverified.

-**FIXED** (2026-08-01) Upwards KO boundary death: correct VFX and SFX never play for fighters.
  ftPhysicsStopVelAll never cleared vel_damage_air; sparkle 79222 -> 2399.99 against a 2400 target.

-KO VFX wrong.
  Owner: I can kind of see that its trying to play the effect but it gets clipped or something so I can't see it fully. Check Source.
  **FIXED** (2026-08-02) all 8 burst scripts take size from the header, so the particle half was denormal-invisible. Now 12 live at 713.

-Results confetti doesn't look right
  Owner: not Fixed. Confetti pieces do not look like there are large enough and don't look like they are falling freely. Check Source.
  MEASURED: header `size` was never byte-swapped, so 20.0 read as a 5.7e-41 denormal. Fixed; needs owner.

-BLOCKED(decision: particle atlas byte budget). Shared bound behind the three rows above.
  The sheet is 8,192 bytes because 16,384, 32,768 and a second page each broke stage
  texture resolves with VRAM free. A5I3 doubled its texels inside that same allocation and
  took admission 14 -> 23 of 47 and misses 1,343 -> 528, but the eight textures still out
  are long animations (28 is 20 frames, 25 is 15, 17 is 10 at 64x64) and no packing wins
  them inside 8,192. Options and measured costs in `docs/optimization/OPTIMIZATION_IDEAS.md`.
