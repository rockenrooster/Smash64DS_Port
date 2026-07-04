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

Modes `161/162` are the active Boundary/Latest pair. In the default
original-manager build they now prove natural Mario/Fox Wait animation plus one
input-driven Wait -> Walk transition through imported `ftanim.c`/`ftkey.c`, on
the Pupupu Mario/Fox battle root. The current short marker summary is:

```text
ftmanager natural-motion: wait=300/300, anim=299/299,
walk=8/8, updates=308, mask=0x3ff
```

Latest verified source-backed detail: TaruCannon kind `3` now routes through
the source-ordered setup shell, installs status `61`, and runs one original
physics tick copying fighter root position from the barrel root. Continuous
TaruCannon update/shoot runtime still waits for Jungle barrel helpers and map
throw-hit data.

Latest renderer detail: opt-in DS 3D hardware submission remains behind
`NDS_RENDERER_HW_TRIANGLES=1`. The current Pupupu stage-inclusive gate proves
matrix, material, texture, depth/fog/alpha, primitive-Z, and
texture-perspective hardware submission with zero texture rejects. The strict
direct Mario/Fox all-DL hardware verifier now passes on live manager-created
fighters: all 14/18 selected DObjs are clean and hardware submits 284/298
fighter triangles. The direct proof now carries the source-equivalent segment
`0xE` material register and RSP vertex cache across selected DObjs. Captures
remain under `artifacts\renderer-*-hw*.png`. Default builds still use the
software preview.

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

Runtime slice 2 graduated the original manager/status/animation path. Default
builds import `ft/ftmanager.c`, the full original common/Mario/Fox status
descriptor tables, and live `ftanim.c`/`ftanimend.c`/`ftkey.c`. The current
natural-motion gate proves Mario/Fox manager creation, valid figatree-backed
joints, 300+ Wait animation frames, and Wait -> Walk on input. A refreshed
Regression prebuild plus all four sharded `-NoBuild` Regression runs passed.
The old gcRunAll/gcDrawAll/stage/MP/dash/live-hit synthetic marker stacks are
now coverage-reduced follow-up work; do not resurrect their motion-extract seam.

## Process Change

Future gameplay slices should import one whole BattleShip TU, or a coherent
adjacent TU group, prove it at the boundary, then graduate it to live runtime.

New harness modes are exceptional. Fold new proofs into the current pair unless
the work reaches a scene-level boundary such as `battle_playable` or
`results_screen`.

## Recommended Next Work

1. Runtime follow-up: rebuild gcDrawAll/stage/MP/dash/live-hit proof coverage
   on top of the natural original-manager runtime, then remove the remaining
   `ftMainSetStatus` compat-replay/cliffmotion seams status-by-status.
2. Renderer follow-up: broaden source-scene coverage, reduce remaining fighter
   all-DL texture rejects, then plan renderer cutover.
3. Camera/HUD/match-flow work for the next `battle_playable` milestone.

## Verification

For docs-only edits, run `.\scripts\check-docs.ps1`. For mechanical split
chunks:

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
