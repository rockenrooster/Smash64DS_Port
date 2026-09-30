# P2-2p8 2026-09-30: the damage meter's source display callback is not called (BANKED)

## Change

`src/import/battleship_ifcommon.c`. The four damage meters' source display
callback (`ifCommonPlayerDamageProcDisplay`, ifcommon.c:772) ran every battle
frame: it copies each digit's Sprite out of the common file, places and scales
its SObjs in soft float (~180 soft-float calls a frame) and hands them to
`lbCommonPrepSObjAttr` / `lbCommonPrepSObjDraw`, which are empty on the DS.
With the lower-screen HUD (`gNdsIFCommonHUDLowerTextMode`, on wherever the
hardware renderer is and the debug HUD is not) nothing reads those SObjs: the
lower screen draws the meter from `sIFCommonPlayerDamageInterface` through
`ndsIFCommonRecordHUDState` / `ndsIFCommonGetBattleHudDamageState`, which the
update callback keeps. ifcommon.c's `gcAddGObjDisplay` is intercepted like its
`gcAddGObjProcess`: the meter's callback is installed behind a gate that
returns while `ndsIFCommonSkipDamageDisplay()` holds (battle scene, lower HUD,
A/B word `gNdsIFCommonDamageDisplaySkip`, default 1). The lower-HUD route test
accepts the gate as the damage callback.

Also: the meter colour (`1 - damage/300` blend, ifcommon.c:815-823,
expression for expression) is kept per player and recomputed only when damage
or colour id moves (~11 soft-float calls a player a frame).

A first version put the test in the capture loop; that loop is in ITCM, which
overflowed by 8 bytes. The gate costs the loop nothing.

## Evidence

Same-ROM A/B (`gNdsIFCommonDamageDisplaySkip` 0 vs 1), WORK-H ticks:

| Run | Skip off P50 / P95 / P99 | Skip on P50 / P95 / P99 |
| --- | --- | --- |
| gate a | 908,352 / 1,221,568 / 1,471,488 | 898,688 / 1,212,544 / 1,461,568 |
| gate b | 908,480 / 1,221,568 / 1,472,192 | 898,304 / 1,212,544 / 1,462,208 |
| gate c | 908,864 / 1,221,568 / 1,471,552 | 898,368 / 1,211,776 / 1,460,288 |
| gate d | 908,352 / 1,221,568 / 1,471,488 | 898,688 / 1,212,544 / 1,461,568 |
| lab Saffron | 1,083,200 / 1,443,072 / 1,740,608 | 1,073,856 / 1,428,992 / 1,746,752 |
| lab Zebes | 1,023,872 / 1,411,136 / 1,741,312 | 1,014,336 / 1,399,168 / 1,731,584 |

Gate: P50 -10.0K, P95 -9.2K (4/4), P99 -10K. Lab P95 -14.1K / -12.0K.
Replay digest IDENTICAL (gate a, lab Saffron; 1,972 samples). Native failures
unchanged (0 gate, 3 Saffron). Heap low-water unchanged.

Pixels: exact-frame captures of the gate ROM at presented frames 700/701
(`artifacts/visibility/2026-09-30_hud-damage-display-skip/`), skip off vs on:
byte-identical PNGs (0 differing pixels); the lower screen shows the timer,
portraits and all four damage percentages.

With the colour memo added (same ROM as above otherwise, two gate runs):
P50 899,072 / P95 1,212,224 / P99 1,462,016, replay digest IDENTICAL to the
skip-on arm, and the frame-700 capture differs from it only in the emulator
title bar's FPS counter (41 pixels, `memo-f700.png`).
