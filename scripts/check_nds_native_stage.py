#!/usr/bin/env python3
"""Falsify drift in the host-exact Dream Land whole-stage packet.

This checker deliberately does not build or launch a ROM.  It regenerates the
packet twice from immutable BattleShip/O2R inputs, checks the committed include
byte-for-byte, and verifies the exact callback, topology, display-list, run,
texture-epoch, material, executable-state, fail-closed, and cross-binding
vertex-cache contracts needed by a later DS-native consumer.
"""

from __future__ import annotations

import json
import shutil
import sys
import tempfile
from collections import Counter
from dataclasses import dataclass, replace
from pathlib import Path

import generate_nds_native_stage as generator


EXPECTED_ROOTS = (
    0x06C0, 0x0798, 0x0820, 0x08A8, 0x0928, 0x0950, 0x0980,
    0x0A18, 0x0A38, 0x0A58, 0x0A78, 0x0B18, 0x0B40, 0x0C38,
    0x0CC0, 0x0D20, 0x0D80, 0x0DE0, 0x0E40, 0x0EA0, 0x0FF8,
    0x1558, 0x1630, 0x16C0, 0x1720, 0x2830, 0x28B0, 0x2958,
    0x29A8, 0x18E0, 0x21E8, 0x22C8, 0x2380, 0x2EB0, 0x2F30,
    0x2FD8, 0x3028, 0x30D0, 0x3120, 0x2970, 0x2A48, 0x2A68,
)

EXPECTED_COMMANDS = (
    27, 17, 17, 16, 5, 6, 19, 4, 4, 4, 20, 5, 31, 17, 12, 12,
    12, 12, 12, 45, 34, 27, 21, 12, 10, 16, 21, 10, 27, 128, 28,
    33, 36, 16, 21, 10, 21, 10, 27, 27, 4, 50,
)

EXPECTED_VERTEX_COMMANDS = (
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 1, 1, 1, 1, 1,
    3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 11, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 2,
)

EXPECTED_SOURCE_VERTICES = (
    4, 4, 5, 5, 5, 7, 3, 3, 4, 4, 5, 5, 22, 3, 3, 3, 3, 3, 3,
    14, 6, 4, 4, 4, 4, 3, 2, 3, 2, 86, 7, 8, 8, 3, 2, 3, 2, 3,
    2, 4, 4, 30,
)

EXPECTED_TRIANGLE_COMMANDS = (
    1, 1, 2, 2, 2, 3, 1, 1, 1, 1, 2, 2, 7, 1, 1, 1, 1, 1, 1,
    4, 2, 1, 1, 1, 1, 1, 1, 1, 1, 38, 3, 3, 3, 1, 1, 1, 1, 1,
    1, 1, 1, 12,
)

EXPECTED_TRIANGLES = (
    2, 2, 3, 3, 3, 5, 1, 1, 2, 2, 3, 3, 12, 1, 1, 1, 1, 1, 1,
    6, 4, 2, 2, 2, 2, 1, 2, 1, 2, 76, 5, 6, 6, 1, 2, 1, 2, 1,
    2, 2, 2, 24,
)

EXPECTED_RUNS = (
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 1, 1, 1, 1, 1,
    3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 2,
)

EXPECTED_EPOCHS = (
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 2,
)

EXPECTED_SEGMENTS = (
    # first DObj, DObjs, owner, link, first binding, bindings, geometry, run, runs
    (0, 21, 0, 4, 0, 20, 0, 0, 26),
    (21, 3, 4, 4, 20, 1, 0, 26, 1),
    (24, 6, 5, 4, 21, 4, 0, 27, 4),
    (30, 7, 6, 4, 25, 4, 0, 31, 4),
    (37, 2, 1, 6, 29, 1, 1, 35, 6),
    (39, 4, 2, 13, 30, 3, 0, 41, 3),
    (43, 10, 7, 16, 33, 6, 0, 44, 6),
    (53, 4, 3, 17, 39, 3, 0, 50, 4),
)

EXPECTED_RUN_TRIANGLES = (
    2, 2, 3, 3, 3, 5, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3, 2, 1,
    1, 1, 1, 1, 1, 4, 1, 1, 4, 2, 2, 2, 2, 1, 2, 1, 2, 12,
    38, 16, 4, 4, 2, 5, 6, 6, 1, 2, 1, 2, 1, 2, 2, 2, 12, 12,
)

EXPECTED_RUN_CLASSES = (
    *(generator.SUBMIT_PROJECTED_NO_Z for _ in range(35)),
    generator.SUBMIT_RAW_CURRENT,
    generator.SUBMIT_RAW_CURRENT,
    generator.SUBMIT_RAW_CURRENT,
    generator.SUBMIT_PROJECTED_RANGE_OR_MATRIX,
    generator.SUBMIT_PROJECTED_RANGE_OR_MATRIX,
    generator.SUBMIT_PROJECTED_RANGE_OR_MATRIX,
    *(generator.SUBMIT_PROJECTED_NO_Z for _ in range(13)),
)

EXPECTED_DENSE_FIRST_VISIT_OFFSETS = (
    0, 4, 8, 13, 18, 23, 30, 33, 36, 40, 44, 49, 54, 58, 62, 67, 72,
    76, 79, 82, 85, 88, 91, 94, 102, 105, 108, 114, 118, 122, 126, 130,
    133, 137, 140, 144, 168, 200, 210, 218, 226, 230, 237, 245, 253, 256,
    260, 263, 267, 270, 274, 278, 282, 298, 312,
)

EXPECTED_STAGE_DEPTH_TRACE_HASH = 0x3BB26905

EXPECTED_PROJECTED_CROSS_MATRIX_RUNS = (32, 34, 45, 47, 49)
EXPECTED_PROJECTED_CROSS_MATRIX_TRIANGLES = 10
EXPECTED_PROJECTED_CROSS_MATRIX_FOREIGN_CORNERS = 15

# gSPModifyVertex(ST) clone index -> source cache vertex and exact new ST.
EXPECTED_CACHE_CLONES = (
    (133, 130, 0, 0),
    (134, 131, 4095, 0),
    (140, 137, 6143, 0),
    (141, 138, 0, 0),
    (256, 253, 8191, 0),
    (257, 254, 0, 0),
    (263, 260, 0, 0),
    (264, 261, 4095, 0),
    (270, 267, 2047, 0),
    (271, 268, 0, 0),
)

EXPECTED_REPLAY_CLASSES = Counter(
    state=423,
    sync=223,
    vertex=69,
    triangle=113,
    control=58,
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise generator.falsify(message)


def fields(rows: tuple[object, ...], name: str) -> tuple[int, ...]:
    return tuple(getattr(row, name) for row in rows)


def verify_input_falsifier(repo_root: Path) -> None:
    """Prove that a one-byte O2R mutation is rejected before packet parsing."""

    spec = generator.O2R_INPUTS["stage_geometry"]
    source = repo_root / spec.path
    with tempfile.TemporaryDirectory(prefix="smash64ds-m3-falsifier-") as temp:
        temp_root = Path(temp)
        target = temp_root / spec.path
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        payload = bytearray(target.read_bytes())
        payload[-1] ^= 0x01
        target.write_bytes(payload)
        try:
            generator.checked_bytes(temp_root, spec)
        except generator.Falsifier as exc:
            require("SHA256" in str(exc), "mutated input failed for the wrong reason")
        else:
            raise generator.falsify("one-byte O2R mutation was accepted")


def verify_consumed_fields_manifest(repo_root: Path) -> int:
    first = generator.render_consumed_fields_manifest(repo_root)
    second = generator.render_consumed_fields_manifest(repo_root)
    require(first == second, "consumed-field manifest is nondeterministic")
    output = repo_root / generator.DEFAULT_CONSUMED_FIELDS_OUTPUT
    require(output.is_file(), f"consumed-field manifest is absent: {output}")
    require(output.read_bytes() == first, "consumed-field manifest is stale")
    manifest = json.loads(first)
    require(
        tuple(manifest["allowed_classifications"]) == generator.FIELD_CLASSES,
        "consumed-field classification vocabulary drifted",
    )
    require(
        [row["index"] for row in manifest["task26_live_operand_order"]]
        == list(range(4)),
        "Task 26 live-operand order is not dense",
    )
    require(
        [row["segment"] for row in manifest["callback_commit_order"]]
        == [1, 2, 5],
        "callback material commit order drifted",
    )
    require(
        [row["slots"] for row in manifest["callback_commit_order"]]
        == [[0], [1], [2, 3]],
        "callback material slot order drifted",
    )
    require(
        manifest["census_dimensions"]
        == {
            "segments": 8,
            "material_snapshots": 4,
            "windows": [
                "Countdown",
                "early",
                "Whispy Wait-to-Open/material-animation boundary",
                "Whispy steady/post-change",
                "late",
                "KO",
                "rebirth",
                "Time Up/Results",
            ],
        },
        "Task 23R census dimensions drifted",
    )
    bound_closures = {
        row["closure"] for row in manifest["source_closures"]
    }
    for group in manifest["transitive_inputs"]:
        require(
            set(group["closures"]) <= bound_closures,
            f"transitive group {group['name']} has an unbound closure",
        )
    try:
        observed = generator.observed_arrow_fields(
            "{ tracked->known; synthetic->unclassified; }",
            ("tracked",),
        )
        generator.validate_observed_field_policy(
            {"tracked.known": generator.FIELD_CLASS_LIVE},
            observed,
            "in-memory falsifier",
        )
    except generator.Falsifier as exc:
        require(
            "unclassified reads" in str(exc),
            "unclassified-read falsifier failed for the wrong reason",
        )
    else:
        raise generator.falsify("unclassified source read was accepted")
    return sum(len(row["fields"]) for row in manifest["source_closures"])


def verify_packet(packet: generator.Packet) -> None:
    bindings = packet.bindings
    require(fields(bindings, "root_offset") == EXPECTED_ROOTS, "binding roots drifted")
    require(
        fields(bindings, "source_command_count") == EXPECTED_COMMANDS,
        "per-binding source command partition drifted",
    )
    require(
        fields(bindings, "vertex_command_count") == EXPECTED_VERTEX_COMMANDS,
        "per-binding VTX command partition drifted",
    )
    require(
        fields(bindings, "source_vertex_count") == EXPECTED_SOURCE_VERTICES,
        "per-binding source vertex partition drifted",
    )
    require(
        fields(bindings, "triangle_command_count") == EXPECTED_TRIANGLE_COMMANDS,
        "per-binding triangle-command partition drifted",
    )
    require(
        fields(bindings, "triangle_count") == EXPECTED_TRIANGLES,
        "per-binding triangle partition drifted",
    )
    require(fields(bindings, "run_count") == EXPECTED_RUNS, "run partition drifted")
    require(
        fields(bindings, "texture_epoch_count") == EXPECTED_EPOCHS,
        "texture epoch partition drifted",
    )

    segments = tuple(
        (
            row.first_dobj,
            row.dobj_count,
            row.owner,
            row.link,
            row.first_binding,
            row.binding_count,
            row.initial_geometry,
            row.first_run,
            row.run_count,
        )
        for row in packet.segments
    )
    require(segments == EXPECTED_SEGMENTS, "callback capture partition drifted")

    material_contract = tuple(
        (
            row.mobj_offset,
            row.binding_index,
            row.asset_index,
            row.material_slot,
            row.segment_index,
            row.source_command_count,
            row.flags,
            row.opcodes,
        )
        for row in packet.materials
    )
    require(
        material_contract
        == (
            (0x0F18, 20, 2, 0, 0, 3, 0x0001, (0xDE, 0xFD, 0xDF)),
            (0x13D8, 22, 2, 1, 0, 3, 0x0001, (0xDE, 0xFD, 0xDF)),
            (
                0x1F78,
                31,
                1,
                2,
                0,
                10,
                0x006B,
                (0xDE, 0xFA, 0xFD, 0xE6, 0xF3, 0xE6, 0xFD, 0xF2, 0xF2, 0xDF),
            ),
            (
                0x1FF0,
                32,
                1,
                3,
                0,
                10,
                0x006B,
                (0xDE, 0xFA, 0xFD, 0xE6, 0xF3, 0xE6, 0xFD, 0xF2, 0xF2, 0xDF),
            ),
        ),
        "segment-E material expansion drifted",
    )

    require(
        tuple(row.triangle_count for row in packet.runs) == EXPECTED_RUN_TRIANGLES,
        "exact run triangle lengths drifted",
    )
    require(
        tuple(row.submit_class for row in packet.runs) == EXPECTED_RUN_CLASSES,
        "run submit classes drifted",
    )
    class_triangles = Counter()
    for row in packet.runs:
        class_triangles[row.submit_class] += row.triangle_count
    require(
        class_triangles
        == Counter(
            {
                generator.SUBMIT_RAW_CURRENT: 66,
                generator.SUBMIT_PROJECTED_NO_Z: 126,
                generator.SUBMIT_PROJECTED_RANGE_OR_MATRIX: 10,
            }
        ),
        "submit-class triangle totals drifted",
    )

    dense_contexts: dict[int, tuple[int, int, int, int, int]] = {}
    dense_first_visits: list[int] = []
    dense_first_visit_offsets = [0]
    dense_references = 0
    projected_references = 0
    projected_dense: set[int] = set()
    for run_index, run in enumerate(packet.runs):
        context = (
            run.binding_index,
            run.texture_epoch,
            run.submit_class,
            run.state_policy,
            run.flags,
        )
        run_alphas: set[int] = set()
        for dense_index in packet.corners[
            run.first_corner : run.first_corner + run.triangle_count * 3
        ]:
            dense_references += 1
            if dense_index not in dense_contexts:
                dense_first_visits.append(dense_index)
            previous = dense_contexts.setdefault(dense_index, context)
            require(
                previous == context,
                f"dense vertex {dense_index}: preparation context conflicts",
            )
            run_alphas.add(packet.vertices[dense_index].rgba & 0xFF)
            if run.submit_class != generator.SUBMIT_RAW_CURRENT:
                projected_references += 1
                projected_dense.add(dense_index)
        require(
            len(run_alphas) == 1,
            f"run {run_index}: source vertex alpha is not uniform",
        )
        dense_first_visit_offsets.append(len(dense_first_visits))
    require(
        (
            dense_references,
            len(dense_contexts),
            projected_references,
            len(projected_dense),
        )
        == (606, 312, 408, 246),
        "dense prepare-once reference census drifted",
    )
    require(
        tuple(dense_first_visit_offsets) == EXPECTED_DENSE_FIRST_VISIT_OFFSETS,
        "dense first-visit run offsets drifted",
    )
    require(
        tuple(sorted(dense_first_visits)) == tuple(range(len(packet.vertices))),
        "dense first-visit plan is not an exact vertex permutation",
    )
    require(
        Counter(
            generator.stage_vertex_coordinate_shift(vertex)
            for vertex in packet.vertices
        )
        == Counter({0: 257, 1: 55}),
        "AOT stage coordinate-shift census drifted",
    )

    # The DS cannot disable depth per polygon, so the no-Z callbacks use two
    # synthetic painter bands around the live source-Z layer.  Hash the exact
    # packet order with the submitted depths used by the profile-2 runtime
    # trace; every no-Z triangle must consume one distinct v16 value.
    trace_hash = 2166136261
    trace_index = 0
    depth = 0x1000 * 6
    source_z_seen = False
    background_depths: list[int] = []
    foreground_depths: list[int] = []
    for segment in packet.segments:
        for run_index in range(segment.first_run, segment.first_run + segment.run_count):
            run = packet.runs[run_index]
            for _triangle_index in range(run.triangle_count):
                phase = 0
                projected_z = 0
                source_zbuffered = int(
                    run.submit_class != generator.SUBMIT_PROJECTED_NO_Z
                )
                if source_zbuffered:
                    if not source_z_seen:
                        depth = (128 - 0x1000) * 6
                        source_z_seen = True
                else:
                    depth -= 6
                    projected_z = depth // 6
                    phase = 2 if source_z_seen else 1
                    (foreground_depths if phase == 2 else background_depths).append(
                        projected_z
                    )
                trace_hash = generator.fnv1a_u32(
                    (
                        0x53445031,
                        trace_index,
                        run.submit_class,
                        source_zbuffered,
                        phase,
                        projected_z,
                        projected_z,
                        projected_z,
                    ),
                    trace_hash,
                )
                trace_index += 1
    require(trace_index == 202, "stage depth trace triangle count drifted")
    require(
        background_depths == list(range(4095, 4023, -1)),
        "background no-Z painter depths are not strict/source-ordered",
    )
    require(
        foreground_depths == list(range(-3969, -4023, -1)),
        "foreground no-Z painter depths are not strict/source-ordered",
    )
    require(
        trace_hash == EXPECTED_STAGE_DEPTH_TRACE_HASH,
        f"stage depth trace hash 0x{trace_hash:08x} drifted",
    )

    expected_run_flags = tuple(
        generator.RUN_FLAG_PROJECTED_CROSS_MATRIX
        if index in EXPECTED_PROJECTED_CROSS_MATRIX_RUNS
        else 0
        for index in range(len(packet.runs))
    )
    require(fields(packet.runs, "flags") == expected_run_flags, "run flags drifted")
    flagged_runs = []
    cross_matrix_triangles = 0
    cross_matrix_foreign_corners = 0
    raw_cross_matrix_triangles = 0
    for run_index, run in enumerate(packet.runs):
        run_corners = packet.corners[
            run.first_corner : run.first_corner + run.triangle_count * 3
        ]
        run_cross_matrix_triangles = 0
        run_foreign_corners = 0
        for first_corner in range(0, len(run_corners), 3):
            foreign_corners = sum(
                packet.vertices[dense_index].matrix_binding != run.binding_index
                for dense_index in run_corners[first_corner : first_corner + 3]
            )
            if foreign_corners:
                run_cross_matrix_triangles += 1
                run_foreign_corners += foreign_corners
                if run.submit_class == generator.SUBMIT_RAW_CURRENT:
                    raw_cross_matrix_triangles += 1
        if run.flags & generator.RUN_FLAG_PROJECTED_CROSS_MATRIX:
            flagged_runs.append(run_index)
            require(
                run.submit_class == generator.SUBMIT_PROJECTED_NO_Z,
                f"run {run_index}: cross-matrix path is not projected/no-Z",
            )
            require(
                run_cross_matrix_triangles == run.triangle_count,
                f"run {run_index}: flagged run mixes matrix ownership",
            )
        require(
            bool(run_cross_matrix_triangles)
            == bool(run.flags & generator.RUN_FLAG_PROJECTED_CROSS_MATRIX),
            f"run {run_index}: independently derived cross-matrix state disagrees",
        )
        cross_matrix_triangles += run_cross_matrix_triangles
        cross_matrix_foreign_corners += run_foreign_corners
    require(
        tuple(flagged_runs) == EXPECTED_PROJECTED_CROSS_MATRIX_RUNS,
        "projected cross-matrix run identities drifted",
    )
    require(
        (
            cross_matrix_triangles,
            cross_matrix_foreign_corners,
            raw_cross_matrix_triangles,
        )
        == (
            EXPECTED_PROJECTED_CROSS_MATRIX_TRIANGLES,
            EXPECTED_PROJECTED_CROSS_MATRIX_FOREIGN_CORNERS,
            0,
        ),
        "cross-matrix triangle/corner contract drifted",
    )

    # Every source display list is selected by exactly one live DObj, and its
    # parent/depth relation stays inside the callback's contiguous DObj span.
    mapped = [row.binding_index for row in packet.dobjs if row.binding_index != generator.INVALID_U16]
    require(sorted(mapped) == list(range(42)), "DObj/list ownership is not bijective")
    for segment in packet.segments:
        end = segment.first_dobj + segment.dobj_count
        for index in range(segment.first_dobj, end):
            row = packet.dobjs[index]
            require(row.owner == segment.owner, f"DObj {index} crossed callback owner")
            if row.parent_index == generator.INVALID_U16:
                require(row.depth == 0, f"DObj {index} root depth changed")
            else:
                require(
                    segment.first_dobj <= row.parent_index < index,
                    f"DObj {index} parent escaped/precedes topology",
                )
                require(
                    packet.dobjs[row.parent_index].depth + 1 == row.depth,
                    f"DObj {index} depth no longer follows its parent",
                )

    require(len(packet.vertices) == 312, "clone-expanded dense vertex count drifted")
    require(sum(EXPECTED_SOURCE_VERTICES) == 302, "source vertex oracle is corrupt")
    for clone_index, source_index, expected_s, expected_t in EXPECTED_CACHE_CLONES:
        clone = packet.vertices[clone_index]
        source = packet.vertices[source_index]
        require(
            (clone.x, clone.y, clone.z, clone.rgba, clone.matrix_binding, clone.cache_slot)
            == (
                source.x,
                source.y,
                source.z,
                source.rgba,
                source.matrix_binding,
                source.cache_slot,
            ),
            f"cache clone {clone_index} no longer preserves its source vertex",
        )
        require(
            (clone.s, clone.t) == (expected_s, expected_t),
            f"cache clone {clone_index} ST drifted",
        )

    require(len(packet.policies) == 41, "state-policy count drifted")
    require(len(packet.state_deltas) == 148, "shared state-delta count drifted")
    require(len(packet.state_sequence) == 423, "state sequence count drifted")
    require(len(packet.state_spans) == 96, "run/tail state-span count drifted")
    require(
        sum(span.sync_count for span in packet.state_spans) == 223,
        "collapsed sync-command count drifted",
    )
    require(packet.slab_bytes() == 12663, "whole-stage slab byte count drifted")
    require(packet.slab_bytes() <= 16 * 1024, "whole-stage slab exceeds 16 KiB")


def state_span_oracle(
    events: list[generator.CommandEvent],
) -> tuple[tuple[tuple[int, int], ...], tuple[int, int]]:
    spans: list[tuple[int, int]] = []
    pending_state = 0
    pending_sync = 0
    in_run = False
    for event in events:
        if event.op not in (generator.OP_TRI1, generator.OP_TRI2):
            in_run = False
        if event.op in generator.STATE_OPS or event.material_event != generator.INVALID_U8:
            pending_state += 1
        elif event.op in generator.SYNC_OPS:
            pending_sync += 1
        if event.op in (generator.OP_TRI1, generator.OP_TRI2) and not in_run:
            spans.append((pending_state, pending_sync))
            pending_state = 0
            pending_sync = 0
            in_run = True
    return tuple(spans), (pending_state, pending_sync)


def verify_command_replay(packet: generator.Packet, repo_root: Path) -> int:
    resources = {
        name: generator.load_o2r(repo_root, spec)
        for name, spec in generator.O2R_INPUTS.items()
    }
    asset_index = {asset.asset_id: index for index, asset in enumerate(packet.assets)}
    binding_order = []
    for owner in generator.OWNER_SPECS:
        resource = resources[owner.resource_name]
        binding_order.extend(
            (owner, resource, root)
            for root in generator.selected_roots(resource, owner)
        )
    binding_lookup = {
        (resource.file_id, root): index
        for index, (_owner, resource, root) in enumerate(binding_order)
    }
    materials = generator.build_material_events(
        resources, binding_lookup, asset_index
    )
    material_by_binding = {
        event.binding_index: index for index, event in enumerate(materials)
    }

    command_classes: Counter[str] = Counter()
    sequence_cursor = 0
    binding_cursor = 0
    for owner in generator.OWNER_SPECS:
        resource = resources[owner.resource_name]
        source_state = generator.SourceState(
            generator.GEOMETRY_ZBUFFER if owner.link == 6 else 0,
            state_hash=generator.fnv1a_u32((0x53454733, owner.owner, owner.link)),
            texture_hash=generator.fnv1a_u32((0x54455833, owner.owner, owner.link)),
        )
        replay_state = replace(source_state)
        for root in generator.selected_roots(resource, owner):
            binding = packet.bindings[binding_cursor]
            material_index = material_by_binding.get(
                binding_cursor, generator.INVALID_U8
            )
            events = generator.walk_display_list(
                resource, root, material_index, materials
            )
            require(
                generator.event_checksum(resource, events)
                == binding.traversal_checksum,
                f"binding {binding_cursor}: 886-command checksum parity failed",
            )

            run_spans, tail_span = state_span_oracle(events)
            require(
                len(run_spans) == binding.run_count,
                f"binding {binding_cursor}: replay run partition changed",
            )
            for local_run, expected_span in enumerate(run_spans):
                actual_span = packet.state_spans[binding.first_run + local_run]
                require(
                    (actual_span.state_count, actual_span.sync_count)
                    == expected_span,
                    f"binding {binding_cursor}: run {local_run} state span changed",
                )
            actual_tail = packet.state_spans[len(packet.runs) + binding_cursor]
            require(
                (actual_tail.state_count, actual_tail.sync_count) == tail_span,
                f"binding {binding_cursor}: tail state span changed",
            )

            in_run = False
            run_cursor = 0
            for event in events:
                op = event.op
                if op not in (generator.OP_TRI1, generator.OP_TRI2):
                    in_run = False
                generator.apply_source_state(resource, source_state, event)
                if op in generator.STATE_OPS or event.material_event != generator.INVALID_U8:
                    command_classes["state"] += 1
                    require(
                        sequence_cursor < len(packet.state_sequence),
                        "state replay exhausted the generated sequence",
                    )
                    delta_index = packet.state_sequence[sequence_cursor]
                    expected_delta = generator.compile_state_delta(
                        resource, event, asset_index
                    )
                    require(
                        packet.state_deltas[delta_index] == expected_delta,
                        f"command {sum(command_classes.values())}: state delta parity failed",
                    )
                    generator.apply_compiled_state_delta(
                        packet.assets, replay_state, packet.state_deltas[delta_index]
                    )
                    sequence_cursor += 1
                elif op in generator.SYNC_OPS:
                    command_classes["sync"] += 1
                elif op in (generator.OP_VTX, generator.OP_MODIFYVTX):
                    command_classes["vertex"] += 1
                elif op in (generator.OP_TRI1, generator.OP_TRI2):
                    command_classes["triangle"] += 1
                elif op in (generator.OP_DL, generator.OP_ENDDL):
                    command_classes["control"] += 1
                else:
                    raise generator.falsify(
                        f"unclassified replay opcode 0x{op:02x}"
                    )
                require(
                    source_state.policy() == replay_state.policy(),
                    f"command {sum(command_classes.values())}: executable state diverged",
                )
                if op in (generator.OP_TRI1, generator.OP_TRI2) and not in_run:
                    run = packet.runs[binding.first_run + run_cursor]
                    require(
                        packet.policies[run.state_policy] == replay_state.policy(),
                        f"binding {binding_cursor}: run {run_cursor} policy diverged",
                    )
                    run_cursor += 1
                    in_run = True
            require(
                run_cursor == binding.run_count,
                f"binding {binding_cursor}: replay did not consume every run",
            )
            binding_cursor += 1

    require(binding_cursor == len(packet.bindings), "replay missed a binding")
    require(
        sequence_cursor == len(packet.state_sequence),
        "replay missed executable state entries",
    )
    require(
        command_classes == EXPECTED_REPLAY_CLASSES,
        f"886-command class partition changed: {command_classes}",
    )
    command_count = sum(command_classes.values())
    require(command_count == 886, f"replayed {command_count} commands, expected 886")
    return command_count


@dataclass
class CommitProbe:
    armed: int = 0
    segments_emitted: int = 0
    gx_mutations: int = 0

    def preflight(
        self, candidate: generator.Packet, reference: generator.Packet
    ) -> None:
        generator.validate_packet(candidate)
        verify_packet(candidate)
        require(candidate == reference, "immutable transaction packet changed")
        self.armed = 1

    def commit(self, packet: generator.Packet) -> None:
        require(self.armed == 1, "unarmed transaction reached commit")
        self.segments_emitted = len(packet.segments)
        self.gx_mutations = len(packet.segments)


def mutate_row(
    packet: generator.Packet, field: str, index: int, **changes: int
) -> generator.Packet:
    rows = list(getattr(packet, field))
    rows[index] = replace(rows[index], **changes)
    return replace(packet, **{field: tuple(rows)})


def verify_fail_closed(packet: generator.Packet) -> int:
    bad_sequence = list(packet.state_sequence)
    bad_sequence[0] = len(packet.state_deltas)
    bad_corners = list(packet.corners)
    bad_corners[0] = len(packet.vertices)
    mutations = (
        (
            "asset provenance",
            mutate_row(
                packet,
                "assets",
                0,
                payload_checksum=packet.assets[0].payload_checksum ^ 1,
            ),
        ),
        ("callback link", mutate_row(packet, "segments", 0, link=5)),
        (
            "DObj topology",
            mutate_row(
                packet, "dobjs", 1, parent_index=generator.INVALID_U16
            ),
        ),
        (
            "DObj transform",
            mutate_row(
                packet,
                "dobjs",
                0,
                transform_flags=packet.dobjs[0].transform_flags ^ 1,
            ),
        ),
        (
            "display-list root",
            mutate_row(
                packet,
                "bindings",
                0,
                root_offset=packet.bindings[0].root_offset + 8,
            ),
        ),
        (
            "material flags",
            mutate_row(
                packet,
                "materials",
                0,
                flags=packet.materials[0].flags ^ 1,
            ),
        ),
        (
            "run corner span",
            mutate_row(packet, "runs", 0, first_corner=len(packet.corners)),
        ),
        (
            "run cross-matrix flag",
            mutate_row(
                packet,
                "runs",
                EXPECTED_PROJECTED_CROSS_MATRIX_RUNS[0],
                flags=(
                    packet.runs[EXPECTED_PROJECTED_CROSS_MATRIX_RUNS[0]].flags
                    ^ generator.RUN_FLAG_PROJECTED_CROSS_MATRIX
                ),
            ),
        ),
        ("corner index", replace(packet, corners=tuple(bad_corners))),
        ("state index", replace(packet, state_sequence=tuple(bad_sequence))),
        (
            "state command",
            mutate_row(
                packet,
                "state_deltas",
                0,
                w0=packet.state_deltas[0].w0 ^ 1,
            ),
        ),
        (
            "state span",
            mutate_row(
                packet,
                "state_spans",
                0,
                state_count=packet.state_spans[0].state_count + 1,
            ),
        ),
    )

    for name, candidate in mutations:
        probe = CommitProbe()
        try:
            probe.preflight(candidate, packet)
        except generator.Falsifier:
            pass
        else:
            raise generator.falsify(f"{name} perturbation armed the owner")
        require(
            (probe.armed, probe.segments_emitted, probe.gx_mutations) == (0, 0, 0),
            f"{name} perturbation mutated GX before rejection",
        )

    valid = CommitProbe()
    valid.preflight(packet, packet)
    valid.commit(packet)
    require(
        (valid.armed, valid.segments_emitted, valid.gx_mutations) == (1, 8, 8),
        "valid fixture did not commit all eight callback segments",
    )
    return len(mutations)


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    output = repo_root / generator.DEFAULT_OUTPUT
    try:
        first = generator.generate(repo_root)
        second = generator.generate(repo_root)
        require(first == second, "two in-process generations differ")
        verify_packet(first)
        replay_commands = verify_command_replay(first, repo_root)
        perturbations = verify_fail_closed(first)
        manifest_fields = verify_consumed_fields_manifest(repo_root)

        rendered_first = generator.render_include(first)
        rendered_second = generator.render_include(second)
        require(rendered_first == rendered_second, "rendered packet is nondeterministic")
        include_hash = generator.sha256(rendered_first)
        require(
            include_hash == generator.EXPECTED_INCLUDE_SHA256,
            f"include SHA256 {include_hash} != pinned {generator.EXPECTED_INCLUDE_SHA256}",
        )
        require(output.is_file(), f"generated include is absent: {output}")
        require(output.read_bytes() == rendered_first, "generated include is stale")
        verify_input_falsifier(repo_root)
    except (generator.Falsifier, OSError, ValueError) as exc:
        print(f"M3_NATIVE_STAGE_CHECK_FAIL: {exc}", file=sys.stderr)
        return 1

    print(
        "M3_NATIVE_STAGE_CHECK_OK "
        f"callbacks={len(first.segments)} dobjs={len(first.dobjs)} "
        f"bindings={len(first.bindings)} runs={len(first.runs)} "
        f"epochs={len(first.epochs)} triangles={len(first.corners) // 3} "
        f"cross_matrix_runs={len(EXPECTED_PROJECTED_CROSS_MATRIX_RUNS)} "
        f"cross_matrix_triangles={EXPECTED_PROJECTED_CROSS_MATRIX_TRIANGLES} "
        f"cross_matrix_foreign_corners={EXPECTED_PROJECTED_CROSS_MATRIX_FOREIGN_CORNERS} "
        f"state_deltas={len(first.state_deltas)} "
        f"state_events={len(first.state_sequence)} "
        f"replay_commands={replay_commands} "
        f"fail_closed_perturbations={perturbations} "
        f"manifest_fields={manifest_fields} "
        f"slab_bytes={first.slab_bytes()} sha256={include_hash}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
