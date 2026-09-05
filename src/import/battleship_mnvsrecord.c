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
 * Gated on NDS_P2_1P_GAME: the Makefile has no NDS_P2_MODES_META flag
 * (verified 2026-09-05; only NDS_P2_1P_GAME gates the P2-6/P2-7 imports), so
 * this rides the campaign flag like the item-5 SoundTest TU until P2-7 mints
 * its own gate.
 *
 * Shell status: the shell requires a native module rather than a source
 * scene. NDS_MENU_SHELL_SCREEN_* covers Title/Mode/VSMode/CSS/SSS/ItemSwitch/
 * VSOptions only, and src/nds/nds_menu_shell_vsoptions.c is the port-native
 * shape for a menu screen -- no native VSRecord module exists and the shell
 * cannot reach this kind today. Stops at the import by design; wiring is
 * P2-7 item 9 (Menu completion), not this slice.
 *
 * Shims vs unresolved, see handoff report:
 * - Menu enums nMNVSRecordKind* (decomp mn/mndef.h:234-243) and
 *   nMNVSRecordRankingKind* (decomp mn/mndef.h:245-258): NOT shimmed here.
 *   Enum members cannot be #ifndef-guarded; owning home is port
 *   include/mn/mndef.h (reported follow-up, blocks compile).
 * - Audio ordinals nSYAudioFGMFoxFoot (decomp gm/gmsound.h:200) and
 *   nSYAudioBGMData (decomp gm/gmsound.h:75): NOT shimmed -- ordinals belong
 *   in include/gm/gmsound.h via check-audio-ordinals (reported follow-up,
 *   blocks compile at :2018,:2043,:2067,:2087,:2110,:2137 and :2201).
 * - ~90 ll* rows (llMNVSRecordMain*/llMNDataCommon*/llMNCommonFonts*/
 *   llMNPlayersPortraits* file IDs + digit/symbol/font/portrait/icon/label/
 *   arrow sprites): left unresolved, need reloc manifest staging (offsets
 *   invented here would be fabricated data).
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

#if NDS_P2_1P_GAME

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

#include "../../decomp/BattleShip-main/decomp/src/mn/mndata/mnvsrecord.c"

#undef mnVSRecordStartScene

void mnVSRecordStartScene(void)
{
    ndsBaseMNVSRecordStartScene();
}

#endif /* NDS_P2_1P_GAME */
