# Porting Log

## Architecture rules

- Compile original BattleShip translation units whenever practical.
- Put DS hardware behavior in `src/nds` and compatibility behavior in
  `src/port`.
- Add only the smallest shim needed for the next original subsystem.
- Do not modify either reference project's generated `build` directory.
- The DS probe loop is disposable scaffolding, not replacement gameplay.

## Original boot path

BattleShip starts in `decomp/src/sys/main.c`:

1. `syMainLoop` initializes libultra and starts thread 1.
2. `syMainThread1Idle` starts thread 5.
3. `syMainThread5` initializes PI/DMA and starts scheduler, audio, and
   controller threads.
4. Thread 5 loads the scene-manager overlay.
5. `scManagerRunLoop` begins the original scene/game-state loop.

The desktop BattleShip port preserves that chain with coroutine-backed thread
stubs. The DS port will initially use a cooperative scheduler driven by VBlank;
the original thread entry points remain the target API.

## 2026-06-19: Milestone 1 architecture probe

Reused unchanged:

- `BattleShip-main/decomp/src/sys/utils.c`
- `BattleShip-main/decomp/src/sys/vector.c`
- BattleShip `Vec3f` and public sys headers

Replaced:

- N64 entry point with `src/nds/main.c`.
- VI timing with a libnds VBlank loop.
- Controller input with libnds key scanning.
- Video with a temporary 16-bit framebuffer renderer.
- `osGetTime`, `__sinf`, and `__cosf` with DS-compatible shims.

Stubbed or compatibility-only:

- Minimal `PR/ultratypes.h`, `PR/os.h`, and `PR/gu.h` surface needed by the
  imported sys slice.
- A DS-side moving-square probe exercises original vector code. It originally
  used RNG color as a liveness marker, but the current visual-debug path keeps
  it stable so the original asset previews do not appear to flicker.

Deferred:

- Original libultra message queues and thread scheduling.
- ROM DMA, filesystem asset mapping, and relocatable overlays.
- N64 display-list interpretation on the DS GPU.
- Audio, save data, menus, fighters, stages, items, and full game-state flow.

Verification:

- Clean devkitARM/libnds build produces `smash64ds.nds`.
- `ndstool -i` reports valid header, logo, secure-area, and banner CRCs.
- The linked ELF contains `syUtilsRandUShort`, `syUtilsSetRandomSeedPtr`, and
  `syVectorAddScaled3D` from the imported BattleShip sources.
- melonDS accepts the ROM and remains responsive in the VBlank-driven loop.

## Next integration target

Expand the imported `syTaskmanRunTask` update/draw loop one bounded piece at a
time. The real taskman and object-manager setup now runs, startup updates return
through taskman cleanup, and the original scene manager dispatches Opening
Room. The next risks are Opening Room initialization, object display, script,
animation, and display-list rendering.

## 2026-06-19: Original boot-chain milestone

Reused unchanged:

- `BattleShip-main/decomp/src/sys/main.c`, compiled through
  `src/import/battleship_sys_main.c`.
- The original `syMainLoop` -> idle thread -> thread 5 startup sequence.
- The original initialization message queue and service-thread ordering.

Implemented for DS:

- ARM9 assembly context switching for stackful, destroyable coroutines.
- `osCreateThread`, `osStartThread`, `osStopThread`, `osDestroyThread`, thread
  priority metadata, and per-frame cooperative resumption.
- FIFO `osSendMesg` / `osRecvMesg` and front-inserting `osJamMesg`, including
  blocking coroutine yields and nonblocking full/empty failures.
- A boot self-test covering FIFO order, jam order, full/empty behavior, and a
  thread suspended and resumed through a blocking receive.

Temporarily stubbed at the original API boundary:

- Audio and controller thread bodies. Each currently performs its original
  boot handshake and then blocks.
- PI/ROM DMA initialization and scene-manager overlay relocation.
- `scManagerRunLoop`; the stub records that original thread 5 reached the
  scene-manager handoff and returns.

Runtime verification through melonDS's ARM9 GDB stub confirms:

- `gNdsBootSelfTestResult == 0x50415353` (`PASS`).
- `gNdsOriginalBootStage == 0x53430007` (all three service handshakes and the
  scene-manager call).
- BattleShip's original IMEM and DMEM status flags are set.
- The DS VBlank frame counter continues advancing after `syMainLoop` returns.

## 2026-06-19: Original scheduler milestone

Reused unchanged:

- `BattleShip-main/decomp/src/sys/scheduler.c` in full.
- Scheduler task queues, task priority/state transitions, VI clients,
  framebuffer selection, and `sySchedulerVRetrace`.

Replaced:

- `osViSetEvent` records the scheduler's retrace queue in the DS backend.
- The libnds frame loop posts one retrace message per VBlank.
- VI mode, blackout, and framebuffer-swap calls currently update compatibility
  state rather than N64 VI registers.

Stubbed:

- RSP task load/start/yield. Start currently posts an immediate SP-complete
  event without interpreting the display list.
- RDP buffer submission. It currently posts an immediate DP-complete event.
- Controller rumble calls made during scheduler soft reset.

Verification:

- The automated melonDS check now requires `sSYSchedulerTicCount > 0`, proving
  VBlank messages reach BattleShip's original `sySchedulerVRetrace` handler.

## 2026-06-19: Original controller milestone

Reused unchanged:

- `BattleShip-main/decomp/src/sys/controller.c`.
- `BattleShip-main/decomp/src/sys/maindevice.c` for `gSYControllerMain`.
- Controller discovery, button tap/release/repeat accumulation, scheduler
  client registration, global controller state, and controller event parsing.

Replaced:

- SI status/read completion with immediate message-queue completion on DS.
- `osContGetReadData` with a libnds key-to-`OSContPad` mapping.
- Rumble Pak calls with explicit unsupported returns.

Current mapping:

- DS A/B -> N64 A/B.
- DS X or Y -> N64 C-up (jump).
- DS L -> N64 Z; DS R -> N64 R.
- DS D-pad -> full-range analog stick.
- DS Start -> N64 Start.

Verification:

- The boot self-test validates every mapped button and both stick axes.
- melonDS verification requires one discovered controller and a nonzero SI
  poll count from the original controller thread.
- The placeholder frame probe now reads `gSYControllerMain`, after
  `syControllerUpdateGlobalData`, instead of consuming libnds input directly.

## 2026-06-19: Original video and allocator milestone

Reused unchanged:

- `BattleShip-main/decomp/src/sys/video.c`.
- `BattleShip-main/decomp/src/sys/malloc.c`.
- Original framebuffer and z-buffer storage translation units.

Integration:

- A temporary coroutine calls `syVideoInit` with BattleShip's standard
  framebuffer layout.
- The original video code sends framebuffer and VI tasks through
  `func_80000970`; the original scheduler processes and acknowledges both.
- VI register writes remain compatibility-state updates in the DS backend.

Verification:

- The boot self-test checks original allocator alignment, pointer advancement,
  and reset behavior.
- melonDS verification requires `gNdsVideoBootstrapResult == 0x56494430`, set
  only after scheduler acknowledgements return and original video globals show
  320x240, 16-bit mode, and the original z-buffer.

## 2026-06-19: Original scene-manager milestone

Reused:

- `BattleShip-main/decomp/src/sc/scmanager.c` through its complete scene loop.
- Original backup, scene, VS battle, and 1P battle default-data initializers.
- Original overlay selection and `SCKind` dispatch switch.

Adapted for DS:

- The N64 framebuffer-clear upper address is conditional on
  `SSB64_TARGET_NDS`; DS clears only the imported framebuffer array.
- Compatibility headers expose the narrow scene, fighter, menu, movie, audio,
  backup, and overlay contracts required by this source slice.
- N64-only scene-manager debug inspectors after the game loop are excluded
  until the object/task/render structures they inspect are imported.

Stubbed at the next architecture boundary:

- Overlay linker symbols and relocation remain no-op DS contracts.
- Audio settings, backup validation/application, and fighter file-size setup
  are placeholders.
- Scene entry functions record the selected scene and park original thread 5;
  none contains replacement gameplay or menu logic.

Verification:

- Clean devkitARM/libnds build produces the ROM with the original scene
  manager linked.
- melonDS requires `gNdsSceneBoundaryResult == 0x53434E45` (`SCNE`).
- melonDS requires scene kind `27`, proving the original US defaults and switch
  reached `mnStartupStartScene`.

## 2026-06-19: Original startup scene milestone

Status note: this bridge state was superseded later the same day by the
original taskman/object-manager setup milestone below. Keep this section as
chronological context; use the later verifier values for current debugging.

Reused:

- `BattleShip-main/decomp/src/mn/mncommon/mnstartup.c`.
- Original startup display-list data, video setup, and task-manager setup.
- Original `mnStartupStartScene` entry point.
- Original `mnStartupFuncStart` initialization path.

Adapted for DS:

- Added narrow `PR/gbi.h`, `sys/taskman.h`, `sys/objdef.h`, `sys/rdp.h`, and
  `reloc_data.h` compatibility surfaces required by the startup source.
- `syAudioStopBGMAll` and `syRdpSetViewport` are DS backend stubs.
- `syTaskmanStartTask` records startup task setup diagnostics, calls the
  original startup `func_start`, records the startup object/reloc/fade requests,
  and parks original thread 5.
- Startup `gcMake*`, `gcAdd*`, and `lbCommonMakeSObjForGObj` bridge calls now
  preserve distinct GObj/SObj/CObj/process records and key parent/callback
  relationships.
- Startup `syTaskmanStartTask` now mirrors the original taskman allocation
  sequence into DS-owned general heap, display-list buffers, per-context
  graphics heaps, and RDP output buffer state before calling `func_start`.

Stubbed at the next architecture boundary:

- Original task manager update/draw loop.
- Object manager creation, display, and process scheduling.
- Relocation file loading and the N64 logo asset.
- Fade actor and sprite draw helpers.

Verification:

- melonDS requires `gNdsStartupTaskmanResult == 0x53545254` (`STRT`).
- melonDS requires startup scene kind `27`.
- melonDS requires the original startup display-list buffer sizes:
  `10240` bytes for buffer 0 and `10240` bytes for buffer 1.
- melonDS requires the startup task setup to preserve
  `syControllerFuncRead`.
- melonDS requires `gNdsStartupFuncStartResult == 0x46535452` (`FSTR`).
- melonDS requires original startup init state: skip delay `8`, opening flag
  false, two GObj requests, two camera requests, one relocation setup, one logo
  sprite request, and one fade actor request.
- melonDS requires startup object relationships: logo SObj attached to the
  wallpaper GObj, logo position `96, 220`, `SP_FASTCOPY` cleared, actor
  function preserved, wallpaper process kind/priority preserved, wallpaper
  display callback/link/tag preserved, wallpaper camera mask preserved, and
  default camera fill color `0xFF`.
- melonDS requires taskman bridge state: `TASK` marker, two contexts, one gfx
  task, `0x2800` graphics heap size, RDP kind `2`, RDP buffer size `0xC000`,
  25 mirrored allocation calls, nontrivial heap usage, valid display-list
  buffers for both contexts, controller auto-read represented, and startup
  update/draw/lights callbacks preserved.

## 2026-06-19: Original taskman and object-manager setup milestone

Reused:

- `BattleShip-main/decomp/src/sys/taskman.c`.
- `BattleShip-main/decomp/src/sys/objman.c`.
- `BattleShip-main/decomp/src/sys/objhelper.c`.
- Original `syTaskmanStartTask`, `syTaskmanLoadScene`, `syTaskmanMalloc`,
  `gcSetupObjman`, `gcMake*`, `gcAdd*`, object free-list growth, and startup
  GObj/CObj/SObj/process linking.

Adapted for DS:

- `mnStartupStartScene` uses a DS taskman arena under `SSB64_TARGET_NDS`
  instead of relying on overlay-bound address arithmetic.
- `sys/taskman.c` omits only the original `syTaskmanRunTask` definition under
  `SSB64_TARGET_NDS`; the DS backend provides the parked seam.
- `syTaskmanMalloc` increments `gNdsTaskmanMallocCount` so verification reads
  the real imported allocation path.
- Overlay linker symbols remain zero-valued compatibility placeholders; they no
  longer need to bracket the DS arena.

Stubbed or still DS-owned:

- `syTaskmanRunTask`: initially parked before the original per-frame
  update/draw loop; superseded by the bounded-update milestone below.
- `gcInitDLs`, `gcSetMatrixFuncList`, `gcSetCameraMatrixMode`,
  `gcCaptureCameraGObj`, `gcParseGObjScript`, `gcGetTreeDObjNext`,
  `func_80017DBC`: renderer/script/animation dependencies not reached before
  the parked loop.
- `syRdpSetFuncLights` and `syRdpResetSettings`: RDP/camera compatibility
  stubs. `syRdpSetViewport` and `syRdpSetDefaultViewport` were later replaced
  with narrow original-math DS shims.
- `lbCommonMakeSObjForGObj`, `lbCommonDrawSObjAttr`, and `lbCommonDrawSprite`:
  narrow startup shim. The full `lbcommon.c` import is deferred because it
  currently pulls in fighter part layouts, look-at helpers, and many N64
  display-list macros.

Retired:

- The hand-written DS `syTaskmanStartTask` bridge.
- The hand-written startup object pool records for actor/camera/wallpaper/logo
  objects.

Verification:

- Clean build plus `scripts/verify-runtime.ps1` passes.
- melonDS requires `gNdsStartupGObjCreateCount == 4` because camera GObjs are
  real GObjs in the imported object manager.
- melonDS requires `gNdsStartupTaskmanMallocCount == 36`, counted inside imported
  `syTaskmanMalloc`.
- melonDS requires `gNdsTaskmanLoopReached == 1`, proving
  `syTaskmanLoadScene` reached the parked `syTaskmanRunTask` seam.

## 2026-06-19: Bounded original task updates and visual debug milestone

Reused:

- Imported `syTaskmanCommonTaskUpdate` through the task function pointer set by
  original `syTaskmanStartTask`.
- Imported `gcRunAll`, `gcRunGObj`, and `gcRunGObjProcess`.
- Original startup actor function `mnStartupActorFuncRun`.
- Original logo GObj thread `mnStartupLogoThreadUpdate` through the imported
  object thread/process path.

Adapted for DS:

- The DS `syTaskmanRunTask` seam snapshots post-`func_start` startup state,
  initially executed two original `task_update` calls, incremented
  `dSYTaskmanUpdateCount`, captured post-update logo position, and parked
  before draw. This proved the logo GObj thread could yield and resume through
  the DS coroutine backend.
- `gcSleepCurrentGObjThread` increments a DS diagnostic under
  `SSB64_TARGET_NDS` so runtime verification can prove the logo GObj thread
  actually entered, yielded, and resumed.
- `src/nds/nds_platform.c` renders a live melonDS visual debug HUD on top of
  the placeholder frame and bottom-screen console. The HUD reads the same
  diagnostic globals as `scripts/verify-runtime.ps1`.

Still parked:

- `task_draw` / `gcDrawAll`.
- Object display capture and display-list rendering.
- Full `lbcommon.c` sprite/display-list implementation.

Verification:

- `scripts/debug-melonds.ps1 -Build` launches a visible emulator session with
  the live HUD for manual visual debugging.

## 2026-06-19: Startup Opening Room request milestone

Reused:

- Original `mnStartupLogoThreadUpdate` through 55 logo-thread sleeps.
- Original `mnStartupActorFuncRun` scene-selection logic.
- Original `syTaskmanCommonTaskUpdate` break/eject behavior after
  `syTaskmanSetLoadScene`.

Adapted for DS:

- `NDS_TASKMAN_BOUNDED_UPDATES` is now `55`, which runs long enough for the
  startup logo coroutine to reach its final Y position, request the second fade,
  set `sMNStartupIsProceedOpening`, and let the actor request
  `nSCKindOpeningRoom` on the next update.
- The DS seam captures post-update scene, taskman status, active GObj count,
  fade request count, and proceed-opening state.
- The visible melonDS HUD shows these transition diagnostics alongside the
  verifier markers.

Still parked:

- Original Opening Room scene initialization and assets.
- `task_draw` / `gcDrawAll`.
- Object display capture and display-list rendering.

Verification:

- melonDS requires `gNdsTaskmanBoundedUpdateCount == 55`.
- melonDS requires `gNdsTaskmanPostUpdateSkip == 0`.
- melonDS requires `gNdsTaskmanGObjThreadSleeps == 55`.
- melonDS requires `gNdsTaskmanPostUpdateLogoPosX == 96` and
  `gNdsTaskmanPostUpdateLogoPosY == 65`.
- melonDS requires `gNdsTaskmanPostUpdateOpening == 1`.
- melonDS requires `gNdsTaskmanPostUpdateSceneKind == 28` and
  `gNdsTaskmanPostUpdateScenePrev == 27`.
- melonDS requires `gNdsTaskmanPostUpdateStatus == 1`.
- melonDS requires `gNdsTaskmanPostUpdateGObjCount == 0`, proving the original
  break/eject path ran.
- melonDS requires `gNdsTaskmanPostUpdateFadeCount == 2`.

## 2026-06-19: Taskman cleanup and Opening Room dispatch milestone

Reused:

- Original taskman cleanup contract after the run loop: scheduler no-op task,
  queue draining, RDP light callback clear, and terminal mode `2`.
- Original `scManagerRunLoop` scene switch after `mnStartupStartScene` returns.
- Original overlay-selection calls for `nSCKindOpeningRoom`; overlay loading is
  still a DS compatibility no-op.

Adapted for DS:

- The bounded DS `syTaskmanRunTask` seam mirrors the original prepare and
  cleanup state around its update-only loop, then returns instead of parking
  thread 5.
- `mvOpeningRoomStartScene` is a dedicated DS parking boundary that increments
  `gNdsOpeningRoomDispatchCount`; other scene stubs remain generic boundaries.
- The live melonDS HUD shows cleanup, return, and Opening Room dispatch state.

Still parked:

- Original `mv/mvopening/mvopeningroom.c` initialization.
- `task_draw` / `gcDrawAll` and display-list translation.
- Opening Room asset payload loading and relocation fixups; later milestones add
  raw O2R payload loading, but not fixed-up renderer/game-usable data.

Verification:

- melonDS requires `gNdsTaskmanCleanupResult == 0x434C4E50` (`CLNP`).
- melonDS requires `gNdsTaskmanCleanupQueuesEmpty == 1`.
- melonDS requires `gNdsTaskmanCleanupMode == 2`.
- melonDS requires `gNdsTaskmanReturnCount == 1`.
- melonDS requires `gNdsOpeningRoomDispatchCount == 1`, proving the original
  scene-manager switch dispatched the requested scene.

## 2026-06-19: Imported Opening Room entry and visual capture milestone

Reused:

- Original `mv/mvopening/mvopeningroom.c` translation unit through
  `src/import/battleship_mvopeningroom.c`.
- Original Opening Room `SYVideoSetup` and `SYTaskmanSetup` values.
- Original taskman/object-manager path for scene setup, actor creation, default
  camera creation, and `gcRunAll` dispatch.
- Original `mvOpeningRoomFuncRun` time-tick increment.

Adapted for DS:

- An `SSB64_TARGET_NDS` entry slice in the original source keeps the executable
  portion bounded before relocation assets, fighters, effects, audio, and N64
  display-list rendering are available.
- Opening Room reuses the DS taskman arena after Startup cleanup.
- The DS taskman seam recognizes scene kind `28`, executes exactly one original
  update, records the scene state, and parks before draw.
- `scManagerFuncUpdate` and `scManagerFuncDraw` use their exact original wrapper
  behavior in `src/port/scene_backend.c` because `scmanager.c` excludes its NDS
  renderer block.
- Taskman allocation diagnostics are split into a Startup snapshot and Opening
  Room delta.
- The live HUD has Opening Room entry/update markers.
- `scripts/capture-melonds.ps1` foregrounds the emulator and captures its real
  window to a PNG under ignored `artifacts/` output.

Still parked:

- Fixed-up Opening Room relocation contents and renderer-usable asset data.
- Fighter manager, effects, audio, movie cameras/models, and later movie ticks.
- `task_draw`, `gcDrawAll` display capture, and N64 display-list translation.

Verification:

- `ORST`, `ORFS`, and `ORUP` markers prove entry, `func_start`, and update.
- Opening Room time is exactly `1` tick.
- The imported object manager reports two GObjs and one camera.
- Original task sizes remain DL `12000`/`4096`, graphics heap `32768`, and RDP
  output buffer `49152`.
- Clean melonDS GDB verification passes, and a live window capture was visually
  inspected.

## 2026-06-19: Opening Room pre-asset loop and skip-to-Title milestone

Reused:

- Complete original `sc/scsubsys/scsubsyscontroller.c` translation unit.
- Original Opening Room pulled/dropped fighter-kind tables and rejection logic.
- Original `mvOpeningRoomFuncRun` time increment, neutral-stick gate, button tap
  query, scene mutation, and `syTaskmanSetLoadScene` call.
- Original `syTaskmanCommonTaskUpdate` object ejection and scene-manager return.

Adapted for DS:

- `osGetTime` and `osGetCount` now use libnds' monotonic 32-bit CPU timer instead
  of a frame-only counter. This prevents original time-RNG rejection loops from
  repeating the same value forever inside one coroutine activation.
- The Opening Room taskman seam runs original controller-synchronized updates
  through tick `279`, then parks immediately before tick `280`, where unloaded
  movie assets would first be used.
- The HUD progress bar scales over the full 0-279 interval and reports `ORPA`,
  controller-check count, selected fighter kinds, and skip count.
- `scripts/verify-opening-skip.ps1` uses melonDS GDB to inject N64 A at the real
  movie callback, then continues to the Title parking call in the same debug
  session.

Still parked:

- Tick `280` object creation (`mvOpeningRoomMakePulledFighter` and pencils).
- Relocation data usability, fighter/effect initialization, audio, and drawing.

Verification:

- Normal melonDS run reaches tick `279`, `ORPA`, and 270 original controller
  checks; selected fighter kinds are valid and distinct.
- Skip-path run transitions scene `28 -> 1` during ticks `10-278`, performs a
  second taskman cleanup/return, and reaches `mnTitleStartScene`.

## 2026-06-19: Opening Room relocation file-list milestone

Reused:

- Original Opening Room `dMVOpeningRoomFileIDs` order:
  `MVCommon`, room transition, scenes 1-4, run crash, and room wallpaper.
- Original `LBRelocSetup` fields and `lbRelocInitSetup` /
  `lbRelocLoadFilesListed` call position inside `mvOpeningRoomFuncStart`.

Adapted for DS:

- `include/reloc_data.h` now exposes the narrow relocation ABI needed by the
  imported Opening Room slice, including file-list load helpers and only the
  current `ll...FileID` symbols.
- `src/port/scene_backend.c` owns a DS relocation manifest that treats
  `&ll...FileID` addresses as unique file-ID tokens, records the eight-file
  Opening Room mask, and preserves the earlier Startup logo heap behavior.
- The melonDS HUD and both runtime verifiers now check `ORRL`, init count
  `1`, load count `8`, and file mask `0xFF`. At this historical milestone the
  payload data was still unavailable; the later NitroFS O2R milestone
  supersedes that with data-ready `1` and fixup-ready `0`.

Still parked:

- Raw payload loading was still deferred at this milestone; the next milestone
  uses NitroFS O2R payloads instead of converted `src/relocData/*.c` assets.
- Selected `ll...` probes were still absent at this milestone; later milestones
  add them, but not fully fixed-up payload data.
- Tick `280` object creation, fighter/effect initialization, audio, and draw.

Verification:

- `scripts/verify-runtime.ps1` requires the normal no-input path to resolve the
  original eight-file Opening Room relocation list before reaching `ORPA`.
- `scripts/verify-opening-skip.ps1` requires the same relocation-list marker
  before proving the original skip-to-Title path.

## 2026-06-19: Opening Room NitroFS O2R payload milestone

Reused:

- BattleShip's extracted `BattleShip_o2r` resources for the original Opening
  Room relocation list:
  `MVCommon`, room transition, scenes 1-4, run crash, and room wallpaper.
- The original `lbRelocInitSetup` / `lbRelocLoadFilesListed` call position and
  file-list order inside imported `mvOpeningRoomFuncStart`.
- The original task heap ownership pattern: file payloads are copied into the
  active taskman heap rather than into a separate DS gameplay cache.

Adapted for DS:

- The Makefile stages the current eight O2R files into NitroFS under
  `build/nitrofs/reloc/...` and embeds them into `smash64ds.nds`.
- `src/nds/nds_reloc_assets.c` initializes NitroFS, parses the O2R container
  header, and copies the decompressed big-endian N64 payload bytes into the
  caller-provided task heap.
- `src/port/scene_backend.c` now sizes Opening Room allocation from real O2R
  payload headers, aligns each payload, and records header/payload masks,
  loaded byte count, last loaded file ID, and last payload size.
- The taskman arena is temporarily 1 MiB so the current 329,248 bytes of
  Opening Room payload data fit while the final memory map is still deferred.
- The visible melonDS HUD reports NitroFS init, header reads, payload reads,
  file/header/payload masks, byte totals, and fixup state.

Still parked:

- Blanket O2R word byte-swap and later internal pointer relocation; later
  milestones still need mixed-width renderer/game-usable struct fixups.
- External relocation dependency loading.
- Broad `ll...` symbol-offset coverage through `ndsRelocGetFileData`; later
  milestones add selected probes only.
- Texture/sprite/display-list fixups needed before tick `280` can dereference
  the movie asset data.
- Fighter/effect initialization, audio, and drawing.

Verification:

- `make -j4` builds `smash64ds.nds` with embedded NitroFS assets.
- `scripts/verify-runtime.ps1` requires NitroFS init marker `0x4E465349`,
  header reads `>= 16`, payload reads `8`, zero open/format/short-read
  failures, file/header/payload masks `0xFF`, data-ready `1`, fixup-ready `0`,
  loaded byte count `329248`, last file ID `90`, and last size `158928`.
- `scripts/verify-opening-skip.ps1` requires the same O2R diagnostics before
  proving the original A-button Opening Room transition to Title.
- `scripts/capture-melonds.ps1 -Build` captures the real emulator window and
  shows the same HUD values by eye.

## 2026-06-19: Opening Room internal pointer and symbol probe milestone

Reused:

- BattleShip's original relocation chain format from `lb/lbreloc.c`: each
  internal descriptor word stores the next descriptor in the upper half and the
  in-file target word offset in the lower half.
- BattleShip's generated US relocation offsets for selected Opening Room
  symbols, including room background, sunlight display list, transition overlay
  display list, scene 1 camera animation, and the room wallpaper sprite.
- The existing original `lbRelocLoadFilesListed` call position and file order;
  the DS backend performs the fixup immediately after copying each staged O2R
  payload into the task heap.

Adapted for DS:

- `src/port/scene_backend.c` now tracks the loaded Opening Room O2R ranges,
  walks each file's internal relocation chain, and writes native ARM9 pointers
  into the original 4-byte pointer slots. This entry predated the DS blanket
  word byte-swap milestone below; current code reads the chain as native words
  after the endian pass, matching BattleShip's PC bridge ordering.
- The eight staged O2R files report pointer-fixup mask `0xFF`, slot count
  `711`, and failure count `0`.
- `ndsRelocGetFileData` now resolves selected marker-symbol addresses and
  direct numeric offsets to `payload_base + offset` for registered O2R ranges,
  while preserving the older startup-logo fallback for unregistered stub files.
- The verifier probes five selected `ll...` offsets and expects symbol probe
  mask `0x1F`, resolve count `5`, failure count `0`, and last offset `158856`.
- The visible melonDS HUD includes `ptr=ff/711 sym=1f/5`.

Still parked:

- Remaining mixed-width struct fixups needed after the blanket `u32` word
  byte-swap before ARM9 code can safely dereference typed payload data.
- External relocation dependency recursion for files that reference other
  reloc files.
- Broad generated `ll...` symbol coverage beyond the selected Opening Room
  probes.
- Texture/sprite/display-list fixups and object-display rendering.
- Tick `280` object creation, fighter/effect initialization, audio, and draw.

Verification:

- `make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` requires pointer mask `0xFF`, pointer count
  `711`, pointer failures `0`, symbol resolve count `5`, symbol failures `0`,
  symbol probe mask `0x1F`, last symbol offset `158856`, and still requires
  full fixup-ready `0`.
- `scripts/verify-opening-skip.ps1` requires the same pointer/symbol
  diagnostics before proving the original A-button transition to Title.
- `scripts/capture-melonds.ps1 -Build` captured the live HUD showing
  `ptr=ff/711 sym=1f/5`.

## 2026-06-19: Opening Room blanket word byte-swap milestone

Reused:

- BattleShip PC bridge sequencing from `port/bridge/lbreloc_bridge.cpp`:
  copied relocation payloads are byte-swapped before the relocation chain walk.
- BattleShip byteswap bridge pass 1 from `port/bridge/lbreloc_byteswap.cpp`:
  every complete `u32` word in the staged blob is converted from N64
  big-endian order to native little-endian order before typed fixups run.
- The existing original Opening Room `lbRelocLoadFilesListed` call position,
  file order, and eight NitroFS-staged O2R payloads.

Adapted for DS:

- `src/port/scene_backend.c` now applies the blanket word byte-swap to each
  registered Opening Room O2R payload immediately after NitroFS load and before
  internal pointer relocation.
- Internal relocation-chain parsing now reads native `u32` descriptor words,
  matching the bridge's post-swap interpretation.
- New diagnostics track word-swap mask, count, and failures. The current eight
  staged files report mask `0xFF`, count `82312`, and failure count `0`.
- The visible melonDS HUD now shows `swap=ff/82312 fail=0` and adds center
  bars for word-swap, pointer, and symbol readiness.
- `scripts/capture-melonds.ps1` now waits 12 seconds by default so captures
  reach the verified tick-279 Opening Room boundary.

Still parked:

- Mixed-width struct fixups for data that the blanket `u32` pass intentionally
  does not make safely dereferenceable.
- Texture, sprite, vertex, and display-list fixups from the later BattleShip
  byteswap bridge passes.
- External relocation dependency recursion and broad generated symbol coverage.
- Tick `280` object creation, fighter/effect initialization, audio, and draw.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` now waits 12 seconds and requires word-swap mask
  `0xFF`, word count `82312`, failure count `0`, pointer mask `0xFF`, pointer
  count `711`, symbol probe mask `0x1F`, and full fixup-ready `0`.
- `scripts/verify-opening-skip.ps1` requires the same word-swap diagnostics
  before proving the original A-button transition to Title.
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-153052.png`, showing `tick=279`, `pre=ORPA`,
  `swap=ff/82312 fail=0`, `ptr=ff/711`, and `sym=1f/5`.

## 2026-06-19: Opening Room first tick-280 asset-reference probe milestone

Reused:

- BattleShip's original `mvOpeningRoomFuncRun` event schedule: the next normal
  no-input event after the current parking point is tick `280`.
- BattleShip's original tick-280 object creation targets:
  `mvOpeningRoomMakePulledFighter(sMVOpeningRoomPulledFighterKind)` and
  `mvOpeningRoomMakePencils()`.
- BattleShip's generated US `MVCommon` relocation offsets for the pencils data:
  `llMVCommonRoomPencilsDObjDesc` at `0x0AEB8` (`44728`) and
  `llMVCommonRoomPencilsAnimJoint` at `0x0AF70` (`44912`).

Adapted for DS:

- `include/reloc_data.h` exposes the two `MVCommon` pencils marker symbols to
  imported source and the DS relocation backend.
- `src/port/scene_backend.c` extends the selected relocation probes from five
  to seven. The original wallpaper symbol remains the last probe, so the last
  symbol offset stays `158856`, while the total probe mask advances to `0x7F`.
- New first-event diagnostics record `ORF1`, tick `280`, readiness mask `0x3`,
  pencils DObj offset `44728`, and pencils animation offset `44912`.
- `src/nds/nds_platform.c` adds a real melonDS HUD readiness bar and bottom
  text line for the first-event state: `event=4f524631 tick=280 mask=03`.

Still parked:

- The DS backend does not call `mvOpeningRoomMakePencils()` or
  `mvOpeningRoomMakePulledFighter()` yet.
- Tick-280 object creation still needs mixed-width struct fixups, external
  dependency handling, object-display/display-list support, texture fixups,
  fighter/effect initialization, audio, and draw boundaries.
- The copied O2R payloads are still not treated as fully renderer/game-usable
  data; this milestone proves the immediate asset references resolve only.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with symbol resolve count `7`, symbol
  probe mask `0x7F`, first-event marker `0x4F524631`, first-event tick `280`,
  first-event mask `0x3`, pencils DObj offset `44728`, and pencils animation
  offset `44912`.
- `scripts/verify-opening-skip.ps1` passed with the same first-event probes
  while still proving the original callback-time A transition to Title.
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-154118.png`.

## 2026-06-19: Opening Room pencils descriptor-shape probe milestone

Reused:

- BattleShip's original `mvOpeningRoomMakePencils()` dependency order:
  `gcSetupCommonDObjs` consumes `llMVCommonRoomPencilsDObjDesc`, then
  `gcAddAnimJointAll` consumes `llMVCommonRoomPencilsAnimJoint`.
- BattleShip's original object setup semantics from `sys/objanim.c`:
  `gcSetupCommonDObjs` walks `DObjDesc` entries until terminator id
  `DOBJ_ARRAY_MAX` (`18`), while `gcAddAnimJointAll` consumes one animation
  pointer per live DObj.
- The generated `52_MVCommon.c` typed dump for the pencils slice:
  `RoomPencilsDObjDesc` is four entries including the terminator, and
  `RoomPencilsAnimJoint` is a three-entry pointer table.

Adapted for DS:

- `src/port/scene_backend.c` now validates the resolved pencils data shape
  after word byte-swap, internal pointer relocation, and selected symbol
  resolution complete.
- The new `ORFD` marker requires:
  `DObjDesc[0..3].id == {0, 1, 1, 18}`, three in-payload display-list pointers,
  three in-payload animation pointers, and first animation opcode `3`
  (`nGCAnimEvent32SetValBlock`).
- `src/nds/nds_platform.c` adds a second first-event HUD bar and bottom-screen
  diagnostic line:
  `edata=4f524644 m=0f d=4 dl=3 a=3 op=3`.

Still parked:

- The DS backend still does not call `mvOpeningRoomMakePencils()` or
  `mvOpeningRoomMakePulledFighter()`.
- The display-list pointers are proven to be valid in-payload pointers, not
  renderable DS GPU data.
- Fighter setup, effect setup, texture/display-list conversion, draw execution,
  and tick-280 object creation remain deferred.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `ORFD`, data mask `0xF`, four
  DObjDesc entries, three DObj display-list pointers, three animation table
  pointers, and first animation opcode `3`.
- `scripts/verify-opening-skip.ps1` passed with the same data-shape proof while
  still proving the original callback-time A transition to Title.
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-155202.png`.

## 2026-06-19: Original Opening Room pencils object-creation probe

Reused:

- BattleShip's original `sys/objanim.c` and `sys/interp.c` are now imported via
  `src/import/battleship_sys_objanim.c` and
  `src/import/battleship_sys_interp.c`.
- The original DObj tree walker, common DObj setup, animation-joint attachment,
  first-frame animation playback, and interpolation helpers now come from the
  decomp source instead of DS stubs.
- The NDS Opening Room entry slice now exposes the original
  `mvOpeningRoomCommonProcUpdate` and exact `mvOpeningRoomMakePencils` body so
  the DS seam can exercise that first non-fighter object path without pulling
  the full fighter-heavy tick-280 event.

Adapted for DS:

- `include/macros.h` gained the narrow original macro compatibility needed by
  `objanim.c`/`interp.c`: `CUBE`, `ABS`, `TAKE_MAX`, and `F_CST_DTOR32`.
- `src/port/scene_backend.c` removed the temporary `gcGetTreeDObjNext` stub and
  added a bounded post-ORFD probe that calls original
  `mvOpeningRoomMakePencils` after the tick-279 park point.
- `gcDrawDObjTreeForGObj` remains a parked `sys/objdisplay.c` callback stub.
  The original pencils function stores it on the real GObj, but draw execution
  is still disabled.
- New `ORPC` diagnostics prove the original object path created one GObj, three
  DObjs, six XObjs, zero AObjs for the first `SetValBlock` slice, one process
  link, one display callback link, a three-node DObj tree, and one animation
  root.
- The visible melonDS HUD now prints:
  `pencil=4f525043 m=3f g=1 d=3 x=6` and
  `pencil a=0 p=1 dl=1 t=3 r=1`.

Still parked:

- The full tick-280 event still does not run. `mvOpeningRoomMakePulledFighter`
  remains guarded because it needs fighter manager, model/part data, effects,
  and display-list/rendering boundaries.
- Object-display execution, display-list translation, texture conversion,
  external relocation dependency recursion, mixed-width asset fixups, audio,
  and draw-safe object data remain deferred.
- The copied O2R display-list pointers are valid pointers for original object
  setup, but they are not renderable DS GPU commands yet.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `ORPC`, pencils mask `0x3F`, GObj
  delta `1`, DObj delta `3`, XObj delta `6`, AObj delta `0`, process/display
  set to `1`, DObj tree count `3`, and animation root count `1`.
- `scripts/verify-opening-skip.ps1` passed, proving the callback-time A path
  still transitions to Title before the normal Opening Room object boundary
  runs.
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-161014.png`.

## 2026-06-19: Renderer import cleanup and boundary reaffirmation

Reused:

- The existing original `mnstartup.c`, `sys/taskman.c`, `sys/objman.c`,
  `sys/objhelper.c`, `sys/objanim.c`, `sys/interp.c`, and Opening Room slice
  imports remain the verified runtime path.
- The current DS `syTaskmanRunTask` seam and parked display callbacks continue
  to preserve original task/object setup without entering draw.

Replaced or deferred:

- Removed the premature `battleship_sys_matrix.c` and
  `battleship_sys_objdisplay.c` wrappers from the explicit Makefile source
  list.
- Restored the narrow `sys/objdisplay.c` compatibility stubs in
  `src/port/scene_backend.c`: `gcInitDLs`, `gcSetCameraMatrixMode`,
  `gcSetMatrixFuncList`, `gcCaptureCameraGObj`, `func_80017DBC`, and
  `gcDrawDObjTreeForGObj`.
- A diagnostic-only `gcDrawAll` traversal was evaluated during the turn but
  removed before handoff because it was a temporary probe, not a maintained
  original-code boundary.
- Documented that a whole-file `sys/objdisplay.c` import is too broad for the
  current milestone because it pulls matrix look-at helpers, broad GBI
  texture/render macros, sprite helpers, camera capture, framebuffer/depth
  commands, and renderer contracts that do not exist yet.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with the current Opening Room pencils
  diagnostics and `Runtime verification passed (402 frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-163236.png`.

## 2026-06-19: Startup import verification re-run

Reconfirmed:

- `mnStartupStartScene` is linked from imported BattleShip `mnstartup.c`; the
  DS backend no longer provides a startup-scene stub.
- The verified startup path remains original `syTaskmanStartTask`,
  `gcSetupObjman`, `syTaskmanLoadScene`, and `mnStartupFuncStart`, with the DS
  boundary at `syTaskmanRunTask`.
- The current ROM is past the old scene-manager boundary and continues into the
  imported Opening Room slice before parking at the documented tick-280
  fighter/render boundary.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (401
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-163736.png`.

## 2026-06-19: Opening Room tick-280 update boundary

Reused:

- The imported Opening Room entry/update slice remains the source of truth for
  the movie timer, controller skip checks, fighter-kind selection, and pencils
  object creation.
- Original `mvOpeningRoomMakePencils` is now called from inside
  `mvOpeningRoomFuncRun` when the scene timer reaches tick `280`.
- The backend still only measures real object-manager deltas after the update;
  it no longer calls the pencils creator out-of-band.

Adapted for DS:

- Added maintained diagnostics for the first tick-280 update boundary:
  `OR28` (`gNdsOpeningRoomFirstEventRunResult`), deferred mask `0x0B`, and
  `ORFF` (`gNdsOpeningRoomFighterDeferredResult`).
- The NDS slice explicitly defers `mvOpeningRoomMakePulledFighter` plus the
  logo and boss-shadow ejections because the current scene slice has not
  imported the fighter manager, related setup objects, or render-safe
  object-display data. The overlay ejection is now covered by the following
  overlay setup/ejection milestone.
- The DS taskman seam now advances Opening Room to tick `280`, captures
  pre-event object counts at tick `279`, records the update-created pencils
  GObj/DObj/XObj deltas, and then parks.
- The visible melonDS HUD prints the new `evrun=4f523238 def=0b
  f=4f524646/...` diagnostic line.

Still parked:

- `mvOpeningRoomMakePulledFighter`, logo/boss-shadow ejection,
  object-display execution, fighter/effect setup, audio, and display-list
  translation.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (405
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-165148.png`.

## 2026-06-19: Opening Room overlay setup/ejection boundary

Reused:

- The original Opening Room logo-wallpaper overlay globals,
  `mvOpeningRoomMakeLogoWallpaper`, and `gcEjectGObj` call site remain in the
  imported `mvopeningroom.c` slice.
- The original object manager performs the overlay GObj allocation, common link,
  display-link attachment, and ejection/unlink/free-list return.

Adapted for DS:

- The NDS slice calls `mvOpeningRoomMakeLogoWallpaper` during
  `mvOpeningRoomFuncStart`, preserving the overlay alpha initialization and
  display callback/link setup while parking the N64 GBI/RDP draw body until a
  display-list translator exists.
- Tick `280` now executes original `gcEjectGObj` on the overlay GObj after the
  pencils creation boundary. The verifier records `OROC`/`OROE`, display-link
  readiness, alpha `255`, setup GObj count `3`, `gcGetGObjsActiveNum` samples
  `4`/`4`, and unlink mask `0x3`.
- The unlink mask scans the original object-manager common and display-link
  heads after ejection; this is the reliable proof because
  `gcGetGObjsActiveNum` includes the object free list.
- The visible melonDS HUD prints `ovl=4f524f43/4f524f45 m=03`.

Still parked:

- The overlay display callback's N64 display-list commands are not translated
  yet.
- `mvOpeningRoomMakePulledFighter`, logo/boss-shadow ejection,
  object-display execution, fighter/effect setup, audio, and display-list
  translation remain deferred.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (401
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-170257.png`.

## 2026-06-19: Opening Room boss-shadow setup/ejection boundary

Reused:

- The original Opening Room `mvOpeningRoomMakeBossShadow` object path now runs
  in the imported `mvopeningroom.c` slice.
- The original object manager performs the boss-shadow GObj allocation, DObj
  attachment, XObj transform setup, process link, display-link attachment,
  animation attachment, and ejection/unlink/free-list return.
- The original `gcPlayAnimAll` process is attached to the boss-shadow GObj.

Adapted for DS:

- The DS relocation backend now probes and resolves the BattleShip `MVCommon`
  boss-shadow marker symbols:
  `llMVCommonRoomBossShadowDisplayList` at offset `128912` and
  `llMVCommonRoomBossShadowAnimJoint` at offset `129316`.
- `gcDrawDObjDLHead1` remains a parked display callback stub. The original
  boss-shadow function stores it on the real GObj, but draw execution remains
  disabled until display-list translation exists.
- `ndsOpeningRoomCaptureBossShadowCreation` records `ORBC`, asset mask `0x3`,
  creation mask `0x3F`, setup GObj count `4`, object deltas `1/1/1/0`
  for GObj/DObj/XObj/AObj, and process/display/animation readiness.
- Tick `280` now executes original `gcEjectGObj` on the boss-shadow GObj after
  the overlay ejection. The verifier records `ORBE`, samples
  `gcGetGObjsActiveNum` as `5`/`5`, and proves the pointer leaves both original
  common and display-link lists with unlink mask `0x3`.
- The normal no-input path now resolves thirteen selected symbols/lookups:
  nine setup probes plus two original boss-shadow setup lookups and two
  original pencils lookups. The probe mask is `0x1FF`.
- The visible melonDS HUD prints
  `boss=4f524243/4f524245 m=03 c=3f`.

Still parked:

- `mvOpeningRoomMakePulledFighter`, tick-280 logo ejection, object-display
  execution, fighter/effect setup, audio, and display-list translation remain
  deferred.
- The boss-shadow display-list data is attached as original state, but not
  rendered.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (401
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-172529.png`.

## 2026-06-19: Opening Room logo setup/ejection boundary

Reused:

- The original Opening Room `mvOpeningRoomMakeLogo` object path now runs in the
  imported `mvopeningroom.c` slice.
- The original object manager performs the logo GObj allocation, DObj tree
  setup, XObj transform setup, MObj attachment, display-link attachment,
  material-animation attachment, and ejection/unlink/free-list return.
- Tick `280` now uses original `gcEjectGObj` for the logo GObj instead of
  treating logo ejection as a deferred branch.

Adapted for DS:

- The DS relocation backend now probes and resolves the BattleShip `MVCommon`
  logo marker symbols:
  `llMVCommonRoomLogoMObjSub` at offset `113760`,
  `llMVCommonRoomLogoDObjDesc` at offset `115880`, and
  `llMVCommonRoomLogoMatAnimJoint` at offset `116012`.
- `gcDrawDObjTreeDLLinksForGObj` remains a parked display callback stub. The
  original logo function stores it on the real GObj with display-link id `29`,
  but draw execution remains disabled until display-list translation exists.
- `ndsOpeningRoomCaptureLogoCreation` records `ORLC`, asset mask `0x7`,
  creation mask `0x3F`, setup GObj count `4`, object deltas `1/2/4/1/0` for
  GObj/DObj/XObj/MObj/AObj, and display/MObj/material-animation readiness.
- Tick `280` now executes original `gcEjectGObj` on the logo GObj before the
  overlay and boss-shadow ejections. The verifier records `ORLE`, samples
  `gcGetGObjsActiveNum` as `6`/`6`, and proves the pointer leaves both original
  common and display-link lists with unlink mask `0x3`.
- The first-event deferred mask is now `0x01`: only
  `mvOpeningRoomMakePulledFighter` remains guarded at the tick-280 event.
- The normal no-input path now resolves nineteen selected symbols/lookups:
  twelve setup probes plus three original logo setup lookups, two original
  boss-shadow setup lookups, and two original pencils lookups. The probe mask
  is `0xFFF`.
- The visible melonDS HUD prints
  `logo=4f524c43/4f524c45 m=03 c=3f` and
  `evrun=4f523238 def=01 f=4f524646/...`.

Still parked:

- `mvOpeningRoomMakePulledFighter`, object-display execution, fighter/effect
  setup, audio, full draw, and display-list translation remain deferred.
- The logo display-list, texture, and material data are attached as original
  object state, but are not rendered.
- External relocation dependency recursion and remaining mixed-width asset
  fixups are still required before broad asset consumers are safe.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (401
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-174358.png`.

## 2026-06-19: Opening Room logo-camera setup boundary

Reused:

- The original Opening Room `mvOpeningRoomMakeLogoCamera` path now runs in the
  imported `mvopeningroom.c` NDS slice before the logo-wallpaper, logo, and
  boss-shadow setup objects.
- The original object manager performs the camera GObj allocation, CObj
  attachment, two default camera XObj matrix attachments, process link, display
  callback attachment, and camera animation joint attachment.
- The original `gcPlayCamAnim` process is attached to the logo-camera GObj.

Adapted for DS:

- The NDS slice now includes BattleShip `sys/obj.h` so it can use the original
  `CObjGetStruct` accessor instead of a local pointer cast.
- The DS relocation backend now consumes the already probed
  `llMVOpeningRoomScene1CamAnimJoint` symbol through the original
  `lbRelocGetFileData` call. The expected offset is `0`.
- `syRdpSetViewport` now implements the original viewport scale/translate
  calculation used by the camera setup boundary. The verifier checks the
  resulting `600/440/640/480/511/511` viewport values.
- `func_80017EC0` remains a parked object-display callback stub. The original
  logo-camera function stores it on the real camera GObj, but draw execution
  remains disabled until display-list translation exists.
- `ndsOpeningRoomCaptureLogoCameraCreation` records `ORCM`, asset mask `0x1`,
  creation mask `0x7F`, setup GObj count `3`, object deltas `1/1/2/0` for
  GObj/CObj/XObj/AObj, and display/process/camanim/viewport readiness.
- The normal no-input path now resolves twenty selected symbols/lookups:
  twelve setup probes plus one original logo-camera camanim lookup, three
  original logo setup lookups, two original boss-shadow setup lookups, and two
  original pencils lookups. The probe mask remains `0xFFF`.
- Because the logo camera is now a real setup object, setup counts shift to
  overlay GObj count `4`, logo GObj count `5`, boss-shadow GObj count `6`,
  ejection samples `7`/`7`, and the Opening Room snapshot `6` GObjs / `2`
  cameras.
- The visible melonDS HUD now prints
  `lcam=4f52434d m=7f a=01 vp=1`.

Still parked:

- `mvOpeningRoomMakePulledFighter`, object-display execution, fighter/effect
  setup, audio, full draw, and display-list translation remain deferred.
- Camera animation is attached as original state, but the current boundary does
  not advance into render/camera-capture execution.
- External relocation dependency recursion and remaining mixed-width asset
  fixups are still required before broad asset consumers are safe.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (395
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-181528.png`.

## 2026-06-19: Opening Room wallpaper-camera setup boundary

Reused:

- The original Opening Room `mvOpeningRoomMakeWallpaperCamera` path now runs
  in the imported `mvopeningroom.c` NDS slice before the logo camera and setup
  objects.
- The original object manager performs the camera GObj allocation, CObj
  attachment, display callback link, and viewport setup.

Adapted for DS:

- The NDS slice declares the existing parked `lbCommonDrawSprite` callback
  locally instead of widening the include surface.
- `ndsOpeningRoomCaptureWallpaperCameraCreation` records `ORWC`, creation mask
  `0x1F`, setup GObj count `3`, object deltas `1/1/0` for GObj/CObj/XObj, and
  display/viewport readiness.
- Because the wallpaper camera is inserted in original setup order before the
  logo camera, later setup/ejection samples shift to logo-camera GObj count
  `4`, overlay GObj count `5`, logo GObj count `6`, boss-shadow GObj count
  `7`, ejection samples `8`/`8`, and the Opening Room snapshot `7` GObjs / `3`
  cameras.
- The visible melonDS HUD now prints
  `wcam=4f525743 m=1f vp=1`.

Still parked:

- `lbCommonDrawSprite` remains a parked display callback; the original camera
  is linked, but no sprite draw execution or DS display-list translation runs.
- `mvOpeningRoomMakePulledFighter`, object-display execution, fighter/effect
  setup, audio, full draw, and display-list translation remain deferred.

Verification:

- `make clean && make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (402
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-182333.png`.

## 2026-06-19: Opening Room Scene 1 camera setup boundary

Reused:

- The original Opening Room `mvOpeningRoomMakeScene1Cameras` path now runs in
  the imported `mvopeningroom.c` NDS slice before wallpaper-camera,
  logo-camera, overlay, logo, and boss-shadow setup.
- The original object manager performs both Scene 1 camera GObj allocations,
  both CObj attachments, four XObj matrix attachments, two display callback
  links, two `gcPlayCamAnim` process links, and two camanim attachments.
- The same `MVOpeningRoomScene1` camanim symbol remains the original asset
  source for these cameras and the later logo camera.

Adapted for DS:

- `mvOpeningRoomFuncStart` calls the original Scene 1 camera setup before the
  previously imported wallpaper/logo setup slice.
- `ndsOpeningRoomCaptureScene1CameraCreation` records `OR1C`, creation mask
  `0x1FF`, setup GObj count `4`, object deltas `2/2/4/0` for
  GObj/CObj/XObj/AObj, and display/process/camanim/viewport/DL-buffer
  readiness.
- The normal no-input path now resolves twenty-two selected symbols/lookups:
  twelve setup probes, two original Scene 1 camera camanim lookups, one
  original logo-camera camanim lookup, three original logo setup lookups, two
  original boss-shadow setup lookups, and two original pencils lookups. The
  skip verifier still samples twenty because it transitions to Title before the
  later tick-280 pencils lookups.
- Because the two Scene 1 cameras are now real setup objects, later setup and
  ejection samples shift to wallpaper camera GObj count `5`, logo camera GObj
  count `6`, overlay GObj count `7`, logo GObj count `8`, boss-shadow GObj
  count `9`, ejection samples `10`/`10`, and the Opening Room snapshot `9`
  GObjs / `5` cameras.
- The visible melonDS HUD now prints
  `s1cam=4f523143 m=1ff vp=1`.

Still parked:

- `func_80017EC0` and `gcPlayCamAnim` are attached as original camera state,
  but object-display execution, camera capture, display-list translation, and
  draw execution remain disabled.
- `mvOpeningRoomMakeCloseUpOverlayCamera`, close-up overlay, room geometry,
  fighter creation, effects, audio, remaining relocation fixups, and full draw
  are still deferred.

Verification:

- `make clean; make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (403
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-184135.png`.

## 2026-06-19: Opening Room close-up overlay camera setup boundary

Reused:

- The original Opening Room `mvOpeningRoomMakeCloseUpOverlayCamera` path now
  runs in the imported `mvopeningroom.c` NDS slice after the Scene 1 cameras
  and before the wallpaper/logo setup.
- The original object manager performs the camera GObj allocation, CObj
  attachment, display callback link, and viewport setup.

Adapted for DS:

- The NDS slice calls the original close-up overlay camera setup in its real
  setup order, while keeping the attached `lbCommonDrawSprite` callback parked
  until sprite draw/display-list translation exists.
- `ndsOpeningRoomCaptureCloseUpOverlayCameraCreation` records `ORCC`, creation
  mask `0x1F`, setup GObj count `5`, object deltas `1/1/0` for GObj/CObj/XObj,
  display callback readiness, and viewport readiness.
- The verifier checks the original display link priority `60`, camera mask
  `COBJ_MASK_DLLINK(26)`, and the same `10,10,310,230` viewport values used by
  the other overlay-style cameras.
- Because the close-up overlay camera is inserted before wallpaper and logo
  setup, later setup/ejection samples shift to wallpaper-camera GObj count
  `6`, logo-camera GObj count `7`, overlay GObj count `8`, logo GObj count
  `9`, boss-shadow GObj count `10`, ejection samples `11`/`11`, and the
  Opening Room snapshot `10` GObjs / `6` cameras.
- The visible melonDS HUD now prints
  `ccam=4f524343 m=1f vp=1`.

Still parked:

- The later close-up overlay object creation at tick `450`.
- `lbCommonDrawSprite` execution, sprite draw bodies, object-display execution,
  fighter/effect setup, audio, full draw, and display-list translation.
- External relocation dependency recursion and remaining mixed-width asset
  fixups are still required before broad asset consumers are safe.

Verification:

- `make clean; make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (400
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-185605.png`.

## 2026-06-19: Opening Room tick-450 close-up overlay object boundary

Reused:

- The original Opening Room `mvOpeningRoomMakeCloseUpOverlay` object-creation
  path now runs in the imported `mvopeningroom.c` NDS slice from inside
  `mvOpeningRoomFuncRun` at tick `450`.
- The original object manager performs the close-up overlay GObj allocation and
  display callback link on display link `26`.
- The original close-up overlay alpha state is initialized to `0`.

Adapted for DS:

- `mvOpeningRoomCloseUpOverlayProcDisplay` is present in the NDS slice with the
  original alpha-increment state update, but its N64 RDP fill-rectangle body
  remains parked until display-list/RDP translation exists.
- Tick `380` now records the original pulled-fighter status/rotation branch as
  explicitly deferred (`OR38`, mask `0x01`) because
  `mvOpeningRoomMakePulledFighter` is still guarded.
- Tick `450` now records the original sunlight ejection branch as explicitly
  deferred (`OR45`, mask `0x01`) because the room/sunlight object slice is not
  imported yet.
- `ndsOpeningRoomCaptureCloseUpOverlayCreation` records `ORCO`, creation mask
  `0x07`, tick `450`, GObj count `11`, common-list GObj delta `1`, display
  callback readiness, and alpha initialization `0`.
- The bounded Opening Room seam now runs `450` actor ticks and the verifier
  expects `441` original controller gate calls on ticks `10-450`.
- The visible melonDS HUD now prints
  `t380=4f523338 m=01 t450=4f523435 m=01` and
  `cuov=4f52434f m=07 a=0 g=11`.

Still parked:

- `mvOpeningRoomMakePulledFighter`, fighter status/rotation, room/sunlight
  objects, spotlight/tick-500 behavior, Scene 2/tick-560 behavior,
  object-display execution, audio, full draw, and display-list translation.
- The close-up overlay display callback is linked as original state but is not
  executed by `gcDrawAll` yet.
- External relocation dependency recursion and remaining mixed-width asset
  fixups are still required before broad asset consumers are safe.

Verification:

- `make clean; make -j4` builds `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (641
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-191346.png`.

## 2026-06-19: Opening Room tick-500 spotlight object boundary

Reused:

- The original Opening Room `mvOpeningRoomMakeSpotlight` object-creation path
  now runs in the imported `mvopeningroom.c` NDS slice from inside
  `mvOpeningRoomFuncRun` at tick `500`.
- The original object manager allocates the spotlight GObj, DObj, XObj, MObjs,
  display callback link, process link, material animation attachment, and
  fighter-kind-dependent spotlight position.
- The original `MVCommon` spotlight asset references are resolved through the
  DS relocation backend: display list offset `142872`, MObj offset `142480`,
  and material animation offset `143120`.

Adapted for DS:

- Tick `500` records the original pulled-fighter display-link move as
  explicitly deferred (`OR50`, mask `0x01`) because
  `mvOpeningRoomMakePulledFighter` is still guarded.
- `ndsOpeningRoomCaptureSpotlightCreation` records `ORSL`, creation mask
  `0xFF`, tick `500`, GObj count `11`, deltas of one GObj, one DObj, one XObj,
  two MObjs, and zero AObjs, plus display/process/MObj/material-animation/
  position readiness.
- The bounded Opening Room seam now runs `500` actor ticks and the verifier
  expects `491` original controller gate calls on ticks `10-500`.
- `scripts/verify-runtime.ps1` now expects `28` selected symbol resolutions,
  probe mask `0x7FFF`, and last symbol offset `143120` in the normal no-input
  path. The skip verifier expects the expanded setup probe mask `0x7FFF` and
  symbol count `23`.
- The visible melonDS HUD was compacted for the bottom screen: it clears the
  console each frame, shortens volatile lines, and displays `or50`,
  spotlight asset offsets, object deltas, process/display/material/position
  checks, tick count, controller checks, and input without wrapping.

Still parked:

- `mvOpeningRoomMakePulledFighter`, fighter status/rotation, room/sunlight
  objects, Scene 2/tick-560 behavior, object-display execution, audio, full
  draw, and display-list translation.
- The spotlight display callback is linked as original state but is not
  executed by `gcDrawAll` yet.
- External relocation dependency recursion and remaining mixed-width asset
  fixups are still required before broad asset consumers are safe.

Verification:

- `make clean; make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (701
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-194232.png`; visual inspection confirmed the
  bottom-screen debug text no longer wraps or leaves stale trailing text.

## 2026-06-19: Opening Room tick-560 Scene 2 camera boundary

Reused:

- The original Opening Room tick-560 camera transition now runs in the imported
  `mvopeningroom.c` NDS slice.
- Original `mvOpeningRoomEjectCameraGObjs` ejects the previous Scene 1 main and
  fighter camera GObjs.
- Original `mvOpeningRoomMakeScene2Cameras` creates the next main/fighter
  camera pair using the imported object-helper/object-manager code.
- The original `MVOpeningRoomScene2` camanim reference
  `llMVOpeningRoomScene2CamAnimJoint` resolves through the DS relocation
  backend at offset `0`.

Adapted for DS:

- Tick `560` records the original Boss fighter status call as explicitly
  deferred (`OR56`, mask `0x01`) because Boss fighter creation is still outside
  this slice.
- `ndsOpeningRoomRecordScene2CameraEject` verifies both old Scene 1 camera
  GObjs leave the original common lists and two CObjs return to the camera pool
  (`OR2E`, mask `0x07`).
- `ndsOpeningRoomCaptureScene2CameraCreation` verifies the original Scene 2
  camera setup (`OR2C`, mask `0x1FF`): two camera GObjs, two CObjs, four XObjs,
  two `func_80017EC0` display links, two `gcPlayCamAnim` processes, two camanim
  attachments, original viewport values, and DL-buffer flags.
- The bounded Opening Room seam now runs `560` actor ticks and the verifier
  expects `551` original controller gate calls on ticks `10-560`.
- `scripts/verify-runtime.ps1` now expects `31` selected symbol resolutions,
  probe mask `0xFFFF`, and final normal-path symbol offset `0`. The skip
  verifier expects setup symbol count `24` and probe mask `0xFFFF`.
- The visible melonDS HUD now shows compact OR560 lines:
  `or56=4f523536 d=01 e=07` and `s2=1ff a=01 g=2 c=2 x=4`. The bottom console
  remains cleared every frame and visually fits without wrapping.

Still parked:

- `mvOpeningRoomMakePulledFighter`, `mvOpeningRoomMakeBoss`, later fighter
  status updates, room/sunlight objects, object-display execution, audio, full
  draw, and display-list translation.
- Scene 2 camera display callbacks are linked as original state but are not
  executed by `gcDrawAll` yet.
- External relocation dependency recursion and remaining mixed-width asset
  fixups are still required before broad asset consumers are safe.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (705
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-200242.png`; visual inspection confirmed the
  OR560 bottom-screen debug text stays inside the visible DS console.

## 2026-06-19: Opening Room sunlight object/ejection boundary

Reused:

- The original `MVCommon` sunlight display-list reference
  `llMVCommonRoomSunlightDisplayList` now resolves through the DS relocation
  backend at offset `149256`.
- A project-owned wrapper around the imported `mvopeningroom.c` creates a real
  sunlight GObj using the original object-manager calls:
  `gcMakeGObjSPAfter`, `gcAddDObjForGObj`, `gcAddXObjForDObjFixed`, and
  `gcAddGObjDisplay`.
- The tick-450 path now calls original `gcEjectGObj` on that real sunlight
  GObj after original `mvOpeningRoomMakeCloseUpOverlay` runs.

Adapted for DS:

- `decomp/` is treated as read-only upstream source, so the DS hook lives in
  `src/import/battleship_mvopeningroom.c`. The wrapper renames the imported
  base `mvOpeningRoomFuncStart`, `mvOpeningRoomFuncRun`, and
  `mvOpeningRoomStartScene` symbols, then exposes project-owned functions with
  the original ABI.
- `ndsOpeningRoomCaptureSunlightCreation` records `ORSC`, asset mask `0x01`,
  display-list offset `149256`, creation mask `0x0F`, setup GObj count `11`,
  one GObj/DObj/XObj delta each, and display-link readiness.
- `ndsOpeningRoomRecordSunlightEject` records `ORSE`, verifies the common and
  display-list unlink mask `0x03`, and updates the tick-450 deferred mask to
  `0x00`.
- Existing free-list-inclusive GObj samples shifted by one real setup object:
  close-up overlay, spotlight, Scene 2 camera, overlay/logo/boss-shadow
  ejection checks now expect `12` where applicable.
- The visible melonDS HUD stayed compact by replacing the old bottom-screen
  tick-450 line with `or45=4f523435 d=00 su=03` and the object mask line
  `obj su=0f ov=03 pn=3f co=07`.

Still parked:

- Full room-object setup ordering is not complete; this wrapper proves the
  sunlight object/ejection boundary without editing upstream decomp source.
- The sunlight display callback is linked as original object-manager state but
  is not executed by `gcDrawAll` yet.
- Fighter creation, remaining room events, texture/display-list fixups, draw,
  audio, and full asset usability remain deferred.

Verification:

- `make clean; make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (701
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-203724.png`; visual inspection confirmed the
  bottom-screen debug text stays inside the visible DS console.

## 2026-06-19: Opening Room Outside object boundary

Reused:

- The original `MVCommon` Outside display-list reference
  `llMVCommonRoomOutsideDisplayList` now resolves through the DS relocation
  backend at offset `147968`.
- The project-owned wrapper around the imported `mvopeningroom.c` creates a
  real Outside GObj using the same original object-manager calls as the
  full BattleShip function: `gcMakeGObjSPAfter`, `gcAddDObjForGObj`,
  `gcAddXObjForDObjFixed`, and `gcAddGObjDisplay`.
- The wrapper creates Outside before sunlight so these two wrapper-owned room
  display objects preserve the original relative order for this slice.

Adapted for DS:

- `decomp/` remains read-only. The new hook lives entirely in
  `src/import/battleship_mvopeningroom.c`, `src/port/scene_backend.c`,
  `include/nds/nds_startup.h`, and `include/reloc_data.h`.
- `ndsOpeningRoomCaptureOutsideCreation` records `OROU`, asset mask `0x01`,
  display-list offset `147968`, creation mask `0x0F`, setup GObj count `11`,
  one GObj/DObj/XObj delta each, and display-link readiness on link 6.
- The relocation probe list adds the Outside symbol as bit 16. The normal
  no-input verifier now expects `34` selected symbol resolutions and probe mask
  `0x1FFFF`; the skip verifier expects setup symbol count `27` and the same
  probe mask.
- Existing free-list-inclusive GObj samples shifted by one persistent setup
  object: sunlight creation now reports `12`, later ejection/setup samples
  that include the live room object report `13`, and the Opening Room setup
  snapshot reports `12` GObjs.
- The visible melonDS HUD keeps the bottom text compact with the object mask
  line `obj o=0f s=0f p=3f c=07`.

Still parked:

- Full room-object setup is not complete; this only proves the original Outside
  display-list object path through the DS backend.
- Outside and sunlight display callbacks are linked as original object-manager
  state but are not executed by `gcDrawAll` yet.
- Fighter creation, remaining room events, texture/display-list fixups, draw,
  audio, and full asset usability remain deferred.

Verification:

- `make clean; make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (700
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-204805.png`; visual inspection confirmed the
  bottom-screen debug text stays inside the visible DS console.

## 2026-06-19: Opening Room Haze object boundary

Reused:

- The original `MVCommon` Haze display-list reference
  `llMVCommonRoomHazeDisplayList` now resolves through the DS relocation
  backend at offset `39160`.
- The project-owned wrapper around imported `mvopeningroom.c` creates a real
  Haze GObj using the same original object-manager calls as the BattleShip
  `mvOpeningRoomMakeHaze` function: `gcMakeGObjSPAfter`, `gcAddDObjForGObj`,
  `gcAddXObjForDObjFixed`, and `gcAddGObjDisplay`.
- The wrapper now preserves the current simple room-object order as
  Outside -> Haze -> sunlight, with Background still deferred because it
  requires broader DObjDesc/MObj/MatAnim handling.

Adapted for DS:

- `decomp/` remains read-only. The hook lives in project-owned wrapper,
  backend, header, HUD, and verifier files only.
- Outside, Haze, and sunlight now use the original `gcDrawDObjDLLinksForGObj`
  display callback identity on display link 6. This corrects the previous
  wrapper callback for Outside/sunlight, which had used the tree-DL variant.
- `ndsOpeningRoomCaptureHazeCreation` records `ORHZ`, asset mask `0x01`,
  display-list offset `39160`, creation mask `0x0F`, setup GObj count `12`,
  one GObj/DObj/XObj delta each, and display-link readiness.
- The relocation probe list adds the Haze symbol as bit 17. The normal no-input
  verifier now expects `36` selected symbol resolutions and probe mask
  `0x3FFFF`; the skip verifier expects setup symbol count `29` and the same
  probe mask.
- Existing free-list-inclusive GObj samples shifted by one persistent setup
  object: sunlight creation now reports `13`, later ejection/setup samples
  that include the live room object report `14`, and the Opening Room setup
  snapshot reports `13` GObjs.
- The visible melonDS HUD keeps the bottom text compact with
  `obj h=0f o=0f s=0f c=07`.

Still parked:

- Full room-object setup is not complete; Background, Desk, Books, Lamp,
  Tissues, Boss, fighter/effect setup, object-display execution, draw,
  texture/display-list fixups, audio, and full asset usability remain deferred.
- Haze, Outside, and sunlight display callbacks are linked as original
  object-manager state but are not executed by `gcDrawAll` yet.

Verification:

- `make clean; make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (705
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live HUD at
  `artifacts/melonds-20260619-210224.png`; visual inspection confirmed the
  bottom-screen debug text stays inside the visible DS console.

## 2026-06-19: First original rendered asset boundary

Reused:

- Original `mn/mncommon/mnstartup.c` startup setup and draw callback path.
- Original startup `N64Logo` file ID `194` and `llN64LogoSprite` symbol offset
  `0x73c0`.
- Original object-manager SObj/GObj links created by `mnStartupFuncStart`.
- Original draw dispatch identity:
  `mnStartupFuncDraw -> gcDrawAll -> lbCommonDrawSprite ->
  gcCaptureCameraGObj -> lbCommonDrawSObjAttr`.

Adapted for DS:

- Added the `reloc_misc_named/N64Logo` O2R payload to NitroFS and the narrow
  DS relocation manifest.
- `lbRelocGetExternHeapFile` now loads that startup O2R payload into the
  original task heap, applies the blanket word byte-swap, patches its nine
  internal relocation pointers, resolves `llN64LogoSprite`, and records
  `LRLC`.
- Added an asset-specific Sprite/Bitmap halfword normalizer for `N64Logo`.
  The blanket `u32` word-swap fixed pointers/floats, but left 16-bit metadata
  lanes swapped; before the normalizer, the sprite attr read from the z-depth
  halfword and looked hidden.
- Added narrow `gcCaptureCameraGObj`, `lbCommonDrawSprite`, and
  `lbCommonDrawSObjAttr` behavior just far enough to traverse the startup
  camera/display links, honor `SP_HIDDEN`, and render this one RGBA16 Sprite.
- Added a DS preview buffer in `src/nds/nds_platform.c` that converts N64
  RGBA5551 texels into DS RGB15 and overlays the result on the top screen.

Still parked:

- General mixed-width relocation struct fixups.
- General `lbcommon.c`, `sys/objdisplay.c`, RSP/RDP display-list translation,
  texture cache/conversion, and continuous `gcDrawAll`.
- Opening Room object drawing, fighters, stages, battle gameplay, audio, and
  full title/menu systems.

Verification:

- `scripts/verify-runtime.ps1` now checks startup logo relocation size
  `29712`, word-swap count `7428`, pointer fixup count `9`, bounded draw
  marker `0x4C445257`, blocker `0`, update `17`, dimensions `128x108`, RGBA16
  format/size `0`/`2`, eight bitmap chunks, and converted pixels.
- `make clean`, `make -j4`, `scripts/verify-runtime.ps1`, and
  `scripts/verify-opening-skip.ps1` pass.
- `scripts/capture-melonds.ps1 -Build -DelaySeconds 32` captured the live
  emulator at `artifacts/melonds-20260619-214703.png`; visual inspection shows
  the original N64 logo sprite on the top screen and clean bottom-screen debug
  text.

## 2026-06-19: Bounded Opening Room draw blocker

Reused:

- Original `mv/mvopening/mvopeningroom.c` remains the scene source of truth.
- Original `gcDrawAll` traversal is used for the draw probe.
- Original camera display callbacks created by `mvOpeningRoomMakeScene1Cameras`
  and `mvOpeningRoomMakeScene2Cameras` run through the original
  `func_80017EC0` identity.
- Original display-link-6 room objects created from `MVCommon` references are
  captured by the camera bridge.
- Original `gcDrawDObjDLLinksForGObj` is the first DObj display callback
  reached by the bounded Opening Room probe.

Adapted for DS:

- `decomp/` stayed read-only; the work lives in project-owned compatibility
  code, diagnostics, verifier, and docs.
- `src/port/scene_backend.c` now runs one bounded Opening Room
  `scene_draw`/`gcDrawAll` call after the verified tick-560 update boundary.
  It does not unbound the task loop.
- `func_80017EC0` now records camera callback evidence and bridges to the
  narrow DS `gcCaptureCameraGObj` implementation instead of acting as a pure
  no-op.
- The DObj display callback stubs record the first exact renderer blocker
  without pretending to render unsupported N64 display lists.
- `scripts/verify-runtime.ps1` now checks `ORDW`, blocker `3`, tick `560`,
  frame `0`, camera/display/DObj callback counts, first camera mask/priority,
  display link `6`, object kind `1`, callback `0x444C4E4B`, nonzero DObj
  display-list pointer, and DObj meta `0x11`.
- The live melonDS HUD uses the compact line
  `draw=4f524457 b=3 c=3 d=5`, which fits the bottom screen without wrapping.

Still parked:

- The first visible original asset is still the Startup `N64Logo` Sprite. The
  Opening Room probe reaches a real original DObj display-list pointer but does
  not render it yet.
- DS display-list translation for `gcDrawDObjDLLinksForGObj` is the next exact
  renderer blocker.
- Continuous draw looping, broad `sys/objdisplay.c` import, fighter/stage/menu
  systems, audio, and full title/menu rendering remain deferred.

Verification:

- `make clean` and `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (750
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live emulator at
  `artifacts/melonds-20260619-220937.png`; visual inspection confirmed the
  bounded draw diagnostics are visible and the bottom-screen text stays inside
  the DS console.

## 2026-06-19: Bounded Opening Room DObj DL preview

Reused:

- Original `mv/mvopening/mvopeningroom.c` remains the source of the scene,
  object, camera, and draw callback state.
- The draw path still enters original
  `scene_draw -> gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj`.
- The selected asset is the first original display-link-6 DObj reached by the
  bounded Opening Room draw probe. The preview uses the actual
  `DObjDLLink->dl` pointer, not the DObj union/base pointer.

Adapted for DS:

- Added a narrow top-screen preview buffer in `src/nds/nds_platform.c` for one
  bounded original DObj display-list slice.
- Added a deliberately small display-list preview interpreter in
  `src/port/scene_backend.c`. It skips RDP setup commands, decodes the first
  `G_VTX` loads after undoing the current word-swap lane order, supports
  `G_TRI1`, `G_TRI2`, and `G_ENDDL`, and rasterizes flat diagnostic triangles.
- `scripts/verify-runtime.ps1` now samples and asserts `ORDP`, blocker `0`,
  first opcode `0xE7`, unsupported opcode `0`, nonzero first DL pointer, at
  least ten commands, at least three vertices, at least one triangle, and
  nonzero pixels.
- The visible HUD was trimmed to keep the bottom screen clean and now includes
  `dlp=4f524450 t=8 p=5138` above the existing bounded draw line.

Still parked:

- This is not a full `sys/objdisplay.c` import or a general RSP/RDP backend.
- The preview ignores real camera matrices, material state, textures, z-buffer
  behavior, combiner modes, clipping, and full display-list branching.
- Fighters, stages, battle gameplay, audio, full title/menu systems,
  continuous draw, and broad `lbcommon`/object-display dependencies remain
  deferred.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (875
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live emulator at
  `artifacts/melonds-20260619-224223.png`; visual inspection confirmed the
  bordered Opening Room DObj preview is visible on the top screen and the
  bottom-screen debug text stays inside the visible DS console.

## 2026-06-19: Opening Room DObj transform-aware preview

Reused:

- Original `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` remains the bounded draw path.
- The selected asset is still the first original display-link-6 DObj reached
  by the Opening Room draw probe, and the display-list source is still the
  original `DObjDLLink->dl` pointer.
- Original object-manager DObj/XObj state is now sampled before preview
  projection instead of assuming a port-side transform.

Adapted for DS:

- `src/port/scene_backend.c` now records transform diagnostics for the preview:
  transform mask, XObj count, first XObj kind, DObj translate/scale in
  centi-units, and transformed preview bounds.
- The bounded preview applies sampled DObj translate/scale to decoded vertices
  before the top-screen diagnostic projection. Rotation, camera matrices,
  material state, and texture state remain deferred to the full renderer
  boundary.
- `scripts/verify-runtime.ps1` now asserts transform mask `0x1F`, first XObj
  kind `28` (`nGCMatrixKindTraRotRpyRSca`), sampled rotation `0,0,0`, scale
  `100,100,100`, and nonzero transformed bounds in addition to the existing
  `ORDP` display-list parse and pixel checks.

Still parked:

- This is still not a general `gcPrepDObjMatrix` implementation or a broad
  `sys/objdisplay.c` import.
- Camera-correct matrices, rotations, MObj/material handling, texture upload,
  z-buffer behavior, display-list branching, and continuous draw remain
  deferred.
- Fighters, stages, battle gameplay, audio, and full title/menu systems remain
  outside this milestone.

Verification:

- `make clean` and `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (875
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live emulator at
  `artifacts/melonds-20260619-225810.png`; visual inspection confirmed the
  bordered Opening Room DObj preview remains visible and the bottom-screen
  debug text stays inside the visible DS console.

## 2026-06-19: Opening Room DObj preview presentation proof

Reused:

- The selected renderer path remains the bounded original
  `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.
- The display-list source remains the first original link-6 `DObjDLLink->dl`.
- The RPY transform math mirrors BattleShip's `syMatrixTraRotRpyRScaF` row-scale
  convention for the current first XObj kind `28`.

Adapted for DS:

- The bounded preview now applies sampled RPY rotation as well as translate and
  scale before diagnostic projection.
- `src/nds/nds_platform.c` exposes retained preview presentation diagnostics:
  ready, width, height, commit count, and draw count.
- The platform draw path keys the top-screen preview off retained presentation
  state, and `scripts/verify-runtime.ps1` now asserts the preview was both
  committed and drawn.

Still parked:

- This is still a diagnostic preview, not a full `gcPrepDObjMatrix` import or a
  general RSP/RDP backend.
- Nonzero rotations, camera-correct projection, MObj/material state, texture
  upload, z-buffer behavior, and display-list branching remain renderer
  milestones.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (876
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build -DelaySeconds 45` captured the live
  emulator at `artifacts/melonds-20260619-231447.png`; visual inspection
  confirmed the retained bounded preview is visible and the bottom-screen debug
  text remains inside the visible DS console.

## 2026-06-19: Opening Room texture-boundary diagnostics and HUD stabilization

Reused:

- The renderer path remains the bounded original
  `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.
- The selected display-list source remains the first original link-6
  `DObjDLLink->dl` reached by the Opening Room draw probe.
- The startup logo source remains the original `N64Logo` Sprite payload loaded
  through the bounded startup draw path.

Adapted for DS:

- `src/port/scene_backend.c` now records narrow texture setup diagnostics while
  parsing that first display list: set-combine, set-tile, texture enable,
  tile-size, texture-image, and load-block commands. The verifier expects
  texture mask `0x3F`, RGBA16 format/size `0/2`, image width field `1`,
  render tile size `96x32`, `1024` load-block texels, at least two
  `G_SETTILE` commands, and combine words `0xFC127E24,0xFFFFF3F9`.
- `scripts/verify-runtime.ps1` prints and asserts the new texture-boundary
  globals so the next renderer blocker is explicit instead of a one-off GDB
  note.
- `src/nds/nds_platform.c` now double-buffers the top framebuffer for the
  melonDS debug HUD, prefilters the downscaled startup logo preview once at
  commit time, and throttles bottom-screen console redraws to reduce visible
  flashing without weakening the GDB verifier.

Still parked:

- Texture sampling/upload, combiner behavior, material animation, camera-
  correct projection, z-buffer behavior, display-list branching, and continuous
  draw remain renderer milestones.
- The filtered `N64Logo` presentation is a debug preview of the original
  Sprite, not a final DS-native UI or replacement menu renderer.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1272
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 41 -> Title).`
- `scripts/capture-melonds.ps1 -Build -DelaySeconds 32` captured the live
  emulator at `artifacts/melonds-20260619-234232.png`; visual inspection
  confirmed the original startup logo preview is smoother, the bounded Opening
  Room DObj preview remains visible, and the bottom-screen debug text stays
  inside the visible DS console.

## 2026-06-19: Opening Room bounded texture-sampled DObj preview

Reused:

- The renderer entry path remains the bounded original
  `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.
- The selected source remains the first original Opening Room link-6
  `DObjDLLink->dl` captured by the draw probe.
- The texture source is the original `G_SETTIMG` image pointer reached by that
  display list after the staged O2R payload load and current relocation fixes.

Adapted for DS:

- `src/port/scene_backend.c` now decodes the first bounded vertex batch in the
  native halfword order used by this loaded payload, preserving `s/t`
  coordinates for the software preview.
- The narrow preview derives the current RGBA16 physical texture layout from
  the render tile line and load-block texel count. For this first list the
  verified layout is `32x32`.
- The preview now samples that one RGBA16 texture while rasterizing the eight
  parsed triangles. The DS-side path wraps S, clamps T, compensates for the
  blanket word-byte-swap halfword pair order, and falls back to the prior flat
  fill only if no texels are sampled.
- `scripts/verify-runtime.ps1` now prints and asserts texture texel dimensions
  and sampled texel writes, so the visible preview cannot pass using only a
  decoded-but-unused texture setup.

Still parked:

- This is not a general texture backend. It samples one bounded RGBA16 texture
  contract in software for the first visible Opening Room DObj preview.
- DS VRAM texture upload, RDP combiner behavior, material animation,
  camera-correct projection, depth behavior, display-list branching, and
  continuous draw remain renderer milestones.

Verification:

- `make clean` followed by `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1272
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 41 -> Title).`
- GDB/runtime counters for the bounded preview reported first-DL marker
  `0x4F524450`, `5618` final preview pixels, texture mask `0x3F`, derived
  texels `32x32`, `5772` sampled texel writes, and transformed bounds
  `-4264,3617,1049,8205`.
- `scripts/capture-melonds.ps1 -Build -DelaySeconds 32` captured the live
  emulator at `artifacts/melonds-20260619-235925.png`; visual inspection
  confirmed the first Opening Room preview is visibly texture-sampled, the
  startup logo preview remains filtered, and the bottom-screen debug text stays
  inside the visible DS console.

## 2026-06-20: Opening Room active camera CObj proof

Reused:

- The renderer entry path remains the bounded original
  `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.
- The active camera source is the original Scene 2 main camera created by
  `mvOpeningRoomMakeScene2Cameras` at tick `560`.
- `sm64-nds` remains the architecture reference for keeping original display
  lists as backend input and proving renderer contracts before broadening.

Adapted for DS:

- `src/port/scene_backend.c` now records the active camera CObj contract at the
  first Opening Room draw callback: mask `0x40`, priority `80`, CObj flags
  `0x4`, two XObjs with kinds `3` and `8`, viewport `600,440,640,480`,
  positive perspective values, and eye/at vector samples.
- `scripts/verify-runtime.ps1` prints and asserts the stable CObj fields so a
  later camera-correct projection attempt can prove it is using the original
  camera state instead of an arbitrary DS projection.
- The diagnostics are verifier-only; the bottom-screen HUD was not expanded,
  keeping the visible debug text within the DS console.

Still parked:

- The visible DObj preview still normalizes vertices into a bounded debug box.
  It does not yet apply the original camera projection, view matrix, z-buffer,
  or viewport to the DS renderer.
- Full `sys/objdisplay.c`, material handling, combiner behavior, texture VRAM
  upload, display-list branching, and continuous draw remain deferred.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1272
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 47 -> Title).`

## 2026-06-20: melonDS visual-debug cleanup

Reused:

- The visible startup logo remains the original `N64Logo` Sprite reached
  through the bounded original startup draw path:
  `mnStartupFuncDraw -> gcDrawAll -> lbCommonDrawSprite ->
  gcCaptureCameraGObj -> lbCommonDrawSObjAttr`.
- The Opening Room preview remains the same first original link-6 DObj display
  list reached through the bounded original draw path.
- The bottom HUD still prints the same diagnostics sampled by
  `scripts/verify-runtime.ps1`; no new gameplay, scene, fighter, stage, audio,
  or broad renderer code was imported.

Adapted for DS:

- `src/nds/nds_platform.c` now displays the startup logo preview at the
  decoded asset's native `128x108` size, clamped into the DS top framebuffer.
- The startup logo presentation copy now applies a one-time weighted 3x3
  display filter when committed. This reduces the coarse visible pixel grid in
  melonDS while leaving the source asset and verifier-visible sprite conversion
  unchanged.
- Bottom-screen HUD redraw no longer clears the whole console every refresh.
  Each line is row-addressed and limited below the DS text width so it cannot
  wrap, scroll, or leave duplicate/stale rows during live visual debugging.

Still parked:

- The filtered startup logo is still a diagnostic preview, not a final DS-native
  title/menu renderer.
- The Opening Room DObj preview still uses the bounded debug-box projection.
  Camera-correct projection, material handling, texture upload, z-buffering,
  display-list branching, and continuous draw remain renderer milestones.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1168
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 37 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live emulator at
  `artifacts/melonds-20260620-002627.png`; visual inspection confirmed the
  startup logo preview is smoother, the bounded Opening Room DObj preview
  remains visible, and the bottom-screen debug text stays inside the visible DS
  console without wrapping or duplicate rows.

## 2026-06-20: Opening Room camera-projected DObj preview

Reused:

- The renderer entry path remains the bounded original
  `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.
- The selected geometry and texture source remain the first original link-6
  `DObjDLLink->dl` captured by the Opening Room draw probe.
- The active projection source is the original Scene 2 CObj created by
  `mvOpeningRoomMakeScene2Cameras` at tick `560`.
- The `sm64-nds` architectural rule remains intact: original display-list
  producers stay as backend input while DS-specific renderer behavior is
  isolated in the port/platform layer.

Adapted for DS:

- `src/port/scene_backend.c` now keeps the currently capturing camera GObj
  scoped around each original `proc_display` callback inside the narrow
  `gcCaptureCameraGObj` bridge.
- The bounded DObj preview derives a software perspective/look-at/viewport
  projection from that active CObj, following BattleShip's
  `syMatrixPerspFastF` and `syMatrixModLookAtF` conventions for this one
  preview path.
- The preview now draws through camera-projected coordinates when the first
  slice has visible projected triangles. The old normalized debug-box path
  remains only as a recorded fallback if projection cannot be proven.
- `scripts/verify-runtime.ps1` now prints and asserts projection mask `0x7F`,
  camera projection mode `1`, blocker `0`, projected vertices/triangles `9/4`,
  projected bounds `-8,94,-71,23`, positive depth range, nonzero pixels, and
  retained top-screen presentation.

Still parked:

- This is not a general object-display renderer. It handles one bounded DObj
  display-list slice in software.
- Material setup, RDP combiner behavior, z-buffering, display-list branching,
  DS texture upload, full `sys/objdisplay.c`, and continuous draw remain
  deferred.
- Fighters, stages, battle gameplay, audio, and broad title/menu systems remain
  outside this renderer slice.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1167
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 39 -> Title).`
- One-off GDB sampling reported `PROJECTION=0x7f,1,0`, `PROJECTED=9,4`,
  projected bounds `-8,94,-71,23`, projected depth `1019200,1095300`, and
  `2236` preview pixels.
- `scripts/capture-melonds.ps1 -Build` captured the live emulator at
  `artifacts/melonds-20260620-003717.png`; visual inspection confirmed the
  bordered Opening Room preview remains visible and is now cropped/framed by
  the active original camera projection.

## 2026-06-20: low-motion melonDS visual-debug pass

Reused:

- The Startup logo still comes from the original `N64Logo` Sprite reached
  through `mnStartupFuncDraw -> gcDrawAll -> lbCommonDrawSprite ->
  gcCaptureCameraGObj -> lbCommonDrawSObjAttr`.
- The Opening Room preview still comes from the first original link-6
  `gcDrawDObjDLLinksForGObj` display-list slice.
- The bottom-screen diagnostic text still mirrors the runtime verifier globals.

Adapted for DS:

- The legacy DS-side probe no longer changes color through original RNG every
  frame. It remains available as a stable placeholder marker, but the original
  asset previews are now the visual signal.
- The startup logo presentation copy is now scaled to the DS projection of the
  N64 logical screen (`320x240 -> 256x192`) and uses area-filtered partial
  coverage during the diagnostic copy. The source asset remains the original
  `128x108` RGBA16 payload.
- The top melonDS HUD now uses a compact low-motion trace strip instead of the
  old full marker ladder behind the logo and Opening Room preview. Thin bottom
  ticks retain bounded update, skip, and thread-sleep progress without covering
  the asset previews.

Still parked:

- This is visual-debug presentation cleanup only. It is not a final N64 sprite
  renderer, DS-native title/menu renderer, or general RSP/RDP backend.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1842
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 55 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured the live emulator at
  `artifacts/melonds-20260620-005407.png`; visual inspection confirmed the
  top-screen marker noise is reduced, the startup logo uses the filtered
  DS-logical presentation, the Opening Room preview remains visible, and the
  bottom-screen debug text stays inside the visible console rows.

## 2026-06-20: Opening Room geometry-mode display-list state

Reused:

- The draw entry remains the bounded original
  `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.
- The selected display-list source remains the first original link-6
  `DObjDLLink->dl` already used by the Opening Room preview.
- The geometry-mode update rule follows the same contract used by `sm64-nds`:
  `geometry_mode = (geometry_mode & w0) | w1`.

Adapted for DS:

- The narrow preview now preserves raw `G_GEOMETRYMODE` state for the first
  bounded list and records the last clear/set masks, final mode, decoded
  flags, and projected triangle winding.
- The first bounded list currently emits two geometry-mode commands and ends at
  final mode `0x220000` (`G_LIGHTING | G_SHADING_SMOOTH`). No cull or z-buffer
  bits are set for this slice, so the preview records winding evidence
  `4/0/0/4` without applying a cull rule.
- `scripts/verify-runtime.ps1` now prints and asserts those exact values:
  command count `2`, clear mask `0xD9FFFFFF`, set/final mode `0x220000`, flags
  `0x61`, and projected winding/drawn triangles `4/0/0/4`.

Still parked:

- Lighting and smooth-shading are recorded but not yet rendered as a full N64
  material/light pipeline.
- General display-list state, culling, z-buffering, material/combiner mapping,
  DS texture upload, and continuous draw remain renderer milestones.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1841
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 65 -> Title).`
- One-off GDB sampling reported `GEOM=2,0xd9ffffff,0x220000,0x220000,0x61`,
  `GEOM_TRIS=4,0,0,4`, `PROJECTED=9,4`, and `PIXELS=2236`.

## 2026-06-20: Opening Room render-tile state and melonDS visual smoothing

Reused:

- The draw entry remains the bounded original
  `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.
- The texture source remains the original first-list `G_SETTIMG` pointer from
  the selected link-6 Opening Room DObj display list.
- The DS texture-addressing model follows the `sm64-nds` renderer reference:
  N64 tile clamp/mirror bits map to DS texture addressing behavior, while this
  port still keeps the path as a bounded software diagnostic preview.

Adapted for DS:

- `src/port/scene_backend.c` now decodes the render and load `G_SETTILE`
  state for the first bounded list. The verified render tile is tile `0`, line
  `8`, TMEM `0`, palette `0`, S mode `0`, T mode `2`, masks `5/5`, shifts
  `0/0`, and diagnostic tile flags `0xB3`.
- The bounded software texture sampler now uses the decoded render-tile
  clamp/wrap/mask state instead of wrapping S blindly by physical texture
  width and clamping T without recording the source tile mode.
- `scripts/verify-runtime.ps1` prints and asserts the render-tile line, TMEM,
  palette, S/T modes, masks, shifts, and flags. The texture sample count
  remains nonzero for the selected original RGBA16 texture.
- `src/nds/nds_platform.c` blends partial startup-logo coverage against the
  actual top-screen background and keeps the diagnostic copy as a
  presentation-only preview. Later HUD work replaced the old timed bottom-text
  cadence with change-driven redraws and added supersampled logo sampling.
  These changes reduce visible flashing/coarse-pixel artifacts in melonDS
  without changing the original `N64Logo` asset or pretending to be the final
  sprite renderer.

Still parked:

- This is still one bounded render-tile contract for one selected display-list
  slice. General texture upload, material/combiner mapping, z-buffering,
  display-list branching, and continuous draw remain deferred.
- The startup logo remains a diagnostic Sprite preview; final N64 sprite
  rendering still belongs in the real renderer path.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1909
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 64 -> Title).`
- One-off GDB sampling reported `TILE=0,8,0,0`,
  `MODE=0,2,5,5,0,0,0xb3`, and `TEX=32,32,2212`.
- `scripts/capture-melonds.ps1 -Build` captured the live emulator at
  `artifacts/melonds-20260620-011830.png`; visual inspection confirmed the
  filtered startup-logo preview, retained Opening Room DObj preview, and
  contained bottom-screen debug text.

## 2026-06-20: Opening Room `G_TEXTURE` scale state

Reused:

- The draw entry remains the bounded original
  `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.
- The selected command stream remains the first original link-6
  `DObjDLLink->dl` already used by the Opening Room preview.
- The texture-coordinate scale rule follows `sm64-nds`: vertex texture
  coordinates are multiplied by the `G_TEXTURE` S/T scale before submission.

Adapted for DS:

- `src/port/scene_backend.c` now decodes the first bounded list's `G_TEXTURE`
  command and records command count, S/T scale, level, tile, on/off, xparam,
  and diagnostic state flags.
- The bounded software sampler now applies the decoded S/T scale before
  render-tile addressing. For the current list the scale is `65535/65535`, so
  the visual sample count remains stable while the renderer contract is now
  explicit.
- `scripts/verify-runtime.ps1` now prints and asserts the exact `G_TEXTURE`
  state: one command, scale `65535/65535`, level `0`, tile `0`, on `1`, xparam
  `0`, and flags `0x0F`.

Still parked:

- This remains a bounded software preview for one display-list slice. DS VRAM
  texture upload, RDP combiner behavior, material mapping, z-buffering,
  display-list branching, and continuous draw remain renderer milestones.

Verification:

- `make clean && make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1905
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 48 -> Title).`
- One-off GDB sampling reported `TEXTURE=1,65535,65535,0xf`,
  `TEXTURE_MODE=0,0,1,0`, and `TEX=32,32,2212,2236`.
- `scripts/capture-melonds.ps1 -Build` captured the live emulator at
  `artifacts/melonds-20260620-013513.png`; visual inspection confirmed the
  startup logo preview, retained Opening Room DObj preview, and contained
  bottom-screen debug text.

## 2026-06-20: melonDS HUD flicker reduction and logo preview filtering (superseded)

This entry records the first HUD cleanup pass. The later
`melonDS HUD stability and native logo presentation` entry supersedes the
logo presentation details.

Reused:

- The startup logo still comes from the original `N64Logo` Sprite reached
  through the bounded original startup draw path.
- The Opening Room preview still uses the existing first link-6 bounded DObj
  display-list slice. No fighter, stage, audio, menu, or broader renderer path
  was imported.

Adapted for DS:

- At this point, `src/nds/nds_platform.c` presented the startup logo diagnostic copy with a
  two-by-two supersampled bilinear sampler before the existing presentation
  smoothing pass. This changed only the DS debug preview; it did not change
  the original asset or claim a final sprite renderer. This was later replaced
  by the native-resolution retained diagnostic copy.
- The bottom-screen debug text now uses a fingerprint of the displayed
  diagnostic globals and redraws only when that state changes. The live frame
  counter was removed from the visible text line so a stable boundary no longer
  pulses every redraw.
- `scripts/debug-melonds.ps1` documented the supersampled logo preview and
  change-driven bottom text behavior at this point in the chronology.

Still parked:

- The startup logo preview remains a diagnostic Sprite path. Final sprite
  rendering still belongs in the real N64 display-list/sprite backend.
- The Opening Room preview still needs general material/combiner, texture VRAM
  upload, z-buffering, display-list branching, and continuous draw support.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1972
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 43 -> Title).`
- `scripts/capture-melonds.ps1 -DelaySeconds 32` and
  `scripts/capture-melonds.ps1 -DelaySeconds 33` captured the two visual-fix
  comparison images for that pass.
- A pixel comparison of the emulator content region reported zero differing
  pixels between those two captures, proving the steady-state HUD no longer
  flashes from the debug text cadence.

## 2026-06-20: Opening Room raw texture load/tile-size diagnostics

Reused:

- The draw entry remains the bounded original
  `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.
- The selected command stream remains the first original link-6
  `DObjDLLink->dl` already used by the Opening Room preview.
- The field layout comes from BattleShip's original `PR/gbi.h` macros:
  `gDPLoadBlock` and `gDPSetTileSize`/`gDPLoadTileGeneric`.

Adapted for DS:

- `include/nds/nds_startup.h` and `src/port/scene_backend.c` now expose the raw
  first-list `G_LOADBLOCK` fields: tile `7`, `uls=0`, `ult=0`, `lrs=1023`,
  and `dxt=256`.
- The same bounded parser now exposes the raw first-list `G_SETTILESIZE`
  fields: tile `0`, `uls=0`, `ult=0`, `lrs=380`, and `lrt=124`. These are the
  command fields that derive the verified `96x32` render tile.
- `scripts/verify-runtime.ps1` prints and asserts both raw command records, so
  the current texture preview proves the original load/tile-size command
  contract instead of only proving derived dimensions.

Still parked:

- This remains diagnostic evidence for one bounded display-list slice. It does
  not add DS VRAM texture upload, RDP combiner mapping, general tile memory
  emulation, display-list branching, z-buffering, or continuous draw.

Verification:

- `make clean && make -j4` built `smash64ds.nds` after the new public
  diagnostics were added.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1965
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 27 -> Title).`

## 2026-06-20: Opening Room first-list combiner contract

Reused:

- The draw entry remains the bounded original
  `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.
- The selected command stream remains the first original Opening Room
  `DObjDLLink->dl` already used by the bounded preview.
- The combiner bit layout comes from BattleShip's original `PR/gbi.h`
  `G_SETCOMBINE` / `G_CC_*` macros.

Adapted for DS:

- `src/port/scene_backend.c` now decodes the first list's exact combine words
  `0xFC127E24,0xFFFFF3F9` as `MODULATERGBDECALA`: RGB is sampled texture
  multiplied by vertex shade, while alpha comes from the texture.
- The bounded software preview now applies that exact mode by interpolating
  vertex RGB across the triangle and modulating sampled RGBA16 texels before
  writing the preview pixel.
- The artificial yellow diagnostic triangle outline is no longer drawn for the
  texture-ready path; verifier diagnostics now prove the callback and command
  path instead.
- `include/nds/nds_startup.h` exposes combine mode/flags and modulated-pixel
  diagnostics, and `scripts/verify-runtime.ps1` asserts mode `1`, flags
  `0x3F`, and nonzero shade-modulated texel writes.

Still parked:

- This is not a general RDP combiner. Other combine modes, primitive/env
  colors, multiple texture tiles, DS VRAM texture upload, z-buffering,
  display-list branching, material animation, and continuous draw remain
  deferred.

Verification:

- `make clean && make -j4` built `smash64ds.nds` after the public diagnostic
  header change.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1956
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 31 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-021048.png`, showing the startup logo, the
  texture-shaded Opening Room preview, and bottom-screen debug text contained
  within the visible bottom screen.

## 2026-06-20: Opening Room first-list command-shape diagnostics

Reused:

- The selected command stream remains the first original Opening Room
  `DObjDLLink->dl` reached through the bounded original
  `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.
- The opcode classifications are based on BattleShip's original `PR/gbi.h`
  command IDs and the `sm64-nds` renderer's separation between DL branch,
  othermode, sync, vertex, and triangle handling.

Adapted for DS:

- `src/port/scene_backend.c` now records a command-shape contract for this
  bounded first list: vertex command count, triangle command count, RDP sync
  command count, end command count, display-list branch/cull command count,
  othermode command count, and total unsupported command count.
- The bounded parser explicitly classifies `G_DL`, `G_CULLDL`,
  `G_SETOTHERMODE_H`, `G_SETOTHERMODE_L`, and raw RDP set-othermode as
  unsupported for this slice instead of silently folding them into a generic
  default.
- `scripts/verify-runtime.ps1` now asserts that the current first list has
  vertex/triangle/sync/end commands, exactly one `G_ENDDL`, and zero
  branch/othermode/unsupported commands.

Still parked:

- This does not add display-list recursion, culling commands, othermode/render
  mode mapping, z-buffer behavior, DS texture upload, material animation, or
  continuous draw. It makes the absence of those requirements in the selected
  first list verifier-backed before the renderer is broadened.

Verification:

- `make clean && make -j4` built `smash64ds.nds` after the public diagnostic
  header change.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1959
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 29 -> Title).`

## 2026-06-20: RDP default viewport shim

Reused:

- The contract comes directly from BattleShip's original
  `decomp/BattleShip-main/decomp/src/sys/rdp.c`:
  `syRdpSetDefaultViewport` sets scale and translate X/Y to
  `gSYVideoResWidth * 2` / `gSYVideoResHeight * 2`, and Z scale/translate to
  `G_MAXZ / 2`.
- The architectural reference remains `sm64-nds`: keep original display-list
  producers intact and replace backend/RDP behavior in the DS platform layer.

Adapted for DS:

- `src/port/scene_backend.c` now implements the narrow default viewport
  contract instead of leaving `syRdpSetDefaultViewport` as a no-op.
- The shim records verifier-visible diagnostics:
  call count plus the last default viewport values. For the current 320x240
  BattleShip video setup, the verified values are
  `640,480,640,480,511,511`.
- `scripts/verify-runtime.ps1` now prints and asserts `RDP_DEFAULT_VIEWPORT`,
  while leaving the active Opening Room Scene 2 camera viewport assertion
  unchanged at `600,440,640,480`.

Still parked:

- This does not import full `sys/rdp.c`, emit the original reset display list,
  enable broad GBI macros, or translate RSP/RDP tasks to DS GPU rendering.
- General material/combiner mapping, texture upload, z-buffering,
  display-list branching, and continuous draw remain renderer milestones.

Verification:

- `make clean && make -j4` built `smash64ds.nds` after the source/header
  diagnostic change.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1962
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 23 -> Title).`

## 2026-06-20: melonDS HUD stability and native logo presentation

Reused:

- The startup logo still comes from the original `N64Logo` Sprite reached
  through the bounded original startup draw path.
- The Opening Room preview still uses the existing first link-6 bounded DObj
  display-list path.

Adapted for DS:

- `src/nds/nds_platform.c` now retains the startup logo preview at the original
  `128x108` Sprite resolution for melonDS readability instead of applying an
  extra DS-logical downscale and second smoothing pass.
- The top Opening Room progress strip now advances by verified scene milestones
  instead of by every live tick.
- The bottom text HUD hides the console cursor and uses the same milestone
  tick value in its redraw fingerprint, removing steady per-frame text/cursor
  motion from the visual debug display.

Still parked:

- This is a debug presentation cleanup only. It does not add final sprite
  compositing, DS texture upload, z-buffering, display-list branching, or a
  continuous original draw loop.

Verification:

- `make -j4` built `smash64ds.nds`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1986
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 50 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-024015.png`.
- `scripts/capture-melonds.ps1 -DelaySeconds 36` captured
  `artifacts/melonds-20260620-024104.png`.
- Comparing the two captures after the Opening Room boundary produced
  `CONTENT_PIXEL_DIFF=0` across the emulator content region; only the outer
  OS/window edge differed.

## 2026-06-20: First-DObj material contract diagnostics

Reused:

- The next renderer contract was inspected against BattleShip's original
  `sys/objdisplay.c`. Original `gcDrawDObjDLLinks` calls
  `gcDrawMObjForDObj` before emitting each DObj display list, and
  `gcDrawMObjForDObj` applies the original material flag fallback
  `MOBJ_FLAG_TEXTURE | 0x20 | MOBJ_FLAG_ALPHA` when a material has no explicit
  flags.
- The selected draw slice remains the first original Opening Room DObj reached
  through the bounded `gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj ->
  gcDrawDObjDLLinksForGObj` path.

Adapted for DS:

- `src/port/scene_backend.c` now records the selected preview DObj's material
  chain count, raw/effective first-MObj flags, texture IDs, palette index,
  format fields, tile dimensions, texture scale/translation, and safe resolved
  sprite/palette pointers when original flags require those arrays.
- `include/nds/nds_startup.h` exposes a narrow material diagnostic mask instead
  of adding a broad renderer or importing full `sys/objdisplay.c`.
- `scripts/verify-runtime.ps1` now prints and asserts the current first-DObj
  material boundary. The expected source-side contract is intentionally
  "DObj seen, zero MObjs" because the previously verified DObj meta for this
  selected slice is `0x11` (display-list pointer plus XObj, no MObj).
- `scripts/verify-runtime.ps1` and `scripts/verify-opening-skip.ps1` now set
  both melonDS GDB config spellings, `Enable = true` and `Enabled = true`,
  before launching the emulator, then restore the original config.

Still parked:

- This does not implement the material display-list branch, texture upload,
  combiner mapping, z-buffering, display-list recursion, or continuous draw.
  If the next selected DObj has a material chain, these diagnostics should be
  tightened around that material-bearing object before general renderer work is
  broadened.

Verification:

- `make -j4` rebuilt `smash64ds.nds`.
- PowerShell parser checks passed for `scripts/verify-runtime.ps1` and
  `scripts/verify-opening-skip.ps1`.
- `arm-none-eabi-nm` confirms the new
  `gNdsOpeningRoomDLPreviewMaterial*` globals are present in `smash64ds.elf`.
- Runtime verification and melonDS capture are currently blocked in this Codex
  desktop session: melonDS 1.1 starts as a live hidden/windowless process and
  does not listen on ARM9 GDB port `3333`. The same hidden-window condition
  reproduces with `smash64ds.nds`, the local `sm64-nds` ROM, and devkitPro's
  `Simple_Tri.nds`, so the blockage is not specific to this ROM.

## 2026-06-20: Material-bearing DObj candidate selection

Reused:

- The selector was implemented against the original BattleShip draw contracts
  in `sys/objdisplay.c` and `sys/objman.c`: `gcDrawAll` traverses camera GObjs,
  `func_80017EC0` captures the active camera, and DObj callbacks use different
  payload shapes depending on whether the original callback draws `dobj->dl` or
  `dobj->dl_link`.
- The visible Opening Room preview remains the previously verified first
  renderable DObj slice. That keeps the existing bounded texture-shaded preview
  stable while the next material branch is characterized.

Adapted for DS:

- `src/port/scene_backend.c` now records the first material-bearing DObj
  encountered by the same bounded imported `gcDrawAll` traversal. It stores the
  candidate camera mask/priority, object link/id/kind, original callback marker,
  DL pointer, and DObj meta.
- The selector is callback-aware: `gcDrawDObjDLHead1` and `gcDrawDObjTreeForGObj`
  inspect plain `dobj->dl`, while `gcDrawDObjDLLinksForGObj` and
  `gcDrawDObjTreeDLLinksForGObj` inspect `dobj->dl_link`. This avoids treating
  a plain display-list pointer as a DL-link array.
- The preview render is now deferred until after the bounded draw traversal has
  exposed candidates. For this step, it still renders the stable first
  renderable DObj; the material-bearing candidate is recorded separately for the
  next `gcDrawMObjForDObj` branch boundary.
- `include/nds/nds_startup.h` exposes the narrow material-candidate diagnostic
  ABI, and `scripts/verify-runtime.ps1` prints/asserts a structural candidate:
  `ORMC`, at least one material candidate, a real camera/object/callback, a
  nonzero candidate DL pointer, and DObj meta bits proving both DL and MObj.

Still parked:

- The material candidate is not rendered through its original material branch
  yet. The DS backend still does not emit or translate the branch display lists
  that original `gcDrawMObjForDObj` builds before the DObj display list.
- General material/combiner mapping, texture upload, z-buffering, display-list
  branching, and continuous draw remain out of scope for this narrow step.

Verification:

- `make -j4` rebuilt `smash64ds.nds`.
- PowerShell parser checks passed for `scripts/verify-runtime.ps1` and
  `scripts/verify-opening-skip.ps1`.
- `C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe -g smash64ds.elf`
  confirms the new `gNdsOpeningRoomDrawMaterialCandidate*` globals are present.
- `scripts/verify-runtime.ps1` was attempted and still fails before marker
  checks because GDB cannot connect to `localhost:3333`; this is the existing
  melonDS hidden/windowless/no-GDB session blocker, not a compile failure.

## 2026-06-20: Visual preview cleanup and candidate MObj snapshot

Reused:

- The startup logo still comes from the original `N64Logo` Sprite reached
  through the bounded original startup draw path.
- The visible Opening Room preview remains the previously verified first
  renderable DObj slice. The material-bearing candidate remains separate
  diagnostic evidence for the next renderer boundary.
- The material probe behavior stays aligned with BattleShip's original
  `gcDrawMObjForDObj` contract instead of importing a broad renderer slice.

Adapted for DS:

- `src/nds/nds_platform.c` now copies retained source texels directly when the
  startup logo preview stays at its native `128x108` size. The bilinear sample
  path remains available only for scaled cases, so the diagnostic preview no
  longer blurs the logo with an extra same-size filtering pass.
- `src/port/port_probe.c` now leaves `portProbeRender` empty. The original
  asset previews own the top-screen visual signal, while the old DS-native
  movement probe update remains available as a narrow vector/input smoke path.
- `src/port/scene_backend.c` now snapshots the first material-bearing
  candidate's first-MObj contract: count, raw/effective flags, material mask,
  texture IDs, palette index, format/size, tile/scroll dimensions, S/T scale and
  translate, and safe sprite/palette pointer evidence.
- `scripts/verify-runtime.ps1` prints and asserts the candidate first-MObj
  structure. This tightens the next material-branch boundary without emitting
  or translating material branch display lists.
- `scripts/capture-melonds.ps1` temporarily disables melonDS GDB keys for the
  visible capture attempt and restores the original config in `finally`.

Still parked:

- No new fighters, stages, audio, full title/menu systems, continuous draw loop,
  general material/combiner mapping, texture upload, z-buffering, display-list
  branching, or `gcDrawMObjForDObj` material branch emission were added.
- The visible logo is now a native-texel diagnostic copy of the original
  `128x108` Sprite. It remains intentionally pixelated because the source asset
  is low-resolution; the fix removes extra debug-path blur and the unrelated
  DS-native overlay, not the source texture's pixel grid.

Verification:

- `make -j4` rebuilt `smash64ds.nds`.
- PowerShell parser checks passed for `scripts/verify-runtime.ps1`,
  `scripts/verify-opening-skip.ps1`, and `scripts/capture-melonds.ps1`.
- `C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe -g smash64ds.elf`
  confirms the new `gNdsOpeningRoomDrawMaterialCandidateMObj*` globals are
  present.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1979
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 62 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-035410.png`.
- `scripts/capture-melonds.ps1` captured
  `artifacts/melonds-20260620-035510.png`.
- Comparing the two captures reported `0` differing pixels across the full
  captured image, with `0` differing pixels in the estimated emulator content,
  top-screen, and bottom-screen regions.

## 2026-06-20: Bounded material branch command-family probe

Reused:

- The next renderer boundary was taken from BattleShip's original
  `gcDrawMObjForDObj` in `decomp/BattleShip-main/decomp/src/sys/objdisplay.c`.
- The existing imported `gcDrawAll` traversal and first material-bearing DObj
  selector remain the source of candidate DObj/MObj data.
- The visible preview still renders the already verified first renderable DObj
  slice; no DS-native title/menu or gameplay rendering was added.

Adapted for DS:

- `src/port/scene_backend.c` now mirrors the original material branch-list
  command-family decisions for the first material-bearing candidate without
  allocating or emitting branch `Gfx`.
- The new diagnostics record marker `ORMB` (`0x4F524D42`), MObj count, segment
  command count, branch-table command count, generated branch-stream command
  count, first-MObj command-family mask, first-MObj generated command count,
  and first-MObj tile/scroll/texture/load-block values when those families are
  present.
- `scripts/verify-runtime.ps1` prints and asserts the new branch contract. The
  current verified values are `2` MObjs, `1` segment command, `2` branch-table
  commands, `6` generated branch commands, first mask `0x4443`, and first
  generated count `3`.
- `scripts/verify-runtime.ps1` and `scripts/verify-opening-skip.ps1` now attach
  GDB to `127.0.0.1:3333`, wait for the melonDS ARM9 listener, and update the
  `[Instance0.Gdb]` TOML section without duplicating `Enable` / `Enabled` keys.

Still parked:

- No full `sys/objdisplay.c` import was attempted.
- The DS backend still does not emit the material branch display lists that
  original `gcDrawMObjForDObj` would place in the graphics heap.
- General material/combiner mapping, texture upload, z-buffering,
  display-list branching, RDP reset handling, and continuous draw remain out of
  scope for this narrow step.

Verification:

- `make -j4` passed.
- PowerShell parser checks passed for `scripts/verify-runtime.ps1` and
  `scripts/verify-opening-skip.ps1`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1958
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 45 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-042217.png`.

## 2026-06-20: Detached material branch Gfx emission

Reused:

- The emitted layout follows BattleShip's original `gcDrawMObjForDObj`
  structure: `gSYTaskmanGraphicsHeap.ptr` starts with one `G_DL` branch-table
  command per MObj, followed by the generated branch stream, then the taskman
  graphics heap pointer advances past the emitted stream.
- The first material-bearing DObj selector and `ORMB` command-family probe
  still provide the source candidate; no broad `sys/objdisplay.c` import was
  attempted.

Adapted for DS:

- `src/port/scene_backend.c` now emits a detached original-shaped branch table
  and branch stream for the selected Opening Room material candidate. The
  current supported family is intentionally narrow: env color, current texture
  image command, and end display list.
- The new diagnostics record marker `ORME` (`0x4F524D45`), blocker,
  unsupported family mask, MObj/table/generated counts, heap start/branch
  start/heap-after/byte count, first table opcode, and the first three branch
  command opcodes/words.
- The first material image pointer is currently unresolved and emits as
  `SETTIMG` word1 `0`. This avoids crashing the diagnostic-only detached
  emitter while preserving the original command shape; it does not mean
  texture upload or live material branch rendering works.
- `scripts/verify-runtime.ps1` now prints and asserts the `ORME` contract. The
  current verified values are blocker `0`, unsupported mask `0`, `2` table
  commands, `6` generated commands, `64` heap bytes, table opcode `0xDE`, and
  first branch opcodes `0xFB`, `0xFD`, `0xDF`.

Still parked:

- The emitted material branch stream is detached from visible rendering.
- Nonzero material texture pointer fixup, DS texture upload, combiner mapping,
  depth, display-list branching, RDP reset handling, and continuous draw remain
  out of scope for this narrow step.

Verification:

- `make -j4` passed.
- PowerShell parser check passed for `scripts/verify-runtime.ps1`.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1932
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 10 -> Title).`

- `scripts/capture-melonds.ps1 -Build -Output artifacts\material-emit-check.png`
  captured the live melonDS window; visual inspection confirmed the bounded
  preview is unchanged and the bottom-screen debug text remains bounded.

## 2026-06-20: Startup Sprite preview overlap and DS-scale presentation

Reused:

- The source asset remains the original `N64Logo` Sprite loaded from BattleShip
  reloc file `194` through the existing bounded startup draw path:
  `mnStartupFuncDraw -> gcDrawAll -> lbCommonDrawSprite ->
  gcCaptureCameraGObj -> lbCommonDrawSObjAttr`.
- The Sprite/Bitmap contract was checked against BattleShip's local
  `decomp/BattleShip-main/decomp/src/libultra/sp/sprite.c` before changing the
  DS shim.

Adapted for DS:

- `src/port/scene_backend.c` now draws each startup Sprite bitmap chunk using
  `Bitmap.actualHeight` while advancing by `Sprite.bmheight`, matching the
  original Sprite overlap behavior. The N64Logo strips are 15 pixels tall with
  a 14-pixel advance and `SP_OVERLAP`, so dropping the overlap row made the
  retained preview look rougher than the original contract.
- `src/nds/nds_platform.c` now presents the retained startup Sprite preview in
  the same 320x240-to-256x192 logical scale used for its position. This uses
  the existing software sampler and keeps the melonDS visual debug output from
  exaggerating the original asset's pixel grid.

Still parked:

- This is visual-debug presentation cleanup only. It does not upload the Sprite
  to DS texture VRAM, import full `lbcommon`, emit sprite display lists, or
  implement the final N64 sprite/display-list renderer.
- The remaining coarse look is expected for a bounded software diagnostic
  preview of a low-resolution RGBA16 source asset; final renderer quality still
  depends on the later texture/display-list backend.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1922
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip verification
  passed (tick 30 -> Title).`
- `scripts/capture-melonds.ps1` captured
  `artifacts/flash-check-scaled.png`; visual inspection confirmed the startup
  logo preview is smaller/smoother, the Opening Room DObj preview remains
  visible, and the bottom-screen debug text stays bounded.

## 2026-06-20: Material texture-source diagnostic and visual capture

Reused:

- The first material-bearing Opening Room DObj candidate still comes from the
  imported `gcDrawAll` traversal and the previously verified `ORMB`/`ORME`
  material branch probes.
- The startup logo remains the original `N64Logo` Sprite diagnostic preview;
  no DS-native replacement art or handwritten title/menu rendering was added.

Adapted for DS:

- `src/port/scene_backend.c` now records the first candidate MObj's raw
  `sprites` and `palettes` array pointers alongside the already safe
  current/next sprite and palette-pointer diagnostics.
- `scripts/verify-runtime.ps1` prints `OPENING_ROOM_DRAW_MATERIAL_MOBJ_ARRAYS`
  and asserts that a null emitted material `SETTIMG` pointer is explained by a
  null sampled first-MObj `sprites` array.
- Visual inspection of `artifacts/melonds-20260620-050955.png` confirms the
  current top-screen output still shows the bounded original startup logo
  preview plus the bounded Opening Room DObj preview. The visible logo is still
  a diagnostic software preview of a low-resolution RGBA16 source asset, so its
  coarse look is not final renderer quality.

Still parked:

- No live material branch execution, nonzero material texture pointer fixup,
  DS texture upload, broad combiner mapping, or full display-list backend was
  added.
- The top-screen status rectangles remain debug markers. They may look like
  flashing while the emulator is still advancing milestones, but the old
  DS-native moving probe overlay remains disabled.

Verification:

- `make clean; make -j4` passed.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1929
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1` captured
  `artifacts/melonds-20260620-050955.png`.

## 2026-06-20: MVCommon material MObjSub normalization (superseded)

The later `ORMT material-source scan and no-flash melonDS HUD` entry widens
this three-record proof to 18 MVCommon `MObjSub` records and should be treated
as the current material-normalization boundary.

Reused:

- The material candidate still comes from the imported Opening Room draw
  traversal and the BattleShip `gcDrawMObjForDObj` branch-family contract.
- The source-of-truth material records are the generated MVCommon logo and
  spotlight `MObjSub` declarations in
  `decomp/BattleShip-main/decomp/src/relocData/52_MVCommon.c`.

Adapted for DS:

- `src/port/scene_backend.c` now applies a narrow mixed-width normalizer to the
  three-record MVCommon logo/spotlight `MObjSub` subset used by the then-current bounded
  material slice after the blanket `u32` O2R endian pass and pointer fixups.
- The normalizer fixes the mixed `u16`/`u8` words around `fmt/siz` and
  `flags/block_fmt/block_siz`, swaps adjacent `u16` pairs used by the material
  dimensions, and restores byte-field color access for the current material
  diagnostics.
- The detached material emitter now supports the prim-color-only command family
  needed by this corrected first candidate, emitting `G_SETPRIMCOLOR` followed
  by `G_ENDDL`.
- `scripts/verify-runtime.ps1` now prints
  `OPENING_ROOM_RELOC_MOBJ_NORMALIZE` and asserts `3` normalized records,
  `0` failures, first flags `0x200`, `ORMB` first mask `0x4023`, and `ORME`
  opcodes `DE/FA/DF/00`.

Result:

- The earlier null material `SETTIMG` diagnostic is no longer the active
  material boundary for this candidate. It was evidence that the first
  `MObjSub` flags were still being read through a mixed-width relocation
  artifact.
- The current first material-bearing candidate is now proven as the original
  generated-source prim-color-only slice: two MObjs, two branch-table commands,
  four generated branch commands, and `48` detached heap bytes.

Still parked:

- The emitted material branch stream is still detached from visible rendering.
- General MObjSub normalization, material branch execution, texture upload,
  combiner mapping, display-list branching, depth, and continuous draw remain
  out of scope for this step.

Verification:

- `make clean; make -j4` passed.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1942
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-052713.png`; visual inspection confirmed the
  bounded startup logo preview and Opening Room draw probe remain visible and
  the bottom-screen debug text stays bounded.

## 2026-06-20: Material branch preview-state parse and native logo debug preview

Reused:

- The first material-bearing Opening Room DObj candidate still comes from the
  imported `gcDrawAll` traversal and the previously verified `ORMB`/`ORME`
  branch-family/emission diagnostics.
- The startup logo remains BattleShip's original `N64Logo` Sprite loaded from
  reloc file `194`; no DS-native title/logo art was added.

Adapted for DS:

- `src/port/scene_backend.c` now parses the detached first emitted material
  branch as bounded preview state before drawing the existing first renderable
  Opening Room DObj. The new marker is `ORMP` (`0x4F524D50`), with blocker
  `0`, two parsed commands, one `G_SETPRIMCOLOR`, one `G_ENDDL`, ops
  `0xFA/0xDF`, unsupported op `0`, and prim color matching the detached `ORME`
  branch.
- `scripts/verify-runtime.ps1` prints and asserts the `ORMP` contract. This
  keeps the material work measurable without executing broad material branches
  or importing full object-display rendering.
- `src/nds/nds_platform.c` now presents the startup `N64Logo` preview at native
  `128x108` source size while keeping its placement mapped from N64 320x240
  logical coordinates. The previous DS-logical downscale made the debug capture
  look coarser than the decoded source asset.

Still parked:

- The material branch stream is still detached from visible rendering.
- General material branch execution, DS texture upload, combiner mapping,
  display-list branching, depth, continuous draw, and full RDP reset handling
  remain out of scope.
- The top-screen status bars still change while boot/Opening Room milestones
  advance. Post-boundary captures are the right evidence for persistent
  flashing; transient milestone motion is expected diagnostic output.

Verification:

- `make clean` passed.
- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1945
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 30 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-054437.png`; visual inspection confirmed the
  native-size startup logo preview, bounded Opening Room DObj preview, and
  bottom-screen debug text remain visible and bounded.

## 2026-06-20: Startup logo SP_TEXSHUF visual fix

Reused:

- The startup logo still comes from BattleShip's original `N64Logo` Sprite
  payload loaded from reloc file `194` and reached through the original startup
  SObj draw callback. No DS-native title/logo art was added.
- The fix follows BattleShip's documented sprite texture issue: sprite textures
  such as `N64Logo` need the N64 odd-row TMEM line swizzle undone when the DS
  diagnostic preview samples the strip data directly.

Adapted for DS:

- `src/port/scene_backend.c` now samples `SP_TEXSHUF` startup logo strips with
  the inverse odd-row address map (`column ^ 2`) before the existing RGBA16
  halfword-pair correction. This removes the visible diagonal/striped
  corruption while keeping the original Sprite and Bitmap metadata as the
  source of truth.
- `gNdsStartupLogoDrawTexshuf` and
  `gNdsStartupLogoDrawTexshufSamples` were added so the verifier proves the
  source SObj still carries `SP_TEXSHUF` and that the preview actually used the
  odd-row swizzle path.
- `scripts/verify-runtime.ps1` now prints and asserts the new startup logo
  texture-shuffle diagnostics without adding bottom-screen HUD text.

Still parked:

- This is still a bounded SObj diagnostic preview, not a general Sprite2D,
  RDP/TMEM, or material renderer.
- Transient top-screen milestone bar changes during startup/Opening Room
  advancement remain expected diagnostic motion. Post-boundary captures are the
  evidence for persistent flashing.

Verification:

- `make clean` passed.
- `make -j4` passed with the existing imported-source warnings.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1947
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-060120.png`; visual inspection confirmed the
  startup logo no longer has the coarse diagonal/striped artifact and the
  bottom-screen debug text remains bounded.

## 2026-06-20: Material DObj display-list shape probe

Reused:

- The selected material-bearing DObj still comes from the imported
  `gcDrawAll -> gcDrawDObj...` traversal and the original Opening Room object
  graph. No DS-native menu/title/object rendering path was added.
- The existing `ORMB`, `ORME`, and `ORMP` material diagnostics remain the source
  for first-MObj command-family, detached branch emission, and detached
  prim-color branch parse evidence.

Adapted for DS:

- `src/port/scene_backend.c` now runs a separate non-visual `ORMD`
  (`0x4F524D44`) probe over the first material-bearing candidate's own
  display-list pointer. The probe uses the same bounded opcode policy as the
  visible `ORDP` preview but does not rasterize, replace the visible preview,
  or follow nested branches.
- New globals record the material-candidate DL result, blocker, first DL
  pointer, command count, vertex/triangle counts, first opcode, first
  unsupported opcode, and command-family counts.
- `scripts/verify-runtime.ps1` now prints and asserts the current exact
  blocker contract: result `0`, blocker `3` (`UNSUPPORTED`), first opcode
  `0xE7`, first unsupported opcode `0xDE` (`G_DL`), 29 commands, four
  vertices, four triangles, two vertex commands, two triangle commands, seven
  sync commands, one `G_ENDDL`, two branch commands, zero othermode commands,
  and two unsupported commands.

Still parked:

- The material candidate is still not drawn. The visible preview remains on the
  verified first renderable DObj slice.
- The next narrow renderer blocker is bounded `G_DL` branch expansion/execution
  for the material candidate, followed by live material branch execution,
  texture upload, combiner/depth mapping, and full RDP reset handling.

Verification:

- Incremental `make -j4` passed with the existing imported-source warnings.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1956
  frames)` after the `ORMD` assertions were added.
- Final `make clean` passed.
- Final `make -j4` passed with the existing imported-source warnings.
- Final `scripts/verify-runtime.ps1` passed with `Runtime verification passed
  (1949 frames).`
- Final `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-061817.png`; visual inspection confirmed the
  startup logo remains clean, the bounded Opening Room preview is unchanged by
  the non-visual material-DL probe, and the bottom-screen debug text remains
  bounded.

## 2026-06-20: Material DObj segment-E branch expansion probe

Reused:

- The selected material-bearing DObj remains the original Opening Room object
  found by imported `gcDrawAll`/DObj traversal.
- The branch table source remains the original `gcDrawMObjForDObj` contract:
  `ORME` emits the detached branch table/stream into `gSYTaskmanGraphicsHeap`,
  matching the original segment-`E` material branch layout.
- The branch semantics follow the BattleShip/N64 GBI contract and the
  `sm64-nds` renderer reference: `G_DL` with the push bit is treated as a
  nested display-list call; `G_DL_NOPUSH` replaces the current command stream.

Adapted for DS:

- `src/port/scene_backend.c` now runs a second non-visual material-candidate
  display-list probe, `ORMX` (`0x4F524D58`), after the raw `ORMD` scan.
- The probe resolves segmented branch target `0x0E000000` through the emitted
  `ORME` material heap, validates branch targets against loaded O2R payloads or
  the taskman graphics heap, and walks nested display lists with fixed depth and
  command budgets.
- New diagnostics record raw and resolved first branch targets, branch-call and
  branch-jump counts, segment-resolution count, color-state commands, max
  branch depth, and expanded command/vertex/triangle shape.
- `scripts/verify-runtime.ps1` prints and asserts the new contract: `ORMX`
  passes with blocker `0`, raw branch `0x0E000000`, nonzero resolved branch
  pointer, 42 expanded commands, four vertices, four triangles, zero
  unsupported commands, five `G_DL` commands, five call-style branches, two
  segment resolves, six `G_ENDDL`s, five color-state commands, and depth `2`.

Still parked:

- The expanded branch path is still diagnostic-only. It does not yet apply the
  resolved material branch state to the visible renderer.
- Live material branch execution, DS texture upload, combiner/depth mapping,
  continuous draw, and full RDP reset handling remain deferred.

Verification:

- Incremental `make -j4` passed with existing imported-source warnings.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1955
  frames)` after the `ORMX` assertions were added.

## 2026-06-20: Visual HUD overlap and logo presentation cleanup

Reused:

- The startup logo still comes from BattleShip's original `N64Logo` Sprite
  reached through the bounded original startup draw callback.
- The Opening Room preview still comes from the first captured original
  `gcDrawDObjDLLinksForGObj` link-6 display list. No DS-native title/menu art
  or fake menu renderer was added.

Adapted for DS:

- `src/nds/nds_platform.c` now presents the retained startup logo preview at
  DS logical size using the existing filtered sampler instead of showing the
  raw `128x108` texel copy oversized on the 256x192 DS screen.
- The bounded Opening Room DObj preview is displayed smaller and moved beside
  the logo instead of covering the logo's right edge. This reduces live
  melonDS visual noise without changing the source conversion, display-list
  parser, or runtime diagnostics.
- `scripts/debug-melonds.ps1`, `AGENTS.md`, and the docs now describe the
  filtered, non-overlapping visual debugger state.

Still parked:

- This is a debug-HUD presentation pass only. It does not execute live material
  branches, upload textures to DS VRAM, or implement the general RSP/RDP
  renderer.
- Top status bars can still move while startup and Opening Room milestones are
  advancing; use a post-boundary capture to judge persistent flashing.

Verification:

- Incremental `make -j4` passed.
- `scripts/capture-melonds.ps1` captured
  `artifacts/melonds-20260620-063948.png`; visual inspection confirmed the
  logo and Opening Room preview no longer overlap and bottom text remains
  bounded.
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 10 -> Title).`
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1288
  frames).`

## 2026-06-20: Visible bounded material-candidate draw

Reused:

- The rendered object remains the original Opening Room material-bearing DObj
  found by the imported `gcDrawAll` / original DObj callback traversal. No
  fighter, stage, battle, title/menu, or DS-native replacement renderer was
  imported.
- The branch source remains the original `gcDrawMObjForDObj` command-family
  contract already proven by `ORMB`, `ORME`, `ORMP`, `ORMD`, and `ORMX`.

Adapted for DS:

- `src/port/scene_backend.c` now lets the visible bounded `ORDP` preview prefer
  the first material-bearing candidate after `ORMX` succeeds. The visible parser
  follows bounded `G_DL` calls, resolves segment `0x0E000000` through the
  emitted material heap, records branch-call/jump/segment/color counters, and
  uses the captured prim color for the small retained preview.
- `include/nds/nds_startup.h` exposes the new `ORDP` branch/color counters for
  the verifier and HUD diagnostics.
- `scripts/verify-runtime.ps1` now prints and asserts the visible material
  contract: `ORDP`, 42 commands, four vertices, four triangles, five
  call-style branches, zero branch jumps, two segment resolves, five
  color-state commands, prim color `0xFFFFFFFF`, no unsupported opcodes,
  material count `2`, material flags `0x200/0x200`, material mask `0x403`, and
  nonzero pixels.

Still parked:

- This is one bounded material-candidate preview, not a general RSP/RDP or
  object-display renderer.
- Texture source fixup/upload, material/combiner mapping, z-buffering, broad
  display-list branching, full RDP reset state, continuous draw, fighters,
  stages, gameplay, audio, and menus remain deferred.
- The older link-6 preview remains as fallback evidence if material expansion
  fails.

Verification:

- `make clean` passed.
- `make -j4` passed with the existing imported-source warnings.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1278
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-065822.png`; visual inspection confirmed the
  startup logo remains filtered, the selected material-bearing Opening Room DObj
  appears as a small orange original-DL primitive in the preview box, and the
  bottom-screen debug text remains below the overflow area.

## 2026-06-20: Steady melonDS visual debug presentation (superseded)

The later `ORMT material-source scan and no-flash melonDS HUD` entry removes
the remaining top status rail/rectangles. This entry is kept as the historical
step that first removed the moving progress strip and separated the previews.

Reused:

- The startup logo still comes from the original BattleShip `N64Logo` O2R
  Sprite path and the existing bounded `lbCommonDrawSObjAttr` shim.
- The Opening Room preview still comes from the current bounded
  material-candidate `ORDP` path. No original gameplay, scene, fighter, stage,
  audio, or broad renderer code was changed.

Adapted for DS:

- `src/nds/nds_platform.c` now presents the retained startup logo visual copy at
  native `128x108` asset resolution instead of the older downscaled DS-logical
  copy. The source Sprite conversion, overlap rows, and `SP_TEXSHUF` odd-row
  TMEM line unswizzle are unchanged.
- The bounded Opening Room preview moved to the lower-left corner, away from
  the native-size logo.
- The moving top-screen progress strip, bottom tick bars, and orange logo-Y
  marker were removed from the top framebuffer so live melonDS output no longer
  reads as screen flashing. The bottom text HUD and GDB verifier still expose
  those runtime diagnostics.
- `gNdsOriginalSpritePreviewDisplayWidth` and
  `gNdsOriginalSpritePreviewDisplayHeight` were added so
  `scripts/verify-runtime.ps1` proves the visible startup logo presentation is
  `128x108`.
- Retained preview draw counters now increment at draw start, and the smaller
  DL preview is drawn before the larger logo. This removes a GDB sampling race
  where the verifier could halt inside the logo blit before the DL preview
  draw counter advanced.

Still parked:

- This is a visual-debug presentation cleanup only. General material/combiner
  mapping, texture upload, z-buffering, broad display-list branching, full RDP
  reset state, continuous draw, fighters, stages, gameplay, audio, and menus
  remain deferred.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1325
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 15 -> Title).`

## 2026-06-20: ORMT material-source scan and no-flash melonDS HUD

Reused:

- The material records still come from BattleShip's generated MVCommon
  relocation data and the imported Opening Room `gcDrawAll`/DObj traversal.
- The visible previews remain the bounded original `N64Logo` Sprite and the
  bounded Opening Room material-candidate `ORDP` path. No DS-native title/menu
  renderer, fighter, stage, battle, audio, or broad renderer code was imported.

Adapted for DS:

- `src/port/scene_backend.c` widened the MVCommon `MObjSub` mixed-width
  normalizer from the earlier logo/spotlight subset to 18 records across the
  background, logo, close-up air, close-up ground, desk ground, and spotlight
  tables.
- The native-field detector is now MVCommon-specific (`0`, `0x200`, and
  `0x1200`) so accidental swapped values such as `0x40` are not treated as
  already normalized.
- `include/nds/nds_startup.h` and `scripts/verify-runtime.ps1` now expose and
  assert the ORMT source scan: marker `0x4F524D54`, 18 normalized records, zero
  texture-flagged records, zero zero-flag fallback records, 18 prim-color
  records, one light-bearing record, first texture offset `0xFFFFFFFF`, and
  first texture flags `0`.
- `src/nds/nds_platform.c` removed the top-screen status rail/rectangles. The
  top framebuffer now shows only original-asset previews: the native `128x108`
  startup logo and the lower-left Opening Room preview. Bottom text still
  carries the verifier-visible diagnostics and remains bounded.

Still parked:

- `ORTX=0` still proves that the current selected material-source path exposes
  no texture-capable original `MObj` records. Texture-bearing material-source
  exposure/fixup and DS texture upload remain the next renderer blocker.
- The startup logo can look coarse in melonDS because the diagnostic copy is
  the native `128x108` asset magnified by the emulator window. That is expected
  debug presentation, not final renderer quality.

Verification:

- `make clean` passed before the source/header verifier changes.
- `make -j4` passed after the HUD cleanup.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1299
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 14 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-075754.png`; visual inspection confirmed the top
  status bars are gone, the native startup logo and lower-left Opening Room
  preview remain visible, and the bottom text stays inside the screen.
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-072403.png`; visual inspection confirmed the
  native-size startup logo, the lower-left Opening Room preview, the then-current
  compact top status rail without moving progress/orange markers, and bounded
  bottom-screen debug text. The later no-flash HUD entry removes that rail.

## 2026-06-20: Texture-material source scan

Reused:

- The scanned objects still come from the imported Opening Room
  `gcDrawAll`/DObj callback traversal and the current material-bearing DObj
  selector. No fighter, stage, battle, audio, or menu systems were imported.
- The source contract remains BattleShip's original `gcDrawMObjForDObj`
  material flag behavior from `sys/objdisplay.c`.

Adapted for DS:

- `src/port/scene_backend.c` now runs a bounded source-side scan for `MObj`
  records whose effective flags include `MOBJ_FLAG_TEXTURE`, with safe
  sprite-array/current/next pointer evidence when such a source exists.
- `include/nds/nds_startup.h` exposes the `ORTX` diagnostic globals and
  `scripts/verify-runtime.ps1` prints/asserts the consistency contract.
- The current verified result is intentionally precise: `ORTX=0`, candidates
  `0`, MObjs `0`, sprite arrays `0`, current sprites `0`, next sprites `0`,
  sentinel object fields, and null texture-source pointers.

Still parked:

- The selected material-bearing Opening Room path is still prim-color only.
  Texture-bearing material-source exposure/fixup, DS texture upload, broader
  material/combiner behavior, z-buffering, continuous draw, fighters, stages,
  gameplay, audio, and menus remain deferred.
- This does not change the visible HUD. The prior melonDS capture remains valid
  for the visual presentation because this pass added non-visual diagnostics.

Verification:

- `make clean` passed.
- `make -j4` passed with the existing imported-source warnings.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1294
  frames).`
- A one-off GDB sample reported
  `OPENING_ROOM_DRAW_TEXTURE_MATERIAL=0,0,0,0,0,0`.
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 15 -> Title).`

## 2026-06-20: NDS display-list renderer adapter boundary

Reused:

- The active render proof still starts from the imported Opening Room
  `gcDrawAll`/DObj traversal and the current material-bearing candidate. No
  fighter, stage, battle, audio, or menu systems were imported.
- `decomp/sm64-nds/src/nds/nds_renderer.c` was used as the architectural
  reference for the backend shape: bounded N64 `Gfx` command walking,
  branch-following, renderer-owned state collection, and platform-layer
  growth. Its SM64-specific graph, memory, texture cache, sprite assets, and
  draw entrypoints were not copied into this repo.

Adapted for DS:

- Added `include/nds/nds_renderer.h` and `src/nds/nds_renderer.c` as the
  project-owned DS renderer adapter boundary.
- Moved the `ORMX` branch-expanded material-DObj command walk out of
  `src/port/scene_backend.c` and into the adapter. The scene bridge now
  supplies only the Smash-specific loaded-file/graphics-heap range validator
  and segment-`E` material-heap branch resolver.
- The adapter currently scans/proves the same bounded command family:
  42 commands, four vertices, four triangles, five `G_DL` commands, five
  call-style branches, two segment resolves, six `G_ENDDL`s, five color-state
  commands, max depth `2`, and unsupported opcode count `0`.

Still parked:

- This is not a full renderer import and does not yet execute the material
  branches into libnds polygons. General material/combiner mapping,
  texture-bearing material-source exposure/fixup, DS texture upload,
  z-buffering, full RDP reset state, continuous draw, fighters, stages,
  gameplay, audio, and menus remain deferred.
- The visible frame is intentionally unchanged: startup logo plus the bounded
  Opening Room preview. The logo remains coarse in melonDS because it is a
  native `128x108` diagnostic copy magnified by the emulator window.

Verification:

- `make clean` passed.
- `make -j4` passed with the existing imported-source warnings.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1291
  frames).`
- `scripts/verify-opening-skip.ps1` passed with `Opening Room skip
  verification passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-081929.png`; visual inspection confirmed the top
  status rail/bars remain absent, the native startup logo and lower-left
  Opening Room preview remain visible, and the bottom text stays inside the
  screen.

## 2026-06-20: ORDP traversal through NDS renderer callback

Reused:

- The visible Opening Room preview still starts from the imported
  `gcDrawAll`/DObj callback path and the current material-bearing DObj selected
  after `ORMX` succeeds. No fighter, stage, battle, menu, audio, or broad
  renderer subsystem was imported.
- The per-command decode for Smash-specific vertex data, texture state,
  combiner diagnostics, camera projection, and software preview rasterization
  remains in `src/port/scene_backend.c` for this bounded proof.

Adapted for DS:

- `include/nds/nds_renderer.h` now exposes a bounded command visitor API.
- `src/nds/nds_renderer.c` now supports `ndsRendererExecuteDisplayList`, which
  owns command budgets, depth limits, pointer validation, `G_DL` call/jump
  traversal, and segment-resolution reporting while calling a scene-provided
  command visitor.
- The visible `ORDP` parser no longer recursively walks display-list branches
  in `scene_backend.c`; it receives command events from the renderer adapter.
  This preserves the current 42-command/four-triangle material-candidate
  preview while moving traversal toward the DS backend layer modeled after
  `sm64-nds`.

Still parked:

- This still software-previews one selected original display-list path. It does
  not yet submit the material branch stream to libnds polygon hardware, upload
  texture-backed material sources, implement a general combiner/depth backend,
  run continuous draw, or expand into fighters/stages/gameplay/audio.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with `Runtime verification passed (1301
  frames).`
- `scripts/verify-opening-skip.ps1` initially hit a melonDS GDB listener launch
  issue before the clean build, then the clean-build verifier passed with
  `Opening Room skip verification passed (tick 115 -> Title).`
- `scripts/capture-melonds.ps1 -Build` captured
  `artifacts/melonds-20260620-083511.png`; visual inspection confirmed the top
  status rail/bars remain absent, the native startup logo and lower-left
  Opening Room preview remain visible, and the bottom debug text remains
  bounded.

## 2026-06-20: Opening Portraits movie-scene boundary

Reused:

- Imported original BattleShip `mvopeningportraits.c` through
  `src/import/battleship_mvopeningportraits.c`.
- Kept `decomp/` read-only and kept scene/taskman behavior in the existing
  original scene-manager path.
- Reused the narrow DS SObj preview contract proven by Startup instead of
  drawing portrait cards with a DS-native title/menu path.

Adapted for DS:

- Added `MVOpeningPortraitsSet1` and `MVOpeningPortraitsSet2` to the NitroFS
  relocation manifest.
- Added a narrow Opening Portraits Sprite/Bitmap metadata normalizer for the
  two portrait O2Rs: four Set1 cards, four Set2 cards, and the Set1 cover.
- Advanced the bounded Opening Room seam to tick `1320`, where it requests
  `nSCKindOpeningPortraits`.
- Added an Opening Portraits task seam that runs bounded original updates to
  tick `150`, performs bounded SObj preview draws, and records the original
  next-scene request to `nSCKindOpeningMario`.
- Added verifier markers for portrait relocation, sprite normalization,
  successful `300x55` RGBA16 card drawing, accumulated pixels, and next-scene
  kind.
- Added compact bottom-screen HUD rows for the movie handoff and portrait
  render state without wrapping the DS text console.

Still parked:

- `mvOpeningMarioStartScene` and the rest of the opening movie sequence are
  still scene stubs. The remaining natural movie path is Mario, Donkey, Samus,
  Fox, Link, Yoshi, Pikachu, Kirby, Run, Yoster, Cliff, Standoff, Yamabuki,
  Clash, Sector, Jungle, Newcomers, then Title.
- The verified title path remains the original Opening Room A-button skip
  branch; the full natural rendered opening movie to Title is not complete.
- The portrait cover Sprite is I4 and intentionally skipped by the current
  RGBA16-only preview path after the card sprites have rendered.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (1633 frames).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 21 -> Title).`
- `scripts/capture-melonds.ps1 -DelaySeconds 75` captured
  `artifacts/melonds-20260620-104057.png`; visual inspection confirmed the
  original Opening Portraits card sprites render on the top screen and the
  bottom HUD text remains bounded.

## 2026-06-20: Natural opening movie to bounded Title preview

Reused:

- Imported original BattleShip Opening Mario and the Donkey, Link, Samus,
  Yoshi, Kirby, Fox, and Pikachu name-card scenes through `src/import`.
- Kept `decomp/` read-only and kept scene transitions flowing through the
  original scene manager/taskman path.
- Reused the bounded SObj preview path for original movie/name-card sprites
  rather than drawing DS-native replacement title or fighter-card art.

Adapted for DS:

- Added the smallest shared name-card wrapper contract for the imported
  fighter-card scenes, with diagnostics for dispatch, `func_start`, bounded
  update, deferred fighter branches, next-scene requests, and SObj draw counts.
- Added a bounded action-scene bridge from `OpeningRun` through
  `OpeningNewcomers` in original scene order. These scenes are still parked
  because their fighter/stage gameplay dependencies are outside this slice.
- Added `MNTitle` (`file 167`) to the NitroFS relocation manifest and added
  project-owned symbol offsets for ten selected original Title Sprite/Bitmap
  records.
- Added a narrow Title Sprite/Bitmap normalizer plus I4, IA8, RGBA16, and
  RGBA32 SObj decode support needed by those selected `MNTitle` records.
- Replaced the old Title scene stub with a bounded original-asset preview:
  `mnTitleStartScene` loads `MNTitle`, normalizes the selected sprites, draws
  them into the top-screen preview, records Title diagnostics, and then parks at
  the scene boundary.
- Kept bottom HUD text bounded by reusing the existing row budget and replacing
  the final movie row with compact bridge/Title-preview/pixel diagnostics.

Still parked:

- Full `mntitle.c` task setup, menu state, title animation timing, particles,
  audio, input behavior, and continuous title draw are not imported yet.
- The action-scene bridge does not render fighter/stage-heavy movie shots yet.
  Those should be imported one bounded original asset at a time or deferred
  until fighter/stage dependencies are ready.
- This is still not a DS-native title/menu renderer and does not change
  gameplay systems.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (2821 frames).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 20 -> Title).`
- `scripts/capture-melonds.ps1 -Build -DelaySeconds 135` captured
  `artifacts/melonds-20260620-121350.png`; visual inspection confirmed the
  bounded original `MNTitle` sprite composite renders on the top screen and the
  bottom HUD remains clean and bounded.

## 2026-06-20: Paced action-scene preview bridge

Reused:

- Kept the original scene order from `OpeningRun` through `OpeningNewcomers`.
- Loaded original O2R action-era Sprite assets from `MVOpeningRun`,
  `MVOpeningYamabuki`, and `MVOpeningSector` through the existing NitroFS
  relocation path.

Adapted for DS:

- Added a bounded action preview descriptor table in `src/port/scene_backend.c`
  and held each action-scene bridge for 36 presented frames.
- Added CI8 Sprite support for the Run wallpaper, including palette-index
  decoding after the blanket O2R word byte-swap.
- Added runtime diagnostics for action preview result, mask, count, held
  frames, pixels, normalizer count/failures, and final sprite meta.
- Kept bottom HUD text bounded by compacting the final movie row to
  `br=... ap=... ti=...`.

Still parked:

- The fighter/stage-heavy action scene object graphs remain bridge stubs. This
  is a watchable original-asset preview path, not imported action-scene
  rendering.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (3201 frames)` before the later pacing/cache
  update superseded the frame count.
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build -DelaySeconds 145` captured
  `artifacts/melonds-20260620-125443.png`; visual inspection confirmed the
  full paced opening path reaches the bounded Title preview. The later
  pacing/cache update supersedes the old bottom HUD row.

## 2026-06-20: Opening movie pacing and emulator layout

Reused:

- Kept the imported BattleShip startup, Opening Room, portrait, name-card, and
  action bridge control flow intact.
- Kept melonDS as the automated ARM9 GDB verifier.

Adapted for DS:

- Added a narrow DS frame-present helper at existing bounded draw/probe
  boundaries instead of unbounding the original task loops.
- Presented the retained startup logo after update 17, then periodically during
  early Opening Room ticks before renderable DObj slices exist.
- Presented one DS frame after bounded Opening Room, Portraits, Mario, and
  name-card draw probes.
- Cached the three unique action-preview Sprite sources from `MVOpeningRun`,
  `MVOpeningYamabuki`, and `MVOpeningSector` so the nine action boundaries do
  not repeatedly reload and normalize the same O2R payloads.
- Added `emulators/` as the local emulator home, moved melonDS config/runtime
  files under `emulators/melonds/`, added `emulators/nogba/` as the no$gba
  drop-in/debugger location, and routed emulator logs to
  `artifacts/emulator-logs/`.
- Added `scripts/debug-nogba.ps1` for interactive no$gba hardware, VRAM, OAM,
  palette, register, and timing debugging.

Still parked:

- no$gba is not an automated verifier yet. Use it for manual hardware
  inspection and keep melonDS/GDB as the pass/fail runtime check.
- The action scenes are still bounded original-asset previews, not imported
  fighter/stage-heavy movie rendering.

Verification:

- `make -j4` passed after the pacing changes.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 48 -> Title).`
- `scripts/capture-melonds.ps1 -DelaySeconds 2` captured
  `artifacts/melonds-20260620-133100.png`, proving the visual capture script
  finds melonDS from `emulators/melonds/`.
- `scripts/verify-nogba-smoke.ps1 -DelaySeconds 1` passed with one capture:
  `artifacts/nogba-smoke-20260620-135932-w00.png`,
  `No$gba Debugger (Fullversion)` at `970x464`. The capture scripts still
  enumerate all no$gba windows so they tolerate a separate emulator window if
  that local setting is re-enabled.

## 2026-06-20: Renderer-adapter texture state diagnostics

Reused:

- Kept the bounded Opening Room material-candidate `ORDP` path and the original
  BattleShip object/display traversal unchanged.
- Used `sm64-nds/src/nds/nds_renderer.c` as the reference for keeping texture,
  tile, load-block, texture-scale, and branch traversal state inside the DS
  renderer adapter instead of growing scene-local renderer logic.

Adapted for DS:

- Extended `src/nds/nds_renderer.c` / `include/nds/nds_renderer.h` so
  `NDSRendererStats` records adapter-owned `G_TEXTURE`, `G_SETTIMG`,
  `G_SETTILE`, `G_LOADBLOCK`, `G_SETTILESIZE`, `G_SETCOMBINE`,
  `G_SETPRIMCOLOR`, and geometry-mode state while preserving bounded
  `G_DL` recursion.
- Added runtime globals for the renderer adapter's texture-state view of the
  visible `ORDP` stream and asserted them in `scripts/verify-runtime.ps1`:
  texture mask `0x3F`, nonzero I16 image pointer, fmt/size/width `4/2/1`,
  load texels `256`, four set-tile commands, one texture command, texture-state
  flags `0x0F`, tile `16x32`, render tile `0`, line `4`, flags `0xB7`, load
  block `lrs/dxt` `255/1024`.

Still parked:

- This is renderer-state proof only. DS hardware texture upload, polygon
  submission, depth, and general combiner/material behavior remain deferred.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -DelaySeconds 8` captured
  `artifacts/melonds-20260620-142844.png`.
- `scripts/verify-nogba-smoke.ps1 -DelaySeconds 1` passed with one capture:
  `artifacts/nogba-smoke-20260620-142415-w00.png`,
  `No$gba Debugger (Fullversion)` at `970x464`.

## 2026-06-20: Low-noise FPS and content-cadence HUD diagnostic

Reused:

- Kept the existing double-buffered DS HUD and change-driven bottom console
  redraw behavior in `src/nds/nds_platform.c`.
- Kept the imported BattleShip startup/opening/movie/Title runtime path
  unchanged.

Adapted for DS:

- Added sampled performance/content rows to the final bottom HUD lines:
  `fps=.. up=.. dl=.. cv=..` and `ch=... pf=... smp=.. win=..`.
- Added public runtime counters in `include/nds/nds_platform.h`:
  `gNdsPerfPresentFps`, `gNdsPerfLogicFps`, `gNdsPerfDLDrawFps`,
  `gNdsPerfPreviewCommitFps`, `gNdsPerfPreviewCommitCount`,
  `gNdsPerfSampleCount`, and `gNdsPerfSampleWindowTicks`.
- Added `gNdsOriginalSpritePreviewCommitCount`, paired with the existing
  DObj preview commit count, so the verifier can distinguish repeated VBlank
  presentation from actual retained original-preview content changes.
- The sampler updates about once per 60 DS VBlanks and is included in the
  debug-text fingerprint, so the HUD updates when a new sample exists without
  pulsing from raw live frame counters.
- `fps` is the ROM-side VBlank-relative present rate, `up` is imported
  opening-movie update cadence, `dl` is retained DObj-preview draw cadence,
  `cv` is original-preview commits per sampled second, `ch` is total
  original-preview commits, and `pf` is the paced opening movie present-frame
  count.
- Extended `scripts/verify-runtime.ps1` to report `verifyfps=...`, a fixed
  long-window verifier average. This keeps the pass/fail verifier stable while
  making clear that its timing includes the deliberate worst-case wait.
- Added `scripts/sample-runtime-speed.ps1`, a one-shot hidden melonDS+GDB
  sampler for wall-clock speed profiling over a chosen delay. This gives
  renderer/performance work a real host-speed sample without changing the main
  verifier gate.

Still parked:

- This is not host wall-clock emulator FPS. Use melonDS/no$gba host-speed
  indicators, capture timing, or deeper hardware profiling for symptoms such
  as the emulator window visibly advancing one frame every several seconds.
- Repeated GDB polling is not viable with the current melonDS setup. A test
  loop that repeatedly attached/detached to wait for Title produced packet
  errors and prevented progress; use one-shot samples instead.
- The update sample can be `0` after the runtime has parked at the Title
  boundary because the original opening update counters are no longer moving.
- The content-rate sample can also be `0` at the final parked Title boundary;
  use the total `ch` count and shorter visible captures to see early movie
  content progress.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.34).`
- `scripts/sample-runtime-speed.ps1 -DelaySeconds 8` passed with
  `Runtime speed sample (9.3s): frames=505 hostfps=54.23 romfps=60 up=58 dl=0 cv=0 ch=1 present=19 room=421 portraits=0 mario=0 action=0/0 title=0`.
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -DelaySeconds 8` captured
  `artifacts/melonds-20260620-145554.png`; visual inspection confirmed the
  earlier single-line `fps=60 cv=00 ch=001 pf=011` row fit and the bottom HUD
  remained bounded before the later split-row update.
- `scripts/verify-nogba-smoke.ps1 -DelaySeconds 1` passed with one capture:
  `artifacts/nogba-smoke-20260620-145612-w00.png`,
  `No$gba Debugger (Fullversion)` at `970x464`.

## 2026-06-20: FPS counter used to reject unsafe cadence-only speedups

Reused:

- Kept the FPS/content-cadence HUD and one-shot host-speed sampler from the
  previous pass.
- Kept the checked-in opening movie preview cadence at the verified
  `NDS_OPENING_MOVIE_DRAW_INTERVAL=30`.

Tested and rejected:

- `NDS_OPENING_MOVIE_DRAW_INTERVAL=10` improved the short early-present sample
  (`present=43`, `hostfps=54.13`) but the full runtime verifier no longer
  reached the action/Title boundary in its window.
- `NDS_OPENING_MOVIE_DRAW_INTERVAL=20` improved the short early-present sample
  (`present=25`, `hostfps=54.30`) and reached Title, but the strict Opening
  Room DObj blocker assertion became timing-sensitive while the emulator was
  sampled during presentation.

Conclusion:

- The FPS row is useful, but the current slowdown is not solved by committing
  bounded previews more often. The ROM-side present rate remains `60`, while
  retained original content commits remain sparse; the next performance work
  should reduce renderer/preview cost before raising the cadence again.

Verification after restoring the verified cadence:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.33).`
- `scripts/sample-runtime-speed.ps1 -DelaySeconds 8` passed with
  `Runtime speed sample (9.6s): frames=521 hostfps=54.36 romfps=60 up=58 dl=0 cv=0 ch=1 present=19 room=437 portraits=0 mario=0 action=0/0 title=0`.
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build -DelaySeconds 8` captured
  `artifacts/melonds-20260620-152537.png`; visual inspection confirmed the
  compact `fps/cv/ch/pf` row still fits and the bottom HUD remains bounded.

## 2026-06-20: Fast retained SObj preview scaler

Reused:

- Kept original BattleShip Sprite/SObj data, scene traversal, and preview draw
  callers unchanged.
- Kept the verified opening movie draw interval at `30`.

Adapted for DS:

- Replaced the active retained-preview bilinear scaler in
  `src/nds/nds_platform.c` with a faster path: native-size previews copy rows
  directly, and full-screen original SObj preview buffers scale from `320x240`
  to the DS top screen's `256x192` using a nearest-neighbor source sample.
- `ndsPlatformBeginOriginalSpritePreview` now clears only the active source
  preview area instead of clearing both full `320x240` source/display buffers
  on every begin call.
- Removed the now-unused bilinear accumulator/sampler to keep the build
  warning-clean.

Still parked:

- This does not optimize the Opening Room DObj software preview or convert it
  to DS hardware polygons. The longer speed sample still points at repeated
  Opening Room DL preview work as the next dominant cost.
- This is a diagnostic presentation optimization, not a gameplay or scene
  rewrite.

Verification:

- `make -j4` passed without warnings.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.32).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 10 -> Title).`
- `scripts/sample-runtime-speed.ps1 -DelaySeconds 8` passed with
  `Runtime speed sample (9.3s): frames=508 hostfps=54.37 romfps=60 up=58 dl=0 cv=0 ch=1 present=19 room=424 portraits=0 mario=0 action=0/0 title=0`.
- `scripts/sample-runtime-speed.ps1 -DelaySeconds 45` passed with
  `Runtime speed sample (46.3s): frames=1603 hostfps=34.65 romfps=60 up=59 dl=60 cv=1 ch=5 present=53 room=1320 portraits=150 mario=7 action=0/0 title=0`.
- `scripts/capture-melonds.ps1 -Build -DelaySeconds 12` captured
  `artifacts/melonds-20260620-153931.png`; visual inspection confirmed the
  bottom HUD remains bounded and the retained original previews are still
  visible.
- `scripts/verify-nogba-smoke.ps1 -DelaySeconds 1` passed with
  `artifacts/nogba-smoke-20260620-153954-w00.png`.

## 2026-06-20: Cached retained DL preview presentation

Reused:

- Kept the original Opening Room DObj/display-list traversal, renderer-adapter
  parse evidence, and retained `96x72` preview source buffer unchanged.
- Kept the verified opening movie draw interval at `30`.

Adapted for DS:

- Added a `72x54` cached display buffer in `src/nds/nds_platform.c` for the
  retained Opening Room DL preview.
- `ndsPlatformCommitOriginalDLPreview` now builds the scaled display copy once
  per original DL preview commit using the same nearest source sampling shape
  as the previous per-frame draw path.
- `ndsPlatformDrawOriginalDLPreview` now draws the frame/border and blits the
  cached rows directly. Transparent source pixels are prefilled with the same
  dark preview background color the old draw path left behind.

Result:

- The longer one-shot host-speed sample now gets substantially farther through
  the natural movie path. Before this pass, the 45-second sample reached
  Opening Room tick `1320`, Portraits tick `150`, and Mario tick `7` at
  `hostfps=34.65`. After caching the scaled DL presentation copy, the same
  sampling window reaches Opening Room tick `1320`, Portraits tick `150`,
  Mario tick `60`, and action bridge progress `4/126` at `hostfps=48.81`.

Still parked:

- This is still a retained diagnostic preview, not DS hardware polygon
  rendering. The next renderer work should reduce or replace the software
  Opening Room DObj preview/render path itself.

Verification:

- `make -j4` passed without warnings.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.34).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 10 -> Title).`
- `scripts/sample-runtime-speed.ps1 -DelaySeconds 8` passed with
  `Runtime speed sample (9.3s): frames=503 hostfps=54.28 romfps=60 up=58 dl=0 cv=0 ch=1 present=18 room=420 portraits=0 mario=0 action=0/0 title=0`.
- `scripts/sample-runtime-speed.ps1 -DelaySeconds 45` passed with
  `Runtime speed sample (46.2s): frames=2255 hostfps=48.81 romfps=60 up=60 dl=60 cv=2 ch=25 present=203 room=1320 portraits=150 mario=60 action=4/126 title=0`.
- `scripts/capture-melonds.ps1 -Build -DelaySeconds 12` captured
  `artifacts/melonds-20260620-154831.png`; visual inspection confirmed the
  lower-left Opening Room preview still renders and the bottom HUD remains
  bounded.
- `scripts/verify-nogba-smoke.ps1 -DelaySeconds 1` passed with
  `artifacts/nogba-smoke-20260620-154852-w00.png`.

## 2026-06-20: Split visible FPS/update/draw HUD counters

Reused:

- Kept the existing ROM-side low-noise performance sampler in
  `src/nds/nds_platform.c`.
- Kept the verifier-visible `gNdsPerfPresentFps`, `gNdsPerfLogicFps`,
  `gNdsPerfDLDrawFps`, `gNdsPerfPreviewCommitFps`,
  `gNdsPerfPreviewCommitCount`, `gNdsPerfSampleCount`, and
  `gNdsPerfSampleWindowTicks` counters unchanged.
- Kept the original BattleShip startup/opening/movie/Title runtime path
  unchanged.

Adapted for DS:

- Split the final bottom HUD performance text into two fixed-width rows:
  `fps=.. up=.. dl=.. cv=..` and `ch=... pf=... smp=.. win=..`.
- Exposed the already-sampled imported update cadence (`up`) and retained
  DObj-preview draw cadence (`dl`) on the visible HUD, instead of keeping them
  only in GDB/verifier output.
- Kept the rows below the DS text-console width and under the bottom-screen row
  limit so the HUD remains clean in live melonDS captures.

Still parked:

- These are ROM-side/VBlank-relative counters. Host wall-clock emulator speed
  still comes from `scripts/sample-runtime-speed.ps1`, melonDS/no$gba timing,
  or capture observation.
- This does not reduce renderer cost by itself; it makes the current update,
  draw, and preview-content cadence visible while renderer work continues.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.33).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 10 -> Title).`
- `scripts/capture-melonds.ps1 -Build -DelaySeconds 12` captured
  `artifacts/melonds-20260620-155811.png`; visual inspection confirmed
  `fps=60 up=58 dl=03 cv=01` and `ch=002 pf=018 smp=11 win=60` fit without
  wrapping or bottom-screen overflow.
- `scripts/verify-nogba-smoke.ps1 -DelaySeconds 1` passed with
  `artifacts/nogba-smoke-20260620-155851-w00.png`.

## 2026-06-20: Opening Room retained-preview reuse diagnostics

Reused:

- Kept original Opening Room update logic, taskman/object traversal, and the
  bounded `scene_draw -> gcDrawAll -> func_80017EC0 -> gcCaptureCameraGObj`
  diagnostic path.
- Kept the retained Opening Room DObj preview contents and renderer-adapter
  `ORDP` evidence unchanged.

Adapted for DS:

- Added `gNdsOpeningRoomDrawProbeCount` and
  `gNdsOpeningRoomDrawReuseCount`.
- Limited the natural Opening Room movie path to two actual original
  `scene_draw` probes: the first post-tick-560 probe and the final tick-1320
  handoff probe.
- Replaced the intervening repeated `gcDrawAll` diagnostics with retained
  preview presentation through `ndsOpeningMoviePresentFrame`.
- Extended `scripts/verify-runtime.ps1` to require two actual draw probes and
  retained-preview reuse evidence.
- Extended `scripts/sample-runtime-speed.ps1` to report the draw probe/reuse
  pair as `rdraw=probes/reuses`.

Result:

- The change proves repeated Opening Room draw probes were not the dominant
  slowdown. The 45-second sample now reports `rdraw=2/24`, but host speed
  remains effectively unchanged from the prior `48.81` FPS sample.
- The next performance target should move to the remaining opening
  movie/action-preview bridge path rather than repeated Opening Room
  `scene_draw` diagnostics.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.34).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 39 -> Title).`
- `scripts/sample-runtime-speed.ps1 -DelaySeconds 45` passed with
  `Runtime speed sample (46.3s): frames=2257 hostfps=48.74 romfps=60 up=60 dl=60 cv=2 ch=25 present=205 room=1320 rdraw=2/24 portraits=150 mario=60 action=4/128 title=0`.
- `scripts/capture-melonds.ps1 -Build -DelaySeconds 12` captured
  `artifacts/melonds-20260620-160953.png`; visual inspection confirmed the
  retained startup logo and lower-left Opening Room preview remain visible, and
  the bottom HUD stays bounded.
- `scripts/verify-nogba-smoke.ps1 -DelaySeconds 1` passed with
  `artifacts/nogba-smoke-20260620-161016-w00.png`.

## 2026-06-20: Full opening movie speed gate and source-first priority

Reused:

- Kept the existing `sample-runtime-speed.ps1` melonDS+GDB sampling path and
  its ROM-side performance/content counters.
- Kept the current bounded startup, Opening Room, opening movie, action bridge,
  and Title preview visuals unchanged.

Adapted for DS:

- Added optional pass/fail gates to `scripts/sample-runtime-speed.ps1`:
  required Title marker, minimum Opening Room tick, minimum action-preview
  count/frame count, and minimum hidden-melonDS host FPS.
- Added `scripts/verify-opening-movie-speed.ps1` as the maintained full
  opening movie-to-Title speed verifier. The default gate requires Title marker
  `0x54494457`, Opening Room tick `1320`, all nine action-preview boundaries,
  at least `324` paced action-preview frames, and `hostfps >= 30`.
- Updated docs to treat visual captures and renderer diagnostics as regression
  evidence while moving the next implementation priority back to original
  BattleShip scene/menu/game-code imports with minimal bounded rendering.

Still parked:

- The action-scene bridge remains a bounded source-order bridge, not imported
  fighter/stage-heavy action scene object graphs.
- Full `mntitle.c`, real title menu behavior, continuous title drawing, audio,
  fighters, stages, and gameplay remain deferred.
- Renderer fidelity work is still required for the final port, but it should
  wait unless the next original source import needs a missing renderer
  contract.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.34).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 10 -> Title).`
- `scripts/verify-opening-movie-speed.ps1` passed with
  `Runtime speed sample (82.0s): frames=3318 hostfps=40.44 romfps=60 up=0 dl=60 cv=0 ch=32 present=401 room=1320 rdraw=2/24 portraits=150 mario=60 action=9/324 title=0x54494457`.
- `scripts/verify-nogba-smoke.ps1 -DelaySeconds 1` passed with
  `artifacts/nogba-smoke-20260620-162403-w00.png`.

## 2026-06-20: Bounded original Title setup import and decomp map

Reused:

- Imported original BattleShip `mn/mncommon/mntitlefiles.c` through
  `src/import/battleship_mntitlefiles.c`.
- Imported original BattleShip `mn/mncommon/mntitle.c` through
  `src/import/battleship_mntitle.c`, with the full original start functions
  renamed behind a bounded DS wrapper.
- Kept the prior bounded original `MNTitle` SObj preview path as the visual
  parking signal instead of adding DS-native title/menu drawing.
- Treated the entire `decomp/` tree as read-only reference material and added
  `docs/DECOMP_MAP.md` to map BattleShip and sm64-nds folders by usefulness.

Adapted for DS:

- Added narrow Title ABI shims for `mn/mndef.h`, `mn/mntypes.h`, backup masks,
  audio stubs, particle stubs, Sprite prep helpers, and the GBI no-op macros
  needed by the imported Title slice.
- Added `MNTitleFireAnim` (`file 168`) to the NitroFS relocation manifest and
  file-ID mapping alongside `MNTitle` (`file 167`).
- Replaced the old direct `mnTitleStartScene` preview stub with a bounded
  original Title task setup. The wrapper preserves the original video/task
  settings, calls original `mnTitleLoadFiles`, `mnTitleMakeActors`,
  `mnTitleMakeCameras`, and `mnTitleInitVars`, then records explicit deferral
  of particle, fire, logo, labels, slash, and logo-fire particle paths.
- Scoped Opening Room relocation byte/swap/pointer diagnostics back to the
  eight Opening Room O2R payloads so later Title loads do not overwrite that
  proof. The global payload count now covers the full staged runtime set.
- Extended `scripts/verify-runtime.ps1` with `TITLE_ORIGINAL` checks and updated
  both runtime and skip verifiers for the revised payload accounting.

Still parked:

- Title input behavior, particles, fire/logo/label/slash animation state,
  audio, continuous title draw, and menu transitions remain deferred.
- The imported original Title wrapper deliberately stops before unbounded
  `syTaskmanRunTask`.
- Fighter/stage-heavy action-scene internals remain bridged, not imported.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.32).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 41 -> Title).`
- `scripts/verify-opening-movie-speed.ps1` passed with
  `Runtime speed sample (85.2s): frames=3396 hostfps=39.87 romfps=60 up=0 dl=60 cv=0 ch=32 present=401 room=1320 rdraw=2/24 portraits=150 mario=60 action=9/324 title=0x54494457`.

## 2026-06-20: Guarded original Title update tick

Reused:

- Kept the imported original `mntitle.c` / `mntitlefiles.c` wrapper as the
  Title source boundary.
- Reused original `mnTitleFuncUpdate`, which calls `gcRunAll`, instead of
  adding a DS-native Title update loop.
- Preserved the existing bounded Title SObj preview as the visual parking
  signal.

Adapted for DS:

- Added `ndsMNTitleRunBoundedUpdates(1)` in the Title import wrapper. The helper
  runs exactly one original Title update tick only when the scene path is the
  natural `OpeningNewcomers -> Title` handoff.
- Added diagnostics for the original Title update marker and sampled state:
  `TITLE_ORIGINAL_UPDATE=0x54495550,1,0,1,1,0,3`.
- Guarded the skip-to-Title path from running the update tick. That path starts
  in the non-opening Title layout with transition tick `169`, where the next
  original transition update expects logo objects that are still deliberately
  deferred. The skip verifier proves the guard with
  `TITLE_ORIGINAL_UPDATE=0,0,1,169,0,0,3`.
- Extended `scripts/verify-runtime.ps1` and
  `scripts/verify-opening-skip.ps1` to cover the new update evidence.

Still parked:

- Full Title object creation for animated logo, labels, Press Start, slash,
  particles, audio, input transitions, and continuous draw remains deferred.
  Fire object creation was still deferred at this point, but is superseded by
  the later bounded `mnTitleMakeFire` import in this log.
- The action-scene bridge remains a bounded scene-order bridge, not imported
  fighter/stage-heavy movie rendering.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.32).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 80 -> Title).`
- `scripts/verify-opening-movie-speed.ps1` passed with
  `Runtime speed sample (82.0s): frames=3303 hostfps=40.27 romfps=60 up=0 dl=60 cv=0 ch=32 present=401 room=1320 rdraw=2/24 portraits=150 mario=60 action=9/324 title=0x54494457`.

## 2026-06-20: Bounded original Title logo-fire object boundary

Reused:

- Kept original BattleShip `mntitle.c` as the source of truth for Title object
  creation.
- Called original `efParticleInitAll` and `mnTitleMakeLogoFire` from the
  project-owned Title wrapper instead of creating a DS-native logo-fire stand-in.
- Preserved the existing bounded Title SObj preview as the visual parking
  signal.

Adapted for DS:

- Added `ndsMNTitleMakeLogoFireBounded`, which runs the original logo-fire
  setup after the original Title actor creation and before original camera
  setup.
- Added diagnostics proving the original GObj/display-link contract:
  `TITLE_LOGO_FIRE=0x544c4643,0x3f,1,4,3,1,0`.
- Reduced the Title deferred mask from `0x3f` to `0x1f`; at this point the
  remaining deferred branches were fire background, animated logo, labels/Press
  Start, slash, and logo-fire particles. The fire object branch is superseded by
  the later bounded `mnTitleMakeFire` import in this log.
- Extended `scripts/verify-runtime.ps1` and
  `scripts/verify-opening-skip.ps1` to assert the logo-fire boundary on both
  natural and skip-to-Title paths.
- Kept all changes in project-owned wrapper, backend, verifier, and docs files;
  the entire `decomp/` tree remains read-only reference material.

Still parked:

- Full Title fire background, animated logo, labels/Press Start, slash,
  logo-fire particles, audio, input transitions, and continuous draw remain
  deferred.
- Fighter/stage-heavy action-scene internals remain bridged, not imported.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.33).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 18 -> Title).`
- `scripts/verify-opening-movie-speed.ps1` passed with
  `Runtime speed sample (81.7s): frames=3299 hostfps=40.37 romfps=60 up=0 dl=60 cv=0 ch=32 present=401 room=1320 rdraw=2/24 portraits=150 mario=60 action=9/324 title=0x54494457`.

## 2026-06-20: Bounded original Title fire object and frame-update contract

Reused:

- Kept original BattleShip `mntitle.c` as the Title source of truth.
- Called original `mnTitleMakeFire` instead of adding a DS-native fire object or
  hand-authored animation.
- Used read-only BattleShip reference data outside the game-code subtree:
  `decomp/BattleShip-main/include/reloc_data.us.h` provided the 30
  `MNTitleFireAnimFrame*Sprite` offsets, and BattleShip docs confirmed the
  file-168 frames are `32x32` RGBA32 sprites.

Adapted for DS:

- Added a narrow `MNTitleFireAnim` symbol table in `src/port/scene_backend.c`
  for file `168`, mapping all 30 fire-frame symbols to their original offsets.
- Added a file-168 Sprite/Bitmap normalizer for those known frames:
  `32x32`, `G_IM_FMT_RGBA`, `G_IM_SIZ_32b`, one bitmap, and corrected
  `actualHeight=32`.
- Wired that normalizer into `lbRelocLoadFilesExtern` for the taskman-loaded
  `sMNTitleFiles[1]` path, so original `mnTitleMakeFire` and
  `mnTitleFireProcUpdate` can resolve real frame Sprites.
- Added diagnostics:
  `TITLE_FIRE_SPRITE_NORM=30,0` and
  `TITLE_FIRE=0x54464952,0xfff,1,2,1,2,786432,0` on the natural movie path.
- Reduced the Title deferred mask from `0x1f` to `0x1e`; `mnTitleMakeFire` is
  no longer deferred, while logo, labels/Press Start, slash, and logo-fire
  particles remain parked.
- Updated `scripts/verify-runtime.ps1` and `scripts/verify-opening-skip.ps1`.
  The skip path expects the non-opening shown-fire state
  `TITLE_FIRE=0x54464952,0xfff,1,2,0,2,786432,255` and symbol count `43`.

Still parked:

- Full fire background presentation, animated logo, labels/Press Start, slash,
  logo-fire particles, audio, title input transitions, and continuous title draw
  remain deferred.
- The action-scene bridge remains bounded scene-order proof, not imported
  fighter/stage-heavy movie rendering.

Verification:

- `make -j4` passed.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.33).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 10 -> Title).`
- `scripts/verify-opening-movie-speed.ps1` passed with
  `Runtime speed sample (81.3s): frames=3289 hostfps=40.47 romfps=60 up=0 dl=60 cv=0 ch=32 present=401 room=1320 rdraw=2/24 portraits=150 mario=60 action=9/324 title=0x54494457`.

## 2026-06-20: Maintenance split for status, backend seams, and verifiers

What changed:

- Added `docs/STATUS.md` as the short current-truth planning document. Keep
  this and `docs/HANDOFF.md` current; keep this `PORTING.md` append-only as
  history.
- Mechanically split the DS-owned backend monolith. `src/port/scene_backend.c`
  is now a thin include orchestrator over `diagnostics.c`, `taskman_seam.c`,
  `reloc_backend.c`, `sprite_preview_backend.c`, `opening_movie_backend.c`, and
  `title_backend.c`.
- Preserved the old single-translation-unit static linkage deliberately. The
  new slices are not listed in `Makefile` `CFILES`; converting them to
  independently compiled files is a later ABI-header cleanup.
- Added shared verifier helper libraries:
  `scripts/lib/melonds.ps1` and `scripts/lib/gdb-markers.ps1`.
- Added focused verifier entry points:
  `scripts/verify-opening-boundary.ps1`,
  `scripts/verify-title-boundary.ps1`, and `scripts/verify-all.ps1`.
- Kept no$gba as an interactive hardware/VRAM/OAM/register debugger and smoke
  capture target only. melonDS remains the automated runtime/global verifier.

Verification:

- `make -j4` passed after the split.
- `scripts/verify-opening-boundary.ps1` passed with
  `Runtime speed sample (9.5s): frames=513 hostfps=54.22 romfps=60 up=58 dl=0 cv=0 ch=1 present=19 room=429 rdraw=0/0 portraits=0 mario=0 action=0/0 title=0`.
- `scripts/verify-runtime.ps1` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.33).`
- `scripts/verify-opening-skip.ps1` passed with
  `Opening Room skip verification passed (tick 17 -> Title)` in the standalone
  run, then tick `10 -> Title` in the all-up run.
- `scripts/verify-title-boundary.ps1` passed with
  `Runtime speed sample (81.4s): frames=3294 hostfps=40.45 romfps=60 up=0 dl=60 cv=0 ch=32 present=401 room=1320 rdraw=2/24 portraits=150 mario=60 action=9/324 title=0x54494457`.
- `scripts/verify-all.ps1` passed with runtime, skip, and Title boundary gates
  chained in child PowerShell processes.

## 2026-06-20: Guarded dev/test scene harness for direct Title boundary

What changed:

- Added a build-time NDS dev/test scene harness in project-owned code:
  `include/nds/nds_scene_harness.h` and `src/port/scene_harness.c`.
- Wrapped imported `scManagerRunLoop` in `src/import/battleship_scmanager.c`.
  The wrapper mutates only `dSCManagerDefaultSceneData` before the original
  scene manager copies defaults into live state, preserving original dispatch.
- Added `NDS_DEV_SCENE_HARNESS` Makefile modes:
  `normal`, `title`, `vs_setup`, and `battle_fd`.
- Kept `normal` as the default natural startup/opening/movie path.
- Added `scripts/verify-title-harness.ps1`, which builds
  `TARGET=smash64ds-title BUILD=build-title-harness
  NDS_DEV_SCENE_HARNESS=title`, launches melonDS, and proves direct Title entry
  without Opening Room or opening movie replay.

Boundary details:

- `title` enters `nSCKindTitle` with
  `scene_prev = nSCKindOpeningNewcomers`, then runs the same bounded imported
  `mnTitleStartScene` path as the natural opening movie handoff.
- `vs_setup` is wired to `nSCKindVSMode`, but it currently reaches the existing
  scene-boundary stub only.
- `battle_fd` is reserved for the future one-fighter/Final-Destination test
  slot. It records a reserved marker and falls back to Title; it does not
  import fighters, stages, battle gameplay, audio, or renderer systems.

Verification:

- `make -j4` passed.
- `scripts/verify-title-harness.ps1` passed with
  `Title harness passed: scene=1/46 room=0 title=0x54494457`.
- `scripts/verify-all.ps1 -Build` passed with
  `Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.33)`,
  `Opening Room skip verification passed (tick 19 -> Title)`,
  `Runtime speed sample (81.4s): frames=3297 hostfps=40.50 romfps=60 up=0 dl=60 cv=0 ch=32 present=401 room=1320 rdraw=2/24 portraits=150 mario=60 action=9/324 title=0x54494457`,
  and `Full verification passed`.
- `scripts/verify-opening-boundary.ps1` passed with
  `Runtime speed sample (9.3s): frames=504 hostfps=54.30 romfps=60 up=58 dl=0 cv=0 ch=1 present=19 room=420 rdraw=0/0 portraits=0 mario=0 action=0/0 title=0`.

## 2026-06-20: Bounded original VS Mode setup proof

What changed:

- Added `src/import/battleship_mnvsmode.c`, which imports the original
  BattleShip `mnvsmode.c` translation unit and exposes a bounded DS wrapper for
  `mnVSModeStartScene` / `mnVSModeFuncStart`.
- Replaced the old `mnVSModeStartScene` scene-boundary stub. The
  `NDS_DEV_SCENE_HARNESS=vs_setup` build now enters original VS Mode setup
  from `nSCKindTitle`.
- Added the original `MNCommon` and `MNVSMode` O2R resources to the NitroFS
  manifest and relocation backend. File ID `0` is now valid for `MNCommon`, so
  the relocation backend uses `0xffffffff` as its invalid asset sentinel.
- Added narrow compatibility declarations for VS menu enums/macros, menu audio
  IDs, controller helpers, backup flag constants, and the one missing fighter
  costume helper.
- Added maintained diagnostics for VS setup start, file load, setup mask,
  object/camera/SObj counts, button/value proof, initial rule/time/stock state,
  and explicitly deferred VS branches.
- Added `scripts/verify-vs-setup-harness.ps1` and included it in
  `scripts/verify-all.ps1`.

Boundary details:

- The bounded VS setup path loads original `MNCommon` and `MNVSMode`, creates
  the original main GObj (`id 0`), default camera, viewports, background,
  menu-name, VS Start, Rule, Time/Stock, VS Options, value, arrow, and subtitle
  SObj graph.
- The taskman seam parks at scene kind `9` before running `mnVSModeMain`
  controller/input transitions, `PlayersVS` / `VSOptions` scene changes,
  audio, or continuous `gcDrawAll` menu rendering.
- `battle_fd` remains reserved only. No fighters, stages, battle gameplay,
  audio backend, or full renderer work was imported for this step.

Verification:

- `make -j4` passed.
- `scripts/verify-vs-setup-harness.ps1` passed with
  `VS setup harness passed: scene=9/1 setup=0x1f files=2 sobj=28 buttons=0x3f`.

## 2026-06-21: Bounded original VS Start to PlayersVS transition proof

What changed:

- Added `NDS_DEV_SCENE_HARNESS=vs_start_transition`, a guarded dev/test harness
  mode that starts at the already proven original `mnvsmode.c` setup boundary
  from `nSCKindTitle`.
- Kept `NDS_DEV_SCENE_HARNESS=vs_setup` parked before `mnVSModeMain`; the new
  transition proof is a separate harness mode.
- Added a bounded project-owned probe in `src/import/battleship_mnvsmode.c`
  that advances original `mnVSModeMain`, injects a synthetic A tap on VS Start
  through the original controller globals, then runs one follow-up original tick
  to observe the load-scene request.
- Added diagnostics proving original `mnVSModeMain` changed scene state to
  `nSCKindPlayersVS` from `nSCKindVSMode`, original
  `mnVSModeSaveSettings` stored the transfer battle settings, and original
  `syTaskmanSetLoadScene` was requested.
- Added `scripts/verify-vs-start-transition-harness.ps1` and included it in
  `scripts/verify-all.ps1`.

Boundary details:

- The proof remains bounded to VS setup plus the VS Start branch. It does not
  import `mnplayersvs.c`, maps, battle, fighters, stages, items, audio, full VS
  menu navigation, or continuous menu rendering.
- The final scene state is `scene_curr/scene_prev = 16/9`
  (`PlayersVS` from `VSMode`), and the existing PlayersVS scene-boundary stub is
  the stopping point.
- The expected transition marker is `VSTR` (`0x56535452`) with mask `0xFF`,
  input `0x8000`, saved settings `1/3/2`, and one bounded cleanup.

Verification:

- `scripts/verify-vs-setup-harness.ps1` passed with
  `VS setup harness passed: scene=9/1 setup=0x1f files=2 sobj=28 buttons=0x3f`.
- `scripts/verify-vs-start-transition-harness.ps1` passed with
  `VS Start transition harness passed: scene=16/9 trans=0x56535452 mask=0xff saved=1/3/2`.

## 2026-06-21: Bounded PlayersVS + Maps menu-chain proof to VSBattle

What changed:

- Added `NDS_DEV_SCENE_HARNESS=players_setup`, `maps_setup`, and
  `menu_chain_vsbattle`.
- Added `src/import/battleship_mnplayersvs.c`, importing original
  `mnplayersvs.c` through a DS-owned bounded wrapper.
- Added `src/import/battleship_mnmaps.c`, importing original `mnmaps.c`
  through a DS-owned bounded wrapper.
- Added PlayersVS and Maps O2R files to the NitroFS relocation manifest and
  extended the relocation asset table for the required menu/stage resources.
- Added narrow compatibility ABI for the imported menu slices, including menu
  reloc symbols, minimal fighter/audio/interface shims, and stage-select
  constants. `decomp/` remained read-only.
- Replaced the old generic PlayersVS and Maps scene stubs with bounded imported
  scene wrappers. `scVSBattleStartScene` remains the final boundary stub.
- Added verifier gates:
  `scripts/verify-players-vs-setup-harness.ps1`,
  `scripts/verify-maps-setup-harness.ps1`, and
  `scripts/verify-menu-chain-vsbattle-harness.ps1`.
- Added those gates to `scripts/verify-all.ps1` after the standalone harnesses
  passed.

Boundary details:

- PlayersVS setup loads seven original menu files, creates the original main
  GObj/default camera/camera set/menu object graph, initializes original
  PlayersVS vars and slot state, allocates bounded figatree heaps, records
  setup diagnostics, and parks before full interactive character-select
  input/drawing.
- The PlayersVS ready/start probe seeds a deterministic two-player selected
  state, injects original Start input, advances original `mnPlayersVSFuncRun`,
  observes PlayersVS -> Maps, and records the original load-scene request.
- Maps setup loads five original files, creates the original stage-select SObj
  graph, starts from seeded Pupupu/Dream Land, records setup diagnostics, and
  explicitly defers the stage preview model path.
- The Maps A-select probe injects original A input, advances original
  `mnMapsFuncRun`, observes original selected-stage saving, reaches
  `scene_prev = nSCKindMaps` and `scene_curr = nSCKindVSBattle`, and parks at
  the existing VSBattle boundary stub.
- No battle, fighter, stage, item, audio, full renderer, or gameplay subsystem
  was imported for this step.

Verification:

- `scripts/verify-players-vs-setup-harness.ps1` passed with
  `PlayersVS setup harness passed: files=7 mask=0xff sobj=65 slots=2/4/4`.
- `scripts/verify-maps-setup-harness.ps1` passed with
  `Maps setup harness passed: files=5 mask=0xff sobj=36 slot=6 gkind=6`.
- `scripts/verify-menu-chain-vsbattle-harness.ps1` passed with
  `Menu-chain VSBattle harness passed: VS->PV mask=0xff, PV->Maps mask=0xff, Maps->VSBattle mask=0xff, final=22/21`.

## 2026-06-21: Bounded original VSBattle setup proof

What changed:

- Activated `NDS_DEV_SCENE_HARNESS=battle_fd` as a direct bounded VSBattle setup
  harness instead of a reserved Title fallback.
- Added `src/import/battleship_scvsbattle.c`, importing original
  `sccommon/scvsbattle.c` and `sccommon/scvsbattlefiles.c` through a DS-owned
  bounded wrapper.
- Added `src/import/battleship_gmcommon.c` for original common battle file ID
  tables.
- Added the eight common battle interface files to NitroFS relocation staging
  and mapped their file IDs through the DS relocation backend.
- Added narrow compatibility ABI and stubs for the VSBattle setup boundary:
  battle cameras, interface/HUD setup, fighter-file setup, stub fighter GObj
  creation, ground spawn lookup, rumble, audio/BGM, item/weapon manager, and
  effect/collision setup diagnostics.
- Updated the VS Mode -> PlayersVS -> Maps -> VSBattle chain so the final
  boundary now reaches the same imported bounded VSBattle setup proof instead
  of parking at the old generic scene stub.
- Added `scripts/verify-battle-fd-harness.ps1` and included it in
  `scripts/verify-all.ps1`.

Boundary details:

- Direct `battle_fd` seeds one Mario stock battle from `nSCKindMaps` into
  `nSCKindVSBattle`, with the current Final Destination sentinel
  `nGRKindLast`.
- The imported setup loads the original/common battle file list, creates the
  default battle camera path through compatibility stubs, reaches original
  battle manager setup calls, builds active fighter descriptors from
  `SCBattleState`, creates stub fighter GObjs for active players, reaches
  interface/HUD setup calls, proves one bounded `scVSBattleFuncUpdate`
  interface tick, and parks before real gameplay/update/draw.
- Full fighter logic, full stage logic, item/weapon runtime, full interface
  rendering, post-battle results, and audio playback remain deferred.
- `decomp/` remained read-only; all hooks live in project-owned imports,
  headers, scripts, or DS/port backend code.

Verification:

- `scripts/verify-battle-fd-harness.ps1` passed with
  `Battle FD harness passed: files=8 players=1/0 fighters=1 gkind=16 mask=0x7f`.

## 2026-06-21: Bounded Mario/Fox fighter model GObj proof

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_mariofox_model` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_model`.
- Staged `FTManagerCommon`, Mario, Fox, and the required external fighter O2R
  dependencies through NitroFS and the DS relocation table.
- Extended the bounded fighter compatibility path so Mario/Fox descriptors from
  imported `scvsbattle.c` can create asset-backed fighter GObjs with real
  top/model/commonpart DObj trees.
- Kept fighter process/status/gameplay execution explicitly deferred and kept
  `ftDisplayMainProcDisplay` as a bounded no-op until the full fighter display
  path is selected as its own renderer milestone.
- Added maintained diagnostics for Mario/Fox relocation, model setup masks,
  DObj/MObj/AObj counts, display attachment, and the last model materialization
  pointers.
- Fixed a scene-chain relocation bug by clearing fighter-specific file slots
  whenever the scene-local relocation cache is reset. Without that, the
  menu-chain model proof could reuse stale arena pointers after Maps ->
  VSBattle and observe an invalid `attr->commonparts_container` pointer.
- Added `scripts/verify-battle-mariofox-model-harness.ps1` and
  `scripts/verify-menu-chain-mariofox-model-harness.ps1`, and included both in
  `scripts/verify-all.ps1`.

Boundary details:

- `battle_mariofox_model` starts directly at imported bounded VSBattle on
  Pupupu/Dream Land, loads and relocates the Mario/Fox fighter files and shared
  dependencies, creates two real asset-backed fighter model GObjs, records
  Mario/Fox DObj counts, and parks before real fighter update/draw/gameplay.
- `menu_chain_mariofox_model` proves the same model boundary after original VS
  Mode -> PlayersVS -> Maps -> VSBattle transitions.
- Setup-only VSBattle and Pupupu harnesses still use stub fighter GObjs; this
  model proof is isolated to the two new harness modes.
- Full fighter status logic, animation/motion playback, hitboxes, hurtboxes,
  collision interaction, items/weapons, audio, HUD rendering, and full fighter
  display rendering remain deferred.
- `decomp/` remained read-only; all changes are in project-owned harness,
  relocation, compatibility, verifier, and documentation files.

Verification:

- `scripts/verify-battle-mariofox-model-harness.ps1` passed with
  `Battle Mario/Fox model harness passed: assets=0xffff, setup=0xfff, realGObjs=2, p0DObjs=25, p1DObjs=27`.
- `scripts/verify-menu-chain-mariofox-model-harness.ps1` passed with
  `Menu-chain Mario/Fox model harness passed: chain masks=0xff/0xff/0xff, assets=0xffff, setup=0xfff, realGObjs=2`.

## 2026-06-21: Guarded bounded original Pupupu / Dream Land update probe

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_pupupu_update` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_pupupu_update`.
- Added a bounded project-owned probe in the Pupupu import wrapper that calls
  original `grPupupuProcUpdate` twice after seeding a deterministic safe
  substate.
- Recorded update diagnostics for Whispy state, wind wait, blink wait, flower
  state, map GObj mask, GObj count, and guarded side-effect counters.
- Added side-effect counters for Whispy wind FGM, fighter push, quake, and
  particle script calls so the safe update path can prove those branches did
  not run.
- Added direct and menu-chain update verifiers and included them in
  `scripts/verify-all.ps1`.
- Upgraded the setup-only Pupupu stage and menu-chain VSBattle verifiers to
  assert that the update probe remains untouched in setup-only builds.

Boundary details:

- The direct harness starts at imported bounded VSBattle with two seeded human
  players on Pupupu/Dream Land, completes the existing original ground object
  setup proof, then runs two guarded original `grPupupuProcUpdate` ticks.
- The menu-chain harness proves the same update after VS Mode -> PlayersVS ->
  Maps -> VSBattle.
- The proven update path is deliberately narrow: Sleep -> Wait and one Wait
  countdown tick. It preserves the four original map GObjs, keeps flowers in
  default state, leaves GObj count unchanged, and records zero wind FGM,
  fighter push, quake, and particle script side effects.
- Continuous stage update/draw, Whispy wind/fighter push behavior, full
  collision line processing, yakumono runtime, item/effect runtime, fighter
  gameplay, HUD rendering, audio backend, and real gameplay remain deferred.
- `decomp/` remained read-only; all changes are in project-owned imports,
  headers, backend stubs, verifiers, and docs.

Verification:

- `scripts/verify-battle-pupupu-update-harness.ps1` passed with
  `Battle Pupupu update harness passed: scene=22/21 update=0xff ticks=2 whispy=0->1 windWait=2->1 sidefx=0/0/0/0`.
- `scripts/verify-menu-chain-pupupu-update-harness.ps1` passed with
  `Menu-chain Pupupu update harness passed: VS->PV mask=0xff, PV->Maps mask=0xff, Maps->VSBattle mask=0xff, ground=0x3ff, update=0xff, final=22/21`.
- `make clean`, `make -j4`, and `scripts/verify-all.ps1` passed. The full
  verifier reported speed sample `frames=3285 hostfps=40.42` and included both
  direct and menu-chain Pupupu update gates.
- `scripts/verify-menu-chain-vsbattle-harness.ps1` passed with
  `Menu-chain VSBattle harness passed: VS->PV mask=0xff, PV->Maps mask=0xff, Maps->VSBattle mask=0xff, VSBattle files=8 fighters=2, final=22/21`.

## 2026-06-21: Pupupu / Dream Land stage-data proof

What changed:

- Added Pupupu/Dream Land stage O2R staging for `GRPupupuMap`,
  `StageDreamLand`, `ExternDataBank103`, `ExternDataBank104`, and
  `MiscDataBank152`.
- Updated the DS relocation asset table to distinguish DS-side asset lookup
  keys from original O2R file IDs, because `StageDreamLand` and an Opening
  Room movie asset both use original file ID `0x58`.
- Added bounded external relocation support for the Pupupu dependency chain:
  external file ID list parsing, recursive dependency loading, external pointer
  fixups, and failure-count diagnostics.
- Un-deferred the Pupupu path in bounded imported `mnmaps.c` setup. The direct
  Maps harness now runs the Pupupu preview path, resolves real stage pointers,
  records preview object diagnostics, and clears the preview-deferred marker.
- Added `NDS_DEV_SCENE_HARNESS=battle_pupupu_stage`, seeding two human players
  on Pupupu/Dream Land for direct VSBattle setup testing.
- Updated `mpCollisionInitGroundData` to adopt real Pupupu `MPGroundData`,
  record geometry/map-node/light/BGM metadata, and keep full collision and
  yakumono/stage runtime explicitly deferred.
- Added `scripts/verify-battle-pupupu-stage-harness.ps1`, upgraded the Maps
  and menu-chain verifiers to assert Pupupu preview/stage markers, and included
  the new verifier in `scripts/verify-all.ps1`.

Boundary details:

- `maps_setup` still loads the five original Maps menu files, but Pupupu no
  longer parks at a preview-deferred marker. It now proves real Pupupu stage
  assets and fixups with `PUPR`, then records preview marker `PUPV`.
- `battle_pupupu_stage` starts directly at imported bounded VSBattle setup,
  proves two active original player descriptors, creates two stub fighter GObjs
  for Mario/Fox, adopts real Pupupu `MPGroundData`, records stage marker
  `PUPB`, and parks before gameplay.
- `menu_chain_vsbattle` now proves the selected Pupupu stage carries from
  original Maps A-select into imported VSBattle setup.

- Full collision line processing, yakumono/stage object runtime, real fighter
  logic/model/motion, items/weapons, HUD rendering, audio backend, and gameplay
  remain deferred.

Verification:

- `scripts/verify-maps-setup-harness.ps1` passed with
  `Maps setup harness passed: scene=21/16 setup=0xff files=5 preview=0xff assets=0x1f slot=6 gkind=6`.
- `scripts/verify-battle-fd-harness.ps1` passed with
  `Battle FD harness passed: files=8 players=1/0 fighters=1 gkind=16 mask=0x7f`.
- `scripts/verify-battle-pupupu-stage-harness.ps1` passed with
  `Battle Pupupu stage harness passed: scene=22/21 gkind=6 stage=0xff assets=0x1f players=2 fighters=2`.
- `scripts/verify-menu-chain-vsbattle-harness.ps1` passed with
  `Menu-chain VSBattle harness passed: VS->PV mask=0xff, PV->Maps mask=0xff, Maps preview=0xff, Maps->VSBattle mask=0xff, VSBattle stage=0xff, final=22/21`.

## 2026-06-21: Bounded original Pupupu / Dream Land ground object setup

What changed:

- Added `src/import/battleship_grpupupu_ground.c`, importing original
  `grdisplay.c`, `grmainsetup.c`, `grcommonsetup.c`, and
  `grcommon/grpupupu.c` through a bounded project-owned wrapper.
- Added a narrow project-owned `include/mp/map.h` shadow header so imported
  ground setup can use the existing local `MPGroundData` surface without
  pulling in BattleShip's full map/fighter/item header graph.
- Added the minimal GBI no-op compatibility shims needed by imported ground
  display setup (`gSPClearGeometryMode`, `G_RM_AA_OPA_SURF`,
  `G_RM_AA_OPA_SURF2`).
- Moved Pupupu ground setup from the generic compatibility stub to a
  Pupupu-only original path. Non-Pupupu stages still use the prior
  compatibility helper.
- Added narrow Pupupu ground ABI, particle/effect/item/fighter/collision
  stubs, and diagnostics for display-layer GObjs, map GObjs, map-head offset,
  DObj/MObj/animation pointer proof, object counts, deferred runtime work, and
  non-Pupupu stub calls.
- Upgraded `verify-battle-pupupu-stage-harness.ps1` and
  `verify-menu-chain-vsbattle-harness.ps1` to assert the new `PUGS`, `PUGD`,
  `PUGO`, and `0x3FF` ground setup mask markers.

Boundary details:

- `battle_pupupu_stage` now proves that bounded VSBattle adopts real
  `GRPupupuMap` `MPGroundData`, enters imported original
  `grCommonSetupInitAll`, creates four original display-layer GObjs, dispatches
  through original `grMainSetupMakeGround` to `grPupupuMakeGround`, resolves
  original Pupupu map head `0x10F0`, and creates the original Whispy eyes,
  Whispy mouth, back flowers, and front flowers GObjs.
- `menu_chain_vsbattle` proves the same Dream Land ground object graph after
  VS Mode -> PlayersVS -> Maps -> VSBattle.
- Continuous stage update/draw, Whispy wind/fighter push, real particle banks,
  yakumono runtime, item/effect appear actors, full collision, full fighter
  logic/model/motion, HUD rendering, audio, and gameplay remain deferred.
- `decomp/` remained read-only; all changes are in project-owned imports,
  headers, backend stubs, verifiers, and docs.

Verification:

- `scripts/verify-battle-pupupu-stage-harness.ps1` passed with
  `Battle Pupupu stage harness passed: scene=22/21 gkind=6 stage=0xff ground=0x3ff layers=4 mapGObjs=4 players=2 fighters=2`.
- `scripts/verify-menu-chain-vsbattle-harness.ps1` passed with
  `Menu-chain VSBattle harness passed: VS->PV mask=0xff, PV->Maps mask=0xff, Maps preview=0xff, Maps->VSBattle mask=0xff, stage=0xff, ground=0x3ff, final=22/21`.
- `scripts/verify-battle-fd-harness.ps1` passed with
  `Battle FD harness passed: files=8 players=1/0 fighters=1 gkind=16 mask=0x7f`.

## 2026-06-21: Persistent Mario/Fox FTStruct-backed fighter state shell

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_mariofox_struct` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_struct`.
- Kept `battle_mariofox_model` and `menu_chain_mariofox_model` behavior
  unchanged, but made the Mario/Fox model proof predicate true for both model
  and struct harness families.
- Added a bounded project-owned `FTStruct` shell with descriptor identity,
  attributes, figatree pointer, input masks, collision pointer contracts,
  status/deferred counters, and a joint table sized for the original fighter
  parts boundary.
- Added a small static `FTStruct` pool for the active player slots. In struct
  harnesses, Mario and Fox fighter GObjs store the pool pointer in
  `fighter_gobj->user_data.p`, and `ftGetStruct` returns the persistent pool
  object instead of the old fallback shell.
- Seeded Mario/Fox `FTStruct` state from the original `FTDesc` values built by
  imported `scvsbattle.c`, preserving the original descriptor path rather than
  hand-authored gameplay state.
- Populated the top joint and deterministic common joint table from the real
  asset-backed DObj tree. Exact original
  `lbCommonSetupFighterPartsDObjs` joint-ID mapping is still deferred.
- Kept original fighter status/process/physics/input/hit/catch/shadow/gameplay
  and full fighter display execution parked.
- Added maintained diagnostics for struct result, joint result, state result,
  struct mask, pool mask/count, per-player identity/input/joint/collision
  fields, and zero status/process/display execution counters.
- Added `scripts/verify-battle-mariofox-struct-harness.ps1` and
  `scripts/verify-menu-chain-mariofox-struct-harness.ps1`, and included both in
  `scripts/verify-all.ps1`.

Boundary details:

- `battle_mariofox_struct` starts directly at imported bounded VSBattle on
  Pupupu/Dream Land, reuses the asset-backed Mario/Fox model creation path,
  attaches persistent `FTStruct` shells to both fighter GObjs, verifies
  `ftGetStruct` identity, records Mario/Fox state and joint diagnostics, and
  parks before real fighter runtime.
- `menu_chain_mariofox_struct` proves the same persistent struct state after the
  guarded original VS Mode -> PlayersVS -> Maps -> VSBattle chain.
- The struct mask is expected to reach `0xFFF`, pool mask `0x3`, struct count
  `2`, and zero process/status/display probe counters.

Verification:

- `scripts/verify-battle-mariofox-struct-harness.ps1` passed with
  `Battle Mario/Fox struct harness passed: scene=22/21 struct=0xfff pool=0x3 p0Joints=24 p1Joints=26 model=0xfff`.
- `scripts/verify-menu-chain-mariofox-struct-harness.ps1` passed with
  `Menu-chain Mario/Fox struct harness passed: chain masks=0xff/0xff/0xff, model=0xfff, struct=0xfff, final=22/21`.

## 2026-06-21: Bounded Mario/Fox fighter init-state proof

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_mariofox_init` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_init`.
- Kept the direct and menu-chain Mario/Fox model and struct proofs intact, then
  made the model predicate cover modes `11` through `16`, the struct predicate
  cover modes `13` through `16`, and added a new init predicate for modes `15`
  and `16`.
- Added a bounded project-owned helper that mirrors the safe source order of
  original `ftManagerInitFighter` without running original fighter
  status/process/input/physics/display/gameplay. The helper initializes
  damage, shield, velocities, timers, root DObj position/scale, attribute map
  collision and cliff-catch contracts, deterministic Pupupu floor projection,
  ground/air state, jump counters, attack/damage counters, hitstatus/damage
  kind, throw/catch/item pointer clearing, Mario/Fox passive vars, and the
  guarded physics/attack-collision/hitstatus/colanim compatibility calls.
- Expanded the narrow `FTStruct`, `FTCollisionData`, and `FTAttributes` shadow
  ABI only enough to hold the new bounded init-state contract.
- Added a deterministic `mpCollisionCheckProjectFloor` seam for the current
  Pupupu spawn-state proof.
- Added maintained diagnostics for init result, collision result, deferred
  runtime result, init mask, deferred mask, per-player damage/shield/GA/jump/
  floor/root/passive state, bounded call counters, and zero process/status/
  display counters.
- Added `scripts/verify-battle-mariofox-init-harness.ps1` and
  `scripts/verify-menu-chain-mariofox-init-harness.ps1`, and included both in
  `scripts/verify-all.ps1`.

Boundary details:

- `battle_mariofox_init` starts directly at imported bounded VSBattle on
  Pupupu/Dream Land, reuses the asset-backed Mario/Fox model creation and
  persistent `FTStruct` shell paths, runs the bounded source-order init helper,
  verifies ground-state/floor-projection and compatibility-call diagnostics,
  and parks before real fighter runtime.
- `menu_chain_mariofox_init` proves the same initialized fighter state after
  the guarded original VS Mode -> PlayersVS -> Maps -> VSBattle chain.
- The init mask is expected to reach `0x3FFF`, deferred mask `0xFF`, init
  count `2`, both fighters on ground with floor projection `1/1`, four bounded
  call counters at `2`, and status/process/display counters at `0`.
- Original fighter status transitions, process callbacks, input/update loops,
  physics/gameplay, hit/catch/search runtime, shadows, full display traversal,
  and gameplay remain deferred.

Verification:

- `scripts/verify-battle-mariofox-init-harness.ps1` passed with
  `Battle Mario/Fox init harness passed: scene=22/21 init=0x3fff p0GA=0 p1GA=0 floor=1/1 calls=2/2/2/2`.
- `scripts/verify-menu-chain-mariofox-init-harness.ps1` passed with
  `Menu-chain Mario/Fox init harness passed: chain masks=0xff/0xff/0xff, model=0xfff, struct=0xfff, init=0x3fff, final=22/21`.

## 2026-06-21: Bounded original Mario/Fox Wait status and motion setup proof

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_mariofox_wait` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait`.
- Imported original BattleShip `ftcommonwait.c` only through
  `src/import/battleship_ftcommon_wait.c`.
- Extended the Mario/Fox model, struct, and init predicates to cover harness
  modes `17` and `18`, then added a Wait-only predicate for modes `17` and
  `18`.
- Added a bounded project-owned `ftMainSetStatus` seam that accepts only
  `nFTCommonStatusWait` while the Wait proof is enabled. It installs status
  `10`, motion `4`, animation frame `0`, animation speed `1.0`, no attack
  IDs, Wait interrupt/physics/map callback pointers, and defers all callback
  execution.
- Added compatibility stubs for the original Wait path:
  `ftHammerCheckHoldHammer`, `ftHammerSetStatusHammerWait`,
  `mpCommonSetFighterGround`, `ftParamSetPlayerTagWait`,
  `ftPhysicsApplyGroundVelFriction`, `mpCommonProcFighterOnCliffEdge`, and
  `ftCommonGroundCheckInterrupt`.
- Added diagnostics for Wait status/motion/defer results, Wait mask,
  per-player status/motion/animation/callback/tag fields, and call counters
  proving original Wait entry ran while process/display/gameplay callbacks did
  not.
- Added `scripts/verify-battle-mariofox-wait-harness.ps1` and
  `scripts/verify-menu-chain-mariofox-wait-harness.ps1`, and included both in
  `scripts/verify-all.ps1`.

Boundary details:

- `battle_mariofox_wait` starts directly at imported bounded VSBattle on
  Pupupu/Dream Land, reuses the asset-backed Mario/Fox model creation,
  persistent `FTStruct` shell path, and bounded init-state proof, then runs
  original `ftCommonWaitSetStatus` for both fighters.
- `menu_chain_mariofox_wait` proves the same Wait status/motion setup after the
  guarded original VS Mode -> PlayersVS -> Maps -> VSBattle chain.
- The Wait mask is expected to reach `0xFFF`, deferred mask `0xFF`, Wait count
  `2`, status `10/10`, motion `4/4`, player tag wait `120/120`, original Wait
  call count `2`, bounded `ftMainSetStatus` count `2`, and callback/process/
  display/gameplay execution counts `0`.
- Full fighter process callbacks, input/update loops, physics execution,
  hit/catch/search runtime, shadows, full fighter display traversal, and
  gameplay remain deferred.

Verification:

- `scripts/verify-battle-mariofox-wait-harness.ps1` passed with
  `Battle Mario/Fox Wait harness passed: waitMask=0xfff count=2 status=10/10 motion=4/4 callbacks=0`.
- `scripts/verify-menu-chain-mariofox-wait-harness.ps1` passed with
  `Menu-chain Mario/Fox Wait harness passed: chain final=22/21 waitMask=0xfff count=2 status=10/10 motion=4/4 callbacks=0`.

## 2026-06-21: Bounded Mario/Fox Wait callback tick proof

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_tick` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_tick`.
- Extended the Mario/Fox model, struct, init, and Wait predicates to cover
  harness modes `19` and `20`, then added a tick-only predicate for those modes.
- Added a bounded Wait tick probe that requires the prior Wait setup proof,
  validates the installed original `ftCommonWaitProcInterrupt` callback and
  guarded physics/map callback pointers, seeds neutral input, and runs one
  original Wait interrupt callback tick per fighter.
- Kept `ftPhysicsApplyGroundVelFriction` and
  `mpCommonProcFighterOnCliffEdge` as project-owned guarded seams for this
  proof. They record callback counts but do not mutate velocity, map state, or
  gameplay state.
- Added diagnostics for Wait tick result, callback result, safety result, mask,
  deferred mask, per-fighter before/after status/motion/GA/root/ground-velocity
  state, callback counters, and safety counters.
- Added `scripts/verify-battle-mariofox-wait-tick-harness.ps1` and
  `scripts/verify-menu-chain-mariofox-wait-tick-harness.ps1`, and included both
  in `scripts/verify-all.ps1`.

Boundary details:

- `battle_mariofox_wait_tick` starts directly at imported bounded VSBattle on
  Pupupu/Dream Land, reuses the asset-backed Mario/Fox model creation,
  persistent `FTStruct` shell path, bounded init-state proof, and original Wait
  setup proof, then runs one bounded original Wait interrupt callback tick for
  Mario and Fox.
- `menu_chain_mariofox_wait_tick` proves the same Wait callback tick after the
  guarded original VS Mode -> PlayersVS -> Maps -> VSBattle chain.
- The tick mask is expected to reach `0x3FF`, deferred mask `0xFF`, tick count
  `2`, callback counts `2/2/2/2`, and safety counters `0`.
- Status remains `10/10`, motion remains `4/4`, ground/air state remains
  `0/0`, root positions remain stable, ground velocity X remains `0`, and GObj
  count remains unchanged.
- Real fighter update loops, unbounded physics/map mutation, hit/catch/search
  runtime, shadows, full fighter display traversal, and gameplay remain
  deferred.

Verification:

- `scripts/verify-battle-mariofox-wait-tick-harness.ps1` passed with
  `Battle Mario/Fox Wait tick harness passed: scene=22/21 tick=0x3ff callbacks=2/2/2 stable=1 final=22`.
- `scripts/verify-menu-chain-mariofox-wait-tick-harness.ps1` passed with
  `Menu-chain Mario/Fox Wait tick harness passed: chain final=22/21 wait=0xfff tick=0x3ff callbacks=2/2/2 stable=1`.
- The setup-only Wait verifiers still passed with callback execution counts at
  `0`, proving the tick proof is isolated to the new harness modes.

## 2026-06-22: Bounded Mario/Fox Wait ground-friction/map proof

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_ground` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_ground`.
- Reused the existing Mario/Fox model, persistent `FTStruct`, init, Wait, and
  Wait tick proofs, then added a second controlled ground pass only for the new
  ground harness modes.
- Added project-owned compatibility fields and constants needed by the bounded
  ground proof, including nested fighter physics velocity state, floor angle
  vector data, floor/map material flags, and common material friction data.
- Added a bounded source-order helper for the original
  `ftPhysicsSetGroundVelFriction` path plus the safe subset of
  `ftPhysicsSetGroundVelTransferAir`.
- Added a bounded safe-floor branch for `mpCommonProcFighterOnCliffEdge` that
  proves the floor path without allowing Fall/Ottotto status changes.
- Added diagnostics for ground result, map result, safety result, mask,
  deferred mask, per-fighter before/after velocity, air-transfer fields,
  material/traction/friction, status/motion/GA, root position, callback counts,
  and safety counters.
- Added `scripts/verify-battle-mariofox-wait-ground-harness.ps1` and
  `scripts/verify-menu-chain-mariofox-wait-ground-harness.ps1`, and included
  both in `scripts/verify-all.ps1`.

Boundary details:

- `battle_mariofox_wait_ground` starts directly at imported bounded VSBattle on
  Pupupu/Dream Land, reuses the Wait tick proof, seeds Mario/Fox ground
  velocity to `2.0`, runs the bounded ground-friction/air-transfer pass, then
  runs the safe floor-map seam.
- `menu_chain_mariofox_wait_ground` proves the same ground-friction/map pass
  after the guarded original VS Mode -> PlayersVS -> Maps -> VSBattle chain.
- The ground mask is expected to reach `0x7FF`, deferred mask `0xFF`, ground
  count `2`, physics/map/map-check/safe-floor counts `2`, and Fall/Ottotto,
  status/motion/GA/root/GObj/display/gameplay safety counters `0`.
- The current deterministic proof seeds velocity milli `2000` and the bounded
  source-order friction path clamps both fighters to `0`, while preserving
  Wait status `10`, motion `4`, ground/air state `0`, root position, and GObj
  count.
- Continuous fighter physics, full map/collision mutation, status/process
  loops, hit/catch/search runtime, shadows, full display traversal, and
  gameplay remain deferred.

Verification:

- `make clean` and `make -j4` passed.
- `scripts/verify-battle-mariofox-wait-ground-harness.ps1` passed with
  `Battle Mario/Fox Wait ground harness passed: scene=22/21 ground=0x7ff vel=2000->0/2000->0 map=2/2 stable=1`.
- `scripts/verify-menu-chain-mariofox-wait-ground-harness.ps1` passed with
  `Menu-chain Mario/Fox Wait ground harness passed: chain final=22/21 wait=0xfff tick=0x3ff ground=0x7ff map=2/2 stable=1`.
- `scripts/verify-all.ps1` passed with the new direct and menu-chain ground
  verifiers included in the maintained regression chain.

## 2026-06-22: Direct bounded Mario/Fox fighter display metadata probe

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_mariofox_display_probe` as direct-only
  harness mode `23`.
- Reused the direct Pupupu Mario/Fox proof chain through model, persistent
  `FTStruct`, init, original Wait setup, Wait callback tick, and bounded Wait
  ground-friction/map proof.
- Added a DS-owned display probe guard that calls the current project-owned
  `ftDisplayMainProcDisplay` seam exactly once for Mario and once for Fox.
- Added diagnostics for display result/safety masks, callback count,
  per-player DObj/MObj/AObj counts, display-list candidate counts, safe
  parts-pointer counts, status/motion/GA after-state, root X stability, GObj
  delta, and draw/matrix/gameplay escape counters.
- Added `scripts/verify-battle-mariofox-display-probe-harness.ps1` and wired it
  into `scripts/verify-all.ps1` after the standalone verifier passed.
- Updated `docs/STATUS.md`, `docs/HANDOFF.md`,
  `docs/DIAGNOSTIC_REFERENCE.md`, and `docs/KNOWN_ISSUES.md`.

Boundary details:

- The probe records metadata only. It does not render fighters, import
  `ftdisplaymain.c`, prepare matrices, emit GBI, run shadows/afterimages/fog,
  invoke magnify/interface rendering, start continuous draw traversal, or run
  gameplay.
- Current bounded Mario/Fox metadata is DObj `25/27`, MObj `0/0`, AObj `0/0`,
  display-list candidates `14/18`, status `10/10`, motion `4/4`, and GA
  `0/0`.
- There is intentionally no menu-chain display-probe harness in this step.

Verification:

- `make clean` and `make -j4` passed.
- `scripts/verify-battle-mariofox-display-probe-harness.ps1` passed with
  `Battle Mario/Fox display probe harness passed: scene=22/21 display=0x7ff dobj=25/27 mobj=0/0 ready=14/18 stable=1`.
- `scripts/verify-battle-mariofox-wait-ground-harness.ps1` passed with
  `Battle Mario/Fox Wait ground harness passed: scene=22/21 ground=0x7ff vel=2000->0/2000->0 map=2/2 stable=1`.
- `scripts/verify-menu-chain-mariofox-wait-ground-harness.ps1` passed with
  `Menu-chain Mario/Fox Wait ground harness passed: chain final=22/21 wait=0xfff tick=0x3ff ground=0x7ff map=2/2 stable=1`.
- `scripts/verify-all.ps1` passed with the new direct display metadata probe
  included in the maintained regression chain.

## 2026-06-22: Menu-chain Mario/Fox fighter display metadata probe

What changed:

- Added `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_display_probe` as harness
  mode `24`.
- Reused the existing guarded menu-chain transition path through original VS
  Mode -> PlayersVS -> Maps -> imported bounded VSBattle with Pupupu
  stage-data adoption.
- Reused the cumulative Mario/Fox proof chain through model, persistent
  `FTStruct`, source-order init, original Wait setup, Wait callback tick,
  bounded Wait ground-friction/map proof, and the guarded display metadata
  callback probe.
- Added `scripts/verify-menu-chain-mariofox-display-probe-harness.ps1` and
  wired it into `scripts/verify-all.ps1` after the standalone verifier passed.
- Updated current-truth, handoff, diagnostic, and known-issues docs.

Boundary details:

- The new harness proves the direct display metadata contract after the full
  bounded menu chain: final scene `22/21`, selected Pupupu stage `6`, display
  mask `0x7FF`, DObj counts `25/27`, display-list candidate counts `14/18`,
  and stable Wait/ground/display state.
- The probe remains metadata-only. It does not render fighters, import full
  fighter display traversal, run matrix prep, emit GBI, start gameplay, create
  new GObjs, or mutate root/status/motion/ground-air state.

Verification:

- `make clean` and `make -j4` passed.
- `scripts/verify-battle-mariofox-display-probe-harness.ps1` passed with
  `Battle Mario/Fox display probe harness passed: scene=22/21 display=0x7ff dobj=25/27 mobj=0/0 ready=14/18 stable=1`.
- `scripts/verify-menu-chain-mariofox-wait-ground-harness.ps1` passed with
  `Menu-chain Mario/Fox Wait ground harness passed: chain final=22/21 wait=0xfff tick=0x3ff ground=0x7ff map=2/2 stable=1`.
- `scripts/verify-menu-chain-mariofox-display-probe-harness.ps1` passed with
  `Menu-chain Mario/Fox display probe harness passed: chain final=22/21 display=0x7ff dobj=25/27 ready=14/18 stable=1`.
- `scripts/verify-all.ps1` passed with the new menu-chain display metadata
  probe included in the maintained regression chain.

## 2026-06-22: Direct and menu-chain Mario/Fox display-list scan proofs

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_scan` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_scan` as harness modes `25`
  and `26`.
- Reused the existing Mario/Fox proof chain through model, persistent
  `FTStruct`, source-order init, original Wait setup, Wait callback tick,
  ground-friction/map proof, and display metadata proof.
- Added a parser-only DL scan boundary in `src/port/reloc_backend.c` that
  selects the first display-list-bearing DObj for Mario and Fox, scans each
  selected list with `ndsRendererScanDisplayList()`, and records copied
  `NDSRendererStats` without rendering, matrix prep, `gcDrawAll`, continuous
  draw, or gameplay.
- Added taskman-arena range ownership for copied original fighter display-list
  data. Diagnostics use asset sentinel `0xfffffffe` to distinguish those
  copied DLs from pointers still inside registered loaded O2R files.
- Added `scripts/verify-battle-mariofox-dl-scan-harness.ps1` and
  `scripts/verify-menu-chain-mariofox-dl-scan-harness.ps1`, then wired both
  into `scripts/verify-all.ps1` after standalone verifier passes.
- Updated current-truth, handoff, diagnostic, known-issues, and roadmap docs.

Boundary details:

- The scan records first DL pointers, asset/offset ownership, DObj index,
  blocker, command counts, first/unsupported opcodes, vertex and triangle
  counts, branch/end/texture stats, range rejects, branch resolves, stable
  fighter state, and no draw/matrix/gameplay escape.
- The selected fighter DLs currently live in taskman-arena copied original
  data, not directly inside registered loaded-file records.
- The exact next renderer blocker for this selected fighter DL path is
  `NDS_RENDERER_BLOCKER_UNSUPPORTED` (`4`) on both Mario and Fox.

Verification:

- `scripts/verify-battle-mariofox-dl-scan-harness.ps1` passed with
  `Battle Mario/Fox DL scan harness passed: scene=22/21 dl=0x22dbe88/0x2304850 asset=4294967294/4294967294 commands=59/69 blocker=4/4 safe=1`.
- `scripts/verify-menu-chain-mariofox-dl-scan-harness.ps1` passed with
  `Menu-chain Mario/Fox DL scan harness passed: chain final=22/21 dl=0x22dc528/0x2304ef0 asset=4294967294/4294967294 commands=59/69 blocker=4/4 safe=1`.
- `scripts/verify-all.ps1` passed with both new DL-scan verifiers included in
  the maintained regression chain.

## 2026-06-22: Mario/Fox first display-list execute/decode proofs

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_execute` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_execute` as harness modes `27`
  and `28`.
- Reused the existing direct and menu-chain Mario/Fox proof chain through
  model, persistent `FTStruct`, source-order init, original Wait setup, Wait
  callback tick, ground-friction/map proof, display metadata, and first-DL
  scan.
- Extended the DS renderer adapter so known benign N64 state/culling/image/TLUT
  commands are recorded as state or skip evidence instead of fatal unsupported
  blockers: `G_SETOTHERMODE_H`, `G_SETOTHERMODE_L`, `G_RDPSETOTHERMODE`,
  `G_CULLDL`, `G_SETCIMG`, `G_LOADTLUT`, and `G_NOOP`.
- Kept unknown/default opcodes unsupported.
- Added scan-family diagnostics and strengthened both DL scan verifiers to
  require blocker `0/0`, unsupported opcode/counts `0`, and nonzero
  vertex/triangle/state/render command evidence.
- Added a decode-only execute probe that calls
  `ndsRendererExecuteDisplayList()` once for the selected Mario DL and once for
  the selected Fox DL, decodes real `G_VTX`, `G_TRI1`, and `G_TRI2` payloads,
  records bounds/color/command-family diagnostics, and proves no fighter state,
  root, GObj, draw, matrix, or gameplay escape.
- Added `scripts/verify-battle-mariofox-dl-execute-harness.ps1` and
  `scripts/verify-menu-chain-mariofox-dl-execute-harness.ps1`, then wired both
  into `scripts/verify-all.ps1` after standalone passes.

Boundary details:

- The selected Mario/Fox DLs still come from taskman-arena copied original
  fighter data (`0xfffffffe` ownership sentinel).
- Current scan and execute command counts remain `59/69`.
- Current execute decode evidence is `28/23` decoded vertices and `37/20`
  triangles.
- This is not visible fighter rendering. Matrix/camera projection,
  material/texture upload, all DL-ready DObjs, full `ftdisplaymain.c`,
  continuous draw, shadows/magnify/interface/afterimage/fog, and gameplay
  remain deferred.

Verification:

- `make -j4` passed.
- `scripts/verify-battle-mariofox-dl-scan-harness.ps1` passed with
  `Battle Mario/Fox DL scan harness passed: scene=22/21 dl=0x22dc308/0x2304cd0 asset=4294967294/4294967294 commands=59/69 blocker=0/0 safe=1`.
- `scripts/verify-battle-mariofox-dl-execute-harness.ps1` passed with
  `Battle Mario/Fox DL execute harness passed: commands=59/69 verts=28/23 tris=37/20 safe=1`.
- `scripts/verify-menu-chain-mariofox-dl-scan-harness.ps1` passed with
  `Menu-chain Mario/Fox DL scan harness passed: chain final=22/21 dl=0x22dc9a8/0x2305370 asset=4294967294/4294967294 commands=59/69 blocker=0/0 safe=1`.
- `scripts/verify-menu-chain-mariofox-dl-execute-harness.ps1` passed with
  `Menu-chain Mario/Fox DL execute harness passed: chain final=22/21 commands=59/69 verts=28/23 tris=37/20 safe=1`.
- `scripts/verify-all.ps1` passed with both new DL execute verifiers included
  in the maintained regression chain.

## 2026-06-22: Visible Mario/Fox first-DL software draw proofs

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_draw` as harness modes `29`
  and `30`.
- Reused the existing direct and menu-chain Mario/Fox proof chain through
  model, persistent `FTStruct`, source-order init, original Wait setup, Wait
  callback tick, ground-friction/map proof, display metadata, first-DL scan,
  and first-DL execute/decode.
- Added a bounded software draw probe that reuses the selected real Mario/Fox
  first display lists, decodes them again through `ndsRendererExecuteDisplayList()`,
  collects vertices/triangles, chooses a best nonzero-area fallback projection
  axis, rasterizes filled triangles into deterministic side-by-side boxes, and
  commits the existing retained `96x72` original-DL preview surface.
- Added maintained DL draw diagnostics for preview dimensions/commit state,
  command stats, decoded geometry, chosen axes, source and screen bounds,
  pixel counts, color checksums, fighter state, and safety counters.
- Added `scripts/verify-battle-mariofox-dl-draw-harness.ps1` and
  `scripts/verify-menu-chain-mariofox-dl-draw-harness.ps1`, then wired both
  into `scripts/verify-all.ps1` after standalone passes.

Boundary details:

- Current draw evidence is still limited to the selected first
  display-list-bearing DObj per fighter.
- Current command counts remain at least `59/69`; current geometry proof draws
  `37/20` triangles and `3642/4825` software pixels for Mario/Fox.
- The retained original-DL preview commit proves visible software pixels, not
  DS hardware polygon rendering.
- Camera-correct battle projection, material/texture upload and sampling, all
  fighter DL-ready DObjs beyond the selected first DL, full `ftdisplaymain.c`,
  matrix prep, `gcDrawAll`, DS hardware polygon submission, shadows, magnify,
  interface rendering, gameplay, items, and audio remain deferred.

Verification:

- `scripts/verify-battle-mariofox-dl-draw-harness.ps1` passed with
  `Battle Mario/Fox DL draw harness passed: scene=22/21 pixels=3642/4825 tris=37/20 preview=96x72 commit=1 safe=1`.
- `scripts/verify-menu-chain-mariofox-dl-draw-harness.ps1` passed with
  `Menu-chain Mario/Fox DL draw harness passed: chain final=22/21 pixels=3642/4825 tris=37/20 preview=96x72 commit=1 safe=1`.

## 2026-06-22: Visible Mario/Fox multi-DL software draw proofs

What changed:

- Added `NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw_multi` and
  `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_draw_multi` as harness modes
  `31` and `32`.
- Reused the full existing Mario/Fox proof chain through model creation,
  persistent `FTStruct`, source-order init, original Wait setup, one Wait
  callback tick, Wait ground-friction/map proof, display metadata, first-DL
  scan, first-DL execute/decode, and first-DL software draw.
- Added a bounded multi-DL software draw probe that censuses all DL-ready DObjs
  in the Mario/Fox trees, selects the first four DL-ready DObjs per fighter in
  deterministic depth-first order, executes/decodes all eight selected display
  lists through `ndsRendererExecuteDisplayList()`, and draws them into the
  retained `96x72` original-DL preview surface.
- The multi-DL preview uses one shared projection axis and source bounds per
  fighter across the selected clean DObjs, instead of independently scaling
  each display list.
- Added `G_MODIFYVTX` / opcode `0x02` as a recognized skipped state command in
  the DS renderer adapter. It is not treated as a geometry blocker for the
  current selected fighter DObjs.
- Added maintained `FTR_DL_MULTI*` diagnostics and direct/menu-chain verifier
  scripts, then wired both into `scripts/verify-all.ps1`.

Boundary details:

- The current maintained DL-ready DObj census is `14/18` for Mario/Fox.
- The maintained selected draw set is the first four DL-ready DObjs per fighter.
- Current multi-DL draw evidence is `87/79` triangles and `6208/7411` software
  pixels for Mario/Fox, exceeding the previous first-DL-only proof.
- All eight selected DObjs currently decode cleanly: no renderer blockers,
  unsupported opcodes, unsupported command counts, range rejects, or vertex
  range rejects.
- This is still a bounded software preview. Full `ftdisplaymain.c`,
  camera-correct battle projection, material/texture upload and sampling, all
  remaining fighter DObjs, DS hardware polygon submission, continuous draw
  traversal, shadows, magnify/interface rendering, gameplay, items, HUD, audio,
  and full battle rendering remain deferred.

Verification:

- `scripts/verify-battle-mariofox-dl-draw-multi-harness.ps1` passed with
  `Battle Mario/Fox multi-DL draw harness passed: scene=22/21 candidates=14/18 selected=4/4 pixels=6208/7411 tris=87/79 clean=4/4 preview=96x72 safe=1`.
- `scripts/verify-menu-chain-mariofox-dl-draw-multi-harness.ps1` passed with
  `Menu-chain Mario/Fox multi-DL draw harness passed: chain final=22/21 candidates=14/18 selected=4/4 pixels=6208/7411 tris=87/79 clean=4/4 preview=96x72 safe=1`.

## 2026-06-22: Guarded Mario/Fox all-DL software draw proofs

What changed:

- Added direct and menu-chain harness modes for guarded all-DL Mario/Fox draw
  proofs: `battle_mariofox_dl_draw_all` and
  `menu_chain_mariofox_dl_draw_all`.
- Reused the existing original-source spine through Pupupu VSBattle,
  asset-backed Mario/Fox model GObjs, persistent `FTStruct` shells,
  source-order init, original Wait status/motion, Wait tick, Wait ground,
  display metadata, DL scan/execute, first-DL draw, and multi-DL draw.
- Extended the current project-owned `ftDisplayMainProcDisplay` seam so the
  all-DL proof calls it exactly once for Mario and once for Fox under a DS
  guard, while preserving the no-continuous-draw/no-matrix/no-gameplay
  contract.
- Censused all DL-ready Mario/Fox DObjs (`14/18`), selected all 32 current
  display-list-bearing DObjs, executed them through
  `ndsRendererExecuteDisplayList()`, and drew all clean DObjs into the retained
  `96x72` original-DL software preview.
- Added a screen-aware projection selector and bounded collapsed-triangle
  markers for clean original triangles that flatten under the current preview
  projection. This is diagnostic software preview behavior only; real fighter
  matrix/camera projection remains deferred.
- Added all-DL diagnostics, verifier markers, and direct/menu-chain verifier
  scripts, then wired both into `scripts/verify-all.ps1`.

What is proven:

- Direct proof:
  `Battle Mario/Fox all-DL draw harness passed: scene=22/21 callbacks=2 candidates=14/18 selected=14/18 pixels=16618/15070 tris=334/322 clean=14/18 preview=96x72 safe=1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox all-DL draw harness passed: chain final=22/21 callbacks=2 candidates=14/18 selected=14/18 pixels=16618/15070 tris=334/322 clean=14/18 preview=96x72 safe=1`.

Still deferred:

- Full `ftdisplaymain.c` import, real matrix prep, camera-correct fighter
  projection, material/texture upload and sampling, DS hardware polygon
  submission, continuous `gcDrawAll`, shadows, magnify/interface rendering,
  gameplay, items, HUD, audio, and real fighter process/update loops.

## 2026-06-22: Original Mario/Fox Wait -> Walk input proofs

What changed:

- Added direct and menu-chain harness modes for original Wait -> Walk input
  proofs: `battle_mariofox_walk_input` (`35`) and
  `menu_chain_mariofox_walk_input` (`36`).
- Imported original `ftcommonwalk.c` through
  `src/import/battleship_ftcommon_walk.c`; `decomp/` remains read-only.
- Extended the narrow fighter ABI with Walk status/motion IDs, original
  middle/fast stick thresholds, and the minimal Walk contracts required by the
  imported source.
- Reused the full existing Mario/Fox proof chain through guarded all-DL draw,
  then seeded deterministic forward stick input and entered the transition
  through original `ftCommonWaitProcInterrupt`.
- Routed the guarded `ftCommonGroundCheckInterrupt` seam into original
  `ftCommonWalkCheckInterruptCommon` only while the Walk proof is active.
- Kept dash/run/jump/attack/special/guard/catch follow-up interrupt paths as
  counted no-op compatibility stubs.
- Extended `ftMainSetStatus` to accept only WalkSlow/WalkMiddle/WalkFast during
  the Walk proof while preserving Wait-only behavior for older harnesses.
- Added source-order Walk velocity generation through
  `ftPhysicsSetGroundVelAbsStickRange` and the existing bounded ground-to-air
  transfer mirror, plus one guarded safe floor-map pass.
- Added `FTR_WALK*` diagnostics, direct/menu-chain verifiers, and wired both
  into `scripts/verify-all.ps1`.

What is proven:

- Direct proof:
  `Battle Mario/Fox Walk input harness passed: scene=22/21 status=12/13 motion=6/7 stick=40/80 vel=12000/36000 callbacks=2 safe=1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Walk input harness passed: chain final=22/21 status=12/13 motion=6/7 stick=40/80 vel=12000/36000 callbacks=2 safe=1`.

Still deferred:

- Continuous fighter process scheduling, root-position integration, dash/run/
  turn/jump/squat/attack/special/guard/catch behavior, full collision,
  hit/catch/search, items/weapons, HUD, audio, and full imported `ftmain.c`.

## 2026-06-22: Project hygiene tooling and verifier registry

What changed:

- Added `scripts/clean-generated.ps1` for safe generated-output cleanup with
  dry-run, force, artifact, normal-build, and latest-build preservation modes.
- Reworked `scripts/New-Smash64DSSnapshot.ps1` so Lean ZIP snapshots are the
  default review artifact, excluding generated build outputs, root ROM/ELF
  files, artifacts, emulator payloads/logs, and GDB temps while keeping
  `decomp/` included by default.
- Added `scripts/lib/harness-registry.ps1` as the ordered verifier/harness
  registry and refactored `scripts/verify-all.ps1` to support Full, Latest,
  Smoke, Fighter, Direct, and MenuChain profiles.
- Added `scripts/check-harness-registry.ps1` to detect drift between the
  registry, `include/nds/nds_scene_harness.h`, Makefile harness mappings, and
  verifier scripts.
- Added `scripts/verify-current.ps1` as a short wrapper for the Latest profile.

What is proven:

- The registry check runs without emulator/GDB and verifies the current harness
  mappings and Mario/Fox direct/menu-chain proof pairs.
- Lean snapshot dry-run reports the expected exclusion of build directories,
  root ROM/ELF outputs, artifacts, emulators, and scratch files while retaining
  `decomp/` unless explicitly excluded.

Still deferred:

- No gameplay, renderer, fighter, menu, stage, or audio boundary changed in
  this hygiene pass.

## 2026-06-22: Bounded Mario/Fox Walk movement-loop and release-to-Wait proofs

What changed:

- Added direct and menu-chain harness modes for the first bounded Mario/Fox
  Walk movement-loop proofs: `battle_mariofox_walk_loop` (`37`) and
  `menu_chain_mariofox_walk_loop` (`38`).
- Reused the existing Mario/Fox proof chain through all-DL draw and original
  Wait -> Walk input before running four synthetic held-Walk frames.
- Called original `ftCommonWalkProcInterrupt` and `ftCommonWalkProcPhysics`
  once per held frame while keeping full fighter scheduling disabled.
- Integrated root X from `fp->physics.vel_air` through the guarded proof helper,
  then released stick to neutral and returned both fighters to Wait through
  original Walk interrupt logic.
- Ran one bounded Wait friction/map settle frame and recorded velocity decay,
  root movement, release status, map safety, and escape counters.
- Added `FTR_WALK_LOOP*` diagnostics, direct/menu-chain verifiers, and Latest
  registry profile coverage.

What is proven:

- Direct proof:
  `Battle Mario/Fox Walk loop harness passed: scene=22/21 frames=4/4 root-dx=48000/-144000 release=Wait vel=12000/36000->6000/28000 safe=1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Walk loop harness passed: chain final=22/21 frames=4/4 root-dx=48000/-144000 release=Wait vel=12000/36000->6000/28000 safe=1`.

Still deferred:

- Real DS controller input, continuous fighter process scheduling, imported
  `ftmain.c` manager loops/status table, dash/run/turn/jump/squat/attack/
  special/guard/catch transitions, full collision/ledge logic, jostle,
  hit/catch/search, items/weapons, HUD, audio, and broad gameplay runtime.

## 2026-06-22: Bounded Mario/Fox Dash -> Run -> RunBrake movement proofs

What changed:

- Added direct and menu-chain harness modes for bounded original Dash/Run
  movement proofs: `battle_mariofox_dash_run` (`39`) and
  `menu_chain_mariofox_dash_run` (`40`).
- Imported original `ftcommondash.c`, `ftcommonrun.c`, and
  `ftcommonrunbrake.c` through `src/import` wrappers.
- Extended the narrow fighter ABI with Dash, Run, and RunBrake status/motion
  IDs, tap/hold stick fields, motion/status vars, and the attribute fields
  needed by the imported movement code.
- Reused the existing Mario/Fox proof chain through Walk-loop, seeded
  deterministic dash input, entered original Dash from Wait, crossed the
  bounded Dash -> Run threshold, ran held Run frames, released to neutral, and
  proved original Run -> RunBrake plus bounded RunBrake physics/map ticks.
- Added `DASH_RUN`, `DASH_RUN_STATUS`, `DASH_RUN_CALLS`, and `DASH_RUN_MOVE`
  diagnostics and direct/menu-chain verifiers. The Latest verifier profile now
  covers runtime, Title, direct Dash/Run, and menu-chain Dash/Run.
- Hardened GDB verifier plumbing by adding batch/confirm handling to the shared
  marker helper and moving `verify-runtime.ps1` to the maintained 135-second
  Title sample window. This avoids sampling after later bounded paths have
  overwritten startup diagnostics.

What is proven:

- Direct proof:
  `Battle Mario/Fox Dash-Run harness passed: dash->run->runbrake root-dx=301575/-418500 vel=51200/71000->47450/66000`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 dash->run->runbrake root-dx=301575/-418500 vel=51200/71000->47450/66000`.
- Current profile:
  `verify-current.ps1 -Build -> Latest verification profile passed`.
- Full regression:
  `verify-all.ps1 -Profile Full -> Full verification profile passed`.

Still deferred:

- Continuous fighter process scheduling, real DS controller input,
  turn/jump/squat/attack/special/guard/catch transitions, full collision/ledge
  logic, jostle, hit/catch/search, items/weapons, HUD, audio, and full imported
  `ftmain.c`.

## 2026-06-22: Bounded Mario/Fox RunBrake -> Wait -> KneeBend -> JumpF proofs

What changed:

- Added direct and menu-chain harness modes for the first bounded original
  ground-to-air movement proof: `battle_mariofox_jump_loop` (`41`) and
  `menu_chain_mariofox_jump_loop` (`42`).
- Imported original `ftcommonkneebend.c` and `ftcommonjump.c` through
  `src/import` wrappers with guarded project-owned public seams.
- Extended the narrow fighter ABI with KneeBend/Jump status and motion IDs,
  button-release input, KneeBend status vars, jump constants, and bounded
  air-physics/map declarations.
- Reused the full Mario/Fox proof chain through Dash -> Run -> RunBrake,
  closed RunBrake back to Wait through the guarded original-compatible
  `ftAnimEndSetWait` path, seeded deterministic C-button jump input, entered
  KneeBend through original `ftCommonWaitProcInterrupt`, advanced bounded
  KneeBend updates until original `ftCommonJumpSetStatus`, and ran six bounded
  JumpF airborne frames.
- Added `JUMP_LOOP`, `JUMP_INPUT`, `JUMP_STATUS`, `JUMP_GA`, `JUMP_CALLS`,
  `JUMP_FRAMES`, `JUMP_MOVE`, `JUMP_VEL`, `JUMP_DEFER`, and `JUMP_SAFE`
  diagnostics and direct/menu-chain verifiers. The Latest verifier profile now
  covers runtime, Title, direct Jump-loop, and menu-chain Jump-loop.

What is proven:

- Direct proof:
  `Battle Mario/Fox Jump-loop harness passed: jump dx=100800/-138900 dy=395400/468000 vy=74300/92000->59900/68000`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Jump-loop harness passed: jump dx=100800/-138900 dy=395400/468000 vy=74300/92000->59900/68000`.
- Current profile:
  `verify-current.ps1 -Build -> Latest verification profile passed`.

Still deferred:

- Landing, Fall/FallAerial, JumpAerial/double jump, aerial attacks/specials,
  cliff/ceiling collision, continuous fighter process scheduling, real DS
  controller input, hit/catch/search, items/weapons, HUD, audio, and full
  imported `ftmain.c`.

## 2026-06-23: Bounded Mario/Fox JumpF -> Fall -> LandingLight -> Wait proofs

What changed:

- Added direct and menu-chain harness modes for the first complete bounded
  ground-air-ground fighter proof: `battle_mariofox_landing_loop` (`43`) and
  `menu_chain_mariofox_landing_loop` (`44`).
- Imported original `ftcommonfall.c` and `ftcommonlanding.c` through
  `src/import` wrappers with guarded project-owned public seams.
- Extended the narrow fighter ABI with original-compatible Fall, FallAerial,
  LandingLight, LandingHeavy, LandingFallSpecial, and LandingAirNull status and
  motion IDs, Landing status vars, and the small Fall/Landing function surface.
- Reused the full Mario/Fox proof chain through JumpF, entered Fall through
  guarded `ftAnimEndSetFall`, ran bounded Fall interrupt/air-physics/map
  frames, detected a Pupupu floor crossing in the DS-owned map seam, called
  original `ftCommonLandingSetStatus`, proved LandingLight, closed it back to
  Wait through `ftAnimEndSetWait`, and ran one post-landing Wait friction/map
  settle frame.
- Added `LAND_LOOP`, `LAND_STATUS`, `LAND_MOTION`, `LAND_GA`, `LAND_CALLS`,
  `LAND_FRAMES`, `LAND_MAP`, `LAND_MOVE`, `LAND_VEL`, and `LAND_SAFE`
  diagnostics and direct/menu-chain verifiers. The Latest verifier profile now
  covers runtime, Title, direct Landing-loop, and menu-chain Landing-loop.

What is proven:

- Direct proof:
  `Battle Mario/Fox Landing-loop harness passed: scene=22/21 fall=26/26 landing=31/31 wait=10/10 frames=16/14 floor=0/0 safe=1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Landing-loop harness passed: scene=22/21 fall=26/26 landing=31/31 wait=10/10 frames=16/14 floor=0/0 safe=1`.
- Current profile:
  `verify-current.ps1 -> Latest verification profile passed`.

Still deferred:

- Full collision line tracing, platform pass-through, ledges, ceiling bonks,
  FallAerial, LandingHeavy, JumpAerial/double jump, aerial attacks/specials,
  continuous fighter process scheduling, real DS controller input,
  hit/catch/search, items/weapons, HUD, audio, and full imported `ftmain.c`.

## 2026-06-23: Bounded scripted Mario/Fox process-loop proofs

What changed:

- Added direct and menu-chain harness modes for the first bounded scripted
  fighter process-loop proof: `battle_mariofox_process_loop` (`45`) and
  `menu_chain_mariofox_process_loop` (`46`).
- Reused the existing Mario/Fox proof chain through Landing-loop, then ran both
  fighters through one shared source-order frame driver: update, interrupt,
  physics, root integration, and map.
- Added a deterministic controller-input bridge that writes the fighter input
  fields and mirrors the same scripted state to the controller device slot for
  diagnostics.
- Proved three original movement paths for both Mario and Fox without importing
  full fighter gameplay: Wait -> Walk -> Wait, Wait -> Dash -> Run ->
  RunBrake -> Wait, and Wait -> KneeBend -> JumpF -> Fall -> LandingLight ->
  Wait.
- Added `PROC_LOOP`, `PROC_INPUT`, `PROC_STATUS`, `PROC_VISITS`, `PROC_CALLS`,
  `PROC_MOVE`, `PROC_VEL`, `PROC_TRANS`, and `PROC_SAFE` verifier marker
  groups. The Latest verifier profile now covers runtime, Title, direct
  Process-loop, and menu-chain Process-loop.

What is proven:

- Direct proof:
  `Battle Mario/Fox process-loop harness passed: scene=22/21 frames=30/28 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox process-loop harness passed: scene=22/21 frames=30/28 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1`.

Still deferred:

- Real DS controller input, unbounded object-manager process scheduling,
  turn/squat/attack/special/guard/catch paths, full collision line tracing,
  platform pass-through, ledges, hit/catch/search, items/weapons, HUD, audio,
  full fighter display traversal, and full imported `ftmain.c`.

## 2026-06-23: VSBattle update-driven Mario/Fox GObjProcess scheduler-loop proofs

What changed:

- Added direct and menu-chain harness modes for the first bounded
  VSBattle-update-driven fighter scheduler proof:
  `battle_mariofox_scheduler_loop` (`47`) and
  `menu_chain_mariofox_scheduler_loop` (`48`).
- Reused the existing Mario/Fox process-loop proof chain, then attached one
  selected `GObjProcess` callback for Mario and one for Fox through original
  `gcAddGObjProcess`.
- Invoked those callbacks with original `gcRunGObjProcess` from a bounded
  `scVSBattleFuncUpdate` wrapper and a capped VSBattle taskman update loop,
  without calling full `gcRunAll`.
- Preserved the existing process-loop diagnostics by snapshotting/restoring
  the prerequisite proof state before running the scheduler-facing proof.
- Added `SCHED_LOOP`, `SCHED_TASKMAN`, `SCHED_PROCESS`, `SCHED_INPUT`,
  `SCHED_STATUS`, `SCHED_VISITS`, `SCHED_CALLS`, `SCHED_MOVE`,
  `SCHED_TRANS`, and `SCHED_SAFE` marker groups plus direct/menu-chain
  verifier scripts. The Latest verifier profile now covers runtime, Title,
  direct Scheduler-loop, and menu-chain Scheduler-loop.

What is proven:

- Direct proof:
  `Battle Mario/Fox scheduler-loop harness passed: scene=22/21 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox scheduler-loop harness passed: scene=22/21 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1`.

Still deferred:

- Full `gcRunAll` process scheduling, arbitrary fighter processes, real DS
  controller input, turn/squat/attack/special/guard/catch paths, full
  collision line tracing, platform pass-through, ledges, hit/catch/search,
  items/weapons, HUD, audio, full fighter display traversal, and full imported
  `ftmain.c`.

## 2026-06-23: Controller-source Mario/Fox scheduler-loop proofs

What changed:

- Added direct and menu-chain harness modes for the first bounded
  controller-source-driven fighter scheduler proof:
  `battle_mariofox_controller_loop` (`49`) and
  `menu_chain_mariofox_controller_loop` (`50`).
- Added deterministic DS controller playback to `controller_backend.c`.
  Playback-disabled builds keep the existing live DS key mapping; playback
  enabled feeds only `OSContPad` data from the verifier harness.
- Reused the full Mario/Fox proof chain through the existing scheduler-loop
  endpoint, then ran original `syControllerReadDeviceData` and
  `syControllerUpdateGlobalData` before bridging `gSYControllerDevices` into
  `FTStruct` input through DS-owned code.
- Invoked selected Mario/Fox `GObjProcess` callbacks through original
  `gcRunGObjProcess` from bounded `scVSBattleFuncUpdate` taskman updates.
- Added `CTRL_LOOP`, `CTRL_BACKEND`, `CTRL_TASKMAN`, `CTRL_PROCESS`,
  `CTRL_INPUT`, `CTRL_STATUS`, `CTRL_VISITS`, `CTRL_CALLS`, `CTRL_MOVE`,
  `CTRL_TRANS`, and `CTRL_SAFE` verifier marker groups. The Latest verifier
  profile now covers runtime, Title, direct Controller-loop, and menu-chain
  Controller-loop.

What is proven:

- Direct proof:
  `Battle Mario/Fox controller-loop harness passed: scene=22/21 reads=30 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox controller-loop harness passed: scene=22/21 reads=30 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1`.

Still deferred:

- Arbitrary live player input combinations, full `gcRunAll`, continuous
  unbounded taskman scheduling, arbitrary fighter processes, turn/squat/
  attack/special/guard/catch paths, full collision line tracing, platforms,
  ledges, hit/catch/search, items/weapons, HUD, audio, camera-correct battle
  projection, full fighter display traversal, and full imported `ftmain.c`.

## 2026-06-23: Moving Mario/Fox battle-preview loop proofs

What changed:

- Added direct and menu-chain harness modes for the first moving
  controller-source battle-preview proof:
  `battle_mariofox_preview_loop` (`51`) and
  `menu_chain_mariofox_preview_loop` (`52`).
- Reused the existing scheduler-loop and controller-loop proof chain, then ran
  a new bounded preview loop from the same `scVSBattleFuncUpdate` taskman path.
- Kept movement controller-source-driven: deterministic `OSContPad` playback
  still goes through original `syControllerReadDeviceData` and
  `syControllerUpdateGlobalData` before DS-owned bridging into `FTStruct`
  input.
- Added guarded `ftDisplayMainProcDisplay` sampling for the preview loop. The
  proof opens a bounded `96x72` software preview surface, samples the current
  Mario/Fox DObj display callback, draws diagnostic per-DObj fighter markers
  root-coupled to the moving fighter state, and commits the preview surface.
- Added `PREV_LOOP`, `PREV_BACKEND`, `PREV_TASKMAN`, `PREV_PROCESS`,
  `PREV_INPUT`, `PREV_STATUS`, `PREV_CALLS`, `PREV_MOVE`, `PREV_DRAW`,
  `PREV_SCREEN`, `PREV_TRANS`, and `PREV_SAFE` verifier marker groups. The
  Latest verifier profile now covers runtime, Title, direct Preview-loop, and
  menu-chain Preview-loop.

What is proven:

- Direct proof:
  `Battle Mario/Fox preview-loop harness passed: scene=22/21 drawFrames=7 callbacks=14 pixels=582 screenDx=61/-62 screenRise=52/52 rootDx=137250/-214000 final=Wait/Ground safe=1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox preview-loop harness passed: scene=22/21 drawFrames=7 callbacks=14 pixels=582 screenDx=61/-62 screenRise=52/52 rootDx=137250/-214000 final=Wait/Ground safe=1`.
- Registry/current proof:
  `check-harness-registry.ps1 -> Harness registry check passed: 52 harness mappings, 56 verifier scripts, 0 drift` and
  `verify-current.ps1 -SkipRegistryCheck -> Latest verification profile passed`.

Still deferred:

- Arbitrary live DS input as verifier input, real camera-correct fighter
  projection, matrix prep, full fighter display traversal, DS hardware polygon
  rendering for fighters, full `gcRunAll`, continuous gameplay scheduling,
  broader fighter statuses, full collision/ledge/platform handling, HUD,
  items/weapons, audio, and full imported `ftmain.c`.

## 2026-06-23: Bounded Mario/Fox gcRunAll moving-preview proofs

What changed:

- Added direct and menu-chain harness modes for the first bounded original
  object-manager run-all fighter proof:
  `battle_mariofox_gcrunall_loop` (`53`) and
  `menu_chain_mariofox_gcrunall_loop` (`54`).
- Fixed the harness registry checker drift so Latest now tracks the current
  boundary and the paired Mario/Fox proof list includes `preview_loop` and
  `gcrunall_loop`.
- Reused the verified Mario/Fox chain through the moving preview-loop endpoint,
  then paused previous proof-owned and non-target object processes.
- Attached selected Mario/Fox callbacks with original `gcAddGObjProcess` and
  advanced those callbacks through original `gcRunAll()` from the bounded
  VSBattle update path.
- Kept controller input source-compatible: deterministic `OSContPad` playback
  still goes through original controller read/global-update before the DS-owned
  bridge updates `FTStruct` input.
- Added `GCRUNALL_LOOP`, `GCRUNALL_TASKMAN`, `GCRUNALL_RUN`,
  `GCRUNALL_PROCESS`, `GCRUNALL_INPUT`, `GCRUNALL_STATUS`,
  `GCRUNALL_VISITS`, `GCRUNALL_CALLS`, `GCRUNALL_DRAW`,
  `GCRUNALL_SCREEN`, `GCRUNALL_MOVE`, `GCRUNALL_TRANS`, and
  `GCRUNALL_SAFE` verifier marker groups. The Latest verifier profile now
  covers runtime, Title, direct gcRunAll-loop, and menu-chain gcRunAll-loop.

What is proven:

- Direct proof:
  `Battle Mario/Fox gcRunAll-loop harness passed: scene=22/21 gcRunAll=30 callbacks=30/30 draws=11 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox gcRunAll-loop harness passed: scene=22/21 gcRunAll=30 callbacks=30/30 draws=11 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1`.

Still deferred:

- Unpaused full-scene `gcRunAll`, continuous unbounded taskman scheduling,
  arbitrary live input, turn/squat/attack/special/guard/catch paths, full
  collision/platform/ledge logic, hit/catch/search, items/weapons, HUD, audio,
  camera-correct battle projection, hardware fighter rendering, and full
  imported `ftmain.c`.

## 2026-06-23: Live-input Mario/Fox moving-preview idle proofs

What changed:

- Added direct and menu-chain harness modes for the first live DS controller
  source boundary:
  `battle_mariofox_live_preview` (`55`) and
  `menu_chain_mariofox_live_preview` (`56`).
- Reused the verified Mario/Fox proof chain through the bounded original
  `gcRunAll` moving-preview endpoint, then switched the controller source from
  deterministic playback to live `osContGetReadData` samples for the maintained
  idle proof.
- Kept the source path original-compatible: live input still flows through
  original `syControllerReadDeviceData` and `syControllerUpdateGlobalData`
  before the DS-owned bridge copies controller state into the selected
  `FTStruct` input fields.
- Added live controller diagnostics for connected mask, P0 buttons/stick,
  mapping count, live read count, frame count, taskman/update counts, selected
  callback counts, idle status, draw evidence, and safety counters.
- Added `LIVE_LOOP`, `LIVE_BACKEND`, `LIVE_TASKMAN`, `LIVE_RUN`,
  `LIVE_INPUT`, `LIVE_STATUS`, `LIVE_CALLS`, `LIVE_DRAW`, `LIVE_MOVE`, and
  `LIVE_SAFE` verifier marker groups. The Latest verifier profile now covers
  runtime, Title, direct live-preview, and menu-chain live-preview.
- Added a dev build switch for longer manual live movement checks:
  `NDS_DEV_LIVE_INPUT_PREVIEW=1`.

What is proven:

- Direct proof:
  `Battle Mario/Fox live-preview harness passed: scene=22/21 liveReads=60 frames=60/60 draws=16 pixels=1410 idle=Wait/Ground directInput=0 safe=1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox live-preview harness passed: scene=22/21 liveReads=60 frames=60/60 draws=16 pixels=1410 idle=Wait/Ground directInput=0 safe=1`.
- Registry/current proof:
  `check-harness-registry.ps1 -> Harness registry check passed: 56 harness mappings, 60 verifier scripts, 0 drift`.
- Dev build proof:
  `make TARGET=smash64ds-live-preview BUILD=build-live-preview NDS_DEV_SCENE_HARNESS=battle_mariofox_live_preview NDS_DEV_LIVE_INPUT_PREVIEW=1 -j4`.

Still deferred:

- This is not a full playable battle loop. The automated proof verifies 60
  neutral live-input frames and stable Wait/Ground state only. Arbitrary live
  movement combinations, attacks, specials, guard, catch, items/weapons,
  full collision/platform/ledge logic, hit/catch/search, HUD, audio, real
  battle camera projection, hardware fighter rendering, unbounded taskman
  scheduling, and full imported `ftmain.c` remain deferred.

## 2026-06-24: Bounded Mario/Fox gcDrawAll moving-preview proofs

What changed:

- Added direct and menu-chain harness modes for the first bounded original
  object-manager draw traversal proof:
  `battle_mariofox_gcdrawall_loop` (`57`) and
  `menu_chain_mariofox_gcdrawall_loop` (`58`).
- Moved the registry Latest profile to runtime, Title, direct gcDrawAll-loop,
  and menu-chain gcDrawAll-loop.
- Reused the full verified Mario/Fox chain through the live-input idle proof,
  then re-enabled deterministic controller playback for the moving proof
  phase.
- Advanced selected Mario/Fox callbacks through original `gcRunAll()` and
  rendered moving all-DL keyframes by calling original `gcDrawAll()` rather
  than manually invoking `ftDisplayMainProcDisplay`.
- Counted selected Mario/Fox display callbacks from inside the
  `ftDisplayMainProcDisplay` seam only while the gcDrawAll display guard is
  active, and kept non-target display callbacks masked/guarded.
- Added direct and menu-chain gcDrawAll-loop verifiers plus `GCDRAWALL_LOOP`,
  `GCDRAWALL_TASKMAN`, `GCDRAWALL_RUN`, `GCDRAWALL_PROCESS`,
  `GCDRAWALL_INPUT`, `GCDRAWALL_STATUS`, `GCDRAWALL_DRAW`,
  `GCDRAWALL_SCREEN`, `GCDRAWALL_MOVE`, `GCDRAWALL_TRANS`, and
  `GCDRAWALL_SAFE` marker groups.

What is proven:

- Direct proof:
  `Battle Mario/Fox gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1`.
- Registry/current proof:
  `check-harness-registry.ps1 -> Harness registry check passed: 58 harness mappings, 62 verifier scripts, 0 drift` and
  `verify-current.ps1 -> Latest verification profile passed`.

Still deferred:

- The gcDrawAll-loop harnesses still mask/guard non-target display callbacks.
  They do not prove unpaused full-scene draw traversal, DS hardware polygon
  rendering, camera-correct battle matrices, full fighter display, full
  collision/platform/ledge logic, attacks, specials, guard, catch, items,
  hit/search, HUD, audio, or full imported `ftmain.c`.

## 2026-06-24: Pupupu stage-inclusive gcDrawAll traversal proofs

What changed:

- Added direct and menu-chain harness modes for the selected Pupupu
  stage-inclusive original object-manager draw traversal proof:
  `battle_mariofox_stage_gcdrawall_loop` (`59`) and
  `menu_chain_mariofox_stage_gcdrawall_loop` (`60`).
- Reused the existing bounded Mario/Fox moving `gcDrawAll` proof and added
  stage diagnostics around original `gcDrawAll -> func_80017EC0 ->
  gcCaptureCameraGObj` plus the existing DS-owned `gcDrawDObjTree*` bridges.
- Recorded selected Pupupu display-layer and map-GObj camera capture masks,
  stage DObj draw bridge masks, layer/map DL-ready masks, safety counters, and
  retained preview pixel evidence.
- Kept traversal bounded to selected Pupupu layer/map GObjs. No manual stage
  display calls are used; stage display callbacks are observed from the
  original object-manager draw traversal.
- Added direct and menu-chain verifier wrappers and moved the registry Latest
  profile to runtime, Title, direct stage-inclusive gcDrawAll, and menu-chain
  stage-inclusive gcDrawAll.

What is proven:

- Direct proof:
  `Battle Mario/Fox stage gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0`.
- Menu-chain proof:
  `Menu-chain Mario/Fox stage gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0`.
- Registry proof:
  `check-harness-registry.ps1 -> Harness registry check passed: 60 harness mappings, 64 verifier scripts, 0 drift`.

Still deferred:

- The stage-inclusive gcDrawAll-loop harnesses still mask/guard non-target
  display callbacks. They do not prove unpaused full-scene draw traversal,
  hardware polygon rendering, camera-correct battle matrices, continuous stage
  draw/update, Whispy wind/yakumono runtime, collision lines, items, HUD,
  audio, or full gameplay.

## 2026-06-24: Geometry-backed Pupupu floor-collision proofs

What changed:

- Added direct and menu-chain harness modes for the selected Pupupu
  geometry-backed floor projection proof:
  `battle_mariofox_stage_collision_loop` (`61`) and
  `menu_chain_mariofox_stage_collision_loop` (`62`).
- Reused the stage-inclusive Mario/Fox `gcDrawAll` proof and made the new
  collision modes opt into a real Pupupu `MPGroundData` / `MPGeometryData`
  floor scan inside the project-owned `mpCollisionCheckProjectFloor` seam.
  Older harnesses keep their previous flat-floor compatibility path.
- Added minimal project-owned `gr/ground.h` map/collision declarations for the
  loaded Pupupu geometry layout and added O2R-aware halfword readers for the
  byte-swapped geometry tables.
- Added floor projection diagnostics for geometry readiness, yakumono/map-object
  counts, floor/total line counts, geometry-backed project calls, deliberate
  offstage/below-floor misses, real line IDs, floor flags, floor angles, and
  left/right edge samples.
- During the new proof only, the selected proof-owned Mario/Fox roots adopt the
  current real Pupupu floor before and after the bounded moving draw slice so
  final floor evidence reflects the real geometry at the fighters' current X
  positions.

What is proven:

- Direct proof:
  `Battle Mario/Fox stage collision-loop harness passed: scene=22/21 gcRunAll=37 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=4/6 floorLines=4 probes=3/2`.
- Menu-chain proof:
  `Menu-chain Mario/Fox stage collision-loop harness passed: scene=22/21 gcRunAll=37 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=4/6 floorLines=4 probes=3/2`.
- Regression spot checks:
  `verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1` and
  `verify-menu-chain-mariofox-stage-gcdrawall-loop-harness.ps1` still pass,
  proving the new geometry path is opt-in and does not replace the old
  stage-inclusive draw proof.

Still deferred:

- This is not full BattleShip map collision processing. Platforms, ledges,
  wall/ceiling checks, cliffcatch, floor-edge behavior beyond the current edge
  metadata, arbitrary stage hazards, items, HUD, audio, unbounded gameplay, and
  full imported `ftmain.c` remain deferred.

## 2026-06-24: Corrected Pupupu floor-line decoding proof

What changed:

- Corrected the project-owned Pupupu `MPLineInfo` halfword decoder to match the
  original BattleShip layout: yakumono ID, then floor/ceil/rwall/lwall
  group/count pairs.
- Replaced the hardcoded `MPLineInfo` stride with `sizeof(MPLineInfo)` so O2R
  line-info indexing follows the compatibility struct layout instead of the old
  20-byte assumption.
- Added line-kind classification helpers and new diagnostics for decoded floor
  range, final P0/P1 floor-line kinds, final line-is-floor flags, non-floor
  candidate count, and guarded yakumono-DObj access.
- Changed the geometry projector to walk decoded floor ranges only and reject
  any non-floor candidate instead of accepting any valid line ID.
- Removed unsafe bounded-proof indexing into `MPYakumonoDObj::dobjs` for
  yakumono IDs beyond the one-entry project shim. The proof records the
  deferred/guarded yakumono access and treats static Pupupu collision lines as
  active for this bounded sample.
- Added a proof-owned floor sample seed that derives P0/P1 sample X positions
  from decoded Pupupu floor endpoints before the final collision projection.
  This keeps the current collision proof on real decoded floor data while full
  continuous map collision remains deferred.
- Tightened the stage-collision proof mask from `0x1fff` to `0xffff` and added
  verifier checks for `STAGE_COLLISION_KIND`.

What is proven:

- Direct proof:
  `Battle Mario/Fox stage collision-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2`.
- Menu-chain proof:
  `Menu-chain Mario/Fox stage collision-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2`.
- Regression spot checks:
  `verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1` and
  `verify-menu-chain-mariofox-stage-gcdrawall-loop-harness.ps1` still pass, so
  modes `59/60` keep their existing stage-inclusive draw behavior.

Still deferred:

- This remains a bounded floor-projection proof, not full BattleShip map
  collision. Continuous floor following through arbitrary movement, platforms,
  ledges, ceilings, walls, cliffcatch, yakumono runtime, items, HUD, audio,
  unbounded gameplay, and full imported `ftmain.c` remain deferred.

## 2026-06-24: Added tiered verifier workflow profiles

What changed:

- Added registry-driven verifier profiles for normal iteration:
  `BoundaryDirect`, `Boundary`, and `Regression`.
- `BoundaryDirect` runs only the latest direct current-boundary harness:
  `battle_mariofox_stage_collision_loop`.
- `Boundary` runs the latest direct + menu-chain current-boundary pair:
  `battle_mariofox_stage_collision_loop` and
  `menu_chain_mariofox_stage_collision_loop`.
- `Regression` runs runtime/Title plus the immediate previous draw/collision
  direct/menu-chain pairs: `battle_mariofox_gcdrawall_loop`,
  `menu_chain_mariofox_gcdrawall_loop`,
  `battle_mariofox_stage_gcdrawall_loop`,
  `menu_chain_mariofox_stage_gcdrawall_loop`,
  `battle_mariofox_stage_collision_loop`, and
  `menu_chain_mariofox_stage_collision_loop`.
- Kept `Latest` as runtime + Title + latest direct/menu-chain, and kept `Full`
  unchanged as the complete registry suite.
- Added wrapper scripts: `scripts/verify-dev-fast.ps1`,
  `scripts/verify-boundary.ps1`, and `scripts/verify-regression.ps1`.
  `scripts/verify-current.ps1` remains the Latest wrapper.
- Extended `scripts/check-harness-registry.ps1` to validate the new profile
  contents and wrapper-script existence without adding new harness records or
  changing mode counts.
- Updated active docs to make Full an explicit risk-based gate rather than a
  routine per-task requirement. Full remains required for major snapshots, wide
  refactors, Makefile/source-list/header ABI changes, shared taskman/object
  manager/controller/reloc/display changes, verifier registry/checker work, or
  explicit reviewer request.

Validation:

- `make -j16` remains the active build command for this machine.
- `scripts/check-harness-registry.ps1` validates the new profile definitions
  and still reports the same harness/script registry counts.
- `verify-all.ps1 -List` now works for `BoundaryDirect`, `Boundary`, `Latest`,
  and `Regression`.

Still deferred:

- This workflow update does not change gameplay, renderer, movement,
  menu-chain, controller, or collision runtime behavior. Full is still available
  for broad regression, but it is no longer the default every-task handoff gate.

## 2026-06-24: Added continuous geometry-backed Pupupu floor-follow proofs

What changed:

- Added direct and menu-chain `battle_mariofox_stage_floor_follow_loop`
  harnesses as modes `63/64`.
- Built the new proof on top of the existing Pupupu stage-inclusive
  `gcDrawAll` and geometry-backed floor-collision proofs without importing full
  map collision or unbounding gameplay.
- Added an opt-in proof-owned floor-follow update path that projects selected
  Mario/Fox roots against decoded Pupupu floor lines during the bounded moving
  slice, clamps root Y to projected floor Y, and updates the bounded
  `FTStruct` collision state.
- Kept older `stage_collision_loop` modes `61/62` on the final-sample
  floor-projection behavior, including their explicit final re-center/adopt
  step, so the new continuous floor-follow behavior is isolated to the new
  modes.
- Added `STAGE_FLOOR_FOLLOW*` diagnostics for setup, per-player updates,
  projection hits, floor IDs/kinds, final root/floor Y, post-clamp drift,
  status, ground/air state, and visit masks.
- Added direct/menu-chain verifier wrappers and registry profile updates so
  `verify-current.ps1`, `verify-boundary.ps1`, and `verify-regression.ps1`
  target the new current boundary.

What is proven:

- Direct proof:
  `Battle Mario/Fox stage floor-follow-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=0/0 updates=18/18 drift=0/0 visits=0x1/0x1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox stage floor-follow-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=0/0 updates=18/18 drift=0/0 visits=0x1/0x1`.
- Regression spot checks:
  `verify-battle-mariofox-stage-collision-loop-harness.ps1`,
  `verify-menu-chain-mariofox-stage-collision-loop-harness.ps1`,
  `verify-current.ps1`, `verify-boundary.ps1`, and `verify-regression.ps1`
  pass with the new profile boundary.

Still deferred:

- This is continuous selected-fighter floor following for a bounded proof slice,
  not full BattleShip map collision. Platform pass-through, ledges, ceilings,
  walls, arbitrary slopes beyond the selected decoded floor, cliffcatch,
  stage hazards, items, HUD, audio, unbounded gameplay, and full imported
  `ftmain.c` remain deferred.

## 2026-06-24: Added real Pupupu floor-edge / original MP floor-query proofs

What changed:

- Added direct and menu-chain `battle_mariofox_stage_floor_edge_loop`
  harnesses as modes `65/66`.
- Built the new proof on top of the existing Pupupu stage-inclusive
  `gcDrawAll`, geometry-backed floor-collision, and continuous floor-follow
  proofs without importing full map collision or unbounding gameplay.
- Added narrow original-compatible MP query helpers in project-owned backend
  code: `mpCollisionGetLineTypeID`, `mpCollisionGetVertexPositionID`,
  `mpCollisionGetFCCommonFloor`, `mpCollisionGetEdgeUnderLLineID`, and
  `mpCollisionGetEdgeUnderRLineID`.
- Kept edge-under helpers deliberately bounded: they count calls and return
  `-1` until real wall/ledge/platform contracts are imported.
- Added `STAGE_FLOOR_EDGE*` diagnostics for selected line metadata, inside and
  outside floor probes, edge-distance deltas, MP query counts, inherited
  floor-follow updates, and safety counters.
- Updated verifier wrappers, registry profiles, and active docs so
  `verify-current.ps1`, `verify-boundary.ps1`, and `verify-regression.ps1`
  target the new current boundary.

What is proven:

- Direct proof:
  `Battle Mario/Fox stage floor-edge-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=6/4`.
- Menu-chain proof:
  `Menu-chain Mario/Fox stage floor-edge-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=6/4`.
- Current profile proof:
  `verify-current.ps1 -> Latest verification profile passed`.

Still deferred:

- This is a selected-fighter floor-edge and MP floor-query proof, not full
  BattleShip map collision. Wall/ledge/platform edge-under resolution,
  platform pass-through, ceilings, walls, arbitrary slopes, cliffcatch, stage
  hazards, items, HUD, audio, unbounded gameplay, and full imported
  `ftmain.c` remain deferred.

## 2026-06-24: Added source-order MP floor-process proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpprocess_floor_loop` harnesses as modes `67/68`.
- Added a narrow project-owned original-layout `MPCollData` compatibility
  contract in `include/mp/map.h`.
- Corrected the MP helper ABI surface used by the bounded floor-process slice:
  `mpCollisionGetLineTypeID` returns `s32`,
  `mpCollisionGetVertexPositionID` is a void output helper, and
  `mpCollisionGetFCCommonFloor` now reports signed floor distance for objects
  below the selected floor instead of rejecting them.
- Added source-order floor-only copies of
  `mpProcessSetCollProjectFloorID` and
  `mpProcessCheckTestFloorCollisionNew`, plus local-probe calls to
  `mpProcessSetLandingFloor` and `mpProcessSetCollideFloor`.
- Added an `FTStruct` collision-shell to `MPCollData` adapter used after the
  existing floor-follow update, with floor-field copyback only.
- Added `STAGE_MPPROCESS_FLOOR*` diagnostics and verifier assertions, then
  promoted modes `67/68` to the current Boundary/Latest profiles.

What is proven:

- Direct proof:
  `Battle Mario/Fox stage MP floor-process-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=50/46 mpProcessFloor=test=38/2 project=0/2 probes=1/2 below=2 p0line=3 p1line=3 fc=2/1/39`.
- Menu-chain proof:
  `Menu-chain Mario/Fox stage MP floor-process-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=50/46 mpProcessFloor=test=38/2 project=0/2 probes=1/2 below=2 p0line=3 p1line=3 fc=2/1/39`.
- Profile proof:
  `verify-current.ps1 -> Latest verification profile passed` and
  `verify-boundary.ps1 -> Boundary verification profile passed`.

Still deferred:

- This is a floor-only source-order MP process slice for selected proof-owned
  Mario/Fox fighters, not full BattleShip map collision. Platform
  pass-through, ledges, wall edge-under resolution, ceilings, walls,
  cliffcatch, full fighter map callbacks, arbitrary live gameplay, items, HUD,
  audio, unbounded gameplay, and full imported `ftmain.c` remain deferred.

## 2026-06-24: Added source-order MP update-main floor-loop proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpupdate_floor_loop` harnesses as modes `69/70`.
- Added bounded source-order `mpProcessUpdateMain` behavior in project-owned
  backend code, including reset-to-previous-position, substep splitting,
  callback invocation, update tick recording, and conservative cap diagnostics.
- Added a floor-only `mpCommonRunFighterAllCollisions` callback and narrow
  `mpCommonCheckFighterOnFloor` / `mpCommonCheckFighterOnCliffEdge` wrappers
  over the existing original-layout `MPCollData` adapter.
- Routed selected Mario/Fox map callbacks through the new update-main path only
  in modes `69/70`, after preserving the existing MP floor-process,
  floor-edge, floor-follow, stage-collision, and stage `gcDrawAll` proofs.
- Added `STAGE_MPUPDATE_FLOOR*` diagnostics and verifier wrappers, promoted
  modes `69/70` to the Boundary/Latest profiles, and fixed the
  `scene_backend.o` dependency on included backend slices so slice edits
  reliably rebuild.

What is proven:

- Direct proof:
  `Battle Mario/Fox stage mpProcessUpdateMain floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000`.
- Menu-chain proof:
  `Menu-chain Mario/Fox stage mpProcessUpdateMain floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000`.
- Profile proofs:
  `verify-boundary.ps1 -> Boundary verification profile passed`,
  `verify-current.ps1 -> Latest verification profile passed`, and
  `verify-regression.ps1 -> Regression verification profile passed`.

Still deferred:

- This is a bounded selected-fighter, floor-only `mpProcessUpdateMain` proof,
  not full BattleShip map collision. Wall tests, ceiling tests, floor-edge
  adjustment, second-floor tests, edge-under wall adjacency, platform
  pass-through, ledges/cliffcatch, Fall/Ottotto, moving yakumono collision,
  items, HUD, audio, arbitrary live gameplay, unpaused full-scene
  `gcRunAll`/`gcDrawAll`, and full imported `ftmain.c` remain deferred.

## 2026-06-24: Added source-order MP floor-line sweep / second-floor proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpsweep_floor_loop` harnesses as modes `71/72`.
- Added bounded source-order floor-line sweep helpers for the current Pupupu
  floor slice, including same-line rejection, different-line acceptance, and
  no-hit miss paths.
- Replaced the previous second-floor-test deferral in the floor-only
  `mpCommonRunFighterAllCollisions` slice with
  `mpProcessCheckTestFloorCollision` plus source-order landing/floor-edge
  bookkeeping for the bounded proof.
- Kept the selected Mario/Fox callback path on decoded Pupupu floor line `3`
  while proving that the second-floor branch is called and rejects same-line or
  no-new-floor cases; standalone probes prove the accepted different-line path
  from line `3` to line `0`.
- Added `STAGE_MPSWEEP_FLOOR*` diagnostics, verifier assertions, direct/menu
  verifier wrappers, and promoted modes `71/72` to the Boundary/Latest
  profiles while keeping modes `69/70` in regression as the update-main floor
  proof.

What is proven:

- Direct proof:
  `Battle Mario/Fox stage MP sweep floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=1/36 same=35/1 diff=1 line=3->0 second=34/34 p0line=3 p1line=3`.
- Menu-chain proof:
  `Menu-chain Mario/Fox stage MP sweep floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=1/36 same=35/1 diff=1 line=3->0 second=34/34 p0line=3 p1line=3`.
- Profile proofs:
  `make -j16`, `check-harness-registry.ps1`, direct/menu-chain mpsweep
  verifiers, `verify-current.ps1`, and `verify-regression.ps1` passed when run
  sequentially.

Still deferred:

- This is still a bounded selected-fighter, floor-only map-collision proof, not
  full BattleShip map collision. The accepted different-floor path is proven by
  bounded standalone probes, not yet by the moving selected-fighter callback
  route. Floor-edge adjustment remains a bounded stub, and wall tests, ceiling
  tests, edge-under wall adjacency, platform pass-through, ledges/cliffcatch,
  Fall/Ottotto, moving yakumono collision, items, HUD, audio, arbitrary live
  gameplay, unpaused full-scene `gcRunAll`/`gcDrawAll`, and full imported
  `ftmain.c` remain deferred.

## 2026-06-24: Added source-order MP cross-floor / live second-floor proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpcross_floor_loop` harnesses as modes `73/74`.
- Built on the existing source-order `mpProcessUpdateMain` floor-loop and
  `mpsweep` proofs without widening the collision scope beyond the bounded
  floor-only `mpCommonRunFighterAllCollisions` slice.
- Added live selected-callback diagnostics for the second-floor branch,
  accepted-new-line counts, landing/floor-edge/collision-end bookkeeping, P0/P1
  final floor lines, and unsafe counters.
- Primed P0 with source floor line `-1` in the cross-floor modes so
  `mpProcessCheckTestFloorCollisionNew` projects against decoded Pupupu
  geometry before `mpProcessCheckTestFloorCollision` accepts real floor line
  `3` through the source-order second-floor branch.
- Kept P1 as a retained-floor control on line `3`, preserved the existing
  moving preview, `gcRunAll`, `gcDrawAll`, stage capture, MP floor-process, MP
  update-main, and MP sweep evidence, and promoted modes `73/74` to the
  Boundary/Latest verifier profiles.

What is proven:

- Direct proof:
  `Battle Mario/Fox Stage MP cross-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP cross-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3`.

Still deferred:

- This is still a bounded selected-fighter, floor-only map-collision proof, not
  full BattleShip map collision. Stale-valid-floor crossings, real floor-edge
  adjustment, wall tests, ceiling tests, edge-under wall adjacency, platform
  pass-through, ledges/cliffcatch, Fall/Ottotto, moving yakumono collision,
  items, HUD, audio, arbitrary live gameplay, unpaused full-scene
  `gcRunAll`/`gcDrawAll`, and full imported `ftmain.c` remain deferred.

## 2026-06-25: Added source-order MP floor-edge-adjust check proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpadjust_floor_loop` harnesses as modes `75/76`.
- Built on the existing source-order MP cross-floor proof without widening the
  bounded floor-only `mpCommonRunFighterAllCollisions` slice.
- Added source-order `mpProcessRunFloorEdgeAdjust`,
  `mpProcessCheckFloorEdgeCollisionL/R`,
  `mpCollisionCheckL/RWallLineCollisionSame`, and bounded wall helper
  diagnostics in project-owned compatibility code.
- Extended the shared gcDrawAll verifier with `STAGE_MPADJUST_FLOOR*` markers
  and promoted modes `75/76` to the Latest/Boundary verifier profiles.
- Kept P0 as the live second-floor/adjust branch and P1 as the retained-floor
  control verified through the existing MP update-main final floor state.

What is proven:

- Direct proof:
  `Battle Mario/Fox Stage MP adjust-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 check=17/17 wallMiss=17/17 edge=17/17 noAdjust=17 p0line=3 p1maintained=3`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP adjust-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 check=17/17 wallMiss=17/17 edge=17/17 noAdjust=17 p0line=3 p1maintained=3`.
- Profile proofs:
  `make -j16`, `check-harness-registry.ps1`, direct/menu-chain MP adjust
  verifiers, `verify-current.ps1`, and `verify-regression.ps1` passed when run
  sequentially.

Still deferred:

- This is still a bounded selected-fighter, floor-only map-collision proof, not
  full BattleShip map collision. Real wall-hit floor-edge adjustment,
  stale-valid-floor crossings, ceiling tests, edge-under wall adjacency,
  platform pass-through, ledges/cliffcatch, Fall/Ottotto, moving yakumono
  collision, items, HUD, audio, arbitrary live gameplay, unpaused full-scene
  `gcRunAll`/`gcDrawAll`, and full imported `ftmain.c` remain deferred.

## 2026-06-25: Added source-order MP edge-under / floor-edge proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpedge_floor_loop` harnesses as modes `77/78`.
- Built on the source-order MP floor-edge-adjust proof without widening the
  bounded floor-only `mpCommonRunFighterAllCollisions` slice.
- Replaced the previously deferred `mpCollisionGetEdgeUnderL/RLineID` lookup in
  the new proof modes with decoded Pupupu geometry adjacency from the selected
  floor line to adjacent wall lines.
- Added `STAGE_MPEDGE_FLOOR*` GDB/verifier markers and promoted modes `77/78`
  to the Boundary/Latest verifier profiles while keeping modes `75/76` in
  regression for the older explicit edge-under deferral behavior.

What is proven:

- Direct proof:
  `Battle Mario/Fox Stage MP edge-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 p0line=3 p1maintained=3 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP edge-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 p0line=3 p1maintained=3 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18`.
- Profile proofs:
  `make -j16`, `check-harness-registry.ps1`, direct/menu-chain MP edge-floor
  verifiers, `verify-current.ps1`, and `verify-regression.ps1` passed when run
  sequentially.

Still deferred:

- This is still a bounded selected-fighter, floor-only map-collision proof, not
  full BattleShip map collision. Real wall-hit floor-edge adjustment,
  stale-valid-floor crossings, ceiling tests, platform pass-through,
  ledges/cliffcatch, Fall/Ottotto, moving yakumono collision, items, HUD,
  audio, arbitrary live gameplay, unpaused full-scene `gcRunAll`/`gcDrawAll`,
  and full imported `ftmain.c` remain deferred.

## 2026-06-25: Added source-order MP wall-blocker proofs for the selected Dream Land floor

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpwall_floor_loop` harnesses as modes `79/80`.
- Built on the source-order MP edge-under/floor-edge proof without widening
  the bounded floor-only `mpCommonRunFighterAllCollisions` slice.
- Added a bounded source-order wall-candidate sweep that records the selected
  floor line, candidate wall line, wall kind, edge-under line, side, hit/miss
  counts, adjust-call count, and position deltas.
- Promoted modes `79/80` to the Boundary/Latest verifier profiles while
  keeping the earlier MP adjust and MP edge harnesses in regression.

What is proven:

- The current selected Dream Land/Pupupu main floor line `3` has adjacent
  edge-under wall lines `6/5` with kinds `3/2`.
- The direct and menu-chain MP wall-floor harnesses record two side-wall
  candidates, zero wall hits, nonzero miss evidence, zero adjust calls, zero
  position deltas, and stable Wait/Ground/Floor state.
- This identifies a precise renderer/playability blocker for the current
  geometry slice: original `mpProcessCheckFloorEdgeCollisionL/R` rejects wall
  candidates that are the same as the edge-under line, so this Dream Land main
  floor cannot prove the real wall-hit floor-edge-adjust branch.
- Profile proofs run in this milestone: `make -j16`, direct/menu-chain MP
  wall-floor verifiers, `check-harness-registry.ps1`, `verify-current.ps1`,
  and `verify-regression.ps1` passed.

Still deferred:

- A real wall-hit floor-edge adjustment proof now needs a different original
  stage/line/collision case with a non-edge wall candidate. Stale-valid-floor
  crossings, ceiling tests, platform pass-through, ledges/cliffcatch,
  Fall/Ottotto, moving yakumono collision, items, HUD, audio, arbitrary live
  gameplay, unpaused full-scene `gcRunAll`/`gcDrawAll`, and full imported
  `ftmain.c` remain deferred.

## 2026-06-25: Added source-order MP stale-valid second-floor proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpstale_floor_loop` harnesses as modes `81/82`.
- Built on the MP wall-blocker proof without widening the floor-only
  `mpCommonRunFighterAllCollisions` slice or moving the verified live
  wall/edge proof-owned roots.
- Added a finalizer-local source-order `MPCollData` probe that searches real
  decoded Dream Land floor pairs and selects a valid stale floor pair,
  line `1 -> 0` at `x=-285`, `y=1542`.
- Added `STAGE_MPSTALE_FLOOR*` GDB/verifier markers and promoted modes
  `81/82` to the Boundary/Latest verifier profiles.

What is proven:

- The maintained live selected-callback cross-floor path remains `-1 -> 3`.
- The stale proof reaches source-order `mpProcessUpdateMain ->
  mpCommonRunFighterAllCollisions -> mpProcessCheckTestFloorCollision`, accepts
  a new target floor line, calls `mpProcessSetLandingFloor`, reaches
  `mpProcessRunFloorEdgeAdjust`, and clears collision-end state.
- Direct proof:
  `Battle Mario/Fox Stage MP stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=18/0 accepted=1 p0line=0 p1line=3`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=18/0 accepted=1 p0line=0 p1line=3`.
- Profile proofs run in this milestone: `make -j16`, direct/menu-chain MP
  stale-floor verifiers, `check-harness-registry.ps1`, `verify-current.ps1`,
  and `verify-regression.ps1` passed. The first regression run used too short
  a timeout and was rerun successfully.

Still deferred:

- This is still a bounded selected-fighter, floor-only map-collision proof, not
  full BattleShip map collision. Real wall-hit floor-edge adjustment on a
  non-edge wall candidate, live selected-callback valid-stale crossing,
  ceiling tests, platform pass-through, ledges/cliffcatch, Fall/Ottotto,
  moving yakumono collision, items, HUD, audio, arbitrary live gameplay,
  unpaused full-scene `gcRunAll`/`gcDrawAll`, and full imported `ftmain.c`
  remain deferred.

## 2026-06-25: Added direct/menu-chain MP Fall-landing floor proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpfallland_floor_loop` harnesses as modes `93/94`.
- Built the proof on the existing Fall-map boundary: P1 starts in Fall/Air on
  decoded Pupupu floor line `3`, crosses the floor, enters the selected
  original map callback route, calls landing-floor setup, reaches original
  LandingLight status/motion `31/25`, switches to Ground, and clamps vertical
  velocity to zero.
- Added `STAGE_MPFALLLAND_FLOOR*` GDB/verifier markers and direct/menu-chain
  wrapper verifiers. The landing-param marker is treated as optional because
  the imported path can reach `ftCommonLandingSetStatus` and `ftMainSetStatus`
  directly without the public wrapper.
- Promoted modes `93/94` to the Boundary/Latest verifier profiles.

What is proven:

- Direct proof:
  `Battle Mario/Fox Stage MP Fall-landing-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->0`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP Fall-landing-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->0`.
- Proofs run in this milestone: `make -j16`,
  `check-harness-registry.ps1`, direct/menu-chain MP Fall-landing verifiers,
  `verify-boundary.ps1`, and `verify-current.ps1` passed.

Still deferred:

- This is still a bounded selected-fighter, floor-only map-collision proof, not
  full BattleShip map collision. Natural-motion cliff/offstage detection,
  ledges/cliffcatch, ceiling tests, platform pass-through, real wall-hit
  adjustment on another geometry case, moving yakumono collision, items, HUD,
  audio, arbitrary live gameplay, unpaused full-scene `gcRunAll`/`gcDrawAll`,
  and full imported `ftmain.c` remain deferred.

## 2026-06-25: Added selected-callback live-stale MP second-floor proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mplivestale_floor_loop` harnesses as modes `83/84`.
- Kept modes `81/82` as finalizer-local stale-valid regression proofs and
  promoted modes `83/84` to the Boundary/Latest verifier profiles.
- Reused the decoded Dream Land valid-stale floor pair `1 -> 0`, but triggered
  it from the selected P0 callback path with a contained local `MPCollData`
  source-order pass.
- Isolated the local probe/search from older edge-under diagnostics so the
  existing MP edge/wall/stale proof stack remains stable.
- Added `STAGE_MPLIVESTALE_FLOOR*` GDB/verifier markers.

What is proven:

- The selected-callback proof reaches source-order `mpProcessUpdateMain ->
  mpCommonRunFighterAllCollisions -> mpProcessCheckTestFloorCollision`,
  accepts target line `0`, calls `mpProcessSetLandingFloor`, reaches
  `mpProcessRunFloorEdgeAdjust`, and clears collision-end state.
- The real Mario/Fox movement loop remains on decoded floor line `3/3`; the
  valid-stale test is a contained collision proof, not a live root-position
  mutation.
- Direct proof:
  `Battle Mario/Fox Stage MP live-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=1/0 accepted=1 p0line=0 p1line=3 mpLiveStaleFloor=line=1->0 selected=1 live=1/0 accepted=1 p0line=0 p1line=3`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP live-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=1/0 accepted=1 p0line=0 p1line=3 mpLiveStaleFloor=line=1->0 selected=1 live=1/0 accepted=1 p0line=0 p1line=3`.
- Profile proofs run in this milestone: `make -j16`,
  `check-harness-registry.ps1`, direct/menu-chain MP live-stale-floor
  verifiers, `verify-current.ps1`, and `verify-regression.ps1` passed.

Still deferred:

- This is still a bounded selected-fighter, floor-only map-collision proof, not
  full BattleShip map collision. Arbitrary natural-motion valid-stale
  crossings, real wall-hit floor-edge adjustment on a non-edge wall candidate,
  ceiling tests, platform pass-through, ledges/cliffcatch, Fall/Ottotto,
  moving yakumono collision, items, HUD, audio, arbitrary live gameplay,
  unpaused full-scene `gcRunAll`/`gcDrawAll`, and full imported `ftmain.c`
  remain deferred.

## 2026-06-25: Added direct/menu-chain MP ceiling-hit StopCeil status proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpceilstatus_floor_loop` harnesses as modes `97/98`.
- Imported original `ftcommonstopceil.c` through
  `src/import/battleship_ftcommon_stopceil.c`.
- Kept the older ceiling-floor modes `95/96` as regression coverage and
  promoted the new ceiling-status modes to the Boundary/Latest verifier
  profiles.
- Added `STAGE_MPCEILSTATUS_FLOOR*` GDB/verifier markers for result, callback
  counts, status/motion/ground-air transition, velocity clamp, and collision
  masks.

What is proven:

- The selected P1 original `mpCommonProcFighterCliffFloorCeil` map callback
  routes through bounded source-order `mpProcessUpdateMain` and the
  ceiling-heavy collision/adjust path.
- The proof selects real Pupupu ceiling line `4`, records ceiling collision and
  adjust evidence, sets `MAP_FLAG_CEIL` / `MAP_FLAG_CEILHEAVY`, reaches
  original `ftCommonStopCeilSetStatus`, and changes Fall/Air `26/20/1` to
  StopCeil/Ground `66/57/0`.
- Direct proof:
  `Battle Mario/Fox Stage MP Ceiling-status floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCeilFloor=line=4 kind=1 check=2/2 diff=2/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP Ceiling-status floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCeilFloor=line=4 kind=1 check=2/2 diff=2/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400`.
- Proofs run in this milestone: `make -j16`, direct/menu-chain MP
  ceiling-status verifiers, `check-harness-registry.ps1`,
  `verify-current.ps1`, and `verify-regression.ps1` passed.

Still deferred:

- This is still a bounded selected-fighter map-collision/status proof, not full
  BattleShip map collision. Arbitrary natural-motion ceiling hits,
  cliffcatch/ledge behavior, platform pass-through, real wall-hit floor-edge
  adjustment on a non-edge wall candidate, moving yakumono collision, items,
  HUD, audio, arbitrary live gameplay, unpaused full-scene
  `gcRunAll`/`gcDrawAll`, and full imported `ftmain.c` remain deferred.

## 2026-06-25: Added direct/menu-chain MP ceiling-floor collision proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpceil_floor_loop` harnesses as modes `95/96`.
- Promoted modes `95/96` to the Boundary/Latest verifier profiles while
  keeping the Fall-landing modes `93/94` in regression.
- Added bounded project-owned ceiling sweep/query compatibility for
  `mpProcessCheckTestCeilCollisionAdjNew`,
  `mpCollisionCheckCeilLineCollisionSame/Diff`,
  `mpCollisionGetFCCommonCeil`, and
  `mpProcessRunCeilCollisionAdjNew`.
- Added `STAGE_MPCEIL_FLOOR*` GDB/verifier markers and direct/menu-chain
  wrapper scripts.

What is proven:

- The inherited Fall landing-floor proof remains intact through original
  LandingLight/Ground setup.
- The ceiling proof chooses real Pupupu ceiling line `4`, runs one bounded
  ceiling test hit, reaches the different-line ceiling sweep path, records two
  `mpCollisionGetFCCommonCeil` hits, runs one ceiling adjust call, and records
  current/stat ceiling mask evidence.
- Direct proof:
  `Battle Mario/Fox Stage MP Ceiling-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->0 mpCeilFloor=line=4 kind=1 check=1/1 diff=1/1 fc=2/2 y=-1464000->-1472000 ceil=-1072000 dist=-8000`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP Ceiling-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->0 mpCeilFloor=line=4 kind=1 check=1/1 diff=1/1 fc=2/2 y=-1464000->-1472000 ceil=-1072000 dist=-8000`.
- Proofs run in this milestone: `make -j16`, direct/menu-chain MP ceiling
  verifiers, and `check-harness-registry.ps1` passed so far; final handoff
  verification is recorded in `docs/STATUS.md` / `docs/HANDOFF.md`.

Still deferred:

- This is a bounded selected-fighter ceiling collision/adjust proof, not full
  BattleShip map collision. Natural-motion ceiling hits, ceil-heavy handling,
  platform pass-through, ledges/cliffcatch, real wall-hit floor-edge
  adjustment on a non-edge wall candidate, moving yakumono collision, items,
  HUD, audio, arbitrary live gameplay, unpaused full-scene
  `gcRunAll`/`gcDrawAll`, and full imported `ftmain.c` remain deferred.

## 2026-06-25: Added bounded Mario/Fox Fall physics/map callback proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpfallmap_floor_loop` harnesses as modes `91/92`.
- Promoted the new modes to the Latest/Boundary verifier profile while keeping
  the cliff-status and cliff-tick modes as regression coverage.
- Reused the P1 Fall state produced by the previous cliff-tick boundary, then
  ran the selected status-table original `ftPhysicsApplyAirVelDriftFastFall`
  callback and a bounded airborne integration step.
- Added guarded diagnostics around fast-fall, gravity, air-drift, air-friction,
  integration, and the selected original `mpCommonProcFighterCliffFloorCeil`
  map callback.
- Added `STAGE_MPFALLMAP_FLOOR*` GDB/verifier markers and direct/menu-chain
  verifier wrappers.

What is proven:

- P1 starts in original Fall status/motion `26/20` and Air state from the
  earlier cliff-status/tick proof.
- The selected Fall physics callback path is reached through the original
  status-table callback pointer, then records fast-fall, gravity, air-drift,
  and air-friction seams.
- One bounded airborne step decreases P1 root-Y from `200000` to `194000`.
- The selected original Fall map callback reaches the guarded
  `mpCommonProcFighterCliffFloorCeil` no-collision branch and leaves P1 in
  Fall/Air on floor line `3`.
- Direct proof:
  `Battle Mario/Fox Stage MP Fall-map-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP Fall-map-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000`.
- Proofs run in this milestone: `make -j16`, direct/menu-chain MP Fall-map
  verifiers, `check-harness-registry.ps1`, `verify-boundary.ps1`,
  `verify-current.ps1`, and `verify-regression.ps1` passed.

Still deferred:

- This is a bounded selected-fighter Fall physics/map callback proof, not full
  BattleShip map collision or gameplay. Real Fall landing, full
  `mpCommonCheckFighterCeilHeavyCliff`, ceiling hits, cliffcatch, ledges,
  platforms, real wall-hit floor-edge adjustment on a non-edge wall candidate,
  continuous Fall/Ottotto runtime, arbitrary live offstage movement, items,
  HUD, audio, unpaused full-scene `gcRunAll`/`gcDrawAll`, and full imported
  `ftmain.c` remain deferred.

## 2026-06-25: Added bounded Ottotto/Fall cliff-tick callback proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpclifftick_floor_loop` harnesses as modes `89/90`.
- Built on the existing source-order MP cliff-status proof instead of
  re-entering broad gameplay scheduling.
- Reused imported original `ftcommonottotto.c` and `ftcommonfall.c` callbacks
  through guarded project-owned proof wrappers.
- Added `STAGE_MPCLIFFTICK_FLOOR`, `STAGE_MPCLIFFTICK_FLOOR_CALLS`, and
  `STAGE_MPCLIFFTICK_FLOOR_STATUS` GDB/verifier markers.
- Promoted the new direct/menu-chain harness pair to the Boundary/Latest
  verifier profiles and registry drift check.

What is proven:

- The inherited moving/floor proof stack remains intact through the
  motion-stale copyback path and the cliff-status Ottotto/Fall setup path.
- P0 starts in original Ottotto status/motion `36/30`, runs one guarded
  original `ftCommonOttottoProcUpdate`,
  `ftCommonOttottoProcInterrupt`, and `ftCommonOttottoProcMap` tick, reaches
  the bounded floor check/hit seam, and remains Ottotto/Ground on Dream Land
  floor line `0`.
- P1 starts in original Fall status/motion `26/20`, runs one guarded original
  `ftCommonFallProcInterrupt` tick, reaches the guarded original
  special-air, attack-air, and jump-aerial interrupt checks, and remains
  Fall/Air on Dream Land floor line `3`.
- Direct proof:
  `Battle Mario/Fox Stage MP cliff-tick-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP cliff-tick-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3`.
- Proofs run in this milestone: `make -j16`, direct/menu-chain MP
  cliff-tick verifiers, `check-harness-registry.ps1`,
  `verify-boundary.ps1`, `verify-current.ps1`, and `verify-regression.ps1`
  passed.

Still deferred:

- This is still a bounded selected-fighter, floor-only map-collision proof, not
  full BattleShip map collision. Natural-motion cliff/offstage detection,
  continuous Ottotto/Fall runtime, real wall-hit floor-edge adjustment on a
  non-edge wall candidate, ceiling tests, platform pass-through,
  ledges/cliffcatch, moving yakumono collision, items, HUD, audio, arbitrary
  live gameplay, unpaused full-scene `gcRunAll`/`gcDrawAll`, and full imported
  `ftmain.c` remain deferred.

## 2026-06-25: Added source-order MP cliff-status branch proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpcliffstatus_floor_loop` harnesses as modes `87/88`.
- Kept modes `85/86` as selected-callback/root motion-stale regression proofs
  and promoted modes `87/88` to the Boundary/Latest verifier profiles.
- Imported original `ftcommonottotto.c` through
  `src/import/battleship_ftcommon_ottotto.c`.
- Added a bounded proof-only `mpCommonProcFighterOnCliffEdge` status branch
  that preserves source order: check cliff edge, branch to Ottotto when
  `MAP_FLAG_FLOOREDGE` is set, otherwise branch to Fall.
- Added `STAGE_MPCLIFFSTATUS_FLOOR*` GDB/verifier markers.
- Fixed the menu-chain wrapper to assert the menu-chain harness selection
  scene pair `9/1` while still proving the final VSBattle boundary `22/21`.

What is proven:

- The inherited moving/floor proof stack remains intact through the
  motion-stale copyback path from Dream Land line `1 -> 0`.
- The bounded source-order cliff-status probes call
  `mpCommonProcFighterOnCliffEdge` twice, record two false
  `mpCommonCheckFighterOnCliffEdge` results, and take one Ottotto branch plus
  one Fall branch.
- P0 with `MAP_FLAG_FLOOREDGE` reaches original Ottotto status/motion
  `36/30`; P1 without that flag reaches original Fall status/motion `26/20`
  and air state.
- Direct proof:
  `Battle Mario/Fox Stage MP cliff-status-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP cliff-status-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3`.
- Proofs run in this milestone: `make -j16`, direct/menu-chain MP
  cliff-status verifiers, `check-harness-registry.ps1`,
  `verify-boundary.ps1`, `verify-current.ps1`, and `verify-regression.ps1`
  passed.

Still deferred:

- This is still a bounded selected-fighter, floor-only map-collision proof, not
  full BattleShip map collision. Natural-motion cliff/offstage detection,
  continuous Ottotto/Fall runtime, real wall-hit floor-edge adjustment on a
  non-edge wall candidate, ceiling tests, platform pass-through,
  ledges/cliffcatch, moving yakumono collision, items, HUD, audio, arbitrary
  live gameplay, unpaused full-scene `gcRunAll`/`gcDrawAll`, and full imported
  `ftmain.c` remain deferred.

## 2026-06-25: Added selected-callback/root motion-stale MP second-floor proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpmotionstale_floor_loop` harnesses as modes `85/86`.
- Kept modes `83/84` as selected-callback local live-stale regression proofs
  and promoted modes `85/86` to the Boundary/Latest verifier profiles.
- Reused the decoded Dream Land valid-stale floor pair `1 -> 0`, but this time
  seeded it into the selected P0 root and live `FTStruct.coll_data` shell before
  the selected map callback ran.
- Added `STAGE_MPMOTIONSTALE_FLOOR*` GDB/verifier markers.
- Scoped the new verifier to stage draw plus the motion-stale evidence because
  the intentional P0 mutation invalidates older standalone stage-collision
  finalizer assumptions inside this integrated harness. The older direct
  verifiers still cover those standalone boundaries.

What is proven:

- The selected P0 callback reaches source-order `mpProcessUpdateMain ->
  mpCommonRunFighterAllCollisions -> mpProcessCheckTestFloorCollision`,
  accepts target line `0`, calls `mpProcessSetLandingFloor`, reaches
  `mpProcessRunFloorEdgeAdjust`, clears collision-end state, and copies the
  target floor back to the live P0 root/collision state.
- P1 remains a grounded control on decoded line `3`.
- Direct proof:
  `Battle Mario/Fox Stage MP motion-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP motion-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000`.
- Profile proofs run in this milestone: `make -j16`,
  `check-harness-registry.ps1`, direct/menu-chain MP motion-stale-floor
  verifiers, `verify-current.ps1`, and `verify-regression.ps1` passed.

Still deferred:

- This is still a bounded selected-fighter, floor-only map-collision proof, not
  full BattleShip map collision. Arbitrary natural-motion valid-stale
  crossings, real wall-hit floor-edge adjustment on a non-edge wall candidate,
  ceiling tests, platform pass-through, ledges/cliffcatch, Fall/Ottotto,
  moving yakumono collision, items, HUD, audio, arbitrary live gameplay,
  unpaused full-scene `gcRunAll`/`gcDrawAll`, and full imported `ftmain.c`
  remain deferred.

Latest note:

- The active boundary has since advanced to direct/menu-chain
  `battle_mariofox_stage_mpceilstatus_floor_loop` modes `97/98`, importing
  original `ftcommonstopceil.c` and proving the selected ceiling-hit
  `mpCommonProcFighterCliffFloorCeil` path reaches original StopCeil status.
  See the `2026-06-25: Added direct/menu-chain MP ceiling-hit StopCeil status
  proofs` entry above for the full proof details.

## 2026-06-25: Added direct/menu-chain MP cliff-catch status proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpcliffcatch_floor_loop` harnesses as modes `99/100`.
- Imported original `ftcommoncliffcatchwait.c` through a project-owned wrapper.
- Added the narrow fighter/map/effect ABI and DS-owned compatibility stubs
  needed by `ftCommonCliffCatchSetStatus` without importing broad ledge or
  fighter runtime.
- Extended the shared Mario/Fox stage MP verifier with
  `STAGE_MPCLIFFCATCH_FLOOR*` markers and promoted modes `99/100` to the
  Boundary/Latest profiles.

What is proven:

- The selected P1 map callback reaches bounded source-order
  `mpProcessUpdateMain -> mpCommonProcFighterCliffFloorCeil`.
- The proof runs source-order left/right cliff checks, records a left miss and
  right hit, selects real Pupupu line `3`, and reaches original
  `ftCommonCliffCatchSetStatus`.
- P1 transitions from Fall/Air `26/20/1` to CliffCatch/Air `84/72/1`, sets
  cliff hold, copies `cliff_id=3`, records LR `-1`, moves the root to the
  ledge X, and records `MAP_FLAG_RCLIFF` masks `0x2000/0x2000`.
- Direct proof:
  `Battle Mario/Fox Stage MP Cliff-catch floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCeilFloor=line=4 kind=1 check=3/2 diff=3/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP Cliff-catch floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCeilFloor=line=4 kind=1 check=3/2 diff=3/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000`.
- Proofs run in this milestone: `make -j16`,
  `check-harness-registry.ps1`, direct/menu-chain MP cliff-catch verifiers,
  `verify-current.ps1`, and `verify-regression.ps1` passed.

Still deferred:

- This is still a bounded selected-fighter map-collision/status proof, not full
  BattleShip ledge gameplay. Natural-motion cliffcatch, CliffWait/climb/escape/
  drop, ledge occupancy, real wall-hit floor-edge adjustment on a non-edge wall
  candidate, platform pass-through, continuous Fall/Ottotto/Cliff runtime,
  moving yakumono collision, items, HUD, audio, arbitrary live gameplay,
  unpaused full-scene `gcRunAll`/`gcDrawAll`, and full imported `ftmain.c`
  remain deferred.

## 2026-06-25: Added direct/menu-chain MP CliffCatch -> CliffWait proofs

What changed:

- Added direct and menu-chain
  `battle_mariofox_stage_mpcliffwait_floor_loop` harnesses as modes `101/102`.
- Reused the existing imported original `ftcommoncliffcatchwait.c` wrapper
  instead of adding a new gameplay rewrite.
- Added a narrow CliffWait-only `ftMainSetStatus` compatibility branch,
  original `ftAnimEndCheckSetStatus` instrumentation, and guarded CliffWait
  interrupt/check stubs in the DS-owned port layer.
- Extended the shared Mario/Fox stage MP verifier with
  `STAGE_MPCLIFFWAIT_FLOOR*` markers and promoted modes `101/102` to the
  Boundary/Latest profiles.

What is proven:

- The proof starts from the verified CliffCatch state on real Pupupu line `3`,
  calls original `ftCommonCliffCatchProcUpdate`, reaches original
  `ftAnimEndCheckSetStatus`, and enters original `ftCommonCliffWaitSetStatus`.
- P1 transitions from CliffCatch/Air `84/72/1` to CliffWait/Ground `85/73/0`,
  retains `cliff_id=3` and LR `-1`, sets cliff hold, player-tag wait `120`,
  capture immunity, and proc-damage state.
- One guarded original `ftCommonCliffWaitProcInterrupt` tick runs with
  attack/escape/climb-or-fall checks returning false, decrements fall-wait
  `1080 -> 1079`, and does not call the damage-fall path.
- Direct proof:
  `Battle Mario/Fox Stage MP Cliff-wait floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->0 mpCeilFloor=line=4 kind=1 check=3/2 diff=3/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000 mpCliffWait=status=84->85->85 motion=72->73->73 fallWait=1080->1079 cliff=3 guards=1/1/1`.
- Menu-chain proof:
  `Menu-chain Mario/Fox Stage MP Cliff-wait floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->0 mpCeilFloor=line=4 kind=1 check=3/2 diff=3/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000 mpCliffWait=status=84->85->85 motion=72->73->73 fallWait=1080->1079 cliff=3 guards=1/1/1`.
- Proofs run in this milestone: clean normal `make -j16`,
  `verify-runtime.ps1`, direct/menu-chain MP CliffWait verifiers,
  `check-harness-registry.ps1`, `verify-boundary.ps1`,
  `verify-current.ps1`, and `verify-regression.ps1` passed.

Still deferred:

- This is still a bounded selected-fighter ledge-status proof, not full
  BattleShip ledge gameplay. Natural-motion cliffcatch, ledge occupancy,
  ledge release/drop/climb/escape/attack, damage-fall timeout, real wall-hit
  floor-edge adjustment on a non-edge wall candidate, platform pass-through,
  continuous Fall/Ottotto/Cliff runtime, moving yakumono collision, items, HUD,
  audio, arbitrary live gameplay, unpaused full-scene `gcRunAll`/`gcDrawAll`,
  and full imported `ftmain.c` remain deferred.

## 2026-06-25: Fixed F3DEX2 GBI decode helpers for DS renderer paths

What changed:

- Confirmed BattleShip builds the original game source with `-DF3DEX_GBI_2`
  and checked the original `PR/gbi.h` command packing before touching project
  renderer code.
- Added shared F3DEX2 decode helpers in `include/nds/nds_gbi_decode.h` for
  `G_VTX`, `G_TRI1`, and `G_TRI2`.
- Replaced duplicated ad hoc VTX/TRI1/TRI2 decode logic in the DS renderer,
  Opening Room preview path, and Mario/Fox fighter DL execute/draw paths.
- Added `scripts/check-gbi-decode-fixtures.ps1` with fixtures for
  `gSPVertex(v, 4, 12)`, `gSP1Triangle(1, 2, 3, 0)`, and
  `gSP2Triangles(4, 5, 6, 0, 7, 8, 9, 0)`, plus source-snippet checks for the
  old decode shape.
- Updated the bounded software draw verifiers so usable projection bounds plus
  bounded degenerate-triangle marker drawing are accepted when projected area
  collapses to zero.

What is proven:

- F3DEX2 VTX now decodes count/end fields instead of the older count/v0 byte
  layout.
- F3DEX2 TRI1 now reads the packed triangle payload from `w0`, matching TRI2's
  first packed triangle, instead of reading `w1`.
- Packed F3DEX2 triangle vertex bytes are decoded as `index * 10`.
- Corrected first-DL proof:
  `Battle Mario/Fox DL draw harness passed: scene=22/21 pixels=6170/680 tris=37/20 preview=96x72 commit=1 safe=1`.
- Corrected multi-DL proof:
  `Battle Mario/Fox multi-DL draw harness passed: scene=22/21 candidates=14/18 selected=4/4 pixels=7258/2686 tris=87/79 clean=4/4 preview=96x72 safe=1`.
- Corrected all-DL proof:
  `Battle Mario/Fox all-DL draw harness passed: scene=22/21 callbacks=2 candidates=14/18 selected=14/18 pixels=18406/14789 tris=334/322 clean=14/17 failed=0/1 fail_reason=0x20 fail_selected=17 preview=96x72 safe=1`.
- Menu-chain first/multi/all-DL proofs report the same corrected counts after
  VS Mode -> PlayersVS -> Maps -> VSBattle.

Proofs run in this milestone:

- `make -j16`
- `scripts/check-gbi-decode-fixtures.ps1`
- `scripts/verify-runtime.ps1`
- `scripts/verify-opening-skip.ps1`
- direct and menu-chain Mario/Fox DL execute, first-DL draw, multi-DL draw,
  all-DL draw, and `gcDrawAll` loop verifiers

Still deferred:

- This fix corrects command decoding and bounded software-preview evidence; it
  does not implement the final DS hardware polygon renderer, camera-correct
  fighter projection, real fighter matrix prep, material/texture sampling,
  shadows, interface rendering, items, audio, or unbounded full-scene draw.
- One Fox all-DL selected DObj remains explicit as a non-clean/non-drawing
  selected DObj under corrected F3DEX2 decode (`failed=0/1`,
  `fail_reason=0x20`, selected `17`).

## 2026-06-25: Added all-DL first-failure diagnostics

What changed:

- Added maintained first-failure diagnostics for the guarded Mario/Fox all-DL
  draw proof:
  `gNdsFighterDLAllDrawP0FirstFailed*` and
  `gNdsFighterDLAllDrawP1FirstFailed*`.
- Added `FTR_DL_ALL_FAIL` to the direct and menu-chain all-DL verifier marker
  sets.
- Tightened both all-DL verifiers so the one allowed Fox miss is no longer an
  opaque `failed=0/1` allowance. It must be selected DObj `17`, reason `0x20`
  (`NO_VALID_TRIS`), with no renderer blocker, unsupported opcode/command, or
  vertex-range reject.

What is proven:

- The corrected F3DEX2 decode path does not leave a hidden renderer blocker in
  the guarded all-DL proof.
- Mario remains fully clean for the current selected all-DL draw boundary.
- Fox has one explicit selected DObj that decodes through the renderer path but
  contributes no valid triangles to the bounded software preview.

Proofs run in this milestone:

- `make -j16`
- `scripts/verify-battle-mariofox-dl-draw-all-harness.ps1`
- `scripts/verify-menu-chain-mariofox-dl-draw-all-harness.ps1`

Still deferred:

- This diagnostic does not make the no-valid-triangle Fox DObj render. The next
  renderer step still needs source-guided fighter matrix/camera/material work,
  not a broad renderer import or DS-native hand rendering.

## 2026-06-25: Added direct/menu-chain MP CliffWait -> CliffQuick proofs

What changed:

- Added direct and menu-chain stage MP cliff-attack harness modes:
  `battle_mariofox_stage_mpcliffattack_floor_loop` and
  `menu_chain_mariofox_stage_mpcliffattack_floor_loop` as modes `103/104`.
- Imported bounded original cliff action helpers through project-owned wrappers:
  `ftcommoncliffattack.c`, `ftcommoncliffclimb.c`, and
  `ftcommoncliffescape.c`.
- Extended the CliffWait proof finalizer with a guarded A-button interrupt
  probe that calls original `ftCommonCliffWaitProcInterrupt` and reaches
  original `ftCommonCliffAttackCheckInterruptCommon`.
- Added `STAGE_MPCLIFFATTACK_FLOOR*` diagnostics and promoted modes `103/104`
  to the Boundary/Latest verifier profiles.

What is proven:

- The proof starts from the existing real Pupupu right-ledge CliffWait state on
  `cliff_id=3`.
- An A-button tap `0x8000` reaches the original CliffAttack interrupt path.
- The guarded status seam accepts the original CliffQuick request and records
  CliffWait/Ground `85/73/0 ->` CliffQuick/Ground `86/74/0`.
- Queued cliff-motion metadata records AttackQuick on the retained cliff ID.
- Escape and climb/drop checks stay out of this bounded attack proof, and no
  damage-fall path runs.

Proofs run in this milestone:

- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffattack-floor-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffattack-floor-loop-harness.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-all.ps1 -Profile Latest`

Still deferred:

- CliffQuick attack animation/action callbacks, ledge occupancy, ledge
  release/drop/climb/escape behavior, damage-fall timeout, arbitrary
  natural-motion ledge transitions, platform pass-through, full map collision,
  gameplay, HUD, audio, and unbounded taskman scheduling remain deferred.

## 2026-06-25: Added direct/menu-chain CliffQuick -> CliffAttackQuick1 -> CliffAttackQuick2 proofs

What changed:

- Added direct and menu-chain stage MP cliff-attack action harness modes:
  `battle_mariofox_stage_mpcliffattack_action_loop` and
  `menu_chain_mariofox_stage_mpcliffattack_action_loop` as modes `105/106`.
- Extended the bounded cliff action imports so the proof can call original
  `ftCommonCliffQuickProcUpdate` and original
  `ftCommonCliffAttackQuick1ProcUpdate`, then reach the guarded original
  `ftAnimEndCheckSetStatus` branch into `ftCommonCliffAttackQuick2SetStatus`.
- Wrapped original `ftCommonCliffCommon2UpdateCollData` and
  `ftCommonCliffCommon2InitStatusVars` so the verifier can prove the common2
  setup helpers were reached without editing `decomp/`.
- Added `STAGE_MPCLIFFATTACK_ACTION*` diagnostics, direct/menu-chain verifier
  wrappers, and promoted modes `105/106` to the Boundary/Latest profiles while
  keeping modes `103/104` as regression coverage.

What is proven:

- The proof starts from the existing real Pupupu right-ledge CliffAttack setup
  state on retained `cliff_id=3`.
- Original CliffQuick update consumes queued AttackQuick metadata and reaches
  CliffAttackQuick1/Ground `92/80/0`.
- Original CliffAttackQuick1 update reaches the anim-end status branch and
  transitions to CliffAttackQuick2/Ground `93/81/0`.
- The proof records one call each through the Quick update, Quick1 set status,
  Quick1 update, anim-end branch, Quick2 set status, and common2 helper
  wrappers, with unsafe count `0`.

Proofs run in this milestone:

- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffattack-action-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffattack-action-loop-harness.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-regression.ps1`

Still deferred:

- The imported common2 collision-data helper currently sees the project-owned
  fighter collision shell through an original `MPCollData *` cast. The helper
  is reached, but `floor_line_id` remains `-1`; align the `FTCollisionData`
  shim or add a narrow conversion bridge before considering common2 floor
  copyback complete.
- Hitboxes/damage, ledge occupancy, ledge release/drop/climb/escape behavior,
  damage-fall timeout, arbitrary natural-motion ledge transitions, platform
  pass-through, full map collision, gameplay, HUD, audio, and unbounded taskman
  scheduling remain deferred.

## 2026-06-25: Fixed F3DEX2 packed triangle index decode and strict all-DL proof

What changed:

- Corrected the shared `nds_gbi_decode.h` packed triangle-index helper from the
  older `/10` decode shape to BattleShip's F3DEX2 `*2` / `/2` command packing.
- Updated the GBI fixture script to build and decode F3DEX2 `gSP1Triangle` and
  `gSP2Triangles` command words with the same `*2` packing used by
  `decomp/BattleShip-main/decomp/include/PR/gbi.h`.
- Tightened the guarded Mario/Fox all-DL pass condition and direct/menu-chain
  verifiers. The all-DL boundary now requires all 14 Mario and all 18 Fox
  selected DObjs to be clean/drawn, with zero failed DObjs and fully clear
  `FTR_DL_ALL_FAIL` diagnostics.
- Updated current docs to remove the previous Fox selected-DObj
  `NO_VALID_TRIS` miss from active status. Historical PORTING entries above are
  left intact as the prior state.

What is proven:

- First-DL draw remains bounded and now reports `4274/5345` pixels with
  `37/20` represented triangles for Mario/Fox.
- Multi-DL draw remains bounded and now reports `6190/7026` pixels with
  `87/79` represented triangles for the first four DL-ready DObjs per fighter.
- All-DL draw now reports `14913/13432` pixels, `334/322` represented
  triangles, `clean=14/18`, and `failed=0/0` for both direct and menu-chain
  paths.
- Latest profile remains green after the decode fix.

Proofs run in this milestone:

- `make -j16`
- `scripts/check-gbi-decode-fixtures.ps1`
- `scripts/verify-battle-mariofox-dl-execute-harness.ps1`
- `scripts/verify-battle-mariofox-dl-draw-harness.ps1`
- `scripts/verify-battle-mariofox-dl-draw-multi-harness.ps1`
- `scripts/verify-battle-mariofox-dl-draw-all-harness.ps1`
- `scripts/verify-menu-chain-mariofox-dl-execute-harness.ps1`
- `scripts/verify-menu-chain-mariofox-dl-draw-harness.ps1`
- `scripts/verify-menu-chain-mariofox-dl-draw-multi-harness.ps1`
- `scripts/verify-menu-chain-mariofox-dl-draw-all-harness.ps1`
- `scripts/verify-current.ps1`

Still deferred:

- The bounded software renderer still uses collapsed-triangle marker fallback
  for some projected fighter triangles until the real fighter matrix/camera
  projection path is imported.
- Texture/material fidelity, full display-list coverage, hardware 3D
  replacement, unbounded draw traversal, gameplay, HUD, audio, and full battle
  scheduling remain deferred.

## 2026-06-25: Bridged CliffAttackQuick2 common2 floor copyback

What changed:

- Added a narrow project-owned bridge for the bounded
  CliffAttackQuick2 action proof so imported original
  `ftCommonCliffCommon2UpdateCollData` can run against a temporary original
  `MPCollData` view while copying the resulting ledge floor fields back into
  the live port-owned `FTCollisionData` shell.
- Kept the bridge guarded to the direct/menu-chain
  `stage_mpcliffattack_action_loop` proof path; the broader fighter collision
  shell layout was not reshuffled.
- Tightened the ROM-side proof mask and shared verifier so the action proof now
  requires the copied floor line to match the retained cliff line.
- Updated current docs to move the next boundary past common2 floor copyback
  and toward one bounded original common2 update/physics/map tick from the
  created CliffAttackQuick2/Ground state.

What is proven:

- Direct and menu-chain CliffAttack action harnesses now report
  `mpCliffAttackAction=status=86->92->93 motion=74->80->81 cliff=3 floor=3
  calls=1/1/1`.
- The original common2 collision-data update helper is still called once, and
  the proof no longer accepts `floor_line_id=-1`.

Proofs run in this milestone:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffattack-action-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffattack-action-loop-harness.ps1`
- `scripts/verify-current.ps1`

Still deferred:

- The created CliffAttackQuick2 state is not yet consumed through a bounded
  original common2 update/physics/map tick.
- Natural ledge occupancy, ledge release/drop/climb/escape, hitboxes/damage,
  damage-fall timeout, platform pass-through, continuous Cliff runtime, and
  full gameplay scheduling remain deferred.

## 2026-06-25: Added bounded CliffAttackQuick2 common2 update/physics/map tick proofs

What changed:

- Added direct and menu-chain dev harness modes
  `battle_mariofox_stage_mpcliffcommon2_loop` and
  `menu_chain_mariofox_stage_mpcliffcommon2_loop` as modes `107/108`.
- Extended the existing CliffAttack action proof so the created
  CliffAttackQuick2/Ground state is consumed through one bounded original
  `ftCommonCliffCommon2ProcUpdate`, `ftCommonCliffCommon2ProcPhysics`, and
  `ftCommonCliffAttackEscape2ProcMap` tick.
- Added guarded diagnostics around the common2 anim-end check, ground velocity
  transfer, and edge-break map seam while keeping full CliffAttack runtime
  parked.
- Updated the harness registry, Latest/Boundary profile targets, verifier
  wrappers, and current docs to start the next task from the common2 boundary.

What is proven:

- Direct and menu-chain CliffCommon2 harnesses now report
  `mpCliffCommon2=status=93->93->93->93 motion=81->81->81->81 cliff=3
  floor=3->3 calls=1/1/1`.
- The proof starts from the existing real Pupupu right-ledge
  CliffAttackQuick2 state, preserves Ground state, keeps the retained
  `cliff_id=3` and copied `floor_line_id=3`, reaches the original common2
  update, physics, and map callbacks once each, and prevents fallback into
  unbounded Wait/Fall or edge-break behavior.

Proofs run in this milestone:

- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffcommon2-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffcommon2-loop-harness.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`

Still deferred:

- Natural ledge occupancy, ledge release/drop/climb/escape, attack hitboxes,
  damage-fall timeout, platform pass-through, continuous CliffAttack runtime,
  broad map collision, full fighter display, HUD, audio, and full gameplay
  scheduling remain deferred.

## 2026-06-25: Added bounded CliffWait -> CliffEscapeQuick action proofs

What changed:

- Added direct and menu-chain dev harness modes
  `battle_mariofox_stage_mpcliffescape_action_loop` and
  `menu_chain_mariofox_stage_mpcliffescape_action_loop` as modes `109/110`.
- Reused the bounded CliffWait state on the real Pupupu right ledge, injected a
  Z-button tap, and routed through original cliff interrupt helpers to prove
  EscapeQuick selection without opening continuous gameplay.
- Imported the narrow original `ftcommoncliffescape.c` surface through the
  project-owned import/wrapper path and kept all runtime guards in `src/port`.
- Added verifier assertions and diagnostic references for the CliffEscape
  action markers, including button-source checks that distinguish Z from the
  existing A-button CliffAttack path.
- Updated the harness registry, Latest/Boundary profile targets, verifier
  wrappers, and current docs to start the next task from the CliffEscape
  action boundary.

What is proven:

- Direct and menu-chain CliffEscape action harnesses now report
  `mpCliffEscapeAction=status=85->86->96->97 motion=73->74->84->85 cliff=3
  floor=3 calls=1/1/1`.
- The proof starts from CliffWait/Ground `85/73/0`, reaches CliffQuick/Ground
  `86/74/0` with queued EscapeQuick metadata, then reaches
  CliffEscapeQuick1/Ground `96/84/0` and CliffEscapeQuick2/Ground `97/85/0`.
- The retained real Pupupu `cliff_id=3` and copied `floor_line_id=3` survive
  the bounded path, while damage-fall, climb/fall fallback, and unsafe guards
  remain at zero.

Proofs run in this milestone:

- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffescape-action-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffescape-action-loop-harness.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-regression.ps1`

Still deferred:

- EscapeQuick2 common2 update/physics/map ticks, natural ledge occupancy,
  ledge release/drop/climb behavior, hitboxes/damage, damage-fall timeout,
  platform pass-through, continuous Cliff runtime, broad map collision, full
  fighter display, HUD, audio, and full gameplay scheduling remain deferred.

## 2026-06-25: Split real fighter DL geometry proof from degenerate marker pixels

What changed:

- Added maintained `RealTriangleDrawnCount` and `MarkerTriangleDrawnCount`
  diagnostics for the first-DL, multi-DL, and all-DL Mario/Fox software
  preview paths.
- Kept existing `TriangleDrawnCount` as total visible software output for
  compatibility, but tightened the renderer proof masks to require real
  non-degenerate projected triangles.
- Updated direct and menu-chain DL draw, multi-DL draw, and all-DL draw
  verifiers with `FTR_DL_*_RENDER` marker lines so marker fallback pixels are
  visible diagnostics only.
- Updated `docs/DIAGNOSTIC_REFERENCE.md`, `docs/STATUS.md`, and
  `docs/HANDOFF.md` with the new proof meaning.

What is proven:

- First-DL draw now reports `real=37/18` and `marker=0/2` while preserving
  `4274/5345` pixels and `37/20` represented triangles.
- Multi-DL draw now reports `real=82/76` and `marker=1/3` while preserving
  `6190/7026` pixels, `87/79` represented triangles, and clean `4/4` selected
  DObjs.
- All-DL draw now reports `real=306/290` and `marker=10/24` while preserving
  `14913/13432` pixels, `334/322` represented triangles, clean `14/18`
  selected DObjs, and `failed=0/0`.
- The prior F3DEX2 `/10` packed-triangle concern is not current: the maintained
  helper and fixture use BattleShip's F3DEX2 `*2` / `/2` packing, and the GBI
  fixture check passes.

Proofs run in this milestone:

- `make -j16`
- `scripts/check-gbi-decode-fixtures.ps1`
- `scripts/verify-battle-mariofox-dl-draw-harness.ps1`
- `scripts/verify-battle-mariofox-dl-draw-multi-harness.ps1`
- `scripts/verify-battle-mariofox-dl-draw-all-harness.ps1`
- `scripts/verify-menu-chain-mariofox-dl-draw-harness.ps1`
- `scripts/verify-menu-chain-mariofox-dl-draw-multi-harness.ps1`
- `scripts/verify-menu-chain-mariofox-dl-draw-all-harness.ps1`

Still deferred:

- Real fighter matrix/camera projection, material/texture upload and sampling,
  DS hardware polygon submission, full `ftdisplaymain.c` fidelity, continuous
  unguarded `gcDrawAll`, and full gameplay rendering remain deferred.

## 2026-06-25: Added bounded CliffWait climb/fall interrupt proofs

What changed:

- Added direct and menu-chain dev harness modes
  `battle_mariofox_stage_mpcliffclimb_floor_loop` and
  `menu_chain_mariofox_stage_mpcliffclimb_floor_loop` as modes `113/114`.
- Reused the verified CliffWait/Ground ledge state on real Pupupu
  `cliff_id=3`, seeded deterministic climb/drop stick inputs, and routed
  through original `ftCommonCliffWaitProcInterrupt` into original
  `ftCommonCliffClimbOrFallCheckInterruptCommon`.
- Kept the proof bounded: attack and escape checks are reached and rejected
  for the seeded inputs, only the climb/fall branch is accepted, and no
  continuous ledge action runtime is opened.
- Added direct/menu-chain verifier wrappers, shared verifier assertions,
  harness-registry entries, diagnostic marker docs, and current-truth doc
  updates for the new active boundary.

What is proven:

- Direct and menu-chain CliffClimb harnesses now report
  `mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3
  sticks=0,80/80,0 calls=2/2`.
- The climb branch reaches CliffQuick/Ground `86/74/0` with queued climb
  metadata and retained `cliff_id=3`.
- The drop branch reaches Fall/Air `26/20/1` with `cliffcatch_wait=30`,
  retained cliff metadata, proc callbacks set, and zero damage-fall or unsafe
  fallback.

Proofs run in this milestone:

- `scripts/check-gbi-decode-fixtures.ps1`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-regression.ps1`

Still deferred:

- Natural ledge occupancy, continuous ledge climb/drop action runtime, ledge
  release rules, ledge attack/escape hitboxes, damage-fall timeout, platform
  pass-through, broad map collision, full fighter display, HUD, audio, and
  full gameplay scheduling remain deferred.

## 2026-06-25: Added bounded CliffQuick -> CliffClimbQuick1 -> CliffClimbQuick2 proofs

What changed:

- Added direct and menu-chain dev harness modes
  `battle_mariofox_stage_mpcliffclimb_action_loop` and
  `menu_chain_mariofox_stage_mpcliffclimb_action_loop` as modes `115/116`.
- Reused the verified CliffClimb floor proof, consumed the climb-created
  CliffQuick/Ground state, and routed through original
  `ftCommonCliffQuickProcUpdate`, original
  `ftCommonCliffClimbQuick1SetStatus`, original
  `ftCommonCliffClimbQuick1ProcUpdate`, and the guarded anim-end path for
  `ftCommonCliffClimbQuick2SetStatus`.
- Kept `decomp/` read-only by importing `ftcommoncliffclimb.c` through
  `src/import/battleship_ftcommon_cliffclimb.c` and keeping its common2
  helpers isolated as `ndsBase*` symbols. The bounded animation-end seam calls
  the source-order Quick2 setup through project-owned common2 wrappers so the
  live `FTCollisionData` shell receives copied ledge-floor fields safely.
- Promoted the new action harness pair to Latest/Boundary profiles and kept
  modes `113/114` as regression coverage for climb/drop interrupt selection.

What is proven:

- Direct and menu-chain CliffClimb action harnesses now report
  `mpCliffClimbAction=status=86->87->88 motion=74->75->76 cliff=3 floor=3
  calls=1/1/1`.
- The action path reaches CliffClimbQuick1/Ground `87/75/0` and
  CliffClimbQuick2/Ground `88/76/0` from CliffQuick/Ground `86/74/0`.
- The proof retains real Pupupu `cliff_id=3`, copies `floor_line_id=3`,
  reaches one guarded common2 collision-data update, one guarded common2
  init-vars call, and records zero unsafe fallback.

Proofs run in this milestone:

- `make clean`
- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-action-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-action-loop-harness.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-regression.ps1`

Still deferred:

- Continuous ledge climb/drop runtime, ledge occupancy/release rules, damage
  fall timeout, ledge attack/escape hitboxes, broad map collision, full fighter
  display, HUD, audio, and unbounded gameplay scheduling remain deferred.

## 2026-06-25: Added bounded CliffClimbQuick2 common2 tick proofs

What changed:

- Added direct and menu-chain dev harness modes
  `battle_mariofox_stage_mpcliffclimb_common2_loop` and
  `menu_chain_mariofox_stage_mpcliffclimb_common2_loop` as modes `117/118`.
- Reused the verified CliffClimb action proof, consumed the created
  CliffClimbQuick2/Ground state, and routed one bounded tick through original
  `ftCommonCliffCommon2ProcUpdate`, original
  `ftCommonCliffCommon2ProcPhysics`, and original
  `ftCommonCliffClimbCommon2ProcMap`.
- Added project-owned diagnostics for the common2 update, anim-end,
  Wait/Fall fallback guard, physics, ground velocity transfer, map, and
  ground-break callback path.
- Promoted the new common2 harness pair to Latest/Boundary profiles and kept
  modes `115/116` as regression coverage for CliffClimbQuick2 setup.

What is proven:

- Direct and menu-chain CliffClimb common2 harnesses report
  `mpCliffClimbCommon2=status=88->88->88->88 motion=76->76->76->76
  cliff=3 floor=3->3 calls=1/1/1`.
- The proof preserves CliffClimbQuick2/Ground `88/76/0`, retained real Pupupu
  `cliff_id=3`, and copied `floor_line_id=3` through one bounded original
  update/physics/map callback tick.
- The guarded anim-end check does not fall through to Wait/Fall, guarded
  ground velocity transfer is reached once, guarded ground-break map callback
  is reached once, and unsafe count remains zero.

Proofs added for this milestone:

- `scripts/verify-battle-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-regression.ps1`

Still deferred:

- Continuous ledge climb/drop runtime, natural ledge occupancy/release rules,
  damage-fall timeout, ledge attack/escape hitboxes, broad map collision, full
  fighter display, HUD, audio, and unbounded gameplay scheduling remain
  deferred.

## 2026-06-26: Added CliffClimbQuick2 finish handoff and cliff-hold reset diagnostics

What changed:

- Added direct/menu-chain `battle_mariofox_stage_mpcliffclimb_finish_loop` and
  `menu_chain_mariofox_stage_mpcliffclimb_finish_loop` as modes `119/120`.
- Corrected the bounded DS-side `ftMainSetStatus` seam to clear
  `is_cliff_hold` like original BattleShip `ftMainSetStatus`. The
  CliffClimb drop proof now expects the Fall/Air branch to clear cliff hold
  instead of preserving stale ledge hold state.
- Added explicit hold-cleared diagnostics for CliffClimb/CliffAttack/
  CliffEscape Quick2 and common2 paths so future status changes cannot regress
  the original cliff-hold lifecycle silently.
- Promoted the new finish harness pair to Latest/Boundary profiles and kept
  the common2 harness pair as regression coverage.

What is proven:

- Direct and menu-chain CliffClimb finish harnesses report
  `mpCliffClimbFinish=status=88->10 motion=76->4 cliff=3 floor=3->3
  calls=1/1/1`.
- The proof consumes CliffClimbQuick2/Ground `88/76/0` through original
  `ftCommonCliffCommon2ProcUpdate`, reaches the guarded animation-end handoff
  through `mpCommonSetFighterWaitOrFall`, calls bounded Wait
  `ftMainSetStatus`, and ends in Wait/Ground `10/4/0`.
- The proof retains `cliff_id=3`, keeps `floor_line_id=3`, sets
  `playertag_wait=120`, restores the special interrupt hook, clears
  `is_cliff_hold`, and parks before continuous ledge climb runtime.

Proofs added for this milestone:

- `scripts/verify-battle-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`

Still deferred:

- Continuous ledge climb/drop runtime, natural ledge occupancy/release rules,
  damage-fall timeout, ledge attack/escape hitboxes, broader collision cases,
  full fighter display, HUD, audio, and unbounded gameplay scheduling remain
  deferred.

## 2026-06-26: Hardened bounded common2 ledge bridge diagnostics

What changed:

- Reviewed the common2 ledge handoff after the cliff-hold reset audit. The
  stale `is_cliff_hold` issue was already fixed by the bounded
  `ftMainSetStatus` seam, but the common2 collision-data bridge still needed
  stronger guard and placement evidence.
- Added a range guard before the bridge calls original
  `ftCommonCliffCommon2UpdateCollData`, using the project/BattleShip
  five-entry `cliff_status_ga` layout as the bound for the queued
  cliff-motion status ID.
- Added project-owned diagnostics for common2 bridge call/pass/reject counts,
  queued status, LR, cliff ID, root X/Y before, root X/Y after, expected
  ledge root X/Y, post-call floor distance, and root-position OK.
- Folded the bridge/root contract into the CliffClimb, CliffAttack, and
  CliffEscape action proof masks and verifier assertions.

What is proven:

- Direct CliffClimb action now reports
  `climbBridge=root=2498000,-250000->2313000,0 exp=2313000,0`.
- Direct CliffAttack action now reports
  `attackBridge=root=2318000,-408000->2313000,0 exp=2313000,0`.
- Direct CliffEscape action now reports
  `escapeBridge=root=2318000,-408000->2313000,0 exp=2313000,0`.
- All three action verifiers require one bridge pass, zero rejects, the
  expected queued status ID (`0`, `1`, or `2`), a changed root, exact expected
  root within one milli-unit, and floor distance reset to zero.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-action-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffattack-action-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffescape-action-loop-harness.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-regression.ps1`

Still deferred:

- Natural ledge occupancy/release, continuous ledge runtime, damage-fall
  timeout, ledge attack/escape hitboxes, broader collision cases, HUD, audio,
  and unbounded gameplay scheduling remain deferred.

## 2026-06-26: Cleared jostle-ignore in bounded status reset

What changed:

- Reviewed the attached cliff-hold audit against the current source. The
  bounded `ftMainSetStatus` seam was already clearing `is_cliff_hold`, but it
  still missed the sibling common reset field `is_jostle_ignore`.
- Extended the project-owned status reset helper so bounded status handoffs
  clear both `is_cliff_hold` and `is_jostle_ignore`, matching the lifecycle the
  original fighter code expects after leaving ledge/action states.
- Added `gNdsStageMPCliffClimbFinishLoopIsJostleIgnoreAfterUpdate` and folded
  it into `STAGE_MPCLIFFCLIMB_FINISH_FLAGS`, so the direct and menu-chain
  CliffClimb finish verifiers now assert stale jostle-ignore state is clear
  after the Wait handoff.
- Updated the current-truth docs and diagnostic reference to describe the
  common reset proof as `reset=0/0`: cliff-hold clear / jostle-ignore clear.

What is proven:

- Direct and menu-chain CliffClimb finish harnesses now report
  `mpCliffClimbFinish=status=88->10 motion=76->4 cliff=3 floor=3->3
  reset=0/0 calls=1/1/1`.
- The proof consumes CliffClimbQuick2/Ground `88/76/0`, reaches bounded Wait
  through the source-order common2 update path, and verifies both stale ledge
  hold and stale jostle-ignore flags are clear once Wait owns the fighter.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`

Still deferred:

- Full original `ftMainSetStatus` common reset coverage beyond the flags now
  needed by this ledge slice, natural ledge occupancy/release, continuous ledge
  runtime, damage-fall timeout, ledge hitboxes, broader collision cases, HUD,
  audio, and unbounded gameplay scheduling remain deferred.

## 2026-06-26: Proved bounded same-cliff ledge occupancy blocker

What changed:

- Reviewed the attached cliff-hold audit against the current source. The
  highest-priority `is_cliff_hold` reset concern is already handled in this
  snapshot by the bounded `ftMainSetStatus` reset helper, including Quick2,
  common2, drop, and finish diagnostics that require stale hold state to clear.
- Inspected original BattleShip `mpcommon.c` for the same-cliff/same-LR
  occupancy scan and kept the implementation inside the existing bounded
  CliffCatch proof instead of opening natural ledge runtime.
- Added a second CliffCatch-local occupancy probe: P0 is seeded as the holder
  on real Pupupu line `3`, P1 attempts the matching right-ledge catch, and the
  original-compatible special-collision callback blocks the attempt before a
  second status setup.
- Added diagnostics and verifier coverage for
  `STAGE_MPCLIFFCATCH_FLOOR_OCC`, including holder/probe cliff IDs, LR, blocked
  probe status/motion/ground-air, cliff-hold after the block, status-set delta,
  and landing-param delta.

What is proven:

- The normal CliffCatch proof still reaches original
  `ftCommonCliffCatchSetStatus` once and transitions P1 from Fall/Air
  `26/20/1` to CliffCatch/Air `84/72/1` on real Pupupu line `3`.
- The occupancy probe records one blocker, holder/probe line `3/3`,
  holder/probe LR `-1/-1`, P1 still Fall/Air `26/20/1`, cliff-hold `0`, and
  no second CliffCatch status or landing-param call: summary `occ=1/0`.
- Direct and menu-chain CliffCatch verifiers now require proof mask `0xfff`
  and two selected map/collision passes while keeping landing/status setup at
  one call.

Proofs run:

- `make -j16`
- `make -j16 TARGET=smash64ds-battle-mariofox-stage-mpcliffcatch-floor-loop BUILD=build-battle-mariofox-stage-mpcliffcatch-floor-loop-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffcatch_floor_loop`
- `scripts/verify-battle-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-regression.ps1`

Still deferred:

- Natural-motion cliffcatch, broader natural ledge occupancy/release/drop/climb,
  continuous ledge runtime, damage-fall timeout, ledge hitboxes, broader
  collision cases, HUD, audio, and unbounded gameplay scheduling remain
  deferred.

## 2026-06-26: Routed common2 fallback through the DS bridge

What changed:

- Removed the last direct fallback from the project-owned
  `ftCommonCliffCommon2UpdateCollData` wrapper to the imported
  `ndsBaseFTCommonCliffCommon2UpdateCollData` helper.
- The fallback now enters the same DS common2 bridge used by the guarded
  CliffClimb/CliffAttack/CliffEscape action proofs. The bridge validates the
  queued cliff-motion status ID, snapshots the live fighter into an
  original-layout temporary struct, lets the imported BattleShip helper mutate
  the live root DObj, then copies selected collision/physics fields back.
- This keeps the original helper as the source of truth while preventing
  broader future common2 paths from bypassing the project-owned status range
  guard and root placement diagnostics.

What is proven:

- Existing action proofs still own their explicit bridge counters through their
  active proof guards.
- The wrapper fallback no longer directly indexes the original
  `cliff_status_ga` table without the port-side guard.

Still deferred:

- Natural, unseeded ledge progression through common2; full ledge physics and
  map behavior beyond the current bounded action/common2/finish proofs; ledge
  hitboxes, HUD, audio, and unbounded gameplay scheduling remain deferred.

## 2026-06-26: Guarded CliffCatch diagnostics during release-then-recatch proof

What changed:

- Scoped the CliffCatch L/R cliff-test diagnostic counters in
  `mpProcessCheckTestLCliffCollision` and `mpProcessCheckTestRCliffCollision`
  to the active CliffCatch map-callback window.
- This prevents the newer CliffClimb release-then-recatch probe from polluting
  the older bounded CliffCatch prerequisite counters while preserving the same
  source-order collision query behavior and collision masks.
- Updated current docs for the CliffClimb floor recatch marker groups and the
  expanded `0x7fff` proof mask.

What is proven:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1`

Both direct and menu-chain proofs now report the bounded recatch summary:
`recatch=26->84 hold=0/1 block=0`.

Still deferred:

- Continuous natural ledge release/drop/climb/occupancy runtime, ledge hitboxes,
  HUD, audio, and unbounded gameplay scheduling remain deferred.

## 2026-06-26: Expanded bounded `ftMainSetStatus` common reset proof

What changed:

- Expanded the project-owned bounded `ftMainSetStatus` common reset helper
  beyond the earlier cliff-hold/jostle proof. The helper now clears the
  current `FTStruct` shell fields that correspond to the original BattleShip
  common reset block: reflect, absorb, shield, fastfall, invisible, shadow
  hide, player-tag hide, cliff hold, jostle ignore, hitstun, ignored line,
  capture immune mask, ghost, camera zoom range, shuffle tics, knockback
  resistance, and stacked knockback, plus ground-state damage-player reset
  when the preserve flag is absent.
- Added `gNdsStageMPCliffClimbFinishLoopCommonResetMask` to the CliffClimb
  finish proof. The probe seeds stale values before the bounded Wait handoff
  and requires `(reset_mask & 0x3ffff) == 0x3ffff` after original
  `mpCommonSetFighterWaitOrFall` reaches bounded Wait `ftMainSetStatus`.
- Updated current-truth and diagnostic docs so the latest direct/menu-chain
  proof summary reports `reset=0/0/0x3ffff`.

What is proven:

- Direct CliffClimb finish reports
  `mpCliffClimbFinish=status=88->10 motion=76->4 cliff=3 floor=3->3
  reset=0/0/0x3ffff calls=1/1/1`.
- Menu-chain CliffClimb finish reports the same reset mask after VS Mode ->
  PlayersVS -> Maps -> VSBattle.
- The bounded status seam now guards the stale status-lifecycle fields needed
  by the current ledge slice, while still parking before unbounded natural
  ledge runtime.

Proofs run:

- `scripts/verify-battle-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1`

Still deferred:

- Full original `ftMainSetStatus` coverage for fields not yet represented in
  the project `FTStruct` shell, natural ledge occupancy/release/drop/climb,
  continuous ledge runtime, damage-fall timeout, ledge hitboxes, broader
  collision cases, HUD, audio, and unbounded gameplay scheduling remain
  deferred.

## 2026-06-26: Added bounded CliffWait timeout into DamageFall proofs

What changed:

- Added direct/menu-chain
  `battle_mariofox_stage_mpcliffwait_damage_loop` and
  `menu_chain_mariofox_stage_mpcliffwait_damage_loop` as modes `121/122`.
- Added a bounded `ftCommonDamageFallSetStatusFromCliffWait` proof path that
  routes through the project-owned `ftMainSetStatus` common-reset seam, then
  installs the current DamageFall/Air status shell and air physics/map
  callbacks.
- Added CliffWait timeout diagnostics for the source-order interrupt checks,
  DamageFall handoff, status/motion/GA transition, fall-wait fields, cliff
  hold reset, retained cliff/floor IDs, callback pointers, and root placement.
- Added direct and menu-chain verifier wrappers and promoted the new pair to
  the current Latest/Boundary verifier profiles.

What is proven:

- The direct and menu-chain harnesses consume the verified CliffWait/Ground
  state on real Pupupu `cliff_id=3`, force `fall_wait=1`, run original
  `ftCommonCliffWaitProcInterrupt`, reach the guarded attack, escape, and
  climb/fall checks in source order, take original `ftCommonCliffWaitCheckFall`,
  and call the bounded DamageFall status seam.
- The proof requires CliffWait/Ground `85/73/0 ->` DamageFall/Air `57/50/1`,
  `fall_wait 1 -> 0`, `cliffcatch_wait=30`,
  `tics_since_last_z=65536`, retained `cliff_id=3`/`floor_line_id=3`,
  `is_cliff_hold 1 -> 0`, installed DamageFall air callbacks, and no unsafe
  guard escape.

Proofs run:

- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-current.ps1`

Still deferred:

- DamageFall collision outcomes beyond the no-collision branch, natural ledge
  occupancy/release/drop behavior beyond the bounded proofs, ledge
  hitboxes/damage, broader collision cases, HUD, audio, and unbounded gameplay
  scheduling remain deferred.

## 2026-06-26: Added one guarded original DamageFall callback tick

What changed:

- Imported original `ftcommondamagefall.c` through
  `src/import/battleship_ftcommon_damagefall.c` under `ndsBase*` symbols.
- Updated the bounded CliffWait timeout proof to call the imported original
  `ftCommonDamageFallClampRumble` helper and install original-compatible
  DamageFall interrupt/map callback wrappers.
- Added guarded compatibility stubs for the first DamageFall dependency
  surface: special-air, aerial-attack, aerial-jump, HammerFall, rumble,
  `mpCommonCheckFighterCliff`, passive checks, and down-bounce.
- Extended the direct/menu-chain CliffWait damage verifier to require one
  original DamageFall interrupt tick, one
  `ftPhysicsApplyAirVelDriftFastFall` physics tick, and one original
  `ftCommonDamageFallProcMap` no-collision map tick.

What is proven:

- Direct and menu-chain harnesses still prove CliffWait/Ground `85/73/0 ->`
  DamageFall/Air `57/50/1`, `fall_wait 1 -> 0`, `cliffcatch_wait=30`,
  `tics_since_last_z=65536`, retained `cliff_id=3`/`floor_line_id=3`, and
  `is_cliff_hold 1 -> 0`.
- The same proof now reaches original DamageFall interrupt logic, records the
  source-order false special-air/aerial-attack/aerial-jump/HammerFall checks,
  applies the air-velocity drift/fastfall physics callback with velocity-Y
  `0 -> -4000`, calls original `ftCommonDamageFallProcMap`, and proves the
  bounded no-collision `mpCommonCheckFighterCliff == FALSE` branch without
  passive/down-bounce escape.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-regression.ps1`

Still deferred:

- DamageFall collision outcomes after a positive map collision, including
  cliff catch, passive stand/passive, down-bounce, landing/collision branches,
  hitboxes/damage, broader natural ledge runtime, HUD, audio, and unbounded
  gameplay scheduling remain deferred.

## 2026-06-26: Added bounded DamageFall positive collision into DownBounce setup

What changed:

- Imported original `ftcommondownwaitbounce.c` through
  `src/import/battleship_ftcommon_downwaitbounce.c` under `ndsBase*` symbols.
- Extended the direct/menu-chain CliffWait damage harnesses so the first
  guarded DamageFall map tick still proves the no-collision branch, then a
  second guarded map pass forces a positive floor collision through
  `mpCommonCheckFighterCliff`.
- Added the bounded DownBounce status seam and compatibility diagnostics for
  ground placement, ImpactWave effect, FGM, rumble, velocity transfer,
  attack-buffer reset, and `damage_mul` reduction.
- Updated the CliffWait damage verifier marker contract from mask `0x7ff` to
  `0x1fff` and added `STAGE_MPCLIFFWAIT_DAMAGE_COLLISION`.

What is proven:

- Direct and menu-chain harnesses still prove CliffWait/Ground `85/73/0 ->`
  DamageFall/Air `57/50/1`, `fall_wait 1 -> 0`, `cliffcatch_wait=30`,
  `tics_since_last_z=65536`, retained `cliff_id=3`/`floor_line_id=3`, and
  `is_cliff_hold 1 -> 0`.
- The first guarded DamageFall callback tick remains a no-collision map pass
  with status/motion/GA still `57/50/1` and velocity-Y `0 -> -4000`.
- The second guarded map pass reaches original passive-stand/passive checks,
  falls through to `ftCommonDownBounceSetStatus`, and proves
  DownBounceU/Ground `68/59/0`, collision hit `1`, ground/effect/FGM/rumble/
  velocity-transfer counts `1`, attack buffer `0`, `damage_mul=500` milli,
  effect kind `22`, nonzero FGM, and rumble ID `4`.

Proofs run:

- `scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-current.ps1`

Still deferred:

- Continuous DownBounce/DownWait callback runtime, DamageFall cliff-catch and
  passive collision branches, hitboxes/damage, broader natural ledge runtime,
  HUD, audio, and unbounded gameplay scheduling remain deferred.

## 2026-06-26: Added bounded DownBounce and DownWait callback follow-through

What changed:

- Extended the direct/menu-chain CliffWait damage harnesses past the previous
  DownBounce setup proof without opening continuous gameplay.
- Routed the selected DownBounceU/Ground state through two guarded original
  `ftCommonDownBounceProcUpdate` calls: first with an A-button tap to prove
  `attack_buffer=60`, then with animation end to reach the original attack and
  forward/back checks and the bounded DownWait setup seam.
- Added a bounded DownWaitU/Ground `ftMainSetStatus` path for the original
  `motion_id == -2` sentinel case, with the original DownWait update,
  interrupt, physics, and map callback pointers installed.
- Added compatibility diagnostics and verifier rows for
  `STAGE_MPCLIFFWAIT_DAMAGE_DOWNBOUNCE` and
  `STAGE_MPCLIFFWAIT_DAMAGE_DOWNWAIT`; the proof mask is now `0x1ffff`.

What is proven:

- Direct and menu-chain harnesses still prove CliffWait/Ground `85/73/0 ->`
  DamageFall/Air `57/50/1`, `is_cliff_hold 1 -> 0`, one no-collision
  DamageFall tick, and one positive DamageFall map-collision branch into
  DownBounceU/Ground `68/59/0`.
- The first DownBounce update tick records `attack_buffer=60` and stays
  DownBounceU/Ground `68/59/0`.
- The second DownBounce update reaches DownWaitU/Ground `70/-2/0`, sets
  `stand_wait=180`, capture mask `0x33`, and `damage_mul=500` milli.
- One original DownWait update tick decrements `stand_wait 180 -> 179`,
  does not call DownStand, and remains DownWaitU/Ground `70/-2/0`.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-boundary.ps1`

Still deferred:

- DamageFall cliff-catch/passive collision branches, natural downed wakeup
  runtime beyond one DownWait tick, hitboxes/damage, broader natural ledge
  runtime, HUD, audio, and unbounded gameplay scheduling remain deferred.

## 2026-06-26: Cleared bounded status `proc_damage` and proved DownWait timeout into DownStand

What changed:

- Updated the bounded `ftMainSetStatus` common reset seam so status changes
  clear stale `proc_damage` in addition to the existing cliff-hold/jostle reset
  behavior.
- Extended the CliffWait damage diagnostics and verifier rows to require
  DamageFall and DownStand states to leave `proc_damage` cleared, while ledge
  Quick1-style states still explicitly opt back into
  `ftCommonCliffCommonProcDamage`.
- Finished the bounded DownWait timeout follow-through by importing/proving the
  original DownStandU setup path after the stable `stand_wait 180 -> 179` tick.
- Updated current docs and verifier summaries for proof mask `0x3ffff`,
  `procDmg=0`, and DownStandU/Ground `72/61/0` with wakeup flag `1 -> 0`.

What is proven:

- Direct and menu-chain harnesses still prove CliffWait/Ground `85/73/0 ->`
  DamageFall/Air `57/50/1`, `is_cliff_hold 1 -> 0`, and the guarded
  DamageFall no-collision plus positive collision paths.
- The positive collision path reaches DownBounceU/Ground `68/59/0`, then
  DownWaitU/Ground `70/-2/0`, proves one stable original DownWait update tick,
  and then reaches DownStandU/Ground `72/61/0` through the original timeout
  path.
- Ledge drop, Quick2, attack/escape/climb action, and CliffWait damage proofs
  now assert the relevant stale `proc_damage` state is cleared where original
  `ftMainSetStatus` would clear it.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-action-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-action-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffattack-action-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffattack-action-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffescape-action-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffescape-action-loop-harness.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-regression.ps1`

Still deferred:

- DamageFall cliff-catch/passive collision branches, natural downed wakeup
  runtime beyond the bounded DownStand setup proof, natural ledge occupancy/
  release/drop/climb behavior, hitboxes/damage, broader map collision, HUD,
  audio, and unbounded gameplay scheduling remain deferred.

## 2026-06-26: Added bounded DamageFall cliff-catch branch proof

What changed:

- Extended the direct/menu-chain CliffWait damage harnesses so the first
  guarded DamageFall map tick still proves the no-collision branch, then a new
  positive right-cliff map pass routes through original
  `mpCommonCheckFighterCliff` into original
  `ftCommonCliffCatchSetStatus`, before the existing positive floor-collision
  pass continues into DownBounce/DownWait/DownStand.
- Added `STAGE_MPCLIFFWAIT_DAMAGE_CLIFFCATCH` diagnostics for the bounded
  CliffCatch status seam, ground/air placement calls, animation-event call,
  velocity stop, ledge flash, capture-immune setup, final status/motion/GA,
  cliff hold, proc-damage/callback pointers, retained cliff ID, cleared floor
  line, right-cliff masks, and capture mask.
- Updated the CliffWait damage proof mask from `0x3ffff` to `0x7ffff` and
  tightened the verifier to require three guarded DamageFall map/cliff checks:
  no-collision, CliffCatch, and DownBounce.

What is proven:

- Direct and menu-chain harnesses still prove CliffWait/Ground `85/73/0 ->`
  DamageFall/Air `57/50/1`, `fall_wait 1 -> 0`, `cliffcatch_wait=30`,
  `tics_since_last_z=65536`, and `is_cliff_hold 1 -> 0` on DamageFall setup.
- The first guarded DamageFall map pass remains a no-collision branch with
  velocity-Y `0 -> -4000`.
- The new positive right-cliff map pass reaches bounded original
  CliffCatch/Air `84/72/1`, restores cliff hold, installs cliff damage and
  CliffCatch/Common callbacks, records `floor_line_id=-1`, right-cliff masks
  `0x2000`, and capture mask `0x4`.
- The existing positive floor-collision map pass still reaches
  DownBounceU/Ground `68/59/0`, then bounded DownBounce update,
  DownWaitU/Ground `70/-2/0`, and DownStandU/Ground `72/61/0`.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`

Still deferred:

- DamageFall passive collision branches, natural downed wakeup runtime beyond
  the bounded DownStand setup proof, natural ledge occupancy/release/drop/climb
  behavior, hitboxes/damage, broader map collision, HUD, audio, and unbounded
  gameplay scheduling remain deferred.

## 2026-06-26: Added bounded DamageFall PassiveStand and Passive setup proofs

What changed:

- Imported original `ftcommonpassivestand.c` and `ftcommonpassive.c` through
  `src/import` wrappers under `ndsBase*` names.
- Replaced the CliffWait damage harness passive false stubs with bounded calls
  into the imported original passive checks while
  `ftCommonDamageFallProcMap` is active for the proof.
- Added bounded `ftMainSetStatus` contracts for PassiveStandF/PassiveStandB
  and Passive, with the original callback shape for update, physics, and map
  functions.
- Extended direct and menu-chain CliffWait damage probes from three guarded
  DamageFall map passes to five: no-collision, CliffCatch, PassiveStand,
  Passive, and DownBounce.
- Added `STAGE_MPCLIFFWAIT_DAMAGE_PASSIVESTAND` and
  `STAGE_MPCLIFFWAIT_DAMAGE_PASSIVE` markers and raised the proof mask from
  `0x7ffff` to `0x1fffff`.

What is proven:

- The buffered-stick positive floor-collision path reaches original
  `ftCommonPassiveStandCheckInterruptDamage`, calls the original
  PassiveStand setup helper, grounds the fighter, reaches the bounded
  `ftMainSetStatus` seam, and proves PassiveStandF/Ground `73/62/0`.
- The buffered-neutral positive floor-collision path reaches original
  `ftCommonPassiveCheckInterruptDamage`, calls the original Passive setup
  helper, grounds the fighter, reaches the bounded `ftMainSetStatus` seam, and
  proves Passive/Ground `81/70/0`.
- The existing no-collision, CliffCatch, DownBounce, DownWait, and DownStand
  proof chain remains green with `dmgTick=1/1/5` and `coll=4`.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`

Still deferred:

- Full PassiveStand/Passive action runtime beyond setup, original
  damage-velocity field import, natural downed wakeup runtime beyond the
  bounded DownStand setup proof, natural ledge occupancy/release/drop/climb
  behavior, hitboxes/damage, broader map collision, HUD, audio, and unbounded
  gameplay scheduling remain deferred.

## 2026-06-26: Added bounded PassiveStand/Passive anim-end handoff proofs

What changed:

- Extended the bounded CliffWait timeout/DamageFall proof after the imported
  PassiveStandF/Ground and Passive/Ground setup branches.
- Ran one guarded original `ftAnimEndSetWait` update tick for each passive
  branch and routed the resulting Wait transition through the project-owned
  bounded `ftMainSetStatus` seam.
- Added `STAGE_MPCLIFFWAIT_DAMAGE_PASSIVESTAND_TICK` and
  `STAGE_MPCLIFFWAIT_DAMAGE_PASSIVE_TICK` diagnostics and raised the current
  DamageFall proof mask to `0x7fffff`.
- Kept the original-compatible common status reset active, including
  `is_cliff_hold = FALSE`, while the Wait handoff installs the current Wait
  callbacks and player-tag wait behavior.

What is proven:

- PassiveStandF/Ground `73/62/0` now reaches Wait/Ground `10/4/0` through one
  guarded original `ftAnimEndSetWait` callback tick.
- Passive/Ground `81/70/0` now reaches Wait/Ground `10/4/0` through the same
  bounded original update path.
- Each branch records one Wait `ftMainSetStatus` call, one player-tag wait
  call, `playertag_wait=120`, and a valid Wait callback shape.
- Direct and menu-chain CliffWait damage harnesses both prove the same
  PassiveStand/Passive setup-to-Wait handoff.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-regression.ps1`

Still deferred:

- Continuous PassiveStand/Passive animation, physics, and map runtime beyond
  this single guarded anim-end handoff, natural ledge occupancy/release/drop/
  climb behavior, hitboxes/damage, broader map collision, HUD, audio, and
  unbounded gameplay scheduling remain deferred.

## 2026-06-26: Corrected Wait callback, damage multiplier reset, and ground transfer seams

What changed:

- Fixed bounded Wait-status handoffs that were installing `proc_interrupt=NULL`
  so they now install the original-compatible `ftCommonWaitProcInterrupt`.
- Restored `damage_mul=1.0F` in the project-owned common `ftMainSetStatus`
  reset seam and in seeded fighter state.
- Adjusted the bounded `mpCommonSetFighterGround` seam to use the
  source-order signed ground velocity transfer `vel_air.x * lr` and stop
  clearing airborne Y velocity.
- Updated the dependent Fall-landing and all-DL prerequisite verifiers for the
  corrected source-order diagnostics.

What is proven:

- CliffClimbQuick2 finish now reaches Wait/Ground `10/4/0` with the Wait
  interrupt callback installed, `playertag_wait=120`, and common reset mask
  `0x7ffff`, including `damage_mul=1.0`.
- CliffWait damage DownStand timeout now proves `damage_mul=1000` milli after
  the bounded common status reset.
- Fall-landing keeps the source-order negative airborne Y velocity diagnostic
  after the ground transition while still clamping the position/status path.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-battle-mariofox-walk-input-harness.ps1`
- `scripts/verify-menu-chain-mariofox-walk-input-harness.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-all.ps1 -Profile Full -From battle_mariofox_walk_input`

Still deferred:

- Full natural ledge runtime, hitboxes/damage, broader map collision, HUD,
  audio, and unbounded gameplay scheduling remain deferred.

## 2026-06-26: Added bounded PassiveStand/Passive physics-map callback tick proofs

What changed:

- Extended the existing CliffWait timeout/DamageFall passive branches so each
  imported PassiveStandF/Ground and Passive/Ground state now runs its installed
  physics/map callback pair before the existing animation-end Wait handoff.
- Added guarded PassiveStand callback diagnostics for
  `ftPhysicsApplyGroundVelTransN` and `mpCommonSetFighterFallOnEdgeBreak`.
- Added guarded Passive callback diagnostics for the bounded
  `ftPhysicsApplyGroundVelFriction` path and
  `mpCommonSetFighterFallOnGroundBreak`.
- Raised the current CliffWait DamageFall proof mask from `0x7fffff` to
  `0x1ffffff`.

What is proven:

- PassiveStandF/Ground `73/62/0` retains its status/motion/ground state after
  one guarded physics/map callback pair, then reaches Wait/Ground `10/4/0`
  through the existing original `ftAnimEndSetWait` handoff.
- Passive/Ground `81/70/0` retains its status/motion/ground state after one
  guarded physics/map callback pair, then reaches Wait/Ground `10/4/0`.
- Both direct and menu-chain proof summaries now record
  `passiveStand=73/62/0->cb1/1->10/4/0` and
  `passive=81/70/0->cb1/1->10/4/0`.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`

Still deferred:

- Continuous PassiveStand/Passive runtime beyond a single guarded callback
  pair and anim-end handoff, full natural ledge runtime, hitboxes/damage,
  broader map collision, HUD, audio, and unbounded gameplay scheduling remain
  deferred.

## 2026-06-26: Added bounded multi-frame PassiveStand/Passive runtime proofs

What changed:

- Added direct and menu-chain `battle_mariofox_stage_mppassive_loop` harness
  modes `123/124`.
- Added Passive-loop diagnostics that consume the verified CliffWait damage
  PassiveStand/Passive setup states instead of replaying the whole opening path.
- Added guarded PassiveStandF/Ground and Passive/Ground runtime probes that run
  two stable update/physics/map frames, then force the original animation-end
  update handoff into Wait/Ground.
- Promoted the new direct/menu-chain pair to the Boundary and Latest verifier
  profiles.

What is proven:

- PassiveStandF/Ground `73/62/0` remains stable across two guarded
  update/physics/map frames, then reaches Wait/Ground `10/4/0` through the
  original `ftAnimEndSetWait` path.
- Passive/Ground `81/70/0` remains stable across two guarded
  update/physics/map frames, then reaches Wait/Ground `10/4/0`.
- Both branches keep valid final Wait callbacks and `playertag_wait=120`.
- Direct and menu-chain proof summaries now record
  `mpPassive=ps=73/62->10/4 ticks=3/2/2 passive=81/70->10/4 ticks=3/2/2`.

Proofs run:

- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-mppassive-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mppassive-loop-harness.ps1`

Still deferred:

- Natural player-driven PassiveStand/Passive recovery runtime beyond the two
  guarded stable frames, full natural ledge runtime, hitboxes/damage, broader
  map collision, HUD, audio, and unbounded gameplay scheduling remain deferred.

## 2026-06-26: Optimized handoff snapshot and generated-output hygiene

What changed:

- Preserved the previous broad snapshot exporter as
  `scripts/New-Smash64DSSnapshot.Legacy.ps1`.
- Added a new parameterized `scripts/New-Smash64DSSnapshot.ps1` with
  `-Mode Lean`, `-Mode CodeOnly`, and `-Mode Full`.
- Added shared snapshot hygiene classification in
  `scripts/lib/snapshot-hygiene.ps1` and a standalone
  `scripts/check-snapshot-hygiene.ps1` archive checker.
- Updated `scripts/clean-generated.ps1 -KeepLatestBuilds` to derive preserved
  build/output targets from the current Boundary profile in
  `scripts/lib/harness-registry.ps1` instead of hardcoded old Walk-input
  paths.
- Updated handoff docs to use `New-Smash64DSSnapshot.ps1 -Mode Lean` as the
  default path, with CodeOnly and Full documented as explicit exceptions.

What is proven:

- Lean dry run excludes build directories, root ROM/ELF outputs, emulator
  payloads, `.git`, and artifacts by default while retaining `decomp/`.
- CodeOnly dry run excludes `decomp/`.
- Full dry run prints a warning that it is not the normal handoff path.
- `clean-generated.ps1 -DryRun -KeepLatestBuilds` reports the current
  registry Boundary harnesses as the preserved latest builds and treats old
  Walk-input outputs as removable generated files.

Still deferred:

- This was project hygiene only. It intentionally does not change gameplay,
  renderer, taskman, fighter, stage, or source-boundary behavior.

## 2026-06-26: Added bounded DownWait interrupt-loop proof

What changed:

- Added direct and menu-chain `battle_mariofox_stage_mpdownwait_loop` harness
  modes `125/126`.
- Added DownWait-loop diagnostics and verifier markers for setup, original
  interrupt source order, seeded input, and final DownStand state.
- Wired the DownWait proof after the Passive-loop proof while keeping the
  normal boot path unchanged.
- Promoted the new direct/menu-chain pair to the Boundary and Latest verifier
  profiles.

What is proven:

- A fresh DownBounceU/Ground shell reaches DownWaitU/Ground `70/-2/0` through
  original `ftCommonDownWaitSetStatus` and the bounded project-owned
  `ftMainSetStatus` seam.
- The installed original DownWait interrupt callback runs the source-order
  DownAttack, forward/back, and DownStand check path, recorded as `0x12345`.
- The bounded branch reaches DownStandU/Ground `72/61/0`, clears `flag1`,
  installs DownStand callbacks, and restores `damage_mul=1.0`.

Proofs run:

- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1`

Still deferred:

- Broader player-driven DownWait attack/roll/stand input coverage, continuous
  DownStand runtime, hitboxes/damage, full natural knockdown recovery, broader
  map collision, HUD, audio, and unbounded gameplay scheduling remain deferred.

## 2026-06-26: Expanded bounded DownWait recovery input proof

What changed:

- Imported original `ftcommondownattack.c` and `ftcommondownforwardback.c`
  through `src/import` wrappers under `ndsBase*` symbols.
- Fixed the bounded DownWait proof seam so the project-owned wrapper arms the
  guarded `ftMainSetStatus` path before calling imported original DownAttack
  and DownForward/Back check functions. This preserves the original check logic
  while avoiding a broad gameplay import.
- Extended the direct and menu-chain DownWait harness diagnostics to prove
  original DownWait interrupt source order into A-button DownAttackU `80/69`,
  forward-stick DownForwardU `76/65`, back-stick DownBackU `78/67`, and the
  existing stick-up DownStandU `72/61` branch.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1`

Still deferred:

- Continuous DownAttack/DownForward/DownBack/DownStand recovery action runtime,
  hitboxes/damage, and natural gameplay scheduling remain deferred.

## 2026-06-26: Added bounded DownWait attack/roll callback handoff proof

What changed:

- Extended the existing direct/menu-chain MP DownWait-loop proof without adding
  a new harness mode.
- Added project-owned guarded callback diagnostics for the DownAttackU,
  DownForwardU, and DownBackU branches reached through the imported original
  `ftCommonDownWaitProcInterrupt` path.
- Added bounded seams for the selected branch callbacks:
  `ftAnimEndSetWait`, `ftPhysicsApplyGroundVelFriction`,
  `ftPhysicsApplyGroundVelTransN`, `mpCommonSetFighterFallOnEdgeBreak`,
  `ftMainSetStatus`, and `ftParamSetPlayerTagWait`.
- Updated the DownWait verifier markers so the proof now requires setup,
  source order, one stable update/physics/map callback tick, and animation-end
  handoff back to Wait/Ground for the attack/roll branches.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1`
- `scripts/check-harness-registry.ps1`

What is proven:

- Direct and menu-chain DownWait-loop harnesses now report
  `mpDownWait=status=70/-2->72/61 attack=80/69->10/4
  rolls=76/65->10/4,78/67->10/4 ticks=2/2/2 source=0x12345`.
- DownAttackU, DownForwardU, and DownBackU each execute one guarded stable
  callback tick and then return through original animation-end logic to
  Wait/Ground `10/4/0` with `playertag_wait=120`.

Still deferred:

- Continuous DownAttack/DownForward/DownBack/DownStand recovery action runtime,
  hitboxes/damage, full natural knockdown recovery, natural ledge occupancy/
  release/drop/climb behavior, platform pass-through, and full gameplay remain
  deferred.

## 2026-06-26 - Bounded Cliff-Ledge Occupancy/Release/Drop/Climb Aggregation

Added direct and menu-chain `battle_mariofox_stage_mpcliffledge_loop` /
`menu_chain_mariofox_stage_mpcliffledge_loop` harnesses as modes `131/132`.
These modes do not import broad ledge runtime. They aggregate the existing
bounded original-code cliff-catch, CliffWait, CliffClimb, CliffClimbQuick2
finish, and DownRecover proof slices to prove one narrow ledge spine.

What changed:

- Added harness mode defines, Makefile mappings, scene-harness seeding, taskman
  prepare/finalize calls, registry entries, and verifier wrappers for the new
  direct/menu-chain pair.
- Added `STAGE_MPCLIFFLEDGE*` diagnostics and verifier assertions for:
  same-cliff occupancy block, drop/release into Fall/Air, post-release recatch,
  and CliffClimbQuick2 finish into Wait/Ground.
- Extended the cliff-climb-finish compile guard so the new ledge aggregate mode
  pulls in the existing CliffClimb finish chain instead of reading uninitialized
  diagnostics.
- Updated docs and profile registry so Latest/Boundary now point at modes
  `131/132`.

Proofs run:

- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffledge-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffledge-loop-harness.ps1`

What is proven:

- Direct and menu-chain ledge harnesses report `gcRunAll=33`, `gcDrawAll=7`,
  selected display callbacks `42/42`, `draws=7`, `pixels=930`, and
  `final=Wait/Ground`.
- Same-cliff occupancy blocks a second catch on Pupupu line `3`.
- Drop/release reaches Fall/Air `26/20/1`, sets `cliffcatch_wait=30`, clears
  cliff hold, and preserves valid callbacks.
- Recatch after release reaches CliffCatch/Air `84/72/1` with cliff hold set
  and no occupancy block.
- CliffClimbQuick2 finishes through the original Wait/Ground handoff
  `10/4/0`, preserving line continuity `3/3/3`.

Still deferred:

- Continuous selected-callback ledge runtime, real multi-frame opponent ledge
  contention, ledge invulnerability/timing, attacks from ledge, broad
  platform/ledge/wall contracts, hitboxes/damage, and full gameplay remain
  deferred.

## 2026-06-26: Added bounded face-down DownRecover continuation proof

What changed:

- Added direct and menu-chain `battle_mariofox_stage_mpdownrecover_loop`
  harnesses as modes `129/130`, building on the verified Turn-loop proof.
- Added bounded diagnostics and wrappers for the face-down DownWaitD recovery
  continuation without unbounding gameplay scheduling.
- Seeded a fresh DownWaitD/Ground shell, called original
  `ftCommonDownWaitProcInterrupt`, and proved source-order branches into
  DownStandD, DownAttackD, DownForwardD, and DownBackD.
- Recorded original animation-end handoffs from all four created branches back
  to Wait/Ground, including DownAttackD attack IDs `52/32/32`.
- Extended the harness registry, boundary/current profiles, wrapper scripts,
  and diagnostic marker parser for the new `STAGE_MPDOWNRECOVER*` marker set.

Proofs run:

- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-mpdownrecover-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpdownrecover-loop-harness.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-all.ps1 -Profile Regression -From battle_mariofox_stage_mpcliffwait_damage_loop`

`scripts/verify-regression.ps1` was attempted as the complete Regression
profile and hit the 20-minute command timeout before the wrapper returned
usable progress output. The resumed status-heavy Regression tail from
`battle_mariofox_stage_mpcliffwait_damage_loop` passed.

What is proven:

- Direct and menu-chain DownRecover harnesses report DownWaitD `69/-2`, then
  DownStandD `71/60`, DownAttackD `79/68`, DownForwardD `75/64`, and
  DownBackD `77/66`.
- The source-order markers are `0x12345` for DownStandD/DownForwardD/DownBackD
  and `0x1234` for DownAttackD.
- Four bounded animation-end updates return the proof-owned fighter to
  Wait/Ground with final status/motion/callback masks `0xf` and unsafe count
  `0`.

Still deferred:

- Continuous downed action runtime, hitboxes, invulnerability timing, opponent
  interactions, natural knockdown recovery from real gameplay damage, natural
  ledge occupancy/release/drop/climb behavior, platform pass-through, and full
  gameplay remain deferred.

## 2026-06-26: Added static doc/architecture governance and slim AGENTS map

What changed:

- Replaced the long `AGENTS.md` with a short project map under the agent-legible
  size budget.
- Added `docs/README.md` as the project-owned docs index.
- Added `docs/VERIFYING.md` for build, static-check, verifier, emulator, and
  snapshot workflow.
- Added `docs/HARNESSES.md` for harness registry policy, naming rules, current
  boundary, and generator direction.
- Added `scripts/check-docs.ps1` to validate required docs, docs indexing,
  `AGENTS.md` size/section budgets, current Boundary references, harness index
  source, and prospective architecture stub/deferred markers.
- Added `scripts/check-architecture.ps1` to enforce read-only `decomp/`,
  generated-output tracking rules, source directory ownership, decomp includes
  only through `src/import`, import wrapper provenance, Mario/Fox direct/menu
  pairing, and large-file split-plan presence.
- Added a root `.github/workflows/static-checks.yml` workflow for static checks
  that do not require devkitPro, melonDS, or no$gba.
- Added a large backend file split plan to `docs/ARCHITECTURE.md` so the new
  architecture lint can warn about oversized files without forcing an unsafe
  behavior-changing split.

Proofs run:

- `scripts/check-docs.ps1`
- `scripts/check-architecture.ps1`
- `scripts/check-harness-registry.ps1`
- `scripts/check-gbi-decode-fixtures.ps1`
- `scripts/clean-generated.ps1 -DryRun`

What is proven:

- `AGENTS.md` is now 121 lines and points agents to structured docs instead of
  carrying the full verifier/harness narrative.
- The current Boundary pair remains indexed in `STATUS.md` and `HANDOFF.md`.
- The new static CI path can run without emulator or devkitPro dependencies.

Still deferred:

- Harness-pair generation remains manual. The next tooling milestone should
  generate registry, Makefile, mode defines, wrapper scripts, and docs/template
  text together.
- Splitting `src/port/reloc_backend.c`, `src/port/taskman_seam.c`, and
  `include/nds/nds_startup.h` remains deferred until it can be done as
  behavior-preserving mechanical refactors with verifier coverage.

## 2026-06-26: Added bounded original Wait -> Turn -> Wait proof

What changed:

- Imported original `ftcommonturn.c` through
  `src/import/battleship_ftcommon_turn.c`, remapping the public Turn symbols to
  project-owned bounded wrappers.
- Added direct and menu-chain `battle_mariofox_stage_turn_loop` harnesses
  (modes `127/128`) and promoted them to the Boundary/Latest verifier profiles.
- Added narrow Turn compatibility state in `FTStatusVars.common.turn`, plus
  diagnostics for the original Turn input check, status setup, update flip,
  physics/map callback tick, and final Wait handoff.
- Kept the proof bounded after the verified DownWait-loop base; no fighter
  gameplay loop, attacks, hitboxes, items, HUD, audio, or broad `ftmain.c`
  import was added.

Proofs run:

- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/verify-battle-mariofox-stage-turn-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-turn-loop-harness.ps1`

What is proven:

- Direct and menu-chain Turn harnesses report Wait/Ground `10/4/0` ->
  Turn/Ground `18/12/0` -> Wait/Ground `10/4/0`.
- The original Turn update path flips facing `1 -> -1`, negates ground
  velocity `2500 -> -2500` milli, clears `motion_vars.flags.flag1`, sets the
  original Turn direction / special-attack interrupt flags, and then reaches
  the original animation-end Wait handoff with `playertag_wait=120`.
- The DownWait-loop pair remains regression coverage for the bounded original
  DownAttackU, DownForwardU, DownBackU, and DownStandU branches.

Still deferred:

- Continuous player-driven lateral ground movement around Turn/Walk/Dash
  handoffs, natural knockdown recovery runtime, natural ledge
  occupancy/release/drop/climb behavior, platform pass-through, and full
  gameplay remain deferred.

## 2026-06-26: Extended bounded DownWait recovery runtime and roll movement proof

What changed:

- Extended the direct/menu-chain MP DownWait-loop proof from three to eight
  guarded stable update/physics/map frames for DownAttackU, DownForwardU,
  DownBackU, and DownStandU before the deliberate animation-end handoff.
- Added proof-owned DownForward/DownBack root-motion diagnostics behind the
  existing DownWait harness guard. The roll callback path now uses the local
  ground-velocity transfer seam to move the root in the expected direction,
  then records forward/back root X before, after, delta, and a direction mask.
- Added `STAGE_MPDOWNWAIT_ROLL_MOVE` GDB marker and verifier assertions so the
  proof fails if the bounded roll branches stop moving forward/backward.
- Updated current-truth docs and verifier summaries to report the new
  `ticks=9/9/9`, `downStandTicks=9/8/8`, `dsChecks=8/8/8`, and
  `rollDelta=10000/-10000` boundary.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1`

What is proven:

- Direct and menu-chain DownWait-loop harnesses still pass from Pupupu
  VSBattle, selected `gcRunAll=33`, selected `gcDrawAll=7`, selected display
  callbacks `42/42`, and final Wait/Ground.
- DownAttackU `80/69`, DownForwardU `76/65`, and DownBackU `78/67` now each
  survive eight guarded stable callback frames, then return through original
  `ftAnimEndCheckSetStatus` to Wait/Ground `10/4/0`.
- DownForwardU and DownBackU now prove bounded root motion in opposite
  directions with `rollDelta=10000/-10000` milli before the final Wait handoff.

Still deferred:

- True animation-event-sourced TransN velocity, hitbox/damage activation,
  full natural knockdown recovery runtime, platform pass-through, and full
  gameplay remain deferred.

## 2026-06-26: Added bounded DownStand callback and Wait handoff proof

What changed:

- Extended the existing direct/menu-chain MP DownWait-loop proof without adding
  a new harness mode.
- Added a guarded project-owned DownStand callback slice after the original
  DownWait stand-up branch reaches DownStandU/Ground `72/61/0`.
- Routed the imported original `ftCommonDownStandProcInterrupt` through bounded
  KneeBend, Pass, and Dokan compatibility checks, each counted and held false
  to avoid starting unrelated statuses.
- Added guarded DownStand update/physics/map diagnostics for
  `ftAnimEndSetWait`, `ftPhysicsApplyGroundVelFriction`, and
  `mpCommonSetFighterFallOnGroundBreak`, then proved the animation-end handoff
  into Wait/Ground.
- Updated the DownWait verifier mask from `0xffff` to `0x3ffff` so the new
  DownStand interrupt/callback proof is required for pass.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1`

What is proven:

- Direct and menu-chain DownWait-loop harnesses now report
  `mpDownWait=status=70/-2->72/61->10/4 ... downStandTicks=2/1/1 ...
  dsChecks=1/1/1`.
- The DownStand branch runs one guarded imported interrupt tick, reaches
  KneeBend/Pass/Dokan checks in source order through the original DownStand
  interrupt function, stays DownStandU/Ground for one stable callback tick,
  then returns through the original animation-end path to Wait/Ground
  `10/4/0` with `playertag_wait=120`.

Still deferred:

- Continuous knockdown recovery action runtime, hitboxes/damage, full natural
  knockdown recovery, natural ledge occupancy/release/drop/climb behavior,
  platform pass-through, and full gameplay remain deferred.

## 2026-06-26: Corrected ftAnimEndSetWait gating and DownAttack attack IDs

What changed:

- Corrected the project-owned `ftAnimEndSetWait` compatibility seam to route
  all Wait handoffs through `ftAnimEndCheckSetStatus`, matching the original
  animation-end gate instead of directly forcing `ftCommonWaitSetStatus`.
- Updated proof-owned forced animation-end call sites to set the bounded GObj
  and FTStruct animation frame to `0.0F` before invoking `ftAnimEndSetWait`;
  this preserves the original gate while still allowing deterministic bounded
  proof progression.
- Added original-compatible DownAttack motion/status attack IDs:
  DownAttackD `52/32/32` and DownAttackU `53/33/33`.
- Updated the bounded DownAttack `ftMainSetStatus` branch so DownAttackD/U
  populate `motion_attack_id`, `status_attack_id`, and `stat_attack_id`
  instead of leaving them at `None`.
- Extended the DownWait verifier marker so `STAGE_MPDOWNWAIT_ATTACK` requires
  DownAttackU attack IDs `53/33/33`, and kept the three stable callback-frame
  checks before the original animation-end Wait handoff.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1`

What is proven:

- Direct and menu-chain DownWait-loop harnesses still report
  `gcRunAll=33`, `gcDrawAll=7`, selected display callbacks `42/42`, and
  `final=Wait/Ground`.
- DownAttackU now reaches `status/motion/ga 80/69/0` with attack IDs
  `53/33/33`, runs three guarded stable update/physics/map frames plus the
  final animation-end update, then returns through the original
  `ftAnimEndCheckSetStatus` path to Wait/Ground `10/4/0`.
- Forward and back roll branches still run the same stable callback and
  original animation-end handoff proof, and the DownStand branch still reaches
  Wait/Ground with valid callbacks.

Still deferred:

- Continuous DownAttack/DownForward/DownBack/DownStand recovery action runtime,
  hitboxes/damage, full natural knockdown recovery, natural ledge occupancy/
  release/drop/climb behavior, platform pass-through, and full gameplay remain
  deferred.

## 2026-06-26: Added selected live-callback CliffWait/CliffClimb/drop proof

What changed:

- Added direct/menu-chain harness modes `133/134`:
  `battle_mariofox_stage_mpclifflive_loop` and
  `menu_chain_mariofox_stage_mpclifflive_loop`.
- Extended the stage MP cliff-ledge boundary with a proof-owned selected P0
  `GObjProcess` callback path instead of only aggregate probe calls.
- Drove original CliffCatch -> CliffWait -> CliffQuick -> CliffClimbQuick1 ->
  CliffClimbQuick2 through the guarded live process, then ran one original
  common2 update/physics/map tick, the original common2 finish handoff into
  Wait/Ground, and a reseeded CliffWait drop into Fall/Air.
- Isolated the live proof's common2 collision-data bridge diagnostics so it
  does not pollute the older aggregate CliffClimb action bridge marker counts.
- Updated harness registry profiles, verifier wrappers, diagnostics, handoff,
  status, and harness docs for the new Latest/Boundary pair.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpclifflive-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpclifflive-loop-harness.ps1`
- `scripts/check-harness-registry.ps1`
- `scripts/check-docs.ps1`
- `scripts/check-architecture.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`

What is proven:

- Direct and menu-chain live cliff harnesses report selected `gcRunAll=33`,
  selected `gcDrawAll=7`, selected display callbacks `42/42`, and
  `final=Wait/Ground`.
- `STAGE_MPCLIFFLIVE` passes with result `0x464c5650`, safe result
  `0x464c5653`, mask `0x3ff`, deferred mask `0xff`, count `1`, and source
  mask `0xfff`.
- The live selected callback chain reaches CliffWait `85/73`, climbs through
  CliffQuick `86/74` into CliffClimbQuick2 `88/76`, finishes to Wait/Ground
  `10/4`, then drops from reseeded CliffWait into Fall/Air `26/20`.

Still deferred:

- Unbounded natural ledge runtime, real multiplayer ledge contention over many
  frames, ledge timing/invulnerability, attacks from ledge, broad platform/wall
  contracts, hitboxes/damage, and full gameplay remain deferred.

## 2026-06-26: Added Pupupu geometry-wide wall-hit scout diagnostics

What changed:

- Extended the existing direct/menu-chain `stage_mpwall_floor_loop` verifiers
  with `STAGE_MPWALL_HIT_SCOUT` markers.
- Added an isolated project-owned scout that scans every currently loaded
  Dream Land floor line for a non-edge wall candidate, then routes each
  candidate through the source-order `mpProcessRunFloorEdgeAdjust` path.
- Snapshot/restored the older MP-adjust and MP-edge diagnostic counters around
  the scout so historical wall-blocker assertions remain stable.
- Updated status, handoff, known-issues, diagnostic-reference, and next-boundary
  docs to record that the current Pupupu map asset is exhausted for this
  wall-hit proof.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpwall-floor-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpwall-floor-loop-harness.ps1`

What is proven:

- Direct and menu-chain wall-floor harnesses still report selected
  `gcRunAll=33`, selected `gcDrawAll=7`, selected display callbacks `42/42`,
  and `final=Wait/Ground`.
- The original wall-blocker proof remains intact: selected Dream Land line `3`
  still resolves edge-under walls `6/5`, records zero wall hits, zero adjust
  calls, and zero position deltas.
- The new geometry-wide scout reports `mpWallHitScout=none floors=4 walls=8
  candidates=6` with unsafe count `0`; no source-order non-edge wall-hit
  adjustment case exists in the currently staged Pupupu geometry.

Still deferred:

- A real wall-hit floor-edge-adjust proof now needs another original stage/map
  asset or collision case. Only `GRPupupuMap` is currently staged for the
  battle harness, so the next wall-hit step should narrowly stage or offline
  scan another original map before adding a direct/menu-chain wall-hit harness.

## 2026-06-26: Added Hyrule wall-hit scout candidate to the MP wall-floor proof

What changed:

- Extended the existing direct/menu-chain `stage_mpwall_floor_loop` verifiers
  with `STAGE_MPWALL_HYRULE_SCOUT`, `STAGE_MPWALL_HYRULE_SCOUT_LINE`, and
  `STAGE_MPWALL_HYRULE_SCOUT_POS` markers.
- Staged the minimal Hyrule Castle map dependency chain needed for the isolated
  scout: `GRHyruleMap`, `StageCastle`, and `ExternDataBank113`.
- Added reloc mappings for `llGRHyruleMapFileID`, `llGRHyruleMapMapHeader`,
  `llStageCastleFileID`, and `ll_113_FileID`.
- Kept the scout isolated from the live Pupupu fighter roots by saving and
  restoring MP collision globals and the older wall-hit scout counters.
- Avoided increasing ARM9 `.bss` after a first static-buffer attempt overflowed
  the DS ARM9 memory budget. The scout-only Hyrule external dependencies now
  reuse the existing aligned opening-action relocation scratch buffer during
  non-normal harness builds.

Proofs run:

- `make -j16`
- `scripts/verify-battle-mariofox-stage-mpwall-floor-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpwall-floor-loop-harness.ps1`

What is proven:

- The direct and menu-chain wall-floor harnesses still preserve the Dream Land
  blocker result: line `3`, edge-under wall `5`/`6`, zero selected wall hits,
  zero selected adjustments, and `mpWallHitScout=none floors=4 walls=8
  candidates=6`.
- The isolated Hyrule scout loads and relocates successfully with marker
  `0x4859524c`, nonzero floor/wall/candidate counts, unsafe count `0`, and a
  real source-order wall-hit candidate:
  `hyruleWallHit=hit floor=5 wall=13 edge=12 side=0 delta=-1600/-388`.

Still deferred:

- The Hyrule result is a scout, not a gameplay proof yet. The next wall-hit
  milestone should promote that exact Hyrule floor/wall pair into a bounded
  direct/menu-chain real wall-hit floor-edge-adjust proof without unbounding
  full map runtime, moving the live Pupupu fighter roots, or importing broad
  stage systems.

## 2026-06-26: Promoted Hyrule wall-hit scout to direct/menu-chain boundary

What changed:

- Added direct/menu-chain harness modes `135/136`:
  `battle_mariofox_stage_mpwallhit_floor_loop` and
  `menu_chain_mariofox_stage_mpwallhit_floor_loop`.
- Moved the Boundary/Latest registry profiles to the new wall-hit pair while
  keeping the previous `mpclifflive` pair in regression coverage.
- Made the new wall-hit modes inherit the current cliff-live proof and added
  wrapper assertions for the cliff-live marker group.
- Added dedicated `STAGE_MPWALLHIT_FLOOR*` verifier markers that promote the
  isolated Hyrule candidate into its own proof result while keeping the live
  VSBattle scene on Pupupu/Dream Land.
- Kept the older Dream Land wall-blocker evidence intact. In the combined
  wall-hit/cliff-live modes, the wall-hit proof records the Dream Land blocker
  diagnostics directly because the strict older MPWall result is finalized
  against early movement counters that the later cliff-live proof mutates.

Proofs run:

- `make -j16`
- `scripts/check-harness-registry.ps1`
- `scripts/check-docs.ps1`
- `scripts/check-architecture.ps1`
- `scripts/check-gbi-decode-fixtures.ps1`
- `scripts/verify-battle-mariofox-stage-mpwall-floor-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpwall-floor-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpclifflive-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpclifflive-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpedge-floor-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpedge-floor-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpadjust-floor-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpadjust-floor-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpcliffledge-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpcliffledge-loop-harness.ps1`
- `scripts/verify-battle-mariofox-stage-mpwallhit-floor-loop-harness.ps1`
- `scripts/verify-menu-chain-mariofox-stage-mpwallhit-floor-loop-harness.ps1`
- `scripts/verify-boundary.ps1`
- `scripts/verify-current.ps1`
- `scripts/verify-regression.ps1`

What is proven:

- The direct harness reaches Pupupu VSBattle from Maps; the menu-chain harness
  reaches the same battle boundary through VS Mode -> PlayersVS -> Maps.
- The current cliff-live proof still passes in both new modes with source mask
  `0xfff`.
- The new wall-hit proof validates Hyrule floor `5`, wall `13`, edge-under
  `12`, side `0`, wall kind `3`, final-floor-OK `1`, and adjustment delta
  `-1600/-388`.
- The old Dream Land wall-blocker path still records zero wall hits and no
  selected-floor adjustment, while the Hyrule proof remains isolated from full
  Hyrule stage runtime.

Still deferred:

- Full Hyrule VSBattle stage setup, live broad wall collision, platform
  pass-through, moving yakumono collision, attacks, items, HUD, audio, and
  unbounded gameplay remain deferred.
- A future live selected-callback wall-hit copyback proof should build on this
  Hyrule candidate without switching the live Pupupu scene or broadening map
  runtime.

## 2026-06-27: Promoted Hyrule wall-hit copyback to direct/menu-chain boundary

- Added direct/menu-chain harness modes `137/138`:
  `battle_mariofox_stage_mpwallcopy_floor_loop` and
  `menu_chain_mariofox_stage_mpwallcopy_floor_loop`.
- Promoted those modes to the Latest/Boundary verifier profiles while keeping
  the Hyrule wall-hit pair `135/136` in regression.
- Wired the new modes through the Makefile harness mapping,
  `scene_harness.c`, menu-chain taskman transition gates, bounded VSBattle
  taskman update/finalize flow, harness registry, registry drift checker, and
  direct/menu verifier wrappers.
- Fixed the partial wall-copy proof's prepare/finalize ordering: `Prepare()`
  no longer marks the proof unsafe before the Hyrule wall-hit finalizer has
  populated the prerequisite result.
- Extended the shared `verify-battle-mariofox-gcdrawall-loop-harness.ps1`
  script with `-RequireStageMPWallCopyFloor`, GDB markers
  `STAGE_MPWALLCOPY_*`, and assertions for proof result/safe result, process
  attach/run/callback/copyback counts, source line IDs, `-1600/-388` delta,
  P0 final floor/ground state, zero unsafe count, and unchanged P1 root.
- The proof keeps the live battle scene on Pupupu/Dream Land, consumes the
  promoted isolated Hyrule wall-hit candidate floor `5`, wall `13`,
  edge-under `12`, side `0`, installs a proof-owned selected P0 process, and
  copies the adjusted result back into the selected P0 root/collision shell.
- Verified:
  - `make -j16`
  - `scripts/check-harness-registry.ps1`
  - `scripts/verify-battle-mariofox-stage-mpwallcopy-floor-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mpwallcopy-floor-loop-harness.ps1`
- Deferred: full Hyrule stage runtime, arbitrary live wall collision, platform
  pass-through, full map runtime, items, HUD, audio, and unbounded gameplay.

## 2026-06-27: Scoped platform/pass-through into a pass-through floor scout

- Added `docs/MP_PASS_THROUGH_SCOUT.md` as the maintained scout for the next
  source-order map-collision boundary after the Hyrule wall-copy proof.
- Identified the original source route:
  `mpCommonCheckFighterPass` / `mpCommonRunFighterSpecialCollisions` use
  `MAP_PROC_TYPE_PASS` to call `mpProcessCheckTestFloorCollisionAdjNew`, whose
  acceptance gate depends on `MAP_VERTEX_COLL_PASS`, `ignore_line_id`, and the
  optional pass callback.
- Recorded the current project gap: project-owned headers do not yet define
  `MAP_VERTEX_COLL_PASS`, and the bounded MP floor-sweep helpers do not yet
  preserve pass-through floor semantics.
- Updated the next-boundary queue to target a direct/menu-chain pass-through
  floor proof before any moving yakumono/platform runtime work.

## 2026-06-27: Promoted pass-through floor proof to direct/menu-chain boundary

- Promoted direct/menu-chain harness modes `139/140`:
  `battle_mariofox_stage_mppass_floor_loop` and
  `menu_chain_mariofox_stage_mppass_floor_loop`.
- The proof consumes the Hyrule wall-hit and wall-copy proofs while keeping the
  live scene on Pupupu/Dream Land.
- The bounded pass-through slice selects Dream Land floor line `0`, verifies
  raw vertex flags `0x4000` carry `MAP_VERTEX_COLL_PASS`, routes through
  `MAP_PROC_TYPE_PASS` twice, rejects the same-line `ignore_line_id` probe,
  accepts the different-line probe, calls the pass callback once with one
  allow and zero denies, and proves P1 root state remains unchanged.
- Fixed the shared GDB verifier surface so `-RequireStageMPPassFloor` is an
  explicit switch with `STAGE_MPPASS_*` markers and assertions. Before this
  fix, PowerShell partial-parameter binding could treat that wrapper switch as
  `-RequireStageMPPassiveLoop`, which made the wrapper green without asserting
  the pass-through proof markers.
- Updated `docs/STATUS.md`, `docs/HANDOFF.md`,
  `docs/DIAGNOSTIC_REFERENCE.md`, `docs/KNOWN_ISSUES.md`,
  `docs/NEXT_BOUNDARY_QUEUE.md`, `docs/ROADMAP.md`, and
  `docs/MP_PASS_THROUGH_SCOUT.md` so modes `139/140` are the current
  Latest/Boundary target and modes `137/138` remain wall-copy regression
  coverage.
- Verified:
  - `make -j16`
  - `scripts/check-harness-registry.ps1`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `scripts/verify-battle-mariofox-stage-mppass-floor-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mppass-floor-loop-harness.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `scripts/verify-all.ps1 -Profile Regression -From battle_mariofox_stage_mpwallhit_floor_loop`
- `scripts/verify-regression.ps1` was also attempted, but the full regression
  wrapper timed out at the 20-minute command limit before returning progress;
  the focused regression resume above covers the changed wall-hit/wall-copy/pass
  boundary range.
- Deferred: moving yakumono/platform activation, natural drop-through input,
  full platform runtime, full Hyrule runtime, broad map collision, gameplay,
  HUD, audio, and unbounded taskman.

## 2026-06-27: Promoted platform existence scout to direct/menu-chain boundary

- Promoted direct/menu-chain harness modes `141/142`:
  `battle_mariofox_stage_mpplatform_floor_loop` and
  `menu_chain_mariofox_stage_mpplatform_floor_loop`.
- The proof consumes the pass-through floor proof, keeps the live scene on
  Pupupu/Dream Land, and calls original `mpCollisionCheckExistPlatformLineID`
  for Dream Land line `0`.
- Current diagnostic result is a precise blocker, not active platform runtime:
  line flags `0x4000`, yakumono id/count `1/1`, no active yakumono DObj,
  predicate `0`, blocker `0x30`.
- Added `STAGE_MPPLATFORM_*` GDB markers/assertions and updated the harness
  registry/checker so `141/142` are the Latest/Boundary targets while `139/140`
  remain pass-through regression coverage.
- Verified:
  - `make -j16`
  - `scripts/check-harness-registry.ps1`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `git diff --check`
  - `scripts/verify-battle-mariofox-stage-mpplatform-floor-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mpplatform-floor-loop-harness.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `scripts/verify-all.ps1 -Profile Regression -From battle_mariofox_stage_mppass_floor_loop`
- `scripts/verify-regression.ps1` was attempted but timed out at the 20-minute
  command limit before returning output. The focused regression tail above
  covers the pass-through/platform range changed by this task.
- Deferred: minimum original-compatible yakumono DObj activation, natural
  drop-through input, moving platform runtime, broad stage runtime, gameplay,
  HUD, audio, and unbounded taskman.

## 2026-06-27: Promoted active platform predicate proof to direct/menu-chain boundary

- Promoted direct/menu-chain harness modes `143/144`:
  `battle_mariofox_stage_mpplatform_active_floor_loop` and
  `menu_chain_mariofox_stage_mpplatform_active_floor_loop`.
- The proof keeps the live battle scene on Pupupu/Dream Land, consumes the
  Hyrule wall-hit, wall-copy, pass-through, and inactive platform blocker
  proofs, then installs a bounded original-compatible yakumono DObj for Dream
  Land line `0`.
- Updated the project-owned yakumono DObj shell to allow bounded indexed DObj
  slots and changed `mpCollisionCheckExistPlatformLineID` to use the original
  indexed predicate shape under bounds checks instead of the earlier id-zero
  shortcut.
- Current diagnostic result: line flags `0x4000`, yakumono id/count `1/1`,
  DObj present `1`, status `1`, anim `0`, predicate active, blocker `0`.
- Kept modes `141/142` as regression coverage for the inactive blocker
  (`dobj=1`, `status=0`, predicate `0`, blocker `0x40`) and moved
  Latest/Boundary registry targets to `143/144`.
- Verified:
  - `make -j16`
  - `scripts/check-harness-registry.ps1`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `git diff --check`
  - `scripts/verify-battle-mariofox-stage-mpplatform-active-floor-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mpplatform-active-floor-loop-harness.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `scripts/verify-battle-mariofox-stage-mpplatform-floor-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mpplatform-floor-loop-harness.ps1`
- Deferred: original platform/yakumono update runtime, natural drop-through
  input, moving platform runtime, broad stage runtime, gameplay, HUD, audio,
  and unbounded taskman.

## 2026-06-27: Promoted platform status/update-tic proof to direct/menu-chain boundary

- Promoted direct/menu-chain harness modes `145/146`:
  `battle_mariofox_stage_mpplatform_tick_floor_loop` and
  `menu_chain_mariofox_stage_mpplatform_tick_floor_loop`.
- The proof keeps the live battle scene on Pupupu/Dream Land, consumes the
  Hyrule wall-hit, wall-copy, pass-through, inactive platform blocker, and
  active platform predicate proofs, then routes Dream Land line `0` /
  yakumono id `1` through the project-owned `mpCollisionSetYakumonoOnID`
  helper and one guarded original-compatible `mpCollisionAdvanceUpdateTic`
  call.
- Current diagnostic result: line flags `0x4000`, yakumono id/count `1/1`,
  DObj present `1`, status `1 -> 1`, predicate active, update tic `0 -> 1`,
  set-on count `1`, advance count `1`, and unsafe count `0`.
- Kept modes `143/144` as regression coverage for the active platform
  predicate and moved Latest/Boundary registry targets to `145/146`.
- Normalized local `C:/devkitPro` / `C:/devkitPro/devkitARM` Makefile paths
  to MSYS-style `/c/...` before including devkitARM rules, and added a narrow
  post-compile dependency-file sanitizer that rewrites malformed
  `C:devkitPro/...` entries to `C:/devkitPro/...`. This prevents generated
  harness `.d` files from breaking the next incremental verifier run.
- Verified:
  - `make -j16`
  - `scripts/check-harness-registry.ps1`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `scripts/clean-generated.ps1 -DryRun`
  - `git diff --check`
  - `scripts/verify-battle-mariofox-stage-mpplatform-tick-floor-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mpplatform-tick-floor-loop-harness.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `scripts/verify-title-harness.ps1` after regenerating and reparsing
    `build-title-harness/main.d`
- Deferred: natural drop-through input, real moving platform motion/runtime,
  broad stage runtime, gameplay, HUD, audio, and unbounded taskman.

## 2026-06-27: Promoted natural drop-through input proof to direct/menu-chain boundary

- Promoted direct/menu-chain harness modes `147/148`:
  `battle_mariofox_stage_mppass_input_loop` and
  `menu_chain_mariofox_stage_mppass_input_loop`.
- Imported original `ftcommonpass.c` and `ftcommonsquat.c` through
  `src/import/battleship_ftcommon_pass.c`, exposing only the bounded
  pass/squat input and status path needed for this proof.
- The proof keeps the live battle scene on Pupupu/Dream Land, consumes the
  Hyrule wall-hit, wall-copy, pass-through, inactive platform, active platform,
  and platform-tick proofs, then seeds original-compatible down input on Dream
  Land pass-through line `0`.
- Current diagnostic result:
  `mpPassInput=line=0 flags=0x4000 squat=28 pass=33 ignore=0`, with tap Y
  `0 -> 254`, pass wait `3 -> 0`, `mpCommonSetFighterAir` and
  `ftPhysicsClampAirVelXMax` reached once, and unsafe count `0`.
- Kept modes `145/146` as regression coverage for platform status/update-tic
  behavior and moved Latest/Boundary registry targets to `147/148`.
- Verified:
  - `make -j16`
  - `scripts/check-harness-registry.ps1`
  - `scripts/verify-battle-mariofox-stage-mppass-input-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mppass-input-loop-harness.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
- Deferred: real moving platform motion/runtime, broad stage runtime, full
  player-driven drop-through runtime, gameplay, HUD, audio, and unbounded
  taskman.

## 2026-06-27: Promoted yakumono position/speed primitive proof to direct/menu-chain boundary

- Promoted direct/menu-chain harness modes `149/150`:
  `battle_mariofox_stage_mpplatform_pos_floor_loop` and
  `menu_chain_mariofox_stage_mpplatform_pos_floor_loop`.
- Added the narrow project-owned `mpCollisionSetYakumonoPosID` compatibility
  seam with original BattleShip semantics from `mpcollision.c`: compute
  `gMPCollisionSpeeds[id]` as target minus current DObj translation, then
  update the DObj translation.
- Added bounded yakumono speed storage beside the existing fixed yakumono DObj
  compatibility shell. This does not import or enable full yakumono animation
  runtime.
- The proof consumes the pass-input boundary, runs on Dream Land line `0` /
  yakumono `1`, and records DObj status `1 -> 1`, platform predicate `1`, and
  speed delta `12000/-4000/2000` with unsafe count `0`.
- Verified:
  - `make -j16`
  - `scripts/check-harness-registry.ps1`
  - `scripts/verify-battle-mariofox-stage-mpplatform-pos-floor-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mpplatform-pos-floor-loop-harness.ps1`

## 2026-06-27 - MP Platform Speed Reader Proof

- Added direct/menu-chain modes `151/152`:
  `battle_mariofox_stage_mpplatform_speed_floor_loop` and
  `menu_chain_mariofox_stage_mpplatform_speed_floor_loop`.
- Replaced the zero-only `mpCollisionGetSpeedLineID` seam with a bounded
  original-compatible line-to-yakumono speed lookup. Invalid cases remain
  bounded instead of entering the original debug infinite loop.
- The proof consumes the existing position proof for Dream Land line `0` /
  yakumono `1`, then reads the speed through `mpCollisionGetSpeedLineID` and
  verifies `12000/-4000/2000`.
- Added verifier markers `STAGE_MPPLATFORM_SPEED`,
  `STAGE_MPPLATFORM_SPEED_SETUP`, and `STAGE_MPPLATFORM_SPEED_VEC`.
- Verified:
  - `scripts/verify-battle-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1`
- Deferred: real moving platform animation runtime, platform-speed collision
  consumption by broader map queries, broad stage runtime, gameplay, HUD,
  audio, and unbounded taskman.

## 2026-06-27 - CliffWait/CliffMotion Queue Shell Regression Fix

- Restored the original no-stick `ftCommonCliffClimbOrFallCheckInterruptCommon`
  side effect in the bounded CliffWait proof path so `is_allow_interrupt`
  becomes true before the follow-up CliffAttack/CliffEscape probes.
- Kept the original BattleShip CliffAttack and CliffEscape interrupt calls in
  path, then normalized the bounded `FTStruct` shell's `cliffmotion` queue
  fields after true returns. This preserves the source-order postcondition
  needed by the direct/menu-chain CliffAttack and CliffEscape action proofs.
- Verified:
  - `scripts/verify-battle-mariofox-stage-mpcliffattack-floor-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mpcliffattack-floor-loop-harness.ps1`
  - `scripts/verify-battle-mariofox-stage-mpcliffescape-action-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mpcliffescape-action-loop-harness.ps1`
  - `scripts/verify-all.ps1 -Profile Regression -From battle_mariofox_stage_mpcliffescape_action_loop`
- Deferred: replacing the large proof-owned `FTStruct` compatibility shell with
  a smaller shared original-status helper.

## 2026-06-27 - MP Platform Dynamic Floor Speed Consumer

- Extended the project-owned floor diff sweep helper to apply the original
  active-yakumono local-space transform from `mpCollisionCheckFloorLineCollisionDiff`:
  previous position is offset by `gMPCollisionSpeeds[yakumono_id]`, current
  position is measured relative to the yakumono DObj, and the hit position is
  returned to world space.
- Added one controlled platform-speed proof probe on Dream Land line `0` /
  yakumono `1` after `mpCollisionGetSpeedLineID` reads `12000/-4000/2000`.
  The new diagnostic marker is `STAGE_MPPLATFORM_SPEED_DYNAMIC`, summarized as
  `dyn=1/2`.
- Kept modes `151/152` as the current boundary rather than adding another
  harness pair.
- Verified:
  - `make -j16`
  - `scripts/verify-dev-fast.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `scripts/verify-regression.ps1`
- Deferred: ceil/wall dynamic diff sweeps and real
  `mpCollisionPlayYakumonoAnim` animation-driven movement.

## 2026-06-27 - MP Ceil Dynamic Transform Alignment

- Extended the bounded project-owned ceil same/diff sweep helper to use the
  original active-yakumono local-space transform from
  `mpCollisionCheckCeilLineCollisionDiff`: previous position is offset by
  `gMPCollisionSpeeds[yakumono_id]`, current position is measured relative to
  the yakumono DObj, and hit output is converted back to world space.
- Kept modes `151/152` as the current boundary. This is source-order helper
  alignment, not a new dynamic ceil asset proof.
- Verified:
  - `make -j16`
  - `scripts/check-harness-registry.ps1`
  - `scripts/verify-battle-mariofox-stage-mpceil-floor-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-mpceil-floor-loop-harness.ps1`
  - `scripts/verify-dev-fast.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `scripts/verify-regression.ps1`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
- Deferred: a controlled dynamic ceil candidate marker, wall diff wrappers, and
  real `mpCollisionPlayYakumonoAnim` animation-driven movement.

## 2026-06-27 - MP Platform Dynamic Ceil Speed Consumer

- Extended the existing platform-speed proof for modes `151/152` with one
  bounded same-yakumono Dream Land ceiling probe. The probe selects a ceiling
  line owned by the same active yakumono as floor line `0`, then calls the
  project-owned original-compatible `mpCollisionCheckCeilLineCollisionDiff`
  path after `mpCollisionGetSpeedLineID` has proven the `12000/-4000/2000`
  speed vector.
- Added diagnostic marker `STAGE_MPPLATFORM_SPEED_DYNAMIC_CEIL`, summarized by
  the verifier as `ceil=1/1`. The platform-speed result mask is now `0x3ff`
  so the current boundary requires both the dynamic floor and dynamic ceiling
  speed-consumer probes.
- Kept the work inside the existing direct/menu-chain platform-speed harness
  pair rather than adding another harness boundary.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `scripts/verify-regression.ps1`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `git diff --check`
- Deferred: wall diff caller coverage and real
  `mpCollisionPlayYakumonoAnim` animation-driven movement.

## 2026-06-27 - MP Platform Bounded Yakumono Animation Playback

- Extended the existing platform-speed proof for modes `151/152` with one
  bounded `mpCollisionPlayYakumonoAnim` slice. The proof seeds controlled
  DObj animation tracks on Dream Land line `0` / yakumono `1`, then runs
  original `gcParseDObjAnimJoint` and `gcPlayDObjAnimJoint` through the gated
  compatibility hook.
- Added diagnostics for the bounded animation tick:
  `STAGE_MPPLATFORM_SPEED_ANIM` records one playback call, MP update tic
  `+1`, status `2 -> 2`, and speed `12000/-4000/2000`. The platform-speed
  result mask is now `0x7ff`, and the verifier summary includes `anim=1`.
- Kept the old deferred behavior outside the platform-speed proof so the
  change does not accidentally activate broad moving-platform runtime.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `scripts/verify-regression.ps1`
- Deferred: wall diff caller coverage, real stage-authored yakumono animation
  scripts, and MP bounds recompute.

## 2026-06-27 - MP Platform Dynamic Wall Speed Consumer

- Extended the existing platform-speed proof for modes `151/152` with
  project-owned `mpCollisionCheckLWallLineCollisionDiff` and
  `mpCollisionCheckRWallLineCollisionDiff` wrappers. They share the bounded
  wall sweep helper and apply the original active-yakumono local-space
  transform before converting the hit back to world space.
- Added one controlled same-yakumono Dream Land wall probe after the
  `mpCollisionGetSpeedLineID` proof. The new diagnostic marker
  `STAGE_MPPLATFORM_SPEED_DYNAMIC_WALL` records probe/hit counts, selected wall
  line, yakumono ID, wall kind, and hit coordinates; the verifier summary now
  includes `wall=1/1`.
- Kept the work inside the existing direct/menu-chain platform-speed harness
  pair instead of adding a new boundary. The platform-speed result mask is now
  `0xfff`, requiring floor, ceiling, wall, and animation speed-consumer proofs.
- Verified:
  - `scripts/verify-battle-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `scripts/verify-regression.ps1`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `scripts/clean-generated.ps1 -DryRun`
  - `git diff --check`
- Deferred: real stage-authored yakumono animation scripts, MP bounds
  recompute, and original live MP wall-diff caller coverage.

## 2026-06-27 - MP Platform Bounded Wall-Process Caller

- Extended the existing platform-speed proof for modes `151/152` with one
  bounded first-probe slice of original
  `mpProcessCheckTestL/RWallCollisionAdjNew`. The proof seeds a contained
  `MPCollData`, forces the original source-order `update_tic !=
  gMPCollisionUpdateTic` branch, and routes through the existing dynamic wall
  diff seam without importing full `mpprocess.c`.
- Added `STAGE_MPPLATFORM_SPEED_PROCESS_WALL`, summarized by the verifier as
  `procwall=1/1`. The platform-speed result mask is now `0x1fff`.
- Kept the work inside the existing direct/menu-chain platform-speed harness
  pair. The original edge/ceil/floor follow-up branches of the wall-adjust
  functions remain deferred until a verifier needs them.
- Verified:
  - `make -j16`
  - `scripts/verify-dev-fast.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - PowerShell parser sweep over `scripts/**/*.ps1`
  - `scripts/clean-generated.ps1 -DryRun`
  - `git diff --check`
- Deferred: real stage-authored yakumono animation scripts, full moving
  platform runtime, and MP bounds recompute.

## 2026-06-27 - MP Platform Bounds Recompute Slice

- Extended the existing platform-speed proof for modes `151/152` with
  original-compatible MP bounds current/diff recompute after the bounded
  yakumono animation tick. This adds the narrow `MPBounds` / `MPAllBounds`
  surface and `gMPCollisionBounds` in project-owned compatibility code.
- Replaced the remaining platform-speed proof's bounded clear/init/bounds
  seams with guarded source-order behavior: `mpCollisionClearYakumonoAll`
  clears yakumono DObj statuses and speeds, `mpCollisionInitYakumonoAll`
  scans the decoded Dream Land collision geometry, and
  `mpCollisionUpdateBoundsCurrent` / `mpCollisionUpdateBoundsDiff` compute the
  current-vs-start stage bounds after the gated animation playback.
- Added `STAGE_MPPLATFORM_SPEED_BOUNDS`, summarized by the verifier as
  `bounds=1`. The platform-speed result mask is now `0x3fff`.
- Kept the work inside the existing direct/menu-chain platform-speed harness
  pair. The proof still uses a controlled seeded animation track; real
  stage-authored yakumono scripts and full moving-platform runtime remain
  deferred.
- Verified:
  - `make -j16`
  - PowerShell parser sweep over `scripts/**/*.ps1`
  - `scripts/verify-dev-fast.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`

## 2026-06-27 - Pupupu Stage-Authored Platform Animation Blocker Diagnostic

- Added `STAGE_MPPLATFORM_SPEED_STAGE_ANIM` to the existing platform-speed
  proof for modes `151/152` without changing the pass mask. The diagnostic
  records the layer animation mask, DObj/MObj authored-animation counts, and
  bounded layer callback count.
- Confirmed the selected Dream Land line `0` / yakumono `1` path still passes
  the speed-consumer boundary but is not a real authored Pupupu layer-1
  animation source. Both direct and menu-chain verifiers record
  `stageanim=0/0x91`: layer/root and selected yakumono parent are present and
  the bounded callback ran once, but there is no authored layer-1 animation
  table/process on that path.
- Updated the handoff/status/scout docs to treat this as a precise blocker,
  not a completed moving-platform subsystem. The platform-speed result mask
  remains `0x3fff`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1 -DelaySeconds 1`
  - `scripts/verify-menu-chain-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - PowerShell parser sweep over `scripts/**/*.ps1`
  - `scripts/clean-generated.ps1 -DryRun`
  - `git diff --check`
- Deferred: scout another original stage/yakumono source path with authored
  motion data, such as Yoster, Sector, or Yamabuki, before importing
  moving-platform runtime.

## 2026-06-27 - Moving-Yakumono Source Scout

- Inspected read-only BattleShip stage sources after the Dream Land
  `stageanim=0/0x91` blocker. The smallest real moving-yakumono candidate is
  Inishie/Mushroom Kingdom scales: `grInishieScaleProcUpdate` moves two scale
  platform DObjs and calls `mpCollisionSetYakumonoPosID` for both scale line
  groups every update.
- Ranked other candidates behind Inishie for the first proof: Yoster clouds
  need fighter stand checks/material animation/particle state, Yamabuki gates
  pull in item/monster gate runtime, and Sector Arwing collision pulls in
  Arwing animation plus weapon/laser state.
- Updated current docs so the next code boundary is an Inishie scale-platform
  harness pair, not another Dream Land seeded-motion proof.
- Verified:
  - source inspection only; no code boundary changed.
- Deferred: `GRInishieMap` NitroFS staging, narrow `GRStruct.inishie` fields,
  and bounded import/proof harnesses for direct + menu-chain Inishie scale
  update.

## 2026-06-27 - Inishie Map Header Dependency Preflight

- Added `GRInishieMap` to the staged NitroFS reloc assets and wired its file
  ID / `llGRInishieMapMapHeader` symbol through the project-owned relocation
  table.
- Added bounded diagnostics for the existing platform-speed proof modes
  `151/152`. The probe runs after the current Dream Land speed proof so the
  dependency scout cannot disturb the active Pupupu battle scene.
- Verified the marker `STAGE_INISHIE_ASSET=0x494e4952,0x14,1,1,1,0,0`,
  summarized by both verifiers as `inishieAsset=header/geometry nodes=1`.
  This proves the Inishie map header path is reachable; it intentionally does
  not import Inishie scale platform GObjs or run `grInishieScaleProcUpdate`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1 -DelaySeconds 1`
  - `scripts/verify-menu-chain-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1`
- Deferred: narrow `GRStruct.inishie` fields, bounded Inishie scale GObj setup,
  stage-data dependency handling beyond the map header, and one original
  `grInishieScaleProcUpdate` tick proving two `mpCollisionSetYakumonoPosID`
  calls.

## 2026-06-27 - Inishie Scale Update Proof

- Added direct/menu-chain modes `153/154`:
  `battle_mariofox_stage_inishie_scale_loop` and
  `menu_chain_mariofox_stage_inishie_scale_loop`.
- Imported a bounded original Inishie/Mushroom Kingdom scale update slice in
  `src/import/battleship_grinishie_scale.c`, centered on
  `grInishieScaleProcUpdate`.
- Added a narrow proof shell that creates two scale platform DObjs, two string
  DObjs, and separate collision yakumono DObjs so
  `mpCollisionSetYakumonoPosID` computes the original speed delta while the
  live VSBattle roots remain on Pupupu/Dream Land.
- The proof inherits the previous pass-through/platform/platform-speed,
  dynamic floor/ceil/wall, process-wall, yakumono animation, bounds, and
  Inishie map-header markers, then records line groups `1/2`, map object kinds
  `5/6`, altitude `80000 -> 72000`, Y
  `363000/362000 -> 435000/290000`, and speed `72000/-72000`.
- Updated the Boundary/Latest registry target to the new pair while keeping
  modes `151/152` as regression coverage for the platform-speed dependency.
- Tightened the shared verifier build path so `verify-all.ps1 -Build` and
  `build-verify-profile.ps1` force `TARGET=smash64ds BUILD=build
  NDS_DEV_SCENE_HARNESS=normal -B`. This prevents runtime verification from
  sampling a stale harness-flavored `smash64ds.nds` when the shared build
  directory previously held a harness object.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1 -DelaySeconds 1`
  - `scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1 -DelaySeconds 5`
  - `scripts/verify-boundary.ps1`
  - `scripts/check-harness-registry.ps1`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `scripts/check-gbi-decode-fixtures.ps1`
  - `scripts/clean-generated.ps1 -DryRun`
  - `git diff --check`
  - PowerShell parser sweep over `scripts/**/*.ps1`
  - `scripts/verify-current.ps1 -Build`
- Deferred: replace more of the proof shell with original
  `grInishieMakeScale` setup by staging scale DObj/model data behind
  `llGRInishieMapScaleDObjDesc` and `llGRInishieMapMapHead`; full Mushroom
  Kingdom stage runtime, Pakkun, Power Block, item/monster runtime, and broad
  map collision remain out of scope.

## 2026-06-27 - Inishie Scale Setup Guard and Asset Blocker

- Inspected original BattleShip `grInishieMakeScale` and
  `grModelSetupGroundDObjs` as the next step after the scale update proof.
  The original setup needs map-relative scale model offsets
  `llGRInishieMapScaleDObjDesc = 0x380`,
  `llGRInishieMapMapHead = 0x5f0`, and
  `llGRInishieMapScaleRetractAnimJoint = 0x734`.
- Confirmed the currently staged `GRInishieMap` O2R asset only carries the
  header/geometry payload needed by the existing dependency proof: `368` bytes
  of data in a `476` byte staged file. Calling original `grInishieMakeScale`
  against that payload reads past the staged data and crashes before the scale
  proof shell records diagnostics.
- Added Makefile variable `NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP`, defaulting
  off, so the source setup and map-object reader expansion stay available as a
  documented guarded path without entering the active ROM until the required
  `StageInishieFile2/3` scale model/display-list data is staged.
- Kept the direct/menu-chain scale update proof on the existing bounded shell,
  preserving the verified original `grInishieScaleProcUpdate` tick and
  `mpCollisionSetYakumonoPosID` speed diagnostics.
- Reduced opening-action preview cache reservation from three entries to one
  for fighter/stage harness modes `>= 11`. This keeps menu-chain stage
  harnesses under the DS ARM9 memory limit while leaving normal boot/opening
  builds on the original three-entry cache.
- Verified:
  - `scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1 -Build -DelaySeconds 1`
  - `scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1 -Build -DelaySeconds 5`
- Deferred: stage/import the narrow `StageInishieFile2/3` scale
  DObj/model/display-list data, then build with
  `NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1` to re-enable the guarded original
  `grInishieMakeScale` setup for the same bounded verifier pair.

## 2026-06-27 - Guarded Inishie Source Setup Proof

- Added a guarded `NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1` path that runs
  original `grInishieMakeScale` instead of the default proof-shell-only setup.
- Because the generated `StageInishieFile2/3` payloads are absent from the
  read-only decomp checkout, the guarded path uses a minimal project-owned,
  offset-compatible source setup shim for `llGRInishieMapScaleDObjDesc`
  (`0x380`), `llGRInishieMapMapHead` (`0x5f0`), and
  `llGRInishieMapScaleRetractAnimJoint` (`0x734`). It supplies only the
  bounded scale DObj/display-list contracts needed by `grInishieMakeScale`.
- Added a source-setup map-object guard so original `grInishieMakeScale` sees
  deterministic ScaleL/ScaleR IDs and positions without dereferencing the
  incomplete `GRInishieMap` geometry `mapobjs` pointer.
- Isolated the guarded source setup from older platform-position/tick
  diagnostics so `mpCollisionSetYakumonoOnID` calls inside source setup do not
  pollute the previous Dream Land platform proof markers.
- Verified default direct/menu-chain Inishie scale proof remains green, and
  opt-in direct/menu-chain source setup builds also pass the same verifier
  boundary.
- Verified:
  - `make TARGET=smash64ds-battle-mariofox-stage-inishie-scale-loop BUILD=build-battle-mariofox-stage-inishie-scale-loop-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_inishie_scale_loop NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1 -B -j16`
  - `scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1 -NoBuild`
  - `make TARGET=smash64ds-battle-mariofox-stage-inishie-scale-loop BUILD=build-battle-mariofox-stage-inishie-scale-loop-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_inishie_scale_loop -B -j16`
  - `scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1 -NoBuild`
  - `scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1`
  - `make TARGET=smash64ds-menu-chain-mariofox-stage-inishie-scale-loop BUILD=build-menu-chain-mariofox-stage-inishie-scale-loop-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_inishie_scale_loop NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1 -B -j16`
  - `scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1 -NoBuild`
- Deferred: replace the offset-compatible shim with narrow original
  `StageInishieFile2/3` scale visual/model data and then decide whether the
  source setup path should become the default current-boundary build.

## 2026-06-27 - Inishie Source Shim Data Tightening

- Confirmed the read-only decomp context has typed `StageInishieFile3` source
  data, but the generated include fragments and compiled O2R payload for
  `StageInishieFile3` are absent from the current checkout.
- Replaced the guarded source shim's zero display-list word with a real
  `G_ENDDL` command and copied the original typed ScaleRetract anim words into
  the offset-compatible `0x734` slot.
- Verified:
  - `make TARGET=smash64ds-battle-mariofox-stage-inishie-scale-loop BUILD=build-battle-mariofox-stage-inishie-scale-loop-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_inishie_scale_loop NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1 -j16`
  - `scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1 -NoBuild`
  - `make TARGET=smash64ds-menu-chain-mariofox-stage-inishie-scale-loop BUILD=build-menu-chain-mariofox-stage-inishie-scale-loop-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_inishie_scale_loop NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1 -j16`
  - `scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1 -NoBuild`
- Deferred: real `StageInishieFile2/3` visual/model/display-list payload
  staging remains required before promoting source setup to default.

## 2026-06-27 - Inishie StageInishieFile3 Raw Asset Proof

- Added opt-in NitroFS staging for read-only
  `decomp/BattleShip-main/decomp/assets/us/relocData/155.vpk0.bin` when
  `NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1`.
- Updated the guarded source setup shim to load the `5136` byte raw
  `StageInishieFile3` payload, validate the DObjDesc, map-head display-list,
  and ScaleRetract offsets, and convert the scale DObjDesc scalar fields from
  big-endian raw data before calling original `grInishieMakeScale`.
- Kept active display-list pointers bounded at `G_ENDDL`. The raw display-list
  pointer words are relocation-chain encoded, so a narrow pointer/material/DL
  fixup pass is still required before using the real scale visual data.
- Added verifier coverage for `STAGE_INISHIE_SCALE_SOURCE` raw-source readiness
  bits when the source setup reaches step `13`.
- Verified:
  - `make TARGET=smash64ds-battle-mariofox-stage-inishie-scale-loop BUILD=build-battle-mariofox-stage-inishie-scale-loop-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_inishie_scale_loop NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1 -j16`
  - `scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1 -NoBuild`
  - `make TARGET=smash64ds-menu-chain-mariofox-stage-inishie-scale-loop BUILD=build-menu-chain-mariofox-stage-inishie-scale-loop-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_inishie_scale_loop NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1 -j16`
  - `scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1 -NoBuild`
- Deferred: pointer/material/display-list relocation for the staged raw
  `StageInishieFile3` scale payload, then deciding whether to promote the
  source setup path to the default boundary.

## 2026-06-27 - Inishie Scale Native DL Pointer Proof

- Kept the source setup scoped to `StageInishieFile3` scale data only.
- Converted the narrow scale Vtx arrays and DL arrays from the staged raw
  payload into native word order for the guarded source path.
- Patched the known DObj and map-head DL pointers used by original
  `grInishieMakeScale`, then scanned the converted DLs with the shared NDS
  renderer scanner before exposing them to the DObjDesc/map-head offsets.
- Tightened `STAGE_INISHIE_SCALE_SOURCE` verification to require source bits
  `8..16`, covering raw load/validation plus native DL scan and pointer fixup.
- Verified:
  - `make TARGET=smash64ds-battle-mariofox-stage-inishie-scale-loop BUILD=build-battle-mariofox-stage-inishie-scale-loop-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_inishie_scale_loop NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1 -j16`
  - `scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1 -NoBuild`
  - `make TARGET=smash64ds-menu-chain-mariofox-stage-inishie-scale-loop BUILD=build-menu-chain-mariofox-stage-inishie-scale-loop-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_stage_inishie_scale_loop NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1 -j16`
  - `scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1 -NoBuild`
- Deferred: texture/material handling and a bounded display traversal/renderer
  proof for the source-enabled Inishie scale GObjs.

## 2026-06-27 - Inishie Source DObj Display Scan Proof

- Kept the source setup guarded behind `NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1`
  and did not add a new harness mode.
- Added source-only diagnostics that prove the four original-created Inishie
  scale DObjs have the expected BattleShip display callbacks and scan cleanly
  through `ndsRendererScanDisplayList`.
- Extended the shared Inishie scale verifier with
  `STAGE_INISHIE_SCALE_DISPLAY`; current source builds report
  `sourceDL=0xff cmds=91 tris=20`.
- Verified source-enabled direct/menu-chain builds with forced rebuilds, then
  rebuilt default direct/menu-chain harnesses without the source flag and
  re-ran both default no-build verifiers.
- Deferred: texture/material handling and actual visible Mushroom Kingdom scale
  rendering.

## 2026-06-27 - Inishie Source Setup Default Boundary

- Promoted the source-backed Inishie scale setup to the default build path for
  the current boundary pair only: `battle_mariofox_stage_inishie_scale_loop`
  and `menu_chain_mariofox_stage_inishie_scale_loop`.
- Kept normal boot and older harnesses on the non-source default by deriving
  `NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP` from `NDS_DEV_SCENE_HARNESS`.
- Moved the current Inishie wrappers and registry entries to separate
  `*-source-harness` build directories so source and non-source objects are not
  mixed by incremental builds.
- Updated current docs so the next boundary is the smallest texture/material
  visibility proof for the source-created Inishie scale, not another setup
  reachability decision.
- Verified:
  - `scripts/check-harness-registry.ps1`
  - `scripts/check-docs.ps1`
  - `scripts/verify-boundary.ps1`
  - `scripts/verify-current.ps1`
  - `make -j16`
  - `scripts/check-architecture.ps1`
  - `scripts/check-gbi-decode-fixtures.ps1`
  - `git diff --check`
- Current direct and menu-chain summaries both report
  `sourceDL=0xff cmds=91 tris=20`.
- Deferred: texture/material handling and actual visible Mushroom Kingdom scale
  rendering.

## 2026-06-27 - Inishie Source DL Material and Preview Proof

- Kept the current `153/154` Inishie scale boundary and did not import broader
  Mushroom Kingdom runtime.
- Added source-DL material diagnostics for the four original-created scale
  DObjs. The verifier now proves texture/material command state from the
  original display-list stream with `tex=0x3f`.
- Added a bounded software source-DL visibility preview for the same DObjs. It
  decodes the patched source Vtx/DL pointers, commits the original-DL preview,
  and records `preview=0x3f px=432`.
- Kept full DS hardware texture upload/material application deferred; this is a
  bounded visibility/material proof, not a full renderer rewrite.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1 -DelaySeconds 1`
  - `scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1`

## 2026-06-27 - Inishie Source Scale Two-Tick Proof

- Kept the existing `153/154` Inishie scale harness pair; no new harness modes
  or broader Mushroom Kingdom runtime were added.
- Extended the bounded proof from one original `grInishieScaleProcUpdate` tick
  to two ticks. The same source-created scale platform/string DObjs now prove
  four `mpCollisionSetYakumonoPosID` calls.
- Updated verifier expectations to require `ticks=2`,
  `alt=80000->64000`, platform Y `363000/362000->427000/298000`, and
  second-tick speed `-8000/8000`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1`

## 2026-06-27 - Inishie Scale Threshold Fall/Sleep Proof

- Kept the existing `153/154` Inishie scale harness pair and did not broaden
  the stage runtime.
- Added a forced threshold probe after the normal two bounded Wait ticks. The
  probe reseeds original `gGRCommonStruct.inishie` state, calls original
  `grInishieScaleProcUpdate`, proves the Wait threshold transition into Fall,
  then calls the original update once more and records the bounded
  Fall-to-Sleep result under the current proof-owned ground/deadzone setup.
- Added `STAGE_INISHIE_SCALE_FALL` diagnostics and verifier checks. The current
  direct/menu-chain summaries report `fall=1->2/0`, with two sparkle calls,
  four fall-phase `mpCollisionSetYakumonoPosID` writes, final platform Y
  `1460000/-741000`, and fall speed `-3000/-3000`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1`
- Deferred: continuous Mushroom Kingdom scale runtime, Pakkun/Power Block/item
  runtime, real hardware texture/material upload, and unbounded stage
  scheduling.

## 2026-06-27 - Inishie Scale Sleep/Retract Branch Proof

- Kept the existing `153/154` Inishie scale harness pair; no new harness modes
  or broader Mushroom Kingdom runtime were added.
- Extended the bounded threshold probe after the Fall/Sleep proof by reseeding
  original `gGRCommonStruct.inishie` to Sleep with `splat_wait = 1`, then
  calling original `grInishieScaleProcUpdate` through the Sleep countdown and
  Retract branch.
- Added `STAGE_INISHIE_SCALE_STEP` diagnostics and verifier checks. The current
  direct/menu-chain summaries report `step=3->0/0`, with four retract-phase
  `mpCollisionSetYakumonoPosID` writes, two platform re-enable calls, retracted
  altitude `0`, restored platform Y `363000/362000`, and final retract speeds
  `-1097000/1103000`.
- Updated the Inishie scale pass mask from `0x3ff` to `0x7ff` for the new
  bounded Sleep/Retract bit.
- Verified:
  - `make -j16`
  - `make -B -j16`
  - `scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1`
  - `scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1`
- Deferred: continuous Mushroom Kingdom scale runtime, Pakkun/Power Block/item
  runtime, real hardware texture/material upload, and unbounded stage
  scheduling.

## 2026-06-27 - PassiveStand/Passive Recover Loop Proof

- Added direct/menu-chain modes `155/156`:
  - `battle_mariofox_stage_mppassive_recover_loop`
  - `menu_chain_mariofox_stage_mppassive_recover_loop`
- Kept the live battle scene on Pupupu/Dream Land and did not import broad
  fighter gameplay, full `ftmain.c`, item/runtime/HUD/audio, or broader map
  collision.
- Reused the existing source-order MP floor, cliff, DamageFall,
  PassiveStand/Passive setup, and bounded Passive-loop proof chain. The new
  modes extend PassiveStandF/Ground and Passive/Ground from two stable frames
  to five guarded stable update/physics/map frames.
- Kept the final transition source-authored: both branches still hand off to
  Wait/Ground through original `ftAnimEndSetWait`. A follow-up entry below
  extends the same bounded handoff proof to PassiveStandB/Ground.
- Added wrapper verifiers and registry placement for Boundary/Latest profiles.
  The menu-chain path needs `-DelaySeconds 3` so GDB captures the post-loop
  finalizer markers after VS Mode -> PlayersVS -> Maps -> VSBattle.
- Current summary markers:
  - `mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3`
  - `mpCliffWaitDamage=status=85->57 motion=73->50 fallWait=1->0 catch=30`
  - `passiveStand=73/62/0->cb1/1->10/4/0`
  - `passive=81/70/0->cb1/1->10/4/0`
  - `mpPassive=ps=73/62->10/4 ticks=6/5/5 passive=81/70->10/4 ticks=6/5/5`
- Verified:
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`
  - `scripts/verify-current.ps1 -DelaySeconds 3`
- Deferred: natural collision-driven recovery selection, arbitrary recovery
  durations, hitlag/damage interaction, full player-driven recovery runtime,
  and broader fighter gameplay.

## 2026-06-27 - Passive Recovery Branch Matrix Proof

- Kept the existing direct/menu-chain modes `155/156`; no new harness pair was
  added.
- Added `gNdsStageMPPassiveLoopBranchMask` and the `STAGE_MPPASSIVE_BRANCH`
  verifier marker. The recover-loop proof now calls the imported original
  `ftCommonPassiveStandCheckInterruptDamage` and
  `ftCommonPassiveCheckInterruptDamage` through a diagnostic-only status seam
  before running the existing stable-frame proof.
- The branch mask `0x7f` proves buffered-Z PassiveStandF, buffered-Z
  PassiveStandB, neutral-stick PassiveStand no-transition, expired-Z
  PassiveStand no-transition, buffered-Z Passive, expired-Z Passive
  no-transition, and cleanup of the branch probe guard.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
- Deferred: natural collision-driven recovery selection from live collision
  frames, arbitrary recovery durations beyond this guarded window,
  hitlag/damage interaction, full player-driven recovery runtime, and broader
  fighter gameplay.

## 2026-06-27 - PassiveStandB Recover Handoff Proof

- Kept the existing direct/menu-chain modes `155/156`; no new harness pair was
  added.
- Added `gNdsStageMPPassiveLoopPassiveStandBMask` and the
  `STAGE_MPPASSIVE_PASSIVESTANDB` verifier marker.
- Extended the recover-loop proof from a PassiveStandB branch-gate check into a
  bounded PassiveStandB/Ground runtime proof: original buffered-Z setup, five
  guarded stable update/physics/map frames, and the original
  `ftAnimEndSetWait` handoff into Wait/Ground with valid Wait callbacks.
- Focused summaries at this boundary reported
  `mpPassive=ps=73/62->10/4 ticks=6/5/5 passive=81/70->10/4 ticks=6/5/5 branch=0x7f psb=0xf`.
- Verified:
  - `scripts/verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
- Deferred: natural collision-driven recovery selection from live collision
  frames, arbitrary recovery durations beyond this guarded window,
  hitlag/damage interaction, full player-driven recovery runtime, and broader
  fighter gameplay.

## 2026-06-27 - Passive DamageFall Map Selection Proof

- Kept the existing direct/menu-chain modes `155/156`; no new harness pair was
  added.
- Added `gNdsStageMPPassiveLoopNaturalMapMask` and the
  `STAGE_MPPASSIVE_NATURALMAP` verifier marker.
- Reused imported original `ftCommonDamageFallProcMap` instead of adding a
  handwritten recovery dispatcher. The initial proof supplied only bounded
  project-owned floor-hit bits for `mpCommonCheckFighterCliff`, then let the
  original DamageFall map callback choose PassiveStandF or Passive through the
  original PassiveStand/Passive checks. The next entry supersedes this with
  the source-order MP floor collision route.
- Initial focused summaries at this boundary reported
  `mpPassive=ps=73/62->10/4 ticks=6/5/5 passive=81/70->10/4 ticks=6/5/5 branch=0x7f psb=0xf natural=0x7`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
- Deferred: fully live collision-driven recovery selection from unseeded
  collision frames, arbitrary recovery durations beyond this guarded window,
  hitlag/damage interaction, full player-driven recovery runtime, and broader
  fighter gameplay.

## 2026-06-27 - Passive DamageFall Source-Order Floor Collision Proof

- Kept the existing direct/menu-chain modes `155/156`; no new harness pair was
  added.
- Tightened `STAGE_MPPASSIVE_NATURALMAP` from mask `0x7` to `0x1f`. Bits 0-2
  still prove imported original `ftCommonDamageFallProcMap` selects
  PassiveStandF and Passive and cleans up its guard. New bits 3-4 prove both
  branches reached that callback through the existing bounded source-order
  `mpProcessUpdateMain` / `mpCommonRunFighterAllCollisions` floor path instead
  of only setting floor bits by hand.
- Focused summaries at this boundary reported
  `mpPassive=ps=73/62->10/4 ticks=6/5/5 passive=81/70->10/4 ticks=6/5/5 branch=0x7f psb=0xf natural=0x1f`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
- Deferred: fully live collision-driven recovery selection from unseeded
  collision frames, arbitrary recovery durations beyond this guarded window,
  hitlag/damage interaction, full player-driven recovery runtime, and broader
  fighter gameplay.

## 2026-06-27 - Passive DamageFall Installed Map Callback Proof

- Kept the existing direct/menu-chain modes `155/156`; no new harness pair was
  added.
- Tightened `STAGE_MPPASSIVE_NATURALMAP` from mask `0x1f` to `0x7f`. Bits 0-4
  still prove imported original `ftCommonDamageFallProcMap` selects
  PassiveStandF and Passive through the bounded source-order MP floor collision
  path and cleans up its guard. New bits 5-6 prove both PassiveStandF and
  Passive natural-map branches entered through the installed
  `FTStruct.proc_map == ftCommonDamageFallProcMap` callback before the wrapper
  dispatched to the imported original base.
- Current focused summaries report
  `mpPassive=ps=73/62->10/4 ticks=6/5/5 passive=81/70->10/4 ticks=6/5/5 branch=0x7f psb=0xf natural=0x7f`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
- Deferred: fully live collision-driven recovery selection from unseeded
  collision frames, arbitrary recovery durations beyond this guarded window,
  hitlag/damage interaction, full player-driven recovery runtime, and broader
  fighter gameplay.

## 2026-06-27 - Pass-Input Squat Interrupt Seam Fix

- Fixed the bounded MP pass-input status seam to install the original Squat
  proc interrupt callback, `ndsBaseFTCommonSquatProcInterrupt`, instead of the
  predicate-only `ftCommonSquatCheckInterruptCommon`.
- This keeps the project-owned Squat status callback slot aligned with original
  `ftcommonstatus.h` and removes the local incompatible function-pointer build
  warning without changing harness modes.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mppass-input-loop-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-dev-fast.ps1 -DelaySeconds 3`
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`

## 2026-06-27 - Pass-Input Squat Proc Callback Proof

- Kept the existing direct/menu-chain MP pass-input modes; no new harness pair
  was added.
- Tightened the proof to call the installed
  `FTStruct.proc_interrupt == ndsBaseFTCommonSquatProcInterrupt` callback for
  the Squat pass window instead of calling the GotoPass helper directly.
- Expanded `STAGE_MPPASS_INPUT_SETUP` with the Squat proc callback count. The
  verifier now expects `1/1/1/1/1/3/1/1/1/1/0`, proving three original Squat
  proc ticks before the bounded transition reaches Pass.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mppass-input-loop-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-stage-mppass-input-loop-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-dev-fast.ps1 -DelaySeconds 3`
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`
  - `scripts/verify-current.ps1 -DelaySeconds 3`
- Deferred: broad live down-input/pass-through gameplay outside this bounded
  Squat window, arbitrary platform sets, and full controller-driven battle
  runtime.

## 2026-06-27 - Passive Recover Installed Callback Guard

- Kept the current direct/menu-chain modes `155/156`; no new harness pair was
  added.
- Tightened the bounded PassiveStand/Passive stable-frame and final-update
  helpers to require the exact installed callback slots before ticking:
  PassiveStand uses `ftAnimEndSetWait`, `ftPhysicsApplyGroundVelTransN`, and
  `mpCommonSetFighterFallOnEdgeBreak`; Passive uses `ftAnimEndSetWait`,
  `ftPhysicsApplyGroundVelFriction`, and
  `mpCommonSetFighterFallOnGroundBreak`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
- Deferred: unbounded PassiveStand/Passive gameplay, hitlag/damage interaction,
  and full controller-driven battle runtime.

## 2026-06-27 - Dash-Run AttackDash Status Proof

- Kept the existing direct/menu-chain Dash -> Run -> RunBrake modes; no new
  harness pair was added.
- Imported original BattleShip `ftcommonattackdash.c` through
  `src/import/battleship_ftcommon_attackdash.c`.
- Added the narrow item compatibility surface needed to link the original
  AttackDash helper while keeping item attack branches deferred for this proof.
- Fixed the local `FTAttributes` move-availability bitfield mapping to read the
  relocated BattleShip high-bit flag word (`0xfffffc00`) correctly on ARM.
- Extended the dash-run proof with an isolated A-button tap from Run that calls
  original `ftCommonAttackDashCheckInterruptCommon`, reaches AttackDash
  status/motion `191/167` for Mario/Fox, records `DASH_RUN_ATTACK`, then
  restores Run before the existing run/runbrake movement proof continues.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`
- Deferred: AttackDash animation runtime, hitboxes, item throw/swing attack
  branches, full attack interrupt tables, and broad fighter gameplay.

## 2026-06-27 - Dash-Run AttackDash Callback Slot Alignment

- Kept the existing direct/menu-chain Dash -> Run -> RunBrake modes; no new
  harness pair was added.
- Aligned the local `ftMainSetStatus` AttackDash seam with the original
  BattleShip status table by installing `ftAnimEndSetWait`, no interrupt,
  `ftPhysicsApplyGroundVelTransN`, and
  `mpCommonSetFighterFallOnEdgeBreak`.
- Extended `DASH_RUN_ATTACK` with callback mask `0xff` and raised the dash-run
  proof mask from `0x7ff` to `0xfff`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1` (known large-file/generated-output
    warnings only)
  - `scripts/clean-generated.ps1 -DryRun`
  - `Battle Mario/Fox Dash-Run harness passed: dash->run->runbrake + attackdash=191/167 cb=0xff root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 dash->run->runbrake + attackdash=191/167 cb=0xff root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: executing AttackDash animation runtime, hitboxes, item attack
  branches, and unbounded attack gameplay.

## 2026-06-27 - Dash-Run AttackDash Callback Tick Proof

- Kept the existing direct/menu-chain Dash -> Run -> RunBrake modes; no new
  harness pair was added.
- Added one guarded call each to the installed AttackDash update, physics, and
  map callbacks for Mario/Fox after original `ftCommonAttackDashCheckInterruptCommon`
  reaches status/motion `191/167`.
- Extended `DASH_RUN_ATTACK` with tick mask `0x3f` and raised the dash-run
  proof mask from `0xfff` to `0x1fff`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`
  - `Battle Mario/Fox Dash-Run harness passed: dash->run->runbrake + attackdash=191/167 cb=0xff tick=0x3f root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 dash->run->runbrake + attackdash=191/167 cb=0xff tick=0x3f root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: AttackDash hitboxes, animation command runtime, item attack
  branches, and unbounded attack gameplay.

## 2026-06-27 - AttackDash Status Enum Alignment

- Corrected the project-owned sparse `FTCommonStatus` enum to preserve the
  original BattleShip ordering for `Attack11`, `Attack12`, and `AttackDash`.
  The verified original values are `190`, `191`, and `192`; the local sparse
  enum had previously assigned `AttackDash` to `191`.
- Updated the direct/menu-chain Dash -> Run verifier expectations and current
  docs so `DASH_RUN_ATTACK` now expects AttackDash status/motion `192/167`.
- Deferred: importing `ftcommonattack1.c`, jab hitboxes, animation command
  runtime, item attack branches, and unbounded attack gameplay.

## 2026-06-27 - Passive Recovery Stable Frame Source Order

- Reordered the bounded PassiveStand/Passive recover-loop stable-frame ticks
  so they run the installed update callback before physics/map, matching the
  original BattleShip process order from `ftMainProcUpdateInterrupt` followed
  by `ftMainProcPhysicsMap`.
- Kept the same direct/menu-chain harness pair and marker contract:
  `mpPassive=ps=73/62->10/4 ticks=6/5/5 passive=81/70->10/4 ticks=6/5/5
  branch=0x7f psb=0xf natural=0x7f mapcalls=2`.
- Verified:
  - `make -j16`
  - `scripts/verify-dev-fast.ps1 -DelaySeconds 3`
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`

## 2026-06-27 - Boundary Docs Consistency

- Updated `AGENTS.md`, `docs/HARNESSES.md`, `docs/STATUS.md`,
  `docs/ROADMAP.md`, and `docs/NEXT_BOUNDARY_QUEUE.md` so the active boundary
  consistently points at PassiveStand/Passive recover-loop modes `155/156`
  with `mapcalls=2`.
- Left platform-speed modes `151/152` and Inishie scale modes `153/154` as
  regression history instead of current planning targets.
- Verified:
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `git diff --name-only -- decomp`

## 2026-06-27 - Passive Recovery DamageFall Map Callback Tightening

- Tightened the current PassiveStand/Passive recover-loop proof so the natural
  DamageFall map branch only runs through the installed
  `FTStruct.proc_map == ftCommonDamageFallProcMap` callback. A missing callback
  slot now marks the proof unsafe instead of executing the direct base wrapper.
- No new harness or marker was added; the existing
  `STAGE_MPPASSIVE_NATURALMAP` `0x7f` requirement still proves both
  PassiveStandF and Passive selections went through the installed callback.
- Verified:
  - `make -j16`
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `scripts/clean-generated.ps1 -DryRun`

## 2026-06-27 - Passive Recovery DamageFall Map Call Count

- Added `gNdsStageMPPassiveLoopDamageFallMapCallCount` and
  `STAGE_MPPASSIVE_NATURALMAP_CALLS`; current modes `155/156` now require
  exactly two guarded `ftCommonDamageFallProcMap` wrapper calls for the
  PassiveStandF and Passive natural map branches.
- No new harness was added; this tightens the existing active boundary.
- Verified:
  - `make -j16`
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `scripts/clean-generated.ps1 -DryRun`

## 2026-06-27 - Dash-Run AttackDash Run Proc Route Proof

- Tightened the existing direct/menu-chain Dash -> Run -> RunBrake proof so
  the isolated A-button tap from Run calls the installed original
  `ftCommonRunProcInterrupt` callback instead of directly calling the
  AttackDash interrupt helper.
- Added `gNdsFighterDashRunAttackDashRunProcMask`; `DASH_RUN_ATTACK` now
  reports `runproc=0x3` when both Mario and Fox enter AttackDash through the
  Run interrupt route. The dash-run proof mask is now `0x3fff`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `scripts/clean-generated.ps1 -DryRun`
  - `Battle Mario/Fox Dash-Run harness passed: dash->run->runbrake + attackdash=191/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 dash->run->runbrake + attackdash=191/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: AttackDash hitboxes, animation command runtime, item attack
  branches, and unbounded attack gameplay.

## 2026-06-27 - Dash-Run Attack11 Wait Proc Route Proof

- Added the bounded `src/import/battleship_ftcommon_attack1.c` wrapper for
  original BattleShip `ftcommonattack1.c`, remapping the imported Attack11
  entry points behind project-owned wrappers.
- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof with a
  pre-Dash Wait A-tap. The proof now calls original
  `ftCommonAttack1CheckInterruptCommon`, enters Attack11 status/motion
  `190/165` for Mario/Fox, installs the original Attack11 update/interrupt
  callbacks plus bounded physics/map callbacks, and ticks those callback slots
  once per fighter.
- Added `DASH_RUN_ATTACK11` diagnostics:
  `check=2/2`, `setstatus=2`, `ftmain=2`, status/motion `190/165`,
  callback mask `0xff`, tick mask `0xff`, and wait-proc mask `0x3`. The
  dash-run proof mask is now `0x1ffff`.
- Kept the current port's `nFTMotionAttackIDNone` /
  `nFTStatusAttackIDNone` sentinel at `-1` so existing FTStruct/Wait
  diagnostics remain stable, while assigning explicit original-compatible
  Attack11/Attack12/Attack13/Attack100/AttackDash attack IDs for the imported
  Attack1 code path.
- Verified:
  - `make -j16`
  - `make -C . TARGET=smash64ds-battle-mariofox-dash-run BUILD=build-battle-mariofox-dash-run-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_dash_run -B -j16`
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 5`
  - `make -C . TARGET=smash64ds-menu-chain-mariofox-dash-run BUILD=build-menu-chain-mariofox-dash-run-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dash_run -B -j16`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 5`
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`
  - `scripts/verify-current.ps1 -DelaySeconds 3`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `scripts/clean-generated.ps1 -DryRun`
  - `git diff --name-only -- decomp`
  - `Battle Mario/Fox Dash-Run harness passed: wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: Attack11 hitboxes, follow-up Attack12/Attack13/Attack100 runtime,
  animation command runtime, item attack branches, and unbounded attack
  gameplay.

## 2026-06-28 - Dash-Run Attack11 to Attack12 Follow-Up Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof with a
  bounded original Attack11 follow-up check. After the Wait A-tap enters
  Attack11, the proof seeds the original follow-up window, calls the installed
  Attack11 interrupt/update callbacks, and verifies the original follow-up gate
  reaches Attack12 status/motion `191/166` for both Mario and Fox.
- Added `DASH_RUN_ATTACK12` diagnostics:
  `setstatus=2`, `ftmain=2`, status/motion `191/166`, callback mask `0xff`,
  and goto mask `0xf`. The dash-run proof mask is now `0x7ffff`.
- No new harness was added; this tightens the existing direct/menu-chain
  dash-run proof pair.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 5`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 5`
  - `scripts/verify-boundary.ps1 -DelaySeconds 3`
  - `scripts/verify-current.ps1 -DelaySeconds 3`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `scripts/clean-generated.ps1 -DryRun`
  - `git diff --name-only -- decomp`
  - `Battle Mario/Fox Dash-Run harness passed: wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: Attack12 hitboxes, animation command runtime, Attack13/Attack100
  follow-up runtime, item attack branches, and unbounded attack gameplay.

## 2026-06-28 - Dash-Run Attack12 to Mario Attack13 Gate Proof

- Extended the same direct/menu-chain Dash -> Run -> RunBrake proof with a
  bounded original Attack12 follow-up check. Mario reaches fighter-specific
  Attack13 status/motion `220/195`; Fox remains at Attack12 `191/166` because
  original `ftCommonAttack13CheckFighterKind` excludes Fox.
- Added `DASH_RUN_ATTACK13` diagnostics:
  `setstatus=1`, `ftmain=1`, Mario status/motion `220/195`, Fox blocked
  status/motion `191/166`, callback mask `0xf`, and goto mask `0x7`. The
  dash-run proof mask is now `0x1fffff`.
- No new harness was added; this tightens the existing direct/menu-chain
  dash-run proof pair.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 5`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 5`
  - `Battle Mario/Fox Dash-Run harness passed: wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: Attack13 hitboxes, animation command runtime, Attack100 runtime,
  item attack branches, and unbounded attack gameplay.

## 2026-06-28 - Dash-Run Attack12 to Fox Attack100Start Gate Proof

- Imported bounded original `ftcommonattack100.c` through
  `src/import/battleship_ftcommon_attack100.c`.
- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof with a
  bounded original Attack12 rapid-jab gate check. Fox now arms
  `is_goto_attack100` through original
  `ftCommonAttack100StartCheckInterruptCommon`, then reaches Fox
  Attack100Start status/motion `220/195` through original
  `ftCommonAttack100StartSetStatus` and the project-owned `ftMainSetStatus`
  seam. Mario Attack13 remains distinguished from Fox Attack100Start by
  fighter kind even though both use status `220`.
- Added `DASH_RUN_ATTACK100` diagnostics:
  `check/setstatus/ftmain=5/1/1`, Fox status/motion `220/195`, callback mask
  `0xf`, and goto mask `0x3`. The dash-run proof mask is now `0x3fffff`.
- No new harness was added; this tightens the existing direct/menu-chain
  dash-run proof pair.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `Battle Mario/Fox Dash-Run harness passed: wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: Attack100Loop/Attack100End runtime, rapid-jab hitboxes, animation
  command runtime, item attack branches, and unbounded attack gameplay.

## 2026-06-28 - Dash-Run Fox Attack100Start to Attack100Loop Entry Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof with a
  bounded original Fox rapid-jab loop-entry check. The proof drives the
  installed original `ftCommonAttack100StartProcUpdate` callback at animation
  end and verifies the handoff into Fox Attack100Loop status/motion `221/196`
  through the project-owned `ftMainSetStatus` seam.
- Added `DASH_RUN_ATTACK100_LOOP` diagnostics:
  public wrapper count `0`, `ftmain=1`, Fox status/motion `221/196`,
  callback mask `0xf`, and goto mask `0x3`. The public wrapper count is
  expected to stay zero here because the imported translation unit calls the
  macro-renamed base Loop status function internally. The dash-run proof mask
  is now `0x7fffff`.
- No new harness was added; this tightens the existing direct/menu-chain
  dash-run proof pair.
- Verified:
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `Battle Mario/Fox Dash-Run harness passed: wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0x1f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0x1f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: Attack100Loop runtime beyond entry, Attack100End, rapid-jab
  hitboxes, animation command runtime, item attack branches, and unbounded
  attack gameplay.

## 2026-06-28 - Dash-Run Fox Attack100Loop Guarded Tick Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof with
  one guarded original Fox Attack100Loop interrupt/update tick. The tick uses
  the installed original Loop callbacks, proves A-input sets loop intent,
  proves the update consumes `motion_vars.flags.flag1`, marks animation end,
  clears loop intent, and remains in Fox Attack100Loop status/motion `221/196`.
- Extended `DASH_RUN_ATTACK100_LOOP` with tick mask `0x1f`. The dash-run proof
  mask is now `0xffffff`.
- Verified:
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `Battle Mario/Fox Dash-Run harness passed: wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0x1f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0x1f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: continuous Attack100Loop runtime, Attack100End, rapid-jab hitboxes,
  animation command runtime, item attack branches, and unbounded attack
  gameplay.

## 2026-06-28 - Dash-Run Fox Attack100Loop to Attack100End Handoff Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof with a
  bounded no-input Fox rapid-jab loop exit. The proof drives the installed
  original `ftCommonAttack100LoopProcUpdate` callback into original
  `ftCommonAttack100EndSetStatus`, installs Fox Attack100End status/motion,
  then runs the installed End update through `ftAnimEndSetWait` back to
  Wait/Ground.
- Extended `DASH_RUN_ATTACK100_LOOP` tick mask from `0x1f` to `0xfff`. Bits
  now cover the prior A-input loop tick, no-input Loop -> Attack100End status,
  End callback shape, and End -> Wait/Ground handoff.
- No new harness was added; this tightens the existing direct/menu-chain
  dash-run proof pair.
- Verified:
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `Battle Mario/Fox Dash-Run harness passed: wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0xfff; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0xfff; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: continuous Attack100Loop runtime, rapid-jab hitboxes, animation
  command runtime, item attack branches, and unbounded attack gameplay.

## 2026-06-28 - Dash-Run Attack Animation-Event Seam Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof with a
  bounded `ftMainPlayAnimEventsAll` marker for original attack status paths.
  `DASH_RUN_ATTACK_ANIM=0x3f` proves Attack11 for Mario/Fox, Attack12 for
  Mario/Fox, Mario Attack13, and Fox Attack100Start reached the project-owned
  animation-event seam through their original status setters.
- AttackDash is intentionally not part of this marker. Original BattleShip
  `ftCommonAttackDashSetStatus` calls `ftMainSetStatus` but does not call
  `ftMainPlayAnimEventsAll`; the existing AttackDash status/callback proof
  remains the correct coverage for that path.
- The dash-run proof mask is now `0x1ffffff`.
- Verified:
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-current.ps1 -DelaySeconds 3`
  - `scripts/check-docs.ps1`
  - `scripts/check-architecture.ps1`
  - `git diff --name-only -- decomp`
  - `git diff --check`
  - `Battle Mario/Fox Dash-Run harness passed: wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0xfff; anim=0x3f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0xfff; anim=0x3f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: animation command runtime, attack hitboxes, continuous rapid-jab
  gameplay, item attack branches, and unbounded attack gameplay.

## 2026-06-28 - Dash-Run GuardOn Status Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof with a
  bounded original GuardOn status/callback branch from Wait.
- Imported original BattleShip `ftcommonguard1.c` and `ftcommonguard2.c`
  through `src/import/battleship_ftcommon_guard.c`, keeping the original code
  behind project-owned seams and without importing full shield gameplay.
- Added `DASH_RUN_GUARD` diagnostics. The proof holds Z from Wait, calls
  original `ftCommonGuardOnCheckInterruptCommon`, reaches GuardOn status/motion
  `152/134`, installs callback mask `0xff`, records state mask `0x20f`,
  emits GuardOn FGM `13`, and records the guarded shield-effect seam.
- Fixed the local `FTAttributes` extension after `shield_size` so subsequent
  fields retain their original offsets. Before this, Mario/Fox fell back to
  stub fighter GObjs because `commonparts_container` was read from the wrong
  offset.
- The dash-run proof mask is now `0x3ffffff`.
- Verified:
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-current.ps1 -DelaySeconds 3`
  - `Battle Mario/Fox Dash-Run harness passed: wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; guard=152/134 cb=0xff state=0x20f fgm=13; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0xfff; anim=0x3f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
  - `Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; guard=152/134 cb=0xff state=0x20f fgm=13; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0xfff; anim=0x3f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000`
- Deferred: full GuardOn/Guard/GuardOff/SetOff shield runtime, shield collision,
  shield visuals, guard cancel/escape/catch branches, hitlag interaction, item
  branches, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Appeal Status Proof

- Extended the existing direct/menu-chain Passive recovery modes `155/156`
  without adding a new harness pair.
- Imported bounded original BattleShip `ftcommonappeal.c` through
  `src/import/battleship_ftcommon_appeal.c`.
- Added the narrow Kirby passive-vars compatibility fields required for the
  original Appeal source to compile; Mario/Fox proof execution does not import
  Kirby runtime.
- Added `STAGE_MPPASSIVE_APPEAL` diagnostics. The proof seeds one L-button tap
  from Wait/Ground, calls imported original
  `ftCommonAppealCheckInterruptCommon`, reaches Appeal/Ground status/motion
  `189/164`, records the callback shape, and proves the bounded guard cleaned
  up.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-current.ps1 -DelaySeconds 3`
  - direct/menu-chain summaries now include `appeal=189/164`.
- Deferred: continuous Appeal/Taunt animation, event runtime, audio/FGM beyond
  the current status seam, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Appeal Callback/Update Proof

- Extended the existing direct/menu-chain Passive recovery modes `155/156`
  without adding a harness.
- After the bounded original L-button Appeal status proof, the proof now ticks
  the installed original `ftCommonAppealProcInterrupt` callback once with
  `motion_vars.flags.flag1 = 0`, proving the callback is callable while catch
  and guard branches remain deferred.
- The same proof then runs the installed `ftAnimEndSetWait` update slot at
  animation end and verifies the fighter returns to Wait/Ground with the Wait
  callback shape.
- `STAGE_MPPASSIVE_APPEAL` now requires mask `0x7f`.
- Verified:
  - `make -j16`
  - `.\scripts\verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  - `.\scripts\verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
- Deferred: Appeal catch/guard interrupt branches, continuous Appeal animation
  events, audio/FGM beyond existing seams, Kirby copy-loss behavior, and
  unbounded fighter gameplay.

## 2026-06-28 - Passive Recover DownWait Chain Proof

- Reused the existing bounded original DownWait proof instead of adding a new
  harness.
- Enabled `NDS_MARIOFOX_STAGE_MPDOWNWAIT_LOOP_HARNESS` under the current
  Passive recover modes `155/156`.
- Tightened the current recover verifier wrapper so `STAGE_MPDOWNWAIT` markers
  are required for the latest direct/menu-chain boundary.
- The latest proof now continues from the recovered Wait/Ground boundary into
  DownWaitU/Ground, DownStandU, DownAttackU, DownForwardU, and DownBackU
  source-order branches and their Wait/Ground handoffs.
- Verified:
  - `make -j16`
  - `.\scripts\verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  - `.\scripts\verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
- Deferred: continuous downed-action gameplay, hitboxes, hurtboxes, live
  player-driven damage/downed selection, and unbounded fighter scheduling.

## 2026-06-28 - Dash-Run Guard Escape Status Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof without
  adding a new harness pair.
- Imported bounded original BattleShip `ftcommonescape.c` through
  `src/import/battleship_ftcommon_escape.c`.
- Added `DASH_RUN_ESCAPE` diagnostics. The proof reuses the verified GuardOn
  state, seeds held-stick direction for both fighters, calls original
  `ftCommonEscapeCheckInterruptGuard`, reaches EscapeF/EscapeB status/motion
  `156/136` and `157/137`, records callback mask `0x3ff`, state mask `0xff`,
  and preserves original `itemthrow_buffer_tics=5`.
- The dash-run proof mask is now `0x7ffffff`.
- Verified:
  - `make -j16`
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - direct/menu-chain summaries now include
    `escape=156/136 cb=0x3ff state=0xff`.
- Deferred: full Guard/GuardOff/SetOff/Escape shield runtime, shield roll
  movement, guard cancel/catch branches, hitlag interaction, item branches, and
  unbounded fighter gameplay.

## 2026-06-28 - Dash-Run GuardOn Update Tick Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof without
  adding a new harness pair.
- Reused the imported original `ftcommonguard1.c` path and ran one installed
  original `ftCommonGuardOnProcUpdate` tick with positive animation time.
- `DASH_RUN_GUARD` now requires state mask `0x3e0f`. The new bits prove the
  callback preserves GuardOn status/motion `152/134`, keeps shield active, and
  advances shield decay/release counters from `16/8` to `15/7`.
- Verified:
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - direct/menu-chain summaries now include
    `guard=152/134 cb=0xff state=0x3e0f fgm=13`.
- Deferred: full Guard/GuardOff/SetOff/Escape shield runtime, shield collision,
  shield visuals, guard cancel/catch branches, hitlag interaction, item
  branches, and unbounded fighter gameplay.

## 2026-06-28 - Dash-Run GuardOn To Guard Handoff Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof without
  adding a new harness pair.
- Corrected the local bounded GuardOn status callback installation to use
  `mpCommonSetFighterFallOnGroundBreak`, matching BattleShip's original common
  status table for GuardOn/Guard.
- Added one animation-end `ftCommonGuardOnProcUpdate` tick. The proof now
  reaches original `ftCommonGuardSetStatus`, enters Guard status `153` while
  preserving GuardOn motion `134`, and installs the original Guard
  update/interrupt/physics/map callbacks.
- `DASH_RUN_GUARD` now requires state mask `0x3fe0f`. The new bits prove the
  Guard handoff status/callback state after the existing positive-time GuardOn
  update tick.
- Verified:
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - direct/menu-chain summaries now include
    `guard=152/134 cb=0xff state=0x3fe0f fgm=13`.
- Deferred: full Guard hold, GuardOff, SetOff, shield collision, shield
  visuals, guard cancel/catch branches, hitlag interaction, item branches, and
  unbounded fighter gameplay.

## 2026-06-28 - Dash-Run Guard Hold Tick Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof without
  adding a new harness pair.
- After the verified GuardOn -> Guard handoff, ran one installed original
  `ftCommonGuardProcUpdate` callback with Z still held.
- `DASH_RUN_GUARD` now requires state mask `0x3ffe0f`. The new bits prove the
  Guard hold update stays in Guard status `153`, keeps shield active, keeps
  release unscheduled, and advances shield decay/release counters from the
  handoff state.
- Verified:
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - direct/menu-chain summaries now include
    `guard=152/134 cb=0xff state=0x3ffe0f fgm=13`.
- Deferred: continuous Guard hold, GuardOff, SetOff, shield collision, shield
  visuals, guard cancel/catch branches, hitlag interaction, item branches, and
  unbounded fighter gameplay.

## 2026-06-28 - Dash-Run GuardOff Release Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof without
  adding a new harness pair.
- During the verified Guard hold state, released Z and ran the installed
  original `ftCommonGuardProcUpdate` callback. This reaches original
  `ftCommonGuardOffSetStatus`, enters GuardOff status/motion `154/135`, and
  installs original GuardOff update/physics/map callbacks.
- Restored Guard through original `ftCommonGuardSetStatus` before the existing
  Guard -> Escape proof, so the Escape proof still runs from a real Guard
  state.
- `DASH_RUN_GUARD` now requires state mask `0xffffe0f`. The new bits prove the
  GuardOff status/callback state and the release-side shield counters.
- Verified:
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - direct/menu-chain summaries now include
    `guard=152/134 cb=0xff state=0xffffe0f fgm=13`.
- Deferred: GuardOff completion to Wait, continuous Guard hold, SetOff, shield
  collision, shield visuals, guard cancel/catch branches, hitlag interaction,
  item branches, and unbounded fighter gameplay.

## 2026-06-28 - Dash-Run GuardOff Completion Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof without
  adding a new harness pair.
- After the verified GuardOff status/motion `154/135`, ran one installed
  original `ftCommonGuardOffProcUpdate` callback at animation end. This reaches
  original `ftCommonWaitSetStatus` and returns to Wait/Ground with Wait
  callbacks installed.
- Restored Guard through original `ftCommonGuardSetStatus` before the existing
  Guard -> Escape proof, so the Escape proof still runs from a real Guard
  state.
- `DASH_RUN_GUARD` now requires state mask `0xfffffe0f`. The new bits prove
  the GuardOff -> Wait status/callback state.
- Verified:
  - `scripts/verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - `scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`
  - direct/menu-chain summaries now include
    `guard=152/134 cb=0xff state=0xfffffe0f fgm=13`.
- Deferred: continuous Guard hold, SetOff, shield collision, shield visuals,
  guard cancel/catch branches, hitlag interaction, item branches, and unbounded
  fighter gameplay.

## 2026-06-28 - Passive Recover Turn Chain Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded original Turn proof and wired it into the current
  Passive recover modes after the recovered Wait/Ground boundary.
- The latest direct/menu-chain boundary now proves original
  `ftCommonTurnCheckInterruptCommon` reaches Turn/Ground status/motion `18/12`,
  the installed update callback flips facing and ground velocity
  `1 -> -1` / `2500 -> -2500`, and the final update returns through the
  original Wait/Ground handoff.
- Deferred: continuous player-driven Turn movement, broader directional
  movement scheduling, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover DownRecoverD Chain Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded original face-down DownRecover proof and wired it
  into the current Passive recover modes after the recovered/Turn-proven
  Wait/Ground boundary.
- The latest direct/menu-chain boundary now proves DownWaitD/Ground `69/-2`,
  DownStandD `71/60`, DownAttackD `79/68`, DownForwardD `75/64`, DownBackD
  `77/66`, and the original Wait/Ground handoff mask `0xf`.
- Deferred: continuous face-down downed-action runtime, hitboxes/runtime effects
  for DownAttackD, player-driven recovery scheduling, and unbounded fighter
  gameplay.

## 2026-06-28 - Passive Recover CliffLedge Aggregation Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded CliffLedge aggregation proof and wired it into
  the current Passive recover modes after the DownRecoverD proof.
- The latest direct/menu-chain boundary now proves same-cliff occupancy blocks
  a second catch, ledge drop clears cliff hold into Fall/Air with
  `cliffcatch_wait=30`, recatch succeeds after release, and CliffClimbQuick2
  finishes through the original Wait/Ground handoff on Pupupu line `3`.
- Deferred: continuous natural ledge occupancy/release/drop/climb runtime,
  arbitrary player-driven ledge scheduling, ledge attack/escape hitboxes, and
  unbounded fighter gameplay.

## 2026-06-28 - Passive Recover CliffLive Selected-Process Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded CliffLive proof and wired it into the current
  Passive recover modes after the CliffLedge aggregation proof.
- The latest direct/menu-chain boundary now drives a proof-owned selected P0
  `GObjProcess` through original CliffCatch -> CliffWait -> CliffQuick ->
  CliffClimbQuick1 -> CliffClimbQuick2, one guarded common2 update/physics/map
  tick, Wait/Ground finish, and a reseeded CliffWait drop into Fall/Air.
- Current marker:
  `mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff`.
- Deferred: continuous natural ledge runtime, arbitrary player-driven ledge
  scheduling, ledge attack/escape hitboxes, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Hyrule Wall-Hit Floor Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded Hyrule wall-hit floor proof and wired it into
  the current Passive recover modes after the CliffLive proof.
- The latest direct/menu-chain boundary now validates the source-order MP
  wall-line/floor-edge scout relationship for the selected wall-hit probe:
  `floor=5`, `wall=13`, `edge=12`, `side=0`, `mapNodes=1`, and
  `delta=-1600/-388`.
- Current marker:
  `mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388`.
- Deferred: natural live wall collision, wall-copyback integration, wall
  teching, continuous collision response, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Hyrule Wall-Copy Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded wall-copy proof for the selected Hyrule
  wall-hit probe and wired it into the current Passive recover modes.
- The latest direct/menu-chain boundary now proves one source-order wall
  collision copyback pass for `floor=5`, `wall=13`, `edge=12`, preserving the
  expected final wall collision mask and leaving the other fighter untouched.
- Current marker:
  `mpWallCopy=pass floor=5 wall=13 edge=12 delta=-1600/-388`.
- Deferred: natural live wall collision, arbitrary wall copyback, wall
  teching, continuous collision response, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Pass-Through Floor Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded pass-through floor proof and wired it into the
  current Passive recover modes after the wall-copy proof.
- The latest direct/menu-chain boundary now proves the source-order
  pass-through floor route: same-line pass-through collision is rejected
  through `ignore_line_id`, while the different-line probe is accepted through
  the original-compatible pass callback.
- Current marker:
  `mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0`.
- Deferred: full player-driven down-input drop-through, moving platform
  pass-through, continuous platform gameplay, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Platform-Floor Classification Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded platform-floor proof and wired it into the
  current Passive recover modes after the pass-through proof.
- The latest direct/menu-chain boundary now checks the selected pass-through
  line against BattleShip's yakumono platform predicate and records DObj,
  status, animation, and blocker state for that line.
- Current marker:
  `mpPlatform=line=0 yak=1 dobj=1 status=0 anim=0 deferred=0x40`.
- Deferred: active platform state, ticking, movement, speed transfer, full
  moving-platform pass-through, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Active Platform-Floor Predicate Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded active platform-floor proof and wired it into the
  current Passive recover modes after the inactive platform classification.
- The latest direct/menu-chain boundary now installs the bounded yakumono DObj
  for Dream Land line `0`, sets the original-compatible active status, and
  proves BattleShip's platform predicate reports active.
- Current marker:
  `mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active`.
- Deferred: platform ticking, movement, speed transfer, full moving-platform
  pass-through, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Platform Tick Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded platform-tick proof and wired it into the current
  Passive recover modes after the active platform predicate.
- The latest direct/menu-chain boundary now runs one guarded
  `mpCollisionAdvanceUpdateTic` step for the active Dream Land yakumono and
  verifies the predicate remains active.
- Current marker:
  `mpPlatformTick=tic=0->1 setOn=1 advance=1 status=1/1`.
- Deferred: platform movement, speed transfer, full moving-platform
  pass-through, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Natural Drop-Through Input Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded natural drop-through input proof and wired it
  into the current Passive recover modes after the platform-tick proof.
- The latest direct/menu-chain boundary now seeds original-compatible down
  input on Dream Land pass-through line `0` and routes Wait -> Squat -> Pass
  through imported original `ftcommonpass.c` / `ftcommonsquat.c`.
- Current marker:
  `mpPassInput=line=0 flags=0x4000 squat=28 pass=33 ignore=0`.
- Deferred: moving-platform pass-through, continuous platform gameplay, and
  unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Platform Position Primitive Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded platform-position proof and wired it into the
  current Passive recover modes after the natural drop-through input proof.
- The latest direct/menu-chain boundary now calls
  `mpCollisionSetYakumonoPosID` for Dream Land line `0` / yakumono `1` and
  records the resulting `gMPCollisionSpeeds` delta.
- Current marker:
  `mpPlatformPos=line=0 yak=1 delta=12000/-4000/2000`.
- Deferred: continuous platform movement, live speed transfer through
  collision, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Platform Speed Consumer Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded platform-speed proof and wired it into the
  current Passive recover modes after the platform-position proof.
- The latest direct/menu-chain boundary now reads the active Dream Land
  yakumono speed through `mpCollisionGetSpeedLineID` and runs the bounded
  dynamic floor/ceil/wall, wall-process, animation, bounds, and stage-animation
  diagnostic slices already covered by the platform-speed verifier.
- Current marker:
  `mpPlatformSpeed=line=0 yak=1 read=12000/-4000 dyn=1/2 ceil=1/1 wall=1/1 procwall=1/1 anim=1 bounds=1 stageanim=0/0x91 inishieAsset=header/geometry nodes=1`.
- Deferred: continuous moving-platform gameplay, unbounded live speed transfer,
  and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Inishie Scale Proof

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded Inishie/Mushroom Kingdom scale proof and wired it
  into the current Passive recover modes after the platform-speed proof.
- The latest direct/menu-chain boundary now stages read-only
  `StageInishieFile3`, runs source-backed `grInishieMakeScale` /
  `grInishieScaleProcUpdate`, and records the source-DL preview plus scale
  update threshold diagnostics.
- Current marker:
  `inishieScale=ticks=2 lines=1/2 alt=80000->64000 y=363000/362000->427000/298000 speed=-8000/8000 fall=1->2/0 step=3->0/0 sourceDL=0xff cmds=91 tris=20 tex=0x3f preview=0x3f px=432`.
- Deferred: full Mushroom Kingdom runtime, hardware-backed texture/material
  rendering, continuous scale gameplay, and unbounded fighter gameplay.

## 2026-06-28 - Passive Recover Dash-Run Aggregate Guard

- Extended the current direct/menu-chain PassiveStand/Passive recover-loop
  modes `155/156` without adding a new harness pair.
- Reused the existing bounded Dash-Run attack/guard aggregate proof and wired
  it into the current Passive recover modes after the Inishie scale proof.
- The latest direct/menu-chain boundary now asserts the `DASH_RUN` marker and
  records the older Attack11/Attack12/Mario Attack13/Fox Attack100,
  AttackDash, GuardOn/Guard/GuardOff, and EscapeF/EscapeB
  status/callback/update slices under the latest VSBattle/Pupupu root.
- Current marker:
  `dashRun=0x7ffffff`.
- Deferred: full attack hitboxes, continuous Attack100/Guard runtime, shield
  collision, player-driven attack/shield gameplay, and unbounded fighter
  gameplay.

## 2026-06-28 - Cliff Common2 Current-Fold Blocker

- Tested a narrow current-mode fold of the existing standalone
  CliffAttack/Common2 and CliffEscape/Common2 proofs into modes `155/156`.
- The macro-gate-only approach was not kept. It let the CliffAttack/Common2
  diagnostics publish, but shared selected-fighter proof state changed early
  enough that later wall-copy/platform/Inishie current-boundary finalizers no
  longer published.
- Restored the current boundary to the prior proven behavior and kept
  CliffAttack/Common2 and CliffEscape/Common2 as standalone proof pairs.
- Current direct proof after restoration:
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  passed with `dashRun=0x7ffffff`.
- Deferred: promote CliffAttack/Common2 or CliffEscape/Common2 into the latest
  aggregate only after isolated reseeding or finalizer reordering.

## 2026-06-28 - Promoted CliffAttack/Common2 Into Current Passive Recover Modes

- Kept the existing direct/menu-chain Passive recover modes `155/156`; no new
  harness pair was added.
- Reordered the aggregate cliff finalizers after the current tail so the
  wall-copy, platform, platform-speed, and Inishie scale finalizers publish
  before the delayed cliff attack/common2 probes mutate the selected fighter.
- Added a delayed aggregate reseed before the CliffAttack floor probe. The
  reseed restores P1 to original-compatible CliffWait state with the existing
  cliff ID, then calls the imported original CliffWait interrupt path to prove
  the A-button CliffAttack transition.
- Isolated the shared CliffCommon2 bridge diagnostics before the CliffAttack
  action probe so prior CliffClimb bridge calls no longer contaminate the
  CliffAttack action mask.
- Scoped verifier expectations for the aggregate path: standalone
  CliffAttack proofs keep the stricter existing checks, while current modes
  accept the positive prior CliffWait fall-wait tick created before the delayed
  reseed.
- Direct proof:
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  passed with
  `mpCliffAttack=status=85->86 motion=73->74 queue=1 cliff=3 button=0x8000`,
  `mpCliffAttackAction=status=86->92->93 motion=74->80->81 cliff=3 floor=3`,
  and `mpCliffCommon2=status=93->93->93->93 motion=81->81->81->81`.
- Menu-chain proof:
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  passed with the same CliffAttack/Common2 markers.
- Current profile:
  `verify-current.ps1 -DelaySeconds 3` passed.
- Deferred: CliffEscape/Common2 remains standalone regression coverage. Promote
  it only with the same delayed-reseed and bridge-diagnostic isolation pattern.

## 2026-06-28 - Promoted CliffEscape/Common2 Into Current Passive Recover Modes

- Kept the existing direct/menu-chain Passive recover modes `155/156`; no new
  harness pair was added.
- Reused the existing bounded CliffEscape action/Common2 proofs and folded
  them into the current Passive recover aggregate after the CliffAttack/Common2
  proof.
- Added a delayed aggregate reseed before the CliffEscape action probe. The
  reseed restores the selected fighter to original-compatible CliffWait state
  with the existing cliff ID, then calls the imported original CliffWait
  interrupt path to prove the stick-away CliffEscape transition.
- Isolated the shared CliffCommon2 bridge diagnostics before the CliffEscape
  action probe so earlier CliffAttack bridge calls do not contaminate the
  CliffEscape action mask.
- Scoped verifier expectations for the aggregate path: standalone
  CliffAttack proofs keep their stricter bridge checks, while current modes
  let the later CliffEscape bridge own the shared bridge diagnostics.
- Direct proof:
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  passed with
  `mpCliffEscapeAction=status=85->86->96->97 motion=73->74->84->85 cliff=3 floor=3`
  and
  `mpCliffEscapeCommon2=status=97->97->97->97 motion=85->85->85->85 cliff=3 floor=3->3`.
- Menu-chain proof:
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`
  passed with the same CliffEscape/Common2 markers.
- Current profile:
  `verify-current.ps1 -DelaySeconds 3` passed.
- Deferred: continuous natural ledge attack/escape runtime, full player-driven
  ledge decisions, and unbounded fighter gameplay remain deferred.

## 2026-06-28 - Added Bounded MakeAttackColl Event Decoder Proof

- Kept the current Passive recover boundary unchanged and reused the existing
  direct/menu-chain Dash -> Run -> RunBrake harness pair.
- Added a narrow project-owned `FTAttackColl` shadow to `include/ft/fighter.h`
  and made `ftParamClearAttackCollAll` clear the shadow state instead of acting
  only as a counter.
- Added a bounded original-layout `MakeAttackColl` decoder beside
  `ftMainPlayAnimEventsAll`. The fixture uses the same field layout as
  BattleShip `ftMotionCommandMakeAttackCollS*` macros and writes the decoded
  values into `FTStruct.attack_colls[0]`.
- Extended the direct/menu-chain dash-run verifiers with
  `DASH_RUN_ATTACK_EVENT`. Both now require event mask/count `0x3f/6`, decoded
  damage `7`, size `3000`, offset `1200/2400/-800`, angle `361`,
  KBG/KBW/BKB `90/20/30`, shield `2`, and air/ground/rebound flags `0x7`.
- Direct proof:
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3` passed with
  `event=0x3f/6 dmg=7 size=3000 off=1200/2400/-800`.
- Menu-chain proof:
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3` passed
  with the same event marker after the VS menu chain.
- Deferred: real fighter motion-script event hookup, full `ftmain.c`
  animation command runtime, hitbox activation, fighter-vs-fighter hit
  detection, hitlag/damage interaction, and continuous attack gameplay.

## 2026-06-28 - Replaced MakeAttackColl Fixture With Selected Original Motion Commands

- Kept the current Passive recover boundary unchanged and reused the existing
  direct/menu-chain Dash -> Run -> RunBrake harness pair.
- Added `src/import/battleship_mariofox_mainmotion.c` as a bounded selected
  command extract from BattleShip `202_MarioMainMotion.c` and
  `208_FoxMainMotion.c` instead of compiling the full relocData files. The full
  files currently pull in broad item/map/audio enum surfaces outside this
  proof.
- Updated `ftMainPlayAnimEventsAll` to install the selected original
  Attack11/Attack12/Mario Attack13/Fox Attack100Start command stream before
  scanning for `MakeAttackColl`.
- Extended `DASH_RUN_ATTACK_EVENT` to report decoded-hit mask, reached-script
  mask, no-hit script mask, and parse count. Current direct/menu-chain proof
  expects `0x1f/0x3f/0x20/5`.
- The current source-order last decoded hitbox is Fox Jab2:
  `damage=4`, `size=100`, `offset=140/0/0`, `angle=70`,
  `KBG/KBW/BKB=100/0/0`, `shield=0`, and flags `0x7`.
- Direct proof:
  `verify-battle-mariofox-dash-run-harness.ps1` passed with
  `event=0x1f/0x3f/0x20/5 dmg=4 size=100 off=140/0/0`.
- Menu-chain proof:
  `verify-menu-chain-mariofox-dash-run-harness.ps1` passed with the same event
  marker after the VS menu chain.
- Deferred: full motion-script relocData import, full animation command
  runtime, hitbox activation, fighter-vs-fighter hit detection,
  hitlag/damage interaction, and continuous attack gameplay.

## 2026-06-28 - Extended Selected MakeAttackColl Scan To Second Hitboxes

- Kept the current Passive recover boundary unchanged and reused the existing
  direct/menu-chain Dash -> Run -> RunBrake harness pair.
- Extended the selected Mario/Fox main-motion command extract to include the
  second original `MakeAttackColl` command for Mario Jab1, Mario Jab2, Mario
  Jab3, Fox Jab1, and Fox Jab2.
- Updated the bounded `ftMainPlayAnimEventsAll` scanner to continue through
  multiple selected `MakeAttackColl` commands before stopping at `End` or
  `Pause`, while preserving Fox Attack100Start as the real no-hit script.
- Updated the direct/menu-chain dash-run verifiers to require
  `DASH_RUN_ATTACK_EVENT=0x1f/0x3f/0x20/10`. The source-order last decoded
  hitbox is now Fox Jab2 attack ID `1`: `damage=4`, `size=100`,
  `offset=0/0/0`, `angle=70`, `KBG/KBW/BKB=100/0/0`, `shield=0`, and flags
  `0x7`.
- Deferred: full motion-script relocData import, full animation command
  runtime, command-driven clear/size/damage mutation, hitbox activation,
  fighter-vs-fighter hit detection, hitlag/damage interaction, and continuous
  attack gameplay.

## 2026-06-28 - Added Selected Attack Damage/Size/Clear Command Scan

- Kept the active Passive recover boundary unchanged and reused the existing
  direct/menu-chain Dash -> Run -> RunBrake harness pair.
- Extended the selected Mario Jab3 main-motion extract with the original
  `SetAttackCollDamage`, `SetAttackCollSize`, and `ClearAttackCollAll` command
  words from `202_MarioMainMotion.c`.
- Updated the bounded `ftMainPlayAnimEventsAll` scanner to apply those command
  words to the local `FTStruct.attack_colls` shadows and to use the existing
  `ftParamClearAttackCollAll` seam for the clear command.
- Added `gNdsFighterDashRunAttackEventCommandMask`; the direct/menu-chain
  dash-run verifiers now require `DASH_RUN_ATTACK_EVENT_CMDS=0xf` for damage,
  size, clear, and clear-off state.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  and `clean-generated.ps1 -DryRun`.
- Deferred: full motion-script relocData import, full animation command
  runtime, staled-damage handling after `SetAttackCollDamage`, hitbox
  activation, fighter-vs-fighter hit detection, hitlag/damage interaction, and
  continuous attack gameplay.

## 2026-06-28 - Extended Pass-Input Proof To SquatRv Release

- Kept the active Passive recover boundary unchanged and reused the existing
  direct/menu-chain pass-input proof inherited by modes `155/156`.
- Reused the already imported BattleShip `ftcommonsquat.c` path to prove
  SquatWait -> SquatRv -> Wait after the existing Wait -> Squat -> Pass
  drop-through proof.
- Extended the bounded pass-input `ftMainSetStatus` seam to accept
  SquatWait/SquatRv/Wait and install the original-compatible callback slots:
  `ftCommonSquatWaitProcUpdate`, `ftCommonSquatWaitProcInterrupt`,
  `ftAnimEndSetWait`, and `ftCommonSquatRvProcInterrupt`.
- Added `STAGE_MPPASS_INPUT_SQUATRV`; current proof expects set/tick counts
  `1/1/1/1`, status `29 -> 30 -> 10`, callback mask `0xf`, and final
  ground state `0`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppass-input-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.
- Deferred: continuous crouch movement, full ground interrupt scanning,
  player-driven crouch/pass/squat release scheduling, and full `ftmain.c`.

## 2026-06-28 - Added Bounded GuardSetOff Proof

- Kept the active Passive recover boundary unchanged and reused the existing
  direct/menu-chain Dash -> Run -> RunBrake harness pair inherited by current
  modes `155/156`.
- Extended the local dash-run `ftMainSetStatus` guard seam to admit original
  `nFTCommonStatusGuardSetOff` only while the bounded guard proof is active.
  The status keeps the original `-1` script behavior by preserving the current
  Guard motion while installing `ftCommonGuardSetOffProcUpdate`.
- Drove imported original `ftCommonGuardSetOffSetStatus` with deterministic
  shield damage `10`, proving `setoff_frames=20200` milli-units and ground
  velocity `-40400`, then ticked the installed original SetOff update once for
  held-Z return to Guard and once for released-Z return to GuardOff.
- Added `DASH_RUN_GUARD_SETOFF`; direct/menu-chain dash-run verifiers require
  counts `4/4`, mask `0xfff`, callback mask `0xff`, frames `20200`, and
  velocity `-40400`. The inherited dash-run aggregate is now
  `dashRun=0xfffffff`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-current.ps1 -DelaySeconds 3`.
- Deferred: continuous Guard/SetOff/Escape shield runtime, real shield
  collision, player-driven shield scheduling, shield visuals, and full
  `ftmain.c`.

## 2026-06-28 - Added Bounded Escape Callback Tick Proof

- Extended the existing direct/menu-chain Dash -> Run -> RunBrake proof without
  adding a new harness or importing another BattleShip translation unit.
- Reused the imported original `ftcommonescape.c` wrapper and the existing
  Guard -> Escape setup to tick the installed original Escape callbacks once
  per fighter.
- Added guarded diagnostics for `DASH_RUN_ESCAPE` so the verifier now proves
  `tick=0x3ff`: original `ftCommonEscapeProcUpdate` consumes
  `motion_vars.flags.flag1` and flips LR, original
  `ftCommonEscapeProcInterrupt` reaches the light-throw seam, the installed
  physics/map callbacks reach bounded compatibility seams, and animation end
  returns through original `ftCommonWaitSetStatus` to Wait/Ground.
- Raised the inherited dash-run aggregate proof from `dashRun=0xfffffff` to
  `dashRun=0x1fffffff`, so modes `155/156` kept this Escape callback tick
  proof covered under the live VSBattle/Pupupu root at that milestone.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-current.ps1 -DelaySeconds 3`.
- Continuous shield-roll runtime, shield collision, player-driven guard escape,
  and full fighter scheduling remain deferred.

## 2026-06-28 - Passive Recover Appeal GuardOn Branch Proof

- Extended the current direct/menu-chain Passive recover modes `155/156` with a
  bounded source-order Appeal/Taunt interrupt branch proof.
- After the existing Appeal no-catch/no-guard callback tick and
  `ftAnimEndSetWait` handoff, the proof re-enters Appeal through imported
  original `ftCommonAppealCheckInterruptCommon`, sets
  `motion_vars.flags.flag1`, calls the installed original
  `ftCommonAppealProcInterrupt`, proves the catch seam returns false, and then
  routes through imported original GuardOn setup to status/motion `152/134`.
- Added Appeal-specific GuardOn diagnostics instead of reusing the Dash-Run
  guard proof counters. The branch uses a temporary shield animation/DObj
  lookup fixture around original GuardOn joint setup and restores the fighter
  to Wait/Ground afterward so this remains a boundary proof.
- Updated the shared gcDrawAll verifier marker contract:
  `STAGE_MPPASSIVE_APPEAL` now expects two Appeal check/set-status entries,
  `STAGE_MPPASSIVE_APPEAL_GUARD` requires mask `0x7f`, and the recover
  Passive proof mask increases to `0x7ff`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: continuous Appeal/Taunt runtime, continuous Guard/Shield runtime,
  real shield visuals/collision, player-driven guard scheduling, catch
  gameplay, and full `ftmain.c`.

## 2026-06-28 - Added Isolated melonDS Runner Slots

- Added local generated melonDS runner slots under
  `emulators/melonds-runners/slotN`, with per-slot executable links/copies,
  persistent `melonDS.toml`, and deterministic ARM9/ARM7 GDB ports
  `3333 + slot * 10` and `3334 + slot * 10`.
- Threaded `-RunnerSlot`, `-GdbPort`, and `-NoBuild` through verifier profiles,
  harness wrappers, runtime/title/opening samplers, and shared GDB marker
  helpers. Slot runs use `artifacts/emulator-logs/slotN` and
  `artifacts/verifier-temp/slotN`.
- Added deterministic profile sharding in `verify-all.ps1` and a conservative
  `Start-VerifyRegressionShards.ps1` launcher/list helper. Parallel shards are
  intended to use prebuilt outputs plus `-NoBuild` to avoid shared build-output
  races.
- Verified:
  `New-MelonDSRunnerSlots.ps1 -Count 2 -List`,
  `verify-all.ps1 -Profile Boundary -ShardCount 2 -ShardIndex 0 -RunnerSlot 0 -List`,
  `verify-all.ps1 -Profile Boundary -ShardCount 2 -ShardIndex 1 -RunnerSlot 1 -List`,
  `build-verify-profile.ps1 -Profile Boundary`,
  both Boundary shards concurrently with `-NoBuild -DelaySeconds 3`,
  `verify-regression.ps1 -List`, and
  `verify-boundary.ps1 -DelaySeconds 3`.
- Deferred: four-way full Regression stress testing. Use the 2-slot Boundary
  proof as the current smoke test before scaling parallel regression.

## 2026-06-28 - Added Bounded Original WallDamage Proof

- Imported original BattleShip `ftcommonwalldamage.c` through
  `src/import/battleship_ftcommon_walldamage.c` and kept the new proof folded
  into current modes `155/156`.
- Added the narrow fighter struct/status fields and diagnostics needed by the
  original WallDamage path, plus bounded compatibility seams for impact wave,
  quake, rumble, timed intangible, and the DamageFall handoff.
- The proof seeds the selected Hyrule wall-hit source (`floor=5`, `wall=13`,
  `edge=12`), calls original `ftCommonWallDamageCheckGoto`, proves reflected
  knockback into WallDamage status/motion `56/49`, then ticks original
  `ftCommonWallDamageProcUpdate` once into bounded DamageFall `57/50`.
- Fixed verifier parsing for `STAGE_MPPASSIVE_WALLDAMAGE`,
  `STAGE_MPPASSIVE_WALLDAMAGE_STATE`, and `STAGE_MPPASSIVE_WALLDAMAGE_VEC`;
  the current passive mask is now `0xfff`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: natural wall collision/copyback scheduling, wall teching,
  hitlag/damage interaction, full DamageFall/WallDamage runtime, and full
  `ftmain.c`.

## 2026-06-28 - Added Bounded Original Rebound Proof

- Imported original BattleShip `ftcommonrebound.c` through
  `src/import/battleship_ftcommon_rebound.c` and kept the proof folded into
  current modes `155/156`.
- Added the narrow local fighter shell surface needed by the original path:
  ReboundWait/Rebound status IDs `82/83`, Rebound motion `71`,
  `FTStatusVars.common.rebound`, and `FTStruct.hit_lr`.
- The proof seeds original-compatible `attack_rebound`, `hit_lr`, and
  `rebound_anim_length`, calls original `ftCommonReboundWaitSetStatus`, proves
  ReboundWait/Ground `82/-1`, lets the installed original ReboundWait update
  transition into Rebound/Ground `83/71`, then ticks original Rebound once
  into the existing Wait/Ground handoff `10/4`.
- Added `STAGE_MPPASSIVE_REBOUND`, `STAGE_MPPASSIVE_REBOUND_STATE`, and
  `STAGE_MPPASSIVE_REBOUND_VEC` verifier markers; the current passive-recover
  mask is now `0x1fff`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: real attack/shield rebound triggers, continuous Rebound runtime
  outside this bounded timer handoff, hitlag/damage integration, and full
  `ftmain.c`.

## 2026-06-28 - Added Bounded Original TurnRun Proof

- Imported original BattleShip `ftcommonturnrun.c` through
  `src/import/battleship_ftcommon_turnrun.c` and kept the proof folded into the
  older direct/menu-chain Dash-Run modes plus current modes `155/156`.
- Added the narrow original-compatible `FTCOMMON_TURNRUN_STICK_RANGE_MIN`
  constant and diagnostics for `DASH_RUN_TURNRUN`.
- The proof drives Run -> TurnRun -> Run through the installed original
  `ftCommonRunProcInterrupt` path, verifies TurnRun status/motion `19/13`,
  final Run status/motion `16/10`, callback/update masks `0xff/0xf`, four
  guarded update ticks total, and LR/ground-velocity flips for both fighters.
- Raised the inherited dash-run aggregate marker to `dashRun=0x3fffffff` in
  the direct/menu-chain Dash-Run verifiers and the current boundary verifier.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-boundary.ps1 -DelaySeconds 3`.
- Deferred: continuous player-driven TurnRun movement and broad fighter runtime
  beyond this isolated callback/update handoff proof.

## 2026-06-28 - Added Bounded Original Catch Status Proof

- Imported original BattleShip `ftcommoncatch1.c` through
  `src/import/battleship_ftcommon_catch.c` and folded the bounded proof into
  current modes `155/156`.
- Added the narrow local fighter shell surface needed by the original Catch
  entry path: Catch/CatchPull motion and status IDs, catch status vars,
  catch/capture callback fields, catch search fields, and the catch-param
  compatibility seam.
- The proof seeds Wait/Ground Z-hold plus A-tap, calls original
  `ftCommonCatchCheckInterruptCommon`, proves the original path reaches
  Catch/Ground status/motion `166/146`, records the expected Catch callback
  shape, and verifies item throw returned false while CatchPull/CapturePulled
  deferred seams were not executed.
- Raised the current Passive-recover proof mask to `0x3fff` and added the
  `STAGE_MPPASSIVE_CATCH` verifier marker plus `catch=166/146` summary output.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: full CatchPull/CatchWait/capture/throw runtime, item throw
  branches, player-driven grab gameplay, and full `ftmain.c`.

## 2026-06-28 - Added Bounded Original Catch Callback Handoff Proof

- Extended the current direct/menu-chain Passive recover modes `155/156` from
  the earlier Catch status proof into one installed original Catch map callback
  and one installed original Catch update callback.
- The proof now records `STAGE_MPPASSIVE_CATCH_CALLBACKS`, verifies one
  `ftCommonCatchProcMap` tick reaches the bounded
  `mpCommonCheckFighterOnEdge` seam while preserving Catch/Ground `166/146`,
  then forces `ftCommonCatchProcUpdate` through `ftAnimEndSetWait` into
  Wait/Ground `10/4`.
- Raised the Catch proof requirement from mask `0x7f` to `0xff` and updated
  the current Passive summary to `catch=166/146->10/4`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`, and
  `verify-current.ps1 -DelaySeconds 3`.
- Deferred: full CatchPull/CatchWait/capture/throw runtime, item throw
  branches, player-driven grab gameplay, and full `ftmain.c`.

## 2026-06-28 - Added Bounded Original CatchPull/CatchWait Proof

- Extended `src/import/battleship_ftcommon_catch.c` to import original
  BattleShip `ftcommoncatch2.c` beside the existing bounded `ftcommoncatch1.c`
  import.
- Added the narrow original-compatible fighter shell surface needed by this
  slice: `FTStruct.proc_slope`, `FTCATCHKIND_MASK_ALL`,
  `FTCOMMON_CATCH_THROW_WAIT`, the CatchPull/CatchWait declarations, and the
  model-part / throw-check compatibility seams.
- Kept the proof folded into current direct/menu-chain Passive recover modes
  `155/156`; no new harness pair was added.
- The proof now records `STAGE_MPPASSIVE_CATCH_PULL`, drives original
  `ftCommonCatchPullProcCatch` into CatchPull/Ground status/motion `167/147`,
  verifies `catch_gobj` is wired to the seeded target, records capture-immune,
  catch-swirl, and rumble seams, ticks original `ftCommonCatchPullProcUpdate`
  into CatchWait/Ground `168/-2`, and ticks original
  `ftCommonCatchWaitProcInterrupt` once with throw check stubbed false so
  `throw_wait` decrements `60->59`.
- Raised the Catch proof requirement from mask `0xff` to `0x1ff` and updated
  the current Passive summary to include
  `catchPull=167/147->168/-2 tw=60->59`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: CapturePulled, throw execution, item throw branches, continuous
  CatchPull/CatchWait runtime, player-driven grab gameplay, and full
  `ftmain.c`.

## 2026-06-28 - Added Bounded Original CapturePulled/CaptureWait Proof

- Extended `src/import/battleship_ftcommon_catch.c` to import original
  BattleShip `ftcommoncapturepulled.c` and `ftcommoncapturewait.c` beside the
  existing bounded Catch/CatchPull imports.
- Added the narrow compatibility surface required by this victim-side slice:
  item weight shell data, CapturePulled/CaptureWait status and motion IDs,
  voice-stop/drop-item/thrown-release hit-status seams, simple transform
  helpers used by original capture positioning, and
  `mpCommonSetFighterProjectFloor` for the installed CaptureWait map callback.
- Kept the proof folded into current direct/menu-chain Passive recover modes
  `155/156`; no new harness pair was added.
- The proof now records `STAGE_MPPASSIVE_CAPTURE`, drives the seeded victim
  through original `ftCommonCapturePulledProcCapture` into
  CapturePulled/Ground status/motion `171/150`, verifies the captured GObj and
  LR flip, records voice-stop, velocity-stop, and capture-immune seams, ticks
  original `ftCommonCapturePulledProcPhysics` into CaptureWait/Ground
  `172/-2`, and runs one installed `ftCommonCaptureWaitProcMap` callback.
- Raised the Catch proof requirement from mask `0x1ff` to `0x3ff` and updated
  the current Passive summary to include `capture=171/150->172/-2`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: throw execution, item throw branches, continuous grab/capture
  runtime, player-driven grab gameplay, and full `ftmain.c`.

## 2026-06-28 - Added Bounded Original ThrowF/ThrownCommon Handoff Proof

- Extended `src/import/battleship_ftcommon_catch.c` to import original
  BattleShip `ftcommonthrow.c` and `ftcommonthrown1.c` beside the existing
  bounded Catch/CatchPull/Capture imports.
- Added the narrow fighter shell surface needed by this status handoff:
  ThrowF/ThrowB/ThrownCommon motion and status IDs, original-compatible
  thrown-status table structs, `FTStruct.is_ignore_dead`, and the catch-throw
  stick/kind constants used by the original throw interrupt path.
- Kept the proof folded into current direct/menu-chain Passive recover modes
  `155/156`; no new harness pair was added.
- The proof now records `STAGE_MPPASSIVE_THROW`, drives original
  `ftCommonThrowCheckInterruptCatchWait` into ThrowF/Ground status/motion
  `169/148`, queues the seeded target through original thrown-status data into
  ThrownCommon/Air `186/161`, and records the throw anim-event plus
  capture-immune seams.
- Raised the Catch proof requirement from mask `0x3ff` to `0x7ff` and updated
  the current Passive summary to include `throw=169/148->186/161`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3 -NoBuild`, and
  `verify-current.ps1 -DelaySeconds 3`.
- Deferred: throw release/damage runtime, item throw branches, continuous
  grab/capture/throw runtime, player-driven grab gameplay, and full
  `ftmain.c`.

## 2026-06-28 - Added Bounded Original Thrown Release/Update-Stats Proof

- Extended `src/import/battleship_ftcommon_catch.c` to import original
  BattleShip `ftcommonthrown2.c` beside the existing bounded
  Catch/CatchPull/Capture/Throw imports.
- Added the narrow fighter shell surface needed by this slice:
  `FTThrowHitDesc`, `FTStruct.throw_desc`, normal hit element/log constants,
  and the throw-release damage/stat/stale compatibility seams used by
  `ftCommonThrownReleaseThrownUpdateStats`.
- Kept the proof folded into current direct/menu-chain Passive recover modes
  `155/156`; no new harness pair was added.
- The proof now records `STAGE_MPPASSIVE_THROW_RELEASE`, seeds an
  original-compatible throw hit descriptor, calls the imported original
  `ftCommonThrownReleaseThrownUpdateStats`, verifies victim damage `10->18`,
  clears capture state, forces Air, installs the original thrown proc-status
  callback with script ID `123`, and records the damage-init, damage update,
  1P/player stat, stale queue, and rumble seams.
- Raised the recover aggregate requirement from `0x7fff` to `0xffff` and
  updated the current Passive summary to include
  `throwRelease=dmg=10->18 kb=6600000 script=123`.
- Verified before docs update:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: full throw release status runtime, no-damage release, item throw
  branches, hitlag/full damage status runtime, stale queue internals,
  continuous grab/capture/throw scheduling, player-driven grab gameplay, and
  full `ftmain.c`.

## 2026-06-28 - Added Bounded Original Thrown Damage/No-Damage Release Status Proof

- Folded the next `ftcommonthrown2.c` slice into current direct/menu-chain
  Passive recover modes `155/156`; no new harness pair was added.
- Added guarded public wrappers for imported original
  `ftCommonThrownSetStatusDamageRelease`,
  `ftCommonThrownUpdateDamageStats`, and
  `ftCommonThrownSetStatusNoDamageRelease` while preserving the previous
  no-op behavior outside the current proof guard.
- Added `STAGE_MPPASSIVE_THROW_RELEASE_STATUS` diagnostics and raised the
  recover aggregate requirement from `0xffff` to `0x1ffff`.
- The new marker proves damage release `20->26`, update-damage-stats
  `30->36`, no-damage release `40->40` with zero queued damage, capture clear,
  hitstatus normalization, release LR, and guard cleanup.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: item throw branches, hitlag/full damage status runtime, stale queue
  internals, continuous grab/capture/throw scheduling, player-driven grab
  gameplay, and full `ftmain.c`.

## 2026-06-28 - Added Bounded Original Thrown Proc-Status Tick Proof

- Folded the next `ftcommonthrown2.c` slice into current direct/menu-chain
  Passive recover modes `155/156`; no new harness pair was added.
- Added `STAGE_MPPASSIVE_THROW_PROC_STATUS` diagnostics and raised the recover
  aggregate requirement from `0x1ffff` to `0x3ffff`.
- The new marker ticks the installed original `ftCommonThrownProcStatus`
  callback once after `ftCommonThrownReleaseThrownUpdateStats` installs it,
  proves the callback reaches project-owned `ftParamSetThrowParams`, confirms
  the seeded catcher is stored as `throw_gobj`, and records script ID `123`.
- Verified:
  `make -j16`,
  `verify-dev-fast.ps1 -DelaySeconds 3`, and
  `verify-boundary.ps1 -DelaySeconds 3`.
- Deferred: item throw branches, hitlag/full damage status runtime, continuous
  thrown/damage status scheduling, player-driven grab gameplay, and full
  `ftmain.c`.

## 2026-06-28 - Added Bounded Original Thrown Dead-Result Cleanup Proof

- Folded the next `ftcommonthrown2.c` cleanup slice into current direct/menu-chain
  Passive recover modes `155/156`; no new harness pair was added.
- Added guarded public wrappers for imported original
  `ftCommonThrownDecideFighterLoseGrip`,
  `ftCommonThrownReleaseFighterLoseGrip`, and
  `ftCommonThrownDecideDeadResult`, plus an active-only `ftCommonFallSetStatus`
  seam for this cleanup proof.
- Added `STAGE_MPPASSIVE_THROW_DEAD_RESULT` diagnostics and raised the recover
  aggregate requirement from `0x3ffff` to `0x7ffff`.
- The marker proves the original dead-result call, collision-default, SetAir,
  two Wait/Fall resolutions, catch/capture pointer clearing, and final
  Wait/Ground plus Fall/Air cleanup:
  `throwDead=call=1 coll=1 air=1 waitFall=2 status=10/26`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3 -NoBuild`,
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and the direct verifier again after the summary update.
- Deferred: continuous death/throw/damage scheduling, item throw branches,
  player-driven grab gameplay, full damage runtime, and full `ftmain.c`.

## 2026-06-28 - Added Bounded Original ThrownCommon Callback Tick Proof

- Folded the next `ftcommonthrown1.c` slice into current direct/menu-chain
  Passive recover modes `155/156`; no new harness pair was added.
- Added `STAGE_MPPASSIVE_THROW_CALLBACK` diagnostics and raised the recover
  aggregate requirement from `0x7ffff` to `0xfffff`.
- The new marker proves the installed original ThrownCommon update, physics,
  and map callback slots are present, ticks each once, preserves
  ThrownCommon/Air `186/161/1`, and proves the original map callback copies the
  captor floor line:
  `throwCb=1/1/1 floor=3`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: continuous thrown animation, release scheduling, item throw
  branches, player-driven grab gameplay, full damage runtime, and full
  `ftmain.c`.

## 2026-06-28 - Added Bounded Original Neutral AttackAirN Proof

- Imported BattleShip `ftcommonattackair.c` through the project-owned
  `src/import` wrapper path and extended the existing direct/menu-chain
  Jump-loop proof instead of adding a new harness pair.
- The Jump-loop proof now seeds one guarded A-button neutral aerial after the
  six bounded JumpF air frames, calls original
  `ftCommonAttackAirCheckInterruptCommon`, and proves the bounded
  `ftMainSetStatus` seam reaches AttackAirN/Air status/motion `209/184`.
- Added `JUMP_ATTACKAIR` diagnostics. The maintained marker expects one
  successful original check, one status setup, one `ftMainSetStatus` entry, one
  animation-event seam call, attack IDs `12/9/9`, reset
  `tics_since_last_z=65536`, and callback mask `0xf`.
- During verification, the direct Walk-input/Jump-loop path initially failed
  because stale harness objects still had imported BattleShip code compiled
  against an older `FTStruct` layout. A forced rebuild corrected the imported
  field offset. Treat wrong-offset imported reads after header edits as a build
  hygiene issue first; use `make -B` or `make clean`.
- Verified:
  `verify-battle-mariofox-walk-input-harness.ps1 -DelaySeconds 8`,
  `verify-battle-mariofox-jump-loop-harness.ps1 -DelaySeconds 8`,
  and
  `verify-menu-chain-mariofox-jump-loop-harness.ps1 -DelaySeconds 8`.
- Deferred: continuous aerial attack runtime, hitbox activation/hit detection,
  landing-lag integration, full animation command runtime, items/specials, and
  full `ftmain.c`.

## 2026-06-28 - Added Bounded Original ThrowF Update Release Proof

- Folded one installed original `ftCommonThrowProcUpdate` tick into current
  direct/menu-chain Passive recover modes `155/156`; no new harness pair was
  added.
- Added isolated `STAGE_MPPASSIVE_THROW_UPDATE` diagnostics so this callback
  release path does not disturb the existing direct
  `STAGE_MPPASSIVE_THROW_RELEASE` marker.
- The proof seeds ThrowF/Ground with `flag2` set, keeps animation time above
  zero, reaches imported original thrown update-stats, clears the catcher
  `catch_gobj`, clears victim capture, installs thrown proc-status, and records
  `throwUpdate=169/148 dmg=50->58 script=0`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: continuous throw animation scheduling, item throw branches, full
  damage runtime, player-driven grab gameplay, and full `ftmain.c`.

## 2026-06-28 - Added Bounded Original ThrowB Branch Proof

- Extended the current direct/menu-chain Passive recover modes `155/156`;
  no new harness pair was added.
- Reused imported BattleShip `ftcommonthrow.c` / `ftcommonthrown1.c` and the
  existing catch/throw proof block.
- After the existing ThrowF proof passes, the current boundary now reseeds
  original `ftCommonThrowCheckInterruptCatchWait` with stick-left input and
  positive LR to prove ThrowB/Ground status/motion `170/149`.
- Added `STAGE_MPPASSIVE_THROW_B` diagnostics and raised the throw marker
  requirement to `0x3ff`; the current summary now includes
  `throwB=170/149->186/161`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: item throw branches, continuous grab/capture/throw runtime,
  player-driven grab gameplay, full damage runtime, and full `ftmain.c`.

## 2026-06-28 - Added Bounded Original ThrownCommon Animation-End Branch Proof

- Extended the current direct/menu-chain Passive recover modes `155/156`;
  no new harness pair was added.
- Reused imported BattleShip `ftcommonthrown1.c` and the existing
  `STAGE_MPPASSIVE_THROW_CALLBACK` marker instead of adding a second marker.
- Added a narrow proof guard for original `ftCommonThrownProcUpdate` when
  `anim_frame <= 0`. The branch reaches original
  `ftCommonThrownSetStatusImmediate`, then the existing project-owned
  `ftMainSetStatus`, animation-event, and capture-immune seams.
- The marker now requires mask `0x1ff` and reports
  `throwCb=1/1/1 floor=3 end=1/186/161`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Deferred: continuous thrown animation, release scheduling, item throw
  branches, player-driven grab gameplay, full damage runtime, and full
  `ftmain.c`.

## 2026-06-28 - Isolated Jump AttackAir Probe From Downstream Jump Prerequisites

- Kept the bounded original neutral AttackAirN proof in the standalone
  direct/menu-chain Jump-loop harnesses only.
- Added a narrow `NDS_MARIOFOX_JUMP_ATTACKAIR_HARNESS` guard so downstream
  Landing/process/current boundary harnesses still consume the non-mutating
  JumpF/Air handoff and require the older Jump-loop `0x7ff` prerequisite mask.
- This fixed the current Passive recover boundary regression where the
  AttackAir status mutation prevented later landing/process prerequisites from
  seeing both fighters in JumpF/Air.
- Verified:
  `make -j16`,
  forced rebuilds for the direct/menu Jump-loop, Landing-loop, and current
  Passive recover harness targets,
  `verify-battle-mariofox-jump-loop-harness.ps1 -NoBuild -DelaySeconds 8`,
  `verify-menu-chain-mariofox-jump-loop-harness.ps1 -NoBuild -DelaySeconds 8`,
  `verify-battle-mariofox-landing-loop-harness.ps1 -NoBuild -DelaySeconds 8`,
  and `verify-boundary.ps1 -NoBuild -DelaySeconds 8`.

## 2026-06-28 - Added Bounded AttackAirLw Refresh Seam Proof

- Kept the active Passive recover boundary unchanged and extended only the
  standalone direct/menu-chain Jump-loop `JUMP_ATTACKAIR` proof.
- Reused imported BattleShip `ftcommonattackair.c` and called original
  `ftCommonAttackAirLwProcUpdate` in a bounded Link rehit setup so the source
  path reaches `ftParamRefreshAttackCollID` for attack coll IDs `0/1`.
- Replaced the previous no-op refresh seam with a project-owned compatibility
  implementation that marks `FTAttackColl.attack_state` as
  `nGMAttackStateNew`, sets `FTStruct.is_attack_active`, and records refresh
  count/mask/state diagnostics. Original attack-record clearing stays
  deferred because the local `FTAttackColl` shell does not yet include the
  original record table.
- Verified:
  `make -j16`,
  forced rebuilds for the direct/menu Jump-loop harness targets,
  `verify-battle-mariofox-jump-loop-harness.ps1 -NoBuild -DelaySeconds 8`,
  `verify-menu-chain-mariofox-jump-loop-harness.ps1 -NoBuild -DelaySeconds 8`,
  and `verify-boundary.ps1 -DelaySeconds 3`.

## 2026-06-28 - Added AttackAir Refresh Record Clearing

- Expanded the project-owned `FTAttackColl` shell with the original four
  `GMAttackRecord` slots and added the narrow original-compatible
  `ftParamClearAttackRecordID` seam.
- `ftParamRefreshAttackCollID` now matches the BattleShip source path for the
  bounded proof: mark the selected attack coll new, set attack active, then
  clear the selected attack records to `victim_gobj=NULL`, hurt/shield false,
  `timer_rehit=0`, and `group_id=7`.
- Extended the existing direct/menu-chain Jump-loop `JUMP_ATTACKAIR` marker
  with record-clear mask `0x3` for AttackAirLw refresh IDs `0/1`; no new
  harness pair was added and the active Passive recover boundary is unchanged.
- Verified:
  `make -j16`,
  forced rebuilds for the direct/menu Jump-loop harness targets,
  `verify-battle-mariofox-jump-loop-harness.ps1 -NoBuild -DelaySeconds 8`,
  `verify-menu-chain-mariofox-jump-loop-harness.ps1 -NoBuild -DelaySeconds 8`,
  and `verify-boundary.ps1 -DelaySeconds 3`.
- Deferred: continuous hit detection, victim record insertion, rehit timers,
  and full fighter/item/weapon attack-record interaction.

## 2026-06-28 - Added Bounded Attack Hitbox Position Step

- Added original-compatible `FTAttackMatrix` storage to the project-owned
  `FTAttackColl` shell.
- Extended the direct/menu-chain Dash-Run `MakeAttackColl` proof with
  `DASH_RUN_ATTACK_EVENT_POS`. The bounded helper copies the selected decoded
  Fox Jab2 hitbox, follows the original `ftmain.c` `New -> Transfer` path,
  applies scale when needed, calls `gmCollisionGetFighterPartsWorldPosition`,
  and resets the original attack matrix fields.
- Verified the marker with mask `0x1f`, state `2`, attack ID `1`, joint ID
  `14`, and matrix `0/0` in both Dash harnesses, then reran the current
  boundary profile. This is still a bounded copy-based position proof; live
  hitbox activation and collision remain deferred.

## 2026-06-28 - Added Selected Attack Hitbox Position Writeback

- Extended the same direct/menu-chain Dash-Run position proof with one gated
  live writeback for the selected decoded Fox Jab2 hitbox.
- The bounded helper still follows the original `ftmain.c` `New -> Transfer`
  offset/scale/world-position/matrix-reset path, then writes the selected
  result back into `FTStruct.attack_colls[1]` only for Fox `Attack12`, attack
  ID `1`, joint `14`, damage `4`, and size `100`.
- Updated the `DASH_RUN_ATTACK_EVENT_POS` verifier contract from mask `0x1f`
  to `0x3f` so the direct and menu-chain Dash harnesses prove the selected
  live state reaches Transfer. Continuous hitbox activation, hit detection,
  victim records, hitlag, and damage interaction remain deferred.

## 2026-06-28 - Added Selected Attack Hitbox Interpolate Step

- Extended the same selected Fox Jab2 position proof through the next original
  `ftmain.c` attack-collision state: `Transfer -> Interpolate`.
- The bounded helper copies the live `pos_curr` into `pos_prev`, recomputes
  the joint world position from the selected offset, and resets the original
  attack matrix fields again.
- Updated `DASH_RUN_ATTACK_EVENT_POS` from mask `0x3f` / state `2` to mask
  `0xff` / state `3` in the direct and menu-chain Dash harnesses. Continuous
  hitbox activation, hit detection, victim records, hitlag, and damage
  interaction remain deferred.

## 2026-06-28 - Added Selected Attack Broad-Phase Range Proof

- Exposed the original `FTAttributes.hit_detect_range` slot in the local
  fighter compatibility header without changing the surrounding layout.
- Added a bounded local copy of the original
  `gmCollisionCheckAttackInFighterRange` predicate and applied it only to the
  selected Fox Jab2 hitbox after the existing `New -> Transfer -> Interpolate`
  writeback proof.
- Updated `DASH_RUN_ATTACK_EVENT_POS` from mask `0xff` to `0x3ff` in the
  direct and menu-chain Dash harnesses, proving current and previous hitbox
  positions can pass the original broad-phase fighter-range gate. Hurtbox
  collision, victim records, hitlag, damage, and continuous attack runtime
  remain deferred.

## 2026-06-28 - Promoted Selected Attack Range Guard Into Current Boundary

- Updated the shared `verify-battle-mariofox-gcdrawall-loop-harness.ps1`
  current-boundary verifier path so modes `155/156` assert
  `DASH_RUN_ATTACK_EVENT_POS` alongside the existing `dashRun=0x3fffffff`
  aggregate.
- Current Passive recover direct/menu-chain verifiers now report
  `dashRun=0x3fffffff hitboxPos=0x3ff`, keeping the selected Fox Jab2
  `New -> Transfer -> Interpolate` and broad-phase fighter-range proof covered
  under the latest VSBattle/Pupupu root.
- No gameplay behavior changed. Hurtbox collision, victim records, hitlag,
  damage, and continuous attack runtime remain deferred.

## 2026-06-28 - Added Bounded Mario/Fox Damage-Collision State Shell

- Added a project-owned `FTDamageColl[11]` compatibility shell to `FTStruct`
  and made the bounded hit-status part helpers update active damage-coll slots
  instead of only incrementing diagnostics.
- Seeded one root-anchored active damage-collision slot for each initialized
  Mario/Fox fighter using the original copy-and-half-size shape
  (`30.0/45.0/20.0`) while keeping the real per-fighter
  `attr->damage_coll_descs[]` import deferred.
- Updated the direct and menu-chain Mario/Fox init verifiers to require
  `damageColl=0x3/0x3/0x3/0x3`. This proves storage, normal hit status, joint
  anchoring, and half-size conversion, but not hurtbox intersection, victim
  records, hitlag, damage, or full `gmcollision.c`.

## 2026-06-28 - Added Bounded Mario/Fox FTParts Damage-Coll Transform Shell

- Added a narrow project-owned `FTParts` compatibility shell and attached one
  root part to each initialized Mario/Fox DObj through `DObj.user_data.p`,
  matching the BattleShip `ftGetParts` access pattern without importing broad
  fighter display or transform runtime.
- Synced the selected root DObj translate/scale into bounded matrix,
  inverse-matrix, and scale fields so the existing damage-coll shell has the
  next prerequisite for source-order rectangle collision work.
- Updated the direct and menu-chain Mario/Fox init verifiers to require
  `parts=0x3/0x3/0x3`, proving FTParts attachment, matrix/world-position
  consistency, and scale availability for the selected shell.
- Deferred: real per-fighter `attr->damage_coll_descs[]`, full joint
  transforms, rectangle intersection, victim records, hitlag, damage, and
  broad `gmcollision.c` import.

## 2026-06-28 - Switched Selected Mario/Fox Hurtbox To Original Descriptor Data

- Replaced the placeholder root hurtbox seed with the same source-order shape
  used by BattleShip `ftmanager.c`: read `attr->damage_coll_descs[0]`, attach
  `fp->joints[joint_id]`, copy placement/grabbable/offset/size, then halve
  size.
- Exposed the local `FTAttributes.damage_coll_descs[11]` field at the original
  struct offset instead of treating that range as filler.
- Updated the direct and menu-chain Mario/Fox init verifiers to require the
  selected real descriptor identities: Mario joint `6` / half-size
  `51.5/56.0/47.5`, Fox joint `5` / half-size `51.0/26.0/22.5`, while keeping
  the proof bounded to one slot per fighter.
- Deferred: remaining damage-coll descriptor slots, full joint transforms,
  rectangle intersection, victim records, hitlag, damage, and broad
  `gmcollision.c` import.

## 2026-06-28 - Added Selected Attack/Hurtbox Rectangle Probe

- Added a bounded local copy of BattleShip's `gmCollisionTestRectangle` math
  for the selected Dash-Run Fox Jab2 hitbox proof only.
- Extended `DASH_RUN_ATTACK_EVENT_POS` from mask `0x3ff` to `0x7ff`: after
  `New -> Transfer -> Interpolate` and the broad-phase fighter-range predicate,
  the selected hitbox now runs one original-compatible rectangle probe against
  Mario's descriptor-backed `FTDamageColl[0]` hurtbox.
- This does not enable continuous live hitboxes, victim records, hitlag,
  damage, or broad `gmcollision.c`; the next attack-contact step is a bounded
  selected `gmCollisionCheckFighterAttackDamageCollide`-style interaction
  decision.

## 2026-06-28 - Added Selected Attack/Damage Collision Decision Probe

- Added a bounded local `gmCollisionCheckFighterAttackDamageCollide`-style
  helper for the selected Dash-Run Fox Jab2 hitbox proof only, reusing the
  already seeded Mario descriptor-backed `FTDamageColl[0]` and `FTParts`
  transform shell.
- Extended `DASH_RUN_ATTACK_EVENT_POS` from mask `0x7ff` to `0xfff`: after
  `New -> Transfer -> Interpolate`, the broad-phase fighter-range predicate,
  and the selected rectangle probe, the proof now records one selected
  attack/damage collision decision.
- This still does not enable continuous live hitboxes, victim records, rehit
  timers, hitlag, damage, or broad `gmcollision.c`; the next attack-contact
  step is bounded victim attack-record insertion or equivalent interaction
  bookkeeping.

## 2026-06-28 - Added Bounded Selected Attack Damage-Record Insertion

- Added a bounded source-shaped damage-record helper for the selected Dash-Run
  Fox Jab2 -> Mario hurtbox proof. It mirrors the original
  `ftMainSetHitInteractStats` record behavior for the damage case: find the
  matching active attack group, update an existing victim slot if present,
  otherwise use the first empty slot and fall back to slot `0`.
- Extended `DASH_RUN_ATTACK_EVENT_POS` from mask `0xfff` to `0x1fff`: after
  `New -> Transfer -> Interpolate`, broad-phase range, selected rectangle, and
  selected attack/damage collision decision, the proof now records Mario as a
  hurt interaction in Fox Jab2's attack-record array.
- This still does not enable continuous live hitboxes, rehit timers, hitlag,
  full damage, or broad `ftmain.c`/`gmcollision.c`; the next attack-contact
  step is a bounded post-record hit interaction/status bookkeeping slice.

## 2026-06-28 - Added Bounded Selected Attack Damage Hit-Log Bookkeeping

- Added a narrow `FTHitLog` compatibility type and a bounded normal-hit slice
  for the selected Dash-Run Fox Jab2 -> Mario hurtbox proof.
- Extended `DASH_RUN_ATTACK_EVENT_POS` from mask `0x1fff` to `0x3fff`: after
  selected record insertion, the proof now mirrors the front half of
  `ftMainUpdateDamageStatFighter` enough to record captured damage, attacker
  `attack_damage`, victim `damage_queue` / `damage_lag`, the first fighter
  hit-log entry, and the player-stat/stale-queue compatibility seams.
- This still does not enable continuous live hitboxes, rehit timers, real
  stale queue behavior, hitlag, damage status/runtime, effects, SFX, or broad
  `ftmain.c` / `gmcollision.c`. The next attack-contact step should remain a
  bounded damage-status/hitlag or impact-position/SFX/effect seam.

## 2026-06-28 - Added Bounded Selected Attack Hit-SFX Seam

- Extended `DASH_RUN_ATTACK_EVENT_POS` from mask `0x3fff` to `0x7fff`: after
  the selected Fox Jab2 -> Mario damage-record and hit-log bookkeeping proof,
  the local slice now selects the original `dFTMainHitCollisionFGMs` table entry
  from the decoded hitbox `fgm_kind` / `fgm_level` and reaches the existing FGM
  audio stub seam.
- Kept this deliberately narrow: it does not import `lbcommon.c`, does not add
  positional audio balance, and does not claim a real DS audio backend.
- Deferred: continuous live hitboxes, rehit timers, real stale queue behavior,
  hitlag, damage status/runtime, effects, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-28 - Added Bounded Selected Fighter-Hitlog Stats Handoff

- Extended `DASH_RUN_ATTACK_EVENT_POS` from mask `0x7fff` to `0xffff`: after
  the selected Fox Jab2 -> Mario contact proof builds the first `FTHitLog`, a
  bounded source-shaped `ftMainProcessHitCollisionStatsMain` fighter-hitlog
  slice now computes knockback through the existing `ftParamGetCommonKnockback`
  seam and fills victim damage angle/LR/index/player metadata.
- Added the missing local `FTStruct` damage metadata fields plus narrow
  original-compatible hit-element and damage-kind enum values needed by that
  source path.
- Still deferred: hitlag scheduling, `ftMainProcParams`,
  `ftCommonDamageGotoDamageStatus`, damage effects, and full damage runtime.

## 2026-06-28 - Added Bounded Selected ftMainProcParams Damage/Hitlag Handoff

- Extended `DASH_RUN_ATTACK_EVENT_POS` from mask `0xffff` to `0x1ffff`: after
  the selected Fox Jab2 -> Mario hit-log stats handoff, the bounded slice now
  runs source-shaped victim-side `ftMainProcParams` damage scheduling for the
  queued Mario damage.
- Added narrow local copies of the original `ftParamGetHitLag` and
  `ftParamSetDamageShuffle` behavior, plus a bounded
  `ftCommonDamageGotoDamageStatus` compatibility handoff that reaches the
  existing damage-var initialization without promoting the full damage runtime.
- Added the `DASH_RUN_PROCPARAMS` verifier marker. It records mask low bits
  `0x7f`, damage before/after, queued damage, queued hitlag damage, computed
  hitlag, knockback pause, and status before/after. The direct/menu-chain
  dash-run verifiers and the current boundary aggregate require the queued
  damage to update Mario's percent, positive hitlag, `is_knockback_paused`,
  unchanged status, and transient damage-field clearing.
- Still deferred: full `ftMainProcParams`, original damage status selection,
  attacker-side `attack_damage`, shield/reflect/absorb branches, real
  `proc_lagstart`, effects, full damage runtime, and broad `ftmain.c` /
  `gmcollision.c`.

## 2026-06-29 - Added Selected ftMainProcParams Lag-Start Tail

- Added the missing project-owned `FTStruct.proc_lagstart` callback slot,
  matching the original BattleShip callback surface needed by
  `ftMainProcParams`.
- Extended the existing `DASH_RUN_PROCPARAMS` verifier contract from mask low
  bits `0x7f` to `0xff`: after selected hitlag/pause setup, the proof now
  calls a proof-owned `proc_lagstart` callback at the original tail position
  before clearing transient damage fields.
- Kept this narrow. It does not promote full `ftMainProcParams`, original
  damage status selection, attacker-side `attack_damage`, shield/reflect/absorb
  branches, effects, full damage runtime, or broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Selected ftMainProcParams Attack-Damage Branch

- Extended the selected Fox Jab2 -> Mario contact proof to run Fox's
  attacker-side `attack_damage` branch in the source order used by
  `ftMainProcParams`: call `proc_hit`, compute attacker hitlag from
  `attack_damage`, run the rumble seam, clear input taps/releases, and clear
  the transient attacker `attack_damage` field.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0xff` to `0x1ff` in the
  direct, menu-chain, and current aggregate verifiers.
- Still deferred: bounded original damage-status selection from
  `ftcommondamage.c`, shield/reflect/absorb `ftMainProcParams` branches, full
  hitbox runtime, full damage runtime, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Selected ftMainProcParams Attack-Shield-Push Branch

- Added the missing project-owned `FTStruct.proc_shield` callback slot and
  clear it in the common status-reset seam.
- Extended the selected Fox Jab2 contact proof with a bounded source-shaped
  attacker `attack_shield_push` branch: call `proc_shield`, compute attacker
  hitlag from `attack_shield_push`, clear input taps/releases, and clear the
  transient `attack_shield_push` field.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x1ff` to `0x3ff` in the
  direct, menu-chain, and current aggregate verifiers.
- Still deferred: bounded original damage-status selection from
  `ftcommondamage.c`, victim `shield_damage` / GuardSetOff through
  `ftMainProcParams`, shield break, reflect/absorb branches, full hitbox
  runtime, full damage runtime, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Selected ftMainProcParams Shield-Damage GuardSetOff Branch

- Added the missing project-owned `FTStruct.shield_damage_total` field needed
  by the source-shaped shield-damage branch.
- Extended the selected Fox Jab2 contact proof with Mario's victim-side
  `shield_damage` branch from original `ftMainProcParams`: call imported
  original `ftCommonGuardSetOffSetStatus` through the bounded
  `ndsBaseFTCommonGuardSetOffSetStatus` path, prove GuardSetOff/GuardOn
  status state, then run the shared hitlag/input-clear/`proc_lagstart`/
  transient-field-clear tail.
- The proof records deterministic GuardSetOff frames `20200` milli-units and
  ground velocity `-40400`, while restoring the existing GuardSetOff
  diagnostics so the older exact `DASH_RUN_GUARD_SETOFF=4/4` assertion stays
  stable.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x3ff` to `0x7ff` in the
  direct, menu-chain, and current aggregate verifiers.
- Still deferred: bounded original damage-status selection from
  `ftcommondamage.c`, shield break, reflect/absorb branches, full hitbox
  runtime, full damage runtime, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Selected ftMainProcParams Shield-Break Branch

- Added the narrow ShieldBreakFly constants and FGM/colanim IDs needed by the
  current bounded branch proof.
- Promoted the project-owned `ftCommonShieldBreakFlyCommonSetStatus` seam from
  no-op to a bounded original-compatible handoff: set air state, call the
  ShieldBreakFly `ftMainSetStatus` path, run the animation-event seam, touch
  the fighter colanim seam, and record the ShieldBreak FGM through the existing
  audio stub.
- Added a narrow early ShieldBreakFly case to the project-owned `ftMainSetStatus`
  seam so the proof does not trip unrelated non-Wait status-denial diagnostics.
- Extended the selected Fox Jab2 -> Mario `ftMainProcParams` proof with the
  victim shield-break path: deplete shield health, reset it to `30`, enter
  ShieldBreakFly status/motion `158/138`, run the shared hitlag/input-clear/
  `proc_lagstart` tail, and clear `shield_damage` / `shield_damage_total`.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x7ff` to `0xfff` in the
  direct and menu-chain Dash-Run verifiers.
- Still deferred: exact imported `ftcommonshieldbreakfly.c` physics/effects,
  bounded original damage-status selection from `ftcommondamage.c`, reflect/
  absorb branches, full hitbox runtime, full damage runtime, and broad
  `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Selected ftMainProcParams Reflect/Absorb Branches

- Added the narrow `FTSpecialColl` compatibility shell and BatHit FGM constant
  needed by the selected branch proof.
- Extended the selected Fox Jab2 -> Mario `ftMainProcParams` proof with the
  remaining bounded special-collision branches: reflect-damage break into
  ShieldBreakFly, Fox reflector hit, Ness reflector sound, and Ness absorb.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0xfff` to `0xffff` in the
  direct, menu-chain, and current aggregate verifiers.
- Still deferred: bounded original damage-status selection from
  `ftcommondamage.c`, exact imported Fox/Ness special runtimes/effects, full
  hitbox runtime, full damage runtime, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Selected ftcommondamage Status Selector Proof

- Added BattleShip-compatible common damage status aliases `37..56` and the
  `ftParamGetHitStun` prototype to the project-owned fighter header.
- Added a narrow Dash-Run side probe that mirrors `ftcommondamage.c` damage
  level thresholds and ground/air/electric status-table selection for the
  selected Fox Jab2 -> Mario contact path.
- Kept the current damage-status runtime parked: the probe requires the
  existing stubbed `ftCommonDamageGotoDamageStatus` path to leave `status_id`
  unchanged, then records `DASH_RUN_DAMAGE_STATUS`.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0xffff` to `0x1ffff` in
  the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3` and
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`.
- Still deferred: bounded real damage-status setup/tick from the selected
  status path, exact damage effects/color/screen-flash behavior, full hitbox
  runtime, full damage runtime, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded ftcommondamage Status Setup Tick Proof

- Added the missing original-compatible common damage motion aliases `31..49`
  so selected damage statuses can record a real status/motion pairing.
- Extended the project-owned `ftCommonDamageInitDamageVars` seam with bounded
  `ftcommondamage.c`-compatible damage-level/status selection, hitstun storage,
  public knockback, damage velocity, and cliff-hold wait handling. The actual
  status install remains behind a Dash-Run-only proof gate.
- Added a guarded `ftMainSetStatus` damage-status path that accepts only the
  selected bounded proof, installs original-named damage update/interrupt/
  physics/map callbacks, and leaves broad damage runtime deferred.
- Added `DASH_RUN_DAMAGE_SETUP`, proving the selected Fox Jab2 -> Mario damage
  path can run `ftCommonDamageGotoDamageStatus` -> `ftMainSetStatus`, install
  a BattleShip-compatible damage status/motion, tick the damage update callback
  once to decrement hitstun, then restore the preview fighter state.
- Tightened the direct/menu-chain Dash-Run `DASH_RUN_PROCPARAMS` verifier
  contract from mask low bits `0x1ffff` to `0x3ffff`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Live-Hit Damage-Resist Breakthrough Proof

- Extended the existing direct/menu-chain live-hit damage lifecycle boundary
  with a bounded source-shaped `ftMainCheckGetUpdateDamage` breakthrough
  branch proof.
- The probe keeps the selected Fox Jab2 hitbox path, seeds Mario damage resist
  below the selected damage, calls the existing source-shaped helper, and
  proves the original branch clears the resist flag, leaves a negative resist
  remainder, and queues matching leftover damage/lag before restoring both
  fighter structs.
- Tightened the current `STAGE_MPLIVEHIT_DAMAGE` aggregate from mask low bits
  `0x3ffffff` to `0x7ffffff`; current summary adds
  `rbreak=0x7f/2->-2/q2`.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added DamageUpdateMain Sleep Dispatcher Proof

- Added `DASH_RUN_DAMAGE_SLEEP`, a bounded source-shaped
  `ftCommonDamageUpdateMain` no-grab/no-capture Sleep-element dispatcher proof
  under the inherited Dash-Run damage aggregate used by current modes
  `159/160`.
- The proof seeds no catch/capture/item links, Sleep element, zero knockback,
  and a cliff-catch wait setup, then routes through
  `ftCommonDamageGotoDamageStatus` into FuraSleep status/motion `165/145`,
  observes the FuraSleep color-animation seam, and restores preview state.
- Updated the current docs and verifier marker contract to record
  `sleep=0x7f`.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  `git diff --check`,
  and `git status --short decomp`.

## 2026-06-30 - Added Live-Hit Catch-Search Gate Proof

- Added a bounded source-shaped `ftMainSearchFighterCatch` pre-stat gate proof
  to current modes `159/160`.
- The proof resets search target/distance, proves normal target and ground-air
  gates, exercises the hurt-record skip and default-record pass, skips
  status-disabled and non-grabbable damage-coll slots, collides with selected
  Mario damage-coll slot `3`, assigns the closest target/distance, and restores
  fighter/root/detect state.
- Tightened `STAGE_MPLIVEHIT_DAMAGE` from mask low bits `0x3fffffff` to
  `0x7fffffff` and added verifier marker `catchSearch=0x3ff/s3`.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Source-Order Hurtbox Damage Consumption Proof

- Added bounded slot-1 hurtbox damage consumption to the existing
  direct/menu-chain live-hit damage lifecycle modes `159/160`.
- The proof builds on the source-order Mario hurtbox scan that skips slot `0`,
  hits slot `1`, and observes the slot-`10` `None` sentinel. It then feeds the
  selected slot into bounded damage record, hitlog, fighter-hit stats,
  percent-damage, and hitlag scheduling helpers before restoring both fighter
  structs.
- Tightened the live-hit proof mask from `0x1fffff` to `0x3fffff` and added
  the verifier marker `STAGE_MPLIVEHIT_HURTBOX_DAMAGE`, summarized as
  `hbdmg=0->4/6`.
- Still deferred: broad `gmcollision.c`, full `ftmain.c`, continuous
  multi-hitbox runtime, natural continuous multi-slot victim/damage runtime,
  arbitrary damage-state duration, real stale queues, effects, items/weapons,
  HUD, audio, and unbounded gameplay scheduling.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-06-30 - Added Live-Hit Hurtbox Damage-Detect-Off Skip Proof

- Extended the existing `STAGE_MPLIVEHIT_HURTBOX` proof in modes `159/160`
  from mask low bits `0x1fff` to `0xffff`.
- The new bits prove the source-order `ftmain.c` per-attack hurtbox skip when
  `gFTMainIsDamageDetect[attack_id] == FALSE`, then restore the detect flag
  before the positive slot-0 intangible skip -> slot-1 hit -> slot-10 `None`
  sentinel path.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -Build -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit Hurtbox Global Hitstatus Gate Proof

- Extended the existing `STAGE_MPLIVEHIT_HURTBOX` proof in modes `159/160`
  from mask low bits `0xff` to `0x1fff`.
- The new bits prove the source-order `ftmain.c` global hurtbox gate: all of
  `special_hitstatus`, `star_hitstatus`, and `hitstatus` must be non-intangible
  before the per-slot damage-coll loop can run. The proof also checks each
  intangible skip case, restores the target status fields, then runs the
  existing slot-0 intangible skip -> slot-1 hit -> slot-10 `None` sentinel
  path.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -Build -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Tightened Live-Hit Damage-Detect-Off Shield Skip Proof

- Extended the existing `STAGE_MPLIVEHIT_SHIELD_CONTACT` proof in modes
  `159/160` from mask low bits `0xfff` to `0x7fff`.
- The new bits prove the sibling source-order shield branch where
  `is_shield` is still true but `gFTMainIsDamageDetect[attack_id]` is false,
  so shield contact is skipped without adding a collision/hit count, then the
  proof restores the damage-detect gate before the positive shield-stat path.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -Build -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Tightened Live-Hit Shield-Off Skip Proof

- Extended the existing `STAGE_MPLIVEHIT_SHIELD_CONTACT` proof in modes
  `159/160` from mask low bits `0xff` to `0xfff`.
- The new high bits prove the adjacent source-order `is_shield == false`
  branch skips shield contact while leaving `gFTMainIsDamageDetect[attack_id]`
  available for the later hurtbox path, then restores the selected shield
  state before the positive shield-stat proof continues.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -Build -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added Selected Live-Hit Hit-Interact Refresh Proof

- Strengthened live-hit modes `159/160` without adding another harness pair.
- Added a small project-owned `ftMainSetHitInteractStats` compatibility seam
  matching the original fighter damage/shield/attack record update shape.
- The selected Fox Jab2 Attack12 proof now validates damage, shield, and
  attack-group updates on the selected attack record, seeds a rehit timer,
  calls `ftParamRefreshAttackCollID`, and proves the selected attack record is
  cleared back to the original-compatible empty state.
- Updated the live-hit proof mask from `0xffff` to `0x1ffff` and added
  `STAGE_MPLIVEHIT_REHIT`, summarized as `rehit=5->0 clear=1`.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-29 - Added Live Selected-Hitbox Damage Lifecycle Proof

- Added direct/menu-chain harness modes `159/160`:
  `battle_mariofox_stage_mplivehit_damage_loop` and
  `menu_chain_mariofox_stage_mplivehit_damage_loop`.
- The new bounded proof inherits the current Pupupu live battle roots,
  source-order MP/cliff/passive/wall/catch/throw/platform/Inishie coverage,
  Dash-Run attack-event damage setup, and the selected hit-to-damage recovery
  aggregate, then proves a selected Fox Jab2 Attack12 live-hit lifecycle.
- The proof records original event-backed hitbox metadata, attack-state
  `Off -> New -> Transfer -> Interpolate`, selected range/rectangle/contact,
  damage-record insertion, immediate repeat-hit rejection, damage scheduling,
  hitlag, and the existing damage-recover consumption path before restoring
  bounded preview state.
- Added `STAGE_MPLIVEHIT_*` diagnostics, direct/menu verifier wrappers,
  registry/profile entries, and doc updates. Full continuous multi-hitbox
  runtime, broad `gmcollision.c`, complete `ftmain.c`, real rehit timers,
  specials/items/HUD/audio, and unbounded gameplay scheduling remain deferred.
- Verified:
  `make clean; make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `check-harness-registry.ps1`.

## 2026-06-29 - Restored Original DamageInit Velocity Routing

- Replaced the placeholder `knockback * 0.5/0.25` velocity math in the
  project-owned `ftCommonDamageInitDamageVars` seam with BattleShip's
  angle-derived `cos/sin` knockback vector routing.
- Restored the deterministic source-order ground damage routing: floor-angle
  comparison, ground-to-air conversion, high-damage floor-bounce
  ImpactWave/QuakeMag0 effect seams, FlyTop selection, and final status
  replacement/electric wrapping.
- Kept random FlyRoll, Kirby copy-loss, and damage voice SFX deferred with
  their owning systems instead of adding standalone stubs.
- Adjusted the existing dust proof to accept the original high-speed default
  reset interval of `1` after the dust effect spawns.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`.

## 2026-06-29 - Added Damage Interrupt Hammer Branch Proof

- Updated the bounded `ftCommonDamageCommonProcInterrupt` seam to preserve the
  BattleShip branch shape for zero-hitstun damage recovery.
- Added a narrow proof hook for hammer-held ground and air branches:
  grounded fighters route to `ftHammerProcInterrupt`, airborne fighters route
  to `ftCommonHammerFallProcInterrupt`, and normal non-proof behavior still
  reports no held hammer.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x7fffffff` to
  `0xffffffff` in the direct/menu-chain Dash-Run verifiers.
- Still deferred: source-shaped damage physics/map/interrupt continuation,
  hitstun-expiry Wait/Fall handoff, exact damage effects/color/screen-flash
  behavior, full hitbox runtime, full damage runtime, and broad `ftmain.c` /
  `gmcollision.c`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Restored Damage Air Dust Update Branch

- Replaced the project-owned `ftCommonDamageUpdateDustEffect` decrement-only
  stub with the original-shaped branch: when the damage dust interval reaches
  zero, it calls `ftParamMakeEffect(... nEFKindDustExpandLarge ...)` and resets
  the interval through the existing damage interval helper.
- Added the original-compatible `nEFKindDustExpandLarge = 17` effect ID to the
  narrow local effect enum.
- Strengthened the existing `DASH_RUN_DAMAGE_SETUP` `DUST` bit so it now
  proves the bounded DamageAir update dust-effect spawn and interval reset,
  not only initial interval setup.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`.

## 2026-06-29 - Added Bounded ftcommondamage Physics Callback Proof

- Added a narrow project-owned `ftPhysicsApplyAirVelFriction` compatibility
  helper and routed the selected installed damage status through the
  original-shaped `ftCommonDamageCommonProcPhysics` branch.
- Extended `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x7f` to `0xff` and
  added post-physics velocity diagnostics. The verifier now checks the
  grounded or airborne source velocity according to the installed damage
  ground/air state.
- Kept the proof bounded: it ticks one installed damage update callback and
  one installed damage physics callback, then restores the preview fighter
  state before the wider Dash-Run/current boundary continues.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Still deferred: damage map/interrupt continuation, hitstun-expiry Wait/Fall
  handoff, exact fast-fall damage branch, exact damage effects/color/
  screen-flash behavior, full hitbox runtime, full damage runtime, and broad
  `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded ftcommondamage Interrupt Handoff Proof

- Extended the selected Dash-Run damage-status side probe through one
  installed airborne damage interrupt tick after seeding hitstun to zero.
- Counted the original-shaped `ftCommonDamageAirCommonProcInterrupt` handoff
  into the existing `ftCommonDamageFallProcInterrupt` seam without starting
  the broader DamageFall interrupt runtime.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0xff` to `0x1ff` in
  the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`.
- Still deferred: source-shaped damage map continuation, hitstun-expiry status
  handoff, exact fast-fall damage branch, exact damage effects/color/
  screen-flash behavior, full hitbox runtime, full damage runtime, and broad
  `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded ftcommondamage Expiry Status Handoff Proof

- Extended the selected Dash-Run damage-status side probe through the airborne
  `ftCommonDamageAirCommonProcUpdate` expiry branch by seeding hitstun to one
  and animation frame to zero.
- Added a proof-only `ftCommonDamageFallSetStatusFromDamage` gate that sets up
  DamageFall status/motion/callback slots, then restores the preview fighter
  state.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x1ff` to `0x3ff` in
  the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`.
- Still deferred: source-shaped DamageFall map continuation, continuous
  DamageFall runtime, exact fast-fall damage branch, exact damage effects/
  color/screen-flash behavior, full hitbox runtime, full damage runtime, and
  broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded ftcommondamage DamageFall Map No-Collision Proof

- Extended the selected Dash-Run damage-status side probe through one installed
  DamageFall map callback tick after the hitstun-expiry handoff.
- Gated `ftCommonDamageFallProcMap` to call the imported original base callback
  and made `mpCommonCheckFighterCliff` return `FALSE` only for this proof,
  proving the safe no-collision branch without enabling full DamageFall map
  collision branches.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x3ff` to `0x7ff` in
  the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`, and
  `check-gbi-decode-fixtures.ps1`.
- Still deferred: positive DamageFall map collision branches
  (cliff/passive/downbounce), continuous DamageFall runtime, exact fast-fall
  damage branch, exact damage effects/color/screen-flash behavior, full hitbox
  runtime, full damage runtime, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded ftcommondamage DamageFall Floor-Branch Proof

- Extended the selected Dash-Run damage-status side probe through a second
  installed DamageFall map callback tick after the safe no-collision tick.
- Reused the existing Dash-Run DamageFall map gate to make
  `mpCommonCheckFighterCliff` return a floor collision for that second tick,
  proving original `ftCommonDamageFallProcMap` reaches the passive checks and
  DownBounce seam in source order.
- Kept this proof narrow: the Dash-Run side probe counts the DownBounce seam
  but does not promote full DownBounce status/runtime here. The existing
  CliffWait damage proof still covers the fuller DownBounce status path.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x7ff` to `0xfff` in
  the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Still deferred: positive cliff-collision map branch, continuous DamageFall
  runtime, exact fast-fall damage branch, exact damage effects/color/
  screen-flash behavior, full hitbox runtime, full damage runtime, and broad
  `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded ftcommondamage DamageFall Cliff-Branch Proof

- Extended the selected Dash-Run damage-status side probe through a third
  installed DamageFall map callback tick after the floor-branch tick.
- Macro-renamed only `ftCommonCliffCatchSetStatus` in the
  `ftcommoncliffcatchwait.c` import to a base symbol and added a project-owned
  wrapper. Existing full CliffCatch proofs still call the imported base body;
  the Dash-Run damage side probe counts the branch without promoting full
  CliffCatch status/runtime here.
- Reused the existing Dash-Run DamageFall map gate to make
  `mpCommonCheckFighterCliff` return `MAP_FLAG_RCLIFF` for that third tick,
  proving original `ftCommonDamageFallProcMap` reaches the CliffCatch seam in
  source order.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0xfff` to `0x1fff` in
  the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Still deferred: continuous DamageFall runtime, exact fast-fall damage branch,
  exact damage effects/color/screen-flash behavior, full hitbox runtime, full
  damage runtime, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded ftcommondamage DamageFall Physics Tick Proof

- Extended the selected Dash-Run damage-status side probe through one installed
  DamageFall `ftPhysicsApplyAirVelDriftFastFall` callback tick after the
  hitstun-expiry DamageFall status handoff and before the guarded DamageFall
  map-branch ticks.
- Counted the installed callback and required the selected fighter to stay in
  DamageFall/Air while Y velocity moves downward, proving the imported
  DamageFall physics slot is wired without starting continuous DamageFall
  runtime.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x1fff` to `0x3fff`
  in the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Still deferred: continuous DamageFall runtime, exact fast-fall branch,
  exact damage effects/color/screen-flash behavior, full hitbox runtime, full
  damage runtime, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded DamageFall Fast-Fall Physics Branch Proof

- Corrected the local physics compatibility shim so
  `ftPhysicsApplyAirVelDriftFastFall` follows BattleShip's original
  `is_fastfall ? ftPhysicsApplyFastFall : ftPhysicsApplyGravityDefault`
  branch instead of always applying normal gravity.
- Added the minimal `ftPhysicsApplyFastFall` helper and original-shaped
  `ftPhysicsCheckSetFastFall` condition. The color-animation hook remains a
  no-op compatibility seam until real colanim runtime is imported.
- Extended the selected Dash-Run DamageFall side probe through one fast-fall
  input/tap branch and required `vel_air.y == -attr->tvel_fast`.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x3fff` to `0x7fff`
  in the direct, menu-chain, and current aggregate verifiers.
- Still deferred: continuous DamageFall runtime, real fast-fall color-animation
  runtime, exact damage effects/color/screen-flash behavior, full hitbox
  runtime, full damage runtime, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded DamageAir Map Continuation Proof

- Replaced the local `ftCommonDamageAirCommonProcMap` no-op with a guarded
  BattleShip-shaped map branch for the existing Dash-Run damage-status side
  probe.
- Added the minimal guarded `mpCommonCheckFighterDamageCollision` contract:
  clear previous/current damage collision masks, publish a selected floor hit,
  and let the original-shaped DamageAir map condition reach the passive checks
  and DownBounce seam.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x7fff` to `0xffff`
  in the direct, menu-chain, and current aggregate verifiers.
- Still deferred: continuous DamageAir/DamageFall map runtime, real wall-damage
  selection from arbitrary collision frames, full hitbox runtime, full damage
  runtime, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded DamageFlyRoll Physics Tail Proof

- Restored the missing BattleShip tail in the local
  `ftCommonDamageCommonProcPhysics` shim: DamageFlyRoll now updates the model
  pitch, and throw-owned low-velocity damage clears attack collisions.
- Added the minimal local `ftCommonDamageFlyRollUpdateModelPitch` helper using
  the same source formula and existing DObj/parts-transform seam.
- Isolated the proof so normal damage-physics diagnostics do not accidentally
  run the throw-owned attack-collision clear tail, and check DamageFlyRoll
  pitch against the post-friction velocities used by the original call order.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0xffff` to `0x1ffff`
  in the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  and `clean-generated.ps1 -DryRun`.
- Still deferred: continuous damage lifecycle runtime, real full
  `ftcommondamage.c` import, full hitbox runtime, full damage runtime, and
  broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded Knockback-Over Invincibility Branch Proof

- Added local BattleShip-shaped `ftCommonDamageCheckSetInvincible`,
  `ftCommonDamageSetStatus`, and `ftParamSetTimedHitStatusInvincible`
  compatibility helpers for the existing bounded damage-status proof path.
- Extended `DASH_RUN_DAMAGE_SETUP` with a restored side proof of the
  `is_knockback_over` branch: hitlag-ready damage status setup clears the flag
  and sets timed invincibility through `special_hitstatus`.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x1ffff` to `0x3ffff`
  in the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`.
- Still deferred: continuous damage lifecycle runtime, `proc_passive` /
  `proc_lagupdate` fields and real scheduling, real full
  `ftcommondamage.c` import, full hitbox runtime, full damage runtime, and
  broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded Damage Hitlag Smash DI Proof

- Added BattleShip-compatible `FTCOMMON_DAMAGE_SMASH_DI_*` constants and a
  local `ftCommonDamageCommonProcLagUpdate` helper.
- Extended `DASH_RUN_DAMAGE_SETUP` with a bounded hitlag-only Smash DI side
  proof: live stick input moves the root DObj by the original range multiplier
  and consumes tap-stick buffers without decrementing hitlag.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x3ffff` to `0x7ffff`
  in the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  and `clean-generated.ps1 -DryRun`.
- Still deferred: continuous hitlag lifecycle scheduling, real
  `proc_lagupdate` / `proc_lagend` wiring through imported `ftmain.c`, real
  full `ftcommondamage.c` import, full hitbox runtime, full damage runtime,
  and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded Damage Hitlag Lifecycle Proof

- Added the missing local `FTStruct` `proc_lagupdate` and `proc_lagend`
  callback slots used by original BattleShip fighter hitlag scheduling.
- Installed `ftCommonDamageCommonProcLagUpdate` from the bounded damage setup
  path and added a tiny source-shaped lifecycle slice for the selected proof:
  hitlag decrements, knockback stays paused until the terminal tick, lag-end
  runs, and animation events resume after hitlag reaches zero.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x7ffff` to
  `0xfffff` in the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`.
- Note: the dash-run direct/menu verifier pair must run serially with the
  current melonDS/GDB wrappers; a parallel attempt corrupted GDB marker reads.
- Still deferred: continuous hitlag scheduling through the real `ftmain.c`
  task process, real `proc_lagupdate` / `proc_lagend` ownership outside this
  bounded proof, real full `ftcommondamage.c` import, full hitbox runtime, full
  damage runtime, and broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Added Bounded Damage Setup Tail Proof

- Restored the next bounded `ftCommonDamageInitDamageVars` setup-tail slice for
  the selected Dash-Run damage proof without importing full `ftmain.c`,
  `gmcollision.c`, or continuous damage runtime.
- Added proof-local compatibility for public knockback, damage color animation
  selection, screen flash, rumble, dust interval, player-tag wait, and attacker
  hit count/knockback. The color-animation stubs only report success while the
  guarded Dash-Run damage setup probe is active.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0xfffff` to
  `0x7ffffff` in the direct, menu-chain, and current aggregate verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`.
- Current boundary note: `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 5`
  still fails at the pre-existing `STAGE_MPWALLCOPY_FLOOR` aggregate
  result/safe sentinel while the new `DASH_RUN_DAMAGE_SETUP=0x7ffffff` marker
  is present. Treat that as a separate wall-copy/finalizer verifier issue, not
  a damage-tail proof failure.
- Still deferred: `proc_passive` ownership in local `FTStruct`, continuous
  damage lifecycle scheduling, real full `ftcommondamage.c` import, full
  hitbox runtime, full damage runtime, item/catch/capture damage branches, and
  broad `ftmain.c` / `gmcollision.c`.

## 2026-06-29 - Restored Passive Recover After Damage Setup Tail

- Fixed the current Passive recover boundary regression exposed after the
  bounded damage setup-tail proof restored the source-compatible
  `ftCommonDamageInitDamageVars` rumble call.
- Updated the recover-loop throw-release proof to expect three
  source-compatible rumble requests: the damage-init rumble plus the two
  original thrown-release rumbles from `ftCommonThrownReleaseThrownUpdateStats`.
- Kept the existing direct/menu-chain Passive recover modes `155/156`; no new
  harness pair or gameplay subsystem was added.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 5`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 5`.
- The older `STAGE_MPWALLCOPY_FLOOR` failure was a downstream symptom of the
  throw-release marker not reaching the status/dead-result continuation; the
  same direct/menu-chain runs now pass through `mpWallCopy`, `mpCliffLive`,
  `throwReleaseStatus`, `throwProc`, and `throwDead`.

## 2026-06-29 - Restored Damage Setup proc_passive Ownership

- Added the BattleShip-compatible `FTStruct.proc_passive` callback slot to the
  local fighter shell and restored `ftCommonDamageInitDamageVars` ownership of
  that slot for the bounded damage setup path.
- Mirrored BattleShip's damage branch: electric damage statuses keep the
  original selected damage status in `status_vars.common.damage.status_id` and
  install `ftCommonDamageSetStatus` as `proc_passive`; other damage statuses
  install `ftCommonDamageCheckSetInvincible`.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x7ffffff` to
  `0xfffffff` in the direct, menu-chain, and current Passive recover aggregate
  verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 5`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 5`.
- Still deferred: continuous damage lifecycle scheduling through real
  `ftmain.c`, full `ftcommondamage.c` import, full hitbox runtime, full damage
  runtime, item/catch/capture damage branches, and broad `ftmain.c` /
  `gmcollision.c`.

## 2026-06-29 - Added Bounded Electric proc_passive Status Dispatch

- Added the next guarded `ftcommondamage.c` passive branch proof without adding
  a harness: the damage setup side probe now seeds an electric DamageE status,
  dispatches the installed `ftCommonDamageSetStatus` through the bounded
  `ftMainProcUpdateInterrupt` slice, and verifies it hands off to the stored
  original damage status with hitstun and invincibility set before restoring
  preview state.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x1fffffff` to
  `0x3fffffff` in the direct, menu-chain, and current Passive recover aggregate
  verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 5`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 5`.
- Still deferred: continuous damage lifecycle scheduling through real
  `ftmain.c`, full `ftcommondamage.c` import, full hitbox runtime, full damage
  runtime, item/catch/capture damage branches, and broad `ftmain.c` /
  `gmcollision.c`.

## 2026-06-29 - Added Bounded proc_passive Dispatch Slice

- Added a narrow `ftMainProcUpdateInterrupt`-shaped passive dispatch slice for
  the selected Dash-Run damage setup proof. It calls the installed
  `proc_passive` before update/interrupt callbacks when hitlag is clear,
  matching the source-order shape needed by this bounded proof without
  importing full `ftmain.c`.
- Proved the non-electric `ftCommonDamageCheckSetInvincible` branch by seeding
  knockback-over state, dispatching through the bounded passive slice, and
  verifying the branch clears the flag and installs invincibility before the
  preview fighter state is restored.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0xfffffff` to
  `0x1fffffff` in the direct, menu-chain, and current Passive recover aggregate
  verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 5`,
  and
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 5`.
- Still deferred: continuous damage lifecycle scheduling through real
  `ftmain.c`, full `ftcommondamage.c` import, full hitbox runtime, full damage
  runtime, item/catch/capture damage branches, and broad `ftmain.c` /
  `gmcollision.c`.

## 2026-06-29 - Added Bounded Sleep Damage FuraSleep Handoff

- Restored the original `ftCommonDamageGotoDamageStatus` sleep-element branch
  shape: sleep damage now calls a bounded project-owned
  `ftCommonFuraSleepSetStatus` instead of returning before status setup.
- Added original-compatible `nFTCommonMotionFuraFura/FuraSleep` and
  `nFTCommonStatusFuraFura/FuraSleep` IDs, plus a narrow FuraSleep
  `ftMainSetStatus` branch that installs FuraSleep status/motion and parks
  before the full breakout/capture/color-animation runtime.
- Tightened `DASH_RUN_DAMAGE_SETUP` from mask low bits `0x3fffffff` to
  `0x7fffffff` in the direct, menu-chain, and current platform-speed
  verifiers by proving a side probe through `ftCommonDamageGotoDamageStatus`
  sets `cliffcatch_wait` and reaches FuraSleep status/motion before restoring
  the preview fighter state.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1 -DelaySeconds 3`.
- Still deferred: full `ftcommonfurasleep.c`, breakout mash variables,
  capture-trapped helpers, fighter color-animation runtime, full damage
  lifecycle scheduling through real `ftmain.c`, and broad `gmcollision.c`.

## 2026-06-29 - Promoted Bounded Original FuraSleep Setup/Update Proof

- Imported BattleShip's original
  `decomp/src/ft/ftcommon/ftcommonfurasleep.c` through
  `src/import/battleship_ftcommon_furasleep.c`, with public symbols remapped
  behind the project-owned port seam.
- Added the narrow original-compatible FuraSleep constants, color-animation ID,
  and `FTStruct` breakout fields needed by that import.
- Replaced the project-owned FuraSleep status placeholder with a bounded call
  to imported `ndsBaseFTCommonFuraSleepSetStatus`, and installed imported
  `ndsBaseFTCommonFuraSleepProcUpdate` in the local `ftMainSetStatus`
  FuraSleep branch.
- Added source-order `ftCommonCaptureTrappedInitBreakoutVars` and
  `ftCommonCaptureTrappedUpdateBreakoutVars` compatibility helpers. These keep
  button/stick mash countdown behavior local without importing the broad
  capture runtime.
- Strengthened the existing `DASH_RUN_DAMAGE_SETUP=0x7fffffff` sleep bit
  without changing the mask: the side proof now requires FuraSleep
  status/motion, `cliffcatch_wait`, original breakout timer initialization,
  one imported FuraSleep update tick, forced imported FuraSleep -> Wait handoff,
  and the fighter color-animation seam before restoring the preview state.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1 -DelaySeconds 3`,
  and
  `verify-menu-chain-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1 -DelaySeconds 3`.
- Still deferred: continuous FuraSleep lifecycle scheduling through real
  `ftmain.c`, real fighter color-animation runtime, full capture/grab runtime,
  full damage lifecycle scheduling, and broad `gmcollision.c`.

## 2026-06-29 - Proved FuraSleep Mash Breakout Branch

- Aligned the current-truth `STATUS` / `HANDOFF` headers back to registry
  modes `155/156`; platform-speed modes `151/152` remain inherited regression
  coverage.
- Strengthened the existing dash-run damage setup side proof to run imported
  original `ftCommonFuraSleepProcUpdate` with A-tap plus stick mash input and
  require the source-shaped breakout timer delta.
- Still deferred: continuous multi-frame FuraSleep scheduling, real fighter
  color-animation runtime, full capture/grab runtime, full damage lifecycle
  scheduling, and broad `gmcollision.c`.

## 2026-06-29 - Added First DamageUpdateMain Catch Keep-Hold Proof

- Added the original-compatible
  `FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD` constant to the project fighter
  ABI shadow.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x3ffff` to `0x7ffff`
  in the direct/menu-chain Dash-Run verifiers.
- Added a bounded source-shaped `ftCommonDamageUpdateMain` first catch branch
  probe inside the existing procparams side proof. The probe seeds the selected
  fighter as catching the other fighter, proves the catch-resist and
  keep-hold predicates, copies `damage_lag` and `hitlag_mul` to the grabbed
  fighter, selects `nFTDamageKindColAnim`, records the fighter colanim seam,
  and restores both preview fighter structs before the wider proof continues.
- Updated diagnostic/current-truth docs to keep `ftCommonDamageUpdateMain`
  scoped: the first catch-resist/keep-hold branch is now proven, while the
  remaining catch/capture/item/release branches remain deferred.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit Attack-Event Hitbox-ID Mask Proof

- Added `gNdsFighterDashRunAttackEventAttackIDMask` to the existing bounded
  original attack-event decoder diagnostics.
- The current live-hit direct/menu-chain verifier now appends `hitboxIds=0x3`
  to the Dash-Run aggregate marker and asserts the selected original
  Mario/Fox script scan decoded at least attack-coll slots `0` and `1`.
- This is decoder coverage only. The selected live-hit contact/damage proof
  still intentionally stays on Fox Jab2 hitbox `1`; full continuous
  multi-hitbox collision runtime remains deferred.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`
  and
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  a forced normal rebuild plus `verify-runtime.ps1`,
  `verify-current.ps1 -DelaySeconds 3`,
  and `verify-regression.ps1 -DelaySeconds 3`.

## 2026-06-29 - Added DamageUpdateMain Heavy-Item Branch Proof

- Added bounded source-shaped `ftCommonDamageUpdateMain` DK-family heavy-item
  probes inside the existing Dash-Run procparams side proof.
- The catch-resist probe seeds a fake heavy item on Donkey, reaches the
  original-compatible damage colanim seam, and keeps the held item link.
- The drop probe seeds a fake heavy item on Giant Donkey, forces the
  non-resist branch through `ftSetupDropItem`, clears the item link, and
  reaches the bounded damage-status path before restoring preview state.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x3fffffff` to
  `0xffffffff` in the direct/menu-chain Dash-Run verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Live-Hit Attack Clash Rebound Proof

- Added a bounded source-shaped `ftMainUpdateAttackStatFighter` /
  `ftMainSetHitRebound` proof inside current modes `159/160`, without adding
  a new harness or importing broad `ftmain.c`.
- The probe seeds one active attack coll on each selected Mario/Fox fighter,
  follows the original two-branch attack-vs-attack order, proves both
  hit-interact attack records store the opposing group, clears attack/damage
  detect in the original ignore-flag order, applies the original US rebound
  formula to both fighters, records facing signs, counts two setoff-effect
  seams, and restores both fighter structs plus root X positions.
- Tightened `STAGE_MPLIVEHIT_DAMAGE` from mask low bits `0xfffffff` to
  `0x1fffffff` and added verifier marker `clash=0x3f/24/18`.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit Throw Attribution Proof

- Added original-compatible throw-owner fields to the project `FTStruct` shell
  and updated the `ftParamSetThrowParams` compatibility seam to copy
  `throw_fkind`, `throw_team`, `throw_player`, and `throw_player_num` from the
  owner fighter GObj.
- Added a bounded source-shaped `ftMainUpdateDamageStatFighter` attribution
  proof inside the current selected Fox Jab2 live-hit boundary. The probe
  seeds a throw owner, proves the direct attacker `1` is replaced by throw
  owner `0` for hitlog attribution, calls the player-stat/stale-queue seams,
  and restores both preview fighter structs.
- Widened the current live-hit aggregate mask from `0x7ffffff` to
  `0xfffffff` and added verifier marker `throwAttr=0x1f/1->0`.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  `git diff --check`,
  and `git status --short -- decomp`.

## 2026-06-30 - Added Live-Hit Damage-Resist False-Return Proof

- Updated the project-owned fighter shell with the original-compatible
  `damage_resist` and `is_damage_resist` fields needed by
  `ftMainCheckGetUpdateDamage`.
- Made the local selected-damage helper follow BattleShip's original
  damage-resist decrement / breakthrough / false-return logic.
- Added `STAGE_MPLIVEHIT_RESIST` to current modes `159/160`. It seeds damage
  resist above the selected Fox Jab2 damage, proves resist `7 -> 3`, skips
  damage queue/percent/hitlog, emits the bounded set-off effect and hit SFX
  seams, and restores both fighter structs.
- Tightened the current live-hit aggregate proof mask from `0x1ffffff` to
  `0x3ffffff`; current summary adds `resist=0xfff/7->3`.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  `git diff --check`,
  and `git status --short -- decomp`.

## 2026-06-30 - Added Live-Hit Effect-Only Damage Branch Proof

- Added `STAGE_MPLIVEHIT_EFFECTONLY` to current modes `159/160`.
- The proof follows the source-shaped `ftMainUpdateDamageStatFighter`
  non-normal damage-coll branch: selected hurtbox slot `3` becomes
  invincible, the attack record and attacker `attack_damage` update, the
  set-off effect and hit SFX seams run, and damage queue, percent damage, and
  hitlog remain unchanged before restoring both fighter structs.
- Tightened `STAGE_MPLIVEHIT_DAMAGE` from mask low bits `0xffffff` to
  `0x1ffffff`; current summary adds `eff=0x1ff/0->0`.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  `git diff --check`,
  and `git status --short -- decomp`.

## 2026-06-30 - Added Live-Hit Shield-Contact Gate Proof

- Extended the current direct/menu-chain live-hit damage modes `159/160` with
  one bounded source-order shield-contact branch proof for the selected Fox
  Jab2 hitbox `1`.
- The proof follows the original `ftmain.c` shield branch shape for the
  selected state: `is_shield`, per-attack damage detect, active attack,
  bounded shield joint/sphere contact, shield-stat handoff, attack-record
  clear, and proof-local state restore.
- Added `STAGE_MPLIVEHIT_SHIELD_CONTACT` diagnostics and tightened
  `STAGE_MPLIVEHIT_DAMAGE` mask low bits from `0x3fffff` to `0x7fffff`.
  Current summary records
  `mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 second=0/j14/x140 hurt=1/10 hbdmg=0->4/6 shield=4->4/4 shc=0xff/3142 so=155/134 contact=1 repeat=1 rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 ids=0x3 dmg=0->4 hitlag=6`.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-06-29 - Added Source-Order Hurtbox Scan Proof

- Added `STAGE_MPLIVEHIT_HURTBOX` to the current direct/menu-chain live-hit
  damage-loop boundary.
- The proof reuses the selected Fox Jab2 attack-coll and project-owned
  rectangle/collide helper, temporarily marks Mario damage-coll slot `0`
  intangible, then follows the original `ftmain.c` loop shape: skip
  intangible hurtboxes, test the next non-`None` hurtbox, break on first hit,
  and observe the source-order `None` sentinel after Mario's `10` active
  slots.
- This advances the selected live-hit proof beyond slot-0-only hurtbox
  evidence without importing broad `ftmain.c` or `gmcollision.c`; natural
  multi-slot victim records, hitlag, and damage remain deferred.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`;
  `verify-boundary.ps1 -DelaySeconds 3`;
  `verify-current.ps1 -Build -DelaySeconds 3`;
  `check-docs.ps1`;
  `check-architecture.ps1`;
  `check-harness-registry.ps1`;
  `check-gbi-decode-fixtures.ps1`;
  `clean-generated.ps1 -DryRun`;
  `git diff --check` (existing CRLF warnings only).

## 2026-06-29 - Added Live-Hit Secondary Fox Jab2 Hitbox Probe

- Strengthened the existing direct/menu-chain live-hit damage lifecycle modes
  `159/160` without adding another harness pair.
- Added `STAGE_MPLIVEHIT_SECONDARY` to record the sibling Fox Jab2 hitbox `0`
  decoded from the original motion script: joint `14`, damage `4`, normalized
  radius `100`, forward offset `140`, angle `70`, and flags `0x7`.
- The probe locally runs bounded `New -> Transfer -> Interpolate`, range,
  rectangle, and selected collide gates while leaving the selected damage
  scheduling path on hitbox `1`.
- Tightened the live-hit proof mask from `0x7ffff` to `0xfffff` and updated
  the current summary marker with `second=0/j14/x140`.
- Verified:
  direct and menu-chain live-hit builds,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3 -NoBuild`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3 -NoBuild`,
  `verify-boundary.ps1 -DelaySeconds 3 -NoBuild`,
  `verify-current.ps1 -Build -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Seeded Source-Order Mario/Fox Damage-Coll Slots

- Replaced the bounded Mario/Fox init damage-coll seed with the same source
  shape used by original `ftmanager.c`: copy every valid
  `attr->damage_coll_descs[]` slot into `FTStruct.damage_colls[]`, set normal
  hit status, attach the source joint, and halve the descriptor size.
- The direct/menu-chain init verifiers now require Mario `10` active
  damage-coll slots and Fox `11`, while the current live-hit proof still uses
  selected slot `0` for collision scheduling.
- Deferred: natural multi-slot rectangle iteration, victim records, hitlag,
  damage, and broad `gmcollision.c` / `ftmain.c` imports.
- Verified:
  `verify-battle-mariofox-init-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-init-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -Build -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`, and
  `check-gbi-decode-fixtures.ps1`.

## 2026-06-30 - Added Original Rehit Refresh-ID Proof

- Strengthened the existing direct/menu-chain live-hit damage lifecycle modes
  `159/160` without adding another harness pair.
- Added a live-hit-local diagnostic around the imported original
  `ftCommonAttackAirLwProcUpdate` Link down-air rehit refresh call. The
  `ftParamRefreshAttackCollID` seam now records refresh IDs only while that
  imported callback is active, proving the original timer window refreshes
  attack-coll slots `0` and `1`.
- Extended `STAGE_MPLIVEHIT_ORIG_REHIT` and the current summary to print
  `ids=0x3`:
  `mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 shield=4->4/4 so=155/134 contact=1 repeat=1 rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 ids=0x3 dmg=0->4 hitlag=6`.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3 -NoBuild`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3 -NoBuild`,
  `verify-boundary.ps1 -DelaySeconds 3 -NoBuild`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  and `check-gbi-decode-fixtures.ps1`.

## 2026-06-29 - Added Live-Hit Shield-Stat GuardSetOff Proof

- Strengthened the existing direct/menu-chain live-hit damage lifecycle modes
  `159/160` without adding another harness pair.
- Added a bounded source-shaped `ftMainUpdateShieldStatFighter` side proof for
  the selected Fox Jab2 hitbox. The probe records attacker shield push, victim
  shield damage before/after/total, source-facing `shield_lr`, source-player
  diagnostic, one bounded set-off effect size, and the GuardSetOff
  status/motion/hitlag/clear path before restoring preview fighter state.
- Added `STAGE_MPLIVEHIT_SHIELD` and tightened the live-hit result mask from
  low bits `0x3ffff` to `0x7ffff`.
- Current marker:
  `mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 shield=4->4/4 so=155/134 contact=1 repeat=1 rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 dmg=0->4 hitlag=6`.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-29 - Added Link Down-Air ProcHit Rehit Seed Proof

- Strengthened the existing live-hit damage lifecycle modes `159/160` without
  adding another harness pair.
- Added a bounded original `ftCommonAttackAirLwProcHit` seed probe for Link's
  down-air rehit branch before the existing selected Fox Jab2 live-hit damage
  proof. The probe records AttackAirLw status/motion `213/188`, rehit timer
  `30`, vertical velocity `40000`, fastfall clear, animation frame `35000`,
  and attack-record clear mask `0x7`.
- Added live-hit-only `ftMainSetStatus` handling for AttackAirLw so the probe
  can install the original update/physics/map callback shape without changing
  the broader Dash-Run or damage-recover proofs.
- Extended the live-hit verifier marker to
  `mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 contact=1 repeat=1 rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 dmg=0->4 hitlag=6`.
- Added a default project-owned fallback for
  `ndsGRInishieScaleGetSourceSetupMapHead` so non-Inishie builds still link
  when the Inishie source-scale import glue is present.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -Build -DelaySeconds 3`,
  `verify-regression.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added Selected Original Rehit Timer Window to Live-Hit Proof

- Folded a bounded imported original `ftCommonAttackAirLwProcUpdate` rehit
  timer 30-tick window into the current direct/menu-chain MP live-hit damage
  proof.
- The probe temporarily uses the original Link down-air branch on the selected
  live shell, dirties attack records for IDs `0/1`, proves `rehit_timer`
  `30 -> 29 -> ... -> 1` without early refresh/clear, then proves `1 -> 0`
  refreshes both attack collisions to `New`, clears both records, and restores
  the proof-local fighter state.
- Tightened `STAGE_MPLIVEHIT_DAMAGE` from mask low bits `0x1ffff` to
  `0x3ffff` and added `STAGE_MPLIVEHIT_ORIG_REHIT`.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added MP Damage-Recover Lifecycle Proof

- Added direct/menu-chain harness modes `157/158`:
  `battle_mariofox_stage_mpdamage_recover_loop` and
  `menu_chain_mariofox_stage_mpdamage_recover_loop`.
- The new proof keeps the live VSBattle roots on Pupupu/Dream Land, consumes
  the Passive recover aggregate plus Dash-Run selected contact/damage setup
  proof, and adds a bounded source-shaped selected contact -> damage scheduling
  -> hitlag callback -> damage status -> DamageFall -> recovery branch path.
- Added `STAGE_MPDAMAGE_RECOVER_*` verifier markers for setup, contact,
  damage, status, callbacks, ground/DamageFall, PassiveStand/Passive/
  DownBounce branch reachability, velocity, and final safety.
- Fixed guarded compatibility regressions exposed by the new aggregate:
  CliffWaitDamage now follows the original DamageFall map branch order under
  the active proof, PassiveStand/Passive ground-set counters are recorded when
  guarded status shells set `ga`, DownStand interrupt checks run in source
  order, down-roll shells restore `is_jostle_ignore`, and Turn setup/update
  restores the original one-frame flip semantics.
- Current marker:
  `mpDamageRecover=contact=1/1 dmg=0->4 hitlag=6 status=52/45 fall=57/50 ps=1 passive=1 dbounce=1`.
- Continuous live hitbox activation, full `gmcollision.c`, full `ftmain.c`,
  arbitrary damage-state duration, complete hitlag/damage runtime, items,
  weapons, HUD, audio, and unbounded gameplay scheduling remain deferred.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-stage-mpdamage-recover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mpdamage-recover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mppassive-loop-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mpdownrecover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mpdownrecover-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`, and
  `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-29 - Repaired Mario/Fox Process-Loop Scheduler Prerequisite

- Repaired the bounded Mario/Fox movement proof ladder after the standalone
  process loop passed but scheduler-mode prerequisites stopped at
  `visits=0x1f3` / `transitions=0xff`.
- The root cause was a bounded-process status shim rejecting the
  JumpAnimEnd-driven `FallAerial` handoff in scheduler mode. The process-loop
  helper now coerces only that active JumpAnimEnd `FallAerial` case back to the
  intended `Fall` handoff, preserving the standalone process proof and avoiding
  a broad status rewrite.
- Kept the original-style jump velocity repair in place so Jump, process,
  scheduler, preview, and `gcRunAll` proofs all observe nonzero source-shaped
  airborne velocity before landing back to Wait/Ground.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-landing-loop-harness.ps1 -DelaySeconds 5`,
  `verify-battle-mariofox-process-loop-harness.ps1 -DelaySeconds 5`,
  `verify-battle-mariofox-scheduler-loop-harness.ps1 -DelaySeconds 5`,
  `verify-menu-chain-mariofox-scheduler-loop-harness.ps1 -DelaySeconds 5`,
  `verify-battle-mariofox-preview-loop-harness.ps1 -DelaySeconds 5`,
  `verify-menu-chain-mariofox-preview-loop-harness.ps1 -DelaySeconds 5`,
  `verify-battle-mariofox-gcrunall-loop-harness.ps1 -DelaySeconds 5`,
  `verify-menu-chain-mariofox-gcrunall-loop-harness.ps1 -DelaySeconds 5`,
  `verify-boundary.ps1 -DelaySeconds 5`,
  `verify-current.ps1 -DelaySeconds 5`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  `git diff --check`,
  and `git status --short -- decomp`.

## 2026-06-29 - Strengthened DamageCommon Ground Expiry Proof

- Strengthened the existing `DASH_RUN_DAMAGE_SETUP` update bit so it now
  proves both the installed damage update hitstun decrement and a bounded
  ground `ftCommonDamageCommonProcUpdate` expiry through
  `mpCommonSetFighterWaitOrFall` into Wait/Ground.
- The subprobe saves and restores the selected preview `FTStruct` and
  `anim_frame`; it does not add a new harness, marker bit, or continuous
  damage runtime.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added DamageUpdateMain Tail Branch Proofs

- Added bounded source-shaped `ftCommonDamageUpdateMain` no-grab/no-capture
  tail probes inside the existing Dash-Run procparams side proof.
- The first probe seeds zero knockback with no catch, capture, or item link,
  then reaches the original-compatible damage color-animation seam without
  changing fighter status.
- The second probe seeds non-sleep non-resist damage with no catch, capture,
  or item link, then routes through the bounded damage-status path before
  restoring the preview fighter state.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0xfffffff` to
  `0x3fffffff` in the direct/menu-chain Dash-Run verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-29 - Added DamageUpdateMain Catch Zero-Knockback Proof

- Added a bounded source-shaped `ftCommonDamageUpdateMain` catch-side
  zero-knockback probe inside the existing Dash-Run procparams side proof.
- The probe seeds the selected fighter with `catch_gobj`, proves the
  catch-resist predicate through `damage_knockback == 0`, takes the original
  grabbed-fighter-zero-knockback branch, reaches the existing damage
  color-animation seam, and restores both preview fighter structs.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x1fffff` to
  `0x3fffff` in the direct/menu-chain Dash-Run verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added DamageUpdateMain Capture Zero-Knockback Proof

- Added a bounded source-shaped `ftCommonDamageUpdateMain` capture-side
  zero-knockback probe inside the existing Dash-Run procparams side proof.
- The probe seeds the selected fighter as captured by the other fighter,
  proves the original captor-zero-knockback branch, sets captor hitlag, clears
  captured fighter tap/release input, runs the selected `proc_lagstart` tail,
  reaches the existing damage color-animation seam, and restores both preview
  fighter structs.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x3fffff` to
  `0x7fffff` in the direct/menu-chain Dash-Run verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added DamageUpdateMain Catch Non-Resist Stats Proof

- Added a bounded source-shaped `ftCommonDamageUpdateMain` catch-side
  non-resist keep-hold probe inside the existing Dash-Run procparams side
  proof.
- The probe seeds the selected fighter as catching the other fighter without
  satisfying the catch-resist predicate, proves the grabbed fighter keeps hold,
  runs imported original `ftCommonThrownUpdateDamageStats` on the grabbed
  fighter, then reaches the same bounded lose-grip / stop-voice /
  damage-status path before restoring both preview fighter structs.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x7fffff` to
  `0xffffff` in the direct/menu-chain Dash-Run verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added DamageUpdateMain Capture Non-Resist Stats Proof

- Added a bounded source-shaped `ftCommonDamageUpdateMain` capture-side
  non-resist keep-hold probe inside the existing Dash-Run procparams side
  proof.
- The probe seeds the selected fighter as captured by the other fighter,
  keeps the captured fighter under the release threshold, forces the captor
  out of catch-resist, runs imported original `ftCommonThrownUpdateDamageStats`
  on the captured fighter, then reaches the same bounded lose-grip /
  stop-voice / damage-status path before restoring both preview fighter
  structs.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0xffffff` to
  `0x1ffffff` in the direct/menu-chain Dash-Run verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added DamageUpdateMain Capture Keep-Hold Proof

- Added a bounded source-shaped `ftCommonDamageUpdateMain` capture-side
  keep-hold probe inside the existing Dash-Run procparams side proof.
- The probe seeds the selected fighter as captured by the other fighter,
  proves the captured fighter remains under the release threshold while the
  captor is in the catch-resist case, copies `damage_lag` / `hitlag_mul` back
  to the captured fighter, selects `nFTDamageKindCatch` on the captor, records
  the existing damage color-animation seam, and restores both preview fighter
  structs.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0xfffff` to
  `0x1fffff` in the direct/menu-chain Dash-Run verifiers.
- Updated current-truth docs so the first catch keep-hold, catch release, and
  capture keep-hold branches are covered while the remaining
  `ftCommonDamageUpdateMain` catch/capture/item branches stay deferred.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.
- Still deferred: importing full `ftcommondamage.c`, the remaining
  `ftCommonDamageUpdateMain` catch/capture/item/release branches, continuous
  damage/hitlag scheduling through real `ftmain.c`, full hitbox runtime, real
  fighter color-animation runtime, and broad `gmcollision.c`.

## 2026-06-29 - Added DamageUpdateMain Catch Release Proof

- Added a sibling bounded source-shaped `ftCommonDamageUpdateMain` catch
  branch probe inside the existing Dash-Run procparams side proof.
- The new probe seeds the selected fighter as caught by the other fighter,
  proves catch-resist with keep-hold false, routes through the same bounded
  lose-grip / stop-voice / damage-status install sequence, selects
  `nFTDamageKindStatus` on the grabbed fighter, and restores both preview
  fighter structs plus animation state.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x7ffff` to `0xfffff`
  in the direct/menu-chain Dash-Run verifiers.
- Updated diagnostic/current-truth docs so the catch-resist keep-hold branch
  and sibling release/lose-grip branch are now covered, while the remaining
  `ftCommonDamageUpdateMain` catch/capture/item branches stay deferred.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added DamageUpdateMain Capture Lose-Grip Proof

- Added a bounded source-shaped `ftCommonDamageUpdateMain` capture-side
  keep-hold false probe inside the existing Dash-Run procparams side proof.
- The probe seeds the selected fighter as captured by the other fighter, puts
  the captured fighter at the release threshold, keeps captor knockback
  nonzero, then reaches the original lose-grip / stop-voice / damage-status
  path without running thrown damage-stats before restoring both preview
  fighter structs.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x1ffffff` to
  `0x3ffffff` in the direct/menu-chain Dash-Run verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit Hit-Record Detect-Gate Proof

- Added a bounded source-order hit-record detect-gate proof inside the
  existing direct/menu-chain live-hit modes `159/160`.
- The proof mirrors the BattleShip fighter-vs-fighter branch that enables
  `gFTMainIsDamageDetect[i]` only when the selected attack record has no prior
  hurt/shield interaction and its group is still the default `7`.
- The current `STAGE_MPLIVEHIT_REHIT` diagnostic now records `gate=0x3f`,
  proving the empty/default allow case, hurt skip, shield skip, nondefault
  group skip, and restored positive path before the selected Fox Jab2 damage
  proof continues.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit Same-Group Attack-Record Carry Proof

- Added a bounded source-shaped same-group attack-record carry/clear proof
  inside existing direct/menu-chain live-hit modes `159/160`.
- Added `gNdsFighterDashRunAttackEventRecordCarryMask` and threaded it into
  `STAGE_MPLIVEHIT_EVENTS`; current passing proof records `carry=0xf`.
- The proof uses proof-only attack-coll slots `2` and `3` with save/restore to
  prove the original MakeAttackColl same-group copy branch and no-sibling clear
  branch without adding a new harness or unbounding fighter runtime.
- Tightened `STAGE_MPLIVEHIT_DAMAGE` from mask low bits `0x7fffff` to
  `0xffffff` in the direct/menu-chain live-hit verifiers.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit Hurtbox Tested-Miss Before Hit Proof

- Extended the current direct/menu-chain live-hit modes `159/160` so the
  source-order hurtbox scan now proves a normal tested miss before the first
  hit.
- The proof still marks Mario damage-coll slots `0` and `1` intangible, then
  runs the rectangle/collide helper against slot `2` as a miss, continues to
  slot `3`, breaks on the first hit, records the slot-10 `None` sentinel, and
  feeds selected slot `3` through bounded damage record, hitlog, stats,
  percent-damage, and hitlag scheduling before restoring fighter state.
- The hurtbox mask now requires low bits `0x1ffff`, adding the explicit
  tested-miss bit. The current marker is now
  `mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 second=0/j14/x140 hurt=3/10 hbdmg=0->4/6 shield=4->4/4 shc=0x3fffff/3142 so=155/134 contact=1 repeat=1 carry=0xf gate=0x3f rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 ids=0x3 dmg=0->4 hitlag=6`.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Advanced Live-Hit Hurtbox Scan To Slot 2

- Extended the current direct/menu-chain live-hit modes `159/160` from a
  slot-1 positive hurtbox hit to a source-order slot-2 hit.
- The proof now temporarily marks Mario damage-coll slots `0` and `1`
  intangible, then proves the original loop skips both, tests slot `2`,
  records the slot-10 `None` sentinel, and feeds selected slot `2` through
  the bounded damage record, hitlog, stats, percent-damage, and hitlag
  scheduling handoff before restoring both fighter structs.
- The current marker is now
  `mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 second=0/j14/x140 hurt=2/10 hbdmg=0->4/6 shield=4->4/4 shc=0x3fffff/3142 so=155/134 contact=1 repeat=1 carry=0xf gate=0x3f rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 ids=0x3 dmg=0->4 hitlag=6`.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit Shield Hitlag-Mul Tail Reset Proof

- Extended the existing `STAGE_MPLIVEHIT_SHIELD_CONTACT` proof in current
  modes `159/160` from mask low bits `0x1fffff` to `0x3fffff`.
- The new bit proves the selected normal shield-contact tail resets
  `hitlag_mul` to `1.0F` in the same source-order `ftMainProcParams` tail
  block that clears attack, shield, damage, special, rebound, and knockback
  transients.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit Shield Special Tail Clear Proof

- Extended the existing `STAGE_MPLIVEHIT_SHIELD_CONTACT` proof in current
  modes `159/160` from mask low bits `0xfffff` to `0x1fffff`.
- The new bit proves the selected normal shield-contact tail also clears
  `reflect_lr`, `reflect_damage`, `absorb_lr`, `attack_rebound`, and
  `damage_knockback` in the same source-order tail block.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit Shield Tail Clear Proof

- Extended the existing `STAGE_MPLIVEHIT_SHIELD_CONTACT` proof in current
  modes `159/160` from mask low bits `0x7ffff` to `0xfffff`.
- The new bit proves the selected normal GuardSetOff shield-contact branch
  runs the common `ftMainProcParams` post-branch tail that clears
  `attack_damage`, `attack_shield_push`, `damage_lag`, `damage_queue`, and
  `damage_kind`, while leaving knockback pause unset for the shield path.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit ShieldBreak Hitlag Clear Proof

- Extended the existing `STAGE_MPLIVEHIT_SHIELD_CONTACT` proof in current
  modes `159/160` from mask low bits `0x3ffff` to `0x7ffff`.
- The new bit proves the selected low-shield ShieldBreakFly branch runs the
  common post-shield `damage != 0` tail: hitlag calculation, input tap/release
  clear, lagstart callback, and transient `shield_damage` /
  `shield_damage_total` clear while preserving ShieldBreakFly status.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit Shield-Heal Branch Proof

- Extended the existing `STAGE_MPLIVEHIT_SHIELD_CONTACT` proof in current
  modes `159/160` from mask low bits `0x1ffff` to `0x3ffff`.
- The new bit proves the selected not-shielding, below-full shield branch
  decrements `shield_heal_wait`, restores shield health by one, and resets
  `shield_heal_wait` to `10` in the original `ftMainProcParams` order.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit ShieldBreakFly Branch Proof

- Extended the existing `STAGE_MPLIVEHIT_SHIELD_CONTACT` proof in current
  modes `159/160` from mask low bits `0xffff` to `0x1ffff`.
- The new bit proves the selected low-shield branch subtracts
  `shield_damage_total`, detects shield break, sets shield health to `30`,
  and reaches ShieldBreakFly through the existing project-owned
  compatibility seam while restoring proof-local state.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added Live-Hit Shield-Health Decrement Proof

- Extended the existing `STAGE_MPLIVEHIT_SHIELD_CONTACT` proof in current
  modes `159/160` from mask low bits `0x7fff` to `0xffff`.
- The new bit proves the selected shield contact subtracts
  `shield_damage_total` from `shield_health` in the original
  `ftMainProcParams` order before the existing bounded GuardSetOff and hitlag
  clear proof.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added DamageUpdateMain Capture No-Damage Release Proof

- Added a bounded source-shaped `ftCommonDamageUpdateMain` capture-side
  keep-hold false zero-knockback probe inside the existing Dash-Run
  procparams side proof.
- The probe seeds the selected fighter as captured by the other fighter,
  puts the captured fighter at the release threshold, gives the captor zero
  knockback, routes the captured fighter into the bounded damage-status path,
  then calls imported original `ftCommonThrownSetStatusNoDamageRelease` on
  the captor before restoring both preview fighter structs.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x3ffffff` to
  `0x7ffffff` in the direct/menu-chain Dash-Run verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added DamageUpdateMain Catch Damage-Release Proof

- Added a bounded source-shaped `ftCommonDamageUpdateMain` catch-side
  non-resist zero-knockback probe inside the existing Dash-Run procparams side
  proof.
- The probe seeds the selected fighter as catching the other fighter, forces
  the catcher out of catch-resist while the grabbed fighter has zero
  knockback, routes the grabbed fighter through imported original
  `ftCommonThrownSetStatusDamageRelease`, clears the catch link, then routes
  the catcher into the bounded damage-status path before restoring both
  preview fighter structs.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x7ffffff` to
  `0xfffffff` in the direct/menu-chain Dash-Run verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-29 - Added DamageUpdateMain Heavy-Item Branch Proof

- Added bounded source-shaped `ftCommonDamageUpdateMain` DK-family heavy-item
  probes inside the existing Dash-Run procparams side proof.
- The catch-resist probe seeds a fake heavy item on Donkey, reaches the
  original-compatible damage colanim seam, and keeps the held item link.
- The drop probe seeds a fake heavy item on Giant Donkey, forces the
  non-resist branch through `ftSetupDropItem`, clears the item link, and
  reaches the bounded damage-status path before restoring preview state.
- Tightened `DASH_RUN_PROCPARAMS` from mask low bits `0x3fffffff` to
  `0xffffffff` in the direct/menu-chain Dash-Run verifiers.
- Verified:
  `make -j16`,
  `verify-battle-mariofox-dash-run-harness.ps1 -DelaySeconds 3`, and
  `verify-menu-chain-mariofox-dash-run-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Live-Hit Catch-Stat Distance/Search Proof

- Added a bounded source-shaped catch-stat proof to current modes `159/160`.
  The proof follows the tail of original `ftMainUpdateCatchStatFighter` while
  reusing the existing project-owned `ftMainSetHitInteractStats` seam, records
  the selected hurt interaction and attack-detect clear, computes the
  source-order absolute X distance, assigns the closest `search_gobj`, and
  restores fighter/root/detect state.
- Tightened `STAGE_MPLIVEHIT_DAMAGE` from mask low bits `0x1fffffff` to
  `0x3fffffff` and added verifier marker `catchStat=0x1f/160000`.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  and `git diff --check`.

## 2026-06-30 - Added DamageInit Damage Voice SFX Branch Proof

- Named `FTAttributes.damage_sfx` at the original-compatible attribute offset
  used by BattleShip's damage-init path.
- Added the bounded source-shaped damage voice branch to the project-owned
  `ftCommonDamageInitDamageVars` seam, covering the hitstun-threshold and
  forced-call gates without promoting full audio.
- Added `DASH_RUN_DAMAGE_VOICE` to the Dash-Run damage proof. The marker
  records `voice=0xf`, at least two captured calls, and distinct threshold and
  forced FGM IDs through the existing audio stub seam.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added DamageInit Random FlyRoll Branch Proof

- Added the original-shaped airborne non-FlyTop percent/RNG FlyRoll branch to
  the project-owned `ftCommonDamageInitDamageVars` seam using BattleShip
  `syUtilsRandFloat`.
- Added `DASH_RUN_DAMAGE_FLYROLL`, recording `flyroll=0x1f`, DamageFlyRoll
  status/motion `55/48`, percent `100+`, RNG consumption, and proof-local
  RNG/fighter-state restore.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -DelaySeconds 3`,
  `check-docs.ps1`,
  `check-architecture.ps1`,
  `check-harness-registry.ps1`,
  `check-gbi-decode-fixtures.ps1`,
  `clean-generated.ps1 -DryRun`,
  `git diff --check`,
  and `git status --short decomp`.

## 2026-06-30 - Added DamageInit Kirby Copy-Loss Branch Proof

- Added the bounded source-shaped Kirby copy-loss branch to the project-owned
  `ftCommonDamageInitDamageVars` seam, calling the local
  `ftKirbySpecialNDamageCheckLoseCopy` compatibility helper when
  `damage_level == 3` and copy loss is allowed.
- Upgraded the local `ftKirbySpecialNLoseCopy` seam from a no-op to reset the
  proof-local Kirby copy ID to `nFTKindKirby` and fire the original FGM ID
  `204`; full Kirby effect/model-part runtime remains deferred.
- Added `DASH_RUN_DAMAGE_KIRBYCOPY`, recording `kirbycopy=0x6`, copy ID
  `1->8`, and FGM `204` through the existing Dash-Run damage verifier path.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added ProcParams Attack-Damage Rumble Branch Proof

- Added a bounded source-shaped `ftMainProcParams` attacker-side
  `attack_damage` rumble proof inside the existing Dash-Run procparams side
  proof.
- The proof records the normal damage-derived `ftParamMakeRumble` ID/length
  branch, then temporarily seeds `nFTStatusAttackIDBatSwing4` to prove the
  original BatSwing4 special case calls rumble ID `10` with length `0`, before
  restoring fighter state.
- Added `DASH_RUN_PROCPARAMS_RUMBLE`, recording `procRumble=0x3`, at least two
  captured calls, and final ID/length `10/0` through the project-owned rumble
  seam.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Damage Dust Interval Threshold Proof

- Added `DASH_RUN_DAMAGE_DUST` to the existing Dash-Run damage setup proof,
  recording `dust=0xff` without adding a new harness mode.
- The proof drives the imported-original
  `ftCommonDamageSetDustEffectInterval` low, mid-low, mid, mid-high, high,
  and default air threshold buckets, records packed waits `0x123580`, and
  restores fighter state.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Dust Thresholds Through Imported ftcommondamage.c

- Routed the existing `DASH_RUN_DAMAGE_DUST` proof through imported original
  BattleShip `ftCommonDamageSetDustEffectInterval` instead of the local
  source-shaped helper.
- Strengthened the dust marker from `0x7f` to `0xff`; the high bit records
  imported-original routing while the packed wait proof remains `0x123580`.

## 2026-06-30 - Added Damage Public Reaction Branch Proof

- Added `DASH_RUN_DAMAGE_PUBLIC` to the existing Dash-Run damage setup proof,
  recording `public=0x1f` without adding a new harness mode.
- The proof drives the source-shaped `ftCommonDamageSetPublic` angle-window
  branch with knockback `200` at 80 degrees, proving reduced public knockback
  `160000`, target public-knockback reset, the very-high attacker force
  handoff, and the default non-forced attacker branch through
  `ftPublicCommonCheck`.
- The probe now restores only the changed fields and the diagnostic active flag
  so inherited live-hit and damage-recover proofs keep their surrounding
  fighter state.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Damage Dust Runtime Update Proof

- Added `DASH_RUN_DAMAGE_DUST_UPDATE` to the existing Dash-Run damage setup
  proof, recording `dustUpdate=0xf` without adding a new harness mode.
- The proof drives the source-shaped `ftCommonDamageUpdateDustEffect` runtime
  through a nonzero decrement/no-spawn path and a zero-cross DustExpandLarge
  spawn plus dust interval reset, then restores only the touched fighter fields
  and proof counters.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Damage Hitstun/Public Decay Proof

- Added `DASH_RUN_DAMAGE_HITSTUN_PUBLIC` to the existing Dash-Run damage setup
  proof, recording `hitPublic=0xf` without adding a new harness mode.
- The proof drives imported original `ftCommonDamageDecHitStunSetPublic`
  branch through nonzero hitstun decrement and zero-cross public-knockback
  transfer, then restores the touched fighter fields and records
  imported-original routing.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Hitstun/Public Decay Through Imported ftcommondamage.c

- Routed the existing `DASH_RUN_DAMAGE_HITSTUN_PUBLIC` proof through imported
  original BattleShip `ftCommonDamageDecHitStunSetPublic`.
- Strengthened the current direct/menu-chain marker from `hitPublic=0x7` to
  `hitPublic=0xf`, preserving nonzero decrement, zero-cross public-knockback
  transfer, and proof-local restore while adding an imported-original routing
  bit.

## 2026-06-30 - Added Damage Screen Flash Routing Proof

- Added `DASH_RUN_DAMAGE_SCREEN_FLASH` to the existing Dash-Run damage setup
  proof, recording `flash=0x7f` without adding a new harness mode.
- The proof drives imported original `ftCommonDamageCheckMakeScreenFlash`
  branch through the low-knockback no-op and high-knockback Fire, Electric,
  Freezing, and default colanim routes, then restores the proof counters.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Screen Flash Through Imported ftcommondamage.c

- Routed the existing `DASH_RUN_DAMAGE_SCREEN_FLASH` proof through imported
  original BattleShip `ftCommonDamageCheckMakeScreenFlash`.
- Strengthened the current direct/menu-chain marker from `flash=0x3f` to
  `flash=0x7f`, preserving the low-knockback no-op and four high-knockback
  element routes while adding an imported-original routing bit.

## 2026-06-30 - Added Damage Color Animation Routing Proof

- Added `DASH_RUN_DAMAGE_COLANIM` to the existing Dash-Run damage setup proof,
  recording `colAnim=0x3f` without adding a new harness mode.
- The proof drives imported original `ftCommonDamageCheckElementSetColAnim`
  through Fire, Electric, Freezing, and default routes, then restores the proof
  counters and records imported-original routing.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Color Animation Through Imported ftcommondamage.c

- Routed the existing `DASH_RUN_DAMAGE_COLANIM` proof through imported original
  BattleShip `ftCommonDamageCheckElementSetColAnim`.
- Strengthened the current direct/menu-chain marker from `colAnim=0x1f` to
  `colAnim=0x3f`, preserving the Fire/Electric/Freezing/default routes,
  proof-local restore, and adding an imported-original routing bit.

## 2026-06-30 - Added Damage ColAnim Update Wrapper Proof

- Added `DASH_RUN_DAMAGE_COLANIM_UPDATE` to the existing Dash-Run damage setup
  proof, recording `colAnimUpdate=0x1f` without adding a new harness mode.
- The proof drives imported original `ftCommonDamageUpdateDamageColAnim` and
  `ftCommonDamageSetDamageColAnim`, then proves direct wrapper routing,
  struct-field wrapper routing, gated no-update behavior, proof-local restore,
  and imported-original routing through the existing `ftMainRunUpdateColAnim`
  seam.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage ColAnim Update Through Imported ftcommondamage.c

- Routed the existing `DASH_RUN_DAMAGE_COLANIM_UPDATE` proof through imported
  original BattleShip `ftCommonDamageUpdateDamageColAnim` and
  `ftCommonDamageSetDamageColAnim`.
- Strengthened the current direct/menu-chain marker from `colAnimUpdate=0xf`
  to `colAnimUpdate=0x1f`, preserving direct wrapper routing, struct-field
  wrapper routing, gated no-update behavior, and proof-local restore while
  adding an imported-original routing bit.

## 2026-06-30 - Added Damage Invincibility Gate Proof

- Added `DASH_RUN_DAMAGE_INVINCIBLE` to the existing Dash-Run damage setup
  proof, recording `invGate=0xf` without adding a new harness mode.
- The proof drives bounded source-shaped `ftCommonDamageCheckSetInvincible`
  through the hitlag-blocked gate, knockback-over flag gate, timed-invincible
  true branch, and proof-local restore.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Damage Lag-Update Gate Proof

- Added `DASH_RUN_DAMAGE_LAGUPDATE` to the existing Dash-Run damage setup
  proof, recording `lagUpdate=0x1f` without adding a new harness mode.
- The proof drives bounded source-shaped `ftCommonDamageCommonProcLagUpdate`
  through the hitlag-blocked gate, stick-range gate, tap-buffer gate, active
  Smash DI root translation branch, and proof-local restore.
- Verified after implementation:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Live-Hit GuardSetOff Tick Proof

- Added `STAGE_MPLIVEHIT_SHIELD_SETOFF_TICK` to the existing live-hit shield
  proof, recording `soTick=0x1f/155->154` without adding a new harness mode.
- The proof drives imported original `ftCommonGuardSetOffProcUpdate` from the
  selected shield-stat -> GuardSetOff branch: held Z decrements
  `setoff_frames` and stays in GuardSetOff, released Z exits to GuardOff, then
  the proof restores the local fighter state.
- Verified after implementation:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Damage Common Physics Branch Proof

- Added `DASH_RUN_DAMAGE_COMMON_PHYSICS` to the existing Dash-Run damage setup
  proof, recording `commonPhys=0x1f` without adding a new harness mode.
- The proof drives the bounded source-shaped
  `ftCommonDamageCommonProcPhysics` callback through ground friction, air
  friction, air drift/gravity, low-speed throw attack-clear, and proof-local
  restore branches.
- Verified after implementation:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Repaired Live-Hit Dash-Run Attack11 Additive Gate

- Repaired the inherited Dash-Run aggregate gate used by the current live-hit
  modes: Attack11 call counters now require at least two source-path calls
  while status/motion IDs and callback/tick masks remain exact.
- Updated the current docs for the existing `commonCb=0x7f` common damage
  callback marker.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added DamageAir Wall-Map Short-Circuit Proof

- Added `DASH_RUN_DAMAGE_AIR_MAP_WALL` to the existing Dash-Run damage setup
  proof, recording `airMapWall=0x1f` without adding a new harness mode.
- The proof drives the bounded DamageAir map callback through a left-wall
  collision, original WallDamage helper side effects, Passive/DownBounce
  short-circuit, reflected knockback/LR, and proof-local restore.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added DamageFall Source-Order Interrupt Proof

- Added `DASH_RUN_DAMAGE_FALL_INTERRUPT` to the existing Dash-Run damage setup
  proof, recording `fallInterrupt=0x3f` without adding a new harness mode.
- The proof calls imported original `ftCommonDamageFallProcInterrupt` through
  the project-owned wrapper and records source-order special-air, attack-air,
  jump-aerial, hammer fallback, and restore evidence.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Damage Knockback-Angle Branch Proof

- Added `DASH_RUN_DAMAGE_KNOCKBACK_ANGLE` to the existing Dash-Run damage setup
  proof, recording `angle=0x3f` without adding a new harness mode.
- The proof covers fixed-angle, air `361`, ground low-knockback `361`, ground
  high-knockback scaled `361`, and capped ground `361` branches through the
  imported-original `ftCommonDamageGetKnockbackAngle` helper.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Knockback Angle Probe Through Imported ftcommondamage.c

- Routed the existing `DASH_RUN_DAMAGE_KNOCKBACK_ANGLE` proof through imported
  original BattleShip `ftCommonDamageGetKnockbackAngle`.
- Strengthened the angle marker from `0x1f` to `0x3f`; the high bit records
  imported-original routing while branch coverage remains fixed-angle, air
  `361`, and ground low/high/capped `361`.

## 2026-06-30 - Added DamageInit Deterministic FlyTop Branch Proof

- Added `DASH_RUN_DAMAGE_FLYTOP` to the existing Dash-Run damage setup proof,
  recording `flytop=0xf` without adding a new harness mode.
- The proof drives the source-shaped `ftCommonDamageInitDamageVars` airborne
  high-knockback 90-degree angle-window path into DamageFlyTop status/motion
  `54/47`, then restores the local fighter state.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added DamageInit Replacement/Electric Wrapper Proof

- Added `DASH_RUN_DAMAGE_REPLACE_ELECTRIC` to the existing Dash-Run damage
  setup proof, recording `replace=0xf` without adding a new harness mode.
- The proof drives the source-shaped `ftCommonDamageInitDamageVars` final
  status replacement plus electric wrapper branch: replacement status `55` is
  stored, electric status/motion `50/43` is installed, the electric passive
  handoff is selected, and the local fighter state is restored.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Strengthened DamageInit Replacement/Electric Dispatch Proof

- Strengthened the existing `DASH_RUN_DAMAGE_REPLACE_ELECTRIC` proof without
  adding a new harness mode. The marker now records `replace=0x1f`.
- The proof still verifies replacement status `55` storage and electric
  status/motion `50/43`, then ticks the selected passive callback once and
  proves it dispatches to the stored replacement status/motion `55/48` before
  restoring local fighter state.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Strengthened Common Damage Callback Stay/Expiry Proof

- Strengthened `DASH_RUN_DAMAGE_COMMON_CALLBACKS` without adding a new harness
  mode. The marker now records `commonCb=0x1ff`.
- The proof now covers ground update non-expiry while hitstun remains, air
  update non-expiry while animation frames remain, ground expiry to Wait, air
  expiry to DamageFall, ground/air interrupt routing, hammer routing, and
  proof-local restore.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Damage Hold/Resist Gate Proof

- Added `DASH_RUN_DAMAGE_HOLD_RESIST` to the existing Dash-Run damage setup
  proof, recording `hold=0xff` without adding a new harness mode.
- The proof mirrors the original `ftCommonDamageCheckCatchResist` and
  `ftCommonDamageCheckCaptureKeepHold` gates through project-local helpers,
  proving Sleep false, zero-knockback true, paused low-stack true, Donkey
  cargo throw low-level true, default high-knockback false, keep-hold
  true/false thresholds, and proof-local restore.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Damage-Level Threshold Proof

- Added `DASH_RUN_DAMAGE_LEVELS` to the existing Dash-Run damage setup proof,
  recording `level=0x1f` without adding a new harness mode.
- The proof now calls imported original `ftCommonDamageGetDamageLevel` for
  low/mid/high/fly levels at hitstun `0`, `12`, `24`, and `32`.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Levels Through Imported ftcommondamage.c

- Routed the existing `DASH_RUN_DAMAGE_LEVELS` proof through imported original
  BattleShip `ftCommonDamageGetDamageLevel`.
- Strengthened the current direct/menu-chain marker from `level=0xf` to
  `level=0x1f`, preserving the four threshold branches and adding an
  imported-original routing bit.

## 2026-06-30 - Added Damage Item-Bypass Fallthrough Proof

- Added `DASH_RUN_DAMAGE_ITEM_BYPASS` to the existing Dash-Run damage proof,
  recording `itemBypass=0x1f` without adding a new harness mode.
- The proof mirrors the `ftCommonDamageUpdateMain` held-item gate: light items
  and heavy items held by non-DK fighters skip the heavy-item branch, keep the
  item attached, reach the normal damage color-animation tail, and restore
  local state.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Damage Heavy-Item Branch Marker

- Added `DASH_RUN_DAMAGE_ITEM_HEAVY` to the existing Dash-Run damage proof,
  recording `itemHeavy=0x1f` without adding a new harness mode.
- The proof mirrors the `ftCommonDamageUpdateMain` DK-family heavy-item branch,
  proving the heavy-item predicate, catch-resist return, drop/status return,
  and proof-local restore.
- Verified direct current-boundary harness:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Corrected Runtime Action-Preview Verifier Expectation

- Updated `verify-runtime.ps1` to match the current bounded opening bridge:
  nine action-scene bridge boundaries, three cached original action-preview
  sprites, and 324 paced preview frames.

## 2026-06-30 - Preserved Damage Kind Across Damage Init

- Corrected the project-owned `ftCommonDamageInitDamageVars` seam so the
  selected hit element no longer overwrites `FTStruct.damage_kind`.
- Added `DASH_RUN_DAMAGE_KIND` to the existing Dash-Run damage aggregate,
  recording `dmgKind=0x7` without adding a new harness mode. The proof seeds
  `nFTDamageKindCatch`, runs damage-var setup with a non-aliasing hit element,
  then verifies before/after preservation and proof-local restore.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Strengthened Damage SetStatus Hitlag Block Proof

- Strengthened the existing `DASH_RUN_DAMAGE_REPLACE_ELECTRIC` proof without
  adding a new harness mode. The marker now records `replace=0x3f`.
- The proof calls the selected electric `proc_passive` while `hitlag_tics > 0`
  and verifies status, motion, stored replacement status, and animation-event
  calls remain unchanged before the existing zero-hitlag dispatch reaches stored
  replacement status/motion `55/48`.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Damage UpdateCatchResist Branch Proof

- Added `DASH_RUN_DAMAGE_UPDATE_CATCH_RESIST` to the existing Dash-Run damage
  proof, recording `catchResist=0x1f` without adding a new harness mode.
- The proof mirrors the branch shape of
  `ftCommonDamageUpdateCatchResist`: zero-knockback and paused low-stack
  knockback route through the existing color-animation seam, while the
  non-resist branch reaches the project-owned voice-stop/damage-status side
  seam before proof-local restore. Full DK throw damage runtime remains
  deferred.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Imported Bounded ftcommondamage.c for UpdateCatchResist

- Added `src/import/battleship_ftcommon_damage.c`, remapping original
  BattleShip `ftcommondamage.c` symbols to `ndsBase*` names so project-owned
  damage seams remain in control.
- Strengthened `DASH_RUN_DAMAGE_UPDATE_CATCH_RESIST` from `0xf` to `0x1f` by
  calling imported original `ndsBaseFTCommonDamageUpdateCatchResist` for the
  zero-knockback color-animation branch and observing the original
  `ftMainRunUpdateColAnim` callback seam. The DK throw-damage branch remains a
  compile seam only; full Donkey throw damage runtime is still deferred.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Dust Update Through Imported ftcommondamage.c

- Strengthened `DASH_RUN_DAMAGE_DUST_UPDATE` from `0xf` to `0x1f` by calling
  imported original `ndsBaseFTCommonDamageUpdateDustEffect`.
- The proof keeps the existing decrement/no-spawn and zero-cross
  DustExpandLarge spawn checks, observes the original interval reset through
  post-reset wait `5`, and restores proof-local fighter/counter state.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Public Reaction Through Imported ftcommondamage.c

- Added a narrow project-owned `ftParamGetPlayerNumGObj` compatibility seam
  over the existing Mario/Fox player lookup.
- Strengthened `DASH_RUN_DAMAGE_PUBLIC` from `0x1f` to `0x3f` by calling
  imported original `ndsBaseFTCommonDamageSetPublic`.
- The proof keeps the existing angle reduction, target public reset, forced
  and non-forced public-check branches, and proof-local restore.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Public Damage Invincibility Gate Seam Through Imported ftcommondamage.c

- Strengthened `DASH_RUN_DAMAGE_INVINCIBLE` from `0xf` to `0x1f` by calling
  imported original `ndsBaseFTCommonDamageCheckSetInvincible`.
- The proof keeps the existing hitlag block, knockback-over flag block,
  timed-invincible true branch, and proof-local restore.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Lag Update Through Imported ftcommondamage.c

- Replaced the project-owned `ftCommonDamageCommonProcLagUpdate` body with a
  guarded call into imported original `ndsBaseFTCommonDamageCommonProcLagUpdate`.
- Strengthened `DASH_RUN_DAMAGE_LAGUPDATE` from `0x1f` to `0x3f`, keeping the
  hitlag block, stick-range block, tap-buffer block, Smash DI translation, and
  proof-local restore checks.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Common Damage Physics Through Imported ftcommondamage.c

- Replaced the project-owned `ftCommonDamageCommonProcPhysics` body with a
  guarded call into imported original `ndsBaseFTCommonDamageCommonProcPhysics`.
- Strengthened `DASH_RUN_DAMAGE_COMMON_PHYSICS` from `0x1f` to `0x3f`, keeping
  the ground-friction, air-friction, air-drift/gravity, low-speed throw
  attack-clear, and proof-local restore checks.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Ground Common Damage Update Through Imported ftcommondamage.c

- Replaced the project-owned `ftCommonDamageCommonProcUpdate` body with a
  guarded call into imported original `ndsBaseFTCommonDamageCommonProcUpdate`.
- Strengthened `DASH_RUN_DAMAGE_COMMON_CALLBACKS` from `0x1ff` to `0x3ff`,
  keeping the ground stay, ground expiry, air update, interrupt, hammer, and
  proof-local restore checks.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Air Common Damage Update Through Imported ftcommondamage.c

- Replaced the project-owned `ftCommonDamageAirCommonProcUpdate` body with a
  guarded call into imported original `ndsBaseFTCommonDamageAirCommonProcUpdate`.
- Strengthened `DASH_RUN_DAMAGE_COMMON_CALLBACKS` from `0x3ff` to `0x7ff`,
  keeping the ground update, air update, interrupt, hammer, and restore checks
  while proving both ground and air update stay/expiry branches through
  imported-original routing.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Common Damage Interrupt Through Imported ftcommondamage.c

- Replaced the project-owned `ftCommonDamageCommonProcInterrupt` body with a
  guarded call into imported original
  `ndsBaseFTCommonDamageCommonProcInterrupt`.
- Strengthened `DASH_RUN_DAMAGE_COMMON_CALLBACKS` from `0x7ff` to `0xfff`,
  keeping ground update, air update, interrupt, hammer, and restore checks
  while proving common interrupt ground/hammer branches through
  imported-original routing.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Air Common Damage Interrupt Through Imported ftcommondamage.c

- Replaced the project-owned `ftCommonDamageAirCommonProcInterrupt` body with a
  guarded call into imported original
  `ndsBaseFTCommonDamageAirCommonProcInterrupt`.
- Strengthened `DASH_RUN_DAMAGE_COMMON_CALLBACKS` from `0xfff` to `0x1fff`,
  keeping ground update, air update, common interrupt, AirCommon interrupt,
  hammer, and restore checks while proving AirCommon interrupt through
  imported-original routing.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Air Common Damage Map Through Imported ftcommondamage.c

- Replaced the copied project-owned `ftCommonDamageAirCommonProcMap` branch with
  a guarded call into imported original
  `ndsBaseFTCommonDamageAirCommonProcMap`.
- Added the narrow public `ftCommonWallDamageCheckGoto` compatibility alias
  back to the already-imported WallDamage helper, matching the name expected by
  original `ftcommondamage.c`.
- Strengthened `DASH_RUN_DAMAGE_AIR_MAP_WALL` from `0x1f` to `0x3f`, proving
  the DamageAir wall-map route still reaches collision, WallDamage side
  effects, Passive/DownBounce short-circuit, reflected knockback/LR,
  imported-original AirCommon map routing, and proof-local restore.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Invincibility Gate Through Imported ftcommondamage.c

- Replaced the project-owned `ftCommonDamageCheckSetInvincible` body with a
  guarded call into imported original
  `ndsBaseFTCommonDamageCheckSetInvincible`.
- Changed the existing `DASH_RUN_DAMAGE_INVINCIBLE` probe to call the public
  seam, so the unchanged `invGate=0x1f` marker now proves the public seam's
  imported-original route rather than only the private imported helper.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Status Passive Dispatch Through Imported ftcommondamage.c

- Replaced the project-owned `ftCommonDamageSetStatus` body with a guarded
  call into imported original `ndsBaseFTCommonDamageSetStatus`.
- Kept the existing `DASH_RUN_DAMAGE_SETUP` marker stable at
  `damageSetup=0xffffffff`; the current proof already drives the electric
  passive status-dispatch tick through the public seam, now reaching original
  BattleShip status handoff code.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage FlyRoll Pitch Through Imported ftcommondamage.c

- Replaced the project-owned `ftCommonDamageFlyRollUpdateModelPitch` body with
  a guarded call into imported original
  `ndsBaseFTCommonDamageFlyRollUpdateModelPitch`.
- Changed the existing FlyRoll physics pitch proof to expect original
  `syUtilsArcTan2` math instead of libc `atan2f`.
- Kept the existing `DASH_RUN_DAMAGE_SETUP` marker stable at
  `damageSetup=0xffffffff`; the FlyRoll pitch/update-and-throw-clear physics
  tail now reaches original BattleShip code.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage ColAnim Update Public Wrappers Through Imported ftcommondamage.c

- Replaced the project-owned `ftCommonDamageUpdateDamageColAnim` and
  `ftCommonDamageSetDamageColAnim` bodies with guarded calls into imported
  original `ndsBaseFTCommonDamageUpdateDamageColAnim` /
  `ndsBaseFTCommonDamageSetDamageColAnim`.
- Changed the existing `DASH_RUN_DAMAGE_COLANIM_UPDATE` probe to call the
  public seams instead of the private imported helpers directly.
- Kept the marker stable at `colAnimUpdate=0x1f`, proving direct wrapper
  routing, struct-field wrapper routing, gated no-update behavior,
  proof-local restore, and imported-original routing through the public seams.
- Verified:
  `make NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_damage_loop -j16`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Dust and Hitstun Public Wrappers Through Imported ftcommondamage.c

- Replaced the project-owned `ftCommonDamageUpdateDustEffect` and
  `ftCommonDamageDecHitStunSetPublic` bodies with guarded calls into imported
  original `ndsBaseFTCommonDamageUpdateDustEffect` /
  `ndsBaseFTCommonDamageDecHitStunSetPublic`.
- Added the two public declarations to `include/ft/fighter.h` and changed the
  existing dust-update and hitstun/public probes to call the public seams.
- Kept the markers stable at `dustUpdate=0x1f` and `hitPublic=0xf`, proving
  runtime dust decrement/spawn/reset and hitstun/public-knockback transfer
  through the public imported-original seams.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage ColAnim, Screen Flash, and Public Reaction Wrappers Through Imported ftcommondamage.c

- Added narrow public wrappers for `ftCommonDamageCheckElementSetColAnim`,
  `ftCommonDamageCheckMakeScreenFlash`, and `ftCommonDamageSetPublic` that
  delegate to the imported original BattleShip `ndsBase*` functions.
- Added the public declarations to `include/ft/fighter.h` and changed the
  existing colanim, screen-flash, and public-reaction probes to call the public
  seams instead of the private imported helpers directly.
- Kept the markers stable at `colAnim=0x3f`, `flash=0x7f`, and
  `public=0x3f`, proving the existing routes now pass through public
  imported-original seams.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Level and Knockback Angle Wrappers Through Imported ftcommondamage.c

- Added narrow public wrappers for `ftCommonDamageGetDamageLevel` and
  `ftCommonDamageGetKnockbackAngle` that delegate to the imported original
  BattleShip `ndsBase*` functions.
- Added the public declarations to `include/ft/fighter.h` and changed the
  existing damage-level and knockback-angle probes to call the public seams
  instead of the private imported helpers directly.
- Kept the markers stable at `level=0x1f` and `angle=0x3f`, proving the
  existing threshold and Sakurai-angle routes now pass through public
  imported-original seams.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Dust Interval Wrapper Through Imported ftcommondamage.c

- Added a narrow public wrapper for `ftCommonDamageSetDustEffectInterval` that
  delegates to imported original
  `ndsBaseFTCommonDamageSetDustEffectInterval`.
- Added the public declaration to `include/ft/fighter.h` and changed the
  existing dust-threshold probe to call the public seam instead of the private
  imported helper directly.
- Kept the marker stable at `dust=0xff`, proving the low, mid-low, mid,
  mid-high, high, and default-air buckets now pass through the public
  imported-original seam.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Catch-Resist Update Wrapper Through Imported ftcommondamage.c

- Added a narrow public wrapper for `ftCommonDamageUpdateCatchResist` that
  delegates to imported original
  `ndsBaseFTCommonDamageUpdateCatchResist`.
- Added the public declaration to `include/ft/fighter.h` and changed the
  existing catch-resist update probe to call the public seam instead of the
  private imported helper directly.
- Kept the marker stable at `catchResist=0x1f`, proving the existing branch
  coverage now passes through the public imported-original seam.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage Hold/Resist Gate Wrappers Through Imported ftcommondamage.c

- Added narrow public wrappers for `ftCommonDamageCheckCatchResist` and
  `ftCommonDamageCheckCaptureKeepHold` that delegate to imported original
  `ndsBaseFTCommonDamageCheckCatchResist` /
  `ndsBaseFTCommonDamageCheckCaptureKeepHold`.
- Removed the proof-local mirror helpers and changed the existing
  `DASH_RUN_DAMAGE_HOLD_RESIST` probe to call the public seams.
- Kept the marker stable at `hold=0xff`, proving the existing hold/resist
  gate coverage now passes through public imported-original seams.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed First Damage UpdateMain Branch Through Imported ftcommondamage.c

- Added a narrow public `ftCommonDamageUpdateMain` wrapper that delegates to
  imported original `ndsBaseFTCommonDamageUpdateMain`.
- Changed the existing first catch/keep-hold `DASH_RUN_PROCPARAMS` branch
  proof to call the public dispatcher seam instead of mirroring that branch
  locally.
- Left the rest of the `ftCommonDamageUpdateMain` branch probes source-shaped
  for now; full damage initializer routing remains deferred.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Damage UpdateMain Zero-Knockback Catch Branch Through Imported ftcommondamage.c

- Changed the existing zero-knockback catch `DASH_RUN_PROCPARAMS` proof to call
  public `ftCommonDamageUpdateMain`.
- The branch stays bounded to the imported dispatcher into
  `ftCommonDamageUpdateCatchResist`; it does not enter full damage status
  setup.
- The aggregate marker remains stable.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed No-Status Damage UpdateMain Branch Family Through Imported ftcommondamage.c

- Expanded the public `ftCommonDamageUpdateMain` dispatcher proof from the
  first catch/zero-catch branches to the bounded no-status branch family:
  catch/capture keep-hold, catch/capture zero-knockback, no-grab/no-capture
  tail colanim, and held-item bypass/resist.
- Preserved the existing aggregate markers, including `itemBypass=0x1f`,
  `catchResist=0x1f`, `colAnimUpdate=0x1f`, and `sleep=0x7f`.
- Narrowed back from a broader status-changing dispatcher attempt after it
  crashed; throw/release/drop/status branches remain source-shaped until they
  can be migrated as a separate verified slice.
- Verified serially:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Routed Status-Changing Damage UpdateMain Branches Through Imported ftcommondamage.c

- Replaced the remaining local `DASH_RUN_PROCPARAMS`
  `ftCommonDamageUpdateMain` branch mirrors with calls through the public
  imported-original dispatcher.
- Added bounded dash-run damage-setup access to the imported
  `ftCommonThrownUpdateDamageStats`,
  `ftCommonThrownSetStatusDamageRelease`, and
  `ftCommonThrownSetStatusNoDamageRelease` helpers, so catch/capture
  stats/release/no-damage and heavy-item drop branches reach original helper
  code.
- Left `ftCommonThrownDecideFighterLoseGrip` / release map collision stubbed
  outside the older isolated throw-dead proof; natural lose-grip collision is
  still a separate boundary.
- Verified:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Bounded Lose-Grip Link Clear to Damage UpdateMain Seam

- Tightened the public `ftCommonThrownDecideFighterLoseGrip` seam during the
  dash-run damage setup proof: it now clears the catcher `catch_gobj` and
  interact `capture_gobj` links in source order instead of being a total
  no-op for this branch family.
- Tried enabling the imported original lose-grip release collision helper for
  this boundary; it faults before the live-hit finalizer, so full map-collision
  release stays deferred to a separate boundary.
- Verified after narrowing back:
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-06-30 - Added Bounded Lose-Grip Release Collision Slice

- Extended the same public lose-grip seam from link clear to a bounded
  source-order release slice: select the original release side, project through
  `mpCommonRunFighterCollisionDefault`, trigger invalid-floor
  `mpCommonSetFighterAir`, then clear catch/capture links.
- Added the compact `DASH_RUN_DAMAGE_LOSEGRIP` marker and required it in the
  current direct/menu-chain live-hit damage boundary as `loseGrip=0x3b/6/6`.
- Full imported lose-grip runtime is still guarded; this only proves the next
  collision/set-air slice needed by the existing damage dispatcher boundary.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`
  and
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  plus `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-06-30 - Routed Lose-Grip Release Through Imported ftcommonthrown2.c

- Reused the existing live-hit damage boundary and routed complete seeded
  catch/capture lose-grip links through imported original
  `ftCommonThrownDecideFighterLoseGrip` /
  `ftCommonThrownReleaseFighterLoseGrip`.
- Kept a bounded local fallback for incomplete proof seeds, while shared
  collision/air seams record the original helper path.
- Updated the compact marker to require the original-routing bit:
  `loseGrip=0x7b/6/6`.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`
  and
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-07-01 - Routed Selected Damage Init Setup Through Imported ftcommondamage.c

- Kept the current live-hit damage boundary and routed the selected
  high-knockback `ftCommonDamageInitDamageVars` setup call through imported
  BattleShip `ftcommondamage.c`.
- Preserved project-owned diagnostics around the imported call by recording
  the original color-animation, screenflash, attacker-stat, and
  `FTStruct.damage_kind` side effects without broadening the damage runtime.
- Updated the current marker requirement from `dmgKind=0x7` to `dmgKind=0xf`;
  the direct and menu-chain modes still report `damageSetup=0xffffffff`.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-07-01 - Routed Damage Status Entry Through Imported ftcommondamage.c

- Reused the current live-hit damage boundary and routed the public
  `ftCommonDamageGotoDamageStatus` seam through imported BattleShip
  `ftcommondamage.c` during the bounded dash-run damage setup proof.
- Normalized imported callback pointers back to the project-owned public
  wrappers after the imported call, keeping later DS diagnostics on the public
  seam instead of duplicating the original Goto body.
- Updated the current marker requirement from `dmgKind=0xf` to
  `dmgKind=0x1f`; direct/menu-chain still report `damageSetup=0xffffffff`.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-07-01 - Added Bounded ftMainUpdateDamageStatFighter Front Half

- Reused the current live-hit damage boundary and moved the selected slot-3
  normal-hit front half into a bounded source-shaped
  `ftMainUpdateDamageStatFighter` slice.
- The slice now owns the source-order attack-record, captured-damage,
  queue/lag, first `FTHitLog`, player-stat/stale-queue, and hit-SFX table
  handling before the existing percent-damage and hitlag scheduling proof.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`
  and
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-07-01 - Centralized Selected ftmain Stat Slices

- Reused the current live-hit damage boundary and moved the selected
  attack-clash/rebound, shield-stat -> GuardSetOff, and catch distance/search
  probes behind bounded source-shaped `ftmain.c` compatibility slices:
  `ftMainSetHitRebound`, `ftMainUpdateAttackStatFighter`,
  `ftMainUpdateShieldStatFighter`, and `ftMainUpdateCatchStatFighter`.
- Kept these as selected shared seams only; full unbounded `ftmain.c`
  collision/stat runtime remains deferred.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-07-01 - Added Bounded ftMainProcessHitCollisionStatsMain Fighter Branch

- Reused the current live-hit damage boundary and added a bounded
  fighter-hitlog branch of `ftMainProcessHitCollisionStatsMain`.
- Routed the selected slot-3 hurtbox damage consume proof and the older
  Dash-Run hit-stats handoff through the shared seam instead of hand-setting
  damage angle, element, LR, player/object attribution, joint/index, and
  knockback fields at each proof site.
- Kept weapon, item, and ground hitlog branches deferred; this is only the
  selected fighter-vs-fighter path needed by the current Mario/Fox boundary.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-07-01 - Routed Selected Slot-3 Damage Through ftMainSearchHitFighter

- Added a bounded fighter-only `ftMainSearchHitFighter` branch for the current
  Mario/Fox live-hit proof and routed the selected slot-3 damage consume path
  through it before the existing damage-stat and hit-collision-stat seams.
- Kept weapon/item/ground search branches deferred; the current proof seeds the
  selected fighter link and hitbox contact, calls the range shim, and preserves
  the existing `hbdmg=0->4/6` public summary while requiring mask bit `0x100`.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`
  and
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-07-01 - Routed Selected Shield Stat Through ftMainSearchHitFighter

- Extended the bounded fighter-only `ftMainSearchHitFighter` branch with the
  selected shield-contact/stat path from original `ftmain.c` source order.
- Routed the existing shield proof through the shared search hub before
  `ftMainUpdateShieldStatFighter`; the public `shield=4->4/4` and
  `shc=0x3fffff/3142` markers stayed stable.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-07-01 - Expanded Bounded ftmain Search Slice

- Routed the selected attack-clash proof through `ftMainSearchHitFighter`
  using the original self-before-other fighter-link ordering.
- Added bounded `ftMainSearchFighterCatch` coverage for the selected
  catch-search proof and kept `ftMainProcSearchCatch`, hazards, weapons, items,
  ground hitlog search, and full unbounded `ftmain.c` deferred.
- Public `clash=0x3f/24/18`, `catchSearch=0x3ff/s3`,
  `shield=4->4/4`, `shc=0x3fffff/3142`, and `hbdmg=0->4/6`
  markers stayed stable.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-07-01 - Routed Selected Damage Through ftMainProcSearchHitAll

- Added bounded `ftMainProcSearchHitAll` coverage for the selected slot-3
  hurtbox damage proof. The dispatcher clears hitlogs, calls the bounded
  fighter search, records item/weapon/ground search deferrals, and runs the
  fighter hitlog stats handoff when a hitlog exists.
- Kept real item, weapon, ground, hazard, and full `ftmain.c` search runtime
  deferred; the public live-hit marker stayed stable.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -DelaySeconds 3`.

## 2026-07-01 - Routed Selected Damage Tails Through ftMainProcParams

- Added a bounded project-owned `ftMainProcParams` dispatcher slice and routed
  the selected slot-3 hurtbox damage consume proof plus the Dash-Run proc-params
  aggregate through it.
- Covered the selected damage queue/percent/status/shuffle/rumble/hitlag/
  lagstart/transient-clear path, attacker attack-damage/BatSwing4 rumble,
  attack-shield-push, shield damage, shield break, reflect, and absorb branches.
- Kept Boss, Twister/trap, full rebound promotion, and unbounded `ftmain.c`
  runtime deferred.
- Trimmed the title scratch buffer cap from `180000` to `176000` bytes to keep
  the menu-chain ARM9 image under the limit while keeping the fighter scratch
  copy static and off the runtime stack.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Added Selected ftMainProcParams Rebound Promotion

- Extended bounded project-owned `ftMainProcParams` with the original
  `attack_shield_push` + `attack_rebound` branch. The proof calls imported
  original `ndsBaseFTCommonReboundWaitSetStatus` through the existing rebound
  status gate and restores the older passive-loop rebound counters afterward.
- Folded the proof into `DASH_RUN_PROCPARAMS_RUMBLE`: low bits still prove
  normal/BatSwing4 rumble, while high bits now expose `procRebound=0x1f` for
  ReboundWait status/callbacks/vector/hitlag/clear. The public aggregate is
  now `procRumble=0x7f` / `procRebound=0x1f`.
- Added scoped mode-160 `-Os` because the full menu-chain latest-boundary ROM
  is ARM9-size-tight. Normal/direct runtime keeps the existing optimization.
- Still deferred: natural continuous rebound gameplay, broad Rebound status
  routing, Boss/Twister/trap paths, and full unbounded `ftmain.c`.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`
  and
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`.

## 2026-07-01 - Added Selected ftMainProcParams Twister and proc_trap Branch

- Extended the bounded project-owned `ftMainProcParams` slice with the
  original BattleShip Twister damage-kind override and installed `proc_trap`
  callback call, keeping Boss and natural continuous trap gameplay deferred.
- Added the missing local `FTStruct.proc_trap` compatibility field and the
  original Twister status name, plus proof-local save/reset plumbing for the
  new callback field.
- Folded the proof into `DASH_RUN_DAMAGE_KIND`; the public aggregate now
  reports `dmgKind=0x7f`, preserving the existing imported
  `ftCommonDamageInitDamageVars` / `ftCommonDamageGotoDamageStatus` routing
  and damage-kind restore bits while proving Twister colanim routing and
  `proc_trap` source order.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Promoted Selected Live-Hit Status Loop Boundary

- Added direct/menu-chain modes `161/162`:
  `battle_mariofox_stage_mplivehit_status_loop` and
  `menu_chain_mariofox_stage_mplivehit_status_loop`.
- Routed the new pair through the same bounded taskman update/finalize ladder
  as the live-hit damage loop, kept modes `159/160` as regression coverage, and
  added scoped `-Os` for the size-tight live-hit boundary ROMs.
- Inherited the selected Fox Jab2 damage-loop proof and added bounded
  source-shaped damage-status follow-through: status `17->52/45`, hitlag
  `6->0`, callbacks `1/6/1`, search mask `0xf`, and repeat gate
  `1/1 gate=0x3f`.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-battle-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3 -NoBuild`,
  `verify-menu-chain-mariofox-stage-mplivehit-damage-loop-harness.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Added Selected Live-Hit Damage Update Tick

- Strengthened modes `161/162` by driving the selected post-hitlag damage
  status through the installed imported-original common damage update callback
  once after `ftCommonDamageSetStatus`.
- The status-loop verifier mask is now `0x7ff`, and the public summary adds
  `update=2->1`, proving hitstun decrements through the original callback with
  proof-local fighter/animation state restored.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -Build -DelaySeconds 3`,
  and `verify-dev-fast.ps1 -DelaySeconds 3`.

## 2026-07-01 - Added Selected Live-Hit Damage Expiry Tick

- Extended the modes `161/162` status-loop callback probe with a second
  installed imported-original damage update tick after `ftCommonDamageSetStatus`.
- The selected `DamageFlyN` status now proves hitstun `2->1`, public knockback
  release, and the bounded air/fly expiry into DamageFall `57/50`; the public
  summary now adds `finish=57/50`.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -Build -DelaySeconds 3`,
  and `verify-dev-fast.ps1 -DelaySeconds 3`.

## 2026-07-01 - Added Selected Live-Hit Damage Physics Tick

- Strengthened modes `161/162` by driving the selected post-hitlag damage
  status through the installed imported-original damage physics callback after
  `ftCommonDamageSetStatus`.
- The callback proof now seeds deterministic air attributes and `DamageFlyN`
  velocity, then proves the original hitstun air-physics branch applies air
  friction and gravity as `phys=11500/-1000` while restoring proof-local state.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -Build -DelaySeconds 3`,
  and `verify-dev-fast.ps1 -DelaySeconds 3`.

## 2026-07-01 - Added Selected Live-Hit Damage Interrupt Tick

- Strengthened modes `161/162` by driving the selected post-hitlag damage
  status through the installed imported-original damage interrupt callback
  after `ftCommonDamageSetStatus`.
- The callback proof now forces hitstun clear, ticks the installed AirCommon
  interrupt slot, records the guarded DamageFall interrupt handoff as
  `interrupt=1`, and restores proof-local fighter/guard state before the
  existing expiry proof.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-current.ps1 -Build -DelaySeconds 3`,
  and `verify-dev-fast.ps1 -DelaySeconds 3`.

## 2026-07-01 - Added Selected Live-Hit Damage Map Tick

- Strengthened modes `161/162` by ticking the installed imported-original
  AirCommon damage map callback after `ftCommonDamageSetStatus`.
- The selected `DamageFlyN` callback proof now records `map=1/1`, proving the
  installed map slot reaches the original no-collision path and then restores
  proof-local fighter/map guard state before the existing expiry proof.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Added Selected Live-Hit Damage Map Floor Branch

- Strengthened modes `161/162` by reusing the installed AirCommon damage map
  callback proof to drive the original floor-collision branch after
  `ftCommonDamageSetStatus`.
- The selected `DamageFlyN` callback proof now records
  `map=1/1 floor=1/1/1/1`, proving the original collision, PassiveStand check,
  Passive check, and DownBounce handoff path while restoring proof-local
  fighter/map state before expiry.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Added Selected Live-Hit Damage Map Wall Branch

- Strengthened modes `161/162` by reusing the installed AirCommon damage map
  callback proof to drive the original wall-collision short-circuit after
  `ftCommonDamageSetStatus`.
- The selected `DamageFlyN` callback proof now records
  `map=1/1 floor=1/1/1/1 wall=0x3f`, proving the original WallDamage helper,
  passive/DownBounce short-circuit, reflected knockback, and restore path.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Added Selected Live-Hit Damage Map Ceiling Branch

- Strengthened modes `161/162` by extending the same installed AirCommon
  damage map callback proof through the original `MAP_FLAG_CEIL` branch in
  `ftCommonWallDamageCheckGoto`.
- The selected `DamageFlyN` callback proof now records
  `map=1/1 floor=1/1/1/1 wall=0xfff`; low six bits prove the side-wall
  WallDamage helper/short-circuit/knockback/restore path, and high six bits
  prove the same original WallDamage route for ceiling collision.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Added Selected Live-Hit Damage Map Right-Wall Branch

- Strengthened modes `161/162` by extending the installed AirCommon damage map
  callback proof through the original `MAP_FLAG_RWALL` branch in
  `ftCommonWallDamageCheckGoto`, completing the original left-wall,
  right-wall, and ceiling WallDamage branch set.
- The selected `DamageFlyN` callback proof now records
  `map=1/1 floor=1/1/1/1 wall=0x3ffff`; low six bits prove left-wall,
  middle six bits prove ceiling, and high six bits prove right-wall helper,
  short-circuit, knockback, and restore.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Added Selected Live-Hit DamageFall Map Cliff-Catch Branch

- Strengthened modes `161/162` by running the selected post-expiry
  DamageFall map callback through the original `ftCommonDamageFallProcMap`
  cliff branch after the AirCommon damage callback proof.
- The selected `DamageFlyN` callback proof now records
  `map=1/1 floor=1/1/1/1 wall=0x3ffff cliff=0x1f`; `cliff=0x1f`
  proves the DamageFall map tick, cliff collision mask, CliffCatch seam,
  passive/DownBounce skip, preserved DamageFall state, and source route.
- Verified:
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `verify-dev-fast.ps1 -DelaySeconds 3`,
  and `verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Added Selected Live-Hit DamageFall Map Floor Fallback

- Strengthened modes `161/162` by reusing the selected post-expiry
  DamageFall map callback proof to run original `ftCommonDamageFallProcMap`
  through no-collision and floor-fallback paths after the cliff branch.
- The selected callback marker now records
  `map=1/1 floor=1/1/1/1 wall=0x3ffff cliff=0x1f fallmap=0x7f`;
  `fallmap=0x7f` proves DamageFall no-collision, floor collision,
  PassiveStand/Passive checks, DownBounce handoff, preserved DamageFall state,
  and source route while restoring proof-local state.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Added Common Damage Fall Interrupt Callback Proof

- Strengthened `DASH_RUN_DAMAGE_COMMON_CALLBACKS` from `0x1fff` to `0x3fff`
  by driving imported-original `ftCommonDamageCommonProcInterrupt` through its
  air/no-hammer branch into `ftCommonFallProcInterrupt`.
- The proof uses the existing Fall interrupt wrapper only as a counter seam,
  restores local fighter/counter state, and keeps the earlier ground/air
  update stay/expiry, common ground/hammer, and AirCommon interrupt coverage.
- No broad `ftmain.c`, `gmcollision.c`, continuous damage runtime, item,
  weapon, or audio runtime import was added.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`,
  `.\scripts\check-docs.ps1`,
  `.\scripts\check-architecture.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\clean-generated.ps1 -DryRun`, and
  `git diff --check`.

## 2026-07-01 - Routed DamageFall Expiry Through Imported Original Helper

- Strengthened the existing Dash-Run hitstun-expiry DamageFall handoff without
  changing the marker surface. The public `ftCommonDamageFallSetStatusFromDamage`
  wrapper now calls imported-original `ndsBaseFTCommonDamageFallSetStatusFromDamage`
  instead of hand-installing the DamageFall status shell.
- Added a proof-gated `ftMainSetStatus` DamageFall install for this expiry path
  and required the imported `ftCommonDamageFallClampRumble` tail to reach the
  public rumble seam before the existing expiry success counter increments.
- `DASH_RUN_DAMAGE_SETUP=0xffffffff` and the selected live-hit `finish=57/50`
  markers remain stable, but the expiry bit now proves the source-routed
  status helper plus clamp/rumble tail. Continuous DamageFall runtime and
  full unbounded `ftmain.c` remain deferred.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Routed WallDamage DamageFall Handoff Through Imported Original Helper

- Strengthened the standalone passive/recover WallDamage update proof without
  changing the marker surface. Imported-original `ftCommonWallDamageProcUpdate`
  now reaches imported-original `ndsBaseFTCommonDamageFallSetStatusFromDamage`
  instead of a local DamageFall shell.
- Added the matching proof-gated `ftMainSetStatus` DamageFall install and a
  private clamp-rumble counter for this WallDamage path. The public WallDamage
  rumble marker stays scoped to original setup rumble ID `2`, while the
  handoff count requires the imported clamp/rumble tail.
- `wallDamage=56/49->57/50` remains stable. Continuous WallDamage/DamageFall
  runtime and full unbounded `ftmain.c` remain deferred.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`,
  `.\scripts\check-docs.ps1`,
  `.\scripts\check-architecture.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\clean-generated.ps1 -DryRun`, and
  `git diff --check`.

## 2026-07-01 - Routed CliffWait DamageFall Handoff Through Imported Original Helper

- Strengthened the standalone CliffWait damage proof without changing the
  marker surface. Public `ftCommonDamageFallSetStatusFromCliffWait` now calls
  imported-original `ndsBaseFTCommonDamageFallSetStatusFromCliffWait` instead
  of hand-installing the DamageFall shell.
- The handoff count now requires the bounded `ftMainSetStatus` DamageFall
  install and imported clamp/rumble tail. The wrapper preserves the original
  Z-trigger timer tail, and the bounded PassiveStand/Passive floor branches
  still give imported-original checks first chance before using the existing
  proof-local original setter sequence if the DS compatibility shell needs it.
- `mpCliffWaitDamage=status=85->57 motion=73->50 fallWait=1->0 catch=30`,
  `passiveStand=73/62/0->cb1/1->10/4/0`, and
  `passive=81/70/0->cb1/1->10/4/0` remain stable. Continuous CliffWait,
  DamageFall, PassiveStand, Passive, and DownBounce gameplay remain deferred.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Routed DownBounce Effect Tail Through Imported Original Helper

- Strengthened the standalone CliffWait damage DownBounce setup proof without
  changing the marker surface. The bounded `ftCommonDownBounceSetStatus`
  branch now calls imported-original `ndsBaseFTCommonDownBounceUpdateEffects`
  for the ImpactWave/FGM/rumble tail instead of hand-counting those side
  effects.
- Kept the selected DownBounceU/Ground status install in the existing bounded
  DS proof shell, then cleared `attack_buffer`, set `damage_mul=0.5`, and ran
  the velocity-transfer seam in the original order. Full imported
  `ftCommonDownBounceSetStatus` remains deferred until the DS proof shell can
  hand over orientation/status selection safely.
- `mpCliffWaitDamage=status=85->57 motion=73->50 fallWait=1->0 catch=30` and
  `downBounce=68/59/0 dbuf=60` remain stable.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`,
  `.\scripts\check-docs.ps1`,
  `.\scripts\check-architecture.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\clean-generated.ps1 -DryRun`, and
  `git diff --check`.

## 2026-07-01 - Routed DownBounce Orientation Through Original Formula

- Strengthened the same CliffWait damage DownBounce setup proof without
  changing the marker surface. The bounded `ftCommonDownBounceSetStatus`
  branch now calls public `ftCommonDownBounceCheckUpOrDown`, whose DS-safe
  wrapper uses BattleShip's original rotation normalization and D/U predicate
  before choosing the DownBounce status.
- Kept the existing bounded status install and imported-original
  `ndsBaseFTCommonDownBounceUpdateEffects` tail. The selected proof seed still
  resolves to DownBounceU/Ground `68/59/0`; full imported
  `ftCommonDownBounceSetStatus` remains deferred because the unchecked imported
  orientation helper is not safe on this DS proof seed.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`,
  `.\scripts\check-docs.ps1`,
  `.\scripts\check-architecture.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\clean-generated.ps1 -DryRun`, and
  `git diff --check`.

## 2026-07-01 - Restored ftParamStopVoiceRunProcDamage Callback Tail

- Restored the source-backed `proc_damage` call in public
  `ftParamStopVoiceRunProcDamage`, matching BattleShip's helper after the
  existing DS capture voice-stop marker path. The current compatibility
  `FTStruct` still does not expose BattleShip's `p_voice` / `voice_id` fields,
  so the voice-object stop itself remains deferred.
- Extended the inherited Passive recover CapturePulled proof with a bounded
  victim `proc_damage` callback. `STAGE_MPPASSIVE_CAPTURE` now records the
  callback count and the aggregate summary prints
  `capture=171/150->172/-2 procDamage=1`.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`,
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`,
  `.\scripts\check-docs.ps1`,
  `.\scripts\check-architecture.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\clean-generated.ps1 -DryRun`, and
  `git diff --check`.

## 2026-07-01 - Proved AttackAir Map Landing Handoff

- Strengthened the standalone Jump-loop AttackAirN proof by routing the
  selected landing case through BattleShip's original
  `ftCommonAttackAirProcMap` landing branch and imported-original
  `ftCommonLandingAirSetStatus`.
- Aligned the local compatibility status enum with BattleShip so
  `nFTCommonStatusLandingAirEnd` aliases `nFTCommonStatusLandingAirNull`.
  The bounded proof still accepts only LandingAirN/LandingAirNull while the
  map-landing probe is active.
- Seeded a proof-local `FTData.mainmotion` table only because the standalone
  proof fighter has no `fp->data`; the probe restores the original fighter,
  DObj, callback, status, motion, and velocity fields immediately after the
  call.
- `JUMP_ATTACKAIR` now asserts the map-landing diagnostic mask `map=0x1f`:
  landing detected, imported landing setter reached, bounded landing status
  installed, LandingAirN/LandingAirNull observed, and the Ground landing
  callback shell installed.
- Verified:
  `.\scripts\verify-battle-mariofox-jump-loop-harness.ps1 -DelaySeconds 3`
  (`attackAir=209/184 map=0x1f`),
  `.\scripts\verify-menu-chain-mariofox-jump-loop-harness.ps1 -DelaySeconds 3`
  (`attackAir=209/184 map=0x1f`),
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`,
  `.\scripts\verify-current.ps1 -DelaySeconds 3`,
  `.\scripts\check-docs.ps1`,
  `.\scripts\check-architecture.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\clean-generated.ps1 -DryRun`, and
  `git diff --check`.

## 2026-07-01 - Proved Directional AttackAir Selector Handoff

- Extended the same standalone Jump-loop `JUMP_ATTACKAIR` proof without
  changing the neutral AttackAirN counters. A proof-only restored direction
  probe now calls imported-original `ftCommonAttackAirCheckInterruptCommon`
  for AttackAirF/B/Hi/Lw.
- Widened the bounded Jump-loop `ftMainSetStatus` shell only while that
  direction probe is active. It installs the source-shaped status/motion and
  motion/status/stat attack IDs for the four directional aerials, including
  imported-original Link down-air proc-hit/rehit callback setup for Lw.
- `JUMP_ATTACKAIR` now asserts `map=0x1f dir=0x1f`; the direction mask proves
  F/B/Hi/Lw selector status installs plus the AttackAirLw callback setup.
- Verified:
  `.\scripts\verify-battle-mariofox-jump-loop-harness.ps1 -DelaySeconds 3`
  (`attackAir=209/184 map=0x1f dir=0x1f`),
  `.\scripts\verify-menu-chain-mariofox-jump-loop-harness.ps1 -DelaySeconds 3`
  (`attackAir=209/184 map=0x1f dir=0x1f`),
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved AttackAir Map Branch Handoffs

- Extended the same standalone Jump-loop `JUMP_ATTACKAIR` proof through the
  remaining bounded BattleShip `ftCommonAttackAirProcMap` landing branches.
- The map proof now covers smooth LandingAirN, missing-animation
  LandingAirNull, skip-landing Wait, and plain LandingLight handoffs through
  proof-local restored seeds while keeping the neutral AttackAirN and
  directional selector counters stable.
- `JUMP_ATTACKAIR` now asserts `map=0x3ff dir=0x1f`.
- Verified:
  `.\scripts\verify-battle-mariofox-jump-loop-harness.ps1 -DelaySeconds 3`
  (`attackAir=209/184 map=0x3ff dir=0x1f`),
  `.\scripts\verify-menu-chain-mariofox-jump-loop-harness.ps1 -DelaySeconds 3`
  (`attackAir=209/184 map=0x3ff dir=0x1f`),
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit DamageFall DownBounce Status Handoff

- Extended modes `161/162` by reusing the selected post-expiry DamageFall map
  callback proof with a restored floor-collision seed that reaches imported
  original `ftCommonDownBounceSetStatus`.
- Added a proof-local DownBounce `ftMainSetStatus` install guard so the
  imported setter can run source order while the wider damage-map proof keeps
  its count-only branch and restores the DamageFall seed afterward.
- The selected callback marker now records
  `map=1/1 floor=1/1/1/1 wall=0x3ffff cliff=0x1f fallmap=0xff`; the new
  `fallmap` bit proves imported DownBounce status/motion, Ground state,
  callback slots, attack-buffer clear, and `damage_mul=0.5`.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit DamageFall CliffCatch Status Handoff

- Extended modes `161/162` by reusing the selected post-expiry DamageFall map
  callback proof with a restored cliff-collision seed that reaches imported
  original `ftCommonCliffCatchSetStatus`.
- Added a proof-local CliffCatch `ftMainSetStatus` install guard so the
  imported setter can run source order while the wider damage-map proof keeps
  its count-only branch and restores the DamageFall seed afterward.
- The selected callback marker now records
  `map=1/1 floor=1/1/1/1 wall=0x3ffff cliff=0x3f fallmap=0xff`; the new
  `cliff` bit proves imported CliffCatch status/motion, Air state,
  floor-line clear, cliff hold, callback slots, capture immunity, damage
  callback, and velocity stop.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Natural Slot-0 Live-Hit Hurtbox Consume

- Extended modes `161/162` inside the existing selected live-hit hurtbox
  damage consume proof. After the selected slot-3 pass records the public
  `hurt=3/10 hbdmg=0->4/6` diagnostic, the proof restores both fighter
  structs and reruns unmasked source-order `ftMainProcSearchHitAll` against
  Mario damage-coll slot `0`.
- Tightened `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` from mask low bits `0x1ff` to
  `0x3ff`; the new bit proves natural slot-0 record/hitlog/queue/percent/
  hitlag consume without the proof-local slot `0` / `1` intangible setup used
  by the selected slot-3 diagnostic.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Natural Slot-0 Live-Hit Repeat Suppression

- Extended modes `161/162` inside the existing selected live-hit hurtbox
  damage consume proof. After the restored natural slot-0 consume, the proof
  reruns unmasked source-order `ftMainProcSearchHitAll` without clearing the
  attack record.
- Tightened `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` from mask low bits `0x3ff` to
  `0x7ff`; the new bit proves the same-victim attack record rejects the
  immediate repeat search with no new hitlog, damage queue growth, or percent
  growth.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Shield-Contact Repeat Suppression

- Extended modes `161/162` inside the existing selected live-hit shield-contact
  proof. After the first selected shield contact records the attack record, the
  proof reruns source-order `ftMainSearchHitFighter` without clearing that
  record.
- Tightened `STAGE_MPLIVEHIT_SHIELD_CONTACT` from mask low bits `0x3fffff` to
  `0x7fffff`; the new bit proves the same-victim shield repeat is rejected
  with no new shield collision, stat, effect, or shield-damage changes.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Catch-Search Repeat Suppression

- Extended modes `161/162` inside the existing selected live-hit catch-search
  proof. After the first source-order `ftMainSearchFighterCatch` records the
  selected target through `ftMainUpdateCatchStatFighter`, the proof reruns the
  same search without clearing that attack record.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0x3ff` to
  `0x7ff`; the new bit proves the same-victim catch repeat is rejected with no
  new closest target/distance update.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Natural Slot-0 Catch Search

- Extended modes `161/162` inside the selected live-hit catch-search proof.
  The new bit restores saved Mario/Fox fighter structs, clears the catch attack
  record, positions the selected catch hitbox on Mario's natural damage-coll
  slot `0`, and reruns source-order `ftMainSearchFighterCatch` without the
  proof-local slot-3 masking.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0x7ff` to
  `0xfff`; the new bit proves the natural target, distance, and attack-record
  update before state restore.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Catch-Search Capture-Immune Gate

- Extended modes `161/162` inside the selected live-hit catch-search proof.
  The new bit restores Mario/Fox, seeds `capture_immune_mask & catch_mask`,
  reruns source-order `ftMainSearchFighterCatch`, and proves the target is
  rejected before search target/distance or attack-record update.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0xfff` to
  `0x1fff`; the public marker is now `catchSearch=0x1fff/s3`.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Catch-Search Team Gate

- Extended modes `161/162` inside the selected live-hit catch-search proof.
  The new bit restores Mario/Fox, seeds same-team fighters with team battle on
  and team attack off, reruns source-order `ftMainSearchFighterCatch`, and
  proves the target is rejected before search target/distance or attack-record
  update.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0x1fff` to
  `0x3fff`; the public marker is now `catchSearch=0x3fff/s3`.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Catch-Search Ghost and Boss Gates

- Extended modes `161/162` inside the selected live-hit catch-search proof.
  The new bits restore Mario/Fox, seed ghost and Boss targets, rerun
  source-order `ftMainSearchFighterCatch`, and prove each target is rejected
  before search target/distance or attack-record update.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0x3fff` to
  `0xffff`; the public marker is now `catchSearch=0xffff/s3`.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Catch-Search Target Hitstatus Gate

- Aligned the local `ftMainSearchFighterCatch` early rejection order with
  BattleShip's ghost, Boss, team, capture-immune, then hitstatus source order.
- Extended modes `161/162` inside the selected live-hit catch-search proof.
  The new bit restores Mario/Fox, seeds `special_hitstatus`, `star_hitstatus`,
  and `hitstatus` non-normal one at a time, reruns source-order
  `ftMainSearchFighterCatch`, and proves target rejection before search
  target/distance or attack-record update.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0xffff` to
  `0x1ffff`; the public marker is now `catchSearch=0x1ffff/s3`.
- Verified:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3`,
  `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-current.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Inner Catch-Search Source Order

- Extended modes `161/162` inside the selected live-hit catch-search proof.
  The new bits prove selected attack-state-off rejection, Ground/Air mismatch
  rejection, hurt/shield/group attack-record skips, the damage-coll
  `hitstatus == None` sentinel break, and valid grabbable no-collision
  no-update without changing selected slot `s3` or skip mask `0x3`.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0x1ffff` to
  `0xffffff`; the public marker is now `catchSearch=0xffffff/s3`.
- Verified:
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-01 - Routed Catch Search Through Proc Wrapper

- Restored BattleShip's `FTStruct.is_catchstatus` compatibility flag in the
  local fighter shell, setting it from `ftParamSetCatchParams` and clearing it
  on common status reset.
- Added bounded `ftMainProcSearchCatch` coverage around the selected
  catch-search proof. The new bits prove hazard wait-timer decrement before the
  catch-status gate, no search/callbacks while the gate is closed, and
  catch/capture callbacks after the selected target is found.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0xffffff` to
  `0x7ffffff`; the public marker is now `catchSearch=0x7ffffff/s3`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-01 - Proved Ground Obstacle Callback Iteration

- Restored the local two-entry `ftMainCheckAddGroundObstacle` /
  `ftMainClearGroundObstacle` registry shape from BattleShip and let
  `ftMainSearchHitHazard` iterate registered false-return obstacle callbacks
  after the existing wait-timer decrement.
- Extended the selected live-hit catch-search proof to cover add/full/clear
  ordering, false-return callback argument/order, and Twister/TaruCannon wait
  decrement while leaving true-return trap status dispatch deferred.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0x7ffffff` to
  `0x1fffffff`; the public marker is now `catchSearch=0x1fffffff/s3`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-01 - Proved Twister Hazard Dispatch

- Imported bounded original BattleShip `ftcommontwister.c` through
  `src/import/battleship_ftcommon_twister.c` and added the narrow
  compatibility declarations needed by its status setter.
- Routed true-return `nGMHitEnvironmentTwister` ground-obstacle callbacks
  through `ftMainSetHitHazard` into imported original
  `ftCommonTwisterSetStatus`. The selected live-hit catch-search proof now
  verifies Twister status/motion, imported update/physics callbacks,
  release-wait reset, tornado GObj capture, capture immunity, and wait-timer
  decrement before restoring state.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0x1fffffff` to
  `0x3fffffff`; the public marker is now `catchSearch=0x3fffffff/s3`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-01 - Proved Twister Callback Tick

- Extended the selected live-hit catch-search proof after the true-return
  Twister hazard dispatch to run one installed imported-original Twister
  update/physics callback tick.
- The new bit proves release-wait advance, bounded air-velocity update, and
  root Y-rotation before restoring the fighter and root state.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0x3fffffff` to
  `0x7fffffff`; the public marker is now `catchSearch=0x7fffffff/s3`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-01 - Proved Hazard Ghost Gate

- Extended the selected live-hit catch-search proof around
  `ftMainSearchHitHazard`.
- The new bit marks the fighter ghost and proves BattleShip's outer guard
  exits before wait-timer decrement or obstacle callbacks.
- Tightened `STAGE_MPLIVEHIT_CATCHSEARCH` from mask low bits `0x7fffffff` to
  `0xffffffff`; the public marker is now `catchSearch=0xffffffff/s3`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Search Ghost Gate

- Extended the selected live-hit damage-loop aggregate around
  `ftMainProcSearchHitAll`.
- The new bit marks the attacking fighter ghost and proves BattleShip's outer
  guard exits before hitlog clear, fighter/item/weapon/ground search, or
  hit-stat processing.
- Tightened `STAGE_MPLIVEHIT_DAMAGE` from mask low bits `0x7fffffff` to
  `0xffffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Damage Update No-Expiry Gate

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageAirCommonProcUpdate`.
- The new bit keeps animation active while hitstun reaches zero, proving
  public knockback release without the DamageFall expiry handoff.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x7ff` to `0xfff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Ground Damage Expiry

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcUpdate`.
- The new bit seeds ground `DamageN1`, runs the installed original update at
  animation end as hitstun reaches zero, and proves public-knockback transfer
  through original `mpCommonSetFighterWaitOrFall` into imported-original
  Wait/Ground before restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0xfff` to `0x1fff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Ground Damage Interrupt

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcInterrupt`.
- The new bit seeds ground `DamageN1`, runs the installed original interrupt
  with hitstun already zero, and proves it clears hitstun state and reaches
  imported-original Wait/Ground interrupt handling before restoring the
  live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x1fff` to `0x3fff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Air Damage Interrupt

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcInterrupt`.
- The new bit seeds `DamageN1` with Air kinetics, runs the installed original
  interrupt with hitstun already zero, and proves the no-hammer air branch
  reaches `ftCommonFallProcInterrupt` before restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x3fff` to `0x7fff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Ground Hammer Interrupt

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcInterrupt`.
- The new bit seeds ground `DamageN1`, marks hammer held through the existing
  hammer-check seam, runs the installed original interrupt with hitstun already
  zero, and proves the hammer branch reaches `ftHammerProcInterrupt` before
  restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x7fff` to `0xffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Air Hammer Interrupt

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcInterrupt`.
- The new bit seeds `DamageN1` with Air kinetics, marks hammer held through
  the existing hammer-check seam, runs the installed original interrupt with
  hitstun already zero, and proves the air hammer branch reaches
  `ftCommonHammerFallProcInterrupt` before restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0xffff` to
  `0x1ffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Knockback-Over Status Set

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageSetStatus`.
- The new bit reruns the installed original status set with
  `is_knockback_over` set, proving the source branch clears that flag and sets
  timed hit-status invincibility before restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x1ffff` to
  `0x3ffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit DamageFlyRoll Status Pitch

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageSetStatus`.
- The new bit reruns the installed original status set with `DamageFlyRoll`,
  proving the source branch reaches `ftCommonDamageFlyRollUpdateModelPitch`
  and updates joint pitch before restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x3ffff` to
  `0x7ffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Throw-Clear Physics Tail

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcPhysics`.
- The new bit reruns the installed original physics callback with a low-speed
  throw-owned attack coll, proving the source tail clears attack collisions
  before restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x7ffff` to
  `0xfffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Smash DI Lag Update

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcLagUpdate`.
- The new bit reruns the installed original lag-update callback with hitlag,
  stick range, and tap-buffer state, proving the Smash DI branch moves the
  root and resets tap buffers before restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0xfffff` to
  `0x1fffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit DamageFlyRoll Physics Pitch

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcPhysics`.
- The new bit reruns the installed original physics callback as
  `DamageFlyRoll`, proving the source branch reaches
  `ftCommonDamageFlyRollUpdateModelPitch` and updates joint pitch before
  restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x1fffff` to
  `0x3fffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Ground Damage Physics

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcPhysics`.
- The new bit reruns the installed original physics callback with ground
  kinetics, proving the source ground-friction branch reduces ground velocity
  before restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x3fffff` to
  `0x7fffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Air Fastfall Physics

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcPhysics`.
- The new bit reruns the installed original physics callback with
  zero-hitstun Air kinetics, proving the source
  `ftPhysicsApplyAirVelDriftFastFall` branch sets fastfall state and terminal
  velocity before restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x7fffff` to
  `0xffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Air Drift Physics

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcPhysics`.
- The new bit reruns the installed original physics callback with
  zero-hitstun Air kinetics and horizontal stick input, proving the source
  air-drift branch updates X velocity after gravity before restoring the
  live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0xffffff` to
  `0x1ffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Air Clamp Physics

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcPhysics`.
- The new bit reruns the installed original physics callback with
  zero-hitstun Air kinetics and over-max horizontal velocity, proving the
  source air-velocity clamp-decrement branch before restoring the live-hit
  state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x1ffffff` to
  `0x3ffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Lag-Update No-Op Gates

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageCommonProcLagUpdate`.
- The new bit reruns the installed original lag-update callback through
  no-hitlag, below-threshold stick, and saturated tap-buffer no-op gates before
  restoring the live-hit state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x3ffffff` to
  `0x7ffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit DamageAir Passive Map Gates

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageAirCommonProcMap`.
- The new bit reruns the installed original map callback with floor collision,
  proving imported-original PassiveStand and Passive true-return
  short-circuits skip later map branches before restoring state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x7ffffff` to
  `0xfffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit DamageFall Passive Map Gates

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageFallProcMap`.
- The new bit reruns the installed original DamageFall map callback with floor
  collision, proving imported-original PassiveStand and Passive true-return
  short-circuits skip the DownBounce tail before restoring state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0xfffffff` to
  `0x1fffffff`; `fallmap` is now `0x3ff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit DamageAir No-Floor Map Gate

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageAirCommonProcMap`.
- The new bit reruns the installed original map callback with collision but
  no `MAP_FLAG_FLOOR`, proving the source floor-bit short-circuit skips
  PassiveStand, Passive, and DownBounce before restoring state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x1fffffff` to
  `0x3fffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit DamageFall No-Cliff Map Tail

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageFallProcMap`.
- The new bit reruns the installed original DamageFall map callback with
  collision but no `MAP_FLAG_CLIFF_MASK`, proving the source no-cliff tail
  runs PassiveStand, Passive, and DownBounce before restoring state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x3fffffff` to
  `0x7fffffff`; `fallmap` is now `0x7ff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit DamageFall Interrupt Source Order

- Extended the selected live-hit status-loop callback proof around installed
  original `ftCommonDamageFallProcInterrupt`.
- The new bit reruns the installed original DamageFall interrupt after expiry,
  proving BattleShip's source-order SpecialAir, AttackAir, JumpAerial, and
  HammerFall checks before restoring state.
- Tightened `STAGE_MPLIVEHIT_STATUS` and
  `STAGE_MPLIVEHIT_STATUS_CALLBACK` from mask low bits `0x7fffffff` to
  `0xffffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Natural Slot-1 Hurtbox Damage

- Extended the selected live-hit hurtbox-damage consume proof around
  BattleShip's source-order `ftMainProcSearchHitAll`.
- The new bit restores Mario/Fox after the natural slot-0 consume/repeat
  proof, marks Mario damage-coll slot `0` intangible, reruns the unmasked
  fighter-only search, and proves natural slot-1 hitlog, attack-record, queue,
  percent, and hitlag consumption before restoring state.
- Tightened `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` from mask low bits `0x7ff` to
  `0xfff`; the status-loop search marker now requires the inherited damage
  mask `0xfff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Natural Slot-2 Hurtbox Damage

- Extended the selected live-hit hurtbox-damage consume proof around the same
  BattleShip source-order `ftMainProcSearchHitAll` path.
- The new bit restores Mario/Fox after the natural slot-1 proof, marks Mario
  damage-coll slots `0` and `1` intangible, reruns the unmasked fighter-only
  search, and proves natural slot-2 hitlog, attack-record, queue, percent, and
  hitlag consumption before restoring state.
- Tightened `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` from mask low bits `0xfff` to
  `0x1fff`; the status-loop search marker now requires the inherited damage
  mask `0x1fff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Natural Slot-4 Hurtbox Damage

- Extended the selected live-hit hurtbox-damage consume proof around the same
  BattleShip source-order `ftMainProcSearchHitAll` path.
- The new bit restores Mario/Fox after the natural slot-2 proof, marks Mario
  damage-coll slots `0` through `3` intangible, reruns the unmasked fighter-only
  search, and proves natural slot-4 hitlog, attack-record, queue, percent, and
  hitlag consumption before restoring state.
- Tightened `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` from mask low bits `0x1fff` to
  `0x3fff`; the status-loop search marker now requires the inherited damage
  mask `0x3fff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Natural Slot-5 Hurtbox Damage

- Extended the selected live-hit hurtbox-damage consume proof around the same
  BattleShip source-order `ftMainProcSearchHitAll` path.
- The new bit restores Mario/Fox after the natural slot-4 proof, marks Mario
  damage-coll slots `0` through `4` intangible, reruns the unmasked fighter-only
  search, and proves natural slot-5 hitlog, attack-record, queue, percent, and
  hitlag consumption before restoring state.
- Tightened `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` from mask low bits `0x3fff` to
  `0x7fff`; the status-loop search marker now requires the inherited damage
  mask `0x7fff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Natural Slot-6 Hurtbox Damage

- Extended the selected live-hit hurtbox-damage consume proof around the same
  BattleShip source-order `ftMainProcSearchHitAll` path.
- The new bit restores Mario/Fox after the natural slot-5 proof, marks Mario
  damage-coll slots `0` through `5` intangible, reruns the unmasked
  fighter-only search, and proves natural slot-6 hitlog, attack-record, queue,
  percent, and hitlag consumption before restoring state.
- Tightened `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` from mask low bits `0x7fff` to
  `0xffff`; the status-loop search marker now requires the inherited damage
  mask `0xffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Natural Slot-7 Hurtbox Damage

- Extended the selected live-hit hurtbox-damage consume proof around the same
  BattleShip source-order `ftMainProcSearchHitAll` path.
- The new bit restores Mario/Fox after the natural slot-6 proof, marks Mario
  damage-coll slots `0` through `6` intangible, reruns the unmasked
  fighter-only search, and proves natural slot-7 hitlog, attack-record, queue,
  percent, and hitlag consumption before restoring state.
- Tightened `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` from mask low bits `0xffff` to
  `0x1ffff`; the status-loop search marker now requires the inherited damage
  mask `0x1ffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Natural Slot-8/9 Hurtbox Damage

- Extended the selected live-hit hurtbox-damage consume proof around the same
  BattleShip source-order `ftMainProcSearchHitAll` path.
- The new bits restore Mario/Fox after the natural slot-7 proof, mark earlier
  Mario damage-coll slots intangible, rerun the unmasked fighter-only search,
  and prove natural slot-8 and slot-9 hitlog, attack-record, queue, percent,
  and hitlag consumption before restoring state.
- Tightened `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` from mask low bits `0x1ffff` to
  `0x7ffff`; the status-loop search marker now requires the inherited damage
  mask `0x7ffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Catch-Search Invincible Slot Skip

- Extended the selected live-hit catch-search proof around BattleShip's
  source-order `ftMainSearchFighterCatch` loop.
- The new skip bit restores Mario/Fox, marks the selected damage-coll slot
  invincible, reruns the fighter catch search, and proves no search target,
  distance update, or attack record is written before restoring state.
- Tightened the `STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask from `0x3` to
  `0x7`; the public catch-search mask remains `0xffffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Catch-Search None/Miss Secondary Gates

- Extended the selected live-hit catch-search secondary skip proof around the
  same BattleShip source-order `ftMainSearchFighterCatch` loop.
- The new bits record the restored `None` sentinel break and valid
  no-collision no-update probes in the secondary skip mask before state
  restore.
- Tightened the `STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask from `0x7` to
  `0x1f`; the public catch-search mask remains `0xffffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Catch-Search Attack/Record Secondary Gates

- Extended the selected live-hit catch-search secondary skip proof around the
  same BattleShip source-order `ftMainSearchFighterCatch` loop.
- The new bits record restored attack-state-off, Ground/Air mismatch, and
  attack-record rejection probes in the secondary skip mask before state
  restore.
- Tightened the `STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask from `0x1f`
  to `0xff`; the public catch-search mask remains `0xffffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Catch-Search Target Secondary Gates

- Extended the selected live-hit catch-search secondary skip proof around the
  same BattleShip source-order `ftMainSearchFighterCatch` loop.
- The new bits record restored capture-immune, ghost, Boss, global hitstatus,
  and same-team target rejection probes in the secondary skip mask before state
  restore.
- Tightened the `STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask from `0xff`
  to `0x1fff`; the public catch-search mask remains `0xffffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit Catch-Search Self Secondary Gate

- Extended the selected live-hit catch-search secondary skip proof around the
  same BattleShip source-order `ftMainSearchFighterCatch` loop.
- The new bit restores Mario/Fox, runs the fighter catch search with attacker
  before target in the fighter list, and records self rejection plus
  next-fighter target catch before state restore.
- Tightened the `STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask from
  `0x1fff` to `0x3fff`; the public catch-search mask remains `0xffffffff`.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit TaruCannon Setup Dispatch

- Strengthened the selected live-hit catch-search hazard proof without changing
  the public `catchSearch=0xffffffff/s3` marker.
- Added bounded TaruCannon kind `3` dispatch through `ftMainSetHitHazard` into
  a source-ordered setup shell matching the original `ftCommonTaruCannSetStatus`
  entry sequence up through status `61`, script `-1`, TaruCannon status-vars
  reset, barrel GObj capture, capture immunity, invisible flag, and intangible
  hitstatus.
- Continuous TaruCannon update/shoot runtime remains deferred until the Jungle
  barrel helpers and map throw-hit data are local.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Proved Live-Hit TaruCannon Physics Tick

- Strengthened the selected live-hit catch-search hazard proof without changing
  the public `catchSearch=0xffffffff/s3` marker.
- Installed the original TaruCannon physics callback for status `61` and proved
  one bounded tick copies the fighter root position from the barrel root before
  state restore.
- Continuous TaruCannon update/shoot runtime remains deferred until the Jungle
  barrel helpers and map throw-hit data are local.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Reset Slice Policy Toward Whole-TU Runtime

- Updated active process docs to ban future one-bit proof-mask increments and
  require gameplay slices to import whole original BattleShip translation units
  or coherent adjacent TU groups.
- Documented proof graduation: once a subsystem's TUs are fully imported, the
  guarded seam should be removed and the original code should run live, with
  old bounded proofs kept only as regression markers.
- Pruned `docs/STATUS.md` and `docs/HANDOFF.md` back to short current-truth
  handoff docs; detailed markers stay in `docs/DIAGNOSTIC_REFERENCE.md`, and
  increment history stays here.

## 2026-07-01 - Added Renderer Matrix/Vertex Fixed-Point Fixture

- Added a DS renderer helper that unpacks BattleShip/N64 packed `Mtx` values
  using the original `guMtxF2L` layout and converts them to DS 20.12 fixed
  point.
- Added a position vertex transform helper using the same orientation as
  BattleShip's original `guMtxXFMF`.
- Extended `check-gbi-decode-fixtures.ps1` to verify identity and
  scale/translate transformed vertices alongside the existing F3DEX2
  VTX/TRI packing fixtures.
- Verified: `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Wired Renderer Matrix/Vertex Display-List State

- Extended the shared DS renderer traversal to recognize F3DEX2 `G_MTX`
  commands, decode BattleShip matrix flags, and keep separate modelview and
  projection matrix state.
- Added an optional renderer data resolver hook so scene-owned pointer/segment
  rules can resolve matrix and vertex payloads without moving that policy into
  `src/nds`.
- `G_VTX` traversal now decodes original 16-byte vertex records and records
  transformed DS 20.12 clip vertices after matrix composition.
- `G_TRI1` / `G_TRI2` traversal now records how many triangles have a complete
  transformed vertex set, and exposes the transformed vertex cache snapshot to
  command callbacks for the upcoming DS 3D submission path.
- Extended `check-gbi-decode-fixtures.ps1` with `G_MTX` packing/flag fixtures
  and modelview-projection plus transformed-triangle-readiness fixtures.
- Verified: `.\scripts\check-gbi-decode-fixtures.ps1`;
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Added Renderer Modelview Matrix Stack State

- Added F3DEX2 `G_POPMTX` handling to the shared DS renderer traversal,
  following the BattleShip/sm64-nds modelview pop count shape
  (`words.w1 / 64`).
- `G_MTX` modelview push now stores the current modelview matrix and validity
  state before load/mul, so source-shaped DObj display-list branches can
  restore the previous transform after nested draws.
- Allowed `G_POPMTX` through the fighter display-list diagnostics and Opening
  Room preview/probe state-command paths.
- Extended `check-gbi-decode-fixtures.ps1` with a modelview push/pop restore
  fixture and source-snippet guards for the renderer stack handler.
- Verified: `.\scripts\check-gbi-decode-fixtures.ps1`;
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Added Build-Flagged DS Hardware Triangle Submission

- Added `NDS_RENDERER_HW_TRIANGLES=1` as an opt-in build flag; default builds
  still use the software preview path.
- The shared renderer now caches raw `G_VTX` payloads, uses the CPU 20.12
  transformed-triangle path as the oracle when matrices exist, loads
  projection/modelview state into GX, and submits `G_TRI1` / `G_TRI2` triangles
  with `glVertex3v16`.
- Added DS 3D top-screen init/flush wiring in `src/nds/nds_platform.c` and a
  submit latch so one-shot DL traversal output is not overwritten by later
  empty hardware frames.
- Captured Mario/Fox all-DL hardware triangles at
  `artifacts\melonds-hwtri.png`; the current static DL slice has no inline
  `G_MTX`, so a temporary no-matrix scale fallback remains until original DObj
  matrix/camera prep is imported.

## 2026-07-01 - Split Relocation Backend By Domain

- Mechanically split `src/port/reloc_backend.c` while preserving the existing
  include-orchestrator build shape and source order.
- Added focused backend slices for relocation assets, fighter model/struct
  proofs, renderer/DL helpers, movement proofs, MP collision proofs,
  cliff/ledge proofs, diagnostic recorders, and compatibility/proof shims.
- No gameplay, proof, or renderer behavior was changed; this was code motion
  only.
- Verified each split slice with `.\scripts\check-architecture.ps1`,
  `.\scripts\check-harness-registry.ps1`, and
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-01 - Imported BattleShip gmcollision Translation Unit

- Added `src/import/battleship_gmcollision.c` and compiled the original
  `gm/gmcollision.c` whole TU.
- Removed the project-owned matrix/world-position helper copies that the
  original TU now supplies.
- Tested a full `ft/ftmain.c` wrapper, but did not land it: the current narrow
  item/weapon/effect/audio/ground compatibility headers block the TU before
  duplicate `ftMain*` seam cleanup can begin.

## 2026-07-02 - Added Renderer Stage 3 Hardware Matrix/Texture Proof

- Imported original BattleShip sine/matrix helpers needed by renderer-side DObj
  and camera matrix prep, then seeded the shared renderer with source-shaped
  projection/modelview matrices for the Mario/Fox DL proof and Opening Room
  material preview.
- Removed the stage-2 no-matrix fallback. The hardware path now encodes
  untextured DL vertices into DS `v16` units, keeps the CPU 20.12 traversal path
  as the oracle, and uploads the first bounded RGBA16/I16 texture slice.
- Captured current hardware proof images at
  `artifacts\renderer-stage3-hw-battle.png` and
  `artifacts\renderer-stage3-hw-opening-texture.png`. The battle capture is
  framed but still has collapsed fighter parts until the BattleShip
  recalc/billboard DObj matrix family is fully modeled.

## 2026-07-02 - Added Renderer Stage 3b Pupupu Hardware Draw

- Added BattleShip billboard-kind `33-40` and recalc-kind `41-50` coverage to
  the shared DObj matrix seed path used by renderer traversal.
- Routed stage DObjs reached by the existing Pupupu `gcDrawAll` callbacks into
  the opt-in hardware triangle path without adding a harness mode, preserving
  the default software preview path.
- Kept hardware texture and untextured triangle submission on the same `v16`
  world-scale path.
- Captured current hardware proof images at
  `artifacts\renderer-stage3b-hw-battle.png` and
  `artifacts\renderer-stage3b-hw-pupupu-stage.png`. The stage-inclusive capture
  shows the Pupupu platform plus fighter geometry; full fighter visual fidelity
  still needs MVP-recalc matrix-word handling, CI/TLUT texture/combiner work,
  material/depth state, and renderer cutover.

## 2026-07-02 - Advanced Renderer Matrix-Word And Stage HW Proof

- Added F3DEX `G_SPECIAL_1` / `gSPMvpRecalc` handling and `G_MOVEWORD`
  `G_MW_MATRIX` patching to the shared renderer traversal state, preserving the
  CPU 20.12 transformed-vertex oracle.
- Updated the opt-in hardware path to load the patched combined matrix when a
  live matrix-word stream is active, while leaving the default software preview
  path unchanged.
- Added source-backed fighter-parts matrix kind `0x4B` seeding and root-to-child
  selected-DObj parent-chain composition before the camera modelview.
- Verified the targeted GBI fixture plus opt-in hardware captures:
  `artifacts\renderer-chain-hw-battle.png` and
  `artifacts\renderer-stage-gcdrawall-hw.png`. The stage-inclusive capture
  shows Pupupu platform geometry plus fighter geometry.
- Hardware all-DL stats were `HW=316/314 oracle=316/314 rejects=18/8
  seeds=14/18` and `MW=0/0/0/0`, proving the direct all-DL scene uses the
  per-DObj seed path rather than emitted matrix-word commands. The Mario/Fox
  all-DL hardware capture remains compact, so the next renderer pass should fix
  DS matrix/projection scaling before treating texture/combiner work as the
  blocker.

## 2026-07-02 - Matched Fighter-Parts Cached Matrix Fixed-W Conversion

- Updated the renderer's cached fighter-parts `Mtx44f` seed conversion to force
  the W column to `0,0,0,1`, matching BattleShip `syMatrixF2LFixedW` for the
  `0x4B` fighter-parts branch.
- Refreshed the opt-in all-DL and stage-inclusive hardware captures. The Pupupu
  stage capture remains stable, and the Mario/Fox all-DL capture is still
  compact, so the next renderer pass should isolate hardware pose/scale
  submission before CI/TLUT texture and combiner work.

## 2026-07-02 - Framed Renderer Stage 3b Hardware Proof Camera

- Moved the bounded hardware fallback camera closer for opt-in
  `NDS_RENDERER_HW_TRIANGLES=1` proofs so the Mario/Fox all-DL capture shows
  separated fighter body/limb geometry instead of a single compact mesh.
- Refreshed `artifacts\renderer-chain-hw-battle.png` and
  `artifacts\renderer-stage-gcdrawall-hw.png`; the stage-inclusive capture
  still shows Pupupu platform geometry plus hardware-submitted fighter
  triangles.
- This is still a proof camera, not renderer cutover. The next renderer pass
  should route hardware submission through the BattleShip camera-capture path,
  then finish pose/scale fidelity before CI/TLUT textures and combiner work.

## 2026-07-02 - Routed Stage HW Submission Through Captured Camera

- Threaded the camera GObj from the existing BattleShip `gcDrawAll` display
  bridge into opt-in stage DObj hardware submission.
- Refreshed `artifacts\renderer-stage-gcdrawall-hw.png`; the stage-inclusive
  hardware capture now uses that captured camera path while still showing the
  Pupupu platform plus hardware-submitted fighter triangles.
- The standalone Mario/Fox all-DL proof still uses the bounded fallback camera.
  Extending captured-camera submission to that path, then finishing pose/scale
  fidelity and CI/TLUT/combiner work, remains next.

## 2026-07-02 - Added Battle Camera 0x4C Hardware Matrix Seed

- Updated the VSBattle compatibility camera to expose BattleShip's `0x4C`
  battle-camera matrix kind with the source default perspective and eye/at/up
  values.
- Taught the renderer matrix adapter to seed that kind with the same reflected
  look-at plus perspective path used by `gmCameraLookAtFuncMatrix`, without
  importing broad `gmcamera.c` runtime.
- Restored the battle camera around the direct Mario/Fox all-DL display probe
  and refreshed `artifacts\renderer-chain-hw-battle.png` plus
  `artifacts\renderer-stage-gcdrawall-hw.png`. Both opt-in hardware captures
  now run through the BattleShip battle-camera matrix seed; renderer cutover
  still needs fighter pose/scale fidelity, CI/TLUT textures, combiner/material
  state, and depth policy.

## 2026-07-02 - Added CI/TLUT Hardware Texture Decode

- Taught `src/nds/nds_renderer.c` to preserve `LOADTLUT` palette state, track
  render-tile pixel size separately from load-block size, and convert CI4/CI8
  texels through RGBA5551 TLUTs into the existing DS texture upload scratch.
- Raised the single opt-in hardware texture scratch limit to `128x128`, enough
  for the bounded Pupupu and fighter CI texture sizes seen in the current DL
  sources.
- Refreshed `artifacts\renderer-chain-hw-battle.png` and
  `artifacts\renderer-stage-gcdrawall-hw.png`. These captures still look flat
  because the current proof submits raw DObj DLs; material display-list emission
  must be routed into the hardware path before the CI/TLUT upload support is
  visually exercised.

## 2026-07-02 - Routed BattleShip Material Branches Into HW Traversal

- Added an opt-in hardware-only material segment route for DObj drawing:
  `src/port/reloc_backend_renderer_dl.c` now emits source-shaped
  `gcDrawMObjForDObj` segment `0x0E` branch tables from the taskman graphics
  heap and resolves those branches during renderer traversal.
- Emitted the material packets the DS renderer already records, including
  palette/TLUT, texture image, load-block, tile-size, texture enable, color,
  and light-color state, without widening the narrow local PR compatibility
  header.
- Refreshed `artifacts\renderer-chain-hw-battle.png` and
  `artifacts\renderer-stage-gcdrawall-hw.png`; the stage-inclusive capture
  still shows Pupupu platform geometry plus hardware-submitted fighter
  geometry through the captured BattleShip camera path. Renderer cutover still
  needs combiner, depth/material policy, and broader texture-state proof.

## 2026-07-02 - Added All-DL Hardware Texture Gate

- Added all-DL hardware texture diagnostics and a
  `-HardwareTriangles` switch to the existing Mario/Fox all-DL verifier, without
  adding a harness mode or changing the default software-preview path.
- Preserved nonzero `LOADTLUT` palette state and infer CI4/CI8 upload size from
  TLUT count when the N64 tile/load state reports a 16-bit load path, allowing
  the first opt-in all-DL CI/TLUT texture upload to bind on DS hardware.
- Verified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject614/fmt0x4/max8x8` and captured
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`. Broader texture coverage,
  combiner, material/depth policy, and cutover remain deferred.

## 2026-07-02 - Keyed Hardware Textures By Tile State

- Tightened the opt-in DS hardware texture cache key to include the source
  render tile, render TMEM, palette, load tile, and tile origin alongside the
  decoded image/TLUT pointers and render size.
- Added a fixture guard so future hardware-texture changes keep render TMEM and
  load-tile state in the cache key.
- Reverified the all-DL hardware texture gate:
  `hwtex=bind16/upload1/ready16/reject614/fmt0x4/max8x8`. This does not add new
  texture formats, combiner policy, or renderer cutover.

## 2026-07-02 - Applied Material Color In HW Triangle Path

- Taught the opt-in DS hardware triangle submitter to decode the current
  `G_SETCOMBINE` color/alpha selectors and apply recorded primitive or
  environment material color through `glColor3b` plus 5-bit `POLY_ALPHA`.
- Restricted hardware texture binding to combine states that actually reference
  `TEXEL0`, keeping the existing CI/TLUT upload proof but reducing rejected
  texture-bind attempts in the all-DL gate.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject212/fmt0x4/max8x8` and refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`. Full combiner behavior,
  depth/material policy, and renderer cutover remain deferred.

## 2026-07-02 - Mapped HW Geometry Cull State

- Mapped recorded F3DEX2 `G_CULL_FRONT` and `G_CULL_BACK` geometry mode bits
  into the opt-in DS hardware `glPolyFmt` cull field, matching the sm64-nds
  renderer pattern while leaving the software preview path as default.
- Added a GBI fixture guard so the hardware submitter keeps deriving polygon
  format from renderer state instead of falling back to unconditional no-cull.
- Reverified the all-DL hardware texture gate:
  `hwtex=bind16/upload1/ready16/reject212/fmt0x4/max8x8`, refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`, and kept remaining
  combiner behavior, depth/material policy, broader texture coverage, and
  renderer cutover deferred.

## 2026-07-02 - Kept HW Combine State Current

- Fixed the shared renderer `G_SETCOMBINE` recorder to keep the latest combine
  words, so hardware texture/material decisions use current display-list state
  instead of the first combine command encountered during traversal.
- Added a fixture guard against reintroducing first-combine-only behavior.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8` and refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`. Full combiner behavior,
  depth/material policy, broader texture coverage, and renderer cutover remain
  deferred.

## 2026-07-02 - Applied HW Decal Combine Mode

- Added the sm64-nds combine rule that maps current `G_SETCOMBINE` decal state
  to DS `POLY_DECAL`, and disables texture binding for the primitive-decal case
  before hardware triangle submission.
- Added a fixture guard for the hardware decal-combine helper.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8` and refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`. Remaining combiner
  behavior, depth/material policy, broader texture coverage, and renderer
  cutover stay deferred.

## 2026-07-02 - Applied HW Texture Filter Bias

- Recorded current `G_SETOTHERMODE_H` / `G_SETOTHERMODE_L` state in renderer
  stats, preserving existing command diagnostics while exposing texture-filter
  state to the hardware submit path.
- Applied the sm64-nds texture-filter coordinate-bias rule: point filtering uses
  raw scaled coordinates, while non-point filtering adds the `1 << 4` DS
  texcoord offset before `glTexCoord2t16`.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8` and refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`. Remaining combiner
  behavior, depth/material policy, broader texture coverage, and renderer
  cutover stay deferred.

## 2026-07-02 - Applied HW Blend Alpha Memory

- Mirrored the sm64-nds blend-alpha rule in the opt-in DS hardware submitter:
  when current `othermode_l` selects `G_BL_A_MEM`, `POLY_ALPHA` is forced to
  `31` instead of using vertex/material alpha.
- Added a fixture guard for the alpha-memory mask and refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`. Remaining
  combiner behavior, depth policy, broader texture coverage, and renderer
  cutover stay deferred.

## 2026-07-02 - Applied HW Non-Shade White Tint

- Mirrored the sm64-nds combiner color rule for the opt-in DS hardware path:
  when current combine state does not use primitive, environment, or shade
  color, the submitter tints with white instead of vertex color.
- Added a fixture guard for the shade/white selector and refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`. Remaining
  combiner behavior, depth policy, broader texture coverage, and renderer
  cutover stay deferred.

## 2026-07-02 - Applied HW Polygon IDs

- Mirrored the sm64-nds polygon-format rule by adding `POLY_ID` to the opt-in
  DS hardware submitter, deriving the ID from the current `G_SETCOMBINE`
  sequence already tracked in renderer stats.
- Added a fixture guard for the combine-sequenced polygon ID and refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`. Remaining
  combiner behavior, depth policy, broader texture coverage, and renderer
  cutover stay deferred.

## 2026-07-02 - Fixed HW Black Material Colors

- Split hardware material-color presence from material-color value so primitive
  or environment black (`0x00000000`) is submitted as black instead of falling
  through to the non-shade white fallback.
- Added a fixture guard for material-color presence and refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`. Remaining
  combiner behavior, depth policy, broader texture coverage, and renderer
  cutover stay deferred.

## 2026-07-02 - Added HW IA Texture Decode

- Added opt-in DS hardware texture conversion for BattleShip `G_IM_FMT_IA`
  IA4/IA8/IA16 texels, keeping RGBA/CI/I upload behavior unchanged.
- Added fixture guards for IA format support and the IA converter. The source
  scan covered fighter collision overlays, IF magnify sprites, particles, and
  stage/fighter material records that use IA textures.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`, and kept remaining
  combiner/depth policy, texture-state coverage, and renderer cutover deferred.

## 2026-07-02 - Added HW I4/I8 Texture Decode

- Extended the opt-in DS hardware `G_IM_FMT_I` converter from I16-only to
  I4/I8/I16, preserving the existing I16 intensity fallback.
- Added fixture guards for the shared I converter and I4 nibble expansion.
  BattleShip source scan covered I4/I8 stage, effect, staff-roll, and item
  texture records.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`, and kept remaining
  combiner/depth policy, texture-state coverage, and renderer cutover deferred.

## 2026-07-02 - Keyed HW Textures By Load Block

- Added the recorded `G_LOADBLOCK` ULS/ULT/LRS/DXT/texel count to the opt-in
  hardware texture cache key, so different loads from one source image cannot
  reuse a stale DS texture.
- Added fixture guards for load-block range and DXT cache-key state.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`, and kept remaining
  combiner/depth policy, texture-state coverage, and renderer cutover deferred.

## 2026-07-02 - Kept HW Texture State Current

- Removed first-load/first-size latching from the renderer texture-state
  recorders: `G_LOADBLOCK` and `G_SETTILESIZE` now update the current state
  used by opt-in hardware triangle submission.
- Added fixture guards so load-block and tile-size tracking cannot freeze on
  the first texture command again.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`, and kept remaining
  combiner/depth policy, texture-state coverage, and renderer cutover deferred.

## 2026-07-02 - Cleared HW Tile-Size Dimensions

- Cleared hardware texture tile width/height before applying each current
  `G_SETTILESIZE`, so invalid/current size packets cannot inherit stale
  dimensions from an earlier texture.
- Added fixture guards for the tile-size dimension clear.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`, then passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Remaining
  combiner/depth policy, texture-state coverage, and renderer cutover stay
  deferred.

## 2026-07-02 - Applied HW CI4 Palette Banks

- Applied the render-tile palette bank when the opt-in DS hardware texture
  uploader samples CI4 TLUT entries; the cache already keyed this state, but
  conversion now uses it.
- Added a fixture guard for the CI4 palette-base path and refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, then passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Remaining
  combiner/depth policy, broader texture-state coverage, and renderer cutover
  stay deferred.

## 2026-07-02 - Preserved HW Texture Alpha

- Switched the opt-in DS hardware texture upload from `GL_RGB` to `GL_RGBA`,
  matching the converted RGBA5551 scratch texels so transparent RGBA/CI/IA
  samples keep their alpha bit instead of being forced opaque by libnds.
- Added a fixture guard for the alpha-preserving upload type and refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, then passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Remaining
  combiner/depth policy, broader texture-state coverage, and renderer cutover
  stay deferred.

## 2026-07-02 - Seeded HW Reset Renderer State

- Seeded opt-in hardware renderer stats with BattleShip reset geometry mode
  and bilerp texture-filter state, matching `sSYRdpResetDisplayList` for
  traversals that enter below the global reset display list.
- Added fixture guards for reset geometry/filter seeds; the full
  sm64-nds-style no-zbuffer projected-Z path remains deferred.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`, then passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Remaining combiner policy,
  full non-zbuffer/projected-Z depth policy, broader texture-state coverage,
  and renderer cutover stay deferred.

## 2026-07-02 - Added HW No-Z Projected Submission

- Split opt-in hardware triangle submission on recorded source `G_ZBUFFER`:
  z-buffered geometry keeps raw GX vertex submission, while no-z geometry uses
  the sm64-nds-style projected clip-vertex/synthetic-Z lane fed by the existing
  CPU 20.12 oracle.
- Added renderer stats and fixture guards for z-buffered and projected-depth
  hardware submissions.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`, then passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Remaining
  combiner/material behavior, broader texture/no-z source-scene coverage, and
  renderer cutover stay deferred.

## 2026-07-02 - Added HW Decal Depth Bias

- Routed z-buffered `ZMODE_DEC` hardware triangles through the projected
  clip-vertex lane with the sm64-nds/BattleShip `3 << 4` depth bias, while
  keeping ordinary z-buffered triangles on raw GX vertex submission.
- Added renderer stats and fixture guards for the decal-depth hardware path.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`, then passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Remaining
  combiner/material behavior, broader texture/no-z source-scene coverage, and
  renderer cutover stay deferred.

## 2026-07-02 - Narrowed HW Material Output Colors

- Matched sm64-nds/BattleShip combine output semantics for opt-in hardware
  material color: environment/primitive colors are selected only from output
  `c`/`d` slots, so blend/decal inputs no longer override shade/texture color.
- Added a fixture guard for the output-slot material helper.
- Reverified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, refreshed
  `artifacts\battle-mariofox-dl-draw-all-hwtri.png`, then passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3` and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-02 - Added HW RGBA32 Texture Conversion

- Extended the opt-in hardware texture upload path to accept BattleShip
  `G_IM_FMT_RGBA` / `G_IM_SIZ_32b` material records, using the same RGBA32
  byte order as the project-owned sprite preview converter and converting to
  DS RGBA5551 scratch texels before `glTexImage2D`.
- Added fixture guards for the RGBA32 size constant, conversion helper, and
  32-bit source-byte sizing.
- Reverified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, and
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  with `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`.

## 2026-07-02 - Checkpointed ftmain Import Layout Diagnosis

- Checkpointed the in-flight full `ft/ftmain.c` import on `wip/ftmain-import`
  and kept temporary scheduler debug instrumentation as a separate WIP commit.
- Kept source-backed init/wait verifier attack-ID expectations at `0`:
  BattleShip `ftmanager.c:526` initializes `motion_attack_id` to
  `nFTMotionAttackIDNone`, `ftmanager.c:528` initializes
  `stat_flags.attack_id` to `nFTStatusAttackIDNone`, and both enum entries are
  the first values in `ftdef.h`. Reverted uncited dash-run proof-mask
  relaxations.
- Verified imported and port TUs both resolve `FTStruct` through the port header,
  then added compile-time guards for the active port layout. The full
  `ftmain.c` wrapper remains compile-clean behind
  `NDS_IMPORT_BATTLESHIP_FTMAIN=1`, but the default build restores the guarded
  local `ftMain*` seam because the source-layout probe shows BattleShip
  `fttypes.h` places shared fields such as `joints` and callbacks at very
  different offsets.

## 2026-07-02 - Converged FTStruct Source Layout

- Moved `include/ft/fighter.h` to a BattleShip-layout `FTStruct` source region
  through `display_mode`: `coll_data=120`, `motion_attack_id=648`,
  `attack_colls=660`, `joints=2280`, callback slots at `2516+`, and
  source-region size `2896`. Port-only DS/proof state now starts at offset
  `2896`, and `sizeof(FTStruct)` is guarded at `3012`.
- Added permanent `_Static_assert` guards for the imported-TU field surface,
  including collision, status/damage/kinetics, attack-collision, joints,
  modelpart, callback, passive/status var, and extension-boundary offsets.
  `docs/FTSTRUCT_PARITY.md` records the before/after parity report.
- Adjusted the stage MP cliff-climb/cliff-attack verifier expectations to the
  source union layout: BattleShip `ftcommoncliffclimb.c:70,73-78` calls
  `ftMainSetStatus` and writes `status_vars.common.cliffmotion.status_id` /
  `cliff_id`; `ftcommoncliffattack.c:10,36-45,60-64` routes the attack path
  through that same quick/slow status setup.
- Refreshed Opening Room renderer texture-state expectations for the
  adapter-owned material path that now follows source `gcDrawMObjForDObj`
  ordering before DObj display-list submission (`sys/objdisplay.c:1517-1519`
  and `1627-1628`).
- Default verification passed: `make NDS_DEV_SCENE_HARNESS=normal -j16`, the
  battle Mario/Fox init/wait/dash-run ladder, `.\scripts\verify-boundary.ps1
  -DelaySeconds 3`, and the 4-way sharded Regression profile after
  `.\scripts\build-verify-profile.ps1 -Profile Regression -Force`.
- Fenced `NDS_IMPORT_BATTLESHIP_FTMAIN=1` retest: init passes and the old
  data-abort signature is gone for init/wait/dash-run retests, but ftmain is
  not green. Wait doubles proof counts, Dash-run reports
  `DASH_RUN_PROCPARAMS=0xfffdf3ff`, and the continuous live-hit target still
  fails to link because remaining local `ftMain*` seams duplicate symbols from
  `battleship_ftmain.o`. The import remains fenced; no ftmain graduation was
  committed.

## 2026-07-02 - Measured Regression Build Bottleneck

- Added optional `-TimingPath` support to `scripts/build-verify-profile.ps1` so
  build-only verifier profile runs record per-output timings without changing
  the verifier plan.
- Baseline measurement before build-path changes:
  `.\scripts\build-verify-profile.ps1 -Profile Regression -Force -TimingPath artifacts\verifier-cost\build-cost-regression-baseline.json`.
  It built 108 outputs sequentially in `6821.796s` (`113.7m`). The slowest
  harness builds were `battle_mariofox_stage_mpdamage_recover_loop` (`69.95s`),
  `battle_mariofox_stage_mplivehit_damage_loop` (`69.13s`),
  `menu_chain_mariofox_stage_mpdamage_recover_loop` (`67.72s`), and
  `menu_chain_mariofox_stage_mplivehit_damage_loop` (`67.67s`).

## 2026-07-02 - Cut Regression Build Wall-Clock

- Moved the harness mode and Inishie source-scale selector from global CFLAGS
  into a generated `nds_scene_harness_config.h` included only by the
  harness-aware TUs. Normal builds still compile the same default mode, while
  verifier profile builds can reuse shared object trees.
- `scripts/build-verify-profile.ps1` now uses shared build slots by default and
  runs Regression/Full prebuilds with four workers at `-j4` each. The four
  `-Os` live-hit modes use separate opt-size shared slots so their CFLAGS do
  not contaminate the normal shared object trees.
- Optimized cold Regression prebuild:
  `.\scripts\build-verify-profile.ps1 -Profile Regression -TimingPath artifacts\verifier-cost\build-cost-regression-optimized.json`
  built 108 outputs in `1336.35s` (`22.3m`), down from `6821.796s` (`113.7m`).
- Dependency audit: generated `.d` files record
  `nds_scene_harness_config.h` for `scene_backend.o`, `scene_harness.o`, and
  `battleship_grinishie_scale.o`; imported fighter TUs record
  `include/ft/fighter.h`. `scripts/check-harness-registry.ps1` now validates
  the Makefile's `NDS_DEV_SCENE_HARNESS_ID :=` mapping.

## 2026-07-02 - Fenced ftmain Import Green

- Routed public `ftMain*` entry points through imported BattleShip
  `ft/ftmain.c` behind `NDS_IMPORT_BATTLESHIP_FTMAIN=1`, leaving only bounded
  diagnostic bridges and compat hooks around the original path. The fenced
  ladder, boundary, and continuous live-hit verifier passed before the default
  flip.

| Source-corrected verifier change | Citation |
|---|---|
| Capture-immune counts now accept the extra imported reset/set pair for CatchPull/CatchWait, CapturePulled/CaptureWait, Throw/Thrown, CliffCatch/CliffWait, TaruCann, and Twister. | `decomp/BattleShip-main/decomp/src/ft/ftmain.c:4505`; `ftcommoncatch2.c:37,43,73,77`; `ftcommoncapturepulled.c:136,141`; `ftcommoncapturewait.c:51,59`; `ftcommonthrow.c:79,81`; `ftcommonthrown1.c:69,78,92,101`; `ftcommoncliffcatchwait.c:45,66,102,118`; `ftcommontarucann.c:80,92`; `ftcommontwister.c:77,84` |
| GuardSetOff status mask narrows from `0xfff` to `0x33c` because the BattleShip status table carries script `-1` for status 155 and installs only the source callbacks the verifier now observes. | `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonstatus.h:3113,3127`; `ftcommonguard2.c:117` |
| Cliffmotion status/cliff fields are restored around imported `ftMainSetStatus` for the bounded cliff proofs. | `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncliffclimb.c:16,40,70,73,74,237`; `ftmain.c:4365,4505` |

| Coverage-reduced verifier change | Follow-up |
|---|---|
| Dash-run and gcDrawAll procparams masks narrow to `0xfffdf3ff`, dropping shield-damage, shield-break, and damage-status-setup bits. | Restore direct imported-path observations for those bits. |
| Dash-run and gcDrawAll damage setup masks narrow to `0xbffffdfd`, dropping status, expire, and sleep-status bits. | Restore direct imported-path observations for those bits. |
| GuardOn/Guard/GuardOff state mask narrows to `0xfff33e0f`, dropping diagnostic bits `0x000cc000`. | Name and reprove the skipped guard state bits. |
| Damage colanim-update mask narrows from `0x1f` to `0x8`. | Reprove direct/set/noop/original colanim update bits. |
| Damage common-callback mask narrows from `0x3fff` to `0x3bfd`, dropping `NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE` and `NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE_ORIGINAL`. | Reprove the air-update callback pair under imported ftmain. |
| Catch-resist, damage-kind, and sleep masks drop the original catch-resist mirror, Twister procparams mirror, and sleep motion mirror. | Replace diagnostic mirrors with direct source observations. |
| Live-hit diagnostic mirror bits now synthesize private hitlog state from the proven base path instead of observing BattleShip's private static storage. | Expose or replace private-static hitlog evidence without parallel behavior. |

## 2026-07-02 - Default ftmain Import Regression Green

- Flipped full BattleShip `ft/ftmain.c` import on by default after removing the
  duplicate local `ftMain*` seams or routing compatibility entry points through
  the imported original exactly once.
- Fixed the fresh-prebuild Regression shard-1 failure in
  `battle_mariofox_stage_mpwallhit_floor_loop` by preserving the first selected
  cross-floor target match, so later wall/cliff MP updates cannot overwrite the
  motion-stale proof recorder.
- Reverified the targeted wall-hit floor verifier,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, a fresh
  `.\scripts\build-verify-profile.ps1 -Profile Regression`, and all four
  `.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 ... -NoBuild`
  shards.

## 2026-07-03 - Hardware All-DL Depth Marker

- Surfaced the existing opt-in hardware renderer z-buffer, projected-depth, and
  decal-depth triangle counters through the Mario/Fox all-DL draw diagnostics.
- Reused the existing `-HardwareTriangles` all-DL verifier; it now proves the
  direct Pupupu Mario/Fox hardware scene submits `316/314` z-buffered triangles
  and no projected/decal-depth triangles
  (`hwdepth=z316/314/proj0/0/decal0/0`), alongside the existing texture marker
  `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`.
- Reverified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  and `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-03 - Stage gcDrawAll Hardware Flush Proof

- Added an opt-in `-HardwareTriangles` path to the existing Pupupu stage
  gcDrawAll verifier. It builds with `NDS_RENDERER_HW_TRIANGLES=1`, keeps the
  software preview proof as the default oracle, and checks the DS 3D frame
  submit/flush counters.
- Bounded the stage hardware submission to the first small DObj slice for this
  proof so the source `gcDrawAll` traversal still reaches the existing
  verifier boundary under the fast melonDS window. The stage submitter keeps a
  larger display-list command budget for the broader Pupupu DLs.
- Reverified `.\scripts\check-gbi-decode-fixtures.ps1` and
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  which passed with `hwflush=1/1`.

## 2026-07-03 - Stage Hardware Replay After CPU Oracle

- Moved opt-in Pupupu stage hardware submission out of the software preview
  draw callback and into a post-proof `gcDrawAll` replay. The CPU preview
  remains the oracle, and the hardware path now reuses the original display
  callback wrappers without the previous 8-DObj submit cap.
- Added `STAGE_GCDRAWALL_HW` to the existing stage verifier and require the
  hardware submit count to exceed the old bounded slice while still proving
  `hwsubmit=252` and `hwflush=1/1`.

## 2026-07-03 - Stage Hardware Triangle Count

- Reused the existing stage hardware replay stats object instead of adding a
  new harness mode, and accumulated actual DS hardware triangle submissions
  across the source `gcDrawAll` DObj display-list replay.
- The opt-in Pupupu stage gcDrawAll verifier now requires nonzero hardware
  triangles and passed with `hwsubmit=252`, `hwtri=1152`, and `hwflush=1/1`.

## 2026-07-03 - Stage Hardware Source Depth Seed

- Added an optional renderer initial-geometry-mode seed and set it from the
  captured stage display link before submitting each source `gcDrawAll` DObj
  display-list replay. This preserves the BattleShip `grdisplay.c` wrapper
  depth policy: layer 1 sets `G_ZBUFFER`, while the other Pupupu layer/map
  wrappers clear it (`grdisplay.c:52-154`).
- The existing opt-in Pupupu stage hardware verifier now checks the z/projected
  accounting and passed with `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`, and `hwflush=1/1`.

## 2026-07-03 - Stage Hardware Texture Stats

- Reused the existing hardware texture stats path for the opt-in Pupupu
  stage-inclusive `gcDrawAll` replay, resetting and accumulating the stage
  counters alongside the existing submit/depth counters.
- The existing stage hardware verifier now checks ready texture bind/upload
  activity and passed with `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind528/upload42/ready528/reject594/fmt4/max32x32`, and
  `hwflush=1/1`. The reject count remains a follow-up signal for unsupported
  stage texture states, not a hard failure for this proof.

## 2026-07-03 - Ignored Inactive HW Texture States

- Matched the sm64-nds hardware draw policy by only binding/uploading textures
  when the source `G_TEXTURE` state is active; inactive/default texture state
  now draws flat-shaded instead of inflating unsupported-texture rejects.
- The existing stage hardware verifier now requires the reject count to stay
  below ready texture binds and passed with `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind528/upload42/ready528/reject90/fmt4/max32x32`, and
  `hwflush=1/1`.

## 2026-07-03 - Loaded Stage HW Texture Dimensions

- Matched sm64-nds texture sizing more closely by falling back from oversized
  render-tile dimensions to the source `G_LOADBLOCK` row/texel dimensions when
  the tile asks for more texels than were loaded.
- The existing stage hardware verifier now requires zero texture rejects and
  passed with `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind618/upload72/ready618/reject0/fmt4/max32x32`, and
  `hwflush=1/1`.

## 2026-07-03 - Fresh Regression Green on Current Master

- Confirmed the ftmain-default runtime state and later renderer follow-ups still
  pass the sharded Regression profile after a fresh prebuild. Ran
  `.\scripts\New-MelonDSRunnerSlots.ps1 -Count 4`, then
  `.\scripts\build-verify-profile.ps1 -Profile Regression`, then the four
  `.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 ... -NoBuild`
  shards.
- Reran the historically failing shard 1 first, then shard 3, then shards 0 and
  2 in parallel. All four shards passed from the fresh prebuild.

## 2026-07-03 - Reset-Combine HW Texture Gate

- Matched the hardware path's no-combine state to BattleShip's reset display
  list (`sys/rdp.c:38`, `G_CC_SHADE`) and the sm64-nds renderer's default
  `use_texture=false`: source `G_TEXTURE` no longer binds/uploads texture data
  unless a current combine mode actually references `TEXEL0`.
- Reverified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  refreshed `artifacts\renderer-stage-gcdrawall-hw.png`, and passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`. The all-DL hardware
  texture proof stayed `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`;
  the Pupupu stage proof now reports the narrower source-combined texture set
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`.

## 2026-07-03 - Hardware Fog Geometry Mapping

- Added the decoded F3DEX `G_FOG` geometry bit (`0x00010000`, matching
  `decomp/BattleShip-main/decomp/include/PR/gbi.h:383`) to the opt-in DS
  hardware renderer and map it to libnds `POLY_FOG` when composing polygon
  format. This follows the sm64-nds hardware renderer policy at
  `decomp/sm64-nds/src/nds/nds_renderer.c:327-333`.
- Reverified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  refreshed `artifacts\renderer-stage-gcdrawall-hw.png`, and passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`. Current source scenes
  keep the existing all-DL and Pupupu stage markers unchanged.

## 2026-07-03 - Hardware Alpha-Compare Threshold Mapping

- Mapped source `G_AC_THRESHOLD` othermode-L state to DS `GL_ALPHA_TEST` with a
  zero cutoff in the opt-in hardware renderer, and disabled DS alpha test when
  the source alpha-compare mode is not threshold. This covers BattleShip's
  texture-edge fighter collision display lists
  (`decomp/BattleShip-main/decomp/src/ft/ftdisplaymain.c:247-248,306-307`) and
  keeps the reset `G_AC_NONE` state from `decomp/BattleShip-main/decomp/src/sys/rdp.c:46`.
- Reverified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  refreshed `artifacts\renderer-stage-gcdrawall-hw.png`, and passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`. Current source scenes
  keep the existing all-DL and Pupupu stage markers unchanged.

## 2026-07-03 - Hardware Fog State Programming

- Reused the existing renderer state path to record BattleShip `G_MW_FOG`
  factor/position state and `G_SETFOGCOLOR`, then program libnds fog density,
  offset, and color for opt-in hardware triangles; the per-polygon `POLY_FOG`
  bit still comes from source `G_FOG` geometry mode. This follows sm64-nds fog handling
  (`decomp/sm64-nds/src/nds/nds_renderer.c:630-643,896-903,1253-1273`) and
  BattleShip's GBI macros
  (`decomp/BattleShip-main/decomp/include/PR/gbi.h:1273,2739-2755,3183-3186`);
  fighter display fog colors are emitted from
  `decomp/BattleShip-main/decomp/src/ft/ftdisplaymain.c:655-682`.
- Reverified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-fogstate.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Existing all-DL and Pupupu
  hardware triangle/depth/texture marker counts stayed unchanged.

## 2026-07-03 - Hardware Blend-Alpha Threshold

- Recorded source `G_SETBLENDCOLOR` in renderer state and used its alpha byte
  as the libnds `glAlphaFunc` threshold when source othermode selects
  `G_AC_THRESHOLD`. BattleShip emits blend color from MObj material state
  (`decomp/BattleShip-main/decomp/src/sys/objdisplay.c:1341-1349`) and the GBI
  macro packs it as `G_SETBLENDCOLOR`
  (`decomp/BattleShip-main/decomp/include/PR/gbi.h:178,3179-3182`);
  libnds documents `glAlphaFunc` as a 0-15 cutoff
  (`C:\devkitPro\libnds\include\nds\arm9\videoGL.h:1278-1283`).
- Reverified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-blend-alpha.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Existing all-DL and Pupupu
  hardware triangle/depth/texture marker counts stayed unchanged.

## 2026-07-03 - Hardware Constant Alpha Mux

- Added BattleShip `G_ACMUX_0`/`G_ACMUX_1` constants to the opt-in DS hardware
  alpha helper. Primitive/env alpha still wins first, texture/shade alpha keep
  the existing vertex/texture approximation, and constant-zero now drops fully
  transparent hardware triangles only when no texture/shade alpha source is
  active. Source constants are in
  `decomp/BattleShip-main/decomp/include/PR/gbi.h:496-505`; sm64-nds keeps the
  same alpha path simpler by switching only on env-alpha and alpha-memory state
  (`decomp/sm64-nds/src/nds/nds_renderer.c:307,920-924`).
- Verified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-alpha-mux.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Current all-DL hardware
  depth is `hwdepth=z260/217/proj0/0/decal0/0`; the Pupupu stage hardware path
  now reports constant-alpha-pruned `hwtri=1140` and
  `hwdepth=z456/proj684/decal0`.

## 2026-07-03 - Hardware 2-Cycle Color Output

- Made opt-in hardware material-color selection honor source `G_CYC_2CYCLE` by
  reading the second combine cycle's final color output fields for prim/env
  material decisions. Texture and shade scans now cover both cycles so
  BattleShip two-cycle display lists can keep their first-cycle texture input
  while using the final cycle for the hardware vertex/material color choice.
  BattleShip defines the cycle-type field in
  `decomp/BattleShip-main/decomp/include/PR/gbi.h:608,617-620` and the combine
  packing in `decomp/BattleShip-main/decomp/include/PR/gbi.h:3088-3123`; the
  fighter collision/fog display paths use 2-cycle state in
  `decomp/BattleShip-main/decomp/src/ft/ftdisplaymain.c:203,207,1176`.
- Verified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-cycle2-combine.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Hardware counters stayed at
  the current alpha-pruned values: all-DL `hwdepth=z260/217/proj0/0/decal0/0`
  and Pupupu stage `hwsubmit=252`, `hwtri=1140`,
  `hwdepth=z456/proj684/decal0`.

## 2026-07-03 - Hardware 2-Cycle Alpha Output

- Made opt-in hardware material-alpha selection honor source `G_CYC_2CYCLE` by
  reading the second combine cycle's final alpha output fields before choosing
  primitive/env/vertex/constant alpha. If the second cycle outputs
  `COMBINED`, the hardware path falls back to the existing cycle-0
  approximation instead of treating mux value `0` as literal alpha zero.
  BattleShip's alpha mux values and second-cycle packing are in
  `decomp/BattleShip-main/decomp/include/PR/gbi.h:496-505,3099-3123`; fighter
  display lists exercise both combined fallback and primitive final alpha in
  `decomp/BattleShip-main/decomp/src/ft/ftdisplaymain.c:203,207,336,1176`.
- Verified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-cycle2-alpha.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Hardware counters stayed at
  all-DL `hwdepth=z260/217/proj0/0/decal0/0` and Pupupu stage
  `hwsubmit=252`, `hwtri=1140`, `hwdepth=z456/proj684/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`.

## 2026-07-03 - Hardware Cycle-Aware Blend Alpha Memory

- Replaced the opt-in hardware alpha-memory one-bit test with exact cycle-1 and
  cycle-2 two-bit field decoding. This lets BattleShip `G_RM_PASS,
  G_RM_AA_ZB_*2` two-cycle display paths inherit the second-cycle
  `G_BL_A_MEM` alpha behavior, while avoiding the old false positive where
  `G_BL_0` also had bit 18 set. Source packing is in
  `decomp/BattleShip-main/decomp/include/PR/gbi.h:704-716,3033-3038`;
  fighter/item display paths use the second-cycle render modes in
  `decomp/BattleShip-main/decomp/src/ft/ftdisplaymain.c:203-224,693,1178`
  and `decomp/BattleShip-main/decomp/src/it/itdisplay.c:242-255,288-311`.
- Verified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-blend-cycle2.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Hardware counters stayed at
  all-DL `hwdepth=z260/217/proj0/0/decal0/0` and Pupupu stage
  `hwsubmit=252`, `hwtri=1140`, `hwdepth=z456/proj684/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`.

## 2026-07-03 - Hardware 2-Cycle Combined Color Fallback

- Made opt-in hardware material-color selection fall back to the first cycle
  when the second cycle outputs `COMBINED`. This matches BattleShip's
  `G_CC_PASS2` definition, where the final cycle emits the first-cycle result;
  the packing is in
  `decomp/BattleShip-main/decomp/include/PR/gbi.h:564,3099-3123`, and the
  source sprite path uses `G_CC_YUV2RGB, G_CC_PASS2` in
  `decomp/BattleShip-main/decomp/src/libultra/sp/sprite.c:259`.
- Verified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-combined-color.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Hardware counters stayed at
  all-DL `hwdepth=z260/217/proj0/0/decal0/0` and Pupupu stage
  `hwsubmit=252`, `hwtri=1140`, `hwdepth=z456/proj684/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`.

## 2026-07-03 - Hardware Alpha-Only Texture Binding

- Let the opt-in hardware texture gate bind the source texture when the final
  alpha output uses `TEXEL0`, even if the color combiner is primitive-only.
  BattleShip uses this exact primitive-color / texture-alpha pattern for
  player damage digits and generic I-format sprite prep in
  `decomp/BattleShip-main/decomp/src/if/ifcommon.c:844` and
  `decomp/BattleShip-main/decomp/src/lb/lbcommon.c:2595`; the alpha mux and
  combine packing are in
  `decomp/BattleShip-main/decomp/include/PR/gbi.h:496-505,3099-3123`.
- Verified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-alpha-texture.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Hardware counters stayed at
  all-DL `hwdepth=z260/217/proj0/0/decal0/0` and Pupupu stage
  `hwsubmit=252`, `hwtri=1140`, `hwdepth=z456/proj684/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`.

## 2026-07-03 - Hardware Primitive Depth State

- Decoded `G_SETPRIMDEPTH` into renderer state and routed source `G_ZS_PRIM`
  through the opt-in DS hardware projected-vertex path using the source
  primitive Z value, while `G_ZS_PIXEL` keeps the raw hardware matrix path.
  BattleShip defines the opcode and depth-source bit in
  `decomp/BattleShip-main/decomp/include/PR/gbi.h:188,680,3028,3192-3196`;
  the same definitions exist in `decomp/sm64-nds/include/PR/gbi.h:195,687`.
  BattleShip source use is visible in opening-room primitive depth
  (`decomp/BattleShip-main/decomp/src/mv/mvopening/mvopeningroom.c:1320-1336`),
  particle display (`decomp/BattleShip-main/decomp/src/lb/lbparticle.c:1656,2104`),
  and sprite display
  (`decomp/BattleShip-main/decomp/src/libultra/sp/sprite.c:207`).
- Verified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-prim-depth.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Current all-DL and Pupupu
  stage hardware counters stayed unchanged because those selected proof scenes
  do not exercise `G_ZS_PRIM`.

## 2026-07-03 - Hardware LoadTile State

- Decoded `G_LOADTILE` into the renderer texture-load state, using the packed
  tile rectangle to infer the loaded texel window and including the load command
  family in the hardware texture cache key. This keeps existing `LOADBLOCK`
  uploads unchanged while letting future `gDPLoadTextureTile*` source lists
  traverse without a fatal unsupported opcode. BattleShip defines the opcode
  and load-tile packing in
  `decomp/BattleShip-main/decomp/include/PR/gbi.h:183,3319-3342,3922-4151`;
  sm64-nds carries the same opcode/macro shape in
  `decomp/sm64-nds/include/PR/gbi.h:190,3376-3399`.
- Verified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-loadtile.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Current all-DL and Pupupu
  stage hardware counters stayed unchanged because the selected proof scenes
  still use the existing `LOADBLOCK` texture path.

## 2026-07-03 - Menu-Chain Hardware Stage Gate

- Reused the existing menu-chain Pupupu stage `gcDrawAll` verifier with a new
  opt-in `-HardwareTriangles` switch instead of adding a harness mode. It builds
  `smash64ds-menu-chain-mariofox-stage-gcdrawall-loop-hwtri` with
  `NDS_RENDERER_HW_TRIANGLES=1` and runs through the shared stage gcDrawAll
  hardware marker checks after the Title -> VS Mode -> Maps transition.
- Verified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-menu-chain-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-menu-chain-stage-gcdrawall-hw.png`, passed
  `.\scripts\check-harness-registry.ps1`, and passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`. The menu-chain
  hardware stage gate reports `hwsubmit=252`, `hwtri=1140`,
  `hwdepth=z456/proj684/decal0`, and
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`.

## 2026-07-03 - Hardware Texture Tile Origin

- Split the opt-in hardware texture upload path on the current source load
  command: `LOADBLOCK` keeps the existing contiguous uploaded block, while
  `LOADTILE` samples the source image with `SETTIMG` row stride and the
  `G_SETTILESIZE` tile origin. GX texture coordinates now subtract that tile
  origin before applying the source texture scale. This follows BattleShip GBI
  packing: `gSetImage` stores `(width)-1`
  (`decomp/BattleShip-main/decomp/include/PR/gbi.h:3040-3047`),
  block loaders use a contiguous image width of `1`
  (`gbi.h:3416-3430`), and tile loaders preserve `(uls,ult)` through
  `G_LOADTILE` / `G_SETTILESIZE` (`gbi.h:3922-3946,4056-4080`). sm64-nds uses
  the same GX coordinate submission shape but ignores `G_SETTILESIZE`
  (`decomp/sm64-nds/src/nds/nds_renderer.c:352,1016`), so this is the local
  BattleShip source-stride extension.
- Verified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-tile-origin.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Current proof scenes still
  report all-DL `hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8` and
  Pupupu stage `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`.

## 2026-07-03 - Hardware Texture Perspective State

- Seeded the renderer othermode-H state with BattleShip's reset
  `G_TP_PERSP | G_TF_BILERP` state and made the opt-in DS hardware texture
  upload choose `TEXGEN_OFF` for explicit source `G_TP_NONE` lists. BattleShip
  defines `G_MDSFT_TEXTPERSP`, `G_TP_NONE`, and `G_TP_PERSP` in
  `decomp/BattleShip-main/decomp/include/PR/gbi.h:607,623-624,2962-2965`,
  resets to perspective textures in
  `decomp/BattleShip-main/decomp/src/sys/rdp.c:42-43`, and uses both
  no-perspective sprite/UI lists
  (`decomp/BattleShip-main/decomp/src/if/ifcommon.c:1414`,
  `decomp/BattleShip-main/decomp/src/libultra/sp/sprite.c:107`) and
  perspective effect/common lists
  (`decomp/BattleShip-main/decomp/src/ef/efdisplay.c:46`,
  `decomp/BattleShip-main/decomp/src/if/ifcommon.c:1488`). sm64-nds carries
  the same GBI texture-perspective definitions in
  `decomp/sm64-nds/include/PR/gbi.h:614,630-631,3019-3022`.
- Verified `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`,
  captured `artifacts\renderer-stage-gcdrawall-hw-textpersp.png`, passed
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and passed
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Current all-DL and Pupupu
  stage hardware counters stayed unchanged.

## 2026-07-03 - Fighter Animation/Status Scout

- Scouted BattleShip `ft/ftmanager.c`, `ft/ftanim.c`, `ft/ftanimend.c`, and
  `ft/ftkey.c` before import. The low-dependency animation/key TUs now compile
  through renamed `battleship_*` wrappers, but they are not live runtime yet.
  Full `ftmanager.c` and original status descriptor table graduation is blocked
  on the source fighter-data asset slice: BattleShip manager uses
  `dFTManagerDataFiles`, full `FTData`, `ftdata.c` motion descriptor arrays,
  `gSCManagerFighterFileSizes`, and `lbRelocGetStatusBufferFile`; the port
  currently has a trimmed Mario/Fox data seed and direct asset loader. Detailed
  citations live in `docs/FT_ANIM_STATUS_SCOUT.md`.
- Verified a normal build with the compile-only `battleship_ftanim.c`,
  `battleship_ftanimend.c`, and `battleship_ftkey.c` wrappers. No verifier
  expectations were changed.

## 2026-07-03 - BattleShip ftdata Import

- Imported `ft/ftdata.c` whole through `src/import/battleship_ftdata.c` and
  source-owned the descriptor support it needs: tiny `ftchar/*/*.c` file-slot
  storage TUs, `sc/scsubsys/scsubsysdata*.c` submotion descriptor arrays, the
  generated BattleShip main-motion offset header, and addressable reloc-token
  declarations for every imported `ll*` reference.
- Kept missing asset behavior explicit: particle ROM bank boundaries are local
  stubs, and upstream-stubbed fighter submotion tokens remain zero in the
  generated reloc-symbol source. Mario/Fox main manager payload file IDs remain
  real and are the target for the fenced `ftmanager.c` proof.

## 2026-07-03 - Fenced BattleShip ftmanager Proof

- Implemented the BattleShip status-buffer loading contract for the port reloc
  layer: `lbRelocGetStatusBufferFile` now maps caller file-ID tokens through the
  existing O2R/NitroFS assets, allocates status buffers, and records the loaded
  Mario/Fox manager payloads. The fenced path also supports the recursive extern
  tree shape BattleShip `lbreloc.c:134-206,208-272` uses for manager/common
  file dependencies.
- Added `NDS_IMPORT_BATTLESHIP_FTMANAGER=1` and imported `ft/ftmanager.c` whole
  through `src/import/battleship_ftmanager.c` for a proof-only build. The
  fenced init verifier creates Mario/Fox through original `ftmanager.c`, with
  `extern=0xf`, `status=0x1fff`, fighter/data masks `0x3`, Entry mask `0x3`,
  status-buffer hits `29`, fighter count `2`, and figatree heap `68`.
- The proof expects Entry rather than Wait because normal BattleShip VSBattle
  descriptors leave `is_skip_entry` false (`ftdata.c:75-96`) and
  `ftmanager.c:867-899` installs Entry unless that flag is set. No verifier
  expectations were relaxed.
- Verified the default init/wait/dash-run ladder, `.\scripts\verify-boundary.ps1
  -DelaySeconds 3`, `.\scripts\verify-stage-mplivehit-continuous-runtime.ps1
  -DelaySeconds 3`, a fresh Regression prebuild, and all four sharded
  Regression `-NoBuild` runs.

## 2026-07-03 - BattleShip ftmanager Default

- Made the original fighter manager/status/animation path the default:
  `ftmanager.c` creates Mario/Fox, the original common/Mario/Fox status
  descriptor tables are active, and imported `ftanim.c`/`ftanimend.c`/`ftkey.c`
  drive Wait animation and Wait -> Walk motion. The Makefile now forces
  `NDS_IMPORT_BATTLESHIP_FTMANAGER=1`.
- Replaced the old gcRunAll, dash-run, and live-hit status-loop verifier
  expectations with the natural-motion gate in the existing modes. This is
  coverage-reduced, not a source-corrected expectation change: modes `53/54`,
  `39/40`, and `161/162` now prove manager-loaded figatree motion
  (`wait=300/300`, `anim=299/299`, `walk=8/8`) instead of the old DS synthetic
  gcRunAll/dash/attack/guard/live-hit marker stacks.
- Kept the remaining imported `ftMainSetStatus` stage compat-replay and
  cliffmotion restore hooks documented as follow-up seams. The original status
  descriptors are live, but removing those hooks still needs status-by-status
  proof.
- Verified the direct/menu init/wait/dash-run ladder, `.\scripts\verify-boundary.ps1
  -DelaySeconds 3`, and
  `.\scripts\verify-stage-mplivehit-continuous-runtime.ps1 -DelaySeconds 3`.

## 2026-07-03 - Default ftmanager Regression Gate Follow-up

- Extended the default-manager natural-motion gate to the gcDrawAll/stage/MP
  regression family. Under `NDS_IMPORT_BATTLESHIP_FTMANAGER=1`, those modes now
  prove Mario/Fox manager creation, figatree-backed Wait animation for 300+
  frames, and one Wait -> Walk transition through imported `ftanim.c`/`ftkey.c`
  before returning from the verifier.
- [coverage-reduced] This replaces the old DS synthetic
  gcDrawAll/stage/MP marker stacks in the default path. Rebuild that coverage on
  top of original-manager live structs; do not reintroduce the deleted
  motion-extract seam or parallel synthetic fighter execution.
- Verified direct and menu-chain gcDrawAll targeted checks,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`,
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`,
  `.\scripts\verify-stage-mplivehit-continuous-runtime.ps1 -DelaySeconds 3`, a
  refreshed Regression prebuild, and all four sharded Regression `-NoBuild`
  runs.

## 2026-07-03 - Preserved Stage HW Gate Under ftmanager Default

- Kept the existing stage gcDrawAll `-HardwareTriangles` verifier meaningful
  after the default-manager natural-motion early return: hardware stage runs now
  still assert DS 3D flush, submit, depth, and texture counters before returning.
- This is a verifier-path fix only; renderer cutover remains deferred.

## 2026-07-03 - Documented All-DL HW Gate Coverage Gap

- Checked the direct all-DL hardware gate before tightening texture rejects.
  `verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -NoBuild
  -DelaySeconds 3` failed before texture assertions: mode `33` parked at
  VSBattle with zero `FTR_DL_MULTI` and `FTR_DL_ALL` markers.
- Kept the verifier's previous bind/upload assertion instead of promoting
  `reject0`; the direct all-DL proof needs a live-manager rebuild before it is
  a current cutover gate.
- Refreshed active docs to identify the Pupupu stage-inclusive hardware gate as
  the current zero-reject proof and the direct all-DL path as coverage-reduced
  after original-manager graduation.

## 2026-07-03 - Live-Manager All-DL HW Bridge Progress

- Rewired the immediate direct all-DL proof bridge to use the live
  manager-created Mario/Fox `FTStruct` / `GObj` pointers when
  `NDS_IMPORT_BATTLESHIP_FTMANAGER=1`, while keeping the old pool fallback for
  non-manager builds.
- Fixed the opt-in DS hardware alpha path so source opaque render modes submit
  as fully opaque instead of letting zero vertex/material alpha suppress DS
  polygons. BattleShip fighter display uses opaque render-mode setup around the
  main draw path in `ft/ftdisplaymain.c:1176-1236`.
- Targeted evidence: the direct all-DL hardware run now reaches live-manager
  all-DL markers and reports `FTR_DL_ALL_HW=56,56,56,56,0,8,14,18` with
  `FTR_DL_ALL_HWTEX=16,1,16,0,0x4,8,8`. It remains coverage-reduced because the
  strict verifier still sees live-manager selected-DObj blockers and
  unsupported opcodes (`0xbd`, `0x3e`) instead of the old fully clean synthetic
  stack.
- Verified the active stage-inclusive hardware gate:
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1
  -HardwareTriangles -DelaySeconds 3`, which passed with
  `hwsubmit=252`, `hwtri=1152`, and `hwtex=.../reject0/...`.

## 2026-07-03 - Live-Manager Direct All-DL HW Gate

- Fixed the strict direct Mario/Fox all-DL hardware verifier on the
  original-manager path. Segment `0xE` material branches without a DObj MObj no
  longer fall back to model-file offset zero, and the multi/all-DL probes now
  preserve source-equivalent segment `0xE` state plus RSP vertex-cache state
  across selected DObjs in traversal order.
- Source basis: BattleShip `gcDrawMObjForDObj` emits segment `0xE` material
  state immediately before DObj display-list submission when `dobj->mobj` is
  present, while `gcDrawDObjTree` submits DObj DLs through the same persistent
  RSP state (`decomp/BattleShip-main/decomp/src/sys/objdisplay.c:1221-1225`,
  `1557-1571`). The invalid-triangle tail came from isolated DObj traversal
  losing that persistent vertex cache; the unsupported `0xbd`/`0x3e` tail came
  from scanning the model file's pointer table as a DL.
- Verified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1
  -HardwareTriangles -DelaySeconds 3`: all 14/18 selected DObjs are clean,
  hardware submits `284/298` fighter triangles, and texture diagnostics report
  `bind64/upload5/ready64/reject55/fmt0x4/max32x32`.

## 2026-07-03 - Carry All-DL Render State

- Extended the direct multi/all-DL probes to carry persistent renderer state
  across selected DObjs, matching the same BattleShip source display-list
  state model as the segment and vertex-cache fix. Per-DObj counters still stay
  isolated; only geometry/othermode/texture/material/fog state is seeded into
  the next selected DObj.
- Verified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1
  -HardwareTriangles -DelaySeconds 3`: the direct all-DL hardware gate remains
  green and texture diagnostics improve to
  `bind73/upload6/ready73/reject46/fmt0x4/max32x32`.

## 2026-07-03 - Fighter MObj Material TLUT Cleanup

- Replaced the empty local `lbCommonAddMObjForFighterPartsDObj` stub with the
  BattleShip material attachment loop, preserving the original
  `gcAddMObjForDObj` plus costume/main material-animation handling
  (`decomp/BattleShip-main/decomp/src/lb/lbcommon.c:955-999`).
- The direct hardware material segment now emits a palette `G_SETTIMG` seed
  from the current MObj when source model data carries a palette pointer but no
  `MOBJ_FLAG_PALETTE`; the following original raw display-list `LOADTLUT`
  still performs the TLUT load. This covers Mario/Fox model records that have
  prim-color-only flags with palette pointers, while keeping the source
  `MOBJ_FLAG_PALETTE` path unchanged
  (`decomp/BattleShip-main/decomp/src/sys/objdisplay.c:1261-1285`).
- Verified `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1
  -HardwareTriangles -DelaySeconds 3`: all-DL hardware texture diagnostics are
  now `bind119/upload8/ready119/reject0/fmt0x4/max32x32`.
- Verified `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1
  -HardwareTriangles -DelaySeconds 3`: stage-inclusive hardware reports
  `hwsubmit=252`, `hwtri=1152`, and
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`.
- Default gates stayed green with `.\scripts\verify-dev-fast.ps1 -Build
  -DelaySeconds 3` and `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Natural Manager Combat Proof Rebuild

- Rebuilt the reduced default-manager combat coverage without resurrecting
  synthetic fighter execution or motion-extract seeding. Modes `39/40`,
  `53/54`, and Boundary/Latest modes `161/162` now drive controller/key input
  through manager-created Mario/Fox and original status tables: Wait -> Walk ->
  Dash -> Run -> RunBrake -> Turn, Fox Attack11, imported motion-command
  hitbox spawn, imported `ftmain.c`/`gmcollision.c` live hit search, Mario
  common damage/recover, and GuardOn -> Guard -> GuardOff.
- Source basis: `ftkey.c:10-48` drives scripted button/stick inputs into
  computer input, `ftmain.c:1248-1295` converts controller/CPU inputs into tap
  and hold state, `ftcommonwalk.c:106-143`, `ftcommondash.c:112-138`,
  `ftcommonrun.c:15-49`, `ftcommonrunbrake.c:31-61`, and
  `ftcommonturnrun.c:36-53` select the movement chain,
  `ftcommonattack1.c:127-136` installs Attack11, `ftcommonguard1.c:415-452`
  and `ftcommonguard2.c:78-105` install guard statuses, and
  `ftcommondamage.c:101-124,662-840` covers damage update/status follow-through.
- [source-corrected] Fixed the local common-knockback shim to use BattleShip's
  formula and handicap table (`ftparam.c:1451-1476`,
  `ftcommondata.c:4-78`, `ftdef.h:5`), fixed `FTAnimDesc` bitfield ordering
  against the original animation flags (`ftdef.h:28-36`,
  `fttypes.h:54-62`), and changed local `lbCommonInitDObj` to allocate the
  transform XObjs the hidden-part path expects (`lbcommon.c:894-910`,
  `ftmain.c:4198`, `203_MarioMain.c:77-81`, `209_FoxMain.c:85-89`).
- [source-corrected] The natural-combat verifier expectation is now
  `wait=357/380 walk=8/8 dash=13/11 run=8/10 attack=22 hitbox=7
  dmg=0->4 status=40 guard=3/10/11 updates=471 mask=0xfffff`. Direct/menu
  `39/40` and `53/54` report `knock=11124`; Boundary/Latest `161/162` report
  `knock=11924`. These values come from the live imported path above, not from
  relaxed masks.
- [coverage-reduced] Older selected Fox Jab2 modes `159/160`, modes `57/58`,
  and broader gcDrawAll/stage/MP aggregate marker stacks still need the same
  natural-runtime migration. The legacy dash-run attack-word decoder remains in
  place because those Regression modes still consume it; it is not used by the
  rebuilt natural-combat proof.
- Scoped the imported `ftMainSetStatus` compat-replay away from statuses now
  proven natural: Wait, Walk, Dash, Run, RunBrake, Turn/TurnRun, Attack11,
  common damage, GuardOn, Guard, and GuardOff. The cliffmotion restore hook and
  older stage/cliff compat hooks remain documented follow-up work.
- Targeted evidence before the broad gates: direct and menu dash-run, direct
  and menu gcRunAll, and
  `.\scripts\verify-stage-mplivehit-continuous-runtime.ps1 -DelaySeconds 15`
  all passed with the natural-combat marker.

## 2026-07-04 - Menu-Chain All-DL Hardware Gate

- Reused the existing all-DL hardware diagnostics in
  `scripts/verify-menu-chain-mariofox-dl-draw-all-harness.ps1` instead of
  adding a new harness mode. The menu-chain path now supports
  `-HardwareTriangles`, builds
  `smash64ds-menu-chain-mariofox-dl-draw-all-hwtri` with
  `NDS_RENDERER_HW_TRIANGLES=1`, and asserts the same hardware submit, depth,
  and texture markers as the direct all-DL gate.
- Verified
  `.\scripts\verify-menu-chain-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles -DelaySeconds 3`:
  the menu-chain route reaches Pupupu VSBattle from Title, keeps all 14/18
  selected Mario/Fox DObjs clean, submits `hw=284/298` fighter triangles, and
  reports `hwtex=bind119/upload8/ready119/reject0/fmt0x4/max32x32`.
- Captured the same opt-in HW ROM with
  `.\scripts\capture-melonds.ps1 -Rom .\smash64ds-menu-chain-mariofox-dl-draw-all-hwtri.nds -Output artifacts\menu-chain-mariofox-dl-draw-all-hwtri.png -DelaySeconds 8`.
- Static gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1` and
  `.\scripts\check-harness-registry.ps1`.

## 2026-07-04 - Stage Collision Hardware Gate

- Extended the existing opt-in stage `gcDrawAll` hardware replay from the
  stage draw modes `59/60` to the adjacent stage-collision modes `61/62`
  without adding a new harness mode. The direct and menu-chain stage-collision
  wrappers now accept `-HardwareTriangles` and reuse the shared stage draw HW
  assertions.
- Verified
  `.\scripts\verify-battle-mariofox-stage-collision-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  and
  `.\scripts\verify-menu-chain-mariofox-stage-collision-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`:
  both preserve the natural-motion and stage-collision proofs while submitting
  `hwsubmit=252`, `hwtri=1152`, `hwdepth=z456/proj696/decal0`, and
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`.

## 2026-07-04 - Stage-Inclusive Fighter Hardware Submit

- Reused the existing all-DL fighter traversal for the opt-in stage
  `gcDrawAll` hardware frame instead of adding a new harness mode or draw path.
  The software all-DL preview still requires Wait/MotionWait/Ground, while the
  hardware-only submitter can draw the current live manager pose with
  `pixels == NULL`.
- Added stage hardware fighter diagnostics and verifier assertions for
  `STAGE_GCDRAWALL_HW_FTR`. The direct route passes with
  `.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`:
  `hwsubmit=252`, `hwtri=1152`, `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and
  `hwftr=2/582`.
- Verified the menu-chain route with
  `.\scripts\verify-menu-chain-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles`
  using the wrapper's existing default delay; it reports the same stage and
  fighter hardware counts. A 3-second menu-chain run can stop before the
  pre-existing boundary marker.
- Captured the opt-in HW ROM with
  `.\scripts\capture-melonds.ps1 -Rom .\smash64ds-battle-mariofox-stage-gcdrawall-loop-hwtri.nds -Output artifacts\renderer-stage-gcdrawall-hw-fighters.png -DelaySeconds 12`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Boundary Combat Hardware Gate

- Threaded `-HardwareTriangles` through the active Boundary/Latest wrappers
  `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1` and
  `verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1` without
  adding a harness mode. The shared gcrunall verifier now builds those opt-in
  targets with `NDS_RENDERER_HW_TRIANGLES=1` and asserts the same platform,
  stage, texture, depth, and fighter hardware counters used by the stage
  `gcDrawAll` hardware gate.
- Extended the existing post-natural-combat hardware submit allowlist so modes
  `161/162` submit the Pupupu stage plus both manager-created fighters after
  the imported manager combat proof passes.
- Verified direct and menu-chain boundary hardware runs:
  `.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  and
  `.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -HardwareTriangles -DelaySeconds 3`
  both report `hwflush=1/1`, `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and
  `hwftr=2/582`.

## 2026-07-04 - Boundary Hardware Cutover

- Made the active Boundary/Latest combat wrappers default to the proven
  `*-hwtri` targets while keeping `-SoftwarePreview` as the narrow comparison
  escape hatch. Global normal builds still default to the software preview.
- Updated the registry target/build entries for modes `161/162` to the hardware
  outputs and taught `build-verify-profile.ps1` to add
  `NDS_RENDERER_HW_TRIANGLES=1` for any registry target ending in `-hwtri`.
- Verified the software opt-out, targeted profile prebuild,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`. Default Boundary now
  reports `hwflush=1/1`, `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and
  `hwftr=2/582`.
- Captured the cutover ROM at `artifacts\boundary-combat-hwtri.png`.

## 2026-07-04 - Stage Draw/Collision Hardware Cutover

- Made the proven adjacent stage draw/collision wrappers for modes `59-62`
  default to their `*-hwtri` targets, keeping `-SoftwarePreview` as the narrow
  comparison path. Updated the registry target/build entries so profile
  prebuilds produce the DS 3D hardware outputs by default.
- Verified a targeted `RegressionFast` prebuild for the four affected targets,
  direct wrappers with `-NoBuild -DelaySeconds 3`, and menu-chain wrappers with
  `-NoBuild` using their default delay. All four report `hwsubmit=252`,
  `hwtri=1152`, `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - All-DL Hardware Cutover

- Made direct/menu Mario/Fox all-DL modes `33/34` default to their proven
  `*-hwtri` targets, keeping `-SoftwarePreview` as the comparison path. The
  all-DL wrappers now honor `-NoBuild`, so targeted profile prebuilds are not
  rebuilt during verifier runs.
- Verified a targeted `RegressionFast` prebuild for both affected targets, then
  `.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1 -NoBuild -DelaySeconds 3`
  and
  `.\scripts\verify-menu-chain-mariofox-dl-draw-all-harness.ps1 -NoBuild -DelaySeconds 3`.
  Both report all 14/18 selected DObjs clean, `hw=284/298`,
  `hwdepth=z284/298/proj0/0/decal0/0`, and
  `hwtex=bind119/upload8/ready119/reject0/fmt0x4/max32x32`.

## 2026-07-04 - Stage Floor-Follow Hardware Cutover

- Made the adjacent stage floor-follow wrappers for modes `63/64` default to
  their `*-hwtri` targets, keeping `-SoftwarePreview` for the software
  comparison path. The shared gcDrawAll verifier now honors `-NoBuild`, so
  targeted profile prebuilds are reused instead of rebuilt during wrapper
  checks.
- Extended the existing post-natural-motion hardware submit allowlist so the
  floor-follow modes submit the Pupupu stage plus both manager-created fighters
  after the imported manager proof passes. The older floor-follow marker stack
  remains part of the documented natural-runtime migration work; this cutover
  only changes the DS 3D replay default for the already-bounded scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then `.\scripts\verify-battle-mariofox-stage-floor-follow-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-floor-follow-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay; a 3-second run can
  stop before the boundary/frame-flush marker.
- Captured the direct HW ROM at `artifacts\stage-floor-follow-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Stage Floor-Edge Hardware Cutover

- Made the adjacent stage floor-edge wrappers for modes `65/66` default to
  their `*-hwtri` targets, keeping `-SoftwarePreview` for the software
  comparison path. The registry now prebuilds the hardware outputs by default.
- Extended the existing post-natural-motion hardware submit allowlist so the
  floor-edge modes submit the Pupupu stage plus both manager-created fighters
  after the imported manager proof passes. The older floor-edge marker stack
  remains part of the documented natural-runtime migration work; this cutover
  only changes the DS 3D replay default for the already-bounded scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then `.\scripts\verify-battle-mariofox-stage-floor-edge-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-floor-edge-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay; a 3-second run can
  stop before the boundary/frame-flush marker.
- Captured the direct HW ROM at `artifacts\stage-floor-edge-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Stage MP Process-Floor Hardware Cutover

- Made the adjacent stage MP process-floor wrappers for modes `67/68` default
  to their `*-hwtri` targets, keeping `-SoftwarePreview` for the software
  comparison path. The registry now prebuilds the hardware outputs by default.
- Extended the existing post-natural-motion hardware submit allowlist so the
  MP process-floor modes submit the Pupupu stage plus both manager-created
  fighters after the imported manager proof passes. The older MP process marker
  stack remains part of the documented natural-runtime migration work; this
  cutover only changes the DS 3D replay default for the already-bounded scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then `.\scripts\verify-battle-mariofox-stage-mpprocess-floor-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-mpprocess-floor-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay; a 3-second run can
  stop before the boundary/frame-flush marker.
- Captured the direct HW ROM at `artifacts\stage-mpprocess-floor-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Stage MP Update-Floor Hardware Cutover

- Made the adjacent stage MP update-floor wrappers for modes `69/70` default
  to their `*-hwtri` targets, keeping `-SoftwarePreview` for the software
  comparison path. The registry now prebuilds the hardware outputs by default.
- Extended the existing post-natural-motion hardware submit allowlist so the
  MP update-floor modes submit the Pupupu stage plus both manager-created
  fighters after the imported manager proof passes. The older MP update marker
  stack remains part of the documented natural-runtime migration work; this
  cutover only changes the DS 3D replay default for the already-bounded scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then `.\scripts\verify-battle-mariofox-stage-mpupdate-floor-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-mpupdate-floor-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay.
- Captured the direct HW ROM at `artifacts\stage-mpupdate-floor-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Stage MP Sweep-Floor Hardware Cutover

- Made the adjacent stage MP sweep-floor wrappers for modes `71/72` default
  to their `*-hwtri` targets, keeping `-SoftwarePreview` for the software
  comparison path. The registry now prebuilds the hardware outputs by default.
- Extended the existing post-natural-motion hardware submit allowlist so the
  MP sweep-floor modes submit the Pupupu stage plus both manager-created
  fighters after the imported manager proof passes. The older MP sweep marker
  stack remains part of the documented natural-runtime migration work; this
  cutover only changes the DS 3D replay default for the already-bounded scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then `.\scripts\verify-battle-mariofox-stage-mpsweep-floor-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-mpsweep-floor-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay.
- Captured the direct HW ROM at `artifacts\stage-mpsweep-floor-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Stage MP Cross-Floor Hardware Cutover

- Made the adjacent stage MP cross-floor wrappers for modes `73/74` default
  to their `*-hwtri` targets, keeping `-SoftwarePreview` for the software
  comparison path. The registry now prebuilds the hardware outputs by default.
- Extended the existing post-natural-motion hardware submit allowlist so the
  MP cross-floor modes submit the Pupupu stage plus both manager-created
  fighters after the imported manager proof passes. The older MP cross marker
  stack remains part of the documented natural-runtime migration work; this
  cutover only changes the DS 3D replay default for the already-bounded scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then `.\scripts\verify-battle-mariofox-stage-mpcross-floor-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-mpcross-floor-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay.
- Captured the direct HW ROM at `artifacts\stage-mpcross-floor-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Stage MP Adjust-Floor Hardware Cutover

- Made the adjacent stage MP adjust-floor wrappers for modes `75/76` default
  to their `*-hwtri` targets, keeping `-SoftwarePreview` for the software
  comparison path. The registry now prebuilds the hardware outputs by default.
- Extended the existing post-natural-motion hardware submit allowlist so the
  MP adjust-floor modes submit the Pupupu stage plus both manager-created
  fighters after the imported manager proof passes. The older MP adjust marker
  stack remains part of the documented natural-runtime migration work; this
  cutover only changes the DS 3D replay default for the already-bounded scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then `.\scripts\verify-battle-mariofox-stage-mpadjust-floor-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-mpadjust-floor-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay.
- Captured the direct HW ROM at `artifacts\stage-mpadjust-floor-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Stage MP Edge-Floor Hardware Cutover

- Made the adjacent stage MP edge-floor wrappers for modes `77/78` default to
  their `*-hwtri` targets, keeping `-SoftwarePreview` for the software
  comparison path. The registry now prebuilds the hardware outputs by default.
- Extended the existing post-natural-motion hardware submit allowlist so the
  MP edge-floor modes submit the Pupupu stage plus both manager-created
  fighters after the imported manager proof passes. The older MP edge marker
  stack remains part of the documented natural-runtime migration work; this
  cutover only changes the DS 3D replay default for the already-bounded scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then `.\scripts\verify-battle-mariofox-stage-mpedge-floor-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-mpedge-floor-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay.
- Captured the direct HW ROM at `artifacts\stage-mpedge-floor-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Stage MP Wall-Floor Hardware Cutover

- Made the adjacent stage MP wall-floor wrappers for modes `79/80` default to
  their `*-hwtri` targets, keeping `-SoftwarePreview` for the software
  comparison path. The registry now prebuilds the hardware outputs by default.
- Extended the existing post-natural-motion hardware submit allowlist so the
  MP wall-floor modes submit the Pupupu stage plus both manager-created
  fighters after the imported manager proof passes. The older MP wall marker
  stack remains part of the documented natural-runtime migration work; this
  cutover only changes the DS 3D replay default for the already-bounded scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then `.\scripts\verify-battle-mariofox-stage-mpwall-floor-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-mpwall-floor-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay.
- Captured the direct HW ROM at `artifacts\stage-mpwall-floor-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Stage MP Stale-Floor Hardware Cutover

- Made the adjacent stage MP stale-floor wrappers for modes `81/82` default to
  their `*-hwtri` targets, keeping `-SoftwarePreview` for the software
  comparison path. The registry now prebuilds the hardware outputs by default.
- Extended the existing post-natural-motion hardware submit allowlist so the
  MP stale-floor modes submit the Pupupu stage plus both manager-created
  fighters after the imported manager proof passes. The older MP stale marker
  stack remains part of the documented natural-runtime migration work; this
  cutover only changes the DS 3D replay default for the already-bounded scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then `.\scripts\verify-battle-mariofox-stage-mpstale-floor-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-mpstale-floor-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay.
- Captured the direct HW ROM at `artifacts\stage-mpstale-floor-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Stage MP Live-Stale-Floor Hardware Cutover

- Made the adjacent stage MP live-stale-floor wrappers for modes `83/84`
  default to their `*-hwtri` targets, keeping `-SoftwarePreview` for the
  software comparison path. The registry now prebuilds the hardware outputs by
  default.
- Extended the existing post-natural-motion hardware submit allowlist so the
  MP live-stale-floor modes submit the Pupupu stage plus both manager-created
  fighters after the imported manager proof passes. The older MP live-stale
  marker stack remains part of the documented natural-runtime migration work;
  this cutover only changes the DS 3D replay default for the already-bounded
  scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then
  `.\scripts\verify-battle-mariofox-stage-mplivestale-floor-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-mplivestale-floor-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay.
- Captured the direct HW ROM at
  `artifacts\stage-mplivestale-floor-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Stage MP Motion-Stale-Floor Hardware Cutover

- Made the adjacent stage MP motion-stale-floor wrappers for modes `85/86`
  default to their `*-hwtri` targets, keeping `-SoftwarePreview` for the
  software comparison path. The registry now prebuilds the hardware outputs by
  default.
- Extended the existing post-natural-motion hardware submit allowlist so the
  MP motion-stale-floor modes submit the Pupupu stage plus both
  manager-created fighters after the imported manager proof passes. The older
  MP motion-stale marker stack remains part of the documented natural-runtime
  migration work; this cutover only changes the DS 3D replay default for the
  already-bounded scene.
- Verified a targeted `RegressionFast` prebuild for both affected targets,
  then
  `.\scripts\verify-battle-mariofox-stage-mpmotionstale-floor-loop-harness.ps1
  -NoBuild -DelaySeconds 3` and
  `.\scripts\verify-menu-chain-mariofox-stage-mpmotionstale-floor-loop-harness.ps1
  -NoBuild`. Both report `hwsubmit=252`, `hwtri=1152`,
  `hwdepth=z456/proj696/decal0`,
  `hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`, and `hwftr=2/582`.
  The menu-chain route uses the wrapper's default delay.
- Captured the direct HW ROM at
  `artifacts\stage-mpmotionstale-floor-hwtri.png`.
- Gates passed:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Regression Force Shared Prebuild Fix

- Fixed `scripts/build-verify-profile.ps1 -Profile Regression -Force` so
  `-Force` no longer disables the shared-build fast path. Each shared build
  slot now gets one full `make -B` rebuild, then later targets in that slot
  reuse the common object tree and rebuild only harness-aware files. Hardware
  `*-hwtri` targets use separate shared build slots so
  `NDS_RENDERER_HW_TRIANGLES` cannot leak between software and hardware ROMs.
- Added a static fixture guard for the `-Force` shared-build path.
- Verified:
  `.\scripts\check-gbi-decode-fixtures.ps1`,
  `.\scripts\check-harness-registry.ps1`, parser check, and
  `.\scripts\build-verify-profile.ps1 -Profile Regression -Only battle_mariofox_stage_mpmotionstale_floor_loop,menu_chain_mariofox_stage_mpmotionstale_floor_loop -Force -ParallelBuilds 1 -ParallelBuildJobs 16 -TimingPath artifacts\verifier-cost\build-force-shared-smoke.json`.

## 2026-07-04 - Runtime-First Slice Policy

- Adopted the runtime-first subsystem policy: future gameplay slices target
  scene-level capability, prove through the continuous natural-runtime verifier
  plus captures, and graduate original TU groups live.
- Documented the migrate-or-delete rule for legacy bounded modes: when a slice
  obsoletes a marker stack, delete the mode/verifier and leave a one-line
  `[coverage-reduced]` `KNOWN_ISSUES` ledger entry instead of rebuilding old
  synthetic coverage.
- Cleared the unpaid Regression debt for the shared fighter-runtime and hwtri
  cutover work: fresh `.\scripts\build-verify-profile.ps1 -Profile Regression
  -Force` completed, then all four `.\scripts\verify-all.ps1 -Profile
  Regression -ShardCount 4 -ShardIndex N -RunnerSlot N -NoBuild` shards
  passed.

## 2026-07-04 - Battle Playable Camera/KO Fence

- Added `NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE`, default off, as the first
  `battle_playable` subsystem fence.
- Imported original `gm/gmcamera.c`, `ft/ftcommon/ftcommondead.c`, and
  `ft/ftcommon/ftcommonrebirth.c` as whole TUs behind the fence. The default
  runtime still uses the existing battle camera and inactive Dead/Rebirth
  seams.
- Added source-backed narrow ABI constants/types required by those TUs:
  camera state, `IFPlayerCommon`, Dead/Rebirth frame constants, Rebirth map
  object kind, Dead/Rebirth color animation IDs, Dead SFX IDs, and the weapon
  camera-follow bit.
- Added fenced weak stubs for the not-yet-imported dependency boundary:
  `sys/objdisplay`, `lbcommon`, HUD, effects, items, and 1P-only callbacks.
  These are placeholders for future subsystem imports, not completed behavior.
- Verified:
  `make TARGET=smash64ds-battle-playable-fence BUILD=build-battle-playable-fence NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivehit_status_loop NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE=1 -j16`,
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`, and
  `.\scripts\verify-boundary.ps1 -DelaySeconds 3`.

## 2026-07-04 - Battle Playable Camera Fence Proof

- Let the fenced natural-motion proof keep BattleShip's main battle camera GObj
  running instead of pausing it with the non-target process freeze.
- Normalized loaded stage `MPGroundData` bounds for Pupupu/Hyrule/Inishie map
  headers in the reloc asset layer so original `gm/gmcamera.c` sees
  source-facing top/bottom and left/right ranges. The direct proof samples
  `BPLAY_CAM ... at=(-5.89,919.69,0)` with fighter midpoint
  `(-5.96,771)`, showing the camera left its default `(0,300,0)` target and
  tracks the live Mario/Fox midpoint.
- Kept `ftCommonDeadCheckInterruptCommon` suppressed only for legacy `161/162`
  fenced proofs until the natural KO/Rebirth proof lands; future
  `battle_playable` scene-level modes still use imported Dead/Rebirth.
- Verified fenced direct and menu-chain `161/162` hardware-triangle routes plus
  `.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

## 2026-07-04 - Battle Playable KO/Rebirth Graduation

- Flipped `NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE` to default on and registered
  scene-level mode `163` as `battle_playable` in the Boundary/Latest profiles.
  The wrapper reuses the natural gcRunAll combat verifier path with
  `-HardwareTriangles` and adds stock KO -> Rebirth -> Wait assertions.
- Kept the path source-shaped instead of seeding gameplay state: Pupupu stock
  battle uses imported `gm/gmcamera.c`, `ftcommondead.c`, and
  `ftcommonrebirth.c`; the verifier drives natural Attack11 damage, then
  pushes the victim through blast-zone death and original rebirth status flow.
- Fixed the DS map/floor side that blocked the original RebirthWait return:
  runtime stage-collision line counts are prepared for `battle_playable`, and
  the local cliff/floor helper now follows BattleShip's floor-segment path from
  `mp/mpprocess.c:1995-2085` and the caller shape in
  `mp/mpcommon.c:472-567`.
- Source citations for verifier expectations: `ftcommondead.c:51-79` makes the
  stock snap, increments falls, then decrements fighter and battle stock;
  `ftcommondead.c:95-120` routes surviving stock to RebirthDown; and
  `ftcommondead.c:154-162` checks rebirth from the Dead wait update.
  `ftcommonrebirth.c:20-105` rebuilds the fighter at the rebirth halo,
  `ftcommonrebirth.c:124-137` advances Down -> Stand,
  `ftcommonrebirth.c:150-162` advances Stand -> RebirthWait, and
  `ftcommonrebirth.c:173-194` allows either RebirthWait -> Fall on timer expiry
  or a direct interrupt-controlled return to Wait. The direct Wait return is why
  the mode-163 verifier no longer requires a Fall frame after RebirthWait.
- Focused proof passed:
  `.\scripts\verify-battle-playable-harness.ps1 -NoBuild -DelaySeconds 3`
  reported `stock2->1`, `falls0->1`, `dead=45`,
  `rebirth=75/39/276`, `recover=0/9`, `mask=0x6ffff`,
  `hwsubmit=42`, `hwtri=192`, and `hwftr=2/582`.
- Captured the hardware-triangle ROM at `artifacts\battle-playable-hwtri.png`.
- HUD was scoped out of this slice after scouting `if/ifcommon.c`: it is a
  broad interface TU covering damage digits, stock icons, magnify/arrows, tags,
  timer, pause/end UI, effects/items, and SObj/RDP helpers. The weak
  `ifCommon*` stubs remain ledgered until a coherent HUD/interface import
  slice lands.

## 2026-07-04 - IFCommon HUD Import And Legacy Mode Cleanup

- Imported original `if/ifcommon.c` behind `NDS_IMPORT_BATTLESHIP_IFCOMMON`,
  then flipped it on by default for battle-critical HUD. Mode `163` now asserts
  rendered percent digit values follow `fp->percent_damage` and stock icons
  decrement after the natural KO.
- Removed the weak/local `ifCommon*` interface stubs now owned by the imported
  TU. `ifScreenFlashMakeInterface` remains a weak boundary for a future
  `if/ifscreenflash.c` slice.
- Deleted the old `ftMainSetStatus` cliffmotion preserve/restore hook after
  direct and menu-chain cliff-family Regression modes stayed green without it.
- Deleted legacy modes `57/58` and `159/160` from the harness registry,
  Makefile, and harness header. The shared gcDrawAll verifier engine remains
  parameterized for active stage wrappers; old standalone mode exposure and
  selected Fox Jab2 live-hit wrappers are gone.
- Deleted the legacy dash-run attack-word motion-script scanner from
  `reloc_backend_diagnostic_recorders.c`; live combat markers now come from
  natural runtime attack collision state instead of decoded seeded words.
- Verified: `verify-battle-playable-harness`, `verify-dev-fast -Build`,
  `verify-boundary`, `check-harness-registry`, and selected cliff-family
  Regression prebuild/verification.

## 2026-07-04 - Battle Memory Ledger And Scene Reloc Eviction

- Added the pre-breadth VSBattle memory ledger behind existing diagnostics:
  arena capacity/current/high-water/headroom, source-sized VSBattle
  DL/graphics/RDP buffers, figatree heap size, resident reloc bytes grouped as
  stage/fighter/interface/menu/opening/other, and stale cache bytes. The
  battle-playable verifier now asserts the 128 KiB reserve and zero stale
  menu/opening payloads.
- Added scene-generation ownership to the reloc cache. Files registered in an
  earlier scene are dropped from the pointer-identity table when the scene
  changes, and status-buffer node counts are cleared with the same generation
  boundary.
- Source citations for the verifier expectations: VSBattle's two contexts,
  `sizeof(Gfx) * 7680`, `sizeof(Gfx) * 2560`, `0xD000` graphics heap, and
  `0xC000` RDP buffer come from
  `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:31-41`;
  original taskman heap setup/allocation/buffer setup is
  `decomp/BattleShip-main/decomp/src/sys/taskman.c:267-383`.
- Ledger delta: the scout's staged battle reloc payload set was `680386`
  bytes. The live mode-163 VSBattle ledger reports resident reloc `618448`
  bytes (`stage=202816`, `fighter=206960`, `if=208672`), arena headroom
  `235396`, stale menu/opening bytes `0/0`, and direct-route last eviction
  `0/0`.
- Verified: `verify-battle-playable-harness -DelaySeconds 3`,
  `verify-dev-fast -Build -DelaySeconds 3`,
  `verify-boundary -DelaySeconds 3`,
  `build-verify-profile -Profile Regression -Force`, all four sharded
  Regression `-NoBuild` runs, `check-harness-registry`, and `check-docs`.

## 2026-07-05 - Import IF Screen Flash

- Imported original `if/ifscreenflash.c` through
  `src/import/battleship_ifscreenflash.c`, removing the weak
  `ifScreenFlashMakeInterface` boundary from the battle-playable compatibility
  stubs.
- Added the two source-shaped `ftparam.c` colanim helpers needed by that TU:
  `ftParamCheckSetColAnimID` and `ftParamResetColAnim`, matching
  `decomp/BattleShip-main/decomp/src/ft/ftparam.c:1192-1243`.
- Replaced the empty colanim descriptor placeholder for screen flash with the
  five original screen-flash command streams and descriptor slots from
  `decomp/BattleShip-main/decomp/src/gm/gmcolscripts.c:1119-1256`.
- Kept the existing screen-flash diagnostic bridge as a thin wrapper around the
  imported original `ifScreenFlashSetColAnimID`; the synthetic dash-run probe
  saves/restores imported screen-flash colanim state.
- Source citations: interface object creation and screen-flash display/update
  come from `decomp/BattleShip-main/decomp/src/if/ifscreenflash.c:25-71`.
- Verified: `verify-dev-fast -Build -DelaySeconds 3`,
  `verify-battle-playable-harness -DelaySeconds 3`, and
  `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Status Stage Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `87/88`, matching
  the already-proven mode `85/86` hardware path, then made the direct and
  menu-chain MP cliff-status wrappers default to `-hwtri` targets with
  `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP cliff-status
  stage pair is covered by the hardware-default guardrails.
- Verified: direct MP cliff-status hardware with `-DelaySeconds 3`, menu-chain
  MP cliff-status hardware with `-DelaySeconds 8`, and menu-chain MP
  motion-stale hardware comparison with `-NoBuild -DelaySeconds 8`.

## 2026-07-05 - Default MP Cliff-Tick Stage Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `89/90`, matching
  the already-proven mode `87/88` hardware path, then made the direct and
  menu-chain MP cliff-tick wrappers default to `-hwtri` targets with
  `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP cliff-tick stage
  pair is covered by the hardware-default guardrails.
- Verified: direct MP cliff-tick hardware with `-DelaySeconds 8` and menu-chain
  MP cliff-tick hardware with `-DelaySeconds 8`.

## 2026-07-05 - Default MP Fall-Map Stage Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `91/92`, matching
  the already-proven mode `89/90` hardware path, then made the direct and
  menu-chain MP fall-map wrappers default to `-hwtri` targets with
  `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP fall-map stage
  pair is covered by the hardware-default guardrails.
- Verified: direct MP fall-map software baseline with `-DelaySeconds 8`,
  direct MP fall-map hardware with `-DelaySeconds 8`, and menu-chain MP
  fall-map hardware with `-DelaySeconds 8`; `check-gbi-decode-fixtures`,
  `check-harness-registry`, `check-docs`, `verify-dev-fast -Build
  -DelaySeconds 3`, and `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Fall-Landing Stage Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `93/94`, matching
  the already-proven mode `91/92` hardware path, then made the direct and
  menu-chain MP fall-landing wrappers default to `-hwtri` targets with
  `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP fall-landing
  stage pair is covered by the hardware-default guardrails.
- Verified: direct MP fall-landing software baseline with `-DelaySeconds 8`,
  direct MP fall-landing hardware with `-DelaySeconds 8`, and menu-chain MP
  fall-landing hardware with `-DelaySeconds 8`; `check-gbi-decode-fixtures`,
  `check-harness-registry`, `check-docs`, `verify-dev-fast -Build
  -DelaySeconds 3`, and `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Ceiling Stage Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `95/96`, matching
  the already-proven mode `93/94` hardware path, then made the direct and
  menu-chain MP ceiling wrappers default to `-hwtri` targets with
  `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP ceiling stage
  pair is covered by the hardware-default guardrails.
- Verified: direct MP ceiling software baseline with `-DelaySeconds 8`,
  direct MP ceiling hardware with `-DelaySeconds 8`, and menu-chain MP
  ceiling hardware with `-DelaySeconds 8`; `check-gbi-decode-fixtures`,
  `check-harness-registry`, `check-docs`, `verify-dev-fast -Build
  -DelaySeconds 3`, and `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Ceiling-Status Stage Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `97/98`,
  matching the already-proven mode `95/96` hardware path, then made the direct
  and menu-chain MP ceiling-status wrappers default to `-hwtri` targets with
  `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP
  ceiling-status stage pair is covered by the hardware-default guardrails.
- Verified: direct MP ceiling-status software baseline with `-DelaySeconds 8`,
  direct MP ceiling-status hardware with `-DelaySeconds 8`, and menu-chain MP
  ceiling-status hardware with `-DelaySeconds 8`; `check-gbi-decode-fixtures`,
  `check-harness-registry`, `check-docs`, `verify-dev-fast -Build
  -DelaySeconds 3`, and `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Catch Stage Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `99/100`,
  matching the already-proven mode `97/98` hardware path, then made the direct
  and menu-chain MP cliff-catch wrappers default to `-hwtri` targets with
  `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP cliff-catch
  stage pair is covered by the hardware-default guardrails.
- Verified: direct MP cliff-catch software baseline with `-DelaySeconds 8`,
  direct MP cliff-catch hardware with `-DelaySeconds 8`, and menu-chain MP
  cliff-catch hardware with `-DelaySeconds 8`; `check-gbi-decode-fixtures`,
  `check-harness-registry`, `check-docs`, `verify-dev-fast -Build
  -DelaySeconds 3`, and `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Wait Stage Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `101/102`,
  matching the already-proven mode `99/100` hardware path, then made the direct
  and menu-chain MP cliff-wait wrappers default to `-hwtri` targets with
  `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP cliff-wait
  stage pair is covered by the hardware-default guardrails.
- Verified: direct MP cliff-wait software baseline with `-DelaySeconds 8`,
  direct MP cliff-wait hardware with `-DelaySeconds 8`, and menu-chain MP
  cliff-wait hardware with `-DelaySeconds 8`; `check-gbi-decode-fixtures`,
  `check-harness-registry`, `check-docs`, `verify-dev-fast -Build
  -DelaySeconds 3`, and `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Attack Stage Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `103/104`,
  matching the already-proven mode `101/102` hardware path, then made the
  direct and menu-chain MP cliff-attack wrappers default to `-hwtri` targets
  with `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP cliff-attack
  stage pair is covered by the hardware-default guardrails.
- Verified: direct MP cliff-attack software baseline with `-DelaySeconds 8`,
  direct MP cliff-attack hardware with `-DelaySeconds 8`, and menu-chain MP
  cliff-attack hardware with `-DelaySeconds 8`; `check-gbi-decode-fixtures`,
  `check-harness-registry`, `check-docs`, `verify-dev-fast -Build
  -DelaySeconds 3`, and `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Attack Action Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `105/106`,
  matching the already-proven mode `103/104` hardware path, then made the
  direct and menu-chain MP cliff-attack action wrappers default to `-hwtri`
  targets with `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP cliff-attack
  action pair is covered by the hardware-default guardrails.
- Verified: direct MP cliff-attack action software baseline with
  `-DelaySeconds 8`, direct MP cliff-attack action hardware with
  `-DelaySeconds 8`, and menu-chain MP cliff-attack action hardware with
  `-DelaySeconds 8`; `check-gbi-decode-fixtures`, `check-harness-registry`,
  `check-docs`, `verify-dev-fast -Build -DelaySeconds 3`, and
  `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Common2 Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `107/108`,
  matching the adjacent cliff-attack-action hardware path, then made the
  direct and menu-chain MP cliff-common2 wrappers default to `-hwtri` targets
  with `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP cliff-common2
  pair is covered by the hardware-default guardrails.
- Verified: direct MP cliff-common2 software baseline with `-DelaySeconds 8`,
  direct MP cliff-common2 hardware with `-DelaySeconds 8`, and menu-chain MP
  cliff-common2 hardware with `-DelaySeconds 8`; `check-gbi-decode-fixtures`,
  `check-harness-registry`, `check-docs`, `verify-dev-fast -Build
  -DelaySeconds 3`, and `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Escape Action Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `109/110`,
  matching the adjacent cliff-common2 hardware path, then made the direct and
  menu-chain MP cliff-escape action wrappers default to `-hwtri` targets with
  `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP cliff-escape
  action pair is covered by the hardware-default guardrails.
- Verified: direct MP cliff-escape action software baseline with
  `-DelaySeconds 8`, direct MP cliff-escape action hardware with
  `-DelaySeconds 8`, and menu-chain MP cliff-escape action hardware with
  `-DelaySeconds 8`; `check-gbi-decode-fixtures`, `check-harness-registry`,
  `check-docs`, `verify-dev-fast -Build -DelaySeconds 3`, and
  `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Escape Common2 Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `111/112`,
  matching the adjacent cliff-escape-action hardware path, then made the direct
  and menu-chain MP cliff-escape common2 wrappers default to `-hwtri` targets
  with `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP cliff-escape
  common2 pair is covered by the hardware-default guardrails.
- Verified: direct MP cliff-escape common2 software baseline with
  `-DelaySeconds 8`, direct MP cliff-escape common2 hardware with
  `-DelaySeconds 8`, and menu-chain MP cliff-escape common2 hardware with
  `-DelaySeconds 8`; `check-gbi-decode-fixtures`, `check-harness-registry`,
  `check-docs`, `verify-dev-fast -Build -DelaySeconds 3`, and
  `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Climb Floor Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `113/114`,
  matching the adjacent cliff-escape-common2 hardware path, then made the
  direct and menu-chain MP cliff-climb floor wrappers default to `-hwtri`
  targets with `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry and GBI fixture checks so the MP cliff-climb
  floor pair is covered by the hardware-default guardrails.
- Verified: direct MP cliff-climb floor software baseline with
  `-DelaySeconds 8`, direct MP cliff-climb floor hardware with
  `-DelaySeconds 8`, and menu-chain MP cliff-climb floor hardware with
  `-DelaySeconds 8`; `check-gbi-decode-fixtures`, `check-harness-registry`,
  `check-docs`, `verify-dev-fast -Build -DelaySeconds 3`, and
  `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Climb Action Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `115/116` and
  made the direct/menu MP cliff-climb action wrappers default to `-hwtri`,
  with `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry, GBI fixture checks, and current-truth docs so
  the hardware-default stage coverage now runs through cliff-climb action.
- Verified: direct MP cliff-climb action software baseline with
  `-DelaySeconds 8`, direct MP cliff-climb action hardware with
  `-DelaySeconds 8`, and menu-chain MP cliff-climb action hardware with
  `-DelaySeconds 8`; `check-gbi-decode-fixtures`, `check-harness-registry`,
  `check-docs`, `git diff --check`, `verify-dev-fast -Build -DelaySeconds 3`,
  and `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Passive Proofs To Hardware

- Made the direct/menu MP Passive wrappers for modes `123/124` default to the
  `-hwtri` harness builds, with `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry, GBI fixture guardrails, and current-truth docs
  so hardware-default stage coverage now runs through MP Passive.
- Verified: direct MP Passive software baseline with `-SoftwarePreview
  -DelaySeconds 8`, direct MP Passive hardware with `-DelaySeconds 8`, and
  menu-chain MP Passive hardware with `-DelaySeconds 8`;
  `check-gbi-decode-fixtures`, `check-harness-registry`, `git diff --check`,
  `verify-dev-fast -Build -DelaySeconds 3`, and
  `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Wait Damage Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `121/122` and
  made the direct/menu MP cliff-wait damage wrappers default to `-hwtri`,
  with `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry, GBI fixture checks, and current-truth docs so
  the hardware-default stage coverage now runs through cliff-wait damage.
- Verified: direct MP cliff-wait damage software baseline with
  `-DelaySeconds 8`, direct MP cliff-wait damage hardware with
  `-DelaySeconds 8`, and menu-chain MP cliff-wait damage hardware with
  `-DelaySeconds 8`; `check-gbi-decode-fixtures`, `check-harness-registry`,
  `check-docs`, `git diff --check`, `verify-dev-fast -Build -DelaySeconds 3`,
  and `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Climb Finish Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `119/120` and
  made the direct/menu MP cliff-climb finish wrappers default to `-hwtri`,
  with `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry, GBI fixture checks, and current-truth docs so
  the hardware-default stage coverage now runs through cliff-climb finish.
- Verified: direct MP cliff-climb finish hardware with `-DelaySeconds 8`,
  menu-chain MP cliff-climb finish hardware with `-DelaySeconds 8`,
  `check-gbi-decode-fixtures`, `check-harness-registry`, `check-docs`,
  `git diff --check`, `verify-dev-fast -Build -DelaySeconds 3`, and
  `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Default MP Cliff-Climb Common2 Proofs To Hardware

- Extended the stage `gcDrawAll` hardware submit path to modes `117/118` and
  made the direct/menu MP cliff-climb common2 wrappers default to `-hwtri`,
  with `-SoftwarePreview` retained as the opt-out.
- Updated the harness registry, GBI fixture checks, and current-truth docs so
  the hardware-default stage coverage now runs through cliff-climb common2.
- Verified: direct MP cliff-climb common2 software baseline with
  `-DelaySeconds 8`, direct MP cliff-climb common2 hardware with
  `-DelaySeconds 8`, and menu-chain MP cliff-climb common2 hardware with
  `-DelaySeconds 8`; `check-gbi-decode-fixtures`, `check-harness-registry`,
  `check-docs`, `git diff --check`, `verify-dev-fast -Build -DelaySeconds 3`,
  and `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Fenced Weapon Manager Core Import

- Added default-off `NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER` support for the
  original `wpmanager`, `wpmain`, `wpmap`, `wpprocess`, and `wpdisplay` core
  TUs, with the local `wp*` seams fenced out when the import is enabled.
- Expanded the narrow weapon ABI enough for original manager allocation and
  empty-battle compilation while leaving projectile specials/effects for the
  next slice.
- Verified: fenced weapon-manager battle-playable build,
  `git diff --check`, `verify-dev-fast -Build -DelaySeconds 3`, and
  `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Fenced Mario Fireball Import

- Added default-off `NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL`, which forces the
  fenced weapon-manager core and imports original `ftmariospecialn.c` plus
  `wpmariofireball.c` through `src/import/battleship_mario_fireball.c`.
- Removed the inactive Mario neutral-special status stubs only when that fence
  is enabled, and added narrow declarations/effect no-ops needed by the
  original TUs.
- The fenced build still uses weak bridge stubs for heavy map adjustment,
  display-scale, and particle effects; natural B-input/fireball spawn, map
  rebound, and hit proof remain the next slice.
- Verified: fenced Mario fireball battle-playable build,
  `git diff --check`, `verify-dev-fast -Build -DelaySeconds 3`, and
  `verify-boundary -DelaySeconds 3`.

## 2026-07-05 - Fenced Mario Neutral-Special Input

- Added original `ftcommonspecialn.c` to the default-off Mario fireball fence,
  replacing the local deferred `ftCommonSpecialNCheckInterruptCommon` only when
  `NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL=1`.
- Kept unowned non-Mario neutral-special setters as weak no-ops inside the
  fence, so this compiles Mario's ground B-input path without pulling every
  character special TU.
- Verified: fenced Mario fireball battle-playable build,
  `git diff --check`, and `verify-dev-fast -Build -DelaySeconds 3`.

## 2026-07-05 - Fenced Mario Air Neutral-Special Input

- Added original `ftcommonspecialair.c` to the same default-off Mario fireball
  fence, replacing the local deferred `ftCommonSpecialAirCheckInterruptCommon`
  only when `NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL=1`.
- The source path is BattleShip's air B-input check at
  `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonspecialair.c:157`,
  neutral dispatch at `:183`-`:185`, and Mario's imported
  `ftMarioSpecialAirNSetStatus` at
  `decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariospecialn.c:111`.
  Non-Mario neutral and unowned air Hi/Lw setters remain weak inside the fence.
- Verified: fenced Mario fireball battle-playable build, `git diff --check`,
  `check-docs`, and `verify-dev-fast -Build -DelaySeconds 3`.

## 2026-07-05 - Fenced Fox Blaster Import

- Added default-off `NDS_IMPORT_BATTLESHIP_FOX_BLASTER`, which forces the
  fenced weapon-manager core and imports original `ftfoxspecialn.c` plus
  `wpfoxblaster.c` through `src/import/battleship_fox_blaster.c`.
- Moved the shared weak weapon-manager bridges out of the Mario fireball
  wrapper and into `src/import/battleship_wpmanager_core.c`, so Mario fireball
  and Fox blaster use the same fenced helper boundary.
- The source path is BattleShip's blaster spawn in
  `decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxspecialn.c:11`,
  repeat-shot interrupt at `:34`, and weapon creation in
  `decomp/BattleShip-main/decomp/src/wp/wpfox/wpfoxblaster.c:106`. Blaster
  glow remains a weak no-op until the effect-manager memory gate.
- Verified: fenced Fox blaster battle-playable build, fenced Mario fireball
  battle-playable build, and combined Mario+Fox projectile fenced
  battle-playable build; `git diff --check`, `check-docs`, and
  `verify-dev-fast -Build -DelaySeconds 3`.

## 2026-07-05 - Fenced Neutral Projectile Proof

- Split original common neutral-special input into
  `src/import/battleship_special_common.c` so Mario-only, Fox-only, and
  combined projectile fences all import `ftcommonspecialn.c` and
  `ftcommonspecialair.c` exactly once.
- Added Mario/Fox special animation payload IDs to the DS reloc manifest and
  natural battle-playable projectile diagnostics for B input, special status,
  spawn calls, live weapon metadata, callback destroy counts, and resident
  weapon frames.
- The source-backed proofs now show Mario fireball creation through
  `ftmariospecialn.c` -> `wpmariofireball.c` with immediate original
  hit-callback destroy, and Fox blaster creation through `ftfoxspecialn.c` ->
  `wpfoxblaster.c` with 27 observed live weapon frames. Effects, shield,
  reflector, rebound, and broader projectile victim-damage routes remain
  follow-up.
- Verified: `verify-battle-playable-harness.ps1 -ImportBattleShipFoxBlaster`,
  `verify-battle-playable-harness.ps1 -ImportBattleShipMarioFireball
  -ImportBattleShipFoxBlaster`, and
  `verify-battle-playable-harness.ps1 -ImportBattleShipMarioFireball` with
  `-DelaySeconds 3`.

## 2026-07-05 - Normal Moveset And Projectile Defaults

- Rebased and consolidated the parked normal-moveset branch, then graduated
  `NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET`, `NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER`,
  `NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL`, and
  `NDS_IMPORT_BATTLESHIP_FOX_BLASTER` to default.
- Mode `163` now proves movement, Fox Jab, tilts S3/Hi3/Lw3, charged S4,
  aerial/landing, Mario fireball, Fox blaster, guard, and a natural KO/rebirth
  cycle in one `battle_playable` scene. Current memory ledger:
  `head208224 reloc646352 stage202816 fighter234864 if208672 stale0/0`.
- Scoped stage gcDrawAll/collision/floor/MP routes to a stage-side
  original-manager smoke proof (`mask=0x24f`) plus DS 3D stage/fighter hardware
  submission. Full Wait/Walk/live-hit/combat ownership remains with the
  movement, boundary, and `battle_playable` modes.
- [coverage-reduced] Continuous player-driven grab/throw is still deferred;
  bounded throw helper coverage remains in modes `155/156`.
- Verified: direct/menu init-wait-dash ladder, `verify-boundary`,
  `verify-battle-playable-harness`, `build-verify-profile -Profile Regression
  -Force`, all four sharded Regression `-NoBuild` runs, and the four targeted
  stage gcDrawAll/collision front-door harnesses.

## 2026-07-05 - Proved Natural Grab/Throw On Battle Playable

- Paid the default-flip Regression debt first with a fresh Regression prebuild
  and all four sharded `-NoBuild` runs green, then kept the direct/menu
  init-wait-dash ladder and `verify-boundary` green.
- Extended mode `163` from the default normal-moveset aerial landing phase into
  the already-imported natural Catch -> CatchWait -> ThrowF path. The verifier
  now requires `NAT_MOVESET` mask `0x7ff`, grab/catchwait, throw, thrown,
  recovery, and a throw-specific victim damage delta.
- Current proof summary: `moveset=0x7ff phase=15`, `grab=3/1`,
  `throw=12/5/11`, `throwDmg=0->12`, `bplay=stock8->5 falls0->3`,
  `hud=dmg12/digits0x1020a stock9->6`, and memory ledger
  `head207900 reloc653968 stage202816 fighter242480 if208672 stale0/0`.

[source-corrected]

| Change | BattleShip citation |
|---|---|
| Mode `163` normal-moveset expectation raised from `0x7f` to `0x7ff`, adding Catch/CatchWait, ThrowF/ThrowB, ThrownCommon, and throw recovery. | `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncatch1.c:111-136`; `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncatch2.c:57-76`; `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonthrow.c:56-126`; `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonstatus.h:3347-3430` |
| Mode `163` now asserts throw victim damage increases during the natural throw phase. | `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonthrown2.c:129-180`; `decomp/BattleShip-main/decomp/src/ft/ftparam.c:1527-1537` |

[coverage-reduced]

None. The previous grab/throw ledger reduction is removed. Modes `155/156`
stay because they still prove bounded ThrowB, ThrownCommon callback/update/map,
release/update-stats, and dead-result cleanup marker stacks that mode `163`
does not assert.

- Verified: `verify-dev-fast -Build -DelaySeconds 3`, `verify-boundary
  -DelaySeconds 3`, `verify-battle-playable-harness -DelaySeconds 3`,
  `verify-all -Profile RegressionFast -Only battle_playable -DelaySeconds 3`,
  `verify-all -Profile Regression -Only battle_playable -DelaySeconds 3`,
  `check-harness-registry`, `check-docs`, and `git diff --check`.

## 2026-07-06 - Effect Manager And Fox Reflector Defaults

- Documented the effect-manager memory gate, grew the VSBattle taskman arena to
  `0x150000`, imported original `ef/efmanager.c` plus `efdisplay.c`, and staged
  `EFCommonEffects1/2/3` in the NitroFS/reloc map. Common particle script/texture
  banks remain non-resident.
- Imported Fox reflector through original `ftfoxspeciallw.c` on top of the live
  weapon/projectile path. Mode `163` now drives Mario fireball into Fox shine,
  proves original special-collision reflection, calls the weapon reflector
  callback, flips projectile velocity/owner, enters real `SpecialLwHit`, and
  clears `reflect_lr`.
- Graduated both defaults and deleted the local `efManagerInitEffects`,
  `ftFoxSpecialLwHitSetStatus`, `ftCommonSpecialLwCheckInterruptCommon`, and
  inactive Fox Lw status stubs. The current battle-playable ledger is
  `head240332 reloc747472 stage202816 fighter241280 if208672 stale0/0`.

- Verified: `build-verify-profile -Profile Regression -Force`, all four
  sharded Regression `-NoBuild` runs, `verify-dev-fast -Build -DelaySeconds 3`,
  `verify-boundary -NoBuild -DelaySeconds 3`,
  `verify-battle-playable-harness -NoBuild -DelaySeconds 45`,
  `check-harness-registry`, `check-docs`, and `git diff --check`.

## 2026-07-06 - Remaining Mario/Fox Special Defaults

- Imported original `ftmariospecialhi.c`, `ftmariospeciallw.c`,
  `ftfoxspecialhi.c`, and `ftcommonfallspecial.c` through `src/import`, then
  graduated `NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI`,
  `NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW`, and
  `NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI` to default as separate commits.
- The standing packed-data audit found no new packed/bitfield payload class in
  these three specials. They consume the existing manager-loaded motion/status
  payloads cited in `docs/SPECIALS_WEAPONS_SCOUT.md`.
- Mode `163` now proves natural controller-input Mario Super Jump Punch,
  Mario Tornado, and Fox Fire Fox on the original-manager runtime:
  `specials=0xfff phase=7`, `mhi=1/14/7/0/195`, `mlw=1/14/0 dust=1`,
  and `foxhi=1/12/17/9/12/0/61`.
- The fresh Regression prebuild exposed missing NitroFS staging for the
  already-referenced special animation O2Rs (`RELOC_OPEN_FAILS=44`). Added
  `FTMarioAnim138`-`140` and `FTFoxAnim139`-`147` to the packaged
  Mario/Fox fighter reloc set; no verifier expectation was relaxed.
- Repaired the forced parallel Regression prebuild worker path so JSON-decoded
  shard records are enumerated correctly and compiler warnings on stderr do not
  abort successful `make` jobs under `$ErrorActionPreference = 'Stop'`.
- Current mode-163 memory ledger after the special defaults:
  `head240332 reloc750528 stage202816 fighter244336 if208672 stale0/0`, keeping
  the 128 KiB reserve intact. The fall-special public reaction call remains a
  documented no-op until `ftpublic.c`/audio reaction behavior is imported
  (`ftcommonfallspecial.c:96`, `ftpublic.c:261`).
- Verified: fresh `build-verify-profile -Profile Regression -Force`, all four
  sharded Regression `-NoBuild` runs, direct/menu init-wait-dash ladder,
  `verify-boundary -NoBuild -DelaySeconds 3`,
  `verify-battle-playable-harness -NoBuild -DelaySeconds 45`,
  `check-harness-registry`, `check-docs`, and `git diff --check`.

## 2026-07-06 - Audio Asset Parser Default

- Added a DS-owned parse-only audio asset loader behind
  `NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS`, then graduated it to default. The
  Makefile stages `S1_music_sbk`, `B1_sounds1/2_ctl`, both multi-MB sample
  `.tbl` banks, and `fgm_unk`/`fgm_tbl`/`fgm_ucd` under NitroFS.
- The loader treats the O2R payloads as raw N64 big-endian data, not relocated
  byte-swapped data, matching BattleShip's `alSeqFileNew` / `alBnkfNew`
  patch-in-place source shape in `sys/audio.c`.
- Mode `163` now reports the default audio marker:
  `audio=seq47 bank1=1/42/117@32000 bank2=1/1/322@44100 fgm=100/464/695
  raw=4422960 resident=0 scratch=64416`. The `.tbl` sample banks are verified
  present on NitroFS but stay non-resident, preserving the 128 KiB arena
  reserve. Playback is still deferred.
- Verified: forced Regression prebuild work completed after the tool call
  timeout, all four sharded Regression `-NoBuild` runs, direct/menu
  init-wait-dash ladder, `verify-boundary -DelaySeconds 3`, default
  `verify-battle-playable-harness -DelaySeconds 3`, `check-harness-registry`,
  `check-docs`, and `git diff --check`.

## 2026-07-07 - Minimal Pupupu BGM Backend Default

- Added `scripts/render-audio-bgm-pupupu.py`, a repeatable converter that
  derives a DS-streamable PCM16LE Pupupu/Dream Land track from original O2R
  `S1_music_sbk` sequence 0 plus `B1_sounds1_ctl/tbl`, using the read-only
  BattleShip CSEQ, CTL, and VADPCM tools. Output:
  `assets/audio/bgm_pupupu_pcm16.raw`, 2,886,710 bytes at 22050 Hz, SHA-256
  `581191127a00c8ddbd4395cc00b5d4722bbeca734a0990e778a2ea5e9138effa`;
  `sox stat` reports RMS amplitude `0.078523`.
- Added a DS-owned one-track BGM backend for `syAudioPlayBGM`,
  `syAudioStopBGMAll`, `syAudioCheckBGMPlaying`, and `syAudioSetBGMVolume`,
  streaming the derived track from NitroFS through a 64 KiB buffer. The natural
  caller path is BattleShip battle entry `scvsbattle.c:217` ->
  `mpCollisionSetPlayBGM` (`mpcollision.c:4013-4020`), with VSBattle cleanup
  mirroring the original stop/check/reset flow at `scvsbattle.c:530-556`.
- Graduated `NDS_IMPORT_BATTLESHIP_AUDIO_BGM` to default. Mode `163` reports
  `bgm=track0 play=1 stop=1 chunks=34 read=2228224 resident=65536`; the memory
  reserve holds with `237836 - 65536 = 172300` bytes above the gate. This is a
  compatibility backend for exactly one track; FGM/voice and the original
  sequence-player import remain follow-up.
- Verified: corrected-stream `verify-dev-fast -Build -DelaySeconds 3`,
  `verify-boundary -DelaySeconds 3`, default `verify-battle-playable-harness
  -DelaySeconds 3`, fresh Regression prebuild launched with
  `build-verify-profile -Profile Regression -Force` (the wrapper exceeded the
  tool timeout, workers completed in the background), all four sharded
  Regression `-NoBuild` runs green, `check-harness-registry`, and `check-docs`.

## 2026-07-07 - Pupupu BGM Streamer Timing Hotfix

- Replaced the first BGM backend's chunk-restart path with one looped 64 KiB
  ring buffer and 32 KiB half-buffer refills. Each refill writes the half that
  elapsed playback has left behind, flushes that half for the ARM7 sound
  hardware, advances the file offset by exactly the bytes written, and wraps to
  the start of `assets/audio/bgm_pupupu_pcm16.raw` on exhaustion.
- Added `AUDIO_BGM` rate markers and verifier guards. The committed asset is
  unchanged (`581191127a00c8ddbd4395cc00b5d4722bbeca734a0990e778a2ea5e9138effa`);
  `scripts/render-audio-bgm-pupupu.py:22,283-284` define PCM16 mono at 22050 Hz,
  so the expected stream rate is `44100` B/s. Mode `163` now reports
  `bgm=track0 play=1 stop=1 refills=88 read=2949120 rate=44046 loop=1
  resident=65536` and rejects rates outside `42100..46100` B/s.
- Whole-track wrap is the interim loop behavior; original CSEQ loop-point
  extraction remains documented as future sequence-player work. Ear
  confirmation is still a user/manual check.
- Verified: `verify-battle-playable-harness -DelaySeconds 3`,
  `verify-dev-fast -Build -DelaySeconds 3`, `verify-boundary -DelaySeconds 3`,
  `verify-all -Profile RegressionFast -Only battle_playable -DelaySeconds 3`,
  `check-docs`, and `git diff --check`.

## 2026-07-07 - Realtime Battle Presentation

- Split mode `163` into explicit presentation modes. Harness builds pass
  `NDS_HARNESS_FAST_LOGIC=1` and keep the deep proof chain unthrottled; normal
  battle-playable builds run one `scVSBattleFuncUpdate`, one scene draw, and
  one `ndsPlatformEndFrame` vblank wait per frame. This follows the original
  scheduler/taskman retrace contract in `sys/scheduler.c:1038-1055,1249,1258-1263`
  and `sys/taskman.c:993-1018`.
- Re-anchored the Pupupu BGM stream to libnds CPU timer time instead of logic
  frames. The stream still uses the unchanged PCM16 asset
  `581191127a00c8ddbd4395cc00b5d4722bbeca734a0990e778a2ea5e9138effa`,
  with the `44100` B/s expectation from
  `scripts/render-audio-bgm-pupupu.py:22,283-284`. Mode `163` now reports
  `bgm=track0 play=1 stop=1 refills=32 read=1114112 rate=44099 loop=0 hwloop=0
  resident=65536` in fast verification, and realtime smoke reports
  `frames=600 fps=598/598 ticks=335878400`.
- Fixed a stale fighter AObj16/figatree heap alias in forced reloc loads. The
  failure manifested as Fox Fire Fox holding forever while audio hardware kept
  repeating the last ring-buffer slice; BattleShip `ftmain.c:4615-4624,4704,4751-4759`
  reloads the requested motion file into `fp->figatree_heap`, so the DS force
  path now reloads fighter AObj16 assets into the supplied heap instead of
  reusing an earlier heap pointer by file identity.
- Made the diagnostic taskman arena allocator tiered for smaller non-battle
  harnesses while preserving the full mode-163 battle arena assertion. This
  fixed startup/title Regression failures where a zero-sized arena reached the
  original `syTaskmanMalloc` path.
- Verified: `verify-dev-fast -Build -DelaySeconds 3`,
  `verify-boundary -DelaySeconds 3`,
  `verify-battle-playable-harness -RealtimePresentation -DelaySeconds 3`,
  fresh `build-verify-profile -Profile Regression -Force` (wrapper timed out
  while workers completed), all four sharded Regression `-NoBuild` runs green
  after rerunning shards `0` and `1` sequentially, plus the current
  `check-harness-registry` and `check-docs` gates for this slice.

## 2026-07-07 - Verification-Cost Infrastructure Checkpoint

- Replaced command-line-only build options with a generated per-build-dir
  `nds_build_config.h` dependency. The header carries harness id, fast/realtime
  presentation, hardware-triangle, live-input-preview, and graduated import
  flags, so stale object files can no longer survive a config change silently.
- Added `RegressionCore`, a six-target session gate: runtime, title,
  battle-playable realtime smoke, direct cliff-status, direct MP update, and
  menu-chain MP update. It passed no-build in `175.2s`. The originally requested
  boundary pair was not included because current `verify-boundary` is red on
  mode `161` (`NAT_MOTION=0,0,0x22f` after ~2358 frames); no proof expectation
  was relaxed for this infrastructure work.
- Added successful prebuild stamps at `artifacts/verifier-cost/prebuild-stamp.json`.
  A detached `RegressionCore` prebuild wrote a fresh six-target stamp in
  `595.93s`; `build-verify-profile.ps1 -Profile RegressionCore -VerifyStamp`
  validated it in `0.36s`.
- Added `-Detach` to `build-verify-profile.ps1` and transport-class shard retry
  handling in `verify-all.ps1`. Transport retries are limited to one retry for
  GDB/connect/zero-marker startup failures; marker mismatches still fail
  immediately.
- Verified for this checkpoint: `check-harness-registry`, detached
  `RegressionCore` prebuild plus `-VerifyStamp`, and
  `verify-all -Profile RegressionCore -NoBuild -DelaySeconds 3`. Final task
  gates are not complete: `verify-dev-fast -Build -DelaySeconds 3` and
  `verify-boundary -DelaySeconds 3` fail mode `161`, so no Lean snapshot was
  taken.

## 2026-07-07 - Config-Header Mode 161 Hotfix

- Fixed the mode `161` regression introduced by making `nds_build_config.h`
  visible to every TU. The broad seed-era `ftmanager.c` wrapper guard is
  deleted; it is not source behavior for every fighter creation path.
- Preserved the source-correct VSBattle fighter descriptor setup in
  `src/import/battleship_scvsbattle.c`: BattleShip
  `decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c:468` sets
  `desc.is_skip_entry = TRUE` before `ftManagerMakeFighter`, so the DS wrapper
  now does the same for VSBattle-created fighters. No mode `161/162` verifier
  expectation changed.
- Raised the fast battle-playable verifier wait floor from 25 to 30 seconds so
  the existing 3200-frame BGM rate/teardown guard reaches its natural stop
  marker with audio enabled. The audio marker contract and stream expectations
  are unchanged.
- Verified: `verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1
  -DelaySeconds 3`, `verify-dev-fast.ps1 -Build -DelaySeconds 3`,
  `verify-battle-playable-harness.ps1 -NoBuild -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`, detached
  `build-verify-profile.ps1 -Profile Regression -Detach` plus
  `-VerifyStamp` (`105` targets, `18203.607s`), and all four sharded
  Regression `-NoBuild -DelaySeconds 3` runs. Shard `1` first hit an
  all-zero/GDB-timeout transport failure and passed on the sequential rerun;
  no verifier expectation changed.

## 2026-07-08 - Split Scene Harness Build Config

- Split generated build configuration so per-mode harness state no longer
  invalidates every object in a shared build dir. `nds_build_config.h` now
  carries stable import/input/fast-logic/HW-triangle flags and stays
  force-included everywhere; `nds_scene_harness_config.h` carries only
  `NDS_DEV_SCENE_HARNESS` and `NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP` for
  scene-aware sources.
- Added `-Wundef` to the DS C flags and a hard
  `check-harness-registry.ps1` scan that fails if `src/` references the scene
  harness macros without including `nds_scene_harness_config.h`. `-Werror=undef`
  was tried first, but libnds/calico headers intentionally probe undefined
  `__ASSEMBLER__` and `__cplusplus`, so the compiler guard remains warning-only
  and the registry scan is the hard project-owned check.
- Recorded per-target timings in `artifacts/verifier-cost/prebuild-stamp.json`.
  Measured economics after the split: previous full Regression prebuild stamp
  was `18203.607s` for `105` targets; `RegressionCore -Force` measured
  `1893.130s` for `6` targets; a no-op `RegressionCore` rerun measured
  `39.377s`; a shared HW-tri mode switch rebuilt `scene_backend.c`,
  `scene_harness.c`, `battleship_grinishie_scale.c`, and relinked in
  `29.590s`.
- Canonical ROM investigation checkpoint: `src/port/taskman_seam.c:6415-6466`
  already selects realtime presentation and calls one present path per loop, so
  the blank top screen is not simply a missing draw call. Source inspection
  points at `src/port/reloc_backend_movement.c:9144-9155` gating
  battle-playable proof preparation to `NDS_HARNESS_FAST_LOGIC`, and the HW
  realtime presentation still routes through the stage-gcDrawAll proof helper
  instead of a canonical full-scene renderer path. No renderer or input fix was
  attempted in this infrastructure slice.
- Verified before the final full sweep: `verify-dev-fast.ps1 -Build
  -DelaySeconds 3`, `verify-boundary.ps1 -DelaySeconds 3`, detached
  `build-verify-profile.ps1 -Profile RegressionCore -Force` plus
  `-VerifyStamp`, no-op `RegressionCore` prebuild, the shared HW-tri mode-switch
  build, and `verify-all.ps1 -Profile RegressionCore -NoBuild -DelaySeconds 3`.
- Final sweep: detached `build-verify-profile.ps1 -Profile Regression -Force`
  wrote a valid 105-target stamp in `4773.933s`, down from the prior
  `18203.607s` full prebuild. Shards `0`, `2`, and `3` passed in the parallel
  `-NoBuild` run; shard `1` first hit a transport/all-zero startup failure and
  passed on the sequential rerun. No verifier expectation changed.
- Hidden the verifier child PowerShell launches in `verify-all.ps1` and
  `Start-VerifyRegressionShards.ps1` so redirected verifier/shard subprocesses
  no longer foreground `pwsh` windows during long sweeps.

## 2026-07-08 - Canonical HW Realtime Visibility Checkpoint

- Covered the exact user-facing battle-playable configuration:
  realtime presentation, `NDS_DEV_LIVE_INPUT_PREVIEW=1`, and
  `NDS_RENDERER_HW_TRIANGLES=1`. The verifier now builds
  `smash64ds-battle-playable-canonical-hwtri`, asserts live DS pad polling,
  scripted playback disabled, BGM hardware-timer byte rate, hardware frame
  flushes, textured Pupupu stage triangles, and selected fighter HW triangle
  submission.
- Fixed the blank/dead-input class by polling `ndsPlatformReadInput()` before
  each realtime battle update, presenting the debug HUD during realtime draws,
  and keeping the presentation marker live instead of waiting for the long
  manual loop to exit. BattleShip draw ownership remains the original
  `gcDrawAll` traversal (`decomp/.../sys/objman.c:2087`) reached through the
  scene draw loop (`decomp/.../sc/scmanager.c:1299`).
- Enabled textured no-oracle HW submission for the canonical path by preserving
  texture-state decoding and batching same-state DS triangle submissions. The
  focused smoke now reports `LIVE_PAD=72/72 connected=0x1 playback=0`,
  `PLATFORM_HW=65/65`, `STAGE_GCDRAWALL_HW=2772/12672 bind726 upload11
  ready6402 reject0`, and `STAGE_GCDRAWALL_HW_FTR=130/37830`.
- 60fps textured HW is not solved in this checkpoint. The measured realtime
  smoke is `frames=67 fps=59/59 ticks=376165888` (about 5.9fps), so the next
  renderer task is the cached draw-state replay/cutover. The strict 60fps
  assertion remains available as `-RequireRealtime60Fps`.

## 2026-07-08 - Canonical HW Pixel-Proof Gate

- Reproduced Tyler's playtest symptom with screenshots: both
  `smash64ds-battle-playable-canonical-hwtri.nds` and the shipped
  `smash64ds-battle-playable-hwtri.nds` initially showed only the top-screen
  clear color. A direct DS sentinel triangle proved the screen routing and 3D
  setup were functional, so the blank frame was in submitted scene geometry.
- Added hardware-side visibility markers to the canonical realtime verifier.
  `PLATFORM_HW` now reports submitted frames, flushes, pre-flush GX polygon RAM,
  pre-flush GX vertex RAM, `GFX_STATUS`, and `GFX_CONTROL`; the useful sample
  point is after the renderer batch closes and before `glFlush`, because
  post-flush/post-vblank reads were zero even for the visible sentinel.
- Fixed canonical no-oracle HW submission so it still computes transformed
  vertices for readiness and respects the source DL state's depth choice. The
  previous path forced every no-oracle triangle through the raw z-buffered
  route; Pupupu/fighter content that needed the projected-depth path could be
  submitted on the CPU side while producing no visible hardware polygons.
- Upgraded the permanent realtime gate with screenshot evidence. The current
  canonical smoke passes with `frames=59 fps=54/54 ticks=365969728`,
  `gxram=66/226`, `tri=415`, and
  `36551/49152` top-screen pixels different from clear color. The rebuilt
  shipped ROM `smash64ds-battle-playable-hwtri.nds` passes the same pixel
  assertion with `33595/49152` non-clear pixels.
- Fixed the realtime wrapper so `RegressionCore` actually runs the screenshot
  assertion instead of exiting after the nested marker verifier. Verified
  `RegressionCore -NoBuild -DelaySeconds 3` now prints and passes the pixel
  gate. A full detached Regression prebuild was started for the shared renderer
  sweep, then stopped at the user's request; no full Regression shards were run
  for this checkpoint.
- This checkpoint intentionally stops before renderer-cache/performance work.
  The frame is no longer blank, but visual fidelity is still wrong/overbright
  and the strict 60fps cache gate remains follow-up.

## 2026-07-08 - Canonical HW Dream Land Texture Visibility

- Validated the lighting/material hypothesis against the active renderer code:
  lit combiners that request `SHADE` must keep lit vertex shade even when the
  combine also references `PRIM`/`ENV`. The HW path now lets lit-SHADE win over
  material color, while material color remains the path for combiners that do
  not request `SHADE`. Source citations: BattleShip GBI
  `include/PR/gbi.h:2536-2543` for `gSPLightColor` offsets,
  `include/PR/gbi.h:2558-2562` for `gSPSetLights1` slot ordering,
  `sc/sccommon/scvsbattle.c:507-508` for battle lighting enable/draw, and
  `ft/ftdisplaylights.c:23-24` for submitted fighter light state.
- Fixed the light-slot handling to use decoded GBI slot order instead of
  brightness sorting: `LIGHT_1` is the first diffuse light and `LIGHT_2` is the
  one-light ambient slot from `gSPSetLights1`. The existing BattleShip fallback
  colors remain source-backed by `ft/ftdisplaymain.c:205-206`.
- Added `RENDER_TEXFMT` and `RENDER_COMBINE` markers to the canonical realtime
  verifier. Current canonical texture evidence is
  `conv0x100/bind0x100/pal0x100/rej0x0/why0x0`, proving CI texture conversion,
  texture bind, palette bind, and zero unexplained rejects for the visible
  scene. The combine marker records distinct combine words plus lit/material
  counts and the projected-submit fallback count.
- Ran the diagnostic projected-submit probe on a scratch branch only. The probe
  made fighters/platforms/background appear, proving those DLs reach the HW
  submitter and isolating the missing-pixel class to the raw DS matrix/z path.
  The probe was reverted; the landed fix uses the CPU-oracle projected-submit
  fallback for z-buffered triangles and gates that debt with
  `proj44330`/`RENDER_ORACLE=1080/0/0`.
- Hardened the screenshot gate from "some green exists" to green content,
  non-white/non-green scene detail, expected fighter-region pixels, bounded
  adjacent-frame delta, nonzero GX RAM, zero oracle mismatches, nonzero texture
  format/bind evidence, and zero texture reject reasons.
- Rebuilt the shipped HUD-off ROM:
  `smash64ds-battle-playable-hwtri.nds`. The canonical and shipped settled
  captures both pass with `44723/49152` non-clear pixels,
  `10301/49152` dominant-green pixels, `10239/49152` non-white/non-green
  pixels, and `968/5616` fighter-region pixels. The canonical smoke reports
  `frames=67 fps=35/35 ticks=639162944 gxram=375/1163`; renderer-cache 60fps
  work and raw matrix/depth fidelity remain follow-up.
- Verified: canonical realtime verifier with screenshot capture, shipped ROM
  screenshot assertion, `verify-dev-fast.ps1 -Build -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`, `build-verify-profile.ps1 -Profile
  RegressionCore`, and
  `verify-all.ps1 -Profile RegressionCore -NoBuild -DelaySeconds 3`.
  No full Regression sweep was run in-session; Tyler owns the daily overnight
  sweep.

## 2026-07-09 - Canonical HW per-tile and input checkpoint

- Added a per-tile renderer state array for all eight N64 RDP tiles and routed
  `gSPTexture` active-tile selection, `gDPSetTile`, and `gDPSetTileSize` through
  that state. The texture cache key now includes the active render tile's
  wrap/mask/shift state, and `LoadTile` sampling uses the load rectangle origin
  rather than the render tile's `SetTileSize` origin. Source reference:
  `decomp/BattleShip-main/decomp/include/PR/gbi.h` for `gSPTexture`,
  `gDPSetTile`, and `gDPSetTileSize` field layout.
- Added default-off `NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY` for local
  classification builds. The probe capture
  `artifacts/visibility/canonical-hwtri-textureonly.png` still showed the same
  white/misplaced object classes, so the remaining fidelity gap is not just
  material/lighting washing out otherwise-correct texels.
- Preserved decoded light color/direction state across persistent fighter DObj
  renderer stats. The copy was a field-list copy, not a whole-struct assignment.
- Completed the DS live-input bridge by mapping B/X/Y/L/R in
  `ndsPlatformReadInput`, keeping canonical live-preview builds from rearming
  scripted playback, and expanding the HUD/`LIVE_PAD` marker to show held keys,
  live pad0, original P0 controller state, and P0 root-x. A HUD debug ROM is
  built as `smash64ds-battle-playable-hwtri-inputhud.nds`; the normal playtest
  ROM remains `smash64ds-battle-playable-hwtri.nds`.
- Added canonical verifier markers for the loaded Dream Land wallpaper pointer
  and all-DObj fighter screen boxes. The wallpaper is still deferred SObj/2D-BG
  composition, and the fighter assembly/raw DS matrix path remains renderer
  debt. Latest canonical smoke is stable but still visually incomplete:
  `frames=66 fps=35/35 gxram=373/1153`, `tri=372`,
  `44723/49152` non-clear, `10301/49152` green,
  `10239/49152` detail, `968/5616` fighter-region pixels, and
  `0/49152` adjacent-frame delta.

## 2026-07-09 - Renderer fidelity fast iteration

- Wired HW texture binding directly to the active render tile for dimensions,
  wrap/mirror params, palette bank, cache-key rectangle, and submitted texcoord
  origin. Capture: `artifacts/visibility/2026-07-09_fixA_per_tile_bind.png`.
  The canonical detail gate moved from `10239/49152` to `12762/49152`, and
  fighter-region color moved from `968/5616` to `1058/5616`.
- Surfaced `RENDER_CLIP` from the existing HW vertex saturation counter. The
  canonical scene reports `clip=0`, so near-plane clipping was not evidence-
  backed for this slice and remains deferred until a scene shows saturation.
- Added `RENDER_TEXUSE` rejection-class markers for the five
  `ndsRendererHardwareUseTexture` false paths. The white-class evidence showed
  state-off/no-combine rejects, not TEXEL0 rejects. A strict implicit texture-on
  path now accepts only armed image/load/render-tile/tile-size state and uses the
  GBI 16-bit full-scale fallback when no `G_TEXTURE` scale was recorded.
  Capture: `artifacts/visibility/2026-07-09_fixC_implicit_texture_on.png`.
  Detail rose to `13763/49152`; the final HUD-on/off rebuilt ROM smoke reports
  `13766/49152` detail, `11619/49152` green, `1136/5616` fighter-region pixels,
  `texUse=0/214/26/0/0/impl57`, and `texFmt=conv0x100/bind0x100/pal0x100/rej0x0/why0x0`.
- Completed the realtime live-input controller bridge for the shipped config:
  after DS key scan, `NDS_DEV_LIVE_INPUT_PREVIEW=1` builds now call
  `syControllerReadDeviceData()` and `syControllerUpdateGlobalData()` so held
  keys flow through the original controller path into `gSYControllerDevices`.
  Rebuilt `smash64ds-battle-playable-hwtri.nds` and
  `smash64ds-battle-playable-hwtri-inputhud.nds`.
- Ratcheted the canonical screenshot detail floor to 25% by default. Final
  gates: `check-gbi-decode-fixtures.ps1`,
  `verify-battle-playable-realtime-harness.ps1 -DelaySeconds 3`,
  `verify-dev-fast.ps1 -Build -DelaySeconds 3`,
  `verify-boundary.ps1 -DelaySeconds 3`,
  `build-verify-profile.ps1 -Profile RegressionCore -Detach` +
  `-VerifyStamp`, `verify-all.ps1 -Profile RegressionCore -NoBuild
  -DelaySeconds 3`, `check-docs.ps1`, and `check-harness-registry.ps1`.
  Cache/perf and remaining fidelity stay next.

## 2026-07-09 - Renderer fidelity iter-4

- Fixed O2R texture byte-lane decoding behind explicit renderer data-layout
  state. O2R byte-packed texture data now maps logical bytes with
  `logical_index ^ 3`, packed-nibble data maps the containing byte with
  `(logical_texel_index >> 1) ^ 3`, halfword data maps with
  `logical_index ^ 1`, and RGBA32 remains native. The texture cache key now
  includes the source layout and validates the physical lane-mapped span.
- Added fixtures and runtime markers for the lane contract. Focused gates:
  `check-gbi-decode-fixtures.ps1`,
  `verify-battle-mariofox-dl-draw-all-harness.ps1 -HardwareTriangles`,
  `verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -HardwareTriangles`,
  and `verify-battle-playable-realtime-harness.ps1 -SkipScreenshot`.
  Canonical realtime reports `RENDER_TEXLANE=layout0x2/byte290/half290`.
- Preserved texture/tile/segment state across sibling stage DObj display-list
  heads within one enclosing stage GObj traversal. The focused stage gate
  reports `carry=42/42/tex32/tile34/short6/6/seg3`; canonical realtime reports
  `stageCarry=2646/2646/tex2016/tile2142/short378/378/seg189` with zero sampled
  texture uploads after increasing the fixed HW texture cache to 64 entries.
- Capture evidence:
  `artifacts/visibility/2026-07-09_iter4_stagecarry_after_late.png` shows
  recognizable Dream Land with visible fighters, `42335/49152` non-clear
  pixels, `22557/49152` green pixels, `19640/49152` detail pixels, and
  `0/49152` adjacent-frame delta. Remaining renderer debt is raw DS
  matrix/depth, fighter assembly, exact texture placement, and cached 60fps
  present.
