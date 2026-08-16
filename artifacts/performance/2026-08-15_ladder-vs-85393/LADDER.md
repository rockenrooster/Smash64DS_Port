# The ladder re-read against the repaired-tree gap — the inventory is now ~1.6x the gap, and only one rung clears alone

> **CORRECTION, 2026-08-15: the gap this file is titled after is superseded, and every
> ratio below is ~4.8% pessimistic.** The banked requirement is **`+81,297`** from
> `build-c200-bank84` — rank-80 **1,226,624 raw / 1,201,677 net**
> (`../2026-08-15_ftanim-full-coverage/REBANK.md`). Multiply every "x the gap" figure
> below by **85,393 / 81,297 = 1.050** to read it against the real requirement.
> **No conclusion in this file changes sign**: the ordering of the rungs, the "only
> compensated 30 Hz clears alone" verdict, and the 0.357x unambiguously-available
> engineering share are all unaffected at that scale.
> The directory name is left as-is so existing citations keep resolving.
>
> **Basis footnote, stated rather than buried.** `+85,393` is `build-c199-bank0`, built
> at bore **0**; `+81,297` is `build-c200-bank84`, built at bore **84**. The shipping
> default is now **0**, so the bore-0 arm is the one that matches the shipped binary —
> but the two arms are **4,096 apart at rank-80, inside the ≥14,080 cross-build floor**,
> i.e. the same level to this instrument. Quote **+81,297** and carry ±4,096 of bore
> basis with it until a re-bank is taken at bore 0.

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **HEAD `1eb6b453803`**
**Zero builds, zero runs.** Every figure below is re-quoted from a census already on
disk; nothing is re-derived and nothing new is measured.
**UNITS: 2 profile cycles = 1 project tick.**

## 0. Basis, stated once

Lane sizes come from the rank-80 marginal census
`../2026-08-15_cfx-ring-draw0/b-c179-pc.csv` (`build-c179-cfxring-b-d0`,
`NDS_TICK_HUD_DRAW=0`, `BOTH_CPU=1`, `BATTLEPACK=1`, `KEEP_CACHE=1`, DLDI on,
`NDS_TASK37_PROFILE=1`), window frames **438..2038 = 1,601 regions**, marginal mask
**the 80 frames with `total_cycles - halt_wait` >= 1,171,083 ticks**. That is `MENU.md`'s
basis and `GXSTACK_IO_DRAW.md` §4's basis, so the two are addable.

**The requirement is not on that basis and does not need to be** — it is a *level* from
the new bank `build-c199-bank0` (1,600 samples, frames 439–2038, DRAW=1, `slips=0`):

```text
rank-80 raw / net    1,230,720 / 1,205,773      gate 1,120,380
EXACT NET GAP        +85,393
```

| denominator this ladder was previously written against | value | factor to today |
|---|---:|---:|
| `MENU.md` header (bank `c170`) | +32,593 | **2.620x** |
| `K-GXC` bank `c185` | +28,689 | **2.977x** |

So **every `x req` in `MENU.md` divides by ~2.6–3.0**, and two rungs additionally need
their *numerators* corrected before the division (§2).

---

## 1. Which rungs still clear on their own

**One. Compensated 30 Hz.** Everything else is now a fraction.

| rung | rank-80 tk/fr | old x (/28,689) | **new x (/85,393)** | clears alone? | call |
|---|---:|---:|---:|:--:|---|
| **Compensated 30 Hz** | 119,744 | 4.17x | **1.402x** | **YES** | OWNER — sacrifice rung 4 |
| Draw side, whole flat lane | **348,268** | 12.14x | **4.078x** | *surface, not a lever* — §3 | mixed |
| Animation evaluate/parse lane | 95,048 | 3.31x | **1.113x** | no | engineering |
| Soft float — **RE-MEASURED 168,060, see note** | ~~94,602~~ | 3.30x | **2.067x** | no | engineering |
| — **of which DRAW-ONLY, convertible** | **34,178** | — | **0.420x** | no | **gate PASSED, falsifier named** |
| — animation's named mechanism | 33,951 | 1.18x | **0.398x** | no | **Task B decides** |
| Stage no-Z band @ **exactly 1.00x** | 22,608 | 0.79x | **0.265x** | no | OWNER (visible) |
| Slice 2 — `gcRunAll` scheduler machinery, **100 % deletion** | 17,786 | 0.62x | **0.208x** | no | engineering |
| — fighter narrow-phase identifiable float | 15,217 | 0.53x | **0.178x** | no | route closed (§2c) |
| Texture-bind collapse | 13,868 | 0.48x | **0.162x** | no | needs a counter first |
| Particle draw kernels | 12,595 | 0.44x | **0.147x** | no | OWNER (visible) |
| In-match asset I/O — deleting **all seven** load frames | 9,863 | 0.34x | **0.116x** | no | engineering |
| `ndsRendererSyncTextureTile` | 8,867 | 0.31x | **0.104x** | no | needs a counter first |
| `NDS_DREAMLAND_CARD_CULL` sub-rung | ~4,600 | 0.16x | **0.054x** | no | OWNER (visible) |
| newlib shipped residual (ceiling 4,267) | ~1,526 | 0.05x | **0.018x** (ceil 0.050x) | no | engineering |
| no-Z bit-identical repeat matrix loads | 1,305 | 0.045x | **0.015x** | no | engineering |

**`GX_COMPOSE`'s −17,152 is not on this ladder: it is already spent.** `build-c199-bank0`
was built with `NDS_R2_FIGHTER_GX_COMPOSE 1` (its generated header was verified), so the
+85,393 gap is measured *with* the compose win already taken. It cannot be subtracted
again. **The published default is still `0`** (`Makefile`), so the shipping ROM does not
yet have it — that is a standing configuration gap, not spare headroom.

---

## 2. Three numerators that must be corrected before the division

**(a) The draw side is 2.0x bigger than `MENU.md` says, not smaller.**
`MENU.md` §6.2's 170,953 is **7 symbols**; `GXSTACK_IO_DRAW.md` §4 enumerates **36** for
**348,268 tk/fr at ~1.02x**. The menu figure was a floor. Against +85,393 the lane is
**4.078x** the gap — still the largest convertible surface on the board, and still flat,
which is what makes it convert ~1:1 (`a-flat-lane-is-the-best-converting-lane`).

**(b) Asset I/O collapses from "3.1x" to 0.116x.** `MENU.md` priced the lane at 100,689
with +67,454 excess. `K-GXFIX` measured what *deleting* it is worth: the work sits on
**7 frames of 1,600** holding 85.4 % of the match's FAT reads, but those frames rank
**3, 5, 10, 13, 14, 16, 23** — deep *inside* the top 80 — so removing them outright moves
rank-80 by **9,863 tk/fr**, reproducing an independent 9,874. That is
`call-share-is-not-cost-share` / `cluster-where-the-percentile-lives`: MENU's 3.1x was
**concentration read as size**. Against +85,393 it is **0.116x. This lane is dead.**

**(c) Particle draw's −30,676 stays retracted.** The real draw kernels are **12,595**
(2.4x below it) — **0.147x**. The owner must be shown 12,595, never 30,676.

**(d) Soft float is 168,060, not 94,602 — and only 34,178 of it is convertible.**
`MENU.md`'s 94,602 is the pre-repair `c179` DRAW=0 capture. Re-measured on the repaired
tree (`../2026-08-15_drawside-softfloat/DRAW_FIXEDPOINT.md`, arm `c200-trackprof-off`,
same marginal-80 basis) the class is **168,060 tk/fr**, cross-checked at 165,187 by direct
`nm` range sum. **That is not good news, because 80.2% of it is simulation or shared and
`PROJECT_GOAL.md` freezes it.** The draw-only + draw+dispatch part, classified from the
linked ELF rather than from names, is **30,638 + 3,540 `sqrtf` = 34,178 = 0.420x**, and it
is **flat at 1.12x** against the class's 2.11x — the concentration is all in the collision
half. Conservative removable ceiling after replacement **24,564 (0.302x)**, on a measured
5.14x same-operation exchange rate that **falsifies the collision lane's 1.00 for this
lane**. It still does not close alone; it is now the largest *engineering-available*
single item on this board by a factor of ~2.7.

---

## 3. The draw side is a surface with no sized waste — that is the finding, restated at the new gap

The lane, by part (same basis, `GXSTACK_IO_DRAW.md` §4):

| part | tk/fr | conc | **x req** |
|---|---:|---:|---:|
| Fighter draw kernel | 113,286 | 1.02–1.03x | **1.327x** |
| **Matrix compose + load** | **85,078** | ~1.05x | **0.996x** |
| Stage segment/run scaffolding | 53,118 | 1.00x | 0.622x |
| Material / texture state | 42,859 | ~1.0x | 0.502x |
| Stage no-Z band | 22,608 | 1.00x | 0.265x |
| Draw-list walk / adapters | 19,479 | ~1.0x | 0.228x |
| Particle draw | 11,840 | 1.19x | 0.139x |
| **total** | **348,268** | ~1.02x | **4.078x** |

- **The gap is now almost exactly the size of the matrix compose + load pipeline**
  (85,078 vs 85,393 = **0.996x**). That is a coincidence, not a plan — but it is the
  right scale of ambition to hold in mind, and it is the lane the last bank's
  attribution already pointed at.
- **~27,467 tk/fr of the lane is GX FIFO `bus_contention`** and is a hardware floor.
  Software-addressable draw work is **320,801**, so closing the gap from the draw side
  alone means deleting **26.6 %** of it (24.5 % of the raw lane).
- **There is still no sized pure-waste item.** The largest exactly-sized category-1 item
  is **1,305 tk/fr (0.015x)** — 5 of 47 no-Z matrix loads per frame that are bit-identical
  repeats inside one triangle. Two candidates were checked and *refused*
  (`AdapterBuildPersistentStageWorldMatrix` already caches; `CommitNativeStageSegment` is
  pure instruction fetch, already refuted as a layout lever). Two survive and **each needs
  one counter before it is an item at all**: `ndsRendererSyncTextureTile` (8,867 tk/fr,
  72.68 syncs/frame) and the texture-bind collapse (103.45 requests -> 55.73 GX binds,
  13,868 tk/fr combined) — **0.266x together.**

**So the draw side is where the bytes are and where the flatness is, and it is *not* a
lever until somebody sizes waste inside it.** At +28,689 that was a comfortable position:
the two counter-gated candidates alone would have been 0.48x of the gap. At +85,393 they
are 0.27x, and the lane needs a fundamentally larger deletion — fewer runs, fewer
segments, fewer matrices — not a micro-cut.

---

## 4. The arithmetic that actually changes the campaign

Summing **every sized item on the board at 100 % conversion**, owner rungs included, with
no double counting (each row is a distinct symbol set; `CARD_CULL` excluded as a subset of
the no-Z band; 30 Hz excluded as a different kind of object):

```text
animation mechanism        33,951
stage no-Z band            22,608     OWNER
slice 2 (100% deletion)    17,786
narrow-phase float         15,217     route measured closed at 2.68
texture-bind collapse      13,868     needs a counter
particle draw kernels      12,595     OWNER
asset I/O (delete all 7)    9,863
SyncTextureTile             8,867     needs a counter
newlib shipped residual     1,526
no-Z repeat matrix loads    1,305
-------------------------------------
TOTAL                     137,586  =  1.611x the requirement
```

Against +28,689 that same inventory was **4.796x**. **The margin for a failed lever has
gone from ~4.8x to ~1.6x**, and 100 % conversion of every item is not on offer.

Strip out what is not engineering-available *today*:

| class | tk/fr | x req |
|---|---:|---:|
| OWNER-gated (visible fidelity) | 35,203 | 0.412x |
| pending Task B's verdict | 33,951 | 0.398x |
| needs a counter before it is an item | 22,735 | 0.266x |
| route measured closed (exchange 2.68) | 15,217 | 0.178x |
| **unambiguously available engineering inventory** | **30,480** | **0.357x** |

> **That last row is the finding.** What is available today, with no owner decision, no
> pending measurement and no new counter, is **30,480 tk/fr — 35.7 % of the gap** — and it
> is four items none of which exceeds 0.21x. **Closure is now a combination or an owner
> decision; it is no longer a lever hunt.**

**AMENDED 2026-08-15 by §2(d).** The inventory above predates the draw-only soft-float
measurement and does not contain it. Adding the **24,564 tk/fr** conservative ceiling
(a distinct symbol set — none of the 35 callers appears in any row above; the draw-side
figure excludes everything in `shared`/`sim-*`) lifts the unambiguously-available
engineering share from **0.357x to 0.659x**, and it is now the largest single engineering
item on the board — 2.7x the next one. It is not free: it needs one falsifier build, a
Q20.12 chain design, an ITCM leaf, a hardware-divider seam, and **owner visual acceptance
on a frame-locked pixel pair**, because precision changes. The animation-representation
row (33,951, "pending Task B's verdict") is **closed at ~1% conversion** and should be
struck from the pending class, not carried.

## 5. What this means for the queue, stated as consequences and not as recommendations

1. **Nothing in the engineering ladder closes alone.** The only rung that does is
   compensated 30 Hz at **1.402x**, which `PROJECT_GOAL.md` puts at **rung 4** of the
   sacrifice order, below both fidelity rungs, and which `MENU.md` §4.3 notes is a
   *mechanism-level* figure (Task 106) that **no census on this tree can confirm or
   refute** — and whose blanket form already regressed and diverged once (`plan.md` §3
   item 8), with `syUtilsRandFloat` sharing one LCG across 135 draw sites.
2. **The owner ladder is now 0.412x, not 1.2x.** Both visible rungs together
   (no-Z band 22,608 + particle kernels 12,595) no longer close the gap even if the owner
   grants both. At +28,689 they did — comfortably. **That changes what the owner is being
   asked for**: previously "accept one visible change and we are done", now "accept two
   visible changes and we are 41 % of the way".
3. **The two counter-gated draw items are now the cheapest unexplored engineering
   volume**, at 0.266x combined for two counters and no build.
4. **Task B is worth 0.398x of the gap and is the largest single pending question.**

---

## 6. What this document does NOT do

- **No new measurement.** Every number is re-quoted with its basis; the only new content
  is division and summation.
- **Does not re-price 30 Hz, and does not recommend it.** Sacrifice-order calls are the
  owner's.
- **Does not touch `MENU.md` or `plan.md`.** They record what was true against their own
  denominators; this file is the re-read, not a rewrite.
- **Does not claim the inventory is complete.** It is the inventory *that has been sized*.
  The 348,268 draw lane is 4.078x the gap and mostly unenumerated as waste — that is an
  argument for looking there, not evidence that nothing is there.
