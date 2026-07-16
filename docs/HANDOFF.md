# Handoff

Updated: 2026-07-15 20:56 Central

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

Boundary now waits for the natural M4 arm call before its next completed-frame
sample. It passes the exact published M3 121/828 owner, frozen water 2/0/1,
22/131072 static residency, full masks, positive pinned hits, zero post-GO
fence work, and Cut G frames 438/439 under `artifacts/visibility`.

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

## Next Packet

Take the bounded M2 Mode-8 ITCM experiment. Mode 8 already emits generated
immutable arrays directly to GX; do not add another packet or mode. First add
the missing pre-GX rejection for active animlocks/shuffle, then place only the
existing lighting, run-prep, and owner executor in the native-fighter ITCM
section. Reuse the existing checks:

```powershell
python .\scripts\check_nds_native_owner_packet.py
python .\scripts\check_nds_native_owner_hierarchy.py
.\scripts\compare-renderer-fast-raw.ps1 -FastRunMode 8 `
  -RendererBenchmarkSamples 8 -RunnerSlot 3 -NoBuild
```

KEEP only with at least 80K saved, combined fighter ticks at or below 337,472,
matrix plus lighting at or below 120K, transport at or below 145K, exact
32/49/67/626 semantics, matching screenshot/reserve, and zero fallback.

## Checkpoint

Run one widest relevant verifier only: Boundary for battle-only work, or Current
instead if normal/shared startup changed. Finish docs and `git status`
inspection, then commit before the Lean snapshot; the snapshot is the final
command.
