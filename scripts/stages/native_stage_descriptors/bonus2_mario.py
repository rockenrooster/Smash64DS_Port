"""Board the Platforms: Mario board static map layers, decoded from source tables.

1P gkind 29 (nGRKindBonus2Mario = nGRKindBonus2Start, include/sc/scene.h:775).
283_GRBonus2MarioMap.c wires gr_desc[0] to file 137's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 3, so both layers draw under their Sec procs (DLLinks,
grdisplay.c dGRDisplayDescs links 4/6). File 137 supplies
``dGRBonus2MarioFile2_Layer0DObj @ 0x9D0`` (6 entries sentinel-included:
DObj 0 null, 4 DLLink-backed bindings, heads 0/1) and
``dGRBonus2MarioFile2_Layer1DObj @ 0x32F0`` (16 entries: DObj 0 null,
5 DLLink-backed bindings on DObjs 11-14). Layer0 links 4/Sec, layer1
links 6/Sec per dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 137's extern fixups all
target file 120 (Bonus1CommonImages1), so stage_images is a third packet
input with asset flag 2 -- same shape as bonus1_mario's file 124.

EXCLUDED (runtime-composed actors, not packet geometry):
- The platforms (small/medium/large plus boarded variants), composed at
  runtime by sc1PBonusStageInitPlatforms/MakePlatforms
  (decomp sc/sc1pmode/sc1pbonusstage.c:539-598) from
  dSC1PBonusStagePlatformDescs (:183) and
  dSC1PBonusStageBoardedPlatformDescs (:211), run only under
  sc1PBonusStageInitBonus2 (:733-739).
- The bumpers: Mario's dSC1PBonusStageBumperDescs row is {0x0, 0x0}
  (:109-113); bumpers only exist on Bonus2 Fox/Samus/Kirby/Purin/Ness
  boards (:115-180), spawned by sc1PBonusStageMakeBumpers (:700-731).
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
    name="bonus2_mario",
    include_sha="61dca032cbe53acb041c4b3dfd6c41592ea19b8b76f573fa6a5ac1ccecef225d",
    generated_segment_index=-1,
    symbol_prefix="Bonus2Mario",
    macro_prefix="BONUS2MARIO_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 20,
        "bindings": 9,
        "commands": 461,
        "vertex_commands": 44,
        "source_vertices": 559,
        "modify_vertex_commands": 0,
        "triangle_commands": 147,
        "triangles": 291,
        "runs": 45,
        "texture_epochs": 25,
        "material_events": 0,
        "submit_classes": (2, 40, 249),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 173,
        "state_deltas": 76,
        "sync_events": 86,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank137",
            "sha256": "012a1a2d0dc4a4b5bd6563b7de71e8ea446c92b1519f6198b71b90a2fe2f58ba",
            "file_id": 137,
            "internal_fixups": 79,
            "external_fixups": 22,
            "payload_sha256": "4ad1a83abe9b289a780d0a3397c7c9701453e4de0e5d45bb3626811df62247ae",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus2MarioMap",
            "sha256": "59fa85207fee968c2f46352438d14fe74411a1af651ad0702b2f725082613736",
            "file_id": 283,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "61a7279e18636588e8ce9fc651cbbc0d93ead7ff6d6a3daf4a47bcd55793a6fa",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/137_GRBonus2MarioFile2.c",
            "sha256": "258e4c218700d749ae3a3538c3e475554a8707286f2b79c2afa120ac9137fd26",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/283_GRBonus2MarioMap.c",
            "sha256": "b6f05c528655a54259484f660917c4378885fa99bb1b7468505b16cfccd64931",
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
            "MPGroundData dGRBonus2MarioMap_header",
            "dGRBonus2MarioFile2_Layer0DObj",
            "dGRBonus2MarioFile2_Layer1DObj",
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
            "llGRBonus2MarioMapFileID",
            "llGRBonus2MarioMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0x9D0, 6, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x32F0, 16, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 4, 0, 20),
        (OWNER_LAYER1, 6, 4, 5, 20, 25),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=20,
    adapter_binding_count=9,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x78, 0x89, 0x11B),
    adapter_asset_sizes=(0x0A70, 0x3D20, 0x00B0),
)
