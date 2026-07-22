# TASK 42 — ADPCM BGM: 4× smaller stream, hardware decode if the seam allows

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.**

**Supersession notice:** the old Task 33 was "not admitted" because its entry
gate (Task 30 shipping) went false. Tyler explicitly authorized ADPCM on
2026-07-21 — that authorization replaces the gate. Task 30's retail verdict
still stands and constrains DESIGN: do NOT smear refill work across frames
(sliced refills regressed the retail histogram); keep the concentrated
whole-half shape and make it smaller instead.

**PRIORITY + COUPLING (Tyler, 2026-07-21): this task runs NEXT, and Phase B is
the priority outcome.** Task 38 (FGM full coverage) is PAUSED at its Phase C
capacity stop. The initial 72,260-byte estimate was superseded by the exact
deduplicated census: full resident coverage has a 300,540-byte dry floor. Phase
B here retires the 64 KiB static PCM ring (`sNdsAudioBgmRing`,
nds_audio_bgm.c:124) in favor of ~16 KiB of ADPCM ping-pong buffers, but the
46,592-byte recovery does not fund that resident floor; Task 38 needs an
on-demand/prefetched cache.
Therefore: (a) the final report MUST state the static-footprint delta and the
new net reserve + full-arena confirmation — Task 38's resume math consumes
those numbers; (b) if only Phase A lands (which ADDS ~8 KiB staging), say so
loudly — Task 38 then stays paused and re-presents its numbers to Tyler rather
than resuming.

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

## Execution — 2026-07-21

Phase B is implemented directly because the bounded source/API census found a
safe existing seam: two fixed one-shot ADPCM channels (14/15), Timer 2 IRQ to a
one-slot Calico mailbox, and a 1 KiB high-priority worker that issues
`soundStart` outside IRQ context. There is no hardware loop; every packet owns
its IMA state header, and file loops seek the exact recorded loop packet/state.

- Four `BGA1` v1 assets: 1,284,428 compressed bytes from 5,125,414 exact source
  PCM bytes; 159 validated packets; maximum payload 8,196 bytes.
- Resident audio buffers: 16,392 bytes, down from the 65,536-byte PCM ring.
- Proof-map `__main_bss_end`: `0x0227e210` before → `0x02272c10` after. Net
  contiguous main-RAM recovery after all code, worker, mailbox, and stack cost:
  **46,592 bytes**.
- Corrected Task 38 funding math: 166,672 current reserve + 46,592 recovery =
  213,264; the 300,540-byte full resident dry floor would leave **40,920 bytes**,
  **57,384 below** its 98,304-byte gate before schedules and exact FX tails.
  Task 42 alone therefore does not fund a full resident pack.
- Static gates passed: all four asset/hash/container/packet/source identities,
  PowerShell parse, shared GBI fixtures, PROOF and LEAN compilation. LEAN listen
  candidate: 11,141,120 bytes, SHA-256
  `0418EB71119F99108D85757566F5CDFE8FF1EB7AE4D3C1682B207CE819C2477E`.

The first Phase-B candidate used ARM9 timer 2 for packet-seam scheduling. That
overwrote Calico's timers 2/3 system tick and corrupted every `cpuGetTiming()`
consumer, visibly breaking FPS/UP and TICKHUD cadence. The retained runtime now
uses free timer 0, and `check-audio-bgm-derived-assets.ps1` rejects timer 2/3.

Open gates: Tyler start/quality/seam listen approval, full-match arena proof,
and device economy queue. Do not approve or ship the ADPCM path from the short
runtime smoke alone.

The 2026-07-21 retained Boundary smoke passed with prepared/seam counts nonzero
and header, packet, seam-miss, timer-drop, error-stop, cleanup-fail, unsafe-write,
and overrun counters all zero. Tyler's audible approval, full-match arena proof,
and retail queue remain open.
