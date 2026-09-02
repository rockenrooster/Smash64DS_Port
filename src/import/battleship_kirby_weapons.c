/* P2-3 Kirby weapon runtime: BattleShip's Final Cutter beam verbatim. */
#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <mp/map.h>
#include <nds/nds_mpprocess_source.h>
#include <reloc_data.h>
#include <string.h>
#include <sys/audio.h>
#include <sys/taskman.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* reloc_data_symbols.us.txt:3860: the beam's WPAttributes overlap KirbyMain's
 * file-handle words at 0x8; reloc_backend_assets.c normalizes and pins them. */
uintptr_t llKirbyMainCutterWeaponAttributes = 0x08u;

#include "battleship_kirby_common.h"

/* wpmap.c, compiled into battleship_wpmanager_core.c. */
void wpMapSetGround(WPStruct *wp);
void wpMapSetAir(WPStruct *wp);

LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
sb32 wpMapTestAllCheckFloor(GObj *weapon_gobj);
sb32 wpMapTestLRWallCheckFloor(GObj *weapon_gobj);

#include "../../decomp/BattleShip-main/decomp/src/wp/wpkirby/wpkirbycutter.c"
