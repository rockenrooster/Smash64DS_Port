"""Board the Platforms: Pikachu board static map layers, decoded from source tables.

1P gkind 38 (nGRKindBonus2Pikachu, include/sc/scene.h:784).
292_GRBonus2PikachuMap.c wires gr_desc[0] to file 146's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 3, so both layers draw under their Sec procs (DLLinks,
grdisplay.c dGRDisplayDescs links 4/6). File 146 supplies
``dGRBonus2PikachuFile2_Layer0DObj @ 0xD58`` (9 entries
sentinel-included: DObj 0 null, 7 DLLink-backed bindings) and
``dGRBonus2PikachuFile2_Layer1DObj @ 0x3690`` (25 entries: DObj 0 null,
13 DLLink-backed bindings, heads 0 only). Layer0 links 4/Sec, layer1
links 6/Sec per dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 146's extern fixups all
target file 122, so stage_images is a third packet input with asset
flag 2 -- same shape as bonus1_mario's file 120.

EXCLUDED (runtime-composed actors, not packet geometry):
- The platforms (small/medium/large plus boarded variants), composed at
  runtime by sc1PBonusStageInitPlatforms/MakePlatforms
  (decomp sc/sc1pmode/sc1pbonusstage.c:539-598) from
  dSC1PBonusStagePlatformDescs (:183) and
  dSC1PBonusStageBoardedPlatformDescs (:211), run only under
  sc1PBonusStageInitBonus2 (:733-739).
- No bumpers on this board: Pikachu's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0} (:163-167); bumpers only exist on Bonus2 Fox/Samus/Kirby/
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
    name="bonus2_pikachu",
    include_sha="ff785d4b492680d809a49b96ab13dc72e502c2be9a7c6ec992c7ab53cba2bed8",
    generated_segment_index=-1,
    symbol_prefix="Bonus2Pikachu",
    macro_prefix="BONUS2PIKACHU_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 32,
        "bindings": 20,
        "commands": 491,
        "vertex_commands": 47,
        "source_vertices": 589,
        "modify_vertex_commands": 0,
        "triangle_commands": 154,
        "triangles": 305,
        "runs": 49,
        "texture_epochs": 29,
        "material_events": 0,
        "submit_classes": (84, 46, 175),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 183,
        "state_deltas": 51,
        "sync_events": 85,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank146",
            "sha256": "5cdc8f381be980e95001ec97832b1b43ea45a2e7f4455aa4641de4818621f21c",
            "file_id": 146,
            "internal_fixups": 127,
            "external_fixups": 16,
            "payload_sha256": "afe44e8415e13b87d9f64e3274441d545cc7195d447c79f1edc6141b12d6004b",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus2PikachuMap",
            "sha256": "f9d22637c854c8e96e0594f1669613f4d59130018fd9f5998821048ba2742b7a",
            "file_id": 292,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "98f1f58ca670ee38a68da4a034d25f7902238d2feea6cc589bc49db7bd0a0386",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/146_GRBonus2PikachuFile2.c",
            "sha256": "138528876253c55e507a555f1427bb9329540abebeb7e96a865fb3f2758ee9d5",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/292_GRBonus2PikachuMap.c",
            "sha256": "1bb1d0522ef0aa6aa5c84d2a1adc0cb2108238bd0c10a80aefb3bf8013649813",
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
            "MPGroundData dGRBonus2PikachuMap_header",
            "dGRBonus2PikachuFile2_Layer0DObj",
            "dGRBonus2PikachuFile2_Layer1DObj",
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
            "llGRBonus2PikachuMapFileID",
            "llGRBonus2PikachuMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0xD58, 9, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x3690, 25, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 7, 0, 23),
        (OWNER_LAYER1, 6, 7, 13, 23, 26),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=32,
    adapter_binding_count=20,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x7A, 0x92, 0x124),
    adapter_asset_sizes=(0x03D0, 0x48F0, 0x00B0),
)
