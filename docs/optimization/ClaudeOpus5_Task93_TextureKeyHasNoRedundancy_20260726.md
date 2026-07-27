# Task 93 E0 — 25 binds a frame, 22 distinct keys: there is nothing to memoise

**Date:** 2026-07-26
**Status:** **STOP. Texture direction closed for the third and final time**, now
with the numbers that close it rather than an inference. No perf change; the
measurement is recorded against the constant it validates.
**Inputs:** `artifacts/task93-texkey.json`,
`scripts/census-texture-key-rebuild.ps1`.

## 1. Why this ran despite Tasks 79, 80 and 81

Task 81 closed the texture direction believing the residual was upload work. A
later census contradicted its premise: **zero texture uploads and zero cache
evictions** in the window. That made
`ndsRendererHardwareResolveOrBindTexture`'s 42,356 ticks/frame pure lookup — and
lookup that repeats is memoisable, which left the direction open in principle
with a note that it "needs a generation counter provably covering all 59 key
fields".

Before building that generation counter, this task asked the only question that
decides whether it is worth building: **does the key actually repeat?**

## 2. It does not

Two stops, differenced, on the Boundary battle:

```
BindCalls              25.0 / frame
PreflightCalls         16.0 / frame   (resolved != NULL, pure resident lookup)
ConsecutiveRepeat       3.0 / frame   (12.0%)

trace: 256 requests, 22 distinct keys
```

**25 binds a frame, 22 distinct keys.** Each frame binds very nearly a distinct
set of textures, once each. The consecutive-repeat rate is 12%, so the cheapest
possible memo — a one-entry front cache holding the last key — would hit on 3
binds out of 25.

A generation counter cannot beat that. Its whole purpose is to let a *lookup*
skip the *rebuild*, and there are only 3 rebuilds a frame worth skipping. At
1,694 ticks per bind that is ~5,000 ticks/frame against a ±8,000 placement noise
floor, before subtracting the counter's own maintenance cost across all 59
fields and the oracle assertion that would have to guard it.

**Do not build the texture generation counter.** The direction is closed on its
own numbers, not on an inference about uploads.

## 3. What the trace does prove: the cache size is right

Replaying the 256-request trace against a FIFO of each candidate size, the same
way Task 90 sized the light-shade LUT:

```
cache size   misses   miss rate
         4      225       87.9%
         8      225       87.9%
        16      225       87.9%
        32       22        8.6%
compulsory       22     (floor)
```

`NDS_RENDERER_HW_TEXTURE_CACHE_COUNT` is **48**. The working set is 22, the
compulsory floor is reached by 32, and 48 carries real headroom — so the cache
evicts nothing in steady state, which is exactly why Task 81 measured zero
evictions and zero uploads.

This is the opposite result from Task 90, and worth stating plainly: **the same
one-run method that found a cache four times too small found this one correctly
sized.** The method distinguishes the two; a hit rate would not have.

The constant now carries that measurement inline and names the script that
re-derives it, so the next person to look at 48 does not have to re-run this
investigation to learn it is deliberate.

## 4. The other per-bind costs, and why they are also not levers

At 25 binds/frame the texture family divides as:

| symbol | ticks/frame | per bind |
|---|---|---|
| `ndsRendererHardwareResolveOrBindTexture` | 42,356 | 1,694 |
| `ndsRendererSyncTextureTile` | 17,656 | 706 |
| `ndsRendererHardwareBindTextureName` | 6,869 | 275 |
| `ndsRendererHardwareFindTexture` | 6,004 | 240 |
| `ndsRendererHardwareTextureKeyHash` | 5,367 | 215 |
| `glBindTexture` | 4,916 | 197 |

`ndsRendererSyncTextureTile` is flat-copies of 19 tile fields and is called from
four places, including unconditionally on every bind — and `G_SETTILE` and
`G_SETTEXTURE` already sync when they mutate the tile, so the bind-path call is
redundant whenever neither has run since. That is a real redundancy, but the
bind path is roughly a third of the callers, so it is worth **~5,900
ticks/frame** — under the noise floor, and it would cost an audit of every
mutation site of `stats->texture_tiles[*]` to invalidate correctly. Not taken.

`memset(&key, 0, sizeof(key))` zeroes 236 bytes per bind and looks like free
money. It is not removable: `ndsRendererHardwareTextureKeyEqual` compares keys
with `memcmp` over the whole struct, so the padding bytes the memset clears are
load-bearing for equality. Removing it would make key comparison depend on
stack garbage.

## 5. Where this leaves the campaign

Directions now closed with measurements rather than estimates:

| direction | closed by | why |
|---|---|---|
| texture memo | **Task 93** | 22 distinct keys in 25 binds; 12% repeats |
| soft-float conversion | Task 92 | 73% state-hash frozen; eligible ~20,000 |
| dense-vertex re-shade | Task 90 | 0.0% redundant over 541 iterations/frame |
| animation compiler (as scoped) | Task 77 E1 | cosmetic-only joint set is empty |
| `mem*` micro-fixes | Tasks 87/88 | boundary mapped, both arms regressed |
| ITCM placement | Tasks 83/89 | packed; layout at a local optimum |

**No class in the frame above 120,000 ticks now has an open lever except
animation**, re-scoped by Task 92 §5 to exactness-preserving layout work — flat
contiguous channel arrays replacing the `aobj->next` walk, precomputed traversal
order, and hoisting the two loop-invariant tests out of the per-channel loop.
That is a subsystem task, not a one-build change.

`WORK-H` P95 stands at 1,726,912 against the 1,120,000 gate: **606,912 over.**
