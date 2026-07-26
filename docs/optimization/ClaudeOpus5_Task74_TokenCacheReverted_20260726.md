# Task 74 — Token memoization reverted; the noise floor is above the target

**Date:** 2026-07-26
**Status:** REVERT. Code restored byte-identical; only an explanatory comment kept.
**Inputs:** `artifacts/task74-tokencache.json` against
`artifacts/task72-oneopen.json` — same instrument, same 128-frame window.

## 1. The change, and why it was safe

`ndsRelocAssetIDForToken` is about a hundred sequential comparisons followed by
two linear scans of the Mario and Fox animation tables, run on every relocation.
Task 71 priced it at 9,306 ticks/frame on a frame that loads an animation.

Memoizing it is provably correct, which is worth recording: `ndsRelocFileID`
returns `(u32)(uintptr_t)file_id` — the *address* of the file-id global, not a
value read from it. Every token in the chain is therefore a link-time constant
and the function is pure in its argument. A direct-mapped cache could never go
stale. The usual objection to memoizing a lookup like this does not apply.

## 2. It measured worse

| bucket | Δ P50 | Δ P95 |
|---|---|---|
| `SRC` (the target) | **+1,472** | **+1,920** |
| `WORK-H` | **+11,584** | −6,528 |
| `STG` (cannot be affected) | +8,128 | +7,744 |
| `FTR` (cannot be affected) | +3,968 | +4,608 |

The targeted bucket got worse at both percentiles. `WORK-H` P50 regressed by
11,584. And `STG` — which a token lookup cannot touch by any mechanism — moved
8,128, which is larger than the entire effect being chased.

## 3. Why, and the rule it earns

Two things, both worth keeping:

**The mechanism was probably never a win.** A hundred compares against link-time
immediates are branch-predictable and already resident; three lookup arrays in
`.main.bss` are three chances at a data-cache miss. Replacing predictable
straight-line work with a pointer chase is not automatically cheaper on a
cache-accurate ARM9.

**More importantly, it is unmeasurable either way.** The 512 bytes of cache
arrays shift layout, and this ROM's placement sensitivity — documented since Task
37 — moves untouched buckets by roughly ±8,000. That is the noise floor of the
instrument for a change that alters code size. The predicted effect was 9,306.
A lever whose size is at the noise floor cannot be adjudicated by this A/B no
matter which way the dice land, and a favourable re-roll would have been luck
misread as evidence.

**Rule: do not chase a lever smaller than ~10,000 ticks with a change that moves
code layout.** Either find one large enough to clear the placement noise, or
measure it with the per-PC profiler windowed on the specific frames, which
attributes by program counter and does not care where the code sits.

Task 72 cleared this bar comfortably — 94,464 in the targeted bucket against the
same ±8,000 floor, which is why its attribution was never in doubt.
