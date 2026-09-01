#!/usr/bin/env python3
"""Report exact-local cross-binding weld candidates for the visible seam bugs.

This is deliberately an analysis/checker, not a renderer mutation.  A candidate
is a dense vertex position used by one owner's emitted source triangles through
two or more logical joint bindings.  The source position and DS VERTEX16 words
must be bit-identical before the GX matrix palette is applied; if they are, any
visible split is downstream of asset decoding / local-coordinate packing.

The current production fighter path leaves joint transformation to GX.  There is
therefore no CPU post-transform `prepared_dense` position that can be copied from
one binding to another.  Without a concrete animation pose, two distinct joint
matrices have no useful finite position-difference bound: the intended joint
motion dominates fixed-point quantisation.  This checker says that explicitly
rather than manufacturing a misleading one-LSB bound.
"""
from __future__ import annotations

import sys
from collections import defaultdict
from pathlib import Path

_scripts_root = Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in sys.path:
    sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402

import generate_nds_native_owners as native  # noqa: E402
from check_native_owner_geometry_closure import (  # noqa: E402
    DENSE_ID_MASK,
    owner_program,
)

REPO = _paths.REPO_ROOT
TARGETS = (("mario", "high"), ("donkey", "high"))


def referenced_dense_ids(program: dict) -> set[int]:
    out: set[int] = set()
    corners = program["packed_corners"]
    for root in program["roots"]:
        for epoch_index in range(root[1], root[1] + root[4]):
            epoch = program["epochs"][epoch_index]
            for run_index in range(epoch[3], epoch[3] + epoch[9]):
                first = program["run_first_corner"][run_index]
                count = program["runs"][run_index][1] * 3
                for word in corners[first:first + count]:
                    out.add(word & DENSE_ID_MASK)
    return out


def cross_binding_groups(program: dict):
    dense = program["dense_vertices"]
    groups: dict[tuple[int, int, int], list[int]] = defaultdict(list)
    for dense_id in sorted(referenced_dense_ids(program)):
        row = dense[dense_id]
        groups[(row[0], row[1], row[2])].append(dense_id)

    result = []
    for xyz, dense_ids in sorted(groups.items()):
        bindings = {dense[dense_id][5] for dense_id in dense_ids}
        if len(bindings) < 2:
            continue
        # One representative per binding is enough to describe the logical
        # cross-joint weld. UV/normal duplicates within the same binding already
        # receive the exact same GX matrix and local VERTEX16 position.
        representative: dict[int, int] = {}
        for dense_id in dense_ids:
            representative.setdefault(dense[dense_id][5], dense_id)
        canonical_binding = min(representative)
        canonical = representative[canonical_binding]
        aliases = [
            representative[binding]
            for binding in sorted(representative)
            if binding != canonical_binding
        ]
        gx = native.pack_fifo_vertex16(*xyz, "weld consistency")
        result.append((xyz, canonical, aliases, representative, gx))
    return result


def binding_metadata(owner: str, detail: str):
    """Return logical binding -> (joint index, representative root offset)."""
    if owner in ("mario", "fox"):
        context = native.build_owner_source_context(REPO, detail)
        names = [name for name, _roots in context["owner_roots"]]
        owner_index = names.index(owner)
        roots = dict(context["owner_roots"])[owner]
        topology = context["owner_topologies"][owner_index]
        root_bindings = list(range(len(roots)))
    else:
        context = native.build_p2_owner_runtime_context(REPO, owner, detail)
        roots = context["roots"]
        topology = context["topology"]
        root_bindings = context["root_bindings"]
    binding_joints = topology[2]
    out = {}
    for ordinal, (root, binding) in enumerate(zip(roots, root_bindings)):
        out.setdefault(binding, (binding_joints[binding], root[0], ordinal))
    return out


def main() -> int:
    total = 0
    for owner, detail in TARGETS:
        program = owner_program(owner, detail)
        dense = program["dense_vertices"]
        metadata = binding_metadata(owner, detail)
        groups = cross_binding_groups(program)
        total += len(groups)
        print(f"{owner} {detail}: {len(groups)} cross-binding exact-local groups")
        for xyz, canonical, aliases, representatives, gx in groups:
            pair_text = ", ".join(
                f"({canonical},{alias})" for alias in aliases
            )
            binding_text = ", ".join(
                f"b{binding}/j{metadata[binding][0]}/"
                f"root0x{metadata[binding][1]:x}->d{dense_id}"
                for binding, dense_id in sorted(representatives.items())
            )
            print(
                f"  xyz={xyz} gx_xy=0x{gx[0]:08x} gx_z=0x{gx[1]:04x} "
                f"bindings=[{binding_text}] weld_pairs=[{pair_text}]"
            )
            # Every representative shares the exact source-local xyz by
            # construction. Pin the generator's DS packing too: no bake-time
            # rounding or per-binding local quantisation is allowed to differ.
            for dense_id in representatives.values():
                row = dense[dense_id]
                if native.pack_fifo_vertex16(
                    row[0], row[1], row[2], "weld consistency"
                ) != gx:
                    raise ValueError(
                        f"{owner} {detail} dense {dense_id}: identical local xyz "
                        "did not produce identical VERTEX16 words"
                    )
        print(
            "  local_v16_split=0; post_matrix_bound=UNBOUNDED_WITHOUT_POSE "
            "(production GX applies distinct joint matrices in hardware)"
        )
    print(f"NATIVE_OWNER_WELD_CANDIDATES_OK groups={total}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
