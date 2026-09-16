#!/usr/bin/env python3
"""Shared helpers for the offline native-packet checks in this directory.

`compile_existing` was byte-identical in test_native_catch_swirl_packet.py,
test_native_ko_reflect_packets.py and test_native_shield_reflector_packets.py.
Three copies of the same root-offset arithmetic is three places to forget when a
fighter's root list changes, and the offsets are cumulative, so a stale copy
mis-bases every fighter after the one that moved.

These scripts are hand-run developer checks: nothing in the Makefile or under
scripts/*.ps1 invokes them.
"""

from __future__ import annotations


def compile_existing(gen, census, resources: "dict[int, object]"):
    """Compile the existing pinned corpus and return its per-fighter compilers.

    `gen` is the generate_nds_entry_effects module and `census` its O2R census
    module; both are passed in because each caller imports them under its own
    sys.path setup. Roots are compiled in a fixed order and each fighter is
    based at the cumulative root count of everything before it.
    """
    del census  # kept in the signature so callers read the same at every site

    order = (
        ("MARIO", "MARIO_ROOTS"),
        ("FOX", "FOX_ROOTS"),
        ("DONKEY", "DONKEY_ROOTS"),
        ("SAMUS", "SAMUS_ROOTS"),
        ("CAPTAIN", "CAPTAIN_ROOTS"),
        ("LINK_SPECIAL2", "LINK_SPECIAL2_ROOTS"),
        ("LINK_MODEL", "LINK_MODEL_SPIN_ROOTS"),
        ("LINK_SPECIAL3", "LINK_SPECIAL3_ROOTS"),
    )

    compilers = []
    base = 0
    for source_name, roots_name in order:
        source = getattr(gen, source_name)
        roots = getattr(gen, roots_name)
        compiler = gen.Compiler(resources[source.file_id], resources)
        compiler.compile_roots(roots, base)
        compilers.append(compiler)
        base += len(roots)
    return tuple(compilers)
