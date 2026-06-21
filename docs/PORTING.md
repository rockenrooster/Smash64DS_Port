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
