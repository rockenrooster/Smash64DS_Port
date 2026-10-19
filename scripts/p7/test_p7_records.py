"""Host tests for P7 U2's wide VS records (src/nds/nds_p7_records.c).

The module is compiled for the host with the save's sibling-file API backed
by files in a temporary directory (the same transaction as nds_backup.c: a
temporary, the canonical file kept as .bak, then the promotion). Checks: a
content's KO counts against the content, never its parent; matches add up;
two players of one fighter share a row; the source's caps hold; a torn
canonical file falls back to .bak, a lone temporary is promoted; Backup
Clear zeroes every row.
"""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def _compiler():
    for name in ("clang", "gcc", "cc"):
        path = shutil.which(name)
        if path:
            return path
    return None


def _source() -> str:
    header = (ROOT / "include/nds/nds_p7.h").read_text()
    header = re.sub(r"^#include[^\n]*", "", header, flags=re.M)
    body = (ROOT / "src/nds/nds_p7_records.c").read_text()
    body = re.sub(r"^#include[^\n]*", "", body, flags=re.M)
    return r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
typedef unsigned long long u64; typedef int s32; typedef signed char s8; typedef float f32;
#define TRUE 1
#define FALSE 0
#define NDS_DIAG(...) do { __VA_ARGS__; } while (0)
#define GMCOMMON_PLAYERS_MAX 4
#define GMCOMMON_FIGHTERS_PLAYABLE_NUM 12
#define UPDATE_INTERVAL 60
#define I_MIN_TO_TICS(q) ((int)((q) * 3600))
enum { nFTKindMario = 0, nFTKindFox = 1, nFTKindMMario = 13 };
enum { nFTPlayerKindMan = 0, nFTPlayerKindCom = 1, nFTPlayerKindNot = 2 };
typedef struct { u8 pkind; u8 fkind; u32 total_damage_given; u32 total_damage_all;
                 u32 total_selfdestructs; u32 total_kos_players[4]; } SCPlayer;
typedef struct SCBattleState { u32 time_passed; SCPlayer players[4]; } SCBattleState;
#define NDS_BACKUP_SIBLING_RECORDS 0u
#define NDS_BACKUP_SIBLING_1P_RECORDS 1u
typedef unsigned char ub8;
/* C3's wide 1P rows share the module (sc/scene.h LBBackup1PRecord). */
typedef struct LBBackup1PRecord { u32 spgame_hiscore; u32 spgame_continues; u32 spgame_total_bonuses;
    u8 spgame_best_difficulty; u32 bonus1_time; u8 bonus1_task_count; u32 bonus2_time;
    u8 bonus2_task_count; ub8 is_spgame_complete; } LBBackup1PRecord;
static struct { LBBackup1PRecord spgame_records[12]; } gSCManagerBackupData;
struct { u8 fkind; } gSCManagerSceneData;
#define NDS_BACKUP_RECOVERY_NAMES 3u
typedef s32 (*NDSBackupWriter)(FILE *tmp, void *ctx);
static char sDir[512];
static const char *name(u32 r) {
    static char buf[3][600];
    const char *suffix[3] = { "", ".bak", ".tmp" };
    snprintf(buf[r], sizeof(buf[r]), "%s/vs.sav%s", sDir, suffix[r]);
    return buf[r];
}
void ndsFsLock(void) {}
void ndsFsUnlock(void) {}
FILE *ndsBackupOpenSibling(u32 s, u32 r, const char *mode) { (void)s; return fopen(name(r), mode); }
s32 ndsBackupPromoteSibling(u32 s) { (void)s; remove(name(0)); return rename(name(2), name(0)) == 0; }
s32 ndsBackupTransactSibling(u32 s, NDSBackupWriter write, void *ctx, FILE **src) {
    FILE *tmp = fopen(name(2), "wb"); s32 ok; (void)s;
    if (tmp == NULL) return FALSE;
    ok = write(tmp, ctx);
    if (fclose(tmp) != 0) ok = FALSE;
    if (src && *src) { fclose(*src); *src = NULL; }
    if (!ok) { remove(name(2)); return FALSE; }
    remove(name(1)); rename(name(0), name(1));
    return rename(name(2), name(0)) == 0;
}
''' + header + "\n" + body + r'''
static SCBattleState sState;
int main(int argc, char **argv)
{
    /* argv: dir, command, then per command */
    NDSP7VSRecord row;
    u32 i;
    if (argc < 3) return 2;
    snprintf(sDir, sizeof(sDir), "%s", argv[1]);
    if (strcmp(argv[2], "match") == 0) {
        /* match t p0 f0 c0 dg0 dt0 sd0 k00 k01 k02 k03 p1 ... (4 players) */
        u8 sel[4];
        sState.time_passed = (u32)strtoul(argv[3], 0, 10);
        for (i = 0; i < 4; i++) {
            char **a = &argv[4 + i * 10];
            u32 k;
            sState.players[i].pkind = (u8)atoi(a[0]);
            sState.players[i].fkind = (u8)atoi(a[1]);
            sel[i] = (u8)ndsP7SelOf(sState.players[i].fkind, (u32)atoi(a[2]));
            sState.players[i].total_damage_given = (u32)strtoul(a[3], 0, 10);
            sState.players[i].total_damage_all = (u32)strtoul(a[4], 0, 10);
            sState.players[i].total_selfdestructs = (u32)strtoul(a[5], 0, 10);
            for (k = 0; k < 4; k++) sState.players[i].total_kos_players[k] = (u32)strtoul(a[6 + k], 0, 10);
        }
        ndsP7RecordsAddMatch(&sState, sel);
        return 0;
    }
    if (strcmp(argv[2], "clear") == 0) { ndsP7RecordsClear(); return 0; }
    if (strcmp(argv[2], "row") == 0) {
        ndsP7RecordsRead((u32)atoi(argv[3]), &row);
        printf("%u %u %u %u %u %u", row.time_used, row.damage_given, row.damage_taken,
               row.selfdestructs, row.games_played, row.player_count_tally);
        for (i = 0; i < NDS_P7_SEL_COUNT; i++) printf(" %u", row.ko_count[i]);
        for (i = 0; i < NDS_P7_SEL_COUNT; i++) printf(" %u", row.played_against[i]);
        printf("\n");
        return 0;
    }
    return 2;
}
'''


NOT = 2


def player(pkind=NOT, fkind=0, content=0, dg=0, dt=0, sd=0, kos=(0, 0, 0, 0)):
    return [pkind, fkind, content, dg, dt, sd, *kos]


class RecordsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc = _compiler()
        if cls.cc is None:
            raise unittest.SkipTest("no host C compiler")
        cls.tmp = tempfile.mkdtemp()
        src = Path(cls.tmp) / "records.c"
        src.write_text(_source())
        cls.exe = str(Path(cls.tmp) / "records")
        subprocess.run([cls.cc, "-std=c11", "-Wall", "-Wextra", "-Werror", str(src), "-o", cls.exe],
                       check=True)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.tmp, ignore_errors=True)

    def setUp(self):
        self.dir = tempfile.mkdtemp(dir=self.tmp)

    def run_exe(self, *args):
        return subprocess.run([self.exe, self.dir, *map(str, args)], check=True,
                              capture_output=True, text=True).stdout.split()

    def match(self, time, *players):
        args = [time]
        for p in players:
            args += p
        for _ in range(4 - len(players)):
            args += player()
        self.run_exe("match", *args)

    def row(self, sel):
        v = list(map(int, self.run_exe("row", sel)))
        return {"time": v[0], "given": v[1], "taken": v[2], "sd": v[3], "games": v[4],
                "tally": v[5], "ko": v[6:46], "against": v[46:86]}

    def test_content_counts_as_itself(self):
        # Falco (content 1, parent Fox) KOs Mario twice; Mario KOs Falco once.
        self.match(3600, player(1, 1, 1, 80, 30, 0, (0, 0, 0, 0)),
                   player(0, 0, 0, 30, 80, 1, (0, 0, 0, 0)))
        self.match(3600, player(1, 1, 1, 10, 5, 0, (0, 2, 0, 0)),
                   player(0, 0, 0, 5, 10, 0, (1, 0, 0, 0)))
        falco = self.row(13)
        mario = self.row(0)
        fox = self.row(1)
        self.assertEqual(falco["ko"][0], 2)
        self.assertEqual(mario["ko"][13], 1)
        self.assertEqual(mario["ko"][1], 0)
        self.assertEqual(fox["games"], 0)
        self.assertEqual(falco["games"], 2)
        self.assertEqual(falco["against"][0], 2)
        self.assertEqual(falco["given"], 90)
        self.assertEqual(mario["sd"], 1)
        self.assertEqual(falco["time"], 120)

    def test_same_fighter_shares_a_row_and_metal_mario(self):
        self.match(600, player(1, 13, 0, 10, 0, 0, (0, 1, 1, 0)),
                   player(1, 13, 0, 10, 0, 0, (1, 0, 0, 0)),
                   player(1, 0, 0, 0, 0, 0, (0, 0, 0, 0)))
        mm = self.row(12)
        self.assertEqual(mm["games"], 2)
        self.assertEqual(mm["ko"][12], 2)
        self.assertEqual(mm["ko"][0], 1)
        self.assertEqual(mm["tally"], 6)

    def test_caps(self):
        for _ in range(2):
            self.match(0, player(1, 0, 0, 999990, 0, 0, (0, 0, 0, 0)),
                       player(1, 1, 0, 0, 0, 0, (0, 0, 0, 0)))
        self.assertEqual(self.row(0)["given"], 999999)

    def test_recovery_and_clear(self):
        self.match(60, player(1, 0, 0, 5, 0, 0, (0, 0, 0, 0)), player(1, 1, 0, 0, 0, 0, (0, 0, 0, 0)))
        self.match(60, player(1, 0, 0, 7, 0, 0, (0, 0, 0, 0)), player(1, 1, 0, 0, 0, 0, (0, 0, 0, 0)))
        canon = Path(self.dir) / "vs.sav"
        bak = Path(self.dir) / "vs.sav.bak"
        self.assertTrue(bak.exists())
        # A torn canonical file: the previous file answers.
        canon.write_bytes(canon.read_bytes()[:100])
        self.assertEqual(self.row(0)["given"], 5)
        # Only a temporary left: it is promoted and read.
        tmp = Path(self.dir) / "vs.sav.tmp"
        shutil.copy(bak, tmp)
        canon.unlink()
        bak.unlink()
        self.assertEqual(self.row(0)["given"], 5)
        self.assertTrue(canon.exists())
        self.run_exe("clear")
        self.assertEqual(self.row(0)["given"], 0)
        self.assertEqual(self.row(0)["games"], 0)


if __name__ == "__main__":
    unittest.main()
