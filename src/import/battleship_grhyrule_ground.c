/* Bounded Hyrule Castle (Hyrule) ground setup import, P2-4 stage 5.
 *
 * Mirrors the four stage wrappers beside it: this translation unit imports
 * decomp gr/grcommon/grhyrule.c verbatim, so the tornado's seven-state machine
 * is mechanically equivalent to the source, and exposes ndsGRHyruleSetupInitAll
 * for the shared grCommonSetupInitAll gate. Only built when
 * NDS_P2_STAGE_HYRULE=1.
 *
 * WHAT THIS STAGE SHARES AND WHAT IT DOES NOT. The tornado uses the
 * ground-OBSTACLE seam -- ftMainCheckAddGroundObstacle (grhyrule.c:172), the
 * same entry point Congo Jungle's barrel cannon uses and the only other caller
 * of it in the game -- so nothing new was needed at the fighter seam for this
 * stage. It is also the cheapest stage in the game on the asset side: its map
 * declares exactly two externs, StageCastle and ExternDataBank113, and both
 * were already staged and rowed for other reasons, so it adds no reloc rows at
 * all. And unlike Yoshi's Island and Congo Jungle it references no
 * llGRHyruleMap* offsets, so this file needs no reloc lvalue block.
 *
 * THE ONE HAZARD THAT IS NOT THE TORNADO. grHyruleTwisterInitVars
 * (grhyrule.c:394-401) answers a Twister map-object count of zero or above ten
 * with `while (TRUE) syDebugPrintf(...)`. That is source behaviour and stays,
 * but on DS there is no console to read it on: it presents as a stage that
 * boots to black and never returns, with no exception and nothing in a log.
 * The admission arm in battleship_grpupupu_ground.c checks the count before
 * entering and refuses with a published counter instead, which is the port's
 * only defence against a map-object import that silently goes wrong.
 */
#if NDS_P2_STAGE_HYRULE

#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <it/item.h>
#include <mn/menu.h>
#include <nds/nds_renderer.h>
#include <nds/nds_startup.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>

/* Forward declarations mirroring the other stage wrappers. */
s32 syUtilsRandIntRange(s32 range);

/* grhyrule.c calls this before defining it (grhyrule.c:172 against :432),
 * and no port header publishes it. */
sb32 grHyruleTwisterCheckGetDamageKind(GObj *ground_gobj, GObj *fighter_gobj,
                                       s32 *kind);


/* Tornado particle bank markers, decomp gr/grcommon/grhyrule.h:9-12. Address
 * identity only, exactly as the Pupupu and Yoster bank markers are: the
 * generated particle pack does not carry a Hyrule bank yet, so
 * efParticleGetLoadBankID resolves to no bank and lbParticleMakeScriptID
 * produces nothing. That is a PRESENTATION gap -- the tornado's funnel is
 * invisible -- and not a gameplay one: every state, its timing, its steering
 * and its damage come from the source and run regardless. */
intptr_t lGRHyruleParticleScriptBankLo;
intptr_t lGRHyruleParticleScriptBankHi;
intptr_t lGRHyruleParticleTextureBankLo;
intptr_t lGRHyruleParticleTextureBankHi;

void ndsBaseGRCommonSetupInitAll(void);

#include "../../decomp/BattleShip-main/decomp/src/gr/grcommon/grhyrule.c"

/* Gameplay transcription notes (all numeric behaviour is the included source,
 * cited per constant -- nothing below re-states a number):
 *
 * - Seven states dispatched by grHyruleTwisterProcUpdate (grhyrule.c:351):
 *   Sleep (:118), Wait (:128) which spawns at a random Twister map object,
 *   Summon (:155) which arms the obstacle and cues, Move (:230), Turn (:291),
 *   Stop (:307) which holds while the fighter is in nFTCommonStatusTwister and
 *   then clears the obstacle and ejects, and Subside (:334).
 * - Steering is not random: grHyruleTwisterGetLR (:193) turns the tornado
 *   toward whichever side of the stage holds more fighters.
 * - Capture test: grHyruleTwisterCheckGetDamageKind (:432), a box giving
 *   nGMHitEnvironmentTwister; the fighter enters the common Twister status.
 * - The CPU is told where it is only while it is moving or turning
 *   (grHyruleTwisterCheckGetPosition, :468, read by ft/ftcomputer.c:4927).
 * - Bounds come from the source MPGroundData, not from any port constant.
 *
 * PRESENTATION ADAPTATIONS (gameplay untouched):
 * - The tornado funnel is unbaked (see the bank markers above).
 * - The castle mesh and background draw through the port's existing DObj
 *   renderer rather than a Hyrule-specific native packet; the law 8 packet for
 *   every stage is one pipeline job (P2-4n1).
 */
void ndsGRHyruleSetupInitAll(void)
{
    ndsBaseGRCommonSetupInitAll();
}

#endif /* NDS_P2_STAGE_HYRULE */
