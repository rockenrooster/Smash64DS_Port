# Dream Land stage owners priced, 2026-10-04 (owner ruling D12c)

## Why

Owner ruling D12 approved stage background LOD for the four-CPU gate. Before
designing one, each Dream Land stage owner's whole draw was priced by not
drawing it: an upper bound for any cheaper version of that layer.

## Method

Gate target built with the render economy compiled in
(`NDS_RENDER_ECONOMY=1`, build `build-gate-econ`; measurement only, not a
shipping configuration). The frame-begin activation of the economy mask
needs telemetry, so the runs poke the active mask directly at the first
frame-complete marker: `-SetGlobals 'gNdsRendererEconomyActiveOwnerMask=M'`
(bit n = stage owner n not committed). Same ROM for every arm, 1,960
presented frames each.

## Result

| mask | owners skipped | WORK P50 | WORK P95 | > 1.12M | dP50 | dP95 |
|---:|---|---:|---:|---:|---:|---:|
| 0 | none (control) | 896,448 | 1,232,960 | 234 | | |
| 1 | layer 0 (far background, 54 no-Z triangles) | 875,584 | 1,211,392 | 204 | -20,864 | -21,568 |
| 2 | main platform | 890,880 | 1,231,872 | 223 | -5,568 | -1,088 |
| 4 | layer 2 | 889,664 | 1,225,920 | 222 | -6,784 | -7,040 |
| 8 | layer 3 | 890,240 | 1,226,240 | 223 | -6,208 | -6,720 |
| 48 | Whispy (owners 4, 5) | 890,368 | 1,229,568 | 224 | -6,080 | -3,392 |
| 192 | flowers (owners 6, 7) | 880,384 | 1,216,960 | 209 | -16,064 | -16,000 |

## Reading

The painter-heavy owners (layer 0, the flowers) cost the most per triangle:
each no-Z triangle carried its own 16-word projection load and a per-frame
depth patch. That led to the exact cut in `2026-10-04_painter-trans`
(one matrix a no-Z run, depth stepped by MTX_TRANS), not to an
approximation; no stage LOD has been shipped. The flowers sway with
Whispy's wind, so freezing them would be visible.
