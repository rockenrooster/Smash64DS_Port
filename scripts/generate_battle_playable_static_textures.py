#!/usr/bin/env python3
"""Generate the canonical static Battle Playable DS texture corpus.

This is an offline M4 integration artifact, not a runtime implementation.  It
walks the pinned BattleShip Pupupu display lists for the three non-water stage
owners, reconstructs the exact primary texture keys consumed by the current DS
renderer, and converts their CI4/RGBA5551 inputs to padded little-endian DS
RGB5A1 bytes.  Exact metadata is emitted as C while pixels are emitted as one
NitroFS-ready binary.  Generated pixels are checked with a separate slow oracle.

The two source-initial Pupupu water composites are included for the retained
frame-0 freeze cut. Natural-lifecycle Whispy mouth/eye and Fox material keys are
also qualified from complete runtime captures and their pinned source bytes.
Other animated actors, fighter variants, weapons, effects, and shadows remain
outside this corpus, so this tool cannot claim M4 complete by itself.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Sequence

import _paths  # noqa: F401,E402  -- moved area modules stay importable

import generate_battle_playable_texture_census as census
import generate_pupupu_water_aot as water


OUTPUT_RELATIVE = Path(
    "src/nds/generated/battle_playable_static_textures.generated.inc"
)
PAYLOAD_OUTPUT_RELATIVE = Path(
    "assets/renderer/battle_playable_static_textures.rgb5a1.bin"
)

OWNER_SPECS = (
    ("pupupu_layer0", 0x1008, 22, 1 << 0),
    ("pupupu_layer1", 0x1CE0, 3, 1 << 1),
    ("pupupu_layer3", 0x2BF8, 5, 1 << 2),
)

FOX_LATE_MATERIAL_OWNER_MASK = 1 << 3
WHISPY_MOUTH_OWNER_MASK = 1 << 4
WHISPY_EYES_OWNER_MASK = 1 << 5
FLOWERS_BACK_OWNER_MASK = 1 << 6
FLOWERS_FRONT_OWNER_MASK = 1 << 7
OWNER_LABELS = tuple((name, mask) for name, _root, _count, mask in OWNER_SPECS) + (
    ("fox_late_material", FOX_LATE_MATERIAL_OWNER_MASK),
    ("whispy_mouth", WHISPY_MOUTH_OWNER_MASK),
    ("whispy_eyes", WHISPY_EYES_OWNER_MASK),
    ("flowers_back", FLOWERS_BACK_OWNER_MASK),
    ("flowers_front", FLOWERS_FRONT_OWNER_MASK),
)

# libnds GL_TEXTURE_TYPE_ENUM. GL_RGB16 is the DS's sixteen-colour paletted
# format at four bits a texel; GL_RGBA is RGB555 plus one alpha bit at sixteen.
DS_FORMAT_PAL16 = 3
DS_FORMAT_RGBA = 8
DS_PALETTE16_ENTRIES = 16

EXPECTED_KEY_COUNT = 35
EXPECTED_OUTPUT_COUNT = 33
# 136,192 / 132,096 until 2026-08-03, when repack_paletted put 22 of the 24
# textures back into the DS's sixteen-colour format their N64 sources were
# already in. Lossless -- EXPECTED_ORACLE_PIXELS is unchanged and the slow
# oracle still compares the same canonical 16-bit image -- and it returns 74,496
# bytes of texture VRAM. The two source-authored Whispy-eye frames add 1,024 B
# of PAL16 texels and 2,048 oracle pixels without changing that representation.
EXPECTED_RESIDENCY_BYTES = 72576
EXPECTED_PAYLOAD_BYTES = 71220
EXPECTED_ORACLE_PIXELS = 85760
EXPECTED_PAYLOAD_SHA256 = (
    "47a0a464d095df43fa470a001e65a9ef2b8d70591bdd0a03e9346c9436149415"
)
EXPECTED_METADATA_SHA256 = (
    "b3b70f058e7e8220bba48e893a26544ba1e34026dfefb242607177dda57413d5"
)
EXPECTED_INCLUDE_SHA256 = (
    # RE-PINNED 2026-08-05, and it is PURE PROVENANCE. The include stamps the
    # census digest in a header comment, so re-pinning the census (see
    # generate_battle_playable_texture_census.py) changes this file's bytes
    # without changing one texture. The diff is exactly one line:
    #   -/* Source census SHA256: 829c895d…. */
    #   +/* Source census SHA256: a7d04e3c…. */
    # EXPECTED_PAYLOAD_SHA256, EXPECTED_METADATA_SHA256, EXPECTED_RESIDENCY_BYTES
    # (61,696), EXPECTED_PAYLOAD_BYTES (61,210) and EXPECTED_ORACLE_PIXELS
    # (65,024) are all UNCHANGED, which is the proof that the corpus itself did
    # not move -- those are the guards over the data, this one is over the
    # emitted text. Same byte count before and after: 29,807.
    "b11bd4bdcc898a8a5a25b8c5b821950404dd559419ff1b724000d1d023a23c16"
)

G_SETTIMG = 0xFD
G_SETTILE = 0xF5
G_LOADTLUT = 0xF0
G_LOADBLOCK = 0xF3
G_LOADTILE = 0xF4
G_SETTILESIZE = 0xF2
G_TEXTURE = 0xD7
G_DL = 0xDE
G_ENDDL = 0xDF
G_TRI1 = 0x05
G_TRI2 = 0x06

FMT_RGBA = 0
FMT_CI = 2
FMT_IA = 3
SIZ_4B = 0
SIZ_8B = 1
SIZ_16B = 2
SIZ_32B = 3

LOAD_KIND_BLOCK = 1 << 5
LOAD_KIND_TILE = 1 << 6
LOAD_TILE = 7
RENDER_TILE = 0
TX_MIRROR = 1
TX_CLAMP = 2
TILE_RENDER_SEEN = 1 << 0
TILE_LOAD_SEEN = 1 << 1
TILE_S_CLAMP = 1 << 2
TILE_S_MIRROR = 1 << 3
TILE_S_MASKED = 1 << 4
TILE_T_CLAMP = 1 << 5
TILE_T_MIRROR = 1 << 6
TILE_T_MASKED = 1 << 7
DATA_LAYOUT_O2R_WORD_SWAPPED = 1
G_TX_DXT_ONE = 1 << 11
MAX_TEXTURE_DIMENSION = 128


class Falsifier(RuntimeError):
    """An exact source, key, conversion, or generated-output invariant moved."""


def falsify(message: str) -> Falsifier:
    return Falsifier(f"FALSIFIER: {message}")


def sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


@dataclass
class TileState:
    set_seen: bool = False
    size_seen: bool = False
    format: int = 0
    size: int = 0
    line: int = 0
    tmem: int = 0
    palette: int = 0
    cmt: int = 0
    maskt: int = 0
    shiftt: int = 0
    cms: int = 0
    masks: int = 0
    shifts: int = 0
    uls: int = 0
    ult: int = 0
    lrs: int = 0
    lrt: int = 0
    width: int = 0
    height: int = 0


@dataclass(frozen=True)
class LoadState:
    image: census.PointerRef
    image_format: int
    image_size: int
    image_width: int
    load_kind: int
    load_tile: int
    load_uls: int
    load_ult: int
    load_lrs: int
    load_dxt: int
    load_texels: int
    load_tmem: int


@dataclass
class DisplayState:
    image: census.PointerRef | None = None
    image_format: int = 0
    image_size: int = 0
    image_width: int = 0
    tlut_image: census.PointerRef | None = None
    tlut_count: int = 0
    texture_seen: bool = False
    texture_on: bool = False
    texture_tile: int = RENDER_TILE
    tiles: list[TileState] = field(
        default_factory=lambda: [TileState() for _ in range(8)]
    )
    loads: list[LoadState] = field(default_factory=list)


@dataclass
class PreparedRecord:
    owner_mask: int
    image: census.PointerRef
    tlut_image: census.PointerRef
    source_block: census.PointerRef
    key_words: tuple[int, ...]
    logical_width: int
    logical_height: int
    upload_width: int
    upload_height: int
    pixels: bytes
    key_sha256: str
    sites: set[int]
    payload_offset: int = 0
    payload_bytes: int = 0
    output_sha256: str = ""
    ds_format: int = 0
    palette: tuple[int, ...] = ()
    palette_offset: int = 0
    palette_entries: int = 0


@dataclass(frozen=True)
class GeneratedArtifacts:
    include: bytes
    payload: bytes
    records: tuple[PreparedRecord, ...]
    output_count: int
    residency_bytes: int
    oracle_pixels: int
    metadata_sha256: str
    census_sha256: str

    def summary(self) -> dict[str, object]:
        owner_counts = {
            name: sum(1 for record in self.records if record.owner_mask & mask)
            for name, mask in OWNER_LABELS
        }
        return {
            "schema": 2,
            "qualification": {
                "offline_static_payload_generated": True,
                "renderer_integrated": False,
                "runtime_keys_complete_for_p1": False,
                "gameplay_zero_conversion_proven": False,
                "m4_complete": False,
            },
            "owners": owner_counts,
            "source_block_count": len(
                {record.source_block.key() for record in self.records}
            ),
            "key_count": len(self.records),
            "unique_output_count": self.output_count,
            "residency_bytes": self.residency_bytes,
            "payload_bytes": len(self.payload),
            "oracle_pixels": self.oracle_pixels,
            "payload_sha256": sha256(self.payload),
            "metadata_sha256": self.metadata_sha256,
            "include_bytes": len(self.include),
            "include_sha256": sha256(self.include),
            "census_sha256": self.census_sha256,
        }


def source_bytes(format_: int, size: int, texels: int) -> int:
    if texels < 0:
        return 0
    if format_ == FMT_CI:
        if size == SIZ_4B:
            return (texels + 1) >> 1
        if size == SIZ_8B:
            return texels
        return 0
    if format_ == FMT_RGBA:
        if size == SIZ_16B:
            return texels * 2
        if size == SIZ_32B:
            return texels * 4
    if format_ == FMT_IA:
        if size == SIZ_4B:
            return (texels + 1) >> 1
        if size == SIZ_8B:
            return texels
        if size == SIZ_16B:
            return texels * 2
    return 0


def line_pixels(size: int, line: int) -> int:
    return {
        SIZ_4B: line * 16,
        SIZ_8B: line * 8,
        SIZ_16B: line * 4,
        SIZ_32B: line * 2,
    }.get(size, 0)


def next_pow2(value: int) -> int:
    result = 8
    while result < value and result < MAX_TEXTURE_DIMENSION:
        result <<= 1
    return result


def tile_flags(tile: TileState) -> int:
    flags = 0
    if tile.cms & TX_CLAMP:
        flags |= TILE_S_CLAMP
    if tile.cms & TX_MIRROR:
        flags |= TILE_S_MIRROR
    if tile.masks:
        flags |= TILE_S_MASKED
    if tile.cmt & TX_CLAMP:
        flags |= TILE_T_CLAMP
    if tile.cmt & TX_MIRROR:
        flags |= TILE_T_MIRROR
    if tile.maskt:
        flags |= TILE_T_MASKED
    return flags


def materializes_masked_clamp(
    mode: int, mask: int, source_extent: int, tile_extent: int
) -> bool:
    if (
        not (mode & TX_CLAMP)
        or mask == 0
        or mask >= 31
        or source_extent == 0
        or tile_extent > MAX_TEXTURE_DIMENSION
    ):
        return False
    mask_extent = 1 << mask
    return (
        tile_extent > mask_extent
        and source_extent >= mask_extent
        and source_extent <= tile_extent
    )


def masked_address(coord: int, mode: int, mask: int) -> int:
    extent = 1 << mask
    period = coord >> mask
    local = coord & (extent - 1)
    if (mode & TX_MIRROR) and (period & 1):
        local = extent - 1 - local
    return local


def n64_rgba5551_to_ds(color: int) -> int:
    if not (color & 1):
        return 0
    red = (color >> 11) & 0x1F
    green = (color >> 6) & 0x1F
    blue = (color >> 1) & 0x1F
    return 0x8000 | red | (green << 5) | (blue << 10)


def block_for_image(
    image: census.PointerRef, blocks: Sequence[dict[str, object]]
) -> tuple[census.PointerRef, int]:
    matches: list[tuple[census.PointerRef, int]] = []
    for block in blocks:
        identity = block["identity"]
        if not isinstance(identity, dict):
            raise falsify("static census block lost its identity")
        asset_id = int(identity["asset_id"])
        offset = int(identity["offset"])
        length = int(block["source_bytes"])
        if image.asset_id == asset_id and offset <= image.offset < offset + length:
            matches.append((census.PointerRef(asset_id, offset), length))
    if len(matches) != 1:
        raise falsify(
            f"image {image.key()} maps to {len(matches)} static source blocks"
        )
    return matches[0]


def active_load(state: DisplayState, tile: TileState) -> LoadState:
    for load in reversed(state.loads):
        if load.load_tmem == tile.tmem:
            return load
    raise falsify(f"render TMEM {tile.tmem} has no bounded texture load")


def make_key_words(
    state: DisplayState,
    tile_index: int,
    tile: TileState,
    load: LoadState,
    format_: int,
    size: int,
    width: int,
    height: int,
) -> tuple[int, ...]:
    if format_ == FMT_CI and state.tlut_image is None:
        raise falsify("static CI key has no TLUT image")
    if format_ != FMT_CI and state.tlut_image is not None:
        raise falsify("non-CI static key unexpectedly has a TLUT image")
    fields = {name: 0 for name in census.EXPECTED_KEY_FIELDS}
    fields.update(
        {
            "image": load.image.offset,
            "image_format": load.image_format,
            "image_size": load.image_size,
            "image_width": load.image_width,
            "tlut_image": state.tlut_image.offset if state.tlut_image else 0,
            "tlut_count": state.tlut_count,
            "data_layout": DATA_LAYOUT_O2R_WORD_SWAPPED,
            "format": format_,
            "size": size,
            "width": width,
            "height": height,
            "render_tile": tile_index,
            "render_tmem": tile.tmem,
            "render_palette": tile.palette,
            "render_tile_cms": tile.cms,
            "render_tile_cmt": tile.cmt,
            "render_tile_masks": tile.masks,
            "render_tile_maskt": tile.maskt,
            "render_tile_shifts": tile.shifts,
            "render_tile_shiftt": tile.shiftt,
            "load_tile": load.load_tile,
            "load_uls": load.load_uls,
            "load_ult": load.load_ult,
            "load_lrs": load.load_lrs,
            "load_dxt": load.load_dxt,
            "load_texels": load.load_texels,
            "tile_uls": tile.uls,
            "tile_ult": tile.ult,
            "tile_lrs": tile.lrs,
            "tile_lrt": tile.lrt,
            "line": tile.line,
            "flags": (
                TILE_RENDER_SEEN
                | (TILE_LOAD_SEEN if state.tiles[LOAD_TILE].set_seen else 0)
                | tile_flags(tile)
                | (load.load_kind << 8)
            ),
        }
    )
    words = tuple(int(fields[name]) for name in census.EXPECTED_KEY_FIELDS)
    if len(words) != 59:
        raise falsify(f"canonical key contains {len(words)} words, expected 59")
    if any(not 0 <= value <= 0xFFFFFFFF for value in words):
        raise falsify("canonical key contains a non-u32 value")
    return words


def resolve_key_geometry(
    state: DisplayState,
) -> tuple[int, TileState, LoadState, int, int, int, int, int, int, bool, bool]:
    tile_index = state.texture_tile if state.texture_seen else RENDER_TILE
    tile = state.tiles[tile_index]
    if not tile.set_seen or not tile.size_seen:
        raise falsify("static textured triangle has incomplete render-tile state")
    load = active_load(state, tile)
    format_ = tile.format
    size = tile.size
    if format_ == FMT_CI and size not in (SIZ_4B, SIZ_8B):
        if state.tlut_count <= 16:
            size = SIZ_4B
        elif state.tlut_count <= 256:
            size = SIZ_8B
        else:
            raise falsify("static CI key has invalid size and TLUT count")
    if (format_, size) not in ((FMT_CI, SIZ_4B), (FMT_IA, SIZ_8B)):
        raise falsify(
            f"static owner escaped the accepted CI4/IA8 lanes: "
            f"format={format_} size={size}"
        )
    loaded_bytes = load.load_texels * (4 if size == SIZ_32B else 2)
    width = tile.width
    height = tile.height
    if (
        width == 0
        or height == 0
        or width > MAX_TEXTURE_DIMENSION
        or height > MAX_TEXTURE_DIMENSION
        or source_bytes(format_, size, width * height) > loaded_bytes
    ):
        width = line_pixels(size, tile.line)
        texels = load.load_texels * 2
        if size == SIZ_4B:
            texels *= 2
        elif size in (SIZ_16B, SIZ_32B):
            texels //= 2
        height = texels // width if width else 0
    if not (0 < width <= MAX_TEXTURE_DIMENSION and 0 < height <= MAX_TEXTURE_DIMENSION):
        raise falsify(f"invalid static key dimensions {width}x{height}")
    source_extent_width = width
    source_extent_height = height
    materialize_s = materializes_masked_clamp(
        tile.cms, tile.masks, source_extent_width, tile.width
    )
    materialize_t = materializes_masked_clamp(
        tile.cmt, tile.maskt, source_extent_height, tile.height
    )
    if materialize_s:
        width = tile.width
    if materialize_t:
        height = tile.height
    upload_width = next_pow2(width)
    upload_height = next_pow2(height)
    if (
        upload_width < width
        or upload_height < height
        or upload_width > MAX_TEXTURE_DIMENSION
        or upload_height > MAX_TEXTURE_DIMENSION
    ):
        raise falsify(f"static key cannot be padded: {width}x{height}")
    if load.load_kind == LOAD_KIND_TILE:
        source_origin_s = load.load_uls >> 2
        source_origin_t = load.load_ult >> 2
        source_width = (
            load.image_width * 2
            if size == SIZ_4B and load.image_size == SIZ_8B
            else load.image_width
        )
    else:
        source_origin_s = 0
        source_origin_t = 0
        source_width = source_extent_width
        if load.load_dxt:
            qwords = (G_TX_DXT_ONE + load.load_dxt - 1) // load.load_dxt
            source_width = line_pixels(size, qwords)
    source_read_width = (1 << tile.masks) if materialize_s else width
    source_read_height = (1 << tile.maskt) if materialize_t else height
    if (
        source_width == 0
        or source_origin_s >= source_width
        or source_read_width > source_width - source_origin_s
    ):
        raise falsify("static key has an invalid source range")
    return (
        tile_index,
        tile,
        load,
        format_,
        size,
        width,
        height,
        upload_width,
        upload_height,
        materialize_s,
        materialize_t,
    )


def convert_fast(
    images: census.O2RResource,
    state: DisplayState,
    tile: TileState,
    load: LoadState,
    width: int,
    height: int,
    upload_width: int,
    upload_height: int,
    materialize_s: bool,
    materialize_t: bool,
) -> tuple[bytes, int]:
    if load.image.asset_id != images.file_id:
        raise falsify("static texels are not in their pinned image asset")
    qwords = (G_TX_DXT_ONE + load.load_dxt - 1) // load.load_dxt
    source_width = line_pixels(tile.size, qwords)
    source_read_width = (1 << tile.masks) if materialize_s else width
    source_read_height = (1 << tile.maskt) if materialize_t else height
    source_last = (source_read_height - 1) * source_width + source_read_width - 1
    required = source_bytes(tile.format, tile.size, source_last + 1)
    if load.image.offset + required > len(images.payload):
        raise falsify(f"texel source {load.image.key()} exceeds asset bounds")
    palette: tuple[int, ...] = ()
    palette_base = 0
    if tile.format == FMT_CI:
        if state.tlut_image is None or state.tlut_image.asset_id != images.file_id:
            raise falsify("static TLUT is not in its pinned image asset")
        palette_base = tile.palette * 16
        palette_entries = palette_base + 16
        if state.tlut_count < palette_entries:
            raise falsify("static CI4 key has an incomplete TLUT")
        palette_end = state.tlut_image.offset + palette_entries * 2
        if palette_end > len(images.payload):
            raise falsify(f"TLUT source {state.tlut_image.key()} exceeds asset bounds")
        palette = tuple(
            n64_rgba5551_to_ds(
                struct.unpack_from(
                    ">H", images.payload, state.tlut_image.offset + i * 2
                )[0]
            )
            for i in range(palette_entries)
        )
    output = [0] * (upload_width * upload_height)
    for y in range(height):
        source_y = masked_address(y, tile.cmt, tile.maskt) if materialize_t else y
        for x in range(width):
            source_x = masked_address(x, tile.cms, tile.masks) if materialize_s else x
            source_index = source_y * source_width + source_x
            if tile.format == FMT_CI:
                packed = images.payload[load.image.offset + (source_index >> 1)]
                palette_index = (
                    (packed >> 4) if not (source_index & 1) else (packed & 0xF)
                )
                color = palette[palette_base + palette_index]
            else:
                value = images.payload[load.image.offset + (source_index ^ 3)]
                intensity = (value >> 4) * 0x11
                alpha = (value & 0xF) * 0x11
                gray = intensity >> 3
                color = 0 if alpha == 0 else 0x8000 | gray | (gray << 5) | (gray << 10)
            output[y * upload_width + x] = color
    return b"".join(struct.pack("<H", color) for color in output), width * height


def convert_slow_oracle(
    images: census.O2RResource,
    state: DisplayState,
    tile: TileState,
    load: LoadState,
    width: int,
    height: int,
    upload_width: int,
    upload_height: int,
    materialize_s: bool,
    materialize_t: bool,
) -> bytes:
    if tile.format == FMT_CI and state.tlut_image is None:
        raise falsify("slow oracle has no TLUT")
    result = bytearray(upload_width * upload_height * 2)
    qwords = (2048 + load.load_dxt - 1) // load.load_dxt
    source_stride = line_pixels(tile.size, qwords)
    for output_y in range(upload_height):
        for output_x in range(upload_width):
            color = 0
            if output_x < width and output_y < height:
                source_x = output_x
                source_y = output_y
                if materialize_s:
                    period = source_x >> tile.masks
                    source_x &= (1 << tile.masks) - 1
                    if (tile.cms & 1) and (period & 1):
                        source_x = (1 << tile.masks) - 1 - source_x
                if materialize_t:
                    period = source_y >> tile.maskt
                    source_y &= (1 << tile.maskt) - 1
                    if (tile.cmt & 1) and (period & 1):
                        source_y = (1 << tile.maskt) - 1 - source_y
                logical_texel = source_y * source_stride + source_x
                if tile.format == FMT_CI:
                    source_byte = images.payload[
                        load.image.offset + logical_texel // 2
                    ]
                    ci = (
                        source_byte >> 4
                        if logical_texel % 2 == 0
                        else source_byte & 15
                    )
                    assert state.tlut_image is not None
                    n64 = struct.unpack_from(
                        ">H",
                        images.payload,
                        state.tlut_image.offset + (tile.palette * 16 + ci) * 2,
                    )[0]
                    if n64 & 1:
                        color = (
                            0x8000
                            | ((n64 >> 11) & 31)
                            | (((n64 >> 6) & 31) << 5)
                            | (((n64 >> 1) & 31) << 10)
                        )
                else:
                    value = images.payload[
                        load.image.offset + (logical_texel ^ 3)
                    ]
                    if value & 0xF:
                        gray = ((value >> 4) * 0x11) >> 3
                        color = 0x8000 | gray | (gray << 5) | (gray << 10)
            struct.pack_into("<H", result, (output_y * upload_width + output_x) * 2, color)
    return bytes(result)


def capture_record(
    owner_mask: int,
    command_offset: int,
    state: DisplayState,
    images: census.O2RResource,
    blocks: Sequence[dict[str, object]],
) -> PreparedRecord:
    (
        tile_index,
        tile,
        load,
        format_,
        size,
        width,
        height,
        upload_width,
        upload_height,
        materialize_s,
        materialize_t,
    ) = resolve_key_geometry(state)
    if format_ == FMT_CI and state.tlut_image is None:
        raise falsify("static CI key lost its TLUT")
    source_block, block_bytes = block_for_image(load.image, blocks)
    pixels, oracle_pixels = convert_fast(
        images,
        state,
        tile,
        load,
        width,
        height,
        upload_width,
        upload_height,
        materialize_s,
        materialize_t,
    )
    oracle = convert_slow_oracle(
        images,
        state,
        tile,
        load,
        width,
        height,
        upload_width,
        upload_height,
        materialize_s,
        materialize_t,
    )
    if pixels != oracle:
        for index, (actual, expected) in enumerate(zip(pixels, oracle)):
            if actual != expected:
                raise falsify(
                    f"{load.image.key()}: pixel oracle byte mismatch at {index}: "
                    f"{actual} != {expected}"
                )
        raise falsify(f"{load.image.key()}: pixel oracle length mismatch")
    key_words = make_key_words(
        state, tile_index, tile, load, format_, size, width, height
    )
    source_last = (
        ((1 << tile.maskt) if materialize_t else height) - 1
    ) * line_pixels(
        size, (G_TX_DXT_ONE + load.load_dxt - 1) // load.load_dxt
    ) + ((1 << tile.masks) if materialize_s else width) - 1
    required_bytes = source_bytes(format_, size, source_last + 1)
    if load.image.offset + required_bytes > source_block.offset + block_bytes:
        raise falsify(
            f"{load.image.key()}: prepared span escapes census block "
            f"{source_block.key()}"
        )
    canonical_key = {
        "image_asset_id": load.image.asset_id,
        "tlut_asset_id": state.tlut_image.asset_id if state.tlut_image else 0,
        "key_words": key_words,
    }
    tlut_image = state.tlut_image or census.PointerRef(0, 0)
    return PreparedRecord(
        owner_mask=owner_mask,
        image=load.image,
        tlut_image=tlut_image,
        source_block=source_block,
        key_words=key_words,
        logical_width=width,
        logical_height=height,
        upload_width=upload_width,
        upload_height=upload_height,
        pixels=pixels,
        key_sha256=sha256(
            json.dumps(
                canonical_key, sort_keys=True, separators=(",", ":")
            ).encode("ascii")
        ),
        sites={command_offset},
        output_sha256=sha256(pixels),
    )


def build_runtime_qualified_water_support_record(
    repo_root: Path, blocks: Sequence[dict[str, object]]
) -> PreparedRecord:
    """Build native stage run 41's exact clamped water-support card key."""
    images = census.load_o2r(repo_root, census.O2R_INPUTS["stage_images"])
    state = DisplayState(
        tlut_image=census.PointerRef(images.file_id, 0x1858),
        tlut_count=16,
        texture_seen=True,
        texture_on=True,
        texture_tile=0,
    )
    state.tiles[0] = TileState(
        set_seen=True,
        size_seen=True,
        format=FMT_CI,
        size=SIZ_4B,
        line=2,
        tmem=0,
        palette=0,
        cmt=2,
        maskt=5,
        shiftt=0,
        cms=2,
        masks=5,
        shifts=0,
        uls=0,
        ult=0,
        lrs=0x2FC,
        lrt=0x17C,
        width=32,
        height=96,
    )
    state.tiles[LOAD_TILE] = TileState(set_seen=True)
    state.loads = [
        LoadState(
            image=census.PointerRef(images.file_id, 0x1880),
            image_format=FMT_CI,
            image_size=SIZ_16B,
            image_width=1,
            load_kind=LOAD_KIND_BLOCK,
            load_tile=LOAD_TILE,
            load_uls=0,
            load_ult=0,
            load_lrs=0xFF,
            load_dxt=0x400,
            load_texels=0x100,
            load_tmem=0,
        )
    ]
    record = capture_record(1 << 2, 0x18E0, state, images, blocks)
    expected_key = (
        0x1880, 2, 2, 1, 0x1858, 16, 1, 2,
        0, 32, 96, 0, 0, 0, 2, 2,
        5, 5, 0, 0, 7, 0, 0, 0xFF,
        0x400, 0x100, 0, 0, 0x2FC, 0x17C, 2, 0x20B7,
    ) + (0,) * 27
    if record.key_words != expected_key:
        raise falsify("water support run 41 key no longer matches native stage")
    if record.output_sha256 != (
        "d3d79a86ca205e2b5c79576d68ed2c75656f8b617c9056b0aba1eac3b2e74ab0"
    ):
        raise falsify(
            "water support run 41 output changed: "
            f"{record.output_sha256}"
        )
    return record


def build_water_records(repo_root: Path) -> list[PreparedRecord]:
    """Build the two exact source-initial TEXEL0/TEXEL1 water keys."""
    source = water.load_source_corpus(repo_root)
    expected_keys = (
        water.WaterKey(0, 0, 1, 769, 0, 1277, 508,
                       769, 0, 1277, 508, 114),
        water.WaterKey(1, 0, 1, 205, 0, 329, 252,
                       205, 0, 329, 252, 114),
    )
    expected_hashes = (
        "f3a908659547f360ec9d3b79f80aa4c5dca829cdb36975a5d3a59667d1fdf532",
        "61b0bb44aa30033d0c8e07d924f6b38ddbafa23807692eb16aab194e57457efe",
    )
    records: list[PreparedRecord] = []
    for owner, spec in enumerate(source.specs):
        state = water.MaterialState(source.bank104.payload, spec)
        state.tick()
        frame_key = water.make_key(spec, state)
        if frame_key != expected_keys[owner]:
            raise falsify(f"{spec.name}: source-initial water key changed")
        pixels = water.render_reference(source, spec, frame_key)
        if sha256(pixels) != expected_hashes[owner]:
            raise falsify(f"{spec.name}: source-initial water output changed")

        render_w1 = 0x00094350 if owner == water.OWNER_LARGE else 0x00094250
        scroll_w1 = 0x01094250
        render_tile = TileState(
            set_seen=True, size_seen=True, format=FMT_CI, size=SIZ_4B,
            line=2, tmem=0, palette=0,
            cmt=(render_w1 >> 18) & 3,
            maskt=(render_w1 >> 14) & 15,
            shiftt=(render_w1 >> 10) & 15,
            cms=(render_w1 >> 8) & 3,
            masks=(render_w1 >> 4) & 15,
            shifts=render_w1 & 15,
            uls=frame_key.tile0_uls, ult=frame_key.tile0_ult,
            lrs=frame_key.tile0_lrs, lrt=frame_key.tile0_lrt,
            width=spec.width, height=spec.height,
        )
        scroll_tile = TileState(
            set_seen=True, size_seen=True, format=FMT_CI, size=SIZ_4B,
            line=2, tmem=64, palette=0,
            cmt=(scroll_w1 >> 18) & 3,
            maskt=(scroll_w1 >> 14) & 15,
            shiftt=(scroll_w1 >> 10) & 15,
            cms=(scroll_w1 >> 8) & 3,
            masks=(scroll_w1 >> 4) & 15,
            shifts=scroll_w1 & 15,
            uls=frame_key.tile1_uls, ult=frame_key.tile1_ult,
            lrs=frame_key.tile1_lrs, lrt=frame_key.tile1_lrt,
            width=spec.width, height=spec.height,
        )
        fields = {name: 0 for name in census.EXPECTED_KEY_FIELDS}
        fields.update(
            image=water.TEXTURE_OFFSETS[frame_key.texture0],
            image_format=FMT_CI, image_size=SIZ_16B, image_width=1,
            tlut_image=water.PALETTE_OFFSET, tlut_count=16,
            data_layout=DATA_LAYOUT_O2R_WORD_SWAPPED,
            format=FMT_CI, size=SIZ_4B,
            width=spec.width, height=spec.height,
            render_tile=0, render_tmem=0, render_palette=0,
            render_tile_cms=render_tile.cms,
            render_tile_cmt=render_tile.cmt,
            render_tile_masks=render_tile.masks,
            render_tile_maskt=render_tile.maskt,
            render_tile_shifts=render_tile.shifts,
            render_tile_shiftt=render_tile.shiftt,
            load_tile=7, load_uls=0, load_ult=0,
            load_lrs=255, load_dxt=1024, load_texels=256,
            tile_uls=render_tile.uls, tile_ult=render_tile.ult,
            tile_lrs=render_tile.lrs, tile_lrt=render_tile.lrt,
            line=render_tile.line,
            flags=(TILE_RENDER_SEEN | TILE_LOAD_SEEN |
                   tile_flags(render_tile) | (LOAD_KIND_BLOCK << 8)),
            texel1_image=water.TEXTURE_OFFSETS[frame_key.texture1],
            texel1_image_format=FMT_CI,
            texel1_image_size=SIZ_16B, texel1_image_width=1,
            texel1_load_kind=LOAD_KIND_BLOCK,
            texel1_render_tmem=scroll_tile.tmem,
            texel1_render_line=scroll_tile.line,
            texel1_render_palette=scroll_tile.palette,
            texel1_render_tile_cms=scroll_tile.cms,
            texel1_render_tile_cmt=scroll_tile.cmt,
            texel1_render_tile_masks=scroll_tile.masks,
            texel1_render_tile_maskt=scroll_tile.maskt,
            texel1_render_tile_shifts=scroll_tile.shifts,
            texel1_render_tile_shiftt=scroll_tile.shiftt,
            texel1_load_tile=6, texel1_load_uls=0, texel1_load_ult=0,
            texel1_load_lrs=255, texel1_load_dxt=1024,
            texel1_load_texels=256,
            texel1_tile_uls=scroll_tile.uls,
            texel1_tile_ult=scroll_tile.ult,
            texel1_tile_lrs=scroll_tile.lrs,
            texel1_tile_lrt=scroll_tile.lrt,
            prim_lod_fraction=frame_key.fraction,
            combine_w0=water.COMBINE_W0,
            combine_w1=water.COMBINE_W1,
        )
        words = tuple(int(fields[name]) for name in census.EXPECTED_KEY_FIELDS)
        canonical_key = {
            "image_asset_id": source.bank103.file_id,
            "tlut_asset_id": source.bank103.file_id,
            "texel1_asset_id": source.bank103.file_id,
            "key_words": words,
        }
        records.append(
            PreparedRecord(
                owner_mask=1 << 2,
                image=census.PointerRef(source.bank103.file_id, words[0]),
                tlut_image=census.PointerRef(source.bank103.file_id, words[4]),
                source_block=census.PointerRef(source.bank103.file_id, words[0]),
                key_words=words,
                logical_width=spec.width, logical_height=spec.height,
                upload_width=spec.width, upload_height=spec.height,
                pixels=pixels,
                key_sha256=sha256(json.dumps(
                    canonical_key, sort_keys=True, separators=(",", ":")
                ).encode("ascii")),
                sites={0x22C8 if owner == water.OWNER_LARGE else 0x2380},
                output_sha256=sha256(pixels),
            )
        )
    return records


def build_runtime_qualified_whispy_record(
    repo_root: Path, blocks: Sequence[dict[str, object]]
) -> PreparedRecord:
    """Build the exact Whispy-mouth key captured in the lifecycle.

    The complete 59-word key was captured at profile frame 699 in stage display
    list 0x1630. Pointer provenance resolved it to actor asset 152. Running the
    capture through the normal conversion and slow oracle binds the retained
    entry to source texture 0x0c20 and palette 0x0998.
    """
    actors = census.load_o2r(repo_root, census.O2R_INPUTS["stage_actors"])
    state = DisplayState(
        tlut_image=census.PointerRef(actors.file_id, 0x0998),
        tlut_count=16,
        texture_seen=True,
        texture_on=True,
        texture_tile=0,
    )
    state.tiles[0] = TileState(
        set_seen=True,
        size_seen=True,
        format=FMT_CI,
        size=SIZ_4B,
        line=1,
        tmem=0,
        palette=0,
        cmt=2,
        maskt=5,
        shiftt=0,
        cms=2,
        masks=4,
        shifts=0,
        uls=0,
        ult=0,
        lrs=0x03C,
        lrt=0x07C,
        width=16,
        height=32,
    )
    state.tiles[LOAD_TILE] = TileState(set_seen=True)
    state.loads = [
        LoadState(
            image=census.PointerRef(actors.file_id, 0x0C20),
            image_format=FMT_CI,
            image_size=SIZ_16B,
            image_width=1,
            load_kind=LOAD_KIND_BLOCK,
            load_tile=LOAD_TILE,
            load_uls=0,
            load_ult=0,
            load_lrs=0x07F,
            load_dxt=0x800,
            load_texels=0x080,
            load_tmem=0,
        )
    ]
    record = capture_record(
        WHISPY_MOUTH_OWNER_MASK, 0x16B0, state, actors, blocks
    )
    expected_key = (
        0x00000C20, 0x00000002, 0x00000002, 0x00000001,
        0x00000998, 0x00000010, 0x00000001, 0x00000002,
        0x00000000, 0x00000010, 0x00000020, 0x00000000,
        0x00000000, 0x00000000, 0x00000002, 0x00000002,
        0x00000004, 0x00000005, 0x00000000, 0x00000000,
        0x00000007, 0x00000000, 0x00000000, 0x0000007F,
        0x00000800, 0x00000080, 0x00000000, 0x00000000,
        0x0000003C, 0x0000007C, 0x00000001, 0x000020B7,
    ) + (0,) * 27
    if record.key_words != expected_key:
        raise falsify("runtime-qualified Whispy key no longer matches its capture")
    if record.output_sha256 != (
        "f66ad1991b31c42e215f2f0c1cf90c6873d53082732603572e07d632286d2aaa"
    ):
        raise falsify("runtime-qualified Whispy output changed")
    if (
        record.image != census.PointerRef(152, 0x0C20)
        or record.tlut_image != census.PointerRef(152, 0x0998)
        or record.logical_width != 16
        or record.logical_height != 32
        or record.upload_width != 16
        or record.upload_height != 32
        or len(record.pixels) != 1024
    ):
        raise falsify(
            "runtime-qualified Whispy record geometry or provenance changed"
        )
    return record


def build_runtime_qualified_whispy_initial_mouth_record(
    repo_root: Path, blocks: Sequence[dict[str, object]]
) -> PreparedRecord:
    """Build Whispy's source-initial mouth texture used by native run 27.

    BattleShip's StagePupupuFile3 display list 0x1558 renders the initial
    Whispy-mouth actor with CI4 texture 0x0e80 and palette 0x0e58.  The native
    stage preflight reaches this run before generic rendering has had any chance
    to upload it, so this exact source-authored frame must be resident before GO.
    This is distinct from the later lifecycle mouth frame at 0x0c20 retained by
    build_runtime_qualified_whispy_record().
    """
    actors = census.load_o2r(repo_root, census.O2R_INPUTS["stage_actors"])
    state = DisplayState(
        tlut_image=census.PointerRef(actors.file_id, 0x0E58),
        tlut_count=16,
        texture_seen=True,
        texture_on=True,
        texture_tile=0,
    )
    state.tiles[0] = TileState(
        set_seen=True,
        size_seen=True,
        format=FMT_CI,
        size=SIZ_4B,
        line=1,
        tmem=0,
        palette=0,
        cmt=2,
        maskt=4,
        shiftt=0,
        cms=2,
        masks=4,
        shifts=0,
        uls=0,
        ult=0,
        lrs=0x03C,
        lrt=0x03C,
        width=16,
        height=16,
    )
    state.tiles[LOAD_TILE] = TileState(set_seen=True)
    state.loads = [
        LoadState(
            image=census.PointerRef(actors.file_id, 0x0E80),
            image_format=FMT_CI,
            image_size=SIZ_16B,
            image_width=1,
            load_kind=LOAD_KIND_BLOCK,
            load_tile=LOAD_TILE,
            load_uls=0,
            load_ult=0,
            load_lrs=0x03F,
            load_dxt=0x800,
            load_texels=0x040,
            load_tmem=0,
        )
    ]
    record = capture_record(
        WHISPY_MOUTH_OWNER_MASK, 0x1558, state, actors, blocks
    )
    expected_key = (
        0x00000E80, 0x00000002, 0x00000002, 0x00000001,
        0x00000E58, 0x00000010, 0x00000001, 0x00000002,
        0x00000000, 0x00000010, 0x00000010, 0x00000000,
        0x00000000, 0x00000000, 0x00000002, 0x00000002,
        0x00000004, 0x00000004, 0x00000000, 0x00000000,
        0x00000007, 0x00000000, 0x00000000, 0x0000003F,
        0x00000800, 0x00000040, 0x00000000, 0x00000000,
        0x0000003C, 0x0000003C, 0x00000001, 0x000020B7,
    ) + (0,) * 27
    if record.key_words != expected_key:
        raise falsify(
            "source-initial Whispy mouth key no longer matches native run 27"
        )
    if record.output_sha256 != (
        "dcece1740ae02304716a5997d3e37195e1b68390fcaaea878f8d1367bde61687"
    ):
        raise falsify("source-initial Whispy mouth output changed")
    if (
        record.image != census.PointerRef(152, 0x0E80)
        or record.tlut_image != census.PointerRef(152, 0x0E58)
        or record.logical_width != 16
        or record.logical_height != 16
        or record.upload_width != 16
        or record.upload_height != 16
        or len(record.pixels) != 512
    ):
        raise falsify(
            "source-initial Whispy mouth geometry or provenance changed"
        )
    return record


def build_runtime_qualified_whispy_initial_mouth_material_record(
    repo_root: Path, blocks: Sequence[dict[str, object]]
) -> PreparedRecord:
    """Build the source-initial animated Whispy-mouth material key.

    BattleShip's MObj at 0x13d8 begins with texture_id_curr == 0, so the
    0x13c8 sprite table selects texture 0x09c0. Native run 28 consumes that
    material with the mouth's 16x32 tile geometry before generic rendering has
    uploaded anything. This key intentionally coexists with the 32x32 Whispy
    eye use of the same source image and the later 0x0c20 mouth frame.
    """
    actors = census.load_o2r(repo_root, census.O2R_INPUTS["stage_actors"])
    state = DisplayState(
        tlut_image=census.PointerRef(actors.file_id, 0x0998),
        tlut_count=16,
        texture_seen=True,
        texture_on=True,
        texture_tile=0,
    )
    state.tiles[0] = TileState(
        set_seen=True,
        size_seen=True,
        format=FMT_CI,
        size=SIZ_4B,
        line=1,
        tmem=0,
        palette=0,
        cmt=2,
        maskt=5,
        shiftt=0,
        cms=2,
        masks=4,
        shifts=0,
        uls=0,
        ult=0,
        lrs=0x03C,
        lrt=0x07C,
        width=16,
        height=32,
    )
    state.tiles[LOAD_TILE] = TileState(set_seen=True)
    state.loads = [
        LoadState(
            image=census.PointerRef(actors.file_id, 0x09C0),
            image_format=FMT_CI,
            image_size=SIZ_16B,
            image_width=1,
            load_kind=LOAD_KIND_BLOCK,
            load_tile=LOAD_TILE,
            load_uls=0,
            load_ult=0,
            load_lrs=0x07F,
            load_dxt=0x800,
            load_texels=0x080,
            load_tmem=0,
        )
    ]
    record = capture_record(
        WHISPY_MOUTH_OWNER_MASK, 0x16B0, state, actors, blocks
    )
    expected_key = (
        0x000009C0, 0x00000002, 0x00000002, 0x00000001,
        0x00000998, 0x00000010, 0x00000001, 0x00000002,
        0x00000000, 0x00000010, 0x00000020, 0x00000000,
        0x00000000, 0x00000000, 0x00000002, 0x00000002,
        0x00000004, 0x00000005, 0x00000000, 0x00000000,
        0x00000007, 0x00000000, 0x00000000, 0x0000007F,
        0x00000800, 0x00000080, 0x00000000, 0x00000000,
        0x0000003C, 0x0000007C, 0x00000001, 0x000020B7,
    ) + (0,) * 27
    if record.key_words != expected_key:
        raise falsify(
            "source-initial Whispy mouth material key no longer matches run 28"
        )
    if record.output_sha256 != (
        "7694587e23982dd2d50e6046c793506c0d16ad2fa32bfda5c19bde79029023ab"
    ):
        raise falsify("source-initial Whispy mouth material output changed")
    if (
        record.image != census.PointerRef(152, 0x09C0)
        or record.tlut_image != census.PointerRef(152, 0x0998)
        or record.logical_width != 16
        or record.logical_height != 32
        or record.upload_width != 16
        or record.upload_height != 32
        or len(record.pixels) != 1024
    ):
        raise falsify(
            "source-initial Whispy mouth material geometry or provenance changed"
        )
    return record


def build_runtime_qualified_whispy_direct_mouth_record(
    repo_root: Path, blocks: Sequence[dict[str, object]]
) -> PreparedRecord:
    """Build Whispy's direct source mouth texture consumed by run 29.

    StagePupupuFile3 display list 0x16c0 keeps the 16x32 mouth tile established
    by the preceding MObj-backed run, then directly loads CI4 palette 0x0868 and
    texture 0x0890. The source census independently identifies 0x0890 as one of
    the mouth actor's two direct texture loads, so this is source provenance,
    not a runtime-address-derived guess.
    """
    actors = census.load_o2r(repo_root, census.O2R_INPUTS["stage_actors"])
    state = DisplayState(
        tlut_image=census.PointerRef(actors.file_id, 0x0868),
        tlut_count=16,
        texture_seen=True,
        texture_on=True,
        texture_tile=0,
    )
    state.tiles[0] = TileState(
        set_seen=True,
        size_seen=True,
        format=FMT_CI,
        size=SIZ_4B,
        line=1,
        tmem=0,
        palette=0,
        cmt=2,
        maskt=5,
        shiftt=0,
        cms=2,
        masks=4,
        shifts=0,
        uls=0,
        ult=0,
        lrs=0x03C,
        lrt=0x07C,
        width=16,
        height=32,
    )
    state.tiles[LOAD_TILE] = TileState(set_seen=True)
    state.loads = [
        LoadState(
            image=census.PointerRef(actors.file_id, 0x0890),
            image_format=FMT_CI,
            image_size=SIZ_16B,
            image_width=1,
            load_kind=LOAD_KIND_BLOCK,
            load_tile=LOAD_TILE,
            load_uls=0,
            load_ult=0,
            load_lrs=0x07F,
            load_dxt=0x800,
            load_texels=0x080,
            load_tmem=0,
        )
    ]
    record = capture_record(
        WHISPY_MOUTH_OWNER_MASK, 0x16C0, state, actors, blocks
    )
    expected_key = (
        0x00000890, 0x00000002, 0x00000002, 0x00000001,
        0x00000868, 0x00000010, 0x00000001, 0x00000002,
        0x00000000, 0x00000010, 0x00000020, 0x00000000,
        0x00000000, 0x00000000, 0x00000002, 0x00000002,
        0x00000004, 0x00000005, 0x00000000, 0x00000000,
        0x00000007, 0x00000000, 0x00000000, 0x0000007F,
        0x00000800, 0x00000080, 0x00000000, 0x00000000,
        0x0000003C, 0x0000007C, 0x00000001, 0x000020B7,
    ) + (0,) * 27
    if record.key_words != expected_key:
        raise falsify("direct Whispy mouth key no longer matches native run 29")
    if record.output_sha256 != (
        "0ca5f33a89f6a5bcd121c44e33d94d62b667938840b5353fa8a19d292d74fae0"
    ):
        raise falsify("direct Whispy mouth output changed")
    if (
        record.image != census.PointerRef(152, 0x0890)
        or record.tlut_image != census.PointerRef(152, 0x0868)
        or record.logical_width != 16
        or record.logical_height != 32
        or record.upload_width != 16
        or record.upload_height != 32
        or len(record.pixels) != 1024
    ):
        raise falsify("direct Whispy mouth geometry or provenance changed")
    return record


def build_runtime_qualified_flower_records(
    repo_root: Path, blocks: Sequence[dict[str, object]]
) -> list[PreparedRecord]:
    """Build the two source-authored Dream Land flower textures.

    BattleShip declares the back and front flower owners over the same two CI4
    images. Native runs 32 and 34 establish three distinct first-frame keys. The
    0x0030 draws at 128x32; the two 0x0460 owner states draw at 128x32 and
    64x32. The old hand-qualified 16x32 state was never touched by the live
    stage, while native run 34 requested the missing 64x32 key. Keeping all
    three source-observed states resident before GO lets both flower owner
    groups preflight without a generic warm-up.
    """
    actors = census.load_o2r(repo_root, census.O2R_INPUTS["stage_actors"])
    records: list[PreparedRecord] = []
    owner_mask = FLOWERS_BACK_OWNER_MASK | FLOWERS_FRONT_OWNER_MASK
    specs = (
        (
            0x0030,
            0x0008,
            0x28B0,
            128,
            4,
            6,
            0x1FC,
            "a978fd14ee3bf428d902af1afb8f2eea243556ae8d70d753ea55aeb3db958a4e",
        ),
        (
            0x0030,
            0x0008,
            0x2EB0,
            64,
            4,
            6,
            0x3FC,
            "785156b328cc4d2f5de17afaddfa9d682356848c5c1eb6be56a95f909c4b1a03",
        ),
        (
            0x0030,
            0x0008,
            0x3028,
            64,
            4,
            6,
            0x0FC,
            "785156b328cc4d2f5de17afaddfa9d682356848c5c1eb6be56a95f909c4b1a03",
        ),
        (
            0x0460,
            0x0438,
            0x29A8,
            128,
            4,
            6,
            0x1FC,
            "878fdedf9528b946348527ea2497ecfb78a59ff01c1d9ace7303a90f675794f0",
        ),
        (
            0x0460,
            0x0438,
            0x29A8,
            64,
            4,
            6,
            0x2FC,
            "1bfc9740c7a1f5d0110791534adb16f00712bfd7d996fb06e874e998f0dfb74b",
        ),
    )
    for (
        image_offset,
        palette_offset,
        command_offset,
        logical_width,
        line,
        masks,
        tile_lrs,
        output_sha,
    ) in specs:
        state = DisplayState(
            tlut_image=census.PointerRef(actors.file_id, palette_offset),
            tlut_count=16,
            texture_seen=True,
            texture_on=True,
            texture_tile=0,
        )
        state.tiles[0] = TileState(
            set_seen=True,
            size_seen=True,
            format=FMT_CI,
            size=SIZ_4B,
            line=line,
            tmem=0,
            palette=0,
            cmt=2,
            maskt=5,
            shiftt=0,
            cms=2,
            masks=masks,
            shifts=0,
            uls=0,
            ult=0,
            lrs=tile_lrs,
            lrt=0x07C,
            width=logical_width,
            height=32,
        )
        state.tiles[LOAD_TILE] = TileState(set_seen=True)
        state.loads = [
            LoadState(
                image=census.PointerRef(actors.file_id, image_offset),
                image_format=FMT_CI,
                image_size=SIZ_16B,
                image_width=1,
                load_kind=LOAD_KIND_BLOCK,
                load_tile=LOAD_TILE,
                load_uls=0,
                load_ult=0,
                load_lrs=0x1FF,
                load_dxt=0x200,
                load_texels=0x200,
                load_tmem=0,
            )
        ]
        record = capture_record(
            owner_mask, command_offset, state, actors, blocks
        )
        expected_key = (
            image_offset, 0x00000002, 0x00000002, 0x00000001,
            palette_offset, 0x00000010, 0x00000001, 0x00000002,
            0x00000000, logical_width, 0x00000020, 0x00000000,
            0x00000000, 0x00000000, 0x00000002, 0x00000002,
            masks, 0x00000005, 0x00000000, 0x00000000,
            0x00000007, 0x00000000, 0x00000000, 0x000001FF,
            0x00000200, 0x00000200, 0x00000000, 0x00000000,
            tile_lrs, 0x0000007C, line, 0x000020B7,
        ) + (0,) * 27
        if record.key_words != expected_key:
            raise falsify(
                f"flower 0x{image_offset:04x} key no longer matches native stage"
            )
        if record.output_sha256 != output_sha:
            raise falsify(
                f"flower 0x{image_offset:04x} output changed: "
                f"{record.output_sha256} != {output_sha}"
            )
        if (
            record.image != census.PointerRef(152, image_offset)
            or record.tlut_image != census.PointerRef(152, palette_offset)
            or record.logical_width != logical_width
            or record.logical_height != 32
            or record.upload_width != logical_width
            or record.upload_height != 32
            or len(record.pixels) != logical_width * 32 * 2
        ):
            raise falsify(
                f"flower 0x{image_offset:04x} geometry or provenance changed"
            )
        records.append(record)
    return records


def build_runtime_qualified_whispy_eye_records(
    repo_root: Path, blocks: Sequence[dict[str, object]]
) -> list[PreparedRecord]:
    """Build both source-authored Whispy eye texture frames.

    BattleShip's StagePupupuFile3 MObj at 0x0f18 names exactly two sprites,
    0x09c0 and 0x0af0, with the shared CI4 palette at 0x0998.  Native-stage
    run 26 consumes this MObj.  A live route capture proved the complete
    59-word renderer key at the first frame is the 32x32 key below; the second
    source frame differs only in the image pointer.  Keeping both frames in the
    pre-GO static set preserves the source material animation while allowing
    native-stage preflight to remain allocation-free.
    """
    actors = census.load_o2r(repo_root, census.O2R_INPUTS["stage_actors"])
    state = DisplayState(
        tlut_image=census.PointerRef(actors.file_id, 0x0998),
        tlut_count=16,
        texture_seen=True,
        texture_on=True,
        texture_tile=0,
    )
    state.tiles[0] = TileState(
        set_seen=True,
        size_seen=True,
        format=FMT_CI,
        size=SIZ_4B,
        line=1,
        tmem=0,
        palette=0,
        cmt=2,
        maskt=5,
        shiftt=0,
        cms=2,
        masks=4,
        shifts=0,
        uls=0,
        ult=0,
        lrs=0x07C,
        lrt=0x07C,
        width=32,
        height=32,
    )
    state.tiles[LOAD_TILE] = TileState(set_seen=True)
    records: list[PreparedRecord] = []
    expected_outputs = {
        0x09C0: "8382c5c2803d7807eac31a3757ed394241910f10eeb77cdab442d9a746bf80fa",
        0x0AF0: "4ccce0bcf49f97bdfe3fff2b14347d8a747c26f6ca5d009012ecd74f64e02d26",
    }
    for image_offset in (0x09C0, 0x0AF0):
        state.loads = [
            LoadState(
                image=census.PointerRef(actors.file_id, image_offset),
                image_format=FMT_CI,
                image_size=SIZ_16B,
                image_width=1,
                load_kind=LOAD_KIND_BLOCK,
                load_tile=LOAD_TILE,
                load_uls=0,
                load_ult=0,
                load_lrs=0x07F,
                load_dxt=0x800,
                load_texels=0x080,
                load_tmem=0,
            )
        ]
        record = capture_record(
            WHISPY_EYES_OWNER_MASK, 0x0FF8, state, actors, blocks
        )
        expected_key = (
            image_offset, 0x00000002, 0x00000002, 0x00000001,
            0x00000998, 0x00000010, 0x00000001, 0x00000002,
            0x00000000, 0x00000020, 0x00000020, 0x00000000,
            0x00000000, 0x00000000, 0x00000002, 0x00000002,
            0x00000004, 0x00000005, 0x00000000, 0x00000000,
            0x00000007, 0x00000000, 0x00000000, 0x0000007F,
            0x00000800, 0x00000080, 0x00000000, 0x00000000,
            0x0000007C, 0x0000007C, 0x00000001, 0x000020B7,
        ) + (0,) * 27
        if record.key_words != expected_key:
            raise falsify(
                f"runtime-qualified Whispy eye 0x{image_offset:04x} key "
                "no longer matches its route capture"
            )
        if record.output_sha256 != expected_outputs[image_offset]:
            raise falsify(
                f"runtime-qualified Whispy eye 0x{image_offset:04x} output changed"
            )
        if (
            record.image != census.PointerRef(152, image_offset)
            or record.tlut_image != census.PointerRef(152, 0x0998)
            or record.logical_width != 32
            or record.logical_height != 32
            or record.upload_width != 32
            or record.upload_height != 32
        ):
            raise falsify(
                f"runtime-qualified Whispy eye 0x{image_offset:04x} "
                "geometry or provenance changed"
            )
        records.append(record)
    return records


def build_runtime_qualified_fox_record(
    repo_root: Path, blocks: Sequence[dict[str, object]]
) -> PreparedRecord:
    """Build the exact late Fox key captured in the canonical lifecycle.

    The complete 59-word key was captured at profile frame 1111 in native
    fighter run 43. Pointer provenance resolved it to FoxModel asset 313.
    Reconstructing the record through the same conversion/oracle functions
    binds the capture to the pinned source block and palette rather than to a
    runtime address or copied pixels.
    """
    fox = census.load_o2r(repo_root, census.O2R_INPUTS["fox_model"])
    state = DisplayState(
        tlut_image=census.PointerRef(fox.file_id, 0x72D8),
        tlut_count=16,
        texture_seen=True,
        texture_on=True,
        texture_tile=0,
    )
    state.tiles[0] = TileState(
        set_seen=True,
        size_seen=True,
        format=FMT_CI,
        size=SIZ_4B,
        line=2,
        tmem=0,
        palette=0,
        cmt=2,
        maskt=5,
        shiftt=0,
        cms=3,
        masks=5,
        shifts=0,
        uls=0x07C,
        ult=0x133,
        lrs=0x178,
        lrt=0x1AF,
        width=64,
        height=32,
    )
    state.tiles[LOAD_TILE] = TileState(set_seen=True)
    state.loads = [
        LoadState(
            image=census.PointerRef(fox.file_id, 0x70D0),
            image_format=FMT_CI,
            image_size=SIZ_16B,
            image_width=1,
            load_kind=LOAD_KIND_BLOCK,
            load_tile=LOAD_TILE,
            load_uls=0,
            load_ult=0,
            load_lrs=0x0FF,
            load_dxt=0x400,
            load_texels=0x100,
            load_tmem=0,
        )
    ]
    record = capture_record(
        FOX_LATE_MATERIAL_OWNER_MASK, 43, state, fox, blocks
    )
    expected_key = (
        0x000070D0, 0x00000002, 0x00000002, 0x00000001,
        0x000072D8, 0x00000010, 0x00000001, 0x00000002,
        0x00000000, 0x00000040, 0x00000020, 0x00000000,
        0x00000000, 0x00000000, 0x00000003, 0x00000002,
        0x00000005, 0x00000005, 0x00000000, 0x00000000,
        0x00000007, 0x00000000, 0x00000000, 0x000000FF,
        0x00000400, 0x00000100, 0x0000007C, 0x00000133,
        0x00000178, 0x000001AF, 0x00000002, 0x000020BF,
    ) + (0,) * 27
    if record.key_words != expected_key:
        raise falsify("runtime-qualified Fox key no longer matches its capture")
    if record.output_sha256 != (
        "332934f7f039f915965adf856a01e34297f02d48127a9e21207c05163a5f1598"
    ):
        raise falsify("runtime-qualified Fox output changed")
    if (
        record.image != census.PointerRef(313, 0x70D0)
        or record.tlut_image != census.PointerRef(313, 0x72D8)
        or record.logical_width != 64
        or record.logical_height != 32
        or record.upload_width != 64
        or record.upload_height != 32
        or len(record.pixels) != 4096
    ):
        raise falsify("runtime-qualified Fox record geometry or provenance changed")
    return record


def walk_display_list(
    resource: census.O2RResource,
    images: census.O2RResource,
    start: int,
    owner_mask: int,
    state: DisplayState,
    blocks: Sequence[dict[str, object]],
    records: dict[str, PreparedRecord],
    stack: tuple[int, ...] = (),
) -> None:
    if start in stack:
        raise falsify(f"recursive static display list at 0x{start:x}")
    if start < 0 or start + 8 > len(resource.payload):
        raise falsify(f"static display list 0x{start:x} is out of range")
    pc = start
    stack += (start,)
    for _ in range(4096):
        w0, w1 = struct.unpack_from(">II", resource.payload, pc)
        op = w0 >> 24
        if op == G_SETTIMG:
            ref = resource.pointer_at(pc + 4)
            if ref is None:
                raise falsify(f"unresolved static SETTIMG at 0x{pc:x}")
            state.image = ref
            state.image_format = (w0 >> 21) & 7
            state.image_size = (w0 >> 19) & 3
            state.image_width = (w0 & 0xFFF) + 1
        elif op == G_SETTILE:
            index = (w1 >> 24) & 7
            tile = state.tiles[index]
            tile.set_seen = True
            tile.format = (w0 >> 21) & 7
            tile.size = (w0 >> 19) & 3
            tile.line = (w0 >> 9) & 0x1FF
            tile.tmem = w0 & 0x1FF
            tile.palette = (w1 >> 20) & 0xF
            tile.cmt = (w1 >> 18) & 3
            tile.maskt = (w1 >> 14) & 0xF
            tile.shiftt = (w1 >> 10) & 0xF
            tile.cms = (w1 >> 8) & 3
            tile.masks = (w1 >> 4) & 0xF
            tile.shifts = w1 & 0xF
        elif op == G_SETTILESIZE:
            index = (w1 >> 24) & 7
            tile = state.tiles[index]
            tile.size_seen = True
            tile.uls = (w0 >> 12) & 0xFFF
            tile.ult = w0 & 0xFFF
            tile.lrs = (w1 >> 12) & 0xFFF
            tile.lrt = w1 & 0xFFF
            tile.width = (
                ((tile.lrs - tile.uls) >> 2) + 1 if tile.lrs >= tile.uls else 0
            )
            tile.height = (
                ((tile.lrt - tile.ult) >> 2) + 1 if tile.lrt >= tile.ult else 0
            )
        elif op == G_LOADTLUT:
            if state.image is None:
                raise falsify(f"LOADTLUT without SETTIMG at 0x{pc:x}")
            state.tlut_image = state.image
            state.tlut_count = ((w1 >> 14) & 0x3FF) + 1
        elif op in (G_LOADBLOCK, G_LOADTILE):
            if state.image is None:
                raise falsify(f"texture load without SETTIMG at 0x{pc:x}")
            tile_index = (w1 >> 24) & 7
            tile = state.tiles[tile_index]
            if not tile.set_seen:
                raise falsify(f"texture load without SETTILE at 0x{pc:x}")
            uls = (w0 >> 12) & 0xFFF
            ult = w0 & 0xFFF
            lrs = (w1 >> 12) & 0xFFF
            dxt = w1 & 0xFFF
            if op == G_LOADBLOCK:
                load_texels = lrs + 1
                load_kind = LOAD_KIND_BLOCK
            else:
                width = ((lrs - uls) >> 2) + 1 if lrs >= uls else 0
                height = ((dxt - ult) >> 2) + 1 if dxt >= ult else 0
                load_texels = width * height
                load_kind = LOAD_KIND_TILE
            state.loads.append(
                LoadState(
                    image=state.image,
                    image_format=state.image_format,
                    image_size=state.image_size,
                    image_width=state.image_width,
                    load_kind=load_kind,
                    load_tile=tile_index,
                    load_uls=uls,
                    load_ult=ult,
                    load_lrs=lrs,
                    load_dxt=dxt,
                    load_texels=load_texels,
                    load_tmem=tile.tmem,
                )
            )
            state.loads = state.loads[-2:]
        elif op == G_TEXTURE:
            state.texture_seen = True
            state.texture_tile = (w0 >> 8) & 7
            state.texture_on = ((w0 >> 1) & 0x7F) != 0
        elif op in (G_TRI1, G_TRI2):
            if state.texture_on:
                record = capture_record(
                    owner_mask, pc, state, images, blocks
                )
                existing = records.get(record.key_sha256)
                if existing is None:
                    records[record.key_sha256] = record
                else:
                    if (
                        existing.key_words != record.key_words
                        or existing.image != record.image
                        or existing.tlut_image != record.tlut_image
                        or existing.pixels != record.pixels
                    ):
                        raise falsify(
                            f"canonical key hash collision {record.key_sha256}"
                        )
                    existing.owner_mask |= owner_mask
                    existing.sites.update(record.sites)
        elif op == G_DL:
            ref = resource.pointer_at(pc + 4)
            if ref is None or ref.asset_id != resource.file_id:
                raise falsify(f"unresolved/external static G_DL at 0x{pc:x}")
            walk_display_list(
                resource,
                images,
                ref.offset,
                owner_mask,
                state,
                blocks,
                records,
                stack,
            )
            if w0 & 0x10000:
                return
        elif op == G_ENDDL:
            return
        pc += 8
        if pc + 8 > len(resource.payload):
            raise falsify(f"unterminated static display list 0x{start:x}")
    raise falsify(f"static display-list guard expired at 0x{start:x}")


def metadata_payload(records: Sequence[PreparedRecord]) -> bytes:
    metadata = [
        {
            "owner_mask": record.owner_mask,
            "image": record.image.as_json(),
            "tlut_image": record.tlut_image.as_json(),
            "source_block": record.source_block.as_json(),
            "key_words": list(record.key_words),
            "logical_dimensions": [record.logical_width, record.logical_height],
            "upload_dimensions": [record.upload_width, record.upload_height],
            "payload_offset": record.payload_offset,
            "payload_bytes": record.payload_bytes,
            "output_sha256": record.output_sha256,
            "sites": sorted(record.sites),
        }
        for record in records
    ]
    return json.dumps(
        metadata, sort_keys=True, separators=(",", ":"), ensure_ascii=True
    ).encode("ascii")


def repack_paletted(pixels: bytes) -> tuple[int, bytes, tuple[int, ...]]:
    """RGB555+A1 -> DS 16-colour paletted, when the image has that few colours.

    LOSSLESS BY CONSTRUCTION AND CHECKED, not "close enough": the palette IS the
    set of colours already present, so every texel keeps the exact halfword it
    had. The N64 source for all of these is a CI4 tile whose sixteen-entry TLUT
    convert_fast already expanded; this puts the indices back.

    `record.pixels` stays the 16-bit form upstream of here, so the slow oracle
    cross-check, `output_sha256` and the dedupe key all keep comparing the same
    canonical image they always did. Only the packed representation changes.

    Index 0 is reserved for the transparent colour whenever the image has one,
    so the DS colour-0-transparent bit means what the alpha bit meant. An image
    with no transparent texel uses all sixteen entries and the bit stays off.
    """
    # ON. It was switched off for one day, 2026-08-03, because the runtime M4
    # residency prepare failed with it -- zero keys and zero bytes where the
    # harness wants 24 and the full corpus -- and the renderer then fell back to
    # ordinary texture resolution while the frame still looked right at 27.8 FPS.
    #
    # THE PREPARE WAS NOT BROKEN BY THIS. It asserted
    # `gNdsRendererBattleStaticTextureBankMask != 3u`, i.e. that the corpus
    # STRADDLES texture banks A and B. That was a restatement of the old size,
    # not a correctness property: at 136,192 bytes the span crosses A's 128 KiB
    # boundary and at 61,696 it does not, so making the corpus smaller was itself
    # the thing the check rejected. nds_renderer.c derives the expected mask from
    # preparedBytes now and the note there records the cost.
    #
    # What it buys, losslessly: 22 of these 24 textures are sixteen-colour CI4
    # sources that were stored expanded to two bytes a texel, so packing them
    # returns 74,496 bytes of the DS's 262,144 to the texture allocator. Same
    # pixels -- the palette is the set of colours already present, and
    # `output_sha256` still compares the canonical 16-bit image.
    # BIT 15 STOPS BEING ALPHA, WHICH IS THE WHOLE HAZARD IN THIS FUNCTION.
    #
    # The source is RGB555+A1: bit 15 is the alpha bit and GL_RGBA honours it per
    # texel. GL_RGB16 does not have per-entry alpha at all -- the only
    # transparency it has is GL_TEXTURE_COLOR0_TRANSPARENT, which applies to
    # index 0 and nothing else. So "every texel keeps the exact halfword it had"
    # is true of the BITS and false of the PICTURE: a transparent texel whose
    # colour bits are non-zero (which is most of them -- an invisible texel's
    # RGB is arbitrary) became an ordinary palette entry and drew SOLID.
    #
    # Measured 2026-08-03: with this uncorrected, the Dream Land top screen went
    # to 0 of 49,152 dominant-green pixels while every byte-level check passed,
    # because the generator's oracle compares the canonical 16-bit image and
    # never the repacked representation.
    #
    # Normalising every transparent texel to 0x0000 fixes it and is exact: alpha
    # 0 means the colour is not observable, so collapsing all of them onto one
    # index changes no visible texel. It also makes the runtime's existing
    # `palette[0] == 0` test correct rather than lucky -- 0x0000 has bit 15
    # clear, so it can never be an OPAQUE colour, which makes "entry 0 is zero"
    # and "index 0 is the transparent slot" the same statement.
    raw = struct.unpack(f"<{len(pixels) // 2}H", pixels)
    values = tuple(0 if (value & 0x8000) == 0 else value for value in raw)
    distinct = sorted(set(values))
    if len(distinct) > DS_PALETTE16_ENTRIES:
        return DS_FORMAT_RGBA, pixels, ()
    if 0 in distinct:
        distinct.remove(0)
        palette = (0,) + tuple(distinct)
    else:
        palette = tuple(distinct)
    index_of = {colour: index for index, colour in enumerate(palette)}
    packed = bytearray((len(values) + 1) // 2)
    for position, colour in enumerate(values):
        index = index_of[colour]
        if position & 1:
            packed[position >> 1] |= index << 4
        else:
            packed[position >> 1] = index
    # DECODE IT BACK AND COMPARE, because nothing else does. `output_sha256`,
    # the slow oracle and the dedupe key all compare `record.pixels`, which is
    # the canonical 16-bit image UPSTREAM of this function -- so before this
    # check the repacked representation was the one artifact in the corpus that
    # shipped unverified, and it shipped wrong.
    #
    # The comparison is what the DS will actually show: index 0 reads as
    # transparent when the image had any transparent texel, every other index
    # reads as its palette entry with bit 15 forced on (GL_RGB16 has no per-entry
    # alpha, so everything it draws is opaque).
    color0_transparent = palette[0] == 0
    for position, expected in enumerate(values):
        byte = packed[position >> 1]
        index = (byte >> 4) if (position & 1) else (byte & 0x0F)
        if index == 0 and color0_transparent:
            decoded = 0
        else:
            decoded = palette[index] | 0x8000
        want = expected if expected == 0 else (expected | 0x8000)
        if decoded != want:
            raise SystemExit(
                f"paletted repack is not lossless at texel {position}: "
                f"decoded 0x{decoded:04x}, expected 0x{want:04x}")
    return DS_FORMAT_PAL16, bytes(packed), palette


def pack_payload(records: Sequence[PreparedRecord]) -> tuple[bytes, int]:
    """Texels first, then every palette, in one payload.

    Two blocks rather than two files because the runtime already streams this
    payload as one span and a palette is thirty-two bytes; a second asset would
    cost a NitroFS open per texture for less than a kilobyte in total.
    """
    payload = bytearray()
    output_offsets: dict[bytes, tuple[int, int, int, tuple[int, ...]]] = {}
    for record in records:
        existing = output_offsets.get(record.pixels)
        if existing is None:
            ds_format, packed, palette = repack_paletted(record.pixels)
            existing = (len(payload), len(packed), ds_format, palette)
            output_offsets[record.pixels] = existing
            payload.extend(packed)
        record.payload_offset, record.payload_bytes, record.ds_format, _ = existing
        record.palette = existing[3]
    palette_offsets: dict[tuple[int, ...], int] = {}
    for record in records:
        if not record.palette:
            record.palette_offset = 0
            record.palette_entries = 0
            continue
        offset = palette_offsets.get(record.palette)
        if offset is None:
            offset = len(payload)
            palette_offsets[record.palette] = offset
            for colour in record.palette:
                payload.extend(struct.pack("<H", colour))
        record.palette_offset = offset
        record.palette_entries = len(record.palette)
    return bytes(payload), len(output_offsets)


def build_include(
    records: Sequence[PreparedRecord],
    payload: bytes,
    census_sha256: str,
    metadata_sha256: str,
) -> bytes:
    palette_offsets = [record.palette_offset for record in records
                       if record.palette_entries]
    palette_base = min(palette_offsets) if palette_offsets else len(payload)
    palette_block = len(payload) - palette_base
    lines = [
        "/* Generated by scripts/generate_battle_playable_static_textures.py. */",
        "/* Metadata only; pixels live in the generated NitroFS payload. */",
        "/* This offline static corpus does not prove M4 complete. */",
        f"/* Source census SHA256: {census_sha256}. */",
        f"/* Payload SHA256: {sha256(payload)}. */",
        f"/* Metadata SHA256: {metadata_sha256}. */",
        "",
        f"#define NDS_BATTLE_STATIC_TEXTURE_KEY_COUNT {len(records)}u",
        f"#define NDS_BATTLE_STATIC_TEXTURE_OUTPUT_COUNT {len({record.pixels for record in records})}u",
        f"#define NDS_BATTLE_STATIC_TEXTURE_PAYLOAD_BYTES {len(payload)}u",
        f"#define NDS_BATTLE_STATIC_TEXTURE_PREPARED_BYTES {sum(record.payload_bytes for record in records)}u",
        f"#define NDS_BATTLE_STATIC_TEXTURE_KEY_WORDS {len(census.EXPECTED_KEY_FIELDS)}u",
        "",
        "/* The palettes are one contiguous block at the tail of the payload, and",
        " * the renderer reads the WHOLE block in a single fseek+fread at prepare",
        " * time. Reading each texture's palette where its record points cost 22",
        " * extra NitroFS round trips inside a scene load that is already the",
        " * longest pause in the game -- enough to push it past the marker window",
        " * the harness allows. One read, then index by (palette_offset - base). */",
        f"#define NDS_BATTLE_STATIC_TEXTURE_PALETTE_BLOCK_OFFSET {palette_base}u",
        f"#define NDS_BATTLE_STATIC_TEXTURE_PALETTE_BLOCK_BYTES {palette_block}u",
        "",
        "static const NDSBattlePlayableStaticTextureRecord",
        "sNdsBattleStaticTextureRecords[NDS_BATTLE_STATIC_TEXTURE_KEY_COUNT] =",
        "{",
    ]
    for record in records:
        lines.extend(
            [
                f"    /* key {record.key_sha256}; output {record.output_sha256}. */",
                "    {",
                f"        0x{record.owner_mask:04x}u, {record.image.asset_id}u, "
                f"{record.tlut_image.asset_id}u, 0u,",
                f"        0x{record.image.offset:08x}u, 0x{record.tlut_image.offset:08x}u,",
                f"        {record.payload_offset}u, {record.payload_bytes}u, "
                f"{record.logical_width}u, {record.logical_height}u, "
                f"{record.upload_width}u, {record.upload_height}u,",
                f"        {record.ds_format}u, {record.palette_entries}u, "
                f"{record.palette_offset}u,",
                "        {",
            ]
        )
        for offset in range(0, len(record.key_words), 8):
            chunk = record.key_words[offset : offset + 8]
            lines.append(
                "            " + ", ".join(f"0x{word:08x}u" for word in chunk) + ","
            )
        lines.extend(["        }", "    },"])
    lines.extend(["};", ""])
    return ("\n".join(lines)).encode("ascii")


def validate_golden(artifacts: GeneratedArtifacts) -> None:
    summary = artifacts.summary()
    expected = {
        "key_count": EXPECTED_KEY_COUNT,
        "unique_output_count": EXPECTED_OUTPUT_COUNT,
        "residency_bytes": EXPECTED_RESIDENCY_BYTES,
        "payload_bytes": EXPECTED_PAYLOAD_BYTES,
        "oracle_pixels": EXPECTED_ORACLE_PIXELS,
        "payload_sha256": EXPECTED_PAYLOAD_SHA256,
        "metadata_sha256": EXPECTED_METADATA_SHA256,
        "include_sha256": EXPECTED_INCLUDE_SHA256,
    }
    if "TO_BE_FILLED" in (
        EXPECTED_PAYLOAD_SHA256,
        EXPECTED_METADATA_SHA256,
        EXPECTED_INCLUDE_SHA256,
    ):
        raise falsify(f"static texture golden values are unpinned: {summary}")
    for name, wanted in expected.items():
        if summary[name] != wanted:
            raise falsify(f"static texture {name} {summary[name]} != pinned {wanted}")


def generate(repo_root: Path) -> GeneratedArtifacts:
    repo_root = repo_root.resolve()
    manifest = census.build_manifest(repo_root)
    census_sha256 = census.check_manifest(manifest)
    static = manifest["static_owners"]
    if not isinstance(static, dict):
        raise falsify("source census lost static_owners")
    blocks = static["source_blocks"]
    if not isinstance(blocks, list) or len(blocks) != 16:
        raise falsify("source census no longer has exactly 16 static blocks")
    geometry = census.load_o2r(repo_root, census.O2R_INPUTS["stage_geometry"])
    images = census.load_o2r(repo_root, census.O2R_INPUTS["stage_images"])
    records_by_hash: dict[str, PreparedRecord] = {}
    for owner_name, root_offset, descriptor_count, owner_mask in OWNER_SPECS:
        state = DisplayState()
        roots = census.dobj_display_lists(geometry, root_offset, descriptor_count)
        if not roots:
            raise falsify(f"{owner_name}: no display-list roots")
        for root in roots:
            walk_display_list(
                geometry,
                images,
                root,
                owner_mask,
                state,
                blocks,
                records_by_hash,
            )
    records = sorted(
        records_by_hash.values(),
        key=lambda record: (
            record.image.asset_id,
            record.image.offset,
            record.tlut_image.offset,
            record.key_words,
        ),
    )
    covered_blocks = {record.source_block.key() for record in records}
    expected_blocks = {
        str(block["identity"]["key"])
        for block in blocks
        if isinstance(block, dict) and isinstance(block.get("identity"), dict)
    }
    if covered_blocks != expected_blocks:
        raise falsify(
            "static prepared corpus does not exactly cover census blocks: "
            f"missing={sorted(expected_blocks - covered_blocks)} "
            f"extra={sorted(covered_blocks - expected_blocks)}"
        )
    owner_union = 0
    for record in records:
        owner_union |= record.owner_mask
    expected_owner_union = sum(mask for _name, _root, _count, mask in OWNER_SPECS)
    if owner_union != expected_owner_union:
        raise falsify(
            f"static owner mask 0x{owner_union:x} != 0x{expected_owner_union:x}"
        )
    records.extend(build_water_records(repo_root))
    water_manifest = manifest["water"]
    if not isinstance(water_manifest, dict):
        raise falsify("source census lost water ownership")
    water_blocks = water_manifest["source_blocks"]
    if not isinstance(water_blocks, list):
        raise falsify("source census lost water source blocks")
    records.append(
        build_runtime_qualified_water_support_record(repo_root, water_blocks)
    )
    dynamic = manifest["dynamic_animated_owners"]
    if not isinstance(dynamic, dict):
        raise falsify("source census lost dynamic_animated_owners")
    dynamic_blocks = dynamic["source_blocks"]
    if not isinstance(dynamic_blocks, list):
        raise falsify("source census lost dynamic source blocks")
    records.append(
        build_runtime_qualified_whispy_record(repo_root, dynamic_blocks)
    )
    records.append(
        build_runtime_qualified_whispy_initial_mouth_record(
            repo_root, dynamic_blocks
        )
    )
    records.append(
        build_runtime_qualified_whispy_initial_mouth_material_record(
            repo_root, dynamic_blocks
        )
    )
    records.append(
        build_runtime_qualified_whispy_direct_mouth_record(
            repo_root, dynamic_blocks
        )
    )
    records.extend(
        build_runtime_qualified_flower_records(repo_root, dynamic_blocks)
    )
    records.extend(
        build_runtime_qualified_whispy_eye_records(repo_root, dynamic_blocks)
    )
    records.append(build_runtime_qualified_fox_record(repo_root, dynamic_blocks))
    records.sort(
        key=lambda record: (
            record.image.asset_id,
            record.image.offset,
            record.tlut_image.offset,
            record.key_words,
        )
    )
    payload, output_count = pack_payload(records)
    metadata_sha256 = sha256(metadata_payload(records))
    generated_include = build_include(
        records, payload, census_sha256, metadata_sha256
    )
    # TEXEL bytes only. Palettes are resident too, but in VRAM F/G, and lumping
    # them in here would hide the one number this budget exists to track: what
    # the 262,144-byte texture banks are actually holding.
    residency_bytes = sum(record.payload_bytes for record in records)
    oracle_pixels = sum(record.logical_width * record.logical_height for record in records)
    return GeneratedArtifacts(
        include=generated_include,
        payload=payload,
        records=tuple(records),
        output_count=output_count,
        residency_bytes=residency_bytes,
        oracle_pixels=oracle_pixels,
        metadata_sha256=metadata_sha256,
        census_sha256=census_sha256,
    )


def summary_lines(artifacts: GeneratedArtifacts) -> Iterable[str]:
    summary = artifacts.summary()
    owners = summary["owners"]
    assert isinstance(owners, dict)
    yield (
        "BATTLE_PLAYABLE_STATIC_TEXTURES_OK "
        f"source_blocks={summary['source_block_count']} "
        f"keys={summary['key_count']} outputs={summary['unique_output_count']} "
        f"residency_bytes={summary['residency_bytes']} "
        f"payload_bytes={summary['payload_bytes']} "
        f"oracle_pixels={summary['oracle_pixels']}"
    )
    yield (
        "BATTLE_PLAYABLE_STATIC_TEXTURE_OWNERS "
        + " ".join(f"{name}={owners[name]}" for name, _mask in OWNER_LABELS)
    )
    yield (
        "BATTLE_PLAYABLE_STATIC_TEXTURE_DIGEST "
        f"payload_sha256={summary['payload_sha256']} "
        f"metadata_sha256={summary['metadata_sha256']} "
        f"include_sha256={summary['include_sha256']}"
    )
    yield (
        "BATTLE_PLAYABLE_STATIC_TEXTURE_QUALIFICATION "
        "renderer_integrated=NO runtime_keys_complete=NO "
        "device_zero_conversion=UNPROVEN m4_complete=NO"
    )


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
    )
    parser.add_argument(
        "--output",
        type=Path,
        help=f"generated include path; default {OUTPUT_RELATIVE.as_posix()}",
    )
    parser.add_argument(
        "--payload-output",
        type=Path,
        help=(
            "generated RGB5A1 payload path; default "
            f"{PAYLOAD_OUTPUT_RELATIVE.as_posix()}"
        ),
    )
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--fixture-json", action="store_true")
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    repo_root = args.repo_root.resolve()
    output = args.output or (repo_root / OUTPUT_RELATIVE)
    payload_output = args.payload_output or (repo_root / PAYLOAD_OUTPUT_RELATIVE)
    if not output.is_absolute():
        output = (repo_root / output).resolve()
    if not payload_output.is_absolute():
        payload_output = (repo_root / payload_output).resolve()
    try:
        artifacts = generate(repo_root)
        if args.check:
            validate_golden(artifacts)
            if not output.is_file():
                raise falsify(f"generated static texture include is absent: {output}")
            actual = output.read_bytes()
            if actual != artifacts.include:
                raise falsify(
                    f"generated static texture include is stale: {output}; "
                    f"actual_sha256={sha256(actual)} expected_sha256={sha256(artifacts.include)}"
                )
            if not payload_output.is_file():
                raise falsify(
                    f"generated static texture payload is absent: {payload_output}"
                )
            actual_payload = payload_output.read_bytes()
            if actual_payload != artifacts.payload:
                raise falsify(
                    f"generated static texture payload is stale: {payload_output}; "
                    f"actual_sha256={sha256(actual_payload)} "
                    f"expected_sha256={sha256(artifacts.payload)}"
                )
        else:
            output.parent.mkdir(parents=True, exist_ok=True)
            if not output.is_file() or output.read_bytes() != artifacts.include:
                output.write_bytes(artifacts.include)
            payload_output.parent.mkdir(parents=True, exist_ok=True)
            if (
                not payload_output.is_file()
                or payload_output.read_bytes() != artifacts.payload
            ):
                payload_output.write_bytes(artifacts.payload)
        if args.fixture_json:
            print(json.dumps(artifacts.summary(), indent=2, sort_keys=True))
        else:
            for line in summary_lines(artifacts):
                print(line)
        return 0
    except (Falsifier, census.Falsifier, OSError, ValueError, struct.error) as exc:
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
