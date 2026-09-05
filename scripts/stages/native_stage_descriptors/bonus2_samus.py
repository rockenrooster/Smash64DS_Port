"""Board the Platforms: Samus board static map layers, decoded from source tables.

1P gkind 32 (nGRKindBonus2Samus, include/sc/scene.h:778).
286_GRBonus2SamusMap.c wires gr_desc[0] to file 140's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1Anim_AnimJoint, with layers 2/3 NULL;
layer_mask is 3, so both layers draw under their Sec procs (DLLinks,
grdisplay.c dGRDisplayDescs links 4/6). File 140 supplies
``dGRBonus2SamusFile2_Layer0DObj @ 0x720`` (4 entries sentinel-included:
DObj 0 null, 2 DLLink-backed bindings) and
``dGRBonus2SamusFile2_Layer1DObj @ 0x1CA8`` (21 entries: DObj 0 null,
9 DLLink-backed bindings, heads 0 only). Layer0 links 4/Sec, layer1
links 6/Sec per dGRDisplayDescs (grdisplay.c:10-26).

Textures live outside the geometry bank: file 140's extern fixups all
target file 121, so stage_images is a third packet input with asset
flag 2 -- same shape as bonus1_mario's file 120.

EXCLUDED (runtime-composed actors, not packet geometry):
- The platforms (small/medium/large plus boarded variants), composed at
  runtime by sc1PBonusStageInitPlatforms/MakePlatforms
  (decomp sc/sc1pmode/sc1pbonusstage.c:539-598) from
  dSC1PBonusStagePlatformDescs (:183) and
  dSC1PBonusStageBoardedPlatformDescs (:211), run only under
  sc1PBonusStageInitBonus2 (:733-739).
- The bumpers, composed at runtime by sc1PBonusStageMakeBumpers
  (:700-731) from this board's dSC1PBonusStageBumperDescs row (:127-131)
  and the map file's BumpersDObjDesc @ 0x2910 / BumpersAnimJoint @ 0x29C0
  templates (llGRBonus2SamusMapBumpersDObjDesc/AnimJoint).
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
    name="bonus2_samus",
    include_sha="7df320994c2a2ca4567a239e82516f442e6079ab95d2811e998d2179a7fb8001",
    generated_segment_index=-1,
    symbol_prefix="Bonus2Samus",
    macro_prefix="BONUS2SAMUS_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 23,
        "bindings": 11,
        "commands": 437,
        "vertex_commands": 34,
        "source_vertices": 358,
        "modify_vertex_commands": 0,
        "triangle_commands": 94,
        "triangles": 188,
        "runs": 42,
        "texture_epochs": 19,
        "material_events": 0,
        "submit_classes": (98, 36, 54),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 195,
        "state_deltas": 54,
        "sync_events": 103,
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
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank140",
            "sha256": "7c3546270b91b77bbbe99116bd61421caedd976747c9ae0d6f0aab88a0101e4d",
            "file_id": 140,
            "internal_fixups": 97,
            "external_fixups": 26,
            "payload_sha256": "f7b2027ba6069d410554142bbc98a175658789d7746e093ee0a7238c25fb4a40",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus2SamusMap",
            "sha256": "4498084d95e377b5acb94109422eaab4243c86514e90f605a0d1a10a869f36d3",
            "file_id": 286,
            "internal_fixups": 0,
            "external_fixups": 6,
            "payload_sha256": "b4fdfb64501ebae76dfa6496ecf1118fe0d966f704da41a423c2b9bd06fe30b3",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/140_GRBonus2SamusFile2.c",
            "sha256": "e4ff6a6a4cbbaf702431267d0d92f4f6933a0dd44d8e138934a774081e8491d0",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/286_GRBonus2SamusMap.c",
            "sha256": "affba499bbce7fd90d99948e4903b7fe6ffe06c401f4fd7b9632463db3186b8c",
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
            "MPGroundData dGRBonus2SamusMap_header",
            "dGRBonus2SamusFile2_Layer0DObj",
            "dGRBonus2SamusFile2_Layer1DObj",
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
            "llGRBonus2SamusMapFileID",
            "llGRBonus2SamusMapMapHeader",
            "llGRBonus2SamusMapBumpersDObjDesc",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_images", 2), ("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0x720, 4, 4,
         "grDisplayLayer0SecProcDisplay", True),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x1CA8, 21, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 2, 0, 18),
        (OWNER_LAYER1, 6, 2, 9, 18, 24),
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
    adapter_asset_ids=(0x79, 0x8C, 0x11E),
    adapter_asset_sizes=(0x07F0, 0x29F0, 0x00B0),
)
