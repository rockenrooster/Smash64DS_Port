#!/usr/bin/env python3
"""P6 A10: a look's shown-on-demand parts as model-part variants.

Effects that move with a fighter are joints of its tree shown by a script's
SetModelPartID (P6_PLAN A10). Here a part rides an existing joint: its
variant draws the joint's own list plus the part's cards (model_writer.py
writes the variant list, the joint's FTModelPartDesc and the reset's
variant 0), so the tree, the clips and the guard stay as they are. A card
is a textured cut-out quad, drawn two-sided, as the look's staff wings are.

The recipe (art/p6/<name>/look/parts.json, committed) gives each part's
joint and variant, its texture (a crop of a kit image with an optional
outline keeping only what lies below a polyline, or an authored image) and
its cards' corners in the model's rest space (a card's optional `st`: its
corners' texel coordinates, for an atlas). Palutena's victory-pose wings are
her smash wings' art (scripts/p6/p6_wings.py): the texture "authored:wings"
is their image and "wings_pose" (a wings recipe key) poses the rig's mesh at
her torso row's rest, its triangles in place of listed cards (a card of
three corners is one triangle). Parts naming one `texture_key`
share one texture (the writer stores a texture file once): the variants of
an animated part, shown in turn by its script. This adds the parts to the
look's scenes (`parts`) for model_writer.py.

    look_parts.py --recipe art/p6/palutena/look/parts.json \\
        --scene assets/p6-art/palutena/3ds2ds/scene_high/scene.json \\
        --scene assets/p6-art/palutena/3ds2ds/scene_low/scene.json [--preview out.png]

Kit-derived textures stay local (D10): the recipe is committed, the scenes
and files are build outputs.
"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
KIT = REPO / "decomp" / "alt_assets" / "Fighters" / "Super Smash Bros 3DS"


class PartsError(SystemExit):
    pass


def _quantize(rgba: np.ndarray, colors: int, cutout: bool):
    sys.path.insert(0, str(HERE))
    import look_blend
    return look_blend.quantize(rgba, colors, cutout)


def authored_halo(size: int) -> np.ndarray:
    """A gold ring on clear: the halo (card "Joints appended": authored)."""
    out = np.zeros((size, size, 4), dtype=np.float32)
    c = (size - 1) / 2.0
    for y in range(size):
        for x in range(size):
            r = math.hypot(x - c, y - c) / c
            if 0.70 <= r <= 0.96:
                # Bright along the ring's middle, darker to its edges.
                k = 1.0 - abs(r - 0.83) / 0.13
                out[y, x] = (200 + 55 * k, 160 + 70 * k, 40 + 60 * k, 255)
    return out


def authored_pillar(w: int, h: int) -> np.ndarray:
    """A column of light: white at its core to cyan at its sides, its top
    thinning out (card "Joints appended": the up smash's pillar, authored)."""
    out = np.zeros((h, w, 4), dtype=np.float32)
    c = (w - 1) / 2.0
    for y in range(h):
        top = min(1.0, y / (0.25 * h))          # 0 at the top row, 1 below a quarter
        for x in range(w):
            d = abs(x - c) / (c + 0.5)
            if d > 0.35 + 0.6 * top:
                continue
            k = max(0.0, 1.0 - d / 0.95)
            out[y, x] = (120 + 135 * k, 220 + 35 * k, 255, 255)
    return out


def authored_pillar_burst(w: int, h: int) -> np.ndarray:
    """The up smash's light burst (owner playtest 2026-10-14: "a strong light
    burst", several sprites), one atlas a quarter of the width apart:
    columns 0-1/4 the core (authored_pillar's white-to-cyan column), 1/4-3/4
    a wider glow with ray-like streaks rising from its foot, and in the last
    quarter a star flare (rows 0-1/4), a round bloom (1/4-1/2) and scattered
    sparks (the lower half)."""
    out = np.zeros((h, w, 4), dtype=np.float32)
    q = w // 4
    out[:, 0:q] = authored_pillar(q, h)
    # The glow: brightest at its middle, streaks of several heights at its
    # sides (a column of rays, not one flat band).
    gw = 2 * q
    c = (gw - 1) / 2.0
    streak = [0.15, 0.55, 0.25, 0.8, 0.35, 0.95, 0.5, 1.0, 1.0, 0.6, 0.9, 0.4, 0.75, 0.3, 0.6, 0.2]
    for x in range(gw):
        d = abs(x - c) / (c + 0.5)
        reach = 1.0 if d < 0.45 else streak[int(x * len(streak) / gw)]
        top = (1.0 - reach) * h
        for y in range(h):
            if y < top:
                continue
            k = max(0.0, 1.0 - d)
            fade = min(1.0, (y - top) / (0.15 * h + 1.0))
            out[y, q + x] = (90 + 150 * k * fade, 190 + 60 * k, 255, 255)
    # The flare: a white centre and eight spikes.
    s = q
    cx = (s - 1) / 2.0
    for y in range(s):
        for x in range(s):
            dx, dy = x - cx, y - cx
            r = math.hypot(dx, dy) / (cx + 0.5)
            a = math.atan2(dy, dx)
            spike = abs(math.cos(4.0 * a)) ** 6
            if r < 0.3 or (r < 1.0 and spike > r * 0.9):
                k = max(0.0, 1.0 - r)
                out[y, 3 * q + x] = (180 + 75 * k, 235 + 20 * k, 255, 255)
    # The bloom: a round glow, white at its middle.
    for y in range(s):
        for x in range(s):
            r = math.hypot(x - cx, y - cx) / (cx + 0.5)
            if r < 0.95:
                k = 1.0 - r
                out[q + y, 3 * q + x] = (110 + 145 * k, 200 + 55 * k, 255, 255)
    # The sparks: small crosses scattered down the lower half, a texel of
    # the shipped size (w / 64 here) an arm.
    unit = max(1, w // 64)
    rng = np.random.default_rng(64)
    for _ in range(14):
        x0 = int(rng.integers(1, s // unit - 1)) * unit
        y0 = int(rng.integers((2 * q) // unit + 1, h // unit - 1)) * unit
        for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
            colour = (255, 255, 255, 255) if (dx, dy) == (0, 0) else (170, 230, 255, 255)
            out[y0 + dy * unit:y0 + (dy + 1) * unit, 3 * q + x0 + dx * unit:3 * q + x0 + (dx + 1) * unit] = colour
    return out


# The up smash's light (owner p6j, 2026-10-14: "redo the blue pillar column
# VFX since we have a good animation to follow"): its palette, index 0 clear.
PILLAR_LIGHT_PALETTE = (
    (0, 0, 0, 0),
    (255, 255, 255, 255), (226, 250, 255, 255), (178, 238, 255, 255), (120, 214, 255, 255),
    (70, 208, 255, 255), (40, 178, 250, 255), (185, 255, 252, 255),
    (250, 250, 250, 255), (231, 232, 237, 255), (207, 210, 218, 255), (180, 184, 195, 255),
    (152, 157, 170, 255),
    (140, 255, 250, 255), (96, 242, 246, 255), (56, 212, 240, 255),
)


def pillar_light_atlas(core: list, size: int = 64) -> tuple[list, tuple]:
    """The up smash's light, as the 3DS's Heavenly Light (the owner's
    half-speed Strong_Up_A clip): a 64-texel CI4 atlas (rows of palette
    indices, PILLAR_LIGHT_PALETTE). Columns 0-31: the core, white down its
    middle to cyan at its edges, its top fading and narrowing (`core`, rows
    of 32 palette indices: p6_gate.py core_block, which draws it; the glow
    around it is p6_gate.py's own image); 32-63, rows 16-47: a bank of dust
    puffs rolling out from its foot along the floor (the foot at column 32),
    shaded from their tops; the rest clear."""
    idx = np.zeros((size, size), dtype=int)
    q = size // 4
    idx[:, 0:2 * q] = np.array(core, dtype=int)
    # The dust: puffs along the floor, the biggest at the foot (column 32),
    # each lit from above (paler at its top, darker at its foot).
    top, depth, fw = q, 2 * q, size - 2 * q
    puffs = ((5.0, 21.0, 10.0), (14.0, 19.0, 11.5), (24.0, 22.0, 8.5), (29.0, 25.0, 5.5),
             (9.0, 13.0, 7.0), (19.0, 12.5, 6.0))
    for y in range(depth):
        for x in range(fw):
            best = None
            for px, py, pr in puffs:
                r = math.hypot(x - px, y - py) / pr
                if r <= 1.0 and (best is None or r < best[0]):
                    best = (r, (y - (py - pr)) / (2.0 * pr))
            if best is None:
                continue
            r, down = best
            shade = down + 0.35 * r
            for limit, k in ((0.35, 8), (0.6, 9), (0.85, 10), (1.1, 11), (9.0, 12)):
                if shade <= limit:
                    idx[top + y, 2 * q + x] = k
                    break
    return idx.tolist(), PILLAR_LIGHT_PALETTE


def texture(spec: dict) -> np.ndarray:
    """The part's texels at its size (RGBA, row 0 the top)."""
    from PIL import Image, ImageDraw
    w, h = spec["size"]
    src = spec["source"]
    if src == "authored:halo":
        img = Image.fromarray(authored_halo(max(w, h) * 4).astype(np.uint8), "RGBA")
    elif src == "authored:pillar":
        img = Image.fromarray(authored_pillar(w * 4, h * 4).astype(np.uint8), "RGBA")
    elif src == "authored:pillar_burst":
        img = Image.fromarray(authored_pillar_burst(w * 4, h * 4).astype(np.uint8), "RGBA")
    elif src == "authored:wings":
        sys.path.insert(0, str(HERE))
        import p6_wings
        img = Image.fromarray(p6_wings.image_rgba(p6_wings.load_wing()), "RGBA")
    elif src.startswith("kit:"):
        img = Image.open(KIT / src[4:]).convert("RGBA")
    else:
        raise PartsError(f"texture source {src!r}")
    if "keep_below" in spec:
        # Clear everything above the polyline (image pixels): another UV
        # island shares the crop's top.
        mask = Image.new("L", img.size, 255)
        poly = [tuple(p) for p in spec["keep_below"]]
        ImageDraw.Draw(mask).polygon([(0, 0)] + poly + [(img.size[0], 0)], fill=0)
        arr = np.array(img)
        arr[..., 3] = np.minimum(arr[..., 3], np.array(mask))
        img = Image.fromarray(arr, "RGBA")
    for box in spec.get("clear", ()):
        # Other islands' pixels inside the crop (image pixels: x0, y0, x1, y1).
        arr = np.array(img)
        arr[box[1]:box[3], box[0]:box[2], 3] = 0
        img = Image.fromarray(arr, "RGBA")
    if "crop" in spec:
        img = img.crop(tuple(spec["crop"]))
    img = img.resize((w, h), Image.BOX)
    return np.asarray(img)


def card_geometry(part: dict, joint_world: np.ndarray, base_vertex: int):
    """The part's vertices (joint-local, as the scene's) and triangles."""
    inv = np.linalg.inv(joint_world)
    w, h = part["texture"]["size"]
    verts, tris = [], []
    shared = {}
    for quad in part["cards"]:
        corners = quad["corners"]
        st = quad.get("st", [[0, 0], [w, 0], [w, h], [0, h]])
        # Lit by an up normal, not the card's own: a two-sided card's face
        # normal points away from the light for one facing (the pillar drew
        # black facing right), and up takes the same light either way.
        n = np.array(part.get("normal", (0.0, 1.0, 0.0)), dtype=float)
        n = n / (np.linalg.norm(n) or 1.0)
        if len(corners) == 3:
            # A mesh's triangle: its corners shared with its neighbours'.
            ids = []
            for p, (s, t) in zip(corners, quad["st"]):
                key = (tuple(round(float(x), 2) for x in p), int(round(s * 32)), int(round(t * 32)))
                if key not in shared:
                    local = inv @ np.array([*p, 1.0])
                    shared[key] = base_vertex + len(verts)
                    verts.append({"joint": part["joint"], "local": [round(float(x), 4) for x in local[:3]],
                                  "model": [float(x) for x in p], "st": [key[1], key[2]],
                                  "flag": 0, "normal": [int(round(x * 127)) for x in n],
                                  "rgba": [255, 255, 255, 255], "object": "part:" + part["name"]})
                ids.append(shared[key])
            tris.append(ids)
            continue
        first = base_vertex + len(verts)
        for p, (s, t) in zip(corners, st):
            local = inv @ np.array([*p, 1.0])
            verts.append({"joint": part["joint"], "local": [round(float(x), 4) for x in local[:3]],
                          "model": [float(x) for x in p], "st": [int(round(s * 32)), int(round(t * 32))],
                          "flag": 0, "normal": [int(round(x * 127)) for x in n], "rgba": [255, 255, 255, 255],
                          "object": "part:" + part["name"]})
        tris.append([first, first + 1, first + 2])
        tris.append([first, first + 2, first + 3])
    return verts, tris


def wing_cards(part: dict, scene: dict) -> list:
    """The smash wings' mesh (p6_wings.py) at the part's "wings_pose",
    attached to her torso row at its rest: three-corner cards in the model's
    rest space, `st` in the image's texels."""
    sys.path.insert(0, str(HERE))
    import p6_wings
    wing = p6_wings.load_wing(scene)
    torso = np.array(scene["joints"][p6_wings.TORSO_ROW]["world"], dtype=float)
    pose = p6_wings.params_of(part["wings_pose"])
    return [{"corners": corners.tolist(), "st": st.tolist()}
            for _side, corners, st in p6_wings.world_mesh(wing, torso, pose)]


def apply(recipe: dict, scene: dict) -> dict:
    parts = []
    for part in recipe["parts"]:
        if "wings_pose" in part:
            part = dict(part, cards=wing_cards(part, scene))
        tex = texture(part["texture"])
        idx, pal = _quantize(tex, 16, True)
        w, h = part["texture"]["size"]
        world = np.array(scene["joints"][part["joint"]]["world"], dtype=float)
        verts, tris = card_geometry(part, world, 0)
        parts.append({
            "name": part["name"], "joint": part["joint"], "variant": part["variant"],
            "material": {"combine": [0, 0], "lit": True, "prim": "ffffffff", "spans": {}, "two_sided": True,
                         "texture": {"file": "part_" + part.get("texture_key", part["name"]),
                                     "width": w, "height": h, "texels": idx,
                                     "palette": pal, "alpha": True, "wrap": ["clamp", "clamp"]}},
            "vertices": verts, "triangles": tris})
    scene["parts"] = parts
    return scene


def preview(scene: dict, out: Path) -> None:
    """The parts' textures side by side (2x), on grey."""
    from PIL import Image
    tiles = []
    for p in scene["parts"]:
        t = p["material"]["texture"]
        pal = np.array(t["palette"], dtype=np.uint8)
        img = Image.fromarray(pal[np.array(t["texels"])], "RGBA")
        tiles.append(img.resize((img.width * 4, img.height * 4), Image.NEAREST))
    W = sum(t.width + 8 for t in tiles)
    H = max(t.height for t in tiles)
    sheet = Image.new("RGBA", (W, H), (80, 80, 80, 255))
    x = 0
    for t in tiles:
        sheet.paste(t, (x, 0), t)
        x += t.width + 8
    sheet.save(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--recipe", type=Path, required=True)
    ap.add_argument("--scene", type=Path, action="append", required=True)
    ap.add_argument("--preview", type=Path)
    args = ap.parse_args()
    recipe = json.loads(args.recipe.read_text(encoding="utf-8"))
    for path in args.scene:
        scene = apply(recipe, json.loads(path.read_text(encoding="utf-8")))
        path.write_bytes((json.dumps(scene, indent=0) + "\n").encode("utf-8"))
        print(f"look parts: {', '.join(p['name'] for p in scene['parts'])} -> {path}")
    if args.preview:
        preview(scene, args.preview)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
