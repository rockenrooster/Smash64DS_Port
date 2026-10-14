#!/usr/bin/env python3
"""P6 costumes: one palette per costume over a look's CI4 textures.

The kit's costumes are recolours on shared UVs (Alternates/c01-c07, the low
textures the look was baked from), so a costume of the look is a palette
over its one index image (P6_PLAN A3b "Costumes"): blender/costume_corr.py
records where each texel of the look's textures sampled the kit; here each
costume's image is sampled at the same places, and each palette entry moves
by the mean change of the texels that use it:

    palette_k[i] = palette_0[i] + mean over texels t with index i of
                   (costume_k(uv_t) - costume_0(uv_t))

so the look's own baked colours (and any retouch) stay where a costume does
not recolour. A material whose palettes all equal its own (the face, the
hair) keeps one palette; the others get `costume_palettes` in the scene,
which model_writer.py turns into MObj palettes (one per costume, picked by
the costume's palette animation as the base's materials are).

    p6_costumes.py --name palutena --corr builds/p6-art/palutena/costumes/corr.npz \\
        --scene builds/p6-art/palutena/3ds2ds/scene_high/scene.json \\
        --scene builds/p6-art/palutena/3ds2ds/scene_low/scene.json [--sheet sheet.png]

Kit-derived: local outputs (D10); the costume picks below are the recipe.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
KIT = REPO / "decomp" / "alt_assets" / "Fighters" / "Super Smash Bros 3DS"
# Each costume's kit alternate (c00 the default), in the base's costume
# order: VS uses 0-3 on the C buttons and 2, 3, 4 for the red, blue and
# green teams (Peach's default_costume rows), so those are red, blue and
# green. The rest fill the base's palette count.
COSTUMES = {
    "palutena": {"kit": "Palutena",
                 "picks": ["c00", "c06", "c05", "c04", "c03", "c07", "c01", "c02", "c00"],
                 "names": ["white", "black", "red", "blue", "green", "purple", "pink", "sky", "white"]},
}
# DS object -> its texture's file key in the scenes (model_writer's names).
OBJECTS = {"DS_face": "DS_face", "DS_hair_cap": "DS_hair_cap", "DS_hair_panels": "DS_hair_panels",
           "DS_chest": "DS_chest", "DS_dress": "DS_dress", "DS_arms": "DS_arms", "DS_hands": "DS_hands",
           "DS_legs": "DS_legs", "DS_staff": "DS_staff"}
# A palette entry that moves less than this (0-255 per channel) stays.
STILL = 8.0


class CostumeError(SystemExit):
    pass


def _image(path: Path) -> np.ndarray:
    from PIL import Image
    return np.asarray(Image.open(path).convert("RGBA"), dtype=np.float32)


def _mir(t):
    t = np.mod(t, 2.0)
    return np.where(t > 1.0, 2.0 - t, t)


def sample(img: np.ndarray, u: np.ndarray, v: np.ndarray) -> np.ndarray:
    """Bilinear samples with mirrored repeat at Blender UVs (v up), as the
    bake sampled (blender/costume_corr.py, bake_tools.sample)."""
    h, w = img.shape[:2]
    x = _mir(u) * w - 0.5
    y = (1.0 - _mir(v)) * h - 0.5
    x0 = np.floor(x).astype(int)
    y0 = np.floor(y).astype(int)
    fx, fy = (x - x0)[..., None], (y - y0)[..., None]

    def g(xx, yy):
        return img[np.clip(yy, 0, h - 1), np.clip(xx, 0, w - 1)]
    return (g(x0, y0) * (1 - fx) * (1 - fy) + g(x0 + 1, y0) * fx * (1 - fy)
            + g(x0, y0 + 1) * (1 - fx) * fy + g(x0 + 1, y0 + 1) * fx * fy)


def kit_images(kit: str, pick: str, names: list[str]) -> list[np.ndarray]:
    """The costume's images of the kit model, by the names the bake used
    (an image an alternate does not recolour is the default's)."""
    base = KIT / kit
    out = []
    for name in names:
        alt = base / "Alternates" / pick / name
        out.append(_image(alt if (pick != "c00" and alt.exists()) else base / name))
    return out


def deltas(corr: np.ndarray, images0: list, images_k: list, size: tuple[int, int]) -> tuple:
    """Per in-game texel (the texture at half the bake's size): the mean
    change over its bake texels, and whether any of them sampled the kit."""
    hb, wb = corr.shape[:2]
    out = np.zeros((hb, wb, 3), dtype=np.float32)
    hit = corr[..., 0] >= 0
    for i in range(len(images0)):
        m = hit & (corr[..., 0] == i)
        if not m.any():
            continue
        u, v = corr[..., 1][m], corr[..., 2][m]
        out[m] = (sample(images_k[i], u, v) - sample(images0[i], u, v))[:, :3]
    w, h = size
    fy, fx = hb // h, wb // w
    d = out.reshape(h, fy, w, fx, 3)
    k = hit.reshape(h, fy, w, fx)
    cnt = k.sum(axis=(1, 3))
    mean = d.sum(axis=(1, 3)) / np.maximum(cnt, 1)[..., None]
    return mean, cnt > 0


def palettes(texture: dict, mean: np.ndarray, valid: np.ndarray) -> list:
    """The costume's palette: each entry moved by its texels' mean change."""
    idx = np.array(texture["texels"], dtype=int)
    base = np.array(texture["palette"], dtype=np.float32)
    out = base.copy()
    for i in range(len(base)):
        m = (idx == i) & valid
        if m.any():
            out[i, :3] = np.clip(base[i, :3] + mean[m].mean(axis=0), 0, 255)
    return [[int(round(c)) for c in col] for col in out]


def build(name: str, corr_path: Path, scenes: list[Path]) -> dict:
    spec = COSTUMES[name]
    corr = np.load(corr_path)
    names = [str(n) for n in corr["__images"]]
    images0 = kit_images(spec["kit"], "c00", names)
    alt = {pick: kit_images(spec["kit"], pick, names) for pick in dict.fromkeys(spec["picks"])}
    loaded = [json.loads(p.read_text(encoding="utf-8")) for p in scenes]
    report = {}
    for obj, key in OBJECTS.items():
        if obj not in corr.files:
            continue
        mats = [m for s in loaded for m in s["materials"] if (m.get("texture") or {}).get("file") == key]
        if not mats:
            continue
        tex = mats[0]["texture"]
        size = (int(tex["width"]), int(tex["height"]))
        pals = []
        for pick in spec["picks"]:
            mean, valid = deltas(corr[obj], images0, alt[pick], size)
            pals.append(palettes(tex, mean, valid))
        moved = max(max(abs(a - b) for ca, cb in zip(p, pals[0]) for a, b in zip(ca[:3], cb[:3]))
                    for p in pals)
        report[key] = round(float(moved), 1)
        for m in mats:
            if moved > STILL:
                m["texture"]["costume_palettes"] = pals
            else:
                m["texture"].pop("costume_palettes", None)
    for p, s in zip(scenes, loaded):
        p.write_bytes((json.dumps(s, indent=0) + "\n").encode("utf-8"))
    return report


def sheet(scene_path: Path, out: Path, names: list[str]) -> None:
    """A palette sheet: each recoloured texture in every costume, side by side."""
    from PIL import Image, ImageDraw
    s = json.loads(scene_path.read_text(encoding="utf-8"))
    rows = [m["texture"] for m in s["materials"] if (m.get("texture") or {}).get("costume_palettes")]
    scale = 2
    cell = 64 * scale
    img = Image.new("RGB", (len(names) * (cell + 4) + 60, len(rows) * (cell + 4) + 16), (40, 40, 40))
    d = ImageDraw.Draw(img)
    for k, nm in enumerate(names):
        d.text((60 + k * (cell + 4), 2), f"{k} {nm}", fill=(230, 230, 230))
    for r, t in enumerate(rows):
        d.text((2, 16 + r * (cell + 4) + cell // 2), t["file"][3:], fill=(230, 230, 230))
        idx = np.array(t["texels"], dtype=int)
        for k, pal in enumerate(t["costume_palettes"]):
            p = np.array(pal, dtype=np.uint8)
            tile = Image.fromarray(p[idx][..., :3], "RGB")
            tile = tile.resize((tile.width * cell // max(tile.width, tile.height),
                                tile.height * cell // max(tile.width, tile.height)), Image.NEAREST)
            img.paste(tile, (60 + k * (cell + 4), 16 + r * (cell + 4)))
    img.save(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--name", required=True, choices=sorted(COSTUMES))
    ap.add_argument("--corr", type=Path, required=True)
    ap.add_argument("--scene", type=Path, action="append", required=True)
    ap.add_argument("--sheet", type=Path)
    args = ap.parse_args()
    report = build(args.name, args.corr, args.scene)
    print("p6 costumes:", ", ".join(f"{k} {'recoloured' if v > STILL else 'kept'} ({v})" for k, v in report.items()))
    if args.sheet:
        sheet(args.scene[0], args.sheet, COSTUMES[args.name]["names"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
