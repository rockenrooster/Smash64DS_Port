"""Host-execute the A8 request validator and actual ARM9 range reader.

The fake peer asserts cache-line ownership and copies deterministic ROM bytes.
Target tests still owe the real PXI/cache/card handshake and sound/input service.
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import pytest

ROOT = Path(__file__).resolve().parents[2]


def run_c(text, extra_flags=()):
    cc = next((shutil.which(c) for c in ("gcc", "clang", "cc") if shutil.which(c)), None)
    assert cc, "host C compiler required"
    with tempfile.TemporaryDirectory(prefix="a8-storage-", dir=ROOT / "builds") as tmp:
        source = Path(tmp) / "storage.c"
        binary = Path(tmp) / "storage.exe"
        source.write_text(text)
        compile_run = subprocess.run([cc, "-std=c11", "-O2", "-Wall", "-Wextra",
                                      "-Werror", "-I", str(ROOT / "include"),
                                      str(source), "-o", str(binary), *extra_flags],
                                     text=True, capture_output=True)
        assert compile_run.returncode == 0, compile_run.stderr
        result = subprocess.run([str(binary)], text=True, capture_output=True)
        assert result.returncode == 0, result.stdout + result.stderr


def function(source, name):
    m = re.search(r"^(?:static )?(?:bool|int|uint32_t) " + name + r"\([^;]*?\)\s*\{", source, re.M)
    assert m
    depth, end = 1, source.index("{", m.start()) + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[m.start():end]


def test_request_bounds_and_reply_identity():
    run_c(r'''
#include <assert.h>
#include <nds/nds_audio_storage.h>
int main(void) {
    NdsAudioStorageRequest r = {NDS_AUDIO_STORAGE_ABI, NDS_AUDIO_STORAGE_READ_CARD,
                               1, 0x100, 0x02001000, 512, {0,0}};
    const uint32_t descriptor = 0x02000000, size = 0x10000;
    assert(ndsAudioStorageValidate(&r, descriptor, size));
    assert(_Alignof(NdsAudioStorageRequest) == 32);
    r.abi ^= 1; assert(!ndsAudioStorageValidate(&r, descriptor, size)); r.abi ^= 1;
    r.sequence = 0; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.sequence = 0x10000; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.sequence = 0xffff;
    r.reserved[1] = 1; assert(!ndsAudioStorageValidate(&r, descriptor, size)); r.reserved[1] = 0;
    r.destination = descriptor; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.destination = descriptor + 16; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.destination = 0x01ffffe0; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.destination = 0x023f0000; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.destination = 0x023effe0; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.bytes = 32; assert(ndsAudioStorageValidate(&r, descriptor, size));
    r.destination = 0x02001001; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.destination = 0x02001000;
    r.bytes = 31; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.bytes = 0; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.bytes = NDS_AUDIO_STORAGE_MAX_READ + 32; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.bytes = 32; r.offset = size - 32; assert(ndsAudioStorageValidate(&r, descriptor, size));
    r.offset++; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.offset = UINT32_MAX; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    r.operation = NDS_AUDIO_STORAGE_OPEN_MAP; r.offset = 0;
    r.destination = descriptor + 32; r.bytes = sizeof(NdsAudioStorageMap);
    assert(ndsAudioStorageValidate(&r, descriptor, size));
    r.destination = descriptor; assert(!ndsAudioStorageValidate(&r, descriptor, size));
    for (uint32_t operation = 1; operation <= 4; ++operation) {
        r.operation = operation; r.offset = r.destination = r.bytes = 0;
        assert(ndsAudioStorageValidate(&r, descriptor, size) ==
               (operation == NDS_AUDIO_STORAGE_OPEN_CARD || operation == NDS_AUDIO_STORAGE_CLOSE_CARD));
    }
    assert(ndsAudioStorageReply(0xffff, 3) < (1u << 26));
    assert(ndsAudioStorageReply(2, 0) != ndsAudioStorageReply(1, 0));
    assert(ndsAudioStorageReply(1, 3) != ndsAudioStorageReply(1, 0));
    return 0;
}
''')


def test_file_header_size_survives_libnds_argv_alias():
    source = (ROOT / "src/nds/nds_audio_storage.c").read_text()
    fixture = r'''
#include <assert.h>
#include <stdint.h>
#include <nds/nds_audio_storage.h>
static struct {
    uint32_t device_capacity, ntr_rom_size;
    uint32_t fat_rom_offset, fat_size, fnt_rom_offset, fnt_size;
} env = {9, 0x02fff160, 0x20f600, 0x2130, 0x20ae00, 0x466e};
#define g_envAppNdsHeader (&env)
''' + function(source, "ndsAudioStorageHeaderWord") + "\n" + function(
        source, "ndsAudioStorageHeaderSize") + r'''
static void word(uint8_t *p, uint32_t v) {
    for (unsigned i=0;i<4;++i) p[i]=(uint8_t)(v>>(8*i));
}
int main(void) {
    uint8_t header[512]={0};
    word(header+0x40,env.fnt_rom_offset);word(header+0x44,env.fnt_size);
    word(header+0x48,env.fat_rom_offset);word(header+0x4c,env.fat_size);
    word(header+0x80,0x03c42000);
    assert(ndsAudioStorageHeaderSize(header,0x03c44800)==0x03c42000);
    assert(ndsAudioStorageHeaderSize(header,0x03c42000-1)==0);
    for (unsigned i=0x40;i<0x50;i+=4) {
        header[i]^=1;assert(!ndsAudioStorageHeaderSize(header,0x03c44800));header[i]^=1;
    }
    env.device_capacity=8;assert(!ndsAudioStorageHeaderSize(header,0x03c44800));
    assert(!ndsAudioStorageCardCapacity(13));
    return 0;
}
'''
    run_c(fixture)
    # A mutation that reads the overwritten RAM field must fail this fixture.
    mutant = fixture.replace("ndsAudioStorageHeaderWord(header + 0x80)",
                              "g_envAppNdsHeader->ntr_rom_size")
    assert mutant != fixture
    with pytest.raises(AssertionError):
        run_c(mutant)


def test_actual_arm7_mapped_reader_across_fragments():
    source = (ROOT / "src/nds/arm7/nds_audio_main.c").read_text()
    run_c(r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <nds/nds_audio_extent.h>
typedef unsigned BlkDevice;
static struct { uintptr_t extents; unsigned extent_count, device; } sFileMap;
static uint8_t sSectorBuffer[512] __attribute__((aligned(4)));
static uint8_t disk[32*512], expected[8*512], result[8*512+32];
static unsigned calls, fail_at;
static bool blkDevReadSectors(unsigned dev, void *out, uint32_t sector, uint32_t count) {
    assert(dev == 0 && ((uintptr_t)out & 3) == 0 && count > 0 && sector + count <= 32);
    if (++calls == fail_at) return false;
    memcpy(out, disk + sector * 512, count * 512);
    return true;
}
''' + f'\n#include "{(ROOT / "src/nds/nds_audio_extent.c").as_posix()}"\n' + function(
        source, "ndsAudioStorageReadMap") + r'''
int main(void) {
    const NdsAudioExtent map[3] = {{0,10,3},{3,2,4},{7,25,1}};
    sFileMap.extents = (uintptr_t)map; sFileMap.extent_count = 3;
    for (unsigned i = 0; i < sizeof(disk); ++i) disk[i] = (uint8_t)(i ^ (i >> 8));
    for (unsigned i = 0; i < 3; ++i)
        memcpy(expected + map[i].file_sector * 512, disk + map[i].device_sector * 512, map[i].sector_count * 512);
    const unsigned sizes[] = {1,31,32,511,512,1001,2049};
    for (unsigned offset = 0; offset < sizeof(expected); offset += 17)
        for (unsigned lane = 0; lane < 4; ++lane)
            for (unsigned i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i) {
                unsigned n = sizes[i] < sizeof(expected) - offset ? sizes[i] : sizeof(expected) - offset;
                memset(result, 0x5a, sizeof(result));
                assert(ndsAudioStorageReadMap(offset, result + lane, n));
                assert(!memcmp(result + lane, expected + offset, n));
                assert(result[lane+n] == 0x5a);
            }
    calls = 0; fail_at = 2;
    assert(!ndsAudioStorageReadMap(0, result, sizeof(expected)) && calls == 2);
    fail_at = 0;
    assert(!ndsAudioStorageReadMap(sizeof(expected), result, 32));
    return 0;
}
''')


def test_real_reader_partial_lines_tail_and_failure():
    source = (ROOT / "src/nds/nds_audio_storage.c").read_text()
    run_c(r'''
#define _GNU_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <nds/nds_audio_storage.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
static uint8_t rom_data[65539];
static uint32_t sRomBytes = sizeof(rom_data);
static uint8_t sStorageBounce[512] __attribute__((aligned(32)));
static NdsAudioStorageRequest sStorageRequest;
static unsigned sStorageMutex, calls, fail_at;
static uint32_t gNdsAudioStorageReads, gNdsAudioStorageBytes, gNdsAudioStorageBounceBytes;
static void mutexLock(unsigned *m) { assert((*m)++ == 0); }
static void mutexUnlock(unsigned *m) { assert(--(*m) == 0); }
static int ndsAudioStorageCall(uint32_t operation, uint32_t offset, void *dst, uint32_t bytes) {
    assert(operation == NDS_AUDIO_STORAGE_READ_CARD && sStorageMutex == 1);
    assert((((uintptr_t)dst | bytes) & 31) == 0);
    assert(bytes > 0 && bytes <= NDS_AUDIO_STORAGE_MAX_READ);
    assert(offset <= sizeof(rom_data) && bytes <= sizeof(rom_data) - offset);
    if (++calls == fail_at) return 0;
    memcpy(dst, rom_data + offset, bytes);
    return 1;
}
''' + function(source, "ndsAudioStorageReadCard") + r'''
int main(void) {
    for (size_t i = 0; i < sizeof(rom_data); ++i) rom_data[i] = (uint8_t)(i ^ (i >> 8));
    uint8_t *buffer;
#ifdef _WIN32
    buffer = VirtualAlloc((void *)0x02010000, 131072, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    buffer = mmap((void *)0x02010000, 131072, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
#endif
    assert(buffer == (void *)0x02010000);
    // All source/destination lane combinations; adjacent dirty bytes survive.
    for (unsigned from = 0; from < 32; ++from) {
        for (unsigned to = 0; to < 32; ++to) {
            memset(buffer, 0xa5, 2048);
            assert(ndsAudioStorageReadCard(NULL, from, buffer + 32 + to, 1001));
            assert(!memcmp(buffer + 32 + to, rom_data + from, 1001));
            for (unsigned i = 0; i < 32 + to; ++i) assert(buffer[i] == 0xa5);
            for (unsigned i = 32 + to + 1001; i < 2048; ++i) assert(buffer[i] == 0xa5);
        }
    }
    // An unaligned destination bounces only its head and tail lines.
    calls = 0;
    memset(buffer, 0xa5, 24576);
    assert(ndsAudioStorageReadCard(NULL, 5, buffer + 7, 20000));
    assert(!memcmp(buffer + 7, rom_data + 5, 20000));
    for (unsigned i = 0; i < 7; ++i) assert(buffer[i] == 0xa5);
    for (unsigned i = 20007; i < 24576; ++i) assert(buffer[i] == 0xa5);
    assert(calls == 3);
    // Cross the peer's bounded-read limit on the aligned fast path.
    calls = 0;
    assert(ndsAudioStorageReadCard(NULL, 1, buffer, 65536));
    assert(calls == 2 && !memcmp(buffer, rom_data + 1, 65536));
    for (unsigned count = 1; count <= 512; ++count) {
        assert(ndsAudioStorageReadCard(NULL, sizeof(rom_data) - count, buffer + 1, count));
        assert(!memcmp(buffer + 1, rom_data + sizeof(rom_data) - count, count));
    }
    unsigned before = calls;
    assert(ndsAudioStorageReadCard(NULL, sizeof(rom_data), NULL, 0));
    assert(!ndsAudioStorageReadCard(NULL, sizeof(rom_data), buffer, 1));
    assert(!ndsAudioStorageReadCard(NULL, UINT32_MAX, buffer, 32));
    assert(!ndsAudioStorageReadCard(NULL, 1, buffer, UINT32_MAX));
    assert(!ndsAudioStorageReadCard(NULL, 1, NULL, 32));
    sRomBytes = 20;
    assert(!ndsAudioStorageReadCard(NULL, 0, buffer, 20));
    sRomBytes = sizeof(rom_data);
    assert(calls == before && !sStorageMutex);
    calls = 0; fail_at = 2;
    assert(!ndsAudioStorageReadCard(NULL, 0, buffer, 65536));
    assert(calls == 2 && !sStorageMutex);
    assert(gNdsAudioStorageBounceBytes > 0 && gNdsAudioStorageBytes > 65536);
#ifdef _WIN32
    assert(VirtualFree(buffer, 0, MEM_RELEASE));
#else
    assert(munmap(buffer, 131072) == 0);
#endif
    return 0;
}
''')


def test_real_call_cache_handoff_and_acknowledgement():
    source = (ROOT / "src/nds/nds_audio_storage.c").read_text()
    run_c(r'''
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <nds/nds_audio_storage.h>
typedef unsigned PxiChannel;
static NdsAudioStorageRequest sStorageRequest;
static uint32_t sStorageSequence;
static uint32_t gNdsAudioStorageRequests, gNdsAudioStorageFailures;
static uint32_t gNdsAudioStorageWaitTicks64, gNdsAudioStorageWaitMaxTicks64, ticks;
static uint32_t tickGetCount(void) { return ticks += 7u; }
static uint8_t output[64] __attribute__((aligned(32)));
static unsigned order, failure, empty;
static void DC_FlushRange(void *address, size_t bytes) {
    if (address == &sStorageRequest) {
        assert(order++ == (empty ? 0u : 1u) && bytes == 32);
        assert(sStorageRequest.abi == NDS_AUDIO_STORAGE_ABI);
        assert(!sStorageRequest.reserved[0] && !sStorageRequest.reserved[1]);
    } else {
        assert(order++ == 0 && address == output && bytes == sizeof(output));
    }
}
static uint32_t pxiSendAndReceive(PxiChannel channel, uint32_t packet) {
    assert(channel == NDS_AUDIO_STORAGE_CHANNEL && order++ == (empty ? 1u : 2u));
    assert(packet == (uint32_t)(uintptr_t)&sStorageRequest >> 5);
    if (!empty) memset(output, 0x65, sizeof(output));
    return ndsAudioStorageReply(sStorageRequest.sequence + (failure == 1),
                                failure == 2 ? NDS_AUDIO_STORAGE_IO_ERROR : 0);
}
static void DC_InvalidateRange(void *address, size_t bytes) {
    assert(order++ == 3 && address == output && bytes == sizeof(output));
}
''' + function(source, "ndsAudioStorageCall") + r'''
int main(void) {
    memset(&sStorageRequest, 0xdd, sizeof(sStorageRequest));
    sStorageSequence = 0xffff;
    assert(ndsAudioStorageCall(NDS_AUDIO_STORAGE_READ_CARD, 100, output, sizeof(output)));
    assert(order == 4 && sStorageRequest.sequence == 1 && !gNdsAudioStorageFailures);
    for (unsigned i = 0; i < sizeof(output); ++i) assert(output[i] == 0x65);
    for (failure = 1; failure <= 2; ++failure) {
        order = 0;
        assert(!ndsAudioStorageCall(NDS_AUDIO_STORAGE_READ_CARD, 100, output, sizeof(output)));
        assert(order == 4 && gNdsAudioStorageFailures == failure);
    }
    failure = 0; order = 0; empty = 1;
    assert(ndsAudioStorageCall(NDS_AUDIO_STORAGE_OPEN_CARD, 0, NULL, 0));
    assert(order == 2 && gNdsAudioStorageRequests == 4);
    return 0;
}
''')


def test_media_owner_chunk_bounds_and_close_generation():
    source = (ROOT / "src/nds/arm7/nds_audio_main.c").read_text()
    run_c(r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
static uint32_t sMediaMutex, sMediaGeneration=1, sRomBytes=40000, calls, close_after, fail_at;
static bool sCardOpen;
static struct { uint32_t extent_count; } sFileMap={1};
static void mutexLock(uint32_t *m) { assert((*m)++==0); }
static void mutexUnlock(uint32_t *m) {
    assert(--(*m)==0);
    if(close_after&&calls==close_after) {sMediaGeneration++;sRomBytes=0;sFileMap.extent_count=0;close_after=0;}
}
static bool ndsAudioStorageReadMap(uint32_t offset,uint8_t *out,uint32_t bytes) {
    assert(sMediaMutex&&bytes<=8192&&offset<=40000&&bytes<=40000-offset);
    if(++calls==fail_at)return false;
    memset(out,(int)(offset/8192),bytes);return true;
}
static bool ntrcardRomRead(int dma,uint32_t offset,void *out,uint32_t bytes) {
    assert(dma==-1);return ndsAudioStorageReadMap(offset,out,bytes);
}
''' + function(source, "ndsAudioStorageReadRom") + r'''
int main(void) {
    uint8_t output[20000];
    assert(ndsAudioStorageReadRom(0,output,sizeof(output))&&calls==3&&!sMediaMutex);
    assert(output[0]==0&&output[8192]==1&&output[16384]==2);
    calls=0;close_after=1;
    assert(!ndsAudioStorageReadRom(0,output,sizeof(output))&&calls==1&&!sMediaMutex);
    sRomBytes=40000;sCardOpen=true;sFileMap.extent_count=0;calls=0;fail_at=2;
    assert(!ndsAudioStorageReadRom(0,output,sizeof(output))&&calls==2&&!sMediaMutex);
    assert(!ndsAudioStorageReadRom(UINT32_MAX,output,10));
    assert(!ndsAudioStorageReadRom(0,NULL,10));
    return 0;
}
''')
