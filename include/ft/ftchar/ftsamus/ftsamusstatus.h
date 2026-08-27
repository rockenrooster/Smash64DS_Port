#ifndef _FTSAMUS_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_SAMUS
/* P2-3 Samus. The project fighter header already mirrors Samus's enum/passive
 * and status-var ABI; publish BattleShip's real descriptor table when she is
 * admitted and keep the inactive 16-entry compatibility table otherwise. */
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftsamus/ftsamusstatus.h"
#else
#define _FTSAMUS_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTSamusSpecialStatusDescs[] = {
    NDS_FT_STATUS_STUB16
};

#endif

#endif
