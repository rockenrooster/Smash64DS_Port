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
import json
import re
import struct
import sys
from dataclasses import astuple, dataclass
from pathlib import Path
from typing import Iterable, Sequence


DEFAULT_OUTPUT = Path("src/nds/nds_native_stage_owner.generated.inc")
DEFAULT_CONSUMED_FIELDS_OUTPUT = Path(
    "docs/optimization/NDS_NATIVE_STAGE_CONSUMED_FIELDS.generated.json"
)
MAX_SLAB_BYTES = 16 * 1024

FIELD_CLASS_IMMUTABLE = "immutable_generation"
FIELD_CLASS_CAMERA = "live_camera_dependent"
FIELD_CLASS_LIVE = "live_camera_independent"
FIELD_CLASS_CALLBACK = "callback_visible_mutation_output"
FIELD_CLASSES = (
    FIELD_CLASS_IMMUTABLE,
    FIELD_CLASS_CAMERA,
    FIELD_CLASS_LIVE,
    FIELD_CLASS_CALLBACK,
)

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
EXPECTED_PROJECTED_CROSS_MATRIX_RUNS = 5
EXPECTED_PROJECTED_CROSS_MATRIX_TRIANGLES = 10
EXPECTED_PROJECTED_CROSS_MATRIX_FOREIGN_CORNERS = 15
EXPECTED_STATE_EVENTS = 423
EXPECTED_STATE_DELTAS = 148
EXPECTED_SYNC_EVENTS = 223
EXPECTED_STATE_SPANS = EXPECTED_RUNS + EXPECTED_BINDINGS

PRODUCTION_PACKET_ABI = 0x4D335031
PRODUCTION_SYMBOL_BYTES = 4

GENERATED_SEGMENT_INDEX = 0
LIVE_OPERAND_ASSET_BASES = 0
LIVE_OPERAND_BINDING_COMPOSED = 1
LIVE_OPERAND_MATERIALS = 2
LIVE_OPERAND_CONFIG = 3
LIVE_OPERAND_COUNT = 4
GENERATED_SEGMENT_COLD_BYTES = 40
GENERATED_SEGMENT_HOT_ROW_BYTES = 2

# Filled after the first independently checked generation.  Keeping the
# packet hash outside the generated file avoids a self-referential checksum.
EXPECTED_INCLUDE_SHA256 = "f055605a13ab93ca9ba1dd378da32770d5fe8984b07be6b246ce8b6e1a6c18a2"

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

RUN_FLAG_PROJECTED_CROSS_MATRIX = 1 << 0

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

SYNC_OPS = frozenset((OP_RDPLOADSYNC, OP_RDPPIPESYNC, OP_RDPTILESYNC))

STATE_EFFECT_OTHERMODE = 2
STATE_EFFECT_COMBINE = 3
STATE_EFFECT_TEXTURE = 4
STATE_EFFECT_GEOMETRY = 5
STATE_EFFECT_IMAGE = 6
STATE_EFFECT_TILE = 7
STATE_EFFECT_LOAD_TLUT = 8
STATE_EFFECT_LOAD_BLOCK = 9
STATE_EFFECT_TILE_SIZE = 10
STATE_EFFECT_PRIM = 11
STATE_EFFECT_BLEND = 12
STATE_EFFECT_MATERIAL = 13
GENERATED_SEGMENT0_EFFECT_MACROS = {
    STATE_EFFECT_OTHERMODE: "OTHERMODE",
    STATE_EFFECT_COMBINE: "COMBINE",
    STATE_EFFECT_TEXTURE: "TEXTURE",
    STATE_EFFECT_GEOMETRY: "GEOMETRY",
    STATE_EFFECT_IMAGE: "IMAGE",
    STATE_EFFECT_TILE: "TILE",
    STATE_EFFECT_LOAD_TLUT: "LOAD_TLUT",
    STATE_EFFECT_LOAD_BLOCK: "LOAD_BLOCK",
    STATE_EFFECT_TILE_SIZE: "TILE_SIZE",
    STATE_EFFECT_BLEND: "BLEND",
}

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


def _classified(classification: str, fields: str) -> dict[str, str]:
    return {field: classification for field in fields.split()}


# The field certificate deliberately follows named production closures instead
# of line numbers. Every pointer-field access in a closure must be classified;
# tracked_bases additionally pins the expected primary inputs so a rename or
# removal fails with a direct diagnostic.
SOURCE_CLOSURE_POLICIES = (
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativeStageValidateTopologyFull",
        "tracked_bases": (
            "binding", "dense", "epoch", "event", "expected", "frame",
            "live", "run", "segment",
        ),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                binding.asset_index binding.first_epoch binding.first_run
                binding.first_vertex binding.material_event binding.root_offset
                binding.run_count binding.source_vertex_count
                binding.texture_epoch_count dense.matrix_binding dense.rgba
                dense.x dense.y dense.z epoch.asset_index epoch.material_event
                epoch.policy_index epoch.source_command_offset event.asset_index
                event.binding_index event.material_slot event.mobj_offset
                event.segment_index event.source_command_count
                expected.binding_index expected.depth expected.owner
                expected.parent_index expected.transform_flags frame.asset_bases
                frame.binding_display_lists frame.dobjs
                frame.topology_generation frame.topology_stamp live.binding_index
                live.depth live.identity live.owner live.parent_index
                live.transform_flags run.binding_index run.first_corner run.flags
                run.state_policy run.submit_class run.texture_epoch
                run.triangle_count segment.binding_count segment.dobj_count
                segment.first_binding segment.first_dobj segment.first_run
                segment.reserved segment.run_count
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                summary.cross_foreign_corners summary.cross_runs
                summary.cross_triangles summary.projected_no_z_triangles
                summary.projected_range_triangles summary.raw_triangles
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativeStageApplyStateSpan",
        "tracked_bases": ("delta", "frame", "span"),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                delta.asset_index delta.effect delta.material_command
                delta.material_event delta.w0 delta.w1 frame.asset_bases
                span.first_state span.state_count span.sync_count
                """,
            ),
            **_classified(FIELD_CLASS_LIVE, "frame.materials"),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                stats.blend_color stats.color_command_count
                stats.sync_command_count
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativeApplyMaterial",
        "tracked_bases": ("material",),
        "fields": {
            **_classified(
                FIELD_CLASS_LIVE,
                """
                material.blend_color material.block_image
                material.block_image_w0 material.command_count
                material.current_image material.current_image_w0
                material.effects material.env_color material.light1
                material.light2 material.load_block_w0 material.load_block_w1
                material.palette_image material.palette_image_w0
                material.palette_tile_w0 material.palette_tile_w1
                material.palette_tlut_w1 material.prim_w0 material.prim_w1
                material.render_tile_size_w0 material.render_tile_size_w1
                material.scroll_tile_size_w0 material.scroll_tile_size_w1
                material.sync_count material.texture_w0 material.texture_w1
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                stats.blend_color stats.branch_call_count
                stats.branch_command_count stats.branch_jump_count
                stats.color_command_count stats.command_count
                stats.end_command_count stats.env_color stats.prim_color
                stats.prim_lod_fraction stats.prim_min_level
                stats.segment_resolve_count stats.sync_command_count
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativeStagePolicyMatches",
        "tracked_bases": ("policy", "stats"),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                policy.combine_w0 policy.combine_w1 policy.geometry_mode
                policy.othermode_h policy.othermode_l
                """,
            ),
            **_classified(
                FIELD_CLASS_LIVE,
                """
                stats.geometry_mode stats.othermode_h stats.othermode_l
                stats.texture_combine_w0 stats.texture_combine_w1
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativeStagePrepareRun",
        "tracked_bases": ("dense", "frame", "render_tile", "run", "stats"),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                dense.matrix_binding dense.rgba dense.s dense.t
                run.first_corner run.state_policy run.submit_class
                run.texture_epoch
                """,
            ),
            **_classified(FIELD_CLASS_CAMERA, "frame.binding_composed"),
            **_classified(
                FIELD_CLASS_LIVE,
                """
                frame.config render_tile.uls render_tile.ult
                stats.blend_color stats.othermode_l stats.texture_scale_s
                stats.texture_scale_t stats.texture_state_flags
                stats.texture_tiles
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                prepared.alpha_ref prepared.alpha_test prepared.poly_fmt
                prepared.texture_entry prepared.texture_format
                prepared.texture_height prepared.texture_name
                prepared.texture_params prepared.texture_width
                prepared.textured prepared_dense.near_inside
                prepared_dense.packed_color prepared_dense.s prepared_dense.t
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererPrepareNativeStageOwner",
        "tracked_bases": ("binding", "frame", "segment"),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                binding.first_run binding.run_count
                frame.asset_bases frame.binding_display_lists frame.dobjs
                segment.binding_count segment.first_binding
                segment.initial_geometry
                """,
            ),
            **_classified(
                FIELD_CLASS_CAMERA,
                "frame.binding_composed frame.projection",
            ),
            **_classified(
                FIELD_CLASS_LIVE,
                "frame.config frame.materials",
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                stats.command_count stats.sync_command_count
                stats.triangle_command_count stats.vertex_command_count
                stats.vertex_count
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererCommitNativeStageSegment",
        "tracked_bases": ("run", "segment"),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                run.binding_index run.first_corner run.submit_class
                run.triangle_count
                segment.first_run segment.owner segment.run_count
                """,
            ),
            **_classified(FIELD_CLASS_CALLBACK, "stats.triangle_count"),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererHardwareResolveOrBindTexture",
        "tracked_bases": (
            "config", "entry", "load", "primary_load", "render_tile",
            "stats", "tile",
        ),
        "fields": {
            **_classified(
                FIELD_CLASS_LIVE,
                """
                config.texture_data_layout entry.green_texels entry.key
                entry.key_generation entry.key_hash entry.last_used_frame
                entry.name entry.nonwhite_texels entry.params entry.pinned
                entry.profile_height entry.profile_width entry.ready
                entry.source_texels load.image load.image_format
                load.image_size load.image_width load.load_dxt load.load_kind
                load.load_lrs load.load_texels load.load_tile load.load_uls
                load.load_ult primary_load.image primary_load.image_format
                primary_load.image_size primary_load.image_width
                primary_load.load_dxt primary_load.load_kind
                primary_load.load_lrs primary_load.load_texels
                primary_load.load_tile primary_load.load_uls
                primary_load.load_ult render_tile.cms render_tile.cmt
                render_tile.flags render_tile.format render_tile.height
                render_tile.line render_tile.lrs render_tile.lrt
                render_tile.masks render_tile.maskt render_tile.palette
                render_tile.set_seen render_tile.shifts render_tile.shiftt
                render_tile.size render_tile.tmem render_tile.uls
                render_tile.ult render_tile.width stats.hardware_texture_format
                stats.hardware_texture_height stats.hardware_texture_ready_count
                stats.hardware_texture_upload_count stats.hardware_texture_width
                stats.prim_lod_fraction stats.texture_combine_w0
                stats.texture_combine_w1 stats.texture_format
                stats.texture_image stats.texture_image_width
                stats.texture_load_block_dxt stats.texture_load_block_lrs
                stats.texture_load_block_uls stats.texture_load_block_ult
                stats.texture_load_kind stats.texture_load_texels
                stats.texture_load_tile stats.texture_size
                stats.texture_state_flags stats.texture_tiles
                stats.texture_tlut_count stats.texture_tlut_image tile.cms
                tile.cmt tile.line tile.lrs tile.lrt tile.masks tile.maskt
                tile.palette tile.shifts tile.shiftt tile.tmem tile.uls tile.ult
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                resolved.entry resolved.format resolved.height resolved.name
                resolved.params resolved.width
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererHardwareColorSource",
        "tracked_bases": ("stats",),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            "stats.env_color stats.prim_color stats.texture_combine_count",
        ),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererHardwareAlphaUsesVertex",
        "tracked_bases": ("stats",),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            "stats.othermode_l stats.texture_combine_count",
        ),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererHardwareUseMaterialColor",
        "tracked_bases": ("stats",),
        "fields": _classified(
            FIELD_CLASS_LIVE, "stats.texture_combine_count"
        ),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererHardwareUseVertexColor",
        "tracked_bases": ("stats",),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            """
            stats.texture_combine_count stats.texture_combine_w0
            stats.texture_combine_w1
            """,
        ),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererHardwareAlpha",
        "tracked_bases": ("stats", "vtx"),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            """
            stats.env_color stats.othermode_l stats.prim_color
            stats.texture_combine_count vtx.a
            """,
        ),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererHardwarePolyFmt",
        "tracked_bases": ("stats",),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            "stats.geometry_mode stats.texture_combine_count",
        ),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererHardwareUseTexture",
        "tracked_bases": ("stats",),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            """
            stats.texture_combine_count stats.texture_combine_w0
            stats.texture_combine_w1 stats.texture_state_flags
            """,
        ),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererHardwareTextureImplicitStateOn",
        "tracked_bases": ("render_tile", "stats"),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            """
            render_tile.height render_tile.line render_tile.set_seen
            render_tile.size_seen render_tile.width stats.texture_image
            stats.texture_load_texels stats.texture_mask
            stats.texture_state_flags stats.texture_tiles
            """,
        ),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererActiveTextureTile",
        "tracked_bases": ("stats",),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            "stats.texture_state_flags stats.texture_tile",
        ),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererHardwareTextureFilterOffset",
        "tracked_bases": ("stats",),
        "fields": _classified(FIELD_CLASS_LIVE, "stats.othermode_h"),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererStageTextureSiteRemember",
        "tracked_bases": ("entry", "state", "stats"),
        "fields": {
            **_classified(
                FIELD_CLASS_LIVE,
                """
                entry.key.prim_lod_fraction entry.key.texel1_image
                entry.key_generation entry.ready state.source_command_site
                stats.texture_source_hash1 stats.texture_source_hash2
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                candidate.site plan.entry plan.entry_generation plan.format
                plan.height plan.prim_lod_fraction plan.semantic_key_hash
                plan.semantic_params plan.site plan.size plan.state_hash1
                plan.state_hash2 plan.texel1_image0 plan.texel1_image1
                plan.texel1_primary_state plan.texel1_tile_state
                plan.uses_texel1 plan.width
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererInitTraversalState",
        "tracked_bases": ("config", "state"),
        "fields": {
            **_classified(
                FIELD_CLASS_LIVE,
                """
                config.initial_geometry_mode config.initial_modelview
                config.initial_projection stats.geometry_mode
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                state.current_matrix_snapshot state.current_transform_vertex_mask
                state.input_vertex_valid_mask state.input_vertices
                state.matrix_generation state.matrix_snapshot_count
                state.matrix_snapshots state.matrix_valid
                state.matrix_word_valid state.modelview
                state.modelview_stack_depth state.modelview_valid
                state.prepared_light_direction_valid
                state.prepared_projected_source_z_valid_mask
                state.prepared_projected_xy_valid_mask
                state.prepared_texcoord_valid_mask
                state.prepared_vertex_color_valid_mask state.projection
                state.projection_valid state.raw_vertex_fit_mask
                state.semantic_branch_path state.semantic_command_index
                state.semantic_tri2_half state.source_command_site
                state.texture_prepare_key_hash state.texture_prepare_params
                state.texture_prepare_valid state.vertex_clip_snapshot
                state.vertex_color_valid_mask state.vertex_colors
                state.vertex_matrix_snapshot state.vertex_valid_mask
                state.vertices stats.hardware_matrix_seed_count
                vertex_storage.input_vertices vertex_storage.vertex_clip_snapshot
                vertex_storage.vertex_colors vertex_storage.vertex_matrix_snapshot
                vertex_storage.vertices
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererResolveDataPointer",
        "tracked_bases": ("config",),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            "config.resolve_data config.user config.validate_range",
        ),
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterBuildCameraMatrices",
        "tracked_bases": ("cobj", "xobj"),
        "fields": _classified(
            FIELD_CLASS_CAMERA,
            """
            cobj.projection.ortho.b cobj.projection.ortho.f
            cobj.projection.ortho.l cobj.projection.ortho.n
            cobj.projection.ortho.r cobj.projection.ortho.scale
            cobj.projection.ortho.t cobj.projection.persp.aspect
            cobj.projection.persp.far cobj.projection.persp.fovy
            cobj.projection.persp.near cobj.projection.persp.norm
            cobj.projection.persp.scale cobj.vec.at.x cobj.vec.at.y
            cobj.vec.at.z cobj.vec.eye.x cobj.vec.eye.y cobj.vec.eye.z
            cobj.vec.up.x cobj.vec.up.y cobj.vec.up.z cobj.xobjs
            cobj.xobjs_num xobj.kind
            """,
        ),
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterBuildDObjXObjMatrix",
        "tracked_bases": ("dobj", "rotate", "scale", "translate", "xobj"),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            """
            dobj.rotate.a dobj.rotate.vec.f.x dobj.rotate.vec.f.y
            dobj.rotate.vec.f.z dobj.scale.vec.f.x dobj.scale.vec.f.y
            dobj.scale.vec.f.z dobj.translate.vec.f.x
            dobj.translate.vec.f.y dobj.translate.vec.f.z rotate.a
            rotate.vec.f.x rotate.vec.f.y rotate.vec.f.z scale.vec.f.x
            scale.vec.f.y scale.vec.f.z translate.vec.f.x translate.vec.f.y
            translate.vec.f.z xobj.kind xobj.mtx
            """,
        ),
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterBuildDObjLocalMatrix",
        "tracked_bases": ("dobj",),
        "fields": _classified(
            FIELD_CLASS_IMMUTABLE, "dobj.xobjs dobj.xobjs_num"
        ),
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterApplyMvpRecalcRpy0x47",
        "tracked_bases": ("cobj", "dobj"),
        "fields": {
            **_classified(
                FIELD_CLASS_CAMERA,
                """
                cobj.projection.persp.aspect cobj.projection.persp.far
                cobj.projection.persp.fovy cobj.projection.persp.near
                cobj.projection.persp.norm cobj.projection.persp.scale
                cobj.xobjs cobj.xobjs_num modelview.m
                """,
            ),
            **_classified(
                FIELD_CLASS_LIVE,
                """
                dobj.rotate.vec.f.x dobj.rotate.vec.f.y dobj.xobjs
                dobj.xobjs_num
                """,
            ),
        },
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterMaterialFlags",
        "tracked_bases": ("mobj",),
        "fields": _classified(FIELD_CLASS_LIVE, "mobj.sub.flags"),
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterMaterialLoadBlock",
        "tracked_bases": ("mobj",),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            "mobj.sub.block_dxt mobj.sub.block_siz mobj.sub.unk36",
        ),
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterMaterialTextureState",
        "tracked_bases": ("mobj",),
        "fields": _classified(
            FIELD_CLASS_LIVE,
            """
            mobj.sub.scau mobj.sub.scav mobj.sub.scrollu mobj.sub.scrollv
            mobj.sub.trau mobj.sub.trav mobj.sub.unk10 mobj.sub.unk24
            mobj.sub.unk28 mobj.sub.unk44
            """,
        ),
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterBuildNativeMaterialSnapshot",
        "tracked_bases": ("mobj",),
        "fields": {
            **_classified(
                FIELD_CLASS_LIVE,
                """
                mobj.lfrac mobj.palette_id mobj.sub.blendcolor
                mobj.sub.block_fmt mobj.sub.block_siz mobj.sub.envcolor
                mobj.sub.fmt mobj.sub.light1color mobj.sub.light2color
                mobj.sub.palettes mobj.sub.prim_m mobj.sub.primcolor
                mobj.sub.siz mobj.sub.sprites mobj.sub.unk08
                mobj.sub.unk0A mobj.sub.unk0C mobj.sub.unk0E
                mobj.sub.unk10 mobj.sub.unk38 mobj.sub.unk3A
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                mobj.texture_id_curr mobj.texture_id_next out.blend_color
                out.block_image out.block_image_w0 out.command_count
                out.current_image out.current_image_w0 out.effects
                out.env_color out.light1 out.light2 out.load_block_w0
                out.load_block_w1 out.palette_image out.palette_image_w0
                out.palette_tile_w0 out.palette_tile_w1 out.palette_tlut_w1
                out.prim_w0 out.prim_w1 out.render_tile_size_w0
                out.render_tile_size_w1 out.scroll_tile_size_w0
                out.scroll_tile_size_w1 out.sync_count out.texture_w0
                out.texture_w1
                """,
            ),
        },
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterBuildNativeStageTopologyStamp",
        "tracked_bases": (
            "dobj", "gobj", "live", "loaded", "workspace", "xobj",
        ),
        "fields": _classified(
            FIELD_CLASS_IMMUTABLE,
            """
            dobj.child dobj.dv dobj.flags dobj.mobj dobj.parent
            dobj.parent_gobj dobj.sib_next dobj.sib_prev dobj.xobjs
            dobj.xobjs_num gobj.dl_link_id gobj.flags gobj.proc_display
            live.binding_index live.depth live.identity live.owner
            live.parent_index live.transform_flags loaded.asset_id
            loaded.data loaded.data_size loaded.owner_generation
            workspace.binding_count workspace.binding_display_lists
            workspace.binding_dobjs workspace.dobj_count workspace.dobjs
            workspace.live_dobjs workspace.loaded workspace.segments xobj.kind
            """,
        ),
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterPrepareNativeStageMatrices",
        "tracked_bases": ("workspace",),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE, "workspace.binding_dobjs"
            ),
            **_classified(
                FIELD_CLASS_CAMERA,
                "workspace.binding_composed workspace.projection",
            ),
        },
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterPrepareNativeStageMaterials",
        "tracked_bases": ("workspace",),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE, "workspace.binding_dobjs"
            ),
            **_classified(FIELD_CLASS_LIVE, "workspace.materials"),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                workspace.material_curr workspace.material_mobjs
                workspace.material_next
                """,
            ),
        },
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterCommitNativeStageMaterials",
        "tracked_bases": ("workspace",),
        "fields": _classified(
            FIELD_CLASS_CALLBACK,
            """
            workspace.material_curr workspace.material_mobjs
            workspace.material_next
            """,
        ),
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterPrepareNativeStageOwner",
        "tracked_bases": ("workspace",),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                workspace.binding_count workspace.binding_display_lists
                workspace.dobj_count workspace.frame.asset_bases
                workspace.frame.binding_display_lists workspace.frame.dobjs
                workspace.frame.topology_generation
                workspace.frame.topology_stamp
                workspace.live_dobjs workspace.loaded
                workspace.topology_generation workspace.topology_stamp
                workspace.topology_valid
                """,
            ),
            **_classified(
                FIELD_CLASS_CAMERA,
                """
                workspace.binding_composed workspace.frame.binding_composed
                workspace.frame.projection workspace.projection
                """,
            ),
            **_classified(
                FIELD_CLASS_LIVE,
                """
                workspace.config workspace.config.immutable_command_span
                workspace.config.max_commands workspace.config.max_depth
                workspace.config.max_list_commands
                workspace.config.resolve_branch workspace.config.resolve_data
                workspace.config.texture_data_layout workspace.config.user
                workspace.config.validate_range workspace.frame
                workspace.frame.config workspace.frame.materials
                workspace.materials workspace.resolver
                workspace.resolver.primary_file workspace.stats
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                "workspace.active workspace.next_segment",
            ),
        },
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterCommitNativeStageDisplay",
        "tracked_bases": ("workspace",),
        "fields": {
            **_classified(FIELD_CLASS_IMMUTABLE, "workspace.segments"),
            **_classified(
                FIELD_CLASS_CALLBACK,
                "workspace.active workspace.next_segment",
            ),
        },
    },
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterFinishNativeStageOwner",
        "tracked_bases": ("workspace",),
        "fields": _classified(
            FIELD_CLASS_CALLBACK,
            "workspace.active workspace.next_segment",
        ),
    },
)


# Task 23R's 588-field source certificate remains a stable baseline.  Task 26
# adds only these generated-program closures, so keep their reads separately
# counted while applying the same fail-closed classification audit.
TASK26_GENERATED_CLOSURE_POLICIES = (
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativeStageValidateGeneratedSegment0",
        "tracked_bases": (
            "certificate", "generated", "run", "segment", "span", "tail",
        ),
        "fields": _classified(
            FIELD_CLASS_IMMUTABLE,
            """
            certificate.asset_base_mask certificate.binding_count
            certificate.dobj_count certificate.final_tail_span
            certificate.first_binding certificate.first_dobj
            certificate.first_run certificate.first_state
            certificate.first_texture_epoch certificate.hot_checksum
            certificate.link certificate.live_operand_mask
            certificate.material_count certificate.owner
            certificate.prepared_dense_checksum
            certificate.prepared_dense_count
            certificate.prepared_dense_offset_count certificate.run_count
            certificate.segment_index certificate.source_checksum
            certificate.state_count certificate.submit_class
            certificate.sync_count certificate.table_checksum
            certificate.texture_epoch_count certificate.triangle_count
            generated.binding_composed_index generated.run_index
            run.binding_index run.flags run.submit_class run.texture_epoch
            run.triangle_count segment.binding_count segment.dobj_count
            segment.first_binding segment.first_dobj segment.first_run
            segment.link segment.owner segment.run_count span.first_state
            span.state_count span.sync_count tail.first_state
            tail.state_count tail.sync_count
            """,
        ),
        "tracked_static_bases": ("sNdsNativeStageValidationCache",),
        "static_fields": _classified(
            FIELD_CLASS_IMMUTABLE,
            """
            sNdsNativeStageValidationCache.prepared_dense_indices
            sNdsNativeStageValidationCache.prepared_dense_offsets
            """,
        ),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativeStagePrepareGeneratedSegment0",
        "tracked_bases": ("certificate", "frame", "stats"),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                certificate.material_count certificate.run_count
                certificate.texture_epoch_count certificate.triangle_count
                frame.asset_bases
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                stats.blend_color stats.color_command_count
                stats.geometry_clear_mask stats.geometry_command_count
                stats.geometry_mode stats.geometry_set_mask
                stats.sync_command_count
                """,
            ),
        },
        "tracked_static_bases": ("sNdsNativeStageValidationCache",),
        "static_fields": _classified(
            FIELD_CLASS_IMMUTABLE,
            "sNdsNativeStageValidationCache.valid",
        ),
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativeStageHashGeneratedSegment0Outputs",
        "tracked_bases": ("prepared", "state", "stats"),
        "fields": _classified(
            FIELD_CLASS_CALLBACK,
            """
            prepared.alpha_ref prepared.alpha_test prepared.near_inside
            prepared.packed_color prepared.poly_fmt prepared.s prepared.t
            prepared.texture_entry prepared.texture_format
            prepared.texture_height prepared.texture_name
            prepared.texture_params prepared.texture_width prepared.textured
            state.texture_prepare_alpha_constant
            state.texture_prepare_decal_depth state.texture_prepare_enabled
            state.texture_prepare_material_color state.texture_prepare_name
            state.texture_prepare_offset state.texture_prepare_origin_s
            state.texture_prepare_origin_t state.texture_prepare_poly_alpha
            state.texture_prepare_poly_fmt state.texture_prepare_prim_depth
            state.texture_prepare_scale_s state.texture_prepare_scale_t
            state.texture_prepare_source_zbuffered
            state.texture_prepare_valid state.texture_prepare_vertex_flags
            stats.blend_color stats.color_command_count
            stats.first_othermode_opcode stats.first_othermode_w0
            stats.first_othermode_w1 stats.geometry_clear_mask
            stats.geometry_command_count stats.geometry_mode
            stats.geometry_set_mask stats.ignored_state_command_count
            stats.othermode_command_count stats.othermode_h stats.othermode_l
            stats.state_command_count stats.sync_command_count
            stats.texture_combine_count stats.texture_combine_w0
            stats.texture_combine_w1 stats.texture_command_count
            stats.texture_image stats.texture_mask stats.texture_source_hash1
            stats.texture_source_hash2 stats.texture_state_flags
            """,
        ),
        "tracked_static_bases": (
            "sNdsNativeStageOwnerExecution",
            "sNdsNativeStageSegment0HotRuns",
            "sNdsNativeStageValidationCache",
        ),
        "static_fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                sNdsNativeStageSegment0HotRuns.run_index
                sNdsNativeStageValidationCache.prepared_dense_indices
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                "sNdsNativeStageOwnerExecution.runs",
            ),
        },
    },
)


GENERATED_RUNTIME_FIELDS = {
    "NDSNativeStageAsset": ("payload_size",),
    "NDSNativeStageSegment": (
        "first_dobj", "dobj_count", "owner", "first_binding",
        "binding_count", "initial_geometry", "first_run", "run_count",
        "reserved",
    ),
    "NDSNativeStageDObj": (
        "parent_index", "binding_index", "transform_flags", "owner", "depth",
    ),
    "NDSNativeStageBinding": (
        "root_offset", "first_vertex", "first_run", "first_epoch",
        "source_vertex_count", "run_count", "texture_epoch_count",
        "asset_index", "material_event",
    ),
    "NDSNativeStageRun": (
        "first_corner", "triangle_count", "binding_index", "texture_epoch",
        "submit_class", "state_policy", "flags",
    ),
    "NDSNativeStageDenseVertex": (
        "x", "y", "z", "s", "t", "matrix_binding", "packed_cache_shift",
        "rgba",
    ),
    "NDSNativeStageTextureEpoch": (
        "source_command_offset", "asset_index", "policy_index",
        "material_event",
    ),
    "NDSNativeStageMaterialEvent": (
        "mobj_offset", "binding_index", "asset_index", "material_slot",
        "segment_index", "source_command_count",
    ),
    "NDSNativeStageStatePolicy": (
        "combine_w0", "combine_w1", "othermode_h", "othermode_l",
        "geometry_mode",
    ),
    "NDSNativeStageStateDelta": (
        "w0", "w1", "effect", "asset_index", "material_event",
        "material_command",
    ),
    "NDSNativeStageStateSpan": ("first_state", "state_count", "sync_count"),
    "NDSNativeStageGeneratedRun": (
        "run_index", "binding_composed_index",
    ),
    "NDSNativeStageGeneratedCertificate": (
        "source_checksum", "table_checksum", "hot_checksum",
        "prepared_dense_checksum", "first_state", "state_count",
        "sync_count", "segment_index", "first_dobj", "dobj_count",
        "owner", "link", "submit_class", "first_binding",
        "binding_count", "first_run", "run_count", "first_texture_epoch",
        "triangle_count", "texture_epoch_count", "live_operand_mask",
        "asset_base_mask", "material_count", "final_tail_span",
        "prepared_dense_count", "prepared_dense_offset_count",
    ),
    "sNdsNativeStageCorners": ("all_indices",),
    "sNdsNativeStageStateSequence": ("all_indices",),
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
class StateDelta:
    w0: int
    w1: int
    effect: int
    asset_index: int
    material_event: int
    material_command: int


@dataclass(frozen=True)
class StateSpan:
    first_state: int
    state_count: int
    sync_count: int


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
    state_deltas: tuple[StateDelta, ...]
    state_sequence: tuple[int, ...]
    state_spans: tuple[StateSpan, ...]
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
            + len(self.state_deltas) * 12
            + len(self.state_sequence)
            + len(self.state_spans) * 4
            + PRODUCTION_SYMBOL_BYTES
        )


@dataclass(frozen=True)
class GeneratedStageRun:
    run_index: int
    binding_composed_index: int


@dataclass(frozen=True)
class GeneratedStageCertificate:
    source_checksum: int
    table_checksum: int
    hot_checksum: int
    prepared_dense_checksum: int
    first_state: int
    state_count: int
    sync_count: int
    segment_index: int
    first_dobj: int
    dobj_count: int
    owner: int
    link: int
    submit_class: int
    first_binding: int
    binding_count: int
    first_run: int
    run_count: int
    first_texture_epoch: int
    triangle_count: int
    texture_epoch_count: int
    live_operand_mask: int
    asset_base_mask: int
    material_count: int
    final_tail_span: int
    prepared_dense_count: int
    prepared_dense_offset_count: int


@dataclass(frozen=True)
class GeneratedStageProgram:
    certificate: GeneratedStageCertificate
    runs: tuple[GeneratedStageRun, ...]
    instructions: tuple[tuple[str, tuple[int, ...]], ...]

    def footprint_bytes(self) -> int:
        return (
            GENERATED_SEGMENT_COLD_BYTES
            + len(self.runs) * GENERATED_SEGMENT_HOT_ROW_BYTES
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


def state_effect(op: int) -> int:
    effects = {
        OP_SETOTHERMODE_H: STATE_EFFECT_OTHERMODE,
        OP_SETOTHERMODE_L: STATE_EFFECT_OTHERMODE,
        OP_RDPSETOTHERMODE: STATE_EFFECT_OTHERMODE,
        OP_SETCOMBINE: STATE_EFFECT_COMBINE,
        OP_TEXTURE: STATE_EFFECT_TEXTURE,
        OP_GEOMETRYMODE: STATE_EFFECT_GEOMETRY,
        OP_SETTIMG: STATE_EFFECT_IMAGE,
        OP_SETTILE: STATE_EFFECT_TILE,
        OP_LOADTLUT: STATE_EFFECT_LOAD_TLUT,
        OP_LOADBLOCK: STATE_EFFECT_LOAD_BLOCK,
        OP_SETTILESIZE: STATE_EFFECT_TILE_SIZE,
        OP_SETPRIMCOLOR: STATE_EFFECT_PRIM,
        OP_SETBLENDCOLOR: STATE_EFFECT_BLEND,
    }
    try:
        return effects[op]
    except KeyError as exc:
        raise falsify(f"unsupported executable state opcode 0x{op:02x}") from exc


def compile_state_delta(
    resource: O2RResource,
    event: CommandEvent,
    asset_index: dict[int, int],
) -> StateDelta:
    if event.material_event != INVALID_U8:
        return StateDelta(
            event.w0,
            event.w1,
            STATE_EFFECT_MATERIAL,
            INVALID_U8,
            event.material_event,
            event.material_command,
        )
    ref = resource.pointer_at(event.source_offset + 4)
    if ref is not None:
        if event.op != OP_SETTIMG or ref.asset_id not in asset_index:
            raise falsify(
                f"asset {resource.file_id}: unsupported state pointer "
                f"for opcode 0x{event.op:02x}"
            )
        resolved_w1 = ref.offset
        resolved_asset = asset_index[ref.asset_id]
    else:
        resolved_w1 = event.w1
        resolved_asset = INVALID_U8
    return StateDelta(
        event.w0,
        resolved_w1,
        state_effect(event.op),
        resolved_asset,
        INVALID_U8,
        INVALID_U8,
    )


def apply_state_words(state: SourceState, op: int, w0: int, w1: int) -> None:
    if op == OP_GEOMETRYMODE:
        state.geometry_mode = (state.geometry_mode & w0) | w1
    elif op == OP_SETCOMBINE:
        state.combine_w0 = w0
        state.combine_w1 = w1
    elif op == OP_RDPSETOTHERMODE:
        state.othermode_h = w0 & 0xFFFFFF
        state.othermode_l = w1
    elif op == OP_SETOTHERMODE_H:
        state.othermode_h = apply_othermode(state.othermode_h, op, w0, w1)
    elif op == OP_SETOTHERMODE_L:
        state.othermode_l = apply_othermode(state.othermode_l, op, w0, w1)


def apply_compiled_state_delta(
    assets: Sequence[StageAsset], state: SourceState, delta: StateDelta
) -> None:
    op = delta.w0 >> 24
    if delta.material_event != INVALID_U8:
        normalized = (
            0x4D415433,
            delta.material_event,
            delta.material_command,
            op,
        )
    elif delta.asset_index != INVALID_U8:
        if delta.asset_index >= len(assets):
            raise falsify("compiled state delta asset index is out of range")
        normalized = (
            op,
            delta.w0,
            assets[delta.asset_index].asset_id,
            delta.w1,
        )
    else:
        normalized = (op, delta.w0, delta.w1)
    state.state_hash = fnv1a_u32(normalized, state.state_hash)
    if op in TEXTURE_INVALIDATING_OPS:
        state.texture_hash = fnv1a_u32(normalized, state.texture_hash)
    if delta.material_event == INVALID_U8:
        apply_state_words(state, op, delta.w0, delta.w1)


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
    apply_state_words(state, op, event.w0, event.w1)


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
    state_deltas: list[StateDelta] = []
    state_delta_lookup: dict[StateDelta, int] = {}
    state_sequence: list[int] = []
    run_state_spans: list[StateSpan] = []
    tail_state_spans: list[StateSpan] = []
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
            pending_state_first = len(state_sequence)
            pending_sync_count = 0

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
                        int(current_run["flags"]),
                    )
                )
                span = current_run["state_span"]
                assert isinstance(span, StateSpan)
                run_state_spans.append(span)
                current_run = None

            for event in events:
                op = event.op
                if op not in (OP_TRI1, OP_TRI2):
                    finish_run()
                if event.material_event != INVALID_U8:
                    material_seen = event.material_event
                if op in STATE_OPS or event.material_event != INVALID_U8:
                    delta = compile_state_delta(resource, event, asset_index)
                    delta_index = state_delta_lookup.get(delta)
                    if delta_index is None:
                        delta_index = len(state_deltas)
                        if delta_index > 0xFF:
                            raise falsify("state-delta table exceeds u8 index")
                        state_delta_lookup[delta] = delta_index
                        state_deltas.append(delta)
                    state_sequence.append(delta_index)
                elif op in SYNC_OPS:
                    pending_sync_count += 1
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
                            "flags": 0,
                            "state_span": StateSpan(
                                pending_state_first,
                                len(state_sequence) - pending_state_first,
                                pending_sync_count,
                            ),
                        }
                        pending_state_first = len(state_sequence)
                        pending_sync_count = 0
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
                        if any(
                            vertices[dense_index].matrix_binding != binding_index
                            for dense_index in dense_indices
                        ):
                            current_run["flags"] = (
                                int(current_run["flags"])
                                | RUN_FLAG_PROJECTED_CROSS_MATRIX
                            )
                        corners.extend(dense_indices)
                        current_run["triangles"] = int(current_run["triangles"]) + 1
                        triangle_count += 1
            finish_run()
            tail_state_spans.append(
                StateSpan(
                    pending_state_first,
                    len(state_sequence) - pending_state_first,
                    pending_sync_count,
                )
            )

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
        tuple(state_deltas),
        tuple(state_sequence),
        tuple(run_state_spans + tail_state_spans),
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
    state_counts = (
        len(packet.state_deltas),
        len(packet.state_sequence),
        len(packet.state_spans),
        sum(span.sync_count for span in packet.state_spans),
    )
    expected_state_counts = (
        EXPECTED_STATE_DELTAS,
        EXPECTED_STATE_EVENTS,
        EXPECTED_STATE_SPANS,
        EXPECTED_SYNC_EVENTS,
    )
    if state_counts != expected_state_counts:
        raise falsify(
            f"state packet counts {state_counts} != {expected_state_counts}"
        )
    if packet.slab_bytes() > MAX_SLAB_BYTES:
        raise falsify(
            f"const slab {packet.slab_bytes()} exceeds {MAX_SLAB_BYTES} bytes"
        )
    cross_matrix_runs = 0
    cross_matrix_triangles = 0
    cross_matrix_foreign_corners = 0
    raw_cross_matrix_triangles = 0
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
            run_corners = packet.corners[
                run.first_corner : run.first_corner + run.triangle_count * 3
            ]
            run_cross_matrix_triangles = 0
            run_foreign_corners = 0
            for first_corner in range(0, len(run_corners), 3):
                triangle_crosses_matrix = False
                for dense_index in run_corners[first_corner : first_corner + 3]:
                    if dense_index >= len(packet.vertices):
                        raise falsify(
                            f"binding {binding_index}: corner is out of range"
                        )
                    matrix_binding = packet.vertices[dense_index].matrix_binding
                    if matrix_binding > binding_index:
                        raise falsify(
                            f"binding {binding_index}: vertex depends on future matrix "
                            f"binding {matrix_binding}"
                        )
                    if matrix_binding != binding_index:
                        triangle_crosses_matrix = True
                        run_foreign_corners += 1
                if triangle_crosses_matrix:
                    run_cross_matrix_triangles += 1
                    if run.submit_class == SUBMIT_RAW_CURRENT:
                        raw_cross_matrix_triangles += 1

            unknown_flags = run.flags & ~RUN_FLAG_PROJECTED_CROSS_MATRIX
            if unknown_flags:
                raise falsify(
                    f"binding {binding_index}: run has unknown flags 0x{unknown_flags:02x}"
                )
            marked_cross_matrix = (
                run.flags & RUN_FLAG_PROJECTED_CROSS_MATRIX
            ) != 0
            if marked_cross_matrix != (run_cross_matrix_triangles != 0):
                raise falsify(
                    f"binding {binding_index}: cross-matrix run flag disagrees with vertices"
                )
            if marked_cross_matrix:
                if run.submit_class != SUBMIT_PROJECTED_NO_Z:
                    raise falsify(
                        f"binding {binding_index}: cross-matrix run is not projected/no-Z"
                    )
                if run_cross_matrix_triangles != run.triangle_count:
                    raise falsify(
                        f"binding {binding_index}: cross-matrix run mixes triangle owners"
                    )
                cross_matrix_runs += 1
                cross_matrix_triangles += run_cross_matrix_triangles
                cross_matrix_foreign_corners += run_foreign_corners
    cross_matrix_counts = (
        cross_matrix_runs,
        cross_matrix_triangles,
        cross_matrix_foreign_corners,
        raw_cross_matrix_triangles,
    )
    expected_cross_matrix_counts = (
        EXPECTED_PROJECTED_CROSS_MATRIX_RUNS,
        EXPECTED_PROJECTED_CROSS_MATRIX_TRIANGLES,
        EXPECTED_PROJECTED_CROSS_MATRIX_FOREIGN_CORNERS,
        0,
    )
    if cross_matrix_counts != expected_cross_matrix_counts:
        raise falsify(
            f"cross-matrix counts {cross_matrix_counts} != "
            f"{expected_cross_matrix_counts}"
        )
    if any(epoch.policy_index >= len(packet.policies) for epoch in packet.epochs):
        raise falsify("texture epoch has no compiled state policy")
    if any(index >= len(packet.state_deltas) for index in packet.state_sequence):
        raise falsify("state sequence references an absent delta")

    valid_effects = {
        STATE_EFFECT_OTHERMODE,
        STATE_EFFECT_COMBINE,
        STATE_EFFECT_TEXTURE,
        STATE_EFFECT_GEOMETRY,
        STATE_EFFECT_IMAGE,
        STATE_EFFECT_TILE,
        STATE_EFFECT_LOAD_TLUT,
        STATE_EFFECT_LOAD_BLOCK,
        STATE_EFFECT_TILE_SIZE,
        STATE_EFFECT_PRIM,
        STATE_EFFECT_BLEND,
        STATE_EFFECT_MATERIAL,
    }
    for index, delta in enumerate(packet.state_deltas):
        op = delta.w0 >> 24
        if delta.effect not in valid_effects:
            raise falsify(f"state delta {index}: unsupported effect")
        if delta.effect == STATE_EFFECT_MATERIAL:
            if delta.material_event >= len(packet.materials):
                raise falsify(f"state delta {index}: material is out of range")
            material = packet.materials[delta.material_event]
            if (
                delta.material_command >= len(material.opcodes)
                or material.opcodes[delta.material_command] != op
                or delta.asset_index != INVALID_U8
            ):
                raise falsify(f"state delta {index}: material command changed")
        else:
            if (
                delta.effect != state_effect(op)
                or delta.material_event != INVALID_U8
                or delta.material_command != INVALID_U8
            ):
                raise falsify(f"state delta {index}: static command changed")
            if delta.effect == STATE_EFFECT_IMAGE:
                if delta.asset_index >= len(packet.assets):
                    raise falsify(f"state delta {index}: image asset is out of range")
            elif delta.asset_index != INVALID_U8:
                raise falsify(f"state delta {index}: unexpected asset binding")

    state_cursor = 0
    for binding_index, binding in enumerate(packet.bindings):
        for run_index in range(
            binding.first_run, binding.first_run + binding.run_count
        ):
            span = packet.state_spans[run_index]
            if span.first_state != state_cursor:
                raise falsify(
                    f"binding {binding_index}: run state span is not contiguous"
                )
            state_cursor += span.state_count
        tail = packet.state_spans[len(packet.runs) + binding_index]
        if tail.first_state != state_cursor:
            raise falsify(
                f"binding {binding_index}: tail state span is not contiguous"
            )
        state_cursor += tail.state_count
    if state_cursor != len(packet.state_sequence):
        raise falsify("state spans do not consume the complete sequence")
    if any(
        span.state_count > 0xFF
        or span.sync_count > 0xFF
        or span.first_state + span.state_count > len(packet.state_sequence)
        for span in packet.state_spans
    ):
        raise falsify("state span exceeds its compact ABI")


def _append_records(words: list[int], tag: int, rows: Sequence[object]) -> None:
    words.extend((tag, len(rows)))
    for row in rows:
        words.extend(int(value) for value in astuple(row))


def _append_values(words: list[int], tag: int, values: Sequence[int]) -> None:
    words.extend((tag, len(values), *(int(value) for value in values)))


def build_generated_segment0_program(packet: Packet) -> GeneratedStageProgram:
    segment = packet.segments[GENERATED_SEGMENT_INDEX]
    binding_indices = tuple(
        range(segment.first_binding, segment.first_binding + segment.binding_count)
    )
    run_indices = tuple(range(segment.first_run, segment.first_run + segment.run_count))
    bindings = tuple(packet.bindings[index] for index in binding_indices)
    runs = tuple(packet.runs[index] for index in run_indices)
    epochs = tuple(
        packet.epochs[index]
        for index in sorted({run.texture_epoch for run in runs})
    )
    run_spans = tuple(packet.state_spans[index] for index in run_indices)
    tail_indices = tuple(len(packet.runs) + index for index in binding_indices)
    tail_spans = tuple(packet.state_spans[index] for index in tail_indices)
    live_tail_indices = tuple(
        index
        for index, span in zip(tail_indices, tail_spans)
        if span.state_count != 0 or span.sync_count != 0
    )
    execution_spans = run_spans + tuple(
        packet.state_spans[index] for index in live_tail_indices
    )
    state_positions = tuple(
        position
        for span in execution_spans
        for position in range(span.first_state, span.first_state + span.state_count)
    )
    state_sequence = tuple(packet.state_sequence[index] for index in state_positions)
    state_delta_indices = tuple(sorted(set(state_sequence)))
    policy_indices = tuple(sorted({run.state_policy for run in runs}))
    corners = tuple(
        packet.corners[index]
        for run in runs
        for index in range(run.first_corner, run.first_corner + run.triangle_count * 3)
    )
    dense_indices = tuple(sorted(set(corners)))
    asset_indices = {
        epoch.asset_index for epoch in epochs
    }
    asset_indices.update(
        packet.state_deltas[index].asset_index
        for index in state_delta_indices
        if packet.state_deltas[index].effect == STATE_EFFECT_IMAGE
    )
    material_indices = {
        packet.state_deltas[index].material_event
        for index in state_delta_indices
        if packet.state_deltas[index].effect == STATE_EFFECT_MATERIAL
    }
    hot_rows = tuple(
        GeneratedStageRun(index, packet.runs[index].binding_index)
        for index in run_indices
    )
    prepared_dense_offsets = [0]
    prepared_dense_indices: list[int] = []
    prepared_dense_seen: set[int] = set()
    for run in runs:
        for corner_index in range(
            run.first_corner, run.first_corner + run.triangle_count * 3
        ):
            dense_index = packet.corners[corner_index]
            if dense_index not in prepared_dense_seen:
                prepared_dense_seen.add(dense_index)
                prepared_dense_indices.append(dense_index)
        prepared_dense_offsets.append(len(prepared_dense_indices))
    expected_run_bindings = tuple(
        binding_index
        for binding_index, binding in zip(binding_indices, bindings)
        for _ in range(binding.first_run, binding.first_run + binding.run_count)
    )

    if (
        (segment.owner, segment.link, segment.first_binding,
         segment.binding_count, segment.first_run, segment.run_count)
        != (OWNER_LAYER0, 4, 0, 20, 0, 26)
    ):
        raise falsify("generated segment-0 callback partition changed")
    if any(
        run.submit_class != SUBMIT_PROJECTED_NO_Z or run.flags != 0
        for run in runs
    ):
        raise falsify("generated segment-0 run class changed")
    if tuple(run.binding_index for run in runs) != expected_run_bindings or tuple(
        row.binding_composed_index for row in hot_rows
    ) != expected_run_bindings:
        raise falsify("generated segment-0 run-owner binding changed")
    if any(binding.material_event != INVALID_U8 for binding in bindings):
        raise falsify("generated segment-0 unexpectedly owns a material event")
    if material_indices:
        raise falsify("generated segment-0 state unexpectedly consumes materials")
    if live_tail_indices != (len(packet.runs) + binding_indices[-1],):
        raise falsify("generated segment-0 binding-tail program changed")
    if state_positions != tuple(range(123)):
        raise falsify("generated segment-0 state order changed")
    if tuple(epoch.asset_index for epoch in epochs) != (1,) * 22:
        raise falsify("generated segment-0 texture asset order changed")
    if {run.texture_epoch for run in runs} != set(range(22)):
        raise falsify("generated segment-0 texture epoch bits changed")
    if sum(run.triangle_count for run in runs) != 54:
        raise falsify("generated segment-0 triangle count changed")
    if len(prepared_dense_indices) != 108 or len(prepared_dense_offsets) != 27:
        raise falsify("generated segment-0 prepared-dense schedule changed")
    if (
        sum(
            stage_vertex_coordinate_shift(packet.vertices[index]) == 0
            for index in prepared_dense_indices
        ),
        sum(
            stage_vertex_coordinate_shift(packet.vertices[index]) == 1
            for index in prepared_dense_indices
        ),
    ) != (78, 30):
        raise falsify("generated segment-0 prepared-dense shift census changed")
    if any(
        (packet.vertices[index].rgba & 0xFF) != 0xFF
        for index in prepared_dense_indices
    ):
        raise falsify("generated segment-0 prepared-dense alpha changed")

    instructions: list[tuple[str, tuple[int, ...]]] = []

    def append_state_span(span: StateSpan) -> None:
        instructions.append(("SYNC", (span.sync_count,)))
        for position in range(
            span.first_state, span.first_state + span.state_count
        ):
            delta = packet.state_deltas[packet.state_sequence[position]]
            effect = GENERATED_SEGMENT0_EFFECT_MACROS.get(delta.effect)
            if effect is None:
                raise falsify(
                    "generated segment-0 has unsupported immutable state "
                    f"effect {delta.effect}"
                )
            if delta.effect == STATE_EFFECT_IMAGE:
                operands = (delta.asset_index, delta.w0, delta.w1)
            elif delta.effect in (STATE_EFFECT_LOAD_TLUT, STATE_EFFECT_BLEND):
                operands = (delta.w1,)
            else:
                operands = (delta.w0, delta.w1)
            instructions.append((effect, operands))

    for run_index, span in zip(run_indices, run_spans):
        append_state_span(span)
        instructions.append(("RUN", (run_index,)))
    append_state_span(packet.state_spans[live_tail_indices[0]])

    asset_base_mask = sum(1 << index for index in asset_indices)
    live_operand_mask = (
        (1 << LIVE_OPERAND_ASSET_BASES)
        | (1 << LIVE_OPERAND_BINDING_COMPOSED)
        | (1 << LIVE_OPERAND_CONFIG)
    )

    source_words: list[int] = []
    _append_records(
        source_words,
        0x4D335341,
        tuple(packet.assets[index] for index in sorted(asset_indices)),
    )
    _append_records(source_words, 0x4D335342, (segment,))
    _append_records(source_words, 0x4D335343, bindings)
    owner = OWNER_SPECS[GENERATED_SEGMENT_INDEX]
    source_checksum = fnv1a_u32(
        source_words,
        fnv1a_bytes(
            b"\0".join(
                value.encode("ascii")
                for value in (
                    owner.name,
                    owner.resource_name,
                    owner.callback,
                )
            )
        ),
    )

    table_words: list[int] = []
    _append_records(table_words, 0x4D335351, (segment,))
    _append_records(
        table_words,
        0x4D335352,
        tuple(
            packet.dobjs[index]
            for index in range(
                segment.first_dobj, segment.first_dobj + segment.dobj_count
            )
        ),
    )
    _append_records(table_words, 0x4D335353, bindings)
    _append_records(table_words, 0x4D335354, runs)
    _append_values(table_words, 0x4D335355, corners)
    _append_records(
        table_words,
        0x4D335356,
        tuple(packet.vertices[index] for index in dense_indices),
    )
    _append_records(table_words, 0x4D335357, epochs)
    _append_records(
        table_words,
        0x4D335358,
        tuple(packet.policies[index] for index in policy_indices),
    )
    _append_records(table_words, 0x4D335359, run_spans)
    _append_records(table_words, 0x4D33535A, tail_spans)
    _append_values(table_words, 0x4D33535B, state_sequence)
    _append_records(
        table_words,
        0x4D33535C,
        tuple(packet.state_deltas[index] for index in state_delta_indices),
    )
    table_checksum = fnv1a_u32(table_words)

    hot_words: list[int] = []
    _append_records(hot_words, 0x4D335348, hot_rows)
    hot_checksum = fnv1a_u32(hot_words)
    prepared_dense_words: list[int] = []
    _append_values(
        prepared_dense_words, 0x4D33535D, prepared_dense_offsets
    )
    _append_values(
        prepared_dense_words, 0x4D33535E, prepared_dense_indices
    )
    prepared_dense_checksum = fnv1a_u32(prepared_dense_words)
    certificate = GeneratedStageCertificate(
        source_checksum=source_checksum,
        table_checksum=table_checksum,
        hot_checksum=hot_checksum,
        prepared_dense_checksum=prepared_dense_checksum,
        first_state=state_positions[0],
        state_count=len(state_positions),
        sync_count=sum(span.sync_count for span in execution_spans),
        segment_index=GENERATED_SEGMENT_INDEX,
        first_dobj=segment.first_dobj,
        dobj_count=segment.dobj_count,
        owner=segment.owner,
        link=segment.link,
        submit_class=SUBMIT_PROJECTED_NO_Z,
        first_binding=segment.first_binding,
        binding_count=segment.binding_count,
        first_run=segment.first_run,
        run_count=segment.run_count,
        first_texture_epoch=min(run.texture_epoch for run in runs),
        triangle_count=sum(run.triangle_count for run in runs),
        texture_epoch_count=len(epochs),
        live_operand_mask=live_operand_mask,
        asset_base_mask=asset_base_mask,
        material_count=len(material_indices),
        final_tail_span=live_tail_indices[0],
        prepared_dense_count=len(prepared_dense_indices),
        prepared_dense_offset_count=len(prepared_dense_offsets),
    )
    compact_u8 = astuple(certificate)[4:]
    if any(
        value < 0 or value > 0xFF for value in compact_u8
    ):
        raise falsify("generated segment-0 certificate exceeds compact ABI")
    if any(
        row.run_index > 0xFF or row.binding_composed_index > 0xFF
        for row in hot_rows
    ):
        raise falsify("generated segment-0 hot row exceeds compact ABI")
    program = GeneratedStageProgram(certificate, hot_rows, tuple(instructions))
    if program.footprint_bytes() != 92:
        raise falsify("generated segment-0 program footprint changed")
    return program


def c_u8(value: int) -> str:
    return f"{value}u"


def round_shift_signed(value: int, shift: int) -> int:
    if shift == 0:
        return value
    bias = 1 << (shift - 1)
    return -(((-value) + bias) >> shift) if value < 0 else (value + bias) >> shift


def stage_vertex_coordinate_shift(vertex: DenseVertex) -> int:
    for shift in range(6):
        if all(
            -2048 <= round_shift_signed(value, shift) <= 2047
            for value in (vertex.x, vertex.y, vertex.z)
        ):
            return shift
    return 6


def c_u16(value: int) -> str:
    return f"0x{value:04x}u"


def c_u32(value: int) -> str:
    return f"0x{value:08x}u"


def render_generated_segment0_program(program: GeneratedStageProgram) -> list[str]:
    lines = [
        "#define NDS_NATIVE_STAGE_SEGMENT0_GENERATED_PROGRAM \\",
        "    do { \\",
    ]
    for opcode, operands in program.instructions:
        if opcode in ("SYNC", "RUN"):
            rendered = ", ".join(c_u8(value) for value in operands)
        elif opcode == "IMAGE":
            rendered = ", ".join(
                (
                    c_u8(operands[0]),
                    c_u32(operands[1]),
                    c_u32(operands[2]),
                )
            )
        else:
            rendered = ", ".join(c_u32(value) for value in operands)
        lines.append(f"        NDS_TASK26_{opcode}({rendered}); \\")
    lines.append("    } while (0)")
    return lines


def render_rows(type_name: str, array_name: str, rows: Sequence[str]) -> list[str]:
    lines = [f"static const {type_name} {array_name}[{len(rows)}] = {{"]
    lines.extend(f"    {row}," for row in rows)
    lines.extend(("};", ""))
    return lines


def render_include(packet: Packet) -> bytes:
    program = build_generated_segment0_program(packet)
    certificate = program.certificate
    hot_bytes = len(program.runs) * GENERATED_SEGMENT_HOT_ROW_BYTES
    lines = [
        "/* Generated by scripts/generate_nds_native_stage.py.  Do not edit. */",
        "/* Production-linkable packet; runtime selection remains external. */",
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
        f"#define NDS_NATIVE_STAGE_STATE_DELTA_COUNT {len(packet.state_deltas)}u",
        f"#define NDS_NATIVE_STAGE_STATE_SEQUENCE_COUNT {len(packet.state_sequence)}u",
        f"#define NDS_NATIVE_STAGE_STATE_SPAN_COUNT {len(packet.state_spans)}u",
        f"#define NDS_NATIVE_STAGE_SLAB_BYTES {packet.slab_bytes()}u",
        f"#define NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT {len(program.runs)}u",
        f"#define NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_COLD_BYTES "
        f"{GENERATED_SEGMENT_COLD_BYTES}u",
        f"#define NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_HOT_BYTES {hot_bytes}u",
        f"#define NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_BYTES "
        f"{program.footprint_bytes()}u",
        f"#define NDS_NATIVE_STAGE_TOTAL_CONST_MAX_BYTES "
        f"{packet.slab_bytes() + program.footprint_bytes()}u",
        f"#define NDS_NATIVE_STAGE_PRODUCTION_PACKET_ABI {c_u32(PRODUCTION_PACKET_ABI)}",
        "#define NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX (1u << 0)",
        "#define NDS_NATIVE_STAGE_COORDINATE_SHIFT 5u",
        f"#define NDS_NATIVE_STAGE_LIVE_OPERAND_ASSET_BASES "
        f"{LIVE_OPERAND_ASSET_BASES}u",
        f"#define NDS_NATIVE_STAGE_LIVE_OPERAND_BINDING_COMPOSED "
        f"{LIVE_OPERAND_BINDING_COMPOSED}u",
        f"#define NDS_NATIVE_STAGE_LIVE_OPERAND_MATERIALS "
        f"{LIVE_OPERAND_MATERIALS}u",
        f"#define NDS_NATIVE_STAGE_LIVE_OPERAND_CONFIG {LIVE_OPERAND_CONFIG}u",
        f"#define NDS_NATIVE_STAGE_LIVE_OPERAND_COUNT {LIVE_OPERAND_COUNT}u",
        f"#define NDS_NATIVE_STAGE_SEGMENT0_SOURCE_CHECKSUM "
        f"{c_u32(certificate.source_checksum)}",
        f"#define NDS_NATIVE_STAGE_SEGMENT0_TABLE_CHECKSUM "
        f"{c_u32(certificate.table_checksum)}",
        f"#define NDS_NATIVE_STAGE_SEGMENT0_HOT_CHECKSUM "
        f"{c_u32(certificate.hot_checksum)}",
        f"#define NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_CHECKSUM "
        f"{c_u32(certificate.prepared_dense_checksum)}",
        f"#define NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_COUNT "
        f"{certificate.prepared_dense_count}u",
        f"#define NDS_NATIVE_STAGE_SEGMENT0_PREPARED_DENSE_OFFSET_COUNT "
        f"{certificate.prepared_dense_offset_count}u",
        "#define NDS_NATIVE_STAGE_SEGMENT0_TEXTURE_EPOCH_MASK "
        "0x00000000003fffffULL",
        "#ifndef NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE",
        "#define NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE 0",
        "#endif",
        "#if (NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE != 0) && \\",
        "    (NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE != 1)",
        '#error "NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE must be 0 or 1"',
        "#endif",
        "#define NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_LINKED_BYTES \\",
        "    (NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE * \\",
        "     NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_BYTES)",
        "#define NDS_NATIVE_STAGE_TOTAL_CONST_BYTES \\",
        "    (NDS_NATIVE_STAGE_SLAB_BYTES + \\",
        "     NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_LINKED_BYTES)",
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
        "    u8 packed_cache_shift;",
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
        "typedef struct NDSNativeStageStateDelta {",
        "    u32 w0;",
        "    u32 w1;",
        "    u8 effect;",
        "    u8 asset_index;",
        "    u8 material_event;",
        "    u8 material_command;",
        "} NDSNativeStageStateDelta;",
        "",
        "typedef struct NDSNativeStageStateSpan {",
        "    u16 first_state;",
        "    u8 state_count;",
        "    u8 sync_count;",
        "} NDSNativeStageStateSpan;",
        "",
        "typedef struct NDSNativeStageGeneratedRun {",
        "    u8 run_index;",
        "    u8 binding_composed_index;",
        "} NDSNativeStageGeneratedRun;",
        "",
        "typedef struct NDSNativeStageGeneratedCertificate {",
        "    u32 source_checksum;",
        "    u32 table_checksum;",
        "    u32 hot_checksum;",
        "    u32 prepared_dense_checksum;",
        "    u8 first_state;",
        "    u8 state_count;",
        "    u8 sync_count;",
        "    u8 segment_index;",
        "    u8 first_dobj;",
        "    u8 dobj_count;",
        "    u8 owner;",
        "    u8 link;",
        "    u8 submit_class;",
        "    u8 first_binding;",
        "    u8 binding_count;",
        "    u8 first_run;",
        "    u8 run_count;",
        "    u8 first_texture_epoch;",
        "    u8 triangle_count;",
        "    u8 texture_epoch_count;",
        "    u8 live_operand_mask;",
        "    u8 asset_base_mask;",
        "    u8 material_count;",
        "    u8 final_tail_span;",
        "    u8 prepared_dense_count;",
        "    u8 prepared_dense_offset_count;",
        "} NDSNativeStageGeneratedCertificate;",
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
        '_Static_assert(sizeof(NDSNativeStageStateDelta) == 12u, "stage state delta ABI");',
        '_Static_assert(sizeof(NDSNativeStageStateSpan) == 4u, "stage state span ABI");',
        '_Static_assert(sizeof(NDSNativeStageGeneratedRun) == 2u, "generated stage run ABI");',
        '_Static_assert(sizeof(NDSNativeStageGeneratedCertificate) == 40u, "generated stage certificate ABI");',
        '_Static_assert(NDS_NATIVE_STAGE_SEGMENT0_PROGRAM_RUN_COUNT == 26u, "generated segment-0 run order");',
        '_Static_assert(NDS_NATIVE_STAGE_SLAB_BYTES <= 16384u, "stage slab exceeds 16 KiB");',
        '_Static_assert(NDS_NATIVE_STAGE_TOTAL_CONST_MAX_BYTES <= 16384u, "generated stage data exceeds 16 KiB");',
        "",
        "#if NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE",
    ]

    lines.extend(
        (
            *render_generated_segment0_program(program),
            "",
            "static const NDSNativeStageGeneratedCertificate",
            "    sNdsNativeStageSegment0ColdCertificate = {",
            f"        {c_u32(certificate.source_checksum)},",
            f"        {c_u32(certificate.table_checksum)},",
            f"        {c_u32(certificate.hot_checksum)},",
            f"        {c_u32(certificate.prepared_dense_checksum)},",
            "        "
            + ", ".join(
                c_u8(value) for value in astuple(certificate)[4:]
            ),
            "    };",
            "",
        )
    )
    lines.extend(
        render_rows(
            "NDSNativeStageGeneratedRun",
            "sNdsNativeStageSegment0HotRuns",
            [
                "{ "
                + ", ".join(
                    (c_u8(row.run_index), c_u8(row.binding_composed_index))
                )
                + " }"
                for row in program.runs
            ],
        )
    )
    lines.extend(("#endif", ""))

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
                        c_u8(
                            row.cache_slot
                            | (stage_vertex_coordinate_shift(row) << 5)
                        ),
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
    lines.extend(
        render_rows(
            "NDSNativeStageStateDelta",
            "sNdsNativeStageStateDeltas",
            [
                "{ "
                + ", ".join(
                    (
                        c_u32(row.w0),
                        c_u32(row.w1),
                        c_u8(row.effect),
                        c_u8(row.asset_index),
                        c_u8(row.material_event),
                        c_u8(row.material_command),
                    )
                )
                + " }"
                for row in packet.state_deltas
            ],
        )
    )
    lines.extend(
        render_rows(
            "u8",
            "sNdsNativeStageStateSequence",
            [c_u8(index) for index in packet.state_sequence],
        )
    )
    lines.extend(
        render_rows(
            "NDSNativeStageStateSpan",
            "sNdsNativeStageStateSpans",
            [
                "{ "
                + ", ".join(
                    (
                        c_u16(row.first_state),
                        c_u8(row.state_count),
                        c_u8(row.sync_count),
                    )
                )
                + " }"
                for row in packet.state_spans
            ],
        )
    )
    lines.extend(
        (
            "const u32 gNdsNativeStageProductionPacketABI =",
            "    NDS_NATIVE_STAGE_PRODUCTION_PACKET_ABI;",
            "",
        )
    )
    return ("\n".join(lines) + "\n").encode("ascii")


def strip_c_non_code(source: str) -> str:
    """Blank comments and literals while preserving source offsets."""

    result = list(source)
    index = 0
    state = "code"
    while index < len(source):
        char = source[index]
        following = source[index + 1] if index + 1 < len(source) else ""
        if state == "code":
            if char == "/" and following == "/":
                result[index] = result[index + 1] = " "
                index += 2
                state = "line"
                continue
            if char == "/" and following == "*":
                result[index] = result[index + 1] = " "
                index += 2
                state = "block"
                continue
            if char in ('"', "'"):
                result[index] = " "
                index += 1
                state = "string" if char == '"' else "character"
                continue
        elif state == "line":
            if char == "\n":
                state = "code"
            else:
                result[index] = " "
            index += 1
            continue
        elif state == "block":
            result[index] = " "
            if char == "*" and following == "/":
                result[index + 1] = " "
                index += 2
                state = "code"
                continue
            index += 1
            continue
        else:
            result[index] = " "
            if char == "\\" and index + 1 < len(source):
                result[index + 1] = " "
                index += 2
                continue
            if ((state == "string" and char == '"') or
                    (state == "character" and char == "'")):
                state = "code"
            index += 1
            continue
        index += 1
    if state in ("block", "string", "character"):
        raise falsify(f"unterminated C {state}")
    return "".join(result)


def named_c_closure(source: str, name: str) -> str:
    """Return one top-level named C function with brace-balanced bounds."""

    code = strip_c_non_code(source)
    pattern = re.compile(
        rf"(?m)^[A-Za-z_][^\n;{{}}]*\b{re.escape(name)}\s*\("
    )
    for match in pattern.finditer(code):
        open_brace = code.find("{", match.end())
        semicolon = code.find(";", match.end())
        if open_brace < 0 or (semicolon >= 0 and semicolon < open_brace):
            continue
        depth = 0
        for index in range(open_brace, len(code)):
            if code[index] == "{":
                depth += 1
            elif code[index] == "}":
                depth -= 1
                if depth == 0:
                    return code[open_brace:index + 1]
        raise falsify(f"{name}: unbalanced function braces")
    raise falsify(f"named source closure is absent: {name}")


def observed_arrow_fields(closure: str, tracked_bases: Sequence[str]) -> set[str]:
    tracked = set(tracked_bases)
    observed_bases = set()
    fields = set()
    for match in re.finditer(
        r"\b([A-Za-z_]\w*)\s*->\s*"
        r"([A-Za-z_]\w*(?:\s*\.\s*[A-Za-z_]\w*)*)",
        closure,
    ):
        observed_bases.add(match.group(1))
        fields.add(
            f"{match.group(1)}."
            + re.sub(r"\s+", "", match.group(2))
        )
    missing_bases = sorted(tracked - observed_bases)
    if missing_bases:
        raise falsify(f"tracked pointer bases are no longer read {missing_bases}")
    return fields


def observed_static_fields(
    closure: str, tracked_bases: Sequence[str]
) -> set[str]:
    fields = set()
    missing_bases = []
    for base in tracked_bases:
        matches = re.findall(
            rf"\b{re.escape(base)}\s*(?:\[[^\]]+\]\s*)?\.\s*"
            r"([A-Za-z_]\w*)",
            closure,
        )
        if not matches:
            missing_bases.append(base)
        fields.update(f"{base}.{field}" for field in matches)
    if missing_bases:
        raise falsify(
            f"tracked static bases are no longer read {sorted(missing_bases)}"
        )
    return fields


def validate_observed_field_policy(
    policy: dict[str, str], observed: set[str], context: str
) -> None:
    invalid = sorted(set(policy.values()) - set(FIELD_CLASSES))
    if invalid:
        raise falsify(f"{context}: invalid field classifications {invalid}")
    unclassified = sorted(observed - set(policy))
    if unclassified:
        raise falsify(f"{context}: unclassified reads {unclassified}")
    missing = sorted(set(policy) - observed)
    if missing:
        raise falsify(f"{context}: classified fields are no longer read {missing}")


def build_consumed_closure_rows(
    repo_root: Path,
    specs: Sequence[dict[str, object]],
    source_cache: dict[str, str],
) -> list[dict[str, object]]:
    closures = []
    for spec in specs:
        path = str(spec["path"])
        if path not in source_cache:
            source_cache[path] = (repo_root / path).read_text(encoding="utf-8")
        name = str(spec["closure"])
        policy = dict(spec["fields"])
        observed = observed_arrow_fields(
            named_c_closure(source_cache[path], name),
            tuple(spec["tracked_bases"]),
        )
        validate_observed_field_policy(policy, observed, f"{path}:{name}")
        static_policy = dict(spec.get("static_fields", {}))
        observed_static = observed_static_fields(
            named_c_closure(source_cache[path], name),
            tuple(spec.get("tracked_static_bases", ())),
        )
        validate_observed_field_policy(
            static_policy, observed_static, f"{path}:{name}:static"
        )
        overlap = set(policy) & set(static_policy)
        if overlap:
            raise falsify(f"{path}:{name}: duplicate field policy {sorted(overlap)}")
        policy.update(static_policy)
        observed.update(observed_static)
        closures.append(
            {
                "path": path,
                "closure": name,
                "fields": [
                    {"access": field, "classification": policy[field]}
                    for field in sorted(observed)
                ],
            }
        )
    return closures


def build_consumed_fields_manifest(repo_root: Path) -> dict[str, object]:
    source_cache: dict[str, str] = {}
    closures = build_consumed_closure_rows(
        repo_root, SOURCE_CLOSURE_POLICIES, source_cache
    )
    task26_closures = build_consumed_closure_rows(
        repo_root, TASK26_GENERATED_CLOSURE_POLICIES, source_cache
    )
    segment0 = build_generated_segment0_program(generate(repo_root)).certificate

    return {
        "schema": "smash64ds.m3-consumed-fields.v1",
        "generated_by": "scripts/generate_nds_native_stage.py",
        "allowed_classifications": list(FIELD_CLASSES),
        "source_closures": closures,
        "task26_generated_closures": task26_closures,
        "generated_runtime_fields": [
            {
                "record": record,
                "classification": FIELD_CLASS_IMMUTABLE,
                "fields": list(fields),
            }
            for record, fields in GENERATED_RUNTIME_FIELDS.items()
        ],
        "root_frame_fields": [
            {
                "fields": [
                    "asset_bases", "dobjs", "binding_display_lists",
                    "topology_generation", "topology_stamp",
                ],
                "classification": FIELD_CLASS_IMMUTABLE,
            },
            {
                "fields": ["projection", "binding_composed"],
                "classification": FIELD_CLASS_CAMERA,
            },
            {
                "fields": ["materials", "config"],
                "classification": FIELD_CLASS_LIVE,
            },
        ],
        "transitive_inputs": [
            {
                "name": "camera_and_dobj_matrix_operands",
                "closures": [
                    "ndsRendererAdapterBuildCameraMatrices",
                    "ndsRendererAdapterBuildDObjXObjMatrix",
                    "ndsRendererAdapterBuildDObjLocalMatrix",
                    "ndsRendererAdapterApplyMvpRecalcRpy0x47",
                    "ndsRendererAdapterPrepareNativeStageMatrices",
                ],
                "disposition": (
                    "recompute from live CObj and DObj operands every frame; "
                    "never cache composed or near-plane results"
                ),
            },
            {
                "name": "material_color_alpha_texture_and_resolver_state",
                "closures": [
                    "ndsRendererAdapterMaterialFlags",
                    "ndsRendererAdapterMaterialLoadBlock",
                    "ndsRendererAdapterMaterialTextureState",
                    "ndsRendererAdapterBuildNativeMaterialSnapshot",
                    "ndsRendererNativeApplyMaterial",
                    "ndsRendererNativeStageApplyStateSpan",
                    "ndsRendererNativeStagePolicyMatches",
                    "ndsRendererNativeStagePrepareRun",
                    "ndsRendererHardwareResolveOrBindTexture",
                    "ndsRendererHardwareColorSource",
                    "ndsRendererHardwareAlphaUsesVertex",
                    "ndsRendererHardwareUseMaterialColor",
                    "ndsRendererHardwareUseVertexColor",
                    "ndsRendererHardwareAlpha",
                    "ndsRendererHardwarePolyFmt",
                    "ndsRendererHardwareUseTexture",
                    "ndsRendererHardwareTextureImplicitStateOn",
                    "ndsRendererActiveTextureTile",
                    "ndsRendererHardwareTextureFilterOffset",
                    "ndsRendererStageTextureSiteRemember",
                    "ndsRendererInitTraversalState",
                    "ndsRendererResolveDataPointer",
                ],
                "disposition": (
                    "retain the current exact state, texture resolver, color, "
                    "alpha, and configuration helpers in Task 26"
                ),
            },
            {
                "name": "visibility_phase_and_callback_restoration",
                "closures": [
                    "ndsRendererAdapterBuildNativeStageTopologyStamp",
                    "ndsRendererPrepareNativeStageOwner",
                    "ndsRendererCommitNativeStageSegment",
                    "ndsRendererAdapterCommitNativeStageMaterials",
                    "ndsRendererAdapterCommitNativeStageDisplay",
                    "ndsRendererAdapterFinishNativeStageOwner",
                ],
                "disposition": (
                    "validate before GX and preserve exact segment and "
                    "texture-ID restoration order"
                ),
            },
        ],
        "segment_order": [
            "layer0", "whispy_eyes", "whispy_mouth", "flowers_back",
            "layer1", "layer2", "flowers_front", "layer3",
        ],
        "material_snapshots": [
            {"slot": 0, "binding": 20, "required_flags": 1},
            {"slot": 1, "binding": 22, "required_flags": 1},
            {"slot": 2, "binding": 31, "required_flags": 107},
            {"slot": 3, "binding": 32, "required_flags": 107},
        ],
        "task26_live_operand_order": [
            {
                "index": 0,
                "operand": "asset_bases[4]",
                "classification": FIELD_CLASS_IMMUTABLE,
                "disposition": "generation_validated_address_binding",
            },
            {
                "index": 1,
                "operand": "binding_composed[42]",
                "classification": FIELD_CLASS_CAMERA,
                "disposition": "recompute_and_bind_each_frame",
            },
            {
                "index": 2,
                "operand": "materials[4]",
                "classification": FIELD_CLASS_LIVE,
                "disposition": "rebuild_and_bind_each_frame",
            },
            {
                "index": 3,
                "operand": "config",
                "classification": FIELD_CLASS_LIVE,
                "disposition": "bind_current_exact_callbacks_and_limits",
            },
        ],
        "task26_segment0_program": {
            "admission": (
                "execute only after the existing generation/stamp topology "
                "admission has populated sNdsNativeStageValidationCache"
            ),
            "segment": 0,
            "dobjs": {
                "first": segment0.first_dobj,
                "count": segment0.dobj_count,
            },
            "bindings": {
                "first": segment0.first_binding,
                "count": segment0.binding_count,
            },
            "runs": {
                "first": segment0.first_run,
                "count": segment0.run_count,
            },
            "triangle_count": segment0.triangle_count,
            "texture_epochs": {
                "first": segment0.first_texture_epoch,
                "count": segment0.texture_epoch_count,
                "mask": "0x00000000003fffff",
            },
            "material_count": segment0.material_count,
            "final_tail_span": segment0.final_tail_span,
            "final_tail_state_sync": [4, 2],
            "prepared_dense_cache": {
                "source": "sNdsNativeStageValidationCache",
                "offset_indices": [
                    0, segment0.prepared_dense_offset_count - 1,
                ],
                "offset_count": segment0.prepared_dense_offset_count,
                "terminal": segment0.prepared_dense_count,
                "first_visit_count": segment0.prepared_dense_count,
                "order_checksum": f"0x{segment0.prepared_dense_checksum:08x}",
                "disposition": (
                    "reuse the Task-14 prepared_dense_offsets/indices; do not "
                    "emit a second schedule"
                ),
            },
            "retained_live_seams": [
                "ndsRendererHardwareResolveOrBindTexture and mutable texture residency",
                "per-frame composed-matrix transforms and near-plane classification",
                "one shared traversal/stats/epoch-mask transaction across all eight segments",
            ],
        },
        "admission_only": [
            "dobjs[57]", "binding_display_lists[42]", "projection",
            "topology_generation", "topology_stamp",
        ],
        "callback_commit_order": [
            {
                "segment": 1,
                "slots": [0],
                "outputs": ["MObj.texture_id_curr", "MObj.texture_id_next"],
            },
            {
                "segment": 2,
                "slots": [1],
                "outputs": ["MObj.texture_id_curr", "MObj.texture_id_next"],
            },
            {
                "segment": 5,
                "slots": [2, 3],
                "outputs": ["MObj.texture_id_curr", "MObj.texture_id_next"],
            },
        ],
        "invalidation_manifest": [
            "asset owner generation, loaded-file identity, data pointer, asset ID, size, or owner generation changes",
            "segment identity, link, callback, display-link order, or visibility flags change",
            "DObj parent/child/sibling, display list, MObj, flags, XObj count/kind, owner, depth, or binding changes",
            "binding display-list identity changes",
            "camera projection or any of 42 composed matrices changes: recompute every frame; never cache near-plane results",
            "any live MObj material, texture selector, color, alpha, scale, scroll, palette, or image input changes: rebuild all four snapshots",
            "callback segment/order mismatch: fail closed; never restore texture IDs at a different seam",
        ],
        "census_dimensions": {
            "segments": 8,
            "material_snapshots": 4,
            "windows": [
                "Countdown", "early",
                "Whispy Wait-to-Open/material-animation boundary",
                "Whispy steady/post-change", "late", "KO", "rebirth",
                "Time Up/Results",
            ],
        },
        "frame_lifetime": (
            "The adapter workspace owns generation-stable topology and asset "
            "pointers, rebuilds matrices and material snapshots before each "
            "synchronous preflight, remains resident through all eight "
            "interleaved commits, and restores captured texture IDs only at "
            "segments 1, 2, and 5."
        ),
        "task26_overlap": {
            "consume": (
                "immutable generated control fields plus the ordered live "
                "operand block above"
            ),
            "retain_current": (
                "generation/stamp admission, all per-frame matrices and "
                "near-plane computation, exact material rebuild, callback "
                "restoration, and pre-GX fail-closed validation"
            ),
            "forbid": (
                "a second topology cache, cached camera/near results, a "
                "generic runtime scanner, or post-GX fallback"
            ),
        },
    }


def render_consumed_fields_manifest(repo_root: Path) -> bytes:
    return (
        json.dumps(
            build_consumed_fields_manifest(repo_root),
            indent=2,
            sort_keys=True,
        )
        + "\n"
    ).encode("utf-8")


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
        "--manifest-output",
        type=Path,
        default=None,
        help=(
            "generated Task 23R consumed-field certificate; default "
            f"{DEFAULT_CONSUMED_FIELDS_OUTPUT.as_posix()}"
        ),
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
    manifest_output = args.manifest_output or (
        repo_root / DEFAULT_CONSUMED_FIELDS_OUTPUT
    )
    if not output.is_absolute():
        output = repo_root / output
    if not manifest_output.is_absolute():
        manifest_output = repo_root / manifest_output
    try:
        packet = generate(repo_root)
        segment0_program = build_generated_segment0_program(packet)
        rendered = render_include(packet)
        rendered_manifest = render_consumed_fields_manifest(repo_root)
        manifest = json.loads(rendered_manifest)
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
            if not manifest_output.is_file():
                raise falsify(
                    f"consumed-field manifest is absent: {manifest_output}"
                )
            if manifest_output.read_bytes() != rendered_manifest:
                raise falsify(
                    f"consumed-field manifest is stale: {manifest_output}"
                )
        else:
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_bytes(rendered)
            manifest_output.parent.mkdir(parents=True, exist_ok=True)
            manifest_output.write_bytes(rendered_manifest)
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
        f"policies={len(packet.policies)} state_deltas={len(packet.state_deltas)} "
        f"state_events={len(packet.state_sequence)} "
        f"state_spans={len(packet.state_spans)} slab_bytes={packet.slab_bytes()} "
        f"segment0_program_runs={len(segment0_program.runs)} "
        f"segment0_program_bytes={segment0_program.footprint_bytes()} "
        f"total_const_max_bytes={packet.slab_bytes() + segment0_program.footprint_bytes()} "
        f"manifest_fields={sum(len(row['fields']) for row in manifest['source_closures'])} "
        f"task26_manifest_fields={sum(len(row['fields']) for row in manifest['task26_generated_closures'])} "
        f"sha256={rendered_hash}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
