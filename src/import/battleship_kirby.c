/* P2-3 Kirby runtime state machine: BattleShip specials verbatim.
 *
 * Inhale (the vacuum, the two-body swallow, spit-as-star and Copy), Final
 * Cutter, Stone and his forward throw keep their source status bodies as the
 * behavioral authority. The copied neutral specials are in
 * battleship_kirby_copy.c, the Final Cutter beam in battleship_kirby_weapons.c
 * and the victim half of Inhale in battleship_ftcommon_capturekirby.c. */
#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <reloc_data.h>
#include <sys/audio.h>
#include <sys/develop.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#include "battleship_kirby_common.h"

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbyspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbyspecialhi.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbyspeciallw.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbythrowf.c"
