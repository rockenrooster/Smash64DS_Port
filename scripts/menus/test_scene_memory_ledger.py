"""Execute the real ledger updates across scene generations and DS setups."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]


class SceneMemoryLedgerTests(unittest.TestCase):
    def test_scene_peak_reset_and_actual_setup(self):
        source = (ROOT / 'src/port/reloc_backend_assets.c').read_text()
        body = function(source, 'ndsRelocUpdateMemoryLedger') + '\n' + function(
            source, 'ndsRelocRecordSceneMemory')
        globals_ = sorted(set(re.findall(r'\bgNds\w+', body)))
        declarations = '\n'.join('static u32 ' + name + ';' for name in globals_)
        program = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
typedef uint32_t u32; typedef uint8_t u8;
#define FALSE 0
typedef struct { u32 words[2]; } Gfx;
typedef struct { void *start, *ptr, *end; } Heap;
typedef struct { u32 asset_id, data_size, owner_scene; } NDSRelocLoadedFile;
typedef struct {
    int contexts_num;
    size_t dl_buffer0_size, dl_buffer1_size, dl_buffer2_size, dl_buffer3_size;
    size_t graphics_arena_size;
    int rdp_output_buffer_size;
} SYTaskmanSceneSetup;
static Heap gSYTaskmanGeneralHeap;
static struct { u32 scene_curr; } gSCManagerSceneData;
static u32 sNdsRelocSceneGeneration, sNdsRelocLoadedFileCount;
static NDSRelocLoadedFile sNdsRelocLoadedFiles[1];
static u32 gFTManagerFigatreeHeapSize;
static u8 arena[1048576];
static size_t ndsTaskmanArenaSize(void) { return sizeof(arena); }
static int ndsRelocAssetIsInterface(u32 x) { (void)x; return 0; }
static int ndsRelocAssetIsStage(u32 x) { (void)x; return 0; }
static int ndsRelocAssetIsFighter(u32 x) { (void)x; return 0; }
static int ndsRelocAssetIsMenu(u32 x) { (void)x; return 0; }
static int ndsRelocAssetIsOpening(u32 x) { (void)x; return 0; }
#define NDS_RELOC_MEMORY_LEDGER_RESERVE_BYTES 131072u
#define NDS_MEMORY_LEDGER_PASS 0x4d4c4544u
static unsigned flushes;
static void DC_FlushAll(void) { ++flushes; }
''' + declarations + '\n' + body + r'''
int main(void) {
    SYTaskmanSceneSetup setup = {2,4096,1024,0,0,8192,4096};
    gSYTaskmanGeneralHeap.start = arena;
    gSYTaskmanGeneralHeap.end = arena + sizeof(arena);
    gSCManagerSceneData.scene_curr = 18;
    sNdsRelocSceneGeneration = 1;
    gSYTaskmanGeneralHeap.ptr = arena + 800000;
    ndsRelocUpdateMemoryLedger();
    if (gNdsMemoryLedgerArenaHighWater != 800000) return 1;
    gSCManagerSceneData.scene_curr = 22;
    sNdsRelocSceneGeneration = 2;
    sNdsRelocLoadedFileCount = 1;
    sNdsRelocLoadedFiles[0] = (NDSRelocLoadedFile){7, 12345, 22};
    gSYTaskmanGeneralHeap.ptr = arena + 600000;
    ndsRelocUpdateMemoryLedger();
    if (gNdsMemoryLedgerArenaHighWater != 600000) return 2;
    gSYTaskmanGeneralHeap.ptr = arena + 650000;
    ndsRelocRecordSceneMemory(&setup);
    if (gNdsMemoryLedgerArenaHighWater != 650000 ||
        gNdsMemoryLedgerArenaHeadroom != sizeof(arena)-650000) return 3;
    if (gNdsMemoryLedgerDLBytes != 10240 ||
        gNdsMemoryLedgerGraphicsBytes != 16384 ||
        gNdsMemoryLedgerRdpBytes != 4096) return 4;
    if (gNdsMemoryLedgerRelocFiles != 1 ||
        gNdsMemoryLedgerRelocBytes != 12345 ||
        gNdsMemoryLedgerRelocOtherBytes != 12345 ||
        gNdsMemoryLedgerRelocStaleFiles != 0 || flushes != 1) return 6;
    sNdsRelocSceneGeneration++;
    gSYTaskmanGeneralHeap.ptr = arena + 500000;
    ndsRelocRecordSceneMemory(&setup);
    if (gNdsMemoryLedgerArenaHighWater != 500000 || flushes != 2) return 5;
    return 0;
}
'''
        cc = shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(cc, 'Host compiler required')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            cfile, exe = path / 'ledger.c', path / 'ledger.exe'
            cfile.write_text(program)
            built = subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                                    str(cfile), '-o', str(exe)], capture_output=True)
            self.assertEqual(built.returncode, 0, built.stderr.decode())
            self.assertEqual(subprocess.run([str(exe)]).returncode, 0)


if __name__ == '__main__':
    unittest.main()
