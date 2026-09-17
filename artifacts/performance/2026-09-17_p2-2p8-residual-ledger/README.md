# What is left: every measured class, and the 333,315 nobody has found

The gate needs **−468,544 tk/fr** from WORK-H P50 1,588,544. This is the whole
campaign in one table, so the owner's decision is made against the ledger rather
than against whichever lane was measured most recently.

**It exists because the board was telling the owner something wrong.** It said
the gate needs "Sacrifice Order 2+3 (fewer joints / fewer transformed objects)".
The joint half of that was **already measured and closed** on 2026-09-16
(`…_p2-2p8-joint-cap-ladder/`), and `gNdsGCDrawsActiveMax` counts live **DObjs**
— for a fighter, a DObj *is* a joint, so "fewer transformed objects" and "fewer
joints" are the same lever wearing two names.

## Live candidates

| candidate | tk/fr | % of gap | status |
|---|---:|---:|---|
| **DTCM hot scalars** | **43,200** | **9.2%** | **BANKED**, confirmed per-PC |
| VRAM arena | 55,669 | 11.9% | sized, unbuilt |
| redundant pre-overwrite clears | 19,466 | 4.2% | sized, unbuilt; needs a redundancy proof per site |
| `GObj`/`DObj` field repack | 10,867 | 2.3% | mechanism unblocked (shadow header needs no decomp edit); HIGH risk |
| buffer 32-byte alignment | 6,027 | 1.3% | sized, unbuilt; alignment not yet established |
| **total if every one lands** | **135,229** | **28.9%** | |
| **RESIDUAL STILL UNFOUND** | **333,315** | **71.1%** | |

Every figure above its own sizing document; only the first is measured on
hardware.

## Closed classes

| class | ceiling | % of gap | why it is closed |
|---|---:|---:|---|
| leaf levers | ~90,000 | 19.2% | spent |
| renderer streaming repack | 78,000 | 16.6% | NO-GO — compulsory traffic (`…_stall-budget/RENDERER_STREAMING_SIZING.md`) |
| SRC bound pose/transform | 70,000 | 14.9% | no conversion measured |
| **joint / per-fighter geometry** | 12,144 | 2.6% | **SPENT** — see below |
| placement re-phasing | — | — | CLOSED: hazard, not lever |
| 30 Hz simulation | ~59% | — | **owner-forbidden** ("NO 30 Hz simulation"; 30 FPS at four players non-negotiable) |

## The joint lever is spent, and that is the correction

`…_p2-2p8-joint-cap-ladder/` ran the owner's own authorisation ("you are allowed
to do test builds of experimental optimizations. Less joints etc.") and returned
three results:

- **Fewer triangles** — ceiling **−12,144**, 2.6% of the gap. No CPU work is
  per-vertex.
- **A genuinely smaller skeleton** — **not implementable as a switch.**
  `NDS_LAB_JOINT_CAP=1` aborts in the CPU AI: a pruned joint leaves `fp->joints[]`
  NULL and `ndsBaseFTComputerSetFighterDamageDetectSize` dereferences it
  (`decomp/.../ft/ftcomputer.c:7970`, `r0 = 0`). Fox's `damage_coll_descs` alone
  names joint ids 5, 6, 8, 9, 12, 14, 15, 19, 20, 24, 25. A reduced skeleton is a
  per-fighter **data re-derivation** — hurtbox descriptors, effect joint ids,
  foot joint ids and every animation track binding re-authored — which is why it
  costs Sacrifice Order 2 **and** 3 rather than 2 alone.
- **Less animation** — deleting **94.7%** of all pose-entry evaluation
  (`gNdsLabPoseJointCapSkipped` 259,778 against 258,836 evaluated in the control)
  produced **no WORK-H reduction**. Not a small win, not a win at the noise
  floor: the frame did not get cheaper. The arms also diverged (slot 1 drew
  18,826 **more** triangles), so no price is readable from it either way.

So the lever the board was still offering the owner is measured, and the largest
sub-lane inside it does not convert even at the impossible limit.

## What this means for the decision

**Every structural class is now either banked, sized-and-small, or closed.** The
best case — banking the one measured win and landing all four unbuilt sizings —
is **135,229**, which is 28.9% of the gap and leaves **333,315 unfound**.

Two independent bounds say that residual is not hiding in layout:

1. The layout ceiling is **424,336** (every line fill in the frame), so even a
   *perfect* data cache leaves WORK-H at 1,164,208 — **44,208 over the gate**
   (`…_p2-2p8-locality-sizing/`).
2. The issue floor alone is **543,509 under** the gate, so no arithmetic
   deletion can close it either.

The Sacrifice Order options that remain are therefore:

- **Order 2 (visual) alone** is not enough: the visual lever inside this ledger
  is fewer triangles at 12,144.
- **Order 2+3 (fewer joints)** is **measured and spent** as a switch, and as a
  data re-derivation it is a per-fighter re-authoring project whose *measured*
  animation half returns nothing.
- **Order 4 (the 60 Hz simulation)** is the only untested class that is large
  enough, and it is currently **owner-forbidden**.

**That is the impasse, and it is an owner decision, not an engineering one.**
The honest statement is that the campaign has not found 71% of its target and
the classes large enough to contain it are each either refuted or ruled out.

## What has not been tried

Recorded so the ledger is not mistaken for an exhaustive search:

- **Packing the hot scalars into one anchored struct** rather than moving them.
  A scalar's literal-pool base load costs *more* than the dereference it feeds
  (19,681 against 34,121 across the 112 moved symbols) and DTCM does not touch
  it. The whole PC-relative literal-pool bucket is **67,858 tk/fr** and has never
  been attacked. Unsized, and it does not consume the DTCM budget.
- **`-fsection-anchors`**, which composes with the above. Unsized.
- Reducing the number of **non-fighter** DObjs (stage, effects, items) inside the
  203 peak — the fighter share is the joint lever, but the remainder has not
  been broken out.
