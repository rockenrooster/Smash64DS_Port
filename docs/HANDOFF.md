# Handoff

Updated: 2026-08-10. **The gate arm's tail was cartridge I/O: the animation
cache arena had been full and refusing loads all match. Fixed at `f082b3c8` —
`WORK-H` P95 1,639,299 → 1,447,318, gap 326,938.** Published pair:
`smash64ds-battle-playable-hwtri.nds` `AFD28273…`, `smash64ds.nds` `54C07FAC…`.
**Every 128-frame measurement in the archive is unusable** — that window reads
the cheapest 6% of the match, understating P95 ~306,000 and the over-gate rate
five times. Use `sample-tick-hud-buckets.ps1 -Samples 1600` (4096 overruns the
match and dies at ring stop 15 of 43).

## The two baselines — label every figure with its arm AND its coverage

Both arms run the **same 60-second match** (coverage 86.7%), windows ending 43
frames past the buzzer. Slips 0 in every row.

| arm | role | `WORK-H` P50 | P95 | over gate |
|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE (owner, 2026-08-05)** | 1,112,576 | **1,447,318** | 754/1600 |
| **Boundary** mode 163 | shipped configuration | 1,082,112 | 1,476,672 | 673/1600 |

**Gate baseline is 1,447,318 as of `f082b3c8`**, less cycle 108's ~23,000 and
cycle 110's ~52,700 `WORK`. Boundary is not re-banked, so 1,476,672 is
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
  2026-08-10. `FTR` is **flat**, on nearly every frame. See cycle 110.
- **Task 56 strips** — REVERT: **the ROM hangs the present loop**; its
  `PERF_LEDGER` KILL row has no completed run behind it.

## RAM: both budgets are near their floor — price a change before writing it

- **Static/boot.** `check-boot-headroom.ps1 -Build <dir>` after every lab build.
  Highest `fake_heap_start` proven to boot **`0x02294804`**, lowest proven to
  fail **`0x02294b24`**. **Text counts as much as bss**; a failing arm reads as
  a hung emulator. **`gSYTaskmanGeneralHeap`** free-min **42,136** against the
  anim cache's 32,768 `KEEP_FREE`; coupled, since freeing `.bss` enlarges it.
- **The `Tex` (dl-pointer, bind-ordinal) memo is REFUTED** — 471 hits of 10,336
  consults, 7,517 of 7,525 fills evicted, `Tex` ticks *up*.

## FTR is re-opened and moving: −56,474 landed, ~314,555 reconciled (cycle 110)

**Banked `FTR` mean is 329,034** against a pre-slice baseline of **385,508**
built and measured for the purpose — that baseline equals the owner's stated
~385–390K, so the reference is right. `ALL` −74,227, `WORK` −52,706. Boundary
passes; `scripts/compare-tick-hud-arms.py` prints every arm. Nine slices, all
deletions or tier moves, none a new abstraction:

- **Task 36 capture hook, out of all five fighter emit loops** (−11,176, **~7,300
  shipped**) — its window only brackets a **stage** run. And **the flat baked
  world compose** (−7,735, exact, same `romSha256` both arms);
  **`BindingParents` is the nearest *bound* ancestor, not the DObj parent.**
- **The `m4x4` intermediates in both per-root matrix loaders** (−3,778) and the
  production root's two 64-byte copies (−11,683): same 64-byte row-major layout,
  so every one of them was copying element i to element i, or copying a copy.
- **The material block is now built 30 times a match, not 59,392** (−14,546 then
  −11,882) — the cycle-98 census found **zero variants** over 20,100 builds. A
  12-byte (MObj, heap gen, complete-input hash) key skips the rebuild, in a row
  owned by the material **DObj**: indexing by selected-root slot rotated between
  frames and was the whole remaining miss. And **the frame summary into
  `.dtcm.fighter`**, recovering the 10,154 that deleting it was worth.

**Two arms are refuted, do not retry them.** Compiling the frame-summary
counters out is worth FTR −7,378 / STG −2,776 and **breaks the gate**:
`verify-all.ps1 -Profile Boundary` runs more than its `-List` row, and
`verify-battle-mariofox-gcrunall-loop-harness.ps1` asserts exact batch and
texture-prepare accounting off those globals. And narrowing the material hash to
the builder's read set returns **bit-identical** counters for +1,155. **Split a
miss counter by reason before theorising about it** — that guess cost a build;
the counter (`MissIdentity=30,606`, `MissInputs=0`) named the fix in one run.

**A census row is not an FTR row.** `FTR` brackets
`ndsFighterDisplayContractSubmit` only, so the flattened parts-invalidation walk
(`ndsFTParamsInvalidateFighterParts`, 86 cycles a joint over 159,748 visits) is a
real −24,215 `WORK` and **−201 `FTR`** — charged to `SRC`/`SINT`. Check the
bracket before sizing a slice off the reconciliation, which is done and
re-runnable with no build: `analyze-fighter-draw-reconciliation.py` resolves
314,555 of the ~331K — matrix 96,207, production driver 54,043, emit 48,115,
adapter driver 44,680, material 35,568, fighter parts 18,711, display contract
17,231 — 28,049 of census-only instrumentation excluded. The two largest groups
spread over 709 and 384 PCs, none above 5.1%: whole-body architecture.

**Next, priced, in FTR:** (1) the display-contract event gather —
`root->preamble.geometry_mode = event->geometry_mode` **2,966** and
`if (event->light_valid)` **1,759**, ~110 cycles an event of pure cache miss
because a 56-byte event straddles two lines and is written a pass earlier; have
the capture pass write the consumer's `NDSRendererNativeFighterPreamble` layout
into its own dense array. (2) the state-delta replay (`ApplyStateSpan` 5,345 +
`ApplyStateDelta` 8,277, ~500 applications a frame over a **static** 70-entry
table and 196-entry sequence) — collapse spans at bake time. **The emit half is
near its floor** (11 instructions, 3 GX words a corner); lower needs a DMA'd
packed stream at ~19–26 KB against ~9,368 B of heap slack, so **RAM is the
blocker**. **Do not cold-split `ndsFighterMarioFoxDLAllDrawForSlot`** — 1,848 of
its 7,108 cold bytes are `NDS_TICK_HUD`-only, absent from the shipped ROM.

**The `SINT` split is DONE and it reordered the queue.** `SINT` +88,082 =
`ftMainPlayAnim` **+60,559** (the animation lane) + `ftComputerProcessAll`
+24,386 (map collision, not AI). `SPHD`/`SHDT`/`SCPU` are not distinct symbol
classes, so `SRC_CPI_OPTIMIZATION.md`'s items 4-6 are retired. Animation is the
largest real discriminator at **72,638 cycles/region, 19.9%**. **The force-load
seam is closed:** `ftmain.c:4623` **discards the return value**, so zero-copy is
structurally impossible — do not add another caching layer to the loader. **The
D-cache census is run** (`analyze-dcache-stalls.py`, no build): loads average
7.07 cyc/ex, excess 17.83%; its largest site is a DMA0CNT spin, not a miss.

**The animation lane is the top `SRC` target: 8.85% of non-idle, ~98,000
ticks/frame at P50**, worth ≈38,700 (~60,000 through to matrices).
`ftAnimParseDObjFigatree` and `gcPlayDObjAnimJoint` are the #1 and #2 soft-float
callers; `AObj` is 36 B × ~360 live = **12,960 B against a 4 KB D-cache**, which
is why `ldrb aobj->kind` costs 24.1 cyc/ex. Constraints on the board: arena not
linked arrays, replace don't coexist, and **derive phase as `frame * step`,
never accumulate** — animation drives hitboxes. Its FTR-side payoff is small:
only `ndsFighterMatrixAngleToIndexExact` (~950) and part of `FloatPow2ToS32`
(2,135) sit inside the bracket, so bank it as a `WORK` lever, not an FTR one.
Also on the board: the sensitivity curve that sizes any proposal (median clears
the gate by only **13,372**) and the CPI table behind "memory-bound" — non-idle
**2.85**, so instruction count is not the lever.

**Do not bring a micro-fix** — R2-06 E11's rule: a load-frame-only ~8,000 cannot
be banked, because relinking moves the tail by more than the saving. Clear
~16,000 in one change — or stack proven deletions into one arm, which is how
cycle 110 banked eight of them. The load frame is priced on the board (premium
650,610/frame; `ndsRelocAssetIDForToken` **CLOSED** as a caching target).
**Measure a placement-sensitive seam on ONE binary with a runtime route** —
`-SetGlobals name=value` pokes a `.data` global at the first frame-complete
marker (standing rule 7), landing after ~3 warm steps, so an OFF arm is partial
and must be scaled; delete the route once the verdict is in. **Every change
needs an engagement counter** — cycle 110 read FTR −13,587 off a skip it could
not prove fired; a `Skip`/`Build` pair showed it firing at only 48.5%.

**Do not re-derive these.** The Makefile's `?= 0` defaults are not the shipped
config (41 overridden). `.text.hot` is closed in both directions
(`linker/nds_hot_text.ld:179-201`); Task 37 census sections C/D are a cost
ranking, never a placement prediction; hoisting the animation range check in
`ndsRelocAssetIDForToken` was done by R2-06 E11 and lost.
**Latent cliff, unowned:** `sNdsAObjEvent32NormalizedCount` reads **973 of 1,024**
after one minute; overflow silently **skips the animation attach**. 8 bytes/entry.
**The load-frame exclusion is REFUTED — do not apply it.** The owner's "loading
states excluded" bar must not go through `SRC > 2x median`: circular for SRC,
swings the gap **3.08x**, drops non-loads (`analyze-load-frame-exclusion.ps1`).
**Boundary for all of it** — a change that alters a visible pixel of the shield,
revival platform, impact wave or reflector needs the owner (`BUGS.md`, by eye).

## Measurement rules that change your FIRST action — board owns the rest

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives
  byte-identical buckets, variance 0. So the 14,080 cross-build figure is
  **placement, not noise**; anything under it needs the `.data` route. Use
  `-Samples 1600 -RingDump -AllowRepeatedFrames`. Ring dumps and per-frame stops
  agree to the tick on one `romSha256`, but a *faster* ROM trips the
  repeated-presented-frame guard 315 times in 1600 on per-frame stops (the 60 Hz
  loop fits two iterations in a presented frame) and 5 on ring dumps. Payload
  IDENTICAL is a stale read and always fatal; DIFFERS is a real second iteration.
- **Judge on `WORK-H`**; buckets locate, they never decide (per-bucket floor
  ≥8,544). **`ALL` is VBlank-quantized** and once hid a +52,928. And **1.85
  cycles of `FTR` mean per byte of added ARM text** — beat your footprint.
- **Disassemble the loop, and read the caller, before designing around it.**
  Cycle 108 built a loader `ftmain.c` discards; cycle 109 aimed a `FTParts` fix
  at two `DObj` fields; cycle 110 found 5 of 16 instructions per fighter corner
  testing a flag no fighter can set, and 3 of 4 matrix conversions copying
  element i to element i. All free to check; none needed a run.
- **Resolve line numbers against the build's own commit** — the profile ELF's
  `NDS_TASK10_GIT_SHORT`, not HEAD. `analyze-symbol-line-profile.py` does it;
  reading c106 against HEAD was ~85 lines adrift, top row on a blank line.

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
ship. Never pass `-j`, never override `MAKEFLAGS`, one build at a time, never
build a published target name for lab work. For a pre-change baseline,
`git checkout HEAD~1 -- src include` then `make p1-tick`, measure, and check the
files back out — flag-identical by construction, which a new `BUILD=` dir is not.
Preserve canonical mode 163, renderer mode 9, mip 0, static textures, source
countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`.
Run `New-Smash64DSSnapshot.ps1` last, and nothing after it.
