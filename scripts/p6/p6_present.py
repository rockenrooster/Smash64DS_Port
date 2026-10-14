#!/usr/bin/env python3
"""P6 A9: an authored content's select art -- portrait, name and series logo
-- appended to the select screen's donor files.

p4_css.py copies Remix's portrait (0xA05), name (0x11) and logo (0x14) files
into the build and points each cell and fighter row at offsets in them; an
authored content shows its base's until this appends its own:

- portrait: a 32x32 RGBA16 sprite block in Remix's layout (texels, bitmap,
  sprite: 0x860 bytes) and its white flash right after it (the select reads
  the flash at the portrait + 0x860). The face is the content's local render
  (`builds/p6-art/<name>/portrait.png`, scripts/p6/blender/portrait.py: the
  look is kit-derived, so the render is too, D10) framed as Remix frames its
  portraits: a 30x30 picture with a one-texel frame over a dark gradient;
  the flash keeps the face and lights the background. Without the render the
  base's portrait stays.
- name: the select's IA8 name sprite composed from the letters of Remix's own
  name sprites (the same font), so nothing new is drawn.
- logo: a series emblem the content names (`LOGOS`): an existing one by
  Remix's name (Fire Emblem), or an authored one drawn here (Kid Icarus).

Everything appended derives from the user's ROM or the local render: build
output only.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
import n64_model as N  # noqa: E402

PORTRAIT_BLOCK = 0x860
SPRITE_BYTES = 0x44
BITMAP_BYTES = 0x10
SP_TEXSHUF = 0x0200
# The name each authored content's sprite spells.
NAMES = {"ike": "IKE", "palutena": "PALUTENA"}
# The series logo: Remix's by its CharacterSelect.asm name, or "authored:<key>".
LOGOS = {"ike": "FIRE_EMBLEM", "palutena": "authored:kid_icarus"}
# Remix's name sprites to cut letters from (each shows exactly its constant's
# name and its letters' bright cores stand apart): the first that has a
# letter gives it.
LETTER_SOURCES = ("PEACH", "LUIGI", "LINK", "LANKY", "MEWTWO", "CONKER", "SONIC", "SHEIK", "CRASH", "FOX",
                  "WOLF", "GOEMON", "PEPPY", "SLIPPY", "DEDEDE")
# Remix's portrait frame (its edge texels) and background, from its own
# portraits (top black to dark red to orange; the flash's white to grey).
FRAME = {"top": (82, 57, 41), "left": (90, 66, 49), "bottom": (90, 66, 49), "right": (66, 49, 33)}
BG = ((0, 0, 0), (90, 0, 16), (173, 90, 0))
BG_FLASH = ((255, 255, 255), (206, 206, 206), (239, 239, 239))
# The widest name the select card holds (CSS_GATE_BOX in generate_mn_ui_kit.py:
# the card is 66 wide and vanilla's DONKEY KONG fills it).
NAME_MAX_WIDTH = 66
# Brighter than this is a letter's core, not its outline.
CORE = 170
# Brighter than this is a letter's body (its core or lit edge), not its shadow.
BODY = 80


class PresentError(SystemExit):
    pass


# ------------------------------------------------------------------ sprites

def _swizzle(rows: list[bytes], shuffled: bool) -> bytes:
    """N64 LoadBlock order: each 8-byte granule of an odd row has its halves
    exchanged (the bake's deswizzle_row, for 4-, 8- and 16-bit texels)."""
    out = bytearray()
    for y, row in enumerate(rows):
        row = bytes(row) + b"\x00" * ((-len(row)) % 8)
        if shuffled and (y & 1):
            row = b"".join(row[k + 4:k + 8] + row[k:k + 4] for k in range(0, len(row), 8))
        out += row
    return bytes(out)


def _rgba5551(rgba: np.ndarray) -> list[bytes]:
    rows = []
    for row in rgba:
        b = bytearray()
        for r, g, bl, a in row:
            b += struct.pack(">H", ((int(r) >> 3) << 11) | ((int(g) >> 3) << 6) | ((int(bl) >> 3) << 1) |
                             (1 if a >= 128 else 0))
        rows.append(bytes(b))
    return rows


def _ia8(intensity: np.ndarray, alpha: np.ndarray, width_img: int) -> list[bytes]:
    rows = []
    for yi in range(intensity.shape[0]):
        b = bytearray(width_img)
        for x in range(intensity.shape[1]):
            b[x] = ((int(intensity[yi, x]) >> 4) << 4) | (int(alpha[yi, x]) >> 4)
        rows.append(bytes(b))
    return rows


def _i4(intensity: np.ndarray) -> list[bytes]:
    rows = []
    for row in intensity:
        b = bytearray()
        for x in range(0, len(row), 2):
            b.append(((int(row[x]) >> 4) << 4) | (int(row[x + 1]) >> 4))
        rows.append(bytes(b))
    return rows


def append_sprite(f: N.RelocFile, template: int, texels: bytes, width: int, width_img: int, height: int,
                  pad_to: int | None = None) -> int:
    """Texels, a bitmap and a sprite copied from the sprite at `template`
    with this size, appended to `f`; returns the sprite's offset. `pad_to`
    pads the block (texels to the sprite's end) to that many bytes."""
    start = len(f.data) + ((-len(f.data)) % 8)
    f.data += bytes(start - len(f.data))
    buf = start
    f.data += texels
    f.data += bytes((-len(f.data)) % 8)
    bitmap = len(f.data)
    f.data += struct.pack(">hhhhIhh", width, width_img, 0, 0, 0, height, 0)
    f.intern[bitmap + 8] = buf
    sprite = len(f.data)
    sp = bytearray(f.data[template:template + SPRITE_BYTES])
    struct.pack_into(">hh", sp, 4, width, height)
    struct.pack_into(">hh", sp, 44, height, height)
    for slot in [s for s in f.intern if template <= s < template + SPRITE_BYTES]:
        if slot - template != 52:
            raise PresentError(f"sprite {template:#x}: a pointer at +{slot - template:#x} this copy cannot carry")
    struct.pack_into(">I", sp, 52, 0)
    f.data += sp
    f.intern[sprite + 52] = bitmap
    if pad_to is not None:
        end = buf + pad_to
        if len(f.data) > end:
            raise PresentError(f"sprite block {len(f.data) - buf:#x} bytes, more than {pad_to:#x}")
        f.data += bytes(end - len(f.data))
    return sprite


def sprite_rgba(f: N.RelocFile, offset: int) -> tuple[np.ndarray, int]:
    """An IA8 or I4 sprite's texels as (intensity, alpha) arrays; for reading
    Remix's names and logos only."""
    width, height = struct.unpack_from(">hh", f.data, offset + 4)
    fmt, siz = f.data[offset + 48], f.data[offset + 49]
    attr = struct.unpack_from(">H", f.data, offset + 20)[0]
    bitmap = f.intern[offset + 52]
    width_img = struct.unpack_from(">h", f.data, bitmap + 2)[0]
    buf = f.intern[bitmap + 8]
    bits = {0: 4, 1: 8, 2: 16}[siz]
    stride = (width_img * bits) // 8
    rows = []
    for y in range(height):
        raw = bytes(f.data[buf + y * stride:buf + (y + 1) * stride])
        if attr & SP_TEXSHUF and (y & 1):
            raw = b"".join(raw[k + 4:k + 8] + raw[k:k + 4] for k in range(0, len(raw), 8))
        rows.append(raw)
    if (fmt, siz) == (3, 1):
        i = np.array([[(b >> 4) * 17 for b in r[:width]] for r in rows])
        a = np.array([[(b & 15) * 17 for b in r[:width]] for r in rows])
        return np.stack([i, a], -1), width
    raise PresentError(f"sprite {offset:#x}: format {fmt}/{siz} not read here")


# ---------------------------------------------------------------- portrait

def _gradient(stops, n: int) -> np.ndarray:
    out = np.zeros((n, 3))
    for y in range(n):
        t = y / (n - 1) * (len(stops) - 1)
        k = min(int(t), len(stops) - 2)
        u = t - k
        out[y] = (1 - u) * np.array(stops[k]) + u * np.array(stops[k + 1])
    return out


def portrait_pair(render: Path) -> tuple[np.ndarray, np.ndarray]:
    """The 32x32 portrait and its flash from a transparent render."""
    from PIL import Image
    im = Image.open(render).convert("RGBA")
    # Premultiplied downsample to the 28x28 inside of the frame.
    arr = np.asarray(im).astype(float) / 255.0
    pm = arr.copy()
    pm[..., :3] *= pm[..., 3:4]
    small = np.asarray(Image.fromarray((pm * 255).astype(np.uint8), "RGBA").resize((28, 28), Image.LANCZOS)).astype(float) / 255.0
    a = small[..., 3:4]
    colour = np.where(a > 1e-3, small[..., :3] / np.maximum(a, 1e-3), 0.0)
    out = []
    for stops, glow in ((BG, 0.0), (BG_FLASH, 0.55)):
        bg = np.repeat(_gradient(stops, 28)[:, None, :] / 255.0, 28, axis=1)
        pic = colour * a + bg * (1 - a)
        if glow:
            # The flash lights the face's edge as Remix's do.
            edge = (a[..., 0] > 0.05) & (a[..., 0] < 0.95)
            pic[edge] = pic[edge] * (1 - glow) + glow
        tile = np.zeros((32, 32, 4))
        tile[1:29, 1:29, :3] = pic
        tile[0:30, 0:30, 3] = 1.0
        tile[0, 0:30, :3] = np.array(FRAME["top"]) / 255.0
        tile[29, 0:30, :3] = np.array(FRAME["bottom"]) / 255.0
        tile[0:30, 0, :3] = np.array(FRAME["left"]) / 255.0
        tile[0:30, 29, :3] = np.array(FRAME["right"]) / 255.0
        out.append(np.clip(np.round(tile * 255), 0, 255).astype(int))
    return out[0], out[1]


# -------------------------------------------------------------------- names

def letters(names_file: N.RelocFile, name_offsets: dict[str, int]) -> dict[str, np.ndarray]:
    """Each letter Remix's names show (IA8: intensity, alpha). The letters'
    outlines touch, so a word splits at its letters' bright cores: each
    letter runs from midway between cores to midway between cores."""
    found: dict[str, np.ndarray] = {}
    for word in LETTER_SOURCES:
        if word not in name_offsets:
            continue
        ia, width = sprite_rgba(names_file, name_offsets[word])
        core = np.where(ia[..., 1] > 128, ia[..., 0], 0).max(axis=0) >= CORE
        runs = []
        x = 0
        while x < width:
            if core[x]:
                x0 = x
                while x < width and core[x]:
                    x += 1
                runs.append((x0, x))
            else:
                x += 1
        if len(runs) != len(word):
            continue
        opaque = np.nonzero(ia[..., 1].max(axis=0) > 0)[0]
        cuts = [int(opaque[0])] + [(runs[k][1] + runs[k + 1][0]) // 2 for k in range(len(runs) - 1)] +                [int(opaque[-1]) + 1]
        for k, c in enumerate(word):
            found.setdefault(c, ia[:, cuts[k]:cuts[k + 1]])
    return found


def compose_name(word: str, glyphs: dict[str, np.ndarray]) -> np.ndarray:
    """The word's letters kerned as Remix's own words are: each letter's
    shadow falls to its left and tucks under the letter before, leaving one
    column of shadow (or nothing) between their bodies in every row. Where
    the two overlap, a body pixel (core or lit edge) shows over a shadow."""
    missing = sorted({c for c in word if c not in glyphs})
    if missing:
        raise PresentError(f"no letter {missing} in Remix's names for {word!r}")
    # Remix's name sprites are 15 or 16 rows; their letters' cores start on
    # the same row of each, so the letters line up by that row.
    tops = {c: int(np.argmax(((glyphs[c][..., 1] > 128) & (glyphs[c][..., 0] >= CORE)).any(axis=1)))
            for c in set(word)}
    top = max(tops.values())
    height = max(top - tops[c] + glyphs[c].shape[0] for c in set(word))
    out = None
    for c in word:
        g = np.zeros((height, glyphs[c].shape[1], 2), dtype=glyphs[c].dtype)
        g[top - tops[c]:top - tops[c] + glyphs[c].shape[0]] = glyphs[c]
        if out is None:
            out = g
            continue
        body_l = (out[..., 1] > 0) & (out[..., 0] >= BODY)
        body_r = (g[..., 1] > 0) & (g[..., 0] >= BODY)
        rows = body_l.any(axis=1) & body_r.any(axis=1)
        if rows.any():
            trail = out.shape[1] - 1 - (out.shape[1] - 1 - np.argmax(body_l[:, ::-1], axis=1))
            lead = np.argmax(body_r, axis=1)
            overlap = int(max(0, min(int((trail + lead)[rows].min()) - 1, g.shape[1] - 1)))
        else:
            overlap = 0
        w = out.shape[1] + g.shape[1] - overlap
        merged = np.zeros((height, w, 2), dtype=out.dtype)
        merged[:, :out.shape[1]] = out
        x0 = out.shape[1] - overlap
        under = merged[:, x0:x0 + g.shape[1]]
        take = (g[..., 1] > 0) & ((under[..., 1] == 0) | (g[..., 0] > under[..., 0]))
        under[take] = g[take]
        out = merged
    if out.shape[1] > NAME_MAX_WIDTH:
        raise PresentError(f"{word!r} is {out.shape[1]} wide; the card holds {NAME_MAX_WIDTH}")
    return out


# ---------------------------------------------------------------- the logos

def kid_icarus_emblem() -> np.ndarray:
    """A 64x48 I4 emblem for Kid Icarus (authored): two spread wings, each of
    four feathers, around an upright arrow, white on clear as Remix's logos
    draw (the select tints them)."""
    h, w = 48, 64
    img = np.zeros((h, w))
    cx = w / 2
    yy, xx = np.mgrid[0:h, 0:w]
    # The arrow: a shaft and a head.
    img[(np.abs(xx - cx + 0.5) <= 1.2) & (yy >= 10) & (yy <= 42)] = 1.0
    head = (yy >= 4) & (yy <= 13) & (np.abs(xx - cx + 0.5) <= (yy - 4) * 0.55 + 0.5)
    img[head] = 1.0
    for side in (-1, 1):
        for k in range(4):
            # Feathers fan out from the shaft, longest on top.
            base_y = 18 + k * 5.5
            length = 26 - k * 4.5
            ang = np.radians(18 + k * 12)
            dx = side * np.cos(ang)
            dy = -np.sin(ang)
            for t in np.linspace(3, length, 160):
                px = cx + dx * t
                py = base_y + dy * t
                thick = 2.6 * (1 - t / (length + 6))
                m = (xx - px) ** 2 + (yy - py) ** 2 <= thick ** 2
                img[m] = 1.0
    return (img * 255).astype(int)


# ------------------------------------------------------------------- apply

def apply(name: str, files: dict[str, Path], portraits: dict[str, int], names: dict[str, int],
          logos: dict[str, int], base_art: str) -> dict:
    """Append `name`'s art to the build's donor copies (files: role -> o2r
    path) and return the offsets its cell and row take (absent: the base's)."""
    out: dict = {}
    render = REPO / "builds" / "p6-art" / name / "portrait.png"
    if render.exists():
        f = N.RelocFile.read(files["portraits"].read_bytes())
        template = portraits[base_art]
        attr = struct.unpack_from(">H", f.data, template + 20)[0]
        pic, flash = portrait_pair(render)
        # The block's texels start where the portrait's own do (0x810 before
        # its sprite): the flash is then exactly one block later.
        sprites = [append_sprite(f, template, _swizzle(_rgba5551(rgba), bool(attr & SP_TEXSHUF)),
                                 32, 32, 32, pad_to=PORTRAIT_BLOCK) for rgba in (pic, flash)]
        if sprites[1] != sprites[0] + PORTRAIT_BLOCK:
            raise PresentError("the portrait's flash is not one block after it")
        first = sprites[0]
        files["portraits"].write_bytes(f.to_bytes())
        out["portrait"] = first
    word = NAMES.get(name)
    if word:
        f = N.RelocFile.read(files["names"].read_bytes())
        glyphs = letters(f, names)
        ia = compose_name(word, glyphs)
        height, width = ia.shape[:2]
        width_img = width + ((-width) % 8)
        template = names[base_art] if base_art in names else names["PEACH"]
        attr = struct.unpack_from(">H", f.data, template + 20)[0]
        texels = _swizzle(_ia8(ia[..., 0], ia[..., 1], width_img), bool(attr & SP_TEXSHUF))
        out["name_sprite"] = append_sprite(f, template, texels, width, width_img, height)
        files["names"].write_bytes(f.to_bytes())
    logo = LOGOS.get(name)
    if logo and logo.startswith("authored:"):
        f = N.RelocFile.read(files["logos"].read_bytes())
        template = logos["FIRE_EMBLEM"]
        attr = struct.unpack_from(">H", f.data, template + 20)[0]
        img = kid_icarus_emblem()
        texels = _swizzle(_i4(img), bool(attr & SP_TEXSHUF))
        out["logo_sprite"] = append_sprite(f, template, texels, 64, 64, 48)
        out["logo"] = "FIRE_EMBLEM"     # its placement: the 64x48 logos share one
        files["logos"].write_bytes(f.to_bytes())
    elif logo:
        out["logo"] = logo
        out["logo_sprite"] = logos[logo]
    return out
