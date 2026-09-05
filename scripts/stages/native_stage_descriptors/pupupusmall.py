"""Small Dream Land's three static map layers, decoded from the original tables.

1P gkind 9 (nGRKindPupupuSmall, grdef.h:25); 256_GRPupupuSmallMap.c sets
layer_mask 0 but wires only gr_desc[0]/[1]/[3] to file 101
(StagePupupuBeta1, the beta Dream Land build this map displays):
layer0 ``dStagePupupuBeta1_DObjDesc_0x0870[7]`` @ 0x0870 (6 live + the
sentinel; DObj 0 null, 5 DL-backed) with the AnimJoint table at 0x09B0,
layer1 ``dStagePupupuBeta1_DObjDesc_0x1258[3]`` @ 0x1258 (2 live,
1 binding), layer3 ``dStagePupupuBeta1_DObjDesc_0x1910[3]`` @ 0x1910
(2 live, 1 binding). gr_desc[2] is NULL: no layer2 owner, so this stage
has three callbacks, not four. Links are the standard dGRDisplayDescs
Pri rows (grdisplay.c:10-43): layer0 link 4, layer1 link 6, layer3 link
17. No DLLink layer, no MObj structs in file 101, so material_sources is
empty -- confirmed by the generator's fail-closed segment-E walk, which
accepts zero materials.

Textures live outside the geometry bank: file 101's 34 extern fixups all
target file 100 (StagePupupuBetaImages, ExternDataBank100), so
stage_images is a third packet input with asset flag 2.

No per-stage gr* ground TU exists for this kind (grmainsetup.c:11-22
lists only the nine VS kinds, and grpupupu.c's grPupupuMakeGround builds
the full Dream Land with Whispy, not this variant), so there is no
gr*Make* dynamic actor to exclude. The three static layers are composed
by grCommonSetupInitAll (grcommonsetup.c:25-28) via
grDisplayMakeGeometryLayer, which is the map-constructor anchor below.
Runtime packet views are still required before admission.
"""

from __future__ import annotations

from native_stage_descriptors import StageDescriptor

OWNER_LAYER0 = 0
OWNER_LAYER1 = 1
OWNER_LAYER3 = 3

DESCRIPTOR = StageDescriptor(
    name="pupupusmall",
    include_sha="bb01e2e7b351531b955b342ef1a9f187adcc907385a99b9854c1530236570603",
    generated_segment_index=-1,
    symbol_prefix="PupupuSmall",
    macro_prefix="PUPUPUSMALL_",
    expected_counts={
        "callbacks": 3,
        "dobjs": 10,
        "bindings": 7,
        "commands": 357,
        "vertex_commands": 27,
        "source_vertices": 160,
        "modify_vertex_commands": 0,
        "triangle_commands": 63,
        "triangles": 124,
        "runs": 21,
        "texture_epochs": 18,
        "material_events": 0,
        "submit_classes": (68, 56, 0),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 161,
        "state_deltas": 68,
        "sync_events": 97,
    },
    o2r_inputs={
        "stage_images": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank100",
            "sha256": "f4565c3cc2b67109dc2e89d7e36bb46494433b41259b597cec08ca1699161e78",
            "file_id": 100,
            "internal_fixups": 0,
            "external_fixups": 0,
            "payload_sha256": "6e686421863adfbb13b4abecd4a5ed46fe28d1deba845b6378b15c05d7e6065e",
        },
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank101",
            "sha256": "271c03bff22085126c1c74b9f7b7fa3dc20d4dc7c8f8d19e3518a7588018da77",
            "file_id": 101,
            "internal_fixups": 48,
            "external_fixups": 34,
            "payload_sha256": "d3a8d194027857751276bf41c4c297877497f16537cd546a9eb9efdfa106b596",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRPupupuSmallMap",
            "sha256": "1fd1c4af660d5d18a748b424df42c9e554504fd4fc12609024f05e4581addebc",
            "file_id": 256,
            "internal_fixups": 1,
            "external_fixups": 6,
            "payload_sha256": "3106405b9bdc4b5d589132ce76c4a6ab66510cfdf7fd1920a88e5db13d01ecc9",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/101_StagePupupuBeta1.c",
            "sha256": "ea8901a1c13b5ae00a65f25de055f650b0c95d16464c048c640ec3a9c6b513a9",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/256_GRPupupuSmallMap.c",
            "sha256": "53ad1d6abe89f94c77e0c68d1734dd4b370e6a277af5790b2e2a1a78d33a8b0d",
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
            "MPGroundData dGRPupupuSmallMap_header",
            "dStagePupupuBeta1_DObjDesc_0x0870",
            "dStagePupupuBeta1_DObjDesc_0x1258",
            "dStagePupupuBeta1_DObjDesc_0x1910",
        ),
        "ground": (
            "grCommonSetupInitAll",
            "grDisplayMakeGeometryLayer",
            "grMainSetupMakeGround",
        ),
        "grdisplay": (
            "grDisplayLayer0PriProcDisplay",
            "grDisplayLayer1PriProcDisplay",
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
            "llGRPupupuSmallMapFileID",
            "llGRPupupuSmallMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0x0870, 7, 4,
         "grDisplayLayer0PriProcDisplay"),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x1258, 3, 6,
         "grDisplayLayer1PriProcDisplay"),
        (OWNER_LAYER3, "layer3", "stage_geometry", 0x1910, 3, 17,
         "grDisplayLayer3PriProcDisplay"),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 5, 0, 12),
        (OWNER_LAYER1, 6, 5, 1, 12, 4),
        (OWNER_LAYER3, 17, 6, 1, 16, 5),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0PriProcDisplay", 4),
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
        ("layer3", "grDisplayLayer3PriProcDisplay", 17),
    ),
    segment0={},
    adapter_segment_count=3,
    adapter_dobj_count=10,
    adapter_binding_count=7,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x64, 0x65, 0x100),
    adapter_asset_sizes=(0x27C0, 0x19A0, 0x00C0),
)
