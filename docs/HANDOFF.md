# Handoff

Updated: 2026-07-17 Central
`P1_EXECUTION_BOARD.md` owns all current state. This file is only the restart
surface.

## Restart
Branch: `codex/wip-natural-combat-source-start-collision`

Boundary: `battle_playable_realtime`, mode `163`.

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Preserve the published intrinsic mode-9 / mip-0 / static-residency / source-countdown
configuration. Dream Land water is exact frame 0/fraction 114 on the original
12 triangles; the animated replacement and its dead implementation are removed.

The integrated fixed-two candidate is 14,612,480 bytes, SHA-256
`162212F4093BFFF9B8C9AA47678019DD627766A5BD66CFBB1BB44FE9B4284DC5`.
Stage painter depth and pause-orbit containment are fixed and user-confirmed.
M3 retains no-Z codegen, dense prepare-once, AOT coordinate shifts, the
zero-shift matrix builder, exact bounded `s16` rounding, Task 6 first-use
attribute preparation, and the valid-color stage seam. Combat stage is now
489,184/489,536 ticks P50/P95.

The M4 Whispy lifecycle repair is kept. Countdown assets now reverse the source
odd-row texture interleave: big GO is direct RGB555+A1 OAM, the opaque shaded
traffic box is A3I5, and only the foreground flare is A5I3. Compact source
atlases use 57,344 texture bytes total and restore pre-GO source-frame residency;
do not add transition-time texture deletion.

## One-Minute Gate

Mode 163 uses locked-30 fixed-two pacing: exactly two source updates per present,
with no vblank debt or catch-up. Slow frames uniformly slow gameplay; zero-slip
phases must also hold 59.0..61.0 updates/s. Old baselines must be resampled.

The published/manual ROM and source runtime gates retain flag `1`, preserving
Fox CPU and the original Wait → countdown → GO/timer path. The separate
`-FastIteration` screenshot launch selects `0` before battle, skipping the
countdown and Fox decisions and freezing the timer at `1:00`. The focused
source-timer gate passes:

```powershell
.\scripts\check-one-minute-match-verifier.ps1
.\scripts\verify-battle-playable-one-minute-match.ps1 -NoBuild
```

The current ROM passes the natural fixed-two qualification: 4,084 committed
updates / 2,042 presents, phase rates 39.9/38.0/39.6/n.a./58.2 updates/s, and
phase slips 196/1039/925/0/3. It exercised imported level-3 Fox AI, expired at
3,600 source ticks, reached Results, retained 138,000 bytes after BGM, and
reported exactly one normal M4 teardown with every post-GO fence counter zero.
The DS taskman seam matches
BattleShip by breaking on `LoadScene` before drawing; the verifier samples the
battle ledger before Results reuses the globals.

## Integrated July 17 Candidate

- Source countdown verifier passes with GO `3 OBJ + 10 quads`, 31,168 OBJ,
  57,344 texture, 608 palette bytes, and zero gameplay conversion/upload.
- Natural visual-effects proof passes: 3 created, 13 rendered, 96 triangles,
  kind mask `0x45`, zero rejects, 176,464-byte reserve.
- The 107,536-byte FGM pack covers 18 exact IDs / 16 unique samples and 11
  collision cues. Natural phase qualification passes 14 plays, 21 envelope
  steps, max 3 live handles, and 174,864-byte headroom. IDs 429/435 continuous
  pitch and fork voice 685 remain explicit fidelity debt.
- Current canonical startup reaches frame 212 with M4 `22/131072`, 646 hits,
  zero hot conversion/upload, and zero post-GO fence work. The full current
  one-minute lifecycle passes through Results and teardown.

Task 8 Cuts E (`bc35a84c57`), F (`11e69a9de4`), and lean G2 (`25ef1397e2`)
are banked. Cut H was not measured; the update-pair audit found no duplicate
display derivation. The one-minute reserve repair shares the non-overlapping
fighter display-list scratch and preserves exact output. Next is Task 9 Phase 0
from `docs/optimization/ClaudeFable5_JumpABC_Tasks_20260715_2326.md`. Do not
merge this branch into main without Tyler's instruction.

## Checkpoint
Countdown source/host/runtime, focused visual-effects, full FGM phase-pack,
Task 8 Boundary, and the current natural one-minute lifecycle pass. Take a Lean
snapshot as the final project command. Release qualification still needs
Tyler's exact-ROM eyeball.
