"""Exercise the actual FAT extent producer/ARM7 lookup against synthetic media."""
import ctypes as C
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

import pytest

ROOT = Path(__file__).resolve().parents[2]


class Volume(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in (
        "fat_sector", "fat_sectors", "data_sector", "cluster_count", "cluster_sectors", "entry_bits")]


class Extent(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in ("file_sector", "device_sector", "sector_count")]


Reader = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint8))


@pytest.fixture(scope="module")
def lib():
    cc = shutil.which("gcc") or shutil.which("clang")
    assert cc
    with tempfile.TemporaryDirectory(prefix="a8-fat-", dir=ROOT / "builds") as tmp:
        dll = Path(tmp) / "extent.dll"
        result = subprocess.run([cc, "-std=c11", "-shared", "-O2", "-Wall", "-Wextra", "-Werror",
                                 "-I", str(ROOT / "include"),
                                 str(ROOT / "src/nds/nds_audio_extent.c"), "-o", str(dll)],
                                capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        module = C.CDLL(str(dll))
        module.ndsAudioFatOpen.argtypes = [C.POINTER(Volume), Reader, C.c_void_p, C.c_uint32, C.c_uint32]
        module.ndsAudioFatExtents.argtypes = [C.POINTER(Volume), Reader, C.c_void_p, C.c_uint32,
                                            C.c_uint32, C.POINTER(Extent), C.c_uint32, C.POINTER(C.c_uint32)]
        module.ndsAudioExtentsValid.argtypes = [C.POINTER(Extent), C.c_uint32, C.c_uint32, C.c_uint32]
        module.ndsAudioExtentResolve.argtypes = [C.POINTER(Extent), C.c_uint32, C.c_uint32,
                                                C.c_uint32, C.POINTER(C.c_uint32), C.POINTER(C.c_uint32)]
        yield module
        if __import__("os").name == "nt":
            C.windll.kernel32.FreeLibrary(C.c_void_p(module._handle))


class Fat:
    def __init__(self, bits, cluster_sectors=4):
        self.bits, self.spc, self.partition = bits, cluster_sectors, 123
        self.clusters = {12: 1000, 16: 5000, 32: 70000}[bits]
        self.reserved, roots = (32, 0) if bits == 32 else (1, 16)
        self.fat_sectors = ((self.clusters + 2) * bits + 4095) // 4096
        self.active = 1 if bits == 32 else 0
        self.fat_start = self.partition + self.reserved + self.active * self.fat_sectors
        self.data = self.partition + self.reserved + 2 * self.fat_sectors + (roots * 32 + 511) // 512
        self.total = self.data - self.partition + self.clusters * self.spc
        self.boot = bytearray(512)
        struct.pack_into("<HBHBHH", self.boot, 11, 512, self.spc, self.reserved, 2, roots,
                         self.total if self.total < 65536 else 0)
        struct.pack_into("<H", self.boot, 22, 0 if bits == 32 else self.fat_sectors)
        struct.pack_into("<I", self.boot, 32, self.total)
        if bits == 32:
            struct.pack_into("<IH", self.boot, 36, self.fat_sectors, 0x81)
        self.boot[510:] = b"\x55\xaa"
        self.fat = bytearray(self.fat_sectors * 512)
        self.fail_sector = None
        self.reader = Reader(self.read)

    def read(self, _, sector, out):
        if sector == self.fail_sector:
            return 0
        if sector == self.partition:
            data = self.boot
        elif self.fat_start <= sector < self.fat_start + self.fat_sectors:
            off = (sector - self.fat_start) * 512
            data = self.fat[off:off+512]
        else:
            data = bytes(512)  # inactive FAT copy is deliberately wrong
        C.memmove(out, bytes(data), 512)
        return 1

    def entry(self, cluster, value):
        if self.bits == 12:
            offset = cluster * 3 // 2
            old, = struct.unpack_from("<H", self.fat, offset)
            shift = 4 if cluster & 1 else 0
            struct.pack_into("<H", self.fat, offset, (old & ~(0xfff << shift)) | ((value & 0xfff) << shift))
        else:
            struct.pack_into("<H" if self.bits == 16 else "<I", self.fat,
                             cluster * (self.bits // 8), value)

    def open(self, lib):
        v = Volume()
        assert lib.ndsAudioFatOpen(C.byref(v), self.reader, None, self.partition, self.partition + self.total)
        assert v.entry_bits == self.bits and v.fat_sector == self.fat_start and v.data_sector == self.data
        return v


@pytest.mark.parametrize("bits", [12, 16, 32])
def test_fragmented_chain_active_fat_and_all_sector_lookups(lib, bits):
    disk = Fat(bits)
    # FAT12 entry 341 straddles a physical FAT-sector boundary.
    chain = [341, 342, 100, 400, 401]
    for a, b in zip(chain, chain[1:]):
        disk.entry(a, b | (0xa0000000 if bits == 32 else 0))
    disk.entry(chain[-1], {12: 0xfff, 16: 0xffff, 32: 0xffffffff}[bits])
    v = disk.open(lib)
    file_bytes = (len(chain) - 1) * disk.spc * 512 + 513
    n = C.c_uint32()
    assert lib.ndsAudioFatExtents(C.byref(v), disk.reader, None, chain[0], file_bytes, None, 0, C.byref(n))
    assert n.value == 3
    out = (Extent * n.value)()
    assert lib.ndsAudioFatExtents(C.byref(v), disk.reader, None, chain[0], file_bytes, out, n.value, C.byref(n))
    assert lib.ndsAudioExtentsValid(out, n.value, file_bytes, disk.partition + disk.total)
    expected = [disk.data + (c - 2) * disk.spc + k for c in chain for k in range(disk.spc)]
    expected = expected[:(file_bytes + 511)//512]
    for sector, lba in enumerate(expected):
        actual, avail = C.c_uint32(), C.c_uint32()
        assert lib.ndsAudioExtentResolve(out, n.value, sector, 100, C.byref(actual), C.byref(avail))
        assert actual.value == lba
        consecutive = 1
        while sector + consecutive < len(expected) and expected[sector+consecutive] == lba + consecutive:
            consecutive += 1
        assert avail.value == consecutive
    assert not lib.ndsAudioExtentResolve(out, n.value, len(expected), 1, C.byref(actual), C.byref(avail))
    assert not lib.ndsAudioExtentResolve(out, n.value, 0, 0, C.byref(actual), C.byref(avail))


@pytest.mark.parametrize("bits", [12, 16, 32])
def test_bad_chain_early_end_cycle_capacity_and_io(lib, bits):
    disk = Fat(bits)
    v = disk.open(lib)
    out, n = (Extent * 8)(), C.c_uint32()
    def build(clusters=3, capacity=8):
        return lib.ndsAudioFatExtents(C.byref(v), disk.reader, None, 2,
                                      clusters * disk.spc * 512, out, capacity, C.byref(n))
    assert not build()  # free cluster in chain
    disk.entry(2, {12: 0xfff, 16: 0xffff, 32: 0xffffffff}[bits])
    assert not build()  # early EOF
    disk.entry(2, 3); disk.entry(3, 2)
    assert not build()  # cycle ends exactly at the requested prefix
    disk.entry(3, 8)
    assert not build(capacity=1)  # second physical extent cannot be dropped
    disk.fail_sector = disk.fat_start
    assert not build()
    disk.fail_sector = None
    disk.entry(2, {12: 0xff7, 16: 0xfff7, 32: 0x0ffffff7}[bits])
    assert not build()


def test_bad_geometry_and_map_admission(lib):
    for field, data in [(11,b"\x00\x04"),(13,b"\x03"),(14,b"\x00\x00"),
                        (16,b"\x00"),(22,b"\x01\x00"),(510,b"\x00\x00")]:
        disk = Fat(16)
        disk.boot[field:field+len(data)] = data
        assert not lib.ndsAudioFatOpen(C.byref(Volume()), disk.reader, None, disk.partition, disk.partition+disk.total)
    disk = Fat(32); struct.pack_into("<H", disk.boot, 40, 0x82)
    assert not lib.ndsAudioFatOpen(C.byref(Volume()), disk.reader, None, disk.partition, disk.partition+disk.total)
    for entries in [[(0,100,4),(5,200,4)], [(0,100,4),(4,103,4)],
                    [(0,0xfffffff0,32)], [(0,100,0)], [(0,100,4)]]:
        out = (Extent * len(entries))(*(Extent(*e) for e in entries))
        assert not lib.ndsAudioExtentsValid(out, len(entries), 8*512, 0xffffffff)
