#!/usr/bin/env python3
"""Bake the fitted kit's albedo onto a look's textures (P6 A3b and A3c).

A texel of a look's texture is a point on the look's surface (its UV-space
triangle's barycentric point); that point's colour is the kit albedo at the
closest point of the fitted kit surface (optionally only the kit regions the
part allows), read through the kit triangle's UVs with mirrored-repeat
addressing (the kit's Mirror UV.txt). SSB64 lights by vertex normals, so the
bake takes albedo only, no shading. Texels no triangle covers are filled from
their neighbours so filtering never reads the background.

Host-only (numpy, Pillow). Kit-derived output: builds/ only (plan D10).
"""
from __future__ import annotations

import json
import math
from pathlib import Path

import numpy as np
from PIL import Image

import look_r as R

# Kit textures that are albedo: the body, face and weapon sheets. The alpha
# sheets (palutena_alpha*, ike_alpha*) hold cards and cut-outs.
OPAQUE_IMAGES = {"palutena_body.png", "palutena_body_low.png", "palutena_face.png",
                 "palutena_face_low.png", "ike_bodyall_high.png", "ike_bodyall_low.png",
                 "ike_face_high.png", "ike_face_low.png"}


class KitAlbedo:
    def __init__(self, fit: dict, kit_dir: Path, role: str):
        tris = R.kit_triangles(fit, role)
        self.region = np.array([t[5] for t in tris])
        self.A = np.array([t[2] for t in tris])
        self.B = np.array([t[3] for t in tris])
        self.C = np.array([t[4] for t in tris])
        meshes = {m["name"]: m for m in fit["meshes"]}
        self.mesh_of = np.array([t[0] for t in tris])
        uv = []
        img = []
        self.images = {}
        for name, index in ((t[0], t[1]) for t in tris):
            m = meshes[name]
            a, b, c = m["triangles"][index]
            if m["uvs"]:
                uv.append([m["uvs"][a][:2], m["uvs"][b][:2], m["uvs"][c][:2]])
            else:
                uv.append([[0, 0], [0, 0], [0, 0]])
            image = m["image"]
            if image not in self.images and image and (kit_dir / image).exists():
                self.images[image] = np.asarray(Image.open(kit_dir / image).convert("RGBA"), dtype=float)
            img.append(image)
        self.uv = np.array(uv, dtype=float)
        self.image = img
        # Alpha-card meshes (hair strands, lace, lashes) carry their cut-out
        # in the alpha channel: their transparent texels are not albedo, and
        # their cards hang off the opaque body, so the bake reads only the
        # opaque meshes (OPAQUE_IMAGES).
        self.opaque = np.array([im in OPAQUE_IMAGES for im in self.image])
        n = np.cross(self.B - self.A, self.C - self.A)
        self.normal = n / np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-9)

    def sample(self, points: np.ndarray, regions=None, normals: np.ndarray | None = None) -> np.ndarray:
        """RGBA (0-255 floats) of the kit albedo at the closest kit point of
        each of `points` (N x 3, the base's model space). With `normals` (the
        look's surface normals there) only kit triangles facing the same way
        count, so a texel never reads an inner layer (the legs under a dress)."""
        if isinstance(regions, dict):
            # {"meshes": [...]} picks kit meshes by name (the face set).
            mask = np.isin(self.mesh_of, list(regions["meshes"]))
        else:
            mask = np.ones(len(self.region), dtype=bool) if regions is None else np.isin(self.region, list(regions))
        mask &= self.opaque
        if not mask.any():
            mask = self.opaque.copy()
        out = np.zeros((len(points), 4))
        for k, p in enumerate(points):
            m = mask
            if normals is not None:
                facing = (self.normal @ normals[k]) > 0.1
                if (m & facing).any():
                    m = m & facing
            idx = np.nonzero(m)[0]
            _pts, bary, d2 = R.closest_on_triangles(p, self.A[idx], self.B[idx], self.C[idx])
            j = int(np.argmin(d2))
            t = idx[j]
            uv = (self.uv[t] * bary[j][:, None]).sum(0)
            out[k] = self._texel(self.image[t], uv)
        return out

    def dominant(self, points: np.ndarray, regions=None, normals=None, k: int = 3,
                 pick: str = "largest") -> np.ndarray:
        """The colour most of the samples share (the largest of k clusters):
        a part's flat colour, not dragged toward its trims and shadows. With
        pick "bright", the brightest cluster holding a fifth of the samples
        (cloth whose albedo carries folds and shadow)."""
        cols = self.sample(points, regions, normals)[:, :3]
        if len(cols) <= k:
            return cols.mean(0)
        rng = np.random.default_rng(0)
        centres = cols[rng.choice(len(cols), k, replace=False)]
        for _ in range(12):
            lab = np.argmin(((cols[:, None, :] - centres[None]) ** 2).sum(2), 1)
            for c in range(k):
                if (lab == c).any():
                    centres[c] = cols[lab == c].mean(0)
        counts = np.bincount(lab, minlength=k)
        if pick == "bright":
            ok = [c for c in range(k) if counts[c] >= 0.2 * len(cols)]
            return max((centres[c] for c in ok), key=lambda c: c @ np.array([0.299, 0.587, 0.114]))
        return centres[int(np.argmax(counts))]

    def _texel(self, image, uv):
        tex = self.images.get(image)
        if tex is None:
            return np.array([255.0, 0.0, 255.0, 255.0])
        h, w = tex.shape[:2]

        def mirror(x):
            x = x % 2.0
            return 2.0 - x if x > 1.0 else x
        u = mirror(float(uv[0]))
        v = mirror(float(uv[1]))
        x = min(w - 1, max(0, int(u * w)))
        y = min(h - 1, max(0, int((1.0 - v) * h)))
        return tex[y, x]


def rasterize(width: int, height: int, tris_uv: np.ndarray, tris_xyz: np.ndarray):
    """Texel centres covered by the UV triangles: (texel list, 3D points,
    owner map, the owning triangle's normal per texel)."""
    texels = []
    points = []
    normals = []
    owner = -np.ones((height, width), dtype=int)
    for k, (uv, xyz) in enumerate(zip(tris_uv, tris_xyz)):
        px = uv[:, 0] * width
        py = (1.0 - uv[:, 1]) * height
        x0, x1 = int(max(0, math.floor(px.min()))), int(min(width - 1, math.ceil(px.max())))
        y0, y1 = int(max(0, math.floor(py.min()))), int(min(height - 1, math.ceil(py.max())))
        (ax, ay), (bx, by), (cx, cy) = zip(px, py)
        den = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy)
        if abs(den) < 1e-12:
            continue
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                sx, sy = x + 0.5, y + 0.5
                l0 = ((by - cy) * (sx - cx) + (cx - bx) * (sy - cy)) / den
                l1 = ((cy - ay) * (sx - cx) + (ax - cx) * (sy - cy)) / den
                l2 = 1 - l0 - l1
                if min(l0, l1, l2) < -0.02 or owner[y, x] >= 0:
                    continue
                owner[y, x] = k
                texels.append((x, y))
                points.append(xyz[0] * l0 + xyz[1] * l1 + xyz[2] * l2)
                n = np.cross(xyz[1] - xyz[0], xyz[2] - xyz[0])
                normals.append(n / max(np.linalg.norm(n), 1e-9))
    return texels, np.array(points), owner, np.array(normals)


def dilate(img: np.ndarray, covered: np.ndarray, passes: int = 8) -> np.ndarray:
    img = img.copy()
    cov = covered.copy()
    for _ in range(passes):
        if cov.all():
            break
        new = cov.copy()
        for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0)):
            shifted = np.roll(np.roll(img, dy, 0), dx, 1)
            scov = np.roll(np.roll(cov, dy, 0), dx, 1)
            take = (~new) & scov
            img[take] = shifted[take]
            new |= take
        cov = new
    return img


def quantize_ci(img: np.ndarray, colors: int):
    """CI texels and an RGBA palette (alpha kept 0/255) for an RGBA image."""
    rgb = Image.fromarray(img[:, :, :3].clip(0, 255).astype(np.uint8), "RGB")
    q = rgb.quantize(colors=colors, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = q.getpalette()[:colors * 3]
    pal += [0] * (colors * 3 - len(pal))
    palette = [(pal[i * 3], pal[i * 3 + 1], pal[i * 3 + 2], 255) for i in range(colors)]
    return np.asarray(q, dtype=np.uint8), palette


def bake_texture(albedo: KitAlbedo, width: int, height: int, tris_uv, tris_xyz, regions=None,
                 supersample: int = 3, smooth: int = 1):
    """A texture's texels from the kit albedo: each texel the mean of
    `supersample`^2 samples over its area (a 3DS texel is far finer than an
    SSB64 one), then a `smooth`-texel median so the quantized result reads
    as flat areas rather than noise."""
    ss = max(1, supersample)
    texels, points, owner, normals = rasterize(width * ss, height * ss, np.asarray(tris_uv, dtype=float),
                                               np.asarray(tris_xyz, dtype=float))
    acc = np.zeros((height, width, 4))
    cnt = np.zeros((height, width))
    if len(points):
        colors = albedo.sample(points, regions, normals)
        for (x, y), c in zip(texels, colors):
            acc[y // ss, x // ss] += c
            cnt[y // ss, x // ss] += 1
    covered = cnt > 0
    img = np.zeros((height, width, 4))
    img[covered] = acc[covered] / cnt[covered][:, None]
    img[:, :, 3] = 255.0
    img = dilate(img, covered)
    if smooth:
        from PIL import ImageFilter
        pil = Image.fromarray(img.clip(0, 255).astype(np.uint8), "RGBA").filter(ImageFilter.MedianFilter(2 * smooth + 1))
        img = np.asarray(pil, dtype=float)
    return img, covered


def save_png(path: Path, img: np.ndarray) -> None:
    Image.fromarray(img.clip(0, 255).astype(np.uint8), "RGBA").save(path)


if __name__ == "__main__":
    raise SystemExit("bake.py is a library: look_k.py and look_r.py call it")
