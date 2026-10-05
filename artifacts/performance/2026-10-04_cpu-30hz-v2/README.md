# CPU decisions at 30 Hz (owner ruling D12d), re-measured on the current ROM, 2026-10-04

Same-ROM A/B word `gNdsCpuDecide30Hz` (0 = source behaviour) on
`build-gate-1004v` (HEAD `be6fc76cc27`), seeds 1-4 (seed 1 = the official
scenario; 2-4 poke `sSYUtilsRandomSeed` at boot), 1,960 frames each. The
decision skip changes the match; the digest diverges at frame 203 on every
seed (pre-divergence frames identical in cost).

| seed | P50 | P95 | > 1.12M | mean WORK |
|---:|---|---|---|---|
| 1 | 846,400 -> 837,760 | 1,180,160 -> 1,190,528 | 153 -> 161 | 859,380 -> 856,936 |
| 2 | 848,768 -> 822,592 | 1,169,472 -> 1,203,072 | 127 -> 160 | 855,601 -> 845,767 |
| 3 | 819,648 -> 834,624 | 1,159,872 -> 1,161,664 | 115 -> 125 | 839,077 -> 847,760 |
| 4 | 852,288 -> 816,512 | 1,221,248 -> 1,173,056 | 160 -> 139 | 870,930 -> 842,975 |

Mean over the seeds: P50 -13.9K, mean WORK -7.9K, P95 -0.6K, over +7.5.
As in `../2026-10-04_cpu-30hz`: the decisions are cheaper on average, but the
P95 frames are combat frames whose cost is elsewhere, and the reshuffle moves
P95 by tens of thousands either way. Not shipped; the word stays 0.
