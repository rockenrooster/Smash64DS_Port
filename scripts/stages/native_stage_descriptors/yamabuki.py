"""Saffron City's map layers, decoded from the original DLLink tables.

VS gkind 7; 264_GRYamabukiMap.c sets layer_mask 10. File 112 supplies layers
0/1/3 at 0x5058/0x6A70/0x8718 with 7/9/3 live DObjs. Layer0 is direct-DL under
Pri link 4 with 6 bindings; layer1/layer3 are DLLink owners under Sec links
6/17 with 9/2 bindings. Layer1 DObjs 5/7 each carry heads 0/1. Layer2 is NULL.
The three layers contain 6/9/2 bindings and 232 triangles, occupying a
14,810-byte slab with linkage metadata.
Both heads keep independent state/vertex caches. No segment program emitted.

Materials: exactly two display lists call into segment-E material programs,
both head-1 DLLink targets in layer1: root 0x6920 (DObj 5) and root 0x6980
(DObj 7). Pairing is file-grounded: dStageYamabukiFile2_Layer1MObj_MObjSub[9]
is file-initialised only at slots 5/7 (see the comment at
112_StageYamabukiFile2.c:902), pointing through the MObjSub*[2] arrays at
payload 0x5390/0x5398 at the MObjSub structs at payload 0x52A0/0x5318. Both
MObjs carry flags 0xA1 with sprite-frame arrays, i.e. opcode programs of
length 5 each -- hence material_command_partition == (5, 5). The head-0
twins (0x6508/0x66D0) make no segment-E call, so they own no material event;
the shared-DObj shape needs no decoder change beyond one event per calling
binding.

REGION_US: map item weights take the non-JP branch (264_GRYamabukiMap.c:35-36).

The separately composed Pokemon gate door and monster/item actors remain
outside these map layers (gryamabuki.c:246 grYamabukiMakeGate composes the
gate GObj with gcDrawDObjTreeDLLinksForGObj at link 6 at gryamabuki.c:252
from llGRYamabukiMapMapHead at gryamabuki.c:257; gryamabuki.c:121
grYamabukiGateAddAnimOffset applies llGRYamabukiMapGateOpenAnimJoint at
gryamabuki.c:129 and llGRYamabukiMapGateCloseAnimJoint at gryamabuki.c:134;
gryamabuki.c:101 grYamabukiGateMakeMonster composes the monster item with
itManagerMakeItemSetupCommon). Runtime packet views, head-group execution,
and native actors are still required before admission.
"""

from native_stage_descriptors import StageDescriptor

DESCRIPTOR = StageDescriptor(
    name="yamabuki",
    include_sha="06c71785ca3f37ce1d95b89bc4e514c35fe45f3fbbaba42171763caadac8b4f8",
    generated_segment_index=-1,
    symbol_prefix="Yamabuki",
    macro_prefix="YAMABUKI_",
    expected_counts={
        "callbacks": 3,
        "dobjs": 19,
        "bindings": 17,
        "commands": 963,
        "vertex_commands": 78,
        "source_vertices": 429,
        "modify_vertex_commands": 0,
        "triangle_commands": 119,
        "triangles": 232,
        "runs": 78,
        "texture_epochs": 67,
        "material_events": 2,
        "submit_classes": (58, 138, 36),
        "state_events": 447,
        "state_deltas": 159,
        "sync_events": 294,
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
    },
    o2r_inputs={
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank112",
            "sha256": "860553a3c63dcdeae2b521724ebd560f925111501aec1db39a1b4a1bc0e20211",
            "file_id": 112,
            "internal_fixups": 293,
            "external_fixups": 0,
            "payload_sha256": "aea98d8356f47b21c78d7315e1cd3450e0332a9c5272dde1c391ca19b9097938",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRYamabukiMap",
            "sha256": "8e17aa95be010e865711353a8ea965f9dc4f8ec9c17f615e453e0a8b3fb4505d",
            "file_id": 264,
            "internal_fixups": 1,
            "external_fixups": 23,
            "payload_sha256": "4236015e06f24d4f8b701053b89df003bae5eac210cf7d3b7fbcddaf5fc53b85",
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
            "path": "decomp/BattleShip-main/decomp/src/relocData/112_StageYamabukiFile2.c",
            "sha256": "0dcbf27a20188455ed1c88c89740794013fc520566574653a97d3204bf1f9ec2",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/264_GRYamabukiMap.c",
            "sha256": "4d539411be73100c8939f86c8f7f23b587c86d2f4b3f036e0b076a3a6f5bc046",
        },
        "ground": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommon/gryamabuki.c",
            "sha256": "fbeccef139f60567aec93c5c31f1e328787856b31a033cb7edaecfd6ba9dc06b",
        },
    },
    text_contract_tokens={
        "map_typed": ("MPGroundData dGRYamabukiMap_ItemHead_0x0014",),
        "ground": ("grYamabukiMakeGround",),
        "grdisplay": ("grDisplayLayer0PriProcDisplay", "grDisplayLayer1SecProcDisplay", "grDisplayLayer3SecProcDisplay"),
        "objdisplay": ("gcDrawDObjTreeDLLinksForGObj",),
        "reloc_symbols": ("llGRYamabukiMapFileID",),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grYamabukiMakeGround(",
    map_constructor_min_count=1,
    asset_order=(("stage_geometry", 1), ("stage_map", 4)),
    owner_specs=((0, "layer0", "stage_geometry", 20568, 8, 4, "grDisplayLayer0PriProcDisplay", False), (1, "layer1", "stage_geometry", 27248, 10, 6, "grDisplayLayer1SecProcDisplay", True), (3, "layer3", "stage_geometry", 34584, 4, 17, "grDisplayLayer3SecProcDisplay", True)),
    material_sources=((112, 26912, 21152), (112, 27008, 21272)),
    material_command_partition=(5, 5),
    segment_partition=((0, 4, 0, 6, 0, 33), (1, 6, 6, 9, 33, 26), (3, 17, 15, 2, 59, 19)),
    callback_partition=(("layer0", "grDisplayLayer0PriProcDisplay", 4), ("layer1", "grDisplayLayer1SecProcDisplay", 6), ("layer3", "grDisplayLayer3SecProcDisplay", 17)),
    segment0={

    },
    adapter_segment_count=3,
    adapter_dobj_count=19,
    adapter_binding_count=17,
    adapter_asset_count=2,
    adapter_material_count=2,
    adapter_asset_ids=(0x70, 0x108),
    adapter_asset_sizes=(0x10270, 0x0340),
)
