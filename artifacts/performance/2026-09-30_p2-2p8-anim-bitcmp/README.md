# P2-2p8: the object-animation sentinel tests compare bits (2026-09-30)

Solo, no subagents. Gate = the four-CPU tick-HUD ROM on Dream Land; run files
in `artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/` under the arm
names below.

## Why

A fresh gate profile at `6020cdb4263` (`builds/p2p8-prof-gate8`, 1,800
frames) counts 6,204 soft-float helper calls a frame. About 580 of them are
`__aeabi_fcmpeq` from the generic object animator -- `gcParseDObjAnimJoint`,
`gcPlayDObjAnimJoint`, `gcParseMObjMatAnimJoint`, `gcPlayMObjMatAnim` and the
camera pair -- testing `anim_wait` against the AOBJ_ANIM_NULL / CHANGED / END
sentinels (once per call, and per AObj inside the players) and each command's
payload against 0.0F.

## Change

`scripts/import-overlays/battleship/src_sys_objanim.patch` routes those 28
tests through `NDS_OA_EQ` / `NDS_OA_NE` / `NDS_OA_NE0`, which compare bits
(`include/nds/nds_fcmp.h`). Exact for every input: the sentinels are finite
and non-zero, so equal bits are equal values and a NaN matches neither; the
zero test folds -0.0 into +0.0 and counts a NaN as non-zero, as `!=` does. The
port wrapper `gcPlayMObjMatAnim` (`src/import/battleship_sys_objanim.c`) takes
the same test. Same-ROM A/B word `gNdsObjAnimBitCompare` (0 = the float
compares).

## Result (same ROM, gate)

| Arm | P50 | P95 | P99 | two-VBlank | SRC median |
|---|---:|---:|---:|---:|---:|
| word off (`oa0_a`, `oa0_b`) | 916,160 / 915,968 | 1,235,008 / 1,236,928 | 1,447,040 / 1,448,576 | 86.8% / 86.7% | 411,456 / 411,264 |
| word on (`oa1_a`, `oa1_b`) | **910,848 / 911,936** | **1,234,048 / 1,229,440** | 1,442,496 / 1,442,432 | 87.1% / 87.2% | 406,976 / 406,656 |

P50 -4.7K, P95 -4.2K, SRC median -4.5K. Replay IDENTICAL on all four arms
against the previous commit's gate run (`b13_a`); native failures 0.

## The rest of the profile (for the next levers)

Tail excess (P93-97 frames against P40-60, 321K in all): the hurtbox kernel
and its local builds ~31K (`ndsP2HbRejectPoints` 29.5K in the band against
7.1K at the median; 13,791 calls a match in only 367 frames, ~2.2K cycles a
call), the status-change pose cluster ~40K (parse +11.9K, play +5.8K,
invalidate +5.4K, shield-pose unpack +4.8K, bind +3.9K, set-status +4.1K),
soft float ~27K. Steady: lean kernel compose 54K a frame, stage GX draw 48K.
