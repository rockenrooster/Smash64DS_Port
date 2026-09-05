"""Break the Targets: Samus board static map layers, decoded from source tables.

1P gkind 20 (nGRKindBonus1Samus, include/sc/scene.h:764).
274_GRBonus1SamusMap.c wires gr_desc[0] to file 127's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 1, so layer0 draws under the Sec proc (DLLinks) and layer1
under the Pri proc (grdisplay.c:193-197). File 127 supplies
``dGRBonus1SamusFile2_Layer0DObj @ 0x950`` (6 entries sentinel-included:
DObj 0 null, 4 DLLink-backed bindings) and
``dGRBonus1SamusFile2_Layer1DObj @ 0x1868`` (4 entries: DObj 0 null,
2 direct-DL bindings). Layer0 links 4/Sec, layer1 link 6/Pri per
dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 127's extern fixups all
target file 121 (Bonus1CommonImages2), so stage_images is a third packet
input with asset flag 2 -- same shape as bonus1_mario's file 120 slot.

EXCLUDED (runtime-composed actors, not packet geometry):
- The 10 targets, composed at runtime by sc1PBonusStageMakeTargets
  (decomp sc/sc1pmode/sc1pbonusstage.c:434), called from
  sc1PBonusStageMakeBonus1Ground (:507-510). Descs are Samus's row of
  dSC1PBonusStageTargetDescs (:19-26); the count is enforced ==
  SCBATTLE_BONUSGAME_TASK_MAX (:461-466, the while(TRUE) hang on mismatch).
- No bumpers on this board: Samus's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0}; bumpers only exist on Bonus2 Fox/Samus/Kirby/
  Purin/Ness boards (:115-180).
- No Bonus2 platforms: sc1PBonusStageInitPlatforms/MakePlatforms
  (:539-598) run only under sc1PBonusStageInitBonus2 (:733-739).
- File 127's trailing ``Layer1Anim_DObjDesc[12]`` NULL-display-list
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
    name="bonus1_samus",
    include_sha="9930759393dc8f7db78b4133b79df04405c94fc7d303b285032b878947f2315f",
    generated_segment_index=-1,
    symbol_prefix="Bonus1Samus",
    macro_prefix="BONUS1SAMUS_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 8,
        "bindings": 6,
        "commands": 275,
        "vertex_commands": 32,
        "source_vertices": 240,
        "modify_vertex_commands": 0,
        "triangle_commands": 62,
        "triangles": 124,
        "runs": 40,
        "texture_epochs": 18,
        "material_events": 0,
        "submit_classes": (45, 40, 39),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 112,
        "state_deltas": 52,
        "sync_events": 61,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank127",
            "sha256": "78a73d2ca98c498132e7e9f9186f4f3a2dc6263432e6e7aa10454996a541e587",
            "file_id": 127,
            "internal_fixups": 52,
            "external_fixups": 14,
            "payload_sha256": "136628f3b079e291379f48a22ec1e7633f98fbb74a601bcfb1a88436092eed83",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus1SamusMap",
            "sha256": "4cbee4c3a71468274c47679040e3b389a5ebeaf87ebc4129704ea77df01d6f56",
            "file_id": 274,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "b79b78b61742f164b4171a875a21c502faf5372f660f6e41193146c8b74f8d5e",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/127_GRBonus1SamusFile2.c",
            "sha256": "35f1b813a7fe2448e51f392e3649bfb93cbfea95bdf36207f4d441f796b65368",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/274_GRBonus1SamusMap.c",
            "sha256": "1b6eddaa24ab1d35b355a1c5fbb8aab6ed880568bb536140795a09ee57f2536c",
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
            "MPGroundData dGRBonus1SamusMap_header",
            "dGRBonus1SamusFile2_Layer0DObj",
            "dGRBonus1SamusFile2_Layer1DObj",
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
            "llGRBonus1SamusMapFileID",
            "llGRBonus1SamusMapMapHeader",
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
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x1868, 4, 6,
         "grDisplayLayer1PriProcDisplay", False),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 4, 0, 20),
        (OWNER_LAYER1, 6, 4, 2, 20, 20),
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
    adapter_asset_ids=(0x79, 0x7F, 0x112),
    adapter_asset_sizes=(0x07F0, 0x1DA0, 0x00B0),
)
