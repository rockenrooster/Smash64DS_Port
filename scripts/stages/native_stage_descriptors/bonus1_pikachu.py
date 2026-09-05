"""Break the Targets: Pikachu board static map layers, decoded from source tables.

1P gkind 26 (nGRKindBonus1Pikachu, include/sc/scene.h:770).
280_GRBonus1PikachuMap.c wires gr_desc[0] to file 133's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 1, so layer0 draws under the Sec proc (DLLinks) and layer1
under the Pri proc (grdisplay.c:193-197). File 133 supplies
``dGRBonus1PikachuFile2_Layer0DObj @ 0xA60`` (7 entries sentinel-included:
DObj 0 null, 5 DLLink-backed bindings) and
``dGRBonus1PikachuFile2_Layer1DObj @ 0x2658`` (4 entries: DObj 0 null,
2 direct-DL bindings). Layer0 links 4/Sec, layer1 link 6/Pri per
dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 133's extern fixups all
target file 122 (Bonus1CommonImages3), so stage_images is a third packet
input with asset flag 2 -- same shape as bonus1_mario's file 120 slot.

EXCLUDED (runtime-composed actors, not packet geometry):
- The 10 targets, composed at runtime by sc1PBonusStageMakeTargets
  (decomp sc/sc1pmode/sc1pbonusstage.c:434), called from
  sc1PBonusStageMakeBonus1Ground (:507-510). Descs are Pikachu's row of
  dSC1PBonusStageTargetDescs (:19-26); the count is enforced ==
  SCBATTLE_BONUSGAME_TASK_MAX (:461-466, the while(TRUE) hang on mismatch).
- No bumpers on this board: Pikachu's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0}; bumpers only exist on Bonus2 Fox/Samus/Kirby/
  Purin/Ness boards (:115-180).
- No Bonus2 platforms: sc1PBonusStageInitPlatforms/MakePlatforms
  (:539-598) run only under sc1PBonusStageInitBonus2 (:733-739).
- File 133's trailing ``Layer1Anim_DObjDesc[12]`` NULL-display-list
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
    name="bonus1_pikachu",
    include_sha="TO_BE_FILLED",
    generated_segment_index=-1,
    symbol_prefix="Bonus1Pikachu",
    macro_prefix="BONUS1PIKACHU_",
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank122",
            "sha256": "46306218319c1ed47b86c4733c411a1e3b37cb571d764fb38eafe064d66d29bc",
            "file_id": 122,
            "internal_fixups": 0,
            "external_fixups": 0,
            "payload_sha256": "e60535bd5d190f560e5cbe418f025c17d9fc40a945e059ea95808a19a9832d4e",
        },
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank133",
            "sha256": "b6d3491a566fc1669b297a9878a43ba2331f2cdbcb8fd066fee6ef54e841f7e0",
            "file_id": 133,
            "internal_fixups": 59,
            "external_fixups": 16,
            "payload_sha256": "993f3156ea1944c17c873eb4c5ed741e258edd84a33947d757cc5fa8da0f0976",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus1PikachuMap",
            "sha256": "2289ce939892cf8d49814d2e6b3fa5d877ee07f705f3192c22f4fbf37078e037",
            "file_id": 280,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "f046ca8654c5d0499c2b2a43d498bae4bc7d8a592248eb7875709ffb5189ce44",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/133_GRBonus1PikachuFile2.c",
            "sha256": "de469e636e5b5a8dcc2dbc60096705eb4068940a044e010a06c613e28ca91f77",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/280_GRBonus1PikachuMap.c",
            "sha256": "d62dbff5ebfad4e64bea9315fa16a8fbda3af9daa35df6843fe8af1b0da8eece",
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
            "MPGroundData dGRBonus1PikachuMap_header",
            "dGRBonus1PikachuFile2_Layer0DObj",
            "dGRBonus1PikachuFile2_Layer1DObj",
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
            "llGRBonus1PikachuMapFileID",
            "llGRBonus1PikachuMapMapHeader",
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
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x2658, 4, 6,
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
    adapter_asset_ids=(0x7A, 0x85, 0x118),
    adapter_asset_sizes=(0x03D0, 0x2CE0, 0x00B0),
)
