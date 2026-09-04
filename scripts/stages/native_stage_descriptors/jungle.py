"""Kongo Jungle's four native map layers (P2-4n1).

Source: 261_GRJungleMap.c's gr_desc at payload 0x14, layer_mask == 0;
grdisplay.c selects primary callbacks at links 4/6/13/17. The original O2R
tables terminate after 19/5/4/10 entries at geometry offsets
0x8B18/0x9878/0xC7D8/0xD188. Their live counts are 18/4/3/9 and bindings
17/3/2/8. The C comments for layers 0/3 are stale; counts here were read
from the pinned binary sentinels, not those comments.

Layer1's AnimJoint at 0x9B20 drives DObjs 1/2 (bindings 17/18), the two
moving platforms. The other bindings have no joint animation or animated
ancestor; the runtime rigid mask is therefore 0x3FF9FFFF. All four map
material tables are NULL, so this packet has no material events.

The separate barrel cannon remains an explicit native-actor requirement:
grJungleMakeTaruCann (grjungle.c:107) makes it from file 158's DObjDesc at
0xA98, using gcDrawDObjTreeForGObj at link 6. Its root uses matrix kind
0x28 plus RotRpyR and its child has transform flag 4; this needs actor
capture/matrix support beyond the four layer callbacks, not omission
from the completed stage.

Source-derived packet: 34 DObjs, 30 bindings, 72 runs, 60 texture epochs,
182 triangles; submit census raw/no-Z/range = 26/136/20, no cross-binding
triangles. Const slab 12,479 bytes. The layer-0 straight-line program is
122 bytes for 41 runs; its 170 dense vertices include 36 at shift 2, in
addition to the legacy shift-0/1 census (118/16).
"""

from native_stage_descriptors import StageDescriptor

DESCRIPTOR = StageDescriptor(
    name="jungle",
    include_sha="881d8aa30b03693c086751171a1316ba27d0627a93bd4cfdf7b173d6342b8cbe",
    generated_segment_index=0,
    symbol_prefix="Jungle",
    macro_prefix="JUNGLE_",
    expected_counts={
        "callbacks": 4,
        "dobjs": 34,
        "bindings": 30,
        "commands": 830,
        "vertex_commands": 75,
        "source_vertices": 350,
        "modify_vertex_commands": 0,
        "triangle_commands": 92,
        "triangles": 182,
        "runs": 72,
        "texture_epochs": 60,
        "material_events": 0,
        "submit_classes": (26, 136, 20),
        "state_events": 359,
        "state_deltas": 96,
        "sync_events": 266,
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
    },
    o2r_inputs={
        "stage_images": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank107",
            "sha256": "788aba60b041c6b84c01917bb78fe00b47a50e7a27a76b9eee262b04c324d0fd",
            "file_id": 107,
            "internal_fixups": 175,
            "external_fixups": 0,
            "payload_sha256": "01eeedeaa38f19273d7f166b4c05667f66e78a1eac16ea5d93107b64ced426e3",
        },
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank108",
            "sha256": "88c28ded72fbb300aa5600fd833832fba4c6acbf0c2813c88abb5233e018e8da",
            "file_id": 108,
            "internal_fixups": 225,
            "external_fixups": 0,
            "payload_sha256": "cd7f42d77f22fc847d9de852dffd1c536e47e1d842f9c0000c283044bd226074",
        },
        "stage_actors": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank158",
            "sha256": "a6a093169e44bcc39b147a3e28db8e4c5172488e43dd550872ce7feffb013787",
            "file_id": 158,
            "internal_fixups": 10,
            "external_fixups": 0,
            "payload_sha256": "ff223221365b42f0634b57771c8286e011308639fbd6a20ba1aaa1a64b16dd37",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRJungleMap",
            "sha256": "438435c382586c08a2097ab33bacdd49f5ac5612881cc64932ed16acdded9938",
            "file_id": 261,
            "internal_fixups": 1,
            "external_fixups": 8,
            "payload_sha256": "9f99b963916ae2a2aca4578fcd610ded09aa080aa7ca0c52b5a91b41dd8a8ad1",
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
            "path": "decomp/BattleShip-main/decomp/src/relocData/108_StageJungleFile2.c",
            "sha256": "6bbdb8b73c80d05968b281f85548180604283967b516d54a0d976451019834f5",
        },
        "actors_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/158_StageJungleFile3.c",
            "sha256": "97bb9aaa752c3e7d7de65a4940adfada235ca805f1bcf1206f2c47ad644cca8d",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/261_GRJungleMap.c",
            "sha256": "760ae13ae4a5f6a7785371c6b85e828839bba89a96fe04f6d57ec83afda0ba32",
        },
        "jungle": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommon/grjungle.c",
            "sha256": "e0ff02783d15d7e39b51bf8b2c4702000ba6259f7d99b779d597463f034aaa7d",
        },
    },
    text_contract_tokens={
        "grdisplay": ("grDisplayLayer0PriProcDisplay", "grDisplayLayer1PriProcDisplay", "grDisplayLayer2PriProcDisplay", "grDisplayLayer3PriProcDisplay", "gcDrawDObjTreeForGObj"),
        "objanim": ("gcPlayAnimAll", "gcParseMObjMatAnimJoint"),
        "objdisplay": ("void gcDrawMObjForDObj", "gSPSegment(dl_head[0]++, 0xE", "void gcDrawDObjTreeForGObj"),
        "reloc_symbols": ("llGRJungleMapFileID", "llGRJungleMapMapHead", "llGRJungleMapTaruCannDefaultAnimJoint"),
        "map_typed": ("MPGroundData dGRJungleMap_MapHeader_0x0014", "dStageJungleFile2_Layer0DObj", "dStageJungleFile2_Layer1DObj", "dStageJungleFile2_Layer2DObj", "dStageJungleFile2_Layer3DObj"),
        "jungle": ("grJungleMakeGround", "grJungleMakeTaruCann", "gcAddAnimJointAll"),
    },
    map_constructor_text_key="jungle",
    map_constructor_token="grJungleMakeGround(",
    map_constructor_min_count=1,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_actors", 1), ("stage_map", 4)),
    owner_specs=((0, "layer0", "stage_geometry", 35608, 19, 4, "grDisplayLayer0PriProcDisplay"), (1, "layer1", "stage_geometry", 39032, 5, 6, "grDisplayLayer1PriProcDisplay"), (2, "layer2", "stage_geometry", 51160, 4, 13, "grDisplayLayer2PriProcDisplay"), (3, "layer3", "stage_geometry", 53640, 10, 17, "grDisplayLayer3PriProcDisplay")),
    material_sources=(),
    material_command_partition=(),
    segment_partition=((0, 4, 0, 17, 0, 41), (1, 6, 17, 3, 41, 11), (2, 13, 20, 2, 52, 6), (3, 17, 22, 8, 58, 14)),
    callback_partition=(("layer0", "grDisplayLayer0PriProcDisplay", 4), ("layer1", "grDisplayLayer1PriProcDisplay", 6), ("layer2", "grDisplayLayer2PriProcDisplay", 13), ("layer3", "grDisplayLayer3PriProcDisplay", 17)),
    segment0={
        "segment_tuple": (0, 4, 0, 17, 0, 41),
        "state_first": 0,
        "state_count": 160,
        "texture_asset": 1,
        "texture_epoch_count": 33,
        "triangle_count": 88,
        "prepared_dense_count": 170,
        "prepared_dense_offset_count": 42,
        "shift_census": (118, 16),
        "program_footprint": 122,
    },
    adapter_segment_count=4,
    adapter_dobj_count=34,
    adapter_binding_count=30,
    adapter_asset_count=4,
    adapter_material_count=0,
    adapter_asset_ids=(107, 108, 158, 261),
    adapter_asset_sizes=(27792, 62944, 3296, 224),
)

