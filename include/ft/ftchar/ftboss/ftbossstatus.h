#ifndef _FTBOSS_STATUS_H_
#define _FTBOSS_STATUS_H_

#include <ft/ftchar/ftcompat_status.h>

/* This port header shadows decomp ft/ftchar/ftboss/ftbossstatus.h for the
 * ftmain.c include (ftmain.c:75), which builds dFTMainSpecialStatusDescs
 * from it. Without the campaign the boss never exists and the sixteen-entry
 * stub keeps that table linkable; with NDS_P2_1P_GAME the real table is
 * owned by src/import/battleship_ftboss.c, which includes the decomp header
 * by path (P2-6 step 7, 2026-09-05), so this one only declares it -- a
 * second definition here would collide at link. */
#if NDS_P2_1P_GAME
extern FTStatusDesc dFTBossSpecialStatusDescs[];
#else
FTStatusDesc dFTBossSpecialStatusDescs[] = {
    NDS_FT_STATUS_STUB16
};
#endif

#endif
