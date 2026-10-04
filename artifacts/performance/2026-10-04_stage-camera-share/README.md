# Stage camera from the frame camera cache, 2026-10-04

`ndsRendererAdapterPrepareNativeStageMatrices` built the battle camera's split
look-at and perspective with `ndsRendererAdapterBuildTask36StageCameraMatrices`
after the frame camera producer (`ndsRendererAdapterBuildCameraMatrices`, the
0x4C path) had already built the same two matrices from the same CObj for the
particle pass. The stage prepare now asks for the frame camera first and copies
the cache entry's `particle.projection` / `particle.modelview` when the entry
belongs to this frame; otherwise it builds them as before. Same-ROM A/B word
`gNdsStageCameraShare` (default 1).

## Equivalence (lab four-CPU ROM, gate configuration)

The stage FIFO-word, span-state and segment-close hashes from
`../2026-10-04_stage-segment-fast` (`stghash.ps1`, `hashcmp.py`) are identical
on 901 of 901 logic frames (600..2400) with the share on, and the closing hash
matches the segment fast path runs.

## Gate (official target, same ROM `build-gate-1004c`, ring dump, 1,960 samples)

| | WORK P50 | WORK P95 | > 1.12M | STG P50 | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `gNdsStageCameraShare=0` | 948,416 | 1,312,896 | 356 | 154,240 | 1,594 / 1,961 |
| `gNdsStageCameraShare=1` | **947,072** | 1,314,048 | **355** | **152,896** | **1,598 / 1,961** |

Replay digest (DGSA/DGSB) identical on all 1,960 rows. The P95 difference is
inside the single-run spread; the STG median is the bucket the change touches.
`gate-share0.*`, `gate-share1.*`.
