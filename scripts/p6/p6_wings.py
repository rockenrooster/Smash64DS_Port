#!/usr/bin/env python3
"""P6 Palutena's wings in her forward and down smash, as a rigged effect
(owner playtest p6i follow-up, 2026-10-14: "the wings need lots of work, they
should articulate and stretch and be mostly symmetrical ... polished and like
they belong in the DS version").

The torso-row card variants of the first pass (art/p6/palutena/look/parts.json
"wings_forward", "wings_down") were one rigid card a wing in three poses. The
wings are now an effect of their own in her special file 2, drawn natively as
her pillar is (scripts/p4/p4_articles.py "wings"), with a skeleton: the kit's
godwing (godwing.dae: a wing a side, four bones each, LwingNa-d; the right
wing the left's mirror) at the look's fit_scale.

Art. Each bone draws one card: the kit wing's feathers of that bone rendered
flat in the wing's plane from its outer side (an impostor), cut out of one
64x64 CI4 atlas (colour 0 clear). The cards of a wing lie a little apart along
the plane's normal, the inner ones in front, as the kit's feathers overlap;
the feathers' edge texels are darkened a little (RIM_SHADE) so the white
wings keep their shape over a bright stage.

Tree (DObjDesc depth in brackets):

    [0] root    her TopN's transform, copied every tic
    [1] torso   her torso row (2) relative to TopN, from her smash clip
    [2] pair    the wings' root between her shoulder blades (the kit's
                WingN, on BustN); its scale grows and shrinks the wings
    [3] La      bone a of the left wing, [4] its card, [4] bone b ...
    [3] Ra      the right wing, the left's mirror

Motion. art/p6/palutena/wings/<smash>.json keys each wing's pose on her
smash clip's frames (the owner's 3DS reference beside it): the pair's size
("size"), its "level" (0..1 of the way from her torso's turn to her TopN's
axes: 1 keeps the wings in the battle camera's plane however her chest
twists), "pair_yaw" (a quarter turn lays the wings across the screen for a
body seen side on), "pair_roll" (one wing a little above the other) and
"pair_pitch"; bone a's turn in the pair's frame ("a": pitch, forward and
back; yaw, out and in; roll, up and down) and "twist" (the whole wing about
its own length: a wing laid flat reads as a thin arc from the side, the
3DS down smash's), bones b-d's turns in their own
bind frames ("b".."d": twist, curl out of the wing's plane, fold in it), each
bone's "stretch" (its card's length and its child's distance) and "fan" (its
card's feather length). Keys pass through a Catmull-Rom spline, stopping at
the ends and at "hold" keys. The bake samples every frame of the
clip in the visible span: the torso node from the clip's own pose (anim.py,
keyposes.py, the look scene's skeleton), then every node's translation, its
rotation as BattleShip's RPY Euler angles (syMatrixRotRpyRF: R = Rz Ry Rx)
and its scale, into a table her code reads every tic
(src/port/nds_p4_palutena.c) at her animation frame (fighter anim_frame), so
the wings stay on her through hitlag and leave with the status.

Kit-derived: the atlas, cards and table live in builds/ (D10); this module
and the motion recipes are the committed part. p6_gate.py writes the tree
into her special file 2 (write); world_cards gives a pose's cards in model
space for review renders.

    p6_wings.py [--atlas atlas.png]     (the rig, the cards and the bake's spans)
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
ATLAS = 64                    # the cards' CI4 atlas side (2 KB, TMEM's half)
CARD_GAP = 3.0                # units between a wing's cards along its normal
RENDER_DENSITY = 2.0          # pixels a unit for the high-resolution segments
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
    card: tuple               # (u0, u1, v0, v1) in the bone frame
    depth: float              # the card's offset along the normal
    image: np.ndarray         # RGBA, high resolution, rows from v1 down to v0
    atlas: tuple = None       # (x, y, w, h) texels in the atlas


@dataclass
class Wing:
    pair_origin: np.ndarray   # the pair node in her torso row's frame
    bones: list               # Bone a..d (left wing)
    normal: np.ndarray
    fit: float
    atlas_rgba: np.ndarray = None
    density: float = 0.0      # atlas texels a unit


def _scene() -> dict:
    return json.loads(SCENE.read_text(encoding="utf-8"))


def _dominant(mesh, tri, names) -> str:
    acc = collections.Counter()
    for i in tri:
        for j, w in mesh.weights[i]:
            acc[j] += w
    return names[acc.most_common(1)[0][0]]


def _render_segment(tris: np.ndarray, uvs: np.ndarray, tex: np.ndarray, card: tuple,
                    density: float) -> np.ndarray:
    """The segment's triangles (bone-frame u, v, depth) textured into an
    image covering `card`, seen from +depth; rows run from v1 down to v0."""
    u0, u1, v0, v1 = card
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


def load_wing(scene: dict | None = None) -> Wing:
    """The kit's left wing as four bones with flat cards (module docstring)."""
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
    segments = {b: [] for b in BONES}
    for tri in mesh.triangles:
        d = _dominant(mesh, tri, names)
        if d.startswith("Lwing"):
            segments[d[-1]].append(tri)
    pos = {i: np.array(model.skinned_position(mesh, i)) * fit - pair_world
           for b in BONES for tri in segments[b] for i in tri}
    pts = np.array(list(pos.values()))
    centred = pts - pts.mean(0)
    _u, _s, vt = np.linalg.svd(centred, full_matrices=False)
    normal = vt[2] / np.linalg.norm(vt[2])
    if normal[0] < 0:
        normal = -normal           # the left wing's outer side: her left
    origins = {b: world[f"LwingN{b}"] - pair_world for b in BONES}
    # The tip: the farthest point of bone d's feathers from its joint.
    d_pts = np.array([pos[i] for tri in segments["d"] for i in tri])
    tip = d_pts[np.argmax(np.linalg.norm(d_pts - origins["d"], axis=1))]
    tex = _kit_texture()
    bones = []
    for k, b in enumerate(BONES):
        nxt = origins[BONES[k + 1]] if k + 1 < len(BONES) else tip
        x = nxt - origins[b]
        x = x - normal * float(np.dot(x, normal))
        x /= np.linalg.norm(x)
        y = np.cross(normal, x)
        frame = np.stack([x, y, normal], axis=1)
        tris = np.array([[(pos[i] - origins[b]) @ frame for i in tri] for tri in segments[b]])
        uvs = np.array([[mesh.pool.uvs[i] for i in tri] for tri in segments[b]], dtype=float)
        flat = tris.reshape(-1, 3)
        card = (float(flat[:, 0].min()) - 2.0, float(flat[:, 0].max()) + 2.0,
                float(flat[:, 1].min()) - 2.0, float(flat[:, 1].max()) + 2.0)
        image = _render_segment(tris, uvs, tex, card, RENDER_DENSITY)
        bones.append(Bone(b, origins[b], frame, card, CARD_GAP * (len(BONES) - 1 - k), image))
    wing = Wing(pair, bones, normal, fit)
    pack_atlas(wing)
    return wing


def _kit_texture() -> np.ndarray:
    from PIL import Image
    return np.asarray(Image.open(KIT / "Yoshi_Wing.png").convert("RGBA"))


# ---------------------------------------------------------------- the atlas

def _shelf(sizes: list, side: int):
    """Shelf-pack (w, h) rectangles, tallest first, into side x side; the
    places, or None."""
    order = sorted(range(len(sizes)), key=lambda i: -sizes[i][1])
    places = [None] * len(sizes)
    x = y = shelf = 0
    for i in order:
        w, h = sizes[i]
        if w > side:
            return None
        if x + w > side:
            x, y, shelf = 0, y + shelf, 0
        if y + h > side:
            return None
        places[i] = (x, y)
        x += w
        shelf = max(shelf, h)
    return places


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


def pack_atlas(wing: Wing) -> None:
    """The densest texels-a-unit at which the four cards fit the atlas (a
    texel of margin each), and each card's place."""
    lo, hi = 0.01, 1.0
    best = None
    for _ in range(40):
        mid = (lo + hi) / 2
        sizes = [(int(math.ceil((b.card[1] - b.card[0]) * mid)) + 1,
                  int(math.ceil((b.card[3] - b.card[2]) * mid)) + 1) for b in wing.bones]
        places = _shelf(sizes, ATLAS)
        if places is None:
            hi = mid
        else:
            lo, best = mid, (mid, sizes, places)
    if best is None:
        raise WingError("wing cards do not fit the atlas")
    density, sizes, places = best
    wing.density = density
    from PIL import Image
    atlas = np.zeros((ATLAS, ATLAS, 4), dtype=float)
    for b, (w, h), (x, y) in zip(wing.bones, sizes, places):
        w0, h0 = w - 1, h - 1
        a, rgb = _box_premultiplied(b.image, w0, h0)
        atlas[y:y + h0, x:x + w0, :3] = rgb
        atlas[y:y + h0, x:x + w0, 3] = a
        b.atlas = (x, y, w0, h0)
    wing.atlas_rgba = atlas


# The feathers' rim: each opaque texel beside a clear one darkened a little
# and cooled, so the white wings keep their shape over a bright stage.
RIM_SHADE = 0.86
RIM_TINT = (0.92, 0.95, 1.0)


def atlas_ci4(wing: Wing) -> tuple[list, list]:
    """The atlas as CI4 indices and an RGBA palette (index 0 clear)."""
    import look_blend
    rgba = wing.atlas_rgba.copy()
    opaque = rgba[..., 3] >= 110
    rgba[..., 3] = np.where(opaque, 255, 0)
    edge = np.zeros_like(opaque)
    for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0)):
        edge |= opaque & ~np.roll(np.roll(opaque, dy, 0), dx, 1)
    rgba[edge, :3] = rgba[edge, :3] * RIM_SHADE * np.array(RIM_TINT)
    return look_blend.quantize(rgba.astype(np.uint8), 16, True)


def card_corners(bone: Bone, density: float) -> list:
    """The card's four corners: (x, y, z) in the bone frame and (s, t) in
    texels, counter-clockwise from the outer side."""
    u0, u1, v0, v1 = bone.card
    x, y, w, h = bone.atlas
    return [((u0, v1, bone.depth), (x, y)),
            ((u0, v0, bone.depth), (x, y + h)),
            ((u1, v0, bone.depth), (x + w, y + h)),
            ((u1, v1, bone.depth), (x + w, y))]


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


def rpy_matrix(x: float, y: float, z: float) -> np.ndarray:
    return rot_z(math.degrees(z)) @ rot_y(math.degrees(y)) @ rot_x(math.degrees(x))


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
    """A wing pose (module docstring): degrees, scales. `level` 0..1 turns
    the pair from her torso's turn toward her TopN's axes (1: the wings keep
    the battle camera's plane however her chest twists)."""
    size: float = 1.0
    pair_pitch: float = 0.0
    level: float = 0.0
    pair_yaw: float = 0.0               # the pair turned about her vertical
    pair_roll: float = 0.0              # one wing up, the other down
    twist: float = 0.0                  # the whole wing about its own length
    a: tuple = (0.0, 0.0, 0.0)           # pitch, yaw, roll in the pair's frame
    bends: dict = field(default_factory=lambda: {b: (0.0, 0.0, 0.0) for b in BONES[1:]})
    stretch: dict = field(default_factory=lambda: {b: 1.0 for b in BONES})
    fan: dict = field(default_factory=lambda: {b: 1.0 for b in BONES})


def bone_locals(wing: Wing, p: Params) -> list[tuple[np.ndarray, np.ndarray]]:
    """Each bone's local (rotation, translation) under its parent (the pair
    node for bone a) for the pose."""
    out = []
    parent_frame = np.eye(3)
    parent_origin = np.zeros(3)
    parent_stretch = 1.0
    for k, bone in enumerate(wing.bones):
        rot = parent_frame.T @ bone.frame
        trans = parent_frame.T @ (bone.origin - parent_origin)
        if k == 0:
            # The wing turns about its own root (the kit's LwingNa), the
            # turn's axes the pair's.
            pitch, yaw, roll = p.a
            rot = rot_z(roll) @ rot_y(yaw) @ rot_x(pitch) @ rot @ rot_x(p.twist)
        else:
            twist, curl, fold = p.bends[bone.name]
            rot = rot @ rot_x(twist) @ rot_y(curl) @ rot_z(fold)
            trans = trans * parent_stretch
        out.append((rot, trans))
        parent_frame, parent_origin = bone.frame, bone.origin
        parent_stretch = p.stretch[bone.name]
    return out


def pair_local(wing: Wing, p: Params, torso: np.ndarray) -> tuple[np.ndarray, np.ndarray, float]:
    """The pair node's local rotation, translation and scale under the torso
    node: `level` of the way to undoing the torso's turn, then its pitch."""
    tr = torso[:3, :3] / np.linalg.norm(torso[:3, :3], axis=0)
    undo = _exp(_log(tr.T) * p.level)
    return (undo @ rot_y(p.pair_yaw) @ rot_z(p.pair_roll) @ rot_x(p.pair_pitch),
            wing.pair_origin.copy(), p.size)


def world_cards(wing: Wing, torso: np.ndarray, p: Params) -> list:
    """Model-space (TopN-relative) card corners for both wings: [(side, bone,
    corners 4x3, st 4x2)]; torso is her torso row's 4x4 model matrix."""
    pr, pt, ps = pair_local(wing, p, torso)
    pair = torso @ _affine(pr, pt, ps)
    out = []
    mirror = np.diag([-1.0, 1.0, 1.0, 1.0])
    for side in ("L", "R"):
        m = pair
        for bone, (rot, trans) in zip(wing.bones, bone_locals(wing, p)):
            local = _affine(rot, trans, 1.0)
            if side == "R":
                local = mirror @ local @ mirror
            m = m @ local
            card = m @ np.diag([p.stretch[bone.name], p.fan[bone.name], 1.0, 1.0])
            corners = []
            st = []
            for (x, y, z), (s, t) in card_corners(bone, wing.density):
                v = np.array([-x if side == "R" else x, y, z, 1.0])
                corners.append((card @ v)[:3])
                st.append((s, t))
            out.append((side, bone.name, np.array(corners), np.array(st, dtype=float)))
    return out


def _affine(rot: np.ndarray, trans: np.ndarray, scale: float) -> np.ndarray:
    m = np.eye(4)
    m[:3, :3] = rot * scale
    m[:3, 3] = trans
    return m


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

NODES_PER_SIDE = 2 * len(BONES)
ROW_VALUES = 6 + 4 + 6 * len(BONES) + 2 * len(BONES)


def _s16(v: float) -> int:
    i = int(round(v))
    if not -0x8000 <= i <= 0x7FFF:
        raise WingError(f"table value {v} overflows s16")
    return i


def bake_row(wing: Wing, torso: np.ndarray, p: Params) -> list[int]:
    """One frame's table row (src/port/nds_p4_palutena.c reads it): the
    torso node's translation and rotation; the pair's rotation and size; per
    left bone its translation and rotation; per card its x and y scale."""
    row = []
    tr = torso[:3, :3]
    sc = np.linalg.norm(tr, axis=0)
    rot = tr / sc
    row += [_s16(v) for v in torso[:3, 3]]
    row += [_s16(a * ANGLE_UNIT) for a in rpy(rot)]
    pr, _pt, ps = pair_local(wing, p, torso)
    row += [_s16(a * ANGLE_UNIT) for a in rpy(pr)] + [_s16(ps * SCALE_UNIT)]
    for rot_b, trans_b in bone_locals(wing, p):
        row += [_s16(v) for v in trans_b] + [_s16(a * ANGLE_UNIT) for a in rpy(rot_b)]
    for b in BONES:
        row += [_s16(p.stretch[b] * SCALE_UNIT), _s16(p.fan[b] * SCALE_UNIT)]
    if len(row) != ROW_VALUES:
        raise WingError(f"row of {len(row)} values, the reader takes {ROW_VALUES}")
    return row


def bake(wing: Wing, recipe: dict, torsos: list[np.ndarray]) -> tuple[int, int, list]:
    """(first frame, last frame, rows) of the frames the wings show."""
    keys = sorted(k["frame"] for k in recipe["keys"])
    first, last = int(math.ceil(keys[0])), min(int(math.floor(keys[-1])), len(torsos) - 1)
    rows = []
    for fr in range(first, last + 1):
        p = evaluate(recipe, fr)
        rows.append(bake_row(wing, torsos[fr], p))
    return first, last, rows


# ---------------------------------------------------------------- the file

# The smash clips the table holds, in its order (src/port/nds_p4_palutena.c
# PAL_WINGS_CLIP_*): her forward smash (AttackS4 and its angled four) and
# down smash (AttackLw4).
CLIPS = ("fsmash", "dsmash")
TABLE_MAGIC = 0x50574E47      # "PWNG"
TREE_ROWS = 3 + 2 * NODES_PER_SIDE + 1       # root, torso, pair, the wings, the end
G_LIGHTING = 0x00020000
G_CULL_BACK = 0x00000400
# The model writer's cut-out state (as p6_gate.py's pillar cards).
CUTOUT_ON = ((0xE2001E01, 0x00000001), (0xE200001C, 0xC4113078))
CUTOUT_OFF = ((0xE2001E01, 0x00000000), (0xE200001C, 0xC4112078))


def _vtx(x: float, y: float, z: float, s: float, t: float) -> bytes:
    xs, ys, zs = (int(round(c)) for c in (x, y, z))
    for c in (xs, ys, zs):
        if not -0x8000 <= c <= 0x7FFF:
            raise WingError(f"card corner ({x}, {y}, {z}) overflows s16")
    return struct.pack(">hhhHhh", xs, ys, zs, 0, int(round(s)), int(round(t))) + bytes((255, 255, 255, 255))


def _card_list(n, file_id: int, tex_at: int, pal_at: int, vtx_at: int) -> list:
    """A card's list: the atlas and its palette loaded as the pillar's cards
    load theirs (p6_gate.py), one quad, unlit, two-sided, cut out."""
    line = ATLAS * 4 // 64
    mask = ATLAS.bit_length() - 1
    clamp = 2
    cmds = [n.Gfx(0xE7000000, 0),
            n.Gfx(0xD9000000 | (0xFFFFFF & ~(G_LIGHTING | G_CULL_BACK)), 0),
            n.Gfx(0xE3001001, 0x00008000)]
    cmds += [n.Gfx(w0, w1) for w0, w1 in CUTOUT_ON]
    cmds += [n.Gfx(0xFC127E05, 0xFF17F3FF), n.Gfx(0xF9000000, 0),
             n.Gfx(0xE7000000, 0), n.Gfx(0xFA000000, 0xFFFFFFFF), n.Gfx(0xE8000000, 0),
             n.Gfx(0xF5000100, 0x05000000),
             n.Gfx(0xF5500000, (7 << 24) | (mask << 14) | (mask << 4)),
             n.Gfx(0xF5400000 | (line << 9), (clamp << 18) | (mask << 14) | (clamp << 8) | (mask << 4)),
             n.Gfx(0xFD100000, pal_at // 4, ("intern", file_id, pal_at)),
             n.Gfx(0xE6000000, 0), n.Gfx(0xF0000000, 0x0503C000), n.Gfx(0xE7000000, 0),
             n.Gfx(0xD7000002, 0xFFFFFFFF),
             n.Gfx(0xF2000000, (((ATLAS - 1) << 2) << 12) | ((ATLAS - 1) << 2)),
             n.Gfx(0xFD500000, tex_at // 4, ("intern", file_id, tex_at)),
             n.Gfx(0xE6000000, 0),
             n.Gfx(0xF3000000, (7 << 24) | ((ATLAS * ATLAS // 4 - 1) << 12) | (2048 // line)),
             n.Gfx(0xE7000000, 0),
             n.Gfx(0x01000000 | (4 << 12) | (4 << 1), vtx_at, ("intern", file_id, vtx_at)),
             n.Gfx(0x06000000 | (0 << 16) | (2 << 8) | 4, (0 << 16) | (4 << 8) | 6),
             n.Gfx(0xE7000000, 0)]
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
    tree's TREE_ROWS rows reserved at `tree_at`): the atlas and palette,
    each card's vertices and list (the right wing's mirrored), the tree,
    then the table. Returns the offsets."""
    import n64_model as n
    scene = scene or _scene()
    wing = load_wing(scene)
    idx, pal = atlas_ci4(wing)
    import model_writer as mw
    app.align(8)
    tex_at = app.put(mw.ci4_bytes(idx))
    app.align(8)
    pal_at = app.put(mw.rgba5551(pal))
    lists = {}
    for side in ("L", "R"):
        for bone in wing.bones:
            corners = card_corners(bone, wing.density)
            blob = b"".join(_vtx(-x if side == "R" else x, y, z, s * 32, t * 32)
                            for (x, y, z), (s, t) in corners)
            app.align(8)
            vtx_at = app.put(blob)
            dl, slots = n.encode_dl(_card_list(n, file_id, tex_at, pal_at, vtx_at))
            app.align(8)
            at = app.put(dl)
            for rel, target in slots.items():
                app.slot(at + rel, target)
            lists[(side, bone.name)] = at
    # The tree: rest values; her code sets every node each tic.
    rest = Params()
    torso_rest = np.array(scene["joints"][TORSO_ROW]["world"], dtype=float)
    rows = [(0, None, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0)),
            (1, None, tuple(torso_rest[:3, 3]), (0.0, 0.0, 0.0)),
            (2, None, tuple(wing.pair_origin), (0.0, 0.0, 0.0))]
    for side in ("L", "R"):
        for k, (bone, (rot, trans)) in enumerate(zip(wing.bones, bone_locals(wing, rest))):
            x, y, z = rpy(rot)
            if side == "R":
                trans = trans * np.array([-1.0, 1.0, 1.0])
                y, z = -y, -z
            rows.append((3 + k, None, tuple(trans), (x, y, z)))
            rows.append((4 + k, lists[(side, bone.name)], (0.0, 0.0, 0.0), (0.0, 0.0, 0.0)))
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
    baked = []
    for name in CLIPS:
        baked.append(bake(wing, load_recipe(name), torso_frames(name, scene)))
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
    return {"tree": tree_at, "table": table_at, "atlas": tex_at, "lists": lists,
            "clips": {name: (c[0], c[1]) for name, c in zip(CLIPS, clip_rows)},
            "bytes": app.here - tex_at}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--atlas", type=Path, help="write the atlas preview (PNG)")
    args = ap.parse_args()
    wing = load_wing()
    print(f"pair {np.round(wing.pair_origin, 1)} normal {np.round(wing.normal, 3)} "
          f"density {wing.density:.3f} texels/unit")
    for b in wing.bones:
        print(b.name, "origin", np.round(b.origin, 1), "card", [round(v, 1) for v in b.card],
              "atlas", b.atlas)
    for name in CLIPS:
        first, last, rows = bake(wing, load_recipe(name), torso_frames(name))
        print(f"{name}: frames {first}-{last}, {len(rows)} rows of {ROW_VALUES}")
    if args.atlas:
        from PIL import Image
        idx, pal = atlas_ci4(wing)
        img = np.array([[pal[i] for i in row] for row in idx], dtype=np.uint8)
        Image.fromarray(img, "RGBA").resize((ATLAS * 6, ATLAS * 6), Image.NEAREST).save(args.atlas)
        print(args.atlas)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
