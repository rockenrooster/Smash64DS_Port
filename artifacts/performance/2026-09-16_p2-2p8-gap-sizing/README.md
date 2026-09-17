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

### The tail has no target, and that is the finding

Excluding every class named above, **989 symbols hold 732,167 tk/fr** and the
largest single one is 20,349. The top of that tail:

| tk/fr | symbol |
|---|---|
| 20,349 | `ndsFTParamsInvalidateSubtree` |
| 16,442 | `ndsRendererAdapterBuildDObjXObjMatrix` |
| 12,463 | `battleship_ftMainProcUpdateInterrupt` |
| 11,937 | `ndsDamageSlashTextureFill` |
| 11,687 | `ndsFighterDisplayContractSubmit` |
| 11,103 | `ndsBaseGcRunAll` |
| 10,438 | `gcCaptureCameraGObj` |
| 10,039 | `ndsRelocGetFileData` |
| 9,884 | `ndsRendererAdapterGetFrameCameraMatrices` |
| 9,722 | `ndsRendererAdapterSourceWorldMulLocal` |
| 9,212 | `ndsRendererAdapterApplyMvpRecalc` |

There is no symbol in this frame worth 455,296, or 100,000, or even 30,000. The
gap is not hiding in one place.

**The pattern is the per-DObj matrix pipeline.** `BuildDObjXObjMatrix` 16,442 +
`SourceWorldMulLocal` 9,722 + `ApplyMvpRecalc` 9,212 +
`LoadHardwareMatrixPair` 8,984 + `BuildDObjLocalMatrix` 7,754 +
`MtxLoadN64ToDS20p12` 7,599 + `GetFrameCameraMatrices` 9,884 +
`MaterialAnimHash` 7,866 is **~77,500 tk/fr spread across eight symbols**, and it
is driven by object *count*, not by op cost. `gNdsGCDrawsActiveMax` is **203**
(`taskman_seam_battle_host.c:777`), so the frame runs that pipeline over roughly
two hundred DObjs.

That reframes the arithmetic-kernel class once more: it is not "float is slow",
it is "two hundred objects are transformed every frame". Any lever that reduces
the object count cuts across the kernels, the pipeline symbols and the emit path
simultaneously — which is the only shape that reaches a 455,296 requirement.

### Culling is absent on the CPU side

`renderer_adapter_stage.c:4906` records that "hardware_triangle_count is a
POST-CULL count", i.e. the DS hardware clips and discards geometry **after the
CPU has already paid its transform and submission**. A CPU-side visibility
rejection before the matrix pipeline would remove the whole per-object cost
rather than the rasterizer cost. Unsized, and Dream Land is small enough that
much of it is on screen most of the time, so this needs measurement before it is
believed — but no CPU cull exists to measure today.

### Two negatives worth recording

- **The stage is not committed twice per present.**
  `ndsRendererAdapterPrepareNativeStageOwner` runs 0.99 calls/frame and
  `ndsRendererCommitNativeStageSegment` 7.94, a ratio of 8.02 — eight segments
  once per present, not four segments in two passes. There is no doubled stage
  submission to reclaim.
- **The pose lane is already half-rate.** `reloc_backend_compat_shims.c:3166`
  records that body joints already run "at 30 Hz under `NDS_FT_POSE_HOLD`", and
  the pose clock advances per tick with only Play held. The obvious "pose runs
  twice, half is discarded" saving was taken long ago.

---

## Owner ruling (third): 30 FPS at four players is required AND no 30 Hz simulation

Both of the two largest levers found so far are therefore unavailable or dead:
the 30 Hz simulation is ruled out, and the stage lane is GX-throughput-bound with
a realized WORK-H conversion measured at zero three times (Tasks 53/54/55 —
Task 53 removed 187,648 ticks of stage CPU prep and `ALL` moved **-128**, because
FIFO backpressure is an inline stall that lands in OTHR, inside WORK-H).

Levers checked and too small, each measured on this build:

| lane | tk/fr | share of the 455,296 gap |
|---|---|---|
| material animation (visual-only, could run per present) | 35,206 total, ~11,000 recoverable | 2.4% |
| stage cache/replay ceiling | ~80,000 | 17.6%, realized ~0 |
| N-squared collision across 4 fighters | <30,000 | 6.6% |
| per-fighter LOD | 0 | already engaged at 3+ fighters |

### The structural candidate: the DS matrix stack is not being used

`ndsRendererLoadHardwareMatrixPair` (`nds_renderer_textures_effects.c:12148`)
issues, per object:

```c
ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
glLoadMatrix4x4(ndsRendererMtx20p12AsM4x4(projection));
ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
glLoadMatrix4x4(ndsRendererMtx20p12AsM4x4(modelview));
```

That `modelview` is a **CPU-computed product**. The DS has a hardware matrix
stack that can do the hierarchy multiply itself — load the camera once, then
`PUSH` / `MULT4x4(local)` / draw / `POP` per object — and **this codebase already
does exactly that on the stage's rigid bindings**: "a rigid binding's captured
stream is PUSH + MULT4x4 of a constant world under the camera the segment
bracket loads live each frame" (`nds_renderer_assets.c:6720`).

GX word traffic is unchanged — `MULT4x4` and `LOAD4x4` are both 16 words — so
this does **not** hit the stage lane's throughput wall. The entire saving is CPU:
the matrix product the GPU would compute instead.

Sized on this build, the CPU matrix-composition pipeline is **14 symbols /
160,576 tk/fr = 35.3% of the gap**:

| tk/fr | symbol |
|---|---|
| 25,410 | `ndsRendererMtxMulAffine20p12` |
| 17,595 | `ndsRendererMtxMul20p12` |
| 16,442 | `ndsRendererAdapterBuildDObjXObjMatrix` |
| 15,010 | `ndsRendererAdapterBuildPersistentStageWorldMatrix` |
| 12,469 | `ndsRendererAdapterBuildFighterTraRotRpyDirect20p12` |
| 11,178 | `ndsRendererMtxCellS16p16` |
| 9,884 | `ndsRendererAdapterGetFrameCameraMatrices` |
| 9,722 | `ndsRendererAdapterSourceWorldMulLocal` |
| 9,212 | `ndsRendererAdapterApplyMvpRecalc` |
| 8,984 | `ndsRendererLoadHardwareMatrixPair` |
| 7,754 | `ndsRendererAdapterBuildDObjLocalMatrix` |
| 7,599 | `ndsRendererMtxLoadN64ToDS20p12` |

Not all of it converts — the camera matrices are built once, and some products
feed collision rather than the GX. But this is the first lane found whose *shape*
matches the requirement: it is driven by object count (`gNdsGCDrawsActiveMax` =
203), it cuts across the arithmetic kernels and the adapter pipeline at once, and
it is not throughput-bound.

**Fidelity note, stated up front:** hardware `MULT4x4` rounds in 20.12 at each
stage where the CPU currently rounds its own product. Output is equivalent, not
bit-identical, so this is a render-fidelity question under the existing doctrine
rather than a free cache — and it needs the Task 49 GX differ on the affected
owners, exactly as the stage replays did.

**Falsifier:** if the per-object local transforms are not expressible as a single
`MULT4x4` under a frame-constant camera — i.e. if `ApplyMvpRecalc` is folding
something per-object that the stack cannot express — the lane collapses to the
camera load alone and is worth ~10,000. The `MvpRecalc` kind-48 path is where to
check that first.

---

## The arithmetic, closed (2026-09-16)

Every category has now been measured. Non-idle work is **1,616,382 tk/fr** and
the gate needs **-496,382 = 30.7% of everything executed**. For scale: the
**top twenty symbols of the profile together are 502,955 tk/fr (31.1%)**, and the
largest single symbol in the frame is 45,692 (2.8%). Closing the gap is
arithmetically equivalent to deleting the entire top twenty.

Everything not killed, with exact sizes:

| lane | tk/fr | note |
|---|---|---|
| invalidation over-clear | **12,000-18,000** | unmeasured, the best remaining candidate |
| `guMtxCatF` | <=13,485 | producers are float; a Q concat pays ~32 conv/call |
| collision matrix family | 25,022 total, **~0 recoverable** | built, engaged, measured +64 P50 |
| material animation | ~11,000 | visual-only half |
| camera matrix copies | ~6,000-8,000 | needs a caller-wide signature change |
| audio / BGM | ~2,000 | averaged over 104 refills in 1,600 frames |
| **total** | **~90,000** | **0.20x the requirement** |

**No combination of what remains reaches 455,296.**

### The smallest sets that would

1. **Per-fighter work down 2.48x**, 187,008 -> 75,424, with the non-fighter floor
   untouched. Nothing on the board has that shape. It means a DS-specific reduced
   fighter skeleton or mesh — a `PROJECT_GOAL.md` fidelity decision under
   Sacrifice Order 2, not an optimization.
2. **Cut the 827,136 non-fighter floor.** 42.6% of it is renderer/draw and the
   stage half is GX-throughput-bound. Note that **46,273 tk/fr of the measured
   floor is the tick-HUD instrument itself** — `ndsPlatformRenderDebugHud`
   20,077, `tickGetCount` 16,583, `ndsIFCommonRecordHUDState` 9,613 — which is
   free in the shipping ROM. That is not a saving to bank, but it does mean the
   gate is being measured on a configuration ~46,000 ticks heavier than the one
   that ships, and re-measuring on the shipping config is owed regardless.
3. **Reopen the 30 Hz simulation** (-294,016, owner-withdrawn). Even taken, it is
   0.65x the requirement and still needs ~160,000 more from (1) or (2).

The honest position: **30 FPS at four fighters is not reachable by optimization
alone from here.** It needs a fidelity decision — reduced per-fighter geometry,
or the 30 Hz simulation, or both — and those are the owner's calls, not
engineering ones. Everything engineering can still contribute is the ~90,000
above, and the invalidation lane is the only part of it above 12,000.
