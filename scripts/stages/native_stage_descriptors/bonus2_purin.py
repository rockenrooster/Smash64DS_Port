"""Board the Platforms: Purin board static map layers, decoded from source tables.

1P gkind 39 (nGRKindBonus2Purin, include/sc/scene.h:785).
293_GRBonus2PurinMap.c wires gr_desc[0] to file 147's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 3, so both layers draw under their Sec procs (DLLinks,
grdisplay.c dGRDisplayDescs links 4/6). File 147 supplies
``dGRBonus2PurinFile2_Layer0DObj @ 0x840`` (5 entries sentinel-included:
DObj 0 null, 3 DLLink-backed bindings) and
``dGRBonus2PurinFile2_Layer1DObj @ 0x3228`` (33 entries: DObj 0 null,
22 DLLink-backed bindings, heads 0/1). Layer0 links 4/Sec, layer1 links
6/Sec per dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 147's extern fixups all
target file 122, so stage_images is a third packet input with asset
flag 2 -- same shape as bonus1_mario's file 120.

EXCLUDED (runtime-composed actors, not packet geometry):
- The platforms (small/medium/large plus boarded variants), composed at
  runtime by sc1PBonusStageInitPlatforms/MakePlatforms
  (decomp sc/sc1pmode/sc1pbonusstage.c:539-598) from
  dSC1PBonusStagePlatformDescs (:183) and
  dSC1PBonusStageBoardedPlatformDescs (:211), run only under
  sc1PBonusStageInitBonus2 (:733-739).
- The bumpers, composed at runtime by sc1PBonusStageMakeBumpers
  (:700-731) from this board's dSC1PBonusStageBumperDescs row (:169-173)
  and the map file's BumpersDObjDesc @ 0x4FE0 / BumpersAnimJoint @ 0x5120
  templates (llGRBonus2PurinMapBumpersDObjDesc/AnimJoint).
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
    name="bonus2_purin",
    include_sha="e86ad6f9ca789eeae47ab63bd92ab2e9d79452aab43a9111c3005116883aca24",
    generated_segment_index=-1,
    symbol_prefix="Bonus2Purin",
    macro_prefix="BONUS2PURIN_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 36,
        "bindings": 25,
        "commands": 786,
        "vertex_commands": 126,
        "source_vertices": 851,
        "modify_vertex_commands": 0,
        "triangle_commands": 226,
        "triangles": 451,
        "runs": 72,
        "texture_epochs": 55,
        "material_events": 0,
        "submit_classes": (268, 38, 145),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 244,
        "state_deltas": 53,
        "sync_events": 163,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank147",
            "sha256": "a99b518519a5382c793dcdf1de60fee07e6b582457e1d6673587b90c48b01a39",
            "file_id": 147,
            "internal_fixups": 271,
            "external_fixups": 47,
            "payload_sha256": "fa643256d8dad2c09fd911cba52974889f7282b06b93613c505e06c04355da05",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus2PurinMap",
            "sha256": "e1a59862d42a657d1cb3adc44f0879bc9f32a3dd750fab134abff449f8e208dc",
            "file_id": 293,
            "internal_fixups": 0,
            "external_fixups": 6,
            "payload_sha256": "b75461138b262cb11ac7cbc1166f6141f0962374de1b9f7ce80da9ce3b6a1e6c",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/147_GRBonus2PurinFile2.c",
            "sha256": "fd5c1522266d8d87b1bbf7d472a6302e17dbc18dfc5f413b162419af3a9e5341",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/293_GRBonus2PurinMap.c",
            "sha256": "a6f472c3fec64bdbb7c31fc38f13dc8738b1a2c74d6c10d01ddcd44e96de4eb7",
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
            "MPGroundData dGRBonus2PurinMap_header",
            "dGRBonus2PurinFile2_Layer0DObj",
            "dGRBonus2PurinFile2_Layer1DObj",
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
            "llGRBonus2PurinMapFileID",
            "llGRBonus2PurinMapMapHeader",
            "llGRBonus2PurinMapBumpersDObjDesc",
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
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x3228, 33, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 3, 0, 19),
        (OWNER_LAYER1, 6, 3, 22, 19, 53),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=36,
    adapter_binding_count=25,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x7A, 0x93, 0x125),
    adapter_asset_sizes=(0x03D0, 0x54E0, 0x00B0),
)
