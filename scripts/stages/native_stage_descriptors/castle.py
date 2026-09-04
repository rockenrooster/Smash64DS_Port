"""Peach's Castle (internal name Castle) stage descriptor.

Third native-packet descriptor beside the frozen Dream Land one and Yoster.
Every number below was measured by running the stage generator against the
pinned Castle inputs, never invented; the probes are recorded in the comments.

VS gkind 0: ``nGRKindCastle = nGRkindStarterStart`` (``gr/grdef.h:11``), the
first starter stage.

Inputs
------
O2R (file id / fixup counts read off each bank's OLER header; SHA-256 over
the file and over its payload measured 2026-09-04):

- stage_geometry  ExternDataBank106  file 106, internal 134, external 0
- stage_actors    MiscDataBank156    file 156, internal 2,  external 0
- stage_map       GRCastleMap        file 259 (``llGRCastleMapFileID`` is
  ``0x103`` in ``reloc_data.us.h``), internal 1, external 9

Castle has no separate images bank (Dream Land's 103, Yoster's 110): every
texture the display lists reference resolves inside file 106 (the
``106_StageCastleFile2.reloc`` chain is intern-only). The selection-screen
sprite bank (file 95, ``StageCastle``) and the wallpaper (file 90,
``MVOpeningRoomWallpaper``, referenced by ``header.wallpaper``) are not
battle geometry and are not packet inputs.

Typed/source controls: ``106_StageCastleFile2.c`` (geometry),
``156_StageCastleFile3.c`` (platform-sweep AnimJoint script),
``259_GRCastleMap.c`` (map header), ``gr/grcommon/grcastle.c`` (stage
logic) plus the shared ``grdisplay.c``, ``sys/objanim.c``,
``sys/objdisplay.c`` and ``reloc_data.us.h``.

The map header's item weights carry a region guard
(``259_GRCastleMap.c:21-25``); the US build takes the ``#else`` (non-JP)
branch, so the descriptor provenance is REGION_US.

Owners
------
The four static geometry layers from ``dGRCastleMap_header.gr_desc``
(``259_GRCastleMap.c``). Table offsets and entry counts (sentinel included,
per the Dream Land convention) come from the ``@ 0x...`` comments in
``106_StageCastleFile2.c``; the header's extern map
(``259_GRCastleMap.reloc``) pins each table to file 106:

- layer0 ``dStageCastleFile2_Layer0DObj @ 0x1850`` (7 entries, 6 live,
  5 DL-backed bindings; DObj 0 has no display list)
- layer1 ``dStageCastleFile2_Layer1DObj @ 0x2A08`` (6 entries, 5 live,
  4 bindings; DObj 0 has no display list)
- layer2 ``dStageCastleFile2_Layer2DObj @ 0x3218`` (4 entries, 3 live,
  2 bindings; DObj 0 has no display list)
- layer3 ``dStageCastleFile2_Layer3DObj @ 0x3600`` (3 entries, 2 live,
  1 binding; DObj 0 has no display list)

Links and callbacks are the standard ``dGRDisplayDescs`` rows
(``grdisplay.c:10-43``): layer N draws under
``grDisplayLayerNPriProcDisplay`` at link 4/6/13/17 (``layer_mask`` is 0,
so the primary procs apply).

NOT covered: the bumper item and the Lakitu ground effect.
``grCastleInitAll`` (``grcastle.c:25-58``) composes the bumper at runtime
with ``itManagerMakeItemSetupCommon(..., nITKindGBumper, ...)``
(``grcastle.c:57``), positioned from the bumper yakumono
(``grcastle.c:48-51``) and steered per-frame by
``grCastleBumperProcUpdate`` (``grcastle.c:12-22``); the left/right
Lakitu effect descs (``ef/efground.c:40-106``) compose the
``dStageCastleFile2`` Lakitu DObj (``llGRCastleMapLakituDObjDesc``,
file 106 ``@ 0x4118``) at runtime. These independent item/effect owners
remain outside the four-layer capture contract and still need their own
native paths. File 156's TraX script drives the invisible bumper controller
made by ``grCastleInitAll``; that GObj has no display list. It is not the
platform animation. The layer-1 and layer-2 joint animations come from file
106 through the map header, and the packet includes their geometry. Their
animated bindings are 6/7/8/9; the runtime rigid mask is ``0xC3F``.

Materials
---------
File 106 contains no MObj structs (no ``MObj`` symbol in
``106_StageCastleFile2.c``) and no display list calls into segment-E
material programs, so ``material_sources`` is empty and
``material_command_partition`` is ``()`` -- the packet has zero
material events.

Expected counts
---------------
Measured generator output for the structure above (``generate()`` passes
``validate_packet`` with exactly these):

callbacks 4, dobjs 16, bindings 12, commands 580, vertex_commands 38,
source_vertices 254 (modifyvtx 0, so dense == source),
triangle_commands 73, triangles 136, runs 40, texture_epochs 35,
material_events 0, submit classes (raw 56, no-Z 62, range/matrix 18),
cross-matrix (0, 0, 0) -- Castle's vertex cache never crosses bindings,
state deltas 111, state events 287, state spans 52 (= 40 runs + 12 tails),
sync events 164. Const slab 8867 bytes (< 16 KiB).

Generated segment program
-------------------------
The straight-line program keeps its Dream Land contract (one submit class
per run, no material ownership/consumption, single live tail at the last
binding, contiguous state/epoch windows); only the window numbers are
per-stage. Segment 0 (layer0) is NOT programmable under that contract:
four tails are live (0/1/3/4). Layer1's 22 runs mix classes 0 and 6.
Two segments meet the run/tail/material/window contract (2 and 3), but
the certificate's compact ABI caps every post-checksum field at 0xFF and
segment 3's state window starts at 259, so the qualifying segment is 2
(layer2): 5 runs, all class-3 no-Z flag-0, no materials, single live
tail at binding 10, states contiguous 232..258 (27), epochs contiguous
29..32 (4, all asset index 0), 17 triangles, 27 prepared-dense vertices
over 6 offsets, shift census (27, 0), all alpha 0xFF, footprint
40 + 5*2 = 50 bytes. The certificate carries the true ``segment_index``
(2); the ``SEGMENT0_`` define names are Dream Land legacy carried by
the shared renderer.

Promotion: the include is emitted to
``src/nds/nds_native_stage_castle.generated.inc`` and its defines and
objects carry the ``NDS_NATIVE_STAGE_CASTLE_*`` / ``sNdsNativeStageCastle*``
namespace, so it links beside the earlier packets in the one translation
unit (``src/nds/nds_renderer_assets.c``) that holds them. The shared struct
typedefs are emitted only by the primary (Dream Land) include; this file
carries an ``#ifndef``/``#error`` guard that fails the build if it is
included first. ``NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE`` is
deliberately NOT namespaced -- the build sets it with -D for all packets.
Compiled only when ``NDS_P2_STAGE_CASTLE=1``; always generated.

Checker coverage: ``check_nds_native_stage.py --stage castle`` verifies
double-generation determinism, the descriptor structural contract
(counts, asset ids/sizes, capture partition, DObj topology, slab
budget), the segment program's determinism, the descriptor-pinned include
SHA, and the namespaced include's freshness. Dream Land's per-binding
oracles (roots, replay classes, cache clones, depth trace, fail-closed
perturbations, consumed-field manifest) stay Dream Land-only until
per-stage oracle entries land.
"""

from __future__ import annotations

from native_stage_descriptors import StageDescriptor

# Owner ids reuse the generator's OWNER_* numbering: the four layer owners
# keep Dream Land's 0-3 so link/callback conventions stay comparable.
OWNER_LAYER0 = 0
OWNER_LAYER1 = 1
OWNER_LAYER2 = 2
OWNER_LAYER3 = 3

DESCRIPTOR = StageDescriptor(
    name="castle",
    include_sha="9bcd42f01760b1d3c6eaa061bb01d73d8ce9913b4ba51f4e991a911fa1b7d7b8",
    generated_segment_index=2,
    # The packet links beside the earlier ones, so its macros and objects
    # take the namespace the docstring's promotion note reserved for them.
    symbol_prefix="Castle",
    macro_prefix="CASTLE_",
    expected_counts={
        "callbacks": 4,
        "dobjs": 16,
        "bindings": 12,
        "commands": 580,
        "vertex_commands": 38,
        "source_vertices": 254,
        "modify_vertex_commands": 0,
        "triangle_commands": 73,
        "triangles": 136,
        "runs": 40,
        "texture_epochs": 35,
        "material_events": 0,
        "submit_classes": (56, 62, 18),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 287,
        "state_deltas": 111,
        "sync_events": 164,
    },
    o2r_inputs={
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank106",
            "sha256": "68d9697a574df02a72a37877abee5ce0e20b959563da379635a049839d35782a",
            "file_id": 106,
            "internal_fixups": 134,
            "external_fixups": 0,
            "payload_sha256": "fe6bed77c27785f02565806cccfabf2349f6245d89b5cc09f984d365bc069a56",
        },
        "stage_actors": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank156",
            "sha256": "77c24dd2085bfa097c89dad86c9262b9fc71bd387a0f5206b452b897ace6679f",
            "file_id": 156,
            "internal_fixups": 2,
            "external_fixups": 0,
            "payload_sha256": "64271d3da692bc1d505ff3cfc33ada881128ed149cce8e9cbe36da2290ba74bf",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRCastleMap",
            "sha256": "6ba83859d0fc01bcfe5215e85864f61bacd5212d785d84b940b4e09618e9d25c",
            "file_id": 259,
            "internal_fixups": 1,
            "external_fixups": 9,
            "payload_sha256": "244962d1f47b5885ddf1cb41b21ab46df832c2b248f2472c0e00e24262baf74f",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/106_StageCastleFile2.c",
            "sha256": "8057187ecc3e8743b737f7ba72cf47bd332aaf36776e41c4e340a4238ea4cf92",
        },
        "actors_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/156_StageCastleFile3.c",
            "sha256": "c0704b5126401bbeec13220248032af35b729797bfe98ae79a434d1dda6275e0",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/259_GRCastleMap.c",
            "sha256": "3cdfdebe2fa33512febad2f9f572be87419c51128be4553bcf40f52e66cede51",
        },
        "castle": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommon/grcastle.c",
            "sha256": "b7cc432ca544c29e9e0d8f7814ec05ee49b785df41ac4c4fb12b9ba517c900b3",
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
            "MPGroundData dGRCastleMap_header",
            "dStageCastleFile2_Layer0DObj",
            "dStageCastleFile2_Layer1DObj",
            "dStageCastleFile2_Layer2DObj",
            "dStageCastleFile2_Layer3DObj",
        ),
        "castle": (
            "grCastleMakeGround",
            "grCastleInitAll",
            "gcAddAnimJointAll",
            "itManagerMakeItemSetupCommon",
        ),
        "grdisplay": (
            "grDisplayLayer0PriProcDisplay",
            "grDisplayLayer1PriProcDisplay",
            "grDisplayLayer2PriProcDisplay",
            "grDisplayLayer3PriProcDisplay",
            "gcDrawDObjTreeForGObj",
        ),
        "objanim": ("gcPlayAnimAll", "gcParseMObjMatAnimJoint"),
        "objdisplay": (
            "void gcDrawMObjForDObj",
            "gSPSegment(dl_head[0]++, 0xE",
            "void gcDrawDObjTreeForGObj",
        ),
        "reloc_symbols": (
            "llGRCastleMapFileID",
            "llGRCastleMapMapHead",
            "llGRCastleMapMapHeader",
        ),
    },
    map_constructor_text_key="castle",
    map_constructor_token="grCastleMakeGround(",
    map_constructor_min_count=1,
    asset_order=(
        ("stage_geometry", 1),
        ("stage_actors", 1),
        ("stage_map", 4),
    ),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0x1850, 7, 4,
         "grDisplayLayer0PriProcDisplay"),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x2A08, 6, 6,
         "grDisplayLayer1PriProcDisplay"),
        (OWNER_LAYER2, "layer2", "stage_geometry", 0x3218, 4, 13,
         "grDisplayLayer2PriProcDisplay"),
        (OWNER_LAYER3, "layer3", "stage_geometry", 0x3600, 3, 17,
         "grDisplayLayer3PriProcDisplay"),
    ),
    # No MObj structs in file 106 and no segment-E calls in its lists:
    # the packet owns zero material events.
    material_sources=(),
    material_command_partition=(),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 5, 0, 11),
        (OWNER_LAYER1, 6, 5, 4, 11, 22),
        (OWNER_LAYER2, 13, 9, 2, 33, 5),
        (OWNER_LAYER3, 17, 11, 1, 38, 2),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0PriProcDisplay", 4),
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
        ("layer2", "grDisplayLayer2PriProcDisplay", 13),
        ("layer3", "grDisplayLayer3PriProcDisplay", 17),
    ),
    segment0={
        "segment_tuple": (OWNER_LAYER2, 13, 9, 2, 33, 5),
        "state_first": 232,
        "state_count": 27,
        "texture_asset": 0,
        "texture_epoch_count": 4,
        "triangle_count": 17,
        "prepared_dense_count": 27,
        "prepared_dense_offset_count": 6,
        "shift_census": (27, 0),
        "program_footprint": 50,
    },
    # Checker copy of the runtime adapter descriptor the runtime owner will
    # add beside sNdsRendererAdapterNativeStageDreamLand. Counts are the live
    # packet counts; asset ids are the O2R file ids in asset_order; sizes are
    # the pinned payload byte counts (17696/64/192).
    adapter_segment_count=4,
    adapter_dobj_count=16,
    adapter_binding_count=12,
    adapter_asset_count=3,
    adapter_material_count=0,
    adapter_asset_ids=(0x6A, 0x9C, 0x103),
    adapter_asset_sizes=(0x4520, 0x0040, 0x00C0),
)
