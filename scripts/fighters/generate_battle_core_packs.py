#!/usr/bin/env python3
"""Generate source-exact LOW-detail fighter core packs for VSBattle.

The existing FPC1 preview packs are intentionally CSS/HIGH-only.  Four-player
VSBattle instead constructs LOW commonparts and falls back to HIGH per joint
when the LOW DObjDesc has no display.  This encoder keeps the structural/data
closure reachable from both source detail triples, plus every non-geometry
Main->Model target, while replacing Gfx/Vtx geometry with the same FPC1 root
identity cells consumed by the native renderer.

Main remains source-offset identity.  Non-Model Main externs are emitted NULL
in FPC1 and listed in a generated patch manifest so the runtime can restore
them after BattleShip's ordinary status/special loaders make those files
resident.  ShieldPose externs are omitted from that manifest: the dedicated
compact ShieldPose owner patches its nine generated slots instead.
"""

from __future__ import annotations

import argparse
import collections
import json
import os
import pathlib
import struct
import sys


ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "fighters"))

import estimate_fighter_pack as est  # noqa: E402
import generate_preview_core_packs as fpc  # noqa: E402
import preview_source_metadata as preview  # noqa: E402


DISPLAY = {
    "mario": "Mario", "fox": "Fox", "donkey": "Donkey", "samus": "Samus",
    "luigi": "Luigi", "link": "Link", "yoshi": "Yoshi",
    "captain": "Captain", "kirby": "Kirby", "pikachu": "Pikachu",
    "purin": "Purin", "ness": "Ness",
}

SHARED_MANIFEST = ROOT / "docs" / "optimization" / "NDS_BATTLE_CORE_PACKS.generated.json"
EXTERN_MAGIC = 0x31584542  # "BEX1" little-endian
EXTERN_VERSION = 1
EXTERN_HEADER_FMT = "<IHH"
EXTERN_ROW_FMT = "<HHH"


class BattlePackError(RuntimeError):
    pass


def _align(value: int, alignment: int = fpc.ALIGN) -> int:
    return (value + alignment - 1) // alignment * alignment


def _owner(objects, offset: int):
    hits = [row for row in objects
            if row.offset <= offset < row.offset + row.size]
    if not hits:
        return None
    # Split/alias rows can overlap.  The narrowest row is the strongest owner
    # for a concrete relocation target; ties stay source-order deterministic.
    return min(hits, key=lambda row: row.size)


def _merge_spans(rows):
    spans = []
    for start, end in sorted((row.offset, row.offset + row.size) for row in rows):
        if spans and start <= spans[-1][1]:
            spans[-1][1] = max(spans[-1][1], end)
        else:
            spans.append([start, end])
    return spans


def _inside(spans, offset: int) -> bool:
    return any(start <= offset < end for start, end in spans)


def _source_payload(file_id: int) -> bytes:
    path = preview.o2r_path_by_id(file_id)
    if path is None:
        raise BattlePackError("asset %d has no O2R" % file_id)
    data, _size = preview.oler_data(path)
    if data is None:
        raise BattlePackError("asset %d is not OLER" % file_id)
    return data


def _asset_ids(meta: dict) -> tuple[int, int]:
    main_ids = {row["target_file"]
                for row in meta["main_intern"]["retained"]}
    if len(main_ids) != 1:
        raise BattlePackError("Main asset id is ambiguous: %r" % main_ids)
    main_id = main_ids.pop()
    reloc_file = meta["model_intern"].get("reloc_file", "")
    try:
        model_id = int(reloc_file.split("_", 1)[0])
    except (TypeError, ValueError):
        raise BattlePackError("Model asset id is ambiguous: %r" % reloc_file)
    return main_id, model_id


def _structural_model_closure(fighter: str, model_id: int, meta: dict,
                              types: est.TypeTable):
    idx, _entry = est.index_closure(fighter, types)
    pf = next((item for item in idx.files if item.file_id == model_id), None)
    if pf is None:
        raise BattlePackError("%s Model %d absent from indexed closure" %
                              (fighter, model_id))

    roots = meta["roots"]
    high_tree = _owner(pf.objects, roots["high_jt"])
    low_tree = _owner(pf.objects, roots["low_jt"])
    if (high_tree is None or low_tree is None or
            high_tree.type_name != "DObjDesc" or
            low_tree.type_name != "DObjDesc" or
            high_tree.count != low_tree.count or high_tree.count < 2):
        raise BattlePackError(
            "%s commonparts JointTree cardinality is not source-closed" % fighter)
    # BattleShip indexes p_mobjsubs / p_costume_matanim_joints by the same
    # DObj index as the JointTree, excluding only the final DOBJ_ARRAY_MAX
    # sentinel.  Some relocData TUs split that logical table into adjacent C
    # arrays (Donkey 0x7370[8] + 0x7390[18], for example); object-by-object
    # compaction must not collapse that intentional source adjacency.
    dispatch_count = high_tree.count - 1
    forced_spans = []
    # preview_source_metadata proves HIGH MObj dispatch == source offset zero.
    seeds = [roots["high_jt"], 0, roots["high_apost"],
             roots["low_jt"], roots["low_mobj"], roots["low_apost"]]

    # Battle uses modelpart/skeleton tables too.  Seed every non-geometry
    # Main->Model target so source status changes cannot inherit the CSS pack's
    # "no demo path" pruning policy.
    main_model_rows = (meta["main_extern"]["kept"] +
                       meta["main_extern"]["pruned"])
    for edge in main_model_rows:
        if edge.get("target_file") != model_id:
            continue
        row = _owner(pf.objects, edge["target_old"])
        if row is None:
            raise BattlePackError(
                "%s Main->Model target 0x%x has no parsed owner" %
                (fighter, edge["target_old"]))
        if row.type_name not in ("Gfx", "Vtx"):
            seeds.append(edge["target_old"])

    queue = collections.deque()
    for offset in seeds:
        row = _owner(pf.objects, offset)
        if row is None:
            raise BattlePackError("%s root 0x%x has no parsed owner" %
                                  (fighter, offset))
        queue.append(row)

    def keep_dispatch(root: int, label: str) -> None:
        end = root + dispatch_count * 4
        if end > len(pf.source["payload"]):
            raise BattlePackError(
                "%s %s dispatch 0x%x..0x%x escapes Model" %
                (fighter, label, root, end))
        for slot in range(root, end, 4):
            raw = int.from_bytes(pf.source["payload"][slot:slot + 4], "big")
            target = pf.source["pointers"].get(slot)
            if raw == 0:
                if target is not None:
                    raise BattlePackError(
                        "%s %s NULL slot 0x%x has reloc target" %
                        (fighter, label, slot))
                continue
            if target is None or target[0] != model_id:
                raise BattlePackError(
                    "%s %s slot 0x%x is non-NULL without local reloc pointer" %
                    (fighter, label, slot))
            target_row = _owner(pf.objects, target[1])
            if target_row is None:
                raise BattlePackError(
                    "%s %s target 0x%x has no parsed owner" %
                    (fighter, label, target[1]))
            if target_row.type_name in ("Gfx", "Vtx"):
                raise BattlePackError(
                    "%s %s dispatch points directly at %s %s" %
                    (fighter, label, target_row.type_name, target_row.symbol))
            queue.append(target_row)
        forced_spans.append((root, end))

    # HIGH MObj dispatch lives at SOURCE OFFSET ZERO for all 12 base fighters;
    # zero is a valid table address, not a NULL pointer.  Several source TUs
    # split that logical table into adjacent C arrays (Samus 16+17 entries at
    # 0x0000..0x0084), so preserve its full non-sentinel JointTree extent just
    # like the LOW MObj and both matanim dispatches.
    keep_dispatch(0, "HIGH MObj")
    keep_dispatch(roots["low_mobj"], "LOW MObj")
    keep_dispatch(roots["high_apost"], "HIGH matanim")
    keep_dispatch(roots["low_apost"], "LOW matanim")

    # Some fighter-owned weapon/article attributes point at a runtime handle
    # CELL in Model rather than directly at the logical table.  BattleShip
    # then advances through that cell and the adjacent pointer cells as one C
    # array even when relocData split them into separate declarations.  Link's
    # Spin Attack is the first live example:
    #   0x110A8 { NULL } + 0x110AC { MObjSub ** }
    # and the same two-word shape appears at 0x11990/0x11994 and
    # 0x11A40/0x11A44 for its animation tables.  Object-by-object packing
    # collapses the adjacency and makes the second DObj read the next unrelated
    # retained object (0x118F8's DObjDLLink in the observed failure).
    #
    # Detect the source shape rather than naming Link offsets: a Main->Model
    # target whose first word is NULL and whose following contiguous words are
    # only NULL or local reloc pointers is a runtime dispatch root.  Preserve
    # that whole pointer run, queue every pointed-to structural object, and stop
    # at the first non-pointer scalar.  A geometry target still fails closed.
    payload = pf.source["payload"]
    pointer_map = pf.source["pointers"]
    runtime_dispatch_roots = set()
    for edge in main_model_rows:
        if edge.get("target_file") != model_id:
            continue
        root = int(edge["target_old"])
        if (root + 4 > len(payload)):
            continue
        if int.from_bytes(payload[root:root + 4], "big") != 0:
            continue
        row = _owner(pf.objects, root)
        if row is None or row.size != 4:
            continue
        cursor = root
        saw_pointer = False
        while cursor + 4 <= len(payload):
            raw = int.from_bytes(payload[cursor:cursor + 4], "big")
            target = pointer_map.get(cursor)
            if raw == 0:
                if target is not None:
                    raise BattlePackError(
                        "%s runtime dispatch NULL slot 0x%x has reloc target" %
                        (fighter, cursor))
                cursor += 4
                continue
            if target is None:
                break
            if target[0] != model_id:
                raise BattlePackError(
                    "%s runtime dispatch 0x%x escapes to asset %d" %
                    (fighter, cursor, target[0]))
            target_row = _owner(pf.objects, target[1])
            if target_row is None:
                raise BattlePackError(
                    "%s runtime dispatch target 0x%x has no parsed owner" %
                    (fighter, target[1]))
            if target_row.type_name in ("Gfx", "Vtx"):
                raise BattlePackError(
                    "%s runtime dispatch 0x%x points directly at %s %s" %
                    (fighter, cursor, target_row.type_name, target_row.symbol))
            queue.append(target_row)
            saw_pointer = True
            cursor += 4
        if saw_pointer and cursor > (root + 4):
            forced_spans.append((root, cursor))
            runtime_dispatch_roots.add(root)

    kept = {}
    geometry_targets = set()
    while queue:
        row = queue.popleft()
        key = (row.offset, row.symbol)
        if key in kept:
            continue
        if row.type_name in ("Gfx", "Vtx"):
            raise BattlePackError("%s structural seed resolved to %s %s" %
                                  (fighter, row.type_name, row.symbol))
        kept[key] = row
        for slot, target in pf.source["pointers"].items():
            if not (row.offset <= slot < row.offset + row.size):
                continue
            target_file, target_offset = target
            if target_file != model_id:
                raise BattlePackError(
                    "%s Model structural row %s escapes to asset %d" %
                    (fighter, row.symbol, target_file))
            target_row = _owner(pf.objects, target_offset)
            if target_row is None:
                raise BattlePackError(
                    "%s Model target 0x%x from %s has no parsed owner" %
                    (fighter, target_offset, row.symbol))
            if target_row.type_name in ("Gfx", "Vtx"):
                geometry_targets.add(target_offset)
                continue
            queue.append(target_row)

    # A few source pointer arrays use the following zero-padding word as their
    # sentinel rather than declaring the NULL as part of the C array.  The
    # consumer still walks until NULL.  Link Spin Attack is the natural-path
    # witness: dLinkModel_Tex_0x114E8[9] has nine MObjSub pointers and the PAD(4)
    # at 0x1150C is gcAddMObjAll's required terminator.  Packing only the
    # declared 36 bytes made the next compact object look like a tenth pointer.
    # Preserve an immediate source zero after every retained pointer array.
    # Fixed-count readers merely carry one unreachable zero; sentinel readers
    # retain the exact linker-layout contract.
    for row in kept.values():
        if (row.pointer_depth <= 0) or (row.offset is None) or (row.size <= 0):
            continue
        end = row.offset + row.size
        if (end + 4 <= len(payload)) and \
                (int.from_bytes(payload[end:end + 4], "big") == 0):
            forced_spans.append((row.offset, end + 4))

    spans = _merge_spans(kept.values())
    spans.extend([[start, end] for start, end in forced_spans])
    spans.sort()
    merged = []
    for start, end in spans:
        if merged and start <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], end)
        else:
            merged.append([start, end])
    spans = merged
    if not spans:
        raise BattlePackError("%s structural Model closure is empty" % fighter)

    # A retained structural span must never swallow a geometry row.  Geometry
    # is replaced by native root identity cells, not copied accidentally.
    for row in pf.objects:
        if row.type_name not in ("Gfx", "Vtx"):
            continue
        if any(start < row.offset + row.size and row.offset < end
               for start, end in spans):
            raise BattlePackError(
                "%s structural span overlaps %s %s @ 0x%x" %
                (fighter, row.type_name, row.symbol, row.offset))

    return pf, spans, geometry_targets


def _build_one(kind: str, meta: dict, types: est.TypeTable,
               shield_by_main: dict[int, int]):
    fighter = DISPLAY[kind]
    fkind = fpc.KIND_ORDER.index(kind)
    main_id, model_id = _asset_ids(meta)
    main = _source_payload(main_id)
    model = _source_payload(model_id)
    pf, spans, geometry_targets = _structural_model_closure(
        fighter, model_id, meta, types)

    def model_rel(source_offset: int) -> int:
        cursor = 0
        for start, end in spans:
            if start <= source_offset < end:
                return cursor + source_offset - start
            cursor += end - start
        raise BattlePackError(
            "%s Model offset 0x%x is outside retained closure" %
            (fighter, source_offset))

    # Any geometry target that a retained Model/Main pointer needs receives one
    # source-identity root cell.  Vtx targets are not valid DObj/modelpart roots
    # and fail closed rather than being disguised as display lists.
    root_targets = set(geometry_targets)
    for edge in meta["main_extern"]["kept"] + meta["main_extern"]["pruned"]:
        if edge.get("target_file") != model_id:
            continue
        target = edge["target_old"]
        if _inside(spans, target):
            continue
        row = _owner(pf.objects, target)
        if row is None:
            raise BattlePackError(
                "%s Main Model target 0x%x has no parsed owner" %
                (fighter, target))
        if row.type_name == "Gfx":
            root_targets.add(target)
        elif row.type_name == "Vtx":
            raise BattlePackError(
                "%s Main points directly at Vtx 0x%x (%s)" %
                (fighter, target, row.symbol))
        else:
            raise BattlePackError(
                "%s Main structural target 0x%x (%s) escaped closure" %
                (fighter, target, row.symbol))

    compact_model = b"".join(model[start:end] for start, end in spans)
    roots = sorted(root_targets)
    root_index = {source: index for index, source in enumerate(roots)}
    roots_body = b"".join(struct.pack(">2I", fpc.ENDDL, source)
                          for source in roots)

    tail_rows = meta.get("tail_files", [])
    tail_bodies = []
    for tail in tail_rows:
        data = _source_payload(int(tail["fid"]))
        if len(data) != int(tail["bytes"]):
            raise BattlePackError("%s tail %d byte size drifted" %
                                  (fighter, tail["fid"]))
        tail_bodies.append(data)

    bodies = [main, compact_model + roots_body] + tail_bodies
    asset_ids = [main_id, model_id] + [int(row["fid"]) for row in tail_rows]
    source_bytes = [len(main), len(model)] + [len(body) for body in tail_bodies]
    if len(bodies) > fpc.MAX_SECTIONS:
        raise BattlePackError("%s needs %d FPC sections" %
                              (fighter, len(bodies)))

    data_offsets = []
    cursor = 0
    for body in bodies:
        cursor = _align(cursor)
        data_offsets.append(cursor)
        cursor += len(body)
    data = bytearray(cursor)
    for base, body in zip(data_offsets, bodies):
        data[base:base + len(body)] = body

    main_base = data_offsets[0]
    model_base = data_offsets[1]
    model_raw_bytes = sum(end - start for start, end in spans)

    def root_target(source_offset: int) -> int:
        return model_base + model_raw_bytes + root_index[source_offset] * 8

    fixups = []
    seen_fixups = {}

    def emit(slot: int, target: int) -> None:
        if slot in seen_fixups:
            if seen_fixups[slot] != target:
                raise BattlePackError(
                    "%s conflicting fixup @ 0x%x: 0x%x / 0x%x" %
                    (fighter, slot, seen_fixups[slot], target))
            return
        seen_fixups[slot] = target
        fixups.append((slot, target))

    for edge in meta["main_intern"]["retained"]:
        emit(main_base + edge["slot_new"], main_base + edge["target_new"])

    tail_base = {int(row["fid"]): data_offsets[2 + index]
                 for index, row in enumerate(tail_rows)}
    shield_asset = shield_by_main.get(main_id)
    external_patches = []
    for edge in meta["main_extern"]["kept"] + meta["main_extern"]["pruned"]:
        slot = main_base + edge["slot_new"]
        dep = int(edge["target_file"])
        source_target = int(edge["target_old"])
        if dep == model_id:
            if _inside(spans, source_target):
                target = model_base + model_rel(source_target)
            else:
                target = root_target(source_target)
            emit(slot, target)
        elif dep in tail_base:
            emit(slot, tail_base[dep] + source_target)
        elif dep == main_id:
            emit(slot, main_base + source_target)
        else:
            emit(slot, fpc.NULL)
            if dep != shield_asset:
                external_patches.append(
                    (edge["slot_new"], dep, source_target))

    # Relocate every pointer word inside retained Model spans directly from the
    # O2R relocation map.  Targets outside the structural closure must be Gfx
    # roots owned by the native renderer; anything else is an incomplete pack.
    for slot, (dep, source_target) in pf.source["pointers"].items():
        if not _inside(spans, slot):
            continue
        if dep != model_id:
            raise BattlePackError(
                "%s retained Model slot 0x%x escapes to asset %d" %
                (fighter, slot, dep))
        compact_slot = model_base + model_rel(slot)
        if _inside(spans, source_target):
            emit(compact_slot, model_base + model_rel(source_target))
            continue
        row = _owner(pf.objects, source_target)
        if row is None:
            raise BattlePackError(
                "%s retained Model target 0x%x has no owner" %
                (fighter, source_target))
        if row.type_name != "Gfx":
            raise BattlePackError(
                "%s retained Model slot 0x%x targets pruned %s %s" %
                (fighter, slot, row.type_name, row.symbol))
        emit(compact_slot, root_target(source_target))

    spans_out = [(0, 0, len(main))]
    model_span_first = len(spans_out)
    rel = 0
    for start, end in spans:
        spans_out.append((start, rel, end - start))
        rel += end - start
    tail_first = []
    for body in tail_bodies:
        tail_first.append(len(spans_out))
        spans_out.append((0, 0, len(body)))

    sections = [
        (main_id, data_offsets[0], len(bodies[0]), len(main), 0, 1, 0, 0),
        (model_id, data_offsets[1], len(bodies[1]), len(model),
         model_span_first, len(spans), model_raw_bytes, len(roots)),
    ]
    for index, row in enumerate(tail_rows):
        sections.append((int(row["fid"]), data_offsets[2 + index],
                         len(tail_bodies[index]), len(tail_bodies[index]),
                         tail_first[index], 1, 0, 0))

    fixup_bytes = b"".join(struct.pack(fpc.FIXUP_FMT, *pair)
                            for pair in fixups)
    span_bytes = b"".join(struct.pack(fpc.SPAN_FMT, *row)
                           for row in spans_out)
    header = struct.pack(
        fpc.HEADER_FMT, fpc.MAGIC, fpc.VERSION,
        64 + 32 * len(sections) + len(data) + len(fixup_bytes) + len(span_bytes),
        fkind, len(sections), len(fixups), len(spans_out), 0,
        len(data), fpc.fnv1a32(bytes(data)), fpc.fnv1a32(fixup_bytes),
        fpc.fnv1a32(span_bytes), main_id, model_id, 0, 0)
    blob = (header +
            b"".join(struct.pack(fpc.SECTION_FMT, *row) for row in sections) +
            bytes(data) + fixup_bytes + span_bytes)
    fpc.decode_pack(blob)

    allocation = (len(data) + len(sections) * struct.calcsize(fpc.SECTION_FMT) +
                  len(spans_out) * struct.calcsize(fpc.SPAN_FMT))
    report = {
        "fighter": fighter,
        "fkind": fkind,
        "main_asset": main_id,
        "model_asset": model_id,
        "file_bytes": len(blob),
        "resident_allocation": allocation,
        "model_structural_bytes": model_raw_bytes,
        "model_spans": len(spans),
        "root_cells": len(roots),
        "fixups": len(fixups),
        "external_patches": len(external_patches),
    }
    return blob, report, external_patches


def _shield_asset_map() -> dict[int, int]:
    path = ROOT / "docs" / "optimization" / "NDS_SHIELD_POSE_ASSETS.generated.json"
    if not path.is_file():
        return {}
    doc = json.loads(path.read_text(encoding="utf-8"))
    return {int(row["main_asset"]): int(row["shield_asset"])
            for row in doc.get("fighters", [])}


def generate(output_dir: pathlib.Path, kinds: list[str]):
    output_dir.mkdir(parents=True, exist_ok=True)
    source_dir = output_dir / "source-metadata"
    result = preview.generate(source_dir, kinds)
    if not result["ok"]:
        raise BattlePackError("preview source metadata is not closed")

    types = est.TypeTable()
    types.load_dirs(est.HEADER_DIRS)
    shield_by_main = _shield_asset_map()
    reports = []
    patch_rows = []
    for kind in kinds:
        meta = json.loads((source_dir / (kind + "_compact_map.json")).read_text(
            encoding="utf-8"))
        blob, report, patches = _build_one(kind, meta, types, shield_by_main)
        (output_dir / ("%02d.fpc" % report["fkind"])).write_bytes(blob)
        if any(not (0 <= value <= 0xFFFF)
               for row in patches for value in row):
            raise BattlePackError(
                "%s external patch escaped u16 storage" % report["fighter"])
        ext = (struct.pack(EXTERN_HEADER_FMT, EXTERN_MAGIC, EXTERN_VERSION,
                           len(patches)) +
               b"".join(struct.pack(EXTERN_ROW_FMT, *row) for row in patches))
        (output_dir / ("%02d.ext" % report["fkind"])).write_bytes(ext)
        report["external_file_bytes"] = len(ext)
        reports.append(report)
        for slot, dep, target in patches:
            patch_rows.append({
                "fkind": report["fkind"],
                "main_asset": report["main_asset"],
                "slot": slot,
                "dep_asset": dep,
                "target_offset": target,
            })

    manifest = {
        "format": "FPC1-battle-low",
        "fighters": reports,
        "external_patches": patch_rows,
    }
    (output_dir / "battle_core_manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return manifest


def _write_shared(manifest: dict) -> None:
    if len(manifest["fighters"]) != len(fpc.KIND_ORDER):
        raise BattlePackError(
            "shared battle-core outputs require the complete 12-kind roster")
    SHARED_MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    SHARED_MANIFEST.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8", newline="\n")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="Generate LOW-detail VSBattle FPC1 packs")
    ap.add_argument("--output-dir", required=True)
    ap.add_argument("--kinds", default="donkey,samus,link,kirby",
                    help="comma-separated base fighter names")
    ap.add_argument("--emit-shared", action="store_true",
                    help="also refresh the complete-roster generated manifest")
    args = ap.parse_args(argv)
    kinds = fpc.parse_kinds(args.kinds)
    try:
        manifest = generate(pathlib.Path(args.output_dir), kinds)
        if args.emit_shared:
            _write_shared(manifest)
    except (BattlePackError, fpc.PackError, OSError, KeyError, ValueError,
            struct.error) as exc:
        print("BATTLE_CORE_PACK_FAIL: %s" % exc)
        return 1
    for row in manifest["fighters"]:
        print("%02d.fpc %-8s file=%d resident=%d model=%d spans=%d roots=%d "
              "fixups=%d extern=%d" %
              (row["fkind"], row["fighter"], row["file_bytes"],
               row["resident_allocation"], row["model_structural_bytes"],
               row["model_spans"], row["root_cells"], row["fixups"],
               row["external_patches"]))
    print("BATTLE_CORE_PACK_OK fighters=%d resident=%d extern=%d" %
          (len(manifest["fighters"]),
           sum(row["resident_allocation"] for row in manifest["fighters"]),
           len(manifest["external_patches"])))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
