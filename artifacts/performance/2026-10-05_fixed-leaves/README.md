# 2026-10-05 Float leaves to fixed point (F2, F3)

Owner, 2026-10-05: "Software floating point should not exist, fixed point
only, its taking ITCM space." Ruling D13 re-baselines the digest.

Clean four-CPU lab (`NDS_LAB_FOURCPU_WORDS=1`), items on. Gate = Dream Land
gate roster; `sz` = Sector Z gate roster; `yo` = 4 x Yoshi, Dream Land;
`-s2` = `sSYUtilsRandomSeed=2`.

## F2 (`build-lab-clean1005f2`, base `build-lab-clean1005lk`)

- `src/import/battleship_sys_utils.c` (new, replaces `utils.c` in CFILES): the
  arctangent family in fixed point -- Q30 magnitude from the float's bits,
  a 257-entry Q30 table, the hardware divider for 1/x and y/x.
- `src/import/battleship_sys_matrix.c`: the Mtx builders' FTOFIX32,
  SINTABLE_RAD_TO_ID and `scale * 256` taken from the bits (exact, not an
  approximation).
- `src/import/battleship_libultra_gu_mtxcatf.c`: guMtxCatF skips the products
  against a zero of `nf` (exact).

| Pair | P50 | P95 | over |
|---|---|---|---|
| gate `gate-lk` -> `gate-f2` | 811,648 -> 810,048 | 1,116,352 -> 1,116,416 | 91 -> 92 |
| 4 x Yoshi `yo-lk` -> `yo-f2` | 889,856 -> 889,984 | 1,275,200 -> 1,271,936 | 248 -> 242 |
| Sector Z `sz-lk` -> `sz-f2` | 899,904 -> 902,144 | 1,252,224 -> 1,256,192 | 190 -> 201 |

Digest leaves at frames 255-295 (the arctangent feeds knockback and item
angles). Flat within the single-run spread.

## F3 (`build-lab-clean1005f3`, base F2)

- `src/port/n64_stubs.c`: sinf/cosf/__sinf/__cosf in fixed point (binary
  angle, Q32 Taylor; host check 3.16e-8 at worst, as newlib's).
- `battleship_sys_utils.c`: atan by table plus the arctangent difference
  identity (host check 6.2e-8 rad, the source's continued fraction 1.7e-7).
- `src/import/battleship_sys_objanim.c`: every stage and item DObj takes the
  port player (fixed-point cubic), not only the Master Hand wallpaper.
- `src/port/reloc_backend_mp_collision.c`: floor/ceiling/wall unit normals
  memoized per integer slope (exact).
- `src/import/battleship_ftcomputer_fixed.c` (new): ftComputerCheckFindTarget
  and ftComputerCheckEvadeDistance in fixed point (decomp definitions weak).
- `src/port/renderer_adapter_fighter.c`: ftparam.c's projection
  (func_ovl2_800EB924, ProjectTarget) in fixed point.
- `battleship_sys_matrix.c`: one ARM converter and a loop syMatrixF2L (the
  inlined F2 conversions had grown it to 1.6 KB of Thumb).

| Run | P50 | P95 | over |
|---|---|---|---|
| gate F2 / F3 | 810,048 / 814,592 | 1,116,416 / 1,108,160 | 92 / 91 |
| gate seed 2 F2 / F3 | 821,568 / 824,960 | 1,111,360 / 1,108,032 | 94 / 89 |
| Sector Z F2 / F3 | 902,144 / 903,168 | 1,256,192 / 1,268,992 | 201 / 223 |
| Sector Z seed 2 F2 / F3 | 905,344 / 915,712 | 1,247,296 / 1,281,152 | 208 / 235 |
| 4 x Yoshi F2 / F3 | 889,984 / 892,416 | 1,271,936 / 1,267,520 | 242 / 248 |

The digests leave each other at frames 255-295, so whole-run percentiles
compare different matches. On the frames before that (identical game state,
paired), WORK median: gate +832 / +64, Sector Z -1,184 / -2,880, Yoshi -1,984.
By bucket, Sector Z's GCRA -2.5K / -3.3K (the fixed-cubic stage animation)
against MCAM +3.1K / +1.7K (the draw shell; the fixed projection is 1.1 KB of
main-RAM code entered cold once per fighter draw where the float form was a
few hundred bytes of calls into ITCM soft float); Dream Land's GCRA +1K.
