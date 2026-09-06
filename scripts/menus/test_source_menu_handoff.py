#!/usr/bin/env python3
"""Source-menu video handoff at the owning seam (P2-7 item 9).

This checks source-menu entry ownership of the two DS overlay layers.
The earlier attribution of the Options screenshot to a stale native plate
was not established: adding the clear alone left the screenshot unchanged.
The visible missing tabs required IA4 decoding and repeat geometry, covered
separately by test_sobj_repeat.py and the captured runtime route.

Mechanism, checked at test time against the actual sources rather than
trusted from comments:

* decomp mn/mnoption/mnoption.c mnOptionFuncStart builds every GObj with id
  0 and a default camera; the file names no wallpaper object anywhere.
* src/port/sprite_preview_backend.c ndsDrawLayeredSObjFrame classifies a
  GObj with id != nGCCommonKindWallpaper as foreground, which composites
  into BG3 (VSResults excepted via dl_link_id 26). A menu with no
  wallpaper-kind GObj therefore never overwrites BG2.
* The native shell entry (ndsMenuShellRun) clears both overlay layers
  before blitting its plate, and the native exit releases OAM through
  ndsUiKitExit (oamClear). The source-menu pump previously only enabled
  the overlay, so a native plate survived underneath.

Live fix, owning seam: src/port/taskman_seam_harness.c
ndsSeamRunSourceMenuScene clears both overlay layers once, before enabling
the overlay and drawing, when is_results == 0. Results (is_results != 0)
keeps its battle-to-results presentation handoff untouched, and every
source-to-source entry clears again, which is safe because each source
menu redraws its full frame every present.

These tests lock that contract two ways. The static tests pin the
reference chain above. The execution tests compile the REAL extracted pump body with
stub platform calls and drive it: one clear per layer per entry, none per
frame, source draws preserved, Results unchanged.

Scope actually proven here: entry-clear ordering and counts, per-frame
absence of clears, draw preservation, Results exemption, source-to-source
re-entry, and the Option-no-wallpaper / classifier / OAM-release chain.
NOT proven here: any ROM/emulator run (pending serialized visual probe by
Main: Option entry shows only source art over a black field, toggle flips
only the Stereo/Mono highlight, B returns to an intact native Mode
Select).
"""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]
HARNESS = ROOT / "src/port/taskman_seam_harness.c"
DECOMP_OPTION = (ROOT / "decomp/BattleShip-main/decomp/src/mn/mnoption"
                 / "mnoption.c")
CLASSIFIER = ROOT / "src/port/sprite_preview_backend.c"
UI_KIT = ROOT / "src/nds/nds_ui_kit.c"

PUMP_SIGNATURE = ("static u32 ndsSeamRunSourceMenuScene("
                  "struct SYTaskFunction *tfunc, u32 is_results)")


def pump_body():
    """The real pump body, extracted from the production TU."""
    return function(HARNESS.read_text(), "ndsSeamRunSourceMenuScene")


HOST_PREAMBLE = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t u32;
typedef int32_t s32;
#define TRUE 1
#define FALSE 0
/* Boundary configuration values the live fix ships under. */
#define NDS_HARNESS_FAST_LOGIC 0
#define NDS_P2_MENU_WALK 0
#define NDS_SEAM_CONTROLLER_PAIR 0
#define NDS_IMPORT_BATTLESHIP_VS_RESULTS 1
#define NDS_IMPORT_BATTLESHIP_AUDIO_BGM 0
#define NDS_SCENE_BOUNDARY_PASS 7u
#define nSYTaskmanStatusLoadScene 3u
#define nSYTaskmanStatusRunning 1u
struct SYTaskFunction
{
    void (*task_update)(struct SYTaskFunction *);
    void (*scene_draw)(void);
};
static u32 sSYTaskmanStatus;
static u32 dSYTaskmanUpdateCount, dSYTaskmanFrameCount;
static u32 gNdsRendererProfileFrameCount;
static u32 gNdsFtPoseEvalTick;
static u32 gNdsSceneBoundaryKind, gNdsSceneBoundaryResult;
static struct { u32 scene_curr; } gSCManagerSceneData;
/* -- recorders: every platform call the pump can make -- */
static u32 rec_clear_bg2, rec_clear_bg3;
static u32 rec_enable, rec_disable, rec_draw, rec_update, rec_record;
static char evlog[4096];
static u32 evn;
static void ev(char c)
{
    if (evn < sizeof(evlog) - 1)
    {
        evlog[evn++] = c;
    }
}
static void ndsPlatformClearOriginalSpriteOverlayLayer(s32 is_foreground)
{
    if (is_foreground != FALSE)
    {
        rec_clear_bg3++;
        ev('3');
    }
    else
    {
        rec_clear_bg2++;
        ev('2');
    }
}
static void ndsPlatformSetOriginalSpriteOverlayEnabled(s32 is_enabled)
{
    if (is_enabled != FALSE)
    {
        rec_enable++;
        ev('E');
    }
    else
    {
        rec_disable++;
        ev('D');
    }
}
static void ndsAudioBackendUpdate(void) {}
static u32 ndsPlatformReadInput(void) { return 0u; }
static void ndsTaskmanSampleGraphicsHeap(void) {}
static void syTaskmanResetGraphicsHeap(void) {}
static void func_80004AB0(void) {}
static void ndsSObjPreviewBeginFrame(void) {}
static void ndsSObjPreviewEndFrame(void) {}
static void ndsPlatformRenderDebugHud(void) {}
static void ndsPlatformEndFrame(void) {}
static void ndsFinishTaskmanRun(void) {}
static void ndsMNVSResultsRecordFrame(void) { rec_record++; ev('R'); }
static u32 g_frames_before_load;
static void script_update(struct SYTaskFunction *tfunc)
{
    (void)tfunc;
    rec_update++;
    if (rec_update >= g_frames_before_load)
    {
        sSYTaskmanStatus = nSYTaskmanStatusLoadScene;
    }
}
static void script_draw(void) { rec_draw++; ev('4'); }
static void reset_state(void)
{
    sSYTaskmanStatus = nSYTaskmanStatusRunning;
    rec_clear_bg2 = rec_clear_bg3 = 0u;
    rec_enable = rec_disable = 0u;
    rec_draw = rec_update = rec_record = 0u;
    gNdsFtPoseEvalTick = 0u;
    evn = 0u;
    evlog[0] = '\0';
}
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "line %d: %s | evlog=%s\n", __LINE__, #x, evlog); \
    return 1; } } while (0)
'''


def pump_program():
    body = pump_body()
    host_body = re.sub(
        r"static u32 ndsSeamRunSourceMenuScene\("
        r"struct SYTaskFunction \*tfunc, u32 is_results\)",
        "static u32 hostSeamRunSourceMenuScene("
        "struct SYTaskFunction *tfunc, u32 is_results)",
        body, count=1)
    assert "hostSeamRunSourceMenuScene" in host_body
    assert "ndsSeamRunSourceMenuScene" not in host_body
    return (HOST_PREAMBLE + "\n" + host_body + r'''
static u32 run_entry(u32 is_results, u32 frames)
{
    struct SYTaskFunction tfunc;
    tfunc.task_update = script_update;
    tfunc.scene_draw = script_draw;
    g_frames_before_load = frames;
    return hostSeamRunSourceMenuScene(&tfunc, is_results);
}
int main(int argc, char **argv)
{
    const char *scenario = (argc > 1) ? argv[1] : "generic";
    if (strcmp(scenario, "generic") == 0)
    {
        /* Native -> source entry, five frames then LoadScene. */
        reset_state();
        CHECK(run_entry(0u, 5u) != FALSE);
        CHECK(rec_clear_bg2 == 1u);
        CHECK(rec_clear_bg3 == 1u);
        CHECK(rec_update == 5u);
        CHECK(rec_draw == 5u); /* source draws preserved, one per update */
        CHECK(rec_record == 0u);
        CHECK(rec_enable == 1u);
        CHECK(rec_disable == 1u); /* LoadScene exit disables the overlay */
        /* Order: both clears, then enable, then draws, then disable. */
        CHECK(evlog[0] == '2' && evlog[1] == '3' && evlog[2] == 'E');
        CHECK(evlog[3] == '4' && evlog[7] == '4');
        CHECK(evlog[8] == 'D' && evlog[9] == '\0');
    }
    else if (strcmp(scenario, "results") == 0)
    {
        /* Battle -> Results entry: no clears, draws and Results arm kept. */
        reset_state();
        CHECK(run_entry(1u, 5u) != FALSE);
        CHECK(rec_clear_bg2 == 0u);
        CHECK(rec_clear_bg3 == 0u);
        CHECK(rec_update == 5u);
        CHECK(rec_draw == 5u);
        CHECK(rec_record == 5u); /* per-frame Results recorder preserved */
        CHECK(gNdsFtPoseEvalTick != 0u); /* pose evaluation tick preserved */
        CHECK(evlog[0] == 'E'); /* enable first: no clear precedes it */
    }
    else if (strcmp(scenario, "twice") == 0)
    {
        /* Source -> source: each entry clears exactly once per layer. */
        u32 first_bg2, first_bg3;
        reset_state();
        CHECK(run_entry(0u, 3u) != FALSE);
        first_bg2 = rec_clear_bg2;
        first_bg3 = rec_clear_bg3;
        CHECK(first_bg2 == 1u && first_bg3 == 1u);
        reset_state();
        CHECK(run_entry(0u, 3u) != FALSE);
        CHECK(rec_clear_bg2 == 1u && rec_clear_bg3 == 1u);
        CHECK(rec_draw == 3u);
    }
    else if (strcmp(scenario, "long") == 0)
    {
        /* Fifty frames: still exactly one clear per layer, never per frame. */
        reset_state();
        CHECK(run_entry(0u, 50u) != FALSE);
        CHECK(rec_clear_bg2 == 1u);
        CHECK(rec_clear_bg3 == 1u);
        CHECK(rec_update == 50u);
        CHECK(rec_draw == 50u);
    }
    else
    {
        fprintf(stderr, "unknown scenario %s\n", scenario);
        return 2;
    }
    printf("HO_HANDOFF_OK %s evlog=%s\n", scenario, evlog);
    return 0;
}
''')


class SourceMenuHandoffTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = next((shutil.which(c) for c in
                             ("clang", "gcc", "cc") if shutil.which(c)), None)
        cls.tmpdir = None
        cls.binary = None
        if cls.compiler is not None:
            cls.tmpdir = tempfile.TemporaryDirectory()
            directory = Path(cls.tmpdir.name)
            source = directory / "handoff_pump.c"
            cls.binary = directory / "handoff_pump.exe"
            source.write_text(pump_program())
            build = subprocess.run(
                [cls.compiler, "-std=c11", "-Wall", "-Wextra",
                 "-Wno-unused-variable", "-Wno-unused-parameter",
                 str(source), "-o", str(cls.binary)],
                capture_output=True, text=True)
            if build.returncode != 0:
                raise AssertionError(
                    f"pump build failed:\n{build.stderr}")

    @classmethod
    def tearDownClass(cls):
        if cls.tmpdir is not None:
            cls.tmpdir.cleanup()

    def run_scenario(self, name):
        self.assertIsNotNone(self.compiler, "Host C compiler required")
        result = subprocess.run([str(self.binary), name],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0,
                         f"{name} failed: {result.stderr}")
        self.assertIn(f"HO_HANDOFF_OK {name}", result.stdout)

    def test_pump_clears_once_per_layer_before_enable_never_per_frame(self):
        """Static shape of the live fix: the entry clear is gated on the
        generic-menu path, both layers fire exactly once before the overlay
        enable, and no clear sits inside the per-frame loop."""
        body = pump_body()
        self.assertIn("if (is_results == 0u)", body)
        self.assertEqual(body.count("ndsPlatformClearOriginal"
                                    "SpriteOverlayLayer"), 2)
        self.assertIn("ndsPlatformClearOriginalSpriteOverlayLayer(FALSE)",
                      body)
        self.assertIn("ndsPlatformClearOriginalSpriteOverlayLayer(TRUE)",
                      body)
        enable = body.index("ndsPlatformSetOriginalSpriteOverlay"
                            "Enabled(TRUE)")
        self.assertLess(body.index("ndsPlatformClearOriginalSpriteOverlay"
                                   "Layer(FALSE)"), enable)
        self.assertLess(body.index("ndsPlatformClearOriginalSpriteOverlay"
                                   "Layer(TRUE)"), enable)
        loop = body.index("while ((tfunc")
        self.assertLess(enable, loop)
        self.assertNotIn("ndsPlatformClearOriginalSpriteOverlayLayer",
                         body[loop:],
                         "no clear may run per frame")

    def test_results_path_unchanged(self):
        """The Results arm keeps its pose tick, recorder, draws, and the
        LoadScene overlay disable; the entry clear must not touch it."""
        body = pump_body()
        self.assertIn("gNdsFtPoseEvalTick = 1u;", body)
        self.assertIn("ndsMNVSResultsRecordFrame();", body)
        self.assertIn("tfunc->scene_draw();", body,
                      "source draws must be preserved")
        self.assertIn("ndsPlatformSetOriginalSpriteOverlayEnabled(FALSE);",
                      body)

    def test_pump_callers_pass_only_known_flags(self):
        """Every pump entry passes a literal Results/generic flag, so no
        caller can take an untested path."""
        text = HARNESS.read_text()
        calls = re.findall(r"ndsSeamRunSourceMenuScene\(tfunc, (\w+)\)", text)
        self.assertGreater(len(calls), 0)
        for call in calls:
            self.assertIn(call, ("0u", "1u"),
                          f"unexpected pump flag: {call}")

    def test_results_caller_keeps_battle_handoff(self):
        """The VSResults branch releases the fighter packets and passes the
        Results flag, which is the presentation handoff the fix preserves."""
        text = HARNESS.read_text()
        results = text.index("nSCKindVSResults")
        call = text.index("ndsSeamRunSourceMenuScene(tfunc, 1u)", results)
        self.assertLess(results, call)
        self.assertIn("ndsRendererFighterPacketRelease();",
                      text[results:call])

    def test_option_scene_has_no_wallpaper_gobj(self):
        """The defect mechanism, from the actual decomp source: Option
        builds every GObj with id 0 under a default camera and names no
        wallpaper object, so it can never overwrite BG2 by itself."""
        source = DECOMP_OPTION.read_text()
        start = source.index("void mnOptionFuncStart(void)")
        func_start = source[start:source.index("dMNOptionVideoSetup", start)]
        self.assertIn("gcMakeDefaultCameraGObj", func_start)
        self.assertIn("mnOptionInitVars();", func_start)
        ids = re.findall(r"gcMakeGObjSPAfter\((\d+),", source)
        self.assertGreater(len(ids), 0)
        self.assertTrue(all(gid == "0" for gid in ids),
                        f"non-zero Option GObj ids: {sorted(set(ids))}")
        self.assertNotIn("Wallpaper", source)

    def test_ds_classifier_sends_non_wallpaper_to_foreground(self):
        """The DS ownership rule the fix relies on: only wallpaper-kind
        SObjs reach the background layer; everything else (all of Option)
        composites into the foreground layer."""
        text = CLASSIFIER.read_text()
        self.assertIn("(gobj->id != nGCCommonKindWallpaper) ? TRUE : FALSE",
                      text)

    def test_native_exit_releases_oam(self):
        """Native OAM needs no entry-side handling: the kit exit clears
        every OAM id it owns, so the pump only owes the bitmap layers."""
        text = UI_KIT.read_text()
        body = function(text, "ndsUiKitExit")
        self.assertIn("oamClear", body)

    def test_generic_entry_clears_once_and_draws(self):
        self.run_scenario("generic")

    def test_results_entry_uncleared_with_arm_preserved(self):
        self.run_scenario("results")

    def test_source_to_source_entry_clears_again(self):
        self.run_scenario("twice")

    def test_long_entry_never_clears_per_frame(self):
        self.run_scenario("long")


if __name__ == "__main__":
    unittest.main()
