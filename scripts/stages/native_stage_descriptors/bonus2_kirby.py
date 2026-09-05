"""Board the Platforms: Kirby board static map layers, decoded from source tables.

1P gkind 37 (nGRKindBonus2Kirby, include/sc/scene.h:783).
291_GRBonus2KirbyMap.c wires gr_desc[0] to file 145's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 3, so both layers draw under their Sec procs (DLLinks,
grdisplay.c dGRDisplayDescs links 4/6). File 145 supplies
``dGRBonus2KirbyFile2_Layer0DObj @ 0xA20`` (7 entries sentinel-included:
DObj 0 null, 5 DLLink-backed bindings) and
``dGRBonus2KirbyFile2_Layer1DObj @ 0x2DB8`` (18 entries: DObj 0 null,
6 DLLink-backed bindings, heads 0 only). Layer0 links 4/Sec, layer1
links 6/Sec per dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 145's extern fixups all
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
  (:700-731) from this board's dSC1PBonusStageBumperDescs row (:157-161)
  and the map file's BumpersDObjDesc @ 0x3920 / BumpersAnimJoint @ 0x3A60
  templates (llGRBonus2KirbyMapBumpersDObjDesc/AnimJoint).
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
    name="bonus2_kirby",
    include_sha="551fa24e1fcf5af1498f6bb1089ee36efc04d8880a64276d57eabd7ee3c17df5",
    generated_segment_index=-1,
    symbol_prefix="Bonus2Kirby",
    macro_prefix="BONUS2KIRBY_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 23,
        "bindings": 11,
        "commands": 539,
        "vertex_commands": 52,
        "source_vertices": 485,
        "modify_vertex_commands": 0,
        "triangle_commands": 135,
        "triangles": 267,
        "runs": 48,
        "texture_epochs": 31,
        "material_events": 0,
        "submit_classes": (44, 42, 181),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 222,
        "state_deltas": 63,
        "sync_events": 119,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank145",
            "sha256": "b1a8c8be2e3462437dbbe3c70b671f68aae7550fa29a5d655090a2019d5f7c9a",
            "file_id": 145,
            "internal_fixups": 104,
            "external_fixups": 28,
            "payload_sha256": "861663e85a6df01fe45560858532c0e52480724066659512ce3183521e79d8a4",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus2KirbyMap",
            "sha256": "7dfdd8c7f5dc9761360dd833c15274542831567a25780b0e2da758a1b396e553",
            "file_id": 291,
            "internal_fixups": 0,
            "external_fixups": 6,
            "payload_sha256": "ebe5afa2568f03a20572d3959af21ceb606027b7cb1192a6f718c787b23cf268",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/145_GRBonus2KirbyFile2.c",
            "sha256": "22d9876fa13ecbc87b7c27d5662608c9a459e8372f78f9226e99881129376bc8",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/291_GRBonus2KirbyMap.c",
            "sha256": "3da7348834fe32a6154d6c16a149662700f3cb0b2ce17d0a43365640ffbc2120",
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
            "MPGroundData dGRBonus2KirbyMap_header",
            "dGRBonus2KirbyFile2_Layer0DObj",
            "dGRBonus2KirbyFile2_Layer1DObj",
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
            "llGRBonus2KirbyMapFileID",
            "llGRBonus2KirbyMapMapHeader",
            "llGRBonus2KirbyMapBumpersDObjDesc",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0xA20, 7, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x2DB8, 18, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 5, 0, 21),
        (OWNER_LAYER1, 6, 5, 6, 21, 27),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=23,
    adapter_binding_count=11,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x7B, 0x91, 0x123),
    adapter_asset_sizes=(0x0BD0, 0x3CD0, 0x00B0),
)
