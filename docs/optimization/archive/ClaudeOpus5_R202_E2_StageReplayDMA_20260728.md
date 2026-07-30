# R2-02 E2 — the stage stops pushing its own command stream

**Date:** 2026-07-28
**Phase:** R2-02 (`Smash64DS_Runtime2_SwitchPlan.md` §7), second landed cut.
**Flag:** `NDS_R2_STAGE_DMA` (default `0`), measured with `NDS_R2_STAGE_DIRECT=1`
held on **both** arms so this isolates E2.
**Verdict: KEEP.** `STG` P50 **256,704 → 225,792**, down on **128 of 128
frames**, and the VBlank histogram gains **twelve 2-VBlank frames** where the
control had one.

---

## 1. What was changed

`ndsRendererTask36ReplayRun` ended in this:

```c
words = &owner->words[run->word_offset];
for (i = 0u; i < run->word_count; i++) { GFX_FIFO = words[i]; }
```

A flat push of a captured GX command stream out of a 32-byte-aligned buffer in
main RAM — about **4,200 words per frame** across the 21 runs, dragged through
the data cache one word at a time. Task 103 timed that shape at 9.51 ticks per
word, and the symbol census puts the whole function at 39,866 ticks/frame.

Replaced with **GXFIFO DMA** on channel 0 — the same idiom libnds `glCallList`
uses. The hardware reads main RAM directly, so the words stop being cache-line
fills, and the transfer self-throttles on FIFO space instead of stalling the
core on every store.

This is §3.3 measured the way §3.3 asks: **the win is ~16 KB/frame of cache
lines that stop being touched**, not instructions that vanished.

Two preconditions were already satisfied and are worth recording because they
are what made this a small change rather than a project:

- **Coherency.** The capture path already ends in
  `DC_FlushRange(owner->words, ...)`, and replay never writes the buffer. No new
  flush, and no per-frame flush.
- **Alignment.** `words[]` is already declared
  `__attribute__((aligned(32)))`.

Only channel 0 is polled before starting, not all four as `glCallList` does. A
DMA on another channel cannot interleave GX commands unless it is also in GXFIFO
mode, and nothing else here is; waiting on an unrelated audio transfer would be
a stall for nothing.

## 2. Measurement

One tree, two builds, `NDS_R2_STAGE_DIRECT=1` on both, `NDS_R2_STAGE_DMA` off
versus on. 128-frame ring dump, frames 439–566, melonDS-Accurate `DE80E46B…`,
git `9c6d505213`.

| bucket | off P50 | on P50 | ΔP50 | off P95 | on P95 | ΔP95 |
|---|---|---|---|---|---|---|
| **`STG`** | **256,704** | **225,792** | **−30,912** | **264,384** | **233,472** | **−30,912** |
| `WORK` | 1,239,296 | 1,207,872 | −31,424 | 1,669,440 | 1,669,696 | +256 |
| `WORK-H` | 1,228,608 | 1,201,536 | −27,072 | 1,643,328 | 1,603,712 | −39,616 |
| `WAIT` | 441,792 | 467,904 | +26,112 | 517,568 | 532,864 | +15,296 |
| `FTR` | 545,472 | 545,152 | −320 | 994,560 | 995,264 | +704 |
| `SRC` | 325,888 | 325,696 | −192 | 683,584 | 663,936 | −19,648 |
| `ALL` | 1,680,000 | 1,680,000 | 0 | 1,680,768 | 1,680,448 | −320 |

**Paired per-frame, matched by ring slot:**

| bucket | frames down | frames up | median Δ |
|---|---|---|---|
| **`STG`** | **128** | **0** | **−30,720** |
| `WORK` | 109 | 19 | −31,040 |
| `WAIT` | 27 | 101 | +30,208 |
| `FTR` | 63 | 62 | 0 |
| `SRC` | 62 | 63 | 0 |
| `BG` / `AUD` / `HUD` | — | — | 0 |

`STG` down on every frame at 4–6× the placement floor is a mechanism. `FTR`,
`SRC`, `BG`, `AUD` and `HUD` all have a median delta of exactly zero, which is
what a correctly isolated change looks like.

**VBlank interval histogram** — the metric §4 says the switch exists to move:

| | 2 | 3 | 4 | 5+ | max | slips |
|---|---|---|---|---|---|---|
| off | **1** | 548 | 12 | 5 | 18 | 0 |
| on | **12** | 540 | 9 | 5 | 18 | 0 |

Twelve 2-VBlank frames against one. That is the first time this campaign has put
a measurable population of frames on the target side of the histogram. `ALL`'s
worst frame also falls from **5,874,368 to 2,800,512**, and its mean drops below
three VBlanks (1,743,392 → 1,675,610).

Realized 30,912 against the replay loop's 39,866 symbol cost — **78%**. The
remainder is the DMA setup and the completion poll, paid 21 times a frame.

## 3. Correctness

- **Boundary green** on the candidate configuration (mode 163, one-minute), ROM
  built `NDS_R2_STAGE_DIRECT=1 NDS_R2_STAGE_DMA=1`.
- Required-region detail **62.542%** vs the default's 62.778% — 17 pixels in
  7200, and the two captures land on different game frames because the candidate
  runs faster.
- Screenshot `artifacts/visibility/r2-02-e2-dma-on-boundary-20260728.png`: tree,
  trunks, bushes, flowers, fence, pond, both platforms and sky all intact. **No
  tearing, no dropped or duplicated geometry** — the specific failure mode a
  bad DMA/GX interaction produces.
- 0 cadence violations, `vbiTotal` 566 on both arms.
- Engagement proven statically: the shipped `ndsRendererTask36ReplayRun` builds
  `0x04000000` with an immediate, polls `[r1,#0xb8]`, then stores source,
  `0x04000400` and `DMA_FIFO | word_count` to `0xb0`/`0xb4`/`0xb8` and polls
  again. The CPU store loop is gone.

## 4. Where R2-02 stands against its gate

```text
STG P50   351,488  baseline
          256,704  after E1a  (-94,784)
          225,792  after E2   (-30,912)
          180,000  §7 gate
          -------
           45,792  still over
```

**R2-02 is not closed.** §3.1 and §7 forbid starting R2-03 until it is. The
remaining stage symbols, from the corrected census:
`ndsRendererCommitNativeStageSegment` 27,082, `ndsRendererNativeStageBeginRun`
13,740, `ndsRendererNativeStageLoadNoZMatrix` 11,638,
`ndsRendererNativeStageApplyStateSpan` 11,442,
`ndsRendererAdapterStageWorldSourceKeyMatches` 7,735.

**E3, the shape §7 actually asks for.** `BeginRun` runs 21 times a frame at ~654
ticks a call to set GX state that is already baked in `run->prepared`, and each
run then needs its own DMA. If the capture recorded the state writes too, the
whole static stage becomes **one contiguous stream and one DMA per frame** —
which is what "the runtime shape is `DreamLand_Run17()`, not
discover/validate/rebuild/resolve/prepare/submit" means. That collapses
`BeginRun`, 20 of the 21 DMA setups, and most of `CommitNativeStageSegment`.
Sized against those symbols it is worth roughly 40,000, which would close the
gate. It needs generator work on the capture side and is the next cut.

## 5. Evidence

| SHA-256 (first 16) | file |
|---|---|
| `3A52FF12500C5360` | `artifacts/performance/r2-02-e2-off-438.json` |
| `C3E31CCF9076B558` | `artifacts/performance/r2-02-e2-on-438.json` |
| — | `artifacts/visibility/r2-02-e2-dma-on-boundary-20260728.png` |
