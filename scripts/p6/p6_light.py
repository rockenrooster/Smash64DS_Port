#!/usr/bin/env python3
"""P6: Palutena's light effects (owner p6x VFX plan, docs/P6/characters/
palutena.md "VFX plan"; the owner's picks 2026-10-16: Ultimate's cyan-white,
the light wings about her height, the taunt with the wings and the ring).

Two DObjDesc trees in her special file 2 after the wings' (p6_gate.py
GLOW_TREE, LIGHT_TREE; scripts/p4/p4_articles.py "glow", "light"), drawn
natively as entry effects, placed and sized every tic by her code
(src/port/nds_p4_palutena.c):

- The gem's glow: a root her code puts at her staff's gem and two cards
  facing the camera, a soft cyan halo and a white core.
- The overhead light wings: a root her code puts above her head and turns
  to the camera, the small glowing orb there, the left wing's card
  (Ultimate's backlight01, three feather strokes of light fanning out from
  beside the orb), the right wing's (the left's mirror), and the two halves
  of the ring about her head for her taunt (a soft ring drawn here:
  Ultimate's backlight00 and 03 carry a filled inner glow that drew as a
  sheet of light over the stage).

Each image is an I4 coverage mask (the DS's A5I3: graded alpha, its colour
the card's PRIM), scaled down from Ultimate's effect textures in the owner's
local rip (decomp/alt_assets/.../Palutenas Effects; never committed, P6 D1,
like her voices); the cards are translucent and unlit: colour PRIM, alpha
TEXEL0 * PRIM (the combine the entry-effect compiler takes for I4).
"""
from __future__ import annotations

import struct
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]

import n64_model as N  # noqa: E402

FX = (REPO / "decomp" / "alt_assets" / "Fighters" / "Super Smash Bros Ultimate" / "Fighters" /
      "Palutenas Effects")

# Rows of each tree: the root, its cards, the end (p6_gate.py allocates them).
GLOW_CARDS = (
    # name, image, half size, PRIM (r, g, b, a)
    ("halo", "glow", 160.0, (150, 228, 255, 190)),
    ("core", "glow", 64.0, (255, 255, 255, 255)),
)
GLOW_ROWS = len(GLOW_CARDS) + 2
# The light's cards, all in one plane her code turns to the camera (owner
# p6ab, 2026-10-17: the 3DS forward smash's wind-up, Strong_Side_A frames
# 0-10: "camera facing", "the 2 light wings separated by the small glowing
# circle"). The orb: a soft white disc ORB_HALF from its centre to the
# card's edge, its centre the tree's origin. The wings' cards: the image's
# anchor (where its three strokes meet, in texels over its side) sits on the
# node's origin, the card WING_W across and WING_H high (the 3DS's wing is
# wider than Ultimate's image); the right wing mirrors the left about x. The
# ring's halves: the left half of the ring, RING_RADIUS across and twice
# that high, mirrored.
ORB_HALF = 115.0
ORB_CORE = 0.6
WING_W = 358.0
WING_H = 244.0
WING_ANCHOR = (0.84, 0.47)
RING_RADIUS = 170.0
LIGHT_CARDS = (
    ("orb", "orb", (235, 250, 255, 240)),
    ("wing_l", "wing", (100, 230, 255, 255)),
    ("wing_r", "wing", (100, 230, 255, 255)),
    ("ring_l", "ring", (190, 242, 255, 230)),
    ("ring_r", "ring", (190, 242, 255, 230)),
)
LIGHT_ROWS = len(LIGHT_CARDS) + 2
IMAGES = {
    # name: (Ultimate's texture (its alpha), width, height)
    "glow": ("ef_palutena_flash01.png", 32, 32),
    "orb": (None, 32, 32),
    "wing": ("ef_palutena_backlight01.png", 64, 64),
    "ring": (None, 32, 64),
}
# The ring: its left half, a band about RING_BAND of the radius wide at
# RING_AT, soft both sides.
RING_AT = 0.78
RING_BAND = 0.16

# The pillar's own states (p6_gate.py), restated for these lists.
G_LIGHTING = 0x00020000
G_CULL_BACK = 0x00000400
XLU_ON = [N.Gfx(0xE2001E01, 0x00000000), N.Gfx(0xE200001C, 0x00504A50)]
CUTOUT_OFF = [N.Gfx(0xE2001E01, 0x00000000), N.Gfx(0xE200001C, 0xC4112078)]
# Colour (0 - 0) * 0 + PRIM, alpha (TEXEL0 - 0) * PRIM + 0, both cycles: the
# entry-effect compiler's I4 coverage combine (generate_nds_entry_effects.py).
I4_COMBINE = N.Gfx(0xFCFF97FF, 0xFF2DFEFF)


class LightError(SystemExit):
    pass


def _alpha(name: str) -> np.ndarray:
    from PIL import Image
    path = FX / name
    if not path.exists():
        raise LightError(f"{path}: Ultimate's effect texture (the owner's local rip) is missing")
    return np.asarray(Image.open(path).convert("RGBA"), dtype=np.float32)[:, :, 3] / 255.0


def image_i4(kind: str) -> tuple[np.ndarray, int, int]:
    """The image as 4-bit coverage levels (rows of 0-15) and its size."""
    from PIL import Image
    source, w, h = IMAGES[kind]
    if kind == "ring":
        # The left half of a ring whose centre is the image's right edge,
        # middle row (the card's origin): a soft band about RING_AT.
        ys, xs = np.mgrid[0:256, 0:128].astype(np.float32)
        r = np.hypot((xs + 0.5 - 128.0) / 128.0, (ys + 0.5 - 128.0) / 128.0)
        a = np.clip(1.0 - np.abs(r - RING_AT) / RING_BAND, 0.0, 1.0) ** 1.5
    elif kind == "orb":
        # A disc full to ORB_CORE of its radius, soft to its edge.
        ys, xs = np.mgrid[0:256, 0:256].astype(np.float32)
        r = np.hypot((xs + 0.5 - 128.0) / 128.0, (ys + 0.5 - 128.0) / 128.0)
        a = np.clip((1.0 - r) / (1.0 - ORB_CORE), 0.0, 1.0) ** 1.5
    else:
        a = _alpha(source)
    if kind == "glow":
        # flash01 is a quarter of a disc (its centre the bottom-right
        # corner): mirrored to the whole disc, its peak made full and its
        # falloff widened (gamma 0.6) so a small card still reads as a glow.
        a = np.concatenate([a, a[:, ::-1]], axis=1)
        a = np.concatenate([a, a[::-1, :]], axis=0)
        a = a / max(float(a.max()), 1e-6)
        a = a ** 0.6
    if kind == "wing":
        # The 3DS's strokes are solid light: Ultimate's soft edges firmed
        # (gamma 0.6) so each stroke reads as a bar at the DS's size.
        a = a / max(float(a.max()), 1e-6)
        a = a ** 0.6
    img = Image.fromarray((np.clip(a, 0.0, 1.0) * 255.0).astype(np.uint8), "L")
    img = img.resize((w, h), Image.LANCZOS)
    levels = np.rint(np.asarray(img, dtype=np.float32) / 255.0 * 15.0).astype(int)
    # No faint haze over the card: the lowest level clear, so a card's
    # rectangle never shows.
    levels[levels <= 1] = 0
    if kind in ("glow", "orb"):
        # The rim fully clear: no square edge where the card ends.
        levels[0, :] = levels[-1, :] = 0
        levels[:, 0] = levels[:, -1] = 0
    return np.clip(levels, 0, 15), w, h


def _i4_bytes(levels: np.ndarray) -> bytes:
    out = bytearray()
    for row in levels:
        for a, b in zip(row[0::2], row[1::2]):
            out.append(((int(a) & 15) << 4) | (int(b) & 15))
    return bytes(out)


def _vtx(x: float, y: float, s: float, t: float) -> bytes:
    xs, ys = int(round(x)), int(round(y))
    for c in (xs, ys):
        if not -0x8000 <= c <= 0x7FFF:
            raise LightError(f"vertex ({x}, {y}) overflows s16")
    return struct.pack(">hhhHhh", xs, ys, 0, 0, int(round(s)), int(round(t))) + bytes((255, 255, 255, 255))


def _card_list(fid: int, tex_at: int, w: int, h: int, vtx_at: int, prim: tuple) -> list:
    """One quad (four vertices at vtx_at), its I4 image loaded as the N64
    loads a 4-bit block (16-bit load tile), translucent at PRIM."""
    line = max(1, w * 4 // 64)                   # 4-bit row in 64-bit words
    masks, maskt = w.bit_length() - 1, h.bit_length() - 1
    clamp = 2
    r, g, b, a = prim
    return [N.Gfx(0xE7000000, 0),
            N.Gfx(0xD9000000 | (0xFFFFFF & ~(G_LIGHTING | G_CULL_BACK)), 0),
            N.Gfx(0xE3001001, 0x00000000)] + XLU_ON + [
            I4_COMBINE, N.Gfx(0xF9000000, 0),
            N.Gfx(0xE7000000, 0), N.Gfx(0xFA000000, (r << 24) | (g << 16) | (b << 8) | a),
            N.Gfx(0xE8000000, 0),
            N.Gfx(0xF5900000, (7 << 24) | (maskt << 14) | (masks << 4)),
            N.Gfx(0xF5800000 | (line << 9), (clamp << 18) | (maskt << 14) | (clamp << 8) | (masks << 4)),
            N.Gfx(0xD7000002, 0xFFFFFFFF),
            N.Gfx(0xF2000000, (((w - 1) << 2) << 12) | ((h - 1) << 2)),
            N.Gfx(0xFD900000, tex_at // 4, ("intern", fid, tex_at)),
            N.Gfx(0xE6000000, 0),
            N.Gfx(0xF3000000, (7 << 24) | (((w * h + 3) // 4 - 1) << 12) | (2048 // line)),
            N.Gfx(0xE7000000, 0),
            N.Gfx(0x01000000 | (4 << 12) | (4 << 1), vtx_at, ("intern", fid, vtx_at)),
            N.Gfx(0x06000000 | (0 << 16) | (2 << 8) | 4, (0 << 16) | (4 << 8) | 6),
            N.Gfx(0xE7000000, 0)] + CUTOUT_OFF + [
            N.Gfx(0xE3001001, 0x00000000),
            N.Gfx(0xD9FFFFFF, G_LIGHTING | G_CULL_BACK),
            N.Gfx(0xDF000000, 0)]


def _corners(kind: str, name: str, w: int, h: int, half: float = 0.0) -> list:
    """The card's corners (x, y, s, t texels), top left first, clockwise."""
    if kind == "orb":
        half = ORB_HALF
    if kind in ("glow", "orb"):
        return [(-half, half, 0, 0), (half, half, w, 0), (half, -half, w, h), (-half, -half, 0, h)]
    if kind == "wing":
        ax, ay = WING_ANCHOR
        left, right = -ax * WING_W, (1.0 - ax) * WING_W
        top, bottom = ay * WING_H, -(1.0 - ay) * WING_H
    else:
        left, right, top, bottom = -RING_RADIUS, 0.0, RING_RADIUS, -RING_RADIUS
    c = [(left, top, 0, 0), (right, top, w, 0), (right, bottom, w, h), (left, bottom, 0, h)]
    if name.endswith("_r"):
        # The right half: the left's mirror about x.
        c = [(-x, y, s, t) for x, y, s, t in c]
    return c


def write(app, fid: int, glow_at: int, light_at: int) -> dict:
    """The images, the cards' lists and both trees' rows (their space
    already allocated at glow_at and light_at)."""
    images = {}
    for kind in IMAGES:
        levels, w, h = image_i4(kind)
        app.align(8)
        images[kind] = (app.put(_i4_bytes(levels)), w, h)
    lists = {}

    def card(name: str, kind: str, prim: tuple, half: float = 0.0) -> int:
        tex_at, w, h = images[kind]
        app.align(8)
        vtx_at = app.put(b"".join(_vtx(x, y, s * 32, t * 32) for x, y, s, t in _corners(kind, name, w, h, half)))
        blob, slots = N.encode_dl(_card_list(fid, tex_at, w, h, vtx_at, prim))
        app.align(8)
        at = app.put(blob)
        for rel, target in slots.items():
            app.slot(at + rel, target)
        return at

    for name, kind, half, prim in GLOW_CARDS:
        lists[name] = card(name, kind, prim, half)
    for name, kind, prim in LIGHT_CARDS:
        lists[name] = card(name, kind, prim)

    def rows(at: int, names: list) -> None:
        tree = [(0, None)] + [(1, lists[n]) for n in names] + [(N.JOINT_TREE_END, None)]
        for k, (ident, dl) in enumerate(tree):
            row = at + k * N.DOBJ_DESC_SIZE
            struct.pack_into(">I", app.f.data, row, ident)
            if dl is not None:
                app.slot(row + 4, ("intern", fid, dl))
            struct.pack_into(">3f", app.f.data, row + 8, 0.0, 0.0, 0.0)
            struct.pack_into(">3f", app.f.data, row + 32, 1.0, 1.0, 1.0)

    rows(glow_at, [c[0] for c in GLOW_CARDS])
    rows(light_at, [c[0] for c in LIGHT_CARDS])
    return {"glow": glow_at, "light": light_at, "lists": lists,
            "images": {k: (v[1], v[2]) for k, v in images.items()}}
