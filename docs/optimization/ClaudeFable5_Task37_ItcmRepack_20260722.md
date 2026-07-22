# Task 37 — hot-code placement repack against measured per-symbol cost

Standing rules apply (`docs/optimization/TASK_STANDING_RULES.md`).

Branch: `codex/task37-itcm-repack`. Status: EXECUTED — see RESULTS below.

> **The census overturned this plan's Phase 2 ordering.** The plan assumed
> `.text.hot`'s 3,716 free bytes were the cheapest win because they need no
> eviction. Measurement says `.text.hot` is not a working tier: it stalls at the
> same rate as ordinary `.main` code. ITCM is the only tier that pays, so the
> repack went there instead, using free space only. The original Phase 2 text is
> kept below unedited as the record of what was believed before measuring.

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

## RESULTS (2026-07-22)

### Phase 0 — instrument

`include/nds/nds_task37_profile.h` implements the emulator's CP15 marker
protocol (the constants are its published interface; no emulator source is
vendored). `NDS_TASK37_PROFILE=1` resets the profiler at presented frame 438 and
dumps at 566, so the CSV covers 128 settled battle frames instead of boot.

- MCR is ARM-only and the battle seam is Thumb, so the marker write is
  `__attribute__((noinline, target("arm")))`. Two interworking calls per run.
- **Lean gate held:** at `NDS_TASK37_PROFILE=0` the published ROM rebuilt to
  `9E27BD3D5DCBE00DC72A47221CFDD170FFE690BC1516F0B16241029F937CE369`,
  11,428,864 bytes — byte-identical to `DECOMP_PIN.txt`.

Driver: `scripts/run-task37-profile-census.ps1`. Analysis:
`scripts/task37_census.py`. Artifacts in `artifacts/task37-census/`.

### Phase 1 — census

128 frames, 500,810,896 emulated cycles, 168,894,530 instructions, 60,709
distinct PCs, 844 of 3,319 FUNC symbols executed. 0.00% unattributed.

**The census asks a question cycles alone cannot answer.** Placement changes
what an instruction *fetch* costs; it does nothing for what a *load* costs. So
every profiled PC is classified by opcode as memory or non-memory, and only
non-memory stall — cycles beyond one per instruction on instructions that touch
no data — is treated as recoverable. `armWaitForIrq` is excluded throughout: it
is 8 bytes of deliberate VBlank spinning that alone accounts for 9.21% of all
cycles, and leaving it in makes the zero-waitstate tier look like the worst one.

```
tier              cycles      insns  cyc/insn  non-mem stall     %   mem stall     %
.itcm         88,250,502  52,480,774    1.68     12,980,124   14.7  22,789,604  25.8
.text.hot     15,851,755   4,805,435    3.30      4,754,539   30.0   6,291,781  39.7
.text.hot.draw 65,601,182 24,985,828    2.63     14,698,523   22.4  25,916,831  39.5
.main        284,956,329  86,619,628    3.29     84,002,286   29.5 114,334,415  40.1
```

Three findings:

1. **ITCM works.** Half the non-memory stall rate of `.main` (14.7% vs 29.5%).
2. **`.text.hot` does not work.** 30.0% non-memory stall against `.main`'s 29.5%,
   and a marginally *worse* cycles-per-instruction. The Task 17 update-path tier
   is not measurably buying anything. Its 3,716 free bytes are therefore not the
   cheap win this plan assumed — they are free because they are not worth much.
3. **`.text.hot.draw` does work.** 22.4% vs 29.5%. Task 32 earned its retail KEEP.

The likely discriminator between 2 and 3 is re-entry frequency, not address
grouping: `.text.hot.draw` holds functions called thousands of times inside one
frame, where grouping into contiguous cache lines compounds; `.text.hot` holds
update-path functions called once per frame, which are cold on arrival no matter
who their neighbours are. Anything built on "group the hot functions together"
should be checked against that before it is trusted.

**The ITCM enumeration gap in this plan's opening section was wrong.** 64
residents cover 31,628 of 31,676 section bytes; only 48 bytes are unnamed, not
~12,636. What the checkers did not enumerate, the ELF always could.

**5,040 bytes of ITCM never executed once** in 128 frames — dead stock-libgcc
float copies (`__addsf3`, `__aeabi_frsub`), the Task 9/16 `*_golden` verifier
references, `ndsRendererHardwareConvertTexel01Ci4Direct` (2,600 B), and others.
That is an eviction budget worth roughly 26.3M non-memory stall cycles if spent.
It was deliberately **not** spent in this task: those symbols are dead *in this
scene's steady state*, which is not the same as dead, and several are pinned by
`check-renderer-itcm-placement.ps1` and the Task 9/16 float checkers. Spending it
is a separate task with its own cross-scene evidence.

The biggest single cost in the program, `ndsRendererHardwareResolveOrBindTexture`
(21.8M cycles, 4.36%), is 10,260 bytes and fits no tier. It is an algorithmic
target, not a placement one.

Likewise `memset`/`memcpy`/`memcmp` cost 38.9M cycles between them (7.8%), but
only 4.8M of that is non-memory stall — the rest is data traffic that placement
cannot touch. `memcmp` alone is called ~3,900 times per frame. **Reducing those
call counts is worth far more than moving the code**, and belongs in its own task.

### Phase 2 — repack (placement only, zero eviction)

`NDS_TASK37_ITCM_LEAVES=1` admits the seven densest non-memory-stall symbols
that fit ITCM's 1,060 free bytes, all from `.main`, all keeping their exact
compiled form — `NDS_TASK37_ITCM_CODE` sets a section and nothing else. No ISA
switch, no optimization change, no eviction, no verifier contract touched.

```
symbol                                  bytes  non-mem stall  mechanism
memset                                    140      2,391,465  libc.a, SHA-pinned objcopy
memcpy                                    170      1,173,373  libc.a, SHA-pinned objcopy
memcmp                                     70      1,249,372  libc.a, SHA-pinned objcopy
__ieee754_sqrtf                           236      1,054,412  libm.a, SHA-pinned objcopy
ndsRendererHardwareTextureSourceBytes     132        644,164  NDS_TASK37_ITCM_CODE
ndsFTParamsInvalidateFighterParts          58        501,888  NDS_TASK37_ITCM_CODE
ndsRendererHardwarePolyFmt                100        372,643  NDS_TASK37_ITCM_CODE
                                    ---------  -------------
                                          906      7,387,317
```

`.itcm` 31,676 → 32,596 of a 32,736 hard cap (140 B free); `.main` −800 B.

`ndsRendererMtxMul20p12` was prepared and then dropped: it carries the largest
raw figure that fits (1,733,057) but it already lives in `.text.hot.draw`, so its
measured tier delta is the smallest of the candidates (22.4%→14.7%, against
29.5%→14.7% for `.main` symbols), and it is the only one whose move would have
required editing a verifier contract
(`scripts/check-task32-draw-hot-text.ps1:22`). Worst candidate, highest cost.

### Phase 3 — emulator A/B

Matched pair, frames 438..637, 200 samples each, same melonDS
(`DE80E46BDCF1FD98`), `slips=0` on both. Windows asserted equal by
`scripts/compare-tick-hud-buckets.ps1`. P50/P95 only — never the means.

```
bucket   ctl P50     cand P50      d P50       %      ctl P95     cand P95      d P95
ALL    1,680,192   1,680,192          0    0.00    2,800,448    2,240,768   -559,680
FTR      591,936     576,640    -15,296   -2.58    1,032,448    1,005,824    -26,624
STG      610,560     570,240    -40,320   -6.60      618,624      578,112    -40,512
BG         4,096       4,224       +128   +3.12        4,224        4,288        +64
AUD        2,240       2,176        -64   -2.86       64,960       64,768       -192
HUD        1,024       1,024          0    0.00      329,152      317,952    -11,200
SRC      326,080     322,432     -3,648   -1.12      985,472      974,976    -10,496
MISC      47,808      47,680       -128   -0.27      168,256      165,568     -2,688
OTHR     126,784     165,696    +38,912  +30.69      530,560      446,784    -83,776
```

**Named work P50 fell 59,328 ticks.** Every named bucket improved or held except
BG (+128, a rounding-scale move on a 4,096-tick bucket).

`OTHR` rising is the correct signature, not a regression. `ALL` is
VBlank-quantized and did not move (both sit on exactly 3 VBlanks), and `OTHR` is
defined as the `ALL` remainder — so work removed from named buckets reappears as
pacing slack inside the same envelope. The question is whether that slack is
enough to change which envelope the frame lands in, and it is:

```
VBlank interval share, normalized by sample count (n=637 each)
              3-VBI   4-VBI   5+-VBI
  control      71.7    23.1     5.2
  candidate    76.0    20.9     3.1
```

**+4.3 points into the 3-VBlank bucket and 5+ frames cut from 5.2% to 3.1%.**
`ALL` P95 fell 559,680 ticks — one whole VBlank interval (560,190), i.e. the 95th
percentile frame moved from a 5-VBlank frame to a 4-VBlank one.

**Against the stated success gate: FTR+SRC P50 fell 18,944 combined, short of the
40,000 the plan required.** The gate is not met as written. It named the wrong
buckets: the census showed the recoverable stall was concentrated in the
renderer/stage path, and that is where the win landed — STG, the bucket this
plan's own opening section called "exhausted... no variance left, hard median
floor", gave up 40,320 ticks (6.60%) to pure code placement. Reporting this as a
pass against the original numbers would be false; reporting it as a failure
would be worse. The measurement is above, the gate is above, and the mismatch is
the finding.

### Exactness — the investigation

The 0-vs-7 gate failed: 692 of 3,892 per-update records differ, in three bursts
(1412..1733, 1992..2327, 2501..2534). What follows is the control set that
narrowed it, because the first two explanations reached for were both wrong and
neither was tested before being believed.

```
control                                    result
same ROM run twice                         IDENTICAL   gate is a valid oracle
mask 1 (memset/memcpy/memcmp)              FAIL 692    same three bursts
mask 2 (__ieee754_sqrtf alone)             FAIL 692    same three bursts
mask 4 (three port functions)              FAIL 692    same three bursts
mask 7 with BGM falsified                  FAIL 692    same three bursts
mask 0 + 800B dead padding in .main        PASS 3892   layout shift is invisible
mask 0 + 800B dead padding in .itcm        PASS 3892   section growth is invisible
```

Read together these are strongly constraining:

- **The gate is deterministic.** Two runs of one ROM produce byte-identical
  records, so the divergences are real signal, not noise. This control should
  have been run before the gate was ever used to judge anything.
- **It is not the moved symbols.** Three disjoint groups — one of them a single
  236-byte float leaf that nothing stores a pointer to — produce a byte-identical
  failure signature. Three unrelated changes do not cause one identical fault.
- **It is not layout.** 800 bytes of never-executed padding in `.main` shifts
  every subsequent address and the arena base, and the gate does not notice. The
  hash's pointer canonicalization is doing its job.
- **It is not ITCM section size or its load image.** The same padding in `.itcm`
  also passes.
- **It is not BGM**, despite the audio symptom that first suggested it.

Padding changes layout but not execution speed. Relocation changes speed. That
is the one variable every failing arm shares and every passing arm does not.

Three explanations were then tested and all three were wrong, which is recorded
here because the pattern matters more than the individual guesses:

```
hypothesis                                  test                        result
async BGM position                          NDS_BGM_FALSIFIER_OFF=1     FAIL, same 692
ARM7 input phase under fast logic            exclude gSYControllerDevices FAIL, same 692
VBlank pacing removes the speed dependence   -RealtimePresentation        blocked: hits the
                                                                          pre-existing locked-30
                                                                          pacing red on this
                                                                          emulator, unrelated to
                                                                          Task 37
```

Guessing one region per run cost three full match lifecycles and found nothing,
so the instrument was changed to answer the question directly.
`NDS_TASK9_STATE_HASH_REGION_MASK` selects which record kinds enter the hash, and
the state was bisected:

```
region set                                                        result
core: scalars/RNG, scene, battle, camera, ground,
      controllers, collision bounds + speeds                      PASS 3892 identical
object tree: GObj..transform                                      FAIL 692
  gameplay objects: GObj, process, fighter, item, weapon, effect  FAIL 692
    fighter/item/weapon/effect payload only                       FAIL 692
      fighter FTStruct alone                                      FAIL 692
```

**The core result is the significant one.** `syUtilsRandSeed()` is inside it, and
so are battle state, camera, ground and collision. All bit-identical for the
entire 3,892-update match. The two builds draw the same random numbers in the
same order and agree on the battle, the camera and the collision world
throughout. Whatever differs, the simulation is not running a different match.

The camera passing is a further constraint worth keeping in view: the camera
tracks fighter positions, and it is identical on every update. That is hard to
reconcile with fighter positions differing.

`FTStruct` is hashed as a single raw blob, so "the fighter region differs" does
not separate a differing position from a differing pointer or animation field.
Region masks cannot resolve further.

**Stopped here, per the standing time-box** (~10 emulator runs or ~1 hour on
open-ended debugging; this had reached roughly twelve full match lifecycles).
The shipping decision does not depend on the answer — the kill criteria settle it
either way — and what remains is a question about the instrument, not about this
task.

### Next step, specified so it is not another guess

Do not continue one hypothesis per run. Split `FTStruct` into chunks emitted
under distinct record kinds: kinds 1..22 are used, bits 23..31 of
`NDS_TASK9_STATE_HASH_REGION_MASK` are free, so eight chunk kinds fit. A binary
search over those chunks localizes the differing offset in three runs, and the
offset maps to a member of `FTStruct` by `offsetof`.

The single most useful constraint to carry in: **the camera is bit-identical on
every one of the 3,892 updates**, and the camera tracks fighter positions. That
is difficult to reconcile with fighter positions differing, which points at a
non-positional member — a pointer, an animation field, or render-cached state.
If the differing offset turns out to be a pointer, check whether it is a code
pointer: `ndsTask9StateCanonicalWord` collapses main-RAM addresses to
`0x20000000` and ITCM addresses to `0x30000000`, so a pointer whose target moved
into ITCM changes canonical class without any behavioural change. That is a real
gap — the canonicalizer neutralizes `.main` relocation precisely so layout does
not register, and simply does not extend that to the ITCM destination. It would
also explain the one fact nothing else has: three disjoint symbol groups
producing one byte-identical failure signature, because any move into ITCM
crosses the same class boundary.

If instead the offset is a genuine gameplay member, Task 37 is a real defect and
stays reverted permanently.

Either outcome is worth having beyond Task 37: this gate currently blocks every
future performance change, and Task 44 already worked around it by proving
exactness with the Task 36 replay word stream instead.

### Exactness — FAILED, and this is why the task does not ship

`scripts/verify-task37-itcm-state-hash-ab.ps1` runs the full match lifecycle on
both builds with the Task 9 per-update state-hash instrument and requires every
record to match. **It does not match.** 692 of 3,892 update records differ
(17.8%), first at update 1412.

Per the kill criteria above — "Any state-hash divergence → REVERT immediately" —
this is not a KEEP. `NDS_TASK37_ITCM_LEAVES` stays 0 in every target, the
published ROM is unchanged at `9E27BD3D...`, and nothing merges.

The evidence says this is probably an instrument limit rather than a real
divergence, but "probably" is not the standard for changing a published ROM, and
redefining the gate after seeing its result would make it worthless:

```
divergence structure                     off vs on
  updates    0..1411   identical         (1,412 records)
  updates 1412..1733   differ            (322)
  updates 1734..1991   identical         (258)
  updates 1992..2327   differ            (336)
  updates 2328..2500   identical         (173)
  updates 2501..2534   differ            (34)
  updates 2535..3891   identical         (1,357 consecutive)
```

- The hash covers `syUtilsRandSeed()`. A divergent RNG stream cannot re-converge,
  and this re-converges three times — so both builds are drawing the same random
  numbers in the same order.
- Heap offset (57,168) and GObj count (646) are byte-identical in **all 3,892**
  records, so allocation behaviour is identical throughout.
- A genuine simulation divergence — two fighters at different positions — does
  not return to bit-identical whole-state three separate times, once for 1,357
  consecutive updates.

That pattern is a transient, self-healing quantity inside the hash, and the most
likely source is a GObj field touched by the draw path rather than by logic. Two
of the seven moved symbols are renderer functions and the measured win is
concentrated in STG, so a change that alters when frames are presented would
perturb exactly such a field and then let it settle.

Supporting circumstantial point: **Task 44, also a performance change, did not
use this gate.** It proved exactness with the Task 36 replay word stream (3,916
words, mask 0xA1, zero fallback) instead. That choice looks deliberate in
hindsight.

**What would settle it** — the instrument already tags its inputs by region
(`NDS_TASK9_STATE_RECORD_SCENE` / `_BATTLE` / `_CAMERA` / `_GROUND` /
`_CONTROLLERS` / `_COLLISION_*` / GObjs). Export a per-region hash instead of one
combined pair and the diverging region names itself in a single run. If it is
GObj draw-state, the fix is to exclude draw-touched fields and the gate becomes
usable for every future perf task; if it is a gameplay region, Task 37 is a real
bug and stays reverted. That work belongs to the instrument, not to this task,
which is why it was not done here under the flag of a task that benefits from
the answer.

### Publication — NOT shipped

Flag stays 0 everywhere. Published ROM unchanged: `smash64ds-battle-playable-hwtri.nds`,
11,428,864 bytes, `9E27BD3D5DCBE00DC72A47221CFDD170FFE690BC1516F0B16241029F937CE369`.
`DECOMP_PIN.txt` and the README pin are unchanged from master.

For the record, the enabled build was produced and measured: it links (ITCM
31,676 → 32,596 of a 32,736 hard cap) and its published-target hash would be
`1818AA775DCFFD52C82B35ED3D4FA6C6D02FCE232E9EE70D9B3F1DA3FDF54207`, 11,428,864
bytes. The lean ROM does not embed `NDS_TASK10_GIT_SHORT`, so that hash is
commit-stable if the task is later revived.

HUD row 23 gains an `L` digit for `NDS_TASK37_ITCM_LEAVES` so a device session
can confirm which ROM booted: `GIT <hash> A<arena> S<t44> C<t36> L<t37>`. That
row costs nothing at `L0` and is kept.

### Phase 4 — retail queue (built, but NOT queued for a session)

`builds/device-queue/task37-itcm-leaves-pair/` holds the pair, targets
`smash64ds-battle-playable-task37-{on,off}-hwtri`. It is left in place because it
is already built and costs nothing to keep, but **it should not be run until the
exactness question is settled** — device time spent confirming a speedup that
might be riding on a behaviour change is wasted either way.

If it is run: the number to check is **not** the bucket medians but the 3-VBlank
share. The emulator says 71.7% → 76.0%; retail sits closer to the cliff (47.7%
in the matched 2026-07-22 pair), so the same tick saving should move more
distribution there, not less.

## Verdict

**WIP / NOT MERGED.** Branch `codex/task37-itcm-repack` is the checkpoint.

What is proven and worth keeping regardless of what happens to the repack:

- The census method and tooling, including the memory/non-memory stall split
  that separates a placement target from an algorithmic one.
- `.text.hot` is not a working tier (30.0% non-memory stall vs `.main`'s 29.5%).
  This is independent of the repack and should inform any future hot-text plan.
- `.itcm` is fully enumerated and holds 5,040 bytes that never execute.
- The Phase 0 instrument, which costs the published ROM nothing (hash-verified).

What is measured but unproven: the −59,328 tick P50 win and the +4.3-point
3-VBlank shift are real measurements of these two ROMs, but until the exactness
gate is resolved it is not established that the candidate ROM is playing the
same game.

## Known context

- `verify-dev-fast.ps1` is currently red on the `battle_playable` locked-30
  pacing contract under the fork. Proven emulator-caused by stashing all edits
  and reproducing on clean `093690b`. Do not attribute it to this task; do not
  "fix" it by reverting the emulator.
