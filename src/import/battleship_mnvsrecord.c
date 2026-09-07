/* P2-7 item 4. VS Record screen, source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mndata/mnvsrecord.c whole,
 * following src/import/battleship_mnsoundtest.c (same mndata/ directory,
 * same slice) and src/import/battleship_sc1pbonusstage.c (scene TU with the
 * scene entry imported as ndsBase* and re-exported under its source name, so
 * a later measured DS arena rebudget has a seam and the diff stays
 * reviewable). The adapter is a verbatim pass-through; no behaviour invented
 * here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md RECORDS/HISCORE rows):
 * - Portrait block reads vs_records damage_given/damage_taken plus the
 *   ranking/use% derivations (:1068-1124, esp. :1120-1123).
 * - Ranking sort keys read vs_records time_used (:1167-1169) alongside KOs,
 *   TKO, SD%, win% and use% (:1146-1180); the time column renders
 *   time_used/3600 minutes and (time_used%3600)/60 seconds (:1484-1515).
 * - BattleScore matrix reads the per-pair ko_count table (:1238-1277);
 *   Indiv view reads ko_count both directions plus played_against /
 *   player_count_tallies (:1648-1716).
 * - B on BattleScore returns to nSCKindData, B elsewhere steps back one
 *   stats kind, A/START steps forward (:1974-2009); U/D walks the ranked
 *   fighter, L/R pages the ranking columns on the Ranking kind and walks the
 *   portrait on the Indiv kind (:2010-2164).
 *
 * Gated on NDS_P2_MENU_SHELL || NDS_P2_1P_GAME like battleship_mndata.c:
 * the shell compiles the source as ndsBaseMNVSRecordStartScene either way
 * and re-exports it as mnVSRecordStartScene only with the shell off, so
 * the native VS Record screen's own mnVSRecordStartScene
 * (src/nds/nds_menu_shell_router.c) wins when the shell is on -- the exact
 * DATA arrangement. The Makefile has no NDS_P2_MODES_META flag
 * (verified 2026-09-05; only NDS_P2_1P_GAME gates the P2-6/P2-7 imports),
 * so this rides the campaign flag like the item-5 SoundTest TU until P2-7
 * mints its own gate.
 *
 * Shell status: the shell's native first view is
 * src/nds/nds_menu_shell_vsrecord.c (NDS_MENU_SHELL_SCREEN_VSRECORD),
 * wired through the router's Populate/Update switch and
 * ndsMenuShellRunVsRecord like the native DATA screen. What still reaches
 * this source scene is the shell-off build, where the DATA menu's VS
 * RECORD row routes by the scene registry. Deeper views (Ranking, Indiv)
 * stay source-side until a later slice wires them.
 *
 * Shims vs unresolved, see handoff report:
 * - Menu enums nMNVSRecordKind* / nMNVSRecordRankingKind* (decomp
 *   mn/mndef.h:234-258), the audio ordinals nSYAudioFGMFoxFoot and
 *   nSYAudioBGMData, and the ~90 ll* rows (llMNVSRecordMain*, llMNDataCommon*,
 *   llMNCommonFonts*, llMNPlayersPortraits*): all carried by the port headers
 *   and include/reloc_data.h since the 2026-09-05 header widening and reloc
 *   staging (census: 0 unstaged symbols in this TU).
 * - Resolved port-side, no action: gSCManagerBackupData.vs_records +
 *   LBBACKUP_MASK_FIGHTER (include/ft/fighter.h:143 + include/sc/scene.h),
 *   nSYAudioFGMBurnS (include/gm/gmsound.h:67), func_800269C0_275C0
 *   (src/port/reloc_backend_compat_shims.c:1435), syAudioPlayBGM
 *   (include/sys/audio.h), ARRAY_COUNT (include/macros.h /
 *   include/reloc_data.h), ovl32 + ovl1_VRAM (DECLARE_OVL in
 *   include/sc/scene.h), SObj/CObj helpers (decomp sys/obj.h, which the port
 *   does not shadow).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnVSRecordStartScene (adapter below) vs
 *   src/port/title_backend.c:433 NDS_SCENE_STUB.
 */

#if NDS_P2_MENU_SHELL || NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <mn/menu.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/controller.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>

#define mnVSRecordStartScene ndsBaseMNVSRecordStartScene
void ndsBaseMNVSRecordStartScene(void);

/* Exact source header decomp mn/mndata/mnvsrecord.h:33. Used at :148 before
 * its definition at :502. */
extern sb32 mnVSRecordCheckHaveFighterKind(s32 fkind);
/* Exact source header mnvsrecord.h:60,63,64,66. Used before definition
 * (:1117, :1164, :1176, :1185). */
extern f32 mnVSRecordGetAvg(s32 fkind);
extern f32 mnVSRecordGetUsePercent(s32 fkind);
extern f32 mnVSRecordGetSDPercent(s32 fkind);
extern f32 mnVSRecordGetWinPercentAgainst(s32 this_fkind, s32 against_fkind);

#include "../../decomp/BattleShip-main/decomp/src/mn/mndata/mnvsrecord.c"

#undef mnVSRecordStartScene

#if !NDS_P2_MENU_SHELL
void mnVSRecordStartScene(void)
{
    ndsBaseMNVSRecordStartScene();
}
#endif

#endif /* NDS_P2_MENU_SHELL || NDS_P2_1P_GAME */
