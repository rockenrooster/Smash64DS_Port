# P2-5 — Complete Item, Summon and Stage-Item Coverage

Finish the existing imported item system and native production pipeline. Track source kinds, reachable draw states and child objects separately. This phase is not another manager import; item-enabled is not item-complete.

## Exact accountability domain

The source `ITKind` enum defines **45 kinds**: 20 common, 2 fighter-owned, 10 stage-owned and 13 Poké Ball Pokémon. `dITManagerProcMakeList` contains NULL entries for the two fighter-owned kinds; their fighter makers are the legitimate creation path. Do not require 45 non-NULL generic maker entries or conflate a maker mask with native coverage.

| Group | Source kinds / owning unit |
|---|---|
| Containers (4) | Box, Taru, Capsule, Egg — `items/containers.md` |
| Passives (3) | Tomato, Heart, Star — `items/passives.md` |
| Swung/state override (5) | Sword, Bat, Harisen, StarRod, Hammer — `items/melee-weapons.md` |
| Held shooters (2) | LGun, FFlower — `items/ranged-weapons.md`; StarRod's child weapon remains linked to its melee owner |
| Self-actors (5) | MSBomb, BombHei, NBumper, GShell, RShell — `items/throwables.md` |
| Poké Ball (1) + summons (13) | MBall plus source Iwark through Mew — `items/pokeball.md` |
| Fighter-owned (2) | NessPKFire and LinkBomb — existing Ness/Link unit contracts, shared item runtime here |
| Stage/bonus-owned (10) | PowerBlock, GBumper, Pakkun, Target, TaruBomb, GLucky, Marumine, Hitokage, Fushigibana, Porygon — owning stage/bonus contracts and this shared pipeline |

These counts refer to kinds, not distinct native shapes, status callbacks, child weapons or atlas cells. Source common/monster/stage data roots use ITCommonData and dependencies; fighter-owned exceptions use their source roots. The actual allocation/decoded payload is measured—never inferred from a fallback size or an old plan's number.

## Package: source-to-output coverage inventory

**Outcome:** Each kind and reachable state has a creator, attributes/status source, model/material/animation root, child dependency set, native renderer owner, audio roots and proof route.

Extend the existing manifest/census rather than build another inventory framework. Read `itdef.h`, `itmanager.c` and per-kind tables; runtime constructor identity decides ownership. Distinguish absent owner, owner with incomplete states, registered-but-unexercised state, and accepted natural behavior. Record counts from the manifest, not hand-maintained prose.

**Exit:** No required kind, draw state or child is unaccounted for. An unexplained missing source/asset mapping is a bounded source task, not permission to add a placeholder behavior.

## Package: residency and reproducible generated output

**Outcome:** The complete required scene inventory fits and builds correctly without a pre-existing generated working tree.

Use scene-specific texture/particle closure under `P2-texture-residency.md`; count keys/views, palettes, layout/alignment and exact required cells, not only byte totals. No required effect may be excluded to preserve a full atlas. A spare rectangle is not proof that palette/format or handle limits fit. A representation change is priced across all affected resource classes.

Wire each added generator input/output into the existing build path and configuration signature. Verify dependency rebuild after touching a source input and after switching flags. Missing prerequisites must fail before a long ROM attempt where practical. Preserve required data for deferred constructors and actor teardown.

## Package: native-owner batches

**Outcome:** A coherent group of kinds produces all source-reachable native output, including secondary roots, state variants and child objects.

Use existing generator templates where the source shape matches. Parameterize the genuine differing transform/root/material case rather than cloning shared runtime code. Do not expand a generic runtime N64 interpreter. A makeable Capsule or Pokémon with an absent sibling root remains incomplete.

**Proof:** Host geometry/material/source checks plus natural item lifecycle captures. Include affected siblings when a template changes. Owner registration and triangle totals are intermediate witnesses; assert that expected visible content appears at the right time and place.

## Package: natural interaction and sound

**Outcome:** The actual fighter/input/engagement path performs pickup, hold, swing/fire, throw/drop/catch, damage, break/payout, reflect/absorb and despawn as applicable.

Reuse source item/fighter status implementations already linked. Confirm their real callers are enabled in the candidate. Preserve priority and resolution order, ownership/credit, item-hand transforms, source ammo/timers, conditionally breakable behavior, surface interactions and original bounded allocation. Hammer/Star change fighter/audio state and must restore it correctly at every source exit.

**Proof:** Select/pick up through ordinary input; exercise useful ground and air variants and loss on damage/KO/scene exit. Observe the child weapon/effect and its damage attribution. Hear/inspect actual output, not merely cue request counts. Keep extensive existing host arbitration/table tests; add only missing discriminating cases.

## Package: integration across modes

Random VS spawns do not exercise stage/bonus kinds or guarantee that a thrown Poké Ball opens. Use short natural triggers for each kind family, then the required ordinary items-on stress run. Test the stage caller for Castle GBumper, Inishie POW/Pakkun, Saffron five monsters and Chansey's eggs, Bonus Target and Race GBumper/TaruBomb; Link/Ness exercise their own makers.

Item Switch must implement source row-to-kind mapping, frequency and commit/cancel rules, shell transfer, source unlock gating and save behavior. Do not add toggles for stage hazards just because their kinds are in the same enum. Training item selection is its own source-limited set.

## Exit checklist

- [ ] All 45 kinds and their required state/child roots are accounted for and natively rendered.
- [ ] Exact required asset sets fit; generated outputs rebuild correctly in clean/configuration-changed builds.
- [ ] Actual source callers and natural player interactions work, with correct ownership and cleanup.
- [ ] Audio, Hammer/Star restoration, containers and summon selection/behavior are source-equivalent.
- [ ] Stage, fighter, bonus, Training and VS integration is proved where applicable.
- [ ] Item Switch, required resource/cadence/stress gates and owner reviews pass.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/it/itdef.h: ITKind`.
- `decomp/BattleShip-main/decomp/src/it/itmanager.c: dITManagerProcMakeList`.
- `decomp/BattleShip-main/decomp/src/it/itmain.c`.
- `decomp/BattleShip-main/decomp/src/mn/mnvsmode/mnvsitemswitch.c`.
- `src/import/battleship_item_link_core.c`.
- `docs/p2/P2-texture-residency.md`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-5-items.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-5-items.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
