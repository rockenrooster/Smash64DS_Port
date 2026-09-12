# Held Shooters — Ray Gun and Fire Flower

Complete the existing source item implementation and native output through its natural callers. The parent `../P2-5-items.md` owns shared manager, admission, inventory and integrated stress requirements; this file supplies the class-specific observable contract.

## Verified source quantities and reuse

`itvars.h` pins Ray Gun's initial ammo at 16 and Fire Flower's at 60; Fire Flower child ammo lifetime is 30. The source use/event callbacks decide when ammo is consumed and how many projectiles are emitted. Do not equate Fire Flower's 60 units with 60 rendered frames or a guessed one-second spray. Source motion/status/event timing remains authoritative.

Reuse `itlgun.c`, `itfflower.c`, common fighter item-use states and existing child weapon/native effect owners. Star Rod is owned by `melee-weapons.md`, with the same child-weapon ownership discipline.

## Package: natural held use

**Outcome:** Real pickup/hold/fire input reaches the source shooting state, hand/muzzle transform, ammo updates and visible child emission.

Test every materially different source-supported ground/air/moving/facing state by its actual status table; do not invent walking-fire support or forbid it without the source. Body/item attachment remains valid during firing, recoil, interruption and return. Source empty behavior and final-ammo transitions must work before throw/drop.

## Package: projectile and spray

Ray Gun bolt trajectory, speed, collision, material/alpha, impact and damage credit come from its source weapon. Fire Flower's short-lived children require correct source emission spacing, hit behavior and native output while several coexist. Preserve source hit/rehit/resolution order rather than summing many hits into one convenient update.

Verify actual reflect/absorb classification from flags and callbacks with an allowed positive and disallowed control. Do not classify a whole family by “energy” appearance. Re-owner a reflected child as source specifies and preserve item/thrower damage credit correctly.

All child textures/palettes/particles and muzzle/impact cues are required scene roots. A loaded gun or Flower model does not prove its spray/bolt assets fit. Use the source bounded pool semantics; do not reduce legal children without approval to pass the frame gate.

## Package: ammo, interruption and teardown

Test full→one remaining→empty, use interrupted by damage/capture/KO as permitted, throwing an empty item, pickup by another fighter and scene exit. Mutable ammo belongs to the actual item. Live children keep valid source owner/asset lifetime even if the held item changes state or despawns.

## Proof and exit

Host source-event and ammo-boundary checks plus ordinary firing establish behavior and reachability. Capture visible bolt/spray and body over start/active/end; inspect actual impact/reflect/absorb and audio. Measure the legal active burst, not only an idle item.

- [ ] Natural use, hand/muzzle attachment, source ammo/empty transitions and interruption pass.
- [ ] Bolt/spray motion/hit timing, source classification and attribution pass.
- [ ] Native child/impact output and actual audio remain complete at legal concurrency.
- [ ] Cleanup, scene residency and required cadence/stress acceptance pass.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/it/itdef.h: ITKind`.
- `decomp/BattleShip-main/decomp/src/it/itvars.h`.
- `decomp/BattleShip-main/decomp/src/it/itmanager.c`.
- `decomp/BattleShip-main/decomp/src/it/itmain.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itlgun.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itfflower.c`.
- `decomp/BattleShip-main/decomp/src/ft/ftcommon`.
- `decomp/BattleShip-main/decomp/src/wp`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/items/ranged-weapons.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/items/ranged-weapons.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
