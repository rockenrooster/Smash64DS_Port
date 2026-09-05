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
 * - mnScreenAdjustFrameProcDisplay draws the crosshair and frame with
 *   gDPFillRectangle into the scene's display list. The DS legacy display
 *   list scanner has no FILLRECT arm (nds_renderer_dl_core.c), so the frame
 *   is not presented; the two sprites are. Recorded as a P2-7 visual delta
 *   for the final pass (docs/VERIFYING.md item 4b); a DS-side frame is a
 *   four-line BG or OAM draw if the owner wants it.
 *
 * Gated on NDS_P2_1P_GAME with the other option scenes (the Makefile has no
 * NDS_P2_MODES_META flag). ovl25_BSS_END comes from DECLARE_OVL(25) in
 * include/sc/scene.h; the port's syTaskmanStartTask wrapper replaces the
 * arena with the DS arena, so the symbol is only ever an operand of the
 * subtraction.
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

/* The N64 centre offsets, recorded only (see the header comment). */
s16 gSYVideoOffsetLeft;
s16 gSYVideoOffsetTop;
static s16 sNdsVideoOffsetRight;
static s16 sNdsVideoOffsetBottom;

void syVideoSetCenterOffsets(s16 left, s16 right, s16 top, s16 bottom)
{
    gSYVideoOffsetLeft = left;
    sNdsVideoOffsetRight = right;
    gSYVideoOffsetTop = top;
    sNdsVideoOffsetBottom = bottom;
}

#define mnScreenAdjustStartScene ndsBaseMNScreenAdjustStartScene
void ndsBaseMNScreenAdjustStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnoption/mnscreenadjust.c"

#undef mnScreenAdjustStartScene

void mnScreenAdjustStartScene(void)
{
    ndsBaseMNScreenAdjustStartScene();
}

#endif /* NDS_P2_1P_GAME */
