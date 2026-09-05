"""Yoshi's Island (internal name Yoster) stage descriptor, P2-4n1 step 4.

Second native-packet descriptor beside the frozen Dream Land one. Every
number below was measured by running the stage generator against the pinned
Yoster inputs, never invented; the probes are recorded in the comments.

Inputs
------
O2R (file id / fixup counts read off each bank's OLER header; SHA-256 over
the file and over its payload measured 2026-09-04):

- stage_images    ExternDataBank110  file 110, internal 0,  external 0
- stage_geometry  ExternDataBank111  file 111, internal 198, external 42
- stage_actors    MiscDataBank154    file 154, internal 16, external 0
- stage_map       GRYosterMap        file 263 (``llGRYosterMapFileID`` is
  ``0x107`` in ``reloc_data.us.h``), internal 1, external 10

Typed/source controls: ``111_StageYosterFile2.c`` (geometry),
``154_StageYosterFile3.c`` (actors), ``263_GRYosterMap.c`` (map header),
``gr/grcommon/gryoster.c`` (stage logic) plus the shared ``grdisplay.c``,
``sys/objanim.c``, ``sys/objdisplay.c`` and ``reloc_data.us.h``.

Owners
------
The four static geometry layers from ``dGRYosterMap_header.gr_desc``
(``263_GRYosterMap.c``). Table offsets and entry counts (sentinel included,
per the Dream Land convention) come from the ``@ 0x...`` comments in
``111_StageYosterFile2.c``:

- layer0 ``dStageYosterFile2_Layer0DObj @ 0xE00`` (19 entries, 18 live,
  15 DL-backed bindings; DObjs 0/14/16 have no display list)
- layer1 ``dStageYosterFile2_Layer1DObj @ 0x4E78`` (6 entries, 5 live,
  1 binding)
- layer2 ``dStageYosterFile2_Layer2DObj @ 0x5448`` (3 entries, 2 live,
  1 binding)
- layer3 ``dStageYosterFile2_Layer3DObj @ 0x5C38`` (4 entries, 3 live,
  2 bindings)

Links and callbacks are the standard ``dGRDisplayDescs`` rows
(``grdisplay.c:10-43``): layer N draws under
``grDisplayLayerNPriProcDisplay`` at link 4/6/13/17 (``layer_mask`` is 0,
so the primary procs apply).

NOT covered: the three cloud platforms. ``grYosterInitAll``
(``gryoster.c:199-257``) builds them at runtime from the
``dStageYosterFile3_DObjDesc_0x0100`` template (``stage_map`` /
``map_nodes``) plus runtime-added ``CloudDisplayList`` children, driven
per-frame by the yakumono collision DObjs. That dynamic composition is not
a static owner table, so the static-owner generator cannot express it;
the packet covers the four static layers (28 DObjs, 19 bindings). The
clouds are recorded here as the known gap, not silently dropped.

Materials
---------
Exactly two display lists call into segment-E material programs, both in
layer0: root ``0xC20`` (DObj 15) and root ``0xD10`` (DObj 17), segment
index 0. Pairing is file-grounded, not guessed:
``dStageYosterFile2_Layer0MObj_MObjSub[18]`` is file-initialised only at
slots 15/17 (see the comment at ``111_StageYosterFile2.c:222``), pointing
through the ``MObj*[2]`` arrays at payload ``0x148``/``0x150`` at the
``MObjSub`` structs at payload ``0x58``/``0xD0``. Both MObjs carry flags
``0x1`` with no palette, i.e. opcode programs ``(0xDE, 0xFD, 0xDF)`` of
length 3 each -- hence ``material_command_partition == (3, 3)``.

Expected counts
---------------
Measured generator output for the structure above (``generate()`` passes
``validate_packet`` with exactly these):

callbacks 4, dobjs 28, bindings 19, commands 592, vertex_commands 53,
source_vertices 331 (modifyvtx 0, so dense == source),
triangle_commands 87, triangles 164, runs 58, texture_epochs 48,
material_events 2, submit classes (raw 35, no-Z 87, range/matrix 42),
cross-matrix (0, 0, 0) -- Yoster's vertex cache never crosses bindings,
state deltas 121, state events 268, state spans 77 (= 58 runs + 19 tails),
sync events 159. Const slab 11236 bytes (< 16 KiB).

Two Yoster shapes the generator had to learn (Dream Land never shows
them; both are no-ops on the Dream Land packet, verified by its frozen
include SHA):

- layer1's list interleaves in-range and out-of-range triangles inside one
  texture epoch, so an epoch's run splits at the submit-class change;
- the lists execute SETENVCOLOR/MTX/MOVEWORD/MOVEMEM/SETPRIMDEPTH/
  SETFOGCOLOR/LOADTILE in state-affecting positions, which now hash
  through the state-effect table like PRIM/BLEND already did.

Generated segment program
-------------------------
The straight-line program keeps its Dream Land contract (one submit class
per run, no material ownership/consumption, single live tail at the last
binding, contiguous state/epoch windows); only the window numbers are
per-stage. Segment 0 (layer0) is NOT programmable under that contract:
bindings 13/14 own material events 0/1, three tails are live (12/13/14),
its 133 states are not contiguous from 0, and its states consume MATERIAL
and PRIM effects. Layer1's 22 runs mix classes 0 and 6. The qualifying
segment is 3 (layer3): 11 runs, all class-3 no-Z flag-0, no materials,
single live tail at binding 18, states contiguous 219..267 (49), epochs
contiguous 39..47 (9, all asset index 1), 32 triangles, 63 prepared-dense
vertices over 12 offsets, shift census (45, 18), all alpha 0xFF,
footprint 40 + 11*2 = 62 bytes. The certificate carries the true
``segment_index`` (3); the ``SEGMENT0_`` define names are Dream Land
legacy carried by the shared renderer.

Promotion (P2-4n1 step 5, 2026-09-04): DONE. The include is emitted to
``src/nds/nds_native_stage_yoster.generated.inc`` and its defines and
objects carry the ``NDS_NATIVE_STAGE_YOSTER_*`` / ``sNdsNativeStageYoster*``
namespace, so it links beside Dream Land's packet in the one translation
unit (``src/nds/nds_renderer_assets.c``) that holds both. The shared struct
typedefs are emitted only by the primary (Dream Land) include; this file
carries an ``#ifndef``/``#error`` guard that fails the build if it is
included first. ``NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE`` is
deliberately NOT namespaced -- the build sets it with -D for both packets.
Compiled only when ``NDS_P2_STAGE_YOSTER=1``; always generated.

Checker coverage: ``check_nds_native_stage.py --stage yoster`` verifies
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
    name="yoster",
    include_sha="c42787b2401e7a318e460aa45b5320f044f65bb51e0a51332945e23105af1f68",
    generated_segment_index=3,
    # P2-4n1 step 5: the packet now links, so its macros and objects take the
    # namespace the docstring's promotion note reserved for them.
    symbol_prefix="Yoster",
    macro_prefix="YOSTER_",
    expected_counts={
        "callbacks": 4,
        "dobjs": 28,
        "bindings": 19,
        "commands": 592,
        "vertex_commands": 53,
        "source_vertices": 331,
        "modify_vertex_commands": 0,
        "triangle_commands": 87,
        "triangles": 164,
        "runs": 58,
        "texture_epochs": 48,
        "material_events": 2,
        "submit_classes": (35, 87, 42),
        "cross_runs": 0,
        "cross_tris": 0,
        "cross_corners": 0,
        "state_events": 268,
        "state_deltas": 121,
        "sync_events": 159,
    },
    o2r_inputs={
        "stage_images": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank110",
            "sha256": "bb17c238cdd8ea96bb1205370064d55a4f7708fa2db805c74ed6326bee88106f",
            "file_id": 110,
            "internal_fixups": 0,
            "external_fixups": 0,
            "payload_sha256": "a29e56feb0dac75bc01b6a1acfb12e81b4380aba99ce02e6662dead9917574e8",
        },
        "stage_geometry": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank111",
            "sha256": "5f348389c800947ce9f62174f1c2a99ca7b9ea9d4eed6235656ae279c568ccf9",
            "file_id": 111,
            "internal_fixups": 198,
            "external_fixups": 42,
            "payload_sha256": "cbe83fc302b56b357782aa1d964b02e6f1ed62bcc1582b1fb863d8a0e15c973b",
        },
        "stage_actors": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank154",
            "sha256": "d8f37b0cbdc8c4c73039f13bbb0553a798fbc88ffdd65f67081cc19bb7f09f85",
            "file_id": 154,
            "internal_fixups": 16,
            "external_fixups": 0,
            "payload_sha256": "cf19db42652cde613960dd30f5f1c20ac8ccc58b68c425a7070306c819002617",
        },
        "stage_map": {
            "path": "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRYosterMap",
            "sha256": "786a07949d794cd98e4ef784a00e6ecc52d1ef1cf52d940a4a6213f2609340ae",
            "file_id": 263,
            "internal_fixups": 1,
            "external_fixups": 10,
            "payload_sha256": "1a4496a01999ebcd751ab5cd535b59b69c278e802055b9331a02100242e7d827",
        },
    },
    text_inputs={
        "geometry_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/111_StageYosterFile2.c",
            "sha256": "c3bcac2cbbf09a39ed3f1304759d0636e700fc8ba2e4b20704838ef079d750e8",
        },
        "actors_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/154_StageYosterFile3.c",
            "sha256": "f353ac6cb9747ce734ed4ddb34fd6f6798d8f38f0c0ae882681e5bd91cc54c6b",
        },
        "map_typed": {
            "path": "decomp/BattleShip-main/decomp/src/relocData/263_GRYosterMap.c",
            "sha256": "7af3e7110f143504fae64d70a72434aa268a49675f45920ae4427c775960e6e9",
        },
        "yoster": {
            "path": "decomp/BattleShip-main/decomp/src/gr/grcommon/gryoster.c",
            "sha256": "878e397050f16620f9e5b4397e64eb71e66079cadb87b68b461186cbc52dc3da",
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
            "MPGroundData dGRYosterMap_header",
            "dStageYosterFile2_Layer0DObj",
            "dStageYosterFile2_Layer1DObj",
            "dStageYosterFile2_Layer2DObj",
            "dStageYosterFile2_Layer3DObj",
        ),
        "yoster": (
            "grYosterMakeGround",
            "grYosterInitAll",
            "gcAddAnimJointAll",
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
            "llGRYosterMapFileID",
            "llGRYosterMapMapHead",
            "llGRYosterMapCloudDisplayList",
        ),
    },
    map_constructor_text_key="yoster",
    map_constructor_token="grYosterMakeGround(",
    map_constructor_min_count=1,
    asset_order=(
        ("stage_images", 2),
        ("stage_geometry", 1),
        ("stage_actors", 1),
        ("stage_map", 4),
    ),
    # (owner, name, resource_name, dobj_offset, descriptor_count, link, callback)
    owner_specs=(
        (OWNER_LAYER0, "layer0", "stage_geometry", 0xE00, 19, 4,
         "grDisplayLayer0PriProcDisplay"),
        (OWNER_LAYER1, "layer1", "stage_geometry", 0x4E78, 6, 6,
         "grDisplayLayer1PriProcDisplay"),
        (OWNER_LAYER2, "layer2", "stage_geometry", 0x5448, 3, 13,
         "grDisplayLayer2PriProcDisplay"),
        (OWNER_LAYER3, "layer3", "stage_geometry", 0x5C38, 4, 17,
         "grDisplayLayer3PriProcDisplay"),
    ),
    # (asset_id, binding_root, mobj_offset); DObjs 15/17 <-> MObjs 0x58/0xD0
    # via Layer0MObj_MObjSub slots 15/17, the only file-initialised slots.
    material_sources=(
        (111, 0xC20, 0x58),
        (111, 0xD10, 0xD0),
    ),
    material_command_partition=(3, 3),
    # (owner, link, first_binding, binding_count, first_run, run_count)
    segment_partition=(
        (OWNER_LAYER0, 4, 0, 15, 0, 23),
        (OWNER_LAYER1, 6, 15, 1, 23, 22),
        (OWNER_LAYER2, 13, 16, 1, 45, 2),
        (OWNER_LAYER3, 17, 17, 2, 47, 11),
    ),
    # (name, callback, link) sorted by owner id
    callback_partition=(
        ("layer0", "grDisplayLayer0PriProcDisplay", 4),
        ("layer1", "grDisplayLayer1PriProcDisplay", 6),
        ("layer2", "grDisplayLayer2PriProcDisplay", 13),
        ("layer3", "grDisplayLayer3PriProcDisplay", 17),
    ),
    segment0={
        "segment_tuple": (OWNER_LAYER3, 17, 17, 2, 47, 11),
        "state_first": 219,
        "state_count": 49,
        "texture_asset": 1,
        "texture_epoch_count": 9,
        "triangle_count": 32,
        "prepared_dense_count": 63,
        "prepared_dense_offset_count": 12,
        "shift_census": (45, 18),
        "program_footprint": 62,
    },
    # Checker copy of the runtime adapter descriptor the runtime owner will
    # add beside sNdsRendererAdapterNativeStageDreamLand. Counts are the live
    # packet counts; asset ids are the O2R file ids in asset_order; sizes are
    # the pinned payload byte counts (21040/47408/1712/192).
    adapter_segment_count=4,
    adapter_dobj_count=28,
    adapter_binding_count=19,
    adapter_asset_count=4,
    adapter_material_count=2,
    adapter_asset_ids=(0x6E, 0x6F, 0x9A, 0x107),
    adapter_asset_sizes=(0x5230, 0xB930, 0x06B0, 0x00C0),
)
