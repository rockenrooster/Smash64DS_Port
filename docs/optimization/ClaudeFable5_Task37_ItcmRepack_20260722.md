# Task 37 — hot-code placement repack against measured per-symbol cost

Standing rules apply (`docs/optimization/TASK_STANDING_RULES.md`).

Branch: `codex/task37-itcm-repack`. Status: PLANNED, not started.

Scope note: the queue calls this "ITCM repack", but the port has **three**
placement tiers, and ITCM is the one with the least room left. The task covers
all three.

## Why this task, and why now

The 2026-07-22 matched-state retail/emulator run (same frame, TIME 00:30, both
fighters 56%/0%, one stock each):

```
bucket   emu P50    retail P50   d      retail spread   retail P95-P50
STG      610,624    622,272     +1.9%      1.01              +6,128
FTR      592,576    609,760     +2.9%      1.76            +460,416
SRC      333,888    340,160     +1.9%      2.81            +615,040
MISC      47,744     48,896     +2.4%      2.69             +82,688
HUD        1,024      1,624       -      210.36            +340,008
AUD        1,280      1,280      0.0%     95.75            +121,280
BG         4,096      4,272     +4.3%      0.99                 -64
```

1. **Named work at retail P50 is about 1,628,000 ticks against a 3-VBlank budget
   of 1,680,570 — 3.1% of headroom.** The loop sits directly on the 3-VBlank
   cliff, which is why a 2-3% higher median on hardware moves 21 points of the
   distribution: emulator 3:69.8% / 4:24.5% / 5+:5.7%, retail 3:47.7% / 4:45.7%
   / 5+:6.6%. At this margin a small median cut has an outsized histogram effect.
2. **STG is exhausted.** Spread 1.01, tail +6,128. Task 36 and Task 44 flattened
   it. Largest single bucket, no variance left, hard median floor.
3. **FTR + SRC are 949,920 retail P50 ticks, 58% of named work**, and both are
   instruction-fetch-heavy interpreted game code. That is the placement target.

The unlock is the emulator. The repo now runs the owner's cache-modelling melonDS
fork, which ships `src/ARM9PerformanceProfiler.cpp` — a host-side profiler
attributing elapsed ARM9 timestamps per PC, explicitly capturing "cache fills,
cache streaming, write-buffer drains, bus waits, interlocks, and pipeline
refills". **Code placement is measurable for the first time.** Stock melonDS
modelled main-RAM waitstates but not icache/dcache, so placement changes produced
almost no observable delta — precisely why this task kept deferring, and why
`.text.hot` / `.text.hot.draw` had to be refereed on device.

Do not carry the Task 10 calibration multipliers into this work; see the
`TICK-HUD BASELINE / FORK EMULATOR` row in `PERF_LEDGER.md`.

## Current placement state

Measured on `smash64ds-battle-playable-tickhud-hwtri.elf`, 2026-07-22:

```
tier              used     cap    free   note
.itcm           31,676  32,736   1,060   zero-waitstate TCM, 96.8% full
.text.hot        4,476   8,192   3,716   Task 17 update set, 45% EMPTY
.text.hot.draw   7,184   8,192   1,008   Task 32 draw set, generated
.main          805,056       -       -   everything else
```

`.itcm` breakdown (`check-renderer-itcm-placement.ps1`, `check-task9-float-itcm`):

```
renderer               16,320  ndsRendererScanList 7,116;
                               ndsRendererHardwareSubmitVertex 3,276;
                               ndsRendererSubmitHardwareTriangle 2,868;
                               ...ConvertTexel01Ci4Direct.isra.0 2,600;
                               ...LitShadeColorPrepared 460
libgcc float stock      1,952  _arm_addsubsf3 684; _arm_muldivsf3 760;
                               _arm_cmpsf2 276; _arm_unordsf2 56;
                               _arm_fixsfsi 92; _arm_fixunssfsi 84
Task 9/16 replacements    768  task16-addsub 404; task16-compare 236;
                               task16-i2f 92; phase2-fcmpeq 36
unenumerated          ~12,636  no checker names this
```

`.text.hot` currently holds eleven explicitly named input sections
(`linker/nds_hot_text.ld:161-176`), and they are exactly the buckets under
attack: `gcRunAll`, `gcRunGObjProcess`, `gcPlayDObjAnimJoint`,
`ndsBaseFTComputerProcessAll`, `ftComputerProcessAll`,
`battleship_ftMainProcSearchHitAll`, `battleship_ftMainProcSearchCatch`,
`ftMainProcPhysicsMap{,Default,Capture}`, `ftMainProcUpdateInterrupt`.

**The cheapest win is not ITCM.** `.text.hot` has 3,716 free bytes under a cap
the linker already asserts, and `.text.hot.draw` another 1,008 — about 4.7 KB of
placement budget that requires no eviction of anything. ITCM has 1,060 and every
admission there costs an eviction. Order the work accordingly.

Placement mechanisms, for reference:
- ITCM, port code: `NDS_RENDERER_HOT_CODE` attribute (`src/nds/nds_renderer.c:22320`).
- ITCM, libgcc: objcopy `--rename-section .text=.itcm` from a SHA-pinned private
  copy (`Makefile:1314-1341`).
- `.text.hot`: explicit per-function input sections in the linker script
  (`linker/nds_hot_text.ld:161-176`), decomp sources untouched.
- `.text.hot.draw`: generated include `nds_task32_draw_hot.inc`, empty for
  control ROMs (`linker/nds_hot_text.ld:180-186`).

## Phase 0 — instrument, no behaviour change

- Vendor `profiling/melonds_arm9_profile_markers.h` from the fork into the port.
  It writes CP15 `c13,c0,1` (Trace Process ID); valid CP15 writes on retail with
  a small cost and no output, so one ROM runs on both.
- Add `NDS_TASK37_PROFILE ?= 0`, config-header emitted and benchmark-flag
  identified like every other flag. At 1, emit region markers on the boundaries
  the tick-HUD buckets already use, so `profiling/compare_retail_regions.py` can
  align emulator regions to retail HUD numbers.
- **Gate:** at `NDS_TASK37_PROFILE=0` the published lean ROM must still build to
  `9E27BD3D5DCBE00DC72A47221CFDD170FFE690BC1516F0B16241029F937CE369`. The
  tick-HUD work on 2026-07-22 tripped exactly this — an empty external-linkage
  function shifted lean layout — so guard the marker helper's *definition*, not
  only its body.
- Never ship `NDS_TASK37_PROFILE=1`.

## Phase 1 — census, decide nothing

- Run the profiling ROM under the fork; collect the per-PC CSV.
- `profiling/symbolize_profile.py` against the matching ELF to aggregate per
  symbol.
- Produce three ranked tables and put them in this file:
  1. Top ~40 symbols by total battle-loop cycles, tagged with bucket and with
     current tier (`.itcm` / `.text.hot` / `.text.hot.draw` / `.main`).
  2. **Every current `.itcm` resident ranked by cycles-per-byte** — the rent each
     pays for its slot. This decides evictions and closes the ~12.6 KB
     unenumerated gap above.
  3. Top `.main` symbols that would fit the 3,716 free `.text.hot` bytes.
- **Kill criterion:** if the best available placement set does not project at
  least ~40,000 ticks of combined FTR+SRC P50 saving, STOP and publish the census
  as the deliverable. A census concluding "placement is already near-optimal" is
  a successful outcome, not a failure.

## Phase 2 — repack, in cost order

1. **Fill `.text.hot` free space first.** No eviction, no risk to the renderer,
   and it targets SRC/FTR directly. Respect the 8 KiB `ASSERT`.
2. **Regenerate `.text.hot.draw`** from measured draw toppers rather than the
   Task 32 device-refereed set, now that the host can measure it.
3. **Only then touch ITCM.** Evict residents whose measured cycles-per-byte falls
   below the best non-resident candidate; admit toppers until full.
   - `check-renderer-itcm-placement.ps1:93` pins the five renderer symbols and
     `.itcm` membership. Any eviction there requires updating that checker with a
     dated citation (standing rules: cite file:line for verifier-expectation
     changes). Same for the Task 9/16 float pins.

Keep the change purely relocational. No algorithmic edits in this task — a mixed
task cannot attribute its own result.

## Phase 3 — emulator A/B

- Control: current master profile-0 tick HUD. Candidate: repacked.
- `scripts/sample-tick-hud-buckets.ps1 -Samples 256 -StartFrame 438` on each;
  compare **P50 and P95 per bucket, never means** (bursty buckets make means
  meaningless — see the ledger row).
- The scene is fully deterministic: two independent 256-sample runs on
  2026-07-22 were byte-identical in every field including min/max. No noise floor
  to average out, so repeat runs are not required.
- **Success gate:** FTR + SRC P50 down >= 40,000 ticks combined, no other bucket's
  P50 up more than 2,000, and the emulator VBI 3-bucket share up.
- **Watch STG.** It is the eviction blast radius: spread 1.01 today, so any
  renderer eviction that hurts shows as an immediate STG P50 rise.
- **Exactness:** placement must be behaviour-neutral. Gate on the existing
  state-hash A/B verifier; any divergence is an immediate REVERT, not a debug.
- Report per tier which moves earned their space, so the next task inherits a
  rent table rather than a verdict.

## Phase 4 — retail confirmation

One batched device A/B pair per the device-test economy rule. Read P50/P95 off
the HUD at `n:128` plus the VBI histogram. Report the 2/3/4/5+ histogram
normalized by sample count, never min FPS.

## Kill criteria (any one ends the task)

- Census projects < ~40,000 ticks addressable → STOP, publish the census.
- Emulator A/B delivers < 25,000 ticks combined FTR+SRC P50 → REVERT.
- Any state-hash divergence → REVERT immediately.
- STG P50 regresses more than 5,000 ticks → REVERT the offending eviction.

## Open questions to settle before Phase 2

1. **Arena divergence.** The matched run showed `A14c` on emulator versus `A13c`
   on retail — the adaptive taskman arena chose 1,359,872 bytes there and
   1,294,336 here, a 64 KiB difference. The platforms are not running quite the
   same configuration. Benign or not, it sits underneath every A/B and should be
   explained before retail numbers accept or reject a change.
2. `compare_retail_regions.py` expects a retail CSV per region; we have HUD
   buckets. Decide whether to map HUD buckets onto marker regions or use HUD P50
   directly as the retail side.
3. Are the 8 KiB caps on `.text.hot` / `.text.hot.draw` physical or policy? If
   policy, the census may justify raising one — but that trades against `.main`
   locality and needs its own measurement, not an assumption.

## Known context

- `verify-dev-fast.ps1` is currently red on the `battle_playable` locked-30
  pacing contract under the fork. Proven emulator-caused by stashing all edits
  and reproducing on clean `093690b`. Do not attribute it to this task; do not
  "fix" it by reverting the emulator.
