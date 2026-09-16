# P2-2p8 gap sizing against the locked-30 gate

Measured 2026-09-16 on the clean, post-deletion canonical baseline
(`build-p2-fourcpu-tickhud`, 1,972 samples, four fighters, Dream Land).
This is a **sizing record, not a candidate**. It exists because the campaign has
been selecting leaf-level levers without a written statement of how far they can
possibly go, and the answer changes what should be worked on next.

## The gate

`1,120,000` ticks is two VBlank intervals. This run's `ALL` P50 is 1,678,016
across exactly three intervals, so one interval is **559,339 ticks** and two is
**1,118,678** — the 1,120,000 figure is the locked-30 frame budget, not an
arbitrary constant. A frame fits 30 FPS when its work fits two intervals.

## Where the four-fighter arm actually sits

| | ticks | vs gate |
|---|---|---|
| WORK-H P50 | 1,575,296 | **1.41x** |
| WORK-H P95 | 2,311,616 | **2.06x** |
| WORK-H max | 6,373,568 | 5.69x |

**1,821 of 1,972 frames — 92.3% — exceed the gate. 151 frames, 7.7%, fit.**

Closing it needs **-455,296** ticks from the median frame (28.9% of it) and
**-1,191,616** from P95 (51.6% of it).

## What the known levers are worth

Measured on this same build, per frame:

| lane | tk/fr | share of the P50 gap |
|---|---|---|
| Entire `__aeabi_fadd` + `__aeabi_fmul` class | 90,169 | 19.8% |
| …of which the collision matrix family | ~49,900 | 11.0% |
| …of which `func_ovl2_800ED490` alone | 16,940 | 3.7% |
| Pose subsystem (Play + Parse + Update) | ~63,800 | 14.0% |
| `memset` + `memcpy` | ~43,300 | 9.5% |
| Every N04.0x code lever so far, combined | ~20,000 | 4.4% |
| The 2026-09-16 clean rebuild (build hygiene) | 50,432 | 11.1% |

**Deleting every soft-float operation in the frame closes under a fifth of the
median gap and under a tenth of the P95 gap.** Campaign 12's whole measured
simulation reservoir, 87,085 tk/fr on the strictly-simulation subsystems, is the
same order. No combination of the lanes above reaches 455,296, let alone
1,191,616.

## What follows

This is not an argument to stop optimizing; the levers above are real and several
have banked. It is an argument that **P2-2p8 as currently specified cannot be
closed by the kind of work the campaign has been selecting**, and that continuing
to pick the next-largest leaf will keep producing 5,000-to-15,000-tick results
against a 455,296-tick requirement.

Three directions, and the choice is the owner's:

1. **Structural reduction in per-fighter work at four players.** The two-fighter
   shipping shell measures 26.4 FPS on the same build, close to target, while the
   four-fighter arm is 1.41x over at the median. The cost is not uniformly
   distributed overhead — it scales with fighter count. A lever that changes what
   each additional fighter costs (draw-call structure, per-fighter LOD, shared
   pose/packet work) is the only class sized to the gap.
2. **Re-scope the four-fighter target.** 1,120,000 is the locked-30 budget and
   the natural product target, but whether *four-player* SSB64 is required to
   hold 30 on DS hardware is a scope question, not a measurement. The source
   game's own four-player behaviour is the reference, and this repo has not
   recorded it. Worth establishing before spending more cycles against a number
   that may not be the right one.
3. **Accept P2-2p8 RED as a known state** and let P2-3/4/5/6/7 acceptance
   proceed, with four-player performance tracked rather than gating.

## Method

`artifacts/verification/p2-2-fourcpu-tickhud.csv`, the per-frame WORK-H column
from the run that produced the current checkpoint. Frame counts are direct, not
modelled. Lane figures come from
`artifacts/performance/2026-09-16_p2-2p8-n0409-profile/` with ticks computed as
`cycles / (2 * regions)`.

---

## Owner ruling 2026-09-16: 30 FPS at four players is required

Direction (2) — re-scoping the target — is off the table. The work is direction
(1), a structural lever sized to 455,296 tk/fr.

### Subsystem attribution of the frame

The top 60 symbols of the clean-payload profile, 301,305,045 cycles over 129
regions, grouped by what they belong to (`ticks = cycles / (2 * regions)`):

| class | tk/fr | share of top-60 |
|---|---|---|
| idle (`armWaitForIrq`) | 272,605 | 23.3% |
| **arithmetic kernels and math leaves** | **197,474** | 16.9% |
| **stage and stage-renderer** | **159,837** | 13.7% |
| fighter draw | 110,343 | 9.4% |
| pose and animation | 96,792 | 8.3% |
| `memset` + `memcpy` + `armCopyMem32` + `DynamicArray` | 50,598 | 4.3% |
| harness instrument (not shipped) | 36,660 | 3.1% |
| particles | 11,155 | 1.0% |
| unclassified within the top 60 | 232,385 | 19.9% |

This revises the earlier sizing in one important way. The **`__aeabi_fadd` +
`__aeabi_fmul` class alone is 90,169 tk/fr**, and that is what the leaf campaign
was chasing. The **whole arithmetic-kernel class is 197,474** — it also contains
the fixed-point matrix kernels (`ndsRendererMtxMul20p12`,
`ndsRendererMtxMulAffine20p12`), the integer divides and the roots. Those are not
waste: they are already DS-native and already optimized. **They shrink only by
performing fewer transforms, not cheaper ones.** Arithmetic kernels plus stage is
357,311 tk/fr, 78% of the gap — so the gap is reachable in principle, but only by
changing how much work is issued, never by making the existing kernels faster.

### The Task 103 stage-phase instrument is broken — do not spend a build on it

`src/port/reloc_backend_movement.c:13548` records that only 39% of the STG bucket
was ever attributed and that "the other 238,254 ticks/frame are outside
`ndsRendererCommitNativeStageSegment` entirely, and no task has ever profiled
them". `NDS_TASK103_STAGE_RUN_PHASE=1` is the instrument that would partition it
into Prepare / Traversal / Display / Finish.

It cost two builds and produced nothing:

1. It does not fit. ITCM is 104 bytes free and the four taps need **360 more**;
   the link fails with "region `itcm' overflowed by 360 bytes". The taps sit
   inside `NDS_R2_ITCM_PACK2_CODE` functions.
2. With room made (evicting `ndsBaseGcPlayMObjMatAnim`, 732 B, from ITCM for the
   lab build only), the ROM **crashes**: `TICKFAULT __excpt_entry pc=01fffd6c
   lr=020d974a`, in `ndsCameraRecordFrame` (`battleship_gmcamera.c:223`) with
   `half_w=0, half_h=3280.00806`, a corrupt backtrace and `sp=0x2fffd9e` — both
   unaligned and outside DTCM. The baseline build of the same source runs 1,972
   samples clean, so this is the census build, not the game.

The eviction was reverted. STG remains unattributed and the instrument needs
repair before it can answer anything.

---

## Owner ruling 2026-09-16 (second): no 30 Hz simulation

The -294,016 lever is withdrawn. The 60 Hz simulation stays.

That is consistent with the Sacrifice Order rather than in tension with it: audio
(1), visual (2) and gameplay (3) fidelity are all ranked as **more** expendable
than the 60 Hz simulation (4). Jumping to 4 skipped 1 through 3. The remaining
455,296 tk/fr must come from those, and the largest untouched block in category 2
is the stage.

### The stage lane, sized from the whole profile

The earlier figure came from the top-60 symbols only. Summing **all 1,234 profiled
symbols**, the stage-renderer family is **54 symbols / 222,758 tk/fr**:

| tk/fr | symbol |
|---|---|
| 27,835 | `ndsRendererCommitNativeStageSegment` |
| 18,199 | `ndsRendererNativeStageEmitNoZTriangle` |
| 16,447 | `ndsRendererNativeStageBeginRun` |
| 15,010 | `ndsRendererAdapterBuildPersistentStageWorldMatrix` |
| 14,155 | `ndsRendererNativeStageEmitNoZVertex` |
| 11,341 | `ndsStageGCDrawAllLoopRecordCapturedDisplay` |
| 11,291 | `ndsRendererNativeStageLoadNoZMatrix` |
| 9,223 | `ndsStageCollisionLoopGeometryReady` |
| 7,778 | `ndsStageMPSweepFloorLoopSweep` |
| 7,699 | `ndsRendererAdapterPrepareNativeStageOwner` |
| 6,904 | `ndsRendererNativeStageTask36EnsureWorld` |
| 6,876 | `ndsRendererAdapterCommitNativeStageDisplay` |

**Raw geometry emission alone — segment commit, triangle emit, vertex emit,
matrix load, run begin — is 87,927 tk/fr, re-issued every frame for geometry that
does not move.** A further 15,010 rebuilds a matrix named "persistent". MP
collision is a separate 46 symbols / 49,297 tk/fr.

The important property: **caching static stage emission is not a fidelity
sacrifice at all.** It produces identical pixels. Unlike every other lane on the
table it costs nothing from the Sacrifice Order, so it should be exhausted first
on those grounds alone.

### Task 103 is confirmed broken, and it was not my eviction

The instrument was retried with a different ITCM relief: `NDS_R2_ANIM_Q_ITCM_ON=0`
evicts `ndsR2AnimValueQItcm` (1,076 B), one of the **21 ITCM residents that never
execute** in this window (5,050 B idle in total — itself worth recording). The
build links, and the ROM crashes with the **same signature**: `excpt_entry`, in
`ndsCameraRecordFrame`, `battleship_gmcamera.c:223`.

Two independent ITCM evictions, one hot (`ndsBaseGcPlayMObjMatAnim`) and one cold,
produce the identical crash. The eviction is not the cause — **the Task 103 taps
are**. The instrument has never been run (no `artifacts/performance/*task103*`
exists) and should be treated as unproven code, not as a tool. Both evictions are
reverted; the tree is unchanged.

STG's composition above was therefore obtained from the profile symbol table
rather than from the instrument, which is cheaper and needed no build.
