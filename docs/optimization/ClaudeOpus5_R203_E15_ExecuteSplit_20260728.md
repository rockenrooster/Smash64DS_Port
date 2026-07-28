# R2-03 E15 — the fighter costs per epoch, not per triangle, and E14's steer was wrong

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** Measurement. The execute partitions completely. **The emit is 20% of
it and ~99 ticks per triangle** — so the captured-command-stream lever E14
recommended addresses at most a fifth of the cost, and that recommendation is
withdrawn here.

## 1. The split

`ndsRendererExecuteNativeFighterOwnerProduction`, bracketed into every phase it
contains. 479 frames, both fighters, per presented frame:

| phase | ticks/frame | share of execute |
|---|---:|---:|
| Preflight | 3,247 | 1.0% |
| Per-root: bind, composed matrix, `glStoreMatrix`, light preamble | 40,785 | 13.1% |
| Per-epoch: two state spans + material | 52,065 | 16.8% |
| **Per-epoch: shade actions** | **86,207** | **27.7%** |
| Run prepare | 42,520 | 13.7% |
| Raw emit | 56,873 | 18.3% |
| Cross emit | 5,820 | 1.9% |
| residual (loop, tail span, end batch) | 18,487 | 5.9% |
| **execute total (instrumented)** | **310,782** | |

Rates: **48.5 epochs and 66.2 runs per frame** across both fighters, 574.9 raw
triangles and 43.1 cross triangles.

**Instrument overhead is real and is stated rather than hidden.** The
uninstrumented execute is 279,617 (E14), so these brackets cost ~31,165
ticks/frame — about 460 `cpuGetTiming()` reads at ~68 ticks each, landing mostly
on the per-run and per-epoch counters. Absolute values are therefore inflated by
roughly 15-20% on `Submit*` and 10% on the epoch phases. The **ranking** is
unaffected, and the ranking is the finding.

## 2. Two corrections to E14

**"447 ticks per hardware triangle" was the wrong statistic.** It divided an
inclusive bracket by a triangle count when most of that bracket is not
per-triangle work at all. The emit is **56,873 + 5,820 over 618 triangles ≈ 99
ticks per triangle**, which is an ordinary number for a textured DS submission
that writes colour, texcoord and vertex words per corner.

This is the census-row-versus-bracket trap the campaign has hit before, in the
opposite direction: E12 found a function whose cost was *outside* itself; here a
bracket's cost was attributed to the wrong denominator inside it.

**The captured-command-stream recommendation is withdrawn.** R2-02 E2's GXFIFO
DMA won for the stage because the stage's per-frame cost genuinely was a flat
push of ~4,200 words. The fighter's push is 20.2% of its execute. Even a perfect
DMA of the entire emit — which is not achievable, since the emit interleaves
per-corner state the stage's replay does not have — caps out around 62,693
ticks/frame against a 250,833 gap.

E14's underlying finding stands unchanged and is what still matters: **the
geometry engine is idle and the ARM9 is the whole cost.** What E15 corrects is
*which* ARM9 work.

## 3. What the fighter actually is

**A per-epoch machine.** 48.5 epochs a frame, each paying two state spans, a
material application, and a shade pass, for an average of **12.7 triangles**.

Per epoch: 1,073 ticks of state + 1,777 ticks of shade = **2,850 ticks before a
single triangle is emitted**, against 1,255 ticks of prepare-and-emit for the
~1.4 runs that epoch contains.

**Roughly 70% of the fighter's execute is per-epoch and per-root setup; 20% is
the geometry.**

## 4. Where the leverage is, in order

1. **Shade actions, 86,207.** The largest single item. R2-03 E1 refuted memoising
   this *across frames* (1,796 of 1,835 frames changed), and that result stands.
   But the question E1 answered is not the only one available: E1 asked whether
   the shade output is stable frame to frame. It never asked whether the shade is
   recomputing **per epoch** something that is constant **per root**, which is a
   within-frame redundancy a cross-frame memo cannot see. 48.5 epochs against
   ~28 roots is the shape that makes that worth asking.
2. **Epoch state spans, 52,065.** R2-02 F ran exactly this analysis for the stage
   and found adjacent-run redundancy; the fighter's spans have never been checked
   for it. 1,073 ticks per epoch to re-apply recorded deltas is a lot when
   consecutive epochs share a material.
3. **Per-root 40,785** over ~28 roots — 1,457 ticks each, and it contains the GX
   matrix load and `glStoreMatrix`. The matrix palette is 31 slots and the
   fighter has ~14 roots per side; whether every root needs its own store is
   unasked.
4. **Run prepare 42,520**, already cut once by E12's texture memo. Diminishing.
5. **Emit 62,693.** Real, ordinary, and the least promising per unit of risk.

## 5. Method note

The first cut of this experiment instrumented
`ndsRendererNativeSubmitRunDirect`, which is the non-production submit path.
Canonical mode 9 routes through `ndsRendererNativeSubmitProductionRun` instead,
so all nine counters read a perfectly plausible **zero** while
`gNdsTask91ExecuteTicks` read 133M.

That is the E5 failure exactly, and it was caught in one run because the rule
written earlier the same day — a liveness counter read before the result — was
followed. `gNdsR2SubmitCalls` is that counter here and reads 31,733.

## 6. Next

**R2-03 E16: ask whether the shade pass is per-epoch work that is actually
per-root.** 86,207 ticks/frame, and E1's refutation does not cover the question.
