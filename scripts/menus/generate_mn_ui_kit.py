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
from dataclasses import dataclass, field, replace
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


def decode_ia4(fileobj: RelocFile, bitmap: Bitmap, sprite: Sprite,
               width: int, height: int) -> list[list[tuple[int, int]]]:
    """4-bit IA (3 intensity + 1 alpha in one nibble) -> rows of (i, a).

    P2-1j.  The option-tab MIDDLE is the only IA4 asset the shell reaches
    (`llMNCommonOptionTabMiddleSprite`, 8x29 fmt=IA siz=4b) and it is the strip
    the source TILES across a button's whole width, so nothing about the tab
    can be baked without it.  The nibble is the RDP's own IA4 layout: the top
    three bits are the intensity ramp over 0..7 and the bottom bit is a hard
    alpha, which is why the tab's interior is solid and only its rounded caps
    carry an edge.
    """
    stride = max(1, bitmap.width_img // 2)
    rows = []
    for y in range(height):
        raw = read_row(fileobj, bitmap, sprite, y, stride)
        row = []
        for x in range(width):
            sx = bitmap.s + x
            nib = raw[sx >> 1]
            nib = (nib >> 4) if (sx & 1) == 0 else (nib & 0xF)
            row.append((((nib >> 1) & 0x7) * 255 // 7, 255 if (nib & 1) else 0))
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


# P2-1k.  The o2r containers this bake reads live in two sibling directories,
# and the split is the source tree's own: the menu art is `reloc_menus`, while
# the series emblems `mnPlayersVSMakeNameAndEmblem` (mnplayersvs.c:613) and
# `mnMapsMakeEmblem` (mnmaps.c:806) draw come from `FTEmblemSprites`, a
# FIGHTER-common file -- `dMNPlayersVSFileIDs[2]` and `dMNMapsFileIDs[0]` are
# both `llFTEmblemSpritesFileID`.  Resolved by search rather than by a
# per-entry directory column so a table row stays one name.
O2R_DIRS = ("reloc_menus", "reloc_fighters_common")


def o2r_path(repo_root: Path, name: str) -> Path:
    base = repo_root / "decomp" / "BattleShip-main" / "BattleShip_o2r"
    for folder in O2R_DIRS:
        candidate = base / folder / name
        if candidate.exists():
            return candidate
    raise ConvertError(
        f"o2r container '{name}' is in none of {list(O2R_DIRS)} under {base}")


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
    # P2-1i. Where each placement LANDED, in absolute DS screen pixels, keyed
    # by its source symbol. A composited surface is otherwise opaque to the
    # runtime, and the main menu needs one number the surface already knows:
    # exactly where each dark entry icon sits, so the bright OBJ that lights
    # it up can be drawn over it rather than near it. Publishing the bake's
    # own answer is what stops the two from drifting apart -- the alternative
    # is a hand-computed constant that silently rots the next time a scale or
    # a centring rule changes.
    sites: dict[str, tuple[int, int, int, int]] = field(default_factory=dict)


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
                         alpha_ramp: bool = False,
                         width_override: int | None = None,
                         lut_override: int | None = None
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
    if lut_override is not None:
        # P2-1j.  `mnPlayersVSSetGateLUT` (mnplayersvs.c:900) draws ONE card
        # sprite through one of eight palettes -- Man/Com x four player colours
        # -- by overwriting `sprite->LUT` in place.  The card sprite's own
        # `lut` field is `llMNPlayersCommonGateMan1PLUT` exactly, which is what
        # proves the header's LUT offsets and the resolved `sprite.lut` are the
        # same coordinate and this override is the source's own operation.
        sprite = replace(sprite, lut=lut_override)
    if width_override is not None:
        # A WRAPPED sprite draws more columns than `sprite.width`: the RDP
        # tiles at 2^masks texels and the SObj's own width is only the piece
        # spDraw hands the rectangle.  The two decal bars are exactly this --
        # `width` 8, `width_img` 16, `masks` 4 (mnmodeselect.c:530,
        # mnvsmode.c:290) -- so tiling the 8 columns the normal path decodes
        # would repeat at half the source's period.
        width = width_override
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
        piece_w = min(piece.width_img if width_override is not None
                      else piece.width, width)
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
    elif sprite.bmfmt == G_IM_FMT_IA and sprite.bmsiz == G_IM_SIZ_4b:
        for y, pixels in enumerate(
                decode_ia4(fileobj, piece, sprite, piece_w, piece_h)):
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
    # A WRAPPED draw: `(lrs, lrt)`, the rectangle the SObj covers in the
    # source's own frame, filled by repeating the texture at its RDP wrap
    # period.  `sobj->cms/cmt = G_TX_WRAP` with `masks`/`maskt` set is the
    # source saying exactly this -- the character/stage-select stone is one
    # 64x32 tile over 300x220 (mnplayersvs.c:1370, mnmaps.c:356) and the
    # mode-select decal bar one 16-wide strip over 96 (mnmodeselect.c:530).
    # `period` is (2^masks, 2^maskt), or None on an axis the source clamps.
    tile: tuple[int, int] | None = None
    period: tuple[int | None, int | None] | None = None
    # P2-1k, owner ruling (f): BACKGROUNDS SPAN THE WHOLE DS SCREEN.  A TILED
    # background needs no resampling to fill the screen -- only a bigger
    # rectangle -- but the tile's PHASE is part of the artwork, so growing the
    # rect must not slide the pattern.  `tile_anchor` is the source coordinate
    # at which texture texel (0,0) sits; it defaults to the placement's own
    # (x, y), which is what every pre-P2-1k tile relied on implicitly.  The
    # stone keeps `(10, 10)` -- the position mnPlayersVSMakeWallpaper
    # (:1370) and mnMapsMakeWallpaper (:367) actually draw it at -- while its
    # rect grows to the whole 320x240 frame, so every texel inside the source's
    # own 300x220 rectangle is bit-identical to the pre-P2-1k bake and the new
    # ones simply continue the same pattern outward.  EXACT, not resampled.
    tile_anchor: tuple[int, int] | None = None
    # P2-1k, same ruling, for a background that is ONE artwork rather than a
    # tile: an explicit destination size in DS pixels, replacing the frame's
    # 4/5.  The collage is a fixed 300x220 plate the source draws at (10,10)
    # and lets the frame's own 10 px margin show around (mnmodeselect.c:527,
    # mnvsmode.c:974); on the DS that margin is the 8 px backdrop border the
    # owner asked to remove, so the plate is resampled to the whole screen.
    # STATED rather than hidden: 300 -> 256 is 0.8533 and 220 -> 192 is 0.8727
    # where both were 0.8, a 2.3% aspect change on the artwork alone.
    dest: tuple[int, int] | None = None
    # P2-1k (f2), the SAME ruling on the OTHER axis: the panel edge this
    # placement's own edge sits flush against, `"top"` or `"bottom"`.
    #
    # (f) removed the viewport's 10 px LEFT/RIGHT margin from the two title
    # bands by widening them to the full 256; their TOP and BOTTOM margins
    # survived it, which is what the owner's screenshot caught.  The source
    # says both bands are frame furniture flush with the active area's own
    # horizontal edges, not artwork at a place inside it:
    # `dMNTitleCommonSpriteDescs` (mntitle.c:70/:87) centres the copyright at
    # y 208 over a 44-row sprite -- 186..230 -- and the upper border at y 15
    # over a 10-row one -- 10..20 -- against a viewport of exactly 10..230
    # (`syRdpSetViewport(..., 10, 10, 310, 230)`).  So each band's outer edge
    # IS the active area's edge, to the row.
    #
    # At the strict 4/5 those two edges land at DS y 8 and 184, leaving 8 rows
    # of not-art at the top of the panel and 8 at the bottom -- on an N64 that
    # is overscan the television eats, on a DS panel it is a strip of fire
    # above the border and a bright strip below the copyright, which is
    # exactly what the owner reported.  Anchoring restores the source's own
    # relationship (band edge == active-area edge) on hardware that has no
    # overscan to hide it; it is derived from the viewport, not nudged to
    # taste.  Deriving `dst_y` from the band's own baked HEIGHT rather than
    # writing 0 and 157 keeps it true if the artwork or the ratio ever change.
    edge: str | None = None
    # `scale` overrides the frame's own 4/5 for this placement's ARTWORK, and
    # `centre_in` is the ratio whose footprint the smaller artwork is centred
    # inside, so the source's LAYOUT is untouched.  This is the P2-1f map-icon
    # split, reused: the four mode-select icons come down to 5/8 because the
    # bright twin of each has to fit one 32x32 OBJ cell, and a bright icon that
    # was a different size from the dark one it replaces would read as a jump.
    scale: tuple[int, int] | None = None
    centre_in: tuple[int, int] | None = None
    # P2-1j.  THE IA COMBINER IS A LERP, NOT A MODULATE, and the option tab is
    # the asset that forces the distinction.  `lbCommonPrepSObjAttr` gives an
    # IA sprite `colour = (PRIMITIVE - ENVIRONMENT) * TEXEL0 + ENVIRONMENT`
    # (lbcommon.c:2598), so a draw site that sets BOTH colours -- every VS-menu
    # button state (`mnVSModeUpdateButton`, mnvsmode.c:231) and the CSS locked
    # slot's question mark (mnplayersvs.c:2437) -- paints a two-colour RAMP
    # across the sprite's intensity.  `tint` alone is the PRIMITIVE; `env` is
    # the other end, and setting it switches this placement to the lerp.
    #
    # NOTE the source's own struct spells the pair the other way round: the
    # `SYColorRGBPair` field named `prim` is assigned to `sobj->envcolor` and
    # the one named `env` to `sobj->sprite.red/green/blue`, so the RDP ENV is
    # the pair's `prim` field.  The values below are in RDP terms.
    env: tuple[int, int, int] | None = None
    # A NOISE-dithered draw, as an expected value.  `mnPlayersVSPortraitProcDisplay`
    # (mnplayersvs.c:361) sets `colour = (NOISE - TEXEL0) * PRIMITIVE + TEXEL0`
    # with PRIMITIVE = 0x30 grey, which is the locked-fighter silhouette's
    # static-television look.  A DS bitmap has no per-frame noise source, so the
    # bake ships E[colour] over a uniform NOISE -- `TEXEL0 + (128 - TEXEL0) *
    # k / 255` -- which is the same mean brightness with the per-frame grain
    # removed.  `noise` is that k.
    noise: int | None = None
    # A flat rectangle rather than a sprite: `gDPFillRectangle` in the source's
    # own frame, with `symbol` left empty.  The VS menu's menu-name plate is
    # one (mnvsmode.c:916: prim A0/78/14 alpha E6 over 225,143..310,230).
    fill: tuple[int, int, int, int] | None = None
    size: tuple[int, int] | None = None
    # A CI sprite drawn through a DIFFERENT palette, by its reloc symbol.  The
    # player panel is one card sprite and eight palettes (see `lut_override`).
    lut_symbol: str | None = None


def place_raster(fileobj: RelocFile | None, part: Placement, offset: int,
                 lut_offset: int | None = None
                 ) -> tuple[int, int, list[list[tuple[int, int, int, int]]]]:
    """One placement decoded, combined and scaled to the DS frame's 4/5.

    Decoded with a NEUTRAL primitive so what comes back is TEXEL0 itself; the
    real primitive is applied once here.  Feeding the tint into the decoder
    instead would modulate twice for any sprite whose container carries a
    non-white primitive of its own.
    """
    if part.fill is not None:
        if part.size is None:
            raise ConvertError("a fill placement needs its own size")
        red, green, blue, alpha = part.fill
        combined = [[(red, green, blue, alpha)] * part.size[0]
                    for _ in range(part.size[1])]
    else:
        period_s = part.period[0] if part.period is not None else None
        sprite, raster = decode_sprite_raster(fileobj, part.symbol, offset,
                                              (255, 255, 255), alpha_ramp=True,
                                              width_override=period_s,
                                              lut_override=lut_offset)
        prim = part.tint if part.tint is not None else (sprite.red,
                                                        sprite.green,
                                                        sprite.blue)
        combined = []
        for row in raster:
            out = []
            for (r, g, b, a) in row:
                if part.flat:
                    out.append((prim[0], prim[1], prim[2],
                                (a * part.alpha) // 255))
                elif part.noise is not None:
                    # E[(NOISE - TEXEL0) * k + TEXEL0] over a uniform NOISE.
                    k = part.noise
                    out.append((r + (((128 - r) * k) // 255),
                                g + (((128 - g) * k) // 255),
                                b + (((128 - b) * k) // 255),
                                (a * part.alpha) // 255))
                elif part.env is not None:
                    # colour = (PRIM - ENV) * TEXEL0 + ENV.  The neutral decode
                    # above leaves r == g == b == the texel's own intensity, so
                    # one channel is the lerp parameter for all three.
                    env = part.env
                    out.append((env[0] + (((prim[0] - env[0]) * r) // 255),
                                env[1] + (((prim[1] - env[1]) * r) // 255),
                                env[2] + (((prim[2] - env[2]) * r) // 255),
                                (a * part.alpha) // 255))
                else:
                    out.append(((r * prim[0]) // 255, (g * prim[1]) // 255,
                                (b * prim[2]) // 255, (a * part.alpha) // 255))
            combined.append(out)
    src_w = len(combined[0])
    src_h = len(combined)
    if part.tile is not None:
        # Repeat at the RDP's own wrap period on each axis the source wraps;
        # an axis the source clamps (maskt == 0 on both decal bars) keeps the
        # texture's own extent and simply stops.
        tile_w, tile_h = part.tile
        wrap_s = part.period[0] if part.period is not None else src_w
        wrap_t = part.period[1] if part.period is not None else src_h
        # Where texture texel (0,0) sits, in the source frame: the placement's
        # own origin unless the rect was widened past it (see `tile_anchor`).
        anchor_x, anchor_y = (part.tile_anchor if part.tile_anchor is not None
                              else (part.x, part.y))
        phase_x = part.x - anchor_x
        phase_y = part.y - anchor_y
        tiled = []
        for y in range(tile_h):
            sy = ((y + phase_y) % wrap_t) if wrap_t else (y + phase_y)
            if not (0 <= sy < src_h):
                tiled.append([(0, 0, 0, 0)] * tile_w)
                continue
            row = combined[sy]
            out_row = []
            for x in range(tile_w):
                sx = ((x + phase_x) % wrap_s) if wrap_s else (x + phase_x)
                out_row.append(row[sx] if (0 <= sx < src_w) else (0, 0, 0, 0))
            tiled.append(out_row)
        combined = tiled
        src_w, src_h = tile_w, tile_h
    if part.dest is not None:
        width, height = part.dest
    else:
        num, den = part.scale if part.scale is not None else FRAME_SCALE
        width = max(1, (src_w * num + den // 2) // den)
        height = max(1, (src_h * num + den // 2) // den)
    left = part.x - (src_w // 2) if part.centred else part.x
    top = part.y - (src_h // 2) if part.centred else part.y
    dst_x = frame_pos(left)
    dst_y = frame_pos(top)
    if part.centre_in is not None:
        # Keep the source's own footprint on the screen and put the smaller
        # artwork in the middle of it, so only the drawing shrinks.
        cnum, cden = part.centre_in
        foot_w = max(1, (src_w * cnum + cden // 2) // cden)
        foot_h = max(1, (src_h * cnum + cden // 2) // cden)
        dst_x += (foot_w - width) // 2
        dst_y += (foot_h - height) // 2
    if part.edge is not None:
        # P2-1k (f2).  The panel edge replaces the source's own viewport inset
        # on this axis; see `Placement.edge`.
        if part.edge == "top":
            dst_y = 0
        elif part.edge == "bottom":
            dst_y = DS_SCREEN_H - height
        else:
            raise ConvertError(
                f"{part.symbol}: edge must be 'top' or 'bottom', "
                f"not {part.edge!r}")
    return (dst_x, dst_y, box_scale(combined, width, height))


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
    # P2-1j.  WHAT THIS SURFACE SITS ON, composited first and then cropped away
    # to the bounding box of `parts` alone.
    #
    # It exists so a surface the runtime RE-BLITS can be OPAQUE.  A VS-menu
    # button changes state under the cursor, and a keyed re-blit would leave
    # the previous state's antialiased edge behind -- the tab's rounded caps
    # carry an alpha ramp, and a 1-bit DS texel cannot erase what it does not
    # cover.  Baking the artwork that is underneath INTO the button makes every
    # texel opaque, so a state change is a whole-row DMA that overwrites the
    # old state exactly, with no runtime memory of what was there.
    under: tuple[Placement, ...] = ()
    # P2-1k.  AN EXPLICIT CANVAS in the source's own frame, `(x, y, w, h)`,
    # replacing the union-of-placements box.
    #
    # It exists because "overwrites the old state exactly" is only true when
    # every state of the same element has the SAME box, and deriving the box
    # from each variant's own contents silently breaks that.  The character
    # select already shipped that bug: `CSS_PANEL_*_NA` came out 53x74 (its
    # shutter doors are 92 source rows) while the MAN/COM variants came out
    # 53x73 (the card is 91), so re-occupying an empty slot left one stale row
    # of door art under the new panel.  A declared box makes the invariant a
    # bake-time fact -- and `convert_surface` REFUSES a placement that escapes
    # it, so the wrong form cannot be written rather than merely avoided.
    box: tuple[int, int, int, int] | None = None


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
    def place_all(parts: tuple[Placement, ...]) -> list:
        out = []
        for part in parts:
            if part.fill is not None:
                out.append(place_raster(None, part, 0))
                continue
            if part.symbol not in offsets:
                raise ConvertError(
                    f"{part.symbol} missing from include/reloc_data.h and "
                    "reloc_data.us.h")
            if part.o2r not in cache:
                cache[part.o2r] = RelocFile(o2r_path(repo_root, part.o2r))
            lut = None
            if part.lut_symbol is not None:
                if part.lut_symbol not in offsets:
                    raise ConvertError(
                        f"{part.lut_symbol} missing from reloc_data")
                lut = offsets[part.lut_symbol]
            out.append(place_raster(cache[part.o2r], part,
                                    offsets[part.symbol], lut))
        return out

    placed = place_all(spec.parts)
    beneath = place_all(spec.under)

    content_left = min(x for x, _, _ in placed)
    content_top = min(y for _, y, _ in placed)
    content_right = max(x + len(raster[0]) for x, _, raster in placed)
    content_bottom = max(y + len(raster) for _, y, raster in placed)
    if spec.box is not None:
        # A DECLARED canvas: every variant of the same element gets the same
        # rectangle, so a re-blit overwrites the previous state exactly.  The
        # declaration is checked against what actually landed, because a box
        # that silently CROPS a state is the same defect it exists to prevent.
        box_x, box_y, box_w, box_h = spec.box
        left = frame_pos(box_x)
        top = frame_pos(box_y)
        right = left + max(1, (box_w * FRAME_SCALE[0] + FRAME_SCALE[1] // 2) //
                           FRAME_SCALE[1])
        bottom = top + max(1, (box_h * FRAME_SCALE[0] + FRAME_SCALE[1] // 2) //
                           FRAME_SCALE[1])
        if ((content_left < left) or (content_top < top) or
                (content_right > right) or (content_bottom > bottom)):
            raise ConvertError(
                f"{spec.token}: a placement escapes the declared box -- "
                f"content ({content_left},{content_top}).."
                f"({content_right},{content_bottom}) against box "
                f"({left},{top})..({right},{bottom}). Widen `box`, or the "
                "surface would crop one of its own states.")
    else:
        left, top = content_left, content_top
        right, bottom = content_right, content_bottom
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
    for x0, y0, raster in (beneath + placed):
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
    sites = {
        part.symbol: (x0, y0, len(raster[0]), len(raster))
        for part, (x0, y0, raster) in zip(spec.parts, placed)
    }
    return Surface(spec.token, spec.token, width, height, left, top, opaque,
                   texels, sites)


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
    lines.append("/* P2-1k (d) -- the rows the title's pop animation owns, i.e.")
    lines.append(" * below the border band and above the copyright band. Both")
    lines.append(" * bands are (f2)-anchored static art the animation must")
    lines.append(" * never erase; see title_anim_band(). */")
    anim_top, anim_bottom = title_anim_band(surfaces)
    lines.append(f"#define NDS_MN_UI_KIT_TITLE_ANIM_TOP {anim_top}")
    lines.append(f"#define NDS_MN_UI_KIT_TITLE_ANIM_BOTTOM {anim_bottom}")
    lines.append("")
    lines.append("/* P2-1i -- mnTitleMakeFire's thirty states, tiled into the")
    lines.append(" * BG3 bitmap. A frame is selected by the affine reference")
    lines.append(" * point alone; PA/PD are the hardware upscale. */")
    lines.append(f"#define NDS_MN_UI_KIT_FIRE_FRAMES {FIRE_FRAMES}u")
    lines.append(f"#define NDS_MN_UI_KIT_FIRE_CELL_W {FIRE_CELL_W}u")
    lines.append(f"#define NDS_MN_UI_KIT_FIRE_CELL_H {FIRE_CELL_H}u")
    lines.append(f"#define NDS_MN_UI_KIT_FIRE_COLS {FIRE_COLS}u")
    lines.append(f"#define NDS_MN_UI_KIT_FIRE_PA {FIRE_PA}")
    lines.append(f"#define NDS_MN_UI_KIT_FIRE_PD {FIRE_PD}")
    lines.append("")
    # P2-1i -- the main menu's four entry sites, taken from where the DARK
    # twins actually landed inside MODE_SELECT. The runtime draws the bright
    # icon at exactly these pixels, so lighting an entry up is a recolour in
    # place; the order is mnModeSelectOption order, which is also the order
    # NDS_MN_UI_KIT_IMAGE_MODE_ICON_1P..DATA are packed in.
    mode_select = next((s for s in surfaces if s.token == "MODE_SELECT"), None)
    if mode_select is not None:
        icons = ("llMNMainControllerIconDarkSprite",
                 "llMNMainConsoleIconDarkSprite",
                 "llMNMainSettingsIconDarkSprite",
                 "llMNMainDataIconDarkSprite")
        missing = [s for s in icons if s not in mode_select.sites]
        if missing:
            raise ConvertError(
                f"MODE_SELECT is missing entry-icon sites: {missing}")
        lines.append("/* P2-1i -- {x, y} of each main-menu entry icon, in DS")
        lines.append(" * screen pixels, as the dark twin was composited. */")
        lines.append("#define NDS_MN_UI_KIT_MODE_ENTRY_COUNT "
                     f"{len(icons)}u")
        rows = ", ".join(
            "{ %d, %d }" % (mode_select.sites[s][0], mode_select.sites[s][1])
            for s in icons)
        lines.append("static const s16 kNdsUiKitModeEntrySite"
                     "[NDS_MN_UI_KIT_MODE_ENTRY_COUNT][2] "
                     "__attribute__((unused)) = {")
        lines.append(f"    {rows},")
        lines.append("};")
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
    # P2-1i. THE 1:1 HAND CURSORS ARE GONE, and their removal is the finding
    # behind owner note (3) rather than a saving. `mnmodeselect.c` contains no
    # cursor of any kind -- the entry it is on is shown by swapping to the
    # BRIGHT icon (:161 vs :213) -- and `mnvsmode.c` shows it by recolouring an
    # option tab (`mnVSModeUpdateButton`, :321). Neither screen has a hand, so
    # nothing sets its size; drawn at 1:1 in a layout scaled to 4/5 it came out
    # a fifth too large for everything around it. Both screens now point the
    # same 4/5 hand the character select uses (CSS_CURSOR_POINT below), which
    # is the only hand the source draws anywhere in this shell
    # (mnplayersvs.c:1723). That frees the 6,144 bytes of main OBJ the four
    # mode-select icons below need.
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
    # ---- P2-1i, the main menu's SELECTED entry. --------------------------
    # `mnModeSelectMake<Entry>` draws a DIFFERENT sprite for the entry the
    # cursor is on: the bright IA8 icon at white (mnmodeselect.c:161/:238/
    # :315/:392) instead of the dark I4 twin at grey 0x96 (:213/:290/:367/
    # :444). The dark four are composited into the MODE_SELECT surface; these
    # four are OBJ cells because exactly one of them moves per cursor move and
    # a bitmap surface cannot change without a re-blit.
    #
    # 5/8, and it is the same CELL FACT P2-1f's map icons are: 51x51 at the
    # frame's own 4/5 is 41x41 and lands in a 64x64 cell (8,192 B) -- four of
    # those is 32,768 and main OBJ has 8,448 free. At 5/8 the icon is 32x32 in
    # a 32x32 cell (2,048 B) and the four fit exactly the space the two 1:1
    # cursors above gave back. The surface's dark twins are baked at the SAME
    # 5/8 inside the source's own 4/5 footprint, so lighting an entry up is a
    # recolour in place and not a resize.
    ("MNMain", "llMNMainControllerIconSprite", "MODE_ICON_1P", (5, 8)),
    ("MNMain", "llMNMainConsoleIconSprite", "MODE_ICON_VS", (5, 8)),
    ("MNMain", "llMNMainSettingsIconSprite", "MODE_ICON_OPTION", (5, 8)),
    ("MNMain", "llMNMainDataIconSprite", "MODE_ICON_DATA", (5, 8)),
    # ---- P2-1j, owner findings (b) and (e): the two ARROW pairs. -----------
    # Both screens draw a blinking left/right pair beside the value the cursor
    # is on, and BOTH are OBJ rather than surface art for the same reason: they
    # blink on a fixed cycle (30 tics on the VS menu, `mnVSModeAnimateRule
    # Arrows`/`...TimeStockArrows` :466/:539; 10 tics on the character select,
    # `mnPlayersVSArrowThreadUpdate` :2626) and hiding an OBJ costs nothing
    # while re-blitting a surface costs a NitroFS read.  They are also the
    # smallest art in the pack: 128 and 256 bytes a cell.
    #
    # The VS pair is I4 modulated by the source's own amber (mnvsmode.c:2578:
    # red 0xFF, green 0xAE, blue 0x00), baked in as the map cursor's red is.
    ("MNCommon", "llMNCommonArrowLSprite", "VS_ARROW_L", (4, 5),
     (0xFF, 0xAE, 0x00)),
    ("MNCommon", "llMNCommonArrowRSprite", "VS_ARROW_R", (4, 5),
     (0xFF, 0xAE, 0x00)),
    # The character select's pair is CI4 and carries its own colour
    # (mnplayersvs.c:2655/:2673 set no primitive at all).
    ("MNPlayersCommon", "llMNPlayersCommonArrowLSprite", "CSS_ARROW_L",
     (4, 5)),
    ("MNPlayersCommon", "llMNPlayersCommonArrowRSprite", "CSS_ARROW_R",
     (4, 5)),
    # The colon between CP LEVEL and its value, white (mnplayersvs.c:2740).
    ("MNCommon", "llMNCommonColonSprite", "COLON", (4, 5)),
    # THE PANEL'S OWN PLAYER TAG, mnPlayersVSMakePlayerKind :2003.  A human
    # slot draws `llMNPlayersCommon<N>PTextSprite` and a CPU one the shared
    # `CPTextSprite`, both in BLACK on the card.  Only 1P exists here because
    # this build fills one human slot; 2P/3P/4P arrive with P2-2's fourth
    # fighter, exactly as the puck block above.
    ("MNPlayersCommon", "llMNPlayersCommon1PTextSprite", "PANEL_1P", (4, 5),
     (0x00, 0x00, 0x00)),
    # 3/4 rather than the frame's 4/5, and it is a CELL fact exactly as P2-1f's
    # map icons were: the DS has no 64x16 OBJ cell, so the CP tag at 4/5 is 34
    # px wide, lands in a 64x32 cell and costs 4,096 B, while its 1P twin at 31
    # px fits a 32x16 one for 1,024. 3/4 is the largest exact ratio that lands
    # CP in the SAME cell as 1P (43 * 3/4 = 32), and it buys 3,072 B of a bank
    # with 3,456 left.
    ("MNPlayersCommon", "llMNPlayersCommonCPTextSprite", "PANEL_CP", (3, 4),
     (0x00, 0x00, 0x00)),
]

# P2-1h.  THE ORIGINAL PRESENTATION ART (owner ruling, 2026-08-18: this is a
# port, so the first-party branding, logos and copyright line ship, converted
# from source like every other asset).
#
# P2-1i.  THE TITLE'S FIELD IS THE FIRE, so both title surfaces bake KEYED
# (background=None): the fire runs on the DS as the BG3 bitmap behind the BG2
# art (`ndsPlatformSetTitleFireEnabled` puts it there), and a keyed
# TITLE_SCREEN is what lets it show everywhere the art has no texel.  The fire
# is on from the first presented frame -- `mnTitleMakeFire` calls
# `mnTitleShowFire` during construction on our branch (mntitle.c:990-993) --
# and its atlas is opaque on every texel, so main BG palette entry 0 is never
# actually seen on this screen; the shell still paints it black as the floor.
TITLE_FIELD = None

# The decal blue every non-title menu sits on (mnmodeselect.c:517).  It is
# baked as a surface's FIELD, not as a margin: `rgba8_to_ds` sends it to
# exactly `RGB15(1, 6, 12)`, which is the texel `NDS_MENU_BACKDROP_BLUE`
# already paints through BG palette entry 0, so an opaque surface over it has
# no seam AND may move by whole-row DMA.
MENU_FIELD = (0x08, 0x33, 0x65)

# P2-1k, OWNER RULING (f): THE BACKGROUNDS SPAN THE WHOLE DS SCREEN.
#
# Every menu scene in this shell draws inside one rectangle, and it is the
# scene camera's own viewport -- `syRdpSetViewport(&cobj->viewport, 10.0F,
# 10.0F, 310.0F, 230.0F)` in every `mn*Make*Camera` (mnplayersvs.c:764/:789/
# :814/:839/:864, mnmaps.c:1145, mnvsmode.c).  The source's ACTIVE AREA is
# therefore 300x220 at (10,10), and the two screens that carry a real
# background say so twice: `mnPlayersVSMakeWallpaper` (:1370) and
# `mnMapsMakeWallpaper` (:364) both wrap the stone over `lrs`/`lrt` = 300x220
# at exactly that origin.
#
# At the strict 4/5 the whole 320x240 FRAME maps to the screen, so that
# viewport lands at (8,8)..(248,184) and the source's own 10 px border becomes
# 8 px of flat DS backdrop on all four sides -- 15,360 of the panel's 49,152
# pixels showing the field instead of art.  On an N64 that border is overscan
# the television eats; a DS panel has no overscan, so it is simply a frame
# around the picture, which is what the owner reported.
#
# THE CHOICE, PER SCREEN, AND IT IS THE SAME ONE EVERYWHERE: the BACKGROUND
# reaches the edges, and every element drawn OVER it keeps the source's own 4/5
# position.  The alternative -- remapping the whole layout so the viewport
# fills the screen -- was rejected because it moves every element on the mode
# select and the VS menu, and those two are owner-PASSED; a background that
# runs to the edge is what the ruling asks for and it is achievable without
# touching one foreground coordinate.
#
# The two mechanisms differ because the two background kinds differ:
#   a TILE needs no resampling, only a bigger rectangle (`tile_anchor` keeps
#     its phase), so the stone is EXACT -- every texel the source's own
#     rectangle covers is bit-identical to the pre-P2-1k bake;
#   a single PLATE has to be resampled, so the collage is stretched 300->256
#     and 220->192 (0.8533 / 0.8727 against 0.8 / 0.8), a 2.3% aspect change on
#     a montage nothing is registered to.
COLLAGE_FULL_BLEED = Placement("MNCommon", "llMNCommonSmashBrosCollageSprite",
                               0, 0, False, dest=(DS_SCREEN_W, DS_SCREEN_H))
STONE_FULL_BLEED = Placement("MNSelectCommon",
                             "llMNSelectCommonStoneBackgroundSprite",
                             0, 0, False, tile=(320, 240), period=(64, 32),
                             tile_anchor=(10, 10))

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
    # P2-1k (f), THE TITLE'S OWN ANSWER, stated because this screen is the one
    # where "the background" is not on BG2 at all.  The title's background is
    # the FIRE, and it has spanned the whole screen since P2-1i -- it is a BG3
    # extended-rotscale bitmap whose affine blows one 51x42 cell up to 256x192,
    # so there is no border behind it to remove.  What is on BG2 here is keyed
    # foreground, and it splits in two:
    #
    #   the WORDMARK pieces, the TM and the emblem are discrete artwork at a
    #     place, and they keep the frame's own 4/5 exactly as the VS menu's
    #     plaque and the character select's portrait boxes do;
    #   the UPPER BORDER and the COPYRIGHT BAND are the title's FRAME
    #     FURNITURE -- both are 300 px wide, i.e. the source's whole active
    #     width (10..310), and both are centred on x = 160.  At 4/5 they stop
    #     8 px short of each panel edge while the fire behind them runs to it,
    #     which is exactly the inset look the ruling removes.  So these two --
    #     and only these two -- are stretched across the full 256, keeping
    #     their own 4/5 height and y.
    #
    # They are re-expressed as top-left placements because `dest` sizes the
    # artwork and `centred` would still put the origin at frame_pos(10) = 8:
    # copyright top = 208 - 44/2 = 186, border top = 15 - 10/2 = 10.
    #
    # P2-1k (f2), owner screenshot: the SAME ruling on the vertical axis.  (f)
    # took the left/right margin off these two and left the top/bottom one on,
    # so the border band sat 8 rows below the panel's top edge with fire above
    # it and the copyright band stopped 8 rows above the bottom with a bright
    # strip below.  Both bands are flush with the source's own active area
    # (10..230) -- see `Placement.edge` for the citation -- so on a DS panel,
    # which has no overscan, each anchors to the panel edge its source edge
    # already touches.  `y` stays the source's own top-left because it is what
    # the derivation is checked against; `edge` is what the bake places by.
    Placement("MNTitle", "llMNTitleCopyrightSprite", 0, 186, False,
              (0xB7, 0xAE, 0x7C), dest=(DS_SCREEN_W, 35), edge="bottom"),
    Placement("MNTitle", "llMNTitleBorderUpperSprite", 0, 10, False,
              (0x14, 0x12, 0x06), dest=(DS_SCREEN_W, 8), edge="top"),
)

# P2-1L (1), OWNER BUG: "a transparent RED overlay artifact on the title
# screen, only on a small part of the right side".
#
# THE ELEMENT IS REAL AND THE PLACEMENT WAS RIGHT.  `mnTitleMakeLogoNoOpening`
# (mntitle.c:1051) is the branch this build takes, it sets the SObj's primitive
# to pure red with `SP_TRANSPARENT`, `mnTitleLogoProcDisplay` (:1011) draws it
# through `gDPSetCombineLERP(0,0,0,PRIMITIVE, TEXEL0,0,PRIMITIVE,0, ...)` -- a
# FLAT primitive colour whose only texture contribution is alpha -- and
# `mnTitleSetEndLogoPosition` (:374) leaves it at scale 1 and
# `sMNTitleLogoAlpha = 0x4C` from tic 170 on.  Its desc centre is (260, 60)
# (:91).  On an N64 that is a 30% red wash over its whole silhouette: over the
# fire, over the border band, over the wordmark, all of it.
#
# A DS 16-BIT BG BITMAP CARRIES ONE ALPHA BIT, so that wash cannot exist in the
# layer this art lives in.  Composited at bake time it survived only where an
# OPAQUE texel happened to lie beneath it -- measured, 2026-08-19: 1,243 texels
# of TITLE_SCREEN, of which 179 are rows 0..6 of the (f2) border band at x
# 202..236 (near-black RGB5 (2,1,0) turned to (11,1,0)) and the other 1,064 are
# scattered tint on the wordmark.  That 179-texel block IS the owner's report:
# a dark red rectangle sitting on the black band on the right.  It is not a
# rendition of the emblem; it is the emblem's shadow on whatever was behind it,
# and the (f2) edge anchoring slid the band under a different part of the
# silhouette than the source has it under, so it is not even the source's
# shadow.
#
# SO IT IS OMITTED, and this is a recorded fidelity delta rather than a
# deletion: source `llMNTitleLogoAnimFullSprite` at (260,60) alpha 0x4C; visible
# delta, no red wash on the title's right side; reason, the destination layer
# has no alpha to blend with, and the surviving fragment reads as corruption
# rather than as the element.  Reinstating it faithfully needs a second layer
# the DS can alpha-blend -- a semi-transparent OBJ (DS OBJ mode 1 with
# BLDALPHA) over the 102x99 silhouette, four 64x64 bitmap sprites -- which is a
# new mechanism on a screen that is already inside its cadence, and therefore
# an owner call, not a bake call.
TITLE_EMBLEM_OMITTED = Placement("MNTitle", "llMNTitleLogoAnimFullSprite",
                                 260, 60, True, (0xFF, 0x00, 0x00),
                                 alpha=0x4C, flat=True)

# P2-1k (d).  THE POP ANIMATION'S OWN PAYLOAD, and the source says which of
# the seven label pieces move.  `mnTitleMakeLabels` (mntitle.c:1180) builds
# TWO display GObjs off one animation DObj tree: kinds 0..4 go on the GObj that
# carries `mnTitleProcUpdate` -> `mnTitlePlayAnim`, and kinds 5..6 (the
# copyright FOOTER and the border HEADER) go on a second GObj whose process is
# `mnTitleUpdateLabelsPosition` (:772) -- which sets each of those two from
# `dMNTitleCommonSpriteDescs` every tic and NEVER reads its DObj.  Their anim
# streams exist and advance; nothing consumes them on this branch.
# `mnTitleSetEndLayout` (:397) confirms the split from the other side: it snaps
# the GObj with `id == 8`, and that GObj's SObj list is exactly five long.
#
# So FIVE pieces animate, not seven, and the two BANDS are static frame
# furniture from the first tic -- which is also why (f2)'s edge anchoring needs
# no pose offset at all: the animation cannot move what it does not drive.
TITLE_ANIM_PARTS = TITLE_PARTS[:5]

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
    # P2-1k (d).  One surface per animated piece, at the frame's own 4/5, so
    # the runtime has a raster it can re-blit at that frame's scale.  They are
    # NOT cacheable: the kit's single cache buffer is PRESS START's, and these
    # six live in a title-scene arena block instead (`ndsUiKitTitleAnimLoad`).
    SurfaceSpec("TITLE_ANIM_CUTOUT", (TITLE_ANIM_PARTS[0],), TITLE_FIELD),
    SurfaceSpec("TITLE_ANIM_SMASH", (TITLE_ANIM_PARTS[1],), TITLE_FIELD),
    SurfaceSpec("TITLE_ANIM_SUPER", (TITLE_ANIM_PARTS[2],), TITLE_FIELD),
    SurfaceSpec("TITLE_ANIM_BROS", (TITLE_ANIM_PARTS[3],), TITLE_FIELD),
    SurfaceSpec("TITLE_ANIM_TM", (TITLE_ANIM_PARTS[4],), TITLE_FIELD),
    # THE SETTLED FRAME, and it exists because a stack of independently
    # quantised pieces is NOT the composite the kit bakes.  A DS texel carries
    # one alpha bit, so where an antialiased `Smash` edge blends over an opaque
    # `Cutout` texel, TITLE_SCREEN holds the BLEND; five separate rasters, each
    # quantised alone, hold neither.
    #
    # P2-1L (1) took the emblem back out of both title surfaces -- see
    # TITLE_EMBLEM_OMITTED above for the source citation, the 1,243-texel
    # measurement and the delta record -- so this composite is now exactly the
    # five animated pieces, and its rectangle shrinks with them from 238x111 at
    # (18,0) to 219x72 at (18,39): 21,300 fewer resident bytes in the title
    # scene's arena block, and the settled rectangle no longer reaches rows the
    # (f2) border band owns.
    #
    # `mnTitleSetEndLayout` snaps at tic 220, so this is the frame the screen
    # then holds: the runtime blits THIS at pose 51 and the resting title is
    # bit-identical to the static title.  `check_title_anim_block` proves that
    # rather than asserting it.  No `box`: the union of these five IS the
    # rectangle wanted.
    SurfaceSpec("TITLE_WORDMARK", TITLE_ANIM_PARTS, TITLE_FIELD),
    # THE MENU COLLAGE.  300x220 CI4 with a 16-entry TLUT, 44 bands of 6 rows
    # stepping 5 -- the asset that forced decode_sprite_raster to transcribe
    # spDraw's band policy.  Both screens that show it place it identically at
    # (10, 10) (mnmodeselect.c:527, mnvsmode.c:974), top-left and untinted, and
    # the source leaves its own 10 px margin showing the decal blue, which the
    # DS backdrop paints at zero cost.
    #
    # P2-1j RETIRED the standalone surface: the mode select composes it into
    # MODE_SELECT and the VS menu into VS_MODE below, so nothing blits the bare
    # collage any more and baking it would ship 84,480 dead bytes.  It survives
    # as the first placement of both plates.
    # P2-1i, owner finding (1). THE CHARACTER AND STAGE SELECTS HAVE A REAL
    # BACKGROUND AND IT IS THE SAME ONE: `mnPlayersVSMakeWallpaper`
    # (mnplayersvs.c:1342) and `mnMapsMakeWallpaper` (mnmaps.c:348) are the
    # same eleven lines -- `llMNSelectCommonStoneBackgroundSprite`, a 64x32
    # CI4 stone tile, `cms`/`cmt = G_TX_WRAP`, `masks = 6` / `maskt = 5` (its
    # own 64x32 period), drawn over `lrs`/`lrt` = 300x220 at (10, 10).  That
    # rectangle is the CSS camera's own viewport (`syRdpSetViewport(..., 10,
    # 10, 310, 230)`), which is what proves lrs/lrt are the rect's SIZE and
    # not its lower-right corner.  One surface, blitted by both screens: the
    # two calls differ in nothing a bake can see.
    #
    # P2-1k RETIRES the standalone stone for the same reason P2-1j retired the
    # standalone collage: the character select already composes it into
    # CSS_SCREEN and the stage select now composes it into SSS_SCREEN, so the
    # bare tile has no consumer and baking it would ship dead bytes.  It
    # survives as `STONE_FULL_BLEED`, the first placement of both plates.
    # P2-1i, owner finding (2), the MAIN MENU.  Everything `mnModeSelectMake*`
    # composes that does not change with the cursor, in the source's own draw
    # order: the collage and both decal bars first (link 2 / display 0), then
    # the MODE SELECT plate and the SMASH emblem, then the four entry icons in
    # their UNSELECTED form and the four English labels (link 3 / display 1).
    # Positions and colours are quoted from mnmodeselect.c:161-579 and are the
    # source's own -- the icons at (169,27)/(128,64)/(87,101)/(46,138), the
    # labels at (224,52)/(183,89)/(142,126)/(102,163) in pure red, the decal
    # bar tinted the same decal blue the DS backdrop already paints.
    #
    # THE SELECTED ICON IS NOT HERE, and that is the one thing this surface
    # cannot carry: `mnModeSelectMake1PMode` swaps to a DIFFERENT sprite
    # (`...IconSprite`, IA8, white) for the highlighted entry and back to
    # `...IconDarkSprite` (I4, grey 0x96) for the rest, and a composited
    # bitmap cannot change on a cursor move without a re-blit.  The four
    # bright icons are OBJ images instead (IMAGE_SOURCES below), drawn over
    # the dark one at the same 5/8 artwork size so the swap is a light-up and
    # not a resize.
    SurfaceSpec("MODE_SELECT",
                (COLLAGE_FULL_BLEED,
                 Placement("MNMain", "llMNMainDecalBarMiddleSprite",
                           0, 37, False, (0x08, 0x33, 0x65),
                           tile=(96, 38), period=(16, None)),
                 Placement("MNMain", "llMNMainDecalBarEdgeSprite",
                           96, 37, False, (0x08, 0x33, 0x65)),
                 Placement("MNMain", "llMNMainModeSelectTextSprite",
                           28, 27, False, (0x3C, 0x73, 0xB4)),
                 Placement("MNMain", "llMNMainSmashLogoSprite",
                           226, 137, False, (0x08, 0x33, 0x65)),
                 Placement("MNMain", "llMNMainControllerIconDarkSprite",
                           169, 27, False, (0x96, 0x96, 0x96),
                           scale=(5, 8), centre_in=(4, 5)),
                 Placement("MNMain", "llMNMainConsoleIconDarkSprite",
                           128, 64, False, (0x96, 0x96, 0x96),
                           scale=(5, 8), centre_in=(4, 5)),
                 Placement("MNMain", "llMNMainSettingsIconDarkSprite",
                           87, 101, False, (0x96, 0x96, 0x96),
                           scale=(5, 8), centre_in=(4, 5)),
                 Placement("MNMain", "llMNMainDataIconDarkSprite",
                           46, 138, False, (0x96, 0x96, 0x96),
                           scale=(5, 8), centre_in=(4, 5)),
                 Placement("MNMain", "llMNMain1PModeTextSprite",
                           224, 52, False, (0xFF, 0x00, 0x00)),
                 Placement("MNMain", "llMNMainVsModeTextSprite",
                           183, 89, False, (0xFF, 0x00, 0x00)),
                 Placement("MNMain", "llMNMainOptionTextSprite",
                           142, 126, False, (0xFF, 0x00, 0x00)),
                 Placement("MNMain", "llMNMainDataTextSprite",
                           102, 163, False, (0xFF, 0x00, 0x00))),
                MENU_FIELD),
]


# ---------------------------------------------------------------------------
# P2-1j -- the VS rules screen, owner finding (b): the source's OWN buttons.
# ---------------------------------------------------------------------------
#
# WHAT THE SOURCE DRAWS, and it is not a cursor.  `mnvsmode.c` has no cursor
# sprite at all: `sMNVSModeCursorIndex` selects which of four BUTTONS is
# recoloured, through `mnVSModeUpdateButton` (:231).  Each button is three
# sprites -- `llMNCommonOptionTab{Left,Middle,Right}Sprite` with the middle
# TILED across `arg3 * 8` pixels at its own 16-texel wrap (`masks = 4`,
# `lrs = arg3 * 8`, :286-306) -- and all four use arg3 = 17, so every button on
# this screen is the same 168x29 plate at a different place.
#
# THE STATE IS A COLOUR PAIR, so the DS cost is two bakes and not two artworks:
#
#   nMNOptionTabStatusHighlight   ENV 82/00/28   PRIM FF/00/28
#   nMNOptionTabStatusNot         ENV 00/00/00   PRIM 82/82/AA
#
# through the IA combiner `colour = (PRIM - ENV) * TEXEL0 + ENV`.  (The third
# status, Selected, is never reached on this screen -- `mnVSModeUpdateButton`
# is only ever called with Highlight or Not, :2157/:2196/:2235/:2277.)
#
# WHY EACH BUTTON IS ITS OWN SURFACE RATHER THAN AN OBJ.  A plate is 134x23
# DS pixels; a bitmap OBJ cell tops out at 64x64, so one button is three cells
# (8,704 B) and the two states of four buttons is 34,816 B against 16,640 B of
# free main OBJ.  As BG2 surfaces they cost no VRAM at all, and each is baked
# WITH the artwork underneath it (`under=VS_BACKGROUND`) so it is fully opaque
# and a state change overwrites the previous state exactly -- no runtime memory
# of what was there, and no leftover from the tab's antialiased caps.
#
# THE RULE-DEPENDENT TEXT rides on the button it sits inside, which is why the
# rule button has TIME and STOCK variants: `mnVSModeMakeRuleValue` (:337) draws
# the value word at (183|187, 78), inside the rule plate's own 97..265 x 70..99
# rectangle, and `mnVSModeMakeTimeStockButton` (:679) swaps its own period text
# the same way.
VS_TAB_HI = ((0x82, 0x00, 0x28), (0xFF, 0x00, 0x28))
VS_TAB_NOT = ((0x00, 0x00, 0x00), (0x82, 0x82, 0xAA))

# mnVSModeMakeBackground (:965) then mnVSModeRenderMenuName (:909), in the
# source's own construction order -- background link 2 / display 0 first, then
# the menu-name GObj's fill rectangle and its three plaque sprites.
VS_BACKGROUND = (
    COLLAGE_FULL_BLEED,
    Placement("MNCommon", "llMNCommonDecalPaperSprite", 140, 143, False,
              (0xA0, 0x78, 0x14)),
    Placement("MNCommon", "llMNCommonDecalPaperSprite", 225, 56, False,
              (0xA0, 0x78, 0x14)),
    Placement("MNVSMode", "llMNVSModeConsoleIconDarkSprite", 10, 10, False,
              (0x99, 0x99, 0x99)),
    # gDPFillRectangle(225, 143, 310, 230) at PRIM A0/78/14 alpha E6, drawn
    # G_RM_AA_XLU_SURF -- a translucent plate, so it is composited here at its
    # own alpha rather than thresholded (mnvsmode.c:916).
    Placement("MNCommon", "", 225, 143, False,
              fill=(0xA0, 0x78, 0x14, 0xE6), size=(86, 88)),
    Placement("MNCommon", "llMNCommonSmashLogoSprite", 235, 158, False,
              (0x00, 0x00, 0x00)),
    Placement("MNVSMode", "llMNVSModeVSTextSprite", 158, 192, False,
              (0x00, 0x00, 0x00)),
    Placement("MNCommon", "llMNCommonGameModeTextSprite", 189, 87, False,
              (0x00, 0x00, 0x00)),
)

# mnVSModeMakeButton, :281.  arg3 = 17 on all four (:2159/:2600/:2851/:2937).
VS_BUTTON_SPAN = 17


def vs_button(token: str, x: int, y: int, state, texts) -> SurfaceSpec:
    env, prim = state
    parts = [
        Placement("MNCommon", "llMNCommonOptionTabLeftSprite", x, y, False,
                  prim, env=env),
        Placement("MNCommon", "llMNCommonOptionTabMiddleSprite", x + 16, y,
                  False, prim, env=env,
                  tile=(VS_BUTTON_SPAN * 8, 29), period=(16, None)),
        Placement("MNCommon", "llMNCommonOptionTabRightSprite",
                  x + 16 + (VS_BUTTON_SPAN * 8), y, False, prim, env=env),
    ]
    parts.extend(texts)
    return SurfaceSpec(token, tuple(parts), MENU_FIELD, under=VS_BACKGROUND)


def vs_text(o2r: str, symbol: str, x: int, y: int,
            rgb=(0x00, 0x00, 0x00)) -> Placement:
    return Placement(o2r, symbol, x, y, False, rgb)


# Every button's own geometry and text, quoted from its Make function.  The
# black text colour is the source's (`sprite.red/green/blue = 0x00` at each
# site); the rule VALUE is white (`color = { 0xFF, 0xFF, 0xFF }`, :2251).
VS_BUTTONS = (
    ("START", 120, 31,
     (vs_text("MNVSMode", "llMNVSModeVSStartTextSprite", 153, 36),)),
    ("RULE_TIME", 97, 70,
     (vs_text("MNVSMode", "llMNVSModeRulePeriodTextSprite", 108, 75),
      vs_text("MNVSMode", "llMNVSModeTimeTextSprite", 187, 78,
              (0xFF, 0xFF, 0xFF)))),
    ("RULE_STOCK", 97, 70,
     (vs_text("MNVSMode", "llMNVSModeRulePeriodTextSprite", 108, 75),
      vs_text("MNVSMode", "llMNVSModeStockTextSprite", 183, 78,
              (0xFF, 0xFF, 0xFF)))),
    ("TIME", 74, 109,
     (vs_text("MNVSMode", "llMNVSModeTimePeriodTextSprite", 97, 113),
      vs_text("MNVSMode", "llMNVSModeMinTextSprite", 197, 120))),
    ("STOCK", 74, 109,
     (vs_text("MNVSMode", "llMNVSModeStockPeriodTextSprite", 106, 114),)),
    ("OPTIONS", 51, 148,
     (vs_text("MNVSMode", "llMNVSModeVSOptionsTextSprite", 71, 151),)),
)

SURFACE_SOURCES.append(SurfaceSpec("VS_MODE", VS_BACKGROUND, MENU_FIELD))
for _name, _x, _y, _texts in VS_BUTTONS:
    SURFACE_SOURCES.append(
        vs_button(f"VS_BTN_{_name}_HI", _x, _y, VS_TAB_HI, _texts))
    SURFACE_SOURCES.append(
        vs_button(f"VS_BTN_{_name}_NOT", _x, _y, VS_TAB_NOT, _texts))


# ---------------------------------------------------------------------------
# P2-1j -- the character select, owner findings (c), (d) and (e).
# ---------------------------------------------------------------------------
#
# (d) THE FIGHTER BOX is `llMNPlayersPortraitsPortraitFireBgSprite`, a 45x43
# RGBA16 plate that `mnPlayersVSMakePortrait` puts behind EVERY portrait --
# locked or not, :2437 and :2503 -- on the source's own 45 px grid,
# `x = (portrait % 6) * 45 + 25`, `y = (portrait >= 6) * 43 + 36`.  Nothing in
# this shell drew it, so the twelve cells sat directly on the stone.
#
# (c) THE LOCKED SLOT is a THREE-layer stack, not the question mark alone
# (`mnPlayersVSMakePortraitShadow`, :2404): the box, then the fighter's own
# SHADOW sprite drawn through `mnPlayersVSPortraitProcDisplay`'s noise
# combiner, then the question mark at ENV 5B/41/33 / PRIM C4/B9/A9.  The
# shadow table is indexed by fkind and is populated for exactly the four
# fighters the retail cart locks -- Luigi, Captain Falcon, Purin, Ness -- and
# is 0x0 for the other eight, so a build whose locked set is wider draws box +
# question mark for those, which is what the source's own table says to do.
#
# The stack is baked into the backdrop because a locked cell never changes for
# the life of the screen, and doing so also frees the OBJ slots the twelve
# portrait cells used to need.
CSS_PORTRAIT_FKIND = (4, 0, 2, 5, 3, 7, 11, 6, 8, 1, 9, 10)
CSS_SHADOW = {
    4: "llMNPlayersPortraitsLuigiShadowSprite",
    7: "llMNPlayersPortraitsCaptainShadowSprite",
    10: "llMNPlayersPortraitsPurinShadowSprite",
    11: "llMNPlayersPortraitsNessShadowSprite",
}
# Which fighters this build HAS (nFTKindMario, nFTKindFox).  Same bound the
# shell's NDS_CSS_FIGHTER_MASK carries.
CSS_BUILT_FKIND = (0, 1)
# mnPlayersVSPortraitProcDisplay's primitive, :361.
CSS_SHADOW_NOISE = 0x30


def css_portrait_pos(portrait: int) -> tuple[int, int]:
    return (((portrait % 6) * 45) + 25, ((1 if portrait >= 6 else 0) * 43) + 36)


def css_screen_parts() -> tuple[Placement, ...]:
    parts = [
        # mnPlayersVSMakeWallpaper, :1370, full-bleed per the P2-1k ruling.
        STONE_FULL_BLEED,
    ]
    for portrait in range(12):
        x, y = css_portrait_pos(portrait)
        fkind = CSS_PORTRAIT_FKIND[portrait]
        parts.append(Placement("MNPlayersPortraits",
                               "llMNPlayersPortraitsPortraitFireBgSprite",
                               x, y, False))
        if fkind in CSS_BUILT_FKIND:
            continue
        shadow = CSS_SHADOW.get(fkind)
        if shadow is not None:
            parts.append(Placement("MNPlayersPortraits", shadow, x, y, False,
                                   noise=CSS_SHADOW_NOISE))
        parts.append(Placement(
            "MNPlayersPortraits",
            "llMNPlayersPortraitsPortraitQuestionMarkSprite", x, y, False,
            (0xC4, 0xB9, 0xA9), env=(0x5B, 0x41, 0x33)))
    return tuple(parts)


SURFACE_SOURCES.append(
    SurfaceSpec("CSS_SCREEN", css_screen_parts(), MENU_FIELD))

# (e) THE PLAYER PANEL.  `mnPlayersVSMakeGate` (:1010) builds one CI4 card per
# slot at `(player * 69 + 22, 126)` and picks its PALETTE by player colour and
# player kind -- `mnPlayersVSSetGateLUT`, :900, eight `llMNPlayersCommonGate
# {Man,Com}<N>PLUT` tables -- so the card is one artwork and eight colourways.
# An EMPTY slot additionally has its two shutter doors closed over it
# (`door_offset` 41: left at `p*69+22`, right at `p*69+47`, :640), which is
# exactly the card's own 66 px width.
#
# Three surfaces per slot, blitted on a kind change, is what a palette swap
# costs on a direct-colour DS bitmap; the alternative was twelve more OBJ cells
# of 53x73, which does not fit a 64x64 cell OR the free bank.
#
# ---------------------------------------------------------------------------
# P2-1k, owner findings (a) and (b): WHAT AN OCCUPIED GATE ACTUALLY DRAWS.
# ---------------------------------------------------------------------------
#
# The owner's report is "the preview boxes aren't rendering the fighter", and
# the source's answer is `mnPlayersVSMakeNameAndEmblem` (mnplayersvs.c:578),
# which `mnPlayersVSMakeGate` builds onto its own GObj (:1110-1113) and
# `mnPlayersVSUpdateNameAndEmblem` (:2397) re-makes every time the slot's
# fighter or kind changes.  It is two sprites and nothing else:
#
#   the SERIES EMBLEM `llFTEmblemSprites<Series>Sprite` at `(p*69+24, 143)`,
#     modulated 0x1E1E1E when the slot is `nFTPlayerKindMan` and 0x444444
#     otherwise (:619-630) -- so the tint is per PLAYER KIND, not per fighter;
#   the FIGHTER NAME `llMNPlayersCommon<Fighter>TextSprite` at `(p*69+22, 201)`,
#     with no primitive set at its draw site, so it keeps the sprite's own white.
#
# THE NAME AND THE CP LEVEL ROW ARE THE SAME ROW, and that is what makes three
# fighter states rather than two.  `mnPlayersVSUpdateHandicapLevel` opens with
# `mnPlayersVSHideFighterName` (:2788), which hides the name_emblem GObj's
# SECOND SObj -- the name (:2575-2585) -- and then puts CP LEVEL at y 201 and
# its colon at y 202, exactly where the name was.  That row is built only when
# handicap is on or the slot is a CPU (:1115), and
# `mnPlayersVSHandicapLevelProcUpdate` DESTROYS it the moment the slot stops
# being a settled fighter (:2689).  So a human slot shows its name; a settled
# CPU slot shows CP LEVEL instead; and a CPU slot whose token is in the
# cursor's hand shows the name again.
#
# WHY SURFACES AND NOT OBJ CELLS.  The emblem is 64x48 and a name up to 47x16,
# so one fighter's pair at the frame's own 4/5 is a 64x64 cell plus a 64x16 one
# = 10,240 B for ONE fighter, against the 3,456 B of free main OBJ P2-1j left
# (`docs/p2/P2-1c-vram-map.md`).  As part of the gate SURFACE they cost bank E
# nothing, and folding them into the card rather than giving them their own
# surface keeps the runtime a single "which surface does this slot want" choice:
# every state of a slot is ONE blit that overwrites the last exactly.
#
# EVERY STATE SHARES ONE DECLARED BOX (`box=` below), which is P2-1k fixing a
# defect the pre-P2-1k bake shipped: the NA variant's shutter doors are 92
# source rows against the card's 91, so its canvas came out 53x74 while
# MAN/COM came out 53x73 and re-occupying an empty slot left a stale row of
# door art.  The box is the union -- the card's own x span (66) and the doors'
# own y span (92) -- and `convert_surface` refuses any placement that escapes
# it.
CSS_GATE_BOX = (22, 126, 66, 92)
CSS_EMBLEM_SYMBOL = ("llFTEmblemSpritesMarioSprite",
                     "llFTEmblemSpritesFoxSprite")
CSS_NAME_SYMBOL = ("llMNPlayersCommonMarioTextSprite",
                   "llMNPlayersCommonFoxTextSprite")
CSS_FIGHTER_TOKEN = ("MARIO", "FOX")
CSS_TINT_MAN = (0x1E, 0x1E, 0x1E)
CSS_TINT_COM = (0x44, 0x44, 0x44)
# (token suffix, gate LUT, doors shut, fighter index or None, emblem tint,
#  draws the name)
CSS_GATE_STATES = [
    ("NA", "GateCom%dPLUT", True, None, None, False),
    ("MAN", "GateMan%dPLUT", False, None, None, False),
    ("COM", "GateCom%dPLUT", False, None, None, False),
]
for _f in range(len(CSS_FIGHTER_TOKEN)):
    _name = CSS_FIGHTER_TOKEN[_f]
    CSS_GATE_STATES.append(
        (f"MAN_{_name}", "GateMan%dPLUT", False, _f, CSS_TINT_MAN, True))
    CSS_GATE_STATES.append(
        (f"COM_{_name}", "GateCom%dPLUT", False, _f, CSS_TINT_COM, False))
    CSS_GATE_STATES.append(
        (f"HOLD_{_name}", "GateCom%dPLUT", False, _f, CSS_TINT_COM, True))


def css_gate(player: int, state: str, lut: str, shut: bool,
             fighter: int | None, tint, with_name: bool) -> SurfaceSpec:
    start = player * 69
    parts = [Placement("MNPlayersCommon", "llMNPlayersCommonRedCardSprite",
                       start + 22, 126, False,
                       lut_symbol="llMNPlayersCommon" + (lut % (player + 1)))]
    if shut:
        parts.append(Placement("MNPlayersCommon",
                               "llMNPlayersCommonSmashLogoCardLeftSprite",
                               start + 22, 126, False))
        parts.append(Placement("MNPlayersCommon",
                               "llMNPlayersCommonSmashLogoCardRightSprite",
                               start + 47, 126, False))
    if fighter is not None:
        parts.append(Placement("FTEmblemSprites", CSS_EMBLEM_SYMBOL[fighter],
                               start + 24, 143, False, tint))
        if with_name:
            parts.append(Placement("MNPlayersCommon",
                                   CSS_NAME_SYMBOL[fighter],
                                   start + 22, 201, False))
    box = (start + CSS_GATE_BOX[0], CSS_GATE_BOX[1],
           CSS_GATE_BOX[2], CSS_GATE_BOX[3])
    return SurfaceSpec(f"CSS_GATE_{player}_{state}", tuple(parts), None,
                       under=css_screen_parts(), box=box)


for _player in range(4):
    for _state, _lut, _shut, _fighter, _tint, _name in CSS_GATE_STATES:
        SURFACE_SOURCES.append(
            css_gate(_player, _state, _lut, _shut, _fighter, _tint, _name))


# ---------------------------------------------------------------------------
# P2-1k, owner finding (c): THE STAGE SELECT'S PLAQUE, PLATE AND NAME ART.
# ---------------------------------------------------------------------------
#
# P2-1f shipped this screen's wallpaper, its ten icons and its cursor, and
# composed the two TEXT elements out of the menu font.  Everything else on the
# right-hand third -- the wooden circle the emblem sits in, the three-part name
# plate, the two decal fills, the tiled preview field -- was never converted,
# which is what "still needs lots of work" names.  `mnMapsFuncStart` builds the
# screen at :1654-1660 and the sites are:
#
#   mnMapsMakePreviewWallpaper (:919) llMNMapsTilesSprite every 16 px from 43
#                              to 139 at y 130 (7 copies, 16x82 each)
#   mnMapsMakePlaque   (:372)  llMNMapsWoodenCircleSprite at (189,124)
#   mnMapsLabelsProcDisplay (:389) gDPFillRectangle 160,128..320,134 at prim
#                              57/60/88 alpha FF, then 194,189..268,193 at black
#                              alpha 33 -- both G_RM_AA_XLU_SURF, so both are
#                              composited here at their own alpha
#   mnMapsMakeLabels   (:409)  llMNMapsStageSelectTextSprite at (172,122)
#                              through the IA lerp ENV 00/00/00 PRIM AF/B1/CC,
#                              then PlateLeft at (174,191), PlateMiddle every
#                              4 px from 186 to 258, PlateRight at (262,191)
#   mnMapsMakeEmblem   (:780)  llFTEmblemSprites<Series>Sprite at (189,124) +
#                              mnMapsSetLogoPosition's own per-gkind offset,
#                              tinted 5C/22/00; RANDOM instead draws
#                              llMNMapsQuestionMarkSprite at (223,144)
#   mnMapsMakeName     (:578)  llMNMaps<Ground>TextSprite at (183,196) in BLACK
#                              -- mnMapsSetNamePosition's REGION_US branch is a
#                              CONSTANT for every ground (:568), the per-ground
#                              table above it being the JP layout
#
# DRAW ORDER IS THE CAMERAS', AND IT RUNS THE OPPOSITE WAY TO INTUITION.  Each
# `mnMapsMake*Camera` passes its own `dl_link_priority` (objhelper.c:513) and a
# HIGHER value is drawn EARLIER, i.e. further back.  Two unambiguous cases in
# this same file prove the direction rather than assuming it: the WALLPAPER is
# 80 and must be the backmost thing on the screen, and the CURSOR is 50 against
# the ICONS' 60 and must draw over the icon it frames.  So the order is
# wallpaper 80, preview wallpaper 70, icons 60, cursor 50, PLAQUE 40, LABELS
# 30, name+emblem 20 -- which puts the STAGE SELECT text and the name plate IN
# FRONT of the wooden circle, not behind it.  They overlap: the circle spans
# (189,124)..(273,209) and the text (172,122)..(284,141).
SSS_EMBLEM_TINT = (0x5C, 0x22, 0x00)
# mnMapsSetLogoPosition, :752 -- indexed by gkind.  The tenth entry {34,20} is
# unreachable: gkind 0xDE takes the other branch and gets its own fixed site.
SSS_LOGO_POS = ((3, 19), (3, 19), (3, 20), (2, 20), (3, 17),
                (-1, 19), (1, 20), (1, 20), (3, 19))
# mnMapsMakeName's own offsets[], :581 -- gkind order.
SSS_NAME_SYMBOL = (
    "llMNMapsPeachsCastleTextSprite", "llMNMapsSectorZTextSprite",
    "llMNMapsCongoJungleTextSprite", "llMNMapsPlanetZebesTextSprite",
    "llMNMapsHyruleCastleTextSprite", "llMNMapsYoshisIslandTextSprite",
    "llMNMapsDreamLandTextSprite", "llMNMapsSaffronCityTextSprite",
    "llMNMapsMushroomKingdomTextSprite")
# mnMapsMakeEmblem's own offsets[], :784 -- gkind order, Mario twice.
SSS_EMBLEM_SYMBOL = (
    "llFTEmblemSpritesMarioSprite", "llFTEmblemSpritesFoxSprite",
    "llFTEmblemSpritesDonkeySprite", "llFTEmblemSpritesMetroidSprite",
    "llFTEmblemSpritesZeldaSprite", "llFTEmblemSpritesYoshiSprite",
    "llFTEmblemSpritesKirbySprite", "llFTEmblemSpritesPMonstersSprite",
    "llFTEmblemSpritesMarioSprite")
# mnMapsGetGroundKind, :455.  DERIVED, not copied from mnMapsGetSlot: that
# function is slot-of-gkind, and the numeric gkind values come from
# mnMapsMakeIcons indexing its icon table BY GKIND (:537) -- Castle->
# PeachsCastle=0, Jungle->CongoJungle=2, Hyrule->HyruleCastle=4,
# Zebes->PlanetZebes=3, Inishie->MushroomKingdom=8, Yoster->YoshisIsland=5,
# Pupupu->DreamLand=6, Sector->SectorZ=1, Yamabuki->SaffronCity=7.
SSS_SLOT_GKIND = (0, 2, 4, 3, 8, 5, 6, 1, 7, 0xDE)

SSS_BACKGROUND = (
    # mnMapsMakeWallpaper, :364, full-bleed per the P2-1k ruling.
    STONE_FULL_BLEED,
    # mnMapsPreviewWallpaperProcDisplay's own fill, :899 -- the dark plate the
    # tiles sit on.  SHIPPED rather than dropped even though the seven opaque
    # tiles cover 43..155 x 130..212 and the fill only 43..152 x 130..211: a
    # fill the source draws is composited, and letting the tiles "probably"
    # cover it is exactly the reasoning that loses an element.
    Placement("MNMaps", "", 43, 130, False, fill=(0x00, 0x00, 0x00, 0x73),
              size=(109, 81)),
    *(Placement("MNMaps", "llMNMapsTilesSprite", x, 130, False)
      for x in range(43, 155, 16)),
    # PLAQUE, camera 40 -- behind the labels.
    Placement("MNMaps", "llMNMapsWoodenCircleSprite", 189, 124, False),
    # LABELS, camera 30 -- both fills first (mnMapsLabelsProcDisplay draws them
    # before its own SObj list), then the text and the three-part plate.
    Placement("MNMaps", "", 160, 128, False, fill=(0x57, 0x60, 0x88, 0xFF),
              size=(160, 6)),
    Placement("MNMaps", "", 194, 189, False, fill=(0x00, 0x00, 0x00, 0x33),
              size=(74, 4)),
    Placement("MNMaps", "llMNMapsStageSelectTextSprite", 172, 122, False,
              (0xAF, 0xB1, 0xCC), env=(0x00, 0x00, 0x00)),
    Placement("MNMaps", "llMNMapsPlateLeftSprite", 174, 191, False),
    *(Placement("MNMaps", "llMNMapsPlateMiddleSprite", x, 191, False)
      for x in range(186, 262, 4)),
    Placement("MNMaps", "llMNMapsPlateRightSprite", 262, 191, False),
)

# The name+emblem pair is camera 20 -- the frontmost thing on the screen -- and
# `mnMapsMakeNameAndEmblem` (:818) ejects and re-makes it on every cursor move,
# so it is its own surface per slot, composited over the furniture above and
# sharing ONE declared box so a move is a single overwriting blit.  The box is
# the union of every variant: x from the name's own 183 to 183+96 (all nine
# name sprites are 96 wide), y from the highest emblem top (124+17) to the
# name's bottom (196+10).  All nine names are baked, not just Dream Land's:
# the cursor shows a name over whichever cell it is on, and P2-1f's
# font-composed version did the same for all nine.
SSS_PLAQUE_BOX = (183, 141, 96, 66)


def sss_plaque(slot: int) -> SurfaceSpec:
    gkind = SSS_SLOT_GKIND[slot]
    if gkind == 0xDE:
        # mnMapsMakeEmblem's RANDOM arm (:793): the question mark at its own
        # fixed site (mnMapsSetLogoPosition, :767), and NO name -- :831 skips
        # mnMapsMakeName for slot 9.
        parts = (Placement("MNMaps", "llMNMapsQuestionMarkSprite",
                           223, 144, False, SSS_EMBLEM_TINT),)
    else:
        dx, dy = SSS_LOGO_POS[gkind]
        parts = (
            Placement("FTEmblemSprites", SSS_EMBLEM_SYMBOL[gkind],
                      189 + dx, 124 + dy, False, SSS_EMBLEM_TINT),
            Placement("MNMaps", SSS_NAME_SYMBOL[gkind], 183, 196, False,
                      (0x00, 0x00, 0x00)),
        )
    return SurfaceSpec(f"SSS_PLAQUE_{slot}", parts, None,
                       under=SSS_BACKGROUND, box=SSS_PLAQUE_BOX)


SURFACE_SOURCES.append(SurfaceSpec("SSS_SCREEN", SSS_BACKGROUND, MENU_FIELD))
for _slot in range(10):
    SURFACE_SOURCES.append(sss_plaque(_slot))


# ---------------------------------------------------------------------------
# P2-1i -- the title screen's own background: `mnTitleMakeFire`.
# ---------------------------------------------------------------------------
#
# THE TITLE SCREEN'S BACKGROUND IS THE FIRE, and the owner overruled P2-1h's
# decision to sacrifice it.  What the source draws (mntitle.c:934-996) is ONE
# GObj carrying TWO SObjs over a black fill camera:
#
#   sobj[0]  texture 12  pos (-32, -16)  scale (12.0, 8.5)
#   sobj[1]  texture  0  pos (  8,   8)  scale ( 9.5,  7.0)
#
# and `mnTitleFireProcUpdate` advances EACH sobj's own texture index by one a
# tic, wrapping at 30 (`mnTitleUpdateFireSprite`, :906).  The phase difference
# is therefore constant at 12, so the pair takes exactly THIRTY distinct
# states and the whole animation is a thirty-entry cycle -- which is what makes
# it bakeable at all.  The textures are `llMNTitleFireAnimFrame1..30Sprite`,
# 32x32 RGBA32 each, blown up 12x/8.5x, so the screen image is a very
# low-frequency field and survives being stored small.
#
# The draw combiner is `colour = TEXEL0, alpha = TEXEL0 * PRIMITIVE`
# (mntitle.c:872) with the primitive alpha `sMNTitleFireAlpha`, which
# `mnTitleShowFire` sets to 0xFF on the branch this build takes (scene_prev is
# never the opening cinematic until P2-7), so the resting composite is the two
# layers alpha-blended at their own texel alpha over black.
#
# THE CAMERA COLOUR IS THE FIELD THE FIRE BURNS OVER, and it is NOT a
# modulator of the artwork -- `mnTitleFireProcDisplay` (:864) sets the combiner
# to RGB = TEXEL0, A = TEXEL0.a * PRIM.a, so the sprites carry their own colour
# and the camera's `color` reaches the screen as a literal fill: the fire
# camera is made with COBJ_FLAG_FILLCOLOR (:1393), which `gcPrepCamera` turns
# into G_CYC_FILL + gDPFillRectangle over the whole viewport
# (sys/objdisplay.c:2750-2756).
#
# THAT FILL IS LOAD-BEARING, measured, not assumed: both fire layers are
# SP_TRANSPARENT and semi-transparent nearly everywhere, so over the thirty
# states the fill's mean transmittance through the pair is 125.4/255 -- 49% of
# the title's field is the fill colour, and only 0.012% of texels are fully
# uncovered.  A bake onto BLACK therefore ships a title about half as bright as
# the source's, which is owner finding 4.  It is baked onto the fill instead.
#
# WHICH fill: `mnTitleInitVars` (:358) seeds a RANDOM one of seven
# `dMNTitleFireColors{R,G,B}` on our branch and `mnTitleFireCameraProcUpdate`
# (:1329) re-rolls it every 260 tics with an 80-tic crossfade.  All seven are
# near-white (min channel 0x64).  A DS background layer has no per-channel
# modulator at 16bpp, so a 16bpp bake has to pin one of the seven; it pins
# entry 0, which is exactly (0xFF, 0xFF, 0xFF) -- the table's own first entry
# and the identity.  THE PINNING IS THE APPROXIMATION, the black field was a
# defect.
FIRE_FILL = (0xFF, 0xFF, 0xFF)   # dMNTitleFireColors{R,G,B}[0]
FIRE_FRAMES = 30
FIRE_PHASE = 12          # sobj[0] starts at texture 12, sobj[1] at 0
FIRE_CELL_W = 51         # 5 columns x 51 = 255 <= 256
FIRE_CELL_H = 42         # 6 rows    x 42 = 252 <= 256
FIRE_COLS = 5
FIRE_ROWS = 6
FIRE_TEX = 32            # every frame sprite is 32x32

# The DS affine steps, in 8.8: PA = cell_w * 256 / 256, PD = cell_h * 256 /
# 192.  BG3 is an extended-rotscale bitmap (`bgInit(3, BgType_Bmp16,
# BgSize_B16_256x256, ...)`) so the whole upscale is the 2D hardware's, the
# frame select is the reference point, and a frame costs FOUR register writes.
FIRE_PA = (FIRE_CELL_W * 256) // DS_SCREEN_W
FIRE_PD = (FIRE_CELL_H * 256) // DS_SCREEN_H

# `sobj->pos`, `sprite.scalex/scaley` for the two layers, in the source's frame.
FIRE_LAYERS = ((-32.0, -16.0, 12.0, 8.5), (8.0, 8.0, 9.5, 7.0))


def _fire_sample(raster, u: float, v: float) -> tuple[int, int, int, int]:
    """Bilinear tap of one 32x32 fire texture.

    The atlas is an UPSAMPLE of the source on both axes (a 51-wide cell spans
    the 256 px screen, which spans about 21 of the layer's 32 texels), so
    bilinear is the reconstruction that matches what an N64 texture rectangle
    at G_TF_BILERP puts on the television -- and point sampling here would
    bake in blocks the hardware upscale would then magnify again.
    """
    if (u < 0.0) or (v < 0.0) or (u > (FIRE_TEX - 1)) or (v > (FIRE_TEX - 1)):
        return (0, 0, 0, 0)
    x0 = int(u)
    y0 = int(v)
    x1 = min(x0 + 1, FIRE_TEX - 1)
    y1 = min(y0 + 1, FIRE_TEX - 1)
    fx = u - x0
    fy = v - y0
    out = []
    for c in range(4):
        top = (raster[y0][x0][c] * (1.0 - fx)) + (raster[y0][x1][c] * fx)
        bot = (raster[y1][x0][c] * (1.0 - fx)) + (raster[y1][x1][c] * fx)
        out.append(int(round((top * (1.0 - fy)) + (bot * fy))))
    return (out[0], out[1], out[2], out[3])


def build_fire_atlas(cache: dict[str, RelocFile], offsets: dict[str, int],
                     repo_root: Path) -> Surface:
    """The thirty composited states, packed 5x6 into one 255x252 BG3 bitmap."""
    o2r = "MNTitleFireAnim"
    if o2r not in cache:
        cache[o2r] = RelocFile(o2r_path(repo_root, o2r))
    fileobj = cache[o2r]
    frames = []
    for i in range(FIRE_FRAMES):
        symbol = f"llMNTitleFireAnimFrame{i + 1}Sprite"
        if symbol not in offsets:
            raise ConvertError(f"{symbol} missing from the reloc headers")
        sprite, raster = decode_sprite_raster(fileobj, symbol, offsets[symbol],
                                              (255, 255, 255), alpha_ramp=True)
        if (sprite.width != FIRE_TEX) or (sprite.height != FIRE_TEX):
            raise ConvertError(
                f"{symbol}: {sprite.width}x{sprite.height}, expected "
                f"{FIRE_TEX}x{FIRE_TEX}")
        frames.append(raster)

    width = FIRE_COLS * FIRE_CELL_W
    height = FIRE_ROWS * FIRE_CELL_H
    texels = [0] * (width * height)
    # One atlas texel spans this many source-frame pixels on each axis; the
    # inverse of the affine the hardware will run.
    step_x = (DS_SCREEN_W / FIRE_CELL_W) * (FRAME_SCALE[1] / FRAME_SCALE[0])
    step_y = (DS_SCREEN_H / FIRE_CELL_H) * (FRAME_SCALE[1] / FRAME_SCALE[0])
    for t in range(FIRE_FRAMES):
        col = t % FIRE_COLS
        row = t // FIRE_COLS
        layers = ((frames[(FIRE_PHASE + t) % FIRE_FRAMES], FIRE_LAYERS[0]),
                  (frames[t % FIRE_FRAMES], FIRE_LAYERS[1]))
        for ay in range(FIRE_CELL_H):
            fy = (ay + 0.5) * step_y
            base = ((row * FIRE_CELL_H) + ay) * width + (col * FIRE_CELL_W)
            for ax in range(FIRE_CELL_W):
                fx = (ax + 0.5) * step_x
                # The fire camera's FILL is the starting colour, not black:
                # see FIRE_FILL.  Both layers then alpha-composite over it in
                # mnTitleFireProcDisplay's own draw order (base SObj, then
                # next), which is the order `layers` is built in.
                red, green, blue = FIRE_FILL
                for raster, (px, py, sx, sy) in layers:
                    r, g, b, a = _fire_sample(raster, (fx - px) / sx,
                                              (fy - py) / sy)
                    if a == 0:
                        continue
                    inv = 255 - a
                    red = ((r * a) + (red * inv)) // 255
                    green = ((g * a) + (green * inv)) // 255
                    blue = ((b * a) + (blue * inv)) // 255
                texels[base + ax] = rgba8_to_ds(red, green, blue, 255)
    return Surface("TITLE_FIRE_ATLAS", "TITLE_FIRE_ATLAS", width, height,
                   0, 0, True, texels)


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


# The four bright entry icons must stay contiguous and in mnModeSelectOption
# order: the main menu indexes them as NDS_MN_UI_KIT_IMAGE_MODE_ICON_1P +
# cursor, against kNdsUiKitModeEntrySite[cursor], so a reordered IMAGE_SOURCES
# would light the wrong entry -- and it would look deliberate, because every
# icon is a plausible thing to see lit.
MODE_ICON_TOKENS = ["MODE_ICON_1P", "MODE_ICON_VS", "MODE_ICON_OPTION",
                    "MODE_ICON_DATA"]


def check_mode_icon_block(images: list[Image]) -> None:
    tokens = [image.token for image in images]
    if not all(token in tokens for token in MODE_ICON_TOKENS):
        raise ConvertError("the main-menu entry icons are missing from the pack")
    first = tokens.index("MODE_ICON_1P")
    if tokens[first:first + 4] != MODE_ICON_TOKENS:
        raise ConvertError(
            "MODE_ICON_1P/VS/OPTION/DATA must be four consecutive entries in "
            f"mnModeSelectOption order; got {tokens[first:first + 4]}")


def title_anim_band(surfaces: list[Surface]) -> tuple[int, int]:
    """The rows the pop animation owns, read out of the bake's own placements.

    Below the border band and above the copyright band.  Those two are the only
    static art the animation's dirty rectangle could reach, and the reason it
    must not is (f2): both are anchored flush with the panel edges, and an
    erase that clipped either would undo that ruling one frame into the
    animation.  Taking the bound from `TITLE_SCREEN.sites` rather than writing
    8 and 157 is what keeps it true if the band artwork or the frame ratio
    move -- the numbers exist only in the bake.
    """
    title = next(s for s in surfaces if s.token == "TITLE_SCREEN")
    _bx, by, _bw, bh = title.sites["llMNTitleBorderUpperSprite"]
    _cx, cy, _cw, _ch = title.sites["llMNTitleCopyrightSprite"]
    top = by + bh
    bottom = cy
    if not (0 < top < bottom <= DS_SCREEN_H):
        raise ConvertError(
            f"title anim band ({top},{bottom}) is not a usable row range")
    return top, bottom


def check_title_anim_block(surfaces: list[Surface]) -> None:
    """Two oracles the pop animation's correctness rests on.

    1. TITLE_WORDMARK is TITLE_SCREEN, exactly, over its own rectangle from the
       band's top row down.  This is what makes the SETTLED frame the animation
       lands on bit-identical to the static title (f2) shipped -- the runtime
       blits this one surface at pose 51 and stops.
    2. Inside the animation's row band, TITLE_SCREEN carries NO art outside that
       rectangle.  The runtime erases its dirty rectangle to transparent every
       frame and only ever restores the wordmark rectangle, so a texel that
       failed this would be erased mid-animation and never come back.
    """
    title = next(s for s in surfaces if s.token == "TITLE_SCREEN")
    mark = next(s for s in surfaces if s.token == "TITLE_WORDMARK")
    top, bottom = title_anim_band(surfaces)

    mismatch = 0
    for y in range(mark.height):
        ty = mark.dst_y + y - title.dst_y
        if (mark.dst_y + y) < top:
            continue
        for x in range(mark.width):
            tx = mark.dst_x + x - title.dst_x
            if mark.texels[y * mark.width + x] != \
                    title.texels[ty * title.width + tx]:
                mismatch += 1
    if mismatch != 0:
        raise ConvertError(
            f"TITLE_WORDMARK differs from TITLE_SCREEN in {mismatch} texels; "
            "the settled animation frame would not be the static title")

    stray = 0
    for y in range(top, bottom):
        for x in range(title.width):
            if title.texels[y * title.width + x] == 0:
                continue
            if (mark.dst_x <= x < mark.dst_x + mark.width) and \
                    (mark.dst_y <= y < mark.dst_y + mark.height):
                continue
            stray += 1
    if stray != 0:
        raise ConvertError(
            f"{stray} TITLE_SCREEN texels sit inside the animation band "
            f"(rows {top}..{bottom}) but outside TITLE_WORDMARK; the "
            "animation's erase would destroy them permanently")

    # 3. PRESS START is the other thing on this screen's BG2, and it lands
    #    INSIDE the band -- (91,134)..(168,148) against rows 8..157 -- so the
    #    animation's erase reaches it on sixteen of the fifty-one poses.  The
    #    runtime's answer is the source's: the label is off the panel for the
    #    fifty-one animated tics and revealed at the snap (`mnTitleMakePressStart`
    #    leaves it hidden, mntitle.c:1261, and it is not shown until tic 280).
    #    Assert the containment that makes ONE erase enough: a label straddling
    #    the band's edge would leave the outside half on screen with the inside
    #    half erased, and no later frame would put either right.
    start = next(s for s in surfaces if s.token == "TITLE_PRESS_START")
    inside = ((start.dst_y >= top) and
              ((start.dst_y + start.height) <= bottom))
    outside = ((start.dst_y + start.height) <= top) or (start.dst_y >= bottom)
    if not (inside or outside):
        raise ConvertError(
            f"TITLE_PRESS_START rows {start.dst_y}.."
            f"{start.dst_y + start.height} straddle the animation band "
            f"({top}..{bottom}); it would be erased in part and restored in "
            "part on every animated frame")


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
        # THE TITLE'S FIELD IS THE FIRE, from the first presented frame -- on
        # our branch `mnTitleMakeFire` calls `mnTitleShowFire` at construction
        # (mntitle.c:990), so there is no black-field phase to preview.  These
        # composites run the BG3 affine the hardware runs, so a wrong PA/PD or
        # a wrong cell origin shows up here rather than on the owner's screen.
        # Two cells, because one still picture cannot show an animation.
        "screen_title": ((0x00, 0x00, 0x00),
                         ["TITLE_SCREEN", "TITLE_PRESS_START"], 0),
        "screen_title_cell15": ((0x00, 0x00, 0x00),
                                ["TITLE_SCREEN", "TITLE_PRESS_START"], 15),
        # The menus' own decal blue, mnmodeselect.c:517 (0x083365).
        "screen_menu": ((0x08, 0x33, 0x65), ["MODE_SELECT"], None),
        # P2-1j.  The two screens whose art is now STATE-dependent, composed in
        # the runtime's own blit order at the state the screen opens in: the VS
        # menu with the cursor on VS START and the rule on TIME, and the
        # character select with slot 1 human, slot 2 CPU and slots 3-4 empty.
        # A per-surface PNG cannot show that a button plate lands on the wrong
        # row or that a panel card covers the portrait grid; this can.
        "screen_vs_mode": ((0x08, 0x33, 0x65),
                           ["VS_MODE", "VS_BTN_START_HI",
                            "VS_BTN_RULE_TIME_NOT", "VS_BTN_TIME_NOT",
                            "VS_BTN_OPTIONS_NOT"], None),
        "screen_vs_mode_rule": ((0x08, 0x33, 0x65),
                                ["VS_MODE", "VS_BTN_START_NOT",
                                 "VS_BTN_RULE_STOCK_HI", "VS_BTN_STOCK_NOT",
                                 "VS_BTN_OPTIONS_NOT"], None),
        "screen_css": ((0x08, 0x33, 0x65),
                       ["CSS_SCREEN", "CSS_PANEL_0_MAN", "CSS_PANEL_1_COM",
                        "CSS_PANEL_2_NA", "CSS_PANEL_3_NA"], None),
    }
    for name, (field, tokens, fire_cell) in scenes.items():
        if not all(token in by_token for token in tokens):
            continue
        frame = [[field] * 256 for _ in range(192)]
        if (fire_cell is not None) and ("TITLE_FIRE_ATLAS" in by_token):
            atlas = by_token["TITLE_FIRE_ATLAS"]
            org_x = (fire_cell % FIRE_COLS) * FIRE_CELL_W
            org_y = (fire_cell // FIRE_COLS) * FIRE_CELL_H
            for y in range(192):
                sy = org_y + ((y * FIRE_PD) >> 8)
                for x in range(256):
                    sx = org_x + ((x * FIRE_PA) >> 8)
                    frame[y][x] = ds_texel_to_rgb(
                        atlas.texels[sy * atlas.width + sx], field)
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
        fileobj = RelocFile(o2r_path(repo_root, args.o2r_file))
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
            cache[o2r_name] = RelocFile(o2r_path(repo_root, o2r_name))
        images.append(convert_image(cache[o2r_name], symbol, token,
                                    offsets[symbol], scale, tint))
    check_digit_block(images)
    check_kind_label_block(images)
    check_mode_icon_block(images)

    surfaces = [convert_surface(cache, offsets, repo_root, spec)
                for spec in SURFACE_SOURCES]
    # The fire atlas is not a SurfaceSpec because it is not one composite at
    # one screen position: it is thirty of them tiled into a 255x252 sheet the
    # BG3 affine reads a cell out of, so it has its own builder and its own
    # blit path.  It rides in the same payload because that is one NitroFS
    # open the title screen already pays for.
    surfaces.append(build_fire_atlas(cache, offsets, repo_root))
    check_title_anim_block(surfaces)

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
