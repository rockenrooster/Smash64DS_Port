# Current Status

This is the short current-truth document for active development. Keep
`docs/PORTING.md` as append-only history; use this file and `docs/HANDOFF.md`
for planning and handoff.

## Current Boundary

The ROM boots through the original BattleShip startup path, runs the bounded
Opening Room and opening movie sequence, and reaches the imported Title scene
boundary. The current Title slice loads original `MNTitle` and
`MNTitleFireAnim` O2R files, creates the original Title actor pair, logo-fire
GObj/display boundary, fire GObj/SObj/process/display boundary, four cameras,
and initial Title vars. It normalizes the 30 original Title fire frame Sprites,
runs one guarded original Title update tick on the natural movie path, and
renders a bounded original Title sprite preview.

This is still a bounded porting boundary, not full Title/menu gameplay.

A guarded NDS dev/test scene harness now exists for faster source-boundary
iteration. Normal builds still follow the natural startup/opening/movie path.
The Title harness build mutates the original-compatible
`dSCManagerDefaultSceneData` before entering imported `scManagerRunLoop`, so it
starts at `nSCKindTitle` with `scene_prev = nSCKindOpeningNewcomers` and still
runs the imported bounded `mnTitleStartScene` path. A VS setup target is wired
to `nSCKindVSMode` with `scene_prev = nSCKindTitle`. That target now imports
`mnvsmode.c` enough to run bounded original `mnVSModeStartScene` /
`mnVSModeFuncStart` setup, load the original `MNCommon` and `MNVSMode` O2R
files, create the original main GObj, default camera, viewports, VS menu
button/value/background/menu-name/subtitle SObj object graph, then park before
`mnVSModeMain` input/update transitions and continuous drawing.
The `vs_start_transition` harness enters the same VS setup boundary, advances
original `mnVSModeMain` through a bounded no-input gate, injects a synthetic
A-button tap on the original VS Start cursor, records original
`mnVSModeSaveSettings`, observes `scene_prev = nSCKindVSMode` and
`scene_curr = nSCKindPlayersVS`, reaches the original load-scene request, and
then parks at the existing PlayersVS scene stub. `mnplayersvs.c` is not
imported.
The future `battle_fd` target is reserved only; it records a reserved marker
and falls back to Title until the original one-fighter/Final-Destination battle
slice is intentionally imported.

## Next Boundary

Use the VS Start transition proof to identify the next narrow original
PlayersVS boundary. Do not import fighters, stages, battle gameplay, audio, or
full renderer systems until that exact boundary requires them.

## Known Blockers

- Full Title input, animated logo, labels/Press Start, slash, logo-fire
  particles, audio, and continuous title drawing remain deferred.
- Full VS Mode navigation/rule editing/options transition, audio, and
  continuous menu drawing remain deferred.
- PlayersVS is still only the existing scene stub/boundary; character select is
  not imported.
- Fighter/stage-heavy opening action scenes are still bounded bridge stubs in
  original order.
- Opening Room DObj rendering is still a bounded preview path, not a general
  display-list-to-DS hardware renderer.
- no$gba has only launch/window smoke automation. Use it interactively for
  VRAM/OAM/register/timing questions only when melonDS cannot answer them.
- Large maintenance files are being split for velocity:
  `src/port/scene_backend.c` and `scripts/verify-runtime.ps1`.

## Verifier Commands

Use PowerShell with devkitPro variables set:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make -j4
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
.\scripts\verify-opening-movie-speed.ps1
.\scripts\verify-title-harness.ps1
.\scripts\verify-vs-setup-harness.ps1
.\scripts\verify-vs-start-transition-harness.ps1
```

For a clean regression after header, Makefile, imported source, or linker-visible
symbol changes:

```powershell
make clean
make -j4
.\scripts\verify-all.ps1
```

Build a direct Title harness ROM without replacing the normal output:

```powershell
make TARGET=smash64ds-title BUILD=build-title-harness NDS_DEV_SCENE_HARNESS=title -j4
.\scripts\verify-title-harness.ps1
```

Build a direct VS setup harness ROM without replacing the normal output:

```powershell
make TARGET=smash64ds-vs-setup BUILD=build-vs-setup-harness NDS_DEV_SCENE_HARNESS=vs_setup -j4
.\scripts\verify-vs-setup-harness.ps1
```

Build a direct VS Start transition harness ROM without replacing the normal
output:

```powershell
make TARGET=smash64ds-vs-start BUILD=build-vs-start-harness NDS_DEV_SCENE_HARNESS=vs_start_transition -j4
.\scripts\verify-vs-start-transition-harness.ps1
```

## Latest Proof

Latest verified state after the VS Start transition harness import:

```text
make -j4
scripts/verify-vs-setup-harness.ps1 -> VS setup harness passed: scene=9/1 setup=0x1f files=2 sobj=28 buttons=0x3f
scripts/verify-vs-start-transition-harness.ps1 -> VS Start transition harness passed: scene=16/9 trans=0x56535452 mask=0xff saved=1/3/2
scripts/verify-runtime.ps1 -> Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.34)
scripts/verify-opening-boundary.ps1 -> frames=504 hostfps=54.30 room=420
scripts/verify-opening-skip.ps1 -> Opening Room skip verification passed (tick 10 -> Title)
scripts/verify-title-boundary.ps1 -> frames=3292 hostfps=40.52 room=1320 action=9/324 title=0x54494457
scripts/verify-all.ps1 -> Full verification passed; speed sample frames=3299 hostfps=40.47 title=0x54494457; VS Start scene=16/9
scripts/verify-title-harness.ps1 -> Title harness passed: scene=1/46 room=0 title=0x54494457
```

`src/port/scene_backend.c` is now a thin include orchestrator over
`diagnostics.c`, `taskman_seam.c`, `reloc_backend.c`,
`sprite_preview_backend.c`, `opening_movie_backend.c`, and `title_backend.c`.
This is the mechanical split stage; those slices intentionally keep the old
single-translation-unit static linkage.

## Do-Not-Touch Constraints

- Treat all of `decomp/` as read-only reference material.
- Do not edit generated build output, generated ROM/ELF artifacts, or generated
  emulator logs/configs except through build and verification commands.
- Do not rewrite Smash gameplay or menus by hand when original BattleShip code
  exists.
- Do not import broad renderer/fighter/stage/audio systems just to satisfy a
  narrow boundary.
- Keep DS-specific behavior in `src/nds` or `src/port`, imports in
  `src/import`, and compatibility declarations in `include`.
