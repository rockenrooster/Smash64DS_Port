/* P2-7 item 8 companion. How-to-Play file setup.
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sccommon/scexplainfiles.c whole (37
 * lines: the file-id table and scExplainSetupFiles, which
 * battleship_scexplain.c's scene start calls at scexplain.c:696). The one
 * symbol it defines is unique to this TU, so it is included under its
 * source name; no shims, no stubs. Both reloc files it lists (SCExplainMain,
 * SCExplainGraphics) were staged by scripts/menus/stage_reloc_file.py on
 * 2026-09-04. Gated with its sibling. */

#if NDS_P2_1P_GAME

#include <ssb_types.h>
#include <reloc_data.h>
#include <sc/scene.h>

#include "../../decomp/BattleShip-main/decomp/src/sc/sccommon/scexplainfiles.c"

#endif /* NDS_P2_1P_GAME */
