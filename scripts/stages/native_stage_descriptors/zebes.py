"""Planet Zebes' single DLLink map layer plus the native acid actor owner.

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

The acid pool is the second owner: grzebes.c:71-115 grZebesMakeAcid
composes the acid GObj from llGRZebesMapAcidDObjDesc (file 157 @ 0xB08,
three entries, two live) and draws it through
gcDrawDObjTreeDLLinksForGObj at link 12 (grzebes.c:83). Its single
DLLink (file 157 @ 0xAF8, head 1 then sentinel head 4) targets the one
display list dStageZebesFile3_DL_0x09D8, so the actor owns exactly one
binding at root 0x09D8. The live sprite material is MObjSub @ 0x8D8
(flags 0x6B, ten opcode commands) fed through the source MObjSub***
wrapper at llGRZebesMapAcidMObjSub (0x8C0, via the 0x8C4 link into the
0x950 list); its frame table @ 0x8C8 points at four 32x32 CI4 textures
(0x00A8/0x02B0/0x04B8/0x06C0) kept live by gcAddMObjAll (grzebes.c:93).
The TRAY joint script @ 0x0B98 (via table 0x0B8C) and the material
script @ 0x0BD8 (via tables 0x0BC8/0x0DC0) stay source-side: the packet
snapshots only the immutable 0x09D8 stream plus the live material state,
while gcAddAnimAll, the proc-update DObj translate.y write
(grzebes.c:112,167-171,190-210), and the hazard registration
(ftMainCheckAddGroundHazard with grZebesAcidCheckGetDamageKind,
grzebes.c:218-225, coll wrapper 0xBC) stay live in the source update
path. The 0x0BD8 script's ROTX/TRAY/SCAY channels are material-script
channels; the packet makes no claim they rotate geometry.

The Ridley/Ship background effects (efground.c:385-388 and 424-427,
wired at efground.c:1017-1022) stay outside this packet, as does the C
packet registration, which the runtime owner adds after all inputs are
stable.
"""

from native_stage_descriptors import StageDescriptor

DESCRIPTOR = StageDescriptor(
    name="zebes",
    include_sha="5e4f0222d0afffca53957105b8d126773d92c53ed6aff542308e64c3ce097968",
    generated_segment_index=-1,
    symbol_prefix="Zebes",
    macro_prefix="ZEBES_",
    expected_counts={
        "callbacks": 2,
        "dobjs": 30,
        "bindings": 26,
        "commands": 632,
        "vertex_commands": 46,
        "source_vertices": 317,
        "modify_vertex_commands": 0,
        "triangle_commands": 82,
        "triangles": 151,
        "runs": 60,
        "texture_epochs": 42,
        "material_events": 19,
        "submit_classes": (92, 7, 52),
        "state_events": 282,
        "state_deltas": 142,
        "sync_events": 173,
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "alpha_clone_vertices": 39,
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
        "stage_actors": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank157",
            "sha256": "7e4a6f970a333635b4fe2591db45d6a28942c294085dc773e9bb58165c62dc2a",
            "file_id": 157,
            "internal_fixups": 16,
            "external_fixups": 0,
            "payload_sha256": "2ba9176aef611ce0043225097b1305c8fad4ce8addfacfd3b42a2cbc755d670f",
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
        "actors_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/157_StageZebesFile3.c",
            "sha256": "7a6daf9c1939b9b35bbb07586bfdc12c3961c2816c4431a1d524e662ec58f0d3",
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
        "ground": (
            "grZebesMakeGround",
            "grZebesMakeAcid",
            "gcDrawDObjTreeDLLinksForGObj",
            "gcAddMObjAll",
            "grZebesAcidCheckGetDamageKind",
        ),
        "grdisplay": ("grDisplayLayer1SecProcDisplay",),
        "objdisplay": ("gcDrawDObjTreeDLLinksForGObj",),
        "reloc_symbols": (
            "llGRZebesMapFileID",
            "llGRZebesMapAcidDObjDesc",
            "llGRZebesMapAcidMObjSub",
        ),
        "actors_typed": (
            "dStageZebesFile3_DL_0x09D8",
            "dStageZebesFile3_DObjDesc_0x0B08",
            "dStageZebesFile3_MObjSub_0x08D8",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grZebesMakeGround(",
    map_constructor_min_count=1,
    asset_order=(("stage_geometry", 1), ("stage_actors", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link,
    #  callback, dl_links)
    owner_specs=(
        (1, "layer1", "stage_geometry", 0x5AC0, 29, 6,
         "grDisplayLayer1SecProcDisplay", True),
        (2, "acid", "stage_actors", 0xB08, 3, 12,
         "gcDrawDObjTreeDLLinksForGObj", True),
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
        # Acid pool: binding root 0x09D8 selects the single MObjSub 0x08D8
        # at segment slot 0 (the DL's 0xDE branch targets segment 0xE + 0).
        (157, 0x9D8, 0x8D8, 0x00),
    ),
    material_command_partition=(3,) * 18 + (10,),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (1, 6, 0, 25, 0, 55),
        (2, 12, 25, 1, 55, 5),
    ),
    callback_partition=(
        ("layer1", "grDisplayLayer1SecProcDisplay", 6),
        ("acid", "gcDrawDObjTreeDLLinksForGObj", 12),
    ),
    segment0={},
    adapter_segment_count=2,
    adapter_dobj_count=30,
    adapter_binding_count=26,
    adapter_asset_count=3,
    adapter_material_count=19,
    adapter_asset_ids=(0x69, 0x9D, 0x101),
    adapter_asset_sizes=(0xDF60, 0xDD0, 0x00E0),
)
