# Handoff

P2 follows `PROJECT_GOAL.md` and `P2_PLAN.md`; `P2_EXECUTION_BOARD.md` owns focus, decisions, artifacts and the
**Execution cursor**. This file is a route, not another task or metric ledger.

## Current route (2026-09-26)

P2-2p8 four-fighter 30 FPS runs on `p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md`
(rulings D1-D9 in its section 8, phase log in section 6). On resume read its
sections 0, 6 and 8, then the board cursor. Evidence:
`artifacts/performance/2026-09-2*_p2-2p8-*`; `scripts/compare-replay-digest.py`
is the gameplay-equivalence check every phase runs.

Phase 1 (fighters): slices 1-7 landed through `98ebd1e2e51`, with slice 7 an
**IMPLEMENTED_NOT_ACCEPTED** checkpoint. Lean defaults and Ness's yo-yo/bat
are covered on the final ROMs; the Results/CSS tint-lifetime repair has natural
transition probes and inspected A/B captures. Receipt:
`artifacts/performance/2026-09-24_p2-2p8-phase1-slice7/README.md`. Runtime jobs
finished; the checkpoint is pushed to origin/master. Root is still r54.
**Owner 09-24: >=95% effort on four concurrent VS fighters; 1P campaign later.**
Pre-stage 1P intros are static; the live-Intro experiment is archived/reverted.
**Phase 2 ongoing:** Task36 retired; all-VS stage compilation and NDL owner
proofs are retained. View-space particles on `608C79AC` pass host/replay and
paired captures; mean/P50 improve, P95 worsens. Not accepted. Dust owner now
preserves native RGB5/alpha5: `8B4D66EE` draws all seven frames, native failures
39->0, replay identical, full stress correctness GREEN with NDL armed. Performance
still RED. Particle experiment routes are retired; `F5CA9FE7` passes naturally
with NDL on (18.95 FPS, WORK-H P95 2.583M). Shipping `8062C536` CSS previews
are exact; reserve free 204,752 B versus required 183,072 B. Current receipt:
`artifacts/performance/2026-09-26_p2-2p8-phase2-particle-ab/README.md`.
Campaign-specific coverage and global renderer retirement remain explicit debt.
No full-test restart: reuse slice 7's proofs and bank measured battle wins (D9).
Phase 3 A8 BGM `0BD4523E`: replay/audio engagement pass,17 tests; performance RED.
HeavyF7443068 admission still5,904 B short. Next: FGM voices/cache; MF2 unlinked.
receipt `artifacts/performance/2026-09-26_p2-2p8-phase3-residency/README.md`.

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
