# The banked "canonical both-CPU gate" is a SINGLE-CPU measurement

`NDS_R2_BOTH_CPU` is a **make variable**, so which arm a ROM runs is fixed at
build time and cannot be selected by the harness. Reading the generated config
header out of each build directory settles what each figure actually measured:

```
build-c124-slice48   NDS_R2_BOTH_CPU 0     <- the ROM behind the banked figure
build-c126-modelpart NDS_R2_BOTH_CPU 0
build-c126-bothcpu   NDS_R2_BOTH_CPU 1
```

`build-c124-slice48` is the ROM `EXHAUSTION.md` and `HANDOFF.md` both describe as
the *"canonical both-CPU gate"*. It was built `BOTH_CPU 0`. **It cannot have run
the both-CPU arm.**

## Measured, same instrument, same day, identical configuration

All runs `-Samples 1600 -RingDump`, DLDI ON, whole match, `slips=0`.

| build | `BOTH_CPU` | `WORK-H` P50 | `WORK-H` P95 | vs 1,120,380 gate |
|---|---:|---:|---:|---|
| `build-c124-slice48` (the bank) | 0 | 899,136 | **1,087,616** | **−32,764 UNDER** |
| `build-c126-modelpart` | 0 | 893,568 | 1,088,640 | −31,740 under |
| `build-c126-bothcpu` (HEAD) | 1 | 938,368 | **1,207,616** | **+87,236 OVER** |

Two independent `BOTH_CPU 0` builds, five commits apart, agree to ~1,000. Three
`BOTH_CPU 1` runs land 1,197,952 / 1,207,168 / 1,207,616. **The ~120,000 spread
is the ARM, not placement and not the changes in between** — which is exactly
what the control below was run to establish.

## The control

Re-measuring the bank's own ROM today returns **1,087,616** against its recorded
1,087,296. The environment has not drifted, the ROM still reproduces its figure,
and therefore the gap to the both-CPU builds is not instrument drift.

This matters because `small-load-frame-cuts-cannot-be-banked` and slice 48 make
placement the default suspect for any cross-build delta on this target. It is
not the explanation here: placement does not sort three builds perfectly by a
build flag while leaving two builds five commits apart within 1,024 of each
other.

## Why this is the gate and not a footnote

`Smash64DS_Runtime2_SwitchPlan.md` R2-07 names the arm in the gate text itself:

> Gate: full demo loop (**Mario CPU vs Fox CPU**, 1-minute and 5-minute match
> lengths) within total budget; battle P95 still ≤ 1.12M DLDI-on.

and again in the STRESS TEST gate:

> **Mario CPU vs Fox CPU** on Dream Land, Full Match … must not exceed 1.12M P95

Mario CPU vs Fox CPU **is** `NDS_R2_BOTH_CPU=1`. `AGENTS.md` says the same thing
from the other side: both gate arms run the one-minute match, and the both-CPU
arm is the stress arm that is *never published as the Boundary figure*.

The `BOTH_CPU 0` arm is Boundary's shipped configuration — Mario as a **human
player receiving no input**, standing still for the whole match. It is a real
and required measurement, and it is comfortably under gate. It is simply not the
arm R2-07 gates on, and it is cheaper precisely because one of the two fighters
does nothing.

## Status this establishes

- **Boundary (mode 163, `BOTH_CPU 0`): PASSES** at ~1,087,600.
- **R2-07's stress gate (`BOTH_CPU 1`): FAILS** at ~1,207,616, **+87,236 over**.

The campaign's headline "the gate is met, not stably met" was measuring the
wrong arm. The corrected reading is that one arm passes and the arm R2-07
specifies does not, by a margin far larger than the placement noise that
qualified the original claim.

## What this does NOT say

- It does not say slices 45/46/48 were wrong. They were measured as same-binary
  or matched-arm comparisons and their *deltas* stand; only the absolute figure
  they were banked against is mislabelled.
- It does not say the changes in `build-c126-bothcpu` cost anything. The
  `BOTH_CPU 0` build at the same commit reads 1,088,640, within ~1,000 of the
  bank five commits earlier.
- It does not re-open the 5-minute or Sudden-Death legs of the gate, which
  remain unmeasured on either arm.

## Next

Re-bank the gate on `BOTH_CPU 1` and re-run attribution there. Every lane
ceiling in `EXHAUSTION.md` was computed on the `BOTH_CPU 0` rows, so the
lane sizes that closed `FTR`, `STG` and the rest are sized against the wrong
distribution and have to be recomputed before any of them is trusted as "dead".
