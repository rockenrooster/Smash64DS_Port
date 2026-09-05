"""Break the Targets: Yoshi board static map layers, decoded from source tables.

1P gkind 23 (nGRKindBonus1Yoshi, include/sc/scene.h:767).
277_GRBonus1YoshiMap.c wires gr_desc[0] to file 130's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 1, so layer0 draws under the Sec proc (DLLinks) and layer1
under the Pri proc (grdisplay.c:193-197). File 130 supplies
``dGRBonus1YoshiFile2_Layer0DObj @ 0xC80`` (9 entries sentinel-included:
DObj 0 null, 7 DLLink-backed bindings) and
``dGRBonus1YoshiFile2_Layer1DObj @ 0x2D68`` (5 entries: DObj 0 null,
3 direct-DL bindings). Layer0 links 4/Sec, layer1 link 6/Pri per
dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 130's extern fixups all
target file 122 (Bonus1CommonImages3), so stage_images is a third packet
input with asset flag 2 -- same shape as bonus1_mario's file 120 slot.

EXCLUDED (runtime-composed actors, not packet geometry):
- The 10 targets, composed at runtime by sc1PBonusStageMakeTargets
  (decomp sc/sc1pmode/sc1pbonusstage.c:434), called from
  sc1PBonusStageMakeBonus1Ground (:507-510). Descs are Yoshi's row of
  dSC1PBonusStageTargetDescs (:19-26); the count is enforced ==
  SCBATTLE_BONUSGAME_TASK_MAX (:461-466, the while(TRUE) hang on mismatch).
- No bumpers on this board: Yoshi's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0}; bumpers only exist on Bonus2 Fox/Samus/Kirby/
  Purin/Ness boards (:115-180).
- No Bonus2 platforms: sc1PBonusStageInitPlatforms/MakePlatforms
  (:539-598) run only under sc1PBonusStageInitBonus2 (:733-739).
- File 130's trailing ``Layer1Anim_DObjDesc[12]`` NULL-display-list
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
    name="bonus1_yoshi",
    include_sha="a6780313ee0f67a6a99bed1d7218c38329823be9d60c8128f8c5641a3dbda1b6",
    generated_segment_index=-1,
    symbol_prefix="Bonus1Yoshi",
    macro_prefix="BONUS1YOSHI_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 12,
        "bindings": 10,
        "commands": 489,
        "vertex_commands": 46,
        "source_vertices": 496,
        "modify_vertex_commands": 0,
        "triangle_commands": 135,
        "triangles": 268,
        "runs": 41,
        "texture_epochs": 22,
        "material_events": 0,
        "submit_classes": (20, 46, 202),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 200,
        "state_deltas": 52,
        "sync_events": 96,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank130",
            "sha256": "4c9c8a34442d86065b13e5d9ac66c693da8c3100b47f3c176342a86c54083eb2",
            "file_id": 130,
            "internal_fixups": 75,
            "external_fixups": 25,
            "payload_sha256": "04b739091006920b85b3ad80396e46c50fd442d58fc9fcf034c312e89f03b1be",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus1YoshiMap",
            "sha256": "58a37f0b9a2d3fd181e4a358b4b904060a81db175f88ff5d0ff3419a2fc8c70b",
            "file_id": 277,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "7410222485baffc4c390245f15de9bb261aaa041435a2c88852781f97476bdf2",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/130_GRBonus1YoshiFile2.c",
            "sha256": "e14dead8be7cc8c35f615b3a76cf860af169a6675d8092f67c739e6317213ad6",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/277_GRBonus1YoshiMap.c",
            "sha256": "da886dd0b20551a6508017789f7acf0ce2d32e162823bf62cfe2e5b3287c294a",
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
            "MPGroundData dGRBonus1YoshiMap_header",
            "dGRBonus1YoshiFile2_Layer0DObj",
            "dGRBonus1YoshiFile2_Layer1DObj",
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
            "llGRBonus1YoshiMapFileID",
            "llGRBonus1YoshiMapMapHeader",
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
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x2D68, 5, 6,
         "grDisplayLayer1PriProcDisplay", False),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 7, 0, 23),
        (OWNER_LAYER1, 6, 7, 3, 23, 18),
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
    adapter_asset_ids=(0x7A, 0x82, 0x115),
    adapter_asset_sizes=(0x03D0, 0x34E0, 0x00B0),
)
