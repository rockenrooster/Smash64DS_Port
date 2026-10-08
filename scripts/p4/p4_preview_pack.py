#!/usr/bin/env python3
"""P4 character-select preview pack: a donor fighter's FPC1 pack.

The original cast's packs (scripts/fighters/generate_preview_core_packs.py)
are compiled from decomp metadata. A donor has only its main and model
files, so this builds the same container from their bytes and relocation
chains: Main is copied whole; Model keeps every byte except geometry -- the
display lists reachable from both JointTrees, the model parts and the access
part, and the vertices they load -- because the native owner image draws the
fighter. Each pointer to a pruned display list becomes a root cell (ENDDL plus
its original offset, the identity the native owner looks up), a pointer to
another file becomes NULL (the motion file and the parent's special and
shield files: nothing on the character select reads them through Main), and
any other pointer into pruned bytes is a failure.

A donor joint whose list is only G_ENDDL (its geometry rides in a
neighbouring joint's list; Falco's joints 11 and 29) has no owner root. Its
cell keeps the source bytes, ENDDL plus 0, which is what the fighter display
contract skips for a P4 fighter (ndsFighterDisplayContractSelectDL); every
other cell carries a nonzero original offset.

The pack kind is NDS_P4_SEL_BASE + content (src/port/reloc_preview_pack.c),
written to fighters/preview/<kind>.fpc; the layout follows
include/nds/nds_preview_pack.h.
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
import ft_layout  # noqa: E402

MAGIC = 0x31435046  # FPC1
VERSION = 2
NULL = 0xFFFFFFFF
ENDDL = 0xDF000000
ALIGN = 16
SEL_BASE = 0x40  # include/nds/nds_p4.h NDS_P4_SEL_BASE
MODELPARTS_ENTRIES = 37 - 4  # nFTPartsJointNumMax - nFTPartsJointCommonStart
FTMODELPART_DESC_PARTS = 2   # FTModelPartDesc.modelparts[1][2]


class PackError(Exception):
    pass


def fnv1a32(data: bytes) -> int:
    h = 0x811C9DC5
    for b in data:
        h ^= b
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h


def align_up(n: int, a: int) -> int:
    return (n + a - 1) // a * a


def merge(ranges):
    out = []
    for a, b in sorted(ranges):
        if out and a <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], b))
        else:
            out.append((a, b))
    return out


def in_ranges(x: int, ranges) -> bool:
    return any(a <= x < b for a, b in ranges)


def dl_roots(main: P.O2RFile, model: P.O2RFile, model_id: int, attr: int, trees) -> set[int]:
    """Model offsets of every display list the fighter's structures name."""
    lay = ft_layout.layout()
    roots: set[int] = set()

    def model_target(slot_off):
        # A part drawn from another file (Fox's 0x13B parts) is NULL in the
        # pack, as the original cast's packs leave it ("other-file").
        ref = main.ptr(slot_off)
        if ref is None or ref[0] != "extern" or ref[1] != model_id:
            return None
        return ref[2]

    for base, count in trees:
        for i in range(count - 1):
            ref = model.ptr(base + i * P.DOBJ_DESC_SIZE + 4)
            if ref is not None:
                if ref[0] != "intern":
                    raise PackError(f"JointTree desc {i}: display list outside the model")
                roots.add(ref[2])
    container = main.ptr(attr + lay["FTAttributes.modelparts_container"])
    if container is not None:
        if container[0] != "intern":
            raise PackError("modelparts_container is not in Main")
        # One entry per joint past the common ones; the array ends with the
        # fighter's joints (the next words are another table).
        entries = min(MODELPARTS_ENTRIES, (trees[0][1] - 1) - 4)
        for i in range(entries):
            desc = main.ptr(container[2] + 4 * i)
            if desc is None:
                continue
            if desc[0] != "intern":
                raise PackError(f"modelparts_desc[{i}] is not in Main")
            for k in range(FTMODELPART_DESC_PARTS):
                dl = model_target(desc[2] + k * lay["sizeof(FTModelPart)"] + lay["FTModelPart.dl"])
                if dl is not None:
                    roots.add(dl)
    access = main.ptr(attr + lay["FTAttributes.accesspart"])
    if access is not None:
        if access[0] != "intern":
            raise PackError("accesspart is not in Main")
        dl = model_target(access[2] + 4)  # FTAccessPart.dl
        if dl is not None:
            roots.add(dl)
    return roots


def walk_geometry(model: P.O2RFile, roots: set[int]):
    """Display-list blocks and vertex ranges reachable from `roots` (F3DEX2)."""
    blocks = []
    vtx = []
    seen: set[int] = set()

    def walk(off):
        if off in seen:
            return
        seen.add(off)
        start = off
        while True:
            if off + 8 > len(model.data):
                raise PackError(f"display list at {start:#x} runs off the model")
            w0 = model.u32(off)
            op = w0 >> 24
            ref = model.ptr(off + 4)
            if op == 0x01:  # G_VTX
                n = (w0 >> 12) & 0xFF
                if ref is None or ref[0] != "intern":
                    raise PackError(f"G_VTX at {off:#x} without an internal address")
                vtx.append((ref[2], ref[2] + n * 16))
            elif op == 0xDE:  # G_DL
                if ref is None and 0x01 <= (model.u32(off + 4) >> 24) <= 0x0F:
                    pass  # a runtime segment (material/eye lists), not a file pointer
                elif ref is None or ref[0] != "intern":
                    raise PackError(f"G_DL at {off:#x} without an internal address")
                else:
                    walk(ref[2])
                if (w0 >> 16) & 0xFF == 1:  # branch: no return
                    off += 8
                    break
            elif op == 0xDF:  # G_ENDDL
                off += 8
                break
            off += 8
        blocks.append((start, off))

    for r in sorted(roots):
        walk(r)
    return merge(blocks), merge(vtx)


def build(o2r_dir: Path, main_id: int, model_id: int, attr: int, kind: int):
    main = P.O2RFile(o2r_dir / f"{main_id:04x}")
    model = P.O2RFile(o2r_dir / f"{model_id:04x}")
    trees = P.donor_tables(o2r_dir, main_id, model_id, attr)["trees"]
    roots = dl_roots(main, model, model_id, attr, trees)
    dls, vtxs = walk_geometry(model, roots)
    pruned = merge(dls + vtxs)
    kept = []
    cur = 0
    for a, b in pruned:
        if a > cur:
            kept.append((cur, a))
        cur = b
    if cur < len(model.data):
        kept.append((cur, len(model.data)))
    kept = [(a, b) for a, b in kept if b > a]
    for a, b in kept:
        if (a | b) & 3:
            raise PackError(f"kept span {a:#x}-{b:#x} is not word aligned")

    # Root cells: every pruned display list a retained slot names.
    def model_class(target: int, where: str):
        for a, b in kept:
            if a <= target < b:
                return ("kept", target)
        if target in roots or any(target == a for a, _ in dls):
            return ("root", target)
        raise PackError(f"{where}: pointer into pruned model bytes at {target:#x}")

    main_len = len(main.data)
    model_rel = {}
    rel = 0
    for a, b in kept:
        model_rel[(a, b)] = rel
        rel += b - a
    kept_bytes = rel

    def remap(target: int) -> int:
        for (a, b), r in model_rel.items():
            if a <= target < b:
                return r + target - a
        raise PackError(f"model offset {target:#x} outside kept spans")

    pairs = []  # (slot section, slot offset, target class, target)
    for slot, (kind_, fid, target) in sorted(main.slots.items()):
        if kind_ == "intern":
            pairs.append((0, slot, "main", target))
        elif fid == model_id:
            pairs.append((0, slot) + model_class(target, f"main slot {slot:#x}"))
        else:
            pairs.append((0, slot, "null", fid))
    for slot, (kind_, fid, target) in sorted(model.slots.items()):
        if in_ranges(slot, pruned):
            continue
        if kind_ != "intern":
            raise PackError(f"model slot {slot:#x} points into file {fid:#x}")
        pairs.append((1, slot) + model_class(target, f"model slot {slot:#x}"))
    cells = sorted({t for _, _, c, t in pairs if c == "root"})
    cell_index = {t: i for i, t in enumerate(cells)}

    main_body = bytes(main.data)
    model_body = b"".join(bytes(model.data[a:b]) for a, b in kept)
    def cell(target: int) -> bytes:
        if struct.unpack_from(">I", model.data, target)[0] >> 24 == 0xDF:
            return struct.pack(">2I", ENDDL, 0)  # a bare G_ENDDL list
        if target == 0:
            raise PackError("a root at model offset 0 reads as an empty list")
        return struct.pack(">2I", ENDDL, target)

    roots_body = b"".join(cell(t) for t in cells)
    sec1_off = align_up(main_len, ALIGN)
    data = bytearray(sec1_off + kept_bytes + len(roots_body))
    data[0:main_len] = main_body
    data[sec1_off:sec1_off + kept_bytes] = model_body
    data[sec1_off + kept_bytes:] = roots_body

    fixups = []
    nulls = {}
    for sec, slot, cls, target in pairs:
        slot_data = slot if sec == 0 else sec1_off + remap(slot)
        if cls == "main":
            if not 0 <= target < main_len:
                raise PackError(f"main slot {slot:#x} targets {target:#x} outside Main")
            fixups.append((slot_data, target))
        elif cls == "kept":
            fixups.append((slot_data, sec1_off + remap(target)))
        elif cls == "root":
            fixups.append((slot_data, sec1_off + kept_bytes + 8 * cell_index[target]))
        else:
            fixups.append((slot_data, NULL))
            nulls[target] = nulls.get(target, 0) + 1
    if len({s for s, _ in fixups}) != len(fixups):
        raise PackError("duplicate fixup slot")

    spans = [(0, 0, main_len)]
    for a, b in kept:
        spans.append((a, model_rel[(a, b)], b - a))
    sections = [
        (main_id, 0, main_len, main_len, 0, 1, 0, 0),
        (model_id, sec1_off, kept_bytes + len(roots_body), len(model.data),
         1, len(kept), kept_bytes, len(cells)),
    ]
    fixup_bytes = b"".join(struct.pack("<2I", s, t) for s, t in fixups)
    span_bytes = b"".join(struct.pack("<3I", *s) for s in spans)
    file_bytes = 64 + 32 * len(sections) + len(data) + len(fixup_bytes) + len(span_bytes)
    header = struct.pack(
        "<16I", MAGIC, VERSION, file_bytes, kind, len(sections), len(fixups), len(spans), 0,
        len(data), fnv1a32(bytes(data)), fnv1a32(fixup_bytes), fnv1a32(span_bytes),
        main_id, model_id, len(model.data), 0)
    blob = (header + b"".join(struct.pack("<8I", *s) for s in sections)
            + bytes(data) + fixup_bytes + span_bytes)
    assert len(blob) == file_bytes
    meta = {
        "kind": kind, "main_bytes": main_len, "model_bytes": len(model.data),
        "model_kept_bytes": kept_bytes, "geometry_bytes": sum(b - a for a, b in pruned),
        "display_lists": len(dls), "roots": len(roots), "root_cells": len(cells),
        "fixups": len(fixups), "null_fixups": {f"{k:#x}": v for k, v in sorted(nulls.items())},
        "file_bytes": file_bytes,
    }
    return blob, meta


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--o2r", type=Path, required=True, help="the content's generated o2r/ directory")
    ap.add_argument("--content", help="the content's registry name; main, model, attributes "
                    "and kind then come from --export-root and scripts/p4/contents.json")
    ap.add_argument("--export-root", type=Path)
    ap.add_argument("--main", type=lambda v: int(v, 0))
    ap.add_argument("--model", type=lambda v: int(v, 0))
    ap.add_argument("--attributes", type=lambda v: int(v, 0))
    ap.add_argument("--kind", type=lambda v: int(v, 0), help="NDS_P4_SEL_BASE + content")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    if args.content is not None:
        import p4_contents  # noqa: E402

        if args.export_root is None:
            ap.error("--content needs --export-root")
        row = p4_contents.enabled_rows([args.content])[0]
        facts = p4_contents.export_facts(args.export_root, args.content)
        args.main, args.model, args.attributes = facts["main"], facts["model"], facts["attributes"]
        args.kind = SEL_BASE + row["id"]
    if None in (args.main, args.model, args.attributes, args.kind):
        ap.error("give --content and --export-root, or --main/--model/--attributes/--kind")
    try:
        blob, meta = build(args.o2r, args.main, args.model, args.attributes, args.kind)
    except PackError as error:
        print(f"p4_preview_pack: {error}", file=sys.stderr)
        return 1
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(blob)
    args.out.with_suffix(".json").write_text(json.dumps(meta, indent=1) + "\n",
                                             encoding="utf-8", newline="\n")
    print(json.dumps(meta))
    return 0


if __name__ == "__main__":
    sys.exit(main())
