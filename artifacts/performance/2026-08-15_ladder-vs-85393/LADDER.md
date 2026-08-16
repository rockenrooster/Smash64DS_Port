# The ladder re-read against the repaired-tree gap — the inventory is now ~1.5x the gap, and only one rung clears alone

> ## RE-QUOTED 2026-08-16 AGAINST **+94,481**, the shipping-renderer gap at HEAD
>
> **Every ratio in this file has been re-divided in place. The denominator is
> `+94,481`, and it is stated at every table that carries a ratio so it cannot be
> separated from the number again.**
>
> **Basis:** `build-c206-shipgx0` — rank-80 **1,239,808 raw / 1,214,861 net**, gate
> 1,120,380, `NDS_R2_FIGHTER_GX_COMPOSE 0` (the renderer the ROM actually ships),
> bore 0, HEAD `b1339828070`, 1,600 samples, frames 439–2038, `slips=0`
> (`../2026-08-16_gxcompose-bank-basis/BASIS.md`).
>
> **Why the old denominators were wrong, twice over.** `+85,393` (`build-c199-bank0`)
> and `+81,297` (`build-c200-bank84`) were both measured with
> `NDS_R2_FIGHTER_GX_COMPOSE=1` while `Makefile:1545` pins the published block to `0`
> — a renderer the user does not run. The conversion applied here is
> **85,393 / 94,481 = 0.904**; a figure previously read against `+81,297` converts by
> **81,297 / 94,481 = 0.860**.
>
> **No conclusion in this file changes sign.** The ordering of the rungs and the
> "only compensated 30 Hz clears alone" verdict are unaffected. Two rows are
> *superseded by later measurement* and are marked as such where they appear:
> the animation-representation mechanism (33,951) is **closed at ~1% conversion**,
> and the draw-side fixed-point ceiling is **~11,000–15,000, not 24,564**, on the
> camera cycle's measured 1.70x (`../2026-08-16_camera-fixedpoint/CAMERA_Q20_12.md`).
>
> The directory name is left as-is so existing citations keep resolving.

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
`build-c206-shipgx0`, the shipping renderer (1,600 samples, frames 439–2038, DRAW=1,
`slips=0`, `GX_COMPOSE 0`):

```text
rank-80 raw / net    1,239,808 / 1,214,861      gate 1,120,380
EXACT NET GAP        +94,481      <- the denominator of every ratio below
```

| denominator this ladder was previously written against | value | factor to today |
|---|---:|---:|
| `MENU.md` header (bank `c170`) | +32,593 | **2.899x** |
| `K-GXC` bank `c185` | +28,689 | **3.293x** |
| `LADDER` v1 (`c199-bank0`, GX=1) | +85,393 | **1.106x** |
| `REBANK` (`c200-bank84`, GX=1, bore 84) | +81,297 | **1.162x** |

So **every `x req` in `MENU.md` divides by ~2.9–3.3**, and two rungs additionally need
their *numerators* corrected before the division (§2).

---

## 1. Which rungs still clear on their own

**One. Compensated 30 Hz.** Everything else is now a fraction.

**Every ratio in this table divides by `+94,481`, the shipping-renderer gap at HEAD
(`build-c206-shipgx0`).**

| rung | rank-80 tk/fr | old x (/28,689) | superseded x (/85,393) | **x of +94,481** | clears alone? | call |
|---|---:|---:|---:|---:|:--:|---|
| **Compensated 30 Hz** — **SUPERSEDED, see note** | ~~119,744~~ | 4.17x | 1.402x | **1.267x** | **YES** | OWNER — sacrifice rung 4 |
| Draw side, whole flat lane | **348,268** | 12.14x | 4.078x | **3.686x** | *surface, not a lever* — §3 | mixed |
| Animation evaluate/parse lane | 95,048 | 3.31x | 1.113x | **1.006x** | no | engineering |
| Soft float — **RE-MEASURED 168,060, see note** | ~~94,602~~ | 3.30x | 2.067x | **1.779x** | no | engineering |
| — **of which DRAW-ONLY, convertible** | **34,178** | — | 0.420x | **0.362x** | no | OWNER (precision) — §2(d) |
| — animation's named mechanism — **CLOSED at ~1%** | ~~33,951~~ | 1.18x | 0.398x | **0.359x** | no | **struck — converts −319 tk/fr** |
| Stage no-Z band @ **exactly 1.00x** | 22,608 | 0.79x | 0.265x | **0.239x** | no | OWNER (visible) |
| Slice 2 — `gcRunAll` scheduler machinery, **100 % deletion** | 17,786 | 0.62x | 0.208x | **0.188x** | no | engineering |
| — fighter narrow-phase identifiable float | 15,217 | 0.53x | 0.178x | **0.161x** | no | route closed (§2c) |
| Texture-bind collapse | 13,868 | 0.48x | 0.162x | **0.147x** | no | needs a counter first |
| Particle draw kernels | 12,595 | 0.44x | 0.147x | **0.133x** | no | OWNER (visible) |
| In-match asset I/O — deleting **all seven** load frames | 9,863 | 0.34x | 0.116x | **0.104x** | no | engineering |
| `ndsRendererSyncTextureTile` | 8,867 | 0.31x | 0.104x | **0.094x** | no | needs a counter first |
| `NDS_DREAMLAND_CARD_CULL` sub-rung | ~4,600 | 0.16x | 0.054x | **0.049x** | no | OWNER (visible) |
| newlib shipped residual (ceiling 4,267) | ~1,526 | 0.05x | 0.018x | **0.016x** (ceil 0.045x) | no | engineering |
| no-Z bit-identical repeat matrix loads | 1,305 | 0.045x | 0.015x | **0.014x** | no | engineering |

> **The 30 Hz row is superseded upward, not downward.** `119,744` was a Task 106
> mechanism-level figure that no census could confirm. Re-derived 2026-08-16 by exact
> re-rank of `build-c206-shipgx0`'s own 1,600 rows — delete half of the `SRC` bucket
> (`SRC` is *exactly* the two 60 Hz logical updates a presented frame runs;
> `taskman_seam.c:4273-4275` names `ndsRunMarioFoxProofUpdate` as its only writer) and
> re-rank — rank-80 moves **291,488**, i.e. **3.085x of +94,481**, because `SRC`
> concentrates **2.09x** on the gate population. See
> `../2026-08-16_gap-position/POSITION.md` §4.

**`GX_COMPOSE`'s −17,152 is not on this ladder, and it is not headroom either.** It is
**RETIRED**: the same-HEAD compile-time pair reads rank-80 +7,040 and P50 −4,288 — inside
both floors and disagreeing in sign — and the paired per-frame rows show the flag moves
work between buckets rather than deleting it (FTR −8,192, STG +6,656, MISC +2,368; whole
draw side **+832**). `+94,481` is measured at `GX_COMPOSE 0`, the shipping renderer, so
nothing about this flag is owed to or subtractable from it.
(`../2026-08-16_gxcompose-bank-basis/BASIS.md`, `../2026-08-16_gap-position/POSITION.md` §3.)

---

## 2. Three numerators that must be corrected before the division

**(a) The draw side is 2.0x bigger than `MENU.md` says, not smaller.**
`MENU.md` §6.2's 170,953 is **7 symbols**; `GXSTACK_IO_DRAW.md` §4 enumerates **36** for
**348,268 tk/fr at ~1.02x**. The menu figure was a floor. **Against +94,481, the
shipping-renderer gap at HEAD**, the lane is **3.686x** the gap — still the largest
convertible surface on the board, and still flat, which is what makes it convert ~1:1
(`a-flat-lane-is-the-best-converting-lane`).

**(b) Asset I/O collapses from "3.1x" to 0.116x.** `MENU.md` priced the lane at 100,689
with +67,454 excess. `K-GXFIX` measured what *deleting* it is worth: the work sits on
**7 frames of 1,600** holding 85.4 % of the match's FAT reads, but those frames rank
**3, 5, 10, 13, 14, 16, 23** — deep *inside* the top 80 — so removing them outright moves
rank-80 by **9,863 tk/fr**, reproducing an independent 9,874. That is
`call-share-is-not-cost-share` / `cluster-where-the-percentile-lives`: MENU's 3.1x was
**concentration read as size**. **Against +94,481** it is **0.104x. This lane is dead.**

**(c) Particle draw's −30,676 stays retracted.** The real draw kernels are **12,595**
(2.4x below it) — **0.133x against +94,481**. The owner must be shown 12,595, never
30,676.

**(d) Soft float is 168,060, not 94,602 — and only 34,178 of it is convertible.**
`MENU.md`'s 94,602 is the pre-repair `c179` DRAW=0 capture. Re-measured on the repaired
tree (`../2026-08-15_drawside-softfloat/DRAW_FIXEDPOINT.md`, arm `c200-trackprof-off`,
same marginal-80 basis) the class is **168,060 tk/fr**, cross-checked at 165,187 by direct
`nm` range sum. **That is not good news, because 80.2% of it is simulation or shared and
`PROJECT_GOAL.md` freezes it.** The draw-only + draw+dispatch part, classified from the
linked ELF rather than from names, is **30,638 + 3,540 `sqrtf` = 34,178 = 0.362x against
+94,481**, and it is **flat at 1.12x** against the class's 2.11x — the concentration is
all in the collision half.

> **AMENDED 2026-08-16 twice, and both amendments cut it.** (i) The 5.14x
> same-operation exchange rate was **refuted in situ at 1.70x** by its own falsifier
> (`../2026-08-16_camera-fixedpoint/CAMERA_Q20_12.md`), so the 24,564 ceiling reads
> **~11,000–15,000 = 0.116x–0.159x of +94,481**. (ii) It is **not
> engineering-available**: the conversion changes pixels (6.5350% on the camera chain
> alone) and is `BLOCKED(decision: draw-side precision)` with the owner. It belongs
> under sacrifice rung 2, visual fidelity — not in the fidelity-neutral inventory.

---

## 3. The draw side is a surface with no sized waste — that is the finding, restated at the new gap

The lane, by part (same basis, `GXSTACK_IO_DRAW.md` §4). **`x req` divides by +94,481,
the shipping-renderer gap at HEAD:**

| part | tk/fr | conc | **x of +94,481** |
|---|---:|---:|---:|
| Fighter draw kernel | 113,286 | 1.02–1.03x | **1.199x** |
| **Matrix compose + load** | **85,078** | ~1.05x | **0.900x** |
| Stage segment/run scaffolding | 53,118 | 1.00x | 0.562x |
| Material / texture state | 42,859 | ~1.0x | 0.454x |
| Stage no-Z band | 22,608 | 1.00x | 0.239x |
| Draw-list walk / adapters | 19,479 | ~1.0x | 0.206x |
| Particle draw | 11,840 | 1.19x | 0.125x |
| **total** | **348,268** | ~1.02x | **3.686x** |

- **The matrix compose + load pipeline is 0.900x of the gap** — the largest single
  draw-side part after the fighter kernel, and the lane the last bank's attribution
  already pointed at. (The v1 of this file noted it was 0.996x of `+85,393`; that
  near-coincidence was an artefact of the wrong denominator and is gone.)
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
animation mechanism        33,951     CLOSED 2026-08-15 -- converts ~1% (-319 tk/fr)
stage no-Z band            22,608     OWNER (visual)
slice 2 (100% deletion)    17,786
narrow-phase float         15,217     route measured closed at 2.68
texture-bind collapse      13,868     needs a counter
particle draw kernels      12,595     OWNER (visual)
asset I/O (delete all 7)    9,863
SyncTextureTile             8,867     needs a counter
newlib shipped residual     1,526
no-Z repeat matrix loads    1,305
-------------------------------------
TOTAL                     137,586  =  1.456x of +94,481
minus the closed animation row
                          103,635  =  1.097x of +94,481
```

Against +28,689 that same inventory was **4.796x**. **The margin for a failed lever has
gone from ~4.8x to ~1.1x once the closed row is struck**, and 100 % conversion of every
remaining item is not on offer either.

Strip out what is not engineering-available *today*. **Every ratio divides by +94,481,
the shipping-renderer gap at HEAD:**

| class | tk/fr | **x of +94,481** |
|---|---:|---:|
| OWNER-gated (visible fidelity) | 35,203 | 0.373x |
| CLOSED — animation representation converts ~1% | ~~33,951~~ | — |
| needs a counter before it is an item | 22,735 | 0.241x |
| route measured closed (exchange 2.68) | 15,217 | 0.161x |
| **unambiguously available engineering inventory** | **30,480** | **0.323x** |

> **That last row is the finding.** What is available today, with no owner decision, no
> pending measurement and no new counter, is **30,480 tk/fr — 32.3 % of the gap** — and it
> is four items none of which exceeds 0.19x. **Closure is now a combination or an owner
> decision; it is no longer a lever hunt.**

**AMENDMENT WITHDRAWN, 2026-08-16.** The 2026-08-15 amendment lifted the
unambiguously-available engineering share from 0.357x to 0.659x by adding the draw-side
soft-float ceiling of 24,564. Both halves of that have since failed:

- **The rate was refuted.** The 5.14x same-operation prior measured 1.70x in situ
  (`../2026-08-16_camera-fixedpoint/CAMERA_Q20_12.md`), so the ceiling reads
  **~11,000–15,000, not 24,564.**
- **It was never engineering-available.** The conversion changes pixels and is
  `BLOCKED(decision: draw-side precision)` with the owner, i.e. it is sacrifice rung 2.

The unambiguously-available engineering share therefore stands at **0.323x**, and the
fidelity-neutral ceiling including both counter-gated items is **53,215 tk/fr = 0.563x**
— see `../2026-08-16_gap-position/POSITION.md` §2, which is the assembled position and
supersedes this section's classification.

## 5. What this means for the queue, stated as consequences and not as recommendations

1. **Nothing in the engineering ladder closes alone.** The only rung that does is
   compensated 30 Hz, at **3.085x** of +94,481 by the 2026-08-16 re-rank (the 1.402x /
   119,744 above is superseded upward — see §1's note). `PROJECT_GOAL.md` puts it at
   **rung 4** of the sacrifice order, below both fidelity rungs; its blanket form already
   regressed and diverged once (`plan.md` §3 item 8), with `syUtilsRandFloat` sharing one
   LCG across 135 draw sites.
2. **The owner ladder is now 0.373x, not 1.2x.** Both visible rungs together
   (no-Z band 22,608 + particle kernels 12,595) no longer close the gap even if the owner
   grants both. At +28,689 they did — comfortably. **That changes what the owner is being
   asked for**: previously "accept one visible change and we are done", now "accept two
   visible changes and we are 37 % of the way".
3. **The two counter-gated draw items are now the cheapest unexplored engineering
   volume**, at 0.241x combined for two counters and no build.
4. **Task B is closed, not pending.** The animation-representation lane converts ~1% of
   its 33,951, so the largest single *pending* engineering question is now the pair in
   point 3.

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
