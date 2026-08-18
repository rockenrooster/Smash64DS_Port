# Ness — P2-3 fighter 8

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftness/`

## Role

The most complex recovery in the game (player-steered projectile that turns
into a self-launch) plus the only absorb mechanic. Unlockable (gating P2-7).

## Moveset uniques

- **PK Thunder (Up-B)**: player steers the bolt head continuously; head has
  a hit, tail has hits; striking Ness himself triggers **PKT2** — a big
  armored self-launch used as recovery and as an attack. Steering feel is
  gameplay, not presentation: input→curvature must be equivalent.
- **PK Fire (B)**: bolt that blooms into a damage pillar on contact; angled
  differently in air (verify).
- **PSI Magnet (Down-B)**: absorbs energy projectiles, heals by absorbed
  damage; needs the projectile-classification flag (energy vs physical) to
  exist on every projectile in the game — add the flag at the projectile
  seam now, populate per article.
- **Yo-yo smashes** (up/down): charge-hold behavior at edges (yo-yo dangles)
  per source; famous edge interactions.
- Floaty double jump with momentum carry; strong b-throw.

## Assets & audio

Kid model, cap costumes, PSI VFX set (hexagon sparks), "PK Thunder!" voice
lines, announcer clip.

## DS notes / risks

- PKT2 collides with terrain mid-flight — trajectory + armor + wall-stop
  rules from source; test on every landed stage's underside geometry.
- PSI Magnet's absorb flag audit touches all prior fighters' projectiles —
  schedule as a sweep row, cheap but wide.

## Acceptance

- [ ] Move inventory sweep vs `ftness` data.
- [ ] PK Thunder steering + PKT2 launch/armor equivalent (replay-verified).
- [ ] Absorb table correct for every existing projectile.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
