# Handoff

Updated: 2026-07-17 11:28 Central
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

The integrated fixed-two candidate is 14,613,504 bytes, SHA-256
`36218F25F3C69929CEBF4F62B8E2BEBFFCCC13739DBBE5B5D28376352677A686`.
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

Task 9's unchanged-libgcc ITCM KEEP moves the source owner
311,744/312,960 -> 260,192/261,312 and matches all 3,892 state rows. Current
ITCM after the emitter split and source-light repair is 28,040/32,768;
gameplay's 16 KiB main-RAM stack has 8,284 bytes measured headroom. Phase 2
stays skipped.
Do not merge this branch into main without Tyler's instruction.

The M2 raw/cross emitter split remains retained. The next measured cut reuses
the fully overwritten capture arena instead of clearing 6,240 bytes per fighter:
detailed capture falls 47,296/47,360 -> 41,152/41,152, combined fighter falls
452,640/455,808 -> 446,464/449,600, and draw falls
1,077,568/1,080,832 -> 1,071,488/1,074,816 with exact pixels and non-timing
state. Current post-light-repair ledger-off is 386,016/389,184; the older
372.1K sample is retired. Generic/fast profile 2 remains exact on frames
180..187 with 686 triangles. Boundary passes on `36218F25...`; current
profile-0 smoke is 22.3 FPS, so full-speed remains red.

Natural KO/rebirth uses real source events and measures 1,261,344/1,524,864 and
1,110,528/1,112,256 active ticks with exact stage/M4/fence contracts.

## Checkpoint
Countdown, effects, FGM, Task 9 identity, one-minute lifecycle, natural
KO/rebirth timing, the M2 split, source-exact light parity, the capture-reset
cut, and Boundary pass.
P1 is incomplete: full-speed, edge retest, audio debt, and Tyler's exact-ROM
eyeball remain. Next: resume one measured M2/full-speed cut from this verified
checkpoint; do not reopen rejected wallpaper/stage designs. The Lean snapshot
closes this packet.
