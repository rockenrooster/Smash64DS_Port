#!/usr/bin/env python3
"""Prove every native fighter owner's geometry survives the whole pipeline.

Two closures, for EVERY landed owner in BOTH detail levels -- which is the part
that was missing. `check_fighter_primitive_streams.py` proves the second closure
only for the frozen Mario/Fox HIGH context (`build_owner_source_context` with no
owner and no detail), so Luigi, Donkey, and every LOW program were outside what
any checker could express. Board row P2-3r17 opened on a missing-triangle report
and had to build this before it could say the tables were innocent.

  1. SOURCE closure. Walk each owner's O2R display lists with a decoder that is
     independent of the generator's export path, and compare -- per drawable
     root, triangle for triangle, in order -- against the triangles the
     generated runs/epochs declare. A span bug that drops the last triangle of
     every run, or a G_TRI2 whose second half is lost, shows up here and
     nowhere else: the runtime validator only checks that spans fit their own
     arrays, which a consistently-truncated table passes.

  2. PRIMITIVE closure. Expand every Task 56 primitive group back into oriented
     triangles under the DS strip rule AND the runtime's BEGIN policy
     (`ndsRendererNativeEmitProductionPrimitiveGroups`: a GL_TRIANGLE group
     following a GL_TRIANGLE group shares the open list, everything else opens
     its own), then compare against the run's source triangles WITH ORIENTATION.
     A reversed strip is back-face culled on hardware with no assert, no
     counter and no hang -- the fighter simply loses polygons.

Run with no arguments; exits non-zero on any mismatch.
"""
from __future__ import annotations

import struct
import sys
from collections import defaultdict
from pathlib import Path

_scripts_root = Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in sys.path:
    sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402  -- puts every scripts/ area folder on sys.path

import generate_nds_native_owners as native  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_fighter_primitive_streams as streams  # noqa: E402

REPO = _paths.REPO_ROOT

# Every owner the runtime can select, in both detail levels. Mario/Fox share one
# combined table set (P2-2's frozen program); P2-3 owners get independent ones.
OWNERS = ("mario", "fox", "luigi", "donkey")
DETAILS = ("high", "low")

DOBJ_DESC_SIZE = native.DOBJ_DESC_SIZE
G_TRI1 = 0x05
G_TRI2 = 0x06
G_ENDDL = 0xDF
MAX_DL_COMMANDS = 4096


def walk_root_triangles(payload: bytes, offset: int) -> list[tuple[int, int, int]]:
    """Independent DL walk: the (v0,v1,v2) cache-slot triples one root draws."""
    tris: list[tuple[int, int, int]] = []
    index = 0
    while True:
        if index >= MAX_DL_COMMANDS:
            raise ValueError(f"root 0x{offset:x}: runaway display list")
        w0, w1 = struct.unpack_from(">II", payload, offset + index * 8)
        op = w0 >> 24
        if op == G_ENDDL:
            return tris
        if op in (G_TRI1, G_TRI2):
            packed = [w0 & 0xFFFFFF]
            if op == G_TRI2:
                packed.append(w1 & 0xFFFFFF)
            for value in packed:
                tris.append((
                    ((value >> 16) & 0xFF) // 2,
                    ((value >> 8) & 0xFF) // 2,
                    (value & 0xFF) // 2,
                ))
        index += 1


def joint_tree(payload: bytes, owner: str, detail: str):
    offset, count = (native.OWNER_JOINT_TREES[owner] if detail == "high"
                     else native.OWNER_JOINT_TREES_LOW[owner])
    out = []
    for i in range(count):
        depth, pointer = struct.unpack_from(
            ">II", payload, offset + i * DOBJ_DESC_SIZE)
        out.append((depth, None if pointer == 0 else (pointer & 0xFFFF) * 4))
    return out


def owner_program(owner: str, detail: str) -> dict:
    """The generated runtime program for one owner, whichever pipeline made it."""
    if owner in ("mario", "fox"):
        context = native.build_owner_source_context(REPO, detail)
        (dense_vertices, dense_color_sources, dense_owners, dense_corners,
         action_dense_first, run_first_corner, run_owners, run_root_bindings,
         run_binding_sets) = native.build_dense_geometry(
            context["vertex"], context["triangles"], context["runs"],
            context["epochs"], context["owner_roots"], REPO)
        owner_cross_slots = [t[3] for t in context["owner_topologies"]]
        (_spans, packed_corners, _rfu, _ruc,
         _rud) = native.build_direct_dense_tables(
            context["vertex"], context["runs"], dense_vertices,
            dense_color_sources, dense_corners, action_dense_first,
            run_first_corner, run_owners, run_root_bindings, run_binding_sets,
            owner_cross_slots, detail)
        return {
            "roots": dict(context["owner_roots"])[owner],
            "runs": context["runs"],
            "epochs": context["epochs"],
            "triangles": context["triangles"],
            "packed_corners": packed_corners,
            "run_first_corner": run_first_corner,
            # Mario and Fox share one run/corner space; the primitive closure
            # covers the whole shared program once, under "mario".
            "shared": True,
        }
    context = native.build_p2_owner_runtime_context(REPO, owner, detail)
    return {
        "roots": context["roots"],
        "runs": context["runs"],
        "epochs": context["epochs"],
        "triangles": context["triangles"],
        "packed_corners": context["packed_corners"],
        "run_first_corner": context["run_first_corner"],
        "shared": False,
    }


def source_closure(owner: str, detail: str, program: dict) -> list[str]:
    payload = native.load_o2r_payload(REPO, owner)
    descriptors = joint_tree(payload, owner, detail)
    setup = native.OWNER_SETUP_PARTS[owner]
    selected = [i for i in range(len(descriptors) - 1)
                if setup[i // 32] & (1 << (31 - (i & 31)))]
    drawable_anywhere = [i for i, (_d, o) in enumerate(descriptors[:-1])
                         if o is not None]
    roots = [descriptors[i][1] for i in selected
             if descriptors[i][1] is not None]
    failures: list[str] = []
    dropped = [i for i in drawable_anywhere if i not in selected]
    if dropped:
        failures.append(
            f"{owner} {detail}: setup_parts excludes drawable descriptors {dropped}")

    runs = program["runs"]
    epochs = program["epochs"]
    triangles = program["triangles"]
    if len(roots) != len(program["roots"]):
        failures.append(
            f"{owner} {detail}: {len(roots)} source roots vs "
            f"{len(program['roots'])} generated roots")
        return failures

    source_total = table_total = 0
    for ordinal, (offset, root) in enumerate(zip(roots, program["roots"])):
        source = walk_root_triangles(payload, offset)
        table = []
        for epoch_index in range(root[1], root[1] + root[4]):
            epoch = epochs[epoch_index]
            for run_index in range(epoch[3], epoch[3] + epoch[9]):
                first, count = runs[run_index][0], runs[run_index][1]
                for t in range(first, first + count):
                    compact = triangles[t] & 0x7FFF
                    table.append((
                        (compact >> 10) & 31, (compact >> 5) & 31, compact & 31))
        source_total += len(source)
        table_total += len(table)
        if source != table:
            if len(source) != len(table):
                failures.append(
                    f"{owner} {detail} root {ordinal} @0x{offset:x}: source "
                    f"{len(source)} triangles vs table {len(table)}")
            else:
                first_bad = next(
                    i for i, (s, t) in enumerate(zip(source, table)) if s != t)
                failures.append(
                    f"{owner} {detail} root {ordinal} @0x{offset:x}: triangle "
                    f"{first_bad} is {table[first_bad]}, source has "
                    f"{source[first_bad]}")
    print(f"  source closure: {len(roots)} roots, {source_total} source "
          f"triangles, {table_total} in tables")
    return failures


def primitive_closure(owner: str, detail: str, program: dict) -> list[str]:
    failures: list[str] = []
    for mode in (1, 2):
        result = streams.check_mode(
            mode, program["runs"], program["packed_corners"],
            program["run_first_corner"])
        print(f"  primitive closure mode {mode}: {result['vertices']:,} vertex "
              f"submissions in {result['groups']:,} groups, "
              f"{result['emitted_triangles']:,} drawn against "
              f"{result['src_triangles']:,}")
        for row in result["bad_set"][:4]:
            failures.append(f"{owner} {detail} mode {mode}: missing/extra {row}")
        for row in result["bad_winding"][:4]:
            failures.append(
                f"{owner} {detail} mode {mode}: run {row[0]} class {row[1]} "
                f"emitted {row[2]} against source {row[3]} -- reversed winding, "
                "back-face culled on hardware")
        if result["bad_set"] and len(result["bad_set"]) > 4:
            failures.append(
                f"{owner} {detail} mode {mode}: {len(result['bad_set'])} "
                "missing/extra triangles in total")
        if result["bad_winding"] and len(result["bad_winding"]) > 4:
            failures.append(
                f"{owner} {detail} mode {mode}: {len(result['bad_winding'])} "
                "reversed triangles in total")
    return failures


def winding_closure(owner: str, detail: str, program: dict) -> list[str]:
    """Every interior edge is traversed in OPPOSITE directions by its two
    triangles. A source mesh that fails this has triangles that cannot all face
    the camera at once, so a hole there is the asset's, not the port's."""
    runs, epochs = program["runs"], program["epochs"]
    corners = program["packed_corners"]
    first_corner = program["run_first_corner"]
    bad = 0
    for root in program["roots"]:
        edges: dict[frozenset, list[tuple[int, int]]] = defaultdict(list)
        for epoch_index in range(root[1], root[1] + root[4]):
            epoch = epochs[epoch_index]
            for run_index in range(epoch[3], epoch[3] + epoch[9]):
                base = first_corner[run_index]
                for t in range(runs[run_index][1]):
                    tri = tuple(corners[base + t * 3 + k] & 0x3FF
                                for k in range(3))
                    for u, v in ((tri[0], tri[1]), (tri[1], tri[2]),
                                 (tri[2], tri[0])):
                        edges[frozenset((u, v))].append((u, v))
        for uses in edges.values():
            if len(uses) == 2 and uses[0] == uses[1]:
                bad += 1
    print(f"  winding closure: {bad} inconsistently wound shared edges")
    return ([f"{owner} {detail}: {bad} inconsistently wound shared edges"]
            if bad else [])


def main() -> int:
    failures: list[str] = []
    for owner in OWNERS:
        for detail in DETAILS:
            print(f"{owner} {detail}:")
            program = owner_program(owner, detail)
            failures += source_closure(owner, detail, program)
            failures += winding_closure(owner, detail, program)
            # Mario and Fox are one run/corner program; expand it once.
            if owner != "fox":
                failures += primitive_closure(owner, detail, program)
    if failures:
        print("NATIVE_OWNER_GEOMETRY_CLOSURE_FAIL")
        for line in failures:
            print(f"  {line}")
        return 1
    print("NATIVE_OWNER_GEOMETRY_CLOSURE_OK "
          "every source triangle reaches the emitted primitive stream "
          "exactly once, with source winding, for every owner and detail")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
