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
  to the camera, the centre there (since owner 2026-10-22 Ultimate's ring
  and dot, backlight00; ORB_STYLES), the left wing's card (Ultimate's
  backlight01, three feather strokes of light fanning out from beside the
  centre), the right wing's (the left's mirror), and the two halves of a
  ring: style B's crisp ring and dot over the centre's glow, or the
  taunt's ring about her head (a soft ring drawn here; no window shows it
  since owner p7h).

Each image is an I4 coverage mask (the DS's A5I3: graded alpha, its colour
the card's PRIM), scaled down from Ultimate's effect textures in the owner's
local rip (decomp/alt_assets/.../Palutenas Effects; never committed, P6 D1,
like her voices); the cards are translucent and unlit: colour PRIM, alpha
TEXEL0 * PRIM (the combine the entry-effect compiler takes for I4).

The light's cards take their PRIM from a material (owner p6ac, 2026-10-17:
the wings "not lasting as long as they should, or the fade out isn't
gradual enough"): each card's DObj has one MObj (MOBJ_FLAG_PRIMCOLOR, its
MObjSub here, the tree's MObjSub*** table at LIGHT_MOBJSUBS) and its list
calls segment E where the PRIM was, so her code fades a card by its live
MObj's alpha, as Ultimate's backlight fades.
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
    # The core 25% bigger and brighter than first made (owner p6ac).
    ("core", "core", 80.0, (255, 255, 255, 255)),
)
GLOW_ROWS = len(GLOW_CARDS) + 2
# The light's cards, all in one plane her code turns to the camera (owner
# p6ab, 2026-10-17: the 3DS forward smash's wind-up, Strong_Side_A frames
# 0-10: "camera facing", "the 2 light wings separated by the small glowing
# circle"). The orb: since owner 2026-10-22 Ultimate's centre
# (ef_palutena_backlight00, the left half of a thin ring round a dot that
# sits low in it, mirrored whole: its colour channels the crisp ring and
# dot, its alpha a soft glow round them that also fills the gap under the
# dot), the style's `orb_half` from its centre to the card's edge, its
# centre the tree's origin; ORB_STYLE picks how it is drawn (ORB_STYLES).
# The wings' cards: the image's
# anchor (where its three strokes meet, in texels over its side) sits on the
# node's origin, the card the style's `wing_size` across and high (A the
# 3DS's, WING_W x WING_H, wider than Ultimate's image; B Ultimate's); the
# right wing mirrors the left about x. The
# ring's halves: the left half of a ring, the style's `ring_radius` across
# and twice that high, mirrored -- the taunt's ring (no window shows it since
# owner p7h), or style B's crisp core.
ORB_HALF = 115.0
# The centre's styles (owner 2026-10-22: "fix the centre ... same/similar
# color as current wings", and an alternate "more ultimate-like within DS
# constraints"). Ultimate draws the centre twice: once translucent (teal)
# and once additively (pale mint where the texture is bright, teal on its
# soft edge), so at full it is a near-white ring and dot in a teal glow, and
# as it fades it goes teal, then blue. The DS draws one translucent colour
# per card, nothing additive.
#   ring (A): one card, the ring and dot crisp with a faint glow of the same
#     colour (`glow` of the alpha), the wings' cyan; the ring halves stay
#     the taunt's (hidden).
#   ultimate (B): two layers -- the orb card the soft glow (the alpha,
#     blurred `blur` texels of 256), teal; the ring halves over it the crisp
#     ring and dot at twice the texels (each half 32x64), near-white mint --
#     and both colours move as the light fades: at full, at half faded
#     (`mid`), faded out (`late`), as Ultimate's colour keys do. Her code
#     reads the ramp from each card's material (nds_p4_palutena.c): blend
#     colour the full colour and alpha, light 1 the half-faded colour, env
#     the faded-out colour; a ring half whose blend alpha is not 0 is the
#     core and shows with the light. Its wings (owner 2026-10-22, "the
#     colors of the wings too", "match the wing size too") as Ultimate's:
#     near-white cyan at full fading to cyan, then blue (Ultimate's strokes
#     are white-hot cores with cyan edges, one colour per card here), and
#     Ultimate's size about its centre, 4.16 x 3.19 ring radii (its wing
#     mesh, x 1.21-12.86 and y -6.11-2.81 about a ring of radius 2.8).
# Colours: (full RGBA, mid RGB, late RGB).
WING_COLOUR = ((100, 230, 255, 255), (100, 230, 255), (100, 230, 255))
TAUNT_RING_COLOUR = ((190, 242, 255, 230), (190, 242, 255), (190, 242, 255))
ORB_STYLES = {
    "ring": {"glow": 0.3, "orb_half": ORB_HALF, "ring_radius": 170.0, "core": False,
             "orb": ((100, 230, 255, 240), (100, 230, 255), (100, 230, 255)),
             "ring": TAUNT_RING_COLOUR},
    "ultimate": {"blur": 3.0, "orb_half": ORB_HALF * 1.1, "orb_fill": 1.0 / 1.1, "ring_radius": ORB_HALF,
                 "core": True,
                 "orb": ((40, 196, 190, 215), (0, 150, 178), (0, 64, 118)),
                 "ring": ((232, 255, 250, 255), (60, 220, 255), (0, 104, 180)),
                 "wing": ((160, 240, 255, 255), (30, 205, 255), (0, 130, 250)), "wing_size": (385.0, 295.0)},
}
# Owner 2026-10-22: B.
ORB_STYLE = "ultimate"
WING_W = 358.0
WING_H = 244.0
WING_ANCHOR = (0.84, 0.47)
# The tree's order is the draw order, and the first card drawn shows on top
# where cards overlap (each draws as its own entry root with the same
# polygon ID; the DS does not draw a translucent pixel over one of the same
# ID): the wings, then style B's crisp ring and dot, the orb's glow last, so
# the glow fills only round them (owner 2026-10-22: the core vanished under
# the glow once both faded). Her code walks the cards in this order
# (nds_p4_palutena.c).
LIGHT_CARDS = (
    # name, image (its colours: card_colours)
    ("wing_l", "wing"),
    ("wing_r", "wing"),
    ("ring_l", "ring"),
    ("ring_r", "ring"),
    ("orb", "orb"),
)


def card_colours(name: str) -> tuple:
    """(full RGBA, mid RGB, late RGB) of a light card in ORB_STYLE."""
    style = ORB_STYLES[ORB_STYLE]
    if name.startswith("wing"):
        return style.get("wing", WING_COLOUR)
    return style["orb"] if name == "orb" else style["ring"]


LIGHT_ROWS = len(LIGHT_CARDS) + 2
# The tree's DObjs (its root, then each card) as gcAddMObjAll walks them:
# one MObjSub*** entry each, the root's NULL.
LIGHT_DOBJS = len(LIGHT_CARDS) + 1
# MObjSub (sys/objtypes.h): flags at 0x30 (MOBJ_FLAG_PRIMCOLOR), primcolor
# (RGBA8888) at 0x50; scau/scav at 0x1C/0x20 one, as a fighter's are.
MOBJSUB_SIZE = 0x78
MOBJ_FLAG_PRIMCOLOR = 0x0200
SEGMENT_E_SLOT0 = 0x0E000000
IMAGES = {
    # name: (Ultimate's texture (its alpha), width, height)
    "glow": ("ef_palutena_flash01.png", 32, 32),
    "core": ("ef_palutena_flash01.png", 32, 32),
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


def _centre_halves() -> tuple:
    """backlight00's left half (256 high, 128 wide): its colour channels (the
    crisp ring and dot, 0-1) and its alpha (the soft glow round them)."""
    from PIL import Image
    path = FX / "ef_palutena_backlight00.png"
    if not path.exists():
        raise LightError(f"{path}: Ultimate's effect texture (the owner's local rip) is missing")
    rgba = np.asarray(Image.open(path).convert("RGBA"), dtype=np.float32) / 255.0
    return rgba[:, :, :3].mean(axis=2), rgba[:, :, 3]


def _whole(half: np.ndarray) -> np.ndarray:
    return np.concatenate([half, half[:, ::-1]], axis=1)


def _fit(a: np.ndarray, fill: float) -> np.ndarray:
    """`a` (square) shrunk to `fill` of itself about its centre, the rest clear."""
    from PIL import Image
    if fill >= 1.0:
        return a
    n = a.shape[0]
    size = int(round(n * fill))
    small = Image.fromarray(a.astype(np.float32), "F").resize((size, size), Image.BOX)
    out = np.zeros_like(a)
    at = (n - size) // 2
    out[at:at + size, at:at + size] = np.asarray(small)
    return out


def _orb_coverage(style: dict) -> np.ndarray:
    """The orb card's coverage, 256x256: A the crisp ring and dot with a faint
    glow; B the glow alone, blurred."""
    from PIL import Image, ImageFilter
    core, glow = (_whole(h) for h in _centre_halves())
    if style["core"]:
        img = Image.fromarray((glow * 255.0).astype(np.uint8), "L")
        a = np.asarray(img.filter(ImageFilter.GaussianBlur(style["blur"])), dtype=np.float32) / 255.0
        a = a / max(float(a.max()), 1e-6)
    else:
        a = np.maximum(core, glow * style["glow"])
    return _fit(a, style.get("orb_fill", 1.0))


def image_i4(kind: str) -> tuple[np.ndarray, int, int]:
    """The image as 4-bit coverage levels (rows of 0-15) and its size."""
    from PIL import Image
    source, w, h = IMAGES[kind]
    centre = kind == "orb" or (kind == "ring" and ORB_STYLES[ORB_STYLE]["core"])
    if kind == "ring" and centre:
        # Style B's crisp ring and dot: backlight00's left half itself.
        a = _centre_halves()[0]
    elif kind == "ring":
        # The left half of a ring whose centre is the image's right edge,
        # middle row (the card's origin): a soft band about RING_AT.
        ys, xs = np.mgrid[0:256, 0:128].astype(np.float32)
        r = np.hypot((xs + 0.5 - 128.0) / 128.0, (ys + 0.5 - 128.0) / 128.0)
        a = np.clip(1.0 - np.abs(r - RING_AT) / RING_BAND, 0.0, 1.0) ** 1.5
    elif kind == "orb":
        a = _orb_coverage(ORB_STYLES[ORB_STYLE])
    else:
        a = _alpha(source)
    if kind in ("glow", "core"):
        # flash01 is a quarter of a disc (its centre the bottom-right
        # corner): mirrored to the whole disc, its peak made full and its
        # falloff widened (gamma 0.6) so a small card still reads as a glow.
        a = np.concatenate([a, a[:, ::-1]], axis=1)
        a = np.concatenate([a, a[::-1, :]], axis=0)
        a = a / max(float(a.max()), 1e-6)
        a = a ** 0.6
        if kind == "core":
            # The core 25% brighter (owner p6ac): its coverage, white PRIM.
            a = a * 1.25
    if kind == "wing":
        # The 3DS's strokes are solid light: Ultimate's soft edges firmed
        # (gamma 0.6) so each stroke reads as a bar at the DS's size.
        a = a / max(float(a.max()), 1e-6)
        a = a ** 0.6
    img = Image.fromarray((np.clip(a, 0.0, 1.0) * 255.0).astype(np.uint8), "L")
    # The centre's crisp edges by area (no ringing about the thin ring).
    img = img.resize((w, h), Image.BOX if centre else Image.LANCZOS)
    levels = np.rint(np.asarray(img, dtype=np.float32) / 255.0 * 15.0).astype(int)
    # No faint haze over the card: the lowest level clear, so a card's
    # rectangle never shows.
    levels[levels <= 1] = 0
    if kind in ("glow", "core", "orb"):
        # The rim fully clear: no square edge where the card ends.
        levels[0, :] = levels[-1, :] = 0
        levels[:, 0] = levels[:, -1] = 0
    elif centre:
        # The core's half: clear but for its right edge (the ring's middle).
        levels[0, :] = levels[-1, :] = 0
        levels[:, 0] = 0
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


def _card_list(fid: int, tex_at: int, w: int, h: int, vtx_at: int, prim: tuple | None,
               quads: int = 1) -> list:
    """One quad (four vertices at vtx_at; or `quads` of them, four vertices
    each), its I4 image loaded as the N64 loads a 4-bit block (16-bit load
    tile), translucent at PRIM (or, prim None, at its DObj's material: a call
    to segment E's first slot)."""
    line = max(1, w * 4 // 64)                   # 4-bit row in 64-bit words
    masks, maskt = w.bit_length() - 1, h.bit_length() - 1
    clamp = 2
    if prim is None:
        colour = N.Gfx(0xDE000000, SEGMENT_E_SLOT0)
    else:
        r, g, b, a = prim
        colour = N.Gfx(0xFA000000, (r << 24) | (g << 16) | (b << 8) | a)
    return [N.Gfx(0xE7000000, 0),
            N.Gfx(0xD9000000 | (0xFFFFFF & ~(G_LIGHTING | G_CULL_BACK)), 0),
            N.Gfx(0xE3001001, 0x00000000)] + XLU_ON + [
            I4_COMBINE, N.Gfx(0xF9000000, 0),
            N.Gfx(0xE7000000, 0), colour,
            N.Gfx(0xE8000000, 0),
            N.Gfx(0xF5900000, (7 << 24) | (maskt << 14) | (masks << 4)),
            N.Gfx(0xF5800000 | (line << 9), (clamp << 18) | (maskt << 14) | (clamp << 8) | (masks << 4)),
            N.Gfx(0xD7000002, 0xFFFFFFFF),
            N.Gfx(0xF2000000, (((w - 1) << 2) << 12) | ((h - 1) << 2)),
            N.Gfx(0xFD900000, tex_at // 4, ("intern", fid, tex_at)),
            N.Gfx(0xE6000000, 0),
            N.Gfx(0xF3000000, (7 << 24) | (((w * h + 3) // 4 - 1) << 12) | (2048 // line)),
            N.Gfx(0xE7000000, 0),
            N.Gfx(0x01000000 | (4 * quads << 12) | (4 * quads << 1), vtx_at, ("intern", fid, vtx_at))] + [
            N.Gfx(0x06000000 | (8 * q << 16) | ((8 * q + 2) << 8) | (8 * q + 4),
                  (8 * q << 16) | ((8 * q + 4) << 8) | (8 * q + 6)) for q in range(quads)] + [
            N.Gfx(0xE7000000, 0)] + CUTOUT_OFF + [
            N.Gfx(0xE3001001, 0x00000000),
            N.Gfx(0xD9FFFFFF, G_LIGHTING | G_CULL_BACK),
            N.Gfx(0xDF000000, 0)]


def _corners(kind: str, name: str, w: int, h: int, half: float = 0.0) -> list:
    """The card's corners (x, y, s, t texels), top left first, clockwise."""
    if kind == "orb":
        half = ORB_STYLES[ORB_STYLE]["orb_half"]
    if kind in ("glow", "orb"):
        return [(-half, half, 0, 0), (half, half, w, 0), (half, -half, w, h), (-half, -half, 0, h)]
    if kind == "wing":
        ax, ay = WING_ANCHOR
        wing_w, wing_h = ORB_STYLES[ORB_STYLE].get("wing_size", (WING_W, WING_H))
        left, right = -ax * wing_w, (1.0 - ax) * wing_w
        top, bottom = ay * wing_h, -(1.0 - ay) * wing_h
    else:
        radius = ORB_STYLES[ORB_STYLE]["ring_radius"]
        left, right, top, bottom = -radius, 0.0, radius, -radius
    c = [(left, top, 0, 0), (right, top, w, 0), (right, bottom, w, h), (left, bottom, 0, h)]
    if name.endswith("_r"):
        # The right half: the left's mirror about x.
        c = [(-x, y, s, t) for x, y, s, t in c]
    return c


def _mobjsub(colours: tuple, core: bool = True) -> bytes:
    """A material that sets only PRIM (MOBJ_FLAG_PRIMCOLOR), in the file's
    big-endian layout (the loader word-swaps it; attachment restores the
    u16/u8 lanes, ndsRelocCopyMObjSubForAttachment). The colours the flag
    leaves unread carry her code's ramp: blend (0x5C) the full colour and
    alpha (alpha 0: not drawn by the ramp; a ring half then the taunt's),
    light 1 (0x60) the colour half faded, env (0x58) faded out."""
    full, mid, late = colours
    sub = bytearray(MOBJSUB_SIZE)
    struct.pack_into(">ff", sub, 0x1C, 1.0, 1.0)
    struct.pack_into(">H", sub, 0x30, MOBJ_FLAG_PRIMCOLOR)
    sub[0x50:0x54] = bytes(full)
    sub[0x58:0x5C] = bytes(late) + b"\xff"
    sub[0x5C:0x60] = bytes(full) if core else bytes(full[:3]) + b"\x00"
    sub[0x60:0x64] = bytes(mid) + b"\xff"
    return bytes(sub)


def write(app, fid: int, glow_at: int, light_at: int, mobj_at: int) -> dict:
    """The images, the cards' lists, both trees' rows (their space already
    allocated at glow_at and light_at) and the light's materials (its
    MObjSub*** table allocated at mobj_at)."""
    images = {}
    for kind in IMAGES:
        levels, w, h = image_i4(kind)
        app.align(8)
        images[kind] = (app.put(_i4_bytes(levels)), w, h)
    lists = {}

    def card(name: str, kind: str, prim: tuple | None, half: float = 0.0) -> int:
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
    for name, kind in LIGHT_CARDS:
        lists[name] = card(name, kind, None)
    # The light's materials: entry 0 (the root) NULL, then per card a
    # NULL-ended list naming its MObjSub.
    subs = {}
    for k, (name, _kind) in enumerate(LIGHT_CARDS):
        app.align(8)
        # A ring half is the core only in a style that draws one there.
        core = not name.startswith("ring") or ORB_STYLES[ORB_STYLE]["core"]
        sub_at = app.put(_mobjsub(card_colours(name), core))
        list_at = app.put(bytes(8))
        app.slot(list_at, ("intern", fid, sub_at))
        app.slot(mobj_at + 4 * (k + 1), ("intern", fid, list_at))
        subs[name] = sub_at

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
    return {"glow": glow_at, "light": light_at, "mobjsubs": mobj_at, "subs": subs, "lists": lists,
            "images": {k: (v[1], v[2]) for k, v in images.items()}}
