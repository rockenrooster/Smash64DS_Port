# Consumed Items — Tomato, Heart and Star

Complete the existing source item implementation and native output through its natural callers. The parent `../P2-5-items.md` owns shared manager, admission, inventory and integrated stress requirements; this file supplies the class-specific observable contract.

## Verified source quantities

`itvars.h` defines Tomato heal 100, Heart heal 999, Heart gravity 0.25 and terminal speed 30, and Star invincibility counter 600. Use the source reader's clamp, tick, acquisition and warning behavior; do not replace the Heart's heal request with an unconditional “set damage to zero” without proving equivalence in the actual supported domain. Source simulation counters are not presented-frame counts.

These kinds live in `itcommon`. Star is a fighter-state/color/audio override, not a passive with no combat-state implications. Reuse the existing item and hit-status/colanim machinery.

## Package: Tomato and Heart acquisition

**Outcome:** The natural source pickup/contact path accepts eligible fighters, applies the source heal amount/timing/clamping, consumes the item once and updates damage/HUD/output correctly.

Preserve source acquisition priority and input/contact semantics; do not assume every healing item automatically heals on arbitrary touch. Test low damage, heal boundary, above boundary, simultaneous eligible participants and ineligible/captured states through the source common acquisition rules. The item must not heal twice when both item/fighter callers run.

Heart uses its source slow fall and surface behavior; native sprite/model and alpha must match the falling/consumed state. Test landing and an affected moving platform if shared surface code changes. Required source cue/effect and particle membership are part of the result.

## Package: Star state and audio lifecycle

**Outcome:** Source collection starts the right invincibility/color/music behavior, preserves source interactions with existing hit-status/armor/shield states, and ends/restores correctly.

Test active duration and warning/end boundary, repeated collection according to source rules, damage/KO/respawn behavior, Hammer interaction and scene exit. Keep source priority of temporary/timed hit-status and colanim overrides; do not treat ordinary respawn invulnerability as a drop-in identical timer.

Source Star moves/bounces before collection; prove that state too. Native rainbow/flash output must be readable and source-derived; reducing it to an arbitrary palette blink needs a measured/approved delta, not the old “approximate” note.

## Proof and exit

Host boundary/source tests price heal and timer/priority semantics cheaply. Natural pickup/contact with visible damage/HUD, actual state/cue and complete Star start/active/end cycle proves the caller. Use existing audio checks for correct return of background music.

- [ ] Source eligibility, healing, clamping, one-time consumption and Heart motion pass.
- [ ] Star source status/color/music acquisition, duration/stacking/exit behavior pass.
- [ ] Native item/effect output, actual audio and resource/scene cleanup pass.
- [ ] Relevant sibling and parent cadence/stress/owner acceptance is banked.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/it/itdef.h: ITKind`.
- `decomp/BattleShip-main/decomp/src/it/itvars.h`.
- `decomp/BattleShip-main/decomp/src/it/itmanager.c`.
- `decomp/BattleShip-main/decomp/src/it/itmain.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/ittomato.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itheart.c`.
- `decomp/BattleShip-main/decomp/src/it/itcommon/itstar.c`.
- `decomp/BattleShip-main/decomp/src/ft/ftparam.c: hit-status and colanim aggregation`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/items/passives.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/items/passives.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
