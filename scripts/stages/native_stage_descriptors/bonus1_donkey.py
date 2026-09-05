"""Break the Targets: Donkey board static map layers, decoded from source tables.

1P gkind 19 (nGRKindBonus1Donkey, include/sc/scene.h:763).
273_GRBonus1DonkeyMap.c wires gr_desc[0] to file 126's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 1, so layer0 draws under the Sec proc (DLLinks) and layer1
under the Pri proc (grdisplay.c:193-197). File 126 supplies
``dGRBonus1DonkeyFile2_Layer0DObj @ 0x840`` (5 entries sentinel-included:
DObj 0 null, 3 DLLink-backed bindings) and
``dGRBonus1DonkeyFile2_Layer1DObj @ 0x1F20`` (5 entries: DObj 0 null,
3 direct-DL bindings). Layer0 links 4/Sec, layer1 link 6/Pri per
dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 126's extern fixups all
target file 120 (Bonus1CommonImages1), so stage_images is a third packet
input with asset flag 2 -- same shape as bonus1_mario's file 124.

EXCLUDED (runtime-composed actors, not packet geometry):
- The 10 targets, composed at runtime by sc1PBonusStageMakeTargets
  (decomp sc/sc1pmode/sc1pbonusstage.c:434), called from
  sc1PBonusStageMakeBonus1Ground (:507-510). Descs are Donkey's row of
  dSC1PBonusStageTargetDescs (:19-26); the count is enforced ==
  SCBATTLE_BONUSGAME_TASK_MAX (:461-466, the while(TRUE) hang on mismatch).
- No bumpers on this board: Donkey's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0}; bumpers only exist on Bonus2 Fox/Samus/Kirby/
  Purin/Ness boards (:115-180).
- No Bonus2 platforms: sc1PBonusStageInitPlatforms/MakePlatforms
  (:539-598) run only under sc1PBonusStageInitBonus2 (:733-739).
- File 126's trailing ``Layer1Anim_DObjDesc[12]`` NULL-display-list
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
    name="bonus1_donkey",
    include_sha="524b7ab5989e3a902b37efa433e2b507ea6095ebbb8e2243347ceef4848b1720",
    generated_segment_index=-1,
    symbol_prefix="Bonus1Donkey",
    macro_prefix="BONUS1DONKEY_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 8,
        "bindings": 6,
        "commands": 296,
        "vertex_commands": 35,
        "source_vertices": 340,
        "modify_vertex_commands": 0,
        "triangle_commands": 85,
        "triangles": 170,
        "runs": 35,
        "texture_epochs": 14,
        "material_events": 0,
        "submit_classes": (40, 38, 92),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 114,
        "state_deltas": 58,
        "sync_events": 54,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank126",
            "sha256": "c99f0481f401f64f66572cb2841cce4020b312590261bf8f9b9e15097839bd86",
            "file_id": 126,
            "internal_fixups": 56,
            "external_fixups": 14,
            "payload_sha256": "3a39f794d79b23bea418c480650a6ed9a6f8d46f60fe2599a6d27db8366c2ed2",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus1DonkeyMap",
            "sha256": "b8be552ab9364553933064ef12e5c88dde8c90dfb44a6a3bb5aec7be1ae37931",
            "file_id": 273,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "aa67387909067acd16fad8561e8683afe2433a307ca17614b5a154ce5f8dcedb",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/126_GRBonus1DonkeyFile2.c",
            "sha256": "1f9e8706f11c98bdb155ed6f92939ef85cb4f7dab638c2049b63e923333321e0",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/273_GRBonus1DonkeyMap.c",
            "sha256": "dba89ece0ad79b69ebdfbee43f47282d8abda32a49b0b8ffe1b1d8d2d93df007",
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
            "MPGroundData dGRBonus1DonkeyMap_header",
            "dGRBonus1DonkeyFile2_Layer0DObj",
            "dGRBonus1DonkeyFile2_Layer1DObj",
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
            "llGRBonus1DonkeyMapFileID",
            "llGRBonus1DonkeyMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0x840, 5, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x1F20, 5, 6,
         "grDisplayLayer1PriProcDisplay", False),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 3, 0, 19),
        (OWNER_LAYER1, 6, 3, 3, 19, 16),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=8,
    adapter_binding_count=6,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x78, 0x7E, 0x111),
    adapter_asset_sizes=(0x0A70, 0x24B0, 0x00B0),
)
