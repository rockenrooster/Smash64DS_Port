#!/usr/bin/env python3
"""Compact ground maps for the nine VS stages (P2-2p8 T1).

WHAT THIS BUILDS AND WHY
------------------------
`mpCollisionInitGroundData` sizes a stage's resident block with
`lbRelocGetFileSize(map)`, which is the map's whole extern tree, and the tree
of every one of the nine VS maps includes the stage's wallpaper container: a
158,928-byte O2R file (44 RGBA16 pixel strips, a 704-byte Bitmap[44] table and
a 72-byte Sprite; one layout for all nine, Sprite at 0x26C88). The DS never
reads those pixels. The background is the converted BG2 image
(`native_wallpaper_<stage>.bin`, generate_native_wallpapers.py); the resident
container is consulted for the Sprite header and for the *address* of its
Bitmap table, which is the key that names the converted image.

This script rewrites each GR<Stage>Map so the map carries those 776 bytes
itself (measured: lane1-stage-ground-files.md, artifacts/performance/
2026-09-28_p2-2p8-ram-supply/):

  * the container's Bitmap[44] table and Sprite are appended at the next
    16-byte boundary (+784 B with padding), byte for byte as the source has
    them -- the runtime's word swap and its Sprite normalizer treat them exactly
    as they treated the container's -- except every Bitmap.buf, which becomes
    NULL (nothing reads pixels);
  * MPGroundData.wallpaper (map header 0x14, +0x48) stops being an EXTERNAL
    fixup into the container and becomes an INTERNAL fixup to the appended
    Sprite; the appended Sprite.bitmap becomes an internal fixup to the
    appended Bitmap table;
  * the container's file id leaves the extern table together with its chain
    slot (order otherwise kept), so the extern tree the loader sizes and loads
    no longer contains the container: 158,144 B less per stage, all nine.

Every existing offset in the map is unchanged (the stub is appended), so the
map's own symbols, the packet's pinned roots and the Dream Land P1 golden pin
keep meaning what they meant. The packet asset SIZE of the map does grow, which
is why the runtime side is a P2-only translation (NDS_P2_COMPACT_GROUND_MAPS,
src/port/reloc_backend_assets.c) and not an edit of the pinned descriptors.

The source files under decomp/ are read only. Output goes to the path named on
the command line (the Makefile passes the build's NitroFS copy of the map).

Usage:
  python generate_compact_ground_maps.py --map reloc_stages/GRCastleMap \
         --output <nitrofs>/reloc/reloc_stages/GRCastleMap [--o2r-root DIR]
  python generate_compact_ground_maps.py --all --output-dir DIR [--o2r-root DIR]
  python generate_compact_ground_maps.py --check [--o2r-root DIR]
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

_p = Path(__file__).resolve().parent
while _p.name != "scripts":
    _p = _p.parent
sys.path.insert(0, str(_p))
import _paths  # noqa: F401,E402

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_O2R_ROOT = REPO_ROOT / "decomp" / "BattleShip-main" / "BattleShip_o2r"

O2R_MAGIC_OFFSET = 4
O2R_MAGIC = b"OLER"
O2R_RESOURCE_HEADER = 0x40
O2R_TABLE_OFFSET = O2R_RESOURCE_HEADER + 12  # extern-id table

# MPGroundData sits at 0x14 in all nine VS maps; `wallpaper` is at +0x48
# (gr_desc[4] 0x40, map_geometry 0x40, layer_mask 0x44, wallpaper 0x48).
GROUND_HEADER_OFFSET = 0x14
GROUND_WALLPAPER_OFFSET = 0x48
WALLPAPER_SLOT = GROUND_HEADER_OFFSET + GROUND_WALLPAPER_OFFSET

# The wallpaper container, identical for all nine (include/reloc_data.h).
CONTAINER_BITMAP_OFFSET = 0x269C8
CONTAINER_SPRITE_OFFSET = 0x26C88
CONTAINER_SIZE = 0x26CD0
BITMAP_COUNT = 44
BITMAP_BYTES = 16          # s16 width,width_img,s,t; void *buf; s16 h,LUToffset
BITMAP_BUF_OFFSET = 8
SPRITE_BYTES = CONTAINER_SIZE - CONTAINER_SPRITE_OFFSET   # 72: 68 + 4 pad
SPRITE_BITMAP_OFFSET = 0x34
STUB_BYTES = (CONTAINER_SPRITE_OFFSET - CONTAINER_BITMAP_OFFSET) + SPRITE_BYTES
STUB_GROWTH = (STUB_BYTES + 15) & ~15  # 784: what a compact map adds to its file

# Big-endian Sprite fields the source guarantees (N64 layout, sp.h).
SPRITE_WIDTH_HEIGHT_OFFSET = 4
SPRITE_NBITMAPS_OFFSET = 40
SPRITE_BMFMT_OFFSET = 48
SPRITE_BMSIZ_OFFSET = 49
G_IM_FMT_RGBA = 0
G_IM_SIZ_16B = 2


@dataclass(frozen=True)
class CompactMapSpec:
    label: str
    gkind: int                # gr/grdef.h order
    map_path: str             # under BattleShip_o2r
    map_file_id: int          # == the DS registry asset id of the map
    container_path: str
    container_file_id: int
    container_asset_id: int   # DS registry identity (kNDSNativeWallpapers key)
    header_offset: int = GROUND_HEADER_OFFSET   # llGR*MapMapHeader

    @property
    def wallpaper_slot(self) -> int:
        return self.header_offset + GROUND_WALLPAPER_OFFSET


# gkind order (gr/grdef.h:11-19). Container choice is what each map header's
# extern list names, which is what generate_native_wallpapers.SOURCES quotes;
# test_compact_ground_maps.py pins the two tables against each other.
MAPS: tuple[CompactMapSpec, ...] = (
    CompactMapSpec("Peach's Castle", 0, "reloc_stages/GRCastleMap", 259,
                   "reloc_movies/MVOpeningRoomWallpaper", 0x5A, 0x5A),
    CompactMapSpec("Sector Z", 1, "reloc_stages/GRSectorMap", 262,
                   "reloc_stages/StageSector", 0x63, 0x10063),
    CompactMapSpec("Kongo Jungle", 2, "reloc_stages/GRJungleMap", 261,
                   "reloc_stages/StageJungle", 0x5C, 0x1005C),
    CompactMapSpec("Planet Zebes", 3, "reloc_stages/GRZebesMap", 257,
                   "reloc_stages/StageZebes", 0x59, 0x10059),
    CompactMapSpec("Hyrule Castle", 4, "reloc_stages/GRHyruleMap", 265,
                   "reloc_stages/StageCastle", 0x5F, 0x1005F),
    CompactMapSpec("Yoshi's Island", 5, "reloc_stages/GRYosterMap", 263,
                   "reloc_stages/StageYoshi", 0x5D, 0x1005D),
    CompactMapSpec("Dream Land", 6, "reloc_stages/GRPupupuMap", 255,
                   "reloc_stages/StageDreamLand", 0x58, 0x10058),
    CompactMapSpec("Saffron City", 7, "reloc_stages/GRYamabukiMap", 264,
                   "reloc_stages/StagePokemon", 0x5E, 0x1005E),
    CompactMapSpec("Mushroom Kingdom", 8, "reloc_stages/GRInishieMap", 260,
                   "reloc_stages/StageHyruleWallpaper", 0x5B, 0x1005B),
    # The 1P arenas and boards (2026-10-01). Their MPGroundData sits at 0x14
    # like the VS maps' or at 0 (reloc_data.h llGR*MapMapHeader); Race to the
    # Finish names no wallpaper container and is not here.
    CompactMapSpec("Small Yoshi's Island", 12, "reloc_stages/GRYosterSmallMap", 270,
                   "reloc_stages/StageYoshi", 0x5D, 0x1005D),
    CompactMapSpec("Meta Crystal", 13, "reloc_stages/GRMetalMap", 269,
                   "reloc_stages/StageLastWallpaper", 0x62, 0x10062),
    CompactMapSpec("Duel Zone", 14, "reloc_stages/GRZakoMap", 268,
                   "reloc_stages/StageInishieWallpaper", 0x61, 0x10061),
    CompactMapSpec("Final Destination", 16, "reloc_stages/GRLastMap", 266,
                   "reloc_stages/StageYamabukiWallpaper", 0x60, 0x10060, 0x0),
    CompactMapSpec("Bonus 1 Mario", 17, "reloc_stages/GRBonus1MarioMap", 271,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 1 Fox", 18, "reloc_stages/GRBonus1FoxMap", 272,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 1 Donkey", 19, "reloc_stages/GRBonus1DonkeyMap", 273,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 1 Samus", 20, "reloc_stages/GRBonus1SamusMap", 274,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 1 Luigi", 21, "reloc_stages/GRBonus1LuigiMap", 275,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 1 Link", 22, "reloc_stages/GRBonus1LinkMap", 276,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 1 Yoshi", 23, "reloc_stages/GRBonus1YoshiMap", 277,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 1 Captain", 24, "reloc_stages/GRBonus1CaptainMap", 278,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 1 Kirby", 25, "reloc_stages/GRBonus1KirbyMap", 279,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 1 Pikachu", 26, "reloc_stages/GRBonus1PikachuMap", 280,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 1 Purin", 27, "reloc_stages/GRBonus1PurinMap", 281,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 1 Ness", 28, "reloc_stages/GRBonus1NessMap", 282,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Mario", 29, "reloc_stages/GRBonus2MarioMap", 283,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Fox", 30, "reloc_stages/GRBonus2FoxMap", 284,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Donkey", 31, "reloc_stages/GRBonus2DonkeyMap", 285,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Samus", 32, "reloc_stages/GRBonus2SamusMap", 286,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Luigi", 33, "reloc_stages/GRBonus2LuigiMap", 287,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Link", 34, "reloc_stages/GRBonus2LinkMap", 288,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Yoshi", 35, "reloc_stages/GRBonus2YoshiMap", 289,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Captain", 36, "reloc_stages/GRBonus2CaptainMap", 290,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Kirby", 37, "reloc_stages/GRBonus2KirbyMap", 291,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Pikachu", 38, "reloc_stages/GRBonus2PikachuMap", 292,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Purin", 39, "reloc_stages/GRBonus2PurinMap", 293,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
    CompactMapSpec("Bonus 2 Ness", 40, "reloc_stages/GRBonus2NessMap", 294,
                   "reloc_stages/StageMetalWallpaper", 0x77, 0x10077, 0x0),
)
MAPS_BY_PATH = {spec.map_path: spec for spec in MAPS}


class CompactMapError(Exception):
    """A source file is not the shape this transform was written against."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise CompactMapError(message)


def align16(value: int) -> int:
    return (value + 15) & ~15


@dataclass
class O2RFile:
    name: str
    header: bytes             # the 0x40-byte resource header, kept verbatim
    file_id: int
    internal_head: int
    external_head: int
    extern_ids: list[int]
    payload: bytearray        # big-endian source bytes


def parse_o2r(raw: bytes, name: str) -> O2RFile:
    require(len(raw) >= O2R_TABLE_OFFSET + 4 and
            raw[O2R_MAGIC_OFFSET:O2R_MAGIC_OFFSET + 4] == O2R_MAGIC,
            f"{name}: not an O2R file")
    file_id, internal_head, external_head, extern_count = struct.unpack_from(
        "<IHHI", raw, O2R_RESOURCE_HEADER)
    table_end = O2R_TABLE_OFFSET + 2 * extern_count
    require(table_end + 4 <= len(raw), f"{name}: truncated extern table")
    extern_ids = list(struct.unpack_from(f"<{extern_count}H", raw,
                                         O2R_TABLE_OFFSET)) if extern_count \
        else []
    data_size = struct.unpack_from("<I", raw, table_end)[0]
    payload = raw[table_end + 4:]
    require(len(payload) == data_size,
            f"{name}: payload {len(payload)} B != declared {data_size} B")
    return O2RFile(name, bytes(raw[:O2R_RESOURCE_HEADER]), file_id,
                   internal_head, external_head, extern_ids,
                   bytearray(payload))


def serialize_o2r(f: O2RFile) -> bytes:
    out = bytearray(f.header)
    out += struct.pack("<IHHI", f.file_id, f.internal_head, f.external_head,
                       len(f.extern_ids))
    out += struct.pack(f"<{len(f.extern_ids)}H", *f.extern_ids)
    out += struct.pack("<I", len(f.payload))
    out += f.payload
    return bytes(out)


def chain_slots(payload: bytes, head: int, name: str
                ) -> list[tuple[int, int]]:
    """[(slot byte offset, target byte offset)] in chain order.

    A slot word is (next slot word index << 16) | (target byte offset >> 2),
    big-endian, and 0xFFFF ends the chain: the walk the runtime does in
    ndsRelocApplyInternalPointerFixups / ndsRelocApplyExternalPointerFixups.
    """
    slots: list[tuple[int, int]] = []
    seen: set[int] = set()
    cursor = head
    while cursor != 0xFFFF:
        slot = cursor * 4
        require(slot + 4 <= len(payload) and slot not in seen,
                f"{name}: malformed relocation chain")
        seen.add(slot)
        word = struct.unpack_from(">I", payload, slot)[0]
        slots.append((slot, (word & 0xFFFF) * 4))
        cursor = word >> 16
    return slots


def write_chain(payload: bytearray, slots: list[tuple[int, int]],
                name: str) -> int:
    """Rewrite the chain words for `slots`; returns the head index."""
    if not slots:
        return 0xFFFF
    for i, (slot, target) in enumerate(slots):
        following = slots[i + 1][0] // 4 if i + 1 < len(slots) else 0xFFFF
        require(slot % 4 == 0 and target % 4 == 0 and target // 4 <= 0xFFFF and
                following <= 0xFFFF and slot + 4 <= len(payload),
                f"{name}: relocation slot/target out of range")
        struct.pack_into(">I", payload, slot, (following << 16) | (target // 4))
    return slots[0][0] // 4


@dataclass(frozen=True)
class CompactMap:
    data: bytes               # the complete O2R file
    source_size: int          # payload bytes of the source map
    compact_size: int         # payload bytes of the compact map
    stub_bitmap_offset: int   # Bitmap[44] table, payload offset
    stub_sprite_offset: int   # Sprite, payload offset
    tree_delta: int           # bytes the map's extern tree loses


def build_compact_map(spec: CompactMapSpec, map_raw: bytes,
                      container_raw: bytes) -> CompactMap:
    m = parse_o2r(map_raw, spec.map_path)
    c = parse_o2r(container_raw, spec.container_path)
    require(m.file_id == spec.map_file_id,
            f"{spec.map_path}: file id {m.file_id} != {spec.map_file_id}")
    require(c.file_id == spec.container_file_id,
            f"{spec.container_path}: file id {c.file_id:#x} != "
            f"{spec.container_file_id:#x}")
    require(len(c.payload) == CONTAINER_SIZE and c.extern_ids == [],
            f"{spec.container_path}: not the 0x{CONTAINER_SIZE:X}-byte "
            "self-contained wallpaper container")
    require(len(m.payload) % 16 == 0,
            f"{spec.map_path}: payload {len(m.payload)} B is not 16-aligned")

    # The container must be nothing but a Bitmap table and a Sprite whose only
    # pointers are the 44 Bitmap.buf slots and Sprite.bitmap.
    c_internal = chain_slots(c.payload, c.internal_head, c.name)
    c_external = chain_slots(c.payload, c.external_head, c.name)
    want_slots = {CONTAINER_BITMAP_OFFSET + i * BITMAP_BYTES + BITMAP_BUF_OFFSET
                  for i in range(BITMAP_COUNT)}
    want_slots.add(CONTAINER_SPRITE_OFFSET + SPRITE_BITMAP_OFFSET)
    require(not c_external and len(c_internal) == len(want_slots) and
            {slot for slot, _ in c_internal} == want_slots,
            f"{c.name}: relocation slots are not Bitmap.buf x{BITMAP_COUNT} + "
            "Sprite.bitmap")
    # The table the converted wallpaper is keyed on names this Bitmap table.
    require(dict(c_internal)[CONTAINER_SPRITE_OFFSET + SPRITE_BITMAP_OFFSET] ==
            CONTAINER_BITMAP_OFFSET,
            f"{c.name}: Sprite.bitmap does not point at "
            f"0x{CONTAINER_BITMAP_OFFSET:X}")
    sprite = bytes(c.payload[CONTAINER_SPRITE_OFFSET:CONTAINER_SIZE])
    width, height = struct.unpack_from(">hh", sprite,
                                       SPRITE_WIDTH_HEIGHT_OFFSET)
    (nbitmaps,) = struct.unpack_from(">h", sprite, SPRITE_NBITMAPS_OFFSET)
    require((width, height, nbitmaps) == (300, 220, BITMAP_COUNT) and
            sprite[SPRITE_BMFMT_OFFSET] == G_IM_FMT_RGBA and
            sprite[SPRITE_BMSIZ_OFFSET] == G_IM_SIZ_16B,
            f"{c.name}: Sprite is not the 300x220 RGBA16 x{BITMAP_COUNT} "
            "wallpaper")

    # The map's `wallpaper` slot must be the one external fixup into it.
    internal = chain_slots(m.payload, m.internal_head, m.name)
    external = chain_slots(m.payload, m.external_head, m.name)
    require(len(external) == len(m.extern_ids),
            f"{m.name}: extern chain and id table disagree")
    wallpaper_slot = spec.wallpaper_slot
    hits = [i for i, (slot, _) in enumerate(external) if slot == wallpaper_slot]
    require(len(hits) == 1, f"{m.name}: no unique external slot at "
            f"0x{wallpaper_slot:X}")
    k = hits[0]
    require(m.extern_ids[k] == spec.container_file_id and
            external[k][1] == CONTAINER_SPRITE_OFFSET and
            m.extern_ids.count(spec.container_file_id) == 1,
            f"{m.name}: wallpaper slot does not name "
            f"{spec.container_path}+0x{CONTAINER_SPRITE_OFFSET:X}")
    require(wallpaper_slot not in {slot for slot, _ in internal},
            f"{m.name}: wallpaper slot is also an internal fixup")

    stub_bitmaps = bytearray(c.payload[CONTAINER_BITMAP_OFFSET:
                                       CONTAINER_SPRITE_OFFSET])
    stub_sprite = bytearray(sprite)
    for i in range(BITMAP_COUNT):   # relocation words, not pixels: NULL
        struct.pack_into(">I", stub_bitmaps,
                         i * BITMAP_BYTES + BITMAP_BUF_OFFSET, 0)

    source_size = len(m.payload)
    bitmap_offset = source_size
    sprite_offset = source_size + len(stub_bitmaps)
    compact_size = align16(sprite_offset + len(stub_sprite))
    require(compact_size - source_size == STUB_GROWTH,
            f"{m.name}: stub growth {compact_size - source_size} != "
            f"{STUB_GROWTH}")
    m.payload.extend(stub_bitmaps)
    m.payload.extend(stub_sprite)
    m.payload.extend(bytes(compact_size - len(m.payload)))

    new_internal = internal + [
        (wallpaper_slot, sprite_offset),
        (sprite_offset + SPRITE_BITMAP_OFFSET, bitmap_offset),
    ]
    new_external = [e for i, e in enumerate(external) if i != k]
    m.internal_head = write_chain(m.payload, new_internal, m.name)
    m.external_head = write_chain(m.payload, new_external, m.name)
    del m.extern_ids[k]

    compact = CompactMap(serialize_o2r(m), source_size, compact_size,
                         bitmap_offset, sprite_offset,
                         align16(CONTAINER_SIZE) - STUB_GROWTH)
    verify_compact_map(spec, map_raw, container_raw, compact)
    return compact


def verify_compact_map(spec: CompactMapSpec, map_raw: bytes,
                       container_raw: bytes, compact: CompactMap) -> None:
    """Re-parse the finished file and re-derive every claim about it."""
    src = parse_o2r(map_raw, spec.map_path)
    con = parse_o2r(container_raw, spec.container_path)
    out = parse_o2r(compact.data, spec.map_path + " (compact)")
    payload = out.payload
    require(out.file_id == src.file_id and out.header == src.header,
            "compact map lost its identity")
    require(len(payload) == compact.compact_size and
            len(payload) == len(src.payload) + STUB_GROWTH and
            len(payload) % 16 == 0, "compact map size")
    require(compact.stub_bitmap_offset == len(src.payload) and
            compact.stub_sprite_offset ==
            len(src.payload) + BITMAP_COUNT * BITMAP_BYTES and
            compact.stub_bitmap_offset % 16 == 0,
            "stub placement")

    s_internal = chain_slots(src.payload, src.internal_head, src.name)
    s_external = chain_slots(src.payload, src.external_head, src.name)
    internal = chain_slots(payload, out.internal_head, out.name)
    external = chain_slots(payload, out.external_head, out.name)
    require(len(internal) == len(s_internal) + 2 and
            len(external) == len(s_external) - 1, "fixup counts")
    kept_external = [e for e in s_external if e[0] != spec.wallpaper_slot]
    require(external == kept_external, "external fixups changed")
    require(internal[:len(s_internal)] == s_internal and
            set(internal[len(s_internal):]) ==
            {(spec.wallpaper_slot, compact.stub_sprite_offset),
             (compact.stub_sprite_offset + SPRITE_BITMAP_OFFSET,
              compact.stub_bitmap_offset)}, "internal fixups")
    k = [e[0] for e in s_external].index(spec.wallpaper_slot)
    require(out.extern_ids == [x for i, x in enumerate(src.extern_ids)
                               if i != k] and
            spec.container_file_id not in out.extern_ids,
            "extern id table")

    # Every source byte outside the relocation words survives unchanged.
    moved = {slot for slot, _ in s_internal + s_external}
    for offset in range(0, len(src.payload), 4):
        if offset not in moved:
            require(payload[offset:offset + 4] == src.payload[offset:offset + 4],
                    f"source word 0x{offset:X} changed")
    # The stub is the container's Bitmap table and Sprite, pixels gone.
    stub = bytes(payload[compact.stub_bitmap_offset:
                         compact.stub_bitmap_offset + STUB_BYTES])
    want = bytearray(con.payload[CONTAINER_BITMAP_OFFSET:CONTAINER_SIZE])
    for i in range(BITMAP_COUNT):
        struct.pack_into(">I", want, i * BITMAP_BYTES + BITMAP_BUF_OFFSET, 0)
    sprite_bitmap = (compact.stub_sprite_offset - compact.stub_bitmap_offset
                     + SPRITE_BITMAP_OFFSET)
    struct.pack_into(">I", want, sprite_bitmap,
                     struct.unpack_from(">I", stub, sprite_bitmap)[0])
    require(stub == bytes(want), "stub bytes differ from the container's")
    require(not any(payload[compact.stub_bitmap_offset + STUB_BYTES:]),
            "stub padding is not zero")


def read_pair(o2r_root: Path, spec: CompactMapSpec) -> tuple[bytes, bytes]:
    paths = (o2r_root / spec.map_path, o2r_root / spec.container_path)
    for path in paths:
        require(path.is_file(), f"required input is absent: {path}")
    return paths[0].read_bytes(), paths[1].read_bytes()


def write_atomic(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + ".tmp")
    tmp.write_bytes(data)
    os.replace(tmp, path)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build the compact ground maps (wallpaper container "
                    "replaced by its 776-byte Sprite/Bitmap stub).")
    parser.add_argument("--o2r-root", type=Path, default=DEFAULT_O2R_ROOT,
                        help="BattleShip_o2r directory (default: the decomp "
                             "export in this repo)")
    parser.add_argument("--map", default=None,
                        help="one map, e.g. reloc_stages/GRCastleMap")
    parser.add_argument("--output", type=Path, default=None,
                        help="output file for --map")
    parser.add_argument("--all", action="store_true",
                        help="write all nine under --output-dir/<map path>")
    parser.add_argument("--output-dir", type=Path, default=None)
    parser.add_argument("--check", action="store_true",
                        help="build and verify all nine, write nothing")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    o2r_root = args.o2r_root.resolve()
    try:
        if args.map is not None:
            spec = MAPS_BY_PATH.get(args.map.replace("\\", "/"))
            require(spec is not None and args.output is not None,
                    "--map needs one of the compact maps and --output")
            compact = build_compact_map(spec, *read_pair(o2r_root, spec))
            write_atomic(args.output, compact.data)
            return 0
        require(args.check or (args.all and args.output_dir is not None),
                "give --map/--output, --all/--output-dir or --check")
        for spec in MAPS:
            compact = build_compact_map(spec, *read_pair(o2r_root, spec))
            if args.all:
                write_atomic(args.output_dir / spec.map_path, compact.data)
            print(f"{spec.label:17s} {Path(spec.map_path).name:14s} "
                  f"{compact.source_size:4d} -> {compact.compact_size:4d} B  "
                  f"stub bitmap 0x{compact.stub_bitmap_offset:04X} sprite "
                  f"0x{compact.stub_sprite_offset:04X}  tree "
                  f"-{compact.tree_delta:,} B")
    except CompactMapError as exc:
        print(f"generate_compact_ground_maps: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
