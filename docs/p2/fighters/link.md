# Link — P2-3 fighter 5

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftlink/`

## Role

Two live articles at once (boomerang out + bomb in hand) — the fighter that
forces article ownership to be right. Scheduled adjacent to P2-5 because his
bomb IS an item.

## Moveset uniques

- **Boomerang (B)**: angleable throw, returns to Link along a homing path,
  catch on return, one airborne at a time; hits on both legs of flight.
- **Bombs (Down-B)**: pulls a held bomb *item* (timer fuse, explodes on
  impact/timeout, can be thrown/dropped/caught, hurts Link too). Implement
  through the item system's held-item seam — this is the bridge unit between
  P2-3 and P2-5; if items aren't started yet, land the bomb as the first
  item-system client rather than a bespoke fork.
- **Spin Attack (Up-B)**: multi-hit ground version, weaker air recovery —
  famously poor recovery overall.
- Sword ranged normals with tip semantics; d-air strong spike; wall-jump? no
  (verify — 64 movement quirks belong to source).

## Assets & audio

Sword+shield model (shield is cosmetic passive block on idle? verify the 64
passive shield behavior), 4 costumes, sword swing/chime SFX, voice grunts,
announcer clip.

## DS notes / risks

- Boomerang return-path steering must be equivalent — it's a gameplay tool,
  not VFX.
- Bomb self-damage/ownership rules; bombs surviving Link's KO (verify).
- Two articles + sword trails = draw/effect budget watch.

## Acceptance

- [ ] Move inventory sweep vs `ftlink` data.
- [ ] Boomerang out/return/catch matrix equivalent.
- [ ] Bomb pull/throw/catch/fuse/self-damage equivalent via item seam.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
