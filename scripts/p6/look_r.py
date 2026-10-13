#!/usr/bin/env python3
"""Track R (P6 A3c): the base's model restyled toward the fitted kit model.

Ops (committed, art/p6/<name>/look/r.json) say where each base vertex goes,
in the forms plan G0 allows: an anchor on the fitted kit's surface (kit mesh,
triangle, barycentric coordinates, an offset along the kit normal) or a free
offset from the base position. No kit coordinate is stored: an anchor is
re-evaluated from the kit at build time, so the ops carry no kit data (D10).

`shrinkwrap` writes the first ops: every base vertex anchored at the closest
point of the kit surface its region allows (REGIONS: base joint -> the kit
joints whose triangles it may land on). `apply` evaluates ops against a
fitted kit (kit_fit.py) and writes the restyled scene.json beside the base's,
normals recomputed from the moved faces, for import_scene.py and the writer.
"""
from __future__ import annotations

import argparse
import copy
import json
import math
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import kit_fit as F  # noqa: E402

# Base joint -> the base joints whose kit regions its vertices may land on
# (a kit triangle's region is the base joint its dominant kit joint maps to;
# unmapped kit joints map through KIT_EXTRA).
REGIONS = {
    "peach": {
        2: {1, 2}, 4: {4}, 5: {5}, 6: {6}, 7: {7, 8}, 8: {8, 9}, 9: {9},
        11: {11}, 12: {12}, 13: {13}, 17: {17, 16}, 19: {19}, 22: {22, 21}, 24: {24},
        25: {1, 25, 26}, 26: {1, 25, 26},
    },
}
KIT_EXTRA = {
    "peach": {
        "TransN": 1, "ThrowN": 1,
        "LSkirtJa": 26, "LSkirtJb": 26, "RSkirtJa": 26, "RSkirtJb": 26,
        # The long hanging hair (the swing chains) is Peach's back hair's
        # (joint 9) to land on; her head hair (8) may land on it too.
        "SWG_LHair1__swing": 9, "SWG_LHair3__swing": 9,
        "SWG_RHair1__swing": 9, "SWG_RHair3__swing": 9,
        "SWG_String0__swing": 5,
    },
    # Ike's kit on Roy: the left shoulder armour on Roy's left pad (20),
    # the headband's tails on the head, the cape's swing chains on Roy's cape
    # (15-18).
    "roy": {
        "TransN": 1, "ThrowN": 1, "LArmorN": 20, "SWG_hatiA1__swing": 9,
        "SWG_RMant1__swing": 16, "SWG_LMant1__swing": 16,
        "SWG_RMant3__swing": 17, "SWG_LMant3__swing": 17,
    },
}


def kit_triangles(fit: dict, role: str):
    """Every fitted kit triangle: (mesh, index, a, b, c, region)."""
    role_map = dict(F.ROLE_MAPS[role])
    role_map.update(KIT_EXTRA[role])
    out = []
    for mesh in fit["meshes"]:
        pos = np.array(mesh["positions"], dtype=float)
        for t, (a, b, c) in enumerate(mesh["triangles"]):
            weights = {}
            for v in (a, b, c):
                for name, w in mesh["weights"][v]:
                    weights[name] = weights.get(name, 0.0) + w
            top = max(weights, key=weights.get) if weights else "TransN"
            out.append((mesh["name"], t, pos[a], pos[b], pos[c], role_map.get(top, -1)))
    return out


def closest_on_triangles(p, A, B, C):
    """Closest points of point p on triangles (A, B, C arrays): (point, u, v, w, d2)."""
    ab = B - A
    ac = C - A
    ap = p - A
    d1 = (ab * ap).sum(1)
    d2 = (ac * ap).sum(1)
    bp = p - B
    d3 = (ab * bp).sum(1)
    d4 = (ac * bp).sum(1)
    cp = p - C
    d5 = (ab * cp).sum(1)
    d6 = (ac * cp).sum(1)
    va = d3 * d6 - d5 * d4
    vb = d5 * d2 - d1 * d6
    vc = d1 * d4 - d3 * d2
    denom = va + vb + vc
    denom = np.where(np.abs(denom) < 1e-12, 1e-12, denom)
    v = vb / denom
    w = vc / denom
    # Clamp to the triangle: region tests in order (Ericson, RTCD 5.1.5).
    u = 1 - v - w
    bary = np.stack([u, v, w], 1)
    # Vertex and edge regions.
    t_ab = np.clip(d1 / np.where((d1 - d3) == 0, 1e-12, d1 - d3), 0, 1)
    t_ac = np.clip(d2 / np.where((d2 - d6) == 0, 1e-12, d2 - d6), 0, 1)
    t_bc = np.clip((d4 - d3) / np.where(((d4 - d3) + (d5 - d6)) == 0, 1e-12, (d4 - d3) + (d5 - d6)), 0, 1)
    cand = [
        (np.stack([1 - t_ab, t_ab, 0 * t_ab], 1)),
        (np.stack([1 - t_ac, 0 * t_ac, t_ac], 1)),
        (np.stack([0 * t_bc, 1 - t_bc, t_bc], 1)),
    ]
    inside = (u >= 0) & (v >= 0) & (w >= 0)
    best = np.where(inside[:, None], bary, cand[0])
    def point(b):
        return A * b[:, 0:1] + B * b[:, 1:2] + C * b[:, 2:3]
    bp_ = point(best)
    bd = ((bp_ - p) ** 2).sum(1)
    for c in cand[1:]:
        cpnt = point(c)
        cd = ((cpnt - p) ** 2).sum(1)
        take = (~inside) & (cd < bd)
        best = np.where(take[:, None], c, best)
        bd = np.where(take, cd, bd)
    return point(best), best, bd


# Each base joint's axis, as two points: a joint row, "top" (the model's top:
# Peach's crown, the look's hair), "floor", or a joint plus a fixed direction
# ("+x", "-x", "fwd": hands and feet keep their size). A vertex maps from the
# base's axis onto the look's -- its place along the axis scaled by the two
# lengths, its offset from the axis turned with it -- so the proportions are
# the look's (owner, 2026-10-13: Peach's model with the baseline's
# proportions); then it is pushed out from the look's axis to the first kit
# surface of its region (the outermost for cloth), else to the closest.
AXES = {
    "peach": {
        2: (2, 7), 4: (4, 5), 5: (5, 6), 6: (6, "+x"), 7: (7, 8), 8: (8, "top"),
        9: ("hair_root", "hair_end"),
        11: (11, 12), 12: (12, 13), 13: (13, "-x"), 17: (17, 19), 19: (19, "fwd"),
        22: (22, 24), 24: (24, "fwd"), 25: (1, "floor"), 26: (1, "floor"),
    },
}
# Joints whose radial ray takes the outermost hit of its region (cloth
# layers), not the first.
OUTERMOST = {"peach": {25, 26}}
# Kit joints whose vertices end the long hair (the swing chains' last links).
HAIR_END_JOINTS = {"SWG_LHair3__swing", "SWG_RHair3__swing"}
# Joints kept rigid at the base's size (owner: Peach's hands, her shoes).
RIGID = {"peach": {6, 13, 19, 24}}
# Joints mapped along their axis to the look's length but keeping the base's
# own shape around it (owner 2026-10-13: Peach's calves).
AXIS_ONLY = {"peach": {17, 22}}
_DIRS = {"+x": (1.0, 0.0, 0.0), "-x": (-1.0, 0.0, 0.0), "fwd": (0.0, 0.0, 1.0)}


# Joints fitted by horizontal slices (owner 2026-10-13: the hair missed a
# chunk, the dress and bodice did not meet): a vertex goes to the outer hull
# of its region's kit slice at its height, in its own direction from its
# axis, so a ring spans the kit's whole cross-section, however flat.
SLICE = {"peach": {2, 9, 25, 26}}


def _slice_anchor(p, o, r, sel, A, B, C):
    """(kit mesh, triangle, barycentric) of the outermost crossing of the
    region's slice at height p.y along r's horizontal direction from the
    slice's centre, or None when the region does not reach that height."""
    y = p[1]
    sa, sb, sc = A[:, 1] - y, B[:, 1] - y, C[:, 1] - y
    segs_p, segs_q, owner = [], [], []
    for k in range(len(A)):
        pts = []
        for (u, v, su, sv) in ((A[k], B[k], sa[k], sb[k]), (B[k], C[k], sb[k], sc[k]), (C[k], A[k], sc[k], sa[k])):
            if su * sv < 0:
                t = su / (su - sv)
                pts.append(u + (v - u) * t)
        if len(pts) >= 2:
            segs_p.append(pts[0])
            segs_q.append(pts[1])
            owner.append(k)
    if not segs_p:
        return None
    P = np.array(segs_p)
    Q = np.array(segs_q)
    mid = (P + Q) / 2
    w = np.linalg.norm(Q - P, axis=1) + 1e-9
    centre = (mid * w[:, None]).sum(0) / w.sum()
    d = np.array([r[0], 0.0, r[2]])
    d /= np.linalg.norm(d)
    p2 = (P - centre)[:, [0, 2]]
    seg = (Q - P)[:, [0, 2]]
    rd = d[[0, 2]]
    den = rd[0] * seg[:, 1] - rd[1] * seg[:, 0]
    ok = np.abs(den) > 1e-12
    safe = np.where(ok, den, 1.0)
    sdist = np.where(ok, (p2[:, 0] * seg[:, 1] - p2[:, 1] * seg[:, 0]) / safe, -1.0)
    u = np.where(ok, (p2[:, 0] * rd[1] - p2[:, 1] * rd[0]) / safe, -1.0)
    hit = ok & (sdist >= 0) & (u >= 0) & (u <= 1)
    if not hit.any():
        return None
    k = int(np.argmax(np.where(hit, sdist, -np.inf)))
    point = P[k] + (Q[k] - P[k]) * u[k]
    t = owner[k]
    # Barycentric of the hit on its kit triangle.
    a, b, c = A[t], B[t], C[t]
    v0, v1, v2 = b - a, c - a, point - a
    d00, d01, d11 = v0 @ v0, v0 @ v1, v1 @ v1
    d20, d21 = v2 @ v0, v2 @ v1
    den2 = d00 * d11 - d01 * d01
    if abs(den2) < 1e-12:
        return None
    bv = (d11 * d20 - d01 * d21) / den2
    bw = (d00 * d21 - d01 * d20) / den2
    return (sel[t][0], sel[t][1], [1.0 - bv - bw, bv, bw])


def _ray_hits(o, d, A, B, C):
    """Distances along the ray (o, d) to triangles (A, B, C) it crosses
    (Moller-Trumbore, both faces), inf where it misses; with barycentrics."""
    e1 = B - A
    e2 = C - A
    pv = np.cross(d, e2)
    det = (e1 * pv).sum(1)
    ok = np.abs(det) > 1e-12
    inv = np.where(ok, 1.0 / np.where(ok, det, 1.0), 0.0)
    tv = o - A
    u = (tv * pv).sum(1) * inv
    qv = np.cross(tv, e1)
    v = (qv * d).sum(1) * inv
    t = (e2 * qv).sum(1) * inv
    hit = ok & (u >= 0) & (v >= 0) & (u + v <= 1) & (t > 1e-6)
    t = np.where(hit, t, np.inf)
    return t, u, v


def _rotation(a, b):
    """3x3 rotation taking unit a to unit b."""
    a = a / np.linalg.norm(a)
    b = b / np.linalg.norm(b)
    v = np.cross(a, b)
    c = float(a @ b)
    if c < -0.999999:
        axis = np.cross(a, (1.0, 0.0, 0.0))
        if np.linalg.norm(axis) < 1e-6:
            axis = np.cross(a, (0.0, 1.0, 0.0))
        axis /= np.linalg.norm(axis)
        return 2 * np.outer(axis, axis) - np.eye(3)
    vx = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + vx + vx @ vx / (1.0 + c)


def shrinkwrap(base: dict, fit: dict, role: str, joints: set | None = None) -> dict:
    tris = kit_triangles(fit, role)
    regions = REGIONS[role]
    axes = AXES.get(role, {})
    ops = {"form": "p6-look-ops-v1", "track": "R", "base_role": role, "vertices": []}
    skel = fit.get("skeleton")
    base_ys = [v["model"][1] for v in base["vertices"]]
    body = [m for m in fit["meshes"] if not m["name"].startswith("weapon")]
    look_ys = [q[1] for m in body for q in m["positions"]]
    tops = {"base": max(base_ys), "look": max(look_ys)}
    floors = {"base": min(base_ys), "look": min(look_ys)}

    def base_joint(j):
        w = base["joints"][j]["world"]
        return np.array([w[0][3], w[1][3], w[2][3]], dtype=float)

    def look_joint(j):
        return np.array(skel[j], dtype=float) if skel else base_joint(j)

    # The long hair (Peach's back hair, joint 9): from the top of its own
    # vertices down to their end on the base; on the look, from the same root
    # (ridden with the joint) down to where the kit's hair ends.
    hair = np.array([v["model"] for v in base["vertices"] if v["joint"] == 9], dtype=float)
    kit_hair = np.array([q for m in fit["meshes"] for q, ws in zip(m["positions"], m["weights"])
                         if ws and max(ws, key=lambda x: x[1])[0] in HAIR_END_JOINTS], dtype=float)

    def ends(spec, side):
        if spec == ("hair_root", "hair_end"):
            root = np.array([hair[:, 0].mean(), hair[:, 1].max(), hair[:, 2].mean()])
            if side == "base":
                return root, np.array([root[0], hair[:, 1].min(), root[2]])
            root = root + look_joint(9) - base_joint(9)
            return root, np.array([root[0], kit_hair[:, 1].min(), kit_hair[:, 2].mean()])
        j0, j1 = spec
        a = base_joint(j0) if side == "base" else look_joint(j0)
        if isinstance(j1, int):
            b = base_joint(j1) if side == "base" else look_joint(j1)
        elif j1 == "top":
            b = np.array([a[0], tops[side], a[2]])
        elif j1 == "floor":
            b = np.array([a[0], floors[side], a[2]])
        else:
            b = a + np.array(_DIRS[j1]) * 100.0
        return a, b
    for index, v in enumerate(base["vertices"]):
        j = v["joint"]
        if joints is not None and j not in joints:
            continue
        allowed = regions.get(j)
        if not allowed:
            continue
        sel = [t for t in tris if t[5] in allowed]
        if not sel:
            continue
        A = np.array([t[2] for t in sel])
        B = np.array([t[3] for t in sel])
        C = np.array([t[4] for t in sel])
        p = np.array(v["model"], dtype=float)
        spec = axes.get(j)
        o = r = None
        if spec is None or j in RIGID.get(role, ()):
            # Rigid: the vertex rides its joint to the look's skeleton.
            p = p + look_joint(j) - base_joint(j)
            if spec is not None:
                a, b = ends(spec, "look")
                d = (b - a) / max(np.linalg.norm(b - a), 1e-9)
                o = a + d * ((p - a) @ d)
                r = p - o
        else:
            a0, b0 = ends(spec, "base")
            a1, b1 = ends(spec, "look")
            d0, d1 = b0 - a0, b1 - a1
            l0, l1 = np.linalg.norm(d0), np.linalg.norm(d1)
            d0, d1 = d0 / l0, d1 / l1
            t = ((p - a0) @ d0) / l0
            off = p - (a0 + d0 * (t * l0))
            o = a1 + d1 * (t * l1)
            r = _rotation(d0, d1) @ off
            p = o + r
        if j in AXIS_ONLY.get(role, ()):
            ops["vertices"].append({"joint": j, "address": v["address"],
                                    "offset": [round(float(c), 3) for c in (p - np.array(v["model"]))]})
            continue
        anchor = None
        if j in SLICE.get(role, ()) and o is not None and np.linalg.norm(r[[0, 2]]) > 1e-6:
            anchor = _slice_anchor(p, o, r, sel, A, B, C)
        if anchor is None and o is not None and np.linalg.norm(r) > 1e-6:
            rd = r / np.linalg.norm(r)
            t, u, w_ = _ray_hits(o, rd, A, B, C)
            if j in OUTERMOST.get(role, ()):
                finite = np.where(np.isfinite(t), t, -np.inf)
                k = int(np.argmax(finite))
            else:
                k = int(np.argmin(t))
            if np.isfinite(t[k]):
                anchor = (sel[k][0], sel[k][1], [1.0 - u[k] - w_[k], u[k], w_[k]])
        if anchor is None:
            pts, bary, d2 = closest_on_triangles(p, A, B, C)
            k = int(np.argmin(d2))
            anchor = (sel[k][0], sel[k][1], list(bary[k]))
        if j in RIGID.get(role, ()):
            # Rigid parts keep their own shape: record the moved point.
            ops["vertices"].append({"joint": j, "address": v["address"],
                                    "offset": [round(float(c), 3) for c in (p - np.array(v["model"]))]})
            continue
        ops["vertices"].append({"joint": j, "address": v["address"],
                                "anchor": {"mesh": anchor[0], "triangle": anchor[1],
                                           "bary": [round(float(x), 5) for x in anchor[2]],
                                           "offset": 0.0}})
    return ops



def snap_to_kit(scene: dict, fit: dict, role: str, spec: dict, base: dict | None = None) -> int:
    """Ops "snap": the selected vertices pushed onto the fitted kit's
    surface along rays from an inner centre (a point, or the axis line
    x = cx, z = cz at the vertex's own height), taking the outermost hit of
    the chosen kit meshes and regions (owner 2026-10-13: the chest and face
    must follow the kit wire). Mirror pairs move together; a vertex on the
    centre plane stays on it. Selection: joints, and a model-space box, and
    base materials (before edits) or material colours (after)."""
    tris = kit_triangles(fit, role)
    meshes = spec.get("kit", {}).get("meshes")
    regions = spec.get("kit", {}).get("regions")
    sel = [t for t in tris if (meshes is None or t[0] in meshes) and (regions is None or t[5] in regions)]
    A = np.array([t[2] for t in sel]); B = np.array([t[3] for t in sel]); C = np.array([t[4] for t in sel])
    verts = scene["vertices"]
    box = spec.get("box")
    want_mats = set(spec.get("materials", ()))
    want_prims = set(spec.get("prims", ()))
    used = {}
    for t in scene["triangles"]:
        for i in t["v"]:
            used.setdefault(i, set()).add(t["material"])
    base_mats = {}
    if base is not None and (want_mats or spec.get("exclude_materials")):
        for t in base["triangles"]:
            for i in t["v"]:
                base_mats.setdefault(i, set()).add(t["material"])
    groups = {}
    for i, v in enumerate(verts):
        if v["joint"] not in spec["joints"]:
            continue
        q = np.array(v["model"], dtype=float)
        if box is not None and not (np.all(q >= np.array(box[0])) and np.all(q <= np.array(box[1]))):
            continue
        if abs(q[0]) < spec.get("min_abs_x", -1.0):
            continue
        if want_mats and not (base_mats.get(i, set()) & want_mats):
            continue
        if spec.get("exclude_materials") and base_mats.get(i, set()) & set(spec["exclude_materials"]):
            continue
        if want_prims and not ({scene["materials"][m]["prim"] for m in used.get(i, ())} & want_prims):
            continue
        groups.setdefault(tuple(np.round(q, 2)), []).append(i)
    # Every vertex of the same joint at a selected position moves with it
    # (the face's corners are the bangs' and the skin's too).
    by_pos = {}
    for i, v in enumerate(verts):
        by_pos.setdefault((v["joint"], tuple(np.round(np.array(v["model"], dtype=float), 2))), []).append(i)
    for key in list(groups):
        js = {verts[i]["joint"] for i in groups[key]}
        groups[key] = sorted({i for j in js for i in by_pos.get((j, key), [])} | set(groups[key]))
    moved = 0
    done = set()
    inv = {}
    for key, idx in groups.items():
        if key in done:
            continue
        q = np.array(key, dtype=float)
        mirror = (-key[0], key[1], key[2])
        centre = np.array(spec.get("centre", (0.0, q[1], 0.0)), dtype=float)
        if spec.get("axis"):
            centre = np.array([spec["axis"][0], q[1], spec["axis"][1]], dtype=float)
        if spec.get("centre_joint") is not None:
            centre = np.array(scene["joints"][spec["centre_joint"]]["world"], dtype=float)[:3, 3]
        if spec.get("bone"):
            # A limb: rays out from the nearest point of its bone segment.
            a = np.array(scene["joints"][spec["bone"][0]]["world"], dtype=float)[:3, 3]
            b = np.array(scene["joints"][spec["bone"][1]]["world"], dtype=float)[:3, 3]
            ab = b - a
            t_ = float(np.clip((q - a) @ ab / max(ab @ ab, 1e-9), 0.0, 1.0))
            centre = a + ab * t_
        side = np.sign(q[0]) if abs(q[0]) > 0.5 else 0.0
        if side < 0 and mirror in groups:
            continue          # its +x partner carries it
        d = q - centre
        if np.linalg.norm(d) < 1e-6:
            continue
        d = d / np.linalg.norm(d)
        t, u, w = _ray_hits(centre, d, A, B, C)
        finite = np.where(np.isfinite(t), t, -np.inf)
        k = int(np.argmax(finite))
        if not np.isfinite(t[k]):
            continue
        dist = float(t[k]) + float(spec.get("offset", 0.0))
        if abs(dist - np.linalg.norm(q - centre)) > spec.get("max", 30.0):
            continue
        new = centre + d * dist
        if spec.get("scale"):
            # Bigger than the kit's, about a point (the face: owner 2026-10-13).
            sc = np.array(spec.get("scale_centre", centre), dtype=float)
            new = sc + (new - sc) * float(spec["scale"])
        if side == 0.0:
            new[0] = 0.0
        targets = [(idx, new)]
        if side > 0 and mirror in groups:
            targets.append((groups[mirror], new * np.array([-1.0, 1.0, 1.0])))
            done.add(mirror)
        for ii, pos in targets:
            for i in ii:
                v = verts[i]
                j = v["joint"]
                if j not in inv:
                    inv[j] = _inverse_affine(scene["joints"][j]["world"])
                v["model"] = [round(float(c), 4) for c in pos]
                local = inv[j] @ np.append(pos, 1.0)
                v["local"] = [int(round(float(c))) for c in local[:3]]
                moved += 1
        done.add(key)
    return moved

def _kit_point(fit_index: dict, anchor: dict):
    mesh = fit_index[anchor["mesh"]]
    a, b, c = mesh["triangles"][anchor["triangle"]]
    pa, pb, pc = (np.array(mesh["positions"][i], dtype=float) for i in (a, b, c))
    u, v, w = anchor["bary"]
    p = pa * u + pb * v + pc * w
    if anchor.get("offset"):
        n = np.cross(pb - pa, pc - pa)
        nn = np.linalg.norm(n)
        if nn > 1e-9:
            p = p + n / nn * anchor["offset"]
    return p


def _inverse_affine(m):
    m = np.array(m, dtype=float)
    out = np.eye(4)
    r = np.linalg.inv(m[:3, :3])
    out[:3, :3] = r
    out[:3, 3] = -r @ m[:3, 3]
    return out


def apply(base: dict, fit: dict, ops: dict) -> dict:
    out = copy.deepcopy(base)
    fit_index = {m["name"]: m for m in fit["meshes"]}
    by_key = {(v["joint"], v["address"]): i for i, v in enumerate(out["vertices"])}
    # The joints at the kit's proportions (kit_fit.py `skeleton`).
    if fit.get("skeleton"):
        for j, pos in zip(out["joints"], fit["skeleton"]):
            for r in range(3):
                j["world"][r][3] = pos[r]
    inv = [_inverse_affine(j["world"]) for j in out["joints"]]
    moved = 0
    for op in ops["vertices"]:
        i = by_key.get((op["joint"], op["address"]))
        if i is None:
            continue
        v = out["vertices"][i]
        if "anchor" in op:
            p = _kit_point(fit_index, op["anchor"])
        else:
            p = np.array(v["model"], dtype=float) + np.array(op.get("offset", (0, 0, 0)), dtype=float)
        v["model"] = [round(float(c), 4) for c in p]
        local = inv[v["joint"]] @ np.append(p, 1.0)
        v["local"] = [int(round(float(c))) for c in local[:3]]
        moved += 1
    # Normals of lit vertices from the moved faces (area-weighted), in each
    # vertex's joint frame, as the Vtx's signed bytes.
    acc = np.zeros((len(out["vertices"]), 3))
    pos = np.array([v["model"] for v in out["vertices"]], dtype=float)
    for t in out["triangles"]:
        a, b, c = t["v"]
        n = np.cross(pos[b] - pos[a], pos[c] - pos[a])
        for k in (a, b, c):
            acc[k] += n
    for i, v in enumerate(out["vertices"]):
        n = acc[i]
        nn = np.linalg.norm(n)
        if nn < 1e-9:
            continue
        local = np.array(out["joints"][v["joint"]]["world"], dtype=float)[:3, :3].T @ (n / nn)
        local = local / max(np.linalg.norm(local), 1e-9)
        v["normal"] = [int(round(float(c) * 127)) for c in local]
    out["restyled"] = {"moved": moved}
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("shrinkwrap")
    s.add_argument("--base", type=Path, required=True)
    s.add_argument("--fit", type=Path, required=True)
    s.add_argument("--role", required=True)
    s.add_argument("--ops", type=Path, required=True)
    a = sub.add_parser("apply")
    a.add_argument("--base", type=Path, required=True)
    a.add_argument("--fit", type=Path, required=True)
    a.add_argument("--ops", type=Path, required=True)
    a.add_argument("--out", type=Path, required=True, help="directory for the restyled scene.json")
    c = sub.add_parser("compose")
    c.add_argument("--base", type=Path, required=True)
    c.add_argument("--fit", type=Path, required=True)
    c.add_argument("--ops", type=Path, required=True, help="the committed look ops (r_<detail>.json)")
    c.add_argument("--anchors", type=Path, required=True, help="the anchor ops (shrinkwrap output, with hand edits)")
    c.add_argument("--role", required=True)
    c.add_argument("--kit-dir", type=Path)
    c.add_argument("--out", type=Path, required=True)
    c.add_argument("--migrate-edits", action="store_true",
                   help="record each edit's deleted triangles as vertex sets in its file")
    args = ap.parse_args()
    if args.cmd == "compose":
        base = json.loads(args.base.read_text(encoding="utf-8"))
        fit = json.loads(args.fit.read_text(encoding="utf-8"))
        ops = json.loads(args.ops.read_text(encoding="utf-8"))
        ops["vertices"] = json.loads(args.anchors.read_text(encoding="utf-8"))["vertices"]
        ops["_migrate_edits"] = args.migrate_edits
        ops["_base_dir"] = str(args.base.parent)
        args.out.mkdir(parents=True, exist_ok=True)
        scene = compose(base, fit, ops, args.role, args.kit_dir, args.out)
        for png in args.base.parent.glob("tex_*.png"):
            target = args.out / png.name
            if not target.exists():
                target.write_bytes(png.read_bytes())
        (args.out / "scene.json").write_text(json.dumps(scene), encoding="utf-8")
        print(f"compose: {len(scene['vertices'])} vertices, {len(scene['triangles'])} triangles, "
              f"{len(scene['materials'])} materials -> {args.out}")
        return 0
    base = json.loads(args.base.read_text(encoding="utf-8"))
    fit = json.loads(args.fit.read_text(encoding="utf-8"))
    if args.cmd == "shrinkwrap":
        ops = shrinkwrap(base, fit, args.role)
        args.ops.parent.mkdir(parents=True, exist_ok=True)
        args.ops.write_text(json.dumps(ops, indent=0), encoding="utf-8")
        print(f"shrinkwrap: {len(ops['vertices'])} anchors -> {args.ops}")
        return 0
    ops = json.loads(args.ops.read_text(encoding="utf-8"))
    out = apply(base, fit, ops)
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "scene.json").write_text(json.dumps(out, indent=0), encoding="utf-8")
    # The base's textures travel with the scene (same names).
    for png in args.base.parent.glob("tex_*.png"):
        target = args.out / png.name
        if not target.exists():
            target.write_bytes(png.read_bytes())
    print(f"apply: {out['restyled']['moved']} vertices moved -> {args.out}")
    return 0


def compose(base: dict, fit: dict, ops: dict, role: str, kit_dir: Path | None, out: Path) -> dict:
    """The full Track R scene: the restyled base (`apply`), the parts the ops
    add (look_k ring parts: the long hair), the recolour table and the
    textures re-baked from the kit albedo through the base's own UVs."""
    import look_k as K
    import bake as B
    scene = apply(base, fit, ops)
    verts = scene["vertices"]

    def put(i, model):
        v = verts[i]
        v["model"] = [round(float(c), 4) for c in model]
        local = _inverse_affine(scene["joints"][v["joint"]]["world"]) @ np.append(np.asarray(model, dtype=float), 1.0)
        v["local"] = [int(round(float(c))) for c in local[:3]]
    # Rigid parts (ops "rigid"): a joint's vertices keep the base's shape,
    # scaled uniformly about the joint and carried to the look's joint
    # (owner 2026-10-13: the face a little bigger than the kit's, so the
    # head is Peach's, smaller, not the kit's).
    for rg in ops.get("rigid", ()):
        s = float(rg.get("scale", 1.0))
        move = np.array(rg.get("move", (0, 0, 0)), dtype=float)
        for jj in rg["joints"]:
            bj = np.array(base["joints"][jj]["world"], dtype=float)[:3, 3]
            lj = np.array(scene["joints"][jj]["world"], dtype=float)[:3, 3]
            for i, v in enumerate(verts):
                if v["joint"] == jj:
                    put(i, lj + (np.array(base["vertices"][i]["model"], dtype=float) - bj) * s + move)
    # Where each vertex sits before the hand placements: the texture
    # keeper below measures the placements' moves from here.
    before_place = [list(v["model"]) for v in verts]
    # Mirror symmetry (ops "symmetrize": base joints): a vertex and its
    # mirror image in the base (the same joint, or the mirror joint) are set
    # to their mean, mirrored; a vertex on the base's centre plane stays on
    # it. The fitted kit is not symmetric (the slit, the sash); the look is.
    if ops.get("symmetrize"):
        mirror_joint = {int(a): int(b) for a, b in ops.get("mirror_joints", {}).items()}
        mirror_joint.update({b: a for a, b in list(mirror_joint.items())})
        keyed = {}
        for i, v in enumerate(base["vertices"]):
            q = v["model"]
            keyed.setdefault((v["joint"], round(q[0]), round(q[1]), round(q[2])), []).append(i)
        flip = np.array((-1.0, 1.0, 1.0))
        wanted = set(ops["symmetrize"])
        done = set()
        for i, v in enumerate(base["vertices"]):
            if v["joint"] not in wanted or i in done:
                continue
            q = v["model"]
            if abs(q[0]) < 0.5:
                p = np.array(verts[i]["model"], dtype=float)
                p[0] = 0.0
                put(i, p)
                continue
            mj = mirror_joint.get(v["joint"], v["joint"])
            partners = keyed.get((mj, round(-q[0]), round(q[1]), round(q[2])))
            if not partners:
                continue
            k = partners[0]
            a = np.array(verts[i]["model"], dtype=float)
            b = np.array(verts[k]["model"], dtype=float)
            mean = (a + b * flip) / 2
            for ii in keyed[(v["joint"], round(q[0]), round(q[1]), round(q[2]))]:
                put(ii, mean)
                done.add(ii)
            for kk in partners:
                put(kk, mean * flip)
                done.add(kk)
    # Placed vertices (ops "place"): every vertex at a base position (of the
    # listed joints) set to an authored look position, and its mirror image
    # likewise unless "mirror" is false (the hair, the waist: Peach's
    # topology, the baseline's shape, placed by eye against the overlay).
    for pl in ops.get("place", ()):
        joints_ = pl.get("joints")
        at = np.array(pl["at"], dtype=float)
        to = np.array(pl["to"], dtype=float)
        pairs = [(at, to)]
        if pl.get("mirror", True) and abs(at[0]) > 0.5:
            pairs.append((at * (-1, 1, 1), to * (-1, 1, 1)))
        hit = 0
        for a, t in pairs:
            for i, v in enumerate(base["vertices"]):
                if joints_ is not None and v["joint"] not in joints_:
                    continue
                if np.abs(np.array(v["model"], dtype=float) - a).max() <= 0.6:
                    put(i, t)
                    hit += 1
        if not hit:
            raise SystemExit(f"place: no base vertex at {pl['at']} (joints {joints_})")
    # Squashes (ops "squash"): a joint's vertices beyond a plane pulled
    # toward it along one model axis -- every vertex at a position alike, so
    # no seam opens between materials (owner 2026-10-13: the face flatter).
    for sq in ops.get("squash", ()):
        axis, start, k = int(sq["axis"]), float(sq["from"]), float(sq["factor"])
        for i, v in enumerate(verts):
            if v["joint"] in sq["joints"] and v["model"][axis] > start:
                p = np.array(v["model"], dtype=float)
                p[axis] = start + (p[axis] - start) * k
                put(i, p)
    # Snaps onto the kit (ops "snap", before the edits): see snap_to_kit.
    for sp in ops.get("snap", ()):
        if sp.get("stage", "before_edits") == "before_edits":
            snap_to_kit(scene, fit, role, sp, base)
    # Texture keepers (ops "keep_texture": base materials): a vertex of these
    # materials that a placement moved (a face corner Peach's bangs share)
    # takes the texture coordinate its new place had on the unmoved face,
    # seen from the front -- each of its triangles' affine map, extended and
    # averaged -- so the drawing stays put and the face only grows or shrinks
    # around it (owner 2026-10-13: the brows tilted when the bangs moved).
    kt = ops.get("keep_texture", ())
    if isinstance(kt, dict):
        keep, k_scale, k_centre = set(kt["materials"]), float(kt.get("scale", 1.0)), np.array(kt.get("centre", (0, 0)), dtype=float)
    else:
        keep, k_scale, k_centre = set(kt), 1.0, np.zeros(2)
    if keep:
        tri_of = {}
        for ti, t in enumerate(base["triangles"]):
            if t["material"] in keep:
                for i in t["v"]:
                    tri_of.setdefault(i, []).append(ti)
        for i, tis in tri_of.items():
            new = np.array(verts[i]["model"][:2], dtype=float)
            if k_scale == 1.0 and np.abs(new - np.array(before_place[i][:2])).max() < 0.5:
                continue
            # A drawing scale: the texture shows k_scale times its size about
            # k_centre (the eyes fitted to the kit's narrower face).
            new = k_centre + (new - k_centre) / k_scale
            sts = []
            for ti in tis:
                a, b, c = base["triangles"][ti]["v"]
                P = np.array([before_place[k][:2] for k in (a, b, c)], dtype=float)
                S_ = np.array([base["vertices"][k]["st"] for k in (a, b, c)], dtype=float)
                M2 = np.array([P[1] - P[0], P[2] - P[0]]).T
                if abs(np.linalg.det(M2)) < 1e-6:
                    continue
                u, w = np.linalg.solve(M2, new - P[0])
                sts.append(S_[0] + (S_[1] - S_[0]) * u + (S_[2] - S_[0]) * w)
            if sts:
                st = np.mean(sts, axis=0)
                verts[i]["st"] = [int(round(float(st[0]))), int(round(float(st[1])))]
    # Dropped parts (ops "drop": base materials per joint, or base triangle
    # indices): their triangles go (owner 2026-10-13: the circlet is painted
    # on the face, so Peach's crown is not drawn; the hem's cover, the skirt
    # being two-sided). Every drop names base triangles, all removed at once.
    gone = set()
    late = set()
    for dr in ops.get("drop", ()):
        hit = {ti for ti, t in enumerate(base["triangles"])
               if (t["material"] in dr.get("materials", ()) and t["joint"] in dr.get("joints", ()))
               or ti in dr.get("tris", ())}
        (late if dr.get("late") else gone).update(hit)
    for ti in late:
        # Flagged now, removed after the edits replay.
        scene["triangles"][ti]["dropped"] = True
    scene["triangles"] = [t for ti, t in enumerate(scene["triangles"]) if ti not in gone]
    # Region scales (ops "scale"): the vertices of the listed base materials
    # on a joint scaled about their centroid, per model axis (the face made
    # flatter (z) and a little bigger (x, y): owner 2026-10-13).
    for sc in ops.get("scale", ()):
        mats = set(sc["materials"])
        idx = sorted({i for t in scene["triangles"] if t["material"] in mats and t["joint"] == sc["joint"]
                      for i in t["v"] if scene["vertices"][i]["joint"] == sc["joint"]})
        if not idx:
            continue
        pts = np.array([scene["vertices"][i]["model"] for i in idx], dtype=float)
        centre = pts.mean(0) + np.array(sc.get("centre_offset", (0, 0, 0)), dtype=float)
        k = np.array(sc["factors"], dtype=float)
        inv_j = _inverse_affine(scene["joints"][sc["joint"]]["world"])
        for i, q in zip(idx, pts):
            new = centre + (q - centre) * k + np.array(sc.get("move", (0, 0, 0)), dtype=float)
            v = scene["vertices"][i]
            v["model"] = [round(float(c), 4) for c in new]
            local = inv_j @ np.append(new, 1.0)
            v["local"] = [int(round(float(c))) for c in local[:3]]
    # Welds (ops "weld"): the top ring of one joint's part snapped onto the
    # bottom ring of another's, vertex to nearest vertex, so the two read as
    # one garment (owner 2026-10-13: "torso and dress should be one mesh").
    for wd in ops.get("weld", ()):
        def ring(joint, end, frac):
            idx = [i for i, v in enumerate(scene["vertices"]) if v["joint"] == joint]
            ys = np.array([scene["vertices"][i]["model"][1] for i in idx])
            lo, hi = ys.min(), ys.max()
            cut = hi - (hi - lo) * frac if end == "top" else lo + (hi - lo) * frac
            return [i for i, y in zip(idx, ys) if (y >= cut if end == "top" else y <= cut)]
        src = ring(wd["joint"], wd.get("end", "top"), wd.get("fraction", 0.15))
        dst = ring(wd["onto"], wd.get("onto_end", "bottom"), wd.get("onto_fraction", 0.15))
        if not src or not dst:
            continue
        D = np.array([scene["vertices"][i]["model"] for i in dst], dtype=float)
        inv_j = _inverse_affine(scene["joints"][wd["joint"]]["world"])
        for i in src:
            q = np.array(scene["vertices"][i]["model"], dtype=float)
            dist = np.linalg.norm((D - q)[:, [0, 2]], axis=1)
            target = D[int(np.argmin(dist))]
            v = scene["vertices"][i]
            v["model"] = [round(float(c), 4) for c in target]
            local = inv_j @ np.append(target, 1.0)
            v["local"] = [int(round(float(c))) for c in local[:3]]
    # Recolour: material index -> RGBA hex, or {joint: hex} for a material
    # several joints share (a split MObj in the writer).
    recolor = {int(k): v for k, v in ops.get("recolor", {}).items()}
    new_mats = []
    remap = {}
    for t in scene["triangles"]:
        m = t["material"]
        rule = recolor.get(m)
        if isinstance(rule, dict):
            colour = rule.get(str(t["joint"]), rule.get("*"))
            key = (m, colour)
        else:
            colour = rule
            key = (m, colour)
        if key not in remap:
            mat = dict(scene["materials"][m])
            if colour:
                mat["prim"] = colour
                if mat.get("texture") and str(m) not in ops.get("bake", {}):
                    mat["texture"] = None
            remap[key] = len(new_mats)
            new_mats.append(mat)
        t["material"] = remap[key]
    scene["materials"] = new_mats
    # Two-sided base materials (ops "two_sided"): the skirt, whose lining
    # shows through the slit.
    two = {int(m) for m in ops.get("two_sided", ())}
    for (m, colour), mi in remap.items():
        if m in two:
            scene["materials"][mi]["two_sided"] = True
    # Re-bake textures named in ops["bake"] (by base material index) from the
    # kit albedo through the restyled triangles' own UVs.
    if kit_dir is not None and ops.get("bake"):
        albedo = B.KitAlbedo(fit, kit_dir, role)
        for (m, colour), mi in remap.items():
            spec = ops["bake"].get(str(m))
            mat = scene["materials"][mi]
            if spec is None or not mat.get("texture"):
                continue
            tris = [t for t in scene["triangles"] if t["material"] == mi]
            w, h = mat["texture"]["width"], mat["texture"]["height"]
            tri_uv, tri_xyz = [], []
            for t in tris:
                uvs = []
                for v in t["v"]:
                    s_, t_ = scene["vertices"][v]["st"]
                    uvs.append((s_ / 32.0 / w, 1.0 - t_ / 32.0 / h))
                tri_uv.append(uvs)
                tri_xyz.append([scene["vertices"][v]["model"] for v in t["v"]])
            img, _cov = B.bake_texture(albedo, w, h, tri_uv, tri_xyz, spec.get("regions"),
                                       supersample=spec.get("supersample", 3), smooth=spec.get("smooth", 0))
            ci, palette = B.quantize_ci(img, spec.get("colors", 16))
            name = f"r_{m}_{w}x{h}.png"
            from PIL import Image
            pil = Image.fromarray(ci, "P")
            pil.putpalette([c for rgba in palette for c in rgba[:3]])
            pil.convert("RGBA").save(out / name)
            mat["texture"] = dict(mat["texture"], file=name, palette=[list(c) for c in palette],
                                  texels=ci.tolist())
            mat["prim"] = "ffffffff"
    # Authored textures (ops "textures": name -> look_k drawing), written
    # beside the scene; "replace" names one as "@name".
    drawn = {}
    for name, spec in ops.get("textures", {}).items():
        drawn[name] = K._drawn_texture(spec, f"t_{name}", out)
    # Texture coordinates rescaled (ops "st_scale"): the skirt's run from hip
    # to hem fitted to its texture, so a hem band sits at the hem.
    for sc in ops.get("st_scale", ()):
        mats_ = set(sc["materials"])
        idx = {i for ti, t in enumerate(base["triangles"]) if t["material"] in mats_ for i in t["v"]
               if base["vertices"][i]["joint"] in sc["joints"]}
        for i in idx:
            s0, t0 = scene["vertices"][i]["st"]
            scene["vertices"][i]["st"] = [int(round(s0 * sc.get("s", 1.0))), int(round(t0 * sc.get("t", 1.0)))]
    # Palette rules (ops "palette": base material -> rules): each palette
    # entry takes the first rule whose hue range (degrees; saturation and
    # value floors optional) holds it: a new hue, saturation and value as
    # multipliers, or a fixed colour. Parameters only; the texels stay.
    import colorsys
    for (m, colour), mi in remap.items():
        rules = ops.get("palette", {}).get(str(m))
        mat = scene["materials"][mi]
        if not rules or not mat.get("texture"):
            continue
        tex = mat["texture"]
        if "texels" not in tex:
            # The base's decoded image: its colours (at most a CI4 palette's
            # sixteen) become the palette, its pixels the texels.
            from PIL import Image
            img = Image.open(Path(ops["_base_dir"]) / tex["file"]).convert("RGBA")
            colours, texels = [], []
            for y in range(img.height):
                row = []
                for x in range(img.width):
                    c = list(img.getpixel((x, y)))
                    if c not in colours:
                        colours.append(c)
                    row.append(colours.index(c))
                texels.append(row)
            if len(colours) > 16:
                raise SystemExit(f"palette: material {m} has {len(colours)} colours")
            tex = dict(tex, palette=colours, texels=texels)
        pal = []
        for rgba in tex["palette"]:
            r, g, b = (c / 255.0 for c in rgba[:3])
            h, s_, v = colorsys.rgb_to_hsv(r, g, b)
            hue = h * 360.0
            for rule in rules:
                lo, hi = rule.get("hue", (0, 360))
                inside = (lo <= hue <= hi) if lo <= hi else (hue >= lo or hue <= hi)
                if not inside or s_ < rule.get("min_sat", 0.0) or v < rule.get("min_val", 0.0) \
                        or s_ > rule.get("max_sat", 1.0):
                    continue
                if "color" in rule:
                    c = rule["color"]
                    r, g, b = (int(c[i:i + 2], 16) / 255.0 for i in (0, 2, 4))
                else:
                    h = (rule["to_hue"] / 360.0) if "to_hue" in rule else h
                    s_ = min(1.0, s_ * rule.get("sat", 1.0))
                    v = min(1.0, v * rule.get("val", 1.0))
                    r, g, b = colorsys.hsv_to_rgb(h, s_, v)
                break
            pal.append([int(round(r * 255)), int(round(g * 255)), int(round(b * 255)), rgba[3]])
        name = f"p_{m}_{tex['width']}x{tex['height']}.png"
        from PIL import Image
        rgba_img = [tuple(pal[i]) for row in tex["texels"] for i in row]
        img = Image.new("RGBA", (tex["width"], tex["height"]))
        img.putdata(rgba_img)
        img.save(out / name)
        mat["texture"] = dict(tex, file=name, palette=pal)
    # Composed textures (face_frames.py) or authored ones replace a base
    # material's image.
    for (m, colour), mi in remap.items():
        src = ops.get("replace", {}).get(str(m))
        mat = scene["materials"][mi]
        if src is None or not mat.get("texture"):
            continue
        if src.startswith("@"):
            tex = drawn[src[1:]]
            mat["texture"] = dict(mat["texture"], file=tex["file"], palette=tex["palette"], texels=tex["texels"])
            mat["prim"] = "ffffffff"
            continue
        src = Path(src)
        target = out / src.name
        target.write_bytes(src.read_bytes())
        mat["texture"] = dict(mat["texture"], file=src.name)
        mat["prim"] = "ffffffff"
    # Materials the edits name (ops "edit_materials": name -> {"texture":
    # a look_k drawing}), added before the edits replay so a captured edit
    # finds them by texture.
    for name, spec in ops.get("edit_materials", {}).items():
        tex = K._drawn_texture(spec["texture"], f"t_{name}", out)
        tex["alpha"] = any(0 in row for row in tex["texels"])
        scene["materials"].append({"name": name, "combine": [0, 0], "lit": True, "prim": "ffffffff",
                                   "texture": tex, "two_sided": bool(spec.get("two_sided"))})
    # Edits captured in Blender (capture_ops.py), replayed in order, each
    # against the scene the ones before it made -- before the added parts,
    # so an edit's indices never depend on how many triangles a part has.
    edits = ops.get("edits") or []
    if isinstance(edits, str):
        edits = [edits]
    for e in edits:
        data = json.loads(Path(e).read_text(encoding="utf-8"))
        replay_edits(scene, data)
        if "_migrated" in data and ops.get("_migrate_edits"):
            data["deleted_tri_verts"] = data.pop("_migrated")
            Path(e).write_text(json.dumps(data, indent=1) + "\n", encoding="utf-8")
    scene["triangles"] = [t for t in scene["triangles"] if not t.pop("dropped", False)]
    for sp in ops.get("snap", ()):
        if sp.get("stage") == "after_edits":
            snap_to_kit(scene, fit, role, sp)
    # Added parts (look_k ring syntax), appended as new vertices/triangles.
    if ops.get("add_parts"):
        kops = {"detail": scene.get("detail", "high"), "materials": ops.get("add_materials", {}),
                "parts": ops["add_parts"]}
        b = K.Builder(base, fit, kops, role)
        for part in kops["parts"]:
            b.part(part)
        # Added parts bake their textured materials from the kit albedo.
        baking = kit_dir is not None and any(m.get("texture") for m in kops["materials"].values())
        extra = b.finish(B.KitAlbedo(fit, kit_dir, role) if baking else None, out)
        offset = len(scene["vertices"])
        moff = len(scene["materials"])
        for v in extra["vertices"]:
            v["address"] = 0x100000 + v["address"]   # new Vtx: the writer allocates
            scene["vertices"].append(v)
        for m in extra["materials"]:
            scene["materials"].append(m)
        for t in extra["triangles"]:
            scene["triangles"].append({"joint": t["joint"], "v": [i + offset for i in t["v"]],
                                       "material": t["material"] + moff})
            if t.get("billboard"):
                scene["triangles"][-1]["billboard"] = True
    recompute_normals(scene)
    return scene


def replay_edits(scene: dict, edits: dict) -> None:
    verts = scene["vertices"]
    inv = [_inverse_affine(j["world"]) for j in scene["joints"]]

    def place(v, model):
        v["model"] = [round(float(c), 4) for c in model]
        local = inv[v["joint"]] @ np.append(np.array(model, dtype=float), 1.0)
        v["local"] = [int(round(float(c))) for c in local[:3]]
    def material(desc):
        # capture_ops.py names a material by its prim colour and texture
        # (older captures: by index); reuse a match, else add an untextured one.
        if isinstance(desc, int):
            return desc
        for mi, m in enumerate(scene["materials"]):
            if m["prim"] == desc["prim"] and (m.get("texture") or {}).get("file") == desc.get("texture"):
                return mi
        if desc.get("texture"):
            raise SystemExit(f"edits: no material with texture {desc['texture']}")
        scene["materials"].append({"combine": scene["materials"][0].get("combine"), "lit": True,
                                   "prim": desc["prim"], "texture": None})
        return len(scene["materials"]) - 1
    for k, d in edits.get("moved", {}).items():
        v = verts[int(k)]
        place(v, [v["model"][i] + d[i] for i in range(3)])
    # Recolours by vertex set when the capture has them: list positions
    # differ from the composed scene's while dropped triangles are still in.
    if "retint_verts" in edits:
        by_set = {frozenset(t["v"]): t for t in scene["triangles"]}
        for r in edits["retint_verts"]:
            t = by_set.get(frozenset(r["v"]))
            if t is None:
                raise SystemExit("edits: a recoloured triangle was not found")
            t["material"] = material(r["material"])
    else:
        for k, desc in edits.get("retint", {}).items():
            scene["triangles"][int(k)]["material"] = material(desc)
    base_n = len(verts)
    for av in edits.get("added_vertices", ()):
        v = {"joint": av["joint"], "address": 0x200000 + len(verts) * 16, "local": [0, 0, 0],
             "model": [0, 0, 0], "uv": list(av.get("uv", (0.0, 0.0))), "st": (0, 0), "flag": 0,
             "rgba": [0, 0, 0, 255], "normal": [0, 0, 0]}
        verts.append(v)
        place(v, av["model"])
    # Deleted triangles by their vertex sets (stable whatever came before);
    # older captures name list positions, recorded here once as vertex sets
    # (look_r.py compose --migrate-edits).
    if "deleted_tri_verts" in edits:
        want = {frozenset(v) for v in edits["deleted_tri_verts"]}
        gone = {i for i, t in enumerate(scene["triangles"]) if frozenset(t["v"]) in want}
        if len(gone) != len(want):
            raise SystemExit(f"edits: {len(want) - len(gone)} deleted triangle(s) not found")
    else:
        gone = set(edits.get("deleted_tris", ()))
        edits["_migrated"] = [sorted(scene["triangles"][i]["v"]) for i in sorted(gone)]
    tris = [t for i, t in enumerate(scene["triangles"]) if i not in gone]
    for at in edits.get("added_tris", ()):
        idx = [r[1] if r[0] == "s" else base_n + r[1] for r in at["v"]]
        tris.append({"joint": at["joint"], "v": idx, "material": material(at["material"])})
    scene["triangles"] = tris


def recompute_normals(scene: dict) -> None:
    """Each vertex's normal from its faces (area-weighted), in its joint's
    frame, as the Vtx's signed bytes."""
    verts = scene["vertices"]
    acc = np.zeros((len(verts), 3))
    pos = np.array([v["model"] for v in verts], dtype=float)
    for t in scene["triangles"]:
        a, b, c = t["v"]
        n = np.cross(pos[b] - pos[a], pos[c] - pos[a])
        for k in (a, b, c):
            acc[k] += n
    for i, v in enumerate(verts):
        n = acc[i]
        if np.linalg.norm(n) < 1e-9:
            continue
        local = np.array(scene["joints"][v["joint"]]["world"], dtype=float)[:3, :3].T @ (n / np.linalg.norm(n))
        local = local / max(np.linalg.norm(local), 1e-9)
        v["normal"] = [int(round(float(c) * 127)) for c in local]


if __name__ == "__main__":
    sys.exit(main())
