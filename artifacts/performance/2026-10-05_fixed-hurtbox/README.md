# 2026-10-05 Hurtbox test decided in fixed point; deferred effect descs

Owner, 2026-10-05: "Software floating point should not exist, fixed point
only."

`build-lab-clean1005z` = `build-lab-clean1005y` (`dac7fea37d8`) plus:

1. **The hurtbox test decided in fixed point** (`src/port/nds_p2_hurtbox_reject.c`
   `ndsP2HbDecidePoints`, wired into `gmCollisionCheckFighterAttackDamageCollide`
   and the weapon and item attack wrappers, `src/import/battleship_gmcollision.c`).
   Where the separation tests cannot prove a miss, the source's float tail
   (`func_ovl2_800EDE00`/`800EDE5C`, `gmCollisionTestRectangle`) decided; now the
   reject's cached fixed world takes the source's cofactor frame at Q26 (hardware
   divider) and the source's clip runs on Q12 points (the R2-07 slice 53 kernels).
   A value outside the fixed guards still falls to the float tail. A/B word
   `gNdsP2HbNarrowFixed`.
2. **Deferred effect descriptors** (`src/import/battleship_efmanager.c`): every
   effect maker retried every deferred desc -- the other fighters', whose files
   never load -- each a file span, token chain and loaded-file scan. A slot is
   retried only when the loaded-file table (`ndsRelocLoadedFilesEpoch`, new) or the
   desc's file slot changed; the per-construction offset mapping is memoized on
   the same key.

Clean four-CPU lab, Dream Land, items on:

| Pair | P50 | P95 | P99 | over | digest |
|---|---|---|---|---|---|
| gate roster `gate-y` -> `gate-z` | 812,672 -> 811,712 | 1,119,424 -> 1,115,072 | 1,393,216 -> 1,384,064 | 95 -> 93 | identical |
| 4 x Yoshi `yo-y` -> `yo-z` | 913,920 -> 912,704 | 1,324,160 -> 1,332,416 | 1,554,752 -> 1,566,208 | 316 -> 317 | 15 frames differ from 1,887 |
| 4 x Pikachu, forced Thunder `pkdb-y2` -> `pkdb-z` | 762,496 -> 762,368 | 1,437,696 -> 1,376,256 | 1,845,888 -> 1,794,176 | 336 -> 313 | identical |

On the gate roster the fixed decision reproduced every float outcome (digest
identical). Yoshi's motions set `is_use_animlocks`, which every fixed path
declined, so these pairs barely touch Yoshi (the late digest difference is a
lock-free motion deciding at a face).

Forced Thunder's -61K at P95 is the deferred-desc retry: its heavy frames make a
trail effect per Pikachu every tick.

## Animation-lock chains in fixed point (`build-lab-clean1005lk`)

`build-lab-clean1005z` plus:

3. **The hurtbox walk for animation-lock fighters** (`ndsP2HbLocalLock`,
   `ndsP2HbWorldOfLock`): gmCollisionSetMatrixNcs's chain -- the local's rows
   times the accumulated scale, its columns divided by the parent's -- at Q30
   rotation / Q16 scale, the per-column 1/scale from the hardware divider, cached
   with the plain walk's slots. The fixed decision, the joint world position and
   the held-item attach now cover Yoshi.
4. **The lean renderer's animation-lock local** (`ndsFtrLeanLockLocal`,
   `src/port/renderer_fighter_lean.c`): lbCommonMatrixTraRotScaInv's integer
   form, the accumulated scale held at Q16 in the kernel (`NDS_FTR_LEAN_ACCUM_ONE`),
   replacing the float local builder (59.5K ticks a frame on 4 x Yoshi in the
   census).

| Pair | P50 | P95 | P99 | over | digest |
|---|---|---|---|---|---|
| gate roster `gate-z` -> `gate-lk` | 811,712 -> 811,648 | 1,115,072 -> 1,116,352 | 1,384,064 -> 1,370,816 | 93 -> 91 | identical |
| 4 x Yoshi `yo-z` -> `yo-lk` | 912,704 -> 889,856 | 1,332,416 -> 1,275,200 | 1,566,208 -> 1,525,696 | 317 -> 248 | from frame 305 |

4 x Yoshi: P95 -57K, two-VBlank misses -69. The digest leaves at frame 305 on
low-order bits (the joint world position now comes from the fixed chain): the
captures at frames 200, 300 and 500 (`artifacts/visibility/2026-10-05_playtest`
`yo-z-*` / `yo-lk-*`, local only) show the same positions and the same damage
(36/32/36/33% at frame 500): no hit changed through frame 500. Owner ruling
D13 re-baselines it.
