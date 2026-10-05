# Fighter joint worlds resident in fixed point (owner ruling D13), 2026-10-04

## Change

Owner direction after the rejected per-call latch conversion
(`2026-10-04_latch-fixed`): "float to fixed back to float? thats inefficient.
should be fixed point the whole way, no float at all."

`NDS_P2_JOINT_RESIDENT` (P2 VS targets): the hurtbox reject's per-epoch world
cache (`src/port/nds_p2_hurtbox_reject.c`, Q26 rotation / Q12 translation) is
the one place a fighter joint's world is built.

- `gmCollisionGetFighterPartsWorldPosition` (hitbox positions, effect and
  item positions) reads it: point in, one fixed transform, point out. The
  decomp body stays as `ndsBaseGmCollisionGetFighterPartsWorldPosition` for
  declines (animation locks, guards).
- The held item's 0x52 matrix reads the parent joint's resident world and
  writes the Q20.12 the GX takes directly (no float, no s15.16 Mtx).
- Neither writes an FTParts latch; the float walk (`func_ovl2_800EDBA4`,
  `gmCollisionTransformMatrixAll`, `func_ovl2_800ED490`) is left to the few
  callers not converted (the hurtbox float decider, Link's afterimage, the
  shield bubble).
- The cache's local builder is now the compact relaxed form (truncating angle
  index, cosine = index + a quarter turn; the lean kernel's relaxation):
  996 B against the exact builder's 3,036 B.

Re-baselines the replay digest (divergence at frame 221 on every seed, the
first hitbox placed by the fixed path). Same-ROM A/B word
`gNdsP2JointResident`, build `build-gate-1004r`.

## Result (gNdsP2JointResident 0 -> 1, same ROM, 1,960 frames each)

| seed | P50 | P95 | > 1.12M | mean WORK |
|---:|---|---|---|---|
| 1 (official) | 842,560 -> 836,800 | 1,178,048 -> 1,169,536 | 141 -> 136 | 855,009 -> 850,417 |
| 2 | 844,224 -> 846,144 | 1,151,104 -> 1,153,792 | 123 -> 123 | 850,935 -> 851,942 |
| 3 | 815,296 -> 816,704 | 1,145,280 -> 1,144,896 | 113 -> 114 | 835,157 -> 835,817 |
| 4 | 846,720 -> 846,912 | 1,205,376 -> 1,208,512 | 150 -> 152 | 865,846 -> 866,252 |

Mean over the seeds: P50 -0.6K, P95 -0.8K, mean WORK -0.6K (the matches
diverge at frame 221, so seeds 2-4 compare different fights). Pre-divergence
frames identical in cost.

Same-ROM profile of the late window (frames 1,200-1,840, seed 1,
`artifacts/task37-census/res-prof-r0` / `-r1`, profile build
`build-gate-prof-r`): mean -16.5K cycles a frame. Removed: ~58K of float
(`__mulsf3` -15.1K, `__aeabi_fadd` -13.1K, sine index -5.6K, compose -5.4K,
local -5.4K, float GetFPWP -3.8K, walk -2.4K, attach -2.2K, ...). Added: ~40K
of fixed (`ndsP2HbWorldOf` +14.4K, `ndsP2HbLocalFromDObj` +13.2K,
`ndsP2JointItemAttach` +5.8K, GetFPWP +2.0K, conversions +3.8K). gdb call
counts over frames 1,300-1,400: float arm 958 composes, 1,186 locals, 234
float GetFPWP walks, 124 latch walks; resident arm 14 / 20 / 0 / 6.

The fixed walk is memory-bound, not arithmetic-bound: ~840 cycles a local and
~1.1K a composed level, most of it D-cache misses (the sine table, DObj and
FTParts reads, cache slots written through the write buffer and read back).
That is the next lever.

Correctness: hurtbox shadow mode (gNdsP2HurtboxRejectMode 2) on the resident
arm, 0 flips through frame 1,900; frame captures (local only) show held Beam
Swords in Link's and Fox's hands through the swing.

## Follow-up: the walk's sine lookups in DTCM, a lighter compose

The local builder reads the lean kernel's DTCM half of the sine table
(`gNdsFtrLeanSinHalf`, weak, once the lean kernel has filled it), and the
resident walk composes with truncating reductions and one unsigned range test
a cell (`ndsP2HbCompose`). Build `build-gate-1004s` (cross-ROM against
`build-gate-1004r` arm 1; the compose's rounding moves the digest, divergence
frame 221):

| run | P50 | P95 | > 1.12M |
|---|---|---|---|
| `gate-s1-r1` (1004r, resident) | 836,800 | 1,169,536 | 136 |
| `gate-s1-tweak` (1004s) | 834,880 | 1,163,328 | 131 |

Pre-divergence frames paired by frame: median -576. Late-window profile
(`res-prof-r1` -> `res-prof-s1`): the joint machinery (walk, compose, local,
float loads, attach) 49.7K -> 46.3K a frame; the out-of-line compose is 11.6K
of it.

## Rejected: the walk in ITCM

Census `res-prof-s1` section D ranked `ndsP2HbCompose`, `ndsP2HbToFixed` and
`ndsP2HbWorldOf` (1,356 B, ~4.6M cycles of non-mem stall over the window) above
five named residents at 2.5-4.2K cycles a byte (`ftMainUpdateMotionEventsAll`,
`ftPhysicsApplyGroundVelFriction`, `ftDisplayLightsDrawReflect`,
`ndsBaseMPProcessUpdateMain`, `ndsRendererParticleFloatToFixed`). Swapped
(build `build-gate-1004t`, digest identical to `gate-s1-tweak`): paired by
frame median +4,160, mean +4,037, 268 of 1,960 frames better; P50/P95
839,552/1,165,952. The evicted residents run every fighter tick and cost more
in main RAM than the walk saved. Reverted (patch kept out of the tree).
