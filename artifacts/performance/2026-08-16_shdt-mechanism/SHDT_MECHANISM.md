# `SHDT` is diagnosed and it does not close the gate. Hit detection running for FREE clears +64,977 by 2,543 ticks; reduced to its own median it does not clear it at all.

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `dd80585d6eb`**
**0 lab builds, 0 emulator runs, 1 harness change (built and verified), no gameplay
value changed, no default flipped, no ROM published, both root ROMs byte-unchanged.**
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.** Every table
states its window.

```text
REQUIREMENT +64,977 net ticks per presented frame at rank-80.  Basis
            build-c219-animitcm-ship, ../2026-08-16_match-io-audit/io-rows.csv,
            gate arm NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1
            NDS_R2_BATTLEPACK_KEEP_CACHE=1, mode 163 one-minute match, 1,600
            samples, frames 439-2038, DLDI ON, slips=0, 2^22-corrected series:
            rank-80 1,210,304 raw / 1,185,357 net, band 41-120 mean 1,217,946.

MECHANISM   AN SHDT EXCURSION IS A PER-FRAME EVENT, NOT A PER-PAIR COST, AND THE
            SOURCE'S LATCH IS ALREADY OPTIMAL.  57 of 1,600 frames run at least
            one (attack collision x hurtbox) test.  On such a frame the victim's
            ~14 hurtbox joints get a world matrix, a 3x3 cofactor inverse and
            axis scales built ONCE EACH, then one rectangle test per pair.
            Proven by a natural experiment: frames carrying 44 pairs run EXACTLY
            2.00x the pair tests of frames carrying 22, and every per-joint
            symbol stays flat (0.93-1.04x) while the frame gets 10,004 ticks
            CHEAPER.  OLS over the 57 engaged frames: -440 tk per pair, R2=0.003.
            Sections 1-2.

OWNER       THE FIGHTER COLLISION TRANSFORM CHAIN, +102,988 tk/fr on the engaged
            frames = 38.4% of their +268,255 work premium, of which 67,712 is
            soft float charged back to six named callers.  The single largest
            item is func_ovl2_800ED490, the 3x4 affine multiply: 22.77 calls per
            engaged frame at EXACTLY 63.0 soft-float library calls each --
            gmcollision.c's own 36 fmul + 27 fadd -- costing 32.7 ARM9 cycles per
            helper call.  93.5% of the premium is genuine marginal work, not
            cache displacement (4.5%).  Section 3.

CEILING     THE LANE CANNOT CLOSE THE GAP.  Exact re-rank of the basis's own
            1,600 rows:
              delete func_ovl2_800ED490 everywhere            -6,704  -> +58,273
              delete the whole chain's excursion             -23,674  -> +41,303
              delete the whole chain everywhere              -44,088  -> +20,889
              delete SHDT down to its own P50 everywhere     -63,040  -> +1,937
              delete the WHOLE SHDT bucket, every frame      -67,520  -> -2,543
            Hit detection at literally zero cost clears the requirement by 2,543.
            Reducing it to its own median leaves +1,937 open.  Section 4.

ITEM A      THE 2^22 SAMPLER FILTER IS BUILT AND VERIFIED.  Detects and corrects
            in scripts/sample-tick-hud-buckets.ps1; all five known-bad rows
            reproduce IO_AUDIT.md section 2's corrected values EXACTLY, frame
            1357 corrects 6 buckets (SCPU plus its parents, as that section's
            Proof 3 predicts), and the largest clean ALL in the run is 3,358,080
            -- a 25% margin below the threshold.  Section 5.

RETRACTED   Four items, three of them from this cycle or its brief.  Section 6.
```

---

## 1. What an `SHDT` excursion frame is

**Basis for every profile figure in sections 1-3:** `build-c191-sitr-profile-c185`
(`NDS_R2_COLLISION_FIXED 0`, `NDS_R2_FIGHTER_GX_COMPOSE 0`, `NDS_R2_BOTH_CPU 1`,
`NDS_R2_BATTLEPACK 1`, `KEEP_CACHE 1`, `NDS_TICK_HUD_DRAW 0` — read back from the
build's own `nds_build_config.h`), `regions=1601`,
`ticks = cycles / (2 x 1600)`. **No build and no emulator run was spent** — the
profile was already on disk and the tool reads it in one pass.

`SHDT` brackets `battleship_ftMainProcSearchHitAll`
(`src/port/reloc_backend_diagnostic_recorders.c:5678-5696`). **57 of 1,600
regions execute `gmCollisionCheckFighterAttackDamageCollide` at least once**;
the other 1,543 never reach a geometry test at all. Against that split:

| | engaged (57) | not (1,543) |
|---|---:|---:|
| non-idle **work** premium | **+268,255 tk/fr** | — |
| `armWaitForIrq` (idle, excluded throughout) | +128,344 | — |

Call counts here are **exact, not sampled**: a function's entry PC executes once
per call, so the profiler's instruction count at that PC *is* that region's call
count (`[[entry-pc-gives-exact-call-counts]]`).

### The chain, from `decomp/BattleShip-main/decomp/src/gm/gmcollision.c`

Per engaged frame, for each of the victim's hurtbox joints:

1. `func_ovl2_800EDBA4` (`:332`) walks up to the nearest ancestor whose world
   matrix is still valid, then `func_ovl2_800ED490` multiplies back **down** —
   36 `fmul` + 27 `fadd` per level.
2. `gmCollisionSetInvertMatrix` (`:228`) takes a 3x3 cofactor inverse plus a
   reciprocal, 61 float ops.
3. `func_ovl2_800EDE5C` (`:472`) takes three `sqrtf` for the joint's axis scales.
4. `gmCollisionTestRectangle` (`:661`) then transforms the attacker's `pos_curr`
   and `pos_prev` into that joint's local frame and clips.

Steps 1–3 are latched behind `FTParts`' four dirty flags, cleared once per
fighter per frame (`ftmain.c:1847`). **Section 2 measures whether that latch
actually holds on the current tree, rather than reading it from the source.**

---

## 2. The falsifier: the cost is per FRAME, not per pair

`[[a-residual-divided-by-a-count-is-not-a-price]]`. Six retractions in this
campaign came from dividing a residual by a count; the audit's own force-load
section is the most recent, where "~228,600 per cache hit" collapsed the moment
the frames were split by load count. So before any per-pair figure was written
down, the engaged frames were split by their exact pair-test count.

**It collapses harder than the force-load case did** (`pair-scaling.txt`):

| pairs on the frame | n | non-idle tk/fr | lift over the zero-pair median | **implied per pair** |
|---|---:|---:|---:|---:|
| 1–4 | 9 | 1,357,815 | +400,848 | **327,966** |
| 5–9 | 2 | 1,420,848 | +463,881 | 66,269 |
| 10–19 | 4 | 1,333,806 | +376,838 | 30,147 |
| 20–29 | 19 | 1,149,156 | +192,189 | 9,039 |
| 30–49 | 19 | 1,171,083 | +214,115 | **5,229** |

**A 63x collapse.** OLS over all 57 engaged frames gives
`non-idle tk/fr = −440 x pairs + 1,244,904` at **R² = 0.003** — the slope is
*negative* and explains nothing. The intercept sits **+287,937 above** the
zero-pair baseline of 956,968. **Engagement costs a fixed amount; the number of
pairs is not a price at any value.**

### 2.1 The natural experiment that says why

Pair counts are quantised. `FTDAMAGECOLL_NUM_MAX` is **11**
(`include/ft/fighter.h:322`), and the loop runs 2.000 simulation ticks per
presented frame, so **one live attack collision contributes exactly 22 pairs**.
The two largest populations are therefore 22 pairs (n=12) and 44 pairs (n=10) —
**the same fight with exactly twice the pair tests.**

| symbol | 22-pair | 44-pair | **x** | what it is |
|---|---:|---:|---:|---|
| `gmCollisionCheckFighterAttackDamageCollide` | 22.00 | 44.00 | **2.00** | the pair loop |
| `gmCollisionTestRectangle` | 22.00 | 44.00 | **2.00** | per-pair overlap test |
| `func_ovl2_800EDE5C` | 22.00 | 44.00 | **2.00** | per-pair entry (latch check) |
| `gmCollisionGetWorldPosition` | 32.17 | 52.80 | 1.64 | |
| `gmCollisionSetInvertMatrix` | 15.67 | 15.40 | **0.98** | **per joint — FLAT** |
| `func_ovl2_800EDBA4` | 15.67 | 15.40 | **0.98** | **per joint — FLAT** |
| `func_ovl2_800ED490` | 24.50 | 23.80 | **0.97** | **per joint — FLAT** |
| `gmCollisionTransformMatrixAll` | 27.75 | 25.80 | **0.93** | **FLAT** |
| `__aeabi_fadd` | 3,712.25 | 3,871.60 | **1.04** | **FLAT** |
| `__aeabi_fmul` | 4,563.08 | 4,459.70 | **0.98** | **FLAT** |
| `sqrtf` | 90.08 | 88.20 | **0.98** | **FLAT** |
| **non-idle total** | **1,185,286** | **1,175,283** | | **−10,004** |

**Doubling the pair tests leaves the soft-float call count and every per-joint
symbol unchanged, and the frame gets 10,004 ticks cheaper.** Three consequences,
and all three close a route:

- **The source's latch is already optimal.** Each joint's matrix, inverse and
  axis scales are computed exactly once per frame no matter how many hitboxes
  test them. `BAND_OWNER.md` §3 asserted this from the dirty flags; it is now
  measured on the current tree. **There is nothing to memoise.**
- **The pair test itself is nearly free.** The second live attack collision adds
  22 `TestRectangle` + 22 `E5C` + 20.6 `GetWorldPosition` — 3,698 tk of self
  cycles — and no soft float above noise.
- **A per-hurtbox broad phase cannot pay.** It would remove pair tests, which
  are the flat 2.00x half. `BAND_OWNER.md` §4 already refuted it from the other
  direction (slice 47's reach bound: `ReachTests 2,373 WouldSkip 0`); this is a
  second, independent refutation.

**So the ~14 joints are the cost.** `gmCollisionSetInvertMatrix` runs **796 times
over the match on 57 frames = 13.96 joints per engaged frame.**

---

## 3. Composition, leaf-attributed on the exact engaged set

The first pass of this analysis used the top 88 regions by
`gmCollisionCheckFighterAttackDamageCollide` cycles; **31 of those 88 carry zero
engagement**, so that mask diluted every premium in it. Everything below is the
exact 57-region set (`c191-engaged57-leafattr.txt`). Negative and positive
controls for the mask itself, taken on the 88-frame version
(`c191-mask-damagecollide.txt`):

| mask | non-idle premium | largest row | collision symbols present? |
|---|---:|---|---|
| the collision mask | +179,942 | `__aeabi_fadd` +29,451 | yes, 7 in the top 22 |
| **random 88 (negative)** | **−19,189** | `armWaitForIrq` +10,083 | **none** |
| profile's own top-88 by cost | +362,157 | `memcpy`, `get_fat`, `armCopyMem32` | **none in the top 8** |

The cost-ranked mask overlaps the collision mask at **37 of 88** against a chance
baseline of 4.8, and reproduces the asset-load ranking instead — so the mask is
measuring the mechanism, not frame cost.

### 3.1 Soft float charged back to its callers

`--attribute-leaves` over 8,746 `bl` sites, engaged-57 mask, **+84,619 tk/fr
charged to callers in total**:

| caller | **+tk/fr (leaf)** | leaf calls, engaged | control | self cycles |
|---|---:|---:|---:|---:|
| **`func_ovl2_800ED490`** — 3x4 affine multiply | **20,696** | 1,434.7 | 47.9 | 7,006 |
| `gmCollisionGetWorldPosition` | 11,117 | 729.8 | 3.6 | 3,620 |
| `func_ovl2_800EDE5C` — axis scales | 9,686 | 251.4 | 1.6 | 1,509 |
| `gmCollisionSetInvertMatrix` — 3x3 inverse | 9,329 | 851.9 | 5.6 | 5,301 |
| `gmCollisionTestRectangle` | 7,391 | 444.5 | 2.2 | 4,489 |
| `gmCollisionTransformMatrixAll` | 5,791 | 566.6 | 39.3 | 5,060 |
| `gmCollisionGetFighterPartsWorldPosition` | 3,494 | 272.2 | 44.0 | — |
| `lbCommonCos` / `lbCommonSin` | 3,805 / 2,477 | 473.2 / 378.6 | 104.4 / 83.5 | — |

**named chain, 10 symbols: +102,988 tk/fr = self 35,277 + leaf 67,712 = 38.4% of
the engaged frame's +268,255 work premium.**

### 3.1.1 The attribution is exact, checked four ways against the source

Dividing each caller's leaf calls by its own invocation count gives the number of
soft-float library calls one invocation makes. **Every one lands on the source's
own operation count to the second decimal** — this was not tuned, it is what the
tool printed:

| caller | leaf calls / invocation | the source's own count | error |
|---|---:|---:|---:|
| `func_ovl2_800ED490` | 1,434.7 / 22.77 = **63.01** | 36 `fmul` + 27 `fadd` = **63** | **+0.0%** |
| `gmCollisionSetInvertMatrix` | 851.9 / 13.96 = **61.02** | 3x3 cofactor inverse + reciprocal = **61** | **+0.0%** |
| `gmCollisionGetWorldPosition` | 729.8 / 40.54 = **18.00** | `:196-205`, 9 `f32` multiplies + 9 adds, no branch = **18** | **+0.0%** |
| `func_ovl2_800EDE5C` | 251.4 / 13.96 = **18.01** | 3 axis scales x (3 `fmul` + 2 `fadd` + 1 `sqrtf`) = **18** | **+0.0%** |

The last two rows also **re-prove the latch independently**. `func_ovl2_800EDE5C`
is entered **27.23** times per engaged frame — once per *pair* — but its leaf work
divides exactly by **13.96**, the *joint* count: it early-exits on
`unk_dobjtrans_0x6` for the other 13.27 entries. Divided by its own call count it
reads a meaningless 9.23. **The work is per joint even where the call is per
pair.**

**And the price of that work:** the extra `__aeabi_fadd` + `__aeabi_fmul` on an
engaged frame is **4,877 calls costing 79,740 tk/fr = 32.7 ARM9 cycles per helper
call.**

### 3.2 It is real work, not cache displacement — measured, not assumed

Several large rows have a *flat* call count (`memcpy` 123.75 vs 118.73,
`lbParticleDrawTextures` 4.00 vs 4.00, `ndsRendererAdapterBuildDObjXObjMatrix`
57.39 vs 55.09) while their cycles rise, which is the
`[[the-compare-was-never-the-cost]]` shape — the same calls costing more because
the frame's working set was displaced. Every symbol was therefore classified by
its own exact call ratio, cycles summed inside each class afterwards so no symbol
could be assigned to the class that flatters it (`collateral.txt`):

| class | symbols | tk/fr | share |
|---|---:|---:|---:|
| **MARGINAL** — calls/frame up ≥ 10% | 415 | **250,755** | **93.5%** |
| COLLATERAL — calls/frame flat within ±10% | 638 | 12,047 | 4.5% |
| NEW — absent off the engaged set | 99 | 5,492 | 2.0% |

**The collateral hypothesis is refuted at 4.5%.** The premium is work that
genuinely happened. The largest collateral row is `memcpy` at +5,864.

### 3.3 What else an engagement drags in

Disjoint families over the same +268,255 (self cycles):

| family | tk/fr | share |
|---|---:|---:|
| fighter collision chain (self only) | 35,277 | 13.2% |
| soft float, all helpers (overlaps the chain's 67,712 leaf) | 94,649 | 35.3% |
| asset I/O — FAT/DLDI/card | 23,848 | 8.9% |
| animation rebuild (the status change the hit causes) | 22,786 | 8.5% |
| trig table `lbCommonSin`/`Cos` | 4,593 | 1.7% |
| hit sound effect `ndsAudioFgmPlayAtPan` (1.04 vs 0.06 calls, 16.0x) | 3,249 | 1.2% |

This confirms `BAND_OWNER.md` §5's co-fire and extends it: **one engagement drives
five costs in four brackets** — the narrow phase in `SHDT`, the status change's
animation load in `SPRM`, the sound-effect pack read and the FAT walk in `SITR`.
The asset-I/O family here is the same lane the audit already priced at **12,736**
and must not be counted twice.

> **The `sqrtf` row is partly stale and is not claimed.** This profile is
> `build-c191`, which predates `build-c215-hwmath-ship`. `../2026-08-16_hwmath-route/HWROUTE.md`
> banked `sqrtf` into ARM state with its leading `SQRTCNT` poll deleted
> (−12,416 at rank-80), so the +5,257 engaged premium on `sqrtf` is already
> partly collected.

---

## 4. Sizing — the exact re-rank, and why the lane cannot close the gap

Every row is an **exact re-sort of the basis's own 1,600 corrected rows**, not an
interpolation (`shdt-size.txt`). Band 41–120 is quoted beside rank-80 because
rank-80 is one order statistic on a steep slope.

| intervention | rank-80 | moved | band 41–120 | **gap** |
|---|---:|---:|---:|---:|
| *(control)* | 1,210,304 | — | 1,217,946 | **+64,977** |
| delete `func_ovl2_800ED490` (self+leaf) everywhere | 1,203,600 | 6,704 | 1,212,715 | +58,273 |
| delete `gmCollisionSetInvertMatrix` everywhere | 1,206,728 | 3,576 | 1,215,210 | +61,401 |
| delete the whole chain's **excursion** (25.2% of `SHDT`) | 1,186,630 | 23,674 | 1,197,387 | +41,303 |
| delete the whole chain **everywhere** (54% of `SHDT`)† | 1,166,216 | 44,088 | 1,172,913 | +20,889 |
| delete `SHDT` down to its own P50, every frame | 1,147,264 | 63,040 | 1,149,093 | **+1,937** |
| **delete the WHOLE `SHDT` bucket, every frame** | **1,142,784** | **67,520** | **1,144,552** | **−2,543** |

> † **The one estimated row, and its assumption is named.** The chain's *self*
> cycles whole match are **2,706 tk/fr** measured; 54% comes from scaling that by
> the leaf/self ratio of **1.92** measured on the engaged set, i.e. it assumes the
> same ratio holds on the 1,543 frames where the chain barely runs. Two
> independent corroborations that 54% is about right: 48.4% of all chain self
> cycles fall on the 57 engaged frames, and `BAND_OWNER.md` §3's itemisation of
> `SHDT`'s *always-run* parts (bracket apparatus + both `ProcSearchHitAll` selves
> + the `k` flag scan + `SearchHitItem`/`SearchHitWeapon`/`SearchGroundHit` +
> `GetBestHitStatusAll` + `GetGroundHitObstacle`) sums to **4,693 tk/fr** — and
> 7,899 + 4,693 = **12,592** against the bucket's measured **14,544**, 13.4% apart
> on two different builds. Every other row in the table is exact.

**Read the last two rows.** Hit detection running at **literally zero cost**
clears the requirement by 2,543 ticks — 3.8% of it. Reducing it to its own median
— i.e. deleting 100% of the excursion, every engaged frame made ordinary — leaves
**+1,937 still open**. There is no fraction of this lane that closes the gap,
because the lane at zero barely does.

The lane, whole match: **P50 4,608 · mean 14,544 · P95 82,175 · max 453,056 ·
total 23,270,656 tk**, with 93 frames over 30,000 carrying 69.7% of it.

### 4.1 Cluster-only cuts saturate almost immediately

| f of the cluster's `SHDT` excess deleted | 0.10 | 0.20 | 0.25 | 0.50 | 1.00 |
|---|---:|---:|---:|---:|---:|
| rank-80 moved | 3,456 | 3,456 | 3,456 | 29,280 | 50,752 |

Flat to 0.25 because once the 33 cluster frames drop below the surrounding
population, rank-80 is set by frames the cut never touched. This reproduces
`BAND_OWNER.md` §4's saturation finding on a new basis two weeks later.

### 4.2 The one sub-lane whose refutation does NOT transfer

`../2026-08-15_cfx-narrow-exchange/EXCHANGE.md` measured the fighter narrow phase
converted to fixed point at an exchange rate of **2.68** — it costs more — and
showed the lane's ceiling was 0.47x even at zero. **That result stands and is not
reopened.** But its stated mechanism was `__udivmoddi4`: the fixed formulation
calls libgcc's bit-by-bit 64-bit divide **4.0 times per narrow-phase entry**,
worth +17,377 at rank-80, "more than the entire float bill the conversion
deletes" (§4).

**`func_ovl2_800ED490` is a 3x4 affine multiply. It contains no division.** So the
divide half of that refutation does not apply to the largest item in the chain,
and the sub-lane is *arithmetically* open. It is not open for any other reason:

- **It is a gameplay-fidelity change, not engineering.** The joint world matrices
  feed `gmCollisionSetInvertMatrix`, whose inverse decides whether a hitbox
  overlaps a hurtbox. A fixed-point matrix produces different hits, damage and
  knockback. **`BLOCKED(decision: owner)` under `PROJECT_GOAL.md` rung 3.**
- **And it is priced at 6,704 = 0.103x of the requirement even if the conversion
  were free.** `func_ovl2_800ED490` costs **1,216 tk/call** (27,702 tk/fr over
  22.77 calls, self+leaf) against **987 tk/fr whole match**.
- **The byte budget forbids it anyway, and this is the decisive half.**
  `HWROUTE.md` §7 measures the marginal price of a byte in this chain at
  **3.61 tk/fr** — `gmCollisionTransformMatrixAll` is one of its four members, and
  the chain already spends 44.6% of its own cost on instruction fetch. The
  collision ring's 3,228 B cost **11,650 tk/fr of fetch alone**, which is
  **11.8x** `func_ovl2_800ED490`'s entire whole-match cost of 987 tk/fr. A
  fixed-point rewrite has to be byte-NEGATIVE to break even, and neither prior
  attempt in this tree was (ring +3,228 B, camera inlining +3,032 B).

> **One comparison deliberately NOT made.** The renderer's
> `ndsRendererMtxMulAffine20p12` runs 54.26 calls/frame at 347 tk/call self, which
> invites a "fixed point is 3.5x cheaper per call" line. **It is not quoted as
> evidence**: nothing here establishes that the two functions perform equivalent
> work (`codegraph_explore` did not return its body, and a matching name is not a
> matching contract). It would need the two bodies compared operation for
> operation first.

**So it is named and sized, and it is not recommended.** Nothing here asks the
owner for a decision, because the decision would buy 0.103x at best.

---

## 5. Item A — the `2^22` sampler filter, built and verified

`scripts/sample-tick-hud-buckets.ps1`, one block after the sample-integrity
checks and before the CSV write, so it covers the ring path and the per-frame
path alike. **Correct rather than reject**: rejecting a row would leave 1,599
samples and move the rank-80 index, while the correction restores the frame's
real value.

- **Detector** `ALL >= (1 << 22)`. Complete by construction — `ALL` contains
  every span, and the artifact can only inflate.
- **Action** subtract 4,194,304 from `ALL` and from every bucket on that row at
  or above it. Each affected span's parents are corrected with it.
- **Audit trail** a new trailing CSV column **`WRAPFIX`** holds how many buckets
  each row had corrected. It goes **last**, after the per-frame globals, so no
  existing column moves; every consumer in `scripts/` reads by name
  (`csv.DictReader` / `Import-Csv`), checked before the column was added.
- **Warning** names each frame, its raw and corrected `ALL`, and the run's
  post-correction `ALL` median, so a correction that lands far from the median —
  which would mean the row was a real stall and the correction wrong — is visible
  in the run log rather than silent.

**Verified against the five known-bad rows of the basis** (`wrapfix-test.txt`):

```text
frame 1056: ALL 5,311,744 -> 1,117,440, 3 bucket(s) corrected
frame 1357: ALL 5,312,192 -> 1,117,888, 6 bucket(s) corrected
frame 1629: ALL 5,872,576 -> 1,678,272, 3 bucket(s) corrected
frame 1750: ALL 5,312,128 -> 1,117,824, 5 bucket(s) corrected
frame 1961: ALL 5,312,064 -> 1,117,760, 3 bucket(s) corrected
ALL FIVE corrected values match IO_AUDIT.md section 2 exactly
rows still >= 2^22 after correction: 0      max ALL after correction: 3,358,080
```

Three things that make this a verification and not a smoke test:

1. **All five corrected values equal the audit's independently derived table.**
2. **Frame 1357 corrects 6 buckets and the others 3.** The audit's Proof 3 says
   1357 carries the artifact in `SCPU` "and therefore in its parents
   `SINT`→`GCRA`→`SRC`" — `SCPU`+`SINT`+`GCRA`+`SRC`+`WORK`+`ALL` = **6**. The
   bucket *count* was not used to build the filter and it lands on the number the
   bucket tree predicts.
3. **Negative control: the threshold has a 25% margin.** The largest clean `ALL`
   in the whole run is 3,358,080 — exactly 6 VBlanks — against a 4,194,304
   threshold. No real frame on this instrument is near it.

**Still not proven: the trigger.** The magnitude, the site (`cpuGetTiming`
`0x020be710` / `tickGetCount` `0x020bfc2c`) and the arithmetic that produces
exactly 2^22 are; *why* the tick IRQ is deferred long enough for
`((timer ^ 0x8000) >> 15) & (IF >> 5)` to miss is not. The filter does not depend
on it — it keys on the proven signature.

---

## 6. Retractions and corrections to the record

**Four, and three are from this cycle or the brief that set it.**
`[[retract-with-the-same-energy]]`.

1. **"The `SHDT` cluster is worth −57,152, 88% of the gap" is a mis-read of the
   audit's own table.** `overgate.py:cut = E[i].sum()` deletes **every leaf's**
   excess on those 33 frames, not `SHDT`'s — the audit's row is correctly headed
   *"delete all excess on…"*. Deleting only `SHDT`'s excess on the same 33 frames
   is **−50,752 (gap +14,225)**. The 57,152 is the value of making those frames
   entirely median in every bucket.
2. **"Run P50 4,608 and mean 14,544 — a 56x concentration" mixes two
   quantities.** mean/P50 is **3.2x**. The 56x is the *cluster's excess* divided
   by the run P50 (259,776 / 4,608 = 56.4). Both are true; only the second is
   about concentration, and it is a property of the cluster, not of the lane.
3. **My own, caught before it reached a conclusion.** I subtracted
   `armWaitForIrq` from `analyze-profile-region-split.py`'s "non-idle premium"
   to get a work premium of 99,705. That figure already excludes idle, so the
   top-88 work premium is **179,942** and every component share I derived from
   99,705 was ~1.8x too large for one tool call. The engaged-57 numbers in §3 are
   computed against the correct 268,255.
4. **My own collateral hypothesis, measured and killed.** §3.2's flat-call-count
   rows suggested the engaged frame's premium might be the rest of the frame
   getting slower. Classified by exact call ratio it is **4.5%**. The lane is
   arithmetic, not displacement.

---

## 7. What this cycle did NOT do

- **No lab build, no emulator run of my own, no ROM published.** Every figure is
  exact arithmetic over two artifacts already on disk. **Boundary
  `verify-all.ps1 -Profile Boundary` PASSED, 0 `Exception:` over the full 18.9 MB
  log** (`boundary.trimmed.log` keeps the verdict lines), including
  `Published ROM contract passed`, `check-decomp-pristine`, `Task 9 float ITCM`
  (`itcm=30164/32768`), `NDS_PARTICLE_BANKS=PASS`, harness registry 0 drift, and
  the `battle_playable` realtime pacing smoke (`frames=212 … rprof=0`,
  `gxstat=0x6000000`). **Both root ROMs SHA-256 identical before and after that
  run**, which rebuilds them: `smash64ds.nds`
  `54C07FAC…6AC68A` (md5 `af968925…`),
  `smash64ds-battle-playable-hwtri.nds` `6C939434…67C99`.
- **The `SHDT` lane was not cut and nothing was banked.** The cycle's product is
  a closed diagnosis and a ceiling, not a deletion.
- **The `2^22` filter was not re-run through the emulator.** It is verified
  against the recorded raw rows of the basis, which is the same data the sampler
  would hand it; the next whole-match run exercises it live and should be checked
  for the warning block. **No prior artifact CSV was rewritten** — the correction
  applies to new runs, and the existing analyses that need it already apply it in
  Python (`overgate.py`, `shdt_size.py`).
- **The trigger of the `2^22` artifact was not established** (§5).
- **Item C, the force-load per-PC account, was not reached.** Cycle 106's 25.1%
  remains the only instruction-level account of that path.
- **The `SITR` cluster (27 frames, −51,200 at rank-80) was not touched**, and it
  is now the largest un-diagnosed item on the board.
- **No default flipped, no fidelity decision taken or asked for.** §4.2 names a
  route and declines it on its own measured size rather than escalating it.
- `build-c205-camtoggle` was not rebuilt.

---

## 8. Reproduction

No emulator. Both inputs are already committed.

```powershell
# 1. the mask, its two controls, and the leaf attribution
C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe -d `
  builds\build-c191-sitr-profile-c185\smash64ds-battle-playable-tickhud-hwtri.elf > c191.dis

python scripts\analyze-profile-region-split.py `
  artifacts\performance\2026-08-15_sitr-direct-children\v3-c191\arm9-profile.csv `
  --census artifacts\performance\2026-08-15_sitr-direct-children\v3-c191\census.json `
  --top-by-symbols gmCollisionCheckFighterAttackDamageCollide --top 88 `
  --check-controls --cache c191-split.npz          # -> c191-mask-damagecollide.txt
# then --regions-file <the 57 engaged region ids> --dis c191.dis --attribute-leaves ...
#                                                  # -> c191-engaged57-leafattr.txt

# 2. the falsifier, the engaged split and the collateral classification
python artifacts\performance\2026-08-16_shdt-mechanism\pair_scaling.py   c191-split.npz
python artifacts\performance\2026-08-16_shdt-mechanism\engaged_split.py  c191-split.npz
python artifacts\performance\2026-08-16_shdt-mechanism\collateral.py     c191-split.npz

# 3. the exact re-rank against the gate series
python artifacts\performance\2026-08-16_shdt-mechanism\shdt_size.py
```

Run these from a normal PowerShell session, not from a Git-Bash-spawned shell —
`verify-all.ps1`'s toolchain pre-flight throws `NDS_RECURSIVE_MAKE=FAIL:127`
under Git Bash's environment and passes under PowerShell.
