AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.
These bugs should be fixed for P1 delivery:

-Whispy blow VFX not correct and not at correct location.
  Owner: still not right, spawning too far away from Whispy the Tree.
  MEASURED: both emitters source-exact and match the owner's N64 reference; dust y=100, leaves y=450 z=-696.

-Some Crowd noise audio cues get cut off.
  OWNER-QUEUED: release ramp replaces the mid-waveform soundKill; 486 ramp steps measured.

-Respawn floating platform isn't visible when respawning.
  LOCALIZED: halo GObj is created and non-NULL; unproven that the DS seam submits its geometry.

-Stray VFX are getting played across the stage when attacks are landed.
  CONTRACT needed: transform fix landed, no measurement yet that hit sparks land on the victim.

-The rolling dodge sound (escape roll?) sounds off, maybe too loud???
  Owner: still doesn't sound right. Check Source.
  LOCALIZED: gain law is source-exact (0.947 applied). Open dimension is its volume LFO: modulator 92, shape 3, amp 110.

-**FIXED** (2026-08-01) the KO burst freezes the game.

-Correct VFX isn't played for various things (running foot dust VFX, fireball hit VFX, fox down B, shield, hard landing vfx, etc)
  MEASURED: dust scripts now packed, rejects 49->0, script starts 137->226; other kinds unverified.

-Upwards KO boundary death: correct VFX and SFX never play for fighters
  LOCALIZED: star path runs and plays FGM 12; unmeasured whether the sparkle reaches the screen.

-KO VFX wrong.
  Owner: I can kind of see that its trying to play the effect but it gets clipped or something so I can't see it fully. Check Source.
  MEASURED: burst is whole, drop mask 0; its cells are 8x8 and some frames are unadmitted.

-Results confetti doesn't look right
  Owner: not Fixed. Confetti pieces do not look like there are large enough and don't look like they are falling freely. Check Source.
  MEASURED: header `size` was never byte-swapped, so 20.0 read as a 5.7e-41 denormal. Fixed; needs owner.

-BLOCKED(decision: particle atlas byte budget). Shared bound behind the three rows above.
  The sheet is 8,192 bytes because 16,384, 32,768 and a second page each broke stage
  texture resolves with VRAM free. A5I3 doubled its texels inside that same allocation and
  took admission 14 -> 23 of 47 and misses 1,343 -> 528, but the eight textures still out
  are long animations (28 is 20 frames, 25 is 15, 17 is 10 at 64x64) and no packing wins
  them inside 8,192. Options and measured costs in `docs/optimization/OPTIMIZATION_IDEAS.md`.
