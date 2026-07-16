# Handoff

Updated: 2026-07-16 05:35 Central
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
The current user-facing candidate is 14,565,376 bytes, SHA-256
`593FBBA217D2AD7F9F87DE2013F38C82517A1DDDF1FE36CDF6110894C379C91E`;
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

The retained M3 path is now 611,392/611,584 stage ticks at frames 438–445 after
removing erroneous cold/Os codegen from the 126-triangle no-Z emitter. Exact
121/828 ownership, frozen water 2/0/1, 22/131072 static residency, zero post-GO
texture work, and identical DS pixels remain intact. M3 is still REWORK,
111,392 ticks above its first 500K gate.

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

Qualify one natural Mario/Fox voice path with the existing audio pack/runtime.
Use the audio-qualification skill, first identify the exact source event/ID from
mode 163, and distinguish the reported 24 unsupported calls from unique IDs.
Do not add a trigger, mixer, mode, or hand-authored waveform.

The one-way platform gate is now green on the current ROM. Its 715-frame route
requires six ordered continued-ascent/strict-descent/downward-crossing flights,
all three platform masks, exact ignore-line Pass crossings, two side cycles,
and 214,544-byte reserve. Evidence is
`artifacts/visibility/2026-07-16_052652-7356809_platform-semantics-p984.png`.
Throw-origin recovery remains open because the existing input driver did not
reach its first natural Dash/Run transition; do not rerun it unchanged.
Fox recovery is green: the current-ROM controller-only route produced 40 source
Recover frames and an offstage-to-line-3 return without KO/rebirth. Evidence is
`artifacts/visibility/2026-07-16_053441-3060146_fox-recovery.png`.

## Checkpoint

Run one widest relevant verifier only: Boundary for battle-only work, or Current
instead if normal/shared startup changed. Finish docs and `git status`
inspection, commit, then run the Lean snapshot as the final command.
