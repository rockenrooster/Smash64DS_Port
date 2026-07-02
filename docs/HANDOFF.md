# Handoff

Use this file for the active handoff only. Keep it under 150 lines. The harness
registry decides the current Boundary/Latest profile; this file summarizes what
to do next.

## Start Here

1. Check the registry view:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

2. Current pair is expected to be:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
```

3. Read `docs/STATUS.md` for current truth, `docs/DIAGNOSTIC_REFERENCE.md` for
full marker strings, and `docs/PORTING.md` for history.

## Current Boundary

Modes `161/162` are the active legacy regression anchor. They prove selected
Fox Jab2 live-hit damage lifecycle plus selected damage-status follow-through
on the Pupupu Mario/Fox battle root. The current short marker summary is:

```text
status=17->52/45, hitlag=6->0, callbacks=1/6/1,
search=0xf, repeat=1/1, gate=0x3f, catchSearch=0xffffffff/s3
```

Latest verified source-backed detail: TaruCannon kind `3` now routes through
the source-ordered setup shell, installs status `61`, and runs one original
physics tick copying fighter root position from the barrel root. Continuous
TaruCannon update/shoot runtime still waits for Jungle barrel helpers and map
throw-hit data.

Latest renderer detail: `src/nds/nds_renderer.c` has opt-in DS 3D hardware
submission behind `NDS_RENDERER_HW_TRIANGLES=1`, fed by source-shaped DObj,
camera, and material state from `src/port/reloc_backend_renderer_dl.c`. Stage
3b added billboard-kind `33-40` and recalc-kind `41-50` seed coverage,
consistent textured/untextured `v16` world scaling, stage DObj submission from
the existing Pupupu `gcDrawAll` traversal, and renderer support for emitted
BattleShip `gSPMvpRecalc` / `G_MW_MATRIX` display-list streams. It also seeds
fighter-parts matrix kind `0x4B`, BattleShip battle-camera matrix kind `0x4C`,
and composes selected DObj parent chains from root to child before the camera
modelview. The latest increment routes source-shaped `gcDrawMObjForDObj`
material branch tables into the opt-in hardware traversal by emitting segment
`0x0E` branch lists plus palette/TLUT, texture image, load-block, tile-size,
texture, color, and light-color packets from the taskman graphics heap.
Captures: `artifacts\renderer-chain-hw-battle.png` and
`artifacts\renderer-stage-gcdrawall-hw.png`. The stage-inclusive capture shows
the Pupupu platform plus hardware-submitted fighter geometry using the captured
BattleShip camera path. The all-DL verifier also has an opt-in hardware texture
gate; run `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`.
It reports `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, with screenshot
`artifacts\battle-mariofox-dl-draw-all-hwtri.png`. The hardware texture cache
key now includes the source render/load tile, TMEM, palette, and tile-origin
state for that opt-in path. The hardware triangle path now applies recorded
primitive/environment material color, including black color values, and alpha
from the current combine state, maps recorded F3DEX2 front/back cull geometry mode to DS polygon cull bits, and
uses the sm64-nds decal-combine, polygon-ID, non-shade white tint, blend
alpha-memory, and texture-filter coordinate-bias rules. The hardware upload
converter now accepts BattleShip `G_IM_FMT_IA` IA4/IA8/IA16 texels for
alpha-bearing source textures used by collision overlays, particles, and
material records; that is the latest gate.
The next renderer pass should finish remaining combiner behavior, depth
policy, remaining texture-state coverage, and renderer cutover. Default builds
still use the software preview.

Latest runtime detail: `gm/gmcollision.c` is now imported as a whole BattleShip
TU via `src/import/battleship_gmcollision.c`, replacing the local
matrix/world-position helper copies. A full `ft/ftmain.c` wrapper was tested
but rejected before landing because the current narrow item/weapon/effect,
audio, and ground headers cannot expose that TU without a broader
compatibility-header split.

## Process Change

Do not continue one-bit proof-mask increments. Future gameplay slices should
import one whole BattleShip TU, or a coherent adjacent group of TUs, prove it at
the boundary, then graduate it to live runtime by removing the guarded seam.

New harness modes are exceptional. Fold new proofs into the current pair unless
the work reaches a scene-level boundary such as `battle_playable` or
`results_screen`.

## Recommended Next Work

1. Renderer follow-up: finish opt-in hardware combiner/depth/material policy,
   broaden remaining texture-state coverage after the first all-DL CI/TLUT gate
   and IA decoder, then plan renderer cutover.
2. Runtime slice 1 follow-up: split/expand the item, weapon, effect, audio, and
   ground compatibility headers enough for full `ft/ftmain.c`, then replace
   the remaining local `ftMain*` seams and add the continuous-runtime verifier.
3. Broaden hardware texture coverage now that all-DL CI/TLUT upload is proven
   and material DL emission reaches the hardware traversal where source MObjs
   expose branchable material state.

## Verification

For docs-only edits:

```powershell
.\scripts\check-docs.ps1
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
```

For mechanical split chunks:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

For shared runtime/common fighter/renderer/harness registry changes, add the
smallest broader check that matches the touched area:

```powershell
.\scripts\verify-current.ps1 -Build -DelaySeconds 3
.\scripts\verify-regression.ps1
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
```

After verified progress, inspect status, optionally commit, then run snapshot
as the last project command:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```

## Avoid

- one-bit proof-mask increments;
- per-branch seed/restore proofs when the whole function can run naturally;
- permanent state restore around already-proven code;
- new harness modes for individual feature bits;
- broad gameplay rewrites outside original BattleShip source;
- broad compatibility headers to make imports easier;
- editing `decomp/`, generated outputs, emulator configs, logs, or runner slots.
