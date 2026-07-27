# Task 92 E0 — Soft-float is 73% frozen, and Task 78 was stopped on a number 2.2× too small

**Date:** 2026-07-26
**Status:** **STOP on soft-float conversion.** One finding reopens Task 78 (§4).
No runtime change.
**Inputs:** `artifacts/task92-softfloat-callers.json`,
`scripts/census-softfloat-callers.ps1`. Two runs, 27,948 and 15,271 samples.
**Follows:** Task 81's partition, which named soft-float as the only
instruction-bound class in the frame and this attribution as the next task.

## 1. Method, and why it worked this time

`break <symbol>` in GDB lands **past the prologue**, where `lr` has often been
spilled. That is why Task 84's `memcpy` attribution resolved 82% of its samples
into BSS data objects — addresses that cannot be return addresses — and why those
numbers were discarded rather than reported. `break *<exact address>` from `nm`
stops on the first instruction, where `lr` still holds the caller. Task 85 proved
this out; this run reuses it against `__aeabi_fadd` (0x01fff264) and
`__aeabi_fmul` (0x01ff82cc), which are 119,912 of the class's 191,810
ticks/frame.

Two independent runs agree to within one point on every gate share, so the split
is stable and not a sampling artifact.

## 2. The gate split

| gate | share | ticks/frame of `fadd`+`fmul` |
|---|---|---|
| **GAMEPLAY (state-hash frozen)** | **73.1–74.0%** | **~88,700** |
| RENDERER (fidelity-gated) | 15.1–16.7% | ~20,000 |
| second-order (float calling float) | 7.8–8.5% | ~10,200 |
| unresolved | 2.3–2.5% | ~3,000 |

**Three quarters of the float traffic is in code the Task 9 state hash and
`PROJECT_GOAL.md`'s mechanical-equivalence contract forbid changing.** It cannot
become fixed point whatever it costs.

The renderer-eligible remainder is **~20,000 ticks/frame** — and converting it
means fixed-point substitution in matrix and lighting math, which is a
fidelity-gated change needing the owner's visual approval, for a return only
2.5× the ±8,000 placement noise floor. Task 50 closed divide and sqrt on the
same reasoning (eligible ceiling ~0.55% of budget). Add and multiply now close
the same way.

**Do not extrapolate the 16.7% to the whole 191,810 class.** This run sampled
`fadd` and `fmul` only; `fdiv` and `sqrtf` were measured separately by Task 50
and have a different, smaller renderer share. The defensible eligible figure is
~20,000, not ~32,000.

## 3. The callers

| caller | share | gate |
|---|---|---|
| **`gcPlayDObjAnimJoint`** | **54.2%** | GAMEPLAY |
| `battleship_ftAnimParseDObjFigatree` | 5.7% | GAMEPLAY |
| `ndsBaseGcPlayMObjMatAnim` | 4.2% | GAMEPLAY |
| `__kernel_cosf` / `__kernel_sinf` | 6.9% | second-order |
| `syMatrixLookAtReflectF` | 3.8% | RENDERER |
| `guMtxCatF` | 3.7% | RENDERER |
| `ndsRendererHardwarePrepareLitDirection` | 3.7% | RENDERER |
| `syMatrixF2L` | 1.8% | RENDERER |
| remaining 61 callers | ~16% | mixed |

69 distinct callers; the top 18 are 91.8%. One function is over half.

`gcPlayDObjAnimJoint` (`decomp/.../sys/objanim.c:714`) is the animation joint
player. Its `nGCAnimKindCubic` branch is a Hermite evaluation costing roughly 12
multiplies and 8 adds per animated channel per frame, and the sample shows ~23
distinct call sites inside it hit an identical number of times — one loop, every
channel, every frame.

## 4. The finding that matters: Task 78 was stopped on the wrong number

`ClaudeOpus5_Task78_AnimationLeverE0_20260726.md` **STOPPED the animation
compiler** because "the whole family — 32 symbols, `gcPlayDObjAnimJoint` through
`battleship_ftAnimParseDObjFigatree` — is 82,807 ticks/frame", against a
≥100,000 target.

That 82,807 counted the animation symbols' **own** cycles. It did not count the
soft-float those symbols call into, because in a per-symbol census a leaf helper
is charged to itself and never to its caller. The attribution above supplies the
missing half:

```
animation share of fadd+fmul     64.1%  x 119,912  =   76,864 ticks/frame
Task 81 animation class (own)                      =  106,700
                                                     ---------
animation, true cost                               ~  183,564 ticks/frame
```

**The animation path is the largest single subsystem in the frame** — larger than
`fighter: native production` (255,061 includes shading and GX, and is a class not
a subsystem) and 2.2× the figure it was killed on. Task 78 was judged against a
≥100,000 gate it clears comfortably.

This is the same error class Task 81's partition was built to end, caught one
level deeper: the overlapping-category problem does not stop at the class
boundary, it reaches into every leaf helper a subsystem calls.

## 5. What this does and does not authorize

**Does not authorize** re-opening Task 78 as originally scoped. Its win was to
come from quantization, rate reduction and lossy pose tables — and Task 77 E1
measured the cosmetic-only joint set as **empty** for both Mario and Fox, so
none of those techniques is available. Animation joints carry hurtboxes; they are
gameplay, verifier-gated, and 73% of this float is frozen for exactly that
reason.

**Does authorize** a re-scoped Task 78 whose win must come from
*exactness-preserving* reorganization only, which the plan already permits on
gameplay bones: precomputed traversal order, flat contiguous channel arrays
replacing the `aobj->next` linked-list walk, and hoisting the two loop-invariant
tests (`dobj->anim_wait != AOBJ_ANIM_END` and
`dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM`) out of the per-channel loop. The
animation class is 68.4% stall, so the data layout is where its recoverable half
sits — not the arithmetic, which is frozen.

That is a real subsystem task, not a one-build change, and it should be scheduled
as such rather than started at the end of a session.

## 6. Verdict

- **Soft-float as a conversion target: CLOSED.** 73% frozen by contract; the
  eligible ~20,000 needs owner fidelity approval for 2.5× the noise floor.
  Together with Task 50 (divide/sqrt) the whole class is now accounted for.
- **Task 81's remaining question is answered, and the answer is the pessimistic
  branch it named:** with soft-float closed, **no class in the frame above
  120,000 ticks is actionable under the current contracts.** The frame is 52–89%
  stall in sixteen of seventeen classes, ITCM is packed, and Tasks 87–89 showed
  the layout at a local optimum.
- **One door is open and it is large:** animation at ~183,564, re-scoped to
  exactness-preserving layout work. It is the only remaining lever of that size.

`WORK-H` P95 is 1,726,912 against a 1,120,000 gate — **606,912** over. Closing it
requires either the animation layout task in §5, or the owner relaxing a
contract. This document does not claim a third option exists.
