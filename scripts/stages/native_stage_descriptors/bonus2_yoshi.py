"""Board the Platforms: Yoshi board static map layers, decoded from source tables.

1P gkind 35 (nGRKindBonus2Yoshi, include/sc/scene.h:781).
289_GRBonus2YoshiMap.c wires gr_desc[0] to file 143's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 3, so both layers draw under their Sec procs (DLLinks,
grdisplay.c dGRDisplayDescs links 4/6). File 143 supplies
``dGRBonus2YoshiFile2_Layer0DObj @ 0xD40`` (9 entries sentinel-included:
DObj 0 null, 7 DLLink-backed bindings) and
``dGRBonus2YoshiFile2_Layer1DObj @ 0x2510`` (22 entries: DObj 0 null,
10 DLLink-backed bindings, heads 0 only). Layer0 links 4/Sec, layer1
links 6/Sec per dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 143's extern fixups all
target file 122, so stage_images is a third packet input with asset
flag 2 -- same shape as bonus1_mario's file 120.

EXCLUDED (runtime-composed actors, not packet geometry):
- The platforms (small/medium/large plus boarded variants), composed at
  runtime by sc1PBonusStageInitPlatforms/MakePlatforms
  (decomp sc/sc1pmode/sc1pbonusstage.c:539-598) from
  dSC1PBonusStagePlatformDescs (:183) and
  dSC1PBonusStageBoardedPlatformDescs (:211), run only under
  sc1PBonusStageInitBonus2 (:733-739).
- No bumpers on this board: Yoshi's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0} (:145-149); bumpers only exist on Bonus2 Fox/Samus/Kirby/
  Purin/Ness boards (:107-180), spawned by sc1PBonusStageMakeBumpers
  (:700-731).
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
    name="bonus2_yoshi",
    include_sha="b0d0ce1159d6bd964e3396b4d4ec7a5a1085012e9fe0a923ab81b207ea4eb558",
    generated_segment_index=-1,
    symbol_prefix="Bonus2Yoshi",
    macro_prefix="BONUS2YOSHI_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 29,
        "bindings": 17,
        "commands": 466,
        "vertex_commands": 40,
        "source_vertices": 426,
        "modify_vertex_commands": 0,
        "triangle_commands": 113,
        "triangles": 226,
        "runs": 44,
        "texture_epochs": 25,
        "material_events": 0,
        "submit_classes": (75, 46, 105),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 195,
        "state_deltas": 53,
        "sync_events": 99,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank143",
            "sha256": "3337d64c1b9278b9885d207a75e4e8b3224def28bac82bdf99f551a6621cb582",
            "file_id": 143,
            "internal_fixups": 122,
            "external_fixups": 25,
            "payload_sha256": "98fd9cd2701fb2f632c79718529ce24df5d34054b78c884c25a26b2818eefad3",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus2YoshiMap",
            "sha256": "dff57f59c0c69d9d2c2933dff7c340a2bec273c589efab0cee959aff2683a7de",
            "file_id": 289,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "d5dcc245f296601efdd3a07c3e8554e54d3c135e743eb26f087d52d820938001",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/143_GRBonus2YoshiFile2.c",
            "sha256": "ccbd0e00eea091df4e164f6144d300a254b9ad40741be36195633c72d3b283bf",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/289_GRBonus2YoshiMap.c",
            "sha256": "7dce988f27328b5aa0ccf92d4650f2a48cf3ab004a319b041bd4f182d11fadea",
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
            "MPGroundData dGRBonus2YoshiMap_header",
            "dGRBonus2YoshiFile2_Layer0DObj",
            "dGRBonus2YoshiFile2_Layer1DObj",
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
            "llGRBonus2YoshiMapFileID",
            "llGRBonus2YoshiMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0xD40, 9, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x2510, 22, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 7, 0, 23),
        (OWNER_LAYER1, 6, 7, 10, 23, 21),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=29,
    adapter_binding_count=17,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x7A, 0x8F, 0x121),
    adapter_asset_sizes=(0x03D0, 0x3810, 0x00B0),
)
