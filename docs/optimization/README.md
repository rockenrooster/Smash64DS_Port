# docs/optimization — layout and convention

**Convention (Tyler, 2026-07-20): one task per .md file.** Naming:
`ClaudeFable5_Task<NN>_<slug>_<YYYYMMDD>.md`. Shared process/doctrine lives once in
`TASK_STANDING_RULES.md`; every task file opens by requiring it. Never append a
second task to an existing file.

## Current contents

- `TASK_STANDING_RULES.md` — standing process rules, fidelity doctrine, device-test
  economy. Read first for any task.
- `ClaudeFable5_Task36_HwCompose_20260720.md` — active task (canonical per-file form).
- `ClaudeFable5_Task37_ItcmRepack_20260720.md` — queued task.
- `ClaudeFable5_Task38_FgmAudit_20260720.md` — queued task: FGM coverage audit +
  missing Up-B/Down-B sounds + layered-hit fixes.
- `ClaudeFable5_Task39_VisualEffects_20260720.md` — queued task (visual-effects audit +
  faithful DS implementations; Phase C sequenced after Task 36). 2026-07-21: preflight
  stopped it pre-Phase-A on a shield-map contradiction; the map is corrected in the file
  (shield is a lane-3 substitute template) and it is ready to resume.
- `ClaudeFable5_Task40_FighterAnimAudit_20260720.md` — queued task: fighter animation
  coverage/correctness audit + faithful fixes (Fox first; run after Task 36).
- `ClaudeFable5_Task41_ShipLeanTickHud_20260721.md`, `..Task42_AdpcmBgm_20260721.md`,
  `..Task43_MicroSweep_20260721.md`, `..Task44_StageSteadyState_20260721.md` — queued
  tasks (see each file's header for sequencing).
- `ClaudeFable5_Task45_FgmFullCoverage_20260721.md` — PRIORITY task: ship every
  battle-reachable FGM cue exactly (reverses Task 38's exclusions; enforces the
  content-completeness doctrine in TASK_STANDING_RULES.md).
- `NDS_NATIVE_STAGE_CONSUMED_FIELDS.generated.json`,
  `NDS_NATIVE_FIGHTER_CONSUMED_FIELDS.generated.json` — generated certificates
  (referenced by HANDOFF and task specs). Not task files; stay here.
- `NATIVE_RENDERER_PLAN.md` — reference plan document, not a task queue; stays.

## Archive

`docs/optimization/archive/` holds the resolved pre-convention multi-task queue
files (JumpA/JumpABC/Review-And-Plan/Publish/tasks.md and the 20260720 perf queue;
its `_stage` duplicate was deleted — the Task 36 per-file spec is the survivor).
Archived 2026-07-21 after the tasks38-40 codex session ended. Archived files keep
their original names; they are history, not templates.
