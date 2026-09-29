# P2-2p8: object draws priced; the stage-DL fast lane (2026-09-29)

Solo, no subagents. Gate = the four-CPU tick-HUD ROM on Dream Land; lab = the
NDS_LAB_FOURCPU_SWEEP ROM (`gNdsLabFourCpuGkind`: 1 Sector Z, 5 Yoshi's
Island, 6 Dream Land, 7 Saffron).

## Where MISC's tail is

Gate rows (`iw_gate`, HEAD `5637a3d495f`): weapons are drawn in 29% of frames at
~46K ticks when present, items in 25% at ~53K, effect models in 21% at ~61K;
the P92-98 band carries +26K MWPN, +25.5K MITM and +20K MEFX over the median.
A draw's cost hardly depends on its size: 479 item submits drew 6,131
triangles (12.8 each) for 25.5M ticks, 524 weapon submits 1,627 triangles
(3.1 each) for 25.6M.

The lab stage-DL census (`stage-dl-census.txt`; effect lists keyed by source
asset in this receipt's lab build) puts the generic stage-DL submit at 36K
ticks a frame on Dream Land, 72K on Yoshi's Island, 88K on Saffron and 117K on
Sector Z, 22K-78K a call. The largest rows: Dream Land's Charge Shot (467
calls, 42K each: owner search 4.3K, matrices 15.2K of which the kind-46
billboard rows 8.7K, owner 20K) and Beam Sword (526 calls, 29K); Saffron's
Beam Sword (2,356 calls, 41K a frame); Yoshi's Island's Capsule (1,689 calls)
and Box; Sector Z's Arwing (7,432 entry draws) and item bumper.

A fresh whole-match gate profile (`builds/p2p8-prof-gate5`, with D-cache fill
census) agrees and shows no hot spot to fix inside the body: its 4.4K ticks a
frame of self time spread over hundreds of candidate tests in a 20 KB Thumb
function with a 4 KB frame; the entry-model scan in front of it is another
1.5K. D-cache fills: 10,527 a frame (12,907 on 09-27), 57% in heap objects.

## Banked: the fast lane (`src/port/renderer_adapter_stage.c`)

When the general body's owner draws a Charge Shot or a Beam Sword list, it
records the route (loaded file, asset, data, root) for that list; a later
draw of the same list skips the entry scan and the body and calls the same
owner with the same inputs (PrepareInitialMatrices, config, persistent stats,
item colours) and the same stats tail. Live admission facts (GObj kind, item
kind, NULL MObj, submit context, no oracle) are tested every draw; anything
else goes to the body as before. Same-ROM A/B word `gNdsStageDLFastLane`
(`fastlane-ab.txt`):

| arm | P50 | P95 | P99 | P90-98 mean | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `fl0a` (0) | 926,528 | 1,256,832 | 1,466,880 | 1,244,089 | 84.3% |
| `fl0b` (0) | 925,696 | 1,256,832 | 1,466,496 | 1,244,582 | 84.3% |
| `fl1a` (1) | **921,216** | 1,255,488 | **1,458,624** | **1,238,096** | 85.3% |
| `fl1b` (1) | **921,216** | 1,256,192 | **1,458,624** | **1,238,317** | 85.3% |

990 fast draws a match; item draw ticks 26.1M -> 22.1M (-8.4K a submit),
weapon 25.9M -> 22.7M (-6.7K a Charge Shot); triangle counts identical, 0
native failures, replay IDENTICAL (both pairs).

## Next

The routed owners still pay their matrices (Charge Shot's float billboard
rows 8.7K, a held Sword's attach chain 5-10K) and their owner body (texture
resolve 3.6K a Charge Shot, the generated N64 state replay, the immediate
vertex loop). More routes: Taru, Box, Capsule, BombHei, the Pokemon.
