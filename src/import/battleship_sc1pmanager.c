/* P2-6 step 1. 1P Game campaign driver (stage loop).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pmanager.c whole (587
 * lines: ally pick, Kirby copy, stage loop with intro/bonus/game/continue/
 * clear dispatch, challenger, ending), following battleship_sc1pbonusstage.c
 * (scene TU imported as ndsBase* and re-exported under its source name).
 *
 * Gated on NDS_P2_1P_GAME like battleship_sc1pgame_tables.c. The scene entry
 * is imported as ndsBaseSC1PManagerUpdateScene and re-exported under its
 * source name, so the dispatch in battleship_scmanager.c
 * (case nSCKind1PGame) keeps calling sc1PManagerUpdateScene while the DS
 * seam stays reviewable. Helpers (GetFighterKindsNum, GetShuffledFighterKind,
 * GetShuffledKirbyCopy, TrySetChallengers, CheckUnlockSoundTest,
 * TrySaveBackup) and data (overlays, Kirby part IDs, challenger kinds,
 * unlock kinds, totals, level drop/guard) keep their source names: the port
 * defines none of them yet, so each has exactly one definition here.
 *
 * Shims vs unresolved (by reading; no compile per owner directive):
 * - syUtilsRandIntRange: shimmed below as a local extern (same line every
 *   other import TU carries; the port links it, no header publishes it).
 * - sc1PGameStartScene: shimmed below as a local extern; PROVIDED by the
 *   step-1 pair battleship_sc1pgame_runtime.c as a DS first-stage boot
 *   (Link/Hyrule through ndsMatchConfigApply). Full N64 taskman boot is NOT
 *   imported.
 * - sc1PIntroStartScene, sc1PBonusStageStartScene, sc1PStageClearStartScene,
 *   sc1PChallengerStartScene, mnPlayers1PGameContinueStartScene,
 *   mnMessageStartScene, mnCongraStartScene, mvEndingStartScene,
 *   scStaffrollStartScene: NOT provided here; they resolve to the
 *   title_backend.c stubs (BonusStage resolves to the step-5 real import when
 *   the flag is on). A stub that stays a stub stays: intro/continue/clear/
 *   challenger/ending/staffroll/congra/message halt at ndsSceneBoundary, so
 *   the manager loop links in step 1 but does not yet run end to end. The
 *   DS-verified path in step 1 is the sc1PGameStartScene bridge alone.
 * - syDmaLoadOverlay, lbBackupWrite, ftParamGetCostumeCommonID: real via
 *   port headers (sys/dma.h, mn/menu.h, ft/fighter.h).
 */

#if NDS_P2_1P_GAME

#include <ft/fighter.h>
#include <gr/ground.h>
#include <mn/menu.h>
#include <mv/movie.h>
#include <sc/scene.h>
#include <sys/dma.h>

/* No port header publishes this; same local extern as every other import TU. */
s32 syUtilsRandIntRange(s32 range);

/* decomp ft/ftdef.h:5 verbatim. Port include/ft/fighter.h lacks it; the
 * included manager TU uses it at sc1pmanager.c:278. */
#ifndef FTCOMMON_HANDICAP_DEFAULT
#define FTCOMMON_HANDICAP_DEFAULT 9
#endif

/* Step-1 pair: DS first-stage boot in battleship_sc1pgame_runtime.c. */
void sc1PGameStartScene(void);

#define sc1PManagerUpdateScene ndsBaseSC1PManagerUpdateScene
void ndsBaseSC1PManagerUpdateScene(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pmanager.c"

#undef sc1PManagerUpdateScene

void sc1PManagerUpdateScene(void)
{
    ndsBaseSC1PManagerUpdateScene();
}

#endif /* NDS_P2_1P_GAME */
