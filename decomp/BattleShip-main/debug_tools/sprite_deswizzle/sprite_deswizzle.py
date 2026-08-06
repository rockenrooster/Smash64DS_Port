#!/usr/bin/env python3
"""
sprite_deswizzle — Standalone fighter-title-card deswizzle experiment.

Can render either:
  - a single letter sprite from IFCommonAnnounceCommon (file_id 37), OR
  - a multi-strip fighter portrait from MVOpeningPortraitsSet1/Set2
    (file_ids 0x35/0x36), which are the big 300x55 RGBA16 sprites the
    mvOpeningRoom battlefield reveal cycles through.

It walks the intern reloc chain of whichever file it needs, resolves
the sprite's bitmap array and per-strip texel pointers, stitches the
strips back together, and renders the result to PNG using a
configurable swizzle strategy.

Usage:
  sprite_deswizzle.py letter <baserom.z64> <letter>   [--strategy NAME]
  sprite_deswizzle.py portrait <baserom.z64> <name>   [--strategy NAME]

  name examples: Samus Mario Fox Pikachu Link Kirby Donkey Yoshi

Strategies:
  none          Dump raw texel bytes, no swizzle
  port_current  Replicate port's current portFixupSpriteBitmapData logic
                (odd-row XOR4 qword swap, gated on bpp in {4,8,16})
  xor4          Plain odd-row XOR4 (swap two 4-byte halves in each qword)
  xor2          Odd-row XOR2 (swap two 2-byte halves in each dword)
  xor1          Odd-row XOR1 (swap byte pairs)
  xor8          Odd-row XOR8 (swap two 8-byte qwords within 16-byte chunks)
  all           Render every strategy

Output: debug_traces/sprite_deswizzle/<name>_<strategy>.png

Requires: Pillow (pip install Pillow)
"""

import argparse
import struct
import sys
from pathlib import Path

# --- Bring in the reloc extractor from the neighboring tool -----------------
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "reloc_extract"))
from reloc_extract import extract_file, read_table_entry  # noqa: E402

# G_IM_FMT_*
FMT_RGBA = 0
FMT_YUV  = 1
FMT_CI   = 2
FMT_IA   = 3
FMT_I    = 4

# G_IM_SIZ_*
SIZ_4b  = 0
SIZ_8b  = 1
SIZ_16b = 2
SIZ_32b = 3

# File ids and offsets — mirror tools/reloc_data_symbols.us.txt
IFCOMMON_ANNOUNCE_FILE_ID = 37

LETTER_OFFSETS = {
    "A": 0x05E0, "B": 0x09A8, "C": 0x0D80, "D": 0x1268,
    "E": 0x1628, "F": 0x1A00, "G": 0x1F08, "H": 0x2408,
    "I": 0x26B8, "J": 0x2A90, "K": 0x2F98, "L": 0x3358,
    "M": 0x3980, "N": 0x3E88, "O": 0x44B0, "P": 0x4890,
    "Q": 0x4F10, "R": 0x5418, "S": 0x57F0, "T": 0x5BD0,
    "U": 0x60D8, "V": 0x65D8, "W": 0x6C00, "X": 0x7108,
    "Y": 0x7608, "Z": 0x7AE8,
}

PORTRAITS_SET1_FILE_ID = 0x35  # 53
PORTRAITS_SET2_FILE_ID = 0x36  # 54

PORTRAIT_OFFSETS = {
    "Samus":   (PORTRAITS_SET1_FILE_ID, 0x9960),
    "Mario":   (PORTRAITS_SET1_FILE_ID, 0x13310),
    "Fox":     (PORTRAITS_SET1_FILE_ID, 0x1CCC0),
    "Pikachu": (PORTRAITS_SET1_FILE_ID, 0x26670),
    "Link":    (PORTRAITS_SET2_FILE_ID, 0x9960),
    "Kirby":   (PORTRAITS_SET2_FILE_ID, 0x13310),
    "Donkey":  (PORTRAITS_SET2_FILE_ID, 0x1CCC0),
    "Yoshi":   (PORTRAITS_SET2_FILE_ID, 0x26670),
}

# CharacterNames file (0x0c) — the per-fighter name textures used by the
# title-screen auto-demo (scAutoDemoInitSObjs).  These are the textures
# that say "MARIO", "FOX", "PIKACHU", etc. and appear in front of each
# fighter on their stage during the intro-room demo.
CHARACTER_NAMES_FILE_ID = 0x0C

CHARACTER_NAME_OFFSETS = {
    "Mario":   0x0138,
    "Fox":     0x0258,
    "Donkey":  0x0378,
    "Samus":   0x04F8,
    "Luigi":   0x0618,
    "Link":    0x0738,
    "Yoshi":   0x0858,
    "Captain": 0x0A38,
    "Kirby":   0x0BB8,
    "Pikachu": 0x0D38,
    "Purin":   0x0F78,
    "Ness":    0x1098,
}


# --- Reloc chain walker -----------------------------------------------------

def walk_reloc_chain(data: bytes, start_word: int) -> dict:
    """Walk the intern reloc chain and return slot_byte_off -> target_byte_off."""
    slot_to_target = {}
    cur = start_word
    while cur != 0xFFFF:
        off = cur * 4
        if off + 4 > len(data):
            break
        word = struct.unpack(">I", data[off:off + 4])[0]
        next_w = (word >> 16) & 0xFFFF
        target_w = word & 0xFFFF
        slot_to_target[off] = target_w * 4
        cur = next_w
    return slot_to_target


# --- Sprite/bitmap readers (BE-native from raw file) -----------------------

class Sprite:
    def __init__(self, data: bytes, sprite_off: int, slot_to_target: dict):
        self.off = sprite_off
        b = data[sprite_off:sprite_off + 68]
        (self.x, self.y, self.width, self.height) = struct.unpack(">hhhh", b[0:8])
        (self.scalex, self.scaley) = struct.unpack(">ff", b[8:16])
        (self.expx, self.expy, self.attr, self.zdepth) = struct.unpack(">hhHh", b[16:24])
        (self.r, self.g, self.bl, self.a) = b[24], b[25], b[26], b[27]
        (self.startTLUT, self.nTLUT) = struct.unpack(">hh", b[28:32])
        (self.LUT_token,) = struct.unpack(">I", b[32:36])
        (self.istart, self.istep) = struct.unpack(">hh", b[36:40])
        (self.nbitmaps, self.ndisplist) = struct.unpack(">hh", b[40:44])
        (self.bmheight, self.bmHreal) = struct.unpack(">hh", b[44:48])
        self.bmfmt = b[48]
        self.bmsiz = b[49]
        (self.bitmap_token,) = struct.unpack(">I", b[52:56])
        (self.rsp_dl_token,) = struct.unpack(">I", b[56:60])
        (self.rsp_dl_next_token,) = struct.unpack(">I", b[60:64])
        (self.frac_s, self.frac_t) = struct.unpack(">hh", b[64:68])

        # Resolve bitmap array via reloc chain
        slot = sprite_off + 52
        if slot not in slot_to_target:
            raise ValueError(f"sprite at 0x{sprite_off:X} has no reloc slot for bitmap")
        self.bitmap_off = slot_to_target[slot]

        # Resolve LUT (may be 0 for non-CI sprites)
        slot_lut = sprite_off + 32
        self.lut_off = slot_to_target.get(slot_lut, None)

    def bpp(self):
        return {SIZ_4b: 4, SIZ_8b: 8, SIZ_16b: 16, SIZ_32b: 32}.get(self.bmsiz, 0)

    def fmt_name(self):
        return {FMT_RGBA: "RGBA", FMT_YUV: "YUV", FMT_CI: "CI",
                FMT_IA: "IA", FMT_I: "I"}.get(self.bmfmt, f"?{self.bmfmt}")

    def summary(self):
        return (f"sprite@0x{self.off:X} {self.width}x{self.height} "
                f"scale=({self.scalex},{self.scaley}) attr=0x{self.attr:04X} "
                f"fmt={self.fmt_name()}{self.bpp()} nbitmaps={self.nbitmaps} "
                f"bmheight={self.bmheight}/{self.bmHreal} "
                f"bitmap@0x{self.bitmap_off:X}")


class Bitmap:
    def __init__(self, data: bytes, off: int, slot_to_target: dict):
        self.off = off
        b = data[off:off + 16]
        (self.width, self.width_img, self.s, self.t) = struct.unpack(">hhhh", b[0:8])
        (self.actualHeight, self.LUToffset) = struct.unpack(">hh", b[12:16])
        slot = off + 8
        if slot not in slot_to_target:
            raise ValueError(f"bitmap at 0x{off:X} has no reloc slot for buf")
        self.buf_off = slot_to_target[slot]

    def summary(self):
        return (f"bm@0x{self.off:X} w={self.width} w_img={self.width_img} "
                f"h={self.actualHeight} s={self.s} t={self.t} "
                f"buf@0x{self.buf_off:X}")


# --- Swizzle strategies ----------------------------------------------------

def _swap_halves(row_bytes: bytearray, row_w: int, half_bytes: int, full_bytes: int):
    """For each `full_bytes`-sized chunk in the row, swap the two `half_bytes`-sized halves."""
    for off in range(0, row_w - full_bytes + 1, full_bytes):
        tmp = bytes(row_bytes[off:off + half_bytes])
        row_bytes[off:off + half_bytes] = row_bytes[off + half_bytes:off + full_bytes]
        row_bytes[off + half_bytes:off + full_bytes] = tmp


def apply_swizzle(texels: bytes, width_img: int, height: int, bpp: int,
                  strategy: str) -> bytes:
    """
    Apply a deswizzle strategy to `texels` (DRAM-order bytes, BE as in the file).
    Returns a new bytes object with the swizzle applied to odd rows only
    (port's current approach).

    Strategies:
      none         — passthrough
      port_current — same as xor4 but gated on row_bytes >= 8 and qword-aligned
      xor4         — swap two 4-byte halves in each 8-byte qword (16bpp TMEM)
      xor2         — swap two 2-byte halves in each 4-byte dword
      xor1         — swap two 1-byte halves in each 2-byte word
      xor8         — swap two 8-byte halves in each 16-byte "qqword"
    """
    if strategy == "none":
        return texels

    row_bytes = (width_img * bpp + 7) // 8
    out = bytearray(texels)

    # Parameterize by swap chunk size
    if strategy in ("port_current", "xor4"):
        half, full = 4, 8
        if strategy == "port_current":
            if row_bytes < 8 or (row_bytes % 8) != 0:
                return bytes(out)
    elif strategy == "xor2":
        half, full = 2, 4
    elif strategy == "xor1":
        half, full = 1, 2
    elif strategy == "xor8":
        half, full = 8, 16
    else:
        raise ValueError(f"unknown strategy: {strategy}")

    for row in range(1, height, 2):
        start = row * row_bytes
        end = start + row_bytes
        row_bytes_ba = bytearray(out[start:end])
        _swap_halves(row_bytes_ba, row_bytes, half, full)
        out[start:end] = row_bytes_ba
    return bytes(out)


# --- Pixel decoders --------------------------------------------------------

def decode_ia8(texels: bytes, width_img: int, height: int) -> list:
    """Return list of (R,G,B,A) tuples, row-major, length width_img*height."""
    pixels = []
    for i in range(width_img * height):
        byte = texels[i] if i < len(texels) else 0
        I4 = (byte >> 4) & 0xF
        A4 = byte & 0xF
        I8 = I4 * 0x11
        A8 = A4 * 0x11
        pixels.append((I8, I8, I8, A8))
    return pixels


def decode_ia4(texels: bytes, width_img: int, height: int) -> list:
    """IA4: 4 bits per pixel, 3 intensity + 1 alpha. Two pixels per byte."""
    pixels = []
    for y in range(height):
        for x in range(width_img):
            byte_idx = (y * width_img + x) // 2
            if byte_idx >= len(texels):
                pixels.append((0, 0, 0, 0))
                continue
            byte = texels[byte_idx]
            nybble = (byte >> 4) & 0xF if (x & 1) == 0 else byte & 0xF
            I3 = (nybble >> 1) & 0x7
            A1 = nybble & 0x1
            I8 = (I3 << 5) | (I3 << 2) | (I3 >> 1)
            A8 = 0xFF if A1 else 0x00
            pixels.append((I8, I8, I8, A8))
    return pixels


def decode_ia16(texels: bytes, width_img: int, height: int) -> list:
    pixels = []
    for i in range(width_img * height):
        if i * 2 + 1 >= len(texels):
            pixels.append((0, 0, 0, 0))
            continue
        I8 = texels[i * 2]
        A8 = texels[i * 2 + 1]
        pixels.append((I8, I8, I8, A8))
    return pixels


def decode_i4(texels: bytes, width_img: int, height: int) -> list:
    pixels = []
    for y in range(height):
        for x in range(width_img):
            byte_idx = (y * width_img + x) // 2
            if byte_idx >= len(texels):
                pixels.append((0, 0, 0, 0))
                continue
            byte = texels[byte_idx]
            nybble = (byte >> 4) & 0xF if (x & 1) == 0 else byte & 0xF
            I8 = nybble * 0x11
            pixels.append((I8, I8, I8, I8))
    return pixels


def decode_i8(texels: bytes, width_img: int, height: int) -> list:
    pixels = []
    for i in range(width_img * height):
        I8 = texels[i] if i < len(texels) else 0
        pixels.append((I8, I8, I8, I8))
    return pixels


def decode_rgba16(texels: bytes, width_img: int, height: int) -> list:
    pixels = []
    for i in range(width_img * height):
        if i * 2 + 1 >= len(texels):
            pixels.append((0, 0, 0, 0))
            continue
        # N64 BE: addr[0]<<8 | addr[1]
        hi = texels[i * 2]
        lo = texels[i * 2 + 1]
        val = (hi << 8) | lo
        r5 = (val >> 11) & 0x1F
        g5 = (val >> 6) & 0x1F
        b5 = (val >> 1) & 0x1F
        a1 = val & 0x1
        r8 = (r5 << 3) | (r5 >> 2)
        g8 = (g5 << 3) | (g5 >> 2)
        b8 = (b5 << 3) | (b5 >> 2)
        a8 = 0xFF if a1 else 0x00
        pixels.append((r8, g8, b8, a8))
    return pixels


def decode_rgba32(texels: bytes, width_img: int, height: int) -> list:
    pixels = []
    for i in range(width_img * height):
        if i * 4 + 3 >= len(texels):
            pixels.append((0, 0, 0, 0))
            continue
        pixels.append((texels[i * 4], texels[i * 4 + 1],
                       texels[i * 4 + 2], texels[i * 4 + 3]))
    return pixels


def decode(texels: bytes, width_img: int, height: int,
           fmt: int, siz: int) -> list:
    if fmt == FMT_IA and siz == SIZ_4b:
        return decode_ia4(texels, width_img, height)
    if fmt == FMT_IA and siz == SIZ_8b:
        return decode_ia8(texels, width_img, height)
    if fmt == FMT_IA and siz == SIZ_16b:
        return decode_ia16(texels, width_img, height)
    if fmt == FMT_I and siz == SIZ_4b:
        return decode_i4(texels, width_img, height)
    if fmt == FMT_I and siz == SIZ_8b:
        return decode_i8(texels, width_img, height)
    if fmt == FMT_RGBA and siz == SIZ_16b:
        return decode_rgba16(texels, width_img, height)
    if fmt == FMT_RGBA and siz == SIZ_32b:
        return decode_rgba32(texels, width_img, height)
    raise NotImplementedError(f"fmt={fmt} siz={siz} not implemented")


# --- PNG writer -------------------------------------------------------------

def save_png(pixels: list, width: int, height: int, path: Path):
    try:
        from PIL import Image  # type: ignore
    except ImportError:
        # Fallback: minimal PPM writer (no alpha) so the script still runs
        # without Pillow.  Save as .ppm next to the .png.
        ppm = path.with_suffix(".ppm")
        with ppm.open("wb") as f:
            f.write(f"P6\n{width} {height}\n255\n".encode())
            for p in pixels:
                f.write(bytes(p[:3]))
        print(f"  wrote {ppm} (Pillow not available; PPM fallback)")
        return
    img = Image.new("RGBA", (width, height))
    img.putdata(pixels)
    img.save(path)
    print(f"  wrote {path}")


# --- Main entry -------------------------------------------------------------

STRATEGIES_ALL = [
    "none", "port_current", "xor4", "xor2", "xor1", "xor8",
]


def load_file(rom: bytes, file_id: int):
    """Return (decompressed_data, reloc_start_word)."""
    info = read_table_entry(rom, file_id)
    data = extract_file(rom, file_id)
    return data, info["reloc_intern"]


def render_sprite_to_pixels(sprite, bitmaps, data, strategy):
    """
    Render a multi-strip sprite by applying `strategy` to each strip and
    stitching the resulting rows back together into a single image.

    Returns (pixels, width, height). Each strip's width comes from its
    own bitmap (all equal in practice); height is the sum of each strip's
    actualHeight.
    """
    width_img = bitmaps[0].width_img
    flat = []
    total_height = 0
    for bm in bitmaps:
        if bm.width_img != width_img:
            raise ValueError(
                f"strip width mismatch: {bm.width_img} vs {width_img}")
        bpp = sprite.bpp()
        tex_bytes = (bm.width_img * bm.actualHeight * bpp + 7) // 8
        raw = data[bm.buf_off:bm.buf_off + tex_bytes]
        fixed = apply_swizzle(
            raw, bm.width_img, bm.actualHeight, bpp, strategy)
        pixels = decode(fixed, bm.width_img, bm.actualHeight,
                        sprite.bmfmt, sprite.bmsiz)
        flat.extend(pixels)
        total_height += bm.actualHeight
    return flat, width_img, total_height


def upscale(pixels, width, height, scale):
    scale = max(1, scale)
    if scale == 1:
        return pixels, width, height
    out = []
    for y in range(height):
        row = pixels[y * width:(y + 1) * width]
        for _ in range(scale):
            for p in row:
                out.extend([p] * scale)
    return out, width * scale, height * scale


def cmd_letter(args):
    letter = args.name.upper()
    if letter not in LETTER_OFFSETS:
        print(f"unknown letter {letter!r}; must be A-Z", file=sys.stderr)
        return 2

    rom = args.rom.read_bytes()
    data, reloc_start = load_file(rom, IFCOMMON_ANNOUNCE_FILE_ID)
    print(f"extracted {len(data)} bytes from file_id={IFCOMMON_ANNOUNCE_FILE_ID} "
          f"reloc_intern=0x{reloc_start:04X}", file=sys.stderr)

    slot_to_target = walk_reloc_chain(data, reloc_start)
    print(f"reloc chain has {len(slot_to_target)} entries", file=sys.stderr)

    sprite = Sprite(data, LETTER_OFFSETS[letter], slot_to_target)
    print(sprite.summary())

    bitmaps = []
    for i in range(sprite.nbitmaps):
        bm = Bitmap(data, sprite.bitmap_off + i * 16, slot_to_target)
        bitmaps.append(bm)
        print(" ", bm.summary())

    return render_and_save(args, sprite, bitmaps, data, letter)


def cmd_portrait(args):
    name = args.name
    key = next((k for k in PORTRAIT_OFFSETS if k.lower() == name.lower()), None)
    if key is None:
        print(f"unknown portrait {name!r}; valid: "
              f"{sorted(PORTRAIT_OFFSETS.keys())}", file=sys.stderr)
        return 2
    file_id, sprite_off = PORTRAIT_OFFSETS[key]
    return _render_sprite_from_file(args, file_id, sprite_off, key)


def cmd_charname(args):
    name = args.name
    key = next((k for k in CHARACTER_NAME_OFFSETS
                if k.lower() == name.lower()), None)
    if key is None:
        print(f"unknown character name {name!r}; valid: "
              f"{sorted(CHARACTER_NAME_OFFSETS.keys())}", file=sys.stderr)
        return 2
    sprite_off = CHARACTER_NAME_OFFSETS[key]
    return _render_sprite_from_file(args, CHARACTER_NAMES_FILE_ID,
                                    sprite_off, f"CharName_{key}")


def _render_sprite_from_file(args, file_id, sprite_off, stem):
    rom = args.rom.read_bytes()
    data, reloc_start = load_file(rom, file_id)
    print(f"extracted {len(data)} bytes from file_id={file_id} "
          f"reloc_intern=0x{reloc_start:04X}", file=sys.stderr)

    slot_to_target = walk_reloc_chain(data, reloc_start)
    print(f"reloc chain has {len(slot_to_target)} entries", file=sys.stderr)

    sprite = Sprite(data, sprite_off, slot_to_target)
    print(sprite.summary())

    bitmaps = []
    for i in range(sprite.nbitmaps):
        bm = Bitmap(data, sprite.bitmap_off + i * 16, slot_to_target)
        bitmaps.append(bm)
        print(" ", bm.summary())

    return render_and_save(args, sprite, bitmaps, data, stem)


def render_and_save(args, sprite, bitmaps, data, stem):
    args.out.mkdir(parents=True, exist_ok=True)
    strategies = STRATEGIES_ALL if args.strategy == "all" else [args.strategy]
    for strat in strategies:
        try:
            pixels, width, height = render_sprite_to_pixels(
                sprite, bitmaps, data, strat)
        except (ValueError, NotImplementedError) as e:
            print(f"  {strat}: {e}")
            continue
        scaled, sw, sh = upscale(pixels, width, height, args.scale)
        out_path = args.out / f"{stem}_{strat}.png"
        save_png(scaled, sw, sh, out_path)
    return 0


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)

    for sub_name, help_text in (
            ("letter", "render a single letter sprite from IFCommonAnnounceCommon"),
            ("portrait", "render a full fighter portrait from MVOpeningPortraitsSet{1,2}"),
            ("charname", "render a fighter-name texture from CharacterNames (auto-demo)")):
        p = sub.add_parser(sub_name, help=help_text)
        p.add_argument("rom", type=Path, help="baserom.us.z64")
        p.add_argument("name", help="letter A-Z or fighter name")
        p.add_argument("--strategy", default="all",
                       help="swizzle strategy or 'all'")
        p.add_argument("--out", type=Path,
                       default=Path("debug_traces/sprite_deswizzle"),
                       help="output directory")
        p.add_argument("--scale", type=int, default=2,
                       help="upscale factor for output PNG (default 2)")

    args = ap.parse_args()
    if args.cmd == "letter":
        return cmd_letter(args)
    elif args.cmd == "portrait":
        return cmd_portrait(args)
    elif args.cmd == "charname":
        return cmd_charname(args)
    return 2


if __name__ == "__main__":
    sys.exit(main())
