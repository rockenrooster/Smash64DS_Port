# Task 63 — Dream Land constrained reduction: E0 census

**Date:** 2026-07-25
**Branch:** `codex/task57-dreamland-world-mesh`
**Plan:** `DreamLand_DS_Native_3D_Stage_Compiler_Plan.md` (Revision 2), Task 63 E0
**Outcome:** **STOP at E0.** The reduction available at full material fidelity
is **9.1%**, against a 15% STOP threshold. The runtime repair was not
performed. `NDS_DREAMLAND_DS_MESH` stays 0; the published ROM is untouched.

Host-only. No build, no emulator, no runtime code changed by this task.

Reproduce:

```bash
python scripts/dreamland_e0_census.py --json scripts/generated/dreamland_e0_census.json
```

## Charter

Revision 2 Part IV gates the runtime repair behind a census:

> Under Part II's constraints, re-derive the candidate ladder and report
> submitted corners, per-submit-class breakdown, and reduction vs 525. **If no
> candidate clears ~15% reduction at full material fidelity, STOP and report.**

The constraints being measured against are Part II.1 (every emitted corner
resolves to exactly one source dense vertex — subset placement only) and
Part II.5 (raw-Z/range geometry may be collapsed; projected-no-Z alpha cards
are drop-only, never re-indexed and never moved).

## 1. The stage decomposes cleanly, and only one part is eligible

| submit class | triangles | where |
|---|---|---|
| raw-Z (0) | 66 | binding 29, runs 35/36/37 — the 3D island and platforms |
| range/matrix (6) | 10 | binding 29, runs 38/39/40 — full-width water/ground spans |
| projected-no-Z (3) | 99 | **26 other bindings, 33 runs, 29 texture epochs** |

The 99 no-Z triangles are spread across 33 runs whose sizes are
`[1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,4,5,5,6,6,12,12]`. Ten
runs are a **single triangle**. These are individual alpha-textured backdrop
cards, each already at its minimum representation — there is no topology to
simplify, only whole cards to delete.

So the entire eligible-for-collapse subset is binding 29's **76 triangles**.

## 2. The lossless lever is empty — 0 triangles

Measured against **two independent fixture sets** at **four screen margins**
(0%, 10%, 25%, 50%):

| fixture set | never-visible triangles |
|---|---|
| Task 58 envelope-derived (8 views) | 0 at every margin |
| gameplay-surface-derived (8 views) | 0 at every margin |

Degenerate (zero-area) triangles: **0**.

Task 58's fixtures turned out to be badly framed — they derive the camera from
the *whole* static envelope, which includes the sky dome (y up to 3816) and the
background cards (x ±3249). That put the look-at point at y≈2251, empty sky
well above an island that lives at y 790–1543, and pushed the eye 1.6× further
out than gameplay framing. This task added a second fixture set framed on the
raw-Z fighting surface (`gameplay_fixtures()`), which is 1.6× closer and looks
at y≈1355. **Both agree: nothing is off-screen.** Dream Land's static mesh has
no redundant geometry to delete for free.

## 3. The island is almost entirely silhouette

The collapsible subset is 76 triangles over 86 vertices — of which
**68 are boundary vertices and only 18 are interior**. There is very little
interior to collapse into.

Collapse ladder (greedy quadric, subset placement, never merging across a
`(run, texture_epoch)` seam). IoU is measured **on the collapsible subset
only** — see §4 for why that matters:

```text
boundary_locked (island outline protected)
  budget  subset  total  corners  reduce  subsetIoU
      60      60    159      477    9.1%     0.9984  OK
      55      54    153      459   12.6%     0.9035  reject
      50      50    149      447   14.9%     0.8311  reject
      45      48    147      441   16.0%     0.7996  reject   <- floor
      40      48    147      441   16.0%     0.7996  reject
      35      48    147      441   16.0%     0.7996  reject

unlocked (outline free to move)
      60      60    159      477    9.1%     0.7109  reject
      45      45    144      432   17.7%     0.3536  reject
      30      30    129      387   26.3%     0.3012  reject
      25      25    124      372   29.1%     0.3012  reject
```

Two things to read off this:

- **With the outline protected, 9.1% is the ceiling** (525 → 477 corners) and
  the silhouette collapses immediately past it: 0.998 → 0.904 → 0.831 over the
  next two rungs. The ladder then floors at 48 triangles because everything
  remaining is locked.
- **The protection is doing real work.** At the same 60-triangle budget,
  locking the boundary scores 0.998 and unlocking it scores 0.711. All of this
  stage's fidelity lives in its outline.

**9.1% < 15%. The E0 gate fails.**

## 4. Why Task 59's oracle green-lit a destroyed island (quantified)

The same collapse candidate, measured two ways:

| candidate | IoU on the collapsible subset | IoU on the full mesh |
|---|---|---|
| unlocked, 60 triangles | **0.7109** | 0.9986 |
| unlocked, 30 triangles | **0.3012** | 0.9976 |

A candidate whose island silhouette is **70% wrong** scores **0.9976** on the
full mesh — comfortably above the 0.95 gate that Task 59 and Task 60 used to
declare c120 "visually acceptable."

The mechanism: the 99 untouched no-Z backdrop cards dominate the rasterized
screen mask, and they are identical in both arms by construction. The metric
therefore measures mostly the part that did not change, and is nearly blind to
the part that did. Task 59 cut raw-Z from 66 to 30 in exactly this way and
reported IoU 0.959.

This is the quantitative root cause of the Task 62 REVERT, and it confirms
Revision 2 Part II.2 (a geometric metric may never serve as the gate). It also
yields a concrete rule for any future stage work: **measure the metric on the
subset that changed, never on the whole frame.**

## 5. The one remaining lever, priced

Everything above concerns reduction at *full* fidelity. There is a second
lever, but it is a fidelity trade that only the owner can authorise: deleting
whole no-Z backdrop cards.

Per-run mean screen coverage across the 8 gameplay fixtures, cheapest first:

```text
 run  tris  corners  mean_screen_cover%   cum_corners  cum_reduction
   7     1        3        0.11                   3        0.6%
   6     1        3        0.12                   6        1.1%
   9     2        6        0.12                  12        2.3%
   8     2        6        0.13                  18        3.4%
  17     1        3        0.22                  21        4.0%
  18     1        3        0.25                  24        4.6%
  51     2        6        0.31                  30        5.7%
  50     2        6        0.32                  36        6.9%
  43     6       18        0.34                  54       10.3%
  19     1        3        0.37                  57       10.9%
  20     1        3        0.49                  60       11.4%
  22     1        3        0.67                  63       12.0%
  21     1        3        0.77                  66       12.6%
  24     1        3        1.05                  69       13.1%
  53    12       36        1.05                 105       20.0%
  25     1        3        1.05                 108       20.6%
  ... remaining runs cover 1.45% to 15.34% each
```

Dropping the 16 cheapest runs is **20.6%** on its own, and composes with the
safe 9.1% island collapse for **≈29.7%** (525 → 369 corners).

Two honest caveats:

1. **These are visible, not hidden.** 1% screen coverage is roughly a 28×28
   pixel region — small, but plainly there. This is deleting scenery, not
   culling waste.
2. **Do not identify these cards by the coordinates in the Task 57 IR.** That
   IR bakes descriptor world matrices rather than live DObj transforms, so its
   positions are unreliable (this is the same defect that put the pass-through
   platforms near ground level in Task 62). Identifying what each run actually
   draws requires a capture with that run suppressed.

Whether ≈29.7% would move `ALL`/P95 is itself unproven. Task 55 measured a
20.6% *non-vertex* word cut and `ALL` did not move at all; the counter-argument
is that Tasks 54/55 localized the floor to the vertex words specifically, which
is what this would cut. It is a real shot, not a sure thing.

## 5b. The remaining lever, measured on hardware — it does not work

§5 priced the scenery-deletion lever on paper. This section measures it. A
lab-only flag `NDS_DREAMLAND_CARD_CULL` (default 0) bakes a 64-bit run mask
that suppresses whole stage runs, so the owner can see and measure the trade
instead of reasoning about a coverage table.

Four arms, one tree, `smash64ds-battle-playable-proof-hwtri`, canonical
Boundary configuration (mode 163, Dream Land, Mario vs level-3 CPU Fox), frame
window 438–445:

| arm | flag | culled runs | ROM sha256 (16) |
|---|---|---|---|
| A | 0 | — | `af8b2a5023676bcf` |
| A0 | 1, mask 0 | none (instrument control) | `04633d8624bf5537` |
| B | 1 | cheapest 10 | `f4db7b140b35f185` |
| C | 1 | cheapest 16 | `235c1f0d8505a9f6` |

### The instrument is proven neutral

- Arm A's ROM hash `af8b2a50…` (11,432,960 B) is **byte-identical to the Task 62
  A/B baseline arm** built before this flag existed. At default 0 the change
  compiles out completely; the override-trap holds.
- Arm A vs arm A0 frame-438 capture: **0 changed pixels** across the entire 3D
  viewport, with identical counters (828 triangles, 1152 GX words). Flag on
  with an empty mask renders exactly like flag off, which is what makes the
  B/C comparisons trustworthy.

### The cull engaged exactly as designed

| arm | stage triangles | Δ | predicted Δ | GX words | Δ |
|---|---|---|---|---|---|
| A0 | 828 | — | — | 1152 | — |
| B | 809 | −19 | −19 ✓ | 1038 | −9.9% |
| C | 792 | −36 | −36 ✓ | 936 | −18.8% |

### Visual cost

Frame-438 pixel diff over the 3D viewport (110,592 px):

- A0 vs B: **2.15% changed**
- A0 vs C: **2.15% changed — the identical pixel set**

The six extra runs in C contribute nothing at the gameplay camera, so C buys
17 more triangles for zero additional visual cost. What is lost in both: small
flower/bush clusters at ground level near the fighters, and a region of the
pond edge in the lower right. Captures:
`2026-07-25_task63-cull-{A,A0,B,C}-frame438.png`,
`2026-07-25_task63-cull-diff-{control,B,C}.png`,
`2026-07-25_task63-cull-sidebyside.png`.

### Performance — the lever is worse than neutral

RENDER_BENCH, steady-state frames 440–445 (P50):

| arm | ALL total | Δ | stage CPU (FTR/STG) | Δ |
|---|---|---|---|---|
| A0 | 1,352,000 | — | 1,011,584 | — |
| B | 1,358,464 | +0.5% | 1,184,512 | **+17.1%** |
| C | 1,357,184 | +0.4% | 1,165,376 | **+15.2%** |

`ALL` is **flat**, and stage CPU work gets materially **worse** — consistently
across all 8 frames with very low variance (B: 1,179,008–1,185,344).

The mechanism is visible in the counters: `RENDER_TEXHASH` jumps from 25 to 58
and `RENDER_TEXEL1` goes from `0,0` to `2,2` in both cull arms. Removing runs
breaks the Task 44 stage steady-state / static-texture-AOT coherence and
re-enters live frozen-water TEXEL0/TEXEL1 material evaluation every frame. That
re-evaluation costs ~+170,000 ticks, which dwarfs the 216 GX words the cull
saves.

**Honest caveat:** this regression is plausibly an artifact of culling inside
the run loop rather than rebuilding the run table offline in the generator; a
generator-level cull would likely keep the texture caching intact. But `ALL` was
flat regardless, and an 18.8% vertex-word cut is the same order as Task 55's
20.6% word cut, which moved `ALL` by zero. Both readings point the same way.

**Verdict: the last lever is closed.** Even granting the scenery loss, deleting
backdrop cards does not buy frame time.

## 6. Disposition

- **E0 gate: failed.** 9.1% available vs 15% required. The runtime repair
  (defects A/B/C in the Revision 2 plan) was **not** performed — fixing a path
  that can at best deliver 9.1% is not a good trade against the risk.
- `NDS_DREAMLAND_DS_MESH` remains **0**. Published ROM untouched. The
  `check-published-roms.ps1` payload guard remains in force.
- The in-flight runtime path stays committed at `dc60d53d9` in its **known-broken**
  state, default-off and documented there. It must not be enabled: 328 of 360
  emissions read the wrong source vertex, the count gate rejects 20 of 120
  triangles, and the no-Z path ignores candidate topology entirely.
- Tasks 64, 65 and 66 are **not entered**.

## 7. What this settles for the campaign

Combined with Tasks 53–55, the stage-geometry axis is now closed on measurement
rather than on guesswork:

- Task 54: `STG+OTHR` ≈ 720K is invariant to CPU-work removal — GX-throughput-bound.
- Task 55: cutting 20.6% of *non-vertex* words moved `ALL` by zero.
- Task 63 (this): cutting *vertex* words at full fidelity yields at most 9.1%.

- Task 63 §5b (this): deleting backdrop cards leaves `ALL` flat and makes stage
  CPU work 15–17% *worse*.

Every lever on this axis is now measured and closed. **Dream Land's stage cost
is at its floor; the next 30 FPS win is somewhere else.** The campaign should
move off stage geometry entirely.

## Deliverables

- `scripts/dreamland_e0_census.py` — the census instrument (visibility cull,
  planarity/structure census, constrained subset-placement collapse ladder,
  subset-restricted IoU, priced no-Z drop table).
- `scripts/generated/dreamland_e0_census.json` — the machine-readable report.
- This document.
