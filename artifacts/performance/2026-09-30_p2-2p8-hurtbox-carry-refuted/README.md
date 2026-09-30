# P2-2p8 2026-09-30: carrying hurtbox worlds across epochs (REFUTED, parked)

Patch kept: `hurtbox-carry-v3.patch` (against `1246ddfe5a1`,
`src/port/nds_p2_hurtbox_reject.c`). Not committed to source.

## Idea

The hurtbox reject kernel rebuilds every tested joint's fixed-point world each
latch epoch (BuildLocal + Compose + walk, ~2,500 cycles a level, ~62% of the
kernel on the gate profile `p2p8-prof-gate9`). Body joints are posed only on the
last source tick of a presented frame, so on the other tick their inputs repeat.
`ndsR2CfxCompose`'s rotation rows read only the parent's rotation and the
local's, and its translation row is `Shr(P.r . L.t) + P.t`, so when a joint's
rotation inputs and its parent's rotation are the ones its slot was built from,
the world is the old one with the translation moved by exactly the parent's
change (or that one row recomputed). The carry keyed each slot on the local's
inputs (rotate/scale/translate bits, or the cached float local's cells under
`transform_update_mode`), named each world's rotation with a stamp only a full
build renews, and moved the translation. A-B word `gNdsP2HbCarry` (0 off, 1 on,
2 = carry AND rebuild, count differences).

## Correctness: exact

`gNdsP2HbCarry=2` on all nine lab stages: `gNdsP2HbCarryMismatches` 0 for every
version below; rejects, passes and box hits identical to carry off.

## Performance

Three versions, same-ROM A/B (carry 0 vs 1), carries as a share of world
builds:

| Version | Slots | Carries | Lab P95 (Sector Z / Dream Land / Saffron / Zebes) | Gate |
| --- | --- | --- | --- | --- |
| v1: TRS locals only | 64, pointer hash | 5-10% | -1.0 / +0.9 / +2.0 / +2.9K | P95 flat |
| v2: + cached float locals | 128, pointer hash | 11-27% | -1.8 / -3.8 / -2.6 / -8.4K | P50 -1.2K, P95 flat |
| v3: + one slot per (player, joint_id) | 160 | 39-47% | -6.1 / -12.2 / -19.4 / -15.4K | P50 -2.4K (4/4), P95 flat (4 pairs) |

Census that drove v2 and v3 (lab Dream Land, v1 then v2): rotation inputs
rarely move on a held tick (164 levels a match); failures were cached-float-
local joints (4,207, always rebuilt with a new stamp) and their descendants
(6,250), then direct-mapped slot conflicts between fighters (1,329 of 7,234
held-tick levels) and their descendants (2,207).

**Cross-build (the shipping question), both with the same tree otherwise:**

| Build | Gate P50 / P95 | Saffron P95 | Zebes P95 |
| --- | --- | --- | --- |
| HEAD kernel | 908,416 / 1,221,952 (4 runs) | 1,435,648 | 1,413,376 |
| v3 carry on | 908,176 / 1,226,608 (4 runs) | 1,435,264 | 1,418,944 |

The same-ROM wins are measured against the carry-off arm of the same ROM, which
pays the machinery (key reads and 17 words of key/stamp stores per build, the
FTParts read ahead of every slot, a 132 B slot, 21 KB of BSS; heap low-water
340,140 -> 321,964 on the gate) and reads ~19K worse than the HEAD kernel on
Saffron. Net of that, no stage improves; the gate P95 is 4.7K worse.

## Why the gate cannot move here

Within the v3 ROM the gate's P95 did not move with 39% of builds carried, while
its P50 fell 2.4K: the kernel's heavy frames are not the frames that set the
gate's P95. The kernel is ~33K by counterfactual removal (`cf95`), but a partial
cut reorders frames under the P95 line rather than lowering it.

## If reopened

The carry is only worth it if its bookkeeping is nearly free: keys compared
from data the build reads anyway, no per-build key stores beyond the rotation
inputs, a slot of two cache lines, and no FTParts read for already-current
slots. Held-tick coverage is the ceiling (~50% of builds).
