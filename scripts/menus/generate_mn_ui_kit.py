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
from dataclasses import dataclass, replace
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


def decode_ia8(fileobj: RelocFile, bitmap: Bitmap, sprite: Sprite,
               width: int, height: int) -> list[list[tuple[int, int]]]:
    """8-bit IA (4 intensity + 4 alpha in one byte) -> rows of (i, a).

    One byte a texel, so the LoadBlock granule is the narrow one (8) exactly as
    I4 and IA16 use -- only the 32-bit path needs 16 (see deswizzle_row).
    """
    stride = max(1, bitmap.width_img)
    rows = []
    for y in range(height):
        raw = read_row(fileobj, bitmap, sprite, y, stride)
        row = []
        for x in range(width):
            packed = raw[bitmap.s + x]
            row.append((((packed >> 4) & 0xF) * 17, (packed & 0xF) * 17))
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


def rgba16_to_rgba8(texel: int) -> tuple[int, int, int, int]:
    """One N64 RGBA5551 texel -> (r, g, b, a) at 8 bits.

    5-bit channels are expanded by `(v << 3) | (v >> 2)` rather than `v << 3`
    so full-scale stays full-scale: 31 must become 255, not 248, or every
    white pixel in the bake comes out three percent grey.
    """
    red = (texel >> 11) & 0x1F
    green = (texel >> 6) & 0x1F
    blue = (texel >> 1) & 0x1F
    return ((red << 3) | (red >> 2), (green << 3) | (green >> 2),
            (blue << 3) | (blue >> 2), 255 if (texel & 1) else 0)


def decode_rgba16(fileobj: RelocFile, bitmap: Bitmap, sprite: Sprite,
                  width: int, height: int) -> list[list[tuple[int, int, int, int]]]:
    """16-bit RGBA (RGBA5551, big-endian) -> rows of (r, g, b, a).

    Granule 8, not the 16 the RGBA32 path needs: a 16-bit texel occupies ONE
    TMEM word half like the 4/8-bit formats do, so the LoadBlock swizzle spans
    the narrow granule.  P2-1f's map icons are this format and the preview
    PNGs are what confirmed the granule (16 combs the icon into 8 px columns).
    """
    stride = max(2, bitmap.width_img * 2)
    rows = []
    for y in range(height):
        raw = read_row(fileobj, bitmap, sprite, y, stride)
        row = []
        for x in range(width):
            base = (bitmap.s + x) * 2
            row.append(rgba16_to_rgba8((raw[base] << 8) | raw[base + 1]))
        rows.append(row)
    return rows


def decode_ci4(fileobj: RelocFile, bitmap: Bitmap, sprite: Sprite,
               width: int, height: int) -> list[list[tuple[int, int, int, int]]]:
    """4-bit colour index through the sprite's own TLUT -> (r, g, b, a).

    The palette is the sprite's `LUT` pointer, `nTLUT` RGBA5551 entries, loaded
    whole by the original (`spDraw`: `gDPSetTextureLUT(G_TT_RGBA16)` then
    `gDPLoadTLUT(nTLUT, 256 + startTLUT, LUT)` -- libultra/sp/sprite.c:236).
    The nibble indexes it directly: `Bitmap.LUToffset` exists in the header
    (PR/sp.h:52) but sprite.c never reads it, so there is no palette-bank
    selection to reproduce and inventing one would be a guess.
    """
    if sprite.lut == 0:
        raise ConvertError("CI4 sprite has no TLUT pointer")
    stride = max(1, bitmap.width_img // 2)
    palette: list[tuple[int, int, int, int]] = []
    for index in range(16):
        base = sprite.lut + ((sprite.start_tlut + index) * 2)
        raw = fileobj.payload[base:base + 2]
        if len(raw) < 2:
            raise ConvertError("TLUT runs past the payload")
        palette.append(rgba16_to_rgba8((raw[0] << 8) | raw[1]))
    rows = []
    for y in range(height):
        raw = read_row(fileobj, bitmap, sprite, y, stride)
        row = []
        for x in range(width):
            sx = bitmap.s + x
            nib = raw[sx >> 1]
            nib = (nib >> 4) if (sx & 1) == 0 else (nib & 0xF)
            row.append(palette[nib])
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
    """Every sprite offset this bake can reach, from both headers that hold one.

    `include/reloc_data.h`'s X-macro rows are the port's own compile-time
    offsets and stay the authority.  They cannot cover the title, though: the
    port ALREADY has `llMNTitleCutoutSprite` and friends as runtime-resolved
    `uintptr_t` globals (`src/port/diagnostics.c`, filled by
    `reloc_backend_assets.c` with a RAM address), so the same names cannot also
    carry a file offset -- an X row for them is a duplicate definition, not a
    second opinion.  The offsets are therefore read straight out of the
    read-only decomp header that owns them, and every symbol the two headers
    SHARE is checked to agree, so this fallback can never quietly disagree with
    the port's table.
    """
    text = (repo_root / "include" / "reloc_data.h").read_text(errors="replace")
    out: dict[str, int] = {}
    for name, value in re.findall(r"X\((ll\w+),\s*(0x[0-9a-fA-F]+|\d+)\)", text):
        out[name] = int(value, 0)
    if not out:
        raise ConvertError("include/reloc_data.h yielded no X(name, offset) rows")

    decomp = (repo_root / "decomp" / "BattleShip-main" / "include" /
              "reloc_data.us.h")
    if decomp.exists():
        pattern = r"#define\s+(ll\w+)\s+\(\(intptr_t\)(0x[0-9a-fA-F]+|\d+)\)"
        for name, value in re.findall(pattern,
                                      decomp.read_text(errors="replace")):
            parsed = int(value, 0)
            if name in out:
                if out[name] != parsed:
                    raise ConvertError(
                        f"{name}: include/reloc_data.h says {out[name]:#x}, "
                        f"reloc_data.us.h says {parsed:#x}")
                continue
            out[name] = parsed
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


@dataclass
class Surface:
    """A menu BACKDROP element: too large for an OBJ cell, drawn into BG2.

    P2-1h.  The menu collage is 300x220 and the title's drop-shadow cutout is
    208x90; a DS OBJ cell tops out at 64x64 and main OBJ VRAM has 8,448 bytes
    free, so neither can be a sprite.  They go to the main engine's BG2 bitmap
    -- the surface the menu shell already CLEARS on every screen entry -- which
    costs no new VRAM and no per-frame work, because a backdrop is composed
    once and then simply stays there.
    """
    name: str
    token: str
    width: int
    height: int
    dst_x: int   # DS top-left, signed: the title logo starts at y = -2
    dst_y: int
    opaque: bool  # no transparent texel, so the runtime may DMA whole rows
    texels: list[int]  # width * height DS BGR5551 halfwords, row-major


def box_scale(raster: list[list[tuple[int, int, int, int]]],
              dst_w: int, dst_h: int) -> list[list[tuple[int, int, int, int]]]:
    """Area-average downscale of an RGBA raster.

    Colour is averaged PREMULTIPLIED by alpha and alpha is averaged on its own,
    so a transparent texel contributes its transparency but not its (undefined)
    colour.  Averaging straight RGB instead pulls the container's black fringe
    into every edge -- the classic downscale halo -- and on a 45x43 portrait
    that reads as a dark outline the original does not have.
    """
    src_h = len(raster)
    src_w = len(raster[0]) if src_h else 0
    if (src_w == 0) or (src_h == 0):
        raise ConvertError("box_scale: empty raster")
    out = []
    for dy in range(dst_h):
        y0 = (dy * src_h) // dst_h
        y1 = max(y0 + 1, ((dy + 1) * src_h) // dst_h)
        row = []
        for dx in range(dst_w):
            x0 = (dx * src_w) // dst_w
            x1 = max(x0 + 1, ((dx + 1) * src_w) // dst_w)
            count = 0
            acc_r = acc_g = acc_b = acc_a = 0
            for sy in range(y0, y1):
                for sx in range(x0, x1):
                    r, g, b, a = raster[sy][sx]
                    acc_r += r * a
                    acc_g += g * a
                    acc_b += b * a
                    acc_a += a
                    count += 1
            if acc_a == 0:
                row.append((0, 0, 0, 0))
            else:
                row.append((acc_r // acc_a, acc_g // acc_a, acc_b // acc_a,
                            acc_a // count))
        out.append(row)
    return out


def decode_sprite_raster(fileobj: RelocFile, symbol: str, offset: int,
                         tint: tuple[int, int, int] | None = None,
                         alpha_ramp: bool = False
                         ) -> tuple[Sprite, list[list[tuple[int, int, int, int]]]]:
    """Decode one sprite's bands into an RGBA raster at the sprite's own size.

    THE BAND POLICY IS spDraw's, TRANSCRIBED (libultra/sp/sprite.c:302-386),
    not "stack each band under the last".  The source advances the destination
    row by `s->bmheight` on every wrap and draws each band `b->actualHeight`
    tall, so consecutive bands OVERLAP when the two differ, and it stops when
    the next band would not fit inside `s->height`:

        y += s->bmheight;                     /* wrap to the next band row */
        bh = b->actualHeight ? : s->bmheight;
        if ((y + bh) > s->height) break;      /* can't wrap any more       */

    Both live assets differ: the 300x220 CI4 menu collage is 44 bands of 6 rows
    stepping 5 (44 * 5 = 220 exactly, with the last band 5 tall so the final
    row lands on 219), and the 45x43 CSS portraits are bands of 21/21/3 rows
    stepping 20.  Stepping by the band height instead put the portraits' lower
    two thirds one row low and clipped the last band from three rows to one --
    a real defect in the shipped P2-1e bake, fixed here because the collage
    forced the policy to be read rather than assumed.
    """
    sprite = fileobj.sprite(offset)
    if sprite.nbitmaps < 1:
        raise ConvertError(f"{symbol}: no bitmaps")
    width = sprite.width
    height = sprite.height
    if tint is not None:
        sprite = replace(sprite, red=tint[0], green=tint[1], blue=tint[2])
    raster = [[(0, 0, 0, 0)] * width for _ in range(height)]

    row = 0
    for index in range(sprite.nbitmaps):
        piece = fileobj.bitmap(sprite.bitmap + index * BITMAP_BYTES)
        if piece.width <= 0:
            break  # spDraw's own loop guard: `b->width > 0`
        band_h = piece.actual_height if piece.actual_height != 0 \
            else sprite.bmheight
        if index > 0:
            # Every band here is full sprite width, so spDraw's horizontal
            # `x += b->width` wrap fires on every one of them.  A narrower
            # band would mean a 2D band grid, which nothing in this pack has
            # and which is a falsifier rather than a silent half-decode.
            if piece.width != sprite.width:
                raise ConvertError(
                    f"{symbol}: band {index} is {piece.width} wide against a "
                    f"{sprite.width}-wide sprite (2D band grid unsupported)")
            row += sprite.bmheight
            if (row + band_h) > height:
                break
        piece_h = min(band_h, height - row)
        if piece_h <= 0:
            break
        piece_w = min(piece.width, width)
        decode_band(fileobj, sprite, piece, symbol, raster, row,
                    piece_w, piece_h, alpha_ramp)
    return sprite, raster


def decode_band(fileobj: RelocFile, sprite: Sprite, piece: Bitmap, symbol: str,
                raster: list[list[tuple[int, int, int, int]]], row: int,
                piece_w: int, piece_h: int, alpha_ramp: bool = False) -> None:
    """One band into `raster` at `row`, in the sprite's own pixel format.

    `alpha_ramp` picks which I4 alpha rule applies.  The RDP's is the ramp --
    `G_CC_MODULATEIDECALA_PRIM` takes alpha straight from TEXEL0, and for an
    I4 texel that is the intensity.  A bitmap-OBJ cell has ONE alpha bit and
    is not composited offline, so the OBJ path keeps `any ink is ink`: at 4/5
    a menu digit is six pixels wide and thresholding its ramp at half would
    eat the strokes.  Surfaces composite here against a known field, so they
    take the ramp and the difference is antialiasing the OBJ cells cannot
    carry.
    """
    if sprite.bmfmt == G_IM_FMT_I and sprite.bmsiz == G_IM_SIZ_4b:
        for y, pixels in enumerate(
                decode_i4(fileobj, piece, sprite, piece_w, piece_h)):
            for x, i in enumerate(pixels):
                raster[row + y][x] = (
                    (sprite.red * i) // 255, (sprite.green * i) // 255,
                    (sprite.blue * i) // 255,
                    i if alpha_ramp else (255 if i else 0))
    elif sprite.bmfmt == G_IM_FMT_IA and sprite.bmsiz == G_IM_SIZ_8b:
        for y, pixels in enumerate(
                decode_ia8(fileobj, piece, sprite, piece_w, piece_h)):
            for x, (i, a) in enumerate(pixels):
                raster[row + y][x] = (
                    (sprite.red * i) // 255, (sprite.green * i) // 255,
                    (sprite.blue * i) // 255, a)
    elif sprite.bmfmt == G_IM_FMT_IA and sprite.bmsiz == G_IM_SIZ_16b:
        for y, pixels in enumerate(
                decode_ia16(fileobj, piece, sprite, piece_w, piece_h)):
            for x, (i, a) in enumerate(pixels):
                raster[row + y][x] = (
                    (sprite.red * i) // 255, (sprite.green * i) // 255,
                    (sprite.blue * i) // 255, a)
    elif sprite.bmfmt == G_IM_FMT_RGBA and sprite.bmsiz == G_IM_SIZ_32b:
        for y, pixels in enumerate(
                decode_rgba32(fileobj, piece, sprite, piece_w, piece_h)):
            for x, (r, g, b, a) in enumerate(pixels):
                raster[row + y][x] = (r, g, b, a)
    elif sprite.bmfmt == G_IM_FMT_RGBA and sprite.bmsiz == G_IM_SIZ_16b:
        for y, pixels in enumerate(
                decode_rgba16(fileobj, piece, sprite, piece_w, piece_h)):
            for x, (r, g, b, a) in enumerate(pixels):
                raster[row + y][x] = (r, g, b, a)
    elif sprite.bmfmt == G_IM_FMT_CI and sprite.bmsiz == G_IM_SIZ_4b:
        for y, pixels in enumerate(
                decode_ci4(fileobj, piece, sprite, piece_w, piece_h)):
            for x, (r, g, b, a) in enumerate(pixels):
                raster[row + y][x] = (r, g, b, a)
    else:
        raise ConvertError(
            f"{symbol}: unsupported fmt={sprite.bmfmt} siz={sprite.bmsiz}")


def convert_image(fileobj: RelocFile, symbol: str, token: str, offset: int,
                  scale: tuple[int, int] | None = None,
                  tint: tuple[int, int, int] | None = None) -> Image:
    """Decode one sprite to an RGBA raster, optionally rescale it, then pack it
    into the smallest DS OBJ cell that holds the result.

    THE SCALE IS THE FRAME RATIO, NOT A TASTE CALL.  The DS screen is 256x192
    and the N64 frame these menus are laid out in is 320x240 -- exactly 0.8 on
    both axes -- so `4/5` reproduces a source sprite at its own relative size on
    the smaller screen.  The CSS portraits take a further step down (32/45) for
    a hardware reason and not an aesthetic one: 45x43 lands in a 64x64 OBJ cell
    (8,192 B) and 32x31 lands in a 32x32 one (2,048 B), and twelve portrait
    cells at the larger size do not fit main OBJ VRAM beside the text budget.
    """
    # THE PRIMITIVE COLOUR AN INTENSITY SPRITE IS MODULATED BY.  An I/IA sprite
    # carries SHAPE only and the drawing code sets `sobj->sprite.red/green/blue`
    # per SObj (mnmaps.c:340); the container's own values are whatever the
    # asset was authored with.  `tint` is that draw-site colour, quoted from the
    # source that sets it, and it is baked in because the kit's image path has
    # no per-slot tint -- unlike its text path, which takes the colour as an
    # argument exactly because a string IS re-tinted per use.
    sprite, raster = decode_sprite_raster(fileobj, symbol, offset, tint)
    width = sprite.width
    height = sprite.height


    if scale is not None:
        num, den = scale
        width = max(1, (sprite.width * num + den // 2) // den)
        height = max(1, (sprite.height * num + den // 2) // den)
        raster = box_scale(raster, width, height)

    cell_w, cell_h = choose_cell(width, height)
    texels = [0] * (cell_w * cell_h)
    for y in range(height):
        for x in range(width):
            texels[y * cell_w + x] = rgba8_to_ds(*raster[y][x])
    return Image(symbol, token, width, height, cell_w, cell_h, texels)


# ---------------------------------------------------------------------------
# Surfaces (menu backdrop art) -> DS BG2 bitmap rows
# ---------------------------------------------------------------------------

# The DS frame is 256x192 and the N64 frame these menus are laid out in is
# 320x240 -- exactly 4/5 on both axes -- so a surface is baked at 4/5 and
# placed at 4/5 of the source's own top-left.  Every position below is derived
# from the source, never nudged: the collage from `sobj->pos.x/y = 10.0F`
# (mnmodeselect.c:527, mnvsmode.c:974) and the title set from
# `mnTitleSetPosition`, which is `desc->pos - size * 0.5` over the CENTRES in
# `dMNTitleCommonSpriteDescs` (mntitle.c:64, :792).
FRAME_SCALE = (4, 5)
DS_SCREEN_W = 256
DS_SCREEN_H = 192


def frame_pos(value: int) -> int:
    """One N64 frame coordinate at the DS frame's 4/5, rounded half up."""
    num, den = FRAME_SCALE
    if value < 0:
        return -((-value * num + den // 2) // den)
    return (value * num + den // 2) // den


@dataclass(frozen=True)
class Placement:
    """One source sprite inside a surface, with its own draw-site state.

    THE COMBINER IS THE SOURCE'S, and it is not the OBJ path's.  spDraw sets
    `G_CC_MODULATEIDECALA_PRIM` for every sprite whose `alpha == 255`
    (libultra/sp/sprite.c:213-232, gbi.h:518), which is
    `colour = TEXEL0 * PRIMITIVE`, `alpha = TEXEL0`.  So an I4 sprite is a
    PRIMITIVE-coloured stencil whose alpha is its own intensity ramp -- not
    the hard-edged `alpha = 255 if intensity else 0` an OBJ cell has to settle
    for, because a bitmap-OBJ texel carries one alpha bit and a surface is
    composited here, offline, against a known field.

    `flat` and `alpha` exist for one sprite: the title emblem draws through
    `mnTitleLogoProcDisplay`'s OWN combiner (mntitle.c:1023), which is
    `colour = PRIMITIVE` flat and `alpha = TEXEL0 * PRIMITIVE_alpha`, with the
    primitive alpha being `sMNTitleLogoAlpha` -- a value the title fades from
    0xFF and clamps at 0x4C (mntitle.c:1042), so 0x4C is its RESTING value and
    the one a static substitute must use.
    """
    o2r: str
    symbol: str
    x: int          # in the source's own 320x240 frame
    y: int
    centred: bool   # title descs are centres; the collage's draw site is not
    tint: tuple[int, int, int] | None = None
    alpha: int = 255
    flat: bool = False


def place_raster(fileobj: RelocFile, part: Placement, offset: int
                 ) -> tuple[int, int, list[list[tuple[int, int, int, int]]]]:
    """One placement decoded, combined and scaled to the DS frame's 4/5.

    Decoded with a NEUTRAL primitive so what comes back is TEXEL0 itself; the
    real primitive is applied once here.  Feeding the tint into the decoder
    instead would modulate twice for any sprite whose container carries a
    non-white primitive of its own.
    """
    sprite, raster = decode_sprite_raster(fileobj, part.symbol, offset,
                                          (255, 255, 255), alpha_ramp=True)
    prim = part.tint if part.tint is not None else (sprite.red, sprite.green,
                                                    sprite.blue)
    combined = []
    for row in raster:
        out = []
        for (r, g, b, a) in row:
            if part.flat:
                out.append((prim[0], prim[1], prim[2],
                            (a * part.alpha) // 255))
            else:
                out.append(((r * prim[0]) // 255, (g * prim[1]) // 255,
                            (b * prim[2]) // 255, (a * part.alpha) // 255))
        combined.append(out)
    num, den = FRAME_SCALE
    width = max(1, (sprite.width * num + den // 2) // den)
    height = max(1, (sprite.height * num + den // 2) // den)
    left = part.x - (sprite.width // 2) if part.centred else part.x
    top = part.y - (sprite.height // 2) if part.centred else part.y
    return (frame_pos(left), frame_pos(top),
            box_scale(combined, width, height))


@dataclass(frozen=True)
class SurfaceSpec:
    token: str
    parts: tuple[Placement, ...]
    # The field the surface is composited over, or None to keep alpha keyed.
    # An opaque field makes the result opaque, which lets the runtime DMA whole
    # rows instead of testing every texel.
    background: tuple[int, int, int] | None = None
    # A surface the runtime keeps in RAM so it can be toggled without a NitroFS
    # read. Exactly one element needs it -- PRESS START blinks -- and the
    # manifest sizes the runtime's single cache buffer from this flag, so a
    # second cacheable surface costs .bss rather than silently overflowing.
    cacheable: bool = False


def convert_surface(cache: dict[str, RelocFile], offsets: dict[str, int],
                    repo_root: Path, spec: SurfaceSpec) -> Surface:
    """Composite one surface: the union of its placements, resolved offline.

    The canvas is the union bounding box of the placements, clipped to the DS
    screen -- so nothing bakes a margin of flat backdrop that main BG palette
    entry 0 already paints for free, and the title's emblem keeps the same
    right-edge clip the source's own 320-wide frame gives it.

    Compositing HERE rather than at runtime is what makes the emblem's 0x4C
    primitive alpha and every antialiased sprite edge survive the trip: the DS
    bitmap the result lands in carries one alpha bit per texel, so a blend
    performed on the console would have nothing to blend with.
    """
    placed = []
    for part in spec.parts:
        if part.symbol not in offsets:
            raise ConvertError(
                f"{part.symbol} missing from include/reloc_data.h and "
                "reloc_data.us.h")
        if part.o2r not in cache:
            cache[part.o2r] = RelocFile(
                repo_root / "decomp" / "BattleShip-main" / "BattleShip_o2r" /
                "reloc_menus" / part.o2r)
        placed.append(place_raster(cache[part.o2r], part,
                                   offsets[part.symbol]))

    left = min(x for x, _, _ in placed)
    top = min(y for _, y, _ in placed)
    right = max(x + len(raster[0]) for x, _, raster in placed)
    bottom = max(y + len(raster) for _, y, raster in placed)
    left = max(left, 0)
    top = max(top, 0)
    right = min(right, DS_SCREEN_W)
    bottom = min(bottom, DS_SCREEN_H)
    width = right - left
    height = bottom - top
    if (width <= 0) or (height <= 0):
        raise ConvertError(f"{spec.token}: composites to an empty canvas")
    # A SURFACE ORIGIN IS NEVER NEGATIVE, and the runtime relies on it: its
    # erase path clamps a negative x to 0 without shortening the run, which
    # would spill the field colour past the surface's right edge.  The clamps
    # above already guarantee this; asserting it is what keeps a future change
    # to them from turning a bake into a drawing bug nobody looks for.
    if (left < 0) or (top < 0):
        raise ConvertError(
            f"{spec.token}: origin ({left},{top}) is off the top-left of the "
            "screen; the runtime's erase path assumes a non-negative origin")

    field = spec.background
    canvas = [[(field[0], field[1], field[2], 255) if field is not None
               else (0, 0, 0, 0) for _ in range(width)] for _ in range(height)]
    for x0, y0, raster in placed:
        for sy, row in enumerate(raster):
            dy = y0 + sy - top
            if (dy < 0) or (dy >= height):
                continue
            target = canvas[dy]
            for sx, (r, g, b, a) in enumerate(row):
                if a == 0:
                    continue
                dx = x0 + sx - left
                if (dx < 0) or (dx >= width):
                    continue
                if a == 255:
                    target[dx] = (r, g, b, 255)
                    continue
                dr, dg, db, da = target[dx]
                inv = 255 - a
                out_a = a + ((da * inv) // 255)
                if out_a == 0:
                    target[dx] = (0, 0, 0, 0)
                    continue
                target[dx] = (
                    ((r * a) + ((dr * da * inv) // 255)) // out_a,
                    ((g * a) + ((dg * da * inv) // 255)) // out_a,
                    ((b * a) + ((db * da * inv) // 255)) // out_a,
                    out_a)

    texels = [0] * (width * height)
    opaque = True
    for y in range(height):
        for x in range(width):
            texel = rgba8_to_ds(*canvas[y][x])
            texels[y * width + x] = texel
            if texel == 0:
                opaque = False
    return Surface(spec.token, spec.token, width, height, left, top, opaque,
                   texels)


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


def build_surface_pack(surfaces: list[Surface]
                       ) -> tuple[bytes, list[tuple[str, int, int, int]]]:
    """A SEPARATE blob from the OBJ pack, and that is the point.

    `ndsUiKitEnter` reads and hashes the whole OBJ pack on every screen entry,
    so a backdrop living in it would cost every screen the bytes of a screen it
    does not show.  Surfaces are streamed individually by index instead: the
    title reads its own set, the two collage screens read one surface, and the
    character/stage selects read none.  Each entry carries its own FNV-1a so a
    single blit is verifiable on its own rather than only in aggregate.
    """
    blob = bytearray()
    table: list[tuple[str, int, int, int]] = []
    for surface in surfaces:
        offset = len(blob)
        for texel in surface.texels:
            blob += struct.pack("<H", texel)
        payload = bytes(blob[offset:])
        table.append((surface.name, offset, len(payload), fnv1a32(payload)))
    return bytes(blob), table


def emit_manifest(path: Path, glyphs: list[Glyph], images: list[Image],
                  image_table: list[tuple[str, int, int]],
                  pack: bytes, surfaces: list[Surface],
                  surface_table: list[tuple[str, int, int, int]],
                  surface_pack: bytes) -> None:
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
    lines.append(f"#define NDS_MN_UI_KIT_SURFACE_PACK_BYTES "
                 f"{len(surface_pack)}u")
    lines.append(f"#define NDS_MN_UI_KIT_SURFACE_COUNT {len(surfaces)}u")
    lines.append(f"#define NDS_MN_UI_KIT_SURFACE_MAX_ROW_BYTES "
                 f"{max((s.width * 2 for s in surfaces), default=0)}u")
    lines.append(f"#define NDS_MN_UI_KIT_SURFACE_MAX_BYTES "
                 f"{max((s.width * s.height * 2 for s in surfaces), default=0)}u")
    cacheable = [spec.token for spec in SURFACE_SOURCES if spec.cacheable]
    lines.append(f"#define NDS_MN_UI_KIT_SURFACE_CACHE_BYTES "
                 f"{max((s.width * s.height * 2 for s in surfaces
                        if s.token in cacheable), default=0)}u")
    lines.append("")
    lines.append("/* Backdrop surfaces, DS BGR5551, drawn into the main BG2 "
                 "bitmap. */")
    lines.append("static const NdsUiKitSurfaceMetric kNdsUiKitSurfaceMetrics"
                 "[NDS_MN_UI_KIT_SURFACE_COUNT] __attribute__((unused)) = {")
    for surface, (name, offset, size, hash32) in zip(surfaces, surface_table):
        lines.append(
            f"    {{ {offset}u, {size}u, {surface.width}u, {surface.height}u, "
            f"{surface.dst_x}, {surface.dst_y}, "
            f"{1 if surface.opaque else 0}u, 0x{hash32:08x}u }}, /* {name} */")
    lines.append("};")
    lines.append("")
    for index, surface in enumerate(surfaces):
        lines.append(
            f"#define NDS_MN_UI_KIT_SURFACE_{surface.token} {index}u")
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
    # (o2r file, reloc_data.h symbol, NDS_MN_UI_KIT_IMAGE_<token>[, scale
    #  [, tint]])
    # `scale` is an exact (numerator, denominator); absent means 1:1.
    # `tint` is the (r, g, b) the SOURCE modulates an intensity sprite by at its
    # draw site; absent keeps the container's own primitive colour.
    ("MNPlayersCommon", "llMNPlayersCommonCursorHandPointSprite",
     "CURSOR_HAND_POINT"),
    ("MNPlayersCommon", "llMNPlayersCommonCursorHandGrabSprite",
     "CURSOR_HAND_GRAB"),
    # P2-1e. The CSS draws twelve 45x43 portraits across a 256 px screen, which
    # the source does across 320, so these two come down to 32/45 -- the ratio
    # that lands them in a 32x32 OBJ cell.  Nothing else in the tree drew them
    # (P2-1c baked them for its demo, P2-1d drew none), so this is a resize of
    # an unused asset rather than a change to a shipped screen.
    ("MNPlayersPortraits", "llMNPlayersPortraitsMarioSprite",
     "PORTRAIT_MARIO", (32, 45)),
    ("MNPlayersPortraits", "llMNPlayersPortraitsFoxSprite", "PORTRAIT_FOX",
     (32, 45)),
    # P2-1d. The menu font has NO digits: mnMapsGetCharacterID maps A-Z ' % .
    # and nothing else, and '0'-'9' are the source's kerning ESCAPES, which
    # advance the cursor by their digit value and draw nothing (mnmaps.c:308).
    # So every number the VS rules screen shows -- the time limit, the stock
    # count -- has to come from the same place the original got it: MNCommon's
    # own digit sprites, which mnVSModeMakeNumber draws one SObj at a time
    # (mnvsmode.c:173).  They are 10x13, which does not fit the kit's 8-row
    # text cell, so they are baked as IMAGES and drawn as one OBJ per digit --
    # which is exactly the original's own structure.
    ("MNCommon", "llMNCommonDigit0Sprite", "DIGIT_0"),
    ("MNCommon", "llMNCommonDigit1Sprite", "DIGIT_1"),
    ("MNCommon", "llMNCommonDigit2Sprite", "DIGIT_2"),
    ("MNCommon", "llMNCommonDigit3Sprite", "DIGIT_3"),
    ("MNCommon", "llMNCommonDigit4Sprite", "DIGIT_4"),
    ("MNCommon", "llMNCommonDigit5Sprite", "DIGIT_5"),
    ("MNCommon", "llMNCommonDigit6Sprite", "DIGIT_6"),
    ("MNCommon", "llMNCommonDigit7Sprite", "DIGIT_7"),
    ("MNCommon", "llMNCommonDigit8Sprite", "DIGIT_8"),
    ("MNCommon", "llMNCommonDigit9Sprite", "DIGIT_9"),
    # SCBATTLE_TIMELIMIT_INFINITE draws this instead of a number
    # (mnvsmode.c:1206).
    ("MNCommon", "llMNCommonInfinitySprite", "INFINITY"),
    # ---- P2-1e, the character-select screen. -----------------------------
    # THE LOCKED SLOT.  mnPlayersVSMakePortrait branches on
    # mnPlayersVSCheckFighterLocked and draws the shadow/question-mark stack
    # instead of the portrait (mnplayersvs.c:429/:374); the question mark is
    # the part a player reads as "locked", so it is the one this bakes.  It is
    # IA8, which is why decode_ia8 exists.
    ("MNPlayersPortraits", "llMNPlayersPortraitsPortraitQuestionMarkSprite",
     "PORTRAIT_LOCKED", (32, 45)),
    # THE THREE CURSOR STATES.  mnPlayersVSUpdateCursor indexes exactly these
    # three sprites by cursor_status (mnplayersvs.c:1723): Pointer=0, Grab=1,
    # Hover=2.  They are baked at the 4/5 frame ratio because the CSS hand has
    # to read as a hand ON a 32 px portrait cell; the unscaled pair above stays
    # untouched because P2-1d's mode-select and VS screens draw it at 1:1 and
    # their cursor offsets are tuned to that cell.
    ("MNPlayersCommon", "llMNPlayersCommonCursorHandPointSprite",
     "CSS_CURSOR_POINT", (4, 5)),
    ("MNPlayersCommon", "llMNPlayersCommonCursorHandGrabSprite",
     "CSS_CURSOR_GRAB", (4, 5)),
    ("MNPlayersCommon", "llMNPlayersCommonCursorHandHoverSprite",
     "CSS_CURSOR_HOVER", (4, 5)),
    # THE TOKENS.  mnPlayersVSUpdatePuck indexes 1P..4P then CP
    # (mnplayersvs.c:3434); this build fills two slots, so it bakes the first
    # player's token and the CP one.  P2-2 adds 2P/3P/4P with the fourth slot.
    ("MNPlayersCommon", "llMNPlayersCommon1PPuckSprite", "PUCK_1P", (4, 5)),
    ("MNPlayersCommon", "llMNPlayersCommonCPPuckSprite", "PUCK_CP", (4, 5)),
    # THE PLAYER-KIND BUTTON.  mnPlayersVSMakePlayerKindSelect indexes these
    # three by pkind (mnplayersvs.c:934), so the block order is
    # Man/Com/Not exactly as nFTPlayerKind* is ordered.
    #
    # THESE FOUR STAY 1:1 while everything around them takes the 4/5 frame
    # ratio, and that is a legibility call made against the preview PNGs, not
    # an oversight: they are 8-to-11-pixel-tall LETTERFORMS, already the
    # smallest text the original menus draw, and at 4/5 "CP LEVEL" loses its
    # first word and "HMN" closes up.  `PROJECT_GOAL.md` asks for a result that
    # stays READABLE, and a fixed-size label is exactly the element a uniform
    # layout scale should not carry.  The cells are the same size either way
    # for CP LEVEL, so the whole cost of the decision is 2,304 bytes.
    ("MNPlayersCommon", "llMNPlayersCommonHmnLabelSprite", "LABEL_HMN"),
    ("MNPlayersCommon", "llMNPlayersCommonCPLabelSprite", "LABEL_CP"),
    ("MNPlayersCommon", "llMNPlayersCommonNALabelSprite", "LABEL_NA"),
    # THE CPU-LEVEL LABEL, mnplayersvs.c:2762.  The handicap twin is not baked:
    # handicap is off in every configuration this build reaches and the row it
    # would draw belongs to P2-5/P2-7's options work.
    ("MNPlayersCommon", "llMNPlayersCommonCPLevelTextSprite", "CP_LEVEL"),
    # ---- P2-1f, the stage select. ----------------------------------------
    # THE SCALE IS 5/8, AND IT IS A CELL FACT, not a taste call.  mnMapsMakeIcons
    # draws 48x36 icons on a 50 px pitch and mnMapsMakeCursor a 62x50 frame
    # around them (mnmaps.c:530/:865); at the frame's own 4/5 the cursor is
    # 50x40 and lands in a 64x64 OBJ cell (8,192 B), while at 5/8 it is 39x31
    # and lands in a 64x32 one (4,096 B) with the icons at 30x23 in 32x32
    # cells.  5/8 is the largest exact ratio at which the SOURCE'S OWN cursor
    # fits a single 64x32 cell, and main OBJ VRAM has 16,640 B free after the
    # P2-1e pack -- so 4/5 for this set would have cost 16,384 of it for three
    # sprites.  The icons keep the source's own 4/5 GRID positions and are
    # centred in the 4/5 footprint, so the layout is the source's and only the
    # artwork inside each cell is smaller.
    #
    # Two new pixel formats arrive with these, and both are the sprite's own:
    # the map icons are RGBA16 (fmt=0 siz=2) and the RANDOM icon is CI4 with a
    # 256-entry TLUT (fmt=2 siz=0).  decode_rgba16/decode_ci4 above.
    ("MNMaps", "llMNMapsDreamLandSprite", "MAP_DREAM_LAND", (5, 8)),
    ("MNMaps", "llMNMapsRandomSmallSprite", "MAP_RANDOM", (5, 8)),
    # The cursor is I4 -- shape only -- and mnMapsMakeCursor modulates it by
    # pure RED (mnmaps.c:876: red 0xFF, green 0x00, blue 0x00).  Baked in,
    # because the kit's image path draws a cell as it is packed.
    ("MNMaps", "llMNMapsCursorSprite", "MAP_CURSOR", (5, 8), (0xFF, 0, 0)),
]

# P2-1h.  THE ORIGINAL PRESENTATION ART (owner ruling, 2026-08-18: this is a
# port, so the first-party branding, logos and copyright line ship, converted
# from source like every other asset).
#
# TITLE_FIELD is the flat field the title art composites over.  The original
# burns a thirty-frame fire animation there (`mnTitleMakeFire`, two 32x32
# RGBA32 sprites blown up 12x/8.5x), which `PROJECT_GOAL.md` names twice as
# sacrificable -- "reduced background animation", "static substitutes for
# expensive animation".  It is DELIBERATELY ABSENT and recorded as such rather
# than approximated badly; black is what the fire itself burns over.
TITLE_FIELD = (0x00, 0x00, 0x00)

# THE TITLE, in the source's own draw order.  `mnTitleMakeLabels` builds kinds
# 0..4 into one GObj -- which is what puts the black drop-shadow cutout BEHIND
# the wordmark -- then 5..6; `mnTitleMakePressStart` adds 7 and
# `mnTitleMakeLogoNoOpening` adds 8, the branch this build takes because
# `scene_prev` cannot be the opening cinematic until P2-7 lands (mntitle.c
# :1051, :1179, :1203, :1221, :1255).  Colours are `mnTitleSetColors`
# (mntitle.c:800); positions are the CENTRES in `dMNTitleCommonSpriteDescs`
# (mntitle.c:64), which `mnTitleSetPosition` turns into a top-left.
#
# Kind 9 (TM2) is deliberately absent, and that is a finding rather than an
# omission: it has a desc entry AND a colour case, and nothing in the US build
# ever constructs it -- `mnTitleMakeSprites`, the only loop that would reach
# it, stops at PressStart and is marked unused in the source itself.
TITLE_PARTS = (
    Placement("MNTitle", "llMNTitleCutoutSprite", 157, 94, True,
              (0x00, 0x00, 0x00)),
    Placement("MNTitle", "llMNTitleSmashSprite", 161, 88, True,
              (0xFF, 0xFE, 0x2A)),
    Placement("MNTitle", "llMNTitleSuperSprite", 55, 96, True,
              (0xFF, 0xFE, 0x2A)),
    Placement("MNTitle", "llMNTitleBrosSprite", 268, 96, True,
              (0xFF, 0xFE, 0x2A)),
    Placement("MNTitle", "llMNTitleTMUnkSprite", 270, 132, True,
              (0x00, 0x00, 0x00)),
    Placement("MNTitle", "llMNTitleCopyrightSprite", 160, 208, True,
              (0xB7, 0xAE, 0x7C)),
    Placement("MNTitle", "llMNTitleBorderUpperSprite", 160, 15, True,
              (0x14, 0x12, 0x06)),
    # The emblem sets its own primitive (mntitle.c:1073) and draws through its
    # own flat combiner at the resting alpha the title fades it to.
    Placement("MNTitle", "llMNTitleLogoAnimFullSprite", 260, 60, True,
              (0xFF, 0x00, 0x00), alpha=0x4C, flat=True),
)

SURFACE_SOURCES = [
    SurfaceSpec("TITLE_SCREEN", TITLE_PARTS, TITLE_FIELD),
    # PRESS START blinks, so it is its own surface: the runtime caches these
    # bytes and redraws or erases them in place, while everything above is
    # composed once per entry and then never touched again.  Composited over
    # the same field, so erasing it is a fill with that field's texel.
    SurfaceSpec("TITLE_PRESS_START",
                (Placement("MNTitle", "llMNTitlePressStartSprite", 162, 177,
                           True, (0xFF, 0xFF, 0xFF)),),
                TITLE_FIELD, cacheable=True),
    # THE MENU COLLAGE.  300x220 CI4 with a 16-entry TLUT, 44 bands of 6 rows
    # stepping 5 -- the asset that forced decode_sprite_raster to transcribe
    # spDraw's band policy.  Both screens that show it place it identically at
    # (10, 10) (mnmodeselect.c:527, mnvsmode.c:974), top-left and untinted, and
    # the source leaves its own 10 px margin showing the decal blue, which the
    # DS backdrop paints at zero cost.
    SurfaceSpec("MENU_COLLAGE",
                (Placement("MNCommon", "llMNCommonSmashBrosCollageSprite",
                           10, 10, False),),
                None),
]


# The digit block must stay contiguous and in ascending order: the runtime
# indexes it as NDS_MN_UI_KIT_IMAGE_DIGIT_0 + digit.  Asserted rather than
# assumed, because a reordered IMAGE_SOURCES would otherwise print 7 for 3.
DIGIT_TOKENS = [f"DIGIT_{d}" for d in range(10)]


def check_digit_block(images: list[Image]) -> None:
    tokens = [image.token for image in images]
    if not all(token in tokens for token in DIGIT_TOKENS):
        raise ConvertError("the digit images are missing from the pack")
    first = tokens.index("DIGIT_0")
    if tokens[first:first + 10] != DIGIT_TOKENS:
        raise ConvertError(
            "DIGIT_0..9 must be ten consecutive entries in ascending order; "
            f"got {tokens[first:first + 10]}")


# The player-kind labels must stay contiguous and in nFTPlayerKind order: the
# runtime indexes them as NDS_MN_UI_KIT_IMAGE_LABEL_HMN + pkind, exactly as
# mnPlayersVSMakePlayerKindSelect indexes its own offsets[] by pkind.
KIND_LABEL_TOKENS = ["LABEL_HMN", "LABEL_CP", "LABEL_NA"]


def check_kind_label_block(images: list[Image]) -> None:
    tokens = [image.token for image in images]
    if not all(token in tokens for token in KIND_LABEL_TOKENS):
        raise ConvertError("the player-kind labels are missing from the pack")
    first = tokens.index("LABEL_HMN")
    if tokens[first:first + 3] != KIND_LABEL_TOKENS:
        raise ConvertError(
            "LABEL_HMN/CP/NA must be three consecutive entries in "
            f"nFTPlayerKind order; got {tokens[first:first + 3]}")


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


def write_surface_previews(out_dir: Path, surfaces: list[Surface]) -> None:
    """One PNG per surface, plus the composed 256x192 title/menu screens.

    The composite is what actually catches a wrong position or a wrong draw
    order: a per-sprite PNG cannot show that the black cutout ended up ON TOP
    of the wordmark.  Composition here is the runtime's own rule -- in list
    order, skipping transparent texels, clipped to the DS screen.
    """
    backdrop = (255, 0, 255)
    for surface in surfaces:
        pixels = bytearray()
        for texel in surface.texels:
            pixels += bytes(ds_texel_to_rgb(texel, backdrop))
        write_png(out_dir / f"mn_ui_kit_{surface.token}.png", surface.width,
                  surface.height, bytes(pixels))

    by_token = {surface.token: surface for surface in surfaces}
    scenes = {
        "screen_title": (TITLE_FIELD,
                         ["TITLE_SCREEN", "TITLE_PRESS_START"]),
        # The menus' own decal blue, mnmodeselect.c:517 (0x083365).
        "screen_menu": ((0x08, 0x33, 0x65), ["MENU_COLLAGE"]),
    }
    for name, (field, tokens) in scenes.items():
        if not all(token in by_token for token in tokens):
            continue
        frame = [[field] * 256 for _ in range(192)]
        for token in tokens:
            surface = by_token[token]
            for y in range(surface.height):
                dy = surface.dst_y + y
                if (dy < 0) or (dy >= 192):
                    continue
                for x in range(surface.width):
                    dx = surface.dst_x + x
                    if (dx < 0) or (dx >= 256):
                        continue
                    texel = surface.texels[y * surface.width + x]
                    if texel != 0:
                        frame[dy][dx] = ds_texel_to_rgb(texel, field)
        rgb = bytearray()
        for row in frame:
            for pixel in row:
                rgb += bytes(pixel)
        write_png(out_dir / f"mn_ui_kit_{name}.png", 256, 192, bytes(rgb))


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
        # By TOKEN, not by symbol: one sprite can be baked twice at different
        # scales (the CSS cursor set) and a symbol-named file would silently
        # overwrite the other bake instead of showing both.
        write_png(out_dir / f"mn_ui_kit_{image.token}.png", image.cell_w,
                  image.cell_h, bytes(pixels))


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
    for entry in IMAGE_SOURCES:
        o2r_name, symbol, token = entry[0], entry[1], entry[2]
        scale = entry[3] if len(entry) > 3 else None
        tint = entry[4] if len(entry) > 4 else None
        if symbol not in offsets:
            raise ConvertError(f"{symbol} missing from include/reloc_data.h")
        if o2r_name not in cache:
            cache[o2r_name] = RelocFile(
                repo_root / "decomp" / "BattleShip-main" / "BattleShip_o2r" /
                "reloc_menus" / o2r_name)
        images.append(convert_image(cache[o2r_name], symbol, token,
                                    offsets[symbol], scale, tint))
    check_digit_block(images)
    check_kind_label_block(images)

    surfaces = [convert_surface(cache, offsets, repo_root, spec)
                for spec in SURFACE_SOURCES]

    pack, image_table = build_pack(glyphs, images)
    surface_pack, surface_table = build_surface_pack(surfaces)

    if args.preview_dir is not None:
        write_previews(args.preview_dir.resolve(), glyphs, images)
        write_surface_previews(args.preview_dir.resolve(), surfaces)

    if args.preview:
        preview_glyphs(glyphs)
        for image, (name, offset, size) in zip(images, image_table):
            print(f"-- {name} src {image.src_w}x{image.src_h} "
                  f"cell {image.cell_w}x{image.cell_h} "
                  f"offset {offset} bytes {size}")
        for surface, (name, offset, size, hash32) in zip(surfaces,
                                                         surface_table):
            print(f"== {name} {surface.width}x{surface.height} at "
                  f"({surface.dst_x},{surface.dst_y}) "
                  f"{'opaque' if surface.opaque else 'keyed'} "
                  f"offset {offset} bytes {size} fnv32 0x{hash32:08x}")
        print(f"pack {len(pack)} bytes fnv32 0x{fnv1a32(pack):08x}")
        print(f"surfaces {len(surface_pack)} bytes")
        return 0

    write_bytes_if_changed(repo_root / "assets" / "menus" / "mn_ui_kit.bin",
                           pack)
    write_bytes_if_changed(repo_root / "assets" / "menus" / "mn_surfaces.bin",
                           surface_pack)
    emit_manifest(repo_root / "src" / "nds" / "generated" /
                  "mn_ui_kit.generated.inc", glyphs, images, image_table, pack,
                  surfaces, surface_table, surface_pack)
    print(f"mn_ui_kit: {len(glyphs)} glyphs, {len(images)} images, "
          f"{len(pack)} bytes, fnv32 0x{fnv1a32(pack):08x}; "
          f"{len(surfaces)} surfaces, {len(surface_pack)} bytes")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ConvertError as exc:
        print(f"generate_mn_ui_kit: {exc}", file=sys.stderr)
        sys.exit(1)
