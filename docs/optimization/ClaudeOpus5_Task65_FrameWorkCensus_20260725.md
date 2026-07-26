# Task 65 — The real work budget: idle, stall, and where the ticks actually are

**Date:** 2026-07-25
**Status:** Census complete. No shipping code changed.
**Instrument:** per-PC ARM9 profiler in the repo melonDS build, windowed by the
Task 37 CP15 markers; new attribution script
`scripts/task65_subsystem_census.py`.
**Inputs:** `artifacts/task65-census/` (arm9-profile.csv, census.txt,
subsystem.txt), ROM `smash64ds-battle-playable-tickhud-hwtri`,
`NDS_TASK37_PROFILE=1`, window presented frames 438–566 (128 frames),
473,920,686 cycles over 58,769 distinct program counters.

Task 64 established that the tick-HUD `ALL` bucket is VBlank-quantized wall
time and so cannot be used to *search* for savings. This task builds the
search-side instrument and answers the question the plan was blocked on: how
much of the frame is real work, and where does it live.

## 0. Units, and one correction to carry forward

The melonDS profiler counts ARM9 core cycles at **67.028 MHz**. The ROM's tick
HUD counts the system timer through `cpuGetTiming()` at **33.514 MHz**. The
factor is exactly **2**, and every number below is stated in tick-HUD units so
it compares directly against the `P95 ≤ 1.12M` budget in `PROJECT_GOAL.md`.

Cross-check: window mean wall 1,851,253 ticks/frame against the Task 56 control
VBlank histogram (3:474, 4:80, 5+:12 → 3.194 VBlanks × 560,190 = 1,789,247).
Agreement to 3.5% across two different runs.

## 1. The frame

| | ticks/frame | share |
|---|---|---|
| wall (what `ALL` measures) | 1,851,253 | 100% |
| **idle VBlank wait** (`armWaitForIrq`) | **323,976** | 17.50% |
| **REAL WORK** | **1,527,277** | 82.50% |
| 30 FPS budget (`PROJECT_GOAL.md`) | 1,120,000 | |
| **GAP** | **407,277** | |

**This resolves the 285K–560K ambiguity: the gap is 407,277 ticks.**

`OTHR` was 275,008 in the Task 56 control and idle here is 323,976 — close
enough that **`OTHR` is essentially the VBlank wait**. GX pipe-full stall is
*not* pooled in `OTHR`; it is distributed inside the named buckets as memory
stall on the instruction that could not retire, which is where a bus wait
belongs and where a bucket-level view cannot see it.

### The composition of that work is the surprise

| | ticks/frame | share of work |
|---|---|---|
| retired instructions | 576,751 | 37.8% |
| **memory stall** (bus, cache, GX backpressure) | **587,532** | **38.5%** |
| non-memory stall (fetch, interlock, refill) | 362,994 | 23.8% |

**62% of the real work is stall, not instruction retirement.** Memory stall
alone (587,532) is larger than the entire gap to 30 FPS (407,277).

This matters for how the next tasks are judged. Specialization that removes
*instructions* is attacking 37.8% of the frame. Removing a load, a cache miss,
a structure walk or a GX write attacks the other 62%. A rewrite that halves the
instruction count of a routine while touching the same memory in the same order
will underdeliver against its estimate, and the estimate is what the plan's
per-task targets are built from.

## 2. Where the work is — by source file, from DWARF, not from names

| ticks/frame | %work | file |
|---|---|---|
| 617,426 | 40.43% | `src/nds/nds_renderer.c` |
| 187,742 | 12.29% | *(no line info — libc `mem*`, `__ieee754_sqrtf`)* |
| 147,465 | 9.66% | `src/port/reloc_backend_renderer_dl.c` |
| 70,548 | 4.62% | `src/nds/nds_task16_float_addsub.s` |
| 69,336 | 4.54% | libgcc `ieee754-sf.S` |
| 50,620 | 3.31% | `decomp/…/src/sys/objanim.c` |
| 34,726 | 2.27% | `src/port/reloc_backend_compat_shims.c` |
| 23,937 | 1.57% | `src/port/reloc_backend_assets.c` |
| 23,032 | 1.51% | `decomp/…/src/ft/ftmain.c` |
| 22,675 | 1.48% | `src/port/reloc_backend_mp_collision.c` |
| 21,300 | 1.39% | libnds `videoGL.h` |
| 17,206 | 1.13% | `decomp/…/src/sys/objman.c` |
| 17,112 | 1.12% | calico `ntrcard.c` |
| 16,618 | 1.09% | `decomp/…/src/ft/ftanim.c` |
| 15,151 | 0.99% | `decomp/…/src/ft/ftdisplaymain.c` |

Rolled up:

- **Renderer — 794,093 ticks, 52.0% of all work.**
  `nds_renderer.c` + `reloc_backend_renderer_dl.c` + libnds `videoGL`.
- **BattleShip simulation — ~155,000 ticks, ~10% of work.** Everything under
  `decomp/` combined, and *half of that* is `objanim.c` / `ftdisplaymain.c` /
  `ftanim.c`, which is animation and display, not gameplay rules.
  **Gameplay simulation proper is roughly 60,000–80,000 ticks.**
- **Cart reads during gameplay — 17,112 ticks.** `_ntrcardRecvByCpu` runs every
  frame in steady state.

## 3. Where the work is — by cross-cutting kernel

| kernel | ticks/frame | %work |
|---|---|---|
| soft-float | 176,585 | 11.56% |
| `mem*` | 138,640 | 9.08% |
| matrix | 137,016 | 8.97% |
| texture-resolve | 114,031 | 7.47% |
| gx-submit | 90,110 | 5.90% |
| rom-read | 17,112 | 1.12% |
| everything else | 853,781 | 55.90% |

These five kernels total **656,382 ticks, 43% of the work** — and they are
spread across every subsystem, so a plan organised only by tick-HUD bucket
rediscovers them from each end and pays for them twice.

Note `soft-float` is already partly hand-written (`nds_task16_float_addsub.s`,
1.19 cycles/instruction). It is not slow per operation; there is simply a great
deal of it. The lever is fixed-point conversion, not a faster float routine —
which `PROJECT_GOAL.md` lists explicitly under allowed techniques.

## 4. Top functions

| ticks/frame | %work | cyc/insn | function |
|---|---|---|---|
| 323,976 | — | — | `armWaitForIrq` *(idle)* |
| 69,847 | 4.57 | 1.19 | `__aeabi_fadd` |
| 59,725 | 3.91 | 2.22 | `memcpy` |
| 57,356 | 3.76 | 3.70 | `memset` |
| 49,964 | 3.27 | 1.13 | `__aeabi_fmul` |
| 39,434 | 2.58 | 4.51 | `ndsRendererTask36ReplayRun` |
| 36,022 | 2.36 | 3.20 | `ndsRendererNativeShadeProductionActions` |
| 33,876 | 2.22 | 2.20 | `gcPlayDObjAnimJoint` |
| 29,001 | 1.90 | 6.94 | `ndsRendererHardwareResolveOrBindTexture` |
| 24,703 | 1.62 | 2.53 | `ndsRendererTask29GXRecord` |
| 23,800 | 1.56 | 1.60 | `ndsRendererMtxMul20p12` |
| 19,503 | 1.28 | 1.91 | `memcmp` |
| 18,707 | 1.22 | 4.28 | `ndsRendererNativeApplyStateDelta` |
| 17,591 | 1.15 | 7.19 | `ndsFTParamsInvalidateFighterParts` |
| 16,689 | 1.09 | 4.68 | `ndsRendererSyncTextureTile` |
| 14,956 | 0.98 | 6.39 | `glLoadMatrix4x4` |

`ndsRendererTask29GXRecord` at 24,703 is the GX command funnel that carries the
Task 36 replay recorder. `NDS_TASK36_HW_COMPOSE := 2` is on the published block
as well as the tick-HUD block, so this cost is in the shipping ROM, not
instrument overhead.

`ndsPlatformTickHudSort` (8,384) and `ndsPlatformRenderDebugHud` (~12,414 for
`nds_platform.c` in total) **are** tick-HUD-only. Subtract roughly 15,000 ticks
when comparing to the published profile-0 ROM.

## 5. What this does to the proposed plan

1. **The gap is 407,277 ticks.** The proposed accumulation totalled 400K, which
   would close it with no margin and with every line item estimated before this
   measurement existed. Budget ~500K.

2. **Task 68's premise does not survive.** It targets `SRC ≈ 320K` for a 75–125K
   saving by replacing generic dispatch with `Mario_Update()` / `FoxCPU3_Update()`.
   All BattleShip code together is ~155K, and gameplay simulation proper is
   ~60–80K. The saving is not available at that size. The `SRC` tick-HUD bucket
   must be charging renderer-side work performed during the source update —
   `gcPlayDObjAnimJoint` at 33,876 is the visible piece.

3. **The renderer is 52% of the work and should be the campaign.** Tasks 66 and
   67 are aimed correctly; they are simply smaller pieces of one much larger
   target than the plan implies.

4. **Task 67 is confirmed and should go first.** `ResolveOrBindTexture` +
   `SyncTextureTile` + `FindTexture` + `BindTextureName` + `CaptureTextureLoad`
   = 114,031 ticks at 5.74 cycles/instruction — a stall-dominated profile, which
   is exactly what repeated identity-proving looks like. No gameplay gate, no
   visual change, and `PROJECT_GOAL.md`'s "compute once, not every frame"
   endorses it directly.

5. **Two levers the plan does not contain**, both allowed outright by
   `PROJECT_GOAL.md`:
   - **Soft-float → fixed point: 176,585 ticks**, the largest single kernel.
   - **Cart reads in steady state: 17,112 ticks.** Small, but "loading time is
     cheap" and "RAM is a performance resource" make preloading nearly free.

6. **Measure against stall, not instruction count.** 62% of the work is stall.
   Per-task targets derived from "how many instructions will this remove" will
   overstate. `cyc/insn` in the table above is the screening column: anything
   above ~4 is waiting, not computing.

## 6. The one instrument still missing

This census reports a **mean over 128 frames**. It cannot produce P50/P95,
because the profiler aggregates the window. The acceptance gate in
`PROJECT_GOAL.md` is a **P95** on work — and no instrument in the repo measures
per-frame work today, only per-frame wall time.

**Recommendation: add a `WAIT` bucket to the tick HUD** that accumulates the
VBlank idle span directly, so `work = ALL − WAIT` becomes a per-frame quantity
with real percentiles. That is a small change, and it converts "search against
work, not `ALL`" from a discipline every future task has to remember into a
number every A/B reads for free. It is the structural fix for the failure that
produced four uninformative verdicts in Tasks 53, 55, 56 and 63.

## 7. Reproducing

```
scripts/run-task37-profile-census.ps1 -Build build-task65-profile \
    -StartFrame 438 -Frames 128 -OutDir <ABSOLUTE PATH>
python scripts/task65_subsystem_census.py artifacts/task65-census/arm9-profile.csv \
    --elf builds/build-task65-profile/smash64ds-battle-playable-tickhud-hwtri.elf \
    --addr2line C:/devkitPro/devkitARM/bin/arm-none-eabi-addr2line.exe \
    --frames 128 --root . --top 45
```

Throughput is **0.76 presented frames/second** — the profiler forces the
cache-accurate interpreter and accounts every instruction. Reaching frame 566
takes 12–13 minutes; budget 45 for a cold run including the build.

`-OutDir` **must be absolute**. melonDS runs with its working directory set to
`emulators/melonds/` and opens `MELONDS_ARM9_PROFILE_CSV` verbatim, so a
relative path silently writes nothing and the whole run is lost with no error.
Fixed defensively in `scripts/run-task37-profile-census.ps1` on 2026-07-25 after
it cost one run here.
