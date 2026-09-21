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
import generate_nds_native_owners as native  # noqa: E402
import preview_source_metadata as preview  # noqa: E402


DISPLAY = {
    "mario": "Mario", "fox": "Fox", "donkey": "Donkey", "samus": "Samus",
    "luigi": "Luigi", "link": "Link", "yoshi": "Yoshi",
    "captain": "Captain", "kirby": "Kirby", "pikachu": "Pikachu",
    "purin": "Purin", "ness": "Ness",
}

SHARED_MANIFEST = ROOT / "docs" / "optimization" / "archive" / "NDS_BATTLE_CORE_PACKS.generated.json"
EXTERN_MAGIC = 0x31584542  # "BEX1" little-endian
EXTERN_VERSION = 2
EXTERN_HEADER_FMT = "<IHHIIII"
EXTERN_ROW_FMT = "<HHH"
FOREIGN_ROW_FMT = "<HHIII"


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


def _is_geometry(row) -> bool:
    # Gfx* arrays are live structural pointer tables (Yoshi's pre/post pairs),
    # not display-list commands. Only the pointed-to geometry becomes cells.
    return row.pointer_depth == 0 and row.type_name in ("Gfx", "Vtx")


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


def _gfx_texture_closure(pf, model_id: int, roots, files=None):
    """Retain IMAGE payload owners while traversing source Gfx pointer edges.

    Both texels and TLUTs are introduced by G_SETTIMG (0xFD). Native geometry
    replaces Gfx/Vtx, but the texture binder still reads these source bytes.
    Follow child lists without retaining their command/vertex storage. Keep
    complete typed image declarations, including interior aliases, so source
    tile widths/strides and palette reads retain their actual allocation.
    """
    files = {model_id: pf} if files is None else files
    queue = collections.deque((model_id, root) for root in roots)
    visited = set()
    images = {}
    image_offsets = set()
    while queue:
        source_id, root = queue.popleft()
        source_file = files[source_id]
        payload = source_file.source["payload"]
        pointers = source_file.source["pointers"]
        row = _owner(source_file.objects, root)
        if row is None or row.type_name != "Gfx" or row.pointer_depth != 0:
            raise BattlePackError("texture closure root %#x is not Gfx" % root)
        identity = (source_id, row.offset, row.size)
        if identity in visited:
            continue
        visited.add(identity)
        for slot, (dep, target) in pointers.items():
            if not (row.offset <= slot < row.offset + row.size):
                continue
            if (slot - row.offset) % 8 != 4:
                raise BattlePackError("Gfx pointer is not a command second word")
            opcode = payload[slot - 4]
            if opcode not in (0xDE, 0xFD):
                continue  # Vertices/lights have native generated owners.
            if dep not in files:
                raise BattlePackError(
                    "Model Gfx %#x escapes to texture/list asset %d" %
                    (root, dep))
            target_file = files[dep]
            target_row = _owner(target_file.objects, target)
            if target_row is None:
                raise BattlePackError("Gfx target %#x has no typed owner" % target)
            if opcode == 0xDE:
                if target_row.type_name != "Gfx" or target_row.pointer_depth != 0:
                    raise BattlePackError("G_DL target is not a source Gfx list")
                queue.append((dep, target))
                continue
            # An O2R pointer occupies the second word of a Gfx command.
            if opcode != 0xFD:
                continue
            if target_row.type_name in ("Gfx", "Vtx") or target_row.pointer_depth != 0:
                raise BattlePackError(
                    "G_SETTIMG %#x targets non-image %s %s" %
                    (slot - 4, target_row.type_name, target_row.symbol))
            if target_row.offset + target_row.size > len(target_file.source["payload"]):
                raise BattlePackError("image declaration escapes Model")
            if (target_row.offset | target_row.size) & 3:
                raise BattlePackError("image declaration is not word aligned")
            images[(dep, target_row.offset, target_row.size)] = target_row
            image_offsets.add((dep, target))
    return images, tuple(sorted(image_offsets))


def _foreign_texture_bank(images, files, model_id):
    """Private source spans, never registered as another fighter's Model."""
    data = bytearray()
    records = []
    for asset in sorted({key[0] for key in images if key[0] != model_id}):
        if asset >= 0xFFFF:
            raise BattlePackError("foreign image asset does not fit native provenance")
        spans = _merge_spans([row for key, row in images.items() if key[0] == asset])
        source = files[asset].source["payload"]
        for start, end in spans:
            records.append((asset, 0, start, len(data), end - start))
            data.extend(source[start:end])
    return records, bytes(data)


# Model-owned weapon lists whose own G_SETTIMG names Model texels. They are
# not fighter programs, so nothing else would keep those image rows resident.
WEAPON_TEXTURE_ROOTS = {"samus": (0xE0D8,),  # Bomb: CI4 0xDF88
                        "ness": (0x8F98,)}   # PK Thunder tail effect: IA8 0x8B58

_SKELETON_CONTEXTS = None


def _skeleton_contexts():
    global _SKELETON_CONTEXTS
    if _SKELETON_CONTEXTS is None:
        import native_skeletons
        _SKELETON_CONTEXTS = native_skeletons.contexts(ROOT)
    return _SKELETON_CONTEXTS


def _native_texture_roots(fighter: str, model_id: int):
    """Use the admitted native programs, including both detail/hat variants."""
    owner = fighter.lower()
    roots = set()
    images = set()
    programs = []
    for detail in ("high", "low"):
        if owner in ("mario", "fox"):
            context = native.build_owner_source_context(ROOT, detail)
            programs.append((detail, context, context[owner + "_roots"]))
        else:
            context = native.build_p2_owner_runtime_context(ROOT, owner, detail)
            programs.append((detail, context, context["roots"]))
    # Electric bodies set their own G_SETTIMG; without these spans the compact
    # Model resolves a null image and the whole fighter draw rejects.
    for (name, detail), context in _skeleton_contexts().items():
        if detail == "high" and name.startswith(owner + "_skeleton"):
            programs.append((None, context, context["roots"]))
    roots.update(WEAPON_TEXTURE_ROOTS.get(owner, ()))
    for detail, context, rows in programs:
        aliases = context.get("runtime_root_aliases", {})
        roots.update(aliases.get(row[0], row[0]) for row in rows)
        for root in rows:
            sequences = []
            for epoch in context["epochs"][root[1]:root[1] + root[4]]:
                sequences.extend(context["sequence"][epoch[0]:epoch[0] + epoch[4]])
                sequences.extend(context["sequence"][epoch[1]:epoch[1] + epoch[5]])
            if root[5]:
                sequences.extend(context["sequence"][root[2]:root[2] + root[5]])
            for index in sequences:
                delta = context["state"][index]
                if delta[2] == 6:
                    asset = delta[3] - 1 if len(delta) > 3 and delta[3] else model_id
                    images.add((asset, delta[1]))
        if owner == "kirby" and detail is not None:
            # Deferred copy-hat images are emitted separately from the body.
            # Their source roots come from the same native variant contract.
            variants = native.P2_MODEL_PART_ROOT_VARIANTS["kirby"][detail]
            roots.update(variants[modelpart - 1][1] for modelpart in
                         native.KIRBY_COPY_HAT_MODEL_PART_IDS)
    return roots, images


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
        if not _is_geometry(row):
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
            if _is_geometry(target_row):
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
            if _is_geometry(target_row):
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
        if _is_geometry(row):
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
            if _is_geometry(target_row):
                geometry_targets.add(target_offset)
                continue
            queue.append(target_row)

    # Gfx-only image references are not structural pointer edges. The native
    # state stream preserves their source offsets and the texture binder reads
    # their payload, so geometry removal must not prune those dependencies.
    texture_roots, emitted_images = _native_texture_roots(fighter, model_id)
    files = {item.file_id: item for item in idx.files}
    texture_rows, image_offsets = _gfx_texture_closure(
        pf, model_id, texture_roots, files)
    for (asset, _offset, _size), row in texture_rows.items():
        if asset == model_id:
            kept[(row.offset, row.symbol)] = row
    if not emitted_images.issubset(image_offsets):
        raise BattlePackError(
            "%s native IMAGE offsets escape source Gfx texture closure: %s" %
            (fighter, sorted(emitted_images.difference(image_offsets))))

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
        if not _is_geometry(row):
            continue
        if any(start < row.offset + row.size and row.offset < end
               for start, end in spans):
            raise BattlePackError(
                "%s structural span overlaps %s %s @ 0x%x" %
                (fighter, row.type_name, row.symbol, row.offset))

    foreign_rows, foreign_data = _foreign_texture_bank(texture_rows, files, model_id)
    return pf, spans, geometry_targets, image_offsets, foreign_rows, foreign_data


def _build_one(kind: str, meta: dict, types: est.TypeTable,
               shield_by_main: dict[int, int]):
    fighter = DISPLAY[kind]
    fkind = fpc.KIND_ORDER.index(kind)
    main_id, model_id = _asset_ids(meta)
    main = _source_payload(main_id)
    model = _source_payload(model_id)
    pf, spans, geometry_targets, image_offsets, foreign_rows, foreign_data = _structural_model_closure(
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
        if _is_geometry(row) and row.type_name == "Gfx":
            root_targets.add(target)
        elif _is_geometry(row) and row.type_name == "Vtx":
            raise BattlePackError(
                "%s Main points directly at Vtx 0x%x (%s)" %
                (fighter, target, row.symbol))
        else:
            raise BattlePackError(
                "%s Main structural target 0x%x (%s) escaped closure" %
                (fighter, target, row.symbol))

    compact_model = b"".join(model[start:end] for start, end in spans)
    for asset, offset in image_offsets:
        if asset != model_id:
            continue
        row = _owner(pf.objects, offset)
        mapped = model_rel(row.offset)
        if compact_model[mapped:mapped + row.size] != model[row.offset:row.offset + row.size]:
            raise BattlePackError("%s image payload lost source bytes" % fighter)
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
        if row.type_name != "Gfx" or row.pointer_depth != 0:
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
        fpc.fnv1a32(span_bytes), main_id, model_id, len(model), 0)
    blob = (header +
            b"".join(struct.pack(fpc.SECTION_FMT, *row) for row in sections) +
            bytes(data) + fixup_bytes + span_bytes)
    fpc.decode_pack(blob)

    allocation = (len(data) + len(sections) * struct.calcsize(fpc.SECTION_FMT) +
                  len(spans_out) * struct.calcsize(fpc.SPAN_FMT))
    foreign_allocation = _align(len(foreign_data) +
                                len(foreign_rows) * struct.calcsize(FOREIGN_ROW_FMT))
    report = {
        "fighter": fighter,
        "fkind": fkind,
        "main_asset": main_id,
        "model_asset": model_id,
        "file_bytes": len(blob),
        "resident_allocation": allocation,
        "model_structural_bytes": model_raw_bytes,
        "model_spans": len(spans),
        "texture_images": [{"asset": asset, "offset": offset}
                           for asset, offset in image_offsets],
        "foreign_texture_spans": len(foreign_rows),
        "foreign_texture_bytes": len(foreign_data),
        "foreign_texture_map_bytes": len(foreign_rows) * struct.calcsize(FOREIGN_ROW_FMT),
        "foreign_texture_allocation": foreign_allocation,
        "resident_with_foreign_textures": allocation + foreign_allocation,
        "root_cells": len(roots),
        "fixups": len(fixups),
        "external_patches": len(external_patches),
    }
    return blob, report, external_patches, foreign_rows, foreign_data


def _shield_asset_map() -> dict[int, int]:
    path = ROOT / "docs" / "optimization" / "archive" / "NDS_SHIELD_POSE_ASSETS.generated.json"
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
        blob, report, patches, foreign_rows, foreign_data = _build_one(
            kind, meta, types, shield_by_main)
        (output_dir / ("%02d.fpc" % report["fkind"])).write_bytes(blob)
        if any(not (0 <= value <= 0xFFFF)
               for row in patches for value in row):
            raise BattlePackError(
                "%s external patch escaped u16 storage" % report["fighter"])
        foreign_map = b"".join(struct.pack(FOREIGN_ROW_FMT, *row) for row in foreign_rows)
        foreign_hash = fpc.fnv1a32(foreign_map + foreign_data)
        ext = (struct.pack(EXTERN_HEADER_FMT, EXTERN_MAGIC, EXTERN_VERSION,
                           len(patches), len(foreign_rows), len(foreign_data),
                           foreign_hash, 0) +
               b"".join(struct.pack(EXTERN_ROW_FMT, *row) for row in patches) +
               foreign_map + foreign_data)
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
    print("BATTLE_CORE_PACK_OK fighters=%d resident=%d foreign=%d total=%d extern=%d" %
          (len(manifest["fighters"]),
           sum(row["resident_allocation"] for row in manifest["fighters"]),
           sum(row["foreign_texture_allocation"] for row in manifest["fighters"]),
           sum(row["resident_with_foreign_textures"] for row in manifest["fighters"]),
           len(manifest["external_patches"])))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
