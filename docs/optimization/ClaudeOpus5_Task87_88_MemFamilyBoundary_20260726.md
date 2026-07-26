# Tasks 87-88 - Where the mem* family stops paying

**Date:** 2026-07-26
**Status:** **Both REVERTED.** Source restored to the shipped Task 86 state. The
`memcpy`/`memset` direction is exhausted, and these two negatives establish
where its boundary is and why.
**Inputs:** `artifacts/task87-B.json`, `artifacts/task88-B.json`, both against
`artifacts/task86-B.json` (shipped).

## 1. Task 87 - inlining the *remaining* matrix copies is a loss

Task 86 converted nine 64-byte struct copies and measured `WORK-H` P95 -18,432.
A re-sample showed 70% of the remaining `memcpy` calls were still 64-byte matrix
copies, so Task 87 mapped every one exactly - `objdump` for the `bl memcpy`
addresses, `addr2line` for the source lines - and converted twelve more.

Call sites in the matrix family fell 22 -> 11, total 205 -> 194. The result:

| | Task 86 (shipped) | Task 87 | delta |
|---|---|---|---|
| `WORK-H` P95 | 1,742,080 | 1,759,808 | **+17,728** |
| `WORK-H` P50 | 1,340,032 | 1,351,040 | **+11,008** |
| VBlank 3-interval | 499 | 490 | -9 |

Better on 10 of 128 frames. **Worse than doing nothing.**

Each inlined 64-byte copy is ~32 instructions where the call was one. Twelve more
sites is roughly 1.5 KB of extra code in the draw path, and the instruction-cache
pressure costs more than the call overhead it removed. Task 86's nine sites were
the hot ones; the next twelve are colder, so they pay the code growth without
earning it back.

## 2. Task 88 - removing a redundant clear is also a loss

Two clears looked provably wasteful:

- `ndsRendererMtxLoadN64ToDS20p12` memsets 64 bytes, then writes all sixteen
  elements unconditionally. The clear only matters on the `src == NULL` path.
- `ndsRendererMtxIdentity20p12` memsets 64 bytes, then sets four diagonal
  entries - replaceable by sixteen explicit stores.

Both correct, both in single functions, so no inlining explosion. Result:

| | Task 86 (shipped) | Task 88 | delta |
|---|---|---|---|
| `WORK-H` P95 | 1,742,080 | 1,751,616 | **+9,536** |
| `WORK-H` P50 | 1,340,032 | 1,348,928 | **+8,896** |
| `STG` P95 | 376,512 | 385,984 | +9,472 |
| VBlank 3-interval | 499 | 488 | -11 |

Better on 12 of 128 frames.

Task 84 E1 predicted exactly this and it is worth quoting, because it was written
before the experiment: *"the clear also warms the cache for the writes that
immediately follow it. Delete it and some of those misses simply move to the
first real write of each field rather than disappearing."*

The measurement adds the second half of the mechanism. `memset` writes a cold
64-byte buffer as a block; sixteen individual stores each take their own miss and
write-allocate. Replacing one library call with sixteen scattered stores to
cold memory is slower even though it does strictly less work in instruction
terms.

## 3. The boundary, stated as a rule

Four experiments in this family now, and they line up:

| task | what | size | result |
|---|---|---|---|
| 85 | inline 2- and 4-byte `memcpy` | 2-4 B | **-40,384 P95** |
| 86 | inline 64-byte copies, 9 hottest sites | 64 B | **-18,432 P95** |
| 87 | inline 64-byte copies, 12 more sites | 64 B | +17,728 |
| 88 | remove/replace `memset` entirely | 64 B | +9,536 |

**Inlining a `mem*` call wins when the call overhead dominates what the call
does, and loses once it does not.** At 2-4 bytes the call is pure overhead and
inlining always wins. At 64 bytes it wins only where the site is hot enough to
amortise ~32 instructions of code growth. And a `memset` is never merely
overhead: it is also the cheapest way to touch cold memory, so removing one can
cost more than it saves.

The corollary for this campaign is that `mem*` is done. What remains is 194 call
sites carrying real bytes, where the library routine is the right tool.

## 4. Method note

Both negatives cost one build and one 128-frame sample each, and neither reached
a verifier. That is the intended economy - but the cheaper check would have been
to notice that Task 86's own result already implied a limit. Nine sites bought
18,432; if the effect were per-site rather than per-hot-site, twelve more should
have bought comparable. It did not, and the reason was available a priori in the
code-size arithmetic.
