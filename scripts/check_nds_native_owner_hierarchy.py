#!/usr/bin/env python3
"""Fail-closed host check for the Mario/Fox no-copy GX hierarchy plan."""

from __future__ import annotations

import argparse
from pathlib import Path

import check_nds_native_owner_packet as packet
import generate_nds_native_owners as native


EXPECTED_SCHEDULES = {
    "mario": (
        0xffff, 0x7fe0, 0x7fe1, 0xfc02, 0xffe3,
        0x4024, 0x4445, 0x7c66, 0xffe3, 0x7c88,
        0x7fe3, 0x48aa, 0x4ccb, 0x7cec, 0x7fed,
        0xffe2, 0x510f, 0x5530, 0x7ff1, 0x7d52,
        0x7fe2, 0x5974, 0x5d95, 0x7ff6, 0x7db7,
    ),
    "fox": (
        0xffff, 0x7fe0, 0x7c01, 0xfc22, 0xffe3,
        0x7c44, 0x7c65, 0x7c86, 0xfca3, 0x7cc8,
        0x7fe3, 0x7cea, 0x7d0b, 0x7d2c, 0x7fed,
        0xffe2, 0x7d4f, 0x7d70, 0x7ff1, 0x7d92,
        0xffe2, 0x7db4, 0x7dd5, 0x7ff6, 0x7df7,
        0x4202, 0x4639,
    ),
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


def build_direct_trace(owner_name: str, context: dict, cross_slots: list[int]):
    """Build the semantic state/corner stream for a no-copy direct owner."""
    states = []
    corners = []
    submit_classes = []
    slot_bindings = {
        slot: binding for binding, slot in enumerate(cross_slots)
        if slot != native.PACKED_GX_SLOT_CURRENT
    }
    for root_index, root in enumerate(context["owner_roots"][owner_name]):
        for epoch_index in range(root[1], root[1] + root[4]):
            epoch = context["epochs"][epoch_index]
            policy = context["direct_policies"][epoch_index]
            textured = native.DIRECT_POLICY_FAMILIES[policy & 3][3] != 0
            states.append((root_index, epoch_index, policy, *epoch))
            for run_index in range(epoch[3], epoch[3] + epoch[9]):
                _, triangle_count, submit_class, _ = context["runs"][run_index]
                corner_first = context["run_first_corner"][run_index]
                for packed in context["packed_corners"][
                        corner_first:corner_first + triangle_count * 3]:
                    dense_id = packed & (native.PACKED_DENSE_ID_LIMIT - 1)
                    if submit_class == 0:
                        require(
                            packed < native.PACKED_DENSE_ID_LIMIT,
                            f"{owner_name}: raw corner retains packed slot bits",
                        )
                        binding = root_index
                    else:
                        palette_slot = packed >> native.PACKED_DENSE_ID_BITS
                        binding = (
                            root_index
                            if palette_slot == native.PACKED_GX_SLOT_CURRENT
                            else slot_bindings.get(palette_slot, -1)
                        )
                    require(binding >= 0,
                            f"{owner_name}: unmapped cross-corner slot")
                    x, y, z = context["dense_vertices"][dense_id][:3]
                    xy, z_word = native.pack_fifo_vertex16(
                        x, y, z, f"{owner_name} direct corner {len(corners)}"
                    )
                    corners.append((
                        root_index, epoch_index, dense_id, binding,
                        dense_id if textured else -1, xy, z_word,
                    ))
                    submit_classes.append(submit_class)
    return tuple(states), tuple(corners), tuple(submit_classes)


def build_packet_trace(owner_name: str, plan: dict, context: dict):
    """Decode the retained exact FIFO packet into the same semantic stream."""
    _, _, commands = packet.decode_payload(plan["words"])
    matrix_roots = {
        word: source for word, source, count in plan["matrix_patches"]
        if count == 16
    }
    colors = dict(plan["color_patches"])
    texcoords = dict(plan["texcoord_patches"])
    begins = {
        (patch[3], patch[4], patch[11]): (patch[9], patch[10])
        for patch in plan["epoch_patches"]
    }
    state_words = {
        native.FIFO_TEX_FORMAT: {
            patch[0]: patch[10] for patch in plan["epoch_patches"]
        },
        native.FIFO_PAL_FORMAT: {
            patch[1]: patch[10] for patch in plan["epoch_patches"]
        },
        native.FIFO_POLY_FORMAT: {
            patch[2]: patch[10] for patch in plan["epoch_patches"]
        },
    }
    slot_bindings = {}
    states = []
    corners = []
    root_index = None
    epoch_index = None
    active_binding = None
    color_source = None
    texcoord_source = None
    pending_state = []

    for command_word, command, parameter_word, command_shift in commands:
        if (command == native.FIFO_MTX_LOAD_4X4 and
                parameter_word in matrix_roots):
            root_index = matrix_roots[parameter_word]
            active_binding = root_index
        elif command == native.FIFO_MTX_STORE:
            require(active_binding is not None,
                    f"{owner_name}: matrix store precedes a root")
            slot_bindings[plan["words"][parameter_word]] = active_binding
        elif command == native.FIFO_MTX_RESTORE:
            slot = plan["words"][parameter_word]
            require(slot in slot_bindings,
                    f"{owner_name}: restore of unstored slot {slot}")
            active_binding = slot_bindings[slot]
        elif command in state_words:
            require(parameter_word in state_words[command],
                    f"{owner_name}: unpatched state command 0x{command:02x}")
            pending_state.append((command, state_words[command][parameter_word]))
        elif command == native.FIFO_BEGIN:
            key = (command_word, parameter_word, command_shift)
            require(key in begins, f"{owner_name}: unpatched BEGIN command")
            expected_root, epoch_index = begins[key]
            require(root_index == expected_root,
                    f"{owner_name}: BEGIN/root association changed")
            require(pending_state == [
                (native.FIFO_TEX_FORMAT, epoch_index),
                (native.FIFO_PAL_FORMAT, epoch_index),
                (native.FIFO_POLY_FORMAT, epoch_index),
            ], f"{owner_name}: epoch {epoch_index} state trace changed")
            pending_state.clear()
            policy = context["direct_policies"][epoch_index]
            states.append((
                root_index, epoch_index, policy, *context["epochs"][epoch_index]
            ))
        elif command == native.FIFO_COLOR:
            require(parameter_word in colors,
                    f"{owner_name}: unpatched COLOR command")
            color_source = colors[parameter_word]
        elif command == native.FIFO_TEX_COORD:
            require(parameter_word in texcoords,
                    f"{owner_name}: unpatched TEX_COORD command")
            texcoord_source = texcoords[parameter_word]
        elif command == native.FIFO_VERTEX16:
            require(None not in (root_index, epoch_index, active_binding,
                                 color_source),
                    f"{owner_name}: incomplete packet corner state")
            corners.append((
                root_index, epoch_index, color_source, active_binding,
                -1 if texcoord_source is None else texcoord_source,
                plan["words"][parameter_word],
                plan["words"][parameter_word + 1],
            ))
            color_source = None
            texcoord_source = None
    require(color_source is None and texcoord_source is None,
            f"{owner_name}: packet ended inside a corner")
    require(not pending_state,
            f"{owner_name}: packet ended inside a state epoch")
    return tuple(states), tuple(corners)


def round_shift(value: int, shift: int = 12) -> int:
    bias = 1 << (shift - 1)
    return (-(((-value) + bias) >> shift)
            if value < 0 else (value + bias) >> shift)


def multiply4(lhs, rhs):
    return tuple(tuple(round_shift(sum(
        lhs[row][k] * rhs[k][column] for k in range(4)
    )) for column in range(4)) for row in range(4))


def fixture_matrix(owner: int, joint: int, scenario: int):
    """Distinct, bounded 20.12 locals exercise branches and signed rounding."""
    one = 1 << 12
    selector = owner * 37 + joint * 11 + scenario * 5
    result = [
        [one, 0, 0, 0], [0, one, 0, 0],
        [0, 0, one, 0], [0, 0, 0, one],
    ]
    result[0][0] += (selector % 5) - 2
    result[0][1] = ((selector * 17) % 257) - 128
    result[1][0] = ((selector * 29) % 255) - 127
    result[1][1] += ((selector >> 1) % 7) - 3
    result[2][2] += ((selector >> 2) % 5) - 2
    result[3][0] = 17 + joint * 13
    result[3][1] = -9 - scenario * 7
    result[3][2] = 3 + owner * 19
    return tuple(tuple(row) for row in result)


def simulate_binding_matrices(
        owner_name: str, owner_slot: int, schedule, scenario: int):
    one = 1 << 12
    camera = (
        (one, 0, (scenario & 1) * 31, 0),
        (0, one, 0, 0),
        (-(scenario & 1) * 31, 0, one, 0),
        (123 + scenario, -234, 345, one),
    )
    identity = (
        (one, 0, 0, 0), (0, one, 0, 0),
        (0, 0, one, 0), (0, 0, 0, one),
    )
    current = identity
    current_joint = 31
    stack = []
    bindings = {}
    slots = {}
    pushes = 0
    pops = 0
    for joint_index, packed in enumerate(schedule):
        parent = packed & 31
        while current_joint != parent:
            require(stack,
                    f"{owner_name}: joint {joint_index} cannot restore parent "
                    f"{parent}")
            current_joint, current = stack.pop()
            pops += 1
        if packed & native.JOINT_SCHEDULE_PUSH_BEFORE:
            stack.append((current_joint, current))
            pushes += 1
        current = multiply4(
            fixture_matrix(owner_slot, joint_index, scenario), current
        )
        current_joint = joint_index
        binding = (packed >> 5) & 31
        physical_slot = (packed >> 10) & 31
        if binding != 31:
            # BattleShip rounds the source world hierarchy first, then applies
            # the camera once. Fixed-point multiplication is not associative.
            composed = multiply4(current, camera)
            bindings[binding] = composed
            if physical_slot != 31:
                slots[physical_slot] = composed
    while stack:
        current_joint, current = stack.pop()
        pops += 1
    require(current_joint == 31,
            f"{owner_name}: hierarchy did not restore the camera")
    return bindings, slots, pushes, pops, camera


def build_source_light_oracle(owner_slot: int, schedule, scenario: int, camera):
    """Independent parent-recursive source-world then world*camera oracle."""
    one = 1 << 12
    identity = (
        (one, 0, 0, 0), (0, one, 0, 0),
        (0, 0, one, 0), (0, 0, 0, one),
    )
    worlds = [None] * len(schedule)

    def resolve(joint_index: int):
        if worlds[joint_index] is not None:
            return worlds[joint_index]
        parent = schedule[joint_index] & 31
        parent_world = identity if parent == 31 else resolve(parent)
        worlds[joint_index] = multiply4(
            fixture_matrix(owner_slot, joint_index, scenario), parent_world)
        return worlds[joint_index]

    bindings = {}
    for joint_index, packed in enumerate(schedule):
        binding = (packed >> 5) & 31
        if binding != 31:
            bindings[binding] = multiply4(resolve(joint_index), camera)
    return bindings


def build_camera_seeded_oracle(owner_slot: int, schedule, scenario: int, camera):
    """Former wrong association, retained only to prove fixture sensitivity."""
    worlds = []
    bindings = {}
    for joint_index, packed in enumerate(schedule):
        parent = packed & 31
        parent_world = camera if parent == 31 else worlds[parent]
        world = multiply4(
            fixture_matrix(owner_slot, joint_index, scenario), parent_world)
        worlds.append(world)
        binding = (packed >> 5) & 31
        if binding != 31:
            bindings[binding] = world
    return bindings


def check_binding_matrix_contract(
        owner_name: str, owner_slot: int, schedule, cross_slots,
        corners, submit_classes):
    binding_cells = 0
    cross_corners = 0
    wrong_association_cells = 0
    for scenario in range(8):
        candidate, slots, pushes, pops, camera = simulate_binding_matrices(
            owner_name, owner_slot, schedule, scenario
        )
        reference = build_source_light_oracle(
            owner_slot, schedule, scenario, camera)
        camera_seeded = build_camera_seeded_oracle(
            owner_slot, schedule, scenario, camera)
        require(candidate == reference,
                f"{owner_name}: CPU binding matrix parity failed")
        wrong_association_cells += sum(
            camera_seeded[binding][row][column] != matrix[row][column]
            for binding, matrix in reference.items()
            for row in range(3) for column in range(3)
        )
        expected_push = native.OWNER_GX_PLAN_COUNTS[owner_name][1]
        require((pushes, pops) == (expected_push, expected_push),
                f"{owner_name}: simulated push/pop count changed")
        binding_cells += len(reference) * 16
        for corner, submit_class in zip(corners, submit_classes):
            root, _, _, binding, _, _, _ = corner
            if binding == root:
                actual = candidate[root]
            else:
                slot = cross_slots[binding]
                require(slot in slots,
                        f"{owner_name}: cross binding {binding} was not stored")
                actual = slots[slot]
            require(actual == reference[binding],
                    f"{owner_name}: cross-corner matrix parity failed")
            if submit_class == 1:
                cross_corners += 1
    expected_wrong_cells = 35 if owner_name == "mario" else 52
    require(wrong_association_cells == expected_wrong_cells,
            f"{owner_name}: association fixture lost sensitivity "
            f"({wrong_association_cells} != {expected_wrong_cells})")
    return binding_cells, cross_corners, wrong_association_cells


def preflight_trace(owner_name: str, expected_state, expected_corners,
                    candidate_state, candidate_corners):
    require(candidate_state == expected_state,
            f"{owner_name}: retained packet state trace mismatch")
    require(candidate_corners == expected_corners,
            f"{owner_name}: retained packet corner trace mismatch")
    return candidate_state, candidate_corners


def commit_trace(preflighted):
    """Accepted immutable traces have no post-commit rejection path."""
    states, corners = preflighted
    return len(states), len(corners), sum(row[2] for row in corners)


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

    data = native.decode_export()
    owner_roots = {
        "mario": native.unpack_many("<IHHHBBBB2x", data["mario_roots"]),
        "fox": native.unpack_many("<IHHHBBBB2x", data["fox_roots"]),
    }
    require(args.generated.is_file(), f"missing generated include: {args.generated}")
    checked_in_generated = args.generated.read_text()
    generated = native.generate(source_root)
    require(checked_in_generated == generated,
            f"stale generated include: {args.generated}")
    retained_plans, _ = packet.parse_generated_plans(checked_in_generated)
    _, context = packet.build_plans(source_root)

    total_pushes = 0
    total_pops = 0
    total_stores = 0
    total_restores = 0
    total_states = 0
    total_corners = 0
    total_binding_cells = 0
    total_cross_corner_samples = 0
    commits = []
    mutations = 0
    for owner_name in ("mario", "fox"):
        payload = native.load_o2r_payload(source_root, owner_name)
        (
            schedule,
            _binding_parents,
            binding_joints,
            cross_slots,
            hierarchy_counts,
        ) = native.decode_joint_topology(
            payload, owner_name, owner_roots[owner_name]
        )
        expected_joints, expected_bindings = native.OWNER_PLAN_COUNTS[owner_name]
        require(len(schedule) == expected_joints,
                f"{owner_name}: joint count changed")
        require(len(binding_joints) == expected_bindings,
                f"{owner_name}: binding count changed")
        require(tuple(schedule) == EXPECTED_SCHEDULES[owner_name],
                f"{owner_name}: packed hierarchy schedule changed")

        expected_slots = [native.PACKED_GX_SLOT_CURRENT] * expected_bindings
        for logical_binding, physical_slot in (
                native.OWNER_CROSS_BINDING_SLOTS[owner_name]):
            expected_slots[logical_binding] = physical_slot
        require(cross_slots == expected_slots,
                f"{owner_name}: logical-to-physical slot map changed")
        require(all(
            slot == native.PACKED_GX_SLOT_CURRENT or
            native.GX_HIERARCHY_SLOT_LIMIT <= slot <
            native.PACKED_GX_SLOT_CURRENT
            for slot in cross_slots
        ), f"{owner_name}: a physical slot overlaps the hierarchy")

        for logical_binding, joint_index in enumerate(binding_joints):
            packed = schedule[joint_index]
            require(((packed >> 5) & 31) == logical_binding,
                    f"{owner_name}: logical binding {logical_binding} was remapped")
            require(((packed >> 10) & 31) == cross_slots[logical_binding],
                    f"{owner_name}: physical slot mismatch at binding "
                    f"{logical_binding}")

        camera_seeds, pushes, pops, max_source_depth = hierarchy_counts
        expected_seed, expected_push, expected_pop, stores, restores = (
            native.OWNER_GX_PLAN_COUNTS[owner_name]
        )
        require((camera_seeds, pushes, pops) ==
                (expected_seed, expected_push, expected_pop),
                f"{owner_name}: seed/push/pop accounting changed")
        require(sum(
            bool(value & native.JOINT_SCHEDULE_PUSH_BEFORE)
            for value in schedule
        ) == pushes, f"{owner_name}: bit-15 push census changed")

        title = owner_name.title()
        require(f"sNdsNative{title}JointSchedule[{expected_joints}]" in generated,
                f"{owner_name}: generated schedule table is absent")
        require(f"sNdsNative{title}CrossPaletteSlots[{expected_bindings}]" in
                generated,
                f"{owner_name}: generated physical-slot table is absent")

        packet_state, packet_corners = build_packet_trace(
            owner_name, retained_plans[owner_name], context
        )
        direct_state, direct_corners, submit_classes = build_direct_trace(
            owner_name, context, cross_slots
        )
        accepted = preflight_trace(
            owner_name, packet_state, packet_corners,
            direct_state, direct_corners,
        )
        owner_slot = 0 if owner_name == "mario" else 1
        (
            binding_cells,
            cross_corner_samples,
            wrong_association_cells,
        ) = check_binding_matrix_contract(
            owner_name, owner_slot, schedule, cross_slots,
            direct_corners, submit_classes,
        )
        commits.append(commit_trace(accepted))

        mutated_state = list(direct_state)
        changed = list(mutated_state[0])
        changed[2] ^= 1
        mutated_state[0] = tuple(changed)
        expect_value_error(
            lambda: preflight_trace(
                owner_name, packet_state, packet_corners,
                tuple(mutated_state), direct_corners,
            ), f"{owner_name}: state mutation reached commit",
        )
        mutations += 1
        mutated_corners = list(direct_corners)
        changed = list(mutated_corners[0])
        changed[2] += 1
        mutated_corners[0] = tuple(changed)
        expect_value_error(
            lambda: preflight_trace(
                owner_name, packet_state, packet_corners,
                direct_state, tuple(mutated_corners),
            ), f"{owner_name}: corner mutation reached commit",
        )
        mutations += 1

        total_states += len(direct_state)
        total_corners += len(direct_corners)
        total_binding_cells += binding_cells
        total_cross_corner_samples += cross_corner_samples

        total_pushes += pushes
        total_pops += pops
        total_stores += stores
        total_restores += restores
        slot_text = ",".join(
            f"{binding}->{slot}"
            for binding, slot in native.OWNER_CROSS_BINDING_SLOTS[owner_name]
        )
        print(
            f"{owner_name}: joints={len(schedule)} bindings={len(binding_joints)} "
            f"cameraSeeds={camera_seeds} push/pop={pushes}/{pops} "
            f"sourceDepth={max_source_depth} stores={stores} restores={restores} "
            f"states/corners={len(direct_state)}/{len(direct_corners)} "
            f"associationGuard={wrong_association_cells} slots={slot_text}"
        )

    parent_mutation = list(EXPECTED_SCHEDULES["mario"])
    parent_mutation[8] = (parent_mutation[8] & ~31) | 7
    mario_roots = context["owner_roots"]["mario"]
    mario_topology = native.decode_joint_topology(
        native.load_o2r_payload(source_root, "mario"),
        "mario", mario_roots,
    )
    mario_trace = build_direct_trace("mario", context, mario_topology[3])
    expect_value_error(
        lambda: check_binding_matrix_contract(
            "mario", 0, parent_mutation, mario_topology[3],
            mario_trace[1], mario_trace[2],
        ), "mario: parent mutation preserved CPU binding parity",
    )
    mutations += 1
    binding_mutation = list(EXPECTED_SCHEDULES["mario"])
    binding5 = (binding_mutation[5] >> 5) & 31
    binding6 = (binding_mutation[6] >> 5) & 31
    binding_mutation[5] = (
        (binding_mutation[5] & ~(31 << 5)) | (binding6 << 5)
    )
    binding_mutation[6] = (
        (binding_mutation[6] & ~(31 << 5)) | (binding5 << 5)
    )
    expect_value_error(
        lambda: check_binding_matrix_contract(
            "mario", 0, binding_mutation, mario_topology[3],
            mario_trace[1], mario_trace[2],
        ), "mario: binding mutation preserved CPU binding parity",
    )
    mutations += 1

    runs = native.unpack_many("<HBBI", data["runs"])
    cross_triangles = sum(count for _, count, submit_class, _ in runs
                          if submit_class == 1)
    require((total_pushes, total_pops, total_stores, total_restores,
             cross_triangles) == (11, 11, 10, 84, 44),
            "combined GX hierarchy accounting changed")
    require((total_states, total_corners) == (49, 1878),
            "combined direct trace cardinality changed")
    require([commit[:2] for commit in commits] == [(18, 960), (31, 918)],
            "commit trace cardinality changed")
    # This is the generated logical matrix/root payload gate, not linked BSS.
    # Actual candidate reserve is enforced by the ROM verifier/map evidence.
    logical_workspace_bytes = (
        sum(counts[0] for counts in native.OWNER_PLAN_COUNTS.values()) * 64 +
        sum(counts[1] for counts in native.OWNER_PLAN_COUNTS.values()) * 12
    )
    require(logical_workspace_bytes <= 4160,
            "logical owner payload exceeds the 4160-byte gate")
    print(
        "combined: push/pop=11/11 stores=10 restores=84 "
        f"crossTriangles=44 states/corners={total_states}/{total_corners} "
        f"bindingCells={total_binding_cells} "
        f"crossCornerSamples={total_cross_corner_samples} "
        f"mutations={mutations} logicalWorkspace={logical_workspace_bytes}/4160 "
        "commits=2 postCommitFailures=0 logicalBindingABI=preserved "
        "packedSchedule=u16"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
