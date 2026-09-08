"""Check source-fixed KO RGB endpoints and execute native palette preparation."""
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/menus"))
sys.path.insert(0, str(ROOT / "scripts/3d_vfx"))
from source_test_helpers import braced, function
import generate_nds_entry_effects as gen


def test_ko_rgb_is_constant_through_source_alpha_animation():
    source = (ROOT / "decomp/BattleShip-main/decomp/src/ef/efmanager.c").read_text()
    assets = (ROOT / "src/nds/nds_renderer_assets.c").read_text()
    resource = gen.census.load_o2r(ROOT, gen.CATCH)
    prim_table = braced(assets, r"static const u32 sNdsEntryKoPrimRgb\[", True)
    env_table = braced(assets, r"static const u32 sNdsEntryKoEnvRgb\[", True)
    prim = [int(v, 16) for v in re.findall(r"0x[0-9a-fA-F]+", prim_table)]
    env = [int(v, 16) for v in re.findall(r"0x[0-9a-fA-F]+", env_table)]
    assert len(prim) == len(env) == 12
    for player, table in enumerate((0x58e0, 0x5800, 0x5950, 0x5870)):
        for part in range(3):
            pointer_table = resource.pointer_at(table + (part + 1) * 4)
            script = resource.pointer_at(pointer_table.offset)
            values = [struct.unpack_from(">I", resource.payload, script.offset + n * 8 + 4)[0]
                      for n in range(3)]
            assert {v & 0xffffff00 for v in values} == {prim[part * 4 + player]}
            assert (values[-1] & 255) == 0
    for part, family in ((0, "Child"), (2, "Sibling")):
        channels = []
        for channel in "RGB":
            declaration = braced(source,
                rf"u8 dEFManagerDeadExplodeEnvColor{family}{channel}\[", True)
            channels.append([int(v, 16) for v in re.findall(r"0x[0-9a-fA-F]+", declaration)])
        for player in range(4):
            expected = sum(channels[c][player] << (24 - c * 8) for c in range(3))
            assert env[part * 4 + player] == expected
    assert env[4:8] == [0xfcf69000] * 4


def test_palette_only_names_are_reused_and_alpha_does_not_change_binding(tmp_path):
    assets = (ROOT / "src/nds/nds_renderer_assets.c").read_text()
    common = (ROOT / "src/nds/nds_renderer_native_common.c").read_text()
    textures = (ROOT / "src/nds/nds_renderer_textures_effects.c").read_text()
    tables = "\n".join(braced(assets, rf"static const u32 {name}\[", True)
                       for name in ("sNdsEntryKoRootOffsets", "sNdsEntryKoPrimRgb", "sNdsEntryKoEnvRgb"))
    code = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
#define TRUE 1
#define FALSE 0
#define GL_TEXTURE_2D 0
#define GL_COLOR_TABLE_WIDTH_EXT 1
#define NDS_ENTRY_EFFECT_TEXTURE_COUNT 3
#define NDS_ENTRY_EFFECT_TEXTURE_A5I3 1
typedef struct {u32 first_group,group_count;} NDSEntryEffectRoot;
typedef struct {u32 texture_slot;} Group;
typedef struct {u32 ds_format,palette_entries; const u16 *palette;} NDSEntryEffectTexture;
static u32 sNdsEntryKoPaletteName[3][4];
static u16 sNdsEntryKoPaletteScratch[8];
static u32 sNdsRendererHardwareBoundTextureName;
static u16 ramp[8]={0,0x1084,0x2108,0x318c,0x4210,0x5294,0x6318,0x7fff};
static const NDSEntryEffectTexture sNdsEntryEffectTextures[3]={{1,8,ramp},{1,8,ramp},{1,8,ramp}};
static const NDSEntryEffectRoot roots[3]={{0,1},{1,1},{2,1}};
static const Group sNdsEntryEffectGroups[3]={{0},{1},{2}};
static u32 sNdsRendererEntryEffectTextureName[3]={100,101,102};
static u16 colors[20][8];
static int active, generated, deleted, fail_at=7;
''' + tables + r'''
static const NDSEntryEffectRoot *ndsRendererEntryEffectRoot(u32 owner,u32 offset)
{
    unsigned part; assert(owner==84);
    for(part=0;part<3;part++) if(offset==sNdsEntryKoRootOffsets[part]) return &roots[part];
    return NULL;
}
static int ndsRendererHardwareFencedGlGenTextures(int n,int *name)
{ assert(n==1); *name=++generated; return 1; }
static void ndsRendererHardwareBindTextureState(u32 name) { active=(int)name; }
static void glAssignColorTable(int target,int name)
{ (void)target; assert(active>=100 && active<=102 && name>0 && name<20); }
static void glColorTableEXT(int target,int a,int width,int b,int c,const u16 *palette)
{ (void)target;(void)a;(void)b;(void)c;assert(width==8); memcpy(colors[active],palette,16); }
static void glGetColorTableParameterEXT(int target,int kind,int *width)
{ (void)target;assert(kind==1);*width=(active==fail_at)?-1:8; }
static void ndsRendererHardwareFencedGlDeleteTextures(int n,int *name)
{ assert(n==1 && *name==fail_at); deleted++; }
''' + function(textures, "ndsRendererHardwareBlendPrimEnvTexel0") + \
        function(common, "ndsRendererPrepareEntryKoPalettes") + \
        function(common, "ndsRendererEntryKoPalette") + r'''
int main(void)
{
    unsigned part,player,alpha;
    assert(!ndsRendererPrepareEntryKoPalettes());
    assert(generated==7 && deleted==1 && sNdsEntryKoPaletteName[1][2]==0);
    fail_at=0;
    assert(ndsRendererPrepareEntryKoPalettes());
    assert(generated==13);
    assert(ndsRendererPrepareEntryKoPalettes() && generated==13);
    for(part=0;part<3;part++) for(player=0;player<4;player++)
    {
        u32 name=ndsRendererEntryKoPalette(part,sNdsEntryKoPrimRgb[part][player],sNdsEntryKoEnvRgb[part][player]);
        assert(name!=0);
        for(alpha=0;alpha<256;alpha++)
            assert(ndsRendererEntryKoPalette(part,sNdsEntryKoPrimRgb[part][player]|alpha,
                       sNdsEntryKoEnvRgb[part][player]|alpha)==name);
        assert(colors[name][0]==ndsRendererHardwareBlendPrimEnvTexel0(0,
                    sNdsEntryKoPrimRgb[part][player],sNdsEntryKoEnvRgb[part][player]));
        assert(colors[name][7]==ndsRendererHardwareBlendPrimEnvTexel0(0x7fff,
                    sNdsEntryKoPrimRgb[part][player],sNdsEntryKoEnvRgb[part][player]));
    }
    assert(!ndsRendererEntryKoPalette(3,0,0));
    assert(!ndsRendererEntryKoPalette(0,0x12345600,0));
    return 0;
}
'''
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler, "Host C compiler required"
    cfile, binary = tmp_path / "ko.c", tmp_path / "ko.exe"
    cfile.write_text(code)
    built = subprocess.run([compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
                            str(cfile), "-o", str(binary)], capture_output=True, text=True, timeout=60)
    assert built.returncode == 0, built.stdout + built.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert run.returncode == 0, run.stdout + run.stderr
