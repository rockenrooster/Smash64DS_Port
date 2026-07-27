AI Agent should mark fixed items with FIXED prefix
-Up B goes through main stage when underneath it.
-grab attacks snap player positions to wrong locations.
-Wind hazard not working, (SFX, VFX, gameplay effects)
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