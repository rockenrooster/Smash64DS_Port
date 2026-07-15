#!/usr/bin/env python3
"""Fail-closed host check for the generated whole-fighter GX FIFO packets."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import struct

import generate_nds_native_owners as native


EXPECTED = {
    "mario": {
        "words": 4034,
        "hash": 0x033874A6,
        "roots": 14,
        "epochs": 18,
        "runs": 30,
        "raw_runs": 21,
        "cross_runs": 9,
        "triangles": 320,
        "raw_triangles": 284,
        "cross_triangles": 36,
        "corners": 960,
        "textured_corners": 192,
        "stores": 8,
        "restores": 70,
    },
    "fox": {
        "words": 3936,
        "hash": 0x791EB7A6,
        "roots": 18,
        "epochs": 31,
        "runs": 37,
        "raw_runs": 33,
        "cross_runs": 4,
        "triangles": 306,
        "raw_triangles": 298,
        "cross_triangles": 8,
        "corners": 918,
        "textured_corners": 189,
        "stores": 2,
        "restores": 14,
    },
}


def require(condition: bool, message: str):
    if not condition:
        raise ValueError(message)


def expect_value_error(callback, message: str):
    try:
        callback()
    except ValueError:
        return
    raise ValueError(message)


def parse_c_uint(value: str) -> int:
    require(value.endswith("u"), f"generated integer lacks u suffix: {value}")
    return int(value[:-1], 0)


def parse_generated_array(
        generated: str, type_name: str, symbol: str, row_width: int | None):
    pattern = re.compile(
        rf"static const {re.escape(type_name)} {re.escape(symbol)}"
        rf"\[(\d+)\]\s*=\s*\n\{{\n(.*?)\n\}};",
        re.DOTALL,
    )
    matches = pattern.findall(generated)
    require(len(matches) == 1, f"generated array {symbol} is missing/ambiguous")
    declared_count = int(matches[0][0])
    body = matches[0][1]
    if row_width is None:
        values = [
            parse_c_uint(value)
            for value in re.findall(r"(?:0x[0-9a-fA-F]+|\d+)u", body)
        ]
        require(len(values) == declared_count,
                f"generated array {symbol} cardinality changed")
        return values

    rows = []
    for row_body in re.findall(r"\{\s*([^{}]+?)\s*\},", body):
        values = [
            parse_c_uint(value)
            for value in re.findall(r"(?:0x[0-9a-fA-F]+|\d+)u", row_body)
        ]
        require(len(values) == row_width,
                f"generated array {symbol} row width changed")
        rows.append(tuple(values))
    require(len(rows) == declared_count,
            f"generated array {symbol} cardinality changed")
    return rows


def parse_generated_plan(generated: str, owner_name: str, owner_slot: int):
    owner_title = owner_name.title()
    prefix = f"sNdsNative{owner_title}Fifo"
    symbols = {
        "words": f"{prefix}Words",
        "matrix_patches": f"{prefix}MatrixPatches",
        "color_patches": f"{prefix}ColorPatches",
        "texcoord_patches": f"{prefix}TexcoordPatches",
        "epoch_patches": f"{prefix}EpochPatches",
    }
    words = parse_generated_array(generated, "u32", symbols["words"], None)
    matrix_patches = parse_generated_array(
        generated, "NDSNativeFifoMatrixPatch",
        symbols["matrix_patches"], 3,
    )
    color_patches = parse_generated_array(
        generated, "NDSNativeFifoWordPatch",
        symbols["color_patches"], 2,
    )
    texcoord_patches = parse_generated_array(
        generated, "NDSNativeFifoWordPatch",
        symbols["texcoord_patches"], 2,
    )
    epoch_patches = parse_generated_array(
        generated, "NDSNativeFifoEpochPatch",
        symbols["epoch_patches"], 13,
    )

    plan_symbol = f"{prefix}Plan"
    plan_pattern = re.compile(
        rf"static const NDSNativeFifoOwnerPlan {re.escape(plan_symbol)}"
        rf"\s*=\s*\n\{{\n(.*?)\n\}};",
        re.DOTALL,
    )
    plan_matches = plan_pattern.findall(generated)
    require(len(plan_matches) == 1,
            f"generated plan {plan_symbol} is missing/ambiguous")
    entries = [
        line.strip().removesuffix(",")
        for line in plan_matches[0].splitlines()
        if line.strip()
    ]
    require(entries[:5] == [
        symbols["words"],
        symbols["matrix_patches"],
        symbols["color_patches"],
        symbols["texcoord_patches"],
        symbols["epoch_patches"],
    ], f"generated plan {plan_symbol} table association changed")
    require(len(entries) == 23 and entries[-1] == "{ 0u, 0u, 0u }",
            f"generated plan {plan_symbol} scalar layout changed")
    scalars = [parse_c_uint(value) for value in entries[5:-1]]
    require(len(scalars) == 17,
            f"generated plan {plan_symbol} scalar count changed")
    (
        template_hash, word_count, matrix_patch_count, color_patch_count,
        texcoord_patch_count, epoch_patch_count, triangle_count,
        raw_triangle_count, cross_triangle_count, run_count, raw_run_count,
        cross_run_count, root_count, epoch_count, store_count, restore_count,
        parsed_owner_slot,
    ) = scalars
    require(
        (word_count, matrix_patch_count, color_patch_count,
         texcoord_patch_count, epoch_patch_count) ==
        (len(words), len(matrix_patches), len(color_patches),
         len(texcoord_patches), len(epoch_patches)),
        f"generated plan {plan_symbol} table counts changed",
    )
    require(parsed_owner_slot == owner_slot,
            f"generated plan {plan_symbol} owner slot changed")
    return {
        "words": words,
        "matrix_patches": matrix_patches,
        "color_patches": color_patches,
        "texcoord_patches": texcoord_patches,
        "epoch_patches": epoch_patches,
        "template_hash": template_hash,
        "triangle_count": triangle_count,
        "raw_triangle_count": raw_triangle_count,
        "cross_triangle_count": cross_triangle_count,
        "run_count": run_count,
        "raw_run_count": raw_run_count,
        "cross_run_count": cross_run_count,
        "root_count": root_count,
        "epoch_count": epoch_count,
        "store_count": store_count,
        "restore_count": restore_count,
    }


def parse_generated_plans(generated: str):
    plans = {
        owner_name: parse_generated_plan(generated, owner_name, owner_slot)
        for owner_slot, owner_name in enumerate(("mario", "fox"))
    }
    max_words_matches = re.findall(
        r"#define NDS_NATIVE_FIFO_MAX_WORDS (\d+)u", generated
    )
    require(len(max_words_matches) == 1,
            "generated maximum FIFO word count is missing/ambiguous")
    max_words = int(max_words_matches[0])
    require(max_words == max(len(plan["words"]) for plan in plans.values()),
            "generated maximum FIFO word count changed")
    return plans, max_words


def build_plans(source_root: Path):
    data = native.decode_export()
    vertex = native.unpack_many("<BBBBIhh", data["vertex"])
    triangles = [
        item[0] for item in native.unpack_many("<H", data["triangles"])
    ]
    runs = native.unpack_many("<HBBI", data["runs"])
    epochs = native.unpack_many("<HHHHBBBBBBBB", data["epochs"])
    mario_roots = native.unpack_many(
        "<IHHHBBBB2x", data["mario_roots"]
    )
    fox_roots = native.unpack_many("<IHHHBBBB2x", data["fox_roots"])
    owner_roots = (("mario", mario_roots), ("fox", fox_roots))
    topologies = [
        native.decode_joint_topology(
            native.load_o2r_payload(source_root, owner_name),
            owner_name,
            roots,
        )
        for owner_name, roots in owner_roots
    ]
    (
        dense_vertices,
        dense_color_sources,
        dense_owners,
        dense_corners,
        action_dense_first,
        run_first_corner,
        run_owners,
        run_root_bindings,
        run_binding_sets,
    ) = native.build_dense_geometry(
        vertex, triangles, runs, epochs, owner_roots, source_root
    )
    del dense_owners
    direct_policies = native.build_direct_epoch_policies(len(epochs))
    (
        action_dense_spans,
        packed_corners,
        run_first_unique,
        run_unique_count,
        run_unique_dense,
    ) = native.build_direct_dense_tables(
        vertex,
        runs,
        dense_vertices,
        dense_color_sources,
        dense_corners,
        action_dense_first,
        run_first_corner,
        run_owners,
        run_root_bindings,
        run_binding_sets,
        [topology[3] for topology in topologies],
    )
    plans = {}
    first_root = 0
    for owner_slot, ((owner_name, roots), topology) in enumerate(
            zip(owner_roots, topologies)):
        plans[owner_name] = native.build_packed_fifo_owner_plan(
            owner_name,
            owner_slot,
            roots,
            first_root,
            epochs,
            runs,
            dense_vertices,
            packed_corners,
            run_first_corner,
            direct_policies,
            topology[3],
        )
        first_root += len(roots)
    return plans, {
        "owner_roots": dict(owner_roots),
        "dense_vertices": dense_vertices,
        "epochs": epochs,
        "runs": runs,
        "direct_policies": direct_policies,
        "packed_corners": packed_corners,
        "action_dense_spans": action_dense_spans,
        "run_first_corner": run_first_corner,
        "run_first_unique": run_first_unique,
        "run_unique_count": run_unique_count,
        "run_unique_dense": run_unique_dense,
    }


def decode_payload(words: list[int]):
    parameter_types: dict[int, int] = {}
    command_counts: dict[int, int] = {}
    commands = []
    cursor = 0
    while cursor < len(words):
        command_word_offset = cursor
        command_word = words[cursor]
        cursor += 1
        packed = [
            (command_word >> (byte_index * 8)) & 0xff
            for byte_index in range(4)
        ]
        for byte_index, command in enumerate(packed):
            require(
                command in native.FIFO_PARAMETER_COUNTS,
                f"word {command_word_offset}: unknown FIFO command "
                f"0x{command:02x}",
            )
            require(command != 0x41, "GFX_END must never be packed")
            parameter_count = native.FIFO_PARAMETER_COUNTS[command]
            require(
                cursor + parameter_count <= len(words),
                f"word {command_word_offset}: truncated params for "
                f"0x{command:02x}",
            )
            command_counts[command] = command_counts.get(command, 0) + 1
            commands.append((
                command_word_offset,
                command,
                cursor,
                byte_index * 8,
            ))
            for parameter_offset in range(parameter_count):
                parameter_types[cursor + parameter_offset] = command
            cursor += parameter_count
    require(cursor == len(words), "packed FIFO decoder did not consume payload")
    return parameter_types, command_counts, commands


def build_epoch_packet_contract(epoch_index: int, context: dict):
    epochs = context["epochs"]
    runs = context["runs"]
    dense_vertices = context["dense_vertices"]
    packed_corners = context["packed_corners"]
    action_dense_spans = context["action_dense_spans"]
    run_first_corner = context["run_first_corner"]
    run_first_unique = context["run_first_unique"]
    run_unique_count = context["run_unique_count"]
    run_unique_dense = context["run_unique_dense"]

    require(epoch_index < len(epochs), f"epoch {epoch_index}: out of range")
    epoch = epochs[epoch_index]
    first_action = epoch[2]
    first_run = epoch[3]
    action_count = epoch[8]
    run_count = epoch[9]
    require(run_count > 0, f"epoch {epoch_index}: packet has no runs")
    require(first_run + run_count <= len(runs),
            f"epoch {epoch_index}: run span is out of range")

    corner_sources = []
    for run_index in range(first_run, first_run + run_count):
        corner_first = run_first_corner[run_index]
        corner_count = runs[run_index][1] * 3
        require(corner_first + corner_count <= len(packed_corners),
                f"epoch {epoch_index} run {run_index}: corners out of range")
        corner_sources.extend(
            packed & (native.PACKED_DENSE_ID_LIMIT - 1)
            for packed in packed_corners[
                corner_first:corner_first + corner_count
            ]
        )
    require(all(source < len(dense_vertices) for source in corner_sources),
            f"epoch {epoch_index}: corner source is out of range")

    require(first_run < len(run_first_unique),
            f"epoch {epoch_index}: first-run unique index is out of range")
    unique_first = run_first_unique[first_run]
    unique_count = run_unique_count[first_run]
    require(unique_first + unique_count <= len(run_unique_dense),
            f"epoch {epoch_index}: first-run unique span is out of range")
    prepared_sources = set(
        run_unique_dense[unique_first:unique_first + unique_count]
    )
    require(first_action + action_count <= len(action_dense_spans),
            f"epoch {epoch_index}: action span is out of range")
    for action_index in range(first_action, first_action + action_count):
        span = action_dense_spans[action_index]
        dense_first = span & (native.PACKED_DENSE_ID_LIMIT - 1)
        dense_count = span >> native.PACKED_DENSE_ID_BITS
        require(dense_count > 0 and
                dense_first + dense_count <= len(dense_vertices),
                f"epoch {epoch_index} action {action_index}: "
                "dense span is out of range")
        prepared_sources.update(range(dense_first, dense_first + dense_count))

    policy = context["direct_policies"][epoch_index]
    textured = native.DIRECT_POLICY_FAMILIES[policy & 0x03][3] != 0
    return corner_sources, prepared_sources, textured


def check_plan(owner_name: str, plan: dict, context: dict):
    expected = EXPECTED[owner_name]
    parameter_types, command_counts, commands = decode_payload(plan["words"])
    require(len(plan["words"]) == expected["words"],
            f"{owner_name}: word count changed")
    require(plan["template_hash"] == expected["hash"],
            f"{owner_name}: template hash changed")
    template_bytes = struct.pack(
        f"<{len(plan['words'])}I", *plan["words"]
    )
    decoded_hash = int.from_bytes(
        hashlib.sha256(template_bytes).digest()[:4], "little"
    )
    require(plan["template_hash"] == decoded_hash,
            f"{owner_name}: plan hash does not match parsed words")
    for actual_key, expected_key in (
            ("root_count", "roots"),
            ("epoch_count", "epochs"),
            ("run_count", "runs"),
            ("raw_run_count", "raw_runs"),
            ("cross_run_count", "cross_runs"),
            ("triangle_count", "triangles"),
            ("raw_triangle_count", "raw_triangles"),
            ("cross_triangle_count", "cross_triangles"),
            ("store_count", "stores"),
            ("restore_count", "restores")):
        require(
            plan[actual_key] == expected[expected_key],
            f"{owner_name}: {actual_key} changed",
        )

    matrix_loads = [
        parameter_word
        for _, command, parameter_word, _ in commands
        if command == native.FIFO_MTX_LOAD_4X4
    ]
    require(
        len(plan["matrix_patches"]) == expected["roots"],
        f"{owner_name}: matrix patch count changed",
    )
    require([word for word, _, _ in plan["matrix_patches"]] ==
            matrix_loads[1:],
            f"{owner_name}: identity projection became patchable")
    for patch_index, (word, source, count) in enumerate(
            plan["matrix_patches"]):
        require(parameter_types.get(word) == native.FIFO_MTX_LOAD_4X4,
                f"{owner_name}: matrix patch {patch_index} has wrong command")
        require(count == 16,
                f"{owner_name}: matrix patch {patch_index} has wrong size")
        require(
            all(parameter_types.get(word + offset) ==
                native.FIFO_MTX_LOAD_4X4 for offset in range(16)),
            f"{owner_name}: matrix patch {patch_index} crosses a command",
        )
        require(
            source == patch_index,
            f"{owner_name}: matrix patch source order changed",
        )

    for name, patches, command, expected_count in (
            ("color", plan["color_patches"], native.FIFO_COLOR,
             expected["corners"]),
            ("texcoord", plan["texcoord_patches"], native.FIFO_TEX_COORD,
             expected["textured_corners"])):
        require(len(patches) == expected_count,
                f"{owner_name}: {name} patch count changed")
        require(
            all(parameter_types.get(word) == command and
                source < len(context["dense_vertices"])
                for word, source in patches),
            f"{owner_name}: invalid {name} patch target/source",
        )
    expected_color_words = [
        parameter_word
        for _, command, parameter_word, _ in commands
        if command == native.FIFO_COLOR
    ]
    expected_texcoord_words = [
        parameter_word
        for _, command, parameter_word, _ in commands
        if command == native.FIFO_TEX_COORD
    ]
    require(
        [word for word, _ in plan["color_patches"]] == expected_color_words,
        f"{owner_name}: ordered COLOR parameter offsets changed",
    )
    require(
        [word for word, _ in plan["texcoord_patches"]] ==
        expected_texcoord_words,
        f"{owner_name}: ordered TEX_COORD parameter offsets changed",
    )

    require(len(plan["epoch_patches"]) == expected["epochs"],
            f"{owner_name}: epoch patch count changed")
    expected_epoch_order = []
    for root_index, root in enumerate(context["owner_roots"][owner_name]):
        expected_epoch_order.extend(
            (root_index, epoch_index)
            for epoch_index in range(root[1], root[1] + root[4])
        )
    require(
        [(patch[9], patch[10]) for patch in plan["epoch_patches"]] ==
        expected_epoch_order,
        f"{owner_name}: epoch root/source order changed",
    )
    texture_commands = [
        (command_word, parameter_word, shift)
        for command_word, command, parameter_word, shift in commands
        if command == native.FIFO_TEX_FORMAT
    ]
    palette_commands = [
        (command_word, parameter_word, shift)
        for command_word, command, parameter_word, shift in commands
        if command == native.FIFO_PAL_FORMAT
    ]
    polygon_commands = [
        (command_word, parameter_word, shift)
        for command_word, command, parameter_word, shift in commands
        if command == native.FIFO_POLY_FORMAT
    ]
    begin_commands = [
        (command_word, parameter_word, shift)
        for command_word, command, parameter_word, shift in commands
        if command == native.FIFO_BEGIN
    ]
    require(
        all(len(command_list) == expected["epochs"] for command_list in (
            texture_commands, palette_commands, polygon_commands,
            begin_commands,
        )),
        f"{owner_name}: epoch state command count changed",
    )
    expected_epoch_offsets = [
        (
            texture_commands[index][1],
            palette_commands[index][1],
            polygon_commands[index][1],
            begin_commands[index][0],
            begin_commands[index][1],
            begin_commands[index][2],
        )
        for index in range(expected["epochs"])
    ]
    require(
        [
            (patch[0], patch[1], patch[2], patch[3], patch[4], patch[11])
            for patch in plan["epoch_patches"]
        ] == expected_epoch_offsets,
        f"{owner_name}: ordered epoch state patch offsets changed",
    )
    next_color = 0
    next_texcoord = 0
    owner_corner_sources = []
    for (
            tex, pal, poly, begin_command_word, begin_param_word,
            color_first, color_count, texcoord_first, texcoord_count,
            root, epoch, begin_shift, flags,
    ) in plan["epoch_patches"]:
        require(parameter_types.get(tex) == native.FIFO_TEX_FORMAT,
                f"{owner_name} epoch {epoch}: bad TEX_FORMAT offset")
        require(parameter_types.get(pal) == native.FIFO_PAL_FORMAT,
                f"{owner_name} epoch {epoch}: bad PAL_FORMAT offset")
        require(parameter_types.get(poly) == native.FIFO_POLY_FORMAT,
                f"{owner_name} epoch {epoch}: bad POLY_FORMAT offset")
        require(parameter_types.get(begin_param_word) == native.FIFO_BEGIN,
                f"{owner_name} epoch {epoch}: bad BEGIN param offset")
        require(begin_shift in (0, 8, 16, 24),
                f"{owner_name} epoch {epoch}: bad BEGIN byte shift")
        require(((plan["words"][begin_command_word] >> begin_shift) & 0xff) ==
                native.FIFO_BEGIN,
                f"{owner_name} epoch {epoch}: command patch is not BEGIN")
        require(flags == 0,
                f"{owner_name} epoch {epoch}: unknown epoch flags")
        require(root < expected["roots"],
                f"{owner_name} epoch {epoch}: root out of range")
        require((color_first, texcoord_first) ==
                (next_color, next_texcoord),
                f"{owner_name} epoch {epoch}: patch spans are not monotonic")
        require(color_first + color_count <= len(plan["color_patches"]),
                f"{owner_name} epoch {epoch}: color span is out of range")
        require(texcoord_first + texcoord_count <=
                len(plan["texcoord_patches"]),
                f"{owner_name} epoch {epoch}: texcoord span is out of range")
        corner_sources, prepared_sources, textured = \
            build_epoch_packet_contract(epoch, context)
        color_sources = [
            source for _, source in plan["color_patches"][
                color_first:color_first + color_count
            ]
        ]
        texcoord_sources = [
            source for _, source in plan["texcoord_patches"][
                texcoord_first:texcoord_first + texcoord_count
            ]
        ]
        require(
            color_sources == corner_sources,
            f"{owner_name} epoch {epoch}: "
            "color sources do not cover emitted corners",
        )
        if textured:
            require(
                texcoord_count == color_count and
                texcoord_sources == corner_sources,
                f"{owner_name} epoch {epoch}: "
                "textured texcoords do not cover emitted corners",
            )
            missing_sources = sorted(set(corner_sources) - prepared_sources)
            require(
                not missing_sources,
                f"{owner_name} epoch {epoch}: "
                f"texcoord sources are not prepared: {missing_sources}",
            )
        else:
            require(
                texcoord_count == 0,
                f"{owner_name} epoch {epoch}: "
                "untextured epoch has texcoord patches",
            )
        owner_corner_sources.extend(corner_sources)
        next_color += color_count
        next_texcoord += texcoord_count
    require(next_color == len(plan["color_patches"]),
            f"{owner_name}: epoch color spans do not cover all patches")
    require(next_texcoord == len(plan["texcoord_patches"]),
            f"{owner_name}: epoch texcoord spans do not cover all patches")

    vertex_words = [
        parameter_word
        for _, command, parameter_word, _ in commands
        if command == native.FIFO_VERTEX16
    ]
    require(
        len(vertex_words) == len(owner_corner_sources) == expected["corners"],
        f"{owner_name}: VERTEX16 source count changed",
    )
    for corner_index, (parameter_word, dense_id) in enumerate(
            zip(vertex_words, owner_corner_sources)):
        x, y, z = context["dense_vertices"][dense_id][:3]
        scaled_x = x * 16
        scaled_y = y * 16
        scaled_z = z * 16
        require(
            all(-0x8000 <= value <= 0x7fff
                for value in (scaled_x, scaled_y, scaled_z)),
            f"{owner_name}: corner {corner_index} dense {dense_id}: "
            f"VERTEX16 signed overflow xyz={x}/{y}/{z} "
            f"scaled={scaled_x}/{scaled_y}/{scaled_z}",
        )
        expected_xy = (
            (scaled_x & 0xffff) | ((scaled_y & 0xffff) << 16)
        )
        expected_z = scaled_z & 0xffff
        require(
            plan["words"][parameter_word] == expected_xy and
            plan["words"][parameter_word + 1] == expected_z,
            f"{owner_name}: corner {corner_index} dense {dense_id}: "
            "VERTEX16 payload/source mismatch",
        )

    require(command_counts.get(native.FIFO_MTX_MODE) == 2,
            f"{owner_name}: matrix mode command count changed")
    require(command_counts.get(native.FIFO_MTX_LOAD_4X4) ==
            expected["roots"] + 1,
            f"{owner_name}: raw-composed 4x4 load count changed")
    require(command_counts.get(native.FIFO_MTX_LOAD_4X3, 0) == 0,
            f"{owner_name}: split 4x3 matrix load returned")
    identity = [
        1 << 12, 0, 0, 0,
        0, 1 << 12, 0, 0,
        0, 0, 1 << 12, 0,
        0, 0, 0, 1 << 12,
    ]
    require(plan["words"][matrix_loads[0]:matrix_loads[0] + 16] == identity,
            f"{owner_name}: packet projection is not immutable identity")
    require(command_counts.get(native.FIFO_MTX_STORE, 0) ==
            expected["stores"], f"{owner_name}: store count changed")
    require(command_counts.get(native.FIFO_MTX_RESTORE, 0) ==
            expected["restores"], f"{owner_name}: restore count changed")
    require(command_counts.get(native.FIFO_BEGIN, 0) == expected["epochs"],
            f"{owner_name}: BEGIN count changed")
    require(all(command != 0x41 for _, command, _, _ in commands),
            f"{owner_name}: GFX_END leaked into packet")
    print(
        f"{owner_name}: words={len(plan['words'])} "
        f"bytes={len(plan['words']) * 4} "
        f"roots/epochs/runs={expected['roots']}/{expected['epochs']}/"
        f"{expected['runs']} triangles={expected['triangles']} "
        f"corners/tex={expected['corners']}/{expected['textured_corners']} "
        f"store/restore={expected['stores']}/{expected['restores']} "
        f"hash=0x{plan['template_hash']:08x}"
    )


def check_reused_dense_epoch_fixture():
    """One dense ID may not make epoch patching collapse to one final value."""
    builder = native.PackedFifoBuilder()
    builder.command(
        native.FIFO_COLOR, [0],
        [(native.FIFO_PATCH_COLOR, (0, 7))],
    )
    builder.command(native.FIFO_VERTEX16, [0, 0])
    builder.command(
        native.FIFO_COLOR, [0],
        [(native.FIFO_PATCH_COLOR, (1, 7))],
    )
    builder.command(native.FIFO_VERTEX16, [0, 0])
    words, patches, _ = builder.finish()
    color_patches = patches[native.FIFO_PATCH_COLOR]
    require(len(color_patches) == 2 and
            color_patches[0][1] == (0, 7) and
            color_patches[1][1] == (1, 7) and
            color_patches[0][0] != color_patches[1][0],
            "synthetic reused-dense fixture collapsed distinct epoch words")
    staged = list(words)
    staged[color_patches[0][0]] = 0x1111
    staged[color_patches[1][0]] = 0x2222
    require(staged[color_patches[0][0]] == 0x1111 and
            staged[color_patches[1][0]] == 0x2222,
            "synthetic reused-dense fixture lost epoch-local output")


def check_vertex16_signed_boundaries():
    xy, z_word = native.pack_fifo_vertex16(
        -2048, 2047, -2048, "VERTEX16 boundary fixture"
    )
    require(xy == 0x7ff08000 and z_word == 0x8000,
            "VERTEX16 signed boundary encoding changed")
    expect_value_error(
        lambda: native.pack_fifo_vertex16(
            -2049, 0, 0, "VERTEX16 lower-bound falsifier"
        ),
        "VERTEX16 accepted a coordinate below its signed range",
    )
    expect_value_error(
        lambda: native.pack_fifo_vertex16(
            2048, 0, 0, "VERTEX16 upper-bound falsifier"
        ),
        "VERTEX16 accepted a coordinate above its signed range",
    )


def check_offset_swap_falsifiers(owner_name: str, plan: dict, context: dict):
    color_swap = dict(plan)
    color_swap["color_patches"] = list(plan["color_patches"])
    color_word0, color_source0 = color_swap["color_patches"][0]
    color_word1, color_source1 = color_swap["color_patches"][1]
    color_swap["color_patches"][0] = (color_word1, color_source0)
    color_swap["color_patches"][1] = (color_word0, color_source1)
    expect_value_error(
        lambda: check_plan(owner_name, color_swap, context),
        f"{owner_name}: checker accepted swapped COLOR parameter offsets",
    )

    texcoord_swap = dict(plan)
    texcoord_swap["texcoord_patches"] = list(plan["texcoord_patches"])
    tex_word0, tex_source0 = texcoord_swap["texcoord_patches"][0]
    tex_word1, tex_source1 = texcoord_swap["texcoord_patches"][1]
    texcoord_swap["texcoord_patches"][0] = (tex_word1, tex_source0)
    texcoord_swap["texcoord_patches"][1] = (tex_word0, tex_source1)
    expect_value_error(
        lambda: check_plan(owner_name, texcoord_swap, context),
        f"{owner_name}: checker accepted swapped TEX_COORD parameter offsets",
    )

    epoch_swap = dict(plan)
    epoch_swap["epoch_patches"] = list(plan["epoch_patches"])
    first_epoch = list(epoch_swap["epoch_patches"][0])
    second_epoch = list(epoch_swap["epoch_patches"][1])
    first_epoch[0], second_epoch[0] = second_epoch[0], first_epoch[0]
    epoch_swap["epoch_patches"][0] = tuple(first_epoch)
    epoch_swap["epoch_patches"][1] = tuple(second_epoch)
    expect_value_error(
        lambda: check_plan(owner_name, epoch_swap, context),
        f"{owner_name}: checker accepted swapped epoch TEX_FORMAT offsets",
    )


def check_plan_association_swap_falsifier(generated: str):
    original = (
        "    sNdsNativeMarioFifoColorPatches,\n"
        "    sNdsNativeMarioFifoTexcoordPatches,"
    )
    swapped = (
        "    sNdsNativeMarioFifoTexcoordPatches,\n"
        "    sNdsNativeMarioFifoColorPatches,"
    )
    require(generated.count(original) == 1,
            "generated Mario plan association fixture changed")
    expect_value_error(
        lambda: parse_generated_plans(generated.replace(original, swapped, 1)),
        "generated artifact parser accepted swapped plan associations",
    )


def check_nonaligned_split_matrix_falsifier():
    """Permanently reject split rounding against mode-8 raw composition."""
    frac = 1 << 12

    def round_shift(value: int, shift: int) -> int:
        bias = 1 << (shift - 1)
        return (-(((-value) + bias) >> shift)
                if value < 0 else (value + bias) >> shift)

    def multiply(lhs: list[list[int]], rhs: list[list[int]]):
        return [
            [
                round_shift(
                    sum(lhs[row][k] * rhs[k][col] for k in range(4)),
                    12,
                )
                for col in range(4)
            ]
            for row in range(4)
        ]

    # This default-battle-camera projection is paired with one nonaligned
    # pre-compose translation compatible with a captured mode-8 raw row of
    # (17, 79989, 169494).  Scaling the two input rows independently changes Z
    # by one hardware LSB; only compose-then-scale preserves mode-8 parity.
    modelview = [
        [frac, 0, 0, 0],
        [0, frac, 0, 0],
        [0, 0, frac, 0],
        [2043, 7075036, -44919906, frac],
    ]
    projection = [
        [8727, 0, 0, 0],
        [0, 11855, 0, 0],
        [0, 0, -4149, -4096],
        [0, 0, -2110682, 0],
    ]
    mode8 = multiply(modelview, projection)
    mode8[3] = [round_shift(value, 8) for value in mode8[3]]
    require(mode8[3] == [17, 79989, 169494, 175468],
            "nonaligned mode-8 raw-composed oracle changed")

    split_modelview = [row[:] for row in modelview]
    split_modelview[3][:3] = [
        round_shift(value, 8) for value in split_modelview[3][:3]
    ]
    split_projection = [row[:] for row in projection]
    split_projection[3] = [
        round_shift(value, 8) for value in split_projection[3]
    ]
    split = multiply(split_modelview, split_projection)
    require(split[3] == [17, 79989, 169493, 175468],
            "nonaligned split-rounding falsifier changed")
    require(split != mode8,
            "unsafe split matrices unexpectedly match raw composition")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
    )
    parser.add_argument(
        "--generated",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "src" / "nds" /
        "nds_native_fighter_owner.generated.inc",
    )
    args = parser.parse_args()
    source_root = args.source_root.resolve()
    require(args.generated.is_file(), f"missing generated include: {args.generated}")
    checked_in_generated = args.generated.read_text()
    plans, max_words = parse_generated_plans(checked_in_generated)
    generated = native.generate(source_root)
    require(checked_in_generated == generated,
            f"stale generated include: {args.generated}")
    _source_plans, context = build_plans(source_root)
    check_reused_dense_epoch_fixture()
    check_vertex16_signed_boundaries()
    check_nonaligned_split_matrix_falsifier()
    check_plan_association_swap_falsifier(checked_in_generated)
    for owner_name in ("mario", "fox"):
        check_offset_swap_falsifiers(owner_name, plans[owner_name], context)
        check_plan(owner_name, plans[owner_name], context)
    require(sum(plan["triangle_count"] for plan in plans.values()) == 626,
            "combined triangle count changed")
    require(sum(len(plan["color_patches"]) for plan in plans.values()) == 1878,
            "combined color patch count changed")
    require(sum(len(plan["texcoord_patches"]) for plan in plans.values()) == 381,
            "combined texcoord patch count changed")
    require(max_words == 4034,
            "maximum staging payload changed")
    print("combined: triangles=626 corners=1878 textured=381 maxStage=16136B")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
