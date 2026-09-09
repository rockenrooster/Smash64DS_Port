"""Saffron City's map layers, decoded from the original DLLink tables.

VS gkind 7; 264_GRYamabukiMap.c sets layer_mask 10. File 112 supplies layers
0/1/3 at 0x5058/0x6A70/0x8718 with 7/9/3 live DObjs. Layer0 is direct-DL under
Pri link 4 with 6 bindings; layer1/layer3 are DLLink owners under Sec links
6/17 with 9/2 bindings. Layer1 DObjs 5/7 each carry heads 0/1. Layer2 is NULL.
The three layers retain 6/9/2 bindings. The owner-authorized haze omission
reduces 232 source triangles to 228 emitted triangles without changing topology.
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

The separately composed Pokemon gate door is a fourth native owner following
the Zebes acid pattern: gryamabuki.c:246 grYamabukiMakeGate composes the gate
GObj with gcDrawDObjTreeDLLinksForGObj at link 6 at gryamabuki.c:252 from
llGRYamabukiMapMapHead at gryamabuki.c:257; gryamabuki.c:121
grYamabukiGateAddAnimOffset applies llGRYamabukiMapGateOpenAnimJoint at
gryamabuki.c:129 and llGRYamabukiMapGateCloseAnimJoint at gryamabuki.c:134.
The spawned monster/item actor remains outside this packet.
"""

from native_stage_descriptors import StageDescriptor

# Owner-authorized native-only omission (docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md):
# remove only Saffron's source haze panel, preserve every other white surface
# (layer-0 floor DL 0x4938, side walls, gate door DLs 0x0420/0x04F0 head 0 in
# MiscDataBank160, collision, and all remaining map geometry).
#
# Stable source identity (measured in
# decomp/BattleShip-main/decomp/src/relocData/112_StageYamabukiFile2.c:2156-2171
# and docs/p2/BUG_NOTES.md "Saffron white band = the source's own haze panel"):
# file 112, DObj table dStageYamabukiFile2_Layer3DObj at file offset 0x8718
# (payload word 34584, the layer-3 owner_spec dobj_offset), DObj slot index 2
# carrying id 2, DLLink table
# dStageYamabukiFile2_Layer1MatAnim_MatAnimJoint_data_0x1664_link1, display
# head 1 (the XLU head drawn with G_RM_AA_XLU_SURF), target DL
# dStageYamabukiFile2_Layer1MatAnim_MatAnimJoint_data_0x1664 (14 Gfx, the
# full-width panel at z 2296, y -888..-8633, combine G_CC_SHADE). The sibling
# link0/head-0 entry (DObj slot 1, DL 0xF4C) is NOT omitted.
#
# omitted_draw_roots below selects exactly that root. The generator keeps its
# binding/DObj identities (still 17 bindings and 19 DObjs, same roots, heads,
# and baked matrices, so runtime topology and admission still validate) and
# emits no triangles, runs, or vertices for it. Host census measured binding
# 16 (global DObj 18, head 1, root 0x8688) going 4 tris / 4 runs / 6 source
# vertices / 1 epoch to zero with every sibling binding bit-identical; the
# counts and include_sha below pin that post-omission packet. The selector
# additionally pins its actual payload DL offset 0x8688; 0x1664 is part of
# the source symbol's generated name, not the payload offset.

DESCRIPTOR = StageDescriptor(
    name="yamabuki",
    include_sha="a2f74075f999eb3ba7dad26fb2c493b33df52e811955d602d771e81ea067ec6f",
    generated_segment_index=-1,
    symbol_prefix="Yamabuki",
    macro_prefix="YAMABUKI_",
    expected_counts={
        "callbacks": 4,
        "dobjs": 24,
        "bindings": 21,
        "commands": 1095,
        "vertex_commands": 92,
        "source_vertices": 468,
        "modify_vertex_commands": 0,
        "triangle_commands": 132,
        "triangles": 243,
        "runs": 92,
        "texture_epochs": 77,
        "material_events": 2,
        "submit_classes": (73, 134, 36),
        "state_events": 505,
        "state_deltas": 159,
        "sync_events": 330,
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "alpha_clone_vertices": 30,
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
        "stage_actors": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank160",
            "sha256": "525f8f43ef30e82b29eaddf4827be29219ffaab3e280b32a1ff58583cba84aea",
            "file_id": 160,
            "internal_fixups": 35,
            "external_fixups": 6,
            "payload_sha256": "99748a0177617cd88f57cace35f6f35dde69b6aa4ff3b39dac1b7de890161a97",
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
        "actors_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/160_StageYamabukiFile4.c",
            "sha256": "9d268f5707e3964a546a694a253d20a07de1083a00d165bbf269fcbd3880de4d",
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
    asset_order=(("stage_geometry", 1), ("stage_actors", 1), ("stage_map", 4)),
    owner_specs=((0, "layer0", "stage_geometry", 20568, 8, 4, "grDisplayLayer0PriProcDisplay", False), (1, "layer1", "stage_geometry", 27248, 10, 6, "grDisplayLayer1SecProcDisplay", True), (3, "layer3", "stage_geometry", 34584, 4, 17, "grDisplayLayer3SecProcDisplay", True), (4, "gate", "stage_actors", 0x08A0, 6, 6, "gcDrawDObjTreeDLLinksForGObj", True)),
    material_sources=((112, 26912, 21152), (112, 27008, 21272)),
    material_command_partition=(5, 5),
    segment_partition=((0, 4, 0, 6, 0, 33), (1, 6, 6, 9, 33, 26), (3, 17, 15, 2, 59, 18), (4, 6, 17, 4, 77, 15)),
    omitted_draw_roots=((112, "layer3", 2, 1, 0x8688),),
    callback_partition=(("layer0", "grDisplayLayer0PriProcDisplay", 4), ("layer1", "grDisplayLayer1SecProcDisplay", 6), ("layer3", "grDisplayLayer3SecProcDisplay", 17), ("gate", "gcDrawDObjTreeDLLinksForGObj", 6)),
    segment0={

    },
    adapter_segment_count=4,
    adapter_dobj_count=24,
    adapter_binding_count=21,
    adapter_asset_count=3,
    adapter_material_count=2,
    adapter_asset_ids=(0x70, 0xA0, 0x108),
    adapter_asset_sizes=(0x10270, 0x0A90, 0x0340),
)
