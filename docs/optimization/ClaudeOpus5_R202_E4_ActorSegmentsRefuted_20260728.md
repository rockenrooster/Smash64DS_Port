# R2-02 E4 — the actor segments are not a mask edit, and now we know why

**Date:** 2026-07-28
**Phase:** R2-02 (`Smash64DS_Runtime2_SwitchPlan.md` §7)
**Verdict:** **REVERT.** Three arms, three refutations. One real defect found and
kept. `NDS_R2_STAGE_ACTORS` is deleted; R2-02's gate stands unmet at `STG` P50
**224,320** against 180,000.

---

## 1. What this was for

R2-02 E3 claimed −51,200 ticks/frame of stage time by admitting Whispy's eyes
and mouth and both flower beds to the Task 36 replay, and was retracted the same
day because it destroys both flower beds. E4 set out to fix it or drop it. It
drops it, but it establishes the mechanism, which is the part worth keeping.

**Control for every arm:** `build-r2-02-e3-off`,
`NDS_R2_STAGE_DIRECT=1 NDS_R2_STAGE_DMA=1`, ROM `7F7A185DE4B67ED8`. 128-frame
ring dump, frames 439–566.

| bucket | control P50 | control P95 |
|---|---:|---:|
| `STG` | 224,320 | 232,640 |
| `WORK` | 1,205,184 | 1,666,048 |

VBlank intervals 2:12 3:540 4:9 5+:5, max 18, 566 frames.

## 2. Two hypotheses refuted before any build

**Task 51's missing shift compensation — refuted on the host.**
`ndsRendererNativeStageTask51EnsureWorld` emits its baked world matrix with no
`coordinate_shift` compensation, while Task 36's `BuildWorld` left-shifts the
rotation rows by it. That would draw a shifted triangle at 1/2^shift scale — a
perfect fit for "collapsed to a smear". Reading the shifts straight out of
`nds_native_stage_owner.generated.inc` kills it: every triangle in segments 1, 2,
3 and 6 carries **shift 0**.

```text
seg name           runs  tris bindings  rigid  per-triangle shift
  0 layer0           26    54 0..19       yes  shift0=31  shift1=23
  1 whispy_eyes       1     4 20..20       NO  shift0=4
  2 whispy_mouth      4     8 21..24       NO  shift0=8
  3 flowers_back      4     6 25..28       NO  shift0=6
  4 layer1            6    76 29..29       NO  shift0=66  shift1=10
  5 layer2            3    17 30..32      yes  shift0=15  shift1=2
  6 flowers_front     6     9 33..38       NO  shift0=9
  7 layer3            4    28 39..41      yes  shift0=20  shift1=8
```

The same table shows the rigid set is exactly segments 0, 5 and 7 — and that the
old replay mask `0xa1` is exactly those three. That equality is the invariant E3
broke.

**Task 51 itself — refuted by the build config.** `NDS_TASK51_STAGE_NATIVE ?= 0`
in the Makefile, no target overrides it, and the E3 and E4 config headers both
read `#define NDS_TASK51_STAGE_NATIVE 0`. `Task51EnsureWorld` is compiled out of
every ROM this campaign has measured. The E3 retraction's leading hypothesis
named a function that never ran, and E3's premise — "Task 51 already gave those
bindings a baked constant world matrix, so nothing about their streams is
per-frame" — was false for the same reason.

## 3. Arm A — the unmatched pop. Real defect, not the cause

`ndsRendererTask36ReplayRun` ended with `task36_local_pushed = TRUE`
unconditionally. `ndsRendererNativeStageTask36EndSegment` pops on that flag. A
rigid run's captured stream does contain `PUSH` + `MULT4x4`, so the pop is owed;
an actor run's stream contains a `LOAD4x4` per triangle and never pushes. E3
therefore bought **four unmatched `glPopMatrix(1)` calls a frame**, one per
admitted actor segment, each underflowing the modelview stack.

Fixed at the seam that knows: the capture now keeps a signed `PUSH`/`POP`
balance per run, faults a run whose balance is outside {0, 1}, and stores the
answer in `NDSRendererTask36ReplayRun::local_pushed`; replay reports that instead
of asserting `TRUE`. Verified over GDB at frame 500 — rigid runs 0, 41, 50 read
`local_pushed=1` with `world_mult_count` 1, 2, 1; actor runs 26, 27, 31, 32, 44
read `local_pushed=0` with `world_mult_count=0`.

**This is kept.** It is a correctness defect in the shipped replay path
independent of any mask. It did not change the picture: the smear survived it
unchanged.

## 4. The cause

Read the captured stream instead of reasoning about it. An actor run's 63 words
decompose exactly as the generic emit writes them: the identity projection and
modelview pair from `ndsRendererLoadHardwareMatrices(NULL, FALSE)` (32 words),
then per triangle a `MATRIX_LOAD4x4` (16 words) and three vertices (12 words).

That `LOAD4x4` is `ndsRendererNativeStageLoadNoZMatrix`, and what it loads is
`binding_composed[binding]` — **projection × view × model, composed on the CPU
for that frame's camera.** Capture bakes one frame's camera into the stream and
replay reissues it forever. The flowers do not move with the camera because they
have been nailed to the camera the capture frame happened to have.

A rigid binding is replayable for the opposite reason: its stream is `PUSH` +
`MULT4x4` of a **constant world** matrix, sitting under the camera that
`ndsRendererNativeStageTask36BeginSegment` loads live at the top of every
segment, every frame.

> **`NDS_TASK36_REPLAY_SEGMENT_MASK` must name exactly the segments whose every
> binding is in `NDS_RENDERER_TASK36_RIGID_BINDING_MASK`.** Not a subset, not a
> superset. Both masks now carry that sentence.

This also explains the shape of the artifact, which no matrix-stack story did:
the smear sits in the same screen band under wildly different camera framings.
Camera-independent screen position is the signature of a baked camera.

## 5. Arm B — widen the rigid mask to everything but layer1. REVERT

`0x000003ffdfffffff` (bindings 0–28, 30–41; layer1's binding 29 left out).
Renders correctly. Costs more than double.

| bucket | control P50 | arm B P50 | Δ |
|---|---:|---:|---:|
| `STG` | 224,320 | **465,088** | **+240,768** |
| `WORK` | 1,205,184 | 1,445,376 | +240,192 |

VBlank intervals went bimodal — 2:109 3:342 4:110 — at an unchanged mean of
3.02, so the pacing did not improve either.

Doubling with a correct picture is the signature of the optimization disabling
itself. `ndsRendererAdapterValidateTask36StageWorld` re-checks a source key per
rigid binding every frame and drops the **whole** mask to 0 on one mismatch;
`ndsRendererTask36ReplayFinishFrame` then invalidates the replay because the
frame's rigid mask no longer equals the compile-time constant. Everything falls
back to the generic per-triangle compose: correct, and twice the price. Whispy is
animated — `ndsRendererAdapterPrepareNativeStageMaterials` already lists bindings
20 and 22 as per-frame material owners.

## 6. Arm C — the flowers alone. REVERT, and the informative one

Rigid mask `0x000003ffde0fffff` (adds only 25–28 and 33–38), replay segment mask
`{0, 3, 5, 6, 7}` — the two masks kept equal, per §4.

The guard accepts it. Read at frame 500,
`sNdsRendererAdapterNativeStageWorkspace.task36_runtime_rigid_mask` is
`0x3ffde0fffff`, identical to the compile-time constant, and
`captured_segment_mask` is `233` = `0xE9`. The cut is engaged.

| bucket | control P50 | arm C P50 | Δ | control P95 | arm C P95 | Δ |
|---|---:|---:|---:|---:|---:|---:|
| `STG` | 224,320 | **173,120** | **−51,200** | 232,640 | 179,520 | −53,120 |
| `WORK` | 1,205,184 | 1,151,744 | −53,440 | 1,666,048 | 1,620,736 | −45,312 |
| `FTR` | 547,776 | 547,392 | −384 | 996,736 | 995,456 | −1,280 |
| `SRC` | 324,416 | 324,608 | +192 | 664,576 | 664,960 | +384 |

VBlank intervals 2:**233** 3:319 4:11 5+:3 against the control's 2:12 3:540 4:9
5+:5. `STG` P50 173,120 is **under R2-02's 180,000 gate**, every other bucket is
flat, and Boundary passes.

**Both flower beds are gone.** The frame-locked crop of `previous.png` against
`latest.png` shows bare grass where the back bed and the front row belong. The
automated numbers saw it this time because the comparison was made: `stage_body`
green 44.86% → 50.49%, detail 52.40% → 48.77% — 418 pixels of blossom turned into
grass. Required-region detail moved only 62.750% → 62.694%, which is again no
help, for the same reason as E3.

The mechanism is in `ndsRendererNativeStageEmitNoZTriangle`, and it is **not**
the `Task36EnsureWorld` failure this section first named — that check never
runs, because the one before it fires first. The rigid branch drops a triangle
outright when any corner is bound to a different binding than the run:

```c
        if (sNdsNativeStageVertices[dense_index].matrix_binding !=
            run->binding_index)
        {
            gNdsRendererM3PostArmFailureCount++;
            return 0u;
        }
```

**The rigid fast path is single-binding by construction, and the flower beds are
the only cross-matrix geometry in Dream Land.** Counted straight out of the
generated packet, and matching the `cross_matrix_runs=5
cross_matrix_triangles=10 cross_matrix_foreign_corners=15` the Boundary stage
check has been printing all along:

```text
segment          cross-matrix triangles   foreign corners
layer0                    0 of 54                0
whispy_eyes               0 of  4                0
whispy_mouth              0 of  8                0
flowers_back              4 of  6                6
layer1                    0 of 76                0
layer2                    0 of 17                0
flowers_front             6 of  9                9
layer3                    0 of 28                0
```

So arm C dropped 10 of the flowers' 15 triangles at that check, on the capture
frame, and every replay frame reproduced the gap. −51,200 is the price of not
drawing them, which is the same thing E3 was really measuring. Two arms, two
different mask edits, the same 51,200.

**This is also why the flowers are expensive in the first place.** A
cross-matrix triangle falls through to the generic tail, which calls
`ndsRendererNativeStageLoadNoZMatrix` **once per vertex** — a full CPU compose
and a 16-word `glLoadMatrix4x4` for each of the three corners:

```c
        if (one_binding == FALSE)
        {
            ndsRendererNativeStageLoadNoZMatrix(
                dense->matrix_binding, vertex_shift, projected_z);
        }
```

15 flower triangles cost **35 matrix loads a frame** (10 × 3 + 5 × 1). Whispy's
12 triangles, all single-binding, cost 12. Per triangle the flowers are 2.3× the
matrix traffic of anything else on the stage, and that ratio — not the vertex
count — is what the 45,349-ticks-for-27-triangles number was measuring.

## 7. What is kept, what is deleted

Kept:

- the capture-side push balance and `local_pushed` (§3), a real defect fix;
- E3's `MATRIX_MULT4x3` opcode and the capture fault on any unencoded **matrix**
  class, which is the hardening that stops a future stream being silently baked
  wrong;
- the `NDS_R2_STAGE_ACTORS_PROOF` falsifier, re-documented as covering vertex
  data only.

Deleted: `NDS_R2_STAGE_ACTORS` and both of its mask branches. A default-off flag
that is known to delete scenery is worse than no flag.

Both published ROMs were rebuilt at defaults and re-verified: Boundary green,
required-region detail **62.750%**, `stage_body` green 44.848% / detail 52.242% —
the control's numbers.

## 8. What the next attempt has to do

Not a mask edit, and **not a world-matrix bake either** — that was this report's
first answer and §6 corrects it. The flower bindings' worlds are already fine:
the runtime rigid-constancy check accepted them in arm C and
`task36_runtime_rigid_mask` held the widened value all run. What disqualifies
them is the *topology*: 10 of their 15 triangles have corners in more than one
binding, and no single-binding fast path can draw such a triangle.

**De-cross the flower triangles in the generator.** For each foreign corner,
emit a duplicate dense vertex whose position is pre-transformed into the run's
binding space:

```text
v' = W_run^-1 · W_foreign · v
```

Both worlds are constant, so this is a compile-time transform. It costs at most
**15 new dense vertices** (312 → 327), adds no runs and no triangles, and leaves
every corner of every flower triangle bound to its own run. Then, and only then:

1. Widen `NDS_RENDERER_TASK36_RIGID_BINDING_MASK` to 25–28 and 33–38, and widen
   `NDS_TASK36_REPLAY_SEGMENT_MASK` to `{0, 3, 5, 6, 7}` in the same commit.
2. Gate the transform on the Task 49 Tier-2 differ (≤ 1.0 screen-px) over the
   de-crossed vertices' screen positions against the CPU-composed oracle. The
   inverse-multiply is where fixed-point error enters and it is the only thing
   that can go wrong quietly.
3. Verify with a frame-locked crop of segments 3 and 6 against the control arm,
   plus `task36_runtime_rigid_mask` and a triangle count read from the same run
   that produced the buckets. A cut that drops geometry reads as a saving.

**Expected size.** The flowers pay 35 of the stage's matrix loads a frame; rigid
plus replay takes that to roughly two world mults. Arm C measured −51,200 for
removing them outright, and most of that is the matrix traffic rather than the
vertex emission, so a correct version should recover the bulk of it — enough on
its own for R2-02's remaining 44,320, though that is a prediction, not a
measurement.

Whispy (20–24) is out of scope: it is materially animated, and at 12
single-binding triangles it was never the expensive half. layer1 (29) is a
separate lever — its runs submit through the raw composed matrix and it is the
largest single generic segment at 22,738 ticks/frame for 76 triangles.

## 9. Cost of the lesson

Three ROM builds, three 128-frame ring dumps, two Boundary runs and four
frame-locked crops to establish that a 15-triangle scenery cut needs generator
work. The two host-side refutations in §2 cost ten minutes and no builds, and
both should have been done before E3 landed — the shift census reads a checked-in
generated file, and the Task 51 refutation is one `grep` of the Makefile.

The cross-matrix census in §6 is the sharpest instance: it is nine lines over the
same checked-in file, it names the exact 10 triangles, and **the answer was
already on screen.** `M3_NATIVE_STAGE_CHECK_OK` prints `cross_matrix_runs=5
cross_matrix_triangles=10 cross_matrix_foreign_corners=15` on every single
Boundary run, and has for the whole campaign. Three arms were spent rediscovering
a number the verifier had been reporting all along. When a subsystem check prints
a counter, read it before designing against the subsystem.
