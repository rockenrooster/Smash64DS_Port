"""Break the Targets: Link board static map layers, decoded from source tables.

1P gkind 22 (nGRKindBonus1Link, include/sc/scene.h:766).
276_GRBonus1LinkMap.c wires gr_desc[0] to file 129's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 1, so layer0 draws under the Sec proc (DLLinks) and layer1
under the Pri proc (grdisplay.c:193-197). File 129 supplies
``dGRBonus1LinkFile2_Layer0DObj @ 0xC80`` (9 entries sentinel-included:
DObj 0 null, 7 DLLink-backed bindings) and
``dGRBonus1LinkFile2_Layer1DObj @ 0x2378`` (5 entries: DObj 0 null,
3 direct-DL bindings). Layer0 links 4/Sec, layer1 link 6/Pri per
dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 129's extern fixups all
target file 123 (Bonus1CommonImages4), so stage_images is a third packet
input with asset flag 2 -- same shape as bonus1_mario's file 120 slot.

EXCLUDED (runtime-composed actors, not packet geometry):
- The 10 targets, composed at runtime by sc1PBonusStageMakeTargets
  (decomp sc/sc1pmode/sc1pbonusstage.c:434), called from
  sc1PBonusStageMakeBonus1Ground (:507-510). Descs are Link's row of
  dSC1PBonusStageTargetDescs (:19-26); the count is enforced ==
  SCBATTLE_BONUSGAME_TASK_MAX (:461-466, the while(TRUE) hang on mismatch).
- No bumpers on this board: Link's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0}; bumpers only exist on Bonus2 Fox/Samus/Kirby/
  Purin/Ness boards (:115-180).
- No Bonus2 platforms: sc1PBonusStageInitPlatforms/MakePlatforms
  (:539-598) run only under sc1PBonusStageInitBonus2 (:733-739).
- File 129's trailing ``Layer1Anim_DObjDesc[12]`` NULL-display-list
  position markers are not referenced by the map header and carry no
  display list; they are not geometry.

No per-stage gr* ground TU exists for this kind (decomp/src/gr holds no
grbonus1.c; the only grbonus TU is grbonus/grbonus3.c), so there is no
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
    name="bonus1_link",
    include_sha="eb717114ef302e4af85df6c16d3f4068a6aa907fdebd98d1a5aefc13254bbf89",
    generated_segment_index=-1,
    symbol_prefix="Bonus1Link",
    macro_prefix="BONUS1LINK_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 12,
        "bindings": 10,
        "commands": 427,
        "vertex_commands": 40,
        "source_vertices": 342,
        "modify_vertex_commands": 0,
        "triangle_commands": 91,
        "triangles": 182,
        "runs": 44,
        "texture_epochs": 25,
        "material_events": 0,
        "submit_classes": (24, 46, 112),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 191,
        "state_deltas": 57,
        "sync_events": 93,
    },
    o2r_inputs={
        "stage_images": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank123",
            "sha256": "ddf7cdad8f3f560e8a9863f9b14af25840a2572a4f563ff78949631fba851c9c",
            "file_id": 123,
            "internal_fixups": 0,
            "external_fixups": 0,
            "payload_sha256": "063a1c5618a568a9791cc4294079761441d60cc89990799ff7b9018a8f0ed94b",
        },
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank129",
            "sha256": "5a0c6342fc1fceb82db78037aba99f19d35eb8842fed282b563680c31a3d572d",
            "file_id": 129,
            "internal_fixups": 69,
            "external_fixups": 22,
            "payload_sha256": "58b8aa8c54ec8fe1a75a80e9bc9d0e9893f33e106ff4a1494bc05175c835cdd0",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus1LinkMap",
            "sha256": "64e781c6ab34024bf40d61c23e17acf78c552476db52d80f9c77c3169c3af9c2",
            "file_id": 276,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "479fe8f2cbd596596baccbf8f55b0f077fa52bf48cd4d9aed7ce46b4dbdd6edf",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/129_GRBonus1LinkFile2.c",
            "sha256": "88b57c69280842acdb38f406837611a5e9686b0c87528ac22d690c9b1afcb39d",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/276_GRBonus1LinkMap.c",
            "sha256": "4591983c4b59c9f2c40868f7b1f691f66ce5888f83ede1ee725fdea1ec9c8c16",
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
            "MPGroundData dGRBonus1LinkMap_header",
            "dGRBonus1LinkFile2_Layer0DObj",
            "dGRBonus1LinkFile2_Layer1DObj",
        ),
        "ground": (
            "grCommonSetupInitAll",
            "grDisplayMakeGeometryLayer",
            "grMainSetupMakeGround",
        ),
        "bonus": (
            "sc1PBonusStageMakeBonus1Ground",
            "sc1PBonusStageMakeTargets",
            "dSC1PBonusStageTargetDescs",
            "SCBATTLE_BONUSGAME_TASK_MAX",
        ),
        "grdisplay": (
            "grDisplayLayer0SecProcDisplay",
            "grDisplayLayer1PriProcDisplay",
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
            "llGRBonus1LinkMapFileID",
            "llGRBonus1LinkMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0xC80, 9, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x2378, 5, 6,
         "grDisplayLayer1PriProcDisplay", False),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 7, 0, 23),
        (OWNER_LAYER1, 6, 7, 3, 23, 21),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=12,
    adapter_binding_count=10,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x7B, 0x81, 0x114),
    adapter_asset_sizes=(0x0BD0, 0x29E0, 0x00B0),
)
