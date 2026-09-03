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

## The real inventory

`dITManagerProcMakeList` (`it/itmanager.c:41-97`) and the kind enum
(`it/itdef.h:91-170`) give **45 kinds**, not the twenty-plus-thirteen this
plan first assumed:

- **20 common** (`itcommon/`) — four containers (Box, Taru, Capsule, Egg) and
  sixteen utility items (Tomato, Heart, Star, Sword, Bat, Harisen, Star Rod,
  Ray Gun, Fire Flower, Hammer, Motion-Sensor Bomb, Bob-omb, Bumper, Green
  Shell, Red Shell, Poke Ball).
- **2 fighter-owned** (`itfighter/`) — Ness's PK Fire pillar and Link's bomb.
  Both are NULL in the manager's table and are made by their fighter.
- **10 stage-spawned** (`itground/`) — POW block, the Mushroom Kingdom bumper,
  Piranha, and the Target and barrel-bomb breakables, plus the five Saffron
  City Pokemon.
- **13 Poke Ball Pokemon** (`itmonster/`).

Corrections to the earlier grouping: the Egg is a **container**, not a
throwable; the Bumper is self-acting rather than thrown; Hammer and Star are
**fighter-state overrides** with their own BGM (`it/itvars.h:36-46,81-90`,
`ft/fthammer.c`); the Poke Ball is a spawner (`itcommon/itmball.c:308-348`);
and the containers live in `itcommon/itbox.c:220-303`, so the exit criterion
that said to verify them against `itground` was pointing at the wrong
directory.

**All of the common, monster and stage item data — models, textures and
animation — lives in one reloc file, `ITCommonData`**, which every descriptor
reaches through `&gITManagerCommonData`. Board row P2-3f48 makes that file
resident for 3,392 bytes, so it is the single prerequisite for this whole
phase, not an optional extra. The two fighter-owned items are the exceptions:
Link's bomb data is in his own reloc file and Ness's pillar in his.

## Batch order

Ordered by which machinery each batch unlocks for the next, not by theme:

1. Manager, physics, despawn and the arrow blink — unlocks everything else.
2. Touch-consumed Tomato, Heart and Star, plus the Hammer's fighter-state and
   BGM seam, which reuses Star's timer path.
3. Swing-and-throw Sword, Bat and Harisen, sharing the breakable and rebound
   work with batch 4.
4. Containers and their payload rolls (`itbox.c:220-303`,
   `itmain.c:575-612`) — unlocks the spawner logic the Poke Ball reuses.
5. Ammo shooters: Ray Gun, Fire Flower, Star Rod — establishes the
   item-owns-a-`wp/`-projectile pattern the Pokemon need.
6. Self-actors: Motion-Sensor Bomb, Bob-omb, both shells, Bumper.
7. Poke Ball, the monster bus, Mew and its 1P bonus flag, then the stage
   hazards, which reuse the monster timers.
8. Regression only for the two fighter-owned items, which already exist.

Clefairy's Metronome dispatches another monster's proc list
(`itmonster/itpippi.c:68-108`), so it lands last within batch 7. Goldeen and
Mew are cosmetic. Selection is a 1/151 Mew roll and otherwise uniform over the
common twelve excluding the last two spawned (`itmain.c:635-699`).

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
