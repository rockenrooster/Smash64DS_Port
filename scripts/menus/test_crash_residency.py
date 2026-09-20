"""Execute the CSS lifetime and GO adapter; check Arwing elision against O2R."""
import re
import runpy
import shutil
import struct
import subprocess
import sys
from pathlib import Path

from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]


def test_item_egg_guard_cannot_be_confused_with_yoshi_egg():
    check = runpy.run_path(str(ROOT / "scripts/check-native-owner-wiring.py"))
    resolve = check["no_program_guard_variable"]
    adapter = "sb32 item_egg_native_handled; sb32 yoshi_egg_native_handled;"
    assert resolve("item_egg", adapter) == "item_egg_native_handled"
    assert resolve("yoshi_egg", adapter) == "yoshi_egg_native_handled"
    assert resolve("item_egg", "sb32 yoshi_egg_native_handled;") is None


def run_c(tmp_path, code):
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler, "host C compiler required"
    source, binary = tmp_path / "residency.c", tmp_path / "residency.exe"
    source.write_text(code)
    result = subprocess.run([compiler, "-std=c11", "-Wall", "-Werror",
                             str(source), "-o", str(binary)],
                            capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stderr
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stderr


def test_effect_retry_keeps_callback_when_file_exists_before_its_slot(tmp_path):
    source = (ROOT / "src/import/battleship_efmanager.c").read_text()
    code = r'''
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
typedef unsigned u32;
typedef int sb32;
typedef struct GObj { int unused; } GObj;
typedef struct {
 void **file_head; void (*proc_display)(GObj*);
 intptr_t o_dobjsetup,o_mobjsub,o_anim_joint,o_matanim_joint;
} EFDesc;
#define FALSE 0
#define NDS_EF_DEFERRED_MAX 2
static EFDesc *sNdsEFDeferredDescs[NDS_EF_DEFERRED_MAX];
static void (*sNdsEFDeferredProcs[NDS_EF_DEFERRED_MAX])(GObj*);
static u32 sNdsEFDeferredCount, gNdsEFDescDeferOverflowCount;
static u32 gNdsEFDescDeferRecoverCount, gNdsEFDescUnknownFileCount;
static u32 gNdsEFDescUnknownFileLast, gNdsEFDescDisabledCount, gNdsEFDescDisabledLast;
static size_t span;
static int valid;
static intptr_t ndsEFManagerResolveOffset(intptr_t v) { return v; }
static size_t ndsEFManagerFileSpan(void **p) { (void)p; return span; }
static int ndsEFManagerMapDescOffsets(const EFDesc *d, EFDesc *mapped)
 { *mapped=*d; return *d->file_head && valid; }
static void draw(GObj *g) { (void)g; }
'''
    for name in ("ndsEFManagerResetDeferredDescs", "ndsEFManagerDeferDesc",
                 "ndsEFManagerRetryDeferredDescs", "ndsEFManagerResolveDescOffsets"):
        code += function(source, name)
    code += r'''
int main(void) {
 void *base=NULL;
 EFDesc d={&base,draw,0x2c30,0,0,0};
 /* A registered file with an unpublished FTData slot must be recoverable. */
 span=12160; valid=1;
 ndsEFManagerResolveDescOffsets(&d);
 assert(d.proc_display==NULL && sNdsEFDeferredCount==1);
 ndsEFManagerResolveDescOffsets(&d);
 assert(sNdsEFDeferredCount==1);
 base=&d; ndsEFManagerRetryDeferredDescs();
 assert(d.proc_display==draw && gNdsEFDescDeferRecoverCount==1);
 ndsEFManagerResetDeferredDescs();
 /* An incomplete preview span must not disable the later battle image. */
 valid=0; ndsEFManagerResolveDescOffsets(&d);
 ndsEFManagerRetryDeferredDescs(); assert(d.proc_display==NULL);
 valid=1; ndsEFManagerRetryDeferredDescs(); assert(d.proc_display==draw);
 ndsEFManagerResetDeferredDescs();
 /* A never-loaded scene restores the definition at the next scene boundary. */
 base=NULL; span=0; ndsEFManagerResolveDescOffsets(&d);
 assert(d.proc_display==NULL);
 ndsEFManagerResetDeferredDescs(); assert(d.proc_display==draw);
 assert(!gNdsEFDescDeferOverflowCount && !sNdsEFDeferredCount);
 return 0;
}
'''
    run_c(tmp_path, code)


def test_single_preview_reuses_files_but_keeps_objects_in_scene_heap(tmp_path):
    css = (ROOT / "src/import/battleship_mnplayers1pgame.c").read_text()
    vs = (ROOT / "src/import/battleship_mnplayersvs.c").read_text()
    names = ("ndsMNPlayers1PGameSkipPreload", "ndsMNPlayers1PGameInitPreviewArena",
             "ndsMNPlayers1PGameRetirePreview", "ndsMNPlayers1PGameMakeFighter",
             "ndsMNPlayers1PGameDestroyFighter")
    code = r'''
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#define TRUE 1
#define FALSE 0
#define NDS_RENDERER_HW_TRIANGLES 1
#define NDS_RENDERER_PROFILE_LEVEL 0
#define NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES (80u*1024u)
enum { nFTKindPlayableStart=0, nFTKindPlayableEnd=11, nFTKindNull=28 };
typedef unsigned u32;
typedef int s32;
typedef struct { unsigned char *start,*ptr,*end; } SYMallocRegion;
typedef struct { s32 fkind, player; } FTDesc;
typedef struct { unsigned nds_slot; } FTStruct;
typedef struct { FTStruct fighter; } GObj;
#define ftGetStruct(g) (&(g)->fighter)
typedef struct {
 void **p_file_main, **p_file_mainmotion, **p_file_submotion, **p_file_model;
 void **p_file_special1, **p_file_special2, **p_file_special3, **p_file_special4;
 void **p_file_shieldpose;
} FTData;
static void *files[12][8];
static FTData data[12], *dFTManagerDataFiles[12];
static SYMallocRegion sNdsPlayers1PGamePreviewArena, *active;
static void *sNdsPlayers1PGamePreviewBase;
static u32 sNdsPlayers1PGamePreviewGeneration, gNdsTaskmanHeapGeneration=1;
static s32 sNdsPlayers1PGamePreviewFkind=nFTKindNull;
static _Alignas(16) unsigned char scene[512*1024];
static size_t used, blocks, loads, destroys;
static GObj object;
static int live, cleanup;
static void *syTaskmanMalloc(size_t bytes, unsigned align) {
 unsigned char *p;
 (void)align;
 if(active) { p=active->ptr; active->ptr+=bytes; assert(active->ptr<=active->end); }
 else { p=scene+used; used+=bytes; assert(used<=sizeof(scene)); ++blocks; }
 return p;
}
static void syMallocInit(SYMallocRegion *r,u32 id,void *base,size_t size)
 { (void)id; r->ptr=r->start=base; r->end=r->start+size; }
static void syMallocReset(SYMallocRegion *r) { assert(cleanup==3); r->ptr=r->start; }
static SYMallocRegion *ndsTaskmanSwapMallocRegion(SYMallocRegion *r)
 { SYMallocRegion *old=active; active=r; return old; }
static void ftManagerSetupFilesAllKind(s32 kind) {
 assert(active==&sNdsPlayers1PGamePreviewArena && !live);
 for(int i=0;i<8;i++) files[kind][i]=syTaskmanMalloc(16,16);
 data[kind].p_file_shieldpose=(void**)files[kind][0]; ++loads;
}
static void ndsFTManagerEnsureOwnerImages(FTDesc *d)
 { (void)d; assert(active==&sNdsPlayers1PGamePreviewArena); syTaskmanMalloc(32,16); }
static void ndsFTManagerRestoreKirbyPreviewMainMotion(void) { assert(!live); }
static void ndsFighterRendererInvalidateMaterialCachesForSlot(u32 slot) { (void)slot; }
static GObj *ftManagerMakeFighter(FTDesc *d) {
 assert(active==NULL && !live); object.fighter.nds_slot=d->player;
 live=1; cleanup=0; return &object;
}
static void ndsFighterManagerRegisterDisplayFighter(GObj *g,u32 slot)
 { (void)g; (void)slot; }
static void ftManagerDestroyFighter(GObj *g)
 { assert(live && g==&object); live=0; ++destroys; }
static void ndsRendererNativeReleaseOwnerImagesInRange(const void *p,size_t size) {
 assert(!live && cleanup==0 && p==sNdsPlayers1PGamePreviewBase);
 assert(size==NDS_PLAYERS_VS_SLOT_RESIDENT_BYTES); cleanup=1;
}
static void ndsRelocReleasePreviewFighter(s32 kind) {
 assert(!live && cleanup==1);
 for(int i=0;i<8;i++) assert(files[kind][i]==NULL);
 assert(data[kind].p_file_shieldpose==NULL); cleanup=2;
}
static void ndsRelocReleaseHeapRange(void *p,size_t size)
 { (void)p; (void)size; assert(cleanup==2); cleanup=3; }
'''
    code += function(vs, "ndsMNPlayersClearPreviewFighterFiles")
    code += "\n".join(function(css, name) for name in names)
    code += r'''
int main(void) {
 for(int k=0;k<12;k++) {
  data[k]=(FTData){&files[k][0],&files[k][1],&files[k][2],&files[k][3],
                  &files[k][4],&files[k][5],&files[k][6],&files[k][7],NULL};
  dFTManagerDataFiles[k]=&data[k]; ndsMNPlayers1PGameSkipPreload(k);
 }
 assert(loads==0 && blocks==0);
 for(int lap=0;lap<3;lap++) for(int k=0;k<12;k++) {
  FTDesc d={k,0}; GObj *g=ndsMNPlayers1PGameMakeFighter(&d);
  assert(blocks==1 && active==NULL);
  ndsMNPlayers1PGameDestroyFighter(g);
  assert(sNdsPlayers1PGamePreviewArena.ptr==sNdsPlayers1PGamePreviewArena.start);
 }
 assert(loads==36 && destroys==36);
 ++gNdsTaskmanHeapGeneration;
 FTDesc d={8,0}; GObj *g=ndsMNPlayers1PGameMakeFighter(&d);
 assert(blocks==2); ndsMNPlayers1PGameDestroyFighter(g);
 return 0;
}
'''
    run_c(tmp_path, code)


def test_go_function_process_yields_for_the_source_sixty_updates(tmp_path):
    source = (ROOT / "decomp/BattleShip-main/decomp/src/if/ifcommon.c").read_text()
    thread = function(source, "ifCommonAnnounceThread")
    assert re.search(r"gcSleepCurrentGObjThread\(60\);\s*gcEjectGObj\(NULL\);", thread)
    adapter = (ROOT / "src/import/battleship_ifcommon.c").read_text()
    code = r'''
#include <assert.h>
typedef unsigned u8;
typedef unsigned u32;
typedef struct GObj { union { int s; } user_data; } GObj;
typedef int GObjProcess;
enum { nGCProcessKindThread, nGCProcessKindFunc };
static unsigned removed, seen_kind, seen_priority;
static void (*seen_proc)(GObj*);
static GObjProcess process;
static void gcEjectGObj(GObj *g) { assert(g); ++removed; }
static void ifCommonAnnounceThread(GObj *g) { (void)g; }
static void other(GObj *g) { (void)g; }
static GObjProcess *gcAddGObjProcess(GObj *g,void (*p)(GObj*),u8 kind,u32 priority)
 { (void)g; seen_proc=p; seen_kind=kind; seen_priority=priority; return &process; }
'''
    code += function(adapter, "ndsIFCommonAnnounceProcUpdate")
    code += function(adapter, "ndsIFCommonAddGObjProcess")
    code += r'''
int main(void) {
 GObj g={0};
 assert(ndsIFCommonAddGObjProcess(&g,ifCommonAnnounceThread,nGCProcessKindThread,5)==&process);
 assert(seen_kind==nGCProcessKindFunc && seen_priority==5);
 for(int i=0;i<60;i++) { seen_proc(&g); assert(!removed); }
 seen_proc(&g); assert(removed==1);
 ndsIFCommonAddGObjProcess(&g,other,nGCProcessKindThread,3);
 assert(seen_kind==nGCProcessKindThread && seen_proc==other && seen_priority==3);
 return 0;
}
'''
    run_c(tmp_path, code)


def test_arwing_removed_dependency_is_only_three_native_baked_texture_refs(tmp_path):
    sys.path.insert(0, str(ROOT / "scripts/3d_vfx"))
    import generate_nds_entry_effects as entry
    resource = entry.census.load_o2r(ROOT, entry.FOX)
    donor = entry.census.load_o2r(ROOT, entry.EXTERN109)
    assert {slot: (target.asset_id, target.offset)
            for slot, target in resource.external.items()} == {
                0x2974: (109, 0x19F8), 0x2AB4: (109, 0x19F8), 0x2B64: (109, 0x19F8)}
    for slot in resource.external:
        assert struct.unpack_from(">I", resource.payload, slot-4)[0] >> 24 == 0xFD
    compiler = entry.Compiler(resource, {161: resource, 109: donor})
    compiler.compile_roots(entry.FOX_ROOTS, len(entry.MARIO_ROOTS))
    assert any(key.image_asset == 109 and key.image_offset == 0x19F8
               for key in compiler.textures)
    runtime = (ROOT / "src/port/reloc_backend_assets.c").read_text()
    code = r'''
#include <assert.h>
#include <stddef.h>
typedef unsigned u32;
typedef int s32;
#define TRUE 1
#define FALSE 0
#define NDS_RENDERER_HW_TRIANGLES 1
#define NDS_RENDERER_PROFILE_LEVEL 0
#define NDS_RELOC_ASSET_FOX_SPECIAL3 161
#define NDS_RELOC_ASSET_EXTERN_DATA_BANK_109 109
static unsigned gNdsSceneManagerCurrIsBattle;
'''
    code += function(runtime, "ndsRelocNativeEntryOwnsDependency")
    code += function(runtime, "ndsRelocResolveNativeEntryExternalFixup")
    code += r'''
int main(void) {
 void *p=0; unsigned slots[]={0x2974,0x2ab4,0x2b64};
 assert(!ndsRelocResolveNativeEntryExternalFixup(161,109,slots[0],0x19f8,&p));
 gNdsSceneManagerCurrIsBattle=1;
 for(unsigned i=0;i<3;i++) {
  assert(ndsRelocResolveNativeEntryExternalFixup(161,109,slots[i],0x19f8,&p)==1);
  assert(p==NULL);
 }
 assert(ndsRelocResolveNativeEntryExternalFixup(161,109,0,0x19f8,&p)==-1);
 assert(ndsRelocResolveNativeEntryExternalFixup(161,109,slots[0],4,&p)==-1);
 assert(!ndsRelocResolveNativeEntryExternalFixup(262,109,slots[0],0x19f8,&p));
 assert(!ndsRelocResolveNativeEntryExternalFixup(161,153,slots[0],0x19f8,&p));
 return 0;
}
'''
    run_c(tmp_path, code)
