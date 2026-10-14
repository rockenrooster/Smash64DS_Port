#!/usr/bin/env python3
"""Fit a 3DS kit model onto a base content's skeleton (P6 A3, "Fit").

The kit's joints map onto the base's by name and role (ROLE_MAPS); the kit
model is scaled uniformly to the base's height and turned into the base's
rest pose: each mapped kit bone is re-aimed along its base bone and every
bone keeps the kit's own length (owner, 2026-10-13: no stretching). The
two-influence skin carries the mesh. Unmapped joints (swing joints, skirt
joints, ThrowN) ride their parent rigidly. `skeleton` gives each base joint's
place in the kit's proportions: the JointTree the look is built on.

`fit(kit, base_scene, role_map)` returns the kit's joints' new model-space
frames and the posed position of every pool vertex, in the base's model space
(Y-up, SSB64 units). Nothing here is written to a tracked file: callers keep
the fitted mesh under builds/ (plan D10).
"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import dae_reader as D  # noqa: E402
import numpy as np  # noqa: E402

# kit joint (sid) -> base JointTree row. Rows from the base's rest pose
# (model_scene.py joints): Peach 2 torso (pivot at the waist), 3/4/5/6 left
# shoulder, upper arm, forearm, hand, 7 neck, 8 head, 9 crown, 10-14 right arm
# and held-item point, 16/17/19 left thigh, shin, foot, 21/22/24 right, 25/26
# the dress. Roy 2 torso, 3-7 left arm (7 the held point), 8/9 neck and head,
# 10-14 right arm and sword, 15-18 cape, 19/20 shoulder pads, 22/23/25 left
# leg, 27/28/30 right leg.
ROLE_MAPS = {
    "peach": {
        "RotN": 1, "HipN": 1, "WaistN": 2, "BustN": 2,
        "LShoulderN": 3, "LShoulderJ": 4, "LArmJ": 5, "LHandN": 6, "LHaveN": 6, "ShieldN": 5,
        "NeckN": 7, "HeadN": 8,
        "RShoulderN": 10, "RShoulderJ": 11, "RArmJ": 12, "RHandN": 13, "RHaveN": 14, "RStickN": 14,
        "LLegJ": 16, "LKneeJ": 17, "LFootJ": 19, "LToeN": 19,
        "RLegJ": 21, "RKneeJ": 22, "RFootJ": 24, "RToeN": 24,
    },
    "roy": {
        "RotN": 1, "HipN": 1, "WaistN": 2, "BustN": 2,
        "LShoulderN": 3, "LShoulderJ": 4, "LArmJ": 5, "LHandN": 6, "LHaveN": 7,
        "NeckN": 8, "HeadN": 9,
        "RShoulderN": 10, "RShoulderJ": 11, "RArmJ": 12, "RHandN": 13, "RHaveN": 14, "SwordN": 14,
        "LLegJ": 22, "LKneeJ": 23, "LFootJ": 25, "LToeN": 25,
        "RLegJ": 27, "RKneeJ": 28, "RFootJ": 30, "RToeN": 30,
    },
}


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def _scale(a, k):
    return (a[0] * k, a[1] * k, a[2] * k)


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _norm(a):
    n = math.sqrt(_dot(a, a))
    return (a[0] / n, a[1] / n, a[2] / n) if n > 1e-9 else (0.0, 0.0, 0.0)


def _rot_between(a, b):
    """3x3 rotation taking unit a to unit b (rows)."""
    a, b = _norm(a), _norm(b)
    v = _cross(a, b)
    c = _dot(a, b)
    if c < -0.999999:
        # 180 degrees about any axis perpendicular to a.
        axis = _norm(_cross(a, (1.0, 0.0, 0.0)) if abs(a[0]) < 0.9 else _cross(a, (0.0, 1.0, 0.0)))
        x, y, z = axis
        return [[2 * x * x - 1, 2 * x * y, 2 * x * z], [2 * x * y, 2 * y * y - 1, 2 * y * z],
                [2 * x * z, 2 * y * z, 2 * z * z - 1]]
    k = 1.0 / (1.0 + c)
    vx, vy, vz = v
    return [[vx * vx * k + c, vx * vy * k - vz, vx * vz * k + vy],
            [vy * vx * k + vz, vy * vy * k + c, vy * vz * k - vx],
            [vz * vx * k - vy, vz * vy * k + vx, vz * vz * k + c]]


def _m3_mul(a, b):
    return [[sum(a[r][k] * b[k][c] for k in range(3)) for c in range(3)] for r in range(3)]


def _m3_vec(a, v):
    return (a[0][0] * v[0] + a[0][1] * v[1] + a[0][2] * v[2],
            a[1][0] * v[0] + a[1][1] * v[1] + a[1][2] * v[2],
            a[2][0] * v[0] + a[2][1] * v[1] + a[2][2] * v[2])


def _m3_t(a):
    return [[a[c][r] for c in range(3)] for r in range(3)]


def _pos(m):
    return (m[3], m[7], m[11])


def _rot3(m):
    return [[m[0], m[1], m[2]], [m[4], m[5], m[6]], [m[8], m[9], m[10]]]


# Base joint -> the kit joint its pivot takes, where that differs from the
# first kit joint mapped onto it (ROLE_MAPS).
SKELETON_FROM = {"peach": {2: "HipN"}}


def fit(kit: D.Model, base: dict, role_map: dict, role_name: str = "") -> dict:
    """The kit fitted onto `base` (a model_scene.py scene dict): scaled
    uniformly to the base's height and turned into the base's rest pose, its
    own bone lengths kept (owner, 2026-10-13: "do not stretch palutena, just
    scale to similar size as peach")."""
    base_pos = [(j["world"][0][3], j["world"][1][3], j["world"][2][3]) for j in base["joints"]]
    names = [j.name for j in kit.joints]
    kit_pos = [_pos(j.world) for j in kit.joints]

    # Owner, 2026-10-13: the same height as the base, top to top -- the top
    # of Peach's crown to the top of Palutena's hair -- floor to floor.
    body = [m for m in (kit.neutral_meshes() or kit.meshes)
            if not m.name.startswith("weapon") and "sword" not in m.name]
    kys = [D.mat_point(m.bind_shape, m.pool.positions[v])[1] for m in body for v in m.used_vertices()]
    bys = [v["model"][1] for v in base["vertices"]]
    s = (max(bys) - min(bys)) / (max(kys) - min(kys))
    children = {i: [c for c, j in enumerate(kit.joints) if j.parent == i] for i in range(len(names))}
    new_pos = [None] * len(names)
    new_rot = [None] * len(names)
    for i, joint in enumerate(kit.joints):
        p = joint.parent
        kit_rot = _rot3(joint.world)
        # Forward kinematics: the kit's own offset from its parent, turned by
        # the parent's re-aim, scaled. Bone lengths stay the kit's.
        if p < 0:
            new_pos[i] = _scale(kit_pos[i], s)
        else:
            off = _sub(kit_pos[i], kit_pos[p])
            prot = _m3_mul(new_rot[p], _m3_t(_rot3(kit.joints[p].world)))
            new_pos[i] = _add(new_pos[p], _scale(_m3_vec(prot, off), s))
        # Re-aim: a mapped joint turns so its bones point where the base's
        # do (the base's rest pose: arms out), toward every mapped child
        # found first down each branch.
        pairs = []
        if names[i] in role_map:
            here = base_pos[role_map[names[i]]]
            stack = list(children[i])
            while stack:
                c = stack.pop(0)
                if names[c] in role_map:
                    kd = _sub(kit_pos[c], kit_pos[i])
                    bd = _sub(base_pos[role_map[names[c]]], here)
                    if math.sqrt(_dot(kd, kd)) > 1e-6 and math.sqrt(_dot(bd, bd)) > 1e-6:
                        pairs.append((kd, bd))
                        continue
                stack.extend(children[c])
        if len(pairs) == 1:
            kd, bd = pairs[0]
            new_rot[i] = _m3_mul(_rot_between(kd, bd), kit_rot)
            continue
        if len(pairs) >= 2:
            # Kabsch: the rotation that best carries the kit's child
            # directions onto the base's (a hip or a bust with three).
            a = np.array([_norm(kd) for kd, _ in pairs])
            b = np.array([_norm(bd) for _, bd in pairs])
            u, _sv, vt = np.linalg.svd(a.T @ b)
            d = np.sign(np.linalg.det(vt.T @ u.T))
            r = vt.T @ np.diag([1.0, 1.0, d]) @ u.T
            new_rot[i] = _m3_mul(r.tolist(), kit_rot)
            continue
        if p < 0:
            new_rot[i] = kit_rot
        else:
            new_rot[i] = _m3_mul(_m3_mul(new_rot[p], _m3_t(_rot3(kit.joints[p].world))), kit_rot)
    # The feet stand where the base's do.
    feet = (new_pos[names.index("LFootJ")][1] + new_pos[names.index("RFootJ")][1]) / 2
    base_feet = (base_pos[role_map["LFootJ"]][1] + base_pos[role_map["RFootJ"]][1]) / 2
    lift = (0.0, base_feet - feet, 0.0)
    new_pos = [_add(q, lift) for q in new_pos]

    def deform(i, x):
        local = _m3_vec(_m3_t(_rot3(kit.joints[i].world)), _sub(x, kit_pos[i]))
        return _add(new_pos[i], _scale(_m3_vec(new_rot[i], local), s))
    meshes = []
    for mesh in kit.meshes:
        used = mesh.used_vertices()
        posed = {}
        for v in used:
            x = D.mat_point(mesh.bind_shape, mesh.pool.positions[v])
            acc = (0.0, 0.0, 0.0)
            total = 0.0
            for joint, weight in mesh.weights[v]:
                acc = _add(acc, _scale(deform(joint, x), weight))
                total += weight
            if total <= 0:
                acc = _scale(x, s)
            elif abs(total - 1.0) > 1e-6:
                acc = _scale(acc, 1.0 / total)
            posed[v] = acc
        meshes.append({"name": mesh.name, "posed": posed})
    body_names = {m.name for m in body}
    low = min(p[1] for m in meshes if m["name"] in body_names for p in m["posed"].values())
    floor = (0.0, min(bys) - low, 0.0)
    for m in meshes:
        m["posed"] = {v: _add(p, floor) for v, p in m["posed"].items()}
    new_pos = [_add(q, floor) for q in new_pos]
    # Each base joint's place in the kit's proportions: its first mapped kit
    # joint, else its base place moved as its nearest placed ancestor.
    first = {}
    for i, n in enumerate(names):
        if n in role_map and role_map[n] not in first:
            first[role_map[n]] = i
    # A base joint whose pivot sits at another kit joint than its first
    # mapped one: Peach's torso turns at her hips, so its geometry starts
    # there (from the waist, the dress top and the bodice never met).
    for b, kit_name in SKELETON_FROM.get(role_name, {}).items():
        if kit_name in names:
            first[b] = names.index(kit_name)
    skeleton = [None] * len(base_pos)
    for bi, j in enumerate(base["joints"]):
        if bi in first:
            skeleton[bi] = new_pos[first[bi]]
            continue
        a = j["parent"]
        while a >= 0 and a not in first and skeleton[a] is None:
            a = base["joints"][a]["parent"]
        moved = _sub(skeleton[a], base_pos[a]) if a >= 0 and skeleton[a] is not None else (0.0, 0.0, 0.0)
        skeleton[bi] = _add(base_pos[bi], moved)
    # "rot": each kit joint's re-aimed rotation, so a caller can pose any
    # point bound to it (look_blend.py: pos + s * rot * kit_rot^T * (x - kit_pos)).
    return {"scale": s, "joints": [{"name": n, "pos": new_pos[i], "stretch": 1.0, "rot": new_rot[i]}
                                   for i, n in enumerate(names)],
            "skeleton": [list(q) for q in skeleton],
            "meshes": meshes}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--kit", type=Path, required=True, help="the kit .dae")
    ap.add_argument("--base", type=Path, required=True, help="the base's scene.json")
    ap.add_argument("--role", required=True, choices=sorted(ROLE_MAPS))
    ap.add_argument("--out", type=Path, required=True, help="fitted mesh JSON (under builds/)")
    ap.add_argument("--all", action="store_true", help="every kit mesh, not only the neutral set")
    args = ap.parse_args()
    kit = D.read(args.kit)
    base = json.loads(args.base.read_text(encoding="utf-8"))
    result = fit(kit, base, ROLE_MAPS[args.role], args.role)
    keep = None if args.all else {m.name for m in kit.neutral_meshes()}
    out = {"kit": args.kit.name, "scale": result["scale"], "joints": result["joints"],
           "skeleton": result["skeleton"], "meshes": []}
    for mesh, fitted in zip(kit.meshes, result["meshes"]):
        if keep is not None and mesh.name not in keep:
            continue
        used = mesh.used_vertices()
        index = {v: k for k, v in enumerate(used)}
        out["meshes"].append({
            "name": mesh.name, "image": mesh.image, "material": mesh.material,
            "positions": [[round(c, 3) for c in fitted["posed"][v]] for v in used],
            "uvs": [list(mesh.pool.uvs[v]) for v in used] if mesh.pool.uvs else [],
            "normals": [list(mesh.pool.normals[v]) for v in used] if mesh.pool.normals else [],
            "triangles": [[index[a], index[b], index[c]] for a, b, c in mesh.triangles],
            "weights": [[[kit.joints[j].name, w] for j, w in mesh.weights[v]] for v in used],
        })
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(out), encoding="utf-8")
    tris = sum(len(m["triangles"]) for m in out["meshes"])
    print(f"{args.kit.name} -> {args.role}: scale {result['scale']:.3f}, {len(out['meshes'])} meshes, "
          f"{tris} triangles -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
