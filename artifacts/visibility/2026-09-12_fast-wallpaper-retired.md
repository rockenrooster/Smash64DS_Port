# Fast-wallpaper seed retirement — 2026-09-12

Scope: retire the obsolete software wallpaper seed path and its verifier/build
selector while preserving the native BG2 wallpaper owner.

## Dead-path proof

- `git log -S` on `ndsPlatformFastWallpaperCanSeed`, `BeginSeed`, `FinishSeed`,
  `QueueTransform`, `RecordSoftwareDraw`, and `Reset` identifies the 2026-09-08
  native-wallpaper conversion at `9a56d104780` among the relevant history.
- `9a56d104780` removed the software seed/capture callers from
  `src/port/sprite_preview_backend.c`. The one surviving caller was
  `ndsPlatformFastWallpaperReset()` in `ndsSObjPreviewBeginFrame()`.
- That surviving reset was not seed work: it only restored the shared BG2 affine.
  The current native owner already owns that transition. Battle taskman exit calls
  `ndsNativeWallpaperInvalidate()`, which queues `(256,256,0,0)` through
  `ndsPlatformQueueNativeWallpaperAffine()`. BG2 clear/layer-mask transitions also
  queue the same native identity. The orchestrator therefore authorized deleting
  the legacy reset owner and its sprite-preview call together with the seed path.
- Pre-edit `rg` over `src`, `include`, and `scripts` found no runtime caller of the
  seed/capture/transform APIs after the 2026-09-08 conversion; remaining references
  were declarations/definitions, stale verifier/probe text, and the reset described
  above.

## Changes

- Package-specific tracked deletions: **936 lines**. Current working-tree diff has
  944 deletions because 8 deletions in `Makefile`/the two verifier wrappers were
  already present from concurrent work and were preserved.
- Deleted lines by package surface: `Makefile` 20; `include/nds/nds_platform.h`
  39; `src/nds/nds_platform.c` 614; `src/port/sprite_preview_backend.c` 12;
  main realtime harness 99; P1 harness 5; static checkers/host tests 52; obsolete
  `scripts/probe-dreamland-wallpaper-seed.ps1` 95.
- Removed the state enum, seed/latest buffers, seed/capture/transform API, all
  `gNdsFastWallpaper*` counters, profile macro/state machine/stubs, the legacy
  affine reset owner, every `NDS_FAST_WALLPAPER_AFFINE` Makefile/config/benchmark
  selector, verifier parameter/identity/GDB/regex/assertion/JSON plumbing, and the
  frozen-P1 selector forwarding.
- `scripts/README.md` had no row for the deleted probe, so it required no edit.
- The native wallpaper queue/commit path and original-sprite final-layer commit
  remain. `test_sprite_scratch_lifetime.py` now treats the old software scratch as
  host-reference-only, matching the native-only ROM source split.
- `check-melonds-policy.ps1` now ignores recursive traversal errors from unreadable
  ignored cache directories while still auditing every readable `.ps1` file.

## Verification

- PASS `pwsh scripts/check-one-minute-match-verifier.ps1`.
- PASS `pwsh scripts/check-gbi-decode-fixtures.ps1` (with host GCC on PATH).
- PASS `python scripts/stages/test_native_wallpaper_platform.py`.
- PASS `python scripts/stages/test_native_wallpaper_runtime.py` (16 tests; host GCC).
- PASS `python scripts/menus/test_sprite_scratch_lifetime.py` (4 tests; host GCC).
- PASS `python scripts/fighters/test_stage_actor_scene_capture.py`.
- PASS `pwsh scripts/check-melonds-policy.ps1`.
- PASS `pwsh scripts/check-docs.ps1`.
- PASS `python scripts/check-untracked-dependencies.py`.
- PASS `pwsh scripts/check-architecture.ps1` (two pre-existing warnings only).
- PASS `pwsh scripts/verify-all.ps1 -Profile Boundary -List`: unchanged three arms
  `p2_shell_loop`, `p2_battle_realtime`, `p2_fourcpu_stress`.
- Final `rg` finds the retired selector only at the two explicitly off-limits
  defaults in `src/port/reloc_backend_movement.c:54-55`; no seed API/counter or
  `FAST_WALLPAPER` verifier marker remains elsewhere in `src/include/scripts/Makefile`.
- `git diff --check` passes for the package paths.

## Build identity

- Shared lane: `ACQUIRED codex-fastwall` before build and `RELEASED codex-fastwall`
  immediately after fresh ELF/ROM completion.
- Build: `make TARGET=smash64ds-p2-shell-hwtri BUILD=build-p2-shell` succeeded after
  putting devkitPro MSYS first on PATH. The first attempt exited before compilation
  because recursive make was launched under Git Bash; the lane was released before
  correcting PATH and reacquiring it.
- ROM: `builds/build-p2-shell/smash64ds-p2-shell-hwtri.nds`, 53,820,416 bytes,
  SHA-256 `D9944FD7D2D9C43B4D6C4AA5E872CFFF58B268272BBFDA7CAAA291870912EA47`.
- ELF: `builds/build-p2-shell/smash64ds-p2-shell-hwtri.elf`, 16,169,728 bytes,
  SHA-256 `557306F03B4A91AB209D47B27A32C48061F7C4571D08052C5E0392987A35E8C2`.
- `arm-none-eabi-nm` reports no `gNdsFastWallpaper` symbol, and generated
  `nds_build_config.h` contains no `NDS_FAST_WALLPAPER_AFFINE` define.

## Known gap outside this package

`src/port/reloc_backend_movement.c` still owns the off-limits two-line default for
`NDS_FAST_WALLPAPER_AFFINE`; the orchestrator must remove it separately.
Boundary runtime was intentionally not run; the orchestrator owns that run.

## Orchestrator follow-up (2026-09-13 00:50)

- `src/port/reloc_backend_movement.c` no longer carries the `NDS_FAST_WALLPAPER_AFFINE`
  default; no reference to the flag or the `gNdsFastWallpaper*` symbols remains in
  `src`, `include`, `scripts` or the Makefile (stale `__pycache__` bytecode only).
- Review fixes: `verify-battle-playable-one-minute-match.ps1` and
  `verify-battle-playable-realtime-harness.ps1` no longer pass the deleted
  `-FastWallpaperAffineMode`; `check-one-minute-match-verifier.ps1` now fails if it
  reappears; the realtime harness's wallpaper-cache clause asserts the ROM-side truth
  (no software compose, no host-only decode cache) instead of the retired 300x220
  seed; `check-melonds-policy.ps1` sweeps are fatal again; the two dangling
  "Device-proven" Makefile comments are gone.

## Orchestrator follow-up 2 (2026-09-13 05:35)

- The realtime harness's `SOBJ_WALL_CACHE`, `SOBJ_WALL_FINAL` and `SOBJ_WALL_BASE`
  printfs read thirteen host-reference-only counters (software decode cache,
  final-layer direct/skip/key/pixel, staging clears) that `--gc-sections` drops
  from every ROM; gdb printed stale addresses (`3934256344`). They now print only
  the eight ROM-side counters (SObj fast draws/ticks, BG2/BG3 clear/copy/final
  bytes), the dead Cut-G retained-wallpaper arm is gone, and the else branch
  asserts fast draws > 0, no BG2 clears/copies and no BG3 final writes inside the
  battle window, whole-layer BG2 final writes, BG3 clears <= 128 KiB and BG3 copies
  bounded per fast draw. The entry-shield palette prepares also count apart now
  (`gNdsEntryEffectNativeShieldPrepareCount`, `ENTRY_NATIVE` fifth field).
