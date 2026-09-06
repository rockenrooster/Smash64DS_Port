/* P2-7 item 5. Screen Adjust, source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mnoption/mnscreenadjust.c whole,
 * following src/import/battleship_mnoption.c (scene TU with the scene entry
 * imported as ndsBase* and re-exported under its source name). The adapter is
 * a verbatim pass-through; no behaviour invented here.
 *
 * Why it exists: the Option screen's SCREEN ADJUST row (mnoption.c:878) writes
 * nSCKindScreenAdjust into scene_curr and scmanager.c:988 dispatches it to
 * mnScreenAdjustStartScene, which until 2026-09-05 was an NDS_SCENE_STUB in
 * src/port/title_backend.c that parks forever: choosing the row hung the
 * game. The scene itself is small (guide and instruction sprites from
 * reloc file 0xf, a frame drawn with fill rectangles, the stick and D-pad
 * nudging the centre offsets within +-14, A/B/START back to Option after
 * writing the save, Z resetting, a five-minute idle return).
 *
 * DS deltas, both recorded:
 * - syVideoSetCenterOffsets / gSYVideoOffsetLeft / gSYVideoOffsetTop are the
 *   N64 VI centre offsets (decomp sys/video.c:33-42, :110). An LCD has no
 *   overscan, so the setter below only records the values the scene reads
 *   back and the save carries (screen_adjust_h/v); nothing moves on screen.
 *   This is the no-op delta battleship_lbbackup.c lbBackupApplyOptions
 *   already documents for the saved offsets.
 * - mnScreenAdjustFrameProcDisplay emits source fill rectangles. The DS
 *   source-menu fill sink handles these beside the source sprites.
 *
 * Available in the VS shell and campaign. The taskman wrapper supplies the
 * DS arena in place of the source overlay-address subtraction.
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

/* The imported sys/video.c owns all four offsets and their setter. */

#define mnScreenAdjustStartScene ndsBaseMNScreenAdjustStartScene
void ndsBaseMNScreenAdjustStartScene(void);

/* Exact source header decomp mn/mnoption/mnscreenadjust.h:9,20. The taskman
 * setup (:34-76) references both before their definitions (:118, :420). */
extern void mnScreenAdjustFuncLights(Gfx **dls);
extern void mnScreenAdjustFuncStart(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnoption/mnscreenadjust.c"

#undef mnScreenAdjustStartScene

void mnScreenAdjustStartScene(void)
{
    ndsBaseMNScreenAdjustStartScene();
}

#endif /* NDS_P2_MENU_SHELL || NDS_P2_1P_GAME */
