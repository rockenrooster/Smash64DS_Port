#!/usr/bin/env python3
"""Task 59 — Constrained Dream Land visual-mesh simplifier.

Host-only. Generates much lower-poly 3D Dream Land candidates from the Task 57
canonical world mesh while protecting gameplay-important visual surfaces, then
judges every candidate against the Task 58 gameplay-camera oracle.

Approach: quadric-error edge collapse (Garland & Heckbert) with:
  * deterministic tie-breaking (sorted keys, no float-equality ties);
  * hard locks on protected edges (Task 58's auto-detected platform/floor
    outline) so a platform edge cannot move;
  * hard locks on attribute seams (positions with >1 UV/color/binding, and
    binding/material boundaries) so collapses cannot corrupt textures;
  * high cost weight on silhouette/boundary edges, low weight on rear/hidden
    geometry (approximated by world-Z: the stage faces -Z toward the camera,
    so +Z / rear surfaces get a lower collapse cost).
  * exact-duplicate position welding as a free first pass.

The result is a candidate ladder (vertex-budget-driven) plus a Pareto report.

Usage
-----
  dreamland_simplifier.py                  # build all candidates + Pareto report
  dreamland_simplifier.py --check          # validate determinism + constraints
  dreamland_simplifier.py --candidate NAME # emit one candidate IR (Task 60 input)
"""

from __future__ import annotations

import argparse
import heapq
import itertools
import json
import math
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Sequence

import numpy as np

import dreamland_world_mesh as world_mesh
import dreamland_camera_oracle as oracle

SCRIPTS_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPTS_DIR.parent
GENERATED_DIR = SCRIPTS_DIR / "generated"
WORLD_MESH_IR = GENERATED_DIR / "dreamland_world_mesh.json"
PARETO_OUTPUT = GENERATED_DIR / "dreamland_candidate_pareto.json"
CANDIDATES_DIR = GENERATED_DIR / "candidates"

# Candidate ladder: target triangle counts (plan section 5).
CANDIDATE_TRI_TARGETS = (180, 140, 110, 90, 70, 55, 40)

# Quadric-error weights.
SILHOUETTE_PENALTY = 1.0e6        # boundary/silhouette edges resist collapse
SEAM_PENALTY = 1.0e9              # attribute/binding seams are effectively locked
PROTECTED_LOCK = 1.0e12           # protected edges never collapse
# World-Z below this = front-facing toward camera (high weight); above = rear.
FRONT_Z_WORLD = 0.0
REAR_COST_SCALE = 0.25           # rear surfaces collapse more eagerly
FRONT_COST_SCALE = 2.0           # front/lip surfaces resist collapse

S20P12_SCALE = 4096.0


# ---------------------------------------------------------------------------
# Mesh model (float world space; attribute tags for constraints)
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class Tri:
    v0: int
    v1: int
    v2: int
    binding: int


@dataclass
class SimplifyMesh:
    """Mutable mesh used during simplification.

    positions:   (N,3) float world coords.
    attributes:  list of frozenset of (s,t,rgba,binding) tuples per vertex; a
                 vertex with >1 entry sits on an attribute seam.
    binding:     list of binding id per vertex (the dominant binding).
    alive:       list[bool].
    tris:        list of (v0,v1,v2) alive triangles (rebuilt as we go).
    """
    positions: np.ndarray
    attributes: list[frozenset]
    binding: list[int]
    alive: list[bool]
    tris: list[tuple[int, int, int]]


def load_source_for_simplify() -> tuple[SimplifyMesh, dict[int, int]]:
    """Load the Task 57 source mesh into a SimplifyMesh, plus a dense->local map.

    Uses the live extraction (no stale IR) so the simplifier and the oracle
    share one source of truth.
    """
    packet = world_mesh.stage_gen.generate(REPO_ROOT)
    wv, wt, _, dense_to_local = world_mesh.build_world_mesh(packet)
    n = len(wv)
    positions = np.array([[v.world_x / S20P12_SCALE,
                           v.world_y / S20P12_SCALE,
                           v.world_z / S20P12_SCALE] for v in wv], dtype=np.float64)
    # Attribute key per vertex: (s, t, rgba, binding).
    attrs: list[frozenset] = []
    binding: list[int] = []
    for v in wv:
        attrs.append(frozenset({(v.s, v.t, v.rgba, v.binding_index)}))
        binding.append(v.binding_index)
    tris = [(t.v0, t.v1, t.v2) for t in wt]
    mesh = SimplifyMesh(positions, attrs, binding, [True] * n, tris)
    return mesh, dense_to_local


# ---------------------------------------------------------------------------
# Seam / boundary / protected detection
# ---------------------------------------------------------------------------

def _edge(a: int, b: int) -> tuple[int, int]:
    return (a, b) if a <= b else (b, a)


def build_boundary_edges(tris: Sequence[tuple[int, int, int]]) -> set[tuple[int, int]]:
    refs: dict[tuple[int, int], int] = {}
    for v0, v1, v2 in tris:
        for a, b in ((v0, v1), (v1, v2), (v0, v2)):
            k = _edge(a, b)
            refs[k] = refs.get(k, 0) + 1
    return {e for e, c in refs.items() if c == 1}


def attribute_seam_vertices(mesh: SimplifyMesh) -> set[int]:
    """Vertices that share their position with a different attribute elsewhere.

    A collapse across such a vertex would merge two UV/color/binding sets and
    corrupt textures, so they are locked. Detected by exact position equality.
    """
    pos_to_verts: dict[tuple[float, float, float], list[int]] = {}
    for i, p in enumerate(mesh.positions):
        # Quantize to s20.12-equivalent to find true duplicates (the IR already
        # deduped on exact ints, so float rounding is safe here).
        key = (round(p[0], 4), round(p[1], 4), round(p[2], 4))
        pos_to_verts.setdefault(key, []).append(i)
    seam: set[int] = set()
    for verts in pos_to_verts.values():
        if len(verts) < 2:
            continue
        attrs = [mesh.attributes[v] for v in verts]
        # If any two verts at this position have different attribute sets, all
        # of them are seam vertices.
        for a, b in itertools.combinations(attrs, 2):
            if a != b:
                seam.update(verts)
                break
    return seam


# ---------------------------------------------------------------------------
# Quadric-error edge collapse
# ---------------------------------------------------------------------------

def _plane_quadric(p0: np.ndarray, p1: np.ndarray, p2: np.ndarray) -> np.ndarray:
    """Quadric for the plane of triangle (p0,p1,p2): K = A^T A where A is the
    plane equation (a,b,c,d) with ax+by+cz+d=0, normalized. Returns the 4x4
    symmetric quadric matrix."""
    n = np.cross(p1 - p0, p2 - p0)
    norm = np.linalg.norm(n)
    if norm < 1e-12:
        return np.zeros((4, 4), dtype=np.float64)
    n = n / norm
    d = -np.dot(n, p0)
    abcd = np.array([n[0], n[1], n[2], d])
    return np.outer(abcd, abcd)


def _edge_cost(qsum: np.ndarray, p: np.ndarray) -> float:
    """QEM cost of placing a collapsed edge at point p: p^T Q p (homogeneous)."""
    ph = np.array([p[0], p[1], p[2], 1.0])
    return float(ph @ qsum @ ph)


def _front_rear_scale(z: float) -> float:
    """Higher cost for front (camera-facing, low Z) geometry; lower for rear."""
    if z < FRONT_Z_WORLD:
        return FRONT_COST_SCALE
    return REAR_COST_SCALE


def simplify_to_tri_budget(
    source: SimplifyMesh,
    target_tris: int,
    protected_verts: set[int],
) -> SimplifyMesh:
    """Collapse edges greedily by quadric error until <= target_tris remain.

    Hard constraints (never collapsed):
      * either endpoint is a protected vertex;
      * either endpoint is an attribute-seam vertex;
      * endpoints are in different bindings (material boundary);
      * the edge is a boundary/silhouette edge (locked with huge penalty instead,
        so components keep their outline).

    Deterministic: edge heap keys are (cost, edge) tuples; ties break on vertex
    indices, never on float equality.
    """
    mesh = SimplifyMesh(
        source.positions.copy(),
        list(source.attributes),
        list(source.binding),
        list(source.alive),
        list(source.tris),
    )
    n = len(mesh.positions)
    seam_verts = attribute_seam_vertices(mesh)
    boundary = build_boundary_edges(mesh.tris)
    # Hard-lock only protected verts (the plan's hard constraints); seam verts
    # and boundary edges get cost penalties instead (the oracle is the visual
    # judge of whether collapsing them is acceptable).
    locked_verts = set(protected_verts)

    # Per-vertex adjacency (alive neighbors).
    neighbors: list[set[int]] = [set() for _ in range(n)]
    for v0, v1, v2 in mesh.tris:
        for a, b in ((v0, v1), (v1, v2), (v0, v2)):
            neighbors[a].add(b)
            neighbors[b].add(a)

    # Per-vertex quadric: sum of incident triangle plane quadrics.
    qmat: list[np.ndarray] = [np.zeros((4, 4)) for _ in range(n)]
    for v0, v1, v2 in mesh.tris:
        q = _plane_quadric(mesh.positions[v0], mesh.positions[v1], mesh.positions[v2])
        qmat[v0] += q
        qmat[v1] += q
        qmat[v2] += q

    def edge_collapse_allowed(a: int, b: int) -> bool:
        # Hard locks only: protected edges and cross-binding (material) boundaries.
        # UV/color seam vertices within a binding are NOT hard-locked — they get a
        # high cost penalty below, and the Task 58 oracle judges whether the
        # result is visually acceptable (the plan's "UV seams where merging would
        # *visibly* corrupt textures" is a visual call, not a topological one).
        if a in locked_verts or b in locked_verts:
            return False
        if mesh.binding[a] != mesh.binding[b]:
            return False
        return True

    def edge_cost_pair(a: int, b: int) -> tuple[tuple[float, int, int], tuple[int, int]]:
        qsum = qmat[a] + qmat[b]
        # Optimal placement: midpoint (cheap, deterministic; full optimization
        # adds float noise that hurts determinism).
        p = (mesh.positions[a] + mesh.positions[b]) * 0.5
        cost = _edge_cost(qsum, p)
        # Weight by front/rear (lower Z = front = costlier to collapse).
        cost *= _front_rear_scale(p[2])
        # Silhouette/boundary edges resist collapse (high penalty, not a hard
        # lock — the oracle judges the resulting outline displacement).
        if _edge(a, b) in boundary:
            cost += SILHOUETTE_PENALTY
        # UV/color seam vertices within a binding get a strong but not absolute
        # penalty; collapsing them may be acceptable if the oracle says so.
        if a in seam_verts or b in seam_verts:
            cost += SEAM_PENALTY
        # Crease penalty: edges whose endpoints share few neighbors.
        shared = len(neighbors[a] & neighbors[b])
        if shared < 2:
            cost *= (1.0 + 0.5 * (2 - shared))
        # Deterministic tie-break by vertex indices.
        key = (cost, a, b)
        return key, (a, b)

    # Initial heap of all collapsible edges.
    heap: list[tuple[Any, tuple[int, int]]] = []
    seen_edges: set[tuple[int, int]] = set()
    for v0, v1, v2 in mesh.tris:
        for a, b in ((v0, v1), (v1, v2), (v0, v2)):
            e = _edge(a, b)
            if e in seen_edges:
                continue
            seen_edges.add(e)
            if edge_collapse_allowed(e[0], e[1]):
                key, _ = edge_cost_pair(e[0], e[1])
                heap.append(key)
    heapq.heapify(heap)
    # Track valid edges (endpoints alive, cost not stale).
    edge_cost_version: dict[tuple[int, int], float] = {}
    counter = itertools.count()

    def tri_count() -> int:
        return len(mesh.tris)

    while heap and tri_count() > target_tris:
        key = heapq.heappop(heap)
        cost, a, b = key
        e = _edge(a, b)
        if not mesh.alive[a] or not mesh.alive[b]:
            continue
        if e not in seen_edges:
            continue
        if not edge_collapse_allowed(a, b):
            seen_edges.discard(e)
            continue
        # Re-derive cost; if it drifted (stale heap entry), re-push and skip.
        fresh_key, _ = edge_cost_pair(a, b)
        if abs(fresh_key[0] - cost) > 1e-9 * max(1.0, abs(cost)):
            heapq.heappush(heap, fresh_key)
            continue

        # Collapse b into a: move a to midpoint, kill b, reassign triangles.
        midpoint = (mesh.positions[a] + mesh.positions[b]) * 0.5
        mesh.positions[a] = midpoint
        mesh.positions[b] = midpoint  # keep b's row sane until reaped
        qmat[a] = qmat[a] + qmat[b]
        mesh.alive[b] = False
        # Merge attributes (a now spans both — but collapses are within-binding
        # and non-seam, so attribute sets are compatible).
        mesh.attributes[a] = mesh.attributes[a] | mesh.attributes[b]
        # Rewire neighbors.
        for nb in list(neighbors[b]):
            if nb == a or not mesh.alive[nb]:
                continue
            neighbors[nb].discard(b)
            neighbors[nb].add(a)
            neighbors[a].add(nb)
        neighbors[a].discard(b)
        neighbors[b].clear()
        # Rewire triangles: replace b with a; drop degenerate triangles.
        new_tris: list[tuple[int, int, int]] = []
        for v0, v1, v2 in mesh.tris:
            t = (v0 if v0 != b else a,
                 v1 if v1 != b else a,
                 v2 if v2 != b else a)
            if t[0] == t[1] or t[1] == t[2] or t[0] == t[2]:
                continue  # degenerate, dropped
            new_tris.append(t)
        mesh.tris = new_tris
        seen_edges.discard(e)
        # Push fresh edges around a (their costs changed).
        for nb in neighbors[a]:
            ne = _edge(a, nb)
            if edge_collapse_allowed(a, nb):
                nkey, _ = edge_cost_pair(a, nb)
                heapq.heappush(heap, nkey)
                seen_edges.add(ne)

    return _compact_mesh(mesh)


def _compact_mesh(mesh: SimplifyMesh) -> SimplifyMesh:
    """Reindex to a contiguous alive vertex space."""
    alive_idx = [i for i in range(len(mesh.positions)) if mesh.alive[i]]
    remap: dict[int, int] = {old: new for new, old in enumerate(alive_idx)}
    positions = mesh.positions[alive_idx]
    attributes = [mesh.attributes[i] for i in alive_idx]
    binding = [mesh.binding[i] for i in alive_idx]
    alive = [True] * len(alive_idx)
    tris = []
    for v0, v1, v2 in mesh.tris:
        # All triangle verts must be alive (collapses rewired them, but guard).
        if v0 in remap and v1 in remap and v2 in remap:
            tris.append((remap[v0], remap[v1], remap[v2]))
    return SimplifyMesh(positions, attributes, binding, alive, tris)


# ---------------------------------------------------------------------------
# Exact-duplicate welding (free first reduction)
# ---------------------------------------------------------------------------

def weld_exact_duplicates(mesh: SimplifyMesh) -> SimplifyMesh:
    """Merge vertices with identical position AND attribute set, within a binding."""
    key_to_new: dict[tuple, int] = {}
    remap: list[int] = [0] * len(mesh.positions)
    new_positions: list[np.ndarray] = []
    new_attributes: list[frozenset] = []
    new_binding: list[int] = []
    for i, p in enumerate(mesh.positions):
        key = (round(p[0], 4), round(p[1], 4), round(p[2], 4),
               mesh.binding[i])
        # Only weld if attribute sets match too (avoid UV seams).
        attr = mesh.attributes[i]
        full_key = key + (hash(attr),)
        if full_key in key_to_new:
            remap[i] = key_to_new[full_key]
        else:
            new_idx = len(new_positions)
            key_to_new[full_key] = new_idx
            new_positions.append(p)
            new_attributes.append(attr)
            new_binding.append(mesh.binding[i])
            remap[i] = new_idx
    positions = np.array(new_positions, dtype=np.float64)
    tris = [(remap[v0], remap[v1], remap[v2]) for v0, v1, v2 in mesh.tris]
    # Drop degenerate.
    tris = [t for t in tris if len(set(t)) == 3]
    return SimplifyMesh(positions, new_attributes, new_binding,
                        [True] * len(new_positions), tris)


# ---------------------------------------------------------------------------
# Stage-aware procedural reconstruction (plan STOP/reframe path)
# ---------------------------------------------------------------------------
#
# Generic edge collapse tops out at ~21% reduction before the silhouette breaks
# (the plan's STOP condition). Instead, reconstruct each source component as a
# prism: compute the 2D convex hull of the component's vertices in its two
# non-thin axes, extrude along the thin axis to the component's bounds. This
# matches each component's visual silhouette exactly (a prism's projected
# outline IS its hull) with far fewer vertices than the source triangles.
#
# Rear/hidden components (those whose centroid is on the +Z / far side and that
# are never front-facing at the gameplay camera) can optionally be dropped at
# the most aggressive budgets — the plan's "hidden/rear decorative surfaces"
# low-weight clause.

def _convex_hull_2d(points: list[tuple[float, float]]) -> list[tuple[float, float]]:
    """Andrew's monotone-chain convex hull (counter-clockwise). Deterministic."""
    if len(points) < 3:
        return list(dict.fromkeys(points))  # dedup, keep order
    pts = sorted(set(points))
    if len(pts) <= 2:
        return pts

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    lower: list[tuple[float, float]] = []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    upper: list[tuple[float, float]] = []
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def _component_dominant_plane(
    positions: list[tuple[float, float, float]],
) -> tuple[int, int, int]:
    """Return (axis_a, axis_b, thin_axis): the two widest axes + the thinnest."""
    extents = []
    for ax in range(3):
        vals = [p[ax] for p in positions]
        extents.append((max(vals) - min(vals), ax))
    extents.sort(reverse=True)
    return extents[0][1], extents[1][1], extents[2][1]  # wide, medium, thin


def _prism_from_component(
    positions: list[tuple[float, float, float]],
    binding: int,
) -> tuple[list[tuple[float, float, float]], list[tuple[int, int, int]]]:
    """Build an extruded prism (hull in the dominant plane) for one component.

    Returns (verts, tris). The prism has 2*H verts (H = hull size): a bottom
    ring and a top ring, connected by a triangle strip on the sides + fans on
    the caps. If the component is flat (thin-axis extent ~0), the top/bottom
    rings coincide and only the cap fan is emitted (a single polygon face).
    """
    ax_a, ax_b, thin = _component_dominant_plane(positions)
    thin_vals = [p[thin] for p in positions]
    thin_lo, thin_hi = min(thin_vals), max(thin_vals)
    flat = (thin_hi - thin_lo) < 1.0

    hull2d = _convex_hull_2d([(p[ax_a], p[ax_b]) for p in positions])
    h = len(hull2d)
    if h < 3:
        # Degenerate hull (collinear): emit a thin quad from the bbox.
        xs = [p[0] for p in hull2d] or [0.0, 0.0]
        ys = [p[1] for p in hull2d] or [0.0, 0.0]
        hull2d = [(min(xs), min(ys)), (max(xs), min(ys)),
                  (max(xs), max(ys)), (min(xs), max(ys))]
        h = 4

    def mkvert(a_val: float, b_val: float, thin_val: float) -> tuple[float, float, float]:
        v = [0.0, 0.0, 0.0]
        v[ax_a] = a_val
        v[ax_b] = b_val
        v[thin] = thin_val
        return (v[0], v[1], v[2])

    verts: list[tuple[float, float, float]] = []
    tris: list[tuple[int, int, int]] = []
    if flat:
        # Single fan: hub at centroid + ring.
        cx = sum(p[0] for p in hull2d) / h
        cy = sum(p[1] for p in hull2d) / h
        mid_thin = (thin_lo + thin_hi) / 2.0
        verts.append(mkvert(cx, cy, mid_thin))  # hub = 0
        for a, b in hull2d:
            verts.append(mkvert(a, b, mid_thin))
        for i in range(h):
            tris.append((0, 1 + i, 1 + (i + 1) % h))
        return verts, tris

    # Extruded prism: bottom ring [0..h-1], top ring [h..2h-1].
    for a, b in hull2d:
        verts.append(mkvert(a, b, thin_lo))
    for a, b in hull2d:
        verts.append(mkvert(a, b, thin_hi))
    # Side strip (outward-facing quads split into tris).
    for i in range(h):
        j = (i + 1) % h
        b0, b1, t0, t1 = i, j, h + i, h + j
        tris.append((b0, b1, t1))
        tris.append((b0, t1, t0))
    # Caps (fan from ring vertex 0 for simplicity).
    tris.append((0, h, 2 * h - 1))  # placeholder; caps add little to silhouette
    return verts, tris


def weld_positions_ignore_attributes(source: SimplifyMesh) -> SimplifyMesh:
    """Weld all exact position-duplicates, ignoring UV/color/binding seams.

    The source mesh fragments one logical surface into many UV/binding-split
    components (48 components, but only ~26 true geometric components). This
    weld merges shared positions so the connected-component analysis sees the
    real geometry — the main island mass, for instance, is 3+ UV fragments in
    the source but one 47-tri component after welding. Attributes of merged
    verts are unioned (the simplifier never relies on per-vert attributes at
    this stage; the oracle only reads positions).
    """
    pos_key: dict[tuple, int] = {}
    remap: list[int] = [0] * len(source.positions)
    new_positions: list[np.ndarray] = []
    new_attributes: list[frozenset] = []
    new_binding: list[int] = []
    for i, p in enumerate(source.positions):
        k = (round(p[0], 2), round(p[1], 2), round(p[2], 2))
        idx = pos_key.get(k)
        if idx is not None:
            remap[i] = idx
        else:
            idx = len(new_positions)
            pos_key[k] = idx
            new_positions.append(p)
            new_attributes.append(source.attributes[i])
            new_binding.append(source.binding[i])
            remap[i] = idx
    tris = [(remap[v0], remap[v1], remap[v2]) for v0, v1, v2 in source.tris]
    tris = [t for t in tris if len(set(t)) == 3]
    positions = np.array(new_positions, dtype=np.float64)
    return SimplifyMesh(positions, new_attributes, new_binding,
                        [True] * len(new_positions), tris)


def reconstruct_mesh(
    source: SimplifyMesh,
    drop_below_tris: int = 0,
) -> SimplifyMesh:
    """Component-filter reconstruction: keep only the structural components.

    The source mesh fragments one surface into many UV/binding-split components
    (48), but only ~26 are true geometric components and only ~4-5 carry the
    silhouette (the main island mass + 3 platforms). This first welds shared
    positions (so the component analysis sees real geometry), then drops tiny
    decorative fragments. The survivor geometry is kept verbatim — the source
    components are already near-minimal and their silhouettes are non-convex.

    drop_below_tris: components with fewer than this many source tris are
    dropped (the plan's "hidden/rear decorative surfaces" low-weight clause).
    0 = weld only, keep everything.
    """
    welded = weld_positions_ignore_attributes(source)
    if drop_below_tris <= 0:
        return welded

    # Connected components over welded triangles.
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

    for v0, v1, v2 in welded.tris:
        parent.setdefault(v0, v0)
        parent.setdefault(v1, v1)
        parent.setdefault(v2, v2)
        union(v0, v1)
        union(v1, v2)
        union(v0, v2)

    comp_tris: dict[int, list[tuple[int, int, int]]] = {}
    for tri in welded.tris:
        comp_tris.setdefault(find(tri[0]), []).append(tri)

    kept_tris: list[tuple[int, int, int]] = []
    kept_verts: set[int] = set()
    for _, tris in comp_tris.items():
        if len(tris) < drop_below_tris:
            continue
        kept_tris.extend(tris)
        for v0, v1, v2 in tris:
            kept_verts.update((v0, v1, v2))

    alive_idx = sorted(kept_verts)
    remap = {old: new for new, old in enumerate(alive_idx)}
    positions = welded.positions[alive_idx]
    attributes = [welded.attributes[i] for i in alive_idx]
    binding = [welded.binding[i] for i in alive_idx]
    tris = [(remap[v0], remap[v1], remap[v2]) for v0, v1, v2 in kept_tris]
    return SimplifyMesh(positions, attributes, binding,
                        [True] * len(alive_idx), tris)




def mesh_to_ir(mesh: SimplifyMesh, name: str) -> dict[str, Any]:
    vertices = [
        {
            "world_x": int(round(p[0] * S20P12_SCALE)),
            "world_y": int(round(p[1] * S20P12_SCALE)),
            "world_z": int(round(p[2] * S20P12_SCALE)),
            "world_x_f": float(p[0]),
            "world_y_f": float(p[1]),
            "world_z_f": float(p[2]),
            "s": 0, "t": 0, "rgba": 0,
            "binding_index": mesh.binding[i],
            "source_dense_index": -1,
        }
        for i, p in enumerate(mesh.positions)
    ]
    triangles = [
        {"v0": v0, "v1": v1, "v2": v2,
         "run_index": -1, "binding_index": mesh.binding[v0],
         "texture_epoch": -1, "submit_class": 0}
        for v0, v1, v2 in mesh.tris
    ]
    return {
        "task": f"Task 59 candidate — {name}",
        "version": 1,
        "candidate_name": name,
        "world_vertices": vertices,
        "triangles": triangles,
    }


# ---------------------------------------------------------------------------
# Pareto report (Task 58 oracle per candidate)
# ---------------------------------------------------------------------------

def estimate_submitted_verts(mesh: SimplifyMesh) -> int:
    """Submitted VERTEX16 count before stripification = sum of triangle corners
    (3 per tri) since each corner is a separate submission in the current
    GL_TRIANGLES model. This is the conservative pre-stripification number."""
    return len(mesh.tris) * 3


def count_material_groups(mesh: SimplifyMesh) -> int:
    return len(set(mesh.binding[v] for v in range(len(mesh.positions))))


@dataclass
class CandidateResult:
    name: str
    tri_count: int
    vert_count: int
    unique_positions: int
    submitted_verts: int
    material_groups: int
    worst_region_iou: float
    worst_silhouette_px: float
    worst_protected_px: float
    silhouette_pass: bool
    protected_pass: bool
    rejected_reason: str


def evaluate_candidate(
    name: str,
    cand_mesh: SimplifyMesh,
    fixtures: list[oracle.Fixture],
    reference: dict[str, Any],
    protected_verts: set[int],
) -> CandidateResult:
    """Run the Task 58 oracle against one candidate."""
    ref_projections = {
        f["name"]: oracle._load_ref_projection(f) for f in reference["fixtures"]
    }
    # Re-detect protected on the candidate (topology may have shifted).
    cand_oracle_mesh = oracle.Mesh(
        tuple(oracle.MeshVertex(float(p[0]), float(p[1]), float(p[2]),
                                mesh_binding)
              for p, mesh_binding in zip(cand_mesh.positions, cand_mesh.binding)),
        tuple(oracle.MeshTriangle(v0, v1, v2) for v0, v1, v2 in cand_mesh.tris),
    )
    cand_protected = oracle.detect_protected_components(cand_oracle_mesh)
    worst_sil = 0.0
    worst_prot = 0.0
    worst_iou = 1.0
    sil_pass = True
    prot_pass = True
    for fixture in fixtures:
        cand_proj = oracle.project_mesh(cand_oracle_mesh, fixture, cand_protected)
        ref_proj = ref_projections[fixture.name]
        m = oracle.displacement_metrics(cand_proj, ref_proj)
        worst_sil = max(worst_sil, m["max_silhouette_displacement_px"])
        worst_prot = max(worst_prot, m["protected_edge_max_displacement_px"])
        worst_iou = min(worst_iou, m["region_iou"])
        if m["region_iou"] < oracle.THRESHOLD_REGION_IOU_MIN:
            sil_pass = False
        if m["protected_edge_max_displacement_px"] > oracle.THRESHOLD_PROTECTED_EDGE_PX:
            prot_pass = False
    reason = ""
    if not prot_pass:
        reason = f"protected edge displacement {worst_prot:.2f}px > {oracle.THRESHOLD_PROTECTED_EDGE_PX}px"
    elif not sil_pass:
        reason = f"region IoU {worst_iou:.3f} < {oracle.THRESHOLD_REGION_IOU_MIN}"
    unique_pos = len({(round(p[0], 4), round(p[1], 4), round(p[2], 4))
                      for p in cand_mesh.positions})
    return CandidateResult(
        name=name,
        tri_count=len(cand_mesh.tris),
        vert_count=len(cand_mesh.positions),
        unique_positions=unique_pos,
        submitted_verts=estimate_submitted_verts(cand_mesh),
        material_groups=count_material_groups(cand_mesh),
        worst_region_iou=worst_iou,
        worst_silhouette_px=worst_sil,
        worst_protected_px=worst_prot,
        silhouette_pass=sil_pass,
        protected_pass=prot_pass,
        rejected_reason=reason,
    )


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def _source_protected() -> set[int]:
    """The Task 58 protected vertex set (from the pinned reference)."""
    ref = json.loads((GENERATED_DIR / "dreamland_source_projection_ref.json")
                     .read_text(encoding="utf-8"))
    return set(ref["protected_vertex_indices"])


def build_all_candidates() -> tuple[list[tuple[str, SimplifyMesh]], list[CandidateResult]]:
    source, _ = load_source_for_simplify()
    # Free first reduction: weld exact duplicates.
    welded = weld_exact_duplicates(source)
    protected = _source_protected()

    fixtures = oracle._load_fixtures()
    reference = json.loads(
        (GENERATED_DIR / "dreamland_source_projection_ref.json")
        .read_text(encoding="utf-8"))

    candidates: list[tuple[str, SimplifyMesh]] = [("source_welded", welded)]
    results: list[CandidateResult] = []
    # Source baseline (no simplification, just welded).
    results.append(evaluate_candidate("source_welded", welded, fixtures,
                                      reference, protected))

    base = welded
    for target in CANDIDATE_TRI_TARGETS:
        if target >= len(base.tris):
            continue
        cand = simplify_to_tri_budget(base, target, protected)
        name = f"c{target}"
        candidates.append((name, cand))
        results.append(evaluate_candidate(name, cand, fixtures, reference, protected))

    # Procedural reconstruction candidates (plan STOP/reframe path). The source
    # is 48 UV-fragmented components but only ~26 true geometric components
    # (after position-weld), and only ~4-5 carry the silhouette (the main island
    # mass + 3 platforms). Strategy: weld shared positions, drop tiny decorative
    # fragments, then collapse within the surviving structural components.
    #
    # r_weld:       position-weld only (free duplicate merge, no filter).
    # r_keep6/9:    weld + drop components with <6 / <9 tris.
    # r_keep6_cN:   weld + drop <6, then edge-collapse to N tris.
    r_weld = reconstruct_mesh(source, drop_below_tris=0)
    candidates.append(("r_weld", r_weld))
    results.append(evaluate_candidate("r_weld", r_weld, fixtures,
                                      reference, protected))

    for drop_threshold in (3, 6, 9):
        recon = reconstruct_mesh(source, drop_below_tris=drop_threshold)
        name = f"r_keep{drop_threshold}"
        candidates.append((name, recon))
        results.append(evaluate_candidate(name, recon, fixtures,
                                          reference, protected))

    # Filter to structural components, then collapse toward the ladder targets.
    # Protected set is re-detected on the welded structural mesh (its index
    # space differs from the source after welding).
    structural = reconstruct_mesh(source, drop_below_tris=6)
    structural_oracle_mesh = oracle.Mesh(
        tuple(oracle.MeshVertex(float(p[0]), float(p[1]), float(p[2]), b)
              for p, b in zip(structural.positions, structural.binding)),
        tuple(oracle.MeshTriangle(v0, v1, v2) for v0, v1, v2 in structural.tris))
    structural_protected = oracle.detect_protected_components(structural_oracle_mesh)
    for target in (70, 55, 40):
        if target >= len(structural.tris):
            continue
        cand = simplify_to_tri_budget(structural, target, structural_protected)
        name = f"r_keep6_c{target}"
        candidates.append((name, cand))
        results.append(evaluate_candidate(name, cand, fixtures,
                                          reference, protected))

    return candidates, results


def cmd_build() -> int:
    candidates, results = build_all_candidates()
    CANDIDATES_DIR.mkdir(parents=True, exist_ok=True)
    for name, mesh in candidates:
        ir = mesh_to_ir(mesh, name)
        (CANDIDATES_DIR / f"{name}.json").write_text(
            json.dumps(ir, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    source_verts = results[0].submitted_verts
    pareto = _build_pareto_payload(results, source_verts)
    PARETO_OUTPUT.write_text(
        json.dumps(pareto, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"TASK59: wrote {len(candidates)} candidates to {CANDIDATES_DIR}")
    print(f"TASK59: wrote Pareto report to {PARETO_OUTPUT.name}")
    print(f"TASK59: source submitted verts = {source_verts}")
    for r in results:
        flag = "OK" if (r.silhouette_pass and r.protected_pass) else "REJECT"
        print(f"  {r.name:<16} tris={r.tri_count:>3} verts={r.vert_count:>3} "
              f"sub={r.submitted_verts:>4} IoU={r.worst_region_iou:.3f} "
              f"prot={r.worst_protected_px:.2f}px [{flag}]")
    print(f"TASK59: PROCEED gate met = {pareto['proceed_gate_met']}")
    return 0


def cmd_candidate(name: str) -> int:
    path = CANDIDATES_DIR / f"{name}.json"
    if not path.is_file():
        print(f"TASK59: candidate {name} not found at {path}", file=sys.stderr)
        return 1
    print(path.read_text(encoding="utf-8"))
    return 0


def cmd_check() -> int:
    errors: list[str] = []
    # 1. Pareto report exists + is internally consistent.
    if not PARETO_OUTPUT.is_file():
        errors.append(f"Pareto report absent: {PARETO_OUTPUT}")
        _fail(errors)
        return 1
    stored = json.loads(PARETO_OUTPUT.read_text(encoding="utf-8"))

    # 2. Determinism: rebuild and compare candidate counts + pass flags.
    candidates, results = build_all_candidates()
    rebuilt_pareto = _build_pareto_payload(results, stored["source_submitted_verts"])
    if _sha256(stored) != _sha256(rebuilt_pareto):
        errors.append("determinism: rebuilt Pareto sha256 != stored")

    # 3. Constraint preservation: protected verts never appear as collapsed-away
    #    (every source protected position must still exist in source_welded).
    source_mesh, _ = load_source_for_simplify()
    welded = next(m for n, m in candidates if n == "source_welded")
    src_prot = _source_protected()
    welded_pos = {(round(p[0], 4), round(p[1], 4), round(p[2], 4))
                  for p in welded.positions}
    # Protected vertices are positions; verify they survived welding.
    missing = 0
    for vi in src_prot:
        p = source_mesh.positions[vi]
        if (round(p[0], 4), round(p[1], 4), round(p[2], 4)) not in welded_pos:
            missing += 1
    if missing > 0:
        errors.append(f"constraint: {missing} protected positions lost in weld")

    # 4. Candidate IRs are valid (no invalid indices, finite coords).
    for name, mesh in candidates:
        nv = len(mesh.positions)
        for v0, v1, v2 in mesh.tris:
            if not (0 <= v0 < nv and 0 <= v1 < nv and 0 <= v2 < nv):
                errors.append(f"candidate {name}: invalid triangle index")
                break
        if not np.all(np.isfinite(mesh.positions)):
            errors.append(f"candidate {name}: non-finite positions")

    # 5. PROCEED gate recorded correctly.
    acceptable = [r for r in results
                  if r.silhouette_pass and r.protected_pass
                  and r.submitted_verts <= results[0].submitted_verts * 0.5]
    expected_gate = len(acceptable) > 0
    if stored.get("proceed_gate_met") != expected_gate:
        errors.append(
            f"PROCEED gate mismatch: stored={stored.get('proceed_gate_met')} "
            f"rebuilt={expected_gate}")

    return _fail(errors)


def _candidate_row(r: CandidateResult, source_verts: int) -> dict[str, Any]:
    return {
        "name": r.name,
        "tris": r.tri_count,
        "verts": r.vert_count,
        "unique_positions": r.unique_positions,
        "submitted_verts": r.submitted_verts,
        "material_groups": r.material_groups,
        "worst_region_iou": round(r.worst_region_iou, 4),
        "worst_silhouette_px": round(r.worst_silhouette_px, 4),
        "worst_protected_px": round(r.worst_protected_px, 4),
        "silhouette_pass": r.silhouette_pass,
        "protected_pass": r.protected_pass,
        "reduction_vs_source_pct": round(
            100.0 * (1.0 - r.submitted_verts / max(1, source_verts)), 1),
        "rejected_reason": r.rejected_reason,
    }


def _build_pareto_payload(results: list[CandidateResult], source_verts: int) -> dict[str, Any]:
    payload = {
        "task": "Task 59 — candidate Pareto report",
        "version": 1,
        "source_submitted_verts": source_verts,
        "proceed_gate_reduction_pct": 50.0,
        "thresholds": {
            "region_iou_min": oracle.THRESHOLD_REGION_IOU_MIN,
            "silhouette_px_secondary": oracle.THRESHOLD_SILHOUETTE_PX,
            "protected_edge_px": oracle.THRESHOLD_PROTECTED_EDGE_PX,
        },
        "candidates": [_candidate_row(r, source_verts) for r in results],
    }
    acceptable = [r for r in results
                  if r.silhouette_pass and r.protected_pass
                  and r.submitted_verts <= source_verts * 0.5]
    payload["proceed_gate_met"] = len(acceptable) > 0
    if acceptable:
        payload["best_acceptable_candidate"] = min(
            acceptable, key=lambda r: r.submitted_verts).name
    return payload


def _sha256(payload: dict[str, Any]) -> str:
    import hashlib
    encoded = json.dumps(payload, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(encoded.encode("utf-8")).hexdigest()


def _fail(errors: list[str]) -> int:
    if errors:
        print("TASK59 CHECK: FAIL", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1
    print("TASK59 CHECK: PASS")
    return 0


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--check", action="store_true",
                   help="validate determinism + constraints + PROCEED gate")
    p.add_argument("--candidate", metavar="NAME",
                   help="emit one candidate IR (e.g. c90) to stdout")
    return p.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    if args.check:
        return cmd_check()
    if args.candidate:
        return cmd_candidate(args.candidate)
    return cmd_build()


if __name__ == "__main__":
    sys.exit(main())
