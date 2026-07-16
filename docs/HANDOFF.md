# Handoff

Updated: 2026-07-16 12:17 Central
`P1_EXECUTION_BOARD.md` owns all current state. This file is only the restart
surface.

## Restart
Branch: `codex/wip-natural-combat-source-start-collision`

Boundary: `battle_playable_realtime`, mode `163`.

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Preserve the published intrinsic mode-9 / mip-0 / static-residency / bitmap-OAM
configuration. Dream Land water is exact frame 0/fraction 114 on the original
12 triangles; the animated replacement and its dead implementation are removed.

The Boundary-verified fixed-two candidate is 14,586,880 bytes, SHA-256
`8B949194C5EF02CCA2A59479F67F99E4A6D73A41E7972DBD95CD3CF78BCF1DAA`.
Stage painter depth and pause-orbit containment are fixed and user-confirmed.
M3 retains no-Z codegen, dense prepare-once, AOT coordinate shifts, the
zero-shift matrix builder, and exact bounded `s16` rounding at
536,032/536,256 ticks.

The M4 Whispy lifecycle repair is kept: later cosmetic mouth/eye images reuse
an otherwise exact resident source-frame key, preventing the late fallback and
40 gameplay conversions. See the board and `PERF_LEDGER.md` for the exact-ROM
windows; do not repeat them before final release qualification.

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
.\scripts\verify-battle-playable-one-minute-match.ps1 -RunnerSlot 2
```

Canonical ROM SHA-256
`8B949194C5EF02CCA2A59479F67F99E4A6D73A41E7972DBD95CD3CF78BCF1DAA`
passes the natural fixed-two qualification: 4,084 committed updates / 2,042
presents, interval 2..5 vblanks, zero early presents, phase rates
39.9/37.9/39.5/n.a./58.2 updates/s, and phase slips 196/1050/931/0/3. It
exercised imported level-3 Fox AI, expired at 3,600 source ticks, reached
Results, retained 140,816 bytes after BGM, and reported exactly one normal M4
teardown with every post-GO fence counter zero. The DS taskman seam matches
BattleShip by breaking on `LoadScene` before drawing; the verifier samples the
battle ledger before Results reuses the globals.

## Next Packet

Tyler withdrew the Jump A/C all-or-nothing keep gates. Correct measured gains
now accumulate even when they do not finish a milestone. Jump A dense reuse is
kept. Jump C native-fighter ITCM placement is also kept: combined fighter
P50/P95 is 419,328/419,392 → 402,560/402,624, draw P50/P95 is
1,245,024/1,247,616 → 1,230,336/1,232,832, and the top-screen A/B is exact.
ITCM is 25,384/32,768. Evidence is under
`artifacts/performance/2026-07-16_m2-itcm-restore-{a,b}.json` and
`artifacts/visibility/2026-07-16_m2-itcm-restore-{a,b}.png`.

Jump A now retains no-Z codegen, dense prepare-once, AOT-packed coordinate
shifts, the exact zero-shift raw-matrix builder, and bounded `s16` vertex
rounding. The latest stage result is 536,032/536,256 ticks with a 0/49,152
native-pixel delta. The constant-depth GX painter already made the old CPU
divider cut obsolete; do not reopen it or the rejected matrix-position reuse.
Jump C Phase 0 is refreshed: local matrix construction is 53,024/53,120 ticks
and lighting is 67,808/68,032. The source integer sine-table path was already
live, but its power-of-two float-to-fixed boundaries still called ARM soft-float
helpers. The retained exact conversion cut moves combined fighter time from
402,560/402,624 to 398,048/398,144. Lighting is already the exact prepared-dot
plus LUT path. The old 480-triangle KRAW extension is superseded: Mode 8 now
owns all 582 raw fighter triangles. Hoisting its raw/textured decisions outside
the corner loop moves combined fighter time again to 397,248/397,312. Batching
the unchanged raw/cross accounting once per owner traversal moves it to
395,264/395,328. AOT-packing the immutable DS `VERTEX16` words in the same
16-byte dense record moves it to 386,880/386,944. Co-locating those words with
prepared color/UV in one 16-byte output record then moves it to
384,000/384,000 with a byte-identical top screen and unchanged total fighter-
table RAM. Continue against production emit work, not lighting.
The current 1.415–1.618M CPU-on P95 predates fixed-two sampling. Locked 30 is
the explicit presentation decision; 60 FPS is not claimed. Continue Jump A
CUT 3-NEW where the new phase update-rate metrics expose slowdown.

## Checkpoint
Boundary already passes for the current ROM; do not rerun it unchanged. Use
Current instead only if normal/shared startup changes. Snapshot is the final
project command.
