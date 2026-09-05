/* P2 Bonus-1 target (kind nITKindTarget). Verbatim-adapted from
 * decomp/BattleShip-main/decomp/src/it/itground/ittarget.c:5-55.
 *
 * The descriptor's file base is the Bonus-1 scene's own item file
 * (decomp sc1pbonusstage.c:318) with no attribute row (offset 0, kept
 * verbatim). The Bonus-stage scene provider is battleship_sc1pbonusstage.c
 * (P2-6 steps 5/6, 2026-09-05); the two scene symbols below are referenced
 * verbatim through local externs.
 *
 * Gated on NDS_P2_ITEM_CORE like the core owner; no fighter flag involved.
 * Every numeric constant below is the decomp source's own macro or literal.
 */
#if NDS_P2_ITEM_CORE

#include <it/item.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <ft/fighter.h>
#include <ef/effect.h>
#include <sys/audio.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp sc1pbonusstage.c:318 (file) and :483 (counter). The Bonus stages
 * are not ported yet; battleship_item_fflower.c:85 carries the same kind of
 * provider-less local extern for its missing symbol. */
extern void *gSC1PBonusStageItemFile;
extern void sc1PBonusStageUpdateTargetCount(void);

/* decomp ef/efmanager.h:91 (ShieldBreak) and the port's own efmanager TU
 * (FireGrind, battleship_efmanager.c:2250). */
extern LBGenerator *efManagerShieldBreakMakeEffect(Vec3f *pos);
extern LBParticle *efManagerFireGrindMakeEffect(Vec3f *pos);

/* decomp ittarget.h:8-9 verbatim. The port publishes no per-kind item procs,
 * so the source header's declarations travel with this TU, exactly as the
 * Tomato and Star files carry theirs. */
extern sb32 itTargetCommonProcDamage(GObj *item_gobj);
extern GObj* itTargetMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);

// 0x8018F130
// decomp ittarget.c:5-27 verbatim.
ITDesc dITTargetItemDesc =
{
    nITKindTarget,                      // Item Kind
    &gSC1PBonusStageItemFile,           // Pointer to item file data?
    0,                                  // Offset of item attributes in file?

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyRSca,     // Main matrix transformations
        nGCMatrixKindNull,              // Secondary matrix transformations?
        0                               // ???
    },

    nGMAttackStateOff,                  // Hitbox Update State
    NULL,                               // Proc Update
    NULL,                               // Proc Map
    NULL,                               // Proc Hit
    NULL,                               // Proc Shield
    NULL,                               // Proc Hop
    NULL,                               // Proc Set-Off
    NULL,                               // Proc Reflector
    itTargetCommonProcDamage            // Proc Damage
};

// 0x8018EE10
// decomp ittarget.c:30-40 verbatim.
sb32 itTargetCommonProcDamage(GObj* item_gobj)
{
    efManagerShieldBreakMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f);
    efManagerFireGrindMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f);

    func_800269C0_275C0(nSYAudioFGMBonus1TargetBreak);

    sc1PBonusStageUpdateTargetCount();

    return TRUE;
}

// 0x8018EE5C
// decomp ittarget.c:43-55 verbatim.
GObj* itTargetMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITTargetItemDesc, pos, vel, flags);

    if (item_gobj != NULL)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        ip->ga = nMPKineticsGround;
        ip->coll_data.floor_line_id = -1;
    }
    return item_gobj;
}

#endif /* NDS_P2_ITEM_CORE */
