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

---

## 5. Task 89 - and placement is at a local optimum too

One more attempt, deliberately in a *different* family in case the boundary was
specific to `mem*`.

Task 82 moved four symbols out of `.text.hot.draw` into ITCM and left **2,672 of
its 8,192 bytes unused**. The tier table in `include/nds/nds_task37_itcm.h` rates
`.text.hot.draw` at 2.63 cycles/instruction against `.main`'s 3.29, so refilling
that space with the densest `.main` symbols by non-mem stall per byte should pay.

Seven admitted, 1,088 bytes, ~28,000 ticks/frame of non-mem stall nominally in
reach: `ndsRendererHardwareBindTextureName`, `ndsRendererLoadHardwareRawComposedMatrix`,
`ndsRelocFindLoadedFileContaining`, `ndsRendererAdapterStageWorldSourceKeyMatches`,
`ndsFighterDisplayContractCountFlags`, `ndsRendererAdapterFindStageWorldEntry`,
`ndsRendererNativeApplyRootLightPreamble`. Section grew 5,628 -> 6,716 bytes and
the symbols relocated, both verified in the ELF.

| | Task 86 (shipped) | Task 89 | delta |
|---|---|---|---|
| `WORK-H` P95 | 1,742,080 | 1,753,728 | **+11,648** |
| `WORK-H` P50 | 1,340,032 | 1,351,424 | **+11,392** |
| VBlank 3-interval | 499 | 491 | -8 |

Better on 10 of 128 frames. Reverted.

## 6. Three failures with one shape

| task | family | mechanism | result |
|---|---|---|---|
| 87 | `mem*` | inline more 64-byte copies | +17,728 |
| 88 | `mem*` | remove redundant clears | +9,536 |
| 89 | placement | refill `.text.hot.draw` | +11,648 |

Three independent, separately-reasoned changes, in two different families, all
regressing by a similar amount. That is not three unlucky guesses - it says the
build **sits at a local optimum**. Tasks 82, 85 and 86 took the wins that were
available at this granularity, and the frame is now arranged such that small
perturbations in any direction cost more than they return.

The practical rule for the next session: **stop looking for micro-fixes.** The
remaining levers named by measurement are design-level and both need work before
they can be attempted:

- **Texture lookup memo**, up to ~40,000 ticks. Task 81 closed this on the belief
  that the residual was upload work; a later census found **zero texture uploads
  and zero cache evictions** in the window, so the 42,420 ticks in
  `ndsRendererHardwareResolveOrBindTexture` are lookup, and memoisable in
  principle. It needs a generation counter that provably covers all 59 key
  fields, with an oracle assertion, not care.
- **`ndsRendererInitStats` call reduction**, up to 41,468 ticks. Task 84 E2 closed
  the two cheap routes; what remains is understanding why 11.7 traversals a frame
  each need a full re-initialisation.

Neither is a one-build change, and attempting either as one is how the last three
tasks went.
