"""Source-derived electric bodies and the actual native program selector."""
from pathlib import Path
import shutil
import subprocess
import sys

import native_skeletons as skeletons

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/menus"))
from test_menu_repair import function


def test_source_skeletons():
    assert skeletons.root_offsets(ROOT, "fox") == (
        0x6240, 0x6320, 0x6a20, 0x6910, 0x6b10, 0x66b0, 0x6a20, 0x6910,
        0x6c10, 0x6d30, 0x6de0, 0x6e90, 0x6d30, 0x6de0, 0x6e90, 0x7020)
    contexts = skeletons.contexts(ROOT)
    for name, sid in skeletons.PROGRAMS:
        ctx = contexts[(f"{name}_skeleton{sid}", "high")]
        assert tuple(root[0] for root in ctx["roots"]) == skeletons.root_offsets(ROOT, name, sid)
        assert ctx["triangles"] and ctx["dense_vertices"]
        assert all(run[2] == 0 for run in ctx["runs"])  # independent live matrices
        assert contexts[(f"{name}_skeleton{sid}", "low")] is ctx


def test_program_selection(tmp_path):
    text = (ROOT / "src/nds/nds_renderer_assets.c").read_text()
    code = r'''
#include <assert.h>
#include <stddef.h>
typedef unsigned u32; typedef unsigned short u16; typedef unsigned char u8;
#define NDS_NATIVE_FIGHTER_OWNER_COUNT 2
#define NDS_RENDERER_NATIVE_FIGHTER_OWNER_MARIO 0
#define NDS_RENDERER_NATIVE_FIGHTER_OWNER_FOX 1
typedef struct { int unused; } NDSNativeFighterRuntimeTables;
typedef struct { u32 root_offset; u16 first_epoch,tail_state_first,source_command_count;
 u8 epoch_count,tail_state_count,tail_sync_count,light_preamble; } NDSNativeRoot;
typedef struct { const NDSNativeFighterRuntimeTables *tables; const NDSNativeRoot *roots;
 u32 root_count; const u8 *cross_palette_slots; const u32 (*root_light_preambles)[2];
 u32 root_light_preamble_count,asset_data_size; } NDSNativeFighterOwnerRuntime;
static u8 sNdsNativeFighterRootPrograms[2],sNdsSkeletonCrossSlots[32];
static NDSNativeRoot canonical_roots[2] = {{10},{20}};
static NDSNativeFighterOwnerRuntime canonical = {NULL,canonical_roots,2};
static const NDSNativeFighterOwnerRuntime *ndsRendererNativeFighterCanonicalOwnerForDetail(u32 s,u32 d)
 { (void)s;(void)d;return &canonical; }
'''
    code += '\n#include "' + (ROOT / "src/nds/generated/nds_native_skeletons.generated.inc").as_posix() + '"\n'
    start = text.index("static const NDSNativeFighterOwnerRuntime *\nndsRendererNativeFighterOwnerForProgramDetail(")
    code += text[start:text.index("\n}", start) + 2]
    for name in ("ndsRendererNativeFighterSetRootProgram", "ndsRendererNativeFighterSelectRootProgram"):
        code += function(text, name)
    code += r'''
int main(void) {
 for(u32 slot=0;slot<2;slot++) for(u32 detail=0;detail<2;detail++) {
    const NDSNativeFighterOwnerRuntime *sk=ndsNativeSkeletonOwner(slot,1);
    u32 roots[32], tried;
    for(u32 i=0;i<sk->root_count;i++) roots[i]=sk->roots[i].root_offset;
    assert(ndsRendererNativeFighterSelectRootProgram(slot,detail,roots,sk->root_count,&tried)==0xfe);
    ndsRendererNativeFighterSetRootProgram(slot,0xfe);
    assert(sNdsNativeFighterRootPrograms[slot]==0xfe);
    assert(ndsRendererNativeFighterOwnerForProgramDetail(slot,detail,0xfe)==sk);
    assert(ndsRendererNativeFighterSelectRootProgram(slot,detail,roots,sk->root_count-1,&tried)==0xff);
    for(u32 i=0;i<sk->root_count;i++) {
        roots[i]^=8;
        assert(ndsRendererNativeFighterSelectRootProgram(slot,detail,roots,sk->root_count,&tried)==0xff);
        roots[i]^=8;
    }
    ndsRendererNativeFighterSetRootProgram(slot,123);
    assert(sNdsNativeFighterRootPrograms[slot]==0);
    roots[0]=10;roots[1]=20;
    assert(ndsRendererNativeFighterSelectRootProgram(slot,detail,roots,2,&tried)==0);
 }
 assert(ndsRendererNativeFighterSelectRootProgram(2,0,NULL,0,NULL)==0xff);
 return 0;
}
'''
    src, exe = tmp_path / "skeleton.c", tmp_path / "skeleton.exe"
    src.write_text(code)
    subprocess.run([shutil.which("gcc"), "-std=c11", "-O2", str(src), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)


def test_creation_residency(tmp_path):
    code = r'''
#include <assert.h>
#include <stddef.h>
typedef unsigned u32;
#define FALSE 0
#define NDS_P2_PIKACHU 1
#define NDS_NATIVE_IMAGE_OWNER_SLOTS 25
#define NDS_NATIVE_IMAGE_SLOT_MARIO_SKELETON1 23
#define NDS_NATIVE_IMAGE_SLOT_FOX_SKELETON1 24
#define NDS_NATIVE_IMAGE_SLOT_PIKACHU 5
enum { nFTKindMario=0,nFTKindFox=1,nFTKindPikachu=9 };
enum { nFTPlayerKindDemo=3,nFTPartsDetailHigh=1,nFTPartsDetailLow=2 };
enum { nSCKindVSBattle=22,nSCKindVSResults=24 };
typedef struct { int fkind,pkind,detail; } FTDesc;
static struct { int scene_curr; } gSCManagerSceneData;
static u32 slots[4],details[4],count;
int ndsRendererNativeEnsureOwnerImage(u32 s,u32 d) { slots[count]=s;details[count++]=d;return 1; }
'''
    code += function((ROOT / "src/import/battleship_ftmanager.c").read_text(), "ndsFTManagerEnsureOwnerImages")
    code += r'''
int main(void) {
 FTDesc d={nFTKindPikachu,nFTPlayerKindDemo,nFTPartsDetailHigh};
 gSCManagerSceneData.scene_curr=nSCKindVSResults;
 ndsFTManagerEnsureOwnerImages(&d);
 assert(count==1 && slots[0]==5 && details[0]==0);
 count=0;gSCManagerSceneData.scene_curr=99;
 ndsFTManagerEnsureOwnerImages(&d);assert(count==2 && details[1]==1);
 count=0;gSCManagerSceneData.scene_curr=nSCKindVSBattle;d.pkind=1;
 ndsFTManagerEnsureOwnerImages(&d);assert(count==1 && details[0]==0);
 count=0;d.detail=nFTPartsDetailLow;
 ndsFTManagerEnsureOwnerImages(&d);assert(count==2 && details[1]==1);
 count=0;d.fkind=nFTKindFox;
 ndsFTManagerEnsureOwnerImages(&d);assert(count==1 && slots[0]==24 && details[0]==0);
 count=0;d.pkind=nFTPlayerKindDemo;
 ndsFTManagerEnsureOwnerImages(&d);assert(count==0);
 ndsFTManagerEnsureOwnerImages(NULL);assert(count==0);
 return 0;
}
'''
    src, exe = tmp_path / "residency.c", tmp_path / "residency.exe"
    src.write_text(code)
    subprocess.run([shutil.which("gcc"), "-std=c11", str(src), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
