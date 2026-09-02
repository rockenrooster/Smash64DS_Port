#ifndef _FTPIKACHU_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_PIKACHU
/* P2-3 Pikachu. The project fighter header mirrors Pikachu's enum/passive and
 * status-var ABI; publish BattleShip's exact eighteen-entry table once his
 * runtime is admitted. Keep the compatibility table for builds where Pikachu
 * is inactive. */
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftpikachu/ftpikachustatus.h"
#else
#define _FTPIKACHU_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTPikachuSpecialStatusDescs[] = {
    NDS_FT_STATUS_STUB16
};

#endif

#endif
