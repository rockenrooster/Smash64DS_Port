"""P7 C1: the contents' own 1P campaign art.

Two of the campaign's per-fighter pictures are tables of the source's twelve:

  * the congratulations still (mncongra.c dMNCongraPictures), a bottom and a
    top plate. Remix gives each of its characters its own pair
    (SinglePlayer.asm set_ending_image: the bottom plate's file, the top the
    next id, both sprites at VICTORY_OFFSET 0x20718 as the vanilla plates);
    the character's own .asm may set it again, and the last setting wins, as
    bass assembles them;
  * the stage intro's name label (sc1pintro.c sc1PIntroMakeName), a sprite in
    the CharacterNames file. Remix appends every character's to that file
    (SinglePlayer.asm add_to_single_player: the name texture's offset, its
    Sprite 0x10 past it; EXTRA's characters with literal offsets).

This reads both from the staging's sources. A content's plates are copied out
of the staged ROM as O2R (the reloc loader's format); its name sprite --
texels, bitmap and Sprite, as Remix lays them out -- becomes a small O2R file
of its own (id NAME_FILE_BASE + content), so the intro loads a few hundred
bytes instead of Remix's 58 KB names file. All of them go to nitro:/reloc/p4
(nds_p4.c ndsP4RelocAssetPath); the header lists them for
battleship_mncongra.c and battleship_sc1pintro.c, which show a content's in
its parent's place. An authored content (P6's, no Remix art; owner
2026-10-19) has its name spelled in the names' font and its plates made from
a still of its own model when one is there (assets/congra/c<NN>.png, local,
scripts/p6/p6_congra_still.py); without it, its parent's.

    p4_onep_art.py --staging <extra-*> --contents "<names>" --out <BUILD>/p4
writes <out>/nds_p4_onep_art.generated.h and <out>/onep_art/o2r/<id>.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import remix_rom as R  # noqa: E402

VICTORY_OFFSET = 0x20718
# The vanilla plates' size (decomp mncongra.c's files), every Remix one alike.
VICTORY_FILE_BYTES = 132960
# sc1pintro.c dSC1PIntroFileIDs[1], llCharacterNamesFileID.
NAMES_FILE = 0xC
# The name sprites' own files, one a content, past every staged and
# synthesized id (staged files end below 0x1700, the contents' synthesized
# blocks and exports below 0x1C00).
NAME_FILE_BASE = 0x1F00
# An authored content's plates: the 300x220 still the congratulations screen
# shows (its upper half the top plate at (10, 10), its lower the bottom at
# (10, 120): mncongra.c), as files in TEMPLATE_PLATE's layout (Mario's bottom
# plate; every plate, vanilla or Remix, shares its Sprite, bitmaps and
# strips), ids CONGRA_FILE_BASE + 2 * content (bottom) and the next (top).
CONGRA_DIR = HERE.parent.parent / "assets" / "congra"
TEMPLATE_PLATE = 186
CONGRA_FILE_BASE = 0x1F40
PLATE_W, PLATE_H = 300, 110


class OnePArtError(RuntimeError):
    pass


def png_rgba(path: Path) -> list[bytes]:
    """An 8-bit RGB or RGBA, non-interlaced PNG's rows as RGBA8 bytes."""
    import zlib
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise OnePArtError(f"{path}: not a PNG")
    at, idat, head = 8, b"", None
    while at < len(data):
        n = struct.unpack_from(">I", data, at)[0]
        tag = data[at + 4:at + 8]
        if tag == b"IHDR":
            head = struct.unpack_from(">IIBBBBB", data, at + 8)
        elif tag == b"IDAT":
            idat += data[at + 8:at + 8 + n]
        at += 12 + n
    width, height, depth, kind, _comp, _filt, interlace = head
    if depth != 8 or kind not in (2, 6) or interlace:
        raise OnePArtError(f"{path}: only 8-bit RGB/RGBA, non-interlaced")
    bpp = 3 if kind == 2 else 4
    raw = zlib.decompress(idat)
    stride = width * bpp
    rows, prev = [], bytearray(stride)
    for y in range(height):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                line[i] = (line[i] + a) & 0xFF
            elif f == 2:
                line[i] = (line[i] + b) & 0xFF
            elif f == 3:
                line[i] = (line[i] + ((a + b) >> 1)) & 0xFF
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 0xFF
        prev = line
        if bpp == 3:
            rows.append(b"".join(bytes(line[x:x + 3]) + b"\xff" for x in range(0, stride, 3)))
        else:
            rows.append(bytes(line))
    return rows


def plate_o2r(rom: R.Rom, rows: list[bytes], file_id: int) -> bytes:
    """One plate (PLATE_H rows of PLATE_W RGBA8 texels) as TEMPLATE_PLATE's
    file with its texels replaced: each bitmap's strip of rows as the staged
    plates store them, a strip's odd rows in TMEM order (the two 64-bit words
    of each 16 bytes swapped)."""
    data = bytearray(rom.file_bytes(TEMPLATE_PLATE))
    slots = rom.reloc_slots(TEMPLATE_PLATE)
    if len(data) != VICTORY_FILE_BYTES or rom.extern_ids(TEMPLATE_PLATE):
        raise OnePArtError(f"file {TEMPLATE_PLATE}: not a victory plate")
    bitmaps = slots[VICTORY_OFFSET + 52][1]
    y = 0
    for i in range(struct.unpack_from(">h", data, VICTORY_OFFSET + 40)[0]):
        at = bitmaps + 16 * i
        width, width_img = struct.unpack_from(">hh", data, at)
        count = struct.unpack_from(">h", data, at + 12)[0]
        buf = slots[at + 8][1]
        if (width, width_img) != (PLATE_W, PLATE_W) or y + count > PLATE_H:
            raise OnePArtError(f"file {TEMPLATE_PLATE}: unexpected plate strips")
        for r in range(count):
            line = bytearray(rows[y + r])
            if r & 1:
                for k in range(0, len(line), 16):
                    line[k:k + 8], line[k + 8:k + 16] = line[k + 8:k + 16], line[k:k + 8]
            data[buf + r * width_img * 4:buf + r * width_img * 4 + len(line)] = line
        y += count
    if y != PLATE_H:
        raise OnePArtError(f"file {TEMPLATE_PLATE}: {y} plate rows")
    blob = bytearray(rom.o2r(TEMPLATE_PLATE))
    head = len(R.O2R_HEADER)
    struct.pack_into("<I", blob, head, file_id)
    blob[head + 16:] = data
    return bytes(blob)


def file_constants(src: Path) -> dict[str, int]:
    text = (src / "File.asm").read_text(encoding="utf-8", errors="replace")
    return {m.group(1): int(m.group(2), 0)
            for m in re.finditer(r"constant\s+(\w+)\s*\(\s*(0x[0-9A-Fa-f]+|\d+)\s*\)", text)}


def asm_order(src: Path) -> list[Path]:
    """SinglePlayer.asm first, every other source after it (each character's
    own settings follow the shared table's)."""
    return [src / "SinglePlayer.asm"] + sorted(p for p in src.rglob("*.asm")
                                               if p.name != "SinglePlayer.asm")


def ending_images(src: Path, files: dict[str, int]) -> dict[str, int]:
    """Character name -> its bottom plate's file id."""
    pattern = re.compile(r"^\s*(?:SinglePlayer\.)?set_ending_image\(\s*Character\.id\.(\w+)\s*,"
                         r"\s*(File\.\w+|0x[0-9A-Fa-f]+|\d+)\s*\)", re.M)
    found: dict[str, int] = {}
    for path in asm_order(src):
        text = path.read_text(encoding="utf-8", errors="replace")
        for m in pattern.finditer(text):
            value = m.group(2)
            if value.startswith("File."):
                if value[5:] not in files:
                    raise OnePArtError(f"{path.name}: {value} is not in File.asm")
                found[m.group(1)] = files[value[5:]]
            else:
                found[m.group(1)] = int(value, 0)
    return found


def name_textures(src: Path) -> dict[str, int]:
    """Character name -> its name texture's offset in the names file (the
    Sprite is 0x10 past it: SinglePlayer.asm name_texture, "non-adjusted")."""
    text = (src / "SinglePlayer.asm").read_text(encoding="utf-8", errors="replace")
    at = text.find("scope name_texture {")
    if at < 0:
        raise OnePArtError("SinglePlayer.asm: no name_texture scope")
    end = text.find("\n    }", at)
    names = {m.group(1): int(m.group(2), 16)
             for m in re.finditer(r"constant\s+(\w+)\s*\(\s*(0x[0-9A-Fa-f]+)\s*\)", text[at:end])}
    pattern = re.compile(r"^\s*(?:SinglePlayer\.)?add_to_single_player\(\s*Character\.id\.(\w+)\s*,"
                         r"\s*(name_texture\.\w+|0x[0-9A-Fa-f]+)\s*,", re.M)
    found: dict[str, int] = {}
    for path in asm_order(src):
        body = path.read_text(encoding="utf-8", errors="replace")
        for m in pattern.finditer(body):
            value = m.group(2)
            if value.startswith("name_texture."):
                key = value[len("name_texture."):]
                if key not in names:
                    raise OnePArtError(f"{path.name}: {value} is not in name_texture")
                found[m.group(1)] = names[key]
            else:
                found[m.group(1)] = int(value, 16)
    return found


def name_sprite_o2r(rom: R.Rom, texture: int, file_id: int) -> tuple[bytes, int]:
    """The name Sprite at `texture` + 0x10 of the names file with its bitmap
    and texels as a file of its own; (the O2R, the Sprite's offset in it).
    Remix lays each out as texels, the bitmap (at `texture`), the Sprite."""
    data = rom.file_bytes(NAMES_FILE)
    slots = rom.reloc_slots(NAMES_FILE)
    bitmap = texture
    sprite = texture + 0x10
    where = f"{NAMES_FILE:#x}+{sprite:#x}"
    bm_ref = slots.get(sprite + 52)
    if bm_ref is None or bm_ref[0] != "intern" or bm_ref[1] != bitmap:
        raise OnePArtError(f"{where}: the name Sprite's bitmap is not at {bitmap:#x}")
    if struct.unpack_from(">h", data, sprite + 40)[0] != 1 or (sprite + 32) in slots:
        raise OnePArtError(f"{where}: the name is not one bitmap without a LUT")
    buf_ref = slots.get(bitmap + 8)
    if buf_ref is None or buf_ref[0] != "intern":
        raise OnePArtError(f"{where}: the name's texels are not internal")
    texels = buf_ref[1]
    # The names file's last Sprite ends it without its last word
    # (frac_s/frac_t), restored here as zero, as the Results ampersand's.
    end = sprite + 0x44
    if not (texels < bitmap and end - len(data) <= 4):
        raise OnePArtError(f"{where}: unexpected name layout")
    if sorted(o for o in slots if texels <= o < end) != [bitmap + 8, sprite + 52]:
        raise OnePArtError(f"{where}: the name's span holds other pointers")
    body = bytearray(data[texels:end].ljust(end - texels, b"\0"))
    first, second = (bitmap + 8 - texels) // 4, (sprite + 52 - texels) // 4
    struct.pack_into(">I", body, bitmap + 8 - texels, (second << 16) | 0)
    struct.pack_into(">I", body, sprite + 52 - texels,
                     (R.NO_RELOC << 16) | ((bitmap - texels) // 4))
    out = bytearray(R.O2R_HEADER)
    out += struct.pack("<IHHI", file_id, first, R.NO_RELOC, 0)
    out += struct.pack("<I", len(body))
    out += body
    return bytes(out), sprite - texels


# Owner 2026-10-19: an authored content's intro name is spelled from Remix's
# own names in the same font (every name is one 12-row I4 picture): each
# letter is cut out of a name that has it, the capital I as the L's stem.
# (letter, source name texture, the source's letters, index); the names are
# vanilla ones Remix keeps at the vanilla offsets.
SPELL_SOURCES = {
    "PEACH": (0xC558, "Peach"),
    "PIKACHU": (0xD28, "Pikachu"),
    "FALCO": (0x1448, "Falco"),
    "MARTH": (0x6578, "Marth"),
    "LINK": (0x728, "Link"),
}
SPELL_LETTERS = {
    "P": ("PEACH", 0), "a": ("PEACH", 2), "e": ("PEACH", 1), "l": ("FALCO", 2),
    "u": ("PIKACHU", 6), "t": ("MARTH", 3), "n": ("LINK", 2), "k": ("PIKACHU", 2),
    "I": ("LINK", 0),
}
SPELL_NAMES = {"ike": "Ike", "palutena": "Palutena"}


def i4_picture(data: bytes, slots: dict, texture: int) -> list[list[int]]:
    """A names-file I4 name as rows of 4-bit intensities, its odd rows'
    TMEM interleave undone (32-bit words swapped in pairs)."""
    sprite = texture + 0x10
    if data[sprite + 49] != 0 or data[sprite + 48] != 4:
        raise OnePArtError(f"name {texture:#x} is not I4")
    width = struct.unpack_from(">h", data, sprite + 4)[0]
    width_img = struct.unpack_from(">h", data, texture + 2)[0]
    rows = struct.unpack_from(">h", data, texture + 12)[0]
    buf = slots[texture + 8][1]
    stride = width_img // 2
    picture = []
    for y in range(rows):
        row = bytearray(data[buf + y * stride:buf + (y + 1) * stride])
        if y & 1:
            for i in range(0, stride, 8):
                row[i:i + 4], row[i + 4:i + 8] = row[i + 4:i + 8], row[i:i + 4]
        picture.append([(row[x >> 1] >> (0 if x & 1 else 4)) & 0xF for x in range(width)])
    return picture


def split_glyphs(picture: list[list[int]], count: int) -> list[list[int]]:
    """The word's `count` letters as column ranges: the cuts are the dimmest
    columns (letters touch through their edge shading) and belong to no
    letter; the font spaces its letters one such column apart."""
    ink = [sum(row[x] for row in picture) for x in range(len(picture[0]))]
    last = max(x for x, v in enumerate(ink) if v)
    first = min(x for x, v in enumerate(ink) if v)
    # best[k][x]: the least ink over k cuts with the k-th at column x.
    inf = float("inf")
    best = [[inf] * (last + 2) for _ in range(count)]
    back = [[-1] * (last + 2) for _ in range(count)]
    best[0][first - 1] = 0
    for k in range(1, count):
        for x in range(first + 1, last):
            for p in range(first - 1, x - 1):
                if best[k - 1][p] + ink[x] < best[k][x]:
                    best[k][x] = best[k - 1][p] + ink[x]
                    back[k][x] = p
    x = min(range(first + 1, last), key=lambda c: best[count - 1][c]) if count > 1 else first - 1
    cuts = [last]
    for k in range(count - 1, 0, -1):
        cuts.append(x)
        x = back[k][x]
    cuts.append(first - 1)
    cuts.reverse()
    cuts[-1] += 1     # the last letter keeps the last inked column
    return [list(range(cuts[i] + 1, cuts[i + 1])) for i in range(count)]


def spell_name(data: bytes, slots: dict, word: str) -> list[list[int]]:
    """`word` set from SPELL_LETTERS' cuts: one empty column, then each
    letter's columns and an empty column (the font's gap)."""
    pictures = {}
    columns = [[0] * 12]
    for letter in word:
        source, index = SPELL_LETTERS[letter]
        if source not in pictures:
            texture, text = SPELL_SOURCES[source]
            pictures[source] = (i4_picture(data, slots, texture), text)
        picture, text = pictures[source]
        if len(picture) != 12:
            raise OnePArtError(f"{source}: not a 12-row name")
        cut = split_glyphs(picture, len(text))[index]
        if letter == "I":
            # The L's stem: the columns its upper half inks (the foot only
            # inks the bottom rows), then a gap.
            upper = {x: sum(picture[y][x] for y in range(7)) for x in cut}
            peak = max(upper.values())
            cut = [x for x in cut if upper[x] * 2 >= peak] + [None]
        for x in cut:
            columns.append([0] * 12 if x is None else [picture[y][x] for y in range(12)])
        if cut[-1] is not None:
            columns.append([0] * 12)
    return [[col[y] for col in columns] for y in range(12)]


def spelled_name_o2r(rom: R.Rom, picture: list[list[int]], file_id: int) -> tuple[bytes, int, int]:
    """A spelled name as a names-file-style I4 Sprite in a file of its own,
    the template's bitmap and Sprite (Peach's) with its own size; (the O2R,
    the Sprite's offset, its width)."""
    data = rom.file_bytes(NAMES_FILE)
    template = SPELL_SOURCES["PEACH"][0]
    width = len(picture[0])
    width_img = (width + 15) & ~15
    stride = width_img // 2
    texels = bytearray()
    for y, row in enumerate(picture):
        packed = bytearray(stride)
        for x, v in enumerate(row):
            packed[x >> 1] |= (v & 0xF) << (0 if x & 1 else 4)
        if y & 1:
            for i in range(0, stride, 8):
                packed[i:i + 4], packed[i + 4:i + 8] = packed[i + 4:i + 8], packed[i:i + 4]
        texels += packed
    bitmap = bytearray(data[template:template + 0x10])
    sprite = bytearray(data[template + 0x10:template + 0x10 + 0x44].ljust(0x44, b"\0"))
    struct.pack_into(">hh", bitmap, 0, width, width_img)
    struct.pack_into(">h", bitmap, 12, len(picture))
    struct.pack_into(">hh", sprite, 4, width, len(picture))
    body = texels + bitmap + sprite
    at_bitmap = len(texels)
    at_sprite = at_bitmap + 0x10
    first, second = (at_bitmap + 8) // 4, (at_sprite + 52) // 4
    struct.pack_into(">I", body, at_bitmap + 8, (second << 16) | 0)
    struct.pack_into(">I", body, at_sprite + 52, (R.NO_RELOC << 16) | (at_bitmap // 4))
    out = bytearray(R.O2R_HEADER)
    out += struct.pack("<IHHI", file_id, first, R.NO_RELOC, 0)
    out += struct.pack("<I", len(body))
    out += body
    return bytes(out), at_sprite, width


def sprite_row(data: bytes, file_id: int, offset: int) -> tuple:
    """The reloc loader's normalize row for the Sprite at `offset` of a
    big-endian file body: (file, offset, width, height, bitmaps, fmt, siz)."""
    width, height = struct.unpack_from(">hh", data, offset + 4)
    bitmaps = struct.unpack_from(">h", data, offset + 40)[0]
    return (file_id, offset, width, height, bitmaps, data[offset + 48], data[offset + 49])


def write_if_changed(path: Path, blob: bytes) -> None:
    if not path.exists() or path.read_bytes() != blob:
        path.write_bytes(blob)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--contents", required=True, help="the enabled contents' names")
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--congra-dir", type=Path, default=CONGRA_DIR,
                    help="authored contents' congratulations stills (c<NN>.png)")
    args = ap.parse_args()

    registry = json.loads((HERE / "contents.json").read_text(encoding="utf-8"))["contents"]
    by_name = {c["name"]: c for c in registry}
    src = args.staging / "src"
    plates = ending_images(src, file_constants(src))
    names = name_textures(src)
    rom = R.Rom(args.staging / "ssb64asm.z64")

    o2r_dir = args.out / "onep_art" / "o2r"
    o2r_dir.mkdir(parents=True, exist_ok=True)
    congra: list[tuple[int, int, int]] = []
    intro: list[tuple[int, int, int]] = []
    sprites: list[tuple] = []
    for name in args.contents.split():
        content = by_name.get(name)
        if content is None:
            raise OnePArtError(f"{name}: not in contents.json")
        remix = content.get("remix")
        if remix is None:
            # An authored content: its name spelled in the names' font; its
            # plates are a rendered still of its own (owner 2026-10-19).
            word = SPELL_NAMES.get(name)
            if word is not None:
                fid = NAME_FILE_BASE + content["id"]
                picture = spell_name(rom.file_bytes(NAMES_FILE), rom.reloc_slots(NAMES_FILE), word)
                blob, offset, width = spelled_name_o2r(rom, picture, fid)
                write_if_changed(o2r_dir / f"{fid:04x}", blob)
                intro.append((content["id"], fid, offset))
                sprites.append((fid, offset, width, len(picture), 1, 4, 0))
            still = args.congra_dir / f"c{content['id']:02d}.png"
            if still.exists():
                rows = png_rgba(still)
                if len(rows) != 2 * PLATE_H or len(rows[0]) != 4 * PLATE_W:
                    raise OnePArtError(f"{still}: not {PLATE_W}x{2 * PLATE_H}")
                bottom = CONGRA_FILE_BASE + 2 * content["id"]
                template = rom.file_bytes(TEMPLATE_PLATE)
                for fid, half in ((bottom, rows[PLATE_H:]), (bottom + 1, rows[:PLATE_H])):
                    write_if_changed(o2r_dir / f"{fid:04x}", plate_o2r(rom, half, fid))
                    sprites.append(sprite_row(template, fid, VICTORY_OFFSET))
                congra.append((content["id"], bottom, bottom + 1))
            continue
        bottom = plates.get(remix)
        if bottom is not None and bottom >= 0x854:
            for fid in (bottom, bottom + 1):
                if len(rom.file_bytes(fid)) != VICTORY_FILE_BYTES or rom.extern_ids(fid):
                    raise OnePArtError(f"{name}: file {fid:#x} is not a victory plate")
                write_if_changed(o2r_dir / f"{fid:04x}", rom.o2r(fid))
                sprites.append(sprite_row(rom.file_bytes(fid), fid, VICTORY_OFFSET))
            congra.append((content["id"], bottom, bottom + 1))
        texture = names.get(remix)
        if texture is not None:
            fid = NAME_FILE_BASE + content["id"]
            blob, offset = name_sprite_o2r(rom, texture, fid)
            write_if_changed(o2r_dir / f"{fid:04x}", blob)
            intro.append((content["id"], fid, offset))
            sprites.append(sprite_row(rom.file_bytes(NAMES_FILE)[texture + 0x10 - offset:],
                                      fid, offset))

    ids = sorted([fid for _, b, t in congra for fid in (b, t)] + [fid for _, fid, _ in intro])
    lines = [
        "/* Generated by scripts/p4/p4_onep_art.py -- do not edit. */",
        "#ifndef NDS_P4_ONEP_ART_GENERATED_H",
        "#define NDS_P4_ONEP_ART_GENERATED_H",
        "",
        "/* X(content, bottom file, top file): a content's own congratulations",
        f" * plates (Remix's set_ending_image); both sprites at {VICTORY_OFFSET:#x}. */",
        f"#define NDS_P4_CONGRA_SPRITE_OFFSET {VICTORY_OFFSET:#x}u",
        "#define NDS_P4_CONGRA_ROWS(X) \\",
    ]
    lines += [f"    X({c}, {b:#x}u, {t:#x}u) \\" for c, b, t in congra]
    lines += ["    /* end */", "",
              "/* X(content, file, sprite offset): a content's own stage intro name",
              " * (Remix's add_to_single_player), a file of its own. */",
              "#define NDS_P4_INTRO_NAME_ROWS(X) \\"]
    lines += [f"    X({c}, {f:#x}u, {o:#x}u) \\" for c, f, o in intro]
    lines += ["    /* end */", "",
              "/* X(file, offset, width, height, bitmaps, fmt, siz): their Sprites, for",
              " * the reloc loader's normalize pass (reloc_backend_assets.c). */",
              "#define NDS_P4_ONEP_ART_SPRITE_ROWS(X) \\"]
    lines += [f"    X({f:#x}u, {o:#x}u, {w}u, {h}u, {n}u, {fm}u, {sz}u) \\"
              for f, o, w, h, n, fm, sz in sprites]
    lines += ["    /* end */", "",
              "/* Their files in nitro:/reloc/p4, ascending. */",
              f"#define NDS_P4_ONEP_ART_FILE_COUNT {len(ids)}u",
              "#define NDS_P4_ONEP_ART_FILE_IDS " + (", ".join(f"{i:#06x}" for i in ids) or "0"),
              "", "#endif", ""]
    # Written every run: make reruns this only when an input changed.
    (args.out / "nds_p4_onep_art.generated.h").write_text("\n".join(lines), encoding="utf-8",
                                                          newline="\n")
    print(f"p4_onep_art: {len(congra)} contents' plates, {len(intro)} names, {len(ids)} files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
