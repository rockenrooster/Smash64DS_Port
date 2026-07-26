#!/usr/bin/env python3
"""Task 63 E0 — constrained reduction census for the Dream Land static stage.

Measures how much submitted-vertex reduction is actually available under the
Revision 2 Part II constraints, BEFORE any runtime code is changed:

  * every emitted corner must resolve to exactly one source dense vertex
    (subset placement only -- no averaged/synthesized vertices);
  * projected-no-Z runs may only have whole triangles dropped, never
    re-indexed and never moved (a billboard's silhouette is its texture);
  * raw-Z / range runs may be collapsed onto surviving source vertices.

This script reports the *lossless* levers first (never-visible and degenerate
triangles), because those cost no fidelity at all and therefore bound the
risk-free part of the win. Collapse ladders are reported separately.

Host-only. Reads Task 57's IR and Task 58's camera fixtures; writes nothing
unless --json is given.

CAVEAT recorded in the output: Task 57's IR bakes *descriptor* world matrices,
not the live DObj transforms, so the three pass-through platforms sit lower
here than they do at runtime. Visibility conclusions are therefore reported at
several screen margins so the sensitivity to that error is visible rather than
hidden. Only culls that survive the widest margin should be treated as safe.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any, Sequence

import dreamland_camera_oracle as oracle

SCRIPTS_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPTS_DIR.parent
GENERATED_DIR = SCRIPTS_DIR / "generated"
WORLD_MESH_IR = GENERATED_DIR / "dreamland_world_mesh.json"

# Submit classes (mirror NDS_RENDERER_HW_SUBMIT_* in nds_renderer.c).
SUBMIT_RAW_Z = 0
SUBMIT_PROJECTED_NO_Z = 3
SUBMIT_PROJECTED_RANGE_OR_MATRIX = 6
SUBMIT_NAMES = {
    SUBMIT_RAW_Z: "raw_z",
    SUBMIT_PROJECTED_NO_Z: "projected_no_z",
    SUBMIT_PROJECTED_RANGE_OR_MATRIX: "range_or_matrix",
}

# Screen margins (fraction of viewport) at which the off-screen cull is
# re-evaluated. 0.00 = strictly outside the viewport; larger = the triangle
# must miss by that much before we believe it, absorbing the baked-matrix
# platform error described in the module docstring.
MARGINS = (0.00, 0.10, 0.25, 0.50)

# The E0 STOP threshold from the Revision 2 plan.
STOP_THRESHOLD_PCT = 15.0

SOURCE_CORNERS = 525


def load_ir() -> dict[str, Any]:
    return json.loads(WORLD_MESH_IR.read_text(encoding="utf-8"))


def fighting_surface_envelope(ir: dict[str, Any]) -> dict[str, list[float]]:
    """Bounds of the raw-Z island geometry -- what the camera actually frames.

    Task 58 derived its fixtures from the *whole* static envelope, which
    includes the sky dome (y up to 3816) and the background cards (x +/-3249).
    That put the look-at point at y~2251 -- empty sky well above an island that
    lives at y 790..1543 -- and pushed the eye 1.6x further out than gameplay
    framing. Everything landed on screen, so no cull could ever be detected.

    The gameplay camera frames the surfaces fighters stand on. Those are
    exactly the raw-Z triangles (submit class 0); the range/matrix spans are
    the water plane below and the no-Z cards are backdrop.
    """
    verts = ir["world_vertices"]
    lo = [float("inf")] * 3
    hi = [float("-inf")] * 3
    for t in ir["triangles"]:
        if int(t["submit_class"]) != SUBMIT_RAW_Z:
            continue
        for key in ("v0", "v1", "v2"):
            v = verts[int(t[key])]
            for axis, name in enumerate(("world_x_f", "world_y_f",
                                         "world_z_f")):
                lo[axis] = min(lo[axis], v[name])
                hi[axis] = max(hi[axis], v[name])
    return {
        "min": lo,
        "max": hi,
        "extent": [hi[i] - lo[i] for i in range(3)],
        "center": [(lo[i] + hi[i]) / 2.0 for i in range(3)],
    }


def gameplay_fixtures(ir: dict[str, Any]) -> list[oracle.Fixture]:
    """Camera fixtures framed on the fighting surface, not the sky dome."""
    env = fighting_surface_envelope(ir)
    cx, cy, cz = env["center"]
    ext = env["extent"]
    half_x = ext[0] / 2.0
    # Headroom above the surface for jumps/knockback, in surface-height units.
    at_y = cy + ext[1] * 0.25
    up = (0.0, 1.0, 0.0)
    fovy = oracle.FOVY_DEG
    zoom = oracle.ZOOM_RANGE_2P

    def dist_for(mult: float) -> float:
        return oracle._gameplay_eye_distance(half_x * zoom * mult, fovy)

    base = dist_for(1.0)
    near = max(100.0, ext[2] * 0.05)
    far = base * 4.0 + 8000.0
    eye_up = ext[1] * 0.15

    out: list[oracle.Fixture] = [
        oracle.Fixture("gp_center_medium", (cx, at_y + eye_up, cz + base),
                       (cx, at_y, cz), up, fovy, near, far,
                       "fighting-surface framing, base 2P zoom"),
        oracle.Fixture("gp_center_close", (cx, at_y + eye_up, cz + dist_for(0.72)),
                       (cx, at_y, cz), up, fovy, near, far,
                       "close zoom"),
        oracle.Fixture("gp_center_maxzoom",
                       (cx, at_y + eye_up, cz + dist_for(1.28)),
                       (cx, at_y, cz), up, fovy, near, far,
                       "maximum normal 2P zoom-out"),
    ]
    for sign, name in ((-1, "gp_left_medium"), (1, "gp_right_medium")):
        edge_x = cx + sign * half_x * 0.78
        out.append(oracle.Fixture(
            name, (edge_x, at_y + eye_up, cz + base), (edge_x, at_y, cz),
            up, fovy, near, far, "horizontal framing"))
    for sign, name in ((1, "gp_high_frame"), (-1, "gp_low_frame")):
        fy = at_y + sign * ext[1] * 0.60
        out.append(oracle.Fixture(
            name, (cx, fy + eye_up, cz + base), (cx, fy, cz),
            up, fovy, near, far, "vertical framing"))
    out.append(oracle.Fixture(
        "gp_respawn_frame", (cx, at_y + ext[1] * 1.10, cz + dist_for(1.18)),
        (cx, at_y + ext[1] * 0.45, cz), up, fovy, near, far,
        "KO/respawn framing (wider + higher)"))
    return out


def triangle_visibility(
    verts: Sequence[dict[str, Any]],
    tri: dict[str, Any],
    vp: list[list[float]],
    margin: float,
) -> bool:
    """Conservative: True if the triangle *might* touch the viewport.

    A triangle straddling the near plane (mixed w signs) always counts as
    visible -- clipping would produce on-screen fragments we cannot cheaply
    predict here. Only a triangle entirely behind the eye, or whose whole
    projected bounding box misses the expanded viewport, is called invisible.
    """
    w = oracle.VIEWPORT_W_PX
    h = oracle.VIEWPORT_H_PX
    mx = w * margin
    my = h * margin

    pts: list[tuple[float, float]] = []
    behind = 0
    for key in ("v0", "v1", "v2"):
        v = verts[int(tri[key])]
        sx, sy, _, cw = oracle.project_point(
            (v["world_x_f"], v["world_y_f"], v["world_z_f"]), vp)
        if cw <= 0.0:
            behind += 1
            continue
        pts.append((sx, sy))

    if behind == 3:
        return False          # entirely behind the eye
    if behind > 0:
        return True           # straddles the near plane -- assume visible
    if not pts:
        return True

    min_x = min(p[0] for p in pts)
    max_x = max(p[0] for p in pts)
    min_y = min(p[1] for p in pts)
    max_y = max(p[1] for p in pts)
    if max_x < -mx or min_x > w + mx:
        return False
    if max_y < -my or min_y > h + my:
        return False
    return True


def world_area(verts: Sequence[dict[str, Any]], tri: dict[str, Any]) -> float:
    p = []
    for key in ("v0", "v1", "v2"):
        v = verts[int(tri[key])]
        p.append((v["world_x_f"], v["world_y_f"], v["world_z_f"]))
    ux = [p[1][i] - p[0][i] for i in range(3)]
    vx = [p[2][i] - p[0][i] for i in range(3)]
    cx = ux[1] * vx[2] - ux[2] * vx[1]
    cy = ux[2] * vx[0] - ux[0] * vx[2]
    cz = ux[0] * vx[1] - ux[1] * vx[0]
    return 0.5 * math.sqrt(cx * cx + cy * cy + cz * cz)


def run_census() -> dict[str, Any]:
    ir = load_ir()
    verts = ir["world_vertices"]
    tris = ir["triangles"]
    # Two fixture sets: Task 58's envelope-derived set (kept for continuity
    # with the Task 59/60 numbers) and a gameplay set framed on the fighting
    # surface. A cull is only claimed when it holds under BOTH.
    fixture_sets = {
        "task58_envelope": oracle.build_fixtures(),
        "gameplay_surface": gameplay_fixtures(ir),
    }
    fixtures = fixture_sets["task58_envelope"] + fixture_sets["gameplay_surface"]
    vps = [(f.name, oracle.view_projection(f)) for f in fixtures]

    by_class: dict[int, int] = {}
    for t in tris:
        by_class[int(t["submit_class"])] = by_class.get(
            int(t["submit_class"]), 0) + 1

    # Degenerate (zero-area) triangles: free to drop under any constraint.
    degenerate = [i for i, t in enumerate(tris) if world_area(verts, t) <= 0.0]

    # Never-visible triangles, per fixture set and per margin. A triangle is
    # only claimed hidden when it is hidden under EVERY fixture of the set.
    per_set: dict[str, dict[float, list[int]]] = {}
    for set_name, set_fixtures in fixture_sets.items():
        set_vps = [oracle.view_projection(f) for f in set_fixtures]
        per_set[set_name] = {}
        for margin in MARGINS:
            per_set[set_name][margin] = [
                i for i, t in enumerate(tris)
                if all(not triangle_visibility(verts, t, vp, margin)
                       for vp in set_vps)
            ]
    # The claim requires agreement across both fixture sets.
    never_visible: dict[float, list[int]] = {
        m: sorted(set(per_set["task58_envelope"][m])
                  & set(per_set["gameplay_surface"][m]))
        for m in MARGINS
    }

    report: dict[str, Any] = {
        "task": "Task 63 E0 — constrained reduction census",
        "version": 1,
        "source": {
            "triangles": len(tris),
            "corners": len(tris) * 3,
            "dense_vertices": len(verts),
            "by_submit_class": {
                SUBMIT_NAMES.get(k, str(k)): v
                for k, v in sorted(by_class.items())
            },
        },
        "fixtures": [f.name for f in fixtures],
        "caveat": (
            "Task 57's IR bakes descriptor world matrices, not live DObj "
            "transforms; pass-through platforms sit lower here than at "
            "runtime. Trust only culls that survive the widest margin."
        ),
        "lossless_levers": {
            "degenerate_triangles": len(degenerate),
            "never_visible_per_fixture_set": {
                set_name: {
                    f"{m:.2f}": len(per_set[set_name][m]) for m in MARGINS
                }
                for set_name in fixture_sets
            },
            "never_visible_by_margin": {
                f"{m:.2f}": {
                    "triangles": len(never_visible[m]),
                    "by_submit_class": _class_breakdown(
                        tris, never_visible[m]),
                }
                for m in MARGINS
            },
        },
    }

    # ---- constrained collapse ladder on the raw-Z / range subset ----
    positions, sub_tris, sub_keys, _dense = build_collapsible_submesh(
        ir, (SUBMIT_RAW_Z, SUBMIT_PROJECTED_RANGE_OR_MATRIX))
    boundary = _boundary_vertices(sub_tris)
    fixed_tris = len(tris) - len(sub_tris)      # no-Z cards, kept whole
    ladder: dict[str, list[dict[str, Any]]] = {}
    for policy, locked in (("boundary_locked", boundary), ("unlocked", set())):
        rows: list[dict[str, Any]] = []
        for budget in (60, 55, 50, 45, 40, 35, 30, 25):
            cand, _, survivors = collapse_subset_placement(
                positions, sub_tris, sub_keys, set(locked), budget)
            iou = _iou_against_source(positions, sub_tris, cand, fixtures)
            total_tris = len(cand) + fixed_tris
            corners = total_tris * 3
            rows.append({
                "budget": budget,
                "subset_tris": len(cand),
                "total_tris": total_tris,
                "corners": corners,
                "reduction_pct": round(
                    100.0 * (SOURCE_CORNERS - corners) / SOURCE_CORNERS, 2),
                "subset_worst_iou": round(iou, 4),
                "surviving_vertices": len(survivors),
            })
            if len(cand) <= budget and rows[-1]["subset_tris"] == rows[-1][
                    "subset_tris"]:
                pass
        ladder[policy] = rows
    report_ladder = {
        "collapsible_subset": {
            "triangles": len(sub_tris),
            "vertices": len(positions),
            "boundary_vertices": len(boundary),
            "interior_vertices": len(positions) - len(boundary),
            "note": (
                "raw-Z + range only; the 99 projected-no-Z alpha cards are "
                "drop-only under Part II.5 and are held whole here"
            ),
        },
        "fixed_no_z_triangles": fixed_tris,
        "ladder": ladder,
    }

    # Free (fidelity-zero) reduction at the most conservative margin.
    safest = MARGINS[-1]
    free = sorted(set(degenerate) | set(never_visible[safest]))
    free_corners = len(free) * 3
    report["lossless_summary"] = {
        "margin_used": safest,
        "triangles_free": len(free),
        "corners_saved": free_corners,
        "reduction_pct": round(100.0 * free_corners / SOURCE_CORNERS, 2),
        "by_submit_class": _class_breakdown(tris, free),
    }
    report["collapse"] = report_ladder

    # Verdict: best reduction that holds the subset silhouette at IoU >= 0.95.
    best = None
    for policy, rows in ladder.items():
        for row in rows:
            if row["subset_worst_iou"] < 0.95:
                continue
            if best is None or row["reduction_pct"] > best[1]["reduction_pct"]:
                best = (policy, row)
    report["verdict"] = {
        "stop_threshold_pct": STOP_THRESHOLD_PCT,
        "best_acceptable": None if best is None else {
            "policy": best[0], **best[1]},
        "proceed": bool(best is not None
                        and best[1]["reduction_pct"] >= STOP_THRESHOLD_PCT),
    }
    return report


# ---------------------------------------------------------------------------
# Constrained collapse ladder (raw-Z / range geometry only)
# ---------------------------------------------------------------------------

def build_collapsible_submesh(
    ir: dict[str, Any], classes: Sequence[int]
) -> tuple[list[tuple[float, float, float]],
           list[tuple[int, int, int]],
           list[tuple[int, int]],
           list[int]]:
    """Extract the collapsible subset as a standalone mesh.

    Returns (positions, triangles, per-triangle (run, epoch) key, source dense
    index per local vertex). Only raw-Z / range geometry is eligible: no-Z
    alpha cards are drop-only under Revision 2 Part II.5.
    """
    verts = ir["world_vertices"]
    remap: dict[int, int] = {}
    positions: list[tuple[float, float, float]] = []
    dense: list[int] = []
    tris: list[tuple[int, int, int]] = []
    keys: list[tuple[int, int]] = []
    for t in ir["triangles"]:
        if int(t["submit_class"]) not in classes:
            continue
        local: list[int] = []
        for key in ("v0", "v1", "v2"):
            gi = int(t[key])
            if gi not in remap:
                remap[gi] = len(positions)
                v = verts[gi]
                positions.append(
                    (v["world_x_f"], v["world_y_f"], v["world_z_f"]))
                dense.append(int(v.get("source_dense_index", -1)))
            local.append(remap[gi])
        tris.append((local[0], local[1], local[2]))
        keys.append((int(t["run_index"]), int(t["texture_epoch"])))
    return positions, tris, keys, dense


def _boundary_vertices(tris: Sequence[tuple[int, int, int]]) -> set[int]:
    counts: dict[tuple[int, int], int] = {}
    for v0, v1, v2 in tris:
        for a, b in ((v0, v1), (v1, v2), (v0, v2)):
            e = (a, b) if a < b else (b, a)
            counts[e] = counts.get(e, 0) + 1
    out: set[int] = set()
    for (a, b), n in counts.items():
        if n == 1:
            out.add(a)
            out.add(b)
    return out


def _quadrics(positions: Sequence[tuple[float, float, float]],
              tris: Sequence[tuple[int, int, int]]) -> list[list[float]]:
    """Per-vertex quadric as a flat 10-float symmetric 4x4 accumulator."""
    q = [[0.0] * 10 for _ in positions]
    for v0, v1, v2 in tris:
        p0, p1, p2 = positions[v0], positions[v1], positions[v2]
        ux = [p1[i] - p0[i] for i in range(3)]
        vx = [p2[i] - p0[i] for i in range(3)]
        nx = ux[1] * vx[2] - ux[2] * vx[1]
        ny = ux[2] * vx[0] - ux[0] * vx[2]
        nz = ux[0] * vx[1] - ux[1] * vx[0]
        L = math.sqrt(nx * nx + ny * ny + nz * nz)
        if L <= 1e-12:
            continue
        a, b, c = nx / L, ny / L, nz / L
        d = -(a * p0[0] + b * p0[1] + c * p0[2])
        coeff = [a * a, a * b, a * c, a * d,
                 b * b, b * c, b * d,
                 c * c, c * d, d * d]
        for vi in (v0, v1, v2):
            for k in range(10):
                q[vi][k] += coeff[k]
    return q


def _quadric_error(q: Sequence[float], p: Sequence[float]) -> float:
    x, y, z = p
    return (q[0] * x * x + 2 * q[1] * x * y + 2 * q[2] * x * z + 2 * q[3] * x
            + q[4] * y * y + 2 * q[5] * y * z + 2 * q[6] * y
            + q[7] * z * z + 2 * q[8] * z + q[9])


def collapse_subset_placement(
    positions: Sequence[tuple[float, float, float]],
    tris: Sequence[tuple[int, int, int]],
    keys: Sequence[tuple[int, int]],
    locked: set[int],
    target_tris: int,
) -> tuple[list[tuple[int, int, int]], list[int], set[int]]:
    """Greedy quadric edge collapse restricted to SUBSET PLACEMENT.

    The survivor of every collapse is one of the two original endpoints, so
    every surviving vertex is still a real source vertex with its own exact
    UV / colour / texture epoch. This is the Revision 2 Part II.1 invariant --
    it is what makes the result material-correct by construction.

    A vertex whose incident triangles span more than one (run, epoch) group,
    or whose group set differs from its partner's, is never collapsed: that
    would merge across a material seam.
    """
    tris = [tuple(t) for t in tris]
    keys = list(keys)
    q = _quadrics(positions, tris)
    parent = list(range(len(positions)))

    def find(x: int) -> int:
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def groups_of(alive_tris: Sequence[tuple[int, int, int]]
                  ) -> dict[int, set[tuple[int, int]]]:
        g: dict[int, set[tuple[int, int]]] = {}
        for ti, (v0, v1, v2) in enumerate(alive_tris):
            for vi in (v0, v1, v2):
                g.setdefault(vi, set()).add(keys[ti])
        return g

    while True:
        live = [(find(a), find(b), find(c)) for a, b, c in tris]
        live_idx = [i for i, t in enumerate(live)
                    if t[0] != t[1] and t[1] != t[2] and t[0] != t[2]]
        if len(live_idx) <= target_tris:
            break
        cur = [live[i] for i in live_idx]
        cur_keys = [keys[i] for i in live_idx]
        vgroups: dict[int, set[tuple[int, int]]] = {}
        for ti, (v0, v1, v2) in enumerate(cur):
            for vi in (v0, v1, v2):
                vgroups.setdefault(vi, set()).add(cur_keys[ti])

        edges: set[tuple[int, int]] = set()
        for v0, v1, v2 in cur:
            for a, b in ((v0, v1), (v1, v2), (v0, v2)):
                edges.add((a, b) if a < b else (b, a))

        best: tuple[float, int, int] | None = None
        for a, b in sorted(edges):
            if a in locked and b in locked:
                continue
            if vgroups.get(a) != vgroups.get(b):
                continue          # material seam -- never merge across it
            if len(vgroups.get(a, ())) != 1:
                continue          # vertex sits on a seam itself
            qa = [q[a][k] + q[b][k] for k in range(10)]
            # Subset placement: the survivor must be an existing vertex.
            options = []
            if b not in locked:
                options.append((_quadric_error(qa, positions[a]), a, b))
            if a not in locked:
                options.append((_quadric_error(qa, positions[b]), b, a))
            if not options:
                continue
            cost, keep, drop = min(options)
            cand = (cost, keep, drop)
            if best is None or cand < best:
                best = cand
        if best is None:
            break
        _, keep, drop = best
        parent[drop] = keep
        for k in range(10):
            q[keep][k] += q[drop][k]

    live = [(find(a), find(b), find(c)) for a, b, c in tris]
    out_tris: list[tuple[int, int, int]] = []
    out_keys: list[int] = []
    for i, t in enumerate(live):
        if t[0] == t[1] or t[1] == t[2] or t[0] == t[2]:
            continue
        out_tris.append(t)
        out_keys.append(i)
    survivors = {v for t in out_tris for v in t}
    return out_tris, out_keys, survivors


def _iou_against_source(
    positions: Sequence[tuple[float, float, float]],
    src_tris: Sequence[tuple[int, int, int]],
    cand_tris: Sequence[tuple[int, int, int]],
    fixtures: Sequence[oracle.Fixture],
) -> float:
    """Worst-case region IoU over the fixtures, on the COLLAPSIBLE SUBSET only.

    Measuring on the full mesh would be meaningless here: the 99 untouched
    no-Z backdrop cards dominate the screen mask, so even a destroyed island
    would score near 1.0. The subset is what changes, so the subset is what
    gets measured.
    """
    verts = tuple(oracle.MeshVertex(p[0], p[1], p[2], 0) for p in positions)
    src = oracle.Mesh(verts, tuple(
        oracle.MeshTriangle(*t) for t in src_tris))
    cand = oracle.Mesh(verts, tuple(
        oracle.MeshTriangle(*t) for t in cand_tris))
    worst = 1.0
    for f in fixtures:
        sp = oracle.project_mesh(src, f, set())
        cp = oracle.project_mesh(cand, f, set())
        m = oracle.displacement_metrics(cp, sp)
        worst = min(worst, m["region_iou"])
    return worst


def _class_breakdown(tris: Sequence[dict[str, Any]],
                     indices: Sequence[int]) -> dict[str, int]:
    out: dict[str, int] = {}
    for i in indices:
        name = SUBMIT_NAMES.get(int(tris[i]["submit_class"]),
                                str(tris[i]["submit_class"]))
        out[name] = out.get(name, 0) + 1
    return out


def print_report(report: dict[str, Any]) -> None:
    src = report["source"]
    print("Task 63 E0 — constrained reduction census")
    print("=" * 62)
    print(f"source: {src['triangles']} triangles, {src['corners']} corners, "
          f"{src['dense_vertices']} dense vertices")
    print("        by submit class: " + ", ".join(
        f"{k}={v}" for k, v in src["by_submit_class"].items()))
    print(f"fixtures: {len(report['fixtures'])} "
          f"({', '.join(report['fixtures'])})")
    print()
    lossless = report["lossless_levers"]
    print(f"degenerate (zero-area) triangles: "
          f"{lossless['degenerate_triangles']}")
    print()
    print("never-visible triangles per fixture set (tris hidden in ALL its views):")
    for set_name, rows in lossless["never_visible_per_fixture_set"].items():
        print(f"  {set_name:20s} " + "  ".join(
            f"m{m}={n}" for m, n in rows.items()))
    print()
    print("never-visible triangles (agreed by BOTH fixture sets) by margin:")
    print(f"  {'margin':>8}  {'tris':>5}  breakdown")
    for margin, entry in lossless["never_visible_by_margin"].items():
        bd = ", ".join(f"{k}={v}" for k, v in entry["by_submit_class"].items())
        print(f"  {margin:>8}  {entry['triangles']:>5}  {bd or '-'}")
    print()
    s = report["lossless_summary"]
    print(f"LOSSLESS TOTAL (margin {s['margin_used']:.2f}): "
          f"{s['triangles_free']} triangles = {s['corners_saved']} corners "
          f"= {s['reduction_pct']}% of {SOURCE_CORNERS}")
    print(f"  by submit class: " + (", ".join(
        f"{k}={v}" for k, v in s["by_submit_class"].items()) or "-"))
    print()

    col = report["collapse"]
    sub = col["collapsible_subset"]
    print(f"collapsible subset (raw-Z + range): {sub['triangles']} triangles, "
          f"{sub['vertices']} vertices "
          f"({sub['boundary_vertices']} boundary / "
          f"{sub['interior_vertices']} interior)")
    print(f"held whole (no-Z alpha cards): {col['fixed_no_z_triangles']} "
          f"triangles")
    print()
    for policy, rows in col["ladder"].items():
        print(f"collapse ladder [{policy}]:")
        print(f"  {'budget':>6} {'subset':>7} {'total':>6} {'corners':>8} "
              f"{'reduce':>7} {'subsetIoU':>10}")
        for r in rows:
            mark = "  OK" if r["subset_worst_iou"] >= 0.95 else "  reject"
            print(f"  {r['budget']:>6} {r['subset_tris']:>7} "
                  f"{r['total_tris']:>6} {r['corners']:>8} "
                  f"{r['reduction_pct']:>6.1f}% {r['subset_worst_iou']:>10.4f}"
                  f"{mark}")
        print()

    v = report["verdict"]
    print("=" * 62)
    if v["best_acceptable"] is None:
        print(f"E0 VERDICT: STOP — no candidate holds subset IoU >= 0.95.")
    else:
        b = v["best_acceptable"]
        print(f"E0 VERDICT: {'PROCEED' if v['proceed'] else 'STOP'} — best "
              f"acceptable = {b['policy']} budget {b['budget']}: "
              f"{b['corners']} corners, {b['reduction_pct']}% reduction "
              f"(subset IoU {b['subset_worst_iou']})")
        print(f"            STOP threshold is "
              f"{v['stop_threshold_pct']}% reduction.")


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", type=Path, default=None,
                        help="write the report as JSON to this path")
    args = parser.parse_args(argv)

    report = run_census()
    print_report(report)
    if args.json is not None:
        args.json.write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n",
            encoding="utf-8")
        print(f"\nwrote {args.json}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
