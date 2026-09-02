/* P2-3 Ness item runtime: BattleShip's PK Fire pillar verbatim.
 *
 * The spark weapon (battleship_ness_weapons.c) turns into this ITEM on
 * contact; it rides the same item manager LinkBomb graduated
 * (battleship_item_link_core.c). */
#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <it/item.h>
#include <mp/map.h>
#include <reloc_data.h>
#include <sys/audio.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* reloc_data_symbols.us.txt:3871: the ITAttributes follow the 52-byte
 * WPAttributes at the head of NessSpecial1. */
uintptr_t llNessSpecial1PKFireItemAttributes = 0x34u;

/* Source prototypes (itnesspkfire.h) and the shared landing check
 * (itmap.c, compiled into battleship_item_link_core.c). */
sb32 itNessPKFireCommonProcUpdate(GObj *item_gobj);
sb32 itNessPKFireCommonUpdateAllCheckDestroy(GObj *item_gobj);
sb32 itNessPKFireWaitProcUpdate(GObj *item_gobj);
sb32 itNessPKFireFallProcUpdate(GObj *item_gobj);
sb32 itNessPKFireWaitProcMap(GObj *item_gobj);
sb32 itNessPKFireFallProcMap(GObj *item_gobj);
sb32 itNessPKFireCommonProcDamage(GObj *item_gobj);
void itNessPKFireWaitSetStatus(GObj *item_gobj);
void itNessPKFireFallSetStatus(GObj *item_gobj);
GObj *itNessPKFireMakeItem(GObj *weapon_gobj, Vec3f *pos, Vec3f *vel);
sb32 itMapCheckLanding(GObj *item_gobj, f32 common_rebound, f32 ground_rebound,
                       void (*proc_map)(GObj *));

/* Exact US constants from it/itvars.h:475-485. */
#define ITPKFIRE_LIFETIME 100
#define ITPKFIRE_HURT_DAMAGE_MUL 3
#define ITPKFIRE_GRAVITY 0.45F
#define ITPKFIRE_TVEL 55.0F
#define ITPKFIRE_MAP_REBOUND_COMMON 0.2F
#define ITPKFIRE_MAP_REBOUND_GROUND 0.5F

#include "../../decomp/BattleShip-main/decomp/src/it/itfighter/itnesspkfire.c"
