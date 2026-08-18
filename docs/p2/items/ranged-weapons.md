# Items: Ranged Weapons — Ray Gun, Fire Flower (P2-5 class 3)

Status: not started · Reference: `decomp/BattleShip-main/decomp/src/it/itfighter/`.

Held shooters with ammo — exercises aim-fire states and item projectiles.

## Members & behavior (source-exact)

- **Ray Gun**: 16 shots (verify count), fast flat-trajectory energy bolts
  with strong knockback per hit, distinctive SFX; empty-gun throw still a
  projectile weapon (throwing any spent item is standard, but the gun's
  throw is a known finisher).
- **Fire Flower**: held flame spray — continuous short-range multi-hit
  stream while held (drains ammo/duration — verify), rapid damage ticks,
  pushback; classic edge-guard tool.

## System features

- Ammo model on held items (count or meter), empty behavior.
- Fire-while-held states (standing/walking fire? — movement rules from
  source), air fire.
- Energy-projectile classification: Ray Gun bolts are absorbable by Ness's
  PSI Magnet and reflectable by Fox's Reflector — the classification flags
  added in P2-3 get their item-side population here.

## DS notes / risks

- Fire Flower's stream = many small hit events + particle load on hot
  frames — effect-pool policy test; visual doctrine allows a sprite-stream
  approximation if telegraph stays exact.
- Ray Gun bolt speed vs 60 Hz collision stepping — tunnel-proof the sweep
  test (fast projectile vs thin hurtbox).

## Acceptance

- [ ] Ammo counts/durations + damage tables equivalent.
- [ ] Reflect/absorb classification correct for both weapons' output.
- [ ] Fire Flower stream cadence + pushback equivalent.
- [ ] Stress measurement with ranged weapons active banked.
