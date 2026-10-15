#!/usr/bin/env python3
"""P6 look from a model built by hand in Blender (Palutena, owner 2026-10-13).

The owner's "3DS to DS" route: Palutena is a 428-triangle model built in
Blender on the owner's baseline (the 3DS kit's low model with its HAL
armature), every texture baked from that baseline. The work copy is dumped
as JSON (kit space, bone weights, per-corner uvs, materials: `--dump`); this
script turns the dump into model_scene.py scenes that model_writer.py writes
into the content's model and main files.

Skeleton. The look keeps the base's JointTree rows and rotations; each row's
place comes from the kit's proportions (kit_fit.fit: uniformly scaled to the
base's height, bones re-aimed into the base's rest pose, the kit's bone
lengths kept), except:

- KEEP_BASE rows keep the base's own translation. The base's animations key
  these rows' translations around a rest of zero (the hips and the torso: 139
  and 12 of Peach's clips) or move them for parts the look does not draw (the
  crown, the held-item point, the dress rows), so only the base's own offsets
  stay right under every clip;
- COINCIDE rows sit on another row, as the base's pairs do (shoulder roots on
  the shoulders, thigh roots on the thighs, ankles on the feet).

Every other row a clip keys plays at the look's own limb lengths through
FTAttributes.translate_scales (vanilla Luigi plays Mario's figatrees that
way): the look's rest translation over the base's, per axis, where the
base's is not zero. The guard scales its rest rows (the ShieldPose file's
copy of the base's tree) by the same table, so every row must come out at
the look's rest; `unscaled` in the report lists any that cannot. The scene
carries the table (`translate_scales`, one Vec3f per fighter joint, model row
r = joint r + 4) for model_writer.py.

Mesh. Each vertex is posed into the base's rest pose by its dominant kit
bone and drawn by one base row (OBJECT_ROWS and the face rules below). A
triangle on a parent and the child drawn right after it keeps both (a seam:
the child draws it from the parent's last load); any other mix moves to the
deepest row's own copies of its corners. Textures are resized (TEXTURE_SCALE,
shared by both details) and turned into CI4 (16 colours; colour 0 clear for
cut-out sprites).

Everything this reads and writes is kit-derived: local, under builds/ (D10).

    look_blend.py --dump builds/p6-art/palutena/3ds2ds/work_dump.json \\
        --kit ".../Palutena/palutena_low_n_c00.dae" \\
        --base builds/p6-art/base/peach/high/scene.json --detail high \\
        --out builds/p6-art/palutena/3ds2ds/scene_high
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import dae_reader as D  # noqa: E402
import kit_fit  # noqa: E402

JOINT_COUNT = 37                # FTPARTS_JOINT_NUM_MAX: translate_scales entries
ROW_JOINT = 4                   # nFTPartsJointCommonStart: model row r is joint r + 4

# Base rows (Peach): 0 root, 1 hips, 2 torso (pivot at the hips), 3/10
# shoulder roots, 4/5/6 left upper arm, forearm, hand, 7 neck, 8 head, 9
# crown, 11/12/13 right arm, 14 right held-item point, 15/20 thigh roots,
# 16/17/18/19 left thigh, shin, ankle, foot, 21-24 right leg, 25/26 the dress,
# 27 the floor point.
KEEP_BASE = {0, 1, 2, 9, 14, 25, 26, 27}
COINCIDE = {3: 4, 10: 11, 15: 16, 20: 21, 18: 19, 23: 24}

BONE_ROWS = {
    "LShoulderJ": 4, "LArmJ": 5, "LHandN": 6, "ShieldN": 5,
    "RShoulderJ": 11, "RArmJ": 12, "RHandN": 13, "RStickN": 14,
    "NeckN": 7, "HeadN": 8,
}
# Per object: a fixed row, or a rule name (vertex and face rules below).
OBJECT_ROWS = {
    "DS_face": 8, "DS_hair_cap": 8,
    # The long hair hangs from the head on Peach's row 9 (the head's child,
    # which draws nothing else in her look): her Ultimate clips turn it along
    # Ultimate's hair chains (ultimate.py), every other clip holds it to the
    # bodice (skirt_keys.py) as it rode before (owner p6p, 2026-10-15: on the
    # bodice it stood out behind her lunges like a board).
    "DS_hair_panels": 9,
    "DS_chest": "chest", "DS_dress": "dress", "DS_arms": "bones", "DS_hands": "bones",
    "DS_legs": "legs",
    "DS_staff": 14, "DS_staff_gem_bb": 14, "DS_staff_wings_sprite": 14,
    "DS_shield_sprite": 5,
}
WAIST_Y = 13.2          # kit height of the dress waist ring (bodice above, skirt below)
ANKLE_Y = 1.3           # kit height of the ankle ring (the shoe below it)
# Two-sided parts: the dress (its inside shows through the slit), the long
# hair (one sheet), the sprites.
TWO_SIDED = {"DS_dress", "DS_hair_panels", "DS_staff_gem_bb", "DS_staff_wings_sprite", "DS_shield_sprite"}
TEXTURE_SCALE = 0.5     # of the baked size, both details (a texture is shared)
# Palette entries a patch of a texture keeps for itself: the box (u0, v0, u1,
# v1 of the baked image, v from its top) and how many entries its texels
# get. k-means over the whole image folds a small patch of a distinct hue into
# a large neighbour: the dress's skin between the bust and the neck (the
# baseline paints it on the dress mesh, beside the chest's skin) became the
# cloth's cream, a pale inverted V on her chest (owner playtest 2026-10-14).
KEEP_PATCHES = {"DS_dress": [((0.375, 0.015625, 0.625, 0.0625), 2)]}

# The staff. The owner's model holds it as the kit's bind pose does: level,
# pointing forward from the right hand, which the base's clips turn into a
# lance held out at about 60 degrees from vertical. It is turned so that the
# base's idle stands it beside her: Peach's Wait clip (motion 4), averaged
# over its frames, carries HELD_AXIS (rest model space) to 10 degrees off
# vertical, leaning forward, and HELD_NORMAL to her side (the battle camera's
# view of a fighter); the wing sprite faces HELD_NORMAL, and the gem quad (no
# billboards yet) too. The shaft slides through the hand until its foot is
# HELD_FOOT below the grip: the idle's hand hangs at the hip (the wrist at y
# 280), so the foot meets the floor; the kit's grip, high on the shaft, put it
# 100 units under the floor.
HELD = ("DS_staff", "DS_staff_gem_bb", "DS_staff_wings_sprite")
HELD_AXIS = (0.11265, 0.72326, 0.68133)
HELD_NORMAL = (0.9296, 0.16547, -0.32935)
HELD_FOOT = 269.0


class LookError(SystemExit):
    pass


def dominant(groups: list) -> str | None:
    return groups[0][0] if groups else None


def vertex_row(obj: str, v: dict) -> int:
    """The row a vertex belongs to by its own bone (before face rules)."""
    rule = OBJECT_ROWS[obj]
    if isinstance(rule, int):
        return rule
    g = dominant(v["groups"])
    if rule == "legs":
        left = v["kit"][0] > 0
        if g and g.endswith("FootJ"):
            if v["kit"][1] >= ANKLE_Y:
                return 18 if left else 23
            return 19 if left else 24
        if g and g.endswith("KneeJ"):
            return 17 if left else 22
        return 16 if left else 21
    if g in BONE_ROWS:
        return BONE_ROWS[g]
    raise LookError(f"{obj}: no row for bone {g!r}")


def face_rows(obj: str, verts: list, tri: list) -> list:
    """The row of each corner of one triangle under the object's rule."""
    rule = OBJECT_ROWS[obj]
    if rule == "dress":
        # The bodice and the skirt's waist ring ride the torso; the rest of
        # the skirt the hips (the legs' parent), the skirt between them a
        # seam (the torso's list draws it from the hips' last load). The base's
        # own dress rows are not used: their clips scale and move them for a
        # floor-length gown.
        return [2 if verts[i]["kit"][1] >= WAIST_Y - 1e-3 else 1 for i in tri]
    if rule == "chest":
        neck = any(dominant(verts[i]["groups"]) == "NeckN" for i in tri)
        return [7, 7, 7] if neck else [2, 2, 2]
    return [vertex_row(obj, verts[i]) for i in tri]


def depth_of(parents: list, j: int) -> int:
    d = 0
    while parents[j] >= 0:
        j = parents[j]
        d += 1
    return d


def settle(rows: list, parents: list) -> list:
    """Corner rows a triangle may keep: one row, or a parent and the child
    drawn right after it (a seam); otherwise all on the deepest row."""
    s = set(rows)
    if len(s) == 1:
        return rows
    if len(s) == 2:
        a, b = sorted(s, key=lambda j: depth_of(parents, j))
        if parents[b] == a and b == a + 1:
            return rows
    deep = max(s, key=lambda j: depth_of(parents, j))
    return [deep] * 3


# ------------------------------------------------------------------ skeleton

def skeleton(kit: D.Model, base: dict) -> tuple[dict, list, list, dict]:
    """The look's joint worlds (4x4, the base's rotations), the fit, and the
    translate_scales table with the rows it could not scale."""
    fit = kit_fit.fit(kit, base, kit_fit.ROLE_MAPS["peach"], "peach")
    skel = [np.array(p, dtype=float) for p in fit["skeleton"]]
    for row, at in COINCIDE.items():
        skel[row] = skel[at].copy()
    parents = [j["parent"] for j in base["joints"]]
    base_worlds = [np.array(j["world"], dtype=float) for j in base["joints"]]
    worlds = []
    for j, p in enumerate(parents):
        if j in KEEP_BASE:
            local = base_worlds[j] if p < 0 else np.linalg.inv(base_worlds[p]) @ base_worlds[j]
            worlds.append(local if p < 0 else worlds[p] @ local)
        else:
            w = base_worlds[j].copy()
            w[:3, 3] = skel[j]
            worlds.append(w)
    scales = [[1.0, 1.0, 1.0] for _ in range(JOINT_COUNT)]
    unscaled = {}
    for j, p in enumerate(parents):
        if p < 0 or j in KEEP_BASE:
            continue
        new = (np.linalg.inv(worlds[p]) @ worlds[j])[:3, 3]
        old = np.array(base["joints"][j]["translate"], dtype=float)
        for k in range(3):
            if abs(old[k]) > 1.0:
                # Negative where the look's joint sits on the other side of
                # its parent (the thigh roots: below Peach's hips, above the
                # look's): the guard's rest (ShieldPose rows times the scale)
                # needs it; no clip keys those rows.
                scales[j + ROW_JOINT][k] = round(float(new[k] / old[k]), 4)
            elif abs(new[k] - old[k]) > 1.0:
                # A keyed translation, and the guard's rest, play the base's offset here.
                unscaled.setdefault(str(j), []).append([k, round(float(old[k]), 2), round(float(new[k]), 2)])
    return fit, worlds, scales, unscaled


# ------------------------------------------------------------------ textures

def _lloyd(pts: np.ndarray, c: np.ndarray, fixed: int = 0) -> np.ndarray:
    """Lloyd iterations from centres `c`; the first `fixed` never move."""
    for _ in range(32):
        lab = ((pts[:, None, :] - c[None]) ** 2).sum(2).argmin(1)
        new = np.array([c[i] if i < fixed or not np.any(lab == i) else pts[lab == i].mean(0)
                        for i in range(len(c))])
        if np.allclose(new, c):
            break
        c = new
    return c


def _kmeans(pts: np.ndarray, k: int, rng) -> np.ndarray:
    """k centres for RGB points: k-means++ seeding, then Lloyd iterations."""
    centres = [pts[rng.integers(len(pts))]]
    for _ in range(1, min(k, len(np.unique(pts, axis=0)))):
        d2 = np.min(((pts[:, None, :] - np.array(centres)[None]) ** 2).sum(2), axis=1)
        centres.append(pts[rng.choice(len(pts), p=d2 / d2.sum())] if d2.sum() > 0 else pts[rng.integers(len(pts))])
    return _lloyd(pts, np.array(centres))


def _cielab(rgb: np.ndarray) -> np.ndarray:
    """sRGB 0-255 to CIELAB (D65)."""
    c = np.asarray(rgb, dtype=float) / 255.0
    c = np.where(c > 0.04045, ((c + 0.055) / 1.055) ** 2.4, c / 12.92)
    xyz = c @ np.array([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722],
                        [0.0193, 0.1192, 0.9505]]).T / np.array([0.95047, 1.0, 1.08883])
    f = np.where(xyz > 0.008856, np.cbrt(xyz), 7.787 * xyz + 16.0 / 116.0)
    return np.stack([116.0 * f[..., 1] - 16.0, 500.0 * (f[..., 0] - f[..., 1]),
                     200.0 * (f[..., 1] - f[..., 2])], -1)


def _keep_patches(pts: np.ndarray, c: np.ndarray, rgba: np.ndarray, cutout: bool, keep) -> np.ndarray:
    """KEEP_PATCHES: each patch's own centres (k-means over its texels) take
    the places of the centres whose texels lose least, by eye (CIELAB), when
    they move to their next nearest -- near-duplicate shades, not a small
    patch of a distinct hue as an RGB cost would pick (her dress's red) --
    then the rest are refitted around them."""
    h, w = rgba.shape[:2]
    fixed: list = []
    for (u0, v0, u1, v1), entries in keep:
        patch = rgba[int(v0 * h):int(v1 * h), int(u0 * w):int(u1 * w)].reshape(-1, 4).astype(float)
        patch = patch[patch[:, 3] >= 128, :3] if cutout else patch[:, :3]
        if len(patch) == 0:
            raise LookError(f"keep patch {(u0, v0, u1, v1)} has no texels")
        fixed += list(_kmeans(patch, entries, np.random.default_rng(1)))
    lab_pts = _cielab(pts)
    for _ in fixed:
        near = ((pts[:, None, :] - c[None]) ** 2).sum(2).argmin(1)
        d = ((lab_pts[:, None, :] - _cielab(c)[None]) ** 2).sum(2)
        cost = [float((np.delete(d[near == i], i, axis=1).min(1) - d[near == i, i]).sum())
                if np.any(near == i) else 0.0 for i in range(len(c))]
        c = np.delete(c, int(np.argmin(cost)), axis=0)
    return _lloyd(pts, np.vstack([np.array(fixed), c]), len(fixed))


def quantize(rgba: np.ndarray, colors: int, cutout: bool, keep=()) -> tuple[list, list]:
    """Indices (rows of ints) and a palette for an RGBA image: k-means in RGB;
    with `cutout`, index 0 is clear and every alpha < 128 texel takes it.
    `keep`: KEEP_PATCHES rows for this image."""
    h, w = rgba.shape[:2]
    px = rgba.reshape(-1, 4).astype(float)
    opaque = px[:, 3] >= 128 if cutout else np.ones(len(px), dtype=bool)
    k = colors - 1 if cutout else colors
    pts = px[opaque, :3]
    if len(pts) == 0:
        pts = px[:, :3]
    c = _kmeans(pts, k, np.random.default_rng(1))
    if keep:
        c = _keep_patches(pts, c, rgba, cutout, keep)
    palette = [[int(round(x)) for x in col] + [255] for col in c]
    while len(palette) < k:
        palette.append(list(palette[-1]))
    idx = ((px[:, None, :3] - np.array([p[:3] for p in palette], dtype=float)[None]) ** 2).sum(2).argmin(1)
    if cutout:
        idx = np.where(opaque, idx + 1, 0)
        palette = [[0, 0, 0, 0]] + palette
    return idx.reshape(h, w).tolist(), palette


def load_texture(path: Path, width: int, height: int) -> np.ndarray:
    from PIL import Image
    img = Image.open(path).convert("RGBA")
    if img.size != (width, height):
        img = img.resize((width, height), Image.BOX)
    return np.asarray(img)


def _unit(v) -> np.ndarray:
    v = np.asarray(v, dtype=float)
    return v / (np.linalg.norm(v) or 1.0)


def _turn(a, b) -> np.ndarray:
    """The smallest rotation taking unit a to unit b."""
    v = np.cross(a, b)
    s = np.linalg.norm(v)
    c = float(np.dot(a, b))
    if s < 1e-9:
        return np.eye(3)
    k = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + k + k @ k * ((1 - c) / (s * s))


def hold_staff(posed: dict, dump: dict) -> dict:
    """The held parts turned and slid (HELD_*), in place."""
    staff = np.array(posed["DS_staff"])
    centre = staff.mean(0)
    axis = np.linalg.svd(staff - centre)[2][0]
    gem = np.array(posed["DS_staff_gem_bb"]).mean(0)
    if np.dot(gem - centre, axis) < 0:
        axis = -axis
    wings = np.array(posed["DS_staff_wings_sprite"])
    normal = _unit(np.cross(wings[1] - wings[0], wings[2] - wings[0]))
    normal = _unit(normal - np.dot(normal, axis) * axis)
    hands = next(o for o in dump["objects"] if o["name"] == "DS_hands")
    hand = np.array([p for p, v in zip(posed["DS_hands"], hands["vertices"]) if v["kit"][0] < 0]).mean(0)
    grip = centre + np.dot(hand - centre, axis) * axis
    a = _unit(HELD_AXIS)
    n = _unit(np.array(HELD_NORMAL) - np.dot(HELD_NORMAL, a) * a)
    turn = np.column_stack([a, n, np.cross(a, n)]) @ np.column_stack([axis, normal, np.cross(axis, normal)]).T
    foot = float(np.min((staff - grip) @ axis))
    slide = -HELD_FOOT - foot
    for name in HELD:
        posed[name] = [grip + turn @ (p - grip) + slide * a for p in posed[name]]
    # The gem quad faced along the shaft: turn it about its centre to the side.
    q = np.array(posed["DS_staff_gem_bb"])
    qc = q.mean(0)
    qn = _unit(np.cross(q[1] - q[0], q[2] - q[0]))
    r = _turn(qn, n if np.dot(qn, n) >= 0 else -n)
    posed["DS_staff_gem_bb"] = [qc + r @ (p - qc) for p in q]
    return {"grip": [round(float(c), 1) for c in grip], "slide": round(slide, 1), "turn_degrees":
            round(float(np.degrees(np.arccos(np.clip((np.trace(turn) - 1) / 2, -1, 1)))), 1)}


def _pow2_down(n: int, least: int) -> int:
    p = least
    while p * 2 <= n:
        p *= 2
    return p


# ---------------------------------------------------------------------- scene

def build(dump: dict, kit: D.Model, base: dict, fit_base: dict, detail: str, out: Path) -> dict:
    """`base`: the base's scene for this detail (preludes); `fit_base`: the
    high scene, so both details share one skeleton."""
    out.mkdir(parents=True, exist_ok=True)
    if [j["parent"] for j in base["joints"]] != [j["parent"] for j in fit_base["joints"]]:
        raise LookError("the detail's tree differs from the high tree")
    fit, worlds, scales, unscaled = skeleton(kit, fit_base)
    s = fit["scale"]
    kj = {j.name: j for j in kit.joints}
    fj = {j["name"]: j for j in fit["joints"]}

    def pose(bone: str, x) -> np.ndarray:
        k = kj[bone]
        kr = np.array([[k.world[0], k.world[1], k.world[2]], [k.world[4], k.world[5], k.world[6]],
                       [k.world[8], k.world[9], k.world[10]]])
        kp = np.array([k.world[3], k.world[7], k.world[11]])
        f = fj[bone]
        return np.array(f["pos"]) + s * (np.array(f["rot"]) @ (kr.T @ (np.asarray(x, dtype=float) - kp)))

    parents = [j["parent"] for j in base["joints"]]
    inv = [np.linalg.inv(w) for w in worlds]
    vertices, triangles, materials = [], [], []
    vkey: dict = {}
    report = {"rows": {}, "moved": 0, "seams": 0, "unscaled": unscaled,
              "translate_scales": {str(j - ROW_JOINT): v for j, v in enumerate(scales) if v != [1.0, 1.0, 1.0]}}
    posed_all = {}
    for obj in dump["objects"]:
        if obj["name"] not in OBJECT_ROWS:
            raise LookError(f"no row rule for {obj['name']}")
        posed_all[obj["name"]] = [pose(dominant(v["groups"]) or "HeadN", v["kit"]) for v in obj["vertices"]]
    if all(name in posed_all for name in HELD):
        report["held"] = hold_staff(posed_all, dump)
    for obj in dump["objects"]:
        name = obj["name"]
        verts = obj["vertices"]
        posed = posed_all[name]
        # Smooth normals in model space (area-weighted over the object).
        nsum = [np.zeros(3) for _ in verts]
        for t in obj["triangles"]:
            a, b, c = (posed[i] for i in t["v"])
            fn = np.cross(b - a, c - a)
            for i in t["v"]:
                nsum[i] += fn
        mats = []
        for m in obj["materials"]:
            img = m["image"]
            if img is None:
                raise LookError(f"{name}: material {m['name']} has no image")
            bw, bh = img["size"]
            w = _pow2_down(int(bw * TEXTURE_SCALE), 16)
            h = _pow2_down(int(bh * TEXTURE_SCALE), 8)
            cutout = bool(m["alpha"])
            texels, palette = quantize(load_texture(Path(img["file"]), w, h), 16, cutout,
                                       KEEP_PATCHES.get(name, ()))
            mats.append(len(materials))
            materials.append({"combine": [0, 0], "lit": True, "prim": "ffffffff", "spans": {},
                              "two_sided": name in TWO_SIDED,
                              "texture": {"file": name, "width": w, "height": h, "texels": texels,
                                          "palette": palette, "alpha": cutout,
                                          "wrap": ["clamp", "clamp"] if cutout else ["repeat", "clamp"]}})
        for t in obj["triangles"]:
            want = face_rows(name, verts, t["v"])
            rows = settle(want, parents)
            if len(set(rows)) == 2:
                report["seams"] += 1
            if rows != want:
                report["moved"] += 1
            corners = []
            for k, i in enumerate(t["v"]):
                row = rows[k]
                n = nsum[i] / (np.linalg.norm(nsum[i]) or 1.0)
                nl = inv[row][:3, :3] @ n
                nl = nl / (np.linalg.norm(nl) or 1.0)
                uv = t["uv"][k] if t["uv"] else [0.0, 0.0]
                key = (name, i, row, round(uv[0], 4), round(uv[1], 4))
                if key not in vkey:
                    local = inv[row] @ np.append(posed[i], 1.0)
                    if any(not -0x8000 <= round(c) <= 0x7FFF for c in local[:3]):
                        raise LookError(f"{name} vertex {i}: local {local[:3]} overflows s16")
                    vkey[key] = len(vertices)
                    vertices.append({"joint": row, "local": [round(float(c), 3) for c in local[:3]],
                                     "model": [round(float(c), 3) for c in posed[i]],
                                     "uv": [float(uv[0]), float(uv[1])], "st": None, "flag": 0,
                                     "normal": [int(round(c * 127)) for c in nl], "rgba": [255, 255, 255, 255],
                                     "object": name})
                corners.append(vkey[key])
            drawn = max(set(rows), key=lambda j: depth_of(parents, j))
            report["rows"][str(drawn)] = report["rows"].get(str(drawn), 0) + 1
            triangles.append({"joint": drawn, "v": corners, "material": mats[t["material"]], "object": name})
    joints = []
    for j, bj in enumerate(base["joints"]):
        jj = dict(bj)
        jj["world"] = worlds[j].tolist()
        jj["has_list"] = any(t["joint"] == j for t in triangles)
        joints.append(jj)
    scene = {"detail": detail, "costume": 0, "joints": joints, "vertices": vertices, "triangles": triangles,
             "materials": materials, "preludes": base.get("preludes") or {}, "source": "look_blend.py",
             "fit_scale": s, "translate_scales": scales}
    (out / "scene.json").write_text(json.dumps(scene), encoding="utf-8")
    report.update({"triangles": len(triangles), "vertices": len(vertices), "materials": len(materials),
                   "texture_bytes": sum(m["texture"]["width"] * m["texture"]["height"] // 2 + 32 for m in materials)})
    (out / "report.json").write_text(json.dumps(report, indent=1, sort_keys=True), encoding="utf-8")
    return report


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--dump", type=Path, required=True)
    ap.add_argument("--kit", type=Path, required=True)
    ap.add_argument("--base", type=Path, required=True, help="the base's scene.json for this detail")
    ap.add_argument("--fit-base", type=Path, help="the base's high scene.json (default: --base)")
    ap.add_argument("--detail", choices=("high", "low"), required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    dump = json.loads(args.dump.read_text(encoding="utf-8"))
    kit = D.read(args.kit)
    base = json.loads(args.base.read_text(encoding="utf-8"))
    fit_base = json.loads(args.fit_base.read_text(encoding="utf-8")) if args.fit_base else base
    report = build(dump, kit, base, fit_base, args.detail, args.out)
    print(json.dumps({k: report[k] for k in ("triangles", "vertices", "materials", "texture_bytes", "seams",
                                              "moved", "rows", "unscaled")}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
