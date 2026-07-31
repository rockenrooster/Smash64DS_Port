---
name: smash64ds-optimize-dma-card-io
description: Analyze, optimize, or debug DMA, memory copies, cache coherency, GXFIFO transfers, VRAM uploads, NitroFS/game-card reads, asset streaming, prefetching, and load-related frame tails in Smash64DS_Port. Use for card-I/O spikes, source bucket excursions, large copies, DMA proposals, streaming audio/asset contention, or transfer-related corruption and stalls.
---

# Optimize DMA and Card I/O

## Mission

Eliminate active-frame I/O and unnecessary movement. Use DMA only when the measured transfer, bus behavior, synchronization, and cache rules make it cheaper than CPU work.

## Owning seams

Use CodeGraph, then inspect:

- `src/nds/nds_reloc_assets.c`;
- file users in `src/nds/nds_audio_bgm.c` and `src/nds/nds_audio_fgm.c`;
- stage/fighter asset preparation and texture upload paths;
- `src/nds/nds_ifcommon_oam.c` and platform copy/flush paths;
- GX replay/submission in `src/nds/nds_renderer.c`;
- current load census and permanent performance evidence;
- archived DMA experiments only as evidence, not automatic conclusions;
- local DS references for cache maintenance and transfer sequencing.

## First rule

A DMA engine shares buses and hardware destinations with the CPU and other engines. "DMA" does not mean "free" or "parallel." Prove:

- source/destination region;
- byte count and alignment;
- cache state;
- DMA channel and trigger;
- whether CPU can do useful independent work;
- destination backpressure;
- completion/synchronization point;
- effect on audio/card/GX traffic.

## Workflow

1. **Trace the transfer.**
   Record:
   - caller and frame phase;
   - source/destination;
   - bytes and frequency;
   - alignment;
   - whether data is mutable;
   - cache flush/invalidate requirement;
   - consumer start time;
   - current CPU copy/read cost;
   - tail or stall bucket.

2. **Remove movement before accelerating it.**
   Prefer:
   - keep data resident;
   - generate directly in final layout;
   - capture once and replay;
   - upload once at scene preparation;
   - dirty ranges;
   - pointer/ownership transfer;
   - compact data that reduces both I/O and decode;
   - prefetch before active gameplay.

3. **For card/NitroFS reads:**
   - instrument completed loads and bytes;
   - correlate every high-source frame with a load event;
   - distinguish metadata lookup, open/seek/read, decode, flush, and consumer work;
   - move predictable reads to match preparation;
   - keep hot assets resident;
   - batch sequential reads;
   - avoid per-cue/per-draw file operations;
   - maintain deterministic failure handling.

4. **For DMA:**
   - use libnds/calico primitives or established local helpers;
   - perform required cache maintenance;
   - ensure buffer lifetime extends through completion;
   - avoid stack buffers for asynchronous transfer;
   - guard overlap and alignment;
   - do not reuse a channel owned by audio/display or another active subsystem;
   - wait only at the latest correct consumer seam;
   - compare against an optimized CPU copy, not an intentionally slow baseline.

5. **For GXFIFO DMA:**
   - measure FIFO/backpressure separately from CPU push-loop cost;
   - verify command packing, count, destination, and completion;
   - check whether the geometry engine serializes the transfer;
   - preserve display-list ordering and dynamic state;
   - classify device-only unless the emulator model is validated for this path.

6. **For VRAM/OAM uploads:**
   - commit in the established VBlank seam;
   - update only dirty ranges;
   - keep bank mapping stable;
   - verify no tearing, stale frame, or one-frame corruption.

## Audio contention

Card reads, main-RAM copies, and cache maintenance can interfere with audio service. When optimizing BGM/FGM loading, load the audio/IPC skill too. Preserve cue timing and channel ownership while moving I/O.

## Measurement

Report:

- transfer count and bytes per frame/window;
- load-event correlation with P95/max frames;
- CPU copy/read ticks;
- overlap window and explicit wait;
- cache-maintenance bytes;
- typed bucket and VBlank histogram;
- device classification.

Card-I/O timing, DMA timing, and VBlank-edge behavior are device-only class by standing rule. Queue candidates for batched device testing.

## Required result

Report:

- transfer diagram;
- before/after movement and I/O counts;
- coherency and lifetime proof;
- error/fallback behavior;
- performance A/B;
- audio/display side effects;
- verifier result;
- KEEP, REVERT, STOP, or WIP.
