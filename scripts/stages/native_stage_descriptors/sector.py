"""Sector Z's two map layers, decoded from the original DLLink tables.

VS gkind 1; 262_GRSectorMap.c sets layer_mask 3 and names file 109's
Layer0/1 DObj tables at 0x44C8/0x8680. Binary sentinels give 12/11 live
DObjs (the C count comments are stale), with 9/10 display-list bindings.
Each DLLink has one real entry; heads 0/1 carry 10/9 bindings overall.
Both layers animate. Six DObjs use source flag 8 (matrix kind 44).

The native packet preserves source DObj preorder and carries explicit head
and binding-to-DObj arrays. Head-local state/vertex caches are independent.
The 299 triangles occupy a 16,307-byte slab, including linkage metadata.
No straight-line segment optimization is required by this packet.

The separately composed Arwing and its weapons/effects remain outside these
map layers (grsector.c:1087 onward loads FoxSpecial3). Runtime packet views,
head-group execution, and native actors are still required before admission.
"""

from native_stage_descriptors import StageDescriptor

DESCRIPTOR = StageDescriptor(
    name="sector",
    include_sha="aaef44a984793149c69df6efd336ba948b651d0dfdf540ebeea6b00913cd1aa6",
    generated_segment_index=-1,
    symbol_prefix="Sector",
    macro_prefix="SECTOR_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 23,
        "bindings": 19,
        "commands": 681,
        "vertex_commands": 76,
        "source_vertices": 587,
        "modify_vertex_commands": 0,
        "triangle_commands": 158,
        "triangles": 299,
        "runs": 70,
        "texture_epochs": 50,
        "material_events": 0,
        "submit_classes": (81, 31, 187),
        "state_events": 244,
        "state_deltas": 114,
        "sync_events": 176,
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
    },
    o2r_inputs={
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank109",
            "sha256": "7670e2e1cd8bd02c895c18e028e57455323a81ee58567312f7f947be38c2f9b7",
            "file_id": 109,
            "internal_fixups": 227,
            "external_fixups": 0,
            "payload_sha256": "d37d5237473d6352b00ccf8162d5d1106526d4c101e22455faff0b518fb02f36",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRSectorMap",
            "sha256": "63c75e3f482a5f614820e1d9790f00a7bb4563237cadb5add2ba724c6919bad3",
            "file_id": 262,
            "internal_fixups": 1,
            "external_fixups": 9,
            "payload_sha256": "30e70b204089cbaf8f9135ebb6629a4892ce60e196b79db824d380d4a24ab07b",
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
            "path": "decomp/BattleShip-main/decomp/src/relocData/109_StageSectorFile2.c",
            "sha256": "70c3f07b175759bb73291000c8a4833bebb4c0ea85c704236d5a56ca93ff2a2e",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/262_GRSectorMap.c",
            "sha256": "d8ff322feb391d1912cecc5231a3819cf02e2427d46c6434c6b82343cf6d61e7",
        },
        "ground": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommon/grsector.c",
            "sha256": "4f5e8b827408fd3c0e45dbf7b0e7e3e51f969d7373cfb9fa3b7bd383a66290a3",
        },
    },
    text_contract_tokens={
        "map_typed": ("MPGroundData dGRSectorMap_MapHeader_0x0014",),
        "ground": ("grSectorMakeGround",),
        "grdisplay": ("grDisplayLayer0SecProcDisplay", "grDisplayLayer1SecProcDisplay"),
        "objdisplay": ("gcDrawDObjTreeDLLinksForGObj",),
        "reloc_symbols": ("llGRSectorMapFileID",),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grSectorMakeGround(",
    map_constructor_min_count=1,
    asset_order=(("stage_geometry", 1), ("stage_map", 4)),
    owner_specs=((0, "layer0", "stage_geometry", 17608, 13, 4, "grDisplayLayer0SecProcDisplay", True), (1, "layer1", "stage_geometry", 34432, 12, 6, "grDisplayLayer1SecProcDisplay", True)),
    material_sources=(),
    material_command_partition=(),
    segment_partition=((0, 4, 0, 9, 0, 17), (1, 6, 9, 10, 17, 53)),
    callback_partition=(("layer0", "grDisplayLayer0SecProcDisplay", 4), ("layer1", "grDisplayLayer1SecProcDisplay", 6)),
    segment0={

    },
    adapter_segment_count=2,
    adapter_dobj_count=23,
    adapter_binding_count=19,
    adapter_asset_count=2,
    adapter_material_count=0,
    adapter_asset_ids=(0x6D, 0x106),
    adapter_asset_sizes=(0xB810, 0x0130),
)
