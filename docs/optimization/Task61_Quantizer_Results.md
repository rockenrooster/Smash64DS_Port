# Task 61 — Coordinate Quantization + Vertex Opcode Selection: Results

Host-only. After Task 60's vertex-count cut, shrinks the GX command stream by
choosing the cheapest legal DS vertex opcode per vertex. No runtime changes.

## Deliverables

- `scripts/dreamland_quantizer.py` — coordinate-basis search + per-vertex opcode
  selector + decode round-trip checker + `--check`.
- `scripts/generated/encoded_streams/*.json` — per-candidate chosen-policy IR
  (Task 62's runtime input: opcode mix + VERTEX10 rebasis scale).
- `scripts/generated/dreamland_quantization_report.json` — GX-word census +
  oracle-IoU verdict per policy + recommendation.

## DS vertex opcodes (GBATEK / libnds videoGL.h)

| opcode | id | words | range | notes |
|--------|----|-------|-------|-------|
| VERTEX16   | 0x23 | 2 | s16.12, ±32768 | current path (lossless for stage) |
| VERTEX10   | 0x2D | 1 | s10.3, ±256 | needs rebasis; 0.125-step quant |
| VERTEX_XY  | 0x21 | 1 | reuse last Z | exact, zero error |
| VERTEX_XZ  | 0x22 | 1 | reuse last Y | exact, zero error |
| VERTEX_YZ  | 0x24 | 1 | reuse last X | exact, zero error |
| VERTEX_DIFF| 0x25 | 1 | s5.6 deltas, ±32 | **infeasible** (stage deltas reach 4638) |

## Selection policy (per vertex, in emission order)

1. **VERTEX_XZ/XY/YZ** (1 word, EXACT) where an axis matches the prior decoded
   vertex — pure win, zero precision loss. Feasible on ~20% of consecutive
   vertex pairs (93/190 Y-matches on c120, etc.).
2. **VERTEX10** (1 word) where the rebased coordinate fits s10.3 — the rebasis
   scales the whole candidate uniformly (scale s, origin o) so every vertex
   `(v−o)/s` fits, with a compensating hardware matrix restoring world shape at
   runtime. Precision trade judged by the oracle.
3. **VERTEX16** (2 words) fallback.

VERTEX_DIFF was measured and rejected (0% feasibility — Dream Land's geometry
has large inter-vertex jumps).

## Result — c120 (the recommended candidate)

Task 60's c120 (261 submitted verts, IoU 0.959) encoded:

```text
policy                  vertex words   reduction   worst-case IoU
VERTEX16 baseline         522            0%         0.9593
VERTEX16 + axis-reuse     396           24%         0.9593  (loss 0.000000)
VERTEX10 + axis-reuse     261           50%         0.9581  (loss 0.001200)
```

- **Axis-reuse is exact**: IoU loss 0.000000 (the reused axis is decoded
  verbatim from the prior vertex).
- **VERTEX10 quantization is essentially invisible**: worst-case 3.164 world
  units of position error per vertex (rebasis scale 50.86), which at the
  gameplay camera distance is sub-pixel — IoU drops only 0.0012 (0.959→0.958).
- Opcode mix: 256 VERTEX10 + 2 VERTEX_XZ + 3 VERTEX_YZ, **0 VERTEX16**.

## VERDICT — PROCEED gate met

The plan's PROCEED gate: *"Keep only encodings that materially reduce GX words
with no visible regression."* **Met.** VERTEX10+axis-reuse halves the per-
vertex word cost (522→261 = **50% vertex-word reduction**) with a 0.0012 IoU
drift — no visible regression.

## Full ladder (recommended policy per candidate)

```text
candidate     src_words  v16+ax  v10+ax  policy   IoU (chosen)
c120              522      396     261    V10      0.958   <- recommended
c140              602      455     301    V10      0.983
r_keep3           674      504     337    V10      0.983
source_welded     754      548     377    V10      0.997
r_weld            762      567     381    V10      0.997
```

VERTEX10 wins on every visually-acceptable candidate; the lower-IoU candidates
(c90/c110, IoU < 0.95) keep VERTEX16+axis-reuse since VERTEX10 can't rescue
their already-broken silhouette.

## Checker (all gates pass)

`python scripts/dreamland_quantizer.py --check`:
1. **Determinism** — rebuilt report sha256-matches the stored report.
2. **Decode round-trip** — axis-reuse opcodes reconstruct exactly (zero error
   by construction); VERTEX10 quantization error is bounded (no overflow).
3. **Overflow guard** — every VERTEX10 quantized value stays in s10.3 range.

Mutation-tested: falsifying c120's `v10_axis_iou` in the stored report is
caught (determinism drift).

## Combined pipeline result (Tasks 57→61)

Stacking every host-side reduction on the recommended candidate:

```text
stage                          submitted verts   vertex GX words
source (Task 57)                     525*             525*
+ Task 59 geometry (c120)            261              522   (50% vert cut @ IoU 0.959)
+ Task 60 strip/quad (c120)          261              522   (topology reorder, no IoU cost)
+ Task 61 VERTEX10+axis (c120)       261              261   (50% word cut, IoU 0.958)
```
\* source = 525 corners (GL_TRIANGLES); the welded dense-vertex count is 255.

End to end, the Dream Land static stage goes from **525 vertex GX words to 261**
(**50.3% reduction**) at **IoU 0.958** (visually acceptable). The reductions
compose cleanly: geometry (Task 59) cuts the triangle count, stripification
(Task 60) reorders for vertex sharing, and opcode selection (Task 61) halves
the word cost per vertex.

## Notes for Task 62 (runtime)

- **c120 + VERTEX10+axis-reuse** is the candidate to integrate. Runtime needs:
  - new write paths for VERTEX10 / VERTEX_XZ / VERTEX_YZ (the current path
    only emits VERTEX16);
  - the VERTEX10 rebasis as a compensating hardware matrix (scale 50.86,
    origin at the candidate's position-space midpoint) loaded once per owner;
  - per-vertex opcode selection in emission order (axis-reuse depends on the
    prior decoded vertex, so it must follow the strip/quad group order).
- The encoded-stream IR at `scripts/generated/encoded_streams/c120.json`
  carries the chosen policy, rebasis scale, and opcode counts.
- VERTEX16 stays as the fallback for any future stage whose geometry doesn't
  fit s10.3 even after rebasis.
