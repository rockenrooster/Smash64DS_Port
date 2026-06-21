# Handoff

Use this file for the active handoff only. Use `docs/STATUS.md` for the short
current-truth state, `docs/DIAGNOSTIC_REFERENCE.md` for diagnostic marker
details, and `docs/PORTING.md` for append-only history.

## Current State

The ROM boots through original BattleShip startup, bounded Opening Room,
Opening Portraits, Opening Mario, the imported fighter name-card scenes, the
bounded action-scene bridge, imported bounded Title setup, and a direct
bounded VS Mode setup harness.

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

This is not full Title/VS/menu import. Full Title input, animated logo,
labels/Press Start, slash, logo-fire particles, audio, continuous title draw,
full VS Mode input/update transitions, continuous VS menu drawing,
fighter/stage-heavy action scene internals, and gameplay remain deferred.

A project-owned NDS dev/test scene harness is now available for faster
boundary iteration. Default builds are unchanged. `NDS_DEV_SCENE_HARNESS=title`
mutates only the original-compatible `dSCManagerDefaultSceneData` before the
imported `scManagerRunLoop` copies it, entering `nSCKindTitle` from
`nSCKindOpeningNewcomers` and then running the same bounded imported
`mnTitleStartScene` path. `NDS_DEV_SCENE_HARNESS=vs_setup` enters the bounded
imported `mnVSModeStartScene` setup proof from Title.
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
```

Latest post-maintenance results:

```text
verify-runtime.ps1 -> Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.34)
verify-opening-boundary.ps1 -> frames=504 hostfps=54.30 room=420
verify-opening-skip.ps1 -> Opening Room skip verification passed (tick 10 -> Title)
verify-title-boundary.ps1 -> frames=3292 hostfps=40.52 room=1320 action=9/324 title=0x54494457
verify-all.ps1 -> Runtime, skip, title boundary, and VS setup harness gates passed
verify-title-harness.ps1 -> Title harness passed: scene=1/46 room=0 title=0x54494457
verify-vs-setup-harness.ps1 -> VS setup harness passed: scene=9/1 setup=0x1f files=2 sobj=28 buttons=0x3f
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
`title`, `vs_setup`, or `battle_fd`. Keep normal builds on the natural path.

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

1. Use the VS setup harness to identify the next narrow original menu boundary
   toward Players VS setup.
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
