AI Agent should mark fixed items with FIXED prefix
FIXED -Up B goes through main stage when underneath it.
  Cause: two layers. The port's stand-in for the source's
  mpCommonRunFighterSpecialCollisions -- the runner every special / project /
  pass / landing entry point uses -- never ran the L/R wall tests and never
  called mpProcessRunCeilEdgeAdjust. Without the wall pass coll_data->mask_unk
  never carries MAP_FLAG_LWALL/RWALL, which is exactly what the ceiling test's
  fallback branches read to catch a rise that enters the platform past the end
  of the ceiling segment. Underneath that, mpProcessRun{L,R}WallCollisionAdjNew
  and mpProcessRunCeilEdgeAdjust were weak no-op bridges in the shipping link,
  because mpprocess.c was compiled as a private check and never linked.
  Fix: restored the missing calls and graduated mpprocess live
  (NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE now defaults to 1), so the real source
  adjusters run. Latest profile green; needs a play test.
FIXED -grab attacks snap player positions to wrong locations.
  Cause: func_ovl0_800C9A38 returned identity plus the joint's *local*
  translate, so ftCommonCapturePulledRotateScale placed the victim at the
  capturer's hand offset measured from the world origin instead of from the
  hand. It now composes the joint's world matrix up the DObj chain. Needs a
  play test.
-Wind hazard not working, (SFX, VFX, gameplay effects)
  Gameplay FIXED: ftParamSetVelPush was a counter-only stub that dropped the
  push vector on the floor, so Whispy's gust had no effect at all. It now does
  the source assignment (ftparam.c:526-531); the consumer chain was already
  live (mpprocess.c:450 adds coll_data.vel_push to the translation,
  ftmain.c:1571 clears it each frame).
  SFX FIXED with a fidelity note: nSYAudioFGMPupupuWhispyWind (285) was never
  packed. Added, but its source loop is not reproduced on DS, so the gust
  sounds once (~0.88 s) instead of sustaining the full 470-tick blow.
  VFX still open: the same particle-bank gap as the other VFX rows below --
  lbParticleMakeScriptID is a skipped stub.
FIXED -Missing SFX 2nd jump sound (double jump) not playing (1st jump sound plays, but not 2nd jump sound)
  Cause: the pack carried nSYAudioVoiceMarioJump (435, the grounded jump) but
  not nSYAudioVoiceMarioJumpAerial (436), which is what
  202_MarioMainMotion.c:118 asks for. Added 436 to the FGM pack. Needs an ear
  check.
-some VFX are wrong/don't look right, running foot dust VFX, fireball hit VFX, fox down B, hard landing vfx
FIXED -Missing SFX, Mario down B, fox up b voice
  Cause: nSYAudioVoiceMarioSpecialLw (432) and nSYAudioVoiceFoxSpecialHi (362)
  were never packed, so both motion-script requests failed closed. Added both.
  Needs an ear check.
-Fox face never changes expression once hit in a match.
-Upwards KO boundary:  VFX and SFX never play for fighters
  SFX half FIXED: the star-KO path asks for nSYAudioFGMDeadUpStar (12) and the
  per-fighter deadup_sfx (Fox 360 / Mario 433); none of the three was packed.
  All three added. VFX half still open.
-KO VFX wrong.