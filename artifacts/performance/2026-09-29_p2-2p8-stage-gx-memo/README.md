# P2-2p8: stage GX words that outlive the frame (2026-09-29)

`src/nds/nds_stage_gx.exec.inc`. The compiled stage program's words are a
persistent buffer that only the per-frame patches write, and two patch kinds
were rewritten every frame from inputs that almost never move:

- MATERIAL (three words a run): the run's polygon format, texture name and
  parameters, resolved through libnds's texture and palette objects. Only a
  texture-cache event moves those objects, and the prepared table's own proof
  already keys on that event (`sNdsRendererHardwareTextureKeyGeneration`).
  Dream Land: 53 a frame at ~130 ticks each (6.9K a frame in the gate profile).
- NOZ and PROJECTION (sixteen words a patch): the camera projection, constant
  within a match unless the FOV moves. The painter pass still rewrites the
  per-triangle depth column every frame.

Each run now keeps a record (heap, allocated with the program body) of what
its words hold: the material inputs with the key generation, and a
projection serial that advances whenever the frame's projection differs from
the last one written (one 64-byte compare per stage-matrix generation). A run
that returns early keeps its old record, so a frame never vouches for words it
did not write. To fit the ITCM, the draw's cold paths moved to main RAM (the
near-boundary refresh, source cross corners, the read-back painter pass, the
PROJECTION patch) and `ndsBaseMPProcessRunFloorEdgeAdjust` (48 B, ~76 ticks a
frame) left the hot text list.

## Measured (gate ROM, same-ROM A/B word `gNdsStageGxMemo`, `memo-ab.txt`)

| arm | P50 | P95 | STG median | two-VBlank |
|---|---:|---:|---:|---:|
| `sm0a` (0) | 927,744 | 1,259,712 | 168,320 | 84.4% |
| `sm0b` (0) | 927,104 | 1,259,712 | 168,320 | 84.1% |
| `sm1a` (1) | **922,368** | **1,254,400** | **163,648** | 85.0% |
| `sm1b` (1) | **922,624** | **1,256,128** | **163,648** | 85.0% |

STG median -4.7K, WORK-H P50 -5.0K, P95 -4.4K; replay IDENTICAL (both pairs).

## Exactness (`memo-verify.txt`)

Mode 2 (lab) lets the memo decide and compares every word it keeps with the
word the patch would have written. Over a full match on all nine VS stages
(lab ROM stages 0-5, 7, 8; the gate ROM for Dream Land): 235,341-452,991
material words and, where static no-Z/projection patches exist (Dream Land,
Yoshi's Island), 1,750,128 and 2,326,960 projection words were kept; **0
differ**. No declines.
</content>
</invoke>
