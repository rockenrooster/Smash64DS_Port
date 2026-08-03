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

WHERE THE SEAM ACTUALLY IS, which is narrower than "implement the DObj path":
the port ALREADY draws effect model trees. battleship_efmanager.c:1080 hands
gcDrawDObjTreeForGObj to gcAddGObjDisplay for effects generally, and it works.
What it substitutes for these rows is the GEOMETRY SOURCE -- :484 builds the
shield with ndsEFManagerBuildDisc(...) and routes it to a hand-written
ndsEFManagerShieldProcDisplay, and the rebirth halo gets the same treatment.
A procedurally generated disc is standing in for a loaded model.

THE EXACT MECHANISM, and the port already wrote it down twice:

  Makefile:1401 (NDS_R2_SOURCE_EFFECTS_FULL ?= 0)
    "these submit their geometry as source effect DL links, which the battle
     hardware path does not consume, so routing them trades a visible
     untextured primitive for nothing on screen. That is the same seam that
     kept the respawn platform invisible."
  battleship_efmanager.c:1240
    "source effect DL links are not submitted by the battle hardware path,
     which is why the respawn platform was invisible."

So the machinery is all present and PROVEN: efManagerMakeEffectForce loads a
source DObjDesc and gcDrawDObjTreeForGObj draws it -- that is exactly how the
KO burst (dEFManagerDeadExplodeEffectDesc) already works, unconditionally, with
its offsets resolved by ndsEFManagerResolveAllDescOffsets. dEFManagerShieldEffectDesc
and dEFManagerImpactWaveEffectDesc are ALREADY in that resolve list; they are
just gated off.

What is missing is one thing: the battle camera does not capture the DL links
these effects register on (the reflector is DL link 15, the impact wave 10),
so their geometry is built and never submitted. The port's stand-in registers
on link 18, which IS captured -- which is why a procedural disc appears and the
real model does not.

THE WORK, in order:
  1. Extend the battle camera's captured DL links to cover the source effect
     links, and make the hardware path submit them.
  2. Set NDS_R2_SOURCE_EFFECTS_FULL = 1.
  3. Retire the ndsEFManagerBuildDisc stand-ins for shield and rebirth.
ACCEPTANCE, already defined at Makefile:1409: a soak whose
gNdsTaskmanGeneralHeapFreeMin stays above 25,600 AND a capture showing they
draw. The 25,600 matters because ifCommonSetMaxNumGObj latches the GObj pool
below it -- an earlier attempt at this flag capped the pool for a whole match,
so this is a memory-risk change and not a cosmetic one.
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
