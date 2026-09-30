# P2-2p8: the floor/ceiling sweeps reject a segment by its x span (2026-09-30)

Solo, no subagents. Gate = the four-CPU tick-HUD ROM on Dream Land; lab = the
NDS_LAB_FOURCPU_SWEEP ROM (Donkey/Samus/Link/Kirby). Run files are in
`artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/` under the arm names
below.

## Why

A Sector Z profile at `4592e84fed0` (`builds/p2p8-prof-sz4`, 1,800 frames;
P50 1.19M, P95 1.62M) is soft-float bound: `__aeabi_fadd` 91K ticks a frame
(P95 counterfactual 129K), `__mulsf3` 34K. Its largest float consumers are
the Arwings' spline arc length (`syInterpGetQuartSum` + the port's memoised
`syInterpGetFracFrame`, ~1,830 helper calls a frame) and the floor sweep's
segment kernel: `ndsMPFCSegmentCrossesKernel` 20.2K ticks a frame of its own
plus ~810 helper calls, `ndsStageMPSweepFloorLoopSweep` 18.8K.

## Change (`src/port/reloc_backend_mp_collision.c`)

The kernel's non-flat branch returns 0 when the segment's x span
[min(v1.x, v2.x), max(v1.x, v2.x)] misses the sweep's
[min(position.x, translate.x), max(...)] -- the same selections and the same
bit compares on the same floats. The floor and ceiling loops now make that test
before calling it, skipping the call, its guards and the subtractions it opens
with. A flat segment (v1.y == v2.y) tests its crossing point instead of the
span, so it always reaches the kernel. The floor loop's flat test compares bits
(the vertices are s16 conversions: never NaN, zero is +0.0), and the ceiling
loop reads the cached vertex floats (the same `(f32)` of the same s16) instead
of converting four ints a segment. Same-ROM A/B word `gNdsMPSweepSegmentXReject`.

## Result (same ROM, word off against on)

| Run | P50 | P95 | P99 | two-VBlank | SRC median | rejects |
|---|---:|---:|---:|---:|---:|---:|
| Sector Z (`sxl0_g1`, `sxl1_g1`) | 1,180,992 / **1,158,720** | 1,621,376 / **1,583,872** | 1,919,552 / 1,888,256 | 41.5% / **44.5%** | 568,128 / 544,064 | 270,573 |
| Peach's Castle (`sxl*_g0`) | 1,007,296 / 1,004,032 | 1,337,024 / 1,334,144 | 1,689,216 / 1,686,016 | 70.9% / 72.1% | 501,952 / 496,896 | 54,150 |
| Saffron (`sxl*_g7`) | 1,085,888 / 1,083,968 | 1,449,920 / 1,446,976 | 1,744,064 / 1,739,520 | 52.8% / 53.2% | 498,880 / 495,872 | 27,653 |
| gate, Dream Land (`sx0_*`, `sx1_*`) | 912,896 / 912,960 | 1,232,640 / 1,232,704 | 1,446,464 / 1,446,464 | 86.8% / 86.8% | 407,744 / 407,744 | 0 |

Replay IDENTICAL: every lab pair, and every gate arm against the previous
commit's gate run (`b13_a`). Dream Land's floors are all flat, so nothing
there takes the new test.
