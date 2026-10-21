#!/usr/bin/env python3
"""P7: the enabled contents' dead article geometry, as C rows.

A content's effects and weapons draw natively from the entry-effect packet
(scripts/3d_vfx/generate_nds_entry_effects.py --p4), which bakes their
geometry and texels: the runtime admits a list by comparing a DObj's
display-list pointer with `*storage + offset` and never reads the list, its
vertices or the images it loads. Those bytes still sit in the content's
special files, so a VS battle lends them as allocation space once the files
load (battleship_ftmanager.c ndsP4LendArticleDeadSpans), as the stages lend
theirs (scripts/stages/generate_stage_dead_spans.py).

A file's range is the display lists, vertex arrays and loaded images its
article roots (articles.json, every enabled content's, unioned: contents
share files) reach, less what a model-part sidecar reaches (the fighter
renderer's). Only a content's own files (ids past the vanilla 0x852) are
lent. A range stays live -- left out -- when a pointer outside the ranges,
in the file or in any staged file, names it other than at a root: a root is
only compared, but a display-list call outside the ranges would read its
target, and an MObj's image and palette tables are what a live material's
draw reads (the TLUT a material root takes). An external relocation's 16
bytes stay (a retried external pass walks that chain). A walk that meets an
opcode F3DEX2 does not have lends nothing of that file.

    p4_dead_spans.py --gen-root <gen> --contents "a b ..." --out <header>

Offsets into the user's files: build output only.
"""
from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import p4_native_owner as P  # noqa: E402

MIN_BYTES = 256
FILE_SPANS = 16  # battleship_ftmanager.c NDS_P4_DEAD_SPAN_FILE_SPANS
VANILLA_LAST_FILE = 0x852


class WalkError(Exception):
    pass


def reach(f: "P.O2RFile", roots) -> tuple[list, list, list]:
    """(display-list blocks, vertex ranges, image ranges) the roots reach."""
    data = f.data
    blocks, vertices, images, seen = [], [], [], set()

    def target(slot):
        ref = f.slots.get(slot)
        return ref[2] if (ref is not None and ref[0] == "intern") else None

    def walk(at):
        if at in seen:
            return
        seen.add(at)
        start, img = at, None
        while True:
            if at + 8 > len(data):
                raise WalkError(f"display list at 0x{start:x} runs off the file")
            w0, w1 = struct.unpack_from(">II", data, at)
            op = w0 >> 24
            if 0x09 <= op <= 0xD2:
                raise WalkError(f"opcode 0x{op:02x} at 0x{at:x}")
            if op == 0x01:
                t = target(at + 4)
                if t is None:
                    raise WalkError(f"G_VTX at 0x{at:x} without an internal address")
                vertices.append((t, t + ((w0 >> 12) & 0xFF) * 16))
            elif op == 0xFD:
                img = (target(at + 4), (w0 >> 19) & 3, (w0 & 0xFFF) + 1)
            elif op in (0xF3, 0xF4, 0xF0) and img is not None and img[0] is not None:
                t, siz, width = img
                if op == 0xF3:
                    n = ((((w1 >> 12) & 0xFFF) + 1) * (4 << siz) + 7) // 8
                elif op == 0xF4:
                    n = ((((w1 & 0xFFF) >> 2) + 1) * width * (4 << siz) + 7) // 8
                else:
                    n = (((w1 >> 14) & 0x3FF) + 1) * 2
                images.append((t, t + n))
            elif op == 0xDE:
                t = target(at + 4)
                if t is not None:
                    walk(t)
                elif at + 4 in f.slots:
                    raise WalkError(f"G_DL at 0x{at:x} into another file")
                if (w0 >> 16) & 0xFF == 1:
                    at += 8
                    break
            elif op == 0xDF:
                at += 8
                break
            at += 8
        blocks.append((start, at))

    for r in sorted(roots):
        walk(r)
    return blocks, vertices, images


def merge(ranges):
    out = []
    for a, b in sorted(ranges):
        if out and a <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], b))
        else:
            out.append((a, b))
    return out


def subtract(ranges, holes):
    out = []
    for a, b in ranges:
        pieces = [(a, b)]
        for x, y in holes:
            pieces = [p for s, e in pieces for p in ((s, min(e, x)), (max(s, y), e)) if p[0] < p[1]]
        out += pieces
    return out


def inside(x, ranges) -> bool:
    return any(a <= x < b for a, b in ranges)


def file_rows(f: "P.O2RFile", roots: set, live_roots: set, foreign_refs) -> list:
    """[(offset, bytes)] of the file's lendable ranges."""
    try:
        blocks, vertices, images = reach(f, roots)
        l_blocks, l_vertices, l_images = reach(f, live_roots)
    except WalkError as exc:
        print(f"P4_DEAD_SPANS {f.file_id:#06x}: {exc}, skipped")
        return []
    dead = subtract(merge(blocks + vertices + images),
                    merge(l_blocks + l_vertices + l_images))
    # An external slot keeps its 16 bytes: a retried external pass reads and
    # writes only the chain's own words.
    granules = sorted({(s & ~15, (s & ~15) + 16) for s, v in f.slots.items()
                       if v[0] == "extern" and inside(s, dead)})
    rows = []
    for a, b in subtract(dead, granules):
        bad = False
        for s, v in f.slots.items():
            if v[0] != "intern" or not (a <= v[2] < b) or inside(s, dead):
                continue
            # A display-list call outside the ranges would read the target;
            # any other pointer to a root (a DObjDesc's list) is compared.
            if v[2] not in roots or (s >= 4 and (f.u32(s - 4) >> 24) == 0xDE):
                bad = True
                break
        if bad or any(a <= t < b and t not in roots for t in foreign_refs):
            continue
        a, b = (a + 15) & ~15, b & ~15
        if b - a >= MIN_BYTES:
            rows.append((a, b - a))
    # The runtime takes a file's first FILE_SPANS ranges: keep the largest.
    return sorted(sorted(rows, key=lambda r: -r[1])[:FILE_SPANS])


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--gen-root", type=Path, required=True)
    ap.add_argument("--contents", required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    roots: dict[int, set] = {}
    live: dict[int, set] = {}
    paths: dict[int, Path] = {}
    for name in args.contents.split():
        art = args.gen_root / name / "articles.json"
        if not art.is_file():
            continue
        for r in json.loads(art.read_text(encoding="utf-8"))["roots"]:
            fid = r["file_id"]
            path = args.gen_root / name / "o2r" / f"{fid:04x}"
            if fid <= VANILLA_LAST_FILE or not path.is_file():
                continue
            paths.setdefault(fid, path)
            if r["storage"] is None:
                live.setdefault(fid, set()).add(r["offset"])
            elif r.get("via", 0) == 0 and not r.get("via_base"):
                roots.setdefault(fid, set()).add(r["offset"])
    # Every staged file's external pointers into a candidate file.
    refs: dict[int, set] = {fid: set() for fid in roots}
    seen = set()
    for path in sorted(args.gen_root.glob("*/o2r/*")):
        if path.name in seen:
            continue
        seen.add(path.name)
        other = P.O2RFile(path)
        for kind, fid, target in other.slots.values():
            if kind == "extern" and fid in refs:
                refs[fid].add(target)
    rows = []
    for fid in sorted(roots):
        f = P.O2RFile(paths[fid])
        rows += [(fid, len(f.data), a, n)
                 for a, n in file_rows(f, roots[fid], live.get(fid, set()), refs[fid])]
    lines = ["/* Generated by scripts/p4/p4_dead_spans.py. Do not edit. */",
             "#ifndef NDS_P4_DEAD_SPANS_GENERATED_H",
             "#define NDS_P4_DEAD_SPANS_GENERATED_H",
             "",
             "/* X(file, file bytes, offset, bytes): a content's file's article display",
             " * lists and vertices, which only the entry-effect packet draws; a VS",
             " * battle lends them as allocation space once the file loads. */",
             "#define NDS_P4_DEAD_SPAN_ROWS(X) \\"]
    lines += [f"    X({fid:#x}u, {size}u, {a:#x}u, {n}u) \\" for fid, size, a, n in rows]
    lines += ["    /* end */", "", "#endif", ""]
    text = "\n".join(lines)
    if not args.out.exists() or args.out.read_text(encoding="utf-8") != text:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(text, encoding="utf-8", newline="\n")
    per_file = {}
    for fid, _, _, n in rows:
        per_file[fid] = per_file.get(fid, 0) + n
    print(f"P4_DEAD_SPANS rows={len(rows)} files={len(per_file)} bytes={sum(per_file.values())} "
          + " ".join(f"{fid:#06x}:{n}" for fid, n in sorted(per_file.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
