# Melee Items — Sword, Bat, Fan, Star Rod and Hammer

Complete the existing source item implementation and native output through its natural callers. The parent `../P2-5-items.md` owns shared manager, admission, inventory and integrated stress requirements; this file supplies the class-specific observable contract.

## Source and fixed facts

Use source `itcommon` kind/status data plus the fighter's common swing/Hammer status handlers. `itvars.h` pins `ITSTARROD_AMMO_MAX=20` and `ITHAMMER_TIME=720`; these are source-state quantities. Initial constants do not by themselves specify decrement timing, movement restrictions, head behavior or end conditions—those come from their consumers. Remove the old speculative head-falls-off mechanic as an instruction; only an actual source path can require it.

## Package: ordinary held-weapon states

**Outcome:** Natural pickup/hold/tilt/smash and other source-supported uses produce the correct fighter motion, attached item transform, attack data and native item/effect output, then restore the source base state.

Sword, Bat and Harisen have distinct source attack/reach/sound/rebound/throw behavior. Use source motion and item attack tables; do not add a remembered shield-break or KO multiplier. Verify correct hand attachment during movement, contact versus miss and left/right facing. Bat's distinctive strong-hit output must distinguish its impact cue from the victim voice.

## Package: Star Rod melee and child stars

A swing and its projectile are separate observable outputs with source ammo/event rules. Test non-consuming/consuming source attacks as defined, tilt/smash child trajectory/lifetime, final ammo and empty behavior, then throw/drop. Preserve the projectile's source collision/reflect/absorb callbacks and owner credit; do not infer them just from its appearance.

Required star/impact textures and child weapon must be in the scene manifest even when the held rod itself already has a native model. Mirrors or repeated pickups must not share mutable ammo state.

## Package: Hammer override and restoration

Preserve source fighter-state transition, allowed controls/movement, automatic attack events, warning material/timer and source BGM override. Read the actual Hammer common statuses for ledge/jump/platform rules rather than restricting movement by guess.

Test natural pickup, active walking/source movement, platform/ledge transitions, damage/capture where allowed, KO, source timeout and scene exit. Verify source attack/attribution, held/body output and actual Hammer music, then correct prior BGM and fighter state on every source exit. Test interaction with Star through the common source priority/restore rules rather than inventing a new music stack.

## Proof and exit

Use existing actual-C item/fighter arbitration tests and source tables for thresholds and common priority; natural input proves reachable use, native active-state pixels and audible restoration. A fake held pointer or direct status setter is not the final proof.

- [ ] Sword/Bat/Fan source attacks, attachment/rebound and hit/miss output pass.
- [ ] Star Rod source ammo transitions, melee plus child projectile and empty behavior pass.
- [ ] Hammer source control/timer/attack/material/music lifecycle and all distinct exits pass.
- [ ] Ordinary throw/drop/KO/scene cleanup and shared resource/stress acceptance pass.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/it/itdef.h: ITKind`.
- `decomp/BattleShip-main/decomp/src/it/itvars.h`.
- `decomp/BattleShip-main/decomp/src/it/itmanager.c`.
- `decomp/BattleShip-main/decomp/src/it/itmain.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itsword.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itbat.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itharisen.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itstarrod.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/ithammer.c`.
- `decomp/BattleShip-main/decomp/src/ft`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/items/melee-weapons.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/items/melee-weapons.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
