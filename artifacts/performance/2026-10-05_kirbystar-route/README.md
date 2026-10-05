# 2026-10-05 Kirby's star quad takes the stage-DL fast lane

The star quad four owners share (ITCommonObject 0x5458: the Star Rod's two
weapons and Kirby's two stars, an effect) gets a fast-lane route
(`NDS_SDL_ROUTE_KIRBYSTAR`, `src/port/renderer_adapter_stage.c`). The body
records the route after its native draw succeeds (word `gNdsStageDLFastMore`);
the fast lane admits it under any owner but an item submit and with no MObj,
seeds the effect layer's prim/env/othermode first when the effect layer
submits it (as the body's prologue does), and calls the same native draw.

Clean sweep ROM `build-lab-clean1005k` (`NDS_LAB_FOURCPU_WORDS=1`), Castle
(gkind 0), same ROM, `gNdsStageDLFastMore` 0 -> 1 (the word gates every
"more" route's recording, so this arm also drops the other fast-lane routes
recorded through it):

| Run | P50 | P95 | over 1,120K | replay |
|---|---|---|---|---|
| `m0-g0` -> `m1-g0` | 889,664 -> 887,296 | 1,196,288 -> 1,196,032 | 167 -> 168 | IDENTICAL (row sequence; ring seams fell at different rows) |

Paired median -64 over 1,901 frames. `m0-g2` (Jungle, word 0) is an unpaired
reference run.
