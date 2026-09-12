# Containers — Crate, Barrel, Capsule and Egg

Complete the existing source item implementation and native output through its natural callers. The parent `../P2-5-items.md` owns shared manager, admission, inventory and integrated stress requirements; this file supplies the class-specific observable contract.

## Source and reuse

The source implementations live under `it/itcommon`, not `itground`. Reuse existing manager/payout/status/physics and native generator work. Source Box/Taru/Capsule/Egg are four common kinds; source-created eggs from a monster still consume the same required kind/child closure. A source maker and one root drawing do not establish every break/explode variant.

## Package: complete normal and explosive state output

Inventory each descriptor's DObj roots, child/sibling roots, status-specific material/animation and fragment/explosion effects. Capsule's structural case must be described from its real roots rather than hidden in a generic “all containers identical” assumption. Every selected root and required breakup piece has native output and an admitted asset lifetime.

Prove appearance/idle, held and thrown/drop motion, impact/damage break, non-explosive payload and source-selected explosive outcome. Use source break thresholds and attack events; constants in `itvars.h` include Box health 15 and Taru health 10, with different explosion-frame constants. These are source logic units/fields, not a rule to advance once per presented frame.

## Package: carry and barrel motion

Preserve the source heavy versus light pickup/hold/walk/throw state selection, attached transforms, pickup priority and control/movement restrictions. Do not synthesize an item in a hand and call that pickup proof. Test ordinary carry, release, interruption, source drop rules and KO cleanup.

Taru's rolling/rotation, slope/surface transitions, damage hitbox and source lifetime/despawn differ from a thrown Capsule. Test ground contact and an affected moving platform; do not use a world-space spin divorced from the source motion.

## Package: payout, fragments and attribution

Read `itbox.c` and common payout setup/callers for actual payload count/rate/explosive decisions. Do not carry unverified fixed “three items” or “one item” prose into a hand-coded replacement. The payload must use the real source item manager, correct positions/velocities and bounded allocation behavior.

Test both payout and explosive cases, child-item pickup, item-on-item damage where source permits, fragment/explosion output, sounds and damage/credit/self-interaction. Failure due to a missing payload/particle root returns to the shared admission owner—not a fake damage-only egg or silent deletion.

## Natural proof and exit

Host checks cover source tables, random-choice boundary cases and generator roots; a short ordinary gameplay route proves carry→throw→break→payload and a source explosive outcome. For eggs, also exercise the natural monster/stage producer. Record expected versus actual state, body/children/pixels/audio and cleanup; don't wait through repeated random matches hoping for a rare branch.

- [ ] Every container state/root and required fragments/explosion/payload is native and source-derived.
- [ ] Source heavy/light pickup, attached movement, throw/drop and barrel surface motion pass.
- [ ] Actual payout/explosive decisions and damage ownership remain source-equivalent.
- [ ] Payload/fragment cleanup, scene resource profile and parent acceptance gates pass.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/it/itdef.h: ITKind`.
- `decomp/BattleShip-main/decomp/src/it/itvars.h`.
- `decomp/BattleShip-main/decomp/src/it/itmanager.c`.
- `decomp/BattleShip-main/decomp/src/it/itmain.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itbox.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/ittaru.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itcapsule.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itegg.c`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/items/containers.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/items/containers.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
