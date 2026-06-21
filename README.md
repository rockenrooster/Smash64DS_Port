# Smash 64 DS Port

This workspace is an incremental Nintendo DS port of the Super Smash Bros. 64
decompilation in `decomp/BattleShip-main/decomp`. The DS backend follows the
general structure of `decomp/sm64-nds/src/nds`: original game code stays intact,
while libultra, rendering, input, audio, storage, overlays, and timing are
replaced behind an isolated platform layer.
The repositories under `decomp/` are read-only upstream references for this
project; local port hooks live in `src/import`, `src/port`, `src/nds`, and
`include`.

The current ROM is an architecture probe. It boots with libnds, runs a 60 Hz DS
loop, renders a placeholder frame, and directly compiles and executes the
original BattleShip `sys/main.c`, `sys/scheduler.c`, `sys/controller.c`,
`sys/video.c`, `sys/malloc.c`, `sys/utils.c`, `sys/vector.c`,
`sys/taskman.c`, `sys/objman.c`, `sys/objhelper.c`, `sys/objanim.c`, and
`sys/interp.c` sources.
BattleShip's original `sc/scmanager.c` now initializes its default state and
selects the US startup scene. BattleShip's original `mnstartup.c` reaches the
task-manager setup, runs `mnStartupFuncStart` through the real task/object
setup path, and enters the DS `syTaskmanRunTask` seam before the unported
draw/render path. The seam executes 55 bounded original task updates
(`syTaskmanCommonTaskUpdate -> syControllerFuncRead -> gcRunAll`), proving the
startup actor update, logo GObj thread coroutine, second fade request, proceed
opening flag, and original taskman load-scene break/eject path run through
imported object-manager code. It then mirrors the original taskman cleanup tail,
returns to `scManagerRunLoop`, and proves the original scene switch dispatches
the imported `mvopeningroom.c`. The NDS source guard preserves the original
Opening Room video/task setup, creates its actor, default camera, two Scene 1
cameras, close-up overlay camera, wallpaper camera, and later logo camera through the original object manager, selects two distinct fighters
with original RNG, and runs its original relocation setup plus the eight-file
Opening Room file list.
The DS backend packages those eight real BattleShip O2R resources into NitroFS
and copies their decompressed payload bytes into the original task heap. It then
applies the blanket `u32` endian pass used before BattleShip relocation-chain
parsing, patches the current files' internal relocation pointer chains, and
resolves a narrow set of BattleShip `ll...` symbol offsets through the DS
relocation backend. The current boundary resolves the BattleShip camera,
`MVCommon`, logo, boss-shadow, Outside, Haze, sunlight, and spotlight symbols used by the current
Opening Room setup and bounded update events:
`llMVOpeningRoomScene1CamAnimJoint` at offset `0`,
`llMVOpeningRoomScene2CamAnimJoint` at offset `0`,
`llMVCommonRoomPencilsDObjDesc` at `44728`,
`llMVCommonRoomPencilsAnimJoint` at `44912`,
`llMVCommonRoomLogoMObjSub` at `113760`,
`llMVCommonRoomLogoDObjDesc` at `115880`, and
`llMVCommonRoomLogoMatAnimJoint` at `116012`, plus Outside display-list offset
`147968`, Haze display-list offset `39160`, sunlight display-list offset
`149256`, and spotlight display-list,
MObj, and material-animation offsets `142872`, `142480`, and `143120`. It also validates the resolved
pencils data shape: four `DObjDesc` entries, three display-list pointers within
`MVCommon`, three animation-joint pointers, and first animation opcode `3`
(`SetValBlock`). It also resolves the BattleShip `MVCommon` boss-shadow display
list at offset `128912` and animation at offset `129316`. The imported Opening
Room setup now calls original `mvOpeningRoomMakeWallpaperCamera`, creating one
real camera GObj, one CObj, the original parked sprite camera display callback,
the original viewport values, and no default XObjs. It also calls original
`mvOpeningRoomMakeScene1Cameras`, creating two real camera GObjs, two CObjs,
four XObjs, two original parked camera display callbacks, two
`gcPlayCamAnim` process links, two camanim joint attachments, original viewport
values, and DL-buffer flags. It also calls original
`mvOpeningRoomMakeCloseUpOverlayCamera`, creating one real camera GObj, one
CObj, the original parked sprite camera display callback, original viewport
values, and no default XObjs. It also calls original
`mvOpeningRoomMakeLogoCamera`, creating one real camera GObj, one CObj, two
XObjs, the original parked camera display callback, the original
cam-animation process, the camanim joint attachment, and the original viewport
values. It also calls original `mvOpeningRoomMakeLogo`,
creating one real GObj, two DObjs, four XObjs, one MObj, a parked display
callback link, and a material animation attachment. The project-owned Opening
Room wrapper also creates real Outside, Haze, and sunlight GObjs, DObjs, XObjs, and
display links from the original room display-list references. The NDS Opening Room update
now reaches tick 280, records fighter creation as the only deferred first-event
portion, then calls original
`mvOpeningRoomMakePencils` from inside `mvOpeningRoomFuncRun`. The backend
measures that update-created object path: one real GObj, three DObjs, six
XObjs, the original process link, a parked display callback link, a three-node
DObj tree, and one animation root. The same update also calls original
`gcEjectGObj` on the logo GObj, the logo-wallpaper overlay GObj created during
Opening Room setup, and the boss-shadow GObj created through original
`mvOpeningRoomMakeBossShadow`; the verifier proves all three pointers leave
both original common and display-link lists.
The update now continues through tick 380 and records the original
pulled-fighter status/rotation branch as explicitly deferred because fighter
creation remains guarded. At tick 450 it calls original
`mvOpeningRoomMakeCloseUpOverlay`, creating one real GObj with the original
display callback on display link 26 and overlay alpha initialized to `0`; the
same tick ejects the real sunlight GObj created by the project-owned Opening
Room wrapper through original `gcEjectGObj` and proves common/display-link
unlinking. The update now reaches tick 500, explicitly defers
the pulled-fighter display-link move because fighter creation remains guarded,
and calls original `mvOpeningRoomMakeSpotlight`. That path builds one real
GObj, one DObj, one XObj, two MObjs, the original display callback on display
link 27, the original `gcPlayAnimAll` process, a material-animation
attachment, and the original fighter-kind-dependent spotlight position. The ROM
then reaches tick 560, calls original `mvOpeningRoomEjectCameraGObjs`, verifies
the previous Scene 1 camera GObjs/CObjs are released, and calls original
`mvOpeningRoomMakeScene2Cameras`. That path creates two Scene 2 camera GObjs,
two CObjs, four XObjs, two original camera display callbacks, two
`gcPlayCamAnim` processes, two camanim attachments, original viewport values,
and DL-buffer flags. The same tick explicitly defers the Boss fighter status
update because Boss fighter creation remains guarded. The ROM parks immediately
after that tick-560 Scene 2 camera boundary because external relocation
dependencies, texture/display-list fixups, fighter setup, remaining room
objects, later movie events, and renderer-safe object data are still deferred.
BattleShip's original `scsubsyscontroller.c` is imported, and a second melonDS
test proves A/B/Start skips from the movie to the Title scene through the
original taskman return.
The verifier reads real startup
GObj/CObj/SObj/process links, display-list buffers, graphics heap setup, RDP
buffer setup, task callbacks, update count, skip-delay state, GObj thread sleep
count, post-update logo position, taskman cleanup state, and Opening Room
dispatch/NitroFS relocation-payload markers from the imported managers and
scene loop. A narrow `lbCommonMakeSObjForGObj` startup shim remains until the
full sprite/render pipeline can be imported safely.
An ARM9 stackful-coroutine backend implements the libultra thread and message
queue behavior needed to run the original startup chain through its
startup-scene handoff. DS VBlank events are consumed by the original scheduler;
libnds keys flow through the original controller state/edge logic. SP/DP task
execution remains a placeholder backend. This is not a gameplay rewrite.

## Build

Install devkitPro with the Nintendo DS toolchain, then run:

```sh
make
```

On Windows PowerShell, when the environment still contains Unix devkitPro
paths, use:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make
```

The output is `smash64ds.nds`. Generated object files and staged NitroFS data
are confined to `build/`; root ROM/ELF outputs are generated artifacts.

Local emulator binaries live under `emulators/` so the repo root stays clean.
The default melonDS path is `emulators/melonds/melonDS.exe`.

Verify the live ARM9 state through melonDS's GDB stub:

```powershell
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
```

For focused active-boundary checks:

```powershell
.\scripts\verify-opening-boundary.ps1
.\scripts\verify-title-boundary.ps1
```

For the maintained all-up regression chain:

```powershell
.\scripts\verify-all.ps1
```

For a quick wall-clock speed sample without waiting for the full verifier
window:

```powershell
.\scripts\sample-runtime-speed.ps1 -DelaySeconds 8
```

For the maintained full opening movie-to-Title host-speed gate:

```powershell
.\scripts\verify-opening-movie-speed.ps1
```

For visual debugging in melonDS, launch the ROM with the live DS HUD:

```powershell
.\scripts\debug-melonds.ps1 -Build
```

Capture the real emulator window as regression evidence:

```powershell
.\scripts\capture-melonds.ps1 -Build
```

For interactive no$gba hardware/debugger inspection, place `NO$GBA.EXE` under
`emulators/nogba/` and launch:

```powershell
.\scripts\debug-nogba.ps1 -Build
```

melonDS remains the automated verifier; no$gba is for manual VRAM/OAM/register
and renderer-timing investigation. For automated no$gba window smoke/capture:

```powershell
.\scripts\verify-nogba-smoke.ps1 -Build
.\scripts\capture-nogba.ps1 -Build -AllWindows
```

See `docs/EMULATOR_STRATEGY.md` for when to choose no$gba over melonDS.

Latest verified visual capture: `artifacts/melonds-20260620-160953.png`.
Visual capture and renderer debugging are currently treated as regression
evidence, not the main next milestone. Near-term work should favor importing
the next original BattleShip scene/menu/game-code boundary with minimal bounded
rendering over polishing preview visuals.

The top screen now favors retained original visual previews: the startup
`N64Logo`, bounded Opening Room DObj preview, opening movie sprite previews, and
bounded Title preview. The bottom HUD reports compact boot/startup/taskman,
Opening Room relocation/object/draw, movie bridge/action/present-frame, sampled
ROM-side FPS/content-cadence, and Title diagnostics without scrolling or
wrapping. Earlier top status bars were
removed because they looked like flashing in live emulator output. Opening Room
diagnostics still cover NitroFS/O2R payload loading, blanket word byte-swap,
internal pointer fixup, symbol probes, first-event readiness, original pencils
object creation, tick-380/tick-450/tick-500/tick-560 deferred-event markers,
close-up overlay object-creation readiness, spotlight
object-creation readiness, Scene 2 camera ejection/creation readiness, and the
Opening Room tick-560 boundary. The center bar tracks ticks 0-560; the bottom
screen is change-redrawn by row and prints the same live
diagnostics used by the verifiers, including
`event=4f524631 tick=280 mask=03` and
`edata=4f524644 m=0f d=4 dl=3 a=3 op=3` when the first tick-280 pencils data
shape is proven, `evrun=4f523238 def=01 f=4f524646/...` when the tick-280
fighter boundary is deferred,
`s1cam=4f523143 m=1ff vp=1` when original Scene 1 camera setup passes,
`ccam=4f524343 m=1f vp=1` when original close-up overlay camera setup passes,
`wcam=4f525743 m=1f vp=1` when original wallpaper-camera setup passes,
`lcam=4f52434d m=7f a=01 vp=1` when original logo-camera setup passes,
`ovl=4f524f43/4f524f45 m=03` when the original overlay setup/ejection boundary
passes, `logo=4f524c43/4f524c45 m=03 c=3f` when original logo setup/ejection
passes, `boss=4f524243/4f524245 m=03 c=3f` when the original boss-shadow
setup/ejection boundary passes, plus `pencil=4f525043 m=3f g=1 d=3 x=6` and
`pencil a=0 p=1 dl=1 t=3 r=1` after the original pencils update path, followed
by `or38=4f523338 d=01`, `or45=4f523435 d=00 su=03`,
`obj d=0f h=0f o=0f s=0f c=07`, `or50=4f523530 d=01 sp=ff`, and
`or56=4f523536 d=01 e=07`, `s2=1ff a=01 g=2 c=2 x=4`, and
`spG=1 d=1 x=1 m=2` at the current tick-560 boundary. The final two rows now
use `fps=.. up=.. dl=.. cv=..` and
`ch=... pf=... smp=.. win=..`; `fps` is the ROM/VBlank-relative present rate,
`up` is imported opening-movie update cadence, `dl` is retained DObj-preview
draw cadence, `cv/ch` are original-preview content cadence, and none of these
are host wall-clock emulator FPS. Lines are intentionally shortened to avoid
wrapping on the DS bottom screen.

## Source layout

- `src/nds`: Nintendo DS entry point and hardware backend.
- `src/port`: N64 compatibility stubs and temporary architecture probes.
- `src/import`: wrappers that compile original BattleShip translation units.
- `include/PR`: minimal libultra-compatible interfaces for ported source.
- `decomp/BattleShip-main/decomp`: read-only original Smash 64 game-code source
  of truth.
- `decomp/BattleShip-main`: read-only BattleShip source/docs/tools/assets
  context for this port.
- `decomp/sm64-nds`: read-only architectural reference only.
- `docs/DECOMP_MAP.md`: folder-by-folder guide to useful read-only reference
  material under `decomp/`.
- `docs/STATUS.md`: short current-truth summary for active planning.
- `docs/ARCHITECTURE.md`: port architecture and subsystem boundaries.
- `docs/HANDOFF.md`: current state, commands, verified boundary, and next work.
- `docs/ROADMAP.md`: milestone plan toward a playable full-game port.
- `docs/KNOWN_ISSUES.md`: active stubs, risks, and compile/runtime caveats.
- `docs/GOAL_DEBUGGING.md`: short build, emulator, and verifier workflow.
- `docs/DIAGNOSTIC_REFERENCE.md`: diagnostic globals, marker meanings, and
  manual GDB reference.
- `docs/EMULATOR_STRATEGY.md`: when to use melonDS, no$gba, or both.
- `docs/PORTING.md`: append-only chronological porting history.
- `AGENTS.md`: repo-specific instructions for future coding agents.
