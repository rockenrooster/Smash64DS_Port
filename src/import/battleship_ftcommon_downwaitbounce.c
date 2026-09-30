#include <ef/effect.h>
#include <ft/fighter.h>
#include <sys/audio.h>

#define ftCommonDownWaitProcUpdate ndsBaseFTCommonDownWaitProcUpdate
#define ftCommonDownWaitProcInterrupt ndsBaseFTCommonDownWaitProcInterrupt
#define ftCommonDownWaitSetStatus ndsBaseFTCommonDownWaitSetStatus
#define ftCommonDownBounceProcUpdate ndsBaseFTCommonDownBounceProcUpdate
#define ftCommonDownBounceCheckUpOrDown \
    ndsBaseFTCommonDownBounceCheckUpOrDown
#define ftCommonDownBounceUpdateEffects \
    ndsBaseFTCommonDownBounceUpdateEffects
#define ftCommonDownBounceSetStatus ndsBaseFTCommonDownBounceSetStatus

void ndsBaseFTCommonDownWaitProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonDownWaitProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonDownWaitSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceProcUpdate(GObj *fighter_gobj);
sb32 ndsBaseFTCommonDownBounceCheckUpOrDown(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceUpdateEffects(GObj *fighter_gobj);
void ndsBaseFTCommonDownBounceSetStatus(GObj *fighter_gobj);

#if NDS_P4_METAKNIGHT
#include <nds/nds_p4_runtime.h>
#include "../../builds/p4/meta-knight-lifecycle/nds_meta_lifecycle.generated.h"
static u16 ndsMetaDownBounceFGM(const FTStruct *fp)
{
    if (ndsP4IsMetaKnight(fp->fkind)) return NDS_META_DOWN_BOUNCE_FGM;
    return dFTCommonDataDownBounceSFX[ndsP4GetThrownScriptColumn(fp->fkind)];
}
#include "../../builds/p4/meta-knight-lifecycle/ftcommondownwaitbounce.c"
#else
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondownwaitbounce.c"
#endif

#undef ftCommonDownWaitProcUpdate
#undef ftCommonDownWaitProcInterrupt
#undef ftCommonDownWaitSetStatus
#undef ftCommonDownBounceProcUpdate
#undef ftCommonDownBounceCheckUpOrDown
#undef ftCommonDownBounceUpdateEffects
#undef ftCommonDownBounceSetStatus
