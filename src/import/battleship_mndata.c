/* P2-7 item 4. DATA menu, source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mndata/mndata.c whole,
 * following src/import/battleship_mnsoundtest.c (same mndata/ directory,
 * same slice) and src/import/battleship_sc1pbonusstage.c (scene TU with the
 * scene entry imported as ndsBase* and re-exported under its source name, so
 * a later measured DS arena rebudget has a seam and the diff stays
 * reviewable). The adapter is a verbatim pass-through; no behaviour invented
 * here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md RECORDS/HISCORE rows):
 * - 3 options Characters/VSRecord/SoundTest, SoundTest gated on the unlock
 *   (decomp mn/mndef.h:110-120 + mndata.c:586-618: mnDataInitVars reads
 *   scene_prev for the initial cursor, then mnDataCheckSoundTestUnlocked
 *   clamps the last available option to VSRecord without the
 *   LBBACKUP_UNLOCK_MASK_SOUNDTEST bit).
 * - A/START enters the option's scene (Characters :673-681, VSRecord
 *   :683-691, SoundTest :693-701, each stopping BGM first); B returns to
 *   nSCKindModeSelect (:704-713); U/D walks with wraparound over the
 *   available range only (:715-769); 5 minutes idle returns to Title
 *   (:633-643).
 * - FuncStart replays the ModeSelect BGM when returning from one of its own
 *   children (:808-816).
 *
 * Gated on NDS_P2_1P_GAME: the Makefile has no NDS_P2_MODES_META flag
 * (verified 2026-09-05; only NDS_P2_1P_GAME gates the P2-6/P2-7 imports), so
 * this rides the campaign flag like the item-5 SoundTest TU until P2-7 mints
 * its own gate.
 *
 * Shell status: the shell requires a native module rather than a source
 * scene. NDS_MENU_SHELL_SCREEN_* covers Title/Mode/VSMode/CSS/SSS/ItemSwitch/
 * VSOptions only, and src/nds/nds_menu_shell_vsoptions.c is the port-native
 * shape for a menu screen -- no native DATA module exists and the shell
 * cannot reach this kind today. Stops at the import by design; wiring is
 * P2-7 item 9 (Menu completion), not this slice.
 *
 * Shims vs unresolved, see handoff report:
 * - Menu enum nMNDataOption* (decomp mn/mndef.h:110-120): NOT shimmed here.
 *   Enum members cannot be #ifndef-guarded; owning home is port
 *   include/mn/mndef.h (reported follow-up, blocks compile).
 * - <lb/library.h> (mndata.c:4, decomp-only header pulling lbtypes.h with a
 *   second LBBackupData beside the port's include/sc/scene.h one): NOT
 *   suppressed here, matching the item-5 sibling; compile reveals whether
 *   the port include tree carries it.
 * - ~13 ll* rows (llMNCommonFileID/llMNDataFileID + tab/frame/logo/collage/
 *   paper/text/icon sprites): left unresolved, need reloc manifest staging
 *   (offsets invented here would be fabricated data).
 * - Resolved port-side, no action: gSCManagerBackupData +
 *   LBBACKUP_UNLOCK_MASK_SOUNDTEST (include/sc/scene.h), nMNOptionTabStatus*
 *   (port include/mn/mndef.h), SYColorRGBPair (include/ssb_types.h),
 *   I_MIN_TO_TICS + ARRAY_COUNT (include/macros.h / include/reloc_data.h),
 *   nSYAudioFGMMenuSelect/nSYAudioFGMMenuScroll2/nSYAudioBGMModeSelect
 *   (include/gm/gmsound.h), func_800269C0_275C0
 *   (src/port/reloc_backend_compat_shims.c:1435), syAudioStopBGMAll/
 *   syAudioPlayBGM (include/sys/audio.h), ovl61 + ovl1_VRAM (DECLARE_OVL in
 *   include/sc/scene.h), SObj/CObj helpers (decomp sys/obj.h, which the port
 *   does not shadow).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnDataStartScene (adapter below) vs
 *   src/port/title_backend.c:416 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>
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

#define mnDataStartScene ndsBaseMNDataStartScene
void ndsBaseMNDataStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mndata/mndata.c"

#undef mnDataStartScene

void mnDataStartScene(void)
{
    ndsBaseMNDataStartScene();
}

#endif /* NDS_P2_1P_GAME */
