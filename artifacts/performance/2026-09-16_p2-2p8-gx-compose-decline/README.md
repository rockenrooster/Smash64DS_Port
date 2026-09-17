# P2-2p8 N05.01: the GX hierarchy compose decline is accidental — and beneficial

Verdict: **the decline stays.** Repairing it is a measured 22,848-tick regression
at P50 and 67,456 at P95 on the four-fighter roster. The accepted bank behind it
does not transfer from two fighters to four.

## What was found

`NDS_R2_FIGHTER_HW_MTX` and `NDS_R2_FIGHTER_GX_COMPOSE` are both `1` in the
shipping four-CPU config, and both are owner-accepted — E17 on 2026-07-28
(-17,600 FTR P50) and Slice 43 on 2026-08-15 (net WORK-H P50 **-8,096**,
rank-80 -17,152, measured on the **Mario+Fox** roster).

They are not running. Entry-PC counts from the current profile:

| symbol | calls/frame |
|---|---|
| `ndsRendererLoadHardwareGxComposedMatrices` | **0** |
| `ndsFighterPacketLoadGxComposedRecord` | **0** |
| `ndsRendererHardwareFighterMultMatrix4x3` | **0** |
| `ndsRendererAdapterComposeOwnerWorldsFlat` (CPU fallback) | 2.92 |
| `ndsRendererAdapterComposeOwnerWorldsSource` (CPU seam) | 0.99 |

2.92 + 0.99 = 3.91 = every fighter draw. `ndsRendererMtxMulAffine20p12` runs
**73.15 calls/frame**, of which 71.57 are the two call sites inside
`ComposeOwnerWorldsFlat`. In August, with GX compose live, it was **1.19
calls/frame**.

## Cause

`ndsRendererAdapterBuildGxSlotTable` (`src/port/renderer_adapter_matrix.c:6008`)
reserves the union of every owner's cross-run palette slots:

```c
cross = ndsRendererNativeFighterCrossPaletteSlots(owner_slot, &cross_count);
if (cross == NULL) { return FALSE; }
```

`ndsRendererNativeFighterCrossPaletteSlots` guards each owner with
`#if NDS_P2_<OWNER>` and falls through to `NULL` for any owner compiled out.
This build has `NDS_P2_KIRBY 1`, which puts
`NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT` at 12 and so walks slots 7-10 —
Pikachu, Yoshi, Ness, Purin — none of which are built. First hole returns `NULL`,
the table build returns `FALSE`, `CaptureOwnerChainsGx` returns `FALSE`, and
control reaches `goto gx_compose_declined`. Every owner, every frame.

`nds_renderer.h:621` anticipated exactly this shape — "A sparse build can admit
Kirby without Pikachu/Yoshi/Ness/Purin" — but the union loop does not tolerate
the holes. It went unnoticed because the roster that accepted the bank was
Mario+Fox, which has none, and because **`gNdsR2GxComposeDeclines` exists but no
verifier asserts on it**.

## The repair, and why it is not kept

The fix is three lines: a `NULL` from an owner that is not in this build means
"no slots to reserve", so `continue`; a `NULL` for the owner being built for is
still fatal. Applied, it works — `gNdsR2GxComposeDeclines` falls from ~3.91 per
frame to **31 over 300 frames**, so GX compose engages for essentially every
draw.

Measured on the canonical directory, 1,972 samples, against the current
checkpoint:

| bucket | declined (current) | GX compose engaged | delta |
|---|---|---|---|
| **WORK-H P50** | 1,584,128 | 1,606,976 | **+22,848** |
| **WORK-H P95** | 2,310,848 | 2,378,304 | **+67,456** |
| **FTR P50** | 356,608 | 369,856 | **+13,248** |
| **FTR P95** | 740,352 | 824,832 | **+84,480** |
| STG P50 | 339,968 | 339,840 | -128 |

Native failures/direct rejects stayed 0/0, heap low-water 111,680 B, draw-plan
hit 6,217 and the N04.08 skip count 7,892 were unchanged, so this is a pure cost
comparison of the two matrix paths on the same workload.

**The CPU multiply it deletes is cheaper than the FIFO traffic it adds, at four
fighters.** That is consistent with the bank's own accounting rather than a
contradiction of it: Slice 43 measured the CPU deletion at -18,165 and the net at
only -8,096, "the FIFO side ate over half". Doubling the roster doubles the
`MATRIX_STORE`/`MATRIX_RESTORE`/`MULT4x4` stream while the CPU multiply it
replaces scales the same way, and past the crossover the net flips sign. FTR P95
+84,480 is where it shows.

**The 2026-08-15 acceptance figure does not describe a four-fighter roster.** It
should not be quoted for one.

## What this means for the lane

The whole "use the DS matrix stack" candidate is spent. It was sized at 160,576
tk/fr from symbol totals; the part that is actually a CPU product the GPU could
do instead is ~24,861, the mechanism for doing it already shipped, and turning it
on costs more than it saves at this roster size. The rest of the 160,576 is local
matrix construction and S16.16 decode that the GX arm performs too, a cached
validity walk, a deliberately CPU-routed fidelity seam, and the falsified
`ApplyMvpRecalc` path.

## Owed follow-up, not done here

The decline is currently accidental. It should become deliberate — the code
should choose the CPU path knowingly on a roster where it is faster, rather than
falling into it through a `NULL` in a union loop — and `gNdsR2GxComposeDeclines`
should be asserted by the four-CPU gate so the choice cannot flip silently in
either direction again. Until that lands, note that "fixing" the sparse-roster
`NULL` handling in isolation regresses WORK-H P50 by 22,848.
