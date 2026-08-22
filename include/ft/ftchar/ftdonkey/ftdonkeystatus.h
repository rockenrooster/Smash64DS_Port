#ifndef _FTDONKEY_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_DONKEY
/* P2-3: DK is deliberately not a DS-local approximation.  The project fighter
 * header mirrors BattleShip's DK enum/vars ABI and ftstatus_callbacks.h mirrors
 * the callback declarations; include only the source descriptor table here.
 * Pulling ftdonkey.h itself would drag the decomp's full ftdef.h into otherwise
 * project-owned translation units and create a second incompatible ABI view. */
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftdonkey/ftdonkeystatus.h"
#else
#define _FTDONKEY_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTDonkeySpecialStatusDescs[] = {
    NDS_FT_STATUS_STUB16
};

#endif

#endif
