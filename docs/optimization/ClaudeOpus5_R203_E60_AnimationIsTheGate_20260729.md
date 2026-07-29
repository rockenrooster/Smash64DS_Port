# R2-03 E60 — animation is 146,942 ticks/frame, and Task 78 stopped it on a self-vs-inclusive error

**Date:** 2026-07-29
**Status:** **ANSWERED, no runtime change.** Re-attributes the frame's largest
addressable block and overturns two standing board claims. Zero builds were
spent on the first half — the profiles were already on disk.
**Inputs:** `artifacts/task37-census/r203-e53-{control,excursion}/census.json`
(already on disk from E53), `artifacts/r2-03-e60-softfloat-callers.json` (new,
one 90 s run on `build-r2-e60-softfloat` at `cde5a4e`),
`artifacts/task92-softfloat-callers.json` (2026-07-27, for the delta).

## 1. Two claims this overturns

The board and `HANDOFF.md` have carried both of these for several cycles:

> **The `SRC` half is float→fixed on the collision path.** `gmcollision.c` is
> verifier-gated by the Task 9 state hash and re-bounding a bit-exact gate is the
> owner's call.

> *(Task 78)* **The ceiling is 84,550 and the target is 100,000.** A *perfect*
> animation compiler — one that evaluated every pose for free — would still fall
> 15,450 short.

**Both are wrong, and they are wrong for the same reason:** a leaf helper is
charged to itself, never to its caller, so every float operation the animation
path executes was booked to `__aeabi_fadd` / `__aeabi_fmul` and read as a
separate, larger, unrelated family. Task 92 built the caller attribution that
fixes this in 2026-07-27 and nobody went back and re-opened Task 78.

## 2. The frame, correctly attributed

Ordinary frames 876–879, the E53 control profile, on the graduated build.
Profiler cycles ÷ 2 = tick-HUD ticks; `armWaitForIrq` 150,837 is idle.

```
total 1,120,324 ticks/frame  −  idle 150,837  =  WORK 969,487
```

Soft-float + libm is **149,168 ticks/frame, 15.4% of WORK**. E60 re-ran Task 92's
caller census on the current build (33,821 samples). The distribution is
essentially unchanged from 2026-07-27 — `gcPlayDObjAnimJoint` 58.06% vs 57.17% —
so R2-04 E5's animation cache did **not** displace it.

| | self | via `fadd`/`fmul` | inclusive |
|---|---:|---:|---:|
| `gcPlayDObjAnimJoint` | 34,022 | **60,509** | **94,531** |
| `battleship_ftAnimParseDObjFigatree` | 12,115 | 5,703 | 17,818 |
| `ndsBaseGcPlayMObjMatAnim` | 5,201 | 4,560 | 9,761 |
| `gcPlayAnimAll` | 7,860 | 0 | 7,860 |
| `ftParamUpdateAnimKeys` | 6,191 | 0 | 6,191 |
| `gcParseMObjMatAnimJoint` | 3,828 | 123 | 3,951 |
| six more | 6,830 | 0 | 6,830 |
| **animation total** | **76,047** | **70,895** | **146,942** |

**Animation is 15.2% of WORK and 146,942 ticks/frame — larger than the entire
108,928-tick gap to the gate.** It is the largest addressable block in the frame.

By gate, of the 104,222 ticks/frame that `fadd`+`fmul` cost today:

| ticks/frame | share | gate |
|---:|---:|---|
| 76,429 | 73.3% | GAMEPLAY (state-hash frozen) |
| 15,709 | 15.1% | RENDERER (fidelity-gated) |
| 8,144 | 7.8% | second-order (float calling float) |
| 3,938 | 3.8% | UNRESOLVED |

**The renderer share is 15,709 and is therefore not worth architecture work** —
switch plan §3.9 puts 10–20K in the "usually too small" band. Every previous
reading that treated the renderer as the float target was sizing the wrong half.

## 3. Collision is not the float cost, by a factor of ~20

The `SRC`-half claim named `gmcollision.c`. Ranked by caller, the entire
collision family is:

```
1,479  ndsStageMPSegmentIntersection2D
  862  ndsMPFloorSegmentCrossesDownwardKernel
  708  gmCameraUpdateInterests
  678  mpProcessUpdateMain
```

Under 4,000 ticks/frame combined, against `gcPlayDObjAnimJoint`'s 94,531. A
perfect fixed-point collision conversion — the thing the board has been holding
for the owner — is worth less than the build-placement noise floor.

The `SRC` *bucket* attribution was not wrong; the *symbol* reading of it was.
`SRC` is source-derived gameplay code, and the animation player is source-derived
gameplay code. It was read as collision because collision is what
`gmcollision.c` suggested, and no caller-level measurement was ever laid against
it.

## 4. Why the excursion decomposition also misled

E53 profiled hitlag frames 910–913 against control 876–879: +420,227 ticks/frame.
Ranking that delta by symbol rather than by bucket:

- **376,434 ticks/frame across 151 symbols that are exactly zero on control** —
  the generic display-list interpreter, i.e. the E54 fighter fallback. Confirmed.
- **173,981 across symbols present on both**, and the top of that list is
  *renderer matrix* work in fixed point (`LoadHardwareMatrixPair` +10,185,
  `MtxMul20p12` +8,478, `BuildDObjLocalMatrix` +8,080,
  `BuildDObjWorldMatrix` +7,739), not float.

**`__aeabi_fadd` does not appear in the growth list at all.** Float is a flat
per-frame cost, not an excursion cost. That is what makes it the right target:
removing it moves P50 and P95 together, one for one, whereas E32 only touches
five frames.

## 5. Task 78's arithmetic, corrected in its own window

Task 78 §3 totalled the animation symbols at 82,807 self ticks, added 1,743 for
hierarchy transform, and concluded the ceiling was 84,550 against a 100,000
target. Its own §4 then lists `__aeabi_fadd` + `__aeabi_fmul` = 119,912 as a
*separate* family and calls it "1.45× the entire animation subsystem".

Applying E60's measured caller shares to that same 119,912:

```
  animation self                                    82,807
+ fadd/fmul called from the animation path (67.9%)  81,429
= inclusive                                        164,236   =  1.64x the target
```

**Task 78 stopped the animation compiler at 1.64× its target believing it was
0.85×.** The two numbers it needed were both in its own report, on facing pages,
in different families.

Tasks 95 and 96 then attacked what remained under that framing — layout and
stall, on the premise Task 92 §5 recorded as *"the data layout is where its
recoverable half sits, not the arithmetic, which is frozen"* — and both were
refuted (Task 95: hoist works, frame regresses; Task 96: 0 of 15,687 adjacent
`AObj` pairs, flattening saves 259.7 lines/frame, far too few for 68,000 ticks
of stall). **Those refutations stand. They refute the layout route, not the
arithmetic route, and the arithmetic route has never been attempted.**

## 6. What "frozen" actually rests on

The freeze is the **Task 9 state hash verifier**, not the product contract.
`PROJECT_GOAL.md` says the opposite in four places:

- "Mechanical equivalence is required. Bit-exact or numerically identical
  execution is not."
- "Small numerical differences are acceptable when they do not materially alter
  gameplay or game feel."
- Allowed Optimization Techniques lists **"fixed-point replacements"**,
  **"precomputed animation data"**, **"quantized animation poses"**, **"reduced
  animation interpolation"**, "compile-time asset conversion", "aggressive
  baking".
- "Compute Once, Not Every Frame — a match may spend several seconds preparing
  … animation programs … precomputed animation data".

Task 77 E1's "the cosmetic-only joint set is EMPTY for both Mario and Fox" is
correctly cited against *dropping* joints, and E57 (this cycle) independently
confirmed why: `gmCollisionGetFighterPartsWorldPosition` (`gm/gmcollision.c:489`)
places every hitbox by walking the live joint chain. **Neither result forbids
computing the same pose more cheaply.** They forbid computing a *different* pose.

## 7. The experiment that follows, and why it needs no gate change

`gcPlayDObjAnimJoint` evaluates a pose from `AObj` tracks whose state comes from
parsing a figatree script. If the pose at frame N of animation A is a pure
function of (A, N) — no gameplay-state dependence beyond `anim_speed` — then it
is **precomputable at load time using the identical float arithmetic**, and the
runtime becomes a table index. That is bit-exact by construction: the Task 9
state hash never sees a different value, so nothing needs re-bounding and the
owner decision disappears.

`PROJECT_GOAL.md` explicitly prices this trade in the project's favour: *"A
solution using nearly all available RAM at 900K ticks is preferable to one using
little RAM at 1.15M ticks."* The repo already has the precedent —
`scripts/generate_pupupu_water_aot.py` AOT-compiles the *material* animation
script, reproducing MIPS single-precision rounding operation by operation, which
is exactly the fidelity a joint table needs.

**E61 must size the table before any code is written** — the standing rule this
campaign learned the hard way, and the one Task 78 broke in the other direction.
The three integers that decide it:

1. distinct (animation, frame) pairs reachable in the Boundary match;
2. bytes per pose (`rotate`/`translate`/`scale` per animated DObj);
3. whether `anim_speed` takes values other than 0 and 1, which would make the
   frame index non-integral and break the pure-function premise.

If the product exceeds the RAM budget, the fallback is not a smaller table but a
**per-fighter generated evaluator** — `PROJECT_GOAL.md`'s named preference for
`Mario_Update()`-style specialization — which removes the script parse and the
`AObj` chain walk while keeping the arithmetic.

## 8. Harness defect fixed at its seam

`scripts/census-softfloat-callers.ps1` printed its absolute scale from a
hardcoded `191,810 ticks/frame (Task 81 partition)`. That partition predates
R2-04 E5 and R2-03 E46; the true figure on the current build is **104,222** for
`fadd`+`fmul`. The script reported shares correctly and absolute ticks 84% too
high. Fixed to state the constant's provenance and staleness rather than assert
it, since the census cannot measure the total itself.
