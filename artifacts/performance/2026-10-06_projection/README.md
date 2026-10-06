# Fighter projection: perspective scales in [4, 8) (2026-10-06)

Owner rows: "During master hand intro (pre fight), MH has brief invisible
frames" and "during the 'close up' version of fighter intros, fighters become
invisible".

## Cause

`ndsProjectToViewport` (src/port/renderer_adapter_fighter.c) is the fixed-point
`func_ovl2_800EB924` that `ftDisplayMainProcDisplay` uses for magnify culling
(and Link's Boomerang for its off-camera lifetime). It converts the camera
matrix's rotation rows to Q28 through `ndsR2CollisionF32ToFixed`, which
declines a left shift of 7 -- every |v| in [4, 8) at Q28 -- although the value
fits, and the projection saturated those cells to 8.0.

The perspective scale is cot(fovy / 2): 4.52 for Master Hand's intro camera
(fovy 25) and 4.01 for the close-up entry (`gmCameraSetStatusPlayerZoom(...,
28.0F)`, focus id 2). The projected point roughly doubled, the bounds test
failed, and the fighter was culled as magnified.

Measured before the fix (walk-1006d, `-StartStage 13`, tick 640): Master Hand
at (900, 1571, -10323), camera m[1][1] = 4.516602, projection y = 136.57 where
the float arithmetic gives 7.6; 8 x 1571.37 - 6774.9 = 5796 and
110 x 5796 / 4668 = 136.6 reproduces the saturated value exactly.
`is_magnify_show` was 1 for ticks 620-652.

## Fix

`ndsProjectRowToQ28`: a cell the shared converter declines converts at Q27 and
doubles. A float in [4, 8) is a multiple of 2^-21, exact at Q27, so the Q28
value is exact; below 4 the path is unchanged, so the battle camera (fovy ~38)
projects bit for bit as before.

## Verification

- walk-1006e, same probe: `is_magnify_show` 0 for every tick 616-664; window
  captures every 2 ticks from 620 to 660 show Master Hand in every frame
  (`artifacts/visibility/2026-10-05_oldexec/mhc4`, local only).
- Lab A/B, q45 -> q46 (`build-lab-clean1006q46`, ring dump, 1,961 frames):

| config | digest | P50 | P95 | over 1.12M | paired median |
|---|---|---|---|---|---|
| gate (Dream Land) | identical | 763,648 -> 763,904 | 1,046,144 -> 1,045,760 | 58 -> 59 | +832 |
| Castle | identical | 800,512 -> 801,216 | 1,095,552 -> 1,098,048 | 77 -> 78 | +1,152 |
| Sector Z | identical | 847,616 -> 849,280 | 1,155,136 -> 1,156,352 | 119 -> 121 | +1,856 |

The +1-2K paired medians are inside the same-ROM spread; the change runs only
when the camera matrix changes.
