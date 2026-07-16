# Handoff

Updated: 2026-07-15 21:13 Central

`P1_EXECUTION_BOARD.md` owns all current state. This file is only the restart
surface.

## Restart

Branch: `codex/wip-natural-combat-source-start-collision`

Boundary: `battle_playable_realtime`, mode `163`.

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Preserve the published intrinsic mode-9 / mip-0 / static-residency / hybrid-OAM
configuration. Dream Land water is exact frame 0/fraction 114 on the original
12 triangles; the animated replacement and its dead implementation are removed.

The executable fleet is now four registry records under `Latest`/`Boundary`;
168 legacy verifier/manager scripts and their public mode mappings are deleted.
The unreachable source-side mode 1-162 lattice remains a separate ROM-parity
cleanup, not part of the next renderer change.

Boundary passes at 19.7 FPS on published ROM
`CE922B60EFFE16D3A05A18ED3B0FD54F0A73A70C8CE9076AF85A5A59D5B96478`.
It retains exact M3 121/828 ownership, frozen water 2/0/1, 22/131072 static
residency, 76 pinned hits, zero post-GO fence work, and Cut G frames 438/439 at
`artifacts/visibility/2026-07-15_canonical_fast_frame438-439_211246-5878396-p43792.png`.

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

Tyler authorized CPU-on automation while the published/manual default remains
paused. The focused source-timer gate now passes:

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
inspection, then commit before the Lean snapshot; the snapshot is the final
command.
