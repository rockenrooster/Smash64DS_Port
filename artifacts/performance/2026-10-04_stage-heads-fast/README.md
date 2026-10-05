# DLLink stages take the one-pass fast commit (head-ordered GX programs), 2026-10-04

## Change

A DLLink stage packet's runs sit in DObj preorder and the runtime draws each
segment head by head (0, 2, 1, 3), so `ndsStageGxCommitFast` -- one patch pass
and one DMA over a segment's contiguous words -- refused every segment with a
head array and those stages took the per-run loop (`ndsStageGxDraw` per run,
four passes). `scripts/stages/compile_nds_stage_gx.py` now lays each segment's
words out in the head passes' order (stable within a head; GX program format
7, the run table still indexed by run), `ndsStageGxFastBuild` gathers the
segment's runs in the same order (a head the passes never draw leaves the
segment to the per-run path), and the single-head gate is gone. The draw order
is the per-run loop's, so the output is the same.

## Result (lab sweep ROM, all stages; `build-lab-sweepall2` -> `-sweepall3`)

Replay digest identical on all nine stages. Window captures of Saffron and
Hyrule at frames 240 and 480: 0 differing pixels against the previous ROM.

| stage | P50 | P95 | > 1.12M | paired median |
|---|---|---|---|---:|
| Castle | 966,592 -> 969,152 | 1,303,232 -> 1,304,640 | 336 -> 344 | +2,176 |
| Sector Z | 1,035,648 -> 1,026,560 | 1,389,568 -> 1,379,648 | 662 -> 629 | -8,896 |
| Jungle | 1,006,272 -> 1,007,872 | 1,419,328 -> 1,431,232 | 564 -> 570 | +960 |
| Zebes | 1,011,008 -> 988,416 | 1,328,704 -> 1,306,112 | 476 -> 389 | -24,128 |
| Hyrule | 896,064 -> 855,744 | 1,155,008 -> 1,111,488 | 138 -> 94 | -40,192 |
| Yoshi's Island | 1,044,864 -> 1,046,784 | 1,394,432 -> 1,395,008 | 626 -> 630 | +1,024 |
| Dream Land | 898,176 -> 898,688 | 1,238,592 -> 1,237,312 | 221 -> 224 | 0 |
| Saffron | 1,066,688 -> 1,023,232 | 1,428,224 -> 1,380,544 | 744 -> 560 | -41,920 |
| Mushroom Kingdom | 1,061,696 -> 1,030,272 | 1,364,416 -> 1,336,512 | 680 -> 532 | -30,848 |

Castle, Jungle, Yoshi's Island and Dream Land have no head array (layer
packets) and read as layout noise.
