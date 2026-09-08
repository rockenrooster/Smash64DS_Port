"""Check source acid GObj capture and updates to existing stage registrations."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/menus'))
sys.path.insert(0, str(ROOT / 'scripts/stages'))
from source_test_helpers import function
import emit_native_stage_runtime_rows as emitter
import register_native_stage_runtime as register
from native_stage_descriptors import get_descriptor


def test_existing_capture_and_census_refresh_is_idempotent(tmp_path, monkeypatch):
    monkeypatch.setattr(register,'ROOT',str(tmp_path))
    desc = get_descriptor('zebes')
    rows = emitter.capture_rows(desc)
    acid = next(row for row in rows if row['name'] == 'acid')
    assert (acid['source'],acid['index'],acid['link'],acid['dl_links']) == ('ZEBES_ACID',0,12,1)
    for attr,path,call in [('MATRIX','src/port/renderer_adapter_matrix.c',register.register_matrix),
                           ('SELECT','src/nds/nds_native_stage_select.inc',register.register_select)]:
        previous = subprocess.check_output(['git','show','28047525acd:'+path])
        dest = tmp_path / Path(path).name
        dest.write_bytes(previous+b'\n/* unrelated edit must survive */\n')
        monkeypatch.setattr(register,attr,str(dest))
        call('zebes',desc,False)
        first = dest.read_bytes()
        assert first != previous
        assert b'unrelated edit must survive' in first
        if attr == 'MATRIX':
            assert b'NDS_RENDERER_ADAPTER_STAGE_CAPTURE_ZEBES_ACID' in first
            assert b'sNdsRendererAdapterNativeStageCaptureZebes[2]' in first
        else:
            assert b'0xff7700ULL' in first  # preserve qualified policy masks
            census = ', '.join(f'{v}u' for v in desc.expected_counts['submit_classes'])
            assert census.encode() in first
        call('zebes',desc,False)
        assert dest.read_bytes() == first


def test_capture_uses_live_acid_gobj_and_source_callback(tmp_path):
    adapter = (ROOT/'src/port/renderer_adapter_stage.c').read_text()
    get = function(adapter,'ndsRendererAdapterNativeStageSegmentGObj')
    matches = function(adapter,'ndsRendererAdapterNativeStageProcMatches')
    callbacks = re.findall(r'grDisplayLayer\d(?:Pri|Sec)ProcDisplay', matches)
    code = '''#include <assert.h>\n#include <stddef.h>\n
typedef unsigned u32; typedef int sb32;
#define TRUE 1
#define FALSE 0
#define NDS_P2_STAGE_ZEBES 1
#define NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER 0
#define NDS_RENDERER_ADAPTER_STAGE_CAPTURE_PUPUPU_MAP 1
#define NDS_RENDERER_ADAPTER_STAGE_CAPTURE_ZEBES_ACID 2
typedef struct GObj {void (*proc_display)(struct GObj *);} GObj;
typedef struct {u32 source,index,layer,dl_links;} NDSRendererAdapterNativeStageCaptureSegment;
static NDSRendererAdapterNativeStageCaptureSegment row={2,0,0,1};
static GObj *gGRCommonLayerGObjs[4],*acid;
static struct {struct {GObj *map_gobj[4];} pupupu;} gGRCommonStruct;
static const NDSRendererAdapterNativeStageCaptureSegment *ndsRendererAdapterNativeStageCaptureRow(u32 i)
{return i==1 ? &row : NULL;}
static void *ndsGRZebesAcidGObj(void) {return acid;}
static void gcDrawDObjTreeDLLinksForGObj(GObj *g) {(void)g;}
''' + '\n'.join('static void '+cb+'(GObj *g) {(void)g;}' for cb in dict.fromkeys(callbacks)) + '\n' + get + '\n' + matches + '''
int main(void) {
GObj a={gcDrawDObjTreeDLLinksForGObj},b={grDisplayLayer1SecProcDisplay};
acid=&a; assert(ndsRendererAdapterNativeStageSegmentGObj(1)==&a);
assert(ndsRendererAdapterNativeStageProcMatches(1,&a));
assert(!ndsRendererAdapterNativeStageProcMatches(1,&b));
acid=&b; assert(ndsRendererAdapterNativeStageSegmentGObj(1)==&b);
acid=NULL; assert(!ndsRendererAdapterNativeStageSegmentGObj(1));
assert(!ndsRendererAdapterNativeStageProcMatches(1,NULL)); return 0; }
'''
    c,exe=tmp_path/'capture.c',tmp_path/'capture.exe'
    c.write_text(code)
    cc=shutil.which('gcc') or shutil.which('clang')
    assert cc
    result=subprocess.run([cc,'-std=c99','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    result=subprocess.run([str(exe)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr


def test_translation_only_stage_nodes_keep_their_source_matrix_kind(tmp_path):
    stage = (ROOT/'src/port/renderer_adapter_stage.c').read_text()
    matrix = (ROOT/'src/port/renderer_adapter_matrix.c').read_text()
    admit = function(stage,'ndsRendererAdapterNativeStageTransformFlags')
    source = (ROOT/'decomp/BattleShip-main/decomp/src/gr/grcommon/grzebes.c').read_text()
    assert 'nGCMatrixKindTra,' in function(source,'grZebesMakeAcid')
    # Admission flag zero means ordinary, not a request to replace Tra by TRS.
    kind = matrix[matrix.index('case nGCMatrixKindTra:'):]
    kind = kind[:kind.index('break;')]
    assert 'syMatrixTra(&mtx, dobj->translate.vec.f.x,' in kind
    assert 'rotate' not in kind and 'scale' not in kind
    code = '''#include <stdint.h>\n#include <stddef.h>\n#include <assert.h>\n
typedef uint16_t u16; typedef int sb32;
#define TRUE 1
#define FALSE 0
enum {nGCMatrixKindTra=3,nGCMatrixKindTraRotRpyRSca=18,nGCMatrixKind48=48,
      nGCMatrixKind46=46,nGCMatrixKindRecalcRotRpyRSca=45};
typedef struct {int kind;} XObj;
typedef struct {unsigned xobjs_num;XObj *xobjs[2];} DObj;
''' + admit + '''
int main(void) {
u16 flags=99; XObj x={nGCMatrixKindTra}; DObj d={1,{&x,NULL}};
assert(ndsRendererAdapterNativeStageTransformFlags(&d,&flags)&&flags==0);
x.kind=nGCMatrixKindTraRotRpyRSca;assert(ndsRendererAdapterNativeStageTransformFlags(&d,&flags)&&flags==0);
x.kind=99;assert(!ndsRendererAdapterNativeStageTransformFlags(&d,&flags));
x.kind=nGCMatrixKindTra;d.xobjs_num=2;assert(!ndsRendererAdapterNativeStageTransformFlags(&d,&flags));
return 0; }
'''
    c,exe=tmp_path/'translation.c',tmp_path/'translation.exe'
    c.write_text(code)
    cc=shutil.which('gcc') or shutil.which('clang')
    result=subprocess.run([cc,'-std=c99','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    result=subprocess.run([str(exe)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
