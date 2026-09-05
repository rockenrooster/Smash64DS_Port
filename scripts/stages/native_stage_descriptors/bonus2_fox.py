"""Board the Platforms: Fox board static map layers, decoded from source tables.

1P gkind 30 (nGRKindBonus2Fox, include/sc/scene.h:776).
284_GRBonus2FoxMap.c wires gr_desc[0] to file 138's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, Layer1MObj_data and
Layer1MatAnim_MatAnimJoint, with layers 2/3 NULL; layer_mask is 3, so
both layers draw under their Sec procs (DLLinks, grdisplay.c
dGRDisplayDescs links 4/6). File 138 supplies
``dGRBonus2FoxFile2_Layer0DObj @ 0x950`` (6 entries: DObj 0 null,
4 DLLink-backed bindings) and ``dGRBonus2FoxFile2_Layer1DObj @ 0x2998``
(21 entries: DObj 0 null, 10 DLLink-backed bindings). Layer0 links 4/Sec,
layer1 links 6/Sec per dGRDisplayDescs (grdisplay.c:10-26).

Material shape: dGRBonus2FoxFile2_Layer1MObj_data feeds gcAddMObjAll;
four bindings enter segment-0xE slot 0 in their display lists -- DObj 11
(root 0x1CC0), DObj 17 (0x26A8), DObj 18 (0x2748), DObj 19 (0x27E0) --
paired with MObjSubs at 0xB10/0xB88/0xC00/0xC78 in DObj order. All four
carry flags 0x1 with no palette (three commands each).

Textures live outside the geometry bank: file 138's extern fixups all
target file 121, so stage_images is a third packet input with asset
flag 2 -- same shape as bonus1_mario's file 120.

EXCLUDED (runtime-composed actors, not packet geometry):
- The platforms (small/medium/large plus boarded variants), composed at
  runtime by sc1PBonusStageInitPlatforms/MakePlatforms
  (decomp sc/sc1pmode/sc1pbonusstage.c:539-598) from
  dSC1PBonusStagePlatformDescs (:183) and
  dSC1PBonusStageBoardedPlatformDescs (:211), run only under
  sc1PBonusStageInitBonus2 (:733-739).
- The bumpers, composed at runtime by sc1PBonusStageMakeBumpers
  (:700-731) from this board's dSC1PBonusStageBumperDescs row (:115-119)
  and the map file's BumpersDObjDesc @ 0xE160 / BumpersAnimJoint @ 0xE350
  templates (llGRBonus2FoxMapBumpersDObjDesc/AnimJoint).
- No targets on this board: sc1PBonusStageMakeTargets (:434) runs only
  under sc1PBonusStageMakeBonus1Ground (:507-510).

No per-stage gr* ground TU exists for this kind (decomp/src/gr holds no
grbonus2.c; the only grbonus TU is grbonus/grbonus3.c), so there is no
gr*Make* dynamic actor to exclude. The two static layers are composed by
grCommonSetupInitAll (grcommonsetup.c:25-28) via
grDisplayMakeGeometryLayer, which is the map-constructor anchor below.
Runtime packet views and native actors are still required before admission.
"""

from __future__ import annotations

from native_stage_descriptors import StageDescriptor

OWNER_LAYER0 = 0
OWNER_LAYER1 = 1

DESCRIPTOR = StageDescriptor(
    name="bonus2_fox",
    include_sha="705fb4e89ce3e1d5f1735e8f0ab0bc49aa7b062f52ab99976d4ebc173b1c7a71",
    generated_segment_index=-1,
    symbol_prefix="Bonus2Fox",
    macro_prefix="BONUS2FOX_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 25,
        "bindings": 14,
        "commands": 550,
        "vertex_commands": 49,
        "source_vertices": 475,
        "modify_vertex_commands": 0,
        "triangle_commands": 134,
        "triangles": 267,
        "runs": 58,
        "texture_epochs": 34,
        "material_events": 4,
        "submit_classes": (110, 40, 117),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 220,
        "state_deltas": 78,
        "sync_events": 127,
    },
    o2r_inputs={
        "stage_images": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank121",
            "sha256": "705821872537e401ea5fcd20bf8b87786e993cddf88b1bea93474e2b558050e3",
            "file_id": 121,
            "internal_fixups": 0,
            "external_fixups": 0,
            "payload_sha256": "fb54463474ffde000ab6a0e129f9d331c282e50a91db962af427126516a6be68",
        },
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank138",
            "sha256": "0e2f2616c82bb743b5a90c14df3b5ad32d1b94dab8c40ebbde3f09346ad89023",
            "file_id": 138,
            "internal_fixups": 167,
            "external_fixups": 58,
            "payload_sha256": "c9c11b0407b6fe2b4146077b2ff21e467f49c87b519a8a56d8899ae8331428bf",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus2FoxMap",
            "sha256": "42ce478804beee47130b87bd330be677302eb295ea72319e1dec5a7a13e4f5e1",
            "file_id": 284,
            "internal_fixups": 0,
            "external_fixups": 8,
            "payload_sha256": "ee88ce0b5731bede74387d1e9839dc83ef69b8eab52a392b3937688211dcd696",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/138_GRBonus2FoxFile2.c",
            "sha256": "5ef0e1c5185b55e9c1d089f77208d820e7ac9ccde9ea1b106b9b88ae759899ac",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/284_GRBonus2FoxMap.c",
            "sha256": "5d9df909428ca13c69ce6d6d7cddb591ecffafd5a8ebc2d78aed806e57828203",
        },
        "ground": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommonsetup.c",
            "sha256": "12a3486f0c9a5d979b13f3c05da26613b7bed6689870af612a258f90e2677455",
        },
        "bonus": {
            "path": "decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstage.c",
            "sha256": "22654f98b072fc3d32425ff3ae8ddc5f52290d6fcbcd166243cce9947776f827",
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
            "MPGroundData dGRBonus2FoxMap_header",
            "dGRBonus2FoxFile2_Layer0DObj",
            "dGRBonus2FoxFile2_Layer1DObj",
            "dGRBonus2FoxFile2_Layer1MObj_data",
        ),
        "ground": (
            "grCommonSetupInitAll",
            "grDisplayMakeGeometryLayer",
            "grMainSetupMakeGround",
        ),
        "bonus": (
            "sc1PBonusStageMakeBonus2Ground",
            "sc1PBonusStageInitBonus2",
            "dSC1PBonusStagePlatformDescs",
            "dSC1PBonusStageBumperDescs",
        ),
        "grdisplay": (
            "grDisplayLayer0SecProcDisplay",
            "grDisplayLayer1SecProcDisplay",
            "gcDrawDObjTreeDLLinksForGObj",
            "gcDrawDObjTreeForGObj",
        ),
        "objanim": ("gcPlayAnimAll", "gcParseMObjMatAnimJoint"),
        "objdisplay": (
            "void gcDrawMObjForDObj",
            "gSPSegment(dl_head[0]++, 0xE",
            "void gcDrawDObjTreeForGObj",
        ),
        "reloc_symbols": (
            "llGRBonus2FoxMapFileID",
            "llGRBonus2FoxMapMapHeader",
            "llGRBonus2FoxMapBumpersDObjDesc",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0x950, 6, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x2998, 21, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    # (asset_id, binding_root, mobj_offset, segment_index)
    material_sources=(
        (138, 0x1CC0, 0x0B10, 0x00),
        (138, 0x26A8, 0x0B88, 0x00),
        (138, 0x2748, 0x0C00, 0x00),
        (138, 0x27E0, 0x0C78, 0x00),
    ),
    material_command_partition=(3, 3, 3, 3),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 4, 0, 20),
        (OWNER_LAYER1, 6, 4, 10, 20, 38),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=25,
    adapter_binding_count=14,
    adapter_asset_count=3,
    adapter_material_count=4,
    adapter_asset_ids=(0x79, 0x8A, 0x11C),
    adapter_asset_sizes=(0x07F0, 0x10C10, 0x00B0),
)
