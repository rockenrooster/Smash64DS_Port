/*
 * Bounded BattleShip ftcommonattack100.c import for the NDS Mario/Fox
 * Attack12 -> Fox rapid-jab proof. Public symbols are remapped so the port
 * backend can guard the original path and keep full rapid-jab gameplay
 * deferred.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <sys/obj.h>

#include <nds/nds_p4_contents.h>
#if NDS_P4
#include <nds/nds_p4.h>
#endif

/* P4 builds name the source's definitions ndsSource*; the ndsBase* entry
 * points below send a P4 content to the files' P4 copy
 * (battleship_ftcommon_jab_p4.c) and everyone else here. */
#if NDS_P4
#define NDS_ATTACK100_NAME(name_) ndsSource##name_
#else
#define NDS_ATTACK100_NAME(name_) ndsBase##name_
#endif
#define ftCommonAttack100StartProcUpdate NDS_ATTACK100_NAME(FTCommonAttack100StartProcUpdate)
#define ftCommonAttack100StartSetStatus NDS_ATTACK100_NAME(FTCommonAttack100StartSetStatus)
#define ftCommonAttack100LoopKirbyUpdateEffect NDS_ATTACK100_NAME(FTCommonAttack100LoopKirbyUpdateEffect)
#define ftCommonAttack100LoopProcUpdate NDS_ATTACK100_NAME(FTCommonAttack100LoopProcUpdate)
#define ftCommonAttack100LoopProcInterrupt NDS_ATTACK100_NAME(FTCommonAttack100LoopProcInterrupt)
#define ftCommonAttack100LoopSetStatus NDS_ATTACK100_NAME(FTCommonAttack100LoopSetStatus)
#define ftCommonAttack100EndSetStatus NDS_ATTACK100_NAME(FTCommonAttack100EndSetStatus)
#define ftCommonAttack100StartCheckInterruptCommon NDS_ATTACK100_NAME(FTCommonAttack100StartCheckInterruptCommon)

void NDS_ATTACK100_NAME(FTCommonAttack100StartProcUpdate)(GObj *fighter_gobj);
void NDS_ATTACK100_NAME(FTCommonAttack100StartSetStatus)(GObj *fighter_gobj);
void NDS_ATTACK100_NAME(FTCommonAttack100LoopKirbyUpdateEffect)(FTStruct *fp);
void NDS_ATTACK100_NAME(FTCommonAttack100LoopProcUpdate)(GObj *fighter_gobj);
void NDS_ATTACK100_NAME(FTCommonAttack100LoopProcInterrupt)(GObj *fighter_gobj);
void NDS_ATTACK100_NAME(FTCommonAttack100LoopSetStatus)(GObj *fighter_gobj);
void NDS_ATTACK100_NAME(FTCommonAttack100EndSetStatus)(GObj *fighter_gobj);
sb32 NDS_ATTACK100_NAME(FTCommonAttack100StartCheckInterruptCommon)(GObj *fighter_gobj);

#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llKirbyMainMotionftKirbyAttack100Effect NDS_RELOC_LVALUE(0x1220u)

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattack100.c"

#undef ftCommonAttack100StartProcUpdate
#undef ftCommonAttack100StartSetStatus
#undef ftCommonAttack100LoopKirbyUpdateEffect
#undef ftCommonAttack100LoopProcUpdate
#undef ftCommonAttack100LoopProcInterrupt
#undef ftCommonAttack100LoopSetStatus
#undef ftCommonAttack100EndSetStatus
#undef ftCommonAttack100StartCheckInterruptCommon
#undef llKirbyMainMotionftKirbyAttack100Effect
#undef NDS_RELOC_LVALUE

#if NDS_P4
#define NDS_ATTACK100_ENTRY(n_)                                                   \
    void ndsBaseFTCommon##n_(GObj *fighter_gobj)                                  \
    {                                                                             \
        if (__builtin_expect(ftGetStruct(fighter_gobj)->nds_p4_content != 0u, 0)) \
        {                                                                         \
            ndsP4Jab##n_(fighter_gobj);                                           \
            return;                                                               \
        }                                                                         \
        ndsSourceFTCommon##n_(fighter_gobj);                                      \
    }
NDS_ATTACK100_ENTRY(Attack100StartProcUpdate)
NDS_ATTACK100_ENTRY(Attack100StartSetStatus)
NDS_ATTACK100_ENTRY(Attack100LoopProcUpdate)
NDS_ATTACK100_ENTRY(Attack100LoopProcInterrupt)
NDS_ATTACK100_ENTRY(Attack100LoopSetStatus)
NDS_ATTACK100_ENTRY(Attack100EndSetStatus)
#undef NDS_ATTACK100_ENTRY

void ndsBaseFTCommonAttack100LoopKirbyUpdateEffect(FTStruct *fp)
{
    if (__builtin_expect(fp->nds_p4_content != 0u, 0))
    {
        ndsP4JabAttack100LoopKirbyUpdateEffect(fp);
        return;
    }
    ndsSourceFTCommonAttack100LoopKirbyUpdateEffect(fp);
}

sb32 ndsBaseFTCommonAttack100StartCheckInterruptCommon(GObj *fighter_gobj)
{
    if (__builtin_expect(ftGetStruct(fighter_gobj)->nds_p4_content != 0u, 0))
    {
        return ndsP4JabAttack100StartCheckInterruptCommon(fighter_gobj);
    }
    return ndsSourceFTCommonAttack100StartCheckInterruptCommon(fighter_gobj);
}
#endif
