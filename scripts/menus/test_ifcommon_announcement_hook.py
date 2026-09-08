"""Exercise the source SObj-creation seam that prepares native end messages."""
import re
import shutil
import subprocess
from pathlib import Path

from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]


def test_source_internal_constructors_reach_native_preparation(tmp_path):
    adapter = (ROOT / 'src/import/battleship_ifcommon.c').read_text()
    source = (ROOT / 'decomp/BattleShip-main/decomp/src/if/ifcommon.c').read_text()
    hook = function(adapter, 'ndsIFCommonMakeSObjForGObj')
    define = adapter.index('#define lbCommonMakeSObjForGObj ndsIFCommonMakeSObjForGObj')
    include = adapter.index('#include "../../decomp/BattleShip-main/decomp/src/if/ifcommon.c"')
    undef = adapter.index('#undef lbCommonMakeSObjForGObj')
    assert define < include < undef
    for name, first in [('TimeUp', 'T'), ('GameSet', 'G')]:
        spelling = re.search(r'dIFCommonAnnounce' + name + r'SpriteData\[.*?\]\s*=\s*\{(.*?)\};', source, re.S)[1]
        letters = re.findall(r'llIFCommonGameStatusBlueLetter([A-Z])Sprite', spelling)
        assert letters == (list('TIMEUP') if name == 'TimeUp' else list('GAMESET'))
        assert letters[0] == first
        constructor = function(source, 'ifCommonAnnounce' + name + 'MakeInterface')
        assert 'gcMakeGObjSPAfter' in constructor
        assert 'ifCommonAnnounceSetAttr' in constructor
    attr = function(source, 'ifCommonAnnounceSetAttr')
    assert 'lbCommonMakeSObjForGObj' in attr
    code = r'''
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
typedef uint32_t u32;
typedef struct { unsigned bmfmt,bmsiz; void *bitmap; } Sprite;
typedef struct { int value; } SObj;
typedef struct { void *obj; unsigned id,dl_link_id; } GObj;
#define FALSE 0
#define nGCCommonKindInterface 1016
#define NDS_NATIVE_FAILURE_SPRITE 3
#define NDS_NATIVE_FAILURE_BAD_ASSET 4
static unsigned char source_bytes[256];
static void *gGMCommonFiles[2]={0,source_bytes};
static struct { unsigned scene_curr; } gSCManagerSceneData={22};
static char llIFCommonGameStatusBlueLetterTSprite;
static char llIFCommonGameStatusBlueLetterGSprite;
#define lbRelocGetFileData(type,base,symbol) ((type)((char*)(base)+((symbol)==&llIFCommonGameStatusBlueLetterTSprite ? 32 : 96)))
static int calls,kind,prepare_ok=1,failures,sequence;
static SObj made;
static int ndsIFCommonNativeOamPrepareAnnouncement(u32 game_set)
{ ++calls; kind=game_set; assert(sequence==0); sequence=1; return prepare_ok; }
static SObj *lbCommonMakeSObjForGObj(GObj *g,Sprite *s)
{ (void)s; sequence=2; if(g)g->obj=&made; return &made; }
static void ndsRendererRecordNativeFailure(u32 d,u32 sc,u32 id,u32 st,u32 root,u32 mat,u32 why)
{ assert(sequence==1); assert(d==3&&sc==22&&id==0x3f80017&&st==3&&why==4); (void)root;(void)mat;++failures; }
''' + hook + r'''
int main(void)
{
    GObj g={0,1016,23};
    Sprite *t=(Sprite*)(source_bytes+32),*e=(Sprite*)(source_bytes+64),*a=(Sprite*)(source_bytes+96);
    t->bmsiz=a->bmsiz=3;
    assert(ndsIFCommonMakeSObjForGObj(&g,t)==&made); assert(calls==1&&kind==0&&sequence==2);
    sequence=0; ndsIFCommonMakeSObjForGObj(&g,e); assert(calls==1);
    g.obj=0;sequence=0; ndsIFCommonMakeSObjForGObj(&g,a); assert(calls==2&&kind==1);
    sequence=0; ndsIFCommonMakeSObjForGObj(&g,t); assert(calls==2); /* GAME SET final T */
    g.obj=0;sequence=0;g.dl_link_id=20;ndsIFCommonMakeSObjForGObj(&g,t);assert(calls==2);
    g.dl_link_id=23;g.obj=0;sequence=0;prepare_ok=0;
    assert(ndsIFCommonMakeSObjForGObj(&g,t)==&made);assert(failures==1&&calls==3);
    g.obj=0;sequence=0;prepare_ok=1;ndsIFCommonMakeSObjForGObj(&g,t);assert(calls==4);
    return 0;
}
'''
    c = tmp_path / 'hook.c'
    c.write_text(code)
    exe = tmp_path / 'hook.exe'
    cc = shutil.which('gcc') or 'C:/devkitPro/msys2/mingw64/bin/gcc.exe'
    result = subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror', str(c), '-o', str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    subprocess.run([str(exe)], check=True, capture_output=True)
