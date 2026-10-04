# Stage GX: COLOR/TEXCOORD sent only when they change, 2026-10-04

## Why

After the translated painter (`2026-10-04_painter-trans`), Dream Land's
program was 5,589 words a frame, 606 of them COLOR commands (one a vertex)
and 606 TEXCOORD; 508 of the COLOR and 58 of the TEXCOORD commands repeated
the value the run had just sent.

## Change

`compile_nds_stage_gx.py`: the geometry engine keeps the current vertex
colour and texture coordinate until the next command, and nothing in a stage
program changes them otherwise (no NORMAL, no texture-matrix change). A run
now sends a baked COLOR or TEXCOORD only when its value differs from the one
the run last sent; a live patch's value is unknown until the frame, so the
next baked value after one is always sent, as is each run's first. Exact: the
geometry engine sees the same colour and texture coordinate at every vertex
(`test_stage_gx.py` replays the two registers and checks every vertex).

Dream Land 5,589 -> 4,890 words a frame (Castle 3,933 -> 3,609).

## Result (official gate, `build-gate-1004p`)

| | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `painter-trans/gate-pt1` | 884,224 | 1,223,680 | 1,548,160 | 214 | 1,732 / 1,961 |
| `gate-ad1` (dedup) | 882,496 | 1,222,848 | 1,549,568 | 209 | 1,734 / 1,961 |

Paired by frame: median -2,112, 1,729 of 1,960 frames better. Replay digest
IDENTICAL; window captures at frames 150 and 1200 pixel-identical to
`gate-pt1`'s ROM; 0 stage declines, 0 native failures.
