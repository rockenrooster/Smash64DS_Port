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
  hand. It now rebuilds and reads the joint's world matrix the way the decomp's
  non-US branch does (func_ovl2_800EDBA4 + parts->mtx_translate). Needs a
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
  Root cause, measured: every named effect does spawn -- the verifier reports
  178/178 Mario/Fox motion calls with bounded DS presentation -- but the
  original particle scripts never run. lbParticleMakeScriptID is a stub
  (reloc_backend_compat_shims.c:12963) and the common particle script/texture
  banks are not resident, so nothing here is textured. In their place,
  13 NDSVisualEffectKind values (nds_effects.h:9-25) collapse onto 12 template
  slots (battleship_efmanager.c:212-226) built from only FOUR untextured
  16-vertex primitives: BuildDust (:301), BuildStar (:269), BuildRing (:333),
  BuildDisc (:373).
  This splits the row in two:
  (a) A real defect -- five kinds render as a *different effect*. The switch at
      battleship_efmanager.c:469-501 maps Coin->Sparkle, Catch->ImpactWave,
      Slash->HitNormal, Rebirth->Death, and Reflector->Shield. "fox down B" is
      that last one: efManagerFoxReflectorMakeEffect (:883) asks for
      nNDSVisualEffectReflector and gets the shield disc, white->red -- P1
      Mario's shield colors on Fox's reflector. The port also never reads
      effect_vars.reflector.status, which ftfoxspeciallw.c:29 sets every frame,
      so the reflector cannot react to its own state either.
      FIXED for the two collapses a Mario-vs-Fox match actually shows:
      Reflector and Rebirth now have their own template slots. Fox's down B
      draws a blue barrier disc instead of Mario's red shield. The hues reuse
      pairs already in the file, so this is an approximation and the owner's
      eye is the gate. Coin/Catch/Slash still share slots -- coins need items
      (off for P1) and neither of the other two was reported, so they stay
      collapsed rather than getting invented colors.
  (b) Fidelity debt -- running foot dust, fireball hit, and hard landing each
      DO have their own template (Dust, Fire, Dust). They look wrong because a
      recolored 16-vertex primitive is standing in for a textured particle
      script. Closing that needs the particle banks ported, which is P2 in
      KNOWN_ISSUES.md:42-47. Not fixable in this pass; the owner is the visual
      oracle for whether the stand-ins are acceptable for P1.
FIXED -Missing SFX, Mario down B, fox up b voice
  Cause: nSYAudioVoiceMarioSpecialLw (432) and nSYAudioVoiceFoxSpecialHi (362)
  were never packed, so both motion-script requests failed closed. Added both.
  Needs an ear check.
FIXED -Fox face never changes expression once hit in a match.
  Cause: ftParamResetTexturePartAll only rewound the FTStruct mirror and left
  the MObj showing whatever the last ftParamSetTexturePartID wrote, so the
  first damage face a fighter took became permanent. ftMainSetStatus calls the
  reset on every status change that lacks FTSTATUS_PRESERVE_TEXTUREPART, and
  the model never heard about it. It now writes the base id back through the
  joint's MObj chain the way the source does (ftparam.c:1147-1189), using the
  container/detail guards ftParamSetTexturePartID already applies. Needs a
  play test.
-Upwards KO boundary:  VFX and SFX never play for fighters
  SFX half FIXED: the star-KO path asks for nSYAudioFGMDeadUpStar (12) and the
  per-fighter deadup_sfx (Fox 360 / Mario 433); none of the three was packed.
  All three added.
  VFX half: same root cause as the VFX row above. efManagerSparkleWhiteDead
  spawns nNDSVisualEffectSparkle at scale 5.0 and efManagerDeadExplode spawns
  nNDSVisualEffectDeath, so something does draw -- a white star and a red ring.
  The original star-KO is a textured particle script that is not resident.
-KO VFX wrong.
  Partly FIXED. nNDSVisualEffectDeath and nNDSVisualEffectRebirth shared one
  template, a red->white ring, so the KO burst and the respawn flash were the
  same effect. Rebirth now has its own white/cyan ring and Death keeps the red
  one. What remains is the untextured-primitive gap: the original KO burst is a
  particle script, and a ring is the stand-in. That half is P2.