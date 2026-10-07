# Handoff

P2 follows `PROJECT_GOAL.md` and `P2_PLAN.md`; `P2_EXECUTION_BOARD.md` owns focus, decisions, artifacts and the
**Execution cursor**. This file is a route, not another task or metric ledger.
**P3 (owner 10-06: multiplayer before the next P95 pass):** `P3_Multiplayer/P3_STATUS.md` -- radio, lockstep, the original-style lobby (every cursor, 1P-4P art, costumes, stage select), build identity and rematch work on 2 and 4 consoles in the melonDS-mp harness, 0 desyncs with CPUs and items on all nine VS stages; hardware validation and the open items are listed there. Next: the P95 pass.

## Current route (2026-09-26)

P2-2p8 four-fighter 30 FPS runs on `p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md` (rulings D1-D9 in its section 8,
phase log in section 6). On resume read its sections 0, 6 and 8, then the board cursor. Evidence:
`artifacts/performance/2026-09-2*_p2-2p8-*`; `scripts/compare-replay-digest.py` is the gameplay-equivalence check.

**State 2026-10-06:** HEAD `f58acc5d92b`-era code is the fastest measured build (official gate
P95 ~1,074K, 96% in two VBlanks; lab sweep worst cases Yoshi's Island ~1.36M, Sector Z ~1.18M).
The uncommitted whole-subsystem soft-float passes measured slower and are parked in
`builds/parked/2026-10-06_softfloat-passes-c15/`. History of the P2 route: `p2/HANDOFF_HISTORY.md`.

**State 2026-10-07 (P95 pass):** clean lab sweep at HEAD (`artifacts/performance/2026-10-07_p95-baseline`):
gate 1,056K, Castle/Zebes/Hyrule/Dream Land pass; Sector Z, Jungle, Saffron, Mushroom Kingdom 1.08-1.20M over
seeds 1-4; Yoshi's Island 1.17-1.37M. Yoshi seed 1's overrun is one Meowth Pay Day brawl (frames ~1430-1590:
up to eight coins against four fighters, +293K ticks a frame over the window before it, about half of it exact
weapon-vs-hurtbox tests inside the source's own hit_detect_range). Skipping each Yoshi's Island stage layer
whole prices it at P95 -21K / -11K / -5K / -9K (layers 0-3, `2026-10-07_yoshi-layers`). N64 ground truth for
visual rows: the vanilla ROM in mupen64plus, cheat-booted into a scene (`p2/BUG_NOTES.md`, Master Hand).

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

Record unavailable helpers and denied Git operations with their retry condition. Preserve owner
edits and qualified artifacts; a doc change does not repair a failure or establish a game PASS.
Bug-sweep lessons (2026-09-19..22): `p2/BUG_NOTES.md` "Standing lessons".
