"""Run the production BGA1 reader on every packed track and corrupt inputs."""
import ctypes as C
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

import pytest

ROOT = Path(__file__).resolve().parents[2]
Reader = C.CFUNCTYPE(C.c_int, C.c_uint32, C.c_void_p, C.c_uint32)


class Spec(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in (
        "rom_offset", "file_bytes", "source_samples", "loop_sample",
        "packet_count", "loop_packet", "loop_record", "track_id")]


class Stream(C.Structure):
    _fields_ = [("spec", Spec), *[(n, C.c_uint32) for n in
                ("offset", "next_packet", "error", "loop_count", "cycle_samples")],
                ("source_samples_loaded", C.c_uint64)]


class Packet(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in ("samples", "bytes", "loop_restart", "final")]


@pytest.fixture(scope="module")
def reader_lib():
    cc = shutil.which("gcc") or shutil.which("clang")
    assert cc
    with tempfile.TemporaryDirectory(prefix="a8-bgm-", dir=ROOT / "builds") as tmp:
        dll = Path(tmp) / "bgm.dll"
        result = subprocess.run([cc, "-std=c11", "-shared", "-O2", "-Wall", "-Wextra", "-Werror",
                                 "-I", str(ROOT / "include"), str(ROOT / "src/nds/nds_bgm_stream.c"),
                                 "-o", str(dll)], capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        lib = C.CDLL(str(dll))
        lib.ndsBgmStreamOpen.argtypes = [C.POINTER(Stream), C.POINTER(Spec), Reader]
        lib.ndsBgmStreamRead.argtypes = [C.POINTER(Stream), C.POINTER(Packet), C.c_void_p, Reader]
        yield lib
        if __import__("os").name == "nt": C.windll.kernel32.FreeLibrary(C.c_void_p(lib._handle))


def source(data):
    fields = struct.unpack_from("<IHH8I", data)
    spec = Spec(0x10000, len(data), fields[4], fields[5], fields[7], fields[8], fields[9], 0)
    mutable = bytearray(data)
    fail_offset = [None]
    @Reader
    def read(offset, out, size):
        offset -= spec.rom_offset
        if offset == fail_offset[0] or offset < 0 or size > len(mutable) - offset: return 0
        C.memmove(out, bytes(mutable[offset:offset+size]), size)
        return 1
    return spec, mutable, read, fail_offset


def test_all_track_packets_and_source_loop_boundaries(reader_lib):
    paths = sorted((ROOT / "assets/audio").glob("bgm_*_ima.bin"))
    assert len(paths) >= 47
    for path in paths:
        data = path.read_bytes()
        spec, _, read, _ = source(data)
        stream, packet, buf = Stream(), Packet(), (C.c_uint8 * 8196)()
        assert reader_lib.ndsBgmStreamOpen(C.byref(stream), C.byref(spec), read), path.name
        cursor, loaded = 40, 0
        total = spec.packet_count + (spec.packet_count - spec.loop_packet if spec.loop_sample != 0xffffffff else 0)
        for i in range(total):
            loop = int(i == spec.packet_count)
            if loop: cursor = spec.loop_record
            samples, size = struct.unpack_from("<II", data, cursor)
            assert reader_lib.ndsBgmStreamRead(C.byref(stream), C.byref(packet), buf, read) == 1, (path.name, i, stream.error)
            assert (packet.samples, packet.bytes, packet.loop_restart) == (samples, size, loop)
            assert packet.final == (spec.loop_sample == 0xffffffff and i+1 == spec.packet_count)
            assert bytes(buf[:size]) == data[cursor+8:cursor+8+size]
            loaded += samples; cursor += 8 + size
        assert stream.source_samples_loaded == loaded
        assert loaded == spec.source_samples + (spec.source_samples - spec.loop_sample if spec.loop_sample != 0xffffffff else 0)
        if spec.loop_sample == 0xffffffff:
            assert reader_lib.ndsBgmStreamRead(C.byref(stream), C.byref(packet), buf, read) == 0


def test_corruption_rejected_without_publishing_packet(reader_lib):
    data = (ROOT / "assets/audio/bgm_pupupu_ima.bin").read_bytes()
    for offset in (0, 4, 6, 8, 12, 16, 20, 24, 28, 32, 36):
        spec, changed, read, _ = source(data)
        changed[offset] ^= 1
        assert not reader_lib.ndsBgmStreamOpen(C.byref(Stream()), C.byref(spec), read)
    for offset, value in ((40, 0), (40, 16385), (44, 4), (44, 8200)):
        spec, changed, read, _ = source(data)
        stream, packet, buf = Stream(), Packet(91,92,93), (C.c_uint8 * 8196)()
        assert reader_lib.ndsBgmStreamOpen(C.byref(stream), C.byref(spec), read)
        struct.pack_into("<I", changed, offset, value)
        assert reader_lib.ndsBgmStreamRead(C.byref(stream), C.byref(packet), buf, read) == -1
        assert (packet.samples, packet.bytes, packet.loop_restart) == (91,92,93)
        assert stream.offset == 40 and stream.next_packet == 0
    for offset in (0,40,48):
        spec, _, read, fail = source(data)
        stream, packet, buf = Stream(), Packet(), (C.c_uint8 * 8196)()
        if offset == 0:
            fail[0] = 0
            assert not reader_lib.ndsBgmStreamOpen(C.byref(stream), C.byref(spec), read)
        else:
            assert reader_lib.ndsBgmStreamOpen(C.byref(stream), C.byref(spec), read)
            fail[0] = offset
            assert reader_lib.ndsBgmStreamRead(C.byref(stream), C.byref(packet), buf, read) == -1
        assert stream.error == 3
