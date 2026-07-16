# Handoff

Updated: 2026-07-15 22:15 Central
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
Boundary passes at 19.7 FPS on published ROM
`19C6CD229C205CD60F7625BED86625AADA5556AEE377CD4A4698154249F98D7E`.
The normal-play stage painter bug is fixed. BattleShip's 66 source-Z / 126
no-Z classification was already correct; the DS counter advanced only one
sixth of a submitted v16 step, so groups of six painter triangles collided.
The final zero-collision profile-2 census is 202 triangles. Published-ROM pixel
evidence is `artifacts/visibility/20260715-stage-depth-final-cutg-frame438.png`
and `artifacts/visibility/20260715-stage-depth-final-moving-frame501.png`.
Pause-orbit corruption is reopened: the earlier scaled-raw repair does not
contain every CPU-projected near-plane crossing.

The retained M3 path improved 664,544/664,640 → 645,248/645,440 stage ticks and
1,183,104/1,183,168 → 1,102,656/1,102,720 draw ticks at frames 438–445. Exact
121/828 ownership, frozen water 2/0/1, 22/131072 static residency, and zero
post-GO texture work remain intact. M3 is still REWORK, 145,248 ticks above its
first 500K gate.

## Rejected M3 Cut

The dense-index prepare-once experiment was exact but too small. Against the
664,544/664,640 baseline it measured 555,584/555,776, saving only
108,960/108,864 versus the required 164,544. The source/checker patch was fully
reverted. Evidence remains at:

```text
artifacts/visibility/m3-dense-prepare-8frames.json
artifacts/visibility/m3-dense-prepare-frame438.png
```

Do not retry or widen that dense-only cut.

## One-Minute Gate

Tyler authorized CPU-on automation while the published/manual default keeps the
shared fast-iteration flag at `0`: Fox CPU and countdown are skipped and the
timer is frozen. DevFast/Boundary and lifecycle gates explicitly select flag
`1`, restoring the original Wait → countdown → GO/timer path. The focused
source-timer gate now passes:

```powershell
.\scripts\check-one-minute-match-verifier.ps1
.\scripts\verify-battle-playable-one-minute-match.ps1 -RunnerSlot 2
```

The run completed 3,891 logic updates, exercised imported level-3 Fox AI,
reached Time Up and Results, retained 163,312 bytes after BGM, and reported one
normal M4 teardown with every post-GO fence counter zero. The DS taskman seam
now matches BattleShip by breaking on `LoadScene` before drawing; the verifier
samples the battle ledger before Results reuses the globals.

## Rejected M2 Cut

The Mode-8 ITCM-only experiment was exact but too small. Frames 600–607 measured
416,576/416,704 → 398,496/398,592 combined fighter ticks, saving only 18,080
versus the required 80K. Stage pixels were byte-identical; only 13 bottom-HUD
pixels changed with live FPS text. The three placements/checker requirements
were reverted. Evidence remains at:

```text
artifacts/performance/2026-07-15_m2-itcm-a.json
artifacts/performance/2026-07-15_m2-itcm-b.json
artifacts/visibility/2026-07-15_m2-itcm-a-frame607.png
artifacts/visibility/2026-07-15_m2-itcm-b-frame607.png
```

The source-faithful pre-GX rejection for active animlocks/shuffle remains; it
falls back to the ordinary BattleShip path before native preparation or GX.

## Next Packet

Stay in Mode 8. Before coding, bound the smallest no-copy direct-contract cut
together with the measured 18K placement gain. Proceed only if attributable
savings can reach 80K; then run the same eight-frame gate and require combined
ticks <=336,576, exact 32/49/67/626 semantics, matching screenshot/reserve, and
zero fallback. Do not add a mode, packet copy, DMA path, or per-root interpreter.

## Checkpoint

Run one widest relevant verifier only: Boundary for battle-only work, or Current
instead if normal/shared startup changed. Finish docs and `git status`
inspection, commit, then run the Lean snapshot as the final command.
