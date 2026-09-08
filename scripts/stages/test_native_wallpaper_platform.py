"""Execute the actual BG2 queue/commit/clear code with hardware register stubs."""
import shutil
import subprocess
from pathlib import Path


def function(text, signature):
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def test_native_wallpaper_affine_ownership(tmp_path):
    repo = Path(__file__).resolve().parents[2]
    source = (repo / "src/nds/nds_platform.c").read_text()
    start = source.index("static s32 sNativeWallpaperAffine[4];")
    end = source.index("void ndsPlatformCommitOriginalSpriteOverlayTransform", start)
    queue = source[start:end]
    clear = function(source, "void ndsPlatformClearOriginalSpriteOverlayLayer(")
    harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
typedef int32_t s32;
typedef uint32_t u32;
typedef uint16_t u16;
#define TRUE 1
#define FALSE 0
#define NDS_RENDERER_HW_TRIANGLES 1
#define NDS_FAST_WALLPAPER_AFFINE 0
static int sOriginalSpriteOverlayBg = 2;
static int sOriginalSpriteOverlayForegroundBg = 3;
static u32 sOriginalSpriteOverlayEpoch[2] = {1, 1};
static u32 gNdsOriginalSpriteBg2ClearBytes, gNdsOriginalSpriteBg3ClearBytes;
static u16 pixels[256 * 192];
static s32 registers[4];
static u32 writes;
static void bgSetAffineMatrixScroll(int bg, s32 pa, s32 pb, s32 pc,
                                    s32 pd, s32 dx, s32 dy)
{
    assert(bg == 2 && pb == 0 && pc == 0);
    registers[0] = pa; registers[1] = pd;
    registers[2] = dx; registers[3] = dy;
    writes++;
}
static u32 ndsPlatformOriginalSpriteOverlayClearPixels(void) { return 256 * 192; }
static void *bgGetGfxPtr(int bg) { assert(bg == 2 || bg == 3); return pixels; }
static void dmaFillHalfWords(u16 value, void *dest, u32 bytes)
{
    u32 i;
    assert(bytes == sizeof(pixels) && dest == pixels);
    for (i = 0; i < bytes / 2; i++) pixels[i] = value;
}
static u32 ndsPlatformAdvanceOriginalSpriteOverlayEpoch(u32 layer)
{
    return ++sOriginalSpriteOverlayEpoch[layer];
}
''' + queue + clear + r'''
int main(void)
{
    assert(ndsPlatformQueueNativeWallpaperAffine(200, 201, -12, 13));
    assert(writes == 0); /* Queue never writes active scanout registers. */
    ndsPlatformCommitNativeWallpaperAffine();
    assert(writes == 1 && registers[0] == 200 && registers[3] == 13);
    ndsPlatformCommitNativeWallpaperAffine();
    assert(writes == 1); /* One commit per queued change. */
    assert(ndsPlatformQueueNativeWallpaperAffine(100, 100, 0, 0));
    sOriginalSpriteOverlayEpoch[0]++;
    ndsPlatformCommitNativeWallpaperAffine();
    assert(writes == 1); /* A replaced bitmap cannot inherit a stale queue. */
    assert(!ndsPlatformQueueNativeWallpaperAffine(0, 200, 0, 0));
    assert(!ndsPlatformQueueNativeWallpaperAffine(200, -1, 0, 0));
    assert(!ndsPlatformQueueNativeWallpaperAffine(32768, 200, 0, 0));
    assert(!ndsPlatformQueueNativeWallpaperAffine(200, 200, 134217728, 0));
    assert(!ndsPlatformQueueNativeWallpaperAffine(200, 200, 0, -134217729));
    sOriginalSpriteOverlayBg = -1;
    assert(!ndsPlatformQueueNativeWallpaperAffine(200, 200, 0, 0));
    sOriginalSpriteOverlayBg = 2;
    assert(ndsPlatformQueueNativeWallpaperAffine(180, 181, 50, 51));
    ndsPlatformClearOriginalSpriteOverlayLayer(FALSE);
    ndsPlatformCommitNativeWallpaperAffine();
    assert(writes == 2 && registers[0] == 256 && registers[1] == 256);
    assert(registers[2] == 0 && registers[3] == 0);
    assert(gNdsOriginalSpriteBg2ClearBytes == sizeof(pixels));
    assert(ndsPlatformQueueNativeWallpaperAffine(190, 191, 60, 61));
    ndsPlatformClearOriginalSpriteOverlayLayer(TRUE);
    ndsPlatformCommitNativeWallpaperAffine();
    assert(writes == 3 && registers[0] == 190 && registers[3] == 61);
    return 0;
}
'''
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler, "A host C compiler is required for native BG ownership tests"
    c_file, binary = tmp_path / "ownership.c", tmp_path / "ownership.exe"
    c_file.write_text(harness)
    compiled = subprocess.run([compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
                               str(c_file), "-o", str(binary)],
                              capture_output=True, text=True, timeout=60)
    assert compiled.returncode == 0, compiled.stdout + compiled.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert run.returncode == 0, run.stdout + run.stderr
