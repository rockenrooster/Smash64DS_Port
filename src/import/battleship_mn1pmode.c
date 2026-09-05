/* P2-6 step 8. 1P Mode menu (1P GAME / TRAINING / BONUS 1 / BONUS 2).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mn1pmode/mn1pmode.c whole,
 * following src/import/battleship_mnoption.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter is a verbatim pass-through; no behaviour invented here.
 *
 * The include OWNS every symbol it defines under its source name
 * (unified-owner rule, see src/import/battleship_sc1pgame_runtime.c file doc).
 * No shims, no stubs in this TU.
 *
 * Reloc: dMN1PModeFileIDs (:39) loads llMNCommonFileID + llMN1PFileID.
 * Both staged: reloc_data.h externs + NDS_MN1P_RELOC_SYMBOLS block;
 * staged 2026-09-04 by scripts/menus/stage_reloc_file.py. No unstaged file.
 *
 * Shims vs unresolved, see handoff report:
 * - Menu enum MN1PModeOptions (decomp mn/mndef.h:205-216:
 *   nMN1PModeOptionStart/1PGame/TrainingMode/Bonus1Practice/Bonus2Practice/
 *   End/EnumCount): NOT defined here. Owning home is port
 *   include/mn/mndef.h (separate header-widening task, blocks compile).
 * - nMNOptionTabStatus* (decomp mn/mndef.h:218-225): already carried by port
 *   include/mn/mndef.h, no action.
 * - ovl1_VRAM / ovl18_BSS_END (used :885): left unresolved; arena owner
 *   decides (link reveals).
 * - All other externs (gc/gm/lb/sy/sc/ft/if/audio/controller/reloc):
 *   left unresolved, no shims, no stubs; link reveals the port owner.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mn1PModeStartScene (adapter below) vs
 *   src/port/title_backend.c:412 NDS_SCENE_STUB.
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

#define mn1PModeStartScene ndsBaseMN1PModeStartScene
void ndsBaseMN1PModeStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mn1pmode/mn1pmode.c"

#undef mn1PModeStartScene

void mn1PModeStartScene(void)
{
    ndsBaseMN1PModeStartScene();
}

#endif /* NDS_P2_1P_GAME */
