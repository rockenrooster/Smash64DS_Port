"""Hyrule Castle's map layers, including its shared-DObj DLLink pair.

VS gkind 4; 265_GRHyruleMap.c sets layer_mask 1. File 113 supplies layers
0/1/3 at 0x4588/0x5700/0x66A8 with 4/4/10 live DObjs. Layer0 DObj 3
emits both head 0 and head 1 display lists; it must retain two bindings to
the same live transform. The three layers contain 4/2/9 bindings and
206 triangles, occupying a 12,786-byte slab with linkage metadata.

Per-head state/vertex caches are distinct. The optional straight-line
segment optimization is omitted. The tornado, actors and effects retain
their independent source lifecycles and still need native rendering.
Runtime packet views and camera-group head execution are not yet wired.
"""

from native_stage_descriptors import StageDescriptor

DESCRIPTOR = StageDescriptor(
    name="hyrule",
    include_sha="2dfdb0059406116f834987d279761ce048982001a17859a704cbb148d72d70e8",
    generated_segment_index=-1,
    symbol_prefix="Hyrule",
    macro_prefix="HYRULE_",
    expected_counts={
        "callbacks": 3,
        "dobjs": 18,
        "bindings": 15,
        "commands": 783,
        "vertex_commands": 69,
        "source_vertices": 408,
        "modify_vertex_commands": 0,
        "triangle_commands": 107,
        "triangles": 206,
        "runs": 72,
        "texture_epochs": 56,
        "material_events": 0,
        "submit_classes": (13, 128, 65),
        "state_events": 341,
        "state_deltas": 92,
        "sync_events": 245,
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
    },
    o2r_inputs={
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank113",
            "sha256": "976a6c1d0fa7c0f766437e8729b37a25effda1b8b6ebdcaf4b9b4b37cf41cbaa",
            "file_id": 113,
            "internal_fixups": 179,
            "external_fixups": 0,
            "payload_sha256": "716a7dd68ca4dc39bc073d14b1232e83026f2b31b8bd7b4dd75890173b609127",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRHyruleMap",
            "sha256": "069823cfc096b3879f6546bdabf15a810790ced5c9b918628c8723a8ffbf0fbe",
            "file_id": 265,
            "internal_fixups": 1,
            "external_fixups": 5,
            "payload_sha256": "01eb6e42be543eb97e81584346a93842d9ffb3a0851db756f317b9e93ba62b95",
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
            "path": "decomp/BattleShip-main/decomp/src/relocData/113_StageHyruleFile2.c",
            "sha256": "9c269eef72030bbc6ab80cfedd6430bac08a8ae1b9d84f242e8ccb6dc14b3421",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/265_GRHyruleMap.c",
            "sha256": "e67eb52bf17e41f2443137a7f94c8194b2af755cb22f63f2deabe068c81f8dd7",
        },
        "ground": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommon/grhyrule.c",
            "sha256": "87aeec2d807695e93ed122297675e44d59084566a7d847afe51e3335042a6d9f",
        },
    },
    text_contract_tokens={
        "map_typed": ("MPGroundData dGRHyruleMap_MapHeader_0x0014",),
        "ground": ("grHyruleMakeGround",),
        "grdisplay": ("grDisplayLayer0SecProcDisplay", "grDisplayLayer1PriProcDisplay", "grDisplayLayer3PriProcDisplay"),
        "objdisplay": ("gcDrawDObjTreeDLLinksForGObj",),
        "reloc_symbols": ("llGRHyruleMapFileID",),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grHyruleMakeGround(",
    map_constructor_min_count=1,
    asset_order=(("stage_geometry", 1), ("stage_map", 4)),
    owner_specs=((0, "layer0", "stage_geometry", 17800, 5, 4, "grDisplayLayer0SecProcDisplay", True), (1, "layer1", "stage_geometry", 22272, 5, 6, "grDisplayLayer1PriProcDisplay", False), (3, "layer3", "stage_geometry", 26280, 11, 17, "grDisplayLayer3PriProcDisplay", False)),
    material_sources=(),
    material_command_partition=(),
    segment_partition=((0, 4, 0, 4, 0, 29), (1, 6, 4, 2, 29, 24), (3, 17, 6, 9, 53, 19)),
    callback_partition=(("layer0", "grDisplayLayer0SecProcDisplay", 4), ("layer1", "grDisplayLayer1PriProcDisplay", 6), ("layer3", "grDisplayLayer3PriProcDisplay", 17)),
    segment0={

    },
    adapter_segment_count=3,
    adapter_dobj_count=18,
    adapter_binding_count=15,
    adapter_asset_count=2,
    adapter_material_count=0,
    adapter_asset_ids=(0x71, 0x109),
    adapter_asset_sizes=(0x6890, 0x00E0),
)
