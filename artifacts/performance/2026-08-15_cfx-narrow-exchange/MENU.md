# What is actually left, priced at rank-80 on this tree

Date: 2026-08-15. Branch `codex/r2-runtime2`. Base HEAD `1e2833cccba`.
Requirement **as written**: **+32,593 net at rank-80** (bank `c170`: 1,177,920 raw /
1,152,973 net).
**UNITS: 2 profile cycles = 1 project tick.**

> ## RE-QUOTED 2026-08-16 AGAINST **+94,481** — the `× requirement` column below is 2.899x too large
>
> **The current denominator is `+94,481`, the shipping-renderer gap at HEAD**
> (`build-c206-shipgx0`: rank-80 1,239,808 raw / 1,214,861 net, gate 1,120,380,
> `NDS_R2_FIGHTER_GX_COMPOSE 0`, bore 0, HEAD `b1339828070`;
> `../2026-08-16_gxcompose-bank-basis/BASIS.md`).
> **Multiply every `× requirement` in this file by `32,593 / 94,481 = 0.345`**, and see
> §1's re-quoted table below, which does it in place.
>
> **Two numerators in this file are also wrong and must be corrected BEFORE the
> division** — the correction is larger than the re-division:
> - **In-match asset I/O `100,689` is concentration read as size.** What *deleting* the
>   lane is worth at rank-80 is **9,863**, because the seven frames carrying 85.4% of the
>   FAT reads rank 3/5/10/13/14/16/23 — deep *inside* the top 80
>   (`../2026-08-15_ladder-vs-85393/LADDER.md` §2b). **0.104x, not 3.1x. Dead lane.**
> - **Soft float `94,602` is a pre-repair `c179` capture.** Re-measured on the repaired
>   tree it is **168,060**, of which only the draw side (**34,178**) is convertible at all
>   and that part is an owner precision decision, not engineering
>   (`../2026-08-15_drawside-softfloat/DRAW_FIXEDPOINT.md`).
> - **Draw side `170,953` is a floor, not the lane.** The enumerated lane is **348,268**
>   over 36 symbols (`GXSTACK_IO_DRAW.md` §4).
>
> The assembled current position is `../2026-08-16_gap-position/POSITION.md`.

## 0. The basis every number below shares — read this before quoting one

All figures come from `../2026-08-15_cfx-ring-draw0/b-c179-pc.csv`, the reduced
per-PC census of `build-c179-cfxring-b-d0`:
`NDS_R2_COLLISION_FIXED_DISPATCH=1`, `NDS_TICK_HUD_DRAW=0`, `NDS_R2_BOTH_CPU=1`,
`NDS_R2_BATTLEPACK=1`, `KEEP_CACHE=1`, DLDI on, `NDS_TASK37_PROFILE=1`,
window frames **438..2038 = 1,601 regions**, marginal mask **the 80 frames with
`total_cycles − halt_wait` ≥ 1,171,083 ticks**. Marginal ticks/frame = cycles/160.

**The marginal-80 population IS the rank-80 population**, which is why these are
quotable against +32,593 and whole-match census rows are not.
Marginal-80 frame total on this arm: **1,316,954 tk/fr** (a profile ROM's
`total_cycles − halt_wait`, not `WORK-H`; it carries the profiler and the tick
HUD, §6).

**The ranking metric is `marg tk/fr`, not concentration.** §0's own law —
"a change that removes 30,000 uniformly moves P95 by 30,000; one that removes
100,000 from only the top 20 frames moves P95 by zero" — says a **flat 1.00x
lane converts 1:1 into the percentile**. Concentration only says how much bigger
the marginal value is than the mean; it is the marginal value that is spent.

---

## 1. The five lanes, by what they are worth at rank-80

**`× req` as written divides by +32,593; `x of +94,481` is the same lane against the
shipping-renderer gap at HEAD. Where the numerator itself was later corrected, the
corrected value is the one to quote.**

| lane | marg tk/fr | conc | × req (+32,593) | **x of +94,481** | corrected numerator → **x of +94,481** | call |
|---|---:|---:|---:|---:|---|---|
| **In-match asset I/O** | **100,689** (excess **+67,454**) | 1.46–19.31× | 3.1× | 1.066x | **9,863 deletion value → 0.104x** | dead lane |
| **Animation evaluate/parse** | **95,048** | 1.36–2.05× | 2.9× | **1.006x** | — | engineering |
| **Soft float** | **94,602** | 1.14–1.51× | 2.9× | 1.001x | **168,060 → 1.779x**; convertible draw half **34,178 → 0.362x** | OWNER (precision) |
| **Draw side, flat** | **170,953** | 1.00–1.05× | 5.2× | 1.809x | **348,268 (36 symbols) → 3.686x** | mixed; the no-Z 22,608 is owner |
| **Particle draw kernels** | **12,595** | 1.19× | 0.39× | **0.133x** | — | owner (visible) |
| tick-HUD apparatus (not a lane) | 8,085 | 4.7–5.6× | — | — | — | vanishes at `NDS_TICK_HUD=0` |

---

## 2. Phase 3 — the `GX_COMPOSE` leak fix. **Still live, still the cheapest correctness-shaped item, but its PRICE IS STALE**

**State on this tree, verified from the Makefile and a built config header, not
from the board:** `NDS_R2_FIGHTER_GX_COMPOSE ?= 0` (`Makefile:755`) and
`override … := 0` at `Makefile:1507`, `:1733`, `:1990`. Every `#if` block is
still compiled-in-able (`nds_renderer.c:19088 :24697 :27799 :32785 :32808`,
`nds_renderer.h:948 :1288`, `reloc_backend_renderer_dl.c:310 :3776 :4366
:14238`). `build-c181-cfxnarrow-b-d0/nds_build_config.h` reads
`#define NDS_R2_FIGHTER_GX_COMPOSE 0`. **It is one flag away, and nothing has
rotted.**

**The price does NOT hold and must not be banked.** The −13,632 was measured
2026-08-11 against a `WORK-H` P95 of **1,258,112** (board §"Slice 43 KEPT",
`…/2026-08-11_c119-lane/SLICE43_GATE.md`). Today's rank-80 is **1,177,344** —
80,768 ticks lower, after slices 45/46/48, the BattlePack and the framebuffer
collapse. The board itself says *"do not arithmetically re-bank"*. Treat −13,632
as **evidence the mechanism pays**, and re-measure it.

**The blocker, and the board's citation for it is stale.** `nds_platform.c:3197`
is now inside a debug-print block; the leak note actually lives at
**`nds_platform.c:3260-3294`**, and the `|| NDS_TICK_HUD` it refers to is the
condition on line **3289**. What that comment banks, measured over 128 presented
frames on a still-blinking ROM: the GXSTAT error bit set on **every** frame, the
position/vector stack level advancing **+3 per frame wrapping mod 32**, and
every wrap-to-0 frame a low-polygon frame — **449/481/482/513/545 at
145/165/165/106/306 triangles against a 378 median, no exceptions**.

**What it would take, in the order that spends least:**

1. **Confirm the leak predates slice 43 — and this now costs ZERO builds.**
   The GXSTAT read is compiled into every `NDS_TICK_HUD` ROM
   (`nds_platform.c:3289-3294` → `gNdsHardwareRendererStatus`), and
   `GX_COMPOSE` is already **0** in every lab build on this tree. So
   `sample-tick-hud-buckets.ps1 -NoBuild -Build <any existing tick-HUD build>
   -PerFrameGlobals gNdsHardwareRendererStatus` reads bits 8..12 per frame on a
   `GX_COMPOSE=0` ROM. If the level advances there too, the leak is a standing
   renderer defect that slice 43 only exposed — and it is worth fixing on its own
   terms. (`-PerFrameGlobals` is mutually exclusive with `-RingDump`, so this is
   its own short run, not a rider on a gate run.)
2. **A cheaper instrument than GXSTAT already exists.** `nds_renderer.c:1546-1547`
   `#define`s `glPushMatrix`/`glPopMatrix` to `ndsRendererTask29GlPushMatrix`/
   `…Pop`, which already funnel through one recorder — a per-frame
   `pushes − pops` counter is a two-line addition at a single seam, and unlike
   GXSTAT it names *which* frame went unbalanced.
3. **Two named suspects, both already written down** (`nds_platform.c:3282-3284`):
   the Whispy native path emits **raw `MATRIX_PUSH`/`MATRIX_POP` FIFO words**
   that bypass the wrapper (`nds_renderer.c:14352`, `:14380`), and
   `ndsRendererEndParticleQuads`' pop is **conditional on two separate flags**.
   There is in-tree precedent for exactly this shape:
   `nds_renderer.c:5652` records *"adding segments to the mask bought four
   unmatched `glPopMatrix(1)` calls a frame."*
4. Then: stack level flat over a whole match, pixel-identical captures at the
   changed geometry, Boundary + soak, and **a fresh A/B for the win**.

**Call: engineering, not owner.** It is a correctness repair; the optimization it
unblocks is a bonus. Its cost is one short run to decide whether it is even
slice-43-specific.

---

## 3. Slice 2 — the epoch-flattened `gcRunAll` vector. **Real, composable, and 0.55× on its own**

Measured (`plan.md` §K-RERANK, 2026-08-15): the entire addressable lane — the
`gcRunAll` **scheduler machinery**, not the process bodies — is **17,786 tk/fr on
the P95 frames**. A *100% deletion* is **1.8× short of +32,593**, and a flat
vector still calls each process, so 100% is not available. The bracket that
isolates what it can touch (`GCRA-REM`) is **3.6–4.4% at 1.17–1.21×**.

Nothing on this tree changes that. `ndsBaseGcRunAll` reads **9,440 tk/fr** at
rank-80 in the c179 census, 1.00× — consistent with a scheduler whose cost is
its children's, not its own.

**Call: engineering. Keep it, do not lead with it.** It composes with anything
else and it is the only item here with no correctness or fidelity risk at all.

---

## 4. The owner ladder (§11) — re-priced at rank-80 on THIS tree

### 4.1 Stage no-Z band — **−22,510 STILL HOLDS, and it is the cleanest arithmetic on the board**

| symbol | bytes | whole tk/fr | **marg tk/fr** | conc | marg calls/fr |
|---|---:|---:|---:|---:|---:|
| `ndsRendererNativeStageLoadNoZMatrix` | 512 | 12,243 | **12,282** | **1.00×** | 47.00 |
| `ndsRendererNativeStageEmitNoZTriangle` | 1,208 | 5,194 | **5,200** | **1.00×** | 27.00 |
| `ndsRendererNativeStageEmitNoZVertex.isra.0` | 468 | 5,126 | **5,126** | **1.00×** | 81.00 |
| **band** | | 22,563 | **22,608** | **1.00×** | |

`RESIDUE.md` §6 priced this band at **22,510** on 2026-08-13. It reads **22,608**
here — **within 0.4%** — and the call counts are flat to two decimal places
(47.00 vs 46.97, 27.00 vs 26.98, 81.00 vs 80.95). **This is the one lane on the
board whose whole value converts 1:1 into rank-80**, because there is no
percentile structure in it at all: 22,608 is **69% of the requirement** from a
single visible-fidelity decision.

`FLAG_SWEEP.md` row 7 already sizes the *cheap* end of it: `NDS_DREAMLAND_CARD_CULL`
masks are **baked**, so the Task 36 capture omits the culled runs and replay
replays the reduced stream — `cheapest10` = 19 tris / 10.9%, `cheapest16` = 36
tris / 20.6%, bounded above by **≈4,600** plus whatever run-level scaffolding a
skipped run removes. **So the ladder has a sub-rung: ~4,600 for 20.6% of stage
scenery, versus 22,608 for the whole depth-disabled band.**

**Call: OWNER.** Visible scenery. Two rungs, both already implemented behind a
flag.

### 4.2 Particle draw reduction — **−30,676 does NOT reproduce and must not be quoted**

| symbol | whole tk/fr | **marg tk/fr** | conc |
|---|---:|---:|---:|
| `lbParticleDrawTextures` | 9,925 | **11,840** | 1.19× |
| `ndsRendererEndParticleQuads` | 605 | 650 | 1.08× |
| `ndsParticleDrawOwnTextureQuad` | 63 | 105 | 1.66× |
| **total** | 10,593 | **12,595** | 1.19× |

The board's −30,676 comes from a whole-match "draw half = 27,758 tk/frame"
(`P1_EXECUTION_BOARD.md:218`). The particle **draw kernels** on this tree total
**10,593 whole match / 12,595 at rank-80** — a factor of 2.4–2.6 below it. The
difference is not necessarily an error: the rest of the old figure lived in
shared renderer symbols (`ndsRendererNativeEmitProductionPrimitiveGroups`,
26,485 at rank-80, **0 icache and 10,287 bus_contention** — GX FIFO writes)
which a symbol census cannot split by producer.

**So: −30,676 is STALE at rank-80 and is not re-confirmable at symbol
granularity.** What is certain is the floor, **12,595**, and that it is a
destructive visible change. Anything above that needs a primitive-attributed
measurement, not a re-quote.

**Call: OWNER**, and the number presented to the owner should be **12,595 known
+ an unmeasured share of a 26,485 shared symbol**, not "30,676".

### 4.3 Compensated 30 Hz — **−119,744, unchanged and un-re-confirmable from a census**

It is a mechanism-level figure (Task 106), not a symbol sum, so no census on this
tree can confirm or refute it. Two things about it *are* current:
`plan.md` §3 item 8 records that blanket 30 Hz poses **regressed and diverged**,
and §3 item 7 records why the particle/effect **update** half is RNG-fatal (one
LCG serves the level-3 AI's 65 draw sites, `efmanager.c`'s 44 and
`lbparticle.c`'s 26) — so any cadence proposal must be checked against
`syUtilsRandFloat` before it is designed.

**Call: OWNER, last resort, in writing.** `PROJECT_GOAL.md` puts it at rung 4 of
the sacrifice order, below both fidelity rungs.

---

## 5. The `_svfiprintf_r` family — **30.6% of it IS in the shipped path, and the ≤769 bound understates the mask ~5.6×**

Answered from the linked ELF (the reader oracle) plus exact entry-PC call counts.

**Referrers of `_svfiprintf_r` (0x020c8e1c) in the c179 ELF — there are exactly
two live paths:**

```text
sniprintf      <- ndsRelocAssetFindEntry  (5 sites)   SHIPPED, ungated
               <- main                    (2 sites)   boot only
_vsniprintf_r  <- vsniprintf <- ndsPlatformPrintDebugLine   TICK-HUD ONLY
```

**Exact call counts (entry PC), marginal-80:**

```text
_svfiprintf_r             2.29 calls/fr   3,950 tk/fr   5.58x
  sniprintf               0.70            154            7.34x   <- shipped path
  _vsniprintf_r           1.57            135            5.00x   <- HUD path
  (0.70 + 1.57 = 2.27, against 2.29 measured)
_vfiprintf_r              3.15            2,792          4.99x   <- HUD only (iprintf)
consolePrintChar         48.83            2,063          5.26x   <- HUD only
ndsRelocAssetFindEntry    0.70            163            7.37x   <- shipped path
```

**So `_svfiprintf_r` is 30.6% shipped and 68.6% apparatus, by exact call share.**

- **Apparatus on the mask (vanishes at `NDS_TICK_HUD=0`):** `_vfiprintf_r` 2,792
  + `consolePrintChar` 2,063 + `iprintf` 104 + `ndsPlatformPrintDebugLine` 143 +
  `vsniprintf` 105 + `_vsniprintf_r` 135 + `_svfiprintf_r`'s HUD share
  ≈ **8,085 tk/fr**, i.e. **0.6% of the marginal frame**. Every measurement in
  this campaign carries it and the shipped ROM does not.
- **Shipped-path residual on the mask:** bounded between **317** (the two
  shipped-only symbols alone) and **4,267** (if `_svfiprintf_r` were entirely the
  asset path); the call-share estimate is **≈1,534**. *The cost split between the
  two callers is NOT measured* — the two call shapes differ (one path formats a
  NitroFS path, the other five `%lu`/`%08lx` conversions) — so this is a bound,
  not a price.
- **The `≤769 tk/frame` bound the board carries is a WHOLE-MATCH figure and it
  still holds.** Whole match: `_svfiprintf_r` 708 tk/fr at 0.39 calls/fr, split
  `sniprintf` 0.09 / `_vsniprintf_r` 0.30 — a **23.1%** shipped share = ≈164,
  plus `sniprintf` 21 and `ndsRelocAssetFindEntry` 22 = **≈207 whole match**.
  The same estimate on the mask is **≈1,526**. **So `≤769` understates the
  rank-80 figure by about 7.4×, to ~1,500 (call-share) with a hard ceiling of
  4,267.**

**Call: engineering, and it is small.** 1,500–4,300 is 4.7–13% of the
requirement. The mechanism is already named in `plan.md` §3 item 9 —
`ndsRelocAssetFindEntry` builds a NitroFS path with `sniprintf` on **every**
animation-asset lookup — and it is a *sub-item of lane 6 below*, not its own
lane: 0.70 lookups per marginal frame is the same rate as the asset acquisitions
that carry it.

---

## 6. What nobody has looked at — and the largest of them is the board's own §9 mechanism, still live under the pack

### 6.1 In-match asset I/O — **100,689 tk/fr at rank-80, +67,454 of it excess**

| symbol | whole tk/fr | **marg tk/fr** | conc | marg calls/fr | call conc |
|---|---:|---:|---:|---:|---:|
| `memcpy` | 13,911 | **33,145** | 2.38× | 327.76 | 2.74× |
| `memset` | 14,238 | **20,735** | 1.46× | 131.53 | 1.38× |
| `armCopyMem32` | 1,774 | **15,791** | **8.90×** | 10.72 | 8.67× |
| `get_fat.isra.0` | 1,477 | **14,140** | **9.57×** | 608.15 | 9.59× |
| `f_lseek` | 926 | **8,899** | **9.61×** | 1.43 | 10.56× |
| `ndsRelocAssetIDForToken` | 688 | **4,220** | 6.13× | 2.86 | 6.45× |
| `ndsRelocNormalizeFighterAObj16File` | 178 | **3,442** | **19.31×** | 0.70 | 7.78× |
| `ndsRelocAssetFindEntry` | 22 | 163 | 7.37× | 0.70 | 7.78× |
| `sniprintf` | 21 | 154 | 7.34× | 0.70 | 7.78× |
| **total** | 33,235 | **100,689** | **3.03×** | | |
| **excess (marg − whole)** | | **+67,454** | | | |

**This is `plan.md` §9's named mechanism, measured on the current tree with the
BattlePack ON, and it is 2.07× the requirement in EXCESS alone.** `get_fat` runs
**608 calls per marginal frame**. `armCopyMem32`'s only two referrers in the
linked ELF are `ntrcardRomRead` and `_dvmCacheCopy` — **cartridge reads during
gameplay.**

**What is NOT settled, and settling it is the next step, not a redesign:** whether
this is unpacked-Mario animation or BGM streaming. `plan.md` §K-RERANK's phase-7
table says Fox (packed) does **0 FAT reads / 0 seeks / 0 relocations / 0
normalizations / 0 path lookups**, while Mario (unpacked, because only one
fighter fits) still does **21 / 42 / 812 / 42 / 833** per match — and SLICE 48
says *"the FAT lane is BGM"*. Both callers exist. The per-fighter counters
already exist and are GO-gated, so **one gate run with them attributes the whole
100,689.**

If it is Mario's: the second fighter's pack is blocked on RAM
(**+258,048 B** needed against an optimistic ceiling of 248,256, `plan.md`
§K-RAM), and K-RAM already names the cheapest lever — *the pack itself is
287,904 B against the 262,144 B it evicts, 1.098× its own displacement*, so a
pack smaller than what it displaces closes it without buying a byte.

### 6.2 The draw side is 170,953 tk/fr at rank-80 and it is FLAT — which makes it the best-converting lane, not the worst

| symbol | **marg tk/fr** | conc | note |
|---|---:|---:|---|
| `ndsFighterMarioFoxDLAllDrawForSlot.constprop.0` | **30,241** | 1.03× | largest single symbol on the mask; 7,544 B, 1.98 calls/fr = 15,273 tk/call |
| `ndsRendererCommitNativeStageSegment` | **27,880** | 1.00× | 2,572 B, 8.00 calls/fr, **15,910 icache** = 0.77 tk/byte/call, *above* the fully-cold rate |
| `ndsRendererNativeEmitProductionPrimitiveGroups` | **26,485** | 1.03× | **0 icache**, 10,287 bus_contention — GX FIFO |
| `ndsRendererExecuteNativeFighterOwnerProduction` | **24,514** | 1.03× | |
| stage no-Z band | **22,608** | 1.00× | §4.1 |
| `ndsRendererNativePrepareProductionRun` | **20,316** | 1.02× | 66.25 calls/fr |
| `ndsRendererMtxMulAffine20p12` | **18,909** | 1.05× | 54.04 calls/fr |
| **total** | **170,953** | **≈1.02×** | **5.2× the requirement** |

> **A correction to hand back, and it matters for the next ranking.**
> `plan.md` §7 demotes the draw side because *"on the 80 frames that set P95 the
> draw side is `FTR` 1.03×, `STG` 1.00×, `MISC` 1.15× — 4.8% of the excess"*.
> That is an **excess** framing, and the gate is a **level**. §0's own law says a
> uniform cut moves P95 by its full value; a 1.00× lane is therefore the *best*
> converting lane there is, not a P50-only one. §7's conclusion is correct for
> the thing it was actually about — moving cold bytes to change *fetch*, which
> `FOOTPRINT.md` §1 has since refuted outright by arithmetic — but it should not
> be read as demoting **deletion of draw work**, which is a different lever and
> the largest flat one on the board.

### 6.3 Animation evaluate/parse — 95,048 tk/fr at rank-80, and one row is 2.05×

| symbol | **marg tk/fr** | conc | marg calls/fr |
|---|---:|---:|---:|
| `ndsR2AnimValueQ` | **27,952** | 1.45× | **391.46** |
| `gcPlayDObjAnimJoint` | **26,691** | 1.36× | 100.12 |
| `ndsR2FtAnimParseDObjFigatree` | **25,960** | **2.05×** | 97.91 |
| `ftParamUpdateAnimKeys` | **14,445** | 1.36× | 5.54 |
| **total** | **95,048** | 1.55× | |

This is §1's architectural step 3 (*compact pose + dirty-joint evaluator*) and it
has never been lane-sized at rank-80. `ndsR2AnimValueQ` at **391 calls per
marginal frame** for 71 tk/call is the shape a table or a dirty-joint gate
attacks; `ndsR2FtAnimParseDObjFigatree` at **2.05×** is the pack's own parse path
and is the only animation row with real percentile structure.

### 6.4 Soft float — 94,602 tk/fr at rank-80, ~97% `issue`, no cache component

`__aeabi_fadd` 43,644 (1.50×, 2,396 calls/fr) · `__mulsf3` 29,507 (1.51×, 2,307
calls/fr) · `__divsf3` 13,737 (1.25×) · `sqrtf` 7,714 (1.14×).
`plan.md` §10 sized this at 82,274 on the **Boundary** arm; on the gate arm at
rank-80 it is **94,602 = 2.9× the requirement**. Its only lever is executing
fewer of them, which is what Task A prices.

**And the asymmetry Task A turns on is visible right here:** `__mulsf3` is 408 B
and pays **0.77 tk of instruction fetch per call** — a body that size would cost
147–320 tk if it were cold, so at 2,307 calls per frame it is **permanently
I-cache resident**. A fixed-point kernel entered ~11 times a frame is not.

### 6.5 One trap, so nobody re-discovers it

The inline-attribution table names `ndsRendererTask29GXRecord` at **16,795 tk/fr**
on the marginal frames. `NDS_TASK29_GX_CENSUS` is **0** in this build, so that
recorder is the `#else` no-op inline and cannot cost anything. This is the
documented `addr2line names deleted and inlined functions` trap in a new costume.
**Do not price it.** The symbol census (which does not list it) is right.
