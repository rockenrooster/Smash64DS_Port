"""Break the Targets: Mario board static map layers, decoded from source tables.

1P gkind 17 (nGRKindBonus1Mario = nGRKindBonus1Start, include/sc/scene.h:755).
271_GRBonus1MarioMap.c wires gr_desc[0] to file 124's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 1, so layer0 draws under the Sec proc (DLLinks) and layer1
under the Pri proc (grdisplay.c:193-197). File 124 supplies
``dGRBonus1MarioFile2_Layer0DObj @ 0xA60`` (7 entries sentinel-included:
DObj 0 null, 5 DLLink-backed bindings) and
``dGRBonus1MarioFile2_Layer1DObj @ 0x1EB0`` (4 entries: DObj 0 null,
2 direct-DL bindings). Layer0 links 4/Sec, layer1 link 6/Pri per
dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 124's extern fixups all
target file 120 (Bonus1CommonImages1), so stage_images is a third packet
input with asset flag 2 -- same shape as yostersmall's file 110.

EXCLUDED (runtime-composed actors, not packet geometry):
- The 10 targets, composed at runtime by sc1PBonusStageMakeTargets
  (decomp sc/sc1pmode/sc1pbonusstage.c:434), called from
  sc1PBonusStageMakeBonus1Ground (:507-510). Descs are Mario's row of
  dSC1PBonusStageTargetDescs (:19-26); the count is enforced ==
  SCBATTLE_BONUSGAME_TASK_MAX (:461-466, the while(TRUE) hang on mismatch).
- No bumpers on this board: Mario's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0} (:109-113); bumpers only exist on Bonus2 Fox/Samus/Kirby/
  Purin/Ness boards (:115-180).
- No Bonus2 platforms: sc1PBonusStageInitPlatforms/MakePlatforms
  (:539-598) run only under sc1PBonusStageInitBonus2 (:733-739).
- File 124's trailing ``Layer1Anim_DObjDesc[12]`` NULL-display-list
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
    name="bonus1_mario",
    include_sha="cd624f001f1189c799eb164ec1d27dd872734c7b3dd6db83caa16956e4054ce2",
    generated_segment_index=-1,
    symbol_prefix="Bonus1Mario",
    macro_prefix="BONUS1MARIO_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 9,
        "bindings": 7,
        "commands": 348,
        "vertex_commands": 43,
        "source_vertices": 326,
        "modify_vertex_commands": 0,
        "triangle_commands": 83,
        "triangles": 166,
        "runs": 35,
        "texture_epochs": 17,
        "material_events": 0,
        "submit_classes": (12, 42, 112),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 144,
        "state_deltas": 52,
        "sync_events": 69,
    },
    o2r_inputs={
        "stage_images": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank120",
            "sha256": "22cc1cbf74033fa043553c10308e9f5dd3cac515e03ae121c9a6032269702cef",
            "file_id": 120,
            "internal_fixups": 0,
            "external_fixups": 0,
            "payload_sha256": "984258037050ec17643d3b5b876c4083cfb8741f435bb25158fa3d3caa5334ef",
        },
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank124",
            "sha256": "484bd441e7320c2f2bfedfa024271f257ab39e52e19c9fa64280c89e84be10cd",
            "file_id": 124,
            "internal_fixups": 65,
            "external_fixups": 17,
            "payload_sha256": "71c04d0dc50dbae8a0453e54469f79224320a87f3be281748e69a8add4ca1b40",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus1MarioMap",
            "sha256": "541bbec8e77d962d892296a8ad7f69b96199b8fe2fdcfa1065f69066c6770717",
            "file_id": 271,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "bc75e0bee920e74c13bfb98249b0e225c288af65f6e8e3e951fffdb71852655f",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/124_GRBonus1MarioFile2.c",
            "sha256": "78fb91820c7599e7d0c16e9d65489c8e1ce6724ceada9f22f98cf94ced659a94",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/271_GRBonus1MarioMap.c",
            "sha256": "a22bd682077aadbaaf6dac2e8bf521506c20f084a809b57ecd0570f907a1813b",
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
            "MPGroundData dGRBonus1MarioMap_header",
            "dGRBonus1MarioFile2_Layer0DObj",
            "dGRBonus1MarioFile2_Layer1DObj",
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
            "llGRBonus1MarioMapFileID",
            "llGRBonus1MarioMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0xA60, 7, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x1EB0, 4, 6,
         "grDisplayLayer1PriProcDisplay", False),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 5, 0, 21),
        (OWNER_LAYER1, 6, 5, 2, 21, 14),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=9,
    adapter_binding_count=7,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x78, 0x7C, 0x10F),
    adapter_asset_sizes=(0x0A70, 0x23C0, 0x00B0),
)
