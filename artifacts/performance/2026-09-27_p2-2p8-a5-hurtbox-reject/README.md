# P2-2p8 A5 step 1: conservative hurtbox reject (2026-09-27)

**Outcome: BANKED.** Shadow oracle 0 flips over 14,153 tests; replay identical.

## Why

`shdtcount` (`69CAB23E`): 12,376 fighter-attack vs fighter-hurtbox tests per
match end in **39 hits**. Each first test of a joint in a tick builds its float
world chain, cofactor inverse and three axis-scale roots (~6,000 cycles); SHDT
is over 50K ticks on 372 of 1,972 frames.

## What

`src/port/nds_p2_hurtbox_reject.c` (ARM) decides only "certainly a miss", ahead
of the decomp float test (`src/import/battleship_gmcollision.c` wraps
`gmCollisionCheckFighterAttackDamageCollide` and, by rename, the weapon and item
attack variants whose only callers are in ftmain.c):

- The joint's world matrix is composed in fixed point with the host-graded
  helpers of `include/nds/nds_r2_collision_fixed.h` (Q26 rotation, Q12
  translation, ~0.002 units at depth 12), walking like func_ovl2_800EDBA4 from
  the source's own cached locals (`transform_update_mode`), the first latched
  ancestor world, or the root. Worlds are cached per DObj for the current
  `gNdsP2HurtboxLatchEpoch`, bumped wherever the port clears FTParts latches
  (the two `ftParamsUpdateFighterPartsTransform` shims) and on every status
  change (hidden-part topology).
- The local box `offset +/- (size + radius / s_k)` becomes a world AABB with
  half extent `sum_k |W[k][c]| size_k + radius sum_k |W[k][c]| / s_k`, bounded
  with `1/s_min` (floor sqrt, ceil divide). The attack's swept segment lies in
  the box of its two ends. Separation on one world axis by more than a 4-unit
  margin (~2,000x the fixed error) proves the miss.
- No source latch is written. `is_use_animlocks` fighters decline.
- `gNdsP2HurtboxRejectMode`: 0 off, 1 reject (default), 2 shadow (decide,
  count, still run the float test; a contradicted proof is a flip).

## Results (route 1)

| run | ROM | mode | rejects | passes | flips | digest |
|---|---|---:|---:|---:|---:|---|
| hbshadow | `7C15DE00` | 2 | 10,270 | 2,106 | 0 | identical |
| hbshadow2 (tight bound + weapons/items) | `56AF4CCB` | 2 | 13,499 | 654 | 0 | identical |
| hbreject2 | `56AF4CCB` | 1 | 13,499 | 654 | 0 | identical |

`hbreject2` against `memo` `69CAB23E`: WORK-H P50/P95/P99 1,258,688/1,740,544/
2,128,768 -> 1,257,472/1,725,376/2,127,744; SRC P95 912,704 -> 880,768; SHDT
P50/P95/max 10,048/216,640/797,760 -> 9,664/154,624/624,832; top-5% WORK-H mean
-28.5K. (The first, looser bound `hbreject`: P50 -14.6K, P95 -7.8K.)
Evidence: all five runs (json/rows/log) in this directory.

## Next

The remaining SHDT tail is the 654 passes (39 hits), hit processing and range
checks; the kernel itself is ~13.5K calls a match.
