# Row-blocked 20.12 matrix multiplies — REVERTED, and the route arms interfere

Cycle 119. Same-binary route A/B on `builds/build-c119-mtx-route`
(`NDS_R2_BOTH_CPU=1`, `NDS_R2_MTX_ROUTE=1`), 1,600 presented frames from 438,
stride 96, DLDI ON. One ELF, four route values.

## Verdict

**REVERT.** Not because it is wrong — it is bit-exact and it really does halve
the executed instruction count in the general kernel — but because no reading
clears the ±8,544 floor and the readings contradict each other.

| route | `WORK-H` P50 | P95 | ΔP95 vs control |
|---|---:|---:|---:|
| 0 control | 967,168 | 1,263,488 | — |
| 1 general only | 966,912 | 1,261,120 | −2,368 |
| 2 affine only | 970,496 | 1,258,304 | −5,184 |
| 3 both | 970,944 | 1,270,400 | **+6,912** |

**The bits are not additive.** Each alone reads as a small win; together they
read as a 6,912 loss — a 14,464 discrepancy. Route 2 also moves P50 and P95 in
OPPOSITE directions on a change that can only ever remove work. Both are
signatures of an effect below the instrument's resolution.

## Why it was worth trying, and it was measured properly

`--attribute-leaves` put the two 20.12 multiplies at **29,932 ticks on a tail
frame across 72.8 calls**, on 80/80 frames, and both walked `rhs->m[k][col]`
DOWN A COLUMN while reloading `lhs->m[row][k]` for every column. Row-blocking
keeps four accumulators per row and reads `rhs->m[k][0..3]` consecutively —
same products, same accumulation order per output cell, so **bit-exact**.

Engagement and exactness were both proven in the same runs, which is what makes
the negative trustworthy:

- `gNdsR2MtxObservedRoute` reads back 0/1/2/3 — the word the KERNEL loaded, not
  the word GDB wrote.
- `gNdsR2MtxRowBlockedAffineCount` 104,550 of 104,767 entries: 99.8% of affine
  calls took the new path.
- `gNdsBattleTextHudP0Damage`/`P1Damage` **130/51 in every arm**, and
  `gNdsR2CubicEvals` 292,679 in every arm. Same match throughout — so unlike
  slice 41 this delta really is a cost delta, and it is still too small.

## Two mechanisms found, both worth keeping

**1. The ITCM workaround ate the optimization.** The scalar form
(`s64 a0..a3`) overflowed `.itcm` by 76 bytes — that section had 584 bytes free
and the route puts BOTH bodies in it. Replacing the scalars with `s64 acc[4]`
to fit is what shipped, and an indexed array is exactly what makes GCC keep
accumulators in memory: stack frame **76 → 124 bytes**, 24 sp-relative accesses
in the hot loop. A register-blocked kernel whose registers are on the stack is
not a register-blocked kernel. **Any A/B route inside an ITCM function has to
fit both arms**, and that constraint can silently invert the thing being
measured.

**2. A same-binary route is not free of interference when both arms are hot.**
The route form exists to delete the cross-build placement floor and it does —
but with route 3 both new bodies execute, so the lab ROM pays I-cache for code
a shipped build would not contain. That is the only reading consistent with
1 alone and 2 alone each helping while 1+2 hurts. **Multi-bit routes over hot
code cannot be summed**, and a bit combination is its own measurement.

## What was ALSO learned: the poke can silently not reach the CPU

The first A/B on this route returned both engagement counters at **zero in the
candidate arm** with byte-identical buckets in both arms — which reads exactly
like "the optimization does nothing". It was a measurement artifact.
`-SetGlobals` verifies its poke by reading the value back, but that readback
goes through GDB, which reads emulated RAM: **it proves the WRITE landed, not
that the ARM9 observes it.** In that link the route word sat at `0x020e3760`,
sharing its 32-byte cache line with `sNdsIFCommonNextOamID` (written per
sprite); the CPU's dirty line writes back its own stale copy over the poke.
`aligned(32)` aligns the START of a 4-byte word and leaves 28 bytes of the line
to neighbours, so it does not prevent this.

What caught it was that the two arms were byte-identical — one run relabelled,
not agreement. **Every route arm should carry a counter written from inside the
guest recording the value the CODE loaded**, not just the harness readback.
Four route words in this tree use the same `aligned(32)` idiom and each is one
relink away from the same silent failure.

## Do not retry

Row-blocking these two kernels is not worth another build. The arithmetic win
is real but bounded by ~5,000 ticks, under the floor, and the lane's remaining
cost is not in the multiply ordering. The live follow-ups from the same
attribution are `ndsRendererAdapterBuildPersistentStageWorldMatrix` (16.35
calls, 9,555 tk, **584 tk/call**) and `ndsRendererAdapterBuildDObjXObjMatrix`
(57.50 calls, 12,233 tk). `ndsRendererLoadHardwareSplitMatrices` is already
closed — R2-03 E23 tried eliding its redundant projection half, engaged on
93.8% of loads, worth −3,008, reverted.
