#!/usr/bin/env python3
"""Execution test for the Options middle-tab IA4 repeat fix.

Measured shape (not IA8): `mnOptionMakeOptionTabs` builds the middle tab with
`cms = 0`, `cmt = 0`, `masks = 4`, `maskt = 0`, `lrs = 17 * 8 = 136`,
`lrt = 29`; the asset row `0x0330u, 8u, 29u, 1u, G_IM_FMT_IA, G_IM_SIZ_4b`
(`src/port/reloc_backend_assets.c`) decodes to an 8x29 IA4 bitmap on a
16-texel stride. The 136-column draw is 8 full 16-texel periods plus a partial
final tile of 8. The old blitter rejected IA4 at the format gate, so the tab
drew nothing; Main has now admitted IA4 with a 3-bit intensity / 1-bit alpha
palette and a combined I4/IA4 sampling arm.

What this proves: the VERBATIM extracted `ndsDrawSObjIntoPreview` (plus its
`ndsSObjMapTexel` helper) is compiled with stubbed unrelated dependencies and
driven with the source-derived middle shape. IA4/4b is admitted, 136 columns
tile with period 16, odd nibbles fill while even nibbles stay transparent,
negative-origin clipping maps to texel 5, TEXSHUF odd rows swap halves, and
the 2x path fills rects. An unsupported format is still rejected. On original
HEAD (or the repeat-only version) the IA4 scenarios return FALSE at the
format gate, so they fail there and pass here. Negative control
`builds/resume-20260905/sobj-repeat/oldgate_check.py` rebuilds the harness
minus only the admission arm: all five IA4 scenarios fail there, badfmt
still passes. No pixel-loop logic is
reimplemented: expectations are zero/nonzero presence, cross-period equality,
and exact nonzero counts from the known half/half fixture.

Not proven here: ROM/emulator runs or screenshots (Main owns those); scaled
non-2x factors; CI/RGBA arms (untouched by this fix).

Known remaining IA4 guard (reported, not edited): `ndsSObjPreviewBasicSupported`
still lacks the IA4/4b arm, so the frame foreground loop and the
portraits/name census skip IA4 SObjs before they reach this blitter.

No live C edits, ROM builds, emulator runs, or asset regen were used.
Scratch binaries live under builds/resume-20260905/sobj-repeat only.
"""
import shutil
import subprocess
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from source_test_helpers import function  # noqa: E402

ROOT = HERE.parent.parent
BACKEND = ROOT / "src/port/sprite_preview_backend.c"
SCRATCH = ROOT / "builds/resume-20260905/sobj-repeat"


def backend_text():
    return BACKEND.read_text()


HARNESS_TEMPLATE = r'''
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;

#define TRUE 1
#define FALSE 0

#define G_IM_FMT_RGBA 0
#define G_IM_FMT_YUV 1
#define G_IM_FMT_CI 2
#define G_IM_FMT_IA 3
#define G_IM_FMT_I 4
#define G_IM_SIZ_4b 0
#define G_IM_SIZ_8b 1
#define G_IM_SIZ_16b 2
#define G_IM_SIZ_32b 3

#define SP_FASTCOPY 0x20u
#define SP_TEXSHUF 0x200u

#define NDS_OPENING_ACTION_PREVIEW_MAX_HEIGHT 264u

#define NDS_STARTUP_LOGO_BLOCKER_NONE 0u
#define NDS_STARTUP_LOGO_BLOCKER_NO_SOBJ 1u
#define NDS_STARTUP_LOGO_BLOCKER_UNSUPPORTED_FORMAT 2u
#define NDS_STARTUP_LOGO_BLOCKER_BAD_DIMENSIONS 3u
#define NDS_STARTUP_LOGO_BLOCKER_BAD_BITMAP_TABLE 4u
#define NDS_STARTUP_LOGO_BLOCKER_BAD_BITMAP_BUFFER 5u
#define NDS_STARTUP_LOGO_BLOCKER_BAD_BITMAP_BUFFER 5u
#define NDS_STARTUP_LOGO_BLOCKER_NO_PREVIEW_BUFFER 6u
#define NDS_STARTUP_LOGO_BLOCKER_NO_VISIBLE_SOBJ 7u
#define NDS_STARTUP_LOGO_DRAW_PASS 0x4c445257u
#define NDS_OPENING_PORTRAITS_DRAW_PASS 0x4f504457u
#define NDS_OPENING_MARIO_DRAW_PASS 0x4f4d4457u
#define NDS_OPENING_NAME_DRAW_PASS 0x4f4e4457u
#define NDS_OPENING_MOVIE_ACTION_PREVIEW_PASS 0x4f4d4150u
#define NDS_TITLE_DRAW_PASS 0x54494457u

enum {
    nSCKindOpeningPortraits = 10u,
    nSCKindOpeningMario = 11u,
    nSCKindTitle = 12u,
    nSCKindOpeningRun = 20u,
    nSCKindOpeningNewcomers = 28u
};

typedef u32 GCUserData;
typedef struct { f32 x, y; } Vec2f;
typedef struct { u8 r, g, b, a; } SYColorRGBA;

typedef struct bitmap {
    s16 width;
    s16 width_img;
    s16 s;
    s16 t;
    void *buf;
    s16 actualHeight;
    s16 LUToffset;
} Bitmap;

typedef struct sprite {
    s16 x, y;
    s16 width, height;
    f32 scalex, scaley;
    s16 expx, expy;
    u16 attr;
    s16 zdepth;
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
    s16 startTLUT;
    s16 nTLUT;
    int *LUT;
    s16 istart;
    s16 istep;
    s16 nbitmaps;
    s16 ndisplist;
    s16 bmheight;
    s16 bmHreal;
    u8 bmfmt;
    u8 bmsiz;
    Bitmap *bitmap;
    void *rsp_dl;
    void *rsp_dl_next;
    s16 frac_s, frac_t;
} Sprite;

typedef struct SObj {
    struct SObj *alloc_free;
    void *parent_gobj;
    struct SObj *next;
    struct SObj *prev;
    Sprite sprite;
    GCUserData user_data;
    Vec2f pos;
    SYColorRGBA envcolor;
    u8 cmt, cms;
    u8 maskt, masks;
    u16 lrs, lrt;
} SObj;

typedef struct NDSRelocLoadedFile { int dummy; } NDSRelocLoadedFile;
typedef struct { u32 scene_curr; } SCManagerSceneData;

/* Production globals referenced by the extracted blitter; benign values. */
u32 gNdsStartupLogoDrawWidth, gNdsStartupLogoDrawHeight;
u32 gNdsStartupLogoDrawFormat, gNdsStartupLogoDrawSize;
u32 gNdsStartupLogoDrawBitmaps, gNdsStartupLogoDrawTexshuf;
u32 gNdsStartupLogoDrawTexshufSamples, gNdsStartupLogoDrawBlocker;
u32 gNdsStartupLogoDrawPixels, gNdsStartupLogoDrawResult;
u32 gNdsOpeningPortraitsDrawWidth, gNdsOpeningPortraitsDrawHeight;
u32 gNdsOpeningPortraitsDrawFormat, gNdsOpeningPortraitsDrawSize;
u32 gNdsOpeningPortraitsDrawBitmaps, gNdsOpeningPortraitsDrawPixels;
u32 gNdsOpeningPortraitsDrawResult, gNdsOpeningPortraitsDrawBlocker;
u32 gNdsOpeningMarioDrawWidth, gNdsOpeningMarioDrawHeight;
u32 gNdsOpeningMarioDrawFormat, gNdsOpeningMarioDrawSize;
u32 gNdsOpeningMarioDrawBitmaps, gNdsOpeningMarioDrawPixels;
u32 gNdsOpeningMarioDrawResult, gNdsOpeningMarioDrawBlocker;
u32 gNdsOpeningNameSceneDrawWidth, gNdsOpeningNameSceneDrawHeight;
u32 gNdsOpeningNameSceneDrawFormat, gNdsOpeningNameSceneDrawSize;
u32 gNdsOpeningNameSceneDrawBitmaps, gNdsOpeningNameSceneDrawPixels;
u32 gNdsOpeningNameSceneDrawResult, gNdsOpeningNameSceneDrawMask;
u32 gNdsOpeningNameSceneDrawBlocker;
u32 gNdsTitleDrawLastWidth, gNdsTitleDrawLastHeight;
u32 gNdsTitleDrawLastFormat, gNdsTitleDrawLastSize;
u32 gNdsTitleDrawPixels, gNdsTitleDrawResult;
u32 gNdsOpeningMovieActionPreviewResult, gNdsOpeningMovieActionPreviewMask;
u32 gNdsOpeningMovieActionPreviewPixels, gNdsOpeningMovieActionPreviewLastKind;
u32 gNdsOpeningMovieActionPreviewLastWidth, gNdsOpeningMovieActionPreviewLastHeight;
u32 gNdsOpeningMovieActionPreviewLastFormat, gNdsOpeningMovieActionPreviewLastSize;
u32 gNdsSObjWallpaperCacheFallbackCount;
SCManagerSceneData gSCManagerSceneData;
static NDSRelocLoadedFile g_test_file;
u32 g_test_blocker = 0xFFFFFFFFu;

/* Unrelated dependencies, stubbed. Only the lerp stub feeds pixels, and the
 * test asserts presence/equality of its output, never its values. */
void ndsRecordSObjDrawBlocker(u32 record_startup, u32 blocker)
{
    (void)record_startup;
    g_test_blocker = blocker;
}
NDSRelocLoadedFile *ndsRelocFindLoadedFileContaining(const void *p, size_t n)
{
    (void)p; (void)n;
    return &g_test_file;
}
int ndsRelocPointerRangeInLoadedFile(NDSRelocLoadedFile *f, const void *p, size_t n)
{
    (void)f; (void)p; (void)n;
    return TRUE;
}
void ndsSObjApplyDreamLandWallpaperStretch(s32 *ox, s32 *oy, u32 *sx, u32 *sy, u32 k)
{
    (void)ox; (void)oy; (void)sx; (void)sy; (void)k;
}
u32 ndsSObjDrawCachedWallpaper(SObj *a, NDSRelocLoadedFile *b, Sprite *c,
    u16 *d, u32 e, u32 f, u32 g, s32 h, s32 i, u32 j, u32 k)
{
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g;
    (void)h; (void)i; (void)j; (void)k;
    return 0u;
}
u16 ndsSpriteLerpPrimEnv(const SObj *sobj, u8 intensity)
{
    (void)sobj;
    return (u16)(0x8000u | (u32)intensity);
}
u16 ndsSpritePackRgb15(u8 r, u8 g, u8 b)
{
    (void)r; (void)g; (void)b;
    return (u16)0x8001u;
}
u16 ndsStartupLogoConvertRgba16(u16 c) { return (u16)(c | 0x8000u); }
u16 ndsStartupLogoReadRgba16Pixel(const u16 *p, u32 w, u32 r, u32 c, u32 t)
{
    (void)p; (void)w; (void)r; (void)c; (void)t;
    return (u16)0x8001u;
}
u16 ndsSpriteConvertRgba32(u32 v) { (void)v; return (u16)0x8001u; }
s32 ndsOpeningIsImportedNameScene(u32 k) { (void)k; return FALSE; }
u32 ndsOpeningNameSceneMask(u32 k) { (void)k; return 0u; }

__EXTRACTED_ADMISSION__
__EXTRACTED_MAPPER__
__EXTRACTED_BLITTER__

#define FB_W 320u
#define FB_H 64u
#define SENTINEL 0x4D4Du

static u8 g_phys[8 * 29];
static u16 g_fb[FB_W * FB_H];
static Bitmap g_bmp;
static SObj g_sobj;

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #x); \
    return 1; } } while (0)

/* Logical byte j of each row holds texels 2j/2j+1; the production `^ 3`
 * word swizzle is fixture layout, not loop logic. Half pattern: texels
 * 0..7 opaque (odd nibbles), 8..15 transparent (even nibbles). */
static void fixture_patterned(void)
{
    u32 row, j;
    for (row = 0u; row < 29u; row++)
    {
        for (j = 0u; j < 8u; j++)
        {
            g_phys[row * 8u + (j ^ 3u)] = (j < 4u) ? 0xFFu : 0xEEu;
        }
    }
}

static void fixture_uniform(u8 value)
{
    memset(g_phys, value, sizeof(g_phys));
}

static void sobj_middle_tab(u8 bmfmt, u16 attr, f32 sx, f32 sy)
{
    memset(&g_sobj, 0, sizeof(g_sobj));
    memset(&g_bmp, 0, sizeof(g_bmp));
    g_sobj.sprite.width = 8;
    g_sobj.sprite.height = 29;
    g_sobj.sprite.scalex = sx;
    g_sobj.sprite.scaley = sy;
    g_sobj.sprite.attr = attr;
    g_sobj.sprite.red = 255u;
    g_sobj.sprite.green = 255u;
    g_sobj.sprite.blue = 255u;
    g_sobj.sprite.alpha = 255u;
    g_sobj.sprite.nbitmaps = 1;
    g_sobj.sprite.bmheight = 29;
    g_sobj.sprite.bmfmt = bmfmt;
    g_sobj.sprite.bmsiz = G_IM_SIZ_4b;
    g_sobj.sprite.bitmap = &g_bmp;
    g_bmp.width = 8;
    g_bmp.width_img = 16;
    g_bmp.buf = g_phys;
    g_bmp.actualHeight = 29;
    g_sobj.cms = 0u;
    g_sobj.cmt = 0u;
    g_sobj.masks = 4u;
    g_sobj.maskt = 0u;
    g_sobj.lrs = 136u;
    g_sobj.lrt = 29u;
}

static void fb_fill(void)
{
    u32 i;
    for (i = 0u; i < FB_W * FB_H; i++)
    {
        g_fb[i] = SENTINEL;
    }
}

static u32 fb_nonzero(u32 y0, u32 rows, u32 x0, u32 cols)
{
    u32 y, x, n = 0u;
    for (y = y0; y < y0 + rows; y++)
    {
        for (x = x0; x < x0 + cols; x++)
        {
            if (g_fb[y * FB_W + x] != SENTINEL)
            {
                n++;
            }
        }
    }
    return n;
}

int main(int argc, char **argv)
{
    const char *scenario = (argc > 1) ? argv[1] : "ia4_repeat_fill";
    s32 rc;

    gSCManagerSceneData.scene_curr = 0u;
    if (strcmp(scenario, "ia4_repeat_fill") == 0)
    {
        /* 136x29 IA4 middle through the real blitter. */
        u32 c;
        fixture_patterned();
        sobj_middle_tab(G_IM_FMT_IA, SP_FASTCOPY, 1.0F, 1.0F);
        CHECK(ndsSObjPreviewBasicSupported(&g_sobj) != FALSE);
        fb_fill();
        rc = ndsDrawSObjIntoPreview(&g_sobj, 0u, g_fb, FB_W, 160u, 32u,
                                    0, 0, 0u, 0u);
        CHECK(rc != FALSE);
        /* 8 opaque texels appear 9 times per row, 8 transparent 8 times. */
        CHECK(fb_nonzero(0u, 29u, 0u, 136u) == 72u * 29u);
        CHECK(g_fb[0] != SENTINEL);
        CHECK(g_fb[7] != SENTINEL);
        CHECK(g_fb[8] == SENTINEL);
        CHECK(g_fb[15] == SENTINEL);
        CHECK(g_fb[16] != SENTINEL);
        CHECK(g_fb[128] != SENTINEL);
        CHECK(g_fb[135] != SENTINEL);
        CHECK(g_fb[136] == SENTINEL);
        CHECK(g_fb[28u * FB_W + 135u] != SENTINEL);
        CHECK(g_fb[29u * FB_W] == SENTINEL);
        for (c = 0u; c < 120u; c++)
        {
            u16 a = g_fb[c];
            u16 b = g_fb[c + 16u];
            CHECK((a != SENTINEL) == (b != SENTINEL));
            if (a != SENTINEL)
            {
                CHECK(a == b);
            }
            CHECK(g_fb[5u * FB_W + c] == a);
        }
    }
    else if (strcmp(scenario, "ia4_alpha_zero") == 0)
    {
        /* All even nibbles: no alpha anywhere, nothing drawn. */
        fixture_uniform(0xEEu);
        sobj_middle_tab(G_IM_FMT_IA, SP_FASTCOPY, 1.0F, 1.0F);
        fb_fill();
        rc = ndsDrawSObjIntoPreview(&g_sobj, 0u, g_fb, FB_W, 160u, 32u,
                                    0, 0, 0u, 0u);
        CHECK(rc == FALSE);
        /* Admitted but fully transparent: the drawn_pixels==0 tail trips,
         * not the format gate (which would record UNSUPPORTED_FORMAT). */
        CHECK(g_test_blocker == NDS_STARTUP_LOGO_BLOCKER_BAD_BITMAP_BUFFER);
        CHECK(fb_nonzero(0u, 32u, 0u, 160u) == 0u);
    }
    else if (strcmp(scenario, "ia4_clip") == 0)
    {
        /* Negative origin through the real clipping path: source columns
         * 5..20 land on dst 0..15, so dst 0..2 fill, 3..10 stay empty,
         * 11..15 fill; column 16 is never written. */
        fixture_patterned();
        sobj_middle_tab(G_IM_FMT_IA, SP_FASTCOPY, 1.0F, 1.0F);
        fb_fill();
        rc = ndsDrawSObjIntoPreview(&g_sobj, 0u, g_fb, FB_W, 16u, 32u,
                                    -5, 0, 0u, 0u);
        CHECK(rc != FALSE);
        CHECK(g_fb[0] != SENTINEL);
        CHECK(g_fb[2] != SENTINEL);
        CHECK(g_fb[3] == SENTINEL);
        CHECK(g_fb[10] == SENTINEL);
        CHECK(g_fb[11] != SENTINEL);
        CHECK(g_fb[15] != SENTINEL);
        CHECK(g_fb[16] == SENTINEL);
        CHECK(g_fb[5u * FB_W + 0u] != SENTINEL);
        CHECK(g_fb[5u * FB_W + 3u] == SENTINEL);
        CHECK(fb_nonzero(0u, 29u, 0u, 16u) == 8u * 29u);
    }
    else if (strcmp(scenario, "ia4_texshuf") == 0)
    {
        /* Odd rows sample `source_x ^ 8`: row 1 swaps the halves. */
        fixture_patterned();
        sobj_middle_tab(G_IM_FMT_IA, SP_FASTCOPY | SP_TEXSHUF, 1.0F, 1.0F);
        fb_fill();
        rc = ndsDrawSObjIntoPreview(&g_sobj, 0u, g_fb, FB_W, 160u, 32u,
                                    0, 0, 0u, 0u);
        CHECK(rc != FALSE);
        CHECK(g_fb[0] != SENTINEL);
        CHECK(g_fb[8] == SENTINEL);
        CHECK(g_fb[FB_W + 0u] == SENTINEL);
        CHECK(g_fb[FB_W + 8u] != SENTINEL);
        CHECK(g_fb[FB_W + 15u] != SENTINEL);
        CHECK(g_fb[FB_W + 16u] == SENTINEL);
    }
    else if (strcmp(scenario, "ia4_scaled") == 0)
    {
        /* 2x scale fills 2-pixel rects per source column. */
        fixture_patterned();
        sobj_middle_tab(G_IM_FMT_IA, 0u, 2.0F, 2.0F);
        fb_fill();
        rc = ndsDrawSObjIntoPreview(&g_sobj, 0u, g_fb, FB_W, 300u, 64u,
                                    0, 0, 0u, 0u);
        CHECK(rc != FALSE);
        CHECK(g_fb[0] != SENTINEL);
        CHECK(g_fb[1] != SENTINEL);
        CHECK(g_fb[16] == SENTINEL);
        CHECK(g_fb[17] == SENTINEL);
        CHECK(g_fb[32] != SENTINEL);
        CHECK(g_fb[272] == SENTINEL);
        CHECK(g_fb[58u * FB_W] == SENTINEL);
        CHECK(fb_nonzero(0u, 58u, 0u, 272u) == 72u * 2u * 58u);
    }
    else if (strcmp(scenario, "badfmt") == 0)
    {
        /* Format validation still rejects what it does not sample. */
        fixture_patterned();
        sobj_middle_tab(G_IM_FMT_YUV, SP_FASTCOPY, 1.0F, 1.0F);
        fb_fill();
        rc = ndsDrawSObjIntoPreview(&g_sobj, 0u, g_fb, FB_W, 160u, 32u,
                                    0, 0, 0u, 0u);
        CHECK(rc == FALSE);
        CHECK(g_test_blocker == NDS_STARTUP_LOGO_BLOCKER_UNSUPPORTED_FORMAT);
        CHECK(fb_nonzero(0u, 32u, 0u, 160u) == 0u);
    }
    else
    {
        fprintf(stderr, "unknown scenario %s\n", scenario);
        return 2;
    }
    printf("SOBJ_IA4_OK %s\n", scenario);
    return 0;
}
'''


def build_blitter_binary():
    compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc")
                     if shutil.which(c)), None)
    if compiler is None:
        raise unittest.SkipTest("Host C compiler required")
    SCRATCH.mkdir(parents=True, exist_ok=True)
    source = SCRATCH / "sobj_ia4_blit.c"
    binary = SCRATCH / "sobj_ia4_blit.exe"
    text = backend_text()
    program = HARNESS_TEMPLATE.replace(
        "__EXTRACTED_ADMISSION__", function(text, "ndsSObjPreviewBasicSupported")).replace(
        "__EXTRACTED_MAPPER__", function(text, "ndsSObjMapTexel")).replace(
        "__EXTRACTED_BLITTER__", function(text, "ndsDrawSObjIntoPreview"))
    assert "ndsDrawSObjIntoPreview" in program
    assert "ndsSObjMapTexel" in program
    source.write_text(program)
    build = subprocess.run(
        [compiler, "-std=c11", "-Wall", "-Wextra",
         "-Wno-unused-parameter", str(source), "-o", str(binary)],
        capture_output=True, text=True)
    if build.returncode != 0:
        raise AssertionError(f"blitter build failed:\n{build.stderr}")
    return binary


class SObjIA4ExecutionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.binary = build_blitter_binary()
        cls.scenarios = ("ia4_repeat_fill", "ia4_alpha_zero", "ia4_clip",
                         "ia4_texshuf", "ia4_scaled", "badfmt")

    def run_scenario(self, name):
        result = subprocess.run([str(self.binary), name],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0,
                         f"{name} failed: {result.stderr}")
        self.assertIn(f"SOBJ_IA4_OK {name}", result.stdout)

    def test_repeat_fill_with_partial_final_tile(self):
        self.run_scenario("ia4_repeat_fill")

    def test_transparent_alpha_draws_nothing(self):
        self.run_scenario("ia4_alpha_zero")

    def test_negative_origin_clipping(self):
        self.run_scenario("ia4_clip")

    def test_texshuf_odd_row_swap(self):
        self.run_scenario("ia4_texshuf")

    def test_scaled_rect_fill(self):
        self.run_scenario("ia4_scaled")

    def test_unsupported_format_rejected(self):
        self.run_scenario("badfmt")


if __name__ == "__main__":
    unittest.main()
