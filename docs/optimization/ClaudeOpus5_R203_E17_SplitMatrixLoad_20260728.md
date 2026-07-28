# R2-03 E17 — the split matrix load: −17,600 and the vector matrix E16 needs

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** KEEP on measurement, **awaiting the owner's visual approval before
graduation.** `NDS_R2_FIGHTER_HW_MTX`, default 0. Boundary green on the default.

## 1. What changed

The fighter composed its modelview and projection on the CPU
(`ndsRendererAdapterComposeNativeRootMatrix`, one 4x4 20.12 multiply per root)
and loaded the product as a single matrix with an identity projection. Now the
projection and modelview are loaded separately and the geometry engine performs
the multiply — the engine E14 measured idle on 946 of 946 fighter submissions.

The compose is skipped outright, not merely bypassed. E16b established it has no
other consumer under canonical mode 9.

**Exactness of the scaling.** `ndsRendererBuildRawHardwareMatrix` divides row 3
of the composed matrix by the world-unit shift. Under the row-vector convention
`C[3] = M[3] x P`, so scaling `M`'s row 3 *before* the multiply gives the same
row 3 after it — the scaling commutes with the right-multiply. The only
difference from the old path is that the engine rounds the product internally
rather than the CPU rounding it in 20.12.

## 2. Result

Identical source, flag 0 versus 1, 128 presented frames each.

| bucket | A: composed | B: split | delta |
|---|---:|---:|---:|
| **WORK P50** | 1,118,144 | 1,099,584 | **−18,560** |
| **WORK P95** | 1,585,408 | 1,528,064 | **−57,344** |
| WORK-H P50 | 1,112,640 | 1,093,632 | −19,008 |
| **FTR P50** | 507,456 | 489,856 | **−17,600** |
| FTR P95 | 969,856 | 962,624 | −7,232 |
| STG P50 | 175,552 | 175,296 | −256 |
| **VBlank 2 / 3** | 320 / 233 | **381 / 167** | **+61 frames at 30 FPS** |

`STG` moves 256 ticks — the stage is untouched and this is the placement floor,
which is the control on whether the `FTR` movement is real.

**The saving matches the mechanism.** ~28 roots a frame, one 4x4x4 20.12 multiply
each: 64 multiplies at roughly 10 ticks is ~640 per root, ~18,000 a frame against
a measured 17,600. A cut whose size matches its cause is a different claim from a
cut that merely correlates with a flag.

The control arm carries one pathological frame (`ALL` max 5,874,496, `FTR` max
4,704,128) which inflates its mean; the candidate's max is 2,800,192. Percentiles
are the read, per the standing rule, and they agree with the histogram.

## 3. Why this is the right first step of E16

The stated purpose is the tick saving, which stands on its own. The second
purpose is that it establishes the matrix state hardware lighting requires: the
vector matrix now holds the **modelview alone** rather than the composed MVP, so
normals can be transformed correctly.

## 4. A correction to E16a/E16b's stated reason

Both documents claimed the fighter loads through "matrix mode 1, position only,
which never updates the vector matrix", and that hardware lighting was blocked by
a missing vector matrix.

**That was wrong.** libnds names DS matrix mode 1 `GL_POSITION` and mode 2
`GL_MODELVIEW`; the existing code already used `GL_MODELVIEW`, so mode 2, and a
vector matrix was being written on every load. It was found when
`GL_MODELVIEW_VECTOR` — a name invented from the wrong model — failed to compile.

The prerequisite survives, for a different and narrower reason: **what was landing
in the vector matrix was the composed MVP**, and normals must never be rotated by
a projection. Same fix, same necessity, wrong cause. Recorded because a reader
acting on the original text would go looking for a matrix-mode change that was
already there.

## 5. `noinline` is load-bearing here

`ndsRendererLoadHardwareSplitMatrices` is marked `noinline` because the caller
`ndsRendererExecuteNativeFighterOwnerProduction` lives in
`.itcm.native_fighter` and ITCM has no headroom — inlining it overflowed the
region by 72 bytes. It runs once per root, so the out-of-line call is free at
this scale. Anything added to that call chain must respect the same limit.

## 6. Disposition

**Not graduated.** Default stays 0 and no `override` is set in the published
`smash64ds-battle-playable-hwtri` block. This is a rendering-side change: vertex
positions now round in hardware rather than on the CPU, a sub-pixel difference,
and `AGENTS.md` gates that on the owner's visual approval rather than on
exactness.

Evidence for that decision: `artifacts/visibility` capture of the candidate at
frame ~480 shows both fighters and the stage rendering correctly with no
distortion.

**Boundary passes in both configurations**, flag 0 and flag 1. The second run
matters and was nearly skipped: the first pass verified only the *default*, which
is the arm that does not ship if this graduates. Approving a change on a green
run of the configuration it replaces is the same error as validating only in the
configuration that carries the instrument — the rule this campaign wrote after
R2-02 F and E12, arrived at from the opposite direction. The candidate
configuration is now verifier-covered, which is also what `AGENTS.md` requires of
any user-facing ROM.

**For the owner:** the ask is to look at the candidate ROM and confirm nothing
reads wrong. On approval, add `override NDS_R2_FIGHTER_HW_MTX := 1` to the
published block and the tick-HUD block, and E16's hardware lighting builds on
top of it.
