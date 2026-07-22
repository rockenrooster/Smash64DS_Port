#!/usr/bin/env python3
"""Generate the canonical static Battle Playable DS texture corpus.

This is an offline M4 integration artifact, not a runtime implementation.  It
walks the pinned BattleShip Pupupu display lists for the three non-water stage
owners, reconstructs the exact primary texture keys consumed by the current DS
renderer, and converts their CI4/RGBA5551 inputs to padded little-endian DS
RGB5A1 bytes.  Exact metadata is emitted as C while pixels are emitted as one
NitroFS-ready binary.  Generated pixels are checked with a separate slow oracle.

The two source-initial Pupupu water composites are included for the retained
frame-0 freeze cut. Natural-lifecycle Whispy-mouth and Fox material keys are
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
OWNER_LABELS = tuple((name, mask) for name, _root, _count, mask in OWNER_SPECS) + (
    ("fox_late_material", FOX_LATE_MATERIAL_OWNER_MASK),
    ("whispy_mouth", WHISPY_MOUTH_OWNER_MASK),
)

EXPECTED_KEY_COUNT = 24
EXPECTED_OUTPUT_COUNT = 23
EXPECTED_RESIDENCY_BYTES = 136192
EXPECTED_PAYLOAD_BYTES = 132096
EXPECTED_ORACLE_PIXELS = 65024
EXPECTED_PAYLOAD_SHA256 = (
    "59bfd565c4c0c18c605107e0f7b49e4cc6c360cd9ce9a67712066c2a58e182d2"
)
EXPECTED_METADATA_SHA256 = (
    "1129f8b59a4be9a44583bef06ba998682b9f7ad14e8844c4e1d843948a668081"
)
EXPECTED_INCLUDE_SHA256 = (
    "cef1256e16f20d90d137312de099d4ff4044883cdc6f1aef38e9c4d8cca66430"
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


def pack_payload(records: Sequence[PreparedRecord]) -> tuple[bytes, int]:
    payload = bytearray()
    output_offsets: dict[bytes, tuple[int, int]] = {}
    for record in records:
        existing = output_offsets.get(record.pixels)
        if existing is None:
            existing = (len(payload), len(record.pixels))
            output_offsets[record.pixels] = existing
            payload.extend(record.pixels)
        record.payload_offset, record.payload_bytes = existing
    return bytes(payload), len(output_offsets)


def build_include(
    records: Sequence[PreparedRecord],
    payload: bytes,
    census_sha256: str,
    metadata_sha256: str,
) -> bytes:
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
    dynamic = manifest["dynamic_animated_owners"]
    if not isinstance(dynamic, dict):
        raise falsify("source census lost dynamic_animated_owners")
    dynamic_blocks = dynamic["source_blocks"]
    if not isinstance(dynamic_blocks, list):
        raise falsify("source census lost dynamic source blocks")
    records.append(
        build_runtime_qualified_whispy_record(repo_root, dynamic_blocks)
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
    residency_bytes = sum(record.upload_width * record.upload_height * 2 for record in records)
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
