# Task 96 — The AObj chain is not the cost, and the last open lever closes

**Date:** 2026-07-26
**Status:** **STOP on the wholesale animation channel rewrite.** No runtime
change. The probe is reverted.
**Inputs:** `artifacts/task96-e1-aobj.json`,
`scripts/census-aobj-chain-layout.ps1`, ROM
`9B837EB385E09B932B84F5DD567A6538740AC48DEF3FE7BC502BDE5215CC1C66`.
**Follows:** Task 95, which named this rewrite as the one remaining lever and
said it should be sized before a session is spent on it.

## 1. The premise under test

Task 92 §5 authorized a re-scoped Task 78 on one sentence:

> The animation class is 68.4% stall, so the data layout is where its
> recoverable half sits — not the arithmetic, which is frozen.

Task 95 then built the incremental version, measured the hoist working and the
frame getting worse anyway, and concluded that the lever had to be pulled
wholesale: flat contiguous channel arrays replacing the `aobj->next` walk, so
the traversal stops chasing pointers entirely.

That is a subsystem-sized task. Before spending one, it has an arithmetic
ceiling worth checking: a DS main-RAM miss is ~30–60 ticks, so the ~68,000
ticks/frame of animation-class stall needs **well over a thousand misses per
frame**. If the walk is a few dozen nodes, it cannot be that stall.

## 2. What the chain actually is

Measured in `gcPlayDObjAnimJoint` itself, so all three callers are counted
(`gcPlayAnimAll`, `ftParamUpdateAnimKeys`, `ndsBaseFTCommonGuardUpdateJoints`).
60 presented frames, 439 → 499.

| quantity | value |
|---|---|
| `AObj` size | 36 bytes |
| joint-player calls | **104.1 / frame** |
| `AObj` nodes walked | **337.8 / frame** |
| nodes per call | 3.25 |
| longest chain | 9 nodes |
| **adjacent pairs** | **0 of 15,687 — 0.0%** |
| 32-byte lines spanned | 675.6 / frame |
| …if packed flat | 415.9 / frame |
| **lines saved by flattening** | **259.7 / frame** |

The chain is exactly as scattered as Task 96 E0 suggested — 0 of 15,687
consecutive pairs are adjacent in memory, at 230× E0's sample. Flattening it
would be a real change to a genuinely fragmented structure.

**It is also worth at most 7,791–15,584 ticks/frame**, and that ceiling is
deliberately generous: it charges a full cold main-RAM miss to every one of the
259.7 avoided line spans, on a structure that is re-walked 104 times per frame
across only 338 distinct nodes and is therefore substantially dcache-resident by
construction.

Against Task 95's own bar — *stop proposing single-lever changes worth under
~20,000 ticks/frame* — the wholesale rewrite does not qualify even at its
theoretical maximum.

## 3. Where the cost actually is, and an instrument agreeing with another

`gcPlayDObjAnimJoint` costs ~99,000 ticks/frame: 33,900 own (Task 81 census
section D) plus the ~65,000 of soft-float Task 92 attributed to it. Divide by
the node count this task measured:

```
soft-float per node      64,992 / 337.8   =  192 ticks
own work per node        33,900 / 337.8   =  100 ticks
                                             ---------
                                             ~293 ticks per AObj node
```

The `nGCAnimKindCubic` Hermite is **22 `fadd`/`fmul` calls per channel** — 13
multiplies, 9 adds/subtracts — counted directly off the evaluation, which
`scripts/generate_pupupu_water_aot.py:501` transcribes operation-for-operation
from the source. So:

```
337.8 nodes x 22 calls              =  7,432 soft-float calls / frame
Task 92's attribution               = 64,992 ticks / frame
                                       ------
                                        8.75 ticks per soft-float call
```

**8.75 ticks is exactly what an ITCM leaf helper costs**, and the two
instruments — a GDB return-address sampler (Task 92) and a source-level node
counter (this task) — were built months and methods apart. They agree on the
call count to within the cost of a `bl`. That is the strongest cross-validation
the campaign has produced, and it also says the cubic branch is taken on
essentially every node: there is no dead weight in the walk to remove.

## 4. The verdict, and the sentence it overturns

Task 92 §5 said the recoverable half is layout, not arithmetic. **Measured, it
is the other way around:**

| component of the joint player | ticks/node | share |
|---|---|---|
| soft-float arithmetic | 192 | **66%** |
| everything else, including the pointer chase | 100 | 34% |
| …of which the chase is at most | 23–46 | **5–16%** |

The arithmetic is two thirds of the cost, and the arithmetic is the part Task 92
§6 closed as frozen — 73–74% of soft-float traffic sits behind the Task 9 state
hash and `PROJECT_GOAL.md`'s mechanical-equivalence contract. The layout is the
part that is legal to change, and it is the small part.

I checked whether the arithmetic has exactness-preserving slack, since bit-exact
reorganization would not be a conversion and would not be frozen. It has a
little and not enough:

- `term1`'s `fsub(f22, f20)` is the exact negation of the `fsub(f20, f22)`
  already computed for `term0`; IEEE makes that a sign-bit flip, so one call
  goes away for free.
- `length_invert * length_invert` is invariant across every frame of a key
  segment and is recomputed each frame.

Two of 22 operations, ~9%, ~6,000 ticks/frame. Below the noise floor plus the
bar, on a path that is verifier-gated to the bit.

## 5. State

`WORK-H` P95 **1,733,888** against the 1,120,000 gate: **613,888 over.**

Every direction the frame offers is now closed with a measurement, not an
estimate:

| direction | closed by |
|---|---|
| texture key memo | Task 93 — 22 distinct keys in 25 binds |
| soft-float conversion | Task 92 — 73% state-hash frozen |
| dense-vertex re-shade | Task 90 — 0.0% redundant |
| animation compiler as originally scoped | Task 77 E1 — cosmetic joint set empty |
| `mem*` micro-fixes | Tasks 87, 88 |
| placement | Tasks 87, 88, 89, 94 |
| incremental animation reorganization | Task 95 — both arms |
| **wholesale animation channel rewrite** | **Task 96 — ceiling 7,791–15,584** |

Task 92 §6 wrote that "no class in the frame above 120,000 ticks is actionable
under the current contracts" and named animation as the one open door. This task
closes that door. **There is now no remaining lever above 20,000 ticks/frame
that the current contracts permit.**

That is a decision point for the owner, not another experiment. The gate is
613,888 ticks from being met and the ways to move it are all outside what the
contracts currently allow — which means the next move is either relaxing one of
them (the sacrifice order in `PROJECT_GOAL.md` names audio fidelity, then visual
fidelity, then 60 Hz simulation, then gameplay fidelity) or accepting the
current frame rate. I am not making that call, and this document does not claim
a third option exists.

## 6. Method notes

The E0 attempt at this measurement was discarded for comparing distinct
*starting* cache lines against total *spanned* lines (Task 95 §6). Both
corrections it demanded are in this run, and the fix is visible in the data:
`LineSpans` came back as exactly 2 × `Nodes`, because a 36-byte object at 4-byte
alignment straddles two 32-byte lines every single time. E0's counter could
never have seen that, which is precisely why its ratio came out below 1.0 and
implied that scattering beats packing.

One script bug worth recording: a GDB `if <cond> / continue / end` at top level
resumes **exactly once**, so a 30-frame window sampled 2 frames. It needs
`while`. The first run's per-frame rates were valid anyway — 330.0 nodes/frame
over 2 frames against 337.8 over 60 — but the window was not what was asked
for, and a two-frame window would not have survived review.

`scripts/census-aobj-chain-layout.ps1` is kept. Re-run it before anyone proposes
flattening the channel representation again; the numbers here are for the
Boundary two-fighter battle and a larger cast would move them.
