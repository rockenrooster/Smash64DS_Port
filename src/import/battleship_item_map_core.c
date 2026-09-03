/* P2 item map + physics core. Verbatim from
 * decomp/BattleShip-main/decomp/src/it/itmap.c and
 * decomp/BattleShip-main/decomp/src/it/itmain.c.
 *
 * The common-item files call three helpers the port headers do not publish:
 * itMainSetGroundAllowPickup (itmain.c:482), itMapTestAllCollisionFlag
 * (itmap.c:150) and itMapCheckCollideAllRebound (itmap.c:156). Only the FIRST
 * is owned here.
 *
 * The other two are already defined: battleship_item_link_core.c:1722 imports
 * the whole of decomp it/itmap.c, so defining them again would be a duplicate
 * symbol at link. They are absent from the linked ELF today only because no
 * built code references them yet and the linker drops what nothing reaches --
 * which is worth stating plainly, because checking the ELF is otherwise the
 * right way to ask what this port defines, and it answers "no" for a function
 * that is present in source and merely unreferenced.
 *
 * Gated on NDS_P2_ITEM_CORE like battleship_item_gbumper.c; no fighter flag
 * involved.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <mp/map.h>
#include <gm/gmsound.h>
#include <sys/audio.h>
#include <sc/scene.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

// 0x80172E74
// decomp src/it/itmain.c:482-496 verbatim.
void itMainSetGroundAllowPickup(GObj *item_gobj) // Airborne item becomes grounded?
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    ip->physics.vel_air.x = ip->physics.vel_air.y = ip->physics.vel_air.z = 0.0F;

    ip->is_allow_pickup = TRUE;

    ip->times_landed = 0;

    itMainResetPlayerVars(item_gobj);
    itMapSetGround(ip);
}

#endif /* NDS_P2_ITEM_CORE */
