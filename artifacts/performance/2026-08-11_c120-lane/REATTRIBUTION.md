# Post-slice-43 re-attribution — ownership moved to the STAGE

Cycle 120. `build-c120-profile-gate` (`NDS_R2_BOTH_CPU=1`, census window 438
+1600, per-frame regions), `task37_census.py --split-top-frames 80
--attribute-leaves` over the arithmetic leaves AND the mid-level builders in one
scan. Every leaf reconciles at **100.0%**.

## Slice 43 confirmed by a fourth instrument

`ndsRendererMtxMulAffine20p12` fell from **54.2 calls/frame (19,175 tk)** to
**1.9 (1,105 tk)** — the 1.9 are the stage's, exactly as designed. Both c119
symbols the slice targeted are **absent from the c120 census entirely**:

| symbol | c119 cycles | c120 |
|---|---:|---|
| `ndsRendererMtxMulAffine20p12` | 57,347,919 | not present |
| `ndsRendererLoadHardwareSplitMatrices` | 34,517,582 | not present |
| `ndsRendererLoadHardwareGxComposedMatrices` | — | 46,162,474 |

`.text.hot.draw` fell **113.8M → 55.7M cycles**, which is the affine multiply
leaving that section. Total instructions fell 1,100,555,400 → **1,074,794,277**.
Net named: 91.9M removed, ~60.9M added back — the FIFO writes and the capture
cost **66% of what was deleted**, which is why the slice measured a third of its
prediction.

## The ranking now (tk on a tail frame, callers on ≥40 of 80)

| tk/frame | tk prem | frames | caller | lane |
|---:|---:|---|---|---|
| 12,149 | 667 | 80/80 | `BuildDObjXObjMatrix` ← `BuildDObjLocalMatrix` | fighter local build |
| **9,769** | **0** | 80/80 | `MtxMul20p12` ← **`PrepareNativeStageOwner`** | stage compose |
| **9,143** | **0** | 80/80 | `BuildPersistentStageWorldMatrix` ← **`PrepareNativeStageOwner`** | stage world validation |
| 5,177 | 232 | 80/80 | `BuildDObjLocalMatrix` ← `DLAllDrawForSlot` | fighter |
| 4,532 | **4,214** | 80/80 | `__udivsi3` ← `ndsPlatformRenderDebugHud` | **instrument, excluded** |
| 3,882 | 1,075 | 80/80 | `fadd` ← `ndsR2FtAnimParseDObjFigatree` | animation, parked |
| 2,925 + 2,067 | | 80/80 | `fadd`/`fmul` ← `ndsBaseGcPlayMObjMatAnim` | material anim |
| 2,456 | 349 | 80/80 | `fadd` ← `ndsStageMPAdjustFloorLoopWallSweep` | collision, FROZEN |

**`ndsRendererAdapterPrepareNativeStageOwner` now owns 18,912 tk/frame across two
rows, both `tk prem` 0 and both 80/80.** Perfectly flat: a deletion moves P50 and
P95 one for one. That is the largest legal candidate on the board.

## Correction: the "legacy float camera 55,865 tk" lane was over-counted

Attributing the actual camera symbols gives **~13,700 tk/frame**, not 55,865:
`syMatrixLookAtReflectF` 4,174 (2.0 calls each from `syMatrixLookAtReflect` and
`gmCameraLookAtFuncMatrix`) plus its own leaves `fmul` 2,325 / `fadd` 1,541 /
`sqrtf` 1,798, `syMatrixPerspFastF` `fdiv` 1,710, `syUtilsArcTan` `fdiv` 1,655,
`syMatrixF2L` `fmul` 1,279, `guMtxCatF` `fadd` 1,174, `syMatrixLookAtF` `fmul`
1,008. The c119 grouping swept in soft-float that belongs to other callers.
**Do not plan a slice against 55,865.**

## Premium is instrument and asset streaming — a different question, do not conflate

Premium (marked minus control) is 1,375,838/frame and its owners are
`ndsPlatformRenderDebugHud` **232,929 = 16.9%**, then printf/console
(`_svfiprintf_r` 38,527, `_vfiprintf_r` 34,299, `__udivmoddi4` 27,509,
`__ssvfiscanf_r` 26,448, `consolePrintChar` 25,264, `__utf8_mbtowc` 19,177) and
asset streaming (`get_fat` 25,327, `armCopyMem32` 25,077,
`ndsRelocNormalizeFighterAObj16File` 25,034, `ndsRelocAssetIDForToken` 23,553,
`f_lseek` 15,986). Most of that is the INSTRUMENT and is not in the published
ROM.

`+cyc/frame` answers "what does the tail ADD"; table F's `tk/frame` answers "what
does this cost ON a tail frame". P95 = P50 + premium, so a flat cut with
`tk prem` 0 lowers both equally — which is what the game+renderer lane offers.

## Data-quality note

`timestamp_discontinuities=1` (c119 had 0). One region of 1,601. Region 1558 is
in the marked set as before; the ranking is unchanged by excluding it, so no row
here depends on the discontinuity.
