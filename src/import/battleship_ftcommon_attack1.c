/*
 * Bounded BattleShip ftcommonattack1.c import for the NDS Mario/Fox
 * Wait -> Attack11 proof. Public symbols are remapped so the port backend can
 * guard the original path and keep follow-up/item branches deferred.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <it/item.h>
#include <sys/obj.h>

#include <nds/nds_p4_contents.h>
#if NDS_P4
#include <nds/nds_p4.h>
#endif

/* P4 builds name the source's definitions ndsSource*; the ndsBase* entry
 * points below send a P4 content to the files' P4 copy
 * (battleship_ftcommon_jab_p4.c) and everyone else here. */
#if NDS_P4
#define NDS_ATTACK1_NAME(name_) ndsSource##name_
#else
#define NDS_ATTACK1_NAME(name_) ndsBase##name_
#endif
#define ftCommonAttack11ProcUpdate NDS_ATTACK1_NAME(FTCommonAttack11ProcUpdate)
#define ftCommonAttack12ProcUpdate NDS_ATTACK1_NAME(FTCommonAttack12ProcUpdate)
#define ftCommonAttack13ProcUpdate NDS_ATTACK1_NAME(FTCommonAttack13ProcUpdate)
#define ftCommonAttack11ProcInterrupt NDS_ATTACK1_NAME(FTCommonAttack11ProcInterrupt)
#define ftCommonAttack12ProcInterrupt NDS_ATTACK1_NAME(FTCommonAttack12ProcInterrupt)
#define ftCommonAttack13ProcInterrupt NDS_ATTACK1_NAME(FTCommonAttack13ProcInterrupt)
#define ftCommonAttack11ProcStatus NDS_ATTACK1_NAME(FTCommonAttack11ProcStatus)
#define ftCommonAttack11SetStatus NDS_ATTACK1_NAME(FTCommonAttack11SetStatus)
#define ftCommonAttack12SetStatus NDS_ATTACK1_NAME(FTCommonAttack12SetStatus)
#define ftCommonAttack13SetStatus NDS_ATTACK1_NAME(FTCommonAttack13SetStatus)
#define ftCommonAttack1CheckInterruptCommon NDS_ATTACK1_NAME(FTCommonAttack1CheckInterruptCommon)
#define ftCommonAttack11CheckGoto NDS_ATTACK1_NAME(FTCommonAttack11CheckGoto)
#define ftCommonAttack12CheckGoto NDS_ATTACK1_NAME(FTCommonAttack12CheckGoto)
#define ftCommonAttack13CheckGoto NDS_ATTACK1_NAME(FTCommonAttack13CheckGoto)

void NDS_ATTACK1_NAME(FTCommonAttack11ProcUpdate)(GObj *fighter_gobj);
void NDS_ATTACK1_NAME(FTCommonAttack12ProcUpdate)(GObj *fighter_gobj);
void NDS_ATTACK1_NAME(FTCommonAttack13ProcUpdate)(GObj *fighter_gobj);
void NDS_ATTACK1_NAME(FTCommonAttack11ProcInterrupt)(GObj *fighter_gobj);
void NDS_ATTACK1_NAME(FTCommonAttack12ProcInterrupt)(GObj *fighter_gobj);
void NDS_ATTACK1_NAME(FTCommonAttack13ProcInterrupt)(GObj *fighter_gobj);
void NDS_ATTACK1_NAME(FTCommonAttack11ProcStatus)(GObj *fighter_gobj);
void NDS_ATTACK1_NAME(FTCommonAttack11SetStatus)(GObj *fighter_gobj);
void NDS_ATTACK1_NAME(FTCommonAttack12SetStatus)(GObj *fighter_gobj);
void NDS_ATTACK1_NAME(FTCommonAttack13SetStatus)(GObj *fighter_gobj);
sb32 NDS_ATTACK1_NAME(FTCommonAttack1CheckInterruptCommon)(GObj *fighter_gobj);
sb32 NDS_ATTACK1_NAME(FTCommonAttack11CheckGoto)(GObj *fighter_gobj);
sb32 NDS_ATTACK1_NAME(FTCommonAttack12CheckGoto)(GObj *fighter_gobj);
sb32 NDS_ATTACK1_NAME(FTCommonAttack13CheckGoto)(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattack1.c"

#undef ftCommonAttack11ProcUpdate
#undef ftCommonAttack12ProcUpdate
#undef ftCommonAttack13ProcUpdate
#undef ftCommonAttack11ProcInterrupt
#undef ftCommonAttack12ProcInterrupt
#undef ftCommonAttack13ProcInterrupt
#undef ftCommonAttack11ProcStatus
#undef ftCommonAttack11SetStatus
#undef ftCommonAttack12SetStatus
#undef ftCommonAttack13SetStatus
#undef ftCommonAttack1CheckInterruptCommon
#undef ftCommonAttack11CheckGoto
#undef ftCommonAttack12CheckGoto
#undef ftCommonAttack13CheckGoto

#if NDS_P4
#define NDS_ATTACK1_ENTRY(n_)                                                     \
    void ndsBaseFTCommon##n_(GObj *fighter_gobj)                                  \
    {                                                                             \
        if (__builtin_expect(ftGetStruct(fighter_gobj)->nds_p4_content != 0u, 0)) \
        {                                                                         \
            ndsP4Jab##n_(fighter_gobj);                                           \
            return;                                                               \
        }                                                                         \
        ndsSourceFTCommon##n_(fighter_gobj);                                      \
    }
#define NDS_ATTACK1_CHECK(n_)                                                     \
    sb32 ndsBaseFTCommon##n_(GObj *fighter_gobj)                                  \
    {                                                                             \
        if (__builtin_expect(ftGetStruct(fighter_gobj)->nds_p4_content != 0u, 0)) \
        {                                                                         \
            return ndsP4Jab##n_(fighter_gobj);                                    \
        }                                                                         \
        return ndsSourceFTCommon##n_(fighter_gobj);                               \
    }
NDS_ATTACK1_ENTRY(Attack11ProcUpdate)
NDS_ATTACK1_ENTRY(Attack12ProcUpdate)
NDS_ATTACK1_ENTRY(Attack13ProcUpdate)
NDS_ATTACK1_ENTRY(Attack11ProcInterrupt)
NDS_ATTACK1_ENTRY(Attack12ProcInterrupt)
NDS_ATTACK1_ENTRY(Attack13ProcInterrupt)
NDS_ATTACK1_ENTRY(Attack11ProcStatus)
NDS_ATTACK1_ENTRY(Attack11SetStatus)
NDS_ATTACK1_ENTRY(Attack12SetStatus)
NDS_ATTACK1_ENTRY(Attack13SetStatus)
NDS_ATTACK1_CHECK(Attack1CheckInterruptCommon)
NDS_ATTACK1_CHECK(Attack11CheckGoto)
NDS_ATTACK1_CHECK(Attack12CheckGoto)
NDS_ATTACK1_CHECK(Attack13CheckGoto)
#undef NDS_ATTACK1_ENTRY
#undef NDS_ATTACK1_CHECK
#endif
