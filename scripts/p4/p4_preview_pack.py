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

The same pack loads a content's fighters in a match (S15), so it also prunes
every alternate model part (part 1 up: the native owner image draws the ones
it carries, and any other declines the native draw with or without its
source list), and writes the battle manifest
beside it (<kind>.ext, reloc_preview_pack.c's BEX1 rows): each Main pointer
left NULL as (slot, file, offset), which the match restores once those files
are loaded. No other file of the content may point into pruned model bytes.

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
EXT_MAGIC = 0x31584542  # BEX1: reloc_preview_pack.c NDS_BATTLE_EXTERN_MAGIC
EXT_VERSION = 2
EXT_MAX = 24            # NDS_BATTLE_EXTERN_MAX
EXT_LOAD_ONLY = 0xFFFF  # NDS_BATTLE_EXTERN_LOAD_ONLY: load the file, patch nothing


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


def dl_roots(main: P.O2RFile, model: P.O2RFile, model_id: int, attr: int, trees,
             dl_pairs: bool = False) -> set[int]:
    """Model offsets of every display list the fighter's structures name.
    With `dl_pairs` (FTCommonPart.flags bit 0, Yoshi's form: Bowser) a
    JointTree entry names a `Gfx *dls[2]` pair, which stays in the pack as
    data; its two lists are the roots."""
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
                if not dl_pairs:
                    roots.add(ref[2])
                    continue
                for k in range(2):
                    dl = model.ptr(ref[2] + 4 * k)
                    if dl is None:
                        continue
                    if dl[0] != "intern":
                        raise PackError(f"JointTree desc {i}: pair list outside the model")
                    roots.add(dl[2])
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


def variant_roots(main: P.O2RFile, model_id: int, attr: int, trees) -> set[int]:
    """Model offsets of every alternate model-part list (part 1 up, both
    details) in the fighter's modelparts table. The native owner image draws
    the ones it carries (P2_MODEL_PART_ROOT_VARIANTS); the others -- the
    variants its pins drop, and Banjo's parts on joints without a list of
    their own -- decline the fighter's native draw whether the slot names
    a source list or a root cell, as no ROM draws a source list here, so
    their bytes are pruned with the rest."""
    lay = ft_layout.layout()
    container = main.ptr(attr + lay["FTAttributes.modelparts_container"])
    roots: set[int] = set()
    if container is None or container[0] != "intern":
        return roots
    rows = {}
    for index in range(min(MODELPARTS_ENTRIES, (trees[0][1] - 1) - 4)):
        ref = main.ptr(container[2] + 4 * index)
        if ref is not None and ref[0] == "intern":
            rows[index] = ref[2]
    starts = sorted(set(rows.values()) | {container[2]})
    size = lay["sizeof(FTModelPart)"]
    for base in rows.values():
        bound = min([s for s in starts if s > base], default=len(main.data))
        k = FTMODELPART_DESC_PARTS  # part 0's two details are dl_roots'
        while base + (k + 1) * size <= bound:
            ref = main.ptr(base + k * size + lay["FTModelPart.dl"])
            if ref is not None and ref[0] == "extern" and ref[1] == model_id:
                roots.add(ref[2])
            k += 1
    return roots


def high_only_spans(main: P.O2RFile, model: P.O2RFile, model_id: int, attr: int,
                    trees, pruned, other_targets: set[int]):
    """Kept model ranges only the high detail reads, for the battle pack a
    match of three or four fighters loads: there a content draws its
    low-detail model in the close-ups too (nds_p4.c
    ndsP4ContentLowDetailOnly). The low detail takes a joint whose low
    DObjDesc names no list from the high rows (lbCommonSetupFighterPartsDObjs,
    ftParamSetModelPartDetailAll, the hidden parts), so those joints' high
    rows stay, and both JointTrees and the per-joint arrays stay whole (they
    are indexed by joint). A region runs from its pointer target to the next
    one; it is pruned when only the other joints' high rows and the high
    model-part rows reach it. Returns the ranges and the slots (Main and
    model offsets, as ("main"|"model", slot)) that point into them, which
    the pack writes NULL."""
    import bisect

    lay = ft_layout.layout()
    D = P.DOBJ_DESC_SIZE

    def main_target(slot):
        ref = main.ptr(slot)
        return ref[2] if (ref is not None and ref[0] == "extern" and ref[1] == model_id) else None

    def model_target(slot):
        ref = model.ptr(slot)
        return ref[2] if (ref is not None and ref[0] == "intern") else None

    container = main.ptr(attr + lay["FTAttributes.commonparts_container"])[2]
    parts = [container + d * lay["sizeof(FTCommonPart)"] for d in range(2)]
    low_tree, low_count = trees[1]
    fallback = {i for i in range(low_count - 1) if model.ptr(low_tree + i * D + 4) is None}
    keep: set[int] = set()      # roots the low detail (or anything else) reads
    high: dict[int, list] = {}  # high-only candidate target -> the slots naming it
    leaves: set[int] = set()    # indexed arrays: kept, walked entry by entry

    def candidate(target, where):
        high.setdefault(target, []).append(where)

    for d in range(2):
        tree, count = trees[d]
        leaves.add(tree)
        for i in range(count - 1):
            t = model_target(tree + i * D + 4)
            if t is None:
                continue
            if d == 1 or i in fallback:
                keep.add(t)
            else:
                candidate(t, ("model", tree + i * D + 4))
        for field in ("p_mobjsubs", "p_costume_matanim_joints"):
            array = main_target(parts[d] + lay[f"FTCommonPart.{field}"])
            if array is None:
                continue
            leaves.add(array)
            for i in range(count - 1):
                t = model_target(array + 4 * i)
                if t is None:
                    continue
                if d == 1 or i in fallback:
                    keep.add(t)
                else:
                    candidate(t, ("model", array + 4 * i))
    # The high arrays may be the low ones: every entry is then the low's.
    for d in range(2):
        for field in ("p_mobjsubs", "p_costume_matanim_joints"):
            a0 = main_target(parts[0] + lay[f"FTCommonPart.{field}"])
            if (a0 is not None) and (a0 == main_target(parts[1] + lay[f"FTCommonPart.{field}"])):
                for i in range(trees[0][1] - 1):
                    t = model_target(a0 + 4 * i)
                    if t is not None:
                        keep.add(t)
    # Model-part rows: modelparts[part][detail], detail 0 the high one.
    rows_slots = set()
    mp = main.ptr(attr + lay["FTAttributes.modelparts_container"])
    size = lay["sizeof(FTModelPart)"]
    if mp is not None and mp[0] == "intern":
        rows = {}
        for index in range(min(MODELPARTS_ENTRIES, (trees[0][1] - 1) - 4)):
            ref = main.ptr(mp[2] + 4 * index)
            if ref is not None and ref[0] == "intern":
                rows[index] = ref[2]
        starts = sorted(set(rows.values()) | {mp[2]})
        for base in rows.values():
            bound = min([s for s in starts if s > base], default=len(main.data))
            k = 0
            while base + (k + 1) * size <= bound:
                for field in ("mobjsubs", "costume_matanim_joints", "main_matanim_joints"):
                    slot = base + k * size + lay[f"FTModelPart.{field}"]
                    rows_slots.add(slot)
                    t = main_target(slot)
                    if t is None:
                        continue
                    if k & 1:
                        keep.add(t)
                    else:
                        candidate(t, ("main", slot))
                k += 1
    # Everything else Main or another file names in the model is kept.
    skip = {parts[d] + lay[f"FTCommonPart.{f}"] for d in range(2)
            for f in ("dobjdesc", "p_mobjsubs", "p_costume_matanim_joints")} | rows_slots
    for slot, (kind_, fid, target) in main.slots.items():
        if kind_ == "extern" and fid == model_id and slot not in skip:
            keep.add(target)
    keep |= other_targets

    targets = sorted({v[2] for v in model.slots.values() if v[0] == "intern"}
                     | keep | set(high) | leaves | {0})
    ends = dict(zip(targets, targets[1:] + [len(model.data)]))
    slots = sorted(model.slots)

    def closure(roots):
        seen: set[int] = set()
        stack = list(roots)
        while stack:
            t = stack.pop()
            if t in seen or t not in ends:
                continue
            seen.add(t)
            if t in leaves:
                continue
            i = bisect.bisect_left(slots, t)
            while i < len(slots) and slots[i] < ends[t]:
                ref = model.slots[slots[i]]
                if ref[0] == "intern":
                    stack.append(ref[2])
                i += 1
        return seen

    kept_regions = closure(keep | leaves)
    high_regions = closure(set(high)) - kept_regions
    spans = merge([(t, ends[t]) for t in high_regions])
    # Slots outside the pruned ranges that name a pruned region: only the
    # candidates' own slots may (closure keeps everything else).
    nulls = []
    for slot in slots:
        ref = model.slots[slot]
        if ref[0] != "intern" or not in_ranges(ref[2], spans):
            continue
        if in_ranges(slot, spans) or in_ranges(slot, pruned):
            continue
        if ("model", slot) not in [w for ws in high.values() for w in ws]:
            raise PackError(f"model slot {slot:#x} names high-only bytes at {ref[2]:#x}")
        nulls.append(("model", slot))
    for t, ws in high.items():
        if t in high_regions:
            nulls.extend(w for w in ws if w[0] == "main")
    return spans, set(nulls)


def build(o2r_dir: Path, main_id: int, model_id: int, attr: int, kind: int,
          extra_roots: set[int] = frozenset(), low_only: bool = False):
    main = P.O2RFile(o2r_dir / f"{main_id:04x}")
    model = P.O2RFile(o2r_dir / f"{model_id:04x}")
    tables = P.donor_tables(o2r_dir, main_id, model_id, attr)
    roots = dl_roots(main, model, model_id, attr, tables["trees"], tables["dl_pairs"])
    variants = variant_roots(main, model_id, attr, tables["trees"])
    roots |= variants | set(extra_roots)
    dls, vtxs = walk_geometry(model, roots)
    pruned = merge(dls + vtxs)
    other_targets = set()
    others = []
    for path in sorted(o2r_dir.iterdir()):
        try:
            fid = int(path.name, 16)
        except ValueError:
            continue
        if fid in (main_id, model_id):
            continue
        for slot, (kind_, target_fid, target) in P.O2RFile(path).slots.items():
            if kind_ == "extern" and target_fid == model_id:
                other_targets.add(target)
                others.append((fid, slot, target))
    # The low-detail battle pack also drops what only the high detail reads;
    # the slots naming it read NULL ("drop": no manifest row restores them).
    drops = set()
    high_bytes = 0
    if low_only:
        spans, drops = high_only_spans(main, model, model_id, attr, tables["trees"],
                                       pruned, other_targets)
        high_bytes = sum(b - a for a, b in spans)
        pruned = merge(pruned + spans)
    # The content's other files resolve their pointers into the model by
    # source offset through the pack's spans: none may name pruned bytes.
    for fid, slot, target in others:
        if in_ranges(target, pruned):
            raise PackError(f"file {fid:#x} slot {slot:#x} points into pruned model "
                            f"bytes at {target:#x}")
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
        elif ("main", slot) in drops:
            pairs.append((0, slot, "drop", target))
        elif fid == model_id:
            pairs.append((0, slot) + model_class(target, f"main slot {slot:#x}"))
        else:
            pairs.append((0, slot, "null", fid))
    for slot, (kind_, fid, target) in sorted(model.slots.items()):
        if in_ranges(slot, pruned):
            continue
        if kind_ != "intern":
            raise PackError(f"model slot {slot:#x} points into file {fid:#x}")
        if ("model", slot) in drops:
            pairs.append((1, slot, "drop", target))
            continue
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
        elif cls == "drop":
            fixups.append((slot_data, NULL))
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

    # The battle manifest (reloc_preview_pack.c, BEX1): every Main pointer
    # the pack left NULL, as (Main slot, file, offset in that file), which a
    # match restores after loading those files. No foreign image bank.
    # Remix built several Main files from their parent's and retargeted the
    # header words' file ids to the donor's own files without moving their
    # offsets (Sheik's, Banjo's and Dedede's Captain words: Falcon Punch at
    # +0x760, Falcon Kick at +0xB08, into 64- and 128-byte files). On the N64
    # they relocate to pointers past their file that nothing reads. The row
    # still loads the file (it is how the content's own special file reaches
    # the status buffer the fighter's file setup reads), but its target is
    # EXT_LOAD_ONLY: the slot stays NULL.
    externs = []
    dangling = []
    for _sec, slot, cls, _fid in pairs:
        if cls != "null":
            continue
        _kind, fid, target = main.slots[slot]
        dep = o2r_dir / f"{fid:04x}"
        if dep.exists() and target >= len(P.O2RFile(dep).data):
            dangling.append(f"{slot:#x}->{fid:#x}+{target:#x}")
            externs.append((slot, fid, EXT_LOAD_ONLY))
            continue
        if max(slot, fid, target) > 0xFFFF:
            raise PackError(f"main slot {slot:#x} -> {fid:#x}+{target:#x} exceeds a manifest row")
        externs.append((slot, fid, target))
    if len(externs) > EXT_MAX:
        raise PackError(f"{len(externs)} Main externs exceed the manifest's {EXT_MAX}")
    ext = (struct.pack("<IHHIIII", EXT_MAGIC, EXT_VERSION, len(externs), 0, 0, fnv1a32(b""), 0)
           + b"".join(struct.pack("<3H", *row) for row in externs))
    meta = {
        "kind": kind, "main_bytes": main_len, "model_bytes": len(model.data),
        "model_kept_bytes": kept_bytes, "geometry_bytes": sum(b - a for a, b in pruned),
        "display_lists": len(dls), "roots": len(roots), "variant_roots": len(variants),
        "root_cells": len(cells),
        "fixups": len(fixups), "null_fixups": {f"{k:#x}": v for k, v in sorted(nulls.items())},
        "file_bytes": file_bytes, "manifest_rows": len(externs), "dangling_externs": dangling,
        "low_only": low_only, "high_only_bytes": high_bytes,
        "dropped_slots": sum(1 for p in pairs if p[2] == "drop"),
    }
    return blob, meta, ext


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
    ap.add_argument("--battle-out", type=Path,
                    help="also write the low-detail battle pack (matches of three or four)")
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
        blob, meta, ext = build(args.o2r, args.main, args.model, args.attributes, args.kind)
        if args.battle_out is not None:
            low, low_meta, low_ext = build(args.o2r, args.main, args.model, args.attributes,
                                           args.kind, low_only=True)
            # Main is the same file: one manifest serves both packs.
            if low_ext != ext:
                raise PackError("the low-detail pack's manifest differs from the preview's")
    except PackError as error:
        print(f"p4_preview_pack: {error}", file=sys.stderr)
        return 1
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(blob)
    args.out.with_suffix(".ext").write_bytes(ext)
    args.out.with_suffix(".json").write_text(json.dumps(meta, indent=1) + "\n",
                                             encoding="utf-8", newline="\n")
    print(json.dumps(meta))
    if args.battle_out is not None:
        args.battle_out.parent.mkdir(parents=True, exist_ok=True)
        args.battle_out.write_bytes(low)
        args.battle_out.with_suffix(".json").write_text(json.dumps(low_meta, indent=1) + "\n",
                                                        encoding="utf-8", newline="\n")
        print(json.dumps(low_meta))
    return 0


if __name__ == "__main__":
    sys.exit(main())
