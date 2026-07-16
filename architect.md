# Task 6 Combat Renderer Cut Architecture

## Objective

Reduce the current Dream Land stage prepare cost without changing generated
packet data, draw order, submit classes, matrix transport, texture state,
alpha semantics, or fallback behavior.

## Owning boundary

`ndsRendererNativeStagePrepareRun` owns per-run validation and preparation.
The generated packet remains immutable. The existing dense-index mask remains
the authority for whether full vertex preparation has already happened.

Alpha behavior remains defined by `ndsRendererHardwareAlpha` and
`ndsRendererHardwareAlphaUsesVertex`:

- constant-alpha runs resolve alpha once from run state;
- vertex-alpha runs preserve the exact per-corner uniformity rejection using
  the source alpha byte;
- full input-vertex construction occurs only for the first preparation of a
  dense index.

The residual color follow-up keeps `ndsRendererHardwarePackedVertexColor` as
the generic caller boundary, including its invalid-vertex lighting fallback.
The stage owner has immutable valid RGBA for every dense vertex, so it reuses
the shared valid-color path. That path preserves the generic material-only and
white branches as well as vertex/material modulation, while omitting only the
invalid-vertex lighting fallback. The owner constructs `NDSRendererInputVertex`
only for no-Z vertices passed to `ndsRendererTransformVertex20p12`.

## Stable invariants

- Stage census: 8 segments, mask 255, 57 DObjs, 42 bindings, 54 runs, 202
  triangles, 49 epochs, and 4 material commits.
- Cross-matrix census: 5 runs, 10 triangles, and 15 foreign corners.
- Steady combat owner census: 121 runs and 828 triangles, partitioned as
  202/320/306, with zero fallback.
- M4 residency remains 22 textures and 131,072 bytes with a zero post-GO
  texture fence.
- The deterministic native 256x192 top-screen comparison remains exact.

## Backward-compatibility framework

- Frozen A: uninstrumented profile-1 mode-9 ROM SHA-256
  `58554D8361E77B6988F8F6C94F2BDB8A8F6FC81EE04D74B19FF08AC46E8E03B1`.
- Synchronized differential windows: 438..445 for stage transitions and
  600..607 for steady combat through
  `scripts/benchmark-renderer-fast-raw.ps1`.
- Contract verifier:
  `scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1`.
- Static regression suite: `scripts/check-gbi-decode-fixtures.ps1` and
  `scripts/check-one-minute-match-verifier.ps1`.
- Final runtime gates: locked-30 realtime smoke and the natural one-minute
  CPU-on match, using only `smash64ds-battle-playable-hwtri.nds` for the
  published build.
