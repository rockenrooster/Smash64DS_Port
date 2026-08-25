/* P2-3r14: VS Stock's last-stock status, imported rather than stubbed.
 *
 * `ftcommonsleep.c` was one of fifteen `ftcommon` TUs this port did not carry,
 * and the only one of the fifteen that does not belong to future-phase content.
 * Its absence was not theoretical: MEASURED 2026-08-25 on a four-CPU Dream Land
 * match driven into the Stock rule at one stock each
 * (`scripts/probe-stock-lastlife.ps1`,
 * `artifacts/verification/2026-08-25_p2-3r14-stock-prefix.txt`),
 * `ftCommonDeadCheckRebirth` was reached ONCE, at source frame 645, with
 * `stock_count == -1` and `game_rules == 2`, and went straight into the
 * two-byte `NDS_WEAK` no-op -- `ftCommonRebirthDownSetStatus` took zero hits, so
 * the Sleep arm was demonstrably the arm taken. The eliminated fighter then sat
 * in `nFTCommonStatusDeadDown` (status 0) for the remaining 1,155 frames of the
 * run with `camera_mode == nFTCameraModeDefault`.
 *
 * That last field is what made it a gameplay defect rather than a cosmetic one.
 * `gmCameraUpdateFighterBounds` (decomp gmcamera.c:270-327) skips exactly ONE
 * camera mode -- `nFTCameraModeGhost` -- and `ftCommonSleepSetStatus` is where a
 * knocked-out player acquires it. Without it every eliminated fighter stays a
 * live camera target at the position it died, which is below the stage, and
 * also keeps inflating `players_num` for `gmCameraGetPlayerNumZoomRange`. So a
 * three- or four-player Stock match dragged its camera toward the bottom blast
 * zone, and zoomed for more players than were playing, from the first KO to the
 * end of the match.
 *
 * The TU is included verbatim. Its whole external surface already existed:
 * `ftCommonRebirthDownSetStatus` and `ftMainSetStatus` are live,
 * `syUtilsRandIntRange` is live, and `ifCommonPlayerStockStealMakeInterface` is
 * compiled in `battleship_ifcommon.o` -- it was simply CALLERLESS, so
 * `--gc-sections` had removed it from the linked image. That is the P2-3r10
 * trap seen from the other side: a source function absent from the ELF because
 * the port never imported the TU that calls it.
 *
 * `ftCommonSleepProcUpdate` comes live with it (the source status table names it
 * for status 4), which is what makes the team-battle stock steal reachable, and
 * `ftCommonSleepCheckIgnorePauseMenu` -- already called from
 * `battleship_ifcommon.c` -- stops answering a constant FALSE.
 */

#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <sc/scene.h>

typedef struct alSoundEffect alSoundEffect;

alSoundEffect *func_800269C0_275C0(u16 sfx_id);
s32 syUtilsRandIntRange(s32 range);
void ftCommonRebirthDownSetStatus(GObj *fighter_gobj);
void ifCommonPlayerStockStealMakeInterface(s32 thief, s32 stolen);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonsleep.c"
