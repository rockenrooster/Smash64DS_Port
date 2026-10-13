#!/usr/bin/env python3
"""A look's face frames from the 3DS kit's face atlas (P6 A3, owner route
2026-10-13: Peach's model reshaped).

Peach's face is two 32x32 CI4 half-face textures whose MObj sprite lists
hold her expressions (open, blink, hurt, ...). Each frame of the look is
composed, not baked: the skin colour, then the kit's matching eye tile (the
atlas holds open, half-lidded, closed and side-glance eyes) scaled into the
box Peach's eye takes in her open frame, then quantized to 16 colours. The
ops (art/p6/<name>/look/face_frames.json) name Peach's frame offsets, the kit
tile each takes (a box in the atlas: coordinates, not texels) and the skin
colour; no kit texel is stored (D10).

    face_frames.py --export builds/p4-staging/export/peach --kit-face <palutena_face.png>
        --ops art/p6/palutena/look/face_frames.json --out builds/p6-art/palutena/face
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import n64_model as N  # noqa: E402
import model_scene as M  # noqa: E402


def peach_frame(fm: N.FighterModel, offset: int, palette_offset: int) -> Image.Image:
    data = fm.model.data
    pal = M.palette_rgba16(data, palette_offset, 16)
    tex = M.decode_texture(data, offset, M.FMT_CI, M.SIZ_4B, 32, 32, pal)
    im = Image.new("RGBA", (32, 32))
    im.putdata([tuple(c) for c in tex])
    return im


def eye_box(frame: Image.Image, skin: tuple, threshold: int = 60) -> tuple:
    """The box of the open eye in Peach's open frame: pixels far from the
    skin colour in the frame's upper two thirds (the lid line under the eye
    is below it)."""
    a = np.asarray(frame.convert("RGB"), dtype=int)
    diff = np.abs(a - np.array(skin)).sum(2)
    mask = diff > threshold
    mask[int(a.shape[0] * 0.62):, :] = False
    ys, xs = np.nonzero(mask)
    return int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1


def compose(kit_face: Image.Image, tile: list, box: tuple, skin: tuple, mirror: bool,
            soft=(22.0, 64.0), saturation: float = 1.0) -> Image.Image:
    """The frame: skin, then the kit eye with its own skin keyed out (a
    pixel near the tile's border colour goes transparent, softly), so no box
    edge shows."""
    out = Image.new("RGBA", (32, 32), skin + (255,))
    x0, y0, x1, y1 = box
    eye = kit_face.crop(tuple(tile)).convert("RGBA")
    # Peach's face UVs run bottom-up (her brow line sits below the eye in
    # the texture), so a tile goes in upside down to read upright on the
    # model (owner 2026-10-13: "your eye textures are upside down").
    eye = eye.transpose(Image.FLIP_TOP_BOTTOM)
    if mirror:
        eye = eye.transpose(Image.FLIP_LEFT_RIGHT)
    a = np.asarray(eye, dtype=float)
    # Keep what is eye, drop what is skin (lit or shaded): skin is a warm hue
    # of middling saturation; lashes and pupil are dark, the white is pale,
    # the iris is green.
    hsv = np.asarray(eye.convert("RGB").convert("HSV"), dtype=float)
    hue = hsv[:, :, 0] * (360.0 / 255.0)
    sat = hsv[:, :, 1] / 255.0
    val = hsv[:, :, 2] / 255.0
    skinish = ((hue < 45.0) | (hue > 330.0)) & (sat > 0.12) & (val > soft[0] / 100.0)
    alpha = np.where(skinish, 0.0, 1.0)
    # Soften the key's edge by one pixel.
    pad = np.pad(alpha, 1, mode="edge")
    alpha = (pad[1:-1, 1:-1] * 4 + pad[:-2, 1:-1] + pad[2:, 1:-1] + pad[1:-1, :-2] + pad[1:-1, 2:]) / 8.0
    if saturation != 1.0:
        grey = a[:, :, :3].mean(2, keepdims=True)
        a[:, :, :3] = np.clip(grey + (a[:, :, :3] - grey) * saturation, 0, 255)
    a[:, :, 3] = alpha * 255.0
    eye = Image.fromarray(a.astype(np.uint8), "RGBA").resize((x1 - x0, y1 - y0), Image.LANCZOS)
    out.alpha_composite(eye, (x0, y0))
    return out


def recolored(fm: N.FighterModel, offset: int, palette_offset: int, mapping: dict,
              width: int = 32, height: int = 32, texel_map: dict | None = None, paint=()):
    """Peach's own texture with its palette entries recoloured (ops
    "palette_map": index -> RGB hex): the texels stay hers, so every
    expression keeps her drawing (owner 2026-10-13: Peach's model restyled).
    "texel_map" (index -> index) merges entries to free palette slots;
    "paint" draws rows of one index (every Nth column from an offset): the
    circlet across the forehead (owner: "gold circlet should be part of face
    texture"). Rows are the texture's own (the model shows them V flipped:
    the forehead is the last rows)."""
    data = fm.model.data
    pal = [list(c) for c in M.palette_rgba16(data, palette_offset, 16)]
    for index, hexrgb in mapping.items():
        pal[int(index)][:3] = [int(hexrgb[i:i + 2], 16) for i in (0, 2, 4)]
    idx = []
    for byte in data[offset:offset + width * height // 2]:
        idx += [byte >> 4, byte & 15]
    if texel_map:
        idx = [int(texel_map.get(str(i), i)) for i in idx]
    for p in paint:
        for r in p["rows"]:
            for x in range(width):
                if "every" in p and (x - p.get("offset", 0)) % p["every"]:
                    continue
                if "columns" in p and x not in p["columns"]:
                    continue
                idx[r * width + x] = p["index"]
    im = Image.new("RGBA", (width, height))
    im.putdata([tuple(pal[i]) for i in idx])
    return im, [tuple(c) for c in pal]


def quantize(img: Image.Image, colors: int = 16):
    q = img.convert("RGB").quantize(colors=colors, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = q.getpalette()[:colors * 3]
    pal += [0] * (colors * 3 - len(pal))
    return np.asarray(q, dtype=np.uint8), [tuple(pal[i * 3:i * 3 + 3]) + (255,) for i in range(colors)]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--export", type=Path, required=True)
    ap.add_argument("--kit-face", type=Path)
    ap.add_argument("--ops", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    ops = json.loads(args.ops.read_text(encoding="utf-8"))
    fm = N.FighterModel.from_export(args.export)
    if ops.get("mode") == "peach-recolor":
        args.out.mkdir(parents=True, exist_ok=True)
        made = {}
        jobs = [(off, int(ops["palette"], 0), ops["palette_map"], 32, 32, f"face_{int(off, 0):05x}.png",
                 ops.get("texel_map"), ops.get("paint", ()))
                for off in ops["frames"]]
        for extra in ops.get("extra", ()):
            w, h = extra["size"]
            jobs.append((extra["texture"], int(extra["palette"], 0), extra["palette_map"], w, h, extra["name"],
                         extra.get("texel_map"), extra.get("paint", ())))
        sheet = Image.new("RGBA", (136 * len(jobs), 264), (40, 40, 40, 255))
        for k, (off, pal_off, mapping, w, h, name, tmap, paint) in enumerate(jobs):
            img, pal = recolored(fm, int(off, 0), pal_off, mapping, w, h, tmap, paint)
            img.save(args.out / name)
            made[off] = {"file": name, "palette": [list(c) for c in pal]}
            orig, _ = recolored(fm, int(off, 0), pal_off, {}, w, h)
            # The sheet shows each texture as the model shows it (V flipped).
            sheet.paste(orig.transpose(Image.FLIP_TOP_BOTTOM).resize((128, 128 * h // w), Image.NEAREST), (k * 136, 0))
            sheet.paste(img.transpose(Image.FLIP_TOP_BOTTOM).resize((128, 128 * h // w), Image.NEAREST), (k * 136, 136))
        sheet.save(args.out / "face_sheet.png")
        (args.out / "frames.json").write_text(json.dumps({"mode": "peach-recolor", "frames": made}, indent=1),
                                              encoding="utf-8")
        print(f"{len(made)} recoloured textures -> {args.out}")
        return 0
    kit_face = Image.open(args.kit_face).convert("RGBA")
    skin = tuple(int(ops["skin"][i:i + 2], 16) for i in (0, 2, 4))
    peach_skin = tuple(int(ops["peach_skin"][i:i + 2], 16) for i in (0, 2, 4))
    ref = peach_frame(fm, int(ops["reference_frame"], 0), int(ops["palette"], 0))
    box = tuple(ops.get("eye_box") or eye_box(ref, peach_skin))
    if ops.get("eye_grow"):
        # A bigger eye than Peach's box (SSB64 faces read by their eyes).
        gx, gy = ops["eye_grow"]
        x0, y0, x1, y1 = box
        box = (max(0, x0 - gx), max(0, y0 - gy), min(32, x1 + gx), min(32, y1 + gy))
    args.out.mkdir(parents=True, exist_ok=True)
    made = {}
    sheet = Image.new("RGBA", (34 * len(ops["frames"]) * 4, 32 * 4 * 2 + 8), (40, 40, 40, 255))
    for k, (offset, spec) in enumerate(ops["frames"].items()):
        img = compose(kit_face, spec["tile"], box, skin, spec.get("mirror", False),
                      tuple(ops.get("key", (22.0, 64.0))), ops.get("saturation", 1.0))
        ci, pal = quantize(img, ops.get("colors", 16))
        q = Image.fromarray(ci, "P")
        q.putpalette([c for rgba in pal for c in rgba[:3]])
        name = f"face_{int(offset, 0):05x}.png"
        q.convert("RGBA").save(args.out / name)
        made[offset] = {"file": name, "palette": [list(c) for c in pal]}
        sheet.paste(peach_frame(fm, int(offset, 0), int(ops["palette"], 0)).resize((128, 128), Image.NEAREST), (k * 136, 0))
        sheet.paste(q.convert("RGBA").resize((128, 128), Image.NEAREST), (k * 136, 136))
    sheet.save(args.out / "face_sheet.png")
    (args.out / "frames.json").write_text(json.dumps({"eye_box": box, "frames": made}, indent=1), encoding="utf-8")
    print(f"{len(made)} frames, eye box {box} -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
