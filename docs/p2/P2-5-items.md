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

## Source pins (verified 2026-09-03)

Read once, cited here so no slice re-derives them. Paths are relative to
`decomp/BattleShip-main/decomp/src/`.

**Kind enum** — `it/itdef.h:91-170`, no explicit initializers, so values are
sequential from 0 and `nITKindEnumCount` (`:168`) closes it at **45 kinds**:
20 common (4 containers `nITKindBox`..`nITKindEgg`, then 16 utility ending at
`nITKindMBall`), 2 fighter articles (`nITKindNessPKFire`, `nITKindLinkBomb`),
10 stage-spawned, and **13 Poké Ball Pokémon** ending at `nITKindMew`. The
`*Start`/`*End` aliases in the enum are the range tests the manager itself
uses — prefer them to literals.

**Manager** — `it/itmanager.c`:

- `itManagerMakeItem` `:229-461` pops the `ITStruct` freelist, makes the GObj
  (`:241`), loads `ITAttributes` through `lbRelocGetFileData(*p_file,
  o_attributes)` (`:249`), picks the OPA/XLU/ColAnim display proc
  (`:251-257`), copies the eight procs out of the kind's `ITDesc`
  (`:419-426`), and attaches `ProcItemMain` / `SearchHitAll` /
  `HitCollisions` (`:415-417`).
- `dITManagerProcMakeList[45]` `:41-97` is the per-kind maker table; the two
  fighter-article slots are `NULL` (`:68-69`) because their owners make them.
  `itManagerMakeItemKind` `:717-720` indexes it, and
  `MakeItemSetupCommon` `:464-477` adds the spawn swirl and spin for
  `index <= nITKindCommonEnd`.
- Spawn law: `AppearanceRatesMin/Max` `:19-38`, `SetItemSpawnWait` `:486-494`
  keyed on `gSCManagerBattleState->item_appearance_rate`,
  `AppearActorProcUpdate` `:497-526`, `MakeAppearActor` `:529-630` (weights =
  the player's toggles × the stage's MP item weights over the common set and
  the stage's Item mapobjs), `SetupContainerDrops` `:633-707`.
- Per-kind data shapes: `ITDesc` `it/ittypes.h:24-39`, `ITAttributes`
  `:143-192`, `ITStatusDesc` `:41-51`, timed events `:112-133` driven by
  `itMainUpdateAttackEvent` (`it/itmain.c:615-632`).
- Carry/throw: `it/itmain.c:406-` attaches to the hand joint; release, drop
  and throw are `:318-403` (`vel * vel_scale`, `times_thrown`, `throw_mul`,
  stale lanes, collision refresh), with per-kind dropped/thrown proc lists at
  `:21`/`:53`. Thrown damage is
  `(base + |vel| * 0.1) * throw_mul * stale + 0.999` (`:265-278`).
- Poké Ball roll: `itMainMakeMonster` `:635-701` — 1/151 Mew once newcomers
  are unlocked, otherwise uniform over the common twelve minus the last two
  spawned, with a 1P Mew bonus (`:692-698`).

**Item switch UI** — `mn/mnvsmode/mnvsitemswitch.c`. Fifteen toggle rows at
x244, y = `i * 10 + 54` (`:152-181`, `:473-488`); the appearance-rate sprite
moves per rate (`:434-470`); cursor geometry `:393-404`. State is two fields
only: `OptionStatuses[16]` (`:92`) mapped to kinds by
`TogglesItemKinds[16]` (`:39-57`), committed to the battle state at
`:589-616` — note Green/Red Shell share a row (`:601-613`), the four
containers are forced on (`:657`), and an all-off selection commits rate 0
(`:632-659`).

**Port state today.** `NDS_P2_ITEM_CORE` is `1` iff any of
`NDS_P2_{LINK,NESS,PIKACHU,PURIN,KIRBY}` is (`Makefile:732-733`), and it
compiles `src/import/battleship_item_link_core.c` only. That file owns the
pool and the now-resident `ITCommonData`, includes `itmap`/`itprocess`/
`itvisuals` verbatim, and its `itManagerMakeItem` **refuses every kind but
`nITKindLinkBomb`** (`:532-537`) — that single condition is the stub standing
between here and all 45 kinds. Link's bomb (kind 21) and Ness's PK Fire
(kind 20) are live behind their own fighter flags. Art for every non-fighter
kind comes from `gITManagerCommonData`, i.e. reloc asset `0xfb`, already
rowed (`src/nds/nds_reloc_assets.c:138`) with its `MiscData086` dependency
(`:139`), so **no slice below is asset-blocked**.

## Slice order (dependency order, from the pins above)

1. Manager, pool, appear actor, container drop tables, arrow and despawn —
   gates everything. Mechanical.
2. Touch-consumed: Tomato, Heart, Star; plus the Hammer and Star fighter
   states and the Hammer's music seam. Mechanical.
3. Swung: Sword, Bat, Harisen — rebound and break. Mechanical.
4. Containers and their payload rolls, Poké Ball spawner, the monster bus,
   Mew and its 1P flag. Pippi last: it dispatches its siblings' procs.
   Mechanical.
5. Ammunition: Star Rod, Ray Gun, Fire Flower — the `wp/` projectile pattern.
   Mechanical.
6. Self-acting: Motion-Sensor Bomb, Bob-omb, both shells, Bumper. Mechanical.
7. Stage hazards that are items: POW block, Green Bumper, Piranha, Target,
   barrel bomb, and Saffron's five — reuse the monster timers. Mechanical.
8. Item switch UI, the rate law end to end, atlas and batching, and the
   items-ON stress measurement. DS adaptation for the UI layout only.

Cheap and batchable: the three ammunition items, the two shells, Saffron's
five, and the twelve common-rate Pokémon. Bespoke: Hammer and Star states,
containers, the Poké Ball monster bus, Bob-omb's walk, Red Shell's homing,
Pippi, and the switch UI.

## Two link-time prerequisites the batch order did not name (2026-09-03)

**The pickup arrow's sprite was not staged.** Every common item calls
`ifCommonItemArrowMakeInterface` on the frame it becomes pickable
(`itbat.c:236`, `itbox.c:459`, `itcapsule.c:281`, `itegg.c:312`,
`itfflower.c:255`, `itgshell.c:576`, `ithammer.c:245`, `itharisen.c:263`,
`itheart.c:181`, `itlgun.c:267`, ...). The three functions themselves were
already here -- `battleship_ifcommon.c` includes the whole source
`if/ifcommon.c`, so grepping `src/` for the name finds only the header
declaration and misses them. What was missing was the asset and the call.
`ifCommonItemArrowSetAttr` loads the sprite from relocData file 87
(`87_IFCommonItem.spritelist`, one sprite named `Arrow`), and
`include/reloc_data.h` rowed both its symbols against
`NDS_RELOC_ASSET_INVALID`. File 87 is now staged: the O2R bank
`reloc_interface/IFCommonItem` joins `NDS_ITEM_RELOC_FILES`, asset `0x57` has
its path row, its token row and its sprite-normalize row, and both symbol rows
name the real asset. The sprite record was read out of the extracted bank
rather than guessed -- 9 by 7, one bitmap, I4, `ndisplist` 36, which is exactly
the `12n + 24` the normalizer derives for one bitmap, so it self-checks.
`itManagerInitItems` now calls `SetAttr` where the source does
(`it/itmanager.c:159`). Order matters here: the source chains the size query,
the allocation and the load into one expression, so calling it before file 87
was staged would have handed a fallback size to `lbRelocGetExternHeapFile` --
the heap-corruption trap `itManagerInitItems` already documents for
ITCommonData.

**The attribute decode is no longer per-kind source.** `itManagerMakeItem` used
to carry one `switch` arm, one pair of file-scope statics and one reset line per
kind. It now keys a single cache by kind (`sNdsItAttributes`,
`sNdsItAttributesFile`, bounded by `NDS_IT_ATTR_KIND_MAX`), so landing a kind is
a descriptor plus its procs. Raise that bound with each batch. A kind with no
validator is admitted rather than refused -- `TRUE` there means *unproved*, and
the batch that lands a kind still owes it an oracle in the shape of
`ndsItValidateGBumperAttributes`.
