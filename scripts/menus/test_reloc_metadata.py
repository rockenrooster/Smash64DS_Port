"""Execute production relocation registration with a bounded scene allocator."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

from source_test_helpers import braced, function

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'src/port/reloc_backend_assets.c'
TYPES = r'''
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef int32_t s32; typedef s32 sb32;
#define TRUE 1
#define FALSE 0
#define NDS_TASK44_STAGE_STEADY 0
#define NDS_RELOC_EXTERN_FILE_ID_CAPACITY 144u
#define NDS_RELOC_LOADED_FILE_CAPACITY 96u
'''


class RelocMetadataTest(unittest.TestCase):
    def test_registration_and_scene_lifetime(self):
        source = SOURCE.read_text()
        record = braced(source, r'typedef struct NDSRelocLoadedFile\s*\{', True)
        body = '\n'.join(function(source, name) for name in (
            'ndsRelocFindLoadedFileByAsset', 'ndsRelocResetLoadedFiles',
            'ndsRelocRegisterLoadedFileImpl'))
        program = TYPES + record + r'''
#include <stdio.h>
typedef struct { u32 data_size; u16 reloc_intern_offset,
    reloc_extern_offset, extern_file_ids_num; } NDSRelocAssetHeader;
static NDSRelocLoadedFile sNdsRelocLoadedFiles[96];
static u32 sNdsRelocLoadedFileCount, sNdsRelocSceneGeneration = 7;
static const void *sNdsRelocRelativeOffsetsMemoBase;
static NDSRelocLoadedFile *sNdsRelocRelativeOffsetsMemo;
static u32 sNdsRelocNormalizedMObjSubs[2], sNdsRelocNormalizedMObjSubCount;
static u32 gNdsOpeningRoomRelocPointerFixupFailCount, failures;
static struct { u32 scene_curr; } gSCManagerSceneData = {22};
static union { uint64_t alignment; u8 bytes[65536]; } heap;
static size_t used, limit = sizeof(heap.bytes), allocs;
static u32 read_count = 3, reads;
static int read_ok = TRUE;
static u16 input_ids[144];
static void ndsAObjEvent32ResetNormalizedScripts(void) {}
static void ndsFighterMarioFoxResetFileSlots(void) {}
static void ndsRelocRecordExternalFixupFail(u32 asset) {
    (void)asset; ++failures;
}
static void *syTaskmanMalloc(size_t size, u32 align) {
    size_t start = (used + align - 1) & ~(size_t)(align - 1);
    if (start > limit || size > limit - start) return NULL;
    used = start + size; ++allocs;
    return heap.bytes + start;
}
static int ndsRelocAssetReadExternFileIDs(u32 asset, u16 *out,
                                       u32 capacity, u32 *count) {
    (void)asset; ++reads;
    if (!read_ok || read_count > capacity) return FALSE;
    memcpy(out, input_ids, read_count * sizeof(*out));
    *count = read_count; return TRUE;
}
''' + body + r'''
#define CHECK(x) do { if (!(x)) { printf("line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void) {
    NDSRelocAssetHeader h = {100, 4, 8, 0};
    u16 known[144];
    for (u32 i=0; i<144; ++i) known[i] = input_ids[i] = (u16)(i+1);
    NDSRelocLoadedFile *p = ndsRelocRegisterLoadedFileImpl(1, 1,
        heap.bytes, &h, NULL, 0, FALSE);
    CHECK(p && p->extern_count == 0 && p->extern_file_ids == NULL);
    CHECK(used == 0 && allocs == 0 && reads == 0);
    CHECK(p->owner_scene == 22 && p->owner_generation == 7);
    h.extern_file_ids_num = 3;
    p = ndsRelocRegisterLoadedFileImpl(1, 1, heap.bytes, &h, known, 3, TRUE);
    CHECK(p && used == 6 && allocs == 1 && p->extern_file_ids[2] == 3);
    known[2] = 99;
    CHECK(p->extern_file_ids[2] == 3); /* The caller's storage does not escape. */
    u16 *ids = p->extern_file_ids;
    sNdsRelocRelativeOffsetsMemo = p;
    sNdsRelocRelativeOffsetsMemoBase = heap.bytes;
    for (unsigned i=0; i<100; ++i)
        CHECK(ndsRelocRegisterLoadedFileImpl(1, 1, heap.bytes, &h, known, 3, TRUE) == p);
    CHECK(allocs == 1 && used == 6 && p->extern_file_ids == ids);
    CHECK(p->extern_file_ids[2] == 99 && !sNdsRelocRelativeOffsetsMemo &&
          !sNdsRelocRelativeOffsetsMemoBase);
    CHECK(ndsRelocRegisterLoadedFileImpl(1, 1, heap.bytes, &h, NULL, 0, FALSE) == p);
    CHECK(reads == 1 && allocs == 1 && p->extern_file_ids[2] == 3);

    NDSRelocLoadedFile before = *p;
    size_t before_used = used;
    for (unsigned fault=0; fault<6; ++fault) {
        NDSRelocAssetHeader bad = h;
        const NDSRelocAssetHeader *hp = &bad;
        const u16 *kp = known; u32 kc = 3; int is_known = TRUE;
        if (fault == 0) hp = NULL;
        if (fault == 1) bad.extern_file_ids_num = 145;
        if (fault == 2) kp = NULL;
        if (fault == 3) kc = 2;
        if (fault == 4) { is_known=FALSE; read_ok=FALSE; }
        if (fault == 5) { is_known=FALSE; read_ok=TRUE; read_count=2; }
        CHECK(!ndsRelocRegisterLoadedFileImpl(1, 1, heap.bytes, hp, kp, kc, is_known));
        CHECK(memcmp(p, &before, sizeof(before)) == 0 && used == before_used);
        CHECK(ids[0] == 1 && ids[2] == 3 && sNdsRelocLoadedFileCount == 1);
    }
    h.extern_file_ids_num = 4; limit = used;
    CHECK(!ndsRelocRegisterLoadedFileImpl(1, 1, heap.bytes, &h, known, 4, TRUE));
    CHECK(memcmp(p, &before, sizeof(before)) == 0 && used == before_used);
    CHECK(!ndsRelocRegisterLoadedFileImpl(2, 1, heap.bytes, &h, known, 4, TRUE));
    CHECK(sNdsRelocLoadedFileCount == 1 && used == before_used);
    limit = sizeof(heap.bytes);
    CHECK(ndsRelocRegisterLoadedFileImpl(1, 1, heap.bytes, &h, known, 4, TRUE) == p);
    CHECK(p->extern_count == 4 && p->extern_file_ids != ids && used == 16);
    h.extern_file_ids_num = 0;
    CHECK(ndsRelocRegisterLoadedFileImpl(1, 1, heap.bytes, &h, NULL, 0, FALSE) == p);
    CHECK(!p->extern_file_ids && allocs == 2);

    ndsRelocResetLoadedFiles();
    CHECK(!sNdsRelocLoadedFileCount && !sNdsRelocLoadedFiles[0].extern_file_ids);
    used = allocs = 0; ++sNdsRelocSceneGeneration;
    h.extern_file_ids_num = 144;
    for (u32 i=0; i<96; ++i) {
        p = ndsRelocRegisterLoadedFileImpl(i+1, 1, heap.bytes, &h, known, 144, TRUE);
        CHECK(p && p->extern_count == 144 && p->owner_generation == 8);
        CHECK(p->extern_file_ids[143] == 144);
    }
    CHECK(used == 96*144*2 && sNdsRelocLoadedFileCount == 96);
    before_used = used;
    CHECK(!ndsRelocRegisterLoadedFileImpl(97, 1, heap.bytes, &h, known, 144, TRUE));
    CHECK(sNdsRelocLoadedFileCount == 96 && used == before_used);
    puts("PASS: exact allocations, reload reuse, failure atomicity, all 96x144 ids");
    return 0;
}
'''
        cc = shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(cc)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            src, exe = path / 'metadata.c', path / 'metadata.exe'
            src.write_text(program)
            result = subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                                     str(src), '-o', str(exe)], capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            result = subprocess.run([str(exe)], capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout.decode())
            # Check the target's actual pointer width and alignment, not a
            # manually transcribed host model of the DS layout.
            src.write_text(TYPES + record + '\n_Static_assert(sizeof(NDSRelocLoadedFile)==56, "DS metadata layout");\n')
            arm = Path('C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe')
            self.assertTrue(arm.exists())
            result = subprocess.run([str(arm), '-mcpu=arm946e-s', '-mthumb',
                                     '-c', str(src), '-o', str(path/'metadata.o')],
                                    capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr.decode())


if __name__ == '__main__':
    unittest.main()
