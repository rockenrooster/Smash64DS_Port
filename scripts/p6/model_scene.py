#!/usr/bin/env python3
"""A fighter model as a scene for Blender (P6 A3): joints, meshes, materials.

`scene(model, detail, costume, out)` writes `scene.json` and the textures it
names (PNG) under `out` -- a directory under assets/p6-art/ (ROM-derived, local:
plan D10's rule for kit data holds for the base's too). The JSON is what
`scripts/p6/blender/import_scene.py` builds a .blend from:

- joints: the JointTree rows with their parent, rest transform (translate,
  XYZ Euler rotation, scale: syMatrixTraRotRpyRSca's order) and model-space
  rest matrix (Y-up, SSB64 units);
- vertices: every Vtx the lists draw, with the joint whose matrix places it,
  its joint-local and model-space position, texture coordinates in texels,
  its normal (lit) or colour;
- triangles: three vertex indices, the joint whose list draws them and a
  material index;
- materials: the state a triangle draws with -- combine words, lighting,
  texture (image, format, size, palette for `costume`) or primitive colour.

The vertex cache is resolved as the RSP sees it (n64_model.FighterModel), so
a seam triangle names vertices of two joints.
"""
from __future__ import annotations

import argparse
import json
import math
import struct
import sys
import zlib
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import n64_model as N  # noqa: E402

G_LIGHTING = 0x00020000

# Texture formats (G_IM_FMT_*) and sizes (G_IM_SIZ_*).
FMT_RGBA, FMT_YUV, FMT_CI, FMT_IA, FMT_I = 0, 1, 2, 3, 4
MOBJ_SPLIT = 0x0002
MOBJ_FRAC = 0x0010
SIZ_4B, SIZ_8B, SIZ_16B, SIZ_32B = 0, 1, 2, 3


# ------------------------------------------------------------------ textures

def rgba5551(v: int) -> tuple:
    r = (v >> 11) & 0x1F
    g = (v >> 6) & 0x1F
    b = (v >> 1) & 0x1F
    return ((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2), 255 if v & 1 else 0)


def decode_texture(data: bytes, off: int, fmt: int, siz: int, width: int, height: int,
                   palette: list | None = None, line_bytes: int | None = None) -> list:
    """Texels (RGBA tuples, row-major) of an N64 texture at `off`. `line_bytes`
    is the TMEM row pitch when a tile's line is wider than the image."""
    bits = {SIZ_4B: 4, SIZ_8B: 8, SIZ_16B: 16, SIZ_32B: 32}[siz]
    pitch = line_bytes if line_bytes else (width * bits + 7) // 8
    out = []
    for y in range(height):
        row = off + y * pitch
        for x in range(width):
            if bits == 4:
                byte = data[row + x // 2] if row + x // 2 < len(data) else 0
                v = (byte >> 4) if x % 2 == 0 else (byte & 0xF)
            elif bits == 8:
                v = data[row + x] if row + x < len(data) else 0
            elif bits == 16:
                v = struct.unpack_from(">H", data, row + 2 * x)[0] if row + 2 * x + 2 <= len(data) else 0
            else:
                v = struct.unpack_from(">I", data, row + 4 * x)[0] if row + 4 * x + 4 <= len(data) else 0
            if fmt == FMT_CI:
                out.append(palette[v] if palette and v < len(palette) else (255, 0, 255, 255))
            elif fmt == FMT_RGBA and bits == 16:
                out.append(rgba5551(v))
            elif fmt == FMT_RGBA and bits == 32:
                out.append(((v >> 24) & 255, (v >> 16) & 255, (v >> 8) & 255, v & 255))
            elif fmt == FMT_IA and bits == 4:
                i = (v >> 1) * 255 // 7
                out.append((i, i, i, 255 if v & 1 else 0))
            elif fmt == FMT_IA and bits == 8:
                i = (v >> 4) * 17
                out.append((i, i, i, (v & 0xF) * 17))
            elif fmt == FMT_IA and bits == 16:
                out.append((v >> 8, v >> 8, v >> 8, v & 255))
            elif fmt == FMT_I and bits == 4:
                out.append((v * 17,) * 3 + (v * 17,))
            elif fmt == FMT_I and bits == 8:
                out.append((v, v, v, v))
            else:
                out.append((255, 0, 255, 255))
    return out


def palette_rgba16(data: bytes, off: int, count: int) -> list:
    return [rgba5551(struct.unpack_from(">H", data, off + 2 * i)[0]) for i in range(count)]


def write_png(path: Path, width: int, height: int, texels: list) -> None:
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            raw += bytes(texels[y * width + x])

    def chunk(tag: bytes, body: bytes) -> bytes:
        return struct.pack(">I", len(body)) + tag + body + struct.pack(">I", zlib.crc32(tag + body) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    path.write_bytes(png)


# ------------------------------------------------------------------- matrices

def euler_xyz(rx: float, ry: float, rz: float) -> list:
    """Rz * Ry * Rx (column vectors): syMatrixRotRpyR's (roll, pitch, yaw)."""
    cx, sx = math.cos(rx), math.sin(rx)
    cy, sy = math.cos(ry), math.sin(ry)
    cz, sz = math.cos(rz), math.sin(rz)
    return [
        [cz * cy, cz * sy * sx - sz * cx, cz * sy * cx + sz * sx],
        [sz * cy, sz * sy * sx + cz * cx, sz * sy * cx - cz * sx],
        [-sy, cy * sx, cy * cx],
    ]


def local_matrix(joint: N.Joint) -> list:
    r = euler_xyz(*joint.rotate)
    s = joint.scale
    t = joint.translate
    return [[r[0][0] * s[0], r[0][1] * s[1], r[0][2] * s[2], t[0]],
            [r[1][0] * s[0], r[1][1] * s[1], r[1][2] * s[2], t[1]],
            [r[2][0] * s[0], r[2][1] * s[1], r[2][2] * s[2], t[2]],
            [0.0, 0.0, 0.0, 1.0]]


def mat_mul(a: list, b: list) -> list:
    return [[sum(a[r][k] * b[k][c] for k in range(4)) for c in range(4)] for r in range(4)]


def transform(m: list, p) -> list:
    return [m[r][0] * p[0] + m[r][1] * p[1] + m[r][2] * p[2] + m[r][3] for r in range(3)]


# ---------------------------------------------------------------------- scene

class _State:
    """The RDP state a triangle draws with, as the list and its MObjs' segment-E
    lists (objdisplay.c gcDrawMObjForDObj) set it: the image register, the
    TLUT and the texture TMEM hold, the tiles, primitive colour."""
    def __init__(self):
        self.combine = (0, 0)
        self.geometry = 0
        self.texture_on = False
        self.image = None           # image register: SETTIMG's or an MObj's offset
        self.tlut = None            # palette offset LOADTLUT loaded
        self.texture = None         # image offset LOADBLOCK loaded
        self.tile = {}              # tile -> (fmt, siz, line, palette)
        self.tile_size = {}         # tile -> (width, height)
        self.prim = 0xFFFFFFFF

    def mobj(self, m, costume: int) -> None:
        if m.flags & N.MOBJ_PALETTE and m.palettes:
            self.image = m.palettes[min(costume, len(m.palettes) - 1)]
            if m.flags & (MOBJ_SPLIT | N.MOBJ_SPRITES):
                self.tlut = self.image
        if m.flags & N.MOBJ_PRIMCOLOR:
            self.prim = m.primcolor
        if m.flags & (MOBJ_FRAC | N.MOBJ_SPRITES) and m.sprites:
            self.image = m.sprites[0]


def scene(fm: N.FighterModel, detail: str, costume: int, out: Path) -> dict:
    out.mkdir(parents=True, exist_ok=True)
    joints = fm.trees[detail][:-1]
    parents = fm.parents(detail)
    world = []
    for index, joint in enumerate(joints):
        local = local_matrix(joint)
        world.append(local if parents[index] < 0 else mat_mul(world[parents[index]], local))
    vertices = []
    vertex_index = {}
    triangles = []
    materials = []
    material_index = {}
    textures = {}
    spans = []                 # per material: {joint: the span commands that set it up}
    preludes = {}              # per joint: its list's commands before the first load
    data = fm.model.data
    cache = [None] * N.VERTEX_CACHE
    for joint, dl in enumerate(fm.lists(detail)):
        if dl is None:
            continue
        mobjs = fm.mobjs(detail, joint)
        st = _State()
        # The state commands since the last vertex load or triangle: a span's
        # material setup, kept per material and joint for the writer
        # (model_writer.py re-emits a material as its base drew it there).
        pending = []
        prelude = None
        for pc, g in N.walk_dl(fm.model, dl):
            op = g.op
            if op not in (N.G_VTX, N.G_TRI1, N.G_TRI2, N.G_ENDDL):
                pending.append([f"{g.w0:08x}", f"{g.w1:08x}",
                                None if g.target is None else [g.target[0], g.target[1], g.target[2]]])
            if op == N.G_VTX:
                if prelude is None:
                    prelude = list(pending)
                span_cmds = list(pending) if pending else None
                pending = []
                if span_cmds is not None:
                    st.span = span_cmds
                n, v0 = N.vtx_load(g)
                for i in range(n):
                    addr = g.target[2] + i * N.VERTEX_SIZE
                    key = (joint, addr)
                    if key not in vertex_index:
                        v = fm.vertex(addr)
                        local = (v.x, v.y, v.z)
                        vertex_index[key] = len(vertices)
                        vertices.append({
                            "joint": joint, "address": addr, "local": local,
                            "model": [round(c, 4) for c in transform(world[joint], local)],
                            "st": (v.s, v.t), "flag": v.flag,
                            "rgba": list(v.rgba), "normal": list(v.normal()),
                        })
                    cache[v0 + i] = vertex_index[key]
            elif op in (N.G_TRI1, N.G_TRI2):
                pending = []
                mat = _material(fm, st, costume, data, textures, out)
                key = json.dumps(mat, sort_keys=True)
                if key not in material_index:
                    material_index[key] = len(materials)
                    materials.append(mat)
                    spans.append({})
                spans[material_index[key]].setdefault(str(joint), getattr(st, "span", None))
                for tri in ((N.tri1(g),) if op == N.G_TRI1 else N.tri2(g)):
                    triangles.append({"joint": joint, "v": [cache[i] for i in tri],
                                      "material": material_index[key]})
            elif op == N.G_SETCOMBINE:
                st.combine = (g.w0, g.w1)
            elif op == N.G_GEOMETRYMODE:
                st.geometry = (st.geometry & (g.w0 & 0xFFFFFF)) | g.w1
            elif op == N.G_TEXTURE:
                st.texture_on = (g.w0 & 0xFF) != 0
            elif op == N.G_SETTIMG:
                st.image = g.target[2] if g.target else None
            elif op == N.G_LOADTLUT:
                st.tlut = st.image
            elif op in (N.G_LOADBLOCK, N.G_LOADTILE):
                st.texture = st.image
            elif op == N.G_SETTILE:
                tile = (g.w1 >> 24) & 7
                st.tile[tile] = ((g.w0 >> 21) & 7, (g.w0 >> 19) & 3, (g.w0 >> 9) & 0x1FF, (g.w1 >> 20) & 0xF,
                                 # cms, cmt (bit 0 mirror, bit 1 clamp), masks, maskt
                                 ((g.w1 >> 8) & 3, (g.w1 >> 18) & 3, (g.w1 >> 4) & 0xF, (g.w1 >> 14) & 0xF))
            elif op == N.G_SETTILESIZE:
                tile = (g.w1 >> 24) & 7
                st.tile_size[tile] = ((((g.w1 >> 12) & 0xFFF) >> 2) + 1, ((g.w1 & 0xFFF) >> 2) + 1)
            elif op == N.G_SETPRIMCOLOR:
                st.prim = g.w1
            elif op == N.G_DL and (g.w1 >> 24) == N.SEGMENT_E:
                index = (g.w1 & 0xFFFFFF) // 8
                if index < len(mobjs):
                    st.mobj(mobjs[index], costume)
        preludes[str(joint)] = prelude
    for mat, sp in zip(materials, spans):
        mat["spans"] = sp
    result = {
        "detail": detail, "costume": costume,
        "joints": [{"index": i, "parent": parents[i], "depth": j.depth,
                    "translate": list(j.translate), "rotate": list(j.rotate), "scale": list(j.scale),
                    "world": world[i], "has_list": fm.lists(detail)[i] is not None}
                   for i, j in enumerate(joints)],
        "vertices": vertices, "triangles": triangles, "materials": materials,
        "preludes": preludes, "lists": {str(j): dl for j, dl in enumerate(fm.lists(detail)) if dl is not None},
    }
    (out / "scene.json").write_text(json.dumps(result, indent=0), encoding="utf-8")
    return result


def _material(fm, st: _State, costume: int, data: bytes, textures: dict, out: Path) -> dict:
    lit = bool(st.geometry & G_LIGHTING)
    mat = {"combine": [st.combine[0], st.combine[1]], "lit": lit,
           "prim": f"{st.prim:08x}", "texture": None}
    tile = st.tile.get(0)
    if not st.texture_on or tile is None or st.texture is None:
        return mat
    fmt, siz, line, pal_index, wrap = tile
    width, height = st.tile_size.get(0, (32, 32))
    image = st.texture
    palette = None
    pal_off = None
    if fmt == FMT_CI and st.tlut is not None:
        pal_off = st.tlut
        count = 16 if siz == SIZ_4B else 256
        palette = palette_rgba16(data, pal_off + (pal_index * 32 if siz == SIZ_4B else 0), count)
    key = (image, fmt, siz, width, height, pal_off)
    if key not in textures:
        name = f"tex_{image:05x}_{fmt}{siz}_{width}x{height}" + (f"_p{pal_off:05x}" if pal_off else "") + ".png"
        line_bytes = line * 8 if line else None
        texels = decode_texture(data, image, fmt, siz, width, height, palette, line_bytes)
        write_png(out / name, width, height, texels)
        textures[key] = name
    # Wrap per axis: "clamp" (cm bit 1, or a zero mask: the RDP clamps an
    # unmasked coordinate), "mirror" (cm bit 0) or "repeat".
    def _wrap(cm, mask):
        return "mirror" if cm & 1 else ("clamp" if (cm & 2) or mask == 0 else "repeat")
    mat["texture"] = {"file": textures[key], "width": width, "height": height,
                      "fmt": fmt, "siz": siz, "wrap": [_wrap(wrap[0], wrap[2]), _wrap(wrap[1], wrap[3])]}
    return mat


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("export", type=Path, help="a content's export directory")
    ap.add_argument("--o2r", type=Path)
    ap.add_argument("--detail", choices=N.DETAILS, default="high")
    ap.add_argument("--costume", type=int, default=0)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    fm = N.FighterModel.from_export(args.export, args.o2r)
    s = scene(fm, args.detail, args.costume, args.out)
    print(f"{args.export.name} {args.detail}: {len(s['joints'])} joints, {len(s['vertices'])} vertices, "
          f"{len(s['triangles'])} triangles, {len(s['materials'])} materials -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
