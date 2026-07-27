# Task 94 — The best remaining placement move regressed, and it retires the estimator that recommended it

**Date:** 2026-07-26
**Status:** **REVERTED.** `WORK-H` P50 **+6,144**, P95 **+9,280**, worse on
**122 of 128 frames**. Fourth consecutive placement regression (87, 88, 89, 94).
**Inputs:** `artifacts/task94-A.json`, `artifacts/task94-B.json`.

## 1. Why this was worth one A/B

I had dismissed this move earlier in the session using Task 83's correction —
the non-mem-stall "in reach" metric is ~18× optimistic per symbol, so
`gcPlayDObjAnimJoint`'s 8,931 ticks/frame "in reach" implied ~500, below the
noise floor.

That was the wrong arithmetic: Task 83's 18× correction applies to the *in-reach*
metric, and I applied it to a move I then re-sized by a different route. The
direct estimate is the tier ratio — own cost × (target rate / current rate):

```
own cost              33,900 t/f at 2.19 cyc/insn   (census section D)
at .itcm tier rate    26,005 t/f at 1.68
predicted saving       7,894 t/f
```

~7,900 is worth one build and one A/B, so it got one. Every other argument was
in its favour too:

- **Largest soft-float caller in the frame** — Task 92 E0 measured
  `gcPlayDObjAnimJoint` at 54.2% of all `fadd`/`fmul` calls.
- **Top-ranked zero-eviction admission** on the Task 81 census.
- **Nothing displaced** — 500 bytes against 720 free, so unlike Task 89's
  draw-hot refill it evicted no resident.
- **Verified to have actually moved** — `0x020013c0` → `0x01fff424`, confirmed by
  `nm` on both arms before sampling.

The mechanism: `gcPlayDObjAnimJoint` is decomp-owned and cannot carry a placement
attribute, but `linker/nds_hot_text.ld` already admits it to `.text.hot` by name,
so moving that one line into the `.itcm` block is the whole change.

## 2. Result

| | A | B | Δ |
|---|---|---|---|
| `WORK-H` P50 | 1,320,768 | 1,326,912 | **+6,144** |
| `WORK-H` P95 | 1,733,888 | 1,736,192 | **+9,280** |
| `FTR` P50 | 543,552 | 545,856 | +2,304 |
| `STG` P50 | 370,048 | 373,760 | **+3,712** |
| VBlank 3-interval | 510 | 509 | −1 |

Per-frame: **6 improved, 0 unchanged, 122 worse**; median **+5,696**. A delta
holding its sign on 122 of 128 frames is a mechanism, by the same rule that
accepted Task 90 — it just points the wrong way.

## 3. The tell, and what it retires

**`STG` rose 3,712 in an arm where the stage never calls the moved function.**
Nothing about the stage path changed except the addresses of the ten other
functions still pinned in `.text.hot`, which all shifted 500 bytes when
`gcPlayDObjAnimJoint` left.

That is the whole explanation, and it retires both sizing estimators:

| estimator | predicted | actual | verdict |
|---|---|---|---|
| non-mem stall "in reach" | ~500 (after Task 83's 18×) | +6,144 | already known loose; sign wrong too |
| **tier cyc/insn ratio** | **−7,894** | **+6,144** | **sign wrong — retired** |

Both price *the symbol that moves* and ignore *the space it vacates*. Task 83
already measured that the vacated space is where the value lives — 69% of Task
82's win was not in the symbols that moved — and neither estimator models it.

A curated fixed-size section is a **working set, not a list**. Removing a member
re-addresses every other member, and at this point the layout term dominates
whatever the moved symbol gains.

## 4. Placement is closed

Tasks 87, 88, 89 and 94 all regressed. Task 82 was the last placement move that
paid, and Task 83 said at the time that the cheap half was taken; four
independent failures since have confirmed it from four different directions
(inlining more copies, removing redundant clears, refilling draw-hot, admitting
to ITCM).

**Do not propose another placement move without a new mechanism** — a new
*candidate* is not a new mechanism. The rule is recorded in
`TASK_STANDING_RULES.md`, and the reason is recorded in `nds_hot_text.ld` beside
the line that came back, so the next person to notice `gcPlayDObjAnimJoint` at
the top of a census ranking finds the result before spending the build.

## 5. State after this task

`WORK-H` P95 1,733,888 against the 1,120,000 gate: **613,888 over**.

Every direction the frame offers is now closed with a measurement:

| direction | closed by |
|---|---|
| texture memo | Task 93 — 22 distinct keys in 25 binds |
| soft-float conversion | Task 92 — 73% state-hash frozen |
| dense-vertex re-shade | Task 90 — 0.0% redundant |
| animation compiler as scoped | Task 77 E1 — cosmetic joint set empty |
| `mem*` micro-fixes | Tasks 87/88 |
| **placement** | **Tasks 87/88/89/94** |

The single remaining lever is animation at ~183,564 ticks/frame, re-scoped by
Task 92 §5 to exactness-preserving layout work on the channel loop — flat
contiguous arrays replacing the `aobj->next` walk, precomputed traversal order,
hoisted loop invariants. Task 94 is evidence *for* that framing, not against it:
the animation cost is not where the code sits, it is what the code walks.
