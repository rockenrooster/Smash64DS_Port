"""Execute the actual BEX publication boundary with host cache/file fixtures."""
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
import unittest

import generate_metaknight_core_contract as contract


ROOT = Path(__file__).resolve().parents[2]


def function_source(text, name):
    pattern = r'(?m)^(?:static\s+)?(?:__attribute__\(\([^\n]*\)\)\s+)?(?:u32|s32|void)\s+' + name + r'\s*\('
    match = next((candidate for candidate in re.finditer(pattern, text)
                  if ';' not in text[candidate.end():text.index('{', candidate.end())]), None)
    if match is None:
        raise AssertionError('missing runtime function ' + name)
    first = text.index('{', match.end())
    depth = 1
    cursor = first + 1
    while depth:
        if text[cursor] == '{': depth += 1
        if text[cursor] == '}': depth -= 1
        cursor += 1
    return text[match.start():cursor]


class MetaCoreRuntimeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.native = ROOT / 'builds/p4/meta-knight-native'
        cls.preview = ROOT / 'assets/fighters/preview_core'
        cls.battle = ROOT / 'assets/fighters/battle_core'
        if not (cls.preview / '29.fpc').exists():
            raise unittest.SkipTest('published qualified Meta core artifacts are absent')
        cls.receipt = contract.qualify(cls.native, cls.preview, cls.battle)

    def test_contract_covers_every_source_extern_and_both_artifacts(self):
        self.assertEqual(len(self.receipt['external_rows']), 11)
        self.assertEqual(self.receipt['fpc_header'][3], 29)
        self.assertEqual(self.receipt['fpc_header'][12:15], [5455, 5456, 75296])
        header = contract.render_header(self.receipt)
        self.assertTrue(all(not line.endswith(chr(92) * 2) for line in header.splitlines()))

    def test_corrupted_row_is_rejected_before_contract_emission(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory)
            shutil.copyfile(self.preview / '29.fpc', target / '29.fpc')
            data = bytearray((self.preview / '29.ext').read_bytes())
            data[26] ^= 1  # Change dependency identity without changing the BEX bank hash.
            (target / '29.ext').write_bytes(data)
            with self.assertRaisesRegex(ValueError, 'source Main dependency rows'):
                contract.qualify(self.native, target, self.battle)

    def test_actual_c_cache_publication_and_corruption_boundaries(self):
        compiler = shutil.which('gcc')
        if compiler is None:
            self.skipTest('host C compiler is unavailable')
        text = (ROOT / 'src/port/reloc_preview_pack.c').read_text()
        functions = '\n\n'.join(function_source(text, name) for name in
                                ('ndsPreviewHash', 'ndsBattleForeignImagesValid',
                                 'ndsRelocPrewarmMetaCoreStatusFiles',
                                 'ndsRelocPatchCompactMainExternsLoaded'))
        harness = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef uint64_t u64; typedef int32_t s32;
#define TRUE 1
#define FALSE 0
#define NDS_P4_METAKNIGHT 1
#define NDS_BATTLE_EXTERN_MAGIC 0x31584542u
#define NDS_BATTLE_EXTERN_VERSION 2u
#define NDS_BATTLE_EXTERN_MAX 24u
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#include "contract.h"
typedef struct {u16 asset_id,reserved;u32 source_offset,data_offset,data_bytes;} NDSBattleForeignImageRow;
typedef struct {u32 magic;u16 version,count;u32 foreign_count,foreign_bytes,foreign_hash,reserved;} NDSBattleExternHeader;
typedef struct {u16 slot,dep_asset,target_offset;} NDSBattleExternRow;
typedef struct {u32 asset_id,owner_generation;u8 reserved[3];void *data;u32 size;} NDSRelocLoadedFile;
typedef struct {u32 file_mainmotion_id,file_submotion_id;void **p_file_mainmotion,**p_file_submotion;} FTData;
typedef struct {u32 generation;NDSBattleForeignImageRow *foreign_images;u32 foreign_count,externs_ready;} NDSPreviewResident;
static NDSPreviewResident sNdsPreviewResidents[13];
static NDSBattleExternRow expected[] = {
#define ROW(s,d,t) {s,d,t},
NDS_META_CORE_EXTERN_ROWS(ROW)
#undef ROW
};
#define sNdsMetaCoreExpectedExterns expected
static u32 sNdsRelocSceneGeneration=7,sNdsRelocStatusBufferCount;
static void *sNdsRelocStatusBuffer;
static u32 gNdsBattleCoreExternLoadCount,gNdsBattleCoreExternPatchCount;
static u32 gNdsBattleCoreForeignImageBytes,gNdsBattleCoreForeignImageRows,gNdsBattleCoreForeignImageLoadCount;
static u8 main_data[2416],dep_data[4][18000];
static void *main_motion,*sub_motion;static FTData fighter={NDS_META_CORE_MOTION_ASSET,NDS_META_CORE_MOTION_ASSET,&main_motion,&sub_motion};
static NDSRelocLoadedFile deps[4];
static int loaded[4],status[4],writes,ensures,opens;
static char actual_path[128];static const char *fixture_path;static jmp_buf failure;
static s32 ndsP4IsMetaKnight(s32 kind){return kind==29;}
static u32 ndsRosterSelectionIndex(u32 kind){return kind<12?kind:kind==29?12:0xffffffffu;}
static s32 ndsPreviewRange(u32 at,u32 size,u32 total){return at<=total&&size<=total-at;}
static void ndsBattleCoreExternHalt(s32 kind){(void)kind;longjmp(failure,1);}
static int index_of(u32 id){for(int i=0;i<4;i++)if(deps[i].asset_id==id)return i;return -1;}
static NDSRelocLoadedFile *ndsRelocFindLoadedFileByAsset(u32 id){int i=index_of(id);return i>=0&&loaded[i]?&deps[i]:NULL;}
static NDSRelocLoadedFile *ndsRelocEnsureLoadedAsset(u32 id){int i=index_of(id);ensures++;if(i<0)return NULL;loaded[i]=1;return &deps[i];}
static void ndsRelocAddStatusBufferFile(u32 id,void *data){int i=index_of(id);if(i<0||data!=deps[i].data)abort();status[i]=1;}
static void *ndsRelocFindStatusNode(void *unused,u32 count,u32 id){(void)unused;(void)count;int i=index_of(id);return i>=0&&status[i]?deps[i].data:NULL;}
static FTData *ndsP4GetFighterData(s32 kind){return kind==29?&fighter:NULL;}
static void *lbRelocGetStatusBufferFile(u32 id){return ndsRelocFindStatusNode(NULL,0,id);}
static void ndsP4BindFighterMotionData(s32 kind){if(kind!=29||main_motion!=deps[3].data||sub_motion!=deps[3].data||deps[3].owner_generation!=sNdsRelocSceneGeneration)abort();}
static s32 ndsPreviewFileOffset(const NDSRelocLoadedFile *f,u32 at,u32 size,u32 *out){if(!ndsPreviewRange(at,size,f->size))return FALSE;*out=at;return TRUE;}
static void ndsRelocWriteNativePointer(void *slot,void *target){
 if(writes>=11||slot!=main_data+expected[writes].slot)abort();
 int i=index_of(expected[writes].dep_asset);
 if(i<0||!status[i]||target!=(u8*)deps[i].data+expected[writes].target_offset)abort();writes++;
}
static u32 ndsRelocReadBe32(const void *p){const u8 *b=p;return (u32)b[0]<<24|(u32)b[1]<<16|(u32)b[2]<<8|b[3];}
static void ndsRelocWriteNative32(void *p,u32 x){memcpy(p,&x,4);}
static void *syTaskmanMalloc(u32 size,u32 alignment){(void)alignment;return malloc(size);}
static void ndsFsLock(void){}static void ndsFsUnlock(void){}
static FILE *fixture_open(const char *path,const char *mode){opens++;strcpy(actual_path,path);return fopen(fixture_path,mode);}
#define fopen fixture_open
'''
        harness += functions
        harness += r'''
int main(int argc,char **argv){
 if(argc!=4)return 9;fixture_path=argv[1];int mode=atoi(argv[2]),battle=atoi(argv[3]);
 const u32 ids[]={232,331,351,NDS_META_CORE_MOTION_ASSET};
 for(int i=0;i<4;i++){deps[i].asset_id=ids[i];deps[i].data=dep_data[i];deps[i].size=18000;deps[i].owner_generation=7;}
 NDSRelocLoadedFile main_file={5455,7,{30,0,0},main_data,2416};
 sNdsPreviewResidents[12].generation=mode==2?6:7;
 if(mode==2)sNdsPreviewResidents[12].externs_ready=TRUE;
 if(setjmp(failure))return mode!=0&&writes==0&&ensures==0?0:10;
 if(mode==0&&!ndsRelocPrewarmMetaCoreStatusFiles(29))return 16;
 int warmed=ensures;
 if(!ndsRelocPatchCompactMainExternsLoaded(29,&main_file,battle))return 11;
 if(mode!=0)return 12;
 if(writes!=11||!status[0]||!status[1]||!status[2]||!status[3]||ensures!=warmed)return 13;
 const char *wanted=battle?"nitro:/fighters/battle/29.ext":"nitro:/fighters/preview/29.ext";
 if(strcmp(actual_path,wanted))return 14;
 int calls=ensures,reads=opens;
 if(!ndsRelocPatchCompactMainExternsLoaded(29,&main_file,battle)||ensures!=calls||opens!=reads||writes!=11)return 15;
 /* Retire/cancel the per-kind publication then reacquire in the same scene.
  * Status/cache bytes keep the original generation and are never reallocated. */
 sNdsPreviewResidents[12].externs_ready=FALSE;writes=0;
 if(!ndsRelocPatchCompactMainExternsLoaded(29,&main_file,battle)||ensures!=calls||writes!=11)return 17;
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            (temp / 'contract.h').write_text(contract.render_header(self.receipt))
            (temp / 'boundary.c').write_text(harness)
            executable = temp / 'boundary.exe'
            result = subprocess.run([compiler, '-std=c11', '-O0', str(temp / 'boundary.c'),
                                     '-o', str(executable)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            good = temp / 'good.ext'
            good.write_bytes((self.preview / '29.ext').read_bytes())
            bad = temp / 'bad.ext'
            corrupted = bytearray(good.read_bytes())
            corrupted[26] ^= 1
            bad.write_bytes(corrupted)
            for path, mode, is_battle in ((good, 0, 0), (good, 0, 1), (bad, 1, 0), (good, 2, 0)):
                checked = subprocess.run([str(executable), str(path), str(mode), str(is_battle)],
                                         capture_output=True, text=True)
                self.assertEqual(checked.returncode, 0, f'boundary mode={mode} battle={is_battle}: {checked.stderr}')


if __name__ == '__main__':
    unittest.main()
