/*
 * Fenced whole BattleShip ft/ftmanager.c import.
 *
 * Default builds keep the current DS manager seam. Set
 * NDS_IMPORT_BATTLESHIP_FTMANAGER=1 to compile and prove the original manager
 * path against the FTData/status-buffer asset slice.
 */
#include <ft/fighter.h>
#include <reloc_data.h>
#include <string.h>
#include <sys/debug.h>
#include <sys/objman.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/*
 * Keep this fenced import on the port's narrow headers. The original
 * lb/library.h pulls broad gm/lb/ft headers that conflict with the active port
 * ABI shadows; reloc_data.h supplies the one macro ftmanager.c needs.
 */
#include <nds/nds_ft_pose.h>

#ifndef _LIBRARY_H_
#define _LIBRARY_H_
#endif

#ifndef bzero
#define bzero(ptr, size) memset((ptr), 0, (size))
#endif

#define ftManagerMakeFighter ndsBaseFTManagerMakeFighter
#define ftManagerDestroyFighter ndsBaseFTManagerDestroyFighter

GObj *ndsBaseFTManagerMakeFighter(FTDesc *desc);
void ndsBaseFTManagerDestroyFighter(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftmanager.c"

#undef ftManagerMakeFighter
#undef ftManagerDestroyFighter

GObj *ftManagerMakeFighter(FTDesc *desc)
{
    return ndsBaseFTManagerMakeFighter(desc);
}

/* P2-3. THE POSE SLOT IS PART OF THE FIGHTER, so it has to die with it.
 *
 * `ndsFtPoseBindBegin` claims one of a small fixed set of pose slots for a
 * fighter GObj and `ndsFtPoseUnbind` is the only way to give one back; before
 * this the sole caller was the event32 attach seam (`lbCommonAddDObjAnimJoint
 * All`), which is a RETARGET, not a death.  Every other release depended on
 * the slot's GObj address being reused or the taskman heap generation moving,
 * and the character select breaks both assumptions: it destroys and remakes a
 * preview fighter inside one scene, on one heap generation, whenever a token
 * is grabbed, dropped or a player kind changes.  A dead fighter therefore kept
 * its slot, and once all of them were held a live fighter's bind failed
 * (`gNdsFtPoseBindFull`) and it dropped to the generic AObj path -- whose pool
 * the pose engine's own budget shrank.
 *
 * Releasing here covers every death: the source calls this for CSS preview
 * rebuilds (mnplayersvs.c:2278) and for battle teardown alike. */
void ftManagerDestroyFighter(GObj *fighter_gobj)
{
    if (fighter_gobj != NULL)
    {
        ndsFtPoseUnbind(fighter_gobj);
    }
    ndsBaseFTManagerDestroyFighter(fighter_gobj);
}
