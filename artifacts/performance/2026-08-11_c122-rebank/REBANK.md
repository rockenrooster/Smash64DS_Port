# c122 — re-bank after the slice 43 withdrawal, and where the tail actually is

Cycle 122. `build-c122-gate`, git `e03ae311204+dirty`, `NDS_R2_BOTH_CPU=1`,
1600 samples from frame 438 (window 440..2038), `-RingDump`, DLDI **ON**,
melonDS sha `DE80E46BDCF1FD98`, `slips=0`, `VBI 2:1642 3:345 4:33 5+:19`.

## The bank

Measured, not derived from the 1,210,560 slice-44 bank — that bank was taken
with `NDS_R2_FIGHTER_GX_COMPOSE=1` and this configuration no longer exists.

| arm | `WORK-H` P50 | `WORK-H` P95 |
|---|---:|---:|
| slice-44 bank (GX compose ON) | 931,648 | 1,210,560 |
| **c122 bank (GX compose OFF)** | **936,512** | **1,225,280** |
| delta | +4,864 | **+14,720** |

Slice 43 claimed −10,624 / −13,632, so the withdrawal gives back what it
claimed at P95 and rather less at P50. **Gap to the 1,120,380 target: 104,900.**

Sanity from the profile pair: c122 runs **+25.9M instructions** against c121 but
**−7.8M cycles** (1,069,903,483 / 3,939,255,650 vs 1,044,023,124 /
3,947,099,258). The CPU joint compose is more instructions and fewer cycles than
the GX path it replaced — the FIFO traffic slice 43 added was not free.

## The tail moved off the renderer

Ranking all 1,600 gate frames by `WORK-H` and comparing the **P95-setting band**
(ranks 60–100, i.e. the frames that actually decide the percentile — not the
handful of 5M outliers) against a baseline band (ranks 400–1200):

| bucket | baseline | P95 band | delta | % | presence |
|---|---:|---:|---:|---:|---|
| `SRC` (≈ `GCRA`, `gcRunAll`) | 328,800 | 590,848 | **+262,048** | 90.7% | 36/41 |
| ↳ `SHDT` hit detect | 4,544 | 89,728 | **+85,184** | 29.5% | 27/41 |
| ↳ `SINT` interrupt | 153,760 | 231,616 | **+77,856** | 27.0% | 21/41 |
| ↳ `SPHD` physics | 59,488 | 67,520 | +8,032 | 2.8% | 16/41 |
| ↳ `SCPU` CPU AI | 44,704 | 37,504 | −7,200 | −2.5% | 10/41 |
| `FTR` fighters | 299,328 | 298,432 | **−896** | −0.3% | **0/41** |
| `STG` stage | 168,704 | 169,152 | **+448** | 0.2% | **0/41** |
| `WAIT` / `OTHR` | — | — | +262,9xx | — | excluded: idle/residual |

`WAIT` tracks frame cost because a 3-VBlank frame waits longer; it is an effect,
not a cause, and `OTHR` is the accounting remainder. Both are excluded from
ownership per the goal, as is `HUD`.

**`FTR` and `STG` are flat.** That retires the post-slice-44 plan — the fighter
LOCAL matrix build (~24,314 tk/frame, 80/80) is real size but separates the two
populations by −896 on 0/41 frames. It is a P50 lane. `SHDT` is the opposite
shape and the one worth having: **4,544 → 89,728, a 20x step**, which is the
slice-44 pattern (work that is nearly absent normally and clusters on the tail),
and the pattern that moves P95 far more than P50.

## The instrument was profiling itself — read this before the next attribution

The first attribution (`build-c122-profile`, `attribution-top80.txt`) is **void
for candidate ranking**. `--split-top-frames 80` marks the 80 costliest frames by
NON-IDLE cycles, but the gate's P95 is `WORK-H`, which subtracts `HUD`. The
tick-HUD's on-screen block re-sorts eleven 128-entry rings and pushes thirteen
printf lines through the libnds console about twice a second, ~345,024 tk each
time — `nds_platform.c:68` says in as many words that it lands "on exactly the
frames the P95 gate is decided on". So the selection picked the HUD's own
refresh frames:

| symbol | +cyc/frame | % premium | presence |
|---|---:|---:|---|
| `ndsPlatformRenderDebugHud` | 223,043 | **17.5%** | 80/80 |
| `armWaitForIrq` (idle) | 109,458 | 8.6% | 80/80 |
| `_svfiprintf_r`/`_vfiprintf_r`/`__ssvfiscanf_r`/`consolePrintChar` | ~119,000 | ~9% | 63–79/80 |
| `get_fat`/`f_lseek`/`f_read`/`_FAT_read_r`/reloc | ~90,000 | ~7% | 40–48/80 |

Its marked set overlaps the gate's real top-80 `WORK-H` frames by only **~20%**.
`NDS_TICK_HUD_DRAW=0` exists for exactly this and **neither
`sample-tick-hud-buckets.ps1` nor `run-task37-profile-census.ps1` sets it**, so
it has to be passed by hand: `-MakeFlags NDS_R2_BOTH_CPU=1,NDS_TICK_HUD_DRAW=0`.
The GATE arm keeps `DRAW=1` — every bank back to slice 44 was measured that way,
and changing it would break comparability with the number being beaten.

`--attribute-leaves` additionally needs `--objdump`; without it the run prints
the split table and then exits 1 on `[WinError 2]`, which reads like the whole
analysis failed when only the leaf pass did.

## Size the lanes BEFORE grinding one — no single lane reaches the target

P95 is a position in a sorted list, so a lane that is huge on the frames it
touches still only moves P95 as far as the frames underneath it. Capping each
bucket at its own baseline median on every frame (a perfect fix, therefore an
upper bound) and re-taking the 80th of 1,600:

| counterfactual | P95 | vs 1,120,380 |
|---|---:|---:|
| current | 1,226,816 | +106,436 |
| remove **100%** of `SHDT` excess | 1,188,096 | **+67,716** |
| remove 75% | 1,192,224 | +71,844 |
| remove 50% | 1,205,440 | +85,060 |
| remove **`SHDT` + `SINT`** excess | **1,069,312** | **−51,068** |

`WORK-H` at ranks 80/100/124/150/200 = 1,226,816 / 1,205,632 / 1,169,728 /
1,143,168 / 1,109,888.

**Hit detection alone cannot clear the gate even if it were free** — ceiling
−38,720 — because only 44 of the 80 tail frames are `SHDT`-high and rank 124 is
already 1,169,728. The two lanes together over-achieve by 51,068, so the target
needs a bundle across both, which is the same conclusion the board reached for
the animation lane ("no single remaining cut clears the floor... they ship as
one bundle").

## Where hit detection's cost actually is

`--split-by-symbol gmCollisionTestRectangle` (124 marked frames vs 1,476
control, premium 820,138/frame) — the population the gate's `SHDT` bucket
selects, which the top-80 split does NOT:

| symbol | +cyc/frame | %prem | presence |
|---|---:|---:|---|
| `armWaitForIrq` (idle, excluded) | 249,928 | 30.5% | 124/124 |
| `__aeabi_fadd` | 86,169 | 10.5% | 124/124 |
| `__aeabi_fmul` | 84,711 | 10.3% | 124/124 |
| `func_ovl2_800ED490` (4×4 affine multiply) | 16,752 | 2.0% | 124/124 |
| `sqrtf` | 12,752 | 1.6% | 124/124 |
| `gmCollisionSetInvertMatrix` | 12,730 | 1.6% | 124/124 |
| `__aeabi_fdiv` | 12,338 | 1.5% | 124/124 |
| `gmCollisionTransformMatrixAll` | 10,802 | 1.3% | 124/124 |
| `gmCollisionTestRectangle` | 10,320 | 1.3% | 124/124 |
| `gmCollisionGetWorldPosition` | 7,931 | 1.0% | 124/124 |

**Soft float is 183,218 cyc/frame — 22.3% of the premium — and the pair test
itself is 1.3%.** `func_ovl2_800ED490` is a 4×4 affine multiply: 36 `fmul` + 24
`fadd` ≈ 1,780 cycles of soft float per call at the board's 25.17/36.43 prices.
The cost is **building each hurtbox joint's world transform**, not testing it.

**The chain walk is already optimal — do not "fix" it.** `func_ovl2_800EDBA4`
walks up only until it meets a joint with `unk_dobjtrans_0x5` set, then walks
back down doing one `parent_world × local` per joint, so shared ancestor
prefixes cost once per frame; `transform_update_mode` separately memoises the
trig local build, and `func_ovl2_800EDE00`/`DE5C` memoise the inverse and
`vec_scale` behind their own dirty flags. The port clears the whole word once a
frame in `ndsFTParamsInvalidateFighterParts`, already flattened at cycle 106.
The only lever left in this lane is **touching fewer joints**: the broad phase
`gmCollisionCheckFighterInFighterRange` gates per ATTACK collision against the
victim's root and `attr->hit_detect_range`, so once it passes, all 11
`damage_colls` transform their joints. A conservative per-hurtbox reach bound
would be an exact call deletion; it must be a true upper bound or it deletes
real hits.

## Files

- `gate.json`, `gate-rows.csv` — the bank and its per-frame rows.
- `profile/` — the `DRAW=1` profile. Section E is instrument-selected; sections
  0/A/B/C/D (whole-match self time, tier stalls, ITCM rent) are still valid.
- `profile-nodraw/` — the corrected attribution.
- `attribution-top80.txt` — the void split, kept so it is not re-run by mistake.
