#!/usr/bin/env python3
"""P6 A11 review: a custom Kirby hat on Kirby, rendered on the host.

Kirby's model at rest (the staged image's main and model files) and one list
of a custom power's hat file (scripts/p4/p4_kirby_custom_hats.py, written by
p4_kirby.py under its --out o2r/), drawn where the game draws it: at Kirby's
joint 6, his body (model row 2). Both go through model_scene.scene (the hat
as a one-joint model), so vertices, the vertex cache, materials and textures
are read as the RSP and the review scenes read them; the render is a small
z-buffered, orthographic, lambert-lit rasterizer (texels nearest, alpha 0
cut out), from the front, three-quarter, side and back, side by side.

    hat_preview.py --staging <remix staging> --hat <o2r/19xx> [--list 0] \\
        --out preview.png [--scale 3] [--detail low]

Build output (derived from the user's ROM): keep it local.
"""
from __future__ import annotations

import argparse
import math
import struct
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "p4"))
import n64_model as N  # noqa: E402
import model_scene as S  # noqa: E402

# Kirby's main and model files and FTAttributes offset in the staged image
# (export-x/kirby/resolved.json descriptor); the hat's joint is his row 2.
KIRBY = (0xE5, 0x148, 0x808)
KIRBY_HAT_ROW = 2
VIEWS = (("front", 0.0), ("three-quarter", 35.0), ("side", 90.0), ("back", 180.0))
LIGHT = (0.35, 0.55, 0.75)
AMBIENT = 0.45
# Kirby's untextured materials take their colour at run time (costume prim
# and lights); the review paints them his default pink and red.
KIRBY_COLOURS = {1: (250, 160, 190), 2: (220, 40, 60)}
# His face (material 0) is a 32x32 CI4 image drawn through a tile whose
# origin is (20.75, 31.25) at S scale 0.8 (row 2's G_TEXTURE d7000002
# ccd2ffff, G_SETTILESIZE f205307d 000cf0f9), which model_scene.scene's
# texture size (lrs + 1) and st (unscaled) do not read: the review applies
# them here.
KIRBY_FACE = {"material": 0, "image": 0x1CF60, "palette": 0x1CF38, "size": (32, 32),
              "scale": (0xCCD2 / 65536.0, 1.0), "origin": (20.75, 31.25)}


class _OneList:
    """A file holding one display list, as model_scene.scene reads a model:
    one joint at rest drawing it, no MObjs."""

    def __init__(self, blob: bytes, offset: int):
        self.model = N.RelocFile.read(blob)
        root = N.Joint(1, offset, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0), (1.0, 1.0, 1.0))
        end = N.Joint(N.JOINT_TREE_END, None, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0), (1.0, 1.0, 1.0))
        self.trees = {d: [root, end] for d in N.DETAILS}
        self.offset = offset

    def lists(self, detail: str):
        return [self.offset]

    def parents(self, detail: str = "high"):
        return [-1]

    def mobjs(self, detail: str, joint: int):
        return []

    def vertex(self, address: int):
        return N.Vertex.unpack(self.model.data, address)


def _png_texels(path: Path):
    """A PNG model_scene wrote (8-bit RGBA, filter 0 rows)."""
    import zlib
    data = path.read_bytes()
    width, height = struct.unpack_from(">II", data, 16)
    at, idat = 8, b""
    while at < len(data):
        n = struct.unpack_from(">I", data, at)[0]
        tag = data[at + 4:at + 8]
        if tag == b"IDAT":
            idat += data[at + 8:at + 8 + n]
        at += 12 + n
    raw = zlib.decompress(idat)
    rows = []
    for y in range(height):
        row = raw[y * (1 + 4 * width) + 1:(y + 1) * (1 + 4 * width)]
        rows.append([tuple(row[4 * x:4 * x + 4]) for x in range(width)])
    return width, height, rows


def _sample(tex, s: float, t: float):
    width, height, rows, wrap = tex

    def axis(c, n, mode):
        i = int(math.floor(c))
        if mode == "clamp":
            return min(max(i, 0), n - 1)
        if mode == "mirror":
            i %= 2 * n
            return i if i < n else 2 * n - 1 - i
        return i % n
    return rows[axis(t, height, wrap[1])][axis(s, width, wrap[0])]


def _meshes(scene: dict, folder: Path, matrix=None, colours=None, face=None, data=None):
    """Each triangle's corners (Kirby's model space, texel st), texture and
    flat colour."""
    textures = {}
    tris = []
    for tri in scene["triangles"]:
        mat = scene["materials"][tri["material"]]
        tex = None
        colour = (colours or {}).get(tri["material"])
        fix = face if (face is not None and tri["material"] == face["material"]) else None
        if mat["texture"] is not None and colour is None:
            name = mat["texture"]["file"]
            if name not in textures:
                w, h, rows = _png_texels(folder / name)
                if fix is not None:
                    w, h = fix["size"]
                    flat = S.decode_texture(data, fix["image"], S.FMT_CI, S.SIZ_4B, w, h,
                                            S.palette_rgba16(data, fix["palette"], 16))
                    rows = [flat[y * w:(y + 1) * w] for y in range(h)]
                textures[name] = (w, h, rows, mat["texture"]["wrap"])
            tex = textures[name]
        corners = []
        for vi in tri["v"]:
            v = scene["vertices"][vi]
            p = v["model"]
            n = v["normal"]
            if matrix is not None:
                p = S.transform(matrix, p)
                n = [sum(matrix[r][c] * n[c] for c in range(3)) for r in range(3)]
            m = math.sqrt(sum(c * c for c in n)) or 1.0
            st = (v["st"][0] / 32.0, v["st"][1] / 32.0)
            if fix is not None:
                st = (st[0] * fix["scale"][0] - fix["origin"][0], st[1] * fix["scale"][1] - fix["origin"][1])
            corners.append((p, [c / m for c in n], st))
        if colour is None and tex is None:
            p = int(mat.get("prim") or "ffffffff", 16)
            colour = ((p >> 24) & 255, (p >> 16) & 255, (p >> 8) & 255)
        tris.append((corners, tex, colour))
    return tris


def render(tris, yaw: float, size: int, extent: float, centre=(0.0, 0.0, 0.0)):
    """An RGB image (rows of tuples): orthographic about `centre`, the camera
    on +Z turned `yaw` degrees about Y toward +X; each triangle lit flat
    (its face normal: the lists' normals are run-time state)."""
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    light = [x / math.sqrt(sum(y * y for y in LIGHT)) for x in LIGHT]
    img = [[(70, 90, 110)] * size for _ in range(size)]
    depth = [[-1e30] * size for _ in range(size)]
    scale = size / (2.0 * extent)

    def view(p):
        x, y, z = p[0] - centre[0], p[1] - centre[1], p[2] - centre[2]
        return (x * c - z * s, y, x * s + z * c)
    for corners, tex, colour in tris:
        pts = [view(p) for p, _, _ in corners]
        e1 = [pts[1][k] - pts[0][k] for k in range(3)]
        e2 = [pts[2][k] - pts[0][k] for k in range(3)]
        face = (e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0])
        fm = math.sqrt(sum(v * v for v in face)) or 1.0
        lam = abs(sum(face[k] / fm * light[k] for k in range(3)))
        shade = min(1.0, AMBIENT + (1.0 - AMBIENT) * lam)
        sx = [(p[0] + extent) * scale for p in pts]
        sy = [(extent - p[1]) * scale for p in pts]
        area = (sx[1] - sx[0]) * (sy[2] - sy[0]) - (sx[2] - sx[0]) * (sy[1] - sy[0])
        if abs(area) < 1e-9:
            continue
        x0, x1 = max(int(min(sx)), 0), min(int(max(sx)) + 1, size - 1)
        y0, y1 = max(int(min(sy)), 0), min(int(max(sy)) + 1, size - 1)
        for py in range(y0, y1 + 1):
            for px in range(x0, x1 + 1):
                qx, qy = px + 0.5, py + 0.5
                w0 = ((sx[1] - qx) * (sy[2] - qy) - (sx[2] - qx) * (sy[1] - qy)) / area
                w1 = ((sx[2] - qx) * (sy[0] - qy) - (sx[0] - qx) * (sy[2] - qy)) / area
                w2 = 1.0 - w0 - w1
                if w0 < 0 or w1 < 0 or w2 < 0:
                    continue
                z = w0 * pts[0][2] + w1 * pts[1][2] + w2 * pts[2][2]
                if z <= depth[py][px]:
                    continue
                if tex is not None:
                    st = [w0 * corners[0][2][k] + w1 * corners[1][2][k] + w2 * corners[2][2][k] for k in (0, 1)]
                    texel = _sample(tex, st[0], st[1])
                    if texel[3] == 0:
                        continue
                    base = texel[:3]
                else:
                    base = colour
                depth[py][px] = z
                img[py][px] = tuple(int(b * shade) for b in base)
    return img


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--hat", type=Path, required=True, help="the hat file (O2R) p4_kirby.py wrote")
    ap.add_argument("--list", type=lambda v: int(v, 0), default=0, help="the list's offset in the file")
    ap.add_argument("--detail", choices=N.DETAILS, default="low")
    ap.add_argument("--size", type=int, default=200)
    ap.add_argument("--extent", type=float, default=230.0, help="half the shown height, model units")
    ap.add_argument("--centre-y", type=float, default=175.0, help="the view's centre height, model units")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    import remix_rom as R
    rom = R.Rom(args.staging / "ssb64asm.z64")
    main_id, model_id, attr = KIRBY
    kirby = N.FighterModel(N.RelocFile.read(rom.o2r(main_id)), N.RelocFile.read(rom.o2r(model_id)), attr)
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        body = S.scene(kirby, args.detail, 0, tmp / "kirby")
        hat = S.scene(_OneList(args.hat.read_bytes(), args.list), args.detail, 0, tmp / "hat")
        row = body["joints"][KIRBY_HAT_ROW]["world"]
        tris = (_meshes(body, tmp / "kirby", colours=KIRBY_COLOURS, face=KIRBY_FACE, data=kirby.model.data) +
                _meshes(hat, tmp / "hat", row))
    images = [render(tris, yaw, args.size, args.extent, (0.0, args.centre_y, 0.0)) for _, yaw in VIEWS]
    width = args.size * len(images) + 4 * (len(images) - 1)
    texels = []
    for y in range(args.size):
        for k, img in enumerate(images):
            texels += [p + (255,) for p in img[y]]
            if k + 1 < len(images):
                texels += [(20, 20, 20, 255)] * 4
    S.write_png(args.out, width, args.size, texels)
    print(f"hat preview: {len(body['triangles'])} body and {len(hat['triangles'])} hat triangles -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
