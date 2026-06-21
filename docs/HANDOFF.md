# Handoff

Use this file for the active handoff only. Use `docs/STATUS.md` for the short
current-truth state, `docs/DIAGNOSTIC_REFERENCE.md` for diagnostic marker
details, and `docs/PORTING.md` for append-only history.

## Current State

The ROM boots through original BattleShip startup, bounded Opening Room,
Opening Portraits, Opening Mario, the imported fighter name-card scenes, the
bounded action-scene bridge, imported bounded Title setup, a direct bounded VS
Mode setup harness, a bounded original VS Start -> PlayersVS transition
harness, bounded imported PlayersVS setup, bounded imported Maps setup, and a
full guarded menu-chain proof to the existing VSBattle boundary stub.

The current Title boundary loads original `MNTitle` and `MNTitleFireAnim`,
creates the original actor/logo-fire/fire/camera/vars boundaries, normalizes
the 30 original Title fire-frame Sprites, runs one guarded original Title update
tick on the natural movie path, and renders a bounded original Title sprite
preview.

The `vs_setup` harness now enters `nSCKindVSMode` from `nSCKindTitle` and
runs imported `mnvsmode.c` setup far enough to load original `MNCommon` and
`MNVSMode`, create the original main GObj, default camera, viewports,
background, menu-name, VS Start, Rule, Time/Stock, VS Options, value, arrow,
and subtitle SObj graph, then parks at the taskman loop boundary.
`NDS_DEV_SCENE_HARNESS=vs_start_transition` starts at the same boundary, runs
the setup proof, advances original `mnVSModeMain` through the input-ready gate,
injects a synthetic A tap on VS Start, proves original `mnVSModeSaveSettings`,
observes `scene_prev = nSCKindVSMode` and `scene_curr = nSCKindPlayersVS`,
observes the original load-scene request, and then reaches the bounded imported
PlayersVS setup boundary.
`NDS_DEV_SCENE_HARNESS=players_setup` starts directly at
`nSCKindPlayersVS` from `nSCKindVSMode`, imports `mnplayersvs.c` through a
DS-owned wrapper, loads the seven original PlayersVS menu files, creates the
original main GObj/default camera/camera set/menu object graph, initializes
original PlayersVS vars and slot state, and parks before continuous
character-select input/drawing.
`NDS_DEV_SCENE_HARNESS=maps_setup` starts directly at `nSCKindMaps` from
`nSCKindPlayersVS`, imports `mnmaps.c` through a DS-owned wrapper, loads the
five original Maps files, creates the original stage-select SObj graph, starts
from seeded Pupupu/Dream Land, and explicitly defers the stage preview model
path.
`NDS_DEV_SCENE_HARNESS=menu_chain_vsbattle` starts at VS Mode, proves original
VS Start -> PlayersVS, injects a deterministic two-player PlayersVS ready/start
state, proves original PlayersVS -> Maps, injects a synthetic Maps A-select on
Pupupu, proves original Maps scene-data saving, and parks at
`scene_curr/scene_prev = 22/21` on the existing VSBattle boundary stub.

This is not full Title/VS/menu import. Full Title input, animated logo,
labels/Press Start, slash, logo-fire particles, audio, continuous title draw,
full VS Mode navigation/rule editing/options transition, continuous VS menu
drawing, full interactive PlayersVS cursor/puck selection, Maps preview model
rendering, fighter/stage-heavy action scene internals, and gameplay remain
deferred.

A project-owned NDS dev/test scene harness is now available for faster
boundary iteration. Default builds are unchanged. `NDS_DEV_SCENE_HARNESS=title`
mutates only the original-compatible `dSCManagerDefaultSceneData` before the
imported `scManagerRunLoop` copies it, entering `nSCKindTitle` from
`nSCKindOpeningNewcomers` and then running the same bounded imported
`mnTitleStartScene` path. `NDS_DEV_SCENE_HARNESS=vs_setup` enters the bounded
imported `mnVSModeStartScene` setup proof from Title.
`NDS_DEV_SCENE_HARNESS=vs_start_transition` enters the same VS setup proof and
then runs the bounded original VS Start transition probe.
`NDS_DEV_SCENE_HARNESS=players_setup` enters bounded imported PlayersVS setup.
`NDS_DEV_SCENE_HARNESS=maps_setup` enters bounded imported Maps setup.
`NDS_DEV_SCENE_HARNESS=menu_chain_vsbattle` proves the VS Mode -> PlayersVS ->
Maps -> VSBattle boundary chain.
`NDS_DEV_SCENE_HARNESS=battle_fd` is reserved only and falls back to Title
while recording a reserved marker; it does not dispatch battle, fighters, or
stages yet.

## Latest Proof

Known-good PowerShell environment:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
```

Latest maintained regression chain:

```powershell
make -j4
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
.\scripts\verify-title-boundary.ps1
.\scripts\verify-all.ps1
.\scripts\verify-title-harness.ps1
.\scripts\verify-vs-setup-harness.ps1
.\scripts\verify-vs-start-transition-harness.ps1
.\scripts\verify-players-vs-setup-harness.ps1
.\scripts\verify-maps-setup-harness.ps1
.\scripts\verify-menu-chain-vsbattle-harness.ps1
```

Latest post-maintenance results:

```text
verify-runtime.ps1 -> Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.34)
verify-opening-boundary.ps1 -> frames=504 hostfps=54.30 room=420
verify-opening-skip.ps1 -> Opening Room skip verification passed (tick 10 -> Title)
verify-title-boundary.ps1 -> frames=3292 hostfps=40.52 room=1320 action=9/324 title=0x54494457
verify-title-harness.ps1 -> Title harness passed: scene=1/46 room=0 title=0x54494457
verify-vs-setup-harness.ps1 -> VS setup harness passed: scene=9/1 setup=0x1f files=2 sobj=28 buttons=0x3f
verify-vs-start-transition-harness.ps1 -> VS Start transition harness passed: scene=16/9 trans=0x56535452 mask=0xff saved=1/3/2
verify-players-vs-setup-harness.ps1 -> PlayersVS setup harness passed: files=7 mask=0xff sobj=65 slots=2/4/4
verify-maps-setup-harness.ps1 -> Maps setup harness passed: files=5 mask=0xff sobj=36 slot=6 gkind=6
verify-menu-chain-vsbattle-harness.ps1 -> Menu-chain VSBattle harness passed: VS->PV mask=0xff, PV->Maps mask=0xff, Maps->VSBattle mask=0xff, final=22/21
verify-all.ps1 -> Full verification passed; speed sample frames=3283 hostfps=40.47 title=0x54494457; menu chain final=22/21
```

## Important Local Boundaries

- `decomp/` is read-only reference material.
- `src/import` owns wrappers for original BattleShip translation units.
- `src/nds` owns DS hardware integration.
- `src/port` owns compatibility/backend seams.
- `include` owns compatibility declarations.
- Do not edit generated build output or emulator logs/configs by hand.

`src/port/scene_backend.c` is intentionally a thin include orchestrator over:

- `src/port/diagnostics.c`
- `src/port/taskman_seam.c`
- `src/port/reloc_backend.c`
- `src/port/sprite_preview_backend.c`
- `src/port/opening_movie_backend.c`
- `src/port/title_backend.c`

The dev/test harness lives in:

- `include/nds/nds_scene_harness.h`
- `src/port/scene_harness.c`
- `src/import/battleship_scmanager.c`

Build-time harness modes are selected with `NDS_DEV_SCENE_HARNESS=normal`,
`title`, `vs_setup`, `vs_start_transition`, `players_setup`, `maps_setup`,
`menu_chain_vsbattle`, or `battle_fd`. Keep normal builds on the natural path.

Do not list those slices separately in `Makefile` `CFILES` until a later ABI
cleanup adds explicit narrow shared headers.

## Verifier Entry Points

- `scripts/verify-opening-boundary.ps1`: fast Opening Room progress gate.
- `scripts/verify-runtime.ps1`: full marker contract through current runtime
  boundary.
- `scripts/verify-opening-skip.ps1`: callback-time input injection and
  skip-to-Title proof.
- `scripts/verify-title-boundary.ps1`: natural movie-to-Title speed gate.
- `scripts/verify-title-harness.ps1`: direct Title scene harness gate without
  replaying Opening Room or the opening movie.
- `scripts/verify-vs-setup-harness.ps1`: direct VS Mode setup harness gate
  from Title without replaying Opening Room, the opening movie, or Title setup.
- `scripts/verify-vs-start-transition-harness.ps1`: direct VS Start transition
  harness gate proving original `mnVSModeMain` changes scene state to
  PlayersVS and then reaches the bounded imported PlayersVS boundary.
- `scripts/verify-players-vs-setup-harness.ps1`: direct PlayersVS setup gate.
- `scripts/verify-maps-setup-harness.ps1`: direct Maps setup gate.
- `scripts/verify-menu-chain-vsbattle-harness.ps1`: full guarded VS Mode ->
  PlayersVS -> Maps -> VSBattle boundary gate.
- `scripts/verify-all.ps1`: maintained regression chain.

Shared verifier helpers:

- `scripts/lib/melonds.ps1`
- `scripts/lib/gdb-markers.ps1`

## Emulator Policy

Use melonDS for automated pass/fail verification. Use no$gba only when melonDS
cannot answer a DS hardware question such as VRAM, OAM, palettes, BG/3D
registers, DMA, or timing. no$gba currently has smoke/window-capture automation
only, not runtime-global verification.

## Next Best Work

1. Use the VSBattle boundary proof to identify the first narrow original
   `scvsbattle.c` setup boundary.
2. Inspect the relevant BattleShip source and headers first.
3. Inspect `decomp/sm64-nds` only for DS backend architecture patterns.
4. Add project-owned shims in `src/port`, `src/nds`, or `include`.
5. Keep the source boundary bounded; do not import broad fighter/stage/audio or
   full renderer systems unless that exact boundary requires them.
6. Extend the smallest verifier that proves the new boundary.
7. Update `docs/STATUS.md`, this handoff, and `docs/PORTING.md`.

## Reference Docs

- `docs/STATUS.md`: short current-truth summary.
- `docs/ROADMAP.md`: milestone status and next gates.
- `docs/KNOWN_ISSUES.md`: stubs, warnings, and risks.
- `docs/GOAL_DEBUGGING.md`: short debug workflow.
- `docs/DIAGNOSTIC_REFERENCE.md`: detailed marker inventory.
- `docs/ARCHITECTURE.md`: subsystem boundaries.
- `docs/DECOMP_MAP.md`: read-only upstream context map.
- `docs/EMULATOR_STRATEGY.md`: melonDS/no$gba choice.
- `docs/PORTING.md`: append-only history.
