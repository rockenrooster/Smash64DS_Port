#!/usr/bin/env python3
"""Host-execute the REAL menu FILL sink from src/port/sprite_preview_backend.c.

Prior review was desk-check only. This test extracts the production bodies
verbatim (source_test_helpers) and runs them in a host harness against
SDK-shaped command words:

  * extracted, unmodified: ndsSpritePackRgb15, ndsMenuFillPackRgba5551,
    ndsMenuFillMapAxis, ndsMenuFillStagingEnsure, ndsMenuFillBlitRect,
    ndsMenuFillSinkFoldWord (all from src/port/sprite_preview_backend.c).
  * stubbed: ndsSObjPreviewBeginStagingLayer (same early-return discipline as
    production: no-op when a layer is already open, else opens 320x240 and
    clears), the frame/glyph staging pointers, gSYVideoResWidth/Height,
    gNdsFrameCounter, and the fill counters. No algorithm under test is
    re-implemented; oracle pixels are hardcoded goldens derived by hand from
    the SDK pack formulas (see below), not from the extracted code.

Reference chain (checked at test time, not trusted from comments):
  * decomp/BattleShip-main/decomp/include/PR/gbi.h: GPACK_RGBA5551,
    GPACK_FILL16, gDPFillRectangle / gsDPFillRectangle, gDPScisFillRectangle.
  * include/PR/gbi.h (port): G_SETFILLCOLOR/G_FILLRECT/G_CYC_FILL values and
    the gDPFillRectangle / gDPSetFillColor / gDPSetPrimColor / gDPSetCycleType
    word expansions the C word-builder mirrors.
  * decomp/BattleShip-main/libultraship/src/fast/interpreter.cpp:
    GfxDpSetFillColor channel split, GfxDpFillRectangle FILL-vs-1CYCLE color
    select (fill color under FILL, pipeline/prim color otherwise) and the
    inclusive [ulx,lrx]x[uly,lry] bounds the sink's <= loop implements.
    ScreenAdjust's symmetric 3px lines (159..161 about 160, 119..121 about
    120) pin inclusive over exclusive; the FILL-mode hardware +1 edge pixel
    is a same-color overdraw into the neighbouring border rect, sub-pixel
    after the 640->320 halve, so the sink's plain-inclusive loop stands.

Scope actually proven here: fold/drain-word handling (FoldWord IS the drain
body; DrainRuntime only walks cursor->head calling it), RGBA5551/BGR555
packing, prim-vs-FILL cycle select, inclusive bounds, 320/640 mapping,
negatives/odd/clipping, layer order (later rects overwrite), pkt-cursor
order across procs, zero/invalid words, and no-stale-wipe staging sharing.
NOT proven here: the scene gate (MENU flag / battle exclusion), EndFrame
static-textbox scheduling, and SObj ordering -- those need the scene table
and DL heads and are audited by trace in the test docstring only. No claim
is made about Results beyond the gate trace.
"""
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from source_test_helpers import function  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
BACKEND = (ROOT / "src/port/sprite_preview_backend.c").read_text(encoding="utf-8")
PORT_GBI = (ROOT / "include/PR/gbi.h").read_text(encoding="utf-8")
SDK_GBI = (ROOT / "decomp/BattleShip-main/decomp/include/PR/gbi.h").read_text(
    encoding="utf-8")
LUS = (ROOT / "decomp/BattleShip-main/libultraship/src/fast/interpreter.cpp"
       ).read_text(encoding="utf-8")


def pin(pattern, text, label):
    if not re.search(pattern, text, re.S):
        raise AssertionError(f"reference drifted, update test: {label}")
    return True


# Port header values the harness mirrors.
pin(r"#define\s+G_SETFILLCOLOR\s+0xf7u", PORT_GBI, "G_SETFILLCOLOR")
pin(r"#define\s+G_FILLRECT\s+0xf6u", PORT_GBI, "G_FILLRECT")
pin(r"#define\s+G_CYC_FILL\s+0x00300000u", PORT_GBI, "G_CYC_FILL")
pin(r"0xe3000a01u.*\(u32\)\(type\)", PORT_GBI, "gsDPSetCycleType words")
pin(r"\(0xf6u\s*<<\s*24\)", PORT_GBI, "gDPFillRectangle opcode")
pin(r"\(0xf7u\s*<<\s*24\)", PORT_GBI, "gDPSetFillColor opcode")
pin(r"\(0xfau\s*<<\s*24\)", PORT_GBI, "gDPSetPrimColor opcode")
# SDK reference formulas the word-builder mirrors.
pin(r"#define\s+GPACK_RGBA5551\(r,\s*g,\s*b,\s*a\)", SDK_GBI, "GPACK_RGBA5551")
pin(r"#define\s+GPACK_FILL16\(w\)", SDK_GBI, "GPACK_FILL16")
pin(r"_SHIFTL\(\(lrx\),\s*14,\s*10\)", SDK_GBI, "gDPFillRectangle lrx field")
pin(r"_SHIFTL\(\(ulx\),\s*14,\s*10\)", SDK_GBI, "gDPFillRectangle ulx field")
# libultraship reference behavior the sink implements.
pin(r"col16\s*>>\s*11", LUS, "GfxDpSetFillColor channel split")
pin(r"mode\s*==\s*G_CYC_FILL", LUS, "GfxDpFillRectangle FILL select")
pin(r"lrx\s*\+=\s*1\s*<<\s*2", LUS, "FILL/COPY edge extension")
# Production decode literals the fold must keep.
pin(r"0xe3000a01u", BACKEND, "fold cycle word")
pin(r"\(w1\s*>>\s*14\)\s*&\s*0x3ffu", BACKEND, "fold ulx field")
pin(r"\(w0\s*>>\s*2\)\s*&\s*0x3ffu", BACKEND, "fold lry field")


def extract_real():
    out = {
        "sprite_pack": function(BACKEND, "ndsSpritePackRgb15"),
        "pack": function(BACKEND, "ndsMenuFillPackRgba5551"),
        "map": function(BACKEND, "ndsMenuFillMapAxis"),
        "staging": function(BACKEND, "ndsMenuFillStagingEnsure"),
        "blit": function(BACKEND, "ndsMenuFillBlitRect"),
        "fold": function(BACKEND, "ndsMenuFillSinkFoldWord"),
    }
    if "sNdsMenuFillCycleIsFill" not in out["fold"]:
        raise AssertionError("extracted FoldWord lost cycle state")
    if "ndsMenuFillBlitRect" not in out["fold"]:
        raise AssertionError("extracted FoldWord lost blit call")
    if "ndsMenuFillStagingEnsure" not in out["blit"]:
        raise AssertionError("extracted BlitRect lost staging call")
    return out


HARNESS_HEAD = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;

typedef union { struct { u32 w0; u32 w1; } words; long long int align; } Gfx;

#define G_CYC_FILL 0x00300000u
#define G_CYC_1CYCLE 0u
#define G_SETFILLCOLOR 0xf7u
#define G_FILLRECT 0xf6u
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Production-shaped globals the extracted bodies read/write. */
static u16 sNdsMenuFillColor = 0u;
static u32 sNdsMenuFillPrim = 0u;
static u32 sNdsMenuFillCycleIsFill = 0u;
static u16 *sNdsSObjFramePreview = NULL;
static u32 sNdsSObjFramePreviewPitch = 0u;
static u32 sNdsSObjFramePreviewDrawCount = 0u;
static u16 *sNdsStaffrollPreview = NULL;
static u32 sNdsStaffrollPreviewPitch = 0u;
static u32 sNdsStaffrollPreviewFrame = 0xffffffffu;
volatile u32 gNdsFrameCounter = 0u;
volatile u32 gNdsMenuFillRectCount = 0u;
volatile u32 gNdsMenuFillPixelCount = 0u;
s32 gSYVideoResWidth = 320;
s32 gSYVideoResHeight = 240;

static u16 sFrame[320u * 240u];
static u16 sGlyph[320u * 240u];
static u32 sBeginCalls = 0u;

/* Same discipline as production ndsSObjPreviewBeginStagingLayer: adopt when
 * open, else open 320x240 cleared exactly once. */
static void ndsSObjPreviewBeginStagingLayer(void)
{
    if (sNdsSObjFramePreview != NULL) { return; }
    sBeginCalls++;
    sNdsSObjFramePreview = sFrame;
    sNdsSObjFramePreviewPitch = 320u;
    memset(sFrame, 0, sizeof(sFrame));
}
'''

HOST_MAIN = r'''
static int sFailures = 0;
#define CHECK(cond, ...) do { \
    if (!(cond)) { printf("FAIL %d: ", __LINE__); printf(__VA_ARGS__); \
        printf("\n"); sFailures++; } \
} while (0)

/* SDK word builders: same expansions as port gbi.h macros (pinned above). */
static Gfx W_CYCLE(u32 type)
{ Gfx g; g.words.w0 = 0xe3000a01u; g.words.w1 = type; return g; }
static Gfx W_FILLCOLOR(u32 c)
{ Gfx g; g.words.w0 = (0xf7u << 24); g.words.w1 = c; return g; }
static Gfx W_PRIM(u8 r, u8 g_, u8 b, u8 a)
{ Gfx g; g.words.w0 = (0xfau << 24); \
  g.words.w1 = ((u32)r << 24) | ((u32)g_ << 16) | ((u32)b << 8) | a; return g; }
static Gfx W_RECT(u32 ulx, u32 uly, u32 lrx, u32 lry)
{ Gfx g; \
  g.words.w0 = (0xf6u << 24) | ((lrx & 0x3ffu) << 14) | ((lry & 0x3ffu) << 2); \
  g.words.w1 = ((ulx & 0x3ffu) << 14) | ((uly & 0x3ffu) << 2); return g; }
static Gfx W_ZERO(void) { Gfx g; g.words.w0 = 0; g.words.w1 = 0; return g; }
/* SDK GPACK_RGBA5551 + GPACK_FILL16 (decomp gbi.h). */
static u16 SDK_PACK5551(u32 r, u32 g_, u32 b, u32 a)
{ return (u16)(((r << 8) & 0xf800u) | ((g_ << 3) & 0x7c0u) | \
               ((b >> 2) & 0x3eu) | (a & 1u)); }
static u32 SDK_FILL16(u16 w) { return ((u32)w << 16) | w; }

static void reset(u32 res_w, u32 res_h)
{
    gSYVideoResWidth = (s32)res_w; gSYVideoResHeight = (s32)res_h;
    sNdsMenuFillColor = 0u; sNdsMenuFillPrim = 0u;
    sNdsMenuFillCycleIsFill = 0u;
    sNdsSObjFramePreview = NULL; sNdsSObjFramePreviewPitch = 0u;
    sNdsSObjFramePreviewDrawCount = 0u;
    sNdsStaffrollPreview = NULL; sNdsStaffrollPreviewPitch = 0u;
    sNdsStaffrollPreviewFrame = 0xffffffffu;
    gNdsFrameCounter = 0u;
    gNdsMenuFillRectCount = 0u; gNdsMenuFillPixelCount = 0u;
    sBeginCalls = 0u;
    memset(sFrame, 0, sizeof(sFrame));
    memset(sGlyph, 0, sizeof(sGlyph));
}
static u16 px(u32 x, u32 y)
{ return sNdsSObjFramePreview[y * sNdsSObjFramePreviewPitch + x]; }

int main(void)
{
    /* A. RGBA5551 goldens (hand-derived from GPACK_RGBA5551 + BGR555 swap). */
    CHECK(ndsMenuFillPackRgba5551(SDK_PACK5551(0x42, 0x3A, 0x31, 1)) == 0x98e8u,
          "textbox fill pack");
    CHECK(ndsMenuFillPackRgba5551(SDK_PACK5551(0x80, 0x00, 0x00, 1)) == 0x8010u,
          "highlight fill pack");
    CHECK(ndsMenuFillPackRgba5551(SDK_PACK5551(0xFF, 0xFF, 0xFF, 0)) == 0xffffu,
          "alpha bit never gates (FILL replaces)");
    CHECK(ndsSpritePackRgb15(0xBF, 0xA4, 0x47) == 0xa297u, "screenadjust prim");
    CHECK(ndsSpritePackRgb15(0x8B, 0x8B, 0x8B) == 0xc631u, "guide prim gray");

    /* B. Axis mapping: identity, halved, odd truncation, res<=0, negative. */
    CHECK(ndsMenuFillMapAxis(159, 320, 320) == 159, "320 identity");
    CHECK(ndsMenuFillMapAxis(346, 640, 320) == 173, "640 halved");
    CHECK(ndsMenuFillMapAxis(584, 640, 320) == 292, "640 right edge");
    CHECK(ndsMenuFillMapAxis(35, 480, 240) == 17, "odd trunc .5 down");
    CHECK(ndsMenuFillMapAxis(347, 640, 320) == 173, "odd 640 trunc");
    CHECK(ndsMenuFillMapAxis(99, 0, 320) == 99, "res<=0 identity");
    CHECK(ndsMenuFillMapAxis(-5, 320, 320) == -5, "negative trunc, clip later");

    /* C. FILL-cycle fold at 640 (textbox left border 346,35,348,164). */
    reset(640, 480);
    { Gfx w0 = W_CYCLE(G_CYC_FILL);
      Gfx w1 = W_FILLCOLOR(SDK_FILL16(SDK_PACK5551(0x42, 0x3A, 0x31, 1)));
      Gfx w2 = W_RECT(346, 35, 348, 164);
      ndsMenuFillSinkFoldWord(w0.words.w0, w0.words.w1);
      ndsMenuFillSinkFoldWord(w1.words.w0, w1.words.w1);
      ndsMenuFillSinkFoldWord(w2.words.w0, w2.words.w1);
      CHECK(gNdsMenuFillRectCount == 1u, "fill rect counted");
      CHECK(gNdsMenuFillPixelCount == 132u, "2x66 halved pixels, got %u",
            gNdsMenuFillPixelCount);
      CHECK(px(173, 17) == 0x98e8u && px(174, 82) == 0x98e8u, "corners painted");
      CHECK(px(175, 17) == 0u && px(173, 83) == 0u, "bounds exclusive outside");
    }

    /* D. Prim-cycle fold at 320 (screenadjust vertical 159,0,161,254). */
    reset(320, 240);
    { Gfx w0 = W_CYCLE(G_CYC_1CYCLE);
      Gfx w1 = W_PRIM(0xBF, 0xA4, 0x47, 0xFF);
      Gfx w2 = W_RECT(159, 0, 161, 254);
      ndsMenuFillSinkFoldWord(w0.words.w0, w0.words.w1);
      ndsMenuFillSinkFoldWord(w1.words.w0, w1.words.w1);
      ndsMenuFillSinkFoldWord(w2.words.w0, w2.words.w1);
      CHECK(gNdsMenuFillPixelCount == 720u, "3x240 (254 clips to 239), got %u",
            gNdsMenuFillPixelCount);
      CHECK(px(159, 0) == 0xa297u && px(161, 239) == 0xa297u, "prim corners");
      CHECK(px(158, 0) == 0u, "left of line clear");
    }

    /* E. Inclusive single pixel + inverted rect draws nothing. */
    reset(320, 240);
    { Gfx w0 = W_CYCLE(G_CYC_FILL);
      Gfx w1 = W_FILLCOLOR(SDK_FILL16(SDK_PACK5551(0x80, 0, 0, 1)));
      ndsMenuFillSinkFoldWord(w0.words.w0, w0.words.w1);
      ndsMenuFillSinkFoldWord(w1.words.w0, w1.words.w1);
      { Gfx r = W_RECT(10, 10, 10, 10);
        ndsMenuFillSinkFoldWord(r.words.w0, r.words.w1); }
      CHECK(gNdsMenuFillPixelCount == 1u && px(10, 10) == 0x8010u,
            "ul==lr paints exactly one");
      { Gfx r = W_RECT(50, 50, 40, 40);
        ndsMenuFillSinkFoldWord(r.words.w0, r.words.w1); }
      CHECK(gNdsMenuFillRectCount == 1u, "inverted rect not counted");
    }

    /* F. Zero/invalid words: PipeSync, rendermode, combine, EndDL skip. */
    reset(320, 240);
    { Gfx z = W_ZERO(), rm, cb;
      rm.words.w0 = 0xe200001cu; rm.words.w1 = 0u;
      cb.words.w0 = (0xfcu << 24); cb.words.w1 = 0u;
      ndsMenuFillSinkFoldWord(z.words.w0, z.words.w1);
      ndsMenuFillSinkFoldWord(rm.words.w0, rm.words.w1);
      ndsMenuFillSinkFoldWord(cb.words.w0, cb.words.w1);
      CHECK(gNdsMenuFillRectCount == 0u && gNdsMenuFillPixelCount == 0u &&
            sNdsSObjFramePreview == NULL, "stubs skip, no lazy staging");
    }

    /* G. Clipping: off-screen tail + guide rect at 320. */
    reset(640, 480);
    { Gfx w0 = W_CYCLE(G_CYC_FILL);
      Gfx w1 = W_FILLCOLOR(SDK_FILL16(SDK_PACK5551(0x80, 0, 0, 1)));
      Gfx r = W_RECT(600, 400, 700, 500);
      ndsMenuFillSinkFoldWord(w0.words.w0, w0.words.w1);
      ndsMenuFillSinkFoldWord(w1.words.w0, w1.words.w1);
      ndsMenuFillSinkFoldWord(r.words.w0, r.words.w1);
      CHECK(gNdsMenuFillPixelCount == 800u, "clipped 20x40, got %u",
            gNdsMenuFillPixelCount);
      CHECK(px(319, 239) == 0x8010u && px(300, 200) == 0x8010u, "clip corners");
    }
    reset(320, 240);
    { Gfx w0 = W_CYCLE(G_CYC_1CYCLE);
      Gfx w1 = W_PRIM(0x8B, 0x8B, 0x8B, 0xFF);
      Gfx r = W_RECT(44, 44, 45, 196);
      ndsMenuFillSinkFoldWord(w0.words.w0, w0.words.w1);
      ndsMenuFillSinkFoldWord(w1.words.w0, w1.words.w1);
      ndsMenuFillSinkFoldWord(r.words.w0, r.words.w1);
      CHECK(gNdsMenuFillPixelCount == 306u, "2x153 guide, got %u",
            gNdsMenuFillPixelCount);
      CHECK(px(44, 44) == 0xc631u && px(45, 196) == 0xc631u, "guide corners");
    }

    /* H. Order: later rect overwrites earlier (display order = paint order). */
    reset(320, 240);
    { Gfx w0 = W_CYCLE(G_CYC_FILL);
      Gfx red = W_FILLCOLOR(SDK_FILL16(SDK_PACK5551(0x80, 0, 0, 1)));
      Gfx blu = W_FILLCOLOR(SDK_FILL16(SDK_PACK5551(0x00, 0x00, 0x80, 1)));
      Gfx r1 = W_RECT(0, 0, 10, 10), r2 = W_RECT(5, 5, 15, 15);
      ndsMenuFillSinkFoldWord(w0.words.w0, w0.words.w1);
      ndsMenuFillSinkFoldWord(red.words.w0, red.words.w1);
      ndsMenuFillSinkFoldWord(r1.words.w0, r1.words.w1);
      ndsMenuFillSinkFoldWord(blu.words.w0, blu.words.w1);
      ndsMenuFillSinkFoldWord(r2.words.w0, r2.words.w1);
      CHECK(px(2, 2) == 0x8010u, "first rect survives outside overlap");
      CHECK(px(7, 7) == ndsMenuFillPackRgba5551(SDK_PACK5551(0, 0, 0x80, 1)),
            "overlap takes later color");
      CHECK(gNdsMenuFillRectCount == 2u, "both rects counted");
    }

    /* I. No stale wipe: glyph pixels survive fills; one Begin per frame. */
    reset(320, 240);
    { Gfx w0 = W_CYCLE(G_CYC_FILL);
      Gfx w1 = W_FILLCOLOR(SDK_FILL16(SDK_PACK5551(0x80, 0, 0, 1)));
      Gfx r = W_RECT(0, 0, 5, 5);
      ndsMenuFillSinkFoldWord(w0.words.w0, w0.words.w1);
      ndsMenuFillSinkFoldWord(w1.words.w0, w1.words.w1);
      ndsMenuFillSinkFoldWord(r.words.w0, r.words.w1); /* opens staging once */
      sNdsSObjFramePreview[100 * 320u + 100] = 0x1234u; /* fake prior glyph */
      ndsMenuFillSinkFoldWord(r.words.w0, r.words.w1);
      CHECK(sBeginCalls == 1u, "single lazy Begin, got %u", sBeginCalls);
      CHECK(sNdsSObjFramePreview[100 * 320u + 100] == 0x1234u,
            "fill does not wipe outside rect");
    }
    /* I2. Glyph-first sharing: adopt open glyph layer, no fresh Begin. */
    reset(320, 240);
    { Gfx w0 = W_CYCLE(G_CYC_FILL);
      Gfx w1 = W_FILLCOLOR(SDK_FILL16(SDK_PACK5551(0x80, 0, 0, 1)));
      Gfx r = W_RECT(0, 0, 2, 2);
      memset(sGlyph, 0, sizeof(sGlyph));
      sGlyph[200 * 320u + 200] = 0x5678u;
      sNdsStaffrollPreview = sGlyph; sNdsStaffrollPreviewPitch = 320u;
      sNdsStaffrollPreviewFrame = gNdsFrameCounter;
      ndsMenuFillSinkFoldWord(w0.words.w0, w0.words.w1);
      ndsMenuFillSinkFoldWord(w1.words.w0, w1.words.w1);
      ndsMenuFillSinkFoldWord(r.words.w0, r.words.w1);
      CHECK(sBeginCalls == 0u, "adopted glyph layer, got %u", sBeginCalls);
      CHECK(sNdsSObjFramePreview == sGlyph, "shared scratch adopted");
      CHECK(sNdsSObjFramePreview[200 * 320u + 200] == 0x5678u,
            "glyph pixel survives fill");
    }

    /* J. Static textbox DL: sync/cycle/rendermode/fillcolor/4 rects/end. */
    reset(640, 480);
    { Gfx dl[9]; int i;
      u16 fill = SDK_PACK5551(0x42, 0x3A, 0x31, 1);
      dl[0] = W_ZERO();
      dl[1] = W_CYCLE(G_CYC_FILL);
      dl[2].words.w0 = 0xe200001cu; dl[2].words.w1 = 0u;
      dl[3] = W_FILLCOLOR(SDK_FILL16(fill));
      dl[4] = W_RECT(346, 35, 348, 164);
      dl[5] = W_RECT(346, 35, 584, 37);
      dl[6] = W_RECT(582, 35, 584, 164);
      dl[7] = W_RECT(346, 162, 584, 164);
      dl[8] = W_ZERO();
      for (i = 0; i < 9; i++)
          ndsMenuFillSinkFoldWord(dl[i].words.w0, dl[i].words.w1);
      CHECK(gNdsMenuFillRectCount == 4u, "static box paints 4, got %u",
            gNdsMenuFillRectCount);
      CHECK(px(173, 17) == 0x98e8u, "box top-left");
      CHECK(px(292, 82) == 0x98e8u, "box bottom-right");
      CHECK(px(230, 50) == 0u, "box interior clear");
    }

    /* K. Pkt-cursor order across procs: cycle persists, color rekeys. */
    reset(320, 240);
    { Gfx cyc = W_CYCLE(G_CYC_FILL);
      Gfx red = W_FILLCOLOR(SDK_FILL16(SDK_PACK5551(0x80, 0, 0, 1)));
      Gfx r1 = W_RECT(0, 0, 4, 4);
      Gfx blu = W_FILLCOLOR(SDK_FILL16(SDK_PACK5551(0, 0, 0x80, 1)));
      Gfx r2 = W_RECT(10, 10, 14, 14);
      Gfx *proc1[] = { &cyc, &red, &r1 };
      Gfx *proc2[] = { &blu, &r2 };
      int i;
      for (i = 0; i < 3; i++)
          ndsMenuFillSinkFoldWord(proc1[i]->words.w0, proc1[i]->words.w1);
      for (i = 0; i < 2; i++)
          ndsMenuFillSinkFoldWord(proc2[i]->words.w0, proc2[i]->words.w1);
      CHECK(px(0, 0) == 0x8010u && px(10, 10) ==
            ndsMenuFillPackRgba5551(SDK_PACK5551(0, 0, 0x80, 1)),
            "proc2 rekeys color under same cycle");
    }

    if (sFailures == 0) { printf("ALL PASS\n"); return 0; }
    printf("%d FAILURES\n", sFailures);
    return 1;
}
'''


def build_source():
    real = extract_real()
    return "\n".join([
        HARNESS_HEAD,
        real["sprite_pack"],
        real["pack"],
        real["map"],
        real["staging"],
        real["blit"],
        real["fold"],
        HOST_MAIN,
    ])


def bounded(text, limit=4000):
    text = (text or "").strip()
    if len(text) <= limit:
        return text
    return text[:limit] + "... [truncated]"


class MenuFillSinkTest(unittest.TestCase):
    MAX_LOG = 4000

    def compile_and_run(self, source_text, prefer=None):
        order = ("clang", "gcc", "cc") if prefer is None else (prefer,)
        compiler = next((shutil.which(c) for c in order
                         if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required (clang/gcc/cc)")
        version = subprocess.run([compiler, "--version"], capture_output=True)
        is_clang = b"clang" in version.stdout.lower()
        error_cap = ["-ferror-limit=8"] if is_clang else ["-fmax-errors=8"]
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "menu_fill_sink.c"
            program = Path(directory) / "menu_fill_sink.exe"
            source.write_text(source_text, encoding="utf-8")
            built = subprocess.run(
                [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                 *error_cap, str(source), "-o", str(program)],
                capture_output=True)
            self.assertEqual(
                built.returncode, 0,
                f"host build failed:\n{bounded(built.stderr.decode('utf-8', 'replace'), self.MAX_LOG)}")  # noqa: E501
            ran = subprocess.run([str(program)], capture_output=True, timeout=30)
            out = ran.stdout.decode("utf-8", "replace")
            err = ran.stderr.decode("utf-8", "replace")
            self.assertEqual(
                ran.returncode, 0,
                f"host run failed (stderr):\n{bounded(err, self.MAX_LOG)}\n"
                f"(stdout):\n{bounded(out, self.MAX_LOG)}")
            self.assertIn("ALL PASS", out,
                          f"missing ALL PASS:\n{bounded(out, self.MAX_LOG)}")

    def test_real_fold_and_blit(self):
        self.compile_and_run(build_source())

    def test_real_fold_and_blit_gcc(self):
        if shutil.which("gcc") is None:
            self.skipTest("gcc not installed")
        self.compile_and_run(build_source(), prefer="gcc")


if __name__ == "__main__":
    unittest.main()
