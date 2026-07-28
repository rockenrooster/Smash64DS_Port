# Task 95 — The animation hoist works and the frame still gets worse

**Date:** 2026-07-26
**Status:** **REVERTED** (both arms). The mechanism is real and measured; the
whole-frame gate moves the wrong way regardless.
**Inputs:** `artifacts/task94-A.json` (baseline), `artifacts/task95-B.json`,
`artifacts/task95-C.json`.

## 1. What was built

Task 92 §5 re-scoped Task 78 to exactness-preserving layout work, and Task 94
established the blocker: the hot call to `gcPlayDObjAnimJoint` is *internal* to
`objanim.c` (inside `gcPlayAnimAll`), so one `#define` renames the definition and
that call together and a port-side replacement links but never runs.

This task went through that blocker rather than around it:

1. `#define gcPlayDObjAnimJoint ndsBaseGcPlayDObjAnimJoint` in the import shim,
   freeing the name.
2. A port-side joint player: the decomp body **verbatim**, with three
   loop-invariant reads hoisted out of the `AObj` walk — `dobj->anim_wait !=
   AOBJ_ANIM_END`, `dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM` (a dependent
   two-load chain), and `dobj->anim_speed`. All three are invariant because the
   loop writes only `aobj->length` and `dobj->rotate/translate/scale`; the
   compiler cannot prove that through the `AObj` pointer and reloads all three
   every iteration.
3. A port-side traversal so the replacement is reached, built on
   `gcGetTreeDObjNext` — which `objanim.c:11` defines as **exactly** the
   `child`/`sib_next`/`parent` sequence `gcPlayAnimAll` walks inline. Traversal
   order is identical by construction, not by inspection.

**The interposition mechanism is now proven end to end** — built, linked, booted,
and measured. That was the open question after Task 94 and it is answered.

## 2. Two arms, both regressions

| arm | `WORK-H` P50 | `WORK-H` P95 | `FTR` P50 | `FTR` P95 | WORK-H worse on |
|---|---|---|---|---|---|
| **B** exported body | +1,920 | **+35,008** | **−2,240** | **−6,400** | 80/128 |
| **C** static body + forwarder | **+7,872** | +2,944 | +768 | −5,888 | 120/128 |

**The hoist itself works.** Arm B improved `FTR` on **98 of 128 frames**, median
−2,688. That is the mechanism doing exactly what Task 92 predicted: the animation
class is 68.4% stall and removing three per-node reloads takes stall out.

**And the frame still got worse.** Arm B's `FTR` gain was swamped by a P95 tail
of +35,008 — exporting the body cost the traversal its inlining. Arm C restored
`static` and added a thin forwarder for the external callers
(`ftParamUpdateAnimKeys`, `ndsBaseFTCommonGuardUpdateJoints`); that recovered the
tail (+2,944) and lost the median instead (+7,872, worse on 120/128).

There is no third arm worth building. The two failure modes trade against each
other: the body must be inlinable for the traversal and addressable for the
external callers, and each choice costs roughly what the other saves.

## 3. The collateral, which is the real finding

`STG` rose in **both** arms — 370,048 → 373,440 (B) and → 375,104 (C) — from a
change that touches only the animation path.

That is the same signature Task 94 found when `STG` rose 3,712 after moving a
function the stage never calls. Editing a hot shared translation unit
re-addresses its neighbours, and **at this local optimum the re-addressing
collateral exceeds the gain from the edit itself.**

This unifies five consecutive results:

| task | change | collateral |
|---|---|---|
| 87 | inline more 64-byte copies | +17,728 (icache) |
| 88 | remove redundant clears | +9,536 |
| 89 | refill `.text.hot.draw` | +11,648 |
| 94 | admit a function to ITCM | `STG` +3,712 |
| **95** | **hoist invariants + port traversal** | **`STG` +3,392 / +5,056** |

Five different mechanisms, five regressions, one shared cause. **The binary's
layout is saturated: any edit large enough to matter perturbs enough neighbours
to cost more than it saves.**

## 4. What this does not say

It does **not** say the animation lever is wrong. Task 92's sizing stands —
animation is ~183,564 ticks/frame including the soft-float it owns, the largest
subsystem in the frame — and this task measured its first slice working (`FTR`
−2,240 P50 on 98/128 frames).

It says the lever cannot be pulled **one slice at a time**. A change that
rewrites the channel representation wholesale — flat contiguous arrays replacing
the `aobj->next` walk, so the traversal stops chasing pointers entirely — changes
enough at once that the layout re-shuffles around a genuinely smaller working
set, instead of paying re-addressing costs for a gain of a few thousand ticks.
That is a different size of task, and Task 95 is the evidence that the
incremental version of it does not pay.

## 5. State

`WORK-H` P95 1,733,888 against the 1,120,000 gate: **613,888 over.**

Every direction closed with a measurement: texture memo (Task 93), soft-float
(92), dense-vertex re-shade (90), animation-compiler-as-scoped (77 E1), `mem*`
(87/88), placement (87/88/89/94), **incremental animation reorganization (95)**.

The remaining work is the wholesale animation channel rewrite, and it should be
scoped as a subsystem task with its own session and its own verifier budget —
not as another single-lever experiment. Five in a row have now failed for the
same reason, which is enough evidence to stop trying that shape.

## 6. Postscript — Task 96 E0 attempted and discarded

The obvious follow-up is to size the wholesale rewrite before spending a session
on it: if the `AObj` chain is already effectively contiguous, flattening it buys
nothing. A read-only probe was built and run for exactly that.

**Its headline metric was invalid and the number is not recorded here.** It
compared *distinct starting cache lines* for the scattered chain against *total
spanned lines* for a hypothetical flat array. `AObj` is 36 bytes, so every node
straddles two 32-byte lines that the counter never saw — the two sides of the
ratio were not the same quantity, and it duly came out below 1.0, implying
scattering is cheaper than packing. It is not.

The probe was also incomplete: it observed only the `gcPlayAnimAll` path, while
Task 95 established at link time that `ftParamUpdateAnimKeys` and
`ndsBaseFTCommonGuardUpdateJoints` also call the joint player directly.

Discarded rather than reported, on the Task 84 precedent — that task threw away
its `memcpy` attribution when 82% of samples resolved into BSS objects that
cannot be return addresses, and the campaign is better for it.

**What a valid version must do**, so the next attempt does not repeat this:

1. Count every 32-byte line each node *spans*, not the line its first byte
   falls in — `(addr + sizeof(AObj) - 1) / 32 - addr / 32 + 1` per node.
2. Compare against a flat array measured the same way.
3. Instrument the joint player itself rather than one of its three callers, so
   `ftParamUpdateAnimKeys` and `ndsBaseFTCommonGuardUpdateJoints` are counted.
4. Ideally pair it with a dcache-miss count rather than inferring misses from
   addresses, since the chain may well be resident.

The one datum that survives is that the chain is genuinely scattered — **0 of 68
consecutive node pairs were adjacent in memory.** That is consistent with the
rewrite premise but does not size it, and sizing is what was needed.

