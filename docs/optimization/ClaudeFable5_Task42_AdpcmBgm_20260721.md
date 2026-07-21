# TASK 42 — ADPCM BGM: 4× smaller stream, hardware decode if the seam allows

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.**

**Supersession notice:** the old Task 33 was "not admitted" because its entry
gate (Task 30 shipping) went false. Tyler explicitly authorized ADPCM on
2026-07-21 — that authorization replaces the gate. Task 30's retail verdict
still stands and constrains DESIGN: do NOT smear refill work across frames
(sliced refills regressed the retail histogram); keep the concentrated
whole-half shape and make it smaller instead.

## Current facts

- BGM is PCM16 mono 22,050 Hz (`SoundFormat_16Bit`, src/nds/nds_audio_bgm.c:420),
  64 KiB looping ring, one 32,768-byte half refilled per ~0.743 s via
  `ndsAudioBgmRefillHalf` → `ndsAudioBgmReadInto` (fread :291 + DC_FlushRange
  :302). Spike ≈ 300K ticks on that frame.
- IMA-ADPCM is 4 bits/sample → 8,192 bytes per half. DS sound channels decode
  IMA in hardware (`SoundFormat_ADPCM` precedent: src/nds/nds_audio_fgm.c:927).
- Asset pipeline: `scripts/render-audio-bgm-pupupu.py` renders the stream +
  header (loop_start_bytes/stream_bytes) consumed by the loader.
- **Hardware trap (design around, do not discover):** a LOOPING ADPCM channel
  latches predictor/index state at the loop-start position and restores it on
  wrap (GBATEK, SOUNDxCNT ADPCM notes). A streaming ring rewrites its contents,
  so the latched state goes stale → audible glitch at every wrap. Hardware-loop
  ADPCM streaming is presumed broken; the viable hardware shape is PING-PONG
  ONE-SHOTS (below), which never uses the hardware loop.

## Honest expectations (report against these)

- Phase A (ARM9 decode into the existing PCM ring): fread 4× smaller, but
  decoding 16,384 samples costs ~80–120K bus ticks even in a tight ARM loop.
  Net spike ≈ 170–240K (from ~300K). Also: 4× smaller asset, 4× smaller
  match-start preload hitch, 4× less card I/O.
- Phase B (hardware ADPCM ping-pong): no CPU decode, flush shrinks to the 8 KiB
  compressed halves. Net spike ≈ 70–100K. This is the real prize; Phase A is
  the safe fallback and ships if B fails its seam gate.

## Phase 0 — encoder + asset

1. Add an IMA-ADPCM output mode to `render-audio-bgm-pupupu.py`: continuous
   predictor encoding across the whole track; a format/version field bump the
   loader refuses to mismatch; loop-seam exactness — emit the predictor/index
   snapshot AT loop_start so seeks and loops restore bit-exact decoder state;
   additionally emit a 4-byte IMA header snapshot at every half-boundary
   (Phase B needs per-buffer headers; cheap to store for all halves).
2. Publish identity: regenerated asset changes the ROM — update the pinned
   identity expectations in the same change (standard rule).

## Phase A — ARM9 decode into the existing PCM16 ring (safe path)

1. `ndsAudioBgmReadInto` reads the 8,192-byte compressed half into a small
   staging buffer, decodes into the existing PCM ring half (tight table-driven
   ARM loop; consider `.itcm` placement only if the census justifies it), then
   flushes the written PCM range. Whole-half at once — no slicing.
2. Decoder state: persists across halves; reset from the stored snapshot
   exactly at loop wrap (file-side loop, not the ring loop). Resync path
   (`ndsAudioBgmResync` :394) must seek AND restore the matching snapshot —
   derive snapshot granularity from what resync needs (per-half snapshots from
   Phase 0 cover it).
3. Gates: profile-1 refill-ticks counter shows the new spike (report exact);
   zero read-fail/overrun/unsafe counters across a full natural match; finite
   and looping tracks both verified (verify-boundary Results audio); **Tyler
   listen check** — track start, loop seam, and general quality vs PCM.

## Phase B — hardware ADPCM ping-pong (the prize; bounded probe first)

1. PROBE (throwaway harness, time-boxed ~1 session): two `SoundFormat_ADPCM`
   ONE-SHOT buffers (halves A/B, each with its own IMA header). While A plays,
   fill B; start B when A ends; alternate. Start scheduling: timer-IRQ or
   tick-deadline from the existing playback-position math (:710-712) —
   measure the seam gap/jitter in samples and report it. **Tyler listen check
   on the probe**: continuous tone + music, listening for seam ticks/pops.
   The probe never touches the shipping loader.
2. If the seam is clean: implement in the loader — replaces `soundPlaySample`
   looping ring with the ping-pong pair; refill becomes fread 8 KiB + flush
   8 KiB (no decode); keep all existing failure counters and add a seam-miss
   counter (loud, on the shared engagement HUD row).
3. Risk fences: a late restart = audible gap → the seam-miss counter must stay
   ZERO in a full-match soak; any IRQ-latency interaction with the frame loop
   gets measured, not assumed. If calico's channel-start latency makes
   sample-accurate seams impossible from ARM9, STOP and report — do not write
   custom ARM7 code in this task.
4. Device: queue the retail pair per device economy (card timing is
   device-class); melonDS refill-ticks + counters are the working referee.

**Kill criteria:** Phase A decode measurably worse than expected (>150K decode
alone) → report before shipping; Phase B seam gap audible to Tyler or
seam-miss counter nonzero in soak → keep Phase A, close B with the probe
numbers. Separate commits per phase; snapshot at the end.
