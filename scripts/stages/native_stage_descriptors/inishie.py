"""Mushroom Kingdom (source name Inishie) map layers, DLLink-packet form.

VS gkind 8; ``grdef.h:21`` gives ``nGRKindInishie = nGRKindUnlockStart`` (the
only unlockable VS stage). ``260_GRInishieMap.c:50`` sets layer_mask 1, so
layer0 draws under ``grDisplayLayer0SecProcDisplay`` (link 4) and layers 1/2/3
under their Pri procs (links 6/13/17 per ``grdisplay.c:10-43``). File 107
supplies Layer0/1/2/3 DObj tables at 0x4A48 (11 entries, 10 live) / 0x6320
(7 entries, 6 live) / 0x6820 (3 entries, 2 live) / 0x6C00 (3 entries, 2 live);
the trailing entry of each table is the ``{ 18, NULL, scale=0 }`` sentinel.
Only layer0 goes through DObjDLLink tables (``107_StageInishieFile2.c:700-754``);
layers 1/2/3 point directly at display lists. Layer0 DObjs 1, 8 and 9 carry two
display heads each (head 0 + head 1); the packet keeps 12 layer0 bindings over
10 live DObjs. Layers 1/2/3 contribute 3/1/1 bindings, for 20 DObjs and
17 bindings overall.

Four layer0 materials (``107_StageInishieFile2.c:330-341`` header table with
slots 2/3/5/7 populated; MObjSubs at payload 0x3780/0x37F8/0x3870/0x38E8; joint
arrays at 0x3960/0x3968/0x3970/0x3978) on DObj 2/3/5/7 roots
0x4108/0x4188/0x4288/0x4348, one segment-E call each, opcode programs (3,3,3,3).

The separately composed scale platforms, Piranha plants and POW block remain
outside these map layers (``grinishie.c``): ``grInishieMakeScale`` (line 345)
builds the see-saw via the file-155 ``DObjDesc_0x0380`` chain (the five
display-list roots 0x01C8/0x02E8/0x0300/0x0328/0x0340) plus two platform DObjs
from file-155 DL 0x05F0 (line 372), all steered live by
``grInishieScaleProcUpdate`` (line 387). ``grInishieMakePakkun`` (line 413)
composes two ``nITKindPakkun`` items (line 427); ``grInishieMakePowerBlock``
(line 507) spawns the ``nITKindPowerBlock`` item (line 465).
``grInishieMakeGround`` (line 572) returns NULL after composing those. File
155 templates for Pakkun (0x0C30) and PowerBlock (0x11F8) remain outside this
packet; only the scale chain is admitted here.

Measured packet (2026-09-05): 20 DObjs / 17 bindings / 146 triangles /
10,281 slab bytes; runs per segment 24/22/2/6; submit classes (50, 64, 32);
states 139 deltas / 318 events / 177 syncs; cross-matrix (0, 0, 0).
"""

from native_stage_descriptors import StageDescriptor

DESCRIPTOR = StageDescriptor(
    name="inishie",
    include_sha="585dcff11560be888526b372de7f5a3a0bbf7f721b6d6fc8b65c800f2ac55f93",
    generated_segment_index=-1,
    symbol_prefix="Inishie",
    macro_prefix="INISHIE_",
    expected_counts={
        "callbacks": 7,
        "dobjs": 27,
        "bindings": 24,
        "commands": 779,
        "vertex_commands": 63,
        "source_vertices": 324,
        "modify_vertex_commands": 4,
        "triangle_commands": 88,
        "triangles": 176,
        "runs": 65,
        "texture_epochs": 54,
        "material_events": 4,
        "submit_classes": (76, 64, 36),
        "state_events": 381,
        "state_deltas": 151,
        "sync_events": 209,
        "cross_runs": 2,
        "cross_tris": 4,
        "cross_corners": 6,
        "alpha_clone_vertices": 6,
    },
    o2r_inputs={
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank107",
            "sha256": "788aba60b041c6b84c01917bb78fe00b47a50e7a27a76b9eee262b04c324d0fd",
            "file_id": 107,
            "internal_fixups": 175,
            "external_fixups": 0,
            "payload_sha256": "01eeedeaa38f19273d7f166b4c05667f66e78a1eac16ea5d93107b64ced426e3",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRInishieMap",
            "sha256": "36d0ea77b152b0d685382aea47744f989089141be1331409920bbba93e95d16b",
            "file_id": 260,
            "internal_fixups": 1,
            "external_fixups": 14,
            "payload_sha256": "32cfb816db5bc2fe1cd115e8cf21f76b14720ec02dc0c88dc34700c5f7abcf66",
        },
        "stage_actors": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank155",
            "sha256": "6efd06fa672c3831efdacea1df123d9e549dda0700a76916f8302d90463fcc02",
            "file_id": 155,
            "internal_fixups": 40,
            "external_fixups": 4,
            "payload_sha256": "8d1c1c037d0b6a3860c6068a75261fbe61b1e6ca4aabe0d930af1b7e01fb197e",
        },
    },
    text_inputs={
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
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/107_StageInishieFile2.c",
            "sha256": "eac5719acc9e1f4e227fb387d270209c7738932436ff8806f3418eba3a249bc5",
        },
        "actors_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/155_StageInishieFile3.c",
            "sha256": "957deacd2307e00983b0ce4aed07d0526358f30607ab3816e9db892a80075ef7",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/260_GRInishieMap.c",
            "sha256": "e2f611d83dc0172f85d32895263d24b66c7e3843f11868ddf8eefa93e64c9a27",
        },
        "ground": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommon/grinishie.c",
            "sha256": "0d43e308dfc672381db606ea90c933d3977b5401691fa61172ab865215f7a549",
        },
    },
    text_contract_tokens={
        "map_typed": ("MPGroundData dGRInishieMap_MapHeader_0x0014",),
        "ground": ("grInishieMakeGround", "grInishieMakeScale", "gcAddDObjForGObj"),
        "actors_typed": (
            "dStageInishieFile3_DObjDesc_0x0380",
            "dStageInishieFile3_DL_0x01C8",
            "dStageInishieFile3_DL_0x02E8",
            "dStageInishieFile3_DL_0x0300",
            "dStageInishieFile3_DL_0x0328",
            "dStageInishieFile3_DL_0x0340",
            "dStageInishieFile3_DL_0x05F0",
            "dStageInishieFile3_Tex_0x0498",
            "dStageInishieFile3_Tex_0x04B0",
        ),
        "grdisplay": ("grDisplayLayer0SecProcDisplay", "grDisplayLayer1PriProcDisplay", "grDisplayLayer2PriProcDisplay", "grDisplayLayer3PriProcDisplay"),
        "objdisplay": (
            "gcDrawDObjTreeDLLinksForGObj",
            "gcDrawDObjTreeForGObj",
            "gcDrawDObjDLHead0",
        ),
        "reloc_symbols": ("llGRInishieMapFileID",),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grInishieMakeGround(",
    map_constructor_min_count=1,
    asset_order=(
        ("stage_geometry", 1),
        ("stage_actors", 1),
        ("stage_map", 4),
    ),
    # The three scale owners follow layer 1 on link 6, exactly in the order
    # grInishieMakeScale creates their GObjs. The two direct roots share the
    # source DL but are separate live platform DObjs.
    owner_specs=(
        (0, "layer0", "stage_geometry", 19016, 11, 4, "grDisplayLayer0SecProcDisplay", True),
        (1, "layer1", "stage_geometry", 25376, 7, 6, "grDisplayLayer1PriProcDisplay", False),
        (4, "scale", "stage_actors", 0x0380, 6, 6, "gcDrawDObjTreeForGObj", False),
        (5, "scale_left", "stage_actors", 0, 1, 6, "gcDrawDObjDLHead0", False, 0x05F0),
        (6, "scale_right", "stage_actors", 0, 1, 6, "gcDrawDObjDLHead0", False, 0x05F0),
        (2, "layer2", "stage_geometry", 26656, 3, 13, "grDisplayLayer2PriProcDisplay", False),
        (3, "layer3", "stage_geometry", 27648, 3, 17, "grDisplayLayer3PriProcDisplay", False),
    ),
    material_sources=(
        (107, 0x4108, 0x3780),
        (107, 0x4188, 0x37F8),
        (107, 0x4288, 0x3870),
        (107, 0x4348, 0x38E8),
    ),
    material_command_partition=(3, 3, 3, 3),
    segment_partition=(
        (0, 4, 0, 12, 0, 25),
        (1, 6, 12, 3, 25, 22),
        (4, 6, 15, 5, 47, 6),
        (5, 6, 20, 1, 53, 2),
        (6, 6, 21, 1, 55, 2),
        (2, 13, 22, 1, 57, 2),
        (3, 17, 23, 1, 59, 6),
    ),
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
        ("layer2", "grDisplayLayer2PriProcDisplay", 13),
        ("layer3", "grDisplayLayer3PriProcDisplay", 17),
        ("scale", "gcDrawDObjTreeForGObj", 6),
        ("scale_left", "gcDrawDObjDLHead0", 6),
        ("scale_right", "gcDrawDObjDLHead0", 6),
    ),
    segment0={

    },
    adapter_segment_count=7,
    adapter_dobj_count=27,
    adapter_binding_count=24,
    adapter_asset_count=3,
    adapter_material_count=4,
    adapter_asset_ids=(0x6B, 0x09B, 0x104),
    adapter_asset_sizes=(0x6C90, 0x1410, 0x0170),
)
