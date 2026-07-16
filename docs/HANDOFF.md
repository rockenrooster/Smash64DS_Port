# Handoff

Updated: 2026-07-16 08:35 Central
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

The retained M3 path now includes no-Z codegen plus dense prepare-once. On the
current bitmap-OAM configuration, same-ROM frames 438–445 move stage
619,744/619,904 → 577,440/577,536 and draw 1,057,856/1,057,920 →
1,013,760/1,013,824. Exact 121/828 ownership, 57/42/54/202/49/4 stage state,
zero fallback/fence work, and 0/120,000 changed top-screen pixels hold. The
500K point is the next target, not a discard gate.
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
kept; re-run Jump C next without its old 80K pre-code rejection gate. Do not
reopen measured regressions or add speculative renderer caches. The current
1.415–1.618M CPU-on P95 still leaves 60 FPS explicitly unmet; the stable 20 FPS
decision remains pending while performance work continues.

The Fireball terminal gate is closed. The earlier far-left theory was false:
the same source-created weapon naturally crosses Pupupu's bottom bound at
lifetime 10, reaches the exact ELF destroy callsite, disappears by the next
completed frame, and changes the deterministic orange ROI from 50 to 0 pixels.

The one-way platform gate is now green on the current ROM. Its 715-frame route
requires six ordered continued-ascent/strict-descent/downward-crossing flights,
all three platform masks, exact ignore-line Pass crossings, two side cycles,
and 214,544-byte reserve. Evidence is
`artifacts/visibility/2026-07-16_052652-7356809_platform-semantics-p984.png`.
Throw-origin recovery is green. The verifier now accepts the source Walk path
and uses nine short source Walk/Dash/Run entries with Wait reevaluation instead
of holding through Run. One natural Mario catch/forward throw took Fox 0→12%,
released statuses 169→186, swept/clamped and DownBounced once on line 3, cleared
every catch link, and retained 202,256 bytes. Evidence is
`artifacts/visibility/2026-07-16_063512-7696185_throw-release-recovery-p21764.png`.
The duplicated capture helper was deleted in favor of the standard repo-local
melonDS window capture; do not restore it.
Fox recovery and continuous damage are green: after 40 source Recover frames
and an offstage-to-line-3 return, Mario damages the same Fox again from 59% to
72%. All 11 damage colliders and global/special/star hit statuses remain normal.
Evidence is `artifacts/visibility/20260716_fox-recovery-post-hit.png`. The same
route plays source FoxSmash1 ID372 and MarioSmash2 ID430 through clean DS
handles with 202,256-byte reserve; Tyler's fighter-voice ear-check is pending.
Dream Land BGM is also green without a ROM change: its exact initial DS ring has
peak 9,928 / RMS 2,283.623, and the natural route observes the live channel bit
in Calico's ARM7-shared mask with clean 44.1 KB/s streaming.

## Checkpoint

Run one widest relevant verifier only: Boundary for battle-only work, or Current
instead if normal/shared startup changed. Finish docs and `git status`
inspection, commit, then run the Lean snapshot as the final command.
