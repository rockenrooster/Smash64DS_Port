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
make -j4
```

Run a clean build after changing headers, Makefile source lists, imported
source wrappers, or linker-visible compatibility symbols:

```powershell
make clean
make -j4
```

Do not edit generated output:

- `build/`
- `smash64ds.elf`
- `smash64ds.nds`
- `smash64ds.ds.gba`

## Runtime Gates

Use melonDS for automated pass/fail verification:

```powershell
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
.\scripts\verify-title-boundary.ps1
.\scripts\verify-all.ps1
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
original-code boundary instead of replaying the full opening. The current
maintained harness target is direct Title entry:

```powershell
.\scripts\verify-title-harness.ps1
```

That verifier builds `smash64ds-title.nds` from
`BUILD=build-title-harness NDS_DEV_SCENE_HARNESS=title`, starts at
`nSCKindTitle` with `scene_prev = nSCKindOpeningNewcomers`, proves
`room=0`, and checks the imported bounded `mnTitleStartScene` markers.

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

1. Reproduce with `make -j4` and the narrowest verifier that covers the change.
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
- `docs/HANDOFF.md`: detailed current boundary and next work.
- `docs/DIAGNOSTIC_REFERENCE.md`: diagnostic globals and manual GDB details.
- `docs/EMULATOR_STRATEGY.md`: melonDS vs no$gba routing.
- `docs/PORTING.md`: append-only history.
