"""Execute campaign allocation and pin enemy replacement's source lifetime."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]


class CampaignCapacityTest(unittest.TestCase):
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
static u32 last_flags; static int last_capacity;
static void ftManagerAllocFighter(u32 f, s32 c) { last_flags=f; last_capacity=c; }
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
