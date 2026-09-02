#ifndef _FTKIRBY_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_KIRBY
/* P2-3 Kirby. The project fighter header mirrors Kirby's enum and status-var ABI;
 * publish BattleShip's exact table once his runtime is admitted. Keep the
 * compatibility table for builds where Kirby is inactive. */
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftkirby/ftkirbystatus.h"
#else
#define _FTKIRBY_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTKirbySpecialStatusDescs[] = {
    NDS_FT_STATUS_STUB16
};

#endif

#endif
