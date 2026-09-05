/* P2-6 step 5 companion. Bonus-stage file setup.
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstagefiles.c whole,
 * same wrapper pattern as battleship_sc1pbonusstage.c (import as ndsBase*,
 * re-export under the source name). Symbols are unique to this TU
 * (sSC1PBonusStageStatusBuffer, sSC1PBonusStageForceStatusBuffer,
 * sc1PBonusStageSetupFiles), so no port TU collides; the rename exists to
 * keep the seam uniform and reviewable.
 *
 * NOTE: this TU defines NO item-file symbol. The task brief's parenthetical
 * gSC1PBonusStageItemFiles (plural) exists nowhere in decomp or src (grep
 * hits: only ittarget.c:8 and sc1pbonusstage.c:318,430, both singular
 * gSC1PBonusStageItemFile). The Target item's provider is the sibling TU:
 * gSC1PBonusStageItemFile is decomp sc1pbonusstage.c:318 and
 * sc1PBonusStageUpdateTargetCount is :483, both defined by
 * battleship_sc1pbonusstage.c behind the same NDS_P2_1P_GAME gate, which is
 * what lets battleship_item_target.c:31,50,80 link. This file's job is the
 * common-file setup both bonus scenes share (sc1PBonusStageFuncStart calls
 * sc1PBonusStageSetupFiles at sc1pbonusstage.c:1008).
 *
 * No shims: every symbol this TU touches (lLBRelocTableAddr,
 * llRelocFileCount, dGMCommonFileIDs, gGMCommonFiles, lbRelocInitSetup,
 * lbRelocLoadFilesListed) is port-provided (reloc_data.h + gm/generic.h).
 * Gated on NDS_P2_1P_GAME with its sibling.
 */

#if NDS_P2_1P_GAME

#include <ssb_types.h>
#include <reloc_data.h>
#include <sc/scene.h>

#define sc1PBonusStageSetupFiles ndsBaseSC1PBonusStageSetupFiles
void ndsBaseSC1PBonusStageSetupFiles(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstagefiles.c"

#undef sc1PBonusStageSetupFiles

void sc1PBonusStageSetupFiles(void)
{
    ndsBaseSC1PBonusStageSetupFiles();
}

#endif /* NDS_P2_1P_GAME */
