#ifndef _FTYOSHI_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_YOSHI
/* P2-3 Yoshi. The project fighter header mirrors Yoshi's enum and status-var
 * ABI; publish BattleShip's exact fourteen-entry table once his runtime is
 * admitted. Keep the compatibility table for builds where Yoshi is inactive. */
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftyoshi/ftyoshistatus.h"
#else
#define _FTYOSHI_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTYoshiSpecialStatusDescs[] = {
    NDS_FT_STATUS_STUB16
};

#endif

#endif
