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
**Phase 2 ongoing:** Task36 retired; all-VS stage compilation, NDL, native MISC
owners and view-space particles/dust landed (receipt
`artifacts/performance/2026-09-26_p2-2p8-phase2-particle-ab/README.md`). CSS
reserve free 253,904 B (09-27) versus required 183,072 B. Campaign coverage and global
renderer retirement remain debt. Bank measured battle wins (D9); no restarts.
Phase 3 A8 BGM committed `d0d02c61a83`; heavy F7443068 admission 5,904 B short;
MF2 unlinked. Receipt `2026-09-26_p2-2p8-phase3-residency/README.md`.
**09-26/27 solo:** leaf, A9-instrument, ITCM-placement (ITCM now full; lab
split builds keep ndsFtPoseUpdate in main RAM) and DS-divide cuts; receipts
`2026-09-2{6,7}_p2-2p8-*`. `FAD6C57E` WORK-H P50/P95 986,112/1,371,008 (sim + present subtrees on DTCM stack, ITCM packs);
lab flags NDS_TICK_HUD_SRC_SPLIT, NDS_FTR_LEAN_PHASE_TICKS, NDS_CAMERA_FRAME_WITNESS, NDS_WHISPY_AOT_TICKS.
Digest `--sequence --resync 4`, control `dtcm3`. P95 set: FTR +283K (DK's 3+
lean states thrash two entries; heap spare buffer banked, 62 -> 44, verify arm
0 mismatches), SINT +122K (motion start), SHDT +106K, SPRM +72K, MISC +60K.

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
