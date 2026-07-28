# R2-02 E7 — the per-binding compose was rebuilding operands, not multiplying

**Date:** 2026-07-28
**Phase:** R2-02 (`Smash64DS_Runtime2_SwitchPlan.md` §7)
**Verdict:** **KEEP, graduated to default-on.** `STG` P50 **224,320 → 212,480**
(−11,840), P95 **232,640 → 219,072** (−13,568). Bit-identical output, so no
fidelity budget is spent. R2-02's gate stands unmet at **212,480** against
180,000 — **32,480 over**.

---

## 1. What this was for

E1a and E2 took `STG` from 351,488 to 224,320. The largest remaining item in the
stage preflight was `prepare matrices` at **54,901 ticks/frame, 44.6%** of it,
composing sixteen dynamic stage bindings once each.

E6 had already established that the world matrices behind those bindings are
warm: memoising `ndsRendererAdapterBuildPersistentStageWorldMatrix` saved only
~84 ticks per binding, and its self-healing refresh cost a P95 cliff of +117,312
on the 84 miss frames. E6 was reverted whole. What it left behind was a negative
result worth acting on — **the chain walk is not the cost**, so the cost is
whatever else the per-binding path does.

**Control:** `build-r2-02-e3-off`, `NDS_R2_STAGE_DIRECT=1 NDS_R2_STAGE_DMA=1`,
ROM `7F7A185DE4B67ED8`.
**Candidate:** `build-r2-02-e7-on`, the same plus `NDS_R2_STAGE_VIEWPROJ=1`, ROM
`DFBE1ED0E2BB97DB`. 128-frame ring dump, frames 439–566 on both.

## 2. The change

`ndsRendererAdapterPrepareNativeStageMatrices`, Task 44 steady-state branch:
compute `camera_modelview × projection` once, then give each dynamic binding one
multiply against it, instead of routing every binding through
`ndsRendererAdapterPrepareNativeStageBindingMatrix`.

A binding whose DObj is NULL, that carries an mvp-recalc RPY 0x47 (which rewrites
the pair *after* composition), or whose persistent world build fails, keeps the
exact original path.

## 3. Result

| bucket | control P50 | E7 P50 | Δ P50 | control P95 | E7 P95 | Δ P95 |
|---|---:|---:|---:|---:|---:|---:|
| `STG` | 224,320 | 212,480 | **−11,840** | 232,640 | 219,072 | **−13,568** |
| `ALL` | 1,680,000 | 1,680,000 | 0 | 1,680,640 | 1,680,512 | −128 |
| `WORK` | 1,205,184 | 1,201,728 | −3,456 | 1,666,048 | 1,669,824 | +3,776 |
| `FTR` | 547,776 | 554,304 | +6,528 | 996,736 | 998,400 | +1,664 |

`STG` is decisive at both percentiles and well clear of the 5,000–7,000
build-placement floor. Every other bucket moves inside it. `ALL` is flat because
it is VBlank-quantized — the frame stays pinned at 3 intervals — which is the
expected shape for a cut of this size and is not evidence against it.

VBlank intervals on the candidate: 2:13 3:538 4:10 5+:5, max 18, 566 frames.

## 4. Why it is exact, and why the first explanation was wrong

The change was designed as an associativity hoist, and the flag comment
originally said so: `(A × B) × C` and `A × (B × C)` do not round identically at
20.12, so it would gate on the Task 49 Tier-2 screen-pixel budget rather than on
bit equality.

**That was wrong on both counts, and measuring it is what showed it.** Dumping
`sNdsRendererAdapterNativeStageWorkspace.binding_composed` out of both running
ROMs at frame 500 found all 42 bindings bit-identical — including the 15 that a
host model of the two orderings says should differ by up to 1,775 LSB. A result
that clean is not rounding luck, so the model was wrong, not the ROM.

It is wrong because there was never a second multiply to reassociate. For the
battle camera, `ndsRendererAdapterBuildCameraMatrices` takes the
`GM_CAMERA_MTX_KIND` branch, which returns

```text
projection      = MtxMul(lookat, persp)
modelview_valid = FALSE
```

so `ndsRendererAdapterPrepareInitialMatrices` falls to `MTXCOPY(modelview,
&dobj_world)` and the compose was already `world × (lookat × persp)` — **one
multiply per binding, not two.** Meanwhile
`ndsRendererAdapterBuildTask36StageCameraMatrices` derives its two halves from
the same `syMatrixLookAtReflect` and `syMatrixPerspFast` calls on the same
`CObj`, so `camera_modelview × projection` reproduces that product bit-for-bit.

E7 therefore does not reassociate anything. It stops re-deriving a
binding-independent product sixteen times.

**The saving is memory traffic, not arithmetic.** Per binding the old path ran
the `sNdsRendererAdapterCameraCache` lookup and three 64-byte `MTXCOPY`s — and
`MTXCOPY` on `NDSRendererMatrix20p12` compiles to `bl memcpy`, which is the
Task 86 note in `include/nds/nds_renderer.h` — to rebuild operands that are
identical for all sixteen. −11,840 over 16 bindings is ~740 ticks each, which is
the shape of a cache lookup plus three 64-byte copies, not of a 4×4 multiply.

This is Task 104's rule applying again from the other side: **size a memory lever
by bytes that stop being touched.** Here bytes stopped being touched and the
predicted arithmetic saving never existed.

## 5. The exactness evidence

Bit-identity was then confirmed across the camera's range of motion, not just at
one frame. Both ROMs, same GDB dump of all 42 composed matrices, the world
matrices, and the camera pair:

| frame | camera moved vs. first | max abs delta: composed / world / projection / camera_modelview |
|---:|---|---|
| 260 | — | 0 / 0 / 0 / 0 |
| 420 | yes | 0 / 0 / 0 / 0 |
| 500 | yes | 0 / 0 / 0 / 0 |
| 700 | yes | 0 / 0 / 0 / 0 |
| 1100 | yes | 0 / 0 / 0 / 0 |
| 1700 | yes | 0 / 0 / 0 / 0 |

`binding_composed[]` is the only thing this function produces, so identical
output over six frames spanning the match is a stronger statement than any
screenshot crop: there is no visual delta to budget, because there is no delta.

Twenty-six of the 42 bindings are all-zero in both arms — the rigid ones never
receive a composed matrix, because Task 36 composes them on the GPU with
`PUSH` + `MULT4x4`. Only the 16 dynamic bindings ({20…29, 33…38}) carry values,
and E7 touches exactly those.

## 6. Graduation

`NDS_R2_STAGE_DIRECT`, `NDS_R2_STAGE_DMA` and `NDS_R2_STAGE_VIEWPROJ` are now
default-on in both the published `smash64ds-battle-playable-hwtri` block and the
`tickhud`/`proof` block. Together they are `STG` P50 **351,488 → 212,480**
(−139,008, −40%).

All three are exactness-preserving, so none of them spends the `PROJECT_GOAL.md`
fidelity budget and none needs the owner's visual-oracle call — that clause
governs approximations. E1a reuses a table that is a pure function of inputs
Task 44 already proves unchanged; E2 sends the identical word stream by DMA
instead of a store loop; E7 is bit-identical as above.

Two details worth keeping:

- The tick-HUD block sets the three **without `override`**. They are the live
  A/B surface for the rest of R2-02, and `override` beats the command line,
  which would leave the measurement target unable to measure the thing it
  exists to measure. The published block keeps `override`, as it should.
- The graduated default tick-HUD build hashes `DFBE1ED0E2BB97DB` — byte-identical
  to the explicit `NDS_R2_STAGE_*=1` lab build. The measurement and the shipping
  configuration are the same binary, which is the standing requirement for that
  block.

## 7. Verifier fallout, and the fix

Boundary failed the first attempt on a static checker, not a runtime gate:

```text
M3_STAGE_FALSIFIER: ndsRendererAdapterPrepareNativeStageMatrices:
unclassified reads ['workspace.binding_composed']
```

Correct, and a good catch by the checker. E7 moved a write of `binding_composed`
into a closure that previously only reached it through
`ndsRendererAdapterPrepareNativeStageBindingMatrix`. The fix is the real
classification, `FIELD_CLASS_CAMERA` — the same class the field already carries
in that sibling closure and in `root_frame_fields` — not a suppression, followed
by regenerating `NDS_NATIVE_STAGE_CONSUMED_FIELDS.generated.json`. The generated
include's SHA is unchanged at `eda2dbd6…`; only the manifest moved.

## 8. What is left, and what this says about aiming the rest

`STG` 212,480, gate 180,000, **32,480 over**.

The item E7 attacked is now ~43,000 and the arithmetic in it is small, so the
next cut is not another multiply. Two observations for whoever takes the next
arm:

1. **`display commit` is now the majority of the bucket** — 129,947 of ~242,000
   on the pre-E7 census, and R2-02 has never attacked it. Every arm so far
   (E1a, E2, E3, E4, E5, E6, E7) worked the preflight. Re-census before assuming
   the target is still there.
2. **Do not size a matrix lever by counting multiplies again.** Both E6 and E7
   were designed against arithmetic and both were resolved by memory traffic —
   E6 found the walk already warm, E7 found the multiply already singular. The
   stage preflight's remaining cost is operand movement.

## 9. Cost of the lesson

One build and one 128-frame sample to get the number; two GDB dump runs per arm
to find out that the stated rationale was wrong. The rationale was wrong in the
safe direction — it predicted a fidelity cost that does not exist — but it was
still wrong in the tree, and a wrong reason in a comment outlives the commit that
introduced it. Correcting it cost less than half an hour and it is why §4 exists.

The generalisable half is in `TASK_STANDING_RULES.md`: **when a change is
supposed to alter fixed-point rounding, dump the fixed-point result and diff it,
rather than reasoning about the ordering or reaching for a screenshot.** It is
cheaper than a frame-locked crop, it is exact, and here it turned a
budget-spending KEEP into a bit-identical one.
