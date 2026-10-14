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
its cards' corners in the model's rest space. This adds the parts to the
look's scenes (`parts`) for model_writer.py.

    look_parts.py --recipe art/p6/palutena/look/parts.json \\
        --scene builds/p6-art/palutena/3ds2ds/scene_high/scene.json \\
        --scene builds/p6-art/palutena/3ds2ds/scene_low/scene.json [--preview out.png]

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


def texture(spec: dict) -> np.ndarray:
    """The part's texels at its size (RGBA, row 0 the top)."""
    from PIL import Image, ImageDraw
    w, h = spec["size"]
    src = spec["source"]
    if src == "authored:halo":
        img = Image.fromarray(authored_halo(max(w, h) * 4).astype(np.uint8), "RGBA")
    elif src == "authored:pillar":
        img = Image.fromarray(authored_pillar(w * 4, h * 4).astype(np.uint8), "RGBA")
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
    for quad in part["cards"]:
        corners = quad["corners"]
        st = quad.get("st", [[0, 0], [w, 0], [w, h], [0, h]])
        # Lit by an up normal, not the card's own: a two-sided card's face
        # normal points away from the light for one facing (the pillar drew
        # black facing right), and up takes the same light either way.
        n = np.array(part.get("normal", (0.0, 1.0, 0.0)), dtype=float)
        n = n / (np.linalg.norm(n) or 1.0)
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


def apply(recipe: dict, scene: dict) -> dict:
    parts = []
    for part in recipe["parts"]:
        tex = texture(part["texture"])
        idx, pal = _quantize(tex, 16, True)
        w, h = part["texture"]["size"]
        world = np.array(scene["joints"][part["joint"]]["world"], dtype=float)
        verts, tris = card_geometry(part, world, 0)
        parts.append({
            "name": part["name"], "joint": part["joint"], "variant": part["variant"],
            "material": {"combine": [0, 0], "lit": True, "prim": "ffffffff", "spans": {}, "two_sided": True,
                         "texture": {"file": "part_" + part["name"], "width": w, "height": h, "texels": idx,
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
