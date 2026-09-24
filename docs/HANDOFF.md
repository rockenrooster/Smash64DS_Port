# Handoff

The destination is P2 under `PROJECT_GOAL.md` and `P2_PLAN.md`.
`P2_EXECUTION_BOARD.md` owns live focus, decisions, artifacts and the
**Execution cursor**. This file is a route, not another task or metric ledger.

## Current route (2026-09-24)

P2-2p8 four-fighter 30 FPS runs on `p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md`
(rulings D1-D9 in its section 8, phase log in section 6). On resume read its
sections 0, 6 and 8, then the board cursor. Evidence:
`artifacts/performance/2026-09-2*_p2-2p8-*`; `scripts/compare-replay-digest.py`
is the gameplay-equivalence check every phase runs.

Phase 1 (fighters): slices 1-6 landed; slice 7 is ready for its candidate
checkpoint (**IMPLEMENTED_NOT_ACCEPTED**). Lean defaults and Ness's yo-yo/bat
are covered on the final ROMs; the Results/CSS tint-lifetime repair has natural
transition probes and inspected A/B captures. Receipt:
`artifacts/performance/2026-09-24_p2-2p8-phase1-slice7/README.md`. All jobs
finished. Root `smash64ds.nds` still matches r54 (C8FC02AA). Next is slice 8:
the separate camera modelview and transient Intro actors, variant kinds and
remaining D8 scene coverage. CSS/Results already reach the common lean path;
their full roster coverage remains due. Then slice 9 deletes production
everywhere, followed by Phase 2. Boundary preflight can rebuild the root ROM,
so preserve r54 until publication is qualified. D9: no re-plan
stops -- keep going and bank every measured win. Owner: optimization only, no 1P
campaign bugs. Carry forward: the shipping configuration cannot load the heaviest four-kind roster
(Captain/Link/Pikachu/Kirby halts ~130 KB short at battle load,
`artifacts/performance/2026-09-23_p2-2p8-shipping-heap-census/`), and the four-CPU
gate ROM has ~0.2 MB more arena than the shipping image -- report the shipping
arena and that roster's load margin with every phase. The all-content
(published) VS character select must keep >= 183,072 B free at its animation
reservation or every 3D preview switches off (188,368 B with slice 4 and compaction;
`artifacts/performance/2026-09-23_css-preview-heap/`). Serial execution only:
**no subagents** (owner, 2026-09-24).

## Continue, do not restart

With an intact context, execute the cursor's next unfinished action. Do not
repeat startup, capability discovery, task selection, profiling or completed
verification merely because the goal is repeated or a turn ended.

After context loss, read the cursor and its linked batch receipt once. Reconcile
only relevant source/asset/config changes and an owned live job before relying
on that state. Missing identity blocks reuse of that proof, not unrelated safe
work. A settled experiment needs a named new invalidator to reopen.

## Owners

`VERIFYING.md` owns command lifecycle, test validity and publication. For P2-2p8,
`p2/native-optimization/13_AGENT_EXECUTION.md` defines cursor phases and fields.
Read only contracts needed by the next action. The board chooses the next batch
only after recording an outcome, a concrete blocker or an owner priority change.

Keep unavailable helpers and denied Git operations recorded with their retry
condition; use permitted serial implementation instead of repeating setup.
Preserve owner edits and qualified artifacts. A documentation change does not
repair a transport failure, authorize denied access or establish a game PASS.

Bug-sweep lessons (2026-09-19..22): `p2/BUG_NOTES.md` "Standing lessons".
