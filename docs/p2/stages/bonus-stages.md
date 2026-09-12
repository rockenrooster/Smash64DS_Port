# Bonus 1 and 2 — Twenty-Four Boards, Two Source Rule Sets

Reuse the imported bonus scene and existing generated board packets. Mario's two boards prove the common runtime first; the other twenty-two are source-profile qualification, not twenty-two independently rewritten game modes.

## Board identity and source scope

The acceptance domain is each of the twelve playable fighters × Break the Targets and Board the Platforms. Resolve source ground/profile, geometry, object placements, camera/bounds, spawn, backdrop/music and associated fighter from the actual source tables. A packet count can include other profiles; do not call it “24 boards accepted.”

| Fighter | Required profiles |
|---|---|
| Mario, Fox, Donkey Kong, Samus | Each one's Bonus 1 and Bonus 2 |
| Link, Yoshi, Captain Falcon, Luigi | Each one's Bonus 1 and Bonus 2 |
| Kirby, Pikachu, Jigglypuff, Ness | Each one's Bonus 1 and Bonus 2 |

Use source IDs as authoritative; labels are for readability. Keep per-board evidence identifiers in the existing inventory/board rather than a manually duplicated tick table here.

## Package: Mario shared rules and natural callers

**Outcome:** Campaign inserts Mario's two source boards; Bonus Practice selects them through its natural menu, with correct distinct timer/score/record semantics.

For Break the Targets, preserve source ten-task target identity, attack eligibility, hit/disable/remaining count, completion and failure. The Target item/native effect is a required P2-5 dependency. Source collision determines which attacks can reach each target; do not make every arbitrary visual overlap a hit.

For Board the Platforms, preserve source platform identity, actual eligible landing/standing predicate, claim-once state, material/visual feedback, remaining count and completion. Do not implement a guessed dwell timer or merely touching a side/bottom as boarding. Read `sc1pbonusstage` and the source bonus ground/item callbacks for the real predicate.

**Proof:** Ordinary movement/attacks complete each Mario board; source failure/timeout where applicable, retry and immediate repeat entry reset all object flags/counters. Hit/land negative controls demonstrate no false claim. A force-complete variable is a harness aid, not final gameplay proof.

## Package: all board profiles

**Outcome:** Every fighter's original movement puzzle, required objects/visuals and camera work with its actual move set.

Validate all source geometry/object-count/identity profiles on the host, then clear all 24 naturally (scripted real input or owner play). Batch shared generator/asset repairs; diagnose whether an unreachable target/platform is wrong board data, collision or fighter movement/recovery. Link the fighter-owned defect rather than altering a board to accommodate it.

Include source projectile/recovery/multijump/collision differences; do not prove Ness's board with Mario's movement. All required materials/target/platform state variants must be in the admitted closure.

## Package: results, records and progression

Campaign and practice have different source consumers of time/completion. Preserve source tally, best-time update comparison, partial-task records, failure state and Sound Test/Luigi unlock events. P2-7 owns persistence/predicates; this scene must emit the real event exactly once.

Test existing best versus improved/worse result, incomplete/failure, final-task completion, repeat entry and fresh process save reload through the actual practice/campaign path. Do not write records during uncontrolled active gameplay storage work.

## Final proof

- [ ] All 24 source profile identities/objects/collision/bounds and native assets pass host validation.
- [ ] Each board can be fully cleared with its proper fighter and fails/restarts according to source.
- [ ] Claimed/broken state, remaining count, visuals/audio, timer and scene return are correct.
- [ ] Campaign insertion, practice selection, records/unlock event and save reload work.
- [ ] Repeated entries, resource limits, 30 Hz presentation and applicable owner review pass.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstage.c`.
- `decomp/BattleShip-main/decomp/src/gr/grbonus`.
- `decomp/BattleShip-main/decomp/src/it/itground`.
- `docs/p2/P2-6-one-player.md`.
- `docs/p2/P2-7-modes-meta.md`.
- `docs/p2/P2-5-items.md`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/stages/bonus-stages.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/stages/bonus-stages.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
