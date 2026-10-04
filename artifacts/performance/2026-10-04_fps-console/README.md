# The FPS console's periodic text leaves the gate ROM, 2026-10-04

## Why

The four-CPU gate ROM (`smash64ds-p2-fourcpu-tickhud-hwtri`) already compiles
out the on-screen tick-HUD block (`NDS_TICK_HUD_DRAW=0`, architecture A9: the
gate measures the game, not the instrument). Beside it, the developer FPS
console (`NDS_BATTLE_FPS_HUD_ENABLED`, on in every live-input build) still
rewrote its FPS/UP, SLIP, VBI and GIT rows -- about five `iprintf` lines through
the libnds text console -- every half second. On the gate rows that is the
HUD bucket's spike on one presented frame in fifteen (HUD P95 35,136, max
58,624, against a median of 19,072). The GDB sampler reads the pacing counters
and the tick ring directly; nothing reads the console text, and the published
ROM has no console at all.

## Change

`NDS_BATTLE_FPS_HUD_DRAW` (Makefile, default 1; the four-CPU gate target
overrides it to 0, beside `NDS_TICK_HUD_DRAW`). With 0, the FPS HUD keeps its
sampling, the published FPS group and the one console clear at match start,
and skips the console writes (`NDS_BATTLE_FPS_HUD_PRINT`,
src/nds/nds_platform.c). The game's lower-screen battle HUD is untouched.

## Result (official gate, `build-gate-1004p`)

| | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank | HUD P95 |
|---|---:|---:|---:|---:|---:|---:|
| `item-replay/gate-routes` | 904,448 | 1,249,216 | 1,569,408 | 241 | 1,714 / 1,961 | 35,136 |
| FPS console off (`gate-fps0`) | 900,800 | 1,237,888 | 1,565,696 | 238 | 1,717 / 1,961 | 28,160 |

Replay digest IDENTICAL over 1,960 samples. This is instrument cost removed
from the measurement, not a game-side saving; the published ROM never paid it.
