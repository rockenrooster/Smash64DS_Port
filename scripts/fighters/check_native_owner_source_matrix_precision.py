#!/usr/bin/env python3
"""Prove the Mario/DK seam fix matches BattleShip's render matrix boundary.

BattleShip does NOT concatenate fighter joints in a 12-fraction-bit matrix
format.  lbCommonFighterPartsFuncMatrix builds one N64 s15.16 local per joint;
Fast3D decodes that local to float and concatenates the hierarchy in float.
The DS GX hierarchy historically converted every local to Q20.12 first, then
multiplied those reduced-precision locals down the tree.  This checker compares
those two pipelines at the source vertices that the native owner really draws.

The candidate route in renderer_adapter_matrix.c retains 20 fractional bits
through the hierarchy and converts only the finished world to Q20.12. Its final
representation error must stay below one source 1/16 coordinate step for every
drawn Mario/DK vertex and within one final DS matrix LSB of Fast3D, while the
old repeated-Q20.12 path is retained here as the positive control.
"""
from __future__ import annotations

import importlib.util
import math
import struct
import sys
from pathlib import Path

_scripts_root = Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in sys.path:
    sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402

import generate_nds_native_owners as native  # noqa: E402
import native_matrix_math as matrix  # noqa: E402

REPO = _paths.REPO_ROOT
OWNERS = ("mario", "donkey")
DETAILS = ("high", "low")
SOURCE_STEP = 1.0 / 16.0


def _load_geometry_closure_module():
    path = Path(__file__).with_name("check_native_owner_geometry_closure.py")
    spec = importlib.util.spec_from_file_location("native_geometry_closure", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


closure = _load_geometry_closure_module()
SINT = matrix._load_sint_table(REPO)


def f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", float(value)))[0]


def fmul(lhs: float, rhs: float) -> float:
    return f32(f32(lhs) * f32(rhs))


def fadd(lhs: float, rhs: float) -> float:
    return f32(f32(lhs) + f32(rhs))


def float_matrix_identity() -> list[list[float]]:
    return [[1.0 if row == col else 0.0 for col in range(4)]
            for row in range(4)]


def float_matrix_mul(lhs: list[list[float]], rhs: list[list[float]]
                     ) -> list[list[float]]:
    """BattleShip Fast3D Interpreter::MatrixMul, including f32 boundaries."""
    out = [[0.0] * 4 for _ in range(4)]
    for row in range(4):
        for col in range(4):
            total = fmul(lhs[row][0], rhs[0][col])
            total = fadd(total, fmul(lhs[row][1], rhs[1][col]))
            total = fadd(total, fmul(lhs[row][2], rhs[2][col]))
            total = fadd(total, fmul(lhs[row][3], rhs[3][col]))
            out[row][col] = total
    return out


def source_fighter_local_s16p16(translate, rotate, scale) -> list[list[int]]:
    """lbCommonFighterPartsFuncMatrix ordinary branch, as unpacked s15.16."""
    sinr, cosr = matrix._sincos_ushort(rotate[0], SINT)
    sinp, cosp = matrix._sincos_ushort(rotate[1], SINT)
    siny, cosy = matrix._sincos_ushort(rotate[2], SINT)

    if scale == (1.0, 1.0, 1.0):
        rows = [
            [(cosp * cosy) >> 14,
             (cosp * siny) >> 14,
             -sinp * 2, 0],
            [((((sinr * sinp) >> 15) * cosy) >> 14) -
             ((cosr * siny) >> 14),
             ((((sinr * sinp) >> 15) * siny) >> 14) +
             ((cosr * cosy) >> 14),
             (sinr * cosp) >> 14, 0],
            [((((cosr * sinp) >> 15) * cosy) >> 14) +
             ((sinr * siny) >> 14),
             ((((cosr * sinp) >> 15) * siny) >> 14) -
             ((sinr * cosy) >> 14),
             (cosr * cosp) >> 14, 0],
        ]
    else:
        scalex = int(scale[0] * 256.0)
        scaley = int(scale[1] * 256.0)
        scalez = int(scale[2] * 256.0)
        rows = [
            [(((cosp * cosy) >> 14) * scalex) >> 8,
             (((cosp * siny) >> 14) * scalex) >> 8,
             (-sinp * scalex) >> 7, 0],
            [((((((sinr * sinp) >> 15) * cosy) >> 14) -
               ((cosr * siny) >> 14)) * scaley) >> 8,
             (((((sinr * sinp) >> 15) * siny) >> 14) +
               ((cosr * cosy) >> 14)) * scaley >> 8,
             (((sinr * cosp) >> 14) * scaley) >> 8, 0],
            [((((((cosr * sinp) >> 15) * cosy) >> 14) +
               ((sinr * siny) >> 14)) * scalez) >> 8,
             (((((cosr * sinp) >> 15) * siny) >> 14) -
               ((sinr * cosy) >> 14)) * scalez >> 8,
             (((cosr * cosp) >> 14) * scalez) >> 8, 0],
        ]
    rows.append([
        matrix._ftofix32(translate[0]),
        matrix._ftofix32(translate[1]),
        matrix._ftofix32(translate[2]),
        1 << 16,
    ])
    return rows


def s16p16_to_float(source: list[list[int]]) -> list[list[float]]:
    return [[f32(cell / 65536.0) for cell in row] for row in source]


def s16p16_to_q20p12(source: list[list[int]]) -> matrix.NdsMatrix20p12:
    return matrix.NdsMatrix20p12([
        [matrix.round_shift_s32(cell, 4) for cell in row]
        for row in source
    ])


RETAINED_FRAC_BITS = 20


def mul_affine_retained(lhs: list[list[int]], rhs: list[list[int]]
                        ) -> list[list[int]]:
    """Compose source locals without reducing them to the DS's Q20.12.

    This is the proposed ARM9 representation: the immutable BattleShip local
    boundary is already s15.16, and retaining those 16 fractional bits through
    the hierarchy removes the soft-float work while postponing the one Q20.12
    conversion until the selected draw binding.
    """
    out = [[0] * 4 for _ in range(4)]
    for row in range(3):
        for col in range(3):
            total = sum(lhs[row][k] * rhs[k][col] for k in range(3))
            out[row][col] = matrix.round_shift_s64(total, RETAINED_FRAC_BITS)
    for col in range(3):
        total = (sum(lhs[3][k] * rhs[k][col] for k in range(3)) +
                 lhs[3][3] * rhs[3][col])
        out[3][col] = matrix.round_shift_s64(total, RETAINED_FRAC_BITS)
    out[3][3] = 1 << RETAINED_FRAC_BITS
    return out


def retained_to_q20p12(source: list[list[int]]) -> matrix.NdsMatrix20p12:
    return matrix.NdsMatrix20p12([
        [matrix.round_shift_s32(cell, RETAINED_FRAC_BITS - 12)
         for cell in row]
        for row in source
    ])


def float_to_q20p12(source: list[list[float]]) -> matrix.NdsMatrix20p12:
    # renderer_adapter_matrix.c F2LFixedWExact -> MtxFromN64.
    return matrix.NdsMatrix20p12([
        [matrix.round_shift_s32(matrix._ftofix32(f32(cell)), 4)
         for cell in row]
        for row in source
    ])


def transform_float(source: list[list[float]], vertex) -> tuple[float, float, float]:
    x, y, z = vertex
    result = []
    for col in range(3):
        total = fmul(source[0][col], x)
        total = fadd(total, fmul(source[1][col], y))
        total = fadd(total, fmul(source[2][col], z))
        total = fadd(total, source[3][col])
        result.append(total)
    return tuple(result)


def transform_q20p12(source: matrix.NdsMatrix20p12, vertex
                     ) -> tuple[float, float, float]:
    x, y, z = vertex
    return tuple(
        (source.m[0][col] * x + source.m[1][col] * y +
         source.m[2][col] * z + source.m[3][col]) / 4096.0
        for col in range(3)
    )


def distance(lhs, rhs) -> float:
    return math.sqrt(sum((lhs[i] - rhs[i]) ** 2 for i in range(3)))


def owner_joint_worlds(owner: str, detail: str, program: dict):
    payload = native.load_o2r_payload(REPO, owner)
    tree_offset, descriptor_count = (
        native.OWNER_JOINT_TREES[owner] if detail == "high"
        else native.OWNER_JOINT_TREES_LOW[owner]
    )
    selected = native._owner_selected_descriptor_indices(
        owner, descriptor_count - 1)
    transforms = []
    for descriptor_index in selected:
        _raw_id, _display, *values = struct.unpack_from(
            ">II9f", payload,
            tree_offset + descriptor_index * native.DOBJ_DESC_SIZE)
        transforms.append((tuple(values[:3]), tuple(values[3:6]),
                           tuple(values[6:9])))

    canonical_roots = program["roots"][:program["canonical_root_count"]]
    schedule, _binding_parents, binding_joints, _cross, _counts = (
        native.decode_joint_topology(payload, owner, canonical_roots, detail)
    )
    parents = [255 if (word & 31) == 31 else (word & 31)
               for word in schedule]
    if len(parents) != len(transforms) + 1:
        raise ValueError(
            f"{owner} {detail}: {len(parents)} joints vs "
            f"{len(transforms)} source descriptors + root")

    source_worlds = [float_matrix_identity() for _ in parents]
    repeated_q12_worlds = [matrix.NdsMatrix20p12.identity() for _ in parents]
    retained_worlds = [
        [[1 << RETAINED_FRAC_BITS if row == col else 0 for col in range(4)]
         for row in range(4)]
        for _ in parents
    ]
    for joint in range(1, len(parents)):
        parent = parents[joint]
        if parent >= joint:
            raise ValueError(f"{owner} {detail}: invalid parent {parent}->{joint}")
        local16 = source_fighter_local_s16p16(*transforms[joint - 1])
        local_float = s16p16_to_float(local16)
        local_q12 = s16p16_to_q20p12(local16)
        source_worlds[joint] = float_matrix_mul(
            local_float, source_worlds[parent])
        repeated_q12_worlds[joint] = matrix.mul_affine_20p12(
            local_q12, repeated_q12_worlds[parent])
        local_retained = [[cell << (RETAINED_FRAC_BITS - 16) for cell in row]
                          for row in local16]
        retained_worlds[joint] = mul_affine_retained(
            local_retained, retained_worlds[parent])

    exact_once_worlds = [float_to_q20p12(world) for world in source_worlds]
    fixed_once_worlds = [retained_to_q20p12(world)
                         for world in retained_worlds]
    return (binding_joints, source_worlds, repeated_q12_worlds,
            fixed_once_worlds, exact_once_worlds)


def check_owner(owner: str, detail: str) -> list[str]:
    program = closure.owner_program(owner, detail)
    (binding_joints, source_worlds, old_worlds, fixed_worlds, exact_worlds) = (
        owner_joint_worlds(owner, detail, program)
    )
    dense_ids: set[int] = set()
    for root in program["roots"]:
        for triangle in closure.emitted_triangles(program, root):
            dense_ids.update(triangle)

    old_errors = []
    fixed_errors = []
    exact_errors = []
    fixed_matrix_mismatches = 0
    fixed_matrix_max_delta = 0
    per_binding: dict[int, list[float]] = {}
    for joint in binding_joints:
        for row in range(4):
            for col in range(4):
                delta = abs(fixed_worlds[joint].m[row][col] -
                            exact_worlds[joint].m[row][col])
                fixed_matrix_mismatches += delta != 0
                fixed_matrix_max_delta = max(fixed_matrix_max_delta, delta)
    for dense_id in dense_ids:
        record = program["dense_vertices"][dense_id]
        binding = record[5]
        if binding >= len(binding_joints):
            continue
        joint = binding_joints[binding]
        vertex = record[:3]
        source_position = transform_float(source_worlds[joint], vertex)
        old_error = distance(
            transform_q20p12(old_worlds[joint], vertex), source_position)
        fixed_error = distance(
            transform_q20p12(fixed_worlds[joint], vertex), source_position)
        exact_error = distance(
            transform_q20p12(exact_worlds[joint], vertex), source_position)
        old_errors.append(old_error)
        fixed_errors.append(fixed_error)
        exact_errors.append(exact_error)
        row = per_binding.setdefault(binding, [0.0, 0.0, 0.0])
        row[0] = max(row[0], old_error)
        row[1] = max(row[1], fixed_error)
        row[2] = max(row[2], exact_error)

    old_max = max(old_errors, default=0.0)
    fixed_max = max(fixed_errors, default=0.0)
    exact_max = max(exact_errors, default=0.0)
    old_over = sum(error > SOURCE_STEP for error in old_errors)
    fixed_over = sum(error > SOURCE_STEP for error in fixed_errors)
    exact_over = sum(error > SOURCE_STEP for error in exact_errors)
    top = sorted(per_binding.items(), key=lambda item: item[1][0], reverse=True)[:5]
    print(
        f"{owner} {detail}: vertices={len(old_errors)} "
        f"repeated_q12_max={old_max:.6f} retained_q16_max={fixed_max:.6f} "
        f"exact_stack_max={exact_max:.6f} "
        f"over_1_16={old_over}->{fixed_over}->{exact_over} "
        f"q12_cell_mismatches={fixed_matrix_mismatches} "
        f"q12_max_delta={fixed_matrix_max_delta}")
    print("  worst bindings old/fixed/exact: " + " ".join(
        f"{binding}:{values[0]:.5f}/{values[1]:.5f}/{values[2]:.5f}"
        for binding, values in top))

    failures = []
    if exact_over != 0:
        failures.append(
            f"{owner} {detail}: exact Fast3D stack leaves {exact_over} vertices "
            "more than one 1/16 source step from the source renderer")
    if exact_max >= old_max:
        failures.append(
            f"{owner} {detail}: exact stack did not improve max error "
            f"({old_max:.6f}->{exact_max:.6f})")
    if fixed_matrix_max_delta > 1:
        failures.append(
            f"{owner} {detail}: retained fixed hierarchy differs from the "
            f"Fast3D result by {fixed_matrix_max_delta} final Q20.12 LSBs")
    if fixed_over != 0:
        failures.append(
            f"{owner} {detail}: retained-s16.16 hierarchy leaves {fixed_over} "
            "vertices more than one 1/16 source step from the source renderer")
    if detail == "high" and old_over == 0:
        failures.append(
            f"{owner} high: repeated-Q20.12 positive control no longer exposes "
            "the measured seam precision loss")
    return failures


def main() -> int:
    failures = []
    for owner in OWNERS:
        for detail in DETAILS:
            failures += check_owner(owner, detail)
    if failures:
        print("NATIVE_OWNER_SOURCE_MATRIX_PRECISION_FAIL")
        for failure in failures:
            print(f"  {failure}")
        return 1
    print(
        "NATIVE_OWNER_SOURCE_MATRIX_PRECISION_OK retained Q20 hierarchy keeps "
        "every drawn Mario/DK bind-pose vertex within one source 1/16 step "
        "and one final DS Q20.12 LSB of the BattleShip Fast3D stack")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
