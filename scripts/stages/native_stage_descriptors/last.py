"""Final Destination's single static map layer, decoded from the original tables.

1P gkind 16 (nGRKindLast, grdef.h:32); 266_GRLastMap.c wires only
gr_desc[1] to file 114's Layer1 table, so Last has no layer0/2/3 at all.
File 114 supplies ``dStageLastFile2_Layer1DObj @ 0x4D48`` (6 entries,
5 live + the sentinel, all five DL-backed). layer_mask is 0, so layer1
draws under grDisplayLayer1PriProcDisplay at link 6. No DLLink layer.

Materials: thirteen display lists call into segment-E material programs,
all length 3: DL_0x4230 and DL_0x45D8 call segs 0/8/0x10/0x18/0x20 each,
DL_0x4980 calls 8/0, DL_0x4D18's child DL_0x4BB8 calls 0. Pairing is
file-grounded: Layer1MObj_MObjSub[1..4] point at the gap_0x20F0 MObjSub*
tables (sub_0x960/sub_0x978 five each, sub_0x990 two, sub_0x99C one),
and segment 8*i matches gcDrawMObjForDObj's branch slot for MObj i.
DObj 0's DL_0x4078 makes no segment-E call (slot 0 is NULL).

The battle wallpaper (StageLastWallpaper o2r bank, file 96 sprite
dStageLastBackground_0x26c88 referenced by 266_GRLastMap.c:26) is a 2D
sprite, not geometry, and is not a packet input -- same rule as Castle.

No per-stage gr* ground TU exists for this kind (grmainsetup.c:11-22
lists only the nine VS kinds; grMainSetupMakeGround at :31-49 never takes
a Last arm), so there is no gr*Make* dynamic actor to exclude. The static
layer is composed by grCommonSetupInitAll (grcommonsetup.c:25-28) via
grDisplayMakeGeometryLayer, which is the map-constructor anchor below.

OUTSIDE this packet (runtime-composed, non-static): the Master Hand boss
background effects in file 114 above 0x86D8 (Effects0-3/MObjSub,
Anims0-3/MatAnimJoint, reloc_data.us.h:4034-4053), composed by
sc1PGameBossMakeWallpaperEffect (sc1pgameboss.c:853, display arms
:461-549, update arms :681-746, spawn :923-962) onto the wallpaper GObj
made at sc1pgameboss.c:1007-1015, plus the boss camera
sc1PGameBossMakeCamera (sc1pgameboss.c:399-460) and its 1P defeat zoom
(sc1pgame.c:1560, :2009). None is a gr*Make* actor; all still need their
own native paths. Runtime packet views are still required before admission.
"""

from __future__ import annotations

from native_stage_descriptors import StageDescriptor

OWNER_LAYER1 = 1

DESCRIPTOR = StageDescriptor(
    name="last",
    include_sha="481280df6f2d82c0d9fbb10bd1855822a18df60cb306b472eac762722bef696f",
    generated_segment_index=-1,
    symbol_prefix="Last",
    macro_prefix="LAST_",
    expected_counts={
        "callbacks": 1,
        "dobjs": 5,
        "bindings": 5,
        "commands": 449,
        "vertex_commands": 32,
        "source_vertices": 355,
        "modify_vertex_commands": 0,
        "triangle_commands": 105,
        "triangles": 198,
        "runs": 56,
        "texture_epochs": 24,
        "material_events": 13,
        "submit_classes": (126, 0, 72),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 170,
        "state_deltas": 81,
        "sync_events": 115,
    },
    o2r_inputs={
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank114",
            "sha256": "bfd53ac9a3d278513a6b4126cabf40ff73cf6e2b5812b96ea6ff69175f54c8f0",
            "file_id": 114,
            "internal_fixups": 675,
            "external_fixups": 0,
            "payload_sha256": "8da67fd19ea8e5eb1228101e6e9b912081020c3b4e71f328ab79aa9193cdd404",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRLastMap",
            "sha256": "efa2c00137a6cf016272085dbadfa637207997ddea5951a1f339cde792596693",
            "file_id": 266,
            "internal_fixups": 0,
            "external_fixups": 6,
            "payload_sha256": "61098ffa3f9e10bcc0b54d67b6efb0cb49fed7574190e392db1e7e0ee86859ec",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/114_StageLastFile2.c",
            "sha256": "764754ff941680a94d1eb05f755475bd7ae247f03fd05e0b8515400cd41caf16",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/266_GRLastMap.c",
            "sha256": "f0bdec0322fc6d7d58ea9c6bb3ff5b10b62bf63d5a0e2b357fe0cbaa35e855a2",
        },
        "ground": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommonsetup.c",
            "sha256": "12a3486f0c9a5d979b13f3c05da26613b7bed6689870af612a258f90e2677455",
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
            "MPGroundData dGRLastMap_header",
            "dStageLastFile2_Layer1DObj",
        ),
        "ground": (
            "grCommonSetupInitAll",
            "grDisplayMakeGeometryLayer",
            "grMainSetupMakeGround",
        ),
        "grdisplay": (
            "grDisplayLayer1PriProcDisplay",
            "gcDrawDObjTreeForGObj",
        ),
        "objanim": ("gcPlayAnimAll", "gcParseMObjMatAnimJoint"),
        "objdisplay": (
            "void gcDrawMObjForDObj",
            "gSPSegment(dl_head[0]++, 0xE",
            "void gcDrawDObjTreeForGObj",
        ),
        "reloc_symbols": (
            "llGRLastMapFileID",
            "llGRLastMapFileHead",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x4D48, 6, 6,
         "grDisplayLayer1PriProcDisplay"),
    ),
    # (asset_id, binding_root, mobj_offset, segment_index): one row per
    # MObjSub, in binding then segment order. Segment 8*i matches
    # gcDrawMObjForDObj's branch slot for MObj i of the owning DObj.
    # DObj 0 (DL_0x4078) owns no MObj (Layer1MObj_MObjSub[0] is NULL,
    # 114_StageLastFile2.c:321) and its DL makes no segment-E call.
    # DObjs 1/2 own five MObjs each (gap_0x20F0 tables sub_0x960/sub_0x978
    # at :1008/:1018), DObj 3 owns two (sub_0x990 at :1028), DObj 4 owns
    # one (sub_0x99C at :1035). DL branch sets match: DL_0x4230 and
    # DL_0x45D8 each call segs 0/8/0x10/0x18/0x20, DL_0x4980 calls 8/0.
    # DL_0x4BB8 (44 Gfx) is not a DObj root -- no DObjDesc points at it --
    # but a child DL of DObj 4's root DL_0x4D18; its single seg-0 branch
    # resolves against DObj 4's seg-0 material, so the row below names
    # root 0x4D18 with DObj 4's MObj (sub_0x99C at :1035, struct @0x29D8).
    material_sources=(
        (114, 0x4230, 0x2438, 0x00),
        (114, 0x4230, 0x24B0, 0x08),
        (114, 0x4230, 0x2528, 0x10),
        (114, 0x4230, 0x25A0, 0x18),
        (114, 0x4230, 0x2618, 0x20),
        (114, 0x45D8, 0x2780, 0x00),
        (114, 0x45D8, 0x27F8, 0x08),
        (114, 0x45D8, 0x2870, 0x10),
        (114, 0x45D8, 0x28E8, 0x18),
        (114, 0x45D8, 0x2960, 0x20),
        (114, 0x4980, 0x2690, 0x00),
        (114, 0x4980, 0x2708, 0x08),
        (114, 0x4D18, 0x29D8, 0x00),
    ),
    material_command_partition=(3,) * 13,
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER1, 6, 0, 5, 0, 56),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=1,
    adapter_dobj_count=5,
    adapter_binding_count=5,
    adapter_asset_count=2,
    adapter_material_count=13,
    adapter_asset_ids=(0x72, 0x10A),
    adapter_asset_sizes=(0x12960, 0x00B0),
)
