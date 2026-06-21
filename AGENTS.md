# AGENTS.md

## Mission

This repo is a Nintendo DS port of the BattleShip Smash 64 decompilation. Keep
moving toward:

```text
Original Smash 64 decomp game code + Nintendo DS backend = playable port
```

Do not turn this into a handwritten clone or DS-native gameplay rewrite.

## Required Working Rules

- Inspect relevant BattleShip source before implementing a feature.
- Inspect `decomp/sm64-nds` when making architectural DS-port decisions.
- Treat everything under `decomp/` as read-only reference source. Those
  directories are independent upstream repositories; make port hooks in
  project-owned wrappers, shims, or headers instead.
- Use the whole `decomp/` tree as useful context, including BattleShip docs,
  tools, generated symbol headers, yamls, O2R resources, and `sm64-nds`;
  `decomp/BattleShip-main/decomp` is the primary original game-code source,
  not the only readable reference.
- Prefer importing original BattleShip translation units through `src/import`.
- Put DS-specific behavior in `src/nds` or `src/port`.
- Put compatibility declarations in `include/`.
- Keep generated outputs untouched.
- Keep changes small enough to build and verify.
- Update docs when adding, replacing, stubbing, or deferring a subsystem.
- Remove temporary probes before handoff; keep only verified diagnostics that are
  part of the maintained runtime boundary.

## Build And Verify

Use this environment on Windows PowerShell:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make -j4
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
```

Use the long opening-movie speed gate when changing movie pacing,
presentation, or renderer cost:

```powershell
.\scripts\verify-opening-movie-speed.ps1
```

Use the direct Title dev/test harness when the task can start from the imported
Title boundary without replaying the full opening:

```powershell
.\scripts\verify-title-harness.ps1
```

Use the direct VS setup harness when the task can start from original
`mnvsmode.c` setup without replaying Opening Room, the opening movie, or Title
setup:

```powershell
.\scripts\verify-vs-setup-harness.ps1
```

The normal build must remain the natural startup/opening path. Harness builds
use `NDS_DEV_SCENE_HARNESS=title`, `vs_setup`, or the reserved `battle_fd`
mode with a separate build target/directory.

Use the visible melonDS HUD when debugging runtime progress by eye:

```powershell
.\scripts\debug-melonds.ps1 -Build
```

Use no$gba for interactive DS hardware/register/VRAM renderer debugging:

```powershell
.\scripts\debug-nogba.ps1 -Build
```

For no$gba automated window smoke/capture:

```powershell
.\scripts\verify-nogba-smoke.ps1 -Build
.\scripts\capture-nogba.ps1 -Build -AllWindows
```

Before a renderer/hardware task, decide whether melonDS, no$gba, or both best
answer the question. Use `docs/EMULATOR_STRATEGY.md`: melonDS remains the
machine-readable GDB verifier; no$gba is preferred for DS hardware state such
as VRAM, OAM, palettes, BG/3D registers, and timing.

Local emulator binaries/configs live under `emulators/`; keep them out of the
repo root and out of source control.

Capture and inspect the actual emulator window when changing visual markers:

```powershell
.\scripts\capture-melonds.ps1 -Build
```

For the current natural opening movie-to-Title visual boundary, use:

```powershell
.\scripts\capture-melonds.ps1 -Build -DelaySeconds 135
```

Run a clean build when changing headers, Makefile source lists, imported source,
or linker-visible compatibility symbols:

```powershell
make clean
make -j4
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
```

## Current Boundary

Use `docs/STATUS.md` as the short current-truth source for active planning and
`docs/HANDOFF.md` for detailed verified boundary evidence. Keep
`docs/PORTING.md` append-only as history, not as the primary planning surface.

Current short version:

- The ROM boots through original BattleShip startup, bounded Opening Room,
  Opening Portraits, Opening Mario, fighter name-card scenes, the bounded
  action-scene bridge, imported bounded Title setup, and direct bounded VS
  setup from the dev harness.
- A guarded dev/test harness can start directly at the same imported Title
  boundary or bounded VS setup boundary for faster iteration; it does not
  replace the normal boot path.
- The current Title slice loads original `MNTitle` and `MNTitleFireAnim`,
  creates original actor/logo-fire/fire/camera/vars boundaries, normalizes the
  30 original fire-frame Sprites, runs one guarded original Title update tick,
  and renders a bounded original Title sprite preview.
- Full Title input, animated logo, labels/Press Start, slash, logo-fire
  particles, audio, continuous title draw, full VS Mode input/update
  transitions, fighter/stage-heavy action scenes, and gameplay remain
  deferred.
- `src/port/scene_backend.c` is intentionally a thin include orchestrator over
  DS-owned backend slices. Do not add those slices to `Makefile` `CFILES` until
  a later ABI cleanup introduces explicit narrow shared headers.
- Do not import all of `sys/objdisplay.c` as a broad next step. Keep renderer
  imports scoped to the next deliberately verified draw boundary.

Latest maintained proof commands are:

```powershell
make -j4
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
.\scripts\verify-title-boundary.ps1
.\scripts\verify-vs-setup-harness.ps1
.\scripts\verify-all.ps1
```

Use no$gba only when melonDS cannot answer the question, especially for VRAM,
OAM, palette, BG/3D register, DMA, or timing inspection. melonDS remains the
automated runtime/global verifier.

## Documentation To Maintain

- `docs/ARCHITECTURE.md`: subsystem boundaries and architecture.
- `docs/STATUS.md`: short current-truth summary for active planning.
- `docs/HANDOFF.md`: current verified state and next work.
- `docs/ROADMAP.md`: milestone status.
- `docs/KNOWN_ISSUES.md`: stubs, warnings, and risks.
- `docs/GOAL_DEBUGGING.md`: short debug and verifier workflow.
- `docs/DIAGNOSTIC_REFERENCE.md`: diagnostic globals and marker inventory.
- `docs/DECOMP_MAP.md`: read-only upstream reference folder map.
- `docs/EMULATOR_STRATEGY.md`: emulator choice and no$gba automation boundary.
- `docs/PORTING.md`: chronological porting log.

Use `docs/STATUS.md` and `docs/HANDOFF.md` for current planning. Keep
`docs/PORTING.md` append-only as history; do not make it the primary planning
surface.

When a runtime diagnostic changes, update both the verifier and the docs that
describe it.

## Editing Policy

- Use `apply_patch` for source edits.
- Do not revert user or generated changes unless explicitly requested.
- Do not edit `build/`, generated ROM/ELF artifacts, or generated emulator
  logs/configs except through build/verification commands.
- Do not edit `decomp/`; use `src/import`, `src/port`, `src/nds`, or `include`
  for DS-port changes.
- Do not add BattleShip's full `include` directory globally without checking
  header conflicts.
- Avoid broad compatibility headers. Add the ABI needed by the imported source
  slice and no more.

## How To Add The Next Original Subsystem

1. Read the original source and headers.
2. Identify the next real runtime boundary.
3. Add a wrapper in `src/import` for the original `.c` file.
4. Add the wrapper to the explicit `CFILES` list in `Makefile`.
5. Add missing ABI to `include/`.
6. Add backend behavior or diagnostics in `src/port` or `src/nds`.
7. Build.
8. Fix the smallest real missing contract.
9. Extend `scripts/verify-runtime.ps1` if the new boundary can be proven.
10. Update docs.

## What Not To Do

- Do not manually approximate moves, physics, hitboxes, or menus.
- Do not replace BattleShip scene logic with a DS-native scene.
- Do not treat a stub as a completed subsystem.
- Do not remove diagnostics without replacing them with stronger evidence.
- Do not optimize memory/rendering before the original code path is proven.
