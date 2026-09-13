"""Execute static-key construction with raw and nonidentity compact reloc views.

The production resolver/key builder are extracted verbatim. Reloc lookup and
span mapping are stubs; this tests their caller's identity/bounds contract,
not the loader or texture upload.
"""
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/menus"))
from source_test_helpers import function


def test_static_keys_map_all_three_pointers_and_reject_missing_spans(tmp_path):
    source = (ROOT / "src/nds/nds_renderer_textures_effects.c").read_text()
    bodies = "\n".join(function(source, name) for name in (
        "ndsRendererHardwareBattleStaticPointer",
        "ndsRendererHardwareBuildBattleStaticTextureKey",
    ))
    fixture = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint32_t u32;
typedef uint8_t u8;
typedef int s32;
#define TRUE 1
#define FALSE 0
#define NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD 2
#define NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD 3
#define NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD 4
typedef struct { u32 width, height, image, tlut_image, texel1_image; }
    NDSRendererHardwareTextureKey;
typedef struct { u32 image_asset_id, tlut_asset_id, image_offset, tlut_offset;
                 u32 key_words[5], logical_width, logical_height; }
    NDSBattlePlayableStaticTextureRecord;
static uintptr_t relocation;
static s32 ndsRelocGetLoadedAssetView(u32 id, const void **base, u32 *size) {
    if (id != 1 && id != 2) return FALSE;
    *base = (void *)(relocation + id * 0x10000u);
    *size = 128;
    return TRUE;
}
#if NDS_P2_MENU_SHELL
static const void *ndsRelocNativeAssetAddress(const void *base, u32 offset) {
    if (offset >= 0x900 && offset < 0x940)
        return (const u8 *)base + offset - 0x900 + 16;
    if (offset == 0xa00) return NULL; /* omitted source span */
    if (offset == 0xb00) return (const u8 *)base + 128; /* bad physical map */
    return (const u8 *)base + offset; /* ordinary/raw asset */
}
#endif
''' + bodies + r'''
int main(void) {
    NDSRendererHardwareTextureKey key;
    NDSBattlePlayableStaticTextureRecord r = {1,2,16,32,{8,8,16,32,48},8,8};
    assert(ndsRendererHardwareBuildBattleStaticTextureKey(&r,&key));
    assert(key.image == 0x10010 && key.tlut_image == 0x20020 && key.texel1_image == 0x10030);
#if NDS_P2_MENU_SHELL
    /* Source offsets exceed physical size and all map nonidentically. */
    r.image_offset=r.key_words[2]=0x900;
    r.tlut_offset=r.key_words[3]=0x910;
    r.key_words[4]=0x920;
    assert(ndsRendererHardwareBuildBattleStaticTextureKey(&r,&key));
    assert(key.image == 0x10010 && key.tlut_image == 0x20020 && key.texel1_image == 0x10030);
    relocation=0x40000;
    assert(ndsRendererHardwareBuildBattleStaticTextureKey(&r,&key));
    assert(key.image == 0x50010 && key.tlut_image == 0x60020 && key.texel1_image == 0x50030);
    r.tlut_offset=r.key_words[3]=0xa00;
    assert(!ndsRendererHardwareBuildBattleStaticTextureKey(&r,&key));
    r.tlut_offset=r.key_words[3]=0x910;
    r.key_words[4]=0xa00;
    assert(!ndsRendererHardwareBuildBattleStaticTextureKey(&r,&key));
    r.key_words[4]=0x920;
    r.image_offset=r.key_words[2]=0xa00;
    assert(!ndsRendererHardwareBuildBattleStaticTextureKey(&r,&key));
    r.image_offset=r.key_words[2]=0xb00;
    assert(!ndsRendererHardwareBuildBattleStaticTextureKey(&r,&key));
#else
    r.image_offset=r.key_words[2]=128;
    assert(!ndsRendererHardwareBuildBattleStaticTextureKey(&r,&key));
    r.image_offset=r.key_words[2]=16;
    relocation=0xffff0000u;
    assert(!ndsRendererHardwareBuildBattleStaticTextureKey(&r,&key));
#endif
    return 0;
}
'''
    compiler = shutil.which("clang") or shutil.which("gcc")
    assert compiler
    path = tmp_path / "static_pointer.c"
    path.write_text(fixture)
    for compact in (0, 1):
        exe = tmp_path / f"static_pointer_{compact}.exe"
        result = subprocess.run([
            compiler, "-std=c11", "-Wall", "-Werror",
            f"-DNDS_P2_MENU_SHELL={compact}", str(path), "-o", str(exe),
        ], capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        result = subprocess.run([str(exe)], capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
