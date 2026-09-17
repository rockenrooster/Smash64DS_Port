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

## Per-candidate evidence (being gathered separately)

Per-candidate sizing is a second pass and is not in this artifact yet. The key
discipline for it is the project's own:
a lane that was accepted at a small roster and then contradicted at four
fighters is **dead until re-derived**, not merely unsized — `An accepted bank
may not hold at 4x` records GX compose accepted at −8,096 on Mario+Fox and then
measured at **+22,848 P50 / +67,456 P95 at four fighters**.

## What this artifact does NOT do

It does not estimate a tick saving for any candidate. Every document above
declined to benchmark, and inventing a figure they did not measure would be the
same error the residual ledger was written to correct.
