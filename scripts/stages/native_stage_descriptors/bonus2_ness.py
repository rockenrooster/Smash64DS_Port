"""Board the Platforms: Ness board static map layers, decoded from source tables.

1P gkind 40 (nGRKindBonus2Ness, include/sc/scene.h:786).
294_GRBonus2NessMap.c wires gr_desc[0] to file 148's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 3, so both layers draw under their Sec procs (DLLinks,
grdisplay.c dGRDisplayDescs links 4/6). File 148 supplies
``dGRBonus2NessFile2_Layer0DObj @ 0xB70`` (8 entries sentinel-included:
DObj 0 null, 6 DLLink-backed bindings) and
``dGRBonus2NessFile2_Layer1DObj @ 0x34A0`` (19 entries: DObj 0 null,
7 DLLink-backed bindings, heads 0 only). Layer0 links 4/Sec, layer1
links 6/Sec per dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 148's extern fixups all
target file 123, so stage_images is a third packet input with asset
flag 2 -- same shape as bonus1_mario's file 120.

EXCLUDED (runtime-composed actors, not packet geometry):
- The platforms (small/medium/large plus boarded variants), composed at
  runtime by sc1PBonusStageInitPlatforms/MakePlatforms
  (decomp sc/sc1pmode/sc1pbonusstage.c:539-598) from
  dSC1PBonusStagePlatformDescs (:183) and
  dSC1PBonusStageBoardedPlatformDescs (:211), run only under
  sc1PBonusStageInitBonus2 (:733-739).
- The bumpers, composed at runtime by sc1PBonusStageMakeBumpers
  (:700-731) from this board's dSC1PBonusStageBumperDescs row (:175-179)
  and the map file's BumpersDObjDesc @ 0x3FE0 / BumpersAnimJoint @ 0x4090
  templates (llGRBonus2NessMapBumpersDObjDesc/AnimJoint).
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
    name="bonus2_ness",
    include_sha="a45c77b8e6581ce6a8f2b4ebb5fb8552b0131f8659c676cd1a518bcd648152a9",
    generated_segment_index=-1,
    symbol_prefix="Bonus2Ness",
    macro_prefix="BONUS2NESS_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 25,
        "bindings": 13,
        "commands": 666,
        "vertex_commands": 80,
        "source_vertices": 534,
        "modify_vertex_commands": 0,
        "triangle_commands": 144,
        "triangles": 288,
        "runs": 68,
        "texture_epochs": 54,
        "material_events": 0,
        "submit_classes": (60, 44, 184),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 268,
        "state_deltas": 83,
        "sync_events": 159,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank148",
            "sha256": "768fa28729b05a7e1c483a2d40a2719ee477e6e87715e4b8632119d4f203c2e2",
            "file_id": 148,
            "internal_fixups": 128,
            "external_fixups": 42,
            "payload_sha256": "7aa682d3d16de39cef1b08cf0f1b43e4731a221ef3d51e60a4847dfdfabc2184",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus2NessMap",
            "sha256": "19ae5a6ec7726f61263c12c5a4092bd276f74e5ecf9b0c67773b7f8ce36937db",
            "file_id": 294,
            "internal_fixups": 0,
            "external_fixups": 6,
            "payload_sha256": "b2fdf6cdb7b5af00d1193e09672478d6eddf2b1ff98e0cd10da0790e7218dfcd",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/148_GRBonus2NessFile2.c",
            "sha256": "93059b468c7964468103f6f00eeb3fb73bd7c3a603695676bd83477a6effbd4e",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/294_GRBonus2NessMap.c",
            "sha256": "3e2442dd142db72d54b0244476cd6d2263b96505a30775c8cc7c0d02502479d5",
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
            "MPGroundData dGRBonus2NessMap_header",
            "dGRBonus2NessFile2_Layer0DObj",
            "dGRBonus2NessFile2_Layer1DObj",
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
            "llGRBonus2NessMapFileID",
            "llGRBonus2NessMapMapHeader",
            "llGRBonus2NessMapBumpersDObjDesc",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0xB70, 8, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x34A0, 19, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 6, 0, 22),
        (OWNER_LAYER1, 6, 6, 7, 22, 46),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=25,
    adapter_binding_count=13,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x7B, 0x94, 0x126),
    adapter_asset_sizes=(0x0BD0, 0x40C0, 0x00B0),
)
