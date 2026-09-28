#!/usr/bin/env python3
"""Lane 2: ORIGINAL label offsets for file 86 (MiscData086).

scripts/items/item_memory_closure.py writes map.json `label_offsets` as the
NEW (compact-layout) offsets, so it cannot be used to partition the original
file.  This module re-runs that script's placement stage verbatim (lines
370-564 of scripts/items/item_memory_closure.py: extern anchors, embedded
offsets, sub/post parents, DObjDesc terminator scans, intern-slot
propagation) and returns the ORIGINAL offsets.  It is a slice of that
script's own text, not a re-implementation, so both agree by construction; the
counts printed by __main__ are compared with the closure report (390 labels,
402 edges, 0 unknown, 402/402 pointer check).
"""
from __future__ import annotations

import re
import struct
import sys
from pathlib import Path

import lane2_o2r as o2r

sys.path.insert(0, str(o2r.REPO / "scripts" / "items"))
from item_memory_closure import *  # noqa: F401,F403
import item_memory_closure as _ic  # noqa: E402


def placement():
    failures = []
    notes = []
    # 1. O2R headers: the real reader logic, real binaries.
    size251, ext251, ids251, _ = parse_o2r(O2R_251, 0xFB)
    size86, ext86, ids86, payload86 = parse_o2r(O2R_86, 0x56)
    measured_tree = size251 + size86
    header_ok = (
        size251 == FILE_251_SIZE
        and size86 == FILE_86_SIZE
        and ext251 == 68
        and ext86 == 0
        and all(v == 86 for v in ids251)
        and measured_tree == TREE_SIZE
    )
    if not header_ok:
        failures.append(
            f"O2R header drift: 251={{size={size251},ext={ext251}}} "
            f"86={{size={size86},ext={ext86}}} tree={measured_tree}"
        )

    # 2. 251 payload census from the real source rows.
    text251 = read_text(SRC_251)
    n_attr = len(re.findall(r"ITAttributes\s+dITCommonData_\w+\[1\]", text251))
    n_wpattr = len(re.findall(r"WPAttributes\s+dITCommonData_\w+\s*=", text251))
    atk_rows = re.findall(r"ITAttackEvent\s+dITCommonData_\w+\[(\d+)\]", text251)
    f32_rows = re.findall(r"f32\s+dITCommonData_\w+\[(\d+)\]", text251)
    # ROM footprints: ITAttributes 0x48 (link_core.c audited layout),
    # WPAttributes 52 (16 ptr/vec/map words + 4 bitfield words, wptypes.h),
    # ITAttackEvent 8 (8+10+8+16 bits, ittypes.h), f32 4.
    census = n_attr * 72 + n_wpattr * 52 + sum(int(n) for n in atk_rows) * 8 + sum(
        int(n) for n in f32_rows
    ) * 4
    census_delta = FILE_251_SIZE - census
    rows251 = parse_251_initializers(text251)

    # 3. Extern anchors: 251 label -> file-0x56 label, no name guessing.
    externs = parse_251_reloc(read_text(RELOC_251))
    anchor: dict[str, int] = {}  # 86-label -> offset
    anchor_src: dict[str, str] = {}
    unanchored: list[str] = []
    for label251, field, off86 in externs:
        row = rows251.get(label251)
        if row is None or field >= len(row):
            unanchored.append(f"{label251}+{field * 4:#x}")
            continue
        target = row[field]
        if target is None:
            continue  # NULL pointer field: nothing to anchor.
        if target in anchor and anchor[target] != off86:
            failures.append(
                f"anchor conflict: {target} at {anchor[target]:#x} vs {off86:#x}"
            )
        anchor[target] = off86
        anchor_src[target] = f"{label251}+{field * 4:#x}"

    # 4. Label graph over file 0x56.
    edges = parse_86_reloc(read_text(RELOC_86))
    decls = parse_86_decl_types(read_text(SRC_86))
    labels: set[str] = set(anchor)
    for s, _, d, _ in edges:
        labels.add(s)
        labels.add(d)
    offsets: dict[str, int] = {}
    for lab in labels:
        off = embedded_offset(lab)
        if off is not None:
            offsets[lab] = off
    for lab, off in anchor.items():
        if lab in offsets and offsets[lab] != off:
            failures.append(
                f"label offset conflict: {lab} embedded {offsets[lab]:#x} "
                f"vs anchored {off:#x}"
            )
        offsets.setdefault(lab, off)
    unknown = sorted(lab for lab in labels if lab not in offsets)
    # Remaining labels resolve by three more existing-metadata rules, to a
    # fixpoint: (a) _sub_/_post_ suffixes against a placed parent prefix;
    # (b) a kind's _data_remainder from the DObjDesc id==18 terminator scan
    # (splitter's own typeITCommonObject.py algorithm); (c) payload intern
    # slots, which are big-endian (chain_link:16, words_num:16) with target
    # words_num*4 (decomp lb/lbreloc.c lbRelocLoadAndRelocFile) in the
    # pre-relocation image.
    data_anchors = sorted(
        (off, lab) for lab, off in anchor.items() if lab.endswith("_DObjDesc")
    )
    scanned: set[str] = set()
    scan_end: dict[str, int] = {}  # DObjDesc label -> tree end (for trims).
    both_known = both_match = 0
    changed = True
    while changed:
        changed = False
        for lab in labels:
            if lab in offsets:
                continue
            rel = parent_relative_offset(lab, offsets)
            if rel is not None and 0 <= rel < FILE_86_SIZE:
                offsets[lab] = rel
                changed = True
        for _off, dlab in data_anchors:
            if dlab in scanned:
                continue
            scanned.add(dlab)
            scan_off, scan_err = scan_remainder_offset(
                payload86, anchor[dlab], dlab
            )
            if scan_err is not None:
                failures.append(scan_err)
                continue
            scan_end[dlab] = scan_off
            # Gap-named remainder parents carry their own embedded offsets;
            # the terminator scan must agree with them exactly.
            stem = dlab[: -len("_DObjDesc")]
            gap_pat = re.compile(
                re.escape(stem) + r"_remainder_gap_0x[0-9A-Fa-f]+$"
            )
            for lab in labels:
                if gap_pat.match(lab) and lab in offsets \
                        and offsets[lab] != scan_off:
                    failures.append(
                        f"remainder scan cross-check: {lab} embedded "
                        f"{offsets[lab]:#x} vs scanned {scan_off:#x}"
                    )
            _t, _o, err = scan_dobj_remainder(
                payload86, anchor[dlab], dlab, labels
            )
            if err is not None:
                failures.append(err)
                continue
            if _t is None:
                continue
            if _t in offsets and offsets[_t] != _o:
                failures.append(
                    f"remainder scan conflict: {_t} embedded "
                    f"{offsets[_t]:#x} vs scanned {_o:#x}"
                )
                continue
            if _t not in offsets:
                offsets[_t] = _o
                changed = True
        for s, so, d, do in edges:
            if s not in offsets or d in offsets:
                continue
            slot = offsets[s] + so
            if slot + 4 > len(payload86):
                continue
            (words,) = struct.unpack_from(">I", payload86, slot)
            cand = (words & 0xFFFF) * 4 - do
            if 0 <= cand < FILE_86_SIZE and cand % 4 == 0:
                offsets[d] = cand
                changed = True
    # Validation pass over fully placed edges (count once). A dst +off
    # aims inside the target array, so the slot must read base + off.
    mismatch: list[str] = []
    for s, so, d, do in edges:
        if s in offsets and d in offsets:
            slot = offsets[s] + so
            if slot + 4 <= len(payload86):
                (words,) = struct.unpack_from(">I", payload86, slot)
                both_known += 1
                if (words & 0xFFFF) * 4 == offsets[d] + do:
                    both_match += 1
                elif len(mismatch) < 8:
                    mismatch.append(
                        f"{s}+{so:#x} -> {d}+{do:#x} "
                        f"(slot reads {(words & 0xFFFF) * 4:#x})"
                    )
    notes.append(
        f"payload pointer check: {both_match}/{both_known} intern slots read "
        "as (BE32&0xFFFF)*4 == target offset"
    )
    for line in mismatch:
        notes.append(f"  slot mismatch: {line}")
    if both_known and both_match / both_known < 0.90:
        failures.append(
            f"payload pointer model guard: match rate "
            f"{both_match}/{both_known} below 0.90"
        )
    unknown = sorted(lab for lab in labels if lab not in offsets)
    if unknown:
        failures.append(
            f"{len(unknown)} labels unplaceable, cannot rewrite relocation: "
            + ", ".join(unknown[:12])
            + ("..." if len(unknown) > 12 else "")
        )
    # Splitter-declared DObjDesc counts must equal the terminator scans;
    # both derive from the same trees, so any disagreement is a model bug.
    for _dlab, _n in parse_86_def_counts(read_text(SRC_86)).items():
        if _dlab in anchor and _dlab in scan_end:
            _scanned = (scan_end[_dlab] - anchor[_dlab]) // 44
            if _scanned != _n:
                failures.append(
                    f"DObjDesc count conflict: {_dlab} declares {_n} "
                    f"but scans {_scanned}"
                )
            else:
                notes.append(f"DObjDesc scan verified: {_dlab} {_n} entries")

    return {"offsets": offsets, "anchor": anchor, "edges": edges, "decls": decls,
            "labels": labels, "scan_end": scan_end, "failures": failures,
            "notes": notes, "payload86": payload86,
            "unknown": sorted(l for l in labels if l not in offsets)}


if __name__ == "__main__":
    r = placement()
    print("labels", len(r["labels"]), "edges", len(r["edges"]), "unknown", len(r["unknown"]),
          "failures", r["failures"])
    for n in r["notes"][:3]:
        print(" ", n)
