# Roster swings the frame by 116,992 — and the contract cannot currently be measured

Every tick figure in P2-2p8 is measured on **one** roster (Donkey/Samus/Link/
Kirby) on **one** stage (Dream Land), while the contract is *every legal
four-fighter lineup on every selectable VS stage*
(`docs/P2_PLAN.md`, `p2/native-optimization/16_ALL_ROSTERS_ALL_STAGES.md`).

This tests whether that matters. It does, in two separate ways.

## The measurement

| roster | WORK-H P50 | ALL P50 | FTR P50 | STG P50 | SRC P50 |
|---|---:|---:|---:|---:|---:|
| Donkey/Samus/Link/Kirby (pinned) | **1,588,544** | 1,677,952 | 353,280 | 345,600 | 553,920 |
| Captain/Luigi/Donkey/Kirby | **1,471,552** | 1,677,888 | 357,056 | 342,656 | 555,136 |
| **delta** | **−116,992** | −64 | +3,776 | −2,944 | +1,216 |

**A legal roster change moves WORK-H by 116,992 ticks — 7.4% of the frame, and
larger than all but one optimization this campaign has measured.** Yet FTR, STG
and SRC are all within noise of each other, so the difference is not in the
named buckets; it is spread through the unnamed remainder.

Gap against the 1,120,000 gate: **468,544** for the pinned roster against
**351,552** for this one — 25% smaller.

## The first caveat, and it is disqualifying for a clean comparison

**This is not a like-for-like roster cost.** The alternate roster loaded **3
shield poses for 4 fighters** — 27 native fixups instead of 36, 9,235 resident
bytes instead of 11,799 — with `gNdsShieldPoseLoadFailCount = 0`, so nothing
failed. Only three were ever asked for.

`include/nds/generated/nds_shield_pose_assets.generated.h` has
`NDS_SHIELD_POSE_ASSET_COUNT 7`, covering Donkey, Samus, Link, Kirby, Captain,
Pikachu and Purin. **Luigi is not in the table** — and neither are Mario, Fox,
Yoshi or Ness.

So part of the 116,992 is simply Luigi having less native content to draw. This
is a **pre-existing content gap**, not a defect introduced here, and it sits
squarely inside P2-3's open acceptance. But it means the honest reading of this
measurement is:

> **The pinned roster is the more expensive of the two, so the 496,382 gap is
> not optimistic for it — but roster cost varies by at least 117,000, part of
> which is missing content rather than genuine roster weight.**

## The second finding, which is the actionable one

**The four-CPU stress harness cannot validate any roster but the pinned one.**
`scripts/verify-p2-four-fighter-stress.ps1:575-585` asserts

```
gNdsShieldPoseLoadCount == 4
gNdsShieldPoseResidentBytes == <constant for Donkey/Samus/Link/Kirby>
gNdsShieldPoseNativeFixupCount == 36
```

and throws *"did not match the selected Donkey/Samus/Link/Kirby contract"*. The
measurement completes first — every bucket figure above is valid — but the run
exits non-zero on a residency expectation that is hard-coded to one lineup.

**The any-roster/any-stage contract therefore cannot be measured today without
parameterising these assertions per roster.** That is a gap in the contract
tooling, and it is worth more than this single data point: it means the
"no universal PASS from one roster" warning on the board is not merely unproven,
it is currently *unprovable*.

## How the arm was built

`NDS_LAB_FOURCPU_KINDS` (Makefile), four space-separated BattleShip `fttypes.h`
ordinals. Empty by default, so the pinned capacity roster and every normal build
are unchanged — verified: a default build regenerates
`NDS_P2_FOUR_CPU_KIND0..3 = 2,3,5,8`. Only kinds this target already admits
(Captain, Donkey, Kirby, Link, Luigi, Samus) can be instantiated;
`nds_match_config.c` refuses any other.

The pinned four were chosen as the **capacity** argmax — the set whose resident
bytes the memory row judges. **Capacity argmax is not frame-cost argmax**, and
nothing had established which roster is the most expensive in ticks. This still
does not: two samples of a large space, one of them content-incomplete.

## What this does and does not change

**Does not change** the recommendation. 468,544 remains the gap for the measured
roster, and no implementable class exceeds ~11% of it.

**Does change** the confidence interval around it. A ±117,000 roster term is
larger than every candidate except the withdrawn 30 Hz simulation, so any future
claim that a lane "closes the gap" must name its roster — and the full matrix
must eventually be run, which needs the harness fixed first.
