# P2-3 — Finish the Roster on the Existing Native Pipeline

The source/asset/native production pipeline is established. Finish the remaining observable fighter surfaces and cross-fighter capabilities. Original Luigi→DK→…→Kirby import order is historical lineage, not today's queue.

## Pipeline contract and reuse

Keep the source-derived `fighter_production_manifest.json`, admission tool, status/attribute normalization, animation/native-owner generators and staged audio/UI products. Build tooling may be shared; runtime specialization is encouraged. Do not demand a new universal abstraction whenever a source fighter needs an explicit variant parameter.

The manifest owns source identity, core/dependency and motion inventories, item animations, status descriptors, models/materials, UI and audio roots. A fighter's closure includes its source-named dependencies even when they use another fighter's name. Generated resources need real Makefile/build prerequisites; clean and configuration-changed builds cannot depend on leftover local outputs.

## Unit owners

| Fighter | Unit file | Special coverage beyond common states |
|---|---|---|
| Luigi | `fighters/luigi.md` | Divergent Mario-family specials; shared cues; entry |
| Donkey Kong | `fighters/dk.md` | Cargo two-body lifecycle; stored charge |
| Captain Falcon | `fighters/falcon.md` | Entry ladder; moving-joint Punch/Kick; Dive capture |
| Samus | `fighters/samus.md` | Charge lifetime; Bomb/bomb-jump; native roll/Bomb body |
| Link | `fighters/link.md` | Returning boomerang; held bomb item; model-part draw changes |
| Pikachu | `fighters/pikachu.md` | Terrain Jolt; Thunder; two-segment recovery; entry |
| Yoshi | `fighters/yoshi.md` | DL pairs; Egg Shield; Egg Lay/capture; armor |
| Ness | `fighters/ness.md` | Controlled Thunder/self-launch; Fire pillar; absorption |
| Jigglypuff | `fighters/jigglypuff.md` | Multijumps; Sing/sleep; Rest; shared animation resolution |
| Kirby | `fighters/kirby.md` | Inhale/spit; copy set/hat/material/article closure; Stone/Cutter |

Mario/Fox retain their P1 evidence but are not exempt from shared native-renderer fixes or current-state coverage. Campaign variants and Master Hand reuse the pipeline but their acceptance belongs to P2-6.

## Package: shared draw-state coverage

**Outcome:** Every required reachable fighter draw state has a correct native output, including guard/roll/grab, specials, model-part swaps, animlocks and child effects.

Work at the owning native adapter/generator/ABI seam. Separate selected status, selected parts, source detail, pre/post DL-pair semantics, foreign model-part ownership, material/alpha and runtime pose. A failed native-plan prevalidation must be visible even when the later validator is never called. An owner claiming a list without output is not successful coverage.

**Proof:** Natural inputs that engage the changed state plus a compact sibling set spanning ordinary owner, non-prefix part mask, DL-pair/pre-matrix owner, cross-matrix owner and foreign-modelpart case as relevant. Capture required body/effect pixels during entry, active state and exit, not only at idle. Fix shared shield/catch/attachment problems once; preserve unchanged gameplay.

## Package: remaining fighter-specific outcomes

Use each unit's source-pinned test matrix. Retain qualified DK cargo, Samus charge/bomb-jump, Link bomb and other applicable proofs. Reopen only the affected dimensions when code/assets/configuration changed. A current owner symptom defeats an old “only feel remains” label.

A coherent batch can fix a common generator defect across multiple fighters and then qualify the affected states. Conversely, unrelated defects should not be welded into a giant “finish all fighters” task. Before changing a numeric behavior, resolve it from the actual US source table/function rather than a moveset sketch.

## Package: articles, copied abilities and shared items

Map every reachable weapon/item/effect to its creator, asset root, native owner, lifetime, attack/attribution and cleanup. Item-owned Link bomb and Ness Fire pillar reuse P2-5; they are not fighter-local pseudo-items. Donor flags or optional constructors cannot silently disable required Kirby copies.

Kirby's base, copy capabilities and costume/modelpart resources must use the scene's actual legal closure. Prove copy gain/use/loss and any source transfer rule before omitting a donor resource. Separate implementation-TU count from donor-power count and identity/hat rows. Profile campaign Kirby Team separately from ordinary VS.

## Package: selectable and complete fighter

Preview lifetime is P2-1; battle residency is P2-2. Availability from engineering admission is not the same as save unlock state. Development may expose testable content under an explicit lab configuration; production uses the verified feature set and source save masks. Do not mark an unfinished fighter accepted merely because its cell is clickable, or permanently disable required content to avoid its blocker.

Required per-fighter surfaces: source entry, CSS idle/selected pose/name/voice, all applicable costumes/team shades, normal movement and attacks, guard/dodge/grab/throw, specials/articles, ledge/down/damage/KO/respawn, HUD/tags and Results. Item pickup/hold/swing/fire/throw animations and behavior are part of each applicable fighter's contract once items are enabled.

## Proof and closure

Use the existing move-inventory and replay machinery; natural control input reaches the source status. Positive engagement is required for every asserted family. Host table/ABI comparisons are valuable, but neither an idle CPU match nor a masked-off route proves its missing states. Preserve mechanical equivalence rather than imposing an unrequested bit-exact numeric redesign.

- [ ] Reproducible generator/admission output and complete source/asset inventories.
- [ ] Each fighter's common and unique state matrix has natural-path behavior and native-output evidence.
- [ ] All relevant child weapons/items/effects and audio are covered, with cleanup/ownership correct.
- [ ] CSS, costumes, HUD, KO/respawn and Results remain valid on the candidate.
- [ ] Actual resource profiles fit; required stress/cadence and subjective owner checks pass before acceptance.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `scripts/fighters/generate_fighter_production_manifest.py`.
- `scripts/fighters/fighter_production_manifest.json`.
- `scripts/fighters/admit_fighter.py`.
- `scripts/fighters/generate_nds_native_owners.py`.
- `src/port/renderer_adapter_fighter.c`.
- `decomp/BattleShip-main/decomp/src/ft/ftdata.c`.
- `decomp/BattleShip-main/decomp/src/ft/ftcommon`.
- `decomp/BattleShip-main/decomp/src/wp`.
- `decomp/BattleShip-main/decomp/src/it/itfighter`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-3-fighter-production.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-3-fighter-production.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
