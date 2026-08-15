# `NDS_R2_FIGHTER_GX_COMPOSE` re-measured on the repaired tree: −17,152 at rank-80, stack flat, blink signature absent

Date: 2026-08-15. Branch `codex/r2-runtime2`. Base HEAD `771cd4b8312`.
Premise: `../2026-08-15_gxstack-io-draw/GXSTACK_IO_DRAW.md` (the leak fix) and
`plan.md` §8 / §K-GXFIX.
Prediction written before the first run: `PREDICTION.md` (same directory).

**UNITS: 2 profile cycles = 1 project tick.** Requirement **+32,593 net at
rank-80** against bank `c170` (1,177,920 raw / 1,152,973 net; apparatus 24,947).
**rank-80 = the 80th-largest `WORK-H` of 1,600 samples**, computed from the rows
CSV; the harness's own `p95` column uses a different estimator and is quoted
beside it, never instead of it.

**Cycle stopped early on the owner's instruction (campaign paused). Task A is
complete and measured; Task B was not started. §6 says exactly what is missing.**

## 0. Outcome first

1. **The slice pays, and the delta clears its floor.** Within the pair,
   `WORK-H` rank-80 **1,189,312 → 1,172,160 = −17,152**; `WORK-H` P50
   **947,360 → 939,264 = −8,096**; whole-match mean **−10,737**. The
   cross-build floors this has to beat are `VERIFYING.md`'s cycle-100
   calibration: **P95 ≥14,080 (sign unreliable)** and **P50 ~5,700 (sign kept
   in all three pairs)**. Both are cleared, in the same direction.
2. **It lands in the bucket it aims at.** `FTR` rank-80 −9,664 / P50 −8,960 /
   mean −11,399, with every other named bucket flat except `STG`, which moves
   the wrong way (+3,200 rank-80 / +2,675 mean) and is **not attributed**.
3. **The control reproduces itself exactly, across builds and sessions.**
   `build-c184-gxc-a` (fresh, this session) and `build-c183-gxstackfix` (built
   and run in the previous session, same source, `NDS_TASK10_GIT_SHORT`
   "ba2c5e5" vs "771cd4b") read rank-80 **1,189,312 on both**, mean 967,926 vs
   968,252, over-gate 130 vs 131. **The control side of this comparison is not
   a placement artifact.**
4. **The acceptance condition the flag was withdrawn on now holds at flag 1.**
   `gNdsHardwareRendererStatus` reads **0x06000000** — position/vector stack
   level **0**, projection level **0**, error bit **clear** — at all **17
   whole-match ring stops (frames 439..2039)** and on all **128 consecutive
   per-frame samples (438..565)** of the flag-1 arm.
5. **The blink's own measured signature is absent.** The withdrawal
   (`e03ae311204`, and the note at `nds_platform.c:3270-3288`) records the
   blink as *low-polygon frames* — 145/165/165/106/306 against a 378 median, at
   the 32-frame stack wrap. Over 128 frames (four full wrap periods) the flag-1
   arm holds `GFX_POLYGON_RAM_USAGE` at **432 / 463.5 / 510** (min/median/max)
   with **zero frames below 350**, against the same-session control's **432 /
   464.5 / 510**, also zero.
6. **Engagement is total and the flip budget is zero.**
   `gNdsR2GxComposeDeclines` **0** whole match; all six end-of-match invariants
   equal the `c170`/`c174`/`c175`/`c176`/`c183` bank on both arms.
7. **What is NOT here: a frame-locked pixel pair.** Both capture ROMs were
   built and neither was captured — the cycle was stopped first. §6.

---

## 1. The arms

Both fresh at HEAD `771cd4b8312`, target
`smash64ds-battle-playable-tickhud-hwtri`, flags
`NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1`,
DLDI on, `NDS_TICK_HUD_DRAW=1`.

| arm | build dir | `NDS_R2_FIGHTER_GX_COMPOSE` |
|---|---|---|
| **A** control | `build-c184-gxc-a` | 0 |
| **B** candidate | `build-c184-gxc-b` | 1 |

`nds_build_config.h` of the two differs in **exactly one define**
(`NDS_R2_FIGHTER_GX_COMPOSE`), machine-diffed, and A's config differs from
`build-c183-gxstackfix`'s in **exactly one** (`NDS_TASK10_GIT_SHORT`).

### 1.1 The flag is compile-time, so the one-byte pair is impossible — stated, not hidden

`NDS_R2_FIGHTER_GX_COMPOSE` selects code with `#if`, in six `#if` blocks across
`nds_renderer.c` and `reloc_backend_renderer_dl.c`. There is no `.data` word to
poke, so `-SetGlobals` cannot express this A/B and the pair **must** be two
separately linked ROMs. `compare-elf-sections.py` was run anyway, to *size* the
placement exposure rather than to claim there is none
(`elf-sections-a-vs-b.txt`):

```text
.itcm            32,152        SIZE   A 32,152 vs B 32,188
.text.hot         4,588         243 differing bytes
.text.hot.draw    5,268         106
.main           927,256        SIZE   A 927,256 vs B 929,296
.main.rw        137,428      16,530
.dtcm             8,800           0
```

**This is a genuine relink and the placement floor is NOT zero.** That is why
the verdict below rests on three things and not on rank-80 alone: the P50 delta
(over its own smaller floor, and P50 kept its sign in all three calibration
pairs), the bucket attribution, and the control's exact self-reproduction.

### 1.2 How the arm was expressed, and why it cannot reach a published ROM

The tick-HUD/proof Makefile block pinned the flag with a bare
`override … := 0` (`Makefile:1733`, from the withdrawal commit), so the arm was
previously unreachable without hand-editing a pin. The pin now reads a
single documented escape:

```make
override NDS_R2_FIGHTER_GX_COMPOSE := $(if $(filter 1,$(NDS_R2_FIGHTER_GX_COMPOSE_LAB)),1,0)
```

Probed on the actual Makefile before any build was spent:

| TARGET | `…_LAB=0` | `…_LAB=1` |
|---|---|---|
| `smash64ds-battle-playable-tickhud-hwtri` | 0 | **1** |
| `smash64ds-battle-playable-proof-hwtri` | 0 | **1** |
| **`smash64ds-battle-playable-hwtri`** (published) | 0 | **0** |

The published block's `override … := 0` (`Makefile:1507`) is untouched and
unconditional, so **no published target can carry the slice whatever is passed
on the command line.** The shipping default is unchanged and was not flipped.

---

## 2. The measurement

Instrument: `sample-tick-hud-buckets.ps1 -NoBuild -RingDump -Samples 1600
-StartFrame 439`, frames **440..2039**, 1,600 samples, both arms, same
invocation, `slips=0` on both.

### 2.1 `WORK-H`, the gate series

| statistic | A (flag 0) | B (flag 1) | delta |
|---|---:|---:|---:|
| **rank-80 (P95 of 1,600)** | **1,189,312** | **1,172,160** | **−17,152** |
| harness `p95` estimator | 1,187,648 | 1,168,960 | −18,688 |
| P50 | 947,360 | 939,264 | **−8,096** |
| P90 (rank-160) | 1,099,712 | 1,090,752 | −8,960 |
| top-1% (rank-16) | 1,528,704 | 1,513,920 | −14,784 |
| whole-match mean | 967,926 | 957,189 | −10,737 |
| frames over 1,120,380 (of 1,600) | 130 | 129 | −1 |
| max | 5,113,280 | 2,325,952 | (see §2.4) |

Net of the approved apparatus (24,947): **A 1,164,365 → B 1,147,213.**

**Two readings of what remains, and they disagree by more than the win.** The
requirement `+32,593` is stated against bank `c170` (1,177,920 raw at rank-80).
This session's control reads **1,189,312**, i.e. **+11,392 above that bank** —
inside the ≥14,080 cross-build floor, so neither level is wrong, but they cannot
both be used. So:

```text
applied to the c170 bank:      32,593 - 17,152 = +15,441 net still to find
banked on THIS pair's own B:   1,147,213 net vs 1,120,380 = +26,833 net still to find
```

**The quotable number is the within-pair −17,152 = 0.53x of the requirement.**
Re-banking the *level* needs a decision about which control is the bank, and
this cycle did not make it.

### 2.2 Where it lands, by bucket (rank-80 of each series, and whole-match mean)

| bucket | A rank-80 | B rank-80 | delta | mean delta |
|---|---:|---:|---:|---:|
| **FTR** | 331,264 | 321,600 | **−9,664** | **−11,399** |
| **STG** | 183,936 | 187,136 | **+3,200** | **+2,675** |
| SRC | 540,224 | 536,128 | −4,096 | −723 |
| MISC | 182,336 | 181,376 | −960 | −1,018 |
| GCRA | — | — | — | −628 |
| SINT | — | — | — | −1,919 |
| BG / AUD / SHDT / SCPU / SPHD | — | — | — | ≤ ±370 |
| WAIT (idle) | — | — | — | **+7,945** |
| ALL (VBlank-quantized) | 1,678,208 | 1,678,080 | −128 | −2,748 |

`FTR` is the fighter draw bucket and it is the only one that moves materially in
the intended direction; `WAIT` rising by about the amount `WORK-H` falls is the
expected shape for work removed from a VBlank-paced loop.

**`STG` +3,200 at rank-80 is a real cost this change carries and I have not
attributed it.** Three candidates, none measured: the relink moved the stage's
text (`.main` grew 2,040 B); the stage's first matrix load now follows a
different GX matrix-mode/projection state left by the fighter path; or GX FIFO
contention from ~1,288 extra words a frame (§2.3) is billed to whichever bucket
is open. **Do not price any of these — pick one only with a measurement.**

### 2.3 Engagement, from the same run that produced the ticks

Whole-run counters, arm B, 2,039 presented frames. **Every one of these symbols
is absent from arm A's ELF** (`nm`) because the `#if` removes them, which is a
stronger negative control than a zero.

| counter | whole run | per frame | derived |
|---|---:|---:|---|
| `gNdsR2GxComposeDeclines` | **0** | 0 | **no binding fell back to the CPU compose** |
| `gNdsR2GxComposeCaptures` | 63,364 | 31.08 | bindings described |
| `gNdsR2GxComposeRoots` | 63,364 | 31.08 | equals Captures exactly |
| `gNdsR2GxComposeLocals` | 110,702 | 54.29 | 1.747 locals per binding |
| `gNdsR2GxComposeMults` | 110,702 | 54.29 | equals Locals exactly |
| `gNdsR2GxComposeRestores` | 55,546 | 27.24 | 87.7% of bindings restore a parent |
| `gNdsR2GxComposeStores` | 41,598 | 20.40 | 65.6% of bindings store a palette slot |
| `gNdsR2GxComposeProjectionSkips` | 59,414 | 29.14 | **93.8% of root loads skip the projection push** |

The GX word bill this implies, **as a count, not a price**: 12 words per
`MATRIX_MULT4x3` (651.5/frame) + 16 per world-scale `MATRIX_MULT4x4`
(497.2/frame) + 16 per non-elided projection push (31.0/frame) + 61.3/frame of
seed loads + 27.2 `MATRIX_RESTORE` + 20.4 `MATRIX_STORE` ≈ **1,288 GX words per
frame added**. **The measured net is −11,399 `FTR` mean; this cycle did not
measure either side of that exchange separately, so no cycles-per-word figure is
derivable from it and none is stated.** (`fifo-word-price-depends-on-queue`: two
writes in the same function have measured 3.5x apart.)

### 2.4 What is NOT a result here

- **The `max` column.** A 5,113,280 vs B 2,325,952 is the known 2^22-tick
  single-bucket artifact (`K0_RERANK` §: every one is 4,194,304 ticks in
  whichever bucket was open, ~1 per 2,100 presented frames, lands on different
  frames on different arms of the same build). It has never touched a banked
  percentile and it does not here — rank-16 moves −14,784, in line with the rest.
- **Cadence.** A `VBI 2:1739 3:277 4:14 5+:9 max 20` (85.29% two-VBlank);
  B `2:1738 3:284 4:8 5+:9 max 19` (85.24%). **Flat.** This is the
  `NDS_TICK_HUD_DRAW=1` arm and §6 of `plan.md` reads cadence from `DRAW=0`, so
  this is a sanity check, not the cadence figure, and it did not move.

---

## 3. Correctness

### 3.1 The matrix stack, at flag 1

| arm | window | stack level | projection level | error bit |
|---|---|---|---|---|
| B (flag 1) | 17 ring stops, frames 439..2039 | **0 at every stop** | 0 | **0** |
| B (flag 1) | 128 per-frame samples, 438..565 | **0 on 128/128** | 0 | **0 on 128/128** |
| A (flag 0) | 17 ring stops + 128 per-frame | 0 | 0 | 0 |

`gNdsHardwareRendererStatus` reads `0x06000000` on every sample of both arms.
For contrast, **every Boundary log in `artifacts/` from 2026-08-03 to
2026-08-15 read `0x6009600`** — level 22, error set (`GXSTACK_IO_DRAW.md` §1.5).

### 3.2 The blink detector

`GFX_POLYGON_RAM_USAGE`, 128 consecutive presented frames 438..565 (four full
32-frame wrap periods), both arms, same session:

| arm | poly min / median / max | frames < 350 | vertex min / median / max |
|---|---|---:|---|
| A (flag 0) | 432 / 464.5 / 510 | **0** | — |
| B (flag 1) | 432 / 463.5 / 510 | **0** | 975 / 1099.5 / 1169 |
| *(the blink, as recorded 2026-08-11)* | *106..306 against a 378 median* | *5 of 128* | — |

**This is the sharpest instrument the repo has for this defect** — the
withdrawal commit records that a 96-frame triangle census could NOT see the
blink (Mario held at exactly 320 submitted triangles on every frame while the
fighter vanished), and that accepted-polygon count is precisely the difference.
It is clean.

### 3.3 The fight is the same fight

End-of-match invariants, both arms, against the `c170`/`c174`/`c175`/`c176`/
`c183` bank:

```text
gNdsBattleTextHudP1Damage            76   ==  bank
gNdsDamageSparkScaleCount            15   ==  bank
gNdsShieldAnimJointAttachCount    1,352   ==  bank
gNdsAObjEvent32NormalizedHighWater 1,266  ==  bank
gNdsBattlePackHits                  197   ==  bank
gNdsObjAnimRunawayCount               0   ==  bank
gNdsRendererTask36CaptureOutcome      2   ==  A
gNdsRendererTask36CaptureSegmentMask 161  ==  A   (0xA1, three replayed segments)
```

All eight identical on A and B, so `route-ab-cannot-price-gameplay-change` does
not apply: the two arms played the same match and the tick delta is a tick
delta.

---

## 4. The prediction, graded

`PREDICTION.md`, written before the first run.

| # | predicted | measured | verdict |
|---|---|---|---|
| 1 | `Declines` 0, `Captures`/`Roots` > 0, symbols absent on A | 0 / 63,364 / 63,364, absent on A | **right** |
| 2 | stack level 0 and error 0 on every sample at flag 1 | 0 / 0 on 17 stops and 128 frames | **right** |
| 3 | P50 −6,000…−14,000 (point −10,600) | **−8,096** | **inside, point estimate 2,504 high** |
| 3 | rank-80 −6,000…−20,000 (point −13,600) | **−17,152** | **inside, point estimate 3,552 low** |
| 4 | net implies CPU side ≈ 2x the FIFO side | not split — no per-side measurement taken | **not answerable this cycle** |
| 5 | pixels identical | **not measured** (§6) | **open** |
| 6 | invariants equal the bank | all eight equal | **right** |

Five of six predicted items were measured and none was wrong. The band on
item 3 was deliberately wide (±40%) because the cross-build floor on this pair
is ≥14,080; a narrower band would not have been honest, and the measured value
landing 26% above the point estimate is inside it, not a hit.

---

## 5. Method notes worth keeping

1. **A control that reproduces itself across builds and sessions is worth more
   than a third arm.** `c183` and `c184-gxc-a` are different ELFs (the embedded
   git string differs), built and run a session apart, and read rank-80
   **1,189,312 on both**. For a compile-time flag — where the falsifier third
   arm is impossible by construction — this is the substitute, and it costs one
   run of a build you already need.
2. **`GFX_POLYGON_RAM_USAGE` is the blink oracle; submitted triangles are not.**
   Any future GX-state change should be graded on accepted polygons over ≥128
   frames (four wrap periods), not on a submission counter.
3. **A pinned Makefile flag needs an escape, not a hand edit.** The pin existed
   so a lab arm could not drift from the published one; hand-editing it to run
   the lab arm is how the pin stops meaning anything. One `$(if $(filter …))`
   makes the lab arm expressible and leaves the published block unconditional —
   and the three-target probe that proves it costs no build at all.
4. **Builds here are 75 seconds and a whole-match gate run is ~150 seconds.**
   Several planning decisions in recent cycles were sized against a much older
   13-minute build figure.

---

## 6. What this cycle did NOT do

The campaign was paused mid-cycle by the owner. Stated plainly:

1. **No frame-locked pixel comparison was taken.** Both capture ROMs exist and
   are byte-verified as the right arms — `build-c184-cap-a` (proof target,
   `GX_COMPOSE 0`, `BOTH_CPU 0`, `TICK_HUD 0`) and `build-c184-cap-b`
   (identical but `GX_COMPOSE 1`) — and no `capture-melonds.ps1
   -ExactTimeRemain` run was started. **So "pixel-identical" is UNMEASURED on
   this tree.** Slice 43's original claim ("frame-locked captures at match tic
   3000 are pixel-identical") was made *before* the blink was found and must not
   be inherited as if it had been re-taken. The remaining acceptance work is one
   `-ExactTimeRemain` pair per arm plus the same-build adjacent-present floor
   beside it (`capture-cut-g-exact-frames.ps1` documents why the floor is
   mandatory, and that at tic 3000 the floor is 30.85%).
2. **Task B — the two draw-lane counters — was not started.** No source was
   edited for it; no counter exists that did not exist before. The design work
   done is recorded in §7 so the next cycle does not repeat it.
3. **The flag was not flipped and nothing was re-banked.**
   `NDS_R2_FIGHTER_GX_COMPOSE` stays `?= 0` and the published block still pins
   it to 0 unconditionally. Flipping a shipping default is
   `BLOCKED(decision: shipping default)` and it is the owner's, even with this
   measurement and even if the pixel pair comes back clean.
4. **No v3 stall capture.** The FIFO-vs-CPU exchange rate for this slice is
   therefore unknown, which is the one place a future retraction could come
   from (§2.3).

## 7. Inherited design for Task B, so it is not re-derived

Both counters are for the draw lane's two surviving "recomputed unchanged"
candidates (`GXSTACK_IO_DRAW.md` §4.2). Neither was implemented.

- **`ndsRendererSyncTextureTile` (`nds_renderer.c:7139-7182`), 8,867 tk/fr at
  72.68 calls/marginal frame.** Read the body before sizing it: it is **not a
  VRAM sync**. It copies 19 fields from `stats->texture_tiles[tile_index]` into
  the `stats->texture_render_*` fields of the same struct, plus two `set_seen`
  tests. That is consistent with its 4,943 tk/fr of `write_buffer` and it means
  the redundancy test is "same `stats`, same `tile_index`, and the source
  `NDSRendererTileState` unchanged since that struct's last sync" — exact, via a
  saved copy plus `memcmp`, which is affordable in a lab build because only the
  *counts* are wanted. Four call sites, and splitting by site is free and
  worth doing (increment a per-site counter beside each call rather than adding
  a parameter, which would change codegen): `ndsRendererRecordTextureState`
  (`:7113`), `ndsRendererRecordSetTile` (`:7242`),
  `ndsRendererRecordSetTileSize` (`:7378`),
  `ndsRendererHardwareResolveOrBindTexture` (`:17634`).
- **The bind collapse.** `ndsRendererHardwareBindTextureName`
  (`nds_renderer.c:12366-12385`) **already elides** on
  `sNdsRendererHardwareBoundTextureName != texture_name`, so the measured
  103.45 requests → 55.73 `glBindTexture` calls is that elision working at 46%.
  The counter that would name a waste item is therefore not "redundant binds"
  (those are already free) but **whether the surviving 55.73 revisit names
  already bound earlier in the same frame** — i.e. an ordering problem — versus
  a genuinely non-repeating sequence. Suggested split: requests / zero-name
  early-outs / elided / issued / issued-with-a-name-already-bound-this-frame,
  the last via a bounded per-frame name set with an explicit overflow counter so
  it fails loudly. Frame boundary is available without a new hook by watching
  `sNdsRendererHardwareFrameSerial`.
- **Publication discipline** (`plan.md` §9 law 1): declare in
  `include/nds/nds_renderer.h`, define beside the code they count with
  `__attribute__((used))` (the repo's own precedent —
  `nds_battlepack_anim.c:6-14`, `nds_reloc_assets.c:158-167` — rather than a
  central `diagnostics.c`), `nm`-verify against `--gc-sections`, gate behind a
  new default-0 census flag, and prove the shipping default is byte-unaffected
  with an ELF/ROM diff (the embedded git hash always differs in one 7-byte run
  at `0x0c8fc0`). `NDS_TASK93_TEXKEY_CENSUS` was checked and does **not** cover
  either of these: it counts key rebuilds inside
  `ndsRendererHardwareResolveOrBindTexture`, which runs 0.175 times a frame.
