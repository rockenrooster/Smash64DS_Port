# Fighter-part latch machinery in fixed point (owner ruling D13): measured, rejected, 2026-10-04

## Change measured

`src/port/nds_p2_latch_fixed.c` (not kept; patch in this receipt's commit
message history only): func_ovl2_800EDBA4, gmCollisionGetFighterPartsWorldPosition,
gmCollisionTransformMatrixAll and func_ovl2_800ED490 replaced by the same
walks in the hurtbox kernel's Q26/Q12 arithmetic
(`include/nds/nds_r2_collision_fixed.h`), FTParts latches still written as
floats, the decomp bodies kept under ndsBase names as the fallback. Same-ROM
A/B word `gNdsP2LatchFixed` (build `build-gate-1004f`). A gdb probe at frames
1300-1301 confirmed the fixed walk engaged (no fallback to the float bodies).

Why it was expected to pay: the late-window gate profile
(`task37-census/gate-prof01`) put ~1.3 walks a frame (mostly the held Beam
Sword's attach) at ~31K cycles including ~24K of soft float -- far more than
the P1 2-fighter rate that slice 52 measured.

## Result (gNdsP2LatchFixed 0 -> 1, same ROM, 1,960 frames each)

| seed | P50 | P95 | > 1.12M | mean WORK | digest diverges |
|---:|---|---|---|---|---:|
| 1 (official) | 845,568 -> 845,312 | 1,178,432 -> 1,181,888 | 153 -> 155 | 858,629 -> 859,326 | frame 221 |
| 2 | 847,680 -> 849,536 | 1,163,392 -> 1,164,928 | 125 -> 130 | 854,333 -> 855,875 | 221 |
| 3 | 818,240 -> 819,776 | 1,157,248 -> 1,158,976 | 114 -> 118 | 837,731 -> 838,990 | 221 |
| 4 | 850,688 -> 852,544 | 1,219,776 -> 1,221,504 | 158 -> 159 | 869,638 -> 871,187 | 217 |

Mean over the seeds: P50 +1.2K, P95 +2.1K, mean WORK +1.3K, over +3. Worse
on every seed. Pre-divergence frames are identical in cost (paired median 0).

## Reading

The walk runs about once a frame, so its code is cold every time: the fixed
replacements (BuildLocal, the 64-bit Load/StoreF32 conversions, the walk)
live in main RAM and miss the I-cache, while the float path is small decomp
code calling soft-float routines that sit in ITCM. Converting per call with
float boundaries does not pay on this hardware even at four-fighter call
rates; slice 52's "needs residency" reading stands (a fixed world must stay
resident and its consumers must read it in fixed). Reverted.
