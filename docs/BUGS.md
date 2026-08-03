**Follow Bug fixing workflow contained in `BUG_FIXING_PROCESS.md`.**
AI Agent should mark fixed items with **FIXED** prefix or a 20 word summary if not fixed yet.
These bugs should be fixed for P1 delivery:

This board carries verdicts and the numbers that check them. The forensics for each
fix live next to the code that owns it -- the particle generator and its checker,
`battleship_lbparticle.c`, `battleship_efmanager.c`, `render-audio-fgm-phase-pack.py`
-- so a row here should not need to be an essay.

-Some Crowd noise audio cues get cut off (like for big hits that reach upper bound KO boundary).
    Owner: Ok if source cuts them off, then lets change that, I don't want the sound cues interrupted if possible.
    **FIXED** (2026-08-03) the audience no longer interrupts itself. Both cuts live in ftpublic.c and both
    go through func_80026738_27338; battleship_ftpublic.c renames that symbol for this TU only, so fighter
    voices and looping SFX keep source behaviour. Not a voice leak: the FGM mixer reclaims a handle on
    REASON_DURATION. gNdsFtPublicCueLetRingCount counts declined interruptions. Boundary green.

=============================================================================
ONE FINDING EXPLAINS FOUR OF THESE ROWS, AND IT IS NOT THE ATLAS (2026-08-03)
=============================================================================
The respawn platform, the shield, Fox's down B and the hard-landing impact wave
are NOT particle sprites in the source. Each is a full EFDesc drawn as an
ANIMATED DObj MODEL TREE, with its own display-list link, geometry, joint
animation and material:

  efmanager.c:1648  dEFManagerRebirthHaloEffectDesc
                    -> llEFCommonEffects3RebirthHaloDObjDesc + ...AnimJoint
                       made by efManagerRebirthHaloMakeEffect(gobj, scale):5994
  efmanager.c:460   dEFManagerShieldEffectDesc
                    -> llFTManagerCommonShieldDObjDesc
  efmanager.c:420   dEFManagerFoxReflectorEffectDesc
                    -> llFoxSpecial2ReflectorDObjDesc + ...StartAnimJoint,
                       texture file gFTDataFoxSpecial2, render gcDrawDObjTreeForGObj
  efmanager.c:201   dEFManagerImpactWaveEffectDesc   <- the hard-landing wave
                    -> llEFCommonEffects1ImpactWaveDObjDesc + MObjSub
                       + AnimJoint + MatAnimJoint, render efManagerImpactWaveProcDisplay

The port draws all four as flat camera-facing quads out of the particle atlas,
using ONE texture lifted from each model's file (e.g. dEFCommonEffects3_
RebirthHalo_glow for the platform). That is why three cycles of atlas work --
bigger cells, more palette entries, source resolution -- never satisfied any of
these rows: the parameters were wrong because the MECHANISM is wrong. "The Halo
is not the correct asset to use" is exactly right; the halo glow is one texture
belonging to a model the port never draws.

What this row family actually needs is the DObj path, not a better sprite. That
is a scoped piece of work (four EFDescs, each with geometry + joint anim), and
it should be planned as such rather than attempted as another atlas tweak.
=============================================================================

-Respawn floating platform isn't visible when respawning after KO.
    Owner: is don't see the floating revival platform at all. the Halo is not the correct asset to use
    ROOT CAUSE FOUND (see banner): it is dEFManagerRebirthHaloEffectDesc, a MODEL with joint animation.
    The port draws one texture from that model's file on a flat quad. Resolution was never the problem.

-Fox down B VFX is not correct or using correct asset.
    Owner: you are still not using the correct asset for Fox's down B reflector.
    ROOT CAUSE FOUND (see banner): dEFManagerFoxReflectorEffectDesc renders via gcDrawDObjTreeForGObj
    from gFTDataFoxSpecial2. It is an animated model, not a sprite. relocData/346 was the wrong lead.

-Shield VFX not correct
    Owner: texture looks cut in half: `artifacts/visibility/2026-08-03_owner_shield-cut-in-half.png`
    ROOT CAUSE FOUND (see banner): dEFManagerShieldEffectDesc is a model (llFTManagerCommonShieldDObjDesc).
    'Cut in half' is a 1:2 source cell on a square quad -- a symptom of drawing a model as a sprite.

-Hard landing vfx not not using correct asset.
    Owner: incorrect asset for the impact wave is being used
    ROOT CAUSE FOUND (see banner): it is dEFManagerImpactWaveEffectDesc -- DObjDesc + MObjSub + AnimJoint
    + MatAnimJoint. A material-animated model. No single atlas texture can be the 'correct asset' for it.
    A hard landing also emits efManagerDustHeavyDoubleMakeEffect (efmanager.c:2982, script 0x58), which IS
    a particle effect -- so this row has a model half and a particle half, and only the wave is the model.

-KO VFX not drawing correctly.
    Owner: Not fixed yet, the "blast pillar" VFX isn't drawing, and doesn't seen to draw on the same z axis as the fighters
    TRACED: efManagerSparkleWhiteDeadMakeEffect (efmanager.c:3725) runs particle script 0x5C, driven from
    ftCommonDeadUpStarProcUpdate. Script 0x5C asks for texture 24, which IS admitted (32x32, source size),
    so the sheet is not the blocker here -- unlike the four rows above, this one IS a particle effect.

-Some "hard hit" (side A attacks that hit) VFX look too big, please apply correct scaling to VFX.
    MEASURED: scale is source-exact (efmanager.c:2175/2197). Last cycle's clamp was in UNREACHABLE code.

-Shield freeze bug happened again. Screenshot: `artifacts/visibility/2026-08-03_owner_shield-freeze.png`
    (copied into the repo from your Pictures folder -- tracked files must not carry your name.)
    **FIXED** (2026-08-03) yesterday's fix converted 2 of the 35 `while (TRUE);` panics the port compiles,
    both in taskman. objman.c held 19 more and they are the ones a shield hits -- it allocates a GObj,
    a DObj and an MObj on the frame it spawns. All 19 record and fail the allocation now.
    gNdsObjmanPanicCount must read 0; gNdsObjmanPanicMask names the site. Boundary green.