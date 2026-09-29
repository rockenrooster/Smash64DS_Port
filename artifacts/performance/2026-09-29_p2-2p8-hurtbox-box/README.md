# P2-2p8: hurtbox kernel cuts from a fresh whole-match profile (2026-09-29)

## The profile

`builds/p2p8-prof-match2` is a whole-match per-frame-region profile of the gate
ROM at `b668b80cadb` (`run-task37-profile-census.ps1`, frames 100..1900).
`gate-counterfactual-p95.txt` (`cf95.py`: the work P95 with one symbol's cycles
removed from every frame) ranks the P95 levers:

| symbol | P95 drop | mean/frame |
|---|---:|---:|
| `__aeabi_fadd` | 63.6K | 47.6K |
| `ndsFtrLeanKernelCompose` | 55.4K | 54.0K |
| `ndsStageGxDraw` | 52.9K | 53.0K |
| `__mulsf3` | 34.9K | 21.5K |
| `ndsFtPosePlay` | 34.6K | 27.0K |
| `ndsP2HbRejectPoints` | 27.9K | 9.0K |
| `ndsFtPoseParse` | 22.1K | 11.8K |
| `ndsR2CfxBuildLocal` | 13.6K | 4.2K |

The hurtbox kernel and its local build together are ~41K of P95 and almost
nothing at the median (`gate-entry-counts.txt`: 26.1 tests and 21.0 local
builds a P90-98 frame against 6.2 and 5.0 at the median; 2,383 and 1,369
cycles each). `gate-float-callers.txt` (`callsplit2.py`, over-gate frames are
those whose work exceeds 1.12M ticks) puts the source's float collision chain
-- attack positions (`gmCollisionGetFighterPartsWorldPosition`) and the float
test of hurtboxes the kernel cannot reject (`func_ovl2_800EDBA4`, the inverse,
`func_ovl2_800EDE5C`) -- at ~880 soft-float calls of premium per over-gate
frame. `gate-long-stalls.txt`: the stage GX draw's DISP3DCNT read is the
largest wait outside the idle loop (7.7K ticks a frame, 572 cycles a read).

## Banked: two exact cuts in the kernel's per-joint build

- `src/port/nds_p2_hurtbox_reject.c`: 1/s_min (the radius term's scale bound)
  came from the hardware square root and divide, whose busy-waits were ~18% of
  the kernel's hottest rows. A 2x32-entry table of ceil(2^24/sqrt(m)) and
  ceil(2^24*sqrt(2/m)) over the top six bits of s^2 gives an upper bound at
  most 1.56% above the exact value; `invsqrt_test.c` checks
  inv^2 * s2 >= 2^78 for every s^2 the RowScales guard admits (1,069,547,521
  values, 0 failures). The looser bound sends 5 of 14,153 tests a match to the
  float test (13,499 -> 13,494 rejects). A/B word `gNdsP2HbInvTable`.
- `include/nds/nds_r2_collision_fixed.h` (`ndsR2CfxBuildLocal`): a joint
  whose three scales are exactly 1.0f -- most of them -- skips the three scale
  conversions and nine 64-bit products, because Shr(rot * 2^26, 30) is exactly
  (rot + 8) >> 4; a zero or subnormal angle indexes 0 for the sine and a
  constant for the cosine (angle + 90 degrees rounds to exactly 90 degrees in
  binary32), skipping the cosine's soft-float add.
  `scripts/check-r2-collision-fixed.ps1` passes (every gated row green, soft
  float confined to the local build). A/B word `gNdsR2CfxFastPaths`, read
  through the header's `NDS_R2_CFX_FAST_PATHS()` hook.

Same ROM, runs in `2026-09-26_p2-2p8-ftr-item-tail/`:

| arm (table, fast) | P50 | P95 | P99 | P90-98 WORK mean | P90-98 SRC mean |
|---|---:|---:|---:|---:|---:|
| `kf00` (0, 0) | 929,600 | 1,276,992 | 1,463,424 | 1,255,197 | 641,871 |
| `kf10` (1, 0) | 929,408 | 1,273,664 | 1,459,840 | 1,252,209 | 637,670 |
| `kf01` (0, 1) | 929,344 | 1,275,200 | 1,464,448 | 1,251,918 | 636,369 |
| `kf11` (1, 1) | **928,320** | **1,271,360** | 1,456,000 | **1,249,986** | **635,969** |

Both arms move the band the same way; together the band mean falls 5.2K and
P95 5.6K. Replay digest IDENTICAL (kf00 vs kf11, and vs the shadow arm `kfs`,
all 1,972 samples). Shadow mode (`gNdsP2HurtboxRejectMode=2`, both cuts on):
0 flips. Native failures 0.

## Tried and reverted: a per-box record

The same attack coll is tried against every hurtbox of a victim, and every
other live attack of the tick tries the same boxes again, so a record per
victim port and damage slot (centre, extents and radius coefficient, keyed on
joint, latch epoch and offset/size bits) let a repeat test skip the world
lookup, 1/s_min, the transform and the extents. It was exact (rejects and
passes unchanged, 0 flips) and 5,609 of 14,153 tests reused a record, but the
same-ROM A/B (`bx0`/`bx1`) read P50 -0.4K and P95 -1.3K, inside the run-to-run
spread, for ~1.8 KB of arena (44 records). The kernel's cost is the first
test of each box -- the chain build -- not the repeats.

Heap note: arena and low-water differ by ~6.4 KB between `ta1_gate` and these
runs for reasons outside this change (the generated particle-bank header the
build rewrites differs from HEAD); compare heap figures only within one build.
