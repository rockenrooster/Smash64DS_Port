# Task 57 — Dream Land Canonical World-Space Mesh: Results

Host-only. Builds the canonical world-space visual mesh IR from the already-validated
stage extraction. No runtime rendering. No gameplay collision change. This is the input
to Tasks 58 (gameplay-camera error oracle) and 59 (constrained simplifier).

## Deliverables

- `scripts/dreamland_world_mesh.py` — host-side world-mesh compiler + checker.
  Consumes `generate_nds_native_stage.generate()` (no decoding duplication);
  bakes static vertices to world space; emits the IR; `--check` rebuilds and
  validates it deterministically (same contract as the stage generator).
- `scripts/generated/dreamland_world_mesh.json` — deterministic IR (regenerated
  artifact; not hand-edited).

## Method

- **Reuse, don't reparse.** The compiler imports the existing extraction
  pipeline (`generate_nds_native_stage.generate()`) and reads its validated
  `Packet` (bindings, runs, corners, `DenseVertex`, baked world matrices).
- **Static / dynamic partition** is by the authoritative `OwnerSpec.resource_name`
  field the generator already records per owner: `stage_geometry` (owners
  `layer0..layer3`) = static fighting stage; `stage_actors` (owners `map0..map3`)
  = dynamic actors (Whispy face/animated parts, flowers), excluded and kept
  separately identifiable. This is a direct attribute, not an inference.
- **World-space bake** uses the exact integer arithmetic of the runtime
  `ndsRendererTransformVertex20p12` (`src/nds/nds_renderer.c:4834`):

      world.x = clamp_s32( m[0][0]*x + m[1][0]*y + m[2][0]*z + m[3][0] )

  Local coords are raw s16 integers (as the runtime feeds `NDSRendererInputVertex`);
  the matrix is the generator's baked s20.12 world table. Result is s20.12,
  bit-identical to the runtime transform. The checker re-runs the transform and
  compares against the stored IR for every static vertex.
- **Triangle index space** is the contiguous static mesh (0..N-1); each vertex
  carries `source_dense_index` provenance back to the generator's dense space.

## Census (exact counts)

```text
SOURCE (all Dream Land bindings)
  source_triangles_total            202
  source_corners_total              606   <- the cited 606 stage submissions
  source_dense_vertices_total       312

STATIC / DYNAMIC PARTITION (by resource_name)
  static_binding_count               27   (stage_geometry: layer0..layer3)
  dynamic_binding_count              15   (stage_actors:   map0..map3)
  static_triangles_source           175
  dynamic_triangles_source           27   <- excluded from the static mesh
  static_dense_vertices_source      255
  dynamic_dense_vertices_source      57

BAKED STATIC MESH
  baked_static_triangles            175
  baked_static_corners              525
  baked_static_dense_vertices       255
  unique_exact_positions            182   <- 73 shared positions across dense verts
  unique_position_uv_color_binding  249   <- dedup mesh vertices before simplification
  texture_epochs_used                34
  bindings_used                      27
  submit_classes                 [0,3,6]  (RAW_CURRENT, PROJECTED_NO_Z, RANGE_OR_MATRIX)

TOPOLOGY
  connected_components               48
  distinct_edges                    382
  boundary_edges                    239
  non_manifold_edges                  0
  degenerate_triangles                0

BOUNDS (world s20.12; units = value/4096)
  X  [-13307904, 13307904]   (-3250.0 .. 3250.0)
  Y  [ -4399104, 15630336]   (-1074.0 .. 3816.0)
  Z  [-11333632,  6311936]   (-2766.9 .. 1541.0)
```

## Checker (all gates pass)

`python scripts/dreamland_world_mesh.py --check`

1. **Determinism** — rebuilt IR sha256 matches the stored IR.
2. **All static triangles represented exactly once** — the 175 emitted triangles,
   mapped back through `source_dense_index`, equal the source static corner
   triples in source order with orientation preserved.
3. **No invalid indices** — all triangle corners in `[0, 254]`.
4. **No NaN/overflow** — all world coords are s20.12 ints in s32 range.
5. **World transform matches runtime** — 0 mismatches across all 255 vertices.
6. **Static/dynamic partition explicit and documented** in the IR.

The checker was mutation-tested (corrupted coordinate → caught; dropped triangle →
caught) to confirm it is a real gate, not a rubber stamp. The stage generator's own
`--check` is unaffected (the extraction pipeline is untouched; this module only
reads its output).

## Notes for downstream tasks

- The 48 connected components confirm the multi-part island structure (main
  platform + 3 floating platforms + Whispy trunk + decorative pieces). Task 59's
  simplifier should preserve this component structure, not merge unrelated parts.
- 182 unique positions from 255 dense verts means 73 near-duplicate positions can
  be welded before any geometric simplification — a cheap, exact first reduction.
- The 606 cited stage submissions are the **full** Dream Land (static+dynamic).
  The static mesh alone is 525 corners / 255 dense verts; the dynamic 27 tris / 57
  dense verts stay separately identifiable for Task 62's split draw path.
- World coordinates are s20.12; Task 58's camera oracle and Task 61's coordinate
  quantization must use this same fixed-point representation to stay bit-exact.
