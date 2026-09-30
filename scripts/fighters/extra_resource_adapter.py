#!/usr/bin/env python3
"""Inspect raw EXTRA resources at the existing native geometry input boundary.

Raw donor files carry the same threaded pointer words as O2R payloads. They
lack the O2R header and resolved asset identities. This adapter validates those
words, preserves symbolic/legacy dependencies and identifies typed model roots;
it never assigns DS file IDs or enables a fighter. Runtime metadata and complete
native owner compilation remain separate, explicit conversion work.

Sources: EXTRA config.yaml/ImportFile.validate_reqlist/rom_util.get_attrib_offset;
BattleShip fttypes.h FTAttributes, FTCommonPartContainer, FTHiddenPart;
sys/objtypes.h DObjDesc and MObjSub; native stage O2RResource/PointerRef.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
import math
import os
from pathlib import Path
import re
import struct
import sys
from typing import Mapping


SENTINEL = 0xFFFF * 4
FTATTR_SIZE = 0x348
DOBJDESC_SIZE = 44
MOBJSUB_SIZE = 0x78
COMMON_JOINT_START = 4
MAX_SOURCE_JOINTS = 64
CONVERSION_GAP = (
    "Typed source roots and pointer domains are available. No DS asset IDs are "
    "assigned. The fighter owner compiler still obtains resource payloads, "
    "joint metadata and expected census through P2-specific dictionaries; it "
    "needs a typed input provider before these roots produce native owner IR. "
    "Resolved callbacks/actions, inherited movesets, shield poses, costume "
    "semantics and reached hidden/modelpart domains remain unqualified."
)


class AdapterError(ValueError):
    """Malformed or semantically unresolved source input; never a warning."""


@dataclass(frozen=True)
class Dependency:
    kind: str
    name: str | None = None
    file_id: int | None = None
    label: str = ""

    def inventory(self) -> dict:
        if self.kind == "symbol":
            return {"kind": self.kind, "name": self.name}
        return {"kind": self.kind, "file_id": self.file_id, "label": self.label}


@dataclass(frozen=True)
class Pointer:
    offset: int
    resource: str | None = None
    dependency: Dependency | None = None

    def inventory(self) -> dict:
        result = {"offset": self.offset}
        if self.resource is not None:
            result["resource"] = self.resource
        if self.dependency is not None:
            result["dependency"] = self.dependency.inventory()
        return result


def parse_requests(text: str) -> tuple[Dependency, ...]:
    """Preserve compiled IDs or uncompiled symbols in donor request order."""
    rows = []
    ended = False
    for number, raw in enumerate(text.splitlines(), 1):
        line = raw.strip()
        if not line:
            continue
        if line.upper().startswith("END OF"):
            ended = True
            continue
        if ended:
            raise AdapterError(f"request row {number}: content after terminator")
        symbol = re.fullmatch(r"\$\{([A-Z][A-Z0-9_]*)\}", line)
        numeric = re.fullmatch(r"([0-9A-Fa-f]{4})(?:\s+(.+))?", line)
        if symbol:
            rows.append(Dependency("symbol", name=symbol[1]))
        elif numeric:
            rows.append(Dependency("donor_file", file_id=int(numeric[1], 16),
                                   label=numeric[2] or ""))
        else:
            raise AdapterError(f"request row {number}: unsupported dependency {line!r}")
    return tuple(rows)


def _span(payload: bytes, offset: int, size: int, label: str,
          alignment: int = 4) -> None:
    if (offset < 0 or size < 0 or offset % alignment or
            offset > len(payload) or size > len(payload) - offset):
        raise AdapterError(f"{label}: unaligned or out-of-bounds span {offset:#x}+{size:#x}")


@dataclass(frozen=True)
class RawResource:
    name: str
    payload: bytes
    internal_head: int
    external_head: int
    internal: Mapping[int, Pointer]
    external: Mapping[int, Pointer]

    def pointer(self, slot: int) -> Pointer | None:
        _span(self.payload, slot, 4, f"{self.name} pointer")
        pointer = self.internal.get(slot) or self.external.get(slot)
        if pointer is not None:
            return pointer  # A relocation to offset zero is NOT a null pointer.
        if struct.unpack_from(">I", self.payload, slot)[0] == 0:
            return None
        raise AdapterError(f"{self.name} pointer {slot:#x}: nonzero word lacks relocation")

    def inventory(self) -> dict:
        return {
            "name": self.name, "bytes": len(self.payload),
            "sha256": hashlib.sha256(self.payload).hexdigest(),
            "internal_head": self.internal_head, "external_head": self.external_head,
            "internal": [{"slot": slot, **ptr.inventory()}
                         for slot, ptr in sorted(self.internal.items())],
            "external": [{"slot": slot, **ptr.inventory()}
                         for slot, ptr in sorted(self.external.items())],
        }

    def as_native_resource(self, asset_id: int, dependency_ids: Mapping[Dependency, int],
                           native_stage_module):
        """Bridge to existing O2RResource only with caller-resolved asset IDs.

        This is an in-memory input bridge, not a second runtime loader or a
        fabricated O2R file. Unresolved dependency IDs fail before conversion.
        """
        ids = [asset_id, *dependency_ids.values()]
        if any(type(value) is not int or not 0 <= value < 0xFFFF for value in ids):
            raise AdapterError("native bridge: asset IDs must be explicit u16 values")
        internal = {slot: native_stage_module.PointerRef(asset_id, ptr.offset)
                    for slot, ptr in self.internal.items()}
        external = {}
        for slot, ptr in self.external.items():
            if ptr.dependency not in dependency_ids:
                raise AdapterError(f"native bridge: unresolved dependency at {slot:#x}")
            external[slot] = native_stage_module.PointerRef(
                dependency_ids[ptr.dependency], ptr.offset)
        spec = native_stage_module.InputSpec(
            self.name, hashlib.sha256(self.payload).hexdigest(), asset_id,
            len(internal), len(external), hashlib.sha256(self.payload).hexdigest())
        return native_stage_module.O2RResource(
            spec, self.payload, self.payload, asset_id, internal, external)


def decode_resource(name: str, payload: bytes, internal_head: int,
                    external_head: int, requests=(), *, local_dependency_ids=None) -> RawResource:
    """Decode byte-addressed heads; encoded next/target fields are word indices."""
    if not isinstance(payload, bytes) or not payload or len(payload) % 4:
        raise AdapterError(f"{name}: raw payload must contain whole 32-bit words")
    requests = tuple(requests)
    local_dependency_ids = dict(local_dependency_ids or {})
    if any(type(key) is not int or not 0 <= key < 0xffff or not isinstance(value, str)
           for key, value in local_dependency_ids.items()):
        raise AdapterError("local dependency identities must be explicit u16-to-name bindings")
    if any(not isinstance(row, Dependency) for row in requests):
        raise AdapterError(f"{name}: request rows must be parsed dependencies")

    def walk(head: int, external: bool) -> dict[int, Pointer]:
        result = {}
        cursor = head
        while cursor != SENTINEL:
            _span(payload, cursor, 4, f"{name} relocation head/next")
            if cursor in result:
                raise AdapterError(f"{name}: relocation cycle at {cursor:#x}")
            next_word, target_word = struct.unpack_from(">HH", payload, cursor)
            target = target_word * 4
            if external:
                if len(result) >= len(requests):
                    raise AdapterError(f"{name}: external chain exceeds request cardinality")
                dependency = requests[len(result)]
                result[cursor] = Pointer(target, resource=local_dependency_ids.get(dependency.file_id),
                                         dependency=dependency)
            else:
                _span(payload, target, 1, f"{name} internal target")
                result[cursor] = Pointer(target, resource=name)
            cursor = next_word * 4
        if external and len(result) != len(requests):
            raise AdapterError(f"{name}: external chain/request cardinality mismatch")
        return result

    internal = walk(internal_head, False)
    external = walk(external_head, True)
    if internal.keys() & external.keys():
        raise AdapterError(f"{name}: internal/external relocation chains overlap")
    return RawResource(name, payload, internal_head, external_head, internal, external)


def _config_heads(config: str) -> dict[str, tuple[int, int]]:
    match = re.search(r"(?m)^offsets:\s*\n((?:[ \t]+[^\n]*\n?)+)", config)
    if match is None:
        raise AdapterError("config: absent offsets mapping")
    result = {}
    for name in ("main", "character"):
        rows = re.findall(
            rf'(?m)^\s+{name}:\s*\["([0-9A-Fa-f]+)",\s*"([0-9A-Fa-f]+)"\]\s*$',
            match[1])
        if len(rows) != 1:
            raise AdapterError(f"config: exactly one {name} relocation-head pair required")
        result[name] = tuple(int(value, 16) for value in rows[0])
    return result


def _local(ptr: Pointer | None, resources: Mapping[str, RawResource],
           label: str) -> tuple[RawResource, int]:
    if ptr is None:
        raise AdapterError(f"{label}: required pointer is null")
    name = ptr.resource
    if ptr.dependency is not None and ptr.dependency.kind == "symbol":
        name = ptr.dependency.name
    if name not in resources:
        raise AdapterError(f"{label}: required resource is unresolved {ptr.inventory()}")
    resource = resources[name]
    _span(resource.payload, ptr.offset, 1, label)
    return resource, ptr.offset


def _pointer_inventory(ptr: Pointer | None, resources: Mapping[str, RawResource]) -> dict | None:
    if ptr is None:
        return None
    if ptr.resource is not None:
        return ptr.inventory()
    if ptr.dependency.kind == "symbol" and ptr.dependency.name in resources:
        return {"resource": ptr.dependency.name, "offset": ptr.offset}
    return ptr.inventory()


def _read_tree(resource: RawResource, offset: int) -> list[dict]:
    rows = []
    for index in range(MAX_SOURCE_JOINTS + 1):
        pos = offset + index * DOBJDESC_SIZE
        _span(resource.payload, pos, DOBJDESC_SIZE, "DObjDesc")
        depth = struct.unpack_from(">I", resource.payload, pos)[0]
        values = struct.unpack_from(">9f", resource.payload, pos + 8)
        if not all(math.isfinite(value) for value in values):
            raise AdapterError("DObjDesc: non-finite source transform")
        ptr = resource.pointer(pos + 4)
        if depth == 18:
            if ptr is not None:
                raise AdapterError("DObjDesc: non-null depth-18 sentinel")
        elif (depth & ~0x8000) >= 18:
            raise AdapterError(f"DObjDesc: unsupported depth/flags {depth:#x}")
        rows.append({"index": index, "offset": pos, "depth": depth,
                     "display_list": None if ptr is None else ptr.inventory(),
                     "translate": list(values[:3]), "rotate": list(values[3:6]),
                     "scale": list(values[6:])})
        if depth == 18:
            return rows
    raise AdapterError("DObjDesc: missing sentinel within setup-parts domain")


def _display_root(resource: RawResource, offset: int) -> dict:
    """Inventory the source grammar; this is not native geometry acceptance."""
    ops = {}
    branches = []
    image_refs = []
    vertex_refs = []
    triangles = 0
    for index in range(256):
        pos = offset + index * 8
        _span(resource.payload, pos, 8, "Gfx")
        w0, w1 = struct.unpack_from(">II", resource.payload, pos)
        op = w0 >> 24
        ops[op] = ops.get(op, 0) + 1
        if op in (0x01, 0xFD):
            ptr = resource.pointer(pos + 4)
            if ptr is None or ptr.resource != resource.name:
                raise AdapterError(f"Gfx {pos:#x}: unresolved image/vertex relocation")
            if op == 0x01:
                count = (w0 >> 12) & 0xFF
                end = (w0 >> 1) & 0x7F
                if count == 0 or end > 32 or end < count:
                    raise AdapterError(f"Gfx {pos:#x}: invalid vertex span")
                _span(resource.payload, ptr.offset, count * 16, "Vtx")
                vertex_refs.append({"slot": pos + 4, "count": count, **ptr.inventory()})
            else:
                image_refs.append({"slot": pos + 4, **ptr.inventory()})
        elif op == 0xDE:
            ptr = resource.internal.get(pos + 4) or resource.external.get(pos + 4)
            if ptr is not None:
                branches.append({"slot": pos + 4, **ptr.inventory()})
            elif w1 >> 24 == 0x0E and (w1 & 7) == 0:
                branches.append({"slot": pos + 4, "material_slot": (w1 & 0xFFFFFF) // 8})
            else:
                raise AdapterError(f"Gfx {pos:#x}: unresolved display-list branch")
        elif op == 0x05:
            triangles += 1
        elif op == 0x06:
            triangles += 2
        elif op == 0xDF:
            return {"resource": resource.name, "offset": offset,
                    "command_count": index + 1, "direct_triangles": triangles,
                    "opcodes": [{"opcode": op, "count": count}
                                for op, count in sorted(ops.items())],
                    "branches": branches, "images": image_refs, "vertices": vertex_refs}
    raise AdapterError(f"Gfx {offset:#x}: missing end within native root command limit")


def _material_chain(ptr: Pointer | None, resources: Mapping[str, RawResource]) -> dict:
    if ptr is None:
        return {"materials": []}
    chain, base = _local(ptr, resources, "MObjSub pointer chain")
    material_refs = []
    for entry in range(64):
        ptr = chain.pointer(base + entry * 4)
        if ptr is None:
            return {"materials": material_refs}
        owner, offset = _local(ptr, resources, "MObjSub")
        _span(owner.payload, offset, MOBJSUB_SIZE, "MObjSub")
        material_refs.append({"type": "MObjSub", "bytes": MOBJSUB_SIZE,
                              "resource": owner.name, "offset": offset})
    raise AdapterError("MObjSub pointer chain: missing null terminator")


def _part_materials(dispatch: Pointer | None, index: int,
                    resources: Mapping[str, RawResource]) -> dict:
    if dispatch is None:
        return {"materials": []}
    table, base = _local(dispatch, resources, "material dispatch")
    return _material_chain(table.pointer(base + index * 4), resources)


def _modelpart_roots(main: RawResource, attrs: int, descriptor_count: int,
                     resources: Mapping[str, RawResource]) -> dict:
    """Bound passive alternatives by referenced source-object boundaries.

    The reachable source joint domain comes from the JointTree, not a guessed
    global modelpart count. Report span provenance: resolved actions must still
    establish which alternatives can execute and how they inherit materials.
    """
    table_ptr = main.pointer(attrs + 0x328)
    if table_ptr is None:
        return {"records": [], "roots": {"high": [], "low": []}}
    table, base = _local(table_ptr, resources, "FTModelPartContainer")
    _span(table.payload, base, descriptor_count * 4, "FTModelPartContainer")
    boundaries = sorted({ptr.offset for ptr in table.internal.values()})
    records = []
    roots = {"high": [], "low": []}
    for index in range(descriptor_count):
        ptr = table.pointer(base + index * 4)
        if ptr is None:
            continue
        owner, start = _local(ptr, resources, "FTModelPartDesc")
        ends = [offset for offset in boundaries if offset > start]
        if owner is not table or not ends:
            raise AdapterError("FTModelPartDesc: missing local bounded source span")
        end = min(ends)
        if (end - start) % 40:
            raise AdapterError("FTModelPartDesc: span is not whole high/low modelpart pairs")
        for modelpart in range((end - start) // 40):
            for detail_index, detail in enumerate(("high", "low")):
                pos = start + (modelpart * 2 + detail_index) * 20
                pointers = [owner.pointer(pos + field * 4) for field in range(4)]
                row = {"type": "FTModelPart", "resource": owner.name, "offset": pos,
                       "bytes": 20, "joint_id": index + COMMON_JOINT_START,
                       "modelpart_id": modelpart, "detail": detail,
                       "display_list": _pointer_inventory(pointers[0], resources),
                       "mobjsubs": _pointer_inventory(pointers[1], resources),
                       "costume_matanim_joints": _pointer_inventory(pointers[2], resources),
                       "main_matanim_joints": _pointer_inventory(pointers[3], resources),
                       "flags": owner.payload[pos + 16]}
                records.append(row)
                if pointers[0] is not None:
                    dl_owner, dl_offset = _local(pointers[0], resources, "FTModelPart Gfx")
                    if row["flags"] & 0xF:
                        raise AdapterError("FTModelPart: custom display mode needs explicit provider")
                    roots[detail].append({"joint_id": row["joint_id"],
                                          "modelpart_id": modelpart,
                                          **_display_root(dl_owner, dl_offset),
                                          **_material_chain(pointers[1], resources)})
    return {"offset": base, "records": records, "roots": roots,
            "span_provenance": "pairs bounded by next referenced local object; action reachability remains unresolved"}


def _accesspart_root(main: RawResource, attrs: int,
                      resources: Mapping[str, RawResource]) -> dict | None:
    ptr = main.pointer(attrs + 0x32C)
    if ptr is None:
        return None
    owner, pos = _local(ptr, resources, "FTAccessPart")
    _span(owner.payload, pos, 16, "FTAccessPart")
    joint = struct.unpack_from(">i", owner.payload, pos)[0]
    dl = owner.pointer(pos + 4)
    result = {"type": "FTAccessPart", "offset": pos, "bytes": 16, "joint_id": joint,
              "display_list": _pointer_inventory(dl, resources),
              "mobjsubs": _pointer_inventory(owner.pointer(pos + 8), resources),
              "costume_matanim_joints": _pointer_inventory(owner.pointer(pos + 12), resources)}
    if dl is not None:
        dl_owner, dl_offset = _local(dl, resources, "FTAccessPart Gfx")
        result["root"] = {**_display_root(dl_owner, dl_offset),
                          **_material_chain(owner.pointer(pos + 8), resources)}
    return result


def _stock_inventory(main: RawResource, attrs: int,
                     resources: Mapping[str, RawResource]) -> dict | None:
    ptr = main.pointer(attrs + 0x340)
    if ptr is None:
        return None
    owner, pos = _local(ptr, resources, "FTSprites")
    _span(owner.payload, pos, 12, "FTSprites")
    sprite_owner, sprite_offset = _local(owner.pointer(pos), resources, "stock Sprite")
    _span(sprite_owner.payload, sprite_offset, 68, "Sprite")
    width, height = struct.unpack_from(">hh", sprite_owner.payload, sprite_offset + 4)
    bitmap_count = struct.unpack_from(">h", sprite_owner.payload, sprite_offset + 40)[0]
    fmt, siz = sprite_owner.payload[sprite_offset + 48:sprite_offset + 50]
    if width <= 0 or height <= 0 or bitmap_count <= 0 or siz not in range(4):
        raise AdapterError("stock Sprite: invalid image domain")
    bitmaps_owner, bitmaps_offset = _local(
        sprite_owner.pointer(sprite_offset + 52), resources, "stock Bitmap")
    bitmaps = []
    for index in range(bitmap_count):
        offset = bitmaps_offset + index * 16
        _span(bitmaps_owner.payload, offset, 16, "Bitmap")
        bitmap_width, width_img, s, t = struct.unpack_from(">4h", bitmaps_owner.payload, offset)
        actual_height, lut_offset = struct.unpack_from(">2H", bitmaps_owner.payload, offset + 12)
        if width_img <= 0 or actual_height <= 0:
            raise AdapterError("stock Bitmap: invalid source stride/height")
        pixels_owner, pixels_offset = _local(bitmaps_owner.pointer(offset + 8), resources, "stock bitmap pixels")
        pixel_bytes = (width_img * actual_height * (4 << siz) + 7) // 8
        _span(pixels_owner.payload, pixels_offset, pixel_bytes, "stock bitmap pixels")
        bitmaps.append({"type": "Bitmap", "resource": bitmaps_owner.name,
                        "offset": offset, "bytes": 16, "width": bitmap_width,
                        "width_img": width_img, "s": s, "t": t,
                        "actual_height": actual_height, "lut_offset": lut_offset,
                        "pixels": {"resource": pixels_owner.name, "offset": pixels_offset,
                                   "bytes": pixel_bytes}})
    luts_owner, luts_offset = _local(owner.pointer(pos + 4), resources, "stock LUT table")
    boundaries = [ref.offset for ref in luts_owner.internal.values()
                  if ref.offset > luts_offset]
    if not boundaries:
        raise AdapterError("stock LUT table: missing bounded source span")
    luts_end = min(boundaries)
    if (luts_end - luts_offset) % 4:
        raise AdapterError("stock LUT table: unaligned source span")
    palettes = []
    for index, slot in enumerate(range(luts_offset, luts_end, 4)):
        palette_owner, palette_offset = _local(luts_owner.pointer(slot), resources, "stock LUT")
        required_bytes = 2 * (16 if siz == 0 else 256) if fmt == 2 and siz <= 1 else None
        if required_bytes is not None:
            _span(palette_owner.payload, palette_offset, required_bytes, "stock indexed palette")
        palettes.append({"index": index, "slot_resource": luts_owner.name, "slot": slot,
                         "resource": palette_owner.name, "offset": palette_offset,
                         "required_indexed_bytes": required_bytes})
    return {"type": "FTSprites", "resource": owner.name, "offset": pos, "bytes": 12,
            "stock_sprite": {"type": "Sprite", "resource": sprite_owner.name,
                             "offset": sprite_offset, "bytes": 68, "width": width, "height": height,
                             "attr": struct.unpack_from(">H", sprite_owner.payload, sprite_offset + 20)[0],
                             "fmt": fmt, "siz": siz,
                             "start_tlut": struct.unpack_from(">h", sprite_owner.payload, sprite_offset + 28)[0],
                             "declared_n_tlut": struct.unpack_from(">h", sprite_owner.payload, sprite_offset + 30)[0],
                             "lut": _pointer_inventory(sprite_owner.pointer(sprite_offset + 32), resources),
                             "bitmaps": bitmaps},
            "stock_luts": {"resource": luts_owner.name, "offset": luts_offset,
                           "end": luts_end, "palettes": palettes,
                           "span_provenance": "bounded by next referenced local object; costume mapping remains a source-resolution dependency"},
            "emblem": _pointer_inventory(owner.pointer(pos + 8), resources)}


def _skeleton_inventory(main: RawResource, attrs: int, descriptor_count: int,
                         selected: list[int], parts: dict,
                         resources: Mapping[str, RawResource]) -> dict:
    """Read the source skeleton selector and both exact FTSkeleton arrays."""
    selector_owner, selector = _local(main.pointer(attrs + 0x344), resources, "skeleton selector")
    _span(selector_owner.payload, selector, 12, "skeleton selector")
    gate = struct.unpack_from(">I", selector_owner.payload, selector)[0]
    gate_descriptor = gate - COMMON_JOINT_START
    if gate_descriptor not in selected or not 0 <= gate_descriptor < descriptor_count:
        raise AdapterError("skeleton selector: gate is outside selected source joints")
    if parts["high"]["descriptors"][gate_descriptor]["display_list"] is None:
        raise AdapterError("skeleton selector: source gate has no common display list")
    result = {"selector_offset": selector, "selector_resource": selector_owner.name,
              "gate_joint": gate, "variants": {}}
    for sid in (1, 2):
        table_owner, start = _local(selector_owner.pointer(selector + sid * 4), resources, "FTSkeleton table")
        _span(table_owner.payload, start, descriptor_count * 8, "FTSkeleton table")
        rows = []
        roots = []
        for index in range(descriptor_count):
            pos = start + index * 8
            ptr = table_owner.pointer(pos)
            flag_word = struct.unpack_from(">I", table_owner.payload, pos + 4)[0]
            flags = flag_word >> 24
            if flag_word & 0xFFFFFF or flags & 0x3F:
                raise AdapterError("FTSkeleton: unsupported pair/mode/flag or nonzero padding")
            row = {"type": "FTSkeleton", "resource": table_owner.name, "offset": pos,
                   "bytes": 8, "source_joint": index + COMMON_JOINT_START,
                   "flags": flags, "display_list": _pointer_inventory(ptr, resources)}
            rows.append(row)
            if ptr is None:
                continue
            if index not in selected:
                raise AdapterError("FTSkeleton: rendered root belongs to an unselected source joint")
            dl_owner, dl_offset = _local(ptr, resources, "FTSkeleton Gfx")
            chosen = "high"  # Skeleton root identity is detail-independent.
            # The source skeleton inherits the current DObj's material chain;
            # material callbacks may select it even with a skeleton override.
            material_by_detail = {
                detail: _part_materials(parts[detail]["material_dispatch"], index, resources)["materials"]
                for detail in ("high", "low")}
            root = {"joint_id": index + COMMON_JOINT_START, "descriptor_index": index,
                    "skeleton_id": sid, "skeleton_flags": flags,
                    **_display_root(dl_owner, dl_offset),
                    "materials": material_by_detail[chosen],
                    "materials_by_detail": material_by_detail}
            material_slots = sorted({branch["material_slot"] for branch in root["branches"]
                                     if "material_slot" in branch})
            root["required_material_slots"] = material_slots
            root["material_binding_by_detail"] = {
                detail: {"kind": "inherited" if material_slots and not materials else "own" if materials else "none",
                         "required_count": max(material_slots, default=-1) + 1,
                         "source_binder_joint": index + COMMON_JOINT_START if materials else None,
                         "provenance": "gcDrawMObjForDObj returns before segment-E update when the live DObj has no MObj"}
                for detail, materials in material_by_detail.items()}
            roots.append(root)
        if not roots:
            raise AdapterError("FTSkeleton: source variant has no rendered roots")
        result["variants"][str(sid)] = {"table_resource": table_owner.name,
                                       "table_offset": start, "rows": rows, "roots": roots}
    return result


def model_inventory(main: RawResource, character: RawResource,
                    attributes_offset: int | None = None) -> dict:
    resources = {main.name: main, character.name: character}
    if attributes_offset is None:
        # Same donor marker and displacement, but ambiguity fails closed.
        positions = [match.start() for match in re.finditer(b"\x00\x64\x00\x64", main.payload)]
        if len(positions) != 1:
            raise AdapterError("FTAttributes: marker ambiguous/absent; supply resolved offset")
        attributes_offset = positions[0] - 0xE4
    _span(main.payload, attributes_offset, FTATTR_SIZE, "FTAttributes")
    setup_resource, setup_offset = _local(main.pointer(attributes_offset + 0x29C),
                                          resources, "setup_parts")
    _span(setup_resource.payload, setup_offset, 8, "setup_parts")
    setup = list(struct.unpack_from(">II", setup_resource.payload, setup_offset))
    common_resource, common_offset = _local(main.pointer(attributes_offset + 0x2D4),
                                             resources, "FTCommonPartContainer")
    _span(common_resource.payload, common_offset, 32, "FTCommonPartContainer")
    parts = {}
    for detail_index, detail in enumerate(("high", "low")):
        pos = common_offset + detail_index * 16
        tree_resource, tree_offset = _local(common_resource.pointer(pos), resources, "JointTree")
        if tree_resource != character:
            raise AdapterError("JointTree: expected the named character resource")
        flags = common_resource.payload[pos + 12]
        if flags & 0xF:
            raise AdapterError("FTCommonPart: paired/custom display mode requires explicit provider")
        parts[detail] = {"tree_offset": tree_offset,
                         "descriptors": _read_tree(tree_resource, tree_offset),
                         "material_dispatch": common_resource.pointer(pos + 4),
                         "costume_dispatch": common_resource.pointer(pos + 8), "flags": flags}
    if len(parts["high"]["descriptors"]) != len(parts["low"]["descriptors"]):
        raise AdapterError("JointTree: high/low descriptor domains differ")
    count = len(parts["high"]["descriptors"]) - 1
    selected = [index for index in range(64)
                if setup[index // 32] & (1 << (31 - index % 32))]
    if any(index >= count for index in selected):
        raise AdapterError("setup_parts: selects beyond descriptor domain")

    hidden_resource, hidden_offset = _local(main.pointer(attributes_offset + 0x2D0),
                                            resources, "FTHiddenPart")
    boundaries = [ptr.offset for ptr in main.internal.values()
                  if ptr.resource == hidden_resource.name and ptr.offset > hidden_offset]
    if not boundaries:
        raise AdapterError("FTHiddenPart: no bounded source span")
    hidden_end = min(boundaries)
    if (hidden_end - hidden_offset) % 16:
        raise AdapterError("FTHiddenPart: bounded span is not whole records")
    hidden = []
    for index, pos in enumerate(range(hidden_offset, hidden_end, 16)):
        root, parent, part_index, joint_kind = struct.unpack_from(">4i", hidden_resource.payload, pos)
        if not 0 <= root < count + COMMON_JOINT_START or not 0 <= parent < MAX_SOURCE_JOINTS:
            raise AdapterError("FTHiddenPart: source joint reference outside descriptor domain")
        hidden.append({"index": index, "offset": pos, "joint_id": root,
                       "parent_joint_id": parent, "part_index": part_index,
                       "joint_kind": joint_kind, "zero_record": not any((root, parent, part_index, joint_kind))})

    skeletons = _skeleton_inventory(main, attributes_offset, count, selected, parts, resources)
    for detail, part in parts.items():
        roots = []
        hidden_roots = []
        for index in range(count):
            # Low NULL display lists use HIGH, while transforms stay LOW.
            ptr_info = part["descriptors"][index]["display_list"]
            chosen = detail if detail == "high" or ptr_info is not None else "high"
            ptr_info = parts[chosen]["descriptors"][index]["display_list"]
            if ptr_info is None:
                continue
            owner = resources[ptr_info["resource"]]
            root = {"descriptor_index": index, "joint_id": index + COMMON_JOINT_START,
                    **_display_root(owner, ptr_info["offset"]),
                    **_part_materials(parts[chosen]["material_dispatch"], index, resources)}
            if index in selected:
                roots.append(root)
            for hidden_row in hidden:
                if hidden_row["joint_id"] == index + COMMON_JOINT_START:
                    hidden_roots.append({"hidden_index": hidden_row["index"], **root})
        part["canonical_roots"] = roots
        part["hidden_roots"] = hidden_roots
        part["direct_triangles"] = sum(root["direct_triangles"] for root in roots)
        for key in ("material_dispatch", "costume_dispatch"):
            part[key] = _pointer_inventory(part[key], resources)
    pointer_fields = {name: _pointer_inventory(main.pointer(attributes_offset + offset), resources)
                      for name, offset in (("animlock", 0x2A0), ("hiddenparts", 0x2D0),
                                           ("commonparts", 0x2D4), ("dobj_lookup", 0x2D8),
                                           ("translate_scales", 0x324), ("modelparts", 0x328),
                                           ("accesspart", 0x32C), ("textureparts", 0x330),
                                           ("sprites", 0x340), ("skeleton", 0x344))}
    return {"attributes_offset": attributes_offset, "attributes_bytes": FTATTR_SIZE,
            "setup_parts": setup, "selected_descriptor_indices": selected,
            "canonical_live_nodes": len(selected) + 1, "details": parts,
            "pointer_fields": pointer_fields, "hidden_records": hidden,
            "modelparts": _modelpart_roots(main, attributes_offset, count, resources),
            "accesspart": _accesspart_root(main, attributes_offset, resources),
            "stock": _stock_inventory(main, attributes_offset, resources),
            "skeletons": skeletons,
            "hidden_span": {"offset": hidden_offset, "end": hidden_end,
                            "provenance": "bounded by the next referenced local object; exact reached hidden-index domain remains unresolved"},
            "conversion_gap": CONVERSION_GAP}


def build_meta_knight_inventory(source_dir: Path, attributes_offset: int | None = None,
                                *, source_resource_ids: Mapping[str, int] | None = None) -> dict:
    source_dir = Path(source_dir)
    config_bytes = (source_dir / "config.yaml").read_bytes()
    requests_bytes = (source_dir / "main_reqlist.txt").read_bytes()
    config = config_bytes.decode("utf-8")
    heads = _config_heads(config)
    source_resource_ids = dict(source_resource_ids or {})
    if source_resource_ids and (set(source_resource_ids) != {"MAIN", "CHARACTER"} or
                                len(set(source_resource_ids.values())) != 2 or
                                any(type(value) is not int or not 0 <= value < 0xffff
                                    for value in source_resource_ids.values())):
        raise AdapterError("resolved donor resources require distinct explicit MAIN/CHARACTER IDs")
    local_ids = {value: name for name, value in source_resource_ids.items()}
    requests = parse_requests(requests_bytes.decode("utf-8"))
    main = decode_resource("MAIN", (source_dir / "main.bin").read_bytes(), *heads["main"], requests,
                           local_dependency_ids=local_ids)
    character_requests_path = source_dir / "character_reqlist.txt"
    character_requests_bytes = character_requests_path.read_bytes() \
        if character_requests_path.exists() else None
    character_requests = parse_requests(character_requests_bytes.decode("utf-8")) \
        if character_requests_bytes is not None else ()
    character = decode_resource("CHARACTER", (source_dir / "character.bin").read_bytes(),
                                *heads["character"], character_requests, local_dependency_ids=local_ids)
    for resource in (main, character):
        for ptr in resource.external.values():
            if ptr.resource == "CHARACTER" or (
                    ptr.dependency.kind == "symbol" and ptr.dependency.name == "CHARACTER"):
                _span(character.payload, ptr.offset, 1, "CHARACTER external target")
    inputs = {"config.yaml": hashlib.sha256(config_bytes).hexdigest(),
              "main_reqlist.txt": hashlib.sha256(requests_bytes).hexdigest()}
    if character_requests_bytes is not None:
        inputs["character_reqlist.txt"] = hashlib.sha256(character_requests_bytes).hexdigest()
    return {"schema_version": 1, "source_format": "EXTRA raw threaded relocation payloads",
            "status": "SOURCE_INVENTORY_NOT_NATIVE_ADMISSION", "character": "Meta Knight",
            "source_resource_ids": source_resource_ids,
            "source_inputs": inputs,
            "resources": [main.inventory(), character.inventory()],
            "model": model_inventory(main, character, attributes_offset)}


def build_meta_knight_model_ir(source_dir: Path, native_asset_id: int,
                               attributes_offset: int | None = None,
                               *, source_resource_ids: Mapping[str, int] | None = None) -> dict:
    """Compile both source-root unions through the production geometry compiler.

    The caller owns the asset ID; raw donor IDs are never guessed. A union is
    storage coverage, not a simultaneous draw vector or a gameplay admission.
    Every root retains its source joint and its canonical/hidden/passive/access
    provenance. Runtime action selection and live hierarchy remain due.
    """
    import generate_nds_native_owners as native

    result = build_meta_knight_inventory(source_dir, attributes_offset,
                                        source_resource_ids=source_resource_ids)
    payload = (Path(source_dir) / "character.bin").read_bytes()
    recorded = next(resource for resource in result["resources"] if resource["name"] == "CHARACTER")
    if hashlib.sha256(payload).hexdigest() != recorded["sha256"]:
        raise AdapterError("character input changed between inventory and native compilation")
    raw = decode_resource("CHARACTER", payload, recorded["internal_head"], recorded["external_head"])
    resource = raw.as_native_resource(native_asset_id, {}, native.stage_manifest)
    provider = native.NativeOwnerSource(resource)
    model = result["model"]
    model_ir = {}
    for detail, part in model["details"].items():
        candidates = [("canonical", row) for row in part["canonical_roots"]]
        candidates += [("hidden", row) for row in part["hidden_roots"]]
        candidates += [("modelpart", row) for row in model["modelparts"]["roots"][detail]]
        accessory = model["accesspart"]
        if accessory is not None and accessory.get("root") is not None:
            candidates.append(("accessory", {"joint_id": accessory["joint_id"], **accessory["root"]}))
        source_roots = []
        seen = set()
        for kind, row in candidates:
            identity = (row["offset"], row["joint_id"])
            if identity in seen:
                continue
            seen.add(identity)
            source_roots.append({"kind": kind, "resource": row["resource"],
                                 "offset": row["offset"], "joint_id": row["joint_id"],
                                 "direct_triangles": row["direct_triangles"],
                                 **{key: row[key] for key in ("modelpart_id", "hidden_index", "descriptor_index")
                                    if key in row}})
        source_joints = sorted({row["joint_id"] for row in source_roots})
        binding_by_joint = {joint: binding for binding, joint in enumerate(source_joints)}
        for row in source_roots:
            row["binding"] = binding_by_joint[row["joint_id"]]
        context = native.build_p2_root_set_runtime_context(
            Path(__file__).resolve().parents[2], "metaknight", detail,
            [row["offset"] for row in source_roots], source_provider=provider,
            root_bindings=[row["binding"] for row in source_roots])
        expected_triangles = sum(row["direct_triangles"] for row in source_roots)
        if len(context["triangles"]) != expected_triangles:
            raise AdapterError(f"{detail}: native IR triangle coverage differs from source roots")
        canonical_bytes = json.dumps(context, sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8")
        model_ir[detail] = {
            "source_asset_id": native_asset_id, "source_roots": source_roots,
            "binding_source_joints": source_joints,
            "counts": {"roots": len(context["roots"]), "triangles": len(context["triangles"]),
                       "dense_vertices": len(context["dense_vertices"]), "state_deltas": len(context["state"]),
                       "epochs": len(context["epochs"]), "runs": len(context["runs"]),
                       "state_only_roots": sum(row[4] == 0 for row in context["roots"])},
            "ir_sha256": hashlib.sha256(canonical_bytes).hexdigest(), "ir": context}
    result["model_ir"] = model_ir
    import native_skeletons
    skeleton_ir = {}
    for sid, variant in model["skeletons"]["variants"].items():
        skeleton_ir[sid] = {}
        for detail in ("high", "low"):
            binding_domain = model_ir[detail]["binding_source_joints"]
            source_roots = [{"kind": "skeleton", "resource": row["resource"],
                             "offset": row["offset"], "joint_id": row["joint_id"],
                             "skeleton_id": int(sid), "skeleton_flags": row["skeleton_flags"],
                             "binding": binding_domain.index(row["joint_id"]), "direct_triangles": row["direct_triangles"]}
                            for index, row in enumerate(variant["roots"])]
            owner_name = f"metaknight_skeleton{sid}"
            context = native_skeletons.explicit_context(provider, owner_name, detail, source_roots,
                                                         binding_source_joints=binding_domain)
            if len(context["triangles"]) != sum(row["direct_triangles"] for row in source_roots):
                raise AdapterError("electric skeleton native IR differs from its source triangle coverage")
            encoded = json.dumps(context, sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8")
            skeleton_ir[sid][detail] = {
                "source_asset_id": native_asset_id, "source_roots": source_roots,
                "binding_source_joints": context["binding_source_joints"],
                "counts": {"roots": len(context["roots"]), "triangles": len(context["triangles"]),
                           "dense_vertices": len(context["dense_vertices"]), "state_deltas": len(context["state"]),
                           "epochs": len(context["epochs"]), "runs": len(context["runs"]),
                           "state_only_roots": sum(row[4] == 0 for row in context["roots"])},
                "ir_sha256": hashlib.sha256(encoded).hexdigest(), "ir": context}
    result["skeleton_ir"] = skeleton_ir
    result["status"] = "NATIVE_MODEL_IR_NOT_RUNTIME_ADMISSION"
    result["model"]["conversion_gap"] = (
        "Both raw detail root unions compile through the existing native geometry pipeline. "
        "Runtime hierarchy/action root selection, materials/textures/costume admission and "
        "resolved gameplay/CPU/audio/copy behavior remain due.")
    return result


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path,
                        default=os.environ.get("META_KNIGHT_SOURCE_DIR"))
    parser.add_argument("--attributes-offset", type=lambda value: int(value, 0))
    parser.add_argument("--model-ir", action="store_true")
    parser.add_argument("--model-asset-id", type=lambda value: int(value, 0))
    parser.add_argument("--donor-main-id", type=lambda value: int(value, 0))
    parser.add_argument("--donor-model-id", type=lambda value: int(value, 0))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    if args.source_dir is None:
        parser.error("--source-dir or META_KNIGHT_SOURCE_DIR is required")
    if args.model_ir and args.model_asset_id is None:
        parser.error("--model-ir requires --model-asset-id from the explicit native mapping")
    if args.model_asset_id is not None and not args.model_ir:
        parser.error("--model-asset-id requires --model-ir")
    if (args.donor_main_id is None) != (args.donor_model_id is None):
        parser.error("resolved donor resources require both --donor-main-id and --donor-model-id")
    resource_ids = None if args.donor_main_id is None else {
        "MAIN": args.donor_main_id, "CHARACTER": args.donor_model_id}
    try:
        result = (build_meta_knight_model_ir(args.source_dir, args.model_asset_id, args.attributes_offset,
                                            source_resource_ids=resource_ids)
                  if args.model_ir else build_meta_knight_inventory(args.source_dir, args.attributes_offset,
                                                                    source_resource_ids=resource_ids))
    except (ValueError, OSError) as error:
        print(f"extra-resource-adapter: {error}", file=sys.stderr)
        return 1
    text = json.dumps(result, indent=2, sort_keys=True, allow_nan=False) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8", newline="\n")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
