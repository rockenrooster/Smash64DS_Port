# 2026-10-05 Stage animation in fixed point; the wall sweep's integer box (Q1, Q2)

Owner, 2026-10-05: "Software floating point should not exist, fixed point
only." Ruling D13 re-baselines the digest. Clean four-CPU lab
(`NDS_LAB_FOURCPU_WORDS=1`), items on; WORK-H from the ring dump, frames >= 64
(`runsum.py`); paired medians from `pairab.py`. Bases: `gate-r2`, `sz-r2`
(`2026-10-05_recorder-deleted`), `yo-r1` (same program as R2).

## Q1 (`build-lab-clean1005q1`): stage and item joint AObjs in Q

`src/import/battleship_sys_objanim.c` now defines the event32 parser
(`gcParseDObjAnimJoint`): every rotate, translate and scale track it writes
is a Q kind -- the representation the fighter parser has used since
Requirement 4 -- so the player's per-node length add, Linear, Step and the
cubic run in integers. The DObj clock (anim_wait, anim_frame, anim_speed)
stays f32 because stage code reads it, and so does the path-parameter track
TraI (Q12 of a path's [0, 1] would step Sector Z's Arwing a couple of units).
`gcGetAObjValue` reads either form (Sector Z's Arwing basis calls it). A/B
word `gNdsStageAnimQ`.

| Pair | P50 | P95 | over | digest | paired median |
|---|---|---|---|---|---|
| gate `gate-r2` -> `gate-q1` | 816,448 -> 814,080 | 1,108,928 -> 1,105,280 | 89 -> 88 | identical | -2.0K |
| Sector Z `sz-r2` -> `sz-q1` | 902,336 -> 896,768 | 1,228,032 -> 1,220,032 | 202 -> 195 | identical | -5.1K |
| 4 x Yoshi `yo-r1` -> `yo-q1` | 893,568 -> 891,200 | 1,264,448 -> 1,263,232 | 244 -> 244 | 146 frames from frame 54 | -- |

The replay digest folds no stage DObj, so the gate and Sector Z digests do
not move. 4 x Yoshi's moves on 146 of 1,960 frames, from frame 54, and
returns to the base between them -- transient state (an item or effect DObj
while it lives), not a fighter's lasting divergence; not traced further.

## Q2 (`build-lab-clean1005q2`, base Q1): the wall sweep's integer box

`src/port/reloc_backend_mp_collision.c`: before a wall segment's surface test
(tilt or flat), its integer x span is tested against the motion's
`[range_lo, range_hi]` (one unit of slack for the tests' own +-0.001) and its
y span against the motion's floor/ceiling y span. A segment outside fails
both surface tests' own bound checks, so the skip is exact.

| Pair | P50 | P95 | over | digest | paired median |
|---|---|---|---|---|---|
| gate `gate-q1` -> `gate-q2` | 814,080 -> 813,504 | 1,105,280 -> 1,107,328 | 88 -> 90 | identical | -0.2K |
| Sector Z `sz-q1` -> `sz-q2` | 896,768 -> 895,616 | 1,220,032 -> 1,219,584 | 195 -> 192 | identical | -1.7K |
| 4 x Yoshi `yo-q1` -> `yo-q2` | 891,200 -> 891,328 | 1,263,232 -> 1,261,184 | 244 -> 237 | identical | -0.4K |
