"""Break the Targets: Fox board static map layers, decoded from source tables.

1P gkind 18 (nGRKindBonus1Fox, include/sc/scene.h:762).
272_GRBonus1FoxMap.c wires gr_desc[0] to file 125's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 1, so layer0 draws under the Sec proc (DLLinks) and layer1
under the Pri proc (grdisplay.c:193-197). File 125 supplies
``dGRBonus1FoxFile2_Layer0DObj @ 0xA60`` (7 entries sentinel-included:
DObj 0 null, 5 DLLink-backed bindings) and
``dGRBonus1FoxFile2_Layer1DObj @ 0x2068`` (6 entries: DObj 0 null,
5 direct-DL bindings). Layer0 links 4/Sec, layer1 link 6/Pri per
dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 125's extern fixups all
target file 121 (Bonus1CommonImages2; Fox, Samus and Captain share it), so stage_images is a third packet
input with asset flag 2 -- same shape as bonus1_mario's file 124.

EXCLUDED (runtime-composed actors, not packet geometry):
- The 10 targets, composed at runtime by sc1PBonusStageMakeTargets
  (decomp sc/sc1pmode/sc1pbonusstage.c:434), called from
  sc1PBonusStageMakeBonus1Ground (:507-510). Descs are Fox's row of
  dSC1PBonusStageTargetDescs (:19-26); the count is enforced ==
  SCBATTLE_BONUSGAME_TASK_MAX (:461-466, the while(TRUE) hang on mismatch).
- No bumpers on this board: Fox's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0}; bumpers only exist on Bonus2 Fox/Samus/Kirby/
  Purin/Ness boards (:115-180).
- No Bonus2 platforms: sc1PBonusStageInitPlatforms/MakePlatforms
  (:539-598) run only under sc1PBonusStageInitBonus2 (:733-739).
- File 125's trailing ``Layer1Anim_DObjDesc[12]`` NULL-display-list
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
    name="bonus1_fox",
    include_sha="e1ba0be44fa787fa76814e0600cbc688e54df2901c2c1d205a31a1db6ab8e022",
    generated_segment_index=-1,
    symbol_prefix="Bonus1Fox",
    macro_prefix="BONUS1FOX_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 11,
        "bindings": 9,
        "commands": 347,
        "vertex_commands": 35,
        "source_vertices": 332,
        "modify_vertex_commands": 0,
        "triangle_commands": 86,
        "triangles": 172,
        "runs": 36,
        "texture_epochs": 21,
        "material_events": 0,
        "submit_classes": (31, 42, 99),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 144,
        "state_deltas": 55,
        "sync_events": 71,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank125",
            "sha256": "70a3379984f30abfbfb595298b6a30cee4e3af9d692b1fef79244c95f4d7a9c1",
            "file_id": 125,
            "internal_fixups": 67,
            "external_fixups": 16,
            "payload_sha256": "d988b9240f854e61d85665039cf87c022a00daeee93d65cb7903af8aee9f9155",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus1FoxMap",
            "sha256": "392bebb7ba3691b7cf5ba4ffcad6efb7f8be6e521608cf599451aa81b854ac4e",
            "file_id": 272,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "ed04d72fdeb66de076bc78640d3b2739e6c874964e20817cae0747b0952fd80f",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/125_GRBonus1FoxFile2.c",
            "sha256": "cfb7e4842aed06d879851569b16cd86f01bb6410337002a3b061a8ddbec92eef",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/272_GRBonus1FoxMap.c",
            "sha256": "eb5b6413b4d67613617f87fbd7557fe26cc7a1003f0aa13c7f770dc032d98019",
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
            "MPGroundData dGRBonus1FoxMap_header",
            "dGRBonus1FoxFile2_Layer0DObj",
            "dGRBonus1FoxFile2_Layer1DObj",
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
            "llGRBonus1FoxMapFileID",
            "llGRBonus1FoxMapMapHeader",
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
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x2068, 6, 6,
         "grDisplayLayer1PriProcDisplay", False),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=((0, 4, 0, 5, 0, 21), (1, 6, 5, 4, 21, 15)),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=11,
    adapter_binding_count=9,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x79, 0x7D, 0x110),
    adapter_asset_sizes=(0x07F0, 0x2760, 0x00B0),
)
