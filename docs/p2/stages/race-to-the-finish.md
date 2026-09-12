# Race to the Finish — Source Course, Hazards and Completion

This replaces the old unsupported multiple-exit/score-tier and forced-scroll sketch. Use the existing Bonus3 source import and packet, and preserve the actual course and camera/scoring consumers rather than implementing another game's Race rules.

## Verified source behavior

`grBonus3FinishProcUpdate` gets the source player fighter and completes only when `fp->ga == nMPKineticsGround` and `(fp->coll_data.floor_flags & MAP_VERTEX_MAT_MASK) == nMPMaterialDetect`. It queues the source completion announcement and bonus-complete sound. This function contains no exit-tier selector. Camera/timer/scoring are determined by their actual campaign/camera consumers, not inferred from that predicate.

`grBonus3MakeBumpers` walks source DObj descriptors, creates `nITKindGBumper` through the stage-parent item path and applies the supplied source joint animation. `grBonus3TaruBombMakeActor` requires exactly one `nMPMapObjKind1PGameBonus3TaruBomb` position. Its update uses an initialized/reset wait of 180 source tics to spawn `nITKindTaruBomb`. Use the actual decrement/update ordering in `grBonus3TaruBombProcUpdate`, not a guessed wall-time interval.

## Package: course/profile and setup

Pin source geometry, detect-material finish surface, camera/blast bounds, spawns, background/material, audio and campaign enemy/trait setup. Validate the exact TaruBomb map-object count before source entry so malformed data cannot enter the source fatal loop. Preserve source map/item base offsets for bumper/bomb attributes and animation.

Reuse the registered course/native packet and existing Polygon admissions as required by the actual campaign setup. Do not remove enemies or source items to fit a temporary preview/heap profile. Account for mandatory future spawns in the admitted set.

## Package: visible interactive hazards

Native animated GBumpers must coincide with their real source collision/knockback and move through the full source animation. Native TaruBomb spawn/motion/hit/explosion/cleanup must follow its source item table and correct credit. Required children/effects/audio belong in the texture/item manifest; a source spawn mask is not proof that players see the hazard.

Test natural collision/avoidance, multiple spawn cycles, source actor lifetime and course progression. Source three-enemy/trait configuration, where selected by the campaign table, needs its actual behavior and output, not stand-ins.

## Package: finish, fail and reset

**Outcome:** A natural full course run reaches the grounded Detect-material surface, ends via source interface/tally and preserves correct run state; source timeout/KO/failure also routes correctly.

Test near-finish air/side contact as negative controls where they do not satisfy the source grounded-floor condition; do not force a completion simply for entering a screen rectangle. Confirm exactly-once announcement/tally despite a condition potentially remaining true across updates. Source camera follows its actual mode; do not add forced-scroll or new exit scoring from the withdrawn sketch.

Repeat after success and failure. Reset source counters, finish state, bumper animations, bombs/children and scene-local allocations; no actor from the previous run survives. P2-6 owns tally/progression and P2-7 the real record/save consumers where applicable.

## Acceptance

- [ ] Source course/collision/finish material, profile, camera/timer/scoring consumers and required spawns are identified and reproduced.
- [ ] Animated GBumpers, TaruBombs, source opponents and required children are natively visible, interactive and audible.
- [ ] Full natural finish and source failure→tally/return work with correct state and exactly-once events.
- [ ] Repeated runs reset correctly, all required resources fit, cadence holds and owner review is banked.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/gr/grbonus/grbonus3.c: grBonus3FinishProcUpdate, grBonus3MakeBumpers, grBonus3TaruBombMakeActor, grBonus3TaruBombProcUpdate`.
- `decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c: Bonus3 entry`.
- `decomp/BattleShip-main/decomp/src/gm/gmcamera.c`.
- `docs/p2/P2-6-one-player.md`.
- `docs/p2/P2-5-items.md`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/stages/race-to-the-finish.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/stages/race-to-the-finish.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
