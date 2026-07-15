#!/usr/bin/env python3
"""Generate the fail-closed Battle Playable texture source-block census.

This host tool answers a deliberately narrower question than the M4 runtime
gate: which BattleShip texel source blocks are statically reachable, which
belong to animated owners, which belong to Dream Land water, and which P1
owners still require a natural-runtime key census.  It does not pretend that
an asset offset is a complete ``NDSRendererHardwareTextureKey`` and therefore
cannot prove zero gameplay conversion or upload work.

The read-only BattleShip O2R resources and their typed source declarations are
pinned.  Display-list and material-table reachability is recovered from the
O2R relocation chains rather than guessed from nearby bytes.  Any source,
topology, renderer-key, or countdown-manifest drift is a falsifier.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


class Falsifier(RuntimeError):
    """A source or census assumption changed and must be reviewed."""


def falsify(message: str) -> Falsifier:
    return Falsifier(f"FALSIFIER: {message}")


def sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


@dataclass(frozen=True)
class InputSpec:
    path: str
    sha256: str
    file_id: int | None = None
    internal_fixups: int | None = None
    external_fixups: int | None = None


O2R_INPUTS = {
    "stage_images": InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank103",
        "a61e74aece06c5f15fa7cd1d6633afd9cc3750c9163caeffe59cab2d157a222a",
        103,
        0,
        0,
    ),
    "stage_geometry": InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank104",
        "3ce7e51da3810dca927521717357a2c44b1c51760bc942b0d4e5bfebe6fd4d52",
        104,
        114,
        57,
    ),
    "stage_actors": InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank152",
        "4a3557fc41fbb06ead175ea25b2dfac5373896cb473800638c7b2924b2f26b1a",
        152,
        147,
        0,
    ),
    "mario_model": InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/MarioModel",
        "be1c3b6f909b42da2a973e2fe1977cd72c046d2447f0c4afa97fe8cd5429854f",
        296,
        232,
        4,
    ),
    "fox_model": InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/FoxModel",
        "8c49ded8144d153b25101afc6c71f5e455ef43e2ddc5326c0f71a72ae740b5a5",
        313,
        286,
        4,
    ),
    "countdown": InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_interface/IFCommonGameStatus",
        "aee5419f4515b03c06985f4d697db7e002604b617c7fbd5eca9bdc8207ad4efe",
        82,
    ),
    "wallpaper": InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/StageDreamLand",
        "c11ce0dcac1ef546094875591a7e9a081b05757b86442981965aa8b51cbff11c",
        88,
    ),
}


TEXT_INPUTS = {
    "stage_images_typed": InputSpec(
        "decomp/BattleShip-main/decomp/src/relocData/103_StagePupupuImages.c",
        "cc528a63fd72c6fc97495b444333b5c458d945f3b1a534a39fd13836d4e0b236",
    ),
    "stage_geometry_typed": InputSpec(
        "decomp/BattleShip-main/decomp/src/relocData/104_StagePupupuFile2.c",
        "3608c144694eceef3639c08155e18bd6155ea91b51c95266f7f7eca2f782c845",
    ),
    "stage_actors_typed": InputSpec(
        "decomp/BattleShip-main/decomp/src/relocData/152_StagePupupuFile3.c",
        "65ea777e827f3c6fa1baaf49b13e7a7ad7e76ec919e112d1ef3c574ead447915",
    ),
    "mario_model_typed": InputSpec(
        "decomp/BattleShip-main/decomp/src/relocData/296_MarioModel.c",
        "be647560b10e9314f0779c5695b7eed3c79ed17bc1ddf8aaa6fa6dfbdf9a9ddb",
    ),
    "fox_model_typed": InputSpec(
        "decomp/BattleShip-main/decomp/src/relocData/313_FoxModel.c",
        "51ae2b044ca79680c007551fc1d8de0ebc35f09032dc27692e9a8d81ff95ead7",
    ),
    "pupupu_gameplay": InputSpec(
        "decomp/BattleShip-main/decomp/src/gr/grcommon/grpupupu.c",
        "dc9f9228e00f9de2ba82d4b3747fbabb523e29d0e431e7bcf5643877e1a5d8be",
    ),
    "reloc_symbols": InputSpec(
        "decomp/BattleShip-main/include/reloc_data.us.h",
        "8c2d5938590e9a38ca2dad6ac0fa45b4742d125ed5d89f305c38774e40551385",
    ),
}


EXPECTED_KEY_FIELDS = (
    "image",
    "image_format",
    "image_size",
    "image_width",
    "tlut_image",
    "tlut_count",
    "data_layout",
    "format",
    "size",
    "width",
    "height",
    "render_tile",
    "render_tmem",
    "render_palette",
    "render_tile_cms",
    "render_tile_cmt",
    "render_tile_masks",
    "render_tile_maskt",
    "render_tile_shifts",
    "render_tile_shiftt",
    "load_tile",
    "load_uls",
    "load_ult",
    "load_lrs",
    "load_dxt",
    "load_texels",
    "tile_uls",
    "tile_ult",
    "tile_lrs",
    "tile_lrt",
    "line",
    "flags",
    "texel1_image",
    "texel1_image_format",
    "texel1_image_size",
    "texel1_image_width",
    "texel1_load_kind",
    "texel1_render_tmem",
    "texel1_render_line",
    "texel1_render_palette",
    "texel1_render_tile_cms",
    "texel1_render_tile_cmt",
    "texel1_render_tile_masks",
    "texel1_render_tile_maskt",
    "texel1_render_tile_shifts",
    "texel1_render_tile_shiftt",
    "texel1_load_tile",
    "texel1_load_uls",
    "texel1_load_ult",
    "texel1_load_lrs",
    "texel1_load_dxt",
    "texel1_load_texels",
    "texel1_tile_uls",
    "texel1_tile_ult",
    "texel1_tile_lrs",
    "texel1_tile_lrt",
    "prim_lod_fraction",
    "combine_w0",
    "combine_w1",
)


EXPECTED_STATIC_OFFSETS = (
    0x0030,
    0x0260,
    0x0490,
    0x0540,
    0x05F0,
    0x0E20,
    0x1650,
    0x1AB0,
    0x2270,
    0x26A0,
    0x28D0,
    0x2B00,
    0x2BB0,
    0x2DE0,
    0x2E90,
    0x2F40,
)


EXPECTED_STAGE_ROOTS = {
    "pupupu_layer0": (
        0x06C0,
        0x0798,
        0x0820,
        0x08A8,
        0x0928,
        0x0950,
        0x0980,
        0x0A18,
        0x0A38,
        0x0A58,
        0x0A78,
        0x0B18,
        0x0B40,
        0x0C38,
        0x0CC0,
        0x0D20,
        0x0D80,
        0x0DE0,
        0x0E40,
        0x0EA0,
    ),
    "pupupu_layer1": (0x18E0,),
    "pupupu_layer3": (0x2970, 0x2A48, 0x2A68),
    "pupupu_water_layer": (0x21E8, 0x22C8, 0x2380),
}


EXPECTED_ACTOR_ROOTS = {
    "whispy_eyes": (0x0FF8,),
    "whispy_mouth": (0x1558, 0x1630, 0x16C0, 0x1720),
    "flowers_back": (0x2830, 0x28B0, 0x2958, 0x29A8),
    "flowers_front": (0x2EB0, 0x2F30, 0x2FD8, 0x3028, 0x30D0, 0x3120),
}


EXPECTED_CENSUS_SHA256 = (
    "4b2a16c348bdff54d397728e7bcf01f3ea3fe54e67beb931521f053df1d8a92f"
)


@dataclass(frozen=True, order=True)
class PointerRef:
    asset_id: int
    offset: int

    def key(self) -> str:
        return f"{self.asset_id:04x}:{self.offset:08x}"

    def as_json(self) -> dict[str, int | str]:
        return {
            "asset_id": self.asset_id,
            "offset": self.offset,
            "key": self.key(),
        }


@dataclass(frozen=True)
class O2RResource:
    spec: InputSpec
    source: bytes
    payload: bytes
    source_sha256: str
    payload_sha256: str
    file_id: int
    data_offset: int
    internal: dict[int, PointerRef]
    external: dict[int, PointerRef]

    def pointer_at(self, slot_offset: int) -> PointerRef | None:
        return self.internal.get(slot_offset) or self.external.get(slot_offset)


def checked_bytes(repo_root: Path, spec: InputSpec) -> bytes:
    path = repo_root / spec.path
    if not path.is_file():
        raise falsify(f"required input is absent: {spec.path}")
    payload = path.read_bytes()
    actual = sha256(payload)
    if actual != spec.sha256:
        raise falsify(f"{spec.path}: SHA256 {actual} != pinned {spec.sha256}")
    return payload


def load_text_inputs(repo_root: Path) -> dict[str, str]:
    result: dict[str, str] = {}
    for name, spec in TEXT_INPUTS.items():
        result[name] = checked_bytes(repo_root, spec).decode("utf-8")
    return result


def load_o2r(repo_root: Path, spec: InputSpec) -> O2RResource:
    source = checked_bytes(repo_root, spec)
    if len(source) < 0x50 or source[4:8] != b"OLER":
        raise falsify(f"{spec.path}: invalid O2R header")
    file_id, internal_head, external_head, extern_count = struct.unpack_from(
        "<IHHI", source, 0x40
    )
    if spec.file_id is not None and file_id != spec.file_id:
        raise falsify(
            f"{spec.path}: file ID 0x{file_id:x} != 0x{spec.file_id:x}"
        )
    extern_ids_offset = 0x4C
    extern_ids_end = extern_ids_offset + extern_count * 2
    if extern_ids_end + 4 > len(source):
        raise falsify(f"{spec.path}: truncated extern table")
    extern_ids = (
        list(struct.unpack_from(f"<{extern_count}H", source, extern_ids_offset))
        if extern_count
        else []
    )
    data_size = struct.unpack_from("<I", source, extern_ids_end)[0]
    data_offset = extern_ids_end + 4
    payload = source[data_offset:]
    if len(payload) != data_size:
        raise falsify(
            f"{spec.path}: payload bytes {len(payload)} != declared {data_size}"
        )

    def walk_chain(
        head: int, dependencies: Sequence[int] | None
    ) -> dict[int, PointerRef]:
        result: dict[int, PointerRef] = {}
        dependency_index = 0
        guard = len(payload) // 4 + 1
        cursor = head
        while cursor != 0xFFFF:
            slot = cursor * 4
            if guard == 0 or slot + 4 > len(payload) or slot in result:
                raise falsify(f"{spec.path}: malformed relocation chain")
            guard -= 1
            word = struct.unpack_from(">I", payload, slot)[0]
            if dependencies is None:
                target_file = file_id
            else:
                if dependency_index >= len(dependencies):
                    raise falsify(f"{spec.path}: extern chain exceeds file-ID table")
                target_file = dependencies[dependency_index]
                dependency_index += 1
            result[slot] = PointerRef(target_file, (word & 0xFFFF) * 4)
            cursor = word >> 16
        if dependencies is not None and dependency_index != len(dependencies):
            raise falsify(
                f"{spec.path}: extern chain has {dependency_index} fixups, "
                f"table has {len(dependencies)}"
            )
        return result

    internal = walk_chain(internal_head, None)
    external = walk_chain(external_head, extern_ids)
    if spec.internal_fixups is not None and len(internal) != spec.internal_fixups:
        raise falsify(
            f"{spec.path}: {len(internal)} internal fixups != "
            f"{spec.internal_fixups}"
        )
    if spec.external_fixups is not None and len(external) != spec.external_fixups:
        raise falsify(
            f"{spec.path}: {len(external)} external fixups != "
            f"{spec.external_fixups}"
        )
    return O2RResource(
        spec=spec,
        source=source,
        payload=payload,
        source_sha256=sha256(source),
        payload_sha256=sha256(payload),
        file_id=file_id,
        data_offset=data_offset,
        internal=internal,
        external=external,
    )


def checked_u32(payload: bytes, offset: int, context: str) -> int:
    if offset < 0 or offset + 4 > len(payload):
        raise falsify(f"{context}: u32 at 0x{offset:x} is out of range")
    return struct.unpack_from(">I", payload, offset)[0]


def dobj_display_lists(
    resource: O2RResource, root_offset: int, descriptor_count: int
) -> tuple[int, ...]:
    result: list[int] = []
    for index in range(descriptor_count):
        slot = root_offset + index * 44 + 4
        if slot + 4 > len(resource.payload):
            raise falsify(
                f"asset {resource.file_id}: DObj root 0x{root_offset:x} is truncated"
            )
        ref = resource.pointer_at(slot)
        if ref is None:
            if checked_u32(resource.payload, slot, "DObj display pointer") != 0:
                raise falsify(
                    f"asset {resource.file_id}: unresolved DObj pointer at 0x{slot:x}"
                )
            continue
        if ref.asset_id != resource.file_id:
            raise falsify(
                f"asset {resource.file_id}: external DObj display list {ref.key()}"
            )
        result.append(ref.offset)
    return tuple(result)


@dataclass(frozen=True)
class TextureLoad:
    image: PointerRef
    palette: PointerRef | None
    command_offset: int


def scan_display_lists(
    resource: O2RResource,
    roots: Sequence[int],
    allow_segmented_dl: bool = False,
) -> tuple[list[TextureLoad], list[PointerRef]]:
    loads: list[TextureLoad] = []
    images: list[PointerRef] = []

    def visit(
        start: int,
        state: dict[str, PointerRef | None],
        stack: tuple[int, ...],
    ) -> None:
        if start in stack:
            raise falsify(
                f"asset {resource.file_id}: recursive display list at 0x{start:x}"
            )
        if start < 0 or start + 8 > len(resource.payload):
            raise falsify(
                f"asset {resource.file_id}: display list 0x{start:x} is out of range"
            )
        stack += (start,)
        pc = start
        for _ in range(4096):
            w0, w1 = struct.unpack_from(">II", resource.payload, pc)
            op = w0 >> 24
            if op == 0xFD:  # G_SETTIMG
                ref = resource.pointer_at(pc + 4)
                if ref is None:
                    raise falsify(
                        f"asset {resource.file_id}: unresolved SETTIMG at 0x{pc:x} "
                        f"(raw 0x{w1:08x})"
                    )
                state["image"] = ref
                images.append(ref)
            elif op == 0xF0:  # G_LOADTLUT
                if state["image"] is None:
                    raise falsify(
                        f"asset {resource.file_id}: LOADTLUT without image at 0x{pc:x}"
                    )
                state["palette"] = state["image"]
            elif op in (0xF3, 0xF4):  # G_LOADBLOCK / G_LOADTILE
                image = state["image"]
                if image is None:
                    raise falsify(
                        f"asset {resource.file_id}: texture load without image at 0x{pc:x}"
                    )
                loads.append(TextureLoad(image, state["palette"], pc))
            elif op == 0xDE:  # G_DL
                ref = resource.pointer_at(pc + 4)
                if (
                    ref is None
                    and allow_segmented_dl
                    and (w1 & 0xFF000000) == 0x0E000000
                ):
                    # Material display lists supplied through runtime segment
                    # 0x0e are intentionally outside this source-DL traversal.
                    # This exception is used only for the separately classified
                    # water owner; it is never admitted to the static census.
                    if w0 & 0x00010000:
                        return
                elif ref is None or ref.asset_id != resource.file_id:
                    raise falsify(
                        f"asset {resource.file_id}: unresolved/external G_DL at "
                        f"0x{pc:x} (raw 0x{w1:08x})"
                    )
                else:
                    visit(ref.offset, state, stack)
                    if w0 & 0x00010000:  # branch list rather than call
                        return
            elif op == 0xDF:  # G_ENDDL
                return
            pc += 8
            if pc + 8 > len(resource.payload):
                raise falsify(
                    f"asset {resource.file_id}: unterminated display list 0x{start:x}"
                )
        raise falsify(
            f"asset {resource.file_id}: display-list guard expired at 0x{start:x}"
        )

    for root in roots:
        visit(root, {"image": None, "palette": None}, ())
    return loads, images


def pointer_table(
    resource: O2RResource,
    offset: int,
    expected: Sequence[int | None],
    target_asset: int | None = None,
) -> list[PointerRef | None]:
    target_asset = resource.file_id if target_asset is None else target_asset
    actual: list[PointerRef | None] = []
    for index, expected_offset in enumerate(expected):
        slot = offset + index * 4
        ref = resource.pointer_at(slot)
        if expected_offset is None:
            if ref is not None or checked_u32(resource.payload, slot, "pointer table") != 0:
                raise falsify(
                    f"asset {resource.file_id}: expected NULL pointer at 0x{slot:x}"
                )
            actual.append(None)
            continue
        wanted = PointerRef(target_asset, expected_offset)
        if ref != wanted:
            raise falsify(
                f"asset {resource.file_id}: pointer 0x{slot:x} is "
                f"{ref.key() if ref else 'NULL'}, expected {wanted.key()}"
            )
        actual.append(ref)
    return actual


@dataclass(frozen=True)
class TexDeclaration:
    symbol: str
    offset: int
    format: str
    width: int
    height: int
    palette_symbol: str | None
    source_bytes: int


TEX_DECL_RE = re.compile(
    r"/\*\s*@tex\s+fmt=(\w+)\s+dim=(\d+)x(\d+)"
    r"(?:\s+lut=([^\s*]+))?\s*\*/\s*"
    r"(u8|u16)\s+(\w+)\[(\d+)\]",
    re.MULTILINE,
)


def symbol_offset(symbol: str) -> int:
    matches = re.findall(r"0x([0-9A-Fa-f]+)", symbol)
    if not matches:
        raise falsify(f"texture symbol has no source offset: {symbol}")
    return int(matches[-1], 16)


def parse_tex_declarations(source: str, context: str) -> dict[int, TexDeclaration]:
    result: dict[int, TexDeclaration] = {}
    for match in TEX_DECL_RE.finditer(source):
        fmt, width, height, palette, kind, symbol, elements = match.groups()
        offset = symbol_offset(symbol)
        declaration = TexDeclaration(
            symbol=symbol,
            offset=offset,
            format=fmt,
            width=int(width),
            height=int(height),
            palette_symbol=palette or None,
            source_bytes=int(elements) * (2 if kind == "u16" else 1),
        )
        if offset in result:
            raise falsify(f"{context}: duplicate @tex offset 0x{offset:x}")
        result[offset] = declaration
    if not result:
        raise falsify(f"{context}: no typed @tex declarations")
    return result


def parse_array(
    source: str, symbol: str, expected_kind: str, expected_elements: int
) -> int:
    match = re.search(
        rf"\b(u8|u16)\s+{re.escape(symbol)}\[(\d+)\]\s*=", source
    )
    if match is None:
        raise falsify(f"missing typed array {symbol}")
    kind, elements = match.groups()
    if kind != expected_kind or int(elements) != expected_elements:
        raise falsify(
            f"{symbol}: {kind}[{elements}] != "
            f"{expected_kind}[{expected_elements}]"
        )
    return int(elements) * (2 if kind == "u16" else 1)


def make_block(
    resource: O2RResource,
    offset: int,
    source_bytes: int,
    symbol: str,
    fmt: str,
    owners: Sequence[str],
    palette: PointerRef | None = None,
    dimensions: tuple[int, int] | None = None,
    note: str | None = None,
) -> dict[str, object]:
    if offset < 0 or offset + source_bytes > len(resource.payload):
        raise falsify(
            f"{symbol}: source range 0x{offset:x}+{source_bytes} exceeds "
            f"asset {resource.file_id}"
        )
    record: dict[str, object] = {
        "identity": PointerRef(resource.file_id, offset).as_json(),
        "symbol": symbol,
        "format": fmt,
        "source_bytes": source_bytes,
        "source_sha256": sha256(resource.payload[offset : offset + source_bytes]),
        "owners": sorted(set(owners)),
        "palette": palette.as_json() if palette else None,
        "declared_dimensions": list(dimensions) if dimensions else None,
        "declared_rgba5551_bytes": (
            dimensions[0] * dimensions[1] * 2 if dimensions else None
        ),
        "full_ci4_allocation_decode_upper_bound_bytes": (
            source_bytes * 4 if fmt.startswith("CI4") else None
        ),
    }
    if note:
        record["note"] = note
    return record


def declaration_block(
    resource: O2RResource,
    declaration: TexDeclaration,
    owners: Sequence[str],
    palette: PointerRef | None = None,
    note: str | None = None,
) -> dict[str, object]:
    return make_block(
        resource,
        declaration.offset,
        declaration.source_bytes,
        declaration.symbol,
        declaration.format,
        owners,
        palette,
        (declaration.width, declaration.height),
        note,
    )


def parse_macro_offsets(source: str, expected: dict[str, int]) -> dict[str, int]:
    result: dict[str, int] = {}
    for name, wanted in expected.items():
        match = re.search(
            rf"^#define\s+{re.escape(name)}\s+\(\(intptr_t\)0x([0-9A-Fa-f]+)\)",
            source,
            re.MULTILINE,
        )
        if match is None:
            raise falsify(f"reloc symbol is absent: {name}")
        actual = int(match.group(1), 16)
        if actual != wanted:
            raise falsify(f"{name}: 0x{actual:x} != 0x{wanted:x}")
        result[name] = actual
    return result


def parse_renderer_contract(repo_root: Path) -> dict[str, object]:
    path = repo_root / "src/nds/nds_renderer.c"
    if not path.is_file():
        raise falsify("src/nds/nds_renderer.c is absent")
    source = path.read_text(encoding="utf-8")
    match = re.search(
        r"typedef struct NDSRendererHardwareTextureKey\s*\{(.*?)\} "
        r"NDSRendererHardwareTextureKey;",
        source,
        re.DOTALL,
    )
    if match is None:
        raise falsify("NDSRendererHardwareTextureKey declaration is absent")
    fields = tuple(re.findall(r"\bu32\s+(\w+)\s*;", match.group(1)))
    if fields != EXPECTED_KEY_FIELDS:
        raise falsify(
            "NDSRendererHardwareTextureKey fields changed; regenerate only after "
            "reviewing the exact key semantics"
        )
    required_tokens = (
        "_Static_assert(sizeof(NDSRendererHardwareTextureKey) == 236u",
        "return (memcmp(a, b, sizeof(*a)) == 0) ? TRUE : FALSE;",
        "#define NDS_RENDERER_HW_TEXTURE_CACHE_COUNT 48u",
        "u32 key_hash;",
    )
    for token in required_tokens:
        if token not in source:
            raise falsify(f"renderer texture contract token is absent: {token}")
    return {
        "key_words": len(fields),
        "key_bytes": len(fields) * 4,
        "fields": list(fields),
        "pointer_identity_fields": ["image", "tlut_image", "texel1_image"],
        "equality": "memcmp over all 236 bytes",
        "current_cache_entries": 48,
        "cache_entry_bytes_profile_lt2": 280,
        "cache_entry_bytes_profile_ge2": 276,
        "source_block_census_is_complete_key_census": False,
    }


def immediate_child_braces(source: str, outer_open: int) -> list[str]:
    level = 0
    entry_start: int | None = None
    result: list[str] = []
    for index in range(outer_open, len(source)):
        char = source[index]
        if char == "{":
            if level == 1:
                entry_start = index
            level += 1
        elif char == "}":
            if level == 2 and entry_start is not None:
                result.append(source[entry_start : index + 1])
                entry_start = None
            level -= 1
            if level == 0:
                return result
    raise falsify("unterminated countdown asset initializer")


def c_integer(token: str) -> int:
    token = token.strip().rstrip("uUlL")
    return int(token, 0)


def parse_countdown_oam(repo_root: Path, countdown: O2RResource) -> dict[str, object]:
    path = repo_root / "src/nds/nds_ifcommon_oam.c"
    if not path.is_file():
        raise falsify("src/nds/nds_ifcommon_oam.c is absent")
    source = path.read_text(encoding="utf-8")
    anchor = source.find("static const NDSIFCommonAssetSpec sNdsIFCommonAssetSpecs")
    if anchor < 0:
        raise falsify("native countdown asset table is absent")
    equals = source.find("=", anchor)
    outer_open = source.find("{", equals)
    entries = immediate_child_braces(source, outer_open)
    names = (
        "go_g",
        "go_o",
        "go_exclaim",
        "traffic_rod",
        "traffic_frame",
        "shadow_initial",
        "shadow_go",
        "red_dim",
        "yellow_dim",
        "blue_dim",
        "red_light",
        "yellow_light",
        "blue_light",
        "red_contour",
        "yellow_contour",
        "blue_contour",
    )
    if len(entries) != len(names):
        raise falsify(f"countdown assets {len(entries)} != {len(names)}")
    records: list[dict[str, object]] = []
    total_tiles = 0
    total_bytes = 0
    for name, entry in zip(names, entries):
        nested = entry.find("{", 1)
        if nested < 0:
            raise falsify(f"countdown {name}: tile initializer is absent")
        header = [part.strip() for part in entry[1:nested].split(",") if part.strip()]
        if len(header) != 8:
            raise falsify(f"countdown {name}: malformed asset header")
        values = [c_integer(value) for value in header]
        offset, *colors, tile_count = values
        if offset >= len(countdown.payload):
            raise falsify(f"countdown {name}: Sprite offset 0x{offset:x} is out of range")
        tiles: list[list[int]] = []
        for tile_match in re.finditer(r"TILE\(([^)]*)\)", entry):
            tile = [c_integer(value) for value in tile_match.group(1).split(",")]
            if len(tile) != 8:
                raise falsify(f"countdown {name}: malformed TILE")
            if any(tile):
                tiles.append(tile)
        if len(tiles) != tile_count:
            raise falsify(
                f"countdown {name}: {len(tiles)} concrete tiles != {tile_count}"
            )
        vram_bytes = sum(tile[4] * tile[5] * 2 for tile in tiles)
        total_tiles += tile_count
        total_bytes += vram_bytes
        records.append(
            {
                "name": name,
                "sprite_offset": offset,
                "runtime_rgb": colors[:3],
                "runtime_env_rgb": colors[3:],
                "tile_count": tile_count,
                "native_obj_vram_bytes": vram_bytes,
                "tiles": [
                    {
                        "source_xy": tile[:2],
                        "content_wh": tile[2:4],
                        "cell_wh": tile[4:6],
                        "pad_xy": tile[6:8],
                    }
                    for tile in tiles
                ],
            }
        )
    if total_tiles != 59 or total_bytes != 93824:
        raise falsify(
            f"native countdown totals {total_tiles} tiles/{total_bytes} bytes "
            "!= 59/93824"
        )
    return {
        "path": "native OAM, outside the GL texture cache",
        "asset_id": countdown.file_id,
        "logical_assets": len(records),
        "native_tiles": total_tiles,
        "native_obj_vram_bytes": total_bytes,
        "assets": records,
        "runtime_zero_hot_conversion_proven_by_host_census": False,
    }


def summarize_blocks(blocks: Sequence[dict[str, object]]) -> dict[str, int]:
    source_bytes = sum(int(block["source_bytes"]) for block in blocks)
    upper = sum(
        int(block["full_ci4_allocation_decode_upper_bound_bytes"] or 0)
        for block in blocks
    )
    declared = sum(int(block["declared_rgba5551_bytes"] or 0) for block in blocks)
    return {
        "source_block_count": len(blocks),
        "source_bytes": source_bytes,
        "declared_rgba5551_bytes_partial": declared,
        "full_ci4_allocation_decode_upper_bound_bytes": upper,
    }


def unique_load_pairs(loads: Iterable[TextureLoad]) -> dict[PointerRef, PointerRef | None]:
    result: dict[PointerRef, PointerRef | None] = {}
    for load in loads:
        previous = result.get(load.image)
        if load.image in result and previous != load.palette:
            raise falsify(
                f"texture {load.image.key()} used with multiple palettes in static scan"
            )
        result[load.image] = load.palette
    return result


def build_manifest(repo_root: Path) -> dict[str, object]:
    texts = load_text_inputs(repo_root)
    resources = {
        name: load_o2r(repo_root, spec) for name, spec in O2R_INPUTS.items()
    }
    stage_images = resources["stage_images"]
    stage_geometry = resources["stage_geometry"]
    stage_actors = resources["stage_actors"]
    mario = resources["mario_model"]
    fox = resources["fox_model"]

    renderer_contract = parse_renderer_contract(repo_root)

    reloc_expected = {
        "llGRPupupuMapMapHead": 0x10F0,
        "llGRPupupuMapWhispyEyesTransformKindsMObjSub": 0x0F00,
        "llGRPupupuMapWhispyMouthTransformKindsMObjSub": 0x13B0,
        "llGRPupupuMapWhispyMouthTransformKindsDObjDesc": 0x1770,
        "llGRPupupuMapFlowersBackTransformKindsDObjDesc": 0x2A80,
        "llGRPupupuMapFlowersFrontTransformKindsDObjDesc": 0x31F8,
        "llGRPupupuMapBrontoDObjDesc": 0x33B8,
        "llGRPupupuMapDededeDObjDesc": 0x3E80,
        "llGRPupupuMapWhispyMouthLeftOpenTexture": 0x2BE0,
        "llGRPupupuMapWhispyMouthLeftBlowTexture": 0x2C30,
        "llGRPupupuMapWhispyMouthLeftCloseTexture": 0x2C80,
        "llGRPupupuMapWhispyMouthRightOpenTexture": 0x2CD0,
        "llGRPupupuMapWhispyMouthRightBlowTexture": 0x2D20,
        "llGRPupupuMapWhispyMouthRightCloseTexture": 0x2D70,
        "llGRPupupuMapWhispyEyesLeft0Texture": 0x33E0,
        "llGRPupupuMapWhispyEyesLeft1Texture": 0x3450,
        "llGRPupupuMapWhispyEyesLeft2Texture": 0x34B0,
        "llGRPupupuMapWhispyEyesRight0Texture": 0x3510,
        "llGRPupupuMapWhispyEyesRight1Texture": 0x35C0,
        "llGRPupupuMapWhispyEyesRight2Texture": 0x3660,
        "llIFCommonGameStatusOrangeLetterGSprite": 0x4D78,
        "llIFCommonGameStatusOrangeLetterOSprite": 0xA730,
        "llIFCommonGameStatusOrangeExclamationMarkSprite": 0xC370,
        "llIFCommonGameStatusRodSprite": 0x20990,
        "llIFCommonGameStatusFrameSprite": 0x21760,
        "llIFCommonGameStatusRodShadowSprite": 0x21878,
        "llIFCommonGameStatusLampRedDimSprite": 0x21950,
        "llIFCommonGameStatusLampYellowDimSprite": 0x21A10,
        "llIFCommonGameStatusLampBlueDimSprite": 0x21BA8,
        "llIFCommonGameStatusLampRedLightSprite": 0x22128,
        "llIFCommonGameStatusLampYellowLightSprite": 0x22588,
        "llIFCommonGameStatusLampBlueLightSprite": 0x22F18,
        "llIFCommonGameStatusLampRedContourSprite": 0x23A28,
        "llIFCommonGameStatusLampYellowContourSprite": 0x24620,
        "llIFCommonGameStatusLampBlueContourSprite": 0x25290,
    }
    reloc_offsets = parse_macro_offsets(texts["reloc_symbols"], reloc_expected)
    for symbol in (
        name
        for name in reloc_expected
        if name.startswith("llGRPupupuMapWhispy")
    ):
        if f"&{symbol}" not in texts["pupupu_gameplay"]:
            raise falsify(f"Pupupu gameplay no longer references {symbol}")

    stage_root_specs = {
        "pupupu_layer0": (0x1008, 22),
        "pupupu_layer1": (0x1CE0, 3),
        "pupupu_layer3": (0x2BF8, 5),
        "pupupu_water_layer": (0x2450, 5),
    }
    stage_owner_loads: dict[str, list[TextureLoad]] = {}
    static_owners: list[dict[str, object]] = []
    for owner, (dobj_offset, count) in stage_root_specs.items():
        roots = dobj_display_lists(stage_geometry, dobj_offset, count)
        if roots != EXPECTED_STAGE_ROOTS[owner]:
            raise falsify(
                f"{owner}: display roots {[hex(value) for value in roots]} changed"
            )
        loads, _ = scan_display_lists(
            stage_geometry,
            roots,
            allow_segmented_dl=(owner == "pupupu_water_layer"),
        )
        stage_owner_loads[owner] = loads
        if owner != "pupupu_water_layer":
            static_owners.append(
                {
                    "name": owner,
                    "asset_id": stage_geometry.file_id,
                    "dobj_root_offset": dobj_offset,
                    "display_list_roots": list(roots),
                    "texture_source_keys": sorted(
                        {
                            load.image.key()
                            for load in loads
                            if load.image.asset_id == stage_images.file_id
                        }
                    ),
                }
            )

    static_loads = [
        load
        for owner, loads in stage_owner_loads.items()
        if owner != "pupupu_water_layer"
        for load in loads
    ]
    static_pairs = unique_load_pairs(static_loads)
    if tuple(sorted(ref.offset for ref in static_pairs)) != EXPECTED_STATIC_OFFSETS:
        raise falsify(
            "static Pupupu texture offsets changed: "
            + ", ".join(hex(ref.offset) for ref in sorted(static_pairs))
        )
    if any(ref.asset_id != stage_images.file_id for ref in static_pairs):
        raise falsify("static Pupupu scan escaped StagePupupuImages")

    stage_declarations = parse_tex_declarations(
        texts["stage_images_typed"], "StagePupupuImages"
    )
    manual_static = {
        0x2B00: ("dStagePupupuImages_palette_0x2B00", "u16", 64),
        0x2DE0: ("dStagePupupuImages_palette_0x2DE0", "u16", 64),
        0x2E90: ("dStagePupupuImages_palette_0x2E90", "u16", 64),
    }
    static_blocks: list[dict[str, object]] = []
    for offset in EXPECTED_STATIC_OFFSETS:
        ref = PointerRef(stage_images.file_id, offset)
        palette = static_pairs[ref]
        owners = [
            owner
            for owner, loads in stage_owner_loads.items()
            if owner != "pupupu_water_layer"
            and any(load.image == ref for load in loads)
        ]
        declaration = stage_declarations.get(offset)
        if declaration is not None:
            if declaration.palette_symbol:
                declared_palette = symbol_offset(declaration.palette_symbol)
                if palette != PointerRef(stage_images.file_id, declared_palette):
                    raise falsify(
                        f"{declaration.symbol}: display palette "
                        f"{palette.key() if palette else 'NULL'} != typed 0x{declared_palette:x}"
                    )
            static_blocks.append(
                declaration_block(stage_images, declaration, owners, palette)
            )
            continue
        if offset not in manual_static:
            raise falsify(f"static texture 0x{offset:x} has no typed source block")
        symbol, kind, elements = manual_static[offset]
        source_bytes = parse_array(
            texts["stage_images_typed"], symbol, kind, elements
        )
        static_blocks.append(
            make_block(
                stage_images,
                offset,
                source_bytes,
                symbol,
                "CI4_from_exact_display_list_load",
                owners,
                palette,
                None,
                "Typed upstream declaration retains the historical palette name; "
                "the exact CI load path proves this is a texel block. Runtime "
                "dimensions remain part of the unclosed full key.",
            )
        )

    # Water is kept wholly separate.  The exact host fixture already proves
    # the period/key/output stream; this census validates its source topology
    # but deliberately does not count its outputs as static cache entries.
    pointer_table(
        stage_geometry, 0x1F60, (0x1BE0, 0x1E10, 0x2040), stage_images.file_id
    )
    pointer_table(
        stage_geometry, 0x1F6C, (0x1BE0, 0x1E10, 0x1E10), stage_images.file_id
    )
    water_frame_specs = (
        (0x1BE0, "dStagePupupuImages_palette_0x1BE0", 276),
        (0x1E10, "dStagePupupuImages_palette_0x1E10", 276),
        (0x2040, "dStagePupupuImages_palette_0x2040", 256),
    )
    water_blocks: list[dict[str, object]] = []
    for offset, symbol, elements in water_frame_specs:
        source_bytes = parse_array(
            texts["stage_images_typed"], symbol, "u16", elements
        )
        water_blocks.append(
            make_block(
                stage_images,
                offset,
                source_bytes,
                symbol,
                "CI4_water_frame",
                ("pupupu_water_large", "pupupu_water_small"),
                PointerRef(stage_images.file_id, 0x1BB8),
                (32, 32),
                "The exact water oracle consumes 512 texel bytes from each frame; "
                "some typed arrays retain trailing source bytes.",
            )
        )
    water_support = stage_declarations[0x1880]
    water_blocks.append(
        declaration_block(
            stage_images,
            water_support,
            ("pupupu_water_layer_support",),
            PointerRef(stage_images.file_id, 0x1858),
        )
    )
    water_load_offsets = {
        load.image.offset for load in stage_owner_loads["pupupu_water_layer"]
    }
    if water_load_offsets != {0x1880, 0x1BB8}:
        raise falsify(
            "water layer display loads changed: "
            + ", ".join(hex(value) for value in sorted(water_load_offsets))
        )

    # Whispy, mouth/eyes, and flowers are finite source blocks, but gameplay
    # changes their material/animation state and thus they are not static keys.
    actor_root_specs = {
        "whispy_eyes": (0x10F0, 4),
        "whispy_mouth": (0x1770, 7),
        "flowers_back": (0x2A80, 8),
        "flowers_front": (0x31F8, 11),
    }
    actor_loads: dict[str, list[TextureLoad]] = {}
    actor_owners: list[dict[str, object]] = []
    for owner, (dobj_offset, count) in actor_root_specs.items():
        roots = dobj_display_lists(stage_actors, dobj_offset, count)
        if roots != EXPECTED_ACTOR_ROOTS[owner]:
            raise falsify(f"{owner}: actor display roots changed")
        loads, _ = scan_display_lists(
            stage_actors, roots, allow_segmented_dl=True
        )
        actor_loads[owner] = loads
        actor_owners.append(
            {
                "name": owner,
                "asset_id": stage_actors.file_id,
                "dobj_root_offset": dobj_offset,
                "display_list_roots": list(roots),
            }
        )
    pointer_table(stage_actors, 0x0F0C, (0x09C0, 0x0AF0, None))
    pointer_table(stage_actors, 0x13C8, (0x09C0, 0x0C20, 0x0AF0, 0x0D50))
    actor_declarations = parse_tex_declarations(
        texts["stage_actors_typed"], "StagePupupuFile3"
    )
    if tuple(sorted(actor_declarations)) != (
        0x0030,
        0x0460,
        0x0890,
        0x09C0,
        0x0AF0,
        0x0C20,
        0x0D50,
        0x0E80,
    ):
        raise falsify("Pupupu actor texture declarations changed")
    actor_block_owners = {
        0x0030: ("flowers_back", "flowers_front"),
        0x0460: ("flowers_back", "flowers_front"),
        0x0890: ("whispy_mouth",),
        0x09C0: ("whispy_eyes", "whispy_mouth"),
        0x0AF0: ("whispy_eyes", "whispy_mouth"),
        0x0C20: ("whispy_mouth",),
        0x0D50: ("whispy_mouth",),
        0x0E80: ("whispy_mouth",),
    }
    actor_blocks = [
        declaration_block(
            stage_actors,
            declaration,
            actor_block_owners[offset],
            (
                PointerRef(stage_actors.file_id, symbol_offset(declaration.palette_symbol))
                if declaration.palette_symbol
                else None
            ),
        )
        for offset, declaration in sorted(actor_declarations.items())
    ]
    direct_actor_texels = {
        owner: {
            load.image.offset
            for load in loads
            if load.image.offset in actor_declarations
        }
        for owner, loads in actor_loads.items()
    }
    if direct_actor_texels != {
        "whispy_eyes": set(),
        "whispy_mouth": {0x0890, 0x0E80},
        "flowers_back": {0x0030, 0x0460},
        "flowers_front": {0x0030, 0x0460},
    }:
        raise falsify(f"Pupupu actor direct texture loads changed: {direct_actor_texels}")

    # Bronto and Dedede are source-spawned Dream Land actors in efground.c.
    stage_geometry_declarations = parse_tex_declarations(
        texts["stage_geometry_typed"], "StagePupupuFile2"
    )
    if tuple(sorted(stage_geometry_declarations)) != (
        0x2D5C,
        0x2EE4,
        0x306C,
        0x3990,
    ):
        raise falsify("Bronto/Dedede texture declarations changed")
    pointer_table(stage_geometry, 0x31F8, (0x3070, 0x2EE8, 0x2D60, None))
    bronto_blocks = [
        make_block(
            stage_geometry,
            texel_offset,
            stage_geometry_declarations[declaration_offset].source_bytes - 4,
            stage_geometry_declarations[declaration_offset].symbol,
            stage_geometry_declarations[declaration_offset].format,
            ("pupupu_bronto",),
            None,
            (
                stage_geometry_declarations[declaration_offset].width,
                stage_geometry_declarations[declaration_offset].height,
            ),
            "Palette ID is material-animated across 0x2ce8/0x2d10/0x2d38; "
            "the typed container has a four-byte prefix before this exact "
            "MObj texel pointer, and the complete renderer-key variants remain "
            "unclosed.",
        )
        for declaration_offset, texel_offset in (
            (0x2D5C, 0x2D60),
            (0x2EE4, 0x2EE8),
            (0x306C, 0x3070),
        )
    ]
    dedede_blocks = [
        declaration_block(
            stage_geometry,
            stage_geometry_declarations[0x3990],
            ("pupupu_dedede",),
            PointerRef(stage_geometry.file_id, 0x3968),
        )
    ]

    # Conservatively retain every typed, relocation-referenced model texel
    # block for Mario/Fox.  Material sprite/palette animation makes these
    # finite dynamic owners rather than static renderer keys.
    fighter_specs = (
        (
            "mario_model_materials",
            mario,
            texts["mario_model_typed"],
            (0x65F0, 0x67A0, 0x69D0, 0x6C78, 0x6D68, 0x6F98),
            (2, 2, 2, 2, 2, 2),
        ),
        (
            "fox_model_materials",
            fox,
            texts["fox_model_typed"],
            (0x70D0, 0x7300, 0x75D0, 0x7700, 0x7830, 0x79D8, 0x7AE0),
            (2, 2, 2, 2, 2, 2, 4),
        ),
    )
    fighter_owners: list[dict[str, object]] = []
    fighter_blocks: list[dict[str, object]] = []
    for owner, resource, source, expected_offsets, expected_ref_counts in fighter_specs:
        declarations = parse_tex_declarations(source, owner)
        if tuple(sorted(declarations)) != expected_offsets:
            raise falsify(f"{owner}: typed texture blocks changed")
        owner_keys: list[str] = []
        for offset, expected_refs in zip(expected_offsets, expected_ref_counts):
            declaration = declarations[offset]
            references = sum(
                1
                for ref in resource.internal.values()
                if offset <= ref.offset < offset + declaration.source_bytes
            )
            if references != expected_refs:
                raise falsify(
                    f"{owner} 0x{offset:x}: {references} relocation references "
                    f"!= {expected_refs}"
                )
            block = declaration_block(
                resource,
                declaration,
                (owner,),
                None,
                "Finite BattleShip model texel block; palette/frame selection is "
                "material-animated and must be enumerated as full runtime keys.",
            )
            block["internal_reference_count"] = references
            fighter_blocks.append(block)
            owner_keys.append(PointerRef(resource.file_id, offset).key())
        fighter_owners.append(
            {
                "name": owner,
                "asset_id": resource.file_id,
                "texture_source_keys": owner_keys,
                "runtime_material_key_variants_closed": False,
            }
        )

    dynamic_blocks = actor_blocks + bronto_blocks + dedede_blocks + fighter_blocks
    dynamic_owners = (
        actor_owners
        + [
            {
                "name": "pupupu_bronto",
                "asset_id": stage_geometry.file_id,
                "dobj_root_offset": reloc_offsets["llGRPupupuMapBrontoDObjDesc"],
                "palette_offsets": [0x2CE8, 0x2D10, 0x2D38],
            },
            {
                "name": "pupupu_dedede",
                "asset_id": stage_geometry.file_id,
                "dobj_root_offset": reloc_offsets["llGRPupupuMapDededeDObjDesc"],
                "palette_offsets": [0x3968],
            },
        ]
        + fighter_owners
    )

    countdown_oam = parse_countdown_oam(repo_root, resources["countdown"])

    uncertainty_specs = (
        (
            "complete_renderer_keys",
            "src/nds/nds_renderer.c",
            "Source blocks do not enumerate all 59 key words, TEXEL1 state, "
            "tile windows, combine state, palette aliases, or material epochs.",
        ),
        (
            "mario_fireball",
            "decomp/BattleShip-main/decomp/src/wp/wpmario/wpmariofireball.c",
            "Reachable Mario weapon owner has no exact asset-ID+offset key set yet.",
        ),
        (
            "fox_blaster",
            "decomp/BattleShip-main/decomp/src/wp/wpfox/wpfoxblaster.c",
            "Reachable Fox projectile owner has no exact asset-ID+offset key set yet.",
        ),
        (
            "fox_reflector_and_common_effects",
            "decomp/BattleShip-main/decomp/src/ef/efmanager.c",
            "Reflector and attack/damage effects select additional effect assets.",
        ),
        (
            "pupupu_particle_bank",
            "decomp/BattleShip-main/decomp/src/ef/efparticle.c",
            "Whispy leaf/dust particle textures are loaded through a runtime bank.",
        ),
        (
            "fighter_shadows",
            "decomp/BattleShip-main/decomp/src/ft/ftshadow.c",
            "The natural fighter-shadow owner is not represented by model @tex blocks.",
        ),
        (
            "device_window",
            "src/port/diagnostics.c",
            "Only synchronized GO-through-teardown device counters can prove zero "
            "conversion, upload preparation, allocation, eviction, refresh, I/O, "
            "or fallback for the accepted ROM.",
        ),
    )
    runtime_uncertainty: list[dict[str, str]] = []
    for owner, path, reason in uncertainty_specs:
        if not (repo_root / path).is_file():
            raise falsify(f"runtime-uncertainty source is absent: {path}")
        runtime_uncertainty.append({"owner": owner, "source": path, "reason": reason})

    source_records = {
        name: {
            "path": resource.spec.path,
            "file_id": resource.file_id,
            "sha256": resource.source_sha256,
            "payload_sha256": resource.payload_sha256,
            "payload_bytes": len(resource.payload),
            "data_offset": resource.data_offset,
            "internal_fixups": len(resource.internal),
            "external_fixups": len(resource.external),
        }
        for name, resource in resources.items()
    }
    source_records.update(
        {
            name: {"path": spec.path, "sha256": spec.sha256}
            for name, spec in TEXT_INPUTS.items()
        }
    )

    manifest: dict[str, object] = {
        "schema": 1,
        "description": "Battle Playable exact source-block texture census",
        "scope": {
            "scene": "canonical battle_playable_realtime Mario vs Fox on Pupupu",
            "gameplay_fence": "GO through battle teardown",
            "results": "later setup/load boundary; outside this census",
            "identity": "BattleShip asset file ID + payload offset",
            "census_kind": "exact source blocks, not complete runtime renderer keys",
        },
        "qualification": {
            "source_block_census_generated": True,
            "complete_runtime_key_census_generated": False,
            "device_counter_window_observed": False,
            "gameplay_zero_conversion_proven": False,
            "m4_static_256_eligible": False,
            "m4_complete": False,
            "admission_hypothesis_69_keys_179328_bytes_validated": False,
            "blocking_reason": (
                "Runtime-only owners and all complete 59-word renderer-key variants "
                "must be closed, then synchronized device counters must pass."
            ),
        },
        "renderer_key_contract": renderer_contract,
        "sources": source_records,
        "reloc_offsets": reloc_offsets,
        "static_owners": {
            "owners": static_owners,
            "source_blocks": static_blocks,
            "summary": summarize_blocks(static_blocks),
            "runtime_key_count": None,
            "runtime_residency_bytes": None,
        },
        "dynamic_animated_owners": {
            "owners": dynamic_owners,
            "source_blocks": dynamic_blocks,
            "summary": summarize_blocks(dynamic_blocks),
            "complete_runtime_key_variants": False,
        },
        "water": {
            "classification": "separate animated representation checkpoint",
            "source_blocks": water_blocks,
            "summary": summarize_blocks(water_blocks),
            "material_palette": PointerRef(stage_images.file_id, 0x1BB8).as_json(),
            "material_offsets": {
                "large_mobj": 0x1F78,
                "small_mobj": 0x1FF0,
                "large_script": 0x2540,
                "small_script": 0x2620,
            },
            "exact_host_fixture": {
                "simulated_frames": 18000,
                "cycle_frames": 216,
                "live_keys": 322,
                "final_outputs": 206,
                "oracle_pixels": 3024896,
                "oracle_mismatches": 0,
                "tight_payload_bytes": 1560960,
                "index_bytes": 11060,
                "runtime_design_promoted": False,
            },
            "mixed_into_static_owner_count": False,
        },
        "runtime_only_uncertainty": runtime_uncertainty,
        "outside_gl_texture_path": {
            "countdown_go": countdown_oam,
            "dream_land_wallpaper": {
                "asset_id": resources["wallpaper"].file_id,
                "sprite_offset": 0x26C88,
                "path": "retained affine BG2, outside the GL texture cache",
            },
            "bottom_hud": {
                "path": "DS lower-screen text",
                "gl_texture_keys": 0,
            },
        },
        "required_device_promotion_evidence": {
            "accepted_rom": "smash64ds-battle-playable-hwtri.nds",
            "window": "256 synchronized post-GO frames for M4-static-256; GO "
            "through battle teardown for M4 completion",
            "must_latch_zero": [
                "static key misses",
                "texture conversion calls and ticks",
                "palette/decode work",
                "allocation and cache eviction",
                "GL create/upload/delete",
                "static refresh",
                "texture-path decompression or I/O",
                "fallback",
            ],
            "temporary_static_checkpoint_exception": (
                "only the two current water refreshes / 36864 bytes"
            ),
            "host_census_can_satisfy_this_evidence": False,
        },
    }

    category_keys: dict[str, set[str]] = {
        "static": {
            str(block["identity"]["key"])  # type: ignore[index]
            for block in static_blocks
        },
        "dynamic": {
            str(block["identity"]["key"])  # type: ignore[index]
            for block in dynamic_blocks
        },
        "water": {
            str(block["identity"]["key"])  # type: ignore[index]
            for block in water_blocks
        },
    }
    for left, right in (("static", "dynamic"), ("static", "water"), ("dynamic", "water")):
        overlap = category_keys[left] & category_keys[right]
        if overlap:
            raise falsify(f"{left}/{right} source-block overlap: {sorted(overlap)}")
    return manifest


def canonical_payload(manifest: dict[str, object]) -> bytes:
    return json.dumps(
        manifest, sort_keys=True, separators=(",", ":"), ensure_ascii=True
    ).encode("utf-8")


def check_manifest(manifest: dict[str, object]) -> str:
    digest = sha256(canonical_payload(manifest))
    if EXPECTED_CENSUS_SHA256 == "TO_BE_FILLED":
        raise falsify(
            f"expected census digest is not pinned; generated digest is {digest}"
        )
    if digest != EXPECTED_CENSUS_SHA256:
        raise falsify(
            f"census SHA256 {digest} != pinned {EXPECTED_CENSUS_SHA256}"
        )
    qualification = manifest["qualification"]
    assert isinstance(qualification, dict)
    forbidden_true = (
        "complete_runtime_key_census_generated",
        "device_counter_window_observed",
        "gameplay_zero_conversion_proven",
        "m4_static_256_eligible",
        "m4_complete",
        "admission_hypothesis_69_keys_179328_bytes_validated",
    )
    for field in forbidden_true:
        if qualification[field] is not False:
            raise falsify(f"fail-closed qualification field became true: {field}")
    static = manifest["static_owners"]
    dynamic = manifest["dynamic_animated_owners"]
    water = manifest["water"]
    uncertainty = manifest["runtime_only_uncertainty"]
    assert isinstance(static, dict) and isinstance(dynamic, dict)
    assert isinstance(water, dict) and isinstance(uncertainty, list)
    if static["summary"] != {
        "source_block_count": 16,
        "source_bytes": 9216,
        "declared_rgba5551_bytes_partial": 16512,
        "full_ci4_allocation_decode_upper_bound_bytes": 36864,
    }:
        raise falsify(f"static source summary changed: {static['summary']}")
    if dynamic["summary"] != {
        "source_block_count": 25,
        "source_bytes": 10760,
        "declared_rgba5551_bytes_partial": 42048,
        "full_ci4_allocation_decode_upper_bound_bytes": 43040,
    }:
        raise falsify(f"dynamic source summary changed: {dynamic['summary']}")
    if water["summary"] != {
        "source_block_count": 4,
        "source_bytes": 2128,
        "declared_rgba5551_bytes_partial": 8192,
        "full_ci4_allocation_decode_upper_bound_bytes": 8512,
    }:
        raise falsify(f"water source summary changed: {water['summary']}")
    if len(uncertainty) < 7:
        raise falsify("runtime-only uncertainty was reduced without runtime proof")
    return digest


LEGACY_GDB_TEX_RE = re.compile(
    r"^TEX i=(?P<index>\d+) name=(?P<name>\d+) "
    r"img=(?P<image>0x[0-9A-Fa-f]+) iw=(?P<image_width>\d+) "
    r"tlut=(?P<tlut>0x[0-9A-Fa-f]+) tc=(?P<tlut_count>\d+) "
    r"fmt=(?P<format>\d+) siz=(?P<size>\d+) "
    r"dim=(?P<width>\d+)x(?P<height>\d+) "
    r"tile=(?P<render_tile>\d+) tmem=(?P<render_tmem>\d+) "
    r"pal=(?P<render_palette>\d+) line=(?P<line>\d+) "
    r"flags=(?P<flags>0x[0-9A-Fa-f]+) "
    r"load=(?P<load0>\d+)/(?P<load1>\d+)/(?P<load2>\d+)/(?P<load3>\d+) "
    r"texels=(?P<texels>\d+) nonwhite=(?P<nonwhite>\d+) "
    r"green=(?P<green>\d+) prof=(?P<prepared_width>\d+)x(?P<prepared_height>\d+)$"
)


def source_keys_by_owner(manifest: dict[str, object]) -> dict[str, set[str]]:
    """Return the exact source-block identities attributable to each owner."""

    result: dict[str, set[str]] = {}
    for section_name in ("static_owners", "dynamic_animated_owners", "water"):
        section = manifest[section_name]
        if not isinstance(section, dict):
            raise falsify(f"manifest section {section_name} is not an object")
        blocks = section.get("source_blocks")
        if not isinstance(blocks, list):
            raise falsify(f"manifest section {section_name} has no source blocks")
        for block in blocks:
            if not isinstance(block, dict):
                raise falsify(f"{section_name} contains a non-object source block")
            identity = block.get("identity")
            owners = block.get("owners")
            if not isinstance(identity, dict) or not isinstance(owners, list):
                raise falsify(f"{section_name} source block lost identity/owners")
            key = identity.get("key")
            if not isinstance(key, str):
                raise falsify(f"{section_name} source block identity has no key")
            for owner in owners:
                if not isinstance(owner, str) or not owner:
                    raise falsify(f"{section_name} source block has invalid owner")
                result.setdefault(owner, set()).add(key)
    return result


def checked_positive_int(value: object, context: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
        raise falsify(f"{context} must be a positive integer")
    return value


def checked_u32_value(value: object, context: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or not 0 <= value <= 0xFFFFFFFF:
        raise falsify(f"{context} must be a 32-bit unsigned integer")
    return value


def checked_pointer_ref(value: object, context: str) -> str:
    if not isinstance(value, dict):
        raise falsify(f"{context} must be an asset-ID/offset object")
    asset_id = checked_positive_int(value.get("asset_id"), f"{context}.asset_id")
    offset = checked_u32_value(value.get("offset"), f"{context}.offset")
    expected_key = f"{asset_id}:0x{offset:x}"
    supplied_key = value.get("key")
    if supplied_key is not None and supplied_key != expected_key:
        raise falsify(
            f"{context}.key {supplied_key!r} does not match {expected_key!r}"
        )
    return expected_key


def is_power_of_two(value: int) -> bool:
    return value > 0 and (value & (value - 1)) == 0


def parse_legacy_gdb_runtime_export(path: Path, payload: str) -> dict[str, object]:
    """Parse the existing cache probe without pretending it is a full key export."""

    lines = payload.splitlines()
    tex_lines = [line.strip() for line in lines if line.startswith("TEX ")]
    malformed = [line for line in tex_lines if LEGACY_GDB_TEX_RE.fullmatch(line) is None]
    if malformed:
        raise falsify(f"{path}: malformed legacy TEX line: {malformed[0]}")
    if not tex_lines:
        raise falsify(f"{path}: legacy cache probe contains no TEX entries")
    if not any(line.startswith("CACHE ") for line in lines):
        raise falsify(f"{path}: legacy cache probe has no CACHE identity line")

    entries: list[dict[str, object]] = []
    indices: set[int] = set()
    names: set[int] = set()
    for line in tex_lines:
        match = LEGACY_GDB_TEX_RE.fullmatch(line)
        assert match is not None
        values = {
            name: int(raw, 16) if raw.startswith("0x") else int(raw)
            for name, raw in match.groupdict().items()
        }
        index = values["index"]
        name = values["name"]
        if index in indices or name in names:
            raise falsify(f"{path}: duplicate cache index/name {index}/{name}")
        indices.add(index)
        names.add(name)
        width = values["width"]
        height = values["height"]
        prepared_width = values["prepared_width"]
        prepared_height = values["prepared_height"]
        if values["texels"] != width * height:
            raise falsify(
                f"{path}: cache entry {index} texels {values['texels']} != "
                f"{width}*{height}"
            )
        if not is_power_of_two(prepared_width) or not is_power_of_two(prepared_height):
            raise falsify(f"{path}: cache entry {index} has non-power-of-two profile")
        if prepared_width < width or prepared_height < height:
            raise falsify(f"{path}: cache entry {index} profile clips logical dimensions")
        if values["nonwhite"] > values["texels"] or values["green"] > values["nonwhite"]:
            raise falsify(f"{path}: cache entry {index} pixel census is inconsistent")
        entries.append(
            {
                "owner": None,
                "source_key": None,
                "complete_key_fingerprint": None,
                "prepared_bytes": prepared_width * prepared_height * 2,
                "logical_rgba5551_bytes": width * height * 2,
                "legacy_cache_index": index,
                "legacy_cache_name": name,
                "raw_image_pointer": f"0x{values['image']:x}",
                "raw_tlut_pointer": f"0x{values['tlut']:x}",
                "partial_key_fields": {
                    field: values[field]
                    for field in (
                        "image_width",
                        "tlut_count",
                        "format",
                        "size",
                        "width",
                        "height",
                        "render_tile",
                        "render_tmem",
                        "render_palette",
                        "line",
                        "flags",
                    )
                },
                "prepared_dimensions": [prepared_width, prepared_height],
            }
        )
    if indices != set(range(len(entries))):
        raise falsify(f"{path}: legacy cache indices are not contiguous from zero")
    entries.sort(key=lambda entry: int(entry["legacy_cache_index"]))
    return {
        "path": str(path),
        "format": "legacy_gdb_cache_probe_partial",
        "renderer_profile": None,
        "harness_mode": None,
        "window_complete": False,
        "owner_complete": False,
        "key_complete": False,
        "device_counters_complete": False,
        "entries": entries,
        "limitations": [
            "owner labels are absent",
            "raw relocated pointers cannot be mapped to asset ID plus offset",
            f"only a partial subset of the {len(EXPECTED_KEY_FIELDS)}-word key is present",
            "the output does not identify profile 2, mode 163, or a complete gameplay window",
        ],
    }


def parse_canonical_runtime_export(path: Path, document: object) -> dict[str, object]:
    """Parse the strict owner-qualified profile-2 interchange format."""

    if not isinstance(document, dict):
        raise falsify(f"{path}: runtime export JSON must be an object")
    if document.get("schema") != 1 or document.get("kind") != "smash64ds-texture-key-export":
        raise falsify(f"{path}: unsupported runtime export schema/kind")
    identity = document.get("identity")
    coverage = document.get("coverage")
    raw_entries = document.get("entries")
    totals = document.get("totals")
    if not isinstance(identity, dict) or not isinstance(coverage, dict):
        raise falsify(f"{path}: runtime export lacks identity/coverage objects")
    if not isinstance(raw_entries, list) or not isinstance(totals, dict):
        raise falsify(f"{path}: runtime export lacks entries/totals")

    profile = identity.get("renderer_profile")
    harness_mode = identity.get("harness_mode")
    if profile != 2:
        raise falsify(f"{path}: renderer_profile must be 2, got {profile!r}")
    if harness_mode != 163:
        raise falsify(f"{path}: harness_mode must be 163, got {harness_mode!r}")
    coverage_fields = (
        "window_complete",
        "owner_complete",
        "key_complete",
        "device_counters_complete",
    )
    for field in coverage_fields:
        if not isinstance(coverage.get(field), bool):
            raise falsify(f"{path}: coverage.{field} must be boolean")

    entries: list[dict[str, object]] = []
    fingerprints: set[str] = set()
    for index, raw in enumerate(raw_entries):
        context = f"{path}: entries[{index}]"
        if not isinstance(raw, dict):
            raise falsify(f"{context} must be an object")
        owner = raw.get("owner")
        if not isinstance(owner, str) or not owner:
            raise falsify(f"{context}.owner must be a nonempty string")
        source_key = checked_pointer_ref(raw.get("image_ref"), f"{context}.image_ref")
        words = raw.get("key_words")
        if not isinstance(words, list) or len(words) != len(EXPECTED_KEY_FIELDS):
            raise falsify(
                f"{context}.key_words must contain exactly {len(EXPECTED_KEY_FIELDS)} words"
            )
        checked_words = [
            checked_u32_value(value, f"{context}.key_words[{word_index}]")
            for word_index, value in enumerate(words)
        ]
        prepared_width = checked_positive_int(
            raw.get("prepared_width"), f"{context}.prepared_width"
        )
        prepared_height = checked_positive_int(
            raw.get("prepared_height"), f"{context}.prepared_height"
        )
        if not is_power_of_two(prepared_width) or not is_power_of_two(prepared_height):
            raise falsify(f"{context} prepared dimensions must be powers of two")
        prepared_bytes = checked_positive_int(
            raw.get("prepared_bytes"), f"{context}.prepared_bytes"
        )
        expected_bytes = prepared_width * prepared_height * 2
        if prepared_bytes != expected_bytes:
            raise falsify(
                f"{context}.prepared_bytes {prepared_bytes} != exact RGB5A1 "
                f"profile allocation {expected_bytes}"
            )
        canonical_key = {
            "owner": owner,
            "image_ref": source_key,
            "words": checked_words,
        }
        fingerprint = sha256(
            json.dumps(canonical_key, sort_keys=True, separators=(",", ":")).encode("ascii")
        )
        if fingerprint in fingerprints:
            raise falsify(f"{context}: duplicate owner-qualified complete key")
        fingerprints.add(fingerprint)
        entries.append(
            {
                "owner": owner,
                "source_key": source_key,
                "complete_key_fingerprint": fingerprint,
                "prepared_bytes": prepared_bytes,
                "logical_rgba5551_bytes": None,
                "prepared_dimensions": [prepared_width, prepared_height],
            }
        )
    observed_total = sum(int(entry["prepared_bytes"]) for entry in entries)
    declared_total = totals.get("prepared_bytes")
    if declared_total != observed_total:
        raise falsify(
            f"{path}: totals.prepared_bytes {declared_total!r} != {observed_total}"
        )
    return {
        "path": str(path),
        "format": "canonical_profile2_owner_key_v1",
        "renderer_profile": profile,
        "harness_mode": harness_mode,
        "window_complete": coverage["window_complete"],
        "owner_complete": coverage["owner_complete"],
        "key_complete": coverage["key_complete"],
        "device_counters_complete": coverage["device_counters_complete"],
        "entries": entries,
        "limitations": [],
    }


def parse_runtime_export(path: Path) -> dict[str, object]:
    if not path.is_file():
        raise falsify(f"runtime export is absent: {path}")
    payload = path.read_text(encoding="utf-8")
    if any(line.startswith("TEX ") for line in payload.splitlines()):
        return parse_legacy_gdb_runtime_export(path, payload)
    try:
        document = json.loads(payload)
    except json.JSONDecodeError as exc:
        raise falsify(f"{path}: unsupported non-JSON runtime export") from exc
    return parse_canonical_runtime_export(path, document)


def discover_runtime_exports(repo_root: Path) -> list[Path]:
    candidates = list(
        repo_root.glob("artifacts/verifier-temp/**/_fighter_cache_probe.gdb.out")
    )
    candidates.extend(repo_root.glob("artifacts/verifier-temp/**/*.texture-keys.json"))
    candidates.extend(repo_root.glob("logs/**/*.texture-keys.json"))
    return sorted({candidate.resolve() for candidate in candidates})


def reconcile_runtime_exports(
    manifest: dict[str, object], manifest_digest: str, paths: Sequence[Path]
) -> dict[str, object]:
    owner_keys = source_keys_by_owner(manifest)
    expected_owners = set(owner_keys)
    exports = [parse_runtime_export(path) for path in paths]
    for export in exports:
        export_entries = export["entries"]
        assert isinstance(export_entries, list)
        export["summary"] = {
            "entry_count": len(export_entries),
            "prepared_bytes_exact": sum(
                int(entry["prepared_bytes"])
                for entry in export_entries
                if isinstance(entry, dict)
            ),
            "logical_rgba5551_bytes_exact_where_exported": sum(
                int(entry["logical_rgba5551_bytes"])
                for entry in export_entries
                if isinstance(entry, dict)
                and entry["logical_rgba5551_bytes"] is not None
            ),
        }
    entries = [
        entry
        for export in exports
        for entry in export["entries"]  # type: ignore[index]
    ]
    observed_by_owner: dict[str, set[str]] = {}
    unresolved_owner_entries = 0
    unresolved_source_entries = 0
    complete_key_entries = 0
    for entry in entries:
        assert isinstance(entry, dict)
        owner = entry["owner"]
        source_key = entry["source_key"]
        fingerprint = entry["complete_key_fingerprint"]
        if owner is None:
            unresolved_owner_entries += 1
        elif not isinstance(owner, str):
            raise falsify("parsed runtime owner is neither string nor null")
        if source_key is None:
            unresolved_source_entries += 1
        elif not isinstance(source_key, str):
            raise falsify("parsed runtime source key is neither string nor null")
        if owner is not None and source_key is not None:
            observed_by_owner.setdefault(owner, set()).add(source_key)
        if fingerprint is not None:
            complete_key_entries += 1

    observed_owners = set(observed_by_owner)
    missing_owners = sorted(expected_owners - observed_owners)
    extra_owners = sorted(observed_owners - expected_owners)
    missing_source_keys: list[dict[str, str]] = []
    extra_source_keys: list[dict[str, str]] = []
    for owner in sorted(expected_owners & observed_owners):
        for key in sorted(owner_keys[owner] - observed_by_owner[owner]):
            missing_source_keys.append({"owner": owner, "source_key": key})
        for key in sorted(observed_by_owner[owner] - owner_keys[owner]):
            extra_source_keys.append({"owner": owner, "source_key": key})
    for owner in extra_owners:
        for key in sorted(observed_by_owner[owner]):
            extra_source_keys.append({"owner": owner, "source_key": key})
    for owner in missing_owners:
        for key in sorted(owner_keys[owner]):
            missing_source_keys.append({"owner": owner, "source_key": key})

    prepared_bytes = sum(int(entry["prepared_bytes"]) for entry in entries)
    logical_bytes = sum(
        int(entry["logical_rgba5551_bytes"])
        for entry in entries
        if entry["logical_rgba5551_bytes"] is not None
    )
    qualification = manifest["qualification"]
    assert isinstance(qualification, dict)
    export_coverage_complete = bool(exports) and all(
        export["renderer_profile"] == 2
        and export["harness_mode"] == 163
        and export["window_complete"] is True
        and export["owner_complete"] is True
        and export["key_complete"] is True
        and export["device_counters_complete"] is True
        for export in exports
    )
    runtime_qualified = (
        qualification["complete_runtime_key_census_generated"] is True
        and export_coverage_complete
        and unresolved_owner_entries == 0
        and unresolved_source_entries == 0
        and complete_key_entries == len(entries)
        and not missing_owners
        and not extra_owners
        and not missing_source_keys
        and not extra_source_keys
    )
    uncertainty = manifest["runtime_only_uncertainty"]
    assert isinstance(uncertainty, list)
    qualification_blockers = [
        "the generated source manifest does not claim a complete runtime-key census"
        if qualification["complete_runtime_key_census_generated"] is not True
        else None,
        "runtime export identity/coverage is incomplete"
        if not export_coverage_complete
        else None,
        "observed entries lack owner or canonical source identity"
        if unresolved_owner_entries or unresolved_source_entries
        else None,
        "owner/source-key reconciliation has missing or extra records"
        if missing_owners or extra_owners or missing_source_keys or extra_source_keys
        else None,
        "complete 59-word keys are absent from some entries"
        if complete_key_entries != len(entries)
        else None,
        "device zero-conversion counters are not part of this host reconciliation",
    ]
    return {
        "schema": 1,
        "kind": "smash64ds-texture-census-runtime-reconciliation",
        "source_manifest_sha256": manifest_digest,
        "exports": exports,
        "summary": {
            "export_count": len(exports),
            "observed_entry_count": len(entries),
            "expected_source_owner_count": len(expected_owners),
            "observed_canonical_owner_count": len(observed_owners),
            "complete_key_entry_count": complete_key_entries,
            "unresolved_owner_entry_count": unresolved_owner_entries,
            "unresolved_source_entry_count": unresolved_source_entries,
            "missing_owners": missing_owners,
            "extra_owners": extra_owners,
            "missing_source_keys": missing_source_keys,
            "extra_source_keys": extra_source_keys,
            "prepared_bytes_exact": prepared_bytes,
            "logical_rgba5551_bytes_exact_where_exported": logical_bytes,
            "runtime_keys": "QUALIFIED" if runtime_qualified else "UNQUALIFIED",
            "device_zero_conversion": "UNPROVEN",
            "manifest_runtime_uncertainty_count": len(uncertainty),
            "qualification_blockers": [
                blocker for blocker in qualification_blockers if blocker is not None
            ],
        },
    }


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="checkout containing the pinned BattleShip and DS sources",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="optional JSON output; omitted by the checker to avoid repo writes",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="validate the exact pinned census and its fail-closed qualification",
    )
    parser.add_argument(
        "--runtime-export",
        type=Path,
        action="append",
        default=[],
        help=(
            "optional profile-2 owner/key export to reconcile; may be repeated. "
            "Malformed or unsupported exports fail closed"
        ),
    )
    parser.add_argument(
        "--reconcile-existing",
        action="store_true",
        help=(
            "also reconcile supported existing *.texture-keys.json exports and "
            "the legacy fighter cache probe when present"
        ),
    )
    parser.add_argument(
        "--reconciliation-json",
        action="store_true",
        help="print the optional runtime reconciliation as JSON",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        manifest = build_manifest(args.repo_root.resolve())
        digest = check_manifest(manifest) if args.check else sha256(canonical_payload(manifest))
        rendered = json.dumps(manifest, indent=2, sort_keys=True) + "\n"
        if args.output is not None:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(rendered, encoding="utf-8", newline="\n")
        elif not args.check:
            sys.stdout.write(rendered)
        if args.check:
            static = manifest["static_owners"]
            dynamic = manifest["dynamic_animated_owners"]
            water = manifest["water"]
            assert isinstance(static, dict) and isinstance(dynamic, dict)
            assert isinstance(water, dict)
            print(
                "BATTLE_PLAYABLE_TEXTURE_CENSUS_OK "
                f"static_blocks={static['summary']['source_block_count']} "  # type: ignore[index]
                f"dynamic_blocks={dynamic['summary']['source_block_count']} "  # type: ignore[index]
                f"water_blocks={water['summary']['source_block_count']} "  # type: ignore[index]
                "runtime_keys=UNQUALIFIED device_zero_conversion=UNPROVEN "
                f"sha256={digest}"
            )
        runtime_paths: list[Path] = []
        for supplied in args.runtime_export:
            runtime_paths.append(
                supplied.resolve()
                if supplied.is_absolute()
                else (args.repo_root.resolve() / supplied).resolve()
            )
        if args.reconcile_existing:
            runtime_paths.extend(discover_runtime_exports(args.repo_root.resolve()))
        runtime_paths = sorted(set(runtime_paths))
        if args.runtime_export or args.reconcile_existing:
            if not runtime_paths:
                print(
                    "BATTLE_PLAYABLE_TEXTURE_RUNTIME_EXPORT_NONE "
                    "runtime_keys=UNQUALIFIED device_zero_conversion=UNPROVEN"
                )
            else:
                reconciliation = reconcile_runtime_exports(manifest, digest, runtime_paths)
                summary = reconciliation["summary"]
                assert isinstance(summary, dict)
                if args.reconciliation_json:
                    print(json.dumps(reconciliation, indent=2, sort_keys=True))
                else:
                    missing_source_keys = summary["missing_source_keys"]
                    extra_source_keys = summary["extra_source_keys"]
                    assert isinstance(missing_source_keys, list)
                    assert isinstance(extra_source_keys, list)
                    print(
                        "BATTLE_PLAYABLE_TEXTURE_RUNTIME_RECONCILE "
                        f"exports={summary['export_count']} "
                        f"entries={summary['observed_entry_count']} "
                        f"complete_keys={summary['complete_key_entry_count']} "
                        f"owners={summary['observed_canonical_owner_count']}/"
                        f"{summary['expected_source_owner_count']} "
                        f"missing_owners={len(summary['missing_owners'])} "
                        f"extra_owners={len(summary['extra_owners'])} "
                        f"missing_source_keys={len(missing_source_keys)} "
                        f"extra_source_keys={len(extra_source_keys)} "
                        f"unresolved_owner_entries={summary['unresolved_owner_entry_count']} "
                        f"unresolved_source_entries={summary['unresolved_source_entry_count']} "
                        f"prepared_bytes={summary['prepared_bytes_exact']} "
                        f"logical_bytes={summary['logical_rgba5551_bytes_exact_where_exported']} "
                        f"runtime_keys={summary['runtime_keys']} "
                        f"device_zero_conversion={summary['device_zero_conversion']}"
                    )
                    missing_owners = summary["missing_owners"]
                    extra_owners = summary["extra_owners"]
                    assert isinstance(missing_owners, list)
                    assert isinstance(extra_owners, list)
                    print(
                        "BATTLE_PLAYABLE_TEXTURE_RUNTIME_OWNER_DIFF "
                        f"missing={','.join(missing_owners) if missing_owners else 'NONE'} "
                        f"extra={','.join(extra_owners) if extra_owners else 'NONE'}"
                    )
                    exports = reconciliation["exports"]
                    assert isinstance(exports, list)
                    for export in exports:
                        assert isinstance(export, dict)
                        export_summary = export["summary"]
                        assert isinstance(export_summary, dict)
                        profile = export["renderer_profile"]
                        print(
                            "BATTLE_PLAYABLE_TEXTURE_RUNTIME_EXPORT "
                            f"path={export['path']} format={export['format']} "
                            f"profile={profile if profile is not None else 'UNPROVEN'} "
                            f"entries={export_summary['entry_count']} "
                            f"prepared_bytes={export_summary['prepared_bytes_exact']} "
                            "logical_bytes="
                            f"{export_summary['logical_rgba5551_bytes_exact_where_exported']}"
                        )
        return 0
    except (Falsifier, OSError, UnicodeError, ValueError, struct.error) as exc:
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
