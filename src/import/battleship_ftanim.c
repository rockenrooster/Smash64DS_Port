/*
 * Compile-only import of BattleShip ft/ftanim.c.
 *
 * The public ftAnim* entry points still resolve to the current DS seams until
 * the fighter-data/status-manager slice can graduate naturally.
 */
#include <ft/fighter.h>
#include <sys/objman.h>

#ifndef AObjAnimAdvance
#define AObjAnimAdvance(script) ((script)++)
#endif
#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif
void gcPlayDObjAnimJoint(DObj *dobj);
void gcParseMObjMatAnimJoint(MObj *mobj);
void gcPlayMObjMatAnim(MObj *mobj);

#define ftAnimGetTargetValue battleship_ftAnimGetTargetValue
#define ftAnimParseDObjFigatree battleship_ftAnimParseDObjFigatree
#define func_ovl2_800ECCA4 battleship_func_ovl2_800ECCA4
#include "../../decomp/BattleShip-main/decomp/src/ft/ftanim.c"
#undef ftAnimGetTargetValue
#undef ftAnimParseDObjFigatree
#undef func_ovl2_800ECCA4
