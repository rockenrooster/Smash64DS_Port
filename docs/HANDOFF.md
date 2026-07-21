# Handoff

Updated: 2026-07-21 (Tasks 38-40 audit gates)

`P1_EXECUTION_BOARD.md` owns current state. This file contains only the restart
surface and next packet.

## Restart

Branch: `codex/tasks38-40-audits`
Boundary: `battle_playable_realtime`, mode `163`

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Preserve canonical mode 163, intrinsic renderer mode 9, mip 0, static texture
residency, source countdown, exact Dream Land water frame 0, and Task 16
compare/i2f/addsub `1/1/1`. Do not edit `decomp/`.

## Next Packet

Task 38's unsupported-FGM census is implemented and verifier-exported. Exact
special cues remain blocked by representation and the 128 KiB resident cap;
never substitute another sound. Tyler's listen pass remains the hit-sound oracle.

Task 39 stopped on its mandated structural-map contradiction: shield already
uses a DS substitute. Resume only from a corrected owner map and Tyler's marked
contact sheet.

Task 40 delivered its mandatory Phase-0 map. Fox has 102 unique unmapped motion
symbols, and invalid lookup can replay stale heap data. Review the report before
adding the Phase-1 cycler/counters or any gameplay-adjacent mapping fix.

Evidence is in the three `2026-07-21_task38/39/40-*` reports under
`artifacts/performance`.

Verification is not green. Audio fixtures and the new marker pass, but DevFast
and Boundary both stop at frame 212 in the existing Task-36 mode-2 water gate:
`RENDER_TEXEL1=2,2,0,...` versus an all-zero assertion. Boundary never reaches
the one-minute census. Reconcile that base-`60e1e66` contract without weakening
it, then rerun DevFast -> Boundary before promotion.

Preserve the unrelated `.gitignore` edit and all untracked optimization task,
stage, review, and README files; they are user-owned.
