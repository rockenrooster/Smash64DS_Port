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

/* decomp it/ittypes.h:15-22 and it/itmanager.c:710-714. The Poke Ball's
 * bookkeeping: which Pokemon came out last and the one before, so the roll can
 * refuse an immediate repeat. */
/* decomp sc/sc1pmode/sc1pgame.c:726. The 1P mode that reads it is P2-6, so
 * it is defined here beside its only writer until that phase owns it. */
__attribute__((used)) ub8 gSC1PGameBonusMewCatcher;

NdsITMonsterData gITManagerMonsterData;

void itManagerInitMonsterVars(void)
{
    gITManagerMonsterData.monster_curr = 0xffu;
    gITManagerMonsterData.monster_prev = 0xffu;
    gITManagerMonsterData.monsters_num =
        (u8)(nITKindMBallMonsterEnd - nITKindMBallMonsterStart);
}

/* decomp it/itmain.c:635-701 verbatim. THE MONSTER BUS -- the Poke Ball opens
 * and this decides what comes out.
 *
 * Mew is a 1-in-151 roll and only once a newcomer has been unlocked
 * (:650-656); everything else is drawn from the common twelve with the last
 * two spawns excluded, which is why the table is rebuilt each time rather than
 * indexed directly. The monsters_num countdown stopping at 10 is the source's
 * (:672-675), as is the 1P Mew-catcher bonus flag. */
GObj *itMainMakeMonster(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    GObj *monster_gobj;
    ITStruct *mp;
    s32 i, j;
    s32 index;
    Vec3f vel;

    vel.x = 0.0F;
    vel.y = 16.0F;
    vel.z = 0.0F;

    if ((gSCManagerBackupData.unlock_mask & LBBACKUP_UNLOCK_MASK_NEWCOMERS) &&
        (syUtilsRandIntRange(151) == 0) &&
        (gITManagerMonsterData.monster_curr != nITKindMew) &&
        (gITManagerMonsterData.monster_prev != nITKindMew))
    {
        index = nITKindMew;
    }
    else
    {
        for (i = j = nITKindMBallCommonStart; i <= nITKindMBallCommonEnd; i++)
        {
            if ((i != gITManagerMonsterData.monster_curr) &&
                (i != gITManagerMonsterData.monster_prev))
            {
                gITManagerMonsterData.monster_id[j - nITKindMBallMonsterStart] =
                    (u8)i;
                j++;
            }
        }
        index = gITManagerMonsterData.monster_id[
            syUtilsRandIntRange(gITManagerMonsterData.monsters_num)];
    }
    if (gITManagerMonsterData.monsters_num != 10)
    {
        gITManagerMonsterData.monsters_num--;
    }
    gITManagerMonsterData.monster_prev = gITManagerMonsterData.monster_curr;
    gITManagerMonsterData.monster_curr = (u8)index;

    monster_gobj = itManagerMakeItemKind(
        item_gobj, index, &DObjGetStruct(item_gobj)->translate.vec.f, &vel,
        (ITEM_FLAG_COLLPROJECT | ITEM_FLAG_PARENT_ITEM));

    if (monster_gobj != NULL)
    {
        mp = itGetStruct(monster_gobj);

        mp->owner_gobj = ip->owner_gobj;
        mp->team = ip->team;
        mp->player = ip->player;
        mp->handicap = ip->handicap;
        mp->player_num = ip->player_num;
        mp->display_mode = ip->display_mode;

        if (gSCManagerBattleState->game_type == nSCBattleGameType1PGame)
        {
            if ((mp->player == gSCManagerSceneData.player) &&
                (mp->kind == nITKindMew))
            {
                gSC1PGameBonusMewCatcher = TRUE;
            }
        }
    }
    return monster_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
