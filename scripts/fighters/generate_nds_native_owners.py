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

import sys as _sys
from pathlib import Path as _Path

_scripts_root = _Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in _sys.path:
    _sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402  -- puts every scripts/ area folder on sys.path

# The single list of arrays that ship as a NitroFS image, shared with
# generate_nds_native_owner_images.py so the two cannot disagree.
from native_owner_image_arrays import NATIVE_OWNER_IMAGE_ARRAYS  # noqa: E402

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
        # `preamble` is tracked in its own right because the root now holds the
        # contract preamble BY REFERENCE: the read spells `input->preamble->
        # flags`, so the arrow scanner sees `input.preamble` and, separately,
        # `preamble.flags`. Without the base here the flags read would vanish
        # from the manifest and the falsifier would report a consumed field as
        # no longer read, which is the opposite of what happened.
        "tracked_bases": (
            "epoch", "execution", "hierarchy", "input", "m2_owner",
            "preamble", "prepared_epoch", "root", "scratch", "state", "stats",
            "tables",
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
                input.preamble preamble.flags prepared_epoch.light_direction_valid
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

# P2-3 grows the source decoder one owner at a time without perturbing the
# frozen Mario/Fox export above.  Keep O2R_ASSETS as the qualified P2-2 set --
# several historical checks deliberately iterate it -- and use this superset
# only for explicitly requested production-pipeline owners.
P2_O2R_ASSETS = {
    **O2R_ASSETS,
    "luigi": (
        Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/LuigiModel"),
        0x0143,
        "793c2f3ae89aa8925f4cd715b40a79b3fe9236c033d84a4e270f09bc88dd4247",
    ),
    "donkey": (
        Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/DonkeyModel"),
        0x013d,
        "bced84a9d8aa1a2c08ed9f87994bb06a836dfb8b7a797fec365dd68655f9e2f8",
    ),
    "captain": (
        Path("decomp/BattleShip-main/BattleShip_o2r"
             "/reloc_fighters_main/CaptainModel"),
        0x014c,
        "bbd56fc89524fc5a5de7d2cb88fdead3c231ad402b6039e1b63e4f1091c4669e",
    ),
    "samus": (
        Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/SamusModel"),
        0x0140,
        "67f9646c0a019704dcae5e3307df2cf8e4e7846339537a70a64f77d42284bbf5",
    ),
    "link": (
        Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/LinkModel"),
        0x0144,
        "93c9ee108c0e8f1680c35d8d11ec980891850cadcac5eed5bd731c43e85f163e",
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
    # decomp dLuigiModel_JointTree (323_LuigiModel.c:1032)
    "luigi": (0x2410, 26),
    # decomp dDonkeyModel_JointTree (317_DonkeyModel.c:1646)
    "donkey": (0x39a8, 27),
    # decomp dCaptainModel_JointTree (332_CaptainModel.c:1888)
    "captain": (0x3be0, 27),
    # decomp dSamusModel_JointTree (320_SamusModel.c:1427)
    "samus": (0x3520, 34),
    # decomp dLinkModel_JointTree (324_LinkModel.c:1159, US arm)
    "link": (0x3ae8, 33),
}

# The SECOND JointTree array in each hashed O2R resource is the low-detail
# model the source itself draws for 3+ fighter matches (scvsbattle.c:188:
# detail = (pl_count + cp_count < 3) ? High : Low).  Same descriptor counts,
# same depth-18 sentinels, same setup_parts selection -- only the display
# targets move deeper into the file.  Runtime proof: the four-CPU stress match
# resolves Mario roots at 0x3c78.. and Fox at 0x4720.., exactly these arrays.
OWNER_JOINT_TREES_LOW = {
    # decomp dMarioModel_JointTree_0x4590 (296_MarioModel.c:1951)
    "mario": (0x4590, 26),
    # decomp dFoxModel_JointTree_0x5510 (313_FoxModel.c:2078)
    "fox": (0x5510, 28),
    # decomp dLuigiModel_JointTree_0x49E8 (323_LuigiModel.c:2284)
    "luigi": (0x49e8, 26),
    # decomp dDonkeyModel_JointTree_0x6EC0 (317_DonkeyModel.c:3379)
    "donkey": (0x6ec0, 27),
    # decomp dCaptainModel_JointTree_0x7900 (332_CaptainModel.c:4071)
    "captain": (0x7900, 27),
    # decomp dSamusModel_JointTree_0x69D0 (320_SamusModel.c:3109)
    "samus": (0x69d0, 34),
    # decomp dLinkModel_JointTree_0x74B0 (324_LinkModel.c:2757, US arm)
    "link": (0x74b0, 33),
}

# Canonical export hashes for the low-detail program, pinned from the same
# pipeline that produced SOURCE_EXPORT_HASHES with only the JointTree offset
# changed.  Regeneration drift fails here exactly as the high set does.
LOW_SOURCE_EXPORT_HASHES = {
    "state":
        "08518517688c89c3afc003208e1575b7c7f1d9731946750d3e16830842d44de2",
    "sequence":
        "db432b3467a7c60f95e9b516f8227dfb9f3741b8f3668ef4886de51c5e86b65a",
    "vertex":
        "c15714bb3c9bfbd8129ed5ae2123ca5bde3c72b314f298c27e48f47f5bb09833",
    "triangles":
        "1ee27200df9a6ae38131be108be263c6fef9421b4768fc4d960e00762a2a57e5",
    "runs":
        "0ed2dd1bf4eae27a5d457073791a22b0004d2e37d50cdab4e65a7ea4eea5800c",
    "epochs":
        "edb010b7ae03c14a303792b235179d31e471ad0579f863c5c7218cc03d4b4801",
    "mario_roots":
        "4d052e3b488b9dc6306fdcde3bbb7ea7beb1d6177e1945efe47669622dcdf584",
    "fox_roots":
        "f433d638ae9c7f5a59e08ea83f7f6d7512681a7b81cf23216262cae32578279f",
}

# Canonical cardinalities per detail level, asserted in
# build_owner_source_context.  High is the historical frozen export; low is
# the same decoder over the low JointTrees.
DETAIL_EXPORT_CARDINALITIES = {
    "high": (54, 168, 76, 626, 67, 49, 14, 18),
    "low": (54, 171, 70, 393, 53, 50, 14, 18),
}
DETAIL_SUBMIT_CLASS_CENSUS = {
    "high": [582, 44],
    "low": [363, 30],
}
DETAIL_LIGHT_CENSUS = {
    "high": (120, 28),
    "low": (104, 24),
}

# Frozen low-detail direct policies.  derive_direct_epoch_policies()
# reproduces the frozen HIGH sets exactly (families from combine pairs,
# cull-none from the geometry word's bit 0x400 at runs time) before it is
# trusted; this is its low-detail output, pinned so regeneration cannot
# silently drift.
LOW_DIRECT_EPOCH_POLICIES: tuple[int, ...] = (
    0x00, 0x01, 0x01, 0x02, 0x00, 0x00, 0x01, 0x03, 0x83, 0x03,
    0x01, 0x01, 0x02, 0x01, 0x01, 0x02, 0x01, 0x01, 0x02, 0x01,
    0x00, 0x81, 0x81, 0x00, 0x01, 0x02, 0x01, 0x02, 0x03, 0x02,
    0x03, 0x00, 0x00, 0x02, 0x03, 0x03, 0x01, 0x02, 0x03, 0x02,
    0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x02, 0x00, 0x00,
)

# dMarioMain_setup_parts / dFoxMain_setup_parts, consumed MSB-first by
# BattleShip lbCommonSetupFighterPartsDObjs.
OWNER_SETUP_PARTS = {
    "mario": (0xffffff00, 0x00000000),
    "fox": (0xffffffc0, 0x00000000),
    # dLuigiMain_setup_parts (221_LuigiMain.c:87)
    "luigi": (0xffffff00, 0x00000000),
    # dDonkeyMain_setup_parts (213_DonkeyMain.c:103)
    "donkey": (0xffffff80, 0x00000000),
    # dCaptainMain_setup_parts (236_CaptainMain.c:103), the same mask
    "captain": (0xffffff80, 0x00000000),
    # dSamusMain_setup_parts (217_SamusMain.c:117). Unlike the earlier owners
    # this deliberately skips descriptors 13..21, so the decoder must follow
    # BattleShip's bit-walk rather than assuming one selected prefix.
    "samus": (0xfff803ff, 0x00000000),
    # dLinkMain_setup_parts (225_LinkMain.c:199). Link is another non-prefix
    # owner: descriptors 13/14 and 31 are omitted by the source bit walk.
    "link": (0xfff9fffe, 0x00000000),
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
    # Luigi's source JointTree has Mario's topology.  The independent Luigi
    # O2R decode finds the same eight logical bindings in cross-matrix runs;
    # keep the physical mapping identical so the DS hierarchy namespace stays
    # stable across the variant pair.
    "luigi": (
        (1, 16), (2, 17), (5, 18), (6, 19),
        (8, 20), (9, 21), (11, 22), (12, 23),
    ),
    # Derived from Donkey's own high/low O2R triangle binding sets. Both detail
    # levels use this exact set; slots 16..25 remain outside the hierarchy stack.
    "donkey": (
        (0, 16), (1, 17), (2, 18), (3, 19), (6, 20),
        (7, 21), (10, 22), (11, 23), (13, 24), (14, 25),
    ),
    # CAPTAIN FALCON HAS NONE, IN EITHER DETAIL, AND THAT IS THE MODEL'S OWN
    # ANSWER RATHER THAN A MISSING PIN.  His decode produces zero submit-class-1
    # runs: every triangle run draws under the current root's own matrix, so no
    # foreign binding is ever restored and no physical palette slot is claimed.
    # Donkey's ten slots (16..25) therefore remain the whole reserved band, and
    # admitting Falcon does not narrow the 26..30 headroom the parent-slot
    # allocator in ndsRendererAdapterBuildGxSlotTable scans downward from.
    "captain": (),
    # Samus also has no cross-matrix run in either source detail level.
    "samus": (),
    # Link HIGH uses three cross-matrix pairs. Keep the six logical binding IDs
    # source-stable and assign only the physical GX stores this detail needs.
    "link": (
        (2, 16), (3, 17), (6, 18), (7, 19), (11, 20), (12, 21),
    ),
}

# Every previously admitted owner uses the same cross-binding set in High and
# Low. Link is the first source model where that is not true: its Low JointTree
# retains only the 11/12 pair. Emit a smaller Low palette map instead of storing
# four matrices that no Low-detail triangle can restore.
OWNER_CROSS_BINDING_SLOTS_LOW = {
    "link": ((11, 16), (12, 17)),
}


def owner_cross_binding_slots(owner_name: str, detail: str):
    if detail == "low" and owner_name in OWNER_CROSS_BINDING_SLOTS_LOW:
        return OWNER_CROSS_BINDING_SLOTS_LOW[owner_name]
    return OWNER_CROSS_BINDING_SLOTS[owner_name]

OWNER_PLAN_COUNTS = {
    "mario": (25, 14),
    "fox": (27, 18),
    "luigi": (25, 14),
    "donkey": (26, 16),
    # 25 selected joints + the synthetic TopN, 17 drawable roots; identical in
    # both details.
    "captain": (26, 17),
    # 23 source-selected parts + the synthetic TopN, 14 drawable roots.
    "samus": (24, 14),
    # 29 source-selected parts + synthetic TopN, 19 drawable roots.
    "link": (30, 19),
}

# camera seeds, hierarchy pushes, hierarchy pops, cross-binding stores, and
# per-corner/current-root restores in the exact flattened owner packet.
OWNER_GX_PLAN_COUNTS = {
    "mario": (1, 5, 5, 8, 70),
    "fox": (1, 6, 6, 2, 14),
    "luigi": (1, 5, 5, 8, 70),
    "donkey": (1, 6, 6, 10, 80),
    # Zero cross-binding stores means zero per-corner restores; see
    # OWNER_CROSS_BINDING_SLOTS above.
    "captain": (1, 6, 6, 0, 0),
    "samus": (1, 5, 5, 0, 0),
    "link": (1, 8, 8, 6, 44),
}

# The low-detail program shares the high skeleton (same pushes/pops/stores);
# only the per-corner restore count tracks the smaller corner population.
DETAIL_GX_PLAN_COUNTS = {
    "high": OWNER_GX_PLAN_COUNTS,
    "low": {
        "mario": (1, 5, 5, 8, 46),
        "fox": (1, 6, 6, 2, 10),
        "luigi": (1, 5, 5, 8, 46),
        "donkey": (1, 6, 6, 10, 74),
        "captain": (1, 6, 6, 0, 0),
        "samus": (1, 5, 5, 0, 0),
        "link": (1, 8, 8, 2, 6),
    },
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

# Source RDP/RSP control opcode -> NDS_NATIVE_STATE_* effect kind, exactly as
# `ndsRendererNativeApplyStateDelta` switches on it.
#
# 0xe2 (G_SETOTHERMODE_L) and 0xf9 (G_SETBLENDCOLOR) were added for Captain
# Falcon (P2-3f5) -- his HIGH-detail model is the first of any owner to contain
# either, and a census over Mario/Fox/Luigi/Donkey in both details is 0/0 for
# both opcodes, so adding them cannot move a landed owner's export.
#
# 0xe2 shares effect 2 (NDS_NATIVE_STATE_OTHERMODE) with 0xe3 rather than
# needing a kind of its own: the runtime applier passes `delta->w0 >> 24` to
# `ndsRendererRecordOtherMode`, which already dispatches on the opcode byte and
# already accumulates othermode_L through the same shift/len bitfield decode the
# source uses (`sm64-nds` src/nds/nds_renderer.c:725 `g_setothermode_l` is the
# same four lines). Nothing about the delta row needed to change.
#
# 0xf9 becomes effect 12 (NDS_NATIVE_STATE_BLEND), which already existed for the
# native STAGE program; this row is what gives the FIGHTER program the same
# case. The DS answer for the pair is the hardware alpha test: with
# othermode_L's G_AC_THRESHOLD bits set, `ndsRendererHardwareApplyAlphaTest`
# emits `glEnable(GL_ALPHA_TEST)` + `glAlphaFunc(blend_color.a >> 4)`, so
# Falcon's blend colour of (0,0,0,0) becomes a reference of 0 -- pass when
# alpha > 0, the plain binary cutout the source asks for. sm64-nds discards
# G_SETBLENDCOLOR outright (`case G_SETBLENDCOLOR: break;`, :1017); this port
# keeps it because it is the alpha reference the threshold compares against.
SOURCE_STATE_EFFECTS = {
    0xe3: 2, 0xe2: 2, 0xfc: 3, 0xd7: 4, 0xd9: 5, 0xfd: 6,
    0xf5: 7, 0xf0: 8, 0xf3: 9, 0xf2: 10, 0xfa: 11, 0xf9: 12,
}
SOURCE_SYNC_OPS = frozenset((0xe6, 0xe7, 0xe8))
SOURCE_TRIANGLE_OPS = frozenset((0x05, 0x06))
SOURCE_ACTION_OPS = frozenset((0x01, 0x02))
SOURCE_MATERIAL_DL = 0xde
SOURCE_END_DL = 0xdf


def _owner_selected_descriptor_indices(owner_name: str,
                                       descriptor_count: int) -> list[int]:
    """Return BattleShip's exact setup_parts selection in source order."""
    if descriptor_count > 64:
        raise ValueError(
            f"{owner_name} JointTree has {descriptor_count} raw descriptors; "
            "setup_parts carries only 64 bits"
        )
    setup_words = OWNER_SETUP_PARTS[owner_name]
    return [
        index for index in range(descriptor_count)
        if setup_words[index // 32] & (1 << (31 - (index & 31)))
    ]


def _owner_joint_descriptors(payload: bytes, owner_name: str,
                             detail: str = "high") -> list[tuple[int, int | None]]:
    """Decode one JointTree with BattleShip's Low-detail DL fallback.

    lbCommonSetupFighterPartsDObjs always takes id/transform from the selected
    detail's descriptor. For Low detail only, a NULL Low display list selects
    the corresponding High display list instead. Keep that source rule here so
    root discovery and topology cannot disagree about which DObj is drawable.
    """
    joint_tree_offset, descriptor_count = (
        OWNER_JOINT_TREES[owner_name] if detail == "high"
        else OWNER_JOINT_TREES_LOW[owner_name]
    )
    if joint_tree_offset + descriptor_count * DOBJ_DESC_SIZE > len(payload):
        raise ValueError(f"{owner_name} JointTree is out of range")
    high_tree_offset, high_descriptor_count = OWNER_JOINT_TREES[owner_name]
    if high_descriptor_count != descriptor_count:
        raise ValueError(
            f"{owner_name} High/Low JointTree cardinality changed: "
            f"{high_descriptor_count}/{descriptor_count}"
        )
    descriptors = []
    for descriptor_index in range(descriptor_count):
        offset = joint_tree_offset + descriptor_index * DOBJ_DESC_SIZE
        depth, reloc_pointer = struct.unpack_from(">II", payload, offset)
        display_offset = None if reloc_pointer == 0 else \
            (reloc_pointer & 0xffff) * 4
        if (detail == "low") and (display_offset is None):
            high_offset = high_tree_offset + descriptor_index * DOBJ_DESC_SIZE
            _high_depth, high_reloc_pointer = struct.unpack_from(
                ">II", payload, high_offset
            )
            if high_reloc_pointer != 0:
                display_offset = (high_reloc_pointer & 0xffff) * 4
        if display_offset is not None and display_offset >= len(payload):
            raise ValueError(
                f"{owner_name} JointTree entry {descriptor_index}: "
                f"display target 0x{display_offset:x} is out of range"
            )
        descriptors.append((depth, display_offset))
    if descriptors[-1] != (18, None):
        raise ValueError(f"{owner_name} JointTree lost its depth-18 sentinel")
    return descriptors


def _discover_owner_roots(payload: bytes, owner_name: str,
                          detail: str = "high") -> list[int]:
    descriptors = _owner_joint_descriptors(payload, owner_name, detail)[:-1]
    selected = _owner_selected_descriptor_indices(owner_name, len(descriptors))
    roots = [
        descriptors[index][1] for index in selected
        if descriptors[index][1] is not None
    ]
    if (owner_name in OWNER_PLAN_COUNTS and
            len(roots) != OWNER_PLAN_COUNTS[owner_name][1]):
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
        # The compact runtime span stores count in five bits, so 1..31 are
        # directly representable.  BattleShip's Samus passive model-part DLs
        # use a legal full-cache gSPVertex count of 32.  Split only that source
        # command into two contiguous internal loads; both retain the original
        # command index, and together they write exactly the same cache slots
        # from exactly the same 32 source vertices. Canonical roots currently
        # never take this arm, so their frozen action stream remains unchanged.
        if count == VERTEX_CACHE_SIZE:
            first_count = VERTEX_CACHE_SIZE // 2
            second_count = VERTEX_CACHE_SIZE - first_count
            actions.append((
                0, command_index, index, first_count, source_offset, 0, 0
            ))
            actions.append((
                0, command_index, index + first_count, second_count,
                source_offset + first_count * SOURCE_VERTEX_SIZE, 0, 0
            ))
        else:
            actions.append((
                0, command_index, index, count, source_offset, 0, 0
            ))
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


def _build_source_export_for_owners(
        repo_root: Path, owner_names: tuple[str, ...], detail: str,
        root_specs_by_owner: dict[str, tuple[tuple[int, int], ...]] | None = None,
        ) -> dict[str, bytes]:
    """Decode one ordered owner set into the shared source-order IR.

    Owner order is part of the ABI: actions/runs/epochs append in that order.
    P2-2 calls this with (Mario, Fox), preserving the frozen export byte for
    byte. P2-3 can ask for a new owner independently, or append one after the
    frozen prefix, without teaching the decoder another fighter-specific path.
    """
    states, sequence, actions, triangles, runs, epochs = [], [], [], [], [], []
    state_lookup = {}
    owner_roots = {}
    for owner_name in owner_names:
        payload = load_o2r_payload(repo_root, owner_name)
        roots = []
        slots = [None] * VERTEX_CACHE_SIZE
        if root_specs_by_owner is not None and owner_name in root_specs_by_owner:
            root_specs = root_specs_by_owner[owner_name]
        else:
            root_specs = tuple(
                (root_offset, root_index)
                for root_index, root_offset in enumerate(
                    _discover_owner_roots(payload, owner_name, detail)
                )
            )
        for root_index, (root_offset, logical_binding) in enumerate(root_specs):
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
                        payload, owner_name, logical_binding, index,
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
                        if None in bindings or logical_binding not in bindings:
                            raise ValueError(
                                f"{owner_name} root {root_index} command {index}: "
                                "triangle uses an invalid binding"
                            )
                        submit_class = 0 if bindings == {logical_binding} else 1
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
    }
    for owner_name in owner_names:
        data[f"{owner_name}_roots"] = _pack_rows(
            "<IHHHBBBB2x", owner_roots[owner_name]
        )
    return data


def build_source_export(repo_root: Path, detail: str = "high"
                        ) -> dict[str, bytes]:
    """Rebuild the frozen Mario/Fox export directly from BattleShip O2R."""
    data = _build_source_export_for_owners(
        repo_root, ("mario", "fox"), detail
    )
    hashes = (SOURCE_EXPORT_HASHES if detail == "high"
              else LOW_SOURCE_EXPORT_HASHES)
    for name, expected_hash in hashes.items():
        actual_hash = hashlib.sha256(data[name]).hexdigest()
        if actual_hash != expected_hash:
            raise ValueError(
                f"{detail} {name}: SHA256 {actual_hash} != {expected_hash}")
    return data


def build_p2_owner_source_export(
        repo_root: Path, owner_name: str, detail: str = "high"
        ) -> dict[str, bytes]:
    """Decode a P2-3 owner without changing the frozen Mario/Fox program."""
    if owner_name not in P2_O2R_ASSETS:
        raise ValueError(f"unknown P2 native owner {owner_name}")
    return _build_source_export_for_owners(repo_root, (owner_name,), detail)


# P2-3 source-derived admission census.  These pins are intentionally at the
# *output* of the generic decoder: they protect the production pipeline from a
# reloc/source-layout drift while leaving Mario/Fox's older byte-hash oracle
# untouched.  The fields are source IR + DS topology facts, not hand-authored
# gameplay parameters.
P2_OWNER_MODEL_CENSUS = {
    "luigi": {
        "high": (23, 56, 42, 320, 32, 20, 14, 264, 960, 9, 44, 8, 70),
        "low": (25, 61, 32, 200, 20, 20, 14, 181, 600, 4, 44, 8, 46),
    },
    "donkey": {
        "high": (60, 259, 57, 318, 62, 34, 16, 315, 954, 22, 64, 0, 80),
        "low": (50, 202, 44, 200, 42, 25, 16, 201, 600, 14, 64, 0, 74),
    },
    "captain": {
        "high": (87, 254, 34, 319, 34, 34, 17, 291, 957, 0, 68, 6, 0),
        "low": (73, 223, 30, 200, 30, 30, 17, 205, 600, 0, 68, 4, 0),
    },
    "samus": {
        "high": (48, 209, 30, 322, 26, 26, 14, 294, 966, 0, 56, 8, 0),
        "low": (48, 203, 23, 199, 23, 23, 14, 174, 597, 0, 56, 0, 0),
    },
    "link": {
        "high": (86, 353, 69, 338, 61, 52, 19, 420, 1014, 13, 52, 32, 44),
        "low": (83, 351, 55, 217, 47, 47, 19, 335, 651, 1, 52, 32, 6),
    },
}

# Admission order is the native-owner slot ABI after frozen Mario/Fox. Keep the
# build flag beside the owner name so generation and runtime selection cannot
# silently disagree about which independent table set owns a slot.
P2_RUNTIME_OWNERS = (
    ("luigi", "NDS_P2_LUIGI"),
    ("donkey", "NDS_P2_DONKEY"),
    ("captain", "NDS_P2_CAPTAIN"),
    ("samus", "NDS_P2_SAMUS"),
    ("link", "NDS_P2_LINK"),
)

# The frozen Mario/Fox owner predates the P2 per-fighter variant machinery,
# but BattleShip's Results Lose motion makes Fox exercise the same source
# contract.  scsubsysdatafox.c:D_ovl1_80391140 executes
# SetModelPartID(16, 1) then SetModelPartID(10, 1).  Those two joints are
# canonical Fox bindings 9 and 4 respectively, and 209_FoxMain.c maps
# model-part 1 to these exact high/low DLs.  Decode the replacement programs at
# the SAME logical binding; never alias them to the canonical geometry.
BASE_MODEL_PART_ROOT_VARIANTS = {
    "fox": {
        "high": (
            (4, 0x6030),  # joint 10 model-part 1
            (9, 0x5dd0),  # joint 16 model-part 1
        ),
        "low": (
            (4, 0x6140),  # joint 10 model-part 1
            (9, 0x5ee0),  # joint 16 model-part 1
        ),
    },
}

# BattleShip passive model-part DLs that replace an already-selected JointTree
# root.  These are not optional cosmetic overlays: ftParamSetModelPartID writes
# the selected FTModelPart::dl directly into the live DObj, so the native owner
# must execute the replacement program at the SAME logical matrix binding.
#
# Source: relocData/213_DonkeyMain.c modelparts_desc_0x058/0x0A8/0x120,
# relocData/236_CaptainMain.c modelparts_desc_0x05C/0x0D4, and
# relocData/217_SamusMain.c modelparts_desc_0x120/0x198.  The O2R hashes above
# pin the backing bytes.  Each listed alternate is independently decoded below
# and is required to be a self-contained RAW root before it can be emitted.
#
# Samus also has passive model-part descriptors on joints omitted by her
# setup_parts mask (notably the 0x2c20/0x8a70 pair).  Those alter live topology,
# not merely one admitted root, so they intentionally stay out of this table;
# they need a topology-owner solution rather than pretending to be a variant of
# a canonical binding.
P2_MODEL_PART_ROOT_VARIANTS = {
    "donkey": {
        "high": (
            (4, 0x7f38),
            (5, 0x8a58),
            (5, 0xa1b8),
            (8, 0x79e8),
        ),
        "low": (
            (4, 0x81d8),
            (5, 0x95b8),
            (5, 0xad08),
            (7, 0x7c88),
        ),
    },
    "captain": {
        "high": (
            (4, 0x94a8),
            (4, 0x9b88),
            (9, 0x86f8),
            (9, 0x8dd8),
        ),
        "low": (
            (4, 0x9800),
            (4, 0x9ed0),
            (9, 0x8a50),
            (9, 0x9120),
        ),
    },
    "samus": {
        "high": (
            (1, 0x8158),
            (1, 0x8708),
            (4, 0x7770),
        ),
        "low": (
            (1, 0x8158),
            (1, 0x8708),
            (4, 0x7bc0),
        ),
    },
}


def build_p2_owner_model_inventory(
        repo_root: Path, owner_name: str) -> dict[str, object]:
    """Return a compact, adversarially checked native-model inventory.

    This is the seam P2-3 needs before emitting a runtime owner.  It proves the
    exact BattleShip high/low JointTrees, display-list IR, hierarchy, cross-
    matrix bindings, light commands and dense DS geometry using the same
    decoder that produced the shipping Mario/Fox owner.  It deliberately does
    not mutate or append to the frozen P2-2 generated program.
    """
    if owner_name not in P2_OWNER_MODEL_CENSUS:
        raise ValueError(f"no P2 native-owner census for {owner_name}")

    relative_path, file_id, expected_hash = P2_O2R_ASSETS[owner_name]
    source_path = repo_root / relative_path
    if hashlib.sha256(source_path.read_bytes()).hexdigest() != expected_hash:
        raise ValueError(f"{owner_name} model O2R changed before inventory")

    details: dict[str, object] = {}
    for detail in ("high", "low"):
        data = build_p2_owner_source_export(repo_root, owner_name, detail)
        state = unpack_many("<IIB3x", data["state"])
        sequence = list(data["sequence"])
        vertex = unpack_many("<BBBBIhh", data["vertex"])
        triangles = [
            item[0] for item in unpack_many("<H", data["triangles"])
        ]
        runs = unpack_many("<HBBI", data["runs"])
        epochs = unpack_many("<HHHHBBBBBBBB", data["epochs"])
        roots = unpack_many(
            "<IHHHBBBB2x", data[f"{owner_name}_roots"]
        )
        payload = load_o2r_payload(repo_root, owner_name)
        topology = decode_joint_topology(
            payload, owner_name, roots, detail
        )
        (joint_schedule, binding_parents, binding_joints,
         cross_slots, hierarchy_counts) = topology
        (_light_state, light_preambles,
         root_prefix_light_count, intra_root_light_count) = \
            decode_epoch_light_color_state(
                payload, owner_name, roots, epochs
            )
        direct_policies = derive_direct_epoch_policies(
            state, sequence, epochs, ((owner_name, roots),),
            expected_policies=None,
        )
        (dense_vertices, dense_color_sources, _dense_owners,
         dense_corners, action_dense_first, run_first_corner,
         run_owners, run_root_bindings,
         run_binding_sets) = build_dense_geometry(
            vertex, triangles, runs, epochs,
            ((owner_name, roots),), repo_root
        )

        # This call is a validator as much as a builder: it proves every cross
        # binding is assigned a legal physical GX slot and independently counts
        # the required restores against DETAIL_GX_PLAN_COUNTS.
        build_direct_dense_tables(
            vertex, runs, dense_vertices, dense_color_sources,
            dense_corners, action_dense_first, run_first_corner,
            run_owners, run_root_bindings, run_binding_sets,
            [cross_slots], detail, (owner_name,),
        )

        observed_cross_bindings: set[int] = set()
        cross_run_count = 0
        for run_index, run in enumerate(runs):
            if run[2] == 1:
                cross_run_count += 1
                observed_cross_bindings.update(run_binding_sets[run_index])
        expected_cross_bindings = {
            binding for binding, _slot
            in owner_cross_binding_slots(owner_name, detail)
        }
        if observed_cross_bindings != expected_cross_bindings:
            raise ValueError(
                f"{owner_name} {detail} cross bindings "
                f"{sorted(observed_cross_bindings)} != "
                f"{sorted(expected_cross_bindings)}"
            )

        restore_count = DETAIL_GX_PLAN_COUNTS[detail][owner_name][4]
        census = (
            len(state), len(sequence), len(vertex), len(triangles),
            len(runs), len(epochs), len(roots), len(dense_vertices),
            len(dense_corners), cross_run_count,
            root_prefix_light_count, intra_root_light_count, restore_count,
        )
        if census != P2_OWNER_MODEL_CENSUS[owner_name][detail]:
            raise ValueError(
                f"{owner_name} {detail} native-model census {census} != "
                f"{P2_OWNER_MODEL_CENSUS[owner_name][detail]}"
            )

        unique_light_preambles = sorted({
            (int(w0), int(w1)) for preamble in light_preambles
            if preamble is not None for w0, w1 in (preamble,)
        })
        details[detail] = {
            "source_ir": {
                "state_deltas": len(state),
                "state_sequence": len(sequence),
                "vertex_actions": len(vertex),
                "triangles": len(triangles),
                "runs": len(runs),
                "epochs": len(epochs),
                "roots": len(roots),
                "roots_sha256": hashlib.sha256(
                    data[f"{owner_name}_roots"]
                ).hexdigest(),
            },
            "dense_geometry": {
                "vertices": len(dense_vertices),
                "corners": len(dense_corners),
            },
            "root_offsets": [f"0x{root[0]:x}" for root in roots],
            "hierarchy": {
                "joints": len(joint_schedule),
                "bindings": len(binding_joints),
                "camera_seeds": hierarchy_counts[0],
                "pushes": hierarchy_counts[1],
                "pops": hierarchy_counts[2],
                "max_source_depth": hierarchy_counts[3],
                "binding_parents": list(binding_parents),
                "binding_joints": list(binding_joints),
            },
            "cross_matrix": {
                "runs": cross_run_count,
                "bindings": sorted(observed_cross_bindings),
                "physical_slots": [
                    [binding, slot]
                    for binding, slot in owner_cross_binding_slots(owner_name, detail)
                ],
                "stores": DETAIL_GX_PLAN_COUNTS[detail][owner_name][3],
                "restores": restore_count,
            },
            "lights": {
                "root_prefix_commands": root_prefix_light_count,
                "intra_root_commands": intra_root_light_count,
                "preambles": [
                    [f"0x{w0:08x}", f"0x{w1:08x}"]
                    for w0, w1 in unique_light_preambles
                ],
            },
            "direct_epoch_policies": [
                f"0x{value:02x}" for value in direct_policies
            ],
        }

    return {
        "owner": owner_name,
        "model_file_id": f"0x{file_id:x}",
        "model_path": str(relative_path).replace("\\", "/"),
        "model_sha256": expected_hash,
        "model_bytes": source_path.stat().st_size,
        "setup_parts": [f"0x{value:08x}" for value in OWNER_SETUP_PARTS[owner_name]],
        "details": details,
    }


def decode_export(repo_root: Path | None = None) -> dict[str, bytes]:
    """Compatibility entry point for the existing generator checkers."""
    if repo_root is None:
        repo_root = _paths.REPO_ROOT
    return build_source_export(Path(repo_root).resolve())


def unpack_many(fmt: str, payload: bytes):
    size = struct.calcsize(fmt)
    if len(payload) % size:
        raise ValueError(f"{len(payload)} bytes is not a multiple of {size}")
    return [item for item in struct.iter_unpack(fmt, payload)]


def load_o2r_payload(repo_root: Path, owner_name: str) -> bytes:
    relative_path, expected_file_id, expected_hash = P2_O2R_ASSETS[owner_name]
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
        state, sequence, epochs, root_groups, additions, detail: str = "high",
        expected_light_additions: int | None = None):
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
    if expected_light_additions is None:
        expected_light_additions = DETAIL_LIGHT_CENSUS[detail][1]
    if len(rebuilt_sequence) != len(sequence) + expected_light_additions:
        raise ValueError(
            "recovered light state sequence changed size: "
            f"{len(rebuilt_sequence)} != "
            f"{len(sequence) + expected_light_additions}"
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


def build_direct_epoch_policies(epoch_count: int, detail: str = "high"
                                ) -> list[int]:
    if detail == "high":
        if epoch_count != 49:
            raise ValueError(
                f"direct policy expects 49 epochs, got {epoch_count}")
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

    if epoch_count != len(LOW_DIRECT_EPOCH_POLICIES):
        raise ValueError(
            f"low direct policy expects {len(LOW_DIRECT_EPOCH_POLICIES)} "
            f"epochs, got {epoch_count}")
    return list(LOW_DIRECT_EPOCH_POLICIES)


def derive_direct_epoch_policies(state, sequence, epochs, owner_roots,
                                 expected_policies=None):
    """Derive per-epoch direct-policy families by replaying state spans.

    The frozen high-detail sets classify specific epoch indices; the low
    program needs the same classification for different indices.  Both
    properties are functions of the state an epoch's runs see, which the
    runtime builds as before-span -> material -> after-span -> runs with
    carryover across epochs: the family from the last G_SETCOMBINE delta
    (effect 3), the cull-none flag from the geometry word (effect 5) having
    bit 0x400 clear at runs time -- that is exactly how the frozen high sets
    behave (epoch 20 clears the bit in its after-span, epoch 21 inherits it
    through empty spans, epoch 22 restores it).  Root tail spans are replayed
    too, because the runtime applies `root->tail_state_*` after the last epoch
    and before the next display list. Donkey makes that ordering observable:
    root 0x30d8 clears culling for its triangle at command 21 and restores it
    in the tail at command 26, so carrying the pre-tail no-cull state into root
    0x31c0 rejects a source-correct live preamble. Pass expected_policies to
    demand an exact match against a known-good table; the high context must
    reproduce its frozen table before the low output is trusted.
    """
    combine_pairs = {
        (combine_w0, combine_w1): family
        for family, (combine_w0, combine_w1, _flags, _textured) in enumerate(
            DIRECT_POLICY_FAMILIES)
    }
    result = [None] * len(epochs)
    for _owner_name, roots in owner_roots:
        combine = None
        # The retained fighter IR carries the F3DEX_GBI_2 geometry word, where
        # G_CULL_BACK is 0x400.  A G_GEOMETRYMODE command is a mask/or update,
        # not a full replacement: Link makes this observable by setting
        # G_TEXTURE_GEN (0x40000) while preserving backface culling.  Tracking
        # only w1 falsely classified every following epoch as CULL_NONE.
        cull_mode = 0x400
        for root in roots:
            (_offset, first_epoch, tail_first, _commands, epoch_count,
             tail_count) = root[:6]
            for epoch_index in range(first_epoch, first_epoch + epoch_count):
                epoch = epochs[epoch_index]
                (before_first, after_first, _first_action, _first_run,
                 before_count, after_count) = epoch[:6]
                for first, count in ((before_first, before_count),
                                     (after_first, after_count)):
                    for i in range(count):
                        delta_index = sequence[first + i]
                        w0, w1, effect = state[delta_index]
                        if effect == 3:  # NDS_NATIVE_STATE_COMBINE
                            combine = (w0, w1)
                        elif effect == 5:  # NDS_NATIVE_STATE_GEOMETRY
                            cull_mode = ((cull_mode & w0) | w1) & 0x400
                if combine is None or combine not in combine_pairs:
                    raise ValueError(
                        f"epoch {epoch_index}: effective combine "
                        f"{combine} matches no direct-policy family")
                family = combine_pairs[combine]
                if cull_mode == 0:
                    family |= DIRECT_POLICY_CULL_NONE
                result[epoch_index] = family
            if tail_count:
                for i in range(tail_count):
                    delta_index = sequence[tail_first + i]
                    w0, w1, effect = state[delta_index]
                    if effect == 3:  # NDS_NATIVE_STATE_COMBINE
                        combine = (w0, w1)
                    elif effect == 5:  # NDS_NATIVE_STATE_GEOMETRY
                        cull_mode = ((cull_mode & w0) | w1) & 0x400
    if any(family is None for family in result):
        raise ValueError("an epoch was never reached by the root walk")
    if expected_policies is not None and result != list(expected_policies):
        for index, (derived, expected) in enumerate(
                zip(result, expected_policies)):
            if derived != expected:
                print(f"epoch {index}: derived 0x{derived:02x} "
                      f"expected 0x{expected:02x}")
        raise ValueError("derived direct policies disagree with the "
                         "expected table")
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
        payload: bytes, owner_name: str, roots: list[tuple],
        detail: str = "high"):
    descriptors = _owner_joint_descriptors(payload, owner_name, detail)
    descriptors = descriptors[:-1]
    raw_descriptor_count = len(descriptors)
    if (raw_descriptor_count == 0) or (descriptors[0][0] != 0):
        raise ValueError(f"{owner_name} JointTree topology cardinality changed")

    selected_indices = _owner_selected_descriptor_indices(
        owner_name, raw_descriptor_count
    )

    root_offsets = [root[0] for root in roots]
    selected_display_offsets = [
        descriptors[index][1] for index in selected_indices
        if descriptors[index][1] is not None
    ]
    if selected_display_offsets != root_offsets:
        raise ValueError(
            f"{owner_name} JointTree display preorder does not match "
            "canonical native roots"
        )
    root_by_offset = {
        offset: binding for binding, offset in enumerate(root_offsets)
    }

    # Mirror lbCommonSetupFighterPartsDObjs exactly: source array_dobjs[id]
    # retains the most recently CREATED DObj at each source id/depth. An
    # unselected descriptor advances the source walk but does not replace that
    # parent. Samus is the first landed owner whose mask exercises this.
    parents = [INVALID_U8]
    bindings = [INVALID_U8]
    active_by_depth: list[int | None] = [None] * 18
    for descriptor_index in selected_indices:
        depth, display_offset = descriptors[descriptor_index]
        if depth >= 18:
            raise ValueError(
                f"{owner_name} descriptor {descriptor_index}: "
                f"invalid live depth {depth}"
            )
        if depth == 0:
            parent = 0
        else:
            parent = active_by_depth[depth - 1]
            if parent is None:
                raise ValueError(
                    f"{owner_name} descriptor {descriptor_index}: missing "
                    f"selected parent at "
                    f"depth {depth - 1}"
                )
        joint_index = len(parents)
        parents.append(parent)
        bindings.append(
            INVALID_U8 if display_offset is None else
            root_by_offset[display_offset]
        )
        active_by_depth[depth] = joint_index

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
    for binding, palette_slot in owner_cross_binding_slots(owner_name, detail):
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

    expected_store_count = DETAIL_GX_PLAN_COUNTS[detail][owner_name][3]
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
        vertex, triangles, runs, epochs, owners, repo_root: Path | None = None,
        owner_root_bindings=None):
    if repo_root is None:
        repo_root = _paths.REPO_ROOT
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
            root_binding = (
                owner_root_bindings[owner_index][root_ordinal]
                if owner_root_bindings is not None else root_ordinal
            )
            if root_binding >= PACKED_GX_SLOT_CURRENT:
                raise ValueError(
                    f"{owner_name} root {root_ordinal}: logical binding "
                    f"{root_binding} is out of range"
                )
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
                                    root_binding,
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
                        if bindings != {root_binding}:
                            raise ValueError(
                                f"raw run {run_index}: bindings {sorted(bindings)} "
                                f"do not match root binding {root_binding}"
                            )
                    elif submit_class == 1:
                        if root_binding not in bindings or len(bindings) < 2:
                            raise ValueError(
                                f"cross run {run_index}: bindings "
                                f"{sorted(bindings)} do not preserve root binding "
                                f"{root_binding} crossing"
                            )
                    else:
                        raise ValueError(
                            f"run {run_index}: unsupported submit class {submit_class}"
                        )
                    run_owners.append(owner_index)
                    run_root_bindings.append(root_binding)
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
        run_root_bindings, run_binding_sets, owner_cross_slots,
        detail: str = "high", owner_names: tuple[str, ...] | None = None,
        validate_cross_census: bool = True):
    if owner_names is None:
        # P2-2's frozen program is Mario/Fox.  P2-3 callers pass their explicit
        # ordered owner set so adding a fighter cannot silently change this
        # validation universe.
        owner_names = tuple(O2R_ASSETS)
    if len(owner_names) != len(owner_cross_slots):
        raise ValueError(
            "direct-table owner names do not match cross-slot table count"
        )
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
    gx_plan_counts = DETAIL_GX_PLAN_COUNTS[detail]
    for owner_index, owner_name in enumerate(owner_names):
        if validate_cross_census:
            expected = {
                binding
                for binding, _ in owner_cross_binding_slots(owner_name, detail)
            }
            if observed_cross_bindings[owner_index] != expected:
                raise ValueError(
                    f"{owner_name} cross-binding census changed: "
                    f"{sorted(observed_cross_bindings[owner_index])}"
                )
            expected_restore_count = gx_plan_counts[owner_name][4]
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
        detail: str = "high",
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

    expected_store = DETAIL_GX_PLAN_COUNTS[detail][owner_name][3]
    expected_restore = DETAIL_GX_PLAN_COUNTS[detail][owner_name][4]
    if (store_count, restore_count) != (expected_store, expected_restore):
        raise ValueError(
            f"{owner_name} packet store/restore {store_count}/{restore_count} "
            f"!= {expected_store}/{expected_restore}"
        )
    expected_triangles = {
        ("high", "mario"): 320, ("high", "fox"): 306,
        ("low", "mario"): 200, ("low", "fox"): 193,
    }[(detail, owner_name)]
    expected_corners = expected_triangles * 3
    expected_textured_corners = {
        ("high", "mario"): 192, ("high", "fox"): 189,
        ("low", "mario"): 150, ("low", "fox"): 111,
    }[(detail, owner_name)]
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


def _strip_extend_active_edge(active_edge, tri):
    """Can `tri` extend a GL_TRIANGLE_STRIP whose active edge is `active_edge`?

    The DS geometry engine emits strip tri k as (v_k, v_{k+1}, v_{k+2}) and
    flips winding every other triangle, so a new triangle extends the strip
    iff it CONTAINS the active edge (the last two emitted verts); the third
    vertex becomes the new emit. Returns (new_vertex, new_active_edge) or None.
    Same validated model as scripts/task56_fighter_topology_census.py.
    """
    shared = [v for v in tri if v in active_edge]
    if len(shared) != 2:
        return None
    new_v = next(v for v in tri if v not in active_edge)
    a1 = active_edge[1]
    return new_v, (a1, new_v)


def _run_triangles(runs, run_index, packed_corners, run_first_corner):
    """Return the list of (v0,v1,v2) denseId triples for one run, in source order."""
    first_tri, count, _submit_class, _mask = runs[run_index]
    c0 = run_first_corner[run_index]
    tris = []
    for t in range(count):
        base = c0 + t * 3
        tris.append(tuple(packed_corners[base + k] & 0x3FF for k in range(3)))
    return tris


def _stripify_run(tris, mode):
    """Compile a run's triangles into DS-native primitive groups.

    Returns a list of groups; each group = (primitive_type, [denseId, ...]).
      GL_TRIANGLES (0): one group per residual triangle (3 verts each).
      GL_TRIANGLE_STRIP (2): connected strips (N+2 verts); the DS hardware flips
        winding per triangle so only the unordered active edge matters.
    Mode 1 = exact source order (a strip breaks whenever the next source triangle
      does not extend the active edge); residuals fall back to GL_TRIANGLES.
    Mode 2 = within-run reorder (longest-strip heuristic: try every start + every
      legal initial active-edge orientation, extend first-fit, keep the longest
      chain). Residuals fall back to GL_TRIANGLES.
    A run may be a MIX of strip groups + residual triangle groups. Cross-matrix
    runs are handled by the caller (they emit one GL_TRIANGLES group, no reorder).
    Degenerate stitches are inserted where a same-direction join is the only
    connection; they are counted as real vertex submissions (DEGENERATE TRAP).
    """
    GL_TRIANGLES = 0
    GL_TRIANGLE_STRIP = 2
    if not tris:
        return []
    if mode == 1:
        groups = []
        i = 0
        n = len(tris)
        while i < n:
            t0 = tris[i]
            # exact source order: the active edge starts as t0's last two verts.
            verts = [t0[0], t0[1], t0[2]]
            active = (t0[1], t0[2])
            j = i + 1
            while j < n:
                res = _strip_extend_active_edge(active, tris[j])
                if res is None:
                    break
                new_v, active = res
                verts.append(new_v)
                j += 1
            if len(verts) >= 4:  # >= 2 triangles in the strip
                groups.append((GL_TRIANGLE_STRIP, verts))
            else:
                groups.append((GL_TRIANGLES, [t0[0], t0[1], t0[2]]))
            i = j
        return groups
    # mode 2: longest-strip heuristic with reordering within the run.
    remaining = set(range(len(tris)))
    groups = []
    while remaining:
        best_chain = []  # list of triangle indices
        best_verts = []  # matching emit-order vertex sequence
        for start in sorted(remaining):
            t0 = tris[start]
            # The initial active edge determines the first triangle's emit
            # order; try all 3 orientations and keep the longest strip.
            #
            # These MUST be the three DIRECTED edges of t0, traversed the way
            # t0 traverses them, so that [apex, ae0, ae1] is a rotation of t0
            # and therefore carries t0's winding. Every later triangle inherits
            # that winding automatically -- the DS flips alternate triangles,
            # and a consistently wound mesh traverses a shared edge in opposite
            # directions from its two sides -- so the whole strip's facing is
            # decided here and nowhere else.
            #
            # (t0[0], t0[2]) is NOT one of those edges: it is t0[2] -> t0[0]
            # reversed, and it emits [t0[1], t0[0], t0[2]], the mirror of t0.
            # The longest-strip heuristic picked it whenever it won on length,
            # and every triangle in such a strip came out backfacing --
            # **35.6% of the fighter's 626 triangles**, culled away on
            # hardware with no assert and no counter to say so. That is what
            # made mode 2 unusable, not its runtime submission cost.
            # scripts/fighters/check_native_owner_geometry_closure.py is the
            # standing proof; run it after touching this function.
            for ae in ((t0[1], t0[2]), (t0[2], t0[0]), (t0[0], t0[1])):
                apex = next(v for v in t0 if v not in ae)
                verts = [apex, ae[0], ae[1]]
                chain = [start]
                active = ae
                rem = remaining - {start}
                while True:
                    ext = None
                    for c in sorted(rem):
                        res = _strip_extend_active_edge(active, tris[c])
                        if res is not None:
                            new_v, active = res
                            verts.append(new_v)
                            chain.append(c)
                            rem = rem - {c}
                            ext = True
                            break
                    if not ext:
                        break
                if len(verts) > len(best_verts):
                    best_verts = verts[:]
                    best_chain = chain[:]
        if len(best_chain) >= 2:
            for c in best_chain:
                remaining.discard(c)
            groups.append((GL_TRIANGLE_STRIP, best_verts))
        else:
            c = min(remaining)
            remaining.discard(c)
            groups.append((GL_TRIANGLES,
                           [tris[c][0], tris[c][1], tris[c][2]]))
    return groups


def build_fighter_primitive_streams(runs, packed_corners, run_first_corner,
                                    mode):
    """Compile every run's triangles into DS-native primitive group tables.

    Returns the 6 generated-table lists:
      run_group_first, run_group_count  (per run)
      group_type, group_first_vertex, group_vertex_count  (per group)
      primitive_vertices  (flat vertex-ref stream; each = a packed dense-corner
                           value from sNdsNativeFighterPackedCorners, preserving
                           the palette-slot high bits for cross-matrix runs)
    Cross-matrix runs (submit_class 1) emit one GL_TRIANGLES group in source
    order (no reorder -- CROSS-MATRIX TRAP). RAW runs (submit_class 0) are
    stripified per `mode`. A run may be a mix of strips + residual triangles.
    """
    GL_TRIANGLES = 0
    run_group_first = []
    run_group_count = []
    group_type = []
    group_first_vertex = []
    group_vertex_count = []
    primitive_vertices = []
    for run_index in range(len(runs)):
        first_tri, count, submit_class, _mask = runs[run_index]
        if count == 0:
            run_group_first.append(len(group_type))
            run_group_count.append(0)
            continue
        if submit_class == 1:
            # cross-matrix: one GL_TRIANGLES group, source order, no reorder.
            c0 = run_first_corner[run_index]
            verts = [packed_corners[c0 + k]
                     for k in range(count * 3)]
            run_group_first.append(len(group_type))
            group_type.append(GL_TRIANGLES)
            group_first_vertex.append(len(primitive_vertices))
            group_vertex_count.append(len(verts))
            primitive_vertices.extend(verts)
            run_group_count.append(1)
            continue
        tris = _run_triangles(runs, run_index, packed_corners,
                              run_first_corner)
        groups = _stripify_run(tris, mode)
        run_group_first.append(len(group_type))
        for gtype, gverts_dense in groups:
            # RAW runs have palette_slot == 0 (build_direct_dense_tables:1510),
            # so the packed corner value == the denseId; store denseIds directly.
            group_type.append(gtype)
            group_first_vertex.append(len(primitive_vertices))
            group_vertex_count.append(len(gverts_dense))
            primitive_vertices.extend(gverts_dense)
        run_group_count.append(len(groups))
    return (run_group_first, run_group_count, group_type,
            group_first_vertex, group_vertex_count, primitive_vertices)


def emit_rows(
        type_name: str, name: str, rows: list[str],
        const: bool = True, attribute: str = "") -> list[str]:
    qualifier = "static const" if const else "static"
    suffix = f" {attribute}" if attribute else ""
    result = [f"{qualifier} {type_name} {name}[{len(rows)}]{suffix} =", "{"]
    result.extend(f"    {row}," for row in rows)
    result.append("};")
    result.append("")
    return result


def render_p2_owner_runtime_program(
        context: dict[str, object]) -> list[str]:
    """Emit one independent P2-3 owner using the production table ABI."""
    owner_name = str(context["owner_name"])
    detail = str(context["detail"])
    owner_title = owner_name.title()
    suffix = "Low" if detail == "low" else ""
    stem = f"sNdsNative{owner_title}Fighter"
    state = context["state"]
    sequence = context["sequence"]
    vertex = context["vertex"]
    triangles = context["triangles"]
    runs = context["runs"]
    epochs = context["epochs"]
    roots = context["roots"]
    dense_vertices = context["dense_vertices"]
    dense_color_sources = context["dense_color_sources"]
    action_dense_spans = context["action_dense_spans"]
    packed_corners = context["packed_corners"]
    run_first_corner = context["run_first_corner"]
    run_first_unique = context["run_first_unique"]
    run_unique_count = context["run_unique_count"]
    run_unique_dense = context["run_unique_dense"]
    direct_policies = context["direct_epoch_policies"]
    topology = context["topology"]
    joint_schedule, binding_parents, binding_joints, cross_slots, _ = topology
    primitive_streams = context["primitive_streams"]
    light_preambles = context["light_preambles"]
    light_indices = context["light_preamble_indices"]
    asset_data_size = int(context["asset_data_size"])
    canonical_root_count = int(context.get("canonical_root_count", len(roots)))
    root_bindings = context.get("root_bindings", list(range(len(roots))))

    # ARRAYS THE IMAGE CARRIES MUST NOT ALSO BE IN THE BINARY.
    #
    # Board row P2-3r4 moves a P2-3 owner's generated tables into a NitroFS
    # image because the ARM9 binary costs the taskman arena one byte for one
    # byte. Moving them is only a saving if the binary stops containing them,
    # so every imaged array is emitted under this owner's image guard. The
    # guard is applied HERE, at the single emission choke point, rather than at
    # twenty-one call sites: adding an array to the image then means adding one
    # name to `native_owner_image_arrays`, not remembering twenty-two places.
    _emit_rows = globals()["emit_rows"]
    _image_guard = f"NDS_NATIVE_OWNER_IMAGE_{owner_name.upper()}"

    def emit_rows(type_name, name, rows, const=True, attribute=""):
        out = _emit_rows(type_name, name, rows, const=const,
                         attribute=attribute)
        base = name[len(stem):] if name.startswith(stem) else ""
        if suffix and base.endswith(suffix):
            base = base[:-len(suffix)]
        if base in NATIVE_OWNER_IMAGE_ARRAYS:
            return [f"#if !{_image_guard}"] + out + ["#endif", ""]
        return out

    lines = [
        f"/* P2-3 {owner_title} {detail} native-owner program. */",
        f"/* Source triangles={len(triangles)}, runs={len(runs)}, "
        f"dense={len(dense_vertices)}, roots={len(roots)}. */",
        "",
    ]
    lines += emit_rows(
        "NDSNativeStateDelta", f"{stem}StateDeltas{suffix}",
        [f"{{ 0x{w0:08x}u, 0x{w1:08x}u, {effect}u, {{ 0u, 0u, 0u }} }}"
         for w0, w1, effect in state],
    )
    lines += emit_rows(
        "u8", f"{stem}StateSequence{suffix}",
        [f"{value}u" for value in sequence],
    )
    lines += emit_rows(
        "NDSNativeVertexAction", f"{stem}VertexActions{suffix}",
        [f"{{ {kind}u, {command}u, {index}u, {count}u, "
         f"0x{offset:08x}u, {s}, {t} }}"
         for kind, command, index, count, offset, s, t in vertex],
    )
    lines += ["#if NDS_RENDERER_HW_TRIANGLES", ""]
    lines += emit_rows(
        "u8", f"{stem}EpochDirectPolicy{suffix}",
        [f"0x{value:02x}u" for value in direct_policies],
    )
    lines += emit_rows(
        "NDSNativeDenseVertex", f"{stem}DenseVertices{suffix}",
        ["{{ 0x{:08x}u, {}, {}, {}u, {}u, 0u }}".format(
             rgba, s, t, binding, cache_slot)
         for x, y, z, s, t, binding, cache_slot, rgba in dense_vertices],
    )
    lines += ["#if NDS_RENDERER_PROFILE_LEVEL < 2", ""]
    lines += emit_rows(
        "NDSNativePreparedDenseVertex", f"{stem}PreparedDense{suffix}",
        ["{{ .gx_xy = 0x{:08x}u, .gx_z = 0x{:04x}u }}".format(
             *pack_fifo_vertex16(x, y, z,
                                 f"{owner_name} {detail} dense vertex"))
         for x, y, z, _s, _t, _binding, _cache_slot, _rgba in dense_vertices],
        const=False,
    )
    lines += ["#endif", ""]
    lines += emit_rows(
        "u16", f"{stem}ActionDenseSpans{suffix}",
        [f"0x{value:04x}u" for value in action_dense_spans],
    )
    lines += emit_rows(
        "u16", f"{stem}DenseColorSource{suffix}",
        [f"{value}u" for value in dense_color_sources],
    )
    lines += emit_rows(
        "u16", f"{stem}PackedCorners{suffix}",
        [f"0x{value:04x}u" for value in packed_corners],
    )
    lines += emit_rows(
        "u16", f"{stem}RunFirstCorner{suffix}",
        [f"{value}u" for value in run_first_corner],
    )
    lines += emit_rows(
        "u16", f"{stem}RunFirstUnique{suffix}",
        [f"{value}u" for value in run_first_unique],
    )
    lines += emit_rows(
        "u8", f"{stem}RunUniqueCount{suffix}",
        [f"{value}u" for value in run_unique_count],
    )
    lines += emit_rows(
        "u16", f"{stem}RunUniqueDense{suffix}",
        [f"{value}u" for value in run_unique_dense],
    )
    lines += emit_rows(
        "u16", f"{stem}Triangles{suffix}",
        [f"0x{value:04x}u" for value in triangles],
    )
    lines += emit_rows(
        "NDSNativeRun", f"{stem}Runs{suffix}",
        [f"{{ {first}u, {count}u, {submit_class}u, 0x{mask:08x}u }}"
         for first, count, submit_class, mask in runs],
    )
    for primitive_mode in (1, 2):
        (run_group_first, run_group_count, group_type,
         group_first_vertex, group_vertex_count,
         primitive_vertices) = primitive_streams[primitive_mode]
        lines += [f"#if NDS_TASK56_FIGHTER_PRIMITIVES == {primitive_mode}"]
        lines += emit_rows(
            "u16", f"{stem}PrimitiveGroupFirst{suffix}",
            [f"{value}u" for value in run_group_first],
        )
        lines += emit_rows(
            "u8", f"{stem}PrimitiveGroupCount{suffix}",
            [f"{value}u" for value in run_group_count],
        )
        lines += emit_rows(
            "u8", f"{stem}PrimitiveGroupType{suffix}",
            [f"{value}u" for value in group_type],
        )
        lines += emit_rows(
            "u16", f"{stem}PrimitiveGroupFirstVertex{suffix}",
            [f"{value}u" for value in group_first_vertex],
        )
        lines += emit_rows(
            "u8", f"{stem}PrimitiveGroupVertexCount{suffix}",
            [f"{value}u" for value in group_vertex_count],
        )
        lines += emit_rows(
            "u16", f"{stem}PrimitiveVertices{suffix}",
            [f"0x{value:04x}u" for value in primitive_vertices],
        )
        lines += ["#endif", ""]
    lines += emit_rows(
        "NDSNativeEpoch", f"{stem}Epochs{suffix}",
        ["{{ {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u }}".format(*row)
         for row in epochs],
    )
    # High and Low have the same topology for admitted BattleShip fighters, but
    # emitting the cross-slot table with each program keeps the runtime table
    # self-contained and makes a future topology difference a generated diff.
    lines += emit_rows(
        "u8", f"sNdsNative{owner_title}CrossPaletteSlots{suffix}",
        [f"{value}u" for value in cross_slots],
    )
    if detail == "high":
        lines += [
            f"#define NDS_NATIVE_{owner_name.upper()}_MODEL_DATA_SIZE "
            f"0x{asset_data_size:x}u",
            "",
        ]
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
            f"static const u32 sNdsNative{owner_title}RootLightPreambles"
            f"[{len(light_preambles)}][2] =",
            "{",
        ]
        lines += [
            f"    {{ 0x{w0:08x}u, 0x{w1:08x}u }},"
            for w0, w1 in light_preambles
        ]
        lines += ["};", ""]
    elif light_preambles != context.get("high_light_preambles", light_preambles):
        # Currently unreachable; documents the ABI expectation for callers that
        # elect to share a preamble table between detail levels.
        raise ValueError(f"{owner_name}: High/Low root light preambles differ")
    root_format = "{{ 0x{:08x}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u }}"
    lines += emit_rows(
        "NDSNativeRoot", f"sNdsNative{owner_title}Roots{suffix}",
        [root_format.format(*row[:7], light_index)
         for row, light_index in zip(
             roots[:canonical_root_count], light_indices[:canonical_root_count]
         )],
    )
    if len(roots) > canonical_root_count:
        variant_rows = []
        for row, light_index, binding in zip(
                roots[canonical_root_count:],
                light_indices[canonical_root_count:],
                root_bindings[canonical_root_count:]):
            variant_rows.append(
                "{{ {}u, {} }}".format(
                    binding, root_format.format(*row[:7], light_index)
                )
            )
        lines += emit_rows(
            "NDSNativeRootVariant",
            f"sNdsNative{owner_title}RootVariants{suffix}",
            variant_rows,
        )
    lines += ["#endif", ""]
    return lines


def build_owner_source_context(
        repo_root: Path, detail: str = "high",
        include_model_part_variants: bool = True,
        ) -> dict[str, object]:
    """Recover the exact shared Mario/Fox source-order program inputs."""
    # Keep the historical byte-hashed export as the admission oracle.  Model-
    # part variants are additive source programs after this frozen prefix; they
    # must never make a canonical source drift look acceptable.
    canonical_data = build_source_export(repo_root, detail)
    canonical_state = unpack_many("<IIB3x", canonical_data["state"])
    canonical_sequence = list(canonical_data["sequence"])
    canonical_vertex = unpack_many("<BBBBIhh", canonical_data["vertex"])
    canonical_triangles = [
        item[0] for item in unpack_many("<H", canonical_data["triangles"])
    ]
    canonical_runs = unpack_many("<HBBI", canonical_data["runs"])
    canonical_epochs = unpack_many("<HHHHBBBBBBBB", canonical_data["epochs"])
    canonical_mario_roots = unpack_many(
        "<IHHHBBBB2x", canonical_data["mario_roots"]
    )
    canonical_fox_roots = unpack_many(
        "<IHHHBBBB2x", canonical_data["fox_roots"]
    )
    if (len(canonical_state), len(canonical_sequence), len(canonical_vertex),
            len(canonical_triangles), len(canonical_runs),
            len(canonical_epochs), len(canonical_mario_roots),
            len(canonical_fox_roots)) != \
            DETAIL_EXPORT_CARDINALITIES[detail]:
        raise ValueError(
            f"canonical native-fighter IR cardinality changed ({detail})")
    class_triangles = [0, 0]
    for _, triangle_count, submit_class, _ in canonical_runs:
        if submit_class >= len(class_triangles):
            raise ValueError(f"unsupported submit class {submit_class}")
        class_triangles[submit_class] += triangle_count
    if class_triangles != DETAIL_SUBMIT_CLASS_CENSUS[detail]:
        raise ValueError(
            f"submit-class census changed ({detail}): {class_triangles}")

    canonical_owner_roots = (
        ("mario", canonical_mario_roots),
        ("fox", canonical_fox_roots),
    )
    if detail == "high":
        canonical_direct_epoch_policies = build_direct_epoch_policies(
            len(canonical_epochs), "high")
    else:
        canonical_direct_epoch_policies = derive_direct_epoch_policies(
            canonical_state, canonical_sequence, canonical_epochs,
            canonical_owner_roots,
            expected_policies=(list(LOW_DIRECT_EPOCH_POLICIES)
                               if LOW_DIRECT_EPOCH_POLICIES else None))

    # Qualify the standing light census on the canonical program before the
    # additive Results variants are decoded.  The variants carry their own
    # source light commands below, but they cannot move this frozen gate.
    canonical_prefix_light_count = 0
    canonical_intra_light_count = 0
    for owner_name, roots in canonical_owner_roots:
        payload = load_o2r_payload(repo_root, owner_name)
        (_light_state, _preambles, prefix_count, intra_count) = \
            decode_epoch_light_color_state(
                payload, owner_name, roots, canonical_epochs
            )
        canonical_prefix_light_count += prefix_count
        canonical_intra_light_count += intra_count
    if (canonical_prefix_light_count, canonical_intra_light_count) != \
            DETAIL_LIGHT_CENSUS[detail]:
        raise ValueError(
            f"native-owner source light command census changed ({detail}): "
            f"prefix={canonical_prefix_light_count}, "
            f"intra-root={canonical_intra_light_count} != "
            f"{DETAIL_LIGHT_CENSUS[detail]}"
        )

    fox_variant_specs = (
        BASE_MODEL_PART_ROOT_VARIANTS.get("fox", {}).get(detail, ())
        if include_model_part_variants else ()
    )
    if fox_variant_specs:
        root_specs = {
            "mario": tuple(
                (root[0], binding)
                for binding, root in enumerate(canonical_mario_roots)
            ),
            "fox": tuple(
                (root[0], binding)
                for binding, root in enumerate(canonical_fox_roots)
            ) + tuple(
                (root_offset, binding)
                for binding, root_offset in fox_variant_specs
            ),
        }
        data = _build_source_export_for_owners(
            repo_root, ("mario", "fox"), detail,
            root_specs_by_owner=root_specs,
        )
    else:
        data = canonical_data

    state = unpack_many("<IIB3x", data["state"])
    sequence = list(data["sequence"])
    vertex = unpack_many("<BBBBIhh", data["vertex"])
    triangles = [item[0] for item in unpack_many("<H", data["triangles"])]
    runs = unpack_many("<HBBI", data["runs"])
    epochs = unpack_many("<HHHHBBBBBBBB", data["epochs"])
    mario_roots = unpack_many("<IHHHBBBB2x", data["mario_roots"])
    fox_roots = unpack_many("<IHHHBBBB2x", data["fox_roots"])
    owner_roots = (("mario", mario_roots), ("fox", fox_roots))
    owner_root_bindings = (
        tuple(range(len(canonical_mario_roots))),
        tuple(range(len(canonical_fox_roots))) +
        tuple(binding for binding, _root_offset in fox_variant_specs),
    )

    # Derive the expanded program rather than hand-copying a policy onto the
    # variants.  The canonical prefix must remain byte-for-byte equivalent to
    # the qualified pre-variant policy table.
    direct_epoch_policies = derive_direct_epoch_policies(
        state, sequence, epochs, owner_roots, expected_policies=None
    )
    if (direct_epoch_policies[:len(canonical_direct_epoch_policies)] !=
            list(canonical_direct_epoch_policies)):
        raise ValueError(
            f"{detail} model-part variants changed canonical direct policies"
        )

    owner_topologies = []
    light_state_additions = {index: ([], []) for index in range(len(epochs))}
    light_preambles = [(0, 0)]
    owner_light_preamble_indices = {}
    owner_light_command_counts = {}
    root_prefix_light_command_count = 0
    intra_root_light_command_count = 0
    for owner_index, (owner_name, roots) in enumerate(owner_roots):
        payload = load_o2r_payload(repo_root, owner_name)
        canonical_roots = canonical_owner_roots[owner_index][1]
        owner_topologies.append(
            decode_joint_topology(payload, owner_name, canonical_roots, detail)
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
    if (len(light_preambles) != 3 or
            light_preambles[1][0] != light_preambles[2][0]):
        raise ValueError(
            "native-owner root light prefixes no longer fit the compact ABI"
        )
    state, sequence, epochs, rebuilt_root_groups = \
        restore_epoch_light_color_state(
            state, sequence, epochs, (mario_roots, fox_roots),
            light_state_additions, detail)
    mario_roots, fox_roots = rebuilt_root_groups

    return {
        "detail": detail,
        "data": data,
        "canonical_data": canonical_data,
        "state": state,
        "sequence": sequence,
        "vertex": vertex,
        "triangles": triangles,
        "runs": runs,
        "epochs": epochs,
        "mario_roots": mario_roots,
        "fox_roots": fox_roots,
        "owner_roots": (("mario", mario_roots), ("fox", fox_roots)),
        "canonical_mario_roots": canonical_mario_roots,
        "canonical_fox_roots": canonical_fox_roots,
        "canonical_owner_roots": canonical_owner_roots,
        "canonical_direct_epoch_policies": canonical_direct_epoch_policies,
        "owner_root_bindings": owner_root_bindings,
        "fox_variant_specs": tuple(fox_variant_specs),
        "owner_topologies": owner_topologies,
        "direct_epoch_policies": direct_epoch_policies,
        "light_preambles": light_preambles,
        "owner_light_preamble_indices": owner_light_preamble_indices,
        "owner_light_command_counts": owner_light_command_counts,
    }


def build_p2_owner_runtime_context(
        repo_root: Path, owner_name: str, detail: str = "high"
        ) -> dict[str, object]:
    """Build the complete independent runtime IR for one P2-3 owner.

    Mario/Fox predate the production pipeline and share one combined table set.
    New owners deliberately get an independent table set instead: this keeps
    the frozen P2-2 arrays byte-identical, makes per-fighter ROM/RAM cost
    reviewable, and lets variant-specific source state (notably Luigi's second
    root-light preamble) remain exact rather than being forced through the old
    two-owner compact assumptions.
    """
    canonical_data = build_p2_owner_source_export(repo_root, owner_name, detail)
    canonical_state = unpack_many("<IIB3x", canonical_data["state"])
    canonical_sequence = list(canonical_data["sequence"])
    canonical_vertex = unpack_many("<BBBBIhh", canonical_data["vertex"])
    canonical_triangles = [
        item[0] for item in unpack_many("<H", canonical_data["triangles"])
    ]
    canonical_runs = unpack_many("<HBBI", canonical_data["runs"])
    canonical_epochs = unpack_many(
        "<HHHHBBBBBBBB", canonical_data["epochs"]
    )
    canonical_roots = unpack_many(
        "<IHHHBBBB2x", canonical_data[f"{owner_name}_roots"]
    )
    canonical_root_count = len(canonical_roots)

    variant_specs = P2_MODEL_PART_ROOT_VARIANTS.get(owner_name, {}).get(
        detail, ()
    )
    root_bindings = list(range(canonical_root_count))
    if variant_specs:
        combined_specs = tuple(
            (root[0], binding)
            for binding, root in enumerate(canonical_roots)
        ) + tuple(
            (root_offset, binding)
            for binding, root_offset in variant_specs
        )
        data = _build_source_export_for_owners(
            repo_root, (owner_name,), detail,
            root_specs_by_owner={owner_name: combined_specs},
        )
        root_bindings.extend(binding for binding, _offset in variant_specs)
    else:
        data = canonical_data
    state = unpack_many("<IIB3x", data["state"])
    sequence = list(data["sequence"])
    vertex = unpack_many("<BBBBIhh", data["vertex"])
    triangles = [item[0] for item in unpack_many("<H", data["triangles"])]
    runs = unpack_many("<HBBI", data["runs"])
    epochs = unpack_many("<HHHHBBBBBBBB", data["epochs"])
    roots = unpack_many("<IHHHBBBB2x", data[f"{owner_name}_roots"])
    owner_roots = ((owner_name, roots),)

    direct_epoch_policies = derive_direct_epoch_policies(
        state, sequence, epochs, owner_roots, expected_policies=None
    )
    payload = load_o2r_payload(repo_root, owner_name)
    # Topology belongs to the canonical JointTree only. Passive model-part
    # variants replace one selected joint's DL; they do not add a joint or a
    # logical matrix binding.
    topology = decode_joint_topology(
        payload, owner_name, canonical_roots, detail
    )
    (light_state, owner_preambles,
     prefix_light_count, intra_light_count) = decode_epoch_light_color_state(
        payload, owner_name, roots, epochs
    )

    # Unlike Mario/Fox, no assumption is made that all non-zero root preambles
    # share the same first light word.  The runtime table carries the exact pair
    # and roots keep the compact u8 index.
    light_preambles = [(0, 0)]
    light_indices = []
    for preamble in owner_preambles:
        if preamble is None:
            light_indices.append(0)
            continue
        if preamble not in light_preambles:
            light_preambles.append(preamble)
        light_indices.append(light_preambles.index(preamble))
    if len(light_preambles) > 0xff:
        raise ValueError(f"{owner_name}: root-light preamble index exceeds u8")

    additions = {index: ([], []) for index in range(len(epochs))}
    for epoch_index, (before, after) in light_state.items():
        additions[epoch_index][0].extend(before)
        additions[epoch_index][1].extend(after)
    state, sequence, epochs, rebuilt_roots = restore_epoch_light_color_state(
        state, sequence, epochs, (roots,), additions, detail,
        expected_light_additions=intra_light_count,
    )
    roots = rebuilt_roots[0]
    owner_roots = ((owner_name, roots),)

    (dense_vertices, dense_color_sources, dense_owners, dense_corners,
     action_dense_first, run_first_corner, run_owners, run_root_bindings,
     run_binding_sets) = build_dense_geometry(
        vertex, triangles, runs, epochs, owner_roots, repo_root,
        owner_root_bindings=(tuple(root_bindings),),
    )
    owner_cross_slots = [topology[3]]
    (action_dense_spans, packed_corners, run_first_unique,
     run_unique_count, run_unique_dense) = build_direct_dense_tables(
        vertex, runs, dense_vertices, dense_color_sources, dense_corners,
        action_dense_first, run_first_corner, run_owners,
        run_root_bindings, run_binding_sets, owner_cross_slots,
        detail, (owner_name,),
    )
    primitive_streams = {
        mode: build_fighter_primitive_streams(
            runs, packed_corners, run_first_corner, mode
        )
        for mode in (1, 2)
    }

    expected = P2_OWNER_MODEL_CENSUS[owner_name][detail]
    # Keep the standing source census on the canonical JointTree exactly as it
    # was before variants existed.  The variant IR is an additive executable
    # appendix, so admitting it must never move the qualified baseline.
    (canonical_dense, _canonical_colors, _canonical_dense_owners,
     canonical_corners, _canonical_action_first, _canonical_run_first,
     _canonical_run_owners, _canonical_run_bindings,
     _canonical_run_sets) = build_dense_geometry(
        canonical_vertex, canonical_triangles, canonical_runs,
        canonical_epochs, ((owner_name, canonical_roots),), repo_root
    )
    (_canonical_light_state, _canonical_preambles,
     canonical_prefix_light_count,
     canonical_intra_light_count) = decode_epoch_light_color_state(
        payload, owner_name, canonical_roots, canonical_epochs
    )
    observed = (
        len(canonical_state), len(canonical_sequence), len(canonical_vertex),
        len(canonical_triangles), len(canonical_runs), len(canonical_epochs),
        canonical_root_count, len(canonical_dense), len(canonical_corners),
        sum(1 for run in canonical_runs if run[2] == 1),
        canonical_prefix_light_count, canonical_intra_light_count,
        DETAIL_GX_PLAN_COUNTS[detail][owner_name][4],
    )
    if observed != expected:
        raise ValueError(
            f"{owner_name} {detail} runtime context census {observed} != {expected}"
        )

    return {
        "owner_name": owner_name,
        "detail": detail,
        # Runtime bounds are against the decoded O2R payload, not the outer
        # resource container. Keep this source-derived so renderer admission
        # cannot drift onto a hand-copied byte count.
        "asset_data_size": len(payload),
        "state": state,
        "sequence": sequence,
        "vertex": vertex,
        "triangles": triangles,
        "runs": runs,
        "epochs": epochs,
        "roots": roots,
        "canonical_root_count": canonical_root_count,
        "root_bindings": root_bindings,
        "variant_specs": list(variant_specs),
        "topology": topology,
        "direct_epoch_policies": direct_epoch_policies,
        "light_preambles": light_preambles,
        "light_preamble_indices": light_indices,
        "light_command_counts": (prefix_light_count, intra_light_count),
        "dense_vertices": dense_vertices,
        "dense_color_sources": dense_color_sources,
        "dense_owners": dense_owners,
        "dense_corners": dense_corners,
        "action_dense_first": action_dense_first,
        "action_dense_spans": action_dense_spans,
        "packed_corners": packed_corners,
        "run_first_corner": run_first_corner,
        "run_first_unique": run_first_unique,
        "run_unique_count": run_unique_count,
        "run_unique_dense": run_unique_dense,
        "primitive_streams": primitive_streams,
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
        context = build_owner_source_context(
            repo_root, include_model_part_variants=False
        )
    elif context.get("fox_variant_specs"):
        # Task 27 is the frozen Mario certificate.  Fox Results variants are an
        # additive runtime appendix and must not alter its checksums merely by
        # making the shared arrays longer.
        context = build_owner_source_context(
            repo_root, context.get("detail", "high"),
            include_model_part_variants=False,
        )
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
        repo_root = _paths.REPO_ROOT
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
        owner_root_bindings=context["owner_root_bindings"],
    )
    if (len(dense_vertices), len(dense_corners)) != (567, 1962):
        raise ValueError(
            "expanded dense fighter geometry cardinality changed: "
            f"{len(dense_vertices)} vertices, {len(dense_corners)} corners"
        )
    if (dense_owners[:541].count(0) != 255 or
            dense_owners[:541].count(1) != 286):
        raise ValueError("canonical owner dense-vertex census changed")
    if dense_owners[541:].count(1) != 26:
        raise ValueError("Fox model-part variant dense-vertex census changed")
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
    # Task 56: host-side fighter stripify. Compile each run's triangles into
    # DS-native primitive groups (GL_TRIANGLE_STRIP + residual GL_TRIANGLES) for
    # both mode 1 (exact source order) and mode 2 (within-run reorder). The
    # generated IR carries both sets in gated #if blocks; the build selects one
    # via NDS_TASK56_FIGHTER_PRIMITIVES. Cross-matrix runs stay GL_TRIANGLES.
    fighter_primitive_streams = {
        1: build_fighter_primitive_streams(
            runs, packed_corners, run_first_corner, 1),
        2: build_fighter_primitive_streams(
            runs, packed_corners, run_first_corner, 2),
    }
    packet_plans = []
    owner_root_first = 0
    canonical_owner_roots = context["canonical_owner_roots"]
    canonical_epoch_count = DETAIL_EXPORT_CARDINALITIES["high"][5]
    canonical_run_count = DETAIL_EXPORT_CARDINALITIES["high"][4]
    for owner_slot, ((owner_name, roots), topology) in enumerate(
            zip(canonical_owner_roots, owner_topologies)):
        packet_plans.append(build_packed_fifo_owner_plan(
            owner_name, owner_slot, roots, owner_root_first,
            epochs[:canonical_epoch_count], runs[:canonical_run_count],
            dense_vertices[:541], packed_corners[:1878],
            run_first_corner[:canonical_run_count],
            direct_epoch_policies[:canonical_epoch_count], topology[3],
        ))
        owner_root_first += len(roots)

    # Low-detail program: the same pipeline over the second JointTree arrays.
    # The source draws these models for pl_count+cp_count >= 3
    # (scvsbattle.c:188), so 3+ fighter matches need this IR to keep the
    # native fighter owner path.
    low_context = build_owner_source_context(repo_root, "low")
    low_state = low_context["state"]
    low_sequence = low_context["sequence"]
    low_vertex = low_context["vertex"]
    low_triangles = low_context["triangles"]
    low_runs = low_context["runs"]
    low_epochs = low_context["epochs"]
    low_owner_roots = low_context["owner_roots"]
    (low_dense_vertices, low_dense_color_sources, low_dense_owners,
     low_dense_corners, low_action_dense_first, low_run_first_corner,
     low_run_owners, low_run_root_bindings,
     low_run_binding_sets) = build_dense_geometry(
        low_vertex, low_triangles, low_runs, low_epochs, low_owner_roots,
        repo_root, owner_root_bindings=low_context["owner_root_bindings"])
    if (len(low_dense_vertices), len(low_dense_corners)) != (438, 1251):
        raise ValueError(
            "expanded low-detail dense geometry cardinality changed: "
            f"{len(low_dense_vertices)} vertices, "
            f"{len(low_dense_corners)} corners"
        )
    if (low_dense_owners[:420].count(0) != 187 or
            low_dense_owners[:420].count(1) != 233):
        raise ValueError("canonical low-detail owner census changed")
    if low_dense_owners[420:].count(1) != 18:
        raise ValueError("Fox low model-part variant dense census changed")
    low_owner_cross_slots = [
        topology[3] for topology in low_context["owner_topologies"]
    ]
    (low_action_dense_spans, low_packed_corners, _low_run_first_unique,
     _low_run_unique_count, _low_run_unique_dense) = \
        build_direct_dense_tables(
            low_vertex, low_runs, low_dense_vertices,
            low_dense_color_sources, low_dense_corners,
            low_action_dense_first, low_run_first_corner,
            low_run_owners, low_run_root_bindings, low_run_binding_sets,
            low_owner_cross_slots, "low")
    low_fighter_primitive_streams = {
        1: build_fighter_primitive_streams(
            low_runs, low_packed_corners, low_run_first_corner, 1),
        2: build_fighter_primitive_streams(
            low_runs, low_packed_corners, low_run_first_corner, 2),
    }
    # P2-3 keeps new owners as independent programs so the qualified Mario/Fox
    # arrays above are not reindexed. They are emitted behind per-owner admission
    # flags and therefore have zero code/data cost in the standing P2-2 build.
    p2_runtime_contexts = {}
    for owner_name, _flag in P2_RUNTIME_OWNERS:
        p2_high_context = build_p2_owner_runtime_context(
            repo_root, owner_name, "high"
        )
        p2_low_context = build_p2_owner_runtime_context(
            repo_root, owner_name, "low"
        )
        # ONE PREAMBLE TABLE PER OWNER, BUT IT IS THE UNION OF BOTH DETAILS.
        #
        # Both owner runtimes point at the single table emitted by the HIGH
        # pass, so the low program's root indices have to address that table.
        # Luigi and Donkey happen to carry identical high/low preamble sets
        # and the original code asserted that they always would.  Captain
        # Falcon does not: his LOW model's root 6 carries
        # `0xffffff00 / 0x804c3300`, a preamble the HIGH model never emits, so
        # the assertion was a false invariant that would have rejected a
        # faithful decode.  Take the UNION instead.  The high table is a prefix
        # of the union, so every high root index is unchanged; the low roots
        # are re-indexed through their own table's position in it.
        merged_preambles = list(p2_high_context["light_preambles"])
        for preamble in p2_low_context["light_preambles"]:
            if preamble not in merged_preambles:
                merged_preambles.append(preamble)
        if len(merged_preambles) > 0xff:
            raise ValueError(
                f"{owner_name}: merged root-light preamble index exceeds u8"
            )
        low_remap = [
            merged_preambles.index(preamble)
            for preamble in p2_low_context["light_preambles"]
        ]
        p2_low_context["light_preamble_indices"] = [
            low_remap[index]
            for index in p2_low_context["light_preamble_indices"]
        ]
        p2_high_context["light_preambles"] = merged_preambles
        p2_low_context["light_preambles"] = merged_preambles
        p2_low_context["high_light_preambles"] = merged_preambles
        p2_runtime_contexts[owner_name] = (p2_high_context, p2_low_context)

    lines = [
        "/* Generated by scripts/generate_nds_native_owners.py. */",
        "/* Canonical export: 32 roots, 49 epochs, 67 runs, 626 triangles. */",
        "/* Dense geometry: 541 immutable vertices, 1878 indexed corners. */",
        "/* High detail O2R lights: 120 root-prefix + 28 intra-root commands. */",
        "/* Low detail O2R lights: 104 root-prefix + 24 intra-root commands. */",
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
        # Designated initializers: R2-03 E29 drops shaded_rgba and packed_color
        # from this struct under NDS_R2_FIGHTER_HW_LIGHT, and the generator does
        # not see build flags. Naming the two fields that carry data keeps one
        # generated table correct for both layouts; the rest zero-initialise.
        ["{{ .gx_xy = 0x{:08x}u, .gx_z = 0x{:04x}u }}".format(
             *pack_fifo_vertex16(x, y, z, f"dense vertex {dense_id}"))
         for dense_id, (x, y, z, _s, _t, _binding, _cache_slot, _rgba)
         in enumerate(dense_vertices)],
        const=False,
        # R2-03 E29. DTCM: single-cycle, uncached, and CPU-only. This table is
        # randomly indexed by 1,878 corners a frame from the emit and rewritten
        # by the UV prepare, against a 4 KB data cache it does not fit in.
        # Audited for the check-task20-dtcm-layout.ps1 placement gate: written
        # and read only by ARM9 code, never a DMA source or destination (the
        # GXFIFO DMA is the stage replay's owner->words) and never touched by
        # the ARM7 or IPC.
        attribute='__attribute__((section(".dtcm.fighter")))',
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
    # Task 56: DS-native fighter primitive streams (GL_TRIANGLE_STRIP + residual
    # GL_TRIANGLES). Two table sets, one per stripify mode; the build selects one
    # via NDS_TASK56_FIGHTER_PRIMITIVES. Topology is compiled host-side; the
    # runtime only walks these tables (no strip finding at runtime).
    for _task56_mode in (1, 2):
        (run_group_first, run_group_count, group_type,
         group_first_vertex, group_vertex_count,
         primitive_vertices) = fighter_primitive_streams[_task56_mode]
        lines += [
            f"#if NDS_TASK56_FIGHTER_PRIMITIVES == {_task56_mode}",
        ]
        lines += emit_rows(
            "u16", "sNdsNativeFighterPrimitiveGroupFirst",
            [f"{value}u" for value in run_group_first],
        )
        lines += emit_rows(
            "u8", "sNdsNativeFighterPrimitiveGroupCount",
            [f"{value}u" for value in run_group_count],
        )
        lines += emit_rows(
            "u8", "sNdsNativeFighterPrimitiveGroupType",
            [f"{value}u" for value in group_type],
        )
        lines += emit_rows(
            "u16", "sNdsNativeFighterPrimitiveGroupFirstVertex",
            [f"{value}u" for value in group_first_vertex],
        )
        lines += emit_rows(
            "u8", "sNdsNativeFighterPrimitiveGroupVertexCount",
            [f"{value}u" for value in group_vertex_count],
        )
        lines += emit_rows(
            "u16", "sNdsNativeFighterPrimitiveVertices",
            [f"0x{value:04x}u" for value in primitive_vertices],
        )
        lines += [
            f"#define NDS_NATIVE_FIGHTER_PRIMITIVE_GROUP_COUNT {_task56_mode}_"
            f"{len(group_type)}",
            f"#define NDS_NATIVE_FIGHTER_PRIMITIVE_VERTEX_COUNT {_task56_mode}_"
            f"{len(primitive_vertices)}",
            "#endif",
            "",
        ]
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
             mario_roots[:len(context["canonical_mario_roots"])],
             owner_light_preamble_indices["mario"]
                 [:len(context["canonical_mario_roots"])])],
    )
    lines += emit_rows(
        "NDSNativeRoot", "sNdsNativeFoxRoots",
        [root_format.format(*row[:7], light_preamble)
         for row, light_preamble in zip(
             fox_roots[:len(context["canonical_fox_roots"])],
             owner_light_preamble_indices["fox"]
                 [:len(context["canonical_fox_roots"])])],
    )
    fox_canonical_root_count = len(context["canonical_fox_roots"])
    lines += emit_rows(
        "NDSNativeRootVariant", "sNdsNativeFoxRootVariants",
        ["{{ {}u, {} }}".format(
             binding,
             root_format.format(*row[:7], light_preamble),
         )
         for row, light_preamble, (binding, _root_offset) in zip(
             fox_roots[fox_canonical_root_count:],
             owner_light_preamble_indices["fox"][fox_canonical_root_count:],
             context["fox_variant_specs"])],
    )
    # ---- Low-detail table set (3+ fighter matches). ----
    # Mirrors the high tables with Low-suffixed names.  Nothing references
    # these yet in the same emitted order as the high set so a runtime
    # detail switch is a name swap, not a re-layout.  Unreferenced static
    # const arrays are dropped by gc-sections, so a runtime without the
    # detail switch pays nothing for them.
    lines += [
        "/* Low-detail program: 32 roots, 50 epochs, 53 runs, 393 triangles. */",
        "/* Dense geometry: 420 immutable vertices, 1179 indexed corners. */",
        "/* Light preambles are shared with the high set (identical words). */",
        "",
    ]
    lines += emit_rows(
        "NDSNativeStateDelta", "sNdsNativeFighterStateDeltasLow",
        [f"{{ 0x{w0:08x}u, 0x{w1:08x}u, {effect}u, {{ 0u, 0u, 0u }} }}"
         for w0, w1, effect in low_state],
    )
    lines += emit_rows(
        "u8", "sNdsNativeFighterStateSequenceLow",
        [f"{value}u" for value in low_sequence],
    )
    lines += emit_rows(
        "NDSNativeVertexAction", "sNdsNativeFighterVertexActionsLow",
        [f"{{ {kind}u, {command}u, {index}u, {count}u, 0x{offset:08x}u, {s}, {t} }}"
         for kind, command, index, count, offset, s, t in low_vertex],
    )
    lines += ["#if NDS_RENDERER_HW_TRIANGLES", ""]
    lines += emit_rows(
        "u8", "sNdsNativeFighterEpochDirectPolicyLow",
        [f"0x{value:02x}u" for value in low_context["direct_epoch_policies"]],
    )
    lines += emit_rows(
        "NDSNativeDenseVertex", "sNdsNativeFighterDenseVerticesLow",
        ["{{ 0x{:08x}u, {}, {}, {}u, {}u, 0u }}".format(
             rgba, s, t, binding, cache_slot)
         for dense_id, (x, y, z, s, t, binding, cache_slot, rgba)
         in enumerate(low_dense_vertices)],
    )
    lines += ["#if NDS_RENDERER_PROFILE_LEVEL < 2", ""]
    lines += emit_rows(
        "NDSNativePreparedDenseVertex", "sNdsNativeFighterPreparedDenseLow",
        ["{{ .gx_xy = 0x{:08x}u, .gx_z = 0x{:04x}u }}".format(
             *pack_fifo_vertex16(x, y, z, f"low dense vertex {dense_id}"))
         for dense_id, (x, y, z, _s, _t, _binding, _cache_slot, _rgba)
         in enumerate(low_dense_vertices)],
        const=False,
        # Main RAM, deliberately NOT .dtcm.fighter: DTCM has ~7 KB free and
        # the low set would not fit beside the high residents.  Cached main
        # RAM for the 4-player path beats the generic-interpreter fallback
        # by an order of magnitude either way.
    )
    lines += ["#endif", ""]
    lines += emit_rows(
        "u16", "sNdsNativeFighterActionDenseFirstLow",
        [f"{value}u" for value in low_action_dense_first],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterActionDenseSpansLow",
        [f"0x{value:04x}u" for value in low_action_dense_spans],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterDenseColorSourceLow",
        [f"{value}u" for value in low_dense_color_sources],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterDenseCornersLow",
        [f"{value}u" for value in low_dense_corners],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterPackedCornersLow",
        [f"0x{value:04x}u" for value in low_packed_corners],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterRunFirstCornerLow",
        [f"{value}u" for value in low_run_first_corner],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterRunFirstUniqueLow",
        [f"{value}u" for value in _low_run_first_unique],
    )
    lines += emit_rows(
        "u8", "sNdsNativeFighterRunUniqueCountLow",
        [f"{value}u" for value in _low_run_unique_count],
    )
    lines += emit_rows(
        "u16", "sNdsNativeFighterRunUniqueDenseLow",
        [f"{value}u" for value in _low_run_unique_dense],
    )
    for owner_index, ((owner_name, _roots), _topology) in enumerate(
            zip(low_owner_roots, low_context["owner_topologies"])):
        owner_title = owner_name.title()
        lines += emit_rows(
            "u8", f"sNdsNative{owner_title}CrossPaletteSlotsLow",
            [f"{value}u" for value in low_owner_cross_slots[owner_index]],
        )
    lines += emit_rows(
        "u16", "sNdsNativeFighterTrianglesLow",
        [f"0x{value:04x}u" for value in low_triangles],
    )
    lines += emit_rows(
        "NDSNativeRun", "sNdsNativeFighterRunsLow",
        [f"{{ {first}u, {count}u, {submit_class}u, 0x{mask:08x}u }}"
         for first, count, submit_class, mask in low_runs],
    )
    for _task56_mode in (1, 2):
        (run_group_first, run_group_count, group_type,
         group_first_vertex, group_vertex_count,
         primitive_vertices) = low_fighter_primitive_streams[_task56_mode]
        lines += [
            f"#if NDS_TASK56_FIGHTER_PRIMITIVES == {_task56_mode}",
        ]
        lines += emit_rows(
            "u16", "sNdsNativeFighterPrimitiveGroupFirstLow",
            [f"{value}u" for value in run_group_first],
        )
        lines += emit_rows(
            "u8", "sNdsNativeFighterPrimitiveGroupCountLow",
            [f"{value}u" for value in run_group_count],
        )
        lines += emit_rows(
            "u8", "sNdsNativeFighterPrimitiveGroupTypeLow",
            [f"{value}u" for value in group_type],
        )
        lines += emit_rows(
            "u16", "sNdsNativeFighterPrimitiveGroupFirstVertexLow",
            [f"{value}u" for value in group_first_vertex],
        )
        lines += emit_rows(
            "u8", "sNdsNativeFighterPrimitiveGroupVertexCountLow",
            [f"{value}u" for value in group_vertex_count],
        )
        lines += emit_rows(
            "u16", "sNdsNativeFighterPrimitiveVerticesLow",
            [f"0x{value:04x}u" for value in primitive_vertices],
        )
        lines += [
            f"#define NDS_NATIVE_FIGHTER_PRIMITIVE_GROUP_COUNT_LOW_"
            f"{_task56_mode}_{len(group_type)}",
            f"#define NDS_NATIVE_FIGHTER_PRIMITIVE_VERTEX_COUNT_LOW_"
            f"{_task56_mode}_{len(primitive_vertices)}",
            "#endif",
            "",
        ]
    lines += emit_rows(
        "NDSNativeEpoch", "sNdsNativeFighterEpochsLow",
        ["{{ {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u }}".format(*row)
         for row in low_epochs],
    )
    lines += emit_rows(
        "NDSNativeRoot", "sNdsNativeMarioRootsLow",
        [root_format.format(*row[:7], light_preamble)
         for row, light_preamble in zip(
             low_context["mario_roots"]
                 [:len(low_context["canonical_mario_roots"])],
             low_context["owner_light_preamble_indices"]["mario"]
                 [:len(low_context["canonical_mario_roots"])])],
    )
    lines += emit_rows(
        "NDSNativeRoot", "sNdsNativeFoxRootsLow",
        [root_format.format(*row[:7], light_preamble)
         for row, light_preamble in zip(
             low_context["fox_roots"]
                 [:len(low_context["canonical_fox_roots"])],
             low_context["owner_light_preamble_indices"]["fox"]
                 [:len(low_context["canonical_fox_roots"])])],
    )
    low_fox_canonical_root_count = len(low_context["canonical_fox_roots"])
    lines += emit_rows(
        "NDSNativeRootVariant", "sNdsNativeFoxRootVariantsLow",
        ["{{ {}u, {} }}".format(
             binding,
             root_format.format(*row[:7], light_preamble),
         )
         for row, light_preamble, (binding, _root_offset) in zip(
             low_context["fox_roots"][low_fox_canonical_root_count:],
             low_context["owner_light_preamble_indices"]["fox"]
                 [low_fox_canonical_root_count:],
             low_context["fox_variant_specs"])],
    )
    lines += ["#endif", ""]
    for owner_name, flag in P2_RUNTIME_OWNERS:
        high_context, low_context = p2_runtime_contexts[owner_name]
        lines += [
            f"#if {flag}",
            f"/* P2-3: independent source-derived {owner_name.title()} runtime owner. */",
            "",
        ]
        lines += render_p2_owner_runtime_program(high_context)
        lines += render_p2_owner_runtime_program(low_context)
        lines += [f"#endif  /* {flag} */", ""]
    return "\n".join(lines)


def build_consumed_fields_manifest(repo_root: Path) -> dict[str, object]:
    # This manifest is the Task-21/27 certificate for the frozen compact
    # Mario/Fox foundation.  Results Fox model-part programs are additive
    # runtime variants and therefore get their own manifest row below instead
    # of silently changing the historical 32/49/67 compact-object census.
    context = build_owner_source_context(
        repo_root, include_model_part_variants=False
    )
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
        "model_part_variants": {
            "fox_results_lose": {
                "source_contract": [
                    "SetModelPartID(16, 1)",
                    "SetModelPartID(10, 1)",
                ],
                "high": [
                    {"binding": binding, "root_offset": f"0x{offset:04x}"}
                    for binding, offset in BASE_MODEL_PART_ROOT_VARIANTS["fox"]["high"]
                ],
                "low": [
                    {"binding": binding, "root_offset": f"0x{offset:04x}"}
                    for binding, offset in BASE_MODEL_PART_ROOT_VARIANTS["fox"]["low"]
                ],
                "disposition": "additive source-decoded native roots",
            },
        },
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
                # Describe THIS generated program, not every future P2 owner
                # whose source topology metadata the module knows about.
                "count": sum(len(topology[0])
                             for topology in context["owner_topologies"]),
                "field_bytes": 2,
                "packed_fields": [
                    "parent:5", "binding:5", "cross_palette_slot:5",
                    "push_before:1",
                ],
            },
            "binding_joints": {
                "count": sum(len(topology[2])
                             for topology in context["owner_topologies"]),
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
        default=_paths.REPO_ROOT
        / "src" / "nds" / "nds_native_fighter_owner.generated.inc",
    )
    parser.add_argument(
        "--source-root", type=Path,
        default=_paths.REPO_ROOT,
        help="repo root containing the read-only BattleShip O2R inputs",
    )
    parser.add_argument(
        "--manifest-output", type=Path,
        default=_paths.REPO_ROOT
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
