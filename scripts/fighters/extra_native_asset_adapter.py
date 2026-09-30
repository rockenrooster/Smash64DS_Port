#!/usr/bin/env python3
"""Compile resolved EXTRA asset/event data into the existing RELO contract.

This host producer never executes donor code. Donor file IDs, MIPS addresses,
and source resource offsets are distinct types. Pointer-bearing commands are
copied as a typed graph and receive normal RELO fixups. Unknown commands or
address domains are errors. Geometry remains an input to the native owner
compiler; this producer does not provide a graphics interpreter.
"""
from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _paths  # noqa: F401
from extra_resource_adapter import AdapterError, Dependency, RawResource, decode_resource
from extra_resolved_actions import parse_symbols

ROOT = Path(__file__).resolve().parents[2]
MAIN_ID, MODEL_ID, MOTION_ID = 5455, 5456, 0x6000
NONE = 0x80000000
RELO_HEADER_BYTES = 0x40
# FT event lengths from BattleShip ft/fttypes.h and ft/ftmain.c.
EVENT_WORDS = {index: 1 for index in range(52)}
EVENT_WORDS.update({3: 5, 4: 5, 7: 2, 12: 2, 13: 2, 31: 4,
                    34: 2, 36: 2, 38: 4, 39: 4, 46: 2})
EVENT_POINTERS = {12: "throw", 13: "damage_table", 34: "script",
                  36: "script", 46: "script"}
# Command.asm: full-byte commands, not the 6-bit vanilla opcode enum.
CUSTOM_BYTES = {0xD0: (1, "frame_speed"), 0xD1: (1, "armour"),
                0xD2: (1, "hitbox_direction"), 0xD3: (1, "translation_scale"),
                0xD4: (1, "velocity_y"), 0xD5: (1, "fast_fall"),
                0xD6: (2, "random_sfx"), 0xD7: (1, "kinetic"),
                0xD8: (1, "hitbox_fgm"), 0xD9: (2, "environment_color"),
                0xDA: (1, "reverse_direction"), 0xDB: (1, "moveset_file_goto"),
                0xDC: (2, "taunt_voice")}


class ConversionError(AdapterError):
    pass


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def span(data: bytes, at: int, length: int, label: str, align: int = 4) -> bytes:
    if at < 0 or at % align or length < 0 or at + length > len(data):
        raise ConversionError(f"{label}: invalid span {at:#x}+{length:#x}")
    return data[at:at + length]


def _vpk_decoder():
    path = ROOT / "scripts/extract-battleship-relocdata.py"
    spec = importlib.util.spec_from_file_location("extra_vpk_decoder", path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module.decode_vpk0


class DonorFiles:
    """Read the NALE reloc table using its patched, source-defined directory."""
    def __init__(self, rom: bytes):
        if rom[:4] != bytes.fromhex("80371240"):
            raise ConversionError("donor files: expected big-endian NALE ROM")
        self.rom = rom
        self.table = struct.unpack_from(">I", span(rom, 0x41F08, 4, "file table"))[0]
        upper = struct.unpack_from(">I", rom, 0x527E8)[0]
        lower = struct.unpack_from(">I", rom, 0x527F8)[0]
        if upper >> 26 != 15 or lower >> 26 not in (9, 13):
            raise ConversionError("donor files: file-count instruction contract changed")
        lo = lower & 0xFFFF
        if lower >> 26 == 9 and lo & 0x8000:
            lo -= 0x10000
        self.count = ((upper & 0xFFFF) << 16) + lo
        if not 0 < self.count < 0xFFFF:
            raise ConversionError("donor files: invalid count")
        raw = span(rom, self.table, (self.count + 1) * 12, "file directory")
        self.rows = list(struct.iter_unpack(">IHHHH", raw))
        self.base = self.table + len(raw)
        self.decode = _vpk_decoder()
        self.cache = {}

    def load(self, file_id: int) -> RawResource:
        if file_id in self.cache:
            return self.cache[file_id]
        if not 0 <= file_id < self.count:
            raise ConversionError(f"donor files: absent ID {file_id}")
        flags, intern, compressed, extern, unpacked = self.rows[file_id]
        start, end = flags & 0x7FFFFFFF, self.rows[file_id + 1][0] & 0x7FFFFFFF
        if end < start or compressed * 4 > end - start:
            raise ConversionError(f"donor file {file_id}: invalid directory range")
        raw = span(self.rom, self.base + start, compressed * 4, f"donor {file_id}", 2)
        payload = self.decode(raw)[0] if flags & 0x80000000 else raw
        if len(payload) != unpacked * 4:
            raise ConversionError(f"donor file {file_id}: decompressed length mismatch")
        reqraw = span(self.rom, self.base + start + compressed * 4,
                      end - start - compressed * 4, f"requests {file_id}", 2)
        requests = tuple(Dependency("donor_file", file_id=value)
                         for (value,) in struct.iter_unpack(">H", reqraw))
        resource = decode_resource(str(file_id), payload, intern * 4, extern * 4, requests)
        self.cache[file_id] = resource
        return resource


def encode_o2r(file_id: int, payload: bytes, internal: dict[int, int],
               external: dict[int, tuple[int, int]]) -> bytes:
    """Deterministic RELO chains. Offsets are byte addresses until encoded."""
    if not 0 <= file_id < 0xFFFF or len(payload) % 4:
        raise ConversionError("RELO: invalid file identity or payload size")
    if internal.keys() & external.keys():
        raise ConversionError("RELO: internal/external slot overlap")
    result = bytearray(payload)
    ids = []
    heads = []
    for pointers, is_external in ((internal, False), (external, True)):
        slots = sorted(pointers)
        heads.append(slots[0] // 4 if slots else 0xFFFF)
        for index, slot in enumerate(slots):
            span(payload, slot, 4, "RELO slot")
            target = pointers[slot]
            if is_external:
                dep, target = target
                if not 0 <= dep < 0xFFFF:
                    raise ConversionError("RELO: dependency ID outside u16")
                ids.append(dep)
            else:
                span(payload, target, 1, "RELO target")
            if target % 4 or not 0 <= target // 4 < 0xFFFF or slot // 4 >= 0xFFFF:
                raise ConversionError("RELO: threaded target/slot outside encoding")
            nxt = slots[index + 1] // 4 if index + 1 < len(slots) else 0xFFFF
            struct.pack_into(">I", result, slot, (nxt << 16) | (target // 4))
    header = bytearray(RELO_HEADER_BYTES)
    header[4:8] = b"OLER"
    return (bytes(header) + struct.pack("<IHHI", file_id, *heads, len(ids)) +
            b"".join(struct.pack("<H", value) for value in ids) +
            struct.pack("<I", len(result)) + bytes(result))


def raw_to_o2r(resource: RawResource, native_id: int,
               mapping: dict[int, int]) -> bytes:
    internal = {slot: ptr.offset for slot, ptr in resource.internal.items()}
    external = {}
    for slot, ptr in resource.external.items():
        donor = ptr.dependency.file_id
        if donor not in mapping:
            raise ConversionError(f"{resource.name}:{slot:#x}: unmapped dependency {donor}")
        external[slot] = (mapping[donor], ptr.offset)
    return encode_o2r(native_id, resource.payload, internal, external)


@dataclass(frozen=True, order=True)
class Address:
    domain: str
    owner: int
    offset: int

    def label(self):
        return f"{self.domain}:{self.owner}:{self.offset:#x}"


class EventCompiler:
    """Typed event graph; copy nodes once, remap every pointer-bearing edge."""
    def __init__(self, files: DonorFiles, linked_rom: bytes,
                 symbols: dict[int, list[str]], mainmotion_id: int = 232,
                 static_regions: tuple[tuple[int, int, int], ...] = ()):
        self.files, self.rom, self.symbols = files, linked_rom, symbols
        self.mainmotion_id = mainmotion_id
        # Source main.asm origin0x02C00000/base0x80400000. Extra static donor
        # segments require explicit (address,length,ROMoffset) provenance.
        self.regions = ((0x80400000, len(linked_rom) - 0x2C00000, 0x2C00000),
                        *static_regions)
        self.nodes = {}
        self.commands = Counter()
        self.custom = Counter()
        self.edges = {}
        self.classification = {}
        self.fallthrough = {}

    def script_address(self, value: int) -> Address | None:
        if value == NONE:
            return None
        return (self.rom_address(value) if value > NONE else
                Address("file", self.mainmotion_id, value))

    def rom_address(self, value: int) -> Address:
        for start, length, offset in self.regions:
            if start <= value < start + length:
                at = value - start + offset
                span(self.rom, at, 4, "donor linked pointer")
                return Address("rom", 0, at)
        raise ConversionError(f"donor pointer {value:#x}: unclassified address segment; "
                              f"symbols={self.symbols.get(value, [])}")

    def read(self, address: Address, length: int) -> bytes:
        data = self.rom if address.domain == "rom" else self.files.load(address.owner).payload
        return span(data, address.offset, length, address.label())

    def pointer(self, address: Address) -> Address:
        if address.domain == "rom":
            return self.rom_address(struct.unpack(">I", self.read(address, 4))[0])
        resource = self.files.load(address.owner)
        ptr = resource.pointer(address.offset)
        if ptr is None:
            raise ConversionError(f"{address.label()}: null required event target")
        return Address("file", ptr.dependency.file_id if ptr.dependency else address.owner,
                       ptr.offset)

    def visit(self, address: Address, kind="script"):
        previous = self.classification.get(address)
        if previous is not None:
            if previous != kind:
                raise ConversionError(f"{address.label()}: conflicting {previous}/{kind}")
            return
        self.classification[address] = kind
        if kind == "throw":
            # FTThrowHitDesc[2]: seven s32 each. thrown2.c accesses row1 for
            # release; keeping only the attack row would drop required data.
            self.nodes[address] = self.read(address, 56)
            return
        if kind == "damage_table":
            # Vanilla FTMotionDamageScript has 2x27 pointers. Preserve every
            # entry and graph edge; new roster indexing needs explicit runtime
            # handling rather than silently reading beyond the source table.
            raw = self.read(address, 2 * 27 * 4)
            self.nodes[address] = raw
            for i in range(54):
                slot = Address(address.domain, address.owner, address.offset + i * 4)
                if self.read(slot, 4) == bytes(4):
                    continue
                target = self.pointer(slot)
                self.edges[(address, i * 4)] = target
                self.visit(target)
            return
        raw = self.read(address, 4)
        word = struct.unpack(">I", raw)[0]
        opcode, byte = word >> 26, word >> 24
        if byte >= 0xD0:
            if byte not in CUSTOM_BYTES:
                raise ConversionError(f"{address.label()}: unknown custom command {word:#010x}")
            words, semantic = CUSTOM_BYTES[byte]
            if semantic in ("random_sfx", "moveset_file_goto"):
                raise ConversionError(f"{address.label()}: {semantic} requires typed table/control lowering")
            self.custom[semantic] += 1
        else:
            if opcode not in EVENT_WORDS:
                raise ConversionError(f"{address.label()}: unknown FT event {word:#010x}")
            words = EVENT_WORDS[opcode]
        self.commands[byte] += 1
        self.nodes[address] = self.read(address, words * 4)
        if byte < 0xD0 and opcode in EVENT_POINTERS:
            target = self.pointer(Address(address.domain, address.owner, address.offset + 4))
            self.edges[(address, 4)] = target
            self.visit(target, EVENT_POINTERS[opcode])
        if byte < 0xD0 and opcode in (0, 35, 36):
            return
        successor = Address(address.domain, address.owner, address.offset + words * 4)
        self.fallthrough[address] = successor
        self.visit(successor)

    def emit(self, descriptors: list[dict], mapping: dict[int, int]) -> tuple[bytes, dict]:
        roots = [self.script_address(row["script_donor_value"]) for row in descriptors]
        for root in roots:
            if root:
                self.visit(root)
        # Commands require contiguous fallthrough. Source domains/files are
        # sorted, and every reachable fallthrough node has been visited.
        data = bytearray(len(descriptors) * 12)
        placed = {}
        for address in sorted(self.nodes):
            placed[address] = len(data)
            data.extend(self.nodes[address])
        for address, successor in self.fallthrough.items():
            if placed[successor] != placed[address] + len(self.nodes[address]):
                raise ConversionError(f"{address.label()}: node placement breaks fallthrough")
        internal = {}
        for (address, delta), target in self.edges.items():
            internal[placed[address] + delta] = placed[target]
        for index, (row, root) in enumerate(zip(descriptors, roots)):
            donor_id = row["animation_file_id"]
            if donor_id and donor_id not in mapping:
                raise ConversionError(f"descriptor {index}: unmapped animation {donor_id}")
            offset = placed[root] if root else NONE
            struct.pack_into(">III", data, index * 12, mapping.get(donor_id, 0),
                             offset, row["flags"])
        output = encode_o2r(MOTION_ID, bytes(data), internal, {})
        return output, {"commands": {hex(k): v for k, v in sorted(self.commands.items())},
                        "custom_semantics": dict(self.custom), "graph_nodes": len(placed),
                        "internal_fixups": len(internal), "bytes": len(data),
                        "donor_addresses_in_runtime": 0}


def load_o2r(path: Path) -> tuple[int, RawResource]:
    raw = path.read_bytes()
    if len(raw) < 0x50 or raw[4:8] != b"OLER":
        raise ConversionError(f"{path}: invalid RELO header")
    file_id, intern, extern, count = struct.unpack_from("<IHHI", raw, 0x40)
    table = span(raw, 0x4C, count * 2, "RELO dependencies", 2)
    size_at = 0x4C + count * 2
    size = struct.unpack("<I", span(raw, size_at, 4, "RELO length", 2))[0]
    payload = raw[size_at + 4:]
    if len(payload) != size:
        raise ConversionError(f"{path}: RELO length mismatch")
    dependencies = tuple(Dependency("donor_file", file_id=value)
                         for (value,) in struct.iter_unpack("<H", table))
    return file_id, decode_resource(str(file_id), payload, intern * 4, extern * 4,
                                   dependencies)


def o2r_index(root: Path) -> dict[int, Path]:
    index = {}
    for path in sorted(root.glob("*/*")):
        if not path.is_file():
            continue
        with path.open("rb") as stream:
            header = stream.read(0x44)
        if len(header) != 0x44 or header[4:8] != b"OLER":
            continue
        file_id = struct.unpack_from("<I", header, 0x40)[0]
        if file_id in index:
            # O2R archives can contain named/raw aliases. Only accept exact
            # aliases; never choose an arbitrary conflicting source.
            if path.read_bytes() != index[file_id].read_bytes():
                raise ConversionError(f"O2R source ID {file_id}: conflicting containers")
            continue
        index[file_id] = path
    return index


def verified_identity(resource: RawResource, native: RawResource) -> None:
    if (resource.payload != native.payload or
            resource.internal != native.internal or
            resource.external != native.external):
        # The resource names may differ, but donor numeric resource names are
        # equal here; equality covers the entire threaded payload and targets.
        raise ConversionError(f"inherited ID {resource.name}: donor/native source identity differs")


def _purin_menu_region(rom: bytes) -> tuple[int, int, int]:
    """Qualify one reached static submotion using BattleShip source words.

    smashbrothers.us.yaml ovl1: ROM1079C0/VRAM803903E0, symbols_us.txt:
    D_ovl1_80392480 and D_ovl1_80392584. The ROM bytes independently check the
    source-defined texture loop and Win2 subroutine before admitting the map.
    """
    source = ROOT / "decomp/BattleShip-main/decomp/src/sc/scsubsys/scsubsysdatapurin.c"
    text = source.read_text(encoding="utf-8")
    if "s32 D_ovl1_80392480[]" not in text or "s32 D_ovl1_80392584[]" not in text:
        raise ConversionError("Purin inherited menu source symbols changed")
    offset = 0x1079C0 + (0x80392480 - 0x803903E0)
    expected = (0xAC000004, 0xAC100004, 0x04000002, 0xAC000005, 0xAC100005,
                0x04000004, 0xAC000004, 0xAC100004, 0x04000002, 0xAC000000,
                0xAC100000, 0x04000001, 0x8C000000)
    if span(rom, offset, len(expected) * 4, "Purin menu texture loop") != struct.pack(
            ">13I", *expected):
        raise ConversionError("Purin inherited menu texture loop differs from BattleShip source")
    win2 = (0x88000000, 0x80392480, 0x04000050, 0x88000000,
            0x80392480, 0x0400000A, 0)
    if span(rom, offset + 0x104, 28, "Purin menu Win2") != struct.pack(
            ">7I", *win2):
        raise ConversionError("Purin inherited menu Win2 differs from BattleShip source")
    return 0x80392480, 0x120, offset


def compile_assets(donor_dir: Path, native_source_root: Path) -> tuple[dict[str, bytes], dict]:
    donor_dir = donor_dir.resolve()
    stage = donor_dir / "extra"
    resolved_bytes = (donor_dir / "resolved-actions.json").read_bytes()
    resolved = json.loads(resolved_bytes)
    if resolved.get("schema") != "smash64ds.extra-resolved-actions.v1":
        raise ConversionError("resolved actions schema changed")
    rom = (stage / "ssb64asm_extra_review.z64").read_bytes()
    symbol_bytes = (stage / "review-symbols.log").read_bytes()
    if resolved["rom_sha256"] != sha(rom) or resolved["symbols_sha256"] != sha(symbol_bytes):
        raise ConversionError("resolved actions do not match frozen review ROM/symbols")
    core = resolved["core_file_ids"]
    if core != [MAIN_ID, 232, 0, MODEL_ID, 331, 0, 0, 0, 0]:
        raise ConversionError(f"Meta Knight source core file identities changed: {core}")
    parameters, menu = resolved["parameters"], resolved["menu_parameters"]
    if len(parameters) != 225 or len(menu) != 15:
        raise ConversionError("Meta Knight descriptor counts changed")
    files_rom = (stage / "smashremix/roms/original_extra.z64").read_bytes()
    files = DonorFiles(files_rom)
    symbols = parse_symbols(symbol_bytes.decode("utf-8"))
    descriptors = parameters + menu
    animation_ids = {row["animation_file_id"] for row in descriptors if row["animation_file_id"]}
    reached = animation_ids | {value for value in core if value}
    todo = list(reached)
    while todo:
        resource = files.load(todo.pop())
        for pointer in resource.external.values():
            target = pointer.dependency.file_id
            if target not in reached:
                reached.add(target)
                todo.append(target)
    # Explicit identity map: own IDs are appended by this frozen donor. Every
    # inherited ID is admitted only by complete byte/relocation identity.
    mapping = {donor: donor for donor in sorted(reached)}
    native_index = o2r_index(native_source_root)
    source_identity = []
    for donor_id in sorted(reached):
        resource = files.load(donor_id)
        if MAIN_ID <= donor_id <= 5551:
            source_identity.append({"donor_file_id": donor_id, "native_file_id": donor_id,
                                    "origin": "own frozen EXTRA resource",
                                    "payload_sha256": sha(resource.payload)})
        else:
            if donor_id not in native_index:
                raise ConversionError(f"inherited donor ID {donor_id}: missing native source")
            _, native = load_o2r(native_index[donor_id])
            verified_identity(resource, native)
            source_identity.append({"donor_file_id": donor_id, "native_file_id": donor_id,
                                    "origin": "verified complete source identity",
                                    "native_source": str(native_index[donor_id]),
                                    "container_sha256": sha(native_index[donor_id].read_bytes()),
                                    "payload_sha256": sha(resource.payload)})
    compiler = EventCompiler(files, rom, symbols, static_regions=(_purin_menu_region(rom),))
    motion, event_report = compiler.emit(descriptors, mapping)
    outputs = {"reloc_extra/MetaKnightMainMotion": motion}
    for donor_id in sorted(reached):
        name = ("MetaKnightMain" if donor_id == MAIN_ID else
                "MetaKnightModel" if donor_id == MODEL_ID else
                f"MetaKnightAnim{donor_id}" if donor_id in animation_ids else
                f"MetaKnightDependency{donor_id}")
        outputs[f"reloc_extra/{name}"] = raw_to_o2r(files.load(donor_id), mapping[donor_id], mapping)
    core_rows = []
    for index, donor_id in enumerate(core):
        native_id = MOTION_ID if index in (1, 2) else mapping.get(donor_id, 0)
        size = event_report["bytes"] if index in (1, 2) else len(files.load(donor_id).payload) if donor_id else 0
        core_rows.append({"donor_file_id": donor_id, "native_file_id": native_id, "size": size})
    max_size = lambda rows: max((len(files.load(row["animation_file_id"]).payload)
                                 for row in rows if row["animation_file_id"]), default=0)
    assets = []
    formats = {}
    for row in descriptors:
        file_id = row["animation_file_id"]
        if not file_id:
            continue
        format_name = "ANIM32" if row["flags"] & 8 else "ANIM16"
        if file_id in formats and formats[file_id] != format_name:
            raise ConversionError(f"animation {file_id}: conflicting format consumers")
        formats[file_id] = format_name

    def allocation_bytes(donor_id):
        seen = set()
        def visit(file_id):
            if file_id in seen:
                return 0
            seen.add(file_id)
            resource = files.load(file_id)
            return ((len(resource.payload) + 15) & ~15) + sum(
                visit(pointer.dependency.file_id) for pointer in resource.external.values())
        return visit(donor_id)

    for name, blob in outputs.items():
        count = struct.unpack_from("<I", blob, 0x48)[0]
        size = struct.unpack_from("<I", blob, 0x4C + count * 2)[0]
        native_id = struct.unpack_from("<I", blob, 0x40)[0]
        role = ("MAIN" if native_id == MAIN_ID else "MODEL" if native_id == MODEL_ID else
                "MOTION" if native_id == MOTION_ID else "SHIELD" if native_id == 331 else
                formats.get(native_id, "DEPENDENCY"))
        alloc = ((size + 15) & ~15) if native_id == MOTION_ID else allocation_bytes(native_id)
        assets.append({"native_file_id": native_id,
                       "path": name, "size": size, "container_sha256": sha(blob),
                       "allocation_size": alloc, "role": role,
                       "animation": name.startswith("reloc_extra/MetaKnightAnim")})
    bindings = {"schema": "smash64ds.p4-native-runtime-bindings.v1",
                "status": "IMPLEMENTED_NOT_ACCEPTED", "character": "MetaKnight",
                "source_rom_sha256": sha(rom), "source_files_rom_sha256": sha(files_rom),
                "resolved_actions_sha256": sha(resolved_bytes), "core_files": core_rows,
                "attribute_offset": resolved["attribute_offset"],
                "mainmotion_table_offset": 0, "menu_table_offset": len(parameters) * 12,
                "mainmotion_count": len(parameters), "menu_count": len(menu),
                "main_script_offset_domain": "relative native motion payload offset",
                "menu_script_offset_domain": "relative native motion payload offset; ndsP4GetMenuEventScript",
                "max_main_animation_size": max_size(parameters),
                "max_menu_animation_size": max_size(menu),
                "file_id_mapping": source_identity, "assets": assets,
                "event_graph": event_report,
                "checks_owed": ["native owner geometry compilation", "runtime integration",
                                "CSS and playable natural-input match verification"]}
    return outputs, bindings


def render_registry(bindings: dict) -> str:
    """Existing loader rows/normalization roles, never a second loader ABI."""
    rows = ["/* Generated by extra_native_asset_adapter.py; do not edit. */",
            f"/* Frozen review ROM SHA256: {bindings['source_rom_sha256']} */",
            "#ifndef NDS_METAKNIGHT_NATIVE_ASSETS_GENERATED_H",
            "#define NDS_METAKNIGHT_NATIVE_ASSETS_GENERATED_H", "",
            f"#define NDS_METAKNIGHT_MAIN_FILE_ID {MAIN_ID}u",
            f"#define NDS_METAKNIGHT_MODEL_FILE_ID {MODEL_ID}u",
            f"#define NDS_METAKNIGHT_MOTION_FILE_ID {MOTION_ID}u",
            f"#define NDS_METAKNIGHT_ATTRIBUTE_OFFSET {bindings['attribute_offset']}u",
            f"#define NDS_METAKNIGHT_MAIN_MOTION_OFFSET {bindings['mainmotion_table_offset']}u",
            f"#define NDS_METAKNIGHT_MENU_MOTION_OFFSET {bindings['menu_table_offset']}u",
            f"#define NDS_METAKNIGHT_MAIN_MOTION_COUNT {bindings['mainmotion_count']}u",
            f"#define NDS_METAKNIGHT_MENU_MOTION_COUNT {bindings['menu_count']}u",
            f"#define NDS_METAKNIGHT_MAX_MAIN_ANIMATION_BYTES {bindings['max_main_animation_size']}u",
            f"#define NDS_METAKNIGHT_MAX_MENU_ANIMATION_BYTES {bindings['max_menu_animation_size']}u",
            "", "/* X(file_id, path, role, payload_bytes, allocation_bytes).",
            " * Roles distinguish AObj16 lane/command normalization from AObj32.",
            " * MAIN uses the existing FTAttributes mixed-width normalizer at",
            " * NDS_METAKNIGHT_ATTRIBUTE_OFFSET; MODEL uses native owner metadata.",
            " * MOTION is already typed FT command data; apply word byte swap and",
            " * threaded pointer fixups. All descriptor offsets stay relative;",
            " * ndsP4GetMenuEventScript adds the payload base for menu statuses. */",
            "#define NDS_METAKNIGHT_NATIVE_ASSETS(X) \\"]
    for index, asset in enumerate(bindings["assets"]):
        suffix = " \\" if index + 1 < len(bindings["assets"]) else ""
        rows.append(f"    X({asset['native_file_id']}u, \"nitro:/reloc/{asset['path']}\", "
                    f"{asset['role']}, {asset['size']}u, {asset['allocation_size']}u){suffix}")
    for format_name in ("ANIM16", "ANIM32"):
        rows += ["", f"#define NDS_METAKNIGHT_{format_name}_ASSETS(X) \\"]
        ids = [asset["native_file_id"] for asset in bindings["assets"]
               if asset["role"] == format_name]
        for index, file_id in enumerate(ids):
            rows.append(f"    X({file_id}u)" + (" \\" if index + 1 < len(ids) else ""))
    rows += ["", "#endif", ""]
    return "\n".join(rows)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--donor-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--native-source-root", type=Path, required=True)
    args = parser.parse_args(argv)
    output = args.output_dir.resolve()
    if not output.is_relative_to(ROOT / "builds") or output.is_relative_to(args.donor_dir.resolve()):
        parser.error("output must be in this checkout's builds/ and outside the frozen donor")
    try:
        outputs, bindings = compile_assets(args.donor_dir, args.native_source_root)
        output.mkdir(parents=True, exist_ok=True)
        for name, blob in outputs.items():
            path = output / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(blob)
        (output / "native-runtime-bindings.json").write_text(
            json.dumps(bindings, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n")
        (output / "nds_metaknight_native_assets.generated.h").write_text(
            render_registry(bindings), encoding="utf-8", newline="\n")
    except (AdapterError, OSError, KeyError, ValueError) as error:
        print(f"extra-native-assets: {error}", file=sys.stderr)
        return 1
    print(f"extra-native-assets: {len(outputs)} resources; "
          f"{bindings['mainmotion_count']} main + {bindings['menu_count']} menu descriptors")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
