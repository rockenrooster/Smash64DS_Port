"""Run the source Results unlock decision through the DS transition adapter."""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from source_test_helpers import braced, function, original_enum

ROOT = Path(__file__).resolve().parents[2]
DECOMP = ROOT / "decomp/BattleShip-main/decomp/src"


def host_source(adapter):
    results = (DECOMP / "mn/mnvsmode/mnvsresults.c").read_text()
    exit_block = braced(results, r"if \(mnVSResultsCheckExit\(\) != FALSE\)\s*\{")
    message = function((DECOMP / "mn/mncommon/mnmessage.c").read_text(), "mnMessageApplyUnlock")
    enums = "\n".join(original_enum(path, name) for path, name in (
        ("ft/ftdef.h", "FTKind"), ("gr/grdef.h", "GRKind"),
        ("sc/scdef.h", "SCKind"), ("lb/lbdef.h", "LBBackupUnlock")))
    lbdef = (DECOMP / "lb/lbdef.h").read_text()
    masks = lbdef[lbdef.index("#define LBBACKUP_MASK_FIGHTER"):lbdef.index("#define LBBACKUP_ERROR_RANDOMKNOCKBACK")]
    return r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define FALSE 0
#define TRUE 1
#define NDS_P2_MENU_SHELL 1
#define NDS_DEMO_FOX_CPU_LADDER 0
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
''' + enums + "\n" + masks + r'''
static struct { int scene_curr, scene_prev, unlock_messages[nLBBackupUnlockEnumCount]; } gSCManagerSceneData;
static struct {
    int unlock_mask, fighter_mask, characters_fkind, vs_itemswitch_battles, ground_mask;
    struct { int is_spgame_complete; } spgame_records[12];
} gSCManagerBackupData;
static int writes, loads, stops, sMNMessageUnlockID;
static u32 gNdsVSResultsRematchCount;
static void lbBackupWrite(void) { writes++; }
static void syAudioStopBGMAll(void) { stops++; }
static void func_800266A0_272A0(void) {}
static int mnVSResultsCheckExit(void) { return TRUE; }
static void ndsSceneManagerRequest(u32 next, u32 previous)
{
    CHECK(next == nSCKindMessage || next == nSCKindPlayersVS);
    CHECK(previous == nSCKindVSResults);
    gSCManagerSceneData.scene_curr = next;
    gSCManagerSceneData.scene_prev = previous;
    loads++;
}
''' + adapter + r'''
#define syTaskmanSetLoadScene ndsMNVSResultsSetLoadScene
static void source_results_exit(void)
{
    s32 unlocks_num, i;
    u16 spgame_complete_mask;
''' + exit_block + "\n}\n" + message + r'''
static void reset(void)
{
    memset(&gSCManagerBackupData, 0, sizeof(gSCManagerBackupData));
    gSCManagerSceneData.scene_curr = nSCKindVSResults;
    for (int i = 0; i < nLBBackupUnlockEnumCount; i++)
        gSCManagerSceneData.unlock_messages[i] = nLBBackupUnlockEnumCount;
    writes = loads = stops = 0;
}
static void complete_starters(void)
{
    gSCManagerBackupData.ground_mask = LBBACKUP_GROUND_MASK_ALL;
    for (int i = 0; i < 12; i++)
        gSCManagerBackupData.spgame_records[i].is_spgame_complete =
            (LBBACKUP_CHARACTER_MASK_STARTER & (1 << i)) != 0;
}
#if NDS_P2_1P_GAME
static void accept(int index, int unlock)
{
    CHECK(gSCManagerSceneData.scene_curr == nSCKindMessage);
    CHECK(gSCManagerSceneData.unlock_messages[index] == unlock);
    sMNMessageUnlockID = unlock;
    mnMessageApplyUnlock();
    CHECK(gSCManagerBackupData.unlock_mask & (1 << unlock));
}
#endif
int main(void)
{
    reset(); gSCManagerBackupData.vs_itemswitch_battles = 99;
    source_results_exit(); CHECK(gSCManagerSceneData.scene_curr == nSCKindPlayersVS);
    CHECK(loads == 1 && writes == 0);
#if NDS_P2_1P_GAME
    reset(); gSCManagerBackupData.vs_itemswitch_battles = 100;
    source_results_exit(); accept(0, nLBBackupUnlockItemSwitch); CHECK(writes == 1);
    reset(); complete_starters(); source_results_exit();
    accept(0, nLBBackupUnlockInishie); CHECK(writes == 1);
    reset(); complete_starters(); gSCManagerBackupData.vs_itemswitch_battles = 100;
    source_results_exit(); accept(0, nLBBackupUnlockItemSwitch);
    accept(1, nLBBackupUnlockInishie); CHECK(writes == 2 && loads == 1);
    gSCManagerSceneData.scene_curr = nSCKindVSResults;
    source_results_exit(); CHECK(gSCManagerSceneData.scene_curr == nSCKindPlayersVS);
    reset(); complete_starters(); gSCManagerBackupData.spgame_records[nFTKindMario].is_spgame_complete = FALSE;
    source_results_exit(); CHECK(gSCManagerSceneData.scene_curr == nSCKindPlayersVS);
#else
    reset(); complete_starters(); gSCManagerBackupData.vs_itemswitch_battles = 100;
    source_results_exit(); CHECK(gSCManagerSceneData.scene_curr == nSCKindPlayersVS);
    CHECK(loads == 1 && writes == 0 && gSCManagerBackupData.unlock_mask == 0);
#endif
    /* The same source application publishes each newcomer and its record-page selection. */
    const int fighters[] = {nFTKindLuigi, nFTKindNess, nFTKindCaptain, nFTKindPurin};
    for (int i = 0; i < 4; i++) {
        reset(); sMNMessageUnlockID = i; mnMessageApplyUnlock();
        CHECK(gSCManagerBackupData.fighter_mask == (1 << fighters[i]));
        CHECK(gSCManagerBackupData.characters_fkind == fighters[i] && writes == 1);
    }
    return 0;
}
'''


class ResultsUnlockRoutingTest(unittest.TestCase):
    def test_source_unlocks_reach_message(self):
        text = (ROOT / "src/import/battleship_mnvsresults.c").read_text()
        self.assertIn("#define syTaskmanSetLoadScene ndsMNVSResultsSetLoadScene", text)
        adapter = function(text, "ndsMNVSResultsSetLoadScene")
        compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc") if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required")
        with tempfile.TemporaryDirectory() as directory:
            source, program = Path(directory) / "unlocks.c", Path(directory) / "unlocks.exe"
            for enabled, broken in ((0, False), (1, False), (1, True)):
                candidate = adapter.replace("(u32)gSCManagerSceneData.scene_curr", "(u32)nSCKindPlayersVS") if broken else adapter
                self.assertEqual(candidate != adapter, broken)
                source.write_text(host_source(candidate))
                subprocess.run([compiler, "-std=c11", "-DREGION_US=1", f"-DNDS_P2_1P_GAME={enabled}",
                                "-Wall", "-Wextra", "-Werror",
                                str(source), "-o", str(program)], check=True)
                result = subprocess.run([str(program)], capture_output=True, text=True)
                self.assertEqual(result.returncode != 0, broken, result.stderr)


if __name__ == "__main__":
    unittest.main()
