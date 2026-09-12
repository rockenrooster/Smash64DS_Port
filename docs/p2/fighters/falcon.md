# Captain Falcon — Joint-Attached Effects and Dive/Entry Closure

Completion contract over the existing imported/native fighter. Current state and owner symptoms live on the board/bug queue; no old “not started” or “only feel remains” header is an executable instruction.

## Reuse; do not restart

Keep the source status/motion enum corrections, two-step entry ladder, native owner and detailed audio inventory. Punch/Kick source logic already exists. An effect call or native packet without visible attached output does not resolve the owner's missing Punch/Kick effects.

## Cohesive completion packages

**Falcon Punch.** Natural ground and air Punch must render the source effect at the correct joint/transform, scale, orientation, animation phase and lifetime. Check both facings and movement during the active effect; do not use a fixed world-space offset or move the fighter to suit a probe. Preserve source hit/timing/kinetics.

**Falcon Kick.** Ground, air, landing and rebound/end states must retain the correct body and joint-attached effect. Test transition frames where source parenting/rotation changes; correct attachment at one idle pose is not sufficient.

**Falcon Dive.** Exercise miss, catch, victim capture, throw/release and recovery with actual control input. Preserve the shared victim status and cleanup on interruption or KO.

**Entry and audio.** Preserve source AppearStart→AppearEnd and its facing/display-link changes, entry vehicle/voice and timing. Source cue banks and measured long-sample treatment remain evidence; shared missing sound fixes belong to the shared audio owner.

## Cross-system and lifetime requirements

Changes to the attachment/material/native-program seam need a sibling effect with different transform behavior. Maintain per-instance effect ownership in mirrors. Large/facing-changing motion cannot reuse a stale pose or camera-space transform. Camera/hit behavior is unchanged by a visual repair.

## Natural-path proof and exit

Capture Punch and every materially different Kick state at start/active/end in both facings, with an engaged effect and visible joint-relative result. Run a Dive capture/exit and entry sequence, plus source/state and audio controls. Do not close the fighter as subjective-feel-only while required VFX remain missing.

The shared ordinary-state, CSS/costume/HUD/Results, source-asset comparison, native-only, resource and stress requirements are in `../P2-3-fighter-production.md`. This unit's checklist supplements them; it does not replace them. Read only the current board residual and relevant retained evidence before a repair. A new source/asset/configuration change invalidates the affected proof, not every previously qualified behavior.

- [ ] All source unique behaviors and required child objects have natural input/output/lifetime coverage.
- [ ] The candidate renders body, attachments, materials and effects in the affected active states—not only idle.
- [ ] Required cues/voices are actually audible and stop/restore correctly.
- [ ] Shared unit acceptance, actual resource profiles, cadence/stress and required owner review pass.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/ft/ftchar/ftcaptain`.
- `docs/p2/fighters/falcon.md at the pre-revision snapshot`.
- `docs/p2/P2-3-fighter-production.md`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/fighters/falcon.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/fighters/falcon.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
