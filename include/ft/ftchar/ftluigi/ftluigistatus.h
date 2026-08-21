#ifndef _FTLUIGI_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_LUIGI
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftluigi/ftluigistatus.h"
#else
#define _FTLUIGI_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTLuigiSpecialStatusDescs[] = {
    NDS_FT_STATUS_STUB16
};

#endif

#endif
