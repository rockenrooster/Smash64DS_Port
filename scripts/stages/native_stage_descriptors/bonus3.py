"""Race to the Finish course static map layers, decoded from source tables.

1P gkind 15 (nGRKindBonus3, include/sc/scene.h:756).
295_GRBonus3Map.c wires gr_desc[0] to file 149's Layer0DObj and
gr_desc[1] to Layer1DObj plus Layer1MObj_MObjSub and
Layer1MatAnim_MatAnimJoint (no AnimJoint), with layers 2/3 NULL;
layer_mask is 2, so layer0 draws under its Pri proc (direct DL,
grdisplay.c dGRDisplayDescs link 4) and layer1 under its Sec proc
(DLLinks, link 6). File 149 supplies ``dGRBonus3File2_Layer0DObj
@ 0x3490`` (12 entries sentinel-included: DObj 0 null, 10 direct-DL
bindings) and ``dGRBonus3File2_Layer1DObj @ 0x6120`` (13 entries:
DObj 0 null, 14 DLLink-backed bindings across heads 0/1).

Material shape: dGRBonus3File2_Layer1MObj_MObjSub is a 12-slot
MObjSub** head paralleling layer1's 12 live DObjs (gcAddMObjAll):
slot 6 chains to the MObjSub at 0x36D0 (flags 0x200, three commands),
slot 10 holds the sub-list [0x3748 (0x200, three commands), 0x37C0
(0x3000, six commands)]. Three bindings enter segment-0xE slots --
DObj 6 root 0x5F10 slot 0, DObj 10 root 0x5D20 slot 8, DObj 10 root
0x6020 slot 0 -- paired head-order then slot-order (0x36D0, 0x37C0,
0x3748).

File 149 carries its own textures (no extern fixups), so the packet
takes only geometry + map inputs. File 162 (GRBonus3File3, course
AnimJoint/node data) is pinned as a text input; its DObjDesc trees
are collision/item data, not display layers.

EXCLUDED (runtime-composed actors, not packet geometry):
- The bumpers, composed at runtime by grBonus3MakeBumpers (decomp
  gr/grbonus/grbonus3.c:15-40) from llGRBonus3MapBumpersDObjDesc /
  llGRBonus3MapBumpersAnimJoint past the DOBJ_ARRAY_MAX sentinel.
- The taru bombs, spawned every 180 ticks by grBonus3TaruBombProcUpdate
  (:43-55) and seeded by grBonus3TaruBombMakeActor (:59-77) from the
  single nMPMapObjKind1PGameBonus3TaruBomb map object; the
  dGRBonus3Map_TaruBomb_ItemAttributes reference file 162's
  DObjDesc_0x0788 tree as item data, not packet geometry.
- The finish detection, run by grBonus3FinishProcUpdate (:80-90) on the
  grounded nMPMaterialDetect floor region and registered by
  grBonus3FinishMakeActor (:92-96); the finish is a floor-material
  region, not coordinates.
- The course map_nodes (file 162 DObjDesc_0x0000 tree), resolved by
  grBonus3InitHeaders (:8-12); collision data, not display layers.
All four ride grBonus3MakeGround (:98-106), called from
grMainSetupMakeGround outside the VS table.

The two static layers are composed by grCommonSetupInitAll
(grcommonsetup.c:25-28) via grDisplayMakeGeometryLayer, which is the
map-constructor anchor below. Runtime packet views and native actors
are still required before admission.
"""

from __future__ import annotations

from native_stage_descriptors import StageDescriptor

OWNER_LAYER0 = 0
OWNER_LAYER1 = 1

DESCRIPTOR = StageDescriptor(
    name="bonus3",
    include_sha="4680e73c5793ddca8cd2db8040eac958981d38ca56b88e2a1268a6d12fcd659c",
    generated_segment_index=-1,
    symbol_prefix="Bonus3",
    macro_prefix="BONUS3_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 23,
        "bindings": 24,
        "commands": 601,
        "vertex_commands": 60,
        "source_vertices": 542,
        "modify_vertex_commands": 0,
        "triangle_commands": 165,
        "triangles": 325,
        "runs": 63,
        "texture_epochs": 41,
        "material_events": 3,
        "submit_classes": (28, 50, 247),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 194,
        "state_deltas": 102,
        "sync_events": 149,
    },
    o2r_inputs={
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank149",
            "sha256": "a310360fe951c90aca3c58b466df0232409938dfc33abd784089c20fa56c818d",
            "file_id": 149,
            "internal_fixups": 161,
            "external_fixups": 0,
            "payload_sha256": "71c0ff7dd3cecb7cb135ccae3cb9df79f1b174a76a3290efa23a525aa3477885",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRBonus3Map",
            "sha256": "28d5923934dac1635a7fe6e08510680227cc19a676349731d1620600a085bae8",
            "file_id": 295,
            "internal_fixups": 0,
            "external_fixups": 7,
            "payload_sha256": "d9ae9532d1ba87e4ec9706132ca4a39b784810fbacc1191c7f60ab78932ab5bf",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/149_GRBonus3File2.c",
            "sha256": "106eab938973b3d19ba8ac81c833a62f59dbfc5df968bf484b8c3562e8b5ea61",
        },
        "geometry_extra": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/162_GRBonus3File3.c",
            "sha256": "d3731b33d778f530c3a33acec0005462f7a77533f031d387552096f3a57fed62",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/295_GRBonus3Map.c",
            "sha256": "42ad725c59805e48fe40ddd7df2003d3251900865f83eccb65f07c7387103025",
        },
        "ground": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommonsetup.c",
            "sha256": "12a3486f0c9a5d979b13f3c05da26613b7bed6689870af612a258f90e2677455",
        },
        "bonus": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grbonus/grbonus3.c",
            "sha256": "9c7273a02496566b8cb5f920c0ee113d35f6ff051900ebae1fca2d5fe77308cc",
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
            "MPGroundData dGRBonus3Map_gap_0x0000",
            "dGRBonus3File2_Layer0DObj",
            "dGRBonus3File2_Layer1DObj",
            "dGRBonus3File2_Layer1MObj_MObjSub",
        ),
        "geometry_extra": (
            "dGRBonus3File3_DObjDesc_0x0000",
            "dGRBonus3File3_DObjDesc_0x0788",
        ),
        "ground": (
            "grCommonSetupInitAll",
            "grDisplayMakeGeometryLayer",
            "grMainSetupMakeGround",
        ),
        "bonus": (
            "grBonus3MakeBumpers",
            "grBonus3TaruBombMakeActor",
            "grBonus3FinishMakeActor",
            "grBonus3MakeGround",
        ),
        "grdisplay": (
            "grDisplayLayer0PriProcDisplay",
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
            "llGRBonus3MapFileID",
            "llGRBonus3MapMapHeader",
            "llGRBonus3MapBumpersDObjDesc",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0x3490, 12, 4,
         "grDisplayLayer0PriProcDisplay", False),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x6120, 13, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    # (asset_id, binding_root, mobj_offset, segment_index)
    material_sources=(
        (149, 0x5F10, 0x36D0, 0x00),
        (149, 0x5D20, 0x37C0, 0x08),
        (149, 0x6020, 0x3748, 0x00),
    ),
    material_command_partition=(3, 6, 3),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 10, 0, 20),
        (OWNER_LAYER1, 6, 10, 14, 20, 43),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0PriProcDisplay", 4),
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=23,
    adapter_binding_count=24,
    adapter_asset_count=2,
    adapter_material_count=3,
    adapter_asset_ids=(0x95, 0x127),
    adapter_asset_sizes=(0x6890, 0x0110),
)
