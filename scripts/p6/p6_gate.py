#!/usr/bin/env python3
"""P6 A10: Palutena's entry gate as a special file (card: she walks out of
golden halo-shaped doors; the gate article is the kit's gate, converted).

The kit's gate (gate/gate.dae, gate.png) is an arched double door, a joint a
door at its outer edge. Its front faces (twelve triangles a door) become one
display list a door, textured from the image's door half (the faces' own
planar mapping) as a 64x64 CI4 image, unlit and two-sided. The file holds a
DObjDesc tree -- the root (placed at her entry position by the entry case,
src/port/nds_p4_palutena.c), a node `front` units toward the camera, and
the two doors on their hinges -- then the lists, the vertices, the image and
its palette, as an O2R container in her special file 2 (the entry effect
draws it natively: scripts/p4/p4_articles.py's "gate" root).

The same file holds her up smash's light pillar (owner playtest 2026-10-14,
round 2: one column in front of her, exactly vertical; owner p6j: redone on
the 3DS's Heavenly Light in the owner's half-speed Strong_Up_A clip): a
second DObjDesc tree at PILLAR_TREE -- a root (placed by her code ahead of
her on the ground, src/port/nds_p4_palutena.c) and cards facing the camera:
the glow (translucent), the core (white down its middle to cyan, cut out),
the flare on the floor at its foot and the dust rolling out from it either
way (translucent) -- from one authored atlas (look_parts.py
pillar_light_atlas), unlit. Her code scales the cards over the effect's
life: the column rises from the floor with a round head, the dust rolls out
and settles, the column thins away.

A third tree at WINGS_TREE holds her forward and down smash's wings
(scripts/p6/p6_wings.py: the kit's godwing rigged, a continuous mesh a wing,
and the table her code poses it from every tic), its table's address in
the word after the tree's rows (WINGS_TABLE_SLOT).

    p6_gate.py --out builds/p6-art/palutena/gate/1b10 [--preview gate.png]

Kit-derived: the file stays local (D10); this recipe is the committed part.
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
import dae_reader  # noqa: E402
import n64_model as N  # noqa: E402
import model_writer as M  # noqa: E402
import p6_wings  # noqa: E402

KIT = REPO / "decomp" / "alt_assets" / "Fighters" / "Super Smash Bros 3DS" / "Palutena" / "gate"
# The O2R resource header every P6 file carries (its own id follows it).
HEADER = bytes(4) + b"OLER" + bytes(4) + bytes.fromhex("efbeadde") * 2 + bytes(0x40 - 20)

FILE_ID = 0x1B10
HEIGHT = 920.0        # world units: 1.3 times her height (her kit's ratio)
FRONT = 260.0         # the doors stand this far toward the camera from her
TEX = 64              # the door image's side (CI4: 2 KB, TMEM's half)
U_CROP = 0.75         # the image's door half (its right quarter is the edges')
DOBJ_END = N.JOINT_TREE_END
PILLAR_HEIGHT = 2500.0     # 2x her first pillar's 1,250 (owner p6f notes; 1.5x and 1.25x before)
PILLAR_TEX = 64            # the atlas (CI4, 2 KB)
# Each card: atlas region (s0, t0, s1, t1 in texels; s1 < s0 mirrors it),
# left and right x, bottom, top, z (toward the camera), and its PRIM alpha
# (255: cut out and opaque, else translucent). The 3DS's column (its clip on
# a model-unit grid): a white core about 85 across in a cyan band about 140,
# a faint glow past it; the dust a bank either way along the floor, 400 and
# more from its foot, 150 high. The dust cards reach out from the foot (x 0)
# and the flare lies over it; her code scales each (src/port/nds_p4_palutena.c,
# the cards in this order).
PILLAR_CARDS = (
    ("glow", (16, 0, 32, 64), -140.0, 140.0, 0.0, PILLAR_HEIGHT, 0.0, 150),
    ("core", (0, 0, 16, 64), -65.0, 65.0, 0.0, PILLAR_HEIGHT, 4.0, 255),
    ("flare", (32, 0, 64, 16), -220.0, 220.0, -40.0, 90.0, 8.0, 210),
    ("dust_l", (64, 16, 32, 48), -460.0, 0.0, -10.0, 220.0, 12.0, 200),
    ("dust_r", (32, 16, 64, 48), 0.0, 460.0, -10.0, 220.0, 12.0, 200),
)
# The pillar's tree, after the gate's five rows (p4_articles.py "pillar"):
# its root, its cards, the end.
PILLAR_TREE = 5 * N.DOBJ_DESC_SIZE
PILLAR_ROWS = len(PILLAR_CARDS) + 2
# The wings' tree after the pillar's rows, then the word that names the
# wings' table (src/port/nds_p4_palutena.c PAL_WINGS_TREE, _TABLE_SLOT;
# p4_articles.py "wings").
WINGS_TREE = PILLAR_TREE + PILLAR_ROWS * N.DOBJ_DESC_SIZE
WINGS_TABLE_SLOT = WINGS_TREE + p6_wings.TREE_ROWS * N.DOBJ_DESC_SIZE
# The model writer's cut-out state (model_writer.CUTOUT_ON / _OFF).
CUTOUT_ON = [N.Gfx(0xE2001E01, 0x00000001), N.Gfx(0xE200001C, 0xC4113078)]
CUTOUT_OFF = [N.Gfx(0xE2001E01, 0x00000000), N.Gfx(0xE200001C, 0xC4112078)]
# Translucent: G_RM_ZB_XLU_SURF / G_RM_ZB_XLU_SURF2 (the DS draws the polygon
# at its PRIM alpha), no alpha compare.
XLU_ON = [N.Gfx(0xE2001E01, 0x00000000), N.Gfx(0xE200001C, 0x00504A50)]
# The pillar's combine: colour TEXEL0, alpha TEXEL0 * PRIM, both cycles.
PILLAR_COMBINE = N.Gfx(0xFCFF97FF, 0xFF2CFE7F)

# F3DEX2 words. Geometry: lighting and back-face culling off for the doors'
# triangles, back on at the end of each list.
G_LIGHTING = 0x00020000
G_CULL_BACK = 0x00000400


class GateError(SystemExit):
    pass


def door_faces(model: dae_reader.Model, mesh) -> list:
    """The mesh's front-facing triangles (+z in the kit's rest pose)."""
    out = []
    for tri in mesh.triangles:
        a, b, c = (np.array(model.skinned_position(mesh, i)) for i in tri)
        n = np.cross(b - a, c - a)
        if np.linalg.norm(n) and (n / np.linalg.norm(n))[2] > 0.9:
            out.append(tri)
    if len(out) != 12:
        raise GateError(f"{mesh.name}: {len(out)} front faces, the kit's gate has 12")
    return out


def image_ci4() -> tuple[bytes, bytes, np.ndarray]:
    from PIL import Image
    img = Image.open(KIT / "gate.png").convert("RGBA")
    w, h = img.size
    crop = img.crop((0, 0, int(round(w * U_CROP)), h)).resize((TEX, TEX), Image.LANCZOS)
    rgba = np.asarray(crop, dtype=np.float32).copy()
    rgba[:, :, 3] = 255
    idx, palette = _quantize(rgba)
    texels = M.ci4_bytes(idx)
    pal = M.rgba5551(palette)
    preview = np.array([[palette[i][:3] for i in row] for row in idx], dtype=np.uint8)
    return texels, pal, preview


def _quantize(rgba: np.ndarray):
    import look_blend
    return look_blend.quantize(rgba, 16, False)


def vtx(x: float, y: float, z: float, s: float, t: float, rgba=(255, 255, 255, 255)) -> bytes:
    xs, ys, zs = (int(round(c)) for c in (x, y, z))
    for c in (xs, ys, zs):
        if not -0x8000 <= c <= 0x7FFF:
            raise GateError(f"vertex ({x}, {y}, {z}) overflows s16")
    return struct.pack(">hhhHhh", xs, ys, zs, 0, int(round(s)), int(round(t))) + bytes(rgba)


def door_list(tex_at: int, pal_at: int, vtx_at: int, count: int, tris: list, fid: int) -> list:
    """A door's list: the image and palette loaded as the P4 entry articles
    load theirs (Sonic's Tails plane), its vertices and triangles."""
    line = TEX * 4 // 64                      # CI4 row in 64-bit words
    mask = TEX.bit_length() - 1
    clamp = 2
    cmds = [
        N.Gfx(0xE7000000, 0),
        N.Gfx(0xD9000000 | (0xFFFFFF & ~(G_LIGHTING | G_CULL_BACK)), 0),
        N.Gfx(0xE8000000, 0),
        N.Gfx(0xF5000100, 0x05000000),
        N.Gfx(0xD7000002, 0xFFFFFFFF),
        N.Gfx(0xF5500000, (7 << 24) | (mask << 14) | (mask << 4)),
        N.Gfx(0xE7000000, 0),
        N.Gfx(0xE3001001, 0x00008000),
        N.Gfx(0xE200001C, 0x00552078),
        N.Gfx(0xFC121824, 0xFF33FFFF),
        N.Gfx(0xE7000000, 0),
        N.Gfx(0xFA000000, 0xFFFFFFFF),
        N.Gfx(0xF5400000 | (line << 9), (clamp << 18) | (mask << 14) | (clamp << 8) | (mask << 4)),
        N.Gfx(0xFD100000, pal_at // 4, ("intern", fid, pal_at)),
        N.Gfx(0xE6000000, 0),
        N.Gfx(0xF0000000, 0x0503C000),
        N.Gfx(0xE7000000, 0),
        N.Gfx(0xD7000002, 0xFFFFFFFF),
        N.Gfx(0xF2000000, (((TEX - 1) << 2) << 12) | ((TEX - 1) << 2)),
        N.Gfx(0xFD500000, tex_at // 4, ("intern", fid, tex_at)),
        N.Gfx(0xE6000000, 0),
        N.Gfx(0xF3000000, (7 << 24) | ((TEX * TEX // 4 - 1) << 12) | (2048 // line)),
        N.Gfx(0xE7000000, 0),
        N.Gfx(0x01000000 | (count << 12) | (count << 1), vtx_at, ("intern", fid, vtx_at)),
    ]
    for k in range(0, len(tris) - 1, 2):
        a, b = tris[k], tris[k + 1]
        cmds.append(N.Gfx(0x06000000 | (a[0] * 2 << 16) | (a[1] * 2 << 8) | a[2] * 2,
                          (b[0] * 2 << 16) | (b[1] * 2 << 8) | b[2] * 2))
    if len(tris) % 2:
        a = tris[-1]
        cmds.append(N.Gfx(0x05000000 | (a[0] * 2 << 16) | (a[1] * 2 << 8) | a[2] * 2, 0))
    cmds += [N.Gfx(0xE7000000, 0),
             N.Gfx(0xE3001001, 0x00000000),
             N.Gfx(0xD9FFFFFF, G_LIGHTING | G_CULL_BACK),
             N.Gfx(0xDF000000, 0)]
    return cmds


def build() -> tuple[bytes, dict]:
    model = dae_reader.read(KIT / "gate.dae")
    meshes = {m.name[0]: m for m in model.meshes}          # "L", "R"
    if set(meshes) != {"L", "R"}:
        raise GateError(f"gate meshes {[m.name for m in model.meshes]}: expected L and R")
    top = max(model.skinned_position(m, v)[1] for m in model.meshes for v in m.used_vertices())
    scale = HEIGHT / top
    f = N.RelocFile(HEADER, FILE_ID, bytearray())
    app = M.Appender(f)
    # The trees first: the gate's root, the front node, the doors; then the
    # pillar's root and cards (filled in below).
    tree_at = app.put(bytes(N.DOBJ_DESC_SIZE * 5))
    pillar_at = app.put(bytes(N.DOBJ_DESC_SIZE * PILLAR_ROWS))
    if pillar_at != PILLAR_TREE:
        raise GateError(f"pillar tree at {pillar_at:#x}, p4_articles.py names {PILLAR_TREE:#x}")
    wings_at = app.put(bytes(N.DOBJ_DESC_SIZE * p6_wings.TREE_ROWS + 4))
    if wings_at != WINGS_TREE:
        raise GateError(f"wings tree at {wings_at:#x}, p4_articles.py names {WINGS_TREE:#x}")
    doors = {}
    for side in ("L", "R"):
        mesh = meshes[side]
        faces = door_faces(model, mesh)
        used = []
        for tri in faces:
            for i in tri:
                if i not in used:
                    used.append(i)
        if len(used) > N.VERTEX_CACHE:
            raise GateError(f"{side} door: {len(used)} vertices, one load holds {N.VERTEX_CACHE}")
        pos = {i: np.array(model.skinned_position(mesh, i)) * scale for i in used}
        xs = [p[0] for p in pos.values()]
        hinge = max(xs) if side == "L" else min(xs)        # the outer edge
        blob = bytearray()
        for i in used:
            x, y, z = pos[i]
            u, v = mesh.pool.uvs[i]
            s = (u / U_CROP) * TEX * 32
            t = (1.0 - v) * TEX * 32
            # Light from above: the top of the door full, its foot a shade warmer.
            k = min(1.0, max(0.0, y / HEIGHT))
            rgba = (255, int(round(214 + 41 * k)), int(round(170 + 85 * k)), 255)
            blob += vtx(x - hinge, y, 0.0, s, t, rgba)
        doors[side] = {"hinge": hinge, "used": used, "faces": faces, "vtx": bytes(blob)}
    app.align(8)
    tex, pal, preview = image_ci4()
    tex_at = app.put(tex)
    app.align(8)
    pal_at = app.put(pal)
    lists = {}
    for side in ("L", "R"):
        d = doors[side]
        app.align(8)
        vtx_at = app.put(d["vtx"])
        tris = [tuple(d["used"].index(i) for i in tri) for tri in d["faces"]]
        cmds = door_list(tex_at, pal_at, vtx_at, len(d["used"]), tris, FILE_ID)
        blob, slots = N.encode_dl(cmds)
        app.align(8)
        at = app.put(blob)
        for rel, target in slots.items():
            app.slot(at + rel, target)
        lists[side] = at
    rows = [(0, None, (0.0, 0.0, 0.0)),
            (1, None, (0.0, 0.0, FRONT)),
            (2, lists["L"], (doors["L"]["hinge"], 0.0, 0.0)),
            (2, lists["R"], (doors["R"]["hinge"], 0.0, 0.0)),
            (DOBJ_END, None, (0.0, 0.0, 0.0))]
    for k, (ident, dl, t) in enumerate(rows):
        at = tree_at + k * N.DOBJ_DESC_SIZE
        struct.pack_into(">I", f.data, at, ident)
        if dl is not None:
            app.slot(at + 4, ("intern", FILE_ID, dl))
        struct.pack_into(">3f", f.data, at + 8, *t)
        struct.pack_into(">3f", f.data, at + 32, 1.0, 1.0, 1.0)
    pillar_lists = pillar(app)
    prow = [(0, None, (0.0, 0.0, 0.0))]
    prow += [(1, pillar_lists[card[0]], (0.0, 0.0, card[6])) for card in PILLAR_CARDS]
    prow.append((DOBJ_END, None, (0.0, 0.0, 0.0)))
    for k, (ident, dl, t) in enumerate(prow):
        at = pillar_at + k * N.DOBJ_DESC_SIZE
        struct.pack_into(">I", f.data, at, ident)
        if dl is not None:
            app.slot(at + 4, ("intern", FILE_ID, dl))
        struct.pack_into(">3f", f.data, at + 8, *t)
        struct.pack_into(">3f", f.data, at + 32, 1.0, 1.0, 1.0)
    wings = p6_wings.write(app, FILE_ID, wings_at)
    app.slot(WINGS_TABLE_SLOT, ("intern", FILE_ID, wings["table"]))
    info = {"tree": tree_at, "pillar": pillar_at, "lists": lists, "pillar_lists": pillar_lists,
            "wings": wings,
            "scale": scale, "hinges": {s: doors[s]["hinge"] for s in doors}, "preview": preview}
    return f.to_bytes(), info


def pillar_atlas() -> tuple[bytes, bytes]:
    """The pillar's atlas as CI4 texels and an RGBA5551 palette (index 0
    clear): look_parts.pillar_light_atlas, its palette padded to 16."""
    import look_parts
    idx, palette = look_parts.pillar_light_atlas(PILLAR_TEX)
    palette = list(palette) + [palette[-1]] * (16 - len(palette))
    return M.ci4_bytes(idx), M.rgba5551(palette)


def pillar(app: "M.Appender") -> dict:
    """The pillar's card lists: each a quad facing the camera (+z) in its
    node's plane, textured from the atlas, unlit; cut out and opaque, or
    translucent at its PRIM alpha."""
    texels, pal = pillar_atlas()
    app.align(8)
    tex_at = app.put(texels)
    app.align(8)
    pal_at = app.put(pal)
    line = PILLAR_TEX * 4 // 64
    mask = PILLAR_TEX.bit_length() - 1
    clamp = 2
    out = {}
    for name, (s0, t0, s1, t1), left, right, bottom, top, _z, alpha in PILLAR_CARDS:
        corners = [(left, top, s0, t0), (right, top, s1, t0),
                   (right, bottom, s1, t1), (left, bottom, s0, t1)]
        app.align(8)
        vtx_at = app.put(b"".join(vtx(x, y, 0.0, s * 32, t * 32) for x, y, s, t in corners))
        cmds = [N.Gfx(0xE7000000, 0),
                N.Gfx(0xD9000000 | (0xFFFFFF & ~(G_LIGHTING | G_CULL_BACK)), 0),
                N.Gfx(0xE3001001, 0x00008000)]
        cmds += CUTOUT_ON if alpha >= 255 else XLU_ON
        cmds += [PILLAR_COMBINE, N.Gfx(0xF9000000, 0),
                 N.Gfx(0xE7000000, 0), N.Gfx(0xFA000000, 0xFFFFFF00 | alpha), N.Gfx(0xE8000000, 0),
                 N.Gfx(0xF5000100, 0x05000000),
                 N.Gfx(0xF5500000, (7 << 24) | (mask << 14) | (mask << 4)),
                 N.Gfx(0xF5400000 | (line << 9), (clamp << 18) | (mask << 14) | (clamp << 8) | (mask << 4)),
                 N.Gfx(0xFD100000, pal_at // 4, ("intern", FILE_ID, pal_at)),
                 N.Gfx(0xE6000000, 0), N.Gfx(0xF0000000, 0x0503C000), N.Gfx(0xE7000000, 0),
                 N.Gfx(0xD7000002, 0xFFFFFFFF),
                 N.Gfx(0xF2000000, (((PILLAR_TEX - 1) << 2) << 12) | ((PILLAR_TEX - 1) << 2)),
                 N.Gfx(0xFD500000, tex_at // 4, ("intern", FILE_ID, tex_at)),
                 N.Gfx(0xE6000000, 0),
                 N.Gfx(0xF3000000, (7 << 24) | ((PILLAR_TEX * PILLAR_TEX // 4 - 1) << 12) | (2048 // line)),
                 N.Gfx(0xE7000000, 0),
                 N.Gfx(0x01000000 | (4 << 12) | (4 << 1), vtx_at, ("intern", FILE_ID, vtx_at)),
                 N.Gfx(0x06000000 | (0 << 16) | (2 << 8) | 4, (0 << 16) | (4 << 8) | 6),
                 N.Gfx(0xE7000000, 0)]
        cmds += CUTOUT_OFF
        cmds += [N.Gfx(0xE3001001, 0x00000000),
                 N.Gfx(0xD9FFFFFF, G_LIGHTING | G_CULL_BACK),
                 N.Gfx(0xDF000000, 0)]
        blob, slots = N.encode_dl(cmds)
        app.align(8)
        at = app.put(blob)
        for rel, target in slots.items():
            app.slot(at + rel, target)
        out[name] = at
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--preview", type=Path)
    args = ap.parse_args()
    blob, info = build()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(blob)
    if args.preview:
        from PIL import Image
        Image.fromarray(info["preview"]).resize((TEX * 4, TEX * 4), Image.NEAREST).save(args.preview)
    w = info["wings"]
    print(f"p6 wings: tree {w['tree']:#x}, table {w['table']:#x}, {w['bytes']} bytes, mesh {w['mesh']}, clips {w['clips']}")
    print(f"p6 gate: {args.out} {len(blob)} bytes, tree {info['tree']:#x}, pillar {info['pillar']:#x}, scale {info['scale']:.2f}, "
          f"hinges L {info['hinges']['L']:.0f} R {info['hinges']['R']:.0f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
