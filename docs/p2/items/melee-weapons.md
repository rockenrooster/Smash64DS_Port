# Items: Melee Weapons — Beam Sword, Bat, Fan, Star Rod, Hammer (P2-5 class 2)

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/it/itfighter/`.

Held weapons that replace A-attack behavior; the class that exercises the
held-item combat seam (P2-3 baked the swing anims — this wires damage).

## Members & behavior (source-exact)

- **Beam Sword**: extends reach on swings; blade length varies by swing
  strength (smash swings reach farthest); glow VFX.
- **Home-Run Bat**: slow smash swing with massive fixed-angle KO power (the
  "home run" identity + its distinctive SFX); normal swings are ordinary.
- **Fan**: fastest swings, tiny damage, huge shield damage (shield-break
  tool), infamous jab-lock behavior — frame data exact.
- **Star Rod**: swings fire star projectiles on tilt/smash (limited stars? —
  verify ammo rule); melee hit + projectile in one action.
- **Hammer**: overrides everything — auto-swing loop, no jumps?? (movement
  restrictions from source), Hammer music overrides BGM, timed duration,
  famous head-falls-off failure roll (verify if in 64 — I believe the
  headless hammer is 64: verify). Test vs ledges, platforms, KOs mid-hammer.

## DS notes / risks

- Swing hitboxes attach to the weapon bone through the same transform path
  as the hand attach — one seam, no per-weapon offsets.
- Hammer's BGM override and control lockout are cross-cutting (audio + input
  seams); its edge cases are the class's real cost.
- Star Rod stars join the projectile pool with owner attribution.

## Acceptance

- [ ] Per-weapon damage/reach/frame tables equivalent.
- [ ] Fan shield damage + Bat KO trajectory verified (community-known feels).
- [ ] Hammer full edge-case matrix (ledge, platform drop, KO, timer end,
      music restore).
- [ ] Stress measurement with weapons active banked.
