#!/usr/bin/env python3
"""Falsify drift in the host-exact Dream Land whole-stage packet.

This checker deliberately does not build or launch a ROM.  It regenerates the
packet twice from immutable BattleShip/O2R inputs, checks the committed include
byte-for-byte, and verifies the exact callback, topology, display-list, run,
texture-epoch, material, and cross-binding vertex-cache contracts needed by a
later DS-native consumer.
"""

from __future__ import annotations

import shutil
import sys
import tempfile
from collections import Counter
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
    require(packet.slab_bytes() == 10076, "whole-stage slab byte count drifted")
    require(packet.slab_bytes() <= 16 * 1024, "whole-stage slab exceeds 16 KiB")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    output = repo_root / generator.DEFAULT_OUTPUT
    try:
        first = generator.generate(repo_root)
        second = generator.generate(repo_root)
        require(first == second, "two in-process generations differ")
        verify_packet(first)

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
        f"slab_bytes={first.slab_bytes()} sha256={include_hash}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
