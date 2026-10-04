# Stage painter depth by MTX_TRANS, 2026-10-04

## Why

The four-CPU gate's stage-owner pricing (`2026-10-04_stage-layers`, render
economy masks) put Dream Land's painter-heavy segments at ~37K ticks a frame
(segment 0 -20.9K, segments 6+7 -16K) for 91 no-Z triangles. Each no-Z
triangle carried its own 16-word projection load (MTX_MODE 0, MTX_LOAD_4x4
with the z column at w x its painter depth, MTX_MODE 2) and a per-frame
depth-column patch: 126 such triangles were ~2,400 of the program's 7,009
words a frame.

## Change

Template version 6 (`scripts/stages/compile_nds_stage_gx.py`,
`src/nds/nds_stage_gx.exec.inc`). A no-Z run that is not cross-matrix, has
no G_TEXTURE_GEN and (if composed) one coordinate shift loads one clip
transform into the position matrix -- a static run the camera's VIEW affine x
projection (`ndsStageGxViewProjection`, once per stage-matrix generation; its
baked world still multiplies onto it in the FIFO), a composed run its composed
matrix -- with the z column at w x the run's first depth. The projection
matrix is identity, and each later triangle sends MTX_TRANS(0, 0, -1), which
on a matrix that receives clip coordinates is z' = z + tz * w: one depth step
nearer, x, y and w untouched. The run's patch (`NOZ_VIEW_RUN` /
`NOZ_COMPOSED_RUN`, aux = its triangle count) consumes all its painter depths,
so the counter, the bands and every later draw see the same depths as before.
Cross-matrix, texgen and multi-shift composed runs keep the per-triangle
program.

Dream Land: 7,009 -> 5,589 words a frame, 297 -> 202 patches. Other VS
stages: Castle 4,816 -> 3,933, Jungle 7,467 -> 5,791, Hyrule 7,723 -> 6,088,
Yoster 6,355 -> 5,250, Yamabuki 7,607 -> 6,664, Inishie 6,595 -> 5,683, Sector
6,794 -> 6,442; Zebes and Final Destination have no eligible runs.

## Equivalence

- Fixed-point model of the geometry engine (scratchpad `gxsim.py`, old and new
  programs on the same synthetic camera, composed matrices and depth counter,
  11 stages): composed runs are exact in x, y and w; static runs (world x
  (view x projection) instead of (world x view) x projection) differ by at
  most 17 LSB of 20.12 in clip x/y and 1 in w (~0.02 px); the painter z's
  spread within a run stays under 0.7 of a depth step, so no ordering flips.
- Gate ROM window captures, old vs new template, same presented frames
  (gameplay identical): frames 150 and 1200 pixel-identical; frames 600 and
  1800 differ in 327 and 179 isolated pixels (pond texels and grass edges),
  no visible change. The per-run executor (fast path off,
  `gNdsStageGxFast=0`) is pixel-identical to the fast path. 0 declines,
  0 native failures. Captures stay local (ROM-derived).

## Result (official gate, `build-gate-1004p`)

| | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `instrument-out/gate-default0` | 895,360 | 1,232,576 | 1,561,280 | 226 | 1,722 / 1,961 |
| `gate-pt1` (translated painter) | 884,224 | 1,223,680 | 1,548,160 | 214 | 1,732 / 1,961 |

Paired by frame: median -10,752, 1,915 of 1,960 frames better (p10 -14.5K,
p90 -6.7K). Replay digest IDENTICAL.
