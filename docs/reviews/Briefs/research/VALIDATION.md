# Validation record

Prepared 2026-09-14, inspected base
`a5c5bc08d8e8661658865216798d600462db948e`.

## Executed here

| Check | Result | Scope |
|---|---|---|
| Candidate suite | 12 tests PASS, no skips | `tests/test_candidates.py` |
| Five source patch syntax/apply/reverse cases | PASS | Reconstructed exact-fragment preimages plus filler, not the full checkout |
| R01 depth/XY and R02 scope structural checks | PASS | Replacement text and call boundaries; no DS rasterization |
| R03 release-before-reset checks | PASS | Three changed source fragments; not whole-scene lifetime proof |
| R04 old helper negative control | Expected failure reproduced | Exact old helper bodies with stand-in tables return the wrong foreign table |
| R04 new helper compile/run | PASS | High/low, source-owner bounds, NULL owner, Link disabled and program-absent configurations |
| R05 generator fragment | PASS | Python AST and mocked Placement/SurfaceSpec layout assertions, not sprite decoding |
| R05 redraw helper compile/run | PASS | Mock native blitter: locked/unlocked, dirty rows, retries and reentry |
| E01 analytic suite | 8 tests PASS, no skips | Plane/area/winding, UV/alpha interpolation, edge agreement, alpha range and budget rejection |

Commands:

```text
python Briefs/tests/test_candidates.py
python Briefs/tests/test_alpha_isobands.py
```

Full outputs are preserved in `HOST_TEST_RESULTS.txt` and
`ALPHA_EXPERIMENT_TEST_RESULTS.txt`. The test harness intentionally identifies
its mocks/fixtures. It does not silently turn a missing compiler into a tested
compile: on other hosts those specific tests report SKIP.

## Not executed or not established

Complete source-tree application/compilation; ARM target linking; ignored O2R or
native owner/surface pack regeneration; effect asset/camera re-decode; natural
menu/game input; DS depth/alpha/translucency behavior; BGM capture; visual fidelity;
allocator/VRAM/GX budgets; actual menu or gameplay performance; widest relevant
verifier; subjective owner acceptance; published ROM identity. No remote writes.

The supplied checker performs complete-checkout *applicability* checks on the
user's machine. Even that successful result is not a target compile or a fix.

## Candidate-specific remaining risk

R01 must use the same active camera and source-depth mapping as the fighter and
must be tested through alpha decay/near clipping. R02 changes siblings and must
honor their explicit geometry/render state. R03 requires confirming no reset
reclaims data still in use by another preview/GX consumer. R04 needs the actual
generated foreign table declarations and every relevant preprocessor combination
in a full target build. R05 needs generated surface bounds, IDs, loader coverage,
staging, native output and menu cadence. E01 needs an explicit source-alpha,
lighting/projection, fixed-point, run-ABI and geometry-budget integration contract.

## Packaging checks

All 36 task-brief patches passed apply/check/reverse on new-file fixtures. These
are document deployment checks, not runtime fixes. Local Markdown links and
source-key references resolved; JSON manifests parsed; five runtime-patch hashes
matched their manifest. No Python cache binaries are included.
