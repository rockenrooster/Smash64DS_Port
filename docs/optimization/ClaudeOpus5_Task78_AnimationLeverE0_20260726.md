# Task 78 E0 — STOP. The animation path is 82,807 ticks; the target was ≥100,000

**Date:** 2026-07-26
**Status:** **STOP at E0.** The animation compiler cannot reach its stated target
because the entire path it would replace is smaller than the target. Reorder to
the texture/material task, which is roughly twice the lever and whose generated
data already exists. No runtime change.
**Input:** `artifacts/task78-anim-census/` — 128 settled frames (439–567), the
same window every Task 69–77 measurement used, per-PC ARM9 profile resolved
through DWARF.

## 1. Why this ran before any code

`COMPILER_FIRST_ARCHITECTURE.md` gives Task 78 a target of "≥100K ticks across
animation, matrices, soft-float, hierarchy, and memory stalls", and the plan
itself flags that figure as a hypothesis derived from overlapping category
totals rather than a measured budget. Task 77 E0 then recommended animation as
the next task because it was the one part of Fighter IR not already generated.

"Not yet built" is not the same as "worth building". This measures it.

## 2. The frame, measured

Window total **1,833,713 ticks/frame**, which agrees with the tick HUD's `ALL`
mean of 1,833,206 to 0.03% — the two instruments are looking at the same frames.
`armWaitForIrq` is 317,945 of that, so **work is 1,515,768**.

| family | ticks/frame | % of frame | % of work |
|---|---|---|---|
| renderer (`ndsRenderer*`) | 739,715 | 40.3% | 48.8% |
| soft-float | 161,471 | 8.8% | 10.7% |
| texture / material resolution | 159,968 | 8.7% | 10.6% |
| matrix | 158,500 | 8.6% | 10.5% |
| `mem*` | 137,193 | 7.5% | 9.1% |
| **animation** | **82,807** | **4.5%** | **5.5%** |
| joint / hierarchy transform | 1,743 | 0.1% | 0.1% |

These families overlap by construction — a renderer matrix multiply is counted
in both `renderer` and `matrix` — so they are a ranking, not a partition. That
is exactly the caveat the plan records, and it is why the animation row is the
one that settles this: it is a *disjoint* subsystem, not a cross-cutting kernel.

## 3. The animation family in full

```
 33,960  gcPlayDObjAnimJoint
 16,320  battleship_ftAnimParseDObjFigatree
  7,986  gcPlayAnimAll
  6,295  ftParamUpdateAnimKeys
  4,991  ndsBaseGcPlayMObjMatAnim
  3,289  gcParseMObjMatAnimJoint
  2,921  gcParseDObjAnimJoint
  1,552  ftMainUpdateColAnim
  1,444  ndsBaseGcPlayAnimAll
    898  gcPlayMObjMatAnim
  ...18 more symbols, 3,151 combined
 ------
 82,807  ticks/frame, 32 symbols
```

Plus 1,743 for joint hierarchy transform, which the plan counted separately and
which is negligible.

**The ceiling is 84,550 and the target is 100,000.** A *perfect* animation
compiler — one that evaluated every pose for free — would still fall 15,450
short of the number the task was written to hit. And it cannot be perfect: the
pose still has to be evaluated, matrices still have to be built, and Task 77 E1
established that no joint on either fighter may be quantized or rate-reduced,
so the cheap approximations that would have widened this are unavailable.

The honest framing is that Task 78 was scoped against a hypothesis and the
hypothesis was wrong by a factor of about 1.2 against its own ceiling — and by
more than that against any achievable fraction of it.

## 4. What the measurement points at instead

The top of the frame is not animation:

```
317,945  armWaitForIrq                    (idle, not work)
 69,882  __aeabi_fadd
 59,230  memcpy
 57,206  memset
 50,060  ndsRendererNativeShadeProductionActions
 50,030  __aeabi_fmul
 42,420  ndsRendererHardwareResolveOrBindTexture
 41,200  ndsRendererTask36ReplayRun
 35,842  ndsFighterMarioFoxDLAllDrawForSlot
 33,960  gcPlayDObjAnimJoint              <- largest animation symbol, 10th overall
```

Two soft-float opcodes alone — `__aeabi_fadd` and `__aeabi_fmul` — cost
**119,912 ticks/frame**, which is 1.45× the entire animation subsystem. `mem*`
costs 135,836. Texture and material resolution costs 159,968, with
`ndsRendererHardwareResolveOrBindTexture` alone at 42,420 — more than
`battleship_ftAnimParseDObjFigatree` and `gcPlayAnimAll` combined.

**Texture and material resolution is the next task**, for three reasons that
agree:

1. It is 159,968 ticks/frame — 1.9× the animation ceiling.
2. Its generated data already exists. Task 77 E0 found
   `sNdsNativeFighterStateDeltas[70]` and `StateSequence[196]` in the generated
   include, so the prepared bindings the plan describes are compiled already.
   The open question the plan raised — "does the runtime consult them or still
   re-resolve?" — now has a measured answer pointing at *re-resolve*.
3. It carries no fidelity risk. Material identity is exact or it is wrong;
   there is nothing to approximate and nothing to put in front of the owner.

## 5. Consequence for the roadmap

This is the second reorder from measurement in two tasks, and they point the
same way: **the renderer is where the frame is, and its generated tables are
under-used rather than absent.**

```text
plan order:        77 IR -> 78 animation -> 79 render -> 80 material
Task 77 E0 said:   IR and render already generated; animation is the gap
this measures:     the gap is small; the *use* of what is generated is the gap
```

Revised order:

1. **Prepared texture/material** (plan's Task 80) — 159,968 available, data
   already generated, no fidelity risk.
2. **Soft-float and `mem*`** — 161,471 and 137,193, cross-cutting, and the
   fixed-point conversion the animation compiler would have done for its own
   path is worth more applied where the floats actually are.
3. **Animation compiler** — 82,807, still a real number and still worth taking
   eventually, but third, and re-scoped to a target it can reach.

Task 78 is not cancelled; it is deferred and its target is withdrawn. When it
returns it should be written against 82,807 with a realistic capture fraction,
not against 100,000.

## 6. Method note

The census window, the frame count and the ROM configuration are identical to
every measurement since Task 69, and the profile total agrees with the tick HUD
to 0.03%. That agreement is the check that matters here: a subsystem census that
disagreed with the gate instrument would not be evidence about the gate.

The animation family was summed by symbol name across 32 symbols rather than by
the census's own subsystem rules, because `SUBSYSTEM_RULES` maps all of
`decomp/.../src/ft` to one `SIM/fighter` bucket and would have hidden animation
inside fighter physics, collision and state machines. That bucket is not wrong,
it is just too coarse to answer this question — which is worth remembering the
next time a subsystem table is used to size a lever.
