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
builds the see-saw via ``grModelSetupGroundDObjs`` with
``llGRInishieMapScaleDObjDesc`` (line 359) plus two platform DObjs from
``llGRInishieMapMapHead`` (line 372) steered by ``grInishieScaleProcUpdate``
(line 387); ``grInishieMakePakkun`` (line 413) composes two ``nITKindPakkun``
items (line 427); ``grInishieMakePowerBlock`` (line 507) spawns the
``nITKindPowerBlock`` item (line 465). ``grInishieMakeGround`` (line 572)
returns NULL after composing those. File 155 templates (scale 0x0380, Pakkun
0x0C30, PowerBlock 0x11F8) are actor data, not map layers.

Measured packet (2026-09-05): 20 DObjs / 17 bindings / 146 triangles /
10,281 slab bytes; runs per segment 24/22/2/6; submit classes (50, 64, 32);
states 139 deltas / 318 events / 177 syncs; cross-matrix (0, 0, 0).
"""

from native_stage_descriptors import StageDescriptor

DESCRIPTOR = StageDescriptor(
    name="inishie",
    include_sha="4de9951163e89385417400597040fb9860274648a69ea0cce51b5f95834e946b",
    generated_segment_index=-1,
    symbol_prefix="Inishie",
    macro_prefix="INISHIE_",
    expected_counts={
        "callbacks": 4,
        "dobjs": 20,
        "bindings": 17,
        "commands": 646,
        "vertex_commands": 53,
        "source_vertices": 268,
        "modify_vertex_commands": 0,
        "triangle_commands": 73,
        "triangles": 146,
        "runs": 54,
        "texture_epochs": 44,
        "material_events": 4,
        "submit_classes": (50, 64, 32),
        "state_events": 318,
        "state_deltas": 139,
        "sync_events": 177,
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
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
        "ground": ("grInishieMakeGround",),
        "grdisplay": ("grDisplayLayer0SecProcDisplay", "grDisplayLayer1PriProcDisplay", "grDisplayLayer2PriProcDisplay", "grDisplayLayer3PriProcDisplay"),
        "objdisplay": ("gcDrawDObjTreeDLLinksForGObj",),
        "reloc_symbols": ("llGRInishieMapFileID",),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grInishieMakeGround(",
    map_constructor_min_count=1,
    asset_order=(("stage_geometry", 1), ("stage_map", 4)),
    owner_specs=((0, "layer0", "stage_geometry", 19016, 11, 4, "grDisplayLayer0SecProcDisplay", True), (1, "layer1", "stage_geometry", 25376, 7, 6, "grDisplayLayer1PriProcDisplay", False), (2, "layer2", "stage_geometry", 26656, 3, 13, "grDisplayLayer2PriProcDisplay", False), (3, "layer3", "stage_geometry", 27648, 3, 17, "grDisplayLayer3PriProcDisplay", False)),
    material_sources=(
        (107, 0x4108, 0x3780),
        (107, 0x4188, 0x37F8),
        (107, 0x4288, 0x3870),
        (107, 0x4348, 0x38E8),
    ),
    material_command_partition=(3, 3, 3, 3),
    segment_partition=((0, 4, 0, 12, 0, 24), (1, 6, 12, 3, 24, 22), (2, 13, 15, 1, 46, 2), (3, 17, 16, 1, 48, 6)),
    callback_partition=(("layer0", "grDisplayLayer0SecProcDisplay", 4), ("layer1", "grDisplayLayer1PriProcDisplay", 6), ("layer2", "grDisplayLayer2PriProcDisplay", 13), ("layer3", "grDisplayLayer3PriProcDisplay", 17)),
    segment0={

    },
    adapter_segment_count=4,
    adapter_dobj_count=20,
    adapter_binding_count=17,
    adapter_asset_count=2,
    adapter_material_count=4,
    adapter_asset_ids=(0x6B, 0x104),
    adapter_asset_sizes=(0x6C90, 0x0170),
)
