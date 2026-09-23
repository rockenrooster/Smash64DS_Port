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
arena and that roster's load margin with every phase. One subagent at a time
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


## Standing lessons, 2026-09-21

Evidence: `artifacts/performance/2026-09-19_remaining-bugs.md` and the commits
that landed each row. Route markers only.

**Census a shared path before changing it.** Three regressions in one day shared
one shape: a repair validated against ONE user of a path many reach. All were
correct for their own consumer with green mutation tests; none was caught by CI.
A mutation test proves the repair for its case and says nothing about siblings.
When a sibling breaks, **scope, do not revert**.

**Static routing is not a runtime load.** Packing 142 Results animations closed
47/47 cells and changed nothing on screen. Where a load can fail silently, the
counter that observes it is part of the repair.

**Re-derive a recorded premise before building on it.** Six of nine rows started
from a wrong fact already in this repo. One grep each would have caught them.
Failure codes are names, not explanations: read the enum.

**`ll*` symbols are linker-absolute -- the ADDRESS is the offset.** Registry uses
need a real `extern` plus a `sNdsKnownAssetSymbols` row; offset uses need the
address-as-offset define; descriptors addressing another file need a span-check
exemption. Recurred four times.

**Falsifiers must follow the producer**, never a copy of its old number.

**A compact pack must keep what its lists LOAD, not only what its structs
point at.** Link's CSS boots drew gray for weeks: the preview pack kept Model
bytes from the first MObj image onward, a display-list TLUT before that mapped
to NULL, and a NULL native image draws untextured with zero rejects (2026-09-22,
`p2/BUG_NOTES.md` "C6"). `test_native_texture_loads_are_retained` guards it.
