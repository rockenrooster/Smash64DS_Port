"""Options / Sound Test / Backup Clear source routines across repeated entries.

The N64 reloads each menu overlay's BSS before every scManagerRunLoop dispatch
(scmanager.c loads the overlay, then calls the scene's StartScene), so every
entry begins from zeroed scene state. The DS keeps these translation units
live (src/import/battleship_mnoption.c, battleship_mnsoundtest.c,
battleship_mnbackupclear.c are whole-TU includes), so re-entry state must be
restored by each scene's own InitVars. This locks that contract -- plus the
return routes, the write path and the audio stop/play arms -- against the
actual decomp routines on a host compiler, with the wrapper adapters asserted
to stay verbatim pass-throughs.

Each scene builds one host program whose globals are never re-zeroed between
entries (the DS retention model), then runs a "broken" variant with one
InitVars line stripped; the stripped program must fail, proving that reset is
load-bearing rather than decorative.
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

WRAPPER_ADAPTERS = {
    # path: (scene entry, base entry, decomp TU, generated overlay TU or
    # None). Backup Clear rides the generated import overlay because the
    # port C standard rejects the source implicit-int no-op
    # func_ovl53_801325CC; the overlay patch carries exactly that one-line
    # conformance fix and nothing else.
    "src/import/battleship_mnoption.c":
        ("mnOptionStartScene", "ndsBaseMNOptionStartScene",
         "mn/mnoption/mnoption.c", None),
    "src/import/battleship_mnsoundtest.c":
        ("mnSoundTestStartScene", "ndsBaseMNSoundTestStartScene",
         "mn/mndata/mnsoundtest.c", None),
    "src/import/battleship_mnbackupclear.c":
        ("mnBackupClearStartScene", "ndsBaseMNBackupClearStartScene",
         "mn/mnoption/mnbackupclear.c",
         "battleship_overlay/src/mn/mnoption/mnbackupclear.c"),
}

OVERLAY_PATCH = (ROOT / "scripts/import-overlays/battleship"
                 / "src_mn_mnoption_mnbackupclear.patch")


def macro_block(path, first_define, end_marker):
    text = (DECOMP / path).read_text()
    start = text.index(first_define)
    end = text.index(end_marker, start)
    return text[start:end]


def scene_macros():
    """mndef.h shared option-input macros plus the three per-scene bindings."""
    common = macro_block("mn/mndef.h",
                         "#define mnCommonCheckGetOptionButtonInput",
                         "typedef enum MNCharactersMotionKind")
    blocks = [common]
    for path, first, end in (
        ("mn/mnoption/mnoption.c", "#define mnOptionCheckGetOptionButtonInput",
         "EXTERNAL VARIABLES"),
        ("mn/mndata/mnsoundtest.c", "#define mnSoundTestCheckGetOptionButtonInput",
         "INITIALIZED DATA"),
        ("mn/mnoption/mnbackupclear.c", "#define mnBackupClearCheckGetOptionButtonInput",
         "INITIALIZED DATA"),
    ):
        blocks.append(macro_block(path, first, end))
    return "\n".join(blocks) + "\n"


PREAMBLE = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define FALSE 0
#define TRUE 1
typedef int8_t s8;
typedef uint8_t u8;
typedef int32_t s32;
typedef uint32_t u32;
typedef uint16_t u16;
typedef float f32;
typedef int sb32;
typedef void GObj;
#define ARRAY_COUNT(x) ((int)(sizeof(x) / sizeof((x)[0])))
/* include/macros.h: UPDATE_INTERVAL 60, so one minute is 3600 tics. */
#define I_MIN_TO_TICS(q) ((q) * 3600)
/* N64 pad bits (PR/controller.h); only uniqueness matters on the host. */
#define A_BUTTON   0x8000
#define B_BUTTON   0x4000
#define Z_TRIG     0x2000
#define START_BUTTON 0x1000
#define U_JPAD 0x0800
#define D_JPAD 0x0400
#define L_JPAD 0x0200
#define R_JPAD 0x0100
#define U_CBUTTONS 0x0008
#define D_CBUTTONS 0x0004
#define L_CBUTTONS 0x0002
#define R_CBUTTONS 0x0001
#define L_TRIG 0x0020
#define R_TRIG 0x0010
/* gm/gmsound.h ordinals used by these scenes; distinct probe values. */
#define nSYAudioFGMMenuSelect 501
#define nSYAudioFGMMenuScroll1 502
#define nSYAudioFGMMenuScroll2 503
#define nSYAudioFGMOptionBackupClear 504
#define nSYAudioBGMModeSelect 10
'''


def shared_shims():
    return r'''
/* -- scripted controller (sc/scsubsys.c seam) -- */
static u32 sc_tap, sc_hold;
static s32 sc_stick_ud, sc_stick_lr;
static sb32 sc_no_input_all;
static sb32 scSubsysControllerGetPlayerTapButtons(u32 mask)
{
    return ((sc_tap & mask) != 0u) ? TRUE : FALSE;
}
static s32 scSubsysControllerGetPlayerHoldButtons(u32 mask)
{
    return ((sc_hold & mask) != 0u) ? TRUE : FALSE;
}
static s32 scSubsysControllerGetPlayerStickUD(s8 range, sb32 up_or_down)
{
    s32 value = up_or_down ? sc_stick_ud : -sc_stick_ud;
    return (value > range) ? value : 0;
}
static s32 scSubsysControllerGetPlayerStickLR(s8 range, sb32 right_or_left)
{
    s32 value = right_or_left ? sc_stick_lr : -sc_stick_lr;
    return (value > range) ? value : 0;
}
static sb32 scSubsysControllerGetPlayerStickInRangeLR(s32 l_min, s32 r_min)
{
    (void)l_min; (void)r_min;
    return ((sc_stick_lr >= -20) && (sc_stick_lr <= 20)) ? TRUE : FALSE;
}
static sb32 scSubsysControllerGetPlayerStickInRangeUD(s32 d_min, s32 u_min)
{
    (void)d_min; (void)u_min;
    return ((sc_stick_ud >= -20) && (sc_stick_ud <= 20)) ? TRUE : FALSE;
}
static sb32 scSubsysControllerCheckNoInputAll(void)
{
    return sc_no_input_all;
}
/* -- shared recorders -- */
static u32 rec_fgm_calls, rec_fgm_last;
static void *func_800269C0_275C0(u16 id)
{
    rec_fgm_calls++; rec_fgm_last = id;
    return NULL;
}
static u32 rec_load_scene_calls;
static void syTaskmanSetLoadScene(void) { rec_load_scene_calls++; }
static u32 rec_stop_all_calls, rec_play_calls, rec_play_last;
static u32 rec_set_volume_calls, rec_set_volume_last;
static u32 rec_fade_calls, rec_fade_vol, rec_fade_frames;
static void syAudioStopBGMAll(void) { rec_stop_all_calls++; }
static void syAudioPlayBGM(s32 player, u32 id)
{
    (void)player; rec_play_calls++; rec_play_last = id;
}
static void syAudioSetBGMVolume(s32 player, u32 vol)
{
    (void)player; rec_set_volume_calls++; rec_set_volume_last = vol;
}
static u32 rec_eject_calls;
static void gcEjectGObj(GObj *object) { (void)object; rec_eject_calls++; }
static struct { u8 scene_prev, scene_curr; } gSCManagerSceneData;
static struct { int is_allow_screenflash, sound_mono_or_stereo; } gSCManagerBackupData;
'''


def enums_for(names_mndef):
    parts = [original_enum("sc/scdef.h", "SCKind")]
    for tag in names_mndef:
        parts.append(original_enum("mn/mndef.h", tag))
    return "\n".join(parts) + "\n"


def option_program(init_vars, func_run):
    source = (DECOMP / "mn/mnoption/mnoption.c").read_text()
    write_backup = function(source, "mnOptionWriteBackup")
    set_colors = function(source, "mnOptionSetOptionSpriteColors")
    return (PREAMBLE
            + enums_for(("MNOptionOptions", "MNOptionTabStatus"))
            + scene_macros()
            + shared_shims()
            + r'''
/* -- scene state (retained between entries like the live DS TU) -- */
static GObj *sMNOptionOptionSoundGObj, *sMNOptionOptionScreenAdjustGObj;
static GObj *sMNOptionOptionBackupClearGObj, *sMNOptionMenuGObj;
static GObj *sMNOptionSoundOptionGObj, *D_ovl60_801337D4;
static s32 sMNOptionOption, sMNOptionOptionChangeWait, sMNOptionTotalTimeTics;
static s32 sMNOptionReturnTic, sMNOptionSoundMonoOrStereo, sMNOptionIsScreenFlash;
static sb32 sMNOptionIsProceedScene;
void mnOptionFuncRun(GObj *gobj);
sb32 dSYAudioSoundQuality = 1;
static u32 rec_quality_calls; static s32 rec_quality_last;
static void syAudioSetQuality(s32 quality)
{
    dSYAudioSoundQuality = quality;
    rec_quality_calls++; rec_quality_last = quality;
}
static u32 rec_backup_writes;
static void lbBackupWrite(void) { rec_backup_writes++; }
static u32 rec_make_menu, rec_make_toggle;
static void mnOptionMakeMenuGObj(void) { rec_make_menu++; }
static void mnOptionMakeSoundToggle(void) { rec_make_toggle++; }
/* -- minimal sprite chain so the real color routine runs -- */
typedef struct { unsigned char r, g, b; } SYColorRGB;
typedef struct { SYColorRGB prim, env; } SYColorRGBPair;
typedef struct MNFakeSObj
{
    struct MNFakeSObj *next, *prev;
    struct { unsigned char red, green, blue; } sprite;
    struct { unsigned char r, g, b; } envcolor;
} SObj;
#define SObjGetStruct(g) ((SObj *)(g))
static SObj sFakeRow[3][3];
static void option_wire_rows(void)
{
    int row, link;
    for (row = 0; row < 3; row++)
    {
        for (link = 0; link < 3; link++)
        {
            sFakeRow[row][link].next = (link < 2) ? &sFakeRow[row][link + 1] : NULL;
        }
    }
    sMNOptionOptionSoundGObj = (GObj *)&sFakeRow[0][0];
    sMNOptionOptionScreenAdjustGObj = (GObj *)&sFakeRow[1][0];
    sMNOptionOptionBackupClearGObj = (GObj *)&sFakeRow[2][0];
}
''' + init_vars + r'''
static void option_entry(unsigned char prev)
{
    gSCManagerSceneData.scene_prev = prev;
    gSCManagerSceneData.scene_curr = nSCKindOption;
    mnOptionInitVars();
}
static void option_frame(u32 tap, u32 hold, sb32 no_input)
{
    sc_tap = tap; sc_hold = hold;
    sc_stick_ud = sc_stick_lr = 0;
    sc_no_input_all = no_input;
    mnOptionFuncRun(NULL);
}
''' + write_backup + "\n" + set_colors + "\n" + func_run + r'''
int main(void)
{
    /* Entry 1: from Mode Select. Ten-tic input gate, then A toggles the
     * mono/stereo row and L returns it -- the source's own semantics. */
    option_wire_rows();
    gSCManagerBackupData.is_allow_screenflash = 1;
    gSCManagerBackupData.sound_mono_or_stereo = 0;
    option_entry(nSCKindModeSelect);
    CHECK(sMNOptionOption == nMNOptionOptionSound);
    CHECK(sMNOptionIsProceedScene == FALSE);
    CHECK(sMNOptionReturnTic == I_MIN_TO_TICS(5));
    option_frame(A_BUTTON, 0, FALSE); /* tic 1.. */
    option_frame(A_BUTTON, 0, FALSE);
    option_frame(A_BUTTON, 0, FALSE);
    option_frame(A_BUTTON, 0, FALSE);
    option_frame(A_BUTTON, 0, FALSE);
    option_frame(A_BUTTON, 0, FALSE);
    option_frame(A_BUTTON, 0, FALSE);
    option_frame(A_BUTTON, 0, FALSE);
    option_frame(A_BUTTON, 0, FALSE);
    CHECK(sMNOptionSoundMonoOrStereo == 1); /* gate held through tic 9 */
    CHECK(rec_quality_calls == 0);
    option_frame(A_BUTTON, 0, FALSE); /* tic 10: toggle to mono */
    CHECK(sMNOptionSoundMonoOrStereo == 0);
    CHECK(rec_quality_calls == 1 && rec_quality_last == 0);
    CHECK(rec_backup_writes == 0); /* toggling never writes the save */
    option_frame(L_TRIG, 0, FALSE); /* L arm reads taps: back to stereo */
    CHECK(sMNOptionSoundMonoOrStereo == 1);
    CHECK(rec_quality_calls == 2 && rec_quality_last == 1);
    option_frame(0, 0, FALSE);
    /* Row walk: one D press moves to Screen Adjust and arms the 12-tic
     * change wait; a held D does not fire again until it expires. */
    option_frame(0, D_JPAD, FALSE);
    CHECK(sMNOptionOption == nMNOptionOptionScreenAdjust);
    CHECK(rec_fgm_last == nSYAudioFGMMenuScroll2);
    option_frame(0, D_JPAD, FALSE);
    CHECK(sMNOptionOption == nMNOptionOptionScreenAdjust);
    { int i; for (i = 0; i < 13; i++) option_frame(0, 0, FALSE); }
    option_frame(0, D_JPAD, FALSE);
    CHECK(sMNOptionOption == nMNOptionOptionBackupClear);
    option_frame(0, D_JPAD, FALSE);
    { int i; for (i = 0; i < 13; i++) option_frame(0, 0, FALSE); }
    option_frame(0, D_JPAD, FALSE); /* wraps to Sound */
    CHECK(sMNOptionOption == nMNOptionOptionSound);
    option_frame(0, 0, FALSE);
    /* A on Screen Adjust writes the save, then reroutes one frame later. */
    { int i; for (i = 0; i < 12; i++) option_frame(0, 0, FALSE); }
    option_frame(0, D_JPAD, FALSE);
    CHECK(sMNOptionOption == nMNOptionOptionScreenAdjust);
    option_frame(A_BUTTON, 0, FALSE);
    CHECK(rec_backup_writes == 1);
    CHECK(gSCManagerBackupData.is_allow_screenflash == 1);
    CHECK(gSCManagerBackupData.sound_mono_or_stereo == 1);
    CHECK(gSCManagerSceneData.scene_prev == nSCKindOption);
    CHECK(gSCManagerSceneData.scene_curr == nSCKindScreenAdjust);
    CHECK(sMNOptionIsProceedScene == TRUE);
    CHECK(rec_load_scene_calls == 0);
    option_frame(0, 0, FALSE);
    CHECK(rec_load_scene_calls == 1);
    /* Entry 2: back from Screen Adjust -- cursor and mixer quality restore. */
    dSYAudioSoundQuality = 0;
    option_entry(nSCKindScreenAdjust);
    CHECK(sMNOptionOption == nMNOptionOptionScreenAdjust);
    CHECK(sMNOptionSoundMonoOrStereo == 0);
    { int i; for (i = 0; i < 10; i++) option_frame(0, 0, FALSE); }
    CHECK(rec_load_scene_calls == 1); /* proceed flag reset: no spurious hop */
    /* Entry 3: back from Backup Clear -- the third row restores too. */
    option_entry(nSCKindBackupClear);
    CHECK(sMNOptionOption == nMNOptionOptionBackupClear);
    /* B writes the save and returns to Mode Select immediately. */
    { int i; for (i = 0; i < 10; i++) option_frame(0, 0, FALSE); }
    option_frame(B_BUTTON, 0, FALSE);
    CHECK(rec_backup_writes == 2);
    CHECK(gSCManagerSceneData.scene_prev == nSCKindOption);
    CHECK(gSCManagerSceneData.scene_curr == nSCKindModeSelect);
    CHECK(rec_load_scene_calls == 2);
    /* Entry 4: five idle minutes return to Title with one save write. */
    option_entry(nSCKindModeSelect);
    { int i; for (i = 0; i < 17999; i++) option_frame(0, 0, TRUE); }
    CHECK(rec_load_scene_calls == 2);
    CHECK(rec_backup_writes == 2);
    option_frame(0, 0, TRUE); /* tic 18000 */
    CHECK(gSCManagerSceneData.scene_curr == nSCKindTitle);
    CHECK(rec_backup_writes == 3);
    CHECK(rec_load_scene_calls == 3);
    return 0;
}
''')


def soundtest_program(init_vars, update_inputs, update_functions, func_run):
    source = (DECOMP / "mn/mndata/mnsoundtest.c").read_text()
    return (PREAMBLE
            + enums_for(("MNSoundTestOptions",))
            + scene_macros()
            + shared_shims()
            + r'''
/* -- scene state (retained between entries like the live DS TU) -- */
static s32 sMNSoundTestOption;
static s32 sMNSoundTestOptionColorR[nMNSoundTestOptionEnumCount];
static s32 sMNSoundTestOptionColorG[nMNSoundTestOptionEnumCount];
static s32 sMNSoundTestOptionColorB[nMNSoundTestOptionEnumCount];
static s32 sMNSoundTestOptionChangeWait, sMNSoundTestDirectionInputKind;
static s32 sMNSoundTestOptionSelectID[nMNSoundTestOptionEnumCount];
static f32 sMNSoundTestSelectIDPositionsX[nMNSoundTestOptionEnumCount];
static s32 sMNSoundTestFadeOutWait;
void mnSoundTestFuncRun(GObj *gobj);
/* Small stand-ins for the three ID tables: the routines only index and
 * ARRAY_COUNT them, so sizes prove the wraparound. */
static u16 dMNSoundTestMusicIDs[3] = { 1001, 1002, 1003 };
static u16 dMNSoundTestSoundIDs[2] = { 2001, 2002 };
static u16 dMNSoundTestVoiceIDs[3] = { 3001, 3002, 3003 };
static void syAudioSetBGMVolumeFade(s32 player, u32 vol, u32 frames)
{
    (void)player;
    rec_fade_calls++; rec_fade_vol = vol; rec_fade_frames = frames;
}
static u32 rec_voice_stop_calls;
static void func_800266A0_272A0(void) { rec_voice_stop_calls++; }
static u32 rec_colors_updates;
static void mnSoundTestUpdateOptionColors(void) { rec_colors_updates++; }
''' + init_vars + r'''
static void soundtest_entry(void)
{
    gSCManagerSceneData.scene_prev = nSCKindData;
    gSCManagerSceneData.scene_curr = nSCKindSoundTest;
    mnSoundTestInitVars();
}
static void soundtest_frame(u32 tap, u32 hold)
{
    sc_tap = tap; sc_hold = hold;
    sc_stick_ud = sc_stick_lr = 0;
    sc_no_input_all = FALSE;
    mnSoundTestFuncRun(NULL);
}
''' + update_inputs + "\n" + update_functions + "\n" + func_run + r'''
int main(void)
{
    /* Entry 1: A on Music stops any BGM, plays table row 0, no volume calls
     * while idle-idle (the per-frame restore only runs outside a fade). */
    soundtest_entry();
    CHECK(sMNSoundTestOption == nMNSoundTestOptionMusic);
    CHECK(sMNSoundTestOptionSelectID[nMNSoundTestOptionMusic] == 0);
    CHECK(sMNSoundTestFadeOutWait == -1);
    soundtest_frame(A_BUTTON, 0);
    CHECK(rec_play_calls == 1 && rec_play_last == 1001);
    CHECK(rec_stop_all_calls == 1);
    /* R walks the Music ID; ChangeWait gates the next press. */
    soundtest_frame(0, R_TRIG);
    CHECK(sMNSoundTestOptionSelectID[nMNSoundTestOptionMusic] == 1);
    soundtest_frame(0, R_TRIG);
    CHECK(sMNSoundTestOptionSelectID[nMNSoundTestOptionMusic] == 1);
    { int i; for (i = 0; i < 25; i++) soundtest_frame(0, 0); }
    soundtest_frame(A_BUTTON, 0);
    CHECK(rec_play_calls == 2 && rec_play_last == 1002);
    /* Z stops everything: BGM all + the FGM/voice helper. */
    soundtest_frame(Z_TRIG, 0);
    CHECK(rec_stop_all_calls >= 3);
    CHECK(rec_voice_stop_calls == 1);
    /* START arms the 120-tic fade; no volume writes while it runs, one
     * stop when it lands, then the 0x7000 per-frame restore resumes. */
    soundtest_frame(START_BUTTON, 0);
    CHECK(rec_fade_calls == 1 && rec_fade_vol == 0 && rec_fade_frames == 120);
    CHECK(sMNSoundTestFadeOutWait == 120);
    CHECK(rec_voice_stop_calls == 2);
    { u32 volume_before = rec_set_volume_calls;
      int i;
      for (i = 0; i < 121; i++) soundtest_frame(0, 0); /* 120 down, 1 to land */
      CHECK(rec_set_volume_calls == volume_before);
      CHECK(sMNSoundTestFadeOutWait == -1); }
    CHECK(rec_set_volume_calls > 0);
    CHECK(rec_set_volume_last == 0x7000);
    /* B exits to Data, stopping all audio and restoring volume first. */
    { u32 stops_before = rec_stop_all_calls;
      soundtest_frame(B_BUTTON, 0);
      CHECK(gSCManagerSceneData.scene_prev == nSCKindSoundTest);
      CHECK(gSCManagerSceneData.scene_curr == nSCKindData);
      CHECK(rec_stop_all_calls == stops_before + 1);
      CHECK(rec_set_volume_last == 0x7000);
      CHECK(rec_load_scene_calls == 1); }
    /* Entry 2: the reset select IDs are load-bearing -- A plays row 0
     * again, not the row 1 retained from entry 1. */
    soundtest_entry();
    CHECK(sMNSoundTestOptionSelectID[nMNSoundTestOptionMusic] == 0);
    soundtest_frame(A_BUTTON, 0);
    CHECK(rec_play_last == 1001);
    /* Entry 3: leave MID-FADE (B at wait 70), then re-enter idle. The
     * fade-wait reset means no spurious stop and an immediate restore. */
    soundtest_frame(START_BUTTON, 0);
    { int i; for (i = 0; i < 50; i++) soundtest_frame(0, 0); }
    CHECK(sMNSoundTestFadeOutWait == 70);
    soundtest_frame(B_BUTTON, 0);
    { u32 stops_before = rec_stop_all_calls, volume_before = rec_set_volume_calls;
      soundtest_entry();
      CHECK(sMNSoundTestFadeOutWait == -1);
      { int i; for (i = 0; i < 130; i++) soundtest_frame(0, 0); }
      CHECK(rec_stop_all_calls == stops_before);
      CHECK(rec_set_volume_calls > volume_before); }
    return 0;
}
''')


def backupclear_program(init_vars, main_menu, confirm_menu, func_run, apply_id):
    source = (DECOMP / "mn/mnoption/mnbackupclear.c").read_text()
    eject_options = function(source, "mnBackupClearEjectOptionGObjs")
    eject_confirm = function(source, "mnBackupClearEjectOptionConfirmGObj")
    tab_colors = function(source, "mnBackupClearUpdateOptionTabColors")
    # implicit-int no-op in the source (:560); typed void here, same body
    noop = braced(source, r"^func_ovl53_801325CC\([^;]*?\)\s*\{")
    noop = re.sub(r"^func_ovl53_801325CC\(", "void func_ovl53_801325CC(", noop)
    return (PREAMBLE
            + enums_for(("MNBackupClearOptions", "MNOptionTabStatus"))
            + scene_macros()
            + shared_shims()
            + r'''
/* -- scene state (retained between entries like the live DS TU) -- */
static GObj *sMNBackupClearOptionNewcomersGObj, *sMNBackupClearOption1PHighScoreGObj;
static GObj *sMNBackupClearOptionBonusStageTimeGObj, *sMNBackupClearOptionVSRecordGObj;
static GObj *sMNBackupClearOptionPrizeGObj, *sMNBackupClearOptionAllDataClearGObj;
static s32 sMNBackupClearOption;
static GObj *sMNBackupClearUnusedGObj, *sMNBackupClearOptionConfirmGObj;
static sb32 sMNBackupClearOptionConfirmYesOrNo;
static s32 sMNBackupClearOptionMenuKind;
static int *sMNBackupClearOptionConfirmLUTOrigin;
static s32 sMNBackupClearOptionChangeWait, sMNBackupClearTotalTimeTics;
static s32 sMNBackupClearUpdateWait, sMNBackupClearOptionConfirmAnimLength;
static s32 sMNBackupClearReturnTic;
void mnBackupClearFuncRun(GObj *gobj);
void mnBackupClearApplyOptionID(s32 option);
static void *sMNBackupClearFiles[3] = { NULL, (void *)1, (void *)2 };
/* -- apply-path recorders, in call order -- */
static u32 rec_clear_order[16], rec_clear_order_n;
static void note_clear(u32 tag) { rec_clear_order[rec_clear_order_n++] = tag; }
static void lbBackupClearNewcomers(void) { note_clear(1); }
static void lbBackupClear1PHighScore(void) { note_clear(2); }
static void lbBackupClearBonusStageTime(void) { note_clear(3); }
static void lbBackupClearVSRecord(void) { note_clear(4); }
static void lbBackupClearPrize(void) { note_clear(5); }
static void lbBackupClearAllData(void) { note_clear(6); }
static void lbBackupApplyOptions(void) { note_clear(7); }
static void lbBackupCorrectErrors(void) { note_clear(8); }
static u32 rec_backup_writes;
static void lbBackupWrite(void) { rec_backup_writes++; rec_clear_order[rec_clear_order_n++] = 9; }
static u32 rec_make_unused, rec_set_colors;
static void mnBackupClearMakeUnused(s32 option) { (void)option; rec_make_unused++; }
static void mnBackupClearSetOptionSpriteColors(void) { rec_set_colors++; }
static u32 rec_confirm_kind, rec_confirm_yesno;
static void mnBackupClearMakeOptionConfirm(sb32 kind, sb32 yes_or_no)
{
    rec_confirm_kind = kind; rec_confirm_yesno = yes_or_no;
}
static int sConfirmPalette;
static int sConfirmLutOrigin;
static int llMNBackupClearOptionConfirmPalette;
#define lbRelocGetFileData(type, file, sym) ((type)&sConfirmPalette)
/* -- minimal sprite so the confirm LUT swap runs for real -- */
typedef struct { unsigned char r, g, b; } SYColorRGB;
typedef struct MNFakeSObj
{
    struct MNFakeSObj *next, *prev;
    struct { unsigned char red, green, blue; int *LUT; } sprite;
} SObj;
#define SObjGetStruct(g) ((SObj *)(g))
static SObj sFakeRows[6], sFakeConfirm;
static void backupclear_wire(void)
{
    int i;
    for (i = 0; i < 6; i++) sFakeRows[i].next = NULL;
    sFakeConfirm.next = NULL;
    sFakeConfirm.sprite.LUT = &sConfirmLutOrigin;
    sMNBackupClearOptionNewcomersGObj = (GObj *)&sFakeRows[0];
    sMNBackupClearOption1PHighScoreGObj = (GObj *)&sFakeRows[1];
    sMNBackupClearOptionBonusStageTimeGObj = (GObj *)&sFakeRows[2];
    sMNBackupClearOptionVSRecordGObj = (GObj *)&sFakeRows[3];
    sMNBackupClearOptionPrizeGObj = (GObj *)&sFakeRows[4];
    sMNBackupClearOptionAllDataClearGObj = (GObj *)&sFakeRows[5];
    sMNBackupClearOptionConfirmGObj = (GObj *)&sFakeConfirm;
}
''' + init_vars + r'''
static void backupclear_entry(void)
{
    gSCManagerSceneData.scene_prev = nSCKindOption;
    gSCManagerSceneData.scene_curr = nSCKindBackupClear;
    mnBackupClearInitVars();
}
static void backupclear_frame(u32 tap, u32 hold)
{
    sc_tap = tap; sc_hold = hold;
    sc_stick_ud = sc_stick_lr = 0;
    sc_no_input_all = FALSE;
    mnBackupClearFuncRun(NULL);
}
''' + eject_options + "\n" + eject_confirm + "\n" + tab_colors + "\n"
            + noop + "\n" + main_menu + "\n" + confirm_menu + "\n"
            + apply_id + "\n" + func_run + r'''
int main(void)
{
    /* Entry 1: UpdateWait 10 eats the first ten tics -- an early A must
     * not open the confirm, the eleventh does, defaulting to NO. */
    backupclear_wire();
    backupclear_entry();
    CHECK(sMNBackupClearOption == nMNBackupClearOptionNewcomers);
    CHECK(sMNBackupClearOptionMenuKind == 0);
    CHECK(sMNBackupClearUpdateWait == 10);
    { int i; for (i = 0; i < 9; i++) backupclear_frame(A_BUTTON, 0); }
    CHECK(rec_confirm_kind == 0);
    backupclear_frame(A_BUTTON, 0); /* tic 10 still waits */
    CHECK(rec_confirm_kind == 0);
    backupclear_frame(A_BUTTON, 0); /* tic 11: confirm, kind 1, cursor NO */
    CHECK(sMNBackupClearOptionMenuKind == 1);
    CHECK(sMNBackupClearOptionConfirmYesOrNo == 1);
    CHECK(rec_confirm_kind == 1 && rec_confirm_yesno == 1);
    CHECK(rec_fgm_last == nSYAudioFGMMenuSelect);
    CHECK(rec_eject_calls == 6); /* the six option rows left the screen */
    /* R moves to YES, A applies Newcomers: one clear call, then error
     * correction, then the save write, then the Backup Clear cue. */
    { int i; for (i = 0; i < 11; i++) backupclear_frame(0, 0); }
    backupclear_frame(R_JPAD, 0);
    CHECK(sMNBackupClearOptionConfirmYesOrNo == 0);
    CHECK(rec_confirm_kind == 1 && rec_confirm_yesno == 0);
    { int i; for (i = 0; i < 11; i++) backupclear_frame(0, 0); }
    backupclear_frame(A_BUTTON, 0);
    CHECK(sMNBackupClearOptionMenuKind == 0);
    CHECK(sMNBackupClearOptionConfirmAnimLength == 60);
    CHECK(sFakeConfirm.sprite.LUT == &sConfirmPalette);
    CHECK(rec_clear_order_n == 3);
    CHECK(rec_clear_order[0] == 1); /* lbBackupClearNewcomers */
    CHECK(rec_clear_order[1] == 8); /* lbBackupCorrectErrors */
    CHECK(rec_clear_order[2] == 9); /* lbBackupWrite */
    CHECK(rec_fgm_last == nSYAudioFGMOptionBackupClear);
    /* The 60-tic confirm animation runs, then the rows come back. */
    { int i; for (i = 0; i < 60; i++) backupclear_frame(0, 0); }
    CHECK(sMNBackupClearOptionConfirmAnimLength == 0);
    CHECK(rec_set_colors == 1);
    CHECK(rec_backup_writes == 1);
    /* Walk to All Data Clear (five rows down) and demand both confirms. */
    { int i;
      for (i = 0; i < 5; i++)
      {
        backupclear_frame(0, D_JPAD);
        { int j; for (j = 0; j < 13; j++) backupclear_frame(0, 0); }
      }
    }
    CHECK(sMNBackupClearOption == nMNBackupClearOptionAllDataClear);
    backupclear_frame(A_BUTTON, 0);
    CHECK(sMNBackupClearOptionMenuKind == 1);
    { int i; for (i = 0; i < 11; i++) backupclear_frame(0, 0); }
    backupclear_frame(R_JPAD, 0); /* YES on the first confirm */
    { int i; for (i = 0; i < 11; i++) backupclear_frame(0, 0); }
    backupclear_frame(A_BUTTON, 0);
    CHECK(sMNBackupClearOptionMenuKind == 2); /* second confirm, not applied */
    CHECK(rec_clear_order_n == 3);
    CHECK(rec_confirm_kind == 2);
    { int i; for (i = 0; i < 11; i++) backupclear_frame(0, 0); }
    backupclear_frame(R_JPAD, 0);
    { int i; for (i = 0; i < 11; i++) backupclear_frame(0, 0); }
    backupclear_frame(A_BUTTON, 0);
    CHECK(rec_clear_order_n == 7);
    CHECK(rec_clear_order[3] == 6); /* lbBackupClearAllData */
    CHECK(rec_clear_order[4] == 7); /* lbBackupApplyOptions */
    CHECK(rec_clear_order[5] == 8); /* lbBackupCorrectErrors */
    CHECK(rec_clear_order[6] == 9); /* lbBackupWrite */
    /* B from the main menu returns to Option immediately. */
    { int i; for (i = 0; i < 70; i++) backupclear_frame(0, 0); }
    backupclear_frame(B_BUTTON, 0);
    CHECK(gSCManagerSceneData.scene_prev == nSCKindBackupClear);
    CHECK(gSCManagerSceneData.scene_curr == nSCKindOption);
    CHECK(rec_load_scene_calls == 1);
    /* Entry 2: the cursor reset to Newcomers is the retained-state fix. */
    backupclear_entry();
    CHECK(sMNBackupClearOption == nMNBackupClearOptionNewcomers);
    CHECK(sMNBackupClearOptionMenuKind == 0);
    CHECK(sMNBackupClearUpdateWait == 10);
    return 0;
}
''')


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


class OptionsReentryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = next((shutil.which(c) for c in
                             ("clang", "gcc", "cc") if shutil.which(c)), None)

    def test_option_scene_reentry_and_routes(self):
        self.assertIsNotNone(self.compiler, "Host C compiler required")
        source = (DECOMP / "mn/mnoption/mnoption.c").read_text()
        init_vars = function(source, "mnOptionInitVars")
        func_run = function(source, "mnOptionFuncRun")
        strips = (
            ("no-proceed-reset", "    sMNOptionIsProceedScene = FALSE;\n"),
            ("no-cursor-restore",
             "        sMNOptionOption = nMNOptionOptionBackupClear;\n"),
        )
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            text = option_program(init_vars, func_run)
            result = compile_and_run(self.compiler, directory, "option", text)
            self.assertEqual(result.returncode, 0, result.stderr)
            for label, strip in strips:
                self.assertIn(strip, init_vars, label)
                broken = option_program(init_vars.replace(strip, "", 1), func_run)
                result = compile_and_run(self.compiler, directory,
                                         f"option_{label}", broken)
                self.assertNotEqual(result.returncode, 0,
                                    f"{label} strip must fail: {result.stderr}")

    def test_soundtest_scene_audio_arms_and_reentry(self):
        self.assertIsNotNone(self.compiler, "Host C compiler required")
        source = (DECOMP / "mn/mndata/mnsoundtest.c").read_text()
        init_vars = function(source, "mnSoundTestInitVars")
        update_inputs = function(source, "mnSoundTestUpdateControllerInputs")
        update_functions = function(source, "mnSoundTestUpdateFunctions")
        func_run = function(source, "mnSoundTestFuncRun")
        strips = (
            ("no-fade-reset", "    sMNSoundTestFadeOutWait = -1;\n"),
            ("no-selectid-reset",
             "    sMNSoundTestOptionSelectID[nMNSoundTestOptionMusic] = "
             "sMNSoundTestOptionSelectID[nMNSoundTestOptionSound] = "
             "sMNSoundTestOptionSelectID[nMNSoundTestOptionVoice] = 0;\n"),
        )
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            text = soundtest_program(init_vars, update_inputs,
                                     update_functions, func_run)
            result = compile_and_run(self.compiler, directory, "soundtest", text)
            self.assertEqual(result.returncode, 0, result.stderr)
            for label, strip in strips:
                self.assertIn(strip, init_vars, label)
                broken = soundtest_program(init_vars.replace(strip, "", 1),
                                           update_inputs, update_functions,
                                           func_run)
                result = compile_and_run(self.compiler, directory,
                                         f"soundtest_{label}", broken)
                self.assertNotEqual(result.returncode, 0,
                                    f"{label} strip must fail: {result.stderr}")

    def test_backupclear_scene_confirm_flow_and_reentry(self):
        self.assertIsNotNone(self.compiler, "Host C compiler required")
        source = (DECOMP / "mn/mnoption/mnbackupclear.c").read_text()
        init_vars = function(source, "mnBackupClearInitVars")
        main_menu = function(source, "mnBackupClearUpdateOptionMainMenu")
        confirm_menu = function(source, "mnBackupClearUpdateOptionConfirmMenu")
        func_run = function(source, "mnBackupClearFuncRun")
        apply_id = function(source, "mnBackupClearApplyOptionID")
        strips = (
            ("no-cursor-reset",
             "    sMNBackupClearOption = nMNBackupClearOptionStart;\n"),
        )
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            text = backupclear_program(init_vars, main_menu, confirm_menu,
                                       func_run, apply_id)
            result = compile_and_run(self.compiler, directory, "backupclear", text)
            self.assertEqual(result.returncode, 0, result.stderr)
            for label, strip in strips:
                self.assertIn(strip, init_vars, label)
                broken = backupclear_program(init_vars.replace(strip, "", 1),
                                             main_menu, confirm_menu,
                                             func_run, apply_id)
                result = compile_and_run(self.compiler, directory,
                                         f"backupclear_{label}", broken)
                self.assertNotEqual(result.returncode, 0,
                                    f"{label} strip must fail: {result.stderr}")

    def test_wrappers_stay_verbatim_pass_throughs(self):
        """The adapters must keep importing the whole source scene: every
        source-flow fix lives in the source routines these tests exercise,
        and any invented adapter behaviour would void that contract. The
        Backup Clear adapter may ride the generated overlay instead of the
        raw decomp path, but only for the single documented conformance
        line, which the overlay patch check below pins down."""
        for path, (scene, base, included, overlay) in WRAPPER_ADAPTERS.items():
            text = (ROOT / path).read_text()
            self.assertIn(f"#define {scene} {base}", text, path)
            if overlay is None:
                self.assertIn(f'#include "../../decomp/BattleShip-main/decomp/src/{included}"',
                              text, path)
            else:
                self.assertIn(f"#include <{overlay}>", text, path)
                self.assertEqual(Path(overlay).name, Path(included).name,
                                 f"{path}: overlay must be the same TU, "
                                 "not a renamed copy")
                self.assertNotIn('#include "../../decomp', text, path)
                self.assert_overlay_patch_is_conformance_only(included)
            adapter = re.search(rf"void {scene}\(void\)\s*\{{(.*?)\}}", text, re.S)
            self.assertIsNotNone(adapter, path)
            body = adapter.group(1)
            self.assertEqual(len(re.findall(r"\w+\(", body)), 1,
                             f"{path}: adapter body must only call {base}")
            self.assertIn(f"{base}();", body, path)

    def assert_overlay_patch_is_conformance_only(self, included):
        """The generated overlay behind the Backup Clear adapter must differ
        from the decomp source by exactly the documented implicit-int fix:
        one hunk giving func_ovl53_801325CC its void return type, with the
        source side still carrying the original typeless line."""
        patch = OVERLAY_PATCH.read_text()
        hunks = [line for line in patch.splitlines()
                 if line.startswith("@@")]
        self.assertEqual(len(hunks), 1,
                         "overlay patch must stay a single hunk")
        self.assertIn("-func_ovl53_801325CC(void)", patch)
        self.assertIn("+void func_ovl53_801325CC(void)", patch)
        added = [line for line in patch.splitlines()
                 if line.startswith("+") and not line.startswith("+++")]
        removed = [line for line in patch.splitlines()
                   if line.startswith("-") and not line.startswith("---")]
        self.assertEqual((removed, added),
                         (["-func_ovl53_801325CC(void)"],
                          ["+void func_ovl53_801325CC(void)"]),
                         "overlay patch must add and remove nothing else")
        source = (DECOMP / included).read_text()
        self.assertIn("\nfunc_ovl53_801325CC(void)\n", source,
                      "decomp source must still carry the implicit-int line "
                      "the overlay fixes")


if __name__ == "__main__":
    unittest.main()
