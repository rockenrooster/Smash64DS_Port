#!/usr/bin/env python3
"""Generate the exact tiled Nintendo DS Pupupu-water host packet.

This is an offline M4 packet, not live renderer integration.  It replays the
pinned BattleShip material scripts, separates each output into an RGB256
CI4-pair atlas and an exact RGB4 visibility atlas, and proves reconstruction
against the current DS RGB5A1 oracle for every unique key in the 216-frame
period.  Generated C data remains deliberately absent from the Makefile until
the pre-GO VRAM transition and two-pass depth contract pass on hardware.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
from typing import Iterable, Sequence, TypeVar

import generate_pupupu_water_aot as source


PAIR_TILE_WIDTH = 32
PAIR_TILE_HEIGHT = 8
PAIR_TILE_BYTES = PAIR_TILE_WIDTH * PAIR_TILE_HEIGHT
VISIBILITY_TILE_BYTES = PAIR_TILE_BYTES // 4

PAIR_ATLAS_WIDTH = 512
PAIR_ATLAS_HEIGHT = 256
PAIR_ATLAS_COLUMNS = PAIR_ATLAS_WIDTH // PAIR_TILE_WIDTH
PAIR_ATLAS_ROWS = PAIR_ATLAS_HEIGHT // PAIR_TILE_HEIGHT
PAIR_ATLAS_BYTES = PAIR_ATLAS_WIDTH * PAIR_ATLAS_HEIGHT

VISIBILITY_ATLAS_WIDTH = 256
VISIBILITY_ATLAS_HEIGHT = 256
VISIBILITY_ATLAS_COLUMNS = VISIBILITY_ATLAS_WIDTH // PAIR_TILE_WIDTH
VISIBILITY_ATLAS_ROWS = VISIBILITY_ATLAS_HEIGHT // PAIR_TILE_HEIGHT
VISIBILITY_ATLAS_BYTES = (
    VISIBILITY_ATLAS_WIDTH * VISIBILITY_ATLAS_HEIGHT // 4
)

FRACTION_MIN = 114
FRACTION_MAX = 153
FRACTION_COUNT = FRACTION_MAX - FRACTION_MIN + 1
PAIR_PALETTE_ENTRIES = 256
PAIR_PALETTE_BYTES = FRACTION_COUNT * PAIR_PALETTE_ENTRIES * 2

EXPECTED_PAIR_TILES = 494
EXPECTED_VISIBILITY_TILES = 243
EXPECTED_LARGE_PAIR_STATES = 33
EXPECTED_LARGE_VISIBILITY_STATES = 38
EXPECTED_SMALL_PAIR_STATES = 41
EXPECTED_SMALL_VISIBILITY_STATES = 46
EXPECTED_KEY_COUNT = 322
EXPECTED_ORACLE_PIXELS = 3_024_896
EXPECTED_STATE_TABLE_BYTES = 8_544
EXPECTED_PLAN_CELLS = 68
EXPECTED_PLAN_VERTICES = 274
EXPECTED_DRAWS_PER_PASS = 68
EXPECTED_TRIANGLES_PER_PASS = 138
EXPECTED_TOTAL_DRAWS = 136
EXPECTED_TOTAL_TRIANGLES = 276
EXPECTED_GX_BEGIN_BATCHES = 2
EXPECTED_EMITTED_VERTICES = EXPECTED_TOTAL_TRIANGLES * 3
EXPECTED_VERTEX_ATTRIBUTE_WRITES = EXPECTED_EMITTED_VERTICES * 3
DEVICE_TICK_BUDGET = 40_000
DEVICE_TICK_REJECT = 50_000
MIN_NET_OWNER_DRAW_SAVING = 100_000

LARGE_VERTEX_OFFSET = 0x20E8
SMALL_VERTEX_OFFSET = 0x2168
SOURCE_VERTEX_COUNT = 8
SOURCE_POLYGON_ORDER = (0, 2, 3, 7, 4, 6, 5, 1)
LAYER1_DOBJ_DESC_OFFSET = 0x2450
LAYER1_DOBJ_DESC_BYTES = 44
OWNER_DOBJ_DESC_INDEX = {
    source.OWNER_LARGE: 2,
    source.OWNER_SMALL: 3,
}
EXPECTED_OWNER_TRANSLATION_XZ = {
    source.OWNER_LARGE: (-435, 885),
    source.OWNER_SMALL: (795, 1080),
}

HEADER_RELATIVE = Path("include/nds/pupupu_water_tiled_aot.h")
MODULE_RELATIVE = Path("src/nds/pupupu_water_tiled_aot.c")
INCLUDE_RELATIVE = Path(
    "src/nds/generated/pupupu_water_tiled_aot.generated.inc"
)

# Filled after the first exact construction.  Updating one of these values is
# an explicit review action, not an automatic side effect of --write.
EXPECTED_DIGESTS: dict[str, str] = {
    "source_stream": "a286f0beea8547344c083b88bb1fbcc844bdb3ea9059c6a5d7c8cfed2dffc288",
    "oracle": "5471dfe68ba1a3346cf64799938feb5f40d6afe8879f2f2c3984bca511a66c22",
    "clipped_oracle": "7e3989a8ced63e5a2a7b3534d686b189a624acfae73fc7eac98ea292da3657ec",
    "pair_atlas": "203e36a96c8125df5204baf02f271a07e0248b8aa51a50bff32715ccfa117f9f",
    "visibility_atlas": "80e31c49f39e1ca704fb83e3b9d403f19cd6e7afb1491308b26321a276fcba44",
    "palettes": "b55d60bedc30df55fc92197b839e97c1ce2dc733f7055ea9e824ddea095d6d47",
    "state_tables": "6405e1a67c07b88efe90885d0b746eb5635de4bfcda7ad331aa79f66bdad2d4e",
    "geometry_plan": "cbe6f7546a0ea6c13e3c28fd055634d4170d0f3dc6df48f3a50c610ae91c483b",
    "combined": "36fbc1c7f4d08f178ac7564b8d5c9c3e3f1dbc2ee1b0f7a60e09ab096b3b2018",
}

T = TypeVar("T")


def fail(message: str) -> source.Falsifier:
    return source.falsify(f"tiled AOT: {message}")


def sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise fail(message)


def pack_visibility(values: bytes) -> bytes:
    require(len(values) == PAIR_TILE_BYTES, "visibility tile shape changed")
    output = bytearray(VISIBILITY_TILE_BYTES)
    for offset, value in enumerate(values):
        require(value in (0, 1), "visibility texel left its one-bit domain")
        output[offset >> 2] |= value << ((offset & 3) * 2)
    return bytes(output)


def unpack_visibility(payload: bytes, offset: int) -> int:
    return (payload[offset >> 2] >> ((offset & 3) * 2)) & 0x3


@dataclass(frozen=True)
class KeyImage:
    pair: bytes
    visibility: bytes
    expected: bytes


@dataclass(frozen=True)
class FrameState:
    pair_state: int
    visibility_state: int


@dataclass(frozen=True)
class OwnerTables:
    spec: source.MObjSpec
    census: source.KeyCensus
    key_images: dict[source.WaterKey, KeyImage]
    pair_states: tuple[tuple[int, ...], ...]
    visibility_states: tuple[tuple[int, ...], ...]
    frames: tuple[FrameState, ...]


@dataclass(frozen=True)
class PlanVertex:
    x_q12: int
    z_q12: int
    s_q16: int
    t_q16: int

    def payload(self) -> bytes:
        return struct.pack("<iiii", self.x_q12, self.z_q12, self.s_q16, self.t_q16)


@dataclass(frozen=True)
class PlanCell:
    first_vertex: int
    owner: int
    cell_index: int
    cell_x: int
    cell_y: int
    vertex_count: int
    triangle_count: int

    def payload(self) -> bytes:
        return struct.pack(
            "<HBBBBBB",
            self.first_vertex,
            self.owner,
            self.cell_index,
            self.cell_x,
            self.cell_y,
            self.vertex_count,
            self.triangle_count,
        )


@dataclass(frozen=True)
class ClipVertex:
    s: Fraction
    t: Fraction
    x: Fraction
    z: Fraction


@dataclass(frozen=True)
class GeneratedModel:
    corpus: source.SourceCorpus
    owners: tuple[OwnerTables, OwnerTables]
    pair_tiles: tuple[bytes, ...]
    visibility_tiles: tuple[bytes, ...]
    pair_atlas: bytes
    visibility_atlas: bytes
    palettes: tuple[tuple[int, ...], ...]
    palette_payload: bytes
    state_payload: bytes
    plan_cells: tuple[PlanCell, ...]
    plan_vertices: tuple[PlanVertex, ...]
    geometry_payload: bytes
    source_stream_payload: bytes
    oracle_payload: bytes
    oracle_mismatches: int
    oracle_pixels: int
    clipped_oracle_payload: bytes
    clipped_oracle_mismatches: int
    clipped_oracle_pixels: int

    def digests(self) -> dict[str, str]:
        combined = (
            self.pair_atlas
            + self.visibility_atlas
            + self.palette_payload
            + self.state_payload
            + self.geometry_payload
        )
        return {
            "source_stream": sha256(self.source_stream_payload),
            "oracle": sha256(self.oracle_payload),
            "clipped_oracle": sha256(self.clipped_oracle_payload),
            "pair_atlas": sha256(self.pair_atlas),
            "visibility_atlas": sha256(self.visibility_atlas),
            "palettes": sha256(self.palette_payload),
            "state_tables": sha256(self.state_payload),
            "geometry_plan": sha256(self.geometry_payload),
            "combined": sha256(combined),
        }

    def summary(self) -> dict[str, object]:
        return {
            "cycle_frames": source.EXPECTED_CYCLE_FRAMES,
            "keys": sum(len(owner.census.unique_keys) for owner in self.owners),
            "oracle_pixels": self.oracle_pixels,
            "oracle_mismatches": self.oracle_mismatches,
            "clipped_oracle_pixels": self.clipped_oracle_pixels,
            "clipped_oracle_mismatches": self.clipped_oracle_mismatches,
            "pair_tiles": len(self.pair_tiles),
            "pair_atlas_bytes": len(self.pair_atlas),
            "visibility_tiles": len(self.visibility_tiles),
            "visibility_atlas_bytes": len(self.visibility_atlas),
            "fractions": len(self.palettes),
            "palette_bytes": len(self.palette_payload),
            "large_pair_states": len(self.owners[0].pair_states),
            "large_visibility_states": len(self.owners[0].visibility_states),
            "small_pair_states": len(self.owners[1].pair_states),
            "small_visibility_states": len(self.owners[1].visibility_states),
            "state_table_bytes": len(self.state_payload),
            "plan_cells": len(self.plan_cells),
            "plan_vertices": len(self.plan_vertices),
            "logical_cells_per_pass": EXPECTED_DRAWS_PER_PASS,
            "triangles_per_pass": sum(cell.triangle_count for cell in self.plan_cells),
            "logical_cell_submissions": EXPECTED_TOTAL_DRAWS,
            "total_triangles": 2 * sum(cell.triangle_count for cell in self.plan_cells),
            "planned_gx_begin_batches": EXPECTED_GX_BEGIN_BATCHES,
            "emitted_vertices": EXPECTED_EMITTED_VERTICES,
            "vertex_attribute_writes": EXPECTED_VERTEX_ATTRIBUTE_WRITES,
            "digests": self.digests(),
        }


def key_image(
    corpus: source.SourceCorpus,
    spec: source.MObjSpec,
    key: source.WaterKey,
) -> KeyImage:
    texture0, texture1 = source.key_sources(corpus, spec, key)
    source0_s, source1_s, source0_t, source1_t = source.render_coordinate_maps(
        spec, key
    )
    pair_lut = source.build_pair_lut(corpus.palette, key.fraction)
    pair = bytearray(spec.width * spec.height)
    visibility = bytearray(spec.width * spec.height)
    for y in range(spec.height):
        row0 = source0_t[y] * source.CI4_SOURCE_WIDTH
        row1 = source1_t[y] * source.CI4_SOURCE_WIDTH
        for x in range(spec.width):
            offset = y * spec.width + x
            index0 = source.ci4_index(texture0, row0 + source0_s[x])
            index1 = source.ci4_index(texture1, row1 + source1_s[x])
            pair_value = (index0 << 4) | index1
            pair[offset] = pair_value
            visibility[offset] = source.resolve_pair(pair_lut[pair_value], x, y) >> 15
    expected = source.render_reference(corpus, spec, key)
    return KeyImage(pair=bytes(pair), visibility=bytes(visibility), expected=expected)


def pair_tile(image: bytes, width: int, x0: int, y0: int) -> bytes:
    return b"".join(
        image[(y0 + y) * width + x0 : (y0 + y) * width + x0 + PAIR_TILE_WIDTH]
        for y in range(PAIR_TILE_HEIGHT)
    )


def visibility_tile(image: bytes, width: int, x0: int, y0: int) -> bytes:
    unpacked = b"".join(
        image[(y0 + y) * width + x0 : (y0 + y) * width + x0 + PAIR_TILE_WIDTH]
        for y in range(PAIR_TILE_HEIGHT)
    )
    return pack_visibility(unpacked)


def intern(value: T, lookup: dict[T, int], values: list[T]) -> int:
    found = lookup.get(value)
    if found is not None:
        return found
    found = len(values)
    lookup[value] = found
    values.append(value)
    return found


def build_owner_tables(
    corpus: source.SourceCorpus,
    census: source.KeyCensus,
    pair_tile_lookup: dict[bytes, int],
    pair_tiles: list[bytes],
    visibility_tile_lookup: dict[bytes, int],
    visibility_tiles: list[bytes],
) -> OwnerTables:
    spec = census.spec
    cycle = census.sequence[: source.EXPECTED_CYCLE_FRAMES]
    require(
        all(
            key == census.sequence[index % source.EXPECTED_CYCLE_FRAMES]
            for index, key in enumerate(census.sequence)
        ),
        f"{spec.name}: full source simulation left the 216-frame period",
    )
    key_images = {
        key: key_image(corpus, spec, key) for key in census.unique_keys
    }
    require(
        set(cycle) == set(census.unique_keys),
        f"{spec.name}: one cycle does not contain every unique source key",
    )

    pair_state_lookup: dict[tuple[int, ...], int] = {}
    pair_states: list[tuple[int, ...]] = []
    visibility_state_lookup: dict[tuple[int, ...], int] = {}
    visibility_states: list[tuple[int, ...]] = []
    frames: list[FrameState] = []
    for key in cycle:
        image = key_images[key]
        pair_ids: list[int] = []
        visibility_ids: list[int] = []
        for y0 in range(0, spec.height, PAIR_TILE_HEIGHT):
            for x0 in range(0, spec.width, PAIR_TILE_WIDTH):
                p = pair_tile(image.pair, spec.width, x0, y0)
                v = visibility_tile(image.visibility, spec.width, x0, y0)
                pair_ids.append(intern(p, pair_tile_lookup, pair_tiles))
                visibility_ids.append(
                    intern(v, visibility_tile_lookup, visibility_tiles)
                )
        pair_state = intern(tuple(pair_ids), pair_state_lookup, pair_states)
        visibility_state = intern(
            tuple(visibility_ids), visibility_state_lookup, visibility_states
        )
        require(pair_state <= 0xFF, f"{spec.name}: pair state no longer fits u8")
        require(
            visibility_state <= 0xFF,
            f"{spec.name}: visibility state no longer fits u8",
        )
        frames.append(FrameState(pair_state, visibility_state))
    return OwnerTables(
        spec=spec,
        census=census,
        key_images=key_images,
        pair_states=tuple(pair_states),
        visibility_states=tuple(visibility_states),
        frames=tuple(frames),
    )


def build_pair_atlas(tiles: Sequence[bytes]) -> bytes:
    require(len(tiles) <= PAIR_ATLAS_COLUMNS * PAIR_ATLAS_ROWS, "pair atlas overflow")
    atlas = bytearray(PAIR_ATLAS_BYTES)
    for tile_id, tile in enumerate(tiles):
        require(len(tile) == PAIR_TILE_BYTES, "pair tile byte count changed")
        x0 = (tile_id % PAIR_ATLAS_COLUMNS) * PAIR_TILE_WIDTH
        y0 = (tile_id // PAIR_ATLAS_COLUMNS) * PAIR_TILE_HEIGHT
        for y in range(PAIR_TILE_HEIGHT):
            dst = (y0 + y) * PAIR_ATLAS_WIDTH + x0
            src = y * PAIR_TILE_WIDTH
            atlas[dst : dst + PAIR_TILE_WIDTH] = tile[src : src + PAIR_TILE_WIDTH]
    return bytes(atlas)


def build_visibility_atlas(tiles: Sequence[bytes]) -> bytes:
    require(
        len(tiles) <= VISIBILITY_ATLAS_COLUMNS * VISIBILITY_ATLAS_ROWS,
        "visibility atlas overflow",
    )
    atlas_stride = VISIBILITY_ATLAS_WIDTH // 4
    tile_stride = PAIR_TILE_WIDTH // 4
    atlas = bytearray(VISIBILITY_ATLAS_BYTES)
    for tile_id, tile in enumerate(tiles):
        require(len(tile) == VISIBILITY_TILE_BYTES, "visibility tile byte count changed")
        x0 = (tile_id % VISIBILITY_ATLAS_COLUMNS) * tile_stride
        y0 = (tile_id // VISIBILITY_ATLAS_COLUMNS) * PAIR_TILE_HEIGHT
        for y in range(PAIR_TILE_HEIGHT):
            dst = (y0 + y) * atlas_stride + x0
            src = y * tile_stride
            atlas[dst : dst + tile_stride] = tile[src : src + tile_stride]
    return bytes(atlas)


def build_palettes(corpus: source.SourceCorpus) -> tuple[tuple[int, ...], ...]:
    palettes: list[tuple[int, ...]] = []
    for fraction in range(FRACTION_MIN, FRACTION_MAX + 1):
        pair_lut = source.build_pair_lut(corpus.palette, fraction)
        palettes.append(tuple(value & 0x7FFF for value in pair_lut))
    return tuple(palettes)


def serialize_palettes(palettes: Sequence[Sequence[int]]) -> bytes:
    output = bytearray()
    for palette in palettes:
        require(len(palette) == PAIR_PALETTE_ENTRIES, "pair palette shape changed")
        output.extend(struct.pack("<256H", *palette))
    return bytes(output)


def serialize_state_tables(owners: Sequence[OwnerTables]) -> bytes:
    output = bytearray()
    for owner in owners:
        for state in owner.pair_states:
            output.extend(struct.pack(f"<{len(state)}H", *state))
        for state in owner.visibility_states:
            output.extend(bytes(state))
    for owner in owners:
        for frame in owner.frames:
            output.extend((frame.pair_state, frame.visibility_state))
    return bytes(output)


def read_source_vertices(
    corpus: source.SourceCorpus, spec: source.MObjSpec
) -> list[ClipVertex]:
    offset = LARGE_VERTEX_OFFSET if spec.owner == source.OWNER_LARGE else SMALL_VERTEX_OFFSET
    raw: list[tuple[int, int, int, int]] = []
    for index in range(SOURCE_VERTEX_COUNT):
        values = struct.unpack_from(
            ">hhhHhhBBBB", corpus.bank104.payload, offset + index * 16
        )
        raw.append((values[0], values[2], values[4], values[5]))
    min_s = min(value[2] for value in raw)
    min_t = min(value[3] for value in raw)
    dobj_offset = (
        LAYER1_DOBJ_DESC_OFFSET
        + OWNER_DOBJ_DESC_INDEX[spec.owner] * LAYER1_DOBJ_DESC_BYTES
    )
    translate_x, translate_y, translate_z = struct.unpack_from(
        ">fff", corpus.bank104.payload, dobj_offset + 8
    )
    require(translate_y == 0.0, f"{spec.name}: pond DObj left the shared Y plane")
    require(
        translate_x.is_integer() and translate_z.is_integer(),
        f"{spec.name}: pond DObj translation is no longer integer-exact",
    )
    tx = int(translate_x)
    tz = int(translate_z)
    require(
        (tx, tz) == EXPECTED_OWNER_TRANSLATION_XZ[spec.owner],
        f"{spec.name}: pond DObj translation changed to {tx},{tz}",
    )
    vertices = [
        ClipVertex(
            s=Fraction(raw[index][2] - min_s),
            t=Fraction(raw[index][3] - min_t),
            x=Fraction(raw[index][0] + tx),
            z=Fraction(raw[index][1] + tz),
        )
        for index in SOURCE_POLYGON_ORDER
    ]
    require(
        max(vertex.s for vertex in vertices) == spec.width * 32,
        f"{spec.name}: source S extent changed",
    )
    require(
        max(vertex.t for vertex in vertices) == spec.height * 32,
        f"{spec.name}: source T extent changed",
    )
    return vertices


def interpolate(a: ClipVertex, b: ClipVertex, axis: str, limit: Fraction) -> ClipVertex:
    av = getattr(a, axis)
    bv = getattr(b, axis)
    ratio = (limit - av) / (bv - av)
    return ClipVertex(
        s=a.s + (b.s - a.s) * ratio,
        t=a.t + (b.t - a.t) * ratio,
        x=a.x + (b.x - a.x) * ratio,
        z=a.z + (b.z - a.z) * ratio,
    )


def clip_axis(
    polygon: Sequence[ClipVertex], axis: str, limit: Fraction, keep_greater: bool
) -> list[ClipVertex]:
    if not polygon:
        return []
    output: list[ClipVertex] = []

    def inside(vertex: ClipVertex) -> bool:
        value = getattr(vertex, axis)
        return value >= limit if keep_greater else value <= limit

    for a, b in zip(polygon, tuple(polygon[1:]) + (polygon[0],)):
        a_inside = inside(a)
        b_inside = inside(b)
        if a_inside:
            output.append(a)
        if a_inside != b_inside:
            output.append(interpolate(a, b, axis, limit))
    deduplicated: list[ClipVertex] = []
    for vertex in output:
        if not deduplicated or vertex != deduplicated[-1]:
            deduplicated.append(vertex)
    if len(deduplicated) > 1 and deduplicated[0] == deduplicated[-1]:
        deduplicated.pop()
    return deduplicated


def clip_cell(
    polygon: Sequence[ClipVertex], x0: int, y0: int, x1: int, y1: int
) -> list[ClipVertex]:
    result = list(polygon)
    for axis, limit, keep_greater in (
        ("s", Fraction(x0), True),
        ("s", Fraction(x1), False),
        ("t", Fraction(y0), True),
        ("t", Fraction(y1), False),
    ):
        result = clip_axis(result, axis, limit, keep_greater)
    return result


def signed_area(polygon: Sequence[ClipVertex]) -> Fraction:
    return sum(
        a.s * b.t - b.s * a.t
        for a, b in zip(polygon, tuple(polygon[1:]) + (polygon[0],))
    ) / 2


def round_fraction(value: Fraction) -> int:
    if value < 0:
        return -round_fraction(-value)
    quotient, remainder = divmod(value.numerator, value.denominator)
    return quotient + (1 if remainder * 2 >= value.denominator else 0)


def build_geometry_plan(
    corpus: source.SourceCorpus,
) -> tuple[tuple[PlanCell, ...], tuple[PlanVertex, ...]]:
    cells: list[PlanCell] = []
    vertices: list[PlanVertex] = []
    for spec in corpus.specs:
        polygon = read_source_vertices(corpus, spec)
        columns = spec.width // PAIR_TILE_WIDTH
        for cell_y, y0_texel in enumerate(range(0, spec.height, PAIR_TILE_HEIGHT)):
            for cell_x, x0_texel in enumerate(range(0, spec.width, PAIR_TILE_WIDTH)):
                x0 = x0_texel * 32
                y0 = y0_texel * 32
                clipped = clip_cell(
                    polygon,
                    x0,
                    y0,
                    x0 + PAIR_TILE_WIDTH * 32,
                    y0 + PAIR_TILE_HEIGHT * 32,
                )
                if len(clipped) < 3 or signed_area(clipped) == 0:
                    continue
                require(signed_area(clipped) < 0, "geometry winding changed")
                first_vertex = len(vertices)
                for vertex in clipped:
                    vertices.append(
                        PlanVertex(
                            x_q12=round_fraction(vertex.x * 4096),
                            z_q12=round_fraction(vertex.z * 4096),
                            s_q16=round_fraction(vertex.s * 2048),
                            t_q16=round_fraction(vertex.t * 2048),
                        )
                    )
                cell_index = cell_y * columns + cell_x
                cells.append(
                    PlanCell(
                        first_vertex=first_vertex,
                        owner=spec.owner,
                        cell_index=cell_index,
                        cell_x=cell_x,
                        cell_y=cell_y,
                        vertex_count=len(clipped),
                        triangle_count=len(clipped) - 2,
                    )
                )
    return tuple(cells), tuple(vertices)


def serialize_stream(owners: Sequence[OwnerTables]) -> bytes:
    output = bytearray()
    for owner in owners:
        for key in owner.census.sequence[: source.EXPECTED_CYCLE_FRAMES]:
            output.extend(
                struct.pack(
                    "<BB8HB",
                    key.texture0,
                    key.texture1,
                    *key.as_list()[2:10],
                    key.fraction,
                )
            )
    return bytes(output)


def atlas_pair(atlas: bytes, tile_id: int, x: int, y: int) -> int:
    atlas_x = (tile_id % PAIR_ATLAS_COLUMNS) * PAIR_TILE_WIDTH + x
    atlas_y = (tile_id // PAIR_ATLAS_COLUMNS) * PAIR_TILE_HEIGHT + y
    return atlas[atlas_y * PAIR_ATLAS_WIDTH + atlas_x]


def atlas_visibility(atlas: bytes, tile_id: int, x: int, y: int) -> int:
    atlas_x = (tile_id % VISIBILITY_ATLAS_COLUMNS) * PAIR_TILE_WIDTH + x
    atlas_y = (tile_id // VISIBILITY_ATLAS_COLUMNS) * PAIR_TILE_HEIGHT + y
    offset = atlas_y * VISIBILITY_ATLAS_WIDTH + atlas_x
    return unpack_visibility(atlas, offset)


def verify_oracle(
    owners: Sequence[OwnerTables],
    pair_atlas: bytes,
    visibility_atlas: bytes,
    palettes: Sequence[Sequence[int]],
) -> tuple[int, int, bytes]:
    mismatches = 0
    pixels = 0
    oracle_payload = bytearray()
    for owner in owners:
        spec = owner.spec
        columns = spec.width // PAIR_TILE_WIDTH
        first_frame = {
            key: frame
            for frame, key in enumerate(
                owner.census.sequence[: source.EXPECTED_CYCLE_FRAMES]
            )
        }
        for key in owner.census.unique_keys:
            frame = first_frame[key]
            state = owner.frames[frame]
            pair_ids = owner.pair_states[state.pair_state]
            visibility_ids = owner.visibility_states[state.visibility_state]
            expected = owner.key_images[key].expected
            palette = palettes[key.fraction - FRACTION_MIN]
            for y in range(spec.height):
                for x in range(spec.width):
                    cell = (y // PAIR_TILE_HEIGHT) * columns + x // PAIR_TILE_WIDTH
                    local_x = x % PAIR_TILE_WIDTH
                    local_y = y % PAIR_TILE_HEIGHT
                    pair = atlas_pair(pair_atlas, pair_ids[cell], local_x, local_y)
                    visible = atlas_visibility(
                        visibility_atlas,
                        visibility_ids[cell],
                        local_x,
                        local_y,
                    )
                    require(visible in (0, 1), "RGB4 visibility atlas used index 2/3")
                    actual = palette[pair] | (visible << 15)
                    expected_value = struct.unpack_from("<H", expected, (y * spec.width + x) * 2)[0]
                    mismatches += actual != expected_value
                    oracle_payload.extend(struct.pack("<H", expected_value))
                    pixels += 1
    return mismatches, pixels, bytes(oracle_payload)


def point_inside_clockwise_fraction(
    polygon: Sequence[ClipVertex], s: Fraction, t: Fraction
) -> bool:
    return all(
        (b.s - a.s) * (t - a.t) - (b.t - a.t) * (s - a.s) <= 0
        for a, b in zip(polygon, tuple(polygon[1:]) + (polygon[0],))
    )


def point_inside_clockwise_q16(
    polygon: Sequence[PlanVertex], s_q16: int, t_q16: int
) -> bool:
    return all(
        (b.s_q16 - a.s_q16) * (t_q16 - a.t_q16)
        - (b.t_q16 - a.t_q16) * (s_q16 - a.s_q16)
        <= 0
        for a, b in zip(polygon, tuple(polygon[1:]) + (polygon[0],))
    )


def verify_clipped_oracle(
    corpus: source.SourceCorpus,
    owners: Sequence[OwnerTables],
    pair_atlas: bytes,
    visibility_atlas: bytes,
    palettes: Sequence[Sequence[int]],
    plan_cells: Sequence[PlanCell],
    plan_vertices: Sequence[PlanVertex],
) -> tuple[int, int, bytes]:
    by_owner_and_index: dict[tuple[int, int], PlanCell] = {}
    owner_counts = [0, 0]
    for cell in plan_cells:
        require(cell.owner in (source.OWNER_LARGE, source.OWNER_SMALL), "plan owner changed")
        owner = owners[cell.owner]
        columns = owner.spec.width // PAIR_TILE_WIDTH
        rows = owner.spec.height // PAIR_TILE_HEIGHT
        state_cell_count = columns * rows
        require(
            cell.cell_x < columns and cell.cell_y < rows,
            "plan cell coordinate is out of range",
        )
        require(
            cell.cell_index == cell.cell_y * columns + cell.cell_x,
            "plan cell index no longer addresses its state-grid cell",
        )
        require(cell.cell_index < state_cell_count, "plan cell index exceeds its state grid")
        require(
            (cell.owner, cell.cell_index) not in by_owner_and_index,
            "plan contains a duplicate state-grid cell",
        )
        require(cell.vertex_count >= 3, "plan cell has fewer than three vertices")
        require(cell.triangle_count == cell.vertex_count - 2, "plan fan triangle count changed")
        require(
            cell.first_vertex + cell.vertex_count <= len(plan_vertices),
            "plan vertex span is out of range",
        )
        polygon = plan_vertices[cell.first_vertex : cell.first_vertex + cell.vertex_count]
        min_s = cell.cell_x * PAIR_TILE_WIDTH << 16
        max_s = (cell.cell_x + 1) * PAIR_TILE_WIDTH << 16
        min_t = cell.cell_y * PAIR_TILE_HEIGHT << 16
        max_t = (cell.cell_y + 1) * PAIR_TILE_HEIGHT << 16
        require(
            all(
                min_s <= vertex.s_q16 <= max_s and min_t <= vertex.t_q16 <= max_t
                for vertex in polygon
            ),
            "plan vertex escaped its state-grid cell",
        )
        by_owner_and_index[(cell.owner, cell.cell_index)] = cell
        owner_counts[cell.owner] += 1
    require(owner_counts == [60, 8], "plan must contain exactly 60 large and 8 small cells")

    mismatches = 0
    pixels = 0
    payload = bytearray()
    for owner in owners:
        spec = owner.spec
        source_polygon = read_source_vertices(corpus, spec)
        columns = spec.width // PAIR_TILE_WIDTH
        first_frame = {
            key: frame
            for frame, key in enumerate(
                owner.census.sequence[: source.EXPECTED_CYCLE_FRAMES]
            )
        }
        coverage: list[tuple[int, int, int]] = []
        for y in range(spec.height):
            for x in range(spec.width):
                cell_index = (y // PAIR_TILE_HEIGHT) * columns + x // PAIR_TILE_WIDTH
                cell = by_owner_and_index.get((spec.owner, cell_index))
                plan_inside = False
                if cell is not None:
                    polygon = plan_vertices[
                        cell.first_vertex : cell.first_vertex + cell.vertex_count
                    ]
                    plan_inside = point_inside_clockwise_q16(
                        polygon, (x << 16) + 0x8000, (y << 16) + 0x8000
                    )
                source_inside = point_inside_clockwise_fraction(
                    source_polygon, Fraction(x * 32 + 16), Fraction(y * 32 + 16)
                )
                require(
                    plan_inside == source_inside,
                    f"{spec.name}: clipped pixel coverage differs at {x},{y}",
                )
                if plan_inside:
                    require(cell is not None, "covered pixel has no plan cell")
                    coverage.append((x, y, cell.cell_index))

        for key in owner.census.unique_keys:
            frame = first_frame[key]
            state = owner.frames[frame]
            pair_ids = owner.pair_states[state.pair_state]
            visibility_ids = owner.visibility_states[state.visibility_state]
            expected = owner.key_images[key].expected
            palette = palettes[key.fraction - FRACTION_MIN]
            for x, y, cell_index in coverage:
                pair = atlas_pair(
                    pair_atlas,
                    pair_ids[cell_index],
                    x % PAIR_TILE_WIDTH,
                    y % PAIR_TILE_HEIGHT,
                )
                visible = atlas_visibility(
                    visibility_atlas,
                    visibility_ids[cell_index],
                    x % PAIR_TILE_WIDTH,
                    y % PAIR_TILE_HEIGHT,
                )
                actual = palette[pair] | (visible << 15)
                expected_value = struct.unpack_from(
                    "<H", expected, (y * spec.width + x) * 2
                )[0]
                mismatches += actual != expected_value
                payload.extend(struct.pack("<H", expected_value))
                pixels += 1
    return mismatches, pixels, bytes(payload)


def build_model(repo_root: Path) -> GeneratedModel:
    corpus = source.load_source_corpus(repo_root.resolve())
    censuses = tuple(
        source.census_keys(corpus.bank104.payload, spec) for spec in corpus.specs
    )
    pair_tile_lookup: dict[bytes, int] = {}
    pair_tiles: list[bytes] = []
    visibility_tile_lookup: dict[bytes, int] = {}
    visibility_tiles: list[bytes] = []
    owners = tuple(
        build_owner_tables(
            corpus,
            census,
            pair_tile_lookup,
            pair_tiles,
            visibility_tile_lookup,
            visibility_tiles,
        )
        for census in censuses
    )
    pair_atlas = build_pair_atlas(pair_tiles)
    visibility_atlas = build_visibility_atlas(visibility_tiles)
    palettes = build_palettes(corpus)
    palette_payload = serialize_palettes(palettes)
    state_payload = serialize_state_tables(owners)
    plan_cells, plan_vertices = build_geometry_plan(corpus)
    geometry_payload = b"".join(cell.payload() for cell in plan_cells) + b"".join(
        vertex.payload() for vertex in plan_vertices
    )
    mismatches, pixels, oracle_payload = verify_oracle(
        owners, pair_atlas, visibility_atlas, palettes
    )
    clipped_mismatches, clipped_pixels, clipped_payload = verify_clipped_oracle(
        corpus,
        owners,
        pair_atlas,
        visibility_atlas,
        palettes,
        plan_cells,
        plan_vertices,
    )
    model = GeneratedModel(
        corpus=corpus,
        owners=owners,  # type: ignore[arg-type]
        pair_tiles=tuple(pair_tiles),
        visibility_tiles=tuple(visibility_tiles),
        pair_atlas=pair_atlas,
        visibility_atlas=visibility_atlas,
        palettes=palettes,
        palette_payload=palette_payload,
        state_payload=state_payload,
        plan_cells=plan_cells,
        plan_vertices=plan_vertices,
        geometry_payload=geometry_payload,
        source_stream_payload=serialize_stream(owners),
        oracle_payload=oracle_payload,
        oracle_mismatches=mismatches,
        oracle_pixels=pixels,
        clipped_oracle_payload=clipped_payload,
        clipped_oracle_mismatches=clipped_mismatches,
        clipped_oracle_pixels=clipped_pixels,
    )
    verify_contract(model)
    return model


def verify_contract(model: GeneratedModel) -> None:
    large, small = model.owners
    require(model.oracle_mismatches == 0, "oracle reconstruction mismatch")
    require(model.clipped_oracle_mismatches == 0, "clipped oracle reconstruction mismatch")
    require(model.oracle_pixels == EXPECTED_ORACLE_PIXELS, "oracle pixel count changed")
    require(
        sum(len(owner.census.unique_keys) for owner in model.owners) == EXPECTED_KEY_COUNT,
        "unique key count changed",
    )
    require(len(model.pair_tiles) == EXPECTED_PAIR_TILES, "pair tile count changed")
    require(
        len(model.visibility_tiles) == EXPECTED_VISIBILITY_TILES,
        "visibility tile count changed",
    )
    require(len(model.pair_atlas) == PAIR_ATLAS_BYTES, "pair atlas size changed")
    require(
        len(model.visibility_atlas) == VISIBILITY_ATLAS_BYTES,
        "visibility atlas size changed",
    )
    require(len(model.palettes) == FRACTION_COUNT, "fraction palette count changed")
    require(len(model.palette_payload) == PAIR_PALETTE_BYTES, "palette size changed")
    require(len(large.pair_states) == EXPECTED_LARGE_PAIR_STATES, "large pair states changed")
    require(
        len(large.visibility_states) == EXPECTED_LARGE_VISIBILITY_STATES,
        "large visibility states changed",
    )
    require(len(small.pair_states) == EXPECTED_SMALL_PAIR_STATES, "small pair states changed")
    require(
        len(small.visibility_states) == EXPECTED_SMALL_VISIBILITY_STATES,
        "small visibility states changed",
    )
    require(len(model.state_payload) == EXPECTED_STATE_TABLE_BYTES, "state-table size changed")
    require(len(model.plan_cells) == EXPECTED_PLAN_CELLS, "geometry cell count changed")
    require(len(model.plan_vertices) == EXPECTED_PLAN_VERTICES, "geometry vertex count changed")
    triangles = sum(cell.triangle_count for cell in model.plan_cells)
    require(triangles == EXPECTED_TRIANGLES_PER_PASS, "geometry triangle count changed")
    require(2 * len(model.plan_cells) == EXPECTED_TOTAL_DRAWS, "two-pass draw count changed")
    require(2 * triangles == EXPECTED_TOTAL_TRIANGLES, "two-pass triangle count changed")
    require(
        EXPECTED_EMITTED_VERTICES == EXPECTED_TOTAL_TRIANGLES * 3,
        "batched GL_TRIANGLES vertex count changed",
    )
    require(
        EXPECTED_VERTEX_ATTRIBUTE_WRITES == EXPECTED_EMITTED_VERTICES * 3,
        "TEXCOORD plus VTX16 register-write count changed",
    )
    require(
        EXPECTED_GX_BEGIN_BATCHES == 2,
        "shared-atlas two-pass executor must use two GX BEGIN batches",
    )
    require(
        DEVICE_TICK_BUDGET < DEVICE_TICK_REJECT < MIN_NET_OWNER_DRAW_SAVING,
        "device timing keep/reject thresholds overlap",
    )
    for name, expected in EXPECTED_DIGESTS.items():
        actual = model.digests().get(name)
        require(actual == expected, f"{name} digest {actual} != pinned {expected}")


def format_u8(payload: bytes, indent: str = "    ") -> str:
    lines = []
    for start in range(0, len(payload), 16):
        values = ", ".join(f"0x{value:02x}" for value in payload[start : start + 16])
        lines.append(f"{indent}{values},")
    return "\n".join(lines)


def format_u16(values: Iterable[int], indent: str = "    ") -> str:
    sequence = list(values)
    lines = []
    for start in range(0, len(sequence), 12):
        row = ", ".join(f"0x{value:04x}" for value in sequence[start : start + 12])
        lines.append(f"{indent}{row},")
    return "\n".join(lines)


def format_palettes(palettes: Sequence[Sequence[int]]) -> str:
    rows = []
    for palette in palettes:
        require(len(palette) == PAIR_PALETTE_ENTRIES, "C palette shape changed")
        rows.append("    {")
        rows.append(format_u16(palette, indent="        "))
        rows.append("    },")
    return "\n".join(rows)


def format_pair_states(states: Sequence[Sequence[int]]) -> str:
    rows = []
    for state in states:
        values = ", ".join(f"{value}u" for value in state)
        rows.append(f"    {{ {values} }},")
    return "\n".join(rows)


def format_visibility_states(states: Sequence[Sequence[int]]) -> str:
    rows = []
    for state in states:
        values = ", ".join(f"{value}u" for value in state)
        rows.append(f"    {{ {values} }},")
    return "\n".join(rows)


def format_frames(frames: Sequence[FrameState]) -> str:
    rows = []
    for start in range(0, len(frames), 8):
        values = ", ".join(
            f"{{ {frame.pair_state}u, {frame.visibility_state}u }}"
            for frame in frames[start : start + 8]
        )
        rows.append(f"    {values},")
    return "\n".join(rows)


def render_header(model: GeneratedModel) -> bytes:
    digests = model.digests()
    return f'''/* Generated by scripts/generate_pupupu_water_tiled_aot.py. */
/* Host-exact M4 packet only; device integration remains unproven. */
#ifndef SSB64_NDS_PUPUPU_WATER_TILED_AOT_H
#define SSB64_NDS_PUPUPU_WATER_TILED_AOT_H

#include <PR/ultratypes.h>

#define NDS_PUPUPU_WATER_TILED_CYCLE_FRAMES 216u
#define NDS_PUPUPU_WATER_TILED_OWNER_COUNT 2u
#define NDS_PUPUPU_WATER_TILED_PAIR_TILE_WIDTH 32u
#define NDS_PUPUPU_WATER_TILED_PAIR_TILE_HEIGHT 8u
#define NDS_PUPUPU_WATER_TILED_PAIR_TILE_COUNT 494u
#define NDS_PUPUPU_WATER_TILED_VISIBILITY_TILE_COUNT 243u
#define NDS_PUPUPU_WATER_TILED_PAIR_ATLAS_WIDTH 512u
#define NDS_PUPUPU_WATER_TILED_PAIR_ATLAS_HEIGHT 256u
#define NDS_PUPUPU_WATER_TILED_PAIR_ATLAS_BYTES 131072u
#define NDS_PUPUPU_WATER_TILED_VISIBILITY_ATLAS_WIDTH 256u
#define NDS_PUPUPU_WATER_TILED_VISIBILITY_ATLAS_HEIGHT 256u
#define NDS_PUPUPU_WATER_TILED_VISIBILITY_ATLAS_BYTES 16384u
#define NDS_PUPUPU_WATER_TILED_FRACTION_MIN 114u
#define NDS_PUPUPU_WATER_TILED_FRACTION_MAX 153u
#define NDS_PUPUPU_WATER_TILED_FRACTION_COUNT 40u
#define NDS_PUPUPU_WATER_TILED_PALETTE_ENTRIES 256u
#define NDS_PUPUPU_WATER_TILED_PALETTE_BYTES 20480u
#define NDS_PUPUPU_WATER_TILED_STATE_TABLE_BYTES 8544u
#define NDS_PUPUPU_WATER_TILED_PLAN_CELL_COUNT 68u
#define NDS_PUPUPU_WATER_TILED_PLAN_VERTEX_COUNT 274u
#define NDS_PUPUPU_WATER_TILED_LOGICAL_CELLS_PER_PASS 68u
#define NDS_PUPUPU_WATER_TILED_TRIANGLES_PER_PASS 138u
#define NDS_PUPUPU_WATER_TILED_LOGICAL_CELL_SUBMISSIONS 136u
#define NDS_PUPUPU_WATER_TILED_TOTAL_TRIANGLES 276u
#define NDS_PUPUPU_WATER_TILED_PLANNED_GX_BEGIN_BATCHES 2u
#define NDS_PUPUPU_WATER_TILED_EMITTED_VERTICES 828u
#define NDS_PUPUPU_WATER_TILED_VERTEX_ATTRIBUTE_WRITES 2484u
#define NDS_PUPUPU_WATER_TILED_DEVICE_TICK_BUDGET 40000u
#define NDS_PUPUPU_WATER_TILED_DEVICE_TICK_REJECT 50000u
#define NDS_PUPUPU_WATER_TILED_MIN_NET_OWNER_DRAW_SAVING 100000u

#define NDS_PUPUPU_WATER_TILED_PAIR_ATLAS_SHA256 "{digests['pair_atlas']}"
#define NDS_PUPUPU_WATER_TILED_CLIPPED_ORACLE_SHA256 "{digests['clipped_oracle']}"
#define NDS_PUPUPU_WATER_TILED_VISIBILITY_ATLAS_SHA256 "{digests['visibility_atlas']}"
#define NDS_PUPUPU_WATER_TILED_PALETTES_SHA256 "{digests['palettes']}"
#define NDS_PUPUPU_WATER_TILED_STATE_TABLES_SHA256 "{digests['state_tables']}"
#define NDS_PUPUPU_WATER_TILED_GEOMETRY_PLAN_SHA256 "{digests['geometry_plan']}"
#define NDS_PUPUPU_WATER_TILED_COMBINED_SHA256 "{digests['combined']}"

enum NDSPupupuWaterTiledOwner {{
    NDS_PUPUPU_WATER_TILED_OWNER_LARGE = 0,
    NDS_PUPUPU_WATER_TILED_OWNER_SMALL = 1
}};

enum NDSPupupuWaterTiledResult {{
    NDS_PUPUPU_WATER_TILED_INVALID = -1,
    NDS_PUPUPU_WATER_TILED_OK = 1
}};

/* Texture-domain polygon vertices are clockwise fans. x/z are layer-parent
 * stage coordinates in 20.12 after applying each source pond DObj's exact
 * static translation; this permits one shared-matrix batch for both owners.
 * s/t are logical pond texels in 16.16. Device integration must still prove
 * this clipping plan and shared parent matrix against the live source mesh. */
typedef struct NDSPupupuWaterTiledPlanVertex {{
    s32 x_q12;
    s32 z_q12;
    s32 s_q16;
    s32 t_q16;
}} NDSPupupuWaterTiledPlanVertex;

typedef struct NDSPupupuWaterTiledPlanCell {{
    u16 first_vertex;
    u8 owner;
    u8 cell_index;
    u8 cell_x;
    u8 cell_y;
    u8 vertex_count;
    u8 triangle_count;
}} NDSPupupuWaterTiledPlanCell;

typedef struct NDSPupupuWaterTiledAssets {{
    const u8 *pair_atlas;
    const u8 *visibility_atlas;
    const u16 *pair_palettes;
    const NDSPupupuWaterTiledPlanCell *plan_cells;
    const NDSPupupuWaterTiledPlanVertex *plan_vertices;
    u16 plan_cell_count;
    u16 plan_vertex_count;
}} NDSPupupuWaterTiledAssets;

typedef struct NDSPupupuWaterTiledFrame {{
    const u16 *pair_tile_ids;
    const u8 *visibility_tile_ids;
    const u16 *pair_palette;
    u16 state_cell_count;
    u8 state_columns;
    u8 state_rows;
    u8 pair_state;
    u8 visibility_state;
    u8 palette_index;
    u8 reserved;
}} NDSPupupuWaterTiledFrame;

/* Atlas contract:
 * - pair_atlas is one 512x256 GL_RGB256 image. A u16 pair tile ID selects
 *   (id % 16) * 32, (id / 16) * 8. Its texel is (CI4_0 << 4) | CI4_1.
 * - visibility_atlas is one 256x256 GL_RGB4 image, four low-to-high 2-bit
 *   indices per byte. A u8 tile ID selects (id % 8) * 32, (id / 8) * 8;
 *   every texel is exactly 0 or 1, with texture index zero transparent.
 * - pair_palette is the 256-entry RGB555 palette selected by live fraction.
 * - cell-local atlas coordinates subtract cell_x * 32 and cell_y * 8 from
 *   each plan vertex's logical s/t before adding the selected tile origin. */

/* The caller supplies the live source material fraction and the corresponding
 * 0..215 material-cycle frame. No upload, allocation, I/O, or conversion is
 * performed by these borrowed immutable views. The intended device path is:
 * RGB4 visibility/depth pass, then RGB256 pair-color pass at equal depth.
 * Each plan cell's cell_index addresses the corresponding 64-entry large or
 * 8-entry small state arrays; state_cell_count is that rectangular grid size,
 * not the number of occupied geometry cells. Expand every clockwise cell fan
 * to GL_TRIANGLES inside one BEGIN per pass: 136 logical cell submissions are
 * two GX primitive batches, 276 triangles, 828 vertices, and 2,484 texture /
 * VTX16 register writes. Device timing and depth behavior remain keep gates. */
s32 ndsPupupuWaterTiledGetAssets(NDSPupupuWaterTiledAssets *out_assets);
s32 ndsPupupuWaterTiledGetFrame(
    u32 owner, u32 cycle_frame, u32 source_fraction,
    NDSPupupuWaterTiledFrame *out_frame);

#endif
'''.encode("utf-8")


def render_module() -> bytes:
    return b'''/* Generated by scripts/generate_pupupu_water_tiled_aot.py. */
/* Host-exact M4 packet only; absent from the production build by design. */
#include "nds/pupupu_water_tiled_aot.h"

#include "generated/pupupu_water_tiled_aot.generated.inc"

_Static_assert(sizeof(NDSPupupuWaterTiledPlanVertex) == 16u,
               "tiled-water plan vertex ABI changed");
_Static_assert(sizeof(NDSPupupuWaterTiledPlanCell) == 8u,
               "tiled-water plan cell ABI changed");
_Static_assert(sizeof(NDSPupupuWaterTiledAssets) == 24u,
               "tiled-water asset-view ABI changed");
_Static_assert(sizeof(NDSPupupuWaterTiledFrame) == 20u,
               "tiled-water frame-view ABI changed");
_Static_assert(sizeof(NDSPupupuWaterTiledGeneratedFrameState) == 2u,
               "tiled-water frame-state ABI changed");
_Static_assert(sizeof(sNdsPupupuWaterPairAtlas) ==
                   NDS_PUPUPU_WATER_TILED_PAIR_ATLAS_BYTES,
               "tiled-water pair atlas size changed");
_Static_assert(sizeof(sNdsPupupuWaterVisibilityAtlas) ==
                   NDS_PUPUPU_WATER_TILED_VISIBILITY_ATLAS_BYTES,
               "tiled-water visibility atlas size changed");
_Static_assert(sizeof(sNdsPupupuWaterPairPalettes) ==
                   NDS_PUPUPU_WATER_TILED_PALETTE_BYTES,
               "tiled-water palette size changed");
_Static_assert(sizeof(sNdsPupupuWaterLargePairStates) +
                   sizeof(sNdsPupupuWaterLargeVisibilityStates) +
                   sizeof(sNdsPupupuWaterSmallPairStates) +
                   sizeof(sNdsPupupuWaterSmallVisibilityStates) +
                   sizeof(sNdsPupupuWaterLargeFrames) +
                   sizeof(sNdsPupupuWaterSmallFrames) ==
                   NDS_PUPUPU_WATER_TILED_STATE_TABLE_BYTES,
               "tiled-water compact state footprint changed");
_Static_assert(sizeof(sNdsPupupuWaterPlanCells) ==
                   NDS_PUPUPU_WATER_TILED_PLAN_CELL_COUNT *
                       sizeof(NDSPupupuWaterTiledPlanCell),
               "tiled-water plan-cell footprint changed");
_Static_assert(sizeof(sNdsPupupuWaterPlanVertices) ==
                   NDS_PUPUPU_WATER_TILED_PLAN_VERTEX_COUNT *
                       sizeof(NDSPupupuWaterTiledPlanVertex),
               "tiled-water plan-vertex footprint changed");
_Static_assert(NDS_PUPUPU_WATER_TILED_PLAN_CELL_COUNT * 2u ==
                   NDS_PUPUPU_WATER_TILED_LOGICAL_CELL_SUBMISSIONS,
               "logical plan-cell submission count changed");
_Static_assert(NDS_PUPUPU_WATER_TILED_TRIANGLES_PER_PASS * 2u ==
                   NDS_PUPUPU_WATER_TILED_TOTAL_TRIANGLES,
               "two-pass triangle count changed");
_Static_assert(NDS_PUPUPU_WATER_TILED_TOTAL_TRIANGLES * 3u ==
                   NDS_PUPUPU_WATER_TILED_EMITTED_VERTICES,
               "GL_TRIANGLES vertex expansion changed");
_Static_assert(NDS_PUPUPU_WATER_TILED_EMITTED_VERTICES * 3u ==
                   NDS_PUPUPU_WATER_TILED_VERTEX_ATTRIBUTE_WRITES,
               "TEXCOORD/VTX16 register-write count changed");
_Static_assert(__alignof__(sNdsPupupuWaterPairAtlas) >= 32u,
               "pair atlas lost DMA alignment");
_Static_assert(__alignof__(sNdsPupupuWaterVisibilityAtlas) >= 32u,
               "visibility atlas lost DMA alignment");
_Static_assert(__alignof__(sNdsPupupuWaterPairPalettes) >= 32u,
               "pair palettes lost DMA alignment");

static void ndsPupupuWaterTiledClearFrame(NDSPupupuWaterTiledFrame *frame)
{
    if (frame == 0)
    {
        return;
    }
    frame->pair_tile_ids = 0;
    frame->visibility_tile_ids = 0;
    frame->pair_palette = 0;
    frame->state_cell_count = 0u;
    frame->state_columns = 0u;
    frame->state_rows = 0u;
    frame->pair_state = 0u;
    frame->visibility_state = 0u;
    frame->palette_index = 0u;
    frame->reserved = 0u;
}

s32 ndsPupupuWaterTiledGetAssets(NDSPupupuWaterTiledAssets *out_assets)
{
    if (out_assets == 0)
    {
        return NDS_PUPUPU_WATER_TILED_INVALID;
    }
    out_assets->pair_atlas = sNdsPupupuWaterPairAtlas;
    out_assets->visibility_atlas = sNdsPupupuWaterVisibilityAtlas;
    out_assets->pair_palettes = &sNdsPupupuWaterPairPalettes[0][0];
    out_assets->plan_cells = sNdsPupupuWaterPlanCells;
    out_assets->plan_vertices = sNdsPupupuWaterPlanVertices;
    out_assets->plan_cell_count = NDS_PUPUPU_WATER_TILED_PLAN_CELL_COUNT;
    out_assets->plan_vertex_count = NDS_PUPUPU_WATER_TILED_PLAN_VERTEX_COUNT;
    return NDS_PUPUPU_WATER_TILED_OK;
}

s32 ndsPupupuWaterTiledGetFrame(
    u32 owner, u32 cycle_frame, u32 source_fraction,
    NDSPupupuWaterTiledFrame *out_frame)
{
    const NDSPupupuWaterTiledGeneratedFrameState *state;
    u32 palette_index;

    ndsPupupuWaterTiledClearFrame(out_frame);
    if ((out_frame == 0) ||
        (cycle_frame >= NDS_PUPUPU_WATER_TILED_CYCLE_FRAMES) ||
        (source_fraction < NDS_PUPUPU_WATER_TILED_FRACTION_MIN) ||
        (source_fraction > NDS_PUPUPU_WATER_TILED_FRACTION_MAX))
    {
        return NDS_PUPUPU_WATER_TILED_INVALID;
    }
    palette_index = source_fraction - NDS_PUPUPU_WATER_TILED_FRACTION_MIN;
    if (owner == NDS_PUPUPU_WATER_TILED_OWNER_LARGE)
    {
        state = &sNdsPupupuWaterLargeFrames[cycle_frame];
        out_frame->pair_tile_ids =
            sNdsPupupuWaterLargePairStates[state->pair_state];
        out_frame->visibility_tile_ids =
            sNdsPupupuWaterLargeVisibilityStates[state->visibility_state];
        out_frame->state_cell_count = 64u;
        out_frame->state_columns = 4u;
        out_frame->state_rows = 16u;
    }
    else if (owner == NDS_PUPUPU_WATER_TILED_OWNER_SMALL)
    {
        state = &sNdsPupupuWaterSmallFrames[cycle_frame];
        out_frame->pair_tile_ids =
            sNdsPupupuWaterSmallPairStates[state->pair_state];
        out_frame->visibility_tile_ids =
            sNdsPupupuWaterSmallVisibilityStates[state->visibility_state];
        out_frame->state_cell_count = 8u;
        out_frame->state_columns = 1u;
        out_frame->state_rows = 8u;
    }
    else
    {
        return NDS_PUPUPU_WATER_TILED_INVALID;
    }
    out_frame->pair_palette = sNdsPupupuWaterPairPalettes[palette_index];
    out_frame->pair_state = state->pair_state;
    out_frame->visibility_state = state->visibility_state;
    out_frame->palette_index = (u8)palette_index;
    return NDS_PUPUPU_WATER_TILED_OK;
}
'''


def render_include(model: GeneratedModel) -> bytes:
    large, small = model.owners
    digests = model.digests()
    cells = "\n".join(
        "    { "
        f"{cell.first_vertex}u, {cell.owner}u, {cell.cell_index}u, "
        f"{cell.cell_x}u, {cell.cell_y}u, {cell.vertex_count}u, "
        f"{cell.triangle_count}u"
        " },"
        for cell in model.plan_cells
    )
    vertices = "\n".join(
        "    { "
        f"{vertex.x_q12}, {vertex.z_q12}, {vertex.s_q16}, {vertex.t_q16}"
        " },"
        for vertex in model.plan_vertices
    )
    text = f'''/* Generated by scripts/generate_pupupu_water_tiled_aot.py. */
/* Device integration is intentionally unproven and not enabled here. */
/* BattleShip103 SHA256: {model.corpus.bank103.source_sha256}. */
/* BattleShip104 SHA256: {model.corpus.bank104.source_sha256}. */
/* Source stream SHA256: {digests['source_stream']}. */
/* Oracle SHA256: {digests['oracle']}. */
/* Clipped oracle SHA256: {digests['clipped_oracle']}. */
/* Combined generated data SHA256: {digests['combined']}. */

typedef struct NDSPupupuWaterTiledGeneratedFrameState {{
    u8 pair_state;
    u8 visibility_state;
}} NDSPupupuWaterTiledGeneratedFrameState;

static const u8 sNdsPupupuWaterPairAtlas[131072]
    __attribute__((aligned(32))) = {{
{format_u8(model.pair_atlas)}
}};

static const u8 sNdsPupupuWaterVisibilityAtlas[16384]
    __attribute__((aligned(32))) = {{
{format_u8(model.visibility_atlas)}
}};

static const u16 sNdsPupupuWaterPairPalettes[40][256]
    __attribute__((aligned(32))) = {{
{format_palettes(model.palettes)}
}};

static const u16 sNdsPupupuWaterLargePairStates[33][64] = {{
{format_pair_states(large.pair_states)}
}};

static const u8 sNdsPupupuWaterLargeVisibilityStates[38][64] = {{
{format_visibility_states(large.visibility_states)}
}};

static const u16 sNdsPupupuWaterSmallPairStates[41][8] = {{
{format_pair_states(small.pair_states)}
}};

static const u8 sNdsPupupuWaterSmallVisibilityStates[46][8] = {{
{format_visibility_states(small.visibility_states)}
}};

static const NDSPupupuWaterTiledGeneratedFrameState
    sNdsPupupuWaterLargeFrames[216] = {{
{format_frames(large.frames)}
}};

static const NDSPupupuWaterTiledGeneratedFrameState
    sNdsPupupuWaterSmallFrames[216] = {{
{format_frames(small.frames)}
}};

static const NDSPupupuWaterTiledPlanCell sNdsPupupuWaterPlanCells[68] = {{
{cells}
}};

static const NDSPupupuWaterTiledPlanVertex sNdsPupupuWaterPlanVertices[274] = {{
{vertices}
}};
'''
    return text.encode("utf-8")


def generated_files(model: GeneratedModel) -> dict[Path, bytes]:
    return {
        HEADER_RELATIVE: render_header(model),
        MODULE_RELATIVE: render_module(),
        INCLUDE_RELATIVE: render_include(model),
    }


def write_files(repo_root: Path, files: dict[Path, bytes]) -> None:
    for relative, payload in files.items():
        path = repo_root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)


def check_files(repo_root: Path, files: dict[Path, bytes]) -> None:
    for relative, expected in files.items():
        path = repo_root / relative
        require(path.is_file(), f"generated file is absent: {relative.as_posix()}")
        actual = path.read_bytes()
        require(actual == expected, f"generated file is stale: {relative.as_posix()}")


def print_summary(model: GeneratedModel) -> None:
    summary = model.summary()
    digests = summary["digests"]
    assert isinstance(digests, dict)
    print(
        "PUPUPU_WATER_TILED_AOT_OK "
        f"cycle={summary['cycle_frames']} keys={summary['keys']} "
        f"oracle_pixels={summary['oracle_pixels']} mismatches={summary['oracle_mismatches']} "
        f"clipped_pixels={summary['clipped_oracle_pixels']} "
        f"clipped_mismatches={summary['clipped_oracle_mismatches']} "
        f"pair_tiles={summary['pair_tiles']} pair_bytes={summary['pair_atlas_bytes']} "
        f"visibility_tiles={summary['visibility_tiles']} "
        f"visibility_bytes={summary['visibility_atlas_bytes']} "
        f"palettes={summary['fractions']} palette_bytes={summary['palette_bytes']} "
        f"state_bytes={summary['state_table_bytes']} "
        f"logical_submissions={summary['logical_cell_submissions']} "
        f"gx_batches={summary['planned_gx_begin_batches']} "
        f"vertices={summary['emitted_vertices']} "
        f"triangles={summary['total_triangles']} combined_sha256={digests['combined']}"
    )


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="checkout containing the pinned BattleShip O2R corpus",
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true", help="write generated C packet")
    mode.add_argument("--check", action="store_true", help="verify generated C packet")
    parser.add_argument(
        "--repeat",
        type=int,
        default=1,
        help="repeat complete construction and require deterministic equality",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    repo_root = args.repo_root.resolve()
    try:
        require(args.repeat >= 1, "repeat count must be positive")
        model = build_model(repo_root)
        files = generated_files(model)
        for _ in range(1, args.repeat):
            repeated = build_model(repo_root)
            require(repeated.summary() == model.summary(), "repeated summary changed")
            require(generated_files(repeated) == files, "repeated generated files changed")
        if args.write:
            write_files(repo_root, files)
        else:
            check_files(repo_root, files)
        print_summary(model)
    except (OSError, ValueError, AssertionError, source.Falsifier) as exc:
        print(f"PUPUPU_WATER_TILED_AOT_FAIL: {exc}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
