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

extern void *gITManagerCommonData;

/* decomp reloc_data.us.h:3731. Zero, and the source's own comment at
 * itmain.c:588 remarks on how odd the expression below reads because of it.
 * Shadowed as an offset so `&` yields 0 the way the source's link-time
 * constant does -- the same trap that aborted Planet Zebes. */
#define llITCommonDataContainerVelocitiesY (*(uintptr_t *)(uintptr_t)0x0u)

/* decomp it/itmain.c:575-611 verbatim. A container rolls one payload out of
 * the manager's weight table and drops it. itMainSetAppearSpin has no port
 * provider yet (it/item.h:536), so the spin the source starts on the dropped
 * item is absent; the item itself, its kind roll and its velocity are the
 * source's. */
sb32 itMainMakeContainerItem(GObj *parent_gobj)
{
    s32 kind;
    Vec3f vel;

    if (gITManagerRandomWeights.weights_sum != 0)
    {
        kind = itMainGetWeightedItemKind(&gITManagerRandomWeights);

        if (kind <= nITKindCommonEnd)
        {
            vel.x = 0.0F;
            vel.y = *(f32 *)((intptr_t)&llITCommonDataContainerVelocitiesY +
                             ((uintptr_t)&((f32 *)gITManagerCommonData)[kind]));
            vel.z = 0.0F;

            if (itManagerMakeItemSetupCommon(
                    parent_gobj, kind,
                    &DObjGetStruct(parent_gobj)->translate.vec.f, &vel,
                    (ITEM_FLAG_COLLPROJECT | ITEM_FLAG_PARENT_ITEM)) != NULL)
            {
                /* itMainSetAppearSpin(parent_gobj, TRUE) -- unported. */
            }
            return TRUE;
        }
    }
    return FALSE;
}

/* decomp it/itmain.c:615-632 verbatim. Walks an item's attack-event script as
 * its multi timer counts down; the clamp at 4 back to 3 is the source's. */
void itMainUpdateAttackEvent(GObj *item_gobj, ITAttackEvent *ev)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->multi == ev[ip->event_id].timer)
    {
        ip->attack_coll.angle = ev[ip->event_id].angle;
        ip->attack_coll.damage = ev[ip->event_id].damage;
        ip->attack_coll.size = ev[ip->event_id].size;

        ip->event_id++;

        if (ip->event_id == 4)
        {
            ip->event_id = 3;
        }
    }
}

/* decomp it/itmain.c:252-262 verbatim, including the self-assignment at :259.
 * The source's own comment there wonders whether damage_player_num was meant;
 * it is transcribed as written, because a port is not the place to decide that
 * and the behaviour it produces is the behaviour the game shipped. */
void itMainCopyDamageStats(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->owner_gobj = ip->damage_gobj;
    ip->team = ip->damage_team;
    ip->player = ip->damage_port;
    ip->player_num = ip->player_num;
    ip->handicap = ip->damage_handicap;
    ip->display_mode = ip->damage_display_mode;
}

#endif /* NDS_P2_ITEM_CORE */
