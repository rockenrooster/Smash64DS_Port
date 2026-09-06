"""Execute DS save I/O against a FAT model with failed writes, renames and reboots."""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from source_test_helpers import braced, function

ROOT = Path(__file__).resolve().parents[2]


def host_source(diagnostic=False):
    header = (ROOT / "include/sc/scene.h").read_text()
    types = "\n".join(braced(header, rf"typedef struct {name}\s*\{{", True)
                      for name in ("LBBackupVSRecord", "LBBackup1PRecord", "LBBackupData"))
    shim = (ROOT / "src/import/battleship_lbbackup.c").read_text()
    functions = "\n".join(function(shim, name) for name in (
        "lbBackupCreateChecksum", "lbBackupIsChecksumValid", "lbBackupWrite",
        "lbBackupIsSramValid", "lbBackupClearAllData"))
    original = (ROOT / "decomp/BattleShip-main/decomp/src/sc/scmanager.c").read_text()
    defaults = braced(original, r"LBBackupData dSCManagerDefaultBackupData\s*=\s*\{", True)
    backup = re.sub(r"^#include[^\n]*", "", (ROOT / "src/nds/nds_backup.c").read_text(), flags=re.M)
    defines = "\n".join(re.findall(r"^#define NDS_BACKUP_\w+[^\n]*",
                                   (ROOT / "include/nds/nds_backup.h").read_text(), re.M))
    result = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define TRUE 1
#define FALSE 0
#define GMCOMMON_FIGHTERS_PLAYABLE_NUM 12
#define I_MIN_TO_TICS(x) ((x) * 60 * 60)
enum { nFTKindMario = 0, nSC1PGameDifficultyEasy = 1 };
typedef uint8_t u8;
typedef uint8_t ub8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef int sb32;
''' + types + "\n" + defines + "\n" + defaults + r'''
static LBBackupData gSCManagerBackupData;
s32 lbBackupCreateChecksum(LBBackupData *backup);
void ndsBackupSramRead(uintptr_t offset, void *data, size_t size);
void ndsBackupSramWrite(void *data, uintptr_t offset, size_t size);
s32 ndsBackupFlush(void);
#define NDS_LBBACKUP_COPY0_OFFSET 0u
#define NDS_LBBACKUP_COPY1_OFFSET ((sizeof(LBBackupData) + 15u) & ~(size_t)15u)
typedef struct { char name[64]; u8 data[NDS_BACKUP_IMAGE_BYTES]; size_t size; int exists; } MockFile;
static MockFile disk[6];
static struct { MockFile *file; size_t offset; } handle;
static int rename_fail, close_fail, short_write, writes, volume;
static int cut_after, mutation_count;
static jmp_buf power_cut;
static void mutation(void) { if (cut_after && ++mutation_count == cut_after) longjmp(power_cut, 1); }
static MockFile *find_file(const char *name)
{
    for (int i = 0; i < 6; i++) if (!strcmp(name, disk[i].name)) return &disk[i];
    CHECK(0); return NULL;
}
static void *mock_fopen(const char *name, const char *mode)
{
    MockFile *file = find_file(name);
    if (!volume || !strncmp(name, "sd:", 3)) return NULL;
    if (mode[0] == 'r' && !file->exists) return NULL;
    if (mode[0] == 'w') { file->exists = 1; file->size = 0; writes++; mutation(); }
    handle.file = file; handle.offset = 0; return &handle;
}
static size_t mock_fread(void *dst, size_t unit, size_t count, void *stream)
{
    (void)stream; CHECK(unit == 1);
    size_t got = handle.file->size < count ? handle.file->size : count;
    memcpy(dst, handle.file->data, got); return got;
}
static size_t mock_fwrite(const void *src, size_t unit, size_t count, void *stream)
{
    (void)stream; CHECK(unit == 1);
    size_t put = short_write ? 31 : count;
    memcpy(handle.file->data, src, put); handle.file->size = put; mutation(); return put;
}
static int mock_fclose(void *stream) { (void)stream; mutation(); return close_fail ? -1 : 0; }
static int mock_remove(const char *name)
{
    MockFile *file = find_file(name);
    int result = file->exists ? 0 : -1;
    file->exists = 0; mutation(); return result;
}
static int mock_rename(const char *from, const char *to)
{
    if (rename_fail == 1 && strstr(from, ".tmp")) return -1;
    if (rename_fail == 2 && strstr(to, ".bak")) return -1;
    MockFile *src = find_file(from), *dst = find_file(to);
    if (!src->exists || dst->exists) return -1; /* FAT does not replace a target. */
    memcpy(dst->data, src->data, src->size); dst->size = src->size;
    dst->exists = 1; src->exists = 0; mutation(); return 0;
}
static int fatInitDefault(void) { return volume; }
#define FILE void
#define fopen mock_fopen
#define fread mock_fread
#define fwrite mock_fwrite
#define fclose mock_fclose
#define remove mock_remove
#define rename mock_rename
''' + backup + "\n" + functions + r'''
static void reboot(void)
{
    sNdsBackupLoaded = sNdsBackupMountTried = 0;
    sNdsBackupPrimaryValid = sNdsBackupTempValid = 0;
    sNdsBackupPath = NULL;
    memset(sNdsBackupImage, 0xa5, sizeof(sNdsBackupImage));
    memset(&gSCManagerBackupData, 0xa5, sizeof(gSCManagerBackupData));
    rename_fail = close_fail = short_write = cut_after = mutation_count = 0;
    gNdsBackupLoadResult = NDS_BACKUP_LOAD_UNTRIED;
}
static void fresh(void)
{
    memset(disk, 0, sizeof(disk));
    const char *names[] = {"fat:/smash64ds.sav", "fat:/smash64ds.sav.bak", "fat:/smash64ds.sav.tmp",
                          "sd:/smash64ds.sav", "sd:/smash64ds.sav.bak", "sd:/smash64ds.sav.tmp"};
    for (int i = 0; i < 6; i++) strcpy(disk[i].name, names[i]);
    volume = 1; reboot();
    CHECK(lbBackupIsSramValid() == FALSE);
    CHECK(gSCManagerBackupData.signature == 666);
}
static void score(u32 value) { gSCManagerBackupData.spgame_records[0].spgame_hiscore = value; lbBackupWrite(); }
static u32 reload(void) { reboot(); CHECK(lbBackupIsSramValid()); return gSCManagerBackupData.spgame_records[0].spgame_hiscore; }
int main(void)
{
    fresh(); score(100); CHECK(reload() == 100); CHECK(reload() == 100);
    for (int fault = 0; fault < 4; fault++) {
        fresh(); score(100);
        if (fault == 0) short_write = 1;
        if (fault == 1) close_fail = 1;
        if (fault == 2) rename_fail = 1;
        if (fault == 3) rename_fail = 2;
        u32 failures = gNdsBackupWriteFailCount;
        score(200); CHECK(gNdsBackupWriteFailCount == failures + 1);
        CHECK(reload() == 100);
    }
    /* Power loss after every persistent operation in an ordinary update. */
    for (int cut = 1; cut <= 6; cut++) {
        fresh(); score(100);
        cut_after = cut; mutation_count = 0;
        if (!setjmp(power_cut)) score(200);
        u32 recovered = reload(); CHECK(recovered == 100 || recovered == 200);
    }
    /* A failed first save leaves only .tmp. Retrying must not truncate it. */
    fresh();
    for (int i = 0; i < 6; i++) disk[i].exists = 0;
    sNdsBackupPrimaryValid = 0;
    rename_fail = 1; score(300);
    int written = writes;
    score(400); CHECK(writes == written);
    CHECK(reload() == 300);
    /* Reject damaged/truncated candidates; source slot 1 can repair slot 0. */
    fresh(); score(100); score(200);
    disk[0].data[0] ^= 1; CHECK(reload() == 200);
    CHECK(lbBackupIsChecksumValid());
    fresh(); score(100); score(200);
    disk[0].size = 17; CHECK(reload() == 100);
    fresh(); score(100); score(200);
    memcpy(disk[2].data, disk[0].data, sizeof(disk[2].data));
    disk[2].size = sizeof(disk[2].data); disk[2].exists = 1;
    disk[0].size = disk[1].size = 7;
    CHECK(reload() == 200);
    /* Completed Backup Clear must remain clear on subsequent boots. */
    fresh(); score(999); gSCManagerBackupData.fighter_mask = 0xfff;
    gSCManagerBackupData.unlock_mask = 0x7f; lbBackupWrite();
    lbBackupClearAllData(); lbBackupWrite();
    CHECK(reload() == 0); CHECK(gSCManagerBackupData.fighter_mask == 0);
    CHECK(gSCManagerBackupData.unlock_mask == 0);
    CHECK(gSCManagerBackupData.spgame_records[0].bonus1_time == I_MIN_TO_TICS(60));
    CHECK(reload() == 0);
    return 0;
}
'''
    if diagnostic:
        # Only the independent FAT fixture's literal filenames change. The
        # production arrays concatenate NDS_BACKUP_FILENAME and select it from
        # compiler flags, so touching a canonical filename fails find_file().
        result = result.replace("fat:/smash64ds.sav", "fat:/smash64ds-diagnostic.sav")
        result = result.replace("sd:/smash64ds.sav", "sd:/smash64ds-diagnostic.sav")
    return result


class BackupRecoveryTest(unittest.TestCase):
    def test_fat_failures_recovery_and_backup_clear(self):
        compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc") if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required")
        with tempfile.TemporaryDirectory() as directory:
            source, program = Path(directory) / "backup.c", Path(directory) / "backup.exe"
            flags = ("NDS_DEV_SCENE_HARNESS", "NDS_P2_MENU_WALK", "NDS_HARNESS_FAST_LOGIC",
                     "NDS_R2_BOTH_CPU", "NDS_P2_FOUR_CPU_STRESS")
            for active in (None, *flags):
                with self.subTest(configuration=active or "normal"):
                    source.write_text(host_source(diagnostic=active is not None))
                    defines = [f"-D{flag}={163 if flag == active == flags[0] else int(flag == active)}"
                               for flag in flags]
                    subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                                    "-DNDS_DEV_SCENE_HARNESS_NORMAL=0", *defines,
                                    str(source), "-o", str(program)], check=True)
                    subprocess.run([str(program)], check=True)


if __name__ == "__main__":
    unittest.main()
