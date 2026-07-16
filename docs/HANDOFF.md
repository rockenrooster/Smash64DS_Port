# Handoff

Updated: 2026-07-16 09:20 Central
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
The current user-facing candidate is 14,574,592 bytes, SHA-256
`A8371FA93E75338F8BABAC445FA4826663979FE48884E215F27630049B3B6C93`;
its next Boundary republication is intentionally deferred to the release
checkpoint.
The normal-play stage painter bug is fixed. BattleShip's 66 source-Z / 126
no-Z classification was already correct; the DS counter advanced only one
sixth of a submitted v16 step, so groups of six painter triangles collided.
The final zero-collision profile-2 census is 202 triangles. Published-ROM pixel
evidence is `artifacts/visibility/20260715-stage-depth-final-cutg-frame438.png`
and `artifacts/visibility/20260715-stage-depth-final-moving-frame501.png`.
Clip-space near-plane containment now removes the pause-orbit corruption and
Tyler confirmed the fix. Paused −33.6° is the preferred Mario underside view.

The retained M3 path includes no-Z codegen, dense prepare-once, and AOT-packed
GX coordinate shifts. Current bitmap-OAM frames 438–445 move stage
578,272/578,560 → 556,256/556,352 and draw 997,440/997,504 →
975,360/975,488 for the latest cut. Exact 121/828 ownership,
57/42/54/202/49/4 stage state, cross 5/10/15, zero fallback/fence work, and
0/240,000 changed DS-screen pixels hold. The 500K point is the next target,
not a discard gate.
The signed-16 rounding codegen lead was also exhausted: it shrank both hot ARM
symbols but saved only 2,048 stage ticks, and its B screenshot was invalid. The
treatment is fully reverted; do not rerun it without a new closing bound.

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
the corner loop moves combined fighter time again to 397,248/397,312. Continue
against production emit/account work, not lighting.
The current 1.415–1.618M CPU-on P95 still leaves 60 FPS explicitly unmet; the
stable 20 FPS decision remains pending while performance work continues.

## Checkpoint

Run one widest relevant verifier only: Boundary for battle-only work, or Current
instead if normal/shared startup changed. Finish docs and `git status`
inspection, commit, then run the Lean snapshot as the final command.
