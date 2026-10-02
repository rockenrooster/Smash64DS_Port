# Handoff

P2 follows `PROJECT_GOAL.md` and `P2_PLAN.md`; `P2_EXECUTION_BOARD.md` owns focus, decisions, artifacts and the
**Execution cursor**. This file is a route, not another task or metric ledger.

## Current route (2026-09-26)

P2-2p8 four-fighter 30 FPS runs on `p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md`
(rulings D1-D9 in its section 8, phase log in section 6). On resume read its
sections 0, 6 and 8, then the board cursor. Evidence:
`artifacts/performance/2026-09-2*_p2-2p8-*`; `scripts/compare-replay-digest.py`
is the gameplay-equivalence check every phase runs.

Phase 1 (fighters): slices 1-7 through `98ebd1e2e51` (slice 7 IMPLEMENTED_NOT_ACCEPTED;
receipt `artifacts/performance/2026-09-24_p2-2p8-phase1-slice7/README.md`). Root is still r54.
**Owner 09-27: VS Mode first, 1P deferred. Any 4 fighters on any stage hit the P95 gate
with items on over a full 1-minute match (plus sudden death); CSS and SSS 100%; Results
and transitions/loading seamless. CSS/SSS audio delay + in-match SFX glitches logged (BUG_NOTES A1/A2).**
**Phase 2/3:** all-VS stage compilation, NDL, MISC owners, view-space particles (receipt
`2026-09-26_p2-2p8-phase2-particle-ab`); A8 BGM `d0d02c61a83` (`2026-09-26_p2-2p8-phase3-residency`).
Global renderer retirement remains debt. Bank measured battle wins (D9); no restarts.
**09-26..30 solo** (receipts `2026-09-{26..30}_p2-2p8-*`). **09-30 owner checkpoint: optimization
paused "not 100% complete"; the 1P campaign is next.** Gate WORK P50/P95 922,240/1,240,256 (target P95
1.12M, RED; `94ea559f062`); lab P95 1.21-1.58M by roster/stage; SZ owner roster 1.37M. 09-30 landed: baked
native owners (`1fe468cc3bb`), transition hold (owner r64, `VERIFIED-hold.md`), world-cache GO retry. Playtest
r65b. Open: owner's rematch VFX loss (not reproduced on 3 stages/2 rosters; ask roster, stage, which VFX),
Hitmonlee undrawn, Link's 3 entry frames on SZ/Saffron, Results photo (tic 0-80 black), lean remats (DK 13/match).
**10-01 1P campaign:** the walk ROM plays 0-13, bonuses, Master Hand, Ending, Staffroll, Congra with 0 battle
failures (table `p2/P2-6-one-player.md`). Landed: intro stills/poses, staff roll, Master Hand, Race/board lights, HUD
anchors, bonus map colours (RSP lighting baked, `390d513c5cb`), Ending room + figure (NitroFS room table), platform
bake. Owner playtest 10-01 (Kirby): 15 of 24 bonus boards drew no map (3-colour TLUT, `98be04a7540`); Polygon Team
froze on a copy hat the match never admitted (`1c1c67b90f4`); polygon packets now replay (`04cc889b6e4`). Owner floor
for content work: P50 < 1.12M (>50% of frames within 2 VBlanks). 10-02: every stage 80-98%, the Race 50.2% (GX
texgen, fast-lane routes, off-screen culls, rigid stage bindings: `p2/P2-6-one-player.md`). Other blob stages
still ship rigid mask 0. Probes: session scratchpad `pace2.ps1`/`pacesweep.ps1`/`profwalk.ps1`.

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
