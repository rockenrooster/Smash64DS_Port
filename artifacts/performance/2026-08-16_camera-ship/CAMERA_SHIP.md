# The camera chain ships in Q20.12 on the owner's decision, the last two leading polls came out with it, and the pair is worth −6,336 tk/fr on one binary

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `f5e13aa3e27`**
1 lab build (`build-c220-camship`), 4 whole-match gate runs, 0 gameplay values changed,
no ROM published, both root ROMs byte-unchanged.
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.** Every table states
its window.

```text
DECISION  THE OWNER ACCEPTED THE ARM.  "I think camera fixed point is ok",
          after playing build-c205-camtoggle and saying of the picture
          "otherwise it looks fine".  That is the acceptance
          CAMERA_Q20_12.md section 8 was BLOCKED on -- a 6.5350% top-screen
          pixel delta against a same-build adjacent-present floor of 35.2217%,
          the first draw-side precision ceiling this project has set.
          NDS_R2_CAMERA_FIXED ?= 0 -> 1.  Nothing here chose it.

PRICE     -6,336 tk/fr PAIRED MEDIAN, ON ONE BINARY, ZERO PLACEMENT FLOOR.
          build-c220-camship, route 0 (float, the old default) against the
          shipped default:
            paired median, whole run        -6,336   1,493/1,600 improve
            paired median, marginal-80      -7,264   69/80 improve
            rank-80                         -8,896   1,219,520 -> 1,210,624
            ranks 41-120 band               -5,425
            P50                             -6,624
            complement  WORK-H -6,336 / WAIT +6,272 / ALL +0
          Section 3.

SPLIT     THE FLIP AND THE POLLS, SEPARATED ON A SECOND SAME-BINARY ROUTE.
          The same route on the PREVIOUS basis binary (polls still in) reads
          -3,776 paired median / -3,072 rank-80.  Difference of differences
          -2,560 for the poll removal, against 2,721 PREDICTED before the run
          from HWROUTE.md's per-call figures at two unrelated sites.  Right
          sign, 94% of the predicted size -- but it carries a residual
          cross-build term and the paired-median cross-build floor is ~5,440,
          so it is CORROBORATION, NOT A PRICE.  Section 4.

NEW BASIS build-c220-camship, rank-80 1,210,624 raw / 1,185,677 net.
          REQUIREMENT +64,977 -> +65,297, i.e. UP 320.  That is not a
          regression: this binary's OWN float arm reads +74,193 on the same
          window, so the change moved the shipping binary -8,896 at rank-80
          and the +320 is placement inside a >=14,080 floor.  The route is the
          price; this is the level.  Section 5.

ENGAGE    PROVEN FIRING AND PROVEN INERT, ON FOUR ARMS.  Shipped default:
          8,152 fixed look-ats, 8,228 fixed perspectives, FLOAT CALLS ZERO --
          every call from frame 1, because a build default has no pre-poke
          frame.  Saturate 0, degenerate 0, rescale 0 on all four arms.
          Section 6.

BYTES     .main 930,960 -> 930,928, -32 B.  .itcm unchanged at 32,516.
          The flip itself costs nothing: both arms were already linked and the
          route word is `.data`.  Section 7.

FILTER    THE 2^22 FILTER RAN LIVE FOR THE FIRST TIME, ON ALL FOUR RUNS, AND
          IT IS LOAD-BEARING.  5 / 2 / 2 / 1 corrected samples.  The artifact
          is DETERMINISTIC PER ARM but lands on DIFFERENT FRAMES in different
          arms, so an uncorrected pair would have been ~3 x 4.19M apart in the
          tail.  It is also why the inherited basis restates.  Section 2.
```

---

## 1. Basis, stated once

| | |
|---|---|
| target | `smash64ds-battle-playable-tickhud-hwtri` (the measurement instrument) |
| builds | `build-c219-animitcm-ship` (previous basis, both arms linked), `build-c220-camship` (this cycle) |
| config | `NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1`, GX_COMPOSE 0 (shipping), bore 0, DLDI on |
| match | Boundary `battle_playable_realtime`, mode 163, one minute |
| window | 1,600 samples, frames 439–2038, `-RingDump`, **`slips=0` on all four runs** |
| series | `WORK-H`; rank-80 = 80th-largest of the run's own 1,600 rows; net = raw − 24,947 apparatus; gate 1,120,380 |
| route | `gNdsR2CameraFixedEnabled`, a `.data` word, poked with `-SetGlobals`; `readback == requested` and `stuck: true` on every poked run |

**Both pairs are same-binary and the harness proves it.** `romSha256` from each run's JSON:
`r0` and `r1` both `9C84CE61…`; `ship220` and `s220a0` both `5AE3F716…`. (The header line's
`sha=DE80E46B…` is the **melonDS binary's** hash and is identical on every run in the tree —
it is not arm identity.)

---

## 2. The inherited basis restates, and the filter is why

`ANIM_ITCM.md` §5 published `build-c219-animitcm-ship` at rank-80 **1,216,896** raw /
1,191,949 net = **+71,569**. On the wrap-corrected data the same run reads **1,210,304** raw
/ 1,185,357 net = **+64,977** — the figure this cycle inherited and the one it is measured
against. The 6,592 difference is exactly the artifact's rank-80 effect.

**Verified rather than assumed.** A fresh run of the *previous* build with
`-SetGlobals gNdsR2CameraFixedEnabled=0` reproduces `ANIM_ITCM.md`'s `ship-rows.csv`
**byte for byte on all 1,600 frames** — paired median +0, min +0, max +0 — once the
sampler's own correction is applied offline to the older CSV. Three things fall out of one
run:

1. **The instrument is exactly deterministic**, tail included, confirming `IO_AUDIT.md` §3.
2. **`-SetGlobals` costs nothing**: the apparatus is identical with and without it, so the
   old un-poked basis run and a poked control arm are the same measurement.
3. **The offline correction is the sampler's**, to the tick.

### 2.1 What the filter did live, and the new thing it shows

| run | arm | corrected samples | frames |
|---|---|---:|---|
| `r0` | c219 float | 5 | 1056, 1357, 1629, 1750, 1961 |
| `r1` | c219 fixed | 2 | 905, 1020 |
| `ship220` | c220 fixed (shipped) | 2 | 1464, 1849 |
| `s220a0` | c220 float | 1 | 1313 |

Every correction lands where the warning says it must: four of the ten corrected `ALL`
values sit within 260 of the run's own median 1,117,632, and the rest land on 1,677,568–
1,678,272, next to the runs' own `ALL` P95 of 1,678,208 — genuinely heavy frames that also
caught the artifact.

**The new observation: the artifact is deterministic *within* an arm and different
*between* arms.** `r0` reproduced the old basis's five artifact frames exactly, yet the
other three arms caught two, two and one, on six other frames. An uncorrected `r0`/`r1` pair
would have carried **three spurious 4,194,304-tick rows** on one side and none on the other.
The filter is not a tidying pass; without it this cycle's tail comparisons would have been
unreadable.

---

## 3. The price, on the binary that ships

`build-c220-camship`, one ROM (`5AE3F716…`), one poked `.data` word.

| statistic | route 0 (float) | shipped default (Q20.12) | Δ |
|---|---:|---:|---:|
| **paired median, whole run** | — | — | **−6,336** (1,493/1,600 = 93.3%) |
| **paired median, marginal-80** | — | — | **−7,264** (69/80) |
| rank-80 raw / net / level | 1,219,520 / 1,194,573 / +74,193 | 1,210,624 / 1,185,677 / **+65,297** | **−8,896** |
| ranks 41–120 band mean | 1,223,781 | 1,218,356 | −5,425 |
| P50 | 936,000 | 929,376 | −6,624 |
| trimmed (8/8) paired mean | — | — | −6,487 |

**The complement is the control and it closes to one sampling quantum.** `ALL` is
VBlank-quantised (`[[all-is-a-quantized-gate]]`), so at a fixed presented cadence a genuine
work deletion must appear as work down and idle up by the same amount:

```text
WORK-H  -6,336      WAIT  +6,272      ALL  +0
```

**Bucket attribution lands exactly where the two converted producers run**, and nowhere else:

| bucket | paired median | frames improved |
|---|---:|---|
| `FTR` (fighter display contract — the game camera) | **−4,416** | 1,590/1,600 = **99.4%** |
| `STG` (stage + fighter matrix prep — the adapter camera) | **−1,856** | 1,446/1,600 = 90.4% |
| `SRC`, `SINT`, `GCRA` (animation, simulation, GC ranges) | **0** | ~47% (noise) |

−4,416 + −1,856 = −6,272 against `WORK-H`'s −6,336. This reproduces `CAMERA_Q20_12.md`
§3.1's `FTR −3,264` / `STG −1,664` signature on a different build, at the larger size the
poll removal buys.

### 3.1 The rank curve is a level

```text
rank      1      5     10     20     40     80    120    160    320    640    800   1200   1600
delta -102.4k  -6.0k +64.1k -19.8k  -3.4k  -8.9k  -6.5k  -3.2k  -9.3k  -6.5k  -6.6k  -4.9k  -5.4k
```

Every rank from 20 down reads −3,200 to −9,344, as a per-frame-constant workload must.
Ranks 1 and 10 are the load-frame population that **permutes between arms** — the pattern
`IO_AUDIT.md` §1 identified and `CAMERA_Q20_12.md`'s correction restated: rank-by-rank
differencing there measures the permutation, not a cost. The paired per-frame median is
immune by construction and is what this document leads with.

---

## 4. Separating the flip from the polls

The two changes were built together, so a second same-binary route on the **previous** basis
binary (`9C84CE61…`, polls still present) isolates the flip alone:

| | c219 route (flip only) | c220 route (flip + polls out) |
|---|---:|---:|
| paired median, whole run | **−3,776** (88.1%) | **−6,336** (93.3%) |
| paired median, marginal-80 | −4,544 (67/80) | −7,264 (69/80) |
| rank-80 | −3,072 | −8,896 |
| P50 | −4,352 | −6,624 |
| complement | WORK-H −3,776 / WAIT +3,776 / ALL +0 | WORK-H −6,336 / WAIT +6,272 / ALL +0 |
| `FTR` / `STG` | −2,432 / −1,280 | −4,416 / −1,856 |

**Difference of differences: −2,560 for the poll removal.**

**Predicted before the run, at 2,721 tk/fr**, from `HWROUTE.md` §2's per-call figures
measured at two unrelated live sites (43.5 tk for a divide's leading poll at 6.00
iterations, 23.0 tk for a root's at 3.00) and this cycle's own engagement counts:

```text
look-ats     8,152 / 2,038 frames = 4.000/fr  x (9 divides + 3 roots)
perspectives 8,228 / 2,038 frames = 4.037/fr  x  5 divides
             --------------------------------------------------------
             56.2 divides/fr x 43.5  +  12.0 roots/fr x 23.0  =  2,721 tk/fr
```

The mechanism is why the transfer is legitimate rather than a borrowed constant: the leading
poll waits out **a restart the function itself caused** — GBATEK, writing `DIVCNT` raises
bit 15 — so it costs a fixed unit latency independent of call context, which is exactly what
6.00 and 3.00 constant iteration counts at two unrelated sites already showed.

**It is not banked.** The two route deltas come from two separately-linked binaries, so the
difference carries a residual placement term, and the measured cross-build paired-median
floor on this campaign is ~5,440 (`CAMERA_Q20_12.md` §3.3). −2,560 is inside it.
`[[a-residual-divided-by-a-count-is-not-a-price]]`, in its difference-of-differences form.
**What is banked is the −6,336 for the pair**, which is same-binary and floor-free.

---

## 5. The level, and why it moves the wrong way by 320

| | rank-80 raw | net | level | ranks 41–120 | P50 |
|---|---:|---:|---:|---:|---:|
| `c219-animitcm-ship` (basis, wrap-corrected) | 1,210,304 | 1,185,357 | **+64,977** | 1,217,946 | 933,344 |
| **`c220-camship` (new basis)** | **1,210,624** | **1,185,677** | **+65,297** | 1,218,356 | 929,376 |
| cross-build Δ | **+320** | | | +410 | −3,968 |

**Do not read anything into +320.** The floor is visible directly in this cycle's own data:
`c220`'s **route-0 arm** — the float chain, byte-identical in behaviour to `c219`'s float
chain — reads rank-80 **1,219,520** against `c219`'s **1,210,304**. That is **+9,216 for
two builds whose float behaviour is identical**, i.e. the ≥14,080 cross-build placement
floor, measured on this pair, on this day, on this window. `ANIM_ITCM.md` §5 measured the
same term at +12,864.

So the arithmetic is: this binary is placed ~9,216 worse at rank-80, the change wins 8,896
back on it, and the residue is +320. **On the binary that ships, the change is worth −8,896
at rank-80 and −6,336 at the paired median.** The requirement is stated at the ship build's
own rank-80 by campaign convention, so it reads **+65,297**.

---

## 6. Engagement, and the negative control

| counter | c219 route 0 | c219 route 1 | **c220 shipped** | c220 route 0 |
|---|---:|---:|---:|---:|
| `…FixedLookAtCalls` | 0 | 8,148 | **8,152** | 4 |
| `…FixedPerspCalls` | 0 | 8,224 | **8,228** | 4 |
| `…FloatLookAtCalls` | 4,076 | 2 | **0** | 4,074 |
| `…FloatPerspCalls` | 4,152 | 2 | **0** | 4,150 |
| `…GameCalls` | 0 | 4,074 | **4,076** | — |
| `…SaturateCount` | 0 | **0** | **0** | 0 |
| `…DegenerateCount` | 0 | **0** | **0** | 0 |
| `…RescaleCount` | 0 | **0** | **0** | 0 |

- **The shipped arm has zero float calls.** A build default has no pre-poke frame, so all
  8,152 look-ats and 8,228 perspectives run in Q20.12 from frame 1. The poked arms' residual
  `2`s (and route 0's mirror-image `4`s) are that one frame, and the two accountings differ
  by exactly the 4 calls involved.
- **`c219` route 1 reproduces `CAMERA_Q20_12.md` §5.1 exactly** — 8,148 / 8,224 / 2 / 2 —
  on a build linked eighteen commits later. The engagement is a property of the match, not
  of the binary.
- **`SaturateCount 0` and `DegenerateCount 0` over 8,152 look-ats** is the range proof: no
  product left Q20.12's ±524,288 and no vector was degenerate. `RescaleCount 0` confirms the
  >32,000 rescale pass never fires in this match.
- **Route 0 is a live negative control that could have failed**: on the shipped binary it
  restores the float chain and reads 4,074 / 4,150 float calls, so the fixed path is genuinely
  bypassable and the `.data` route still works after the default moved.

### 6.1 The Task 9 state hash question, answered rather than deferred

`CAMERA_Q20_12.md` §9.4 flagged that `gGMCameraStruct` is hashed by
`nds_task9_state_hash.c` and said "a default flip must measure it first".

**That instrument cannot answer this question, and the Makefile already says so.** From its
own note at `Makefile:1683`: the hash "asserts bit-exactness and this change is authorized
NON-bit-exact, so it can only ever report 'differs', which says nothing about whether
gameplay moved" — and `NDS_TASK9_STATE_HASH ?= 0`, with nothing in `verify-all.ps1`
referencing it. A precision change the owner has accepted will move it by construction.

The instruments that *can* fail are the ones this cycle ran: the eight routed symbols
classify **`draw+dispatch`, 100.0%** from the linked ELF, `gGMCameraMatrix`'s four readers
are all display or present callbacks (`CAMERA_Q20_12.md` §5.2), the animation and simulation
buckets do not move at the median (§3), and Boundary's realtime pacing smoke re-checks the
geometry counters end to end (§8).

---

## 7. Bytes, and the pixels

```text
.main   930,960 -> 930,928     -32 B      the four deleted poll sequences
.itcm    32,516 ->  32,516       0 B      unchanged; 220 B free on the instrument
```

**The flip itself costs zero bytes.** `gNdsR2CameraFixedEnabled` is a `.data` word and both
arms were already linked, so `NDS_R2_CAMERA_FIXED` only changes one initialised word — which
is what makes the whole measurement a same-binary route and what keeps
`NDS_R2_CAMERA_FIXED=0` available as a per-build override that restores the float chain byte
for byte.

**No new pixel capture was taken and none is owed.** The captures the decision was made on
are `artifacts/visibility/2026-08-16_camera-fixedpoint/`: `lock1694-route0-a.png` (float),
`lock1694-route1-a.png` (Q20.12) and `diff-lock1694-top.png`. The measured deltas —
6.5350% and 3.6325% of the top screen at two simulation-locked tics, against a same-build
adjacent-present floor of 35.2217% / 37.7983% — stand as the accepted ceiling.

**Cadence needs no re-run either.** `../2026-08-16_camera-cadence/CADENCE.md` §2 measured
both arms whole-match on the ROM the owner played, each as the boot arm, two independent
instruments: fixed **1,956/69/5/13** against float **1,953/72/5/13**, max 26 on both,
**0 cadence violations**, four VBlanks shorter over 2,043 presented frames. The shipped arm
is the slightly better one on presented cadence, so the quantisation failure mode did not
bite.

---

## 8. Verification

**Boundary green, 0 `Exception:`, nine checks** — `boundary-full.log`: GBI decode fixtures,
particle bank pack, harness registry, attack visual effects, Task 9 float ITCM, renderer ITCM
placement, Task 20 DTCM layout, `battle_playable_realtime`, published ROM contract.

**Its realtime pacing smoke is a NEGATIVE control here, and it is exact.** Against the last
four Boundary runs on this tree (`c206` / `c209b` / `c217` / `c219`):

| | `c219` | this cycle | Δ |
|---|---|---|---|
| `binds` / `vtx` / `tri` | `54 / 2484 / 828` | `54 / 2484 / 828` | **identical** |
| `ftrTri` | `132712/p067840/p164872/own424` | `132712/p067840/p164872/own424` | **identical** |
| `ticks` | 294,353,408 | **294,353,408** | **0** |
| proof-ROM `.itcm` | 30,164 / `renderer=12896` | 30,164 / `renderer=12896` | identical |

The camera chain does not run in the proof ROM's boot/Pupupu scene, so this is the control
that the change is **inert outside its own path**: two separately-linked binaries, the same
212 frames, the same geometry, and the same tick count *to the tick*. It is not a second
price for the battle.

> **`Tee-Object` CANNOT CAPTURE THIS AND THE FIRST RUN'S LOG PROVED IT.**
> `verify-all.ps1:167` writes each child checker's stdout with
> `[Console]::Out.Write($stdout)` — the console handle, not the pipeline — so
> `verify-all.ps1 … *>&1 | Tee-Object` produced a **three-line** `boundary.log` holding only
> the driver's own `Write-Output` calls. The verdict was still trustworthy (the pass line is
> gated on a per-verifier count that throws on a mismatch, `verify-all.ps1:319`), but every
> counter above was missing. Re-captured with `cmd`'s own redirect, and written into
> `docs/VERIFYING.md` so the next reader does not spend the run.
- Root ROMs byte-unchanged and not rebuilt:
  `smash64ds.nds` `54c07fac…`, `smash64ds-battle-playable-hwtri.nds` `6c939434…`
  (the bore-84 link, which must not be published), identical before and after.
- **Nothing published.** `build-c220-camship` is a lab build of the tick-HUD instrument.
- `build-c205-camtoggle` was not rebuilt. `decomp/` untouched.

---

## 9. What this cycle did NOT do, and what re-bases

- **The draw-side soft-float census re-bases and this cycle did not re-derive it.** With the
  chain fixed-point by default, the draw-side category loses the **11,504 tk/fr whole-match
  gross** `CAMERA_Q20_12.md` §4 measured (`syMatrixLookAtReflectF` + its `sqrtf`, two thirds
  of `syMatrixPerspFastF`, all of `syMatrixF2L`, `ndsCameraCatCamera`,
  `gmCameraLookAtFuncMatrix`), leaving ≈22,521. **Any figure quoted against the old split —
  shared 57,521 / sim-only 39,537 / sim+dispatch 37,662 / draw 34,178 — is now stale**, and
  the draw row is the one that moved; the camera is `draw+dispatch` 100.0%, so `sim-only`
  and `shared` are untouched. A precise re-derivation needs a new per-PC census, because the
  existing one is `build-c200-trackprof-off` at `GX_COMPOSE=1`. Stated so that the next
  reader does not repeat `MENU.md`'s 94,602, which was wrong for fourteen cycles for exactly
  this reason.
- **The poll removal was not priced on a same-binary route** (§4). It would need its own
  route word; it is sized, corroborated and shipped as strictly-less-work.
- **The particle camera is still float.** `ndsParticleSetCurrentCamera`'s `syMatrixLookAtF`,
  `syMatrixOrthoF` and the last live `guMtxCatF` are ~5,100 tk/fr gross and are the cheapest
  remaining piece of this lane. Its concat is the exact shape the 5.14× prior was measured
  on — but see `../2026-08-16_itcm-census/ITCM_CENSUS.md` §4 before sizing it at that rate.
- **`gmCameraCheckTargetInBounds`** (92.2 tk/fr) was left in float.
- **No pixel capture, no cadence run, no toggle-ROM rebuild** (§7 says why each is owed
  nothing).

---

## 10. Reproduction

```powershell
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c220-camship `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1
foreach ($b in 'build-c219-animitcm-ship','build-c220-camship') {
  foreach ($arm in 0,1) {
    pwsh -File scripts\sample-tick-hud-buckets.ps1 -Build $b -NoBuild -RunnerSlot 6 `
        -RingDump -Samples 1600 -StartFrame 438 -TimeoutSeconds 3600 `
        -SetGlobals "gNdsR2CameraFixedEnabled=$arm" -ExtraGlobals gNdsR2CameraFixedLookAtCalls,... `
        -RowsCsv ...-rows.csv -JsonOut ....json
  }
}
```

`analyze.py <a-rows.csv> <b-rows.csv> [series]` produces every table above;
`--fixwrap-a` applies the sampler's 2^22 correction offline to a pre-2026-08-16 CSV, which
is what §2 needs to compare the inherited basis with a corrected run.
