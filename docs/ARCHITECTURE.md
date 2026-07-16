# Architecture

## Goal

```text
BattleShip source + Nintendo DS backend = playable port
```

`decomp/` is read-only. Portable original gameplay remains original code; the
project supplies Nintendo DS platform services, rendering, audio, input, memory,
asset relocation, and narrow ABI compatibility.

## Fidelity Boundary

Gameplay, hitboxes, collision, physics, timing, rules, camera meaning, and state
flow stay source-faithful. Presentation targets roughly 90% overall likeness.
After one measured cosmetic attempt, a cheaper recognizable source-derived DS
representation may replace pixel-exact presentation. It may not change gameplay
geometry, telegraphs, depth meaning, or scene state. Evidence includes the
source, visible delta, measured reason, and `artifacts/visibility` screenshot.

Dream Land water is the precedent: exact BattleShip frame 0/fraction 114 is
preloaded on the original 12 triangles. Later material animation is ignored.

## Ownership

| Surface | Owner |
|---|---|
| `decomp/BattleShip-main/decomp` | Read-only gameplay/source reference |
| `decomp/sm64-nds` | Read-only DS backend reference |
| `src/import` | Coherent original translation-unit imports |
| `src/nds` | libnds hardware/backend implementation |
| `src/port` | Platform-neutral seams, diagnostics, reloc/task integration |
| `include` | Minimal BattleShip-compatible declarations and DS APIs |
| `scripts` | Builds, focused checks, captures, verifiers |
| `assets` | Source-derived DS-ready payloads |
| `builds` | Generated lab/build output |

Do not copy gameplay into DS files. A compatibility seam should normalize ABI or
platform ownership, then call the original function.

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
taskman seams map original scheduling to the single ARM9 runtime without changing
callback order. DS VBlank is presentation pacing; source logic still advances at
the original 60 Hz contract.

`osGetTime` and `osGetCount` use shared libnds CPU timers 0/1. Other DS code must
not claim those timers. Live DS buttons and stick values feed the original input
path. Wait/3/2/1/GO owns the exact control lock and timer start.

## Renderer

The renderer consumes original display traversal and maps supported N64 state to
DS BG, OAM, and GX hardware.

### 2D

- Cut G M1 keeps one complete source Dream Land wallpaper seed in 256x192 BG2.
- Live `grWallpaperCalcPersp` state updates native affine registers.
- Countdown/traffic-light/GO SObjs use setup-converted bitmap OAM.
- FPS, timer, fighter identity, stock, and damage are change-driven lower-screen
  text. Gameplay presentation remains on the top screen.

### 3D

- Profile 2 is the generic independent semantic oracle.
- Mode 8 is the retained AOT Mario/Fox owner.
- Mode 9 adds the complete Dream Land owner.
- Owners validate complete topology/state before GX mutation and fall back as a
  whole owner on unsupported state.
- Effects and weapons outside an owner remain in original display order.

The current M2-M4 owner contracts and tick gates live only in
`optimization/NATIVE_RENDERER_PLAN.md`; measurements and rejected designs live
only in `PERF_LEDGER.md`.

### Textures

The P1 scene manifest converts source textures offline, assigns stable keys, and
preloads exactly 131,072 bytes into VRAM A before GO. Gameplay may bind resident
keys but may not convert, decode, allocate, read files, create/upload/delete GL
resources, evict, or refresh. Results may establish a new prewarm boundary.

The retired animated tiled-water asset, generator, residency path, draw path,
checks, and build selector are deleted.

## Audio

BattleShip chooses music and FGM IDs. ARM9 maps original requests to the DS audio
backend; ARM7 owns playback/refill. Host muting never disables ROM audio state or
counters. Source pitch, required voices, and audible mixed output remain release
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
