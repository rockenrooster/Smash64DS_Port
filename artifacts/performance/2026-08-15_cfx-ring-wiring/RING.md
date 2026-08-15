# Slice 52 WIRED, MEASURED, TICK-NEUTRAL — and the collision decisions did not move

Date: 2026-08-15. Branch `codex/r2-runtime2`. Base HEAD `223fb499ca5`.
Prediction and flip budget: `PREDICTION.md` beside this file, written before the
first build.

## 0. Outcome first

**The ring is wired and engaged, no collision decision changed, and the gate did
not move.**

A/B/A', where **A' and B are byte-identical except one byte** (§5), so this
comparison has no placement floor at all:

```text
                    A' c176 (off)   B c175 (ON)   A c174 (off)   B - A'
WORK-H P50              942,336        942,400        942,272       +64
WORK-H P90            1,094,976      1,094,912      1,094,080       -64
WORK-H P95            1,173,120      1,174,016      1,173,376      +896
rank-80               1,173,696      1,177,344      1,173,760    +3,648
top-1%                1,513,920      1,517,504      1,511,936    +3,584
VBI 2/3/4/5+        1736/285/8/9   1741/278/9/10  1744/275/11/8
slips                         0              0              0
required at rank-80                                             -32,593
```

**0.00x of the requirement, and every percentile that moves at all moves the
wrong way.** The two controls bracket each other to +/-896 at P95 and +/-64 at
rank-80; the candidate sits 3,648 ticks OUTSIDE that bracket at rank-80, so the
small cost is real rather than noise. It is a cost of about 900-3,650 ticks, not
a saving of 32,593. The predicted
+25,000…+32,000 in `PREDICTION.md` §2 is **REFUTED by measurement**, and so is
the board's +30,000…+38,000 for this shape of the change.

The mechanism is not inert and this is not a wiring failure. Every counter says
it ran, on the right frames, on every joint it was offered:

```text
gNdsCfxRingPrepareCalls    1,938   == gNdsCfxFighterDamagePhaseCalls exactly
gNdsCfxRingChainFixed      1,006   gNdsCfxRingChainDeclined        0
gNdsCfxRingLocalsBuilt     1,314   gNdsCfxRingComposes         1,621
gNdsCfxRingInvertFixed     1,006   gNdsCfxRingInvertDeclined       0
gNdsCfxRingScaleFixed      1,006   gNdsCfxRingScaleDeclined        0
every one of the nine = 0 on arm A                 <- the negative control
```

**Zero declines of any kind.** The domain guards the falsifier exists to protect
never fired once in a whole both-CPU match: not the rotation guard, not the
translation guard, not the s² guard, not the determinant guard, not the chain
depth. The live domain is inside the proven domain, which is what the widened
0.9937-2.0479 re-grade predicted.

## 1. The flip budget was met exactly: zero

Stated before the run, measured after. Every one of these is byte-identical on
arm A and arm B:

```text
gNdsCfxFighterDamagePhaseCalls   1,938 / 1,938      (same fight)
gNdsCfxFighterDamagePhaseHits       20 /    20      (same decisions)
gNdsCfxFighterShieldPhaseCalls       0 /     0      (unproven, not inert)
gNdsCfxFighterShieldPhaseHits        0 /     0
gNdsBattleTextHudP0Damage            0 /     0
gNdsBattleTextHudP1Damage           76 /    76
gNdsStarKOSparkleCount               0 /     0
gNdsDamageSparkScaleCount           15 /    15
gNdsShieldAnimJointAttachCount   1,352 / 1,352
gNdsAObjEvent32NormalizedHighWater 1,266 / 1,266
gNdsObjAnimRunawayCount              0 /     0
gNdsTaskmanGeneralHeapFreeMin   52,864 /52,864
gNdsTaskmanArenaAllocFailCount       0 /     0
gNdsBattlePackHits                 197 /   197
slips                                0 /     0
```

The same pair also matches the `c170-seam-bp1` bank arm (P1 damage 76, runaway 0,
heap low-water 52,864, pack hits 197), so all three binaries fought the same
match. `route-ab-cannot-price-gameplay-change` is satisfied in the only direction
that matters: there is no tick delta to explain, and there is no fight delta
either.

**Correctness result: the fixed-point producers reproduce the float producers'
gameplay outcome exactly over a whole match.** That is worth keeping even though
the ticks are not.

## 2. What was wired, and the one place it departs from the brief

Three PRODUCERS moved to fixed point behind an f32 boundary; the CONSUMER did
not move at all.

| decomp body | after |
|---|---|
| `func_ovl2_800EDBA4` chain walk | fixed interior, f32 per joint |
| `gmCollisionTransformMatrixAll` | replaced inside the chain (`ndsR2CfxBuildLocal`) |
| `func_ovl2_800ED490` compose | replaced inside the chain (`ndsR2CfxCompose`) |
| `func_ovl2_800EDE00` inverse | Q26 cofactor, f32 into `unk_dobjtrans_0x9C` |
| `func_ovl2_800EDE5C` axis scales | integer roots, f32 into `vec_scale` |
| `gmCollisionTestRectangle` | **untouched decomp float** |
| `gmCollisionTestSphere` | **untouched decomp float** |
| `gmCollisionGetWorldPosition` | **untouched decomp float** |

**Departure, stated as one.** The brief and `STACK.md` §5 assume
`unk_dobjtrans_0x9C`'s 64 bytes are reinterpreted as a fixed `NDSR2CfxFrame`,
which is precisely what obliges `gmCollisionTestSphere` to be converted (§5.1 —
it writes a shield hit's knockback ANGLE and a normal, continuous values no flip
count can express). Keeping the slot f32 removes that obligation entirely and
leaves every collision decision in decomp code on decomp comparisons, which is
why this change could be graded by a bound instead of argued. The price was
`gmCollisionTestRectangle` (6,568 tk/fr on the P95 set) and
`gmCollisionGetWorldPosition` (7,021), left on the table on purpose.

That departure is **not** why the result is zero. See §4: the three producers
that *were* converted are worth ~2,400 tk/fr at P50 by their own measured call
counts and static sizes, and that did not appear either.

## 3. The instrument, and why the null is trustworthy

The usual flag falsifier does not work for this change: at
`NDS_R2_COLLISION_FIXED=0` the objects leave the link entirely, so the
"candidate layout" arm is the control layout again. `NDS_R2_COLLISION_FIXED_DISPATCH`
instead flips one initialised word of `.data`, read through a `volatile` the
compiler cannot fold, so both arms compile and link the same code.

Arms, all `NDS_R2_BOTH_CPU=1`, `NDS_R2_BATTLEPACK=1`,
`NDS_R2_BATTLEPACK_KEEP_CACHE=1`, DLDI on, `NDS_TICK_HUD_DRAW` default, mode 163
one-minute, `-Samples 1600 -StartFrame 438 -RingDump`, frames 439-2038:

| arm | build | dispatch | rows |
|---|---|---|---|
| A | `build-c174-cfxring-a` | 0 | `a-c174-off-rows.csv` |
| B | `build-c175-cfxring-b` | 1 | `b-c175-on-rows.csv` |
| A' | `build-c176-cfxring-a2` | 0 | `a2-c176-off-rows.csv` |

**A structural correction found by reading the symbol table, not by assuming.**
Arm A was built before `gNdsCfxRingEnable` carried `section(".data")`. With a
zero initialiser the linker put it in `.bss`; with a one it goes to `.data`. So
A and B differ by four bytes of each, and `arm-none-eabi-objcopy --only-section`
reports `.text.hot`, `.main` and `.main.rw` DIFFER between them. A' repeats arm A
with the attribute; §5 records the section comparison against B.

**And a comparison that read IDENTICAL and proved nothing.** The first section
check ran `--only-section=.text` on both ELFs and got the same hash — the SHA-256
of the *empty string*, because this linker script has no `.text`: the code is in
`.itcm`, `.text.hot`, `.text.hot.draw` and `.main`. `prove-the-control-differs`
applies to a byte comparison as much as to a run.

Percentile convention: the harness's own (ascending index `int(n*p)-1`).
`rank.py` beside this file prints that and the 80th-largest separately, and was
validated against `build-c147-ctl` before use — it reproduces that arm's
published P50 924,864, P95 1,210,944 and rank-80 1,212,224 to the tick.

## 4. Why zero — two named candidates, neither of them measured

The counters and the shipped ARM make the arithmetic side concrete. Sizes are
from `nm` on the arm-B ELF; the ring's kernels are all `-marm`, so bytes/4 is
exactly the static ARM instruction count.

```text
entry point                        bytes   ARM instructions
ndsR2CollisionFixedLoadF32           448    112
ndsR2CollisionFixedStoreF32          516    129   (12 conversions, ~11 each)
ndsR2CollisionFixedCompose           344     86
ndsR2CollisionFixedInvertF32       1,592    398
ndsR2CollisionFixedAxisScalesF32     912    228   plus 3 isqrt loops
ndsR2CollisionFixedBuildLocal      1,180    295
ndsR2CfxPrepareFighterJoint          528     -    (Thumb glue)
                                   -----
added ARM text on the hot path     5,596 bytes
```

Against the profile's measured float prices (`…/2026-08-15_k1-owner-pricing/`
§5: one compose 1,290 ticks, one inverse 1,297, `TransformMatrixAll` 512 plus
its share of `lbCommonSin`/`Cos`, `EDE5C` ~787 per working call), the run's own
call counts give an expected whole-match saving of roughly:

```text
1,621 composes  x ~1,095 saved  = 1.78M ticks  -> ~1,110 tk/fr
1,006 inverts   x ~1,000        = 1.01M        ->   ~630
1,314 locals    x   ~590        = 0.78M        ->   ~485
1,006 scales    x   ~300        = 0.30M        ->   ~190
                                                 -------
                                                  ~2,415 tk/fr at P50
```

**Measured at P50: +64, i.e. nothing, and at rank-80 a 3,648-tick COST.** So
~2,500 tk/fr of predicted saving is unaccounted for, and there are two
candidates. **Neither is measured, and this cycle does not choose between them:**

1. **The f32 boundary.** The run's own counters put the number of scalar
   conversions at roughly 90,000 per match — `12 x LocalsBuilt` +
   `12 x Composes` + `27 x InvertFixed` + `15 x ScaleFixed` plus the chain-boundary
   reloads — about 60 per frame whole-match and, at the damage phase's 10.81x
   presence, several hundred on the frames that set P95. At ~11 instructions each
   that is not obviously enough to cancel 2,415, which is why it is the *weaker*
   candidate.
2. **The I-cache footprint of 5,596 bytes of new ARM text that now EXECUTES.**
   This is R2-07 L7's documented failure mode reproduced with a better
   instrument: L7 measured 2,332 bytes of added ARM text costing 1.85
   cycles/frame per byte. At that rate 5,596 bytes is ~10,350 cycles/frame =
   **~5,175 ticks/frame**, which more than absorbs the arithmetic win. Arm A
   *links* those bytes and does not execute them and measures −4,288 against the
   `c170` bank, so linking them is free; only executing them is not.

**The discriminator is free of a build**: a v3 stall capture on arm B against
arm A splits `icache_fill` from `issue` directly. Candidate 2 predicts
`icache_fill` up and `issue` down; candidate 1 predicts both roughly flat with
the change inside `issue`. That is the next cycle's first read, and it costs one
560 s capture and no rebuild.

## 5. Section identity between the arms — the placement floor is ZERO, verified

`arm-none-eabi-objcopy -O binary --only-section=<s>` on
`build-c176-cfxring-a2` (dispatch 0) and `build-c175-cfxring-b` (dispatch 1),
byte-compared with `cmp -l`:

```text
.itcm            32,152 B    differing bytes 0
.text.hot         4,588 B    differing bytes 0
.text.hot.draw    5,268 B    differing bytes 0
.main           932,864 B    differing bytes 0
.main.rw        137,428 B    differing bytes 1     <- and it is THE bit
.dtcm             8,800 B    differing bytes 0
```

The one differing byte is at `.main.rw` offset 0x3F24. `.main.rw` has VMA
`0x020E7640` and `nm` puts `gNdsCfxRingEnable` at `0x020EB564`;
`0x020EB564 − 0x020E7640 = 0x3F24`. **The two ROMs differ in exactly the
dispatch switch and in nothing else.**

So the usual caveats do not apply to the A'/B comparison: there is no
cross-build placement floor, no `±17,000`, no `±24,064` one-line spread, and no
`gc-sections` difference. Whatever separates A' from B is the dispatch. It is
`−1,344` at P95.

Arm A (`build-c174-cfxring-a`) does NOT have this property — it predates the
`section(".data")` attribute and its `.text.hot`, `.main` and `.main.rw` all
differ from B. It is reported as a third sample of the control's behaviour, not
as the falsifier.

## 6. What this closes and what it opens

**Closed by measurement:** *"give `func_ovl2_800EDBA4` a fixed-point interior
with an f32 boundary and collect +30,000…+38,000 at rank-80"*. It is wired, it
engages on every offered joint with zero declines, it changes no collision
decision, and it is worth **0**. The prediction was mine and the board's; both
are retracted here rather than in a later cycle.

**Opened, and now with evidence behind it:** the header's own design sentence —
*"the representation crosses the float boundary exactly twice per joint per
frame … instead of once per call"* — is **not achievable while `mtx_translate`,
`unk_dobjtrans_0x10`, `unk_dobjtrans_0x9C` and `vec_scale` all stay f32.** This
wiring crosses it four to six times per joint per frame because every
intermediate is stored back into an f32 field a decomp consumer reads. Any
version of this cluster that can pay has to keep the fixed representation
*resident* across the whole narrow phase, which means the `unk_dobjtrans_0x9C`
reinterpretation the board already proved available (`sNdsFighterPartsPool`,
`ndsFighterPartsSyncDObj`, `ndsFighterPartsSetIdentity` all absent from both
linked ELFs) **and** a fixed `mtx_translate`, which the seam correction ruled out
on its fifteen referrers. That is a much larger change than this one, and it now
has a measured reason to exist rather than a projected one.

**Kept regardless of the ticks:**
- the falsifier is re-graded at the live domain the game actually visits,
  0.9937-2.0479 instead of the stale 1.1138-1.1199, and it is GREEN;
- `T8`, a new falsifier section that grades the WIRING rather than the kernels —
  the f32 fields the ring writes against the f32 fields the decomp float
  producers write, with `unk_dobjtrans_0x10` round-tripped on every level;
- the nine engagement counters and the byte-identical-dispatch falsifier, which
  are what made this a two-hour verdict instead of an argument.
