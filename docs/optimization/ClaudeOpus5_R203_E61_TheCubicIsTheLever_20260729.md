# R2-03 E61 — it is the cubic. The pose table is refuted by size; fixed-point is worth ~50,000

**Date:** 2026-07-29
**Status:** **ANSWERED.** Sizes the three integers E60 asked for, refutes the
load-time pose table before a subsystem was spent on it, and leaves exactly one
lever with a price on it. No runtime change; census behind `NDS_R2_ANIM_CENSUS`,
default 0.
**Inputs:** one 90 s run on the census ROM, counters read through
`sample-tick-hud-buckets.ps1 -ExtraGlobals`. Window ≈ 925 battle frames.
**Method:** the Task 95 interposition — `#define gcPlayDObjAnimJoint
ndsBaseGcPlayDObjAnimJoint` before the decomp `#include`, port-side counter that
delegates the real work unchanged. Counting only; it cannot change a value.

**Instrument cross-check:** longest chain **9**, identical to Task 96's
independent measurement, and 96,308 calls ÷ Task 96's 104.1 calls/frame = 925
frames against a run that ended at 934. Two instruments, same subject.

## 1. The kind mix — it is the cubic, and nothing else

| kind | nodes | share | per frame | float ops each |
|---|---:|---:|---:|---:|
| **Cubic** | 138,201 | **54.8%** | **149.4** | ~14 |
| Step | 109,859 | 43.6% | 118.7 | 0 |
| Linear | 4,182 | 1.7% | 4.5 | 2 |
| Other | 0 | 0.0% | 0.0 | — |
| (None, skipped) | 29,032 | — | 31.4 | — |

**The cubic is 99.6% of the animation's float.** Step evaluates a single compare
and Linear is 1.7% of nodes. E60 measured 60,509 ticks/frame of `fadd`+`fmul`
inside `gcPlayDObjAnimJoint`; across 149.4 cubic nodes that is **405 ticks per
cubic evaluation** — 14 soft-float operations at ~29 ticks each, including call
overhead. The arithmetic model in E60 §2 holds at node granularity.

This also settles that E60's per-node reading was not an averaging artifact: had
the nodes been mostly Linear, 405 ticks each would have been impossible and the
cost would have had to be layout — which is what Tasks 95 and 96 assumed and
failed to find.

## 2. `anim_speed` — never 0, and 0.5 exists

| value | calls | share |
|---|---:|---:|
| `1.0` | 65,633 | 99.726% |
| `0.0` | **0** | 0.000% |
| `0.5` (bits `0x3F000000`) | 180 | 0.274% |

`length` is a pure accumulator of `anim_speed`, so this decides whether a pose
is indexed by an integer. It is not — but 0.5 is **dyadic**, so a half-frame
index is still exact, at 2× the entries. No other value occurs. (Note hitlag
does *not* set `anim_speed = 0`; E31 established it acts through `shuffle_tics`
and the animation-lock path instead, which is consistent with this.)

## 3. Discarded evaluations — there are none

`GOBJ_FLAG_NOANIM` skips: **0**. Every pose computed is a pose used, so there is
no free win from evaluating less. 4,271 calls (4.6/frame) are at `ANIM_END`,
which skips the `length` advance but still evaluates.

## 4. The load-time pose table is REFUTED by size

E60 proposed it as the bit-exact route. Sized with §1–§3:

```
272.7 nodes/frame / 2 anim ticks   = 136.3 nodes per anim tick
                                   ≈ 68 per fighter  ->  273 bytes per pose
80-frame animation, half-frame     = 42.6 KB per animation per fighter
63 animations reachable (Task 40)  = 2.62 MB resident
```

**DS main RAM is 4 MB and the match already occupies most of it.** Streaming the
current animation on transition instead is 42.6 KB per transition — 7–11 ms on a
DS cart, most of a frame, on a transition that happens constantly in Smash.

**Do not propose the pose table again.** It is refuted on memory and bandwidth,
not on correctness; it *would* have been bit-exact. That is exactly the check
`TASK_STANDING_RULES.md` requires before a subsystem-sized task, and it is the
check Task 78 skipped in the other direction.

## 5. What is left, priced

Every remaining route changes float results. There is no bit-exact option.

| route | ticks/frame after | **saves** |
|---|---:|---:|
| fixed-point cubic, 14 ops @ ~5 ticks | 10,458 | **50,051** |
| float Horner after per-parse pre-expansion, 6 ops @ 29 | 25,996 | 34,512 |
| **fixed-point Horner**, 6 ops @ ~4 ticks | 3,735 | **56,774** |

Pre-expansion is available because `length_invert`, `value_base`,
`value_target`, `rate_base` and `rate_target` are all constant between parse
events — the cubic is a fixed polynomial in `length`, so it can be folded to
`aL³ + bL² + cL + d` once per parse and evaluated with three multiplies and
three adds. Reassociation alone makes that inexact even in float, so it buys
nothing the fixed-point conversion does not.

## 6. Where this leaves the gate

```
gap to gate                            108,928
E32   (fighter fallback, E54)         − 51,136
fixed-point cubic (this)              ~− 50,000
                                      ─────────
                                       ~  7,800 remaining
```

**Those two levers together close the gate**, and each is blocked on a different
owner decision:

- **E32** is blocked on the hurt-flash regression. Six experiments (E48–E59)
  failed to find the mechanism and that line is closed; it is now a rendering
  question under the fidelity budget, i.e. the owner's visual approval, not a
  measurement.
- **The cubic** is blocked on the Task 9 state hash. `PROJECT_GOAL.md` requires
  mechanical equivalence and explicitly permits "fixed-point replacements" and
  "small numerical differences … when they do not materially alter gameplay";
  the hash asserts bit-exactness, which is a stronger claim than the contract
  makes. Re-bounding it is the owner's call — but it is now a *priced* call
  worth ~50,000 ticks/frame, not the unpriced "float→fixed on the collision
  path" the board carried for several cycles, which E60 showed is worth under
  4,000.

**Recommended framing for that decision:** the change is confined to
`gcGetInterpValueCubic`'s evaluation of already-parsed track state. It does not
touch parsing, collision, physics, or CPU logic. The observable difference is
sub-LSB drift in joint angles, which reaches gameplay only through
`gmCollisionGetFighterPartsWorldPosition` (E57) — so the honest acceptance test
is not the hash but a hitbox-overlap differential over a full match, which the
Task 9 fixtures can be re-bounded to express.
