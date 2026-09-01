#ifndef _FTLINK_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_LINK
/* P2-3 Link. The project fighter header mirrors Link's enum/passive/status-var
 * ABI; publish BattleShip's exact seventeen-entry table once his runtime is
 * admitted. Keep the compatibility table for builds where Link is inactive. */
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftlink/ftlinkstatus.h"
#else
#define _FTLINK_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTLinkSpecialStatusDescs[] = {
    NDS_FT_STATUS_STUB16
};

#endif

#endif
