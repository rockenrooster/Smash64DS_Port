/* P2-7 item 5. Options menu, source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mnoption/mnoption.c whole,
 * following src/import/battleship_sc1pbonusstage.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter is a verbatim pass-through; no behaviour invented here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md OPTIONS rows):
 * - 3 rows Sound/ScreenAdjust/BackupClear, decomp mn/mndef.h:124-130.
 * - Sound row = mono/stereo toggle only (:423-430 + :808; volumes UNVERIFIED).
 * - FLASH toggle is_allow_screenflash (:810, :820 + lbtypes.h:271).
 * - Write path mnOptionWriteBackup :818-824 (screenflash + mono/stereo, then
 *   lbBackupWrite); also on the ScreenAdjust/BackupClear A arms (:878, :891),
 *   the B arm (:906) and the 5-minute idle arm (:846).
 * - ScreenAdjust effect writes screen_adjust_h/v
 *   (mnscreenadjust.c:262-265); on DS that effect is an intentional no-op
 *   delta (plan item 5), already recorded in
 *   src/import/battleship_lbbackup.c lbBackupApplyOptions. The row itself is
 *   imported, not deleted.
 *
 * Gated on NDS_P2_1P_GAME: the Makefile has no NDS_P2_MODES_META flag
 * (verified 2026-09-05), so this rides the campaign flag like the P2-6 step 5
 * bonus-stage TU until P2-7 mints its own gate.
 *
 * Shell status: the P2-1 shell reaches only native screens today -- VS Options
 * and Item Switch are intercepted to src/nds modules
 * (taskman_seam_harness.c:92-112) and ModeSelect refuses OPTION/DATA
 * (nds_menu_shell_mode_vs.c:88-90). No RunOption arm exists, so this source
 * scene is not reachable through the current shell; wiring the ModeSelect
 * OPTION entry plus the scene-table rows is P2-7 item 9 (Menu completion),
 * not this slice. Stops at the import by design.
 *
 * Shims vs unresolved, see handoff report:
 * - Menu enum nMNOptionOption* (decomp mn/mndef.h:122-132): NOT shimmed here.
 *   Enum members cannot be #ifndef-guarded; owning home is port
 *   include/mn/mndef.h (reported follow-up, blocks compile).
 * - dSYAudioSoundQuality (decomp sys/audio.c:76) + syAudioSetQuality (decomp
 *   sys/audio.h:206, audio.c:1255): NOT shimmed or stubbed here -- mixer owns
 *   them (reported follow-up; link reveals).
 * - ~14 ll* rows (llMNCommonFileID/llMNOptionFileID + tab/text/icon sprites):
 *   left unresolved, need reloc manifest staging (offsets invented here would
 *   be fabricated data).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnOptionStartScene (adapter below) vs
 *   src/port/title_backend.c:423 NDS_SCENE_STUB.
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

#define mnOptionStartScene ndsBaseMNOptionStartScene
void ndsBaseMNOptionStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnoption/mnoption.c"

#undef mnOptionStartScene

void mnOptionStartScene(void)
{
    ndsBaseMNOptionStartScene();
}

#endif /* NDS_P2_1P_GAME */
