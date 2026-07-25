#!/usr/bin/env python3
"""Task 58 — Dream Land gameplay-camera screen-space error oracle.

Host-only. No runtime/renderer changes. Builds a deterministic visual-fidelity
metric so later mesh simplification (Task 59) is judged by what the player
actually sees on screen, not by arbitrary 3D geometric distance.

The renderer is camera-agnostic: it consumes pre-composed projection x view x
world matrices. The Dream Land default camera converges FOV to 38.0
(gmCameraAdjustFOV(38.0), gmcamera.c:636) and tracks the live fighter
centroid; the 2-player zoom range multiplier is 1.32
(dGMCameraPlayerZoomRanges, gmcamera.c:43). Internal viewport ~320x240
(vscale 600/440, taskman_seam.c:5749).

Key de-risk: the oracle compares *source mesh vs candidate mesh* through the
SAME projection. It therefore does NOT need to bit-replicate the adapter's
fixed-point guPerspective+guLookAt build — it needs (1) fixtures grounded in
the real camera envelope (bounds/FOV/distance), and (2) a consistent
perspective projection matching the DS effective FOV/aspect. Same-projection-
on-both-sides is what makes the silhouette-displacement metric meaningful.

Usage
-----
  dreamland_camera_oracle.py                  # build fixtures + source ref
  dreamland_camera_oracle.py --check          # validate stored artifacts
  dreamland_camera_oracle.py --compare MESH   # compare a candidate mesh
  dreamland_camera_oracle.py --self           # identity probe (must be ~0)
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import math
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Sequence

import dreamland_world_mesh as world_mesh

SCRIPTS_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPTS_DIR.parent
GENERATED_DIR = SCRIPTS_DIR / "generated"
FIXTURES_OUTPUT = GENERATED_DIR / "dreamland_camera_fixtures.json"
REFERENCE_OUTPUT = GENERATED_DIR / "dreamland_source_projection_ref.json"
WORLD_MESH_IR = GENERATED_DIR / "dreamland_world_mesh.json"

# ---------------------------------------------------------------------------
# Camera constants (from decomp gmcamera.c / taskman_seam.c)
# ---------------------------------------------------------------------------

# Dream Land default camera converges here (gmCameraDefaultFuncCamera, gmcamera.c:636).
FOVY_DEG = 38.0
# 2-player zoom range multiplier (dGMCameraPlayerZoomRanges[2], gmcamera.c:43).
ZOOM_RANGE_2P = 1.32
# Internal DS 3D viewport. vscale=(600,440), vtrans=(640,480) (taskman_seam.c:5749)
# maps to a 320x240 device-pixel framebuffer region for the 3D view.
VIEWPORT_W_PX = 320
VIEWPORT_H_PX = 240
ASPECT = VIEWPORT_W_PX / VIEWPORT_H_PX

# Configurable thresholds (plan section 6; not sacred).
# Primary visual metric is region IoU (1.0 = pixel-identical outline). The
# equivalent-displacement-px figure is secondary. A candidate passes the
# silhouette gate when worst-case IoU across fixtures >= REGION_IOU_MIN.
THRESHOLD_PROTECTED_EDGE_PX = 0.5
THRESHOLD_SILHOUETTE_PX = 1.5
THRESHOLD_DECORATIVE_PX = 2.0
THRESHOLD_REGION_IOU_MIN = 0.95      # >=95% region overlap = visually acceptable

# Protected-edge auto-detect: top-facing surfaces within this many degrees of +Y.
PROTECTED_MAX_TILT_DEG = 15.0
# Number of largest near-horizontal components to protect (main floor + 3 platforms).
PROTECTED_COMPONENT_COUNT = 4

# s20.12 -> float.
S20P12_SCALE = 4096.0


# ---------------------------------------------------------------------------
# Fixtures — analytic envelope
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class Fixture:
    name: str
    eye: tuple[float, float, float]
    at: tuple[float, float, float]
    up: tuple[float, float, float]
    fovy_deg: float
    near: float
    far: float
    source_note: str


def _stage_envelope() -> dict[str, float]:
    """Read the static-mesh world bounds + centroid from Task 57's IR."""
    ir = json.loads(WORLD_MESH_IR.read_text(encoding="utf-8"))
    b = ir["census"]["bounds_s20p12"]
    min_xyz = [b["min"][i] / S20P12_SCALE for i in range(3)]
    max_xyz = [b["max"][i] / S20P12_SCALE for i in range(3)]
    extent = [max_xyz[i] - min_xyz[i] for i in range(3)]
    center = [(min_xyz[i] + max_xyz[i]) / 2.0 for i in range(3)]
    return {
        "min": min_xyz, "max": max_xyz, "extent": extent, "center": center,
    }


def _gameplay_eye_distance(half_extent_x: float, fovy_deg: float) -> float:
    """Distance along +Z so the horizontal stage extent fits at the given FOV.

    Approximates gmCameraGetClampDimensionsMax framing: the eye sits in front
    of the stage far enough that the fighting surface's horizontal half-extent
    is visible. Uses the horizontal FOV derived from the vertical FOV + aspect.
    """
    hfov = 2.0 * math.atan(math.tan(math.radians(fovy_deg) / 2.0) * ASPECT)
    dist = half_extent_x / math.tan(hfov / 2.0)
    return dist


def build_fixtures() -> list[Fixture]:
    env = _stage_envelope()
    cx, cy, cz = env["center"]
    ext = env["extent"]
    half_x = ext[0] / 2.0
    # The gameplay band: fighters stand on surfaces roughly in the upper-middle
    # of the Y range; the camera "at" targets a point slightly above center.
    at_y_gameplay = cy + ext[1] * 0.18
    # Front-view eye sits in front of the stage (+Z) and above the at target.
    base_dist = _gameplay_eye_distance(half_x * ZOOM_RANGE_2P, FOVY_DEG)
    eye_height_offset = ext[1] * 0.10
    near = max(100.0, ext[2] * 0.05)
    far = ext[2] * 6.0 + base_dist * 2.0

    up = (0.0, 1.0, 0.0)
    fixtures: list[Fixture] = []

    # center_medium: canonical normal framing.
    fixtures.append(Fixture(
        "center_medium",
        (cx, at_y_gameplay + eye_height_offset, cz + base_dist),
        (cx, at_y_gameplay, cz), up, FOVY_DEG, near, far,
        "stage center; base 2-player zoom distance"))

    # left_medium / right_medium: at shifted to horizontal extremes, eye tracks.
    for sign, name in ((-1, "left_medium"), (1, "right_medium")):
        edge_x = cx + sign * half_x * 0.78
        fixtures.append(Fixture(
            name,
            (edge_x, at_y_gameplay + eye_height_offset, cz + base_dist),
            (edge_x, at_y_gameplay, cz), up, FOVY_DEG, near, far,
            f"horizontal {'left' if sign<0 else 'right'} framing"))

    # center_close: pull eye in (smaller zoom multiplier -> closer).
    close_dist = _gameplay_eye_distance(half_x * (ZOOM_RANGE_2P * 0.72), FOVY_DEG)
    fixtures.append(Fixture(
        "center_close",
        (cx, at_y_gameplay + eye_height_offset, cz + close_dist),
        (cx, at_y_gameplay, cz), up, FOVY_DEG, near, far,
        "close zoom (reduced multiplier)"))

    # center_maxzoom: pull eye back (max 2-player zoom-out).
    maxzoom_dist = _gameplay_eye_distance(half_x * (ZOOM_RANGE_2P * 1.28), FOVY_DEG)
    fixtures.append(Fixture(
        "center_maxzoom",
        (cx, at_y_gameplay + eye_height_offset, cz + maxzoom_dist),
        (cx, at_y_gameplay, cz), up, FOVY_DEG, near, far,
        "maximum normal 2-player zoom-out"))

    # high_frame / low_frame: vertical framing from fighters near top/bottom.
    for sign, name in ((1, "high_frame"), (-1, "low_frame")):
        at_y = at_y_gameplay + sign * ext[1] * 0.22
        fixtures.append(Fixture(
            name,
            (cx, at_y + eye_height_offset, cz + base_dist),
            (cx, at_y, cz), up, FOVY_DEG, near, far,
            f"{'high' if sign>0 else 'low'} vertical framing"))

    # respawn_frame: slightly wider + higher (KO/respawn). Flagged for
    # material-difference check so it can be dropped if redundant.
    fixtures.append(Fixture(
        "respawn_frame",
        (cx, at_y_gameplay + ext[1] * 0.35, cz + base_dist * 1.18),
        (cx, at_y_gameplay + ext[1] * 0.12, cz), up, FOVY_DEG, near, far,
        "representative KO/respawn framing (wider+higher)"))

    return fixtures


def serialize_fixtures(fixtures: list[Fixture]) -> dict[str, Any]:
    env = _stage_envelope()
    return {
        "task": "Task 58 — Dream Land gameplay camera fixtures",
        "version": 1,
        "constants": {
            "fovy_deg": FOVY_DEG,
            "zoom_range_2p": ZOOM_RANGE_2P,
            "viewport_w_px": VIEWPORT_W_PX,
            "viewport_h_px": VIEWPORT_H_PX,
            "aspect": ASPECT,
            "protected_max_tilt_deg": PROTECTED_MAX_TILT_DEG,
            "protected_component_count": PROTECTED_COMPONENT_COUNT,
        },
        "thresholds_px": {
            "protected_edge": THRESHOLD_PROTECTED_EDGE_PX,
            "silhouette": THRESHOLD_SILHOUETTE_PX,
            "decorative": THRESHOLD_DECORATIVE_PX,
        },
        "stage_envelope_world_units": {
            "min": env["min"], "max": env["max"],
            "extent": env["extent"], "center": env["center"],
        },
        "fixtures": [
            {
                "name": f.name,
                "eye": list(f.eye),
                "at": list(f.at),
                "up": list(f.up),
                "fovy_deg": f.fovy_deg,
                "near": f.near,
                "far": f.far,
                "source_note": f.source_note,
            }
            for f in fixtures
        ],
    }


# ---------------------------------------------------------------------------
# Projection (float; consistent on both sides of a comparison)
# ---------------------------------------------------------------------------

def perspective_matrix(fovy_deg: float, aspect: float,
                       near: float, far: float) -> list[list[float]]:
    """Right-handed perspective projection (column-vector, OpenGL convention).

    Maps the eye-space frustum to clip space; clip.w = -z_eye (the row-3 -1
    puts -z_eye into w so the perspective divide projects correctly).
    """
    f = 1.0 / math.tan(math.radians(fovy_deg) / 2.0)
    nf = 1.0 / (near - far)
    return [
        [f / aspect, 0.0, 0.0, 0.0],
        [0.0, f, 0.0, 0.0],
        [0.0, 0.0, (far + near) * nf, 2.0 * far * near * nf],
        [0.0, 0.0, -1.0, 0.0],
    ]


def lookat_matrix(eye: Sequence[float], at: Sequence[float],
                  up: Sequence[float]) -> list[list[float]]:
    """Right-handed look-at view matrix (column-vector convention: translation
    in column 3; world point multiplied as M * v with v a column vector).

    forward = normalize(eye - at); the camera looks down -forward (i.e. toward
    at from eye). right = normalize(up x forward); up' = forward x right.
    """
    fx = eye[0] - at[0]
    fy = eye[1] - at[1]
    fz = eye[2] - at[2]
    rlen = math.sqrt(fx * fx + fy * fy + fz * fz)
    if rlen == 0.0:
        raise ValueError("lookat: eye == at")
    fx, fy, fz = fx / rlen, fy / rlen, fz / rlen
    # right = up x forward
    rx = up[1] * fz - up[2] * fy
    ry = up[2] * fx - up[0] * fz
    rz = up[0] * fy - up[1] * fx
    rlen = math.sqrt(rx * rx + ry * ry + rz * rz)
    if rlen == 0.0:
        raise ValueError("lookat: up parallel to forward")
    rx, ry, rz = rx / rlen, ry / rlen, rz / rlen
    # up' = forward x right
    ux = fy * rz - fz * ry
    uy = fz * rx - fx * rz
    uz = fx * ry - fy * rx
    # Column-vector form: rows are basis vectors, translation in column 3.
    # out * [x,y,z,1]^T = [rx*x+ry*y+rz*z - dot(r,eye), ...].
    return [
        [rx, ry, rz, -(rx * eye[0] + ry * eye[1] + rz * eye[2])],
        [ux, uy, uz, -(ux * eye[0] + uy * eye[1] + uz * eye[2])],
        [fx, fy, fz, -(fx * eye[0] + fy * eye[1] + fz * eye[2])],
        [0.0, 0.0, 0.0, 1.0],
    ]


def matmul(a: list[list[float]], b: list[list[float]]) -> list[list[float]]:
    out = [[0.0] * 4 for _ in range(4)]
    for i in range(4):
        for j in range(4):
            out[i][j] = sum(a[i][k] * b[k][j] for k in range(4))
    return out


def project_point(world: Sequence[float], vp: list[list[float]]) -> tuple[float, float, float, float]:
    """Project a world point through a 4x4 view-projection matrix.

    Returns (screen_x_px, screen_y_px, depth, w). screen coords are in DS
    device pixels over the 3D viewport region; y is top-down (0 at top).
    """
    cx = vp[0][0] * world[0] + vp[0][1] * world[1] + vp[0][2] * world[2] + vp[0][3]
    cy = vp[1][0] * world[0] + vp[1][1] * world[1] + vp[1][2] * world[2] + vp[1][3]
    cz = vp[2][0] * world[0] + vp[2][1] * world[1] + vp[2][2] * world[2] + vp[2][3]
    cw = vp[3][0] * world[0] + vp[3][1] * world[1] + vp[3][2] * world[2] + vp[3][3]
    if cw == 0.0:
        return (0.0, 0.0, 0.0, 0.0)
    ndcx = cx / cw
    ndcy = cy / cw
    ndcz = cz / cw
    # NDC [-1,1] -> device pixels; flip Y to top-down.
    sx = (ndcx * 0.5 + 0.5) * VIEWPORT_W_PX
    sy = (1.0 - (ndcy * 0.5 + 0.5)) * VIEWPORT_H_PX
    return (sx, sy, ndcz, cw)


def view_projection(fixture: Fixture) -> list[list[float]]:
    p = perspective_matrix(fixture.fovy_deg, ASPECT, fixture.near, fixture.far)
    v = lookat_matrix(fixture.eye, fixture.at, fixture.up)
    return matmul(p, v)


# ---------------------------------------------------------------------------
# Mesh loading (Task 57 IR schema)
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class MeshVertex:
    x: float
    y: float
    z: float
    binding_index: int


@dataclass(frozen=True)
class MeshTriangle:
    v0: int
    v1: int
    v2: int


@dataclass(frozen=True)
class Mesh:
    vertices: tuple[MeshVertex, ...]
    triangles: tuple[MeshTriangle, ...]


def load_mesh(path: Path) -> Mesh:
    ir = json.loads(path.read_text(encoding="utf-8"))
    verts = tuple(
        MeshVertex(v["world_x_f"], v["world_y_f"], v["world_z_f"],
                   v["binding_index"])
        for v in ir["world_vertices"]
    )
    tris = tuple(
        MeshTriangle(t["v0"], t["v1"], t["v2"]) for t in ir["triangles"]
    )
    return Mesh(verts, tris)


def load_source_mesh() -> Mesh:
    """Rebuild the source mesh from the live extraction (no stale IR)."""
    packet = world_mesh.stage_gen.generate(REPO_ROOT)
    wv, wt, _, _ = world_mesh.build_world_mesh(packet)
    verts = tuple(
        MeshVertex(v.world_x / S20P12_SCALE, v.world_y / S20P12_SCALE,
                   v.world_z / S20P12_SCALE, v.binding_index)
        for v in wv
    )
    tris = tuple(MeshTriangle(t.v0, t.v1, t.v2) for t in wt)
    return Mesh(verts, tris)


# ---------------------------------------------------------------------------
# Silhouette + protected-edge detection
# ---------------------------------------------------------------------------

def _tri_normal_world(verts: Sequence[MeshVertex], tri: MeshTriangle) -> tuple[float, float, float]:
    a = verts[tri.v0]
    b = verts[tri.v1]
    c = verts[tri.v2]
    ux, uy, uz = b.x - a.x, b.y - a.y, b.z - a.z
    vx, vy, vz = c.x - a.x, c.y - a.y, c.z - a.z
    nx = uy * vz - uz * vy
    ny = uz * vx - ux * vz
    nz = ux * vy - uy * vx
    rlen = math.sqrt(nx * nx + ny * ny + nz * nz)
    if rlen == 0.0:
        return (0.0, 0.0, 0.0)
    return (nx / rlen, ny / rlen, nz / rlen)


def _edge_key(a: int, b: int) -> tuple[int, int]:
    return (a, b) if a <= b else (b, a)


def connected_components(mesh: Mesh) -> dict[int, int]:
    """vertex index -> component root id (union-find over triangle edges)."""
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

    for tri in mesh.triangles:
        # Seed every triangle vertex as its own root before union, so the
        # returned map covers every vertex referenced by any triangle (a
        # vertex that is only ever a union target would otherwise be absent).
        for v in (tri.v0, tri.v1, tri.v2):
            parent.setdefault(v, v)
        union(tri.v0, tri.v1)
        union(tri.v1, tri.v2)
        union(tri.v0, tri.v2)
    return {v: find(v) for v in parent}


def detect_protected_components(mesh: Mesh) -> set[int]:
    """Return the set of vertex indices belonging to protected components.

    Protected = the PROTECTED_COMPONENT_COUNT largest connected components
    whose mean triangle normal tilts less than PROTECTED_MAX_TILT_DEG from +Y
    (i.e. near-horizontal top-facing surfaces: the main floor + 3 platforms).
    """
    comps = connected_components(mesh)
    # Aggregate per component: vertex set + mean normal.
    comp_verts: dict[int, set[int]] = {}
    comp_normals: dict[int, list[tuple[float, float, float]]] = {}
    for tri in mesh.triangles:
        n = _tri_normal_world(mesh.vertices, tri)
        # Skip degenerate (zero) normals.
        if n == (0.0, 0.0, 0.0):
            continue
        root = comps[tri.v0]
        comp_verts.setdefault(root, set()).update((tri.v0, tri.v1, tri.v2))
        comp_normals.setdefault(root, []).append(n)

    # Score each component: must be near-horizontal; rank by vertex count.
    candidates: list[tuple[int, int, float]] = []  # (root, vert_count, mean_tilt_deg)
    for root, verts in comp_verts.items():
        normals = comp_normals.get(root, [])
        if not normals:
            continue
        mean_ny = sum(n[1] for n in normals) / len(normals)
        # tilt from +Y: cos(tilt) = mean_ny (normalized mean).
        mag = math.sqrt(sum(ni * ni for n in normals for ni in [n[0], n[1], n[2]]) / max(1, len(normals)))
        # Use the mean y-component directly as a proxy for up-facing; clamp.
        up = max(-1.0, min(1.0, mean_ny))
        tilt_deg = math.degrees(math.acos(up))
        if tilt_deg <= PROTECTED_MAX_TILT_DEG:
            candidates.append((root, len(verts), tilt_deg))
    candidates.sort(key=lambda c: c[1], reverse=True)
    protected_roots = {root for root, _, _ in candidates[:PROTECTED_COMPONENT_COUNT]}
    protected: set[int] = set()
    for root in protected_roots:
        protected.update(comp_verts[root])
    return protected


def silhouette_edges(mesh: Mesh) -> list[tuple[int, int]]:
    """Boundary edges (shared by exactly one triangle) — the mesh outline."""
    refs: dict[tuple[int, int], int] = {}
    for tri in mesh.triangles:
        for a, b in ((tri.v0, tri.v1), (tri.v1, tri.v2), (tri.v0, tri.v2)):
            key = _edge_key(a, b)
            refs[key] = refs.get(key, 0) + 1
    return [e for e, c in refs.items() if c == 1]


# ---------------------------------------------------------------------------
# Per-fixture projection + metrics
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class FixtureProjection:
    """Projected source mesh under one fixture, for comparison.

    The primary visual-fidelity metric is the rasterized filled region: the
    silhouette is compared as the 2D UNION of projected triangles (what the
    player sees), not a per-fragment boundary point cloud. This is LOD-
    independent — a coarser mesh that fills the same screen region scores
    well, while genuine outline movement is caught. (Plan Task 58 req 5:
    "coarse image mask difference".)
    """
    name: str
    # Rasterized filled mask: 1 byte per pixel over the 320x240 viewport.
    region_mask: bytes
    protected_points: tuple[tuple[float, float], ...]
    bbox: tuple[float, float, float, float]  # min_x, min_y, max_x, max_y


def project_mesh(mesh: Mesh, fixture: Fixture,
                 protected_verts: set[int]) -> FixtureProjection:
    vp = view_projection(fixture)
    # Project all vertices once.
    screen: list[tuple[float, float]] = []
    for v in mesh.vertices:
        sx, sy, _, _ = project_point((v.x, v.y, v.z), vp)
        screen.append((sx, sy))
    # Rasterize the union of projected triangles (filled region). This is the
    # true visual silhouette: what the player sees as the stage's outline.
    region = bytearray(VIEWPORT_W_PX * VIEWPORT_H_PX)
    for tri in mesh.triangles:
        ax, ay = screen[tri.v0]
        bx, by = screen[tri.v1]
        cx, cy = screen[tri.v2]
        _rasterize_triangle(region, ax, ay, bx, by, cx, cy)
    # Protected points: projected protected vertices.
    prot_pts = [screen[i] for i in range(len(mesh.vertices)) if i in protected_verts]
    xs = [p[0] for p in screen]
    ys = [p[1] for p in screen]
    bbox = (min(xs), min(ys), max(xs), max(ys)) if xs else (0, 0, 0, 0)
    return FixtureProjection(fixture.name, bytes(region), tuple(prot_pts), bbox)


def _rasterize_triangle(mask: bytearray, ax: float, ay: float,
                        bx: float, by: float, cx: float, cy: float) -> None:
    """Fill triangle (ax,ay)-(bx,by)-(cx,cy) into the mask via barycentric test.

    Clips to the viewport; samples pixel centers. Backface/direction-agnostic
    (the visual region is the filled area regardless of facing).
    """
    x0 = max(0, int(min(ax, bx, cx) - 1))
    y0 = max(0, int(min(ay, by, cy) - 1))
    x1 = min(VIEWPORT_W_PX - 1, int(max(ax, bx, cx) + 1))
    y1 = min(VIEWPORT_H_PX - 1, int(max(ay, by, cy) + 1))
    for py in range(y0, y1 + 1):
        fy = py + 0.5
        row = py * VIEWPORT_W_PX
        for px in range(x0, x1 + 1):
            fx = px + 0.5
            d1 = (fx - bx) * (ay - by) - (ax - bx) * (fy - by)
            d2 = (fx - cx) * (by - cy) - (bx - cx) * (fy - cy)
            d3 = (fx - ax) * (cy - ay) - (cx - ax) * (fy - ay)
            has_neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
            has_pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
            if not (has_neg and has_pos):
                mask[row + px] = 1


def _percentile(sorted_vals: list[float], pct: float) -> float:
    if not sorted_vals:
        return 0.0
    # Nearest-rank percentile.
    idx = max(0, min(len(sorted_vals) - 1,
                     int(math.ceil(pct / 100.0 * len(sorted_vals))) - 1))
    return sorted_vals[idx]


def displacement_metrics(
    candidate_proj: FixtureProjection,
    reference_proj: FixtureProjection,
) -> dict[str, float]:
    """Rasterized-region visual difference + directed protected-edge distance.

    Primary metric: the symmetric-difference of the two filled-region masks,
    reported as (a) IoU (intersection-over-union, 1.0 = identical outline) and
    (b) the symmetric-difference pixel count converted to an equivalent
    displacement: sqrt(symdiff / perimeter). The perimeter is approximated by
    the source region's pixel boundary length so the metric scales like a
    pixel-distance along the outline. Protected edges stay as a directed
    nearest-point distance over the projected protected vertices (these are
    comparable 1:1 across LODs).
    """
    ref_mask = reference_proj.region_mask
    cand_mask = candidate_proj.region_mask
    assert len(ref_mask) == len(cand_mask) == VIEWPORT_W_PX * VIEWPORT_H_PX
    inter = 0
    symdiff = 0
    ref_only = 0
    for i in range(len(ref_mask)):
        r = ref_mask[i]
        c = cand_mask[i]
        if r and c:
            inter += 1
        elif r or c:
            symdiff += 1
        if r and not c:
            ref_only += 1
    uni = inter + symdiff
    iou = inter / uni if uni else 1.0
    # Equivalent outline displacement: treat symdiff as a band of width w along
    # the source outline of perimeter P => symdiff ~= w * P => w = symdiff / P.
    # Approximate P by the source boundary pixel count (edge between filled/
    # empty). Convert to a px figure.
    src_perim = _mask_perimeter(ref_mask)
    equiv_displacement_px = (symdiff / src_perim) if src_perim > 0 else float(symdiff)

    # Protected-edge directed distance (candidate protected -> source protected).
    ref_prot = reference_proj.protected_points
    cand_prot = candidate_proj.protected_points
    prot_dists: list[float] = []
    if cand_prot and ref_prot:
        for px_, py_ in cand_prot:
            best = float("inf")
            for qx, qy in ref_prot:
                dx = px_ - qx
                dy = py_ - qy
                d = dx * dx + dy * dy
                if d < best:
                    best = d
            prot_dists.append(math.sqrt(best))
    prot_dists.sort()
    prot_max = prot_dists[-1] if prot_dists else 0.0
    prot_p95 = _percentile(prot_dists, 95.0)

    rb = reference_proj.bbox
    cb = candidate_proj.bbox
    bbox_delta = {
        "min_x": abs(cb[0] - rb[0]),
        "min_y": abs(cb[1] - rb[1]),
        "max_x": abs(cb[2] - rb[2]),
        "max_y": abs(cb[3] - rb[3]),
    }
    return {
        # Primary visual metric: higher IoU is better (1.0 = identical).
        "region_iou": iou,
        # Equivalent outline displacement in px (symdiff / source perimeter).
        "max_silhouette_displacement_px": equiv_displacement_px,
        "p95_silhouette_displacement_px": equiv_displacement_px,
        "symmetric_difference_px": symdiff,
        "ref_only_px": ref_only,
        # Protected edges stay directed nearest-point.
        "protected_edge_max_displacement_px": prot_max,
        "protected_edge_p95_displacement_px": prot_p95,
        "projected_bbox_delta_px": bbox_delta,
        "region_filled_px": inter + (uni - inter),  # union size
    }


def _mask_perimeter(mask: bytes) -> int:
    """Count 4-connected boundary edges of the filled region (approx perimeter)."""
    perim = 0
    for y in range(VIEWPORT_H_PX):
        row = y * VIEWPORT_W_PX
        up_row = (y - 1) * VIEWPORT_W_PX
        down_row = (y + 1) * VIEWPORT_W_PX
        for x in range(VIEWPORT_W_PX):
            if not mask[row + x]:
                continue
            if x == 0 or not mask[row + x - 1]:
                perim += 1
            if x == VIEWPORT_W_PX - 1 or not mask[row + x + 1]:
                perim += 1
            if y == 0 or not mask[up_row + x]:
                perim += 1
            if y == VIEWPORT_H_PX - 1 or not mask[down_row + x]:
                perim += 1
    return perim


# ---------------------------------------------------------------------------
# Serialization helpers
# ---------------------------------------------------------------------------

def _sha256_json(payload: dict[str, Any]) -> str:
    encoded = json.dumps(payload, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(encoded.encode("utf-8")).hexdigest()


def build_reference(mesh: Mesh, fixtures: list[Fixture]) -> dict[str, Any]:
    protected = detect_protected_components(mesh)
    projections = [project_mesh(mesh, f, protected) for f in fixtures]
    return {
        "task": "Task 58 — Dream Land source projection reference",
        "version": 1,
        "source_mesh_sha256_note": (
            "rebuilt live from generate_nds_native_stage + dreamland_world_mesh; "
            "determinism gate is the --check rebuild-vs-stored sha256"),
        "protected_component_count_target": PROTECTED_COMPONENT_COUNT,
        "protected_vertex_count": len(protected),
        "protected_vertex_indices": sorted(protected),
        "fixtures": [
            {
                "name": p.name,
                # Region mask: base64 of the 320x240 filled-region bitmap. The
                # displacement metric compares masks directly (symmetric diff).
                "region_mask_b64": base64.b64encode(p.region_mask).decode("ascii"),
                "region_filled_pixels": sum(p.region_mask),
                "protected_points": [list(pt) for pt in p.protected_points],
                "bbox": list(p.bbox),
            }
            for p in projections
        ],
    }


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_build() -> int:
    fixtures = build_fixtures()
    fixtures_ir = serialize_fixtures(fixtures)
    FIXTURES_OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    FIXTURES_OUTPUT.write_text(
        json.dumps(fixtures_ir, indent=2, sort_keys=True) + "\n",
        encoding="utf-8")
    mesh = load_source_mesh()
    reference = build_reference(mesh, fixtures)
    REFERENCE_OUTPUT.write_text(
        json.dumps(reference, indent=2, sort_keys=True) + "\n",
        encoding="utf-8")
    print(f"TASK58: wrote {FIXTURES_OUTPUT.name} ({len(fixtures)} fixtures)")
    print(f"TASK58: wrote {REFERENCE_OUTPUT.name} "
          f"(protected {len(reference['protected_vertex_indices'])} verts)")
    print(f"TASK58: fixtures sha256 = {_sha256_json(fixtures_ir)}")
    print(f"TASK58: reference sha256 = {_sha256_json(reference)}")
    return 0


def _load_fixtures() -> list[Fixture]:
    ir = json.loads(FIXTURES_OUTPUT.read_text(encoding="utf-8"))
    return [
        Fixture(
            f["name"], tuple(f["eye"]), tuple(f["at"]), tuple(f["up"]),
            f["fovy_deg"], f["near"], f["far"], f["source_note"])
        for f in ir["fixtures"]
    ]


def _load_ref_projection(fix_entry: dict[str, Any]) -> FixtureProjection:
    """Reconstruct a FixtureProjection from the stored reference JSON."""
    return FixtureProjection(
        fix_entry["name"],
        base64.b64decode(fix_entry["region_mask_b64"]),
        tuple(tuple(pt) for pt in fix_entry["protected_points"]),
        tuple(fix_entry["bbox"]),
    )


def cmd_compare(candidate_path: Path) -> int:
    fixtures = _load_fixtures()
    ref_ir = json.loads(REFERENCE_OUTPUT.read_text(encoding="utf-8"))
    candidate = load_mesh(candidate_path)
    protected = detect_protected_components(candidate)
    ref_projections = {
        f["name"]: _load_ref_projection(f) for f in ref_ir["fixtures"]
    }
    report: list[dict[str, Any]] = []
    worst_sil = 0.0
    worst_prot = 0.0
    worst_iou = 1.0
    for fixture in fixtures:
        cand_proj = project_mesh(candidate, fixture, protected)
        ref_proj = ref_projections[fixture.name]
        m = displacement_metrics(cand_proj, ref_proj)
        sil_pass = m["region_iou"] >= THRESHOLD_REGION_IOU_MIN
        prot_pass = m["protected_edge_max_displacement_px"] <= THRESHOLD_PROTECTED_EDGE_PX
        worst_sil = max(worst_sil, m["max_silhouette_displacement_px"])
        worst_prot = max(worst_prot, m["protected_edge_max_displacement_px"])
        worst_iou = min(worst_iou, m["region_iou"])
        report.append({
            "fixture": fixture.name,
            "metrics": m,
            "silhouette_pass": sil_pass,
            "protected_pass": prot_pass,
        })
    summary = {
        "task": "Task 58 — candidate vs source comparison",
        "candidate": str(candidate_path),
        "thresholds": {
            "region_iou_min": THRESHOLD_REGION_IOU_MIN,
            "protected_edge_px": THRESHOLD_PROTECTED_EDGE_PX,
            "silhouette_px_secondary": THRESHOLD_SILHOUETTE_PX,
            "decorative_px": THRESHOLD_DECORATIVE_PX,
        },
        "worst_region_iou": worst_iou,
        "worst_silhouette_displacement_px": worst_sil,
        "worst_protected_displacement_px": worst_prot,
        "all_pass": all(r["silhouette_pass"] and r["protected_pass"] for r in report),
        "per_fixture": report,
    }
    print(json.dumps(summary, indent=2, sort_keys=True))
    return 0 if summary["all_pass"] else 2


def cmd_self() -> int:
    """Identity probe: compare the source mesh to its own reference (must be ~0)."""
    fixtures = _load_fixtures()
    ref_ir = json.loads(REFERENCE_OUTPUT.read_text(encoding="utf-8"))
    mesh = load_source_mesh()
    protected = set(ref_ir["protected_vertex_indices"])
    ref_projections = {
        f["name"]: _load_ref_projection(f) for f in ref_ir["fixtures"]
    }
    worst_iou = 1.0
    worst_prot = 0.0
    for fixture in fixtures:
        cand_proj = project_mesh(mesh, fixture, protected)
        ref_proj = ref_projections[fixture.name]
        m = displacement_metrics(cand_proj, ref_proj)
        worst_iou = min(worst_iou, m["region_iou"])
        worst_prot = max(worst_prot, m["protected_edge_max_displacement_px"])
    # Identity: IoU must be 1.0 and protected-edge displacement 0 (same mesh).
    ok = (worst_iou >= 1.0 - 1e-9) and (worst_prot < 1e-6)
    print(f"TASK58 identity probe: worst IoU = {worst_iou:.10f}, "
          f"protected = {worst_prot:.10f} px")
    return 0 if ok else 1


def cmd_check() -> int:
    errors: list[str] = []

    # 1. Determinism: rebuild fixtures + reference, compare sha256.
    if not FIXTURES_OUTPUT.is_file():
        errors.append(f"fixtures absent: {FIXTURES_OUTPUT}")
    if not REFERENCE_OUTPUT.is_file():
        errors.append(f"reference absent: {REFERENCE_OUTPUT}")
    if not errors:
        fixtures = build_fixtures()
        rebuilt_fx_ir = serialize_fixtures(fixtures)
        stored_fx_ir = json.loads(FIXTURES_OUTPUT.read_text(encoding="utf-8"))
        if _sha256_json(rebuilt_fx_ir) != _sha256_json(stored_fx_ir):
            errors.append("fixtures determinism: rebuilt sha256 != stored")

        mesh = load_source_mesh()
        rebuilt_ref = build_reference(mesh, fixtures)
        stored_ref = json.loads(REFERENCE_OUTPUT.read_text(encoding="utf-8"))
        if _sha256_json(rebuilt_ref) != _sha256_json(stored_ref):
            errors.append("reference determinism: rebuilt sha256 != stored")

        # 2. Validity: finite eye/at/fov; projection invertible; non-empty sil.
        for f in fixtures:
            for coord in (*f.eye, *f.at, *f.up, f.fovy_deg, f.near, f.far):
                if not math.isfinite(coord):
                    errors.append(f"fixture {f.name}: non-finite coordinate")
                    break
            try:
                vp = view_projection(f)
            except ValueError as exc:
                errors.append(f"fixture {f.name}: projection failed: {exc}")
                continue
            det = sum(vp[i][j] * ((-1) ** (i + j)) for i in range(4) for j in range(4))
            if not math.isfinite(det) or det == 0.0:
                errors.append(f"fixture {f.name}: singular view-projection")

        ref_by_name = {p["name"]: p for p in stored_ref["fixtures"]}
        for f in fixtures:
            p = ref_by_name.get(f.name)
            if p is None:
                errors.append(f"reference missing fixture {f.name}")
                continue
            if p.get("region_filled_pixels", 0) == 0:
                errors.append(f"fixture {f.name}: empty region (mesh not visible)")

        # 3. Protected-edge auto-detect determinism.
        prot_rebuilt = detect_protected_components(mesh)
        prot_stored = set(stored_ref["protected_vertex_indices"])
        if prot_rebuilt != prot_stored:
            errors.append(
                f"protected-edge determinism: rebuilt {len(prot_rebuilt)} verts != "
                f"stored {len(prot_stored)}")

    # 4. Identity probe: source vs its own reference must be ~0 displacement.
    if not errors:
        identity = cmd_self()
        if identity != 0:
            errors.append("identity probe: source-vs-self displacement != 0")

    if errors:
        print("TASK58 CHECK: FAIL", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1
    print("TASK58 CHECK: PASS")
    return 0


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--check", action="store_true",
                   help="validate stored fixtures + reference deterministically")
    p.add_argument("--build-reference", action="store_true",
                   help="build fixtures + source projection reference")
    p.add_argument("--compare", type=Path, metavar="MESH",
                   help="compare a candidate mesh IR against the source reference")
    p.add_argument("--self", action="store_true",
                   help="identity probe: source vs its own reference (must be ~0)")
    return p.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    if args.check:
        return cmd_check()
    if args.build_reference:
        return cmd_build()
    if args.compare:
        return cmd_compare(args.compare)
    if args.self:
        return cmd_self()
    # Default: build (mirrors Task 57 default).
    return cmd_build()


if __name__ == "__main__":
    sys.exit(main())
