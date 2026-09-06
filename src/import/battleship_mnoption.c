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
 * Available in the VS shell and campaign through the source-menu pump.
 * The native Mode Select OPTION route uses the source scene registry.
 *
 * Shims vs unresolved, see handoff report:
 * - Menu enum nMNOptionOption* (decomp mn/mndef.h:122-132): in
 *   include/mn/mndef.h since the 2026-09-05 header widening.
 * - dSYAudioSoundQuality (decomp sys/audio.c:76) + syAudioSetQuality (decomp
 *   sys/audio.h:206, audio.c:1255): owned by the DS mixer since 2026-09-05
 *   (src/nds/nds_audio_bgm.c; mono centres every FGM pan at voice start).
 * - The ~14 ll* rows (llMNCommon*, llMNOption*): staged in include/reloc_data.h
 *   except llMNCommonSlashSprite (census 2026-09-05), queued for the
 *   reloc-staging agent as an --extend of MNCommon.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnOptionStartScene (adapter below) vs
 *   src/port/title_backend.c:423 NDS_SCENE_STUB.
 */

#if NDS_P2_MENU_SHELL || NDS_P2_1P_GAME

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

#endif /* NDS_P2_MENU_SHELL || NDS_P2_1P_GAME */
