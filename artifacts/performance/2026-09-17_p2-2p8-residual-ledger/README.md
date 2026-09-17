# What is left: every measured class, and the 321,866 nobody has found

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
| literal-pool packing | 11,449 | 2.4% | sized below; invasive source change |
| **total if every one lands** | **146,678** | **31.3%** | |
| **RESIDUAL STILL UNFOUND** | **321,866** | **68.7%** | |

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
best case — banking the one measured win and landing all five unbuilt sizings —
is **146,678**, which is 31.3% of the gap and leaves **321,866 unfound**.

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

- ~~Packing the hot scalars into one struct~~ and ~~`-fsection-anchors`~~ — both
  **now sized, see the section below**. Anchors are inert; packing is worth
  11,449 on the measured set.
- Reducing the number of **non-fighter** DObjs (stage, effects, items) inside the
  203 peak — the fighter share is the joint lever, but the remainder has not
  been broken out.

## The literal-pool lane, sized — it is not the rescue either

This was the last item on the "not tried" list, and the biggest: PC-relative
literal-pool loads are **67,858 tk/fr**, larger than any remaining candidate,
and on ARM946E-S they are data-side reads of `.text` that allocate D-cache lines
holding code. Two proposals existed for it. Both are now settled.

### `-fsection-anchors` is inert in this build

Measured directly, eight independent 4-byte statics read in one function, then
the same eight packed into one struct, at the shipping flags:

| arm | 8 separate statics | packed struct |
|---|---:|---:|
| shipping (`-fdata-sections`) | **8** pool loads | **1** |
| `-fdata-sections -fsection-anchors` | **8** — no effect | 1 |
| `-fsection-anchors` without `-fdata-sections` | **1** | 1 |

**Section anchors do nothing while `-fdata-sections` is on**, because each static
is alone in its own section and there is nothing to anchor it to. Dropping
`-fdata-sections` would enable them, but that is the granularity the DTCM lane
and `--gc-sections` both depend on. The flag proposal is dead; the *packing*
proposal is what carries the mechanism, and it needs no codegen flag at all.

### Packing is real, and worth 11,449 on the measured set

Today each base load fetches its own pool word, so there is one pool load per
(function, symbol) pair. Packed, a function touching *k* symbols needs one pool
load for the struct base plus *k* immediate-offset loads. So the ceiling is
exactly the base loads that collapse into a shared base.

Measured across the 112 DTCM symbols against the banked profile:

| | value |
|---|---:|
| pool words holding their addresses | 1,201 |
| base-load PCs | 1,542 |
| **distinct functions touching the set** | **645** |
| base-load stall | 19,681.4 tk/fr |
| packed: base loads 1,542 → 645 | **−58.2%** |
| **ceiling** | **11,449 tk/fr — 2.4% of the gap** |

**The shape of the distribution is why it is not larger.** Of the 645 functions,
**441 touch exactly one** of the 112 symbols; 97 touch two, 42 touch three, and
only 34 touch five or more. A function that reads one static still needs one
pool load whether that static is packed or not, so two thirds of the call sites
are unreachable by this lever by construction.

Scaling the same 58.2% to the whole 67,858 bucket gives an absolute upper bound
of ~39,000 — and that assumes the same clustering holds program-wide *and* that
every unrelated static in the binary can be packed together, which is a very
large invasive source change for a bound that still does not reach the gate.

**Verdict: real, 2.4% on the clean subset, ~8% program-wide at an unrealistic
limit. It joins the ledger as another sub-10% candidate rather than changing
its conclusion.** The residual stands.
