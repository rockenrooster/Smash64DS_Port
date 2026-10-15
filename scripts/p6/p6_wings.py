#!/usr/bin/env python3
"""P6 Palutena's wings in her forward and down smash, as a rigged effect
(owner playtest p6i follow-up, 2026-10-14: "the wings need lots of work, they
should articulate and stretch and be mostly symmetrical ... polished and like
they belong in the DS version"; then "your wings look disconnected in
parts").

The wings are an effect of their own in her special file 2, drawn natively as
her pillar is (scripts/p4/p4_articles.py "wings"), with a skeleton: the kit's
godwing (godwing.dae: a wing a side, four bones each, LwingNa-d; the right
wing the left's mirror) at the look's fit_scale.

Art. A wing is one continuous mesh: the kit wing's feathers rendered flat in
the wing's plane from its outer side (an impostor) into one 128x32 CI4 image
(colour 0 clear; the feathers' edge texels darkened a little, RIM_SHADE, so
the white wings keep their shape over a bright stage), mapped onto a grid of
COLS columns along the wing by ROWS rows across it. Each column rides one
bone (the bone whose span along the wing holds it); a quad between two
bones' columns is drawn by the later bone with the earlier bone's vertices
still in the vertex cache (each bone loads into its own half of the cache),
so it stretches across the joint (the entry-effect compiler records those
corners' earlier matrix; the runtime keeps its slot) and the wing bends
without gaps. The first pass's card a bone came apart at the joints.

Tree (DObjDesc depth in brackets):

    [0] root    her TopN's transform, copied every tic
    [1] torso   her torso row (2) relative to TopN, from her smash clip
    [2] pair    the wings' root between her shoulder blades (the kit's
                WingN, on BustN); its scale grows and shrinks the wings
    [3] La..Ld  the left wing's bones, each relative to the pair (the chain
                composed offline), its list its part of the mesh
    [3] Ra..Rd  the right wing's, the left's mirror

Motion. art/p6/palutena/wings/<smash>.json keys each wing's pose on her
smash clip's frames (the owner's 3DS reference beside it): the pair's size
("size"), its "level" (0..1 of the way from her torso's turn to her TopN's
axes: 1 keeps the wings in the battle camera's plane however her chest
twists), "pair_yaw" (a quarter turn lays the wings across the screen for a
body seen side on), "pair_roll" (one wing a little above the other) and
"pair_pitch"; bone a's turn in the pair's frame ("a": pitch, forward and
back; yaw, out and in; roll, up and down) and "twist" (the whole wing about
its own length: a wing laid flat reads as a thin arc from the side, the
3DS down smash's), bones b-d's turns in their own bind frames ("b".."d":
twist, curl out of the wing's plane, fold in it), each bone's "stretch"
(along its length, its child's distance with it) and "fan" (across it, the
feathers' length). Keys pass through a Catmull-Rom spline, stopping at the
ends and at "hold" keys. The bake samples every frame of the clip in the
visible span: the torso node from the clip's own pose (anim.py, keyposes.py,
the look scene's skeleton), the pair, and each bone's translation, rotation
(BattleShip's RPY Euler angles: syMatrixRotRpyRF, R = Rz Ry Rx) and scale
relative to the pair, into a table her code reads every tic
(src/port/nds_p4_palutena.c) at her animation frame (fighter anim_frame), so
the wings stay on her through hitlag and leave with the status.

Kit-derived: the image, mesh and table live in builds/ (D10); this module
and the motion recipes are the committed part. p6_gate.py writes the tree
into her special file 2 (write); world_mesh gives a pose's triangles in
model space (review renders, and her victory poses' static wings:
look_parts.py "wings_pose").

    p6_wings.py [--image wing.png]      (the rig, the mesh and the bake's spans)
"""
from __future__ import annotations

import argparse
import collections
import json
import math
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
import dae_reader  # noqa: E402

KIT = REPO / "decomp" / "alt_assets" / "Fighters" / "Super Smash Bros 3DS" / "Palutena" / "godwing"
SCENE = REPO / "builds" / "p6-art" / "palutena" / "3ds2ds" / "scene_high" / "scene.json"
RECIPES = REPO / "art" / "p6" / "palutena" / "wings"
BONES = ("a", "b", "c", "d")
TORSO_ROW = 2                 # her torso row (fighter joint 6)
TEX_W, TEX_H = 128, 32        # the wing's image: CI4, TMEM's half (2 KB)
COLS = (3, 3, 3, 4)           # mesh columns a bone (d's last at the tip)
ROWS = 3                      # across the wing: leading edge, middle, feathers' ends
BANK = 16                     # vertex cache slots a bone (two halves of 32)
RENDER_DENSITY = 2.0          # pixels a unit for the high-resolution render
ANGLE_UNIT = 65536.0 / (2.0 * math.pi)   # table angles: s16 binary angle
SCALE_UNIT = 4096.0                      # table scales: s16 / 4096


class WingError(SystemExit):
    pass


# ---------------------------------------------------------------- kit wing

@dataclass
class Bone:
    name: str                 # "a".."d"
    origin: np.ndarray        # in the pair node's frame (her torso's axes)
    frame: np.ndarray         # 3x3, columns x (along the bone), y, z (normal)


@dataclass
class Vert:
    bone: int                 # the bone it rides (0..3)
    local: np.ndarray         # in that bone's frame
    st: tuple                 # texels in the wing's image


@dataclass
class Wing:
    pair_origin: np.ndarray   # the pair node in her torso row's frame
    bones: list               # Bone a..d (left wing)
    normal: np.ndarray
    fit: float
    image: np.ndarray = None  # RGBA, TEX_H x TEX_W (before the rim)
    verts: list = field(default_factory=list)        # Vert, column by column
    tris: dict = field(default_factory=dict)         # bone -> [(i, j, k)]
    bone_verts: dict = field(default_factory=dict)   # bone -> [vertex index]


def _scene() -> dict:
    return json.loads(SCENE.read_text(encoding="utf-8"))


def _dominant(mesh, tri, names) -> str:
    acc = collections.Counter()
    for i in tri:
        for j, w in mesh.weights[i]:
            acc[j] += w
    return names[acc.most_common(1)[0][0]]


def _render(tris: np.ndarray, uvs: np.ndarray, tex: np.ndarray, box: tuple,
            density: float) -> np.ndarray:
    """Triangles (plane u, v, depth) textured into an image covering `box`
    (u0, u1, v0, v1), seen from +depth; rows run from v1 down to v0."""
    u0, u1, v0, v1 = box
    w = max(1, int(math.ceil((u1 - u0) * density)))
    h = max(1, int(math.ceil((v1 - v0) * density)))
    img = np.zeros((h, w, 4), dtype=float)
    zbuf = np.full((h, w), -1e30)
    th, tw = tex.shape[:2]
    for k in range(len(tris)):
        p = tris[k]
        sx = (p[:, 0] - u0) * density
        sy = (v1 - p[:, 1]) * density
        x0, x1 = int(max(0, math.floor(sx.min()))), int(min(w - 1, math.ceil(sx.max())))
        y0, y1 = int(max(0, math.floor(sy.min()))), int(min(h - 1, math.ceil(sy.max())))
        if x1 < x0 or y1 < y0:
            continue
        ax, ay, bx, by, cx, cy = sx[0], sy[0], sx[1], sy[1], sx[2], sy[2]
        den = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy)
        if abs(den) < 1e-9:
            continue
        xs, ys = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
        w0 = ((by - cy) * (xs - cx) + (cx - bx) * (ys - cy)) / den
        w1 = ((cy - ay) * (xs - cx) + (ax - cx) * (ys - cy)) / den
        w2 = 1.0 - w0 - w1
        inside = (w0 >= -1e-4) & (w1 >= -1e-4) & (w2 >= -1e-4)
        if not inside.any():
            continue
        z = w0 * p[0, 2] + w1 * p[1, 2] + w2 * p[2, 2]
        u = w0 * uvs[k, 0, 0] + w1 * uvs[k, 1, 0] + w2 * uvs[k, 2, 0]
        v = w0 * uvs[k, 0, 1] + w1 * uvs[k, 1, 1] + w2 * uvs[k, 2, 1]
        tx = np.clip(((u % 1.0) * tw).astype(int), 0, tw - 1)
        ty = np.clip(((1.0 - (v % 1.0)) * th).astype(int), 0, th - 1)
        texel = tex[ty, tx].astype(float)
        sub = zbuf[y0:y1 + 1, x0:x1 + 1]
        ok = inside & (texel[..., 3] >= 40) & (z > sub)
        img[y0:y1 + 1, x0:x1 + 1][ok] = texel[ok]
        sub[ok] = z[ok]
    return img


def _box_premultiplied(src: np.ndarray, w: int, h: int) -> tuple[np.ndarray, np.ndarray]:
    """Area-averaged alpha and colour of an RGBA image at w x h, the colour
    averaged over coverage (premultiplied) so clear texels add no black."""
    from PIL import Image

    def box(chan: np.ndarray) -> np.ndarray:
        return np.asarray(Image.fromarray(chan.astype(np.float32), "F").resize((w, h), Image.BOX),
                          dtype=float)

    alpha = src[..., 3] / 255.0
    a = box(alpha)
    rgb = np.stack([box(src[..., c] * alpha) for c in range(3)], axis=-1)
    rgb = np.where(a[..., None] > 1e-3, rgb / np.maximum(a[..., None], 1e-3), 0.0)
    return a * 255.0, rgb.clip(0.0, 255.0)


def load_wing(scene: dict | None = None) -> Wing:
    """The kit's left wing: its four bones, its image and its mesh (module
    docstring)."""
    scene = scene or _scene()
    fit = float(scene["fit_scale"])
    torso = np.array(scene["joints"][TORSO_ROW]["world"], dtype=float)
    model = dae_reader.read(KIT / "godwing.dae")
    mesh = model.meshes[0]
    names = [j.name for j in model.joints]
    world = {j.name: np.array([j.world[3], j.world[7], j.world[11]]) * fit for j in model.joints}
    # The pair node: the kit's WingN, in her torso row's frame (its rest
    # frame is the model's axes: x her left, y up, z her front).
    pair_world = world["WingN"]
    pair = pair_world - torso[:3, 3]
    wing_tris = [tri for tri in mesh.triangles if _dominant(mesh, tri, names).startswith("Lwing")]
    pos = {i: np.array(model.skinned_position(mesh, i)) * fit - pair_world
           for tri in wing_tris for i in tri}
    pts = np.array(list(pos.values()))
    centred = pts - pts.mean(0)
    _u, _s, vt = np.linalg.svd(centred, full_matrices=False)
    normal = vt[2] / np.linalg.norm(vt[2])
    if normal[0] < 0:
        normal = -normal           # the left wing's outer side: her left
    origins = {b: world[f"LwingN{b}"] - pair_world for b in BONES}
    d_tris = [tri for tri in wing_tris if _dominant(mesh, tri, names) == "LwingNd"]
    d_pts = np.array([pos[i] for tri in d_tris for i in tri])
    tip = d_pts[np.argmax(np.linalg.norm(d_pts - origins["d"], axis=1))]
    bones = []
    for k, b in enumerate(BONES):
        nxt = origins[BONES[k + 1]] if k + 1 < len(BONES) else tip
        x = nxt - origins[b]
        x = x - normal * float(np.dot(x, normal))
        x /= np.linalg.norm(x)
        y = np.cross(normal, x)
        bones.append(Bone(b, origins[b], np.stack([x, y, normal], axis=1)))
    wing = Wing(pair, bones, normal, fit)
    # The wing's plane: from bone a's joint toward the tip (u), across (v).
    o = origins["a"]
    u_ax = tip - o
    u_ax = u_ax - normal * float(np.dot(u_ax, normal))
    u_ax /= np.linalg.norm(u_ax)
    v_ax = np.cross(normal, u_ax)
    plane = np.stack([u_ax, v_ax, normal], axis=1)
    tris = np.array([[(pos[i] - o) @ plane for i in tri] for tri in wing_tris])
    uvs = np.array([[mesh.pool.uvs[i] for i in tri] for tri in wing_tris], dtype=float)
    flat = tris.reshape(-1, 3)
    box = (float(flat[:, 0].min()) - 2.0, float(flat[:, 0].max()) + 2.0,
           float(flat[:, 1].min()) - 2.0, float(flat[:, 1].max()) + 2.0)
    big = _render(tris, uvs, _kit_texture(), box, RENDER_DENSITY)
    a, rgb = _box_premultiplied(big, TEX_W, TEX_H)
    wing.image = np.concatenate([rgb, a[..., None]], axis=-1)
    _mesh(wing, plane, o, box)
    return wing


def _mesh(wing: Wing, plane: np.ndarray, o: np.ndarray, box: tuple) -> None:
    """The grid: COLS columns a bone along u (a bone's from where its joint
    falls on u; bone a's from the image's start, d's last at its end), ROWS
    rows across; each vertex in its bone's frame, its texel in the image."""
    u0, u1, v0, v1 = box
    starts = [u0] + [float((b.origin - o) @ plane[:, 0]) for b in wing.bones[1:]] + [u1]
    cols = []
    for k in range(len(BONES)):
        n = COLS[k]
        last = k == len(BONES) - 1
        for i in range(n):
            frac = i / (n - 1) if last else i / n
            cols.append((starts[k] + (starts[k + 1] - starts[k]) * frac, k))
    rows = [v1 + (v0 - v1) * r / (ROWS - 1) for r in range(ROWS)]
    index = {}
    for c, (u, k) in enumerate(cols):
        bone = wing.bones[k]
        for r, v in enumerate(rows):
            p = o + plane[:, 0] * u + plane[:, 1] * v
            local = bone.frame.T @ (p - bone.origin)
            st = ((u - u0) / (u1 - u0) * TEX_W, (v1 - v) / (v1 - v0) * TEX_H)
            index[(c, r)] = len(wing.verts)
            wing.bone_verts.setdefault(k, []).append(len(wing.verts))
            wing.verts.append(Vert(k, local, st))
    for k in range(len(BONES)):
        if len(wing.bone_verts[k]) > BANK:
            raise WingError(f"bone {BONES[k]}: {len(wing.bone_verts[k])} vertices, a bank holds {BANK}")
        wing.tris[k] = []
    for c in range(len(cols) - 1):
        owner = cols[c + 1][1]
        if cols[c][1] not in (owner, owner - 1):
            raise WingError("a quad spans more than two bones")
        for r in range(ROWS - 1):
            q0, q1, q2, q3 = index[(c, r)], index[(c + 1, r)], index[(c + 1, r + 1)], index[(c, r + 1)]
            wing.tris[owner] += [(q0, q1, q2), (q0, q2, q3)]


def _kit_texture() -> np.ndarray:
    from PIL import Image
    return np.asarray(Image.open(KIT / "Yoshi_Wing.png").convert("RGBA"))


# The feathers' rim: each opaque texel beside a clear one darkened a little
# and cooled, so the white wings keep their shape over a bright stage.
RIM_SHADE = 0.86
RIM_TINT = (0.92, 0.95, 1.0)


def image_ci4(wing: Wing) -> tuple[list, list]:
    """The wing's image as CI4 indices and an RGBA palette (index 0 clear)."""
    import look_blend
    rgba = wing.image.copy()
    opaque = rgba[..., 3] >= 110
    rgba[..., 3] = np.where(opaque, 255, 0)
    edge = np.zeros_like(opaque)
    for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0)):
        edge |= opaque & ~np.roll(np.roll(opaque, dy, 0), dx, 1)
    rgba[edge, :3] = rgba[edge, :3] * RIM_SHADE * np.array(RIM_TINT)
    return look_blend.quantize(rgba.astype(np.uint8), 16, True)


def image_rgba(wing: Wing) -> np.ndarray:
    """The CI4 image back as RGBA (previews, her victory poses' texture)."""
    idx, pal = image_ci4(wing)
    return np.array([[pal[i] for i in row] for row in idx], dtype=np.uint8)


# ---------------------------------------------------------------- the rig

def rot_x(deg: float) -> np.ndarray:
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    return np.array([[1.0, 0.0, 0.0], [0.0, c, -s], [0.0, s, c]])


def rot_y(deg: float) -> np.ndarray:
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    return np.array([[c, 0.0, s], [0.0, 1.0, 0.0], [-s, 0.0, c]])


def rot_z(deg: float) -> np.ndarray:
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    return np.array([[c, -s, 0.0], [s, c, 0.0], [0.0, 0.0, 1.0]])


def rpy(r: np.ndarray) -> tuple[float, float, float]:
    """BattleShip's DObj rotation (rotate.x, .y, .z in radians) for a
    rotation matrix: syMatrixRotRpyRF builds R = Rz(z) Ry(y) Rx(x) (column
    vectors; its rows are R's columns)."""
    sy = -float(r[2, 0])
    y = math.asin(max(-1.0, min(1.0, sy)))
    if abs(sy) < 0.99999:
        x = math.atan2(float(r[2, 1]), float(r[2, 2]))
        z = math.atan2(float(r[1, 0]), float(r[0, 0]))
    else:                       # gimbal lock: all the turn in x
        x = math.atan2(-float(r[1, 2]), float(r[1, 1]))
        z = 0.0
    return x, y, z


def _log(r: np.ndarray) -> np.ndarray:
    """A rotation matrix's rotation vector (axis times angle, radians)."""
    c = max(-1.0, min(1.0, (float(np.trace(r)) - 1.0) / 2.0))
    ang = math.acos(c)
    if ang < 1e-9:
        return np.zeros(3)
    w = np.array([r[2, 1] - r[1, 2], r[0, 2] - r[2, 0], r[1, 0] - r[0, 1]])
    if math.pi - ang < 1e-6:
        # Half a turn: the axis from the symmetric part.
        k = int(np.argmax(np.diag(r)))
        axis = (r[:, k] + np.eye(3)[:, k]) / math.sqrt(2.0 * (1.0 + r[k, k]))
        return axis * ang
    return w / (2.0 * math.sin(ang)) * ang


def _exp(v: np.ndarray) -> np.ndarray:
    ang = float(np.linalg.norm(v))
    if ang < 1e-12:
        return np.eye(3)
    k = v / ang
    kx = np.array([[0.0, -k[2], k[1]], [k[2], 0.0, -k[0]], [-k[1], k[0], 0.0]])
    return np.eye(3) + math.sin(ang) * kx + (1.0 - math.cos(ang)) * (kx @ kx)


@dataclass
class Params:
    """A wing pose (module docstring): degrees, scales."""
    size: float = 1.0
    pair_pitch: float = 0.0
    level: float = 0.0
    pair_yaw: float = 0.0               # the pair turned about her vertical
    pair_roll: float = 0.0              # one wing up, the other down
    twist: float = 0.0                  # the whole wing about its own length
    a: tuple = (0.0, 0.0, 0.0)          # pitch, yaw, roll in the pair's frame
    bends: dict = field(default_factory=lambda: {b: (0.0, 0.0, 0.0) for b in BONES[1:]})
    stretch: dict = field(default_factory=lambda: {b: 1.0 for b in BONES})
    fan: dict = field(default_factory=lambda: {b: 1.0 for b in BONES})


def bone_poses(wing: Wing, p: Params) -> list[tuple[np.ndarray, np.ndarray, tuple]]:
    """Each bone's (rotation, translation, (stretch, fan)) relative to the
    pair node: the chain composed (a child's joint moves with its parent's
    stretch), its own scale left for its own vertices."""
    out = []
    parent_frame, parent_origin, parent_stretch = np.eye(3), np.zeros(3), 1.0
    rot_w, trans_w = np.eye(3), np.zeros(3)
    for k, bone in enumerate(wing.bones):
        rot = parent_frame.T @ bone.frame
        trans = parent_frame.T @ (bone.origin - parent_origin)
        if k == 0:
            # The wing turns about its own root (the kit's LwingNa), the
            # turn's axes the pair's; then about its own length.
            pitch, yaw, roll = p.a
            rot = rot_z(roll) @ rot_y(yaw) @ rot_x(pitch) @ rot @ rot_x(p.twist)
            rot_w, trans_w = rot, trans
        else:
            twist, curl, fold = p.bends[bone.name]
            rot = rot @ rot_x(twist) @ rot_y(curl) @ rot_z(fold)
            trans_w = trans_w + rot_w @ (trans * parent_stretch)
            rot_w = rot_w @ rot
        out.append((rot_w.copy(), trans_w.copy(), (p.stretch[bone.name], p.fan[bone.name])))
        parent_frame, parent_origin = bone.frame, bone.origin
        parent_stretch = p.stretch[bone.name]
    return out


def pair_local(wing: Wing, p: Params, torso: np.ndarray) -> tuple[np.ndarray, np.ndarray, float]:
    """The pair node's local rotation, translation and scale under the torso
    node: `level` of the way to undoing the torso's turn, then its yaw, roll
    and pitch."""
    tr = torso[:3, :3] / np.linalg.norm(torso[:3, :3], axis=0)
    undo = _exp(_log(tr.T) * p.level)
    return (undo @ rot_y(p.pair_yaw) @ rot_z(p.pair_roll) @ rot_x(p.pair_pitch),
            wing.pair_origin.copy(), p.size)


MIRROR = np.diag([-1.0, 1.0, 1.0])


def world_mesh(wing: Wing, torso: np.ndarray, p: Params) -> list:
    """Model-space (TopN-relative) triangles of both wings for a pose:
    [(side, corners 3x3, st 3x2)]; torso is her torso row's 4x4 model
    matrix."""
    pr, pt, ps = pair_local(wing, p, torso)
    pair_r = torso[:3, :3] @ pr * ps
    pair_t = torso[:3, :3] @ pt + torso[:3, 3]
    poses = bone_poses(wing, p)
    out = []
    for side in ("L", "R"):
        placed = []
        for vert in wing.verts:
            rot, trans, (sx, sy) = poses[vert.bone]
            local = vert.local * np.array([sx, sy, 1.0])
            if side == "R":
                rot, trans, local = MIRROR @ rot @ MIRROR, MIRROR @ trans, MIRROR @ local
            placed.append(pair_r @ (rot @ local + trans) + pair_t)
        for k in range(len(BONES)):
            for tri in wing.tris[k]:
                out.append((side, np.array([placed[i] for i in tri]),
                            np.array([wing.verts[i].st for i in tri], dtype=float)))
    return out


# ---------------------------------------------------------------- the motion

KEY_FIELDS = ("size", "pair_pitch", "level", "pair_yaw", "pair_roll", "twist", "a", "b", "c",
              "d", "stretch", "fan")


def _key_vector(key: dict) -> np.ndarray:
    """A key's parameters as one vector (defaults where it names none)."""
    unknown = set(key) - set(KEY_FIELDS) - {"frame", "hold", "comment"}
    if unknown:
        raise WingError(f"key at frame {key.get('frame')}: unknown fields {sorted(unknown)}")
    v = [float(key.get("size", 1.0)), float(key.get("pair_pitch", 0.0)),
         float(key.get("level", 0.0)), float(key.get("pair_yaw", 0.0)),
         float(key.get("pair_roll", 0.0)), float(key.get("twist", 0.0))]
    v += [float(x) for x in key.get("a", (0.0, 0.0, 0.0))]
    for b in BONES[1:]:
        v += [float(x) for x in key.get(b, (0.0, 0.0, 0.0))]
    stretch = key.get("stretch", 1.0)
    v += [float(x) for x in (stretch if isinstance(stretch, list) else [stretch] * 4)]
    fan = key.get("fan", 1.0)
    v += [float(x) for x in (fan if isinstance(fan, list) else [fan] * 4)]
    return np.array(v)


def _params(v: np.ndarray) -> Params:
    p = Params(size=float(v[0]), pair_pitch=float(v[1]), level=float(v[2]),
               pair_yaw=float(v[3]), pair_roll=float(v[4]), twist=float(v[5]), a=tuple(v[6:9]))
    p.bends = {b: tuple(v[9 + 3 * i:12 + 3 * i]) for i, b in enumerate(BONES[1:])}
    p.stretch = {b: float(v[18 + i]) for i, b in enumerate(BONES)}
    p.fan = {b: float(v[22 + i]) for i, b in enumerate(BONES)}
    return p


def params_of(key: dict) -> Params:
    """One key's pose (her victory poses' static wings)."""
    return _params(_key_vector(key))


def evaluate(recipe: dict, frame: float) -> Params | None:
    """The wing pose at a clip frame: the keys through a Catmull-Rom spline
    (zero tangents at the ends and at "hold" keys); None outside the keys
    (the wings hidden)."""
    keys = sorted(recipe["keys"], key=lambda k: k["frame"])
    frames = [float(k["frame"]) for k in keys]
    if frame < frames[0] or frame > frames[-1]:
        return None
    vecs = [_key_vector(k) for k in keys]
    if len(keys) == 1:
        return _params(vecs[0])
    i = max(j for j in range(len(keys) - 1) if frames[j] <= frame)
    i = min(i, len(keys) - 2)
    f0, f1 = frames[i], frames[i + 1]
    t = 0.0 if f1 == f0 else (frame - f0) / (f1 - f0)

    def tangent(j: int) -> np.ndarray:
        if j == 0 or j == len(keys) - 1 or keys[j].get("hold"):
            return np.zeros_like(vecs[j])
        return (vecs[j + 1] - vecs[j - 1]) / (frames[j + 1] - frames[j - 1])

    m0, m1 = tangent(i) * (f1 - f0), tangent(i + 1) * (f1 - f0)
    h00, h10 = 2 * t ** 3 - 3 * t ** 2 + 1, t ** 3 - 2 * t ** 2 + t
    h01, h11 = -2 * t ** 3 + 3 * t ** 2, t ** 3 - t ** 2
    return _params(h00 * vecs[i] + h10 * m0 + h01 * vecs[i + 1] + h11 * m1)


def load_recipe(name: str) -> dict:
    return json.loads((RECIPES / f"{name}.json").read_text(encoding="utf-8"))


# ---------------------------------------------------------------- her clip

def torso_frames(clip_name: str, scene: dict | None = None) -> list[np.ndarray]:
    """Her torso row's model-space matrix at every frame of an anims recipe's
    built clip (anim.py's build, the clip the game plays)."""
    sys.path.insert(0, str(REPO / "scripts" / "p4"))
    import anim as A
    import coverage as C
    import dress_check as DC
    scene = scene or _scene()
    f = C.load(REPO / "builds" / "p4-staging" / "extra-96621af",
               REPO / "builds" / "p4-staging" / "export-x" / "palutena")
    DC.apply_scene_skeleton(f, scene)
    recipe = json.loads((REPO / "art" / "p6" / "palutena" / "anims" / f"{clip_name}.json")
                        .read_text(encoding="utf-8"))
    if int(recipe.get("keyposes", {}).get("first_slot", 0)) != 0:
        raise WingError(f"{clip_name}: a TransN clip; the torso bake assumes none")
    clip = A.decode(A.build(recipe, f.rom))
    length = max((st.length for st in clip.streams.values()), default=0)
    return [mats[TORSO_ROW] for _root, mats in f.pose_frames(clip, 0, length)]


# ---------------------------------------------------------------- the table

ROW_VALUES = 6 + 4 + 8 * len(BONES)


def _s16(v: float) -> int:
    i = int(round(v))
    if not -0x8000 <= i <= 0x7FFF:
        raise WingError(f"table value {v} overflows s16")
    return i


def _angle16(a: float) -> int:
    """A turn in radians as an s16 binary angle, wrapped (a half turn either
    way is the same turn: +pi rounds to -0x8000)."""
    i = int(round(a * ANGLE_UNIT))
    return ((i + 0x8000) % 0x10000) - 0x8000


def bake_row(wing: Wing, torso: np.ndarray, p: Params) -> list[int]:
    """One frame's table row (src/port/nds_p4_palutena.c reads it): the
    torso node's translation and rotation; the pair's rotation and size; per
    left bone its translation, rotation and x and y scale (relative to the
    pair)."""
    row = []
    tr = torso[:3, :3]
    rot = tr / np.linalg.norm(tr, axis=0)
    row += [_s16(v) for v in torso[:3, 3]]
    row += [_angle16(a) for a in rpy(rot)]
    pr, _pt, ps = pair_local(wing, p, torso)
    row += [_angle16(a) for a in rpy(pr)] + [_s16(ps * SCALE_UNIT)]
    for rot_b, trans_b, (sx, sy) in bone_poses(wing, p):
        row += [_s16(v) for v in trans_b] + [_angle16(a) for a in rpy(rot_b)]
        row += [_s16(sx * SCALE_UNIT), _s16(sy * SCALE_UNIT)]
    if len(row) != ROW_VALUES:
        raise WingError(f"row of {len(row)} values, the reader takes {ROW_VALUES}")
    return row


def bake(wing: Wing, recipe: dict, torsos: list[np.ndarray]) -> tuple[int, int, list]:
    """(first frame, last frame, rows) of the frames the wings show."""
    keys = sorted(k["frame"] for k in recipe["keys"])
    first, last = int(math.ceil(keys[0])), min(int(math.floor(keys[-1])), len(torsos) - 1)
    rows = []
    for fr in range(first, last + 1):
        rows.append(bake_row(wing, torsos[fr], evaluate(recipe, fr)))
    return first, last, rows


# ---------------------------------------------------------------- the file

# The smash clips the table holds, in its order (src/port/nds_p4_palutena.c
# PAL_WINGS_CLIP_*): her forward smash (AttackS4 and its angled four) and
# down smash (AttackLw4).
CLIPS = ("fsmash", "dsmash")
TABLE_MAGIC = 0x50574E48      # "PWNH": the mesh's table (bones relative to the pair)
TREE_ROWS = 3 + 2 * len(BONES) + 1           # root, torso, pair, the bones, the end
G_LIGHTING = 0x00020000
G_CULL_BACK = 0x00000400
# The model writer's cut-out state (as p6_gate.py's pillar cards).
CUTOUT_ON = ((0xE2001E01, 0x00000001), (0xE200001C, 0xC4113078))
CUTOUT_OFF = ((0xE2001E01, 0x00000000), (0xE200001C, 0xC4112078))


def _vtx(x: float, y: float, z: float, s: float, t: float) -> bytes:
    xs, ys, zs = (int(round(c)) for c in (x, y, z))
    for c in (xs, ys, zs):
        if not -0x8000 <= c <= 0x7FFF:
            raise WingError(f"vertex ({x}, {y}, {z}) overflows s16")
    return struct.pack(">hhhHhh", xs, ys, zs, 0, int(round(s)), int(round(t))) + bytes((255, 255, 255, 255))


def _tri_cmds(n, tris: list) -> list:
    out = []
    for k in range(0, len(tris) - 1, 2):
        a, b = tris[k], tris[k + 1]
        out.append(n.Gfx(0x06000000 | (a[0] * 2 << 16) | (a[1] * 2 << 8) | a[2] * 2,
                         (b[0] * 2 << 16) | (b[1] * 2 << 8) | b[2] * 2))
    if len(tris) % 2:
        a = tris[-1]
        out.append(n.Gfx(0x05000000 | (a[0] * 2 << 16) | (a[1] * 2 << 8) | a[2] * 2, 0))
    return out


def _bone_list(n, file_id: int, tex_at: int, pal_at: int, vtx_at: int, count: int,
               base: int, tris: list) -> list:
    """A bone's list: the image and its palette loaded as the pillar's cards
    load theirs (p6_gate.py), the bone's vertices into its bank of the
    vertex cache (from `base`), its triangles by cache slot (those reaching
    back to the previous bone's bank use the vertices its list left there),
    unlit, two-sided, cut out."""
    line = TEX_W * 4 // 64
    masks, maskt = TEX_W.bit_length() - 1, TEX_H.bit_length() - 1
    clamp = 2
    cmds = [n.Gfx(0xE7000000, 0),
            n.Gfx(0xD9000000 | (0xFFFFFF & ~(G_LIGHTING | G_CULL_BACK)), 0),
            n.Gfx(0xE3001001, 0x00008000)]
    cmds += [n.Gfx(w0, w1) for w0, w1 in CUTOUT_ON]
    cmds += [n.Gfx(0xFC127E05, 0xFF17F3FF), n.Gfx(0xF9000000, 0),
             n.Gfx(0xE7000000, 0), n.Gfx(0xFA000000, 0xFFFFFFFF), n.Gfx(0xE8000000, 0),
             n.Gfx(0xF5000100, 0x05000000),
             n.Gfx(0xF5500000, (7 << 24) | (maskt << 14) | (masks << 4)),
             n.Gfx(0xF5400000 | (line << 9), (clamp << 18) | (maskt << 14) | (clamp << 8) | (masks << 4)),
             n.Gfx(0xFD100000, pal_at // 4, ("intern", file_id, pal_at)),
             n.Gfx(0xE6000000, 0), n.Gfx(0xF0000000, 0x0503C000), n.Gfx(0xE7000000, 0),
             n.Gfx(0xD7000002, 0xFFFFFFFF),
             n.Gfx(0xF2000000, (((TEX_W - 1) << 2) << 12) | ((TEX_H - 1) << 2)),
             n.Gfx(0xFD500000, tex_at // 4, ("intern", file_id, tex_at)),
             n.Gfx(0xE6000000, 0),
             n.Gfx(0xF3000000, (7 << 24) | ((TEX_W * TEX_H // 4 - 1) << 12) | (2048 // line)),
             n.Gfx(0xE7000000, 0),
             n.Gfx(0x01000000 | (count << 12) | ((base + count) << 1), vtx_at, ("intern", file_id, vtx_at))]
    cmds += _tri_cmds(n, tris)
    cmds += [n.Gfx(0xE7000000, 0)]
    cmds += [n.Gfx(w0, w1) for w0, w1 in CUTOUT_OFF]
    cmds += [n.Gfx(0xE3001001, 0x00000000),
             n.Gfx(0xD9FFFFFF, G_LIGHTING | G_CULL_BACK),
             n.Gfx(0xDF000000, 0)]
    return cmds


def _halfword_pairs(values: list) -> bytes:
    """s16 values for a file the loader byte-swaps a u32 at a time: each
    pair goes in swapped, so the runtime reads them in order."""
    if len(values) % 2:
        values = values + [0]
    return b"".join(struct.pack(">hh", values[i + 1], values[i]) for i in range(0, len(values), 2))


def write(app, file_id: int, tree_at: int, scene: dict | None = None) -> dict:
    """The wings into her special file (p6_gate.py's Appender `app`, the
    tree's TREE_ROWS rows reserved at `tree_at`): the image and palette,
    each bone's vertices and list (the right wing's mirrored), the tree,
    then the table. Returns the offsets."""
    import n64_model as n
    import model_writer as mw
    scene = scene or _scene()
    wing = load_wing(scene)
    idx, pal = image_ci4(wing)
    app.align(8)
    tex_at = app.put(mw.ci4_bytes(idx))
    app.align(8)
    pal_at = app.put(mw.rgba5551(pal))
    lists = {}
    for side in ("L", "R"):
        slot = {}
        for k in range(len(BONES)):
            base = (k % 2) * BANK
            for j, vi in enumerate(wing.bone_verts[k]):
                slot[vi] = base + j
            blob = b""
            for vi in wing.bone_verts[k]:
                vert = wing.verts[vi]
                x, y, z = vert.local
                blob += _vtx(-x if side == "R" else x, y, z, vert.st[0] * 32, vert.st[1] * 32)
            app.align(8)
            vtx_at = app.put(blob)
            for tri in wing.tris[k]:
                for vi in tri:
                    if wing.verts[vi].bone not in (k, k - 1):
                        raise WingError(f"bone {BONES[k]}: a corner on bone {wing.verts[vi].bone}")
            tris = [tuple(slot[vi] for vi in tri) for tri in wing.tris[k]]
            dl, slots = n.encode_dl(_bone_list(n, file_id, tex_at, pal_at, vtx_at,
                                               len(wing.bone_verts[k]), base, tris))
            app.align(8)
            at = app.put(dl)
            for rel, target in slots.items():
                app.slot(at + rel, target)
            lists[(side, BONES[k])] = at
    # The tree: rest values; her code sets every node each tic.
    torso_rest = np.array(scene["joints"][TORSO_ROW]["world"], dtype=float)
    rows = [(0, None, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0)),
            (1, None, tuple(torso_rest[:3, 3]), (0.0, 0.0, 0.0)),
            (2, None, tuple(wing.pair_origin), (0.0, 0.0, 0.0))]
    rest = bone_poses(wing, Params())
    for side in ("L", "R"):
        for k, (rot, trans, _scale) in enumerate(rest):
            if side == "R":
                rot, trans = MIRROR @ rot @ MIRROR, MIRROR @ trans
            rows.append((3, lists[(side, BONES[k])], tuple(trans), rpy(rot)))
    rows.append((n.JOINT_TREE_END, None, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0)))
    if len(rows) != TREE_ROWS:
        raise WingError(f"{len(rows)} tree rows, {TREE_ROWS} reserved")
    for k, (ident, dl, t, r) in enumerate(rows):
        at = tree_at + k * n.DOBJ_DESC_SIZE
        struct.pack_into(">I", app.f.data, at, ident)
        if dl is not None:
            app.slot(at + 4, ("intern", file_id, dl))
        struct.pack_into(">3f", app.f.data, at + 8, *t)
        struct.pack_into(">3f", app.f.data, at + 20, *r)
        struct.pack_into(">3f", app.f.data, at + 32, 1.0, 1.0, 1.0)
    # The table.
    baked = [bake(wing, load_recipe(name), torso_frames(name, scene)) for name in CLIPS]
    app.align(8)
    table_at = app.here
    head = 12 + 12 * len(CLIPS)
    body = b""
    clip_rows = []
    for first, last, rows_ in baked:
        clip_rows.append((first, last, head + len(body)))
        body += b"".join(_halfword_pairs(r) for r in rows_)
    blob = struct.pack(">III", TABLE_MAGIC, ROW_VALUES, len(CLIPS))
    blob += b"".join(struct.pack(">III", *c) for c in clip_rows)
    app.put(blob + body)
    return {"tree": tree_at, "table": table_at, "image": tex_at, "lists": lists,
            "clips": {name: (c[0], c[1]) for name, c in zip(CLIPS, clip_rows)},
            "bytes": app.here - tex_at,
            "mesh": {"verts": len(wing.verts), "tris": sum(len(t) for t in wing.tris.values())}}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--image", type=Path, help="write the wing's image (PNG, 4x)")
    args = ap.parse_args()
    wing = load_wing()
    print(f"pair {np.round(wing.pair_origin, 1)} normal {np.round(wing.normal, 3)}")
    for k, b in enumerate(wing.bones):
        print(b.name, "origin", np.round(b.origin, 1), "verts", len(wing.bone_verts[k]),
              "tris", len(wing.tris[k]))
    for name in CLIPS:
        first, last, rows = bake(wing, load_recipe(name), torso_frames(name))
        print(f"{name}: frames {first}-{last}, {len(rows)} rows of {ROW_VALUES}")
    if args.image:
        from PIL import Image
        Image.fromarray(image_rgba(wing), "RGBA").resize((TEX_W * 4, TEX_H * 4), Image.NEAREST).save(args.image)
        print(args.image)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
