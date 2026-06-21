# Diagnostic Reference

This file preserves the detailed diagnostic-global inventory, marker meanings,
manual GDB notes, and historical verifier details. For day-to-day debugging
workflow, use `docs/GOAL_DEBUGGING.md`.

## Build Environment

Use devkitPro/libnds. On this machine, the known-good PowerShell setup is:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make -j4
```

Clean build:

```powershell
make clean
make -j4
```

Generated outputs:

- `build/`
- `smash64ds.elf`
- `smash64ds.nds`
- `smash64ds.ds.gba`

Do not edit generated output.

## Emulator Layout

Local emulator binaries and configs live under `emulators/`:

```text
emulators/melonds/melonDS.exe
emulators/melonds/melonDS.toml
emulators/nogba/NO$GBA.EXE
```

Generated emulator stdout/stderr logs are written under
`artifacts/emulator-logs/`. Emulator binaries/configs are ignored by Git; the
repo tracks only scripts and layout docs.

## Runtime Verification

With `emulators/melonds/melonDS.exe` in the workspace:

```powershell
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
.\scripts\verify-opening-boundary.ps1
.\scripts\verify-title-boundary.ps1
.\scripts\verify-title-harness.ps1
.\scripts\verify-vs-setup-harness.ps1
.\scripts\verify-vs-start-transition-harness.ps1
.\scripts\verify-players-vs-setup-harness.ps1
.\scripts\verify-maps-setup-harness.ps1
.\scripts\verify-menu-chain-vsbattle-harness.ps1
.\scripts\verify-all.ps1
```

Use `verify-opening-boundary.ps1` as the quick current Opening Room progress
gate, `verify-title-boundary.ps1` for the natural movie-to-Title speed gate,
`verify-title-harness.ps1` for the direct imported Title boundary without
Opening Room/movie replay, `verify-vs-setup-harness.ps1` for the direct
bounded imported VS Mode setup proof from Title,
`verify-vs-start-transition-harness.ps1` for the bounded original VS Start to
PlayersVS transition proof, `verify-players-vs-setup-harness.ps1` for bounded
imported PlayersVS setup, `verify-maps-setup-harness.ps1` for bounded imported
Maps setup, `verify-menu-chain-vsbattle-harness.ps1` for the guarded VS Mode ->
PlayersVS -> Maps -> VSBattle proof, and `verify-all.ps1` for the maintained
full regression chain. The shared PowerShell helpers live in
`scripts/lib/melonds.ps1` and `scripts/lib/gdb-markers.ps1`.

The script:

1. ensures melonDS has a config under `emulators/melonds/`
2. enables the ARM9 GDB stub (`Enabled = true`, plus the melonDS 1.1
   compatibility key `Enable = true`) without duplicating either key in
   `[Instance0.Gdb]`
3. launches the ROM hidden
4. waits for the melonDS ARM9 listener on port `3333`
5. connects `arm-none-eabi-gdb` to `127.0.0.1:3333`
6. reads diagnostic globals from `smash64ds.elf`
7. restores or removes the temporary melonDS config

The verifier is intentionally stronger than "the ROM opened." It checks that
specific original systems ran.

`verify-opening-skip.ps1` attaches while the movie is advancing, breaks at the
real `mvOpeningRoomFuncRun` callback after tick 9, injects N64 A into the real
BattleShip controller device, and continues to the Title parking call. It
verifies scene `28 -> 1`, object ejection, and the second taskman return.

`verify-title-harness.ps1` builds `TARGET=smash64ds-title
BUILD=build-title-harness NDS_DEV_SCENE_HARNESS=title`, starts the imported
scene manager from `nSCKindTitle` with `scene_prev =
nSCKindOpeningNewcomers`, verifies `gNdsOpeningRoomTickCount == 0`, and checks
the same bounded imported Title markers as the natural path.

`verify-vs-setup-harness.ps1` builds `TARGET=smash64ds-vs-setup
BUILD=build-vs-setup-harness NDS_DEV_SCENE_HARNESS=vs_setup`, starts the
imported scene manager from `nSCKindVSMode` with `scene_prev = nSCKindTitle`,
verifies Opening Room and Title setup did not replay, and checks the bounded
imported VS setup markers.

`verify-vs-start-transition-harness.ps1` builds `TARGET=smash64ds-vs-start
BUILD=build-vs-start-harness NDS_DEV_SCENE_HARNESS=vs_start_transition`,
starts from the same original `mnvsmode.c` setup boundary, advances bounded
original `mnVSModeMain`, injects a synthetic A tap through the original
controller globals, proves original `mnVSModeSaveSettings` and
`syTaskmanSetLoadScene` ran, and verifies the bounded imported PlayersVS
boundary was reached as `scene_curr/scene_prev = 16/9`.

`verify-players-vs-setup-harness.ps1` builds
`TARGET=smash64ds-players-vs BUILD=build-players-vs-setup-harness
NDS_DEV_SCENE_HARNESS=players_setup`, starts from `nSCKindPlayersVS` with
`scene_prev = nSCKindVSMode`, verifies Opening Room/Title/VS transition work
did not replay, and checks the bounded imported PlayersVS setup markers.

`verify-maps-setup-harness.ps1` builds `TARGET=smash64ds-maps
BUILD=build-maps-setup-harness NDS_DEV_SCENE_HARNESS=maps_setup`, starts from
`nSCKindMaps` with `scene_prev = nSCKindPlayersVS`, verifies the seeded
Pupupu/Dream Land cursor, and checks the bounded imported Maps setup markers.

`verify-menu-chain-vsbattle-harness.ps1` builds
`TARGET=smash64ds-menu-chain BUILD=build-menu-chain-vsbattle-harness
NDS_DEV_SCENE_HARNESS=menu_chain_vsbattle`, proves original VS Start ->
PlayersVS, bounded PlayersVS ready/start -> Maps, bounded Maps A-select ->
VSBattle, and verifies final `scene_curr/scene_prev = 22/21`.

## Visual melonDS Debugging

Use the visible HUD when you need to watch runtime progress directly:

```powershell
.\scripts\debug-melonds.ps1 -Build
```

The HUD is rendered by `src/nds/nds_platform.c`:

- Top screen: no status rail, progress bars, or debug rectangles. The top
  framebuffer is reserved for original-asset previews so live melonDS output
  does not read as flashing.
- Top previews: converted original startup `N64Logo` Sprite from the bounded
  draw pass, the smaller bounded Opening Room material-candidate DObj
  display-list preview, the original Opening Portraits/name-card SObjs during
  the natural movie path, and the bounded original `MNTitle` sprite composite
  once the Title boundary is reached.
- Moving top-screen markers are disabled. The older compact trace strip, thin
  progress ticks, orange logo-position marker, and status rectangles were
  removed from the top framebuffer because they read as flashing once original
  assets became the visual signal.
- Bottom screen text: live diagnostic globals also checked by
  `scripts/verify-runtime.ps1`, including
  `event=4f524631 tick=280 mask=03` and
  `edata=4f524644 m=0f d=4 dl=3 a=3 op=3` once the first-event probes pass,
  plus `evrun=4f523238 def=01 f=4f524646/...`,
  `s1cam=4f523143 m=1ff vp=1`,
  `ccam=4f524343 m=1f vp=1`,
  `wcam=4f525743 m=1f vp=1`,
  `lcam=4f52434d m=7f a=01 vp=1`,
  `ovl=4f524f43/4f524f45 m=03`,
  `logo=4f524c43/4f524c45 m=03 c=3f`,
  `boss=4f524243/4f524245 m=03 c=3f`,
  `pencil=4f525043 m=3f g=1 d=3 x=6`, and
  `pencil a=0 p=1 dl=1 t=3 r=1` after the original pencils update path,
  followed by `or38=4f523338 d=01`, `or45=4f523435 d=00 su=03`,
  `obj d=0f h=0f o=0f s=0f c=07`, `or50=4f523530 d=01 sp=ff`,
  `or56=4f523536 d=01 e=07`, `s2=1ff a=01 g=2 c=2 x=4`,
  `dlp=4f524450 t=4 p>0 x=3f`, and
  `draw=4f524457 b=3 c=3 d=5` at the Opening Room bounded draw boundary, plus
  compact movie rows such as `mv h=... p=...`,
  `por t=150 d=4f504457 v=4`, `mario t=60 v=... px=...`,
  `name m=fe d=ff c=7`, `fps=60 up=.. dl=.. cv=..`, and
  `ch=... pf=... smp=.. win=..` after the natural movie path reaches the Title
  preview through the paced action bridge. The FPS rows are sampled about once
  per 60 DS VBlanks: `fps` is the ROM-side present rate, `up` is imported
  opening-movie update cadence, `dl` is retained DObj-preview draw cadence,
  `cv` is original-preview commits per sampled second, `ch` is total
  original-preview commits, and `pf` is the paced present-frame count. They do
  not measure host wall-clock emulator slowdown directly.
  The top
  framebuffer is double-buffered for the HUD, the startup logo preview is
  placed from N64 logical coordinates without downscaling the retained copy, the
  Opening Room preview is smaller and moved away from the native logo, the
  legacy DS-native moving probe overlay is disabled, and the bottom console
  redraw is change-driven with milestone-bucketed tick text, a hidden cursor,
  and row-addressed short lines to avoid text wrapping, scrolling, full-screen
  clear flashes, and steady-state pulse from live frame counters on the DS
  bottom screen.

Use this for visual triage; use `verify-runtime.ps1` for pass/fail evidence.

## no$gba Hardware Debugging

Use no$gba when renderer or hardware behavior needs interactive inspection of
DS registers, VRAM, OAM, palettes, backgrounds, timings, or debugger state:

```powershell
.\scripts\debug-nogba.ps1 -Build
```

The launcher defaults to `emulators/nogba/NO$GBA.EXE` and accepts `-NoGba` and
`-Rom` overrides. no$gba is not wired into the automated pass/fail verifier
yet; use it to diagnose hardware/rendering questions, then prove stable runtime
behavior with the melonDS verifier scripts.

The no$gba debugger build can run as one combined debug window or as separate
debugger/emulator windows depending on local settings. This machine is
currently configured for one `No$gba Debugger (Fullversion)` window. Capture all
visible no$gba windows for renderer/hardware handoff evidence:

```powershell
.\scripts\capture-nogba.ps1 -Build -AllWindows
.\scripts\verify-nogba-smoke.ps1 -Build
```

Use `docs/EMULATOR_STRATEGY.md` before renderer work to decide whether the next
test belongs in melonDS, no$gba, or both.

Capture the actual melonDS window to a PNG (default: timestamped file under
`artifacts/`) with:

```powershell
.\scripts\capture-melonds.ps1 -Build
```

For the current natural movie-to-Title visual boundary, use a longer wait:

```powershell
.\scripts\capture-melonds.ps1 -Build -DelaySeconds 145
```

This launches the real ROM, temporarily disables melonDS GDB in the config for
the visible capture attempt, foregrounds melonDS, waits for the live HUD to
reach the requested boundary, captures the emulator
window, and restores the original config. Inspect the PNG when changing HUD
layout or when handoff needs visual evidence; generated captures are ignored by
Git. If melonDS starts hidden/windowless, treat that as an emulator/session
launch issue before changing ROM code. The latest smoke capture after moving
emulators under `emulators/` is `artifacts/melonds-20260620-160953.png`; use a
longer `-DelaySeconds` value when the handoff needs a post-opening Title
capture.
The startup logo can look coarse when melonDS magnifies the native `128x108`
debug copy; that is expected for the diagnostic path and is separate from
renderer-quality work.
Visual capture is currently regression evidence, not the next implementation
focus. Prefer source-code boundary work first; only spend time on visual
debugging when the next original scene/menu import needs a renderer contract or
when a capture is needed to prove a maintained regression.

## Diagnostic Globals

Boot and OS:

- `gNdsBootSelfTestResult`: queue/thread self-test, expected `0x50415353`.
- `gNdsOriginalBootStage`: service-thread and scene bitfield, expected
  `0x53430007`.

Scheduler and frame loop:

- `sSYSchedulerTicCount`: original scheduler VBlank count, must be nonzero.
- `gNdsFrameCounter`: DS frame loop count, must be nonzero.
- `gNdsPerfPresentFps`: ROM-side present rate sampled over about 60 DS
  VBlanks. This is not host wall-clock FPS.
- `gNdsPerfLogicFps`: sampled imported opening-movie update rate. This can be
  `0` at the final runtime verifier sample after the movie has parked at Title.
- `gNdsPerfDLDrawFps`: sampled retained DObj-preview draw rate.
- `gNdsOriginalSpritePreviewCommitCount` /
  `gNdsOriginalDLPreviewCommitCount`: total retained original-preview content
  commits by source.
- `gNdsPerfPreviewCommitFps`: sampled original-preview content commits per
  second.
- `gNdsPerfPreviewCommitCount`: total retained original-preview commits.
- `gNdsPerfSampleCount` / `gNdsPerfSampleWindowTicks`: prove the low-noise
  sampler has published at least one bounded window.

Runtime speed sampling:

- `scripts/verify-runtime.ps1` reports `verifyfps=...` in its final pass line.
  This is a fixed-window verifier average and should not be treated as live
  emulator speed.
- `scripts/sample-runtime-speed.ps1 -DelaySeconds 8` is the quick one-shot
  host-speed probe. Current evidence is `frames=503`, `hostfps=54.28`,
  `romfps=60`, and `room=420` after about nine wall-clock seconds.
- A longer one-shot sample after limiting retained Opening Room DObj work to
  two original draw probes plus retained-preview reuse reached `frames=2257`,
  `hostfps=48.74`, `romfps=60`, `dl=60`, `ch=25`, `room=1320`,
  `rdraw=2/24`, `portraits=150`, `mario=60`, and action progress `4/128`
  after about 46 wall-clock seconds.
- `scripts/verify-opening-movie-speed.ps1` is the maintained full-opening
  host-speed gate. It wraps the sample script with checks for Title marker
  `0x54494457`, Opening Room tick `1320`, all nine action-preview boundaries,
  at least `324` paced action-preview frames, and a default hidden-melonDS
  host-speed floor of `30` FPS. Current passing evidence is `frames=3289`,
  `hostfps=40.47`, `romfps=60`, `room=1320`, `rdraw=2/24`,
  `portraits=150`, `mario=60`, `action=9/324`, and
  `title=0x54494457`.
- The FPS/content counters already rejected cadence-only speedups for this
  boundary. `NDS_OPENING_MOVIE_DRAW_INTERVAL=10` missed the Title/action
  verifier window, and `20` reached Title but made the strict Opening Room
  DObj blocker sample unreliable. Keep the verified interval at `30` until
  renderer cost is reduced.
- Avoid repeated GDB polling for progress. In testing, repeated attach/detach
  caused melonDS packet errors and prevented the Title boundary from being
  reached.

Controller:

- `gSYControllerConnectedNum`: original controller discovery count, expected
  `1`.

Opening movie / Opening Portraits:

- `gNdsOpeningMovieRoomHandoffResult`: Opening Room natural movie handoff
  marker, expected `0x4F4D5248`.
- `gNdsOpeningMovieRoomHandoffTick`: current verified natural handoff tick,
  expected `1320`.
- `gNdsOpeningMovieRoomHandoffScene`: requested next scene, expected `29`
  (`nSCKindOpeningPortraits`).
- `gNdsOpeningPortraitsStartResult`: imported Opening Portraits scene start
  marker, expected `0x4F505354`.
- `gNdsOpeningPortraitsFuncStartResult`: imported Opening Portraits task
  `func_start` marker, expected `0x4F504653`.
- `gNdsOpeningPortraitsRelocResult`: portrait O2R relocation/normalization
  marker, expected `0x4F50524C`.
- `gNdsOpeningPortraitsSpriteNormalizeCount` /
  `gNdsOpeningPortraitsSpriteNormalizeFailCount`: current expected pair `9,0`
  for four Set1 cards, four Set2 cards, and the Set1 cover metadata.
- `gNdsOpeningPortraitsDrawResult`: bounded original SObj preview draw marker,
  expected `0x4F504457`.
- `gNdsOpeningPortraitsDrawWidth` / `Height` / `Format` / `Size` /
  `Bitmaps` / `Pixels`: current successful card-draw evidence,
  `300,55,0,2,11` with nonzero accumulated pixels.
- `gNdsOpeningPortraitsNextSceneResult`: original next-scene marker, expected
  `0x4F504E58`.
- `gNdsOpeningPortraitsNextSceneKind`: expected `30`
  (`nSCKindOpeningMario`).
- `gNdsOpeningMarioDrawResult`: bounded original Mario name-card SObj draw
  marker, expected `0x4F4D4457`.
- `gNdsOpeningNameSceneDispatchMask` / `DrawMask`: imported name-card scene
  coverage, expected `0xFE` dispatch and `0xFF` draw for Donkey through
  Pikachu.
- `gNdsOpeningMovieBridgeResult`: bounded action-scene bridge marker,
  expected `0x4F4D4252`.
- `gNdsOpeningMovieBridgeMask`: action-scene bridge coverage for
  `OpeningRun` through `OpeningNewcomers`, expected `0x1FF`.
- `gNdsOpeningMovieTitleResult`: natural Title dispatch marker, expected
  `0x4F4D5449`.
- `gNdsSceneHarnessResult`: dev/test harness marker. Direct Title harness
  expects `0x4841524E` (`HARN`); the reserved battle slot reports
  `0x48525356` (`HRSV`) until a real battle boundary exists.
- `gNdsSceneHarnessMode`: build-time harness mode: `0` normal, `1` direct
  Title, `2` bounded VS setup from Title, `3` reserved battle/Final-Destination
  slot, `4` bounded VS Start to PlayersVS transition from Title, `5` direct
  PlayersVS setup, `6` direct Maps setup, and `7` guarded VS Mode -> PlayersVS
  -> Maps -> VSBattle chain.
- `gNdsSceneHarnessSceneCurr` / `ScenePrev`: default scene pair preseeded
  before imported `scManagerRunLoop` copies it. Direct Title expects `1/46`
  (`nSCKindTitle` from `nSCKindOpeningNewcomers`).
- `gNdsSceneHarnessReservedMask`: `0` for the maintained Title, VS setup,
  VS Start transition, PlayersVS, Maps, and menu-chain harnesses. The reserved
  battle slot sets bit `0` and falls back to Title instead of dispatching
  fighters/stages.
- `gNdsTitleOriginalStartResult` / `FuncStartResult`: imported
  `mntitle.c` bounded start markers, expected `0x54495354` and `0x54494653`.
- `gNdsTitleOriginalSetupMask`: current bounded Title setup coverage,
  expected `0xF` for file load, actor creation, camera creation, and var init.
- `gNdsTitleOriginalLoadedFileCount`: original Title file-list load count,
  expected `2` (`MNTitle` and `MNTitleFireAnim`).
- `gNdsTitleOriginalGObjCount` / `CameraCount`: bounded original Title setup
  evidence, expected at least two actor GObjs and exactly four camera GObjs.
- `gNdsTitleOriginalDeferredMask`: explicitly deferred Title branches,
  expected `0x1E` for the remaining logo, labels/Press Start, slash, and
  logo-fire-particle paths after the bounded fire object branch runs.
- `gNdsTitleOriginalLogoFireResult` / `LogoFireMask`: original
  `efParticleInitAll` plus `mnTitleMakeLogoFire` boundary proof, expected
  `TITLE_LOGO_FIRE=0x544c4643,0x3f,1,4,3,1,0`.
- `gNdsTitleFireSpriteNormalizeCount` / `FailCount`: original
  `MNTitleFireAnim` frame Sprite normalization proof, expected `30,0`.
- `gNdsTitleOriginalFireResult` / `FireMask`: original `mnTitleMakeFire`
  object/process/display proof. Natural movie path expects
  `TITLE_FIRE=0x54464952,0xfff,1,2,1,2,786432,0`; the skip-to-Title path
  expects the same marker with shown-fire state
  `TITLE_FIRE=0x54464952,0xfff,1,2,0,2,786432,255`.
- `gNdsTitleOriginalUpdateResult` / `UpdateCount`: bounded original Title
  update proof, expected `0x54495550` and `1` on the natural
  `OpeningNewcomers -> Title` path.
- `gNdsTitleOriginalLayout` / `TransitionTics` /
  `StartActorProcess` / `ProceedScene` / `ProceedWait`: expected
  `0,1,1,0,3` after the one safe update tick. In the skip-to-Title verifier,
  the guarded updater deliberately leaves `TITLE_ORIGINAL_UPDATE=0,0,1,169,0,0,3`
  because that path starts at the later non-opening layout boundary.
- `gNdsTitleRelocResult`: original `MNTitle` O2R load/normalization marker for
  the retained preview, expected `0x5449524C`.
- `gNdsTitlePreviewResult`: bounded Title sprite preview marker, expected
  `0x54495056`.
- `gNdsTitleDrawResult`: at least one Title SObj rendered, expected
  `0x54494457`.
- `gNdsTitleSpriteNormalizeCount` / `FailCount`: current expected pair `10,0`.
- `gNdsTitleDrawVisibleSObjCount` / `RenderableSObjCount` /
  `SObjCount` / `Pixels`: current expected counts `10,10,10` with nonzero
  accumulated pixels.
- `gNdsVSModeOriginalStartResult` / `FuncStartResult`: imported
  `mnvsmode.c` bounded start markers, expected `0x56535354` (`VSST`) and
  `0x56534653` (`VSFS`).
- `gNdsVSModeOriginalRelocResult`: original VS setup file-list load marker,
  expected `0x5653524C` (`VSRL`) after `MNCommon` and `MNVSMode` load.
- `gNdsVSModeOriginalSetupResult`: bounded original VS setup marker, expected
  `0x56535355` (`VSSU`).
- `gNdsVSModeOriginalSetupMask`: current bounded VS setup coverage, expected
  `0x1F` for file load, main GObj, default camera, viewports, and menu SObj
  graph creation.
- `gNdsVSModeOriginalLoadedFileCount`: original VS setup file-list load count,
  expected `2`.
- `gNdsVSModeOriginalGObjCount` / `CameraCount` / `SObjCount`: current
  bounded VS setup object proof, expected at least `8`, at least `1`, and at
  least `20`; current verifier proof is `17,1,28`.
- `gNdsVSModeOriginalButtonMask`: button/value SObj proof, expected `0x3F`
  for VS Start, Rule, Time/Stock, VS Options, Rule value, and Time/Stock value
  object creation.
- `gNdsVSModeOriginalCursorIndex` / `Rule` / `Time` / `Stock`: original VS
  setup state sampled after `mnVSModeFuncStartVars`; current harness proof is
  `0,0,3,2`.
- `gNdsVSModeOriginalDeferredMask`: explicitly deferred VS branches, expected
  `0x7` for `mnVSModeMain` input/update, scene transition, and continuous
  drawing.
- `gNdsVSModeStartTransitionResult`: bounded original VS Start transition
  marker. Expected pass is `0x56535452` (`VSTR`); fail marker is
  `0x5653464C` (`VSFL`).
- `gNdsVSModeStartTransitionMask`: expected `0xFF`. Bit `0` proves the setup
  boundary was complete at entry; bit `1` proves original `mnVSModeMain`
  reached the input-ready tick window; bit `2` proves the synthetic A tap was
  injected through original controller globals; bit `3` proves original
  `mnVSModeMain` changed scene state to `VSMode -> PlayersVS`; bit `4` proves
  original `mnVSModeSaveSettings` stored rule/time/stock; bit `5` proves the
  original start branch set `sMNVSModeExitInterrupt`; bit `6` proves the
  follow-up original tick requested `syTaskmanSetLoadScene`; bit `7` proves
  bounded cleanup ran before returning to the scene manager.
- `gNdsVSModeStartTransitionUpdateCount`: bounded original `mnVSModeMain` call
  count, expected at least `11` for nine idle ticks, one A tap tick, and one
  follow-up load-scene tick.
- `gNdsVSModeStartTransitionInputMask`: synthetic input used for the transition,
  expected `0x8000` (`A_BUTTON`).
- `gNdsVSModeStartTransitionScenePrevBefore` /
  `SceneCurrBefore`: expected `1/9` before the A tap.
- `gNdsVSModeStartTransitionScenePrevAfterTap` /
  `SceneCurrAfterTap`: expected `9/16` after original `mnVSModeMain` accepts
  VS Start.
- `gNdsVSModeStartTransitionScenePrevFinal` /
  `SceneCurrFinal`: expected `9/16` after the follow-up original load-scene
  tick.
- `gNdsVSModeStartTransitionExitInterrupt`: expected `1`, proving the original
  VS exit-interrupt flag was set by the start branch.
- `gNdsVSModeStartTransitionTaskmanStatus`: expected `1`
  (`nSYTaskmanStatusLoadScene`) after the follow-up original tick.
- `gNdsVSModeStartTransitionSavedRule` /
  `SavedTime` / `SavedStock`: expected `1/3/2`, proving the original transfer
  battle settings were saved as time rule, three minutes, two stock.
- `gNdsVSModeStartTransitionButtonMaskAfter`: expected `0x3F`, proving the
  original VS setup button/value SObj proof still survived the transition
  probe.
- `gNdsVSModeStartTransitionCleanupCount`: expected `1`, proving the bounded
  transition probe performed one object cleanup before returning to the scene
  manager.
- `gNdsPlayersVSOriginalStartResult` / `FuncStartResult`: imported
  `mnplayersvs.c` bounded start markers, expected `0x50565354` (`PVST`) and
  `0x50564653` (`PVFS`).
- `gNdsPlayersVSOriginalRelocResult`: original PlayersVS file-list load marker,
  expected `0x5056524C` (`PVRL`) after seven menu files load.
- `gNdsPlayersVSOriginalSetupResult`: bounded original PlayersVS setup marker,
  expected `0x50565355` (`PVSU`).
- `gNdsPlayersVSOriginalSetupMask`: expected `0xFF`. It covers reloc/file
  load, main GObj, default camera, controller-order/vars setup, fighter-manager
  compatibility calls, figatree heap allocation, camera/UI object graph setup,
  and light-parameter setup.
- `gNdsPlayersVSOriginalLoadedFileCount`: expected `7`.
- `gNdsPlayersVSOriginalGObjCount` / `CameraCount` / `SObjCount`: bounded
  PlayersVS object proof; current verifier proof is `sobj=65`.
- `gNdsPlayersVSOriginalControllerOrderMask`,
  `SlotKindMask`, `SlotSelectedMask`, `CursorCount`, `PuckCount`,
  `GateCount`, `PortraitCount`, and `FigatreeHeapCount`: setup-state evidence
  sampled after original PlayersVS init and object creation.
- `gNdsPlayersVSOriginalTime` / `Stock` / `GameRule` / `IsTeam` /
  `IsStageSelect`: expected `3/2/time-rule/0/1` for the seeded VS defaults.
- `gNdsPlayersVSReadyTransitionResult`: bounded original PlayersVS ready/start
  marker. Expected pass is `0x50565452` (`PVTR`); fail marker is
  `0x5056464C` (`PVFL`).
- `gNdsPlayersVSReadyTransitionMask`: expected `0xFF`. It proves setup was
  complete, deterministic two-player selected state was seeded, original
  `mnPlayersVSFuncRun` reached the input-ready/proceed window, synthetic Start
  was injected as `0x1000`, original scene state changed PlayersVS -> Maps,
  the original load-scene request was observed, and bounded cleanup ran.
- `gNdsPlayersVSReadyTransitionScenePrevBefore` /
  `SceneCurrBefore` / `ScenePrevFinal` / `SceneCurrFinal`: expected
  `9/16 -> 16/21` in the menu-chain harness.
- `gNdsPlayersVSReadyTransitionPlayerCount` /
  `CpuCount` / `P0FKind` / `P1FKind` / `StageSelect`: expected at least two
  players, zero CPUs, deterministic Mario/Fox player seeds, and stage select
  enabled for the bounded proof.
- `gNdsMapsOriginalStartResult` / `FuncStartResult`: imported `mnmaps.c`
  bounded start markers, expected `0x4D415053` (`MAPS`) and `0x4D504653`
  (`MPFS`).
- `gNdsMapsOriginalRelocResult`: original Maps file-list load marker, expected
  `0x4D50524C` (`MPRL`) after five files load.
- `gNdsMapsOriginalSetupResult`: bounded original Maps setup marker, expected
  `0x4D505355` (`MPSU`).
- `gNdsMapsOriginalSetupMask`: expected `0xFF`. It covers reloc/file load,
  bounded model-heap proof, main GObj, default camera, vars, camera set,
  wallpaper/plaque/labels/icons/name/cursor SObj graph, and explicit preview
  defer recording.
- `gNdsMapsOriginalLoadedFileCount`: expected `5`.
- `gNdsMapsOriginalCursorSlot` / `GroundKind`: expected `6/6` for the seeded
  Pupupu/Dream Land direct Maps and menu-chain harnesses.
- `gNdsMapsOriginalPreviewDeferred` / `DeferredMask`: expected `1/0x1`,
  proving the stage preview model path is deliberately parked.
- `gNdsMapsSelectTransitionResult`: bounded original Maps A-select marker.
  Expected pass is `0x4D53454C` (`MSEL`); fail marker is `0x4D53464C`
  (`MSFL`).
- `gNdsMapsSelectTransitionMask`: expected `0xFF`. It proves setup was
  complete, cursor/ground kind were seeded to Pupupu, original input-ready
  ticks ran, synthetic A was injected as `0x8000`, original scene data saved
  the selected ground kind, scene state changed Maps -> VSBattle, load-scene
  status was observed, and bounded cleanup ran.
- `gNdsMapsSelectTransitionScenePrevBefore` /
  `SceneCurrBefore` / `ScenePrevFinal` / `SceneCurrFinal`: expected
  `16/21 -> 21/22` in the menu-chain harness.
- `gNdsMapsSelectTransitionSelectedSlot` /
  `SelectedGKind`: expected `6/6`.
- `gNdsControllerPollCount`: DS SI/controller polls, must be nonzero.
- `gSYControllerMain`: original global controller state after update.

Video:

- `gNdsVideoBootstrapResult`: original video bootstrap result, expected
  `0x56494430`.

NitroFS and O2R payload loading:

- `gNdsRelocAssetInitResult`: NitroFS initialization marker, expected
  `0x4E465349` (`NFSI`).
- `gNdsRelocAssetHeaderReadCount`: O2R header reads. Runtime verification
  expects at least `16`; the current full-opening staged set reads headers for
  startup, Opening Room, Opening Portraits, name cards, action previews, and
  Title resources.
- `gNdsRelocAssetPayloadReadCount`: O2R payload copies, expected `33` in the
  full runtime verifier. The early Opening Room skip verifier expects `12`
  because it loads Startup, Opening Room, and bounded Title preview resources.
- `gNdsRelocAssetOpenFailCount`, `gNdsRelocAssetFormatFailCount`, and
  `gNdsRelocAssetShortReadCount`: loader failure counters, expected `0`.

Scene and startup:

- `gNdsSceneBoundaryResult`: scene boundary marker, expected `0x53434E45`.
- `gNdsSceneBoundaryKind`: scene at the current DS parking boundary, expected
  `28` at the bounded Opening Room draw/preview boundary after the tick-560 Scene 2
  camera boundary.
- `gNdsStartupTaskmanResult`: startup reached task-manager start, expected
  `0x53545254`.
- `gNdsStartupTaskmanSceneKind`: startup scene kind, expected `27`.
- `gNdsStartupTaskmanDL0Size`: original startup DL buffer 0 size, expected
  `10240`.
- `gNdsStartupTaskmanDL1Size`: original startup DL buffer 1 size, expected
  `10240`.
- `gNdsStartupTaskmanControllerSet`: startup task setup used original
  `syControllerFuncRead`, expected `1`.
- `gNdsStartupFuncStartResult`: startup `func_start` ran, expected
  `0x46535452`.
- `gNdsStartupSkipAllowWait`: original skip delay after `mnStartupFuncStart`,
  expected `8`.
- `gNdsStartupProceedOpening`: original opening flag after startup init,
  expected `0`.
- `gNdsStartupGObjCreateCount`: startup created actor, default camera,
  wallpaper camera, and wallpaper GObjs through the imported object manager,
  expected `4`.
- `gNdsStartupCameraCreateCount`: startup requested default and wallpaper
  cameras, expected `2`.
- `gNdsStartupRelocInitCount`: startup initialized relocation once, expected
  `1`.
- `gNdsStartupSpriteCreateCount`: startup made one logo sprite object,
  expected `1`.
- `gNdsStartupFadeCreateCount`: startup requested one fade actor, expected `1`.
- `gNdsStartupWallpaperParentValid`: logo SObj parent link points at the
  wallpaper GObj, expected `1`.
- `gNdsStartupLogoPosX` / `gNdsStartupLogoPosY`: original logo position,
  expected `96` and `220`.
- `gNdsStartupLogoFastcopyCleared`: startup cleared `SP_FASTCOPY`, expected
  `1`.
- `gNdsStartupLogoRelocResult`: startup `N64Logo` O2R load/fixup marker,
  expected `0x4C524C43` (`LRLC`).
- `gNdsStartupLogoRelocSize`: startup `N64Logo` payload size, expected
  `29712`.
- `gNdsStartupLogoRelocWordSwapCount`: word byte-swaps for the startup logo
  payload, expected `7428`.
- `gNdsStartupLogoRelocPointerFixupCount`: internal pointer slots patched for
  the startup logo payload, expected `9`.
- `gNdsStartupLogoDrawResult`: bounded startup logo draw marker, expected
  `0x4C445257` (`LDRW`).
- `gNdsStartupLogoDrawBlocker`: startup logo draw blocker enum, expected `0`.
- `gNdsStartupLogoDrawCallbackCount`: number of times `lbCommonDrawSObjAttr`
  ran for the startup logo path, expected at least `1`.
- `gNdsStartupLogoDrawUpdateCount`: bounded update index used for the draw
  sample, expected `17`.
- `gNdsStartupLogoDrawWidth` / `Height`: converted Sprite size, expected
  `128` / `108`.
- `gNdsStartupLogoDrawFormat` / `Size` / `Bitmaps`: Sprite format metadata,
  expected `0` / `2` / `8` for RGBA16 chunks.
- `gNdsStartupLogoDrawPixels`: converted pixel count, expected greater than
  `1000` and currently `13824`.
- `gNdsStartupLogoDrawSObjAttr`: original SObj Sprite attributes sampled from
  the draw callback; the verifier requires `SP_TEXSHUF` (`0x200`) to remain set
  for the startup logo asset.
- `gNdsStartupLogoDrawTexshuf` /
  `gNdsStartupLogoDrawTexshufSamples`: prove the DS startup preview applied the
  inverse odd-row TMEM line swizzle for the original `SP_TEXSHUF` Sprite strips;
  expected `1` and more than `1000` swizzled samples.
- `gNdsStartupActorFuncSet`: actor GObj keeps `mnStartupActorFuncRun`, expected
  `1`.
- `gNdsStartupWallpaperProcessKind`: wallpaper process kind, expected `0`
  (`nGCProcessKindThread`).
- `gNdsStartupWallpaperProcessPriority`: wallpaper process priority, expected
  `1`.
- `gNdsStartupWallpaperDisplaySet`: wallpaper display callback/link/tag
  survived the imported object-manager setup, expected `1`.
- `gNdsStartupWallpaperCameraMaskLow`: wallpaper camera mask low bits, expected
  `1`.
- `gNdsStartupDefaultCameraColor`: default camera fill color, expected `0xFF`.
- `gNdsTaskmanBridgeResult`: startup reached the DS task-loop parking seam,
  expected
  `0x5441534B`.
- `gNdsTaskmanContexts`: original startup task contexts, expected `2`.
- `gNdsTaskmanTaskGfxNum`: original startup task gfx count, expected `1`.
- `gNdsTaskmanGraphicsHeapSize`: per-context graphics heap size, expected
  `10240`.
- `gNdsTaskmanRdpKind`: startup RDP output buffer kind after taskman setup,
  expected `2`.
- `gNdsTaskmanRdpBufferSize`: startup RDP output buffer size, expected `49152`.
- `gNdsStartupTaskmanMallocCount`: real imported `syTaskmanMalloc` calls
  through startup task/object setup, expected `36`.
- `gNdsTaskmanMallocCount`: cumulative imported taskman allocations across
  Startup and Opening Room; it must exceed the startup snapshot.
- `gNdsTaskmanGeneralHeapUsed`: imported taskman general heap usage, expected greater
  than `90000`.
- `gNdsTaskmanDLContextsValid`: display-list buffers allocated for both startup
  contexts, expected `2`.
- `gNdsTaskmanControllerAutoRead`: startup represented BattleShip's controller
  auto-read contract, expected `1`.
- `gNdsTaskmanSceneUpdateSet`, `gNdsTaskmanSceneDrawSet`,
  `gNdsTaskmanLightsSet`: startup task callbacks preserved, expected `1`.
- `gNdsTaskmanLoopReached`: `syTaskmanLoadScene` reached the bounded DS
  `syTaskmanRunTask` seam, expected `1`.
- `gNdsTaskmanBoundedUpdateCount`: bounded original startup updates completed
  inside the DS seam, expected `55`.
- `gNdsTaskmanPostUpdateSkip`: startup actor skip delay after the bounded
  updates, expected `0`.
- `gNdsTaskmanGObjThreadSleeps`: logo GObj thread entered
  `gcSleepCurrentGObjThread`, expected `55`.
- `gNdsTaskmanPostUpdateLogoPosX` / `gNdsTaskmanPostUpdateLogoPosY`: logo SObj
  position after bounded updates, expected `96` and `65`.
- `gNdsTaskmanPostUpdateOpening`: original logo thread set
  `sMNStartupIsProceedOpening`, expected `1`.
- `gNdsTaskmanPostUpdateSceneKind`: requested next scene, expected `28`
  (`nSCKindOpeningRoom`).
- `gNdsTaskmanPostUpdateScenePrev`: previous scene, expected `27`
  (`nSCKindStartup`).
- `gNdsTaskmanPostUpdateStatus`: original taskman status after the scene
  request, expected `1` (`nSYTaskmanStatusLoadScene`).
- `gNdsTaskmanPostUpdateGObjCount`: active GObjs after the original break/eject
  path, expected `0`.
- `gNdsTaskmanPostUpdateFadeCount`: startup fade requests after the logo
  thread runs, expected `2`.
- `gNdsTaskmanCleanupResult`: taskman cleanup completion marker, expected
  `0x434C4E50` (`CLNP`).
- `gNdsTaskmanCleanupQueuesEmpty`: context/reset/game-tic queues drained after
  cleanup, expected `1`.
- `gNdsTaskmanCleanupMode`: original taskman terminal mode after cleanup,
  expected `2`.
- `gNdsTaskmanReturnCount`: bounded taskman seam returned to its caller,
  expected `1`.
- `gNdsOpeningRoomDispatchCount`: original scene manager dispatched
  `mvOpeningRoomStartScene`, expected `1`.
- `gNdsOpeningRoomStartResult`: imported scene entry marker, expected
  `0x4F525354` (`ORST`).
- `gNdsOpeningRoomFuncStartResult`: imported scene `func_start`, expected
  `0x4F524653` (`ORFS`).
- `gNdsOpeningRoomRelocResult`: Opening Room relocation-list marker, expected
  `0x4F52524C` (`ORRL`).
- `gNdsOpeningRoomRelocInitCount`: Opening Room called `lbRelocInitSetup`,
  expected `1`.
- `gNdsOpeningRoomRelocLoadCount`: number of original Opening Room file IDs
  passed through `lbRelocLoadFilesListed`, expected `8`.
- `gNdsOpeningRoomRelocFileMask`: bitmask of resolved Opening Room file IDs,
  expected `0xFF`.
- `gNdsOpeningRoomRelocHeaderMask`: bitmask of validated Opening Room O2R
  headers, expected `0xFF`.
- `gNdsOpeningRoomRelocPayloadMask`: bitmask of copied Opening Room O2R
  payloads, expected `0xFF`.
- `gNdsOpeningRoomRelocContentReady`: payload-byte readiness, expected `1`
  after the NitroFS/O2R load milestone.
- `gNdsOpeningRoomRelocFixupReady`: full mixed-width struct and
  renderer-specific asset fixup readiness, expected `0` until those phases
  exist.
- `gNdsOpeningRoomRelocBytesLoaded`: total copied Opening Room payload bytes,
  expected `329248`.
- `gNdsOpeningRoomRelocLastFileID` / `gNdsOpeningRoomRelocLastSize`: last file
  loaded in the current list, expected `90` / `158928`.
- `gNdsOpeningRoomRelocWordSwapMask`: bitmask of current O2R files whose
  payload words were blanket byte-swapped from N64 big-endian to ARM9 native
  order, expected `0xFF`.
- `gNdsOpeningRoomRelocWordSwapCount`: swapped `u32` words across the current
  eight payloads, expected `82312`.
- `gNdsOpeningRoomRelocWordSwapFailCount`: invalid staged file counter for the
  blanket word-swap pass, expected `0`.
- `gNdsOpeningRoomRelocPointerFixupMask`: bitmask of current O2R files whose
  internal relocation pointer chains were patched, expected `0xFF`.
- `gNdsOpeningRoomRelocPointerFixupCount`: patched internal pointer slots,
  expected `711`.
- `gNdsOpeningRoomRelocPointerFixupFailCount`: malformed chain or unsupported
  external dependency counter, expected `0`.
- `gNdsOpeningRoomRelocSymbolResolveCount`: selected `ll...` offset probes
  plus the real original Scene 1/Scene 2 camera, logo-camera, logo,
  boss-shadow, pencils, Outside, Haze, sunlight, Desk, and spotlight lookups
  resolved through `ndsRelocGetFileData`, expected `38` in the normal no-input
  path.
- `gNdsOpeningRoomRelocSymbolResolveFailCount`: selected symbol probe failures,
  expected `0`.
- `gNdsOpeningRoomRelocSymbolProbeMask`: selected BattleShip symbol probe mask,
  expected `0x7FFFF`.
- `gNdsOpeningRoomRelocLastSymbolOffset`: final normal-path lookup offset,
  expected `0` (`llMVOpeningRoomScene2CamAnimJoint`).
- `gNdsOpeningRoomRelocMObjSubNormalizeCount` /
  `MObjSubNormalizeFailCount` / `MObjSubFirstFlags`: narrow MVCommon
  `MObjSub` mixed-width normalization evidence for the current material probe,
  expected `18`, `0`, and `0x200`.
- `gNdsOpeningRoomRelocMObjSubSourceResult`: ORMT source-scan marker, expected
  `0x4F524D54`.
- `gNdsOpeningRoomRelocMObjSubTextureFlagCount`: normalized records whose
  effective flags include `MOBJ_FLAG_TEXTURE`, expected `0`.
- `gNdsOpeningRoomRelocMObjSubZeroFlagCount`: normalized records that fell
  back to zero flags, expected `0`.
- `gNdsOpeningRoomRelocMObjSubPrimColorCount`: normalized records whose
  effective flags include `MOBJ_FLAG_PRIMCOLOR`, expected `18`.
- `gNdsOpeningRoomRelocMObjSubLightCount`: normalized records whose effective
  flags include the light bit, expected `1`.
- `gNdsOpeningRoomRelocMObjSubFirstTextureOffset` /
  `gNdsOpeningRoomRelocMObjSubFirstTextureFlags`: first texture-bearing source
  offset/flags if one is present, expected `0xFFFFFFFF` and `0`.
- `gNdsOpeningRoomFirstEventResult`: first tick-280 asset-reference probe
  marker, expected `0x4F524631` (`ORF1`).
- `gNdsOpeningRoomFirstEventTick`: original event tick proven by the probe,
  expected `280`.
- `gNdsOpeningRoomFirstEventProbeMask`: pencils DObj/animation readiness mask,
  expected `0x3`.
- `gNdsOpeningRoomFirstEventPencilsDObjOffset`: BattleShip
  `llMVCommonRoomPencilsDObjDesc` offset resolved through the DS backend,
  expected `44728`.
- `gNdsOpeningRoomFirstEventPencilsAnimOffset`: BattleShip
  `llMVCommonRoomPencilsAnimJoint` offset resolved through the DS backend,
  expected `44912`.
- `gNdsOpeningRoomFirstEventDataResult`: first tick-280 pencils data-shape
  marker, expected `0x4F524644` (`ORFD`).
- `gNdsOpeningRoomFirstEventDataMask`: descriptor/table readiness mask,
  expected `0xF`.
- `gNdsOpeningRoomFirstEventPencilsDObjEntries`: resolved pencils DObjDesc
  entries including the terminator, expected `4`.
- `gNdsOpeningRoomFirstEventPencilsDLPtrs`: in-payload display-list pointers
  reachable from the first three DObjDesc entries, expected `3`.
- `gNdsOpeningRoomFirstEventPencilsAnimJoints`: in-payload animation script
  pointers reachable from the animation table, expected `3`.
- `gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode`: first animation command
  opcode, expected `3` (`nGCAnimEvent32SetValBlock`).
- `gNdsOpeningRoomFirstEventRunResult`: tick-280 update callback marker,
  expected `0x4F523238` (`OR28`).
- `gNdsOpeningRoomFirstEventDeferredMask`: deferred first-event bitmask,
  expected `0x01` because fighter creation is the only deferred branch.
- `gNdsOpeningRoomFighterDeferredResult`: deferred fighter boundary marker,
  expected `0x4F524646` (`ORFF`).
- `gNdsOpeningRoomFighterDeferredKind`: fighter kind that would have been
  passed to `mvOpeningRoomMakePulledFighter`, expected to equal
  `gNdsOpeningRoomPulledFighterKind`.
- `gNdsOpeningRoomOverlayCreateResult`: original logo-wallpaper overlay setup
  marker, expected `0x4F524F43` (`OROC`).
- `gNdsOpeningRoomOverlayDisplaySet`: overlay display callback/link readiness,
  expected `1`.
- `gNdsOpeningRoomOverlayAlphaInit`: original overlay alpha initialization,
  expected `255`.
- `gNdsOpeningRoomOverlayCreateGObjCount`: setup-time object count after
  overlay creation, expected `8`.
- `gNdsOpeningRoomOverlayEjectResult`: original overlay ejection marker,
  expected `0x4F524F45` (`OROE`).
- `gNdsOpeningRoomOverlayEjectBeforeGObjCount` /
  `gNdsOpeningRoomOverlayEjectAfterGObjCount`: original
  `gcGetGObjsActiveNum` samples around `gcEjectGObj`, expected `14` / `14`
  because that helper includes free-list objects.
- `gNdsOpeningRoomOverlayEjectUnlinkedMask`: proof that the overlay pointer was
  removed from both original common and display-link lists, expected `0x3`.
- `gNdsOpeningRoomScene1CameraCreateResult`: original
  `mvOpeningRoomMakeScene1Cameras` creation marker, expected `0x4F523143`
  (`OR1C`).
- `gNdsOpeningRoomScene1CameraCreateMask`: readiness mask for the two original
  camera GObjs, two CObjs, four XObjs, display/process/camanim/viewport, and
  DL-buffer checks, expected `0x1FF`.
- `gNdsOpeningRoomScene1CameraCreateGObjCount`: setup-time object count after
  Scene 1 camera creation, expected `4`.
- `gNdsOpeningRoomScene1CameraGObjDelta` / `CObjDelta` / `XObjDelta` /
  `AObjDelta`: original object-manager deltas from Scene 1 camera setup,
  expected `2` / `2` / `4` / `0`.
- `gNdsOpeningRoomScene1CameraDisplaySet` /
  `ProcessSet` / `AnimSet` / `ViewportSet` / `DLBufferSet`: original parked
  display callbacks, process callbacks, camanim attachments, viewport values,
  and DL-buffer flags, expected `1` for each.
- `gNdsOpeningRoomCloseUpOverlayCameraCreateResult`: original
  `mvOpeningRoomMakeCloseUpOverlayCamera` creation marker, expected
  `0x4F524343` (`ORCC`).
- `gNdsOpeningRoomCloseUpOverlayCameraCreateMask`: readiness mask for original
  close-up overlay camera GObj/CObj/no-XObj/display/viewport checks, expected
  `0x1F`.
- `gNdsOpeningRoomCloseUpOverlayCameraCreateGObjCount`: setup-time object count
  after close-up overlay camera creation, expected `5`.
- `gNdsOpeningRoomCloseUpOverlayCameraGObjDelta` / `CObjDelta` / `XObjDelta`:
  original object-manager deltas from close-up overlay camera setup, expected
  `1` / `1` / `0`.
- `gNdsOpeningRoomCloseUpOverlayCameraDisplaySet` /
  `gNdsOpeningRoomCloseUpOverlayCameraViewportSet`: original parked sprite
  camera display callback and viewport checks, expected `1` / `1`.
- `gNdsOpeningRoomWallpaperCameraCreateResult`: original
  `mvOpeningRoomMakeWallpaperCamera` creation marker, expected `0x4F525743`
  (`ORWC`).
- `gNdsOpeningRoomWallpaperCameraCreateMask`: readiness mask for original
  wallpaper-camera GObj/CObj/no-XObj/display/viewport checks, expected `0x1F`.
- `gNdsOpeningRoomWallpaperCameraCreateGObjCount`: setup-time object count
  after wallpaper-camera creation, expected `6`.
- `gNdsOpeningRoomWallpaperCameraGObjDelta` / `CObjDelta` / `XObjDelta`:
  original object-manager deltas from wallpaper-camera setup, expected
  `1` / `1` / `0`.
- `gNdsOpeningRoomWallpaperCameraDisplaySet` /
  `gNdsOpeningRoomWallpaperCameraViewportSet`: original parked sprite camera
  display callback and viewport checks, expected `1` / `1`.
- `gNdsOpeningRoomLogoCameraAssetMask`: logo-camera camanim readiness mask,
  expected `0x1`.
- `gNdsOpeningRoomLogoCameraAnimOffset`: BattleShip
  `llMVOpeningRoomScene1CamAnimJoint` offset, expected `0`.
- `gNdsOpeningRoomLogoCameraCreateResult`: original
  `mvOpeningRoomMakeLogoCamera` creation marker, expected `0x4F52434D`
  (`ORCM`).
- `gNdsOpeningRoomLogoCameraCreateMask`: readiness mask for the original
  camera GObj/CObj/XObj/display/process/camanim/viewport checks, expected
  `0x7F`.
- `gNdsOpeningRoomLogoCameraCreateGObjCount`: setup-time object count after
  logo-camera creation, expected `7`.
- `gNdsOpeningRoomLogoCameraGObjDelta` / `CObjDelta` / `XObjDelta` /
  `AObjDelta`: original object-manager deltas from logo-camera setup, expected
  `1` / `1` / `2` / `0`.
- `gNdsOpeningRoomLogoCameraDisplaySet` /
  `gNdsOpeningRoomLogoCameraProcessSet` /
  `gNdsOpeningRoomLogoCameraAnimSet` /
  `gNdsOpeningRoomLogoCameraViewportSet`: original parked camera display
  callback, process callback, camanim attachment, and viewport checks, expected
  `1` / `1` / `1` / `1`.
- `gNdsOpeningRoomLogoAssetMask`: logo DObj/MObj/MatAnim readiness mask,
  expected `0x7`.
- `gNdsOpeningRoomLogoDObjOffset`: BattleShip
  `llMVCommonRoomLogoDObjDesc` offset, expected `115880`.
- `gNdsOpeningRoomLogoMObjOffset`: BattleShip `llMVCommonRoomLogoMObjSub`
  offset, expected `113760`.
- `gNdsOpeningRoomLogoMatAnimOffset`: BattleShip
  `llMVCommonRoomLogoMatAnimJoint` offset, expected `116012`.
- `gNdsOpeningRoomLogoCreateResult`: original `mvOpeningRoomMakeLogo` creation
  marker, expected `0x4F524C43` (`ORLC`).
- `gNdsOpeningRoomLogoCreateMask`: readiness mask for the original logo
  GObj/DObj/XObj/MObj/display/material-animation checks, expected `0x3F`.
- `gNdsOpeningRoomLogoCreateGObjCount`: setup-time object count after logo
  creation, expected `9`.
- `gNdsOpeningRoomLogoGObjDelta` / `DObjDelta` / `XObjDelta` / `MObjDelta` /
  `AObjDelta`: original object-manager deltas from logo setup, expected
  `1` / `2` / `4` / `1` / `0`.
- `gNdsOpeningRoomLogoDisplaySet` / `LogoMObjSet` /
  `LogoMatAnimSet`: original parked display callback, material object, and
  material animation attachment checks, expected `1` / `1` / `1`.
- `gNdsOpeningRoomLogoEjectResult`: original logo ejection marker, expected
  `0x4F524C45` (`ORLE`).
- `gNdsOpeningRoomLogoEjectBeforeGObjCount` /
  `gNdsOpeningRoomLogoEjectAfterGObjCount`: original
  `gcGetGObjsActiveNum` samples around ejection, expected `14` / `14`.
- `gNdsOpeningRoomLogoEjectUnlinkedMask`: proof that the logo pointer was
  removed from both original common and display-link lists, expected `0x3`.
- `gNdsOpeningRoomBossShadowAssetMask`: boss-shadow display-list and animation
  readiness mask, expected `0x3`.
- `gNdsOpeningRoomBossShadowDisplayListOffset`: BattleShip
  `llMVCommonRoomBossShadowDisplayList` offset, expected `128912`.
- `gNdsOpeningRoomBossShadowAnimOffset`: BattleShip
  `llMVCommonRoomBossShadowAnimJoint` offset, expected `129316`.
- `gNdsOpeningRoomBossShadowCreateResult`: original
  `mvOpeningRoomMakeBossShadow` creation marker, expected `0x4F524243`
  (`ORBC`).
- `gNdsOpeningRoomBossShadowCreateMask`: readiness mask for the original
  boss-shadow GObj/DObj/XObj/process/display/animation checks, expected
  `0x3F`.
- `gNdsOpeningRoomBossShadowCreateGObjCount`: setup-time object count after
  boss-shadow creation, expected `10`.
- `gNdsOpeningRoomBossShadowGObjDelta` / `DObjDelta` / `XObjDelta` /
  `AObjDelta`: original object-manager deltas from setup, expected
  `1` / `1` / `1` / `0`.
- `gNdsOpeningRoomBossShadowProcessSet` /
  `gNdsOpeningRoomBossShadowDisplaySet` /
  `gNdsOpeningRoomBossShadowAnimSet`: original process, parked display
  callback, and animation attachment checks, expected `1` / `1` / `1`.
- `gNdsOpeningRoomBossShadowEjectResult`: original boss-shadow ejection
  marker, expected `0x4F524245` (`ORBE`).
- `gNdsOpeningRoomBossShadowEjectBeforeGObjCount` /
  `gNdsOpeningRoomBossShadowEjectAfterGObjCount`: original
  `gcGetGObjsActiveNum` samples around ejection, expected `14` / `14`.
- `gNdsOpeningRoomBossShadowEjectUnlinkedMask`: proof that the boss-shadow
  pointer was removed from both original common and display-link lists,
  expected `0x3`.
- `gNdsOpeningRoomPencilsCreateResult`: original
  `mvOpeningRoomMakePencils` creation marker from inside the tick-280 update,
  expected `0x4F525043` (`ORPC`).
- `gNdsOpeningRoomPencilsCreateMask`: readiness mask for the original pencils
  GObj/DObj/XObj/process/display/animation-root checks, expected `0x3F`.
- `gNdsOpeningRoomPencilsGObjDelta`: original object-manager common GObj
  increase from the pencils call, expected `1`.
- `gNdsOpeningRoomPencilsDObjDelta`: DObj increase from
  `gcSetupCommonDObjs`, expected `3`.
- `gNdsOpeningRoomPencilsXObjDelta`: XObj increase from original transform
  setup, expected `6`.
- `gNdsOpeningRoomPencilsAObjDelta`: AObj increase during the first
  `gcPlayAnimAll`, expected `0` for this first `SetValBlock` slice.
- `gNdsOpeningRoomPencilsProcessSet`: original common update process attached,
  expected `1`.
- `gNdsOpeningRoomPencilsDisplaySet`: parked `gcDrawDObjTreeForGObj` display
  callback attached, expected `1`.
- `gNdsOpeningRoomPencilsDObjTreeCount`: DObj tree nodes walked by original
  `gcGetTreeDObjNext`, expected `3`.
- `gNdsOpeningRoomPencilsAnimRootCount`: root animation marker count, expected
  `1`.
- `gNdsOpeningRoomTick380DeferredResult`: tick-380 deferred branch marker,
  expected `0x4F523338` (`OR38`).
- `gNdsOpeningRoomTick380DeferredMask`: deferred fighter-status/rotation mask,
  expected `0x01`.
- `gNdsOpeningRoomTick450RunResult`: tick-450 update marker, expected
  `0x4F523435` (`OR45`).
- `gNdsOpeningRoomTick450DeferredMask`: no remaining tick-450 deferred branch,
  expected `0x00`.
- `gNdsOpeningRoomOutsideAssetMask`: Outside display-list readiness mask,
  expected `0x1`.
- `gNdsOpeningRoomOutsideDisplayListOffset`: BattleShip
  `llMVCommonRoomOutsideDisplayList` offset, expected `147968`.
- `gNdsOpeningRoomOutsideCreateResult`: wrapper-created Outside object marker,
  expected `0x4F524F55` (`OROU`).
- `gNdsOpeningRoomOutsideCreateMask`: readiness mask for the Outside
  GObj/DObj/XObj/display-link checks, expected `0x0F`.
- `gNdsOpeningRoomOutsideCreateGObjCount`: setup-time object count after
  Outside creation, expected `11`.
- `gNdsOpeningRoomOutsideGObjDelta` / `DObjDelta` / `XObjDelta`: object-manager
  deltas from Outside setup, expected `1` / `1` / `1`.
- `gNdsOpeningRoomOutsideDisplaySet`: original display callback/link readiness
  for display link 6, expected `1`.
- `gNdsOpeningRoomHazeAssetMask`: Haze display-list readiness mask, expected
  `0x1`.
- `gNdsOpeningRoomHazeDisplayListOffset`: BattleShip
  `llMVCommonRoomHazeDisplayList` offset, expected `39160`.
- `gNdsOpeningRoomHazeCreateResult`: wrapper-created Haze object marker,
  expected `0x4F52485A` (`ORHZ`).
- `gNdsOpeningRoomHazeCreateMask`: readiness mask for the Haze
  GObj/DObj/XObj/display-link checks, expected `0x0F`.
- `gNdsOpeningRoomHazeCreateGObjCount`: setup-time object count after Haze
  creation, expected `12`.
- `gNdsOpeningRoomHazeGObjDelta` / `DObjDelta` / `XObjDelta`: object-manager
  deltas from Haze setup, expected `1` / `1` / `1`.
- `gNdsOpeningRoomHazeDisplaySet`: original display callback/link readiness
  for display link 6, expected `1`.
- `gNdsOpeningRoomSunlightAssetMask`: sunlight display-list readiness mask,
  expected `0x1`.
- `gNdsOpeningRoomSunlightDisplayListOffset`: BattleShip
  `llMVCommonRoomSunlightDisplayList` offset, expected `149256`.
- `gNdsOpeningRoomSunlightCreateResult`: wrapper-created sunlight object marker,
  expected `0x4F525343` (`ORSC`).
- `gNdsOpeningRoomSunlightCreateMask`: readiness mask for the sunlight
  GObj/DObj/XObj/display-link checks, expected `0x0F`.
- `gNdsOpeningRoomSunlightCreateGObjCount`: setup-time object count after
  sunlight creation, expected `13`.
- `gNdsOpeningRoomSunlightGObjDelta` / `DObjDelta` / `XObjDelta`: object-manager
  deltas from sunlight setup, expected `1` / `1` / `1`.
- `gNdsOpeningRoomSunlightDisplaySet`: original display callback/link readiness
  for display link 6, expected `1`.
- `gNdsOpeningRoomSunlightEjectResult`: sunlight ejection marker, expected
  `0x4F525345` (`ORSE`).
- `gNdsOpeningRoomSunlightEjectBeforeGObjCount` /
  `gNdsOpeningRoomSunlightEjectAfterGObjCount`: original
  `gcGetGObjsActiveNum` samples around ejection, expected `14` / `14`.
- `gNdsOpeningRoomSunlightEjectUnlinkedMask`: proof that the sunlight pointer
  was removed from both original common and display-link lists, expected `0x3`.
- `gNdsOpeningRoomCloseUpOverlayCreateResult`: original
  `mvOpeningRoomMakeCloseUpOverlay` creation marker from inside the tick-450
  update, expected `0x4F52434F` (`ORCO`).
- `gNdsOpeningRoomCloseUpOverlayCreateMask`: readiness mask for the close-up
  overlay GObj/display-link/alpha checks, expected `0x07`.
- `gNdsOpeningRoomCloseUpOverlayCreateTick`: original actor tick at creation,
  expected `450`.
- `gNdsOpeningRoomCloseUpOverlayCreateGObjCount`: original
  `gcGetGObjsActiveNum` sample after close-up overlay creation, expected `14`.
- `gNdsOpeningRoomCloseUpOverlayGObjDelta`: original object-manager common GObj
  increase from the close-up overlay call, expected `1`.
- `gNdsOpeningRoomCloseUpOverlayDisplaySet`: original display callback/link
  readiness for display link 26, expected `1`.
- `gNdsOpeningRoomCloseUpOverlayAlphaInit`: original close-up overlay alpha
  initialization, expected `0`.
- `gNdsOpeningRoomTick500RunResult`: tick-500 update marker, expected
  `0x4F523530` (`OR50`).
- `gNdsOpeningRoomTick500DeferredMask`: deferred pulled-fighter display-link
  movement mask, expected `0x01`.
- `gNdsOpeningRoomSpotlightAssetMask`: spotlight display-list/MObj/material
  animation asset readiness mask, expected `0x07`.
- `gNdsOpeningRoomSpotlightDisplayListOffset`: BattleShip
  `llMVCommonRoomSpotlightDisplayList` offset, expected `142872`.
- `gNdsOpeningRoomSpotlightMObjOffset`: BattleShip
  `llMVCommonRoomSpotlightMObjSub` offset, expected `142480`.
- `gNdsOpeningRoomSpotlightMatAnimOffset`: BattleShip
  `llMVCommonRoomSpotlightMatAnimJoint` offset, expected `143120`.
- `gNdsOpeningRoomSpotlightCreateResult`: original
  `mvOpeningRoomMakeSpotlight` creation marker from tick 500, expected
  `0x4F52534C` (`ORSL`).
- `gNdsOpeningRoomSpotlightCreateMask`: readiness mask for spotlight
  GObj/DObj/XObj/MObj/display/process/material-animation/position checks,
  expected `0xFF`.
- `gNdsOpeningRoomSpotlightCreateTick`: original actor tick at creation,
  expected `500`.
- `gNdsOpeningRoomSpotlightCreateGObjCount`: original
  `gcGetGObjsActiveNum` sample after spotlight creation, expected `14`.
- `gNdsOpeningRoomSpotlightGObjDelta` / `DObjDelta` / `XObjDelta` /
  `MObjDelta` / `AObjDelta`: object-manager deltas for spotlight creation,
  expected `1`, `1`, `1`, `2`, and `0`.
- `gNdsOpeningRoomSpotlightDisplaySet`, `ProcessSet`, `MObjSet`,
  `MatAnimSet`, `PositionSet`: spotlight object link checks, expected `1`.
- `gNdsOpeningRoomTick560RunResult`: tick-560 update marker, expected
  `0x4F523536` (`OR56`).
- `gNdsOpeningRoomTick560DeferredMask`: deferred Boss fighter status mask,
  expected `0x01`.
- `gNdsOpeningRoomScene2CameraAssetMask`: Scene 2 camanim readiness mask,
  expected `0x01`.
- `gNdsOpeningRoomScene2CameraAnimOffset`: BattleShip
  `llMVOpeningRoomScene2CamAnimJoint` offset, expected `0`.
- `gNdsOpeningRoomScene2CameraEjectResult` / `EjectMask`: original Scene 1
  camera ejection marker and readiness mask, expected `0x4F523245` / `0x07`.
- `gNdsOpeningRoomScene2CameraCreateResult` / `CreateMask`: original Scene 2
  camera creation marker and readiness mask, expected `0x4F523243` / `0x1FF`.
- `gNdsOpeningRoomScene2CameraGObjDelta` / `CObjDelta` / `XObjDelta` /
  `AObjDelta`: object-manager deltas after Scene 1 camera ejection, expected
  `2`, `2`, `4`, and `0`.
- `gNdsOpeningRoomScene2CameraDisplaySet`, `ProcessSet`, `AnimSet`,
  `ViewportSet`, `DLBufferSet`: Scene 2 camera link checks, expected `1`.
- `gNdsOpeningRoomUpdateResult`: first imported actor update, expected
  `0x4F525550` (`ORUP`).
- `gNdsOpeningRoomTickCount`: actor time at the normal no-input boundary,
  expected `560`.
- `gNdsOpeningRoomPreAssetResult`: tick-279 pre-event marker, expected
  `0x4F525041` (`ORPA`).
- `gNdsOpeningRoomControllerCheckCount`: original shared controller gate calls
  on ticks 10-560, expected `551`.
- `gNdsOpeningRoomPulledFighterKind` / `DroppedFighterKind`: distinct members
  of the original `{Mario,Fox,Donkey,Samus,Link,Yoshi,Kirby,Pikachu}` set.
- `gNdsOpeningRoomSkipToTitleCount`: `0` in normal verification and `1` in the
  skip-path verifier.
- `gNdsOpeningRoomGObjCount` / `gNdsOpeningRoomCameraCount`: post-`func_start`
  original object-manager counts, expected `13` / `6`.
- Opening Room task setup diagnostics preserve DL sizes `12000` / `4096`,
  graphics heap `32768`, and RDP output buffer `49152`.
- `gNdsOpeningRoomDrawResult`: bounded Opening Room draw marker, expected
  `0x4F524457` (`ORDW`).
- `gNdsOpeningRoomDrawBlocker`: current full-renderer blocker reached by the
  bounded Opening Room draw path, expected `3` for general DObj display-list
  translation. The separate preview path is tracked by
  `gNdsOpeningRoomDLPreviewBlocker`.
- `gNdsOpeningRoomDrawTickCount`: actor tick when the latest actual draw probe
  ran, expected `1320` for the final Opening Room handoff draw.
- `gNdsOpeningRoomDrawFrameCount`: taskman frame counter when the draw probe
  ran, expected `1`; this proves the probe is still bounded and not the
  continuous draw loop.
- `gNdsOpeningRoomDrawProbeCount`: number of actual original `scene_draw`
  probes run for the retained Opening Room preview, expected `2`.
- `gNdsOpeningRoomDrawReuseCount`: number of repeated movie presentations that
  reused the retained Opening Room preview instead of rerunning `gcDrawAll`;
  the 45-second speed sample currently reports `24`.
- `gNdsOpeningRoomDrawCameraCallbackCount`: original `func_80017EC0` camera
  display callbacks reached by the probes, expected greater than `0`.
- `gNdsOpeningRoomDrawDisplayCallbackCount`: camera-captured display callbacks
  reached by the probes, expected greater than `0`.
- `gNdsOpeningRoomDrawDObjCallbackCount`: DObj display callbacks reached by the
  probes, expected greater than `0`.
- `gNdsOpeningRoomDrawFirstCameraMaskLow` / `FirstCameraPriority`: first camera
  evidence, currently `0x40` / `80`.
- `gNdsOpeningRoomDrawFirstCameraFlags`: active first draw camera CObj flags,
  currently `0x4` (`COBJ_FLAG_DLBUFFERS`) for the Scene 2 main camera.
- `gNdsOpeningRoomDrawFirstCameraXObjCount` / `XObjKind0` / `XObjKind1`:
  active first draw camera matrix evidence, currently `2,3,8`
  (`nGCMatrixKindPerspFastF` plus the Scene 2 camera-vector kind).
- `gNdsOpeningRoomDrawFirstCameraViewportScaleX` /
  `ViewportScaleY` / `ViewportTransX` / `ViewportTransY`: original viewport
  values sampled from the active draw CObj, expected `600,440,640,480`.
- `gNdsRdpDefaultViewportSetCount`: number of original default viewport
  initializations performed by the DS RDP shim, expected greater than `0`.
- `gNdsRdpDefaultViewportScaleX` / `ScaleY` / `TransX` / `TransY` /
  `ScaleZ` / `TransZ`: last default viewport values written by
  `syRdpSetDefaultViewport`, expected `640,480,640,480,511,511` for the
  current 320x240 BattleShip video setup.
- `gNdsOpeningRoomDrawFirstCameraNear100` / `Far100` / `FovY100`: active
  perspective values in centi-units. The verifier requires positive near/fovy
  and `far > near` because the bounded DObj preview now draws through this
  active original camera state.
- `gNdsOpeningRoomDrawFirstCameraEyeX100` / `EyeY100` / `EyeZ100` and
  `AtX100` / `AtY100` / `AtZ100`: active CObj eye/at vectors in centi-units.
  These are sampled as the source for the bounded camera-projected preview.
- `gNdsOpeningRoomDrawFirstObjectDLLink` / `FirstObjectID` /
  `FirstObjectKind`: first captured display object evidence, currently display
  link `6`, object ID `0`, object kind `1` (`nGCCommonKindDObj`).
- `gNdsOpeningRoomDrawFirstCallback`: first DObj display callback marker,
  currently `0x444C4E4B` (`gcDrawDObjDLLinksForGObj`).
- `gNdsOpeningRoomDrawFirstDObjDL`: first DObj display-list pointer, expected
  nonzero.
- `gNdsOpeningRoomDrawFirstDObjMeta`: DObj readiness bits, currently `0x11`
  for display-list pointer plus XObj.
- `gNdsOpeningRoomDrawMaterialCandidateResult`: marker proving the bounded
  imported draw traversal found a material-bearing DObj candidate, expected
  `0x4F524D43` (`ORMC`) once melonDS/GDB verification is available.
- `gNdsOpeningRoomDrawMaterialCandidateCount`: number of material-bearing DObj
  candidates observed by the draw probe. The verifier requires at least `1`.
- `gNdsOpeningRoomDrawMaterialCandidateCameraMaskLow` /
  `CameraPriority`: camera evidence for the first material-bearing candidate.
  The verifier checks for a nonzero mask and a real priority.
- `gNdsOpeningRoomDrawMaterialCandidateObjectDLLink` /
  `ObjectID` / `ObjectKind`: display object evidence for the first
  material-bearing candidate. Object kind must be `1` (`DObj`).
- `gNdsOpeningRoomDrawMaterialCandidateCallback`: original display callback
  marker for the material-bearing candidate. Valid values are the existing DObj
  draw callbacks; this is callback-aware so `dobj->dl` and `dobj->dl_link`
  paths are not conflated.
- `gNdsOpeningRoomDrawMaterialCandidateDObjDL` /
  `DObjMeta`: selected material-bearing candidate display-list pointer and
  DObj readiness bits. The verifier checks for a nonzero DL pointer and meta
  bits proving both display-list and MObj are present.
- `gNdsOpeningRoomDrawMaterialCandidateMObjCount`: first material-bearing
  candidate MObj chain count. The verifier requires at least `1`.
- `gNdsOpeningRoomDrawMaterialCandidateMObjFlags` /
  `MObjEffectiveFlags` / `MObjMask`: raw first-MObj flags, original fallback
  flags after the `gcDrawMObjForDObj` material contract, and a presence mask.
  The current verifier requires raw flags `0x200` for the corrected first
  MVCommon material candidate, DObj/MObj presence, and non-sentinel effective
  flags; individual texture/tile/load-block branch checks are conditional on
  the recorded material mask.
- `gNdsOpeningRoomDrawMaterialCandidateMObjTextureCurr` /
  `TextureNext` / `PaletteIndex` / `Lfrac100`: current/next texture IDs,
  palette index, and interpolation fraction for the first candidate MObj.
- `gNdsOpeningRoomDrawMaterialCandidateMObjFormat` / `Size` /
  `BlockFormat` / `BlockSize`: texture format metadata resolved from the first
  candidate MObj. The verifier checks that the primary format/size are not the
  reset sentinel.
- `gNdsOpeningRoomDrawMaterialCandidateMObjTileWidth` / `TileHeight` /
  `ScrollWidth` / `ScrollHeight`: texture tile and scroll dimensions recorded
  before any renderer branch is emitted.
- `gNdsOpeningRoomDrawMaterialCandidateMObjScaleS100` / `ScaleT100` /
  `TranslateS100` / `TranslateT100`: S/T scale and translation sampled from the
  first candidate MObj in centi-units.
- `gNdsOpeningRoomDrawMaterialCandidateMObjSpriteArray` /
  `PaletteArray`: raw first-MObj sprite/palette array pointers. These remain
  useful source-side diagnostics, but the corrected current first material
  candidate is prim-color only and does not emit a material `SETTIMG` command.
- `gNdsOpeningRoomDrawMaterialCandidateMObjSpriteCurr` / `SpriteNext` /
  `PalettePtr`: safe pointer evidence for the first candidate MObj's sprite and
  palette records. These are diagnostics only; they do not mean texture upload
  or material branch rendering exists.
- `gNdsOpeningRoomDrawTextureMaterialResult`: marker for the first
  texture-capable material source found by the bounded imported draw traversal.
  It remains `0` for the current verified slice; the pass marker is
  `0x4F525458` (`ORTX`) when an effective `MOBJ_FLAG_TEXTURE` material is found.
- `gNdsOpeningRoomDrawTextureMaterialCandidateCount` /
  `MObjCount` / `SpriteArrayCount` / `SpriteCurrCount` / `SpriteNextCount`:
  source-side texture-material scan counts. Current expected values are all
  `0`, proving the selected bounded material path exposes no texture-bearing
  `MObj` source yet.
- `gNdsOpeningRoomDrawTextureMaterialObjectDLLink` / `ObjectID` /
  `ObjectKind` / `Callback`, `DObjDL` / `DObjMeta`, `MObjFlags` /
  `MObjEffectiveFlags` / `MObjMask`, `MObjTextureCurr` / `MObjTextureNext`, and
  `MObjSpriteArray` / `MObjSpriteCurr` / `MObjSpriteNext`: captured evidence for
  the first future `ORTX` candidate. Current reset/sentinel values are expected
  while `ORTX` result is `0`.
- `gNdsOpeningRoomDrawMaterialBranchResult`: marker proving the DS backend
  mirrored the original `gcDrawMObjForDObj` branch-list command-family
  decisions for the first material-bearing candidate, expected `0x4F524D42`
  (`ORMB`).
- `gNdsOpeningRoomDrawMaterialBranchMObjCount`: MObj count used for the original
  branch table. The verifier requires it to match the candidate MObj count;
  current value is `2`.
- `gNdsOpeningRoomDrawMaterialBranchSegmentCommands`: original
  `gSPSegment(..., 0xE, gSYTaskmanGraphicsHeap.ptr)` command count. Expected
  `1`.
- `gNdsOpeningRoomDrawMaterialBranchTableCommands`: original
  `gSPBranchList(&new_dl[i], branch_dl)` table command count. Expected to match
  the MObj count; current value is `2`.
- `gNdsOpeningRoomDrawMaterialBranchGeneratedCommands`: total commands that the
  branch stream would generate after the table, excluding actual `Gfx`
  allocation. Current value is `4`.
- `gNdsOpeningRoomDrawMaterialBranchFirstMask`: command-family mask for the
  first MObj branch. The verifier requires segment/table/end evidence and
  conditionally checks texture/tile/scroll/load-block families when the material
  mask says they are present. Current value is `0x4023`, proving the corrected
  prim-color-only source material shape.
- `gNdsOpeningRoomDrawMaterialBranchFirstGeneratedCommands`: first-MObj branch
  stream command count. Current value is `2`.
- `gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleS` /
  `FirstTextureScaleT`: original `gSPTexture` scale values when the first MObj
  emits a texture command. Current candidate first-MObj does not emit texture,
  so both are `0`.
- `gNdsOpeningRoomDrawMaterialBranchFirstTileUls` / `FirstTileUlt` /
  `FirstTileLrs` / `FirstTileLrt`: original `gDPSetTileSize` values when the
  first MObj emits tile-size setup. Current values are all `0`.
- `gNdsOpeningRoomDrawMaterialBranchFirstScrollUls` / `FirstScrollUlt` /
  `FirstScrollLrs` / `FirstScrollLrt`: original scroll tile-size values when
  emitted. Current values are all `0`.
- `gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockTexels` /
  `FirstLoadBlockDxt`: original `gDPLoadBlock` texel/DXT values when the first
  MObj emits a load-block path. Current values are `0,0`.
- `gNdsOpeningRoomDrawMaterialEmitResult`: marker proving the DS backend
  emitted a detached original-shaped material branch table/stream into
  `gSYTaskmanGraphicsHeap`, expected `0x4F524D45` (`ORME`).
- `gNdsOpeningRoomDrawMaterialEmitBlocker`: blocker enum for the detached
  material emission path. Expected `0`.
- `gNdsOpeningRoomDrawMaterialEmitUnsupportedMask`: command-family mask that
  still prevents detached emission. Expected `0` for the current candidate.
- `gNdsOpeningRoomDrawMaterialEmitMObjCount` /
  `TableCommands` / `GeneratedCommands`: emitted MObj/table/branch-stream
  counts. Current values are `2`, `2`, and `4`.
- `gNdsOpeningRoomDrawMaterialEmitHeapStart` /
  `BranchStart` / `HeapAfter` / `HeapBytes`: graphics heap allocation evidence.
  Current byte count is `48`.
- `gNdsOpeningRoomDrawMaterialEmitFirstTableOp`: first table opcode, expected
  `0xDE` (`G_DL`/branch-list).
- `gNdsOpeningRoomDrawMaterialEmitFirstBranchOp0` /
  `FirstBranchOp1` / `FirstBranchOp2`: first emitted branch opcodes, expected
  `0xFA`, `0xDF`, `0x00`.
- `gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_*` /
  `FirstBranchW1_*`: raw first-branch command words. The current first command
  is `G_SETPRIMCOLOR`, followed by `G_ENDDL`; the visible bounded preview now
  consumes this emitted branch state for the selected material candidate.
- `gNdsOpeningRoomDLPreviewMaterialBranchResult`: marker proving the bounded
  DL preview consumed the detached material branch table and both generated
  branch streams as preview state, expected `0x4F524D50` (`ORMP`).
- `gNdsOpeningRoomDLPreviewMaterialBranchBlocker`: preview-state blocker enum,
  expected `0`.
- `gNdsOpeningRoomDLPreviewMaterialBranchCommandCount` /
  `PrimCount` / `EndCount`: parsed detached branch-table stream counts,
  expected `4`, `2`, and `2`.
- `gNdsOpeningRoomDLPreviewMaterialBranchFirstOp` /
  `SecondOp` / `UnsupportedOp`: first parsed material branch opcodes, expected
  `0xFA`, `0xDF`, and `0`.
- `gNdsOpeningRoomDLPreviewMaterialBranchPrimColor` /
  `PrimLod` / `PrimM`: first parsed `G_SETPRIMCOLOR` state. The verifier
  requires the prim color to match the detached `ORME` branch and the current
  `lod/m` fields to be `0,0`.
- `gNdsOpeningRoomMaterialDLProbeResult`: marker for the material-bearing
  candidate's own display-list shape probe. It remains `0` until the candidate
  list can be consumed without unsupported commands; the pass marker is
  `0x4F524D44` (`ORMD`).
- `gNdsOpeningRoomMaterialDLProbeBlocker`: blocker enum for that non-visual
  material-candidate DL probe. Current expected value is `3`
  (`UNSUPPORTED`), proving the probe reached a real list but stopped at a
  command the bounded renderer does not execute yet.
- `gNdsOpeningRoomMaterialDLProbeFirstDL`: nonzero original display-list
  pointer selected from the first material-bearing DObj candidate.
- `gNdsOpeningRoomMaterialDLProbeCommandCount` /
  `VertexCount` / `TriangleCount`: current list shape is 29 commands, four
  vertices, and four triangles.
- `gNdsOpeningRoomMaterialDLProbeFirstOpcode` /
  `UnsupportedOpcode`: current first opcode is `0xE7`; first unsupported opcode
  is `0xDE` (`G_DL`), making nested display-list branch handling the next
  renderer blocker for this material candidate.
- `gNdsOpeningRoomMaterialDLProbeVertexCommandCount` /
  `TriangleCommandCount` / `SyncCommandCount` / `EndCommandCount` /
  `BranchCommandCount` / `OtherModeCommandCount` /
  `UnsupportedCommandCount`: current command-family counts are `2`, `2`, `7`,
  `1`, `2`, `0`, and `2`.
- `gNdsOpeningRoomMaterialDLExpandResult`: marker proving the bounded
  branch-expanded material-candidate display-list probe passed through
  `src/nds/nds_renderer.c`, expected `0x4F524D58` (`ORMX`).
- `gNdsOpeningRoomMaterialDLExpandBlocker`: blocker enum for the expanded
  probe, expected `0`.
- `gNdsOpeningRoomMaterialDLExpandFirstDL`: same first material-candidate
  display-list pointer sampled by `ORMD`, expected nonzero.
- `gNdsOpeningRoomMaterialDLExpandFirstBranchDL`: first raw branch target seen
  while expanding the material-candidate list. Current expected value is
  `0x0E000000`, matching the original segment-`E` material branch table
  contract from `gcDrawMObjForDObj`.
- `gNdsOpeningRoomMaterialDLExpandFirstResolvedBranchDL`: native pointer after
  resolving the first segment-`E` branch through the emitted `ORME`
  `gSYTaskmanGraphicsHeap` table, expected nonzero.
- `gNdsOpeningRoomMaterialDLExpandCommandCount` /
  `VertexCount` / `TriangleCount` / `UnsupportedCommandCount`: expanded shape,
  expected `42`, `4`, `4`, and `0`.
- `gNdsOpeningRoomMaterialDLExpandFirstOpcode` /
  `UnsupportedOpcode`: expanded first opcode and first unsupported opcode,
  expected `0xE7` and `0`.
- `gNdsOpeningRoomMaterialDLExpandVertexCommandCount` /
  `TriangleCommandCount` / `SyncCommandCount` / `EndCommandCount` /
  `BranchCommandCount` / `BranchCallCount` / `BranchJumpCount` /
  `SegmentResolveCount` / `OtherModeCommandCount`: current command-family
  counts are `2`, `2`, `7`, `6`, `5`, `5`, `0`, `2`, and `0`.
- `gNdsOpeningRoomMaterialDLExpandColorCommandCount` /
  `MaxDepth`: current values are `5` color-state commands and branch depth `2`.
- `gNdsOpeningRoomDLPreviewResult`: bounded Opening Room DObj display-list
  preview marker, expected `0x4F524450` (`ORDP`). The visible preview now
  receives commands from `ndsRendererExecuteDisplayList`; the scene bridge
  still owns the Smash-specific vertex/texture/camera decode.
- `gNdsOpeningRoomDLPreviewBlocker`: preview blocker enum, expected `0`.
- `gNdsOpeningRoomDLPreviewCommandCount`: parsed first-DL command count,
  expected `42` for the branch-expanded material candidate.
- `gNdsOpeningRoomDLPreviewVertexCount`: decoded vertex count, expected at
  `4`.
- `gNdsOpeningRoomDLPreviewTriangleCount`: rasterized triangle count, expected
  `4`.
- `gNdsOpeningRoomDLPreviewPixelCount`: written preview pixels, expected
  greater than `0`; exact count can change as the bounded diagnostic
  presentation is tightened.
- `gNdsOpeningRoomDLPreviewFirstOpcode`: first command opcode, expected
  `0xE7`.
- `gNdsOpeningRoomDLPreviewUnsupportedOpcode`: unsupported command seen by the
  preview interpreter, expected `0`.
- `gNdsOpeningRoomDLPreviewVertexCommandCount`: number of `G_VTX` commands in
  the parsed branch-expanded list, expected `2`.
- `gNdsOpeningRoomDLPreviewTriangleCommandCount`: number of `G_TRI1`/`G_TRI2`
  commands in the parsed branch-expanded list, expected `2`.
- `gNdsOpeningRoomDLPreviewSyncCommandCount`: number of RDP sync commands in
  the parsed branch-expanded list, expected `7`.
- `gNdsOpeningRoomDLPreviewEndCommandCount`: number of `G_ENDDL` commands in
  the parsed branch-expanded list, expected `6`.
- `gNdsOpeningRoomDLPreviewBranchCommandCount`: number of `G_DL` or
  `G_CULLDL` commands seen by the bounded preview, expected `5`.
- `gNdsOpeningRoomDLPreviewBranchCallCount` /
  `BranchJumpCount` / `SegmentResolveCount`: branch execution shape for the
  selected material candidate, expected `5`, `0`, and `2`.
- `gNdsOpeningRoomDLPreviewColorCommandCount` /
  `PrimColor`: material color-state evidence from the branch-expanded stream,
  expected `5` and `0xFFFFFFFF`.
- `gNdsOpeningRoomDLPreviewOtherModeCommandCount`: number of `G_SETOTHERMODE`
  or raw RDP set-othermode commands seen by the bounded preview, expected `0`.
- `gNdsOpeningRoomDLPreviewUnsupportedCommandCount`: total unsupported command
  count seen by the bounded preview, expected `0`.
- `gNdsOpeningRoomDLPreviewRendererTextureMask` /
  `TextureImage` / `TextureFormat` / `TextureSize` /
  `TextureImageWidth`: the renderer adapter's own `G_SETCOMBINE`,
  `G_SETTIMG`, `G_SETTILE`, `G_SETTILESIZE`, `G_TEXTURE`, and `G_LOADBLOCK`
  state decode for the visible `ORDP` path. Current expected values are mask
  `0x3F`, nonzero image pointer, format `4`, size `2`, and image width field
  `1`.
- `gNdsOpeningRoomDLPreviewRendererTextureLoadTexels` /
  `TextureSetTileCount` / `TextureCommandCount`: adapter-owned load/tile/
  texture-command counts, currently `256`, `4`, and `1`.
- `gNdsOpeningRoomDLPreviewRendererTextureStateFlags`: adapter-owned
  `G_TEXTURE` state flags, currently `0x0F`.
- `gNdsOpeningRoomDLPreviewRendererTextureTileWidth` /
  `TextureTileHeight` / `TextureRenderTile` / `TextureRenderTileLine` /
  `TextureRenderTileFlags`: adapter-owned tile decode for the bounded
  preview, currently `16`, `32`, render tile `0`, line `4`, and flags
  `0xB7`.
- `gNdsOpeningRoomDLPreviewRendererTextureLoadBlockLrs` /
  `TextureLoadBlockDxt`: adapter-owned first load-block fields, currently
  `255` and `1024`.
- `gNdsOpeningRoomDLPreviewFirstDL`: first parsed `DObjDLLink->dl` pointer,
  expected nonzero.
- `gNdsOpeningRoomDLPreviewTransformMask`: sampled original DObj/XObj
  transform evidence, expected `0x1F` for DObj, translate, rotate, scale, and
  XObj.
- `gNdsOpeningRoomDLPreviewXObjCount`: first previewed DObj XObj count,
  expected at least `1`.
- `gNdsOpeningRoomDLPreviewFirstXObjKind`: first previewed XObj kind, expected
  `28` (`nGCMatrixKindTraRotRpyRSca`).
- `gNdsOpeningRoomDLPreviewTranslateX100` / `TranslateY100` /
  `TranslateZ100`: sampled DObj translation in centi-units.
- `gNdsOpeningRoomDLPreviewRotateX100` / `RotateY100` / `RotateZ100`: sampled
  DObj RPY rotation in centi-radians, currently `0,0,0` for the first previewed
  object.
- `gNdsOpeningRoomDLPreviewScaleX100` / `ScaleY100` / `ScaleZ100`: sampled
  DObj scale in centi-units, expected `117,100,117` for the selected
  material candidate.
- `gNdsOpeningRoomDLPreviewMinX` / `MaxX` / `MinY` / `MaxY`: transformed
  preview bounds; the verifier requires nonzero width and height.
- `gNdsOpeningRoomDLPreviewProjectionMask`: bounded camera-projection evidence,
  expected `0x1F` for active camera, viewport, perspective, look-at, and
  projected vertices. The current selected material primitive is too small for
  the earlier camera-projected triangle path and uses the bounded retained
  preview fallback for visible pixels.
- `gNdsOpeningRoomDLPreviewProjectionMode`: projection draw mode, expected `2`
  for the current material-candidate preview.
- `gNdsOpeningRoomDLPreviewProjectionBlocker`: projection blocker enum,
  expected `6`.
- `gNdsOpeningRoomDLPreviewProjectedVertexCount` /
  `ProjectedTriangleCount`: camera-projected slice evidence, currently `4/0`.
- `gNdsOpeningRoomDLPreviewProjectedMinX` / `ProjectedMaxX` /
  `ProjectedMinY` / `ProjectedMaxY`: projected preview-space bounds,
  currently `3,31,-23,-14`.
- `gNdsOpeningRoomDLPreviewProjectedMinDepth100` /
  `ProjectedMaxDepth100`: projected camera depth range in centi-units,
  currently `470900..565500`.
- `gNdsOpeningRoomDLPreviewGeometryCommandCount`: number of
  `G_GEOMETRYMODE` commands consumed from the first bounded list, currently
  `2`.
- `gNdsOpeningRoomDLPreviewGeometryClearMask` /
  `GeometrySetMask`: last raw geometry-mode command masks, currently
  `0xD9FFFFFF` and `0x20000`.
- `gNdsOpeningRoomDLPreviewGeometryFinalMode`: final N64 geometry-mode state
  for the selected bounded list, currently `0x20000`.
- `gNdsOpeningRoomDLPreviewGeometryFlags`: DS diagnostic decode of the final
  geometry-mode state, currently `0x21`.
- `gNdsOpeningRoomDLPreviewGeometryPositiveWinding` /
  `GeometryNegativeWinding` / `GeometryZeroArea` /
  `GeometryDrawnTriangles`: projected triangle winding evidence for the
  drawn slice, currently `0/0/4/4`.
- `gNdsOpeningRoomDLPreviewTextureMask`: first preview DL texture setup
  evidence, expected `0x3F` for set-combine, tile, texture enable, tile-size,
  texture-image, and load-block commands.
- `gNdsOpeningRoomDLPreviewTextureImage`: first `G_SETTIMG` image pointer,
  expected nonzero.
- `gNdsOpeningRoomDLPreviewTextureFormat` / `TextureSize`: first texture image
  format/size, currently `4`/`2` for this material-candidate command state.
- `gNdsOpeningRoomDLPreviewTextureImageWidth`: first texture-image width field,
  currently `1` because this list uses a load-block image setup.
- `gNdsOpeningRoomDLPreviewTextureTileWidth` /
  `TextureTileHeight`: decoded first render tile size, expected `16x32`.
- `gNdsOpeningRoomDLPreviewTextureLoadTexels`: decoded first load-block texel
  count, expected `256`.
- `gNdsOpeningRoomDLPreviewTextureLoadBlockTile` /
  `TextureLoadBlockUls` / `TextureLoadBlockUlt` /
  `TextureLoadBlockLrs` / `TextureLoadBlockDxt`: raw first `G_LOADBLOCK`
  command fields, currently `7,0,0,255,1024`.
- `gNdsOpeningRoomDLPreviewTextureTileSizeTile` /
  `TextureTileSizeUls` / `TextureTileSizeUlt` /
  `TextureTileSizeLrs` / `TextureTileSizeLrt`: raw first
  `G_SETTILESIZE` command fields, currently `0,0,0,60,124`.
- `gNdsOpeningRoomDLPreviewTextureTexelWidth` /
  `TextureTexelHeight`: derived physical texture layout for this bounded
  material-candidate preview, currently `0x0` because no resolved material
  texture source is uploaded yet.
- `gNdsOpeningRoomDLPreviewTextureSamplePixels`: number of preview pixel writes
  that sampled the bounded original texture, expected `0` for this
  prim-color-only material candidate.
- `gNdsOpeningRoomDLPreviewTextureSetTileCount`: number of `G_SETTILE`
  commands in the first parsed list, expected `4`.
- `gNdsOpeningRoomDLPreviewTextureCombineW0` /
  `TextureCombineW1`: first set-combine command words, expected
  `0xFC6F96DF,0xFF2E7F3F`.
- `gNdsOpeningRoomDLPreviewTextureCombineMode`: decoded bounded combiner mode,
  expected `0` for the current material-candidate command state.
- `gNdsOpeningRoomDLPreviewTextureCombineFlags`: decoded bounded combiner
  evidence, expected `0x1` for set-combine seen.
- `gNdsOpeningRoomDLPreviewTextureModulatedPixels`: number of preview pixel
  writes that applied decoded shade modulation to sampled texture texels,
  expected `0` for this material-candidate preview.
- `gNdsOpeningRoomDLPreviewTextureRenderTile`: first render tile selected by
  `G_SETTILE`, currently `0`.
- `gNdsOpeningRoomDLPreviewTextureRenderTileLine` /
  `TextureRenderTileTmem` / `TextureRenderTilePalette`: decoded render-tile
  line/TMEM/palette fields, currently `4/0/0`.
- `gNdsOpeningRoomDLPreviewTextureRenderTileCms` /
  `TextureRenderTileCmt`: decoded S/T address mode bits, currently `2/2`.
- `gNdsOpeningRoomDLPreviewTextureRenderTileMasks` /
  `TextureRenderTileMaskt`: decoded S/T mask widths, currently `5/5`.
- `gNdsOpeningRoomDLPreviewTextureRenderTileShifts` /
  `TextureRenderTileShiftt`: decoded S/T shifts, currently `0/0`.
- `gNdsOpeningRoomDLPreviewTextureRenderTileFlags`: DS diagnostic decode of
  the render/load tile state, currently `0xB7` for render seen, load seen,
  S/T clamped, and S/T masked.
- `gNdsOpeningRoomDLPreviewTextureCommandCount`: number of `G_TEXTURE`
  commands consumed from the first parsed list, currently `1`.
- `gNdsOpeningRoomDLPreviewTextureScaleS` /
  `TextureScaleT`: decoded `G_TEXTURE` S/T scale values, currently
  `65535/65535`.
- `gNdsOpeningRoomDLPreviewTextureLevel` /
  `TextureTile` / `TextureOn` / `TextureXParam`: decoded `G_TEXTURE` level,
  tile, on/off, and xparam fields, currently `0/0/1/0`.
- `gNdsOpeningRoomDLPreviewTextureStateFlags`: DS diagnostic decode of
  `G_TEXTURE` state, currently `0x0F` for seen, on, nonzero S scale, and
  nonzero T scale.
- `gNdsOpeningRoomDLPreviewMaterialCount`: number of original `MObj` nodes
  attached to the selected preview DObj. The verifier expects `2` for the
  selected material-bearing candidate.
- `gNdsOpeningRoomDLPreviewMaterialFlags` /
  `MaterialEffectiveFlags`: raw first-MObj flags and the effective flags
  BattleShip's original `gcDrawMObjForDObj` would use after applying its
  default `TEXTURE | 0x20 | ALPHA` fallback. Expected `0x200/0x200`.
- `gNdsOpeningRoomDLPreviewMaterialMask`: material diagnostic readiness bits.
  Expected `0x403` for the current material-bearing candidate.
- `gNdsOpeningRoomDLPreviewMaterialTextureCurr` /
  `MaterialTextureNext` / `MaterialPaletteIndex` /
  `MaterialLfrac100`: first-MObj animation/material IDs when a material chain
  exists. Expected `0,0,0,0` for the current candidate.
- `gNdsOpeningRoomDLPreviewMaterialFormat` / `MaterialSize` /
  `MaterialBlockFormat` / `MaterialBlockSize`: first-MObj texture format
  fields when a material chain exists. Expected `4,2,4,1`.
- `gNdsOpeningRoomDLPreviewMaterialTileWidth` / `MaterialTileHeight` /
  `MaterialScrollWidth` / `MaterialScrollHeight`: first-MObj tile dimensions
  when a material chain exists. Expected `16,32,16,32`.
- `gNdsOpeningRoomDLPreviewMaterialScaleS100` /
  `MaterialScaleT100` / `MaterialTranslateS100` /
  `MaterialTranslateT100`: first-MObj texture scale/translation in centi-units
  when a material chain exists. Expected `100,100,0,0`.
- `gNdsOpeningRoomDLPreviewMaterialSpriteCurr` /
  `MaterialSpriteNext` / `MaterialPalettePtr`: resolved first-MObj sprite and
  palette pointers when original material flags require those arrays. Expected
  `0` for the current first DObj.
- `gNdsOriginalSpritePreviewDisplayWidth` /
  `DisplayHeight`: retained startup logo presentation size. Expected `128`
  and `108`, proving the melonDS visual debug copy is native-size rather than
  the older downscaled DS-logical copy.
- `gNdsOriginalDLPreviewReady` / `Width` / `Height`: retained platform
  presentation state for the top-screen preview, expected `1`, `96`, and `72`.
- `gNdsOriginalDLPreviewCommitCount` / `DrawCount`: retained platform
  presentation counters; both must be at least `1` so parsing cannot pass
  without the visible preview being committed and drawn.

## Failure Triage

If build fails:

1. Identify the first missing symbol/type from the compiler output.
2. Inspect the original BattleShip source and header that define it.
3. Add the smallest compatibility declaration in `include/`.
4. Add a DS backend stub or implementation in `src/port` or `src/nds`.
5. Do not edit `decomp/`; keep hooks in project-owned wrappers/shims.
6. Do not edit generated build output.
7. Do not rewrite original gameplay or scene logic to avoid the missing symbol.

If runtime verification fails:

1. Read the full GDB output printed by the script.
2. Determine the earliest failed marker in boot order.
3. Inspect the source that should set that marker.
4. Check whether a thread is parked, a queue is empty/full, or a service stub is
   returning too early.
5. Add a targeted diagnostic global if the boundary is ambiguous.

If the emulator does not start:

1. Confirm `smash64ds.nds` and `smash64ds.elf` exist.
2. Confirm `emulators/melonds/melonDS.exe` exists or pass `-MelonDS`.
3. If melonDS stays alive but has no visible/capturable window and no ARM9 GDB
   listener on `3333`, first inspect `[Instance0.Gdb]` in
   `emulators/melonds/melonDS.toml` for duplicate `Enable` / `Enabled` keys,
   then test another known-good ROM such as devkitPro's
   `Simple_Tri.nds`. If that also starts hidden/windowless, treat it as a
   melonDS/session launch issue rather than a Smash64DS runtime regression.
4. Delete stale `emulators/melonds/melonDS.toml` only if it is local generated
   config and not a user-edited file.
5. Re-run `make clean; make -j4`.

## Useful Manual GDB Commands

After launching melonDS with GDB enabled:

```powershell
C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe .\smash64ds.elf
```

Inside GDB:

```gdb
target remote 127.0.0.1:3333
p/x gNdsOriginalBootStage
p/x gNdsSceneBoundaryResult
p gNdsStartupTaskmanSceneKind
p/x gNdsStartupLogoRelocResult
p gNdsStartupLogoRelocSize
p gNdsStartupLogoRelocWordSwapCount
p gNdsStartupLogoRelocPointerFixupCount
p/x gNdsStartupLogoDrawResult
p gNdsStartupLogoDrawBlocker
p gNdsStartupLogoDrawCallbackCount
p gNdsStartupLogoDrawUpdateCount
p gNdsStartupLogoDrawWidth
p gNdsStartupLogoDrawHeight
p gNdsStartupLogoDrawBitmaps
p gNdsStartupLogoDrawPixels
p/x gNdsTaskmanCleanupResult
p gNdsOpeningRoomDispatchCount
p/x gNdsOpeningRoomRelocResult
p/x gNdsOpeningRoomRelocFileMask
p/x gNdsOpeningRoomRelocHeaderMask
p/x gNdsOpeningRoomRelocPayloadMask
p gNdsOpeningRoomRelocBytesLoaded
p gNdsOpeningRoomRelocFixupReady
p gNdsOpeningRoomRelocSymbolResolveCount
p/x gNdsOpeningRoomRelocSymbolProbeMask
p gNdsOpeningRoomRelocMObjSubNormalizeCount
p gNdsOpeningRoomRelocMObjSubNormalizeFailCount
p/x gNdsOpeningRoomRelocMObjSubFirstFlags
p/x gNdsOpeningRoomRelocMObjSubSourceResult
p gNdsOpeningRoomRelocMObjSubTextureFlagCount
p gNdsOpeningRoomRelocMObjSubZeroFlagCount
p gNdsOpeningRoomRelocMObjSubPrimColorCount
p gNdsOpeningRoomRelocMObjSubLightCount
p/x gNdsOpeningRoomRelocMObjSubFirstTextureOffset
p/x gNdsOpeningRoomRelocMObjSubFirstTextureFlags
p/x gNdsOpeningRoomFirstEventResult
p gNdsOpeningRoomFirstEventTick
p/x gNdsOpeningRoomFirstEventProbeMask
p gNdsOpeningRoomFirstEventPencilsDObjOffset
p gNdsOpeningRoomFirstEventPencilsAnimOffset
p/x gNdsOpeningRoomFirstEventDataResult
p/x gNdsOpeningRoomFirstEventDataMask
p gNdsOpeningRoomFirstEventPencilsDObjEntries
p gNdsOpeningRoomFirstEventPencilsDLPtrs
p gNdsOpeningRoomFirstEventPencilsAnimJoints
p gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode
p/x gNdsOpeningRoomFirstEventRunResult
p/x gNdsOpeningRoomFirstEventDeferredMask
p/x gNdsOpeningRoomFighterDeferredResult
p gNdsOpeningRoomFighterDeferredKind
p/x gNdsOpeningRoomOverlayCreateResult
p/x gNdsOpeningRoomOverlayEjectResult
p/x gNdsOpeningRoomOverlayEjectUnlinkedMask
p/x gNdsOpeningRoomScene1CameraCreateResult
p/x gNdsOpeningRoomScene1CameraCreateMask
p gNdsOpeningRoomScene1CameraCreateGObjCount
p gNdsOpeningRoomScene1CameraGObjDelta
p gNdsOpeningRoomScene1CameraCObjDelta
p gNdsOpeningRoomScene1CameraXObjDelta
p gNdsOpeningRoomScene1CameraAObjDelta
p gNdsOpeningRoomScene1CameraDisplaySet
p gNdsOpeningRoomScene1CameraProcessSet
p gNdsOpeningRoomScene1CameraAnimSet
p gNdsOpeningRoomScene1CameraViewportSet
p gNdsOpeningRoomScene1CameraDLBufferSet
p/x gNdsOpeningRoomCloseUpOverlayCameraCreateResult
p/x gNdsOpeningRoomCloseUpOverlayCameraCreateMask
p gNdsOpeningRoomCloseUpOverlayCameraCreateGObjCount
p gNdsOpeningRoomCloseUpOverlayCameraGObjDelta
p gNdsOpeningRoomCloseUpOverlayCameraCObjDelta
p gNdsOpeningRoomCloseUpOverlayCameraXObjDelta
p gNdsOpeningRoomCloseUpOverlayCameraDisplaySet
p gNdsOpeningRoomCloseUpOverlayCameraViewportSet
p/x gNdsOpeningRoomLogoCameraAssetMask
p gNdsOpeningRoomLogoCameraAnimOffset
p/x gNdsOpeningRoomLogoCameraCreateResult
p/x gNdsOpeningRoomLogoCameraCreateMask
p gNdsOpeningRoomLogoCameraCreateGObjCount
p gNdsOpeningRoomLogoCameraGObjDelta
p gNdsOpeningRoomLogoCameraCObjDelta
p gNdsOpeningRoomLogoCameraXObjDelta
p gNdsOpeningRoomLogoCameraAObjDelta
p gNdsOpeningRoomLogoCameraViewportSet
p/x gNdsOpeningRoomLogoAssetMask
p gNdsOpeningRoomLogoDObjOffset
p gNdsOpeningRoomLogoMObjOffset
p gNdsOpeningRoomLogoMatAnimOffset
p/x gNdsOpeningRoomLogoCreateResult
p/x gNdsOpeningRoomLogoCreateMask
p/x gNdsOpeningRoomLogoEjectResult
p/x gNdsOpeningRoomLogoEjectUnlinkedMask
p/x gNdsOpeningRoomBossShadowAssetMask
p gNdsOpeningRoomBossShadowDisplayListOffset
p gNdsOpeningRoomBossShadowAnimOffset
p/x gNdsOpeningRoomBossShadowCreateResult
p/x gNdsOpeningRoomBossShadowCreateMask
p/x gNdsOpeningRoomBossShadowEjectResult
p/x gNdsOpeningRoomBossShadowEjectUnlinkedMask
p/x gNdsOpeningRoomPencilsCreateResult
p/x gNdsOpeningRoomPencilsCreateMask
p/x gNdsOpeningRoomTick380DeferredResult
p/x gNdsOpeningRoomTick380DeferredMask
p/x gNdsOpeningRoomTick450RunResult
p/x gNdsOpeningRoomTick450DeferredMask
p/x gNdsOpeningRoomOutsideAssetMask
p gNdsOpeningRoomOutsideDisplayListOffset
p/x gNdsOpeningRoomOutsideCreateResult
p/x gNdsOpeningRoomOutsideCreateMask
p gNdsOpeningRoomOutsideCreateGObjCount
p gNdsOpeningRoomOutsideGObjDelta
p gNdsOpeningRoomOutsideDObjDelta
p gNdsOpeningRoomOutsideXObjDelta
p gNdsOpeningRoomOutsideDisplaySet
p/x gNdsOpeningRoomHazeAssetMask
p gNdsOpeningRoomHazeDisplayListOffset
p/x gNdsOpeningRoomHazeCreateResult
p/x gNdsOpeningRoomHazeCreateMask
p gNdsOpeningRoomHazeCreateGObjCount
p gNdsOpeningRoomHazeGObjDelta
p gNdsOpeningRoomHazeDObjDelta
p gNdsOpeningRoomHazeXObjDelta
p gNdsOpeningRoomHazeDisplaySet
p/x gNdsOpeningRoomSunlightAssetMask
p gNdsOpeningRoomSunlightDisplayListOffset
p/x gNdsOpeningRoomSunlightCreateResult
p/x gNdsOpeningRoomSunlightCreateMask
p gNdsOpeningRoomSunlightCreateGObjCount
p gNdsOpeningRoomSunlightGObjDelta
p gNdsOpeningRoomSunlightDObjDelta
p gNdsOpeningRoomSunlightXObjDelta
p gNdsOpeningRoomSunlightDisplaySet
p/x gNdsOpeningRoomSunlightEjectResult
p gNdsOpeningRoomSunlightEjectBeforeGObjCount
p gNdsOpeningRoomSunlightEjectAfterGObjCount
p/x gNdsOpeningRoomSunlightEjectUnlinkedMask
p/x gNdsOpeningRoomCloseUpOverlayCreateResult
p/x gNdsOpeningRoomCloseUpOverlayCreateMask
p gNdsOpeningRoomCloseUpOverlayCreateTick
p gNdsOpeningRoomCloseUpOverlayCreateGObjCount
p gNdsOpeningRoomCloseUpOverlayGObjDelta
p gNdsOpeningRoomCloseUpOverlayDisplaySet
p gNdsOpeningRoomCloseUpOverlayAlphaInit
p/x gNdsOpeningRoomTick500RunResult
p/x gNdsOpeningRoomTick500DeferredMask
p/x gNdsOpeningRoomSpotlightAssetMask
p gNdsOpeningRoomSpotlightDisplayListOffset
p gNdsOpeningRoomSpotlightMObjOffset
p gNdsOpeningRoomSpotlightMatAnimOffset
p/x gNdsOpeningRoomSpotlightCreateResult
p/x gNdsOpeningRoomSpotlightCreateMask
p gNdsOpeningRoomSpotlightCreateTick
p gNdsOpeningRoomSpotlightCreateGObjCount
p gNdsOpeningRoomSpotlightGObjDelta
p gNdsOpeningRoomSpotlightDObjDelta
p gNdsOpeningRoomSpotlightXObjDelta
p gNdsOpeningRoomSpotlightMObjDelta
p gNdsOpeningRoomSpotlightAObjDelta
p gNdsOpeningRoomSpotlightDisplaySet
p gNdsOpeningRoomSpotlightProcessSet
p gNdsOpeningRoomSpotlightMObjSet
p gNdsOpeningRoomSpotlightMatAnimSet
p gNdsOpeningRoomSpotlightPositionSet
p/x gNdsOpeningRoomDrawResult
p gNdsOpeningRoomDrawBlocker
p gNdsOpeningRoomDrawTickCount
p gNdsOpeningRoomDrawFrameCount
p gNdsOpeningRoomDrawCameraCallbackCount
p gNdsOpeningRoomDrawDisplayCallbackCount
p gNdsOpeningRoomDrawDObjCallbackCount
p/x gNdsOpeningRoomDrawFirstCameraMaskLow
p gNdsOpeningRoomDrawFirstCameraPriority
p/x gNdsOpeningRoomDrawFirstCameraFlags
p gNdsOpeningRoomDrawFirstCameraXObjCount
p gNdsOpeningRoomDrawFirstCameraXObjKind0
p gNdsOpeningRoomDrawFirstCameraXObjKind1
p gNdsOpeningRoomDrawFirstCameraViewportScaleX
p gNdsOpeningRoomDrawFirstCameraViewportScaleY
p gNdsOpeningRoomDrawFirstCameraViewportTransX
p gNdsOpeningRoomDrawFirstCameraViewportTransY
p gNdsRdpDefaultViewportSetCount
p gNdsRdpDefaultViewportScaleX
p gNdsRdpDefaultViewportScaleY
p gNdsRdpDefaultViewportTransX
p gNdsRdpDefaultViewportTransY
p gNdsRdpDefaultViewportScaleZ
p gNdsRdpDefaultViewportTransZ
p gNdsOpeningRoomDrawFirstCameraNear100
p gNdsOpeningRoomDrawFirstCameraFar100
p gNdsOpeningRoomDrawFirstCameraFovY100
p gNdsOpeningRoomDrawFirstCameraEyeX100
p gNdsOpeningRoomDrawFirstCameraEyeY100
p gNdsOpeningRoomDrawFirstCameraEyeZ100
p gNdsOpeningRoomDrawFirstCameraAtX100
p gNdsOpeningRoomDrawFirstCameraAtY100
p gNdsOpeningRoomDrawFirstCameraAtZ100
p gNdsOpeningRoomDrawFirstObjectDLLink
p gNdsOpeningRoomDrawFirstObjectID
p gNdsOpeningRoomDrawFirstObjectKind
p/x gNdsOpeningRoomDrawFirstCallback
p/x gNdsOpeningRoomDrawFirstDObjDL
p/x gNdsOpeningRoomDrawFirstDObjMeta
p/x gNdsOpeningRoomDLPreviewResult
p gNdsOpeningRoomDLPreviewBlocker
p gNdsOpeningRoomDLPreviewCommandCount
p gNdsOpeningRoomDLPreviewVertexCount
p gNdsOpeningRoomDLPreviewTriangleCount
p gNdsOpeningRoomDLPreviewPixelCount
p/x gNdsOpeningRoomDLPreviewFirstOpcode
p/x gNdsOpeningRoomDLPreviewUnsupportedOpcode
p gNdsOpeningRoomDLPreviewUnsupportedCommandCount
p gNdsOpeningRoomDLPreviewVertexCommandCount
p gNdsOpeningRoomDLPreviewTriangleCommandCount
p gNdsOpeningRoomDLPreviewSyncCommandCount
p gNdsOpeningRoomDLPreviewEndCommandCount
p gNdsOpeningRoomDLPreviewBranchCommandCount
p gNdsOpeningRoomDLPreviewBranchCallCount
p gNdsOpeningRoomDLPreviewBranchJumpCount
p gNdsOpeningRoomDLPreviewSegmentResolveCount
p gNdsOpeningRoomDLPreviewColorCommandCount
p/x gNdsOpeningRoomDLPreviewPrimColor
p gNdsOpeningRoomDLPreviewOtherModeCommandCount
p/x gNdsOpeningRoomDLPreviewFirstDL
p/x gNdsOpeningRoomDLPreviewTransformMask
p gNdsOpeningRoomDLPreviewXObjCount
p gNdsOpeningRoomDLPreviewFirstXObjKind
p gNdsOpeningRoomDLPreviewTranslateX100
p gNdsOpeningRoomDLPreviewTranslateY100
p gNdsOpeningRoomDLPreviewTranslateZ100
p gNdsOpeningRoomDLPreviewRotateX100
p gNdsOpeningRoomDLPreviewRotateY100
p gNdsOpeningRoomDLPreviewRotateZ100
p gNdsOpeningRoomDLPreviewScaleX100
p gNdsOpeningRoomDLPreviewScaleY100
p gNdsOpeningRoomDLPreviewScaleZ100
p gNdsOpeningRoomDLPreviewMinX
p gNdsOpeningRoomDLPreviewMaxX
p gNdsOpeningRoomDLPreviewMinY
p gNdsOpeningRoomDLPreviewMaxY
p/x gNdsOpeningRoomDLPreviewProjectionMask
p gNdsOpeningRoomDLPreviewProjectionMode
p gNdsOpeningRoomDLPreviewProjectionBlocker
p gNdsOpeningRoomDLPreviewProjectedVertexCount
p gNdsOpeningRoomDLPreviewProjectedTriangleCount
p gNdsOpeningRoomDLPreviewProjectedMinX
p gNdsOpeningRoomDLPreviewProjectedMaxX
p gNdsOpeningRoomDLPreviewProjectedMinY
p gNdsOpeningRoomDLPreviewProjectedMaxY
p gNdsOpeningRoomDLPreviewProjectedMinDepth100
p gNdsOpeningRoomDLPreviewProjectedMaxDepth100
p/x gNdsOpeningRoomDLPreviewTextureMask
p/x gNdsOpeningRoomDLPreviewTextureImage
p gNdsOpeningRoomDLPreviewTextureFormat
p gNdsOpeningRoomDLPreviewTextureSize
p gNdsOpeningRoomDLPreviewTextureImageWidth
p gNdsOpeningRoomDLPreviewTextureTileWidth
p gNdsOpeningRoomDLPreviewTextureTileHeight
p gNdsOpeningRoomDLPreviewTextureLoadTexels
p gNdsOpeningRoomDLPreviewTextureLoadBlockTile
p gNdsOpeningRoomDLPreviewTextureLoadBlockUls
p gNdsOpeningRoomDLPreviewTextureLoadBlockUlt
p gNdsOpeningRoomDLPreviewTextureLoadBlockLrs
p gNdsOpeningRoomDLPreviewTextureLoadBlockDxt
p gNdsOpeningRoomDLPreviewTextureTileSizeTile
p gNdsOpeningRoomDLPreviewTextureTileSizeUls
p gNdsOpeningRoomDLPreviewTextureTileSizeUlt
p gNdsOpeningRoomDLPreviewTextureTileSizeLrs
p gNdsOpeningRoomDLPreviewTextureTileSizeLrt
p gNdsOpeningRoomDLPreviewTextureTexelWidth
p gNdsOpeningRoomDLPreviewTextureTexelHeight
p gNdsOpeningRoomDLPreviewTextureSamplePixels
p gNdsOpeningRoomDLPreviewTextureSetTileCount
p/x gNdsOpeningRoomDLPreviewTextureCombineW0
p/x gNdsOpeningRoomDLPreviewTextureCombineW1
p gNdsOpeningRoomDLPreviewTextureRenderTile
p gNdsOpeningRoomDLPreviewTextureRenderTileLine
p gNdsOpeningRoomDLPreviewTextureRenderTileTmem
p gNdsOpeningRoomDLPreviewTextureRenderTilePalette
p gNdsOpeningRoomDLPreviewTextureRenderTileCms
p gNdsOpeningRoomDLPreviewTextureRenderTileCmt
p gNdsOpeningRoomDLPreviewTextureRenderTileMasks
p gNdsOpeningRoomDLPreviewTextureRenderTileMaskt
p gNdsOpeningRoomDLPreviewTextureRenderTileShifts
p gNdsOpeningRoomDLPreviewTextureRenderTileShiftt
p/x gNdsOpeningRoomDLPreviewTextureRenderTileFlags
p gNdsOpeningRoomDLPreviewTextureCommandCount
p gNdsOpeningRoomDLPreviewTextureScaleS
p gNdsOpeningRoomDLPreviewTextureScaleT
p gNdsOpeningRoomDLPreviewTextureLevel
p gNdsOpeningRoomDLPreviewTextureTile
p gNdsOpeningRoomDLPreviewTextureOn
p gNdsOpeningRoomDLPreviewTextureXParam
p/x gNdsOpeningRoomDLPreviewTextureStateFlags
p/x gNdsOpeningRoomDLPreviewRendererTextureMask
p/x gNdsOpeningRoomDLPreviewRendererTextureImage
p gNdsOpeningRoomDLPreviewRendererTextureFormat
p gNdsOpeningRoomDLPreviewRendererTextureSize
p gNdsOpeningRoomDLPreviewRendererTextureImageWidth
p gNdsOpeningRoomDLPreviewRendererTextureLoadTexels
p gNdsOpeningRoomDLPreviewRendererTextureSetTileCount
p gNdsOpeningRoomDLPreviewRendererTextureCommandCount
p/x gNdsOpeningRoomDLPreviewRendererTextureStateFlags
p gNdsOpeningRoomDLPreviewRendererTextureTileWidth
p gNdsOpeningRoomDLPreviewRendererTextureTileHeight
p gNdsOpeningRoomDLPreviewRendererTextureRenderTile
p gNdsOpeningRoomDLPreviewRendererTextureRenderTileLine
p/x gNdsOpeningRoomDLPreviewRendererTextureRenderTileFlags
p gNdsOpeningRoomDLPreviewRendererTextureLoadBlockLrs
p gNdsOpeningRoomDLPreviewRendererTextureLoadBlockDxt
p gNdsOriginalDLPreviewReady
p gNdsOriginalDLPreviewWidth
p gNdsOriginalDLPreviewHeight
p gNdsOriginalSpritePreviewCommitCount
p gNdsOriginalDLPreviewCommitCount
p gNdsOriginalDLPreviewDrawCount
p gNdsPerfPresentFps
p gNdsPerfLogicFps
p gNdsPerfDLDrawFps
p gNdsPerfPreviewCommitFps
p gNdsPerfPreviewCommitCount
p gNdsPerfSampleCount
p gNdsPerfSampleWindowTicks
p/x gNdsOpeningRoomUpdateResult
p gNdsOpeningRoomTickCount
p/x gNdsOpeningRoomPreAssetResult
p gNdsOpeningRoomControllerCheckCount
p sSYSchedulerTicCount
p gSYControllerMain
detach
quit
```

Prefer adding checks to `scripts/verify-runtime.ps1` once a diagnostic becomes
part of the expected port contract.

## Debugging Principle

A passing probe proves only the boundary it checks. Do not use a narrow marker
as proof of a broad milestone. The startup object and taskman diagnostics now
prove imported `sys/taskman.c`, `sys/objman.c`, and `sys/objhelper.c` created
the initial actor/camera/wallpaper/logo relationships, startup heap/buffer
setup, bounded `gcRunAll` updates through the startup Opening Room request, and
the original taskman break/eject and cleanup paths. They also prove the original
scene-manager switch dispatched `mvOpeningRoomStartScene`, and the imported
Opening Room relocation setup, file-ID list, NitroFS O2R payload copying,
blanket word byte-swap, internal pointer-chain relocation, selected
symbol-offset probes, first tick-280 pencils asset-reference probes,
Scene 1/logo-camera camanim probes, logo, Outside, Haze, and sunlight asset-reference probes,
first pencils descriptor/animation table shape, tick-280 deferred fighter marker, original
`mvOpeningRoomMakeScene1Cameras`, original
`mvOpeningRoomMakeCloseUpOverlayCamera`, original
`mvOpeningRoomMakeWallpaperCamera`, original
`mvOpeningRoomMakeLogoCamera`, original `mvOpeningRoomMakeLogo`, original
`mvOpeningRoomMakePencils` object creation from inside `mvOpeningRoomFuncRun`,
original Scene 1 cameras, close-up overlay camera, wallpaper-camera,
logo-camera, logo, overlay, boss-shadow, Outside, Haze, and sunlight setup/ejection, and the
actor/camera/Scene1-camera/close-up-camera/wallpaper-camera/logo-camera/overlay/logo/boss-shadow/Outside/Haze/sunlight
slice reached and passed the tick-280 pencils object boundary. They also prove
the tick-380 fighter-status/rotation branch is explicitly deferred, the
tick-450 close-up overlay object is created through original object-manager
state and real sunlight ejection leaves original lists, the tick-500 spotlight
object is created through original object-manager/material-animation state
while pulled-fighter display-link movement is explicitly deferred, and the
tick-560 Scene 2 camera transition runs while Boss fighter status is explicitly
deferred. The bounded Opening Room draw marker proves the next renderer
boundary reaches original `gcDrawAll`, original `func_80017EC0`, the narrow DS
camera capture bridge, original DObj display callbacks, and the first
material-bearing DObj candidate, then proves that candidate's branch-expanded
display list can be parsed and rasterized by a deliberately narrow DS preview
path.
The skip verifier additionally proves imported
shared controller logic and the original Title transition. They do not prove
external relocation dependency recursion, texture/display-list fixups,
fighter/effect object initialization, pulled-fighter execution, remaining
room-object events, later camera/fighter-status events, continuous draw looping,
the continuous draw half of taskman, full `lbcommon` rendering, or DS
display-list translator work. The visual HUD now proves one bounded original
`N64Logo` Sprite preview path and one bounded material-bearing Opening Room
DObj display-list preview path. The runtime verifier also checks that
material-bearing candidate's first-MObj diagnostic contract, detached
original-shaped prim-color branch emission, branch-expanded `G_DL` walk, and
live bounded preview consumption for that selected path. The new `ORTX` marker
proves the current selected material path has no texture-bearing original
`MObj` source yet. It still does not prove the general original renderer, broad
material/combiner mapping, texture upload, z-buffering, or continuous draw
semantics.
