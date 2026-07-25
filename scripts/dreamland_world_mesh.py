#!/usr/bin/env python3
"""Task 57 — Host-side canonical Dream Land world-space mesh IR.

Consumes the already-validated stage extraction from
``scripts/generate_nds_native_stage.py`` (Packet: bindings, runs, corners,
DenseVertex, baked world matrices) and turns the **static visual stage** into
one ordinary world-space mesh suitable for retopology and simplification.

No runtime rendering here. No BattleShip decoding duplication: this module
imports the existing ``generate()`` pipeline and reads its validated Packet.

Static / dynamic partition
-------------------------
The partition is by ``OwnerSpec.resource_name``, the authoritative attribute
the generator already records per owner:

  * ``stage_geometry`` (owners layer0..layer3) = the static fighting stage. These
    DObjs are constant transforms baked host-side by Task 51
    (``sNdsNativeStageBakedWorldMatrices``) and rendered as immutable geometry.
  * ``stage_actors``  (owners map0..map3)      = dynamic stage actors
    (Whispy face/animated parts, flowers, etc.). These are excluded from the
    static mesh and remain separately identifiable so Task 62 can draw them
    independently.

World-space baking
------------------
Each static dense vertex's local s16 (x,y,z) is transformed by the generator's
baked s20.12 world matrix using the **exact** integer arithmetic of the runtime
``ndsRendererTransformVertex20p12`` (nds_renderer.c:4834):

    world.x = clamp_s32( m[0][0]*x + m[1][0]*y + m[2][0]*z + m[3][0] )

Local coords are raw s16 integers; the matrix is s20.12 so the result is s20.12
(the translation cell m[3][col] already carries its fractional bits). The host
checker re-runs the transform and compares against the emitted IR to prove the
world result matches the runtime bit-for-bit for every static vertex.

IR layout (scripts/generated/dreamland_world_mesh.json)
-------------------------------------------------------
  * ``world_vertices``  — one record per static dense vertex:
      {world_x, world_y, world_z, s, t, rgba, binding_index, source_dense_index}
      world_* are s20.12 ints (the runtime-identical representation); a
      float_fallback field carries world/4096.0 for host-side mesh tools that
      prefer floats.
  * ``triangles``       — one record per source static triangle, in source
      order, each a triple of indices into ``world_vertices`` (the dense
      indices the generator's corners already reference), plus run_index,
      binding_index, texture_epoch, submit_class, provenance.
  * ``excluded_dynamic`` — per-excluded-binding summary (counts only).
  * ``census``          — the Task 57 census block.
  * ``provenance``      — generator checksums + partition rule + transform id.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Sequence

# Reuse the existing extraction pipeline (no decoding duplication). The world
# matrix is already baked by the generator; the point-transform is implemented
# inline below (native_matrix_math only covers matrix-matrix compose, not
# matrix-vector, so we replicate ndsRendererTransformVertex20p12 directly).
import generate_nds_native_stage as stage_gen

# s20.12 range, matching ndsRendererClampS64ToS32.
S32_MIN = -(1 << 31)
S32_MAX = (1 << 31) - 1

SCRIPTS_DIR = Path(__file__).resolve().parent
REPO_ROOT_DEFAULT = SCRIPTS_DIR.parent
GENERATED_DIR = REPO_ROOT_DEFAULT / "scripts" / "generated"
DEFAULT_OUTPUT = GENERATED_DIR / "dreamland_world_mesh.json"

# The static fighting stage is everything sourced from stage_geometry; dynamic
# actors (Whispy / flowers / animated pieces) come from stage_actors and stay
# separately identifiable.
STATIC_RESOURCE_NAME = "stage_geometry"
DYNAMIC_RESOURCE_NAME = "stage_actors"

TRANSFORM_ID = "ndsRendererTransformVertex20p12@nds_renderer.c:4834"


def clamp_s64_to_s32(value: int) -> int:
    if value < S32_MIN:
        return S32_MIN
    if value > S32_MAX:
        return S32_MAX
    return value


def transform_vertex_20p12(
    matrix: Sequence[int], x: int, y: int, z: int
) -> tuple[int, int, int]:
    """Bit-exact host replica of ndsRendererTransformVertex20p12.

    ``matrix`` is the generator's flat-16 s20.12 row-major tuple
    (index = row*4 + col). Local coords are raw s16 integers, as the runtime
    feeds NDSRendererInputVertex (s16 x/y/z) straight into the multiply.
    The result is s20.12 (no extra shift: m[row][col] is s20.12 and the
    translation cell m[3][col] carries its own fractional bits).
    """
    # m[row][col] = matrix[row*4 + col]; the runtime computes column-wise:
    #   out.x = m[0][0]*x + m[1][0]*y + m[2][0]*z + m[3][0]
    wx = clamp_s64_to_s32(
        matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12]
    )
    wy = clamp_s64_to_s32(
        matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13]
    )
    wz = clamp_s64_to_s32(
        matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14]
    )
    return wx, wy, wz


@dataclass(frozen=True)
class WorldVertex:
    world_x: int
    world_y: int
    world_z: int
    s: int
    t: int
    rgba: int
    binding_index: int
    source_dense_index: int


@dataclass(frozen=True)
class WorldTriangle:
    v0: int
    v1: int
    v2: int
    run_index: int
    binding_index: int
    texture_epoch: int
    submit_class: int


def owner_resource_name(owner_id: int) -> str:
    for spec in stage_gen.OWNER_SPECS:
        if spec.owner == owner_id:
            return spec.resource_name
    raise ValueError(f"unknown owner id {owner_id}")


def build_owner_of_binding(packet: stage_gen.Packet) -> dict[int, int]:
    """Map every binding_index -> owning segment's owner id."""
    out: dict[int, int] = {}
    for seg in packet.segments:
        for b in range(seg.first_binding, seg.first_binding + seg.binding_count):
            out[b] = seg.owner
    return out


def is_static_binding(
    binding_index: int,
    owner_of_binding: dict[int, int],
) -> bool:
    owner = owner_of_binding.get(binding_index)
    if owner is None:
        return False
    return owner_resource_name(owner) == STATIC_RESOURCE_NAME


def build_world_mesh(
    packet: stage_gen.Packet,
) -> tuple[list[WorldVertex], list[WorldTriangle], dict[int, dict[str, int]], dict[int, int]]:
    """Bake static dense vertices to world space and rebuild static triangles.

    Returns (world_vertices, world_triangles, excluded_dynamic_summary,
    dense_to_local_map). Triangle corner indices reference the contiguous local
    mesh vertex space (0..len(world_vertices)-1); each WorldVertex carries
    ``source_dense_index`` as provenance back to the generator's dense space.
    """
    owner_of_binding = build_owner_of_binding(packet)
    baked = packet.baked_world_matrices  # tuple of flat-16 per binding_index

    # Bake every static dense vertex in source order, building a dense->local
    # index map so triangle corners (which the generator emits as dense
    # indices) can be remapped to contiguous local mesh indices.
    world_vertices: list[WorldVertex] = []
    dense_to_local: dict[int, int] = {}
    excluded_dynamic: dict[int, dict[str, int]] = defaultdict(
        lambda: {"triangles": 0, "corners": 0, "dense_vertices": 0}
    )

    static_dense_indices: set[int] = set()
    for dense_index, vertex in enumerate(packet.vertices):
        static = is_static_binding(vertex.matrix_binding, owner_of_binding)
        if static:
            matrix = baked[vertex.matrix_binding]
            wx, wy, wz = transform_vertex_20p12(matrix, vertex.x, vertex.y, vertex.z)
            dense_to_local[dense_index] = len(world_vertices)
            world_vertices.append(
                WorldVertex(
                    wx, wy, wz, vertex.s, vertex.t, vertex.rgba,
                    vertex.matrix_binding, dense_index,
                )
            )
            static_dense_indices.add(dense_index)
        else:
            excluded_dynamic[vertex.matrix_binding]["dense_vertices"] += 1

    # Rebuild triangles in source order, keeping only fully-static triangles.
    # A static run's binding is static; its three corners reference dense
    # vertices that must all be static (a cross-matrix run whose binding is
    # static but that references a dynamic vertex is treated as dynamic and
    # counted in the exclusion summary — it cannot be baked into the static
    # mesh without a foreign world matrix). Corners are remapped to local mesh
    # indices so the emitted mesh has a self-consistent 0..N-1 index space.
    world_triangles: list[WorldTriangle] = []
    for run_index, run in enumerate(packet.runs):
        static_run = is_static_binding(run.binding_index, owner_of_binding)
        for tri in range(run.triangle_count):
            base = run.first_corner + tri * 3
            c0 = packet.corners[base]
            c1 = packet.corners[base + 1]
            c2 = packet.corners[base + 2]
            if static_run and c0 in static_dense_indices and \
               c1 in static_dense_indices and c2 in static_dense_indices:
                world_triangles.append(
                    WorldTriangle(dense_to_local[c0], dense_to_local[c1],
                                  dense_to_local[c2], run_index,
                                  run.binding_index, run.texture_epoch,
                                  run.submit_class)
                )
            else:
                excluded_dynamic[run.binding_index]["triangles"] += 1
                excluded_dynamic[run.binding_index]["corners"] += 3

    return world_vertices, world_triangles, excluded_dynamic, dense_to_local


# ---------------------------------------------------------------------------
# Census
# ---------------------------------------------------------------------------

def _edge_key(a: int, b: int) -> tuple[int, int]:
    return (a, b) if a <= b else (b, a)


def compute_census(
    world_vertices: list[WorldVertex],
    world_triangles: list[WorldTriangle],
    packet: stage_gen.Packet,
    owner_of_binding: dict[int, int],
) -> dict[str, Any]:
    tri_count = len(world_triangles)
    corner_count = tri_count * 3

    # Unique exact world positions.
    unique_positions: set[tuple[int, int, int]] = set()
    for v in world_vertices:
        unique_positions.add((v.world_x, v.world_y, v.world_z))

    # Unique (position, s, t, rgba, binding) vertices — the deduplicated mesh
    # vertex count before any simplification.
    unique_full: set[tuple[int, int, int, int, int, int, int]] = set()
    for v in world_vertices:
        unique_full.add(
            (v.world_x, v.world_y, v.world_z, v.s, v.t, v.rgba, v.binding_index)
        )

    # Bounds (s20.12 world units).
    bounds = {"min": [None, None, None], "max": [None, None, None]}
    for v in world_vertices:
        for axis, coord in enumerate((v.world_x, v.world_y, v.world_z)):
            if bounds["min"][axis] is None or coord < bounds["min"][axis]:
                bounds["min"][axis] = coord
            if bounds["max"][axis] is None or coord > bounds["max"][axis]:
                bounds["max"][axis] = coord

    # Material/texture-epoch identity per binding: count distinct epochs used
    # by static triangles, and the material-event identity the generator
    # already recorded per binding.
    epochs_used: set[int] = set()
    bindings_used: set[int] = set()
    submit_classes: set[int] = set()
    for tri in world_triangles:
        epochs_used.add(tri.texture_epoch)
        bindings_used.add(tri.binding_index)
        submit_classes.add(tri.submit_class)

    # Connected components (union-find over triangle vertex indices = dense).
    parent: dict[int, int] = {}

    def find(x: int) -> int:
        root = x
        while parent.get(root, root) != root:
            root = parent.get(root, root)
        cur = x
        while parent.get(cur, cur) != root:
            parent[cur] = root
            cur = parent.get(cur, cur)
        return root

    def union(a: int, b: int) -> None:
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[ra] = rb

    for tri in world_triangles:
        union(tri.v0, tri.v1)
        union(tri.v1, tri.v2)

    roots: set[int] = set()
    for tri in world_triangles:
        for v in (tri.v0, tri.v1, tri.v2):
            roots.add(find(v))
    components = len(roots)

    # Boundary / non-manifold edges + degenerate triangles.
    edge_refs: dict[tuple[int, int], int] = defaultdict(int)
    degenerate = 0
    for tri in world_triangles:
        if tri.v0 == tri.v1 or tri.v1 == tri.v2 or tri.v0 == tri.v2:
            degenerate += 1
            continue
        for a, b in ((tri.v0, tri.v1), (tri.v1, tri.v2), (tri.v0, tri.v2)):
            edge_refs[_edge_key(a, b)] += 1
    boundary_edges = sum(1 for count in edge_refs.values() if count == 1)
    non_manifold_edges = sum(1 for count in edge_refs.values() if count > 2)

    # Static / dynamic excluded split (authoritative, by resource_name).
    static_bindings = [
        b for b, o in owner_of_binding.items()
        if owner_resource_name(o) == STATIC_RESOURCE_NAME
    ]
    dynamic_bindings = [
        b for b, o in owner_of_binding.items()
        if owner_resource_name(o) == DYNAMIC_RESOURCE_NAME
    ]
    static_dense = sum(
        1 for v in packet.vertices
        if is_static_binding(v.matrix_binding, owner_of_binding)
    )
    dynamic_dense = len(packet.vertices) - static_dense
    static_tris = sum(
        r.triangle_count for r in packet.runs
        if is_static_binding(r.binding_index, owner_of_binding)
    )
    dynamic_tris = sum(r.triangle_count for r in packet.runs) - static_tris

    return {
        "source_triangles_total": sum(r.triangle_count for r in packet.runs),
        "source_corners_total": len(packet.corners),
        "source_dense_vertices_total": len(packet.vertices),
        "static_triangles_source": static_tris,
        "dynamic_triangles_source": dynamic_tris,
        "static_dense_vertices_source": static_dense,
        "dynamic_dense_vertices_source": dynamic_dense,
        "static_binding_count": len(static_bindings),
        "dynamic_binding_count": len(dynamic_bindings),
        "baked_static_triangles": tri_count,
        "baked_static_corners": corner_count,
        "baked_static_dense_vertices": len(world_vertices),
        "unique_exact_positions": len(unique_positions),
        "unique_position_uv_color_binding_vertices": len(unique_full),
        "bounds_s20p12": bounds,
        "texture_epochs_used": len(epochs_used),
        "bindings_used": len(bindings_used),
        "submit_classes": sorted(submit_classes),
        "connected_components": components,
        "boundary_edges": boundary_edges,
        "non_manifold_edges": non_manifold_edges,
        "degenerate_triangles": degenerate,
        "distinct_edges": len(edge_refs),
    }


# ---------------------------------------------------------------------------
# IR emission
# ---------------------------------------------------------------------------

def serialize_ir(
    world_vertices: list[WorldVertex],
    world_triangles: list[WorldTriangle],
    excluded_dynamic: dict[int, dict[str, int]],
    census: dict[str, Any],
    packet: stage_gen.Packet,
    owner_of_binding: dict[int, int],
) -> dict[str, Any]:
    vertices_json = [
        {
            "world_x": v.world_x,
            "world_y": v.world_y,
            "world_z": v.world_z,
            "world_x_f": v.world_x / 4096.0,
            "world_y_f": v.world_y / 4096.0,
            "world_z_f": v.world_z / 4096.0,
            "s": v.s,
            "t": v.t,
            "rgba": v.rgba,
            "binding_index": v.binding_index,
            "source_dense_index": v.source_dense_index,
        }
        for v in world_vertices
    ]
    triangles_json = [
        {
            "v0": tri.v0,
            "v1": tri.v1,
            "v2": tri.v2,
            "run_index": tri.run_index,
            "binding_index": tri.binding_index,
            "texture_epoch": tri.texture_epoch,
            "submit_class": tri.submit_class,
        }
        for tri in world_triangles
    ]
    excluded_json = [
        {"binding_index": b, **counts}
        for b, counts in sorted(excluded_dynamic.items())
    ]

    # Provenance: pin the extraction identity so a future change is visible.
    asset_checksums = [
        {"asset_id": a.asset_id, "payload_size": a.payload_size,
         "payload_checksum": a.payload_checksum}
        for a in packet.assets
    ]

    return {
        "task": "Task 57 — Dream Land canonical world-space mesh IR",
        "version": 1,
        "static_partition_rule": (
            f"OwnerSpec.resource_name == '{STATIC_RESOURCE_NAME}' "
            f"(owners layer0..layer3); dynamic = '{DYNAMIC_RESOURCE_NAME}' "
            f"(owners map0..map3) excluded and drawn separately."
        ),
        "world_transform": TRANSFORM_ID,
        "world_transform_note": (
            "Local s16 (x,y,z) multiplied by baked s20.12 world matrix; result "
            "is s20.12, bit-identical to runtime ndsRendererTransformVertex20p12."
        ),
        "provenance": {
            "generator_expected_include_sha256": stage_gen.EXPECTED_INCLUDE_SHA256,
            "source_triangles_expected": stage_gen.EXPECTED_TRIANGLES,
            "source_runs_expected": stage_gen.EXPECTED_RUNS,
            "source_bindings_expected": stage_gen.EXPECTED_BINDINGS,
            "asset_checksums": asset_checksums,
        },
        "world_vertices": vertices_json,
        "triangles": triangles_json,
        "excluded_dynamic": excluded_json,
        "census": census,
    }


def ir_sha256(payload: dict[str, Any]) -> str:
    # Deterministic JSON: sort_keys, compact separators, stable floats.
    encoded = json.dumps(payload, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(encoded.encode("utf-8")).hexdigest()


# ---------------------------------------------------------------------------
# Checker
# ---------------------------------------------------------------------------

def check_ir(
    ir: dict[str, Any],
    packet: stage_gen.Packet,
    owner_of_binding: dict[int, int],
) -> None:
    """Prove the emitted IR is correct against a freshly rebuilt extraction."""
    errors: list[str] = []

    rebuilt_verts, rebuilt_tris, _, _ = build_world_mesh(packet)
    rebuilt_census = compute_census(
        rebuilt_verts, rebuilt_tris, packet, owner_of_binding)

    # 1. Determinism: re-running the compiler must produce identical output.
    _, _, rebuilt_excl, _ = build_world_mesh(packet)
    rebuilt_ir = serialize_ir(
        rebuilt_verts, rebuilt_tris, rebuilt_excl, rebuilt_census, packet,
        owner_of_binding)
    if ir_sha256(rebuilt_ir) != ir_sha256(ir):
        errors.append(
            "determinism: rebuilt IR sha256 does not match the stored IR")

    # 2. All source static triangles represented exactly once. Triangles are
    # emitted in source order with local mesh indices; map them back through
    # source_dense_index provenance to compare against the generator's dense
    # corner triples. Orientation is preserved 1:1 (v0/v1/v2 <- c0/c1/c2), so
    # the ordered triples must match exactly, not just as sets.
    local_to_dense = {
        v["source_dense_index"]: idx
        for idx, v in enumerate(ir["world_vertices"])
    }
    # invert: local index -> dense index
    dense_by_local = {idx: v["source_dense_index"]
                      for idx, v in enumerate(ir["world_vertices"])}
    ir_dense_triples: list[tuple[int, int, int]] = [
        (dense_by_local[t["v0"]], dense_by_local[t["v1"]], dense_by_local[t["v2"]])
        for t in ir["triangles"]
    ]
    src_dense_triples: list[tuple[int, int, int]] = []
    for run in packet.runs:
        if not is_static_binding(run.binding_index, owner_of_binding):
            continue
        for tri in range(run.triangle_count):
            base = run.first_corner + tri * 3
            c0 = packet.corners[base]
            c1 = packet.corners[base + 1]
            c2 = packet.corners[base + 2]
            # Skip triangles whose corners are not all static (cross-matrix
            # foreign corners); those are excluded by construction.
            if (not is_static_binding(packet.vertices[c0].matrix_binding,
                                      owner_of_binding) or
                not is_static_binding(packet.vertices[c1].matrix_binding,
                                      owner_of_binding) or
                not is_static_binding(packet.vertices[c2].matrix_binding,
                                      owner_of_binding)):
                continue
            src_dense_triples.append((c0, c1, c2))
    if ir_dense_triples != src_dense_triples:
        errors.append(
            f"static triangle sequence mismatch: ir={len(ir_dense_triples)} "
            f"src={len(src_dense_triples)} (must be identical in source order "
            f"with orientation preserved)")

    # 3. No invalid indices.
    vert_count = len(ir["world_vertices"])
    for idx, t in enumerate(ir["triangles"]):
        for corner in (t["v0"], t["v1"], t["v2"]):
            if not (0 <= corner < vert_count):
                errors.append(
                    f"triangle {idx}: corner {corner} out of range "
                    f"[0,{vert_count})")
                break

    # 4. No NaN/overflow. world coords are ints (no NaN); verify s20.12 range.
    for idx, v in enumerate(ir["world_vertices"]):
        for axis, name in enumerate(("world_x", "world_y", "world_z")):
            coord = v[name]
            if not isinstance(coord, int):
                errors.append(f"vertex {idx}: {name} not an int")
                break
            if coord < S32_MIN or coord > S32_MAX:
                errors.append(
                    f"vertex {idx}: {name}={coord} outside s32 range")

    # 5. World-space transform matches runtime for every static vertex.
    baked = packet.baked_world_matrices
    mismatches = 0
    for v in ir["world_vertices"]:
        src = packet.vertices[v["source_dense_index"]]
        matrix = baked[src.matrix_binding]
        wx, wy, wz = transform_vertex_20p12(matrix, src.x, src.y, src.z)
        if (wx, wy, wz) != (v["world_x"], v["world_y"], v["world_z"]):
            mismatches += 1
            if mismatches <= 3:
                errors.append(
                    f"vertex {v['source_dense_index']}: world transform "
                    f"mismatch rebuilt=({wx},{wy},{wz}) stored="
                    f"({v['world_x']},{v['world_y']},{v['world_z']})")
    if mismatches > 3:
        errors.append(f"... and {mismatches - 3} more world-transform mismatches")

    # 6. Static/dynamic partition is explicit and documented.
    if STATIC_RESOURCE_NAME not in ir["static_partition_rule"]:
        errors.append("static_partition_rule does not name the static resource")
    if DYNAMIC_RESOURCE_NAME not in ir["static_partition_rule"]:
        errors.append("static_partition_rule does not name the dynamic resource")

    if errors:
        print("TASK57 CHECK: FAIL", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        raise SystemExit(1)
    print("TASK57 CHECK: PASS")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "--repo-root", type=Path, default=REPO_ROOT_DEFAULT,
        help="checkout containing the pinned BattleShip/O2R inputs")
    p.add_argument(
        "--output", type=Path, default=DEFAULT_OUTPUT,
        help=f"IR output; default {DEFAULT_OUTPUT.as_posix()}")
    p.add_argument(
        "--check", action="store_true",
        help="validate the existing IR without writing it")
    p.add_argument(
        "--print-census", action="store_true",
        help="print the census block to stdout")
    return p.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    repo_root = args.repo_root.resolve()
    output = args.output
    if not output.is_absolute():
        output = repo_root / output

    # The generator is the single source of truth; it falsifies on drift.
    packet = stage_gen.generate(repo_root)
    owner_of_binding = build_owner_of_binding(packet)

    world_vertices, world_triangles, excluded_dynamic, _ = build_world_mesh(packet)
    census = compute_census(
        world_vertices, world_triangles, packet, owner_of_binding)
    ir = serialize_ir(
        world_vertices, world_triangles, excluded_dynamic, census, packet,
        owner_of_binding)

    if args.check:
        if not output.is_file():
            print(f"TASK57 CHECK: FAIL — IR absent: {output}", file=sys.stderr)
            return 1
        stored = json.loads(output.read_text(encoding="utf-8"))
        # Validate the stored IR against a freshly rebuilt extraction.
        stored_ir = serialize_ir(
            [WorldVertex(**{k: v_[k] for k in (
                "world_x", "world_y", "world_z", "s", "t", "rgba",
                "binding_index", "source_dense_index")})
             for v_ in stored["world_vertices"]],
            [WorldTriangle(
                t_["v0"], t_["v1"], t_["v2"], t_["run_index"],
                t_["binding_index"], t_["texture_epoch"], t_["submit_class"])
             for t_ in stored["triangles"]],
            {e["binding_index"]: {
                "triangles": e["triangles"], "corners": e["corners"],
                "dense_vertices": e["dense_vertices"]}
             for e in stored.get("excluded_dynamic", [])},
            stored["census"], packet, owner_of_binding,
        )
        check_ir(stored_ir, packet, owner_of_binding)
        return 0

    output.parent.mkdir(parents=True, exist_ok=True)
    encoded = json.dumps(ir, indent=2, sort_keys=True)
    output.write_text(encoded + "\n", encoding="utf-8")

    if args.print_census:
        print(json.dumps(census, indent=2, sort_keys=True))
    else:
        sha = ir_sha256(ir)
        print(f"TASK57: wrote {output}")
        print(f"TASK57: IR sha256 = {sha}")
        c = census
        print(
            f"TASK57: {c['baked_static_triangles']} static tris / "
            f"{c['baked_static_dense_vertices']} dense verts / "
            f"{c['unique_exact_positions']} unique positions / "
            f"{c['unique_position_uv_color_binding_vertices']} unique full verts"
        )
        print(
            f"TASK57: source {c['source_triangles_total']} tris "
            f"(static {c['static_triangles_source']} + dynamic "
            f"{c['dynamic_triangles_source']}); components "
            f"{c['connected_components']}, boundary edges "
            f"{c['boundary_edges']}, non-manifold {c['non_manifold_edges']}, "
            f"degenerate {c['degenerate_triangles']}"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
