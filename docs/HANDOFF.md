# Handoff

Updated: 2026-08-10. **The gate arm's tail was cartridge I/O: the animation
cache arena had been full and refusing loads all match. Fixed at `f082b3c8` —
`WORK-H` P95 1,639,299 → 1,447,318, gap 326,938.** Published pair:
`smash64ds-battle-playable-hwtri.nds` `AFD28273…`, `smash64ds.nds` `54C07FAC…`.

## Read this first: every 128-frame measurement in the archive is unusable

**The 128-frame window reads the cheapest 6% of the match** — P95 understated
~306,000 and the over-gate rate five times. Use `sample-tick-hud-buckets.ps1`
with **`-Samples 1600`** (4096 overruns the match and dies at ring stop 15 of
43). Never take a gate reading on 128 frames again.

## The two baselines — label every figure with its arm AND its coverage

Both arms run the **same 60-second match** (coverage 86.7%), windows ending 43
frames past the buzzer. Slips 0 in every row.

| arm | role | `WORK-H` P50 | P95 | over gate |
|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE (owner, 2026-08-05)** | 1,112,576 | **1,447,318** | 754/1600 |
| **Boundary** mode 163 | shipped configuration | 1,082,112 | 1,476,672 | 673/1600 |

**Gate baseline is 1,447,318 as of `f082b3c8`**, less cycle 108's ~23,000 and
cycle 110's ~22,800; head reads P50 1,107,008 / P95 1,411,283 / 707 over gate.
Boundary is not re-banked, so its 1,476,672 is stale-high. The soak's long match
is `NDS_R2_SOAK_MATCH_MINUTES`, and `probe-match-window.ps1` reads the match
timer out of the guest so a window cannot claim coverage it did not have.

The owner's bar: the whole match under the P95 budget on the both-CPU config,
loading states excluded; the shipped ROM stays the Boundary hwtri pair.
`Makefile:305-308` forbids reporting a both-CPU P95 as the Boundary figure.
**Re-pin `EXPECTED_CENSUS_SHA256` in the commit that changes what it covers.**

## What is dead, so nobody re-derives it

- **Effect DObj submits** — Boundary-only: 99.3% of the Boundary excursion but
  **~12.1%** of the gate arm's; G3 refuted cycles 88–91.
- **Projectiles** (44 ticks/frame) · **Particles** (flat ~47,000, a P50 lever only,
  retiring SwitchPlan §7 option 2 as a *gate* answer) · **texture thrash** ·
  **`Find`** · **`Material`** · **the force-load seam**.
- **`FTR` as the *P95 discriminator*** (+13,768 between the populations). That is
  NOT "FTR is exhausted" — reading it that way is what the owner re-opened on
  2026-08-10. `FTR` is ~363,000 **flat**, on nearly every frame. See cycle 110.
- **Task 56 strips** — REVERT: **the ROM hangs the present loop**; its
  `PERF_LEDGER` KILL row has no completed run behind it.

## RAM: both budgets are near their floor — price a change before writing it

- **Static/boot.** `check-boot-headroom.ps1 -Build <dir>` after every lab build.
  Highest `fake_heap_start` proven to boot **`0x02294804`**, lowest proven to
  fail **`0x02294b24`**. **Text counts as much as bss**; a failing arm reads as
  a hung emulator.
- **`gSYTaskmanGeneralHeap`.** Free-min **42,136** against the anim cache's
  32,768 `KEEP_FREE`; coupled, since freeing `.bss` enlarges the heap.
- **The `Tex` (dl-pointer, bind-ordinal) memo is REFUTED** — 471 hits of 10,336
  consults, 7,517 of 7,525 fills evicted, `Tex` ticks *up*.

## FTR is re-opened and moving: −22,689 landed, ~314,555 reconciled (cycle 110)

**Banked `FTR` mean is now 362,819** against a pre-slice baseline of **385,508**
built and measured for the purpose — that baseline equals the owner's stated
~385–390K, so the reference is right. `WORK-H` mean −22,844. Boundary passes, the
visibility screenshot shows both fighters correctly articulated, and every
control bucket drifts under ±950 and non-monotonically across the four arms while
`FTR` falls monotonically (`scripts/compare-tick-hud-arms.py` prints the table).
Three deletions, all abstractions that did not belong on the fighter path:

- **Task 36 capture hook, out of all five fighter emit loops** (−11,176 measured,
  **~7,300 shipped** — the effect-packet half is `NDS_TICK_HUD`-only). The
  capture window only ever brackets a **stage** run, so it could never fire.
  Untextured emit 19 → 11 instructions a corner, textured 25 → 14.
- **The flat baked world compose** (−7,735, exact, same `romSha256` both arms,
  3,951 calls / 0 rejects, every other bucket flat ±40). Graduated: route and
  counters deleted. **`BindingParents` is the nearest *bound* ancestor, not the
  DObj parent** — the chain between two bindings still has to be walked.
- **The `m4x4` intermediates in both per-root matrix loaders** (−3,778).
  `NDSRendererMatrix20p12` and libnds' `m4x4` are the same 64-byte row-major
  layout, so the conversion was copying element i to element i.

**The reconciliation is done and re-runnable with no build:**
`scripts/analyze-fighter-draw-reconciliation.py` resolves 314,555 of the ~331K to
named symbols — matrix 96,207, production driver 54,043, emit 48,115, adapter
driver 44,680, material 35,568, fighter parts 18,711, display contract 17,231 —
with 28,049 of census-only instrumentation excluded. The two largest groups
spread over 709 and 384 PCs, none above 5.1%: whole-body architecture cost.

**The next architecture is the per-run descriptor: 89,611 ticks/frame** over
~30–37 runs a frame (production driver 54,043 + material 35,568 ≈ 2,400 a run),
re-deriving texture params, poly format and UV scale every frame for runs whose
descriptors are immutable. It needs no new RAM. **The emit half is near its
floor** (11 instructions, 3 GX words a corner); lower needs a DMA'd packed
stream at ~19–26 KB against ~9,368 B of heap slack — **RAM is the blocker, not
the mechanism.** Also priced: the two 64-byte copies in
`ndsRendererNativeBindProductionRoot` (5,958, half read either way). **Do not
cold-split `ndsFighterMarioFoxDLAllDrawForSlot`** — 1,848 of its 7,108 cold bytes
are `NDS_TICK_HUD`-only and absent from the shipped ROM.

**The `SINT` split is DONE and it reordered the queue.** `SINT` +88,082 =
`ftMainPlayAnim` **+60,559** (the animation lane) + `ftComputerProcessAll`
+24,386 (map collision, not AI). `SPHD`/`SHDT`/`SCPU` are not distinct symbol
classes, so `SRC_CPI_OPTIMIZATION.md`'s items 4-6 are retired. Animation is the
largest real discriminator at **72,638 cycles/region, 19.9%**.

**The force-load seam is closed (cycle 108).** `ftmain.c:4623` **discards the
return value** and animates from `fp->figatree_heap`, so zero-copy is
structurally impossible. Do not add another caching layer to the loader.
**The D-cache census is run** (`analyze-dcache-stalls.py`, no build): loads
average 7.07 cyc/ex, excess 17.83% of non-idle; its largest site is a DMA0CNT
spin, not a miss, and a load after a `memset` is charged that memset's drain.

**`ndsFTParamsInvalidateFighterParts`: the *pool* fix is refuted** — the
root-joint precondition is false (`TopN` is 0) and its two expensive loads are
`DObj` fields, so the dead `FTParts` pool cannot reach them. But it is **15,777
ticks/frame**, not the 6,560 cycle 109 quoted, so the flattened subtree sweep is
still worth doing on its own.

**The animation lane is the other top target: 8.85% of non-idle, ~98,000
ticks/frame at P50**, worth ≈38,700 (~60,000 through to matrices).
`ftAnimParseDObjFigatree` and `gcPlayDObjAnimJoint` are the #1 and #2 soft-float
callers; `AObj` is 36 B × ~360 live = **12,960 B against a 4 KB D-cache**, which
is why `ldrb aobj->kind` costs 24.1 cyc/ex. Constraints on the board: arena not
linked arrays, replace don't coexist, and **derive phase as `frame * step`,
never accumulate** — animation drives hitboxes.

Also on the board: the sensitivity curve that sizes any proposal (median clears
the gate by only **13,372**; a body-wide 50,000 moves 238 frames from 20 to 30
FPS) and the CPI table behind "memory-bound" — non-idle **2.85**. Instruction
count is not the lever.

**Do not bring a micro-fix** — R2-06 E11's rule: a load-frame-only ~8,000 cannot
be banked, because relinking moves the tail by more than the saving. Clear
~16,000 in one change — or stack several proven deletions into one arm, which is
how cycle 110's 11,176 + 7,735 + 3,778 banked. The load frame is priced on the
board (premium 650,610/frame; `ndsRelocAssetIDForToken` **CLOSED** as a caching
target, its two scans still unbounded).

**Measure a placement-sensitive seam on ONE binary with a runtime route.** Two
separately-linked arms of cycle 108's prebake read P50 25,760 apart — 4.5× the
cross-build floor — with the *better* arm reading worse. `-SetGlobals name=value`
pokes a `.data` global at the first frame-complete marker (standing rule 7); the
poke lands after ~3 warm steps, so an OFF arm is partial and must be scaled.
Delete the route once the verdict is in — cycle 110 graduated one immediately.

**Do not re-derive these.** The Makefile's `?= 0` defaults are not the shipped
config (41 overridden). `.text.hot` is closed in both directions
(`linker/nds_hot_text.ld:179-201`) and Task 37 census sections C/D are a cost
ranking, never a placement prediction. Hoisting the animation range check in
`ndsRelocAssetIDForToken` was done by R2-06 E11 and lost.

**Latent cliff, unowned:** `sNdsAObjEvent32NormalizedCount` reads **973 of 1,024**
after one minute; overflow silently **skips the animation attach**. 8 bytes/entry.
**The load-frame exclusion is REFUTED — do not apply it.** The owner's "loading
states excluded" bar must not go through `SRC > 2x median`: circular for SRC,
swings the gap **3.08x**, drops non-loads (`analyze-load-frame-exclusion.ps1`).

**Boundary for all of it.** Same geometry, textures and materials. A change that
alters a visible pixel of the shield, revival platform, impact wave or reflector
needs the owner (closed `BUGS.md` row, confirmed by eye).

## Measurement rules that change your FIRST action — board owns the rest

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives
  byte-identical buckets, variance 0. So the 14,080 cross-build figure is
  **placement, not noise**, and no number of runs averages it away. Anything
  under it needs the `.data` route. Use `-Samples 1600`, `-AllowRepeatedFrames`,
  `-NoBuild`.
- **Judge on `WORK-H`**; buckets locate, they never decide (per-bucket floor
  ≥8,544). **`ALL` is VBlank-quantized** and once hid a +52,928.
- **1.85 cycles of `FTR` mean per byte of added ARM text** — beat your footprint.
- **Disassemble the loop, and read the caller, before designing around it.**
  Cycle 108 built a loader `ftmain.c` discards; cycle 109 aimed a `FTParts` fix
  at two `DObj` fields; cycle 110 found 5 of 16 instructions per fighter corner
  testing a flag no fighter can ever set, and 3 of 4 matrix conversions copying
  element i to element i. All four were free to check and none needed a run.

## Restart surface — parked items live on the board's **Parked** list

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

`docs/P1_EXECUTION_BOARD.md` is the only dynamic queue (history in
`docs/optimization/archive/P1_EXECUTION_BOARD_pre-cycle79.md`);
`Smash64DS_Runtime2_SwitchPlan.md` is the charter; `docs/BUGS.md` carries the
owner's verdicts — they edit it directly, so preserve their wording.

A clean checkout must build through `build.ps1`, not bare `make`: four of six
generated `.inc` files are gitignored. `make p1-tick` builds the measuring ROM,
`make p1` the published battle pair; bare `make` builds the P2 ROM P1 does not
ship. Never pass `-j`, never override `MAKEFLAGS`, one build at a time, and never
build a published target name for lab work. For a pre-change baseline,
`git checkout HEAD~1 -- src include` then `make p1-tick`, measure, and check the
files back out — it is flag-identical by construction, which a new `BUILD=` dir
is not.
Preserve canonical mode 163, renderer mode 9, mip 0, static textures, source
countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`.
Run `New-Smash64DSSnapshot.ps1` last, and nothing after it.
