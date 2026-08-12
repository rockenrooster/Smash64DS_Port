# Post-slice-44 re-attribution — the stage lane is halved, the fighter LOCAL build is now #1

Cycle 121. `build-c121-profile` (`NDS_R2_BOTH_CPU=1`,
`NDS_R2_STAGE_VALIDATE_STRIDE=8`), whole-match census window 438 + 1600,
per-frame regions, `timestamp_discontinuities=0`. Every leaf reconciles at
100.0%.

## Slice 44 confirmed by the census

| symbol | c120 | c121 | Δ |
|---|---:|---:|---:|
| `StageWorldSourceKeyMatches` calls/frame | 54.0 | **9.9** | −44.1 |
| `StageWorldSourceKeyMatches` tk/frame | 7,719 | **1,640** | −6,079 |
| `BuildPersistentStageWorldMatrix` cyc/call | 1,806 | **652** | −64% |
| `BuildPersistentStageWorldMatrix` tk/frame | 9,017 | **5,329** | −3,688 |
| total cycles | 3,981,830,319 | **3,947,099,258** | −34,731,061 |
| total instructions | 1,074,794,277 | **1,044,023,124** | −30,771,153 |

−34.7M cycles is −21,707 cyc/frame = **−10,838 tk on the mean**, against −17,088
measured at P50 and −35,904 at P95 — the tail paid more than the mean, which is
what a cut on work that clusters on heavy frames looks like
(`mean-self-time-predicts-p50-not-p95`).

`ndsRendererAdapterPrepareNativeStageOwner` now owns **14,801 tk/frame** across
its two rows (was 18,912): `MtxMul20p12` 9,590 and
`BuildPersistentStageWorldMatrix` 5,211, both `tk prem` **0**, both 80/80. The
`MtxMul20p12` half is the 16 dynamic bindings' `world × view_projection`, which
the stride cannot touch — the camera moves every frame.

## The ranking now (tk on a tail frame, callers present on ≥40 of 80)

| tk/frame | tk prem | frames | caller | lane |
|---:|---:|---|---|---|
| **11,874** | **530** | 80/80 | `BuildDObjXObjMatrix` ← `BuildDObjLocalMatrix` | **fighter local build** |
| 9,590 | 0 | 80/80 | `MtxMul20p12` ← `PrepareNativeStageOwner` | stage compose |
| **7,187** | **288** | 80/80 | `memcpy` ← **`BuildDObjXObjMatrix`** | **fighter local build** |
| 5,253 | 198 | 80/80 | `BuildDObjLocalMatrix` ← `DLAllDrawForSlot` | fighter local build |
| 5,211 | 0 | 80/80 | `BuildPersistentStageWorldMatrix` ← `PrepareNativeStageOwner` | stage |
| 4,358 | 4,046 | 80/80 | `__udivsi3` ← `ndsPlatformRenderDebugHud` | **instrument, excluded** |
| 2,332 | 0 | 80/80 | `fmul` ← `syMatrixLookAtReflectF` | legacy float camera |
| 2,246 | 678 | 80/80 | `fadd` ← `ndsR2FtAnimParseDObjFigatree` | animation, parked |
| 2,050 + 1,954 | | 80/80 | `fmul`/`fadd` ← `ndsBaseGcPlayMObjMatAnim` | material anim |
| 1,883 | 66 | 80/80 | `memcpy` ← `DLAllDrawForSlot` | fighter |
| 1,811 | 0 | 80/80 | `sqrtf` ← `syMatrixLookAtReflectF` | legacy float camera |

**The fighter local-matrix build is now the largest legal lane at ~24,314
tk/frame** — `BuildDObjXObjMatrix` 11,874 + its own `memcpy` 7,187 +
`BuildDObjLocalMatrix` 5,253 — on 56.1 calls a frame, present on 80/80 frames,
`tk prem` 530/288/198. Nearly flat, so a deletion moves P50 and P95 together.

## The shape of that lane, from the source

`ndsRendererAdapterBuildDObjXObjMatrix` (`reloc_backend_renderer_dl.c:1526`)
builds an **N64 float `Mtx`** through the `syMatrix*` family and then converts
it with `ndsRendererAdapterMtxFromN64(&mtx, out)` at line 1786. 52.8 of the 56.1
calls a frame carry a `memcpy` charged directly to this function at 272.6
cyc/call — one 64-byte matrix copy per call, before 12 useful floats are written
over it. 423.7 cyc/call total, so the copy is **64% of the builder**.

## …and `--pc-detail` says the lane is memory-bound, not arithmetic-bound

Run on the same profile, no build spent
(`pcdetail-dobjxobj.txt`). 35,366,808 cycles over **324 distinct PCs**, 85,992
entries = 53.7 calls/frame:

| pc | cycles | %fn | insns | cyc/insn | instruction |
|---|---:|---:|---:|---:|---|
| 0x02042c22 | 2,355,110 | 6.7 | 85,480 | **27.55** | `ldr r2, [r3, #0]` |
| 0x02042c2c | 1,746,695 | 4.9 | 64,126 | **27.24** | `ldr r5, [r4, #64]` |
| 0x020426bc | 1,251,903 | 3.5 | 85,994 | 14.56 | `pop {r4-r7}` |
| 0x02042c1a | 988,777 | 2.8 | 85,480 | 11.57 | `ldr r3, [r4, r3]` |
| 0x02042c2a | 962,844 | 2.7 | 64,125 | **15.02** | `ldr r3, [r4, #28]` |
| 0x02042d46 | 879,569 | 2.5 | 256,248 | 3.43 | `ldr r1, [r2, r5]` |

The top PC is **6.7%** of the function and the profile has no peak after it —
there is no instruction to delete. The expensive rows are all `ldr` at 11–28
cyc/insn on `[r4, #28/#32/#36/#64]`, i.e. **DObj fields**: 53.7 cold DObj
touches a frame against a 4 KB D-cache. The one hot loop (0x02042d46–0x02042d7e,
`lsrs r3, r1, #23` = float exponent extraction, so `MtxFromN64` inlined) runs at
2.0–3.4 cyc/insn and is not the problem.

**So this lane is the same shape as the one slice 44 just cut: memory-bound on
scattered object fields, not arithmetic-bound.** Extending
`ndsRendererAdapterBuildFighterTraRotRpyDirect20p12` to more kinds would delete
the float intermediate and the conversion, but it would still read the same
DObj fields, so it cannot recover the 11–28 cyc/insn rows that are 60%+ of the
cost. Price that before building it.

**The lever the shape actually points at — a local-matrix memo — is a
DO-NOT-RETRY.** The Task 91 comment at `reloc_backend_renderer_dl.c:1790` argues
for exactly it ("a memo on that needs no numerical equivalence argument
whatsoever") and it has been built and killed **twice**. The comment should not
be read as an open invitation.

## memcpy overall is bigger than any one caller

`memcpy` self-time is 54,194,697 cycles = **16,912 tk/frame on the mean**, and
table F prices it at **49,671 tk on a tail frame** across 364.8 calls. Only
three callers clear the 40/80 presence bar (7,187 + 1,883 + 1,838 = 10,908); the
rest is spread thin. Do not plan a slice against 49,671 — plan it against the
callers that are actually named.
