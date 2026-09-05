"""Planet Zebes' single DLLink map layer with eighteen bound MObjs.

VS gkind 3 (nGRKindZebes, grdef.h); 257_GRZebesMap.c sets layer_mask 2 and
names file 105's Layer1 DObj table at 0x5AC0 (llGRZebesMapDataStart) with 28
live DObjs behind a 29th sentinel. 257's header wires only gr_desc[1], so
Zebes has no layer0 at all. Layer1 draws through grDisplayLayer1SecProc
Display (grdisplay.c dGRDisplayDescs[1], GObj link 6) on
gcDrawDObjTreeDLLinksForGObj: heads carry 21/4 bindings and 25 roots exist.

Material shape: dStageZebesFile2_Layer1MObj_MObjSub at 0x2B48 is an
MObjSub** per DObj (gcAddMObjAll, objanim.c:2429). Eight DObj slots hold
NULL-terminated MObjSub* tables -- DObjs 1/2 own three MObjs each, DObj 27
owns seven, DObjs 5/6/10/19/24 own one -- 18 MObjSubs in total.
gcDrawMObjForDObj (objdisplay.c:1204) generates one branch slot per MObj,
and each binding's display list selects material i with a segment-0xE
branch at byte offset 8*i. Zebes' lists branch in reverse MObj order; all
18 MObjSubs carry flags 0x0004 (palette-only, three commands each).

The runtime-composed actors stay outside this packet: the acid pool
(grzebes.c:71-115 grZebesMakeAcid, llGRZebesMapAcidDObjDesc/MObjSub/
AnimJoint/MatAnimJoint in file 3), and the Ridley/Ship background effects
(efground.c:385-388 and 424-427, wired at efground.c:1017-1022). Their
native rendering and the C packet registration are still owed.
"""

from native_stage_descriptors import StageDescriptor

DESCRIPTOR = StageDescriptor(
    name="zebes",
    include_sha="7e75f487ae70cb231be5df79cfdb2d2c7cd88086385ea8079215fdf25184338d",
    generated_segment_index=-1,
    symbol_prefix="Zebes",
    macro_prefix="ZEBES_",
    expected_counts={
        "callbacks": 1,
        "dobjs": 28,
        "bindings": 25,
        "commands": 586,
        "vertex_commands": 45,
        "source_vertices": 309,
        "modify_vertex_commands": 0,
        "triangle_commands": 78,
        "triangles": 144,
        "runs": 51,
        "texture_epochs": 41,
        "material_events": 18,
        "submit_classes": (92, 0, 52),
        "state_events": 251,
        "state_deltas": 123,
        "sync_events": 165,
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
    },
    o2r_inputs={
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank105",
            "sha256": "2cf584df3ed7bed141315f2d3ba0fd122e683fb608aad3254d8aec6b8bf93b8e",
            "file_id": 105,
            "internal_fixups": 322,
            "external_fixups": 0,
            "payload_sha256": "2e8bb0dcec6832c5d6b8a61ccae076d828225153e5fd3a80045c083260d32f27",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRZebesMap",
            "sha256": "96e4adfaa36d75721e5cf10e54f033eb26901264af1250e126075f9ffc27a279",
            "file_id": 257,
            "internal_fixups": 1,
            "external_fixups": 7,
            "payload_sha256": "8264f731c2fa58dd5815996536b051826758492d116b8e35fba01b2e5a1ba14d",
        },
    },
    text_inputs={
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
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/105_StageZebesFile2.c",
            "sha256": "65fbf7b4d62802b51c2f527bfb7084509814f8e16169010352e44ece72c94e32",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/257_GRZebesMap.c",
            "sha256": "4dffa84fa68c805bd6ac5d5e43c329d4429f575c889fba365594e213619b8635",
        },
        "ground": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommon/grzebes.c",
            "sha256": "7e278c7a7fdc4d033284fd3fa1477ee42e646f816643aa15631b933fba0f538e",
        },
    },
    text_contract_tokens={
        "map_typed": ("MPGroundData dGRZebesMap_MapHeader_0x0014",),
        "ground": ("grZebesMakeGround",),
        "grdisplay": ("grDisplayLayer1SecProcDisplay",),
        "objdisplay": ("gcDrawDObjTreeDLLinksForGObj",),
        "reloc_symbols": ("llGRZebesMapFileID",),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grZebesMakeGround(",
    map_constructor_min_count=1,
    asset_order=(("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link,
    #  callback, dl_links)
    owner_specs=(
        (1, "layer1", "stage_geometry", 0x5AC0, 29, 6,
         "grDisplayLayer1SecProcDisplay", True),
    ),
    # (asset_id, binding_root, mobj_offset, segment_index): one row per
    # MObjSub of the eight material DObjs, in binding then segment order.
    # Segment 8*i matches gcDrawMObjForDObj's branch slot for MObj i.
    material_sources=(
        (105, 0x4890, 0x2C98, 0x00),
        (105, 0x4890, 0x2D10, 0x08),
        (105, 0x4890, 0x2D88, 0x10),
        (105, 0x4C50, 0x2E00, 0x00),
        (105, 0x4C50, 0x2E78, 0x08),
        (105, 0x4C50, 0x2EF0, 0x10),
        (105, 0x4E50, 0x2F68, 0x00),
        (105, 0x4ED8, 0x2FE0, 0x00),
        (105, 0x4FE0, 0x3058, 0x00),
        (105, 0x5278, 0x30D0, 0x00),
        (105, 0x5410, 0x3148, 0x00),
        (105, 0x5510, 0x31C0, 0x00),
        (105, 0x5510, 0x3238, 0x08),
        (105, 0x5510, 0x32B0, 0x10),
        (105, 0x5510, 0x3328, 0x18),
        (105, 0x5510, 0x33A0, 0x20),
        (105, 0x5510, 0x3418, 0x28),
        (105, 0x5510, 0x3490, 0x30),
    ),
    material_command_partition=(3,) * 18,
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (1, 6, 0, 25, 0, 51),
    ),
    callback_partition=(
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=1,
    adapter_dobj_count=28,
    adapter_binding_count=25,
    adapter_asset_count=2,
    adapter_material_count=18,
    adapter_asset_ids=(105, 257),
    adapter_asset_sizes=(57184, 224),
)
