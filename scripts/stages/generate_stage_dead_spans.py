#!/usr/bin/env python3
"""The VS stages' dead geometry bytes, as C rows.

A native stage's geometry file still carries the display lists and vertex
arrays its packet was generated from. Once the packet loads nothing reads
them: the owner draws the packet, and a layer DObj's display-list pointer is
only compared by the binding check. A VS battle lends them as allocation
space (reloc_backend_compat_shims.c ndsNativeStageLendDeadSpans), as
scripts/p5/stage_generate.py dead_spans does for the Remix stages.

The ranges are what the layer owners' DObjs reach (the grDisplayLayer
callbacks: the runtime skips a lent layer's source draw), less anything an
actor owner reaches, merged, shrunk to 16-byte bounds. Any other pointer
into them -- from the file itself or from another file of the stage --
falsifies, as does an image load inside one.

    python scripts/stages/generate_stage_dead_spans.py --output <header>
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent))
import generate_nds_native_stage as gen  # noqa: E402
from native_stage_descriptors import get_descriptor  # noqa: E402

# The VS stages with a blob packet, by gr/grdef.h kind.
STAGES = (("castle", 0), ("sector", 1), ("jungle", 2), ("zebes", 3),
          ("hyrule", 4), ("yoster", 5), ("yamabuki", 7), ("inishie", 8))
MIN_BYTES = 512


class DeadSpanError(SystemExit):
    pass


def load(spec: dict) -> "gen.O2RResource":
    return gen.load_o2r(gen._paths.REPO_ROOT, gen.InputSpec(spec["path"], spec["sha256"], spec["file_id"]))


def be32(data: bytes, at: int) -> int:
    return struct.unpack_from(">I", data, at)[0]


def reach(res, roots, slots):
    """(display-list blocks, vertex ranges, image ranges) the roots reach."""
    data = res.payload
    blocks, vertices, images, seen = [], [], [], set()

    def walk(at):
        if at in seen:
            return
        seen.add(at)
        start, img = at, None
        while True:
            if at + 8 > len(data):
                raise DeadSpanError(f"file {res.file_id}: display list at 0x{start:x} runs off the file")
            w0, w1 = struct.unpack_from(">II", data, at)
            op = w0 >> 24
            if op == 0x01:
                if at + 4 not in slots:
                    raise DeadSpanError(f"file {res.file_id}: G_VTX at 0x{at:x} without an address")
                vertices.append((slots[at + 4], slots[at + 4] + ((w0 >> 12) & 0xFF) * 16))
            elif op == 0xFD:
                img = (slots.get(at + 4), (w0 >> 19) & 3, (w0 & 0xFFF) + 1)
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
                if at + 4 in slots:
                    walk(slots[at + 4])
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


def owner_roots(res, slots, offset, count, dl_links, identity):
    roots = set()
    for i in range(count):
        slot = offset + i * 0x2C + 4
        if slot not in slots:
            continue
        identity.add(slot)
        if not dl_links:
            roots.add(slots[slot])
            continue
        at = slots[slot]
        while be32(res.payload, at) != 4:
            if at + 4 in slots:
                identity.add(at + 4)
                roots.add(slots[at + 4])
            at += 8
    return roots


def stage_rows(name: str, gkind: int):
    desc = get_descriptor(name)
    inputs = desc.o2r_inputs
    if "stage_geometry" not in inputs:
        return []
    geo = load(inputs["stage_geometry"])
    slots = {slot: ref.offset for slot, ref in geo.internal.items()}
    identity = set()
    layer_roots, actor_roots = set(), set()
    for spec in desc.owner_specs:
        key, offset, count, callback = spec[2], spec[3], spec[4], spec[6]
        dl_links = bool(spec[7]) if len(spec) > 7 else False
        if key != "stage_geometry":
            continue
        roots = owner_roots(geo, slots, offset, count, dl_links, identity)
        if callback.startswith("grDisplayLayer"):
            layer_roots |= roots
        else:
            actor_roots |= roots
    blocks, vertices, images = reach(geo, layer_roots, slots)
    a_blocks, a_vertices, a_images = reach(geo, actor_roots, slots)
    dead = subtract(merge(blocks + vertices), merge(a_blocks + a_vertices + a_images))
    if geo.external:
        # Its extern chain runs through its display lists, and a failed
        # external pass retries later: nothing of such a file is lent.
        print(f"STAGE_DEAD_SPANS {name}: file {geo.file_id} has external relocations, skipped")
        return []
    for slot, target in slots.items():
        inside = any(a <= target < b for a, b in dead)
        if inside and not any(a <= slot < b for a, b in dead) and slot not in identity:
            raise DeadSpanError(f"{name}: {geo.file_id}+0x{slot:x} points into a dead range")
    for a, b in images:
        if any(a < y and x < b for x, y in dead):
            raise DeadSpanError(f"{name}: an image load at 0x{a:x} overlaps a dead range")
    for key, spec in inputs.items():
        if key == "stage_geometry":
            continue
        other = load(spec)
        for ref in other.external.values():
            if ref.asset_id == geo.file_id and any(a <= ref.offset < b for a, b in dead):
                raise DeadSpanError(f"{name}: file {other.file_id} points into a dead range of {geo.file_id}")
    rows = []
    for a, b in dead:
        a, b = (a + 15) & ~15, b & ~15
        if b - a >= MIN_BYTES:
            rows.append((gkind, geo.file_id, len(geo.payload), a, b - a))
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--output", type=Path, required=True)
    args = ap.parse_args()
    lines = ["/* Generated by scripts/stages/generate_stage_dead_spans.py. Do not edit. */",
             "#ifndef NDS_STAGE_DEAD_SPANS_GENERATED_H",
             "#define NDS_STAGE_DEAD_SPANS_GENERATED_H",
             "",
             "/* X(gkind, file, file bytes, offset, bytes): a VS stage geometry file's",
             " * dead display-list and vertex bytes, lent to the battle once the stage's",
             " * packet loads. */",
             "#define NDS_STAGE_DEAD_SPAN_ROWS(X) \\"]
    total = 0
    for name, gkind in STAGES:
        for row in stage_rows(name, gkind):
            lines.append(f"    X({row[0]}, {row[1]:#x}u, {row[2]}u, {row[3]:#x}u, {row[4]}u) \\")
            total += row[4]
    lines += ["    /* end */", "", "#endif", ""]
    text = "\n".join(lines)
    if not args.output.exists() or args.output.read_text(encoding="utf-8") != text:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8", newline="\n")
    print(f"STAGE_DEAD_SPANS bytes={total}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
