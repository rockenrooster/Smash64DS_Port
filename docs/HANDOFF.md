# Handoff

Updated: 2026-07-16 09:50 Central
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

The executable fleet is four registry records under `Latest`/`Boundary`; the
unreachable source-side mode 1-162 lattice is separate ROM-parity cleanup.
The Boundary-verified user-facing candidate is 14,578,688 bytes, SHA-256
`B4E6EC10A50413A3E2AF5829A39F9971CD234A8CC90F4A5416ED1242165A3915`.
Stage painter depth and pause-orbit containment are fixed and user-confirmed.
M3 retains no-Z codegen, dense prepare-once, AOT coordinate shifts, and the
zero-shift matrix builder at 541,952/542,272 ticks.

The M4 Whispy lifecycle repair is kept: later cosmetic mouth/eye images reuse
an otherwise exact resident source-frame key, preventing the late fallback and
40 gameplay conversions. See the board and `PERF_LEDGER.md` for the exact-ROM
windows; do not repeat them before final release qualification.

## One-Minute Gate

The published/manual ROM and source runtime gates retain flag `1`, preserving
Fox CPU and the original Wait → countdown → GO/timer path. The separate
`-FastIteration` screenshot launch selects `0` before battle, skipping the
countdown and Fox decisions and freezing the timer at `1:00`. The focused
source-timer gate passes:

```powershell
.\scripts\check-one-minute-match-verifier.ps1
.\scripts\verify-battle-playable-one-minute-match.ps1 -RunnerSlot 2
```

The run completed 3,891 logic updates, exercised imported level-3 Fox AI,
reached Time Up and Results, retained 163,312 bytes after BGM, and reported one
normal M4 teardown with every post-GO fence counter zero. The DS taskman seam
now matches BattleShip by breaking on `LoadScene` before drawing; the verifier
samples the battle ledger before Results reuses the globals.

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
shifts, and the exact zero-shift raw-matrix builder. The latest stage result is
541,952/542,272 ticks. The constant-depth GX painter already made the old CPU
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
16-byte dense record moves it to 386,880/386,944. Continue against production
emit work, not lighting.
The current 1.415–1.618M CPU-on P95 still leaves 60 FPS explicitly unmet; the
stable 20 FPS decision remains pending while performance work continues.

## Checkpoint

Boundary already passes for the current ROM; do not rerun it unchanged. Use
Current instead only if normal/shared startup changes. Snapshot is the final
project command.
