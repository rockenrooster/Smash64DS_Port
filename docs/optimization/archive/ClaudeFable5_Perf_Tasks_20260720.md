# Performance task queue — 2026-07-20 (Claude Fable 5 planner)

Five tasks, priority order: **30 → 31 → 32 → 34**, with **33 deferred** (only run it
after Task 30 ships and audio still shows in P95). Run each as its own /task with
separate commits. Tasks 30/31/32 are independent of each other; Task 34 is the big
structural one and should go last.

House rules that apply to EVERY task below (do not restate per task, just obey):

- decomp/ is read-only. Port-side edits only (src/nds, src/port, include, linker, scripts, docs).
- AGENTS.md hard rule: a performance feature must prove ENGAGEMENT on retail hardware
  (a counter on a HUD row, or a device photo), not only a melonDS win. A feature that
  silently degrades or disables itself on device may not ship enabled.
- Device A/B evidence = the presentation-interval histogram (2/3/4/5+ vblank buckets,
  `gNdsBattlePlayablePacingPresentIntervalBucket` in src/port/diagnostics.c) plus HUD row
  photos. Never min-FPS eyeballing.
- Diagnostics/observer ROMs must build with `NDS_FAST_WALLPAPER_AFFINE=0` when
  `NDS_RENDERER_PROFILE_LEVEL>=1` (profile-1 + affine OOMs the taskman arena — known,
  unfixed, accepted). The shipping profile-0 ROM keeps affine=1.
- Verify chain: `.\scripts\verify-dev-fast.ps1` then `.\scripts\verify-boundary.ps1`.
  Full sharded Regression ONLY if you touch shared/imported TUs, once, at the END of the
  session. Long builds run detached (Start-Process → log → poll completion stamp), never
  foreground.
- Final steps of every task: brief HANDOFF.md/PORTING.md update, then
  `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean`.
- Time-box open-ended debugging to ~10 emulator runs or ~1 hour; then checkpoint to a WIP
  branch and report honestly. Never leave the session with uncommitted verified work.
- You cannot hear audio and you cannot see the screen. Audio-quality gates are "flag for
  the owner listen check". Rendered-output gates need `capture-melonds.ps1` screenshots with a
  non-clear-color pixel assertion, not submission counters.

Note for Tasks 30/33: docs/HANDOFF.md says "Do not start a BGM fix". That line was about
chasing BGM as the *device dip source* (device-cleared, 503/503 histogram). the owner has now
explicitly requested BGM *spike smoothing* (2026-07-20). Update the HANDOFF line to say:
"BGM cleared as dip source; spike-smoothing (Task 30) authorized 2026-07-20."

---

## TASK 30 — BGM refill slicing (kill the ~300K one-frame spike)

**Problem.** BGM streaming refills one half-ring per ~0.743 s: `ndsAudioBgmUpdate`
(src/nds/nds_audio_bgm.c:686) → while-loop at :722 → `ndsAudioBgmRefillHalf` (:356) →
`ndsAudioBgmReadInto` (:253), which does one `fread` of `NDS_AUDIO_BGM_HALF_BYTES`
(32,768 bytes, :291) plus `DC_FlushRange` (:302) synchronously inside the frame. Measured
cost ~301K ticks on that one frame (25R matrix, audioUpdateShell P95). That single frame
can cross a vblank bucket boundary. Goal: spread the same work over several frames so no
single frame pays more than ~75K.

**Design.** Turn the one-shot half refill into a slice state machine:

1. Add `#define NDS_BGM_REFILL_SLICE_BYTES 8192u` near the other NDS_AUDIO_BGM defines,
   with `_Static_assert(NDS_AUDIO_BGM_HALF_BYTES % NDS_BGM_REFILL_SLICE_BYTES == 0u)`.
   8 KiB = 4 slices per half; each slice ≈ 75K ticks (fread+flush scale ~linearly).
2. New static state: pending write position, bytes remaining in the current half, and the
   half's deadline info. When the :722 loop decides a refill is due, instead of calling
   `ndsAudioBgmRefillHalf` for the whole half, START a pending refill (record write_half,
   write_pos, bytes_remaining = HALF) and immediately do the FIRST slice. Each subsequent
   `ndsAudioBgmUpdate` call performs one more slice: call `ndsAudioBgmReadInto(ring +
   write_pos + done, NDS_BGM_REFILL_SLICE_BYTES)`. `ReadInto` already handles loop-wrap,
   file offset tracking, and flushes exactly the range it wrote — reuse it unchanged.
   Only advance `sNdsAudioBgmNextRefillByte/Tick` (:752-753) when the LAST slice of the
   half completes, so the due/overrun math stays identical.
3. **Catch-up clause (mandatory safety):** before doing a slice, compute how many ticks
   remain until playback enters the write half (`sNdsAudioBgmNextRefillTick +
   NDS_AUDIO_BGM_HALF_TICKS - sNdsAudioBgmTimerTicksTotal` — check this formula against
   the code, cite lines in your report). If remaining ticks < 2 frame periods' worth
   (use BUS_CLOCK/15 per frame as the conservative period), finish ALL remaining slices
   in this call (degrades to today's behavior instead of risking an audio underrun).
4. The resync path `ndsAudioBgmResync` (:394) and `ndsAudioBgmFailPlayback` must CANCEL
   any pending slice state before doing their thing (resync refills a whole half itself —
   let it stay whole; it's the rare recovery path).
5. The match-start preload (full-CHUNK `ndsAudioBgmReadInto` at :622) stays whole — it is
   outside gameplay.
6. Counters: `gNdsAudioBgmRefillTicksLast/Max` (:382-386) now measure per-slice cost —
   that is the point; note the semantic change in a comment. Add
   `gNdsAudioBgmSliceCount` (increments per slice) and expose it on the profile-1 debug
   HUD next to the existing BGM row so engagement is photographable on device.
7. Keep the `NDS_HARNESS_FAST_LOGIC` path (:696-700) working — slices are fine there,
   just make sure finite tracks still finish (verify-boundary covers this).

**Gates.**
- verify-dev-fast + verify-boundary green.
- Profile-1 melonDS capture of an early-combat window (reuse the task-25R capture flow,
  e.g. the early600-607 window): `gNdsAudioBgmRefillTicksMax` ≤ 100K (was ~301K), zero
  `gNdsAudioBgmUnsafeWriteCount`, zero `gNdsAudioBgmOverrunCount`, zero
  `gNdsAudioBgmReadFailCount` across a full natural match soak (countdown → KO → rebirth
  → time-up → results).
- audioUpdateShell P95 in the captured window drops accordingly; UPD/DRW buckets
  unchanged (this must not touch them).
- Flag for the owner: one listen check on device or melonDS (you cannot hear); loop seam and
  track start must sound unchanged.

**Stop rule.** If slicing ever trips the unsafe-write or overrun counters in soak, revert
to whole-half refill, commit the instrumentation only, and report the timing math.

---

## TASK 31 — Move the gameplay coroutine stack into DTCM

**Problem.** Gameplay runs on malloc-backed main-RAM coroutine stacks
(src/port/coroutine.c:95). Every push/pop in every gameplay function pays main-RAM
waitstates (device streaming multiplier ~×1.73 vs melonDS). DTCM (16 KiB at 0x02FF0000,
linker/nds_hot_text.ld:20) is nearly empty on the ARM9 side: task-20R measured a
15,848-byte free gap, and the gameplay stack high-water was only ~8,100 bytes. Task 20R
closed because the *intact 16 KiB* stack doesn't fit — a right-sized stack does. This is
the biggest untried device-side UPDATE win.

**Step 0 — census first (do not skip).** Instrument `portCoroutineCreate`
(src/port/coroutine.c:80) to record each coroutine's id, requested size, and buffer
address (a small static table + profile-1 HUD line or GDB-readable globals). Stacks are
sized by thread id at src/port/libultra_os.c:127-128 (`id < 100` → service size, else
GObj size — find both defines and report their values). Run one natural match and answer:
**how many coroutines are live during combat, and which one carries the gameplay
high-water tracked by `gNdsTask20GameplayStackHighWater` (libultra_os.c:17)?**
- If exactly ONE big gameplay-class coroutine exists (expected: the ~16 KiB one at
  ~0x0228F200): proceed.
- If SEVERAL large coroutines are live concurrently: STOP, report the census table, and
  do not improvise a multi-stack scheme — that needs a planner decision.

**Step 1 — re-measure high water.** Poison-fill already exists
(NDS_TASK20_STACK_POISON, coroutine.c:105; high-water counter coroutine.c:45-53,
:164). Run a FULL lifecycle soak — countdown, heavy combat, shield break if you can
trigger it, KO, rebirth, time-up, results — and record both
`gNdsTask20GameplayStackHighWater` and `gNdsTask20MainStackHighWater` (libultra_os.c:21).

**Step 2 — size and place.** Choose
`stack_bytes = round_up(gameplay_high_water + 2048, 1024)` (expected ≈ 10-12 KiB).
Fit check before writing code: `stack_bytes` + main-thread high water + 1024 margin must
fit the free DTCM window. Derive the CURRENT free window from the .map file (the build
already emits one, Makefile:311): DTCM region length 0x3e80 minus `.dtcm` + `.dtcm.bss`
contents; remember Calico's main user stack grows DOWN from `__sp_usr` =
top-of-DTCM (nds_hot_text.ld:47) into the same region. If the fit check fails, STOP and
report the numbers — do not ship a tight fit.

Placement: a static buffer in the DTCM BSS, e.g.
`static u8 sNdsGameplayDtcmStack[NDS_TASK31_DTCM_STACK_BYTES]
__attribute__((section(".sbss.ndsGameplayStack"), aligned(8)));`
(the linker script collects `*(.sbss .sbss.*)` into `.dtcm.bss`, nds_hot_text.ld:145).
The link will hard-fail if the region overflows — that is a feature.

**Step 3 — wire it.** Add `portCoroutineCreateStatic(entry, arg, buffer, size)` beside
`portCoroutineCreate` (same poison fill, same alignment fixups, a flag so destroy never
frees the static buffer). In `osStartThread` (libultra_os.c:118-132), route ONLY the
identified gameplay thread to the DTCM buffer, behind `#if NDS_TASK31_DTCM_STACK`
(Makefile define, default 1 for the shipping target, with the malloc path kept intact
when 0). Everything else keeps malloc.

**Step 4 — guards (fail loud, never silent).** Keep the poison fill. Add a per-frame
canary check of the word at the stack's lowest address in the existing vblank/HUD path;
on trip, increment a counter and print a loud marker line — a corrupted gameplay stack
must never be a silent wrong-behavior source. Also print the stack buffer ADDRESS on a
profile-1 HUD row: a device photo showing 0x02FFxxxx is the engagement proof (a main-RAM
fallback would show 0x02xxxxxx below 0x02FF0000).

**Gates.**
- verify-dev-fast + verify-boundary green; one full-match soak with zero canary trips and
  high-water + 2048 ≤ stack_bytes re-confirmed.
- melonDS A/B (same-ROM windows, task-25R flow): expect small-to-nothing — melonDS is NOT
  the referee here (no cache/TCM fidelity). Record it anyway.
- DEVICE A/B, one session, two ROMs (flag on/off), same scripted scenario: histogram +
  typed UPD HUD row photos. This is the decisive gate. Prepare both ROMs and a one-line
  flash/run checklist for the owner.

**Stop rule.** Census shows multiple big coroutines, or fit check fails, or any canary
trip in soak → checkpoint, report, stop. Do not widen scope to other stacks.

---

## TASK 32 — Draw-path hot-text grouping (extend Task 17 to the stage/draw hot set)

**Problem.** Task 17 grouped the UPDATE-path hot functions into the leading `.text.hot`
output section (linker/nds_hot_text.ld:161-176, ≤8 KiB assert) and bought −11.4% UPD on
device while melonDS showed ~nothing (no icache model). The DRAW path — which the 25R
matrix says is dominated by the constant ~454K stage owner — never got the same
treatment. Same axis, same mechanism, device-refereed.

**Steps.**
1. Build the current shipping target and keep the .map (Makefile:311 already emits one).
2. Derive the draw-path candidate set. Start from the call path: the per-frame draw
   entry (`gcDrawAll` and what it calls for stage DObj traversal), the stage draw
   callbacks among the imported battleship TUs, and the src/nds/nds_renderer.c
   submission/material/matrix helpers that are NOT already ITCM residents (nds_renderer.c
   has ITCM placements — do not displace or duplicate them). Rank candidates two ways and
   intersect: (a) size + call-frequency reasoning from the map and code reading;
   (b) a GDB PC-sample histogram over ~30 s of scripted combat (existing scripted-melonDS
   GDB infra; sample during the draw window — the DRW bucket boundaries are visible in
   src/nds/nds_platform.c's phase counters). Report the ranked list with sizes.
3. Add a SECOND output section `.text.hot.draw` in linker/nds_hot_text.ld immediately
   after `.text.hot`, with its own `ASSERT(SIZEOF(.text.hot.draw) <= 8192, ...)`, listing
   the chosen `*file.o(.text.symbol)` lines exactly like :162-172. Two separate ≤8 KiB
   groups are correct (update and draw run in different phases of the frame; each group
   should be compact, they don't need to co-reside).
   IMPORTANT: also add `.text.hot.draw` to the EXCLUDE line in `.main` if needed so
   nothing double-places; and check `__main_lma/__main_start` (:272-273) still point at
   the FIRST main-RAM section — if `.text.hot` remains first, no change needed.
4. Verify placement: map shows every listed symbol inside `.text.hot.draw`; total size
   reported; no symbol vanished (gc-sections can drop things — the input lines must match
   real section names, check with `arm-none-eabi-nm`/`objdump -h` on the named .o files).
5. melonDS A/B: expect ~no change (record it, task-25R windows). Behavior must be
   pixel-identical — placement only.
6. DEVICE A/B, one session: histogram + typed DRW HUD row photos, flag-off vs flag-on
   ROMs (gate the section list behind a Makefile define like NDS_TASK32_DRAW_HOT_TEXT so
   an A/B pair is buildable). Success = DRW row drop and/or histogram shift; failure =
   flat. Either way the result is recorded and the section list stays only if it wins.

**Constraints.** Pure placement task: zero code edits inside decomp-imported functions,
no ITCM changes, no Thumb conversions (Task 12's blanket-Thumb failure is graveyard).
If the owner is flashing Task 31's A/B the same day, hand him ONE bundle with all four ROMs
and an ordered checklist — one device session, not two.

---

## TASK 33 — DEFERRED: ADPCM BGM (only if audio still shows after Task 30)

**Gate to even start:** Task 30 shipped AND a fresh profile-1 capture still shows
audioUpdateShell P95 > ~40K, or the owner complains about the match-start preload hitch.
Otherwise skip — do not run this task just because it is written down.

**Why 4×.** BGM is PCM16 mono 22,050 Hz (`SoundFormat_16Bit`, nds_audio_bgm.c:420),
~44,100 bytes/s of card I/O. IMA-ADPCM is 4 bits/sample → ~11,025 bytes/s. The FGM pack
already plays hardware ADPCM (`SoundFormat_ADPCM`, src/nds/nds_audio_fgm.c:927), and the
DS sound channels decode it for free.

**Hardware hazard you MUST design around (do not discover it by shipping):** the DS
sound hardware latches the ADPCM predictor/step state when playback crosses the loop
start and restores it on loop wrap (GBATEK, SOUNDxCNT ADPCM notes). Our BGM ring is a
LOOPING buffer whose contents are rewritten continuously — the latched state at position
0 will not match the freshly written data on the next wrap. Hardware-ADPCM ring
streaming is therefore presumed broken until proven otherwise.

**Approach A (safe, recommended): keep the PCM16 ring, shrink the FILE.**
- scripts/render-audio-bgm-pupupu.py gains an IMA-ADPCM output mode (file header keeps
  loop_start/stream_bytes semantics, now in ADPCM bytes; bump the format/version field
  and keep the loader refusing mismatched versions).
- `ndsAudioBgmReadInto` reads the 4× smaller compressed slice into a small staging
  buffer, then an ARM9 IMA decoder (small, table-driven — put it with the other hot
  leaves if profiling justifies) expands into the existing PCM16 ring, then flushes only
  the written PCM range. Predictor state persists across slices; reset it exactly at
  loop_start using a stored header snapshot (encoder emits predictor+index at the loop
  point so the seam is exact).
- Net per-slice (on top of Task 30's slicing): ~2 KiB fread + ~10-15K decode + flush —
  the refill disappears into noise. Match-start preload also shrinks 4×.
**Approach B (bounded experiment only, optional):** one throwaway harness probe that
plays a hardware-ADPCM looping ring with live rewrites and checks for wrap glitches —
the owner listen check required (you cannot hear). Only if A's decode cost measures worse
than expected. Do not ship B without that proof.

**Gates.** verify chain green; full-match soak zero read-fail/overrun; profile-1 capture
shows refill slice ticks at noise level; loop seam flagged for the owner listen check;
byte-identity note: this CHANGES the generated BGM asset — regenerate via the pinned
script, update the asset-identity notes the publish pipeline relies on (DECOMP_PIN /
identity report expectations) in the same commit, or the public build.ps1 identity gate
will fail for strangers.

---

## TASK 34 — Immutable stage stream + GX-FIFO DMA (the stage moonshot)

**Problem.** Stage draw costs ~454K melonDS ticks per frame, constant across all phases
(25R matrix) — it is traversal + re-derivation + word EMISSION cost, not transform math
(the GX does transforms; gxFlush is 64 ticks). The stage is rigid except Whispy's mouth,
flower sway, and water ST scroll. Task 29's census showed the emitted stage GX words are
bit-identical frame to frame (2,775 conserved words over frames 438-445) — task 29
closed at the retail-proof gate, NOT by falsification. The graveyard's FIFO-replay
failure was CPU-COPY transport (copy cost ≈ derivation cost, +124K); this task's delta is
(a) DMA transport instead of CPU copy, (b) overlapping the DMA with the source-update
pair. melonDS will NOT price this honestly — the device histogram is the only referee.

Run as three phases, each its own commit, each with a kill criterion.

**Phase E1 — boundary certificate (measurement only, no behavior change).**
1. Extend the task-29-style capture to dump the FULL per-frame stage GX word stream
   (commands + words, per DObj) across three windows with a MOVING camera (reuse the 25R
   same-ROM windows: countdown438-445, early600-607, whispy1398-1405).
2. Classify per DObj: (a) bit-identical across all captured frames, (b) varying. Expected
   split: one per-frame prologue (projection/position matrix from the live camera) +
   live DObjs (Whispy jaw, flowers, water) varying; the rigid majority identical. The
   23R consumed-fields certificate (artifacts/performance/2026-07-18_task23r-phase0.md)
   is the cross-check for which DObjs can legally be static.
3. Report: exact byte size of the would-be baked buffer (expect tens of KiB), the DObj
   partition, and the seam definition (what state the baked stream assumes on entry and
   guarantees on exit — matrix stack depth, bound texture/material state).
**Kill criterion:** if the "identical" partition is under ~60% of stage words, stop —
the bake can't win. Report and close.

**Phase E2 — bake + CPU replay (correctness stage; expect ~cost-neutral in melonDS).**
1. At match start (after stage load, before countdown ends), capture one frame's stage
   stream for the certified-static DObjs into a main-RAM buffer: 4-byte aligned,
   `DC_FlushRange` ONCE at bake time, immutable afterwards.
   ARENA GUARD (hard requirement — the affine OOM lesson): allocate the buffer BEFORE
   the taskman arena reservation or from a static array sized from E1's measurement;
   after boot, assert the adaptive arena (src/port/diagnostics.c:7306-7350) still got its
   full size and print it on the HUD — a degraded arena is a FAILED gate even if the
   game boots.
2. Per frame: CPU emits the live prologue (camera matrices), then REPLAYS the baked
   buffer with a tight word-copy loop to the GX FIFO, then draws the excluded live DObjs
   (Whispy/flowers/water) and fighters through the existing path, in the original order
   contract from E1's seam definition.
3. Fail-closed runtime state machine (mirror the affine feature's shape): any
   invalidation signal (stage overlay/mask change, viewport change, any certified-static
   DObj going animated) → this frame falls back to the full CPU path and increments
   `gNdsStageStreamFallbackCount`. Engagement counters `gNdsStageStreamReplayCount` /
   `BakedDObjCount` on a profile-1 HUD row (device photo = engagement proof), RAM
   counters profile-agnostic like the pacing histogram.
4. Gate: melonDS screenshots (capture-melonds.ps1) baked vs flag-off across the three
   windows — PIXEL-IDENTICAL stage rendering (non-clear-color assertion; counters prove
   submission, not display). Full-match soak: zero fallbacks outside legitimate
   invalidation frames, zero GX FIFO error/overflow symptoms, results screen clean.
   melonDS tick A/B recorded but NOT a kill signal either way (graveyard says CPU-copy ≈
   neutral; that is expected and fine at this stage).
**Kill criterion:** any non-reproducible visual divergence or GX hang in soak after the
time-box → checkpoint on a WIP branch, report, stop.

**Phase E3 — DMA transport + update overlap (the actual win; device-refereed).**
1. Replace the E2 CPU copy with GX-FIFO DMA: one DMA channel in geometry-FIFO mode
   (the hardware throttles on FIFO-half-empty; check what Calico 1.2.0 exposes for DMA —
   find its dma API header; do NOT mix libnds legacy dma macros with Calico's runtime
   without checking ownership of the channel and IRQ).
2. Frame order: CPU prologue → arm the DMA for the baked buffer → run the SOURCE UPDATE
   pair while the DMA feeds the FIFO → poll/wait DMA complete → draw live DObjs +
   fighters. The overlap with the update pair is where the device win comes from; the
   fixed-two scheduler contract is untouched (still exactly 2 source updates per
   present — behavior-sacred).
3. Safety: never let CPU-side GX FIFO writes interleave with an active geometry DMA;
   assert DMA-idle before the live-DObj pass; keep a Makefile flag
   (NDS_TASK34_STAGE_STREAM with 0=off, 1=CPU replay, 2=DMA) so every A/B pair is
   buildable.
4. Gates: melonDS soak stability + pixel-identity again (melonDS DMA timing is
   approximate — record ticks, trust nothing); then DEVICE session: flag-off vs flag-2
   ROMs, scripted same scenario, histogram + typed DRW HUD row photos + engagement/
   fallback counter photos. Success = DRW drops toward the prologue+live remainder and
   the histogram shifts a bucket; that is the first structural stage win since the
   campaigns closed.
**Kill criterion:** DMA-mode instability (hangs, FIFO underflow artifacts, ARM7/audio
contention symptoms) not fixed within the time-box, or a flat device histogram → keep
E2 committed but flag-off, report, and we close the stage lane for good.

**Fences.** This is NOT the graveyard CPU-copy replay (cite the graveyard entry in your
report and state the two deltas). No fighter DObjs in the bake — fighters are excluded
entirely. No edits to the wallpaper/affine feature (separate 2D-engine machinery). Stage
visual output must remain pixel-identical in every shipped configuration.

<!-- FGM audit moved to ClaudeFable5_Task38_FgmAudit_20260720.md per TASK_STANDING_RULES.md
     (one task per file; 36/37 were already taken). Removed block follows in that file. -->

## MOVED — TASK 38 (FGM audit): see ClaudeFable5_Task38_FgmAudit_20260720.md
