#ifndef _FTNESS_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_NESS
/* P2-3 Ness. The project fighter header mirrors Ness's enum and status-var ABI;
 * publish BattleShip's exact table once his runtime is admitted. Keep the
 * compatibility table for builds where Ness is inactive. */
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftness/ftnessstatus.h"
#else
#define _FTNESS_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTNessSpecialStatusDescs[] = {
    NDS_FT_STATUS_STUB16
};

#endif

#endif
