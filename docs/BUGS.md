AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.
These bugs should be fixed for P1 delivery:

-Whispy blow VFX not correct and not at correct location.
  Particle draw now applies the source LBTransform; it drew at the script-local origin. Owner look pending. Owner: still not right, spawning too far away from Whispy the Tree.

-Some Crowd noise audio cues get cut off.
  Crowd actor live, every cue packed, miss ring empty. Truncation cause not yet measured.

-Respawn floating platform isn't visible when respawning.
  Rebirth moved to the DS visual seam: the battle hardware path submits no source effect DL links.

-Stray VFX are getting played across the stage when attacks are landed.
  Same owner as Whispy: untransformed particles drew at the world origin, not on the victim.

-The rolling dodge sound (escape roll?) sounds off, maybe too loud???
  nSYAudioVoiceFoxEscape is the pack's loudest cue: peak DS volume 106 against a median 81.

-**FIXED** (2026-08-01) the KO burst freezes the game.  

-Correct VFX isn't played for various things (running foot dust VFX, fireball hit VFX, fox down B, shield, hard landing vfx, etc)
  Motion-script effects now reach their source makers; 1.3% of particle frames still miss the atlas.

-Upwards KO boundary death: correct VFX and SFX never play for fighters
  Cues packed, sparkle source-routed; the transform fix should now place it on the fighter.

-KO VFX wrong.
  Burst builds its full tree and particle now, drop mask 0. Appearance needs an owner look.
  Owner: I can kind of see that its trying to play the effect but it gets clipped or something so I can't see it fully.

-Results confetti doesn't look right
  Confetti asks for GENLINK(3), which the broken macro sent out of bounds. Fixed with the burst.

