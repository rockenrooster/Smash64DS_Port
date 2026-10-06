# Lean lock local really in ARM state; particle centres in fixed point (2026-10-06, q19 -> q20)

Changes:

- `ndsFtrLeanLockLocal` (renderer_fighter_lean.c) is `noinline`. Commit
  `c6c5ed936ee` marked it `target("arm")`, but GCC inlined it into its only
  caller, the Thumb `ndsFtrLeanSlowLocal`, and compiled it as Thumb: the q19
  disassembly still had its nine `__aeabi_lmul` calls there (156 calls a frame
  on 4 x Yoshi, where every joint locks). Same for `ndsIfxFracFrame` (the
  interp solver's entry, inlined into the Thumb memo) and
  `ftComputerCheckEvadeDistance` (Thumb, two `__aeabi_lmul` sites, 21 calls a
  frame) now in ARM state.
- Particle draws (battleship_lbparticle.c, owner: "Software floating point
  should not exist, fixed point only"): a transformed particle's centre is
  three 64-bit dot products of the transform's Q16/Q8 form, built once per
  transform and pass, straight into the submitter's Q8
  (`ndsRendererSubmitParticleQuadQ8`, split out of
  `ndsRendererSubmitParticleQuad`). It was nine soft-float products and nine
  adds per particle, then the submitter's own float -> Q8 conversion. The
  transform's axis magnitudes come from the same Q16 rows through the math
  unit's square root; the toward-eye bias has a Q8 form. Out-of-range values
  keep the float products. Render only. The route-1 diagnostic block that
  wrote the Whispy draw position (dead at route 7) is gone.

Lab four-CPU ROM, one run each, same seeds; WORK-H (`runsum.py`), paired
(`pairab.py`):

| config | P50 q19 -> q20 | P95 q19 -> q20 | paired median | digest |
|---|---|---|---|---|
| gate (Dream Land) | 779,136 -> 780,672 | 1,069,248 -> 1,066,176 | +640 | identical |
| Castle (gkind 0) | 822,336 -> 822,912 | 1,176,640 -> 1,178,112 | +128 | identical |
| Saffron (gkind 7) | 855,296 -> 856,768 | 1,185,280 -> 1,188,800 | +1,664 | identical |
| Sector Z (gkind 1) | 862,656 -> 862,720 | 1,171,200 -> 1,163,712 | 0 | identical |
| 4 x Yoshi | 863,808 -> 852,288 | 1,218,176 -> 1,206,848 | -9,088 | identical |

The lock local's ARM state pays -11K on 4 x Yoshi (P50 and P95). The other
rosters do not lock and move within the layout drift seen between builds
(+/-2K paired).
