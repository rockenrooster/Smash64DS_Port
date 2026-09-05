"""Board the Platforms: Captain board static map layers, decoded from source tables.

1P gkind 36 (nGRKindBonus2Captain, include/sc/scene.h:782).
290_GRBonus2CaptainMap.c wires gr_desc[0] to file 144's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 3, so both layers draw under their Sec procs (DLLinks,
grdisplay.c dGRDisplayDescs links 4/6). File 144 supplies
``dGRBonus2CaptainFile2_Layer0DObj @ 0xEA0`` (11 entries
sentinel-included: DObj 0 null, 9 DLLink-backed bindings) and
``dGRBonus2CaptainFile2_Layer1DObj @ 0x4198`` (15 entries: DObj 0 null,
4 DLLink-backed bindings, heads 0/1). Layer0 links 4/Sec, layer1 links
6/Sec per dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 144's extern fixups all
target file 121, so stage_images is a third packet input with asset
flag 2 -- same shape as bonus1_mario's file 120.

EXCLUDED (runtime-composed actors, not packet geometry):
- The platforms (small/medium/large plus boarded variants), composed at
  runtime by sc1PBonusStageInitPlatforms/MakePlatforms
  (decomp sc/sc1pmode/sc1pbonusstage.c:539-598) from
  dSC1PBonusStagePlatformDescs (:183) and
  dSC1PBonusStageBoardedPlatformDescs (:211), run only under
  sc1PBonusStageInitBonus2 (:733-739).
- No bumpers on this board: Captain's dSC1PBonusStageBumperDescs row is
  {0x0, 0x0} (:151-155); bumpers only exist on Bonus2 Fox/Samus/Kirby/
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
    name="bonus2_captain",
    include_sha="7c11db1123af3085cf1e9aa233c350f9b0940e021539662c373221edd30ea621",
    generated_segment_index=-1,
    symbol_prefix="Bonus2Captain",
    macro_prefix="BONUS2CAPTAIN_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 24,
        "bindings": 13,
        "commands": 706,
        "vertex_commands": 69,
        "source_vertices": 681,
        "modify_vertex_commands": 0,
        "triangle_commands": 184,
        "triangles": 365,
        "runs": 77,
        "texture_epochs": 54,
        "material_events": 0,
        "submit_classes": (20, 50, 295),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 288,
        "state_deltas": 86,
        "sync_events": 150,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank144",
            "sha256": "3126c32f79dcbd6513628558d3fd513baae63ba9291998ed2096cf54ed7db75f",
            "file_id": 144,
            "internal_fixups": 114,
            "external_fixups": 32,
            "payload_sha256": "372c78984ab150e38d21af3a2aa0c67429c7f3ff89bcc557cfa953a113c272b5",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus2CaptainMap",
            "sha256": "6918b09eba2d97ed1c13e395be1397752d233d16d8a3c1df6d2e7f80fb9caa20",
            "file_id": 290,
            "internal_fixups": 0,
            "external_fixups": 5,
            "payload_sha256": "6680d5de0f069b8c8bb12f526d1c70b80c066d1fec479bd0e9bbe48367b1457d",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/144_GRBonus2CaptainFile2.c",
            "sha256": "feb83b24c4c9ff6cd914fc06d386dba3da6e496ae16355f804fdbd759d2ef4de",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/290_GRBonus2CaptainMap.c",
            "sha256": "e1dc03cd77039b8a5f5e9a1f51b5620596a6317434bac04a6ae218cdbcdcac09",
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
            "MPGroundData dGRBonus2CaptainMap_header",
            "dGRBonus2CaptainFile2_Layer0DObj",
            "dGRBonus2CaptainFile2_Layer1DObj",
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
            "llGRBonus2CaptainMapFileID",
            "llGRBonus2CaptainMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0xEA0, 11, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x4198, 15, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 9, 0, 25),
        (OWNER_LAYER1, 6, 9, 4, 25, 52),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0SecProcDisplay", 4),
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=24,
    adapter_binding_count=13,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x79, 0x90, 0x122),
    adapter_asset_sizes=(0x07F0, 0x4F90, 0x00B0),
)
