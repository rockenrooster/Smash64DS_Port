"""Dream Land stage descriptor, frozen exactly as the generator hardcoded it.

Every value below is copied verbatim from
``scripts/stages/generate_nds_native_stage.py`` before parameterisation;
no numeric value, hash, path or token is new. The generator aliases its
former module constants to this descriptor, so the default run is
byte-identical.
"""

from __future__ import annotations

from native_stage_descriptors import StageDescriptor

# Owner ids, kept in the generator's OWNER_* numbering so the stored
# partition tuples stay plain ints.
OWNER_LAYER0 = 0
OWNER_LAYER1 = 1
OWNER_LAYER2 = 2
OWNER_LAYER3 = 3
OWNER_MAP0 = 4
OWNER_MAP1 = 5
OWNER_MAP2 = 6
OWNER_MAP3 = 7

DESCRIPTOR = StageDescriptor(
    name="dreamland",
    include_sha="e210eabb2bf9a8983629e814aedc135f03e77bd1154649479b0548b3994627b4",
    generated_segment_index=0,
    expected_counts={
        "callbacks": 8,
        "dobjs": 57,
        "bindings": 42,
        "commands": 886,
        "vertex_commands": 59,
        "source_vertices": 302,
        "modify_vertex_commands": 10,
        "triangle_commands": 113,
        "triangles": 202,
        "runs": 54,
        "texture_epochs": 49,
        "material_events": 4,
        "submit_classes": (66, 126, 10),
        "cross_runs": 5,
        "cross_tris": 10,
        "cross_corners": 15,
        "state_events": 423,
        "state_deltas": 148,
        "sync_events": 223,
    },
    o2r_inputs={
        "stage_images": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank103",
            "sha256": "a61e74aece06c5f15fa7cd1d6633afd9cc3750c9163caeffe59cab2d157a222a",
            "file_id": 103,
            "internal_fixups": 0,
            "external_fixups": 0,
            "payload_sha256": "4109fa4ac31fdf36d25ea228f8475a1efa6758114942ee65b051e56358684afe",
        },
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank104",
            "sha256": "3ce7e51da3810dca927521717357a2c44b1c51760bc942b0d4e5bfebe6fd4d52",
            "file_id": 104,
            "internal_fixups": 114,
            "external_fixups": 57,
            "payload_sha256": "1d82f9304458528341452b9610f55952a4951a1ead4c41cd0c5ebdab10380ebd",
        },
        "stage_actors": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank152",
            "sha256": "4a3557fc41fbb06ead175ea25b2dfac5373896cb473800638c7b2924b2f26b1a",
            "file_id": 152,
            "internal_fixups": 147,
            "external_fixups": 0,
            "payload_sha256": "cc0fc629911e04c4bdbb2d7ce9098df6b2a9a62847b08fd44f6ce9158cbc2187",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRPupupuMap",
            "sha256": "7df485462836872c0e00685876a4aa724977f480cccf861d0d41d0a19b2e224e",
            "file_id": 255,
            "internal_fixups": 1,
            "external_fixups": 9,
            "payload_sha256": "f0b62e005050c3597b4fd01abd77dadcde1fb7a339948d789c3538ef750c7e05",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/104_StagePupupuFile2.c",
            "sha256": "3608c144694eceef3639c08155e18bd6155ea91b51c95266f7f7eca2f782c845",
        },
        "actors_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/152_StagePupupuFile3.c",
            "sha256": "65ea777e827f3c6fa1baaf49b13e7a7ad7e76ec919e112d1ef3c574ead447915",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/255_GRPupupuMap.c",
            "sha256": "dabbca356a698411c8691d10c1272bb8a48b24ef6f39362f5eafa4bbae45a69c",
        },
        "pupupu": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommon/grpupupu.c",
            "sha256": "dc9f9228e00f9de2ba82d4b3747fbabb523e29d0e431e7bcf5643877e1a5d8be",
        },
        "grdisplay": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grdisplay.c",
            "sha256": "d48f187c90f66f2284625977a9e5cd8450108407f91c4d4a9247d28f5646ac03",
        },
        "objanim": {
            "path": "decomp/BattleShip-main/decomp/src/sys/objanim.c",
            "sha256": "eddedabd7aaffb4090e01fe0edcfac77f4262f42b91a3fe8faeddae2e3356dde",
        },
        "objdisplay": {
            "path": "decomp/BattleShip-main/decomp/src/sys/objdisplay.c",
            "sha256": "11f20ae08baf696ea1eff535bdede9bb21952f51e0508da0266ec21bc8eed9eb",
        },
        "reloc_symbols": {
            "path": "decomp/BattleShip-main/include/reloc_data.us.h",
            "sha256": "8c2d5938590e9a38ca2dad6ac0fa45b4742d125ed5d89f305c38774e40551385",
        },
    },
    text_contract_tokens={
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
    },
    map_constructor_text_key="pupupu",
    map_constructor_token="grPupupuMakeMapGObj(",
    map_constructor_min_count=5,
    asset_order=(
        ("stage_images", 2),
        ("stage_geometry", 1),
        ("stage_actors", 1),
        ("stage_map", 4),
    ),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0x1008, 22, 4,
         "grDisplayLayer0PriProcDisplay"),
        (OWNER_MAP0, "map0", "stage_actors", 0x10F0, 4, 4,
         "grDisplayLayer0PriProcDisplay"),
        (OWNER_MAP1, "map1", "stage_actors", 0x1770, 7, 4,
         "grDisplayLayer0PriProcDisplay"),
        (OWNER_MAP2, "map2", "stage_actors", 0x2A80, 8, 4,
         "grDisplayLayer0PriProcDisplay"),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x1CE0, 3, 6,
         "grDisplayLayer1PriProcDisplay"),
        (OWNER_LAYER2, "layer2", "stage_geometry", 0x2450, 5, 13,
         "grDisplayLayer2PriProcDisplay"),
        (OWNER_MAP3, "map3", "stage_actors", 0x31F8, 11, 16,
         "grDisplayLayer3PriProcDisplay"),
        (OWNER_LAYER3, "layer3", "stage_geometry", 0x2BF8, 5, 17,
         "grDisplayLayer3PriProcDisplay"),
    ),
    # (asset_id, binding_root, mobj_offset)
    material_sources=(
        (152, 0x0FF8, 0x0F18),
        (152, 0x1630, 0x13D8),
        (104, 0x22C8, 0x1F78),
        (104, 0x2380, 0x1FF0),
    ),
    material_command_partition=(3, 3, 10, 10),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 20, 0, 26),
        (OWNER_MAP0, 4, 20, 1, 26, 1),
        (OWNER_MAP1, 4, 21, 4, 27, 4),
        (OWNER_MAP2, 4, 25, 4, 31, 4),
        (OWNER_LAYER1, 6, 29, 1, 35, 6),
        (OWNER_LAYER2, 13, 30, 3, 41, 3),
        (OWNER_MAP3, 16, 33, 6, 44, 6),
        (OWNER_LAYER3, 17, 39, 3, 50, 4),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0PriProcDisplay", 4),
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
        ("layer2", "grDisplayLayer2PriProcDisplay", 13),
        ("layer3", "grDisplayLayer3PriProcDisplay", 17),
        ("map0", "grDisplayLayer0PriProcDisplay", 4),
        ("map1", "grDisplayLayer0PriProcDisplay", 4),
        ("map2", "grDisplayLayer0PriProcDisplay", 4),
        ("map3", "grDisplayLayer3PriProcDisplay", 16),
    ),
    segment0={
        "segment_tuple": (OWNER_LAYER0, 4, 0, 20, 0, 26),
        "state_count": 123,
        "texture_asset": 1,
        "texture_epoch_count": 22,
        "triangle_count": 54,
        "prepared_dense_count": 108,
        "prepared_dense_offset_count": 27,
        "shift_census": (78, 30),
    },
    # P2-4n1 step 3: checker copy of the step-2 runtime descriptor
    # sNdsRendererAdapterNativeStageDreamLand, verbatim. Counts equal the
    # C workspace maxima while one stage exists; asset ids are the O2R
    # file ids in asset_order; sizes are the pinned payload byte counts.
    adapter_segment_count=8,
    adapter_dobj_count=57,
    adapter_binding_count=42,
    adapter_asset_count=4,
    adapter_material_count=4,
    adapter_asset_ids=(0x67, 0x68, 0x98, 0xFF),
    adapter_asset_sizes=(0x2FC0, 0x43F0, 0x3700, 0x00C0),
)
