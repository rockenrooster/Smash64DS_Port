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

Latest renderer detail: opt-in DS 3D hardware submission lives behind
`NDS_RENDERER_HW_TRIANGLES=1` and is fed by source-shaped DObj, camera, and
material state. It now covers BattleShip billboard/recalc matrix seeds, Pupupu
stage-inclusive submission, material branch packets, CI/IA/I/RGBA texture
uploads, material color/alpha, source culling, reset geometry/filter seeds,
sm64-nds-style no-z / decal-depth submission, F3DEX
`G_FOG`/`G_MW_FOG`/`G_SETFOGCOLOR` -> DS fog state, and `G_SETBLENDCOLOR`
alpha / `G_AC_THRESHOLD` -> DS alpha-test threshold state, plus BattleShip
`G_ACMUX_0`/`G_ACMUX_1` constant-alpha mux handling and 2-cycle final-output
material color selection.
Current captures:
`artifacts\renderer-stage-gcdrawall-hw-cycle2-combine.png`,
`artifacts\renderer-stage-gcdrawall-hw-alpha-mux.png`,
`artifacts\renderer-stage-gcdrawall-hw-blend-alpha.png`,
`artifacts\renderer-stage-gcdrawall-hw-fogstate.png`, and
`artifacts\battle-mariofox-dl-draw-all-hwtri.png`; the opt-in all-DL verifier
reports `hwdepth=z260/217/proj0/0/decal0/0` and
`hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, and the opt-in Pupupu
stage gcDrawAll verifier reports `hwsubmit=252`, `hwtri=1140`, and
`hwdepth=z456/proj684/decal0`, `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`,
and `hwflush=1/1`. Default builds still use the software preview.

Latest runtime detail: `gm/gmcollision.c` is imported as a whole BattleShip TU
via `src/import/battleship_gmcollision.c`, replacing the local
matrix/world-position helper copies. The shared `FTStruct` source region in
`include/ft/fighter.h` now matches BattleShip `fttypes.h` through
`display_mode`; `joints` is at `2280`, callback slots start at `2516`, the
source region is `2896` bytes, and DS/proof-only fields live after that
boundary. Static layout guards freeze the source offsets and extension boundary.
Full BattleShip `ft/ftmain.c` is now imported by default through
`src/import/battleship_ftmain.c`; the duplicate local `ftMain*` seams are gone
or routed through the imported original once. The default ladder, boundary,
continuous live-hit verifier, and four-way sharded Regression passed after a
fresh Regression prebuild, and all four Regression shards were rerun green on
current `master` after the renderer follow-ups. The regression-cycle fix
preserves the first selected cross-floor target match so later wall/cliff MP
updates cannot erase motion-stale proof evidence.

## Process Change

Do not continue one-bit proof-mask increments. Future gameplay slices should
import one whole BattleShip TU, or a coherent adjacent group of TUs, prove it at
the boundary, then graduate it to live runtime by removing the guarded seam.

New harness modes are exceptional. Fold new proofs into the current pair unless
the work reaches a scene-level boundary such as `battle_playable` or
`results_screen`.

## Recommended Next Work

1. Renderer follow-up: finish opt-in hardware combiner/material policy,
   broaden remaining texture-state and no-z/decal source-scene coverage after
   the first all-DL CI/TLUT gate plus IA/I decoders, then plan renderer
   cutover.
2. Runtime follow-up: delete the remaining `ftMainSetStatus` compat-replay and
   cliffmotion restore duplicate-behavior seams status-by-status as those source
   proofs graduate.
3. Broaden hardware texture coverage now that all-DL CI/TLUT upload is proven
   and material DL emission reaches the hardware traversal where source MObjs
   expose branchable material state; the Pupupu stage hardware path now proves
   zero rejects for its current source-loaded texture states.

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
smallest broader check that matches the touched area. For shared fighter
runtime, use sharded regression, not the serial 45-minute run:

```powershell
.\scripts\verify-current.ps1 -Build -DelaySeconds 3
.\scripts\New-MelonDSRunnerSlots.ps1 -Count 4
.\scripts\build-verify-profile.ps1 -Profile Regression -Force
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex N -RunnerSlot N -NoBuild
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
