#ifndef _FTCAPTAIN_STATUS_H_
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_CAPTAIN
/* P2-3f5: same narrow wrapper shape every landed fighter uses.  The project
 * fighter header mirrors BattleShip's Captain enum/vars ABI and
 * ftstatus_callbacks.h mirrors the callback declarations; include only the
 * source descriptor table here.  Pulling ftcaptain.h itself would drag the
 * decomp's full ftdef.h into otherwise project-owned translation units and
 * create a second incompatible ABI view. */
#include <ft/ftstatus_callbacks.h>
#include "../../../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftcaptain/ftcaptainstatus.h"
#else
#define _FTCAPTAIN_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

FTStatusDesc dFTCaptainSpecialStatusDescs[] = {
    NDS_FT_STATUS_STUB16
};

#endif

#endif
