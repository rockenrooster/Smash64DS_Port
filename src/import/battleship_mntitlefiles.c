/* Compile the original BattleShip Title file-list loader.
 *
 * This keeps MNTitle's file table and mnTitleLoadFiles sourced from upstream
 * while the DS backend supplies NitroFS/O2R loading behind lbReloc.
 */
#include <mn/menu.h>
#include <reloc_data.h>

#include "../../decomp/BattleShip-main/decomp/src/mn/mncommon/mntitlefiles.c"
