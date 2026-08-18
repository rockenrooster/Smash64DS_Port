# Captain Falcon — P2-3 fighter 3

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftcaptain/`

## Role

Cheap, fast win after DK: no projectiles, no articles, standard grab — but
the fastest fall speed and run speed in the game, so he stress-tests movement
extremes (traction, landing, edge slips) that slower fighters never reach.
Unlockable (gating P2-7).

## Moveset uniques

- **Falcon Punch (B)**: long wind-up single hit, signature VFX (falcon
  silhouette) + voice line.
- **Falcon Kick (Down-B)**: ground slide / air diagonal dive variants,
  fire trail.
- **Falcon Dive (Up-B)**: command-grab recovery — connects as a grab,
  explosion release, regrab-after-release allowed (verify rules in source).
- Fastest faller; gravity/air-speed extremes; knee f-air sweetspot timing.

## Assets & audio

Model with helmet, 4 costumes; voice bank is identity-critical ("Falcon
PUNCH!", "YES!") — announcer clip, crowd chant.

## DS notes / risks

- Falcon Dive is grab-logic inside a special — reuse the grab seam, not a
  parallel implementation.
- His speed makes him a good early camera/engagement stress case at 4
  fighters; run a 4×Falcon stress arm when he lands.

## Acceptance

- [ ] Move inventory sweep vs `ftcaptain` data.
- [ ] Falcon Dive grab/release/regrab semantics equivalent.
- [ ] Fast-fall/landing/edge behavior spot-checks at speed extremes.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
