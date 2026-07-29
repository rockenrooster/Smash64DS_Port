# R2-03 E30 — decomposing the P95 tail, and the instrument in it

**Date:** 2026-07-29
**Status:** the median is inside the gate; the tail is a separate problem with
three named causes, one of which was the measuring instrument.

## Why this exists

E28 and E29 took 58,304 ticks/frame out of the fighter draw. `WORK` P50 fell
from 1,071,488 to 1,010,240 — **inside the 1,120,000 gate**. `WORK` P95 moved
from 1,496,064 to 1,467,840, essentially not at all.

That is the whole finding: **the steady-state fighter cost and the P95 gate are
now different problems, and more median cuts will not close the gate.** This
document names what does own it, on the current Runtime 2 build. The board's
previous tail analysis (Tasks 75/106/107) predates the R2 branch and attributed
the tail to `SRC` alone.

## Method

Decompose the *actual worst frames*, not independently-sorted columns. Each
column's P95 is a different frame, so a per-bucket percentile table cannot say
what any one expensive frame was doing. Take the 8 highest-`WORK` frames and
difference every bucket against the median-`WORK` frame.

## Result: three independent causes

Excess over the median frame, summed across the 8 worst of 128 frames:

| bucket | excess | share of WORK excess |
|---|---:|---:|
| **FTR** | 2,538,432 | **41.9%** |
| **HUD** | 1,868,608 | **30.9%** |
| **SRC** | 1,298,624 | **21.4%** |
| OTHR | 680,832 | 11.2% |
| MISC | 331,264 | 5.5% |
| STG | 10,752 | 0.2% |

They do not co-occur. Of the 8 worst frames, 3 are HUD-only, 3 are FTR-only, and
2 are both. Three mechanisms, not one.

### 1. HUD — 345,024 ticks, and it was the instrument (resolved)

`HUD` is 960 at the median and **345,024** on 9 of 128 frames, with a second
cluster at ~74,500 on 6 more. The 345,024 frames are periodic: 453, 468, 480,
494, 507, 520, 534, 546, 559 — a mean gap of 13.25 presented frames, which at
2.235 VBlanks per present is **0.494 s**. `NDS_BATTLE_FPS_HUD_SAMPLE_TICKS` is
`BUS_CLOCK / 2`.

`gNdsRendererProfileHudTicks` brackets `ndsPlatformRenderDebugHud`, and inside it
the `NDS_BATTLE_TICK_HUD_ENABLED` block re-sorts eleven 128-entry rings and
pushes thirteen `vsnprintf`/`iprintf` lines through the libnds text console.
**None of that exists in the published ROM**, and the GDB sampler reads
`sBattleTickHudRing` directly and never touches `sBattleTickHudP50/P95`. For a
scripted measurement the entire block is instrument cost with no reader.

`NDS_TICK_HUD_DRAW=0` removes it. Measured against the same build with it on:
mean `HUD` −22,547/frame, `WORK` P95 1,548,032 -> **1,467,840**, VBlank histogram
`2:446 3:109 4:9` -> **`2:472 3:87 4:4`**, and frames over the gate 39/128 ->
35/128. `FTR`/`WORK` medians moved +3,328/+3,648, below the 5,000–7,000 build
placement floor and in the direction placement noise takes when code is removed.

**Every performance measurement this campaign has taken on the tick-HUD ROM has
carried ~345,024 ticks of instrument cost on ~7% of frames — landing on exactly
the frames the P95 gate is decided on.** The default stays 1 so a device read or
screenshot still shows the HUD; measurement runs should pass 0.

One trap paid for: with nothing in the ROM reading the ring, dead-store
elimination plus `--gc-sections` deleted `sBattleTickHudRing` outright and the
sampler failed with "Attempt to take address of value not located in memory". It
is now `volatile`. **A measurement buffer whose only consumer is a debugger has
to say so in the type.**

**The ~74,500 residue is not the instrument and does not go away.**
`NDS_BATTLE_FPS_HUD_ENABLED` depends only on `HARNESS_FAST_LOGIC`,
`HW_TRIANGLES`, `LIVE_INPUT_PREVIEW` and `DEBUG_HUD` — not on `NDS_TICK_HUD` —
so the published block satisfies it too and the published ROM renders
`ndsPlatformRenderBattleFpsHud` and `ndsPlatformRenderBattleTextHud`. Those are
the ~74,500 frames, and 345,024 − 74,500 ≈ 270,500 is the tick-HUD block's own
share (9 × 270,500 = 2.43M against the 2.89M total that `NDS_TICK_HUD_DRAW=0`
actually removed, which is the right order).

So the published ROM pays roughly **74,500 ticks of `iprintf` HUD text on a
minority of frames**, through the same `vsnprintf` + libnds console path. That is
a real, separate, and probably cheap target — a fixed-width digit blitter instead
of `printf` — but it is a fraction of the tail and should be queued behind the
FTR bursts.

### 2. FTR — two 5-frame bursts where the fighter draw doubles (open)

`FTR` is 401,856 at the median and **894,528–911,872** on 10 of 128 frames. The
distribution is bimodal with nothing in between, and the frames are not
scattered:

```
478 479 480 481 482        544 545 546 547 548
```

**Two contiguous bursts of exactly five presented frames, 62 frames apart.** That
is not placement noise and not a per-frame cost — it is a discrete event, twice
in the window, each lasting five frames, during which the fighter draw costs
2.25x its median.

**Measured, and it is a fallback.** Two probes, both on the burst windows against
a control window of the same length:

*Geometry does not spike.* `gNdsFighterDLAllDrawP0HardwareTriangleCount` per
frame: 468–473 **256.0**, 473–478 **384.0**, 478–483 (burst) **320.0**, 483–488
**320.0**. The window immediately *before* the burst draws more triangles per
frame than the burst does. Triangle count and `FTR` ticks are uncorrelated.

*The native execute gets **cheaper**.* Phase census, 5-frame windows:

| phase, ticks/frame | 468 control | 478 burst | 544 burst |
|---|---:|---:|---:|
| Preflight | 4,134 | 2,534 | 2,534 |
| Root | 55,091 | 33,510 | 33,498 |
| State replay | 78,566 | 50,970 | 51,059 |
| Shade | 27,840 | 17,882 | 18,086 |
| Submit | 121,357 | 73,882 | 73,702 |
| **sum** | **286,988** | **178,778** | **178,879** |
| epoch calls | 58.8 | **38.2** | **38.2** |
| submit calls | 80.4 | **49.0** | **49.0** |

The native owner runs **38.2 epochs instead of 58.8** and spends **178,800
instead of 286,988** ticks — while `FTR` for the whole frame *doubles*. The two
bursts are counter-identical (191 epochs, 245 submits, 108 material
invalidations over 5 frames): the same event, twice.

**So the work left the native path.** Fewer epochs are drawn natively and the
cost reappears outside every bracket the census owns — which is the generic
DObj/display-list interpreter. This is a **native-owner fallback**, not a slow
native path, and no amount of optimising the native execute will touch it.

**Next step is exact and cheap:** build with `NDS_TASK68_FALLBACK_CENSUS=1` and
sample with `scripts/sample-tick-hud-buckets.ps1 -FallbackCensus`, which rings
`gNdsTickHudNativeOwnerFallbackCount` and
`gNdsTickHudNativeOwnerFallbackByReason[]` per frame. The reason code names the
eligibility test that fails on frames 478–482, and that test is the fix site.
(The counters live under that flag only, which is why a plain tick-HUD build
reports "No symbol gNdsTickHudNativeOwnerFallbackCount".)

Candidates for what the reason will turn out to be, in the order they should be
checked:

- A fighter falling off the native owner path into the generic one for the
  duration of a state. `gNdsRendererProfileSourceVertexLoadCount` and the
  native-owner fallback counters already exist and are per-frame ringable.
- Task 39's hurt flash: it writes `input->materials[]` live, and a material
  application invalidates the texture prepare (28.0/frame at the median). A
  damage state lasting five frames fits both the duration and the 62-frame gap
  between two hits.
- A third owner being drawn (an effect with its own root set).

The instrument to use is the existing per-frame ring plus the phase census
windowed on frames 478–482 — the same method E15 used, restricted to the burst.
Do **not** start by optimising the fighter draw further; the median path is not
what these frames are running.

### 3. SRC — asset loading (known, previously sized)

522,944–802,624 against a 326,784 median on 9 frames. This is the population
Task 75 E0 characterised: all load frames are `SRC` excursions, but 2 of 7
excursions carry no cartridge activity, and eliminating on-demand loading was
sized at ~103,488. That analysis stands and is unchanged by Runtime 2.

## Where the gate stands

| | P50 | P95 | gate |
|---|---:|---:|---:|
| WORK, instrument off | **1,010,240** | 1,467,840 | 1,120,000 |

**P50 passes. P95 is 1.31x, a gap of 347,840** — against 1.37x at the R2-08
readiness measurement. Of that gap the tail decomposition attributes ~42% to the
FTR bursts and ~21% to `SRC`.

## The rule this generalises to

When a subsystem's median falls and its P95 does not, stop cutting the median.
The two are different code paths, and the gate is the tail. Decompose the worst
frames against the median frame *by bucket* before proposing anything — it costs
one script over data you already have, and it is what separated three independent
causes here from the single "renderer fast-path dropout" the board had recorded.
