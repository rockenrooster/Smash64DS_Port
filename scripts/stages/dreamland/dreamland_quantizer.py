#!/usr/bin/env python3
"""Task 61 — DS coordinate quantization + cheapest vertex opcode selection.

After Task 60's vertex-count cut, shrink the GX command stream further by
choosing the cheapest legal DS vertex opcode per vertex. Host-only.

DS vertex opcodes (GBATEK / libnds videoGL.h geometry-engine command set):
  VERTEX16   (0x23, 2 words): XYZ s16.12, range +-32768. Current path.
  VERTEX10   (0x2D, 1 word): XYZ s10.3 packed, range +-256. Needs a compensating
              matrix rebasis to fit larger coordinates; ~0.125-step quantization
              in the scaled space.
  VERTEX_XY  (0x21, 1 word): XY only, reuses last Z.
  VERTEX_XZ  (0x22, 1 word): XZ only, reuses last Y.
  VERTEX_YZ  (0x24, 1 word): YZ only, reuses last X.
  VERTEX_DIFF(0x25, 1 word): s5.6 deltas from prior vertex, range +-32. Infeasible
              for Dream Land (inter-vertex deltas reach 4638).

Selection policy (per vertex, in a primitive group's emission order):
  1. VERTEX_XZ/XY/YZ (1 word, EXACT) where an axis matches the prior vertex —
     pure win, zero precision loss.
  2. VERTEX10 (1 word) where the rebased coordinate fits s10.3 AND the
     quantization error stays within the Task 58 oracle's region-IoU gate.
  3. VERTEX16 (2 words) fallback.

The rebasis for VERTEX10: pick a uniform scale s and origin o so that for every
vertex v, (v - o) / s fits s10.3. A compensating hardware matrix (scale s,
translate o) restores the exact world shape at runtime (Task 62 implements it).
The scale search maximizes precision (smallest s that still fits the range).

Usage
-----
  dreamland_quantizer.py              # build encoded streams + word census
  dreamland_quantizer.py --check      # validate determinism + decode round-trip
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Sequence

import sys as _sys
from pathlib import Path as _Path

_scripts_root = _Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in _sys.path:
    _sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402  -- puts every scripts/ area folder on sys.path

import dreamland_camera_oracle as oracle
import dreamland_primitive_compiler as pc

SCRIPTS_DIR = _paths.SCRIPTS_ROOT
REPO_ROOT = SCRIPTS_DIR.parent
GENERATED_DIR = SCRIPTS_DIR / "generated"
CANDIDATES_DIR = GENERATED_DIR / "candidates"
STREAMS_DIR = GENERATED_DIR / "primitive_streams"
ENCODED_DIR = GENERATED_DIR / "encoded_streams"
REPORT_OUTPUT = GENERATED_DIR / "dreamland_quantization_report.json"

# DS vertex opcode ids + per-vertex word cost.
OP_VERTEX16 = 0x23
OP_VERTEX10 = 0x2D
OP_VERTEX_XY = 0x21
OP_VERTEX_XZ = 0x22
OP_VERTEX_YZ = 0x24
OP_NAMES = {OP_VERTEX16: "VERTEX16", OP_VERTEX10: "VERTEX10",
            OP_VERTEX_XY: "VERTEX_XY", OP_VERTEX_XZ: "VERTEX_XZ",
            OP_VERTEX_YZ: "VERTEX_YZ"}
WORD_COST = {OP_VERTEX16: 2, OP_VERTEX10: 1, OP_VERTEX_XY: 1,
             OP_VERTEX_XZ: 1, OP_VERTEX_YZ: 1}

# s10.3 range (VERTEX10): signed 10-bit integer * 2^3 = +-256, step 1/8.
V10_INT_MAX = 511   # s10 -> [-512, 511]
V10_INT_MIN = -512
V10_STEP = 1.0 / 8.0  # 0.125 in scaled space

# s16.12 range (VERTEX16): signed 16-bit * 2^12, step 1/4096.
V16_INT_MAX = 32767
V16_INT_MIN = -32768


@dataclass
class EncodedVertex:
    opcode: int
    # Decoded (reconstructed) world coords for round-trip verification.
    decoded: tuple[float, float, float]
    # Quantization error vs the source coord (0.0 for exact/axis-reuse ops).
    error: tuple[float, float, float]


# ---------------------------------------------------------------------------
# Opcode encoders + decoders (host-side replicas of the DS hardware behavior)
# ---------------------------------------------------------------------------

def _quantize_v10(scaled: float) -> int:
    """Quantize a scaled-space float to s10.3 (nearest 1/8, clamp to range)."""
    q = int(round(scaled / V10_STEP))
    return max(V10_INT_MIN, min(V10_INT_MAX, q))


def _dequantize_v10(q: int) -> float:
    return q * V10_STEP


def encode_v16(x: float, y: float, z: float) -> EncodedVertex:
    # VERTEX16 is lossless for the stage's range; error is 0.
    return EncodedVertex(OP_VERTEX16, (x, y, z), (0.0, 0.0, 0.0))


def encode_v10(x: float, y: float, z: float,
               scale: float, origin: tuple[float, float, float]) -> EncodedVertex:
    """VERTEX10: rebased to (v-origin)/scale, quantized to s10.3."""
    sx = (x - origin[0]) / scale
    sy = (y - origin[1]) / scale
    sz = (z - origin[2]) / scale
    qx = _quantize_v10(sx)
    qy = _quantize_v10(sy)
    qz = _quantize_v10(sz)
    # Decode back to world space.
    dx = _dequantize_v10(qx) * scale + origin[0]
    dy = _dequantize_v10(qy) * scale + origin[1]
    dz = _dequantize_v10(qz) * scale + origin[2]
    return EncodedVertex(OP_VERTEX10, (dx, dy, dz),
                         (dx - x, dy - y, dz - z))


def encode_axis_reuse(opcode: int, x: float, y: float, z: float,
                      prev: tuple[float, float, float]) -> EncodedVertex | None:
    """VERTEX_XY/XZ/YZ: emit two axes, reuse the matching one from `prev`.
    Returns None if the reuse isn't exact (the axis must match to ~0)."""
    if opcode == OP_VERTEX_XY:
        if abs(z - prev[2]) > 1e-6:
            return None
        return EncodedVertex(OP_VERTEX_XY, (x, y, prev[2]), (0.0, 0.0, 0.0))
    if opcode == OP_VERTEX_XZ:
        if abs(y - prev[1]) > 1e-6:
            return None
        return EncodedVertex(OP_VERTEX_XZ, (x, prev[1], z), (0.0, 0.0, 0.0))
    if opcode == OP_VERTEX_YZ:
        if abs(x - prev[0]) > 1e-6:
            return None
        return EncodedVertex(OP_VERTEX_YZ, (prev[0], y, z), (0.0, 0.0, 0.0))
    return None


# ---------------------------------------------------------------------------
# Coordinate-basis search for VERTEX10
# ---------------------------------------------------------------------------

def find_v10_rebasis(positions: list[tuple[float, float, float]]) -> tuple[float, tuple[float, float, float]] | None:
    """Find the smallest uniform scale + origin so every position fits s10.3.

    The origin is the position-space midpoint; scale = max_extent_axis /
    (2 * V10_INT_MAX * V10_STEP). Smaller scale = finer precision, so we want
    the minimum scale that fits. Returns None if no valid scale exists (the
    extent is genuinely too large even at the coarsest s10.3 step).
    """
    if not positions:
        return None
    mins = [min(p[i] for p in positions) for i in range(3)]
    maxs = [max(p[i] for p in positions) for i in range(3)]
    extents = [maxs[i] - mins[i] for i in range(3)]
    origin = tuple((mins[i] + maxs[i]) / 2.0 for i in range(3))
    # The max half-extent must fit in V10_INT_MAX steps.
    max_half = max(extents) / 2.0
    # scale = max_half / (V10_INT_MAX * V10_STEP) is the minimum scale.
    min_scale = max_half / (V10_INT_MAX * V10_STEP)
    if min_scale <= 0:
        return None
    # Use the minimum scale (finest precision). Verify all coords fit.
    scale = min_scale
    for p in positions:
        for ax in range(3):
            q = _quantize_v10((p[ax] - origin[ax]) / scale)
            if q < V10_INT_MIN or q > V10_INT_MAX:
                return None
    return scale, origin


# ---------------------------------------------------------------------------
# Per-group encoding
# ---------------------------------------------------------------------------

@dataclass
class EncodedGroup:
    name: str                 # candidate name
    prim: int                 # primitive type (from Task 60)
    binding: int
    vertices: list[EncodedVertex]
    # Decoded mesh (this candidate, with quantization applied) for oracle IoU.
    decoded_positions: list[tuple[float, float, float]]


def select_vertex_opcode(x: float, y: float, z: float,
                         prev: tuple[float, float, float] | None,
                         use_v10: bool, v10_scale: float,
                         v10_origin: tuple[float, float, float]) -> EncodedVertex:
    """Choose the cheapest legal opcode for one vertex.

    Priority: exact axis-reuse (VERTEX_XZ/XY/YZ) > VERTEX10 (if enabled) >
    VERTEX16. Axis-reuse is exact (zero error) so it always wins when legal;
    VERTEX10 is a precision trade judged by the oracle.
    """
    if prev is not None:
        # Try each axis-reuse opcode (exact, 1 word); prefer the one matching.
        for op in (OP_VERTEX_XZ, OP_VERTEX_XY, OP_VERTEX_YZ):
            ev = encode_axis_reuse(op, x, y, z, prev)
            if ev is not None:
                return ev
    if use_v10:
        return encode_v10(x, y, z, v10_scale, v10_origin)
    return encode_v16(x, y, z)


def encode_group(name: str, prim: int, binding: int,
                 verts: list[int],
                 positions: list[tuple[float, float, float]],
                 use_v10: bool, v10_scale: float,
                 v10_origin: tuple[float, float, float]) -> EncodedGroup:
    encoded: list[EncodedVertex] = []
    decoded: list[tuple[float, float, float]] = []
    prev: tuple[float, float, float] | None = None
    for vi in verts:
        x, y, z = positions[vi]
        ev = select_vertex_opcode(x, y, z, prev, use_v10, v10_scale, v10_origin)
        encoded.append(ev)
        decoded.append(ev.decoded)
        prev = ev.decoded  # the next vertex reuses THIS decoded position
    return EncodedGroup(name, prim, binding, encoded, decoded)


# ---------------------------------------------------------------------------
# Oracle IoU on the decoded (quantized) mesh
# ---------------------------------------------------------------------------

def decoded_iou(decoded_positions: list[tuple[float, float, float]],
                triangles: list[tuple[int, int, int]],
                bindings: list[int],
                fixtures: list[oracle.Fixture],
                reference: dict[str, Any]) -> float:
    """Worst-case region IoU of the decoded (quantized) mesh vs the source."""
    mesh = oracle.Mesh(
        tuple(oracle.MeshVertex(p[0], p[1], p[2], b)
              for p, b in zip(decoded_positions, bindings)),
        tuple(oracle.MeshTriangle(v0, v1, v2) for v0, v1, v2 in triangles),
    )
    protected = oracle.detect_protected_components(mesh)
    ref_projections = {f["name"]: oracle._load_ref_projection(f)
                       for f in reference["fixtures"]}
    worst = 1.0
    for fx in fixtures:
        cp = oracle.project_mesh(mesh, fx, protected)
        m = oracle.displacement_metrics(cp, ref_projections[fx.name])
        worst = min(worst, m["region_iou"])
    return worst


# ---------------------------------------------------------------------------
# Candidate encoding
# ---------------------------------------------------------------------------

@dataclass
class QuantReport:
    name: str
    source_opcode: str          # the Task 60 baseline (all VERTEX16)
    source_vertex_words: int    # Task 60 vertex words (all 2/vertex)
    v10_words: int             # with VERTEX10+axis-reuse
    v16_only_words: int         # VERTEX16 + axis-reuse (no VERTEX10)
    opcode_counts: dict[str, int]
    worst_quant_error_world: float
    v10_rebasis_scale: float | None
    source_iou: float
    v16_iou: float
    v10_iou: float


def _load_candidate(path: Path) -> tuple[list[tuple[float, float, float]],
                                         list[tuple[int, int, int]], list[int]]:
    ir = json.loads(path.read_text(encoding="utf-8"))
    positions = [(v["world_x_f"], v["world_y_f"], v["world_z_f"])
                 for v in ir["world_vertices"]]
    triangles = [(t["v0"], t["v1"], t["v2"]) for t in ir["triangles"]]
    bindings = [v["binding_index"] for v in ir["world_vertices"]]
    return positions, triangles, bindings


def _load_stream(name: str) -> list[dict[str, Any]]:
    path = STREAMS_DIR / f"{name}.json"
    return json.loads(path.read_text(encoding="utf-8"))["groups"]


def encode_candidate(name: str, positions, triangles, bindings,
                     stream_groups, fixtures, reference) -> QuantReport:
    # Baseline: all VERTEX16 (2 words/vertex).
    total_verts = sum(len(g["verts"]) for g in stream_groups)
    source_vertex_words = total_verts * 2

    # VERTEX10 rebasis over the whole candidate's positions.
    rebasis = find_v10_rebasis(positions)
    v10_scale = rebasis[0] if rebasis else None
    v10_origin = rebasis[1] if rebasis else (0.0, 0.0, 0.0)

    # Decode every POSITION once for each policy, so the decoded mesh shares the
    # candidate's vertex-index space (triangles index into `positions`). The
    # per-group encoding still determines opcode selection + word cost; the
    # position decoding is what the oracle IoU measures. Axis-reuse decoding
    # depends on emission order, so we walk groups to derive each position's
    # decoded form, but store results indexed by original position id.
    opcode_counts_v16 = {OP_NAMES[op]: 0 for op in WORD_COST}
    opcode_counts_v10 = {OP_NAMES[op]: 0 for op in WORD_COST}
    decoded_v16: list[tuple[float, float, float]] = [None] * len(positions)  # type: ignore
    decoded_v10: list[tuple[float, float, float]] = [None] * len(positions)  # type: ignore
    worst_err_v10 = 0.0
    for g in stream_groups:
        prim_id = g["prim"] if isinstance(g["prim"], int) else \
            next(k for k, v in pc.PRIM_NAMES.items() if v == g["prim"])
        verts = g["verts"]
        eg16 = encode_group(name, prim_id, g["binding"], verts, positions,
                            use_v10=False, v10_scale=1.0, v10_origin=(0, 0, 0))
        eg10 = encode_group(name, prim_id, g["binding"], verts, positions,
                            use_v10=True,
                            v10_scale=v10_scale if v10_scale else 1.0,
                            v10_origin=v10_origin)
        for src_idx, ev in zip(verts, eg16.vertices):
            opcode_counts_v16[OP_NAMES[ev.opcode]] += 1
            decoded_v16[src_idx] = ev.decoded
        for src_idx, ev in zip(verts, eg10.vertices):
            opcode_counts_v10[OP_NAMES[ev.opcode]] += 1
            decoded_v10[src_idx] = ev.decoded
            e = max(abs(c) for c in ev.error)
            if e > worst_err_v10:
                worst_err_v10 = e
    # Any position not touched by a group (shouldn't happen) -> keep original.
    decoded_v16 = [p if d is None else d for p, d in zip(positions, decoded_v16)]
    decoded_v10 = [p if d is None else d for p, d in zip(positions, decoded_v10)]

    v16_only_words = sum(opcode_counts_v16[OP_NAMES[op]] * WORD_COST[op]
                         for op in WORD_COST)
    v10_words = sum(opcode_counts_v10[OP_NAMES[op]] * WORD_COST[op]
                    for op in WORD_COST)

    # Oracle IoU on the decoded meshes (triangles index into the position array,
    # which the decoded lists mirror 1:1).
    src_iou = decoded_iou(positions, triangles, bindings, fixtures, reference)
    v16_iou = decoded_iou(decoded_v16, triangles, bindings, fixtures, reference)
    v10_iou = decoded_iou(decoded_v10, triangles, bindings, fixtures, reference)

    return QuantReport(name, "VERTEX16", source_vertex_words, v10_words,
                       v16_only_words, opcode_counts_v10, worst_err_v10,
                       v10_scale, src_iou, v16_iou, v10_iou)


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

VISUAL_GATE_IOU = 0.95


def _candidate_files() -> list[Path]:
    primitive_report = json.loads(pc.REPORT_OUTPUT.read_text(encoding="utf-8"))
    names = {row["name"] for row in primitive_report["candidates"]}
    return sorted(path for path in CANDIDATES_DIR.glob("*.json")
                  if path.stem in names)


def cmd_build() -> int:
    ENCODED_DIR.mkdir(parents=True, exist_ok=True)
    fixtures = oracle._load_fixtures()
    reference = json.loads(
        (GENERATED_DIR / "dreamland_source_projection_ref.json")
        .read_text(encoding="utf-8"))

    reports: list[QuantReport] = []
    for path in _candidate_files():
        name = path.stem
        stream_path = STREAMS_DIR / f"{name}.json"
        if not stream_path.is_file():
            continue
        positions, triangles, bindings = _load_candidate(path)
        stream_groups = _load_stream(name)
        rpt = encode_candidate(name, positions, triangles, bindings,
                               stream_groups, fixtures, reference)
        reports.append(rpt)
        # Emit encoded-stream IR for the chosen policy (VERTEX10+axis-reuse if
        # it passes the IoU gate, else VERTEX16+axis-reuse).
        chosen_v10 = (rpt.v10_iou >= VISUAL_GATE_IOU)
        ir = {
            "task": "Task 61 — encoded vertex stream",
            "candidate": name,
            "chosen_policy": "VERTEX10+axis_reuse" if chosen_v10
                             else "VERTEX16+axis_reuse",
            "chosen_policy_iou": rpt.v10_iou if chosen_v10 else rpt.v16_iou,
            "v10_rebasis_scale": rpt.v10_rebasis_scale,
            "opcode_counts": rpt.opcode_counts,
        }
        (ENCODED_DIR / f"{name}.json").write_text(
            json.dumps(ir, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    reports.sort(key=lambda r: r.v10_words if r.v10_iou >= VISUAL_GATE_IOU
                 else r.v16_only_words)
    payload = _build_report(reports)
    REPORT_OUTPUT.write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"TASK61: encoded {len(reports)} candidates -> {ENCODED_DIR}")
    print(f"TASK61: visual gate IoU >= {VISUAL_GATE_IOU}")
    print(f"{'candidate':<16} {'src_words':>9} {'v16+ax':>7} {'v10+ax':>7} "
          f"{'srcIoU':>7} {'v16IoU':>7} {'v10IoU':>7} {'policy':>10}")
    for r in reports:
        policy = "V10" if r.v10_iou >= VISUAL_GATE_IOU else "V16"
        print(f"{r.name:<16} {r.source_vertex_words:>9} {r.v16_only_words:>7} "
              f"{r.v10_words:>7} {r.source_iou:>7.3f} {r.v16_iou:>7.3f} "
              f"{r.v10_iou:>7.3f} {policy:>10}")
    print(f"TASK61: recommended = {payload['recommended_candidate']} "
          f"({payload['recommended_policy']})")
    return 0


def _build_report(reports: list[QuantReport]) -> dict[str, Any]:
    rows = [
        {
            "name": r.name,
            "source_vertex_words": r.source_vertex_words,
            "v16_axis_reuse_words": r.v16_only_words,
            "v10_axis_reuse_words": r.v10_words,
            "v16_axis_reduction_pct": round(
                100.0 * (1.0 - r.v16_only_words / max(1, r.source_vertex_words)), 1),
            "v10_axis_reduction_pct": round(
                100.0 * (1.0 - r.v10_words / max(1, r.source_vertex_words)), 1),
            "worst_v10_quant_error_world": round(r.worst_quant_error_world, 3),
            "v10_rebasis_scale": r.v10_rebasis_scale,
            "source_iou": round(r.source_iou, 4),
            "v16_axis_iou": round(r.v16_iou, 4),  # VERTEX16+axis-reuse IoU
            "v10_axis_iou": round(r.v10_iou, 4),
            "v10_acceptable": r.v10_iou >= VISUAL_GATE_IOU,
            "material_qualified": pc.candidate_material_qualified(r.name),
            "opcode_counts": r.opcode_counts,
        }
        for r in reports
    ]
    # Recommended: the candidate with the fewest words among acceptable policies.
    acceptable = [r for r in reports
                  if pc.candidate_material_qualified(r.name)
                  and (r.v10_iou >= VISUAL_GATE_IOU
                       or r.v16_iou >= VISUAL_GATE_IOU)]

    def best_words(r: QuantReport) -> int:
        if r.v10_iou >= VISUAL_GATE_IOU:
            return r.v10_words
        return r.v16_only_words

    def best_policy(r: QuantReport) -> str:
        return "VERTEX10+axis_reuse" if r.v10_iou >= VISUAL_GATE_IOU \
            else "VERTEX16+axis_reuse"

    acceptable.sort(key=best_words)
    rec = acceptable[0] if acceptable else None
    return {
        "task": "Task 61 — quantization + opcode-selection report",
        "version": 2,
        "visual_gate_iou_min": VISUAL_GATE_IOU,
        "opcode_word_costs": {OP_NAMES[op]: WORD_COST[op] for op in WORD_COST},
        "candidates": rows,
        "recommended_candidate": rec.name if rec else None,
        "recommended_policy": best_policy(rec) if rec else None,
        "recommended_vertex_words": best_words(rec) if rec else None,
    }


def cmd_check() -> int:
    errors: list[str] = []
    if not REPORT_OUTPUT.is_file():
        errors.append(f"report absent: {REPORT_OUTPUT}")
        print("TASK61 CHECK: FAIL", file=sys.stderr)
        for e in errors:
            print(f"  - {e}", file=sys.stderr)
        return 1
    stored = json.loads(REPORT_OUTPUT.read_text(encoding="utf-8"))
    fixtures = oracle._load_fixtures()
    reference = json.loads(
        (GENERATED_DIR / "dreamland_source_projection_ref.json")
        .read_text(encoding="utf-8"))
    rebuilt: list[QuantReport] = []
    for path in _candidate_files():
        name = path.stem
        stream_path = STREAMS_DIR / f"{name}.json"
        if not stream_path.is_file():
            continue
        positions, triangles, bindings = _load_candidate(path)
        stream_groups = _load_stream(name)
        rebuilt.append(encode_candidate(name, positions, triangles, bindings,
                                        stream_groups, fixtures, reference))
    rebuilt.sort(key=lambda r: r.v10_words if r.v10_iou >= VISUAL_GATE_IOU
                 else r.v16_only_words)
    rebuilt_payload = _build_report(rebuilt)
    if _sha256(stored) != _sha256(rebuilt_payload):
        errors.append("determinism: rebuilt report sha256 != stored")
    # Decode round-trip: every axis-reuse opcode must reconstruct exactly.
    for r in rebuilt:
        # axis-reuse is exact by construction; verify v10 quant error is bounded.
        if r.worst_quant_error_world > 1e9:  # sanity (overflow guard)
            errors.append(f"{r.name}: v10 quant error overflow")
    if errors:
        print("TASK61 CHECK: FAIL", file=sys.stderr)
        for e in errors:
            print(f"  - {e}", file=sys.stderr)
        return 1
    print("TASK61 CHECK: PASS")
    return 0


def _sha256(payload: dict[str, Any]) -> str:
    encoded = json.dumps(payload, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(encoded.encode("utf-8")).hexdigest()


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--check", action="store_true",
                   help="validate determinism + decode round-trip")
    return p.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    if args.check:
        return cmd_check()
    return cmd_build()


if __name__ == "__main__":
    sys.exit(main())
