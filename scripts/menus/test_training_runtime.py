"""Training-mode source correctness: menu speed cadence and port integration.

Proves, against the read-only BattleShip originals, the behavior the two
P2-7 item 3 wrapper TUs import:
  - src/import/battleship_sc1ptrainingmode.c (sc/sc1pmode/sc1ptrainingmode.c)
  - src/import/battleship_mntraining.c (mn/mnplayers/mnplayers1ptraining.c)

Host part: compiles the original sc1PTrainingModeCheckLagTic plus its lag
table verbatim and checks the run/skip cadence every speed setting must
produce (Full = every tick, 2Thirds = 2 run / 1 skip, Half = alternate,
Quarter = 1 run / 3 skip), and that the CP menu order maps onto the dummy
behavior table in order.
Port part: text-level pins that the wrappers are whole-TU textual includes
with pass-through adapters, carry no duplicate shims (port headers own the
enums/structs/audio IDs), the title_backend stubs yield under NDS_P2_1P_GAME,
the harness routes both training scenes, and the wallpaper/sprite reloc rows
exist.
"""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from source_test_helpers import braced, function, original_enum

ROOT = Path(__file__).resolve().parents[2]
DECOMP = ROOT / "decomp/BattleShip-main/decomp/src"
TRAINING = (DECOMP / "sc/sc1pmode/sc1ptrainingmode.c").read_text()


def host_source():
    lag = braced(
        TRAINING,
        r"u8 dSC1PTrainingModeLagIntervals\s*\[\s*\]\s*\[2\]",
        True,
    )
    check = function(TRAINING, "sc1PTrainingModeCheckLagTic")
    dummy = braced(
        TRAINING,
        r"s32 dSC1PTrainingModeDummyBehaviors\s*\[\s*\]",
        True,
    )
    speed = original_enum("sc/scdef.h", "SC1PTrainingModeSpeed")
    cp = original_enum("sc/scdef.h", "SC1PTrainingModeCP")
    behavior = braced(
        (DECOMP / "ft/ftdef.h").read_text(),
        r"typedef enum FTComputerBehaviorKind\s*\{",
        True,
    )
    return r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define FALSE 0
#define TRUE 1
typedef int32_t s32;
typedef uint8_t u8;
typedef int sb32;
''' + speed + "\n" + cp + "\n" + behavior + "\n" + lag + "\n" + dummy + r'''
static struct { s32 speed_menu_option; u8 lagtic_wait; u8 frameadvance_wait; } sSC1PTrainingModeMenu;
''' + check + r'''
/* Returns FALSE when the frame runs, TRUE when it is a lag/skip tick. */
static int run_ticks(s32 speed, int ticks)
{
    int runs = 0;
    sSC1PTrainingModeMenu.speed_menu_option = speed;
    sSC1PTrainingModeMenu.lagtic_wait = sSC1PTrainingModeMenu.frameadvance_wait = 0;
    for (int i = 0; i < ticks; i++) {
        if (!sc1PTrainingModeCheckLagTic()) runs++;
    }
    return runs;
}
int main(void)
{
    CHECK(nSC1PTrainingModeMenuSpeedFull == 0);
    CHECK(nSC1PTrainingModeMenuSpeed2Thirds == 1);
    CHECK(nSC1PTrainingModeMenuSpeedHalf == 2);
    CHECK(nSC1PTrainingModeMenuSpeedQuarter == 3);
    CHECK(nSC1PTrainingModeMenuSpeedEnumCount == 4);
    CHECK(nSC1PTrainingModeMenuCPStand == 0);
    CHECK(nSC1PTrainingModeMenuCPAttack == 4);
    CHECK(nSC1PTrainingModeMenuCPEnumCount == 5);
    CHECK(dSC1PTrainingModeDummyBehaviors[nSC1PTrainingModeMenuCPStand] == nFTComputerBehaviorStand);
    CHECK(dSC1PTrainingModeDummyBehaviors[nSC1PTrainingModeMenuCPWalk] == nFTComputerBehaviorWalk);
    CHECK(dSC1PTrainingModeDummyBehaviors[nSC1PTrainingModeMenuCPEvade] == nFTComputerBehaviorEvade);
    CHECK(dSC1PTrainingModeDummyBehaviors[nSC1PTrainingModeMenuCPJump] == nFTComputerBehaviorJump);
    CHECK(dSC1PTrainingModeDummyBehaviors[nSC1PTrainingModeMenuCPAttack] == nFTComputerBehaviorDefault);
    CHECK(run_ticks(nSC1PTrainingModeMenuSpeedFull, 12) == 12);
    CHECK(run_ticks(nSC1PTrainingModeMenuSpeed2Thirds, 12) == 8);
    CHECK(run_ticks(nSC1PTrainingModeMenuSpeedHalf, 12) == 6);
    CHECK(run_ticks(nSC1PTrainingModeMenuSpeedQuarter, 12) == 3);
    return 0;
}
'''


class TrainingRuntimeTest(unittest.TestCase):
    def test_source_speed_cadence_and_dummy_mapping(self):
        compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc")
                         if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required")
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "training.c"
            program = Path(directory) / "training.exe"
            source.write_text(host_source())
            subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra",
                            "-Wno-unused-variable",
                            str(source), "-o", str(program)], check=True)
            result = subprocess.run([str(program)], capture_output=True,
                                    text=True)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_wrappers_are_passthrough_imports(self):
        scene = (ROOT / "src/import/battleship_sc1ptrainingmode.c").read_text()
        css = (ROOT / "src/import/battleship_mntraining.c").read_text()
        self.assertIn(
            '#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1ptrainingmode.c"',
            scene)
        self.assertIn(
            '#include "../../decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayers1ptraining.c"',
            css)
        for text, base, name in (
                (scene, "ndsBaseSC1PTrainingModeStartScene",
                 "sc1PTrainingModeStartScene"),
                (css, "ndsBaseMNPlayers1PTrainingStartScene",
                 "mnPlayers1PTrainingStartScene")):
            self.assertIn(f"#define {name} {base}", text)
            self.assertIn(f"void {name}(void)", text)
            self.assertIn(f"{base}();", text)
        # No duplicate shims: port headers own every enum/struct/audio ID.
        for text in (scene, css):
            self.assertNotIn("typedef enum SC1PTrainingMode", text)
            self.assertNotIn("struct SC1PTrainingModeMenu\n{", text)
            self.assertNotIn("typedef struct SC1PTrainingModeMenu", text)
            self.assertNotIn("#define nSYAudioBGMTrainingMode", text)
            self.assertNotIn("#define nSYAudioFGMTrainingSel2", text)
            self.assertNotIn("#define nSYAudioVoiceAnnounceTrainingMode", text)
        scene_h = (ROOT / "include/sc/scene.h").read_text()
        for token in ("SC1PTrainingModeMain", "SC1PTrainingModeCP",
                      "SC1PTrainingModeItem", "SC1PTrainingModeSpeed",
                      "SC1PTrainingModeView",
                      "SC1PTrainingModeMenuOptionSprites",
                      "SC1PTrainingModeSprites", "SC1PTrainingModeFiles"):
            self.assertIn(token, scene_h)
        self.assertIn("} SC1PTrainingModeMenu;", scene_h)
        gmsound = (ROOT / "include/gm/gmsound.h").read_text()
        self.assertIn("nSYAudioBGMTrainingMode = 42,", gmsound)
        self.assertIn("nSYAudioFGMTrainingSel2 = 162,", gmsound)
        self.assertIn("nSYAudioVoiceAnnounceTrainingMode = 530,", gmsound)
        mntypes = (ROOT / "include/mn/mntypes.h").read_text()
        self.assertIn("} MNPlayersSlotTraining;", mntypes)

    def test_scene_wiring_outside_wrappers(self):
        backend = (ROOT / "src/port/title_backend.c").read_text()
        for name in ("mnPlayers1PTrainingStartScene",
                     "sc1PTrainingModeStartScene"):
            at = backend.index(f"NDS_SCENE_STUB({name})")
            gate = backend.rfind("#if !NDS_P2_1P_GAME", 0, at)
            self.assertNotEqual(gate, -1, name)
            self.assertNotIn("#endif", backend[gate:at].replace(
                "#if !NDS_P2_1P_GAME", "", 1))
        harness = (ROOT / "src/port/taskman_seam_harness.c").read_text()
        self.assertIn("case nSCKindPlayers1PTraining:", harness)
        self.assertIn("(gSCManagerSceneData.scene_curr == nSCKind1PTrainingMode)",
                      harness)
        reloc = (ROOT / "include/reloc_data.h").read_text()
        for token in ("llGRWallpaperTrainingBlackFileID",
                      "llGRWallpaperTrainingYellowFileID",
                      "llGRWallpaperTrainingBlueFileID",
                      "llSC1PTrainingModeFileID",
                      "llSC1PTrainingModeMenuOptionSpriteArray"):
            self.assertIn(token, reloc)
        makefile = (ROOT / "Makefile").read_text()
        self.assertIn(
            "CFILES += battleship_mnmessage.c battleship_sc1ptrainingmode.c battleship_mntraining.c",
            makefile)


if __name__ == "__main__":
    unittest.main()
