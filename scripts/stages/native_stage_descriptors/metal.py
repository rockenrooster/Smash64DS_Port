"""Meta Crystal's four static map layers, decoded from the original tables.

1P gkind 13 (nGRKindMetal, grdef.h:29); 269_GRMetalMap.c sets layer_mask 0
and wires all four gr_desc rows to file 117. File 117 supplies layers 0-3
at 0x1B10/0x2EE0/0x3468/0x3DD8 with 3/3/3/3 DObjDesc entries (2 live + the
sentinel each; DObj 0 has no display list, DObj 1 carries the layer DL).
Layer links/callbacks are the standard dGRDisplayDescs Pri rows
(grdisplay.c:10-43): links 4/6/13/17, layer_mask 0 so the primary procs
apply. No DLLink layer exists on this stage.

Materials: file 117 carries per-layer MObjSub tables plus per-layer
MatAnimJoint scripts, and all four layer DLs call into segment-E
material programs: DL_0x1708 calls segs 0/8/0x10/0x18/0x20, DL_0x2950
calls 0x10/8/0 (reverse order), DL_0x3368 calls 0, DL_0x3B50 calls
0/8/0x10/0x18. Pairing is file-grounded: the MObjSub tables hold 5/3/1/4
MObjSubs at 0xC38-0xE18 / 0x1E20+0x1ED0+0x1F48 / 0x3228 / 0x3558-0x36C0,
and segment 8*i matches gcDrawMObjForDObj's branch slot for MObj i --
thirteen material events, twelve of length 3 and one of length 2.

The battle wallpaper (StageMetalWallpaper o2r bank, file 98 sprite
dStageMetalBackground_0x26c88 referenced by 269_GRMetalMap.c:41) is a
2D sprite, not geometry, and is not a packet input -- same rule as Castle.

REGION_US: map item weights take the non-JP branch (269_GRMetalMap.c:27).

No per-stage gr* ground TU exists for this kind (grmainsetup.c:11-22
lists only the nine VS kinds; grMainSetupMakeGround at :31-49 never takes
a Metal arm), so there is no gr*Make* dynamic actor to exclude. The four
static layers are composed by grCommonSetupInitAll (grcommonsetup.c:25-28)
via grDisplayMakeGeometryLayer, which is the map-constructor anchor below.
Runtime packet views and native actors are still required before admission.
"""

from __future__ import annotations

from native_stage_descriptors import StageDescriptor

OWNER_LAYER0 = 0
OWNER_LAYER1 = 1
OWNER_LAYER2 = 2
OWNER_LAYER3 = 3

DESCRIPTOR = StageDescriptor(
    name="metal",
    include_sha="56476bf5ec999348d15aac5e4c3728bb6b41945efd6ef7328f405a79062da5bf",
    generated_segment_index=-1,
    symbol_prefix="Metal",
    macro_prefix="METAL_",
    expected_counts={
        "callbacks": 4,
        "dobjs": 8,
        "bindings": 4,
        "commands": 458,
        "vertex_commands": 29,
        "source_vertices": 362,
        "modify_vertex_commands": 0,
        "triangle_commands": 80,
        "triangles": 153,
        "runs": 33,
        "texture_epochs": 26,
        "material_events": 13,
        "submit_classes": (62, 81, 10),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 224,
        "state_deltas": 109,
        "sync_events": 108,
    },
    o2r_inputs={
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank117",
            "sha256": "e1b882308a723c49a1d491a4717531d8ec989a3cd0bec2667247f4759a0fd676",
            "file_id": 117,
            "internal_fixups": 151,
            "external_fixups": 0,
            "payload_sha256": "b29c29d47d4e84b2d7e90cecacffab8f224f7182a904a3d1ca761085a5675a9e",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRMetalMap",
            "sha256": "b3dd9bc4d5c31909e32dde1f760ff35e64c68084e8b913e64949dd21771b4236",
            "file_id": 269,
            "internal_fixups": 1,
            "external_fixups": 14,
            "payload_sha256": "ddf8e4e6b3f83ad559e7f085e0277f7a3e16a1f8c0eb0d2b3e96ca405fc90be1",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/117_StageMetalFile2.c",
            "sha256": "d543637d7bfea71df12683a2b63479a37ecab25164eb898121d15bff2948b274",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/269_GRMetalMap.c",
            "sha256": "6f20b793724cc391b6d2db67deb67cc5d9d11086daafbd3d66ac1cc8af133f3c",
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
            "MPGroundData dGRMetalMap_header",
            "dStageMetalFile2_Layer0DObj",
            "dStageMetalFile2_Layer1DObj",
            "dStageMetalFile2_Layer2DObj",
            "dStageMetalFile2_Layer3DObj",
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
            "llGRMetalMapFileID",
            "llGRMetalMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0x1B10, 3, 4,
         "grDisplayLayer0PriProcDisplay"),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x2EE0, 3, 6,
         "grDisplayLayer1PriProcDisplay"),
        (OWNER_LAYER2, "layer2", "stage_geometry", 0x3468, 3, 13,
         "grDisplayLayer2PriProcDisplay"),
        (OWNER_LAYER3, "layer3", "stage_geometry", 0x3DD8, 3, 17,
         "grDisplayLayer3PriProcDisplay"),
    ),
    # (asset_id, binding_root, mobj_offset, segment_index): one row per
    # MObjSub, in binding then segment order. Segment 8*i matches
    # gcDrawMObjForDObj's branch slot for MObj i of the owning DObj.
    # L0's five MObjSubs (0xC38..0xE18) feed DL_0x1708's five branches;
    # L1's three (0x1E20/0x1ED0/0x1F48) feed DL_0x2950 in reverse DL order;
    # L2's one (0x3228) feeds DL_0x3368; L3's four (0x3558..0x36C0) feed
    # DL_0x3B50.
    material_sources=(
        (117, 0x1708, 0x0C38, 0x00),
        (117, 0x1708, 0x0CB0, 0x08),
        (117, 0x1708, 0x0D28, 0x10),
        (117, 0x1708, 0x0DA0, 0x18),
        (117, 0x1708, 0x0E18, 0x20),
        (117, 0x2950, 0x1E20, 0x00),
        (117, 0x2950, 0x1ED0, 0x08),
        (117, 0x2950, 0x1F48, 0x10),
        (117, 0x3368, 0x3228, 0x00),
        (117, 0x3B50, 0x3558, 0x00),
        (117, 0x3B50, 0x35D0, 0x08),
        (117, 0x3B50, 0x3648, 0x10),
        (117, 0x3B50, 0x36C0, 0x18),
    ),
    material_command_partition=(3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 1, 0, 8),
        (OWNER_LAYER1, 6, 1, 1, 8, 20),
        (OWNER_LAYER2, 13, 2, 1, 28, 1),
        (OWNER_LAYER3, 17, 3, 1, 29, 4),
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
    adapter_dobj_count=8,
    adapter_binding_count=4,
    adapter_asset_count=2,
    adapter_material_count=13,
    adapter_asset_ids=(0x75, 0x10D),
    adapter_asset_sizes=(0x3FC0, 0x00C0),
)
