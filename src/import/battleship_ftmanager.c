/*
 * Fenced whole BattleShip ft/ftmanager.c import.
 *
 * Default builds keep the current DS manager seam. Set
 * NDS_IMPORT_BATTLESHIP_FTMANAGER=1 to compile and prove the original manager
 * path against the FTData/status-buffer asset slice.
 */
#include <ft/fighter.h>
#include <reloc_data.h>
#include <string.h>
#include <sys/debug.h>
#include <sys/objman.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/*
 * Keep this fenced import on the port's narrow headers. The original
 * lb/library.h pulls broad gm/lb/ft headers that conflict with the active port
 * ABI shadows; reloc_data.h supplies the one macro ftmanager.c needs.
 */
#ifndef _LIBRARY_H_
#define _LIBRARY_H_
#endif

#ifndef bzero
#define bzero(ptr, size) memset((ptr), 0, (size))
#endif

#define ftManagerMakeFighter ndsBaseFTManagerMakeFighter

GObj *ndsBaseFTManagerMakeFighter(FTDesc *desc);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftmanager.c"

#undef ftManagerMakeFighter

GObj *ftManagerMakeFighter(FTDesc *desc)
{
    return ndsBaseFTManagerMakeFighter(desc);
}
