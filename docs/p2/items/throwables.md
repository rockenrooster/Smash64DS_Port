# Self-Acting Items — Shells, Bob-omb, Mine and Bumper

Complete the existing source item implementation and native output through its natural callers. The parent `../P2-5-items.md` owns shared manager, admission, inventory and integrated stress requirements; this file supplies the class-specific observable contract.

## Source distinctions

These common items use `itcommon` behaviors. NBumper (ordinary item) and GBumper (stage/bonus item) are distinct source kinds; share implementation only where their actual source behavior agrees. Do not copy the ordinary placement/lifetime into Castle or Race.

Useful pinned parameters in `itvars.h`: Green Shell lifetime 240 and damage-all wait 32; Red Shell lifetime 480 and damage-all wait 16; Bob-omb walk-wait 180 and flash-wait 480; Motion-Sensor Bomb detection delay 100. These are named source fields with source decrement/entry semantics, not general wall-time promises. For the mine's radius, use the actual squared-distance comparison; the macro is already squared.

## Package: shells

**Outcome:** Natural pickup/throw/hit activation, sliding/patrol/source steering, rebound, interactions, lifetime/despawn and damage attribution render/behave correctly.

Use each shell's own source target, floor/air and owner/team-damage delay rules. Test initial owner immunity versus later source damage-all state, target changes, repeated hits and rebound, slope/moving floor contact and loss of floor. Do not infer ownership decay merely from a bounce, or replace source target selection with “nearest fighter” unless the code establishes it.

Body rotation, dust/trail/impact and source sounds have native routes and admitted lifetimes. Red and Green are not interchangeable parameter rows when callbacks differ.

## Package: Bob-omb and Motion-Sensor Bomb

Bob-omb must follow its source idle→walk→warning/explosion branches, damage/throw activation and child smoke/blast behavior. Test source interruption/chain reactions and cleanup. Exact state transition conditions matter more than a guessed total wait obtained by adding every constant.

Mine placement/attachment, arming, detection, visibility and blast follow source surface/status rules; do not add wall/ceiling sticking from an uncertain sketch. Verify source eligible target and threshold cases, and native low-visibility treatment without making a required telegraph wholly invisible.

## Package: NBumper and contextual producers

NBumper source placement/contact/despawn and attack attribution must work through the item path. GBumper stays under the shared P2-5/stage contracts and gets source animation/placement at Castle/Race callers.

The old document proposed Bob-omb rain in Sudden Death without verifying this version's source. Before adding any scripted rain, inspect the actual source sudden-death spawn path. Preserve it when present; do not introduce behavior from another Smash title to satisfy that old sentence. A context producer is separately qualified from an item's isolated lifecycle.

## Proof and exit

Use host transition/timer/threshold tests and natural item use for each family. Capture the actual motion/warning/blast/dust, source damage/credit and sound, then another spawn or scene entry to prove teardown. Legal concurrent actors define the resource workload; no arbitrary pool reduction.

- [ ] Shell motion/target/floor, owner/team-delay, lifetime and native child output pass.
- [ ] Source Bob-omb/mine activation, warning/detection/blast and cleanup pass.
- [ ] Ordinary versus stage Bumper behavior and real contextual producers remain distinct and correct.
- [ ] Source assets/audio, relevant siblings, resource profile and stress/cadence acceptance pass.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/it/itdef.h: ITKind`.
- `decomp/BattleShip-main/decomp/src/it/itvars.h`.
- `decomp/BattleShip-main/decomp/src/it/itmanager.c`.
- `decomp/BattleShip-main/decomp/src/it/itmain.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itgshell.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itrshell.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itbombhei.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itmsbomb.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itnbumper.c`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/items/throwables.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/items/throwables.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
