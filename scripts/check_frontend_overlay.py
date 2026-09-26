#!/usr/bin/env python3
"""Reject resident ELF32 ARM relocations into the lazily loaded frontend.

The input is the final linked ARM ELF produced with ``--emit-relocs``.  Only
relocations whose *source section* is SHF_ALLOC are considered; nonalloc debug
sections and relocations originating inside the overlay are ignored.  The
allowlist is an exact symbol-pair document, never a file, section, glob, or
relocation-type exemption.

Allowlist format::

    {"schema_version": 1, "pairs": [
      {"source_symbol": "resident_owner", "target_symbol": "frontend_entry",
       "reason": "Lifecycle reviewed: ..."}
    ]}

CLI::

    python scripts/check_frontend_overlay.py smash64ds.elf --required \
        --allowlist scripts/frontend_overlay_allowlist.json

The JSON report is printed to stdout on both pass and policy failure. Exit 0
means the check passed, 1 means policy failure, and 2 means malformed input or
invocation. ``overlay_bytes`` is the total allocated, file-backed byte count
for ``.ovl.frontend`` and its allocated suffix sections.
"""

from __future__ import annotations

import argparse
from bisect import bisect_right
import json
import struct
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any


ELF_MAGIC = b"\x7fELF"
ELFCLASS32 = 1
ELFDATA2LSB = 1
ELFDATA2MSB = 2
ET_EXEC = 2
EM_ARM = 40

SHT_NULL = 0
SHT_PROGBITS = 1
SHT_SYMTAB = 2
SHT_STRTAB = 3
SHT_RELA = 4
SHT_NOBITS = 8
SHT_REL = 9
SHT_DYNSYM = 11
SHT_SYMTAB_SHNDX = 18

SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4
SHN_UNDEF = 0
SHN_LORESERVE = 0xFF00
SHN_ABS = 0xFFF1
SHN_COMMON = 0xFFF2
SHN_XINDEX = 0xFFFF

STB_LOCAL = 0
STT_OBJECT = 1
STT_FUNC = 2
STT_SECTION = 3
STT_FILE = 4
STT_NOTYPE = 0

OVERLAY_SECTION = ".ovl.frontend"
OVERLAY_PREFIX = ".ovl.frontend."
OVERLAY_MIN_ALIGNMENT = 4
OVERLAY_RUNTIME_ALIGNMENT = 32


class CheckerError(Exception):
    """Invalid ELF or allowlist input."""


@dataclass(frozen=True)
class Section:
    index: int
    name: str
    kind: int
    flags: int
    address: int
    offset: int
    size: int
    link: int
    info: int
    alignment: int
    entry_size: int

    @property
    def alloc(self) -> bool:
        return bool(self.flags & SHF_ALLOC)

    @property
    def overlay(self) -> bool:
        return self.alloc and (
            self.name == OVERLAY_SECTION or self.name.startswith(OVERLAY_PREFIX)
        )


@dataclass(frozen=True)
class Symbol:
    index: int
    name: str
    value: int
    size: int
    binding: int
    kind: int
    section_index: int
    identity: str
    exact: bool


@dataclass(frozen=True)
class Relocation:
    offset: int
    symbol_index: int
    kind: int
    addend: int | None


@dataclass(frozen=True)
class SourceSymbolIndex:
    """Sorted, section-local symbol intervals for fast relocation attribution."""

    entries: list[tuple[int, int, Symbol]]
    starts: list[int]
    prefix_max_end: list[int]

    def resolve(self, section: Section, offset: int) -> tuple[str, bool, int]:
        cursor = bisect_right(self.starts, offset) - 1
        candidates: list[tuple[int, int, Symbol]] = []
        while cursor >= 0 and self.prefix_max_end[cursor] > offset:
            start, end, symbol = self.entries[cursor]
            if start <= offset < end:
                rank = 0 if symbol.kind in (STT_FUNC, STT_OBJECT) else 1
                candidates.append((rank, end - start, symbol))
            cursor -= 1
        if not candidates:
            return f"<section:{section.name}+0x{offset:x}>", False, STT_NOTYPE
        best_rank = min(item[0] for item in candidates)
        ranked = [item for item in candidates if item[0] == best_rank]
        best_span = min(item[1] for item in ranked)
        best = [item[2] for item in ranked if item[1] == best_span]
        identities = {(symbol.identity, symbol.kind) for symbol in best}
        if len(identities) != 1:
            return f"<ambiguous:{section.name}+0x{offset:x}>", False, STT_NOTYPE
        identity, kind = next(iter(identities))
        return identity, best[0].exact, kind


class Elf32:
    """Small read-only ELF32 parser for linked ARM REL/RELA records."""

    def __init__(self, data: bytes):
        self.data = data
        if len(data) < 52 or data[:4] != ELF_MAGIC:
            raise CheckerError("input is not an ELF file")
        ident = data[:16]
        if ident[4] != ELFCLASS32:
            raise CheckerError(f"ELF class must be 32-bit (got {ident[4]})")
        if ident[5] not in (ELFDATA2LSB, ELFDATA2MSB):
            raise CheckerError(f"unsupported ELF byte order {ident[5]}")
        if ident[6] != 1:
            raise CheckerError("unsupported ELF identification version")
        self.endian = "<" if ident[5] == ELFDATA2LSB else ">"

        header = struct.unpack_from(self.endian + "HHIIIIIHHHHHH", data, 16)
        (self.elf_type, self.machine, version, _entry, _phoff, shoff, _flags,
         ehsize, _phentsize, _phnum, shentsize, shnum, shstrndx) = header
        if version != 1 or ehsize < 52:
            raise CheckerError("invalid ELF32 header version or size")
        if self.machine != EM_ARM:
            raise CheckerError(f"ELF machine must be ARM (got {self.machine})")
        if self.endian != "<":
            raise CheckerError("the Nintendo DS ARM ELF must be little-endian")
        if self.elf_type != ET_EXEC:
            raise CheckerError(
                f"input must be a linked ET_EXEC ELF (got type {self.elf_type})"
            )
        if shoff == 0 or shentsize < 40:
            raise CheckerError("ELF has no usable section header table")
        self.section_entry_size = shentsize
        section_zero = self._read_section_header(shoff, 0)
        if shnum == 0:
            shnum = section_zero[5]
        if shnum == 0:
            raise CheckerError("ELF section header table is empty")
        if shstrndx == SHN_XINDEX:
            shstrndx = section_zero[6]
        if shnum > 0x100000:
            raise CheckerError(f"implausible section count {shnum}")
        if shoff + shentsize * shnum > len(data):
            raise CheckerError("section header table extends past EOF")

        raw_sections = [self._read_section_header(shoff, index)
                        for index in range(shnum)]
        if shstrndx >= shnum:
            raise CheckerError("section-name string table index is out of range")
        name_table = self._section_bytes_from_raw(raw_sections[shstrndx])

        sections: list[Section] = []
        for index, raw in enumerate(raw_sections):
            (name_offset, kind, flags, address, offset, size, link, info,
             alignment, entry_size) = raw
            name = self._cstring(name_table, name_offset)
            if kind != SHT_NOBITS and offset + size > len(data):
                raise CheckerError(
                    f"section {index} ({name or '<unnamed>'}) extends past EOF"
                )
            sections.append(Section(index, name, kind, flags, address, offset,
                                    size, link, info, alignment, entry_size))
        self.sections = sections
        self.section_by_index = {section.index: section for section in sections}
        self.section_names: dict[str, list[Section]] = defaultdict(list)
        for section in sections:
            self.section_names[section.name].append(section)

    def _read_section_header(self, section_offset: int, index: int):
        offset = section_offset + self.section_entry_size * index
        if offset + 40 > len(self.data):
            raise CheckerError(f"section header {index} extends past EOF")
        return struct.unpack_from(self.endian + "IIIIIIIIII", self.data, offset)

    def _section_bytes_from_raw(self, raw) -> bytes:
        (_name, kind, _flags, _address, offset, size, _link, _info,
         _alignment, _entry_size) = raw
        if kind == SHT_NOBITS:
            return b""
        if offset + size > len(self.data):
            raise CheckerError("section data extends past EOF")
        return self.data[offset:offset + size]

    @staticmethod
    def _cstring(table: bytes, offset: int) -> str:
        if offset == 0:
            return ""
        if offset >= len(table):
            return "<bad-string-offset>"
        end = table.find(b"\0", offset)
        if end < 0:
            end = len(table)
        return table[offset:end].decode("utf-8", errors="replace")

    def section_bytes(self, section: Section) -> bytes:
        if section.kind == SHT_NOBITS:
            return b""
        return self.data[section.offset:section.offset + section.size]

    def symbols(self, table: Section) -> list[Symbol]:
        if table.kind not in (SHT_SYMTAB, SHT_DYNSYM):
            raise CheckerError(f"section {table.name} is not a symbol table")
        if table.link >= len(self.sections):
            raise CheckerError(f"symbol table {table.name} has invalid string table")
        strings = self.section_bytes(self.sections[table.link])
        entry_size = table.entry_size or 16
        if entry_size < 16 or table.size % entry_size:
            raise CheckerError(f"malformed symbol table {table.name}")

        extended: list[int] | None = None
        for section in self.sections:
            if section.kind == SHT_SYMTAB_SHNDX and section.link == table.index:
                raw = self.section_bytes(section)
                if len(raw) % 4:
                    raise CheckerError(f"malformed extended indices in {section.name}")
                extended = list(struct.unpack(self.endian + "I" * (len(raw) // 4), raw))
                break

        count = table.size // entry_size
        result: list[Symbol] = []
        current_file = ""
        for index in range(count):
            offset = table.offset + index * entry_size
            name_offset, value, size, info, _other, section_index = \
                struct.unpack_from(self.endian + "IIIBBH", self.data, offset)
            name = self._cstring(strings, name_offset)
            binding = info >> 4
            kind = info & 0xF
            if kind == STT_FILE:
                current_file = name
            if section_index == SHN_XINDEX:
                if extended is None or index >= len(extended):
                    raise CheckerError(
                        f"symbol {name or index} needs a missing extended section index"
                    )
                section_index = extended[index]
            exact = bool(name) and kind != STT_SECTION and not name.startswith("<")
            identity = name
            if binding == STB_LOCAL and name and current_file:
                identity = f"{current_file}::{name}"
            result.append(Symbol(index, name, value, size, binding, kind,
                                 section_index, identity, exact))
        return result

    def relocations(self, section: Section) -> list[Relocation]:
        if section.kind not in (SHT_REL, SHT_RELA):
            return []
        rela = section.kind == SHT_RELA
        expected = 12 if rela else 8
        entry_size = section.entry_size or expected
        if entry_size < expected or section.size % entry_size:
            raise CheckerError(f"malformed relocation section {section.name}")
        records: list[Relocation] = []
        for index in range(section.size // entry_size):
            offset = section.offset + index * entry_size
            r_offset, r_info = struct.unpack_from(self.endian + "II", self.data, offset)
            addend = None
            if rela:
                addend = struct.unpack_from(self.endian + "i", self.data, offset + 8)[0]
            records.append(Relocation(r_offset, r_info >> 8, r_info & 0xFF, addend))
        return records


def load_allowlist(path: Path | None) -> tuple[set[tuple[str, str]], dict[tuple[str, str], str]]:
    if path is None:
        return set(), {}
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise CheckerError(f"cannot read allowlist {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise CheckerError(f"invalid allowlist JSON: {exc}") from exc
    if not isinstance(document, dict) or document.get("schema_version") != 1:
        raise CheckerError("allowlist must be an object with schema_version: 1")
    pairs = document.get("pairs")
    if not isinstance(pairs, list):
        raise CheckerError("allowlist 'pairs' must be a JSON array")
    allowed: set[tuple[str, str]] = set()
    reasons: dict[tuple[str, str], str] = {}
    for index, pair in enumerate(pairs):
        if not isinstance(pair, dict):
            raise CheckerError(f"allowlist pair {index} must be an object")
        source = pair.get("source_symbol")
        target = pair.get("target_symbol")
        reason = pair.get("reason", "")
        if not isinstance(source, str) or not source.strip():
            raise CheckerError(f"allowlist pair {index} needs source_symbol")
        if not isinstance(target, str) or not target.strip():
            raise CheckerError(f"allowlist pair {index} needs target_symbol")
        if not isinstance(reason, str):
            raise CheckerError(f"allowlist pair {index} reason must be a string")
        if any(token in source or token in target for token in ("*", "?")):
            raise CheckerError(f"allowlist pair {index} may not use wildcards")
        if source.startswith("<") or target.startswith("<"):
            raise CheckerError(
                f"allowlist pair {index} must name real ELF symbols, not section fallbacks"
            )
        key = (source, target)
        if key in allowed:
            raise CheckerError(f"duplicate allowlist pair {source!r} -> {target!r}")
        allowed.add(key)
        reasons[key] = reason
    return allowed, reasons


def _section_offset(elf: Elf32, section: Section, address: int) -> int:
    """Resolve r_offset to section-relative offset in linked ET_EXEC output."""
    if section.address <= address < section.address + section.size:
        return address - section.address
    # Some linkers retain section-relative offsets in emitted relocations.
    if address < section.size:
        return address
    raise CheckerError(
        f"relocation offset 0x{address:08x} is outside source section {section.name}"
    )


def _symbol_offset(elf: Elf32, section: Section, symbol: Symbol) -> int:
    if section.address <= symbol.value < section.address + section.size:
        return symbol.value - section.address
    return symbol.value


def _source_symbol_indexes(elf: Elf32, symbols: list[Symbol]
                           ) -> dict[int, SourceSymbolIndex]:
    by_section: dict[int, list[tuple[int, Symbol]]] = defaultdict(list)
    for symbol in symbols:
        if symbol.kind not in (STT_FUNC, STT_OBJECT, STT_NOTYPE):
            continue
        section = elf.section_by_index.get(symbol.section_index)
        if section is None or not section.alloc or section.overlay:
            continue
        start = _symbol_offset(elf, section, symbol)
        if 0 <= start < section.size:
            by_section[section.index].append((start, symbol))

    indices: dict[int, SourceSymbolIndex] = {}
    for section_index, rows in by_section.items():
        section = elf.section_by_index[section_index]
        rows.sort(key=lambda row: (row[0], row[1].kind, row[1].identity))
        unique_starts = sorted({start for start, _symbol in rows})
        next_start = {
            start: (unique_starts[index + 1]
                    if index + 1 < len(unique_starts) else section.size)
            for index, start in enumerate(unique_starts)
        }
        entries: list[tuple[int, int, Symbol]] = []
        for start, symbol in rows:
            end = start + symbol.size if symbol.size > 0 else next_start[start]
            end = min(max(end, start), section.size)
            if end > start:
                entries.append((start, end, symbol))
        entries.sort(key=lambda item: (item[0], item[1], item[2].identity))
        starts = [item[0] for item in entries]
        prefix: list[int] = []
        maximum = 0
        for _start, end, _symbol in entries:
            maximum = max(maximum, end)
            prefix.append(maximum)
        indices[section_index] = SourceSymbolIndex(entries, starts, prefix)
    return indices


def _overlay_target(elf: Elf32, symbol: Symbol,
                    overlay_indices: set[int]) -> tuple[str, bool, int, str | None]:
    if symbol.section_index not in overlay_indices:
        return "", False, symbol.kind, None
    section = elf.section_by_index[symbol.section_index]
    if symbol.kind == STT_SECTION or not symbol.name:
        return f"<section:{section.name}>", False, symbol.kind, section.name
    return symbol.identity, symbol.exact, symbol.kind, section.name


def _relocation_detail(source_section: Section, source_offset: int,
                       source_symbol: str, source_kind: int,
                       target_section: str, target_symbol: str,
                       target_kind: int, relocation: Relocation) -> dict[str, Any]:
    return {
        "source_section": source_section.name,
        "source_offset": f"0x{source_offset:08x}",
        "source_symbol": source_symbol,
        "source_symbol_type": source_kind,
        "target_section": target_section,
        "target_symbol": target_symbol,
        "target_symbol_type": target_kind,
        "relocation_type": relocation.kind,
        "addend": relocation.addend,
    }


def inspect_elf(data: bytes, *, elf_name: str,
                allowed_pairs: set[tuple[str, str]] | None = None,
                allowlist_reasons: dict[tuple[str, str], str] | None = None,
                required: bool = False) -> dict[str, Any]:
    allowed_pairs = allowed_pairs or set()
    allowlist_reasons = allowlist_reasons or {}
    elf = Elf32(data)
    primary_overlay = elf.section_names.get(OVERLAY_SECTION, [])
    overlay_sections = [section for section in elf.sections if section.overlay]
    overlay_indices = {section.index for section in overlay_sections}
    overlay_bytes = sum(section.size for section in overlay_sections
                        if section.kind != SHT_NOBITS)
    errors: list[str] = []

    if required:
        if len(primary_overlay) != 1:
            if not primary_overlay:
                errors.append(f"required overlay section {OVERLAY_SECTION} is missing")
            else:
                errors.append(f"multiple {OVERLAY_SECTION} sections found")
        else:
            overlay = primary_overlay[0]
            if not overlay.alloc:
                errors.append(f"{OVERLAY_SECTION} is not SHF_ALLOC")
            if overlay.kind == SHT_NOBITS:
                errors.append(f"{OVERLAY_SECTION} is not file-backed")
            if overlay.size == 0:
                errors.append(f"{OVERLAY_SECTION} is empty")
            if (overlay.alignment < OVERLAY_MIN_ALIGNMENT or
                    overlay.alignment & (overlay.alignment - 1) != 0):
                errors.append(
                    f"{OVERLAY_SECTION} alignment {overlay.alignment} is below "
                    f"the required power-of-two {OVERLAY_MIN_ALIGNMENT}"
                )
            elif overlay.address % overlay.alignment != 0:
                errors.append(
                    f"{OVERLAY_SECTION} address 0x{overlay.address:08x} is not "
                    f"aligned to {overlay.alignment}"
                )
            if overlay.address % OVERLAY_RUNTIME_ALIGNMENT != 0:
                errors.append(
                    f"{OVERLAY_SECTION} start address 0x{overlay.address:08x} is not "
                    f"{OVERLAY_RUNTIME_ALIGNMENT}-byte aligned"
                )
            if ((overlay.address + overlay.size) % OVERLAY_RUNTIME_ALIGNMENT != 0):
                errors.append(
                    f"{OVERLAY_SECTION} end address "
                    f"0x{overlay.address + overlay.size:08x} is not "
                    f"{OVERLAY_RUNTIME_ALIGNMENT}-byte aligned"
                )

    symbol_tables: dict[int, list[Symbol]] = {}
    source_indexes: dict[int, dict[int, SourceSymbolIndex]] = {}
    relocation_sections = [section for section in elf.sections
                           if section.kind in (SHT_REL, SHT_RELA)]
    resident_alloc_relocation_count = 0
    crossing: dict[tuple[str, str], dict[str, Any]] = {}
    allowed_crossing: dict[tuple[str, str], dict[str, Any]] = {}

    for relocation_section in relocation_sections:
        source_section = elf.section_by_index.get(relocation_section.info)
        if (source_section is None or not source_section.alloc or
                source_section.overlay):
            continue
        if relocation_section.link >= len(elf.sections):
            raise CheckerError(
                f"relocation section {relocation_section.name} has invalid symbol table link"
            )
        symtab_section = elf.sections[relocation_section.link]
        if symtab_section.index not in symbol_tables:
            symbol_tables[symtab_section.index] = elf.symbols(symtab_section)
        symbols = symbol_tables[symtab_section.index]
        if symtab_section.index not in source_indexes:
            source_indexes[symtab_section.index] = _source_symbol_indexes(elf, symbols)
        section_source_index = source_indexes[symtab_section.index].get(
            source_section.index
        )

        for relocation in elf.relocations(relocation_section):
            resident_alloc_relocation_count += 1
            if relocation.symbol_index == 0:
                continue
            if relocation.symbol_index >= len(symbols):
                raise CheckerError(
                    f"relocation in {relocation_section.name} has invalid symbol index "
                    f"{relocation.symbol_index}"
                )
            target = symbols[relocation.symbol_index]
            target_symbol, target_exact, target_kind, target_section_name = \
                _overlay_target(elf, target, overlay_indices)
            if target_section_name is None:
                continue
            source_offset = _section_offset(elf, source_section, relocation.offset)
            if section_source_index is None:
                source_name = f"<section:{source_section.name}+0x{source_offset:x}>"
                source_exact = False
                source_kind = STT_NOTYPE
            else:
                source_name, source_exact, source_kind = section_source_index.resolve(
                    source_section, source_offset
                )
            pair = (source_name, target_symbol)
            record = _relocation_detail(
                source_section, source_offset, source_name, source_kind,
                target_section_name, target_symbol, target_kind, relocation,
            )
            pair_table = (allowed_crossing if
                          pair in allowed_pairs and source_exact and target_exact
                          else crossing)
            entry = pair_table.setdefault(pair, {
                "source_symbol": source_name,
                "target_symbol": target_symbol,
                "count": 0,
                "relocations": [],
            })
            entry["count"] += 1
            entry["relocations"].append(record)

    if required and resident_alloc_relocation_count == 0:
        errors.append(
            "no resident allocated-section relocations were emitted; "
            "link with --emit-relocs"
        )

    def ordered_pairs(table: dict[tuple[str, str], dict[str, Any]]):
        result = []
        for key in sorted(table):
            item = table[key]
            item["relocations"].sort(
                key=lambda rel: (rel["source_section"], rel["source_offset"],
                                 rel["relocation_type"])
            )
            if key in allowlist_reasons and allowlist_reasons[key]:
                item["reason"] = allowlist_reasons[key]
            result.append(item)
        return result

    unknown_pairs = ordered_pairs(crossing)
    allowed_pairs_report = ordered_pairs(allowed_crossing)
    unused_allowlist = sorted(allowed_pairs - set(allowed_crossing))
    if unknown_pairs:
        errors.append(
            f"{len(unknown_pairs)} unallowlisted resident-to-overlay symbol pair(s)"
        )

    return {
        "schema_version": 1,
        "elf": elf_name,
        "elf_type": elf.elf_type,
        "machine": elf.machine,
        "overlay_section": OVERLAY_SECTION,
        "required": required,
        "overlay_present": bool(primary_overlay),
        "overlay_bytes": overlay_bytes,
        "overlay_sections": [
            {
                "name": section.name,
                "address": f"0x{section.address:08x}",
                "size": section.size,
                "alignment": section.alignment,
                "alloc": section.alloc,
            }
            for section in overlay_sections
        ],
        "relocation_sections": len(relocation_sections),
        "resident_alloc_relocations": resident_alloc_relocation_count,
        "allowed_crossing_pairs": allowed_pairs_report,
        "unknown_crossing_pairs": unknown_pairs,
        "unused_allowlist_pairs": [
            {"source_symbol": source, "target_symbol": target}
            for source, target in unused_allowlist
        ],
        "errors": errors,
        "passed": not errors,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("elf", type=Path, help="final linked ELF32 ARM image")
    parser.add_argument("--allowlist", type=Path,
                        help="JSON exact source_symbol/target_symbol pairs")
    parser.add_argument("--required", action="store_true",
                        help=f"require a valid nonempty {OVERLAY_SECTION} section")
    parser.add_argument("--json", type=Path,
                        help="also write the complete report to this path")
    args = parser.parse_args(argv)
    try:
        allowed, reasons = load_allowlist(args.allowlist)
        report = inspect_elf(
            args.elf.read_bytes(), elf_name=str(args.elf),
            allowed_pairs=allowed, allowlist_reasons=reasons,
            required=args.required,
        )
    except (OSError, CheckerError) as exc:
        report = {
            "schema_version": 1,
            "elf": str(args.elf),
            "required": args.required,
            "overlay_section": OVERLAY_SECTION,
            "overlay_bytes": 0,
            "allowed_crossing_pairs": [],
            "unknown_crossing_pairs": [],
            "errors": [str(exc)],
            "passed": False,
        }
        exit_code = 2
    else:
        exit_code = 0 if report["passed"] else 1

    encoded = json.dumps(report, indent=2, sort_keys=True)
    print(encoded)
    if args.json is not None:
        try:
            args.json.write_text(encoded + "\n", encoding="utf-8")
        except OSError as exc:
            print(f"cannot write report {args.json}: {exc}", file=sys.stderr)
            return 2
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
