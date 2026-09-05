/* P2-6 step 7 (Boss). Master Hand core + status table.
 *
 * Source import: textual includes of
 * decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosscommon.c,
 * ftbossdefault.c, ftbosswait.c, ftbossmove.c, ftbossappear.c,
 * ftbossdeadcenter.c, ftbossdeadleft.c, ftbossdeadright.c
 * plus the status table
 * decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossstatus.h,
 * following battleship_yoshi.c / battleship_donkey.c (per-fighter behavior
 * TUs; ftboss.c data slots stay owned by battleship_ftchar_data_slots.c).
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c): this include
 * owns dFTBossSpecialStatusDescs and every symbol of the 8 core files under
 * its source name; no renamed private copies. Attack bodies live in
 * battleship_ftboss_status_{1..4}.c; their SetStatus/Proc decls below let the
 * table and the core AI (ftBossWaitDecideStatusComputer) link.
 *
 * Table publish: decomp keeps the table in ftbossstatus.h and publishes it
 * via ftmain.c:75,92 (dFTBossSpecialStatusDescs). Port ftmain is
 * reloc_backend_ftmain_runtime.c; wiring Boss kind -> this table there is the
 * orchestrator's seam (header/CFILES edits out of scope here).
 * Reloc tokens: ftboss behavior files use NO ll*FileID tokens (grep over the
 * dir: zero hits). Only wpbossbullet.c needs llBossMainMotionBullet* offsets.
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - nFTBossStatus* / nFTBossMotion* enums, FTBOSS_ATTACK_WAIT_* and the
 *   ftbossfunctions.h declarations: in include/ft/ftchar/ftboss/ftboss.h
 *   since 2026-09-05, shared by this TU and the four status TUs.
 * - FTBOSS_ATTACK_WAIT_MAX / FTBOSS_ATTACK_WAIT_LEVEL_DIV: verbatim from
 *   decomp ftboss.h:6-7 (used by ftbosscommon.c:155).
 * - DObjGetStruct: same local macro as battleship_yoshi.c:17.
 * - syUtilsRandIntRange / syUtilsRandUShort / syVector*: local externs, same
 *   as battleship_sc1pgame_runtime.c:62 and battleship_captain.c:65-68 (no
 *   port header publishes them).
 * - mpCommonUpdateFighterProjectFloor: local extern; defined by port
 *   battleship_ftstatus_map_physics_shims.c (table's Proc Map arm).
 * - ftCommonAppearProcUpdate: local extern; defined by port
 *   battleship_ftcommon_entry.c:141 (Appear row's Proc Update arm; distinct
 *   from the weak ftCommonAppearSetStatus stub, no collision).
 * - Everything else (ftMainSetStatus, ftPhysics*, ftCommonTurn*, ftParam*,
 *   mpCollision*, gmCollision*, gMPCollisionBounds) comes from port headers.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   none in this TU. ftboss.c data slots stay in battleship_ftchar_data_slots.c.
 */

#if NDS_P2_1P_GAME

#include <ft/fighter.h>
#include <gm/generic.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <sc/scene.h>
#include <ft/ftchar/ftboss/ftboss.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* Same local externs as battleship_sc1pgame_runtime.c:62 (range) and the
 * vector decls in battleship_captain.c:65-68; no port header publishes them. */
s32 syUtilsRandIntRange(s32 range);
u16 syUtilsRandUShort(void);
extern f32 syVectorNorm3D(Vec3f *dst);
extern f32 syVectorMag3D(Vec3f *src);
extern Vec3f *syVectorSub3D(Vec3f *dst, Vec3f *sub);
extern Vec3f *syVectorScale3D(Vec3f *dst, f32 scale);
extern void syVectorDiff3D(Vec3f *dst, Vec3f *a, Vec3f *b);

/* Table arms owned outside this TU (decls only; no port header carries them). */
void mpCommonUpdateFighterProjectFloor(GObj *fighter_gobj);
void ftCommonAppearProcUpdate(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosscommon.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossdefault.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbosswait.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossmove.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossappear.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossdeadcenter.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossdeadleft.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossdeadright.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftboss/ftbossstatus.h"

#endif /* NDS_P2_1P_GAME */
