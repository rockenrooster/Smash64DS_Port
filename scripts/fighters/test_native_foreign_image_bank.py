"""Host-execute private IMAGE span validation and source-qualified lookup."""
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/menus"))
from source_test_helpers import function


def test_private_image_bank_collision_missing_and_scene_lifetime():
    source = (ROOT / "src/port/reloc_preview_pack.c").read_text()
    row_type = re.search(r"typedef struct NDSBattleForeignImageRow.*?} NDSBattleForeignImageRow;", source, re.S)[0]
    body = r'''
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;
#define TRUE 1
#define FALSE 0
#define NDS_P2_COMPACT_BATTLE_FIGHTERS 1
#define NDS_PREVIEW_PACK_MAX_SECTIONS 4
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
typedef struct { u32 asset_id; void *data; u32 data_size, owner_scene, owner_generation; u8 reserved[2]; } NDSRelocLoadedFile;
typedef struct { u32 first_span, span_count; } NDSPreviewPackSection;
typedef struct { u32 source_offset, data_offset, data_bytes; } NDSPreviewPackSpan;
''' + row_type + r'''
typedef struct {
    u32 generation;
    NDSPreviewPackSection *sections;
    NDSPreviewPackSpan *spans;
    NDSBattleForeignImageRow *foreign_images;
    u32 foreign_count;
} NDSPreviewResident;
static NDSPreviewResident sNdsPreviewResidents[12];
static u32 sNdsRelocSceneGeneration = 7;
static struct { u32 scene_curr; } gSCManagerSceneData = { 16 };
static NDSRelocLoadedFile files[3];
static NDSRelocLoadedFile *ndsRelocFindLoadedFileByData(void *base) {
    for (unsigned i=0;i<3;i++) if(files[i].data==base) return &files[i];
    return NULL;
}
static NDSRelocLoadedFile *ndsRelocFindLoadedFileByAsset(u32 asset) {
    for (unsigned i=0;i<3;i++) if(files[i].asset_id==asset) return &files[i];
    return NULL;
}
'''
    for name in ("ndsPreviewRange", "ndsPreviewSection", "ndsPreviewFileOffset",
                 "ndsBattleForeignImagesValid", "ndsRelocNativeForeignImageAddress"):
        body += function(source, name) + "\n"
    body += r'''
int main(void) {
    u8 compact_owner[8]={0}, raw_owner[8]={0}, raw_yoshi[128]={0};
    struct { NDSBattleForeignImageRow rows[2]; u8 payload[40]; } bank = {
        {{338,0,0x9ec8,0,32},{338,0,0x9ef0,32,8}}, {0}
    };
    NDSPreviewPackSection sections[2]={{0,1},{0,1}};
    NDSPreviewPackSpan spans[1]={{0x9ec8,4,32}};
    files[0]=(NDSRelocLoadedFile){328,compact_owner,8,16,7,{9,0}};
    files[1]=(NDSRelocLoadedFile){324,raw_owner,8,16,7,{0,0}};
    files[2]=(NDSRelocLoadedFile){338,raw_yoshi,128,16,7,{0,0}};
    sNdsPreviewResidents[8]=(NDSPreviewResident){7,sections,spans,bank.rows,2};
    assert(ndsBattleForeignImagesValid(bank.rows,2,40));
    assert(ndsRelocNativeForeignImageAddress(compact_owner,338,0x9ec8)==bank.payload);
    assert(ndsRelocNativeForeignImageAddress(compact_owner,338,0x9ef4)==bank.payload+36);
    /* A real Yoshi loaded at the same asset identity cannot shadow this bank. */
    assert(ndsRelocNativeForeignImageAddress(compact_owner,338,4)==NULL);
    assert(ndsRelocNativeForeignImageAddress(compact_owner,339,0x9ec8)==NULL);
    assert(ndsRelocNativeForeignImageAddress(compact_owner,338,0x9ee8)==NULL);
    assert(ndsRelocNativeForeignImageAddress(raw_owner,338,4)==raw_yoshi+4);
    assert(ndsRelocNativeForeignImageAddress(raw_owner,338,128)==NULL);
    files[2].reserved[0]=9; files[2].reserved[1]=1;
    assert(ndsRelocNativeForeignImageAddress(raw_owner,338,0x9ec9)==raw_yoshi+5);
    files[2].owner_generation=6;
    assert(ndsRelocNativeForeignImageAddress(raw_owner,338,0x9ec9)==NULL);
    sNdsPreviewResidents[8].generation=6;
    assert(ndsRelocNativeForeignImageAddress(compact_owner,338,0x9ec8)==NULL);
    sNdsPreviewResidents[8].generation=7;
    files[0].owner_scene=15;
    assert(ndsRelocNativeForeignImageAddress(compact_owner,338,0x9ec8)==NULL);
    assert(ndsBattleForeignImagesValid(NULL,0,0));
    assert(!ndsBattleForeignImagesValid(bank.rows,0,40));
    bank.rows[1].reserved=1; assert(!ndsBattleForeignImagesValid(bank.rows,2,40));
    bank.rows[1].reserved=0; bank.rows[1].data_offset=36;
    assert(!ndsBattleForeignImagesValid(bank.rows,2,40));
    bank.rows[1].data_offset=32; bank.rows[1].source_offset=0x9ed0;
    assert(!ndsBattleForeignImagesValid(bank.rows,2,40));
    bank.rows[1].source_offset=0xfffffffcu;
    assert(!ndsBattleForeignImagesValid(bank.rows,2,40));
    bank.rows[1]=bank.rows[0]; assert(ndsBattleForeignImagesValid(bank.rows,2,40));
    return 0;
}
'''
    compiler = shutil.which("clang") or shutil.which("gcc")
    assert compiler, "host C compiler required"
    with tempfile.TemporaryDirectory(prefix="nds-foreign-image-") as work:
        cfile = Path(work) / "bank.c"
        exe = Path(work) / "bank.exe"
        cfile.write_text(body)
        subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                        str(cfile), "-o", str(exe)], check=True, timeout=30)
        subprocess.run([str(exe)], check=True, timeout=10)
