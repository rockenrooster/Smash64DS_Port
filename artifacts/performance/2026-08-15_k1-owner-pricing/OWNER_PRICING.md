# The three owners, priced per-PC on the shipping candidate — and the largest one is a layer nobody has priced on the right population

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `48741fcaf05`**
**One build (`build-c172-profile-shipcand`), one v3 stall capture, one gate run.**
Predecessor: `…/2026-08-15_k0-rerank/K0_RERANK.md`.

---

## 0. Outcome first

```text
SITR    IS FLAT IN ITS OWN BODY.  battleship_ftMainProcUpdateInterrupt is
        8,119 tk/fr self on the 80 frames that set P95 and 1.06x its whole-match
        rate.  Its whole exclusive static closure is 20,278 tk/fr over 29
        functions.  SITR's +152,483 bracket excess is therefore NOT in the proc
        -- it is in leaves the proc dispatches to, and the proc is the wrong
        thing to write a mechanism against.  Same for the scheduler: gcRunAll
        1.03x, gcRunGObjProcess 1.09x.

SHDT    IS A LAYER, NOT A LANE, AND THE LAYER IS FIGHTER-PART WORLD MATRICES
        IN f32.  SHDT's exclusive closure is 6,279 tk/fr; the spike is in the
        SHARED geometry it calls.  gmCollisionCheckFighterAttackDamageCollide
        10.81x - gmCollisionTestRectangle 11.70x - gmCollisionSetInvertMatrix
        11.54x - gmCollisionGetWorldPosition 11.46x - func_ovl2_800EDE5C 11.27x
        - func_ovl2_800ED490 7.68x - func_ovl2_800EDBA4 6.90x -
        gmCollisionTransformMatrixAll 5.19x.  HANDOFF's "SHDT CLOSED, bar
        47,424" is a MICRO-OPTIMIZATION verdict on a WHOLE-MATCH bar; plan.md
        §3's caveat applies verbatim.

TRIO    THE SOFT-FLOAT TRIO IS 104,667 tk/fr ON THE P95 SET (fadd 51,063 +
        fmul 39,520 + fdiv 14,084) AND ITS CALLERS DO CONCENTRATE.  The largest
        single caller is func_ovl2_800ED490 at 11,808 tk/fr -- 2.2x the "largest
        single caller 5,402" that closed this lane in plan.md §9 -- and the
        collision/stage-MP subsystem is the LARGEST caller group at 2.41% of
        non-idle.  §9's "soft float does not promote" was measured WHOLE MATCH
        on a different binary.  RETRACTED on this one.

PACKAGE ONE change, and it is already written, compiled and host-graded:
        WIRE src/port/nds_r2_collision_fixed.c.  Its own header says "NOTHING
        CALLS THESE YET, by design".  The cluster it replaces is 57,375 tk/fr on
        the 80 frames that set P95, of which 39,248 is soft-float and sqrtf CALL
        cost that a fixed kernel deletes outright rather than replaces.  Against
        +32,593 that is 1.20x on the deleted component alone and 1.76x on the
        cluster.

        SLICE 52 AS SCOPED IS STILL SHORT: its deletable ring is 20,329 tk/fr
        (0.62x).  The package must extend the seam by ONE function --
        func_ovl2_800EDBA4 -- keeping parts->mtx_translate f32 at the boundary
        so its fifteen referrers are untouched.  NOT IMPLEMENTED THIS CYCLE.

COUNTER LANDED: gNdsCfxFighterDamagePhaseCalls/Hits and …ShieldPhaseCalls/Hits
        on the two ring entry points that have ZERO in-TU callers (verified from
        the linked ELF).  plan.md §9 law 1 satisfied before the code change.
```

---

## 1. The capture, and why the old one could not answer this

`build-c172-profile-shipcand` = the shipping candidate's flags
(`NDS_R2_BOTH_CPU=1`, `NDS_R2_BATTLEPACK=1`, `NDS_R2_BATTLEPACK_KEEP_CACHE=1`)
plus `NDS_TICK_HUD_DRAW=0` and `NDS_TASK37_PROFILE=1`. Built at HEAD
`48741fcaf05` (`NDS_TASK10_GIT_SHORT "48741fc"`), verified from the build's own
`nds_build_config.h` rather than from the invocation.

```text
format=melonDS-arm9-retail-profile-v3   regions=1601   window frames 438..2038
instructions 1,009,157,563   cycles 3,765,596,866
stall_partition_residual 0   timestamp_discontinuities 0
icache_fill 1,123,959,694  dcache_fill 876,815,602  issue 657,182,993
halt_wait 662,465,240      write_buffer 162,541,819 interlock 137,779,519
```

Mask: `total_cycles − halt_wait`, never `total_cycles` (a region is a presented
frame and `total_cycles` is VBlank-quantised — sorting on it sorts rounding
noise). The 80 marginal frames are those at or above **1,177,548 ticks**.
Basis for every figure below: `ticks/frame = cycles / (2 × 80) = cycles / 160`.

**Falsifier band — the two profile arms agree, so the arm is not the finding.**

| | `c159` (2026-08-14, pre-pack, pre-fix, pre-seam) | **`c172` (this capture)** | Δ |
|---|---:|---:|---:|
| median frame | 954,434 | 956,395 | +0.21% |
| rank-80 (the mask) | 1,174,997 | 1,177,548 | +0.22% |
| rank-16 (top 1%) | — | 1,428,497 | |
| max | — | 3,105,940 | |

The two captures' work distributions are within 0.22%, so anything that moves
between them is a real change in *where* the work is, not in *how much*.

**Instrument check.** `-RunnerSlot` silently overrides `-MelonDS` and that trap
returned a v2 census with no stall columns once already; this run named the
attributor and no slot, and the capture carries all eight stall columns with a
zero partition residual, which a v2 build cannot produce.

---

## 2. `SITR` — what it actually spends its time on

`SITR = SINT − SCPU`, i.e. `battleship_ftMainProcUpdateInterrupt` less the
level-3 AI. Read the body (`decomp/…/ft/ftmain.c:1214-1572`) and it is: input
edge detection, hitlag/intangible/invincible/heal counters,
`ftMainPlayAnimEventsAll`, `ftMainRunUpdateColAnim`, the three status function
pointers `proc_passive`/`proc_update`/`proc_interrupt`, and a two-fighter jostle
loop. Almost all of that is indirect dispatch or a call into the animation-event
interpreter — the proc itself holds arithmetic for the stick buffers and little
else.

The measurement agrees, and it is the answer to *"say what it spends its time on
before anyone designs anything"*:

| | self tk/fr on the P95 set | whole-match tk/fr | presence |
|---|---:|---:|---:|
| `battleship_ftMainProcUpdateInterrupt` | **8,119** | 7,669 | **1.06x** |
| `ndsBaseGcRunAll` | 9,386 | 9,116 | 1.03x |
| `gcRunGObjProcess` | 5,318 | 4,886 | 1.09x |
| `ftGetStruct` | 9,498 | 8,320 | 1.14x |

`SINT`'s **entire exclusive** static closure is **20,278 tk/fr over 29
functions** — the proc body 8,119, then `ftComputerCheckDetectTarget` 1,853,
`ftComputerCheckFindTarget` 1,383, `ndsStageMPCeilFloorLoopSweep` 1,299,
`ftComputerUpdateInputs` 1,180 (and the `ftComputer*` rows belong to `SCPU`,
which `SITR` subtracts).

**So `SITR` is a bracket around dispatch, not a body of work.** A mechanism
written against `SITR` would have to attack what it dispatches *to*: the
animation-event interpreter (`ftParamUpdateAnimKeys` 14,680 at 1.38x,
`gcPlayDObjAnimJoint` 27,108 at 1.36x, `ndsR2AnimValueQ` 27,909 at 1.45x) — all
of which are shared with `SPRM` and the draw side, and none of which spikes.
`plan.md` §9's *"no mechanism has ever been priced against `SITR` itself"* is now
answered: there is nothing in `SITR` itself to price. **Closed as a target.**

---

## 3. `SHDT` — lane or layer, stated

**Layer.** `SHDT`'s *exclusive* closure is 6,279 tk/fr over 32 functions
(`battleship_ftMainSearchHitFighter` 2,218 at 1.61x, `…SearchGroundHit` 747,
`…SearchHitWeapon` 645, `…ProcSearchHitAll` 543). Its bracket excess is
+86,833 at 16.34x. The difference is entirely in code `SHDT` shares with
`SPHD`, `SPRM`, `SCPU` and the renderer — the **fighter-part world-matrix and
hurtbox geometry in `gm/gmcollision.c`**, which was UNFROZEN 2026-08-13.

`HANDOFF.md` records `SHDT` CLOSED on a bar of 47,424 with band-only cuts
saturating at 78,016 and *"fixed point only"*. That verdict is a
micro-optimisation verdict measured on a whole-match mean. `plan.md` §3's
standing caveat — *a lane closed as a micro-optimization says nothing about
whether the work should exist* — applies, and the closing note itself already
named the exit: **fixed point only.**

---

## 4. The trio, attributed to its CALLERS, on the frames that set P95

Method: the instruction count at a `bl <helper>` PC **is** that site's exact
dynamic call count, and the reduced per-PC CSV carries `marg_instructions`, so a
P95-masked caller attribution costs no build and no extra emulator pass. Rate =
the helper's own marginal cycles ÷ its exact marginal call count — a measured
rate, not a residual divided by a count. Tooling: `analyze-leaf-helper-
attribution.py --pc-csv … --mask marginal` and `census-marginal-frame-owners.py
--census-out`, both added this cycle (§8).

```text
helper            calls (80 fr)   cycles      cyc/call
__aeabi_fmul            243,439   6,323,184       26.0
__aeabi_fadd            147,578   8,170,121       36.3
__aeabi_fsub             77,266      77,266       36.3  (thunk -> fadd)
__aeabi_fdiv             19,105   2,253,409      117.9
attributed 19,384,853 cycles = 9.14% of non-idle
```

| caller | tk/fr | float calls/fr | subsystem |
|---|---:|---:|---|
| **`func_ovl2_800ED490`** | **11,808** | 776.5 | gmcollision — the 4x4 compose |
| **`gmCollisionSetInvertMatrix`** | **6,120** | 406.4 | gmcollision |
| `ndsBaseGcPlayMObjMatAnim` | 5,500 | 437.6 | animation |
| **`gmCollisionGetWorldPosition`** | **5,310** | 340.9 | gmcollision |
| `syMatrixLookAtReflectF` | 4,871 | 319.4 | camera |
| `ndsStageMPAdjustFloorLoopWallSweep` | 4,701 | 275.3 | `mp*` — **FROZEN** |
| **`gmCollisionTestRectangle`** | **4,466** | 212.8 | gmcollision |
| **`gmCollisionTransformMatrixAll`** | **4,318** | 335.3 | gmcollision |
| `ndsR2FtAnimParseDObjFigatree` | 4,094 | 225.3 | animation |
| `lbCommonCos` / `lbCommonSin` | 3,266 / 2,137 | 310.9 / 248.8 | trig |
| `mpCollisionGetFCCommonFloor` | 2,961 | 170.6 | `mp*` — **FROZEN** |
| **`gmCollisionGetFighterPartsWorldPosition`** | **2,790** | 179.1 | gmcollision |

Subsystem roll-up on the P95 set: **collision / stage MP 2.41%** of non-idle
(largest), other 2.30%, gameplay 1.38%, matrices 1.25%, animation 1.10%, CPU AI
0.27%, renderer 0.25%, particles 0.18%.

> **This retracts `plan.md` §9's "soft float does not promote".** That reading
> was *"74,283 tk/fr caller-attributed whole match, largest single caller
> 5,402"* — a whole-match attribution on `build-c159`. Masked to the frames the
> gate is scored on, the largest single caller is **11,808** and seven of the
> top twelve are one family in one file. The lane concentrates; the earlier
> population hid it.

**Two corroborations that cost nothing and could have failed.**

1. The profile says `func_ovl2_800ED490` issues **63.0** soft-float calls per
   invocation (62,118 calls ÷ 986 invocations). Its source
   (`gmcollision.c:208-225`) is **36 multiplies and 27 adds = 63**. Profile and
   source agree to the operation.
2. Price that op mix at the profile's *own* measured helper rates —
   `36 × 26.0 cyc + 27 × 36.3 cyc = 1,916 cycles = 958 ticks` — against the
   attribution, which is `11,808 tk/fr ÷ 12.325 calls/fr = 958 ticks`.
   **Identical.** Three independent quantities (the source's operation count,
   the profile's per-site call counts, the profile's per-helper rates) close on
   one number, which is what makes the 1,290 tk/compose a price rather than an
   estimate.

---

## 5. The cluster, priced

All figures: ticks/frame on the 80 frames that set P95, `build-c172`.
*self* = the symbol's own census row; *float* = soft-float attributed by exact
call count × measured rate; *sqrtf* likewise.

| body | self | float | sqrtf | **total** | presence | calls/fr | **tk per call** |
|---|---:|---:|---:|---:|---:|---:|---:|
| `func_ovl2_800ED490` (compose) | 4,055 | 11,808 | — | **15,863** | 7.68x | 12.3 | **1,290** |
| `gmCollisionSetInvertMatrix` | 2,524 | 6,120 | — | **8,644** | 11.54x | 6.7 | **1,297** |
| `gmCollisionTransformMatrixAll` | 3,514 | 4,318 | — | **7,832** | 5.19x | 15.3 | 512 |
| `gmCollisionGetWorldPosition` | 1,711 | 5,310 | — | **7,021** | 11.46x | 18.9 | 371 |
| `gmCollisionTestRectangle` | 2,102 | 4,466 | — | **6,568** | 11.70x | 11.3 | 581 |
| `func_ovl2_800EDE5C` (part scale) | 681 | 1,505 | 2,931 | **5,117** | 11.27x | 11.3 | 453 |
| `gmCollisionGetFighterPartsWorldPosition` | 1,803 | 2,790 | — | **4,593** | 3.54x | 2.3 | 2,014 |
| `func_ovl2_800EDBA4` (chain walk) | 1,737 | — | — | **1,737** | 6.90x | 7.0 | 248 |
| **cluster** | **18,127** | **36,317** | **2,931** | **57,375** | | | |

Plus `lbCommonSin` + `lbCommonCos` **4,975 tk/fr**, six per local-matrix build,
which the fixed kernel replaces with its own sine table.

**One 4x4 matrix compose costs 1,290 ticks.** 960 of those are 63 soft-float
library calls at 26–36 cycles each. That is the defect, and it is arithmetic,
not cache: the trio's rows are 96% `issue` with essentially zero `icache_fill`
or `dcache_fill`.

**Against +32,593 net at the 80th-largest frame:**

```text
float + sqrtf call cost, deleted outright by a fixed kernel   39,248   1.20x
   ... plus the sin/cos the fixed local build replaces        44,223   1.36x
whole cluster (float deleted + self replaced by integer)      57,375   1.76x
```

---

## 6. The package — and it is already written

`src/port/nds_r2_collision_fixed.c` + `include/nds/nds_r2_collision_fixed.h`
already contain `ndsR2CfxBuildLocal`, `ndsR2CfxCompose`,
`ndsR2CfxMakeFrameCofactor`, `ndsR2CfxWorldToLocal`, `ndsR2CfxTransformPoint`
and `ndsR2CfxTestRectangle`, built `-marm` in their own TU (Thumb has no SMULL),
with a build check that fails if any soft-float or `__aeabi_lmul` call survives
in the object. The file's own header: *"NOTHING CALLS THESE YET, by design …
Wiring them — deleting the float bodies, not wrapping them — is a separate cycle
with its own map check and gate A/B."* Board row: **Slice 52 — SEAM CORRECTED,
NOT WIRED. Re-brief before spending a build.** Its live-domain grading is
already done: 152 of 152 live joint matrices admitted, zero declines, max
fixed-vs-float **0.0003662** against a **0.0200** bound.

### 6.1 Why slice 52 as scoped does not clear the gate, measured

The seam correction's deletable set, priced on the P95 set:

| deletable body | total | of which float/sqrtf |
|---|---:|---:|
| `gmCollisionSetInvertMatrix` | 8,644 | 6,120 |
| `gmCollisionTestRectangle` | 6,568 | 4,466 |
| `func_ovl2_800EDE5C` | 5,117 | 4,436 |
| `gmCollisionTestSphere` | **0 — present at `0x0207fcd0`/0x4ac, never executed** | — |
| **ring** | **20,329** | **15,022** |
| + `gmCollisionGetWorldPosition` (the ≤31,278 arm) | 27,350 | 20,332 |

**0.46x–0.84x the requirement.** Short. The board's row already suspected this
and said *"brief cycle C as a measurement, not a landing"* — it was right, for a
reason it could not see: it compared **22,324 tk/fr certain** against a
**whole-match** bar of 47,424, and both halves of that comparison are the wrong
population. On the population the gate is scored on, the same ring is 11.3–11.7x
more present and the requirement is +32,593, not 47,424.

### 6.2 The one extension that closes it

**Give `func_ovl2_800EDBA4` a fixed-point interior with an f32 boundary.**

`func_ovl2_800EDBA4` is the chain walk: it climbs to the first ancestor whose
world matrix is valid, builds each missing local with
`gmCollisionTransformMatrixAll`, and composes back down with
`func_ovl2_800ED490`. The seam correction blocks converting either callee
because `parts->mtx_translate` stays f32 and EDBA4 has **fifteen** referrers
including the renderer adapter and `battleship_ftMainProcParams`.

Keep `mtx_translate` f32 and that objection disappears: nothing outside EDBA4
changes. Inside it —

- each missing local is built by `ndsR2CfxBuildLocal` (proven, 1,180 B, and it
  deletes the six `lbCommonSin`/`Cos` per local);
- each compose is `ndsR2CfxCompose` (proven, 344 B);
- the result converts to f32 exactly once per joint, when `mtx_translate` is
  written.

Those are the **1,524 of the 4,448 proven bytes the seam correction wrote off
as having "no call site at this seam"**. With EDBA4 converted they have one, and
`gmCollisionSetInvertMatrix` / `func_ovl2_800EDE5C` / `gmCollisionTestRectangle`
can then take their fixed inputs directly instead of re-reading f32.

**Prediction, made before the build** (this is a prediction, not a measurement):

| removed | tk/fr |
|---|---:|
| `func_ovl2_800ED490` soft float | 11,808 |
| `gmCollisionTransformMatrixAll` soft float | 4,318 |
| `lbCommonSin`/`Cos` under the local build | ≈4,975 |
| `gmCollisionSetInvertMatrix` soft float | 6,120 |
| `gmCollisionTestRectangle` soft float | 4,466 |
| `func_ovl2_800EDE5C` soft float + sqrtf | 4,436 |
| **subtotal deleted** | **36,123** |
| paid back: f32↔fixed boundary, ~24 conversions × 7.0 chains/fr | ≈ −840 |
| paid back: integer arithmetic replacing 18,127 of self time | not predicted |
| **net predicted at rank-80** | **+30,000 … +38,000** |

**Which frames it comes off:** the ones where a hitbox is live. Every body above
is 5.2–11.7x present on the P95 set, so the cut lands *on the frames that set
P95* rather than uniformly — the exact shape `plan.md` §0 demands and the shape
`mean-self-time-predicts-P50-not-P95` says to check for.

### 6.3 The instrument

Cross-build floor here is ~17,000 P95, so a two-build comparison under that
measures the linker. **A/B/A with a flag falsifier** (slice-51 method, which
bracketed to 2,752):

- **A** — `NDS_R2_COLLISION_FIXED=0`: the candidate's layout, dispatch reverted.
- **B** — `NDS_R2_COLLISION_FIXED=1`.
- **A′** — repeat A.

Both arms must report, from the same run that produced the buckets:

```text
gNdsCfxFighterDamagePhaseCalls   equal on A and B   (same fight)
gNdsCfxFighterDamagePhaseHits    equal on A and B   (same decisions)
gNdsCfxFighterShieldPhaseCalls   equal on A and B
gNdsCfxFighterShieldPhaseHits    equal on A and B
end-of-match invariant pair      equal on A and B
```

A tick delta with a moved hit count is a changed fight, not a saving
(`route-ab-cannot-price-gameplay-change`). A fixed kernel that quietly stops
returning `TRUE` would otherwise read as a large, clean win.

---

## 7. The counter landed this cycle

`plan.md` §9 law 1: *land a per-frame engagement counter on the spiking quantity,
on the gate arm, before any code changes.* Landed in
`src/import/battleship_gmcollision.c`, gated `#if NDS_TICK_HUD` so the published
ROM carries zero bytes, `__attribute__((used))` so `--gc-sections` cannot
collect a debugger-only global.

**Site chosen from the linked ELF, not from grep.**
`gmCollisionCheckFighterAttackDamageCollide` has exactly two referrers and
`…AttackShieldCollide` exactly one, all three in `battleship_ftmain.c` —
**zero in-TU callers**, which is what makes the `#define`-before-`#include`
rename capture every call. `gmCollisionTestRectangle` and
`gmCollisionSetInvertMatrix` do **not** have that property (their callers are
inside `gmcollision.c`), so renaming those would move the definition *and* the
call sites and the counter would read zero forever. That trap is written into
the comment beside the counter.

The chosen entry point is **10.81x present** on the P95 set and is the gateway
to every body in §5's table.

### 7.1 It fires, and it reproduces the profile

`build-c173-cfxcount-bp1` = the shipping-candidate gate arm (`BOTH_CPU=1`,
`BATTLEPACK=1`, `KEEP_CACHE=1`, `DRAW=1`), `-Samples 1600 -StartFrame 438
-RingDump`, DLDI on, window 439–2038, ROM `8F39BAB2F829C690…`.

```text
gNdsCfxFighterDamagePhaseCalls   1,938        gNdsCfxFighterDamagePhaseHits   20
gNdsCfxFighterShieldPhaseCalls       0        gNdsCfxFighterShieldPhaseHits    0
gNdsBattlePackHits                 197        gNdsTaskmanArenaChosenSize 1,548,288
gNdsTaskmanArenaAllocFailCount       0        gNdsTaskmanGeneralHeapFreeMin 52,864
```

1,938 / 1,600 = **1.21 calls per frame whole-match**. The profile's own numbers
put the same function at **13.1 calls/frame on the P95 frames** (self 875 tk/fr
÷ 67 tk/call). 13.1 / 1.21 = **10.8x** — the counter, taken on a different arm
with a different instrument, reproduces the capture's 10.81x presence. That is
the law-1 artifact.

**`gNdsCfxFighterShieldPhaseCalls` reads 0 because nothing shielded an attack in
this match. That arm is UNPROVEN, not proven inert** — an arm that cannot
produce the event reads 0 either way.

`BattlePackHits` 197 and heap low-water 52,864 are identical to the banked arm,
so the fight did not move.

**Boundary GREEN at the shipping default** on this tree — `Boundary
verification profile passed.`, **0 `Exception:`** in the log
(`boundary.trimmed.log`). The counter is structurally absent from it rather
than merely unexercised: the proof target's own generated
`nds_build_config.h` reads `#define NDS_TICK_HUD 0` and `nm` on
`smash64ds-battle-playable-proof-hwtri.elf` finds **zero** `gNdsCfxFighter*`
symbols. Both published targets pin the same value
(`Makefile:1455` for `smash64ds-battle-playable-hwtri`, `:1944` for the
published-basename block; nothing raises `NDS_TICK_HUD` except the tickhud,
results-lab and task37/44-off variants), so neither root ROM can contain it.

**And a figure that must not be read as a bank.** This run's `WORK-H` is
**P50 951,744 / P95 1,200,960** against `build-c170-seam-bp1`'s banked
940,320 / 1,177,920 — **+23,040**, inside the ±24,064 one-line spread but above
the ~17,000 floor. The counter's own rate bounds *its* contribution at 1,938
wrapper calls over 1,600 frames ≈ **18–24 ticks/frame**, three orders under, so
it is not the cause. The only other source difference between the two binaries
is the K0 site counters (`0f121aec2c2`/`48741fcaf05`) — a candidate, not a
measurement. **The +23,040 is unattributed and is handed forward; `c170-seam-
bp1` remains the bank.** VBI 2:1731 3:287 4:11 5+:9 max 19, slips 0,
cadence violations 0.

---

## 8. Tooling added, so the next cycle does not re-derive this

Both changes exist because the question *"which caller drives this helper ON THE
FRAMES THAT SET P95"* was previously inexpressible, and the campaign answered
the whole-match question instead and closed a lane on it.

- `census-marginal-frame-owners.py --census-out <path>` writes a
  `census.json`-shaped file whose cycles are the **marginal mask**, so
  `analyze-subtree-attribution.py` and `analyze-leaf-helper-attribution.py` rank
  the P95 frames with no code change of their own. One row per address range
  with `aliases` listed — `nm` reports `__aeabi_fmul` and `__mulsf3` at the same
  address, and the first smoke test of this export read the entire soft-float
  multiply lane as **zero** for exactly that reason.
- `analyze-leaf-helper-attribution.py --pc-csv <reduced csv> --mask marginal`
  takes its call counts from `marg_instructions` instead of a full 3.6 GB pass,
  resolves helper names through `aliases`, and prints `tk/fr` and `calls/fr` on
  the stated basis. It refuses to take both a profile and a `--pc-csv`.

Validated against the banked `c159` capture before use: `__aeabi_fadd` came back
7,796,990 marginal cycles ÷ 160 = **48,731 tk/fr**, matching that arm's banked
census row to the tick.

---

## 9. What this cycle did NOT do

- **The package was not implemented.** Pricing is decisive; the change is not
  small — it converts a shared chain walk with fifteen referrers and needs its
  own map check, host falsifier re-grade at the widened scale domain (the board
  records the live domain is **0.9937–2.0479**, not the inherited 1.1138–1.1199)
  and an A/B/A. `AGENTS.OPUS.md`'s splitting rule: a finished subset beats an
  unverified whole.
- **No `Latest` verifier run.** The only source edit is `#if NDS_TICK_HUD`-gated
  and absent from the published ROM by construction.
- **Slice 2 was not briefed, designed or re-opened.** No draw-side work. No
  cadence work. No flag flips.
- **Task C, the 2^22 discriminator, was not run — and the handed-forward
  invocation cannot run as written.** `-PerFrameGlobals` is *incompatible with
  `-RingDump` by construction* (`sample-tick-hud-buckets.ps1:229`), and a
  whole-match gate run requires `-RingDump`. The correct vehicle is
  **`-PerStopGlobals gNdsRelocAssetPayloadReadCount,gNdsR2AnimCacheFills`**,
  which stitches per ring stop; at a 96-frame stride it answers *"did a FAT read
  happen in the window containing that frame"*, not *"on that frame"*. A
  per-frame answer needs a non-`-RingDump` run, i.e. 1,600 gdb stops.
- **No root ROM was rebuilt.** Hashed before the first build and after the last:
  `smash64ds.nds` `54c07fac80c50418…`,
  `smash64ds-battle-playable-hwtri.nds` `2015fbd1f68b81c0…` — unchanged.

---

## 10. Reproduction

```powershell
# the arm: shipping candidate + DRAW=0 + the profiler (560 s, writes 3.6 GiB, not committed)
.\scripts\run-task37-profile-census.ps1 -MelonDS emulators\melonds-attributor\melonDS.exe `
    -Build build-c172-profile-shipcand -StartFrame 438 -Frames 1600 -TimeoutSeconds 7200 `
    -MakeFlags NDS_R2_BOTH_CPU=1,NDS_TICK_HUD_DRAW=0,NDS_R2_BATTLEPACK=1,NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    -OutDir artifacts/performance/2026-08-15_k1-owner-pricing/v3-c172

# reduce once; the reduced CSV IS committed so nobody re-scans 3.6 GB
python scripts/census-marginal-frame-owners.py --reduce `
    --profile artifacts/performance/2026-08-15_k1-owner-pricing/v3-c172 `
    --out artifacts/performance/2026-08-15_k1-owner-pricing/c172-p95-pc.csv --marginal 80

# rank, and emit the marginal-mask census.json the caller attribution needs
python scripts/census-marginal-frame-owners.py --report --owner-roots `
    --pc-csv artifacts/performance/2026-08-15_k1-owner-pricing/c172-p95-pc.csv `
    --build builds/build-c172-profile-shipcand --top 34 `
    --census-out artifacts/performance/2026-08-15_k1-owner-pricing/c172-p95-census.json

# the trio, attributed to its callers, on the 80 frames that set P95
arm-none-eabi-objdump -d builds/build-c172-profile-shipcand/*.elf > c172.dis
python scripts/analyze-leaf-helper-attribution.py `
    --pc-csv artifacts/performance/2026-08-15_k1-owner-pricing/c172-p95-pc.csv --mask marginal `
    --census artifacts/performance/2026-08-15_k1-owner-pricing/c172-p95-census.json `
    --dis c172.dis --helpers softfloat --top 30
# and the same tool for the builders rather than the leaves, which is how the
# cluster's per-call prices and exact call counts came out:
#   --helpers func_ovl2_800EDBA4,func_ovl2_800ED490,gmCollisionTransformMatrixAll,…

# the law-1 counter arm: one gate run, engagement + inertness + the fight unchanged
.\scripts\sample-tick-hud-buckets.ps1 -RunnerSlot 2 -Build build-c173-cfxcount-bp1 `
    -Samples 1600 -StartFrame 438 -RingDump -TimeoutSeconds 2400 `
    -MakeFlags NDS_R2_BOTH_CPU=1,NDS_R2_BATTLEPACK=1,NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    -ExtraGlobals gNdsCfxFighterDamagePhaseCalls,gNdsCfxFighterDamagePhaseHits,`
gNdsCfxFighterShieldPhaseCalls,gNdsCfxFighterShieldPhaseHits,gNdsBattlePackHits,`
gNdsTaskmanArenaChosenSize,gNdsTaskmanArenaAllocFailCount,gNdsTaskmanGeneralHeapFreeMin `
    -RowsCsv …\c173-cfxcount-rows.csv -JsonOut …\c173-cfxcount.json
```
