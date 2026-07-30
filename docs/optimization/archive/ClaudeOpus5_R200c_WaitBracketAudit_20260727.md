# R2-00c — the WAIT bracket is accurate, and the excursion is real work

**Date:** 2026-07-27
**Phase:** R2-00 fallout. Board row *"the gate metric is measuring phantom
work"* — the highest-value row on `docs/P1_EXECUTION_BOARD.md`.
**Verdict: the claim is refuted.** The tick-HUD `WAIT` bracket is accurate to a
constant **−851 ticks/frame** on clean frames and excursion frames alike. The
gate metric is sound.

---

## 1. What R2-00a claimed, and why it looked true

R2-00a measured the ARM9's halted cycles with the new stall attributor and found
them essentially equal on excursion and median frames (796,250 vs 801,881, 0.7%
apart) while the tick HUD reported `WAIT` as 210,752 vs 804,736. Since
`WORK = ALL − WAIT`, idle the HUD failed to count would reappear as work that
never happened — 588,353 ticks against the HUD's own claimed +593,856, which is
99.1% agreement and looks like a closed case.

**The two numbers came from two different binaries.** Halt was measured in a
`NDS_TASK37_PROFILE=1` profile ROM; `WAIT` was measured in a tick-HUD ROM. Those
are different builds with different code placement, so frame 453 does not name
the same workload in both, and the comparison has no ground.

This was not carelessness — before this audit there was no ROM carrying both
instruments, so a cross-binary comparison was the only one available. The fix is
in §6.

## 2. The experiment

One ROM with **both** instruments: `smash64ds-battle-playable-tickhud-hwtri`
built `NDS_TASK37_PROFILE=1 NDS_TASK37_PROFILE_PER_FRAME_REGION=1`. One run.
Both readings come out of the same 128 frames of the same binary.

The alignment is exact by construction. In `taskman_seam.c:4972` the profiler
marker is written **immediately before** `ndsBattlePlayableFrameCompleteMarker`,
which is where the sampler's GDB stop lands, and after
`ndsPlatformTickHudSample()` has latched the ring entry. So the profiler's
reset/dump and the ring's 128 entries bracket identical iterations.

`NDS_TASK37_PROFILE_PER_FRAME_REGION` is new here. The profiler already keyed
its accumulators on a 16-bit region id set through the same CP15 marker channel
(`std::unordered_map<u16, RegionEntry>`), and nothing was using it. Numbering
each frame as its own region turns a window total into 128 per-frame rows, at
the cost of one extra CP15 write per frame in profile builds. That matters
because a window total cannot separate "accurate everywhere" from "over-counts
on clean frames, under-counts on the tail" — and the tail is what the P95 gate
is decided on.

## 3. Result

Emulator minus tick-HUD, per frame, 128 frames:

| quantity | median Δ | max abs Δ | median relative |
|---|---|---|---|
| `ALL` vs profiler `total_cycles`/2 | **+685** | 802 | **0.04%** |
| `WAIT` vs profiler `halt_wait`/2 | **−851** | 142,428 | **−0.23%** |

`ALL` agreeing to 0.04% confirms both the window alignment and the ×2
cycles→ticks conversion independently, before `WAIT` is read at all.

**125 of 128 frames sit within ±2,000 ticks of the constant −851 offset.** The
one large outlier is frame 439 — the first region after the reset marker, whose
boundary does not coincide with the HUD's frame origin, and which is also a HUD
console-redraw frame. It is a boundary artifact of region 1, not a mechanism.

The split that decides the claim:

| population | frames | median Δ (`halt`/2 − `WAIT`) |
|---|---|---|
| `WAIT` below 60% of median (the excursion frames) | 27 | **−860** |
| everything else | 100 | **−847** |

**Identical.** The bracket is exactly as accurate on the frames R2-00a's claim
was about as it is everywhere else. There is no 293,000-tick/frame shortfall,
and the HUD reading slightly *high* by a constant is the expected shape: the
bracket encloses a few instructions either side of the halt.

Also re-confirmed on this window, which definitely contains excursion frames:
`gx_paid`, `gx_blamed`, `dma_hold`, `cart_spin` and `gx_stall_events` are all
**exactly zero**, and `stall_partition_residual` is 0.

## 4. So the excursion is real work — and here is what it is

`armWaitForIrq` is **−323,450 ticks/frame** on the low-WAIT frames, matching the
`WAIT` drop. The idle genuinely left, and something genuinely replaced it:
**+286,619 ticks/frame** of extra execution, which the same per-frame regions
can now attribute by function. Top contributors, low-WAIT frames minus normal
frames, both normalised per frame:

| ticks/frame | group | functions |
|---|---|---|
| **~49,600** | softfloat / soft-arithmetic | `__aeabi_fadd` 22,203, `__aeabi_fmul` 19,200, `__udivmoddi4` 4,413, `__ieee754_sqrtf` 3,826 |
| **~44,300** | **the tick HUD measuring itself** | `ndsPlatformTickHudSort` 19,605, `_svfprintf_r` 8,400, `_vfiprintf_r` 5,887, `__ssvfiscanf_r` 4,682, `consolePrintChar` 4,350, `__utf8_mbtowc` 3,522, `__getreent` 2,256 |
| **~36,000** | cart read, relocation, bulk copy | `_ntrcardRecvByCpu` 10,212, `memcpy` 6,371, `memset` 5,305, `ndsRelocPointerRangeInLoadedFile` 4,355, `memmove` 2,670, `ntrcardRomRead` 2,427, `ndsRelocApplyWordByteSwap` 2,372, `ndsRelocAssetIDForToken` 2,261 |
| **~14,500** | geometry submission | `ndsRendererHardwareSubmitVertex` 6,379, `ndsRendererSubmitHardwareTriangle` 4,650, `ndsRendererScanList` 3,444 |
| **~5,700** | collision | `func_ovl2_800ED490` 3,307, `gmCollisionSetInvertMatrix` 2,420 |
| **~2,700** | animation | `battleship_ftAnimParseDObjFigatree` |

The rest is a long diffuse tail across ~59,000 program counters. **No single
cause dominates** — which is exactly why five previous tasks hunted this and
found nothing. It is not one mechanism; it is four unrelated ones landing on the
same frames.

Two of those deserve immediate comment.

**Cart reads on a "load-free" frame.** `_ntrcardRecvByCpu` and `ntrcardRomRead`
are cartridge traffic, and they are 12,639 ticks/frame *higher* on the excursion
frames, with relocation and bulk copy behind them. The frames Task 108 and
R2-00a called load-free are not load-free; the fallback census that classified
them counted a different thing. Task 75's preload work is aimed at the right
target after all — but at ~36,000 ticks/frame on 21% of frames, not at the
103,488 it estimated, and that estimate should still be re-derived.

**The instrument is 15% of the excursion it is reporting.** `WORK-H` subtracts
the `HUD` bucket, which is the console redraw *as bracketed*. But
`ndsPlatformTickHudSample()` — and the percentile sort inside it — runs *after*
the buckets are latched for the iteration, so its cost lands in the **next**
frame's `ALL`, where no bracket can remove it. `ndsPlatformTickHudSort` alone is
19,605 ticks/frame of the excursion. Counting only what is unambiguously the
instrument (the sort plus `consolePrintChar`) that is ~24,000 ticks/frame; the
libc formatted-I/O family above it is very likely the same HUD but its callers
were not traced, so it is quoted separately rather than folded in.

## 5. What this changes

Reversing R2-00a §6 point by point:

1. **`WORK-H` P95 is not inflated by a mis-scoped bracket.** The gate metric is
   sound. It *is* inflated by roughly 24,000–44,000 ticks/frame of deferred
   instrument cost on the tail frames, against a `WORK` P95 of 1,803,648 — about
   2%, not the 33% the phantom-work reading implied. The 1.12M gap is real.
2. **Task 75's ~103,488 preload estimate does not inherit a measurement
   artifact.** It should still be re-derived, because the excursion it was sized
   against is now known to be four causes, only one of which is loading.
3. **The board's highest-value row closes.** Optimization is not being steered
   by a metric that manufactures work.
4. R2-00a's other findings stand unchanged: no GX, DMA or cart *stall*, ledger
   closed, and the attributor reproduces the prior census bit-identically.

The excursion is now a real optimization target worth ~287,000 ticks/frame on
21% of frames, with a measured composition and no single lever.

## 6. The rule this produces

**Never compare two instruments across two binaries.** Placement differs, so a
frame index does not name the same workload, and the disagreement you measure is
the build, not the instrument. Put both instruments in one ROM and read them
from one run. Recorded in `TASK_STANDING_RULES.md`.

## 7. A second defect, found while attributing the excursion

`ndsRendererTask29GXRecord` came back as 24,240 ticks/frame. It is not in the
binary — `NDS_TASK29_GX_CENSUS` is 0 and the whole function is behind that
`#if`, and `nm` has no such symbol. **addr2line resolves through DWARF, which
still describes functions the linker garbage-collected**, so it will confidently
name a symbol that does not exist and charge real cycles to it.

`task65_subsystem_census.py` used `addr2line -f` for the function name, so this
is not a scratch-script problem. Fixed: the census now bisects each PC into the
ELF symbol table and lets that override addr2line wherever the two disagree.
The source path still comes from addr2line, because that is what the subsystem
classifier keys on and DWARF is right about the file even when it is wrong about
the symbol.

On the R2-00c window it **renames 18,987 of 59,366 PCs — 32%.** The per-symbol
table was that unreliable. The aggregates were not: REAL WORK comes out at
**1,446,638** ticks/frame against R2-00b's published 1,446,348, a 0.02%
difference, so R2-00b's headline numbers and its subsystem split stand. It is
the "which function" answer that was wrong, and that is the answer optimization
picks targets from.

### The frame, re-ranked on attribution that holds

From the same run, by cross-cutting kernel, against REAL WORK of 1,446,638:

| group | ticks/frame | % of work | cyc/insn | mem stall |
|---|---|---|---|---|
| soft-float | **177,857** | **12.3%** | 1.19 | 1,586 |
| matrix | **156,627** | **10.8%** | 2.35 | 54,686 |
| gx-submit | 144,852 | 10.0% | 2.72 | 54,627 |
| texture-resolve | 108,681 | 7.5% | 4.91 | 53,983 |
| `mem*` | 98,207 | 6.8% | 2.60 | 54,412 |
| rom-read | 10,562 | 0.7% | 2.52 | 2,502 |

Two readings that change what to do next:

- **Soft-float is the largest block in the frame and it is not stalled.** At
  1.19 cycles per instruction it is retiring almost as fast as the core can go,
  so there is nothing to win by making it faster — `__aeabi_fadd` is already
  hand-written ITCM assembly from Task 16. The only lever is calling it less,
  which means float→fixed at the call sites, in imported gameplay and animation
  code. Large, structural, and now measured at 177,857 ticks/frame.
- **Matrix construction is 156,627, not the 55,077 R2-02 E2 was sized at.** The
  bracket around `ndsRendererAdapterPrepareNativeStageMatrices` sees one call;
  the symbol census sees `ndsRendererMtxMul20p12` 29,663,
  `LoadHardwareMatrixPair` 20,176, `BuildDObjLocalMatrix` 18,596,
  `MtxMulAffine20p12` 16,784, `MtxLoadN64ToDS20p12` 13,793,
  `BuildDObjWorldMatrix` 12,880, `PrepareInitialMatrices` 12,233 and more, across
  stage *and* fighter. E2 should be scoped against that number.

## 8. Evidence

| SHA-256 (first 16) | file |
|---|---|
| `6253A659E79408E2` | window-total ROM (`build-waitaudit`) |
| — | `scratchpad/waitaudit/{arm9-profile.*,hud.json}` |
| — | `scratchpad/waitaudit-pf/{arm9-profile.*,hud.json}` (per-frame regions) |

Emulator: `emulators/melonds-attributor/melonDS.exe`, SHA-256
`D81FC0BF318756FD…` — the R2-00a stall-attributor build, installed repo-local
under its own directory rather than replacing `emulators/melonds/melonDS.exe`,
so every measurement taken with `DE80E46B…` stays comparable.
`check-melonds-policy.ps1` passes with it present.
