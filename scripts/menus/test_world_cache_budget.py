"""Run the production optional-cache allocators at the scene reserve boundary."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from source_test_helpers import function

ROOT=Path(__file__).resolve().parents[2]


class WorldCacheBudgetTest(unittest.TestCase):
    def test_reserve_alignment_and_scene_retry(self):
        source=(ROOT/'src/port/renderer_adapter_matrix.c').read_text()
        body='\n'.join(function(source,name) for name in (
            'ndsRendererAdapterEnsureDObjWorldCache',
            'ndsRendererAdapterEnsureStageWorldCache'))
        # These allocator helpers only depend on the entries' sizes. The
        # target ELF measures these as 68 and 72 bytes; no matrix math is mocked.
        program=r'''
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
typedef uint32_t u32; typedef int sb32;
#define TRUE 1
#define FALSE 0
#define NDS_RELOC_MEMORY_LEDGER_RESERVE_BYTES 131072u
#define NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT 128u
#define NDS_RENDERER_ADAPTER_STAGE_WORLD_CACHE_COUNT 64u
#define NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE 64u
typedef struct {unsigned char bytes[68];} NDSRendererAdapterDObjWorldCacheEntry;
typedef struct {unsigned char bytes[72];} NDSRendererAdapterStageWorldCacheEntry;
static NDSRendererAdapterDObjWorldCacheEntry *sNdsRendererAdapterDObjWorldCache;
static NDSRendererAdapterStageWorldCacheEntry *sNdsRendererAdapterStageWorldCache;
static u32 sNdsRendererAdapterDObjWorldCacheAllocationAttempted;
static u32 sNdsRendererAdapterStageWorldCacheAllocationAttempted;
static u32 sNdsRendererAdapterDObjWorldCacheCount;
static u32 sNdsRendererAdapterDObjWorldCacheDynamicLimit;
static struct {unsigned char *ptr,*end;} gSYTaskmanGeneralHeap;
static _Alignas(16) unsigned char arena[262144];
static unsigned allocations;
static void ndsRelocUpdateMemoryLedger(void) {}
static void *syTaskmanMalloc(size_t size,u32 align) {
    uintptr_t p=((uintptr_t)gSYTaskmanGeneralHeap.ptr+align-1)&~(uintptr_t)(align-1);
    if(p>(uintptr_t)gSYTaskmanGeneralHeap.end || size>(uintptr_t)gSYTaskmanGeneralHeap.end-p) return NULL;
    gSYTaskmanGeneralHeap.ptr=(unsigned char *)(p+size); ++allocations; return (void *)p;
}
static void reset(size_t free_bytes,size_t misalignment) {
    sNdsRendererAdapterDObjWorldCache=NULL; sNdsRendererAdapterStageWorldCache=NULL;
    sNdsRendererAdapterDObjWorldCacheAllocationAttempted=0;
    sNdsRendererAdapterStageWorldCacheAllocationAttempted=0;
    sNdsRendererAdapterDObjWorldCacheCount=0;
    sNdsRendererAdapterDObjWorldCacheDynamicLimit=128;
    gSYTaskmanGeneralHeap.ptr=arena+misalignment;
    gSYTaskmanGeneralHeap.end=arena+misalignment+free_bytes; allocations=0;
}
'''+body+r'''
#define CHECK(x) do {if(!(x)){printf("line%d: %s\n",__LINE__,#x);return 1;}} while(0)
int main(void) {
    for(size_t bias=0;bias<16;++bias) {
        size_t pad=(16-bias)&15;
        reset(131072+8704+pad-1,bias);
        CHECK(!ndsRendererAdapterEnsureDObjWorldCache() && !allocations);
        gSYTaskmanGeneralHeap.end=arena+sizeof(arena);
        CHECK(!ndsRendererAdapterEnsureDObjWorldCache() && !allocations);
        reset(131072+8704+pad,bias);
        CHECK(ndsRendererAdapterEnsureDObjWorldCache() && allocations==1);
        CHECK(gSYTaskmanGeneralHeap.end-gSYTaskmanGeneralHeap.ptr==131072);
        CHECK(!ndsRendererAdapterEnsureStageWorldCache() && allocations==1);
        reset(131072+8704+4608+pad,bias);
        CHECK(ndsRendererAdapterEnsureStageWorldCache() && allocations==2);
        CHECK(gSYTaskmanGeneralHeap.end-gSYTaskmanGeneralHeap.ptr==131072);
        CHECK(sNdsRendererAdapterDObjWorldCacheDynamicLimit==64);
        CHECK(ndsRendererAdapterEnsureStageWorldCache() && allocations==2);
    }
    reset(33312,0);
    CHECK(!ndsRendererAdapterEnsureStageWorldCache() && !allocations);
    CHECK(gSYTaskmanGeneralHeap.end-gSYTaskmanGeneralHeap.ptr==33312);
    return 0;
}
'''
        cc=shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(cc)
        with tempfile.TemporaryDirectory() as directory:
            d=Path(directory); c=d/'cache.c'; exe=d/'cache.exe'; c.write_text(program)
            r=subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],capture_output=True)
            self.assertEqual(r.returncode,0,r.stderr.decode())
            r=subprocess.run([str(exe)],capture_output=True)
            self.assertEqual(r.returncode,0,r.stdout.decode())


if __name__=='__main__': unittest.main()
