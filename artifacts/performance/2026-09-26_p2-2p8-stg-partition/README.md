# P2-2p8: where STG goes (2026-09-26, attribution only)

Lab ROM `NDS_TASK103_STAGE_RUN_PHASE=1` (build `build-p2p8-task103`), canonical
four-CPU match on Dream Land, ticks per frame over frames 1..1973. Lab timers
inflate the bucket (STG 277K here vs 264K in the gate ROM).

| span | ticks/frame | per call |
|---|---:|---:|
| prepare (`ndsRendererAdapterPrepareNativeStageOwner`, once) | 114,133 | |
| - matrix preparation | 92,426 (101,544 with E5 timers) | |
| -- Task36 stage camera (LookAt + PerspFast) | 4,170 | |
| -- frame camera cache fill | 5,081 | |
| -- world matrices, 27 dynamic bindings | 36,425 | 1,349 |
| -- composition (world x camera) | 17,760 | 658 |
| -- MVP recalc, 11 billboard bindings | 31,651 | 2,877 |
| - admit/validate/material/config/owner | 20,587 | |
| display commits (37 displays, 8 segments) | 163,099 | 4,359 |
| - segment commits | 150,864 | 18,858 |
| finish | 392 | |

Per-PC profile (`builds/p2p8-tail-profile-bc35`, 384 frames) inside the
commits: `ndsStageGxDraw` 65,184 ticks/frame across ~1,485 patch visits, the
largest lines being the no-Z Z-column writes (four 64-bit round-shifts per
patched corner, ~7.5K) and the kind dispatch of the first patch loop (~6K); a
second full pass over all patches only to find the no-Z ones costs ~5K.

No single cheap lever: the camera builds are duplicated (~4K), and the rest
is per-binding and per-patch work proportional to the stage. The Phase 2 goal
(STG <= 40K) needs the dynamic bindings and billboards off the per-frame
path, not tuning. Files: `task103*-route1` (json/rows/log).
