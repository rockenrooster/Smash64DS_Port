# 2026-10-05 The CPU attack pick in fixed point; particle centres refuted (Q4-Q6)

Owner, 2026-10-05: "Software floating point should not exist, fixed point
only." Ruling D13 re-baselines the digest. Clean four-CPU lab
(`NDS_LAB_FOURCPU_WORDS=1`), items on; WORK-H from the ring dump, frames >= 64
(`runsum.py`); paired medians from `pairab.py`. Base: Q3
(`2026-10-05_camera-fixed`).

## ftComputerCheckDetectTarget at Q12

`src/import/battleship_ftcomputer_fixed.c` (the decomp definition is weak in
`battleship_ftcomputer.c`): the per-attack prediction of both fighters
hit_start_frame ticks ahead -- ~30 soft-float operations an attack, ~150 a
frame in the float census -- at Q12, its inputs read once a call at the first
attack that reaches it, the two apex frames computed once (the source
recomputes them per attack), the detect boxes converted at the compare. Every
branch, write and random draw is the source's.

- **Q4** (64-bit products): the TU is Thumb, so each int64 multiply was a
  `__aeabi_lmul` call (17 in the function): Sector Z paired +0.8K, 4 x Yoshi
  +1.0K on identical games -- slower than the float it replaced.
- **Q6** (32-bit products, lazy inputs; the camera's zoom products 32-bit
  too): Sector Z +0.3K, 4 x Yoshi -0.6K, digests identical; the gate's AI
  now decides differently from frame 424 (paired -1.0K over the 364 frames
  before), and that match reads P95 1,087,616.

| Run | P50 | P95 | over |
|---|---|---|---|
| `gate-q6` | 802,176 | 1,087,616 | 77 |
| `sz-q6` | 893,184 | 1,218,560 | 189 |
| `yo-q6` | 886,848 | 1,258,688 | 238 |

## Refuted: particle centres in Q8 (Q5)

lbparticle's transformed particles computed their world centre from the
LBTransform affine at Q16 and submitted it in Q8 (a new submit entry). Same
ROM, A/B word `gNdsParticleCenterQ8` 1 vs 0: gate +2.2K, Sector Z +3.0K,
4 x Yoshi +3.0K paired (`*-q5-p0` vs `*-q5`): the per-particle 64-bit products
were libgcc calls in Thumb and the submit gained a call layer. Reverted.
