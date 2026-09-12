# Architecture

## Goal

```text
BattleShip source + Nintendo DS backend = playable port
```

`decomp/` is read-only. Keep competitive original code; generated or manually
specialized DS implementations are equally valid when mechanically equivalent.
The port owns platform services, native rendering, audio, input, memory,
relocation and narrow ABI integration.

## Fidelity Boundary

`PROJECT_GOAL.md` owns fidelity and allowed adaptations. Exact equality applies
only to a named guaranteed quantity, not every numeric value or pixel. Do not
create a separate likeness percentage or automatic approximation allowance here.

## Ownership

| Surface | Owner |
|---|---|
| `decomp/BattleShip-main/decomp` | Read-only gameplay/source reference |
| `decomp/sm64-nds` | Read-only DS backend reference for a rough port |
| `decomp/sm64ds-decomp` | Read-only DS backend reference for an official Nintendo port implementation |
| `src/import` | Coherent original translation-unit imports |
| `src/nds` | libnds hardware/backend implementation |
| `src/port` | Platform-neutral seams, diagnostics, reloc/task integration |
| `include` | Minimal BattleShip-compatible declarations and DS APIs |
| `scripts` | Builds, focused checks, captures, verifiers |
| `assets` | Source-derived DS-ready payloads |
| `builds` | Generated lab/build output |

Keep coherent source imports in `src/import`, DS/backend implementations in
`src/nds` or `src/port`, and compatibility declarations in `include`. A seam may
call competitive original code or a mechanically equivalent native specialization.

## ROM Topology

Exactly two root ROMs are published:

- `smash64ds.nds`: original launch path.
- `smash64ds-battle-playable-hwtri.nds`: P1 battle path.

All diagnostics and experiments use non-published targets under `builds/`.
Canonical battle is harness mode `163`, `battle_playable_realtime`. Renderer
implementation selectors are internal laboratory controls, not game modes.

## Runtime Flow

Normal launch follows original startup/opening/title scene dispatch. P1 enters
the original VSBattle setup with Mario, level-3 Fox, Dream Land, items off, and
the one-minute Time rule. BattleShip owns fighter creation, state, animation,
collision, camera, rules, timer, scoring, KO/rebirth, Time Up, and Results.

The DS task loop supplies:

1. controller samples;
2. original update/process scheduling;
3. original display traversal captured by the DS renderer seam;
4. GX/OAM/BG submission;
5. audio refill and VBlank pacing.

Harness-only setup may select a scene or runtime option, but it must not script
combat or replace natural source state.

## Source Imports And ABI

Runtime-first work imports the smallest coherent original subsystem group that
can run naturally. Compatibility headers expose only required layout and symbols.
When a TU group is live, remove the proof seam or inactive duplicate instead of
maintaining two behaviors.

Relocation code validates file, symbol, asset, and generation provenance before
returning DS-native records. Unknown or nonresident content fails closed. Broad
weak stubs are temporary boundaries and stay listed in `KNOWN_ISSUES.md`.

## Scheduling, Input, And Timing

BattleShip object/process order remains authoritative. The port's coroutine and
taskman seams map original scheduling to the single ARM9 runtime. The original
60 Hz simulation is preferred, not sacred; a compensated lower-rate
implementation must prove mechanically equivalent behavior under the goal
contract. DS VBlank remains presentation pacing.

`osGetTime` and `osGetCount` use shared libnds CPU timers 0/1. Other DS code must
not claim those timers. Live DS buttons and stick values feed the original input
path. Wait/3/2/1/GO owns the exact control lock and timer start.

## Renderer

The renderer consumes original display traversal and maps supported N64 state to
DS BG, OAM, and GX hardware.

### 2D

- Cut G M1 keeps one complete source Dream Land wallpaper seed in 256x192 BG2.
- Live `grWallpaperCalcPersp` state updates native affine registers.
- Countdown source assets are decoded once from O2R with format-specific
  `SP_TEXSHUF` inversion: odd rows use `x^8` for 4-bit, `x^4` for 8-bit, and
  `x^2` for both RGBA16 and RGBA32. A comb pattern is a decode failure, not a
  filtering problem.
- Big GO uses prepare-time premultiplied resampling into direct RGB555+A1 OAM;
  no runtime palette quantization, subpixel placement, or antialiasing remains.
- The traffic housing/dim lamps use one opaque shaded A3I5 hardware atlas. Only
  the source flare/core/contour uses graded A5I3 alpha, queued after the housing.
- FPS, timer, fighter identity, stock, and damage are change-driven lower-screen
  text. Gameplay presentation remains on the top screen.

### 3D

Required architecture, not a claim that every path has finished migration:

- `PROJECT_GOAL.md` Native Rendering governs all target builds; reference
  interpretation remains host-side.
- Native owners validate complete topology/state before GX mutation. Unsupported
  required content needs a native implementation, not replay or hidden emission.
- Typed live transforms/materials may share native CPU and GX kernels. Preserve
  observable effect/weapon ordering.

`src/nds/nds_renderer.c` identifies the component includes. Runtime 1 owner plans
and `archive/Smash64DS_Runtime2_SwitchPlan.md` are history, not live policy.
`P2_PLAN.md` owns phase intent; the board owns current state and budgets.

### Textures

The P1 scene manifest converts source textures offline, assigns stable keys, and
preloads exactly 131,072 bytes into VRAM A before GO. Gameplay may bind resident
keys but may not convert, decode, allocate, read files, create/upload/delete GL
resources, evict, or refresh. Results may establish a new prewarm boundary.

The retired animated tiled-water asset, generator, residency path, draw path,
checks, and build selector are deleted.

## Audio

BattleShip chooses music and FGM IDs. ARM9 maps original requests to the DS audio
backend; ARM7 owns sample playback, while ARM9 performs the BGM file reads,
ring-buffer refill, and cache maintenance synchronously in the update path
(nds_audio_bgm.c fread + DC_FlushRange) — a known hardware-sensitive owner on
flashcart media. Host muting never disables ROM audio state or counters. Source pitch, required voices, and audible mixed output remain release
gates where listed on the P1 board.

## Memory

Mode 163 uses a fixed battle arena and must retain at least 128 KiB measured
reserve after the resident BGM buffer adjustment. Production owners use fixed
workspaces and no per-frame heap allocation. VRAM bank ownership, prepared bytes,
arena high-water, stack, and teardown are verifier-visible.

The N64's fixed framebuffer addresses and overlay model are not safe on DS.
Asset/overlay work must use explicit DS storage, validated lifetimes, and measured
reserve rather than guessed caching or DMA.

## Results And Scene Teardown

Original Time Up dispatches through battle teardown into VS Results. Scene exit
must release or invalidate battle-owned renderer, texture, audio, reloc, and arena
state exactly once. A verifier pass requires the original scene transition plus
zero stale pointers, safety faults, and fence violations.

## Compatibility Policy

- Add only ABI required by an imported source path.
- Keep enum values, field offsets, and callback signatures aligned with
  BattleShip.
- Do not globally include BattleShip's N64 libc headers; they conflict with
  devkitARM/libnds.
- Fix the shared seam once rather than patching every caller.
- A compile-only symbol or weak stub is not runtime completion.

## Large Backend File Split Plan

The retained mode-163 source currently compiles through a large amalgamated
scene backend, so one slice edit can rebuild unrelated legacy code. First delete
superseded diagnostic modes. Then move only retained runtime imports into normal
translation units without changing symbols or behavior. Split large compatibility
headers by existing subsystem owner when touched; do not perform a speculative
whole-tree refactor before the measured build bottleneck requires it.
