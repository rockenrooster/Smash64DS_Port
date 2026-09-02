#ifndef _FTPURIN_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_PURIN
/* P2-3 Purin. The project fighter header mirrors Purin's enum and status-var ABI;
 * publish BattleShip's exact table once his runtime is admitted. Keep the
 * compatibility table for builds where Purin is inactive. */
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftpurin/ftpurinstatus.h"
#else
#define _FTPURIN_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTPurinSpecialStatusDescs[] = {
    NDS_FT_STATUS_STUB16
};

#endif

#endif
