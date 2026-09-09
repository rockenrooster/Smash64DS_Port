"""Host-execute native shield palette preparation and its failure/retry path."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/menus"))
from source_test_helpers import braced, function


def test_shield_palette_variants_use_source_colors_and_retain_live_names(tmp_path):
    assets = (ROOT / "src/nds/nds_renderer_assets.c").read_text()
    common = (ROOT / "src/nds/nds_renderer_native_common.c").read_text()
    textures = (ROOT / "src/nds/nds_renderer_textures_effects.c").read_text()
    source = (ROOT / "decomp/BattleShip-main/decomp/src/ef/efmanager.c").read_text()
    colors = braced(source, r"SYColorRGBPair dEFManagerShieldColors\[", True)
    endpoints = re.findall(r"\{\s*\{([^}]+)\},\s*\{([^}]+)\}", colors)
    assert len(endpoints) == 5
    expected = []
    for prim, env in endpoints:
        assert [int(v, 16) for v in re.findall(r"0x[0-9a-fA-F]+", prim)] == [255] * 3
        r, g, b = [int(v, 16) for v in re.findall(r"0x[0-9a-fA-F]+", env)]
        expected.append((r << 24) | (g << 16) | (b << 8) | 0xc0)
    environment = braced(assets, r"static const u32 sNdsEntryShieldEnvironment\[", True)
    actual = [int(v, 16) for v in re.findall(r"0x[0-9a-fA-F]+", environment)]
    assert actual == expected
    blend = function(textures, "ndsRendererHardwareBlendPrimEnvTexel0")
    prepare = function(common, "ndsRendererPrepareEntryShieldTextures")
    state_start = common.index("        stats->othermode_h = (initial_othermode_h")
    state_end = common.index("        ndsRendererRecordSetCombine", state_start)
    state_updates = common[state_start:state_end]
    harness = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
#define FALSE 0
#define TRUE 1
#define NDS_ENTRY_EFFECT_TEXTURE_A3I5 3
typedef struct {
    u32 ds_format, palette_entries, width, height;
    const u16 *palette;
} NDSEntryEffectTexture;
static u32 sNdsEntryShieldTextureName[5];
static u32 gNdsEntryEffectNativeTexturePrepareCount;
static u16 captured[5][32];
static u32 calls, fail_call;
static void ndsRendererEntryEffectTextureFill(void) {}
static s32 ndsRendererHardwarePrepareIFCommonA3I5Atlas(
    u32 width, u32 height, const u16 *palette, void (*fill)(void),
    void *user, u32 *name)
{
    u32 slot = (u32)(name - sNdsEntryShieldTextureName);
    assert(width == 16 && height == 32 && slot < 5 && user != 0);
    assert(fill == ndsRendererEntryEffectTextureFill);
    calls++;
    if (calls == fail_call) { return FALSE; }
    memcpy(captured[slot], palette, sizeof(captured[slot]));
    *name = 100 + slot;
    return TRUE;
}
''' + environment + blend + prepare + r'''
typedef struct { u32 a, b; } Pair;
typedef struct { u32 prim_color_index, env_color_index; } Group;
typedef struct { u32 prim_color, env_color, othermode_h, othermode_l; } Stats;
static u32 sNdsEntryEffectPrimColors[2] = {0xffffffffu, 0xffffffc0u};
static u32 sNdsEntryEffectEnvColors[1] = {0xffffffffu};
static void apply_state(Stats *stats, const Pair *othermode_state,
                         const Pair *othermode_writes, const Group *group,
                         u32 color_writes)
{
    u32 initial_prim_color = stats->prim_color;
    u32 initial_env_color = stats->env_color;
    u32 initial_othermode_h = stats->othermode_h;
    u32 initial_othermode_l = stats->othermode_l;
''' + state_updates + r'''
}
int main(void)
{
    /* Thirty-two entries now: the shield moved to A3I5 because the palette
     * entry count IS how many distinct colours it can show, and eight banded. */
    u16 palette[32];
    NDSEntryEffectTexture texture = {3, 32, 16, 32, palette};
    unsigned variant, color, channel;
    for (color = 0; color < 32; color++)
    {
        /* The generator's own 32-entry grayscale ramp: identity per channel. */
        palette[color] = (u16)(color | (color << 5) | (color << 10));
    }
    fail_call = 3;
    assert(!ndsRendererPrepareEntryShieldTextures(&texture));
    assert(calls == 3 && gNdsEntryEffectNativeTexturePrepareCount == 2);
    assert(sNdsEntryShieldTextureName[0] == 100);
    assert(sNdsEntryShieldTextureName[1] == 101);
    assert(sNdsEntryShieldTextureName[2] == 0);
    fail_call = 0;
    assert(ndsRendererPrepareEntryShieldTextures(&texture));
    assert(calls == 6 && gNdsEntryEffectNativeTexturePrepareCount == 5);
    assert(ndsRendererPrepareEntryShieldTextures(&texture));
    assert(calls == 6); /* Reuse cannot rewrite palettes of queued polygons. */
    for (variant = 0; variant < 5; variant++)
    {
        assert(sNdsEntryShieldTextureName[variant] == 100 + variant);
        for (color = 0; color < 32; color++)
        {
            unsigned result = 0;
            unsigned intensity = palette[color] & 31;
            for (channel = 0; channel < 3; channel++)
            {
                unsigned env = (sNdsEntryShieldEnvironment[variant] >>
                                (27 - channel * 8)) & 31;
                unsigned value = (env * (31 - intensity) + 31 * intensity + 15) / 31;
                result |= value << (channel * 5);
            }
            assert(captured[variant][color] == result);
        }
    }
    texture.palette_entries = 16;
    assert(!ndsRendererPrepareEntryShieldTextures(&texture));
    assert(calls == 6);
    {
        Stats stats = {0x11223344, 0xff0000c0, 0xa5a5a5a5, 0x5a5a5a5a};
        Pair baked = {0, 1}, writes = {0, 3};
        Group group = {0, 0};
        apply_state(&stats, &baked, &writes, &group, 1);
        assert(stats.prim_color == 0xffffffff); /* Explicit white is a write. */
        assert(stats.env_color == 0xff0000c0); /* Player red survives. */
        assert(stats.othermode_h == 0xa5a5a5a5);
        assert(stats.othermode_l == 0x5a5a5a59); /* Only alpha-compare bits change. */
        stats.prim_color = 0x12345678;
        stats.env_color = 0x87654321;
        stats.othermode_l = 4;
        baked.a = 0x8000; baked.b = 0x553049;
        writes.a = 0xc000; writes.b = 0xfffffffb;
        apply_state(&stats, &baked, &writes, &group, 0);
        assert(stats.prim_color == 0x12345678 && stats.env_color == 0x87654321);
        assert(stats.othermode_l == 0x55304d); /* Reflector preserves unwritten bit 2. */
    }
    return 0;
}
'''
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler, "Host C compiler required"
    cfile, binary = tmp_path / "shield.c", tmp_path / "shield.exe"
    cfile.write_text(harness)
    compiled = subprocess.run([compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
                               str(cfile), "-o", str(binary)],
                              capture_output=True, text=True, timeout=60)
    assert compiled.returncode == 0, compiled.stdout + compiled.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert run.returncode == 0, run.stdout + run.stderr


if __name__ == "__main__":
    raise SystemExit(subprocess.run(
        [sys.executable, "-m", "pytest", str(Path(__file__).resolve()), "-q"],
        cwd=ROOT,
    ).returncode)
