# Wall sweeps: the all-reject fast path takes dynamic groups, 2026-10-04

## Change

`ndsMPWallSweepStaticMiss` (src/port/reloc_backend_mp_collision.c) answered a
wall sweep before its prologue only when every group it examines is static.
A dynamic group (an animated or moving yakumono: Saffron's door, Castle's and
Jungle's moving parts) sent the call to the full sweep even when the group was
nowhere near. It now applies `ndsMPSweepGroupReject`'s own dynamic shift (the
range minus the yakumono's X edge, plus its speed in the Diff form, through
the same `ndsMPWallSweepEdgeTrunc` memo), so a dynamic group is rejected
exactly when the sweep would reject it. The Same/Diff wrappers pass the form.

Profiles before the change (`artifacts/task37-census/sw-prof-g7` against
`-g6`): on Saffron 64 of ~128 wall sweeps a frame fell through to the full
sweep (Dream Land: 3.6).

## Result (lab sweep ROM, all `NDS_P2_STAGE_*` on; `build-lab-sweepall` ->
`build-lab-sweepall2`, preset roster, items on, 1,960 frames)

Replay digest identical on all nine stages, so frames pair one to one.

| stage | P50 | P95 | > 1.12M | paired median |
|---|---|---|---|---:|
| Castle | 1,002,752 -> 966,592 | 1,346,944 -> 1,303,232 | 440 -> 336 | -36,928 |
| Sector Z | 1,036,672 -> 1,035,648 | 1,389,504 -> 1,389,568 | 661 -> 662 | -768 |
| Jungle | 1,010,368 -> 1,006,272 | 1,427,520 -> 1,419,328 | 579 -> 564 | -2,368 |
| Zebes | 1,012,672 -> 1,011,008 | 1,333,632 -> 1,328,704 | 488 -> 476 | -1,408 |
| Hyrule | 896,512 -> 896,064 | 1,153,024 -> 1,155,008 | 139 -> 138 | -512 |
| Yoshi's Island | 1,045,248 -> 1,044,864 | 1,396,480 -> 1,394,432 | 626 -> 626 | -192 |
| Dream Land | 898,048 -> 898,176 | 1,238,336 -> 1,238,592 | 223 -> 221 | -64 |
| Saffron | 1,090,880 -> 1,066,688 | 1,456,576 -> 1,428,224 | 858 -> 744 | -23,936 |
| Mushroom Kingdom | 1,060,288 -> 1,061,696 | 1,360,192 -> 1,364,416 | 685 -> 680 | 0 |

`a2-g*.csv` here; the `a1-g*.csv` baseline is in
`../2026-10-04_stage-sweep/`.
