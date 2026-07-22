# Handoff

Updated: 2026-07-21 (Tasks 38-40 audit closeout)

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

Task 39's corrected-map Phase C implements hurt flash, OAM hit sparks, and the
flat transparent 2D shield with white shine. Tyler approved all three in the
same ROM. The published/release-equivalent targets enable them; DevFast and
Boundary pass. Task 39 is complete and no visual row needs replay.

Task 40 is complete: Mario 195/195 and Fox 209/209 non-null motions are
Tyler-approved. The full source-
backed bank, AObj16/AObj32 loader fix, `DEATH` letter `T`, watchdog, resume, and
Fox-AI-off cycler are retained. Do not rerun known-good rows. The full corrected
numeric duration matrix remains provisional; only resume specifically requested
or unverified rows if Tyler asks.

Evidence is under `artifacts/performance/2026-07-21_task38/39/40-*`; Task 40's
423-row CSV and two contact sheets are permanent. Profile-0 ROM is 14,958,592
bytes / `AEE10EB3...`, net reserve 166,672, audit-symbol hits zero. DevFast's
components and the final Boundary are green. The retired Regression fleet stays
retired.

Preserve the unrelated `.gitignore` edit and all untracked optimization task,
stage, review, and README files; they are user-owned.
