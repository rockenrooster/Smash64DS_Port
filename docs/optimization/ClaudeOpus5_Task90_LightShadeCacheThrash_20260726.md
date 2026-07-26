# Task 90 — The light-shade LUT cache held 4 entries against a working set of 6

**Date:** 2026-07-26
**Status:** KEEP. Boundary green. `FTR` P50 −19,584, P95 −16,512; `WORK-H`
P50 −17,920, P95 −15,040; improved on **128 of 128 frames**.
**Inputs:** `artifacts/task90-A.json` (4 entries), `artifacts/task90-B.json`
(8 entries), `artifacts/task90-shade-census.json`,
`artifacts/task90-shade-census-cache8.json`.

## 1. Why this ran, and what it was originally looking for

The session's strategic read was that the remaining levers are design-level and
that `FTR` — 57.5% of the `WORK-H` P95 at spread 1.78, against a stage that went
native and is now flat at spread 1.02 — is where the gap lives. This task was
scoped as the E0 that sizes the fighter-native opportunity before committing to
it, on the Task 78 principle: a plausible plan item gets one cheap census before
it gets a build.

It did not find what it went looking for, and it found something better on the
way. Both results are below; the negative one first, because it closes a
direction.

## 2. The re-shade hypothesis is dead: 0.0% redundancy

`ndsRendererNativeShadeProductionActions` is the largest non-libc, non-idle
symbol in the frame — **50,509 ticks/frame**, ahead of texture resolve. It walks
epochs and shades dense vertices into `sNdsNativeFighterPreparedDense`, which is
a **persistent** array. That framing suggests an obvious memo: any iteration
whose shading inputs are unchanged since that dense id was last written is
recomputing a value already in memory.

Measured over the Boundary window with a deliberately over-wide key (LUT
identity, material colour, modulate, both light colours, geometry mode, and the
prepared light direction — so it can only *under*-report redundancy):

```
ShadeDenseIter    541.0 / frame
ShadeComputed     519.0 / frame
ShadeAliased       22.0 / frame   (4.1%)
ShadeRedundant      0.0 / frame   (0.0%)
ShadeKeyChanges    46.5 / frame   out of 49.0 calls
```

**Zero.** 541 dense vertices, each shaded exactly once per fighter per frame,
with the key changing on 46.5 of 49 calls. There is no re-shade to eliminate.
This is the third memoisation direction the campaign has opened and closed
(after Tasks 79 and 81), and it closed for the cheapest reason yet: the work was
never duplicated.

**Do not re-open dense-vertex memoisation.** The array is persistent, which
makes the idea look free, but the inputs genuinely change every call.

## 3. What the same run found instead

The probe also counted the light-shade LUT cache, because
`ndsRendererHardwareGetLightShadeLut` is **17,556 ticks/frame** — a suspicious
figure for something whose job is a lookup.

```
LutGetCalls        49.0 / frame
LutBuilds           6.0 / frame   (12.2% miss)
```

A miss is not a lookup. It rebuilds `entry->rgb[128]`, three channels per step,
and 6 of those per frame is ~15,000 ticks — essentially the whole symbol.

## 4. A hit rate cannot size a cache; the request trace can

12.2% misses is equally consistent with a working set that barely overflows and
one far too large to ever hold, and those want opposite decisions. So the probe
recorded the **actual request sequence** — the last 128 `(diffuse, ambient)`
pairs in order — and the host replays it against a FIFO of each candidate size:

```
LUT request trace: 128 requests, 6 distinct (diffuse, ambient) pairs

cache size   misses   miss rate
         4       17       13.3%
         8        6        4.7%
        16        6        4.7%
        32        6        4.7%
compulsory        6     (floor)
```

The working set is **6**. The cache held **4**. Every frame it evicted a live
pair and rebuilt its table. At 8 entries the replay reaches the compulsory floor
— one build per distinct pair, for the whole match — and 16 and 32 buy nothing
over 8. That is the entire finding, and it is why the constant is 8 and not a
rounder, more comfortable 16.

Confirmed in the ROM before any A/B: rebuilding the census arm at 8 entries took
`LutBuilds` from **6.0/frame to 0.0/frame**, exactly as the replay predicted.

## 5. Result

One constant, `NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT` 4u → 8u, plus its
`_Static_assert` (2,096 → 4,192 bytes of BSS).

| | A (4 entries) | B (8 entries) | Δ |
|---|---|---|---|
| `FTR` P50 | 563,136 | 543,552 | **−19,584** |
| `FTR` P95 | 998,464 | 981,952 | **−16,512** |
| `WORK-H` P50 | 1,338,688 | 1,320,768 | **−17,920** |
| `WORK-H` P95 | 1,741,952 | 1,726,912 | **−15,040** |
| VBlank 3-interval | 497 | 510 | +13 |
| VBlank 4-interval | 64 | 51 | −13 |

Per-frame, over the same 128-frame window: **128 improved, 0 unchanged, 0
worse**; median −19,712, best −26,496, worst −13,376. A delta that holds its
sign on every frame is a mechanism, not placement luck — and the mechanism was
predicted from the trace before the A/B ran, which is the stronger claim.

`STG`, `SRC`, `MISC` and `BG` are unmoved, as expected: the cache is only
consulted from the fighter production path.

## 6. The checker edit, stated plainly

`scripts/check-gbi-decode-fixtures.ps1:1762-1764` pinned the cache at
`4u`/`2096u`, and Boundary failed on the first attempt.

That pin is a **RAM-budget ratchet**, not a correctness bound — its own message
called 2,096 bytes "measured". It was raised to `8u`/`4192u` with the
measurement that justifies it, and the assertion messages now name
`scripts/census-light-shade-lut.ps1` so the next person can re-derive the number
instead of treating it as magic.

Nothing about exactness moved. `NDS_RENDERER_HW_LIGHT_SHADE_LUT_COUNT` is still
128. The assertions that the cache is content-keyed by both source light colours
(:1765), that the LUT preserves the exact per-channel integer result and source
alpha (:1766), and that incomplete light state still takes the generic fallback
(:1767) are untouched and still pass.

This is deliberately not the Task 82 E1 situation, where a placement change
wanted to edit a *set-membership* contract to make itself pass and was reverted
instead. The distinction worth keeping: raising a documented budget with a
better measurement of the same quantity is maintenance; editing a correctness
assertion to accommodate a change is not.

## 7. Standing rule this earns

**A cache's hit rate does not tell you its size — replay the request trace.**
Four entries at 87.8% hits looked healthy enough that nothing had questioned it
in eighty-nine tasks. The trace showed the working set was 6, the fix was one
digit, and the confirmation (`LutBuilds` 6 → 0) was available before spending an
A/B. Any fixed-size cache in this codebase deserves the same one-run treatment
before anyone assumes it is fine.

Added to `docs/optimization/TASK_STANDING_RULES.md`.

## 8. What this leaves for the fighter-native question

Task 90 did not answer it. The E0 that was scoped — what fraction of `FTR`'s
P95 is generic translation replaceable by generated tables — is still open, and
`FTR` P95 is now 981,952, still 88% of the entire 1,120,000 budget on its own.

Two things this run did establish about that path:

- The fighter is **already partly native**. `sNdsNativeFighterRuns`, epochs,
  state deltas and prepared dense vertices all exist and execute; 49 epochs run
  per frame. Profile 0 is not a greenfield.
- The per-call fixed cost is amortised over only **11 dense vertices** (541
  across 49 calls). Whatever the fighter path spends per *call* is being paid 49
  times a frame against very short runs. That ratio, not the per-vertex work, is
  where the next census should look.

Gap to target after this task: **606,912** (from 622,080).
