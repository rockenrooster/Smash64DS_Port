#!/usr/bin/env python3
"""Compile small, actual C function extractions; this is NOT a ROM/emulator test.

Usage: python test_menu_repair.py --repo /path/to/Smash64DS_Port [--cc cc]
Needs a host C11 compiler. Does not modify the checkout or build DS assets.
The test extracts the edited functions and the actual scene-stub guard block
from the supplied checkout. OS, video, allocation and surface I/O are test doubles.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def function(text: str, name: str) -> str | None:
    # These deliberately small target functions have no braces inside literals.
    pattern = re.compile(r"(?m)^(?:static\s+)?(?:void|u8|u32|s32|NdsUiKitSurfaceId)\s+" +
                         re.escape(name) + r"\s*\([^;{}]*\)\s*\{")
    match = pattern.search(text)
    if match is None:
        return None
    opening = text.index("{", match.start())
    depth = 0
    for index in range(opening, len(text)):
        depth += (text[index] == "{") - (text[index] == "}")
        if depth == 0:
            return text[match.start():index + 1]
    raise ValueError(f"Unclosed function: {name}")


def compile_command(compiler: str, c_file: Path, executable: Path,
                    defines: list[str]) -> list[str]:
    if Path(compiler).name.lower() in {"cl", "cl.exe"}:
        msvc_defines = ["/D" + value[2:] for value in defines]
        return [compiler, "/nologo", "/std:c11", "/O2", "/W4", "/WX",
                *msvc_defines, str(c_file), "/Fo:" + str(c_file.with_suffix(".obj")),
                "/Fe:" + str(executable)]
    return [compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
            *defines, str(c_file), "-o", str(executable)]


ENTRY_PREAMBLE = r'''
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
typedef uint16_t u16;
typedef struct { void *arena_start; size_t arena_size; } SceneSetup;
typedef struct { SceneSetup scene_setup; void (*func_start)(void); unsigned marker; } SYTaskmanSetup;
typedef struct { void *zbuffer; } SYVideoSetup;
unsigned starts, videos, parked, scene_curr, seen_scene;
unsigned char arena[4096];
u16 zbuffer;
void original_start(void) { assert(!"source func_start must not execute"); }
SYTaskmanSetup dMNVSModeTaskmanSetup = {{NULL, 17}, original_start, 0x12345678u};
SYVideoSetup dMNVSModeVideoSetup;
#define SYVIDEO_ZBUFFER_START(w,h,x,y,t) (&zbuffer)
void *ndsTaskmanArenaStart(void) { return arena; }
size_t ndsTaskmanArenaSize(void) { return sizeof(arena); }
void syVideoInit(SYVideoSetup *v) {
    assert(v == &dMNVSModeVideoSetup && v->zbuffer == &zbuffer); ++videos;
}
void syTaskmanStartTask(SYTaskmanSetup *s) {
    assert(s->scene_setup.arena_start == arena);
    assert(s->scene_setup.arena_size == sizeof(arena));
    assert(s->func_start == NULL && s->marker == 0x12345678u);
    assert(dMNVSModeTaskmanSetup.scene_setup.arena_start == NULL);
    assert(dMNVSModeTaskmanSetup.scene_setup.arena_size == 17);
    assert(dMNVSModeTaskmanSetup.func_start == original_start);
    seen_scene = scene_curr; ++starts;
}
#define NDS_SCENE_STUB(name) void name(void) { ++parked; }
'''
ENTRY_MAIN = r'''
int main(void) {
#if NDS_P2_MENU_SHELL
    void (*routes[])(void) = {mnModeSelectStartScene, mnVSOptionsStartScene, mnVSItemSwitchStartScene};
    /* Re-entry and source setup immutability, not a memory-leak measurement. */
    for (unsigned lap = 0; lap < 4; ++lap) {
        for (unsigned r = 0; r < 3; ++r) {
            scene_curr = 100 + r;
            unsigned before = starts;
            routes[r]();
            assert(starts == before + 1);
            assert(videos == starts && parked == 0);
            assert(scene_curr == 100 + r && seen_scene == scene_curr);
        }
    }
#else
    mnVSOptionsStartScene(); mnVSItemSwitchStartScene();
    assert(parked == 2 && starts == 0 && videos == 0);
#endif
    puts("entry-point assertions passed"); return 0;
}
'''
ROW_PREAMBLE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
typedef uint32_t u32;
typedef uint8_t u8;
typedef unsigned NdsUiKitSurfaceId;
#define NDS_MENU_VSOPTIONS_ROWS 5u
#define NDS_MENU_VSOPTIONS_ITEMSWITCH 4u
#define FALSE 0
NdsUiKitSurfaceId sMenuVsOptionsRowSurface[5], wanted[5];
u8 sMenuVsOptionsHaveItemSwitch;
u32 gNdsMenuShellVsOptionsBlitCount, attempts, fail_at;
NdsUiKitSurfaceId ndsMenuShellVsOptionsWantSurface(u32 row) { return wanted[row]; }
int ndsUiKitBlitSurfaces(const NdsUiKitSurfaceId *surface, u32 count) {
    assert(surface != NULL && count == 1); ++attempts;
    return attempts != fail_at;
}
'''
ROW_MAIN = r'''
int main(int argc, char **argv) {
    assert(argc == 2);
    sMenuVsOptionsHaveItemSwitch = 1;
    for (u32 i = 0; i < 5; ++i) wanted[i] = 10 + i;
    switch (atoi(argv[1])) {
    case 0: /* Zero budget must not write. */
        ndsMenuShellVsOptionsSyncRows(0);
        assert(attempts == 0 && gNdsMenuShellVsOptionsBlitCount == 0); break;
    case 1: /* One budget must not write two rows. */
        ndsMenuShellVsOptionsSyncRows(1);
        assert(attempts == 1 && gNdsMenuShellVsOptionsBlitCount == 1);
        assert(sMenuVsOptionsRowSurface[0] == wanted[0] && sMenuVsOptionsRowSurface[1] == 0); break;
    case 2: /* Entry can populate all rows. */
        ndsMenuShellVsOptionsSyncRows(5);
        assert(attempts == 5 && gNdsMenuShellVsOptionsBlitCount == 5); break;
    case 3: /* Idle does not re-blit. */
        for (u32 i = 0; i < 5; ++i) sMenuVsOptionsRowSurface[i] = wanted[i];
        ndsMenuShellVsOptionsSyncRows(5); assert(attempts == 0); break;
    case 4: /* Failed I/O must not mark content drawn; next call retries. */
        fail_at = 1; ndsMenuShellVsOptionsSyncRows(5);
        assert(attempts == 1 && gNdsMenuShellVsOptionsBlitCount == 0);
        for (u32 i = 0; i < 5; ++i) assert(sMenuVsOptionsRowSurface[i] == 0);
        fail_at = 0; ndsMenuShellVsOptionsSyncRows(5);
        assert(attempts == 6 && gNdsMenuShellVsOptionsBlitCount == 5); break;
    case 5: /* Two dirty rows: deselect old and highlight new. */
        for (u32 i = 0; i < 5; ++i) sMenuVsOptionsRowSurface[i] = wanted[i];
        ++wanted[1]; ++wanted[3]; ndsMenuShellVsOptionsSyncRows(5);
        assert(attempts == 2 && gNdsMenuShellVsOptionsBlitCount == 2); break;
    case 6: /* Failure after one success preserves exactly that success. */
        fail_at = 2; ndsMenuShellVsOptionsSyncRows(5);
        assert(attempts == 2 && gNdsMenuShellVsOptionsBlitCount == 1);
        assert(sMenuVsOptionsRowSurface[0] == wanted[0] && sMenuVsOptionsRowSurface[1] == 0);
        fail_at = 0; ndsMenuShellVsOptionsSyncRows(5);
        assert(gNdsMenuShellVsOptionsBlitCount == 5 && attempts == 6); break;
    case 7: /* Locked Item Switch is absent rather than merely unselectable. */
        sMenuVsOptionsHaveItemSwitch = 0;
        ndsMenuShellVsOptionsSyncRows(5);
        assert(attempts == 4 && gNdsMenuShellVsOptionsBlitCount == 4);
        assert(sMenuVsOptionsRowSurface[4] == 0); break;
    default: assert(!"unknown case");
    }
    puts("row assertions passed"); return 0;
}
'''

HANDICAP_PREAMBLE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint8_t u8;
typedef uint32_t u32;
#define NDS_MATCH_FIGHTERS_MAX 4u
#define NDS_MENU_VSOPTIONS_HANDICAP 0u
#define NDS_MENU_VSOPTIONS_TEAM 1u
#define NDS_MENU_VSOPTIONS_STAGE 2u
#define nSCBattleHandicapOff 0u
#define nSCBattleHandicapOn 1u
#define nSCBattleHandicapAuto 2u
#define NDS_UI_KIT_SFX_VALUE 1u
typedef struct { u8 handicap; } NdsMatchFighterConfig;
typedef struct {
    u8 handicap_mode, is_team_attack, is_stage_select, damage_ratio;
    NdsMatchFighterConfig fighters[NDS_MATCH_FIGHTERS_MAX];
} NdsMatchConfig;
typedef struct { u8 handicap; } TestPlayer;
typedef struct { TestPlayer players[NDS_MATCH_FIGHTERS_MAX]; } TestBattleState;
NdsMatchConfig gNdsMatchConfig;
TestBattleState gSCManagerTransferBattleState;
u8 sMenuVsOptionsHandicap, sMenuVsOptionsTeam, sMenuVsOptionsStage, sMenuVsOptionsDamage;
u32 sMenuVsOptionsCursor, gNdsMenuShellVsOptionsCommitCount;
u32 gNdsMenuShellVsOptionsCommitHandicap, gNdsMenuShellVsOptionsCommitDamage;
u32 apply_calls, sfx_calls;
void ndsMatchConfigApply(const NdsMatchConfig *cfg) { assert(cfg == &gNdsMatchConfig); ++apply_calls; }
void ndsUiKitSfx(u32 sfx) { assert(sfx == NDS_UI_KIT_SFX_VALUE); ++sfx_calls; }
'''

HANDICAP_MAIN = r'''
static void expect_handicap(u8 value) {
    for (u32 i = 0; i < NDS_MATCH_FIGHTERS_MAX; ++i) {
        assert(gNdsMatchConfig.fighters[i].handicap == value);
        assert(gSCManagerTransferBattleState.players[i].handicap == value);
    }
}
int main(void) {
    sMenuVsOptionsCursor = NDS_MENU_VSOPTIONS_HANDICAP;
    sMenuVsOptionsHandicap = nSCBattleHandicapOff;
    for (u32 i = 0; i < NDS_MATCH_FIGHTERS_MAX; ++i) {
        gNdsMatchConfig.fighters[i].handicap = (u8)(i + 1u);
        gSCManagerTransferBattleState.players[i].handicap = (u8)(i + 2u);
    }
    ndsMenuShellVsOptionsConfirm();
    assert(sMenuVsOptionsHandicap == nSCBattleHandicapOn); expect_handicap(9u);
    ndsMenuShellVsOptionsConfirm();
    assert(sMenuVsOptionsHandicap == nSCBattleHandicapAuto); expect_handicap(5u);
    ndsMenuShellVsOptionsConfirm();
    assert(sMenuVsOptionsHandicap == nSCBattleHandicapOff); expect_handicap(9u);

    for (u32 i = 0; i < NDS_MATCH_FIGHTERS_MAX; ++i) {
        gNdsMatchConfig.fighters[i].handicap = 1u;
        gSCManagerTransferBattleState.players[i].handicap = 2u;
    }
    ndsMenuShellVsOptionsSave();
    assert(apply_calls == 1u && gNdsMatchConfig.handicap_mode == nSCBattleHandicapOff);
    expect_handicap(9u);
    puts("handicap option assertions passed"); return 0;
}
'''

APPLY_PREAMBLE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint8_t ub8;
typedef uint32_t u32;
typedef int32_t s32;
#define TRUE 1u
#define FALSE 0u
#define NDS_MATCH_FIGHTERS_MAX 4
#define nSCBattleHandicapOff 0u
#define nSCBattleHandicapOn 1u
#define nSCBattleHandicapAuto 2u
#define nSCBattleGameType1PGame 1u
#define NDS_MATCH_NO_GAME_TYPE 0xffu
#define NDS_MATCH_NO_SPGAME_STAGE 0xffu
#define SCBATTLE_GAMERULE_TIME 1u
#define nFTPlayerKindMan 0u
#define nFTPlayerKindCom 1u
#define GMCOMMON_PLAYERS_MAX 4u
#define nSC1PGameStageCommonEnd 8u
#define nIFPlayerTagKindHeart 5u
#define NDS_P2_1P_GAME 0
typedef struct {
    u8 fkind, pkind, costume, shade, team, handicap, level;
    u8 stock_count, is_spgame_enemy, copy_kind;
} NdsMatchFighterConfig;
typedef struct {
    u8 gkind, game_type, spgame_stage;
    u32 game_rules;
    u8 time_limit, stocks, handicap_mode, is_team_battle, is_team_attack;
    u8 is_stage_select, is_reset_players;
    u32 item_toggles;
    u8 item_appearance_rate, damage_ratio;
    NdsMatchFighterConfig fighters[NDS_MATCH_FIGHTERS_MAX];
} NdsMatchConfig;
typedef struct {
    u8 player, team, fkind, pkind, costume, shade, color, tag;
    u8 is_single_stockicon, handicap, level, stock_count, is_spgame_enemy;
} SCPlayerData;
typedef struct {
    u8 game_type, gkind;
    u32 game_rules;
    u8 time_limit, stocks, handicap, is_team_battle, is_team_attack;
    u8 is_stage_select, is_reset_players;
    u32 item_toggles;
    u8 item_appearance_rate, damage_ratio;
    SCPlayerData players[NDS_MATCH_FIGHTERS_MAX];
    u8 pl_count, cp_count;
} SCBattleState;
typedef struct { u8 gkind, spgame_stage; } SCSceneData;
SCBattleState dSCManagerDefaultBattleState, gSCManagerTransferBattleState;
SCSceneData dSCManagerDefaultSceneData, gSCManagerSceneData;
u8 dIFCommonPlayerTeamColorIDs[4] = {0u, 1u, 2u, 3u};
volatile u32 gNdsMatchConfigHandicapClampCount;
'''

APPLY_MAIN = r'''
static void seed_cfg(NdsMatchConfig *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->game_type = NDS_MATCH_NO_GAME_TYPE;
    cfg->spgame_stage = NDS_MATCH_NO_SPGAME_STAGE;
    for (u32 i = 0; i < NDS_MATCH_FIGHTERS_MAX; ++i) cfg->fighters[i].pkind = nFTPlayerKindMan;
}
int main(void) {
    NdsMatchConfig cfg;
    const u8 bad[4] = {0u, 10u, 5u, 255u};
    seed_cfg(&cfg);
    cfg.handicap_mode = nSCBattleHandicapOff;
    for (u32 i = 0; i < 4u; ++i) cfg.fighters[i].handicap = bad[i];
    ndsMatchConfigApply(&cfg);
    assert(gSCManagerTransferBattleState.players[0].handicap == 1u);
    assert(gSCManagerTransferBattleState.players[1].handicap == 9u);
    assert(gSCManagerTransferBattleState.players[2].handicap == 5u);
    assert(gSCManagerTransferBattleState.players[3].handicap == 9u);
    assert(gNdsMatchConfigHandicapClampCount == 3u);

    cfg.handicap_mode = nSCBattleHandicapOn;
    gSCManagerTransferBattleState.players[0].handicap = 0u;
    gSCManagerTransferBattleState.players[1].handicap = 10u;
    gSCManagerTransferBattleState.players[2].handicap = 5u;
    gSCManagerTransferBattleState.players[3].handicap = 9u;
    ndsMatchConfigApply(&cfg);
    assert(gSCManagerTransferBattleState.players[0].handicap == 1u);
    assert(gSCManagerTransferBattleState.players[1].handicap == 9u);
    assert(gSCManagerTransferBattleState.players[2].handicap == 5u);
    assert(gSCManagerTransferBattleState.players[3].handicap == 9u);
    assert(gNdsMatchConfigHandicapClampCount == 5u);

    cfg.game_type = nSCBattleGameType1PGame;
    for (u32 i = 0; i < 4u; ++i) {
        cfg.fighters[i].handicap = 4u;
        gSCManagerTransferBattleState.players[i].handicap = 7u;
    }
    ndsMatchConfigApply(&cfg);
    for (u32 i = 0; i < 4u; ++i) assert(gSCManagerTransferBattleState.players[i].handicap == 4u);
    puts("match apply handicap clamp assertions passed"); return 0;
}
'''

ITEMS_PREAMBLE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
#define NDS_MENU_ITEMSWITCH_ROWS 16u
#define nSCBattleItemSwitchNone 0u
#define nSCBattleItemSwitchVeryHigh 5u
#define NDS_UI_KIT_SFX_VALUE 1u
typedef struct { u8 item_appearance_rate; u32 item_toggles; } NdsMatchConfig;
u8 sMenuItemsStatus[NDS_MENU_ITEMSWITCH_ROWS];
u32 sMenuItemsCursor;
NdsMatchConfig gNdsMatchConfig;
u32 gNdsMenuShellItemsCommitCount, gNdsMenuShellItemsCommitToggles;
u32 gNdsMenuShellItemsCommitRate, sfx_calls, apply_calls;
void ndsUiKitSfx(u32 sfx) { assert(sfx == NDS_UI_KIT_SFX_VALUE); ++sfx_calls; }
u32 ndsMatchConfigItemTogglesFromRows(const u8 *rows) {
    u32 toggles = 0u;
    for (u32 i = 0; i < 15u; ++i) if (rows[i] != 0u) toggles |= (1u << i);
    return toggles;
}
void ndsMatchConfigApply(const NdsMatchConfig *cfg) { assert(cfg == &gNdsMatchConfig); ++apply_calls; }
'''

ITEMS_MAIN = r'''
int main(void) {
    sMenuItemsCursor = 1u;
    sMenuItemsStatus[1] = 0u;
    ndsMenuShellItemsAdjust(-1); assert(sMenuItemsStatus[1] == 1u && sfx_calls == 1u);
    ndsMenuShellItemsAdjust(-1); assert(sMenuItemsStatus[1] == 1u && sfx_calls == 1u);
    ndsMenuShellItemsAdjust(1); assert(sMenuItemsStatus[1] == 0u && sfx_calls == 2u);
    ndsMenuShellItemsAdjust(1); assert(sMenuItemsStatus[1] == 0u && sfx_calls == 2u);
    ndsMenuShellItemsConfirm(); assert(sMenuItemsStatus[1] == 1u && sfx_calls == 3u);
    ndsMenuShellItemsConfirm(); assert(sMenuItemsStatus[1] == 0u && sfx_calls == 4u);

    sMenuItemsCursor = 0u; sMenuItemsStatus[0] = nSCBattleItemSwitchNone;
    ndsMenuShellItemsAdjust(-1); assert(sMenuItemsStatus[0] == nSCBattleItemSwitchVeryHigh);
    ndsMenuShellItemsAdjust(1); assert(sMenuItemsStatus[0] == nSCBattleItemSwitchNone);

    gNdsMatchConfig.item_appearance_rate = 2u;
    sMenuItemsStatus[0] = 5u;
    for (u32 i = 1u; i < NDS_MENU_ITEMSWITCH_ROWS; ++i) sMenuItemsStatus[i] = 0u;
    ndsMenuShellItemsSave();
    assert(gNdsMatchConfig.item_toggles == 0u && gNdsMatchConfig.item_appearance_rate == 2u);
    sMenuItemsStatus[1] = 1u;
    ndsMenuShellItemsSave();
    assert(gNdsMatchConfig.item_toggles == 1u && gNdsMatchConfig.item_appearance_rate == 5u);
    assert(apply_calls == 2u);
    puts("item switch assertions passed"); return 0;
}
'''

CSS_RANDOM_PREAMBLE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint32_t u32;
#define TRUE 1u
#define FALSE 0u
#define nGRKindInishie 8u
#define nGRKindPupupu 6u
#define nGRKindStarterEnd 7u
#define nGRKindUnlockEnd 8u
#define LBBACKUP_UNLOCK_MASK_INISHIE 1u
typedef struct { u8 gkind; } TestScene;
TestScene gNdsMatchConfig, dSCManagerDefaultSceneData, gSCManagerSceneData;
typedef struct { u32 unlock_mask; } TestBackup;
TestBackup gSCManagerBackupData;
u32 gNdsMenuShellCssRandomCount, gNdsMenuShellCssRandomFallbackCount;
u8 locked[9], rolls[64];
u32 roll_pos;
static u32 ndsMenuShellSssGroundLocked(u32 gkind) { return (gkind > 8u) ? TRUE : locked[gkind]; }
u8 syUtilsRandTimeUCharRange(u32 range) {
    assert(range == ((gSCManagerBackupData.unlock_mask & LBBACKUP_UNLOCK_MASK_INISHIE) ? 9u : 8u));
    return rolls[roll_pos++ & 63u];
}
'''

CSS_RANDOM_MAIN = r'''
static void lock_all(void) { memset(locked, 1, sizeof(locked)); memset(rolls, 8, sizeof(rolls)); roll_pos = 0u; }
int main(void) {
    gSCManagerBackupData.unlock_mask = 0u;
    lock_all(); locked[4] = 0u; rolls[0] = 4u; gSCManagerSceneData.gkind = 4u;
    ndsMenuShellCssRandomizeStage();
    assert(gNdsMatchConfig.gkind == 4u && ndsMenuShellSssGroundLocked(gNdsMatchConfig.gkind) == FALSE);

    lock_all(); locked[2] = 0u; locked[6] = 0u; memset(rolls, 2, sizeof(rolls));
    gSCManagerSceneData.gkind = 2u;
    ndsMenuShellCssRandomizeStage();
    assert(gNdsMatchConfig.gkind == 6u && gNdsMenuShellCssRandomFallbackCount == 1u);

    lock_all(); locked[2] = 0u; locked[7] = 0u; rolls[0] = 7u; gSCManagerSceneData.gkind = 2u;
    ndsMenuShellCssRandomizeStage();
    assert(gNdsMatchConfig.gkind == 7u && gNdsMatchConfig.gkind <= nGRKindInishie);
    gSCManagerBackupData.unlock_mask = LBBACKUP_UNLOCK_MASK_INISHIE;
    lock_all(); locked[2] = 0u; locked[8] = 0u; rolls[0] = 8u; gSCManagerSceneData.gkind = 2u;
    ndsMenuShellCssRandomizeStage();
    assert(gNdsMatchConfig.gkind == 8u);
    assert(gNdsMenuShellCssRandomCount == 4u);
    puts("CSS random ground assertions passed"); return 0;
}
'''


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", required=True, type=Path)
    parser.add_argument("--cc", default=os.environ.get("CC", "cc"))
    args = parser.parse_args(argv)
    compiler = shutil.which(args.cc)
    if compiler is None:
        parser.error(f"Host compiler not found: {args.cc}; supply --cc gcc or clang")
    router = (args.repo / "src/nds/nds_menu_shell_router.c").read_text(encoding="utf-8")
    backend = (args.repo / "src/port/title_backend.c").read_text(encoding="utf-8")
    options = (args.repo / "src/nds/nds_menu_shell_vsoptions.c").read_text(encoding="utf-8")
    items = (args.repo / "src/nds/nds_menu_shell_items.c").read_text(encoding="utf-8")
    css = (args.repo / "src/nds/nds_menu_shell_css.c").read_text(encoding="utf-8")
    match_config = (args.repo / "src/port/nds_match_config.c").read_text(encoding="utf-8")
    stub_start = backend.index("NDS_SCENE_STUB(mnUnusedFightersStartScene)")
    stub_end = backend.index("#if !NDS_P2_1P_GAME", stub_start)
    stubs = backend[stub_start:stub_end]  # Keeps the actual shell-off guard.
    names = ["ndsMenuShellStartNative2DScene", "mnModeSelectStartScene",
             "mnVSOptionsStartScene", "mnVSItemSwitchStartScene"]
    bodies = [body for name in names if (body := function(router, name)) is not None]
    rows = function(options, "ndsMenuShellVsOptionsSyncRows")
    handicap_set = function(options, "ndsMenuShellVsOptionsSetHandicapSettings")
    handicap_confirm = function(options, "ndsMenuShellVsOptionsConfirm")
    handicap_save = function(options, "ndsMenuShellVsOptionsSave")
    clamp_handicap = function(match_config, "ndsMatchConfigClampPublishedHandicap")
    apply_match = function(match_config, "ndsMatchConfigApply")
    items_adjust = function(items, "ndsMenuShellItemsAdjust")
    items_confirm = function(items, "ndsMenuShellItemsConfirm")
    items_save = function(items, "ndsMenuShellItemsSave")
    css_random = function(css, "ndsMenuShellCssRandomizeStage")
    vsoptions_surface = function(options, "ndsMenuShellVsOptionsWantSurface")
    css_draw_arrows = function(css, "ndsMenuShellCssDrawArrows")
    css_touch_arrows = function(css, "ndsMenuShellCssCheckLevelArrows")
    if (not bodies or rows is None or handicap_set is None or handicap_confirm is None or
            handicap_save is None or clamp_handicap is None or apply_match is None or
            items_adjust is None or items_confirm is None or items_save is None or
            css_random is None or vsoptions_surface is None or css_draw_arrows is None or
            css_touch_arrows is None):
        raise ValueError("Expected menu function definitions not found; inspect the new source layout.")
    entry = ENTRY_PREAMBLE + "\n#if NDS_P2_MENU_SHELL\n" + "\n".join(bodies) + "\n#endif\n" + stubs + ENTRY_MAIN
    row_code = ROW_PREAMBLE + rows + ROW_MAIN
    handicap_code = (HANDICAP_PREAMBLE + handicap_set + handicap_confirm +
                     handicap_save + HANDICAP_MAIN)
    damage_code = (HANDICAP_PREAMBLE + r'''
typedef int32_t s32;
#define NDS_MENU_VSOPTIONS_DAMAGE 3u
#define NDS_MENU_VSOPTIONS_DAMAGE_MIN 50u
#define NDS_MENU_VSOPTIONS_DAMAGE_MAX 200u
void ndsMenuShellVsOptionsDrawDamage(void) {}
''' + handicap_set + function(options, "ndsMenuShellVsOptionsAdjust") + r'''
int main(void) {
    sMenuVsOptionsCursor = NDS_MENU_VSOPTIONS_DAMAGE;
    for (int value = 50; value <= 200; ++value) {
        for (int step = -3; step <= 3; ++step) {
            if (step == 0 || step == -2 || step == 2) continue;
            sMenuVsOptionsDamage = value;
            ndsMenuShellVsOptionsAdjust(step);
            assert(sMenuVsOptionsDamage == 50 + (value - 50 + step + 151) % 151);
        }
    }
    sMenuVsOptionsCursor = NDS_MENU_VSOPTIONS_HANDICAP;
    sMenuVsOptionsHandicap = nSCBattleHandicapOff;
    ndsMenuShellVsOptionsAdjust(-3);
    assert(sMenuVsOptionsHandicap == nSCBattleHandicapAuto);
    puts("damage tap/held/wrap assertions passed"); return 0;
}
''')
    assert "sMenuVsOptionsHaveItemSwitch = 1u;" in function(options, "ndsMenuShellVsOptionsLoad")
    update_options = " ".join(
        function(options, "ndsMenuShellUpdateVsOptions").split())
    # M02 (owner, 2026-09-21): the held magnitude moved behind a row-local
    # helper so Damage repeats by five while every other row keeps three.
    # Taps are still +-1, so pin the call rather than the old literal.
    assert "(taps & NDS_INPUT_LEFT) ? -1 : -ndsMenuShellVsOptionsHeldStep()" in update_options
    assert "(taps & NDS_INPUT_RIGHT) ? 1 : ndsMenuShellVsOptionsHeldStep()" in update_options
    apply_code = APPLY_PREAMBLE + clamp_handicap + apply_match + APPLY_MAIN
    items_code = ITEMS_PREAMBLE + items_adjust + items_confirm + items_save + ITEMS_MAIN
    css_random_code = CSS_RANDOM_PREAMBLE + css_random + CSS_RANDOM_MAIN
    tour_code = r'''
#include <assert.h>
typedef unsigned u32;
#define TRUE 1u
#define FALSE 0u
#define NDS_CSS_PORTRAITS 12u
#define NDS_CSS_WALK_TOUR_HOLD_TICS 24u
u32 sCssWalkTourFinished, gNdsMenuShellWalkLoops, sCssStartWait;
u32 gNdsMenuShellWalkBudget = 1, sCssWalkTourKind = 12, sCssWalkTourHold;
u32 gNdsFighterDLAllDrawP0HardwareTriangleCount, sCssWalkTourTriBase;
u32 gNdsMenuShellCssWalkTourTriangles[12], gNdsMenuShellCssWalkTourDrewMask;
u32 gNdsMenuShellCssWalkTourDoneCount, gNdsMenuShellCssWalkTourKindMask;
u32 sMenuWalkCursor = 30, sMenuWalkTimer = 39, sMenuWalkHold = 1, sMenuWalkHeld = 32;
u32 restored;
void ndsMenuShellCssWalkRestoreGate(void) { ++restored; }
void ndsMenuShellCssWalkTourSelect(u32 kind) { (void)kind; assert(0); }
u32 ndsMenuShellCssFighterLocked(u32 kind) { (void)kind; return 0; }
''' + function(css, "ndsMenuShellCssWalkTourStep") + r'''
int main(void) {
    assert(ndsMenuShellCssWalkTourStep() == TRUE);
    assert(restored == 1 && sCssWalkTourFinished && gNdsMenuShellCssWalkTourDoneCount == 1);
    assert(sMenuWalkCursor == 0 && sMenuWalkTimer == 0 && sMenuWalkHold == 0 && sMenuWalkHeld == 0);
    assert(sCssStartWait == 0); /* Ordinary controller input must still start the match. */
    assert(ndsMenuShellCssWalkTourStep() == FALSE && restored == 1);
    return 0;
}
'''
    results: list[tuple[str, bool]] = []
    with tempfile.TemporaryDirectory(prefix="smash-menu-test-") as temp:
        path = Path(temp)
        for label, code, defines, cases in (
            ("shell-on-entry", entry, ["-DNDS_P2_MENU_SHELL=1", "-DNDS_P2_1P_GAME=0"], [None]),
            ("shell-off-stubs", entry, ["-DNDS_P2_MENU_SHELL=0", "-DNDS_P2_1P_GAME=0"], [None]),
            ("row-budget", row_code, [], list(range(8))),
            ("handicap-rules", handicap_code, [], [None]),
            ("damage-repeat-policy", damage_code, [], [None]),
            ("match-apply-clamp", apply_code, [], [None]),
            ("item-rules", items_code, [], [None]),
            ("css-random-ground", css_random_code, [], [None]),
            ("css-tour-resumes-input", tour_code, [], [None]),
        ):
            c_file, executable = path / f"{label}.c", path / (label + (".exe" if os.name == "nt" else ""))
            c_file.write_text(code, encoding="utf-8")
            built = subprocess.run(compile_command(compiler, c_file, executable, defines),
                                   capture_output=True, text=True, cwd=path)
            if built.returncode:
                print(f"FAIL {label}: compile\n{built.stderr}")
                results.append((label, False)); continue
            for case in cases:
                name = label if case is None else f"{label}/{case}"
                ran = subprocess.run([str(executable), *([] if case is None else [str(case)])], capture_output=True, text=True)
                good = ran.returncode == 0
                results.append((name, good))
                print(f"{'PASS' if good else 'FAIL'} {name}")
                if not good:
                    print(ran.stderr.strip())

        locked_tokens = (
            "VS_OPTIONS_HANDICAP_ON_LOCKED", "VS_OPTIONS_HANDICAP_AUTO_LOCKED",
            "VS_OPTIONS_HANDICAP_OFF_LOCKED", "VS_OPTIONS_TEAM_ON_LOCKED",
            "VS_OPTIONS_TEAM_OFF_LOCKED", "VS_OPTIONS_STAGE_ON_LOCKED",
            "VS_OPTIONS_STAGE_OFF_LOCKED", "VS_OPTIONS_DAMAGE_LABEL_LOCKED",
        )
        locked_ok = all(token in vsoptions_surface for token in locked_tokens)
        results.append(("locked-layout-family-selection", locked_ok))
        print(f"{'PASS' if locked_ok else 'FAIL'} locked-layout-family-selection")

        arrow_ok = ("NDS_CSS_CURSOR_SLOT" not in css_draw_arrows and
                    "NDS_CSS_CURSOR_SLOT" not in css_touch_arrows)
        results.append(("css-arrow-gating", arrow_ok))
        print(f"{'PASS' if arrow_ok else 'FAIL'} css-arrow-gating")
    failures = sum(not passed for _, passed in results)
    print(f"{len(results) - failures}/{len(results)} host cases passed. ROM/runtime verification is separate.")
    return int(failures != 0)


def test_repository_menu_repairs() -> None:
    """Include the bundled extracted-C cases in the repository pytest suite."""
    compiler = shutil.which("gcc") or shutil.which("clang") or shutil.which("cl")
    assert compiler is not None, "A host C11 compiler is required"
    assert main(["--repo", str(Path(__file__).resolve().parents[2]),
                 "--cc", compiler]) == 0


if __name__ == "__main__":
    raise SystemExit(main())
