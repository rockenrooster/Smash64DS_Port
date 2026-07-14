# Goal Debugging

This file is the short workflow guide for proving progress toward the real
goal: original BattleShip Smash 64 code running through a Nintendo DS backend.
For the full diagnostic-global inventory and marker meanings, use
`docs/DIAGNOSTIC_REFERENCE.md`.

## Build

Use PowerShell:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make -j16
```

Run a clean build after changing headers, Makefile source lists, imported
source wrappers, or linker-visible compatibility symbols:

```powershell
make clean
make -j16
```

If an imported BattleShip function appears to read the wrong `FTStruct` or
other shadow-header field offset after a header edit, assume stale objects
before changing the ABI again. Rebuild the affected harness with `make -B` or
run the clean build above, then re-check the disassembly/marker.

Do not edit generated output:

- `build/`
- `smash64ds.elf`
- `smash64ds.nds`
- `smash64ds.ds.gba`

## Runtime Gates

Use the tiered verifier workflow instead of running the entire historical suite
for every task:

```powershell
.\scripts\verify-dev-fast.ps1
.\scripts\verify-boundary.ps1
.\scripts\verify-current.ps1
.\scripts\verify-regression.ps1
```

- Inner loop: `verify-dev-fast.ps1` after build and registry check.
- Normal handoff: `verify-boundary.ps1` and `verify-current.ps1`.
- Shared/runtime-heavy handoff: `verify-boundary.ps1`,
  `verify-regression.ps1`, and `verify-current.ps1`.
- Full suite: `verify-all.ps1 -Profile Full` for major snapshots, wide
  refactors, Makefile/source-list/header ABI changes, taskman/object-manager/
  controller/reloc/display shared changes, verifier registry/checker work, or
  explicit reviewer request.

`verify-current.ps1` is equivalent to `verify-all.ps1 -Profile Latest`; normally
run only one of those. If Full times out, report the exact verifier running at
timeout and resume later with `verify-all.ps1 -Profile Full -From <name>`.

For slow regression runs, use isolated melonDS runner slots instead of
parallelizing the default emulator/config:

```powershell
.\scripts\New-MelonDSRunnerSlots.ps1 -Count 4
.\scripts\build-verify-profile.ps1 -Profile Regression
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex 0 -RunnerSlot 0 -NoBuild
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex 1 -RunnerSlot 1 -NoBuild
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex 2 -RunnerSlot 2 -NoBuild
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex 3 -RunnerSlot 3 -NoBuild
```

`emulators/melonds-runners/` is generated and gitignored. Do not commit local
melonDS copies/configs, shard logs, or generated verifier temp files. Use
`RegressionFast` only when deliberately skipping the long runtime verifier.
The current smoke proof for this workflow is a 2-way Boundary run: prebuild
`Boundary`, then run shard `0` on runner slot `0` and shard `1` on runner slot
`1` with `-NoBuild`; both shards passed with slot-local logs and distinct GDB
ports.

Two advisory tools help choose and tune verifier runs:

```powershell
.\scripts\suggest-verification.ps1
.\scripts\suggest-verification.ps1 -ChangedFiles src/port/scene_harness.c
.\scripts\measure-verifier-cost.ps1 -Profile Boundary -WhatIf
.\scripts\measure-verifier-cost.ps1 -Profile Latest
```

`suggest-verification.ps1` maps git or explicit changed files to the smallest
conservative checks it can justify. `measure-verifier-cost.ps1` reuses the
maintained registry profiles, lists plans with `-WhatIf`, and writes timed
results under `artifacts/verifier-cost/` when it runs verifiers.

For handoff hygiene, use the generated-output cleaner dry run and the Lean
snapshot exporter:

```powershell
.\scripts\clean-generated.ps1 -DryRun
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```

Use `-Mode CodeOnly` for small review-only snapshots that do not need
`decomp/`. Use `-Mode Full` only for explicit debugging/repro needs because it
may include generated/local payloads. The preserved old exporter lives at
`scripts/New-Smash64DSSnapshot.Legacy.ps1` for fallback/debug reference only.
Normal Lean handoffs keep build-critical decomp source/reference files and the
top-level BattleShip O2R asset tree, but exclude upstream decomp build output,
baseroms, generated ROM/ELF/NDS files, duplicate nested O2R copies, and tool
caches. Use `-IncludeDecompGenerated` only for explicit debug snapshots. When
snapshot policy changes, prove coverage with:

```powershell
.\scripts\check-snapshot-build-context.ps1 -Mode Lean -Source .
```

Use melonDS for automated pass/fail verification:

```powershell
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
.\scripts\verify-title-boundary.ps1
.\scripts\verify-vs-setup-harness.ps1
.\scripts\verify-vs-start-transition-harness.ps1
.\scripts\verify-players-vs-setup-harness.ps1
.\scripts\verify-maps-setup-harness.ps1
.\scripts\verify-battle-fd-harness.ps1
.\scripts\verify-battle-mariofox-dash-run-harness.ps1
.\scripts\verify-battle-mariofox-jump-loop-harness.ps1
.\scripts\verify-battle-mariofox-landing-loop-harness.ps1
.\scripts\verify-battle-mariofox-process-loop-harness.ps1
.\scripts\verify-battle-mariofox-scheduler-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-floor-edge-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcross-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpadjust-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpedge-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpwall-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpstale-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mplivestale-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpmotionstale-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpfallland-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpceil-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1
.\scripts\verify-menu-chain-vsbattle-harness.ps1
.\scripts\verify-menu-chain-mariofox-dash-run-harness.ps1
.\scripts\verify-menu-chain-mariofox-jump-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-landing-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-process-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-scheduler-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-floor-edge-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcross-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpadjust-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpedge-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpwall-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpstale-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mplivestale-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpmotionstale-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpfallland-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpceil-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1
.\scripts\verify-regression.ps1
```

Use the faster Opening Room progress gate for inner-loop checks:

```powershell
.\scripts\verify-opening-boundary.ps1
```

Use the full opening movie-to-Title speed gate when changing movie pacing,
preview cadence, renderer cost, or title-boundary timing:

```powershell
.\scripts\verify-opening-movie-speed.ps1
```

Use the dev/test scene harness when the next task can start at a proven
original-code boundary instead of replaying the full opening. The maintained
harness targets are direct Title entry, bounded VS setup from Title, bounded
VS Start to PlayersVS transition from Title, direct PlayersVS setup, direct
Maps setup, direct bounded VSBattle setup, the guarded VS Mode -> PlayersVS
-> Maps -> VSBattle setup chain, and the current direct/menu-chain Mario/Fox
VSBattle update-driven scheduler-loop boundary through the current
source-order MP DownWait interrupt-loop boundary:

```powershell
.\scripts\verify-title-harness.ps1
.\scripts\verify-vs-setup-harness.ps1
.\scripts\verify-vs-start-transition-harness.ps1
.\scripts\verify-players-vs-setup-harness.ps1
.\scripts\verify-maps-setup-harness.ps1
.\scripts\verify-battle-fd-harness.ps1
.\scripts\verify-menu-chain-vsbattle-harness.ps1
.\scripts\verify-battle-mariofox-dash-run-harness.ps1
.\scripts\verify-menu-chain-mariofox-dash-run-harness.ps1
.\scripts\verify-battle-mariofox-jump-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-jump-loop-harness.ps1
.\scripts\verify-battle-mariofox-landing-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-landing-loop-harness.ps1
.\scripts\verify-battle-mariofox-process-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-process-loop-harness.ps1
.\scripts\verify-battle-mariofox-scheduler-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-scheduler-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-floor-edge-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-floor-edge-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpsweep-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpsweep-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcross-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcross-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpadjust-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpadjust-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpedge-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpedge-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpwall-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpwall-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpstale-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpstale-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mplivestale-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mplivestale-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpmotionstale-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpmotionstale-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpclifftick-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpclifftick-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpfallmap-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpfallmap-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpfallland-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpfallland-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpceil-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpceil-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpceilstatus-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpceilstatus-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffattack-action-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffattack-action-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffescape-action-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffescape-action-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffescape-common2-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffescape-common2-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1
```

That verifier builds `smash64ds-title.nds` from
`BUILD=build-title-harness NDS_DEV_SCENE_HARNESS=title`, starts at
`nSCKindTitle` with `scene_prev = nSCKindOpeningNewcomers`, proves
`room=0`, and checks the imported bounded `mnTitleStartScene` markers.
The VS setup verifier builds `smash64ds-vs-setup.nds` from
`BUILD=build-vs-setup-harness NDS_DEV_SCENE_HARNESS=vs_setup`, starts at
`nSCKindVSMode` with `scene_prev = nSCKindTitle`, proves `room=0` and no
Title setup replay, then checks the imported bounded `mnVSModeStartScene`
markers.
The VS Start transition verifier builds `smash64ds-vs-start.nds` from
`BUILD=build-vs-start-harness NDS_DEV_SCENE_HARNESS=vs_start_transition`,
starts from that same original VS setup boundary, advances bounded original
`mnVSModeMain`, injects A on VS Start, proves original settings save and
load-scene request, then parks at the bounded imported PlayersVS boundary.
The PlayersVS setup verifier builds `smash64ds-players-vs.nds` from
`BUILD=build-players-vs-setup-harness NDS_DEV_SCENE_HARNESS=players_setup`,
starts at `nSCKindPlayersVS` from `nSCKindVSMode`, and checks the bounded
imported `mnPlayersVSStartScene` setup markers.
The Maps setup verifier builds `smash64ds-maps.nds` from
`BUILD=build-maps-setup-harness NDS_DEV_SCENE_HARNESS=maps_setup`, starts at
`nSCKindMaps` from `nSCKindPlayersVS`, and checks the bounded imported
`mnMapsStartScene` setup markers with the seeded Pupupu cursor.
The Battle FD verifier builds `smash64ds-battle-fd.nds` from
`BUILD=build-battle-fd-harness NDS_DEV_SCENE_HARNESS=battle_fd`, starts at
`nSCKindVSBattle` from `nSCKindMaps`, seeds one Mario and the current Final
Destination sentinel, and checks the bounded imported `scVSBattleStartScene`
/ `scVSBattleStartBattle` setup markers.
The menu-chain verifier builds `smash64ds-menu-chain.nds` from
`BUILD=build-menu-chain-vsbattle-harness
NDS_DEV_SCENE_HARNESS=menu_chain_vsbattle`, proves original VS Start ->
PlayersVS, bounded PlayersVS ready/start -> Maps, bounded Maps A-select ->
VSBattle, and checks the same imported bounded VSBattle setup boundary.
The Dash/Run verifiers build `smash64ds-battle-mariofox-dash-run.nds` and
`smash64ds-menu-chain-mariofox-dash-run.nds`, prove original Dash -> Run ->
RunBrake movement through imported `ftcommondash.c`, `ftcommonrun.c`, and
`ftcommonrunbrake.c`, and park before continuous fighter scheduling/gameplay.
The Jump-loop verifiers build `smash64ds-battle-mariofox-jump-loop.nds` and
`smash64ds-menu-chain-mariofox-jump-loop.nds`, prove original RunBrake -> Wait
closeout, KneeBend entry through original Wait interrupt, original JumpF setup
through imported `ftcommonkneebend.c` and `ftcommonjump.c`, and six bounded
airborne frames before continuous fighter scheduling/gameplay.
The Landing-loop verifiers build
`smash64ds-battle-mariofox-landing-loop.nds` and
`smash64ds-menu-chain-mariofox-landing-loop.nds`, prove original JumpF -> Fall
through imported `ftcommonfall.c`, bounded Fall descent to the Pupupu floor,
original LandingLight setup through imported `ftcommonlanding.c`, and
LandingLight -> Wait closeout before continuous fighter scheduling/gameplay.
The Process-loop verifiers build the direct and menu-chain Mario/Fox
process-loop harnesses, drive the proven Walk, Dash/Run/RunBrake, and
Jump/Fall/Landing paths through one bounded source-order frame loop, and park
before object-manager scheduling/gameplay.
The Scheduler-loop verifiers build the direct and menu-chain Mario/Fox
scheduler-loop harnesses, attach selected `GObjProcess` callbacks with
`gcAddGObjProcess`, invoke them through `gcRunGObjProcess`, and run from the
bounded `scVSBattleFuncUpdate` taskman update path.

The shared verifier helpers live in:

- `scripts/lib/melonds.ps1`
- `scripts/lib/gdb-markers.ps1`

## Visual Checks

Use visible melonDS only when you need to inspect the live HUD or capture a
regression image:

```powershell
.\scripts\debug-melonds.ps1 -Build
.\scripts\capture-melonds.ps1 -Build
```

For the natural movie-to-Title visual boundary, use a longer capture delay:

```powershell
.\scripts\capture-melonds.ps1 -Build -DelaySeconds 145
```

Visual capture is regression evidence. Prefer source-boundary work and
machine-readable verifier checks unless a renderer contract is blocking the
next import.

## no$gba

Use no$gba when melonDS cannot answer a DS hardware question:

```powershell
.\scripts\debug-nogba.ps1 -Build
.\scripts\capture-nogba.ps1 -Build -AllWindows
.\scripts\verify-nogba-smoke.ps1 -Build
```

Good no$gba use cases:

- VRAM
- OAM
- palettes
- BG/3D registers
- DMA/timing
- debugger-visible renderer state

Do not build a deep no$gba automation layer unless a real hardware/register
blocker appears. melonDS remains the automated runtime/global verifier.

## Failure Triage

1. Reproduce with `make -j16` and the narrowest verifier that covers the change.
2. If startup, scene flow, relocation, or object counts changed, inspect
   `docs/DIAGNOSTIC_REFERENCE.md` and the relevant verifier marker.
3. Inspect original BattleShip source and headers before adding or changing a
   shim.
4. Inspect `decomp/sm64-nds` before changing DS backend architecture.
5. Add the smallest diagnostic global needed to prove the next boundary.
6. Update `docs/STATUS.md`, `docs/HANDOFF.md`, and the verifier when a runtime
   contract changes.

## Current References

- `docs/STATUS.md`: short current-truth state.
- `docs/P1_EXECUTION_BOARD.md`: only dynamic queue and acceptance decisions.
- `docs/HANDOFF.md`: exact current commands and artifact handoff.
- `docs/DIAGNOSTIC_REFERENCE.md`: diagnostic globals and manual GDB details.
- `docs/EMULATOR_STRATEGY.md`: melonDS vs no$gba routing.
- `docs/PORTING.md`: append-only history.
