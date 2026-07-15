#!/usr/bin/env python3
"""Generate the immutable Dream Land whole-stage transaction packet.

This host-only generator reads pinned BattleShip O2R resources and the pinned
typed/source control files.  It expands the exact eight stage callback owners,
including the four runtime segment-E material programs, into a compact packet
of callback segments, DObj topology, display-list bindings, triangle runs,
dense source vertices, corners, texture epochs, and material events.

The output is data only.  It does not enable a renderer path and cannot claim
M3 timing or parity.  Any source, relocation, topology, command, or packet
cardinality drift is a falsifier that must be reviewed rather than normalized.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


DEFAULT_OUTPUT = Path("src/nds/nds_native_stage_owner.generated.inc")
MAX_SLAB_BYTES = 16 * 1024

EXPECTED_CALLBACKS = 8
EXPECTED_DOBJS = 57
EXPECTED_BINDINGS = 42
EXPECTED_COMMANDS = 886
EXPECTED_VERTEX_COMMANDS = 59
EXPECTED_SOURCE_VERTICES = 302
EXPECTED_MODIFY_VERTEX_COMMANDS = 10
# The ten ST modifications target cache vertices loaded by the preceding
# binding.  Clone-on-write keeps the already-submitted source vertex immutable
# while preserving the modified cache value for the following triangles.
EXPECTED_DENSE_VERTICES = EXPECTED_SOURCE_VERTICES + EXPECTED_MODIFY_VERTEX_COMMANDS
EXPECTED_TRIANGLE_COMMANDS = 113
EXPECTED_TRIANGLES = 202
EXPECTED_RUNS = 54
EXPECTED_TEXTURE_EPOCHS = 49
EXPECTED_MATERIAL_EVENTS = 4
EXPECTED_SUBMIT_CLASSES = (66, 126, 10)

# Filled after the first independently checked generation.  Keeping the
# packet hash outside the generated file avoids a self-referential checksum.
EXPECTED_INCLUDE_SHA256 = "e37b0a99f2520003d43ad7ee6b76d5e0e12687b287618d3d2093366d52328eac"

INVALID_U8 = 0xFF
INVALID_U16 = 0xFFFF

OP_VTX = 0x01
OP_MODIFYVTX = 0x02
OP_TRI1 = 0x05
OP_TRI2 = 0x06
OP_TEXTURE = 0xD7
OP_MTX = 0xDA
OP_GEOMETRYMODE = 0xD9
OP_MOVEWORD = 0xDB
OP_MOVEMEM = 0xDC
OP_DL = 0xDE
OP_ENDDL = 0xDF
OP_SETOTHERMODE_H = 0xE3
OP_SETOTHERMODE_L = 0xE2
OP_RDPSETOTHERMODE = 0xEF
OP_RDPPIPESYNC = 0xE7
OP_RDPLOADSYNC = 0xE6
OP_RDPTILESYNC = 0xE8
OP_SETPRIMDEPTH = 0xEE
OP_SETCOMBINE = 0xFC
OP_SETFOGCOLOR = 0xF8
OP_SETBLENDCOLOR = 0xF9
OP_SETENVCOLOR = 0xFB
OP_SETPRIMCOLOR = 0xFA
OP_SETTIMG = 0xFD
OP_SETTILE = 0xF5
OP_LOADTILE = 0xF4
OP_LOADBLOCK = 0xF3
OP_LOADTLUT = 0xF0
OP_SETTILESIZE = 0xF2

SUBMIT_RAW_CURRENT = 0
SUBMIT_PROJECTED_NO_Z = 3
SUBMIT_PROJECTED_RANGE_OR_MATRIX = 6

MOBJ_FLAG_ALPHA = 1 << 0
MOBJ_FLAG_SPLIT = 1 << 1
MOBJ_FLAG_PALETTE = 1 << 2
MOBJ_FLAG_FRAC = 1 << 4
MOBJ_FLAG_TEXTURE = 1 << 7
MOBJ_FLAG_PRIMCOLOR = 1 << 9
MOBJ_FLAG_ENVCOLOR = 1 << 10
MOBJ_FLAG_BLENDCOLOR = 1 << 11
MOBJ_FLAG_LIGHT1 = 1 << 12
MOBJ_FLAG_LIGHT2 = 1 << 13

GEOMETRY_ZBUFFER = 1 << 0
DEFAULT_OTHERMODE_H = (1 << 19) | (2 << 12)

TEXTURE_INVALIDATING_OPS = frozenset(
    (
        OP_TEXTURE,
        OP_GEOMETRYMODE,
        OP_SETCOMBINE,
        OP_SETTIMG,
        OP_SETTILE,
        OP_LOADTILE,
        OP_LOADBLOCK,
        OP_LOADTLUT,
        OP_SETTILESIZE,
        OP_SETOTHERMODE_H,
        OP_SETOTHERMODE_L,
        OP_RDPSETOTHERMODE,
        OP_SETENVCOLOR,
        OP_SETPRIMCOLOR,
    )
)

STATE_OPS = frozenset(
    (
        OP_TEXTURE,
        OP_MTX,
        OP_GEOMETRYMODE,
        OP_MOVEWORD,
        OP_MOVEMEM,
        OP_SETOTHERMODE_H,
        OP_SETOTHERMODE_L,
        OP_RDPSETOTHERMODE,
        OP_SETPRIMDEPTH,
        OP_SETCOMBINE,
        OP_SETFOGCOLOR,
        OP_SETBLENDCOLOR,
        OP_SETENVCOLOR,
        OP_SETPRIMCOLOR,
        OP_SETTIMG,
        OP_SETTILE,
        OP_LOADTILE,
        OP_LOADBLOCK,
        OP_LOADTLUT,
        OP_SETTILESIZE,
    )
)


class Falsifier(RuntimeError):
    """A pinned M3 source or transaction invariant changed."""


def falsify(message: str) -> Falsifier:
    return Falsifier(f"M3_STAGE_FALSIFIER: {message}")


def sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def fnv1a_u32(words: Iterable[int], seed: int = 2166136261) -> int:
    value = seed
    for word in words:
        word &= 0xFFFFFFFF
        for shift in (0, 8, 16, 24):
            value ^= (word >> shift) & 0xFF
            value = (value * 16777619) & 0xFFFFFFFF
    return value


def fnv1a_bytes(payload: bytes, seed: int = 2166136261) -> int:
    value = seed
    for byte in payload:
        value ^= byte
        value = (value * 16777619) & 0xFFFFFFFF
    return value


@dataclass(frozen=True)
class InputSpec:
    path: str
    sha256: str
    file_id: int | None = None
    internal_fixups: int | None = None
    external_fixups: int | None = None
    payload_sha256: str | None = None


O2R_INPUTS = {
    "stage_images": InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank103",
        "a61e74aece06c5f15fa7cd1d6633afd9cc3750c9163caeffe59cab2d157a222a",
        103,
        0,
        0,
        "4109fa4ac31fdf36d25ea228f8475a1efa6758114942ee65b051e56358684afe",
    ),
    "stage_geometry": InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank104",
        "3ce7e51da3810dca927521717357a2c44b1c51760bc942b0d4e5bfebe6fd4d52",
        104,
        114,
        57,
        "1d82f9304458528341452b9610f55952a4951a1ead4c41cd0c5ebdab10380ebd",
    ),
    "stage_actors": InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank152",
        "4a3557fc41fbb06ead175ea25b2dfac5373896cb473800638c7b2924b2f26b1a",
        152,
        147,
        0,
        "cc0fc629911e04c4bdbb2d7ce9098df6b2a9a62847b08fd44f6ce9158cbc2187",
    ),
    "stage_map": InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRPupupuMap",
        "7df485462836872c0e00685876a4aa724977f480cccf861d0d41d0a19b2e224e",
        255,
        1,
        9,
        "f0b62e005050c3597b4fd01abd77dadcde1fb7a339948d789c3538ef750c7e05",
    ),
}

TEXT_INPUTS = {
    "geometry_typed": InputSpec(
        "decomp/BattleShip-main/decomp/src/relocData/104_StagePupupuFile2.c",
        "3608c144694eceef3639c08155e18bd6155ea91b51c95266f7f7eca2f782c845",
    ),
    "actors_typed": InputSpec(
        "decomp/BattleShip-main/decomp/src/relocData/152_StagePupupuFile3.c",
        "65ea777e827f3c6fa1baaf49b13e7a7ad7e76ec919e112d1ef3c574ead447915",
    ),
    "map_typed": InputSpec(
        "decomp/BattleShip-main/decomp/src/relocData/255_GRPupupuMap.c",
        "dabbca356a698411c8691d10c1272bb8a48b24ef6f39362f5eafa4bbae45a69c",
    ),
    "pupupu": InputSpec(
        "decomp/BattleShip-main/decomp/src/gr/grcommon/grpupupu.c",
        "dc9f9228e00f9de2ba82d4b3747fbabb523e29d0e431e7bcf5643877e1a5d8be",
    ),
    "grdisplay": InputSpec(
        "decomp/BattleShip-main/decomp/src/gr/grdisplay.c",
        "d48f187c90f66f2284625977a9e5cd8450108407f91c4d4a9247d28f5646ac03",
    ),
    "objanim": InputSpec(
        "decomp/BattleShip-main/decomp/src/sys/objanim.c",
        "eddedabd7aaffb4090e01fe0edcfac77f4262f42b91a3fe8faeddae2e3356dde",
    ),
    "objdisplay": InputSpec(
        "decomp/BattleShip-main/decomp/src/sys/objdisplay.c",
        "11f20ae08baf696ea1eff535bdede9bb21952f51e0508da0266ec21bc8eed9eb",
    ),
    "reloc_symbols": InputSpec(
        "decomp/BattleShip-main/include/reloc_data.us.h",
        "8c2d5938590e9a38ca2dad6ac0fa45b4742d125ed5d89f305c38774e40551385",
    ),
}


@dataclass(frozen=True, order=True)
class PointerRef:
    asset_id: int
    offset: int


@dataclass(frozen=True)
class O2RResource:
    spec: InputSpec
    source: bytes
    payload: bytes
    file_id: int
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


def load_o2r(repo_root: Path, spec: InputSpec) -> O2RResource:
    source = checked_bytes(repo_root, spec)
    if len(source) < 0x50 or source[4:8] != b"OLER":
        raise falsify(f"{spec.path}: invalid O2R header")
    file_id, internal_head, external_head, extern_count = struct.unpack_from(
        "<IHHI", source, 0x40
    )
    if spec.file_id is not None and file_id != spec.file_id:
        raise falsify(f"{spec.path}: file ID {file_id} != {spec.file_id}")
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
    payload = source[extern_ids_end + 4 :]
    if len(payload) != data_size:
        raise falsify(
            f"{spec.path}: payload bytes {len(payload)} != declared {data_size}"
        )
    if spec.payload_sha256 is not None and sha256(payload) != spec.payload_sha256:
        raise falsify(f"{spec.path}: payload SHA256 changed")

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
            target_file = file_id
            if dependencies is not None:
                if dependency_index >= len(dependencies):
                    raise falsify(f"{spec.path}: extern chain exceeds file table")
                target_file = dependencies[dependency_index]
                dependency_index += 1
            result[slot] = PointerRef(target_file, (word & 0xFFFF) * 4)
            cursor = word >> 16
        if dependencies is not None and dependency_index != len(dependencies):
            raise falsify(f"{spec.path}: extern chain/file table mismatch")
        return result

    internal = walk_chain(internal_head, None)
    external = walk_chain(external_head, extern_ids)
    if spec.internal_fixups is not None and len(internal) != spec.internal_fixups:
        raise falsify(
            f"{spec.path}: internal fixups {len(internal)} != "
            f"{spec.internal_fixups}"
        )
    if spec.external_fixups is not None and len(external) != spec.external_fixups:
        raise falsify(
            f"{spec.path}: external fixups {len(external)} != "
            f"{spec.external_fixups}"
        )
    return O2RResource(spec, source, payload, file_id, internal, external)


def load_and_validate_text(repo_root: Path) -> dict[str, str]:
    texts = {
        name: checked_bytes(repo_root, spec).decode("utf-8")
        for name, spec in TEXT_INPUTS.items()
    }
    required = {
        "map_typed": (
            "MPGroundData dGRPupupuMap_header",
            "dStagePupupuFile2_data_0x1008",
            "dStagePupupuFile2_Layer0Anim_DObjDesc_0x1CE0",
            "dStagePupupuFile2_gap_0x22D0_sub_0x180",
            "dStagePupupuFile2_Layer3Anim_DObjDesc_0x2BF8",
        ),
        "pupupu": (
            "grPupupuMakeMapGObj",
            "grPupupuUpdateGObjAnims",
            "gcAddAnimAll",
            "gcAddAnimJointAll",
        ),
        "grdisplay": (
            "grDisplayLayer0PriProcDisplay",
            "grDisplayLayer1PriProcDisplay",
            "grDisplayLayer2PriProcDisplay",
            "grDisplayLayer3PriProcDisplay",
            "gcDrawDObjTreeForGObj",
        ),
        "objanim": ("gcPlayAnimAll", "gcParseMObjMatAnimJoint"),
        "objdisplay": (
            "void gcDrawMObjForDObj",
            "gSPSegment(dl_head[0]++, 0xE",
            "void gcDrawDObjTreeForGObj",
        ),
        "reloc_symbols": (
            "llGRPupupuMapFileID",
            "llGRPupupuMapMapHead",
            "llGRPupupuMapWhispyMouthTransformKindsDObjDesc",
        ),
    }
    for name, tokens in required.items():
        for token in tokens:
            if token not in texts[name]:
                raise falsify(f"{TEXT_INPUTS[name].path}: missing contract {token!r}")
    return texts


OWNER_LAYER0 = 0
OWNER_LAYER1 = 1
OWNER_LAYER2 = 2
OWNER_LAYER3 = 3
OWNER_MAP0 = 4
OWNER_MAP1 = 5
OWNER_MAP2 = 6
OWNER_MAP3 = 7


@dataclass(frozen=True)
class OwnerSpec:
    owner: int
    name: str
    resource_name: str
    dobj_offset: int
    descriptor_count: int
    link: int
    callback: str


# Actual gcCaptureCameraGObj link order, not constructor/source declaration
# order.  Keeping binding/run spans contiguous removes runtime indirection.
OWNER_SPECS = (
    OwnerSpec(OWNER_LAYER0, "layer0", "stage_geometry", 0x1008, 22, 4,
              "grDisplayLayer0PriProcDisplay"),
    OwnerSpec(OWNER_MAP0, "map0", "stage_actors", 0x10F0, 4, 4,
              "grDisplayLayer0PriProcDisplay"),
    OwnerSpec(OWNER_MAP1, "map1", "stage_actors", 0x1770, 7, 4,
              "grDisplayLayer0PriProcDisplay"),
    OwnerSpec(OWNER_MAP2, "map2", "stage_actors", 0x2A80, 8, 4,
              "grDisplayLayer0PriProcDisplay"),
    OwnerSpec(OWNER_LAYER1, "layer1", "stage_geometry", 0x1CE0, 3, 6,
              "grDisplayLayer1PriProcDisplay"),
    OwnerSpec(OWNER_LAYER2, "layer2", "stage_geometry", 0x2450, 5, 13,
              "grDisplayLayer2PriProcDisplay"),
    OwnerSpec(OWNER_MAP3, "map3", "stage_actors", 0x31F8, 11, 16,
              "grDisplayLayer3PriProcDisplay"),
    OwnerSpec(OWNER_LAYER3, "layer3", "stage_geometry", 0x2BF8, 5, 17,
              "grDisplayLayer3PriProcDisplay"),
)


@dataclass(frozen=True)
class MaterialSource:
    asset_id: int
    binding_root: int
    mobj_offset: int


MATERIAL_SOURCES = (
    MaterialSource(152, 0x0FF8, 0x0F18),
    MaterialSource(152, 0x1630, 0x13D8),
    MaterialSource(104, 0x22C8, 0x1F78),
    MaterialSource(104, 0x2380, 0x1FF0),
)


@dataclass(frozen=True)
class MaterialEvent:
    mobj_offset: int
    binding_index: int
    asset_index: int
    material_slot: int
    segment_index: int
    source_command_count: int
    flags: int
    opcodes: tuple[int, ...]


@dataclass(frozen=True)
class CommandEvent:
    source_offset: int
    op: int
    w0: int
    w1: int
    asset_id: int
    material_event: int = INVALID_U8
    material_command: int = INVALID_U8


@dataclass(frozen=True)
class StageAsset:
    asset_id: int
    payload_size: int
    payload_checksum: int
    flags: int


@dataclass(frozen=True)
class StageSegment:
    first_dobj: int
    dobj_count: int
    owner: int
    link: int
    first_binding: int
    binding_count: int
    initial_geometry: int
    first_run: int
    run_count: int


@dataclass(frozen=True)
class StageDObj:
    source_checksum: int
    parent_index: int
    binding_index: int
    transform_flags: int
    owner: int
    depth: int


@dataclass(frozen=True)
class StageBinding:
    root_offset: int
    traversal_checksum: int
    first_vertex: int
    first_run: int
    first_epoch: int
    source_command_count: int
    vertex_command_count: int
    source_vertex_count: int
    triangle_command_count: int
    triangle_count: int
    run_count: int
    texture_epoch_count: int
    asset_index: int
    material_event: int


@dataclass(frozen=True)
class StageRun:
    first_corner: int
    triangle_count: int
    binding_index: int
    texture_epoch: int
    submit_class: int
    state_policy: int
    flags: int


@dataclass(frozen=True)
class DenseVertex:
    x: int
    y: int
    z: int
    s: int
    t: int
    matrix_binding: int
    cache_slot: int
    rgba: int


@dataclass(frozen=True)
class TextureEpoch:
    source_command_offset: int
    asset_index: int
    policy_index: int
    material_event: int
    flags: int


@dataclass(frozen=True)
class StatePolicy:
    state_hash: int
    texture_hash: int
    combine_w0: int
    combine_w1: int
    othermode_h: int
    othermode_l: int
    geometry_mode: int


@dataclass(frozen=True)
class Packet:
    assets: tuple[StageAsset, ...]
    segments: tuple[StageSegment, ...]
    dobjs: tuple[StageDObj, ...]
    bindings: tuple[StageBinding, ...]
    runs: tuple[StageRun, ...]
    vertices: tuple[DenseVertex, ...]
    corners: tuple[int, ...]
    epochs: tuple[TextureEpoch, ...]
    materials: tuple[MaterialEvent, ...]
    policies: tuple[StatePolicy, ...]
    source_command_count: int
    vertex_command_count: int
    triangle_command_count: int

    def slab_bytes(self) -> int:
        return (
            len(self.assets) * 16
            + len(self.segments) * 12
            + len(self.dobjs) * 12
            + len(self.bindings) * 24
            + len(self.runs) * 8
            + len(self.vertices) * 16
            + len(self.corners) * 2
            + len(self.epochs) * 8
            + len(self.materials) * 12
            + len(self.policies) * 28
        )


def checked_u32(payload: bytes, offset: int, context: str) -> int:
    if offset < 0 or offset + 4 > len(payload):
        raise falsify(f"{context}: u32 at 0x{offset:x} is out of range")
    return struct.unpack_from(">I", payload, offset)[0]


def material_opcodes(flags: int, has_palettes: bool) -> tuple[int, ...]:
    # First command is the segment table's branch-list command.  The rest are
    # exactly gcDrawMObjForDObj's generated material program, including ENDDL.
    result = [OP_DL]
    if not (flags & MOBJ_FLAG_PALETTE) and has_palettes:
        result.append(OP_SETTIMG)
    if flags & MOBJ_FLAG_PALETTE:
        result.append(OP_SETTIMG)
        if flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA):
            result.extend(
                (OP_RDPTILESYNC, OP_SETTILE, OP_RDPLOADSYNC, OP_LOADTLUT,
                 OP_RDPPIPESYNC)
            )
    if flags & MOBJ_FLAG_LIGHT1:
        result.extend((OP_MOVEWORD, OP_MOVEWORD))
    if flags & MOBJ_FLAG_LIGHT2:
        result.extend((OP_MOVEWORD, OP_MOVEWORD))
    if flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8):
        result.append(OP_SETPRIMCOLOR)
    if flags & MOBJ_FLAG_ENVCOLOR:
        result.append(OP_SETENVCOLOR)
    if flags & MOBJ_FLAG_BLENDCOLOR:
        result.append(OP_SETBLENDCOLOR)
    if flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT):
        result.append(OP_SETTIMG)
        if flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA):
            result.extend((OP_RDPLOADSYNC, OP_LOADBLOCK, OP_RDPLOADSYNC))
    if flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA):
        result.append(OP_SETTIMG)
    if flags & 0x20:
        result.append(OP_SETTILESIZE)
    if flags & 0x40:
        result.append(OP_SETTILESIZE)
    if flags & MOBJ_FLAG_TEXTURE:
        result.append(OP_TEXTURE)
    result.append(OP_ENDDL)
    return tuple(result)


def build_material_events(
    resources: dict[str, O2RResource],
    binding_lookup: dict[tuple[int, int], int],
    asset_index: dict[int, int],
) -> tuple[MaterialEvent, ...]:
    by_id = {resource.file_id: resource for resource in resources.values()}
    result: list[MaterialEvent] = []
    for slot, source in enumerate(MATERIAL_SOURCES):
        resource = by_id[source.asset_id]
        if source.mobj_offset + 0x78 > len(resource.payload):
            raise falsify(f"MObjSub 0x{source.mobj_offset:x} is truncated")
        flags = struct.unpack_from(">H", resource.payload, source.mobj_offset + 0x30)[0]
        palette_ref = resource.pointer_at(source.mobj_offset + 0x2C)
        palette_word = checked_u32(
            resource.payload, source.mobj_offset + 0x2C, "MObj palettes"
        )
        if palette_ref is None and palette_word != 0:
            raise falsify(f"MObjSub 0x{source.mobj_offset:x}: unresolved palettes")
        opcodes = material_opcodes(flags, palette_ref is not None)
        binding_index = binding_lookup.get((source.asset_id, source.binding_root))
        if binding_index is None:
            raise falsify(
                f"material root {source.asset_id}:0x{source.binding_root:x} "
                "is not a selected binding"
            )
        result.append(
            MaterialEvent(
                source.mobj_offset,
                binding_index,
                asset_index[source.asset_id],
                slot,
                0,
                len(opcodes),
                flags,
                opcodes,
            )
        )
    if tuple(event.source_command_count for event in result) != (3, 3, 10, 10):
        raise falsify(
            "segment-E command partition changed: "
            f"{tuple(event.source_command_count for event in result)}"
        )
    return tuple(result)


def descriptor_rows(
    resource: O2RResource,
    owner: OwnerSpec,
    global_first: int,
    binding_lookup: dict[tuple[int, int], int],
) -> tuple[list[StageDObj], list[int]]:
    result: list[StageDObj] = []
    display_offsets: list[int] = []
    depth_stack: list[int] = []
    for local_index in range(owner.descriptor_count):
        offset = owner.dobj_offset + local_index * 44
        if offset + 44 > len(resource.payload):
            raise falsify(f"{owner.name}: DObjDesc table is truncated")
        descriptor_id = checked_u32(resource.payload, offset, "DObj id")
        ref = resource.pointer_at(offset + 4)
        raw_pointer = checked_u32(resource.payload, offset + 4, "DObj display")
        if local_index == owner.descriptor_count - 1:
            if descriptor_id != 18 or ref is not None or raw_pointer != 0:
                raise falsify(f"{owner.name}: invalid DObj sentinel")
            continue
        depth = descriptor_id & 0xFFF
        transform_flags = (descriptor_id >> 12) & 0xF
        if local_index == 0:
            if depth != 0:
                raise falsify(f"{owner.name}: first DObj depth is {depth}")
            parent_index = INVALID_U16
        else:
            if depth == 0 or depth - 1 >= len(depth_stack):
                raise falsify(
                    f"{owner.name}: DObj {local_index} has no parent at depth {depth}"
                )
            parent_index = global_first + depth_stack[depth - 1]
        if len(depth_stack) <= depth:
            depth_stack.append(local_index)
        else:
            depth_stack[depth] = local_index
            del depth_stack[depth + 1 :]

        binding_index = INVALID_U16
        normalized_pointer = 0
        if ref is not None:
            if ref.asset_id != resource.file_id:
                raise falsify(f"{owner.name}: external DObj display list")
            binding_index = binding_lookup.get((ref.asset_id, ref.offset), INVALID_U16)
            if binding_index == INVALID_U16:
                raise falsify(
                    f"{owner.name}: display list 0x{ref.offset:x} is unbound"
                )
            normalized_pointer = ref.offset
            display_offsets.append(ref.offset)
        elif raw_pointer != 0:
            raise falsify(f"{owner.name}: unresolved DObj display pointer")

        descriptor_bytes = bytearray(resource.payload[offset : offset + 44])
        descriptor_bytes[4:8] = struct.pack(">I", normalized_pointer)
        result.append(
            StageDObj(
                fnv1a_bytes(bytes(descriptor_bytes)),
                parent_index,
                binding_index,
                transform_flags,
                owner.owner,
                depth,
            )
        )
    return result, display_offsets


def selected_roots(resource: O2RResource, owner: OwnerSpec) -> list[int]:
    roots: list[int] = []
    for index in range(owner.descriptor_count - 1):
        slot = owner.dobj_offset + index * 44 + 4
        ref = resource.pointer_at(slot)
        if ref is None:
            if checked_u32(resource.payload, slot, "DObj display") != 0:
                raise falsify(f"{owner.name}: unresolved display pointer")
            continue
        if ref.asset_id != resource.file_id:
            raise falsify(f"{owner.name}: external selected display list")
        roots.append(ref.offset)
    return roots


def decode_vertex(resource: O2RResource, offset: int) -> tuple[int, ...]:
    if offset < 0 or offset + 16 > len(resource.payload):
        raise falsify(f"vertex 0x{offset:x} is out of range")
    x, y, z, _flag, s, t, r, g, b, a = struct.unpack_from(
        ">hhhHhhBBBB", resource.payload, offset
    )
    if a == 0:
        a = 0xFF
    return x, y, z, s, t, (r << 24) | (g << 16) | (b << 8) | a


def decode_triangles(op: int, w0: int, w1: int) -> tuple[tuple[int, int, int], ...]:
    packed = [w0 & 0xFFFFFF]
    if op == OP_TRI2:
        packed.append(w1 & 0xFFFFFF)
    result = []
    for triangle in packed:
        result.append(
            (
                ((triangle >> 16) & 0xFF) // 2,
                ((triangle >> 8) & 0xFF) // 2,
                (triangle & 0xFF) // 2,
            )
        )
    return tuple(result)


@dataclass
class SourceState:
    geometry_mode: int
    othermode_h: int = DEFAULT_OTHERMODE_H
    othermode_l: int = 0
    combine_w0: int = 0
    combine_w1: int = 0
    state_hash: int = 2166136261
    texture_hash: int = 2166136261

    def policy(self) -> StatePolicy:
        return StatePolicy(
            self.state_hash,
            self.texture_hash,
            self.combine_w0,
            self.combine_w1,
            self.othermode_h,
            self.othermode_l,
            self.geometry_mode,
        )


def apply_othermode(current: int, op: int, w0: int, w1: int) -> int:
    if op == OP_RDPSETOTHERMODE:
        return w1
    bits = (w0 & 0xFF) + 1
    pos = (w0 >> 8) & 0xFF
    if bits > 32 or pos >= 32 or bits + pos > 32:
        return current
    shift = 32 - pos - bits
    mask = 0xFFFFFFFF if bits == 32 else ((1 << bits) - 1) << shift
    return (current & ~mask) | (w1 & mask)


def normalized_event_words(
    resource: O2RResource, event: CommandEvent
) -> tuple[int, ...]:
    if event.material_event != INVALID_U8:
        return (
            0x4D415433,
            event.material_event,
            event.material_command,
            event.op,
        )
    ref = resource.pointer_at(event.source_offset + 4)
    if ref is not None:
        return (event.op, event.w0, ref.asset_id, ref.offset)
    return (event.op, event.w0, event.w1)


def apply_source_state(
    resource: O2RResource, state: SourceState, event: CommandEvent
) -> None:
    op = event.op
    if op in STATE_OPS or event.material_event != INVALID_U8:
        state.state_hash = fnv1a_u32(
            normalized_event_words(resource, event), state.state_hash
        )
    if op in TEXTURE_INVALIDATING_OPS:
        state.texture_hash = fnv1a_u32(
            normalized_event_words(resource, event), state.texture_hash
        )
    if event.material_event != INVALID_U8:
        return
    if op == OP_GEOMETRYMODE:
        state.geometry_mode = (state.geometry_mode & event.w0) | event.w1
    elif op == OP_SETCOMBINE:
        state.combine_w0 = event.w0
        state.combine_w1 = event.w1
    elif op == OP_RDPSETOTHERMODE:
        state.othermode_h = event.w0 & 0xFFFFFF
        state.othermode_l = event.w1
    elif op == OP_SETOTHERMODE_H:
        state.othermode_h = apply_othermode(
            state.othermode_h, op, event.w0, event.w1
        )
    elif op == OP_SETOTHERMODE_L:
        state.othermode_l = apply_othermode(
            state.othermode_l, op, event.w0, event.w1
        )


def walk_display_list(
    resource: O2RResource,
    start: int,
    material_index: int,
    materials: Sequence[MaterialEvent],
    stack: tuple[int, ...] = (),
) -> list[CommandEvent]:
    if start in stack:
        raise falsify(f"asset {resource.file_id}: recursive DL 0x{start:x}")
    if start < 0 or start + 8 > len(resource.payload):
        raise falsify(f"asset {resource.file_id}: DL 0x{start:x} is out of range")
    stack += (start,)
    result: list[CommandEvent] = []
    pc = start
    segment_calls = 0
    for _ in range(4096):
        w0, w1 = struct.unpack_from(">II", resource.payload, pc)
        op = w0 >> 24
        result.append(CommandEvent(pc, op, w0, w1, resource.file_id))
        if op == OP_DL:
            ref = resource.pointer_at(pc + 4)
            if ref is not None:
                if ref.asset_id != resource.file_id:
                    raise falsify(
                        f"asset {resource.file_id}: external DL at 0x{pc:x}"
                    )
                result.extend(
                    walk_display_list(
                        resource, ref.offset, material_index, materials, stack
                    )
                )
                if w0 & (1 << 16):
                    return result
            elif (w1 & 0xFF000000) == 0x0E000000:
                if material_index == INVALID_U8:
                    raise falsify(
                        f"asset {resource.file_id}: unbound segment-E at 0x{pc:x}"
                    )
                material = materials[material_index]
                segment_index = w1 & 0xFFFFFF
                if segment_index != material.segment_index:
                    raise falsify(
                        f"material {material_index}: segment index "
                        f"0x{segment_index:x} != 0x{material.segment_index:x}"
                    )
                segment_calls += 1
                for command_index, material_op in enumerate(material.opcodes):
                    result.append(
                        CommandEvent(
                            0x80000000
                            | (material_index << 8)
                            | command_index,
                            material_op,
                            material_op << 24,
                            0,
                            resource.file_id,
                            material_index,
                            command_index,
                        )
                    )
                if w0 & (1 << 16):
                    return result
            else:
                raise falsify(
                    f"asset {resource.file_id}: unresolved DL at 0x{pc:x}"
                )
        if op == OP_ENDDL:
            if material_index != INVALID_U8 and segment_calls != 1:
                raise falsify(
                    f"material binding at 0x{start:x}: segment calls "
                    f"{segment_calls} != 1"
                )
            return result
        pc += 8
        if pc + 8 > len(resource.payload):
            break
    raise falsify(f"asset {resource.file_id}: unterminated DL 0x{start:x}")


def event_checksum(resource: O2RResource, events: Sequence[CommandEvent]) -> int:
    value = 2166136261
    for event in events:
        value = fnv1a_u32((event.source_offset,), value)
        value = fnv1a_u32(normalized_event_words(resource, event), value)
    return value


def validate_callback_contract(texts: dict[str, str]) -> None:
    expected = (
        ("layer0", "grDisplayLayer0PriProcDisplay", 4),
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
        ("layer2", "grDisplayLayer2PriProcDisplay", 13),
        ("layer3", "grDisplayLayer3PriProcDisplay", 17),
        ("map0", "grDisplayLayer0PriProcDisplay", 4),
        ("map1", "grDisplayLayer0PriProcDisplay", 4),
        ("map2", "grDisplayLayer0PriProcDisplay", 4),
        ("map3", "grDisplayLayer3PriProcDisplay", 16),
    )
    actual = tuple(
        (owner.name, owner.callback, owner.link)
        for owner in sorted(OWNER_SPECS, key=lambda item: item.owner)
    )
    if actual != expected:
        raise falsify(f"callback partition changed: {actual}")
    # The hashes pin the complete files; these targeted checks keep failures
    # comprehensible when the source contract is deliberately updated.
    for _name, callback, _link in expected:
        if callback not in texts["grdisplay"]:
            raise falsify(f"missing callback definition {callback}")
    if texts["pupupu"].count("grPupupuMakeMapGObj(") < 5:
        raise falsify("Pupupu map constructor call partition changed")


def generate(repo_root: Path) -> Packet:
    repo_root = repo_root.resolve()
    texts = load_and_validate_text(repo_root)
    validate_callback_contract(texts)
    resources = {
        name: load_o2r(repo_root, spec) for name, spec in O2R_INPUTS.items()
    }
    resource_by_id = {resource.file_id: resource for resource in resources.values()}
    if len(resource_by_id) != len(resources):
        raise falsify("duplicate O2R file IDs")

    assets = tuple(
        StageAsset(
            resources[name].file_id,
            len(resources[name].payload),
            fnv1a_bytes(resources[name].payload),
            flags,
        )
        for name, flags in (
            ("stage_images", 2),
            ("stage_geometry", 1),
            ("stage_actors", 1),
            ("stage_map", 4),
        )
    )
    asset_index = {asset.asset_id: index for index, asset in enumerate(assets)}

    owner_roots: dict[int, list[int]] = {}
    binding_order: list[tuple[OwnerSpec, O2RResource, int]] = []
    for owner in OWNER_SPECS:
        resource = resources[owner.resource_name]
        roots = selected_roots(resource, owner)
        owner_roots[owner.owner] = roots
        binding_order.extend((owner, resource, root) for root in roots)
    if len(binding_order) != EXPECTED_BINDINGS:
        raise falsify(f"selected bindings {len(binding_order)} != {EXPECTED_BINDINGS}")
    if len({(resource.file_id, root) for _, resource, root in binding_order}) != len(
        binding_order
    ):
        raise falsify("selected display-list identities are not unique")
    binding_lookup = {
        (resource.file_id, root): index
        for index, (_owner, resource, root) in enumerate(binding_order)
    }
    materials = build_material_events(resources, binding_lookup, asset_index)
    material_by_binding = {event.binding_index: index for index, event in enumerate(materials)}

    dobjs: list[StageDObj] = []
    segment_dobj_spans: dict[int, tuple[int, int]] = {}
    for owner in OWNER_SPECS:
        resource = resources[owner.resource_name]
        first = len(dobjs)
        rows, display_offsets = descriptor_rows(
            resource, owner, first, binding_lookup
        )
        if display_offsets != owner_roots[owner.owner]:
            raise falsify(f"{owner.name}: DObj/list preorder changed")
        dobjs.extend(rows)
        segment_dobj_spans[owner.owner] = (first, len(rows))
    if len(dobjs) != EXPECTED_DOBJS:
        raise falsify(f"live DObjs {len(dobjs)} != {EXPECTED_DOBJS}")

    bindings: list[StageBinding] = []
    runs: list[StageRun] = []
    vertices: list[DenseVertex] = []
    corners: list[int] = []
    epochs: list[TextureEpoch] = []
    policies: list[StatePolicy] = []
    policy_lookup: dict[StatePolicy, int] = {}
    source_command_total = 0
    vertex_command_total = 0
    triangle_command_total = 0
    modify_vertex_total = 0
    segments: list[StageSegment] = []

    binding_cursor = 0
    for owner in OWNER_SPECS:
        resource = resources[owner.resource_name]
        roots = owner_roots[owner.owner]
        first_binding = binding_cursor
        first_run = len(runs)
        state = SourceState(
            GEOMETRY_ZBUFFER if owner.link == 6 else 0,
            state_hash=fnv1a_u32((0x53454733, owner.owner, owner.link)),
            texture_hash=fnv1a_u32((0x54455833, owner.owner, owner.link)),
        )
        # F3DEX2's vertex cache persists across display lists in one callback.
        # Dream Land uses that contract in five adjacent map-list pairs: the
        # second list modifies ST on slots 0/1 loaded by the first list, then
        # loads slots 2/3 and submits the stitched quad.
        slots: dict[int, int] = {}
        for root in roots:
            binding_index = binding_cursor
            binding_cursor += 1
            material_index = material_by_binding.get(binding_index, INVALID_U8)
            events = walk_display_list(
                resource, root, material_index, materials
            )
            first_vertex = len(vertices)
            binding_first_run = len(runs)
            first_epoch = len(epochs)
            texture_prepare_valid = False
            current_epoch = INVALID_U8
            current_run: dict[str, object] | None = None
            vertex_commands = 0
            source_vertices = 0
            triangle_commands = 0
            triangle_count = 0
            modify_vertex_commands = 0
            material_seen = INVALID_U8

            def finish_run() -> None:
                nonlocal current_run
                if current_run is None:
                    return
                classes = current_run["classes"]
                assert isinstance(classes, set)
                if len(classes) != 1:
                    raise falsify(
                        f"binding {binding_index}: mixed submit classes in one run"
                    )
                run_class = next(iter(classes))
                policy = state.policy()
                policy_index = policy_lookup.get(policy)
                if policy_index is None:
                    policy_index = len(policies)
                    if policy_index > 0xFF:
                        raise falsify("state-policy table exceeds u8 index")
                    policy_lookup[policy] = policy_index
                    policies.append(policy)
                run_epoch = int(current_run["epoch"])
                if epochs[run_epoch].policy_index == INVALID_U8:
                    epoch = epochs[run_epoch]
                    epochs[run_epoch] = TextureEpoch(
                        epoch.source_command_offset,
                        epoch.asset_index,
                        policy_index,
                        epoch.material_event,
                        epoch.flags,
                    )
                elif epochs[run_epoch].policy_index != policy_index:
                    raise falsify(
                        f"binding {binding_index}: reused texture epoch changed state"
                    )
                runs.append(
                    StageRun(
                        int(current_run["first_corner"]),
                        int(current_run["triangles"]),
                        binding_index,
                        run_epoch,
                        run_class,
                        policy_index,
                        0,
                    )
                )
                current_run = None

            for event in events:
                op = event.op
                if op not in (OP_TRI1, OP_TRI2):
                    finish_run()
                if event.material_event != INVALID_U8:
                    material_seen = event.material_event
                apply_source_state(resource, state, event)
                if op in TEXTURE_INVALIDATING_OPS:
                    texture_prepare_valid = False
                if op == OP_MODIFYVTX:
                    modify_vertex_commands += 1
                    where = (event.w0 >> 16) & 0xFF
                    encoded_slot = event.w0 & 0xFFFF
                    if where != 0x14 or (encoded_slot & 1) != 0:
                        raise falsify(
                            f"binding {binding_index}: unsupported MODIFYVTX "
                            f"where=0x{where:02x} slot=0x{encoded_slot:04x}"
                        )
                    cache_slot = encoded_slot >> 1
                    dense_index = slots.get(cache_slot)
                    if dense_index is None:
                        raise falsify(
                            f"binding {binding_index}: MODIFYVTX uses unloaded "
                            f"cache slot {cache_slot}"
                        )
                    source = vertices[dense_index]
                    s = (event.w1 >> 16) & 0xFFFF
                    t = event.w1 & 0xFFFF
                    if s & 0x8000:
                        s -= 0x10000
                    if t & 0x8000:
                        t -= 0x10000
                    # Packet vertices are immutable.  Clone the cached vertex
                    # so triangles already emitted by the preceding binding
                    # retain their original ST while later triangles see the
                    # exact gSPModifyVertex result.
                    slots[cache_slot] = len(vertices)
                    vertices.append(
                        DenseVertex(
                            source.x,
                            source.y,
                            source.z,
                            s,
                            t,
                            source.matrix_binding,
                            source.cache_slot,
                            source.rgba,
                        )
                    )
                if op == OP_VTX:
                    vertex_commands += 1
                    count = (event.w0 >> 12) & 0xFF
                    end = (event.w0 >> 1) & 0x7F
                    if count == 0 or count > 32 or end < count or end > 32:
                        raise falsify(
                            f"binding {binding_index}: invalid F3DEX2 VTX command"
                        )
                    v0 = end - count
                    ref = resource.pointer_at(event.source_offset + 4)
                    if ref is None or ref.asset_id != resource.file_id:
                        raise falsify(
                            f"binding {binding_index}: unresolved/external VTX source"
                        )
                    if ref.offset + count * 16 > len(resource.payload):
                        raise falsify(f"binding {binding_index}: VTX source truncated")
                    for index in range(count):
                        x, y, z, s, t, rgba = decode_vertex(
                            resource, ref.offset + index * 16
                        )
                        dense_index = len(vertices)
                        vertices.append(
                            DenseVertex(
                                x, y, z, s, t, binding_index, v0 + index, rgba
                            )
                        )
                        slots[v0 + index] = dense_index
                    source_vertices += count
                elif op in (OP_TRI1, OP_TRI2):
                    if current_run is None:
                        if not texture_prepare_valid:
                            current_epoch = len(epochs)
                            epochs.append(
                                TextureEpoch(
                                    event.source_offset,
                                    asset_index[resource.file_id],
                                    INVALID_U8,
                                    material_seen,
                                    0,
                                )
                            )
                            texture_prepare_valid = True
                        if current_epoch == INVALID_U8:
                            raise falsify(f"binding {binding_index}: run has no epoch")
                        current_run = {
                            "first_corner": len(corners),
                            "triangles": 0,
                            "epoch": current_epoch,
                            "classes": set(),
                        }
                    triangle_commands += 1
                    for indices in decode_triangles(op, event.w0, event.w1):
                        dense_indices = []
                        for cache_slot in indices:
                            if cache_slot not in slots:
                                raise falsify(
                                    f"binding {binding_index}: triangle uses unloaded "
                                    f"cache slot {cache_slot}"
                                )
                            dense_indices.append(slots[cache_slot])
                        source_z = (state.geometry_mode & GEOMETRY_ZBUFFER) != 0
                        if not source_z:
                            submit_class = SUBMIT_PROJECTED_NO_Z
                        else:
                            raw_fit = all(
                                -2048 <= coordinate <= 2047
                                for dense_index in dense_indices
                                for coordinate in (
                                    vertices[dense_index].x,
                                    vertices[dense_index].y,
                                    vertices[dense_index].z,
                                )
                            )
                            submit_class = (
                                SUBMIT_RAW_CURRENT
                                if raw_fit
                                else SUBMIT_PROJECTED_RANGE_OR_MATRIX
                            )
                        classes = current_run["classes"]
                        assert isinstance(classes, set)
                        classes.add(submit_class)
                        corners.extend(dense_indices)
                        current_run["triangles"] = int(current_run["triangles"]) + 1
                        triangle_count += 1
            finish_run()

            binding_run_count = len(runs) - binding_first_run
            binding_epoch_count = len(epochs) - first_epoch
            source_commands = len(events)
            bindings.append(
                StageBinding(
                    root,
                    event_checksum(resource, events),
                    first_vertex,
                    binding_first_run,
                    first_epoch,
                    source_commands,
                    vertex_commands,
                    source_vertices,
                    triangle_commands,
                    triangle_count,
                    binding_run_count,
                    binding_epoch_count,
                    asset_index[resource.file_id],
                    material_index,
                )
            )
            source_command_total += source_commands
            vertex_command_total += vertex_commands
            triangle_command_total += triangle_commands
            modify_vertex_total += modify_vertex_commands

        dobj_first, dobj_count = segment_dobj_spans[owner.owner]
        segments.append(
            StageSegment(
                dobj_first,
                dobj_count,
                owner.owner,
                owner.link,
                first_binding,
                len(roots),
                1 if owner.link == 6 else 0,
                first_run,
                len(runs) - first_run,
            )
        )

    packet = Packet(
        assets,
        tuple(segments),
        tuple(dobjs),
        tuple(bindings),
        tuple(runs),
        tuple(vertices),
        tuple(corners),
        tuple(epochs),
        materials,
        tuple(policies),
        source_command_total,
        vertex_command_total,
        triangle_command_total,
    )
    if modify_vertex_total != EXPECTED_MODIFY_VERTEX_COMMANDS:
        raise falsify(
            f"MODIFYVTX commands {modify_vertex_total} != "
            f"{EXPECTED_MODIFY_VERTEX_COMMANDS}"
        )
    validate_packet(packet)
    return packet


def validate_packet(packet: Packet) -> None:
    counts = (
        len(packet.segments),
        len(packet.dobjs),
        len(packet.bindings),
        packet.source_command_count,
        packet.vertex_command_count,
        sum(binding.source_vertex_count for binding in packet.bindings),
        packet.triangle_command_count,
        len(packet.corners) // 3,
        len(packet.runs),
        len(packet.epochs),
        len(packet.materials),
    )
    expected = (
        EXPECTED_CALLBACKS,
        EXPECTED_DOBJS,
        EXPECTED_BINDINGS,
        EXPECTED_COMMANDS,
        EXPECTED_VERTEX_COMMANDS,
        EXPECTED_SOURCE_VERTICES,
        EXPECTED_TRIANGLE_COMMANDS,
        EXPECTED_TRIANGLES,
        EXPECTED_RUNS,
        EXPECTED_TEXTURE_EPOCHS,
        EXPECTED_MATERIAL_EVENTS,
    )
    if counts != expected:
        raise falsify(f"packet counts {counts} != {expected}")
    if len(packet.vertices) != EXPECTED_DENSE_VERTICES:
        raise falsify(
            f"dense vertices {len(packet.vertices)} != {EXPECTED_DENSE_VERTICES}"
        )
    if len(packet.corners) != EXPECTED_TRIANGLES * 3:
        raise falsify("corner count is not three per triangle")
    class_counts = (
        sum(run.triangle_count for run in packet.runs if run.submit_class == 0),
        sum(run.triangle_count for run in packet.runs if run.submit_class == 3),
        sum(run.triangle_count for run in packet.runs if run.submit_class == 6),
    )
    if class_counts != EXPECTED_SUBMIT_CLASSES:
        raise falsify(
            f"submit classes {class_counts} != {EXPECTED_SUBMIT_CLASSES}"
        )
    expected_segments = (
        (OWNER_LAYER0, 4, 0, 20, 0, 26),
        (OWNER_MAP0, 4, 20, 1, 26, 1),
        (OWNER_MAP1, 4, 21, 4, 27, 4),
        (OWNER_MAP2, 4, 25, 4, 31, 4),
        (OWNER_LAYER1, 6, 29, 1, 35, 6),
        (OWNER_LAYER2, 13, 30, 3, 41, 3),
        (OWNER_MAP3, 16, 33, 6, 44, 6),
        (OWNER_LAYER3, 17, 39, 3, 50, 4),
    )
    actual_segments = tuple(
        (
            segment.owner,
            segment.link,
            segment.first_binding,
            segment.binding_count,
            segment.first_run,
            segment.run_count,
        )
        for segment in packet.segments
    )
    if actual_segments != expected_segments:
        raise falsify(f"segment partition {actual_segments} != {expected_segments}")
    if tuple(event.source_command_count for event in packet.materials) != (
        3,
        3,
        10,
        10,
    ):
        raise falsify("material event command partition changed")
    if packet.slab_bytes() > MAX_SLAB_BYTES:
        raise falsify(
            f"const slab {packet.slab_bytes()} exceeds {MAX_SLAB_BYTES} bytes"
        )
    for binding_index, binding in enumerate(packet.bindings):
        if binding.first_run + binding.run_count > len(packet.runs):
            raise falsify(f"binding {binding_index}: run span is out of range")
        if binding.first_epoch + binding.texture_epoch_count > len(packet.epochs):
            raise falsify(f"binding {binding_index}: epoch span is out of range")
        for run in packet.runs[
            binding.first_run : binding.first_run + binding.run_count
        ]:
            if run.binding_index != binding_index:
                raise falsify(f"binding {binding_index}: noncontiguous run ownership")
            if run.first_corner + run.triangle_count * 3 > len(packet.corners):
                raise falsify(f"binding {binding_index}: run corner span is invalid")
            for dense_index in packet.corners[
                run.first_corner : run.first_corner + run.triangle_count * 3
            ]:
                if dense_index >= len(packet.vertices):
                    raise falsify(f"binding {binding_index}: corner is out of range")
                matrix_binding = packet.vertices[dense_index].matrix_binding
                if matrix_binding > binding_index:
                    raise falsify(
                        f"binding {binding_index}: vertex depends on future matrix "
                        f"binding {matrix_binding}"
                    )
    if any(epoch.policy_index >= len(packet.policies) for epoch in packet.epochs):
        raise falsify("texture epoch has no compiled state policy")


def c_u8(value: int) -> str:
    return f"{value}u"


def c_u16(value: int) -> str:
    return f"0x{value:04x}u"


def c_u32(value: int) -> str:
    return f"0x{value:08x}u"


def render_rows(type_name: str, array_name: str, rows: Sequence[str]) -> list[str]:
    lines = [f"static const {type_name} {array_name}[{len(rows)}] = {{"]
    lines.extend(f"    {row}," for row in rows)
    lines.extend(("};", ""))
    return lines


def render_include(packet: Packet) -> bytes:
    lines = [
        "/* Generated by scripts/generate_nds_native_stage.py.  Do not edit. */",
        "/* Host-exact transaction data only; no runtime path is enabled here. */",
        "",
        f"#define NDS_NATIVE_STAGE_ASSET_COUNT {len(packet.assets)}u",
        f"#define NDS_NATIVE_STAGE_SEGMENT_COUNT {len(packet.segments)}u",
        f"#define NDS_NATIVE_STAGE_DOBJ_COUNT {len(packet.dobjs)}u",
        f"#define NDS_NATIVE_STAGE_BINDING_COUNT {len(packet.bindings)}u",
        f"#define NDS_NATIVE_STAGE_SOURCE_COMMAND_COUNT {packet.source_command_count}u",
        f"#define NDS_NATIVE_STAGE_VERTEX_COMMAND_COUNT {packet.vertex_command_count}u",
        f"#define NDS_NATIVE_STAGE_SOURCE_VERTEX_COUNT "
        f"{sum(row.source_vertex_count for row in packet.bindings)}u",
        f"#define NDS_NATIVE_STAGE_CACHE_CLONE_COUNT "
        f"{len(packet.vertices) - sum(row.source_vertex_count for row in packet.bindings)}u",
        f"#define NDS_NATIVE_STAGE_DENSE_VERTEX_COUNT {len(packet.vertices)}u",
        f"#define NDS_NATIVE_STAGE_TRIANGLE_COMMAND_COUNT {packet.triangle_command_count}u",
        f"#define NDS_NATIVE_STAGE_TRIANGLE_COUNT {len(packet.corners) // 3}u",
        f"#define NDS_NATIVE_STAGE_CORNER_COUNT {len(packet.corners)}u",
        f"#define NDS_NATIVE_STAGE_RUN_COUNT {len(packet.runs)}u",
        f"#define NDS_NATIVE_STAGE_TEXTURE_EPOCH_COUNT {len(packet.epochs)}u",
        f"#define NDS_NATIVE_STAGE_MATERIAL_EVENT_COUNT {len(packet.materials)}u",
        f"#define NDS_NATIVE_STAGE_STATE_POLICY_COUNT {len(packet.policies)}u",
        f"#define NDS_NATIVE_STAGE_SLAB_BYTES {packet.slab_bytes()}u",
        "",
        "typedef struct NDSNativeStageAsset {",
        "    u32 asset_id;",
        "    u32 payload_size;",
        "    u32 payload_checksum;",
        "    u32 flags;",
        "} NDSNativeStageAsset;",
        "",
        "typedef struct NDSNativeStageSegment {",
        "    u16 first_dobj;",
        "    u8 dobj_count;",
        "    u8 owner;",
        "    u8 link;",
        "    u8 first_binding;",
        "    u8 binding_count;",
        "    u8 initial_geometry;",
        "    u16 first_run;",
        "    u8 run_count;",
        "    u8 reserved;",
        "} NDSNativeStageSegment;",
        "",
        "typedef struct NDSNativeStageDObj {",
        "    u32 source_checksum;",
        "    u16 parent_index;",
        "    u16 binding_index;",
        "    u16 transform_flags;",
        "    u8 owner;",
        "    u8 depth;",
        "} NDSNativeStageDObj;",
        "",
        "typedef struct NDSNativeStageBinding {",
        "    u32 root_offset;",
        "    u32 traversal_checksum;",
        "    u16 first_vertex;",
        "    u16 first_run;",
        "    u16 first_epoch;",
        "    u16 source_command_count;",
        "    u8 vertex_command_count;",
        "    u8 source_vertex_count;",
        "    u8 triangle_command_count;",
        "    u8 triangle_count;",
        "    u8 run_count;",
        "    u8 texture_epoch_count;",
        "    u8 asset_index;",
        "    u8 material_event;",
        "} NDSNativeStageBinding;",
        "",
        "typedef struct NDSNativeStageRun {",
        "    u16 first_corner;",
        "    u8 triangle_count;",
        "    u8 binding_index;",
        "    u8 texture_epoch;",
        "    u8 submit_class;",
        "    u8 state_policy;",
        "    u8 flags;",
        "} NDSNativeStageRun;",
        "",
        "typedef struct NDSNativeStageDenseVertex {",
        "    s16 x;",
        "    s16 y;",
        "    s16 z;",
        "    s16 s;",
        "    s16 t;",
        "    u8 matrix_binding;",
        "    u8 cache_slot;",
        "    u32 rgba;",
        "} NDSNativeStageDenseVertex;",
        "",
        "typedef struct NDSNativeStageTextureEpoch {",
        "    u32 source_command_offset;",
        "    u8 asset_index;",
        "    u8 policy_index;",
        "    u8 material_event;",
        "    u8 flags;",
        "} NDSNativeStageTextureEpoch;",
        "",
        "typedef struct NDSNativeStageMaterialEvent {",
        "    u32 mobj_offset;",
        "    u16 binding_index;",
        "    u8 asset_index;",
        "    u8 material_slot;",
        "    u8 segment_index;",
        "    u8 source_command_count;",
        "    u16 flags;",
        "} NDSNativeStageMaterialEvent;",
        "",
        "typedef struct NDSNativeStageStatePolicy {",
        "    u32 state_hash;",
        "    u32 texture_hash;",
        "    u32 combine_w0;",
        "    u32 combine_w1;",
        "    u32 othermode_h;",
        "    u32 othermode_l;",
        "    u32 geometry_mode;",
        "} NDSNativeStageStatePolicy;",
        "",
        '_Static_assert(sizeof(NDSNativeStageAsset) == 16u, "stage asset ABI");',
        '_Static_assert(sizeof(NDSNativeStageSegment) == 12u, "stage segment ABI");',
        '_Static_assert(sizeof(NDSNativeStageDObj) == 12u, "stage DObj ABI");',
        '_Static_assert(sizeof(NDSNativeStageBinding) == 24u, "stage binding ABI");',
        '_Static_assert(sizeof(NDSNativeStageRun) == 8u, "stage run ABI");',
        '_Static_assert(sizeof(NDSNativeStageDenseVertex) == 16u, "stage vertex ABI");',
        '_Static_assert(sizeof(NDSNativeStageTextureEpoch) == 8u, "stage epoch ABI");',
        '_Static_assert(sizeof(NDSNativeStageMaterialEvent) == 12u, "stage material ABI");',
        '_Static_assert(sizeof(NDSNativeStageStatePolicy) == 28u, "stage policy ABI");',
        '_Static_assert(NDS_NATIVE_STAGE_SLAB_BYTES <= 16384u, "stage slab exceeds 16 KiB");',
        "",
    ]

    lines.extend(
        render_rows(
            "NDSNativeStageAsset",
            "sNdsNativeStageAssets",
            [
                "{ "
                + ", ".join(
                    (
                        c_u32(asset.asset_id),
                        c_u32(asset.payload_size),
                        c_u32(asset.payload_checksum),
                        c_u32(asset.flags),
                    )
                )
                + " }"
                for asset in packet.assets
            ],
        )
    )
    lines.extend(
        render_rows(
            "NDSNativeStageSegment",
            "sNdsNativeStageSegments",
            [
                "{ "
                + ", ".join(
                    (
                        c_u16(row.first_dobj),
                        c_u8(row.dobj_count),
                        c_u8(row.owner),
                        c_u8(row.link),
                        c_u8(row.first_binding),
                        c_u8(row.binding_count),
                        c_u8(row.initial_geometry),
                        c_u16(row.first_run),
                        c_u8(row.run_count),
                        "0u",
                    )
                )
                + " }"
                for row in packet.segments
            ],
        )
    )
    lines.extend(
        render_rows(
            "NDSNativeStageDObj",
            "sNdsNativeStageDObjs",
            [
                "{ "
                + ", ".join(
                    (
                        c_u32(row.source_checksum),
                        c_u16(row.parent_index),
                        c_u16(row.binding_index),
                        c_u16(row.transform_flags),
                        c_u8(row.owner),
                        c_u8(row.depth),
                    )
                )
                + " }"
                for row in packet.dobjs
            ],
        )
    )
    lines.extend(
        render_rows(
            "NDSNativeStageBinding",
            "sNdsNativeStageBindings",
            [
                "{ "
                + ", ".join(
                    (
                        c_u32(row.root_offset),
                        c_u32(row.traversal_checksum),
                        c_u16(row.first_vertex),
                        c_u16(row.first_run),
                        c_u16(row.first_epoch),
                        c_u16(row.source_command_count),
                        c_u8(row.vertex_command_count),
                        c_u8(row.source_vertex_count),
                        c_u8(row.triangle_command_count),
                        c_u8(row.triangle_count),
                        c_u8(row.run_count),
                        c_u8(row.texture_epoch_count),
                        c_u8(row.asset_index),
                        c_u8(row.material_event),
                    )
                )
                + " }"
                for row in packet.bindings
            ],
        )
    )
    lines.extend(
        render_rows(
            "NDSNativeStageRun",
            "sNdsNativeStageRuns",
            [
                "{ "
                + ", ".join(
                    (
                        c_u16(row.first_corner),
                        c_u8(row.triangle_count),
                        c_u8(row.binding_index),
                        c_u8(row.texture_epoch),
                        c_u8(row.submit_class),
                        c_u8(row.state_policy),
                        c_u8(row.flags),
                    )
                )
                + " }"
                for row in packet.runs
            ],
        )
    )
    lines.extend(
        render_rows(
            "NDSNativeStageDenseVertex",
            "sNdsNativeStageVertices",
            [
                "{ "
                + ", ".join(
                    (
                        str(row.x),
                        str(row.y),
                        str(row.z),
                        str(row.s),
                        str(row.t),
                        c_u8(row.matrix_binding),
                        c_u8(row.cache_slot),
                        c_u32(row.rgba),
                    )
                )
                + " }"
                for row in packet.vertices
            ],
        )
    )
    corner_lines = ["static const u16 sNdsNativeStageCorners[606] = {"]
    for start in range(0, len(packet.corners), 12):
        corner_lines.append(
            "    "
            + ", ".join(c_u16(value) for value in packet.corners[start : start + 12])
            + ","
        )
    corner_lines.extend(("};", ""))
    lines.extend(corner_lines)
    lines.extend(
        render_rows(
            "NDSNativeStageTextureEpoch",
            "sNdsNativeStageTextureEpochs",
            [
                "{ "
                + ", ".join(
                    (
                        c_u32(row.source_command_offset),
                        c_u8(row.asset_index),
                        c_u8(row.policy_index),
                        c_u8(row.material_event),
                        c_u8(row.flags),
                    )
                )
                + " }"
                for row in packet.epochs
            ],
        )
    )
    lines.extend(
        render_rows(
            "NDSNativeStageMaterialEvent",
            "sNdsNativeStageMaterialEvents",
            [
                "{ "
                + ", ".join(
                    (
                        c_u32(row.mobj_offset),
                        c_u16(row.binding_index),
                        c_u8(row.asset_index),
                        c_u8(row.material_slot),
                        c_u8(row.segment_index),
                        c_u8(row.source_command_count),
                        c_u16(row.flags),
                    )
                )
                + " }"
                for row in packet.materials
            ],
        )
    )
    lines.extend(
        render_rows(
            "NDSNativeStageStatePolicy",
            "sNdsNativeStageStatePolicies",
            [
                "{ "
                + ", ".join(
                    (
                        c_u32(row.state_hash),
                        c_u32(row.texture_hash),
                        c_u32(row.combine_w0),
                        c_u32(row.combine_w1),
                        c_u32(row.othermode_h),
                        c_u32(row.othermode_l),
                        c_u32(row.geometry_mode),
                    )
                )
                + " }"
                for row in packet.policies
            ],
        )
    )
    return ("\n".join(lines) + "\n").encode("ascii")


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="checkout containing the pinned BattleShip/O2R inputs",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help=f"generated include; default {DEFAULT_OUTPUT.as_posix()}",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="compare the existing include without writing it",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    repo_root = args.repo_root.resolve()
    output = args.output or (repo_root / DEFAULT_OUTPUT)
    if not output.is_absolute():
        output = repo_root / output
    try:
        packet = generate(repo_root)
        rendered = render_include(packet)
        rendered_hash = sha256(rendered)
        if (
            EXPECTED_INCLUDE_SHA256 != "TO_BE_FILLED"
            and rendered_hash != EXPECTED_INCLUDE_SHA256
        ):
            raise falsify(
                f"generated include SHA256 {rendered_hash} != "
                f"{EXPECTED_INCLUDE_SHA256}"
            )
        if args.check:
            if not output.is_file():
                raise falsify(f"generated include is absent: {output}")
            if output.read_bytes() != rendered:
                raise falsify(f"generated include is stale: {output}")
        else:
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_bytes(rendered)
    except (Falsifier, OSError, struct.error, UnicodeError, ValueError) as exc:
        print(f"M3_NATIVE_STAGE_GENERATION_FAIL: {exc}", file=sys.stderr)
        return 1
    print(
        "M3_NATIVE_STAGE_GENERATION_OK "
        f"callbacks={len(packet.segments)} dobjs={len(packet.dobjs)} "
        f"bindings={len(packet.bindings)} commands={packet.source_command_count} "
        f"source_vertices={sum(row.source_vertex_count for row in packet.bindings)} "
        f"dense_vertices={len(packet.vertices)} runs={len(packet.runs)} "
        f"epochs={len(packet.epochs)} triangles={len(packet.corners) // 3} "
        f"policies={len(packet.policies)} slab_bytes={packet.slab_bytes()} "
        f"sha256={rendered_hash}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
