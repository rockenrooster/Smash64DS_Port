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

Both arms run the **same 60-second match** (coverage 86.7%), both windows ending
43 frames past the buzzer. Slips 0 in every row.

| arm | role | `WORK-H` P50 | P95 | over gate |
|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE (owner, 2026-08-05)** | 1,112,576 | **1,447,318** | 754/1600 |
| **Boundary** mode 163 | shipped configuration | 1,082,112 | 1,476,672 | 673/1600 |

**Gate baseline is 1,447,318 as of `f082b3c8`**, less cycle 108's ~23,000; head
reads P50 1,107,008 / P95 1,411,283 / 707 over gate. Boundary is not re-banked,
so its 1,476,672 is stale-high. The soak's long match is
`NDS_R2_SOAK_MATCH_MINUTES`, and `probe-match-window.ps1` reads the match timer
out of the guest so a window cannot claim coverage it did not have.

The owner's bar: the whole match under the P95 budget on the both-CPU config,
loading states excluded; the shipped ROM stays the Boundary hwtri pair.
`Makefile:305-308` forbids reporting a both-CPU P95 as the Boundary figure.
**Re-pin `EXPECTED_CENSUS_SHA256` in the commit that changes what it covers** —
a stale pin kept the verifier red for 35 commits.

## What is dead, so nobody re-derives it

- **Effect DObj submits** — Boundary-only: 99.3% of the Boundary excursion but
  **~12.1%** of the gate arm's; G3 refuted cycles 88–91.
- **Projectiles** (44 ticks/frame) · **Particles** (flat ~47,000, a P50 lever only,
  retiring SwitchPlan §7 option 2 as a *gate* answer) · **texture thrash** ·
  **`Find`** · **`Material`** · **`FTR` as the gate** · **the force-load seam**.
- **Task 56 strips** — REVERT: **the ROM hangs the present loop**; its
  `PERF_LEDGER` KILL row has no completed run behind it.

## RAM: both budgets are near their floor — price a change before writing it

- **Static/boot.** `check-boot-headroom.ps1 -Build <dir>` after every lab build.
  Highest `fake_heap_start` proven to boot **`0x02294804`**, lowest proven to fail
  **`0x02294b24`**; the gate arm links at `0x0228c004` for **34,816** proven.
  **Text counts as much as bss**, and a failing arm reads as a hung emulator.
- **`gSYTaskmanGeneralHeap`.** Free-min **42,136** against the anim cache's
  32,768 `KEEP_FREE`; coupled, since freeing `.bss` enlarges the heap.
- **The `Tex` (dl-pointer, bind-ordinal) memo is REFUTED** — 471 hits of 10,336
  consults, 7,517 of 7,525 fills evicted, `Tex` ticks *up*.

## Next single step — port-side rewrite of the figatree parser

**Measure it with the `.data` route. There is no other method.** Build with
`NDS_R2_ANIM_CUT_ROUTE=1` (default 0, and it must stay 0 for anything published
— the disassembly shows a gated-off build folds all the way back), then poke
`-SetGlobals gNdsR2AnimCutRoute=0` for the pre-cut arm. Cycle 109 proved this
works: identical `romSha256` in both arms, poke read back at end of run,
`gNdsR2CubicEvals` **identical at 292,857** in both arms as a
semantic-equivalence control. It priced two animation cuts at **−3,742 mean
`WORK-H`, with `SRC` agreeing at −3,888** — a cut **3.8x smaller than the
14,080-tick placement term that was hiding it.**

**The slice:** the parser's remaining soft float is **≈7.9M cycles ≈ 9,000
ticks/frame** — `fdiv` **1,494,619** at 109.4 cycles a call (most expensive
helper in the build by 3x) on `1.0F / payload` at exactly two sites,
`ftanim.c:170` and `:244`, plus `fsub` 1,753,743, `fadd` 1,454,187, `fcmpeq`
921,383, `fmul` 920,213, `i2f` 796,253. Two facts make it numerically free:
the payload is **a u16** (`relocdata_types.h`), so a reciprocal table hits every
time, and there is **no `-ffast-math`**, so a compile-time `1.0f/n` initializer
is **bit-identical** to the runtime divide. The u16→f32 conversions go through
the already-proven `ndsR2S32ToF32Bits` — a u16 has ≤16 significant bits, so no
rounding occurs at all.

**Do it as a port rewrite of `ftAnimParseDObjFigatree` in
`src/port/reloc_backend_compat_shims.c:1545`**, which already defines that
symbol and today merely forwards. **Not** by extending
`decomp-patches/battleship/src_ft_ftanim.patch`: the owner's 2026-08-06 decision
migrates those eight patches out and lists this one. `battleship_ftanim.c`'s
`#define` renames the parser's definition and call sites **together**, so no
macro can redirect only the calls — that is why the seam has to be the shim.
~300 lines of gameplay-critical transcription; it is unwritten.

**The `SINT` split is DONE and it reordered the queue.** `SINT` +88,082 =
`ftMainPlayAnim` **+60,559** (the animation lane) + `ftComputerProcessAll`
+24,386 (map collision, not AI) = 84,945 of 88,082. `SPHD`/`SHDT`/`SCPU`
**do not appear as distinct symbol classes** — their deltas spread across
collision and soft float, neither competitive — so `SRC_CPI_OPTIMIZATION.md`'s
items 4-6 are retired. Animation is the largest real discriminator at
**72,638 cycles/region, 19.9%**. `FTR` separates the populations by only
+13,768, retiring fighter draw as a gate lever for good.

**The force-load seam is closed (cycle 108).** `ftmain.c:4623` **discards the
return value** and animates from `fp->figatree_heap`, so zero-copy is
structurally impossible. Do not add another caching layer to the loader.

**The D-cache census is run** (`analyze-dcache-stalls.py`, no build): loads
average 7.07 cyc/ex, excess 17.83% of non-idle. Its largest site is **not a
miss** — `ndsRendererTask36ReplayRun` spins on DMA0CNT — and a load after a
`memset` is charged that memset's drain, so the two never add.

**`ndsFTParamsInvalidateFighterParts` is retired (cycle 109), both premises
failed.** The root-joint precondition is false (`TopN` is 0), and its two
expensive loads are **`DObj` fields**, so the dead `FTParts` pool cannot reach
them; 1.40% of non-idle. **The pool is ordinary cleanup, not the fix.**

**The animation lane is the top target: 86,636,950 cycles = 8.85% of non-idle,
~98,000 ticks/frame at P50.** `ftAnimParseDObjFigatree` and `gcPlayDObjAnimJoint`
are the **#1 and #2 soft-float callers in the build**. `AObj` is 36 bytes and
~360 live = **12,960 B against a 4KB D-cache, 3.2x**, which is why `ldrb
aobj->kind` costs **24.1 cyc/ex, 20.9% of `gcPlayDObjAnimJoint`**. Worth
≈38,700 ticks/frame, ~60,000 through to matrices. Constraints on the board:
arena not linked arrays (34,816 B headroom), replace don't coexist, and
**derive phase as `frame * step`, never accumulate** — animation drives hitboxes.

Also on the board: the sensitivity curve that sizes any proposal (median clears
the gate by only **13,372**; a body-wide 50,000 moves 238 frames from 20 to 30
FPS) and the CPI table behind "memory-bound" — non-idle **2.85**,
`ftMainProcUpdateInterrupt` **11.53**. Instruction count is not the lever.

**Do not bring a micro-fix** — R2-06 E11's rule: a load-frame-only ~8,000 cannot
be banked, because relinking moves the tail by more than the saving. Clear
~16,000 in one change, or **move the work off the gameplay frame** (cycle 105's
arena fix, cycle 108's prebake). The load frame itself is priced on the board
(premium 650,610/frame; `ndsRelocAssetIDForToken` **CLOSED** as a caching
target, its two scans still unbounded).

**Measure a placement-sensitive seam on ONE binary with a runtime route.** Two
separately-linked arms of cycle 108's prebake differed only by 3,584 bytes of
scratch and read P50 25,760 apart — 4.5× the cross-build floor — with the
*better* arm reading worse. `sample-tick-hud-buckets.ps1 -SetGlobals name=value`
pokes a `.data` global at the first frame-complete marker (standing rule 7); the
poke lands after ~3 warm steps, so an OFF arm is partial and must be scaled.

**Do not re-derive these.** The Makefile's `?= 0` defaults are not the shipped
config (41 overridden). `.text.hot` is closed in both directions
(`linker/nds_hot_text.ld:179-201`) and the Task 37 census sections C/D are a cost
ranking, never a placement prediction. Hoisting the animation range check in
`ndsRelocAssetIDForToken` was done by R2-06 E11 and lost
(`reloc_backend_assets.c:1840-1895`). These cost one null build to re-learn.

**Latent cliff, unowned:** `sNdsAObjEvent32NormalizedCount` reads **973 of 1,024**
after one minute; overflow silently **skips the animation attach**. 8 bytes/entry.

**The load-frame exclusion is REFUTED — do not apply it.** The owner's "loading
states excluded" bar must not go through `SRC > 2x median`: circular for SRC,
swings the gap **3.08x**, drops frames that are not loads
(`scripts/analyze-load-frame-exclusion.ps1`).

**Boundary for all of it.** Same geometry, textures and materials. A change that
alters a visible pixel of the shield, revival platform, impact wave or reflector
needs the owner (closed `BUGS.md` row, confirmed by eye).

## Measurement rules this cycle established or re-proved

The board's standing-rules section owns the measurement law. The four that
change your FIRST action:

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives
  byte-identical buckets, variance 0. So the 14,080 cross-build figure is
  **placement, not noise**, and no number of runs can average it away. Anything
  under it needs the `.data` route above. Use `-Samples 1600` (4096 overruns the
  match), `-AllowRepeatedFrames`, `-NoBuild`.
- **Judge on `WORK-H`**; buckets locate, they never decide (per-bucket floor
  ≥8,544). **`ALL` is VBlank-quantized** and once hid a +52,928.
- **1.85 cycles of `FTR` mean per byte of added ARM text** — beat your footprint.
- **Disassemble a hot load, and read the caller, before designing around it.**
  Cycle 108 built a loader `ftmain.c` discards; cycle 109 aimed a `FTParts` fix
  at two loads that turned out to be `DObj` fields. Both were free to check.

## Restart surface

Parked items live on the board's **Parked** list, one place not two.

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
ship. Never pass `-j`, never override `MAKEFLAGS`, one build at a time, and
never build a published target name for lab work — those hardcode output to the
project root whatever `BUILD=` says.
Preserve canonical mode 163, renderer mode 9, mip 0, static textures, source
countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`.

Run `New-Smash64DSSnapshot.ps1` last, and nothing after it.
