"""Execute the CSS lifetime and GO adapter; check Arwing elision against O2R."""
import re
import runpy
import shutil
import struct
import subprocess
import sys
from pathlib import Path

from source_test_helpers import function, source as read_split

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
static u32 sNdsEFDeferredTriedEpoch[NDS_EF_DEFERRED_MAX];
static void *sNdsEFDeferredTriedBase[NDS_EF_DEFERRED_MAX];
static u32 sNdsEFDeferredCount, gNdsEFDescDeferOverflowCount;
static u32 gNdsEFDescDeferRecoverCount, gNdsEFDescUnknownFileCount;
static u32 gNdsEFDescUnknownFileLast, gNdsEFDescDisabledCount, gNdsEFDescDisabledLast;
static size_t span;
static int valid;
/* The loaded-file table's epoch: a file registered or dropped bumps it. */
static u32 files_epoch;
static u32 ndsRelocLoadedFilesEpoch(void) { return files_epoch; }
static intptr_t ndsEFManagerResolveOffset(intptr_t v) { return v; }
static size_t ndsEFManagerFileSpan(void **p) { (void)p; return span; }
static int ndsEFManagerMapDescOffsets(const EFDesc *d, EFDesc *mapped)
 { *mapped=*d; return *d->file_head && valid; }
static void draw(GObj *g) { (void)g; }
/* The three ITCommonObject-relative descs the resolver exempts by identity. */
static EFDesc dEFManagerMBallThrownEffectDesc, dEFManagerCaptureKirbyStarEffectDesc,
              dEFManagerLoseKirbyStarEffectDesc;
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
 /* Nothing changed since that failed retry: the slot is not tried again. */
 valid=1; ndsEFManagerRetryDeferredDescs(); assert(d.proc_display==NULL);
 /* The battle image arrives (the loaded-file table changes). */
 files_epoch++; ndsEFManagerRetryDeferredDescs(); assert(d.proc_display==draw);
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
/* The adapter's other two arms, outside this case. */
static void ifCommonSuddenDeathThread(GObj *g) { (void)g; }
static void ndsIFCommonSuddenDeathProcUpdate(GObj *g) { (void)g; }
static void ifCommonCountdownThread(GObj *g) { (void)g; }
static void ndsIFCommonCountdownProcUpdate(GObj *g) { (void)g; }
typedef struct { int unused; } NDSIFCommonCountdown;
static NDSIFCommonCountdown sNdsIFCommonCountdown;
static int ndsIFCommonNativeOamPrepareClouds(void) { return 1; }
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
    runtime = read_split(ROOT / "src/port/reloc_backend_assets.c")
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
/* The P4 arms (a lab stand-in's skipped donor, a content's baked entry
 * article) own nothing in this case: Fox's Arwing alone. */
static unsigned gNdsP4RelocOutOfRangeSlots;
static int ndsP4LabSkipsDependency(u32 owner, u32 dep) { (void)owner; (void)dep; return FALSE; }
static int ndsP4NativeOwnsDependency(u32 owner, u32 dep) { (void)owner; (void)dep; return FALSE; }
static int ndsP4NativeBakedRef(u32 owner, u32 dep, u32 slot, u32 target)
 { (void)owner; (void)dep; (void)slot; (void)target; return FALSE; }
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
