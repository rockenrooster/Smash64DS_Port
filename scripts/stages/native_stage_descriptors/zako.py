"""Duel Zone's single static map layer, decoded from the original tables.

1P gkind 14 (nGRKindZako, grdef.h:30); 268_GRZakoMap.c wires only
gr_desc[1] to file 116's table, so Zako has no layer0/2/3 at all. File
116 supplies ``dStageBattlefieldFile2_DObjDesc_0x4170[3]`` @ 0x4170
(2 live + the sentinel; DObj 0 has no display list, DObj 1 carries
DL_0x3AB0). layer_mask is 0, so layer1 draws under
grDisplayLayer1PriProcDisplay at link 6. No DLLink layer.

Materials: file 116 contains no MObj structs (no MObj symbol in
116_StageBattlefieldFile2.c) and gr_desc[1] carries no MObj/MatAnim
(268_GRZakoMap.c:24), so material_sources is empty -- the packet owns
zero material events unless the generator's fail-closed segment-E walk
says otherwise.

No per-stage gr* ground TU exists for this kind (grmainsetup.c:11-22
lists only the nine VS kinds; grMainSetupMakeGround at :31-49 never takes
a Zako arm), so there is no gr*Make* dynamic actor to exclude. The static
layer is composed by grCommonSetupInitAll (grcommonsetup.c:25-28) via
grDisplayMakeGeometryLayer, which is the map-constructor anchor below.
The Fighting Polygon Team fighters are fighter actors, not stage ground.
Runtime packet views are still required before admission.
"""

from __future__ import annotations

from native_stage_descriptors import StageDescriptor

OWNER_LAYER1 = 1

DESCRIPTOR = StageDescriptor(
    name="zako",
    include_sha="98d4c228aa42e5e009e664c90aabb72c470d230e390dda73b09e702e7211945a",
    generated_segment_index=-1,
    symbol_prefix="Zako",
    macro_prefix="ZAKO_",
    expected_counts={
        "callbacks": 1,
        "dobjs": 2,
        "bindings": 1,
        "commands": 216,
        "vertex_commands": 17,
        "source_vertices": 237,
        "modify_vertex_commands": 0,
        "triangle_commands": 70,
        "triangles": 138,
        "runs": 16,
        "texture_epochs": 15,
        "material_events": 0,
        "submit_classes": (138, 0, 0),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 70,
        "state_deltas": 47,
        "sync_events": 58,
    },
    o2r_inputs={
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank116",
            "sha256": "0bbf49775f6605d3de6f73ce156401f4ee8ac32f87b575919760f12384251b8b",
            "file_id": 116,
            "internal_fixups": 43,
            "external_fixups": 0,
            "payload_sha256": "6af75a2222efc359426b2842cb772ef0a1d230ec196b01eff1536b009b4833ec",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRZakoMap",
            "sha256": "722631ce257ec241bcf692dc78c6896ff87872b40da2de88697ecde282267e8e",
            "file_id": 268,
            "internal_fixups": 1,
            "external_fixups": 3,
            "payload_sha256": "4b7dadadf705dfa6b2c2c7e4473b4df757f2f03a472e35c5e1154053f4f4df3a",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/116_StageBattlefieldFile2.c",
            "sha256": "ce97e88f30eeab83a145cc27bc2de475e0db366ee484086f87153a13d24703ca",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/268_GRZakoMap.c",
            "sha256": "4051de6a4fbfe3589628d26230a487621931c371c9dd27e4521c027412d35f8a",
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
            "MPGroundData dGRZakoMap_header",
            "dStageBattlefieldFile2_DObjDesc_0x4170",
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
            "llGRZakoMapFileID",
            "llGRZakoMapMapHeader",
        ),
    },
    map_constructor_text_key="ground",
    map_constructor_token="grDisplayMakeGeometryLayer(",
    map_constructor_min_count=4,
    asset_order=(("stage_geometry", 1), ("stage_map", 4)),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x4170, 3, 6,
         "grDisplayLayer1PriProcDisplay"),
    ),
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER1, 6, 0, 1, 0, 16),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
    ),
    segment0={},
    adapter_segment_count=1,
    adapter_dobj_count=2,
    adapter_binding_count=1,
    adapter_asset_count=2,
    adapter_material_count=0,
    adapter_asset_ids=(0x74, 0x10C),
    adapter_asset_sizes=(0x43B0, 0x00C0),
)
