# Exact incremental wallpaper maps and dirty-row DMA

IDEA ID: `WALLPAPER_INCREMENTAL_DMA`

HYPOTHESIS: Retained BG2 already contains the previous exact frame. Alternating
exact X/Y source maps can identify dirty rows and columns without caching final
pixels; cache-flushed DMA for full dirty rows avoids the scattered VRAM-store
cost that defeated the first incremental-only measurement.

TARGETED EXCLUSIVE COUNTER: profile-1 wallpaper and whole draw.

MEASURED UPPER BOUND: the disabled 128-frame wallpaper median was 344,672 ticks,
17.9% of the 1,926,624-tick whole draw. Complete wallpaper removal was therefore
the strict upper bound.

LIVE-TREE RECONCILIATION: started from `a2d073043f`; retained K-RAW mode 3,
direct TRI remainder, persistent stage worlds, compact CI4 rows, exact wallpaper
recurrence, and all newer source/gameplay work. `decomp/` was read only.

FILES/FUNCTIONS CHANGED: `src/port/sprite_preview_backend.c`
(`ndsSObjWallpaperFinalSourceMatches`, `ndsSObjDrawOpaqueWallpaperFinal`,
`ndsSObjDrawCachedWallpaperFinal`), precise DMA/cache includes in
`src/port/scene_backend.c`, benchmark selector/export plumbing, static fixtures,
and active docs.

BUILD TARGET/FLAGS: canonical/coarse/forensic mode-163 HW-triangle targets;
scene `-O2 -mthumb`, renderer `-O2 -marm`, wallpaper function targeted `O3`.
Time to the first dirty-map measurement was about five minutes; the DMA
refinement measured about ten minutes after editing began. Five build
invocations produced the initial, DMA, forensic, final coarse, and canonical
artifacts.

BASELINE/EXPERIMENT ROM SHA-256: final 128-frame profile-1 A/B used the same ROM,
`6242A7EEFFBCD91002B0E6BB6AF2441D265000B9ADB977A52FE74898EA03574E`.
The final canonical ROM was
`BAC62CE2F36C290BC7D1B52099FB4FEDAFEED400F7D4BE7294A95C33A6E50C2C`.

FRAME/LOGIC-TICK WINDOW: disabled frames `223..350`, candidate `227..354`;
both are 128 contiguous synchronized warm frames with one source-timer reset
allowed by the existing profiler.

A0 MEDIAN/P95: present `2,106,016/2,485,120`; draw
`1,926,624/1,955,648`; wallpaper `344,672/348,480`.

B0 MEDIAN/P95: present `2,092,640/2,214,720`; draw
`1,812,256/1,900,288`; wallpaper `237,088/340,032`.

A1 OR DISABLED-CONTROL MEDIAN/P95: the preceding same-ROM 32-frame A/B/A was
draw `1,926,048/1,930,176 -> 1,846,176/1,900,288 ->
1,922,880/1,928,512`; wallpaper controls both returned to
`348,160/348,800`.

ACTIVE/WAIT SPLIT: disabled active/wait `1,963,296/1,992,768` and
`152,192/500,992`; candidate `1,849,216/1,936,960` and
`243,968/474,880`. Conservation error stayed `0/0`; wait is not added to draw.

OWNER SPLIT: stage `813,760/827,520 -> 813,920/827,520`; Mario
`358,080/378,112 -> 358,080/378,112`; Fox `393,472/400,256 ->
393,440/400,320`. The measured delta is isolated to wallpaper/draw.

OP/PROGRAM BYTES/FALLBACKS: no prepared program. Existing K-RAW remains
`45/540` runs/triangles (`60/246/234` by owner), with `47/7/0` bounded
state/vertex/command fallbacks. Map/row scratch is 1,408 existing pixels; a
source or BG2 epoch mismatch performs a full exact redraw.

ITCM/DTCM/BSS/STACK/ARENA DELTA: ITCM remains `20,376/32,768` with 18,640
renderer bytes; DTCM remains zero and canonical main BSS remains 1,875,504.
No arena or final-frame buffer was added. The shipping const-propagated mapper
is 1,104 bytes with a 128-byte total compiler frame; profile 1 retains the A/B
branch in 1,584 bytes with a 144-byte frame.

SEMANTIC TRACE RESULT: unchanged 828 owner events and source provenance; profile
2 retains 202/320/306 stage/Mario/Fox events with zero overflow.

ORACLE/COUNTER/GX RESULT: renderer oracle `2484/0/0`; wallpaper map/pixel
`23296/0` and `2555904/0`; 828 triangles and `648/44/126/10` submit classes;
`121/707/121` batches; `98/730` texture prepare/reuse; exact accepted upload
pairs including 36,864 bytes. Dynamic GX endpoint varies with sampled animation
frame but remains populated and verifier-valid. BG2/BG3 staging, clears, and
copies remain zero; committed bytes equal twice the physical pixel writes.

CAPTURE RESULT: synchronized canonical capture
`artifacts/visibility/2026-07-13_canonical_fast_051730-5150474-p5240.png`
passed full visibility, green/detail, region, horizontal-detail, and paired-frame
change gates and was published as `latest.png`.

VERIFIER COMMANDS AND RESULTS: docs, architecture, registry, and GBI static
checks passed; DevFast, final profile-2 eight-frame forensic mode 1, P1Gate,
and Boundary `161/162/163` passed. The long P1Gate was rerun `-NoBuild` after
two outer shell timeouts; its complete registered result passed. Full Regression
is intentionally excluded per user direction.

DECISION: KEEP. Profile-1 draw median improves 114,368 ticks (5.9%) and P95
improves 55,360; canonical profile-0 draw is `1,690,176/1,867,392`, improving
169,376/102,208 ticks from the previous checkpoint.

NEXT MEASURED BOTTLENECK: stage owner at about 814K median ticks. Select one
coarse complete-stage owner kernel; do not revive fixed schedules, per-VTX
memoization, small `glCallList` calls, or the generic packet VM.
