/* P2-3 Kirby Copy: the ten copied neutral specials, BattleShip verbatim.
 *
 * Each copy status calls the copied fighter's own article maker through the
 * port ABI (Mario's fireball index, Samus's charge level/release, Link's
 * boomerang, Pikachu's air jolt, Ness's PK Fire spark, Falcon's punch
 * effect); every one of those fighters' flags must be on with Kirby's. */
#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <reloc_data.h>
#include <sys/audio.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#include "battleship_kirby_common.h"

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopymariospecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopyfoxspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopysamusspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopydonkeyspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopylinkspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopypikachuspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopynessspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopypurinspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopycaptainspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbycopyyoshispecialn.c"
