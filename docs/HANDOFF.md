# Handoff

The destination is P2 under `PROJECT_GOAL.md` and `P2_PLAN.md`.
`P2_EXECUTION_BOARD.md` owns live focus, decisions, artifacts and the
**Execution cursor**. This file is a route, not another task or metric ledger.

## Current route (2026-09-23)

P2-2p8 four-fighter 30 FPS runs on `p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md`
(owner rulings in its section 8, phase log in section 6). On resume read that
document's sections 0, 6 and 8, then the board cursor. Phase evidence lives under
`artifacts/performance/2026-09-2*_p2-2p8-*`; `scripts/compare-replay-digest.py`
is the gameplay-equivalence check every phase runs.

Phase 0 closed 2026-09-23; Phase 1 (fighters) is active. Carry into every
phase: the shipping configuration cannot load the heaviest four-kind roster
(Captain/Link/Pikachu/Kirby halts ~130 KB short at battle load,
`artifacts/performance/2026-09-23_p2-2p8-shipping-heap-census/`), and the four-CPU
gate ROM has ~0.2 MB more arena than the shipping image -- report the shipping
arena and that roster's load margin with every phase. The all-content
(published) VS character select must keep >= 183,072 B free at its animation
reservation or every 3D preview switches off (192,464 B after Phase 1 slice 4;
`artifacts/performance/2026-09-23_css-preview-heap/`). One subagent at a time
(owner, 2026-09-22).

## Continue, do not restart

With an intact context, execute the cursor's next unfinished action. Do not
repeat startup, capability discovery, task selection, profiling or completed
verification merely because the goal is repeated or a turn ended.

After context loss, read the cursor and its linked batch receipt once. Reconcile
only relevant source/asset/config changes and an owned live job before relying
on that state. Missing identity blocks reuse of that proof, not unrelated safe
work. A settled experiment needs a named new invalidator to reopen.

An unseeded cursor is initialized once from the newest applicable local evidence
and relevant diff. Replace the unseeded state immediately; do not leave a standing
instruction to rediscover a named historical candidate. A newer live cursor wins
over this documentation package's migration seed.

## Owners

`VERIFYING.md` owns command lifecycle, test validity and publication. For P2-2p8,
`p2/native-optimization/13_AGENT_EXECUTION.md` defines cursor phases and fields.
Read only contracts needed by the next action. The board chooses the next batch
only after recording an outcome, a concrete blocker or an owner priority change.

Keep unavailable helpers and denied Git operations recorded with their retry
condition; use permitted serial implementation instead of repeating setup.
Preserve owner edits and qualified artifacts. A documentation change does not
repair a transport failure, authorize denied access or establish a game PASS.

## Standing lessons

Route markers from the 2026-09-19..22 bug sweeps live in `p2/BUG_NOTES.md`
("Standing lessons, 2026-09-21" and "... 2026-09-21 night").
