# SRC's top candidate, sized before building: NO-GO, and three of its premises are not in the profile

`docs/optimization/SRC.md` (owner, 2026-09-16) proposes as its first candidate a
**bound fixed-point pose/transform domain shared by gameplay and rendering**,
removing "fixed-to-float publication, later conversions, repeated hierarchy
discovery, and separate transform preparation". SRC is the largest bucket in the
four-fighter frame (P50 **555,584** against WORK-H 1,588,928 and a gate of
1,120,000), and the board has never attacked it, so it deserved a real sizing.

The document says of itself that it is source-grounded and unmeasured. This is
the measurement, taken against the existing per-PC profile. **No ROM was built.**

## Instrument, validated first

Exact `bl`/`blx`-site call graph over
`artifacts/performance/2026-09-16_p2-2p8-n0409-profile/arm9-profile.csv` joined
to the linked ELF's symbol ranges; ticks = cycles / 258 (regions = 129).
Reproduced before any new number was trusted:

| check | expected | measured |
|---|---|---|
| `guMtxCatF` calls | 4.961/fr | **4.961** |
| `__aeabi_fmul` bl-site capture | 409,912 of 409,916 | **409,912** |
| `__aeabi_fadd` + `fmul` | 90,169 tk/fr | **90,169** |
| collision family self | 8,367 tk/fr | **8,367** |
| `InvalidateSubtree` | 20,348 tk/fr, CPI 5.60 | **20,349 / 5.60** |
| whole-frame work | 1,616,382 | **1,616,422** |

## Three of the candidate's premises are not in the profile

1. **"Fixed-to-float publication" is 2,814 tk/fr, not a cost centre.**
   `ndsR2FixedToF32`/`ndsR2F32ToFixed` are not even functions — they are
   `static inline` integer bit kernels (`include/nds/nds_anim_fixed.h:114,277`),
   already hand-rolled for exactly this reason. **The pose engine issues zero
   float operations**: all four `bl __aeabi_fmul` sites inside `ndsFtPosePlay`
   execute 0 times. The three pose bodies do total 74,630 tk/fr, which is
   SRC.md's "about 74.6K" — but that is evaluation cost, not conversion cost.

2. **"Later conversions" on the render side are zero.** The entire fighter
   matrix path is already Q20.12 with no soft-float calls:
   `MtxMulAffine20p12` 25,410, `MtxMul20p12` 17,595, `TraRotRpyDirect20p12`
   12,469, `SourceWorldMulLocal` 9,722, `StoreSplitModelview` 12,040 — all
   0.0 float calls/frame. The 11,178 tk/fr `MtxCellS16p16` is a split-precision
   `Mtx` **unpack** (`src/nds/nds_renderer_dl_core.c:117`), not a conversion.

3. **"Separate transform preparation" is the collision family — and it is
   already built, engaged and measured at zero.** The only float on the
   transform path is 8,367 self + ~17,000 leaf = **25,022 tk/fr**, which is
   `NDS_R2_COLLISION_FIXED`: wired, engaged, **+64 WORK-H P50**
   (`…/2026-08-15_cfx-ring-wiring/RING.md`), issue −1,717 against icache_fill
   +1,854 (`…/2026-08-15_cfx-ring-split/SPLIT.md`). Rebuilding it inside a bound
   domain re-runs a measured failure.

Total deletable conversion on both seams: **13,992 tk/fr** (217.6 pose
publications at 25.9 cyc + 904.9 render unpacks at 24.7 cyc). That is the
ceiling of "delete the conversions", and it is 2.8% of the gap.

`ndsF32AddBits` (15,592 tk/fr) is **not** available: it is the IEEE-exact clock
wired under the owner's same-behaviour instruction and bit-proven over 1.12
billion operations (`src/nds/nds_ft_pose.c:138-155`). Moving it to fixed point
changes frame timing.

## Reach

| group | total tk/fr |
|---|---:|
| A pose/anim evaluation (already Q) | 105,470 |
| B material-anim programs | 15,086 |
| C validity / hierarchy discovery | 21,651 |
| D collision transform reconstruction | 29,325 *(built, measured +64)* |
| E motion events / col-anim | 6,171 |
| **candidate-reachable** | **177,703 = 32% of SRC** |
| untouched (map queries, AI, physics, interrupt, hit detect, scheduler) | 112,028 |
| unreached residual (GObj indirect dispatch) | 234,325 |

Net reachable and not already spent: **148,378 = 27% of SRC**.

## conv/op does not bind — and does not save it either

| seam | ops deleted/fr | conv/fr | conv/op |
|---|---:|---:|---|
| pose → render | **0** (both halves already Q) | 217.6 removed | n/a |
| pose → collision | 1,250.2 | ~54 | **0.043** — passes by 13×, and measured **+64** |
| the document's own bridge step | 0 | 217.6 added and kept | **∞** |

Campaign 12's exchange rate was already cleared by 13× on the one seam that has
float arithmetic, and that lane still did not pay. conv/op is not the binding
constraint here; **fetch is**.

## It has the collision lane's shape

SRC.md requires bind-time tables for dense joint ids, parent ids, subtree
intervals, static local transforms, track output addresses, materials and
exception classes, plus generated typed arrays and executors.
`SPLIT.md` measured resident code at **0.4871 tk/fr per `.main` byte**, so
8–16 KB of new tables and executors is **+3,900 to +7,800 tk/fr** of icache
fill, and the marginal-to-whole ratio on rank-80 frames was 3.22×. `.itcm` has
**104 bytes free** of 32,632, so placement relief is unavailable.

Against that, the deletable arithmetic is at most **57,034** issue slots,
because the reachable work is **63% stall**:

| target | tk/fr | CPI | issue | stall |
|---|---:|---:|---:|---:|
| candidate-reachable SRC self | 154,824 | 2.71 | 57,034 | **97,790 (63%)** |
| `ndsFTParamsInvalidateSubtree` | 20,349 | 5.60 | 3,637 | 16,712 (82%) |
| `ndsFtPoseParse` | 20,392 | 3.37 | 6,055 | 14,338 (70%) |
| `ndsFtPosePlay` | 34,210 | 1.73 | 19,814 | 14,396 (42%) |

## Verdict

| | tk/fr | % of the 496,382 gap |
|---|---:|---:|
| floor | **-4,000** | 0.8% |
| mid | **-25,000** | 5.0% |
| ceiling | **-70,000** | **14.1%** |

**NO-GO as the top SRC candidate.** It cannot reach the gate alone — not within
7× at its ceiling — and SRC.md concedes the point itself at line 112: *"SRC
alone is not established as sufficient to close the entire deficit."*

The two halves worth keeping from it are reachable **without** a pose/transform
domain rewrite and **without** adding bind tables:

- the validity/hierarchy walk, **-10,000 to -15,000**
- the FTR-side copy/convert/traverse, **≤ -30,000** of the 53,122 tk/fr in
  `MtxCellS16p16` + `StoreSplitModelview` + `SourceWorldMulLocal` +
  `ComposeOwnerWorldsSource` + `BuildDObjLocalMatrix` + `PrepareInitialMatrices`
  (the 55,474 of Q matrix arithmetic survives any representation change)

## Correction to N05.03's own sizing — in the favourable direction

`…/2026-09-16_p2-2p8-n0409-profile/CANDIDATE_SELECTION.md` splits
`ndsFTParamsInvalidateSubtree` as "clear loops 56%, descendant flatten 21%".
**Per-PC, that is inverted.**

| PC | rate | tk/fr | CPI | instruction |
|---|---:|---:|---:|---|
| `0x01ffcc8c` | 184.4/fr | 3,876 | **42.04** | `ldr r3,[r6,#16]` |
| `0x01ffcc8e` | 184.4/fr | 2,832 | **30.71** | `ldr r6,[r6,#8]` |
| `0x01ffcd6e` | 102.6/fr | 2,492 | **48.59** | `ldr r0,[r3,#0]` |
| `0x01ffcce2` | 290.1/fr | 632 | 4.36 | `str r5,[r1,#4]` ← **the clear** |

The **walk is 59%** and the **clear is 14%**. The lever therefore deletes
pointer-chase dcache stalls rather than issue slots, which is the opposite of
what its recorded falsifier assumed ("only the issue slots go, about -6,700").
That falsifier is wrong in the favourable direction, and the band narrows to
**-10,000 to -15,000** from the earlier -3,636..-19,700.

It also makes N05.03 the only candidate in this area whose mechanism attacks
**stall** rather than issue — which matters, because every lane this campaign
has lost, lost to fetch.
