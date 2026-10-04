# Hurtbox reject: Q15 locals and DTCM sines, rejected, 2026-10-04

The late-window profile (`artifacts/task37-census/sz-lateprof03`) put the
hurtbox reject kernel at ~100K cycles in a tail frame, 60% of it world
building: `ndsR2CfxBuildLocal` 28% (~1,270 cycles a call, six main-RAM sine
lookups and an int64 cell array), the Q26 compose 14% (~470 cycles).

Tried in `build-gate-1004m` (patch `hurtbox-q15-rejected.patch` here):

- scale-1.0 joints build their local rotation as Q15 halfwords and compose
  with the parent's Q26 world through SMULWB/SMLAWB (same-ROM word
  `gNdsP2HbQ15`);
- every local build reads its sines from the lean kernel's DTCM half table
  (`ndsFtrLeanSinHalfTable`), with one mirrored index into either copy.

Lab shadow run (Dream Land, gate configuration, frames 300-1990,
`gNdsP2HurtboxRejectMode=2`): 21,610 rejects, 313 passes, 0 flips; 7,743 Q15
levels composed; largest per-level delta against the Q26 local 3,520 Q26
steps (5.2e-5) in a rotation cell, 0 in a translation.

Official gate:

| | WORK P50 | WORK P95 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|
| committed pack (`build-gate-1004l`, `itcm-pack4/gate-pack2`) | 920,704 | 1,259,712 | 280 | 1,678 / 1,961 |
| `build-gate-1004m`, `gNdsP2HbQ15=0` (DTCM sines only) | 918,656 | 1,265,216 | 271 | -- |
| `build-gate-1004m`, Q15 (default) | 918,464 | 1,264,256 | 272 | 1,678 / 1,961 |

Same ROM, Q15 against Q26: paired median 0, P95 -1K -- inside the
single-run P95 spread. Against the committed pack: P50 -2.2K, P95 +4.5K,
two-VBlank count unchanged. Replay digest identical everywhere. Reverted:
the precision change bought nothing measurable.
