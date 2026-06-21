# Known Issues

## Local Tooling Issues

- Local emulator binaries/configs now live under `emulators/`. melonDS remains
  the automated GDB/runtime verifier. no$gba is available through
  `scripts/debug-nogba.ps1` for interactive DS hardware, VRAM, OAM, palette,
  and timing inspection, and through `scripts/verify-nogba-smoke.ps1` for a
  launch/window-capture smoke check. It has no automated runtime-global or
  renderer-correctness verifier yet. The debugger build's window layout is a
  local setting: the current machine is configured for one combined debug
  window, while other settings can expose separate debugger and emulator
  windows. `scripts/capture-nogba.ps1 -AllWindows` handles both layouts.
- melonDS 1.1 previously started in this Codex desktop session as a live process
  with a hidden, untitled top-level window and no ARM9 GDB listener on `3333`.
  That reproduced with `smash64ds.nds`, the local `sm64-nds` ROM, and the
  devkitPro `Simple_Tri.nds` sample, so if it recurs, treat it as an
  emulator/session launch issue before changing ROM code. The verifier scripts
  now write melonDS's `Enable = true` compatibility key in addition to
  `Enabled = true`, without duplicating either key in `[Instance0.Gdb]`. A
  duplicate `Enable` / `Enabled` key in that TOML section can prevent the ARM9
  listener from opening and produce a misleading GDB connection timeout. The
  capture script temporarily disables true GDB keys for visible capture and
  restores the original config afterward. The current material-branch run
  verified GDB and window capture successfully.
- The current natural opening movie-to-Title visual appears after the default
  startup/Opening Room capture window. Use
  `scripts/capture-melonds.ps1 -Build -DelaySeconds 145` when the goal is to
  capture the Title preview after the paced action bridge. Shorter captures may
  correctly show only startup, Opening Room, portrait cards, name-card scenes,
  or one of the action-scene preview windows.
- Visual-debugging polish is now lower priority than moving the original source
  boundary forward. Keep captures and HUD counters as regression evidence, but
  do not spend the next milestone on visual fidelity unless it blocks a
  source-driven scene/menu import.

## Active Stub Boundaries

- The NDS dev/test scene harness is a build-time diagnostic entry point, not a
  replacement scene system. `NDS_DEV_SCENE_HARNESS=title` starts from
  `nSCKindTitle` by pre-seeding `dSCManagerDefaultSceneData` before imported
  `scManagerRunLoop` copies it, then runs the bounded imported Title path.
  `NDS_DEV_SCENE_HARNESS=vs_setup` now starts from `nSCKindVSMode` with
  `scene_prev = nSCKindTitle`, runs bounded imported `mnvsmode.c` setup,
  loads original `MNCommon`/`MNVSMode`, creates the original VS setup
  GObj/camera/SObj graph, and parks before `mnVSModeMain` input/update
  transitions and continuous drawing. `NDS_DEV_SCENE_HARNESS=vs_start_transition`
  runs the same setup, proves the original VS Start -> PlayersVS state change,
  and parks at the existing PlayersVS stub. `NDS_DEV_SCENE_HARNESS=battle_fd`
  is a reserved future slot and deliberately falls back to Title while
  recording a reserved marker; it does not create a fighter, load Final
  Destination, dispatch battle, or import gameplay.
- `syTaskmanRunTask` runs one bounded startup draw pass at update `17`, then
  55 bounded original startup updates through the Opening Room request and
  original load-scene break/eject path, mirrors the taskman cleanup tail, and
  returns to the scene manager. The subsequent Opening Room taskman seam runs
  560 bounded Opening Room updates and executes one bounded Opening Room
  `scene_draw`/`gcDrawAll` probe. It still returns before BattleShip's
  continuous draw/render path.
- `mvopeningportraits.c`, `mvopeningmario.c`, and the Donkey/Link/Samus/Yoshi/
  Kirby/Fox/Pikachu name-card scenes are imported enough to run bounded updates,
  draw original SObj previews, and hand off through the natural scene sequence.
  The fighter/stage-heavy action scenes from `OpeningRun` through
  `OpeningNewcomers` are still bounded bridge stubs in original order. They now
  show a paced original-action-asset SObj preview for each scene boundary, but
  this is not imported gameplay/action rendering and does not instantiate those
  scenes' fighter/stage object graphs.
- `mnTitleStartScene` now dispatches through imported `mntitle.c` /
  `mntitlefiles.c` for a bounded original Title setup slice: it loads
  `MNTitle` and `MNTitleFireAnim`, creates the original actor pair, four
  cameras, the original logo-fire GObj/display link, the original fire
  GObj/SObj/process/display boundary, and initial Title vars, normalizes the 30
  original `MNTitleFireAnim` frame sprites, then runs one guarded original
  `mnTitleFuncUpdate -> gcRunAll` tick on the natural
  `OpeningNewcomers -> Title` path before rendering ten selected original
  Sprite/Bitmap records through the bounded DS SObj preview path. Full title
  input, fire background presentation, animated logo, labels/Press Start,
  slash, logo-fire particles, audio, and continuous title draw remain deferred.
- `mnVSModeStartScene` now dispatches through imported `mnvsmode.c` for a
  bounded original VS setup slice, and the VS Start -> PlayersVS transition is
  proven through original `mnVSModeMain`. Full VS Mode navigation, rule/value
  editing, `VSOptions`, audio, and continuous menu drawing remain deferred.
  PlayersVS is still only a stub boundary; character select is not imported.
- `mvopeningroom.c` is imported with an NDS entry slice. Original video/task
  setup, relocation setup/file-list resolution, actor/default-camera,
  Scene 1 camera, close-up overlay camera, wallpaper-camera, and logo-camera
  creation,
  NitroFS O2R payload copying, blanket `u32` word byte-swap, internal
  pointer-chain relocation, selected symbol-offset probes, first tick-280
  pencils asset-reference probes, logo-camera camanim probes, logo asset
  probes, boss-shadow asset probes, pencils descriptor/animation table shape
  probes, tick-280 deferred fighter diagnostics, original
  `mvOpeningRoomMakeScene1Cameras`, original
  `mvOpeningRoomMakeCloseUpOverlayCamera`, original
  `mvOpeningRoomMakeWallpaperCamera`, original
  `mvOpeningRoomMakeLogoCamera`, original `mvOpeningRoomMakeLogo`, original
  `mvOpeningRoomMakePencils` object creation from inside the update callback,
  original logo, logo-wallpaper overlay, and boss-shadow setup/ejection,
  wrapper-created Outside, Haze, sunlight, and Desk setup plus sunlight
  ejection, tick-380 deferred
  fighter-status/rotation diagnostics, original tick-450
  `mvOpeningRoomMakeCloseUpOverlay` object creation, original tick-500
  `mvOpeningRoomMakeSpotlight` object
  creation, tick-500 deferred pulled-fighter display-link diagnostics, original
  tick-560 Scene 1 camera ejection plus Scene 2 camera creation, tick-560
  deferred Boss fighter status diagnostics, fighter-kind selection, ticks
  1-560, and one bounded Opening Room draw probe execute.
  Renderer/game-usable display-list and texture data, fighter models, effects,
  audio, rendering, remaining room objects, later Opening Room events, and
  `mvOpeningRoomMakePulledFighter`
  remain guarded.
- `lbCommonMakeSObjForGObj`, `lbCommonDrawSObjAttr`, and `lbCommonDrawSprite`
  are narrow startup `N64Logo` shims. They are sufficient to traverse the
  startup camera/display-link path, skip hidden SObjs, validate the one RGBA16
  Sprite/Bitmap asset, and copy it into a DS preview buffer. Full
  `lb/lbcommon.c` currently pulls in fighter part data and N64 display-list
  rendering definitions that are not ready.
- Object-display and object-script dependencies are still no-op or parked
  stubs. The bounded Opening Room draw probe now executes enough original
  object-display structure to reach `func_80017EC0`,
  `gcCaptureCameraGObj`, and `gcDrawDObjDLLinksForGObj`, then records the
  first exact blocker as DObj display-list translation (`ORDW`, blocker `3`,
  active Scene 2 camera CObj flags `0x4`, XObjs `2/3/8`, viewport
  `600,440,640,480`, callback `0x444C4E4B`, nonzero DObj display-list
  pointer, DObj meta `0x11`). A narrow diagnostic preview records that first
  linked DObj as fallback evidence, then prefers the first material-bearing
  link-27 `gcDrawDObjDLHead1` candidate once bounded branch expansion succeeds.
  The active `ORDP` path now parses the 42-command branch-expanded stream
  through `src/nds/nds_renderer.c`, classifies renderer state/skip/render
  commands, applies a best non-degenerate fallback plane because camera
  projection still has no projected triangles, samples the inline
  `G_IM_FMT_I`/16-bit texture state as a CPU I16 preview, and writes nonzero
  pixels to the retained top-screen preview. The renderer adapter now also
  decodes that bounded texture/tile/load state itself and the verifier asserts
  it, but it does not upload textures to DS VRAM, submit hardware polygons, or
  provide general camera/material/combiner behavior. `ORTX=0` still means the
  source-side scan found no texture-bearing original `MObj`; it does not mean
  the selected display list lacks inline texture commands.
  The DObj draw callbacks (`gcDrawDObjTreeForGObj`,
  `gcDrawDObjTreeDLLinksForGObj`, `gcDrawDObjDLLinksForGObj`, and
  `gcDrawDObjDLHead1`) still do not provide general N64 GBI/RDP rendering.
  The Opening Room logo-wallpaper overlay display callback is linked and its
  alpha state is initialized, `gcDrawDObjTreeDLLinksForGObj` is stored on the
  real logo GObj, `gcDrawDObjDLLinksForGObj` is stored on the real Outside,
  Haze, sunlight, and Desk GObjs, `gcDrawDObjDLHead1` is stored on the real
  boss-shadow GObj, `lbCommonDrawSprite` is stored on the real close-up overlay
  camera and wallpaper-camera GObjs, `mvOpeningRoomCloseUpOverlayProcDisplay`
  is stored on the real tick-450 close-up overlay GObj,
  `gcDrawDObjDLHead1` is stored on the real tick-500 spotlight GObj, and
  `func_80017EC0` is stored on the real Scene 1, Scene 2, and logo-camera
  GObjs, but those N64 GBI/RDP drawing bodies remain parked until
  display-list translation exists.
  `gcPlayCamAnim` is attached to the logo-camera GObj, but the current boundary
  verifies attachment/setup only. `gcParseGObjScript` is reached by the
  default camera GObj on each bounded update, but startup has no active GObj
  scripts yet. Original `sys/objanim.c` and `sys/interp.c` are imported for
  DObj setup and animation playback.
- Directly importing all of `sys/objdisplay.c` is currently too broad. It
  exposes missing `LookAt` matrix ABI, GBI texture/render-mode macros, sprite
  helpers, camera capture, and framebuffer/depth display-list commands before a
  DS display-list translator exists. Keep the current display stubs until a
  smaller original-backed draw slice is selected and verified.
- Relocation symbols and `lbReloc*` functions are still partial. The DS
  manifest resolves current Startup/Opening Room file IDs and loads current
  Opening Room O2R payload bytes from NitroFS. Blanket `u32` word byte-swap,
  internal pointer-chain relocation, and selected `ll...` symbol-offset
  resolution are implemented for the staged Opening Room files. The current
  `MVOpeningRoomScene1` Scene 1/logo-camera symbol, the `MVOpeningRoomScene2`
  camera symbol, plus the `MVCommon`
  pencils, logo, boss-shadow, Outside, Haze, sunlight, Desk, and spotlight symbols needed by the first
  tick-280 asset event, setup objects, tick-500 spotlight object, and tick-560
  Scene 2 camera setup resolve
  through the same backend, and the immediate pencils
  `DObjDesc`/animation table shape is validated. Original
  `mvOpeningRoomMakeScene1Cameras`,
  `mvOpeningRoomMakeCloseUpOverlayCamera`, `mvOpeningRoomMakeWallpaperCamera`,
  `mvOpeningRoomMakeLogoCamera`,
  `mvOpeningRoomMakeLogo`,
  `mvOpeningRoomMakePencils`, and `mvOpeningRoomMakeBossShadow` can consume
  those narrow slices. `mvOpeningRoomMakeSpotlight` also consumes the spotlight
  display-list/MObj/material-animation slice, and the project-owned wrapper
  consumes the Outside, Haze, sunlight, and Desk object slices. The startup
  `N64Logo` asset is the only renderable relocation slice: it has a
  asset-specific Sprite/Bitmap halfword normalizer after the blanket `u32`
  endian pass. The MVCommon material slice also has a narrow mixed-width
  normalizer for 18 background/logo/close-up/desk/spotlight `MObjSub` records
  used by the current material probe. The ORMT verifier proves no texture
  flags, no zero-flag fallback, 18 prim-color records, one light-bearing
  record, and no first texture offset for that selected source set. General mixed-width
  struct fixups, external dependency recursion, texture/display-list fixups,
  fighter data, and full symbol coverage are not implemented yet.
- Audio functions are stubs. No BGM, SFX, sequence, sample, or mixer backend is
  implemented.
- Save/backup functions are stubs. No persistent SRAM/flash behavior exists.
- RSP/RDP graphics tasks are acknowledged but display lists are not generally
  translated to DS rendering. The visible startup `N64Logo` is a bounded Sprite
  preview conversion that preserves the original bitmap overlap rows and
  presents a native `128x108` retained copy for melonDS debugging. The visible Opening Room DObj
  preview is now a narrow bounded display-list interpreter for the first
  material-bearing candidate: it uses active Scene 2 camera state, original
  DObj/XObj transform state, the emitted segment-`E` prim-color branch stream,
  42 parsed commands, four vertices, four triangles, five `G_DL` calls, two
  segment resolves, five color-state commands, prim color `0xFFFFFFFF`, and
  nonzero pixels. The original default RDP viewport math is implemented and
  verified for camera creation. The older link-6 preview remains a fallback,
  and the `ORMB`/`ORME`/`ORMP`/`ORMD`/`ORMX` diagnostics prove only this one
  selected material-candidate path. The new source-side texture-material scan
  currently reports `ORTX=0` with no texture-capable `MObj` candidates, so the
  selected material path is still prim-color only even though the fallback
  link-6 display-list preview can decode one inline `G_SETTIMG` texture.
  The `ORMX` branch-expanded command walk and visible `ORDP` command traversal
  now go through `src/nds/nds_renderer.c` with scene-provided
  validation/segment callbacks, which is the intended DS backend boundary, but
  it still only scans/proves and software-previews the current command family.
  General material/combiner mapping, texture source fixup and upload,
  z-buffering, broad display-list execution, full RDP reset state, and
  continuous draw are still not implemented.
- Overlay loading is a no-op. Overlay linker symbols exist only as compatibility
  placeholders.
- `osGetTime`/`osGetCount` now use libnds CPU timers 0/1. Other DS code must not
  claim those timers without replacing the shared monotonic timing backend.

## Source Compatibility Caveats

- `decomp/` contains independent upstream repositories and is read-only for
  this port. Use `docs/DECOMP_MAP.md` to decide which reference folders are
  useful, and keep local hooks in project-owned wrappers and compatibility
  layers.
- The Makefile intentionally does not include BattleShip's `include` directory
  globally because its N64 libc headers can conflict with devkitARM/libnds
  headers.
- DS shadow headers in `include/` are incomplete by design. Add only the ABI
  required by imported source and document broad stubs.
- `include/ft/fighter.h` and other shadow headers expose only the ABI needed by
  imported slices. They must keep enum values/layouts aligned with BattleShip
  when expanded.

## Current Build Warnings

Known nonfatal warnings come from original decomp code and temporary ABI
differences:

- signed/unsigned comparison in `sys/main.c`
- unused parameters or matching placeholders in original code
- scheduler pointer type mismatches from decomp task structs
- maybe-uninitialized warnings in `sys/vector.c`
- maybe-uninitialized warnings in imported `sys/objman.c`
- strict-aliasing/maybe-uninitialized warnings in imported `sys/objanim.c`
- maybe-uninitialized warning in imported `sys/interp.c`
- array-bounds warning in imported `sys/taskman.c` matching decomp-era globals
- unused parameter in `mnStartupActorFuncRun`

Do not silence warnings globally unless they block real signal. Prefer fixing or
isolating the compatibility type that causes the warning.

## Runtime Risks

- A broad stub can make verification pass while hiding missing behavior. When
  adding a stub, add a diagnostic or document the boundary.
- The DS has much tighter memory constraints than the N64. Do not optimize
  memory layout before proving the original code path, but expect overlays,
  assets, and display lists to need a deliberate memory plan.
- The DS taskman arena is currently 1 MiB to prove the Opening Room O2R payload
  path. This is not a final overlay/memory strategy.
- The N64 framebuffer dimensions and fixed memory addresses are unsafe on DS.
  Any original code that writes fixed `0x80xxxxxx` ranges must be inspected and
  guarded.
- Display-list translation is a major correctness boundary. Placeholder frames
  prove bootability, not rendering fidelity.
- The live melonDS HUD is diagnostic output. It now includes the bounded
  original `N64Logo` Sprite preview, the material-candidate Opening Room DObj
  preview, `dlp=4f524450 t=4 p>0 x=3f`, and
  `draw=4f524457 b=3 c=3 d=5`, but it does not prove that general original
  display lists or material branch lists are being rendered. The old DS-native
  moving probe render path is intentionally disabled so it does not flash over
  the original asset previews. The startup logo is still a software diagnostic
  preview; it is now presented as a native `128x108` retained copy and applies the
  original `SP_TEXSHUF` odd-row TMEM line unswizzle. The Opening Room preview
  is smaller and moved to the lower-left corner to avoid overlap in
  post-boundary captures. The moving top progress/orange markers and top status
  rectangles are disabled to avoid reading diagnostics as screen flashing. The
  startup logo can still look coarse because the visible diagnostic copy is the
  native `128x108` asset magnified by melonDS, not final renderer quality. The
  bottom HUD FPS row is ROM/VBlank-relative, so it can report normal pacing
  even when the host emulator window is visibly slow; `cv/ch` report retained
  original-preview content cadence, which is closer to visible movie progress.
  `scripts/sample-runtime-speed.ps1` reports one-shot hidden melonDS host speed;
  current 8-second evidence is `hostfps=54.28` with Opening Room at tick `420`.
  A 45-second one-shot sample after limiting retained Opening Room DObj work to
  two original draw probes plus `24` retained-preview reuse presentations still
  reached Opening Room tick `1320`, Portraits tick `150`, Mario tick `60`, and
  action bridge progress `4/128` at `hostfps=48.74`. That effectively rules out
  repeated Opening Room draw probes as the dominant slowdown.
  `scripts/verify-opening-movie-speed.ps1` now provides the maintained
  full-opening speed gate and passes with `hostfps=40.47`, `room=1320`,
  `rdraw=2/24`, `portraits=150`, `mario=60`, `action=9/324`, and
  `title=0x54494457`. The next performance target remains the opening
  movie/title bridge and later original scene imports, but performance work
  should follow the source-code import plan rather than more visual polish.
  Lowering `NDS_OPENING_MOVIE_DRAW_INTERVAL` below the verified `30` is not a
  safe fix yet: interval `10` missed the Title/action verifier window, and
  interval `20` reached Title but made the strict Opening Room DObj blocker
  sample unreliable.
  Use that value, emulator host-speed indicators, and capture timing for
  wall-clock performance symptoms. Do not repeatedly poll GDB for progress;
  repeated attach/detach has produced melonDS packet errors.
- `scripts/capture-melonds.ps1` captures pixels from the visible desktop window.
  Keep melonDS unobstructed; the script foregrounds it and temporarily disables
  GDB for visible capture, but Windows focus policy and hidden-window launches
  can still affect captures in remote or locked sessions.

## Documentation Gaps To Keep Closed

When importing a new subsystem, update:

- `docs/PORTING.md` with reused, replaced, stubbed, and verified behavior.
- `docs/STATUS.md` with the short current boundary and latest proof.
- `docs/HANDOFF.md` with the new runtime boundary.
- `docs/ROADMAP.md` status if a milestone changes.
- `docs/KNOWN_ISSUES.md` if a stub is added or removed.
- `docs/GOAL_DEBUGGING.md` when verifier workflow changes.
- `docs/DIAGNOSTIC_REFERENCE.md` when diagnostic globals or marker meanings
  change.
