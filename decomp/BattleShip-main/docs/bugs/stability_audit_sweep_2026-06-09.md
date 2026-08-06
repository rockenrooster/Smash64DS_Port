# Full-Codebase Stability Audit Sweep — 2026-06-09

Seven parallel subsystem audits (port core glue, coroutines, audio+bridges,
Android, GUI/hires/enhancements, decomp known-bug-class sweep, libultraship
fork) covering all four platforms (Windows, macOS, Linux, Android). Every
finding was independently re-verified against the code before patching; the
decomp sweep across all 7 known bug classes (LP64 stride, reloc-token misuse,
unsized enum-indexed arrays, overlay BSS leaks, missing portFixupSprite,
u8-pair halfswap order, BE bitfield positional inits) found **zero new
instances** — all existing mitigations held through the competitive-ruleset /
JP / widescreen / FD-CSS / Android commits.

## Fixes landed (by subsystem)

### libultraship (submodule, branch `ssb64`)

| Severity | File | Fix |
|---|---|---|
| critical | `src/fast/backends/gfx_metal.cpp` | `DrawTriangles` had no bounds check on `mCurrentVertexBufferOffset` before the `memcpy` into the per-frame vertex pool slot (fixed 4.9 MB). A frame with enough draw flushes silently overflowed into adjacent heap/driver memory. Now checks capacity and drops the draw with an SPDLOG_ERROR (one visually-wrong frame instead of corruption). macOS only. |
| high | `src/fast/backends/gfx_metal.cpp` | Every depth-test/mask change created a `MTLDepthStencilState` via `newDepthStencilState` (+1 retain) and never released it after `setDepthStencilState` (which retains internally) — one GPU object leaked per depth-mode change, and SSB64 toggles depth constantly (2D sprites vs 3D). Added the missing `release()`. |
| high | `src/fast/interpreter.cpp` | All 12 `ImportTexture*` replacement paths dereferenced `mMaskedTextures.find(...)->second` with no `end()` check — UB/SIGSEGV if the registry entry was unregistered or named differently than the resource path. Extracted a guarded `GetMaskedReplacementData` helper returning nullptr on miss; every caller already null-checks `addr` and falls back. |
| high | `src/ship/audio/WasapiAudioPlayer.{h,cpp}` | `DoInit` registered `this` as an `IMMNotificationClient` but nothing ever unregistered it; the class had no destructor and `AudioPlayer::~AudioPlayer` doesn't call `DoClose`. Windows kept delivering device-change callbacks to a dangling `this` after shutdown (UAF on audio-device plug/unplug at exit). Added a destructor that unregisters + closes. |
| perf | `include/fast/interpreter.h` | `TextureCacheKey::Hasher` hashed only `texture_addr`, ignoring the other 8 fields `operator==` compares. The game's bump heaps cycle many distinct textures through the same addresses, so the 1024-entry cache degraded toward per-bucket linear scans on every `TextureCacheLookup`. Replaced with FNV-1a over every compared field. |

Verified non-issues: the packed-DL cache is already evicted by
`lbreloc_bridge.cpp` on heap reclaim + scene reset (growth bounded by live
files); `ControlDeck` shutdown ordering remains correct.

### Coroutines (`port/coroutine_*.{cpp,S}`)

- **Guard pages (macOS/Linux/Android)**: coroutine stacks were plain
  `malloc`/`posix_memalign` blocks — a stack overflow silently corrupted
  adjacent heap. Stacks are now `mmap`ed with a `PROT_NONE` page at the low
  (overflow) end, converting overflow into an immediate SIGSEGV with a
  useful `fault_addr` via the existing crash handler. (Win32 fibers already
  get a guard page from `CreateFiber`.)
- **Destroy-while-running guards**: POSIX and Win32 `port_coroutine_destroy`
  now abort with a message if called on the executing coroutine (Android
  already had this). On POSIX that was a straight UAF; on Win32,
  `DeleteFiber` on the running fiber kills the thread.
- **`thread_local` current-coroutine pointer** on POSIX/Win32 to match the
  Android backend (future-proofing; all backends are single-thread today).
- macOS: `_DARWIN_C_SOURCE` added next to `_XOPEN_SOURCE` (strict-POSIX mode
  hides `MAP_ANONYMOUS`).
- Verified clean: the aarch64 asm callee-saved set, alignment, FP chain,
  TLS, the noreturn trampoline, and the GObj thread create/destroy cycle.

### Port glue

- `port_watchdog.cpp`: `WalkFPChain` (crash-handler FP walker) dereferenced
  frame pointers with only an alignment check — a corrupt chain re-faulted
  inside the handler with the signal blocked, so the kernel killed the
  process with no log output. Added a `write(fd, addr, len)`-to-`/dev/null`
  readability probe (async-signal-safe EFAULT check) per frame record.
- `port_save.cpp`: zero-pad loop ignored `fwrite` failures (full disk) and
  fell through to a misleading partial state; now fails fast with a log.
- `first_run.cpp`: `QuoteCommandArg` double-quoted args for POSIX `sh -c`,
  leaving `$`, backticks, and backslashes live in paths fed to
  `std::system()` (silently wrong torch workdir for e.g. `$` in `$HOME`).
  Now single-quotes on POSIX; Windows keeps double-quoting (CreateProcessA
  argv rules).
- `port_aobj_fixup.cpp`: `sHalfswappedRanges` accumulated duplicate `{base,
  end}` entries on every figatree heap reload (bump-reset heaps reuse
  addresses) and is linearly scanned on the per-joint hot path; now updates
  in place on matching base.
- `resource/RelocPointerTable.cpp`: classic `realloc` misuse — return
  assigned directly to `sSlots`, so OOM nulled the live table and the
  following `memset(NULL+…)` was UB. Now checked (abort with log; the table
  is load-bearing).
- Deleted dead root-level `os.cpp` — it duplicated `osContGetReadData` et
  al. with pre-bugfix bodies and was one stray CMake glob away from
  re-introducing the OSContPad overrun.
- `enhancements/Updater.cpp` (Windows): updater wrote `update_game.bat` /
  `update_temp.zip` to the process CWD (Start-Menu launches give a profile
  dir) and extracted there. Whole flow now anchored to the executable's
  directory; download URLs are validated (https + no quote/space/control
  chars) before being spliced into the curl command line.

### Audio bridge

- `bridge/audio_bridge.cpp` `parseBook`: `order`/`npred` from CTL data
  multiplied into an `s32` with no validation — corrupt/foreign data could
  overflow → negative bookLen → tiny alloc → heap overrun in the copy loop.
  Now bounds-checked (legal N64 ADPCM: order ≤ 8, npred ≤ 8).
- BGM sequence buffer allocation now loops over the player count instead of
  hardcoding slot 0 (latent NULL-deref if `SYAUDIO_BGMPLAYERS_NUM` grows).
- **False positives worth recording**: two "HIGH races" reported between the
  game thread and "audio thread" (`portAudioPurgeFGMs` list walk, BGM status
  flags) are not real — `n_alAudioFrame` runs in the audio *coroutine* on
  the same host thread as the game coroutine (cooperative scheduler); the
  only real thread boundary is LUS's own ring buffer. Similarly,
  `portAudioPushSilence` already clamps `byteLen` to `sizeof(sSilenceBuf)`.

### Hires / GUI / CSS

- `hires/HiResPack.cpp`: `SourceDumpDedupId` had an operator-precedence bug
  (`|` binds looser than `^`), so fmt/siz never mixed into the low word —
  dump-mode dedup collisions silently skipped source dumps. Parenthesized.
- `hires/HiResHook.cpp`: CRC table init used a racy non-atomic `bool`;
  replaced with a magic-static lambda-initialized table.
- `gui/Menu.cpp`: `RemoveSidebarSearch` could compute `size()-1` on an empty
  vector (wraps to SIZE_MAX → uncaught `std::out_of_range` mid-ImGui-frame);
  search path concatenated a possibly-null `tooltip` into `std::string` (UB).
  Both guarded.
- `css_icons/port_css_stage_assets.cpp`: `w*h` as `int` in
  `convertToRGBA16BE` (latent overflow for large future assets) → `size_t`.
- Verified non-issue: `HiResPack::Lookup` thread-safety — the Fast3D
  interpreter runs synchronously inside `osSpTaskStartGo` on the single
  cooperative-scheduler thread, and `Init()` completes before the game loop
  starts; the GUI only reads stats written once at startup.

### Android

- **`port/android_touch_overlay.cpp` (UAF across relaunch)**: `sJoystick` /
  `sAttachAttempted` are process-lifetime statics, but Android keeps the
  process alive across Activity relaunches while `SDL_Quit` frees every
  joystick. The next session's first touch hit a freed `SDL_Joystick*`.
  Added `port_touch_overlay_shutdown()` (close/detach/reset) called from
  `PortShutdown` while SDL is still up.
- **`port/android_torch_bridge.cpp` (64 MB leak)**: extraction leaked the
  `Companion` singleton — including the full ROM `std::vector` — into the
  game process (Boot and game Activities share one process). Now deleted
  after `Init` returns; `gRomData` is an instance member so this frees it.
- **Manifest**: `density|fontScale` added to `configChanges`
  (`resizeableActivity="true"` + API 30 multi-window density changes were
  recreating the Activity mid-frame — the exact crash the list exists to
  prevent).
- **`AnalogStickView`**: `ACTION_CANCEL` went through the pointer-ID check
  using `getActionIndex()` (always 0 for CANCEL) — tracking a secondary
  pointer left the stick stuck at its last deflection through incoming
  calls/home swipes. CANCEL now resets unconditionally.
- **`TouchOverlay` lifecycle leaks**: the `InputDeviceListener` was never
  unregistered and the 100 ms menu-visibility poller never cancelled — both
  hold the full overlay View hierarchy across Activity recreations. Added
  `TouchOverlay.uninstall()` wired to `BattleShipActivity.onDestroy`.
- **`BootActivity`**: SAF-staged ROM copy (`cacheDir/user_rom.z64`, ~64 MB)
  is now deleted after extraction — the picker UI already promised "the ROM
  is discarded". Dev-shortcut ROM paths are untouched.
- **`port.cpp`**: `SDL_HINT_DISPLAY_USABLE_BOUNDS` was hardcoded
  `0,0,1920,1080` (wrong on the ultrawide devices commit `f2684d2c`
  targets); now queried from `SDL_GetDisplayUsableBounds` on the JVM-attached
  SDL_main thread before the hint is set, with the 1080p string as fallback.

## Verification

- `cmake --build build-us --target ssb64 -j 4` clean after each phase.
- 12-second boot smoke test: archives load, audio banks parse through the
  new `parseBook` validation, Metal renders with the new bounds guard, SIGTERM
  shutdown runs the full destructor chain cleanly.
- Windows/Android-only changes (WASAPI destructor, fiber guard, manifest,
  Java) are compile-verified by inspection only — covered by the next CI
  release build.

## Audit hooks for future sessions

- Any `find(...)->second` on a map whose entries can be unregistered is the
  masked-texture class; grep for `->second.` next to `.find(` in fork code.
- Objective-C-style Metal objects from `new*` factory methods need a
  matching `release()` even when an encoder/state-setter retains them.
- Process-lifetime native statics + Android Activity relaunch = stale-handle
  UAF; anything caching an SDL object must reset in `PortShutdown`.
- The audio "thread" is a coroutine — do not add mutexes to game/audio
  shared state without first checking the cooperative-scheduler topology.
