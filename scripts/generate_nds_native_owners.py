#!/usr/bin/env python3
"""Generate the canonical Mario/Fox native-owner IR from exact O2R inputs.

The generator walks the source display lists, preserves their compact root
light prefixes, and restores intra-root G_MW_LIGHTCOL changes in source order.
The old runtime executor remains excluded because it re-entered generic
state/triangle machinery and failed the big-jump performance gate.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

import generate_nds_native_stage as stage_manifest


DEFAULT_CONSUMED_FIELDS_OUTPUT = Path(
    "docs/optimization/NDS_NATIVE_FIGHTER_CONSUMED_FIELDS.generated.json"
)

FIELD_CLASS_IMMUTABLE = stage_manifest.FIELD_CLASS_IMMUTABLE
FIELD_CLASS_CAMERA = stage_manifest.FIELD_CLASS_CAMERA
FIELD_CLASS_LIVE = stage_manifest.FIELD_CLASS_LIVE
FIELD_CLASS_CALLBACK = stage_manifest.FIELD_CLASS_CALLBACK
FIELD_CLASSES = stage_manifest.FIELD_CLASSES


def _classified(classification: str, fields: str) -> dict[str, str]:
    return {field: classification for field in fields.split()}


SOURCE_CLOSURE_POLICIES = (
    {
        "path": "src/port/reloc_backend_renderer_dl.c",
        "closure": "ndsRendererAdapterPrepareNativeOwnerHierarchy",
        "tracked_bases": ("fp", "joint", "m2_owner", "workspace", "xobj"),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                joint.parent joint.xobjs joint.xobjs_num workspace.hierarchy.joint_bindings
                workspace.hierarchy.joint_count workspace.hierarchy.joint_parents
                workspace.hierarchy_bindings workspace.hierarchy_joints
                workspace.hierarchy_parents xobj.kind
                """,
            ),
            **_classified(
                FIELD_CLASS_CAMERA,
                """
                workspace.hierarchy.camera_modelview workspace.hierarchy.joint_locals
                workspace.hierarchy.projection workspace.hierarchy_camera_modelview
                workspace.hierarchy_locals workspace.hierarchy_projection
                """,
            ),
            **_classified(
                FIELD_CLASS_LIVE,
                "fp.is_use_animlocks fp.shuffle_tics",
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                m2_owner.m2_camera_fetch_count m2_owner.m2_camera_fetch_ticks
                m2_owner.m2_local_matrix_build_count m2_owner.m2_local_matrix_ticks
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativePreflightFighterHierarchy",
        "tracked_bases": (
            "epoch", "execution", "hierarchy", "input", "m2_owner",
            "prepared_epoch", "root", "scratch", "state", "stats", "tables",
        ),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                epoch.action_count epoch.after_state_count epoch.after_state_first
                epoch.after_sync_count epoch.before_state_count epoch.before_state_first
                epoch.before_sync_count epoch.first_run epoch.material_slot epoch.run_count
                hierarchy.joint_bindings hierarchy.joint_count hierarchy.joint_parents
                hierarchy.root_count input.root_offset root.epoch_count root.first_epoch
                root.root_offset root.tail_state_count root.tail_state_first root.tail_sync_count
                tables.binding_joints tables.joint_count tables.root_count tables.roots
                tables.schedule
                """,
            ),
            **_classified(
                FIELD_CLASS_CAMERA,
                """
                execution.hierarchy_world hierarchy.camera_modelview hierarchy.joint_locals
                hierarchy.projection prepared_epoch.light_direction
                state.matrix state.modelview state.prepared_light_direction
                """,
            ),
            **_classified(
                FIELD_CLASS_LIVE,
                """
                execution.hierarchy_epochs execution.hierarchy_runs execution.preflight_stats
                execution.traversal hierarchy.config hierarchy.roots input.composed_matrix
                input.config input.material_count input.materials input.modelview_matrix
                input.preamble input.preamble.flags prepared_epoch.light_direction_valid
                scratch.blocker scratch.geometry_mode scratch.light_color_1
                scratch.light_color_2 scratch.light_color_mask scratch.light_dir_mask
                state.current_transform_vertex_mask state.input_vertex_valid_mask
                state.matrix_generation state.matrix_valid state.modelview_valid
                state.prepared_light_direction_valid state.prepared_texcoord_valid_mask
                state.prepared_vertex_color_valid_mask state.texture_prepare_valid
                state.vertex_color_valid_mask state.vertex_valid_mask stats.blocker
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                "m2_owner.m2_lighting_shading_ticks m2_owner.m2_run_prepare_ticks",
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativePrepareHierarchyTexcoords",
        "tracked_bases": ("dense", "prepared", "prepared_run"),
        "fields": {
            **_classified(FIELD_CLASS_IMMUTABLE, "dense.s dense.t"),
            **_classified(
                FIELD_CLASS_LIVE,
                """
                prepared.s prepared.t prepared_run.origin_s prepared_run.origin_t
                prepared_run.scale_s prepared_run.scale_t prepared_run.texture_offset
                prepared_run.textured
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativeBeginHierarchyBatch",
        "tracked_bases": ("entry", "prepared_run", "stats"),
        "fields": {
            **_classified(
                FIELD_CLASS_LIVE,
                """
                entry.last_used_frame entry.params entry.pinned prepared_run.poly_fmt
                prepared_run.texture_entry prepared_run.texture_format
                prepared_run.texture_height prepared_run.texture_name
                prepared_run.texture_params prepared_run.texture_width prepared_run.textured
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                stats.hardware_texture_format stats.hardware_texture_height
                stats.hardware_texture_ready_count stats.hardware_texture_width
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererNativeCommitHierarchyRoot",
        "tracked_bases": (
            "epoch", "execution", "input", "m2_owner", "prepared_epoch",
            "prepared_run", "root", "run", "state", "stats",
        ),
        "fields": {
            **_classified(
                FIELD_CLASS_IMMUTABLE,
                """
                epoch.after_state_count epoch.after_state_first epoch.after_sync_count
                epoch.before_state_count epoch.before_state_first epoch.before_sync_count
                epoch.first_run epoch.material_slot epoch.run_count root.epoch_count
                root.first_epoch root.source_command_count root.tail_state_count
                root.tail_state_first root.tail_sync_count run.submit_class run.triangle_count
                """,
            ),
            **_classified(
                FIELD_CLASS_CAMERA,
                """
                execution.hierarchy_world prepared_epoch.light_direction state.matrix
                state.modelview state.prepared_light_direction
                """,
            ),
            **_classified(
                FIELD_CLASS_LIVE,
                """
                execution.hierarchy_epochs execution.hierarchy_runs
                input.materials input.preamble prepared_epoch.light_direction_valid
                prepared_run.textured state.current_transform_vertex_mask
                state.input_vertex_valid_mask state.matrix_generation state.matrix_valid
                state.modelview_valid state.prepared_light_direction_valid
                state.prepared_texcoord_valid_mask state.prepared_vertex_color_valid_mask
                state.texture_prepare_valid state.vertex_color_valid_mask state.vertex_valid_mask
                """,
            ),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                m2_owner.m2_corner_emit_account_ticks
                m2_owner.m2_lighting_shading_ticks m2_owner.m2_run_prepare_ticks
                stats.command_count stats.end_command_count stats.first_opcode
                stats.triangle_count
                """,
            ),
        },
    },
    {
        "path": "src/nds/nds_renderer.c",
        "closure": "ndsRendererExecuteNativeFighterOwnerHierarchy",
        "tracked_bases": ("hierarchy", "m2_owner", "stats"),
        "fields": {
            **_classified(
                FIELD_CLASS_CAMERA,
                "hierarchy.camera_modelview hierarchy.joint_locals hierarchy.projection",
            ),
            **_classified(FIELD_CLASS_LIVE, "hierarchy.roots"),
            **_classified(
                FIELD_CLASS_CALLBACK,
                """
                m2_owner.m2_corner_emit_account_ticks
                m2_owner.m2_corner_emit_run_count m2_owner.m2_lighting_epoch_count
                m2_owner.m2_lighting_shading_ticks m2_owner.m2_root_gx_count
                m2_owner.m2_root_gx_ticks m2_owner.m2_run_prepare_count
                m2_owner.m2_run_prepare_ticks stats.hardware_matrix_seed_count
                stats.matrix_pop_count stats.matrix_push_count
                """,
            ),
        },
    },
)


SOURCE_EXPORT_HASHES = {
    "state": "b332130988708066b956a2c43160ffe77c07c8281cbb7da380e2ad8492252045",
    "sequence": "3feef24dc68c5a1600c196b4b57cb7402163ab02c439b3c16d758315928b2a2e",
    "vertex": "0582f7e74a4649498a9f60e6518d0aa494416258cc42cb170520dd1db3a3d36b",
    "triangles": "b9e792e1730d8fb1e170eb4b7aad1070eb1bc762f9be31ea02aed66f41901246",
    "runs": "ba135565be9a942bae556f12fbf221e56d6feebb71e172472cfc19805803486c",
    "epochs": "7f0fa1a3dba3660c899e7c3184ef5f62bddec6b38d96ad4cbd9966f7946ce9e5",
    "mario_roots": "bbc51381b9baa20e09b6a14e12e3ef6d9d253b7845cd2b4718b5148d5e38cac2",
    "fox_roots": "2f82f144939c5c952f79b8961593b84e7c2d6484a92bee07b84c9014d395a7c7",
}

O2R_ASSETS = {
    "mario": (
        Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/MarioModel"),
        0x0128,
        "be1c3b6f909b42da2a973e2fe1977cd72c046d2447f0c4afa97fe8cd5429854f",
    ),
    "fox": (
        Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/FoxModel"),
        0x0139,
        "8c49ded8144d153b25101afc6c71f5e455ef43e2ddc5326c0f71a72ae740b5a5",
    ),
}

# These are the primary JointTree DObjDesc arrays in the exact hashed O2R
# resources above.  BattleShip's source declarations are
# dMarioModel_JointTree (file offset 0x2200) and dFoxModel_JointTree (0x2938).
# Mario contains 25 raw descriptors and Fox contains 27, followed by the
# depth-18 sentinel. BattleShip ftmanager.c creates a separate synthetic TopN
# root, then lbCommonSetupFighterPartsDObjs applies the character setup mask:
# Mario selects raw descriptors 0..23 and Fox selects 0..25. Thus both live
# trees retain the raw cardinality after adding TopN and dropping the final
# unselected raw leaf. (The Mario source comment says 28 entries, but the exact
# hashed initializer/O2R payload reaches its sentinel at descriptor 25.)
OWNER_JOINT_TREES = {
    "mario": (0x2200, 26),
    "fox": (0x2938, 28),
}

# dMarioMain_setup_parts / dFoxMain_setup_parts, consumed MSB-first by
# BattleShip lbCommonSetupFighterPartsDObjs.
OWNER_SETUP_PARTS = {
    "mario": (0xffffff00, 0x00000000),
    "fox": (0xffffffc0, 0x00000000),
}

# Slots 0..15 remain reserved for the camera seed and live GX hierarchy stack.
# Only bindings observed in canonical cross-matrix runs receive an owner-local
# physical store slot. Logical binding IDs remain the source/native-owner IDs;
# never replace them with these physical GX slots. A packed corner uses slot 31
# as the logical current-root slot, and restores alternate bindings through the
# explicit mapping below.
OWNER_CROSS_BINDING_SLOTS = {
    "mario": (
        (1, 16), (2, 17), (5, 18), (6, 19),
        (8, 20), (9, 21), (11, 22), (12, 23),
    ),
    "fox": ((16, 16), (17, 17)),
}

OWNER_PLAN_COUNTS = {
    "mario": (25, 14),
    "fox": (27, 18),
}

# camera seeds, hierarchy pushes, hierarchy pops, cross-binding stores, and
# per-corner/current-root restores in the exact flattened owner packet.
OWNER_GX_PLAN_COUNTS = {
    "mario": (1, 5, 5, 8, 70),
    "fox": (1, 6, 6, 2, 14),
}

DIRECT_POLICY_CULL_NONE = 0x80
DIRECT_POLICY_FAMILIES = (
    # combine_w0, combine_w1, vertex-context flags, textured
    (0xfc127e05, 0xff17f3ff, "VERTEX|TEXTURE", 1),
    (0xfc327e05, 0xff17fdff, "MATERIAL|VERTEX", 0),
    (0xfcfffe05, 0xff167dff, "VERTEX", 0),
    (0xfc327e05, 0xff17f7ff, "MATERIAL|VERTEX", 0),
)
DIRECT_POLICY_TEXTURED_EPOCHS = frozenset((0, 4, 5, 19, 24, 30, 31, 47, 48))
DIRECT_POLICY_LIT_ONLY_EPOCHS = frozenset(
    (3, 11, 14, 17, 22, 26, 28, 32, 36, 38, 42, 46)
)
DIRECT_POLICY_ALT_ALPHA_EPOCHS = frozenset((7, 8, 27, 29, 33, 34, 37))
DIRECT_POLICY_CULL_NONE_EPOCHS = frozenset((20, 21))

O2R_RESOURCE_HEADER_SIZE = 0x40
SOURCE_VERTEX_SIZE = 16
VERTEX_CACHE_SIZE = 32
INVALID_DENSE_VERTEX = 0xffff
INVALID_U8 = 0xff
PACKED_DENSE_ID_BITS = 10
PACKED_DENSE_ID_LIMIT = 1 << PACKED_DENSE_ID_BITS
PACKED_GX_SLOT_CURRENT = 31
GX_CAMERA_SEED_SLOT = 0
GX_HIERARCHY_SLOT_LIMIT = 16
JOINT_SCHEDULE_PUSH_BEFORE = 1 << 15
DOBJ_DESC_SIZE = 44
SOURCE_G_MOVEWORD = 0xdb
SOURCE_G_MW_LIGHTCOL = 0x0a
SOURCE_LIGHTCOL_OFFSETS = (0x00, 0x04, 0x18, 0x1c)
SOURCE_G_DL = 0xde
SOURCE_SEGMENT_E = 0x0e
NATIVE_STATE_LIGHT_COLOR = 14

# Nintendo DS packed geometry FIFO command IDs.  These are REG2ID(register)
# values from libnds videoGL.h/video.h.  Keep the generator independent of a
# host libnds install, then decode every generated payload in the companion
# checker before it can reach the ARM build.
FIFO_NOP = 0x00
FIFO_MTX_MODE = 0x10
FIFO_MTX_STORE = 0x13
FIFO_MTX_RESTORE = 0x14
FIFO_MTX_LOAD_4X4 = 0x16
FIFO_MTX_LOAD_4X3 = 0x17
FIFO_COLOR = 0x20
FIFO_TEX_COORD = 0x22
FIFO_VERTEX16 = 0x23
FIFO_POLY_FORMAT = 0x29
FIFO_TEX_FORMAT = 0x2a
FIFO_PAL_FORMAT = 0x2b
FIFO_BEGIN = 0x40

FIFO_PARAMETER_COUNTS = {
    FIFO_NOP: 0,
    FIFO_MTX_MODE: 1,
    FIFO_MTX_STORE: 1,
    FIFO_MTX_RESTORE: 1,
    FIFO_MTX_LOAD_4X4: 16,
    FIFO_MTX_LOAD_4X3: 12,
    FIFO_COLOR: 1,
    FIFO_TEX_COORD: 1,
    FIFO_VERTEX16: 2,
    FIFO_POLY_FORMAT: 1,
    FIFO_TEX_FORMAT: 1,
    FIFO_PAL_FORMAT: 1,
    FIFO_BEGIN: 1,
}

FIFO_PATCH_COMPOSED = "composed"
FIFO_PATCH_COLOR = "color"
FIFO_PATCH_TEXCOORD = "texcoord"
FIFO_PATCH_EPOCH_TEX = "epoch_tex"
FIFO_PATCH_EPOCH_PAL = "epoch_pal"
FIFO_PATCH_EPOCH_POLY = "epoch_poly"
FIFO_PATCH_EPOCH_BEGIN = "epoch_begin"
FIFO_PATCH_EPOCH_BEGIN_PARAM = "epoch_begin_param"

SOURCE_STATE_EFFECTS = {
    0xe3: 2, 0xfc: 3, 0xd7: 4, 0xd9: 5, 0xfd: 6,
    0xf5: 7, 0xf0: 8, 0xf3: 9, 0xf2: 10, 0xfa: 11,
}
SOURCE_SYNC_OPS = frozenset((0xe6, 0xe7, 0xe8))
SOURCE_TRIANGLE_OPS = frozenset((0x05, 0x06))
SOURCE_ACTION_OPS = frozenset((0x01, 0x02))
SOURCE_MATERIAL_DL = 0xde
SOURCE_END_DL = 0xdf


def _discover_owner_roots(payload: bytes, owner_name: str) -> list[int]:
    joint_tree_offset, descriptor_count = OWNER_JOINT_TREES[owner_name]
    if joint_tree_offset + descriptor_count * DOBJ_DESC_SIZE > len(payload):
        raise ValueError(f"{owner_name} JointTree is out of range")
    descriptors = []
    for descriptor_index in range(descriptor_count):
        offset = joint_tree_offset + descriptor_index * DOBJ_DESC_SIZE
        depth, reloc_pointer = struct.unpack_from(">II", payload, offset)
        display_offset = None if reloc_pointer == 0 else \
            (reloc_pointer & 0xffff) * 4
        if display_offset is not None and display_offset >= len(payload):
            raise ValueError(
                f"{owner_name} JointTree entry {descriptor_index}: "
                f"display target 0x{display_offset:x} is out of range"
            )
        descriptors.append((depth, display_offset))
    if descriptors[-1] != (18, None):
        raise ValueError(f"{owner_name} JointTree lost its depth-18 sentinel")
    descriptors = descriptors[:-1]
    setup_words = OWNER_SETUP_PARTS[owner_name]
    selected = [
        index for index in range(len(descriptors))
        if setup_words[index // 32] & (1 << (31 - (index & 31)))
    ]
    if selected != list(range(len(selected))):
        raise ValueError(f"{owner_name} setup_parts is not a contiguous prefix")
    if len(selected) + 1 != len(descriptors):
        raise ValueError(f"{owner_name} selected JointTree cardinality changed")
    if any(offset is not None for _, offset in descriptors[len(selected):]):
        raise ValueError(f"{owner_name} setup_parts dropped a drawable descriptor")
    roots = [
        offset for _, offset in descriptors[:len(selected)]
        if offset is not None
    ]
    if len(roots) != OWNER_PLAN_COUNTS[owner_name][1]:
        raise ValueError(f"{owner_name} drawable root cardinality changed")
    return roots


def _source_commands(payload: bytes, owner_name: str, root_offset: int):
    commands = []
    while len(commands) < 256:
        offset = root_offset + len(commands) * 8
        if offset + 8 > len(payload):
            raise ValueError(f"{owner_name} root 0x{root_offset:x} is truncated")
        w0, w1 = struct.unpack_from(">II", payload, offset)
        commands.append((w0 >> 24, w0, w1))
        if (w0 >> 24) == SOURCE_END_DL:
            return commands
    raise ValueError(f"{owner_name} root 0x{root_offset:x} exceeds 255 commands")


def _decode_control(owner_name: str, root_index: int, commands):
    before, after = [], []
    before_sync = after_sync = 0
    material = INVALID_U8
    after_material = False
    for command_index, op, w0, w1 in commands:
        if op == SOURCE_MATERIAL_DL:
            if material != INVALID_U8 or (w1 >> 24) != SOURCE_SEGMENT_E or \
                    (w1 & 7) != 0:
                raise ValueError(
                    f"{owner_name} root {root_index} command {command_index}: "
                    "invalid material callback"
                )
            material = (w1 & 0x00ffffff) // 8
            if material >= INVALID_U8:
                raise ValueError(f"{owner_name} root {root_index}: material overflow")
            after_material = True
        elif op in SOURCE_SYNC_OPS:
            if after_material:
                after_sync += 1
            else:
                before_sync += 1
        elif (op == SOURCE_G_MOVEWORD and
              ((w0 >> 16) & 0xff) == SOURCE_G_MW_LIGHTCOL and
              (w0 & 0xffff) in SOURCE_LIGHTCOL_OFFSETS):
            continue
        elif op in SOURCE_STATE_EFFECTS:
            if op == 0xfd:
                w1 = (w1 & 0xffff) * 4
            row = (w0, w1, SOURCE_STATE_EFFECTS[op])
            (after if after_material else before).append(row)
        else:
            raise ValueError(
                f"{owner_name} root {root_index} command {command_index}: "
                f"unsupported control opcode 0x{op:02x}"
            )
    return before, after, before_sync, after_sync, material


def _append_state_span(rows, states, state_lookup, sequence):
    first = len(sequence)
    for row in rows:
        state_index = state_lookup.get(row)
        if state_index is None:
            state_index = len(states)
            if state_index >= 256:
                raise ValueError("native fighter state table exceeds u8 indices")
            state_lookup[row] = state_index
            states.append(row)
        sequence.append(state_index)
    return first, len(rows)


def _decode_action(
        payload: bytes, owner_name: str, root_index: int, command_index: int,
        command, slots, actions) -> None:
    op, w0, w1 = command
    if op == 0x01:
        count = (w0 >> 12) & 0xff
        end = (w0 >> 1) & 0x7f
        index = end - count
        source_offset = (w1 & 0xffff) * 4
        if (count == 0 or index < 0 or end > VERTEX_CACHE_SIZE or
                source_offset + count * SOURCE_VERTEX_SIZE > len(payload)):
            raise ValueError(
                f"{owner_name} root {root_index} command {command_index}: "
                "invalid vertex load"
            )
        actions.append((0, command_index, index, count, source_offset, 0, 0))
        for slot in range(index, end):
            slots[slot] = root_index
    else:
        index = (w0 & 0xffff) >> 1
        if ((w0 >> 16) & 0xff) != 0x14 or index >= VERTEX_CACHE_SIZE or \
                slots[index] is None:
            raise ValueError(
                f"{owner_name} root {root_index} command {command_index}: "
                "invalid MODIFYVTX ST"
            )
        s, t = struct.unpack(">hh", w1.to_bytes(4, "big"))
        actions.append((1, command_index, index, 0, 0, s, t))


def _pack_rows(fmt: str, rows) -> bytes:
    return b"".join(struct.pack(fmt, *row) for row in rows)


def build_source_export(repo_root: Path) -> dict[str, bytes]:
    """Rebuild the former profile export directly from Mario/Fox O2R data."""
    states, sequence, actions, triangles, runs, epochs = [], [], [], [], [], []
    state_lookup = {}
    owner_roots = {}
    for owner_name in ("mario", "fox"):
        payload = load_o2r_payload(repo_root, owner_name)
        roots = []
        slots = [None] * VERTEX_CACHE_SIZE
        for root_index, root_offset in enumerate(
                _discover_owner_roots(payload, owner_name)):
            commands = _source_commands(payload, owner_name, root_offset)
            triangle_blocks = []
            command_index = 0
            while command_index < len(commands) - 1:
                if commands[command_index][0] not in SOURCE_TRIANGLE_OPS:
                    command_index += 1
                    continue
                block_start = command_index
                while (command_index < len(commands) - 1 and
                       commands[command_index][0] in SOURCE_TRIANGLE_OPS):
                    command_index += 1
                triangle_blocks.append((block_start, command_index))
            if not triangle_blocks:
                raise ValueError(f"{owner_name} root {root_index} has no triangles")

            first_epoch = len(epochs)
            cursor = 0
            for block_start, block_end in triangle_blocks:
                action_indices = [
                    index for index in range(cursor, block_start)
                    if commands[index][0] in SOURCE_ACTION_OPS
                ]
                if not action_indices or any(
                        commands[index][0] not in SOURCE_ACTION_OPS
                        for index in range(action_indices[0], block_start)):
                    raise ValueError(
                        f"{owner_name} root {root_index}: malformed action span"
                    )
                control = [
                    (index, *commands[index])
                    for index in range(cursor, action_indices[0])
                ]
                before, after, before_sync, after_sync, material = \
                    _decode_control(owner_name, root_index, control)
                before_first, before_count = _append_state_span(
                    before, states, state_lookup, sequence
                )
                if after:
                    after_first, after_count = _append_state_span(
                        after, states, state_lookup, sequence
                    )
                else:
                    after_first, after_count = 0xffff, 0

                first_action = len(actions)
                for index in action_indices:
                    _decode_action(
                        payload, owner_name, root_index, index,
                        commands[index], slots, actions
                    )

                first_run = len(runs)
                current_class = None
                run_first = len(triangles)
                run_mask = 0
                for index in range(block_start, block_end):
                    op, w0, w1 = commands[index]
                    for half, indices in enumerate(
                            stage_manifest.decode_triangles(op, w0, w1)):
                        bindings = {slots[slot] for slot in indices}
                        if None in bindings or root_index not in bindings:
                            raise ValueError(
                                f"{owner_name} root {root_index} command {index}: "
                                "triangle uses an invalid binding"
                            )
                        submit_class = 0 if bindings == {root_index} else 1
                        if current_class is not None and \
                                submit_class != current_class:
                            runs.append((
                                run_first, len(triangles) - run_first,
                                current_class, run_mask,
                            ))
                            run_first = len(triangles)
                            run_mask = 0
                        current_class = submit_class
                        compact = (indices[0] << 10) | (indices[1] << 5) | indices[2]
                        if op == 0x06 and half == 0:
                            compact |= 0x8000
                        triangles.append(compact)
                        run_mask |= sum(1 << slot for slot in set(indices))
                runs.append((
                    run_first, len(triangles) - run_first,
                    current_class, run_mask,
                ))
                epochs.append((
                    before_first, after_first, first_action, first_run,
                    before_count, after_count, before_sync, after_sync,
                    len(actions) - first_action, len(runs) - first_run,
                    material, block_start,
                ))
                cursor = block_end

            tail_control = [
                (index, *commands[index])
                for index in range(cursor, len(commands) - 1)
            ]
            tail, tail_after, tail_sync, tail_after_sync, tail_material = \
                _decode_control(owner_name, root_index, tail_control)
            if tail_after or tail_after_sync or tail_material != INVALID_U8:
                raise ValueError(f"{owner_name} root {root_index}: invalid tail")
            if tail:
                tail_first, tail_count = _append_state_span(
                    tail, states, state_lookup, sequence
                )
            else:
                tail_first, tail_count = 0xffff, 0
            roots.append((
                root_offset, first_epoch, tail_first, len(commands),
                len(epochs) - first_epoch, tail_count, tail_sync, 0,
            ))
        owner_roots[owner_name] = roots

    data = {
        "state": _pack_rows("<IIB3x", states),
        "sequence": bytes(sequence),
        "vertex": _pack_rows("<BBBBIhh", actions),
        "triangles": _pack_rows("<H", ((value,) for value in triangles)),
        "runs": _pack_rows("<HBBI", runs),
        "epochs": _pack_rows("<HHHHBBBBBBBB", epochs),
        "mario_roots": _pack_rows("<IHHHBBBB2x", owner_roots["mario"]),
        "fox_roots": _pack_rows("<IHHHBBBB2x", owner_roots["fox"]),
    }
    for name, expected_hash in SOURCE_EXPORT_HASHES.items():
        actual_hash = hashlib.sha256(data[name]).hexdigest()
        if actual_hash != expected_hash:
            raise ValueError(f"{name}: SHA256 {actual_hash} != {expected_hash}")
    return data


def decode_export(repo_root: Path | None = None) -> dict[str, bytes]:
    """Compatibility entry point for the existing generator checkers."""
    if repo_root is None:
        repo_root = Path(__file__).resolve().parents[1]
    return build_source_export(Path(repo_root).resolve())


def unpack_many(fmt: str, payload: bytes):
    size = struct.calcsize(fmt)
    if len(payload) % size:
        raise ValueError(f"{len(payload)} bytes is not a multiple of {size}")
    return [item for item in struct.iter_unpack(fmt, payload)]


def load_o2r_payload(repo_root: Path, owner_name: str) -> bytes:
    relative_path, expected_file_id, expected_hash = O2R_ASSETS[owner_name]
    path = repo_root / relative_path
    source = path.read_bytes()
    actual_hash = hashlib.sha256(source).hexdigest()
    if actual_hash != expected_hash:
        raise ValueError(
            f"{owner_name} O2R: SHA256 {actual_hash} != {expected_hash}"
        )
    if len(source) < O2R_RESOURCE_HEADER_SIZE + 16:
        raise ValueError(f"{owner_name} O2R: truncated resource header")
    if source[4:8] != b"OLER":
        raise ValueError(f"{owner_name} O2R: invalid resource magic")
    file_id = struct.unpack_from("<I", source, O2R_RESOURCE_HEADER_SIZE)[0]
    if file_id != expected_file_id:
        raise ValueError(
            f"{owner_name} O2R: file ID 0x{file_id:x} != 0x{expected_file_id:x}"
        )
    extern_count = struct.unpack_from(
        "<I", source, O2R_RESOURCE_HEADER_SIZE + 8
    )[0]
    data_size_offset = O2R_RESOURCE_HEADER_SIZE + 12 + extern_count * 2
    if data_size_offset + 4 > len(source):
        raise ValueError(f"{owner_name} O2R: truncated extern table")
    data_size = struct.unpack_from("<I", source, data_size_offset)[0]
    data_offset = data_size_offset + 4
    data_end = data_offset + data_size
    if data_end != len(source):
        raise ValueError(
            f"{owner_name} O2R: data ends at 0x{data_end:x}, "
            f"file ends at 0x{len(source):x}"
        )
    return source[data_offset:data_end]


def decode_epoch_light_color_state(
        payload: bytes, owner_name: str, roots, epochs):
    """Recover compact root-prefix and exact intra-root light state."""
    result = {index: ([], []) for index in range(len(epochs))}
    preambles = []
    prefix_command_count = 0
    intra_root_command_count = 0
    for root_index, root in enumerate(roots):
        if root[7] != 0:
            raise ValueError(
                f"{owner_name} root {root_index}: reserved byte is not free"
            )
        if root[0] + root[3] * 8 > len(payload):
            raise ValueError(
                f"{owner_name} root {root_index}: source command span is truncated"
            )
        light_commands = []
        for command_index in range(root[3]):
            w0, w1 = struct.unpack_from(
                ">II", payload, root[0] + command_index * 8
            )
            if ((w0 >> 24) != SOURCE_G_MOVEWORD or
                    ((w0 >> 16) & 0xff) != SOURCE_G_MW_LIGHTCOL):
                continue
            offset = w0 & 0xffff
            if offset not in SOURCE_LIGHTCOL_OFFSETS:
                raise ValueError(
                    f"{owner_name} root {root_index}: unsupported "
                    f"G_MW_LIGHTCOL offset 0x{offset:x}"
                )
            light_commands.append((command_index, w0, w1))
        for pair_index in range(0, len(light_commands), 2):
            pair = light_commands[pair_index:pair_index + 2]
            if (len(pair) != 2 or
                    (pair[0][1] & 0xffff) not in (0x00, 0x18) or
                    (pair[1][1] & 0xffff) !=
                    ((pair[0][1] & 0xffff) + 4) or
                    pair[0][2] != pair[1][2]):
                raise ValueError(
                    f"{owner_name} root {root_index}: split light color pair"
                )

        first_root_triangle = epochs[root[1]][11]
        prefix_lights = [command for command in light_commands
                         if command[0] < first_root_triangle]
        if not prefix_lights:
            preambles.append(None)
        else:
            prefix_offsets = [w0 & 0xffff for _index, w0, _w1
                              in prefix_lights]
            if prefix_offsets != [0x00, 0x04, 0x18, 0x1c]:
                raise ValueError(
                    f"{owner_name} root {root_index}: light prefix is not "
                    "the compact two-pair layout"
                )
            preambles.append((prefix_lights[0][2], prefix_lights[2][2]))
        prefix_command_count += len(prefix_lights)

        previous_triangle = -1
        consumed_lights = 0
        for epoch_index in range(root[1], root[1] + root[4]):
            epoch = epochs[epoch_index]
            first_triangle = epoch[11]
            if (first_triangle <= previous_triangle or
                    first_triangle >= root[3]):
                raise ValueError(
                    f"{owner_name} root {root_index}: invalid epoch triangle "
                    f"index {first_triangle}"
                )
            material_calls = []
            epoch_lights = []
            for command_index in range(previous_triangle + 1, first_triangle):
                w0, w1 = struct.unpack_from(
                    ">II", payload, root[0] + command_index * 8
                )
                if ((w0 >> 24) == SOURCE_G_MOVEWORD and
                        ((w0 >> 16) & 0xff) == SOURCE_G_MW_LIGHTCOL):
                    epoch_lights.append((command_index, w0, w1))
                if ((w0 >> 24) == SOURCE_G_DL and
                        (w1 >> 24) == SOURCE_SEGMENT_E):
                    material_calls.append((command_index, w1 & 0xffffff))

            material_slot = epoch[10]
            if material_slot == INVALID_U8:
                if material_calls:
                    raise ValueError(
                        f"{owner_name} root {root_index} epoch {epoch_index}: "
                        "unexpected material call"
                    )
                material_command = None
            else:
                expected_segment = material_slot * 8
                matches = [index for index, segment in material_calls
                           if segment == expected_segment]
                if len(matches) != 1 or len(material_calls) != 1:
                    raise ValueError(
                        f"{owner_name} root {root_index} epoch {epoch_index}: "
                        f"material slot {material_slot} does not own one "
                        "segment-E call"
                    )
                material_command = matches[0]

            before, after = result[epoch_index]
            for command_index, w0, w1 in epoch_lights:
                if command_index < first_root_triangle:
                    continue
                target = (before if material_command is None or
                          command_index < material_command else after)
                target.append((w0, w1, NATIVE_STATE_LIGHT_COLOR))
                intra_root_command_count += 1
            consumed_lights += len(epoch_lights)
            previous_triangle = first_triangle

        tail_lights = [index for index, _w0, _w1 in light_commands
                       if index > previous_triangle]
        if tail_lights:
            raise ValueError(
                f"{owner_name} root {root_index}: light commands after the "
                f"final triangle epoch at {tail_lights}"
            )
        if consumed_lights != len(light_commands):
            raise ValueError(
                f"{owner_name} root {root_index}: recovered "
                f"{consumed_lights}/{len(light_commands)} light commands"
            )
    return (result, preambles, prefix_command_count,
            intra_root_command_count)


def restore_epoch_light_color_state(
        state, sequence, epochs, root_groups, additions):
    """Fold recovered light words into the existing before/after state ABI."""
    state = list(state)
    rebuilt_sequence = []
    rebuilt_epochs = []
    for epoch_index, epoch in enumerate(epochs):
        before = (list(sequence[epoch[0]:epoch[0] + epoch[4]])
                  if epoch[4] else [])
        after = (list(sequence[epoch[1]:epoch[1] + epoch[5]])
                 if epoch[5] else [])
        for target, recovered in zip((before, after), additions[epoch_index]):
            for delta in recovered:
                if delta not in state:
                    state.append(delta)
                target.append(state.index(delta))
        if len(before) > 0xff or len(after) > 0xff:
            raise ValueError(f"epoch {epoch_index}: state span exceeds u8")
        row = list(epoch)
        row[0] = len(rebuilt_sequence) if before else 0xffff
        rebuilt_sequence.extend(before)
        row[1] = len(rebuilt_sequence) if after else 0xffff
        rebuilt_sequence.extend(after)
        row[4] = len(before)
        row[5] = len(after)
        rebuilt_epochs.append(tuple(row))
    rebuilt_root_groups = []
    for roots in root_groups:
        rebuilt_roots = []
        for root in roots:
            tail = (list(sequence[root[2]:root[2] + root[5]])
                    if root[5] else [])
            row = list(root)
            row[2] = len(rebuilt_sequence) if tail else 0xffff
            rebuilt_sequence.extend(tail)
            rebuilt_roots.append(tuple(row))
        rebuilt_root_groups.append(rebuilt_roots)
    if len(state) > 0x100 or len(rebuilt_sequence) > 0xffff:
        raise ValueError("recovered light state exceeds the compact state ABI")
    if len(rebuilt_sequence) != len(sequence) + 28:
        raise ValueError(
            "recovered light state sequence changed size: "
            f"{len(rebuilt_sequence)} != {len(sequence) + 28}"
        )
    return state, rebuilt_sequence, rebuilt_epochs, rebuilt_root_groups


def decode_source_vertex(payload: bytes, source_offset: int):
    if source_offset < 0 or source_offset + SOURCE_VERTEX_SIZE > len(payload):
        raise ValueError(f"source vertex offset 0x{source_offset:x} is out of range")
    x, y, z, _, s, t, r, g, b, a = struct.unpack_from(
        ">hhhHhhBBBB", payload, source_offset
    )
    if a == 0:
        a = 0xff
    rgba = (r << 24) | (g << 16) | (b << 8) | a
    return x, y, z, s, t, rgba


def build_direct_epoch_policies(epoch_count: int) -> list[int]:
    if epoch_count != 49:
        raise ValueError(f"direct policy expects 49 epochs, got {epoch_count}")
    classified = (
        DIRECT_POLICY_TEXTURED_EPOCHS |
        DIRECT_POLICY_LIT_ONLY_EPOCHS |
        DIRECT_POLICY_ALT_ALPHA_EPOCHS
    )
    if max(classified | DIRECT_POLICY_CULL_NONE_EPOCHS) >= epoch_count:
        raise ValueError("direct policy names an out-of-range epoch")
    if ((DIRECT_POLICY_TEXTURED_EPOCHS & DIRECT_POLICY_LIT_ONLY_EPOCHS) or
            (DIRECT_POLICY_TEXTURED_EPOCHS &
             DIRECT_POLICY_ALT_ALPHA_EPOCHS) or
            (DIRECT_POLICY_LIT_ONLY_EPOCHS &
             DIRECT_POLICY_ALT_ALPHA_EPOCHS)):
        raise ValueError("direct policy families overlap")

    result = []
    for epoch_index in range(epoch_count):
        if epoch_index in DIRECT_POLICY_TEXTURED_EPOCHS:
            family = 0
        elif epoch_index in DIRECT_POLICY_LIT_ONLY_EPOCHS:
            family = 2
        elif epoch_index in DIRECT_POLICY_ALT_ALPHA_EPOCHS:
            family = 3
        else:
            family = 1
        if epoch_index in DIRECT_POLICY_CULL_NONE_EPOCHS:
            family |= DIRECT_POLICY_CULL_NONE
        result.append(family)
    return result


def build_joint_push_flags(owner_name: str, parents: list[int]):
    """Recover BattleShip's root-or-next-sibling matrix-push decisions.

    Joint rows are already child/sibling preorder. Parent indices plus this
    single flag let a direct executor pop back to the next row's parent without
    widening the packed u16 schedule or adding a synthetic camera row.
    """
    expected_joint_count, _ = OWNER_PLAN_COUNTS[owner_name]
    if len(parents) != expected_joint_count:
        raise ValueError(
            f"{owner_name} hierarchy expects {expected_joint_count} joints, "
            f"got {len(parents)}"
        )
    if not parents or parents[0] != INVALID_U8:
        raise ValueError(f"{owner_name} hierarchy has no synthetic TopN root")

    children = [[] for _ in parents]
    depths = [0] * len(parents)
    for joint_index, parent in enumerate(parents):
        if joint_index == 0:
            continue
        if (parent == INVALID_U8) or (parent >= joint_index):
            raise ValueError(
                f"{owner_name} joint {joint_index}: parent {parent} is not "
                "an earlier preorder joint"
            )
        children[parent].append(joint_index)
        depths[joint_index] = depths[parent] + 1

    preorder = []

    def visit(joint_index: int):
        preorder.append(joint_index)
        for child in children[joint_index]:
            visit(child)

    visit(0)
    if preorder != list(range(len(parents))):
        raise ValueError(
            f"{owner_name} hierarchy is not in BattleShip child/sibling preorder"
        )

    next_siblings = [INVALID_U8] * len(parents)
    for child_list in children:
        for child_offset, child in enumerate(child_list[:-1]):
            next_siblings[child] = child_list[child_offset + 1]

    push_flags = [
        (parent == INVALID_U8) or (next_siblings[joint_index] != INVALID_U8)
        for joint_index, parent in enumerate(parents)
    ]
    push_count = sum(push_flags)
    # gcDrawDObjTree emits one matching pop after each pushed DObj subtree.
    pop_count = push_count
    expected_seed, expected_push, expected_pop, _, _ = (
        OWNER_GX_PLAN_COUNTS[owner_name]
    )
    if (expected_seed, push_count, pop_count) != (
            1, expected_push, expected_pop):
        raise ValueError(
            f"{owner_name} hierarchy accounting changed: "
            f"seed/push/pop=1/{push_count}/{pop_count}"
        )
    max_source_depth = max(depths)
    # A direct executor seeds the camera once, then follows this exact preorder;
    # slots 0..15 conservatively cover every live hierarchy depth.
    if max_source_depth >= GX_HIERARCHY_SLOT_LIMIT:
        raise ValueError(
            f"{owner_name} hierarchy depth {max_source_depth} reaches reserved "
            "cross-binding slots"
        )
    return push_flags, (1, push_count, pop_count, max_source_depth)


def decode_joint_topology(
        payload: bytes, owner_name: str, roots: list[tuple]):
    joint_tree_offset, descriptor_count = OWNER_JOINT_TREES[owner_name]
    tree_end = joint_tree_offset + descriptor_count * DOBJ_DESC_SIZE
    if tree_end > len(payload):
        raise ValueError(f"{owner_name} JointTree is out of range")

    depths = []
    display_offsets = []
    for descriptor_index in range(descriptor_count):
        descriptor_offset = joint_tree_offset + descriptor_index * DOBJ_DESC_SIZE
        depth, reloc_pointer = struct.unpack_from(
            ">II", payload, descriptor_offset
        )
        if reloc_pointer == 0:
            display_offset = None
        else:
            # O2R internal reloc words carry the next relocation word in the
            # high half and the pointer target word offset in the low half.
            display_offset = (reloc_pointer & 0xffff) * 4
            if display_offset >= len(payload):
                raise ValueError(
                    f"{owner_name} JointTree entry {descriptor_index}: "
                    f"display target 0x{display_offset:x} is out of range"
                )
        depths.append(depth)
        display_offsets.append(display_offset)

    if ((depths[-1] != 18) or (display_offsets[-1] is not None)):
        raise ValueError(f"{owner_name} JointTree lost its depth-18 sentinel")
    depths = depths[:-1]
    display_offsets = display_offsets[:-1]
    raw_descriptor_count = descriptor_count - 1
    if (len(depths) != raw_descriptor_count) or (depths[0] != 0):
        raise ValueError(f"{owner_name} JointTree topology cardinality changed")

    setup_words = OWNER_SETUP_PARTS[owner_name]
    selected_indices = []
    for descriptor_index in range(raw_descriptor_count):
        word_index = descriptor_index // 32
        bit_index = 31 - (descriptor_index & 31)
        if setup_words[word_index] & (1 << bit_index):
            selected_indices.append(descriptor_index)
    if selected_indices != list(range(len(selected_indices))):
        raise ValueError(
            f"{owner_name} setup_parts is no longer one contiguous prefix"
        )
    selected_count = len(selected_indices)
    if selected_count + 1 != raw_descriptor_count:
        raise ValueError(
            f"{owner_name} synthetic-TopN live cardinality changed"
        )
    if any(offset is not None for offset in display_offsets[selected_count:]):
        raise ValueError(
            f"{owner_name} setup_parts dropped a drawable raw descriptor"
        )
    depths = depths[:selected_count]
    display_offsets = display_offsets[:selected_count]

    root_offsets = [root[0] for root in roots]
    selected_display_offsets = [
        offset for offset in display_offsets if offset is not None
    ]
    if selected_display_offsets != root_offsets:
        raise ValueError(
            f"{owner_name} JointTree display preorder does not match "
            "canonical native roots"
        )
    root_by_offset = {
        offset: binding for binding, offset in enumerate(root_offsets)
    }

    raw_parents = []
    raw_bindings = []
    depth_stack = []
    for joint_index, (depth, display_offset) in enumerate(
            zip(depths, display_offsets)):
        if depth >= 18:
            raise ValueError(
                f"{owner_name} joint {joint_index}: invalid live depth {depth}"
            )
        if joint_index == 0:
            if depth != 0:
                raise ValueError(f"{owner_name} JointTree has no depth-zero root")
            parent = INVALID_U8
        else:
            if (depth == 0) or (depth > (depths[joint_index - 1] + 1)):
                raise ValueError(
                    f"{owner_name} joint {joint_index}: discontinuous depth {depth}"
                )
            if depth - 1 >= len(depth_stack):
                raise ValueError(
                    f"{owner_name} joint {joint_index}: missing parent at "
                    f"depth {depth - 1}"
                )
            parent = depth_stack[depth - 1]
        if len(depth_stack) <= depth:
            depth_stack.append(joint_index)
        else:
            depth_stack[depth] = joint_index
            del depth_stack[depth + 1:]
        raw_parents.append(parent)
        raw_bindings.append(
            INVALID_U8 if display_offset is None else
            root_by_offset[display_offset]
        )

    # ftManagerMakeFighter creates TopN before the selected JointTree prefix.
    # Shift every raw index by one and parent raw roots directly to TopN.
    parents = [INVALID_U8]
    bindings = [INVALID_U8]
    parents.extend(
        0 if parent == INVALID_U8 else parent + 1
        for parent in raw_parents
    )
    bindings.extend(raw_bindings)

    expected_joint_count, expected_binding_count = OWNER_PLAN_COUNTS[owner_name]
    if len(parents) != expected_joint_count:
        raise ValueError(
            f"{owner_name} live joint count {len(parents)} != "
            f"{expected_joint_count}"
        )
    if len(roots) != expected_binding_count:
        raise ValueError(
            f"{owner_name} logical binding count {len(roots)} != "
            f"{expected_binding_count}"
        )

    binding_joints = [INVALID_U8] * len(roots)
    for joint_index, binding in enumerate(bindings):
        if binding != INVALID_U8:
            if binding_joints[binding] != INVALID_U8:
                raise ValueError(
                    f"{owner_name} binding {binding} appears more than once"
                )
            binding_joints[binding] = joint_index
    if any(joint == INVALID_U8 for joint in binding_joints):
        raise ValueError(f"{owner_name} JointTree does not cover every binding")

    binding_parents = []
    for binding, joint_index in enumerate(binding_joints):
        ancestor = parents[joint_index]
        while ((ancestor != INVALID_U8) and
               (bindings[ancestor] == INVALID_U8)):
            ancestor = parents[ancestor]
        binding_parents.append(
            INVALID_U8 if ancestor == INVALID_U8 else bindings[ancestor]
        )

    cross_slots = [PACKED_GX_SLOT_CURRENT] * len(roots)
    physical_slots = set()
    for binding, palette_slot in OWNER_CROSS_BINDING_SLOTS[owner_name]:
        if binding >= len(roots):
            raise ValueError(
                f"{owner_name} cross binding {binding} is out of range"
            )
        if ((palette_slot < GX_HIERARCHY_SLOT_LIMIT) or
                (palette_slot >= PACKED_GX_SLOT_CURRENT) or
                (palette_slot == GX_CAMERA_SEED_SLOT)):
            raise ValueError(
                f"{owner_name} cross binding {binding}: physical slot "
                f"{palette_slot} overlaps the camera/hierarchy namespace"
            )
        if palette_slot in physical_slots:
            raise ValueError(
                f"{owner_name} physical slot {palette_slot} is not unique"
            )
        physical_slots.add(palette_slot)
        cross_slots[binding] = palette_slot

    expected_store_count = OWNER_GX_PLAN_COUNTS[owner_name][3]
    if len(physical_slots) != expected_store_count:
        raise ValueError(
            f"{owner_name} GX store count {len(physical_slots)} != "
            f"{expected_store_count}"
        )

    push_flags, hierarchy_counts = build_joint_push_flags(
        owner_name, parents
    )
    joint_schedule = []
    for joint_index, (parent, binding) in enumerate(zip(parents, bindings)):
        packed_parent = 31 if parent == INVALID_U8 else parent
        packed_binding = 31 if binding == INVALID_U8 else binding
        palette_slot = (
            31 if binding == INVALID_U8 else cross_slots[binding]
        )
        if ((packed_parent > 31) or (packed_binding > 31) or
                (palette_slot > 31)):
            raise ValueError(
                f"{owner_name} joint {joint_index}: topology exceeds packed ABI"
            )
        joint_schedule.append(
            packed_parent |
            (packed_binding << 5) |
            (palette_slot << 10) |
            (JOINT_SCHEDULE_PUSH_BEFORE if push_flags[joint_index] else 0)
        )
    return (
        joint_schedule,
        binding_parents,
        binding_joints,
        cross_slots,
        hierarchy_counts,
    )


def build_dense_geometry(
        vertex, triangles, runs, epochs, owners, repo_root: Path | None = None):
    if repo_root is None:
        repo_root = Path(__file__).resolve().parents[1]
    repo_root = Path(repo_root).resolve()
    payloads = {
        owner_name: load_o2r_payload(repo_root, owner_name)
        for owner_name, _ in owners
    }
    dense_vertices = []
    dense_color_sources = []
    dense_owners = []
    dense_corners = []
    action_dense_first = [INVALID_DENSE_VERTEX] * len(vertex)
    run_first_corner = [INVALID_DENSE_VERTEX] * len(runs)
    run_owners = []
    run_root_bindings = []
    run_binding_sets = []
    next_epoch = 0
    next_action = 0
    next_run = 0

    for owner_index, (owner_name, roots) in enumerate(owners):
        payload = payloads[owner_name]
        slots = [None] * VERTEX_CACHE_SIZE
        for root_ordinal, root in enumerate(roots):
            first_epoch = root[1]
            epoch_count = root[4]
            if first_epoch != next_epoch:
                raise ValueError(
                    f"{owner_name} root {root_ordinal}: epoch {first_epoch} "
                    f"is not source-order epoch {next_epoch}"
                )
            for epoch_index in range(first_epoch, first_epoch + epoch_count):
                if epoch_index >= len(epochs):
                    raise ValueError(f"epoch {epoch_index} is out of range")
                epoch = epochs[epoch_index]
                first_action = epoch[2]
                first_run = epoch[3]
                action_count = epoch[8]
                run_count = epoch[9]
                if first_action != next_action:
                    raise ValueError(
                        f"epoch {epoch_index}: action {first_action} is not "
                        f"source-order action {next_action}"
                    )
                if first_run != next_run:
                    raise ValueError(
                        f"epoch {epoch_index}: run {first_run} is not "
                        f"source-order run {next_run}"
                    )

                for action_index in range(
                        first_action, first_action + action_count):
                    if action_index >= len(vertex):
                        raise ValueError(f"vertex action {action_index} is out of range")
                    kind, _, index, count, source_offset, s, t = vertex[action_index]
                    if index >= VERTEX_CACHE_SIZE:
                        raise ValueError(
                            f"vertex action {action_index}: slot {index} is out of range"
                        )
                    if kind == 0:
                        if index + count > VERTEX_CACHE_SIZE:
                            raise ValueError(
                                f"vertex action {action_index}: slots "
                                f"{index}..{index + count - 1} are out of range"
                            )
                        if source_offset + count * SOURCE_VERTEX_SIZE > len(payload):
                            raise ValueError(
                                f"vertex action {action_index}: source block is out of range"
                            )
                        if count:
                            action_dense_first[action_index] = len(dense_vertices)
                        for block_index in range(count):
                            decoded = decode_source_vertex(
                                payload,
                                source_offset + block_index * SOURCE_VERTEX_SIZE,
                            )
                            dense_id = len(dense_vertices)
                            dense_vertices.append(
                                (
                                    *decoded[:5],
                                    root_ordinal,
                                    index + block_index,
                                    decoded[5],
                                )
                            )
                            dense_color_sources.append(dense_id)
                            dense_owners.append(owner_index)
                            slots[index + block_index] = dense_id
                    elif kind == 1:
                        previous_id = slots[index]
                        if previous_id is None:
                            raise ValueError(
                                f"vertex action {action_index}: MODIFY_ST slot "
                                f"{index} is not live"
                            )
                        x, y, z, _, _, binding, cache_slot, rgba = (
                            dense_vertices[previous_id]
                        )
                        if cache_slot != index:
                            raise ValueError(
                                f"vertex action {action_index}: MODIFY_ST slot "
                                f"{index} aliases source slot {cache_slot}"
                            )
                        dense_id = len(dense_vertices)
                        action_dense_first[action_index] = dense_id
                        dense_vertices.append(
                            (x, y, z, s, t, binding, index, rgba)
                        )
                        dense_color_sources.append(
                            dense_color_sources[previous_id]
                        )
                        dense_owners.append(owner_index)
                        slots[index] = dense_id
                    else:
                        raise ValueError(
                            f"vertex action {action_index}: unsupported kind {kind}"
                        )
                    next_action += 1

                for run_index in range(first_run, first_run + run_count):
                    if run_index >= len(runs):
                        raise ValueError(f"run {run_index} is out of range")
                    first_triangle, triangle_count, submit_class, required_mask = (
                        runs[run_index]
                    )
                    if first_triangle + triangle_count > len(triangles):
                        raise ValueError(f"run {run_index}: triangle range is out of bounds")
                    run_first_corner[run_index] = len(dense_corners)
                    actual_mask = 0
                    bindings = set()
                    for triangle_index in range(
                            first_triangle, first_triangle + triangle_count):
                        compact = triangles[triangle_index] & 0x7fff
                        triangle_slots = (
                            (compact >> 10) & 31,
                            (compact >> 5) & 31,
                            compact & 31,
                        )
                        for slot in triangle_slots:
                            actual_mask |= 1 << slot
                            dense_id = slots[slot]
                            if dense_id is None:
                                raise ValueError(
                                    f"run {run_index}: triangle {triangle_index} "
                                    f"uses non-live slot {slot}"
                                )
                            dense_corners.append(dense_id)
                            if dense_vertices[dense_id][6] != slot:
                                raise ValueError(
                                    f"run {run_index}: dense vertex {dense_id} "
                                    f"records slot {dense_vertices[dense_id][6]}, "
                                    f"used through slot {slot}"
                                )
                            bindings.add(dense_vertices[dense_id][5])
                    if actual_mask != required_mask:
                        raise ValueError(
                            f"run {run_index}: slot mask 0x{actual_mask:08x} != "
                            f"0x{required_mask:08x}"
                        )
                    if submit_class == 0:
                        if bindings != {root_ordinal}:
                            raise ValueError(
                                f"raw run {run_index}: bindings {sorted(bindings)} "
                                f"do not match root {root_ordinal}"
                            )
                    elif submit_class == 1:
                        if root_ordinal not in bindings or len(bindings) < 2:
                            raise ValueError(
                                f"cross run {run_index}: bindings "
                                f"{sorted(bindings)} do not preserve root crossing"
                            )
                    else:
                        raise ValueError(
                            f"run {run_index}: unsupported submit class {submit_class}"
                        )
                    run_owners.append(owner_index)
                    run_root_bindings.append(root_ordinal)
                    run_binding_sets.append(frozenset(bindings))
                    next_run += 1
                next_epoch += 1

    if (next_epoch, next_action, next_run) != (
            len(epochs), len(vertex), len(runs)):
        raise ValueError(
            "owner traversal did not consume every epoch, vertex action, and run"
        )
    if len(dense_vertices) > INVALID_DENSE_VERTEX:
        raise ValueError("dense vertex IDs exceed the u16 encoding")
    if len(dense_corners) != len(triangles) * 3:
        raise ValueError(
            f"dense corner count {len(dense_corners)} != {len(triangles) * 3}"
        )
    if any(value == INVALID_DENSE_VERTEX for value in action_dense_first):
        raise ValueError("a canonical vertex action created no dense record")
    if any(value == INVALID_DENSE_VERTEX for value in run_first_corner):
        raise ValueError("a canonical run has no dense first-corner offset")
    if not (len(dense_vertices) == len(dense_color_sources) ==
            len(dense_owners)):
        raise ValueError("dense metadata cardinality mismatch")
    if not (len(runs) == len(run_owners) == len(run_root_bindings) ==
            len(run_binding_sets)):
        raise ValueError("run metadata cardinality mismatch")
    return (
        dense_vertices,
        dense_color_sources,
        dense_owners,
        dense_corners,
        action_dense_first,
        run_first_corner,
        run_owners,
        run_root_bindings,
        run_binding_sets,
    )


def build_direct_dense_tables(
        vertex, runs, dense_vertices, dense_color_sources, dense_corners,
        action_dense_first, run_first_corner, run_owners,
        run_root_bindings, run_binding_sets, owner_cross_slots):
    if len(dense_vertices) >= PACKED_DENSE_ID_LIMIT:
        raise ValueError(
            f"{len(dense_vertices)} dense IDs exceed the 10-bit direct ABI"
        )
    action_dense_spans = []
    for action_index, action in enumerate(vertex):
        kind, _, _, count, _, _, _ = action
        dense_count = count if kind == 0 else 1
        dense_first = action_dense_first[action_index]
        if ((dense_count == 0) or (dense_count > 31) or
                (dense_first >= PACKED_DENSE_ID_LIMIT) or
                ((dense_first + dense_count) > len(dense_vertices))):
            raise ValueError(
                f"vertex action {action_index}: dense span does not fit "
                "the packed direct ABI"
            )
        action_dense_spans.append(dense_first | (dense_count << 10))

    packed_corners = []
    run_first_unique = []
    run_unique_count = []
    run_unique_dense = []
    observed_cross_bindings = [set() for _ in owner_cross_slots]
    owner_restore_counts = [0 for _ in owner_cross_slots]
    for run_index, run in enumerate(runs):
        _, triangle_count, submit_class, _ = run
        corner_first = run_first_corner[run_index]
        corner_count = triangle_count * 3
        if corner_first != len(packed_corners):
            raise ValueError(
                f"run {run_index}: dense corners are not source ordered"
            )
        owner_index = run_owners[run_index]
        root_binding = run_root_bindings[run_index]
        cross_slots = owner_cross_slots[owner_index]
        if submit_class == 1:
            observed_cross_bindings[owner_index].update(
                run_binding_sets[run_index]
            )
            if ((root_binding >= len(cross_slots)) or
                    (cross_slots[root_binding] == PACKED_GX_SLOT_CURRENT)):
                raise ValueError(
                    f"cross run {run_index}: current binding {root_binding} "
                    "has no restorable GX palette slot"
                )
            current_palette_slot = cross_slots[root_binding]
            active_palette_slot = current_palette_slot

        unique = []
        seen = set()
        for dense_id in dense_corners[
                corner_first:corner_first + corner_count]:
            if dense_id >= PACKED_DENSE_ID_LIMIT:
                raise ValueError(
                    f"run {run_index}: dense ID {dense_id} is not packable"
                )
            binding = dense_vertices[dense_id][5]
            if submit_class == 0:
                if binding != root_binding:
                    raise ValueError(
                        f"raw run {run_index}: non-current binding {binding}"
                    )
                palette_slot = 0
            else:
                if binding == root_binding:
                    palette_slot = PACKED_GX_SLOT_CURRENT
                else:
                    if binding >= len(cross_slots):
                        raise ValueError(
                            f"cross run {run_index}: binding {binding} is out "
                            "of range"
                        )
                    palette_slot = cross_slots[binding]
                    if palette_slot == PACKED_GX_SLOT_CURRENT:
                        raise ValueError(
                            f"cross run {run_index}: binding {binding} has no "
                            "GX palette slot"
                        )
                physical_palette_slot = (
                    current_palette_slot
                    if palette_slot == PACKED_GX_SLOT_CURRENT else palette_slot
                )
                if physical_palette_slot != active_palette_slot:
                    owner_restore_counts[owner_index] += 1
                    active_palette_slot = physical_palette_slot
            packed_corners.append(dense_id | (palette_slot << 10))
            if dense_id not in seen:
                seen.add(dense_id)
                unique.append(dense_id)

        if ((submit_class == 1) and
                (active_palette_slot != current_palette_slot)):
            owner_restore_counts[owner_index] += 1

        if len(run_unique_dense) > 0xffff:
            raise ValueError("direct run unique list exceeds its u16 index ABI")
        if len(unique) > 0xff:
            raise ValueError(
                f"run {run_index}: {len(unique)} unique dense IDs exceed u8"
            )
        run_first_unique.append(len(run_unique_dense))
        run_unique_count.append(len(unique))
        run_unique_dense.extend(unique)

    if len(packed_corners) != len(dense_corners):
        raise ValueError("packed direct corner cardinality mismatch")
    for owner_index, (owner_name, _) in enumerate(O2R_ASSETS.items()):
        expected = {
            binding
            for binding, _ in OWNER_CROSS_BINDING_SLOTS[owner_name]
        }
        if observed_cross_bindings[owner_index] != expected:
            raise ValueError(
                f"{owner_name} cross-binding census changed: "
                f"{sorted(observed_cross_bindings[owner_index])}"
            )
        expected_restore_count = OWNER_GX_PLAN_COUNTS[owner_name][4]
        if owner_restore_counts[owner_index] != expected_restore_count:
            raise ValueError(
                f"{owner_name} GX restore count "
                f"{owner_restore_counts[owner_index]} != "
                f"{expected_restore_count}"
            )
    for dense_id, color_source in enumerate(dense_color_sources):
        if ((color_source > dense_id) or
                (color_source >= PACKED_DENSE_ID_LIMIT)):
            raise ValueError(
                f"dense vertex {dense_id}: invalid color source {color_source}"
            )

    return (
        action_dense_spans,
        packed_corners,
        run_first_unique,
        run_unique_count,
        run_unique_dense,
    )


def pack_fifo_vertex16(x: int, y: int, z: int, context: str) -> tuple[int, int]:
    """Encode one GX VERTEX16 without silently wrapping signed coordinates."""
    scaled_x = x * 16
    scaled_y = y * 16
    scaled_z = z * 16
    if any(
            value < -0x8000 or value > 0x7fff
            for value in (scaled_x, scaled_y, scaled_z)):
        raise ValueError(
            f"{context}: VERTEX16 signed overflow "
            f"xyz={x}/{y}/{z} scaled={scaled_x}/{scaled_y}/{scaled_z}"
        )
    return (
        (scaled_x & 0xffff) | ((scaled_y & 0xffff) << 16),
        scaled_z & 0xffff,
    )


class PackedFifoBuilder:
    """Serialize libnds packed commands and remember parameter word patches."""

    def __init__(self):
        self.words: list[int] = []
        self.pending: list[
            tuple[int, list[int], list[object | None], object | None]
        ] = []
        self.patches: dict[str, list[tuple]] = {
            FIFO_PATCH_COMPOSED: [],
            FIFO_PATCH_COLOR: [],
            FIFO_PATCH_TEXCOORD: [],
            FIFO_PATCH_EPOCH_TEX: [],
            FIFO_PATCH_EPOCH_PAL: [],
            FIFO_PATCH_EPOCH_POLY: [],
            FIFO_PATCH_EPOCH_BEGIN: [],
            FIFO_PATCH_EPOCH_BEGIN_PARAM: [],
        }
        self.command_counts: dict[int, int] = {}

    def command(
            self,
            command: int,
            parameters: list[int] | tuple[int, ...] = (),
            parameter_tags: list[object | None] | tuple[object | None, ...] = (),
            command_tag: object | None = None,
    ):
        expected = FIFO_PARAMETER_COUNTS.get(command)
        if expected is None:
            raise ValueError(f"unsupported packed FIFO command 0x{command:02x}")
        parameters = list(parameters)
        if len(parameters) != expected:
            raise ValueError(
                f"FIFO command 0x{command:02x}: {len(parameters)} params != "
                f"{expected}"
            )
        if parameter_tags:
            parameter_tags = list(parameter_tags)
            if len(parameter_tags) != expected:
                raise ValueError(
                    f"FIFO command 0x{command:02x}: patch tag cardinality "
                    "does not match parameters"
                )
        else:
            parameter_tags = [None] * expected
        self.pending.append(
            (command, parameters, parameter_tags, command_tag)
        )
        self.command_counts[command] = self.command_counts.get(command, 0) + 1
        if len(self.pending) == 4:
            self.flush()

    def flush(self):
        if not self.pending:
            return
        command_word = 0
        command_word_offset = len(self.words)
        for byte_index, (command, _, _, command_tag) in enumerate(self.pending):
            command_word |= command << (byte_index * 8)
            if command_tag is not None:
                patch_kind, patch_source = command_tag
                self.patches[patch_kind].append(
                    (command_word_offset, byte_index * 8, patch_source)
                )
        self.words.append(command_word)
        for _, parameters, parameter_tags, _ in self.pending:
            for value, tag in zip(parameters, parameter_tags):
                word_offset = len(self.words)
                self.words.append(value & 0xffffffff)
                if tag is not None:
                    patch_kind, patch_source = tag
                    self.patches[patch_kind].append(
                        (word_offset, patch_source)
                    )
        self.pending.clear()

    def finish(self):
        self.flush()
        if not self.words:
            raise ValueError("packed FIFO owner payload is empty")
        if len(self.words) > 0xffff:
            raise ValueError("packed FIFO owner payload exceeds u16 word offsets")
        return self.words, self.patches, self.command_counts


def build_packed_fifo_owner_plan(
        owner_name: str,
        owner_slot: int,
        roots: list[tuple],
        owner_root_first: int,
        epochs: list[tuple],
        runs: list[tuple],
        dense_vertices: list[tuple],
        packed_corners: list[int],
        run_first_corner: list[int],
        direct_epoch_policies: list[int],
        cross_slots: list[int],
):
    """Build one immutable whole-fighter FIFO template.

    Raw-composed matrices, live colors/texcoords, and epoch texture/poly state
    remain zero placeholders.  The ARM packet preflight patches those words
    only after the complete live owner contract has been accepted.
    """
    builder = PackedFifoBuilder()
    epoch_patch_words: dict[int, dict[str, int]] = {}
    raw_triangles = 0
    cross_triangles = 0
    raw_runs = 0
    cross_runs = 0
    restore_count = 0
    store_count = 0
    triangle_count = 0
    corner_count = 0
    textured_corner_count = 0

    # Source vertices are submitted in source/256 DS 4.12 coordinates.  Match
    # mode 8 exactly: keep GX projection at identity, CPU-compose each root,
    # then divide the complete composed row 3 by 256 before its 4x4 load.
    # Scaling split modelview/projection rows changes fixed-point rounding for
    # ordinary nonaligned live matrices and is permanently rejected by the
    # packet checker.
    builder.command(FIFO_MTX_MODE, [0])       # GL_PROJECTION
    builder.command(
        FIFO_MTX_LOAD_4X4,
        [
            1 << 12, 0, 0, 0,
            0, 1 << 12, 0, 0,
            0, 0, 1 << 12, 0,
            0, 0, 0, 1 << 12,
        ],
    )
    builder.command(FIFO_MTX_MODE, [2])       # GL_MODELVIEW

    for local_root, root in enumerate(roots):
        global_root = owner_root_first + local_root
        root_offset, first_epoch, _, _, epoch_count, _, _, _ = root
        del root_offset, global_root
        current_palette_slot = cross_slots[local_root]

        builder.command(
            FIFO_MTX_LOAD_4X4,
            [0] * 16,
            [(FIFO_PATCH_COMPOSED, local_root)] + [None] * 15,
        )
        if current_palette_slot != PACKED_GX_SLOT_CURRENT:
            builder.command(FIFO_MTX_STORE, [current_palette_slot])
            store_count += 1

        for epoch_index in range(first_epoch, first_epoch + epoch_count):
            epoch = epochs[epoch_index]
            first_run = epoch[3]
            run_count = epoch[9]
            textured = (
                DIRECT_POLICY_FAMILIES[
                    direct_epoch_policies[epoch_index] & 0x03
                ][3] != 0
            )
            epoch_patch_words[epoch_index] = {}
            builder.command(
                FIFO_TEX_FORMAT, [0],
                [(FIFO_PATCH_EPOCH_TEX, epoch_index)],
            )
            builder.command(
                FIFO_PAL_FORMAT, [0],
                [(FIFO_PATCH_EPOCH_PAL, epoch_index)],
            )
            builder.command(
                FIFO_POLY_FORMAT, [0],
                [(FIFO_PATCH_EPOCH_POLY, epoch_index)],
            )
            builder.command(
                FIFO_BEGIN,
                [0],
                [(FIFO_PATCH_EPOCH_BEGIN_PARAM, epoch_index)],
                (FIFO_PATCH_EPOCH_BEGIN, epoch_index),
            )  # GL_TRIANGLE, or a same-arity POLY_FORMAT reuse patch

            for run_index in range(first_run, first_run + run_count):
                _, run_triangle_count, submit_class, _ = runs[run_index]
                active_palette_slot = current_palette_slot
                run_corner_first = run_first_corner[run_index]
                run_corner_count = run_triangle_count * 3

                if submit_class == 0:
                    raw_runs += 1
                    raw_triangles += run_triangle_count
                elif submit_class == 1:
                    if current_palette_slot == PACKED_GX_SLOT_CURRENT:
                        raise ValueError(
                            f"{owner_name} cross run {run_index}: current root "
                            "has no physical palette slot"
                        )
                    cross_runs += 1
                    cross_triangles += run_triangle_count
                else:
                    raise ValueError(
                        f"{owner_name} run {run_index}: submit class "
                        f"{submit_class} is unsupported"
                    )

                for corner_offset in range(run_corner_count):
                    packed = packed_corners[run_corner_first + corner_offset]
                    dense_id = packed & (PACKED_DENSE_ID_LIMIT - 1)
                    palette_slot = packed >> PACKED_DENSE_ID_BITS
                    if dense_id >= len(dense_vertices):
                        raise ValueError(
                            f"{owner_name} run {run_index}: dense ID "
                            f"{dense_id} is out of range"
                        )
                    if submit_class == 1:
                        if palette_slot == PACKED_GX_SLOT_CURRENT:
                            palette_slot = current_palette_slot
                        if palette_slot != active_palette_slot:
                            builder.command(FIFO_MTX_RESTORE, [palette_slot])
                            active_palette_slot = palette_slot
                            restore_count += 1

                    x, y, z, _, _, _, _, _ = dense_vertices[dense_id]
                    xy, z_word = pack_fifo_vertex16(
                        x,
                        y,
                        z,
                        f"{owner_name} run {run_index} corner "
                        f"{corner_offset} dense {dense_id}",
                    )
                    builder.command(
                        FIFO_COLOR, [0],
                        [(FIFO_PATCH_COLOR, (epoch_index, dense_id))],
                    )
                    if textured:
                        builder.command(
                            FIFO_TEX_COORD, [0],
                            [(FIFO_PATCH_TEXCOORD,
                              (epoch_index, dense_id))],
                        )
                        textured_corner_count += 1
                    builder.command(FIFO_VERTEX16, [xy, z_word])
                    corner_count += 1

                if ((submit_class == 1) and
                        (active_palette_slot != current_palette_slot)):
                    builder.command(
                        FIFO_MTX_RESTORE, [current_palette_slot]
                    )
                    restore_count += 1
                triangle_count += run_triangle_count

    words, patches, command_counts = builder.finish()
    for patch_kind, field_name in (
            (FIFO_PATCH_EPOCH_TEX, "tex"),
            (FIFO_PATCH_EPOCH_PAL, "pal"),
            (FIFO_PATCH_EPOCH_POLY, "poly"),
            (FIFO_PATCH_EPOCH_BEGIN_PARAM, "begin_param")):
        for word_offset, epoch_index in patches[patch_kind]:
            epoch_patch_words[epoch_index][field_name] = word_offset
    color_patches = []
    color_spans = {}
    for word_offset, (epoch_index, dense_id) in patches[FIFO_PATCH_COLOR]:
        if epoch_index not in color_spans:
            color_spans[epoch_index] = [len(color_patches), 0]
        expected_index = color_spans[epoch_index][0] + \
            color_spans[epoch_index][1]
        if expected_index != len(color_patches):
            raise ValueError(
                f"{owner_name} epoch {epoch_index}: color patches are not "
                "a contiguous owner span"
            )
        color_patches.append((word_offset, dense_id))
        color_spans[epoch_index][1] += 1
    texcoord_patches = []
    texcoord_spans = {}
    for word_offset, (epoch_index, dense_id) in \
            patches[FIFO_PATCH_TEXCOORD]:
        if epoch_index not in texcoord_spans:
            texcoord_spans[epoch_index] = [len(texcoord_patches), 0]
        expected_index = texcoord_spans[epoch_index][0] + \
            texcoord_spans[epoch_index][1]
        if expected_index != len(texcoord_patches):
            raise ValueError(
                f"{owner_name} epoch {epoch_index}: texcoord patches are "
                "not a contiguous owner span"
            )
        texcoord_patches.append((word_offset, dense_id))
        texcoord_spans[epoch_index][1] += 1

    epoch_patches = []
    color_cursor = 0
    texcoord_cursor = 0
    for epoch_index in sorted(epoch_patch_words):
        fields = epoch_patch_words[epoch_index]
        begin_command = [
            (word, shift)
            for word, shift, source in patches[FIFO_PATCH_EPOCH_BEGIN]
            if source == epoch_index
        ]
        if ((set(fields) != {"tex", "pal", "poly", "begin_param"}) or
                (len(begin_command) != 1)):
            raise ValueError(
                f"{owner_name} epoch {epoch_index}: incomplete state patch"
            )
        root_index = next(
            local_root
            for local_root, root in enumerate(roots)
            if root[1] <= epoch_index < root[1] + root[4]
        )
        color_first, color_count = color_spans.get(
            epoch_index, (color_cursor, 0)
        )
        texcoord_first, texcoord_count = texcoord_spans.get(
            epoch_index, (texcoord_cursor, 0)
        )
        if ((color_first != color_cursor) or
                (texcoord_first != texcoord_cursor)):
            raise ValueError(
                f"{owner_name} epoch {epoch_index}: patch spans are not "
                "monotonic"
            )
        epoch_patches.append((
            fields["tex"], fields["pal"], fields["poly"],
            begin_command[0][0], fields["begin_param"],
            color_first, color_count, texcoord_first, texcoord_count,
            root_index, epoch_index, begin_command[0][1], 0,
        ))
        color_cursor += color_count
        texcoord_cursor += texcoord_count

    expected_store = OWNER_GX_PLAN_COUNTS[owner_name][3]
    expected_restore = OWNER_GX_PLAN_COUNTS[owner_name][4]
    if (store_count, restore_count) != (expected_store, expected_restore):
        raise ValueError(
            f"{owner_name} packet store/restore {store_count}/{restore_count} "
            f"!= {expected_store}/{expected_restore}"
        )
    expected_triangles = 320 if owner_name == "mario" else 306
    expected_corners = expected_triangles * 3
    expected_textured_corners = 192 if owner_name == "mario" else 189
    if (triangle_count, corner_count, textured_corner_count) != (
            expected_triangles, expected_corners, expected_textured_corners):
        raise ValueError(
            f"{owner_name} packet geometry census changed: "
            f"tri/corner/tex={triangle_count}/{corner_count}/"
            f"{textured_corner_count}"
        )
    matrix_patches = [
        (word_offset, source, 16)
        for word_offset, source in patches[FIFO_PATCH_COMPOSED]
    ]
    template_bytes = struct.pack(
        f"<{len(words)}I", *words
    )
    template_hash = int.from_bytes(
        hashlib.sha256(template_bytes).digest()[:4], "little"
    )
    return {
        "words": words,
        "matrix_patches": matrix_patches,
        "color_patches": color_patches,
        "texcoord_patches": texcoord_patches,
        "epoch_patches": epoch_patches,
        "template_hash": template_hash,
        "triangle_count": triangle_count,
        "raw_triangle_count": raw_triangles,
        "cross_triangle_count": cross_triangles,
        "run_count": raw_runs + cross_runs,
        "raw_run_count": raw_runs,
        "cross_run_count": cross_runs,
        "root_count": len(roots),
        "epoch_count": len(epoch_patches),
        "store_count": store_count,
        "restore_count": restore_count,
        "command_counts": command_counts,
    }


def emit_rows(
        type_name: str, name: str, rows: list[str],
        const: bool = True) -> list[str]:
    qualifier = "static const" if const else "static"
    result = [f"{qualifier} {type_name} {name}[{len(rows)}] =", "{"]
    result.extend(f"    {row}," for row in rows)
    result.append("};")
    result.append("")
    return result


def build_owner_source_context(repo_root: Path) -> dict[str, object]:
    """Recover the exact shared Mario/Fox source-order program inputs."""
    data = build_source_export(repo_root)
    state = unpack_many("<IIB3x", data["state"])
    sequence = list(data["sequence"])
    vertex = unpack_many("<BBBBIhh", data["vertex"])
    triangles = [item[0] for item in unpack_many("<H", data["triangles"])]
    runs = unpack_many("<HBBI", data["runs"])
    epochs = unpack_many("<HHHHBBBBBBBB", data["epochs"])
    mario_roots = unpack_many("<IHHHBBBB2x", data["mario_roots"])
    fox_roots = unpack_many("<IHHHBBBB2x", data["fox_roots"])
    if (len(state), len(sequence), len(vertex), len(triangles), len(runs),
            len(epochs), len(mario_roots), len(fox_roots)) != (
            54, 168, 76, 626, 67, 49, 14, 18):
        raise ValueError("canonical native-fighter IR cardinality changed")
    class_triangles = [0, 0]
    for _, triangle_count, submit_class, _ in runs:
        if submit_class >= len(class_triangles):
            raise ValueError(f"unsupported submit class {submit_class}")
        class_triangles[submit_class] += triangle_count
    if class_triangles != [582, 44]:
        raise ValueError(f"submit-class census changed: {class_triangles}")

    direct_epoch_policies = build_direct_epoch_policies(len(epochs))
    owner_roots = (("mario", mario_roots), ("fox", fox_roots))
    owner_topologies = []
    light_state_additions = {index: ([], []) for index in range(len(epochs))}
    light_preambles = [(0, 0)]
    owner_light_preamble_indices = {}
    owner_light_command_counts = {}
    root_prefix_light_command_count = 0
    intra_root_light_command_count = 0
    for owner_name, roots in owner_roots:
        payload = load_o2r_payload(repo_root, owner_name)
        owner_topologies.append(
            decode_joint_topology(payload, owner_name, roots)
        )
        (owner_light_state, owner_light_preambles,
         owner_prefix_command_count, owner_intra_command_count) = \
            decode_epoch_light_color_state(
                payload, owner_name, roots, epochs)
        owner_light_command_counts[owner_name] = (
            owner_prefix_command_count, owner_intra_command_count
        )
        for epoch_index, (before, after) in owner_light_state.items():
            light_state_additions[epoch_index][0].extend(before)
            light_state_additions[epoch_index][1].extend(after)
        indices = []
        for preamble in owner_light_preambles:
            if preamble is None:
                indices.append(0)
                continue
            if preamble not in light_preambles:
                light_preambles.append(preamble)
            indices.append(light_preambles.index(preamble))
        owner_light_preamble_indices[owner_name] = indices
        root_prefix_light_command_count += owner_prefix_command_count
        intra_root_light_command_count += owner_intra_command_count
    if (root_prefix_light_command_count, intra_root_light_command_count) != \
            (120, 28):
        raise ValueError(
            "native-owner source light command census changed: "
            f"prefix={root_prefix_light_command_count}, "
            f"intra-root={intra_root_light_command_count} != 120,28"
        )
    if (len(light_preambles) != 3 or
            light_preambles[1][0] != light_preambles[2][0]):
        raise ValueError(
            "native-owner root light prefixes no longer fit the compact ABI"
        )
    state, sequence, epochs, rebuilt_root_groups = \
        restore_epoch_light_color_state(
            state, sequence, epochs, (mario_roots, fox_roots),
            light_state_additions)
    mario_roots, fox_roots = rebuilt_root_groups

    return {
        "data": data,
        "state": state,
        "sequence": sequence,
        "vertex": vertex,
        "triangles": triangles,
        "runs": runs,
        "epochs": epochs,
        "mario_roots": mario_roots,
        "fox_roots": fox_roots,
        "owner_roots": (("mario", mario_roots), ("fox", fox_roots)),
        "owner_topologies": owner_topologies,
        "direct_epoch_policies": direct_epoch_policies,
        "light_preambles": light_preambles,
        "owner_light_preamble_indices": owner_light_preamble_indices,
        "owner_light_command_counts": owner_light_command_counts,
    }


def _append_checksum_rows(words: list[int], tag: int, rows) -> None:
    rows = tuple(rows)
    words.extend((tag, len(rows)))
    for row in rows:
        words.extend(int(value) for value in row)


def build_generated_mario_program(
        repo_root: Path, context: dict[str, object] | None = None
        ) -> dict[str, object]:
    """Build Task 27's fixed Mario joint/root/epoch/run program."""
    if context is None:
        context = build_owner_source_context(repo_root)
    roots = context["mario_roots"]
    epochs = context["epochs"]
    runs = context["runs"]
    state = context["state"]
    sequence = context["sequence"]
    policies = context["direct_epoch_policies"]
    light_indices = context["owner_light_preamble_indices"]["mario"]
    schedule, binding_parents, binding_joints, cross_slots, hierarchy_counts = \
        context["owner_topologies"][0]
    events = []
    root_rows = []
    epoch_rows = []
    run_rows = []
    state_rows = []
    state_events = []
    root_order = []
    epoch_order = []
    run_order = []
    raw_runs = 0
    cross_runs = 0
    triangle_count = 0
    phase_ids = {"before": 0, "after": 1, "tail": 2}

    def append_state_span(
            phase: str, root_index: int, epoch_index: int | None,
            first: int, count: int) -> None:
        for offset in range(count):
            sequence_index = first + offset
            delta_index = sequence[sequence_index]
            w0, w1, effect = state[delta_index]
            state_rows.append((
                phase_ids[phase], root_index,
                0xff if epoch_index is None else epoch_index,
                sequence_index, delta_index, effect, w0, w1,
            ))
            state_events.append({
                "phase": phase,
                "root": root_index,
                "epoch": epoch_index,
                "sequence_index": sequence_index,
                "delta_index": delta_index,
                "effect": effect,
                "w0": f"0x{w0:08x}",
                "w1": f"0x{w1:08x}",
            })

    for joint_index, packed in enumerate(schedule):
        binding = (packed >> 5) & 31
        events.append(("JOINT", (
            joint_index, 1 if packed & JOINT_SCHEDULE_PUSH_BEFORE else 0,
        )))
        if binding != 31:
            root = roots[binding]
            root_order.append(binding)
            root_rows.append((
                binding, *root[:7], light_indices[binding],
                binding_joints[binding], cross_slots[binding],
            ))
            events.append(("ROOT", (
                binding, binding_joints[binding], cross_slots[binding],
                root[1], root[4], root[3],
            )))
            for epoch_index in range(root[1], root[1] + root[4]):
                epoch = epochs[epoch_index]
                epoch_order.append(epoch_index)
                epoch_rows.append((binding, epoch_index, policies[epoch_index],
                                   *epoch))
                append_state_span(
                    "before", binding, epoch_index, epoch[0], epoch[4])
                append_state_span(
                    "after", binding, epoch_index, epoch[1], epoch[5])
                events.append(("EPOCH", (
                    epoch_index, policies[epoch_index], epoch[3], epoch[9],
                    epoch[10], epoch[8],
                )))
                for run_index in range(epoch[3], epoch[3] + epoch[9]):
                    run = runs[run_index]
                    run_order.append(run_index)
                    run_rows.append((binding, epoch_index, run_index, *run))
                    raw_runs += run[2] == 0
                    cross_runs += run[2] == 1
                    triangle_count += run[1]
                    events.append(("RUN", (
                        run_index, epoch_index, run[2], run[1],
                    )))
            append_state_span(
                "tail", binding, None, root[2], root[5])
            events.append(("ROOT_END", (binding,)))

        next_parent = ((schedule[joint_index + 1] & 31)
                       if joint_index + 1 < len(schedule) else 31)
        cursor = joint_index
        pop_count = 0
        while cursor != next_parent:
            cursor_packed = schedule[cursor]
            if cursor_packed & JOINT_SCHEDULE_PUSH_BEFORE:
                pop_count += 1
            cursor = cursor_packed & 31
        if pop_count:
            events.append(("POP", (pop_count,)))

    if (root_order != list(range(14)) or
            epoch_order != list(range(18)) or
            run_order != list(range(30))):
        raise ValueError("Mario generated program lost source order")
    if (len(schedule), len(root_order), len(epoch_order), len(run_order),
            triangle_count, raw_runs, cross_runs) != (25, 14, 18, 30,
                                                      320, 21, 9):
        raise ValueError("Mario generated program cardinality changed")
    if sum(value[0] for opcode, value in events if opcode == "POP") != 5:
        raise ValueError("Mario generated program pop census changed")

    source_words = [0x4d325341, O2R_ASSETS["mario"][1]]
    source_words.extend(bytes.fromhex(O2R_ASSETS["mario"][2]))
    table_words = []
    _append_checksum_rows(table_words, 0x4d325352, root_rows)
    _append_checksum_rows(table_words, 0x4d325353, epoch_rows)
    _append_checksum_rows(table_words, 0x4d325354, run_rows)
    _append_checksum_rows(table_words, 0x4d325358, state_rows)
    _append_checksum_rows(
        table_words, 0x4d325355, ((index, value)
                                  for index, value in enumerate(schedule)))
    _append_checksum_rows(
        table_words, 0x4d325356, ((index, value)
                                  for index, value in enumerate(binding_joints)))
    _append_checksum_rows(
        table_words, 0x4d325357, ((index, value)
                                  for index, value in enumerate(cross_slots)))
    opcode_ids = {
        "JOINT": 1, "ROOT": 2, "EPOCH": 3,
        "RUN": 4, "ROOT_END": 5, "POP": 6,
    }
    event_words = [0x4d325350, len(events)]
    for opcode, operands in events:
        event_words.extend((opcode_ids[opcode], len(operands), *operands))

    return {
        "events": tuple(events),
        "root_rows": tuple(root_rows),
        "epoch_rows": tuple(epoch_rows),
        "run_rows": tuple(run_rows),
        "state_events": tuple(state_events),
        "root_order": tuple(root_order),
        "epoch_order": tuple(epoch_order),
        "run_order": tuple(run_order),
        "schedule": tuple(schedule),
        "binding_parents": tuple(binding_parents),
        "binding_joints": tuple(binding_joints),
        "cross_slots": tuple(cross_slots),
        "hierarchy_counts": tuple(hierarchy_counts),
        "source_checksum": stage_manifest.fnv1a_u32(source_words),
        "table_checksum": stage_manifest.fnv1a_u32(table_words),
        "event_checksum": stage_manifest.fnv1a_u32(event_words),
        "triangle_count": triangle_count,
        "raw_run_count": raw_runs,
        "cross_run_count": cross_runs,
        "light_command_counts": tuple(
            context["owner_light_command_counts"]["mario"]),
    }


def render_generated_mario_program(program: dict[str, object]) -> list[str]:
    lines = [
        f"#define NDS_NATIVE_MARIO_GENERATED_SOURCE_CHECKSUM "
        f"0x{program['source_checksum']:08x}u",
        f"#define NDS_NATIVE_MARIO_GENERATED_TABLE_CHECKSUM "
        f"0x{program['table_checksum']:08x}u",
        f"#define NDS_NATIVE_MARIO_GENERATED_EVENT_CHECKSUM "
        f"0x{program['event_checksum']:08x}u",
        "#define NDS_NATIVE_MARIO_GENERATED_PROGRAM \\",
        "    do { \\",
    ]
    for opcode, operands in program["events"]:
        rendered = ", ".join(f"{value}u" for value in operands)
        lines.append(f"        NDS_TASK27_{opcode}({rendered}); \\")
    lines.extend(("    } while (0)", ""))
    return lines


def generate(repo_root: Path | None = None) -> str:
    if repo_root is None:
        repo_root = Path(__file__).resolve().parents[1]
    repo_root = Path(repo_root).resolve()
    context = build_owner_source_context(repo_root)
    state = context["state"]
    sequence = context["sequence"]
    vertex = context["vertex"]
    triangles = context["triangles"]
    runs = context["runs"]
    epochs = context["epochs"]
    mario_roots = context["mario_roots"]
    fox_roots = context["fox_roots"]
    owner_roots = context["owner_roots"]
    owner_topologies = context["owner_topologies"]
    direct_epoch_policies = context["direct_epoch_policies"]
    light_preambles = context["light_preambles"]
    owner_light_preamble_indices = context["owner_light_preamble_indices"]
    mario_program = build_generated_mario_program(repo_root, context)
    (
        dense_vertices,
        dense_color_sources,
        dense_owners,
        dense_corners,
        action_dense_first,
        run_first_corner,
        run_owners,
        run_root_bindings,
        run_binding_sets,
    ) = build_dense_geometry(
        vertex,
        triangles,
        runs,
        epochs,
        owner_roots,
        repo_root,
    )
    if (len(dense_vertices), len(dense_corners)) != (541, 1878):
        raise ValueError(
            "canonical dense fighter geometry cardinality changed: "
            f"{len(dense_vertices)} vertices, {len(dense_corners)} corners"
        )
    if dense_owners.count(0) != 255 or dense_owners.count(1) != 286:
        raise ValueError("canonical owner dense-vertex census changed")
    owner_cross_slots = [topology[3] for topology in owner_topologies]
    (
        action_dense_spans,
        packed_corners,
        run_first_unique,
        run_unique_count,
        run_unique_dense,
    ) = build_direct_dense_tables(
        vertex, runs, dense_vertices, dense_color_sources, dense_corners,
        action_dense_first, run_first_corner, run_owners,
        run_root_bindings, run_binding_sets, owner_cross_slots,
    )
    packet_plans = []
    owner_root_first = 0
    for owner_slot, ((owner_name, roots), topology) in enumerate(
            zip(owner_roots, owner_topologies)):
        packet_plans.append(build_packed_fifo_owner_plan(
            owner_name, owner_slot, roots, owner_root_first,
            epochs, runs, dense_vertices, packed_corners,
            run_first_corner, direct_epoch_policies, topology[3],
        ))
        owner_root_first += len(roots)

    lines = [
        "/* Generated by scripts/generate_nds_native_owners.py. */",
        "/* Canonical export: 32 roots, 49 epochs, 67 runs, 626 triangles. */",
        "/* Dense geometry: 541 immutable vertices, 1878 indexed corners. */",
        "/* Exact O2R state: 120 root-prefix and 28 intra-root light commands. */",
        "",
    ]
    lines += render_generated_mario_program(mario_program)
    lines += emit_rows(
        "NDSNativeStateDelta", "sNdsNativeFighterStateDeltas",
        [f"{{ 0x{w0:08x}u, 0x{w1:08x}u, {effect}u, {{ 0u, 0u, 0u }} }}"
         for w0, w1, effect in state],
    )
    lines += emit_rows(
        "u8", "sNdsNativeFighterStateSequence",
        [f"{value}u" for value in sequence],
    )
    lines += emit_rows(
        "NDSNativeVertexAction", "sNdsNativeFighterVertexActions",
        [f"{{ {kind}u, {command}u, {index}u, {count}u, 0x{offset:08x}u, {s}, {t} }}"
         for kind, command, index, count, offset, s, t in vertex],
    )
    lines += ["#if NDS_RENDERER_HW_TRIANGLES", ""]
    lines += [
        "/* Direct policy: low two epoch bits select a family; bit 0x80 */",
        "/* suppresses culling. Raw corners are plain 10-bit dense IDs. */",
        "/* Cross corners pack GX slots in bits 10..14; slot 31 means */",
        "/* logical current root, restored through its real palette slot. */",
        "/* ActionDenseSpans pack first dense in bits 0..9 and count in */",
        "/* bits 10..14. DenseColorSource preserves MODIFY_ST shading. */",
        "/* JointSchedule packs parent joint, logical binding, and physical */",
        "/* GX slot into successive 5-bit fields; 31 means none/root/current */",
        "/* by field. Bit 15 preserves BattleShip's root-or-next-sibling */",
        "/* push. Array order is child/sibling preorder; seed the camera once */",
        "/* before row 0 and derive matching pops from later parent rows. */",
        "",
    ]
    policy_flag_expressions = {
        "VERTEX|TEXTURE":
            "NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX | "
            "NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE",
        "MATERIAL|VERTEX":
            "NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL | "
            "NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX",
        "VERTEX": "NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX",
    }
    lines += emit_rows(
        "NDSNativeDirectPolicy", "sNdsNativeFighterDirectPolicies",
        ["{{ 0x{:08x}u, 0x{:08x}u, {}, {}u, {{ 0u, 0u }} }}".format(
            combine_w0, combine_w1, policy_flag_expressions[flags], textured)
         for combine_w0, combine_w1, flags, textured in
         DIRECT_POLICY_FAMILIES],
    )
    lines += emit_rows(
        "u8", "sNdsNativeFighterEpochDirectPolicy",
        [f"0x{value:02x}u" for value in direct_epoch_policies],
    )
    lines += emit_rows(
        "NDSNativeDenseVertex", "sNdsNativeFighterDenseVertices",
        ["{{ 0x{:08x}u, {}, {}, {}u, {}u, 0u }}".format(
             rgba, s, t, binding, cache_slot)
         for dense_id, (x, y, z, s, t, binding, cache_slot, rgba)
         in enumerate(dense_vertices)],
    )
    lines += ["#if NDS_RENDERER_PROFILE_LEVEL < 2", ""]
    lines += emit_rows(
        "NDSNativePreparedDenseVertex", "sNdsNativeFighterPreparedDense",
        ["{{ 0x{:08x}u, 0u, 0x{:04x}u, 0u, 0, 0 }}".format(
             *pack_fifo_vertex16(x, y, z, f"dense vertex {dense_id}"))
         for dense_id, (x, y, z, _s, _t, _binding, _cache_slot, _rgba)
         in enumerate(dense_vertices)],
        const=False,
    )
    lines += ["#endif", ""]
    lines += emit_rows(
        "u16", "sNdsNativeFighterActionDenseFirst",
        [f"{value}u" for value in action_dense_first],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterActionDenseSpans",
        [f"0x{value:04x}u" for value in action_dense_spans],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterDenseColorSource",
        [f"{value}u" for value in dense_color_sources],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterDenseCorners",
        [f"{value}u" for value in dense_corners],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterPackedCorners",
        [f"0x{value:04x}u" for value in packed_corners],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterRunFirstCorner",
        [f"{value}u" for value in run_first_corner],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterRunFirstUnique",
        [f"{value}u" for value in run_first_unique],
    )
    lines += emit_rows(
        "u8", "sNdsNativeFighterRunUniqueCount",
        [f"{value}u" for value in run_unique_count],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterRunUniqueDense",
        [f"{value}u" for value in run_unique_dense],
    )
    for owner_index, (owner_name, roots) in enumerate(owner_roots):
        (
            joint_schedule,
            binding_parents,
            binding_joints,
            cross_slots,
            _hierarchy_counts,
        ) = owner_topologies[owner_index]
        owner_title = owner_name.title()
        lines += emit_rows(
            "u8", f"sNdsNative{owner_title}CrossPaletteSlots",
            [f"{value}u" for value in cross_slots],
        )
        lines += emit_rows(
            "u8", f"sNdsNative{owner_title}BindingParents",
            [f"{value}u" for value in binding_parents],
        )
        lines += emit_rows(
            "u8", f"sNdsNative{owner_title}BindingJoints",
            [f"{value}u" for value in binding_joints],
        )
        lines += emit_rows(
            "u16", f"sNdsNative{owner_title}JointSchedule",
            [f"0x{value:04x}u" for value in joint_schedule],
        )
    lines += [
        "#if 0  /* Retained host-only exact packet fixture. */",
        "",
        "/* Whole-owner packed GX FIFO templates. Parameter offsets are */",
        "/* relative to the payload after the leading runtime word count. */",
        "/* GFX_END is intentionally absent per libnds glCallList. */",
        "",
    ]
    for owner_slot, ((owner_name, _), plan) in enumerate(
            zip(owner_roots, packet_plans)):
        owner_title = owner_name.title()
        lines += emit_rows(
            "u32", f"sNdsNative{owner_title}FifoWords",
            [f"0x{value:08x}u" for value in plan["words"]],
        )
        lines += emit_rows(
            "NDSNativeFifoMatrixPatch",
            f"sNdsNative{owner_title}FifoMatrixPatches",
            [f"{{ {word}u, {source}u, {count}u }}"
             for word, source, count in plan["matrix_patches"]],
        )
        lines += emit_rows(
            "NDSNativeFifoWordPatch",
            f"sNdsNative{owner_title}FifoColorPatches",
            [f"{{ {word}u, {source}u }}"
             for word, source in plan["color_patches"]],
        )
        lines += emit_rows(
            "NDSNativeFifoWordPatch",
            f"sNdsNative{owner_title}FifoTexcoordPatches",
            [f"{{ {word}u, {source}u }}"
             for word, source in plan["texcoord_patches"]],
        )
        lines += emit_rows(
            "NDSNativeFifoEpochPatch",
            f"sNdsNative{owner_title}FifoEpochPatches",
            [
                "{{ {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, "
                "{}u, {}u, {}u, {}u }}".format(*row)
                for row in plan["epoch_patches"]
            ],
        )
        lines += [
            f"static const NDSNativeFifoOwnerPlan "
            f"sNdsNative{owner_title}FifoPlan =",
            "{",
            f"    sNdsNative{owner_title}FifoWords,",
            f"    sNdsNative{owner_title}FifoMatrixPatches,",
            f"    sNdsNative{owner_title}FifoColorPatches,",
            f"    sNdsNative{owner_title}FifoTexcoordPatches,",
            f"    sNdsNative{owner_title}FifoEpochPatches,",
            f"    0x{plan['template_hash']:08x}u,",
            f"    {len(plan['words'])}u,",
            f"    {len(plan['matrix_patches'])}u,",
            f"    {len(plan['color_patches'])}u,",
            f"    {len(plan['texcoord_patches'])}u,",
            f"    {len(plan['epoch_patches'])}u,",
            f"    {plan['triangle_count']}u,",
            f"    {plan['raw_triangle_count']}u,",
            f"    {plan['cross_triangle_count']}u,",
            f"    {plan['run_count']}u,",
            f"    {plan['raw_run_count']}u,",
            f"    {plan['cross_run_count']}u,",
            f"    {plan['root_count']}u,",
            f"    {plan['epoch_count']}u,",
            f"    {plan['store_count']}u,",
            f"    {plan['restore_count']}u,",
            f"    {owner_slot}u,",
            "    { 0u, 0u, 0u },",
            "};",
            "",
        ]
    max_packet_words = max(len(plan["words"]) for plan in packet_plans)
    lines += [
        f"#define NDS_NATIVE_FIFO_MAX_WORDS {max_packet_words}u",
        "",
        "#endif",
        "",
    ]
    lines += ["#endif", ""]
    lines += emit_rows(
        "u16", "sNdsNativeFighterTriangles",
        [f"0x{value:04x}u" for value in triangles],
    )
    lines += emit_rows(
        "NDSNativeRun", "sNdsNativeFighterRuns",
        [f"{{ {first}u, {count}u, {submit_class}u, 0x{mask:08x}u }}"
         for first, count, submit_class, mask in runs],
    )
    lines += emit_rows(
        "NDSNativeEpoch", "sNdsNativeFighterEpochs",
        ["{{ {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u }}".format(*row)
         for row in epochs],
    )
    lines += [
        f"#define NDS_NATIVE_ROOT_LIGHT1 0x{light_preambles[1][0]:08x}u",
        f"#define NDS_NATIVE_ROOT_LIGHT2_1 0x{light_preambles[1][1]:08x}u",
        f"#define NDS_NATIVE_ROOT_LIGHT2_2 0x{light_preambles[2][1]:08x}u",
        "",
    ]
    root_format = (
        "{{ 0x{:08x}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u }}"
    )
    lines += emit_rows(
        "NDSNativeRoot", "sNdsNativeMarioRoots",
        [root_format.format(*row[:7], light_preamble)
         for row, light_preamble in zip(
             mario_roots, owner_light_preamble_indices["mario"])],
    )
    lines += emit_rows(
        "NDSNativeRoot", "sNdsNativeFoxRoots",
        [root_format.format(*row[:7], light_preamble)
         for row, light_preamble in zip(
             fox_roots, owner_light_preamble_indices["fox"])],
    )
    return "\n".join(lines)


def build_consumed_fields_manifest(repo_root: Path) -> dict[str, object]:
    context = build_owner_source_context(repo_root)
    data = context["data"]
    mario_roots = context["mario_roots"]
    fox_roots = context["fox_roots"]
    epochs = context["epochs"]
    runs = context["runs"]
    triangles = struct.unpack(f"<{len(data['triangles']) // 2}H", data["triangles"])
    mario_program = build_generated_mario_program(repo_root, context)
    closures = stage_manifest.build_consumed_closure_rows(
        repo_root, SOURCE_CLOSURE_POLICIES, {}
    )

    return {
        "schema": "smash64ds.m2-consumed-fields.v1",
        "generated_by": "scripts/generate_nds_native_owners.py",
        "allowed_classifications": list(FIELD_CLASSES),
        "source_closures": closures,
        "table_provenance": [
            {
                "owner": owner,
                "path": str(path).replace("\\", "/"),
                "resource_offset": f"0x{resource_offset:04x}",
                "sha256": sha256,
            }
            for owner, (path, resource_offset, sha256) in O2R_ASSETS.items()
        ],
        "compact_program": {
            "source_order": ["mario", "fox"],
            "roots": {
                "count": len(mario_roots) + len(fox_roots),
                "mario": len(mario_roots),
                "fox": len(fox_roots),
                "record_bytes": 16,
                "index_fields": ["first_epoch", "epoch_count"],
            },
            "epochs": {
                "count": len(epochs),
                "record_bytes": 16,
                "index_fields": ["first_run", "run_count", "material_slot"],
            },
            "runs": {
                "count": len(runs),
                "record_bytes": 8,
                "index_fields": ["first_triangle", "triangle_count", "submit_class"],
            },
            "triangles": len(triangles),
            "corners": len(triangles) * 3,
            "joint_schedule": {
                "count": sum(counts[0] for counts in OWNER_PLAN_COUNTS.values()),
                "field_bytes": 2,
                "packed_fields": [
                    "parent:5", "binding:5", "cross_palette_slot:5",
                    "push_before:1",
                ],
            },
            "binding_joints": {
                "count": sum(counts[1] for counts in OWNER_PLAN_COUNTS.values()),
                "field_bytes": 1,
                "consumer": "Task 27 generated fighter program",
                "lookup": "checked direct indices in exact source order",
            },
        },
        "generated_mario_program": {
            "schema": "smash64ds.task27-mario-program.v1",
            "status": "phase_a_retained_runtime_reverted",
            "source_order": {
                "roots": list(mario_program["root_order"]),
                "epochs": list(mario_program["epoch_order"]),
                "runs": list(mario_program["run_order"]),
            },
            "checksums": {
                "source": f"0x{mario_program['source_checksum']:08x}",
                "tables": f"0x{mario_program['table_checksum']:08x}",
                "events": f"0x{mario_program['event_checksum']:08x}",
            },
            "counts": {
                "joints": len(mario_program["schedule"]),
                "roots": len(mario_program["root_order"]),
                "epochs": len(mario_program["epoch_order"]),
                "runs": len(mario_program["run_order"]),
                "raw_runs": mario_program["raw_run_count"],
                "cross_runs": mario_program["cross_run_count"],
                "triangles": mario_program["triangle_count"],
                "corners": mario_program["triangle_count"] * 3,
                "root_prefix_light_commands":
                    mario_program["light_command_counts"][0],
                "intra_root_light_commands":
                    mario_program["light_command_counts"][1],
            },
            "root_program": [
                {
                    "root": row[0],
                    "root_offset": f"0x{row[1]:08x}",
                    "first_epoch": row[2],
                    "tail_state_first": row[3],
                    "source_command_count": row[4],
                    "epoch_count": row[5],
                    "tail_state_count": row[6],
                    "tail_sync_count": row[7],
                    "light_preamble": row[8],
                    "binding_joint": row[9],
                    "cross_palette_slot": row[10],
                }
                for row in mario_program["root_rows"]
            ],
            "epoch_program": [
                {
                    "root": row[0],
                    "epoch": row[1],
                    "direct_policy": row[2],
                    "before_state_first": row[3],
                    "after_state_first": row[4],
                    "first_action": row[5],
                    "first_run": row[6],
                    "before_state_count": row[7],
                    "after_state_count": row[8],
                    "before_sync_count": row[9],
                    "after_sync_count": row[10],
                    "action_count": row[11],
                    "run_count": row[12],
                    "material_slot": row[13],
                    "first_source_triangle": row[14],
                }
                for row in mario_program["epoch_rows"]
            ],
            "run_program": [
                {
                    "root": row[0],
                    "epoch": row[1],
                    "run": row[2],
                    "first_triangle": row[3],
                    "triangle_count": row[4],
                    "submit_class": row[5],
                    "required_vertex_mask": f"0x{row[6]:08x}",
                }
                for row in mario_program["run_rows"]
            ],
            "immutable_state_effects": list(mario_program["state_events"]),
            "live_operands": [
                "projection and camera modelview",
                "joint locals and parent/binding identity",
                "root preamble and material snapshots",
                "prepared light direction and shaded colors",
                "texture residency, parameters, palette, and alpha",
                "persistent vertex cache and cross-matrix palette slots",
            ],
            "validation": {
                "timing": "complete before the first GX mutation",
                "failure": "return to the caller before GX; no post-GX fallback",
                "field_coverage": "source_closures plus ownership_contracts",
            },
            "continuation_gate": {
                "measured_mario_p95_ceiling_ticks": 171520,
                "minimum_combined_fighter_p50_saving_ticks": 8000,
                "minimum_projected_both_fighters_saving_ticks": 35000,
            },
            "runtime_disposition": {
                "verdict": "REVERT_MARIO_STOP_BEFORE_FOX",
                "matrix_delta_p50_p95_ticks": [-3136, -3008],
                "mario_delta_p50_p95_ticks": [128, 128],
                "draw_delta_p50_p95_ticks": [2624, 2560],
                "reason": (
                    "the local matrix reduction was erased inside Mario and "
                    "regressed complete draw; the 8K continuation gate failed"
                ),
            },
        },
        "task21c_disposition": {
            "verdict": "REVERT_RUNTIME_KEEP_FOUNDATION",
            "retained": [
                "u16 joint schedules", "u8 binding-joint indices",
                "16-byte root/epoch records", "8-byte run records",
                "consumed-field and invalidation manifest",
            ],
            "reverted": (
                "the adapter direct-index binding consumer; its exact same-slot A/B "
                "reduced matrix ticks but regressed complete fighter draw and P95"
            ),
        },
        "prepared_record_census": {
            "record": "NDSNativeHierarchyPreparedRun",
            "record_bytes": 56,
            "records": len(epochs),
            "array_bytes": len(epochs) * 56,
            "line_bytes": 32,
            "array_line_equivalents": (len(epochs) * 56 + 31) // 32,
            "hot_fields": {
                "bytes_per_record": 40,
                "fields": [
                    "texture_entry", "texture_name", "texture_params", "poly_fmt",
                    "scale_s", "scale_t", "origin_s", "origin_t",
                    "texture_offset", "textured",
                ],
            },
            "cold_validation_fields": {
                "bytes_per_record": 12,
                "fields": ["texture_format", "texture_width", "texture_height"],
            },
            "unconsumed_fields": {
                "bytes_per_record": 4,
                "fields": ["vertex_flags"],
            },
            "task21b_disposition": (
                "record split and clear deletion require an independently positive "
                "exact A/B; retain the 56-byte layout when that gate is not met"
            ),
        },
        "ownership_contracts": [
            {
                "name": "root_selection",
                "classification": FIELD_CLASS_IMMUTABLE,
                "inputs": [
                    "generated root_offset/first_epoch/epoch_count",
                    "live selected display-list root_offset equality",
                ],
                "invalidation": "asset identity, generation, root order, or root offset",
            },
            {
                "name": "matrix_input",
                "classification": FIELD_CLASS_CAMERA,
                "inputs": [
                    "live camera projection/modelview", "live DObj local matrices",
                    "generated parent/binding schedule",
                ],
                "invalidation": "recompute every frame; never cache camera or DObj operands",
            },
            {
                "name": "material_progression",
                "classification": FIELD_CLASS_LIVE,
                "inputs": [
                    "generated epoch material_slot", "live material snapshots",
                    "before/after state spans",
                ],
                "invalidation": "any material, color, alpha, image, tile, or selector change",
            },
            {
                "name": "light_preambles",
                "classification": FIELD_CLASS_CAMERA,
                "inputs": [
                    "generated root light_preamble", "live source preamble",
                    "live light colors/direction", "live binding world matrix",
                ],
                "invalidation": "recompute prepared direction for every live owner frame",
            },
            {
                "name": "run_class_texture_alpha",
                "classification": FIELD_CLASS_LIVE,
                "inputs": [
                    "generated submit_class/required_mask", "live texture residency",
                    "live poly alpha/color/material state",
                ],
                "invalidation": "resident texture, palette, dimensions, poly format, or alpha change",
            },
            {
                "name": "vertex_cache_and_cross_matrix",
                "classification": FIELD_CLASS_LIVE,
                "inputs": [
                    "generated dense vertex/corner stream", "persistent owner vertex cache",
                    "generated logical binding and physical cross-palette slot",
                ],
                "invalidation": (
                    "preserve source-order cache ownership across all roots; reject before GX "
                    "on topology, binding, corner, or matrix mismatch"
                ),
            },
        ],
        "invalidation_manifest": [
            "asset data pointer, asset ID, size, owner generation, root offset, or material count changes",
            "joint parent/child/sibling order, XObj kind, selected display-list, or binding identity changes",
            "camera projection/modelview or any DObj local matrix changes: recompute every frame",
            "material, light, texture, alpha, color, geometry, or selector changes: rebuild live preparation",
            "generated root/epoch/run/schedule provenance or cardinality changes: regenerate and reverify",
            "any validation mismatch: fail before GX; no fallback after mutation",
        ],
        "task27_inputs": [
            "u16 parent/binding/cross-slot joint schedules",
            "u8 binding-joint direct indices",
            "16-byte root and epoch records",
            "8-byte run records",
            "root/epoch/run source order and complete consumed-field closure",
        ],
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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output", type=Path,
        default=Path(__file__).resolve().parents[1]
        / "src" / "nds" / "nds_native_fighter_owner.generated.inc",
    )
    parser.add_argument(
        "--source-root", type=Path,
        default=Path(__file__).resolve().parents[1],
        help="repo root containing the read-only BattleShip O2R inputs",
    )
    parser.add_argument(
        "--manifest-output", type=Path,
        default=Path(__file__).resolve().parents[1]
        / DEFAULT_CONSUMED_FIELDS_OUTPUT,
    )
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    source_root = args.source_root.resolve()
    manifest_output = args.manifest_output
    if not manifest_output.is_absolute():
        manifest_output = source_root / manifest_output
    generated = generate(source_root)
    rendered_manifest = render_consumed_fields_manifest(source_root)
    if args.check:
        if not args.output.is_file() or args.output.read_text() != generated:
            raise SystemExit(f"stale generated native-owner IR: {args.output}")
        if (not manifest_output.is_file() or
                manifest_output.read_bytes() != rendered_manifest):
            raise SystemExit(
                f"stale generated native-owner consumed fields: {manifest_output}"
            )
        return 0
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if not args.output.is_file() or args.output.read_text() != generated:
        args.output.write_text(generated)
    manifest_output.parent.mkdir(parents=True, exist_ok=True)
    if (not manifest_output.is_file() or
            manifest_output.read_bytes() != rendered_manifest):
        manifest_output.write_bytes(rendered_manifest)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
