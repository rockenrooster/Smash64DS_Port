# Quiet dynamic stage bindings validated one frame in four, 2026-10-04

## Why

Slice 44 already revalidates rigid stage bindings round-robin (stride 8 on
the gate and the published ROM). The dynamic bindings (not in the generated
rigid mask) still re-walked their parent chains and matched every node's
source key every frame -- the persistent world machinery
(`ndsRendererAdapterPersistentStageWorldPtr`, `...SourceKeyMatches`,
`...FindStageWorldEntry`) was ~45K cycles a frame in `sz-lateprof05` -- and
on Dream Land those chains almost never move.

## Change (owner ruling D12c: stage background LOD)

`ndsRendererAdapterPrepareNativeStageBindingMatrix` keeps, per dynamic
binding, the number of frame validations in a row whose chain walk rebuilt
nothing (`sNdsRendererAdapterStageWorldRebuilds` read around the walk). A
binding quiet for 8 of them is revalidated one frame in `gNdsStageDynStride`
(4; the slice 44 cursor spreads them), and reuses its persistent world on the
others; the camera compose still runs every frame, so nothing lags the
camera. Any rebuild returns the binding to every-frame validation: a part
that starts moving after a quiet spell shows it at most three frames late.
Bindings whose DObj chain holds a yakumono (moving map collision) are never
strided, so a platform's render never lags the collision fighters stand on.
A/B word `gNdsStageDynStride` (1 = every frame, the old behaviour).

## Result (official gate, `build-gate-1004p`, same ROM)

| | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `gate-s1` (stride 1) | 864,128 | 1,204,160 | 1,524,672 | 179 | 1,766 / 1,961 |
| `gate-s4` (stride 4) | 856,064 | 1,192,192 | 1,518,656 | 169 | 1,781 / 1,961 |
| `gate-s4y` (+ yakumono pin) | 855,744 | 1,192,064 | 1,520,960 | 169 | 1,778 / 1,961 |

Paired against `gate-s1`: s4 median -8,128 (1,889 of 1,960 frames better),
s4y median -8,000 (1,883). Replay digest IDENTICAL (render only). Window
captures at frames 150, 600, 1200 and 1800, stride 1 against stride 4:
pixel-identical.
