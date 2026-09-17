# The second roster ever measured fails native rendering 7,679 times

> **CORRECTION, and it is the headline.** This document originally read the
> alternate roster's 116,992-tick advantage as evidence that roster composition
> swings frame cost. **It is not.** That arm produces **7,679 native rendering
> failures** against the canonical roster's **0**, and is missing Luigi's
> ShieldPose asset. It is cheaper partly because it draws less and partly
> because it is **broken**. No cost conclusion can be drawn from it. The
> measurement and the harness findings below stand; the cost interpretation
> does not.


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

The 116,992 is real as a *measurement* and meaningless as a *roster cost*: see
the correction above and the native-failure section below. The alternate arm is
missing content and failing to render. What is worth noting is that FTR, STG and
SRC are each within noise of the canonical roster, so whatever that arm is not
doing does not show up in any named bucket — it is spread through the unnamed
remainder, which is also where its 7,679 failures live.

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
squarely inside P2-3's open acceptance.

Combined with the 7,679 native failures below, **no roster-cost conclusion
survives from this pair.** Whether the canonical roster is the most expensive
legal lineup remains unknown, and answering it needs a roster that is both
content-complete and rendering natively.

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

## The native-render failure, which is the real finding

| counter | canonical | Captain/Luigi/Donkey/Kirby |
|---|---:|---:|
| `gNdsRendererNativeFailure.count` | **0** | **7,679** |
| domain / scene / reason | 0 / 0 / 0 | 1 / 22 / 2 |

`AGENTS.md` requires every ROM, including diagnostics, to be native-only with no
fallback switches and no missing required content. A legal four-fighter lineup
producing 7,679 native failures is therefore a **P2 correctness gap**, and it is
worth more than any tick figure in this document.

It went unseen because the gate that detects it could not report it. With
`nativeFailureCount` nonzero and no direct reject recorded,
`nativeDirectRejectSite` is 0, `(0 -band -bnot 1) - 1` is -1, and the `x` format
specifier throws — so the run ended in *"Error formatting a string: Format
specifier was invalid"* instead of naming 7,679 failures. **A gate that finds
the defect and then hides it.** Reproduced exactly and guarded.

## What this does and does not change

**Does not change** the recommendation. 468,544 remains the gap for the measured
roster, and no implementable class exceeds ~11% of it.

**Does change** what must happen before any lane is accepted. The full matrix
has to run, the harness is now capable of running it, and the first roster it
could reach is **failing native rendering** — so the contract is not merely
unproven, it is currently **failing** on the one extra sample taken. Any future
claim that a lane "closes the gap" must name its roster, and a roster that does
not render natively cannot be used to price anything.
