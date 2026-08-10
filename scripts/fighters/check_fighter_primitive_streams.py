#!/usr/bin/env python3
"""Prove the Task 56 fighter primitive streams draw the source triangles.

A stripifier is only correct if expanding its groups under the DS geometry
engine's own rule reproduces the source triangle set **with the source
winding**. Vertex-count reduction proves nothing about that, and a strip whose
first triangle is emitted with reversed winding is culled away silently -- the
fighter simply loses polygons, with no assert, no counter, and no hang.

So this expands every generated group back into oriented triangles and compares
them, per run, against the run's source triangles. Rotation-equivalent triples
match; a reversed triple does not.

DS strip rule: for strip vertices v0..v(n-1), triangle k is
(v_k, v_k+1, v_k+2) for even k and (v_k+1, v_k, v_k+2) for odd k -- the engine
flips every other triangle's winding so the whole strip keeps one facing.

Run with no arguments; exits non-zero on any mismatch.
"""
from __future__ import annotations

import sys as _sys
from pathlib import Path as _Path

_scripts_root = _Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in _sys.path:
    _sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402  -- puts every scripts/ area folder on sys.path

import generate_nds_native_owners as native  # noqa: E402

_run_triangles = native._run_triangles
build_fighter_primitive_streams = native.build_fighter_primitive_streams
build_owner_source_context = native.build_owner_source_context

GL_TRIANGLES = 0
GL_TRIANGLE_STRIP = 2


def rotations(tri):
    a, b, c = tri
    return {(a, b, c), (b, c, a), (c, a, b)}


def expand(gtype, verts):
    """Oriented triangles one DS vertex list draws."""
    out = []
    if gtype == GL_TRIANGLES:
        for i in range(0, len(verts) - 2, 3):
            out.append((verts[i], verts[i + 1], verts[i + 2]))
        return out
    for k in range(len(verts) - 2):
        if (k & 1) == 0:
            out.append((verts[k], verts[k + 1], verts[k + 2]))
        else:
            out.append((verts[k + 1], verts[k], verts[k + 2]))
    return out


def expand_run(groups):
    """Oriented triangles the RUNTIME draws for a whole run.

    This models `ndsRendererNativeEmitProductionPrimitiveGroups`, which does not
    issue a BEGIN_VTXS per group: consecutive GL_TRIANGLE groups share one
    vertex list, because separate triangles concatenate harmlessly. Everything
    else opens its own list.

    Expanding each group independently instead is what let a shipped regression
    through: the emitter's original condition skipped the BEGIN between ADJACENT
    STRIPS too, welding them into one list, and this checker could not see it
    because it was validating the DATA under a policy the runtime did not
    follow. `groups` is [(type, [dense ids])] in emit order.
    """
    out = []
    pending_type = None
    pending = []
    for gtype, verts in groups:
        if (pending_type == GL_TRIANGLES) and (gtype == GL_TRIANGLES):
            pending.extend(verts)
            continue
        if pending_type is not None:
            out.extend(expand(pending_type, pending))
        pending_type, pending = gtype, list(verts)
    if pending_type is not None:
        out.extend(expand(pending_type, pending))
    return out


def check_mode(mode, runs, packed_corners, run_first_corner):
    (group_first, group_count, group_type, group_first_vertex,
     group_vertex_count, primitive_vertices) = build_fighter_primitive_streams(
        runs, packed_corners, run_first_corner, mode)

    total_src = 0
    total_emit = 0
    total_verts = 0
    bad_winding = []
    bad_set = []
    for run_index in range(len(runs)):
        _first_tri, count, submit_class, _mask = runs[run_index]
        if count == 0:
            continue
        src = [tuple(v & 0x3FF for v in t)
               for t in _run_triangles(runs, run_index, packed_corners,
                                       run_first_corner)]
        run_groups = []
        g0 = group_first[run_index]
        for g in range(g0, g0 + group_count[run_index]):
            v0 = group_first_vertex[g]
            verts = [v & 0x3FF for v in
                     primitive_vertices[v0:v0 + group_vertex_count[g]]]
            total_verts += len(verts)
            run_groups.append((group_type[g], verts))
        emitted = expand_run(run_groups)

        total_src += len(src)
        total_emit += len(emitted)
        if len(emitted) != len(src):
            bad_set.append((run_index, submit_class, len(src), len(emitted)))
            continue
        # Pair them up as multisets of rotation-normalised triples, then check
        # each emitted triple against the source ORIENTATION, not just its
        # vertex set: a reversed triple has the same three vertices.
        pool = {}
        for t in src:
            pool.setdefault(frozenset(t), []).append(t)
        for t in emitted:
            key = frozenset(t)
            cands = pool.get(key)
            if not cands:
                bad_set.append((run_index, submit_class, t, "absent"))
                continue
            match = next((c for c in cands if t in rotations(c)), None)
            if match is None:
                bad_winding.append((run_index, submit_class, t, cands[0]))
                cands.pop()
            else:
                cands.remove(match)

    return {
        "mode": mode,
        "groups": len(group_type),
        "vertices": total_verts,
        "src_triangles": total_src,
        "emitted_triangles": total_emit,
        "bad_winding": bad_winding,
        "bad_set": bad_set,
    }


def main():
    context = build_owner_source_context(_paths.REPO_ROOT)
    runs = context["runs"]
    vertex = context["vertex"]
    triangles = context["triangles"]
    epochs = context["epochs"]
    owner_roots = context["owner_roots"]
    owner_topologies = context["owner_topologies"]
    build_dense_geometry = native.build_dense_geometry
    build_direct_dense_tables = native.build_direct_dense_tables

    (dense_vertices, dense_color_sources, dense_owners, dense_corners,
     action_dense_first, run_first_corner, run_owners, run_root_bindings,
     run_binding_sets) = build_dense_geometry(
        vertex, triangles, runs, epochs, owner_roots, _paths.REPO_ROOT)
    owner_cross_slots = [topology[3] for topology in owner_topologies]
    (_action_dense_spans, packed_corners, _rfu, _ruc,
     _rud) = build_direct_dense_tables(
        vertex, runs, dense_vertices, dense_color_sources, dense_corners,
        action_dense_first, run_first_corner, run_owners,
        run_root_bindings, run_binding_sets, owner_cross_slots)

    failed = False
    for mode in (1, 2):
        r = check_mode(mode, runs, packed_corners, run_first_corner)
        raw = f"{r['vertices']:,} vertex submissions in {r['groups']:,} groups"
        print(f"mode {r['mode']}: {raw}, "
              f"{r['emitted_triangles']:,} triangles drawn against "
              f"{r['src_triangles']:,} source")
        if r["bad_set"]:
            failed = True
            print(f"  MISSING/EXTRA: {len(r['bad_set'])}")
            for row in r["bad_set"][:8]:
                print(f"    {row}")
        if r["bad_winding"]:
            failed = True
            print(f"  REVERSED WINDING: {len(r['bad_winding'])} triangles "
                  f"({100.0 * len(r['bad_winding']) / max(r['emitted_triangles'], 1):.1f}%)"
                  " -- these are culled away on hardware")
            for row in r["bad_winding"][:8]:
                print(f"    run {row[0]} class {row[1]}: emitted {row[2]} "
                      f"against source {row[3]}")
        if not r["bad_set"] and not r["bad_winding"]:
            print("  OK: every source triangle drawn once, with source winding")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
