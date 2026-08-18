#!/usr/bin/env python3
"""P2-1c. Bake the SSB64 menu UI assets into a DS-native pack at build time.

WHAT THIS CONVERTS AND WHY IT IS A BUILD STEP
---------------------------------------------
`PROJECT_GOAL.md` says compile-time asset conversion and aggressive baking are
the preferred shape, and `AGENTS.md` says loading time is cheap while gameplay
CPU is not.  The menu font, hand cursor and CSS portraits are immutable data:
every byte of the N64 sprite container -- the RELO header, the big-endian
payload, the internal pointer list, the 4-bit intensity nibbles, the N64
odd-row LoadBlock swizzle -- can be resolved once, here, and never again on
the ARM9.

INPUTS (read-only reference; never written)
  decomp/BattleShip-main/BattleShip_o2r/reloc_menus/MNCommonFonts     (file 33)
  decomp/BattleShip-main/BattleShip_o2r/reloc_menus/MNPlayersCommon   (file 17)
  decomp/BattleShip-main/BattleShip_o2r/reloc_menus/MNPlayersPortraits(file 19)
  include/reloc_data.h  -- the payload offset of every sprite, already in-tree.
                           Parsed rather than re-copied so the two cannot drift.

OUTPUTS (generated; gitignored exactly like every other asset here)
  assets/menus/mn_ui_kit.bin                    -- the pack, staged to NitroFS
  src/nds/generated/mn_ui_kit.generated.inc     -- the manifest the runtime
                                                   compiles in (offsets, sizes,
                                                   glyph metrics, FNV-1a check)

THE CONTAINER FORMAT (derived from src/port/reloc_backend_assets.c, which is
the runtime that already reads these files, not from a guess)
  bytes 0x00..0x4f  header, LITTLE-endian:
                      0x40 u32 file id
                      0x44 u16 reloc_intern_offset  (word index, 0xffff = none)
                      0x46 u16 reloc_extern_offset
                      0x4c u32 data_size
  bytes 0x50..       payload, BIG-endian (the runtime u32-byte-swaps it).
  Internal pointers are a linked list threaded through the payload itself:
  the word at `reloc_intern` holds (next_word_index << 16) | target_word_index,
  and is overwritten with the resolved address.  Offline we keep the offsets.

THE PIXEL FORMAT
  Glyphs are G_IM_FMT_I / G_IM_SIZ_4b: 4-bit intensity, `width_img` texels a
  row, and SP_TEXSHUF set -- the N64 LoadBlock swizzle, which stores every ODD
  row with its 32-bit halves exchanged.  Missing that swizzle is this repo's
  most-repeated asset bug (memory: "audit interleave FIRST"), so the decoder
  applies it and `--preview` prints the glyphs as ASCII for a human check.

  Intensity is preserved as 8-bit (nibble * 17) rather than resolved to a
  colour here: the source modulates each string by a primitive RGB
  (mnmaps.c:340 `sobj->sprite.red/green/blue`), so the tint belongs to the
  draw call and the shape belongs to the pack.

Run standalone with --preview to see what the pack contains without building.
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

# ---------------------------------------------------------------------------
# Container
# ---------------------------------------------------------------------------

RELO_HEADER_BYTES = 0x50
RELO_MAGIC = 0x52454C4F  # 'RELO'

G_IM_FMT_RGBA, G_IM_FMT_YUV, G_IM_FMT_CI, G_IM_FMT_IA, G_IM_FMT_I = range(5)
G_IM_SIZ_4b, G_IM_SIZ_8b, G_IM_SIZ_16b, G_IM_SIZ_32b = range(4)

SPRITE_BYTES = 68
BITMAP_BYTES = 16


class ConvertError(RuntimeError):
    """A deterministic conversion falsifier -- never a warning."""


@dataclass(frozen=True)
class Bitmap:
    width: int
    width_img: int
    s: int
    t: int
    buf: int  # payload offset, already resolved
    actual_height: int
    lut_offset: int


@dataclass(frozen=True)
class Sprite:
    x: int
    y: int
    width: int
    height: int
    attr: int
    red: int
    green: int
    blue: int
    alpha: int
    start_tlut: int
    n_tlut: int
    lut: int  # payload offset or 0
    nbitmaps: int
    bmheight: int
    bm_h_real: int
    bmfmt: int
    bmsiz: int
    bitmap: int  # payload offset


class RelocFile:
    """One o2r RELO container with its internal pointers resolved to offsets."""

    def __init__(self, path: Path) -> None:
        raw = path.read_bytes()
        if len(raw) < RELO_HEADER_BYTES:
            raise ConvertError(f"{path.name}: shorter than a RELO header")
        magic = struct.unpack_from("<I", raw, 4)[0]
        if magic != RELO_MAGIC:
            raise ConvertError(f"{path.name}: magic {magic:#x} is not 'RELO'")
        self.path = path
        self.file_id = struct.unpack_from("<I", raw, 0x40)[0]
        self.reloc_intern = struct.unpack_from("<H", raw, 0x44)[0]
        self.data_size = struct.unpack_from("<I", raw, 0x4C)[0]
        if RELO_HEADER_BYTES + self.data_size > len(raw):
            raise ConvertError(
                f"{path.name}: declared payload {self.data_size} exceeds file")
        self.payload = bytearray(raw[RELO_HEADER_BYTES:
                                     RELO_HEADER_BYTES + self.data_size])
        self.pointer_targets: dict[int, int] = {}
        self._walk_internal_relocs()

    def _walk_internal_relocs(self) -> None:
        """Reproduce ndsRelocApplyInternalPointerFixups without writing."""
        cursor = self.reloc_intern
        guard = (len(self.payload) // 4) + 1
        while cursor != 0xFFFF:
            slot = cursor * 4
            if slot + 4 > len(self.payload):
                raise ConvertError(
                    f"{self.path.name}: reloc slot {slot:#x} out of range")
            if guard == 0:
                raise ConvertError(f"{self.path.name}: reloc list does not end")
            guard -= 1
            word = struct.unpack_from(">I", self.payload, slot)[0]
            nxt = (word >> 16) & 0xFFFF
            target = (word & 0xFFFF) * 4
            if target >= len(self.payload):
                raise ConvertError(
                    f"{self.path.name}: reloc target {target:#x} out of range")
            self.pointer_targets[slot] = target
            cursor = nxt

    # -- typed payload reads (payload is big-endian) ------------------------

    def u8(self, off: int) -> int:
        return self.payload[off]

    def u16(self, off: int) -> int:
        return struct.unpack_from(">H", self.payload, off)[0]

    def s16(self, off: int) -> int:
        return struct.unpack_from(">h", self.payload, off)[0]

    def pointer(self, off: int) -> int:
        """A pointer field: 0 when null, else the payload offset it resolves to."""
        if off in self.pointer_targets:
            return self.pointer_targets[off]
        raw = struct.unpack_from(">I", self.payload, off)[0]
        if raw == 0:
            return 0
        raise ConvertError(
            f"{self.path.name}: pointer at {off:#x} = {raw:#x} is not in the "
            "internal reloc list (an external dependency this bake cannot take)")

    def sprite(self, off: int) -> Sprite:
        if off + SPRITE_BYTES > len(self.payload):
            raise ConvertError(f"{self.path.name}: sprite {off:#x} out of range")
        return Sprite(
            x=self.s16(off + 0), y=self.s16(off + 2),
            width=self.s16(off + 4), height=self.s16(off + 6),
            attr=self.u16(off + 20),
            red=self.u8(off + 24), green=self.u8(off + 25),
            blue=self.u8(off + 26), alpha=self.u8(off + 27),
            start_tlut=self.s16(off + 28), n_tlut=self.s16(off + 30),
            lut=self.pointer(off + 32),
            nbitmaps=self.s16(off + 40),
            bmheight=self.s16(off + 44), bm_h_real=self.s16(off + 46),
            bmfmt=self.u8(off + 48), bmsiz=self.u8(off + 49),
            bitmap=self.pointer(off + 52))

    def bitmap(self, off: int) -> Bitmap:
        if off + BITMAP_BYTES > len(self.payload):
            raise ConvertError(f"{self.path.name}: bitmap {off:#x} out of range")
        return Bitmap(
            width=self.s16(off + 0), width_img=self.s16(off + 2),
            s=self.s16(off + 4), t=self.s16(off + 6),
            buf=self.pointer(off + 8),
            actual_height=self.s16(off + 12), lut_offset=self.s16(off + 14))


# ---------------------------------------------------------------------------
# Pixel decode
# ---------------------------------------------------------------------------

SP_TEXSHUF = 0x0200


def deswizzle_row(row: bytes, row_index: int, shuffled: bool,
                  granule: int) -> bytes:
    """Undo the N64 LoadBlock swizzle on odd rows.

    Each granule of an odd row is stored with its two halves exchanged.  The
    granule is the DRAM span one RDP TMEM word covers: 8 bytes for 4/8/16-bit
    textures, and 16 bytes for 32-bit ones, because a 32-bit texel is split
    across TMEM's low (RG) and high (BA) halves so two TMEM words are consumed
    per 8 DRAM bytes.

    MEASURED, not assumed: `--preview-dir` renders the bake as PNGs, and the
    45x43 RGBA32 portraits are legible only at granule 16 (8 gives the classic
    per-pixel comb).  Getting this wrong is the single most-repeated asset bug
    in this repository, so the granule is a parameter and never a constant.
    """
    if (not shuffled) or ((row_index & 1) == 0):
        return bytes(row)
    half = granule // 2
    padded = bytes(row) + b"\x00" * ((-len(row)) % granule)
    out = bytearray(len(padded))
    for base in range(0, len(padded), granule):
        out[base:base + half] = padded[base + half:base + granule]
        out[base + half:base + granule] = padded[base:base + half]
    return bytes(out[:len(row)])


def read_row(fileobj: RelocFile, bitmap: Bitmap, sprite: Sprite,
             y: int, stride: int, granule: int = 8) -> bytes:
    """One texture row, de-swizzled.

    The swizzle parity belongs to the row's position in the LOADED image, so
    it keys on `bitmap.t + y`, not on `y`.  Every sprite here has t == 0, but a
    multi-band portrait is exactly the shape where that would stop being true.
    """
    absolute = bitmap.t + y
    start = bitmap.buf + (absolute * stride)
    raw = fileobj.payload[start:start + stride]
    if len(raw) < stride:
        raise ConvertError(
            f"row {absolute} runs past the payload (need {stride} bytes)")
    return deswizzle_row(raw, absolute, (sprite.attr & SP_TEXSHUF) != 0,
                         granule)


def decode_i4(fileobj: RelocFile, bitmap: Bitmap, sprite: Sprite,
              width: int, height: int) -> list[list[int]]:
    """4-bit intensity -> rows of 8-bit intensity."""
    stride = max(1, bitmap.width_img // 2)
    rows: list[list[int]] = []
    for y in range(height):
        raw = read_row(fileobj, bitmap, sprite, y, stride)
        row = []
        for x in range(width):
            sx = bitmap.s + x
            nib = raw[sx >> 1]
            nib = (nib >> 4) if (sx & 1) == 0 else (nib & 0xF)
            row.append(nib * 17)
        rows.append(row)
    return rows


def decode_ia16(fileobj: RelocFile, bitmap: Bitmap, sprite: Sprite,
                width: int, height: int) -> list[list[tuple[int, int]]]:
    """16-bit IA (8 intensity + 8 alpha) -> rows of (intensity, alpha)."""
    stride = max(2, bitmap.width_img * 2)
    rows = []
    for y in range(height):
        raw = read_row(fileobj, bitmap, sprite, y, stride)
        row = []
        for x in range(width):
            base = (bitmap.s + x) * 2
            row.append((raw[base], raw[base + 1]))
        rows.append(row)
    return rows


def decode_rgba32(fileobj: RelocFile, bitmap: Bitmap, sprite: Sprite,
                  width: int, height: int) -> list[list[tuple[int, int, int, int]]]:
    """32-bit RGBA -> rows of (r, g, b, a)."""
    stride = max(4, bitmap.width_img * 4)
    rows = []
    for y in range(height):
        raw = read_row(fileobj, bitmap, sprite, y, stride, granule=16)
        row = []
        for x in range(width):
            base = (bitmap.s + x) * 4
            row.append((raw[base], raw[base + 1], raw[base + 2], raw[base + 3]))
        rows.append(row)
    return rows


def rgba8_to_ds(red: int, green: int, blue: int, alpha: int) -> int:
    """RGBA8888 -> DS BGR5551.  Bit 15 is the bitmap-OBJ opacity bit."""
    if alpha < 128:
        return 0
    return ((1 << 15) | ((blue >> 3) << 10) | ((green >> 3) << 5) |
            (red >> 3))


# ---------------------------------------------------------------------------
# reloc_data.h -- the offsets, parsed so they cannot drift from the runtime
# ---------------------------------------------------------------------------

def load_reloc_offsets(repo_root: Path) -> dict[str, int]:
    text = (repo_root / "include" / "reloc_data.h").read_text(errors="replace")
    out: dict[str, int] = {}
    for name, value in re.findall(r"X\((ll\w+),\s*(0x[0-9a-fA-F]+|\d+)\)", text):
        out[name] = int(value, 0)
    if not out:
        raise ConvertError("include/reloc_data.h yielded no X(name, offset) rows")
    return out


# ---------------------------------------------------------------------------
# Font
# ---------------------------------------------------------------------------

# mnmaps.c:186 mnMapsGetCharacterID -- the source's own glyph order.
GLYPH_NAMES = [f"Letter{chr(ord('A') + i)}" for i in range(26)] + [
    "SymbolApostrophe", "SymbolPercent", "SymbolPeriod"]

GLYPH_CELL_W = 8
GLYPH_CELL_H = 8


@dataclass
class Glyph:
    name: str
    width: int
    height: int
    pixels: list[list[int]]


def convert_font(repo_root: Path, offsets: dict[str, int]) -> list[Glyph]:
    path = (repo_root / "decomp" / "BattleShip-main" / "BattleShip_o2r" /
            "reloc_menus" / "MNCommonFonts")
    fileobj = RelocFile(path)
    if fileobj.file_id != offsets["llMNCommonFontsFileID"]:
        raise ConvertError(
            f"MNCommonFonts file id {fileobj.file_id:#x} != reloc_data.h "
            f"{offsets['llMNCommonFontsFileID']:#x}")
    glyphs: list[Glyph] = []
    for name in GLYPH_NAMES:
        symbol = f"llMNCommonFonts{name}Sprite"
        if symbol not in offsets:
            raise ConvertError(f"{symbol} missing from include/reloc_data.h")
        sprite = fileobj.sprite(offsets[symbol])
        if sprite.bmfmt != G_IM_FMT_I or sprite.bmsiz != G_IM_SIZ_4b:
            raise ConvertError(
                f"{symbol}: expected I4, got fmt={sprite.bmfmt} "
                f"siz={sprite.bmsiz}")
        if sprite.nbitmaps != 1:
            raise ConvertError(f"{symbol}: {sprite.nbitmaps} bitmaps, expected 1")
        bitmap = fileobj.bitmap(sprite.bitmap)
        width, height = bitmap.width, bitmap.actual_height
        if not (1 <= width <= GLYPH_CELL_W) or not (1 <= height <= GLYPH_CELL_H):
            raise ConvertError(
                f"{symbol}: {width}x{height} does not fit the "
                f"{GLYPH_CELL_W}x{GLYPH_CELL_H} cell")
        glyphs.append(Glyph(name, width, height,
                            decode_i4(fileobj, bitmap, sprite, width, height)))
    return glyphs


# ---------------------------------------------------------------------------
# Images (cursor, portraits) -> DS bitmap-OBJ cells
# ---------------------------------------------------------------------------

# DS OBJ cell sizes that libnds' SpriteSize enum can express, largest first is
# not useful here: pick the smallest cell each image fits in.
OBJ_CELLS = [(8, 8), (16, 16), (32, 32), (64, 64),
             (16, 8), (32, 8), (32, 16), (64, 32),
             (8, 16), (8, 32), (16, 32), (32, 64)]


def choose_cell(width: int, height: int) -> tuple[int, int]:
    best = None
    for cw, ch in OBJ_CELLS:
        if cw >= width and ch >= height:
            area = cw * ch
            if best is None or area < best[0]:
                best = (area, cw, ch)
    if best is None:
        raise ConvertError(f"{width}x{height} exceeds a 64x64 OBJ cell")
    return best[1], best[2]


@dataclass
class Image:
    name: str
    token: str
    src_w: int
    src_h: int
    cell_w: int
    cell_h: int
    texels: list[int]  # cell_w * cell_h DS BGR5551 halfwords


def convert_image(fileobj: RelocFile, symbol: str, token: str,
                  offset: int) -> Image:
    sprite = fileobj.sprite(offset)
    if sprite.nbitmaps < 1:
        raise ConvertError(f"{symbol}: no bitmaps")
    bitmap = fileobj.bitmap(sprite.bitmap)
    width = sprite.width
    height = sprite.height
    cell_w, cell_h = choose_cell(width, height)
    texels = [0] * (cell_w * cell_h)

    # Multi-bitmap sprites stack vertically: bitmap i covers rows
    # [i * bmheight, ...).  Walk them in order and stop at the sprite height.
    row = 0
    for index in range(sprite.nbitmaps):
        piece = fileobj.bitmap(sprite.bitmap + index * BITMAP_BYTES)
        piece_h = min(piece.actual_height, height - row)
        if piece_h <= 0:
            break
        piece_w = min(piece.width, width)
        if sprite.bmfmt == G_IM_FMT_I and sprite.bmsiz == G_IM_SIZ_4b:
            for y, pixels in enumerate(
                    decode_i4(fileobj, piece, sprite, piece_w, piece_h)):
                for x, i in enumerate(pixels):
                    texels[(row + y) * cell_w + x] = rgba8_to_ds(
                        (sprite.red * i) // 255, (sprite.green * i) // 255,
                        (sprite.blue * i) // 255, 255 if i else 0)
        elif sprite.bmfmt == G_IM_FMT_IA and sprite.bmsiz == G_IM_SIZ_16b:
            for y, pixels in enumerate(
                    decode_ia16(fileobj, piece, sprite, piece_w, piece_h)):
                for x, (i, a) in enumerate(pixels):
                    texels[(row + y) * cell_w + x] = rgba8_to_ds(
                        (sprite.red * i) // 255, (sprite.green * i) // 255,
                        (sprite.blue * i) // 255, a)
        elif sprite.bmfmt == G_IM_FMT_RGBA and sprite.bmsiz == G_IM_SIZ_32b:
            for y, pixels in enumerate(
                    decode_rgba32(fileobj, piece, sprite, piece_w, piece_h)):
                for x, (r, g, b, a) in enumerate(pixels):
                    texels[(row + y) * cell_w + x] = rgba8_to_ds(r, g, b, a)
        else:
            raise ConvertError(
                f"{symbol}: unsupported fmt={sprite.bmfmt} siz={sprite.bmsiz}")
        row += piece_h
        if row >= height:
            break
    return Image(symbol, token, width, height, cell_w, cell_h, texels)


# ---------------------------------------------------------------------------
# Pack + manifest
# ---------------------------------------------------------------------------

PACK_VERSION = 1


def fnv1a32(data: bytes) -> int:
    h = 0x811C9DC5
    for byte in data:
        h ^= byte
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h


def build_pack(glyphs: list[Glyph], images: list[Image]
               ) -> tuple[bytes, list[tuple[str, int, int]]]:
    """Glyph cells first (byte per texel), then image cells (halfword each)."""
    blob = bytearray()
    for glyph in glyphs:
        cell = bytearray(GLYPH_CELL_W * GLYPH_CELL_H)
        for y, row in enumerate(glyph.pixels):
            for x, value in enumerate(row):
                cell[y * GLYPH_CELL_W + x] = value
        blob += cell
    # Halfword alignment for the image block; the runtime DMAs it as words.
    while len(blob) % 4:
        blob.append(0)
    image_table = []
    for image in images:
        offset = len(blob)
        for texel in image.texels:
            blob += struct.pack("<H", texel)
        image_table.append((image.name, offset, len(blob) - offset))
    return bytes(blob), image_table


def emit_manifest(path: Path, glyphs: list[Glyph], images: list[Image],
                  image_table: list[tuple[str, int, int]],
                  pack: bytes) -> None:
    lines = [
        "/* GENERATED by scripts/menus/generate_mn_ui_kit.py -- do not edit. */",
        "#ifndef NDS_MN_UI_KIT_GENERATED_INC",
        "#define NDS_MN_UI_KIT_GENERATED_INC",
        "",
        f"#define NDS_MN_UI_KIT_PACK_VERSION {PACK_VERSION}u",
        f"#define NDS_MN_UI_KIT_PACK_BYTES {len(pack)}u",
        f"#define NDS_MN_UI_KIT_PACK_FNV32 0x{fnv1a32(pack):08x}u",
        f"#define NDS_MN_UI_KIT_GLYPH_COUNT {len(glyphs)}u",
        f"#define NDS_MN_UI_KIT_GLYPH_CELL_W {GLYPH_CELL_W}u",
        f"#define NDS_MN_UI_KIT_GLYPH_CELL_H {GLYPH_CELL_H}u",
        (f"#define NDS_MN_UI_KIT_GLYPH_BLOCK_BYTES "
         f"{len(glyphs) * GLYPH_CELL_W * GLYPH_CELL_H}u"),
        f"#define NDS_MN_UI_KIT_IMAGE_COUNT {len(images)}u",
        "",
        "/* One row per glyph, in mnMapsGetCharacterID order (A..Z ' % .).",
        " * Marked unused because the demo translation unit includes this file",
        " * for the image indices alone. */",
        "static const NdsUiKitGlyphMetric kNdsUiKitGlyphMetrics"
        "[NDS_MN_UI_KIT_GLYPH_COUNT] __attribute__((unused)) = {",
    ]
    for glyph in glyphs:
        lines.append(f"    {{ {glyph.width}u, {glyph.height}u }}, "
                     f"/* {glyph.name} */")
    lines.append("};")
    lines.append("")
    lines.append("/* Image cells, already in DS BGR5551 with bit 15 = opaque. */")
    lines.append("static const NdsUiKitImageMetric kNdsUiKitImageMetrics"
                 "[NDS_MN_UI_KIT_IMAGE_COUNT] __attribute__((unused)) = {")
    for image, (name, offset, size) in zip(images, image_table):
        lines.append(
            f"    {{ {offset}u, {size}u, {image.cell_w}u, {image.cell_h}u, "
            f"{image.src_w}u, {image.src_h}u }}, /* {name} */")
    lines.append("};")
    lines.append("")
    for index, image in enumerate(images):
        lines.append(f"#define NDS_MN_UI_KIT_IMAGE_{image.token} {index}u")
    lines.append("")
    lines.append("#endif /* NDS_MN_UI_KIT_GENERATED_INC */")
    write_if_changed(path, "\n".join(lines) + "\n")


def write_if_changed(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_text(errors="replace") == text:
        return
    path.write_text(text)


def write_bytes_if_changed(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_bytes() == data:
        return
    path.write_bytes(data)


# ---------------------------------------------------------------------------

IMAGE_SOURCES = [
    # (o2r file, reloc_data.h symbol, NDS_MN_UI_KIT_IMAGE_<token>)
    ("MNPlayersCommon", "llMNPlayersCommonCursorHandPointSprite",
     "CURSOR_HAND_POINT"),
    ("MNPlayersCommon", "llMNPlayersCommonCursorHandGrabSprite",
     "CURSOR_HAND_GRAB"),
    ("MNPlayersPortraits", "llMNPlayersPortraitsMarioSprite",
     "PORTRAIT_MARIO"),
    ("MNPlayersPortraits", "llMNPlayersPortraitsFoxSprite", "PORTRAIT_FOX"),
]


def write_png(path: Path, width: int, height: int, rgb: bytes) -> None:
    """Minimal RGB8 PNG so a human can look at the bake without a ROM."""
    import zlib

    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw += rgb[y * width * 3:(y + 1) * width * 3]

    def chunk(tag: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    png = (b"\x89PNG\r\n\x1a\n" +
           chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
           chunk(b"IDAT", zlib.compress(bytes(raw), 9)) +
           chunk(b"IEND", b""))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def ds_texel_to_rgb(texel: int, backdrop: tuple[int, int, int]) -> tuple[int, int, int]:
    if (texel & 0x8000) == 0:
        return backdrop
    r = (texel & 0x1F) << 3
    g = ((texel >> 5) & 0x1F) << 3
    b = ((texel >> 10) & 0x1F) << 3
    return (r, g, b)


def write_previews(out_dir: Path, glyphs: list[Glyph],
                   images: list[Image]) -> None:
    """A glyph strip and one image per sprite, on magenta so alpha is visible."""
    backdrop = (255, 0, 255)
    strip_w = sum(g.width + 1 for g in glyphs) + 1
    strip_h = GLYPH_CELL_H + 2
    rgb = bytearray(backdrop * (strip_w * strip_h))
    cursor = 1
    for glyph in glyphs:
        for y, row in enumerate(glyph.pixels):
            for x, value in enumerate(row):
                base = ((y + 1) * strip_w + cursor + x) * 3
                rgb[base:base + 3] = bytes((value, value, value))
        cursor += glyph.width + 1
    write_png(out_dir / "mn_ui_kit_font.png", strip_w, strip_h, bytes(rgb))

    for image in images:
        pixels = bytearray()
        for texel in image.texels:
            pixels += bytes(ds_texel_to_rgb(texel, backdrop))
        name = image.name.replace("ll", "", 1)
        write_png(out_dir / f"mn_ui_kit_{name}.png", image.cell_w, image.cell_h,
                  bytes(pixels))


def preview_glyphs(glyphs: list[Glyph]) -> None:
    ramp = " .:-=+*#%@"
    for glyph in glyphs:
        print(f"-- {glyph.name} {glyph.width}x{glyph.height}")
        for row in glyph.pixels:
            print("   " + "".join(
                ramp[min(len(ramp) - 1, value * len(ramp) // 256)]
                for value in row))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".", type=Path)
    parser.add_argument("--preview", action="store_true",
                        help="print the decoded glyphs and image metrics only")
    parser.add_argument("--preview-dir", type=Path, default=None,
                        help="also write PNGs of the bake into this directory")
    parser.add_argument("--list-images", action="store_true",
                        help="print every candidate sprite in an o2r file")
    parser.add_argument("--o2r-file", default="MNPlayersCommon")
    args = parser.parse_args(argv)

    repo_root = args.repo_root.resolve()
    offsets = load_reloc_offsets(repo_root)

    if args.list_images:
        path = (repo_root / "decomp" / "BattleShip-main" / "BattleShip_o2r" /
                "reloc_menus" / args.o2r_file)
        fileobj = RelocFile(path)
        prefix = "ll" + args.o2r_file
        for name, offset in sorted(offsets.items(), key=lambda kv: kv[1]):
            if not name.startswith(prefix) or not name.endswith("Sprite"):
                continue
            try:
                sprite = fileobj.sprite(offset)
                bitmap = fileobj.bitmap(sprite.bitmap)
            except ConvertError as exc:
                print(f"{name:60s} SKIP {exc}")
                continue
            print(f"{name:60s} {sprite.width:3d}x{sprite.height:3d} "
                  f"fmt={sprite.bmfmt} siz={sprite.bmsiz} "
                  f"n={sprite.nbitmaps} bmh={sprite.bmheight} "
                  f"img_w={bitmap.width_img} tlut={sprite.n_tlut} "
                  f"attr={sprite.attr:#06x}")
        return 0

    glyphs = convert_font(repo_root, offsets)

    images: list[Image] = []
    cache: dict[str, RelocFile] = {}
    for o2r_name, symbol, token in IMAGE_SOURCES:
        if symbol not in offsets:
            raise ConvertError(f"{symbol} missing from include/reloc_data.h")
        if o2r_name not in cache:
            cache[o2r_name] = RelocFile(
                repo_root / "decomp" / "BattleShip-main" / "BattleShip_o2r" /
                "reloc_menus" / o2r_name)
        images.append(convert_image(cache[o2r_name], symbol, token,
                                    offsets[symbol]))

    pack, image_table = build_pack(glyphs, images)

    if args.preview_dir is not None:
        write_previews(args.preview_dir.resolve(), glyphs, images)

    if args.preview:
        preview_glyphs(glyphs)
        for image, (name, offset, size) in zip(images, image_table):
            print(f"-- {name} src {image.src_w}x{image.src_h} "
                  f"cell {image.cell_w}x{image.cell_h} "
                  f"offset {offset} bytes {size}")
        print(f"pack {len(pack)} bytes fnv32 0x{fnv1a32(pack):08x}")
        return 0

    write_bytes_if_changed(repo_root / "assets" / "menus" / "mn_ui_kit.bin",
                           pack)
    emit_manifest(repo_root / "src" / "nds" / "generated" /
                  "mn_ui_kit.generated.inc", glyphs, images, image_table, pack)
    print(f"mn_ui_kit: {len(glyphs)} glyphs, {len(images)} images, "
          f"{len(pack)} bytes, fnv32 0x{fnv1a32(pack):08x}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ConvertError as exc:
        print(f"generate_mn_ui_kit: {exc}", file=sys.stderr)
        sys.exit(1)
