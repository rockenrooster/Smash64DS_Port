# Samus — P2-3 fighter 4

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftsamus/`

## Role

First stored-state projectile fighter: Charge Shot introduces cross-state
persistent charge plus a spawned bomb article, on a heavy-floaty chassis.

## Moveset uniques

- **Charge Shot (B)**: multi-stage charge, storable, cancel/resume, release
  size/speed/damage scale with charge; charge VFX while held.
- **Bombs (Down-B)**: morph-ball drop, timed pop, **bomb-jump must work** —
  the pop boosts Samus; recovery tech players expect.
- **Screw Attack (Up-B)**: multi-hit rise, weak knockback out.
- Heavy but floatiest fall in class; high first jump; long non-tether ledge
  reach (no grapple in 64).

## Assets & audio

Arm-cannon rig (muzzle attach point for charge VFX/shot spawn), 4 costumes,
beam/bomb SFX from source set, announcer clip.

## DS notes / risks

- Charge persistence across hitstun/KO/respawn per source (players know the
  rules; verify, don't guess).
- Bomb article: physics + owner attribution through the projectile seam
  (`wp/`/`it/itfighter` — check where BattleShip keeps it).
- Charge Shot at full size is a big translucent projectile — effect-pool and
  fill-rate check on DS.

## Acceptance

- [ ] Move inventory sweep vs `ftsamus` data.
- [ ] Charge store/cancel/resume/release matrix equivalent; persistence rules
      verified.
- [ ] Bomb-jump reproduces (scripted input replay).
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
