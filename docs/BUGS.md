AI Agent should mark fixed items with FIXED prefix

-Wind hazard not working, (SFX, VFX, gameplay effects)  [gameplay+SFX FIXED]
  Gameplay FIXED: ftParamSetVelPush was a counter-only stub that dropped the
  push vector on the floor, so Whispy's gust had no effect at all. It now does
  the source assignment (ftparam.c:526-531); the consumer chain was already
  live (mpprocess.c:450 adds coll_data.vel_push to the translation,
  ftmain.c:1571 clears it each frame).
  SFX FIXED: nSYAudioFGMPupupuWhispyWind (285) was never packed. Added, and
  then it still puffed once and stopped -- the sample is 0.88 s but the blow
  is 470 ticks x 5750 us = 2.70 s, so two thirds of the gust was silence.
  (An earlier note here said 7.8 s; that used the wrong tick rate.)
  Root cause, and it was not the runtime:
  - The DS side was already complete. ndsAudioFgmValidateCachedEntry accepts
    flags & ~1u == 0, so a looping entry already passed validation, and the
    play path already passed the loop bit and loop point through to
    soundPlaySample (nds_audio_fgm.c:1158-1162), with the duration clock
    releasing the handle at 470 ticks.
  - The generator never emitted one. All four branches in
    render-audio-fgm-phase-pack.py hardcoded `flags = 0` and
    `loop_point_words = 0`. The `"loop": True, loop_start: 48,
    loop_end: 13348` on cue 285's selector was descriptive source metadata
    that nothing acted on. My earlier note claiming it "ships with the loop
    live" was wrong.
  Fixed as a DS hardware loop, not an AOT render. The DS ADPCM channel
  latches predictor/index when it reaches PNT and restores them on every
  repeat, so putting PNT at the first word after the IMA header makes the
  latched state the header state and every cycle decodes bit-identically by
  construction. Cost is zero extra bytes -- the pack got 28 B smaller.
  AOT was the other option and would have fit (~20 KB for 2.70 s), but it
  buys nothing over a loop the hardware does for free, and 626 only renders
  AOT because its loop feeds a volume ramp that a hardware repeat cannot.
  Shape: body = pcm[48:13352], 13304 samples = 1663 words after a 1-word
  PNT; the 4 samples past loop_end are the source's own tail, taken as
  alignment debt so the seam is real audio rather than synthetic guard
  nibbles. 6656 B, SNR 34.6 dB, ~3.07 cycles per blow.
  The unused ima_encode_loop_body / ima_ds_repeat_cycles / ima_repeat_oracle
  machinery is now live and proves it: three restored cycles hash identically,
  and all three negative controls fire (carried decoder state diverges, PNT
  at the header rejected, LEN counting the whole buffer rejected).
  check-audio-fgm-phase-pack.ps1 pins the flag, the PNT/LEN geometry and the
  three proofs so the loop cannot be dropped again silently.
  Latest profile green. Needs an ear check.
  VFX still open: the same particle-bank gap as the other VFX rows below --
  lbParticleMakeScriptID is a skipped stub.
  
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
  Five causes eliminated with evidence, none of them it:
  1. Inverted culling -- refuted by the probe above.
  2. A skipped display list -- FTR_DISPLAY_CONTRACT reports 14 and 18 DLs
     per frame, which is exactly Mario's 14 and Fox's 18 in
     296_MarioModel.c / 303_NFoxModel.c, and selected equals submitted
     (6784 == 6784 over 212 frames). Every DL in both models runs.
  3. An unsupported opcode aborting a DL mid-way -- that sets
     NDS_RENDERER_BLOCKER_UNSUPPORTED and would drop submitted below
     selected. They are equal.
  4. Clipping or DS limits -- 2484 of 6144 vertices, 828 of 2048 polygons,
     rej=0, drop0.
  5. The wrong detail level. Mario's model carries two joint trees and
     203_MarioMain.c orders them High then Low:
       commonparts[0] = dMarioModel_JointTree        = 14 DLs (High)
       commonparts[1] = dMarioModel_JointTree_0x4590 = 22 DLs (Low)
     Note Low has MORE parts, not fewer -- it splits the model further
     rather than shedding triangles (373 vs 396 commands). scvsbattle.c
     picks High when pl_count + cp_count < 3, and Boundary is 1+1, so High
     is correct and 14 is what the port runs.
  SETTLED 2026-07-27: nothing is missing. The title of this row is wrong.
  Two measurements close the "is geometry being dropped" question:
  6. The source has the geometry, and it is closed. Parsed Mario's
     14 display lists straight out of the decomp: the torso
     (Joint_0x1668) is 36 triangles over 25 vertices with 54 edges,
     zero boundary edges and zero non-manifold edges -- Euler 20-54+36
     = 2, a closed shell. It is capped at the bottom. (Weld the vertices
     on position first: N64 splits a vertex per normal/UV, so v1/v3,
     v2/v5, v7/v19, v11/v13 and v6/v18 are the same point and an
     unwelded count reports 12 fake boundary edges.) So this is a port
     defect, not an open-shell model the original also shows.
  7. Every triangle is submitted, every frame. Mario's High tree totals
     exactly 320 triangles across its 14 lists. The port submits
     67,840 over 212 frames = 320.000/frame. Fox: 64,872 = 306/frame.
     The old 313-per-owner contract was the average of two different
     models and could never have shown this; it is now pinned per
     fighter at Mario 320 / Fox 306, so a future geometry loss moves a
     number instead of hiding in the mean.
  Two more mechanisms eliminated while getting there:
  8. A half-dropped G_TRI2 would have cost exactly half the model. Both
     the general path (nds_renderer.c:14713-14736) and the fast raw
     batch path (:14900-14912) decode and submit both halves; the fast
     path rejects the whole plan rather than emit a partial one.
  9. Vertex overflow in the DS 4.12 conversion. NDS_RENDERER_HW_WORLD_UNIT_SHIFT
     is 8, so the model coordinate is shifted left by 4 and saturates
     past |2047|. Mario's torso maxes at 92. Not close.
  So all 320 triangles exist, are closed, and reach the hardware -- and a
  hole is still visible. They are landing in the wrong place. That also
  explains the culling probe: forcing POLY_CULL_NONE cannot fill a gap
  between two solid parts, because no surface lies along that ray at all.
  NEXT: the torso is a single display list under a single matrix, so a bad
  torso matrix would move the whole torso, not hole it. The seam is
  between parts. Suspect the leg chain: both level-2 leg nodes carry the
  compound rotation {-pi/2, 0, -pi/2} and hang the thigh/shin/foot off it,
  and the reported band is exactly where the thigh should overlap the
  torso's lower rim. Compose the rest-pose joint transforms for
  dMarioModel_JointTree entries 15-17 and 20-24 and compare the thigh's
  world-space top against the torso's bottom rim (y = -52..-74). Bug #2 in
  this same pass was a local matrix used where a world matrix was
  required, which is the same class of error.