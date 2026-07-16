# Handoff

Updated: 2026-07-15 20:04 Central

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

The 20:01 rebuilt-ROM frames 438/439 capture passed visibility and detail gates
under `artifacts/visibility`. Boundary itself is open: its smoke attached at
battle frame 46 before M4 arm (`arm=0`, prepared=22/131072). Do not weaken that
gate or call it a pass; align its sample with the natural post-GO window.

## Next Packet

M3 is device-semantic-correct but measures 664,544/664,640 stage ticks. Make one
bounded change only:

- `src/nds/nds_renderer.c`: prepare repeated stage corners once by dense index.
- `scripts/check_nds_native_stage.py`: enforce zero preparation-tuple conflicts.
- 606 references map to 312 vertices; projected work should fall 408 to 246.
- Expected saving: 170-210K ticks.
- KEEP only at <=500K P50 with at least 164,544 saved, improved P95, exact
  8/57/42/54/202 ownership, zero fallback/fence, and matching screenshot.

Use eight synchronized frames for A and B. Record ticks, FPS, screenshots under
`artifacts/visibility`, and automated screenshot analysis. Run A2 only if A/B is
near the gate, noisy, surprising, or inconsistent.

```powershell
python .\scripts\check_nds_native_stage.py
.\scripts\benchmark-renderer-fast-raw.ps1 -FastRunMode 9 `
  -StaticTextureAotMode 1 -IFCommonHybridOamMode 1 `
  -RendererProfileLevel 1 -RendererM2DetailedLedger `
  -RendererBenchmarkSamples 8 -RendererBenchmarkStartFrame 438 -RunnerSlot 3
```

After M3 settles, root-cause the isolated one-minute M4 gate's missing terminal
acceptance marker. Do not count the first nonzero invocation as M4 evidence.

## Checkpoint

Run one widest relevant verifier only: Boundary for battle-only work, or Current
instead if normal/shared startup changed. Finish docs and `git status`
inspection, then commit before the Lean snapshot; the snapshot is the final
command.
