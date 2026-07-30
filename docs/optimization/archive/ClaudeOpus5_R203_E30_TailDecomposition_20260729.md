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

**So the work left the native execute** — fewer epochs are drawn there and the
cost reappears outside every bracket the phase census owns. `FTR` also brackets
the DObj walk, the revalidation and the owner prep (E2/E3: 113,199/frame; E4:
MatrixPrep 91,338/frame), so the excess is in one of those or in a fallback to
the generic interpreter. **Optimising the native execute cannot touch these
frames either way.**

### ANSWERED: it is the hitlag shuffle disabling the native owner

`NDS_TASK68_FALLBACK_CENSUS=1` over frames **460..500** (40 frames, containing the
478–482 burst):

| counter | value |
|---|---:|
| `gNdsTickHudNativeOwnerFallbackCount` | **5** |
| reason [2] `AnimLock` | **5** |
| every other reason | 0 |
| `Calls` / `Eligible` (denominators) | 82 / 82 |

**Five fallbacks, all `AnimLock`, one per burst frame.** The site is
`reloc_backend_renderer_dl.c:12224`:

```c
if (native_owner_enabled && (production_mode || hierarchy_mode) &&
    ((fp->is_use_animlocks != FALSE) || (fp->shuffle_tics != 0u)))
{
    native_owner_enabled = FALSE;      /* whole fighter -> generic path */
}
```

`shuffle_tics` is SSB64's hitlag shuffle — `fttypes.h:1146` calls it the "Model
shift timer", set in `ftparam.c:236` from `ftParamGetHitLag(...)` when the
fighter is hit. Two hits about two seconds apart, each with roughly five
presented frames of hitlag, is exactly the observed signature.

**And the source says it is trivially absorbable.** `ftdisplaymain.c:1205`:

```c
if (fp->shuffle_tics != 0) {
    syMatrixAdvanceW(m, gSYTaskmanGraphicsHeap);
    syMatrixTra(m, shuffle.x, shuffle.y, 0.0F);
    gSPMatrix(..., G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
}
ftDisplayMainDrawAll(fighter_gobj);
if (fp->shuffle_tics != 0) { gSPPopMatrix(..., G_MTX_MODELVIEW); }
```

**One PUSH, one whole-fighter translate by `(x, y, 0)` out of the small constant
table `dFTDisplayMainShufflePositions[is_shuffle_electric][shuffle_frame_index]`,
one POP.** It is not a per-joint effect and it does not change geometry,
materials or animation — it shifts the entire model. `lbcommon.c:1627` does the
same thing for the attached-DObj path by adding the offset into `f[3][0]` and
`f[3][1]` of the local matrix.

The native owner already loads a per-root matrix
(`ndsRendererLoadHardwareSplitMatrices`, E17). **Folding the shuffle offset into
that load — or pushing one translate before the owner's roots and popping after —
reproduces the source exactly at essentially zero per-frame cost**, instead of
disabling the native path for the whole fighter for the duration of every hit.

Expected: removes ~500,000 excess ticks on the ~10 burst frames per 128, i.e.
41.9% of the tail excess, and it is mechanically equivalent by construction
rather than by approximation.

**Caveat to check before implementing:** `is_use_animlocks` is the other half of
the disjunction and is *not* covered by this. The census says all five fallbacks
in this window are the same reason code, which is shared by both conditions, so
confirm which of the two fired — split the counter or read `fp->shuffle_tics`
directly on a burst frame — before assuming the shuffle is the whole story.

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

## Harness defect fixed: the census window collapsed silently, twice

`census-fighter-draw-phases.ps1` requested 5 frames from 478 and sampled
483–484; it requested 30 frames from 470 and sampled 500–501. Both printed a
complete, plausible counter table. The second one produced a **"no fallback
occurred"** reading from a window that did not contain the frames under
investigation, and that reading was briefly written down as a refutation.

GDB `if` at top level resumes exactly once — already a standing rule from
Task 96 — so when a stop is missed the script's own `continue` lands on a later
frame and nothing notices. The script now throws unless the A stop is exactly
`StartFrame` and the achieved window is `WindowFrames` or one more. **A
measurement that quietly answers a different question is worse than one that
fails.** Prefer 30+ frame windows; small ones miss stops most often.

## The rule this generalises to

When a subsystem's median falls and its P95 does not, stop cutting the median.
The two are different code paths, and the gate is the tail. Decompose the worst
frames against the median frame *by bucket* before proposing anything — it costs
one script over data you already have, and it is what separated three independent
causes here from the single "renderer fast-path dropout" the board had recorded.
