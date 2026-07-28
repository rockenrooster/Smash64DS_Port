# R2-03 E0 — soft-float is one function, not a culture

> **Mislabelled, corrected 2026-07-28.** "R2-03" in
> `Smash64DS_Runtime2_SwitchPlan.md` §7 is **Fighter direct draw (static pose)**.
> This work is not that phase and does not advance it. It is a Runtime 1 harvest
> of the kind §1 sanctions ("keep harvesting exact wins while R2 is built"). The
> filename stays per the never-rename rule; the phase number in it is wrong.
> R2-03 remains unowned.

**Date:** 2026-07-28
**Phase:** E0 sizing only. No runtime change, no build, no emulator run beyond
the R2-00c capture this reuses.
**Finding:** of 181,817 ticks/frame of soft-float, **`gcPlayDObjAnimJoint`
accounts for 40,211** — 22% of the block from a single function, and 36,236 of
that is `__aeabi_fadd` alone.

---

## 1. Why the caller had to be found, and why a profile could not do it

R2-00c ranked soft-float as the largest block in the frame at 177,857
ticks/frame, 12.3% of work — and measured it at **1.19 cycles per instruction**.
It is not stalled. `__aeabi_fadd` is already hand-written ITCM assembly from
Task 16. There is nothing to win by making it faster; the only lever is calling
it less, so "who calls it" is the entire question.

A PC profile cannot answer that. Every cycle is recorded inside `__aeabi_fadd`
and the caller is invisible. But **the call instructions are themselves PCs**,
and the profiler records how many times each retired. So:

```
cost charged to caller F for routine R
    = (times F's `bl R` sites retired) x (R's ticks / R's total calls)
```

Exact up to R's cost varying with its arguments. This needs no new run — it
reads the R2-00c per-frame capture against the disassembly.

One trap worth recording: matching `b`/`bx` as well as `bl`/`blx` counts a
routine's own internal branches as calls to itself, and put `__aeabi_fadd` at
the top of its own caller list with 62,029 ticks/frame of nonsense. Call sites
are `bl`/`blx` only.

## 2. Where the float goes

127 frames, `build-waitaudit-pf`. 59% of the block attributes to a named caller;
the remainder is indirect calls and veneers, which is enough to rank but not to
audit.

| ticks/frame | caller | dominant routine |
|---|---|---|
| **40,211** | **`gcPlayDObjAnimJoint`** | `__aeabi_fadd` 36,236 |
| 15,016 | `sqrtf` | `__ieee754_sqrtf` 14,257 |
| 5,158 | `battleship_ftAnimParseDObjFigatree` | `__aeabi_fadd` 2,765 |
| 3,916 | `guMtxCatF` | `__aeabi_fadd` 3,916 |
| 3,619 | `ndsBaseGcPlayMObjMatAnim` | `__aeabi_fadd` 3,071 |
| 2,946 | `ndsRendererHardwarePrepareLitDirection` | `__aeabi_fadd` 1,958 |
| 2,771 | `func_ovl2_800ED490` (gmcollision) | `__aeabi_fadd` 2,770 |
| 2,194 | `ndsStageMPSweepFloorLoopSweep` | `__aeabi_i2f` 1,202 |
| 2,042 / 2,016 | `__kernel_cosf` / `__kernel_sinf` | `__aeabi_fadd` 1,976 each |

Cost per call, which is its own result:

| ticks/call | routine | calls/frame | ticks/frame |
|---|---|---|---|
| 30.6 | `__aeabi_fadd` | 2,320 | 70,991 |
| **223.1** | `__ieee754_sqrtf` | 64 | 14,258 |
| 116.3 | `__ieee754_rem_pio2f` | 14 | 1,589 |
| 8.2 | `__aeabi_i2f` | 410 | 3,367 |
| 5.3 | `__aeabi_fcmpeq` | 1,263 | 6,655 |
| 7.4 | `__aeabi_fcmplt` | 372 | 2,742 |

## 3. Two targets, and they are different in kind

**(a) `gcPlayDObjAnimJoint` — 40,211 ticks/frame.** 36,236 ticks of
`__aeabi_fadd` at 30.6 ticks each is roughly **1,184 float additions per
frame** from one function. That is the AObj animation accumulator: each joint
track advances its value by a per-frame rate, and ~57 DObjs across two fighters
with translate/rotate/scale tracks is exactly that shape. The function also
carries 34,148 ticks/frame of its own self-time, so the joint-playback path is
~74,000 ticks/frame all in — **5.1% of the frame's work in one place.**

Converting the accumulator to fixed point removes most of the 36,236. But this
is a **gameplay path**, not a rendering one: the accumulated value becomes the
pose, and hitboxes derive from part positions. So it is verifier-gated and
mechanically-equivalent-or-nothing, with the Task 37 state-hash determinism
check as the guard — not a fidelity-budget change the owner can eyeball.
`PROJECT_GOAL.md` allows a DS-side equivalent of a `decomp/` algorithm; it does
not allow the poses to drift.

**(b) `__ieee754_sqrtf` — 14,258 ticks/frame at 223 ticks per call.** Only 64
calls a frame, but each one costs as much as seven float adds. This is a
newlib generic square root running on a core with no FPU. A fixed-point
integer sqrt, or a LUT plus one Newton step, is a well-bounded, self-contained
replacement with no gameplay surface of its own beyond its callers'
tolerance — `syVectorMag3D`, `ndsRendererHardwarePrepareLitDirection`,
`syMatrixLookAtReflectF`. **This is the cheaper first cut and it should go
first**, because it is small, isolated, and it calibrates how much the
verifier tolerates before the animation accumulator is touched.

## 4. What E1 should do

1. **`__ieee754_sqrtf` first.** Replace with a fixed-point sqrt behind
   `NDS_R2_FIXED_SQRT`. Size: ~14,258 ticks/frame, kill line ~5,000 (the
   placement noise floor, Task 100). Gate: `WORK` P50/P95 A/B on one tree, plus
   Boundary, plus the state hash.
2. **Then `gcPlayDObjAnimJoint`.** Size it properly at E0 before writing —
   `TASK_STANDING_RULES.md` §"size a rewrite before you scope it". The number to
   beat is 40,211 of soft-float plus whatever of its 34,148 self-time the
   conversion also removes.

Do not attack "soft-float" as a programme. It is 181,817 ticks/frame spread over
640 functions with static call sites, but **two functions hold 30% of it**, and
the rest is a tail that will not repay the risk.

## 5. Cost

One disassembly and one arithmetic pass over an existing capture. No build, no
emulator run.
