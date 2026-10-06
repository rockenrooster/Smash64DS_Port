# Flat segments skip the float interpolation; the lean learn is one pass (2026-10-06, q20 -> q21)

Changes (both exact):

- `ndsMPLineDistanceFC` / `ndsMPLineDistanceFCf` (reloc_backend_mp_collision.c):
  a flat segment's height is `v1y` bit for bit. With `v1y == v2y` the factor
  `(v2y - v1y)` is +0, a finite quotient times +0 is a signed zero, and `v1y`
  (an exact s16, never -0) plus a signed zero is `v1y`. The quotient is finite
  when the point is finite and `v1x != v2x`, which the test requires. Every
  flat floor/ceiling query skips three subtracts, a divide, a multiply and an
  add (the CPU AI's floor checks run ~50 a frame on Dream Land). The vertical
  test in `mpCollisionGetFCCommonFloor` compares bits, not floats.
- `ndsFtrLeanLearnVariant` (nds_renderer_native_common.c) records the
  differences in the one pass that finds them and stops at the seventeenth;
  it used to count every difference in the ~16 KB list, then walk it again to
  record them -- two main-RAM reads of two lists after every materialization.

Lab four-CPU ROM, one run each; WORK-H (`runsum.py`), paired (`pairab.py`):

| config | P50 q20 -> q21 | P95 q20 -> q21 | paired median | digest |
|---|---|---|---|---|
| gate (Dream Land) | 780,672 -> 777,088 | 1,066,176 -> 1,064,128 | -2,816 | identical |
| Castle (gkind 0) | 822,912 -> 820,672 | 1,178,112 -> 1,172,608 | -2,624 | identical |
| Saffron (gkind 7) | 856,768 -> 852,160 | 1,188,800 -> 1,183,744 | -3,776 | identical |
| Sector Z (gkind 1) | 862,720 -> 859,968 | 1,163,712 -> 1,165,888 | -2,112 | identical |
| 4 x Yoshi | 852,288 -> 849,664 | 1,206,848 -> 1,195,264 | -3,776 | identical |

Over-gate shape at q21 (`overdist.py`): Castle needs 28 more frames under the
gate for P95 (25 of its 123 over-gate frames are within 50K), Sector Z 30,
4 x Yoshi 75, Saffron (q20) 58.
