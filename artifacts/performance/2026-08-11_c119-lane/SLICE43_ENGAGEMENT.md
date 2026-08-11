# Slice 43 engagement — and the two defects only the counters could separate

Cycle 119. `builds/build-c119-gxcompose`, `NDS_R2_BOTH_CPU=1`,
`NDS_R2_FIGHTER_GX_COMPOSE=1`, 96-sample ring dump from frame 438.
**These are engagement runs, not gate readings** — a 96-frame window reads the
cheapest part of the match ([[whole-match-instrument-only]]).

## Run 2, after the fixes: every predicted count lands exactly

| counter | total | per frame | what it should be |
|---|---:|---:|---|
| `gNdsR2GxComposeRoots` | 17,120 | 32.06 | 14 Mario + 18 Fox bindings |
| `gNdsR2GxComposeCaptures` | 17,120 | 32.06 | **ratio 1.000 — every owner described** |
| `gNdsR2GxComposeDeclines` | **0** | — | was 535 of 535 |
| `gNdsR2GxComposeMults` | 30,017 | 56.2 | = `Locals`, every captured local issued |
| `gNdsR2GxComposeLocals` | 30,017 | 56.2 | CPU pass did 52.5 affine multiplies |
| `gNdsR2GxComposeRestores` | 14,980 | **28.0** | 32 − 4 root-parented (3 Mario, 1 Fox) |
| `gNdsR2GxComposeStores` | 11,235 | **21.0** | 9 Mario + 12 Fox distinct parents |

`Restores` and `Stores` landing exactly on the two numbers derived by hand from
the baked `binding_parents` tables is the part worth trusting: it says the
palette tree is being *reconstructed correctly*, not merely that the code ran.

`Mults` at 56.2 against the CPU's 52.5 is the one number above design. The extra
~4 are Mario's three root-parented bindings re-walking their shared ancestors —
the same duplication that caused defect 1. The geometry engine absorbs them, but
the FIFO cost is ~7% over the estimate.

## Run 1 caught two defects, and neither is visible without the other counter

`Roots` read **32.06/frame — full engagement by itself.** `Captures` read 9,630,
which is exactly `18 x 535`. Only Fox was ever described.

**1. Mario declined every owner, 535 of 535.** The chain store was sized at
`NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX` (27) on the reasoning that every joint
sits on exactly one binding's chain. False for Mario: its binding-parent table is
`255 0 1 2 0 0 5 6 255 8 9 255 11 12` — **three** bindings with no bound
ancestor, each walking to the DObj root, so their shared ancestors are captured
once per root chain. It reached 26 and tripped the bound. Fox's table has one
such binding and fits in 27, which is why the defect was per-fighter and why a
single "does it engage" check would have passed. Now a dedicated 48-entry array.

**2. A declining owner drew the previous owner's skeleton.** The capture is
all-or-nothing, but `BuildNativeProductionInputs` filled the root descriptors and
the root loop consumed them unconditionally, so Mario's 14 roots emitted **Fox's
joint chains** every frame while the adapter had quietly CPU-composed Mario
correctly. `root->gx_valid` now gates it and a declining owner falls back to
`ndsRendererLoadHardwareSplitMatrices`.

**The lesson is about the shape of the counter, not the count.** One counter on
the consuming side (`Roots`) reported full engagement while the producing side
had covered 56% of it. A ratio between the two sides is what made the gap
visible; either alone was consistent with success.
