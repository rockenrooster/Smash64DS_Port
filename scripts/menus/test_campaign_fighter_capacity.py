"""Execute campaign allocation and pin enemy replacement's source lifetime."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]


class CampaignCapacityTest(unittest.TestCase):
    def test_compact_battle_residency_uses_shared_battle_scene_predicate(self):
        scene_manager = (ROOT / 'src/port/nds_scene_manager.c').read_text()
        preview = (ROOT / 'src/port/reloc_preview_pack.c').read_text()
        manager = (ROOT / 'src/import/battleship_ftmanager.c').read_text()
        runtime = (ROOT / 'src/import/battleship_sc1pgame_runtime.c').read_text()
        one_player = (ROOT / 'decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c').read_text()
        versus = (ROOT / 'decomp/BattleShip-main/decomp/src/sc/sccommon/scvsbattle.c').read_text()
        native_address = function(preview, 'ndsRelocNativeAssetAddress')
        preview_load = function(preview, 'ndsRelocPreviewFighterLoadBegin')
        battle_core = function(preview, 'ndsRelocUseBattleCoreFighterData')
        patch_externs = function(preview, 'ndsRelocPatchCompactBattleMainExterns')
        manager_setup = function(manager, 'ftManagerSetupFilesAllKind')
        scene_exit = function(scene_manager, 'ndsSceneManagerExit')
        one_player_start = function(runtime, 'sc1PGameStartScene')

        for kind in ('nSCKindVSBattle', 'nSCKind1PGame', 'nSCKind1PBonusStage',
                     'nSCKind1PTrainingMode'):
            self.assertRegex(
                scene_manager,
                rf'\{{ \(u8\){kind}, NDS_SCENE_FLAG_ARENA_RESET \| NDS_SCENE_FLAG_BATTLE,')
        self.assertRegex(
            scene_manager,
            r'\{ \(u8\)nSCKind1PScoreUnk, NDS_SCENE_FLAG_ARENA_RESET \| '
            r'NDS_SCENE_FLAG_MENU,\s*NDS_SCENE_TRANSITION_SOURCE \},')
        compact_admission_bodies = (
            (native_address, 1, 0),
            (preview_load, 1, 1),
            (patch_externs, 1, 0),
            (manager_setup, 0, 1),
        )
        # Every site asks the one battle-core predicate, which reads the
        # scene's battle flag (and adds Results, the 1P continue and the
        # ending, the demo-pose scenes that stand one fighter).
        self.assertIn('gNdsSceneManagerCurrIsBattle != 0u', battle_core)
        self.assertNotIn('nSCKindVSBattle', battle_core)
        for body, false_sites, true_sites in compact_admission_bodies:
            self.assertNotIn('nSCKindVSBattle', body)
            self.assertEqual(
                len(re.findall(r'ndsRelocUseBattleCoreFighterData\(\)\s*==\s*FALSE', body)),
                false_sites)
            self.assertEqual(
                len(re.findall(r'ndsRelocUseBattleCoreFighterData\(\)\s*!=\s*FALSE', body)),
                true_sites)
        self.assertRegex(
            preview_load,
            r'if\s*\(\(ndsRelocUseBattleCoreFighterData\(\)\s*!=\s*FALSE\)\s*&&\s*'
            r'\(fkind\s*<\s*12\)\)\s*\{\s*path\s*=\s*battle_path;')
        self.assertRegex(
            manager_setup,
            r'\(preview\s*==\s*2\)\s*&&\s*'
            r'\(ndsRelocUseBattleCoreFighterData\(\)\s*!=\s*FALSE\)')
        self.assertRegex(scene_exit,
                         r'gNdsSceneManagerCurrIsBattle\s*=\s*0u\s*;')
        self.assertIn('ftManagerSetupFilesAllKind(gSCManagerBattleState->players[i].fkind);',
                      one_player)
        self.assertIn('ftManagerSetupFilesAllKind(gSCManagerBattleState->players[player].fkind);',
                      versus)
        prepare = function(scene_manager, 'ndsSceneManagerPrepareDrawMemory')
        self.assertNotIn('ndsPlatformReserveOriginalSpritePreview', prepare)
        self.assertRegex(
            one_player_start,
            r'dSC1PGameTaskmanSetup\.scene_setup\.dl_buffer1_size\s*=\s*'
            r'sizeof\(Gfx\)\s*\*\s*64u\s*;')

    def test_actual_wrapper_and_wave_lifetime(self):
        source = (ROOT / 'decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgame.c').read_text()
        spawn = function(source, 'sc1PGameSpawnEnemyTeamNext')
        self.assertLess(spawn.index('ftManagerDestroyFighter('), spawn.index('ftManagerMakeFighter('))
        self.assertNotRegex(spawn, r'gSCManagerBattleState->players\[[^]]+\]\.pkind\s*=(?!=)')
        wrapper = function((ROOT / 'src/import/battleship_sc1pgame_runtime.c').read_text(),
                           'ndsSC1PGameAllocFighters')
        program = r'''
#include <assert.h>
#include <stddef.h>
typedef unsigned u32; typedef int s32;
#define GMCOMMON_PLAYERS_MAX 4
#define nFTPlayerKindNot 2
static struct { struct { int pkind; } players[4]; } battle, *gSCManagerBattleState;
static u32 last_flags; static int last_capacity, hat_prepares;
static void ftManagerAllocFighter(u32 f, s32 c) { last_flags=f; last_capacity=c; }
/* Every build has Kirby: the wrapper readies his hats after each allocation. */
static void ndsSC1PGamePrepareKirbyHats(void) { hat_prepares++; }
''' + wrapper + r'''
int main(void) {
    gSCManagerBattleState = &battle;
    battle.players[0].pkind=0; battle.players[1].pkind=1;
    battle.players[2].pkind=2; battle.players[3].pkind=2;
    ndsSC1PGameAllocFighters(0x1234,4); assert(last_capacity==2 && last_flags==0x1234);
    battle.players[2].pkind=1; battle.players[3].pkind=1;
    ndsSC1PGameAllocFighters(1,4); assert(last_capacity==4);
    gSCManagerBattleState=NULL;
    ndsSC1PGameAllocFighters(7,4); assert(last_capacity==4 && last_flags==7);
    assert(hat_prepares==3);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            c = Path(directory) / 'capacity.c'
            exe = c.with_suffix('.exe')
            c.write_text(program)
            result = subprocess.run([shutil.which('gcc'), '-std=c11', str(c),
                                     '-o', str(exe)], capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            subprocess.run([str(exe)], check=True, capture_output=True)


if __name__ == '__main__':
    unittest.main()
