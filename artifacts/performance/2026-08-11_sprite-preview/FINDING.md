# Sprite-preview pair — measured before building. It is NOT a per-frame cost.

Task: eliminate/DS-native-ize the 307,200 B `sOriginalSpritePreview` +
`sOriginalSpriteDisplayPreview` pair and its per-frame render/copy cost,
measured against the true top-80 P95 frames.

## 1. It is not a double buffer, and the name is the reason it looks like one

On hwtri (the shipped configuration) the two surfaces have **different jobs**:

| symbol | bytes | role on hwtri |
|---|---:|---|
| `sOriginalSpritePreview` | 153,600 | 320x240 **N64-space staging** surface |
| `sOriginalSpriteDisplayPreview` | 153,600 | **sprite decode cache** (`ndsPlatformGetOriginalSpriteDecodeCache`) |

The "display" surface is only a display buffer on `!NDS_RENDERER_HW_TRIANGLES`:
its writers (`nds_platform.c:783,789`) are in the `#else` arm and its reader
(`:2002`) is in `#if !NDS_RENDERER_HW_TRIANGLES`. On hwtri it is returned by
`ndsPlatformGetOriginalSpriteDecodeCache` instead. This is deliberate, and the
code says so at `nds_platform.c:762-766`: *"Downscaling in place is safe ... it
leaves the retained display buffer free for immutable decoded sprite data
without adding a second pixel buffer."*

**So neither buffer is redundant and neither can simply be deleted.**

## 2. There is no per-frame rendering/copy cost

Whole-match engagement counters, 1600 frames, gate arm:

| counter | value |
|---|---:|
| `gNdsSObjForegroundStagingClearBytes` | 153,600 = **one** clear |
| `gNdsSObjBackgroundStagingClearBytes` | **0** |
| `gNdsOriginalSpritePreviewCommitCount` | **2** |
| `gNdsSObjWallpaperFinalDirectCount` | 1 |

The staging path runs **about twice in a one-minute match**, not once a frame.

## 3. Against the true top-80 P95 frames: it is ONE frame

`--split-by-symbol ndsPlatformCommitOriginalSpritePreviewLayer`:

```
1 marked frame vs 1599 control
  marked   6,723,668 cycles/frame
  control  2,490,859 cycles/frame
  premium  4,232,809/frame  = 2,113,442 ticks
  marked frames (region ids): 1558
```

Region 1558 is the battle→Results transition. That single frame's premium is
memcpy 29.5%, `ndsDrawSObjIntoPreview` 25.2%, the commit 23.0%, memset 16.3%.

**Consequence: optimising this CANNOT move P95.** P95 is the 80th most expensive
frame of 1600; this is the single most expensive one. Deleting its cost entirely
moves P100/P99.9 and leaves the 80th frame untouched. The 25,531 cyc/frame that
made this look like a 1.9% tail owner is one frame's 4.2M cycles divided across
the 80 marked frames — the same arithmetic that killed the animation attach
slice. See [[cluster-where-the-percentile-lives]].

## 4. What it IS worth fixing for

A **6-VBlank frame** (6,723,668 cycles = 3,357 kticks, ~2.7x a 2-VBlank frame)
at the battle→Results transition is a visible hitch, and `PROJECT_GOAL.md`
requires exactly this: *"GAME SET, the Results screen, and Sudden Death must
hold their presented cadence without perceptible hitching."* That is a real,
named requirement — it is simply not the P95 gate, and the work should be
proposed and judged against cadence rather than against P95.

## 5. RAM, honestly

- **Neither buffer can be deleted** (both live, different roles).
- **Staging could shrink 320x240 -> 256x192** (−55,296 B) only if drawing at DS
  resolution directly is acceptable; today it draws at 320x240 and downsamples,
  which is effectively supersampling, so this is a fidelity trade needing owner
  sign-off, not a free win.
- The decode cache genuinely uses 320x240 plus map scratch beyond it.

Maximum honest RAM recovery here is **~55 KB with a visual trade**, against
169,152 B already recovered — and zero P95 movement.
