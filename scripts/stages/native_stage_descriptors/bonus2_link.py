"""Board the Platforms: Link board static map layers, decoded from source tables.

1P gkind 34 (nGRKindBonus2Link, include/sc/scene.h:780).
288_GRBonus2LinkMap.c wires gr_desc[0] to file 142's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 3, so both layers draw under their Sec procs (DLLinks,
grdisplay.c dGRDisplayDescs links 4/6). File 142 supplies
``dGRBonus2LinkFile2_Layer0DObj @ 0x938`` (13 entries sentinel-included:
DObj 0 null, 11 DLLink-backed bindings) and
``dGRBonus2LinkFile2_Layer1DObj @ 0x3C28`` (19 entries: DObj 0 null,
7 DLLink-backed bindings, heads 0 only). Layer0 links 4/Sec, layer1
links 6/Sec per dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 142's extern fixups all
target file 123, so stage_images is a third packet input with asset
flag 2 -- same shape as bonus1_mario's file 120.

EXCLUDED (runtime-composed actors, not packet geometry):
- The platforms (small/medium/large plus boarded variants), composed at
  runtime by sc1PBonusStageInitPlatforms/MakePlatforms
  (decomp sc/sc1pmode/sc1pbonusstage.c:539-598) from
  dSC1PBonusStagePlatformDescs (:183) and
  dSC1PBonusStageBoardedPlatformDescs (:211), run only under
  sc1PBonusStageInitBonus2 (:733-739).
- No bumpers on this board: Link's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0} (:139-143); bumpers only exist on Bonus2 Fox/Samus/Kirby/
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
    name="bonus2_link",
    include_sha="3ed2517f2407f2837ddaf60a740ef72524cf27fd80b0b758c15317d4a7cfa5b3",
    generated_segment_index=-1,
    symbol_prefix="Bonus2Link",
    macro_prefix="BONUS2LINK_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 30,
        "bindings": 18,
        "commands": 548,
        "vertex_commands": 55,
        "source_vertices": 713,
        "modify_vertex_commands": 0,
        "triangle_commands": 199,
        "triangles": 396,
        "runs": 58,
        "texture_epochs": 45,
        "material_events": 0,
        "submit_classes": (68, 54, 274),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 159,
        "state_deltas": 69,
        "sync_events": 113,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank142",
            "sha256": "2bc73553809ede9312ac64786b79f9dfcce9f3ac8c2a773c2cfc84424069c051",
            "file_id": 142,
            "internal_fixups": 116,
            "external_fixups": 29,
            "payload_sha256": "8e80bc66d289c80c005e64ca41cc71939026bb7e701bfab5071570ab7b2aa182",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus2LinkMap",
            "sha256": "593779d9f75ab1287f73d3130fe2faae59cb4a7283e15f9732129dee8aa16c68",
            "file_id": 288,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "636490942e113b343f454549a9694535ca13f190c51e942c22c9bd6ab5be0c23",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/142_GRBonus2LinkFile2.c",
            "sha256": "a0f7554b39fd0bd8bcafe59b950f9ed21a48b22895807c988ef7e808e18dfd58",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/288_GRBonus2LinkMap.c",
            "sha256": "030cd2b798249a67947e6b0dd3705b59a0b03ed33759146bd018fe66f72ab103",
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
            "MPGroundData dGRBonus2LinkMap_header",
            "dGRBonus2LinkFile2_Layer0DObj",
            "dGRBonus2LinkFile2_Layer1DObj",
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
            "llGRBonus2LinkMapFileID",
            "llGRBonus2LinkMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0x938, 13, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x3C28, 19, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 11, 0, 13),
        (OWNER_LAYER1, 6, 11, 7, 13, 45),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=30,
    adapter_binding_count=18,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x7B, 0x8E, 0x120),
    adapter_asset_sizes=(0x0BD0, 0x4950, 0x00B0),
)
