# Combat Renderer Cut Architecture

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

## Task 8 Phase 0.5 conservation

The compile-gated `NDS_RENDERER_M3_PHASE0_PROFILE` lab extends the existing
Phase-0 sample rather than creating another telemetry mode. Its synchronized
draw equation is:

```text
draw = gcDrawAll shell + stage + Mario + Fox + fighter guard
     + present shell + wallpaper + foreground + outer draw shell
```

The present and loop residuals are separately decomposed into frame reset,
present tail, flush preparation, profile bookkeeping, sample publication, and
their outer shells. Wallpaper timing covers setup, X-map, Y-map, physical pixel
writes, and commit; it records exactly 192 visited rows. The non-incremental
oracle must write exactly 49,152 pixels, while the retained production
incremental path must write a positive subset no larger than 49,152.

## Task 8 Cut E generation contract

`ndsRendererAdapterPrepareNativeStageOwner` owns scene-generation topology.
The first accepted frame of a reloc generation performs the complete asset,
segment, display-link, DObj hierarchy, transform-kind, binding, display-list,
and generated-packet validation. A cached frame may bypass only those immutable
walks when every stamped identity still matches.

Per-frame work remains live and mandatory: camera and DObj matrices, material
snapshots and texture indices, colors, selection/visibility, animation state,
texture resolution, alpha policy, near-plane classification, and prepared run
state. A missing or mismatched stamp re-enters the complete validator before
any GX mutation. The Phase-0 lab must include a one-shot stamped-byte fault that
proves this slow path remains reachable and rejects or revalidates safely.

### Cut E accepted implementation

The adapter cache stores only authoritative reloc generation, GObj/DObj/XObj,
and display-list topology identities. The renderer cache stores only the summary
produced by one complete generated-table validation. Scene reset invalidates both
caches. Production telemetry is limited to full-validation, hit, and mismatch
counters; the one-shot fault and revalidation counters are linked only in the
Phase-0 lab.

The accepted profile-1 production ROM is SHA-256
`6496D8907BFD67A46647A246A77EEEC1326D195B7ED8DEF13D2A65A9538518CB`.
Against frozen ROM `07FBFCB21586AA3964432ADD9055A98DB29E0D317895B02E1B1FE6DFEBD67765`,
frames 600..607 save 18,816/18,752 stage ticks and 19,712/19,840
draw ticks at P50/P95. The native frame-607 comparison is exactly 0/49,152
changed pixels. The final-source lab ROM
`9246A45B17FFBAF0D79BE8067AAC2697F408B2EDA420979606EB1229D58C9DC7`
reports two full validations, 605 hits, and exactly one mismatch, injection, and
successful revalidation. The explicit pacing smoke preserves fixed-two cadence
but reaches only 19.0 presents/s; Cut E is a banked gain, not a locked-30 claim.

## Task 8 Cut F fighter-matrix contract

`ndsRendererAdapterBuildFighterTraRotRpyExact` owns only the display adapter's
unscaled mode-0 fighter matrix seam. BattleShip's `SINTABLE_RAD_TO_ID` macro and
`syMatrixRotRpyR` remain authoritative for binary32 angle conversion, source
table lookup, integer products and shifts, matrix packing, and fixed W. The
adapter reproduces that conversion with integer operations, uses the imported
`gSYSinTable`, and reuses the existing exact 16.16 translation conversion.

The complete builder is one ARM-state function because the adapter translation
unit is otherwise Thumb and would lower its 64-bit multiplies to a libgcc call.
Any unsupported angle or translation fails closed to `syMatrixTraRotRpyR` before
the matrix is consumed. Cached matrices, scaled transforms, generic fallback,
gameplay callers, matrix layout, and BattleShip source remain unchanged.

The backward-compatibility framework has three independent layers:

- `scripts/check-fighter-matrix-angle-index.ps1` executes 234,881,492 source
  comparisons, exhausting both signs and every mantissa in the nontrivial
  exponent classes 117..130; lower accepted classes are covered by endpoint
  checks and the product-magnitude-below-one proof. It also requires ARM UMULL
  code generation with no `__aeabi_*` helper call and is part of DevFast.
- Profile 2 rebuilds every eligible matrix with BattleShip's original function
  and byte-compares the complete `Mtx`. Matrix comparisons share the renderer
  oracle mismatch counter; the reported 2,484 sample field counts vertex-oracle
  samples, not fighter matrices. The live-Fox frames 600..607 gate is zero
  mismatch with 40 eligible matrices in the detailed profile-1 census.
- The accepted profile-1 and forensic ROM SHA-256 identities are respectively
  `5784EE4F7C3C213557E1A3AEEE43549794F465F7C831BB70CB0F2639A969A725`
  and `796765A83CD796AB065B0FC634CE177C333FC649FF2C43A603ED1DD09BCF9CD0`.
