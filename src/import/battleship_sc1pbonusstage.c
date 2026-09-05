/* P2-6 step 5. Bonus 1 (Break the Targets) + Bonus 2 (Board the Platforms).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstage.c whole,
 * following battleship_scvsbattle.c (scene TU with its taskman/video setup
 * and start/update/draw functions), NOT a data-only transcription (the
 * pattern for slices of TUs whose function bodies cannot link yet; here
 * the data and the logic are inseparable -- MakeTargets indexes
 * dSC1PBonusStageTargetDescs by gkind, the platform procs index the
 * platform/bumper descs, and UpdateTargetCount drives the task HUD).
 *
 * Gated on NDS_P2_1P_GAME like the ladder tables. Provides the two symbols
 * src/import/battleship_item_target.c needs:
 * decomp sc1pbonusstage.c:318 gSC1PBonusStageItemFile (singular -- the task
 * brief's plural gSC1PBonusStageItemFiles exists nowhere in decomp or src;
 * ittarget.c:8 and battleship_item_target.c:31,50 both use the singular)
 * and decomp sc1pbonusstage.c:483 sc1PBonusStageUpdateTargetCount
 * (called by battleship_item_target.c:80).
 *
 * Rename follows battleship_ftcommon_shieldbreakfly.c / battleship_scmanager.c:
 * the scene entry is imported as ndsBase* and re-exported under its source
 * name, so a later measured DS arena rebudget (ovl6_BSS_END, DL buffers,
 * graphics/RDP sizes -- the scvsbattle precedent) has a seam and the diff
 * stays reviewable. The adapter is a verbatim pass-through; no behaviour
 * invented here.
 *
 * Shims vs unresolved, see handoff report:
 * - SCBATTLE_BONUSGAME_TASK_MAX + SC1PGAME_BONUS_MASK0_PERFECT: in
 *   include/sc/scene.h since 2026-09-05 (decomp sc/scdef.h:5,:33).
 * - GRBonusTarget and the GRStruct bonus1/bonus2/bonus3 union members: in
 *   include/gr/ground.h since 2026-09-05 (decomp gr/grvars.h:236-270,
 *   grtypes.h:62-64); the local GRBonusTarget shim was removed with them.
 * - ~60 ll* asset rows (llITBonus1ObjectHeaderFileID, 36 Bonus1 target
 *   triples, 10 Bonus2 bumper pairs, 18 Bonus2 platform rows, Bonus2Common /
 *   SC1PStageClear3 / IFCommon timer sprites): left unresolved, need reloc
 *   manifest staging like the stage-clear table's 72 rows (offsets invented
 *   here would be fabricated data).
 * - sc1PManagerCheckUnlockSoundTest (called :1235,:1247): left unresolved.
 *   It gates the SoundTest unlock message -- stubbing TRUE/FALSE would invent
 *   unlock behaviour.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   sc1PBonusStageInitBonus2 (:733) + sc1PBonusStageMakeBonus1Ground (:507)
 *   vs src/import/battleship_grpupupu_ground.c:711,716 stubs;
 *   sc1PBonusStageStartScene (adapter below) vs
 *   src/port/title_backend.c:439 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <it/item.h>
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

#define sc1PBonusStageStartScene ndsBaseSC1PBonusStageStartScene
void ndsBaseSC1PBonusStageStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstage.c"

#undef sc1PBonusStageStartScene

void sc1PBonusStageStartScene(void)
{
    ndsBaseSC1PBonusStageStartScene();
}

#endif /* NDS_P2_1P_GAME */
