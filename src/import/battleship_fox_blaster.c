#if NDS_IMPORT_BATTLESHIP_FOX_BLASTER

#include <ef/effect.h>
#include <ft/fighter.h>
#include <nds/nds_effects.h>
#include <nds/nds_startup.h>
#include <reloc_data.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifndef FTFOX_BLASTER_HOLD_JOINT
#define FTFOX_BLASTER_HOLD_JOINT 17
#endif

#ifndef FTFOX_BLASTER_SPAWN_OFF_X
#define FTFOX_BLASTER_SPAWN_OFF_X 60.0F
#endif

#ifndef WPBLASTER_VEL_X
#define WPBLASTER_VEL_X 160.0F
#endif

#ifndef WPBLASTER_ADD_SCALE_X
#define WPBLASTER_ADD_SCALE_X (16.0F / 3.0F)
#endif

#ifndef WPBLASTER_CLAMP_SCALE_X
#define WPBLASTER_CLAMP_SCALE_X (160.0F / 3.0F)
#endif

uintptr_t llFoxSpecial1BlasterWeaponAttributes;

extern f32 syUtilsArcTan2(f32 y, f32 x);
extern void gcDrawDObjDLHead1(GObj *gobj);
extern void wpDisplayMain(GObj *weapon_gobj,
                          void (*proc_display)(GObj *));
sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);
sb32 wpFoxBlasterProcUpdate(GObj *weapon_gobj);
sb32 wpFoxBlasterProcMap(GObj *weapon_gobj);
sb32 wpFoxBlasterProcHit(GObj *weapon_gobj);
sb32 wpFoxBlasterProcHop(GObj *weapon_gobj);
sb32 wpFoxBlasterProcReflector(GObj *weapon_gobj);
void ftFoxSpecialNSetStatus(GObj *fighter_gobj);
void ftFoxSpecialAirNSetStatus(GObj *fighter_gobj);

/* Keep the source impact event while using the bounded untextured DS shape. */
__attribute__((weak)) LBParticle *efManagerFoxBlasterGlowMakeEffect(Vec3f *pos)
{
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectHitElectric, pos,
                                       0.55F, 1, NULL);
    return NULL;
}

#define wpFoxBlasterMakeWeapon battleship_wpFoxBlasterMakeWeapon
#include "../../decomp/BattleShip-main/decomp/src/wp/wpfox/wpfoxblaster.c"
#undef wpFoxBlasterMakeWeapon

/* wpManager selects func_ovl3_80167618 for this descriptor, whose source
 * callback eventually reaches lbCommonDObjScaleXProcDisplay. The port's
 * shared compatibility definition of that function is deliberately a no-op:
 * effect descriptors also reference it, so making it globally draw would
 * revive unrelated source models and can double-submit their DS replacements.
 *
 * Own the seam here instead. wpDisplayMain retains the source weapon's
 * translucent/no-Z render state, while gcDrawDObjDLHead1 hands this one DObj
 * to the DS renderer. This is also the valid interpreted control for the
 * native-quad lab; the old zero-draw callback is not a performance baseline. */
static void ndsFoxBlasterProcDisplay(GObj *weapon_gobj)
{
    wpDisplayMain(weapon_gobj, gcDrawDObjDLHead1);
}

#if NDS_R2_POSITION_PROBE
/* BUGS.md "Fox's muzzle flash and laser spawn at the wrong Y", the A-vs-B
 * invariant. This wrapper is the one port seam that runs at the exact spawn
 * instant: ftfoxspecialn.c has just resolved local {60,0,0} through
 * gmCollisionGetFighterPartsWorldPosition and is handing the result here, and
 * wpFoxBlasterMakeWeapon passes that same vector on to
 * efManagerFoxBlasterGlowMakeEffect -- so `*pos` IS both the beam origin and
 * the flash position, and there is no second offset anywhere to measure.
 *
 * A is that value. B rebuilds joint 17 through func_ovl2_800EDBA4 and
 * transforms the same local {60,0,0} by parts->mtx_translate, which is the arm
 * the newly visible gun overlay uses. Source-equivalent routes; if they differ,
 * the defect is the shared fighter-parts cache and Fox must not be touched.
 *
 * ORDERING CAVEAT, stated because it changes how the flags read: A has ALREADY
 * run by the time this executes, so the three state words below are captured
 * AFTER it, not before. is_use_animlocks is unaffected either way. The other
 * two are still the reading that matters: in the animlocks-FALSE arm A walks
 * parents and only ever sets transform_update_mode, never unk_dobjtrans_0x5, so
 * unk_dobjtrans_0x5 == 0 here means A took the walking arm and never rebuilt a
 * world matrix at all. */
/* `used` on every one: their only consumer is GDB, and --gc-sections collects a
 * global with no in-ROM reader. That is not hypothetical here -- it took
 * Boundary RED on "Missing ELF symbol" once already. */
#define NDS_POSITION_PROBE_GLOBAL __attribute__((used))
NDS_POSITION_PROBE_GLOBAL f32 gNdsFoxSpawnAX, gNdsFoxSpawnAY, gNdsFoxSpawnAZ;
NDS_POSITION_PROBE_GLOBAL f32 gNdsFoxSpawnBX, gNdsFoxSpawnBY, gNdsFoxSpawnBZ;
NDS_POSITION_PROBE_GLOBAL f32 gNdsFoxSpawnDX, gNdsFoxSpawnDY, gNdsFoxSpawnDZ;
NDS_POSITION_PROBE_GLOBAL u32 gNdsFoxSpawnAnimLocks;
NDS_POSITION_PROBE_GLOBAL s32 gNdsFoxSpawnUpdateMode;
NDS_POSITION_PROBE_GLOBAL u32 gNdsFoxSpawnTrans5;
NDS_POSITION_PROBE_GLOBAL u32 gNdsFoxSpawnProbeCount;
NDS_POSITION_PROBE_GLOBAL f32 gNdsFoxSpawnWorldMtx[16];
#define NDS_FOX_POSITION_CHAIN_MAX 18u
NDS_POSITION_PROBE_GLOBAL u32 gNdsFoxSpawnChainDepth;
NDS_POSITION_PROBE_GLOBAL u32 gNdsFoxSpawnChainDObj[NDS_FOX_POSITION_CHAIN_MAX];
NDS_POSITION_PROBE_GLOBAL u32 gNdsFoxSpawnChainMode[NDS_FOX_POSITION_CHAIN_MAX];
NDS_POSITION_PROBE_GLOBAL f32 gNdsFoxSpawnChainLocal[
    NDS_FOX_POSITION_CHAIN_MAX * 16u];

static void ndsFoxBlasterProbeSpawn(GObj *fighter_gobj, Vec3f *pos)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    DObj *joint = (fp != NULL) ? fp->joints[FTFOX_BLASTER_HOLD_JOINT] : NULL;
    FTParts *parts = (joint != NULL) ? ftGetParts(joint) : NULL;
    Vec3f b;

    gNdsFoxSpawnProbeCount++;
    gNdsFoxSpawnAX = pos->x;
    gNdsFoxSpawnAY = pos->y;
    gNdsFoxSpawnAZ = pos->z;
    if (parts == NULL)
    {
        gNdsFoxSpawnUpdateMode = -1;
        return;
    }
    gNdsFoxSpawnAnimLocks = (u32)fp->is_use_animlocks;
    gNdsFoxSpawnUpdateMode = parts->transform_update_mode;
    gNdsFoxSpawnTrans5 = (u32)parts->unk_dobjtrans_0x5;

    b.x = FTFOX_BLASTER_SPAWN_OFF_X;
    b.y = 0.0F;
    b.z = 0.0F;
    if (parts->unk_dobjtrans_0x5 == 0)
    {
        func_ovl2_800EDBA4(joint);
    }
    {
        s32 row;
        s32 col;
        DObj *cursor = joint;
        u32 depth = 0u;

        for (row = 0; row < 4; row++)
        {
            for (col = 0; col < 4; col++)
            {
                gNdsFoxSpawnWorldMtx[(row * 4) + col] =
                    parts->mtx_translate[row][col];
            }
        }
        while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
               (depth < NDS_FOX_POSITION_CHAIN_MAX))
        {
            FTParts *cursor_parts = ftGetParts(cursor);

            gNdsFoxSpawnChainDObj[depth] = (u32)(uintptr_t)cursor;
            if (cursor_parts != NULL)
            {
                gNdsFoxSpawnChainMode[depth] =
                    (u32)cursor_parts->transform_update_mode;
                for (row = 0; row < 4; row++)
                {
                    for (col = 0; col < 4; col++)
                    {
                        gNdsFoxSpawnChainLocal[(depth * 16u) +
                            ((u32)row * 4u) + (u32)col] =
                            cursor_parts->unk_dobjtrans_0x10[row][col];
                    }
                }
            }
            depth++;
            cursor = cursor->parent;
        }
        gNdsFoxSpawnChainDepth = depth;
    }
    gmCollisionGetWorldPosition(parts->mtx_translate, &b);

    gNdsFoxSpawnBX = b.x;
    gNdsFoxSpawnBY = b.y;
    gNdsFoxSpawnBZ = b.z;
    gNdsFoxSpawnDX = pos->x - b.x;
    gNdsFoxSpawnDY = pos->y - b.y;
    gNdsFoxSpawnDZ = pos->z - b.z;
}
#endif /* NDS_R2_POSITION_PROBE */

GObj *wpFoxBlasterMakeWeapon(GObj *fighter_gobj, Vec3f *pos)
{
    GObj *weapon_gobj;

    gNdsFighterProjectileProofSpawnCallCount++;
#if NDS_R2_POSITION_PROBE
    ndsFoxBlasterProbeSpawn(fighter_gobj, pos);
#endif
    weapon_gobj = battleship_wpFoxBlasterMakeWeapon(fighter_gobj, pos);
    if (weapon_gobj != NULL)
    {
        weapon_gobj->proc_display = ndsFoxBlasterProcDisplay;
        gNdsFighterProjectileProofSpawnSuccessCount++;
    }
    return weapon_gobj;
}

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftfox/ftfoxspecialn.c"

#endif
