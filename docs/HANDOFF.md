# Handoff

Updated: 2026-07-17 07:02 Central
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
`A89F143B447F7511D80AAB12D423BF8B70EDA4844E4396750F6CEAABF6F295DC`.
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
.\scripts\verify-battle-playable-one-minute-match.ps1
```

The isolated published-equivalent proof passes natural fixed-two qualification:
4,084 committed updates / 2,042 presents, phase rates
39.9/37.4/39.3/n.a./58.2 updates/s, and phase slips 196/1088/946/0/3. It
exercised imported level-3 Fox AI, expired at 3,600 source ticks, reached
Results, retained 166,672 bytes after BGM, and reported exactly one normal M4
teardown with every post-GO fence counter zero. Its ROM is `9C35F4B3...`; the
isolated build leaves the public ROM byte-identical.
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

Task 9 is complete. Six unchanged GCC 15.2.0 Thumb-multilib objects (1,952 code
bytes) now link from ITCM ahead of libgcc. The frames-600..607 two-update source
owner moved 311,744/312,960 -> 260,192/261,312 ticks P50/P95, a
51,552/51,648-tick (16.54%/16.50%) KEEP. The supreme gate matched all 3,892
per-update state rows exactly; Current and the natural one-minute gate pass.
Canonical ITCM is 28,128/32,768 with the renderer's 23,640 bytes unchanged.
Phase 2 was skipped because unchanged libgcc already produced a decisive gain.
Report-only: gameplay uses a 16 KiB main-RAM coroutine stack, not DTCM; the
measured update-600 high-water used 8,100 bytes with 8,284 bytes headroom.
Do not merge this branch into main without Tyler's instruction.

Natural KO/rebirth timing now uses profile-1 ROM `32C957AD...`, the real KO FGM
trace, and BattleShip's `ftCommonRebirthDownSetStatus`, never scripted combat.
KO is 1,261,344/1,524,864 active ticks; rebirth is 1,110,528/1,112,256. Both
keep exact stage/M4/fence contracts; evidence is `20260717-*-natural-profile1`.

## Checkpoint
Countdown, effects, FGM, Task 9 identity, one-minute lifecycle, and natural
KO/rebirth timing pass. P1 is incomplete: full-speed, edge retest, audio debt,
and Tyler's exact-ROM eyeball remain. Next: measured M2 fighter emit; do not
reopen rejected wallpaper/stage designs. The Lean snapshot closes this packet.
