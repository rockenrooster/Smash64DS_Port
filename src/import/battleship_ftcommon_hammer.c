/* The fighter half of the HAMMER: holding one changes how the fighter moves.
 * Includes decomp src/ft/fthammer.c and src/ft/ftcommon/ftcommonhammerwalk.c,
 * ftcommonhammerturn.c, ftcommonhammerkneebend.c, ftcommonhammerfall.c,
 * ftcommonhammerlanding.c whole, the way battleship_ftcommon_itemuse.c
 * includes its own files.
 *
 * Fourth instance of the same shape in one night. Picking the Hammer up
 * already starts its timer and its music (battleship_ftcommon_get.c), but the
 * fighter never entered any Hammer state: eight of the callbacks the status
 * table names were weak NDS_INACTIVE_STATUS_STUBs, and ftHammerCheckHoldHammer,
 * ftHammerProcInterrupt, ftHammerUpdateStats and ftHammerSetStatusHammerWait
 * were no-op or counter shims, so "is the Hammer held?" always answered no
 * and "become Hammer" did nothing.
 *
 * Gated on NDS_P2_ITEM_CORE: with no items there is nothing to hold, and the
 * weak stubs remain the right answer. The stubs this replaces are weak, so a
 * strong definition here wins the link; the SetStatus/CheckHold/UpdateStats
 * shims are not, and are fenced by the same condition in
 * reloc_backend_compat_shims.c, as is ftHammerProcMap in
 * battleship_ftstatus_map_physics_shims.c.
 */
#if NDS_P2_ITEM_CORE

#include <ft/fighter.h>
#include <it/item.h>

/* ft/ftcommon.h and ft/ftcommondata.h CANNOT be reached the way <ef/efdef.h>
 * was in battleship_ftcommon_itemuse.c: both pull decomp ft/ftdef.h, which
 * redeclares every enumerator the port's own ft/fighter.h already defines. So
 * the four constants this file needs are transcribed below with their source
 * lines. F_CLC_DTOR32 itself is fine -- the port header already has it. */
/* decomp src/ft/ftcommon.h:224 (REGION_US; :226 is 0x48 elsewhere). Matches
 * the port's own nGMColAnimFighterHammer = 73 = 0x49. */
#define FTCOMMON_HAMMER_COLANIM_ID 0x49
/* decomp src/ft/ftcommon.h:229-231. */
#define FTCOMMON_HAMMER_TURN_FRAMES 12
#define FTCOMMON_HAMMER_SKIPLANDING_VEL_Y_MAX (-20.0F)
#define FTCOMMON_HAMMER_TURN_ROTATE_STEP (-(F_CLC_DTOR32(180.0F) / FTCOMMON_HAMMER_TURN_FRAMES)) // -0.2617994F

/* Defined in its own item TU (battleship_item_hammer.c:167), present wherever
 * the item core is. */
extern void itHammerCommonSetColAnim(GObj *item_gobj);
/* Imported whole with battleship_ftcommon_wait.c; not a port header name. */
extern sb32 ftCommonWaitCheckInputSuccess(GObj *fighter_gobj);

/* The drop-through helpers ftcommonhammerfall.c calls. The canonical common
 * names exist only under NDS_P2_DONKEY (battleship_ftcommon_pass.c); the
 * ndsBase bodies are unconditional, so route to them the way that TU's own
 * include block does. */
extern void ndsBaseFTCommonPassSetStatusParam(GObj *fighter_gobj,
                                              s32 status_id, f32 frame_begin,
                                              u32 flags);
extern sb32 ndsBaseFTCommonPassCheckInputSuccess(FTStruct *fp);
#define ftCommonPassSetStatusParam ndsBaseFTCommonPassSetStatusParam
#define ftCommonPassCheckInputSuccess ndsBaseFTCommonPassCheckInputSuccess

/* Defined further down the included sources than their first use. */
sb32 ftHammerCheckStatusHammerAll(GObj *fighter_gobj);
sb32 ftHammerCheckMotionWaitOrWalk(GObj *fighter_gobj);
f32 ftHammerGetAnimFrame(GObj *fighter_gobj);
u32 ftHammerGetStatUpdateFlags(GObj *fighter_gobj);
void ftHammerSetColAnim(GObj *fighter_gobj);
sb32 ftHammerCheckGotoHammerWait(GObj *fighter_gobj);
sb32 ftCommonHammerWalkCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonHammerTurnCheckInterruptCommon(GObj *fighter_gobj);
void ftCommonHammerFallSetStatusJump(GObj *fighter_gobj);
void ftCommonHammerLandingSetStatus(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/fthammer.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonhammerwalk.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonhammerturn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonhammerkneebend.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonhammerfall.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonhammerlanding.c"

#undef ftCommonPassSetStatusParam
#undef ftCommonPassCheckInputSuccess

#endif /* NDS_P2_ITEM_CORE */
