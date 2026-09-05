"""Small Yoshi's Island's four static map layers, decoded from the original tables.

1P gkind 12 (nGRKindYosterSmall, grdef.h:28); 270_GRYosterSmallMap.c sets
layer_mask 0 and wires all four gr_desc rows to file 118, with an
AnimJoint only on layer0. File 118 supplies layers 0-3 at
0xA88/0x37F0/0x3CA8/0x4498 with 15/3/3/4 DObjDesc entries (sentinel
included): layer0 holds 14 live DObjs (DObj 0 null, 13 DL-backed),
layer1/layer2 hold 2 live with 1 binding each, layer3 holds 3 live with
2 bindings. Links/callbacks are the standard dGRDisplayDescs Pri rows
(grdisplay.c:10-43): links 4/6/13/17. No DLLink layer, no MObj structs
anywhere in file 118, so material_sources is empty -- confirmed by the
generator's fail-closed segment-E walk, which accepts zero materials.

Textures live outside the geometry bank: file 118's 36 extern fixups all
target file 110 (StageYosterImages, the same ExternDataBank110 Yoster
uses), so stage_images is a third packet input with asset flag 2.

REGION_US: map item weights take the non-JP branch
(270_GRYosterSmallMap.c:20).

No per-stage gr* ground TU exists for this kind (grmainsetup.c:11-22
lists only the nine VS kinds, and gryoster.c's grYosterMakeGround builds
the full-size island's clouds, not this variant), so there is no gr*Make*
dynamic actor to exclude. The four static layers are composed by
grCommonSetupInitAll (grcommonsetup.c:25-28) via
grDisplayMakeGeometryLayer, which is the map-constructor anchor below.
The Yoshi Team fighters are fighter actors, not stage ground. Runtime
packet views are still required before admission.
"""

from __future__ import annotations

from native_stage_descriptors import StageDescriptor

OWNER_LAYER0 = 0
OWNER_LAYER1 = 1
OWNER_LAYER2 = 2
OWNER_LAYER3 = 3

DESCRIPTOR = StageDescriptor(
    name="yostersmall",
    include_sha="e6998e02b7b6a521a218bb271112a698a4080b8c8c18750f76017e0f576d3a53",
    generated_segment_index=-1,
    symbol_prefix="YosterSmall",
    macro_prefix="YOSTERSMALL_",
    expected_counts={
        "callbacks": 4,
        "dobjs": 21,
        "bindings": 17,
        "commands": 525,
        "vertex_commands": 50,
        "source_vertices": 323,
        "modify_vertex_commands": 0,
        "triangle_commands": 85,
        "triangles": 160,
        "runs": 50,
        "texture_epochs": 45,
        "material_events": 0,
        "submit_classes": (53, 83, 24),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 227,
        "state_deltas": 111,
        "sync_events": 142,
    },
    o2r_inputs={
        "stage_images": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank110",
            "sha256": "bb17c238cdd8ea96bb1205370064d55a4f7708fa2db805c74ed6326bee88106f",
            "file_id": 110,
            "internal_fixups": 0,
            "external_fixups": 0,
            "payload_sha256": "a29e56feb0dac75bc01b6a1acfb12e81b4380aba99ce02e6662dead9917574e8",
        },
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank118",
            "sha256": "f56278f03a4a08a96c830aeb022b0012dc6d7c51767ed65344384ee19e9377ca",
            "file_id": 118,
            "internal_fixups": 96,
            "external_fixups": 36,
            "payload_sha256": "65f594dccfb1e3a136c870cdff9146a5f4ae2fcc09a65537b4dced6f3e8464f1",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRYosterSmallMap",
            "sha256": "a2d1fad8ae1143757b6be5ecdce2730c5d828a99703f3683718934f78e67160f",
            "file_id": 270,
            "internal_fixups": 1,
            "external_fixups": 7,
            "payload_sha256": "3c99c1a87b582f48720c15e63f58a92900edeea62b1163c764ccbcb9d28ff234",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/118_StageYosterSmallFile2.c",
            "sha256": "c68e6c5026edae87a315f9012bddcbf8875e91762ff0dc2d23fee49bdf10e75b",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/270_GRYosterSmallMap.c",
            "sha256": "9e72d51852b7cc08be206721a1ef4774918695097d59819f519234866c0c6e56",
        },
        "ground": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommonsetup.c",
            "sha256": "12a3486f0c9a5d979b13f3c05da26613b7bed6689870af612a258f90e2677455",
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
            "MPGroundData dGRYosterSmallMap_header",
            "dStageYosterSmallFile2_Layer0DObj",
            "dStageYosterSmallFile2_Layer1DObj",
            "dStageYosterSmallFile2_Layer2DObj",
            "dStageYosterSmallFile2_Layer3DObj",
        ),
        "ground": (
            "grCommonSetupInitAll",
            "grDisplayMakeGeometryLayer",
            "grMainSetupMakeGround",
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
            "llGRYosterSmallMapFileID",
            "llGRYosterSmallMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0xA88, 15, 4,
         "grDisplayLayer0PriProcDisplay"),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x37F0, 3, 6,
         "grDisplayLayer1PriProcDisplay"),
        (OWNER_LAYER2, "layer2", "stage_geometry", 0x3CA8, 3, 13,
         "grDisplayLayer2PriProcDisplay"),
        (OWNER_LAYER3, "layer3", "stage_geometry", 0x4498, 4, 17,
         "grDisplayLayer3PriProcDisplay"),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 13, 0, 21),
        (OWNER_LAYER1, 6, 13, 1, 21, 16),
        (OWNER_LAYER2, 13, 14, 1, 37, 2),
        (OWNER_LAYER3, 17, 15, 2, 39, 11),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0PriProcDisplay", 4),
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
        ("layer2", "grDisplayLayer2PriProcDisplay", 13),
        ("layer3", "grDisplayLayer3PriProcDisplay", 17),
    ),
    segment0={},
    adapter_segment_count=4,
    adapter_dobj_count=21,
    adapter_binding_count=17,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x6E, 0x76, 0x10E),
    adapter_asset_sizes=(0x5230, 0x4550, 0x00C0),
)
