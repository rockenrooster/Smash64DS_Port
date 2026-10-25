"""Host tests for P7 U9's VS Record numbers (src/nds/nds_p7_vsrecord.c).

The module is compiled for the host over seeded wide rows (the records file's
read stubbed), and every number the screen draws is checked against the
source's formulas (mn/mndata/mnvsrecord.c) written out here over the shown
fighters: the twelve under the source's lock (Ness, Purin, Captain and Luigi
behind the save's fighter mask), Metal Mario and the compiled contents under
the P7 block's.
"""
import random
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SEL_COUNT = 40
SECRET = {4: 'luigi', 7: 'captain', 10: 'purin', 11: 'ness'}
CONTENTS = [c for c in range(1, 21) if c != 12]


def _compiler():
    for name in ("clang", "gcc", "cc"):
        path = shutil.which(name)
        if path:
            return path
    return None


def _source() -> str:
    header = (ROOT / "include/nds/nds_p7.h").read_text()
    header = re.sub(r"^#include[^\n]*", "", header, flags=re.M)
    body = (ROOT / "src/nds/nds_p7_vsrecord.c").read_text()
    body = re.sub(r"^#include[^\n]*", "", body, flags=re.M)
    return r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
typedef unsigned long long u64; typedef int s32; typedef signed char s8; typedef float f32;
typedef unsigned char ub8;
#define TRUE 1
#define FALSE 0
#define NDS_MMARIO 1
#define NDS_P4 1
#define NDS_P4_CONTENT_LIMIT 21
#define LBBACKUP_MASK_FIGHTER(kind) (1u << (kind))
enum { nFTKindMario, nFTKindFox, nFTKindDonkey, nFTKindSamus, nFTKindLuigi, nFTKindLink,
       nFTKindYoshi, nFTKindCaptain, nFTKindKirby, nFTKindPikachu, nFTKindPurin, nFTKindNess };
struct LBBackup1PRecord;
static struct { u16 fighter_mask; } gSCManagerBackupData;
typedef struct NDSP4Fighter { int unused; } NDSP4Fighter;
static const NDSP4Fighter sFighter;
static u32 sContentCompiled[NDS_P4_CONTENT_LIMIT];
static u32 sSelUnlocked[64];
const NDSP4Fighter *ndsP4Fighter(u32 content)
{
    return (content < NDS_P4_CONTENT_LIMIT && sContentCompiled[content]) ? &sFighter : NULL;
}
''' + header + r'''
static NDSP7VSRecord sRows[NDS_P7_SEL_COUNT];
u32 ndsP7FighterUnlocked(u32 sel) { return sSelUnlocked[sel]; }
u32 ndsP7DevOpen(void) { return 0u; }
void ndsP7RecordsReadRange(u32 first, u32 count, NDSP7VSRecord *rows)
{
    u32 i;
    for (i = 0; i < count; i++) rows[i] = sRows[first + i];
}
''' + body + r'''
static NDSP7VsRecordData sData;
int main(int argc, char **argv)
{
    /* stdin: mask, then per sel: unlocked, sd, games, tally, time, ko[40], tallies[40], against[40];
       then the compiled contents (21 flags). */
    u32 sel, k, i, j;
    unsigned v;
    (void)argc; (void)argv;
    if (scanf("%u", &v) != 1) return 2;
    gSCManagerBackupData.fighter_mask = (u16)v;
    for (sel = 0; sel < NDS_P7_SEL_COUNT; sel++) {
        NDSP7VSRecord *r = &sRows[sel];
        if (scanf("%u", &v) != 1) return 2; sSelUnlocked[sel] = v;
        if (scanf("%u", &v) != 1) return 2; r->selfdestructs = (u16)v;
        if (scanf("%u", &v) != 1) return 2; r->games_played = (u16)v;
        if (scanf("%u", &v) != 1) return 2; r->player_count_tally = (u16)v;
        if (scanf("%u", &v) != 1) return 2; r->time_used = v;
        for (k = 0; k < NDS_P7_SEL_COUNT; k++) { if (scanf("%u", &v) != 1) return 2; r->ko_count[k] = (u16)v; }
        for (k = 0; k < NDS_P7_SEL_COUNT; k++) { if (scanf("%u", &v) != 1) return 2; r->player_count_tallies[k] = (u16)v; }
        for (k = 0; k < NDS_P7_SEL_COUNT; k++) { if (scanf("%u", &v) != 1) return 2; r->played_against[k] = (u16)v; }
    }
    for (k = 0; k < NDS_P4_CONTENT_LIMIT; k++) { if (scanf("%u", &v) != 1) return 2; sContentCompiled[k] = v; }
    ndsP7VsRecordLoad(&sData);
    printf("count %u\n", sData.count);
    for (i = 0; i < sData.count; i++) printf("sel %u\n", sData.sel[i]);
    printf("total %u\n", ndsP7VsRecordTotalTKO(&sData));
    for (i = 0; i < sData.count; i++) {
        printf("row %u %u %u %.4f %.4f %u %.4f %.4f\n", ndsP7VsRecordKOs(&sData, i), ndsP7VsRecordTKO(&sData, i),
               ndsP7VsRecordTimeUsed(&sData, i), ndsP7VsRecordWinPercent(&sData, i),
               ndsP7VsRecordSDPercent(&sData, i), 0u, ndsP7VsRecordUsePercent(&sData, i),
               ndsP7VsRecordAvg(&sData, i));
        for (j = 0; j < sData.count; j++)
            printf("pair %u %u %u %.4f %.4f\n", i, j, ndsP7VsRecordKOsAgainst(&sData, i, j),
                   ndsP7VsRecordWinPercentAgainst(&sData, i, j), ndsP7VsRecordAvgAgainst(&sData, i, j));
    }
    return 0;
}
'''


class VsRecordTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc = _compiler()
        if cls.cc is None:
            raise unittest.SkipTest("no host C compiler")
        cls.tmp = tempfile.TemporaryDirectory()
        src = Path(cls.tmp.name) / "vsrecord.c"
        src.write_text(_source())
        cls.exe = str(Path(cls.tmp.name) / "vsrecord.exe")
        subprocess.run([cls.cc, "-std=c11", "-Wall", "-Wextra", "-Werror", str(src), "-o", cls.exe],
                       check=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def _run(self, seed, mask, unlocked, compiled):
        rnd = random.Random(seed)
        rows = []
        for sel in range(SEL_COUNT):
            rows.append({
                'unlocked': 1 if sel in unlocked else 0,
                'sd': rnd.randint(0, 40), 'games': rnd.randint(0, 90), 'tally': rnd.randint(0, 300),
                'time': rnd.randint(0, 50000),
                'ko': [rnd.randint(0, 60) for _ in range(SEL_COUNT)],
                'tallies': [rnd.randint(0, 200) for _ in range(SEL_COUNT)],
                'against': [rnd.randint(0, 60) for _ in range(SEL_COUNT)],
            })
        words = [str(mask)]
        for r in rows:
            words += [str(r['unlocked']), str(r['sd']), str(r['games']), str(r['tally']), str(r['time'])]
            words += [str(x) for x in r['ko'] + r['tallies'] + r['against']]
        words += ['1' if c in compiled else '0' for c in range(21)]
        out = subprocess.run([self.exe], input=' '.join(words), capture_output=True, text=True, check=True).stdout
        return rows, out.split('\n')

    def _expected_shown(self, mask, unlocked, compiled):
        shown = []
        for sel in range(SEL_COUNT):
            if sel < 12:
                ok = (mask >> sel) & 1 if sel in SECRET else 1
            elif sel == 12:
                ok = sel in unlocked
            else:
                c = sel - 12
                ok = c < 21 and c in compiled and sel in unlocked
            if ok:
                shown.append(sel)
        return shown

    def _check(self, seed, mask, unlocked, compiled):
        rows, lines = self._run(seed, mask, unlocked, compiled)
        shown = self._expected_shown(mask, unlocked, compiled)
        self.assertEqual(lines[0], f'count {len(shown)}')
        self.assertEqual([int(l.split()[1]) for l in lines[1:1 + len(shown)]], shown)
        kos = [sum(rows[a]['ko'][b] for b in shown) for a in shown]
        tko = [min(9999, sum(rows[b]['ko'][a] for b in shown) + rows[a]['sd']) for a in shown]
        total = sum(tko)
        self.assertEqual(lines[1 + len(shown)], f'total {total}')
        games = sum(rows[a]['games'] for a in shown)
        at = 2 + len(shown)
        for i, a in enumerate(shown):
            f = lines[at].split()
            at += 1
            self.assertEqual(int(f[1]), kos[i])
            self.assertEqual(int(f[2]), tko[i])
            self.assertEqual(int(f[3]), rows[a]['time'])
            self.assertAlmostEqual(float(f[4]), (kos[i] / total * 100.0) if total else 0.0, places=2)
            self.assertAlmostEqual(float(f[5]), (rows[a]['sd'] / tko[i] * 100.0) if tko[i] else 0.0, places=2)
            self.assertAlmostEqual(float(f[7]), (rows[a]['games'] / games * 100.0) if games else 0.0, places=2)
            self.assertAlmostEqual(float(f[8]), (rows[a]['tally'] / rows[a]['games']) if rows[a]['games'] else 0.0,
                                   places=3)
            for j, b in enumerate(shown):
                p = lines[at].split()
                at += 1
                self.assertEqual((int(p[1]), int(p[2])), (i, j))
                kf, ka = rows[a]['ko'][b], rows[b]['ko'][a]
                self.assertEqual(int(p[3]), kf)
                self.assertAlmostEqual(float(p[4]), (kf / (kf + ka) * 100.0) if kf + ka else 0.0, places=2)
                pa = rows[a]['against'][b]
                self.assertAlmostEqual(float(p[5]), (rows[a]['tallies'][b] / pa) if pa else 0.0, places=3)

    def test_everyone_open(self):
        sels = set(range(SEL_COUNT))
        self._check(1, 0xfff, sels, set(CONTENTS))

    def test_secret_four_locked_and_contents_partly(self):
        self._check(2, 0x0ff & ~((1 << 4) | (1 << 7)), {12, 13, 14, 20, 31, 32}, set(CONTENTS))

    def test_retired_and_uncompiled_contents_stay_out(self):
        # Banjo (12 -> sel 24) is never compiled; an unlocked bit cannot show him.
        self._check(3, 0xfff, set(range(SEL_COUNT)), set(CONTENTS) - {5})

    def test_empty_records(self):
        self._check(4, 0xfff, set(), set())


if __name__ == '__main__':
    unittest.main()
