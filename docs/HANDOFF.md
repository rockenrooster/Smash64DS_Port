# Handoff

Updated: 2026-08-10. **The gate arm's tail was cartridge I/O: the animation
cache arena had been full and refusing loads all match. Fixed at `f082b3c8` —
`WORK-H` P95 1,639,299 → 1,447,318, gap 326,938.** Published pair:
`smash64ds-battle-playable-hwtri.nds` `AFD28273…`, `smash64ds.nds` `54C07FAC…`.
**Every 128-frame measurement in the archive is unusable** — that window reads
the cheapest 6% of the match, understating P95 ~306,000 and the over-gate rate
five times. Use `sample-tick-hud-buckets.ps1 -Samples 1600`.

## The two baselines — label every figure with its arm AND its coverage

Both arms run the **same 60-second match** (coverage 86.7%), windows ending 43
frames past the buzzer. Slips 0 in every row.

| arm | role | `WORK-H` P50 | P95 | over gate |
|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE (owner, 2026-08-05)** | 1,112,576 | **1,447,318** | 754/1600 |
| **Boundary** mode 163 | shipped configuration | 1,082,112 | 1,476,672 | 673/1600 |

**Gate baseline is 1,447,318 as of `f082b3c8`**, less cycle 108's ~23,000 and
cycle 110's `WORK` −21,388 mid-cycle plus a further −11,014 on slice 18.
Boundary is not re-banked, so 1,476,672 is
stale-high. The soak's long match is `NDS_R2_SOAK_MATCH_MINUTES`;
`probe-match-window.ps1` reads the match timer out of the guest so a window
cannot claim coverage it did not have. The owner's bar: the whole match under
the P95 budget on the both-CPU config, loading states excluded; the shipped ROM
stays the Boundary hwtri pair. `Makefile:305-308` forbids reporting a both-CPU
P95 as the Boundary figure. **Re-pin `EXPECTED_CENSUS_SHA256` in the commit that
changes what it covers.**

## What is dead, so nobody re-derives it

- **Effect DObj submits** — Boundary-only: 99.3% of the Boundary excursion but
  **~12.1%** of the gate arm's; G3 refuted cycles 88–91.
- **Projectiles** (44 ticks/frame) · **Particles** (flat ~47,000, a P50 lever only,
  retiring SwitchPlan §7 option 2 as a *gate* answer) · **texture thrash** ·
  **`Find`** · **`Material`** · **the force-load seam**.
- **`FTR` as the *P95 discriminator*** (+13,768 between the populations). NOT
  "FTR is exhausted" — reading it that way is what the owner re-opened on
  2026-08-10, and cycle 110 then took 21.4% off it. `FTR` is **flat**, on nearly
  every frame, which is exactly why it is worth cutting.
- **Task 56 strips** — REVERT: **the ROM hangs the present loop**; its
  `PERF_LEDGER` KILL row has no completed run behind it.

## RAM: both budgets are near their floor — price a change before writing it

- **Static/boot.** `check-boot-headroom.ps1 -Build <dir>` after every lab build.
  Highest `fake_heap_start` proven to boot **`0x02294804`**, lowest proven to
  fail **`0x02294b24`**. **Text counts as much as bss**; a failing arm reads as
  a hung emulator. **`gSYTaskmanGeneralHeap`** free-min **42,136** against the
  anim cache's 32,768 `KEEP_FREE`; coupled, since freeing `.bss` enlarges it.
  **The `Tex` (dl-pointer, bind-ordinal) memo is REFUTED** — 471 hits of 10,336
  consults, 7,517 of 7,525 fills evicted, `Tex` ticks *up*.

## FTR is re-opened and moving: −82,602 landed, 78% named (cycle 110)

**Banked `FTR` mean is 302,906** against a pre-slice baseline of **385,508**
built and measured for the purpose — that baseline equals the owner's stated
~385–390K, so the reference is right. **−82,602, 21.4%.** `WORK` −21,388 from
the mid-cycle bank, `WORK-H` P95 −21,248 on slice 15 alone. Boundary passes on
every landed slice; `scripts/compare-tick-hud-arms.py` prints every arm.

**The biggest single lever was the I-cache, not arithmetic.**
`ndsFighterMarioFoxDLAllDrawForSlot` was the largest non-idle symbol in the ROM
at **4.21 cycles per instruction**, 10,708 bytes against an **8 KB** I-cache,
and **73.6% of it never executed** (1,414 distinct PCs of 4,772 instructions).
Outlining the never-*entered* bodies took it to **7,516** in the instrument and
**7,236** in the proof/published configuration, from ~9,680 — not a tick-HUD
artefact. Recipe, no build needed: `task37_census.py --pc-detail SYM` for the
executed PC set, diff against `objdump`, `addr2line` four points inside each
cold run. Run it on any hot symbol over ~4 KB.

**Entry count is the discriminator, not cold-byte count.** Bodies never
*entered* win (a flag never set, a mode never selected, a fail-closed branch,
a once-per-match primer). Cold bytes inside a body that IS entered lose:
`BuildDObjXObjMatrix` is 72% cold and outlining its four alternate matrix kinds
cost **+14,963**, because every joint enters `GetDObjVectorTracks` and returns
early. And **once the function is under 8 KB the lever is spent** — two more
never-entered bodies then cost **+4,959**.

Fourteen landed slices, all deletions or tier moves, none a new abstraction; the
board carries each one's evidence. The findings that generalise:

- **The DObj world cache has ZERO readers** — `Find` and `BuildDObjWorldMatrix`
  both execute 0 cycles over a match while `Store` burned 4,744,740 and ~4 KB a
  frame of writes through a 4 KB D-cache. **Ask what reads a cache before
  optimising what fills it.**
- **The compose does not fold its base in until a joint contributes** (−10,804):
  one call per binding was *copy the base in, multiply it straight back out*.
- **The material block is built 30 times a match, not 59,392** — a (MObj, heap
  gen, animatable-input hash) key, in a row owned by the material **DObj**, since
  indexing by selected-root slot rotated between frames. `BindingParents` is the
  nearest *bound* ancestor, not the DObj parent. **Same 64-byte row-major layout
  everywhere**, so the `m4x4` intermediates were copying element i to element i.

**Compiling the frame-summary counters out is refuted**: worth FTR −7,378 /
STG −2,776 and it **breaks the gate**, because `verify-all.ps1 -Profile Boundary`
runs more than its `-List` row and `…gcrunall-loop-harness.ps1` asserts exact
batch and texture-prepare accounting off those globals. **A census row is not an
FTR row** either: the bracket is `ndsFighterDisplayContractSubmit` only, so the
flattened parts-invalidation walk is a real −24,215 `WORK` and **−201 `FTR`**.

**Reconciliation, re-run on c115 with an INDEPENDENT tick factor** (0.4993
ticks/cycle, from `ALL` against total cycles — deriving it from the FTR sum is
circular and overstated coverage by 22%): **35 named symbols = 244,774 tk/fr,
78.1% of `FTR`**; the 68,647 residual is bounded by shared leaves straddling FTR
and STG/SRC — float lib 66,750, `memcpy`/`memset` 29,895, texture binds 22,596,
whole-frame totals.

**Next, priced, in FTR** (c115, ticks/frame): production driver
`ExecuteNativeFighterOwnerProduction` **26,307** + `NativePrepareProductionRun`
**25,286**; state replay `Task36ReplayRun` **17,796** + `ApplyStateDelta`
**9,010**, ~500 applications a frame over a **static** 70-entry table and
196-entry sequence — collapse spans at bake time; `BuildFighterTraRotRpyDirect`
**17,704** (97.9% hot, nothing to place); `LoadHardwareSplitMatrices` **13,126**,
whose E23 projection-skip stays refuted (a content-keyed skip costs **+4,566**;
a 64-byte `memcmp` beats eighteen FIFO writes). **The emit half is near its
floor** (11 instructions, 3 GX words a corner); lower needs a DMA'd packed
stream at ~19–26 KB against ~9,368 B of heap slack, so **RAM is the blocker**.
**`FIXEDPOINT_ANIMATION.md` is still unimplemented** and is a `WORK`/`SRC`
lever, not an `FTR` one — `__aeabi_fadd` alone is 31,245 tk/fr whole-frame.

**The `SINT` split is DONE and it reordered the queue.** `SINT` +88,082 =
`ftMainPlayAnim` **+60,559** (the animation lane) + `ftComputerProcessAll`
+24,386 (map collision, not AI). `SPHD`/`SHDT`/`SCPU` are not distinct symbol
classes, so `SRC_CPI_OPTIMIZATION.md`'s items 4-6 are retired. **The force-load
seam is closed:** `ftmain.c:4623` **discards the return value**. **The D-cache
census is run** (`analyze-dcache-stalls.py`, no build): loads average 7.07
cyc/ex, excess 17.83%; its largest site is a DMA0CNT spin, not a miss.

**The animation lane is the top `SRC` target: 8.85% of non-idle, ~98,000
ticks/frame at P50**, worth ≈38,700 (~60,000 through to matrices).
`ftAnimParseDObjFigatree` and `gcPlayDObjAnimJoint` are the #1 and #2 soft-float
callers; `AObj` is 36 B × ~360 live = **12,960 B against a 4 KB D-cache**, which
is why `ldrb aobj->kind` costs 24.1 cyc/ex. Constraints on the board: arena not
linked arrays, replace don't coexist, **derive phase as `frame * step`, never
accumulate** (it drives hitboxes). A `WORK` lever: ~3,085 is inside `FTR`.

**Do not bring a micro-fix** — R2-06 E11's rule: a load-frame-only ~8,000 cannot
be banked, because relinking moves the tail by more than the saving. Clear
~16,000 in one change, or stack proven deletions into one arm — cycle 110 banked
fourteen. The load frame is priced on the board (premium 650,610/frame;
`ndsRelocAssetIDForToken` **CLOSED** as a caching target). **Every change needs
an engagement counter** — cycle 110 read FTR −13,587 off a skip it could not
prove fired.

**Do not re-derive these.** The Makefile's `?= 0` defaults are not the shipped
config (41 overridden). `.text.hot` is closed in both directions
(`linker/nds_hot_text.ld:179-201`) and measures **3.30 cyc/insn, worse than
`.main`**; census sections C/D are a cost ranking, never a placement prediction.
**Latent cliff, unowned:** `sNdsAObjEvent32NormalizedCount` reads **973 of
1,024** after one minute; overflow silently **skips the animation attach**.
**The load-frame exclusion is REFUTED — do not apply it.** The owner's "loading
states excluded" bar must not go through `SRC > 2x median`: circular for SRC,
swings the gap **3.08x**, drops non-loads. **Boundary for all of it** — a change
altering a visible pixel of the shield, revival platform, impact wave or
reflector needs the owner (`BUGS.md`, by eye).

## Measurement rules that change your FIRST action — board owns the rest

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives
  byte-identical buckets, variance 0 — re-proven cycle 110 when a reverted slice
  rebuilt its predecessor's ROM sha and every bucket to the tick. So the 14,080
  cross-build figure is **placement, not noise**; anything under it needs the
  `.data` route. Use `-Samples 1600 -RingDump -AllowRepeatedFrames`; a *faster*
  ROM trips the repeated-presented-frame guard on per-frame stops (the 60 Hz loop
  fits two iterations in a presented frame). Payload IDENTICAL is a stale read
  and always fatal; DIFFERS is a real second iteration.
- **Judge on `WORK-H`**; buckets locate, they never decide (per-bucket floor
  ≥8,544). **`ALL` is VBlank-quantized** and once hid a +52,928. And **1.85
  cycles of `FTR` mean per byte of added ARM text** — beat your footprint; a
  116-byte seed setup ate a slice's entire win.
- **Disassemble the loop, read the caller, and TAKE THE ENTRY-PC COUNT, before
  designing around it.** Cycle 108 built a loader `ftmain.c` discards; cycle 109
  aimed a `FTParts` fix at two `DObj` fields; cycle 110 published a wrong
  mechanism for a slice by dividing a symbol total by a guessed per-call cost
  instead of reading the prologue's execution count. Free (`--pc-detail`).
- **Resolve line numbers against the build's own commit** — the profile ELF's
  `NDS_TASK10_GIT_SHORT`, not HEAD; c106 against HEAD was ~85 lines adrift.

## Restart surface — parked items live on the board's **Parked** list

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

`docs/P1_EXECUTION_BOARD.md` is the only dynamic queue (history in
`docs/optimization/archive/P1_EXECUTION_BOARD_pre-cycle79.md`);
`Smash64DS_Runtime2_SwitchPlan.md` is the charter; `docs/BUGS.md` carries the
owner's verdicts — preserve their wording.
A clean checkout must build through `build.ps1`, not bare `make`: four of six
generated `.inc` files are gitignored. `make p1-tick` builds the measuring ROM,
`make p1` the published battle pair; bare `make` builds the P2 ROM P1 does not
ship. Never pass `-j`, never override `MAKEFLAGS`, one build at a time, never
build a published target name for lab work.
Preserve canonical mode 163, renderer mode 9, mip 0, static textures, source
countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`.
Run `New-Smash64DSSnapshot.ps1` last, and nothing after it.
