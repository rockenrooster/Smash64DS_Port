# P2-5 — Items (system + all 20 items + 13 Pokémon)

Items are a system plus content. The system lands once; items batch through it
by class. Fighter-side item states/animations already exist per fighter
(P2-3's pipeline bakes them), so this phase never reopens fighter work.

## System core (first slice)

1. **Item manager**: spawn scheduler (rules-driven frequency, per-stage spawn
   regions, active-item cap), item GObj lifecycle, despawn flash/timeout —
   mechanically equivalent to `it/itmanager.c` + `it/itmain.c`.
2. **Item physics**: throw/drop/bounce/rest, surface interaction, ownership
   and hit-attribution (thrown items hit with thrower's credit).
3. **Fighter interaction seam**: pickup priority, held-item hand attach,
   tilt/smash/air/dash throws, shield-drop, catch — wiring fighter states
   (already baked) to item states (`it/itfighter/`).
4. **Engagement integration**: item hitboxes and hurtboxes join the P2-2
   broadphase; projectile items join projectile ownership rules.
5. **Item switch UI** (VS menu) + spawn-rate law from the original.
6. **Draw**: small-model batching/atlas per class; projectile visuals through
   the effect pool caps.

## Item classes (batch order)

| # | Class | File | Members |
|---|---|---|---|
| 1 | Containers | `items/containers.md` | Crate, Barrel, Capsule, Egg — spawn infra + payload rolls first |
| 2 | Melee weapons | `items/melee-weapons.md` | Beam Sword, Home-Run Bat, Fan, Star Rod, Hammer |
| 3 | Ranged weapons | `items/ranged-weapons.md` | Ray Gun, Fire Flower |
| 4 | Throwables | `items/throwables.md` | Green Shell, Red Shell, Bob-omb, Motion-Sensor Bomb, Bumper |
| 5 | Passives | `items/passives.md` | Maxim Tomato, Heart Container, Star |
| 6 | Poké Ball | `items/pokeball.md` | Ball + 13 Pokémon (`it/itmonster/`) — biggest, last |

## Reference

`decomp/BattleShip-main/decomp/src/it/` — `itcommon/` (shared behavior),
`itfighter/` (fighter-held), `itground/` (stage-spawned), `itmonster/`
(Pokémon), `itmanager.c`/`itmap.c` (spawning), `itvisuals.c`. Fighter-article
overlap in `wp/` (e.g. Link's bomb) — reconcile ownership per item.

## Risks

- Frame cost: items add engagement targets and draw calls on already-hot
  frames. Every class closes with a stress measurement; the moment items
  land, the standing stress config flips to **items ON** and stays there.
- Bob-omb walking, Red Shell homing, and Pokémon are effectively lightweight
  actors — cap concurrent actives per original behavior, verify despawn.
- Hammer overrides fighter control + music — cross-cutting state, test with
  every movement edge (ledges, platforms, KO).

## Exit criteria

- [ ] Item switch UI + spawn law equivalent to original.
- [ ] All 20 items + 13 Pokémon per unit DoD (class file checklists).
- [ ] Stress config includes all items ON; gate measurements banked.
- [ ] Containers explode/payout equivalence verified against `itground`.
