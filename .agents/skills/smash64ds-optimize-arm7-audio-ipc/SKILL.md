---
name: smash64ds-optimize-arm7-audio-ipc
description: Analyze, implement, optimize, or debug Smash64DS_Port audio and ARM7 communication, including BGM and FGM playback, DS mixer channels, sample caches, NitroFS streaming, ARM7 acknowledgements, IPC/shared state, cue timing, channel ownership, audio-related freezes, and frame hitches. Use for audio backend work, sound omissions, mixer contention, IPC stalls, or moving service work off ARM9.
---

# Optimize ARM7 Audio and IPC

## Mission

Keep gameplay-facing audio event timing correct while minimizing ARM9 work, synchronous waits, card contention, and cache churn. Do not trade away battle-reachable cues without explicit owner approval.

## Owning seams

Use CodeGraph, then inspect:

- `src/nds/nds_audio_bgm.c`;
- `src/nds/nds_audio_fgm.c`;
- their headers under `include/nds/`;
- `src/nds/nds_platform.c` service order;
- ARM7 acknowledgement diagnostics and freeze diagnostics;
- generated audio packs and build scripts;
- `docs/AUDIO_BACKEND_SCOUT.md`;
- BattleShip sound call sites and timing;
- local DS references for ARM7 service, mixer, IPC, and streaming.

The current FGM backend has explicit metadata, cache slots, handles, channel owners, generations, envelope service, and diagnostics. Preserve those ownership invariants unless replacing them with a proven simpler model.

## Workflow

1. **Trace the source event.**
   - Identify the BattleShip cue/BGM call and required timing.
   - Record ID, loop behavior, volume, pan, pitch/frequency, envelope, stop semantics, and simultaneous-call bursts.
   - Name what must be mechanically equivalent and what presentation fidelity may vary.

2. **Trace the backend state machine.**
   - load/lookup;
   - cache residency;
   - handle allocation/generation;
   - channel selection/ownership;
   - ARM7 command/acknowledgement;
   - envelope or duration service;
   - stop/recycle;
   - failure and unsupported paths.

3. **Classify the problem.**
   - missing/unsupported cue;
   - wrong mapping;
   - channel exhaustion or stealing;
   - stale handle/generation;
   - cache miss/read hitch;
   - synchronous ARM7 acknowledgement;
   - service cadence too frequent;
   - card/DMA contention;
   - audio-induced freeze;
   - quality/format limitation.

4. **Prefer event-driven work.**
   - Send compact commands only when state changes.
   - Keep hot samples resident based on a measured request trace.
   - Preload predictable match cues.
   - Batch metadata and sequential pack reads.
   - Service envelopes/timers at the lowest correct rate.
   - Avoid ARM9 polling or blocking waits.
   - Use bounded ring/queue semantics with explicit overflow behavior.
   - Preserve channel and generation ownership.

5. **Coordinate card and DMA behavior.**
   Load the DMA/card skill for streaming or cache work. Measure reads, bytes, cache maintenance, and contention. Never hide a frame hitch by muting or skipping the cue.

6. **Verify correctness.**
   - Cue inclusion and lookup counters;
   - play/stop counts;
   - maximum active handles/channels;
   - generation mismatch/stale stop;
   - cache request trace and miss floor;
   - expected phase/KO/loop masks;
   - no freeze or queue overflow;
   - canonical battle and transitions.

7. **Verify performance.**
   - ARM9 audio service ticks;
   - source/load correlation;
   - `WORK-H` P50/P95;
   - frame-tail changes;
   - card/DMA device classification.

## Human gate

Agents cannot hear. Do not claim audio quality, balance, or subjective fidelity is acceptable. Provide:

- exact ROM hash;
- scenes/cues to listen for;
- source-versus-candidate differences;
- expected timing and known fidelity debt;
- owner listen-check request.

Automated counters prove dispatch and backend state, not audible quality.

## Sacrifice order

The project may trade audio fidelity before visual or gameplay fidelity, but only within `PROJECT_GOAL.md` and with explicit reporting. Acceptable examples can include lower sample rate, simpler envelopes, or mono/format changes when timing and cue identity remain recognizable. Silently absent cues or changed gameplay telegraphs are not acceptable.

## Required result

Report:

- source event and backend path;
- state-machine or ownership defect;
- cache/channel/IPC measurements;
- cue coverage counters;
- ARM9 and frame-tail A/B;
- device-only queue status if applicable;
- listen-check script for the owner;
- KEEP, REVERT, STOP, or WIP.
