"""Execute the source-effect attachment path with real mixed-field conversion."""
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/menus"))
from source_test_helpers import braced, function


def test_tree_attachment_converts_loaded_material_without_mutating_source(tmp_path):
    types = (ROOT / "decomp/BattleShip-main/decomp/src/sys/objtypes.h").read_text()
    assets = (ROOT / "src/port/reloc_backend_assets.c").read_text()
    compat = (ROOT / "src/port/reloc_backend_compat_shims.c").read_text()
    material = braced(types, r"struct MObjSub\s*\{", True)
    normalize = "\n".join(function(assets, name) for name in (
        "ndsRelocSwapS16Pair", "ndsRelocReverseColorPackBytes",
        "ndsRelocNormalizeMObjSubWordSwapped"))
    attach = function(compat, "lbCommonAddMObjForTreeDObjs")
    source = r'''
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;
typedef union { struct { u8 r,g,b,a; } s; u32 pack; } SYColorPack;
typedef struct MObjSub MObjSub;
''' + material + normalize + r'''
typedef struct DObj { struct DObj *next; } DObj;
static struct { u32 scene_curr; } gSCManagerSceneData = {22};
#define NDS_NATIVE_FAILURE_STAGE 2
#define NDS_NATIVE_FAILURE_BAD_ASSET 4
static MObjSub raw, native, attached[4];
static int copies, attachments, failures, reject;
static s32 ndsRelocCopyMObjSubForAttachment(MObjSub *dst, const MObjSub *src)
{
    if (reject) return -1;
    *dst = *src;
    copies++;
    if (src == &raw) { ndsRelocNormalizeMObjSubWordSwapped(dst); return 1; }
    return 0;
}
static void gcAddMObjForDObj(DObj *dobj, MObjSub *src)
{
    assert(dobj != NULL && src != &raw && src != &native);
    attached[attachments++] = *src;
}
static DObj *lbCommonGetTreeDObjNextFromRoot(DObj *dobj, DObj *root)
{
    (void)root;
    return dobj->next;
}
static void ndsRendererRecordNativeFailure(u32 domain,u32 scene,u32 id,u32 status,
                                           u32 root,u32 material,u32 reason)
{
    (void)id; (void)status; (void)root; (void)material;
    assert(domain==2 && scene==22 && reason==4);
    failures++;
}
''' + attach + r'''
int main(void)
{
    DObj second = {NULL}, first = {&second};
    MObjSub *raw_list[] = {&raw, NULL}, *native_list[] = {&native, NULL};
    MObjSub **tables[] = {raw_list, native_list};
    /* 84:0x22d0 after the loader's u32 swap, before mixed-field repair. */
    raw.pad00=0x0402; raw.fmt=0; raw.siz=0;
    raw.flags=0x0400; raw.block_fmt=0; raw.block_siz=2;
    raw.unk08=0; raw.unk0A=32; raw.unk0C=16; raw.unk0E=32;
    raw.primcolor.s.r=255; raw.primcolor.s.g=125;
    raw.primcolor.s.b=255; raw.primcolor.s.a=255;
    native.flags=0x0200; native.fmt=4; native.siz=2;
    native.block_fmt=4; native.block_siz=0;
    lbCommonAddMObjForTreeDObjs(&first, tables);
    assert(copies==2 && attachments==2 && failures==0);
    assert(attached[0].flags==0x0200 && attached[0].pad00==0);
    assert(attached[0].fmt==4 && attached[0].siz==2);
    assert(attached[0].block_fmt==4 && attached[0].block_siz==0);
    assert(attached[0].unk08==32 && attached[0].unk0A==0);
    assert(attached[0].unk0C==32 && attached[0].unk0E==16);
    assert(attached[0].primcolor.s.g==255 && attached[0].primcolor.s.b==125);
    assert(attached[1].flags==0x0200 && attached[1].fmt==4);
    assert(raw.flags==0x0400 && raw.pad00==0x0402);
    reject=1;
    lbCommonAddMObjForTreeDObjs(&first, tables);
    assert(failures==1 && attachments==2);
    return 0;
}
'''
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler, "Host C compiler required"
    c_file, binary = tmp_path / "attachment.c", tmp_path / "attachment.exe"
    c_file.write_text(source)
    compiled = subprocess.run([compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
                               "-Wno-pointer-to-int-cast", str(c_file), "-o", str(binary)],
                              capture_output=True, text=True, timeout=60)
    assert compiled.returncode == 0, compiled.stdout + compiled.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert run.returncode == 0, run.stdout + run.stderr
