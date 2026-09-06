"""Host-check common battle texture ordering, reentry and delayed 1P BGM."""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from source_test_helpers import braced, function, original_enum

ROOT = Path(__file__).resolve().parents[2]
DECOMP = ROOT / "decomp/BattleShip-main/decomp/src"


def sources():
    vs = (ROOT / "src/import/battleship_scvsbattle.c").read_text()
    harness = (ROOT / "src/port/taskman_seam_harness.c").read_text()
    prepare = function(vs, "ndsBattlePrepareSceneTextures")
    gate = braced(harness, r"if \(\(gNdsSceneManagerCurrIsBattle != 0u\)")
    return vs, harness, prepare, gate


def host_source(prepare, gate, vs):
    enums = original_enum("sc/scdef.h", "SCKind") + "\n" + original_enum("sc/scdef.h", "SC1PGameStageKind")
    start_bgm = function(vs, "ndsSCVSBattleStartPlayBGM")
    source_go = function((DECOMP / "sc/sc1pmode/sc1pgame.c").read_text(), "sc1PGameSetGameStart")
    registry = (ROOT / "src/port/nds_scene_manager.c").read_text()
    cases = []
    for kind in ("VSBattle", "1PGame", "1PBonusStage", "1PTrainingMode", "AutoDemo", "Explain", "1PIntro"):
        flags = re.search(r"\{\s*\(u8\)nSCKind" + kind + r",\s*([^,]+),", registry)[1]
        cases.append(f"{{nSCKind{kind}, {flags}}}")
    return r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define TRUE 1
#define FALSE 0
#define NDS_R2_PARTICLE_DRAW 1
#define NDS_R2_IMPACT_WAVE_NATIVE 1
#define NDS_R2_REBIRTH_HALO_NATIVE 1
#define NDS_RENDERER_HW_TRIANGLES 1
#define NDS_R2_ANIM_CACHE 0
#define NDS_SCENE_FLAG_ARENA_RESET 1
#define NDS_SCENE_FLAG_MENU 2
#define NDS_SCENE_FLAG_BATTLE 4
typedef uint32_t u32;
typedef uint8_t u8;
''' + enums + r'''
static struct { int scene_curr, spgame_stage; } gSCManagerSceneData;
static u32 gNdsSceneManagerCurrIsBattle;
static int bank_registered, at_go, resets, sSC1PGameIsStartStage;
static char order[64];
static void step(char code)
{
    CHECK(bank_registered && !at_go);
    size_t len = strlen(order); CHECK(len + 1 < sizeof(order));
    order[len] = code; order[len + 1] = 0;
}
static void ndsIFCommonNativeOamReleaseCloudTextures(void) { step('C'); }
static void ndsRendererHardwareDiscardParticleAtlas(void) { step('D'); }
static void ndsRendererHardwareResetSceneTextureVram(void) { step('R'); resets++; }
static int ndsRendererHardwarePrepareBattleStaticTextures(void) { step('S'); return TRUE; }
static int ndsIFCommonNativeOamPrepareClouds(void) { step('T'); return TRUE; }
static int ndsRendererHardwarePrepareParticleAtlas(void) { step('P'); return TRUE; }
static int ndsRendererHardwarePrepareImpactWaveTextures(void) { step('I'); return TRUE; }
static int ndsRendererHardwarePrepareRebirthHaloTextures(void) { step('H'); return TRUE; }
static int ndsRendererHardwarePrepareEntryEffectTextures(void) { step('E'); return TRUE; }
static void ndsSCVSBattleBeginScenePlacement(void) {}
static void mpCollisionSetPlayBGM(void) {}
''' + prepare + "\n" + start_bgm + "\n" + source_go + r'''
static void runner_entry(void)
{
''' + gate + r'''
}
int main(void)
{
    const struct { int kind, flags; } cases[] = {
''' + ",\n".join(cases) + r'''
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        for (int entry = 0; entry < 2; entry++) {
            gSCManagerSceneData.scene_curr = cases[i].kind;
            gNdsSceneManagerCurrIsBattle = !!(cases[i].flags & NDS_SCENE_FLAG_BATTLE);
            order[0] = 0; resets = at_go = 0;
            bank_registered = 1; /* Source func_start has completed ground/bank setup. */
            if (cases[i].kind == nSCKindVSBattle) ndsSCVSBattleStartPlayBGM();
            runner_entry();
            if (!gNdsSceneManagerCurrIsBattle) { CHECK(resets == 0); continue; }
            CHECK(resets == 1 && !strcmp(order, "CDRSTPIHE"));
            /* Actual source GO callback: Metal/Zako defer only their music. */
            at_go = 1;
            const int stages[] = {nSC1PGameStageMMario, nSC1PGameStageZako, nSC1PGameStageLink};
            for (unsigned stage = 0; stage < sizeof(stages) / sizeof(stages[0]); stage++) {
                gSCManagerSceneData.spgame_stage = stages[stage];
                sc1PGameSetGameStart();
                CHECK(resets == 1 && !strcmp(order, "CDRSTPIHE"));
            }
        }
    }
    return 0;
}
'''


class BattleTextureEntryTest(unittest.TestCase):
    def test_source_lifecycle_and_no_music_hook(self):
        vs, harness, prepare, gate = sources()
        self.assertEqual(harness.count("ndsBattlePrepareSceneTextures();"), 1)
        outer = harness.index("if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle)\n#if NDS_P2_1P_GAME")
        self.assertLess(outer, harness.index(gate))
        self.assertLess(harness.index(gate), harness.index("gNdsTaskmanContexts = 2;", outer))
        taskman = (DECOMP / "sys/taskman.c").read_text()
        load = function(taskman, "syTaskmanLoadScene")
        self.assertLess(load.index("func_start();"), load.index("syTaskmanRunTask("))
        for path, name in (("src/port/reloc_backend_compat_shims.c", "mpCollisionSetPlayBGM"),
                           ("src/port/reloc_backend_mp_collision.c", "mpCollisionSetBGM")):
            self.assertNotIn("ndsBattlePrepareSceneTextures", function((ROOT / path).read_text(), name))
        for forbidden in ("gNdsMatchConfig", "gSCManagerTransferBattleState", "ndsR2AnimCachePreload"):
            self.assertNotIn(forbidden, prepare)
        self.assertIn("ndsBattlePrepareSceneTextures();", function(vs, "ndsSCVSBattleStartPlayBGM"))

    def test_order_and_reentry(self):
        vs, _harness, prepare, gate = sources()
        compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc") if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required")
        with tempfile.TemporaryDirectory() as directory:
            source, program = Path(directory) / "textures.c", Path(directory) / "textures.exe"
            for broken in (False, True):
                candidate = gate.replace("!= (u8)nSCKindVSBattle", "== (u8)nSCKindVSBattle") if broken else gate
                self.assertEqual(candidate != gate, broken)
                source.write_text(host_source(prepare, candidate, vs))
                subprocess.run([compiler, "-std=c11", "-DREGION_US=1", "-Wall", "-Wextra", "-Werror",
                                str(source), "-o", str(program)], check=True)
                result = subprocess.run([str(program)], capture_output=True, text=True)
                self.assertEqual(result.returncode != 0, broken, result.stderr)


if __name__ == "__main__":
    unittest.main()
