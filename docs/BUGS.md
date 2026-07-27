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
  13 NDSVisualEffectKind values (nds_effects.h:9-25) share a small set of
  template slots (battleship_efmanager.c) built from only FOUR untextured
  16-vertex primitives: BuildDust, BuildStar, BuildRing, BuildDisc.
  This splits the row in two:
  (a) A real defect -- before this pass, five kinds rendered as a *different
      effect*: Coin->Sparkle, Catch->ImpactWave, Slash->HitNormal,
      Rebirth->Death, and Reflector->Shield. "fox down B" is
      that last one: efManagerFoxReflectorMakeEffect asks for
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
  (b) Running foot dust, fireball hit, and hard landing each DO have their own
      template (Dust, Fire, Dust). They look wrong because a recolored
      16-vertex primitive is standing in for a textured particle script.
      This is P1, not P2 -- the contract is that this exact match is identical
      to the full game with these settings, so the real scripts have to run.
      Sized 2026-07-27 and it fits: efmanager reaches 26 of 119 efcommon
      scripts naming 18 of 47 textures = 129,768 B, plus 4,896 B for Dream
      Land's grpupupu bank, against 210,320 B measured arena headroom. The
      banks are position-independent so no relocData tooling is needed.
      OPEN: port lb/lbparticle.c (2,961 lines), ef/efparticle.c,
      ef/efdisplay.c, add a DS pack step, and draw textured quads.
      See KNOWN_ISSUES.md for the full measurement.
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
-mario underside area geometry missing
  Reproduced and localized 2026-07-27 from artifacts/visibility/latest.png:
  Dream Land's grass and a flower show THROUGH Mario's lower torso, in the
  band where the overalls meet the legs. It is a real hole, not a shading
  artifact -- the pixels are stage background, not Mario's interior.
  Mechanism narrowed, cause not yet found:
  - The fighter DLs run with geometry mode 0x222005 =
    G_ZBUFFER|G_SHADE|G_CULL_BACK|G_LIGHTING|G_SHADING_SMOOTH (read off
    ftrContract=.../geom0x222005 in the Boundary run). G_CULL_BACK is
    therefore active for Mario.
  - ndsRendererHardwarePolyFmt (nds_renderer.c:7658) handles that correctly:
    it starts at POLY_CULL_NONE and clears POLY_CULL_FRONT when the source
    asks for CULL_BACK, leaving front-faces-only. So culling is faithful.
  - With back faces legitimately culled, seeing background means the
    FRONT-facing geometry that should cap that junction is never submitted.
    The renderer reports no drops (rej=0, drop0) and we are far under the DS
    per-frame limits (vtx 2484 / tri 828 of 6144 / 2048), so it is not
    clipping or overflow.
  TESTED and REFUTED -- it is not inverted culling. A lab probe forced
  POLY_CULL_NONE for every polygon, ignoring the source cull bits, and the
  hole stayed exactly where it was: same position, same size, in the same
  crop of the same capture point. If the front/back convention were flipped,
  rendering both faces would have filled it with at least the interior back
  surface. It did not, so nothing is being rasterized there at all. Probe
  removed after answering; the A/B captures are the evidence.
  NEXT: find which DObj/DL is absent. The harness pins fighters at 313
  triangles per owner per frame; if that contract was pinned against an
  already-incomplete model, the count cannot flag the loss. Compare the
  submitted per-DObj triangle counts against Mario's source model DL, and
  check the DObjDLLink list_id / detail level the port selects -- SSB64
  carries up to 4 DL lists per DObj and picking a lower-detail list would
  drop exactly this kind of cap.