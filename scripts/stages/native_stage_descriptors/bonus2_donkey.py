"""Board the Platforms: Donkey board static map layers, decoded from source tables.

1P gkind 31 (nGRKindBonus2Donkey, include/sc/scene.h:777).
285_GRBonus2DonkeyMap.c wires gr_desc[0] to file 139's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 3, so both layers draw under their Sec procs (DLLinks,
grdisplay.c dGRDisplayDescs links 4/6). File 139 supplies
``dGRBonus2DonkeyFile2_Layer0DObj @ 0x848`` (11 entries sentinel-included:
DObj 0 null, 9 DLLink-backed bindings) and
``dGRBonus2DonkeyFile2_Layer1DObj @ 0x2D90`` (15 entries: DObj 0 null,
3 DLLink-backed bindings, heads 0 only). Layer0 links 4/Sec, layer1
links 6/Sec per dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 139's extern fixups all
target file 120 (Bonus1CommonImages1), so stage_images is a third packet
input with asset flag 2 -- same shape as bonus1_mario's file 124.

EXCLUDED (runtime-composed actors, not packet geometry):
- The platforms (small/medium/large plus boarded variants), composed at
  runtime by sc1PBonusStageInitPlatforms/MakePlatforms
  (decomp sc/sc1pmode/sc1pbonusstage.c:539-598) from
  dSC1PBonusStagePlatformDescs (:183) and
  dSC1PBonusStageBoardedPlatformDescs (:211), run only under
  sc1PBonusStageInitBonus2 (:733-739).
- No bumpers on this board: Donkey's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0} (:121-125); bumpers only exist on Bonus2 Fox/Samus/Kirby/
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
    name="bonus2_donkey",
    include_sha="e8d1e2e67f037bfcf4b9fdc325012700df4e07567d58b3e9122c1fe7323928dc",
    generated_segment_index=-1,
    symbol_prefix="Bonus2Donkey",
    macro_prefix="BONUS2DONKEY_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 24,
        "bindings": 12,
        "commands": 381,
        "vertex_commands": 34,
        "source_vertices": 540,
        "modify_vertex_commands": 0,
        "triangle_commands": 154,
        "triangles": 304,
        "runs": 37,
        "texture_epochs": 29,
        "material_events": 0,
        "submit_classes": (26, 50, 228),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 106,
        "state_deltas": 69,
        "sync_events": 71,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank139",
            "sha256": "c23d14c19cb9c22358ab00ce8a4f0a5df7416d2742f05ed9ce6944caa9d09348",
            "file_id": 139,
            "internal_fixups": 71,
            "external_fixups": 16,
            "payload_sha256": "0a8625c190a4a8e7584a98edd34ff4079677c0318da8f12eaee570bbad48b869",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus2DonkeyMap",
            "sha256": "dbedec42f4d0954e8fb322bb2bbccc2c90929601cb0c24c63dbf5232da40bc9f",
            "file_id": 285,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "74e363e2d7e120ef76040fdeb26b5bcda528f3b27bea036275bc7996a590d27f",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/139_GRBonus2DonkeyFile2.c",
            "sha256": "6d5e97352d508ccc2abe04ef3234d1d12d328286fb0be199bd1bae84b825e692",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/285_GRBonus2DonkeyMap.c",
            "sha256": "4f07fc503c87b3635bbefeaf309efd69f509f5f5ccb818a62fb60b570e79faf9",
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
            "MPGroundData dGRBonus2DonkeyMap_header",
            "dGRBonus2DonkeyFile2_Layer0DObj",
            "dGRBonus2DonkeyFile2_Layer1DObj",
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
            "llGRBonus2DonkeyMapFileID",
            "llGRBonus2DonkeyMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0x848, 11, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x2D90, 15, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 9, 0, 11),
        (OWNER_LAYER1, 6, 9, 3, 11, 26),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=24,
    adapter_binding_count=12,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x78, 0x8B, 0x11D),
    adapter_asset_sizes=(0x0A70, 0x35C0, 0x00B0),
)
