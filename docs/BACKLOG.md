This file should only contain minor bugs to be fixed at a later date.

## Results confetti (moved out of BUGS.md 2026-08-03, owner's call: work on it later)

Everything below is already established, so the next cycle starts from evidence
rather than from scratch.

**The structural difference, found.** `mnvsresults.c:3208` makes TWO emitters at
different depths on different generator links, and `efmanager.c:6206` is what
splits them:

    pos0 = (0, 1000, -1000)  is_genlink_mask FALSE -> bankID | MASK_GENLINK(3)  BEHIND
    pos1 = (0, 1000,  -400)  is_genlink_mask TRUE  -> bankID                    IN FRONT

`LBPARTICLE_MASK_GENLINK(3)` is 32, so they land in alloc slots 0 and 4. The
owner's "falls behind fighter instead of infront" is one of those two not
reaching the screen.

**Ruled out.** The port's per-link gate `gobj->camera_mask & (1 << link)` is
SOURCE-EXACT -- `lbparticle.c:1500` uses the same test. GDB confirms both calls
execute (breakpoints hit at `mnvsresults.c:3216` and `:3217`). So neither the
draw loop nor the call site is the defect.

**Still open.** Whether the near (-400) emitter ALLOCATES.
`efManagerConfettiMakeEffect` returns NULL if `lbParticleMakeScriptID` finds no
generator, if `lbParticleAddTransformForStruct` fails, or if `xf->users_num`
is 0. That is the next measurement.

**The port's non-source FAN is still in place**, deliberately. It turns each
source call into three at x -900/0/+900. It is wrong on the merits -- it widens
the axis the owner is not complaining about, reads as off-centre once the
Results camera moves, and divides the fixed 384-struct pool six ways (~64 pieces
per emitter where the source gives 192) -- and it was removed and then RESTORED
on 2026-08-03 because removing it could not be verified either way. Remove it
again only alongside a working measurement.

**The instrument is blind, and this is the trap to avoid.**
`capture-results-tic.ps1 -Tic 420` shows NO confetti with the fan present and NO
confetti with it removed -- identical arms. It cannot measure this row. Do not
grade a confetti change on it.
`probe-results-confetti.ps1` is also broken: it reads `gNdsConfettiFanCount`,
and `--gc-sections` deletes any `volatile` the ROM never reads. A new counter
must be named in a marker block in the same change that adds it, or it measures
nothing.

Owner screenshot: `artifacts/visibility/2026-08-03_owner_confetti-behind-fighter.png`

## SACRIFICE: every animated particle is a still image (accepted 2026-08-04)

**What is given up.** The quad atlas packs ONE cell per texture, so 19 of the 32
admitted textures ship a single frame of an animation and play as a still. The
frame cap is global and it is 1; commit `3d002c39` names and ranks all nineteen,
`ba773a24` traces the coin case end to end. Among the effects a player would
name: Mario's up-B coin (DamageCoin), the generic hit flashes
(DamageNormalLight/Heavy), the fireball hit (DamageFire), the running-foot dust
(DustDash), the white sparkles (SparkleWhite/Multi), Dream Land's Whispy
leaves, and the KO burst's star (DeadExplode).

**Why it stays.** The cap cannot simply rise. At cap 2 the shelf packer refuses
DustDash and DustHeavy outright, and a binary-absent effect is worse than a
frozen one -- that is the failure class this sheet already spent two cycles
fixing. Cap 3 refuses five live textures. `3d002c39` measured a per-texture
budget that would restore 8 of the 19 with nothing dropped; spending it is the
owner's call and has not been made.

**One frame is now chosen; eighteen are not.** EF common texture 25
(DamageCoin) holds source frame 2, the owner's pick by eye from
`artifacts/visibility/2026-08-04_upb-coin-timeline-source-vs-port.png` -- its
frame 0 was the most opaque of the fifteen and read as a flat yellow blob. Every
other multi-frame texture still holds frame 0, and holds it by accident of
`ndsParticleQuadFrameFor`'s nearest-earlier clamp rather than by anyone judging
frame 0 representative. `QUAD_HELD_FRAME` in
`scripts/generate_nds_particle_banks.py` is the one-line-per-texture register if
the owner wants another one chosen.

**The residual a contact sheet cannot show.** The coin's starburst phase (source
frames 12-14) opens at size 255 expecting a sparse, spread shape. A held frame
is solid whichever one is picked, so those frames still draw a large solid coin.
Frame 2 improves the look; it does not remove that size/shape mismatch. Closing
it needs either the starburst frames on the sheet or a size curve, not a
different held frame.

### Verbatim row as it stood in BUGS.md

```
-Results confetti doesn't look right.
    Owner: confetti falls behind fighter instead of infront and is not centered on the camera view: `artifacts/visibility/2026-08-03_owner_confetti-behind-fighter.png`
    **STRUCTURAL DIFFERENCE FOUND** (mnvsresults.c:3208 + efmanager.c:6206). The source makes TWO confetti
    emitters at DIFFERENT DEPTHS, on DIFFERENT GENERATOR LINKS:
      pos0 = (0, 1000, -1000) is_genlink_mask FALSE -> bankID | LBPARTICLE_MASK_GENLINK(3)  = link 3, BEHIND
      pos1 = (0, 1000,  -400) is_genlink_mask TRUE  -> bankID                                = link 0, IN FRONT
    You are seeing link 3 and not link 0, which is exactly "falls behind the fighter instead of in front".
    The port's per-link gate `gobj->camera_mask & (1 << link)` is SOURCE-EXACT (lbparticle.c:1500 uses the
    same test), so the draw loop is not the defect. A GDB run confirms BOTH calls execute -- breakpoints
    hit at mnvsresults.c:3216 and :3217. So both emitters are made; what is unproven is whether the
    second one ALLOCATES (efManagerConfettiMakeEffect returns NULL on a short pool) and whether the
    Results GObj's camera_mask carries both slots. LBPARTICLE_MASK_GENLINK(3)=32 puts them in alloc
    slots 0 and 4, not 0 and 3.
    The port's non-source horizontal FAN is now REMOVED (efManagerConfettiMakeEffect is no longer
    overridden). It spread pieces to +/-900 in world x -- which is what makes them read as off-centre once
    the Results camera moves -- and it divided the fixed 384-struct pool six ways, ~64 pieces per emitter
    where the source gives 192. Source structure restored: two emitters, both at x=0, depths -400 and
    -1000. Boundary green. Still open: whether the near (-400, in front) emitter allocates.
    Both emitters sit at x=0: "not centered on the camera view" is the Results camera, not the emitter.
```
