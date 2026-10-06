# CLZ and 64-bit divides off libgcc (2026-10-06, q18 -> q19)

Follow-up to `2026-10-06_lmul-arm`. The objdump census of `bl` targets found
the remaining Thumb callers of `__clzsi2`/`__clzdi2` (ARMv5TE Thumb-1 has no
CLZ) and the two 64-bit divides in hot fixed-point code
(`__aeabi_ldivmod`).

Changes:

- `syUtilsArcTan2` (battleship_sys_utils.c) and `ftComputerCheckFindTarget`
  (battleship_ftcomputer_fixed.c) compiled in ARM state: their normalisation
  `__builtin_clz` becomes one CLZ instruction instead of a libgcc call.
- `ndsAtanUnitQ30`'s cubic correction `d3 / 3` and `gmCameraUpdateInterests`'
  Q16 interpolation divide go through `ndsR2HwMathDivideFast` (the DS math
  unit). The unit truncates toward zero exactly as C's `/` does, so the
  results are the same words.

Lab four-CPU ROM `smash64ds-p2-fourcpu-tickhud-hwtri`, one run each, same
seeds; WORK-H from `runsum.py`, paired from `pairab.py`:

| config | P50 q18 -> q19 | P95 q18 -> q19 | paired median | digest |
|---|---|---|---|---|
| gate (Dream Land) | 783,744 -> 779,136 | 1,068,736 -> 1,069,248 | -3,392 | identical |
| Castle (gkind 0) | 827,456 -> 822,336 | 1,180,992 -> 1,176,640 | -3,520 | identical |
| Saffron (gkind 7) | 858,368 -> 855,296 | 1,190,912 -> 1,185,280 | -3,264 | identical |
| Sector Z (gkind 1) | 867,584 -> 862,656 | 1,168,320 -> 1,171,200 | -4,224 | identical |
| 4 x Yoshi | 867,200 -> 863,808 | 1,220,352 -> 1,218,176 | -3,264 | identical |

Medians improve 3-5K everywhere; P95 moves within the single-run spread
(+/-5K). Kept.
