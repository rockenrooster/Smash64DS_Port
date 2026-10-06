# 2026-10-06 Hurtbox joint cache 64 -> 128 slots, cofactor frames cached; reflection light memo

Clean four-CPU lab (`NDS_LAB_FOURCPU_WORDS=1`), items on; WORK-H from the ring
dump, frames >= 64 (`runsum.py`); paired figures from `pairab.py` (WORK,
frames >= 60). Base: `2026-10-05_interp-fixed` q10.

## Why

The 4 x Yoshi over-gate profile (`artifacts/task37-census/yo-yoprof10`, 384
frames from 200, 24 over two VBlanks) put hit detection at the top of the
premium: `ndsR2CfxMakeFrameCofactor` +45K cycles a marked frame (18/24),
`ndsP2HbLocalLock` +39K (19/24), `ndsP2HbRejectPoints` +36K, `ndsP2HbCompose`
+17K, `ndsP2HbWorldOfLock` +12K. Two causes:

- the joint-world cache (`src/port/nds_p2_hurtbox_reject.c`) had 64
  direct-mapped slots while four fighters bring ~100 joints (hurtbox joints and
  their chains) to the hit-detection phase, so chains evicted each other and
  were recomposed within the same tick;
- the narrow test rebuilt the joint's cofactor frame (the inverse) for every
  attack that reached it, though it depends only on the joint's world.

## What changed

- 128 slots (hash `>> 25`), +14.6 KB BSS.
- Each slot keeps its world's narrow-test frame (cofactor inverse, and the lock
  walk's 1/nscale for animation-lock fighters), reset wherever the world is
  rewritten, like `inv_smin_q26`.
- `ftDisplayLightsDrawReflect` (four degree-to-radian divides, two sines, two
  cosines a call, every fighter draw, the stage's same two angles) keeps the
  last direction bytes by the angles' bits.

All three are caches of exact results: every digest is identical.

## Runs

| Run | P50 | P95 | over |
|---|---|---|---|
| `yo-q10` (base) | 884,736 | 1,254,976 | 232 |
| `yo-q11` / `yo-q11b` | 880,576 / 880,704 | 1,238,464 / 1,234,368 | 214 / 214 |
| `sz-q10` = `sz-q10b` (base, bit-identical reruns) | 884,480 | 1,188,032 | 155 |
| `sz-q11` / `sz-q11b` | 881,856 / 882,240 | 1,191,424 / 1,188,608 | 156 / 152 |
| `gate-q11` | 796,800 | 1,079,936 | 78 |

Paired: 4 x Yoshi P95 -15.9K, P50 -3.8K; Sector Z P95 +0 (q10b -> q11b), P50
-2.1K; gate P95 -4.8K, P50 -3.3K. `sz-q11`'s 5.0M frame (1247) is an
uncorrected 2^22 timer wrap (ALL - 4,194,304 = the median), not a stall.
