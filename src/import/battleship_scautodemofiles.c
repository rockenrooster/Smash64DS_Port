/* P2-7 item 7 companion. Attract-demo file setup.
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sccommon/scautodemofiles.c whole (38
 * lines: the file-id table and scAutoDemoSetupFiles, which
 * battleship_scautodemo.c's scene start calls at scautodemo.c:629). The one
 * symbol it defines is unique to this TU, so it is included under its
 * source name; no shims, no stubs. Every reloc file it lists resolves through
 * the port's ll*FileID tokens (reloc_data.h). Gated with its sibling. */

#if NDS_P2_1P_GAME

#include <ssb_types.h>
#include <reloc_data.h>
#include <sc/scene.h>

#include "../../decomp/BattleShip-main/decomp/src/sc/sccommon/scautodemofiles.c"

#endif /* NDS_P2_1P_GAME */
