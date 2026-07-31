---
name: smash64ds-budget-ram-vram-assets
description: Design, audit, or optimize main RAM, VRAM, arenas, heaps, stacks, resident assets, texture and palette storage, cache sizing, allocation lifetime, generated data, and memory-capacity tradeoffs in Smash64DS_Port. Use for OOMs, taskman pressure, VRAM bank plans, texture failures, asset streaming versus residency, cache sizing, memory corruption, or trading ROM/RAM for runtime speed.
---

# Budget RAM, VRAM, and Assets

## Mission

Treat DS memory as a performance resource while preserving stability and all battle-reachable content. Spend ROM and loading time aggressively when they reduce active-match work.

## Read first

Read the project contract and standing rules. Use CodeGraph, then inspect:

- `src/nds/nds_reloc_assets.c`;
- `src/nds/nds_platform.c` for VRAM/display configuration;
- `src/nds/nds_renderer.c` and `include/nds/nds_renderer.h`;
- audio resident/cache budgets in `src/nds/nds_audio_bgm.c` and `src/nds/nds_audio_fgm.c`;
- `src/port/diagnostics.c` taskman/arena ownership;
- linker files, generated map, and Makefile budget assertions;
- local `sm64-nds` and `sm64ds-decomp` memory/asset strategies.

## Memory model to produce

For any substantial change, create a compact lifetime table:

| Region | Owner | Bytes | Alignment | Created | Last use | Mutable | DMA/GX visible |
|---|---|---:|---:|---|---|---|---|

Cover:

- static/BSS/data;
- stack and high-water reserve;
- taskman/scene/fighter/stage arenas;
- resident fighter and stage data;
- decoded textures and palettes;
- GX capture/replay streams;
- OAM/BG buffers;
- audio metadata and sample caches;
- diagnostics enabled in the measured build;
- emergency/stability reserve.

Do not call unmeasured free space "available."

## Workflow

1. **Identify the failure mode.**
   - hard allocation failure;
   - overlap/corruption;
   - fragmentation;
   - stale pointer/lifetime bug;
   - taskman arena exhaustion;
   - VRAM bank conflict;
   - texture/palette capacity;
   - card-I/O tail caused by non-residency;
   - cache miss rate or cache pollution.

2. **Trace ownership and lifetime.**
   - Locate allocator, caller, release/reset, and every alias.
   - Check scene transitions, match restart, results, and failure paths.
   - Confirm alignment and whether DMA or hardware reads the region.

3. **Choose the cheapest representation.**
   - generated compact native data;
   - immutable ROM data read sequentially;
   - load-time expansion into a runtime-friendly layout;
   - dense indices and narrow fields where range-proven;
   - stage/fighter-specific storage;
   - predecoded resident data when card reads affect P95;
   - 2D or 3D hardware-native texture formats;
   - separate hot metadata from cold payload.

4. **Size caches from traces, not hit rate.**
   - Record the request-key sequence behind a lab flag.
   - Replay it host-side for candidate sizes.
   - Find the compulsory miss floor: number of distinct keys.
   - Pick the smallest size that reaches the floor.
   - Confirm the ROM reaches the predicted miss count before A/B.
   - If no practical size approaches the floor, change the representation.

5. **Handle VRAM as a whole-system plan.**
   - Record every bank's mode, screen engine, texture/palette/BG/OBJ role, and transition.
   - Avoid remapping banks during active gameplay unless the measured win justifies synchronization and invalidation costs.
   - Confirm main/sub screen routing and BG/OAM layer ownership.
   - Validate texture/palette formats and address alignment.

6. **Prove stability.**
   - Add compile-time assertions for fixed layouts and budgets.
   - Add bounded runtime guards for allocation/lifetime failures.
   - Test cold boot, repeated match restart, scene exit, and the canonical battle.
   - Treat corruption, flashing, nondeterminism, or silent fallback as failure.

## Content rule

Capacity pressure is not permission to exclude battle-reachable sounds, effects, animations, or geometry. Present measured alternatives:

- enlarge or rebalance the budget;
- move work/data to ROM or loading time;
- compress cold storage and expand once;
- evict only with a proven bounded working set;
- replace presentation only within the fidelity contract and owner approval.

Never silently drop content.

## Performance rules

Memory changes can improve or regress CPU locality. Measure both:

- bytes and peak reserve;
- allocations/resets per frame;
- card reads and loaded bytes;
- cache request trace/miss floor;
- typed tick buckets;
- device-only classification for cache-sensitive effects.

A lower byte count is not automatically faster. A denser representation that adds per-frame decode can lose.

## Required result

Report:

- before/after lifetime table;
- peak RAM and minimum reserve;
- VRAM bank map when relevant;
- cache trace and compulsory floor when relevant;
- card-I/O/residency change;
- verifier and restart/stability results;
- performance A/B;
- fidelity/content impact;
- KEEP, REVERT, STOP, or WIP.
