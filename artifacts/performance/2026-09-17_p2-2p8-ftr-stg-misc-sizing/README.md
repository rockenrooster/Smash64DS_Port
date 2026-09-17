# Sizing FTR / STG / MISC against the gate they are meant to close

The board records the owner's 2026-09-16 ruling on `docs/optimization/*` as
**SRC NO-GO; FTR/STG/MISC UNSIZED**. This sizes them. No implementation.

## What the three documents are, and what they are not

All three say so themselves:

- `FTR.md`: *"The performance figures below come from the repository's recorded
  measurements; I did not build or benchmark a new ROM in this review."*
- `STG.md`: *"These are source-grounded candidates, not newly measured speedups.
  I did not build or run a ROM."*
- `MISC.md`: *"I have not built or benchmarked these new candidates. Their tick
  savings still need controlled measurements."*

So every number in them is **inherited**, and several are already stale in our
favour: FTR cites a checkpoint at WORK-H P50 1,584,128, STG the same, MISC
1,654,208, and the README 1,680,384. The current qualified figure is
**1,600,960**.

They are candidate lists, not sized levers. That is exactly what "UNSIZED"
means, and it is not a criticism of the documents.

## The ceiling that sizes all three at once

From the last Boundary-GREEN four-fighter run (`a4eb24c9a85`, three arms, native
failures 0, slips 0):

```
ALL     1,677,952  2,798,144   100.0
FTR       350,144    736,960    20.9
STG       385,088    427,648    22.9
SRC       543,040  1,027,520    32.4
MISC      238,720    464,000    14.2
OTHR      284,096    560,384    16.9
WAIT      256,512    532,224    15.3
HUD        20,160    455,168     1.2
AUD         3,712    122,240     0.2
WORK-H  1,600,960  2,320,576    95.4
```

Gate is 1,120,000. **WORK-H P50 1,600,960 means the gap is 480,960 ticks.**

| lane | P50 | entire lane vs the gap |
|---|---|---|
| SRC | 543,040 | **112.9%** — the only lane large enough, and **owner NO-GO** |
| STG | 385,088 | 80.1% |
| FTR | 350,144 | 72.8% |
| MISC | 238,720 | 49.6% |

**Not one of FTR, STG or MISC can close the gate even if its entire subsystem
were deleted.** Deleting all of STG still leaves 95,872 over; all of FTR leaves
130,816 over; all of MISC leaves 242,240 over.

**And they cannot be added.** The lane percentages sum to 124% of ALL, so the
lanes are not disjoint — a saving attributed to two lanes is not two savings.
Treat each figure strictly as an upper bound on deleting that one lane, never as
a term in a sum. (`self-time-is-not-a-subsystem-budget` is the standing rule
here; it has already killed one lever on this project.)

## What that implies, stated plainly

1. **No single document closes the gate.** Any plan that reaches 1,120,000 needs
   a large fraction of *two* of these three, or a reversal of the SRC NO-GO.
2. **A realistic FTR/STG/MISC campaign is a partial-credit exercise.** Useful —
   the residual ledger still has 321,866 unfound ticks — but it should be
   proposed as partial credit, not as the path to 30 FPS.
3. **The SRC ruling is the highest-leverage open decision on the project.** SRC
   is the only lane whose entire size exceeds the gap. That is an owner call
   and it is already recorded as NO-GO; nothing here reopens it. It is worth
   knowing that it is the only single-lane path.

## Per-candidate evidence: FTR's six, against what the repo already measured

The discipline: a lane accepted at a small roster and then contradicted at four
fighters is **dead until re-derived**, not merely unsized.

| # | FTR candidate | verdict | evidence |
|---|---|---|---|
| 2 | Direct pose-to-packet matrix production | **DEAD** | The repo **built and measured exactly this**. `…/2026-09-15_p2-2p8-pose-draw-pilot/` (N05.05), same ROM, route bit only: **FTR median +14,336, lost all 128 frames**; WORK median +12,864, lost 104/128. It engaged (10,554 native-transform draws) and died on 48,720 misses / 12,861 stale. **FTR.md does not cite it.** Serialization is separately exhausted across four arms N04.04–N04.07, all under the 14,080 floor |
| 3 | Lossless GX packet compilation | **DEAD** | Task 55 delivered it losslessly — replay buffer 3,916 → 3,561 words, **−9.1%** — and **`ALL` moved +64, flat**. Per-PC says why: *"~28 of the 40.5 cycles are the GX writes… the stall is **per vertex**, not per word"*, which also refutes `VTX_10`. GX compose is the same lane at four fighters: **+22,848 P50 / +67,456 P95** |
| 1 | Prebound fighter submission | **SIZED, small** | Ceiling **19,300 tk/fr** (`HANDOFF_P1_FINAL.md:12`, corrected down from 34,307), and its two largest sub-deletions are **already banked**: −27,264/−22,912 (prechecked replay) and −5,504/−2,816 (packet input refresh). The residue is precheck cost ≈23.6K. Carries a hard invalidator: *"REFUTED, TWICE OVER — DO NOT BUILD A DObj-TREE-KEYED MEMO"*; the obvious content key read *unchanged* on 49 of the 51 frames the contract actually changed |
| 5 | Direct-index Link texgen | **SIZED, smallest** | Ceiling is a fraction of **8,053 tk/fr**, and FTR says so itself. Premise verified true verbatim in current code. Inherits the `n0503-flat-cache` hazard, where a lookup-miss repair worked as predicted (SRC −15,040) and was cancelled by dcache eviction (+51,520 STG) — *"fetch, not arithmetic"* |
| 6 | Versioned lighting/tint | **BELOW FLOOR** | Already half-built: `ndsFighterPacketApplyTint` early-returns on unchanged modulate+hash. What remains is one hash loop and one sqrt + three divides per replay — under the 14,080 cross-build floor on this repo's own precedent |
| 4 | Root-local variants | **GENUINELY UNSIZED, and discouraging** | The miss population is captured but never correlated with expensive frames. What the captures show: **root-count and texture-residency misses read zero in every four-CPU capture**, and the miss rate is ~2.6% (178 records vs 6,673 hits), so the whole-fighter-rebuild population may be small. Against it stands the hardest constraint on the board — see below |

### Correction to the evidence pass itself

An earlier draft of this section listed `ndsFighterDisplayContractCountFlags` as
**4,117 tk/fr of free deletion** — a recursive DObj walk whose only outputs are
two debug counters nothing in Boundary reads. **That is not available.** The
call site is inside `#if NDS_R2_FTR_CONTRACT_CENSUS`
(`renderer_adapter_fighter.c:937-939`), and that flag is **0 in both the
four-CPU gate build and the profile build**
(`builds/build-p2-fourcpu-tickhud/nds_build_config.h:204`,
`builds/build-p2p8-n0409-profile/nds_build_config.h:198`). The cost was
recorded when the census was on; it is already not paid. The repo's own comment
says so and the draft read past it.

### What candidate 4 collides with

Root programs shipped today, but they are **whole-owner alternate root vectors**
matched by full-vector scan, so a program switch changes `preamble_hash` and
`input_count` and invalidates the entire packet — precisely what candidate 4
says should stop. The fragment-level assembly it asks for does not exist.

And a variant bank is resident growth by construction, which is the one thing
the four-CPU build cannot absorb: ~1 KB cost 14 native failures and a 4,096-byte
arena step-down today (`…/2026-09-17_p2-3f53-yoshi-egg-owner/`), and at scale
**+28,848 B cost WORK-H P50 +70,016 with P0/P1 triangle counts identical**
(`…/2026-09-17_p2-3f47-kirby-copy-hats/`) — carrying cost, not draw cost.

## What this artifact does NOT do

It does not estimate a tick saving for any candidate. Every document above
declined to benchmark, and inventing a figure they did not measure would be the
same error the residual ledger was written to correct.
