# ITCM: ndsStageGxDraw's 3,576 B go to the joint walk, 2026-10-04

With every stage program head-ordered (`../2026-10-04_stage-heads-fast`), all
stage segments take `ndsStageGxCommitFast` and `ndsStageGxDraw` runs only for
a declined segment. It leaves ITCM; `ndsP2HbLocalFromDObj`, `ndsP2HbWorldOf`,
`ndsP2HbCompose`, `ndsP2HbToFixed` (the resident fighter joint walk) and
`ndsMPWallSweepEdgeTrunc` (the wall fast path's dynamic edge memo) take it
(linker/nds_hot_text.ld fifth pack). Placement only.

Replay digest identical everywhere, so frames pair one to one.

Official gate (`build-gate-1004w` against `gate-s1-tweak`, which also lacks
the Blastoise, wall and head-order commits; none moves Dream Land's digest):
P50/P95 834,880/1,163,328 -> 831,744/1,154,496, over 131 -> 123, paired median
-2,304, mean -3,672.

Lab sweep ROM (`build-lab-sweepall3` -> `-sweepall4`):

| stage | P50 | P95 | > 1.12M | paired median |
|---|---|---|---|---:|
| Castle | 969,152 -> 962,880 | 1,304,640 -> 1,289,792 | 344 -> 321 | -4,544 |
| Sector Z | 1,026,560 -> 1,022,976 | 1,379,648 -> 1,375,680 | 629 -> 605 | -2,304 |
| Jungle | 1,007,872 -> 1,003,904 | 1,431,232 -> 1,411,328 | 570 -> 547 | -2,816 |
| Zebes | 988,416 -> 981,568 | 1,306,112 -> 1,297,088 | 389 -> 364 | -3,392 |
| Hyrule | 855,744 -> 850,432 | 1,111,488 -> 1,100,352 | 94 -> 79 | -5,952 |
| Yoshi's Island | 1,046,784 -> 1,041,728 | 1,395,008 -> 1,387,840 | 630 -> 613 | -3,072 |
| Dream Land | 898,688 -> 892,800 | 1,237,312 -> 1,226,624 | 224 -> 203 | -4,544 |
| Saffron | 1,023,232 -> 1,017,856 | 1,380,544 -> 1,371,008 | 560 -> 533 | -3,712 |
| Mushroom Kingdom | 1,030,272 -> 1,025,344 | 1,336,512 -> 1,316,480 | 532 -> 497 | -3,904 |
