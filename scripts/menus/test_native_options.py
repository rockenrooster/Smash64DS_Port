"""Native Options / Backup Clear contract against actual source behavior.

The N64 source of truth is decomp/BattleShip-main/decomp/src/mn/mnoption/.
Tests 1-2 compile and run the ACTUAL decomp routines on test-only scratch
data (the test_options_reentry harness vectors). Tests 3-6 compile and run
the ACTUAL native DS screens in src/nds/nds_menu_shell_option.c and
src/nds/nds_menu_shell_backupclear.c -- extracted verbatim with
source_test_helpers.function, with only type/stub shims around them -- on
host scratch structs and temp state. No Python behavior mirror: Python only
checks the compiled program exit code. No test touches the user's real save;
no ROM build or emulator run.
"""

import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DECOMP = ROOT / "decomp/BattleShip-main/decomp/src"
sys.path.insert(0, str(Path(__file__).resolve().parent))
from source_test_helpers import braced, function, original_enum  # noqa: E402
import test_options_reentry as reentry  # noqa: E402

OPTION_C = ROOT / "src/nds/nds_menu_shell_option.c"
BACKUP_C = ROOT / "src/nds/nds_menu_shell_backupclear.c"
GENERATOR = ROOT / "scripts/menus/generate_mn_ui_kit.py"


def compile_and_run(compiler, directory, name, text):
    source = directory / f"{name}.c"
    program = directory / f"{name}.exe"
    source.write_text(text)
    build = subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra",
                            "-Wno-unused-variable", "-Wno-unused-parameter",
                            str(source), "-o", str(program)],
                           capture_output=True, text=True)
    if build.returncode != 0:
        raise AssertionError(f"{name} build failed:\n{build.stderr}")
    return subprocess.run([str(program)], capture_output=True, text=True)


def native_defines(text, prefix):
    """Verbatim #define lines for prefix; no mirrored constants."""
    out = []
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("#define " + prefix):
            out.append(stripped)
        if stripped.startswith("#define NDS_MENU_VS_SURFACE_NONE"):
            out.append(stripped)
        if stripped.startswith("#define NDS_MENU_SHELL_SCREEN_"):
            out.append(stripped)
    seen = []
    for line in out:
        if line not in seen:
            seen.append(line)
    return "\n".join(seen) + "\n"


BACKUP_SURFACES = r'''
typedef unsigned short BKId;
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR 20u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_NEWCOMERS 21u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_NEWCOMERS_HI 22u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_1P 23u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_1P_HI 24u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_BONUS 25u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_BONUS_HI 26u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_VS 27u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_VS_HI 28u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_PRIZE 29u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_PRIZE_HI 30u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_ALL_DATA 31u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_ALL_DATA_HI 32u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM1_YES 33u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM1_NO 34u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM2_YES 35u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM2_NO 36u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM1_YES_FLASH 37u
#define NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM2_YES_FLASH 38u
'''

OPTION_SURFACES = r'''
#define NDS_MN_UI_KIT_SURFACE_OPTION 40u
#define NDS_MN_UI_KIT_SURFACE_OPTION_SOUND_STEREO_HI 41u
#define NDS_MN_UI_KIT_SURFACE_OPTION_SOUND_STEREO 42u
#define NDS_MN_UI_KIT_SURFACE_OPTION_SOUND_MONO_HI 43u
#define NDS_MN_UI_KIT_SURFACE_OPTION_SOUND_MONO 44u
#define NDS_MN_UI_KIT_SURFACE_OPTION_SCREEN_ADJUST_HI 45u
#define NDS_MN_UI_KIT_SURFACE_OPTION_SCREEN_ADJUST 46u
#define NDS_MN_UI_KIT_SURFACE_OPTION_BACKUP_CLEAR_HI 47u
#define NDS_MN_UI_KIT_SURFACE_OPTION_BACKUP_CLEAR 48u
'''

BACKUP_PREAMBLE = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define FALSE 0
#define TRUE 1
typedef uint8_t u8;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef u16 NdsUiKitSurfaceId;
typedef void alSoundEffect;
#define NDS_INPUT_LEFT (1u << 0)
#define NDS_INPUT_RIGHT (1u << 1)
#define NDS_INPUT_UP (1u << 2)
#define NDS_INPUT_DOWN (1u << 3)
#define NDS_INPUT_A (1u << 4)
#define NDS_INPUT_START (1u << 5)
#define NDS_INPUT_B (1u << 6)
#define NDS_UI_KIT_SFX_MOVE 0u
#define NDS_UI_KIT_SFX_CONFIRM 1u
#define NDS_UI_KIT_SFX_VALUE 3u
enum { nSCKindTitle = 0, nSCKindModeSelect = 1, nSCKindOption = 2, nSCKindBackupClear = 3, nSCKindScreenAdjust = 4 };
#define nSYAudioFGMOptionBackupClear 504u
#define nSYAudioFGMMenuSelect 501u
#define nSYAudioFGMMenuScroll2 503u
/* -- scratch globals owned by the harness TU -- */
u32 sMenuBackupCursor, sMenuBackupMenuKind, sMenuBackupYesOrNo, sMenuBackupWait, sMenuBackupApplyTics;
u32 sMenuBackupAppliedConfirmKind;
NdsUiKitSurfaceId sMenuBackupRowSurface[6];
NdsUiKitSurfaceId sMenuBackupConfirmSurface;
u32 gNdsMenuShellBackupBlitCount, gNdsMenuShellBackupApplyCount, gNdsMenuShellBackupApplyTarget;
/* -- blit stub with injectable failure -- */
static u32 blit_attempts, blit_fail_at, blit_log_n, plate_blits;
static NdsUiKitSurfaceId blit_log[128];
static s32 ndsUiKitBlitSurfaces(const NdsUiKitSurfaceId *s, u32 count)
{
    CHECK(s != 0 && count == 1u);
    blit_attempts++;
    if (blit_attempts == blit_fail_at) return 0;
    if (blit_log_n < 128) blit_log[blit_log_n++] = s[0];
    if (s[0] == 20u) plate_blits++;
    return 1;
}
static u32 sfx_total, sfx_last;
static void ndsUiKitSfx(u32 cue) { sfx_total++; sfx_last = cue; }
static u32 ndsMenuShellDirection(u32 held, u32 taps, u32 mask)
{
    (void)held;
    return (((taps & mask) != 0u) ? TRUE : FALSE);
}
static u32 goto_calls, goto_last;
static void ndsMenuShellGoto(u32 kind) { goto_calls++; goto_last = kind; }
static u32 rec_order[32], rec_order_n, rec_writes, fgm_calls, fgm_last;
static void note(u32 t) { if (rec_order_n < 32) rec_order[rec_order_n++] = t; }
static void lbBackupClearNewcomers(void) { note(1); }
static void lbBackupClear1PHighScore(void) { note(2); }
static void lbBackupClearBonusStageTime(void) { note(3); }
static void lbBackupClearVSRecord(void) { note(4); }
static void lbBackupClearPrize(void) { note(5); }
static void lbBackupClearAllData(void) { note(6); }
static void lbBackupApplyOptions(void) { note(7); }
static void lbBackupCorrectErrors(void) { note(8); }
static void lbBackupWrite(void) { rec_writes++; note(9); }
static alSoundEffect *ndsAudioFgmPlay(u16 id) { fgm_calls++; fgm_last = id; return 0; }
'''

OPTION_PREAMBLE = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define FALSE 0
#define TRUE 1
typedef uint8_t u8;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef u16 NdsUiKitSurfaceId;
#define NDS_INPUT_LEFT (1u << 0)
#define NDS_INPUT_RIGHT (1u << 1)
#define NDS_INPUT_UP (1u << 2)
#define NDS_INPUT_DOWN (1u << 3)
#define NDS_INPUT_A (1u << 4)
#define NDS_INPUT_START (1u << 5)
#define NDS_INPUT_B (1u << 6)
#define NDS_UI_KIT_SFX_MOVE 0u
#define NDS_UI_KIT_SFX_CONFIRM 1u
#define NDS_UI_KIT_SFX_VALUE 3u
enum { nSCKindTitle = 0, nSCKindModeSelect = 1, nSCKindOption = 2, nSCKindBackupClear = 3, nSCKindScreenAdjust = 4 };
u32 sMenuOptionCursor;
u8 sMenuOptionSound, sMenuOptionFlash;
NdsUiKitSurfaceId sMenuOptionRowSurface[3];
u32 gNdsMenuShellOptionBlitCount, gNdsMenuShellOptionCommitCount;
struct { u8 scene_prev, scene_curr; } gSCManagerSceneData;
struct { int is_allow_screenflash, sound_mono_or_stereo; } gSCManagerBackupData;
static s32 dSYAudioSoundQuality = 1;
static u32 blit_attempts, blit_fail_at, blit_log_n;
static s32 ndsUiKitBlitSurfaces(const NdsUiKitSurfaceId *s, u32 count)
{
    CHECK(s != 0 && count == 1u);
    blit_attempts++;
    if (blit_attempts == blit_fail_at) return 0;
    if (1) { (void)s; }
    blit_log_n++;
    return 1;
}
static u32 sfx_total, sfx_last;
static void ndsUiKitSfx(u32 cue) { sfx_total++; sfx_last = cue; }
static u32 ndsMenuShellDirection(u32 held, u32 taps, u32 mask)
{
    (void)held;
    return (((taps & mask) != 0u) ? TRUE : FALSE);
}
static u32 goto_calls, goto_last;
static void ndsMenuShellGoto(u32 kind) { goto_calls++; goto_last = kind; }
static u32 quality_calls, quality_last;
static void syAudioSetQuality(s32 q) { dSYAudioSoundQuality = q; quality_calls++; quality_last = (u32)q; }
static u32 rec_writes;
static void lbBackupWrite(void) { rec_writes++; }
'''

BACKUP_FUNCS = (
    "ndsMenuShellBackupWantRow",
    "ndsMenuShellBackupWantConfirm",
    "ndsMenuShellBackupSyncRows",
    "ndsMenuShellBackupSyncConfirm",
    "ndsMenuShellBackupClearLoad",
    "ndsMenuShellPopulateBackupClear",
    "ndsMenuShellBackupApply",
    "ndsMenuShellBackupOpenConfirm",
    "ndsMenuShellBackupCancel",
    "ndsMenuShellUpdateBackupMain",
    "ndsMenuShellUpdateBackupConfirm",
    "ndsMenuShellUpdateBackupClear",
)

OPTION_FUNCS = (
    "ndsMenuShellOptionWantSurface",
    "ndsMenuShellOptionSyncRows",
    "ndsMenuShellOptionLoad",
    "ndsMenuShellOptionWriteBackup",
    "ndsMenuShellPopulateOption",
    "ndsMenuShellOptionSoundLeft",
    "ndsMenuShellOptionSoundRight",
    "ndsMenuShellOptionSoundFlip",
    "ndsMenuShellUpdateOption",
)


def backup_program(native_text, main):
    defines = native_defines(native_text, "NDS_MENU_BACKUP_")
    bodies = "\n".join(function(native_text, name) for name in BACKUP_FUNCS)
    plate = "static const NdsUiKitSurfaceId kNdsMenuBackupPlate[] = { NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR };\n"
    return BACKUP_PREAMBLE + BACKUP_SURFACES + defines + plate + bodies + main


def option_program_native(native_text, main):
    defines = native_defines(native_text, "NDS_MENU_OPTION_")
    bodies = "\n".join(function(native_text, name) for name in OPTION_FUNCS)
    plate = "static const NdsUiKitSurfaceId kNdsMenuOptionPlate[] = { NDS_MN_UI_KIT_SURFACE_OPTION };\n"
    return OPTION_PREAMBLE + OPTION_SURFACES + defines + plate + bodies + main


BACKUP_FLOW_MAIN = r'''
static void frame(u32 tap) { ndsMenuShellUpdateBackupClear(0u, tap); }
static void idle(unsigned n) { unsigned i; for (i = 0; i < n; i++) frame(0u); }
int main(void)
{
    /* Entry: cursor Newcomers, main menu, NO, 10-tic gate. */
    ndsMenuShellBackupClearLoad();
    CHECK(sMenuBackupCursor == NDS_MENU_BACKUP_NEWCOMERS);
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_MAIN);
    CHECK(sMenuBackupYesOrNo == NDS_MENU_BACKUP_NO);
    CHECK(sMenuBackupWait == NDS_MENU_BACKUP_ENTRY_WAIT);
    CHECK(NDS_MENU_BACKUP_ENTRY_WAIT == 10u);
    CHECK(sMenuBackupApplyTics == 0u);
    /* Ten-tic gate eats A; eleventh opens kind-1 confirm defaulting to NO. */
    { unsigned i; for (i = 0; i < 9; i++) frame(NDS_INPUT_A); }
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_MAIN);
    frame(NDS_INPUT_A);
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_MAIN);
    frame(NDS_INPUT_A);
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_CONFIRM1);
    CHECK(sMenuBackupYesOrNo == NDS_MENU_BACKUP_NO);
    /* Navigation wrap on the main menu (fresh load to leave the gate). */
    ndsMenuShellBackupClearLoad();
    idle(11);
    frame(NDS_INPUT_UP);
    CHECK(sMenuBackupCursor == (NDS_MENU_BACKUP_ROWS - 1u));
    frame(NDS_INPUT_DOWN);
    CHECK(sMenuBackupCursor == NDS_MENU_BACKUP_NEWCOMERS);
    /* Cancel paths: A on NO cancels, B cancels. */
    ndsMenuShellBackupClearLoad();
    idle(11);
    frame(NDS_INPUT_A);
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_CONFIRM1);
    idle(11);
    frame(NDS_INPUT_A);
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_MAIN);
    CHECK(sMenuBackupWait == NDS_MENU_BACKUP_ENTRY_WAIT);
    idle(11);
    frame(NDS_INPUT_A);
    idle(11);
    frame(NDS_INPUT_B);
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_MAIN);
    /* R->YES, L->NO inside the confirm. */
    ndsMenuShellBackupClearLoad();
    idle(11);
    frame(NDS_INPUT_A);
    idle(11);
    frame(NDS_INPUT_RIGHT);
    CHECK(sMenuBackupYesOrNo == NDS_MENU_BACKUP_YES);
    frame(NDS_INPUT_LEFT);
    CHECK(sMenuBackupYesOrNo == NDS_MENU_BACKUP_NO);
    /* Apply Newcomers: clear->correct->write order plus the cue. */
    frame(NDS_INPUT_RIGHT);
    CHECK(sMenuBackupYesOrNo == NDS_MENU_BACKUP_YES);
    idle(1);
    rec_order_n = 0; rec_writes = 0; fgm_calls = 0;
    frame(NDS_INPUT_A);
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_MAIN);
    CHECK(sMenuBackupApplyTics == NDS_MENU_BACKUP_APPLY_TICS);
    CHECK(NDS_MENU_BACKUP_APPLY_TICS == 60u);
    CHECK(rec_order_n == 3);
    CHECK(rec_order[0] == 1);
    CHECK(rec_order[1] == 8);
    CHECK(rec_order[2] == 9);
    CHECK(rec_writes == 1);
    CHECK(fgm_calls == 1 && fgm_last == nSYAudioFGMOptionBackupClear);
    /* 60-tic flash runs, then input works again. */
    { unsigned i; for (i = 0; i < 60; i++) frame(0u); }
    CHECK(sMenuBackupApplyTics == 0u);
    /* All Data Clear needs the second confirm defaulting to NO. */
    { int i; for (i = 0; i < 5; i++) { frame(NDS_INPUT_DOWN); } }
    CHECK(sMenuBackupCursor == (NDS_MENU_BACKUP_ROWS - 1u));
    frame(NDS_INPUT_A);
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_CONFIRM1);
    idle(11);
    frame(NDS_INPUT_RIGHT);
    idle(1);
    rec_order_n = 0; rec_writes = 0;
    frame(NDS_INPUT_A);
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_CONFIRM2);
    CHECK(sMenuBackupYesOrNo == NDS_MENU_BACKUP_NO);
    CHECK(rec_order_n == 0);
    idle(11);
    frame(NDS_INPUT_RIGHT);
    idle(1);
    frame(NDS_INPUT_A);
    CHECK(rec_order_n == 4);
    CHECK(rec_order[0] == 6);
    CHECK(rec_order[1] == 7);
    CHECK(rec_order[2] == 8);
    CHECK(rec_order[3] == 9);
    { unsigned i; for (i = 0; i < 60; i++) frame(0u); }
    /* B from the main menu returns to Options. */
    frame(NDS_INPUT_B);
    CHECK(goto_calls == 1 && goto_last == (u32)nSCKindOption);
    return 0;
}
'''

BACKUP_BLIT_MAIN = r'''
int main(void)
{
    /* Zero budget blits nothing; idle blits nothing. */
    ndsMenuShellBackupClearLoad();
    blit_attempts = 0; blit_fail_at = 0; blit_log_n = 0;
    gNdsMenuShellBackupBlitCount = 0;
    ndsMenuShellBackupSyncRows(0u);
    CHECK(blit_attempts == 0 && gNdsMenuShellBackupBlitCount == 0);
    ndsMenuShellBackupSyncRows(NDS_MENU_BACKUP_ROWS);
    CHECK(gNdsMenuShellBackupBlitCount == NDS_MENU_BACKUP_ROWS);
    { u32 before = blit_attempts; ndsMenuShellBackupSyncRows(NDS_MENU_BACKUP_ROWS); CHECK(blit_attempts == before); }
    /* One budget blits at most one row. */
    ndsMenuShellBackupClearLoad();
    gNdsMenuShellBackupBlitCount = 0; blit_attempts = 0; blit_log_n = 0;
    sMenuBackupCursor = (NDS_MENU_BACKUP_ROWS - 1u);
    { unsigned i; for (i = 0; i < 6u; i++) sMenuBackupRowSurface[i] = ndsMenuShellBackupWantRow(i); }
    sMenuBackupCursor = NDS_MENU_BACKUP_NEWCOMERS;
    ndsMenuShellBackupSyncRows(1u);
    CHECK(gNdsMenuShellBackupBlitCount == 1);
    /* Failed row blit keeps the byte cache dirty so the next call retries. */
    ndsMenuShellBackupClearLoad();
    gNdsMenuShellBackupBlitCount = 0; blit_attempts = 0; blit_fail_at = 1; blit_log_n = 0;
    ndsMenuShellBackupSyncRows(NDS_MENU_BACKUP_ROWS);
    CHECK(gNdsMenuShellBackupBlitCount == 0);
    { unsigned i; for (i = 0; i < 6u; i++) CHECK(sMenuBackupRowSurface[i] == NDS_MENU_VS_SURFACE_NONE); }
    blit_fail_at = 0;
    ndsMenuShellBackupSyncRows(NDS_MENU_BACKUP_ROWS);
    CHECK(gNdsMenuShellBackupBlitCount == NDS_MENU_BACKUP_ROWS);
    /* Failed confirm blit retries instead of sticking the dialog as drawn. */
    ndsMenuShellBackupClearLoad();
    { unsigned i; for (i = 0; i < 11u; i++) ndsMenuShellUpdateBackupClear(0u, 0u); }
    ndsMenuShellUpdateBackupClear(0u, NDS_INPUT_A);
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_CONFIRM1);
    CHECK(sMenuBackupConfirmSurface != NDS_MENU_VS_SURFACE_NONE);
    gNdsMenuShellBackupBlitCount = 0; blit_attempts = 0;
    sMenuBackupConfirmSurface = NDS_MENU_VS_SURFACE_NONE;
    blit_fail_at = 1;
    ndsMenuShellBackupSyncConfirm();
    CHECK(gNdsMenuShellBackupBlitCount == 0);
    CHECK(sMenuBackupConfirmSurface == NDS_MENU_VS_SURFACE_NONE);
    blit_fail_at = 0;
    ndsMenuShellBackupSyncConfirm();
    CHECK(sMenuBackupConfirmSurface != NDS_MENU_VS_SURFACE_NONE);
    CHECK(gNdsMenuShellBackupBlitCount == 1);
    /* Dismissing the dialog repaints the full backing plate, not just rows. */
    plate_blits = 0; blit_log_n = 0;
    ndsMenuShellBackupCancel();
    CHECK(sMenuBackupMenuKind == NDS_MENU_BACKUP_MENU_MAIN);
    CHECK(plate_blits >= 1);
    return 0;
}
'''

OPTION_FLOW_MAIN = r'''
static void frame(u32 tap) { ndsMenuShellUpdateOption(0u, tap); }
int main(void)
{
    /* Entry cursor from scene_prev; sound from mixer; flash preserved. */
    dSYAudioSoundQuality = 1;
    gSCManagerBackupData.is_allow_screenflash = 1;
    gSCManagerSceneData.scene_prev = (u8)nSCKindModeSelect;
    ndsMenuShellOptionLoad();
    CHECK(sMenuOptionCursor == NDS_MENU_OPTION_SOUND);
    CHECK(sMenuOptionSound == 1u);
    CHECK(sMenuOptionFlash == 1u);
    gSCManagerSceneData.scene_prev = (u8)nSCKindScreenAdjust;
    ndsMenuShellOptionLoad();
    CHECK(sMenuOptionCursor == NDS_MENU_OPTION_SCREEN_ADJUST);
    gSCManagerSceneData.scene_prev = (u8)nSCKindBackupClear;
    ndsMenuShellOptionLoad();
    CHECK(sMenuOptionCursor == NDS_MENU_OPTION_BACKUP_CLEAR);
    gSCManagerSceneData.scene_prev = (u8)nSCKindModeSelect;
    dSYAudioSoundQuality = 0;
    ndsMenuShellOptionLoad();
    CHECK(sMenuOptionSound == 0u);
    /* Wrap both ends. */
    frame(NDS_INPUT_UP);
    CHECK(sMenuOptionCursor == NDS_MENU_OPTION_BACKUP_CLEAR);
    frame(NDS_INPUT_DOWN);
    CHECK(sMenuOptionCursor == NDS_MENU_OPTION_SOUND);
    /* Sound toggle: L stereo, R mono, A flip; cue plus mixer; no save write. */
    rec_writes = 0; quality_calls = 0;
    frame(NDS_INPUT_LEFT);
    CHECK(sMenuOptionSound == 1u && quality_calls == 1 && quality_last == 1u && sfx_last == NDS_UI_KIT_SFX_VALUE);
    CHECK(rec_writes == 0);
    frame(NDS_INPUT_RIGHT);
    CHECK(sMenuOptionSound == 0u && quality_calls == 2 && quality_last == 0u);
    CHECK(rec_writes == 0);
    frame(NDS_INPUT_A);
    CHECK(sMenuOptionSound == 1u && quality_calls == 3);
    CHECK(rec_writes == 0);
    /* Write path is flash plus mono/stereo, then the save write. */
    sMenuOptionFlash = 1u; sMenuOptionSound = 1u;
    gSCManagerBackupData.is_allow_screenflash = 0;
    gSCManagerBackupData.sound_mono_or_stereo = 0;
    rec_writes = 0;
    ndsMenuShellOptionWriteBackup();
    CHECK(gSCManagerBackupData.is_allow_screenflash == 1);
    CHECK(gSCManagerBackupData.sound_mono_or_stereo == 1);
    CHECK(rec_writes == 1);
    /* A on Backup Clear writes and routes there; B writes and goes to Mode Select. */
    ndsMenuShellOptionLoad();
    frame(NDS_INPUT_DOWN); frame(NDS_INPUT_DOWN);
    CHECK(sMenuOptionCursor == NDS_MENU_OPTION_BACKUP_CLEAR);
    rec_writes = 0; goto_calls = 0;
    frame(NDS_INPUT_A);
    CHECK(rec_writes == 1 && goto_calls == 1 && goto_last == (u32)nSCKindBackupClear);
    goto_calls = 0; rec_writes = 0;
    frame(NDS_INPUT_B);
    CHECK(rec_writes == 1 && goto_calls == 1 && goto_last == (u32)nSCKindModeSelect);
    /* Screen Adjust activation is harmless: no shared write, no route,
       never a backup-clear side effect and never a stuck screen. */
    ndsMenuShellOptionLoad();
    frame(NDS_INPUT_DOWN);
    CHECK(sMenuOptionCursor == NDS_MENU_OPTION_SCREEN_ADJUST);
    { u32 writes_before = rec_writes; u32 goes_before = goto_calls;
      frame(NDS_INPUT_A);
      CHECK(rec_writes == writes_before);
      CHECK(goto_calls == goes_before); }
    return 0;
}
'''

OPTION_BLIT_MAIN = r'''
int main(void)
{
    gSCManagerSceneData.scene_prev = (u8)nSCKindModeSelect;
    dSYAudioSoundQuality = 1;
    gSCManagerBackupData.is_allow_screenflash = 1;
    ndsMenuShellOptionLoad();
    blit_attempts = 0; blit_fail_at = 0;
    gNdsMenuShellOptionBlitCount = 0;
    ndsMenuShellOptionSyncRows(0u);
    CHECK(blit_attempts == 0 && gNdsMenuShellOptionBlitCount == 0);
    ndsMenuShellOptionSyncRows(NDS_MENU_OPTION_ROWS);
    CHECK(gNdsMenuShellOptionBlitCount == NDS_MENU_OPTION_ROWS);
    { u32 before = blit_attempts; ndsMenuShellOptionSyncRows(NDS_MENU_OPTION_ROWS); CHECK(blit_attempts == before); }
    sMenuOptionCursor = NDS_MENU_OPTION_BACKUP_CLEAR;
    { unsigned i; for (i = 0; i < 3u; i++) sMenuOptionRowSurface[i] = ndsMenuShellOptionWantSurface(i); }
    sMenuOptionCursor = NDS_MENU_OPTION_SOUND;
    gNdsMenuShellOptionBlitCount = 0; blit_attempts = 0;
    ndsMenuShellOptionSyncRows(1u);
    CHECK(gNdsMenuShellOptionBlitCount == 1);
    ndsMenuShellOptionLoad();
    gNdsMenuShellOptionBlitCount = 0; blit_attempts = 0; blit_fail_at = 1;
    ndsMenuShellOptionSyncRows(NDS_MENU_OPTION_ROWS);
    CHECK(gNdsMenuShellOptionBlitCount == 0);
    { unsigned i; for (i = 0; i < 3u; i++) CHECK(sMenuOptionRowSurface[i] == NDS_MENU_VS_SURFACE_NONE); }
    blit_fail_at = 0;
    ndsMenuShellOptionSyncRows(NDS_MENU_OPTION_ROWS);
    CHECK(gNdsMenuShellOptionBlitCount == NDS_MENU_OPTION_ROWS);
    return 0;
}
'''


class NativeOptionsContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = next((shutil.which(c) for c in
                             ("clang", "gcc", "cc") if shutil.which(c)), None)

    def test_source_backup_confirm_vectors_on_test_data(self):
        """Run the ACTUAL source confirm flow on test data.

        Uses the same harness as test_options_reentry (real InitVars,
        real main/confirm menus, real FuncRun, real ApplyOptionID with
        recorder clears): entry cursor Newcomers, UpdateWait 10 eats ten
        tics, eleventh A opens kind-1 confirm defaulting to NO, R moves
        to YES, A applies Newcomers with clear->correct->write order and
        the BackupClear cue, AllDataClear needs the second confirm, B
        returns to Option, re-entry resets the cursor.
        """
        self.assertIsNotNone(self.compiler, "Host C compiler required")
        source = (DECOMP / "mn/mnoption/mnbackupclear.c").read_text()
        text = reentry.backupclear_program(
            function(source, "mnBackupClearInitVars"),
            function(source, "mnBackupClearUpdateOptionMainMenu"),
            function(source, "mnBackupClearUpdateOptionConfirmMenu"),
            function(source, "mnBackupClearFuncRun"),
            function(source, "mnBackupClearApplyOptionID"))
        with tempfile.TemporaryDirectory() as directory:
            result = compile_and_run(self.compiler, Path(directory),
                                     "native_opt_source_vectors", text)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_source_option_entry_vectors_on_test_data(self):
        """Run the ACTUAL Options entry/route flow on test data."""
        self.assertIsNotNone(self.compiler, "Host C compiler required")
        source = (DECOMP / "mn/mnoption/mnoption.c").read_text()
        text = reentry.option_program(
            function(source, "mnOptionInitVars"),
            function(source, "mnOptionFuncRun"))
        with tempfile.TemporaryDirectory() as directory:
            result = compile_and_run(self.compiler, Path(directory),
                                     "native_opt_source_option", text)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_native_backup_flow_matches_source(self):
        """Execute the ACTUAL native Backup Clear on scratch state."""
        self.assertIsNotNone(self.compiler, "Host C compiler required")
        native_text = BACKUP_C.read_text()
        text = backup_program(native_text, BACKUP_FLOW_MAIN)
        with tempfile.TemporaryDirectory() as directory:
            result = compile_and_run(self.compiler, Path(directory),
                                     "native_backup_flow", text)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_native_backup_blit_retry_and_plate(self):
        """Execute native Backup row/confirm blit accounting on scratch."""
        self.assertIsNotNone(self.compiler, "Host C compiler required")
        native_text = BACKUP_C.read_text()
        text = backup_program(native_text, BACKUP_BLIT_MAIN)
        with tempfile.TemporaryDirectory() as directory:
            result = compile_and_run(self.compiler, Path(directory),
                                     "native_backup_blit", text)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_native_option_flow_matches_source(self):
        """Execute the ACTUAL native Options on scratch state."""
        self.assertIsNotNone(self.compiler, "Host C compiler required")
        native_text = OPTION_C.read_text()
        text = option_program_native(native_text, OPTION_FLOW_MAIN)
        with tempfile.TemporaryDirectory() as directory:
            result = compile_and_run(self.compiler, Path(directory),
                                     "native_option_flow", text)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_native_option_blit_retry(self):
        """Execute native Options row budget/retry accounting on scratch."""
        self.assertIsNotNone(self.compiler, "Host C compiler required")
        native_text = OPTION_C.read_text()
        text = option_program_native(native_text, OPTION_BLIT_MAIN)
        with tempfile.TemporaryDirectory() as directory:
            result = compile_and_run(self.compiler, Path(directory),
                                     "native_option_blit", text)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_generator_carries_source_artwork(self):
        """The kit bake carries the source Options/Backup artwork."""
        gen = GENERATOR.read_text()
        for token in ("OPTION_SURFACE_SPECS", "BACKUP_CLEAR_SURFACE_SPECS",
                      "OPTION_BACKGROUND", "BACKUP_CLEAR_BACKGROUND",
                      "llMNOptionSoundTextSprite",
                      "llMNOptionScreenAdjustTextSprite",
                      "llMNOptionBackupClearTextSprite",
                      "llMNOptionStereoTextSprite",
                      "llMNOptionMonoTextSprite",
                      "llMNBackupClearOptionNewcomersSprite",
                      "llMNBackupClearOptionAllDataClearSprite",
                      "llMNBackupClearOptionYesSprite",
                      "llMNBackupClearOptionNoSprite",
                      "llMNBackupClearOptionCircleSprite",
                      "llMNBackupClearAreYouSureTextSprite",
                      "llMNBackupClearIsOkayTextSprite",
                      "llMNBackupClearHeaderBackupClearSprite"):
            self.assertIn(token, gen, token)
        # Appended after VS options so no pre-existing id moves.
        self.assertLess(gen.index("VS_OPTIONS_SURFACE_SPECS"),
                        gen.index("OPTION_SURFACE_SPECS"))
        self.assertLess(gen.index("OPTION_SURFACE_SPECS"),
                        gen.index("BACKUP_CLEAR_SURFACE_SPECS"))
        # The native screens reference exactly these baked surfaces.
        option_text = OPTION_C.read_text()
        backup_text = BACKUP_C.read_text()
        for token in ("NDS_MN_UI_KIT_SURFACE_OPTION",
                      "NDS_MN_UI_KIT_SURFACE_OPTION_SOUND_STEREO",
                      "NDS_MN_UI_KIT_SURFACE_OPTION_SCREEN_ADJUST_HI",
                      "NDS_MN_UI_KIT_SURFACE_OPTION_BACKUP_CLEAR_HI"):
            self.assertIn(token, option_text, token)
        for token in ("NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR",
                      "NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_NEWCOMERS_HI",
                      "NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_ALL_DATA_HI",
                      "NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM1_YES",
                      "NDS_MN_UI_KIT_SURFACE_BACKUP_CLEAR_CONFIRM2_NO"):
            self.assertIn(token, backup_text, token)


if __name__ == "__main__":
    unittest.main()
