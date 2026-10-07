"""Read an SSB64 ROM image (vanilla US or a Smash Remix build) for the P4 source adapter.

Covers the reloc file table (whose length Remix grows in place), VPK0 file
extraction, a file's internal/external relocation chains, the Remix code-region
address map, and Torch's SSB64:RELOC O2R container, so a donor file becomes the
same input the existing DS generators already read from BattleShip_o2r.

The container layout mirrors decomp/BattleShip-main/torch/src/factories/ssb64/
RelocFactory.cpp (RelocBinaryExporter): a 0x40-byte LUS header, then
u32 file_id, u16 reloc_intern, u16 reloc_extern, u32 extern count, u16 extern
ids, u32 data size and the decompressed big-endian file bytes, all header fields
little-endian. `verify_vanilla_o2r` proves the writer byte-identical against the
Torch export before any donor file is trusted to it.
"""
from __future__ import annotations

import hashlib
import importlib.util
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

_SCRIPTS = Path(__file__).resolve().parents[1]

RELOC_TABLE_ROM = 0x001AC870
RELOC_ENTRY_SIZE = 12
VANILLA_US_FILE_COUNT = 2132

# Remix assembles its code and inserted data at ROM 0x02C00000, loaded at
# RAM 0x80400000 (main.asm: `origin 0x02C00000; base 0x80400000`).
REMIX_CODE_ROM = 0x02C00000
REMIX_CODE_RAM = 0x80400000

# The vanilla fighter overlay that holds FTData, motion-desc and status-desc
# tables maps RAM to ROM by this constant (Character.asm: `- 0x80084800`).
FT_OVERLAY_RAM_MINUS_ROM = 0x80084800
# The vanilla menu-motion arrays use this one (`- 0x80288A20`).
MENU_OVERLAY_RAM_MINUS_ROM = 0x80288A20

O2R_HEADER = (
    b"\x00\x00\x00\x00" + b"OLER" + b"\x00\x00\x00\x00"
    + b"\xef\xbe\xad\xde\xef\xbe\xad\xde" + b"\x00" * 0x2C
)
assert len(O2R_HEADER) == 0x40

NO_RELOC = 0xFFFF


def _load_vpk0():
    path = _SCRIPTS / "extract-battleship-relocdata.py"
    spec = importlib.util.spec_from_file_location("_p4_vpk0", path)
    mod = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = mod  # its dataclasses resolve their module by name
    spec.loader.exec_module(mod)
    return mod.decode_vpk0


_decode_vpk0 = None


def decode_vpk0(data: bytes) -> bytes:
    global _decode_vpk0
    if _decode_vpk0 is None:
        _decode_vpk0 = _load_vpk0()
    return _decode_vpk0(data)[0]


def sha1(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


@dataclass(frozen=True)
class TableEntry:
    compressed: bool
    data_offset: int
    reloc_intern: int
    compressed_words: int
    reloc_extern: int
    decompressed_words: int


class RomError(RuntimeError):
    pass


class Rom:
    """A z64 (big-endian) SSB64 image with its reloc table discovered."""

    def __init__(self, path: Path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        if self.data[:4] != b"\x80\x37\x12\x40":
            raise RomError(f"{path}: not a big-endian z64 image")
        self.file_count = self._discover_file_count()
        self.data_start = RELOC_TABLE_ROM + (self.file_count + 1) * RELOC_ENTRY_SIZE
        self._file_cache: dict[int, bytes] = {}

    # -- table ---------------------------------------------------------------
    def _raw_entry(self, index: int) -> TableEntry:
        off = RELOC_TABLE_ROM + index * RELOC_ENTRY_SIZE
        word, ri, cs, re_, ds = struct.unpack(">IHHHH", self.data[off:off + 12])
        return TableEntry(bool(word >> 31), word & 0x7FFFFFFF, ri, cs, re_, ds)

    def _discover_file_count(self) -> int:
        # The sentinel is the first entry whose sizes and chains are all zero;
        # Remix grows the table in place, so the count is not a constant.
        prev = -1
        for index in range(0, 0x4000):
            e = self._raw_entry(index)
            if e.data_offset < prev:
                raise RomError(f"reloc table offsets not monotonic at {index}")
            if (e.compressed_words == 0 and e.decompressed_words == 0
                    and e.reloc_intern == 0 and e.reloc_extern == 0 and index > 0):
                return index
            prev = e.data_offset
        raise RomError("reloc table sentinel not found")

    def entry(self, file_id: int) -> TableEntry:
        if not 0 <= file_id < self.file_count:
            raise RomError(f"file {file_id:#x} outside table of {self.file_count}")
        return self._raw_entry(file_id)

    def file_bytes(self, file_id: int) -> bytes:
        cached = self._file_cache.get(file_id)
        if cached is not None:
            return cached
        e = self.entry(file_id)
        start = self.data_start + e.data_offset
        size = e.decompressed_words * 4
        if e.compressed:
            raw = self.data[start:start + e.compressed_words * 4]
            out = decode_vpk0(raw)
            # Torch's decoder writes into a buffer of the table's size; a final
            # back-reference may run past it (Remix 0x153F: 11,833 of 11,832).
            if len(out) < size:
                raise RomError(f"file {file_id:#x}: vpk0 gave {len(out)} of {size}")
            out = out[:size]
        else:
            out = self.data[start:start + size]
        self._file_cache[file_id] = out
        return out

    def extern_ids(self, file_id: int) -> list[int]:
        """The u16 file IDs between this file's data and the next file's.

        Vanilla files carry exactly one ID per external-chain slot. Files the
        donor's injector appended can trail unchained IDs (Remix 0x8AC: eight
        copies of 299, empty chain) that the source loader never reads, so the
        list is cut to the chain's length; vanilla lists are unchanged."""
        e = self.entry(file_id)
        nxt = self._raw_entry(file_id + 1)
        lo = self.data_start + e.data_offset + e.compressed_words * 4
        hi = self.data_start + nxt.data_offset
        if hi < lo:
            raise RomError(f"file {file_id:#x}: extern region inverted")
        ids = [int.from_bytes(self.data[a:a + 2], "big") for a in range(lo, hi - 1, 2)]
        return ids[:self._extern_chain_length(file_id)]

    def _extern_chain_length(self, file_id: int) -> int:
        e = self.entry(file_id)
        if e.reloc_extern == NO_RELOC:
            return 0
        data = self.file_bytes(file_id)
        w, n = e.reloc_extern, 0
        while w != NO_RELOC:
            off = w * 4
            if off + 4 > len(data) or n > len(data) // 4:
                raise RomError(f"file {file_id:#x}: extern chain leaves file")
            n += 1
            w = int.from_bytes(data[off:off + 4], "big") >> 16
        return n

    def o2r(self, file_id: int) -> bytes:
        e = self.entry(file_id)
        ext = self.extern_ids(file_id)
        body = self.file_bytes(file_id)
        out = bytearray(O2R_HEADER)
        out += struct.pack("<IHHI", file_id, e.reloc_intern, e.reloc_extern, len(ext))
        for x in ext:
            out += struct.pack("<H", x)
        out += struct.pack("<I", len(body))
        out += body
        return bytes(out)

    # -- relocation ----------------------------------------------------------
    def reloc_slots(self, file_id: int) -> dict[int, tuple]:
        """Map byte offset of each pointer slot to ('intern', target_offset) or
        ('extern', extern_file_id, target_offset)."""
        e = self.entry(file_id)
        data = self.file_bytes(file_id)
        slots: dict[int, tuple] = {}
        w = e.reloc_intern
        guard = 0
        while w != NO_RELOC:
            off = w * 4
            if off + 4 > len(data):
                raise RomError(f"file {file_id:#x}: intern chain leaves file at {off:#x}")
            val = int.from_bytes(data[off:off + 4], "big")
            slots[off] = ("intern", (val & 0xFFFF) * 4)
            w = val >> 16
            guard += 1
            if guard > len(data):
                raise RomError(f"file {file_id:#x}: intern chain cycles")
        ext = self.extern_ids(file_id)
        w = e.reloc_extern
        k = 0
        while w != NO_RELOC:
            off = w * 4
            if off + 4 > len(data):
                raise RomError(f"file {file_id:#x}: extern chain leaves file at {off:#x}")
            val = int.from_bytes(data[off:off + 4], "big")
            if k >= len(ext):
                raise RomError(f"file {file_id:#x}: extern chain longer than id list")
            slots[off] = ("extern", ext[k], (val & 0xFFFF) * 4)
            k += 1
            w = val >> 16
        return slots

    # -- address spaces ------------------------------------------------------
    def remix_rom_offset(self, ram: int) -> int:
        if not REMIX_CODE_RAM <= ram < REMIX_CODE_RAM + (len(self.data) - REMIX_CODE_ROM):
            raise RomError(f"{ram:#010x} outside the Remix code region")
        return REMIX_CODE_ROM + (ram - REMIX_CODE_RAM)

    def read_ram(self, ram: int, size: int) -> bytes:
        """Read Remix-region or fighter-overlay RAM addresses from the image."""
        if ram >= REMIX_CODE_RAM:
            off = self.remix_rom_offset(ram)
        else:
            off = ram - FT_OVERLAY_RAM_MINUS_ROM
        return self.data[off:off + size]

    def u32_ram(self, ram: int) -> int:
        return int.from_bytes(self.read_ram(ram, 4), "big")

    def u32(self, rom_off: int) -> int:
        return int.from_bytes(self.data[rom_off:rom_off + 4], "big")


def load_symbols(path: Path) -> tuple[dict[str, int], dict[int, list[str]]]:
    """bass `-sym` log: `<hex address> <scoped.name>` per line."""
    by_name: dict[str, int] = {}
    by_addr: dict[int, list[str]] = {}
    for line in Path(path).read_text(encoding="utf-8", errors="replace").splitlines():
        parts = line.split()
        if len(parts) != 2:
            continue
        try:
            addr = int(parts[0], 16)
        except ValueError:
            continue
        by_name.setdefault(parts[1], addr)
        by_addr.setdefault(addr, []).append(parts[1])
    return by_name, by_addr


def load_decomp_symbols(path: Path) -> dict[int, str]:
    """decomp symbols/symbols_us.txt: `name = 0xADDR;` per line."""
    out: dict[int, str] = {}
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        line = line.split("//")[0].strip().rstrip(";")
        if "=" not in line:
            continue
        name, _, value = line.partition("=")
        try:
            addr = int(value.strip(), 16)
        except ValueError:
            continue
        out.setdefault(addr, name.strip())
    return out


def battleship_o2r_index(yaml_dir: Path) -> dict[int, tuple[str, str]]:
    """file_id -> (group folder, symbol) from BattleShip's yamls/us/reloc_*.yml."""
    out: dict[int, tuple[str, str]] = {}
    for yml in sorted(Path(yaml_dir).glob("reloc_*.yml")):
        group = yml.stem
        symbol = None
        for raw in yml.read_text(encoding="utf-8").splitlines():
            line = raw.strip()
            if line.startswith("symbol:"):
                symbol = line.split(":", 1)[1].strip()
            elif line.startswith("file_id:") and symbol is not None:
                out[int(line.split(":", 1)[1].strip(), 0)] = (group, symbol)
                symbol = None
    return out


def verify_vanilla_o2r(rom: Rom, o2r_root: Path, yaml_dir: Path) -> dict:
    """Compare this writer against Torch's export for every indexed vanilla file."""
    index = battleship_o2r_index(yaml_dir)
    same = differ = missing = 0
    first_diff = None
    for file_id, (group, symbol) in sorted(index.items()):
        path = Path(o2r_root) / group / symbol
        if not path.exists():
            missing += 1
            continue
        if path.read_bytes() == rom.o2r(file_id):
            same += 1
        else:
            differ += 1
            if first_diff is None:
                first_diff = (file_id, f"{group}/{symbol}")
    return {"identical": same, "different": differ, "missing_reference": missing,
            "first_difference": first_diff, "indexed": len(index)}
