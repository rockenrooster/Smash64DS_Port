#!/usr/bin/env python3
"""Host-execute the REAL fade TUs (src/import/battleship_lbfade.c +
src/port/video_blackout.c).

Compiles both production files verbatim with stub sys/obj.h + sys/objman.h
(GObj pool, wiring recorders, eject counter) and a minimal sys/scheduler.h
(SYTaskVi forward only; neither TU dereferences it) and drives them:

  * black-caller proof (static): every decomp d*FadeColor feeding
    lbFadeMakeActor is {0,0,0,...} -- the scope the MASTER_BRIGHT path
    depends on. sc1PGameBossProcDisplayFadeColor/Alpha are a separate boss
    wallpaper proc pair, not lbFade callers, and are excluded by name.
  * wiring: lbFadeMakeActor registers lbFadeProcDisplay at the caller's link
    priority on all cameras plus a Func-kind update proc (source lbfade.c:77).
  * lifecycle: alpha ramp saturates over fade_length ticks; proceed + eject
    fire exactly at fade_length+2 (source lbfade.c:34-56).
  * latch: display publishes, peek is non-destructive, discard clears.
  * hardware path (REAL code, no re-implementation): display -> peeked level
    -> ndsLBFadePushHardwareFrame -> ndsVideoSetSourceFade latch ->
    ndsVideoResolveBrightnessValue register value, with blackout precedence
    and non-black scope refusal.
  * composition order (static): ndsPlatformEndFrame pushes the fade after all
    draws and before ndsVideoBlackoutCommit, in both the HW-triangle and the
    software-framebuffer arms.
  * ownership (static): video_blackout.c is the sole REG_MASTER_BRIGHT writer;
    no new BG layer is allocated (only BG2/BG3 bitmap overlays exist); the
    public header carries no proposal patch comment and no software blend API.

Reference chain (checked at test time, not trusted from comments):
  * decomp/.../src/lb/lbfade.c: float ramp, a==0 inversion, fade_length+2,
    rect 10,10,GS_SCREEN_*-10, G_RM_CLD_SURF src-over, gcEjectGObj.
  * decomp/.../src/sys/objtypes.h: GOBJ_PRIORITY_DEFAULT == S32_MIN.
  * decomp/.../src/sys/objdef.h: nGCProcessKindThread, nGCProcessKindFunc order.
  * decomp config.h: GS_SCREEN 320x240 (rect 10,10,310,230).

Scope actually proven here: black-caller scope, wiring, ramp endpoints +
midpoint, proceed/eject timing, latch discipline, level quantization goldens,
blackout precedence, register goldens, hook order, sole register ownership,
no new layer. NOT proven here: any ROM/emulator run (pending visual
acceptance on device: full-screen black fade tracks the source ramp within
one brightness step, outer 10 px overscan ring fades with the frame).
"""
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TU = ROOT / "src/import/battleship_lbfade.c"
BLACKOUT_TU = ROOT / "src/port/video_blackout.c"
HDR = ROOT / "include/lb/lbfade_ds.h"
NDS_VIDEO_HDR = ROOT / "include/nds/nds_video.h"
BACKEND = (ROOT / "src/port/sprite_preview_backend.c").read_text(encoding="utf-8")
PLATFORM = (ROOT / "src/nds/nds_platform.c").read_text(encoding="utf-8")
BLACKOUT_SRC = BLACKOUT_TU.read_text(encoding="utf-8")
SRC = (ROOT / "decomp/BattleShip-main/decomp/src/lb/lbfade.c").read_text(
    encoding="utf-8")
OBJTYPES = (ROOT / "decomp/BattleShip-main/decomp/src/sys/objtypes.h"
            ).read_text(encoding="utf-8")
OBJDEF = (ROOT / "decomp/BattleShip-main/decomp/src/sys/objdef.h").read_text(
    encoding="utf-8")
CONFIG = (ROOT / "decomp/BattleShip-main/decomp/include/config.h").read_text(
    encoding="utf-8")
PORT_OBJMAN = (ROOT / "include/sys/objman.h").read_text(encoding="utf-8")
TU_TEXT = TU.read_text(encoding="utf-8")
HDR_TEXT = HDR.read_text(encoding="utf-8")
NDS_VIDEO_TEXT = NDS_VIDEO_HDR.read_text(encoding="utf-8")


def pin(pattern, text, label):
    if not re.search(pattern, text, re.S):
        raise AssertionError(f"reference drifted, update test: {label}")
    return True


def pin_absent(pattern, text, label):
    if re.search(pattern, text, re.S):
        raise AssertionError(f"forbidden path present, update code: {label}")
    return True


# Source behavior the TU must keep.
pin(r"\(\(f32\)\s*sLBFadeAlphaCurrent\s*/\s*\(f32\)\s*sLBFadeAlphaMax\)\s*\*\s*255\.0F",  # noqa: E501
    SRC, "float ramp")
pin(r"if\s*\(sLBFadeColor\.a\s*==\s*0\)\s*\{\s*alpha\s*=\s*0xFF\s*-\s*alpha;",
    SRC, "fade-from inversion")
pin(r"sLBFadeLength\s*=\s*fade_length\s*\+\s*2;", SRC, "length+2")
pin(r"gDPFillRectangle\(.*?,\s*10,\s*10,\s*GS_SCREEN_WIDTH_DEFAULT\s*-\s*10,\s*GS_SCREEN_HEIGHT_DEFAULT\s*-\s*10\)",  # noqa: E501
    SRC, "source rect")
pin(r"G_RM_CLD_SURF", SRC, "src-over render mode")
pin(r"gcEjectGObj\(gobj\)", SRC, "eject")
pin(r"#define\s+GS_SCREEN_WIDTH_DEFAULT\s+320", CONFIG, "screen 320")
pin(r"#define\s+GS_SCREEN_HEIGHT_DEFAULT\s+240", CONFIG, "screen 240")
pin(r"#define\s+GOBJ_PRIORITY_DEFAULT\s+S32_MIN", OBJTYPES, "link priority")
pin(r"nGCProcessKindThread,\s*\n\s*nGCProcessKindFunc,", OBJTYPES + OBJDEF,
    "proc kind order")
# Stub signatures below must mirror the port prototypes.
pin(r"gcMakeGObjSPAfter\(u32 id, void \(\*func_run\)\(GObj\*\), u8 link, u32 priority\)",  # noqa: E501
    PORT_OBJMAN, "make prototype")
pin(r"func_80009F74\(GObj \*gobj, void \(\*proc_display\)\(GObj\*\), u32 priority, u64 arg3, u32 camera_tag\)",  # noqa: E501
    PORT_OBJMAN, "display prototype")
pin(r"gcAddGObjProcess\(GObj \*gobj, void \(\*proc\)\(GObj\*\), u8 kind, u32 pri\)",  # noqa: E501
    PORT_OBJMAN, "process prototype")
# Owned production code the hook depends on.
pin(r"\(f32\)current\s*/\s*\(f32\)max\)\s*\*\s*255\.0F", TU_TEXT, "TU ramp")
pin(r"fade_length\s*\+\s*2", TU_TEXT, "TU length+2")
pin(r"ndsLBFadePushHardwareFrame", TU_TEXT, "TU push hook")
pin(r"ndsLBFadeAlphaToHardwareLevel", TU_TEXT, "TU level map")
pin(r"ndsVideoSetSourceFade", TU_TEXT, "TU drives brightness latch")
pin(r"#define\s+NDS_LBFADE_RECT_ULX\s+10", HDR_TEXT, "rect ulx")
pin(r"#define\s+NDS_LBFADE_RECT_LRX\s+310", HDR_TEXT, "rect lrx")
pin(r"#define\s+NDS_LBFADE_RECT_LRY\s+230", HDR_TEXT, "rect lry")
pin(r"ndsVideoSetSourceFade", NDS_VIDEO_TEXT, "video fade latch decl")
pin(r"ndsVideoResolveBrightnessValue", NDS_VIDEO_TEXT, "video resolve decl")
pin(r"ndsVideoResolveBrightnessValue", BLACKOUT_SRC, "blackout resolve impl")
pin(r"REG_MASTER_BRIGHT_SUB", BLACKOUT_SRC, "blackout writes both screens")
# Deleted wrong paths must stay deleted: no staging blend anywhere, no
# consume-before-commit lifetime, no unapplied proposal patch in the header.
pin_absent(r"ndsLBFadeBlendPreviewRect", BACKEND, "backend staging blend")
pin_absent(r"ndsLBFadeBlendPreviewRect", TU_TEXT, "TU staging blend")
pin_absent(r"ndsLBFadeBlendPixel", TU_TEXT, "TU blend pixel")
pin_absent(r"ndsLBFadeConsumeFrame", TU_TEXT, "TU consume lifetime")
pin_absent(r"MAIN HARDWARE HOOK PATCH|\+void ndsPlatformUpdateFadeBlend|proposal, not applied",  # noqa: E501
           HDR_TEXT, "header proposal patch comment")
pin_absent(r"ndsLBFadeBlend", HDR_TEXT, "header blend API")
# Final hook order in the platform: one push after all draws, before the one
# register commit, in both EndFrame arms (hwtri + software framebuffer).
pin(r"ndsLBFadePushHardwareFrame\(\);\s*\n\s*ndsVideoBlackoutCommit\(\);",
    PLATFORM, "push-before-commit order")
if len(re.findall(r"ndsLBFadePushHardwareFrame\(\);", PLATFORM)) != 2:
    raise AssertionError("platform must push in both EndFrame arms")
pin(r'#include <lb/lbfade_ds\.h>', PLATFORM, "platform fade include")
# Backend keeps only the lifetime half: stale-latch discard at BeginFrame.
pin(r"ndsLBFadeDiscardFrame\(\);", BACKEND, "backend discards")


def assert_black_caller_scope():
    """Every decomp color feeding lbFadeMakeActor is {0,0,0,...}.

    The MASTER_BRIGHT path can only fade toward black, so this is the scope
    proof, read from the decomp sources at test time. The boss
    sc1PGameBossProcDisplayFadeColor grey ramp is a separate wallpaper proc
    (float step, not lbFade) and is excluded by construction: it matches
    neither the FadeColor definition shape nor any lbFadeMakeActor call.
    """
    decomp_src = ROOT / "decomp/BattleShip-main/decomp/src"
    defs = []
    for path in sorted(decomp_src.rglob("*.c")):
        text = path.read_text(encoding="utf-8")
        for match in re.finditer(
                r"SYColorRGBA\s+(\w*FadeColor)\s*=\s*\{\s*"
                r"(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)\s*,\s*"
                r"(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)\s*\};", text):
            defs.append((path, match))
    if len(defs) != 11:
        raise AssertionError(
            f"expected 11 decomp FadeColor defs, found {len(defs)}; "
            "a caller was added or removed, re-scope the hardware path")
    for path, match in defs:
        r, g, b = (int(match.group(i), 16) for i in (2, 3, 4))
        if (r, g, b) != (0, 0, 0):
            raise AssertionError(
                f"non-black fade caller {match.group(1)} in "
                f"{path}: MASTER_BRIGHT cannot express it")
    return True


def assert_sole_brightness_owner():
    """Only video_blackout.c writes REG_MASTER_BRIGHT (both screens)."""
    writers = []
    for base in (ROOT / "src", ROOT / "include"):
        for path in sorted(base.rglob("*.c")) + sorted(base.rglob("*.h")):
            try:
                text = path.read_text(encoding="utf-8")
            except UnicodeDecodeError:
                continue
            if re.search(r"REG_MASTER_BRIGHT(_SUB)?\s*=", text):
                writers.append(path.relative_to(ROOT).as_posix())
    if writers != ["src/port/video_blackout.c"]:
        raise AssertionError(
            f"MASTER_BRIGHT writers must be exactly video_blackout.c, "
            f"found: {writers}")
    return True


def assert_no_new_layer():
    """No BG layer beyond the two bitmap overlays (BG2/BG3); the fade needs
    none, so none is allocated."""
    if re.search(r"bgInit\s*\(\s*1\s*,", PLATFORM):
        raise AssertionError("fade must not allocate BG1")
    if len(re.findall(r"bgInit\s*\(", PLATFORM)) != 2:
        raise AssertionError("overlay layer count changed, re-prove VRAM use")
    return True


assert_black_caller_scope()
assert_sole_brightness_owner()
assert_no_new_layer()


STUB_OBJ_H = r'''
#ifndef TEST_SYS_OBJ_H
#define TEST_SYS_OBJ_H
#include <ssb_types.h>
#include <stddef.h>
typedef struct GObj { u32 id; u8 link; u32 link_priority; } GObj;
#define GOBJ_PRIORITY_DEFAULT ((s32)0x80000000)
#endif
'''

STUB_OBJMAN_H = r'''
#ifndef TEST_SYS_OBJMAN_H
#define TEST_SYS_OBJMAN_H
#include <sys/obj.h>
enum { nGCProcessKindThread, nGCProcessKindFunc };
GObj *gcMakeGObjSPAfter(u32 id, void (*func_run)(GObj *), u8 link,
                        u32 priority);
void func_80009F74(GObj *gobj, void (*proc_display)(GObj *), u32 priority,
                   u64 arg3, u32 camera_tag);
void *gcAddGObjProcess(GObj *gobj, void (*proc)(GObj *), u8 kind, u32 pri);
void gcEjectGObj(GObj *gobj);
#endif
'''

STUB_SCHEDULER_H = r'''
#ifndef TEST_SYS_SCHEDULER_H
#define TEST_SYS_SCHEDULER_H
/* Neither TU under test dereferences this; sys/video.h needs the name for
 * the syVideoApplySettingsNoBlock prototype only. */
typedef struct SYTaskVi SYTaskVi;
#endif
'''

STUB_C = r'''
#include <sys/objman.h>
static GObj sPool[8];
static u32 sNext;
GObj *tMakeGObj; u32 tMakeId; u8 tMakeLink; u32 tMakePrio;
GObj *tDispGObj; void (*tDispProc)(GObj *); u32 tDispPrio; u64 tDispArg;
u32 tDispCam; u32 tDispCalls;
GObj *tProcGObj; void (*tProcFunc)(GObj *); u8 tProcKind; u32 tProcPrio;
GObj *tEjected; u32 tEjectCalls;
GObj *gcMakeGObjSPAfter(u32 id, void (*func_run)(GObj *), u8 link,
                        u32 priority)
{
    (void)func_run;
    tMakeGObj = &sPool[sNext % 8u]; sNext++;
    tMakeGObj->id = id; tMakeGObj->link = link;
    tMakeGObj->link_priority = priority;
    tMakeId = id; tMakeLink = link; tMakePrio = priority;
    return tMakeGObj;
}
void func_80009F74(GObj *gobj, void (*proc_display)(GObj *), u32 priority,
                   u64 arg3, u32 camera_tag)
{
    tDispGObj = gobj; tDispProc = proc_display; tDispPrio = priority;
    tDispArg = arg3; tDispCam = camera_tag; tDispCalls++;
}
void *gcAddGObjProcess(GObj *gobj, void (*proc)(GObj *), u8 kind, u32 pri)
{
    tProcGObj = gobj; tProcFunc = proc; tProcKind = kind; tProcPrio = pri;
    return NULL;
}
void gcEjectGObj(GObj *gobj) { tEjected = gobj; tEjectCalls++; }
'''

HOST_MAIN = r'''
#include <stdio.h>
#include <string.h>
#include <sys/objman.h>
#include <lb/lbfade_ds.h>
#include <nds/nds_video.h>
#include <sys/video.h>

void lbFadeProcUpdate(GObj *gobj);
void lbFadeProcDisplay(GObj *gobj);
void lbFadeMakeActor(u32 id, s32 link, u32 link_priority, SYColorRGBA *color,
                     s32 fade_length, sb32 is_eject_gobj,
                     sb32 *is_proceed_scene);
extern GObj *tMakeGObj; extern u32 tMakeId; extern u8 tMakeLink;
extern u32 tMakePrio;
extern GObj *tDispGObj; extern void (*tDispProc)(GObj *);
extern u32 tDispPrio; extern u64 tDispArg; extern u32 tDispCam;
extern u32 tDispCalls;
extern GObj *tProcGObj; extern void (*tProcFunc)(GObj *);
extern u8 tProcKind; extern u32 tProcPrio;
extern GObj *tEjected; extern u32 tEjectCalls;

static int sFailures = 0;
#define CHECK(cond, ...) do { \
    if (!(cond)) { printf("FAIL %d: ", __LINE__); printf(__VA_ARGS__); \
        printf("\n"); sFailures++; } \
} while (0)

int main(void)
{
    SYColorRGBA from = { 0, 0, 0, 0x00 };
    SYColorRGBA to = { 0, 0, 0, 0xFF };
    SYColorRGBA red = { 0xFF, 0, 0, 0xFF };
    sb32 proceed = FALSE;
    u8 r, g, b, a;
    u32 level;
    u32 i;

    /* A. Wiring: Transition link 13, priority 10, all cameras, Func update. */
    lbFadeMakeActor(7u, 13, 10u, &from, 12, TRUE, &proceed);
    CHECK(gNdsLBFadeCreateCount == 1u, "create counted");
    CHECK(tMakeId == 7u && tMakeLink == 13u &&
          (s32)tMakePrio == (s32)0x80000000, "make link/priority");
    CHECK(tDispCalls == 1u && tDispProc == &lbFadeProcDisplay &&
          tDispGObj == tMakeGObj, "display proc wired");
    CHECK(tDispPrio == 10u && tDispArg == 0u && tDispCam == ~0u,
          "display prio/cameras");
    CHECK(tProcFunc == &lbFadeProcUpdate && tProcKind == 1u &&
          tProcPrio == 0u && tProcGObj == tMakeGObj, "func update wired");

    /* B. Fade-from ramp (base a==0): 255 -> 128 -> 0; ends at tick 12. */
    lbFadeProcDisplay(tMakeGObj);
    CHECK(ndsLBFadePeekFrame(&r, &g, &b, &a) == TRUE, "frame live");
    CHECK(a == 255u, "from t0 opaque, got %u", a);
    CHECK(r == 0u && g == 0u && b == 0u, "from color black");
    CHECK(ndsLBFadePeekHardwareLevel(&level) == TRUE && level == 16u,
          "from t0 level 16, got %u", level);
    ndsLBFadeDiscardFrame();
    CHECK(ndsLBFadePeekFrame(NULL, NULL, NULL, NULL) == FALSE, "discarded");
    for (i = 0u; i < 6u; i++) { lbFadeProcUpdate(tMakeGObj); }
    lbFadeProcDisplay(tMakeGObj);
    CHECK(ndsLBFadePeekFrame(&r, &g, &b, &a) == TRUE && a == 128u,
          "from t6 128, got %u", a);
    CHECK(ndsLBFadePeekHardwareLevel(&level) == TRUE && level == 8u,
          "from t6 level 8, got %u", level);
    for (i = 0u; i < 6u; i++) { lbFadeProcUpdate(tMakeGObj); }
    lbFadeProcDisplay(tMakeGObj);
    CHECK(ndsLBFadePeekFrame(&r, &g, &b, &a) == TRUE && a == 0u,
          "from t12 clear, got %u", a);
    CHECK(ndsLBFadePeekHardwareLevel(&level) == TRUE && level == 0u,
          "from t12 level 0, got %u", level);
    CHECK(proceed == FALSE && tEjectCalls == 0u, "no early proceed/eject");
    ndsLBFadeDiscardFrame();

    /* C. Proceed + eject fire exactly at fade_length+2 (tick 14). */
    lbFadeProcUpdate(tMakeGObj);
    CHECK(proceed == FALSE && tEjectCalls == 0u, "tick 13 quiet");
    lbFadeProcUpdate(tMakeGObj);
    CHECK(proceed == TRUE, "proceed at tick 14");
    CHECK(tEjectCalls == 1u && tEjected == tMakeGObj, "eject at tick 14");
    lbFadeProcUpdate(tMakeGObj);
    CHECK(tEjectCalls == 1u, "no double eject");

    /* D. Fade-to ramp (base a==0xFF), len 10: 0 -> 127 -> 255. */
    proceed = FALSE;
    lbFadeMakeActor(7u, 13, 10u, &to, 10, TRUE, &proceed);
    lbFadeProcDisplay(tMakeGObj);
    CHECK(ndsLBFadePeekFrame(&r, &g, &b, &a) == TRUE && a == 0u,
          "to t0 clear, got %u", a);
    CHECK(ndsLBFadePeekHardwareLevel(&level) == TRUE && level == 0u,
          "to t0 level 0");
    for (i = 0u; i < 5u; i++) { lbFadeProcUpdate(tMakeGObj); }
    lbFadeProcDisplay(tMakeGObj);
    CHECK(ndsLBFadePeekFrame(&r, &g, &b, &a) == TRUE && a == 127u,
          "to t5 127, got %u", a);
    CHECK(ndsLBFadePeekHardwareLevel(&level) == TRUE && level == 8u,
          "to t5 level 8, got %u", level);
    for (i = 0u; i < 5u; i++) { lbFadeProcUpdate(tMakeGObj); }
    lbFadeProcDisplay(tMakeGObj);
    CHECK(ndsLBFadePeekFrame(&r, &g, &b, &a) == TRUE && a == 255u,
          "to t10 solid, got %u", a);
    CHECK(ndsLBFadePeekHardwareLevel(&level) == TRUE && level == 16u,
          "to t10 level 16");
    ndsLBFadeDiscardFrame();

    /* E. Level quantization goldens: (alpha*16+127)/255, endpoints exact. */
    CHECK(ndsLBFadeAlphaToHardwareLevel(0u) == 0u, "level 0");
    CHECK(ndsLBFadeAlphaToHardwareLevel(1u) == 0u, "level alpha 1");
    CHECK(ndsLBFadeAlphaToHardwareLevel(8u) == 1u, "level alpha 8");
    CHECK(ndsLBFadeAlphaToHardwareLevel(16u) == 1u, "level alpha 16");
    CHECK(ndsLBFadeAlphaToHardwareLevel(127u) == 8u, "level alpha 127");
    CHECK(ndsLBFadeAlphaToHardwareLevel(128u) == 8u, "level alpha 128");
    CHECK(ndsLBFadeAlphaToHardwareLevel(255u) == 16u, "level 16");

    /* F. Non-black scope refusal: hue the register cannot express draws no
     * fade rather than a wrong one. */
    lbFadeMakeActor(7u, 13, 10u, &red, 10, TRUE, NULL);
    lbFadeProcDisplay(tMakeGObj);
    level = 99u;
    CHECK(ndsLBFadePeekHardwareLevel(&level) == FALSE, "non-black refused");
    CHECK(level == 0u, "refused level 0, got %u", level);
    ndsLBFadeDiscardFrame();

    /* G. Final push path (REAL setter + REAL latch + REAL resolve):
     * fade-only frames fade with no staging commit involved. */
    ndsVideoSetBlackout(FALSE);
    lbFadeMakeActor(7u, 13, 10u, &from, 12, TRUE, NULL);
    lbFadeProcDisplay(tMakeGObj);
    ndsLBFadePushHardwareFrame();
    CHECK(ndsVideoGetSourceFade() == 16u, "pushed 16, got %u",
          ndsVideoGetSourceFade());
    CHECK(ndsVideoResolveBrightnessValue(ndsVideoGetBlackout(),
          ndsVideoGetSourceFade()) == (u16)((2u << 14) | 16u),
          "register full black 0x%04x",
          ndsVideoResolveBrightnessValue(0u, 16u));
    for (i = 0u; i < 6u; i++) { lbFadeProcUpdate(tMakeGObj); }
    lbFadeProcDisplay(tMakeGObj);
    ndsLBFadePushHardwareFrame();
    CHECK(ndsVideoGetSourceFade() == 8u, "pushed 8, got %u",
          ndsVideoGetSourceFade());
    CHECK(ndsVideoResolveBrightnessValue(0u, 8u) == (u16)((2u << 14) | 8u),
          "register mid 0x%04x", ndsVideoResolveBrightnessValue(0u, 8u));
    /* Blackout precedence: full black wins, fade recovers after. */
    ndsVideoSetBlackout(TRUE);
    CHECK(ndsVideoResolveBrightnessValue(ndsVideoGetBlackout(),
          ndsVideoGetSourceFade()) == (u16)((2u << 14) | 16u),
          "blackout wins");
    ndsVideoSetBlackout(FALSE);
    CHECK(ndsVideoResolveBrightnessValue(ndsVideoGetBlackout(),
          ndsVideoGetSourceFade()) == (u16)((2u << 14) | 8u),
          "fade recovers");
    /* Native shell frames have no SObj discard. The previous push consumed
     * its source frame, so the next push alone must restore visibility. */
    ndsLBFadePushHardwareFrame();
    CHECK(ndsVideoGetSourceFade() == 0u, "inactive pushes 0");
    CHECK(ndsVideoResolveBrightnessValue(0u, 0u) == (u16)0u, "clear is 0");
    CHECK(ndsVideoResolveBrightnessValue(1u, 0u) == (u16)((2u << 14) | 16u),
          "blackout alone is full");
    CHECK(ndsVideoResolveBrightnessValue(0u, 99u) == (u16)((2u << 14) | 16u),
          "resolve clamps");

    if (sFailures == 0) { printf("ALL PASS\n"); return 0; }
    printf("%d FAILURES\n", sFailures);
    return 1;
}
'''


def bounded(text, limit=4000):
    text = (text or "").strip()
    if len(text) <= limit:
        return text
    return text[:limit] + "... [truncated]"


class SourceFadeTest(unittest.TestCase):
    MAX_LOG = 4000

    def compile_and_run(self, prefer=None):
        order = ("clang", "gcc", "cc") if prefer is None else (prefer,)
        compiler = next((shutil.which(c) for c in order
                         if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required (clang/gcc/cc)")
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            sysdir = d / "stub" / "sys"
            sysdir.mkdir(parents=True)
            (sysdir / "obj.h").write_text(STUB_OBJ_H, encoding="utf-8")
            (sysdir / "objman.h").write_text(STUB_OBJMAN_H, encoding="utf-8")
            (sysdir / "scheduler.h").write_text(STUB_SCHEDULER_H,
                                                encoding="utf-8")
            (d / "stub.c").write_text(STUB_C, encoding="utf-8")
            (d / "hostmain.c").write_text(HOST_MAIN, encoding="utf-8")
            program = d / "source_fade.exe"
            built = subprocess.run(
                [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                 "-I", str(d / "stub"), "-I", str(ROOT / "include"),
                 str(TU), str(BLACKOUT_TU), str(d / "stub.c"),
                 str(d / "hostmain.c"), "-o", str(program)],
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

    def test_real_fade_tus(self):
        self.compile_and_run()

    def test_real_fade_tus_gcc(self):
        if shutil.which("gcc") is None:
            self.skipTest("gcc not installed")
        self.compile_and_run(prefer="gcc")


if __name__ == "__main__":
    unittest.main()
