# Old machinery deletion (2026-10-06)

Owner directive: "Delete Old machinery." Each step removes a concluded A/B arm
(route word, lean level, toggle) and keeps only the shipped arm. A deletion must
leave the replay digest identical and the time neutral; the paired medians are
the same-trajectory comparison (frames matched by index, digest-identical runs).

Configs: `gate` = Four-CPU gate (Dream Land), `g0` = Castle
(`gNdsLabFourCpuGkind=0`), `sz` = Sector Z (`gNdsLabFourCpuGkind=1`). Lab ROM
`smash64ds-p2-fourcpu-tickhud-hwtri`, clean (`NDS_LAB_FOURCPU_WORDS=1`).

## q33: camera A/B arms

Deleted: the float camera chain and `gNdsR2CameraFixedEnabled` (Q20.12 shipped
since 2026-08-16), the lean level word `gNdsCameraMatrixLeanEnabled` (level 2
shipped since cycle 103), the SELECT toggle build (`NDS_R2_CAMERA_FIXED_TOGGLE`)
and its HUD indicator, and the three Makefile knobs. `gmCameraLookAtFuncMatrix`
is now the former fixed body at level 2 semantics (projection Mtx still taken
from the graphics heap, W2b).

Baseline q32 = `artifacts/performance/2026-10-06_fixed-collision/*-q32.csv`.

| config | digest diff | P50 | P95 | over | paired median |
|---|---|---|---|---|---|
| gate | 0 | 778,048 -> 776,896 | 1,062,656 -> 1,059,840 | 65 -> 65 | -1,088 |
| g0 | 0 | 811,840 -> 810,048 | 1,159,808 -> 1,156,480 | 119 -> 116 | -2,496 |
| sz | 0 | 863,808 -> 858,880 | 1,168,064 -> 1,163,648 | 134 -> 128 | -2,880 |

Neutral to slightly positive (the level/route loads and the dead float arm's
code are gone from the hot path), digest identical. Kept.
