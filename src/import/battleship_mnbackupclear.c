/* P2-7 item 5. Backup Clear screen, source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mnoption/mnbackupclear.c whole,
 * following src/import/battleship_sc1pbonusstage.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter is a verbatim pass-through; no behaviour invented here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md OPTIONS rows):
 * - 6 targets Newcomers/1PHighScore/BonusStageTime/VSRecord/Prize/AllData,
 *   rows :78-99, option enum decomp mn/mndef.h:134-147.
 * - Apply path mnBackupClearApplyOptionID :525-557: one lbBackupClear* call
 *   per target (AllDataClear also lbBackupApplyOptions), then always
 *   lbBackupCorrectErrors + lbBackupWrite + the BackupClear FGM cue.
 * - AllDataClear needs a second confirm (menuKind 2, :683-692); every other
 *   target confirms once. B returns to nSCKindOption (:604-612).
 * - All six lbBackupClear* bodies already landed 2026-09-04 in
 *   src/import/battleship_lbbackup.c, declared in include/sc/scene.h.
 *
 * Gated on NDS_P2_1P_GAME: the Makefile has no NDS_P2_MODES_META flag
 * (verified 2026-09-05), so this rides the campaign flag like the P2-6 step 5
 * bonus-stage TU until P2-7 mints its own gate.
 *
 * Shell status: same as the Options import -- no native BackupClear module
 * exists and the shell cannot reach this kind today; wiring is P2-7 item 9
 * (Menu completion), not this slice. Stops at the import by design.
 *
 * Shims vs unresolved, see handoff report:
 * - Menu enum nMNBackupClearOption* (decomp mn/mndef.h:134-147) and
 *   nSYAudioFGMOptionBackupClear (gm/gmsound.h:364): in include/mn/mndef.h
 *   and include/gm/gmsound.h since the 2026-09-05 header widening.
 * - Implicit-int definition func_ovl53_801325CC (:560, no return type): left
 *   as-is; compile reveals whether the port's C standard accepts it.
 * - The ~20 ll* rows (llMNCommon*, llMNBackupClear*, the header option file
 *   and the Yes/No palettes): staged in include/reloc_data.h (census: 0
 *   unstaged symbols in this TU).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnBackupClearStartScene (adapter below) vs
 *   src/port/title_backend.c:413 NDS_SCENE_STUB.
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

#define mnBackupClearStartScene ndsBaseMNBackupClearStartScene
void ndsBaseMNBackupClearStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnoption/mnbackupclear.c"

#undef mnBackupClearStartScene

void mnBackupClearStartScene(void)
{
    ndsBaseMNBackupClearStartScene();
}

#endif /* NDS_P2_1P_GAME */
