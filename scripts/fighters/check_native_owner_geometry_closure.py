#!/usr/bin/env python3
"""Prove every native fighter owner's geometry survives the whole pipeline.

THE ONE ORACLE for the native fighter owner tables. It absorbs the former
`check_fighter_primitive_streams.py`, which could only ever build the frozen
Mario/Fox HIGH context (`build_owner_source_context` with no owner and no
detail), so Luigi, Donkey and every LOW program were outside what any checker
could express -- which is how a defect the owner could see survived every gate.
Every closure below runs for EVERY landed owner in BOTH detail levels.

  1. SOURCE closure. Walk each owner's O2R display lists with a decoder that is
     independent of the generator's export path, and compare -- per drawable
     root, triangle for triangle, in order -- against the triangles the
     generated runs/epochs declare. A span bug that drops the last triangle of
     every run, or a G_TRI2 whose second half is lost, shows up here and
     nowhere else: the runtime validator only checks that spans fit their own
     arrays, which a consistently-truncated table passes.

  2. VERTEX closure. Closure 1 compares triangles as CACHE-SLOT triples, which
     proves the topology and says NOTHING about which vertex a slot held. This
     re-walks the same display lists maintaining the real 32-entry vertex cache
     (G_VTX loads, G_MODIFYVTX ST overrides) and checks that the dense vertex
     each emitted corner points at carries that slot's exact source position,
     texcoord and normal. A dense id that aliases a slot a later G_VTX
     overwrote passes closure 1 and moves one corner onto another vertex's
     position -- a collapsed (invisible) or stretched triangle, which is what
     P2-3r17's owner reported as "a vertex placement issue".

  3. MATRIX-ROUTING closure. Every corner's baked GX palette slot must be the
     slot its own vertex's joint binding owns -- `NDS_NATIVE_GX_MATRIX_CURRENT`
     when the vertex belongs to the run's own root, that binding's stored slot
     otherwise, and a RAW run's corners must carry no slot bits at all because
     the raw emitters read the word unmasked as a dense id. A corner routed to
     the wrong joint lands wherever that joint is.

  4. FACING closure. Every triangle's geometric normal must agree with its own
     three vertex normals. Closure 5 only proves neighbouring triangles agree
     with EACH OTHER, which a wholly inside-out island passes while hardware
     back-face culling deletes it.

  5. WINDING closure. Every interior edge is traversed in opposite directions
     by its two triangles.

  6. PRIMITIVE closure. Expand every Task 56 primitive group back into oriented
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

REPO = _paths.REPO_ROOT

# Every owner the runtime can select, in both detail levels. Mario/Fox share one
# combined table set (P2-2's frozen program); P2-3 owners get independent ones.
OWNERS = ("mario", "fox", "luigi", "donkey", "captain", "samus")
DETAILS = ("high", "low")

DOBJ_DESC_SIZE = native.DOBJ_DESC_SIZE
VERTEX_CACHE_SIZE = native.VERTEX_CACHE_SIZE
SOURCE_VERTEX_SIZE = native.SOURCE_VERTEX_SIZE
G_VTX = 0x01
G_MODIFYVTX = 0x02
G_TRI1 = 0x05
G_TRI2 = 0x06
G_ENDDL = 0xDF
MAX_DL_COMMANDS = 4096

# The runtime's packed-corner encoding (src/nds/nds_renderer.c).
DENSE_ID_MASK = 0x3FF
PACKED_CORNER_MATRIX_SHIFT = 10
GX_MATRIX_CURRENT = 31
GX_MATRIX_SLOT_MAX = 30
RUN_RAW_CURRENT = 0
RUN_CROSS_MATRIX = 1

GL_TRIANGLES = 0
GL_TRIANGLE_STRIP = 2


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
    high_offset, high_count = native.OWNER_JOINT_TREES[owner]
    if high_count != count:
        raise ValueError(
            f"{owner}: High/Low JointTree cardinality {high_count}/{count}")
    out = []
    for i in range(count):
        depth, pointer = struct.unpack_from(
            ">II", payload, offset + i * DOBJ_DESC_SIZE)
        display = None if pointer == 0 else (pointer & 0xFFFF) * 4
        if detail == "low" and display is None:
            _high_depth, high_pointer = struct.unpack_from(
                ">II", payload, high_offset + i * DOBJ_DESC_SIZE)
            if high_pointer != 0:
                display = (high_pointer & 0xFFFF) * 4
        out.append((depth, display))
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
            "dense_vertices": dense_vertices,
            "cross_slots": owner_cross_slots[
                [name for name, _ in context["owner_roots"]].index(owner)],
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
        "dense_vertices": context["dense_vertices"],
        "cross_slots": context["topology"][3],
        "shared": False,
    }



def emitted_triangles(program: dict, root) -> list[tuple[int, int, int]]:
    """The dense-id triples one root's runs emit, in emit order."""
    out = []
    for epoch_index in range(root[1], root[1] + root[4]):
        epoch = program["epochs"][epoch_index]
        for run_index in range(epoch[3], epoch[3] + epoch[9]):
            base = program["run_first_corner"][run_index]
            for t in range(program["runs"][run_index][1]):
                out.append(tuple(
                    program["packed_corners"][base + t * 3 + k] & DENSE_ID_MASK
                    for k in range(3)))
    return out


def walk_root_cache(payload: bytes, offset: int, slots: list) -> list:
    """The same walk as walk_root_triangles, but carrying the real vertex
    cache: each corner comes back as (source_offset, s_override, t_override)
    so a G_MODIFYVTX is reproduced exactly as the hardware would see it.

    `slots` persists ACROSS roots on purpose -- the source cache does, and the
    generator's own dense walk relies on it (Mario's entry barrel reuses six
    slots the preceding rim loaded)."""
    out = []
    index = 0
    while True:
        if index >= MAX_DL_COMMANDS:
            raise ValueError(f"root 0x{offset:x}: runaway display list")
        w0, w1 = struct.unpack_from(">II", payload, offset + index * 8)
        op = w0 >> 24
        if op == G_ENDDL:
            return out
        if op == G_VTX:
            count = (w0 >> 12) & 0xFF
            end = (w0 >> 1) & 0x7F
            first = end - count
            source = (w1 & 0xFFFF) * 4
            for k in range(count):
                slots[first + k] = (source + k * SOURCE_VERTEX_SIZE, None, None)
        elif op == G_MODIFYVTX:
            slot = (w0 & 0xFFFF) >> 1
            s_value, t_value = struct.unpack(">hh", w1.to_bytes(4, "big"))
            slots[slot] = (slots[slot][0], s_value, t_value)
        elif op in (G_TRI1, G_TRI2):
            packed = [w0 & 0xFFFFFF]
            if op == G_TRI2:
                packed.append(w1 & 0xFFFFFF)
            for value in packed:
                out.append(tuple(
                    slots[((value >> shift) & 0xFF) // 2]
                    for shift in (16, 8, 0)))
        index += 1


def vertex_closure(owner: str, detail: str, program: dict) -> list[str]:
    payload = native.load_o2r_payload(REPO, owner)
    descriptors = joint_tree(payload, owner, detail)
    setup = native.OWNER_SETUP_PARTS[owner]
    offsets = [descriptors[i][1] for i in range(len(descriptors) - 1)
               if (setup[i // 32] & (1 << (31 - (i & 31)))) and
               descriptors[i][1] is not None]
    dense = program["dense_vertices"]
    slots = [None] * VERTEX_CACHE_SIZE
    failures: list[str] = []
    checked = 0
    for ordinal, (offset, root) in enumerate(zip(offsets, program["roots"])):
        source = walk_root_cache(payload, offset, slots)
        table = emitted_triangles(program, root)
        if len(source) != len(table):
            failures.append(
                f"{owner} {detail} root {ordinal}: {len(source)} source "
                f"triangles vs {len(table)} emitted")
            continue
        for index, (src_tri, dense_tri) in enumerate(zip(source, table)):
            for corner in range(3):
                source_offset, s_override, t_override = src_tri[corner]
                x, y, z, s, t, rgba = native.decode_source_vertex(
                    payload, source_offset)
                if s_override is not None:
                    s, t = s_override, t_override
                record = dense[dense_tri[corner]]
                checked += 1
                if (record[0], record[1], record[2]) != (x, y, z):
                    failures.append(
                        f"{owner} {detail} root {ordinal} triangle {index} "
                        f"corner {corner}: dense {dense_tri[corner]} is at "
                        f"{(record[0], record[1], record[2])} but the source "
                        f"cache held {(x, y, z)} (source 0x{source_offset:x})")
                elif (record[3], record[4]) != (s, t):
                    failures.append(
                        f"{owner} {detail} root {ordinal} triangle {index} "
                        f"corner {corner}: dense {dense_tri[corner]} texcoord "
                        f"{(record[3], record[4])} vs source {(s, t)}")
                elif record[7] != rgba:
                    failures.append(
                        f"{owner} {detail} root {ordinal} triangle {index} "
                        f"corner {corner}: dense {dense_tri[corner]} "
                        f"normal/colour 0x{record[7]:08x} vs source "
                        f"0x{rgba:08x}")
    print(f"  vertex closure: {checked} corners carry their own source "
          f"position, texcoord and normal")
    return failures


def matrix_routing_closure(owner: str, detail: str, program: dict) -> list[str]:
    dense = program["dense_vertices"]
    cross = program["cross_slots"]
    corners = program["packed_corners"]
    failures: list[str] = []
    checked = 0
    for ordinal, root in enumerate(program["roots"]):
        for epoch_index in range(root[1], root[1] + root[4]):
            epoch = program["epochs"][epoch_index]
            for run_index in range(epoch[3], epoch[3] + epoch[9]):
                _first, count, submit_class, _mask = program["runs"][run_index]
                base = program["run_first_corner"][run_index]
                for k in range(count * 3):
                    word = corners[base + k]
                    binding = dense[word & DENSE_ID_MASK][5]
                    slot = word >> PACKED_CORNER_MATRIX_SHIFT
                    checked += 1
                    if submit_class == RUN_RAW_CURRENT:
                        # The raw emitters read the word UNMASKED as a dense id.
                        if slot != 0:
                            failures.append(
                                f"{owner} {detail} run {run_index} corner {k}: "
                                f"raw corner carries slot bits {slot}")
                        if binding != ordinal:
                            failures.append(
                                f"{owner} {detail} run {run_index} corner {k}: "
                                f"raw corner binding {binding} is not root "
                                f"{ordinal}")
                        continue
                    want = (GX_MATRIX_CURRENT if binding == ordinal
                            else cross[binding])
                    if slot != want:
                        failures.append(
                            f"{owner} {detail} run {run_index} corner {k}: "
                            f"binding {binding} routed to GX slot {slot}, "
                            f"its own joint owns {want}")
                    elif (binding != ordinal) and (want > GX_MATRIX_SLOT_MAX):
                        failures.append(
                            f"{owner} {detail} run {run_index} corner {k}: "
                            f"binding {binding} has no stored GX slot")
    print(f"  matrix-routing closure: {checked} corners routed to their own "
          f"joint's GX palette slot")
    return failures


def _signed_normal_byte(value: int) -> int:
    return value - 256 if value >= 128 else value


def facing_closure(owner: str, detail: str, program: dict) -> list[str]:
    """A triangle whose geometric normal opposes its own vertex normals is
    inside-out. The winding closure below cannot see that: a wholly flipped
    island is internally consistent, and hardware back-face culling deletes it
    with no assert, no counter and no hang."""
    dense = program["dense_vertices"]
    corners = program["packed_corners"]
    failures: list[str] = []
    checked = 0
    for root in program["roots"]:
        for epoch_index in range(root[1], root[1] + root[4]):
            epoch = program["epochs"][epoch_index]
            for run_index in range(epoch[3], epoch[3] + epoch[9]):
                base = program["run_first_corner"][run_index]
                for t in range(program["runs"][run_index][1]):
                    ids = [corners[base + t * 3 + k] & DENSE_ID_MASK
                           for k in range(3)]
                    records = [dense[i] for i in ids]
                    if len({r[5] for r in records}) != 1:
                        # Cross-matrix: the three corners are in different
                        # joint spaces, so a model-space normal is meaningless.
                        continue
                    a, b, c = [(r[0], r[1], r[2]) for r in records]
                    ux, uy, uz = (b[0] - a[0], b[1] - a[1], b[2] - a[2])
                    wx, wy, wz = (c[0] - a[0], c[1] - a[1], c[2] - a[2])
                    nx = uy * wz - uz * wy
                    ny = uz * wx - ux * wz
                    nz = ux * wy - uy * wx
                    if (nx, ny, nz) == (0, 0, 0):
                        continue
                    sx = sy = sz = 0
                    for record in records:
                        sx += _signed_normal_byte((record[7] >> 24) & 0xFF)
                        sy += _signed_normal_byte((record[7] >> 16) & 0xFF)
                        sz += _signed_normal_byte((record[7] >> 8) & 0xFF)
                    checked += 1
                    if (nx * sx + ny * sy + nz * sz) < 0:
                        failures.append(
                            f"{owner} {detail} run {run_index} triangle {t} "
                            f"(dense {ids}) faces away from its own vertex "
                            "normals -- inside-out, back-face culled on "
                            "hardware")
    print(f"  facing closure: {checked} single-binding triangles face the way "
          f"their own vertex normals do")
    return failures


def _rotations(triangle):
    a, b, c = triangle
    return {(a, b, c), (b, c, a), (c, a, b)}


def _expand(group_type, vertices):
    """Oriented triangles one DS vertex list draws.

    DS strip rule: for strip vertices v0..v(n-1), triangle k is
    (v_k, v_k+1, v_k+2) for even k and (v_k+1, v_k, v_k+2) for odd k -- the
    engine flips every other triangle's winding so the strip keeps one facing.
    """
    out = []
    if group_type == GL_TRIANGLES:
        for i in range(0, len(vertices) - 2, 3):
            out.append((vertices[i], vertices[i + 1], vertices[i + 2]))
        return out
    for k in range(len(vertices) - 2):
        if (k & 1) == 0:
            out.append((vertices[k], vertices[k + 1], vertices[k + 2]))
        else:
            out.append((vertices[k + 1], vertices[k], vertices[k + 2]))
    return out


def _expand_run(groups):
    """Oriented triangles the RUNTIME draws for a whole run.

    Models `ndsRendererNativeEmitProductionPrimitiveGroups`, which does not
    issue a BEGIN_VTXS per group: consecutive GL_TRIANGLE groups share one
    vertex list, because separate triangles concatenate harmlessly. Everything
    else opens its own.

    Expanding each group INDEPENDENTLY instead is what let a shipped regression
    through on 2026-08-10: the emitter's original condition skipped the BEGIN
    between adjacent STRIPS too, welding them into one list, and the checker
    could not see it because it was validating the DATA under a policy the
    runtime did not follow. Keep the two in step.
    """
    out = []
    pending_type = None
    pending: list[int] = []
    for group_type, vertices in groups:
        if (pending_type == GL_TRIANGLES) and (group_type == GL_TRIANGLES):
            pending.extend(vertices)
            continue
        if pending_type is not None:
            out.extend(_expand(pending_type, pending))
        pending_type, pending = group_type, list(vertices)
    if pending_type is not None:
        out.extend(_expand(pending_type, pending))
    return out


def check_mode(mode, runs, packed_corners, run_first_corner):
    (group_first, group_count, group_type, group_first_vertex,
     group_vertex_count,
     primitive_vertices) = native.build_fighter_primitive_streams(
        runs, packed_corners, run_first_corner, mode)

    total_src = total_emit = total_verts = 0
    bad_winding = []
    bad_set = []
    for run_index in range(len(runs)):
        _first_tri, count, submit_class, _mask = runs[run_index]
        if count == 0:
            continue
        src = [tuple(v & DENSE_ID_MASK for v in t)
               for t in native._run_triangles(
                   runs, run_index, packed_corners, run_first_corner)]
        run_groups = []
        g0 = group_first[run_index]
        for g in range(g0, g0 + group_count[run_index]):
            v0 = group_first_vertex[g]
            vertices = [v & DENSE_ID_MASK for v in
                        primitive_vertices[v0:v0 + group_vertex_count[g]]]
            total_verts += len(vertices)
            run_groups.append((group_type[g], vertices))
        emitted = _expand_run(run_groups)

        total_src += len(src)
        total_emit += len(emitted)
        if len(emitted) != len(src):
            bad_set.append((run_index, submit_class, len(src), len(emitted)))
            continue
        pool: dict = {}
        for triangle in src:
            pool.setdefault(frozenset(triangle), []).append(triangle)
        for triangle in emitted:
            candidates = pool.get(frozenset(triangle))
            if not candidates:
                bad_set.append((run_index, submit_class, triangle, "absent"))
                continue
            match = next(
                (c for c in candidates if triangle in _rotations(c)), None)
            if match is None:
                bad_winding.append(
                    (run_index, submit_class, triangle, candidates[0]))
                candidates.pop()
            else:
                candidates.remove(match)

    return {
        "mode": mode,
        "groups": len(group_type),
        "vertices": total_verts,
        "src_triangles": total_src,
        "emitted_triangles": total_emit,
        "bad_winding": bad_winding,
        "bad_set": bad_set,
    }


def source_closure(owner: str, detail: str, program: dict) -> list[str]:
    payload = native.load_o2r_payload(REPO, owner)
    descriptors = joint_tree(payload, owner, detail)
    setup = native.OWNER_SETUP_PARTS[owner]
    selected = [i for i in range(len(descriptors) - 1)
                if setup[i // 32] & (1 << (31 - (i & 31)))]
    roots = [descriptors[i][1] for i in selected
             if descriptors[i][1] is not None]
    failures: list[str] = []

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
        result = check_mode(
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
            failures += vertex_closure(owner, detail, program)
            failures += matrix_routing_closure(owner, detail, program)
            failures += facing_closure(owner, detail, program)
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
          "every source triangle reaches the emitted primitive stream exactly "
          "once, carrying its own source vertex, routed to its own joint's GX "
          "slot, facing outward, with source winding, for every owner and "
          "detail")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
