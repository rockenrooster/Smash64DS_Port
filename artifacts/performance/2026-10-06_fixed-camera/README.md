# Fixed point: particle transforms, the battle camera, wallpaper, billboard Mod1 (2026-10-06, q24 -> q27)

Owner, 2026-10-06: "Software floating point should not exist, fixed point
only, its taking ITCM space." Census first (`softfloat-castle-q24*.txt`, the
Peach's Castle profile `castle-prof24`, `fcallers.py` / `fbyfile.py`):
soft float is a ~119K tick-per-frame floor on Castle (heavy frames only
+12.7K over the rest), spread over 330 callers; the largest domains are
collision (mpprocess.c 17.9K, mp_collision 7.1K, floor crossing 3.8K),
vector helpers 11.6K, hit detection 8.7K, matrices 8.5K, camera 7.4K, fighter
main 6.1K, the pose clock 4.9K, AI 4.3K, particles 4.3K, wallpaper 4.1K.

This batch takes the render-only domains (no particle, camera or background
value reaches the replay digest; every run below is digest-identical):

- q25: the particle LBTransform builder (`syMatrixTraRotRpyRScaF`, six
  `__sinf`/`__cosf` and ~40 float products a build, 2.8 builds a frame) in
  integers (table sine, Q30 rotation, Q16 scales), and the default battle
  camera (`gmCameraDefaultFuncCamera` and its nine helpers) in Q12, with the
  interest box's bound clamps on the ground data's integers.
- q26: the wallpaper perspective (`grWallpaperCalcPersp` and the native BG2
  owner's copy) in integers: math-unit square root, the syUtilsArcTan2 kernel
  at Q30, scale and position at Q16.
- q27: the kind-48 billboard Mod1 (`syMatrixLookAtF` x the perspective plus
  `guMtxCatF`, ~5K ticks a frame on Castle) as its closed form: the look-at
  turns about X only, so the rows are two direction cosines times five terms
  of the camera's 20.12 projection.

Lab four-CPU ROM, one run each, WORK-H (`runsum.py`), paired (`pairab.py`):

| config | step | P50 | P95 | paired | digest |
|---|---|---|---|---|---|
| gate (Dream Land) | q24 -> q25 | 775,616 -> 769,344 | 1,060,480 -> 1,050,560 | -7,040 | identical |
| Castle (gkind 0) | q24 -> q25 | 808,704 -> 801,984 | 1,161,536 -> 1,145,600 | -6,784 | identical |
| Sector Z (gkind 1) | q24 -> q25 | 856,000 -> 851,072 | 1,157,888 -> 1,152,128 | -6,464 | identical |
| gate | q25 -> q26 | 769,344 -> 770,624 | 1,050,560 -> 1,050,688 | +1,344 | identical |
| Castle | q25 -> q26 | 801,984 -> 801,600 | 1,145,600 -> 1,146,048 | -1,280 | identical |
| Sector Z | q25 -> q26 | 851,072 -> 850,624 | 1,152,128 -> 1,150,016 | -896 | identical |
| gate | q26 -> q27 | 770,624 -> 767,232 | 1,050,688 -> 1,050,368 | -2,624 | identical |
| Castle | q26 -> q27 | 801,600 -> 800,256 | 1,146,048 -> 1,151,616 | -2,304 | identical |
| Saffron (gkind 7) | q24 -> q27 | 848,128 -> 837,824 | 1,175,360 -> 1,164,672 | -9,600 | identical |

Castle q27 frame 1446 (WORK-H 5.1M) is the cpuGetTiming 2^22 wrap: its SRC
less 4,194,304 is the 544K every other build reads there.
