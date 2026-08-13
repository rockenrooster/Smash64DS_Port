# The `SHDT` 88-frame band has an owner: the fighter hurtbox narrow phase

**Outcome: named, sized, and NOT cut this cycle.** The band that contains the
P95 frame is the **fighter world-transform + hurtbox-overlap chain**
(`gmCollisionCheckFighterAttackDamageCollide` and the four latched transform
stages behind it), at **+67,230 ticks/frame = 42.3%** of the band's premium.
The cut it implies needs **47,424 ticks/frame off those 88 frames** to pay
16,000 `WORK-H` P95, i.e. **35% of a chain that is already memoised exactly
once per joint per frame and whose arithmetic is inside the frozen-float
fence.** No design reaches that inside this cycle's rails, so this document
hands forward the attribution, not a change.

**No build, no emulator run, no ROM, no source change to any runtime path.**
Both root ROMs are untouched. Every figure is an exact per-region cycle total or
an exact dynamic call count off two artifacts that were already on disk.

**And the measurement the last cycle costed as "a counter inside the bracket,
which is a build" did not need one.** The profiler emits one row per
`(region, pc)`, so the region axis already *is* a per-frame series for every
symbol in the ROM. Carrying the gate run's own `SHDT` column in as a mask and
splitting the profile on it takes 26 seconds. That is the reusable finding, and
it now lives in `scripts/analyze-profile-region-split.py`.

---

## 0. Sources, alignment, and how to reproduce

- `../2026-08-12_c123-rebank/profile/` — `build-c123-profile`, `BOTH_CPU 1`,
  `NDS_TICK_HUD_DRAW 0`, `regions=1601`, per-`(region, pc)`.
- `../2026-08-12_c130-fire-gate/c130-gate-rows.csv` — the 1,600-sample
  `BOTH_CPU 1` whole-match gate run, lane level, current code. **This supplies
  the mask.**

`ticks/frame = cycles / (2 x regions)` (`../2026-08-13_c-residue/RESIDUE.md` §0).

**Alignment: profile region `i+1` <-> gate row `i`.** Not assumed — measured, by
correlating profile non-idle against the gate `SHDT` column at nine offsets
(`band-split.txt`):

| offset | −3 | −2 | −1 | 0 | **+1** | +2 | +3 |
|---|---:|---:|---:|---:|---:|---:|---:|
| r | .030 | .064 | .030 | .075 | **.239** | .043 | .019 |

The chosen offset beats its best neighbour 3.2x. It is still only r = .239
across two builds, and **that attenuates every premium below**: a mask that is
partly wrong dilutes the difference it measures. **All profile-side premiums
here are lower bounds.** The gate arm's own `WORK-H` band premium is 322,591
tk/frame against the profile's 158,866 — a factor 2.03, which is the dilution.

```
python scripts/analyze-profile-region-split.py \
  artifacts/performance/2026-08-12_c123-rebank/profile/arm9-profile.csv \
  --census artifacts/performance/2026-08-12_c123-rebank/profile/census.json \
  --gate-csv artifacts/performance/2026-08-12_c130-fire-gate/c130-gate-rows.csv \
  --gate-lane SHDT --gate-min 30000 --control-lane SHDT --control-max 10000 \
  --check-align --check-controls --dis <objdump -d of the profiled ELF> \
  --attribute-leaves "__aeabi_fadd,__aeabi_fmul,...,sqrtf,memcpy,memset" \
  --symbols "<the chain>"                      # -> band-split.txt
  ... --gate-lane SPRM --gate-min 30000        # -> sprm-split.txt
```

---

## 1. The band, on the gate arm

| | band (`SHDT` >= 30,000) | control (`SHDT` < 10,000) |
|---|---:|---:|
| frames | **88, in 38 runs** | 1,497 |
| `SHDT` | 181,740 | 4,478 (**x40.6**) |
| `SPRM` | 51,900 | 1,964 (**x26.4**) |
| `WORK-H` | 1,279,156 | 956,566 (**+322,591**) |

The rank-80 frame — the frame that *is* P95 at `WORK-H` 1,220,480 — carries
`SHDT` 189,312 and `SPRM` 122,752. **43 of the 88 band frames are inside the
top 80**, and the band's ranks run 3 .. 530.

---

## 2. What the band frames were executing — and the controls that make it a finding

Profile split on the gate's `SHDT` mask, ticks/frame, band vs control. **Call
counts are exact** (a function's entry PC executes once per call), and a call
RATIO is what names a mechanism; the cycle premium only sizes it.

| symbol | calls/fr band | control | ratio | +self | +leaf | +total |
|---|---:|---:|---:|---:|---:|---:|
| `func_ovl2_800ED490` (affine 3x4 multiply) | 14.50 | 1.42 | **10.2x** | 4,132 | 12,432 | **16,564** |
| `gmCollisionGetWorldPosition` | 26.49 | 2.17 | **12.2x** | 2,176 | 6,778 | **8,954** |
| `gmCollisionSetInvertMatrix` | 9.02 | 0.87 | **10.4x** | 2,995 | 5,531 | **8,526** |
| `gmCollisionTestRectangle` | 17.60 | 1.09 | **16.1x** | 2,770 | 4,530 | **7,300** |
| `func_ovl2_800EDE5C` (axis scales, 3x `sqrtf`) | 17.60 | 1.11 | **15.9x** | 862 | 5,636 | **6,498** |
| `gmCollisionTransformMatrixAll` | 16.17 | 2.14 | 7.6x | 2,878 | 3,368 | **6,246** |
| `lbCommonSin` + `lbCommonCos` | 64.69 | 22.06 | 2.9x | 2,744 | 3,662 | **6,406** |
| `gmCollisionGetFighterPartsWorldPosition` | 2.17 | 0.45 | 4.8x | 1,219 | 1,822 | **3,041** |
| `func_ovl2_800EDBA4` (parent-chain world walk) | 9.11 | 0.96 | 9.5x | 1,637 | 0 | **1,637** |
| `gmCollisionCheckFighterAttackDamageCollide` | 16.28 | 0.37 | **44.0x** | 1,336 | 0 | **1,336** |
| `gmCollisionCheckFighterInFighterRange` | 1.80 | 0.10 | 18.0x | 349 | 118 | **467** |
| weapon-side collide + range | 1.98 | 1.02 | 1.9x | 231 | 0 | **231** |
| **chain total** | | | | **23,329** | **43,902** | **67,230** |

**42.3% of the band's non-idle premium (+158,866 tk/frame).** Without the leaf
half the chain reads 23,329 and looks like a minor row: **65% of it is soft
float that a per-PC profiler charges to `__aeabi_fmul`.** `__aeabi_fmul` alone
runs 3,231 calls/frame on band frames against 1,595 on control, and
`__aeabi_fadd` 2,836 against 1,635.

### The controls (`band-split.txt`, `--check-controls`)

| mask | chain premium | non-idle premium | chain share |
|---|---:|---:|---:|
| **`SHDT` >= 30,000** | **+23,299 self** | +159,078 | **14.6%** |
| top 88 by gate `WORK-H` | +8,201 | +200,691 | 4.1% |
| top 88 by profile non-idle | +24,716 | +452,591 | 5.5% |
| top 88 by gate `SINT` | +455 | +160,914 | 0.3% |
| **random 88 (negative)** | **−1,642** | −31,742 | — |

The negative control's largest row is 489 tk/frame and no collision symbol
appears in it. The **cost**-ranked mask reproduces c123's split-top80 exactly —
`armWaitForIrq`, `memcpy`, `armCopyMem32` x16.0, `get_fat` x17.5,
`ndsRelocNormalizeFighterAObj16File` x9.1 — **with no collision symbol in its
top eight.** The two masks overlap at 21 of 88 (chance 4.8). So
`RESIDUE.md`'s exclusion of the asset-load band as *this* band's owner is
confirmed on a second, stronger test, and the thing it was hiding is now named.

### One candidate refuted despite having the right shape

`ndsAObjEvent32NormalizeScript` fires 342 times a match on **127 frames in 103
runs**, max **428,178 cycles** on one frame — the "~38 events of ~180,000
ticks" silhouette almost exactly. It correlates with the gate `SHDT` series at
**r = 0.031**. It is not the owner. Shape is not attribution.

---

## 3. The mechanism, from the source

`decomp/BattleShip-main/decomp/src/gm/gmcollision.c`. Per engaged frame, for the
~9 hurtbox joints of a victim:

1. `gmCollisionCheckFighterAttackDamageCollide` (`:1379`) runs once per
   (live attack collision x hurtbox) — a full cross product, **16.28/frame**,
   with no per-hurtbox broad phase.
2. It calls `func_ovl2_800EDE00` (`:455`), which on the joint's first touch this
   frame runs `func_ovl2_800EDBA4` (`:332`) — walk to the nearest ancestor whose
   world matrix is valid, then `func_ovl2_800ED490` back down, 36 `fmul` +
   27 `fadd` per level — and then `gmCollisionSetInvertMatrix` (`:228`), a 3x3
   cofactor inverse plus a reciprocal, 61 float ops.
3. `func_ovl2_800EDE5C` (`:472`) takes three `sqrtf` for the joint's axis scales.
4. `gmCollisionTestRectangle` (`:661`) then transforms the attacker's `pos_curr`
   and `pos_prev` into that joint's local frame (2 x 9 `fmul` + 9 `fadd`), pays
   three `fdiv` for `radius / scale`, and clips.

**Every stage is already latched.** `FTParts` carries four dirty flags —
`transform_update_mode` (local matrix), `unk_dobjtrans_0x5` (world matrix),
`unk_dobjtrans_0x7` (inverse), `unk_dobjtrans_0x6` (axis scales) — cleared once
per fighter per frame (`ftmain.c:1847`). Each joint's matrix, inverse and scale
is computed **at most once per frame** no matter how many hitboxes test it.
`func_ovl2_800EDBA4`'s ancestor walk stops at the first already-valid parent, so
the 14.50 matrix multiplies per band frame are the union of the paths, not the
sum. **There is no duplicated computation to memoise away.** This closes the
51.7% "in-bracket and unattributed" residue from
`../2026-08-13_shdt-broadphase/REFUTED_PAIR_REJECT.md` §4: it was never a
missing early-out, it is the transform chain the surviving 5.31% of pairs drive.

---

## 4. Sizing — and why the number is not 16,000

P95 is the 80th largest of 1,600; baseline `WORK-H` 1,220,480, gate 1,120,380.
Subtract D from the 88 band frames and re-take rank 80
(`band-cut-rank-arithmetic.txt`):

| D removed from the band | ΔP95 |
|---:|---:|
| 8,000 | −4,032 |
| 16,000 | −7,104 |
| 32,000 | −9,024 |
| 40,000 | −12,928 |
| **47,424** | **−16,000** |
| 64,000 | −19,392 |
| the whole 322,591 premium | −78,016 (**saturation**) |

Two facts set the bar. A band-only cut **saturates at 78,016** — below that the
1,512 non-band frames set P95 (their own 80th largest is 1,142,464) — and it
**returns less than 1:1 until the band is deeply cut**, because only 43 of the
88 band frames are inside the top 80 to begin with.

**So: 47,424 ticks/frame off the band buys the 16,000.** Against the chain's
gate-arm size — 42.3% of the gate's 322,591 premium, i.e. **≈136,456 gate
ticks/frame** — that is a **35% reduction of the fighter collision transform
chain**. It is a real, reachable-looking ratio, and every route to it is blocked
today:

| route | status |
|---|---|
| fewer hurtboxes tested (per-hurtbox broad phase) | **refuted** — slice 47's reach bound never rejects (`ReachTests 2,373 WouldSkip 0`); `gmCollisionCheckFighterInFighterRange` has already put the attacker inside the arm's-reach AABB and the hurtboxes fill it |
| memoise / hoist / reuse | **nothing to reuse** — four dirty flags already give exactly one computation per joint per frame (§3) |
| cheaper arithmetic (fixed point, cheaper inverse, cheaper `1/scale`) | **`BLOCKED`** — float in `gmcollision`/`mp*`/`ftMain*`/`ftComputer` is frozen to exact memo/hoist/reuse/deletion. The three `radius / scale` divides in `gmCollisionTestRectangle` are 79% of the whole `__aeabi_fdiv` band premium (52.8 divides/frame x 117.9 cyc) and are worth only 3,112 tk/frame — 6.6% of the bar |
| reuse the renderer's joint matrices | the renderer builds every joint's world matrix each frame in 20.12 fixed point; substituting it changes collision results, so it is a gameplay-fidelity call, not an agent's |
| round-robin / phase-spread (rung 2) | **not applicable** — hit detection must run every simulation tick |

---

## 5. `SPRM`'s half of the co-fire is a DIFFERENT owner

Splitting the same profile on the gate's `SPRM` column instead
(`sprm-split.txt`, 20 frames over 30,000) gives a completely different table:
`f_read` x20.9 (leaf work **+11,284** tk/frame), `_read_r` x20.0, `f_lseek`
x12.5, `get_fat` x10.5, `armCopyMem32` x10.3, `mutexLock`/`mutexUnlock`/
`threadRemoveWaiter` x17.8, `ndsRelocNormalizeFighterAObj16File` x4.5.

**`SPRM`'s excursion is the synchronous animation-file load** that a landed
hit's status change triggers inside `ftMainProcParams`
(`ftCommonDamageGotoDamageStatus` -> `ftMainSetStatus`). `battleship_ftMainProcParams`'
own self cycles are flat on the profile — mean 3,372 cycles, max 6,008, a 1.8x
range, not 26x — so none of it is that function's own code.

That is the whole co-fire: **one engagement drives two different costs in two
different brackets.** The narrow phase runs in `SHDT`; the status change it
causes loads a file in `SPRM`. The asset-load family is on **22 of the 88**
`SHDT` band frames and worth +12,521 tk/frame there — a minority of that band,
and the majority of `SPRM`'s.

`SPRM` remains closed by arithmetic (`RESIDUE.md` §2: 13,056 deleted entirely),
so this is attribution, not a lever.

---

## 6. What this cycle did NOT do

- **No instrumented build.** The brief's Phase 2 was a lab build with per-frame
  counters inside the bracket. It was not spent, because the profile already on
  disk answers the same question at higher resolution (every symbol, not the
  2–3 a counter set would have named) and with no boot-headroom cost. **Boot
  headroom price of this cycle: zero bytes** — no `.text`, no `.bss`, no
  `check-boot-headroom.ps1` run needed.
- Did not attribute the remaining **57.7%** of the `SHDT` band premium. Named
  fragments inside it: the asset-load family +12,521 (22/88 frames), the
  animation lane (`ndsR2FtAnimParseDObjFigatree` +3,002, `ndsR2AnimValueQ`
  +2,098, `gcPlayDObjAnimJoint` +1,758), `ndsAudioFgmPlayAtPan` +1,558 (x7.6 —
  the hit SFX), `mutexLock`/`mutexUnlock` +3,052. `armWaitForIrq` is +61,178
  on band frames: they present later, so they also wait longer.
- Did not re-measure on current code. The mask is the current gate arm; the
  per-symbol composition is c123, which predates `NDS_R2_FOX_GUN_OVERLAY` and
  the flame fix (+9,856 `WORK-H` P95 combined, inside that arm's repeat spread).
  Neither touches `gmcollision`.
- Did not test whether a **fixed-point** narrow phase is mechanically
  equivalent. That is the only route left with the measured size, and it is
  `BLOCKED(decision: owner)` under the float freeze.
