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
 * - SCBATTLE_BONUSGAME_TASK_MAX + SC1PGAME_BONUS_MASK0_PERFECT: shimmed below,
 *   verbatim from decomp sc/scdef.h:5,:33 (port include/sc/scene.h lacks both).
 * - GRBonusTarget: shimmed below, verbatim from decomp gr/grvars.h:245-251
 *   (port include/gr/ground.h lacks it).
 * - GRStruct bonus1/bonus2/bonus3 union members (decomp gr/grtypes.h:62-64,
 *   gr/grvars.h:236-261): NOT shimmed here -- struct layout can only come
 *   from include/gr/ground.h (reported follow-up, blocks compile).
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

/* decomp sc/scdef.h:5 verbatim. Port include/sc/scene.h carries the bonus
 * enum but not this count; call sites (:461,494,1193,1213,1219,1247). */
#ifndef SCBATTLE_BONUSGAME_TASK_MAX
#define SCBATTLE_BONUSGAME_TASK_MAX 10
#endif

/* decomp sc/scdef.h:33 verbatim. Used by sc1PBonusStageSetBonusStats :1096. */
#ifndef SC1PGAME_BONUS_MASK0_PERFECT
#define SC1PGAME_BONUS_MASK0_PERFECT (1 << nSC1PGameBonusPerfect)
#endif

/* decomp gr/grvars.h:245-251 verbatim. Port include/gr/ground.h has no
 * GRBonusTarget; the descriptor array :19 needs the type. The union members
 * that USE it (gGRCommonStruct.bonus1/bonus2) still need the header edit. */
#ifndef NDS_SC1PBONUSSTAGE_GRBONUSTARGET_DEFINED
#define NDS_SC1PBONUSSTAGE_GRBONUSTARGET_DEFINED 1
typedef struct GRBonusTarget
{
    intptr_t start;
    intptr_t dobjdesc;
    intptr_t anim_joint;
} GRBonusTarget;
#endif

#define sc1PBonusStageStartScene ndsBaseSC1PBonusStageStartScene
void ndsBaseSC1PBonusStageStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstage.c"

#undef sc1PBonusStageStartScene

void sc1PBonusStageStartScene(void)
{
    ndsBaseSC1PBonusStageStartScene();
}

#endif /* NDS_P2_1P_GAME */
