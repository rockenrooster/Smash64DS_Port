# Lean fighter kernel at render precision, 2026-10-04

## Change

`src/nds/nds_ftr_lean_kernel.c`, compile-time `NDS_FTR_LEAN_RELAXED`
(Makefile default 1). The lean kernel's outputs are display-list words only
(each drawn root's LOAD4x3 site and a texgen root's Q20.12 world); no
gameplay code reads them. Its arithmetic reproduced the old compose bit for
bit so the lean oracle routes could grade it. Relaxed:

- the Q20 compose truncates instead of rounding half away from zero (the
  bases keep 20 fraction bits; the hardware keeps 12);
- the Q20 -> Q12 output steps round half up (two instructions, no sign split);
- the angle index truncates the 48-bit mantissa product instead of
  reproducing the float multiply's round-to-nearest-even step (an index one
  step off at a rounding edge, 1/4096 of a turn).

Class: render precision (owner ruling D13's float/fixed latitude applied to
render-only words; no visible approximation). Kernel 4,920 -> 4,476 bytes,
same ITCM address. `NDS_FTR_LEAN_RELAXED=0` restores the exact kernel the
lean oracle routes (2/3, lab pokes) grade.

## Result

Gate target, 1,960 presented frames, slots 6/7 concurrently.

| arm | build | WORK P50 | WORK P95 | > 1.12M | two-VBlank |
|---|---|---:|---:|---:|---:|
| control (`gate-c1`) | build-gate-1004p | 855,744 | 1,192,064 | 169 | 1,778/1,961 |
| relaxed (`gate-r1`) | build-gate-1004r | 850,176 | 1,184,960 | 160 | 1,788/1,961 |

Paired by frame: median -6,720, mean -6,337. Replay digest identical over
1,960 samples (`scripts/compare-replay-digest.py --sequence --resync 4`).
The control reproduces the dyn-stride receipt's official numbers exactly.

Captures (local only, `artifacts/local/gatecap/lr1_*` vs `lc1_*`, frames 300,
900, 1,500; top screen 400x300): frame 300 pixel-identical, frame 1,500 three
pixels, frame 900 172 pixels -- an arc on Donkey Kong's head outline and a
few edge pixels on his arm, single-pixel rasterisation edges from sub-LSB
vertex moves. 0 native failures on both ROMs.
