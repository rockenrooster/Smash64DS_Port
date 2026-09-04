/* P2-5 Poke Ball / MBall (kind nITKindMBall). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itcommon/itmball.c:12-508.
 *
 * Art is the shared ITCommonData (reloc 0xfb) the item core already holds
 * resident -- no new asset work. The reloc tokens for the attribute row
 * (0x6E4), the model-data base (0x9430) and the open matanim joint (0x9520)
 * are decomp/BattleShip-main/include/reloc_data.us.h:3760 and :3802-:3803;
 * the port's generated reloc header publishes none of the MBall tokens, so
 * this TU owns all three uintptr_t tokens the same way
 * battleship_item_gbumper.c:28 owns GBumper's (local tokens, no generator
 * involvement, no hand-edited generated file).
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 * The monster roll calls the ported itMainMakeMonster bus
 * (battleship_item_map_core.c, declared in it/item.h) rather than
 * reimplementing the Pokemon roll. Symbols the port headers do not publish
 * yet (anim/effect managers) are referenced verbatim and listed in the task
 * report -- no values invented here.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>
#include <sys/audio.h>
#include <gr/ground.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp/BattleShip-main/include/reloc_data.us.h:3760. */
uintptr_t llITCommonDataMBallItemAttributes = 0x6E4u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3802. */
uintptr_t llITCommonDataMBallDataStart = 0x9430u;
/* decomp/BattleShip-main/include/reloc_data.us.h:3803. */
uintptr_t llITCommonDataMBallMatAnimJoint = 0x9520u;

extern void *gITManagerCommonData;

/* decomp sys/objanim.h:19 and :52. No port header in this TU's chain
 * publishes them; battleship_item_bombhei.c:50-55 carries the same kind of
 * local externs. */
extern void gcAddMObjMatAnimJoint(MObj *mobj, AObjEvent32 *matanim_joint,
                                   f32 anim_frame);
extern void gcPlayAnimAll(GObj *gobj);

/* efManagerMBallRaysMakeEffect (decomp ef/efmanager.h:124) is DEFERRED, not
 * merely undeclared. It builds its effect from dEFManagerMBallRaysEffectDesc,
 * and this port names that desc in exactly one place --
 * battleship_efmanager.c:1493, inside NDS_EF_ROSTER_DESCS_PIKACHU -- so with
 * Pikachu off the desc does not exist and the maker cannot be linked, let
 * alone called. The rays are the ball's opening flash and nothing else reads
 * them: every consumer of item_vars.mball.effect_gobj here (:423, :535) already
 * NULL-checks it, because the source's own maker returns NULL on a full effect
 * pool. Leaving the field NULL is therefore the source's own empty-pool path,
 * not a new one.
 *
 * Same deferral the item core already takes for itMainSetAppearSpin and
 * efManagerItemSpawnSwirlMakeEffect. When a landed fighter or the item core
 * owns EFCommonEffects3's rays desc, move that row out of the Pikachu list
 * into one gated on either owner -- a duplicate row would resolve the desc's
 * offsets twice. */

/* decomp itmball.h:8-29 verbatim. The port publishes no per-kind item procs,
 * so the source header's declarations travel with this TU, exactly as the
 * Tomato and Star files carry theirs. */
extern void itMBallOpenAddAnim(GObj *item_gobj);
extern void itMBallOpenClearAnim(GObj *item_gobj);
extern sb32 itMBallFallProcUpdate(GObj *item_gobj);
extern sb32 itMBallWaitProcMap(GObj *item_gobj);
extern sb32 itMBallFallProcMap(GObj *item_gobj);
extern void itMBallWaitSetStatus(GObj *item_gobj);
extern void itMBallFallSetStatus(GObj *item_gobj);
extern void itMBallHoldSetStatus(GObj *item_gobj);
extern sb32 itMBallThrownProcUpdate(GObj *item_gobj);
extern sb32 itMBallThrownProcMap(GObj *item_gobj);
extern sb32 itMBallCommonProcHit(GObj *item_gobj);
extern sb32 itMBallCommonProcReflector(GObj *item_gobj);
extern void itMBallThrownSetStatus(GObj *item_gobj);
extern void itMBallDroppedSetStatus(GObj *item_gobj);
extern sb32 itMBallOpenProcUpdate(GObj *mball_gobj);
extern sb32 itMBallOpenProcMap(GObj *item_gobj);
extern void itMBallOpenInitVars(GObj *item_gobj);
extern void itMBallOpenSetStatus(GObj *item_gobj);
extern sb32 itMBallOpenAirProcUpdate(GObj *mball_gobj);
extern sb32 itMBallOpenAirProcMap(GObj *item_gobj);
extern void itMBallOpenAirSetStatus(GObj *item_gobj);
extern GObj *itMBallMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

/* decomp itmball.c:12-34 verbatim, adapted only for the port's ITDesc shape
 * (o_attributes is const void * here, lbRelocGetFileData takes the token
 * the same way). */
ITDesc dITMBallItemDesc =
{
    nITKindMBall,                           // Item Kind
    &gITManagerCommonData,                  // Pointer to item file data?
    &llITCommonDataMBallItemAttributes,     // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindNull,                  // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0                                   // ???
    },

    nGMAttackStateOff,                      // Hitbox Update State
    itMBallFallProcUpdate,                  // Proc Update
    itMBallFallProcMap,                     // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    NULL                                    // Proc Damage
};

/* decomp itmball.c:36-121 verbatim. */
ITStatusDesc dITMBallStatusDescs[/* */] =
{
    // Status 0 (Ground Wait)
    {
        NULL,                               // Proc Update
        itMBallWaitProcMap,                 // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 1 (Air Wait Fall)
    {
        itMBallFallProcUpdate,              // Proc Update
        itMBallFallProcMap,                 // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 2 (Fighter Hold)
    {
        NULL,                               // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 3 (Fighter Throw)
    {
        itMBallThrownProcUpdate,            // Proc Update
        itMBallThrownProcMap,               // Proc Map
        itMBallCommonProcHit,               // Proc Hit
        itMBallCommonProcHit,               // Proc Shield
        itMainCommonProcHop,                // Proc Hop
        itMBallCommonProcHit,               // Proc Set-Off
        itMBallCommonProcReflector,         // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 4 (Fighter Drop)
    {
        itMBallFallProcUpdate,              // Proc Update
        itMBallThrownProcMap,               // Proc Map
        itMBallCommonProcHit,               // Proc Hit
        itMBallCommonProcHit,               // Proc Shield
        itMainCommonProcHop,                // Proc Hop
        itMBallCommonProcHit,               // Proc Set-Off
        itMBallCommonProcReflector,         // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 5 (Ground Open)
    {
        itMBallOpenProcUpdate,             // Proc Update
        itMBallOpenProcMap,                // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    },

    // Status 6 (Air Open)
    {
        itMBallOpenAirProcUpdate,             // Proc Update
        itMBallOpenAirProcMap,                // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        itMBallCommonProcReflector,         // Proc Reflector
        NULL                                // Proc Damage
    }
};

/* decomp itmball.c:129-139 verbatim. */
enum itMBallStatus
{
    nITMBallStatusWait,
    nITMBallStatusFall,
    nITMBallStatusHold,
    nITMBallStatusThrown,
    nITMBallStatusDropped,
    nITMBallStatusOpen,
    nITMBallStatusOpenAir,
    nITMBallStatusEnumCount
};

// 0x8017C690
// decomp itmball.c:148-156 verbatim.
void itMBallOpenAddAnim(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    void *matanim_joint = itGetPData(ip, &llITCommonDataMBallDataStart, &llITCommonDataMBallMatAnimJoint);

    gcAddMObjMatAnimJoint(dobj->child->child->sib_next->mobj, matanim_joint, 0.0F);
    gcPlayAnimAll(item_gobj);
}

// 0x8017C6F8
// decomp itmball.c:159-164 verbatim.
void itMBallOpenClearAnim(GObj *item_gobj)
{
    DObj *dobj = DObjGetStruct(item_gobj);

    dobj->child->sib_next->mobj->matanim_joint.event32 = NULL;
}

// 0x8017C710
// decomp itmball.c:167-178 verbatim.
sb32 itMBallFallProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITMBALL_GRAVITY, ITMBALL_TVEL);
    itVisualsUpdateSpin(item_gobj);

    dobj->child->sib_next->rotate.vec.f.z = dobj->rotate.vec.f.z;

    return FALSE;
}

// 0x8017C768
// decomp itmball.c:181-186 verbatim.
sb32 itMBallWaitProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, itMBallFallSetStatus);

    return FALSE;
}

// 0x8017C790
// decomp itmball.c:189-194 verbatim.
sb32 itMBallFallProcMap(GObj *item_gobj)
{
    itMapCheckDestroyDropped(item_gobj, ITMBALL_MAP_REBOUND_COMMON, ITMBALL_MAP_REBOUND_GROUND, itMBallWaitSetStatus);

    return FALSE;
}

// 0x8017C7C8
// decomp itmball.c:197-201 verbatim.
void itMBallWaitSetStatus(GObj *item_gobj)
{
    itMainSetGroundAllowPickup(item_gobj);
    itMainSetStatus(item_gobj, dITMBallStatusDescs, nITMBallStatusWait);
}

// 0x8017C7FC
// decomp itmball.c:204-212 verbatim.
void itMBallFallSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;

    itMapSetAir(ip);
    itMainSetStatus(item_gobj, dITMBallStatusDescs, nITMBallStatusFall);
}

// 0x8017C840
// decomp itmball.c:215-224 verbatim.
void itMBallHoldSetStatus(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    DObjGetStruct(item_gobj)->rotate.vec.f.y = 0.0F;

    ip->item_vars.mball.owner_gobj = ip->owner_gobj;

    itMainSetStatus(item_gobj, dITMBallStatusDescs, nITMBallStatusHold);
}

// 0x8017C880
// decomp itmball.c:227-238 verbatim.
sb32 itMBallThrownProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    itMainApplyGravityClampTVel(ip, ITMBALL_GRAVITY, ITMBALL_TVEL);
    itVisualsUpdateSpin(item_gobj);

    dobj->child->sib_next->rotate.vec.f.z = dobj->rotate.vec.f.z;

    return FALSE;
}

// 0x8017C8D8
// decomp itmball.c:241-252 verbatim.
sb32 itMBallThrownProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->item_vars.mball.is_rebound != FALSE)
    {
        itMapCheckLanding(item_gobj, ITMBALL_MAP_REBOUND_COMMON, ITMBALL_MAP_REBOUND_GROUND, itMBallOpenSetStatus);
    }
    else itMapCheckDestroyDropped(item_gobj, ITMBALL_MAP_REBOUND_COMMON, ITMBALL_MAP_REBOUND_GROUND, itMBallOpenSetStatus);

    return FALSE;
}

// 0x8017C94C
// decomp itmball.c:255-266 verbatim.
sb32 itMBallCommonProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;

    ip->item_vars.mball.is_rebound = TRUE;

    itMainVelSetRebound(item_gobj);

    return FALSE;
}

// 0x8017C97C
// decomp itmball.c:269-291 verbatim.
sb32 itMBallCommonProcReflector(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    FTStruct *fp;
    GObj *fighter_gobj;

    ip->attack_coll.attack_state = nGMAttackStateOff;

    ip->item_vars.mball.is_rebound = TRUE;

    itMainVelSetRebound(item_gobj);

    fighter_gobj = ip->item_vars.mball.owner_gobj;
    ip->owner_gobj = fighter_gobj;
    fp = ftGetStruct(fighter_gobj);

    ip->team = fp->team;
    ip->player = fp->player;
    ip->player_num = fp->player_num;
    ip->handicap = fp->handicap;

    return FALSE;
}

// 0x8017C9E0
// decomp itmball.c:294-298 verbatim.
void itMBallThrownSetStatus(GObj *item_gobj)
{
    itMBallOpenAddAnim(item_gobj);
    itMainSetStatus(item_gobj, dITMBallStatusDescs, nITMBallStatusThrown);
}

// 0x8017CA14
// decomp itmball.c:301-305 verbatim.
void itMBallDroppedSetStatus(GObj *item_gobj)
{
    itMBallOpenAddAnim(item_gobj);
    itMainSetStatus(item_gobj, dITMBallStatusDescs, nITMBallStatusDropped);
}

// 0x8017CA48
// decomp itmball.c:308-348 verbatim.
sb32 itMBallOpenProcUpdate(GObj *mball_gobj)
{
    ITStruct *mball_ip = itGetStruct(mball_gobj);
    ITStruct *monster_ip;
    GObj *monster_gobj;
    Vec3f vel;
    s32 unused[2];

    if (mball_ip->multi == 0)
    {
        vel.x = vel.y = vel.z = 0.0F;

        if (dITManagerForceMonsterKind == 0)
        {
            itMainMakeMonster(mball_gobj);

            return TRUE;
        }
        monster_gobj = itManagerMakeItemKind(mball_gobj, dITManagerForceMonsterKind + (nITKindMBallMonsterStart - 1), &DObjGetStruct(mball_gobj)->translate.vec.f, &vel, (ITEM_FLAG_COLLPROJECT | ITEM_FLAG_PARENT_ITEM));

        if (monster_gobj != NULL)
        {
            monster_ip = itGetStruct(monster_gobj);

            monster_ip->owner_gobj = mball_ip->owner_gobj;
            monster_ip->team = mball_ip->team;
            monster_ip->player = mball_ip->player;
            monster_ip->handicap = mball_ip->handicap;
            monster_ip->player_num = mball_ip->player_num;
            monster_ip->display_mode = mball_ip->display_mode;
        }
        return TRUE;
    }
    mball_ip->multi--;

    if (mball_ip->item_vars.mball.effect_gobj != NULL)
    {
        DObjGetStruct(mball_ip->item_vars.mball.effect_gobj)->translate.vec.f = DObjGetStruct(mball_gobj)->translate.vec.f;
    }
    return FALSE;
}

// 0x8017CB38
// decomp itmball.c:351-362 verbatim.
sb32 itMBallOpenProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (mpCollisionCheckExistLineID(ip->attach_line_id) == FALSE)
    {
        ip->is_attach_surface = FALSE;

        itMBallOpenAirSetStatus(item_gobj);
    }
    return FALSE;
}

// 0x8017CB84
// decomp itmball.c:365-406 verbatim.
void itMBallOpenInitVars(GObj *item_gobj)
{
    s32 unused[2];
    DObj *dobj = DObjGetStruct(item_gobj);
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *child;
    DObj *sibling;

    ip->physics.vel_air.x = 0.0F;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.z = 0.0F;

    child = dobj->child;
    child->flags ^= DOBJ_FLAG_HIDDEN;

    sibling = dobj->child->sib_next;
    sibling->flags ^= DOBJ_FLAG_HIDDEN;

    func_800269C0_275C0(nSYAudioFGMMBallOpen);

    ip->attach_line_id = ip->coll_data.floor_line_id;

    ip->is_attach_surface = TRUE;

    if ((ip->player != -1) && (ip->player != GMCOMMON_PLAYERS_MAX))
    {
        GObj *fighter_gobj = gSCManagerBattleState->players[ip->player].fighter_gobj;

        if (fighter_gobj != NULL)
        {
            FTStruct *fp = ftGetStruct(fighter_gobj);

            ftParamMakeRumble(fp, 8, 20);
        }
    }
    /* efManagerMBallRaysMakeEffect(&dobj->translate.vec.f) -- deferred; see
     * the note above the status-desc table. */
    ip->item_vars.mball.effect_gobj = NULL;

    itMBallOpenClearAnim(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;
    ip->attack_coll.can_reflect = FALSE;
}

// 0x8017CC88
// decomp itmball.c:409-413 verbatim.
void itMBallOpenSetStatus(GObj *item_gobj)
{
    itMBallOpenInitVars(item_gobj);
    itMainSetStatus(item_gobj, dITMBallStatusDescs, nITMBallStatusOpen);
}

// 0x8017CCBC
// decomp itmball.c:416-456 verbatim.
sb32 itMBallOpenAirProcUpdate(GObj *mball_gobj)
{
    ITStruct *mball_ip = itGetStruct(mball_gobj);
    ITStruct *monster_ip;
    GObj *monster_gobj;
    Vec3f vel;
    s32 unused[2];

    if (mball_ip->multi == 0)
    {
        vel.x = vel.y = vel.z = 0.0F;

        if (dITManagerForceMonsterKind == 0)
        {
            itMainMakeMonster(mball_gobj);

            return TRUE;
        }
        monster_gobj = itManagerMakeItemKind(mball_gobj, dITManagerForceMonsterKind + (nITKindMBallMonsterStart - 1), &DObjGetStruct(mball_gobj)->translate.vec.f, &vel, (ITEM_FLAG_COLLPROJECT | ITEM_FLAG_PARENT_ITEM));

        if (monster_gobj != NULL)
        {
            monster_ip = itGetStruct(monster_gobj);

            monster_ip->owner_gobj = mball_ip->owner_gobj;
            monster_ip->team = mball_ip->team;
            monster_ip->player = mball_ip->player;
            monster_ip->handicap = mball_ip->handicap;
            monster_ip->player_num = mball_ip->player_num;
            monster_ip->display_mode = mball_ip->display_mode;
        }
        return TRUE;
    }
    mball_ip->multi--;

    if (mball_ip->item_vars.mball.effect_gobj != NULL)
    {
        DObjGetStruct(mball_ip->item_vars.mball.effect_gobj)->translate.vec.f = DObjGetStruct(mball_gobj)->translate.vec.f;
    }
    return FALSE;
}

// 0x8017CDAC
// decomp itmball.c:459-464 verbatim.
sb32 itMBallOpenAirProcMap(GObj *item_gobj)
{
    itMapCheckDestroyDropped(item_gobj, ITMBALL_MAP_REBOUND_COMMON, ITMBALL_MAP_REBOUND_GROUND, itMBallOpenSetStatus);

    return FALSE;
}

// 0x8017CDE4
// decomp itmball.c:467-470 verbatim.
void itMBallOpenAirSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITMBallStatusDescs, nITMBallStatusOpenAir);
}

// 0x8017CE0C
// decomp itmball.c:473-508 verbatim.
GObj* itMBallMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITMBallItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        DObj *dobj = DObjGetStruct(item_gobj);
        ITStruct *ip = itGetStruct(item_gobj);
#if defined(REGION_US)
        Vec3f translate = dobj->translate.vec.f;
#endif

        dobj->child->flags = DOBJ_FLAG_HIDDEN;
        dobj->child->sib_next->flags = DOBJ_FLAG_NONE;

        gcAddXObjForDObjFixed(dobj, nGCMatrixKindTraRotRpyR, 0);
        gcAddXObjForDObjFixed(dobj->child->sib_next, 0x46, 0);

#if defined(REGION_US)
        dobj->translate.vec.f = translate;
#else
        dobj->translate.vec.f = *pos;
#endif

        ip->multi = ITMBALL_SPAWN_WAIT;

        ip->item_vars.mball.is_rebound = FALSE;

        ip->is_unused_item_bool = TRUE;

        dobj->rotate.vec.f.z = 0.0F;

        ip->arrow_gobj = ifCommonItemArrowMakeInterface(ip);
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
