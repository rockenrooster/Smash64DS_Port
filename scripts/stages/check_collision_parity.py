#!/usr/bin/env python3
"""Compare all nine VS stages' source collision data with their staged O2R data.

This is a host-only static parity check.  The live DS path copies these collision
integers without numerical conversion: O2R words are byte-swapped exactly and
the retained N64 halfwords are read high-half then low-half by
include/nds/nds_mp_topology.h.  Accordingly every integer comparison is exact;
light_angle is compared as exact binary32 bits.

Use --check for a gate-style nonzero exit when any stage fails.
"""
from __future__ import annotations

import argparse
import re
import struct
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence


HERE = Path(__file__).resolve().parent
DEFAULT_REPO_ROOT = HERE.parents[1]

PASS_FLAG = 0x4000
CLIFF_FLAG = 0x8000
GROUND_HEADER_OFFSET = 0x14

# N64 32-bit ABI offsets in MPGroundData (decomp/.../src/mp/mptypes.h).
# The live loader pins the two s16 runs with _Static_asserts in
# reloc_backend_assets.c:9024-9048 and normalizes them with exact half swaps.
GROUND_MAP_GEOMETRY = 64
GROUND_LAYER_MASK = 68
GROUND_LIGHT_ANGLE = 96
GROUND_CAMERA_BOUNDS = 108
GROUND_MAP_BOUNDS = 116
GROUND_BGM_ID = 124
GROUND_ITEM_WEIGHTS = 132
GROUND_ALT_WARNING = 136
GROUND_TEAM_BOUNDS = 138

# N64 32-bit ABI offsets in MPGeometryData.
GEOM_YAKUMONO_COUNT = 0
GEOM_VERTEX_DATA = 4
GEOM_VERTEX_ID = 8
GEOM_VERTEX_LINKS = 12
GEOM_LINE_INFO = 16
GEOM_MAPOBJ_COUNT = 20
GEOM_MAPOBJS = 24


class CollisionCheckError(RuntimeError):
    pass


@dataclass(frozen=True)
class StageSpec:
    gkind: int
    name: str
    root_name: str
    root_id: int
    header_source: str
    geometry_source: str
    geometry_id: int


STAGES = (
    StageSpec(0, "Peach's Castle", "GRCastleMap", 0x103,
              "259_GRCastleMap.c", "106_StageCastleFile2.c", 106),
    StageSpec(1, "Sector Z", "GRSectorMap", 0x106,
              "262_GRSectorMap.c", "109_StageSectorFile2.c", 109),
    StageSpec(2, "Congo Jungle", "GRJungleMap", 0x105,
              "261_GRJungleMap.c", "108_StageJungleFile2.c", 108),
    StageSpec(3, "Planet Zebes", "GRZebesMap", 0x101,
              "257_GRZebesMap.c", "105_StageZebesFile2.c", 105),
    StageSpec(4, "Hyrule Castle", "GRHyruleMap", 0x109,
              "265_GRHyruleMap.c", "113_StageHyruleFile2.c", 113),
    StageSpec(5, "Yoshi's Island", "GRYosterMap", 0x107,
              "263_GRYosterMap.c", "111_StageYosterFile2.c", 111),
    StageSpec(6, "Dream Land", "GRPupupuMap", 0x0FF,
              "255_GRPupupuMap.c", "104_StagePupupuFile2.c", 104),
    StageSpec(7, "Saffron City", "GRYamabukiMap", 0x108,
              "264_GRYamabukiMap.c", "112_StageYamabukiFile2.c", 112),
    StageSpec(8, "Mushroom Kingdom", "GRInishieMap", 0x104,
              "260_GRInishieMap.c", "107_StageInishieFile2.c", 107),
)


@dataclass(frozen=True)
class PointerRef:
    file_id: int
    offset: int


class O2RResource:
    """Minimal full RELO decoder matching the port's internal/external chains."""

    def __init__(self, path: Path) -> None:
        self.path = path
        try:
            raw = path.read_bytes()
        except OSError as exc:
            raise CollisionCheckError(f"missing O2R resource {path}: {exc}") from exc
        if len(raw) < 0x50 or raw[4:8] != b"OLER":
            raise CollisionCheckError(f"{path}: invalid OLER container")
        self.file_id, internal_head, external_head, extern_count = struct.unpack_from(
            "<IHHI", raw, 0x40
        )
        ids_at = 0x4C
        ids_end = ids_at + extern_count * 2
        if ids_end + 4 > len(raw):
            raise CollisionCheckError(f"{path}: truncated O2R extern table")
        self.extern_ids = list(struct.unpack_from(
            f"<{extern_count}H", raw, ids_at
        )) if extern_count else []
        data_size = struct.unpack_from("<I", raw, ids_end)[0]
        data_at = ids_end + 4
        if data_at + data_size != len(raw):
            raise CollisionCheckError(
                f"{path}: payload bytes {len(raw) - data_at} != declared {data_size}"
            )
        self.payload = raw[data_at:]
        self.pointers: dict[int, PointerRef] = {}
        self._walk_chain(internal_head, None)
        self._walk_chain(external_head, self.extern_ids)

    def _walk_chain(self, head: int, dependencies: Sequence[int] | None) -> None:
        cursor = head
        dependency_index = 0
        guard = len(self.payload) // 4 + 1
        while cursor != 0xFFFF:
            slot = cursor * 4
            if guard == 0 or slot + 4 > len(self.payload) or slot in self.pointers:
                raise CollisionCheckError(f"{self.path}: malformed RELO chain at 0x{slot:x}")
            guard -= 1
            word = self.u32(slot)
            target_file = self.file_id
            if dependencies is not None:
                if dependency_index >= len(dependencies):
                    raise CollisionCheckError(
                        f"{self.path}: external RELO chain exceeds dependency table"
                    )
                target_file = dependencies[dependency_index]
                dependency_index += 1
            self.pointers[slot] = PointerRef(target_file, (word & 0xFFFF) * 4)
            cursor = word >> 16
        if dependencies is not None and dependency_index != len(dependencies):
            raise CollisionCheckError(
                f"{self.path}: external RELO chain used {dependency_index} IDs, "
                f"header declares {len(dependencies)}"
            )

    def _need(self, offset: int, size: int, what: str) -> None:
        if offset < 0 or size < 0 or offset + size > len(self.payload):
            raise CollisionCheckError(
                f"{self.path}: {what} range 0x{offset:x}+0x{size:x} outside "
                f"payload 0x{len(self.payload):x}"
            )

    def bytes(self, offset: int, size: int, what: str) -> bytes:
        self._need(offset, size, what)
        return self.payload[offset:offset + size]

    def u8(self, offset: int) -> int:
        self._need(offset, 1, "u8")
        return self.payload[offset]

    def u16(self, offset: int) -> int:
        self._need(offset, 2, "u16")
        return struct.unpack_from(">H", self.payload, offset)[0]

    def s16(self, offset: int) -> int:
        self._need(offset, 2, "s16")
        return struct.unpack_from(">h", self.payload, offset)[0]

    def u32(self, offset: int) -> int:
        self._need(offset, 4, "u32")
        return struct.unpack_from(">I", self.payload, offset)[0]

    def pointer(self, offset: int, what: str) -> PointerRef:
        ref = self.pointers.get(offset)
        if ref is None:
            raw = self.u32(offset)
            if raw == 0:
                raise CollisionCheckError(f"{self.path}: {what} at 0x{offset:x} is NULL")
            raise CollisionCheckError(
                f"{self.path}: {what} at 0x{offset:x} is not in a RELO chain "
                f"(raw 0x{raw:08x})"
            )
        return ref


@dataclass(frozen=True)
class HeaderData:
    geometry_symbol: str | None
    geometry_ref: PointerRef | None
    layer_mask: int
    light_angle_bits: tuple[int, int, int]
    camera_bounds: tuple[int, int, int, int]
    map_bounds: tuple[int, int, int, int]
    bgm_id: int
    item_weights: tuple[int, ...]
    alt_warning: int
    team_bounds: tuple[int, ...]


Vertex = tuple[int, int, int]
Link = tuple[int, int]
LineInfo = tuple[int, ...]
MapObj = tuple[int, int, int]


@dataclass(frozen=True)
class GeometryData:
    symbol: str | None
    yakumono_count: int
    vertices: tuple[Vertex, ...]
    vertex_ids: tuple[int, ...]
    vertex_links: tuple[Link, ...]
    line_infos: tuple[LineInfo, ...]
    mapobjs: tuple[MapObj, ...]


@dataclass(frozen=True)
class SourceStage:
    header: HeaderData
    geometry: GeometryData


@dataclass(frozen=True)
class Segment:
    kind: int
    first: Vertex
    last: Vertex
    vertex_count: int


def preprocess_region_us(text: str) -> str:
    """Resolve the REGION_US/REGION_JP conditionals used by these source files."""
    out: list[str] = []
    # (parent_active, condition_was_true)
    stack: list[tuple[bool, bool]] = []
    active = True
    for line in text.splitlines(keepends=True):
        m = re.match(r"\s*#if\s+defined\s*\(\s*REGION_(US|JP)\s*\)\s*$", line)
        if not m:
            m = re.match(r"\s*#ifdef\s+REGION_(US|JP)\s*$", line)
        if m:
            cond = m.group(1) == "US"
            stack.append((active, cond))
            active = active and cond
            continue
        if re.match(r"\s*#else\s*$", line) and stack:
            parent, cond = stack[-1]
            stack[-1] = (parent, not cond)
            active = parent and not cond
            continue
        if re.match(r"\s*#endif\s*$", line) and stack:
            parent, _cond = stack.pop()
            active = parent
            continue
        if active:
            out.append(line)
    if stack:
        raise CollisionCheckError("unterminated REGION_US/REGION_JP conditional")
    return "".join(out)


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def brace_body(text: str, brace: int, what: str) -> tuple[str, int]:
    if brace < 0 or brace >= len(text) or text[brace] != "{":
        raise CollisionCheckError(f"{what}: initializer opening brace missing")
    depth = 0
    quote: str | None = None
    escaped = False
    for i in range(brace, len(text)):
        ch = text[i]
        if quote is not None:
            if escaped:
                escaped = False
            elif ch == "\\":
                escaped = True
            elif ch == quote:
                quote = None
            continue
        if ch in ('"', "'"):
            quote = ch
        elif ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1:i], i + 1
    raise CollisionCheckError(f"{what}: unterminated initializer")


def split_top_level(body: str) -> list[str]:
    parts: list[str] = []
    start = 0
    curly = paren = square = 0
    quote: str | None = None
    escaped = False
    for i, ch in enumerate(body):
        if quote is not None:
            if escaped:
                escaped = False
            elif ch == "\\":
                escaped = True
            elif ch == quote:
                quote = None
            continue
        if ch in ('"', "'"):
            quote = ch
        elif ch == "{":
            curly += 1
        elif ch == "}":
            curly -= 1
        elif ch == "(":
            paren += 1
        elif ch == ")":
            paren -= 1
        elif ch == "[":
            square += 1
        elif ch == "]":
            square -= 1
        elif ch == "," and curly == 0 and paren == 0 and square == 0:
            part = body[start:i].strip()
            if part:
                parts.append(part)
            start = i + 1
    tail = body[start:].strip()
    if tail:
        parts.append(tail)
    return parts


def find_initializer(text: str, type_name: str, symbol: str | None = None) -> tuple[str, str]:
    name = re.escape(symbol) if symbol is not None else r"(?P<name>[A-Za-z_]\w*)"
    pattern = re.compile(rf"\b{re.escape(type_name)}\s+{name}\s*=\s*\{{")
    matches = list(pattern.finditer(text))
    if len(matches) != 1:
        label = symbol or type_name
        raise CollisionCheckError(f"source: expected one initializer for {label}, found {len(matches)}")
    match = matches[0]
    found_name = symbol if symbol is not None else match.group("name")
    body, _ = brace_body(text, match.end() - 1, found_name)
    return found_name, body


def find_array(text: str, type_name: str, symbol: str) -> tuple[int | None, str]:
    pattern = re.compile(
        rf"\b{re.escape(type_name)}\s+{re.escape(symbol)}\s*"
        rf"(?:\[\s*(?P<count>\d+)\s*\])?\s*=\s*\{{"
    )
    matches = list(pattern.finditer(text))
    if len(matches) != 1:
        raise CollisionCheckError(
            f"source: expected one {type_name} array/object {symbol}, found {len(matches)}"
        )
    match = matches[0]
    body, _ = brace_body(text, match.end() - 1, symbol)
    count = int(match.group("count")) if match.group("count") else None
    return count, body


def parse_int(expr: str, what: str) -> int:
    token = expr.strip()
    m = re.fullmatch(r"([+-]?(?:0[xX][0-9a-fA-F]+|\d+))[uUlL]*", token)
    if not m:
        raise CollisionCheckError(f"source: {what} is not an integer literal: {expr!r}")
    return int(m.group(1), 0)


def all_ints(body: str) -> list[int]:
    tokens = re.findall(
        r"(?<![A-Za-z0-9_.])([+-]?(?:0[xX][0-9a-fA-F]+|\d+))[uUlL]*",
        body,
    )
    return [int(token, 0) for token in tokens]


def as_s16(value: int) -> int:
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


def symbol_from_expr(expr: str, what: str) -> str:
    names = re.findall(r"\b[A-Za-z_]\w*\b", expr)
    data_names = [name for name in names if name.startswith("d")]
    if not data_names:
        raise CollisionCheckError(f"source: cannot resolve symbol for {what}: {expr!r}")
    return data_names[-1]


def final_hex_offset(symbol: str) -> int | None:
    matches = re.findall(r"_0x([0-9a-fA-F]+)", symbol)
    return int(matches[-1], 16) if matches else None


def float32_bits(expr: str, what: str) -> int:
    token = expr.strip()
    token = re.sub(r"[fFlL]$", "", token)
    try:
        value = float(token)
        return struct.unpack(">I", struct.pack(">f", value))[0]
    except (ValueError, OverflowError) as exc:
        raise CollisionCheckError(f"source: {what} is not a finite binary32 literal: {expr!r}") from exc


def parse_bgm_ids(root: Path) -> dict[str, int]:
    path = root / "decomp/BattleShip-main/decomp/src/gm/gmsound.h"
    text = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
    match = re.search(r"\btypedef\s+enum\s+gmMusicID\s*\{", text)
    if not match:
        raise CollisionCheckError(f"{path}: gmMusicID enum not found")
    body, _ = brace_body(text, match.end() - 1, "gmMusicID")
    result: dict[str, int] = {}
    current = 0
    for field in split_top_level(body):
        field = field.strip()
        if not field:
            continue
        if "=" in field:
            name, rhs = field.split("=", 1)
            current = parse_int(rhs, f"gmMusicID {name.strip()}")
        else:
            name = field
        name = name.strip()
        if not re.fullmatch(r"[A-Za-z_]\w*", name):
            raise CollisionCheckError(f"{path}: malformed gmMusicID entry {field!r}")
        result[name] = current
        current += 1
    return result


def parse_item_weights(header_text: str, symbol: str) -> tuple[int, ...]:
    # Eight maps use MPItemWeights; Sector Z's source deliberately uses a raw
    # u8[20] and casts its address in MPGroundData.  Both are the same live
    # twenty-byte field class.
    for type_name in ("MPItemWeights", "u8"):
        try:
            declared, body = find_array(header_text, type_name, symbol)
            values = tuple(value & 0xFF for value in all_ints(body))
            if declared is not None and declared != len(values):
                raise CollisionCheckError(
                    f"source: {symbol} declares {declared} entries, parsed {len(values)}"
                )
            if len(values) != 20:
                raise CollisionCheckError(
                    f"source: {symbol} has {len(values)} item weights, expected 20"
                )
            return values
        except CollisionCheckError as exc:
            if "found 0" not in str(exc):
                raise
    raise CollisionCheckError(f"source: item-weight symbol {symbol} is not MPItemWeights or u8[20]")


def parse_source_header(path: Path, bgm_ids: dict[str, int]) -> HeaderData:
    text = preprocess_region_us(path.read_text(encoding="utf-8", errors="replace"))
    clean = strip_comments(text)
    _name, body = find_initializer(clean, "MPGroundData")
    fields = split_top_level(body)
    if len(fields) != 31:
        raise CollisionCheckError(
            f"{path}: MPGroundData parsed {len(fields)} top-level fields, expected 31"
        )
    geometry_symbol = symbol_from_expr(fields[1], "MPGroundData.map_geometry")
    layer_mask = parse_int(fields[2], "layer_mask") & 0xFF
    light_body = fields[8].strip()
    if not (light_body.startswith("{") and light_body.endswith("}")):
        raise CollisionCheckError(f"{path}: light_angle is not a brace initializer")
    light_fields = split_top_level(light_body[1:-1])
    if len(light_fields) != 3:
        raise CollisionCheckError(f"{path}: light_angle has {len(light_fields)} values")
    light_bits = tuple(float32_bits(v, "light_angle") for v in light_fields)
    camera = tuple(as_s16(parse_int(v, "camera bound")) for v in fields[9:13])
    map_bounds = tuple(as_s16(parse_int(v, "map bound")) for v in fields[13:17])
    bgm_name = fields[17].strip()
    if bgm_name not in bgm_ids:
        raise CollisionCheckError(f"{path}: unknown gmMusicID {bgm_name!r}")
    item_symbol = symbol_from_expr(fields[19], "MPGroundData.item_weights")
    item_weights = parse_item_weights(clean, item_symbol)
    alt_warning = as_s16(parse_int(fields[20], "alt_warning"))
    team = tuple(as_s16(parse_int(v, "team bound")) for v in fields[21:29])
    return HeaderData(
        geometry_symbol=geometry_symbol,
        geometry_ref=None,
        layer_mask=layer_mask,
        light_angle_bits=light_bits,  # type: ignore[arg-type]
        camera_bounds=camera,  # type: ignore[arg-type]
        map_bounds=map_bounds,  # type: ignore[arg-type]
        bgm_id=bgm_ids[bgm_name],
        item_weights=item_weights,
        alt_warning=alt_warning,
        team_bounds=team,
    )


def parse_source_geometry(path: Path) -> GeometryData:
    text = preprocess_region_us(path.read_text(encoding="utf-8", errors="replace"))
    clean = strip_comments(text)
    geom_symbol, body = find_initializer(clean, "MPGeometryData")
    fields = split_top_level(body)
    if len(fields) != 7:
        raise CollisionCheckError(
            f"{path}: MPGeometryData parsed {len(fields)} fields, expected 7"
        )
    yakumono_count = parse_int(fields[0], "yakumono_count") & 0xFFFF
    vertex_symbol = symbol_from_expr(fields[1], "vertex_data")
    ids_symbol = symbol_from_expr(fields[2], "vertex_id")
    links_symbol = symbol_from_expr(fields[3], "vertex_links")
    line_info_symbol = symbol_from_expr(fields[4], "line_info")
    mapobj_count = parse_int(fields[5], "mapobj_count") & 0xFFFF
    mapobjs_symbol = symbol_from_expr(fields[6], "mapobjs")

    vertex_count, vertex_body = find_array(clean, "MPVertexData", vertex_symbol)
    ids_count, ids_body = find_array(clean, "u16", ids_symbol)
    links_count, links_body = find_array(clean, "MPVertexLinks", links_symbol)
    line_count, line_body = find_array(clean, "MPLineInfo", line_info_symbol)
    mapobjs_count, mapobjs_body = find_array(clean, "MPMapObjData", mapobjs_symbol)
    if None in (vertex_count, ids_count, links_count, line_count, mapobjs_count):
        raise CollisionCheckError(f"{path}: collision table is missing an explicit array size")

    vertex_values = all_ints(vertex_body)
    ids_values = all_ints(ids_body)
    links_values = all_ints(links_body)
    line_values = all_ints(line_body)
    mapobj_values = all_ints(mapobjs_body)
    expected_lengths = (
        ("vertices", vertex_count * 3, len(vertex_values)),
        ("vertex_ids", ids_count, len(ids_values)),
        ("vertex_links", links_count * 2, len(links_values)),
        ("line_infos", line_count * 9, len(line_values)),
        ("mapobjs", mapobjs_count * 3, len(mapobj_values)),
    )
    for what, expected, actual in expected_lengths:
        if expected != actual:
            raise CollisionCheckError(
                f"{path}: {what} initializer has {actual} scalars, expected {expected}"
            )
    if yakumono_count != line_count:
        raise CollisionCheckError(
            f"{path}: yakumono_count {yakumono_count} != MPLineInfo count {line_count}"
        )
    if mapobj_count != mapobjs_count:
        raise CollisionCheckError(
            f"{path}: mapobj_count {mapobj_count} != MPMapObjData count {mapobjs_count}"
        )

    vertices = tuple(
        (as_s16(vertex_values[i]), as_s16(vertex_values[i + 1]), vertex_values[i + 2] & 0xFFFF)
        for i in range(0, len(vertex_values), 3)
    )
    vertex_ids = tuple(value & 0xFFFF for value in ids_values)
    links = tuple(
        (links_values[i] & 0xFFFF, links_values[i + 1] & 0xFFFF)
        for i in range(0, len(links_values), 2)
    )
    line_infos = tuple(
        tuple(value & 0xFFFF for value in line_values[i:i + 9])
        for i in range(0, len(line_values), 9)
    )
    mapobjs = tuple(
        (mapobj_values[i] & 0xFFFF, as_s16(mapobj_values[i + 1]), as_s16(mapobj_values[i + 2]))
        for i in range(0, len(mapobj_values), 3)
    )
    return GeometryData(
        symbol=geom_symbol,
        yakumono_count=yakumono_count,
        vertices=vertices,
        vertex_ids=vertex_ids,
        vertex_links=links,
        line_infos=line_infos,
        mapobjs=mapobjs,
    )


def parse_source_stage(root: Path, stage: StageSpec, bgm_ids: dict[str, int]) -> SourceStage:
    base = root / "decomp/BattleShip-main/decomp/src/relocData"
    header = parse_source_header(base / stage.header_source, bgm_ids)
    geometry = parse_source_geometry(base / stage.geometry_source)
    if header.geometry_symbol != geometry.symbol:
        raise CollisionCheckError(
            f"{stage.name}: header names geometry {header.geometry_symbol}, "
            f"geometry source defines {geometry.symbol}"
        )
    return SourceStage(header, geometry)


def table_ref(resource: O2RResource, slot: int, expected_file: int, what: str) -> PointerRef:
    ref = resource.pointer(slot, what)
    if ref.file_id != expected_file:
        raise CollisionCheckError(
            f"{resource.path}: {what} points to file 0x{ref.file_id:x}, "
            f"expected 0x{expected_file:x}"
        )
    return ref


def derive_staged_counts(
    geometry: O2RResource,
    vertex: PointerRef,
    ids: PointerRef,
    links: PointerRef,
    line_info: PointerRef,
    mapobjs: PointerRef,
) -> tuple[int, int, int, int]:
    offsets = (vertex.offset, ids.offset, links.offset, line_info.offset, mapobjs.offset)
    if list(offsets) != sorted(offsets) or len(set(offsets)) != len(offsets):
        raise CollisionCheckError(
            f"{geometry.path}: collision table pointers are not strictly ascending: "
            + ", ".join(f"0x{x:x}" for x in offsets)
        )
    vertex_span = ids.offset - vertex.offset
    vertex_count, vertex_pad = divmod(vertex_span, 6)
    if vertex_pad not in (0, 2):
        raise CollisionCheckError(
            f"{geometry.path}: vertex span 0x{vertex_span:x} leaves {vertex_pad} "
            "bytes after 6-byte MPVertexData records"
        )
    if vertex_pad:
        pad = geometry.bytes(vertex.offset + vertex_count * 6, vertex_pad, "vertex alignment pad")
        if any(pad):
            raise CollisionCheckError(
                f"{geometry.path}: nonzero vertex alignment padding {pad.hex()}"
            )
    id_span = links.offset - ids.offset
    link_span = line_info.offset - links.offset
    line_info_span = mapobjs.offset - line_info.offset
    if id_span % 2:
        raise CollisionCheckError(f"{geometry.path}: vertex-ID span is not u16 aligned")
    if link_span % 4:
        raise CollisionCheckError(f"{geometry.path}: vertex-link span is not 4-byte records")
    line_count, line_pad = divmod(line_info_span, 18)
    if line_pad not in (0, 2):
        raise CollisionCheckError(
            f"{geometry.path}: line-info span 0x{line_info_span:x} leaves {line_pad} "
            "bytes after 18-byte MPLineInfo records"
        )
    if line_pad:
        pad = geometry.bytes(line_info.offset + line_count * 18, line_pad,
                             "line-info alignment pad")
        if any(pad):
            raise CollisionCheckError(
                f"{geometry.path}: nonzero line-info alignment padding {pad.hex()}"
            )
    # Do not infer the ID table count from this physical span.  IDO pads an
    # odd number of semantically used u16 IDs with one zero u16 so the
    # following MPVertexLinks table starts on a word boundary (Castle is the
    # canonical example).  The caller derives the used count from link ranges
    # and uses id_span only to verify the expected zero alignment element.
    return vertex_count, id_span, link_span // 4, line_count


def decode_staged_geometry(
    resource: O2RResource,
    geometry_ref: PointerRef,
) -> GeometryData:
    g = geometry_ref.offset
    yakumono_count = resource.u16(g + GEOM_YAKUMONO_COUNT)
    vertex_ref = table_ref(resource, g + GEOM_VERTEX_DATA, resource.file_id, "vertex_data")
    ids_ref = table_ref(resource, g + GEOM_VERTEX_ID, resource.file_id, "vertex_id")
    links_ref = table_ref(resource, g + GEOM_VERTEX_LINKS, resource.file_id, "vertex_links")
    line_ref = table_ref(resource, g + GEOM_LINE_INFO, resource.file_id, "line_info")
    mapobjs_ref = table_ref(resource, g + GEOM_MAPOBJS, resource.file_id, "mapobjs")
    mapobj_count = resource.u16(g + GEOM_MAPOBJ_COUNT)
    vertex_count, id_span, links_count, line_count = derive_staged_counts(
        resource, vertex_ref, ids_ref, links_ref, line_ref, mapobjs_ref
    )
    vertices = tuple(
        struct.unpack_from(">hhH", resource.payload, vertex_ref.offset + i * 6)
        for i in range(vertex_count)
    )
    links = tuple(
        struct.unpack_from(">HH", resource.payload, links_ref.offset + i * 4)
        for i in range(links_count)
    )
    used_ids = max((first + count for first, count in links), default=0)
    ids_count = used_ids + (used_ids & 1)
    if id_span != ids_count * 2:
        raise CollisionCheckError(
            f"{resource.path}: vertex-ID link ranges require {used_ids} IDs "
            f"(+{ids_count - used_ids} alignment), physical span contains {id_span // 2}"
        )
    vertex_ids = tuple(resource.u16(ids_ref.offset + i * 2) for i in range(ids_count))
    if ids_count != used_ids and vertex_ids[-1] != 0:
        raise CollisionCheckError(
            f"{resource.path}: nonzero vertex-ID alignment element 0x{vertex_ids[-1]:04x}"
        )
    # Reading the raw big-endian halfwords sequentially is exactly equivalent
    # to ndsMPO2RReadU16Kernel's high-half/low-half rule after the loader's
    # blanket u32 byte swap.  Keep the 9-halfword stride; never sizeof() this.
    line_infos = tuple(
        struct.unpack_from(">9H", resource.payload, line_ref.offset + i * 18)
        for i in range(line_count)
    )
    mapobjs = tuple(
        struct.unpack_from(">Hhh", resource.payload, mapobjs_ref.offset + i * 6)
        for i in range(mapobj_count)
    )
    return GeometryData(
        symbol=None,
        yakumono_count=yakumono_count,
        vertices=vertices,
        vertex_ids=vertex_ids,
        vertex_links=links,
        line_infos=line_infos,
        mapobjs=mapobjs,
    )


def decode_staged_header(root_resource: O2RResource) -> HeaderData:
    h = GROUND_HEADER_OFFSET
    geometry_ref = root_resource.pointer(h + GROUND_MAP_GEOMETRY, "map_geometry")
    item_ref = root_resource.pointer(h + GROUND_ITEM_WEIGHTS, "item_weights")
    if item_ref.file_id != root_resource.file_id:
        raise CollisionCheckError(
            f"{root_resource.path}: item_weights unexpectedly points to external "
            f"file 0x{item_ref.file_id:x}"
        )
    light_bits = tuple(root_resource.u32(h + GROUND_LIGHT_ANGLE + i * 4) for i in range(3))
    camera = tuple(root_resource.s16(h + GROUND_CAMERA_BOUNDS + i * 2) for i in range(4))
    map_bounds = tuple(root_resource.s16(h + GROUND_MAP_BOUNDS + i * 2) for i in range(4))
    team = tuple(root_resource.s16(h + GROUND_TEAM_BOUNDS + i * 2) for i in range(8))
    weights = tuple(root_resource.u8(item_ref.offset + i) for i in range(20))
    return HeaderData(
        geometry_symbol=None,
        geometry_ref=geometry_ref,
        layer_mask=root_resource.u8(h + GROUND_LAYER_MASK),
        light_angle_bits=light_bits,  # type: ignore[arg-type]
        camera_bounds=camera,  # type: ignore[arg-type]
        map_bounds=map_bounds,  # type: ignore[arg-type]
        bgm_id=root_resource.u32(h + GROUND_BGM_ID),
        item_weights=weights,
        alt_warning=root_resource.s16(h + GROUND_ALT_WARNING),
        team_bounds=team,
    )


def external_bank_path(root: Path, file_id: int) -> Path:
    return (
        root / "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data"
        / f"ExternDataBank{file_id}"
    )


def decode_staged_stage(root: Path, stage: StageSpec) -> tuple[HeaderData, GeometryData]:
    root_path = (
        root / "decomp/BattleShip-main/BattleShip_o2r/reloc_stages" / stage.root_name
    )
    root_resource = O2RResource(root_path)
    if root_resource.file_id != stage.root_id:
        raise CollisionCheckError(
            f"{root_path}: embedded file ID 0x{root_resource.file_id:x} != "
            f"expected 0x{stage.root_id:x}"
        )
    header = decode_staged_header(root_resource)
    assert header.geometry_ref is not None
    if header.geometry_ref.file_id != stage.geometry_id:
        raise CollisionCheckError(
            f"{stage.name}: map_geometry targets file 0x{header.geometry_ref.file_id:x}, "
            f"expected 0x{stage.geometry_id:x}"
        )
    geometry_path = external_bank_path(root, header.geometry_ref.file_id)
    geometry_resource = O2RResource(geometry_path)
    if geometry_resource.file_id != header.geometry_ref.file_id:
        raise CollisionCheckError(
            f"{geometry_path}: embedded file ID 0x{geometry_resource.file_id:x} != "
            f"RELO target 0x{header.geometry_ref.file_id:x}"
        )
    geometry = decode_staged_geometry(geometry_resource, header.geometry_ref)
    return header, geometry


def line_kinds(geometry: GeometryData, who: str) -> tuple[int, ...]:
    kinds: list[int | None] = [None] * len(geometry.vertex_links)
    for info_index, info in enumerate(geometry.line_infos):
        if len(info) != 9:
            raise CollisionCheckError(f"{who}: line info {info_index} has {len(info)} halfwords")
        for kind in range(4):
            first = info[1 + kind * 2]
            count = info[2 + kind * 2]
            if first + count > len(kinds):
                raise CollisionCheckError(
                    f"{who}: line info {info_index} kind {kind} range "
                    f"{first}+{count} exceeds {len(kinds)} links"
                )
            for line_id in range(first, first + count):
                if kinds[line_id] is not None:
                    raise CollisionCheckError(
                        f"{who}: line {line_id} assigned to kinds {kinds[line_id]} and {kind}"
                    )
                kinds[line_id] = kind
    missing = [i for i, kind in enumerate(kinds) if kind is None]
    if missing:
        raise CollisionCheckError(f"{who}: links without MPLineInfo ownership: {missing[:8]}")
    return tuple(int(kind) for kind in kinds)


def expand_segments(geometry: GeometryData, who: str) -> tuple[Segment, ...]:
    kinds = line_kinds(geometry, who)
    out: list[Segment] = []
    for line_id, (first_corner, count) in enumerate(geometry.vertex_links):
        if count == 0:
            raise CollisionCheckError(f"{who}: line {line_id} has zero vertices")
        end = first_corner + count
        if end > len(geometry.vertex_ids):
            raise CollisionCheckError(
                f"{who}: line {line_id} corners {first_corner}+{count} exceed "
                f"{len(geometry.vertex_ids)} IDs"
            )
        ids = geometry.vertex_ids[first_corner:end]
        if not ids:
            raise CollisionCheckError(f"{who}: line {line_id} has no vertex IDs")
        bad = [value for value in ids if value >= len(geometry.vertices)]
        if bad:
            raise CollisionCheckError(
                f"{who}: line {line_id} references vertex {bad[0]}, "
                f"only {len(geometry.vertices)} vertices"
            )
        out.append(Segment(kinds[line_id], geometry.vertices[ids[0]], geometry.vertices[ids[-1]], count))
    return tuple(out)


def flag_line_count(geometry: GeometryData, flag: int, who: str) -> int:
    # Validate ownership and ranges through the same expansion path first.
    line_kinds(geometry, who)
    total = 0
    for line_id, (first_corner, count) in enumerate(geometry.vertex_links):
        ids = geometry.vertex_ids[first_corner:first_corner + count]
        if len(ids) != count:
            raise CollisionCheckError(f"{who}: line {line_id} has truncated ID range")
        if any(vertex_id >= len(geometry.vertices) for vertex_id in ids):
            raise CollisionCheckError(f"{who}: line {line_id} references missing vertex")
        if any(geometry.vertices[vertex_id][2] & flag for vertex_id in ids):
            total += 1
    return total


@dataclass
class Comparison:
    first_diff: str | None = None
    missing_segments: str | None = None
    differing_segments: str | None = None

    def diff(self, message: str) -> None:
        if self.first_diff is None:
            self.first_diff = message


def first_sequence_difference(
    comparison: Comparison,
    table: str,
    source: Sequence[object],
    staged: Sequence[object],
) -> None:
    shared = min(len(source), len(staged))
    for i in range(shared):
        if source[i] != staged[i]:
            comparison.diff(f"{table} index {i}: source {source[i]} vs staged {staged[i]}")
            return
    if len(source) != len(staged):
        comparison.diff(f"{table} count: source {len(source)} vs staged {len(staged)}")


def compare_stage(source: SourceStage, staged_header: HeaderData,
                  staged_geometry: GeometryData, stage: StageSpec) -> tuple[Comparison, tuple[Segment, ...], tuple[Segment, ...]]:
    result = Comparison()
    sg = source.geometry
    tg = staged_geometry
    if sg.yakumono_count != tg.yakumono_count:
        result.diff(
            f"yakumono_count: source {sg.yakumono_count} vs staged {tg.yakumono_count}"
        )
    first_sequence_difference(result, "vertices", sg.vertices, tg.vertices)
    first_sequence_difference(result, "vertex_id", sg.vertex_ids, tg.vertex_ids)
    first_sequence_difference(result, "vertex_links", sg.vertex_links, tg.vertex_links)
    first_sequence_difference(result, "line_info", sg.line_infos, tg.line_infos)
    if len(sg.mapobjs) != len(tg.mapobjs):
        result.diff(f"mapobj_count: source {len(sg.mapobjs)} vs staged {len(tg.mapobjs)}")
    first_sequence_difference(result, "mapobjs", sg.mapobjs, tg.mapobjs)

    sh = source.header
    header_fields: tuple[tuple[str, object, object], ...] = (
        ("layer_mask", sh.layer_mask, staged_header.layer_mask),
        ("camera_bound", sh.camera_bounds, staged_header.camera_bounds),
        ("map_bound", sh.map_bounds, staged_header.map_bounds),
        ("bgm_id", sh.bgm_id, staged_header.bgm_id),
        ("item_weights", sh.item_weights, staged_header.item_weights),
        ("alt_warning", sh.alt_warning, staged_header.alt_warning),
        ("team_bounds", sh.team_bounds, staged_header.team_bounds),
        ("light_angle_bits", sh.light_angle_bits, staged_header.light_angle_bits),
    )
    for name, a, b in header_fields:
        if a != b:
            result.diff(f"{name}: source {a} vs staged {b}")

    expected_offset = final_hex_offset(sg.symbol or "")
    if staged_header.geometry_ref is None:
        result.diff("map_geometry: staged pointer missing")
    else:
        if staged_header.geometry_ref.file_id != stage.geometry_id:
            result.diff(
                f"map_geometry file: source 0x{stage.geometry_id:x} vs "
                f"staged 0x{staged_header.geometry_ref.file_id:x}"
            )
        if expected_offset is not None and staged_header.geometry_ref.offset != expected_offset:
            result.diff(
                f"map_geometry offset: source 0x{expected_offset:x} vs "
                f"staged 0x{staged_header.geometry_ref.offset:x}"
            )

    source_segments = expand_segments(sg, f"{stage.name} source")
    staged_segments = expand_segments(tg, f"{stage.name} staged")
    if len(staged_segments) < len(source_segments):
        first_missing = len(staged_segments)
        result.missing_segments = (
            f"index {first_missing}: source has {len(source_segments)}, staged has "
            f"{len(staged_segments)}"
        )
        result.diff(
            f"segments count: source {len(source_segments)} vs staged {len(staged_segments)}"
        )
    elif len(staged_segments) > len(source_segments):
        result.differing_segments = (
            f"count source {len(source_segments)} vs staged {len(staged_segments)} "
            f"({len(staged_segments) - len(source_segments)} extra staged)"
        )
        result.diff(
            f"segments count: source {len(source_segments)} vs staged {len(staged_segments)}"
        )
    for i, (a, b) in enumerate(zip(source_segments, staged_segments)):
        if a != b:
            result.differing_segments = f"index {i}: source {a} vs staged {b}"
            result.diff(f"segments index {i}: source {a} vs staged {b}")
            break

    # A raw-table match and an expanded-segment mismatch cannot both be true.
    raw_tables_equal = (
        sg.vertices == tg.vertices
        and sg.vertex_ids == tg.vertex_ids
        and sg.vertex_links == tg.vertex_links
        and sg.line_infos == tg.line_infos
    )
    if raw_tables_equal and source_segments != staged_segments:
        raise CollisionCheckError(
            f"{stage.name}: raw collision tables match but expanded segments differ; decoder bug"
        )
    return result, source_segments, staged_segments


def fmt_bounds(values: Sequence[int]) -> str:
    return f"{values[0]}/{values[1]}/{values[2]}/{values[3]}"


def status(a: object, b: object) -> str:
    return "OK" if a == b else "DIFF"


def report_stage(
    stage: StageSpec,
    source: SourceStage,
    staged_header: HeaderData,
    staged_geometry: GeometryData,
    comparison: Comparison,
) -> list[str]:
    sg = source.geometry
    tg = staged_geometry
    sh = source.header
    th = staged_header
    passed = comparison.first_diff is None
    source_pass = flag_line_count(sg, PASS_FLAG, f"{stage.name} source")
    staged_pass = flag_line_count(tg, PASS_FLAG, f"{stage.name} staged")
    source_cliff = flag_line_count(sg, CLIFF_FLAG, f"{stage.name} source")
    staged_cliff = flag_line_count(tg, CLIFF_FLAG, f"{stage.name} staged")
    lines = [f"STAGE {stage.gkind} {stage.name}: {'PASS' if passed else 'FAIL'}"]
    lines.append(
        "  counts: "
        f"yakumono {sg.yakumono_count}/{tg.yakumono_count}, "
        f"verts {len(sg.vertices)}/{len(tg.vertices)}, "
        f"ids {len(sg.vertex_ids)}/{len(tg.vertex_ids)}, "
        f"links {len(sg.vertex_links)}/{len(tg.vertex_links)}, "
        f"lineinfos {len(sg.line_infos)}/{len(tg.line_infos)}, "
        f"mapobjs {len(sg.mapobjs)}/{len(tg.mapobjs)}"
    )
    if comparison.first_diff is not None:
        lines.append(f"  first_diff: {comparison.first_diff}")
    lines.append(
        f"  missing_segments: {comparison.missing_segments or 'none'}"
    )
    lines.append(
        f"  differing_segments: {comparison.differing_segments or 'none'}"
    )
    lines.append(
        f"  blast: map_bound {fmt_bounds(sh.map_bounds)} vs {fmt_bounds(th.map_bounds)} "
        f"{status(sh.map_bounds, th.map_bounds)}"
    )
    lines.append(
        f"  camera: camera_bound {fmt_bounds(sh.camera_bounds)} vs "
        f"{fmt_bounds(th.camera_bounds)} {status(sh.camera_bounds, th.camera_bounds)}"
    )
    lines.append(
        f"  alt_warning: {sh.alt_warning} vs {th.alt_warning} "
        f"{status(sh.alt_warning, th.alt_warning)}"
    )
    lines.append(
        f"  pass_lines: {source_pass} source, staged {staged_pass} "
        f"{status(source_pass, staged_pass)}"
    )
    lines.append(
        f"  cliff_lines: {source_cliff} source, staged {staged_cliff} "
        f"{status(source_cliff, staged_cliff)}"
    )
    return lines


def report_unresolvable(stage: StageSpec, source: SourceStage | None, message: str) -> list[str]:
    if source is None:
        counts = "yakumono ?/?, verts ?/?, ids ?/?, links ?/?, lineinfos ?/?, mapobjs ?/?"
        blast = camera = "?/ ?/ ?/ ?".replace(" ", "")
        alt = "?"
    else:
        g = source.geometry
        counts = (
            f"yakumono {g.yakumono_count}/?, verts {len(g.vertices)}/?, "
            f"ids {len(g.vertex_ids)}/?, links {len(g.vertex_links)}/?, "
            f"lineinfos {len(g.line_infos)}/?, mapobjs {len(g.mapobjs)}/?"
        )
        blast = fmt_bounds(source.header.map_bounds)
        camera = fmt_bounds(source.header.camera_bounds)
        alt = str(source.header.alt_warning)
    return [
        f"STAGE {stage.gkind} {stage.name}: FAIL (unstaged/unresolvable)",
        f"  counts: {counts}",
        f"  first_diff: staging: {message}",
        "  missing_segments: staged collision is unavailable/unresolvable",
        "  differing_segments: none established",
        f"  blast: map_bound {blast} vs ? DIFF",
        f"  camera: camera_bound {camera} vs ? DIFF",
        f"  alt_warning: {alt} vs ? DIFF",
        "  pass_lines: ? source/staged (staged unavailable)",
        "  cliff_lines: ? source/staged (staged unavailable)",
    ]


def run(root: Path) -> tuple[bool, list[str]]:
    bgm_ids = parse_bgm_ids(root)
    all_pass = True
    output: list[str] = []
    dream_land_pass = False
    for stage in STAGES:
        source: SourceStage | None = None
        try:
            source = parse_source_stage(root, stage, bgm_ids)
            staged_header, staged_geometry = decode_staged_stage(root, stage)
            comparison, _source_segments, _staged_segments = compare_stage(
                source, staged_header, staged_geometry, stage
            )
            stage_lines = report_stage(
                stage, source, staged_header, staged_geometry, comparison
            )
            stage_pass = comparison.first_diff is None
        except (CollisionCheckError, OSError, UnicodeError, struct.error) as exc:
            stage_lines = report_unresolvable(stage, source, str(exc))
            stage_pass = False
        if output:
            output.append("")
        output.extend(stage_lines)
        all_pass &= stage_pass
        if stage.gkind == 6:
            dream_land_pass = stage_pass
    output.append("")
    if not dream_land_pass:
        output.append("CONTROL Dream Land: FAIL (do not trust non-control failures until decoder/staging is fixed)")
        all_pass = False
    output.append(f"COLLISION_PARITY: {'PASS' if all_pass else 'FAIL'} ({len(STAGES)} VS stages)")
    return all_pass, output


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n", 1)[0])
    parser.add_argument("--repo-root", type=Path, default=DEFAULT_REPO_ROOT)
    parser.add_argument(
        "--check", action="store_true",
        help="return exit status 1 when any stage differs or is unstaged/unresolvable",
    )
    args = parser.parse_args(argv)
    root = args.repo_root.resolve()
    started = time.perf_counter()
    passed, lines = run(root)
    for line in lines:
        print(line)
    print(f"WALL_TIME: {time.perf_counter() - started:.3f}s")
    return 1 if args.check and not passed else 0


if __name__ == "__main__":
    sys.exit(main())
