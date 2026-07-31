#!/usr/bin/env python3
"""Task 60 — DS-native primitive topology compiler.

Turns Task 59's low-poly Dream Land candidates (and the welded source) into
topology designed for the Nintendo DS geometry engine: GL_TRIANGLE_STRIP and
GL_QUAD where beneficial, GL_TRIANGLES only for residual geometry. Host-only.

This is NOT Task 55-style lossless stripification of original topology — the
input mesh may be reordered aggressively within render-equivalence. The prize
is fewer TRANSFORMED vertex submissions (and fewer GX words), not prettier
topology.

Reuses the validated Task 56 winding model (_strip_extend: a triangle extends a
DS GL_TRIANGLE_STRIP iff it contains the active edge, the last two emitted
verts; the DS hardware flips winding per-triangle).

Strategy (per candidate, within compatible material/binding groups):
  1. Build the triangle adjacency graph.
  2. Greedily form GL_QUAD pairs: two triangles sharing exactly one edge with
     4 distinct verts -> one GL_QUAD (4 verts for 2 tris).
  3. Stripify the remaining triangles (longest-strip heuristic, active-edge
     tracked).
  4. Residual single triangles -> GL_TRIANGLES.

DS submission-cost model (per the plan):
  - transformed vertex submissions (the prize)
  - BEGIN count (one per primitive group)
  - GX words (COLOR + optional TEXCOORD + VERTEX16 per vertex, BEGIN per group)
  - texture/material transitions (binding changes between groups)

Usage
-----
  dreamland_primitive_compiler.py             # compile all candidates + report
  dreamland_primitive_compiler.py --check     # validate determinism + topology
"""

from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import sys
from dataclasses import dataclass, field
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

import dreamland_simplifier as simplifier
import task56_fighter_topology_census as t56

SCRIPTS_DIR = _paths.SCRIPTS_ROOT
REPO_ROOT = SCRIPTS_DIR.parent
GENERATED_DIR = SCRIPTS_DIR / "generated"
CANDIDATES_DIR = GENERATED_DIR / "candidates"
REPORT_OUTPUT = GENERATED_DIR / "dreamland_primitive_cost_report.json"
STREAMS_DIR = GENERATED_DIR / "primitive_streams"

# DS primitive type ids (libnds videoGL.h: GL_TRIANGLES=0, GL_QUAD=1, GL_TRIANGLE_STRIP=2).
PRIM_TRIANGLES = 0
PRIM_QUAD = 1
PRIM_STRIP = 2
PRIM_NAMES = {PRIM_TRIANGLES: "GL_TRIANGLES", PRIM_QUAD: "GL_QUAD",
              PRIM_STRIP: "GL_TRIANGLE_STRIP"}

# GX word costs (libnds geometry-engine command sizes).
WORDS_COLOR = 1        # one COLOR word per transformed vertex
WORDS_TEXCOORD = 1     # one TEXCOORD word per textured vertex
WORDS_VERTEX16 = 2     # VERTEX16 = XY + Z (two words)
WORDS_BEGIN = 1        # one BEGIN word per primitive group


@dataclass(frozen=True)
class Tri:
    v: tuple[int, int, int]
    binding: int
    run: int
    texture_epoch: int
    submit_class: int
    segment: int


@dataclass
class PrimGroup:
    prim: int            # PRIM_TRIANGLES / PRIM_QUAD / PRIM_STRIP
    verts: list[int]     # ordered vertex indices (strip/quad order)
    binding: int
    run: int
    texture_epoch: int
    submit_class: int
    segment: int


@dataclass
class CompiledMesh:
    name: str
    tri_count: int
    groups: list[PrimGroup]
    # For validation: the canonical triangle multiset each group expands to.
    expanded_tris: tuple[tuple[int, int, int], ...]


# ---------------------------------------------------------------------------
# Winding model (reused from Task 56)
# ---------------------------------------------------------------------------

def _strip_extend(active_edge, tri):
    """Task 56's active-edge strip extension. ``active_edge`` is an ordered
    pair (a0, a1) = the last two emitted verts in strip order. Returns
    (new_vertex, new_active_edge) or None. A triangle extends a DS strip iff it
    contains the active edge; the new active edge is (a1, new_vertex)."""
    return t56._strip_extend(active_edge, tri)


def _quad_from_pair(t_a: tuple[int, int, int],
                    t_b: tuple[int, int, int]) -> tuple[int, int, int, int] | None:
    """If t_a and t_b share exactly one edge and have 4 distinct verts, return
    them as a GL_QUAD vertex order (4 verts). GL_QUAD (v0,v1,v2,v3) expands to
    triangles (v0,v1,v2) and (v0,v2,v3); the shared edge (v0,v2) is the
    DIAGONAL. So the two shared verts go at v0,v2 and the two unshared verts
    (one from each source triangle) go at v1,v3, ordered so the expanded
    triangles match the source winding."""
    union = set(t_a) | set(t_b)
    if len(union) != 4:
        return None
    if len(set(t_a) & set(t_b)) != 2:
        return None
    source = sorted((_oriented_canon(t_a), _oriented_canon(t_b)))
    for quad in itertools.permutations(sorted(union)):
        expanded = sorted((
            _oriented_canon((quad[0], quad[1], quad[2])),
            _oriented_canon((quad[0], quad[2], quad[3])),
        ))
        if expanded == source:
            return quad
    return None  # no valid quad arrangement (degenerate)


def _same_winding(a: tuple[int, int, int], b: tuple[int, int, int]) -> bool:
    """True if triangle a is the same as b in the same cyclic orientation."""
    for off in range(3):
        if a == tuple(b[(i + off) % 3] for i in range(3)):
            return True
    return False


# ---------------------------------------------------------------------------
# Compiler: quads first, then strips, then residual triangles
# ---------------------------------------------------------------------------

def _triangle_groups_by_binding(tris: list[Tri]) -> list[list[Tri]]:
    """Partition triangles by their complete source render identity."""
    by_binding: dict[tuple[int, int, int, int, int], list[Tri]] = {}
    for t in tris:
        key = (
            t.segment, t.run, t.texture_epoch, t.submit_class, t.binding,
        )
        by_binding.setdefault(key, []).append(t)
    return [group for _, group in sorted(by_binding.items())]


def _compile_group(group: list[Tri]) -> list[PrimGroup]:
    """Compile one binding group into quads + strips + residual triangles."""
    if not group:
        return []
    first = group[0]
    remaining: set[int] = set(range(len(group)))
    groups: list[PrimGroup] = []

    # 1. Greedy quad formation: find pairs sharing exactly one edge (4 verts),
    #    preferring pairs that maximize quad count. Deterministic: iterate
    #    edges in sorted order, pair the first available two triangles.
    edge_to_tris: dict[tuple[int, int], list[int]] = {}
    for i, t in enumerate(group):
        tv = t.v
        for a, b in ((tv[0], tv[1]), (tv[1], tv[2]), (tv[0], tv[2])):
            edge_to_tris.setdefault((min(a, b), max(a, b)), []).append(i)

    quad_edges = sorted(e for e, ts in edge_to_tris.items() if len(ts) >= 2)
    used: set[int] = set()
    quads: list[PrimGroup] = []
    for edge in quad_edges:
        candidates = [i for i in edge_to_tris[edge] if i not in used]
        if len(candidates) < 2:
            continue
        i, j = candidates[0], candidates[1]
        q = _quad_from_pair(group[i].v, group[j].v)
        if q is None:
            continue
        quads.append(PrimGroup(
            PRIM_QUAD, list(q), first.binding, first.run,
            first.texture_epoch, first.submit_class, first.segment,
        ))
        used.add(i)
        used.add(j)
    groups.extend(quads)

    # 2. Stripify the remaining triangles (longest-strip, active-edge tracked).
    # Track the ORDERED active edge (last two emitted verts) so the strip's
    # vertex emission sequence reproduces the exact source triangles. A frozenset
    # active edge loses the ordering and drops triangles; the ordered pair keeps
    # the DS hardware's winding-flip semantics exact.
    strip_pool = [i for i in remaining if i not in used]
    pool_set = set(strip_pool)

    def _try_strip(start: int) -> tuple[list[int], list[int]]:
        """Greedily extend a strip from `start`; return (chain, emitted_verts).

        The strip's emitted vertex sequence is v0,v1,v2,...; triangle k is
        (v_k, v_{k+1}, v_{k+2}) with DS hardware winding flip. The first triangle
        is (v0,v1,v2); the active edge (last two emitted) is (v1,v2). We try all
        3 cyclic orderings of the first triangle as (v0,v1,v2) to maximize how
        many later triangles can extend (their shared edge must be (v1,v2))."""
        t0 = group[start].v
        best_chain: list[int] = []
        best_emit: list[int] = []
        # Three rotations: (t0[a], t0[b], t0[c]) as (v0, v1, v2).
        for v0, v1, v2 in ((t0[0], t0[1], t0[2]),
                           (t0[1], t0[2], t0[0]),
                           (t0[2], t0[0], t0[1])):
            chain = [start]
            emit = [v0, v1, v2]
            active = (v1, v2)  # ordered last-two emitted
            rem = pool_set - {start}
            while True:
                extended = False
                for c in sorted(rem):
                    tri_c = group[c].v
                    if active[0] in tri_c and active[1] in tri_c:
                        new_v = next(vv for vv in tri_c if vv not in active)
                        triangle_index = len(emit) - 2
                        emitted = (
                            (active[0], active[1], new_v)
                            if triangle_index % 2 == 0
                            else (active[1], active[0], new_v)
                        )
                        if not _same_winding(emitted, tri_c):
                            continue
                        chain.append(c)
                        emit.append(new_v)
                        active = (active[1], new_v)
                        rem = rem - {c}
                        extended = True
                        break
                if not extended:
                    break
            if len(chain) > len(best_chain):
                best_chain = chain[:]
                best_emit = emit[:]
        return best_chain, best_emit

    while pool_set:
        # Find the longest strip over all possible starts.
        overall_best_chain: list[int] = []
        overall_best_emit: list[int] = []
        for start in sorted(pool_set):
            chain, emit = _try_strip(start)
            if len(chain) > len(overall_best_chain):
                overall_best_chain = chain[:]
                overall_best_emit = emit[:]
        if not overall_best_chain:
            break
        groups.append(PrimGroup(
            PRIM_STRIP, overall_best_emit, first.binding, first.run,
            first.texture_epoch, first.submit_class, first.segment,
        ))
        for c in overall_best_chain:
            pool_set.discard(c)

    # 3. Residual single triangles -> GL_TRIANGLES (3 verts each).
    for i in sorted(pool_set):
        groups.append(PrimGroup(
            PRIM_TRIANGLES, list(group[i].v), first.binding, first.run,
            first.texture_epoch, first.submit_class, first.segment,
        ))

    return groups


# ---------------------------------------------------------------------------
# Expansion (for validation): group -> canonical triangles
# ---------------------------------------------------------------------------

def expand_group(g: PrimGroup) -> list[tuple[int, int, int]]:
    """Expand a primitive group back to its source triangles."""
    if g.prim == PRIM_TRIANGLES:
        return [tuple(g.verts)]
    if g.prim == PRIM_QUAD:
        v = g.verts
        # GL_QUAD: (v0,v1,v2) and (v0,v2,v3)
        return [(v[0], v[1], v[2]), (v[0], v[2], v[3])]
    if g.prim == PRIM_STRIP:
        v = g.verts
        tris = []
        for i in range(len(v) - 2):
            tris.append(
                (v[i], v[i + 1], v[i + 2])
                if i % 2 == 0
                else (v[i + 1], v[i], v[i + 2])
            )
        return tris
    return []


def _canon(tri: tuple[int, int, int]) -> tuple[int, int, int]:
    """Canonical orientation-independent triangle key (sorted verts)."""
    return tuple(sorted(tri))


def _oriented_canon(tri: tuple[int, int, int]) -> tuple[int, int, int]:
    """Canonical cyclic rotation that preserves triangle orientation."""
    return min(tuple(tri[(i + off) % 3] for i in range(3)) for off in range(3))


# ---------------------------------------------------------------------------
# Cost model
# ---------------------------------------------------------------------------

@dataclass
class CostReport:
    name: str
    tri_count: int
    group_count: int
    quad_groups: int
    strip_groups: int
    tri_groups: int
    submitted_verts: int        # transformed vertex submissions (the prize)
    begins: int                 # one per group
    gx_words: int               # COLOR + TEXCOORD + VERTEX16 + BEGIN
    material_transitions: int   # binding changes between consecutive groups
    reduction_vs_source_pct: float


def estimate_cost(name: str, groups: list[PrimGroup], tri_count: int,
                  source_submitted: int) -> CostReport:
    submitted = sum(len(g.verts) for g in groups)
    begins = len(groups)
    # GX words: per vertex = COLOR(1) + TEXCOORD(1) + VERTEX16(2) = 4; per
    # group = BEGIN(1). (Stage is textured; assume TEXCOORD present.)
    per_vertex_words = WORDS_COLOR + WORDS_TEXCOORD + WORDS_VERTEX16
    gx_words = submitted * per_vertex_words + begins * WORDS_BEGIN
    # Material transitions: count binding changes in emit order.
    transitions = 0
    prev_binding = None
    for g in groups:
        key = (g.run, g.texture_epoch, g.submit_class, g.binding)
        if prev_binding is not None and key != prev_binding:
            transitions += 1
        prev_binding = key
    quad_g = sum(1 for g in groups if g.prim == PRIM_QUAD)
    strip_g = sum(1 for g in groups if g.prim == PRIM_STRIP)
    tri_g = sum(1 for g in groups if g.prim == PRIM_TRIANGLES)
    reduction = 100.0 * (1.0 - submitted / max(1, source_submitted))
    return CostReport(name, tri_count, begins, quad_g, strip_g, tri_g,
                      submitted, begins, gx_words, transitions, reduction)


# ---------------------------------------------------------------------------
# Candidate compilation
# ---------------------------------------------------------------------------

def _load_candidate_mesh(path: Path) -> tuple[list[tuple[float, float, float]], list[Tri]]:
    """Load a Task 59 candidate IR into (positions, triangles)."""
    ir = json.loads(path.read_text(encoding="utf-8"))
    positions = [(v["world_x_f"], v["world_y_f"], v["world_z_f"])
                 for v in ir["world_vertices"]]
    tris = [Tri(
                (t["v0"], t["v1"], t["v2"]),
                t["binding_index"],
                t["run_index"],
                t["texture_epoch"],
                t["submit_class"],
                t["source_segment"],
            )
            for t in ir["triangles"]]
    return positions, tris


def compile_candidate(name: str, positions: list, tris: list[Tri]) -> CompiledMesh:
    """Compile one candidate's triangles into DS-native primitive groups."""
    groups: list[PrimGroup] = []
    for binding_group in _triangle_groups_by_binding(tris):
        groups.extend(_compile_group(binding_group))
    expanded: list[tuple[int, int, int]] = []
    for g in groups:
        expanded.extend(expand_group(g))
    return CompiledMesh(name, len(tris), groups, tuple(expanded))


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

def validate_compiled(cm: CompiledMesh, source_tris: list[Tri]) -> list[str]:
    """Return a list of error strings (empty = valid)."""
    errors: list[str] = []
    # 1. Oriented triangle multiset equivalence: every source triangle appears
    #    exactly once with the same winding.
    src_canon = sorted(_oriented_canon(t.v) for t in source_tris)
    exp_canon = sorted(_oriented_canon(t) for t in cm.expanded_tris)
    if src_canon != exp_canon:
        src_count: dict = {}
        for t in src_canon:
            src_count[t] = src_count.get(t, 0) + 1
        exp_count: dict = {}
        for t in exp_canon:
            exp_count[t] = exp_count.get(t, 0) + 1
        missing = sum(max(0, src_count.get(k, 0) - exp_count.get(k, 0))
                      for k in src_count)
        extra = sum(max(0, exp_count.get(k, 0) - src_count.get(k, 0))
                    for k in exp_count)
        errors.append(f"{cm.name}: triangle multiset mismatch "
                      f"(missing={missing}, extra={extra})")
    # 2. No invalid vertex indices in groups.
    for gi, g in enumerate(cm.groups):
        for v in g.verts:
            if v < 0:
                errors.append(f"{cm.name} group {gi}: negative vertex {v}")
    # 3. Strip parity: each strip group with k verts must expand to k-2 tris.
    for gi, g in enumerate(cm.groups):
        if g.prim == PRIM_STRIP:
            expected = len(g.verts) - 2
            actual = len(expand_group(g))
            if expected != actual:
                errors.append(f"{cm.name} strip {gi}: parity {actual} != {expected}")
        if g.prim == PRIM_QUAD and len(g.verts) != 4:
            errors.append(f"{cm.name} quad {gi}: {len(g.verts)} verts != 4")
    return errors


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

SOURCE_SUBMITTED_BASELINE = 525  # source_welded GL_TRIANGLES (Task 59 Pareto)


def cmd_build() -> int:
    STREAMS_DIR.mkdir(parents=True, exist_ok=True)
    reports: list[CostReport] = []
    compiled_all: dict[str, CompiledMesh] = {}

    # Compile every Task 59 candidate + the welded source.
    pareto = json.loads(
        (GENERATED_DIR / "dreamland_candidate_pareto.json")
        .read_text(encoding="utf-8")
    )
    candidate_names = {row["name"] for row in pareto["candidates"]}
    candidate_files = sorted(
        path for path in CANDIDATES_DIR.glob("*.json")
        if path.stem in candidate_names
    )
    for path in candidate_files:
        name = path.stem
        positions, tris = _load_candidate_mesh(path)
        cm = compile_candidate(name, positions, tris)
        compiled_all[name] = cm
        # Emit primitive-stream IR.
        stream_ir = {
            "task": "Task 60 — DS primitive stream",
            "candidate": name,
            "groups": [
                {
                    "prim": PRIM_NAMES[g.prim],
                    "verts": g.verts,
                    "binding": g.binding,
                    "run_index": g.run,
                    "texture_epoch": g.texture_epoch,
                    "submit_class": g.submit_class,
                    "source_segment": g.segment,
                }
                for g in cm.groups
            ],
        }
        (STREAMS_DIR / f"{name}.json").write_text(
            json.dumps(stream_ir, indent=2, sort_keys=True) + "\n",
            encoding="utf-8")
        reports.append(estimate_cost(name, cm.groups, cm.tri_count,
                                     SOURCE_SUBMITTED_BASELINE))

    # Validation across all.
    val_errors: list[str] = []
    for path in candidate_files:
        name = path.stem
        _, tris = _load_candidate_mesh(path)
        val_errors.extend(validate_compiled(compiled_all[name], tris))
    if val_errors:
        print("TASK60 VALIDATION: FAIL", file=sys.stderr)
        for err in val_errors[:10]:
            print(f"  - {err}", file=sys.stderr)
        return 1

    reports.sort(key=lambda r: r.submitted_verts)
    report = _build_report(reports)
    REPORT_OUTPUT.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"TASK60: compiled {len(reports)} candidates -> {STREAMS_DIR}")
    print(f"TASK60: report -> {REPORT_OUTPUT.name}")
    print(f"TASK60: source baseline = {SOURCE_SUBMITTED_BASELINE} submitted verts")
    iou_by_name = {c["name"]: c["worst_region_iou"]
                   for c in report["candidates"]}
    accept_by_name = {c["name"]: c["visually_acceptable"]
                      for c in report["candidates"]}
    material_by_name = {c["name"]: c["material_qualified"]
                        for c in report["candidates"]}
    print(f"{'candidate':<16} {'tris':>4} {'quads':>5} {'strips':>6} {'tris_g':>6} "
          f"{'sub_verts':>9} {'IoU':>5} {'reduce':>7}")
    for r in reports:
        flag = "OK" if (accept_by_name.get(r.name, False) and
                         material_by_name.get(r.name, False)) else "  "
        print(f"{r.name:<16} {r.tri_count:>4} {r.quad_groups:>5} {r.strip_groups:>6} "
              f"{r.tri_groups:>6} {r.submitted_verts:>9} "
              f"{iou_by_name.get(r.name, 0.0):>5.3f} "
              f"{r.reduction_vs_source_pct:>6.1f}% {flag}")
    print(f"TASK60: target <=200 met = {report['target_200_met']}, "
          f"stretch <=150 met = {report['stretch_150_met']}")
    print(f"TASK60: recommended = {report['recommended_candidate']}")
    return 0


def cmd_check() -> int:
    errors: list[str] = []
    if not REPORT_OUTPUT.is_file():
        errors.append(f"report absent: {REPORT_OUTPUT}")
        print("TASK60 CHECK: FAIL", file=sys.stderr)
        for e in errors:
            print(f"  - {e}", file=sys.stderr)
        return 1
    stored = json.loads(REPORT_OUTPUT.read_text(encoding="utf-8"))

    # Rebuild and compare.
    rebuilt_reports: list[CostReport] = []
    compiled_all: dict[str, CompiledMesh] = {}
    val_errors: list[str] = []
    stored_names = {row["name"] for row in stored["candidates"]}
    for path in sorted(
        path for path in CANDIDATES_DIR.glob("*.json")
        if path.stem in stored_names
    ):
        name = path.stem
        positions, tris = _load_candidate_mesh(path)
        cm = compile_candidate(name, positions, tris)
        compiled_all[name] = cm
        rebuilt_reports.append(estimate_cost(name, cm.groups, cm.tri_count,
                                             SOURCE_SUBMITTED_BASELINE))
        val_errors.extend(validate_compiled(cm, tris))
    if val_errors:
        errors.append(f"topology validation failed: {len(val_errors)} errors")
    rebuilt_reports.sort(key=lambda r: r.submitted_verts)
    rebuilt_payload = _build_report(rebuilt_reports)
    if _sha256(stored) != _sha256(rebuilt_payload):
        errors.append("determinism: rebuilt report sha256 != stored")
    if errors:
        print("TASK60 CHECK: FAIL", file=sys.stderr)
        for e in errors:
            print(f"  - {e}", file=sys.stderr)
        return 1
    print("TASK60 CHECK: PASS")
    return 0


def _load_task59_iou() -> dict[str, float]:
    """Load each Task 59 candidate's worst-case region IoU (geometry-only, so
    identical here — Task 60 doesn't change geometry). Used to gate the
    recommendation: only visually-acceptable (IoU >= 0.95) candidates may be
    recommended. Candidates absent from Task 59's Pareto (e.g. c120, generated
    by Task 60's fine-grained sweep) are evaluated live."""
    import dreamland_camera_oracle as oracle
    import dreamland_simplifier as simpl
    pareto_path = GENERATED_DIR / "dreamland_candidate_pareto.json"
    iou: dict[str, float] = {}
    if pareto_path.is_file():
        for c in json.loads(pareto_path.read_text(encoding="utf-8"))["candidates"]:
            iou[c["name"]] = c.get("worst_region_iou", 0.0)
    return iou


def _live_iou(name: str) -> float | None:
    """Evaluate a candidate's IoU live (for candidates not in Task 59's Pareto).
    Returns None if the candidate file isn't present."""
    path = CANDIDATES_DIR / f"{name}.json"
    if not path.is_file():
        return None
    import dreamland_camera_oracle as oracle
    mesh = oracle.load_mesh(path)
    fixtures = oracle._load_fixtures()
    ref = json.loads((GENERATED_DIR / "dreamland_source_projection_ref.json")
                     .read_text(encoding="utf-8"))
    ref_projections = {f["name"]: oracle._load_ref_projection(f)
                       for f in ref["fixtures"]}
    protected = oracle.detect_protected_components(mesh)
    worst = 1.0
    for fx in fixtures:
        cp = oracle.project_mesh(mesh, fx, protected)
        m = oracle.displacement_metrics(cp, ref_projections[fx.name])
        worst = min(worst, m["region_iou"])
    return worst


def candidate_material_qualified(name: str) -> bool:
    """True only when every emitted vertex/triangle retains source render identity."""
    path = CANDIDATES_DIR / f"{name}.json"
    if not path.is_file():
        return False
    ir = json.loads(path.read_text(encoding="utf-8"))
    vertices = ir.get("world_vertices", [])
    triangles = ir.get("triangles", [])
    if not vertices or not triangles or not ir.get("material_qualified", False):
        return False
    if not all(
        v.get("material_source_count", 0) > 0
        and v.get("run_index", -1) >= 0
        and v.get("texture_epoch", -1) >= 0
        and v.get("submit_class", -1) >= 0
        and all(field in v for field in (
            "local_x", "local_y", "local_z", "s", "t", "rgba",
        ))
        for v in vertices
    ):
        return False
    for triangle in triangles:
        key = (
            triangle.get("run_index", -1),
            triangle.get("texture_epoch", -1),
            triangle.get("submit_class", -1),
            triangle.get("binding_index", -1),
        )
        if min(key) < 0 or triangle.get("source_segment", -1) < 0:
            return False
        for field in ("v0", "v1", "v2"):
            index = triangle.get(field, -1)
            if index < 0 or index >= len(vertices):
                return False
            vertex = vertices[index]
            if (
                vertex["run_index"],
                vertex["texture_epoch"],
                vertex["submit_class"],
                vertex["binding_index"],
            ) != key:
                return False
    return True


def _build_report(reports: list[CostReport]) -> dict[str, Any]:
    task59_iou = _load_task59_iou()
    # For candidates not in Task 59's Pareto, evaluate IoU live.
    cand_iou: dict[str, float] = {}
    for r in reports:
        if r.name in task59_iou:
            cand_iou[r.name] = task59_iou[r.name]
        else:
            live = _live_iou(r.name)
            cand_iou[r.name] = live if live is not None else 0.0
    VISUAL_GATE = 0.95
    payload = {
        "task": "Task 60 — DS primitive cost report",
        "version": 2,
        "source_submitted_verts_baseline": SOURCE_SUBMITTED_BASELINE,
        "visual_gate_iou_min": VISUAL_GATE,
        "cost_model": {
            "per_vertex_gx_words": WORDS_COLOR + WORDS_TEXCOORD + WORDS_VERTEX16,
            "begin_words": WORDS_BEGIN,
            "prim_ids": {v: k for k, v in PRIM_NAMES.items()},
        },
        "candidates": [
            {
                "name": r.name, "tris": r.tri_count, "groups": r.group_count,
                "quad_groups": r.quad_groups, "strip_groups": r.strip_groups,
                "tri_groups": r.tri_groups, "submitted_verts": r.submitted_verts,
                "begins": r.begins, "gx_words": r.gx_words,
                "material_transitions": r.material_transitions,
                "reduction_vs_source_pct": round(r.reduction_vs_source_pct, 1),
                "worst_region_iou": round(cand_iou.get(r.name, 0.0), 4),
                "visually_acceptable": cand_iou.get(r.name, 0.0) >= VISUAL_GATE,
                "material_qualified": candidate_material_qualified(r.name),
            }
            for r in reports
        ],
    }
    # Recommended = cheapest visually-acceptable candidate (the plan's "select
    # the cheapest candidate that still looks acceptable"). If none acceptable,
    # note that.
    acceptable = [r for r in reports
                  if cand_iou.get(r.name, 0.0) >= VISUAL_GATE
                  and candidate_material_qualified(r.name)]
    acceptable.sort(key=lambda r: r.submitted_verts)
    if acceptable:
        payload["recommended_candidate"] = acceptable[0].name
        payload["recommended_submitted_verts"] = acceptable[0].submitted_verts
    else:
        payload["recommended_candidate"] = None
    # Targets apply only to visually-acceptable candidates.
    payload["target_200_met"] = any(
        r.submitted_verts <= 200 for r in acceptable)
    payload["stretch_150_met"] = any(
        r.submitted_verts <= 150 for r in acceptable)
    return payload


def _sha256(payload: dict[str, Any]) -> str:
    encoded = json.dumps(payload, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(encoded.encode("utf-8")).hexdigest()


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--check", action="store_true",
                   help="validate determinism + topology equivalence")
    return p.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    if args.check:
        return cmd_check()
    return cmd_build()


if __name__ == "__main__":
    sys.exit(main())
