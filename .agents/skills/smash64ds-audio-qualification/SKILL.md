---
name: smash64ds-audio-qualification
description: Implement or qualify one Smash64DS P1 audio path from original source event to audible Nintendo DS output. Use for Dream Land BGM channel/PCM proof, FGM, Mario/Fox voices, crowd cues, loop points, decode/mix/transfer cost, underruns, cleanup, or reserve. Separates trigger, asset, mixer, channel, acoustic, timing, and memory evidence and requires a natural exact-ROM event. Do not use for generic renderer work or a marker-only audio claim.
---

# Objective

Close one P1 audio requirement with source-faithful event timing, correct asset
selection and looping, actual DS channel/PCM output, stable cadence, safe memory,
and a natural exact-ROM audible qualification where required.

# 1. Trace the complete source-to-speaker chain

Identify and record:

1. original BattleShip event/call and its natural trigger;
2. sound/voice/BGM ID, bank/table/reloc entry, format, rate, channels, loop points,
   priority, and stop/replacement policy;
3. import/provider selection and any fallback/stub;
4. decode or predecode path;
5. mix, resample, volume/pan/envelope, queue, copy/DMA/cache maintenance;
6. ARM7/ARM9 ownership and synchronization;
7. DS hardware channel allocation/start/stop state;
8. produced PCM peak/RMS/envelope or equivalent independent acoustic signal;
9. underrun, overrun, stale, cleanup, and reserve counters;
10. emulator host-volume policy versus ROM audio state.

A source event marker, command ACK, allocated buffer, or user report alone proves
only one link in this chain.

# 2. Establish exact timing and asset contracts

Read the original source and asset metadata before implementation. Preserve:

- natural event timing and once/repeat cadence;
- source priority, interruption, overlap, and stop semantics;
- exact loop start/end and no-growth behavior;
- voice/fighter ownership and winner/Results transitions;
- sample order, channel count, rate, signedness, and endian/packing rules;
- scene teardown and channel/resource release.

Do not substitute a convenient sound, trigger a cue from a verifier-only branch,
or accept a continuously retriggered sound because it is audible.

# 3. Choose the cheapest correct implementation boundary

Prefer setup-time/AOT asset preparation and bounded runtime playback. Separate
performance counters for:

- asset/file lookup or I/O;
- decode/conversion/resampling;
- mixing/envelope work;
- bytes copied or transferred;
- queue wait and synchronization;
- DS channel start/refill/stop;
- audio-thread and main-thread active ticks.

Use ARM7 offload, DMA, larger buffers, or residency only after measuring the
exclusive wall and proving synchronization, cache coherency, channel conflicts,
latency, reserve, and failure behavior.

# 4. Build independent proof

Use the current focused natural mode-163 route. Require applicable evidence:

- exact source event count and logic-tick timing;
- correct resolved asset/hash and loop metadata;
- enabled DS channel mask and channel parameters;
- nonzero PCM peak plus bounded RMS/envelope or a host acoustic oracle;
- refill cadence, no underrun/overrun, no stale queue, and clean stop;
- no duplicate trigger or leaked channel after KO/Results/scene teardown;
- memory reserve/high-water and no renderer/input/lifecycle regression;
- exact-ROM audible retest for requirements that cannot be fully automated.

Automation may mute the host while ROM-side audio remains live; do not mistake
host volume zero for disabled game audio. Conversely, ROM-side ACKs are not
proof that a human-audible waveform reached the output.

# 5. Fast decision ladder

1. Host fixture for asset/decode/loop/acoustic invariants.
2. Source event and provider-resolution check.
3. Isolated DS channel/PCM smoke where already supported.
4. Natural mode-163 trigger with short state and acoustic capture.
5. Repeated event/loop/cleanup run.
6. One-minute lifecycle/memory/audio safety gate.
7. Exact-ROM audible qualification and current broader route only for a candidate
   that passes the earlier gates.

Stop immediately on wrong asset, cadence, loop, channel, silence, clipping,
underrun, leak, reserve loss, or a synthetic trigger.

# 6. Performance changes

When the packet claims frame-time improvement, also apply
`$smash64ds-perf-experiment`. A smaller audio counter is not a win if latency,
audibility, cadence, reserve, or total active-frame P95 regresses.

# Output

Use `references/audio-evidence-packet.md`. Return a coherent patch/commit and one
of `KEEP`, `REWORK`, `REVERT`, or `BLOCKED`. Worker lanes do not edit central
truth docs or publish ROMs.
