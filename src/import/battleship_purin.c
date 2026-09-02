/* P2-3 Jigglypuff (source: Purin) runtime state machine: BattleShip specials
 * verbatim -- Pound, Sing and Rest. Her five midair jumps and the sleep the
 * victims of Sing and she herself (after Rest) fall into are shared seams the
 * port already carries (ftcommonjumpaerial.c, ftcommonsleep.c). */
#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* Exact constants from BattleShip ft/ftchar/ftpurin/ftpurin.h. */
#define FTPURIN_JUMPAERIAL_VEL_MUL 0.8F
#define FTPURIN_POUND_VEL_BASE 65.0F
#define FTPURIN_POUND_VEL_MUL 0.92F

/* Source sibling declarations (BattleShip ftpurinfunctions.h); the
 * status-table callbacks are in ftstatus_callbacks.h. */
void ftPurinSpecialNInitStatusVars(GObj *fighter_gobj);
f32 ftPurinSpecialNGetAngle(s32 stick_y);
void ftPurinSpecialNSwitchStatusAir(GObj *fighter_gobj);
void ftPurinSpecialAirNSwitchStatusGround(GObj *fighter_gobj);
void ftPurinSpecialNSetStatus(GObj *fighter_gobj);
void ftPurinSpecialAirNSetStatus(GObj *fighter_gobj);
void ftPurinSpecialHiSwitchStatusAir(GObj *fighter_gobj);
void ftPurinSpecialAirHiSwitchStatusGround(GObj *fighter_gobj);
void ftPurinSpecialHiSetStatus(GObj *fighter_gobj);
void ftPurinSpecialAirHiSetStatus(GObj *fighter_gobj);
void ftPurinSpecialLwSwitchStatusAir(GObj *fighter_gobj);
void ftPurinSpecialAirLwSwitchStatusGround(GObj *fighter_gobj);
void ftPurinSpecialLwSetStatus(GObj *fighter_gobj);
void ftPurinSpecialAirLwSetStatus(GObj *fighter_gobj);

GObj *efManagerPurinSingMakeEffect(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftpurin/ftpurinspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftpurin/ftpurinspecialhi.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftpurin/ftpurinspeciallw.c"
