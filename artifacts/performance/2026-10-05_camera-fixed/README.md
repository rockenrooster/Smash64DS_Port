# 2026-10-05 The camera's interest box in fixed point; integer floor brackets (Q3)

Owner, 2026-10-05: "Software floating point should not exist, fixed point
only." Clean four-CPU lab (`NDS_LAB_FOURCPU_WORDS=1`), items on; WORK-H from
the ring dump, frames >= 64 (`runsum.py`); paired medians from `pairab.py`.
Base: Q2 (`2026-10-05_stage-anim-q`).

`build-lab-clean1005q3`:

- `src/import/battleship_gmcamera_fixed.c` (new; `battleship_gmcamera.c` makes
  the decomp definition weak): gmCameraUpdateInterests with positions at Q12,
  the zoom products at Q16 and the interest box in integers; only the four
  results leave as floats. The bounds clamps are the source's own calls.
  The float census put the source at ~150 soft-float calls a frame on every
  roster.
- `src/port/reloc_backend_mp_collision.c` (ndsMPProjectFloorGeometry): the
  bracket tests `(f32)x <= px` / `(f32)x >= px` on integer vertices are
  `x <= floor(px)` / `x >= ceil(px)`, taken once per platform group from px's
  bits -- exact (a px the bits cannot bound keeps the float tests).

| Pair | P50 | P95 | over | digest | paired median |
|---|---|---|---|---|---|
| gate `gate-q2` -> `gate-q3` | 813,504 -> 812,288 | 1,107,328 -> 1,108,480 | 90 -> 90 | identical | +0.1K |
| Sector Z `sz-q2` -> `sz-q3` | 895,616 -> 893,248 | 1,219,584 -> 1,217,600 | 192 -> 191 | identical | -2.1K |
| 4 x Yoshi `yo-q2` -> `yo-q3` | 891,328 -> 888,768 | 1,261,184 -> 1,261,184 | 237 -> 242 | identical | -0.7K |

The camera feeds no digested state, so every digest holds.

Float census after Q2 (`build-lab-fcen1005q2`, frame 1,900, `sites-*-q2.txt`):
3,332 soft-float calls a frame on the gate roster (3,537 at F3), Sector Z
4,101, 4 x Yoshi 3,544.
